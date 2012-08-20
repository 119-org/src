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
psp_core_cb_ict_kill_transaction (int type, osip_transaction_t * tr)
{
  static int ict_killed = 0;

  ict_killed++;
  if (ict_killed % 100 == 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO1, NULL,
		   "Number of killed ICT transaction = %i\n", ict_killed));
    }
  osip_remove_transaction ((osip_t *) tr->config, tr);
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_ict_kill_transaction!\n"));
  module_wakeup (core->sfp->module);	/* this module needs to be waken up */
}

void
psp_core_cb_nict_kill_transaction (int type, osip_transaction_t * tr)
{
  static int nict_killed = 0;

  nict_killed++;
  if (nict_killed % 100 == 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO1, NULL,
		   "Number of killed NICT transaction = %i\n", nict_killed));
    }
  osip_remove_transaction ((osip_t *) tr->config, tr);
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_nict_kill_transaction!\n"));
  module_wakeup (core->sfp->module);	/* this module needs to be waken up */
}

void
psp_core_cb_transport_error (int type, osip_transaction_t * tr, int error)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_ERROR, NULL,
	       "psp_core_cb_network_error! \n"));
}

void
psp_core_cb_sndreq_retransmission (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_sndreq_retransmission! \n"));
}

void
psp_core_cb_rcvresp_retransmission (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcvresp_retransmission! \n"));
}

void
psp_core_cb_sndinvite (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_sndinvite!\n"));
}

void
psp_core_cb_sndack (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_sndack!\n"));
}

void
psp_core_cb_sndbye (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_sndbye!\n"));
}

void
psp_core_cb_sndcancel (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_sndcancel!\n"));
}

void
psp_core_cb_sndinfo (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_sndinfo!\n"));
}

void
psp_core_cb_sndoptions (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_sndoptions!\n"));
}

void
psp_core_cb_sndregister (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_sndregister!\n"));
}

void
psp_core_cb_sndprack (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_sndprack!\n"));
}

void
psp_core_cb_sndunkrequest (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_sndunkrequest!\n"));
}

void
psp_core_cb_rcv1xx (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcv1xx!\n"));
  if (!(MSG_TEST_CODE (sip, 100)))
    module_wakeup (core->sfp->module);	/* this module needs to be waken up */
}

void
psp_core_cb_rcv2xx (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcv2xx!\n"));
  /* this MUST be a response for the statefull module */
  module_wakeup (core->sfp->module);	/* this module needs to be waken up */
}

void
psp_core_cb_rcv3xx (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcv3xx!\n"));
  /* this MUST be a response for the statefull module */
  module_wakeup (core->sfp->module);	/* this module needs to be waken up */
}

void
psp_core_cb_rcv4xx (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcv4xx!\n"));
  module_wakeup (core->sfp->module);	/* this module needs to be waken up */
}

void
psp_core_cb_rcv5xx (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcv5xx!\n"));
  module_wakeup (core->sfp->module);	/* this module needs to be waken up */
}

void
psp_core_cb_rcv6xx (int type, osip_transaction_t * tr, osip_message_t * sip)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "psp_core_cb_rcv6xx!\n"));
  module_wakeup (core->sfp->module);	/* this module needs to be waken up */
}
