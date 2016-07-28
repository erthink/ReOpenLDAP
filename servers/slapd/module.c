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

module_t* module_handle( const char *file_name )
{
	module_t *module;
	file_name = safe_basename(file_name);

	for ( module = module_list; module; module = module->next ) {
		if ( !strcmp( safe_basename(module->name), file_name )) {
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

int module_unload( const char *file_name )
{
	module_t *module = module_handle( file_name );
	if ( module ) {
		module_int_unload( module );
		return 0;
	}
	return -1;	/* not found */
}

int module_load(const char* file_name, int argc, char *argv[])
{
	module_t *module;
	const char *error;
	int rc;
	MODULE_INIT_FN initialize;

	module = module_handle( file_name );
	if ( module ) {
		Debug( LDAP_DEBUG_ANY, "module_load: (%s) already loaded\n",
			module->name );
		return -1;
	}

	const char* modname = safe_basename(file_name);
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
				file_name );
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
				file_name );
			return 0;
		}
	}

	module = (module_t *)ch_calloc(1, sizeof(module_t) + strlen(file_name));
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

	initialize = module_resolve(module, "modinit");
	if (initialize == NULL) {
		Debug(LDAP_DEBUG_ANY, "module %s: no %s() function found\n",
			file_name, "modinit");

		lt_dlclose(module->lib);
		ch_free(module);
		return -1;
	}

	/* The imported initmod() routine passes back result code,
	 * non-zero of which indicates an error. */
	rc = initialize(argc, argv);
	if (rc == -1) {
		Debug(LDAP_DEBUG_ANY, "module %s: modinit() failed, error code %d\n",
			file_name, rc);

		lt_dlclose(module->lib);
		ch_free(module);
		return rc;
	}

	module->next = module_list;
	module_list = module;

	Debug(LDAP_DEBUG_CONFIG, "module %s: registered\n", file_name);
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
			ch_free(module);

			Debug(LDAP_DEBUG_CONFIG, "module %s: unloaded\n", module->name);
		}
	}
	return rc;
}

#endif /* SLAPD_DYNAMIC_MODULES */

