/* init.c - initialize mdb backend */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2016 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>
#include <ac/unistd.h>
#include <ac/stdlib.h>
#include <ac/errno.h>
#include <sys/stat.h>
#include "back-mdb.h"
#include <lutil.h>
#include <ldap_rq.h>
#include "config.h"

static const struct berval mdmi_databases[] = {
	BER_BVC("ad2i"),
	BER_BVC("dn2i"),
	BER_BVC("id2e"),
	BER_BVNULL
};

static int
mdb_id_compare( const MDB_val *a, const MDB_val *b )
{
	return mdbx_cmp2int( *(ID *)a->mv_data, *(ID *)b->mv_data );
}

#if MDBX_MODE_ENABLED
static void
mdbx_debug(int type, const char *function, int line, const char *msg, va_list args)
{
	int level = 0;

	if (type & MDBX_DBG_ASSERT)
		level |= LDAP_DEBUG_ANY;

	if (type & MDBX_DBG_PRINT)
		level |= LDAP_DEBUG_NONE;

	if (type & MDBX_DBG_TRACE)
		level |= LDAP_DEBUG_TRACE;

	if (LogTest(level))
		lutil_debug_va(msg, args);
}
#endif /* MDBX_MODE_ENABLED */

#ifdef MDBX_LIFORECLAIM
/* perform kick/kill a laggard readers */
static int
mdb_oom_handler(MDB_env *env, int pid, void* thread_id, size_t txnid, unsigned gap, int retry)
{
	uint64_t now_ns = ldap_now_ns();
	struct mdb_info *mdb = mdb_env_get_userctx(env);

	if (retry < 0) {
		double elapsed = (now_ns - mdb->mi_oom_timestamp_ns) * 1e-9;
		const char* suffix = "s";
		if (elapsed < 1) {
			elapsed *= 1000;
			suffix = "ms";
		}

		if (gap) {
			Debug( LDAP_DEBUG_ANY, "oom-handler: done, txnid %zu, got/forward %u, elapsed/waited %.3f%s, %d retries\n",
				   txnid, gap, elapsed, suffix, -retry );
		} else {
			Debug( LDAP_DEBUG_ANY, "oom-handler: unable, txnid %zu, elapsed/waited %.3f%s, %d retries\n",
				   txnid, elapsed, suffix, -retry );
		}

		mdb->mi_oom_timestamp_ns = 0;
		return 0;
	}

	if (! mdb->mi_oom_flags)
		return -1;
	else if (! mdb->mi_oom_timestamp_ns) {
		mdb->mi_oom_timestamp_ns = now_ns;
		Debug( LDAP_DEBUG_ANY, "oom-handler: begin, txnid %zu lag %u\n",
			   txnid, gap );
	}

	if ( (mdb->mi_oom_flags & MDBX_OOM_KILL) && pid != getpid() ) {
		if ( kill( pid, SIGKILL ) == 0 ) {
			Debug( LDAP_DEBUG_ANY, "oom-handler: txnid %zu lag %u, KILLED pid %i\n",
				   txnid, gap, pid );
			ldap_pvt_thread_yield();
			return 2;
		}
		if ( errno == ESRCH )
			return 0;
		Debug( LDAP_DEBUG_ANY, "oom-handler: retry %d, UNABLE kill pid %i: %s\n",
			   retry, pid, STRERROR(errno) );
	}

	if ( (mdb->mi_oom_flags & MDBX_OOM_YIELD) && ! slapd_shutdown ) {
		if (retry == 0)
			Debug( LDAP_DEBUG_ANY, "oom-handler: txnid %zu lag %u, WAITING\n",
				   txnid, gap );
		if (retry < 42)
			ldap_pvt_thread_yield();
		else
			usleep(retry);
		return 0;
	}

	mdb->mi_oom_timestamp_ns = 0;
	return -1;
}
#endif /* MDBX_LIFORECLAIM */

static int
mdb_db_init( BackendDB *be, ConfigReply *cr )
{
	struct mdb_info	*mdb;
	int rc;

	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(mdb_db_init) ": Initializing mdb database\n" );

#if MDBX_MODE_ENABLED
	unsigned flags = mdbx_setup_debug(MDBX_DBG_DNT, mdbx_debug, MDBX_DBG_DNT);
	flags &= ~(MDBX_DBG_TRACE | MDBX_DBG_EXTRA | MDBX_DBG_ASSERT);
	if (reopenldap_mode_check())
		flags |=
#	if LDAP_DEBUG > 2
				MDBX_DBG_TRACE | MDBX_DBG_EXTRA |
#	endif /* LDAP_DEBUG > 2 */
				MDBX_DBG_ASSERT;

	mdbx_setup_debug(flags, (MDBX_debug_func*) MDBX_DBG_DNT, MDBX_DBG_DNT);
#endif /* MDBX_MODE_ENABLED */

	/* allocate backend-database-specific stuff */
	mdb = (struct mdb_info *) ch_calloc( 1, sizeof(struct mdb_info) );

	/* DBEnv parameters */
	mdb->mi_dbenv_home = ch_strdup( SLAPD_DEFAULT_DB_DIR );
	mdb->mi_dbenv_flags = 0;
	mdb->mi_dbenv_mode = SLAPD_DEFAULT_DB_MODE;

	mdb->mi_search_stack_depth = DEFAULT_SEARCH_STACK_DEPTH;
	mdb->mi_search_stack = NULL;

	mdb->mi_mapsize = DEFAULT_MAPSIZE;
	mdb->mi_rtxn_size = DEFAULT_RTXN_SIZE;

	be->be_private = mdb;
	be->be_cf_ocs = be->bd_info->bi_cf_ocs;

#ifndef MDB_MULTIPLE_SUFFIXES
	SLAP_DBFLAGS( be ) |= SLAP_DBFLAG_ONE_SUFFIX;
#endif

	slap_backtrace_set_dir( mdb->mi_dbenv_home );

	ldap_pvt_thread_mutex_init( &mdb->mi_ads_mutex );

	rc = mdb_monitor_db_init( be );

	return rc;
}

static int
mdb_db_close( BackendDB *be, ConfigReply *cr );

static int
mdb_db_open( BackendDB *be, ConfigReply *cr )
{
	int rc, i;
	struct mdb_info *mdb = (struct mdb_info *) be->be_private;
	struct stat stat1;
	uint32_t flags;
	char *dbhome;
	MDB_txn *txn;

	if ( be->be_suffix == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(mdb_db_open) ": need suffix.\n" );
		return -1;
	}

	Debug( LDAP_DEBUG_ARGS,
		LDAP_XSTRING(mdb_db_open) ": \"%s\"\n",
		be->be_suffix[0].bv_val );

	/* Check existence of dbenv_home. Any error means trouble */
	rc = stat( mdb->mi_dbenv_home, &stat1 );
	if( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(mdb_db_open) ": database \"%s\": "
			"cannot access database directory \"%s\" (%d).\n",
			be->be_suffix[0].bv_val, mdb->mi_dbenv_home, errno );
		return -1;
	}

	/* mdb is always clean */
	be->be_flags |= SLAP_DBFLAG_CLEAN;

	rc = mdb_env_create( &mdb->mi_dbenv );
	if( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(mdb_db_open) ": database \"%s\": "
			"mdb_env_create failed: %s (%d).\n",
			be->be_suffix[0].bv_val, mdb_strerror(rc), rc );
		goto fail;
	}

	mdb_env_set_userctx(mdb->mi_dbenv, mdb);

	if ( mdb->mi_readers ) {
		rc = mdb_env_set_maxreaders( mdb->mi_dbenv, mdb->mi_readers );
		if( rc != 0 ) {
			Debug( LDAP_DEBUG_ANY,
				LDAP_XSTRING(mdb_db_open) ": database \"%s\": "
				"mdb_env_set_maxreaders failed: %s (%d).\n",
				be->be_suffix[0].bv_val, mdb_strerror(rc), rc );
			goto fail;
		}
	}

	rc = mdb_env_set_mapsize( mdb->mi_dbenv, mdb->mi_mapsize );
	if( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(mdb_db_open) ": database \"%s\": "
			"mdb_env_set_mapsize failed: %s (%d).\n",
			be->be_suffix[0].bv_val, mdb_strerror(rc), rc );
		goto fail;
	}

	rc = mdb_env_set_maxdbs( mdb->mi_dbenv, MDB_INDICES );
	if( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(mdb_db_open) ": database \"%s\": "
			"mdb_env_set_maxdbs failed: %s (%d).\n",
			be->be_suffix[0].bv_val, mdb_strerror(rc), rc );
		goto fail;
	}

#ifdef MDBX_LIFORECLAIM
#if MDBX_MODE_ENABLED
	rc = mdbx_env_set_syncbytes( mdb->mi_dbenv, mdb->mi_txn_cp_kbyte * 1024ul);
#else
	rc = mdb_env_set_syncbytes( mdb->mi_dbenv, mdb->mi_txn_cp_kbyte * 1024ul);
#endif /* MDBX_MODE_ENABLED */
	if( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(mdb_db_open) ": database \"%s\": "
			"mdb_env_set_sync_threshold failed: %s (%d).\n",
			be->be_suffix[0].bv_val, mdb_strerror(rc), rc );
		goto fail;
	}

#if MDBX_MODE_ENABLED
	mdbx_env_set_oomfunc( mdb->mi_dbenv, mdb_oom_handler );
#else
	mdb_env_set_oomfunc( mdb->mi_dbenv, mdb_oom_handler );
#endif /* MDBX_MODE_ENABLED */

	if ( (slapMode & SLAP_SERVER_MODE) && SLAP_MULTIMASTER(be) &&
			((MDBX_OOM_YIELD & mdb->mi_oom_flags) == 0 || mdb->mi_renew_lag == 0)) {
		snprintf( cr->msg, sizeof(cr->msg), "database \"%s\": "
			"for properly operation in multi-master mode"
			" 'oom-handler yield' and 'dreamcatcher' are recommended",
			be->be_suffix[0].bv_val );
		Debug( LDAP_DEBUG_ANY, LDAP_XSTRING(mdb_db_open) ": %s\n", cr->msg );
	}
#endif /* MDBX_LIFORECLAIM */

#ifdef HAVE_EBCDIC
	strcpy( path, mdb->mi_dbenv_home );
	__atoe( path );
	dbhome = path;
#else
	dbhome = mdb->mi_dbenv_home;
#endif

	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(mdb_db_open) ": database \"%s\": "
		"dbenv_open(%s).\n",
		be->be_suffix[0].bv_val, mdb->mi_dbenv_home);

	flags = mdb->mi_dbenv_flags;

#ifdef MDBX_PAGEPERTURB
	if (reopenldap_mode_check())
		flags |= MDBX_PAGEPERTURB;
#endif

	if ( slapMode & SLAP_TOOL_QUICK )
		flags |= MDB_NOSYNC|MDB_WRITEMAP;

	if ( slapMode & SLAP_TOOL_READONLY)
		flags |= MDB_RDONLY;

	rc = mdb_env_open( mdb->mi_dbenv, dbhome,
			flags, mdb->mi_dbenv_mode );

	if ( rc ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(mdb_db_open) ": database \"%s\" cannot be opened: %s (%d). "
			"Restore from backup!\n",
			be->be_suffix[0].bv_val, mdb_strerror(rc), rc );
		goto fail;
	}

	rc = mdb_txn_begin( mdb->mi_dbenv, NULL, flags & MDB_RDONLY, &txn );
	if ( rc ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(mdb_db_open) ": database \"%s\" cannot be opened: %s (%d). "
			"Restore from backup!\n",
			be->be_suffix[0].bv_val, mdb_strerror(rc), rc );
		goto fail;
	}

	/* open (and create) main databases */
	for( i = 0; mdmi_databases[i].bv_val; i++ ) {
		flags = MDB_INTEGERKEY;
		if( i == MDB_ID2ENTRY ) {
			if ( !(slapMode & (SLAP_TOOL_READMAIN|SLAP_TOOL_READONLY) ))
				flags |= MDB_CREATE;
		} else {
			if ( i == MDB_DN2ID )
				flags |= MDB_DUPSORT;
			if ( !(slapMode & SLAP_TOOL_READONLY) )
				flags |= MDB_CREATE;
		}

		rc = mdb_dbi_open( txn,
			mdmi_databases[i].bv_val,
			flags,
			&mdb->mi_dbis[i] );

		if ( rc != 0 ) {
			snprintf( cr->msg, sizeof(cr->msg), "database \"%s\": "
				"mdb_dbi_open(%s/%s) failed: %s (%d).",
				be->be_suffix[0].bv_val,
				mdb->mi_dbenv_home, mdmi_databases[i].bv_val,
				mdb_strerror(rc), rc );
			Debug( LDAP_DEBUG_ANY,
				LDAP_XSTRING(mdb_db_open) ": %s\n",
				cr->msg );
			goto fail;
		}

		if ( i == MDB_ID2ENTRY )
			mdb_set_compare( txn, mdb->mi_dbis[i], mdb_id_compare );
		else if ( i == MDB_DN2ID ) {
			MDB_cursor *mc;
			MDB_val key, data;
			mdb_set_dupsort( txn, mdb->mi_dbis[i], mdb_dup_compare );
			/* check for old dn2id format */
			rc = mdb_cursor_open( txn, mdb->mi_dbis[i], &mc );
			/* first record is always ID 0 */
			rc = mdb_cursor_get( mc, &key, &data, MDB_FIRST );
			if ( rc == 0 ) {
				rc = mdb_cursor_get( mc, &key, &data, MDB_NEXT );
				if ( rc == 0 ) {
					int len;
					unsigned char *ptr;
					ptr = data.mv_data;
					len = (ptr[0] & 0x7f) << 8 | ptr[1];
					if (data.mv_size < 2*len + 4 + 2*sizeof(ID)) {
						snprintf( cr->msg, sizeof(cr->msg),
						"database \"%s\": DN index needs upgrade, "
						"run \"slapindex entryDN\".",
						be->be_suffix[0].bv_val );
						Debug( LDAP_DEBUG_ANY,
							LDAP_XSTRING(mdb_db_open) ": %s\n",
							cr->msg );
						if ( !(slapMode & SLAP_TOOL_READMAIN ))
							rc = LDAP_OTHER;
						mdb->mi_flags |= MDB_NEED_UPGRADE;
					}
				}
			}
			mdb_cursor_close( mc );
			if ( rc == LDAP_OTHER )
				goto fail;
		}
	}

	rc = mdb_ad_read( mdb, txn );
	if ( rc ) {
		mdb_txn_abort( txn );
		goto fail;
	}

	/* slapcat doesn't need indexes. avoid a failure if
	 * a configured index wasn't created yet.
	 */
	if ( !(slapMode & SLAP_TOOL_READONLY) ) {
		rc = mdb_attr_dbs_open( be, txn, cr );
		if ( rc ) {
			mdb_txn_abort( txn );
			goto fail;
		}
	}

	rc = mdb_txn_commit(txn);
	if ( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY,
			LDAP_XSTRING(mdb_db_open) ": database %s: "
			"txn_commit failed: %s (%d)\n",
			be->be_suffix[0].bv_val, mdb_strerror(rc), rc );
		goto fail;
	}

	/* monitor setup */
	rc = mdb_monitor_db_open( be );
	if ( rc != 0 ) {
		goto fail;
	}

	mdb->mi_flags |= MDB_IS_OPEN;

	return 0;

fail:
	mdb_db_close( be, NULL );
	return rc;
}

static int
mdb_db_close( BackendDB *be, ConfigReply *cr )
{
	int rc;
	struct mdb_info *mdb = (struct mdb_info *) be->be_private;

	/* monitor handling */
	(void)mdb_monitor_db_close( be );

	mdb->mi_flags &= ~MDB_IS_OPEN;

	if( mdb->mi_dbenv ) {
		mdb_reader_flush( mdb->mi_dbenv );
	}

	if ( mdb->mi_dbenv ) {
		if ( mdb->mi_dbis[0] ) {
			int i;

			mdb_attr_dbs_close( mdb );
			for ( i=0; i<MDB_NDB; i++ )
				mdb_dbi_close( mdb->mi_dbenv, mdb->mi_dbis[i] );

			/* force a sync, but not if we were ReadOnly,
			 * and not in Quick mode.
			 */
			if (!(slapMode & (SLAP_TOOL_QUICK|SLAP_TOOL_READONLY))) {
				rc = mdb_env_sync( mdb->mi_dbenv, 1 );
				if( rc != 0 ) {
					Debug( LDAP_DEBUG_ANY,
						"mdb_db_close: database \"%s\": "
						"mdb_env_sync failed: %s (%d).\n",
						be->be_suffix[0].bv_val, mdb_strerror(rc), rc );
				}
			}
		}

		mdb_env_close( mdb->mi_dbenv );
		mdb->mi_dbenv = NULL;
	}

	if ( mdb->mi_search_stack ) {
		ch_free( mdb->mi_search_stack );
		mdb->mi_search_stack = NULL;
	}

	return 0;
}

static int
mdb_db_destroy( BackendDB *be, ConfigReply *cr )
{
	struct mdb_info *mdb = (struct mdb_info *) be->be_private;

	/* stop and remove checkpoint task */
	if ( mdb->mi_txn_cp_task ) {
		struct re_s *re = mdb->mi_txn_cp_task;
		mdb->mi_txn_cp_task = NULL;
		ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
		if ( ldap_pvt_runqueue_isrunning( &slapd_rq, re ) )
			ldap_pvt_runqueue_stoptask( &slapd_rq, re );
		ldap_pvt_runqueue_remove( &slapd_rq, re );
		ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	}

	/* monitor handling */
	(void)mdb_monitor_db_destroy( be );

	if( mdb->mi_dbenv_home ) ch_free( mdb->mi_dbenv_home );

	mdb_attr_index_destroy( mdb );

	ch_free( mdb );
	be->be_private = NULL;

	return 0;
}

int
mdb_back_initialize(
	BackendInfo	*bi )
{
	int rc;

	static const char * const controls[] = {
		LDAP_CONTROL_ASSERT,
		LDAP_CONTROL_MANAGEDSAIT,
		LDAP_CONTROL_NOOP,
		LDAP_CONTROL_PAGEDRESULTS,
		LDAP_CONTROL_PRE_READ,
		LDAP_CONTROL_POST_READ,
		LDAP_CONTROL_SUBENTRIES,
		LDAP_CONTROL_X_PERMISSIVE_MODIFY,
#ifdef LDAP_X_TXN
		LDAP_CONTROL_X_TXN_SPEC,
#endif
		NULL
	};

	/* initialize the underlying database system */
	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(mdb_back_initialize) ": initialize "
		MDB_UCTYPE " backend\n" );

	bi->bi_flags |=
		SLAP_BFLAG_INCREMENT |
		SLAP_BFLAG_SUBENTRIES |
		SLAP_BFLAG_ALIASES |
		SLAP_BFLAG_REFERRALS;

	bi->bi_controls = controls;

	{	/* version check */
		int major, minor, patch, ver;
		char *version = mdb_version( &major, &minor, &patch );
#ifdef HAVE_EBCDIC
		char v2[1024];

		/* All our stdio does an ASCII to EBCDIC conversion on
		 * the output. Strings from the MDB library are already
		 * in EBCDIC; we have to go back and forth...
		 */
		strcpy( v2, version );
		__etoa( v2 );
		version = v2;
#endif
		ver = (major << 24) | (minor << 16) | patch;
		if( ver != MDB_VERSION_FULL ) {
			/* fail if a versions don't match */
			Debug( LDAP_DEBUG_ANY,
				LDAP_XSTRING(mdb_back_initialize) ": "
				"MDB library version mismatch:"
				" expected " MDB_VERSION_STRING ","
				" got %s\n", version );
			return -1;
		}

		Debug( LDAP_DEBUG_TRACE, LDAP_XSTRING(mdb_back_initialize)
			": %s\n", version );
	}

	bi->bi_open = 0;
	bi->bi_close = 0;
	bi->bi_config = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = mdb_db_init;
	bi->bi_db_config = config_generic_wrapper;
	bi->bi_db_open = mdb_db_open;
	bi->bi_db_close = mdb_db_close;
	bi->bi_db_destroy = mdb_db_destroy;

	bi->bi_op_add = mdb_add;
	bi->bi_op_bind = mdb_bind;
	bi->bi_op_compare = mdb_compare;
	bi->bi_op_delete = mdb_delete;
	bi->bi_op_modify = mdb_modify;
	bi->bi_op_modrdn = mdb_modrdn;
	bi->bi_op_search = mdb_search;

	bi->bi_op_unbind = 0;

	bi->bi_extended = mdb_extended;

	bi->bi_chk_referrals = 0;
	bi->bi_operational = mdb_operational;

	bi->bi_has_subordinates = mdb_hasSubordinates;
	bi->bi_entry_release_rw = mdb_entry_release;
	bi->bi_entry_get_rw = mdb_entry_get;

	/*
	 * hooks for slap tools
	 */
	bi->bi_tool_entry_open = mdb_tool_entry_open;
	bi->bi_tool_entry_close = mdb_tool_entry_close;
	bi->bi_tool_entry_first = backend_tool_entry_first;
	bi->bi_tool_entry_first_x = mdb_tool_entry_first_x;
	bi->bi_tool_entry_next = mdb_tool_entry_next;
	bi->bi_tool_entry_get = mdb_tool_entry_get;
	bi->bi_tool_entry_put = mdb_tool_entry_put;
	bi->bi_tool_entry_reindex = mdb_tool_entry_reindex;
	bi->bi_tool_sync = 0;
	bi->bi_tool_dn2id_get = mdb_tool_dn2id_get;
	bi->bi_tool_entry_modify = mdb_tool_entry_modify;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	rc = mdb_back_init_cf( bi );

	return rc;
}

#if	(SLAPD_MDB == SLAPD_MOD_DYNAMIC)

SLAP_BACKEND_INIT_MODULE( mdb )

#endif /* SLAPD_MDB == SLAPD_MOD_DYNAMIC */
