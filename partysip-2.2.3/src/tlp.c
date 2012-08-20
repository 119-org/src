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

#ifdef SCTP_SUPPORT
#ifdef _SCTPLIB
#include <ext_socket.h>
#else
#include <netinet/sctp.h>
/* #include <sctp_lib.h> */	/* for bindx */
#endif
#endif

int
pspm_tlp_init (pspm_tlp_t ** pspm)
{
  int i;

  (*pspm) = (pspm_tlp_t *) osip_malloc (sizeof (pspm_tlp_t));
  if (*pspm == NULL)
    return -1;
  (*pspm)->tlp_plugins = NULL;
  i = module_init (&((*pspm)->module), MOD_THREAD, "tlp");
  if (i != 0)
    goto mti_error1;

  return 0;

mti_error1:
  osip_free (*pspm);
  return -1;
}

void
pspm_tlp_free (pspm_tlp_t * m)
{
  tlp_plugin_t *p;

  module_free (m->module);

  for (p = m->tlp_plugins; p != NULL; p = m->tlp_plugins)
    {
      m->tlp_plugins = p->next;
      tlp_plugin_free (p);
    }
  osip_free (m);
}

int
pspm_tlp_release (pspm_tlp_t * pspm)
{
  int i;

  i = module_release (pspm->module);
  if (i != 0)
    return -1;
  return 0;
}

int
pspm_tlp_start (pspm_tlp_t * pspm, void *(*func_start) (void *), void *arg)
{
  int i;

  i = module_start (pspm->module, func_start, arg);
  if (i != 0)
    return -1;
  return 0;
}

int
pspm_tlp_execute (pspm_tlp_t * pspm, int sec_max, int usec_max,
		  int max_analysed)
{
  fd_set tlp_fdset;
  int max_fd;
  struct timeval tv;

  while (1)
    {
      int i;
      int s;
      tlp_plugin_t *plug;

      tv.tv_sec = sec_max;
      tv.tv_usec = usec_max;

      max_analysed--;
      FD_ZERO (&tlp_fdset);
      s = ppl_pipe_get_read_descr (pspm->module->wakeup);
      if (s <= 1)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "No wakeup value in module tlp!\n"));
	  return -1;
	}
      FD_SET (s, &tlp_fdset);

      max_fd = s;
      /* add all descriptor in fdset, so any incoming message will wake up this thread */
      for (plug = pspm->tlp_plugins; plug != NULL; plug = plug->next)
	{
	  if (plug->in_socket > 0)
	    {
	      if (max_fd < plug->in_socket)
		max_fd = plug->in_socket;
	      FD_SET (plug->in_socket, &tlp_fdset);
	      if (plug->in_socket!=plug->out_socket
		  && plug->out_socket>0)
		{
		  FD_SET (plug->out_socket, &tlp_fdset);
		  if (max_fd<plug->out_socket)
		    max_fd = plug->out_socket;
		}
	      if (plug->in_socket!=plug->mcast_socket
		  && plug->mcast_socket>0)
		{
		  FD_SET (plug->mcast_socket, &tlp_fdset);
		  if (max_fd<plug->mcast_socket)
		    max_fd = plug->mcast_socket;
		}
	    }
	}

      if ((sec_max == -1) || (usec_max == -1))
#ifdef _SCTPLIB
        i = ext_select (max_fd + 1, &tlp_fdset, NULL, NULL, NULL);
#else
	i = select (max_fd + 1, &tlp_fdset, NULL, NULL, NULL);
#endif
      else
#ifdef _SCTPLIB
        i = ext_select (max_fd + 1, &tlp_fdset, NULL, NULL, &tv);
#else
	i = select (max_fd + 1, &tlp_fdset, NULL, NULL, &tv);
#endif
      if (FD_ISSET (s, &tlp_fdset))
	{			/* time to wake up: we receive "q" to quit and any other char to read all socket */
	  char tmp[2];

	  i = ppl_pipe_read (pspm->module->wakeup, tmp, 1);
	  if (i == 1 && tmp[0] == 'q')
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO2, NULL,
			   "Exiting tlp module!\n"));
	      return 0;
	    }
	}

      for (plug = pspm->tlp_plugins; plug != NULL; plug = plug->next)
	{
	  if (plug->in_socket > 0)
	    {
	      if (FD_ISSET (plug->in_socket, &tlp_fdset))
		{
		  plug->rcv_func->cb_rcv_func (1);
		}
	      else if (plug->out_socket>0
		       && FD_ISSET (plug->out_socket, &tlp_fdset))
		{
		  plug->rcv_func->cb_rcv_func (1);
		}
	      else if (plug->mcast_socket>0
		       && FD_ISSET (plug->mcast_socket, &tlp_fdset))
		{
		  plug->rcv_func->cb_rcv_func (1);
		}
	    }
	  else
	    {
	      plug->rcv_func->cb_rcv_func (5);
	    }
	}

      if (max_analysed == 0)
	return 0;
    }
}

int
pspm_tlp_send_message (pspm_tlp_t * pspm, osip_transaction_t * tr,
		       osip_message_t * sip, char *host, int port, int out_socket)
{
  int i;
  tlp_plugin_t *plug;

  if (pspm->module == NULL || pspm->module->wakeup == NULL)
    return -1;
  for (plug = pspm->tlp_plugins; plug != NULL; plug = plug->next)
    {
      i = plug->snd_func->cb_snd_func (tr, sip, host, port, out_socket);
      /* if i==-2 wrong plugin -> we continue... */
      if (i == -1)
	return -1;
      if (i == 0)
	return 0;
    }
  return -1;			/* no plugin for this message... */
}
