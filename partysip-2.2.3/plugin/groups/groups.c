/*
  The groups plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The groups plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The groups plugin is distributed in the hope that it will be useful,
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

#include "groups.h"

groups_ctx_t *groups_context = NULL;

extern psp_plugin_t PPL_DECLARE_DATA groups_plugin;
extern char groups_name_config[50];

/*
  This plugin can be configured to:
  *
  * set forking or sequential mode
  * ask for record-routing

  */

/*
  Configuration sample for REQUEST processing.

  <groups>
  record-route on
  mode sf_forking

  group0   support
  domain0  atosc.org
  mode0    sf_forking
  members0 sip:803@atosc.org|sip:801@atosc.org|sip:aymeric@atosc.org

  group1   svi
  mode1    sf_sequential
  members1 sip:svi1@atosc.org|sip:svi2@atosc.org|sip:svi3@atosc.org

  </groups>
*/

#define FORKING_MODE        0x04
#define SEQ_MODE            0x08

#define ISSET_FORKING_MODE(flag)    ((~flag|~FORKING_MODE)==~FORKING_MODE)
#define SET_FORKING_MODE(flag)      (flag=((flag&0x0FF)|FORKING_MODE))
#define ISSET_SEQUENTIAL_MODE(flag) ((~flag|~SEQ_MODE)==~SEQ_MODE)
#define SET_SEQUENTIAL_MODE(flag)   (flag=((flag&0x0FF)|SEQ_MODE))

#define R_ROUTE_MODE        0x10

#define ISSET_R_ROUTE_MODE(flag)    ((~flag|~R_ROUTE_MODE)==~R_ROUTE_MODE)
#define SET_R_ROUTE_MODE(flag)      (flag= flag|R_ROUTE_MODE)


static int groups_load_members(grp_t *grp, char *members)
{
  char *dest;
  int index = 0;

  char *tmp = members;
  char *sep;
  sep = strchr(members, '|'); /* find beginning of prefix */

  while (sep!=NULL && index<MAX_MEMBERS)
    {
      dest = grp->members[index];
      if (sep-tmp<254)
	osip_strncpy(dest, tmp, sep-tmp);
      else
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
				"groups plugin: members url must be shorter than 254\n"));
	}

      index++;
      tmp = sep+1;
      sep = strchr(tmp, '|'); /* find beginning of prefix */
    }

  dest = grp->members[index];
  if (tmp!=NULL && strlen(tmp)<254)
    {
      osip_strncpy(dest, tmp, strlen(tmp));
    }

  for (index=0;index<MAX_MEMBERS;index++)
    {
      int i;
      osip_uri_t *uri;

      dest = grp->members[index];
      if (dest[0]=='\0')
	break;
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
			    "groups plugin: members of %s: %s\n",
			    grp->group,
			    dest));

      osip_uri_init(&uri);
      i = osip_uri_parse(uri, dest);
      osip_uri_free(uri);
      if (i!=0)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
				"groups plugin: Malformed members URL in group %s!\n",
				grp->group));
	  return -1;
	}
    }

  return 0;
}

static int groups_load_config()
{
  char group[20];
  char domain[20];
  char mode[20];
  char members[20];
  char groupx[20];
  char domainx[20];
  char modex[20];
  char membersx[20];
  int  index = 0;

  config_element_t *elem;
  int i;

  strcpy(group, "group");
  strcpy(domain, "domain");
  strcpy(mode, "mode");
  strcpy(members, "members");

  sprintf(groupx, "group%i", index);
  sprintf(domainx, "domain%i", index);
  sprintf(modex, "mode%i", index);
  sprintf(membersx, "members%i", index);

  elem = psp_config_get_sub_element(groupx, groups_name_config, NULL);
  
  while (elem!=NULL)
    {
      char *u = NULL;
      char *d = NULL;
      char *m = NULL;
      char *ms = NULL;
      /*
	group0   support
	domain0  atosc.org
	mode0    sf_forking
	members0 sip:803@atosc.org|sip:801@atosc.org|sip:aymeric@atosc.org
      */
      u = elem->value;
      elem = psp_config_get_sub_element(domainx, groups_name_config, NULL);
      if (elem!=NULL)
	d = elem->value;
      elem = psp_config_get_sub_element(modex, groups_name_config, NULL);
      if (elem!=NULL)
	m = elem->value;
      elem = psp_config_get_sub_element(membersx, groups_name_config, NULL);
      if (elem!=NULL)
	ms = elem->value;

      if (u==NULL || u[0]=='\0')
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
				"groups plugin: Missing required group name (index=%i)\n",
				index));
	}
      else if (ms==NULL || ms[0]=='\0')
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
				"groups plugin: Missing required members for group %s!\n",
				u));
	}
      else
	{
	  grp_t *grp;
	  grp = &(groups_context->grps[index]);

	  osip_strncpy(grp->group, u, 254);
	  if (d!=NULL && d[0]!='\0')
	    osip_strncpy(grp->domain, d, 254);

	  grp->flag=0;
	  if (m == NULL)
	    SET_FORKING_MODE (grp->flag);
	  else if (0 == strcmp (m, "sf_forking"))
	    SET_FORKING_MODE (grp->flag);
	  else if (0 == strcmp (m, "statefull"))
	    SET_FORKING_MODE (grp->flag);
	  else if (0 == strcmp (m, "sf_sequential"))
	    SET_SEQUENTIAL_MODE (grp->flag);
	  else
	    SET_FORKING_MODE (grp->flag);

	  if (ISSET_FORKING_MODE (grp->flag))
	    {
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
				      "groups plugin: group %s configured in forking mode!\n", u));
	    }
	  if (ISSET_SEQUENTIAL_MODE (grp->flag))
	    {
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
				      "groups plugin: group %s configured in sequential mode!\n", u));
	    }


	  i = groups_load_members(grp, ms);
	  if (i!=0)
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
				    "groups plugin: Malformed members definition for group %s!\n",
				    u));
	      return -1;
	    }
	}

      index++;
      sprintf(groupx, "group%i", index);
      sprintf(domainx, "domain%i", index);
      sprintf(modex, "mode%i", index);
      sprintf(membersx, "members%i", index);

      elem = psp_config_get_sub_element(groupx, groups_name_config, NULL);
    }
  return 0;
}

int
groups_ctx_init()
{
  config_element_t *elem;
  int i;

  groups_context =
    (groups_ctx_t *) osip_malloc (sizeof (groups_ctx_t));
  if (groups_context == NULL)
    return -1;

  memset(groups_context, 0, sizeof(groups_ctx_t));

  groups_context->flag = 0;
  elem = psp_config_get_sub_element ("mode", groups_name_config, NULL);
  if (elem == NULL || elem->value == NULL)
    SET_FORKING_MODE (groups_context->flag);
  else if (0 == strcmp (elem->value, "sf_forking"))
    SET_FORKING_MODE (groups_context->flag);
  else if (0 == strcmp (elem->value, "statefull"))
    SET_FORKING_MODE (groups_context->flag);
  else if (0 == strcmp (elem->value, "sf_sequential"))
    SET_SEQUENTIAL_MODE (groups_context->flag);
  else
    SET_FORKING_MODE (groups_context->flag);
  
  if (ISSET_FORKING_MODE (groups_context->flag))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "groups plugin: plugin configured in forking mode!\n"));
    }
  if (ISSET_SEQUENTIAL_MODE (groups_context->flag))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "groups plugin: plugin configured in sequential mode!\n"));
    }
  
  elem = psp_config_get_sub_element ("record-route", groups_name_config, NULL);
  if (elem == NULL || elem->value == NULL)
    {				/* default mode is off */
    }
  else if (0 == strcmp (elem->value, "off"))
    {
    }
  else if (0 == strcmp (elem->value, "on"))
    SET_R_ROUTE_MODE (groups_context->flag);
  else
    goto gci_error1;		/* error, bad option */

  if (ISSET_R_ROUTE_MODE (groups_context->flag))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "groups plugin: configured to do record-routing!\n"));
    }

  /* load the configuration behavior for INVITE */
  i = groups_load_config();
  if (i!=0) goto gci_error2;

  return 0;

 gci_error2:
 gci_error1:
  osip_free(groups_context);
  groups_context = NULL;
  return -1;
}

void
groups_ctx_free()
{
  if (groups_context==NULL) return;

  osip_free(groups_context);
  groups_context = NULL;
}


/* HOOK METHODS */

/*
  This method returns:
  -2 if plugin consider this request should be totally discarded!
  -1 on error
  0  nothing has been done
  1  things has been done on psp_req element
*/
int cb_groups_search_location(psp_request_t *psp_req)
{
  location_t *loc;
  osip_route_t *route;
  int i;
  int index;
  int match;
  grp_t *grp=NULL;

  osip_uri_param_t *psp_param;
  osip_message_t *request;
  request = psp_request_get_request(psp_req);

  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
	   "groups plugin: entering cb_groups_search_location\n"));

  /* default OUTPUT */
  if (ISSET_R_ROUTE_MODE(groups_context->flag))
    psp_request_set_property(psp_req, PSP_STAY_ON_PATH);
  else
    psp_request_set_property(psp_req, 0);

  psp_request_set_mode(psp_req, PSP_SFULL_MODE);

  i=0;
  for (;!osip_list_eol(&request->routes, i);i++)
    {
      osip_message_get_route (request, i, &route);
      if (0 != psp_core_is_responsible_for_this_route(route->url))
	{
	  psp_request_set_mode (psp_req, PSP_SFULL_MODE);
	  psp_request_set_state (psp_req, PSP_MANDATE);
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
				  "groups plugin: mandate statefull handling for route.\n"));
	  return 0;
	}
    }

  psp_request_set_state(psp_req, PSP_MANDATE);

  if (i>1)
    {
      psp_request_set_uas_status(psp_req, 482); /* loop? */
      psp_request_set_mode(psp_req, PSP_UAS_MODE);
      return 0;
    }
  if (i==1)
    {
      osip_message_get_route(request, 0, &route); /* should be the first one */
      /* if this route header contains the "psp" parameter, it means
	 the element does not come from a pre-route-set header (in this
	 last case, we want to execute the plugin for the initial request) */
      /* NON compliant UAs (not returning this parameter) are guilty. */
      osip_uri_uparam_get_byname (route->url, "psp", &psp_param);
      if (psp_param!=NULL)
	{
	  psp_request_set_state(psp_req, PSP_MANDATE);
	  psp_request_set_mode (psp_req, PSP_SFULL_MODE);
	  /* got it, leave this plugin. */
	  return 0;
	}
    }

  if (request->req_uri->username==NULL
      || request->req_uri->host==NULL)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "groups plugin: No username in uri.\n"));
      psp_request_set_state(psp_req, PSP_PROPOSE);
      psp_request_set_uas_status(psp_req, 404);
      psp_request_set_mode(psp_req, PSP_UAS_MODE);
      return 0;
    }

  /* search for a group */
  match=0;
  for (index=0;index<MAX_GROUPS;index++)
    {
      grp = &(groups_context->grps[index]);
      if (grp->group[0]!='\0')
	{
	  if (osip_strcasecmp(grp->group, request->req_uri->username)==0)
	    {
	      if (grp->domain[0]=='\0')
		{ match=1; break; }
	      else if (osip_strcasecmp(grp->domain, request->req_uri->host)==0)
		{ match=1; break; }
	    }
	}
      grp=NULL;
    }

  if (match==1 && grp!=NULL)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "groups plugin: A group match this call (%s).\n",
			      grp->group));
      
      for (index=0;index<MAX_MEMBERS;index++)
	{
	  osip_uri_t *uri;
	  int i;
	  char *dest;

	  dest = grp->members[index];
	  if (dest[0]=='\0')
	    break;
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO3,NULL,
				"groups plugin: members of %s: %s\n",
				grp->group,
				dest));
	  
	  osip_uri_init(&uri);
	  i = osip_uri_parse(uri, dest);
	  
	  if (i==0)
	    {
	      i = location_init(&loc, uri, 3600);
	      if (i!=0)
		{ /* This can only happen in case we don't have enough memory */
		  osip_uri_free(uri);
		  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_BUG,NULL,
					"groups plugin: Could not create location info!\n"));
		}
	      else
		{
		  ADD_ELEMENT(psp_req->locations, loc);
		}
	    }
	}
      return 0;
    }

  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
			"groups plugin: Didn't do anything with this request?\n"));
  psp_request_set_state(psp_req, PSP_PROPOSE);
  psp_request_set_uas_status(psp_req, 404);
  psp_request_set_mode(psp_req, PSP_UAS_MODE);
  
  return 0;

}

