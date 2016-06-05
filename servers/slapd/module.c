/* $ReOpenLDAP$ */
/* Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
 * Copyright (c) 2015,2016 Peter-Service R&D LLC <http://billing.ru/>.
 *
 * This file is part of ReOpenLDAP.
 *
 * ReOpenLDAP is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReOpenLDAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ---
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
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


struct module_regtable_t {
	char *type;
	MODULE_LOAD_FN proc;
} module_regtable[] = {
		{ "null", load_null_module },
#ifdef SLAPD_EXTERNAL_EXTENSIONS
		{ "extension", load_extop_module },
#endif
		{ NULL, NULL }
};

typedef struct module_loaded_t {
	struct module_loaded_t *next;
	lt_dlhandle lib;
	char name[1];
} module_loaded_t;

module_loaded_t *module_list = NULL;

static int module_int_unload (module_loaded_t *module);

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

static const char* safe_basename(const char* path) {
	const char *slash = strrchr(path, '/');
	return slash ? slash + 1 : path;
}

void * module_handle( const char *file_name )
{
	module_loaded_t *module;
	file_name = safe_basename(file_name);

	for ( module = module_list; module; module= module->next ) {
		if ( !strcmp( safe_basename(module->name), file_name )) {
			return module;
		}
	}
	return NULL;
}

int module_unload( const char *file_name )
{
	module_loaded_t *module;

	module = module_handle( file_name );
	if ( module ) {
		module_int_unload( module );
		return 0;
	}
	return -1;	/* not found */
}

int module_load(const char* file_name, int argc, char *argv[])
{
	module_loaded_t *module;
	const char *error;
	int rc, len;
	MODULE_INIT_FN initialize;

	module = module_handle( file_name );
	if ( module ) {
		Debug( LDAP_DEBUG_ANY, "module_load: (%s) already loaded\n",
			module->name );
		return -1;
	}

	const char* file_basename = safe_basename(file_name);
	len = strcspn(file_basename, ".<>?;:'\"[]{}`~!@#%^&*()-=\\|		");
	if (len < 1 || len > 32) {
		Debug( LDAP_DEBUG_ANY, "module_load: (%s) invalid module name\n",
			file_basename );
		return -1;
	}

	/* If loading a backend, see if we already have it */
	if ( !strncasecmp( file_basename, "back_", 5 )) {
		char *name = (char *)file_basename + 5;
		char *dot = strchr( name, '.');
		if (dot) *dot = '\0';
		rc = backend_info( name ) != NULL;
		if (dot) *dot = '.';
		if ( rc ) {
			Debug( LDAP_DEBUG_CONFIG, "module_load: (%s) already present (static)\n",
				file_name );
			return 0;
		}
	} else {
		/* check for overlays too */
		char *dot = strchr( file_basename, '.' );
		if ( dot ) *dot = '\0';
		rc = overlay_find( file_basename ) != NULL;
		if ( dot ) *dot = '.';
		if ( rc ) {
			Debug( LDAP_DEBUG_CONFIG, "module_load: (%s) already present (static)\n",
				file_name );
			return 0;
		}
	}

	module = (module_loaded_t *)ch_calloc(1, sizeof(module_loaded_t) +
		strlen(file_name));
	if (module == NULL) {
		Debug(LDAP_DEBUG_ANY, "module_load failed: (%s) out of memory\n", file_name);
		return -1;
	}
	strcpy( module->name, file_name );

	/*
	 * The result of lt_dlerror(), when called, must be cached prior
	 * to calling Debug. This is because Debug is a macro that expands
	 * into multiple function calls.
	 */
	if ((module->lib = lt_dlopenext(file_name)) == NULL) {
		error = lt_dlerror();
		Debug(LDAP_DEBUG_ANY, "lt_dlopenext failed: (%s) %s\n", file_name,
			error);

		ch_free(module);
		return -1;
	}

	Debug(LDAP_DEBUG_CONFIG, "loaded module %s\n", file_name);

	char entry[64];
	snprintf(entry, sizeof(entry), "%.*s_ReOpenLDAP_modinit",
		len, file_basename);

	if ((initialize = lt_dlsym(module->lib, entry)) == NULL) {
		Debug(LDAP_DEBUG_ANY, "module %s: no %s() function found\n",
			file_name, entry);

		lt_dlclose(module->lib);
		ch_free(module);
		return -1;
	}

	/* The imported init_module() routine passes back the type of
	 * module (i.e., which part of slapd it should be hooked into)
	 * or -1 for error.  If it passes back 0, then you get the
	 * old behavior (i.e., the library is loaded and not hooked
	 * into anything).
	 *
	 * It might be better if the conf file could specify the type
	 * of module.  That way, a single module could support multiple
	 * type of hooks. This could be done by using something like:
	 *
	 *    moduleload extension /usr/local/reopenldap/whatever.so
	 *
	 * then we'd search through module_regtable for a matching
	 * module type, and hook in there.
	 */
	rc = initialize(argc, argv);
	if (rc == -1) {
		Debug(LDAP_DEBUG_ANY, "module %s: init_module() failed\n",
			file_name);

		lt_dlclose(module->lib);
		ch_free(module);
		return rc;
	}

	if (rc >= (int)(sizeof(module_regtable) / sizeof(struct module_regtable_t))
		|| module_regtable[rc].proc == NULL)
	{
		Debug(LDAP_DEBUG_ANY, "module %s: unknown registration type (%d)\n",
			file_name, rc);

		module_int_unload(module);
		return -1;
	}

	rc = (module_regtable[rc].proc)(module, file_name);
	if (rc != 0) {
		Debug(LDAP_DEBUG_ANY, "module %s: %s module could not be registered\n",
			file_name, module_regtable[rc].type);

		module_int_unload(module);
		return rc;
	}

	module->next = module_list;
	module_list = module;

	Debug(LDAP_DEBUG_CONFIG, "module %s: %s module registered\n",
		file_name, module_regtable[rc].type);

	return 0;
}

int module_path(const char *path)
{
	return lt_dlsetsearchpath( path );
}

void *module_resolve (const void *module, const char *name)
{
	if (module == NULL || name == NULL)
		return(NULL);
	return(lt_dlsym(((module_loaded_t *)module)->lib, name));
}

static int module_int_unload (module_loaded_t *module)
{
	module_loaded_t *mod;
	MODULE_TERM_FN terminate;

	if (module != NULL) {
		/* remove module from tracking list */
		if (module_list == module) {
			module_list = module->next;
		} else {
			for (mod = module_list; mod; mod = mod->next) {
				if (mod->next == module) {
					mod->next = module->next;
					break;
				}
			}
		}

		/* call module's terminate routine, if present */
		if ((terminate = lt_dlsym(module->lib, "term_module"))) {
			terminate();
		}

		/* close the library and free the memory */
		lt_dlclose(module->lib);
		ch_free(module);
	}
	return 0;
}

int load_null_module (const void *module, const char *file_name)
{
	return 0;
}

#ifdef SLAPD_EXTERNAL_EXTENSIONS
int
load_extop_module (
	const void *module,
	const char *file_name
)
{
	SLAP_EXTOP_MAIN_FN *ext_main;
	SLAP_EXTOP_GETOID_FN *ext_getoid;
	struct berval oid;
	int rc;

	ext_main = (SLAP_EXTOP_MAIN_FN *)module_resolve(module, "ext_main");
	if (ext_main == NULL) {
		return(-1);
	}

	ext_getoid = module_resolve(module, "ext_getoid");
	if (ext_getoid == NULL) {
		return(-1);
	}

	rc = (ext_getoid)(0, &oid, 256);
	if (rc != 0) {
		return(rc);
	}
	if (oid.bv_val == NULL || oid.bv_len == 0) {
		return(-1);
	}

	/* FIXME: this is broken, and no longer needed,
	 * as a module can call load_extop() itself... */
	rc = load_extop( &oid, ext_main );
	return rc;
}
#endif /* SLAPD_EXTERNAL_EXTENSIONS */
#endif /* SLAPD_DYNAMIC_MODULES */

