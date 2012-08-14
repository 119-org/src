/*
  The ls_sfull plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The ls_sfull plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The ls_sfull plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <partysip/partysip.h>
#include "ls_sfull.h"

ls_sfull_ctx_t *ls_sfull_context = NULL;

extern psp_plugin_t PPL_DECLARE_DATA ls_sfull_plugin;
extern char sfull_name_config[50];

/*
  This plugin can be configured to:
  * act as a redirect server.
  *
  * It can also be configured to mandate record-routing.
  */

/*
  <sfull>
  mode redirect
  mode statefull
  record-route on
  </sfull>
*/
#define REDIRECT_MODE      0x01

#define ISSET_REDIRECT_MODE(flag)    ((~flag|~REDIRECT_MODE)==~REDIRECT_MODE)
#define SET_REDIRECT_MODE(flag)      (flag= flag|REDIRECT_MODE)

#define R_ROUTE_MODE       0x10

#define ISSET_R_ROUTE_MODE(flag)     ((~flag|~R_ROUTE_MODE)==~R_ROUTE_MODE)
#define SET_R_ROUTE_MODE(flag)       (flag= flag|R_ROUTE_MODE)

int
ls_sfull_ctx_init ()
{
  config_element_t *elem;

  ls_sfull_context = (ls_sfull_ctx_t *) osip_malloc (sizeof (ls_sfull_ctx_t));
  if (ls_sfull_context == NULL)
    return -1;

  ls_sfull_context->flag = 0;
  elem = psp_config_get_sub_element ("mode", sfull_name_config, NULL);
  if (elem == NULL || elem->value == NULL)
    {
    }
  else if (0 == strcmp (elem->value, "redirect"))
    SET_REDIRECT_MODE (ls_sfull_context->flag);
  else if (0 == strcmp (elem->value, "statefull"))
    {
    }
  else
    goto lsci_error1;		/* error, bad option */

  elem = psp_config_get_sub_element ("record-route", sfull_name_config, NULL);
  if (elem == NULL || elem->value == NULL)
    {
    }
  else if (0 == strcmp (elem->value, "off"))
    {
    }
  else if (0 == strcmp (elem->value, "on"))
    SET_R_ROUTE_MODE (ls_sfull_context->flag);
  else
    goto lsci_error1;		/* error, bad option */


  if (ISSET_REDIRECT_MODE (ls_sfull_context->flag))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "ls_sfull plugin: configured to do redirect request!\n"));
    }

  if (ISSET_R_ROUTE_MODE (ls_sfull_context->flag))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "ls_sfull plugin: configured to do record-routing!\n"));
    }

  return 0;
lsci_error1:
  ls_sfull_ctx_free ();
  return -1;
}

void
ls_sfull_ctx_free ()
{
  if (ls_sfull_context == NULL)
    return;

  osip_free (ls_sfull_context);
  ls_sfull_context = NULL;
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
cb_ls_sfull_search_location (psp_request_t * psp_req)
{
  osip_route_t *route;
  osip_uri_t *url;
  location_t *loc;
  int i;
  osip_message_t *request;
  request = psp_request_get_request(psp_req);

  /* default OUTPUT */
  if (ISSET_R_ROUTE_MODE (ls_sfull_context->flag))
    psp_request_set_property (psp_req, PSP_STAY_ON_PATH);
  else
    psp_request_set_property (psp_req, 0);

  if (!ISSET_REDIRECT_MODE (ls_sfull_context->flag))
    /* state full (default mode) */
    psp_request_set_mode (psp_req, PSP_SFULL_MODE);
  else
    {				/* redirect mode */
      psp_request_set_uas_status (psp_req, 302);
      psp_request_set_mode (psp_req, PSP_UAS_MODE);
    }
  psp_request_set_state (psp_req, PSP_MANDATE);

  i = 0;
  for (; !osip_list_eol (&request->routes, i); i++)
    {
      osip_message_get_route (request, i, &route);
      if (0 != psp_core_is_responsible_for_this_route (route->url))
	{
	  psp_request_set_mode (psp_req, PSP_SFULL_MODE);
	  psp_request_set_state (psp_req, PSP_MANDATE);
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
				  "ls_sfull plugin: mandate statefull handling for route.\n"));
	  return 0;
	}
    }
  psp_request_set_state (psp_req, PSP_MANDATE);

  if (i > 1)
    {
      psp_request_set_uas_status (psp_req, 482);	/* loop? */
      psp_request_set_mode (psp_req, PSP_UAS_MODE);
      return 0;
    }
  if (i == 1)
    {
      if (0 ==
	  psp_core_is_responsible_for_this_domain (request->req_uri))
	{
	  psp_request_set_state (psp_req, PSP_PROPOSE);	/* ls_localdb will handle it */
	  psp_request_set_uas_status (psp_req, 404);
	  psp_request_set_mode (psp_req, PSP_UAS_MODE);
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_WARNING, NULL,
				  "ls_sfull plugin: Useless Route in SIP request.\n"));
	  return 0;
	}
    }

  /* in the case where we detect an IP address in the request-URI
     which is not the local proxy, then we should forward it to
     this destination. */
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
			  "ls_sfull plugin: checking if we are responsible for request-URI '%s'\n",
			  request->req_uri->host));
  if (0 ==
      psp_core_is_responsible_for_this_request_uri (request->req_uri))
    {
      psp_request_set_state (psp_req, PSP_CONTINUE);
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "ls_sfull plugin: ignore this request which belongs to this domain.\n"));
      return 0;
    }

  /* we MUST add ONE a location in psp_req */
  i = osip_uri_clone (request->req_uri, &url);
  if (i != 0)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "ls_sfull plugin: Could not clone request-uri!\n"));
      psp_request_set_uas_status (psp_req, 400);
      psp_request_set_mode (psp_req, PSP_UAS_MODE);
      psp_request_set_state (psp_req, PSP_MANDATE);
      return -1;		/* error case (process can continue...) */
    }
  i = location_init (&loc, url, 3600);
  if (i != 0)
    {				/* This can only happen in case we don't have enough memory */
      osip_uri_free (url);
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_BUG, NULL,
			      "ls_sfull plugin: Could not create location info!\n"));
      psp_request_set_uas_status (psp_req, 400);
      psp_request_set_mode (psp_req, PSP_UAS_MODE);
      psp_request_set_state (psp_req, PSP_MANDATE);
      return -1;		/* error case (process can continue...) */
    }
  ADD_ELEMENT (psp_req->locations, loc);
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			  "ls_sfull plugin: mandate statefull handling for route.\n"));

  return 0;
}
