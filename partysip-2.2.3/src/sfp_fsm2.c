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

#include <partysip/partysip.h>

#include <partysip/osip_msg.h>

#include <ppl/ppl_dns.h>
#include "proxyfsm.h"

extern psp_core_t *core;

int
sfp_create_branch (psp_request_t * req, location_t *loc, ppl_dns_ip_t * ips)
{
  osip_transaction_t *transaction;
  osip_event_t *evt;
  sfp_branch_t *br;
  struct in6_addr addr;
  int i;
  osip_uri_t *url2;
  osip_message_t *request = psp_request_get_request(req);

  i = sfp_branch_init (&br, req->branch);
  if (i != 0) return -1;

  i = osip_uri_clone (loc->url, &url2);
  if (i != 0) { sfp_branch_free (br); return -1; }

  /* if an ips->name is not an IP: we replace the url host with the FQDN.
     (In this case, we are sure ips->name is not a domain name but a real FQDN.) */
  if (ips != NULL && (int) (i = ppl_inet_pton ((const char *)ips->name, (void *)&addr)) == -1)
    {
      osip_free (url2->host);
      url2->host = osip_strdup (ips->name);
    }

  sfp_branch_set_url (br, url2);

  if (ips != NULL && ips->next != NULL)	/* fallback exists */
    sfp_branch_clone_and_set_fallbacks (br, ips->next);

  i = osip_uri_clone (br->url, &url2);
  if (i != 0) { sfp_branch_free (br); return -1; }

  /* clone the request... start the new transaction... */
  i = osip_msg_sfp_build_request_osip_to_forward (req, &(br->request),
						  url2,
						  loc->path,
						  request,
						  __IS_PSP_STAY_ON_PATH (req->
									 flag));
  if (i != 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "sfp module: Could not create the request to forward for statefull transaction!\n"));
      sfp_branch_free (br);
      return -1;
    }
  i = pspm_sfp_call_plugins_for_request (core->sfp, req, br);
  if (i != 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "sfp module: Could not call plugins for this request!\n"));
      sfp_branch_free (br);
      return -1;
    }

  /* create a new outgoing transaction and add the REQUEST */
  evt = osip_new_outgoing_sipmessage (br->request);
  br->request = NULL; /* avoid double free and speed the proxy... */

  transaction = osip_create_transaction (core->psp_osip->osip, evt);
  if (transaction == NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "sfp module: Could not create a transaction for this request!\n"));
      osip_event_free(evt);
      sfp_branch_free (br);
      return -1;
    }
  sfp_branch_set_transaction (br, transaction);
  /* The oSIP stack transaction element has an entry for the resolved IP
     address where the stack should send the request. As we are using
     probably using a FQDN in the rq-uri (or the route), we can force
     the stack to keep the FQDN value there while it still can access
     the real IP of this host. */
  if (ips != NULL)
    {
      if (ips->srv_ns_flag == PSP_SRV_LOOKUP)	/* take the resolved port */
	{
	}
      else			/* for NS_LOOKUP, always rewrite the port number of the transaction */
	{
	  if (br->url->port != NULL)
	    {
	      if (((struct sockaddr_storage*)ips->addrinfo->ai_addr)->ss_family==AF_INET)
		{
		  struct sockaddr_in *addr;
		  addr = (struct sockaddr_in *) ips->addrinfo->ai_addr;
		  addr->sin_port = htons ((short) osip_atoi (br->url->port));
		}
	      else
		{
		  struct sockaddr_in6 *addr;
		  addr = (struct sockaddr_in6 *) ips->addrinfo->ai_addr;
		  addr->sin6_port = htons ((short) osip_atoi (br->url->port));
		}
	    }
	}

      if (((struct sockaddr_storage*)ips->addrinfo->ai_addr)->ss_family==AF_INET)
	{ /* IPv4 */
	  struct sockaddr_in *addr;
	  addr = (struct sockaddr_in *) ips->addrinfo->ai_addr;
	  if (MSG_IS_INVITE (request))
	    osip_ict_set_destination (transaction->ict_context,
				 ppl_inet_ntop ((struct sockaddr*)ips->addrinfo->ai_addr),
				 ntohs (addr->sin_port));
	  else
	    osip_nict_set_destination (transaction->nict_context,
				  ppl_inet_ntop ((struct sockaddr*)ips->addrinfo->ai_addr),
				  ntohs (addr->sin_port));
	}
      else
	{ /* IPv6 */
	  struct sockaddr_in6 *addr;
	  addr = (struct sockaddr_in6 *) ips->addrinfo->ai_addr;
	  if (MSG_IS_INVITE (request))
	    osip_ict_set_destination (transaction->ict_context,
				 ppl_inet_ntop ((struct sockaddr*)ips->addrinfo->ai_addr),
				 ntohs (addr->sin6_port));
	  else
	    osip_nict_set_destination (transaction->nict_context,
				  ppl_inet_ntop ((struct sockaddr*)ips->addrinfo->ai_addr),
				  ntohs (addr->sin6_port));
	}
    }

  /* the firewall is based on the call-id value of the call. We should
     use at least callid+to+from instead. */

  if (core->iptables_dynamic_natrule == 1 && MSG_IS_BYE(evt->sip))
    {
      char *hash_value;
      i = osip_call_id_to_str(evt->sip->call_id, &hash_value);
      if (i == 0)
	{
	  firewall_entry_remove_rule (hash_value);
	  osip_free(hash_value);
	}
      /* continue anyway. */
    }
  else if (core->iptables_dynamic_natrule == 1
	   || core->masquerade_sdp==1)
    {
      char *host;
      int port;
      struct in6_addr addr;
      osip_transaction_get_destination(transaction, &host, &port);
      if (host==NULL) /* In this case, the message is sent to this url */
	host = evt->sip->req_uri->host;

      i = -1;
      if (host!=NULL && ppl_inet_pton ((const char *)host, (void *)&addr) != -1)
	{
	  i = firewall_fix_request_osip_to_forward (evt->sip, host);
	}
      else
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "module sfp: firewall module failed (no target_host defined)!\n"));
	  osip_list_add (core->sfp->broken_transactions, br->out_tr, 0);
	  br->out_tr = NULL;
	  osip_event_free(evt);
	  sfp_branch_free (br);
	  return -1;
	}

      if (i == -1)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "module sfp: firewall module failed!\n"));
	  osip_list_add (core->sfp->broken_transactions, br->out_tr, 0);
	  br->out_tr = NULL;
	  osip_event_free(evt);
	  sfp_branch_free (br);
	  return -1;
	}
    }

  osip_transaction_add_event (transaction, evt);

  ADD_ELEMENT (req->branch_notanswered, br);

  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO2, NULL,
	       "sfp module: transaction correctly forwarded!\n"));

  psp_osip_wakeup (core->psp_osip);
  return 0;
}
 
int
sfp_forward_response (psp_request_t * req, sfp_branch_t *br)
{
  osip_event_t *evt;
  osip_message_t *responsexxx;
  int i;

  i = osip_msg_sfp_build_response_osip_to_forward (&responsexxx,
						   br->out_tr->last_response);
  if (i != 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "module sfp: Cannot forward xxx response!\n"));
      return -1;
    }

  if (core->iptables_dynamic_natrule == 1
      || core->masquerade_sdp==1)
    {
      i =
	firewall_fix_200ok_osip_to_forward (responsexxx);
      if (i == -1)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "module sfp: firewall module failed!\n"));
	  /* continue? */
	}
    }

  i = pspm_sfp_call_plugins_for_snd_response (core->sfp, req, responsexxx);
  if (i != 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "module sfp: Could not call plugins!\n"));
      osip_message_free (responsexxx);
      return -1;
    }

  evt = osip_new_outgoing_sipmessage (responsexxx);
  osip_transaction_add_event(req->inc_tr, evt);
  psp_osip_wakeup (core->psp_osip);  
  return 0;
}

int
sfp_cancel_pending_branch(psp_request_t *req, sfp_branch_t *br)
{
  osip_transaction_t *transaction;
  osip_event_t *evt;
  osip_route_t *route;
  osip_route_t *route2;
  osip_message_t *cancel;
  int i;
  int pos;
  if (br==NULL
      || br->out_tr==NULL
      || br->out_tr->orig_request==NULL
      || br->out_tr->state == ICT_TERMINATED
      || br->out_tr->state == ICT_COMPLETED
      || br->out_tr->state == NICT_TERMINATED
      || br->out_tr->state == NICT_COMPLETED)
    return -1;

  if (br->already_cancelled==0)
    return -1;
  
  if (br->out_tr->state == ICT_CALLING)
    return -1;

  /* rfc3261 recommends to only CANCEL INVITEs. */
  if (MSG_IS_INVITE(br->out_tr->orig_request))
    { }
  else
    return -1; /* silently discard the request for a CANCEL */

  i = osip_msg_build_cancel (&cancel, br->out_tr->orig_request);
  if (i!=0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "sfp module: Can't build cancel request!\n"));
      return -1;
    }

  /* add the same route-set than in the previous request */
  pos=0;
  while (!osip_list_eol (&br->out_tr->orig_request->routes, pos))
    {
      route = (osip_route_t *) osip_list_get (&br->out_tr->orig_request->routes, pos);
      i = osip_route_clone (route, &route2);
      if (i != 0)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "sfp module: Can't add route set in cancel request!\n"));
	  osip_message_free(cancel);
	  return -1;
	}
      osip_list_add(&cancel->routes, route2, -1);
      pos++;
    }

  evt = osip_new_outgoing_sipmessage (cancel);
  transaction = osip_create_transaction (core->psp_osip->osip, evt);
  if (transaction == NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "sfp module: Could not create a CANCEL transaction!\n"));
      osip_event_free (evt);
      return -1;
    }

  br->already_cancelled = 0;

  evt->transactionid = transaction->transactionid;
  osip_list_add (core->sfp->broken_transactions, transaction, 0);
  osip_transaction_add_event (transaction, evt);
  psp_osip_wakeup (core->psp_osip);
  return 0;
}

int
sfp_forward_ack(psp_request_t *req)
{
  location_t *ptr;
  location_t *loc;
  int i;

  /* This method should be called only when a route header exist
     in the request. (else ACK must be sent to the contact field
     of 200 OK or are consumed by ACK > 299 */

  /* For statefull mode, the plugins have to provide
     locations to be resolved... */
  for (loc = req->locations; loc != NULL; loc = ptr)
    {
      struct in6_addr addr;
      int retv;

      ptr = loc->next;

      retv = ppl_inet_pton ((const char *)loc->url->host, (void *)&addr);

      if (retv == -1) /* error */
	{
	  psp_request_free(req);
	  return -1;
	}

      if (retv == 0)	/* not an ip address, ask for a resolution */
	{
	  ppl_dns_entry_t *dns;	  
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO2, NULL,
		       "Trying to resolv '%s' for ACK!\n", loc->url->host));
	  ppl_dns_query_host (&dns, osip_strdup(loc->url->host), 5060);
	  if (dns != NULL)		/* success! */
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO2, NULL,
			   "Successfully resolved the domain '%s' for ACK!\n", loc->url->host));
	      osip_free(loc->url->host);
	      loc->url->host = osip_malloc(strlen(dns->dns_ips->name));
	      strcpy(loc->url->host, dns->dns_ips->name);
	      ppl_dns_entry_free(dns);
	    }
	  /* retry with the new value */
	  retv = ppl_inet_pton ((const char *)loc->url->host, (void *)&addr);
	}

      if (retv == 0)	/* not an ip address, ask for a resolution */
	{
	  psp_core_event_resolv_add_query (osip_strdup (loc->url->host));
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "sfp module: we have to send the ACK now, but the ACK contains a FQDN!\n"));
	}
      else
	{
	  /* WE DON'T NEED TO WAIT! this location can be tried right now. */
	  osip_message_t *ack;
	  int port;

	  i = osip_msg_sfp_build_request_osip_to_forward (req, &ack, loc->url,
							  loc->path,
							  req->inc_ack,
							  __IS_PSP_STAY_ON_PATH
							  (req->flag));
	  if (i != 0)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_ERROR, NULL,
			   "sfp module: could not build ACK to forward!\n"));
	      psp_request_free(req);
	      return -1;
	    }
	  else
	    {
	      i = pspm_sfp_call_plugins_for_request (core->sfp, req, NULL);
	      if (i != 0)
		{
		  OSIP_TRACE (osip_trace
			      (__FILE__, __LINE__, OSIP_ERROR, NULL,
			       "sfp module: Could not call plugins for this request!\n"));
		}
	    }
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO2, NULL,
		       "sfp module: we have to send the ACK now!\n"));

	  if (loc->path!=NULL)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_BUG, NULL,
			   "sfp module: We have to recalculate the route!\n"));
	      /* get the initial route */
	      /* TODO ... */
	    }

	  port = 5060;
	  if (loc->url->port != NULL)
	    port = osip_atoi (loc->url->port);
	  psp_core_cb_snd_message (NULL, ack, loc->url->host, port, -1);
	  osip_message_free (ack);
	  loc->url = NULL;

	  REMOVE_ELEMENT (req->locations, loc);
	  location_free (loc);
	}
    }

  psp_request_free(req);
  return 0;
}

int
sfp_answer_request(psp_request_t * req)
{
  osip_message_t *request;
  osip_message_t *response;
  osip_event_t *evt;
  int i;
  
  request = psp_request_get_request(req);
  if (request==NULL)
    return -1;
  i = osip_msg_build_response (&response, req->uas_status, request);
  if (i != 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "sfp module: Could not build response!\n"));
      return -1;
    }

  /* if the psp_req_t element contains locations and is a 3xx, insert locations */
  if (300 <= req->uas_status && req->uas_status <= 399)
    {
      location_t *loc;

      for (loc = req->locations; loc != NULL; loc = loc->next)
	{
	  char *tmp; /* convert to contact header */

	  if (loc->url != NULL && 0 == osip_uri_to_str (loc->url, &tmp))
	    {
	      /* insert contact in SIP message */
	      i = osip_message_set_contact (response, tmp);
	      if (i != 0)
		{
		  OSIP_TRACE (osip_trace
			      (__FILE__, __LINE__, OSIP_WARNING, NULL,
			       "Bad contat header given by plugin to module uap!\n"));
		}
	    }
	}
      if (osip_list_size (&response->contacts) == 0)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "A plugin ask for redirection without giving a contact!\n"));
	  /* so we change the status code to 404 Not found */
	  req->uas_status = 404;
	  osip_message_set_status_code (response, 404);
	  osip_free (response->reason_phrase);
	  osip_message_set_reason_phrase (response, osip_strdup(osip_message_get_reason (404)));
	}
    }

  i = pspm_sfp_call_plugins_for_snd_response (core->sfp, req, response);
  if (i != 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "module sfp: Could not call plugins!\n"));
      osip_message_free (response);
      return -1;
    }

  /* send a copy of the response to the osip stack */
  evt = osip_new_outgoing_sipmessage (response);
  osip_transaction_add_event (req->inc_tr, evt);
  psp_osip_wakeup (core->psp_osip);
  return 0;  
}
