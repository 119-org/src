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
#include <ppl/ppl_dns.h>

int
pspm_resolv_init (pspm_resolv_t ** resolv)
{
  int i;

  *resolv = (pspm_resolv_t *) osip_malloc (sizeof (pspm_resolv_t));
  if (*resolv == NULL)
    return -1;

  i = module_init (&((*resolv)->module), MOD_THREAD, "resolv");
  if (i != 0)
    {
      osip_free (*resolv);
      *resolv = NULL;
      return -1;
    }
  return 0;
}

void
pspm_resolv_free (pspm_resolv_t * resolv)
{
  if (resolv == NULL)
    return;
  module_free (resolv->module);
  osip_free (resolv);
  return;
}


int
pspm_resolv_release (pspm_resolv_t * resolv)
{
  int i;

  if (resolv == NULL)
    return -1;
  i = module_release (resolv->module);
  if (i != 0)
    return -1;
  return 0;
}

int
pspm_resolv_start (pspm_resolv_t * resolv, void *(*func_start) (void *),
		   void *arg)
{
  int i;

  if (resolv == NULL)
    return -1;
  i = module_start (resolv->module, func_start, arg);
  if (i != 0)
    return -1;
  return 0;
}

int
pspm_resolv_execute (pspm_resolv_t * resolv, int sec_max, int usec_max,
		     int max)
{
  char *address;

  fd_set resolv_fdset;
  int max_fd;
  struct timeval tv;

  if (resolv == NULL)
    return -1;

  while (1)
    {
      int s;
      int i;

      tv.tv_sec = sec_max;
      tv.tv_usec = usec_max;

      max--;
      FD_ZERO (&resolv_fdset);
      s = ppl_pipe_get_read_descr (resolv->module->wakeup);
      if (s <= 1)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "No wakeup value in the resolver module!\n"));
	  return -1;
	}
      FD_SET (s, &resolv_fdset);

      max_fd = s;

      if ((sec_max == -1) || (usec_max == -1))
	i = select (max_fd + 1, &resolv_fdset, NULL, NULL, NULL);
      else
	i = select (max_fd + 1, &resolv_fdset, NULL, NULL, &tv);

      if (FD_ISSET (s, &resolv_fdset))
	{			/* time to wake up: we receive "q" to quit and any other char to read all socket */
	  char tmp[2];

	  i = ppl_pipe_read (resolv->module->wakeup, tmp, 1);
	  if (i == 1 && tmp[0] == 'q')
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO2, NULL,
			   "Exiting from the resolver module!\n"));
	      return 0;
	    }
	}
      address = ppl_dns_tryget_next_query ();
      while (address != NULL)
	{
	  pspm_resolv_handle_dns_query (resolv, address);
	  address = ppl_dns_tryget_next_query ();
	}

      if (max == 0)
	return 0;
    }
}


void
pspm_resolv_handle_dns_query (pspm_resolv_t * resolv, char *address)
{
  ppl_dns_entry_t *dns;
  int i;

#ifndef DO_NOT_USE_CACHE
  /* locate if we already have that value in cache */
  i = ppl_dns_get_result (&dns, address);
  if (i == 0)
    {
      osip_free (address);
      return;			/* we have already resolved that address */
    }
#endif

  ppl_dns_query (&dns, address, "udp");
  if (dns != NULL)		/* success! */
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "Successfully resolved the domain '%s' udp!\n", address));
      psp_core_dns_add_domain_result (dns);
      return;
    }

  /*   if it's not a domain name, it can be a FQDN */
  ppl_dns_query_host (&dns, address, 5060);
  if (dns != NULL)		/* success! */
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "INFO: Successfully resolved the hostname '%s' 5060!\n",
		   address));
      psp_core_dns_add_domain_result (dns);
      return;
    }
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_ERROR, NULL,
	       "hostname '%s' can't be resolved!\n", address));
  /* not found... We should add it in the "black list for a while" */
  psp_core_dns_add_domain_error (address);
}
