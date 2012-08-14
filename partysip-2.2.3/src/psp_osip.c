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

static void *
osip_timers_thread (psp_osip_t * psp_osip)
{

  fd_set osip_fdset;
  struct timeval tv;

  while (1)
    {

      int s;
      int i;

      osip_timers_gettimeout(psp_osip->osip, &tv);
      if (tv.tv_sec>15)
	{
	  /* tv.tv_sec = 15; */
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO3, NULL,
		       "eXosip: Timer is set to a very long value!\n"));
	}
      else
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO3, NULL,
		       "eXosip: timer sec:%i usec:%i!\n",
		       tv.tv_sec, tv.tv_usec));
	}

      FD_ZERO (&osip_fdset);
      s = ppl_pipe_get_read_descr (core->psp_osip->wakeup);
      if (s <= 1)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_BUG, NULL,
		       "No wakeup value in module osip!\n"));
	  return NULL;
	}
      FD_SET (s, &osip_fdset);

      i = select (s + 1, &osip_fdset, NULL, NULL, &tv);

      if (FD_ISSET (s, &osip_fdset))
	{
	  char tmp[51];

	  i = ppl_pipe_read (core->psp_osip->wakeup, tmp, 50);
	  if (i == 1 && tmp[0] == 'q')
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO2, NULL,
			   "osip module: Exiting!\n"));
	      return 0;
	    }
	  else
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO2, NULL,
			   "osip module: wake up!\n"));
	    }
	}

      psp_core_lock ();
      osip_timers_ict_execute (psp_osip->osip);
      osip_timers_ist_execute (psp_osip->osip);
      osip_timers_nict_execute (psp_osip->osip);
      osip_timers_nist_execute (psp_osip->osip);

      osip_ict_execute (psp_osip->osip);
      osip_nict_execute (psp_osip->osip);
      osip_ist_execute (psp_osip->osip);
      osip_nist_execute (psp_osip->osip);
      psp_core_unlock ();

      if (psp_osip->exit_flag == 1)
	return NULL;
    }
  return 0;
}

int
psp_osip_init (psp_osip_t ** psp_osip, int delay)
{
  osip_t *osip;
  int i;

  /* initialize osip */
  i = osip_init (&osip);
  if (i != 0)
    return -1;

  osip_set_cb_send_message (osip, &psp_core_cb_snd_message);

  osip_set_kill_transaction_callback (osip, OSIP_ICT_KILL_TRANSACTION, &psp_core_cb_ict_kill_transaction);
  osip_set_kill_transaction_callback (osip, OSIP_IST_KILL_TRANSACTION, &psp_core_cb_ist_kill_transaction);
  osip_set_kill_transaction_callback (osip, OSIP_NICT_KILL_TRANSACTION, &psp_core_cb_nict_kill_transaction);
  osip_set_kill_transaction_callback (osip, OSIP_NIST_KILL_TRANSACTION, &psp_core_cb_nist_kill_transaction);

  osip_set_message_callback (osip, OSIP_ICT_STATUS_2XX_RECEIVED_AGAIN, &psp_core_cb_rcvresp_retransmission);
  osip_set_message_callback (osip, OSIP_ICT_STATUS_3456XX_RECEIVED_AGAIN, &psp_core_cb_rcvresp_retransmission);
  osip_set_message_callback (osip, OSIP_ICT_INVITE_SENT_AGAIN, &psp_core_cb_sndreq_retransmission);
  osip_set_message_callback (osip, OSIP_IST_STATUS_2XX_SENT_AGAIN, &psp_core_cb_sndresp_retransmission);
  osip_set_message_callback (osip, OSIP_IST_STATUS_3456XX_SENT_AGAIN, &psp_core_cb_sndresp_retransmission);
  osip_set_message_callback (osip, OSIP_IST_INVITE_RECEIVED_AGAIN, &psp_core_cb_rcvreq_retransmission);
  osip_set_message_callback (osip, OSIP_NICT_STATUS_2XX_RECEIVED_AGAIN, &psp_core_cb_rcvresp_retransmission);
  osip_set_message_callback (osip, OSIP_NICT_STATUS_3456XX_RECEIVED_AGAIN,
				    &psp_core_cb_rcvresp_retransmission);
  osip_set_message_callback (osip, OSIP_NICT_REQUEST_SENT_AGAIN, &psp_core_cb_sndreq_retransmission);
  osip_set_message_callback (osip, OSIP_NIST_STATUS_2XX_SENT_AGAIN, &psp_core_cb_sndresp_retransmission);
  osip_set_message_callback (osip, OSIP_NIST_STATUS_3456XX_SENT_AGAIN, &psp_core_cb_sndresp_retransmission);
  osip_set_message_callback (osip, OSIP_NIST_REQUEST_RECEIVED_AGAIN,
				     &psp_core_cb_rcvreq_retransmission);

  /*  osip_set_cb_killtransaction(osip,&psp_core_cb_killtransaction);
     osip_set_cb_endoftransaction(osip,&psp_core_cb_endoftransaction); */

  osip_set_transport_error_callback (osip, OSIP_ICT_TRANSPORT_ERROR, &psp_core_cb_transport_error);
  osip_set_transport_error_callback (osip, OSIP_IST_TRANSPORT_ERROR, &psp_core_cb_transport_error);
  osip_set_transport_error_callback (osip, OSIP_NICT_TRANSPORT_ERROR, &psp_core_cb_transport_error);
  osip_set_transport_error_callback (osip, OSIP_NIST_TRANSPORT_ERROR, &psp_core_cb_transport_error);

  osip_set_message_callback (osip, OSIP_ICT_INVITE_SENT, &psp_core_cb_sndinvite);
  osip_set_message_callback (osip, OSIP_ICT_ACK_SENT, &psp_core_cb_sndack);
  osip_set_message_callback (osip, OSIP_NICT_REGISTER_SENT, &psp_core_cb_sndregister);
  osip_set_message_callback (osip, OSIP_NICT_BYE_SENT, &psp_core_cb_sndbye);
  osip_set_message_callback (osip, OSIP_NICT_CANCEL_SENT, &psp_core_cb_sndcancel);
  osip_set_message_callback (osip, OSIP_NICT_INFO_SENT, &psp_core_cb_sndinfo);
  osip_set_message_callback (osip, OSIP_NICT_OPTIONS_SENT, &psp_core_cb_sndoptions);
  osip_set_message_callback (osip, OSIP_NICT_SUBSCRIBE_SENT, &psp_core_cb_sndoptions);
  osip_set_message_callback (osip, OSIP_NICT_NOTIFY_SENT, &psp_core_cb_sndoptions);
  /*  osip_set_cb_nict_sndprack   (osip,&psp_core_cb_sndprack); */
  osip_set_message_callback (osip, OSIP_NICT_UNKNOWN_REQUEST_SENT, &psp_core_cb_sndunkrequest);

  osip_set_message_callback (osip, OSIP_ICT_STATUS_1XX_RECEIVED, &psp_core_cb_rcv1xx);
  osip_set_message_callback (osip, OSIP_ICT_STATUS_2XX_RECEIVED, &psp_core_cb_rcv2xx);
  osip_set_message_callback (osip, OSIP_ICT_STATUS_3XX_RECEIVED, &psp_core_cb_rcv3xx);
  osip_set_message_callback (osip, OSIP_ICT_STATUS_4XX_RECEIVED, &psp_core_cb_rcv4xx);
  osip_set_message_callback (osip, OSIP_ICT_STATUS_5XX_RECEIVED, &psp_core_cb_rcv5xx);
  osip_set_message_callback (osip, OSIP_ICT_STATUS_6XX_RECEIVED, &psp_core_cb_rcv6xx);

  osip_set_message_callback (osip, OSIP_IST_STATUS_1XX_SENT, &psp_core_cb_snd1xx);
  osip_set_message_callback (osip, OSIP_IST_STATUS_2XX_SENT, &psp_core_cb_snd2xx);
  osip_set_message_callback (osip, OSIP_IST_STATUS_3XX_SENT, &psp_core_cb_snd3xx);
  osip_set_message_callback (osip, OSIP_IST_STATUS_4XX_SENT, &psp_core_cb_snd4xx);
  osip_set_message_callback (osip, OSIP_IST_STATUS_5XX_SENT, &psp_core_cb_snd5xx);
  osip_set_message_callback (osip, OSIP_IST_STATUS_6XX_SENT, &psp_core_cb_snd6xx);

  osip_set_message_callback (osip, OSIP_NICT_STATUS_1XX_RECEIVED, &psp_core_cb_rcv1xx);
  osip_set_message_callback (osip, OSIP_NICT_STATUS_2XX_RECEIVED, &psp_core_cb_rcv2xx);
  osip_set_message_callback (osip, OSIP_NICT_STATUS_3XX_RECEIVED, &psp_core_cb_rcv3xx);
  osip_set_message_callback (osip, OSIP_NICT_STATUS_4XX_RECEIVED, &psp_core_cb_rcv4xx);
  osip_set_message_callback (osip, OSIP_NICT_STATUS_5XX_RECEIVED, &psp_core_cb_rcv5xx);
  osip_set_message_callback (osip, OSIP_NICT_STATUS_6XX_RECEIVED, &psp_core_cb_rcv6xx);

  osip_set_message_callback (osip, OSIP_NIST_STATUS_1XX_SENT, &psp_core_cb_snd1xx);
  osip_set_message_callback (osip, OSIP_NIST_STATUS_2XX_SENT, &psp_core_cb_snd2xx);
  osip_set_message_callback (osip, OSIP_NIST_STATUS_3XX_SENT, &psp_core_cb_snd3xx);
  osip_set_message_callback (osip, OSIP_NIST_STATUS_4XX_SENT, &psp_core_cb_snd4xx);
  osip_set_message_callback (osip, OSIP_NIST_STATUS_5XX_SENT, &psp_core_cb_snd5xx);
  osip_set_message_callback (osip, OSIP_NIST_STATUS_6XX_SENT, &psp_core_cb_snd6xx);

  osip_set_message_callback (osip, OSIP_IST_INVITE_RECEIVED, &psp_core_cb_rcvinvite);
  osip_set_message_callback (osip, OSIP_IST_ACK_RECEIVED, &psp_core_cb_rcvack);
  osip_set_message_callback (osip, OSIP_IST_ACK_RECEIVED_AGAIN, &psp_core_cb_rcvack2);
  osip_set_message_callback (osip, OSIP_NIST_REGISTER_RECEIVED, &psp_core_cb_rcvregister);
  osip_set_message_callback (osip, OSIP_NIST_BYE_RECEIVED, &psp_core_cb_rcvbye);
  osip_set_message_callback (osip, OSIP_NIST_CANCEL_RECEIVED, &psp_core_cb_rcvcancel);
  osip_set_message_callback (osip, OSIP_NIST_INFO_RECEIVED, &psp_core_cb_rcvinfo);
  osip_set_message_callback (osip, OSIP_NIST_OPTIONS_RECEIVED, &psp_core_cb_rcvoptions);
  osip_set_message_callback (osip, OSIP_NIST_SUBSCRIBE_RECEIVED, &psp_core_cb_rcvoptions);
  osip_set_message_callback (osip, OSIP_NIST_NOTIFY_RECEIVED, &psp_core_cb_rcvoptions);
  osip_set_message_callback (osip, OSIP_NIST_UNKNOWN_REQUEST_RECEIVED, &psp_core_cb_rcvunkrequest);

  (*psp_osip) = (psp_osip_t *) osip_malloc (sizeof (psp_osip_t));
  if ((*psp_osip) == NULL)
    goto poi_error0;

  (*psp_osip)->osip = osip;

  if (100 < delay < 500)
    (*psp_osip)->delay = delay;
  else
    delay = 500;

  (*psp_osip)->exit_flag = 0;	/* 0 not set, 1 set */
  (*psp_osip)->timers = NULL;

  (*psp_osip)->wakeup = ppl_pipe ();
  if ((*psp_osip)->wakeup == NULL)
    goto poi_error0;

  (*psp_osip)->timers =
    osip_thread_create (20000, (void *(*)(void *)) osip_timers_thread,
			(void *) (*psp_osip));

  if ((*psp_osip)->timers == NULL)
    goto poi_error1;

  return 0;

poi_error1:
  (*psp_osip)->exit_flag = 1;
  psp_osip_free (*psp_osip);
  return -1;
poi_error0:
  osip_release (osip);
  return -1;
}

int
psp_osip_wakeup (psp_osip_t * psp_osip)
{
  int i;
  char q[2] = "w";

  if (psp_osip == NULL)
    return -1;
  i = ppl_pipe_write (psp_osip->wakeup, &q, 1);
  if (i != 1)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "could not write in pipe!\n"));
      perror ("error while writing in pipe");
      return -1;
    }
  return 0;
}

static void
_osip_kill_transaction (osip_list_t * transactions)
{
  osip_transaction_t *transaction;

  if (!osip_list_eol (transactions, 0))
    {
      /* some transaction are still used by osip,
         transaction should be released by modules! */
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "module sfp: _osip_kill_transaction transaction should be released by modules!\n"));
    }

  while (!osip_list_eol (transactions, 0))
    {
      transaction = osip_list_get (transactions, 0);
      osip_transaction_free (transaction);
    }
}

void
psp_osip_free (psp_osip_t * psp_osip)
{
  char q[2] = "q";
  int i;

  if (psp_osip == NULL)
    return;

  psp_osip->exit_flag = 1;	/* this value is checked by the timer thread */
  if (psp_osip->timers != NULL)
    {
      i = ppl_pipe_write (psp_osip->wakeup, &q, 1);
      if (i != 1)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_BUG, NULL,
		       "could not write in pipe!\n"));
	  return;
	}
      i = osip_thread_join (psp_osip->timers);
      if (i != 0)
	printf ("ERROR: can't stop timer thread\n");
      osip_free (psp_osip->timers);
      psp_osip->timers = NULL;
    }

  /* now it's time to free all transactions... */
  _osip_kill_transaction (&psp_osip->osip->osip_ict_transactions);
  _osip_kill_transaction (&psp_osip->osip->osip_nict_transactions);
  _osip_kill_transaction (&psp_osip->osip->osip_ist_transactions);
  _osip_kill_transaction (&psp_osip->osip->osip_nist_transactions);

  i = ppl_pipe_close (psp_osip->wakeup);
  if (i == -1)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "could not close pipe!\n"));
    }
  psp_osip->wakeup = NULL;

  osip_release (psp_osip->osip);
  osip_free(psp_osip);
}


int
psp_osip_release (psp_osip_t * psp_osip)
{
  char q[2] = "q";
  int i;

  if (psp_osip == NULL)
    return -1;
  psp_osip->exit_flag = 1;
  if (psp_osip->timers != NULL)
    {
      i = ppl_pipe_write (psp_osip->wakeup, &q, 1);
      if (i != 1)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "could not write in pipe!\n"));
	  return -1;
	}
      i = osip_thread_join (psp_osip->timers);
      if (i != 0)
	{
	  psp_osip->timers = NULL;
	  printf ("ERROR: can't stop timer thread\n");
	  return -1;
	}
      else
	osip_free (psp_osip->timers);
    }
  psp_osip->timers = NULL;
  return 0;
}

int
psp_osip_set_delay (psp_osip_t * psp_osip, int delay)
{
  if (psp_osip == NULL)
    return -1;
  psp_osip->delay = delay;
  return 0;
}
