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

extern psp_core_t *core;


void
psp_core_cb_ist_kill_transaction (int type, osip_transaction_t * tr)
{
  static int ist_killed = 0;

  ist_killed++;
  if (ist_killed % 100 == 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO1, NULL,
		   "Number of killed IST transaction = %i\n", ist_killed));
    }

  osip_remove_transaction ((osip_t *) tr->config, tr);

  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_ist_kill_transaction!\n"));
}

void
psp_core_cb_nist_kill_transaction (int type, osip_transaction_t * tr)
{
  static int nist_killed = 0;

  nist_killed++;
  if (nist_killed % 100 == 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO1, NULL,
		   "Number of killed NIST transaction = %i\n", nist_killed));
    }
  osip_remove_transaction ((osip_t *) tr->config, tr);
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_nist_kill_transaction!\n"));
}

void
psp_core_cb_sndresp_retransmission (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_sndresp_retransmission!\n"));
}

void
psp_core_cb_rcvreq_retransmission (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcvreq_retransmission!\n"));
}

void
psp_core_cb_rcvinvite (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcvinvite!\n"));
  psp_core_event_add_sfp_inc_traffic (tr);
}

void
psp_core_cb_rcvack (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcvack!\n"));
}

void
psp_core_cb_rcvack2 (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcvack2!\n"));
}

void
psp_core_cb_rcvregister (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcvregister!\n"));
  psp_core_event_add_sfp_inc_traffic (tr);
}

void
psp_core_cb_rcvbye (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcvbye!\n"));
  psp_core_event_add_sfp_inc_traffic (tr);
}

void
psp_core_cb_rcvcancel (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcvcancel!\n"));
  psp_core_event_add_sfull_cancel (tr);
}

void
psp_core_cb_rcvinfo (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcvinfo!\n"));
  psp_core_event_add_sfp_inc_traffic (tr);
}

void
psp_core_cb_rcvoptions (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcvoptions!\n"));
  psp_core_event_add_sfp_inc_traffic (tr);
}

void
psp_core_cb_rcvprack (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcvprack!\n"));
}

void
psp_core_cb_rcvunkrequest (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcvunkrequest!\n"));
  psp_core_event_add_sfp_inc_traffic (tr);
}

void
psp_core_cb_snd1xx (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_snd1xx!\n"));
  if (MSG_IS_RESPONSE_FOR (sip, "INVITE"))
    {

    }
  else if (MSG_IS_RESPONSE_FOR (sip, "REGISTER"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "BYE"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "OPTIONS"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "INFO"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "CANCEL"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "NOTIFY"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "SUBSCRIBE"))
    {
    }
  else
    {
    }
}

void
psp_core_cb_snd2xx (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_snd2xx!\n"));
  if (MSG_IS_RESPONSE_FOR (sip, "INVITE"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "REGISTER"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "BYE"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "OPTIONS"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "INFO"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "CANCEL"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "NOTIFY"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "SUBSCRIBE"))
    {
    }
  else
    {
    }

}

void
psp_core_cb_snd3xx (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_snd3xx!\n"));
  if (MSG_IS_RESPONSE_FOR (sip, "INVITE"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "REGISTER"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "BYE"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "OPTIONS"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "INFO"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "CANCEL"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "NOTIFY"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "SUBSCRIBE"))
    {
    }
  else
    {
    }
}

void
psp_core_cb_snd4xx (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_snd4xx!\n"));
  if (MSG_IS_RESPONSE_FOR (sip, "INVITE"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "REGISTER"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "BYE"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "OPTIONS"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "INFO"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "CANCEL"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "NOTIFY"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "SUBSCRIBE"))
    {
    }
  else
    {
    }
}

void
psp_core_cb_snd5xx (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_snd5xx!\n"));
  if (MSG_IS_RESPONSE_FOR (sip, "INVITE"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "REGISTER"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "BYE"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "OPTIONS"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "INFO"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "CANCEL"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "NOTIFY"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "SUBSCRIBE"))
    {
    }
  else
    {
    }

}

void
psp_core_cb_snd6xx (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_snd6xx!\n"));
  if (MSG_IS_RESPONSE_FOR (sip, "INVITE"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "REGISTER"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "BYE"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "OPTIONS"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "INFO"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "CANCEL"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "NOTIFY"))
    {
    }
  else if (MSG_IS_RESPONSE_FOR (sip, "SUBSCRIBE"))
    {
    }
  else
    {
    }
}
