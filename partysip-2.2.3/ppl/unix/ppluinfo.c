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

#include <ppl/ppl_uinfo.h>
#include <ppl/ppl_dbm.h>

struct osip_mutex *ppl_uinfo_mutex;

/* hack for psp_users access to users list */
ppl_uinfo_t *user_infos;

static ppl_dbm_t *dbm;

/**
 * Intialize the uinfo list entry.
 */
PPL_DECLARE (int)
ppl_uinfo_init ()
{
  ppl_uinfo_mutex = osip_mutex_init ();
  if (ppl_uinfo_mutex == 0)
    return -1;
  user_infos = NULL;

  dbm = NULL; /* no dbm support */
  
  return 0;
}

/**
 * Flush the uinfo list entries in file.
 */
PPL_DECLARE (int)
ppl_uinfo_flush_dbm ()
{
  ppl_uinfo_t *uinfo;
  int modified = 0;
  int i;
  /* force update for expired registrations */
  for (uinfo = user_infos; uinfo != NULL; uinfo = uinfo->next)
    {
      binding_t *b;
      binding_t *bnext;
      modified = 0;
      for (bnext = uinfo->bindings; bnext != NULL;)
	{
	  b = bnext;
	  bnext = b->next;
	  i = ppl_uinfo_check_binding (b);
	  if (i != 0)
	    {			/* binding is expired */
	      ppl_uinfo_remove_binding (uinfo, b);
	      modified = 1;
	    }
	}
      if (modified==1)
	ppl_uinfo_store_bindings(uinfo);
    }
#if defined(HAVE_GDBM_H) || defined(HAVE_GDBM_NDBM_H) || defined(HAVE_DB_H) || defined(HAVE_DB1_DB_H) || defined(HAVE_DB2_DB_H) || defined(HAVE_DB3_DB_H) || defined(HAVE_NDBM_H) || defined(HAVE_DBM_H)
  if (dbm==NULL) return 0; /* this is simpler this way */
  return ppl_dbm_flush(dbm, O_RDWR | O_CREAT);
#else
  return 0;
#endif
}

/**
 * Set the dbm file.
 */
PPL_DECLARE (ppl_status_t)
ppl_uinfo_set_dbm(char *recovery_file)
{
  dbm=NULL;
#if defined(HAVE_GDBM_H) || defined(HAVE_GDBM_NDBM_H) || defined(HAVE_DB_H) || defined(HAVE_DB1_DB_H) || defined(HAVE_DB2_DB_H) || defined(HAVE_DB3_DB_H) || defined(HAVE_NDBM_H) || defined(HAVE_DBM_H)
  if (recovery_file!=NULL)
    {
      int i;
      i = ppl_dbm_open(&dbm, recovery_file, O_RDWR | O_CREAT);
      if (i!=0)
	return -1;
    }
#endif
  return 0;
}

#if 0
/**
 * Get uinfo entry.
 */
static ppl_status_t
ppl_uinfo_add_aor (ppl_uinfo_t * uinfo, osip_uri_t * aor)
{
  aor_t *_aor;

  if (uinfo == NULL)
    return -1;

  _aor = (aor_t *) osip_malloc (sizeof (aor_t));
  if (_aor == NULL)
    return -1;
  _aor->url = aor;
  _aor->next = NULL;
  _aor->parent = NULL;

  ADD_ELEMENT (uinfo->aor, _aor);
  return 0;
}
#endif

/**
 * Get uinfo entry.
 */
PPL_DECLARE (ppl_uinfo_t *) ppl_uinfo_find_by_aor (osip_uri_t * aor)
{
  ppl_uinfo_t *uinfo;

  if (user_infos == NULL)
    return NULL;
  if (aor == NULL || aor->username == NULL || aor->host == NULL)
    return NULL;
  for (uinfo = user_infos; uinfo != NULL; uinfo = uinfo->next)
    {
      aor_t *aaor;

      for (aaor = uinfo->aor; aaor != NULL; aaor = aaor->next)
	{
	  /* this way we'll accept jack@ip and jack@fqdn as
	     the same user! This will work until partysip
	     support multiple domains. */
	  /*  if (0==strcmp(aor->username, aaor->url->username)
	     &&0==strcmp(aor->host, aaor->url->host)) */
	  if (0 == strcasecmp (aor->username, aaor->url->username))
	    return uinfo;
	}
    }
  return NULL;
}

/**
 * Get uinfo entry.
 */
PPL_DECLARE (ppl_uinfo_t *) ppl_uinfo_find_by_login (char *login)
{
  ppl_uinfo_t *uinfo;

  if (user_infos == NULL)
    return NULL;
  if (login == NULL)
    return NULL;		/* these are public registration */
  for (uinfo = user_infos; uinfo != NULL; uinfo = uinfo->next)
    {
      if (uinfo->login != NULL && 0 == strcmp (login, uinfo->login))
	return uinfo;
    }
  return NULL;
}

/**
 * Create a new uinfo entry.
 */
PPL_DECLARE (ppl_uinfo_t *)
ppl_uinfo_create (osip_uri_t * url, char *login, char *passwd)
{
  ppl_uinfo_t *uinfo;
  aor_t *aor;
  int i;
  osip_uri_t *dest;

  uinfo = (ppl_uinfo_t *) osip_malloc (sizeof (ppl_uinfo_t));
  if (uinfo == NULL)
    return NULL;
  uinfo->status = 2; /* away */
  uinfo->login = login;
  uinfo->passwd = passwd;
  uinfo->aor = NULL;
  uinfo->aor_3rd_parties = NULL;
  uinfo->bindings = NULL;
  uinfo->next = NULL;
  uinfo->parent = NULL;

  aor = (aor_t *) osip_malloc (sizeof (aor_t));
  if (aor == NULL)
    {
      osip_free (uinfo);
      return NULL;
    }
  aor->url = NULL;
  aor->next = NULL;
  aor->parent = NULL;

  i = osip_uri_clone (url, &dest);
  if (i != 0)
    {
      osip_free (uinfo);
      osip_free (aor);
      return NULL;
    }
  aor->url = dest;

  uinfo->aor = aor;
  ADD_ELEMENT (user_infos, uinfo);
  return uinfo;
}


/**
 * Sort uinfo database.
 */
static int ppl_uinfo_sort (ppl_uinfo_t * uinfo)
{
  /* read all the q= value and sort... */
  /*
     osip_generic_param_t *q;
     osip_generic_param_t *q2;
     binding_t *b;
     binding_t *b2;
     int i;
   */

  /* qvalue can be 1.000  or 0.873  or 0.1  or 1 ... */
  /* this is hard to compare and sort!               */
  /* TODO */
  return 0;
}

/**
 * Chek if binding is deprecated.
 */
PPL_DECLARE (int) ppl_uinfo_check_binding (binding_t * bind)
{
  if (bind == NULL)
    return -1;
  if (bind->when < ppl_time ())
    return -1;
  return 0;
}

/**
 * Add a new binding (a contact header) to a uinfo.
 * OBSOLETE: use ppl_uinfo_add_binding_with_path() instead.
 */
PPL_DECLARE (int)
ppl_uinfo_add_binding (ppl_uinfo_t * uinfo, osip_contact_t * con, char *exp)
{
  int i;
  osip_contact_t *dest;
  binding_t *bind;
  osip_generic_param_t *exp_p;
  osip_generic_param_t *q;
  ppl_time_t now;
  int length;
  char *tmp;

  if (uinfo == NULL)
    return -1;
  now = ppl_time ();

  if (con == NULL)
    return -1;
  if (con->displayname != NULL && 0 == strcmp (con->displayname, "*"))
    {
      ppl_uinfo_remove_all_bindings (uinfo);
      return 0;
    }
  /*  we MUST check if the contact info already exist */
  for (bind = uinfo->bindings; bind != NULL; bind = bind->next)
    {
      if (bind->contact == NULL || bind->contact->url == NULL || con->url == NULL)	/* no */
	{
	}
      else if (bind->contact->url->username != NULL
	       && con->url->username != NULL
	       && bind->contact->url->host != NULL && con->url->host != NULL)
	{
	  if (0 ==
	      strcasecmp (bind->contact->url->username, con->url->username)
	      && 0 == strcasecmp (bind->contact->url->host, con->url->host))
	    {			/* find a match! replace the entry... */
	      /* TODO?: ip and FQDN also must match... */
	      ppl_uinfo_remove_binding (uinfo, bind);
	      break;
	    }
	}
      else if (bind->contact->url->username == NULL
	       && con->url->username == NULL
	       && bind->contact->url->host != NULL && con->url->host != NULL)
	if (0 == strcasecmp (bind->contact->url->host, con->url->host))
	  {
	    ppl_uinfo_remove_binding (uinfo, bind);
	    break;
	  }
    }

  bind = (binding_t *) osip_malloc (sizeof (binding_t));
  bind->next = NULL;
  bind->parent = NULL;
  i = osip_contact_clone (con, &dest);
  if (i != 0)
    {
      if (uinfo->bindings==NULL)
	uinfo->status = 2;
      else
	uinfo->status = 1;
      osip_free (bind);
      return -1;
    }
  bind->path = NULL;

  i = osip_contact_param_get_byname (dest, "expires", &exp_p);
  if (i != 0)
    {
      if (exp != NULL)
	tmp = osip_strdup (exp);
      else
	tmp = osip_strdup ("3600");
      osip_contact_param_add (dest, osip_strdup ("expires"), tmp);
      length = atoi (tmp);
    }
  else
    {
      length = atoi (exp_p->gvalue);
    }

  bind->contact = dest;
  if (length <= 0)
    {				/* this registration is expired! */
      if (uinfo->bindings==NULL)
	uinfo->status = 2;
      else
	uinfo->status = 1;
      osip_contact_free (bind->contact);
      osip_free (bind->path);
      osip_free (bind);
      return 0;
    }
  bind->when = now + length;	/* registration will expire in 'length' seconds */

  /* avoid too much sorting... */
  i = osip_contact_param_get_byname (dest, "q", &q);
  if (i != 0
      || (q != NULL && q->gvalue != NULL && 0 == strncmp (q->gvalue, "1", 1)))
    {
      ADD_ELEMENT (uinfo->bindings, bind);
    }
  else
    {
      APPEND_ELEMENT (binding_t, uinfo->bindings, bind);
    }
  ppl_uinfo_sort (uinfo);

  if (uinfo->bindings==NULL)
    uinfo->status = 2;
  else
    uinfo->status = 1;

  return 0;
}

/**
 * Add a new binding (a contact header) to a uinfo.
 * This method also record the Path fields (rfc3327.txt)
 */
PPL_DECLARE (int)
ppl_uinfo_add_binding_with_path (ppl_uinfo_t * uinfo, osip_contact_t * con,
				 char *exp, char *path)
{
  int i;
  osip_contact_t *dest;
  binding_t *bind;
  osip_generic_param_t *exp_p;
  osip_generic_param_t *q;
  ppl_time_t now;
  int length;
  char *tmp;

  if (uinfo == NULL)
    return -1;
  now = ppl_time ();

  if (con == NULL)
    return -1;
  if (con->displayname != NULL && 0 == strcmp (con->displayname, "*"))
    {
      ppl_uinfo_remove_all_bindings (uinfo);
      return 0;
    }
  /*  we MUST check if the contact info already exist */
  for (bind = uinfo->bindings; bind != NULL; bind = bind->next)
    {
      if (bind->contact == NULL || bind->contact->url == NULL || con->url == NULL)	/* no */
	{
	}
      else if (bind->contact->url->username != NULL
	       && con->url->username != NULL
	       && bind->contact->url->host != NULL && con->url->host != NULL)
	{
	  if (0 ==
	      strcasecmp (bind->contact->url->username, con->url->username)
	      && 0 == strcasecmp (bind->contact->url->host, con->url->host))
	    {			/* find a match! replace the entry... */
	      /* TODO?: ip and FQDN also must match... */
#ifndef ENABLE_MPATROL /* just to ease local test of forking proxy */
	      ppl_uinfo_remove_binding (uinfo, bind);
#endif
	      break;
	    }
	}
      else if (bind->contact->url->username == NULL
	       && con->url->username == NULL
	       && bind->contact->url->host != NULL && con->url->host != NULL)
	if (0 == strcasecmp (bind->contact->url->host, con->url->host))
	  {
	    ppl_uinfo_remove_binding (uinfo, bind);
	    break;
	  }
    }

  bind = (binding_t *) osip_malloc (sizeof (binding_t));
  bind->next = NULL;
  bind->parent = NULL;
  i = osip_contact_clone (con, &dest);
  if (i != 0)
    {
      if (uinfo->bindings==NULL)
	uinfo->status = 2;
      else
	uinfo->status = 1;
      osip_free (bind);
      return -1;
    }
  bind->path = osip_strdup(path);

  i = osip_contact_param_get_byname (dest, "expires", &exp_p);
  if (i != 0)
    {
      if (exp != NULL)
	tmp = osip_strdup (exp);
      else
	tmp = osip_strdup ("3600");
      osip_contact_param_add (dest, osip_strdup ("expires"), tmp);
      length = atoi (tmp);
    }
  else
    {
      length = atoi (exp_p->gvalue);
    }

  bind->contact = dest;
  if (length <= 0)
    {				/* this registration is expired! */
      if (uinfo->bindings==NULL)
	uinfo->status = 2;
      else
	uinfo->status = 1;
      osip_contact_free (bind->contact);
      osip_free (bind->path);
      osip_free (bind);
      return 0;
    }
  bind->when = now + length;	/* registration will expire in 'length' seconds */

  /* avoid too much sorting... */
  i = osip_contact_param_get_byname (dest, "q", &q);
  if (i != 0
      || (q != NULL && q->gvalue != NULL && 0 == strncmp (q->gvalue, "1", 1)))
    {
      ADD_ELEMENT (uinfo->bindings, bind);
    }
  else
    {
      APPEND_ELEMENT (binding_t, uinfo->bindings, bind);
    }
  ppl_uinfo_sort (uinfo);

  if (uinfo->bindings==NULL)
    uinfo->status = 2;
  else
    uinfo->status = 1;

  return 0;
}


/**
 * Remove a binding.
 */
PPL_DECLARE (int)
ppl_uinfo_remove_binding (ppl_uinfo_t * uinfo, binding_t * bind)
{
  if (uinfo == NULL || bind == NULL)
    return -1;
  REMOVE_ELEMENT (uinfo->bindings, bind);
  osip_contact_free (bind->contact);
  osip_free (bind->path);
  osip_free (bind);
  return 0;
}

/**
 * Add a authorized third party in uinfo.
 */
PPL_DECLARE (int) ppl_uinfo_add_third_party (ppl_uinfo_t * uinfo, osip_uri_t * url)
{
  aor_t *aor;
  osip_uri_t *dest;
  int i;

  aor = (aor_t *) osip_malloc (sizeof (aor_t));
  if (aor == NULL)
    return -1;
  aor->url = NULL;
  aor->next = NULL;
  aor->parent = NULL;

  i = osip_uri_clone (url, &dest);
  if (i != 0)
    {
      osip_free (aor);
      return -1;
    }
  aor->url = dest;

  ADD_ELEMENT (uinfo->aor_3rd_parties, aor);
  return 0;
}

/**
 * Delete a binding entry. (internal!)
 */
PPL_DECLARE (void) ppl_uinfo_binding_free (binding_t * bind)
{
  if (bind == NULL)
    return;
  osip_contact_free (bind->contact);
  osip_free (bind->path);
  osip_free (bind);
  return;
}

/**
 * Delete all entries.
 */
PPL_DECLARE (void) ppl_uinfo_remove (ppl_uinfo_t * uinfo)
{
  aor_t *aor;
  binding_t *bind;

  if (uinfo == NULL)
    return;

  if (dbm!=NULL
      &&uinfo->aor!=NULL
      &&uinfo->aor->url!=NULL
      &&uinfo->aor->url->username!=NULL)
    {
      ppl_dbm_delete(dbm, uinfo->aor->url->username);
    }

  REMOVE_ELEMENT (user_infos, uinfo);
  osip_free (uinfo->login);
  osip_free (uinfo->passwd);


  for (aor = uinfo->aor; aor != NULL; aor = uinfo->aor)
    {
      REMOVE_ELEMENT (uinfo->aor, aor);
      osip_uri_free (aor->url);
      osip_free (aor);
    }

  for (aor = uinfo->aor_3rd_parties; aor != NULL;
       aor = uinfo->aor_3rd_parties)
    {
      REMOVE_ELEMENT (uinfo->aor_3rd_parties, aor);
      osip_uri_free (aor->url);
      osip_free (aor);
    }
  for (bind = uinfo->bindings; bind != NULL; bind = uinfo->bindings)
    {
      REMOVE_ELEMENT (uinfo->bindings, bind);
      ppl_uinfo_binding_free (bind);
    }
  osip_free (uinfo);
  return;
}

/**
 * Remove all bindings for this user (and force deletion if force==1).
 */
static void
_ppl_uinfo_remove_all_bindings (ppl_uinfo_t * uinfo, int force)
{
  binding_t *b;

  if (force == 1)
    {
      if (uinfo->bindings==NULL)
	uinfo->status = 2;
      ppl_uinfo_remove (uinfo);
      return;
    }

  for (b = uinfo->bindings; b != NULL; b = uinfo->bindings)
    {
      REMOVE_ELEMENT (uinfo->bindings, b);
      osip_contact_free (b->contact);
      osip_free (b->path);
      osip_free (b);
    }
}

/**
 * Remove all bindings for this user (and delete if it does not contains
 * any static entry).
 */
PPL_DECLARE (void)
ppl_uinfo_remove_all_bindings (ppl_uinfo_t * uinfo)
{
  _ppl_uinfo_remove_all_bindings (uinfo, -1);
  if (uinfo->bindings==NULL)
    uinfo->status = 2;
}

PPL_DECLARE (void)
ppl_uinfo_store_bindings (ppl_uinfo_t * uinfo)
{
#if defined(HAVE_GDBM_H) || defined(HAVE_GDBM_NDBM_H) || defined(HAVE_DB_H) || defined(HAVE_DB1_DB_H) || defined(HAVE_DB2_DB_H) || defined(HAVE_DB3_DB_H) || defined(HAVE_NDBM_H) || defined(HAVE_DBM_H)
  if (dbm!=NULL)
    ppl_dbm_store(dbm, uinfo);
#endif
}

/**
 * Delete all entries.
 */
PPL_DECLARE (void)
ppl_uinfo_free_all ()
{
  ppl_uinfo_t *uinfo;

  for (uinfo = user_infos; uinfo != NULL; uinfo = user_infos)
    {
      ppl_uinfo_remove (uinfo);
    }
  osip_mutex_destroy (ppl_uinfo_mutex);
}

/**
 * Close dbm file.
 */
PPL_DECLARE (void)
ppl_uinfo_close_dbm ()
{
#if defined(HAVE_GDBM_H) || defined(HAVE_GDBM_NDBM_H) || defined(HAVE_DB_H) || defined(HAVE_DB1_DB_H) || defined(HAVE_DB2_DB_H) || defined(HAVE_DB3_DB_H) || defined(HAVE_NDBM_H) || defined(HAVE_DBM_H)
  ppl_uinfo_t *uinfo;
  int modified = 0;
  int i;
  /* force update for expired registrations */
  for (uinfo = user_infos; uinfo != NULL; uinfo = uinfo->next)
    {
      binding_t *b;
      binding_t *bnext;
      modified = 0;
      for (bnext = uinfo->bindings; bnext != NULL;)
	{
	  b = bnext;
	  bnext = b->next;
	  i = ppl_uinfo_check_binding (b);
	  if (i != 0)
	    {			/* binding is expired */
	      ppl_uinfo_remove_binding (uinfo, b);
	      modified = 1;
	    }
	}
      if (modified==1)
	ppl_uinfo_store_bindings(uinfo);
    }

  if (dbm!=NULL)
    {
      ppl_dbm_close(dbm);
      osip_free(dbm);
      dbm=NULL;
    }
#endif  
}
