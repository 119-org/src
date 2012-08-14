/*
  The rgstrar plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The rgstrar plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The rgstrar plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <partysip/partysip.h>
#include "rgstrar.h"
#include <osipparser2/osip_parser.h>
#include <ppl/ppl_uinfo.h>

rgstrar_ctx_t *rgstrar_context = NULL;

extern psp_plugin_t PPL_DECLARE_DATA rgstrar_plugin;

int
rgstrar_ctx_init ()
{
  rgstrar_context = (rgstrar_ctx_t *) osip_malloc (sizeof (rgstrar_ctx_t));
  if (rgstrar_context == NULL)
    return -1;

  return 0;
}

void
rgstrar_ctx_free ()
{
  if (rgstrar_context == NULL)
    return;

  osip_free (rgstrar_context);
  rgstrar_context = NULL;
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
cb_rgstrar_update_contact_list (psp_request_t * psp_req)
{
  ppl_uinfo_t *user;
  osip_contact_t *co;
  int i;
  int pos = 0;
  char *expires;
  osip_header_t *head;
  osip_message_t *request;
  request = psp_request_get_request(psp_req);

  expires = NULL;

  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
			  "rgstrar plugin: Updating contact list of a user!\n"));

  if (request->to==NULL
      || request->to->url==NULL
      || request->to->url->username==NULL)
    {
      psp_request_set_uas_status (psp_req, 403);
      psp_request_set_mode (psp_req, PSP_UAS_MODE);
      psp_request_set_state (psp_req, PSP_MANDATE);
      return 0;
    }

  user = ppl_uinfo_find_by_aor (request->to->url);
  if (user == NULL)
    user = ppl_uinfo_create (request->to->url, NULL, NULL);
  if (user == NULL)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "rgstrar plugin: Address of Record is not valid!\n"));
      psp_request_set_uas_status (psp_req, 400);
      psp_request_set_mode (psp_req, PSP_UAS_MODE);
      psp_request_set_state (psp_req, PSP_MANDATE);
      return 0;
    }

  i = osip_message_get_expires (request, 0, &head);
  if (i >= 0)
    expires = head->hvalue;

  /* rfc3327.txt:
     Extension Header Field for Registering Non-Adjacent Contacts */
  /* build the complete path header */
  {
    osip_header_t *hpath;
    char path[500]; /* should be enough! */
    int  max_length = 499;
    *path = '\0';
    pos = 0;
    pos = osip_message_header_get_byname (request, "path", pos, &hpath);
    while (pos>=0)
      {
	if (hpath->hvalue==NULL)
	  {
	    OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_WARNING, NULL,
				    "Empty pass!!\n"));
	  }
	else
	  {
	    int header_length = strlen(hpath->hvalue);
	    OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
				    "Path header found: %s\n", hpath->hvalue));
	    if (499-strlen(path)<header_length)
	      { /* path header too long, discard other entries */
	      }
	    else
	      {
		if (max_length==499) /* don't add a coma */
		  osip_strncpy(path, hpath->hvalue, header_length);
		else
		  {
		    osip_strncpy(path+strlen(path), ",", 1);
		    osip_strncpy(path+strlen(path), hpath->hvalue, header_length);
		  }
		max_length = 499 - strlen(path);
	      }
	  }
	pos++;
	pos = osip_message_header_get_byname (request, "path", pos, &hpath);
      }

    pos = 0;
    while (!osip_list_eol (&request->contacts, pos))
      {
	co = osip_list_get (&request->contacts, pos);
	if (*path=='\0')
	  i = ppl_uinfo_add_binding_with_path (user, co, expires, NULL);
	else
	  i = ppl_uinfo_add_binding_with_path (user, co, expires, path);
	if (i != 0)
	  {
	    OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				    "rgstrar plugin: A contact header is not correct (user is removed)!\n"));
	    psp_request_set_uas_status (psp_req, 400);
	    psp_request_set_mode (psp_req, PSP_UAS_MODE);
	    psp_request_set_state (psp_req, PSP_MANDATE);
	    ppl_uinfo_store_bindings(user);
	    return 0;
	  }
	pos++;
      }
  }

  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			  "INFO: rgstrar plugin: User Registration saved!\n"));
  psp_request_set_uas_status (psp_req, 200);
  psp_request_set_mode (psp_req, PSP_UAS_MODE);
  psp_request_set_state (psp_req, PSP_MANDATE);

  return 0;
}

/* only called for 2xx response */
int
cb_rgstrar_add_contacts_in_register (psp_request_t * psp_req, osip_message_t *response)
{
  osip_message_t *request;
  request = psp_request_get_request(psp_req);

  if (MSG_IS_REGISTER (request))
    {
      osip_header_t *head;
      int add_expire;

      osip_contact_t *co;
      osip_contact_t *co2;
      binding_t *bnext;
      binding_t *b;
      ppl_uinfo_t *user;	/* should be saved in psp_req.. */

      int i;

      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
			      "rgstrar plugin: Adding bindings in 200 OK for REGISTER!!\n"));

      user = ppl_uinfo_find_by_aor (request->to->url);
      if (user == NULL)
	{
	  /* TODO: REWRITE ERROR CODE AS 403 Forbidden */
	  psp_request_set_state (psp_req, PSP_STOP);
	  psp_request_set_mode (psp_req, PSP_UAS_MODE);	/* not needed!! */
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_BUG, NULL,
				  "rgstrar plugin: user does not exist for a registration.\n"));
	  return -2;
	}

      co = osip_list_get (&request->contacts, 0);
      if (co != NULL && co->displayname != NULL)
	{
	  if (0 == strcmp (co->displayname, "*"))
	    {
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO3, NULL,
				      "rgstrar plugin: removing user contact list.\n"));
	      ppl_uinfo_remove_all_bindings (user);
	      psp_request_set_state (psp_req, PSP_CONTINUE);
	      osip_message_set_expires (response, "0");
	      ppl_uinfo_store_bindings(user);
	      return 0;
	    }
	}

      for (bnext = user->bindings; bnext != NULL;)
	{
	  b = bnext;
	  bnext = b->next;
	  i = ppl_uinfo_check_binding (b);
	  if (i != 0)
	    {			/* binding is expired */
	      ppl_uinfo_remove_binding (user, b);
	    }
	  else
	    {
	      i = osip_contact_clone (b->contact, &co2);
	      if (i != 0)
		{
		  ppl_uinfo_remove_all_bindings (user);
		  /* TODO: REWRITE ERROR CODE AS 400 Bad Request */
		  psp_request_set_state (psp_req, PSP_STOP);
		  psp_request_set_mode (psp_req, PSP_UAS_MODE);	/* not needed!! */
		  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_BUG, NULL,
					  "rgstrar plugin: Request is corrupted (Bad contact)!\n"));
		  ppl_uinfo_store_bindings(user);
		  return -1;	/* ask the core application to discard the request */
		}
	      osip_list_add (&response->contacts, co2, -1);
	    }
	}

      add_expire = 0;
      i = osip_message_get_expires (response, 0, &head);
      if (i < 0)
	{
	  /* not a global header, look in each contact field. */
	  int pos;

	  pos = 0;
	  while (!osip_list_eol (&response->contacts, pos))
	    {
	      osip_generic_param_t *exp;

	      co = osip_list_get (&response->contacts, pos);
              i = osip_contact_param_get_byname (co, "expires", &exp);
	      if (exp == NULL)	/* at least, on expire is missing */
		{
		  add_expire = 1;
		  break;
		}
	      pos++;
	    }

	  if (add_expire == 1)
	    osip_message_set_expires (response, "3600");
	}
      ppl_uinfo_store_bindings(user);
    }

  psp_request_set_state (psp_req, PSP_CONTINUE);
  return 0;
}
