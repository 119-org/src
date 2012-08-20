/*
  The auth plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The auth plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The auth plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <partysip/partysip.h>
#include <partysip/psp_config.h>
#include "auth.h"
#include <osipparser2/osip_parser.h>
#include <ppl/ppl_uinfo.h>

auth_ctx_t *auth_context = NULL;

extern psp_plugin_t PPL_DECLARE_DATA auth_plugin;

int
auth_ctx_init ()
{
  config_element_t *elem;
  elem = psp_config_get_sub_element ("force_use_of_407", "auth", NULL);
  auth_context = (auth_ctx_t *) osip_malloc (sizeof (auth_ctx_t));

  auth_context->force_use_of_407 = 1;
  if (elem != NULL && 0 == strncmp ("on", elem->value, 2))
    auth_context->force_use_of_407 = 2;
  else if (elem != NULL && 0 == strncmp ("detect", elem->value, 6))
    auth_context->force_use_of_407 = 0;

  if (auth_context == NULL)
    return -1;

  return 0;
}

void
auth_ctx_free ()
{
  if (auth_context == NULL)
    return;

  osip_free (auth_context);
  auth_context = NULL;
}

static int
auth_validate_credential_for_user (ppl_uinfo_t * user,
				   osip_proxy_authorization_t * p_auth,
				   char *method)
{
  /* find the pending_auth element */
  char *response;
  char *nonce;
  char *opaque;
  char *realm;

  /* char *qop; */
  char *uri;

  nonce = osip_proxy_authorization_get_nonce (p_auth);
  opaque = osip_proxy_authorization_get_opaque (p_auth);
  if (opaque == NULL || nonce == NULL)
    return -1;

  realm = osip_proxy_authorization_get_realm (p_auth);
  if (realm == NULL)
    return -1;
  response = osip_proxy_authorization_get_response (p_auth);
  if (response == NULL)
    return -1;
  uri = osip_proxy_authorization_get_uri (p_auth);
  if (uri == NULL)
    return -1;

  {
    /* verify credential */
    char *pszAlg = "MD5";
    char *pszUser = user->login;
    char *pszRealm = osip_strdup_without_quote (realm);
    char *pszPass = user->passwd;
    char *pszNonce = osip_strdup_without_quote (nonce);
    char *pszCNonce = NULL;

    char *szNonceCount = NULL;	/*  used with algo=md5-sess (not implemented) */
    char *pszMethod = method;
    char *pszURI = osip_strdup_without_quote (uri);
    char *pszQop = NULL;
    char *pszResponse = osip_strdup_without_quote (response);

    HASHHEX HA1;
    HASHHEX HA2 = "";
    HASHHEX Response;

    /*    if (qop!=NULL)
       pszQop = osip_strdup_without_quote(qop); */

    /* A1 = unq(username : unq(realm) : passwd ) */
    ppl_md5_DigestCalcHA1 (pszAlg, pszUser, pszRealm, pszPass, pszNonce,
			   pszCNonce, HA1);

    if (0==osip_strcasecmp(method, "ACK"))
      {
	ppl_md5_DigestCalcResponse (HA1, pszNonce, szNonceCount, pszCNonce,
				    pszQop, "INVITE", pszURI, HA2, Response);
      }
    else
      {
	ppl_md5_DigestCalcResponse (HA1, pszNonce, szNonceCount, pszCNonce,
				    pszQop, pszMethod, pszURI, HA2, Response);
      }
    osip_free (pszRealm);
    osip_free (pszNonce);
    osip_free (pszURI);
    osip_free (pszQop);

    OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
			    "auth plugin: authentication response:!\nlocal:%s remote:%s\n",
			    pszResponse, Response));

    if (0 == strcmp (pszResponse, Response))
      {
	osip_free (pszResponse);
	return 0;
      }
    osip_free (pszResponse);
  }
  return -1;
}

static ppl_uinfo_t *
auth_ctx_find_private_user (osip_proxy_authorization_t * p_auth)
{
  ppl_uinfo_t *user;
  char *username;
  char *qusername;

  /* search for a user context */
  qusername = osip_proxy_authorization_get_username (p_auth);
  if (qusername == NULL)
    return NULL;
  username = osip_strdup_without_quote (qusername);
  user = ppl_uinfo_find_by_login (username);
  if (user != NULL)
    {
      osip_free (username);
      return user;
    }
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_WARNING, NULL,
			  "auth plugin: Could not find user: %s\n",
			  username));
  osip_free (username);
  return NULL;
}

/* HOOK METHODS */

/*
  This method returns:
  -2 if plugin consider this request should be totally discarded!
  -1 on error
  0  nothing has been done
  1  things has been done on psp_req element
*/
int
cb_auth_validate_credentials (psp_request_t * psp_req)
{
  char *realm = psp_config_get_element ("serverrealm");
  ppl_uinfo_t *user;
  osip_proxy_authorization_t *p_auth;
  osip_authorization_t *h_auth;
  int i;
  int pos;
  int use407 = 0;
  osip_message_t *request;
  request = psp_request_get_request(psp_req);

  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
			  "auth plugin: Processing authentication in request!\n"));

  if (auth_context->force_use_of_407 == 0)	/* detect! */
    {
      osip_header_t *__ua;
      psp_request_set_uas_status (psp_req, 401);
      osip_message_get_user_agent (request, 0, &__ua);
      if (__ua != NULL && NULL != strstr (__ua->hvalue, "buggyUA"))
	{
	  use407 = 1;
	}
    }
  else if (auth_context->force_use_of_407 == 2)	/* on */
    {
      use407 = 1;
    }

  if (!MSG_IS_REGISTER (request) || use407 == 1)
    {
      /* default OUTPUT */
      psp_request_set_uas_status (psp_req, 407);
      psp_request_set_mode (psp_req, PSP_UAS_MODE);
      psp_request_set_state (psp_req, PSP_MANDATE);

      /* ANY proxy_authorization for us? */
      osip_message_get_proxy_authorization (request, 0, &p_auth);
      pos = 0;
      while (p_auth != NULL)	/* find the proxy_authorization for this proxy */
	{
	  if (0 != strcmp (p_auth->realm, realm))
	    {
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
				      "auth plugin: authentication header belongs to another proxy!\nlocal:%s remote:%s\n",
				      realm, p_auth->realm));
	    }
	  else
	    {
	      user = auth_ctx_find_private_user (p_auth);
	      if (user != NULL)
		{
		  i = auth_validate_credential_for_user (user, p_auth,
							 request->
							 sip_method);
		  if (i != 0)
		    {
		      OSIP_TRACE (osip_trace
				  (__FILE__, __LINE__, OSIP_WARNING, NULL,
				   "auth plugin: Bad credential for user!\n"));
		      /* may be we should answer 403 forbidden, because we have
		         detected that somebody tried to login! */
		      return 0;
		    }

		  psp_request_set_state (psp_req, PSP_CONTINUE);
		  /* remove the proxy_authorization header! */
		  osip_list_remove (&request->proxy_authorizations, pos);
		  osip_proxy_authorization_free (p_auth);

		  OSIP_TRACE (osip_trace
			      (__FILE__, __LINE__, OSIP_INFO4, NULL,
			       "auth plugin: Valid credential for user!\n"));
		  psp_request_set_mode (psp_req, PSP_SFULL_MODE);
		  psp_request_set_state (psp_req, PSP_CONTINUE);
		  return 0;	/* do nothing.. */
		}
	      /* it's my authentication header, but the username
	         is unknown to partysip */
	      psp_request_set_uas_status (psp_req, 403);
	      return 0;
	    }
	  pos++;
	  osip_message_get_proxy_authorization (request, pos, &p_auth);
	}
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_WARNING, NULL,
			      "auth plugin: Bad credential for user!\n"));
      return 0;
    }

  psp_request_set_uas_status (psp_req, 401);
  psp_request_set_mode (psp_req, PSP_UAS_MODE);
  psp_request_set_state (psp_req, PSP_MANDATE);

  /* ANY authorization for us? */
  osip_message_get_authorization (request, 0, &h_auth);
  pos = 0;
  while (h_auth != NULL)	/* find the authorization for this proxy */
    {
      if (0 != strcmp (h_auth->realm, realm))
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
				  "auth plugin: authentication header belongs to another proxy!\nlocal:%s remote:%s\n",
				  realm, h_auth->realm));
	}
      else
	{
	  user = auth_ctx_find_private_user (h_auth);
	  if (user != NULL)
	    {
	      i = auth_validate_credential_for_user (user, h_auth,
						     request->
						     sip_method);
	      if (i != 0)
		{
		  OSIP_TRACE (osip_trace
			      (__FILE__, __LINE__, OSIP_WARNING, NULL,
			       "auth plugin: Bad credential for user!\n"));
		  /* may be we should answer 403 forbidden, because we have
		     detected that somebody tried to login! */
		  return 0;
		}

	      /* remove the authorization header! */
	      osip_list_remove (&request->authorizations, pos);
	      osip_authorization_free (h_auth);

	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
				      "auth plugin: Valid credential for user!\n"));
	      psp_request_set_mode (psp_req, PSP_SFULL_MODE);
	      psp_request_set_state (psp_req, PSP_CONTINUE);
	      return 0;		/* do nothing.. */
	    }
	  /* it's my authentication header, but the username
	     is unknown to partysip */
	  psp_request_set_uas_status (psp_req, 403);
	  return 0;
	}
      pos++;
      osip_message_get_authorization (request, pos, &h_auth);
    }

  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_WARNING, NULL,
			  "auth plugin: Bad credential for user!\n"));
  return 0;
}


int
cb_auth_add_credentials (psp_request_t * psp_req, osip_message_t * response) /* HOOK MIDDLE */
{
  char *nonce;
  char *opaque;

  /*  char *qop; */
  char *realm;
  osip_proxy_authenticate_t *p_auth;
  osip_www_authenticate_t *w_auth;
  int status = psp_request_get_uas_status (psp_req);
  osip_message_t *request;
  request = psp_request_get_request(psp_req);

  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
			  "auth plugin: check if we need to add credentials in this 4xx!\n"));

  if (status != 407 && status != 401)
    {
      psp_request_set_state (psp_req, PSP_CONTINUE);
      return 0;
    }

  /* The request DOES not contains any credential */
  /* We have to challenge user */

  /* we just have to challenge the users */
  {
    MD5_CTX Md5Ctx;
    HASH HTMP;
    HASHHEX HTMPHex;
    int time_stamp;
    char *now;
    int i;

    /* build a nonce string */
    nonce = (char *) osip_malloc (HASHHEXLEN + 1 + 2);	/* +2 for the quotes */
    now = (char *) osip_malloc (30);
    time_stamp = ppl_time ();
    sprintf (now, "%i", time_stamp);
    ppl_MD5Init (&Md5Ctx);
    ppl_MD5Update (&Md5Ctx, (unsigned char *) now, strlen (now));
    osip_free (now);
    ppl_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
    now = psp_config_get_element ("magicstring2");
    ppl_MD5Update (&Md5Ctx, (unsigned char *) now, strlen (now));
    ppl_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
    ppl_MD5Update (&Md5Ctx, (unsigned char *) request->cseq->number,
		   strlen (request->cseq->number));
    ppl_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
    ppl_MD5Update (&Md5Ctx,
		   (unsigned char *) request->call_id->number,
		   strlen (request->call_id->number));
    ppl_MD5Final ((unsigned char *) HTMP, &Md5Ctx);
    ppl_md5_hash_osip_to_hex (HTMP, HTMPHex);
    sprintf (nonce, "\"%s\"", HTMPHex);

    /* build an opaque string */
    opaque = (char *) osip_malloc (HASHHEXLEN + 1 + 2);	/* +2 is for the quotes */
    now = (char *) osip_malloc (30);
    time_stamp = ppl_time ();
    sprintf (now, "%i", time_stamp);
    ppl_MD5Init (&Md5Ctx);
    ppl_MD5Update (&Md5Ctx, (unsigned char *) now, strlen (now));
    osip_free (now);
    ppl_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);
    ppl_MD5Final ((unsigned char *) HTMP, &Md5Ctx);
    ppl_md5_hash_osip_to_hex (HTMP, HTMPHex);
    sprintf (opaque, "\"%s\"", HTMPHex);

    /*    qop = osip_malloc(9);
       sprintf(qop, "\"auth\""); */

    realm = osip_strdup (psp_config_get_element ("serverrealm"));

    /* store data in a auth_ctx_t context */

    if (status == 407)
      {
	i = osip_proxy_authenticate_init (&p_auth);
	if (i != 0)
	  return -1;
	osip_proxy_authenticate_set_auth_type (p_auth, osip_strdup ("Digest"));
	/*      osip_proxy_authenticate_set_qop_options(p_auth, qop); */
	osip_proxy_authenticate_set_nonce (p_auth, nonce);
	/* AMD!!!         osip_proxy_authenticate_set_opaque (p_auth, opaque); */
	osip_proxy_authenticate_set_realm (p_auth, realm);
	osip_proxy_authenticate_set_opaque (p_auth, opaque);

	{
	  osip_header_t *__ua;
	  osip_message_get_user_agent (request, 0, &__ua);
	  /* except for msn? which other need it? */
	  if (__ua != NULL && NULL != strstr (__ua->hvalue, "buggyUA"))
	    {
	      char *domain;
	      char *domain2;
	      i = osip_uri_to_str (request->req_uri, &domain);
	      if (i != 0)
		return -1;
	      domain2 = (char *) osip_malloc (strlen (domain) + 3);
	      sprintf (domain2, "\"%s\"", domain);
	      osip_proxy_authenticate_set_domain (p_auth, domain2);
	      osip_proxy_authenticate_set_algorithm_MD5 (p_auth);
	      osip_free (domain);
	    }
	}

	osip_list_add (&response->proxy_authenticates, p_auth, -1);

	psp_request_set_state (psp_req, PSP_CONTINUE);
	return 0;
      }
    else if (status == 401)
      {
	i = osip_www_authenticate_init (&w_auth);
	if (i != 0)
	  return -1;
	osip_www_authenticate_set_auth_type (w_auth, osip_strdup ("Digest"));
	/*      osip_www_authenticate_set_qop_options(w_auth, qop); */
	osip_www_authenticate_set_nonce (w_auth, nonce);
	osip_www_authenticate_set_opaque (w_auth, opaque);
	osip_www_authenticate_set_realm (w_auth, realm);

	osip_list_add (&response->www_authenticates, w_auth, -1);

	psp_request_set_state (psp_req, PSP_CONTINUE);
	return 0;
      }
  }
  psp_request_set_state (psp_req, PSP_CONTINUE);
  return 0;
}
