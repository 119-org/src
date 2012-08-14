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
pspm_sfp_init (pspm_sfp_t ** pspm)
{
  int i;

  (*pspm) = (pspm_sfp_t *) osip_malloc (sizeof (pspm_sfp_t));
  if (*pspm == NULL)
    return -1;

  memset(*pspm, 0, sizeof(pspm_sfp_t));

  i = module_init (&((*pspm)->module), MOD_THREAD, "sfp");
  if (i != 0)
    goto psi_error1;


  (*pspm)->sfull_request = (osip_fifo_t *) osip_malloc (sizeof (osip_fifo_t));
  if ((*pspm)->sfull_request == NULL)
    goto psi_error3;
  osip_fifo_init ((*pspm)->sfull_request);

  (*pspm)->sfull_cancels = (osip_fifo_t *) osip_malloc (sizeof (osip_fifo_t));
  if ((*pspm)->sfull_cancels == NULL)
    goto psi_error4;
  osip_fifo_init ((*pspm)->sfull_cancels);

  (*pspm)->broken_transactions = (osip_list_t *) osip_malloc (sizeof (osip_list_t));
  if ((*pspm)->broken_transactions == NULL)
    goto psi_error5;
  osip_list_init ((*pspm)->broken_transactions);

  (*pspm)->osip_message_traffic = (osip_fifo_t *) osip_malloc (sizeof (osip_fifo_t));
  if ((*pspm)->osip_message_traffic == NULL)
    goto psi_error6;
  osip_fifo_init ((*pspm)->osip_message_traffic);

  (*pspm)->sip_ack = (osip_fifo_t *) osip_malloc (sizeof (osip_fifo_t));
  if ((*pspm)->sip_ack == NULL)
    goto psi_error7;
  osip_fifo_init ((*pspm)->sip_ack);

  i = sfp_inc_func_tab_init (&((*pspm)->rcv_invites));
  if (i != 0)
    goto psi_error8;
  i = sfp_inc_func_tab_init (&((*pspm)->rcv_acks));
  if (i != 0)
    goto psi_error8;
  i = sfp_inc_func_tab_init (&((*pspm)->rcv_registers));
  if (i != 0)
    goto psi_error8;
  i = sfp_inc_func_tab_init (&((*pspm)->rcv_byes));
  if (i != 0)
    goto psi_error8;
  i = sfp_inc_func_tab_init (&((*pspm)->rcv_optionss));
  if (i != 0)
    goto psi_error8;
  i = sfp_inc_func_tab_init (&((*pspm)->rcv_infos));
  if (i != 0)
    goto psi_error8;
  i = sfp_inc_func_tab_init (&((*pspm)->rcv_cancels));
  if (i != 0)
    goto psi_error8;
  i = sfp_inc_func_tab_init (&((*pspm)->rcv_notifys));
  if (i != 0)
    goto psi_error8;
  i = sfp_inc_func_tab_init (&((*pspm)->rcv_subscribes));
  if (i != 0)
    goto psi_error8;
  i = sfp_inc_func_tab_init (&((*pspm)->rcv_unknowns));
  if (i != 0)
    goto psi_error8;

  i = sfp_fwd_func_tab_init (&((*pspm)->fwd_invites));
  if (i != 0)
    goto psi_error8;
  i = sfp_fwd_func_tab_init (&((*pspm)->fwd_acks));
  if (i != 0)
    goto psi_error8;
  i = sfp_fwd_func_tab_init (&((*pspm)->fwd_registers));
  if (i != 0)
    goto psi_error8;
  i = sfp_fwd_func_tab_init (&((*pspm)->fwd_byes));
  if (i != 0)
    goto psi_error8;
  i = sfp_fwd_func_tab_init (&((*pspm)->fwd_optionss));
  if (i != 0)
    goto psi_error8;
  i = sfp_fwd_func_tab_init (&((*pspm)->fwd_infos));
  if (i != 0)
    goto psi_error8;
  i = sfp_fwd_func_tab_init (&((*pspm)->fwd_cancels));
  if (i != 0)
    goto psi_error8;
  i = sfp_fwd_func_tab_init (&((*pspm)->fwd_notifys));
  if (i != 0)
    goto psi_error8;
  i = sfp_fwd_func_tab_init (&((*pspm)->fwd_subscribes));
  if (i != 0)
    goto psi_error8;
  i = sfp_fwd_func_tab_init (&((*pspm)->fwd_unknowns));
  if (i != 0)
    goto psi_error8;

  i = sfp_rcv_func_tab_init (&((*pspm)->rcv_1xxs));
  if (i != 0)
    goto psi_error8;
  i = sfp_rcv_func_tab_init (&((*pspm)->rcv_2xxs));
  if (i != 0)
    goto psi_error8;
  i = sfp_rcv_func_tab_init (&((*pspm)->rcv_3xxs));
  if (i != 0)
    goto psi_error8;
  i = sfp_rcv_func_tab_init (&((*pspm)->rcv_4xxs));
  if (i != 0)
    goto psi_error8;
  i = sfp_rcv_func_tab_init (&((*pspm)->rcv_5xxs));
  if (i != 0)
    goto psi_error8;
  i = sfp_rcv_func_tab_init (&((*pspm)->rcv_6xxs));
  if (i != 0)
    goto psi_error8;

  i = sfp_snd_func_tab_init (&((*pspm)->snd_1xxs));
  if (i != 0)
    goto psi_error8;
  i = sfp_snd_func_tab_init (&((*pspm)->snd_2xxs));
  if (i != 0)
    goto psi_error8;
  i = sfp_snd_func_tab_init (&((*pspm)->snd_3xxs));
  if (i != 0)
    goto psi_error8;
  i = sfp_snd_func_tab_init (&((*pspm)->snd_4xxs));
  if (i != 0)
    goto psi_error8;
  i = sfp_snd_func_tab_init (&((*pspm)->snd_5xxs));
  if (i != 0)
    goto psi_error8;
  i = sfp_snd_func_tab_init (&((*pspm)->snd_6xxs));
  if (i != 0)
    goto psi_error8;

  __proxy_load_fsm ();

  return 0;

psi_error8:
  module_release ((*pspm)->module);
  pspm_sfp_free (*pspm);
  return -1;

psi_error7:
  osip_fifo_free ((*pspm)->osip_message_traffic);
psi_error6:
  osip_free ((*pspm)->broken_transactions);
psi_error5:
  osip_fifo_free ((*pspm)->sfull_cancels);
psi_error4:
  osip_fifo_free ((*pspm)->sfull_request);
psi_error3:
  /*
     osip_mutex_destroy ((*pspm)->mut_psp_requests);
     psi_error2: */

  module_release ((*pspm)->module);
  module_free ((*pspm)->module);
psi_error1:
  osip_free (*pspm);
  return -1;
}

void
pspm_sfp_free (pspm_sfp_t * pspm)
{
  sfp_plugin_t *p;
  psp_request_t *req;
  osip_transaction_t *tr;
  osip_message_t *ack;

  if (pspm == NULL)
    return;

  sfp_inc_func_tab_free (pspm->rcv_invites);
  sfp_inc_func_tab_free (pspm->rcv_acks);
  sfp_inc_func_tab_free (pspm->rcv_registers);
  sfp_inc_func_tab_free (pspm->rcv_byes);
  sfp_inc_func_tab_free (pspm->rcv_optionss);
  sfp_inc_func_tab_free (pspm->rcv_infos);
  sfp_inc_func_tab_free (pspm->rcv_cancels);
  sfp_inc_func_tab_free (pspm->rcv_notifys);
  sfp_inc_func_tab_free (pspm->rcv_subscribes);
  sfp_inc_func_tab_free (pspm->rcv_unknowns);

  sfp_fwd_func_tab_free (pspm->fwd_invites);
  sfp_fwd_func_tab_free (pspm->fwd_acks);
  sfp_fwd_func_tab_free (pspm->fwd_registers);
  sfp_fwd_func_tab_free (pspm->fwd_byes);
  sfp_fwd_func_tab_free (pspm->fwd_optionss);
  sfp_fwd_func_tab_free (pspm->fwd_infos);
  sfp_fwd_func_tab_free (pspm->fwd_cancels);
  sfp_fwd_func_tab_free (pspm->fwd_notifys);
  sfp_fwd_func_tab_free (pspm->fwd_subscribes);
  sfp_fwd_func_tab_free (pspm->fwd_unknowns);

  sfp_rcv_func_tab_free (pspm->rcv_1xxs);
  sfp_rcv_func_tab_free (pspm->rcv_2xxs);
  sfp_rcv_func_tab_free (pspm->rcv_3xxs);
  sfp_rcv_func_tab_free (pspm->rcv_4xxs);
  sfp_rcv_func_tab_free (pspm->rcv_5xxs);
  sfp_rcv_func_tab_free (pspm->rcv_6xxs);

  sfp_snd_func_tab_free (pspm->snd_1xxs);
  sfp_snd_func_tab_free (pspm->snd_2xxs);
  sfp_snd_func_tab_free (pspm->snd_3xxs);
  sfp_snd_func_tab_free (pspm->snd_4xxs);
  sfp_snd_func_tab_free (pspm->snd_5xxs);
  sfp_snd_func_tab_free (pspm->snd_6xxs);

  module_free (pspm->module);

  tr = osip_fifo_tryget (pspm->osip_message_traffic);
  while (tr != NULL)
    {
      osip_transaction_free (tr);
      tr = osip_fifo_tryget (pspm->osip_message_traffic);
    }

  osip_fifo_free (pspm->osip_message_traffic);

  ack = osip_fifo_tryget (pspm->sip_ack);
  while (ack != NULL)
    {
      osip_message_free (ack);
      ack = osip_fifo_tryget (pspm->sip_ack);
    }

  osip_fifo_free (pspm->sip_ack);

  req = osip_fifo_tryget (pspm->sfull_request);
  while (req != NULL)
    {
      psp_request_free (req);
      req = osip_fifo_tryget (pspm->sfull_request);
    }

  osip_fifo_free (pspm->sfull_request);

  tr = osip_fifo_tryget (pspm->sfull_cancels);
  while (tr != NULL)
    {
      if (tr->state == NIST_TERMINATED)
	{
	  osip_transaction_free2 (tr);
	}
      else
	{
	  osip_transaction_free (tr);
	}
      tr = osip_fifo_tryget (pspm->sfull_cancels);
    }

  osip_fifo_free (pspm->sfull_cancels);

  tr = osip_list_get (pspm->broken_transactions, 0);
  while (tr != NULL)
    {
      osip_list_remove (pspm->broken_transactions, 0);
      if (tr->state == NICT_TERMINATED || tr->state == ICT_TERMINATED
	  || tr->state == NIST_TERMINATED || tr->state == IST_TERMINATED)
	{
	  osip_transaction_free2 (tr);
	}
      else
	{
	  osip_transaction_free (tr);
	}
      tr = osip_list_get (pspm->broken_transactions, 0);
    }

  osip_free (pspm->broken_transactions);

  /*
     osip_mutex_destroy (pspm->mut_psp_requests);
   */

  for (req = pspm->psp_requests; req != NULL; req = pspm->psp_requests)
    {
      REMOVE_ELEMENT (pspm->psp_requests, req);
      psp_request_free (req);
    }

  for (p = pspm->sfp_plugins; p != NULL; p = pspm->sfp_plugins)
    {
      REMOVE_ELEMENT (pspm->sfp_plugins, p);
      sfp_plugin_free (p);
    }

  __proxy_unload_fsm();
  osip_free(pspm);
}


int
pspm_sfp_release (pspm_sfp_t * pspm)
{
  int i;

  i = module_release (pspm->module);
  if (i != 0)
    return -1;
  return 0;
}

int
pspm_sfp_start (pspm_sfp_t * pspm, void *(*func_start) (void *), void *arg)
{
  int i;

  i = module_start (pspm->module, func_start, arg);
  if (i != 0)
    return -1;
  return 0;
}

int
pspm_sfp_execute (pspm_sfp_t * pspm, int sec_max, int usec_max, int max)
{
  osip_transaction_t *inc_tr;
  osip_message_t *ack;
  psp_request_t *req;
  psp_request_t *pnext;

  fd_set sfp_fdset;
  int max_fd;
  struct timeval tv;

  if (pspm == NULL)
    return -1;

  while (1)
    {
      int s;
      int i;

      tv.tv_sec = sec_max;
      tv.tv_usec = usec_max;

      max--;
      FD_ZERO (&sfp_fdset);
      s = ppl_pipe_get_read_descr (pspm->module->wakeup);
      if (s <= 1)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "No wakeup value in module sfp!\n"));
	  return -1;
	}
      FD_SET (s, &sfp_fdset);

      max_fd = s;

      if ((sec_max == -1) || (usec_max == -1))
	i = select (max_fd + 1, &sfp_fdset, NULL, NULL, NULL);
      else
	i = select (max_fd + 1, &sfp_fdset, NULL, NULL, &tv);

      if (FD_ISSET (s, &sfp_fdset))
	{
	  char tmp[51];

	  i = ppl_pipe_read (pspm->module->wakeup, tmp, 50);
	  if (i >= 1 && tmp[0] == 'q')
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO2, NULL,
			   "sfp module: Exiting!\n"));
	      return 1;
	    }
	  else
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO3, NULL,
			   "sfp module: wake up!\n"));
	    }
	}

      psp_core_lock ();
      
      /**************************/
      /* Handle incoming ACK    */
      ack = osip_fifo_tryget (pspm->sip_ack);
      while (ack != NULL)
	{
	  pspm_sfp_inc_dispatch_ack (pspm, ack);
	  ack = osip_fifo_tryget (pspm->sip_ack);
	}

      /*****************************************/
      /*    handle new incoming transactions   */
      inc_tr = osip_fifo_tryget (pspm->osip_message_traffic);
      while (inc_tr != NULL)
	{
	  pspm_sfp_inc_dispatch_traffic (pspm, inc_tr);
	  inc_tr = osip_fifo_tryget (pspm->osip_message_traffic);
	}

      /*********************************************/
      /*    handle new incoming CANCEL requests    */
      inc_tr = osip_fifo_tryget (pspm->sfull_cancels);
      while (inc_tr != NULL)
	{
	  /* psp_req element containing a request coming from imp */
	  dispatch_cancel_event (pspm, inc_tr);
	  inc_tr = osip_fifo_tryget (pspm->sfull_cancels);
	}

      /***********************************************************/
      /* Prepare statefull context for new psp_request_t elements.   */
      req = osip_fifo_tryget (pspm->sfull_request);
      while (req != NULL)
	{
	  /* psp_req element containing a request coming from imp */
	  psp_event_t * evt = __psp_event_new(EVT_RCV_REQUEST);

	  if (__IS_PSP_SEQ_MODE (req->flag))
	    {
	      location_t *loc;
	      location_t *ptr = NULL;
	      if (req->locations!=NULL)
		ptr = req->locations->next;
	      /* move all secondary locations to the fallback list */
	      for (loc=ptr; loc != NULL; loc = ptr)
		{
		  ptr = loc->next;
		  REMOVE_ELEMENT(req->locations, loc);
		  ADD_ELEMENT(req->fallback_locations, loc);
		}
	    }
	  if (req->fallback_locations!=NULL
	      && req->timer_nofinalanswer_length>60 * 1000)
	    req->timer_nofinalanswer_length = 60 * 1000;

	  psp_request_execute(req, evt);

	  if (req->inc_ack==NULL) /* ACK are forwarded and deleted */
	    { ADD_ELEMENT(pspm->psp_requests, req); }

	  req = osip_fifo_tryget (pspm->sfull_request);
	}

      /* for each state, try to build possible events. */
      for (pnext = pspm->psp_requests, req = pnext; pnext != NULL;
	   req = pnext)
	{
	  psp_event_t *evt;
	  pnext = req->next;

	  evt = pspm_sfp_test_received_1xx(req);
	  if (evt!=NULL)
	    psp_request_execute(req, evt);
	  evt = pspm_sfp_test_received_2xx(req);
	  if (evt!=NULL)
	    psp_request_execute(req, evt);
	  evt = pspm_sfp_test_received_3456xx(req);
	  if (evt!=NULL)
	    psp_request_execute(req, evt);

	  evt = pspm_sfp_new_location_received(req);
	  if (evt!=NULL)
	    psp_request_execute(req, evt);

	  evt = pspm_sfp_need_start_fallback_location(req);
	  if (evt!=NULL)
	    psp_request_execute(req, evt);

	  evt = pspm_sfp_need_timeout_nofinalanswer(req);
	  if (evt!=NULL)
	    psp_request_execute(req, evt);
	  evt = pspm_sfp_need_timeout_noanswer(req);
	  if (evt!=NULL)
	    psp_request_execute(req, evt);
	  evt = pspm_sfp_need_timeout_close(req);
	  if (evt!=NULL)
	    psp_request_execute(req, evt);
	}


      for (pnext = pspm->psp_requests, req = pnext; pnext != NULL;
	   req = pnext)
	{
	  pnext = req->next;
	  if (req->state==PROXY_CLOSED)
	    {
	      REMOVE_ELEMENT(pspm->psp_requests, req);
	      psp_request_free(req);
	    }
	}

      psp_core_unlock ();

      if (max == 0)
	return 0;
    }
}

static int
cancel_match_invite (osip_transaction_t * invite, osip_message_t * cancel)
{
  osip_generic_param_t *br;
  osip_generic_param_t *br2;
  osip_via_t *via;

  osip_via_param_get_byname (invite->topvia, "branch", &br);
  via = osip_list_get (&cancel->vias, 0);
  if (via == NULL)
    return -1;			/* request without via??? */
  osip_via_param_get_byname (via, "branch", &br2);
  if (br != NULL && br2 == NULL)
    return -1;
  if (br2 != NULL && br == NULL)
    return -1;
  if (br2 != NULL && br != NULL)	/* compliant UA  :) */
    {
      if (br->gvalue != NULL && br2->gvalue != NULL &&
	  0 == strcmp (br->gvalue, br2->gvalue))
	return 0;
      return -1;
    }
  /* old backward compatibility mechanism */
  if (0 != osip_call_id_match (invite->callid, cancel->call_id))
    return -1;
  if (core->disable_check_for_osip_to_tag_in_cancel == 0)
    {
      if (0 != osip_to_tag_match (invite->to, cancel->to))
	return -1;
    }
  if (0 != osip_from_tag_match (invite->from, cancel->from))
    return -1;
  if (0 != strcmp (invite->cseq->number, cancel->cseq->number))
    return -1;
  if (0 != osip_via_match (invite->topvia, via))
    return -1;
  return 0;
}

void
dispatch_cancel_event(pspm_sfp_t * pspm, osip_transaction_t * tr_cancel)
{
  psp_event_t *evt2;
  psp_request_t *req;
  osip_event_t *evt;
  osip_message_t *response;
  int i;

  /* find the transaction */
  for (req = pspm->psp_requests; req != NULL; req = req->next)
    {
      if (req->inc_tr!=NULL
	  && 0 == cancel_match_invite (req->inc_tr, tr_cancel->orig_request))
	{
	  break;
	}
    }

  osip_list_add (pspm->broken_transactions, tr_cancel, 0);
  if (req == NULL
      || req->inc_tr==NULL
      || req->state == PROXY_CALLING /* no answer received!! */
      || req->inc_tr->state == IST_COMPLETED
      || req->inc_tr->state == IST_TERMINATED)
    {
      if (req != NULL)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO2, NULL,
		       "module sfp: Request already terminated.!\n"));
	}
      else
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_WARNING, NULL,
		       "module sfp: No valid INVITE found; answer to request with 481.!\n"));
	}

      i = osip_msg_build_response (&response, 481, tr_cancel->orig_request);
      /* should we fix the to tag with one of the provisonnal response? */
      if (i != 0)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "module sfp: Cannot build 481 response!\n"));
	  /* There is no effect on other transactions */
	  /* BUG fix: rsp_ctx_set_state (req->rsp_ctx, RSPCTX_BROKEN); */
	  return;
	}
      evt = osip_new_outgoing_sipmessage (response);
      evt->transactionid = tr_cancel->transactionid;
      osip_transaction_add_event (tr_cancel, evt);
      psp_osip_wakeup (core->psp_osip);
      return;
    }


  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO2, NULL,
	       "module sfp: Cancelling the request.!\n"));

  /* reply 200 Ok */
  i = osip_msg_build_response (&response, 200, tr_cancel->orig_request);
  /* should we fix the to tag with one of the provisonnal response? */
  if (i != 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "module sfp: Cannot build 200 response for CANCEL!\n"));

      return;
    }
  evt = osip_new_outgoing_sipmessage (response);
  evt->transactionid = tr_cancel->transactionid;
  osip_transaction_add_event (tr_cancel, evt);
  psp_osip_wakeup (core->psp_osip);

  /* send event to the state machine */
  evt2 = __psp_event_new (EVT_RCV_CANCEL);
  if (evt2!=NULL)
    psp_request_execute(req, evt2);
}


psp_event_t *
pspm_sfp_need_start_fallback_location(psp_request_t *req)
{
  struct timeval now;
  gettimeofday(&now, NULL);

  if (req->fallback_locations==NULL)
    return NULL; /* This event could not happen when no location is given */

  if (req->state==PROXY_CALLING
      || req->state==PROXY_PROCEEDING)
    {
      if (req->timer_noanswer_start.tv_sec != -1 && 
	  timercmp(&now, &req->timer_noanswer_start, > ))
	return __psp_event_new (EVT_FALLBACK_LOCATION);
      else if (req->timer_nofinalanswer_start.tv_sec != -1 && 
	  timercmp(&now, &req->timer_nofinalanswer_start, > ))
	return __psp_event_new (EVT_FALLBACK_LOCATION);
    }
  return NULL;
}

psp_event_t *
pspm_sfp_new_location_received(psp_request_t *req)
{
  location_t *loc;
  location_t *ptr;
  for (loc = req->locations; loc != NULL; loc = ptr)
    {
      int is_an_ip;
      struct in6_addr addr;
      ptr = loc->next;
      is_an_ip = ppl_inet_pton ((const char *)loc->url->host, (void *)&addr);
      if (is_an_ip == -1)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_BUG, NULL,
		       "module sfp: The bad location should be already removed!\n"));
	  REMOVE_ELEMENT (req->locations, loc);
	  location_free(loc);
	}
      else if (is_an_ip == 0)
	{
	  ppl_dns_entry_t *dns_result;
	  int i;
	  i = ppl_dns_get_result (&dns_result, loc->url->host);
	  if (i == 0)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO2, NULL,
			   "module sfp: A location has been resolved!\n"));
	      return __psp_event_new (EVT_NEW_LOCATION);
	    }
	  else
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO3, NULL,
			   "module sfp: Keep waiting for the resolution to succeed...!\n"));
	    }
	}
      else
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_BUG, NULL,
		       "module sfp: The correct location should be already removed!\n"));
	  REMOVE_ELEMENT (req->locations, loc);
	  location_free (loc);
	}
    }
  return NULL;
}

psp_event_t *
pspm_sfp_need_timeout_noanswer(psp_request_t *req)
{
  struct timeval now;
  gettimeofday(&now, NULL);

  if (req->state==PROXY_PRE_CALLING
      ||req->state==PROXY_CALLING)
    {
      if (req->timer_noanswer_start.tv_sec == -1)
	return NULL;
      if (timercmp(&now, &req->timer_noanswer_start, > ))
	return __psp_event_new (TIMEOUT_NOANSWER);
    }
  return NULL;
}

psp_event_t *
pspm_sfp_need_timeout_nofinalanswer(psp_request_t *req)
{
  struct timeval now;
  gettimeofday(&now, NULL);

  if (req->state==PROXY_PROCEEDING)
    {
      if (req->timer_nofinalanswer_start.tv_sec == -1)
	return NULL;
      if (timercmp(&now, &req->timer_nofinalanswer_start, > ))
	return __psp_event_new (TIMEOUT_NOFINALANSWER);
    }
  return NULL;
}

psp_event_t *
pspm_sfp_need_timeout_close(psp_request_t *req)
{
  struct timeval now;
  gettimeofday(&now, NULL);

  if (req->state==PROXY_CLOSING || req->state==PROXY_PROCEEDING)
    {
      if (req->state==PROXY_CLOSING)
	{
	  sfp_branch_t *br;
	  int to_close = 0; /* yes */

	  if (req->inc_tr==NULL)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO2, NULL,
			   "module sfp: close this empty transaction!\n"));
	      return __psp_event_new (TIMEOUT_CLOSE);
	    }

	  /* if all transaction are terminated -> send a TIMEOUT_CLOSE */
	  for (br = req->branch_notanswered;
	       br != NULL;
	       br = br->next)
	    {
	      if (br->out_tr==NULL)
		{}
	      else if (br->out_tr->state == NICT_TERMINATED
		       || br->out_tr->state == ICT_TERMINATED)
		{}
	      else
		{
		  to_close = 1; /* do not close */
		  break;
		}
	    }
	  for (br = req->branch_answered;
	       br != NULL && to_close != 1;
	       br = br->next)
	    {
	      if (br->out_tr==NULL)
		{}
	      else if (br->out_tr->state == NICT_TERMINATED
		       || br->out_tr->state == ICT_TERMINATED)
		{}
	      else
		{
		  to_close = 1; /* do not close */
		  break;
		}
	    }
	  for (br = req->branch_completed;
	       br != NULL && to_close != 1;
	       br = br->next)
	    {
	      if (br->out_tr==NULL)
		{}
	      else if (br->out_tr->state == NICT_TERMINATED
		       || br->out_tr->state == ICT_TERMINATED)
		{}
	      else
		{
		  to_close = 1; /* do not close */
		  break;
		}
	    }
	  for (br = req->branch_cancelled;
	       br != NULL && to_close != 1;
	       br = br->next)
	    {
	      if (br->out_tr==NULL)
		{}
	      else if (br->out_tr->state == NICT_TERMINATED
		       || br->out_tr->state == ICT_TERMINATED)
		{}
	      else
		{
		  to_close = 1; /* do not close */
		  break;
		}
	    }
	  if (to_close==0)
	    {
	      if (req->inc_tr->state == NIST_TERMINATED
		  || req->inc_tr->state == IST_TERMINATED)
		{}
	      else to_close = 1; /* do not close */
	    }
	  if (to_close==0) /* all transaction are finished */
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO2, NULL,
			   "module sfp: all transaction are terminated here!\n"));
	      return __psp_event_new (TIMEOUT_CLOSE);
	    }
	}

      if (req->timer_close_start.tv_sec == -1)
	return NULL;
      if (timercmp(&now, &req->timer_close_start, > ))
	return __psp_event_new (TIMEOUT_CLOSE);
    }
  return NULL;
}

psp_event_t *
pspm_sfp_test_received_1xx(psp_request_t *req)
{
  if (req->state==PROXY_CALLING || req->state==PROXY_PROCEEDING)
    {
      sfp_branch_t *br;
      for (br = req->branch_notanswered;
	   br != NULL;
	   br = br->next)
	{
	  if (br->out_tr==NULL)
	    {}
	  else if (br->out_tr->last_response==NULL)
	    {}
	  else if (!MSG_IS_STATUS_1XX(br->out_tr->last_response))
	    {}
	  else if (br->out_tr->last_response->status_code>br->last_code_forwarded)
	    {
	      psp_event_t *evt;
	      evt = __psp_event_new (EVT_RCV_STATUS_1XX);
	      __psp_event_set_response(evt, br->branchid);
	      return evt;
	    }
	}

      for (br = req->branch_answered;
	   br != NULL;
	   br = br->next)
	{
	  if (br->out_tr==NULL)
	    {}
	  else if (br->out_tr->last_response==NULL)
	    {}
	  else if (!MSG_IS_STATUS_1XX(br->out_tr->last_response))
	    {}
	  else if (br->out_tr->last_response->status_code>br->last_code_forwarded)
	    {
	      psp_event_t *evt;
	      evt = __psp_event_new (EVT_RCV_STATUS_1XX);
	      __psp_event_set_response(evt, br->branchid);
	      return evt;
	    }
	}
    }
  return NULL;
}

psp_event_t *
pspm_sfp_test_received_2xx(psp_request_t *req)
{
  if (req->state==PROXY_CALLING || req->state==PROXY_PROCEEDING)
    {
      sfp_branch_t *br;
      for (br = req->branch_notanswered;
	   br != NULL;
	   br = br->next)
	{
	  if (br->out_tr==NULL)
	    {}
	  else if (br->out_tr->last_response==NULL)
	    {}
	  else if (!MSG_IS_STATUS_2XX(br->out_tr->last_response))
	    {}
	  else if (br->out_tr->last_response->status_code>br->last_code_forwarded)
	    {
	      psp_event_t *evt;
	      evt = __psp_event_new (EVT_RCV_STATUS_2XX);
	      __psp_event_set_response(evt, br->branchid);
	      return evt;
	    }
	}

      for (br = req->branch_answered;
	   br != NULL;
	   br = br->next)
	{
	  if (br->out_tr==NULL)
	    {}
	  else if (br->out_tr->last_response==NULL)
	    {}
	  else if (!MSG_IS_STATUS_2XX(br->out_tr->last_response))
	    {}
	  else if (br->out_tr->last_response->status_code>br->last_code_forwarded)
	    {
	      psp_event_t *evt;
	      evt = __psp_event_new (EVT_RCV_STATUS_2XX);
	      __psp_event_set_response(evt, br->branchid);
	      return evt;
	    }
	}
    }
  return NULL;
}

psp_event_t *
pspm_sfp_test_received_3456xx(psp_request_t *req)
{
  if (req->state==PROXY_CALLING || req->state==PROXY_PROCEEDING)
    {
      sfp_branch_t *br;
      for (br = req->branch_notanswered;
	   br != NULL;
	   br = br->next)
	{
	  if (br->out_tr==NULL)
	    return NULL;
	  else if (br->out_tr->last_response==NULL)
	    return NULL;
	  else if (br->out_tr->last_response->status_code<299)
	    return NULL;
	  else if (br->out_tr->last_response->status_code>br->last_code_forwarded)
	    {
	      psp_event_t *evt;
	      evt = __psp_event_new (EVT_RCV_STATUS_3456XX);
	      __psp_event_set_response(evt, br->branchid);
	      return evt;
	    }
	}

      for (br = req->branch_answered;
	   br != NULL;
	   br = br->next)
	{
	  if (br->out_tr==NULL)
	    {}
	  else if (br->out_tr->last_response==NULL)
	    {}
	  else if (br->out_tr->last_response->status_code<299)
	    {}
	  else if (br->out_tr->last_response->status_code>br->last_code_forwarded)
	    {
	      psp_event_t *evt;
	      evt = __psp_event_new (EVT_RCV_STATUS_3456XX);
	      __psp_event_set_response(evt, br->branchid);
	      return evt;
	    }
	}
    }
  return NULL;
}

int
pspm_sfp_call_plugins_for_request (pspm_sfp_t * pspm, psp_request_t * req,
				   sfp_branch_t * sfp_branch)
{
  sfp_fwd_func_tab_t *t;
  sfp_fwd_func_t *f;
  osip_message_t *request;
  request = psp_request_get_request(req);
  if (request==NULL) return -1;

  if (MSG_IS_INVITE (request))
    t = pspm->fwd_invites;
  else if (MSG_IS_ACK (request))
    t = pspm->fwd_acks;
  else if (MSG_IS_REGISTER (request))
    t = pspm->fwd_registers;
  else if (MSG_IS_BYE (request))
    t = pspm->fwd_byes;
  else if (MSG_IS_OPTIONS (request))
    t = pspm->fwd_optionss;
  else if (MSG_IS_INFO (request))
    t = pspm->fwd_infos;
  else if (MSG_IS_CANCEL (request))
    t = pspm->fwd_cancels;
  else if (MSG_IS_NOTIFY (request))
    t = pspm->fwd_notifys;
  else if (MSG_IS_SUBSCRIBE (request))
    t = pspm->fwd_subscribes;
  else
    t = pspm->fwd_unknowns;

  for (f = t->func_hook_really_first; f != NULL; f = f->next)
    {
      f->cb_fwd_request_func (req);
      if (__IS_PSP_STOP (req->flag))
	return 0;
    }
  for (f = t->func_hook_first; f != NULL; f = f->next)
    {
      f->cb_fwd_request_func (req);
      if (__IS_PSP_STOP (req->flag))
	return 0;
    }
  for (f = t->func_hook_middle; f != NULL; f = f->next)
    {
      f->cb_fwd_request_func (req);
      if (__IS_PSP_STOP (req->flag))
	return 0;
    }
  for (f = t->func_hook_last; f != NULL; f = f->next)
    {
      f->cb_fwd_request_func (req);
      if (__IS_PSP_STOP (req->flag))
	return 0;
    }
  for (f = t->func_hook_really_last; f != NULL; f = f->next)
    {
      f->cb_fwd_request_func (req);
      if (__IS_PSP_STOP (req->flag))
	return 0;
    }
  return 0;
}

int
pspm_sfp_call_plugins_for_rcv_response (pspm_sfp_t * pspm, psp_request_t * req,
					sfp_branch_t * branch)
{
  sfp_rcv_func_tab_t *t;
  sfp_rcv_func_t *f;
  osip_message_t *response;
  if (req==NULL)
    return -1;
  else if (req->inc_tr==NULL)
    return -1;
  if (req->inc_tr->last_response==NULL)
    return -1;
  response = req->inc_tr->last_response;

  if (MSG_IS_STATUS_1XX (response))
    t = pspm->rcv_1xxs;
  else if (MSG_IS_STATUS_2XX (response))
    t = pspm->rcv_2xxs;
  else if (MSG_IS_STATUS_3XX (response))
    t = pspm->rcv_3xxs;
  else if (MSG_IS_STATUS_4XX (response))
    t = pspm->rcv_4xxs;
  else if (MSG_IS_STATUS_5XX (response))
    t = pspm->rcv_5xxs;
  else if (MSG_IS_STATUS_6XX (response))
    t = pspm->rcv_6xxs;
  else
    {
      return -1;
    }

  for (f = t->func_hook_really_first; f != NULL; f = f->next)
    {
      f->cb_rcv_answer_func (req);
      if (__IS_PSP_STOP (req->flag))
	return 0;
    }
  for (f = t->func_hook_first; f != NULL; f = f->next)
    {
      f->cb_rcv_answer_func (req);
      if (__IS_PSP_STOP (req->flag))
	return 0;
    }
  for (f = t->func_hook_middle; f != NULL; f = f->next)
    {
      f->cb_rcv_answer_func (req);
      if (__IS_PSP_STOP (req->flag))
	return 0;
    }
  for (f = t->func_hook_last; f != NULL; f = f->next)
    {
      f->cb_rcv_answer_func (req);
      if (__IS_PSP_STOP (req->flag))
	return 0;
    }
  for (f = t->func_hook_really_last; f != NULL; f = f->next)
    {
      f->cb_rcv_answer_func (req);
      if (__IS_PSP_STOP (req->flag))
	return 0;
    }
  return 0;
}


int
pspm_sfp_call_plugins_for_snd_response (pspm_sfp_t * pspm,
					psp_request_t * req,
					osip_message_t * response)
{
  sfp_snd_func_tab_t *t;
  sfp_snd_func_t *f;

  if (MSG_IS_STATUS_1XX (response))
    t = pspm->snd_1xxs;
  else if (MSG_IS_STATUS_2XX (response))
    t = pspm->snd_2xxs;
  else if (MSG_IS_STATUS_3XX (response))
    t = pspm->snd_3xxs;
  else if (MSG_IS_STATUS_4XX (response))
    t = pspm->snd_4xxs;
  else if (MSG_IS_STATUS_5XX (response))
    t = pspm->snd_5xxs;
  else if (MSG_IS_STATUS_6XX (response))
    t = pspm->snd_6xxs;
  else
    {
      printf ("ERROR: I do not understand this response");
      return -1;
    }

  for (f = t->func_hook_really_first; f != NULL; f = f->next)
    {
      f->cb_snd_answer_func (req, response);
      if (__IS_PSP_STOP (req->flag))
	return 0;
    }
  for (f = t->func_hook_first; f != NULL; f = f->next)
    {
      f->cb_snd_answer_func (req, response);
      if (__IS_PSP_STOP (req->flag))
	return 0;
    }
  for (f = t->func_hook_middle; f != NULL; f = f->next)
    {
      f->cb_snd_answer_func (req, response);
      if (__IS_PSP_STOP (req->flag))
	return 0;
    }
  for (f = t->func_hook_last; f != NULL; f = f->next)
    {
      f->cb_snd_answer_func (req, response);
      if (__IS_PSP_STOP (req->flag))
	return 0;
    }
  for (f = t->func_hook_really_last; f != NULL; f = f->next)
    {
      f->cb_snd_answer_func (req, response);
      if (__IS_PSP_STOP (req->flag))
	return 0;
    }
  return 0;
}

