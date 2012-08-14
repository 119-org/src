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
#include <ppl/ppl_dns.h>
#include <osip_msg.h>

extern psp_core_t *core;

PPL_DECLARE (int)
psp_core_dns_add_domain_error (char *address)
{
  ppl_dns_error_add (address);
  module_wakeup (core->sfp->module);	/* this module needs to be waken up */
  return 0;
}

PPL_DECLARE (int) psp_core_dns_add_domain_result (ppl_dns_entry_t * dns)
{
  ppl_dns_add_domain_result (dns);
  module_wakeup (core->sfp->module);	/* this module needs to be waken up */
  return 0;
}

/*
int
psp_core_request_is_for_this_proxy (osip_message_t * sip)
{
  osip_via_t *via;
  osip_generic_param_t *b;

  // partysip generate 2 kinds of branches:
  // The 8th char "f" for statefull answers
  // and "l" for stateless response.  :-/
  via = osip_list_get (sip->vias, 0);
  osip_via_param_get_byname (via, "branch", &b);
  if (b == NULL)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "discard response with Not acceptable Via!\n"));
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "This response may be for another proxy?!\n"));
      return -1;
    }
  if (strlen (b->gvalue) < 8)
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "discard response with Not acceptable Via!\n"));
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_ERROR, NULL,
                   "This response may be for another proxy?!\n"));
      return -1;
    }

  if (0 == strncmp ("z9hG4bKl", b->gvalue, 8)
      || 0 == strncmp ("z9hG4bKf", b->gvalue, 8))
    {
      OSIP_TRACE (osip_trace
                  (__FILE__, __LINE__, OSIP_INFO3, NULL,
                   "imp module: This response can be for this proxy (initial check passed)!\n"));
      return 0;
    }
  return -1;
}
*/

PPL_DECLARE (int) psp_core_event_resolv_add_query (char *address)
{
  ppl_dns_add_domain_query (address);
  module_wakeup (core->resolv->module);
  return 0;
}

PPL_DECLARE (int) psp_core_event_add_sfp_inc_traffic (osip_transaction_t * inc_tr)
{
  osip_fifo_add (core->sfp->osip_message_traffic, inc_tr);
  module_wakeup (core->sfp->module);
  return 0;
}

PPL_DECLARE (int) psp_core_event_add_sfp_inc_ack (osip_message_t * ack)
{
  osip_fifo_add (core->sfp->sip_ack, ack);
  module_wakeup (core->sfp->module);
  return 0;
}

PPL_DECLARE (int) psp_core_event_add_sip_message (osip_event_t * evt)
{
  osip_transaction_t *transaction;
  osip_message_t *answer1xx;
  int i;

  if (MSG_IS_REQUEST (evt->sip))
    {
      /* delete request where cseq method does not match
	 the method in request-line */
      if (evt->sip->cseq==NULL || evt->sip==NULL
	  || evt->sip->cseq->method==NULL || evt->sip->sip_method==NULL)
	{
	  osip_event_free (evt);
	  return -1;
	}
      if (0 != strcmp (evt->sip->cseq->method, evt->sip->sip_method))
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_WARNING, NULL,
		       "core module: Discard invalid message with method!=cseq!\n"));
	  osip_event_free (evt);
	  return -1;
	}
    }

  i = psp_core_find_osip_transaction_and_add_event (evt);
  if (i == 0)
    {
      psp_osip_wakeup (core->psp_osip);
      return 0;			/*evt consumed */
    }
  if (MSG_IS_REQUEST (evt->sip))
    {
      if (MSG_IS_ACK (evt->sip))
	{			/* continue as it is a new transaction... */
	  osip_route_t *route;

	  /* If a route header is present, then plugins will give
	     the correct location. */
	  osip_message_get_route (evt->sip, 0, &route);
	  if (route == NULL)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO1, NULL,
			   "core module: This is a late ACK to discard!\n"));
	      /* It can be a ACK for 200 ok, but those ACK SHOULD
	         never go through this proxy! (and should be sent to the
	         contact header of the 200ok */
#ifdef SUPPORT_FOR_BROKEN_UA
	      /* if this ACK has a request-uri that is not us,
		 forward the message there. How should I modify this
		 message?? */
	      
	      if (evt!=NULL
		  && evt->sip!=NULL
		  && evt->sip!=NULL
		  && evt->sip->req_uri!=NULL)
		{
		  if (psp_core_is_responsible_for_this_domain(evt->sip->req_uri)!=0)
		    {
		      int port = 5060;
		      if (evt->sip->req_uri->port != NULL)
			port = osip_atoi (evt->sip->req_uri->port);
		      psp_core_cb_snd_message(NULL, evt->sip,
					      evt->sip->req_uri->host,
					      port, -1);
 		    }
		}
#endif
	      osip_event_free (evt);
	      return 0;
	    }

	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO1, NULL,
		       "core module: This is a ACK for INVITE!\n"));
	  psp_core_event_add_sfp_inc_ack (evt->sip);
	  osip_free (evt);
	  return 0;
	}

      /* we can create the transaction and send a 1xx */
      transaction = osip_create_transaction (core->psp_osip->osip, evt);
      if (transaction == NULL)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO3, NULL,
		       "core module: Could not create a transaction for this request!\n"));
	  osip_event_free (evt);
	  return -1;
	}

      /* now, all retransmissions will be handled by oSIP. */

      /* From rfc3261: (Section: 16.2)
         "Thus, a stateful proxy SHOULD NOT generate 100 (Trying) responses
         to non-INVITE requests." */
      if (MSG_IS_INVITE (evt->sip))
	{
	  i = osip_msg_build_response (&answer1xx, 100, evt->sip);
	  if (i != 0)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_ERROR, NULL,
			   "sfp module: could not create a 100 Trying for this transaction. (discard it and let the transaction die itself)!\n"));
	      osip_event_free (evt);
	      return -1;
	    }

	  osip_transaction_add_event (transaction, evt);

	  evt = osip_new_outgoing_sipmessage (answer1xx);
	  evt->transactionid = transaction->transactionid;
	  osip_transaction_add_event (transaction, evt);
	}
      else
	osip_transaction_add_event (transaction, evt);

      psp_osip_wakeup (core->psp_osip);
      return 0;
    }
  else
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO3, NULL,
		   "sfp module: No module seems to be able to forward this response!\n"));
      /* this is probably a late answer? */
      /* let's forward it! */
      i = psp_core_handle_late_answer (evt->sip);
      if (i != 0)
	{
	  osip_event_free (evt);
	  return -1;
	}
      osip_event_free (evt);
    }
  return 0;
}

PPL_DECLARE (int) psp_core_event_add_sfull_request (psp_request_t * req)
{
  osip_fifo_add (core->sfp->sfull_request, req);
  module_wakeup (core->sfp->module);
  return 0;
}

PPL_DECLARE (int) psp_core_event_add_sfull_cancel (osip_transaction_t * tr_cancel)
{
  osip_fifo_add (core->sfp->sfull_cancels, tr_cancel);
  module_wakeup (core->sfp->module);
  return 0;
}
