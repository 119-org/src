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



#ifndef _PPL_DBM_H_
#define _PPL_DBM_H_

#include "ppl.h"
#include <ppl/ppl_uinfo.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>  /* for O_RDWR and O_CREAT */
#endif

/*
  #if defined(HAVE_GDBM_H)
  // untested (GnuDBM)
  #include <gdbm.h>
*/
#if defined(HAVE_GDBM_NDBM_H)
/* tested (GnuDBM) */
#include <gdbm-ndbm.h>

#elif defined(HAVE_DB_H)
/* tested (Berkeley DB3) */
#define DB_DBM_HSEARCH 1
#include <db.h>

#elif defined(HAVE_DB1_DB_H)
/* untested */
#define DB_DBM_HSEARCH 1
#include <db1/db.h>

#elif defined(HAVE_DB2_DB_H)
/* untested */
#define DB_DBM_HSEARCH 1
#include <db2/db.h>

#elif defined(HAVE_DB3_DB_H)
/* Does this exist? */
#define DB_DBM_HSEARCH 1
#include <db3/db.h>

#elif defined(HAVE_NDBM_H)
/* untested */
#include <ndbm.h>

#elif defined(HAVE_DBM_H)
/* untested */
#include <dbm.h>
#endif


/**
 * @file ppl_dbm.h
 * @brief PPL Dbm Handling Routines
 */

/**
 * @defgroup PPL_DBM Dbm Handling
 * @ingroup PPL
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif


#if defined(HAVE_GDBM_H) || defined(HAVE_GDBM_NDBM_H) || defined(HAVE_DB_H) || defined(HAVE_DB1_DB_H) || defined(HAVE_DB2_DB_H) || defined(HAVE_DB3_DB_H) || defined(HAVE_NDBM_H) || defined(HAVE_DBM_H)
/**
 * Structure for storing a dbm descriptor
 * @defvar ppl_dbm_t
 */
  typedef struct ppl_dbm_t ppl_dbm_t;

  struct ppl_dbm_t
  {
    char dbfile[255];
    DBM  *dbm;
  };

  typedef datum ppl_datum_t;

#else

  /* not implemented */
/**
 * Structure for storing a dbm descriptor
 * @defvar ppl_dbm_t
 */
  typedef struct ppl_dbm_t ppl_dbm_t;

  struct ppl_dbm_t
  {
    char *dbfile;
  };

  typedef int ppl_datum_t;

#endif

/**
 * Intialize the uinfo list entry.
 */
PPL_DECLARE (int) ppl_dbm_open(ppl_dbm_t **dbm, char *dbfile, int flag);

/**
 * Flush entries in file.
 */
PPL_DECLARE (int) ppl_dbm_flush(ppl_dbm_t *_dbm, int flag);

/**
 * Store values in dbm database.
 */
PPL_DECLARE (int) ppl_dbm_store(ppl_dbm_t *dbm, ppl_uinfo_t *uinfo);

/**
 * Insert values in dbm database.
 */
PPL_DECLARE (int) ppl_dbm_insert(ppl_dbm_t *dbm, ppl_uinfo_t *uinfo); 

/**
 * Delete values in dbm database.
 */
PPL_DECLARE (int) ppl_dbm_delete(ppl_dbm_t *dbm, char *key); 

/**
 * fetch info from the dbm database.
 */
PPL_DECLARE (int) ppl_dbm_fetch(ppl_dbm_t *dbm, char *_key, ppl_uinfo_t **uinfo);

/**
 * close the dbm database.
 */
PPL_DECLARE (void) ppl_dbm_close(ppl_dbm_t *dbm); 

#endif
