/*
  The ls_localdb plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The ls_localdb plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The ls_localdb plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <partysip/partysip.h>
#include "ls_localdb.h"
#include <ppl/ppl_uinfo.h>

ls_localdb_ctx_t *ls_localdb_context = NULL;

extern psp_plugin_t PPL_DECLARE_DATA ls_localdb_plugin;
extern char localdb_name_config[50];


/*
  This plugin can be configured to:
  * act as a redirect server.
  *
  * It can also be configured to mandate record-routing.
  */

/*
  Configuration sample for REQUEST processing.

  <localdb>
  #  mode sf_forking
  #  mode sf_sequential
  #  mode redirect
  record-route on
  </localdb>
*/

#define REDIRECT_MODE       0x01
/* #define STATEFULL_MODE      0x04 */
#define FORKING_MODE        0x04
#define SEQ_MODE            0x08


#define ISSET_REDIRECT_MODE(flag)   ((~flag|~REDIRECT_MODE)==~REDIRECT_MODE)
#define SET_REDIRECT_MODE(flag)     (flag=((flag&0x0FF)|REDIRECT_MODE))
/*
  #define ISSET_STATEFULL_MODE(flag) ((~flag|~STATEFULL_MODE)==~STATEFULL_MODE)
  #define SET_STATEFULL_MODE(flag)   (flag=((flag&0x0FF)|STATEFULL_MODE))
*/
#define ISSET_FORKING_MODE(flag)  ((~flag|~FORKING_MODE)==~FORKING_MODE)
#define SET_FORKING_MODE(flag)    (flag=((flag&0x0FF)|FORKING_MODE))
#define ISSET_SEQUENTIAL_MODE(flag) ((~flag|~SEQ_MODE)==~SEQ_MODE)
#define SET_SEQUENTIAL_MODE(flag)    (flag=((flag&0x0FF)|SEQ_MODE))

#define R_ROUTE_MODE        0x10

#define ISSET_R_ROUTE_MODE(flag)     ((~flag|~R_ROUTE_MODE)==~R_ROUTE_MODE)
#define SET_R_ROUTE_MODE(flag)       (flag= flag|R_ROUTE_MODE)

int
ls_localdb_ctx_init ()
{
  config_element_t *elem;

  ls_localdb_context =
    (ls_localdb_ctx_t *) osip_malloc (sizeof (ls_localdb_ctx_t));
  if (ls_localdb_context == NULL)
    return -1;

  ls_localdb_context->flag = 0;
  elem = psp_config_get_sub_element ("mode", localdb_name_config, NULL);
  if (elem == NULL || elem->value == NULL)
    SET_FORKING_MODE (ls_localdb_context->flag);
  else if (0 == strcmp (elem->value, "redirect"))
    SET_REDIRECT_MODE (ls_localdb_context->flag);
  else if (0 == strcmp (elem->value, "sf_forking"))
    SET_FORKING_MODE (ls_localdb_context->flag);
  else if (0 == strcmp (elem->value, "statefull"))
    SET_FORKING_MODE (ls_localdb_context->flag);
  else if (0 == strcmp (elem->value, "sf_sequential"))
    SET_SEQUENTIAL_MODE (ls_localdb_context->flag);
  else
    goto llci_error1;		/* error, bad option */

  elem = psp_config_get_sub_element ("record-route", localdb_name_config, NULL);
  if (elem == NULL || elem->value == NULL)
    {				/* default mode is off */
    }
  else if (0 == strcmp (elem->value, "off"))
    {
    }
  else if (0 == strcmp (elem->value, "on"))
    SET_R_ROUTE_MODE (ls_localdb_context->flag);
  else
    goto llci_error1;		/* error, bad option */

  if (ISSET_REDIRECT_MODE (ls_localdb_context->flag))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "ls_localdb plugin: configured to do redirect request!\n"));
    }
  if (ISSET_FORKING_MODE (ls_localdb_context->flag))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "ls_localdb plugin: configured to run in statefull forking mode!\n"));
    }
  if (ISSET_SEQUENTIAL_MODE (ls_localdb_context->flag))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "ls_localdb plugin: configured to run in statefull sequential mode!\n"));
    }

  if (ISSET_R_ROUTE_MODE (ls_localdb_context->flag))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "ls_localdb plugin: configured to do record-routing!\n"));
    }

  return 0;

llci_error1:
  ls_localdb_ctx_free ();
  return -1;
}

void
ls_localdb_ctx_free ()
{
  if (ls_localdb_context == NULL)
    return;

  osip_free (ls_localdb_context);
  ls_localdb_context = NULL;
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
cb_ls_localdb_search_user_location (psp_request_t * psp_req)
{
  osip_route_t *route;
  ppl_uinfo_t *uinfo;
  int i;
  int numlocs = 0 /* DAB */ ;
  osip_message_t *request;
  request = psp_request_get_request(psp_req);

  /* default OUTPUT */
  if (ISSET_R_ROUTE_MODE (ls_localdb_context->flag))
    psp_request_set_property (psp_req, PSP_STAY_ON_PATH);
  else
    psp_request_set_property (psp_req, 0);

  /* mode */
  if (ISSET_FORKING_MODE (ls_localdb_context->flag))
    psp_request_set_mode (psp_req, PSP_FORK_MODE);
  else if (ISSET_SEQUENTIAL_MODE (ls_localdb_context->flag))
    psp_request_set_mode (psp_req, PSP_SEQ_MODE);
  else if (ISSET_REDIRECT_MODE (ls_localdb_context->flag))
    {
      psp_request_set_uas_status (psp_req, 302);
      psp_request_set_mode (psp_req, PSP_UAS_MODE);
    }
  else
    psp_request_set_mode (psp_req, PSP_FORK_MODE);

  psp_request_set_state (psp_req, PSP_MANDATE);

  /* THIS IS MANDATORY FOR ALL PLUGINS TO CHECK THAT!
     this is rarely used as a previous plugin 'ls_localdb' should have
     checked for it. In case you remove it. */
  osip_message_get_route (request, 1, &route);
  if (route != NULL)
    {
      if (ISSET_SEQUENTIAL_MODE (ls_localdb_context->flag))
	psp_request_set_mode (psp_req, PSP_SEQ_MODE);
      else
	psp_request_set_mode (psp_req, PSP_FORK_MODE);
      psp_request_set_state (psp_req, PSP_MANDATE);
      return 0;			/* do nothing.. */
    }

  if (request->req_uri->username == NULL)
    {
      /* Propose 484, but let's keep searching if another plugin can find
	 the destination. */
      psp_request_set_uas_status (psp_req, 484);
      psp_request_set_state (psp_req, PSP_CONTINUE);
      psp_request_set_mode (psp_req, PSP_UAS_MODE);
      return 0;
    }

  /* look for this URI in our list of users */
  uinfo = ppl_uinfo_find_by_aor (request->req_uri);
  if (uinfo != NULL)
    {
      /* add all locations to the psp_req element */
      binding_t *bind;
      binding_t *bindnext;
      location_t *loc;
      osip_uri_t *url;

      bind = uinfo->bindings;
      bindnext = uinfo->bindings;
      for (; bind != NULL; bind = bindnext)
	{
	  bindnext = bind->next;
	  i = ppl_uinfo_check_binding (bind);
	  if (i != 0)
	    {			/* binding is expired */
	      ppl_uinfo_remove_binding (uinfo, bind);
	    }
	}
      bind = uinfo->bindings;
      if (bind == NULL)
	{			/* user is not around... */
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				  "ls_localdb plugin: User Exist but has no valid registration!\n"));
	  psp_request_set_uas_status (psp_req, 480);
	  psp_request_set_mode (psp_req, PSP_UAS_MODE);
	  psp_request_set_state (psp_req, PSP_MANDATE);
	  return 0;
	}

      bindnext = uinfo->bindings;	/* DAB */
      for (; bind != NULL; bind = bindnext)	/* DAB */
	{			/* DAB */
	  /* If this is an INVITE Request,  collect all locations  DAB */
#ifdef EXPERIMENTAL_FORK
	  if (MSG_IS_INVITE (request))
	    bindnext = bind->next;	/* DAB */
	  else
	    bindnext = NULL;	/* DAB */
#else
	  /* always accept only ONE location even for INVITE */
	  /* this is a limitation for stability reason as the forking mode
	     is unfinished (calculation of the best response is not compliant
	     to the rfc3261.txt behavior. */
	  bindnext = NULL;	/* loop will be execute onece only */
#endif

	  i = osip_uri_clone (bind->contact->url, &url);
	  if (i != 0)
	    {
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				      "ls_localdb plugin: Could not clone contact info!\n"));
	      psp_request_set_uas_status (psp_req, 400);
	      psp_request_set_mode (psp_req, PSP_UAS_MODE);
	      psp_request_set_state (psp_req, PSP_MANDATE);
	      return -1;
	    }
	  i = location_init (&loc, url, 3600);
	  if (i != 0)
	    {
	      osip_uri_free (url);
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_BUG, NULL,
				      "ls_localdb plugin: Could not create location info!\n"));
	      psp_request_set_uas_status (psp_req, 400);
	      psp_request_set_mode (psp_req, PSP_UAS_MODE);
	      psp_request_set_state (psp_req, PSP_MANDATE);
	      return -1;
	    }

	  /* new support for rfc3327.txt and Path header */
	  if (bind->path!=NULL)
	    location_set_path(loc, osip_strdup(bind->path));

	  ADD_ELEMENT (psp_req->locations, loc);
	  numlocs++;		/* DAB */
	}			/* DAB */

      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "ls_localdb plugin: %d locations found!\n",
			      numlocs));
      return 0;
    }

  /* no user location found in local database... */
  psp_request_set_uas_status (psp_req, 404);
  psp_request_set_state (psp_req, PSP_CONTINUE);
  psp_request_set_mode (psp_req, PSP_UAS_MODE);
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			  "ls_localdb plugin: No location found for known user: return 404 Not found!!\n"));
  return 0;
}
