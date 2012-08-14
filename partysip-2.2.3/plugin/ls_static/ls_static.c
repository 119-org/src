/*
  The ls_static plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The ls_static plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The ls_static plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <partysip/partysip.h>
#include <partysip/psp_utils.h>

#include <ppl/ppl_dns.h>

#include "ls_static.h"

ls_static_ctx_t *ls_static_context = NULL;

extern psp_plugin_t PPL_DECLARE_DATA ls_static_plugin;

/*
  This plugin can be configured to:
  * act as a redirect server.

  * reject all messages with unknown Request-URI.

  * mandate statefull for REQUEST with a recognized IP/FQDN in the host of the request-uri

  * mandate record-routing to stay on the path for the duration of the call.
       ("proxy-mode")

  */

/*
  Configuration sample:.
  
  <static>

  mode          statefull
  # record-route  on

  forward    192.168.1.101    192.168.1.56
  forward    wellx.com        192.168.1.56

  reject     sip.no-ip.com    403

  #with this entry, the next plugins are never called!!
  reject     *                403
  
  </static>
*/

#define REDIRECT_MODE      0x10
#define R_ROUTE_MODE       0x01

#define ISSET_REDIRECT_MODE(flag)    ((~flag|~REDIRECT_MODE)==~REDIRECT_MODE)
#define SET_REDIRECT_MODE(flag)      (flag=flag|REDIRECT_MODE)

#define ISSET_R_ROUTE_MODE(flag)     ((~flag|~R_ROUTE_MODE)==~R_ROUTE_MODE)
#define SET_R_ROUTE_MODE(flag)       (flag=flag|R_ROUTE_MODE)


static int
ls_static_load_forward_config ()
{
  config_element_t *elem;
  config_element_t *next_elem;
  char *ip1;
  char *ip2;
  int i;

  elem = psp_config_get_sub_element ("forward", "static", NULL);

  while (elem != NULL)
    {
      config_element_t *cfgel_new;
      /*
         forward [ip1] [ip2]
       */
      i = psp_util_get_and_set_next_token (&ip1, elem->value, &ip2);
      if (i != 0 || ip1 == NULL)
	return -1;
      osip_clrspace (ip1);
      osip_clrspace (ip2);

      cfgel_new = (config_element_t *) osip_malloc (sizeof (config_element_t));
      cfgel_new->next = NULL;
      cfgel_new->parent = NULL;
      cfgel_new->name = ip1;
      cfgel_new->value = osip_strdup (ip2);
      cfgel_new->sub_config = NULL;
      ADD_ELEMENT (ls_static_context->elem_forward, cfgel_new);

      next_elem = elem;
      if (next_elem == NULL)
	return 0;
      elem = psp_config_get_sub_element ("forward", "static", next_elem);
    }
  return 0;
}

static int
ls_static_load_reject_config ()
{
  config_element_t *elem;
  config_element_t *next_elem;
  char *ip1;
  char *code;
  int i;

  elem = psp_config_get_sub_element ("reject", "static", NULL);

  while (elem != NULL)
    {
      config_element_t *cfgel_new;
      /*
         reject [ip1] [code]
       */
      i = psp_util_get_and_set_next_token (&ip1, elem->value, &code);
      if (i != 0 || ip1 == NULL)
	return -1;
      osip_clrspace (ip1);
      osip_clrspace (code);

      cfgel_new = (config_element_t *) osip_malloc (sizeof (config_element_t));
      cfgel_new->next = NULL;
      cfgel_new->parent = NULL;
      cfgel_new->name = ip1;
      cfgel_new->value = osip_strdup (code);
      cfgel_new->sub_config = NULL;
      ADD_ELEMENT (ls_static_context->elem_reject, cfgel_new);

      next_elem = elem;
      if (next_elem == NULL)
	return 0;
      elem = psp_config_get_sub_element ("reject", "static", next_elem);
    }
  return 0;
}

int
ls_static_ctx_init ()
{
  config_element_t *elem;
  int i;

  ls_static_context = (ls_static_ctx_t *) osip_malloc (sizeof (ls_static_ctx_t));
  if (ls_static_context == NULL)
    return -1;

  ls_static_context->elem_forward = NULL;
  ls_static_context->elem_reject = NULL;

  ls_static_context->flag = 0;
  elem = psp_config_get_sub_element ("mode", "static", NULL);
  if (elem == NULL || elem->value == NULL)
    {
    }
  else if (0 == strcmp (elem->value, "redirect"))
    SET_REDIRECT_MODE (ls_static_context->flag);
  else if (0 == strcmp (elem->value, "statefull"))
    {
    }
  else
    goto lsci_error1;		/* error, bad option */

  elem = psp_config_get_sub_element ("record-route", "static", NULL);
  if (elem == NULL || elem->value == NULL)
    {
    }
  else if (0 == strcmp (elem->value, "off"))
    {
    }
  else if (0 == strcmp (elem->value, "on"))
    SET_R_ROUTE_MODE (ls_static_context->flag);
  else
    goto lsci_error1;		/* error, bad option */

  /* load the configuration behavior for INVITE */
  i = ls_static_load_forward_config ();
  if (i != 0)
    goto lsci_error2;

  i = ls_static_load_reject_config ();
  if (i != 0)
    goto lsci_error2;

  if (ISSET_R_ROUTE_MODE (ls_static_context->flag))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "ls_static plugin: configured to do record-routing!\n"));
    }

  if (ISSET_REDIRECT_MODE (ls_static_context->flag))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "ls_static plugin: configured in redirect mode!\n"));
    }

  return 0;

lsci_error2:
lsci_error1:
  osip_free (ls_static_context);
  ls_static_context = NULL;
  return -1;
}

void
ls_static_ctx_free ()
{
  config_element_t *elem;
  if (ls_static_context == NULL)
    return;

  for (elem = ls_static_context->elem_forward;
       elem != NULL; elem = ls_static_context->elem_forward)
    {
      REMOVE_ELEMENT (ls_static_context->elem_forward, elem);
      osip_free (elem->name);
      osip_free (elem->value);
      osip_free (elem);
    }
  for (elem = ls_static_context->elem_reject;
       elem != NULL; elem = ls_static_context->elem_reject)
    {
      REMOVE_ELEMENT (ls_static_context->elem_reject, elem);
      osip_free (elem->name);
      osip_free (elem->value);
      osip_free (elem);
    }

  osip_free (ls_static_context);
  ls_static_context = NULL;
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
cb_ls_static_search_location (psp_request_t * psp_req)
{
  osip_route_t *route;
  config_element_t *elem;
  location_t *loc;
  osip_uri_t *url;
  int i;
  osip_message_t *request;
  request = psp_request_get_request(psp_req);

  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			  "ls_static plugin: entering cb_ls_static_search_location\n"));

  /* default OUTPUT */
  if (ISSET_R_ROUTE_MODE (ls_static_context->flag))
    psp_request_set_property (psp_req, PSP_STAY_ON_PATH);
  else
    psp_request_set_property (psp_req, 0);

  if (!ISSET_REDIRECT_MODE (ls_static_context->flag))
    /* state full or default mode */
    psp_request_set_mode (psp_req, PSP_SFULL_MODE);
  else
    {
      psp_request_set_uas_status (psp_req, 302);
      psp_request_set_mode (psp_req, PSP_UAS_MODE);
    }

  i = 0;
  for (; !osip_list_eol (&request->routes, i); i++)
    {
      osip_message_get_route (request, i, &route);
      if (0 != psp_core_is_responsible_for_this_route (route->url))
	{
	  psp_request_set_mode (psp_req, PSP_SFULL_MODE);
	  psp_request_set_state (psp_req, PSP_MANDATE);
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
				  "ls_static plugin: mandate statefull handling for route.\n"));
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
      osip_uri_param_t *psp_param;
      osip_message_get_route (request, 0, &route);	/* should be the first one */
      /* if this route header contains the "psp" parameter, it means
         the element does not come from a pre-route-set header (in this
         last case, we want to execute the plugin for the initial request) */
      /* NON compliant UAs (not returning this parameter) are guilty. */
      osip_uri_uparam_get_byname (route->url, "psp", &psp_param);
      if (psp_param != NULL)
	{
	  psp_request_set_state (psp_req, PSP_MANDATE);
	  psp_request_set_mode (psp_req, PSP_SFULL_MODE);
	  /* got it, leave this plugin. */
	  return 0;
	}
    }

  /* depending on which behavior we want, we should set the MANDATE or
     PROPOSE state. In one case, further plugins will never be called */

  psp_request_set_state (psp_req, PSP_MANDATE);

  for (elem = ls_static_context->elem_forward; elem != NULL;
       elem = elem->next)
    {
      if (request->req_uri != NULL
	  && request->req_uri->host != NULL
	  && 0 == strcmp (request->req_uri->host,
			  elem->name))
	{			/* found a match: forward to elem->value */
	  i = osip_uri_clone (request->req_uri, &url);
	  if (i != 0)
	    {
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				      "ls_static plugin: Could not clone request-uri!\n"));
	      psp_request_set_uas_status (psp_req, 400);
	      psp_request_set_mode (psp_req, PSP_UAS_MODE);
	      psp_request_set_state (psp_req, PSP_MANDATE);
	      return -1;
	    }
	  /* replace with the new IP from elem->value */
	  osip_free (url->host);
	  url->host = osip_strdup (elem->value);
	  i = location_init (&loc, url, 3600);
	  if (i != 0)
	    {			/* This can only happen in case we don't have enough memory */
	      osip_uri_free (url);
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_BUG, NULL,
				      "ls_static plugin: Could not create location info!\n"));
	      psp_request_set_uas_status (psp_req, 400);
	      psp_request_set_mode (psp_req, PSP_UAS_MODE);
	      psp_request_set_state (psp_req, PSP_MANDATE);
	      return -1;
	    }
	  ADD_ELEMENT (psp_req->locations, loc);
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
				  "ls_static plugin: mandate statefull (or redirect) mode for request.\n"));
	  return 0;
	}
    }

  for (elem = ls_static_context->elem_reject; elem != NULL; elem = elem->next)
    {
      if ((request->req_uri != NULL
	   && request->req_uri->host != NULL
	   && 0 == strcmp (request->req_uri->host,
			   elem->name)) || 0 == strcmp ("*", elem->name))
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
				  "ls_static plugin: REJECTING request with code: %i\n",
				  osip_atoi (elem->value)));
	  psp_request_set_uas_status (psp_req, osip_atoi (elem->value));
	  psp_request_set_mode (psp_req, PSP_UAS_MODE);
	  return 0;
	}
    }
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			  "ls_static plugin: Didn't do anything with this request?\n"));
  psp_request_set_state (psp_req, PSP_PROPOSE);
  psp_request_set_uas_status (psp_req, 404);
  psp_request_set_mode (psp_req, PSP_UAS_MODE);

  return 0;
}
