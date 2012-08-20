/*
  This is the ppl library. It provides a portable interface to usual OS features
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The ppl library free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The ppl library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with the ppl library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/



#ifndef _PPL_UINFO_H_
#define _PPL_UINFO_H_

#include "ppl.h"
#include <ppl/ppl_time.h>
#include <osipparser2/osip_parser.h>
#include <osip2/osip_mt.h>

/**
 * @file ppl_uinfo.h
 * @brief PPL Uinfo Handling Routines
 */

/**
 * @defgroup PPL_UINFO Uinfo Handling
 * @ingroup PPL
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Structure for storing a uinfo descriptor
 * @defvar ppl_uinfo_t
 */
  typedef struct aor_t aor_t;

  struct aor_t
  {
    osip_uri_t *url;
    aor_t *next;
    aor_t *parent;
  };

  typedef struct binding_t binding_t;

  struct binding_t
  {
    osip_contact_t *contact;
    char      *path;      /* rfc3327.txt Extension Header Field
			     for Registering Non-Adjacent Contacts */
    ppl_time_t when;
    binding_t *next;
    binding_t *parent;
  };

  typedef struct ppl_uinfo_t ppl_uinfo_t;

  struct ppl_uinfo_t
  {
    /* presence info */
    int status;

    aor_t *aor;
    char *login;
    char *passwd;
    aor_t *aor_3rd_parties;
    binding_t *bindings;
    ppl_uinfo_t *next;
    ppl_uinfo_t *parent;
  };


/**
 * Intialize the uinfo list entry.
 */
    PPL_DECLARE (int) ppl_uinfo_init (void);

/**
 * Set the dbm file.
 */
    PPL_DECLARE (ppl_status_t) ppl_uinfo_set_dbm(char *recovery_file);

/**
 * Flush dbm file.
 */
    PPL_DECLARE (ppl_status_t) ppl_uinfo_flush_dbm (void);

/**
 * Get uinfo entry.
 */
    PPL_DECLARE (ppl_uinfo_t *) ppl_uinfo_find_by_aor (osip_uri_t * aor);

/**
 * Get uinfo entry.
 */
    PPL_DECLARE (ppl_uinfo_t *) ppl_uinfo_find_by_login (char *login);

/**
 * Create a new uinfo entry.
 */
    PPL_DECLARE (ppl_uinfo_t *) ppl_uinfo_create (osip_uri_t * url, char *login,
						  char *passwd);

/**
 * Delete a binding entry. (internal!)
 */
    PPL_DECLARE (void) ppl_uinfo_binding_free (binding_t * bind);

/**
 * Delete all entries.
 */
    PPL_DECLARE (void) ppl_uinfo_remove (ppl_uinfo_t * uinfo);

/**
 * Chek if binding is deprecated.
 */
    PPL_DECLARE (int) ppl_uinfo_check_binding (binding_t * bind);

/**
 * Remove a binding.
 */
    PPL_DECLARE (int) ppl_uinfo_remove_binding (ppl_uinfo_t * uinfo,
						binding_t * bind);

/**
 * Bind to a uinfo.
 * OBSOLETE: use ppl_uinfo_add_binding_with_path() instead.
 */
    PPL_DECLARE (int) ppl_uinfo_add_binding (ppl_uinfo_t * uinfo,
					     osip_contact_t * con, char *exp);

/**
 * Bind to a uinfo
 * This method also record the Path fields (rfc3327.txt)
 */
    PPL_DECLARE (int) ppl_uinfo_add_binding_with_path (ppl_uinfo_t * uinfo,
						       osip_contact_t * con,
						       char *exp, char *path);

/**
 * Add a authorized third party in uinfo.
 */
    PPL_DECLARE (int) ppl_uinfo_add_third_party (ppl_uinfo_t * uinfo,
						 osip_uri_t * url);

/**
 * Remove all bindings for this user (and delete if it does not contains
 * any static entry).
 */
    PPL_DECLARE (void) ppl_uinfo_remove_all_bindings (ppl_uinfo_t * uinfo);

/**
 * Update data in dbm file.
 */
    PPL_DECLARE (void) ppl_uinfo_store_bindings (ppl_uinfo_t * uinfo);

/**
 * Delete all uinfo entry.
 */
    PPL_DECLARE (void) ppl_uinfo_free_all (void);

/**
 * Close dbm file.
 */
    PPL_DECLARE (void) ppl_uinfo_close_dbm (void);

/**
 * Remove all bindings for this user (and delete if it does not contains
 * any static entry).
 */
    PPL_DECLARE (void) ppl_uinfo_delete (ppl_uinfo_t * uinfo);


#ifdef __cplusplus
}
#endif
/** @} */
#endif
