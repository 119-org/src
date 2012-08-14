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
#include <osip_msg.h>
#include <ppl/ppl_uinfo.h>

extern psp_core_t *core;

int
pspm_sfp_inc_dispatch_ack (pspm_sfp_t * pspm, osip_message_t * ack)
{
  psp_request_t *psp_req;
  int i;

  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO3, NULL,
	       "imp module: dispatching ACK!\n"));

  /* so it's a new request. (it must be?) */
  /* it's time to create a psp_request_t element and call plugins */

  i = psp_request_init (&psp_req, NULL, ack);
  if (i != 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "Could not create psp_req!\n"));
      osip_message_free(ack);
      return -1;
    }

  i = pspm_sfp_inc_call_plugins (pspm, psp_req);
  if (i == -2)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "psp_req rejected by plugin!\n"));
      psp_request_free (psp_req);
      return -1;
    }
  i = pspm_sfp_inc_dispatch_psp_request (pspm, psp_req);
  if (i != 0)
    return -1;

  return 0;
}

int
pspm_sfp_inc_dispatch_traffic (pspm_sfp_t * pspm, osip_transaction_t * inc_tr)
{
  psp_request_t *psp_req;
  int i;
  static int date = 0;
  int now;

  if (date==0)
    date = time(NULL);
  now = time(NULL);
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO3, NULL,
	       "imp module: dispatching request!\n"));

  /* so it's a new request. (it must be?) */
  /* it's time to create a psp_request_t element and call plugins */

  i = psp_request_init (&psp_req, inc_tr, NULL);
  if (i != 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "Could not create psp_req!\n"));
      /* inc_tr should be freed here? */
      
      return -1;
    }

  i = pspm_sfp_inc_call_plugins (pspm, psp_req);
  if (i == -2)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "psp_req rejected by plugin!\n"));
      psp_request_free (psp_req);
      return -1;
    }

  if (core->recovery_delay!=-1 && now-date > core->recovery_delay)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "Flush Data in dbm file!\n"));
      ppl_uinfo_flush_dbm();
      date = now;
    }

  i = pspm_sfp_inc_dispatch_psp_request (pspm, psp_req);
  if (i != 0)
    return -1;

  return 0;
}

int
pspm_sfp_inc_dispatch_psp_request (pspm_sfp_t * pspm, psp_request_t * req)
{
  /* case1: flag in req->flag contains the bit for UAS_MODE */
  if (__IS_PSP_UAS_MODE (req->flag))
    {
      if (req->inc_tr==NULL)
	{
	  /* if a ACK is coming here, it could be a late ACK
	     for wich the transaction is already finished.
	     We can safely discard it. */
	  if (req!=NULL && req->inc_ack!=NULL && req->inc_ack->sip_method!=NULL)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_WARNING, NULL,
			   "A psp_req element without transaction is discarded (%s)!\n", req->inc_ack->sip_method));
	    }
	  else
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_ERROR, NULL,
			   "A broken psp_req element without transaction is discarded!\n"));
	    }
	  psp_request_free(req);
	  return 0;
	}

      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO1, NULL,
		   "Swithing to uas mode!\n"));
    }
  else if (__IS_PSP_FORK_MODE (req->flag))
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO1, NULL,
		   "Swithing to sfull (forking) mode!\n"));
    }
  else if (__IS_PSP_SEQ_MODE (req->flag))
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO1, NULL,
		   "Swithing to sfull (sequential) mode!\n"));
    }
  else
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_BUG, NULL,
		   "BUG: No mode asked?? deleting request!\n"));
      psp_request_free (req);
      return -1;
    }

  /* new architecture -> always handled by the sfp module */
  psp_core_event_add_sfull_request (req);
  return 0;
}

int
pspm_sfp_inc_call_plugins (pspm_sfp_t * pspm, psp_request_t * req)
{
  sfp_inc_func_tab_t *t;
  sfp_inc_func_t *f;
  osip_message_t *request;
  request = psp_request_get_request(req);
  if (request==NULL) return -1;

  if (MSG_IS_INVITE (request))
    t = pspm->rcv_invites;
  else if (MSG_IS_ACK (request))
    t = pspm->rcv_acks;
  else if (MSG_IS_REGISTER (request))
    t = pspm->rcv_registers;
  else if (MSG_IS_BYE (request))
    t = pspm->rcv_byes;
  else if (MSG_IS_OPTIONS (request))
    t = pspm->rcv_optionss;
  else if (MSG_IS_INFO (request))
    t = pspm->rcv_infos;
  else if (MSG_IS_CANCEL (request))
    t = pspm->rcv_cancels;
  else if (MSG_IS_NOTIFY (request))
    t = pspm->rcv_notifys;
  else if (MSG_IS_SUBSCRIBE (request))
    t = pspm->rcv_subscribes;
  else
    t = pspm->rcv_unknowns;

  for (f = t->func_hook_really_first; f != NULL; f = f->next)
    {
      f->cb_rcv_func (req);
      if (__IS_PSP_MANDATE (req->flag))
	goto mandate;
      if (__IS_PSP_STOP (req->flag))
	return -2;
    }
  for (f = t->func_hook_first; f != NULL; f = f->next)
    {
      f->cb_rcv_func (req);
      if (__IS_PSP_MANDATE (req->flag))
	goto mandate;
      if (__IS_PSP_STOP (req->flag))
	return -2;
    }
  for (f = t->func_hook_middle; f != NULL; f = f->next)
    {
      f->cb_rcv_func (req);
      if (__IS_PSP_MANDATE (req->flag))
	goto mandate;
      if (__IS_PSP_STOP (req->flag))
	return -2;
    }
  for (f = t->func_hook_last; f != NULL; f = f->next)
    {
      f->cb_rcv_func (req);
      if (__IS_PSP_MANDATE (req->flag))
	goto mandate;
      if (__IS_PSP_STOP (req->flag))
	return -2;
    }
  for (f = t->func_hook_really_last; f != NULL; f = f->next)
    {
      f->cb_rcv_func (req);
      if (__IS_PSP_MANDATE (req->flag))
	goto mandate;
      if (__IS_PSP_STOP (req->flag))
	return -2;
    }
  return 0;

    
mandate:
  for (f = t->func_hook_final; f != NULL; f = f->next)
    {
      f->cb_rcv_func (req);
      if (__IS_PSP_MANDATE (req->flag))
	return 0;
      if (__IS_PSP_STOP (req->flag))
	return -2;
    }
  return 0;
}
