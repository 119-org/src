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

#include <ppl/ppl_dbm.h>

#if defined(HAVE_GDBM_H) || defined(HAVE_GDBM_NDBM_H) || defined(HAVE_DB_H) || defined(HAVE_DB1_DB_H) || defined(HAVE_DB2_DB_H) || defined(HAVE_DB3_DB_H) || defined(HAVE_NDBM_H) || defined(HAVE_DBM_H)

/**
 * Intialize the uinfo list entry.
 */
PPL_DECLARE (int) ppl_dbm_open(ppl_dbm_t **_dbm, char *dbfile, int flag)
{
  ppl_datum_t nextkey;
  ppl_datum_t key;
  if (dbfile==NULL)
    return -1;

  *_dbm = (ppl_dbm_t *) osip_malloc (sizeof (ppl_dbm_t));
  if (*_dbm==NULL) return -1;

  osip_strncpy((*_dbm)->dbfile, dbfile, strlen(dbfile));

  (*_dbm)->dbm = dbm_open(dbfile, flag, 0666);
  if (!(*_dbm)->dbm)
    {
      osip_free(*_dbm);
      *_dbm = NULL;
      return -1;
    }
  
  /* reload all uinfo:
   */
#if (defined(HAVE_DBM_H) && defined (__OpenBSD__)) || defined(HAVE_DB_H) || defined(HAVE_DB1_DB_H) || defined(HAVE_DB2_DB_H) || defined(HAVE_DB3_DB_H)
  for (key = dbm_firstkey ((*_dbm)->dbm); key.dptr;
       key = dbm_nextkey ((*_dbm)->dbm))
    {
      ppl_uinfo_t *uinfo;
      int i;
      char tmp[255];
      osip_strncpy(tmp, key.dptr, key.dsize);
      i = ppl_dbm_fetch((*_dbm), tmp, &uinfo);
      if (i!=0)
	{ /* don't like this entry?? */ }
      nextkey = key;
    }
#else
  for (key = dbm_firstkey ((*_dbm)->dbm); key.dptr;
       key = dbm_nextkey ((*_dbm)->dbm, nextkey))
    {
      ppl_uinfo_t *uinfo;
      int i;
      char tmp[255];
      osip_strncpy(tmp, key.dptr, key.dsize);
      i = ppl_dbm_fetch((*_dbm), tmp, &uinfo);
      if (i!=0)
	{ /* don't like this entry?? */ }
      nextkey = key;
    }
#endif

  return 0;
}

/**
 * Flush entry in file.
 */
PPL_DECLARE (int) ppl_dbm_flush(ppl_dbm_t *_dbm, int flag)
{
  if (_dbm==NULL||_dbm->dbm==NULL)
    return -1;
  ppl_dbm_close(_dbm);
  
  _dbm->dbm = dbm_open(_dbm->dbfile, flag, 0666);
  if (!_dbm->dbm)
  {
    return -1;
  }
  return 0;
}

/**
 * Store values in dbm database.
 */
PPL_DECLARE (int) ppl_dbm_store(ppl_dbm_t *dbm, ppl_uinfo_t *uinfo)
{
  binding_t *binding;
  int len;
  char *_string;
  char *__string;
  ppl_datum_t key;
  ppl_datum_t data;
  char *tmp;
  int i;

  if (dbm==NULL||dbm->dbm==NULL)
    return -1;

  if (uinfo==NULL
      || uinfo->aor==NULL
      || uinfo->aor->url==NULL
      || uinfo->aor->url->username==NULL)
    return -1;

  len = 25;
  _string = (char *) osip_malloc(len+1);
  

  __string = _string;
  sprintf(__string, "%i", uinfo->status);
  sprintf(__string+1, "|");

  __string = _string+2;

  i = osip_uri_to_str(uinfo->aor->url, &tmp);
  if (i!=0)
    {
      osip_free(_string);
      return -1;
    }

  if (strlen(tmp) > len-strlen(_string)-2)
    {
      len = len+strlen(tmp);
      _string = (char *) realloc(_string, len);
      if (_string==NULL)
	return -1;
      __string = _string+2;
    }
  sprintf(__string, "%s|", tmp); /* username in the To header */

  osip_free(tmp);

  for (binding = uinfo->bindings; binding != NULL; binding = binding->next)
    {
      i = osip_contact_to_str(binding->contact, &tmp);
      if (i!=0)
	{
	  osip_free(_string);
	  return -1;
	}
      if (strlen(tmp) > len-strlen(_string)-2)
	{
	  len = len+strlen(tmp);
	  _string = (char *) realloc(_string, len);
	  if (_string==NULL)
	    return -1;
	  __string = _string+2;
	}

      __string = __string + strlen(__string);
      sprintf(__string, "%s|", tmp); /* username in the To header */
      osip_free(tmp);
    }
  
  key.dptr  = (void *)uinfo->aor->url->username;
  key.dsize = strlen(uinfo->aor->url->username)+1;
  data.dptr  = (void *)_string;
  data.dsize = strlen(_string)+1;

  i = dbm_store(dbm->dbm, key, data, DBM_REPLACE);

  osip_free(_string);
  if (i!= 0)
    return -1;
  return 0;
}

/**
 * Insert values in dbm database.
 */
PPL_DECLARE (int) ppl_dbm_insert(ppl_dbm_t *dbm, ppl_uinfo_t *uinfo)
{
  binding_t *binding;
  int len;
  char *_string;
  char *__string;
  ppl_datum_t key;
  ppl_datum_t data;
  char *tmp;
  int i;

  if (dbm==NULL||dbm->dbm==NULL)
    return -1;

  if (uinfo==NULL
      || uinfo->aor==NULL
      || uinfo->aor->url==NULL
      || uinfo->aor->url->username==NULL)
    return -1;

  len = 25;
  _string = (char *) osip_malloc(len+1);
  

  __string = _string;
  sprintf(__string, "%i", uinfo->status);
  sprintf(__string+1, "|");

  __string = _string+2;

  i = osip_uri_to_str(uinfo->aor->url, &tmp);
  if (i!=0)
    {
      osip_free(_string);
      return -1;
    }

  if (strlen(tmp) > len-strlen(_string)-2)
    {
      len = len+strlen(tmp);
      _string = (char *) realloc(_string, len);
      if (_string==NULL)
	return -1;
      __string = _string+2;
    }
  sprintf(__string, "%s|", tmp); /* username in the To header */

  osip_free(tmp);

  for (binding = uinfo->bindings; binding != NULL; binding = binding->next)
    {
      i = osip_contact_to_str(binding->contact, &tmp);
      if (i!=0)
	{
	  osip_free(_string);
	  return -1;
	}
      if (strlen(tmp) > len-strlen(_string)-2)
	{
	  len = len+strlen(tmp);
	  _string = (char *) realloc(_string, len);
	  if (_string==NULL)
	    return -1;
	  __string = _string+2;
	}

      __string = __string + strlen(__string);
      sprintf(__string, "%s|", tmp); /* username in the To header */
      osip_free(tmp);
    }
  
  key.dptr  = (void *)uinfo->aor->url->username;
  key.dsize = strlen(uinfo->aor->url->username)+1;
  data.dptr  = (void *)_string;
  data.dsize = strlen(_string)+1;

  i= dbm_store(dbm->dbm, key, data, DBM_INSERT);
  osip_free(_string);
  if (i!= 0)
    return -1;
  return 0;
}

/**
 * Delete values in dbm database.
 */
PPL_DECLARE (int) ppl_dbm_delete(ppl_dbm_t *dbm, char *_key)
{
  ppl_datum_t key;
  if (dbm==NULL||dbm->dbm==NULL)
    return -1;

  key.dptr  = (void *)_key;
  key.dsize = strlen(_key)+1;
  return dbm_delete(dbm->dbm, key);
}

/**
 * fetch info from the dbm database.
 */
PPL_DECLARE (int) ppl_dbm_fetch(ppl_dbm_t *dbm, char *_key, ppl_uinfo_t **uinfo)
{
  ppl_datum_t data;
  ppl_datum_t key;
  char *beg_url;
  char *end_url;
  char *tmp;
  osip_uri_t *url;
  int i;

  if (dbm==NULL||dbm->dbm==NULL||_key==NULL)
    return -1;

  key.dptr  = (void *)_key;
  key.dsize = strlen(_key)+1;
  data = dbm_fetch (dbm->dbm, key);

  *uinfo=NULL;
  if (data.dptr==NULL)
    return -1;
  if (*(data.dptr)=='\0'||data.dptr[1]=='\0')
    {
#if defined(HAVE_GDBM_H) || defined(HAVE_GDBM_NDBM_H)
      /* osip_free(data.dptr); */
#endif
      return -1;
    }
  end_url = strchr(data.dptr+2,'|');
  if (end_url==NULL)
    {
#if defined(HAVE_GDBM_H) || defined(HAVE_GDBM_NDBM_H)
      /* osip_free(data.dptr); */
#endif
      return -1;
    }

  if (data.dptr[1]=='|')
    {
      tmp = (char *) osip_malloc(end_url-data.dptr);
      osip_strncpy(tmp, data.dptr+2, end_url-data.dptr-2);
    }
  else
    {
      tmp = (char *) osip_malloc(end_url-data.dptr+2);
      osip_strncpy(tmp, data.dptr, end_url-data.dptr);
    }
  osip_uri_init(&url);
  i = osip_uri_parse(url,tmp);
  osip_free(tmp);
  if (i!=0)
    {
      osip_uri_free(url);
#if defined(HAVE_GDBM_H) || defined(HAVE_GDBM_NDBM_H)
      /* osip_free(data.dptr); */
#endif
      return -1;
    }

  *uinfo = ppl_uinfo_create(url, NULL, NULL);
  osip_uri_free(url);
  if (*uinfo==NULL)
    {
#if defined(HAVE_GDBM_H) || defined(HAVE_GDBM_NDBM_H)
      /* osip_free(data.dptr); */
#endif
      return -1;
    }
  (*uinfo)->status = 0;

  if (data.dptr[0]=='0')
    { /* ERROR? -> back to away? */
      (*uinfo)->status = 2;
    }
  else if (data.dptr[0]=='1')
    { /* online */
      (*uinfo)->status = 1;
    }
  else if (data.dptr[0]=='2')
    { /* away */
      (*uinfo)->status = 2;
    }
  else if (data.dptr[0]=='3')
    { /* busy */
      (*uinfo)->status = 3;
    }
  else if (data.dptr[0]=='4')
    { /* back in a few minutes */
      (*uinfo)->status = 4;
    }

  if (end_url[1]!='\0')
    {
      beg_url = end_url+1;
      end_url = strchr(beg_url,'|');
      while (end_url!=NULL)
	{
	  osip_contact_t *contact;
	  tmp = (char *) osip_malloc(end_url-beg_url+2);
	  osip_strncpy(tmp, beg_url, end_url-beg_url);
	  osip_contact_init(&contact);
	  i = osip_contact_parse(contact,tmp);
	  osip_free(tmp);
	  if (i!=0)
	    {
	      osip_contact_free(contact);
#if defined(HAVE_GDBM_H) || defined(HAVE_GDBM_NDBM_H)
	      /* osip_free(data.dptr); */
#endif
	      ppl_uinfo_remove(*uinfo);
	      *uinfo=NULL;
	      return -1;
	    }
	  ppl_uinfo_add_binding(*uinfo, contact, NULL);
	  osip_contact_free(contact);
	  
	  if (end_url[1]!='\0')
	    break;
	  beg_url = end_url+1;
	  end_url = strchr(beg_url,'|');
	  
	}
    }
#if defined(HAVE_GDBM_H) || defined(HAVE_GDBM_NDBM_H)
  /*  osip_free(data.dptr); */
#endif

  if ((*uinfo)->status == 1 && (*uinfo)->bindings==NULL)
     (*uinfo)->status = 2;

  return 0;
}

/**
 * close the dbm database.
 */
PPL_DECLARE (void) ppl_dbm_close(ppl_dbm_t *dbm)
{
  if (dbm==NULL||dbm->dbm==NULL)
    return;

  dbm_close (dbm->dbm);
}
#endif

#if !defined(HAVE_GDBM_H) && !defined(HAVE_GDBM_NDBM_H) && !defined(HAVE_DB_H) && !defined(HAVE_DB1_DB_H) && !defined(HAVE_DB2_DB_H) && !defined(HAVE_DB3_DB_H) && !defined(HAVE_NDBM_H) && !defined(HAVE_DBM_H)

/**
 * Intialize the uinfo list entry.
 */
PPL_DECLARE (int) ppl_dbm_open(ppl_dbm_t **dbm, char *dbfile, int flag)
{
  *dbm=NULL;
  return 0;
}

/**
 * Flush entry in file.
 */
PPL_DECLARE (int) ppl_dbm_flush(ppl_dbm_t *_dbm, int flag)
{
  return -1;
}

/**
 * Store values in dbm database.
 */
PPL_DECLARE (int) ppl_dbm_store(ppl_dbm_t *dbm, ppl_uinfo_t *uinfo)
{
  return -1;
}

/**
 * Insert values in dbm database.
 */
PPL_DECLARE (int) ppl_dbm_insert(ppl_dbm_t *dbm, ppl_uinfo_t *uinfo)
{
  return -1;
}

/**
 * Delete values in dbm database.
 */
PPL_DECLARE (int) ppl_dbm_delete(ppl_dbm_t *dbm, char *key)
{
  return -1;
}

/**
 * fetch info from the dbm database.
 */
PPL_DECLARE (int) ppl_dbm_fetch(ppl_dbm_t *dbm, char *key, ppl_uinfo_t **uinfo)
{
  *uinfo=NULL;
  return -1;
}

/**
 * close the dbm database.
 */
PPL_DECLARE (void) ppl_dbm_close(ppl_dbm_t *dbm)
{
  return ;
}

#endif
