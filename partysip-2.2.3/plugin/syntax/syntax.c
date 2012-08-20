/*
  The syntax plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003,2004,2005  Aymeric MOIZARD - <jack@atosc.org>
  
  The syntax plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The syntax plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <partysip/partysip.h>
#include "syntax.h"

#ifndef WIN32
/* to be tested on windows */
extern psp_core_t *core;
#endif

extern psp_plugin_t PPL_DECLARE_DATA syntax_plugin;

extern char supported_schemes[200];

/*
  0: default   <--- for INVITE: be statefull. For other request: be stateless
  1: no verification
  2: limited checks
  3: full checks */
int mode = 0;

/* HOOK METHODS */

/*
  This method returns:
  -2 if plugin consider this request should be totally discarded!
  -1 on error
  0  nothing has been done
  1  things has been done on psp_request element
*/
int
cb_check_syntax_in_request (psp_request_t * psp_req)
{
#ifndef WIN32
  char     *serverport;
  char     *servervia;
  int       spiral;
  int       i;
#endif

  osip_header_t *maxfwd;
  osip_header_t *p_require;

#ifdef WIN32
  unsigned long int one_inet_addr;
#else
#ifdef IPV6_SUPPORT
  struct in6_addr inaddr;
  int inet_pton_result;
#else
  struct in_addr inaddr;
#endif
#endif
  osip_message_t *request;
  request = psp_request_get_request(psp_req);

  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
			  "syntax plugin: validate syntax.\n"));


  /* verify validity of request just in case */
  if (request == NULL
      || request->req_uri == NULL
      || (request->req_uri->host == NULL
	  && request->req_uri->string == NULL))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "syntax plugin: Bad Request-Line.\n"));
      psp_request_set_state (psp_req, PSP_STOP);
      return -2;		/* ask the core application to discard the request */
    }

  if (NULL == osip_message_get_from (request))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "syntax plugin: No From header.\n"));
      psp_request_set_state (psp_req, PSP_STOP);
      return -2;
    }

  if (NULL == osip_message_get_call_id (request))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "syntax plugin: No Call-ID header.\n"));
      psp_request_set_state (psp_req, PSP_STOP);
      return -2;
    }

  if (NULL == osip_message_get_cseq (request))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "syntax plugin: No CSeq header.\n"));
      psp_request_set_state (psp_req, PSP_STOP);
      return -2;
    }

  if (NULL == osip_message_get_to (request))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "syntax plugin: No To header.\n"));
      psp_request_set_state (psp_req, PSP_STOP);
      return -2;
    }

  if (request->req_uri->scheme == NULL)
    request->req_uri->scheme = osip_strdup ("sip");
  if (0 != strcmp (request->req_uri->scheme, "sip")
      && 0 != strcmp (request->req_uri->scheme, "sips")
      && NULL == strstr (supported_schemes,
			 request->req_uri->scheme))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "syntax plugin: Url Scheme not supported by proxy.\n"));
      /* by now, we only accept "sip:" and "sips:" urls */
      psp_request_set_state (psp_req, PSP_MANDATE);
      psp_request_set_mode (psp_req, PSP_UAS_MODE);
      psp_request_set_uas_status (psp_req, 416);
      return 0;
    }

  if (0 == strcmp (request->req_uri->scheme, "sip")
      || 0 == strcmp (request->req_uri->scheme, "sips"))
    {
#ifdef WIN32
      if ((int)
	  (one_inet_addr =
	   inet_addr (request->req_uri->host)) == -1)
	{

	}
#else
#ifdef IPV6_SUPPORT
      inet_pton_result = ppl_inet_pton (request->req_uri->host,
					  (void *)&inaddr);
      if (inet_pton_result <= 0)
#else
      if (INADDR_NONE ==
	  inet_aton (request->req_uri->host, &inaddr))
#endif
	{
	  /* TODO: check if it's an valid fqdn! */
	  /*
	     if (isavalidfqdn())
	     {
	     psp_request_set_state (psp_req, PSP_MANDATE);
	     psp_request_set_mode (psp_req, PSP_UAS_MODE);
	     psp_request_set_uas_status (psp_req, 400);
	     }
	     else
	     {
	     OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
	     "syntax plugin: Ip address detected!\n"));
	     }
	   */
	}
#endif
      else
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
				  "syntax plugin: IP address detected!\n"));
	}
    }

  osip_message_get_max_forwards (request, 0, &maxfwd);
  if (maxfwd != NULL)
    {
      if (maxfwd->hvalue != NULL
	  && 1 == strlen (maxfwd->hvalue)
	  && 0 == strncmp (maxfwd->hvalue, "0", 1))
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				  "syntax plugin: Too much Hop for request!\n"));
	  /* by now, we only accept "sip:" and "sips:" urls */
	  psp_request_set_state (psp_req, PSP_MANDATE);
	  psp_request_set_mode (psp_req, PSP_UAS_MODE);
	  psp_request_set_uas_status (psp_req, 483);	/* Too Many Hops */
	  return 0;
	}
    }

#ifndef WIN32
  /* this is the correct place for doing loop detection! */
  serverport = psp_config_get_element ("serverport_udp");

  { /* the via is calculated this way! somewhat ugly! */
    servervia=NULL;
    if (core->ext_ip!=NULL)
      servervia = core->ext_ip;
    else if (core->masquerade_via==1 && core->remote_natip!=NULL)
      servervia = core->remote_natip;
    else
      servervia = core->serverip[0]; /* default one */

  }

  if (!osip_list_eol(&request->vias, 1)) /* only one via? */
    {
      i=0;
      spiral=0;
      for (;!osip_list_eol(&request->vias, i);i++)
	{
	  osip_via_t *via;
	  osip_message_get_via (request, i, &via);
	  if (0 == strcmp(servervia, via->host))
	    {
	      if ((serverport!=NULL && via->port!=NULL &&
		   0==strcmp(serverport, via->port))
		  
		  || (serverport!=NULL && via->port==NULL &&
		      0==strcmp(serverport, "5060"))
		  
		  || (serverport==NULL && via->port!=NULL &&
		      0==strcmp(via->port, "5060"))
		  
		  || (serverport==NULL && via->port==NULL))
		{
		  if (i==0) /* This is a loop over myself! */
		    {
		      psp_request_set_state(psp_req, PSP_MANDATE);
		      psp_request_set_uas_status(psp_req, 482);
		      psp_request_set_mode(psp_req, PSP_UAS_MODE);
		      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
					      "syntax plugin: loop detected.\n"));
		      return 0;
		    }
		  /* might be a spiral (a valid loop) */
		  spiral++; /* give one chance for a spiral */
		  /*
		    To detect spiralling, the branch parameter must be
		    calculated by including all the elements that affect
		    routing. In 0.5.5, partysip don't use those elements
		    to build the branch, so this is not possible.
		    
		    By the way, partysip offer the capability to loop once
		    over himself. If the request comes a second time, then
		    it will be considered to be a loop.
		  */
		  if (spiral==2)
		    { /* we assume it's a loop now */
		      psp_request_set_state(psp_req, PSP_MANDATE);
		      psp_request_set_uas_status(psp_req, 482);
		      psp_request_set_mode(psp_req, PSP_UAS_MODE);
		      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_WARNING, NULL,
					      "syntax plugin: We've certainly discovered a loop.\n"));
		      return 0;
		    }
		  
		}
	    }
	}
    }
#endif

  /* Checking Proxy-Require extensions. */
  /* at this step, none is supported! */
  osip_message_get_proxy_require (request, 0, &p_require);
  if (p_require != NULL)
    {				/* at least one exist, but we don't support any extensions */
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "syntax plugin: Unsupported feature in Proxy-Require header.\n"));
      psp_request_set_state (psp_req, PSP_MANDATE);
      psp_request_set_mode (psp_req, PSP_UAS_MODE);
      psp_request_set_uas_status (psp_req, 420);
      return 0;
    }

  psp_request_set_state (psp_req, PSP_CONTINUE);
  return 0;
}

int
cb_complete_answer_on_4xx (psp_request_t * psp_req, osip_message_t *response)
{
  osip_header_t *p_require;
  osip_header_t *unsupported;
  int i;
  osip_message_t *request;
  request = psp_request_get_request(psp_req);

  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO3, NULL,
			  "syntax plugin: completing 4xx answer.\n"));

  if (psp_request_get_uas_status (psp_req) == 420)	/* on Bad Extension */
    {
      int pos = 0;

      /* copy all supported header in unsupported headers */
      pos = osip_message_get_proxy_require (request, pos, &p_require);
      while (p_require != NULL)
	{
	  if (p_require->hname == NULL || p_require->hvalue == NULL)
	    break;
	  i = osip_header_clone (p_require, &unsupported);
	  if (i != 0)
	    break;
	  osip_free (unsupported->hname);
	  unsupported->hname = osip_strdup ("Unsupported");
	  osip_list_add (&response->headers, unsupported, -1);
	  pos++;
	  pos = osip_message_get_proxy_require (request, pos, &p_require);
	}
    }
  return 0;
}
