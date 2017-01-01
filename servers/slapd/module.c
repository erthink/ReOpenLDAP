/* $ReOpenLDAP$ */
/* Copyright 1990-2017 ReOpenLDAP AUTHORS: please see AUTHORS file.
 * All rights reserved.
 *
 * This file is part of ReOpenLDAP.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include "reldap.h"
#include <stdio.h>
#include "slap.h"

#ifdef SLAPD_DYNAMIC_MODULES

#include <ltdl.h>

typedef int (*MODULE_INIT_FN)(
	int argc,
	char *argv[]);
typedef int (*MODULE_LOAD_FN)(
	const void *module,
	const char *filename);
typedef int (*MODULE_TERM_FN)(void);

struct module {
	struct module *next;
	lt_dlhandle lib;
	char name[1];
};

module_t *module_list = NULL;

static const char* safe_basename(const char* path) {
	const char *slash = strrchr(path, '/');
	return slash ? slash + 1 : path;
}

void *module_resolve( const module_t* module, const char *item )
{
	char entry[64];
	if (module == NULL || item == NULL)
		return(NULL);

	const char* modname = safe_basename(module->name);
	if (strncmp(modname, "contrib-", 8) == 0)
		modname += 8;

	int len = strcspn(modname, ".<>?;:'\"[]{}`~!@#%^&*()=\\|		");
	if (len < 1 || len > 32) {
		Debug( LDAP_DEBUG_ANY, "module_resolve: (%s) invalid module name\n",
			modname );
		return NULL;
	}

	snprintf(entry, sizeof(entry), "%.*s_ReOpenLDAP_%s",
		len, modname, item);

	char *hyphen;
	for ( hyphen = entry; *hyphen; ++hyphen ) {
		if ( *hyphen == '-' )
			*hyphen = '_';
	}

	return lt_dlsym(((module_t *)module)->lib, entry);
}

module_t* module_handle( const char *filename )
{
	module_t *module;
	filename = safe_basename(filename);

	for ( module = module_list; module; module = module->next ) {
		if ( !strcmp( safe_basename(module->name), filename )) {
			return module;
		}
	}
	return NULL;
}

static int module_int_unload (module_t *module);

int module_init (void)
{
	if (lt_dlinit()) {
		const char *error = lt_dlerror();
		Debug(LDAP_DEBUG_ANY, "lt_dlinit failed: %s\n", error);

		return -1;
	}

#ifndef SLAPD_MAINTAINER_DIR
	return module_path( LDAP_MODULEDIR );
#else
	return module_path( SLAPD_MAINTAINER_DIR ":" LDAP_MODULEDIR);
#endif
}

int module_kill (void)
{
	/* unload all modules before shutdown */
	while (module_list != NULL) {
		module_int_unload(module_list);
	}

	if (lt_dlexit()) {
		const char *error = lt_dlerror();
		Debug(LDAP_DEBUG_ANY, "lt_dlexit failed: %s\n", error);
		return -1;
	}
	return 0;
}

int module_unload( const char *filename )
{
	module_t *module = module_handle( filename );
	if ( module ) {
		module_int_unload( module );
		return 0;
	}
	return -1;	/* not found */
}

static lt_dlhandle slapd_lt_dlopenext_global( const char *filename )
{
	lt_dlhandle handle = 0;
	lt_dladvise advise;

	if (!lt_dladvise_init (&advise) && !lt_dladvise_ext (&advise)
			&& !lt_dladvise_global (&advise))
		handle = lt_dlopenadvise (filename, advise);

	lt_dladvise_destroy (&advise);

	return handle;
}

int module_load(const char* filename, int argc, char *argv[])
{
	module_t *module;
	const char *error;
	int rc;
	MODULE_INIT_FN initialize;

	module = module_handle( filename );
	if ( module ) {
		Debug( LDAP_DEBUG_ANY, "module_load: (%s) already loaded\n",
			module->name );
		return -1;
	}

	const char* modname = safe_basename(filename);
	if (strncmp(modname, "contrib-", 8) == 0)
		modname += 8;

	/* If loading a backend, see if we already have it */
	if ( !strncasecmp( modname, "back_", 5 )) {
		char *name = (char *)modname + 5;
		char *dot = strchr( name, '.');
		if (dot) *dot = '\0';
		rc = backend_info( name ) != NULL;
		if (dot) *dot = '.';
		if ( rc ) {
			Debug( LDAP_DEBUG_CONFIG, "module_load: (%s) already present (static)\n",
				filename );
			return 0;
		}
	} else {
		/* check for overlays too */
		char *dot = strchr( modname, '.' );
		if ( dot ) *dot = '\0';
		rc = overlay_find( modname ) != NULL;
		if ( dot ) *dot = '.';
		if ( rc ) {
			Debug( LDAP_DEBUG_CONFIG, "module_load: (%s) already present (static)\n",
				filename );
			return 0;
		}
	}

	module = (module_t *)ch_calloc(1, sizeof(module_t) + strlen(filename));
	if (module == NULL) {
		Debug(LDAP_DEBUG_ANY, "module_load failed: (%s) out of memory\n", filename);
		return -1;
	}
	strcpy( module->name, filename );

	/*
	 * The result of lt_dlerror(), when called, must be cached prior
	 * to calling Debug. This is because Debug is a macro that expands
	 * into multiple function calls.
	 */
	if ((module->lib = slapd_lt_dlopenext_global(filename)) == NULL) {
		error = lt_dlerror();
		Debug(LDAP_DEBUG_ANY, "lt_dlopenext failed: (%s) %s\n", filename,
			error);

		ch_free(module);
		return -1;
	}

	Debug(LDAP_DEBUG_CONFIG, "loaded module %s\n", filename);

	initialize = module_resolve(module, "modinit");
	if (initialize == NULL) {
		Debug(LDAP_DEBUG_ANY, "module %s: no %s() function found\n",
			filename, "modinit");

		lt_dlclose(module->lib);
		ch_free(module);
		return -1;
	}

	/* The imported initmod() routine passes back result code,
	 * non-zero of which indicates an error. */
	rc = initialize(argc, argv);
	if (rc == -1) {
		Debug(LDAP_DEBUG_ANY, "module %s: modinit() failed, error code %d\n",
			filename, rc);

		lt_dlclose(module->lib);
		ch_free(module);
		return rc;
	}

	module->next = module_list;
	module_list = module;

	Debug(LDAP_DEBUG_CONFIG, "module %s: registered\n", filename);
	return 0;
}

int module_path(const char *path)
{
	return lt_dlsetsearchpath( path );
}

static int module_int_unload (module_t *module)
{
	int rc = -1;

	if (module != NULL) {
		/* call module's terminate routine, if present */
		MODULE_TERM_FN  terminate = module_resolve(module, "modterm");
		rc = terminate ? terminate() : 0;

		if (rc != 0) {
			Debug(LDAP_DEBUG_ANY,
				  "module %s: could not be unloaded, error code %d\n",
				  module->name, rc);
		} else {
			/* remove module from tracking list */
			if (module_list == module) {
				module_list = module->next;
			} else {
				module_t *mod;
				for (mod = module_list; mod; mod = mod->next) {
					if (mod->next == module) {
						mod->next = module->next;
						break;
					}
				}
			}

			/* close the library and free the memory */
			lt_dlclose(module->lib);

			Debug(LDAP_DEBUG_CONFIG, "module %s: unloaded\n", module->name);
			ch_free(module);
		}
	}
	return rc;
}

#endif /* SLAPD_DYNAMIC_MODULES */
