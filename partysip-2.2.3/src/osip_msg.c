/*
  The partysip program is a modular SIP proxy server (SIP -rfc3261-)
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  Partysip is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  Partysip is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <partysip.h>
#include <ppl/ppl_md5.h>

#include <osip_msg.h>

extern psp_core_t *core;


static char *
osip_to_tag_new_random ()
{
  char *tmp = (char *) osip_malloc (33);
  unsigned int number = osip_build_random_number ();

  sprintf (tmp, "%u", number);
  return tmp;
}


/* status code between 101 and 299 are forbidden...
   (except for REGISTER where 200 is allowed).
*/
int
osip_msg_build_response (osip_message_t ** dest, int status, osip_message_t * request)
{
  osip_generic_param_t *tag;
  osip_message_t *response;
  char *tmp;
  int pos;
  int i;

  *dest = NULL;
  i = osip_message_init (&response);
  if (i != 0)
    return -1;

  osip_message_set_version (response, osip_strdup ("SIP/2.0"));
  osip_message_set_status_code (response, status);

  tmp = osip_strdup(osip_message_get_reason (status));
  if (tmp == NULL)
    osip_message_set_reason_phrase (response, osip_strdup ("Unknown status code"));
  else
    osip_message_set_reason_phrase (response, tmp);

  osip_message_set_method (response, NULL);
  osip_message_set_uri (response, NULL);

  i = osip_to_clone (request->to, &(response->to));
  if (i != 0)
    goto mcubr_error_1;

  i = osip_to_get_tag (response->to, &tag);
  if (i != 0)
    {				/* we only add a tag if it does not already contains one! */
      if (status == 200 && MSG_IS_REGISTER (request))
	{
	  osip_to_set_tag (response->to, osip_to_tag_new_random ());
	}
      else if (status >= 200)
	{
	  osip_to_set_tag (response->to, osip_to_tag_new_random ());
	}
    }

  i = osip_from_clone (request->from, &(response->from));
  if (i != 0)
    goto mcubr_error_1;

  pos = 0;
  while (!osip_list_eol (&request->vias, pos))
    {
      osip_via_t *via;
      osip_via_t *via2;

      via = (osip_via_t *) osip_list_get (&request->vias, pos);
      i = osip_via_clone (via, &via2);
      if (i != -0)
	goto mcubr_error_1;
      osip_list_add (&response->vias, via2, -1);
      pos++;
    }

  i = osip_call_id_clone (request->call_id, &(response->call_id));
  if (i != 0)
    goto mcubr_error_1;
  i = osip_cseq_clone (request->cseq, &(response->cseq));
  if (i != 0)
    goto mcubr_error_1;

  if (core->banner[0]!='\0')
    osip_message_set_server (response, core->banner);


  if (MSG_IS_STATUS_2XX(response) && MSG_IS_PUBLISH(request))
    {
      MD5_CTX Md5Ctx;
      HASH HA1;
      HASHHEX ToTag;
      osip_via_t *via;
      osip_generic_param_t *br;
      via = (osip_via_t *)osip_list_get(&request->vias,0);
      osip_via_param_get_byname (via, "branch", &br);
      if (br==NULL || br->gvalue==NULL)
	{
	  if (request->cseq==NULL || request->cseq->number==NULL)
	    goto mcubr_error_1;
	}
      
      ppl_MD5Init(&Md5Ctx);
      if (br!=NULL && br->gvalue!=NULL)
	ppl_MD5Update(&Md5Ctx, (unsigned char *)br->gvalue, strlen(br->gvalue));
      ppl_MD5Update(&Md5Ctx, (unsigned char *)":", 1);
      if (request->cseq!=NULL && request->cseq->number!=NULL)
	ppl_MD5Update(&Md5Ctx, (unsigned char *)request->cseq->number, strlen(request->cseq->number));
      ppl_MD5Update(&Md5Ctx, (unsigned char *)":", 1);
      ppl_MD5Final((unsigned char *)HA1, &Md5Ctx);
      ppl_md5_hash_to_hex(HA1, ToTag);
      osip_message_set_header(response, "SIP-ETag", ToTag);
    }
  
  if (MSG_IS_STATUS_2XX(response) && MSG_IS_SUBSCRIBE(request))
    {
      char tmp[1024];
      osip_header_t *event;
      /* support for initiating SUBSCRIBE dialog */
      if (request->req_uri->username==NULL)
	{
	  /* should not be accepted... */
#ifdef WIN32
	  if (core->masquerade_via==1 && core->remote_natip!=NULL)
	    _snprintf(tmp, 1024, "sip:%s", core->remote_natip);
	  else
	    _snprintf(tmp, 1024, "sip:%s", core->serverip[0]);
#else
	  if (core->masquerade_via==1 && core->remote_natip!=NULL)
	    snprintf(tmp, 1024, "sip:%s", core->remote_natip);
	  else
	    snprintf(tmp, 1024, "sip:%s", core->serverip[0]);
#endif
	}
      else
	{
#ifdef WIN32
	  if (core->masquerade_via==1 && core->remote_natip!=NULL)
	    _snprintf(tmp, 1024, "sip:%s@%s", request->req_uri->username,
		      core->remote_natip);
	  else
	    _snprintf(tmp, 1024, "sip:%s@%s", request->req_uri->username,
		      core->serverip[0]);
#else
	  if (core->masquerade_via==1 && core->remote_natip!=NULL)
	    snprintf(tmp, 1024, "sip:%s@%s", request->req_uri->username,
		     core->remote_natip);
	  else
	    snprintf(tmp, 1024, "sip:%s@%s", request->req_uri->username,
		     core->serverip[0]);
#endif
	}
      osip_message_set_contact(response, tmp);

      /* copy event header */
      osip_message_header_get_byname(request, "event", 0, &event);
      if (event==NULL || event->hvalue==NULL)
	{
	  /* serach for compact form of Event header: "o" */
	  osip_message_header_get_byname(request, "o", 0, &event);
	  if (event==NULL || event->hvalue==NULL)
	    {
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_WARNING, NULL,
				      "missing event header in SUBSCRIBE request\n"));
	    }
	}
      
      if (event!=NULL && event->hvalue!=NULL)
	osip_message_set_header(response, "Event", event->hvalue);

      /* copy all record-route values */
      pos=0;
      while (!osip_list_eol(&request->record_routes, pos))
	{
	  osip_record_route_t *rr;
	  osip_record_route_t *rr2;
	  rr = osip_list_get(&request->record_routes, pos);
	  i = osip_record_route_clone(rr, &rr2);
	  if (i!=0) return -1;
	  osip_list_add(&response->record_routes, rr2, -1);
	  pos++;
	}
    }

  *dest = response;
  return 0;

mcubr_error_1:
  osip_message_free (response);
  return -1;
}

int
osip_msg_sfp_build_response_osip_to_forward (osip_message_t ** dest, osip_message_t * response)
{
  osip_message_t *fwd;
  osip_via_t *via;
  int i;

  *dest = NULL;
  i = osip_message_clone (response, &fwd);
  if (i != 0)
    return -1;

  /* remove top via... and send */
  via = osip_list_get (&fwd->vias, 0);
  if (via == NULL)		/* remote error: no via!! */
    {
      osip_message_free (fwd);
      return -1;
    }
  osip_list_remove (&fwd->vias, 0);
  osip_via_free (via);

  if (core->banner[0]!='\0')
    osip_message_set_server (fwd, core->banner);

  *dest = fwd;
  return 0;
}

int
osip_msg_sfp_build_request_osip_to_forward (psp_request_t * req, osip_message_t ** dest,
					    osip_uri_t * req_uri,   char *path,
					    osip_message_t * request, int stayonpath)
{
  osip_generic_param_t *branch_param;
  osip_via_t *via;
  char branch[45];
  int i;
  char number[3];

  i =
    osip_msg_default_build_request_osip_to_forward (dest, req_uri, path, request,
					       stayonpath);
  if (i != 0)
    return -1;

  /* ACK don't need a specific Via branch (the one just generated is fine!).
     This method will only be called when forwarding ACK for 2xx! (in the other
     cases, the transaction layer will handle it alone. */

  /* calculate the branch number */
  if (!MSG_IS_ACK (request))
    {
      /* fix the via... so we'll match the modified Via with the slp_branch. */
      psp_core_sfp_generate_branch_for_request (request, branch);
      if (branch[0] == '\0')
	{
	  osip_message_free (*dest);
	  *dest = NULL;
	  return -1;		/* there are some cases where we can't
				   build the branch? */
	}

      sprintf (number, "%i", req->branch_index);
      req->branch_index++;

      branch[40] = '.';
      branch[41] = number[0];
      branch[42] = number[1];
      branch[43] = '\0';

      /* get the Top Via and fix the branch */
      osip_message_get_via (*dest, 0, &via);
      if (via == NULL)
	{
	  osip_message_free (*dest);
	  *dest = NULL;
	  return -1;
	}
      osip_via_param_get_byname (via, "branch", &branch_param);
      if (branch_param == NULL)
	{			/* BUG? */
	  osip_message_free (*dest);
	  *dest = NULL;
	  return -1;
	}
      osip_free (branch_param->gvalue);
      branch_param->gvalue = osip_strdup(branch);
    }

  return 0;
}

/* Add a header to a SIP message at the top of the list.    */
/* This method is taken from libosip-0.9.3 so partysip
   is still able to link with older osip version. */
static int
_osip_message_set_topheader (osip_message_t * sip, char *hname, char *hvalue)
{
  osip_header_t *h;
  int i;

  if (hname == NULL)
    return -1;

  i = osip_header_init (&h);
  if (i != 0)
    return -1;

  h->hname = (char *) osip_malloc (strlen (hname) + 1);

  if (h->hname == NULL)
    {
      osip_header_free (h);
      return -1;
    }
  osip_strncpy (h->hname, hname, strlen (hname));
  osip_clrspace (h->hname);

  if (hvalue != NULL)
    {				/* some headers can be null ("subject:") */
      h->hvalue = (char *) osip_malloc (strlen (hvalue) + 1);
      if (h->hvalue == NULL)
	{
	  osip_header_free (h);
	  return -1;
	}
      osip_strncpy (h->hvalue, hvalue, strlen (hvalue));
      osip_clrspace (h->hvalue);
    }
  else
    h->hvalue = NULL;
  sip->message_property = 2;
  osip_list_add (&sip->headers, h, 0);
  return 0;			/* ok */
}

/* keep this one ! */
int
osip_msg_default_build_request_osip_to_forward (osip_message_t ** dest,
						osip_uri_t * req_uri,
						char *path,
						osip_message_t * request,
						int stayonpath)
{
  osip_header_t *maxfwd;
  osip_via_t *via;
  osip_route_t *route;
  osip_route_t *route2;
  osip_message_t *fwd;
  char branch[45];
  char *serverip;
  char *serverport;
  int i;

  *dest = NULL;
  i = osip_message_clone (request, &fwd);
  if (i != 0)
    return -1;


  /* TODO from rfc3261:
     if the URI contains unknown parameters, they MUST be removed.
   */

  /* add a via header... */
  psp_core_default_generate_branch_for_request (request, branch);
  if (branch[0] == '\0')
    goto pclbr_error_1;		/* there are some cases where we can't
				   build the branch? */

  /* Oupss! BUG
     serverip = psp_config_get_element ("serverip");
  */
  serverip=NULL;
  if (core->ext_ip!=NULL)
    serverip = core->ext_ip;
  if (serverip==NULL)
    serverip = core->serverip[0];

  if (serverip == NULL)
     {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "Bad configuration\n"));
      goto pclbr_error_1;
    }
  serverport = psp_config_get_element ("serverport_udp");

  /*        8.   Add a Via header field value   */

  i = osip_via_init (&via);
  if (i != 0)
    goto pclbr_error_1;
  i = osip_via_set_branch (via, osip_strdup(branch));
  if (i != 0)
    {
      osip_via_free (via);
      goto pclbr_error_2;
    }
  via_set_version (via, osip_strdup ("2.0"));
  if (core->masquerade_via==1 && core->remote_natip!=NULL)
    via_set_host (via, osip_strdup (core->remote_natip));
  else
    via_set_host (via, osip_strdup (serverip));
  /** SUPPORT ONLY UDP BY NOW!! **/
  if (serverport != NULL)
    via_set_port (via, osip_strdup (serverport));
  via_set_protocol (via, osip_strdup ("UDP"));

  osip_list_add (&fwd->vias, via, 0);

  /*        3.   Update the Max-Forwards header field */
  osip_message_get_max_forwards (fwd, 0, &maxfwd);
  if (maxfwd != NULL)
    {
      char *tmp;
      int maxf;

      if (maxfwd->hvalue != NULL)
	{
	  maxf = osip_atoi (maxfwd->hvalue);
	  maxf--;
	  tmp = (char *) osip_malloc (strlen (maxfwd->hvalue) + 2);
	  if (tmp == NULL)
	    goto pclbr_error_2;
	  osip_free (maxfwd->hvalue);
	  sprintf (tmp, "%i", maxf);
	  maxfwd->hvalue = tmp;
	}
    }

  /* route pre-processing:
     "If the first value in the Route header field indicates this proxy,
     the proxy MUST remove that value from the request." */
  osip_message_get_route (fwd, 0, &route);
  if (route != NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "Checking if we are responsible for '%s'\n",
		   route->url->host));
      i = psp_core_is_responsible_for_this_route (route->url);
      if (i == 0)		/* yes, it is */
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO4, NULL,
		       "I detect a route inserted by me, I remove it.\n"));
	  osip_list_remove (&fwd->routes, 0);
	  osip_route_free (route);
	}
    }
  /*        4.   Optionally add a Record-route header field value */
  if (stayonpath && (MSG_IS_INVITE (request)
		     || MSG_IS_SUBSCRIBE (request)
		     || MSG_IS_NOTIFY(request)))
    {
      /* AMD: 14/10/2002 This information is usefull only for INVITE 
	 AMD: 20/04/2005 It's also necessary for SUBSCRIBE and NOTIFY */
      osip_record_route_t *r_route;
      char *tmp;
      osip_uri_t *url_of_proxy;

      i = osip_record_route_init (&r_route);
      if (i != 0)
	goto pclbr_error_2;
      i = osip_uri_init (&url_of_proxy);
      if (i != 0)
	{
	  osip_record_route_free (r_route);
	  goto pclbr_error_2;
	}
      osip_uri_set_host (url_of_proxy,
		   osip_strdup (psp_core_get_ip_of_local_proxy ()));
      tmp = (char *) osip_malloc (9);
      sprintf (tmp, "%i", psp_core_get_port_of_local_proxy ());
      osip_uri_set_port (url_of_proxy, tmp);
      osip_uri_uparam_add (url_of_proxy, osip_strdup ("lr"), NULL);
      osip_uri_uparam_add (url_of_proxy, osip_strdup ("psp"),
		      osip_strdup (psp_config_get_element ("magicstring2")));
      osip_record_route_set_url (r_route, url_of_proxy);
      /* insert above all other record-route */
      osip_list_add (&fwd->record_routes, r_route, 0);
    }

  /*        9.   Add a Content-Length header field if necessary */
  if (fwd->content_length == NULL)
    {
      if (osip_list_size (&fwd->bodies) == 0)
	osip_message_set_content_length (fwd, "0");
      /* else should be refused before... but I prefer to let
         other deal with that */
    }

  /* several annoying cases:
     No locations were found but a location comes from route processing: */
  osip_message_get_route (request, 0, &route);
  osip_message_get_route (request, 1, &route2);
  if (route2 != NULL)
    {
    }				/* We are sure to use the record-route mechanism and message
				   is already correct. */
  else if (route == NULL ||
	   0 == psp_core_is_responsible_for_this_route (route->url))
    {				/* The message might not be already correct if the route is not me */
      if (req_uri != NULL)
	{
	  osip_uri_free (fwd->req_uri);
	  fwd->req_uri = NULL;	/* mandatory, bcz the next call can fail
					   and address MUST never be NULL if not
					   allocated... */
	  osip_message_set_uri (fwd, req_uri);
	}
    }

  if (path!=NULL)
    {
      char *beg;
      char *end;
      /* A location contains a set of Path headers. Those headers MUST
	 be set appended as "Route" headers to the new Request */
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO4, NULL,
		   "osip_msg module: build a request with a Pre-Route set.\n"));
      beg = path;
      end = strchr(beg, ','); /* hope you don't have a comma in your path :< */
      if (end==NULL)
	{
	   /* Where should we append this route? Top or Bottom?
	      (in fact, we won't probably get any other route header there.) */
	  osip_message_set_route(fwd, path);
	}
      else
	{
	  /* TODO: please support insertion of several Path header :< */
	  /* TODO */
	}
    }

  /*  handle rfc3327:
      Extension Header Field for Registering Non-Adjacent Contacts */
  if (core->rfc3327==1 && MSG_IS_REGISTER(fwd)) /* option is active */
    {
      /* add a path field pointing to myself. (so I'll be on the route
	 of incoming requests coming from the registrar.) */
      osip_record_route_t *r_route;
      char *tmp;
      osip_uri_t *url_of_proxy;

      i = osip_record_route_init (&r_route);
      if (i != 0)
	goto pclbr_error_2;
      i = osip_uri_init (&url_of_proxy);
      if (i != 0)
	{
	  osip_record_route_free (r_route);
	  goto pclbr_error_2;
	}
      osip_uri_set_host (url_of_proxy,
		   osip_strdup (psp_core_get_ip_of_local_proxy ()));
      tmp = (char *) osip_malloc (9);
      sprintf (tmp, "%i", psp_core_get_port_of_local_proxy ());
      osip_uri_set_port (url_of_proxy, tmp);
      osip_uri_uparam_add (url_of_proxy, osip_strdup ("lr"), NULL);
      osip_uri_uparam_add (url_of_proxy, osip_strdup ("psp"),
		      osip_strdup (psp_config_get_element ("magicstring2")));
      osip_record_route_set_url (r_route, url_of_proxy);

      /* get string for new 'Path' header: */
      i = osip_record_route_to_str(r_route, &tmp);
      osip_record_route_free (r_route);
      if (i != 0)
	{
	  goto pclbr_error_2;
	}
      /* insert above all other Path header */
      _osip_message_set_topheader(fwd, "Path", tmp);
      osip_free(tmp);
    }

  if (core->banner[0]!='\0')
    osip_message_set_user_agent (fwd, core->banner);

  *dest = fwd;
  return 0;

pclbr_error_2:
  osip_message_free (fwd);
  return -1;
pclbr_error_1:
  osip_message_free (fwd);
  return -1;
}


int
osip_msg_modify_ack_osip_to_be_forwarded (osip_message_t * ack, int stayonpath)
{
  osip_header_t *maxfwd;
  osip_via_t *via;
  osip_route_t *route;
  char branch[45];
  char *serverip;
  char *serverport;
  int i;

  /* will it be unique? To be verified. */
  psp_core_default_generate_branch_for_request (ack, branch);
  if (branch == '\0')
    return -1;			/* there are some cases where we can't
				   build the branch? */

  /* Oupss! BUG
     serverip = psp_config_get_element ("serverip");
  */
  serverip=NULL;
  if (core->ext_ip!=NULL)
    serverip = core->ext_ip;
  if (serverip==NULL)
    serverip = core->serverip[0];

  if (serverip == NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "Bad configuration\n"));
      goto pclbr_error_1;
    }
  serverport = psp_config_get_element ("serverport_udp");

  /*        8.   Add a Via header field value   */

  i = osip_via_init (&via);
  if (i != 0)
    goto pclbr_error_1;
  i = osip_via_set_branch (via, osip_strdup(branch));
  if (i != 0)
    {
      osip_via_free (via);
      goto pclbr_error_2;
    }
  via_set_version (via, osip_strdup ("2.0"));
  if (core->masquerade_via==1 && core->remote_natip!=NULL)
    via_set_host (via, osip_strdup (core->remote_natip));
  else
    via_set_host (via, osip_strdup (serverip));
  /** SUPPORT ONLY UDP BY NOW!! **/
  if (serverport != NULL)
    via_set_port (via, osip_strdup (serverport));
  via_set_protocol (via, osip_strdup ("UDP"));

  osip_list_add (&ack->vias, via, 0);

  /*        3.   Update the Max-Forwards header field */
  osip_message_get_max_forwards (ack, 0, &maxfwd);
  if (maxfwd != NULL)
    {
      char *tmp;
      int maxf;

      if (maxfwd->hvalue != NULL)
	{
	  maxf = osip_atoi (maxfwd->hvalue);
	  maxf--;
	  tmp = (char *) osip_malloc (strlen (maxfwd->hvalue) + 2);
	  if (tmp == NULL)
	    goto pclbr_error_2;
	  osip_free (maxfwd->hvalue);
	  sprintf (tmp, "%i", maxf);
	  maxfwd->hvalue = tmp;
	}
    }

  /* route pre-processing:
     "If the first value in the Route header field indicates this proxy,
     the proxy MUST remove that value from the request." */
  osip_message_get_route (ack, 0, &route);
  if (route != NULL)
    {
      i = psp_core_is_responsible_for_this_route (route->url);
      if (i == 0)		/* yes, it is */
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO4, NULL,
		       "I detect a route inserted by me, I remove it.\n"));
	  osip_list_remove (&ack->routes, 0);
	  osip_route_free (route);
	  /* in this case, we have previously requested to stay on the path.
	     We don't want to change that decision now, so we override
	     the possible value of stayonpath! */
	  stayonpath = 1;
	}
    }
  /*        4.   Optionally add a Record-route header field value */
  /* AMD: 14/10/2002: ACK don't need record-route info. */

  /*        9.   Add a Content-Length header field if necessary */
  if (ack->content_length == NULL)
    {
      if (osip_list_size (&ack->bodies) == 0)
	osip_message_set_content_length (ack, "0");
      /* else should be refused before... but I prefer to let
         other deal with that */
    }

pclbr_error_2:
  return -1;
pclbr_error_1:
  return -1;
}


int
osip_msg_build_cancel (osip_message_t ** dest, osip_message_t * request_cancelled)
{
  int i;
  osip_message_t *request;

  i = osip_message_init (&request);
  if (i != 0)
    return -1;

  /* prepare the request-line */
  request->sip_method = osip_strdup ("CANCEL");
  request->sip_version = osip_strdup ("SIP/2.0");
  request->status_code = 0;
  request->reason_phrase = NULL;

  i =
    osip_uri_clone (request_cancelled->req_uri,
	       &(request->req_uri));
  if (i != 0)
    goto gc_error_1;

  i = osip_to_clone (request_cancelled->to, &(request->to));
  if (i != 0)
    goto gc_error_1;
  i = osip_from_clone (request_cancelled->from, &(request->from));
  if (i != 0)
    goto gc_error_1;

  /* set the cseq and call_id header */
  i = osip_call_id_clone (request_cancelled->call_id, &(request->call_id));
  if (i != 0)
    goto gc_error_1;
  i = osip_cseq_clone (request_cancelled->cseq, &(request->cseq));
  if (i != 0)
    goto gc_error_1;
  osip_free (request->cseq->method);
  request->cseq->method = osip_strdup ("CANCEL");

  /* copy ONLY the top most Via Field (this method is also used by proxy) */
  {
    osip_via_t *via;
    osip_via_t *via2;

    i = osip_message_get_via (request_cancelled, 0, &via);
    if (i != 0)
      goto gc_error_1;
    i = osip_via_clone (via, &via2);
    if (i != 0)
      goto gc_error_1;
    osip_list_add (&request->vias, via2, -1);
  }

  osip_message_set_content_length (request, "0");
  osip_message_set_max_forwards (request, "70");	/* a UA should start a request with 70 */

  if (core->banner[0]!='\0')
    osip_message_set_user_agent (request, core->banner);

  *dest = request;
  return 0;

gc_error_1:
  osip_message_free (request);
  *dest = NULL;
  return -1;
}
