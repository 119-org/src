/*
  The udp plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The udp plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The udp plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <partysip/partysip.h>
#include <ppl/ppl_dns.h>
#include <ppl/ppl_socket.h>
#include "udp.h"

#include <fcntl.h>

/* to avoid warnings on WIN32 setsockopt calls */
#ifdef WIN32
#define OPTVALCAST (const char*)
#else
#define OPTVALCAST 
#endif

local_ctx_t *ctx = NULL;
static int ipv6_enable = 0;

int
local_ctx_init (int in_port, int out_port)
{
#ifdef MULTICAST_SUPPORT
  char *multicast;
#endif
  int option;
  int atry;
  int i;

  struct sockaddr_in6 raddr6;
  struct sockaddr_in raddr;

  ctx = (local_ctx_t *) osip_malloc (sizeof (local_ctx_t));
  if (ctx == NULL)
    return -1;

  ctx->mcast_socket = -1; /* if unused */
  ctx->in_port = in_port;

  /* not used by now */
  ctx->out_port = out_port;

  {
    char *atmp = psp_config_get_element ("ipv6_enable");
    if (atmp!=NULL && 0==osip_strncasecmp(atmp, "on", 2))
      ipv6_enable = 1;
  }
  if (ipv6_enable==1)
    ctx->in_socket = ppl_socket (PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
  else
    ctx->in_socket = ppl_socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (ctx->in_socket == -1)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "udp plugin: cannot create descriptor for port %i!\n",
			      ctx->in_port));
      goto lci_error1;
    }

  option = 1;
  i = setsockopt (ctx->in_socket, SOL_SOCKET, SO_REUSEADDR,
		  OPTVALCAST &option, sizeof option);
  if (i!=0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_WARNING, NULL,
		   "upd plugin; UDP listener SO_REUSE_ADDR failed %i (%i)!\n",
		   ctx->in_port, i));
    }

  if (out_port == in_port)
    ctx->out_socket = ctx->in_socket;
  else
    {
      if (ipv6_enable==1)
	ctx->out_socket = ppl_socket (PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
      else
	ctx->out_socket = ppl_socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);

      if (ctx->out_socket == -1)
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				  "udp plugin: cannot create descriptor for port %i!\n",
				  ctx->out_port));
	  goto lci_error2;
	}
      i = (-1);
      atry = 0;
      while (i < 0 && atry < 40)
	{
	  atry++;
	  if (ipv6_enable==1)
	    {
	      raddr6.sin6_addr = in6addr_any;
	      raddr6.sin6_port = htons ((short) ctx->out_port);
	      raddr6.sin6_family = AF_INET6;
	      i = ppl_socket_bind (ctx->out_socket,
				   (struct sockaddr *) &raddr6, sizeof (raddr6));
	    }
	  else
	    {
	      raddr.sin_addr.s_addr = htons (INADDR_ANY);
	      raddr.sin_port = htons ((short) ctx->out_port);
	      raddr.sin_family = AF_INET;
	      i = ppl_socket_bind (ctx->out_socket,
				   (struct sockaddr *) &raddr, sizeof (raddr));
	    }

	  if (i < 0)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_WARNING, NULL,
			   "udp plugin: cannot bind on port %i!\n",
			   ctx->out_port));
	      ctx->out_port++;
	    }
	}

      if (i != 0)
	goto lci_error3;
    }

  if (ipv6_enable==1)
    {
      raddr6.sin6_addr = in6addr_any;
      raddr6.sin6_port = htons ((short) ctx->in_port);
      raddr6.sin6_family = AF_INET6;
      i = ppl_socket_bind (ctx->in_socket,
			   (struct sockaddr *) &raddr6, sizeof (raddr6));
    }
  else
    {
      raddr.sin_addr.s_addr = htons (INADDR_ANY);
      raddr.sin_port = htons ((short) ctx->in_port);
      raddr.sin_family = AF_INET;
      i = ppl_socket_bind (ctx->in_socket,
			   (struct sockaddr *) &raddr, sizeof (raddr));
    }
  
  if (i < 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_WARNING, NULL,
		   "udp plugin: cannot bind on port %i!\n",
		   ctx->in_port));
      return -1;
    }

#ifdef MULTICAST_SUPPORT

  multicast = psp_config_get_element ("multicast");
  if (multicast == NULL)
    {
      ctx->mcast_socket = -1; /* disabled */
    }
  else if (0 == osip_strncasecmp (multicast, "on", 2))
    {
      struct ip_mreq mreq;
      struct ipv6_mreq mreq6;

      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "udp plugin: Multicast is enabled!\n"));

#ifdef __linux
      /* linux has support for listening on specific interface. */
      {
	char *if_mcast = psp_config_get_element ("if_mcast");
	char *if_ipmcast = psp_config_get_element ("if_ipmcast");
	if (if_mcast==NULL&&if_ipmcast==NULL)
	  {
	    if (ipv6_enable==1)
	      memcpy ((void *)mreq6.ipv6mr_multiaddr.s6_addr, (void const *)&in6_addr_any, 16);
	    else
	      mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	  }
	else if (if_mcast!=NULL)
	  {
	    char *tmp_name;
	    char *tmp_ip;
	    char *tmp_mask;
	    if (ipv6_enable==1)
	      {
		if (0 != ppl_dns_get_local_fqdn (&tmp_name, &tmp_ip, &tmp_mask,if_mcast, 0, AF_INET6))
		  {
		    OSIP_TRACE (osip_trace
				(__FILE__, __LINE__, OSIP_ERROR, NULL,
				 "udp plugin: error while looking for %s interface!\n",
				 if_mcast));
		    goto lci_error5;
		  }
	      }
	    else
	      {
		if (0 != ppl_dns_get_local_fqdn (&tmp_name, &tmp_ip, &tmp_mask,if_mcast, 0, AF_INET))
		  {
		    OSIP_TRACE (osip_trace
				(__FILE__, __LINE__, OSIP_ERROR, NULL,
				 "udp plugin: error while looking for %s interface!\n",
				 if_mcast));
		    goto lci_error5;
		  }
	      }
	      
	    if (ipv6_enable==1)
	      ppl_inet_pton ((const char *)tmp_ip, (void *)&mreq6.ipv6mr_multiaddr.s6_addr);
	    else
	      mreq.imr_interface.s_addr = inet_addr(tmp_ip);

	    OSIP_TRACE (osip_trace
			(__FILE__, __LINE__, OSIP_INFO1, NULL,
			 "udp plugin: Enabling multicast on %s interface!\n",
			 if_mcast));
	    OSIP_TRACE (osip_trace
			(__FILE__, __LINE__, OSIP_INFO1, NULL,
			 "udp plugin: %s %s %s!\n",
			 tmp_name, tmp_ip, tmp_mask));
	    
	    osip_free(tmp_name);
	    osip_free(tmp_ip);
	    osip_free(tmp_mask);
	  }
	else if (if_ipmcast!=NULL)
	  {
	    if (ipv6_enable==1)
	      ppl_inet_pton ((const char *)if_ipmcast, (void *)&mreq6.ipv6mr_multiaddr.s6_addr);
	    else
	      mreq.imr_interface.s_addr = inet_addr(if_ipmcast);
	  }
      }
#else /* non __linux */
      {
	char *if_ipmcast = psp_config_get_element ("if_ipmcast");
	if (if_ipmcast==NULL)
	  {
	    if (ipv6_enable==1)
	      memcpy ((void *)mreq6.ipv6mr_multiaddr.s6_addr, (void const *)&in6_addr_any, 16);
	    else
	      mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	  }
	else
	  {
	    if (ipv6_enable==1)
	      ppl_inet_pton ((const char *)if_ipmcast, (void *)&mreq6.ipv6mr_multiaddr.s6_addr);
	    else
	      mreq.imr_interface.s_addr = inet_addr(if_ipmcast);
	  }
      }
#endif /* __linux */

      if (ipv6_enable==0)
	{
	  /* For IPv4, sip.mcast.org is the well-known IPv4 multicast IP address*/
	  /* 224.0.1.75 is the multicast IP address of sip.mcast.org */
	  mreq.imr_multiaddr.s_addr = inet_addr("224.0.1.75");
      
	  if (ctx->in_port==5060)
	    ctx->mcast_socket = ctx->in_socket; /* reuse the same socket */
	  else
	    { /* open a new socket on 5060 for multicast */
	      raddr.sin_addr.s_addr = htons (INADDR_ANY);
	      raddr.sin_port = htons ((short) 5060); /* always 5060 */
	      raddr.sin_family = AF_INET;
	      
	      ctx->mcast_socket = ppl_socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	      if (ctx->mcast_socket<0)
		{
		  OSIP_TRACE (osip_trace
			      (__FILE__, __LINE__, OSIP_WARNING, NULL,
			       "upd plugin; Cannot create socket for multicast!\n"));
		  ctx->mcast_socket = -1;
		  goto skip_multicast;
		}
	      
	      option = 1;
	      i = setsockopt (ctx->mcast_socket, SOL_SOCKET, SO_REUSEADDR,
			      OPTVALCAST &option, sizeof option);
	      if (i!=0)
		{
		  OSIP_TRACE (osip_trace
			      (__FILE__, __LINE__, OSIP_WARNING, NULL,
			       "upd plugin; UDP listener SO_REUSE_ADDR failed %i (%i)!\n",
			       5060, i));
		  ppl_socket_close(ctx->mcast_socket);
		  ctx->mcast_socket = -1;
		  goto skip_multicast;
		}
	      
	      i = ppl_socket_bind (ctx->mcast_socket,
				   (struct sockaddr *) &raddr, sizeof (raddr));
	      if (i < 0)
		{
		  OSIP_TRACE (osip_trace
			      (__FILE__, __LINE__, OSIP_WARNING, NULL,
			       "udp plugin: disabling multicast support %i (%s)!\n",
			       5060, strerror (errno)));
		  ppl_socket_close(ctx->mcast_socket);
		  ctx->mcast_socket = -1;
		  goto skip_multicast;
		}
	    }
	  
	  option=0;
	  if(0 != setsockopt(ctx->mcast_socket, IPPROTO_IP, IP_MULTICAST_LOOP,
			     OPTVALCAST &option, sizeof(option)))
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_WARNING, NULL,
			   "udp plugin; cannot disable loopback for multicast socket. (%i)!\n",
			   5060));
	      if (ctx->in_port!=5060)
		ppl_socket_close(ctx->mcast_socket);
	      ctx->mcast_socket = -1;
	      goto skip_multicast;
	    }
	  
	  /* Enable Multicast support */
	  option=1;
	  if (0 != setsockopt(ctx->mcast_socket, IPPROTO_IP, IP_MULTICAST_TTL,
			      OPTVALCAST &option, sizeof(option)))
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_WARNING, NULL,
			   "udp plugin: cannot set multicast ttl value. (%i)!\n",
			   5060));
	      if (ctx->in_port!=5060)
		ppl_socket_close(ctx->mcast_socket);
	      ctx->mcast_socket = -1;
	      goto skip_multicast;
	    }
	  
	  if (0 != setsockopt (ctx->mcast_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			       OPTVALCAST &mreq, sizeof(mreq)))
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_WARNING, NULL,
			   "udp plugin; cannot set multicast ttl value. (%i)!\n",
			   5060));
	      if (ctx->in_port!=5060)
		ppl_socket_close(ctx->mcast_socket);
	      ctx->mcast_socket = -1;
	      goto skip_multicast;
	    }
	}
    skip_multicast:
    }

#endif /* !MULTICAST_SUPPORT */

  /* set the socket to never block on recv() calls */
#ifndef WIN32
  if (0 != fcntl (ctx->in_socket, F_SETFL, O_NONBLOCK))
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "udp plugin; cannot set O_NONBLOCK to the file desciptor (%i)!\n",
		   ctx->in_port));
      goto lci_error5;
    }
  if (0 != fcntl (ctx->out_socket, F_SETFL, O_NONBLOCK))
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "udp plugin; cannot set O_NONBLOCK to the file desciptor (%i)!\n",
		   ctx->out_port));
      goto lci_error5;
    }

#else
  {
    int timeout = 0;
    int err;

    err = setsockopt (ctx->in_socket,
		      SOL_SOCKET,
		      SO_RCVTIMEO, OPTVALCAST &timeout, sizeof (timeout));
    if (err != NO_ERROR)
      {
	/* failed for some reason... */
	OSIP_TRACE (osip_trace
		    (__FILE__, __LINE__, OSIP_ERROR, NULL,
		     "udp plugin; cannot set O_NONBLOCK to the file desciptor (%i)!\n",
		     ctx->in_port));
	goto lci_error5;
      }
    err = setsockopt (ctx->out_socket,
		      SOL_SOCKET,
		      SO_RCVTIMEO, OPTVALCAST &timeout, sizeof (timeout));
    if (err != NO_ERROR)
      {
	/* failed for some reason... */
	OSIP_TRACE (osip_trace
		    (__FILE__, __LINE__, OSIP_ERROR, NULL,
		     "udp plugin; cannot set O_NONBLOCK to the file desciptor (%i)!\n",
		     ctx->out_port));
	goto lci_error5;
      }
  }
#endif

  return 0;

lci_error5:
lci_error3:
  ppl_socket_close (ctx->out_socket);
lci_error2:
  ppl_socket_close (ctx->in_socket);
lci_error1:
  osip_free (ctx);
  ctx = NULL;
  return -1;
}

void
local_ctx_free ()
{
  if (ctx == NULL)
    return;
  if (ctx->in_socket != -1)
    {
      ppl_socket_close (ctx->in_socket);
      ctx->in_socket = -1;
    }
  if (ctx->in_port == ctx->out_port)
    ctx->out_socket = (-1);
  else if (ctx->out_socket != -1)
    ppl_socket_close (ctx->out_socket);
  osip_free (ctx);
  ctx = NULL;
}


#ifdef MULTICAST_SUPPORT
#ifndef IN_MULTICAST
#   define IN_MULTICAST(a)         IN_CLASSD(a)
#endif
#endif

/* max_analysed = maximum number of message when this method can parse 
  messages whithout returning.

   This method returns:
   -1 on error
   0  on no message available
   1  on max_analysed reached
*/
int
cb_rcv_udp_message (int max)
{
  fd_set memo_fdset;
  fd_set osip_fdset;
  int max_fd;
  struct sockaddr_in6 sa6;
  struct sockaddr_in sa4;
  struct sockaddr *sa;
  char *buf;
  int i;
  struct timeval tv;

#ifdef __linux
  socklen_t slen;
#else
  int slen;
#endif

  if (ctx == NULL)
    return -1;

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  FD_ZERO (&memo_fdset);
  FD_SET (ctx->in_socket, &memo_fdset);
  if (ctx->out_socket>0
      &&ctx->out_socket!=ctx->in_socket)
    FD_SET (ctx->out_socket, &memo_fdset);
  if (ctx->mcast_socket>0
      &&ctx->mcast_socket!=ctx->in_socket)
    FD_SET (ctx->mcast_socket, &memo_fdset);

  /* create a set of file descritor to control the stack */

  for (; max != 0; max--)
    {
      osip_fdset = memo_fdset;

      max_fd = ctx->in_socket;
      if (max_fd < ctx->out_socket)
	max_fd = ctx->out_socket;
      if (max_fd < ctx->mcast_socket)
	max_fd = ctx->mcast_socket;

      i = select (max_fd + 1, &osip_fdset, NULL, NULL, NULL);
      buf = (char *) osip_malloc (SIP_MESSAGE_MAX_LENGTH * sizeof (char) + 3);

      if (ipv6_enable==1)
	{
	  slen = sizeof (sa6);
	  sa = (struct sockaddr *)&sa6;
	}
      else
	{
	  slen = sizeof (sa4);
	  sa = (struct sockaddr *)&sa4;
	}

      if (0 == i)
	{
	  osip_free (buf);
	  return -1;		/* no message: timout expires */
	}
      else if (-1 == i)
	{
	  osip_free (buf);
	  return -2;		/* error */
	}
      else if (FD_ISSET (ctx->in_socket, &osip_fdset))
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO2, NULL,
		       "UDP MESSAGE RECEIVED\n"));
	  i = ppl_socket_recv (ctx->in_socket, buf, SIP_MESSAGE_MAX_LENGTH, 0,
			       sa, &slen);
	}
      else if (ctx->out_socket!=ctx->in_socket
	       &&FD_ISSET (ctx->out_socket, &osip_fdset))
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO2, NULL,
		       "UDP MESSAGE RECEIVED\n"));
	  i = ppl_socket_recv (ctx->out_socket, buf, SIP_MESSAGE_MAX_LENGTH, 0,
			       sa, &slen);
	}
      else if (ctx->mcast_socket>0
	       &&FD_ISSET (ctx->mcast_socket, &osip_fdset))
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO2, NULL,
		       "UDP MULTICAST MESSAGE RECEIVED\n"));
	  i =
	    ppl_socket_recv (ctx->mcast_socket, buf, SIP_MESSAGE_MAX_LENGTH, 0,
			     sa, &slen);
	}

      if (i > 0)
	{
	  char *ip_address = ppl_inet_ntop (sa);
	  /* Message might not end with a "\0" but we know the number of */
	  /* char received! */
	  osip_strncpy (buf + i, "\0", 1);
	  if (ip_address==NULL)
	    {
	      osip_free(buf);
	      return -1; /* missing information from socket?? */
	    }
	  if (ipv6_enable==1)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO2, NULL,
			   "udp plugin: RCV UDP MESSAGE (from %s:%i)\n",
			   ip_address, ntohs (sa6.sin6_port)));
	    }
	  else
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO2, NULL,
			   "udp plugin: RCV UDP MESSAGE (from %s:%i)\n",
			   /* inet_ntoa (sa.sin_addr), ntohs (sa.sin_port))); */
			   ip_address, ntohs (sa4.sin_port)));
	    }
	  if (ctx->mcast_socket>0)
	    {
	      /* check maddr parameter to detect multicast data */
	      /* do they need special processing? */
	    }
#ifndef HIDE_MESSAGE
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO1, NULL, "\n%s\n", buf));
#endif
	  if (ipv6_enable==1)
	    udp_process_message (buf, ip_address,
				 ntohs (sa6.sin6_port), i);
	  else
	    udp_process_message (buf, ip_address,
				 /* inet_ntoa (sa.sin_addr), */
				 ntohs (sa4.sin_port), i);
	  osip_free(ip_address);
	}
      else if (i == -1)
	{
	  if (errno == EAGAIN)
	    {
	      osip_free (buf);
	      return 0;
	    }
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "udp plugin: error while receiving data!\n"));
	  osip_free (buf);
	  return -1;
	}
    }
  /* max is reached */
  return 1;
}

static int
__osip_message_fix_last_via_header (osip_message_t * request, char *ip_addr, int port)
{
  osip_generic_param_t *rport;
  osip_via_t *via;
  /* get Top most Via header: */
  if (request == NULL || request == NULL)
    return -1;
  if (MSG_IS_RESPONSE (request))
    return 0;			/* Don't fix Via header */

  via = osip_list_get (&request->vias, 0);
  if (via == NULL || via->host == NULL)
    /* Hey, we could build it? */
    return -1;

  osip_via_param_get_byname (via, "rport", &rport);

  /* detect rport */
  if (rport != NULL)
    {
      if (rport->gvalue == NULL)
	{
	  rport->gvalue = (char *) osip_malloc (9);
#ifdef WIN32
	  _snprintf (rport->gvalue, 8, "%i", port);
#else
	  snprintf (rport->gvalue, 8, "%i", port);
#endif
	}			/* else bug? */
    }

  /* only add the received parameter if the 'sent-by' value does not contains
     this ip address */
  if (0 == osip_strcasecmp (via->host, ip_addr))	/* don't need the received parameter */
    return 0;
  else
    osip_via_set_received (via, osip_strdup (ip_addr));

  return 0;
}

int
udp_process_message (char *buf, char *ip_addr, int port, int length)
{
  osip_event_t *evt;

  if (buf == NULL
      || *buf == '\0'
      || buf[1] == '\0'
      || buf[2] == '\0' || buf[3] == '\0' || buf[4] == '\0' || buf[5] == '\0')
    {
      osip_free (buf);
      return -1;
    }

  evt = osip_parse (buf, length);
  osip_free (buf);
  if (evt == NULL)
    /* discard... */
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "udp module: Could not parse response!\n"));
      return -1;
    }
  if (evt->sip == NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "udp module: Could not parse response!\n"));
      osip_event_free (evt);
      return -1;
    }
  udp_log_event (evt);

  /* modify the request to add a "received" parameter in the last Via. */
  /* for libosip>0.9.2 use osip_message_fix_last_via_header from osip */
  __osip_message_fix_last_via_header (evt->sip, ip_addr, port);

  if (evt->sip == NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "udp module: Probably a bad via header!\n"));
      osip_event_free (evt);
      return -1;
    }
  /* modify the request so it's compliant with the latest draft */
  psp_core_fix_strict_router_issue (evt);

  psp_core_event_add_sip_message (evt);
  return 0;
}


int
udp_log_event (osip_event_t * evt)
{
#ifdef SHOW_LIMITED_MESSAGE
  osip_via_t *via;
  osip_generic_param_t *b;

  via = osip_list_get (evt->sip->vias, 0);
  osip_via_param_get_byname (via, "branch", &b);
  if (b == NULL)
    {
      if (MSG_IS_REQUEST (evt->sip))
	{
	}
      else
	{
	}
      return -1;
    }
  if (MSG_IS_REQUEST (evt->sip))
    {
    }
  else
    {
    }
#endif
  return 0;
}

static int udp_plugin_get_new_socket(struct sockaddr_storage *addr)
{
  struct sockaddr_in6 raddr6;
  struct sockaddr_in raddr;
  int option;
  int i;

  int socket;

  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_WARNING, NULL,
	       "udp plugin: Building a new connect socket %i!\n",
	       ctx->in_port));
		
  if (ipv6_enable==1)
    socket = ppl_socket (PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
  else
    socket = ppl_socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (socket == -1)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "udp plugin: cannot create descriptor for port %i!\n"));
      return -1;
    }

  option = 1;
  i = setsockopt (socket, SOL_SOCKET, SO_REUSEADDR,
		  OPTVALCAST &option, sizeof option);
  if (i!=0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_WARNING, NULL,
		   "upd plugin; UDP listener SO_REUSE_ADDR failed %i (%i)!\n",
		   ctx->in_port, i));
    }

  if (ipv6_enable==1)
    {
      memcpy ((void *)&(raddr6.sin6_addr.s6_addr), (const void *)&in6addr_any, 16);
      raddr6.sin6_port = htons (0);
      raddr6.sin6_family = AF_INET6;
      i = ppl_socket_bind (socket,
			   (struct sockaddr *) &raddr6, sizeof (raddr6));
    }
  else
    {
      raddr.sin_addr.s_addr = htons (INADDR_ANY);
      raddr.sin_port = htons (0);
      raddr.sin_family = AF_INET;
      i = ppl_socket_bind (socket,
			   (struct sockaddr *) &raddr, sizeof (raddr));
    }
  if (i < 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_WARNING, NULL,
		   "udp plugin: cannot bind on port %i!\n",
		   ctx->in_port));
      ppl_socket_close(socket);
      return -1;
    }

  i = connect(socket,(struct sockaddr *) addr,sizeof(struct sockaddr));
  if (i < 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_WARNING, NULL,
		   "udp plugin: cannot connect socket %i!\n",
		   ctx->in_port));
      ppl_socket_close(socket);
      return -1;
    }

  return socket;
}

/* return
   -1 on error
   0  on success
*/
int
cb_snd_udp_message (osip_transaction_t * transaction,	/* read only element */
		    osip_message_t * message,	/* message to send           */
		    char *host,	/* proposed destination host */
		    int port,	/* proposed destination port */
		    int socket)	/* proposed socket (if any)  */
{
  size_t length;
  int i;
  char *buf;
  int sock;
  struct addrinfo *addrinfo;
  struct sockaddr_storage addr;

  if (ctx == NULL)
    return -1;

  i = osip_message_to_str (message, &buf, &length);

  if (i != 0)
    return -1;
#ifndef HIDE_MESSAGE
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL, "\n%s\n", buf));
#endif
  /* For RESPONSE, oSIP ALWAYS provide host and port from the top via */
  /* For REQUEST,  oSIP SOMETIMES provide host and port to use which
     may be different from the request uri */

  if (transaction==NULL && MSG_IS_REQUEST(message))
    {
      /* recalculate destination */
      osip_route_t *route;
      host = NULL;
      port=5060;
      osip_message_get_route (message, 0, &route);
      if (route != NULL)
	{
	  int port = 5060;
	  
	  if (route->url->port != NULL)
	    port = osip_atoi (route->url->port);
	  host = route->url->host;
	}
    }

  if (host == NULL)
    {				/* when host is NULL, we use the request uri value */
      host = message->req_uri->host;
      if (message->req_uri->port != NULL)
	port = osip_atoi (message->req_uri->port);
      else
	port = 5060;
    }

  i = ppl_dns_get_addrinfo(&addrinfo, host, port);
  if (i!=PPL_SUCCESS)
    {
      osip_free (buf);
      return -1;
    }
#if 0
  memcpy (&addr, addrinfo->ai_addr, addrinfo->ai_addrlen);
  freeaddrinfo (addrinfo);
#endif

  /* #define SUPPORT_ICMP */
#ifdef SUPPORT_ICMP
  sock = 0; /* use the default */
  if (transaction!=NULL)
  {
    if (MSG_IS_ACK(message))
      {
	if (transaction->out_socket!=0 && transaction->out_socket!=-1)
	  {
	    ppl_socket_close(transaction->out_socket);
	    transaction->out_socket = 0;
	  }
      }
    if (MSG_IS_REQUEST(message))
      {
	if (transaction->out_socket==0 || transaction->out_socket==-1)
	  transaction->out_socket = udp_plugin_get_new_socket(&addr);
	if (transaction->out_socket==0 || transaction->out_socket==-1) /* use default for ACK */
	  transaction->out_socket = 0;
      }
  }

  /* connect(sock,(struct sockaddr *) &addr,sizeof(addr)); */
#endif

#ifdef SUPPORT_ICMP
  if (MSG_IS_RESPONSE(message)
      || MSG_IS_ACK(message)
      || transaction->out_socket==0)
    i = sendto (ctx->out_socket, (const void *) buf, length, 0,
		(struct sockaddr *) &addr, sizeof (addr));
  else
    i = send (transaction->out_socket, (const void *) buf, length, 0);
  
#else
  sock = ctx->out_socket;
  i = sendto (sock, (const void *) buf, length, 0,
	      addrinfo->ai_addr, addrinfo->ai_addrlen);
  /*(struct sockaddr *) &addr, sizeof (addr)); */
  freeaddrinfo (addrinfo);
#endif
  if (0 > i)
    {
      osip_free (buf);
#ifdef WIN32
      i = WSAGetLastError ();
      if (WSAECONNREFUSED == i)
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				  "SIP_ECONNREFUSED - No remote server.\n"));
	  return 1;		/* I prefer to answer 1 by now..
				   we'll see later what's better */
	}
#else
      i = errno;
      if (ECONNREFUSED == i)
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				  "SIP_ECONNREFUSED - No remote server.\n"));
	  return 1;		/* I prefer to answer 1 by now..
				   we'll see later what's better */
	}
#endif
#ifdef WIN32
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "SIP_NETWORK_ERROR - Network error %i sending message to %s on port %i.\n",
			      i, host, port));
#else
      /* XXX use some configure magic to detect if we have the right strerror_r()
       * function (the one that returns int, not char *) and then use strerror_r()
       * instead.
       */
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "SIP_NETWORK_ERROR - Network error %i (%s) sending message to %s on port %i.\n",
			      i, strerror (i), host, port));
#endif
      /* SIP_NETWORK_ERROR; */
      return -1;
    }

#ifdef WIN32
  i = WSAGetLastError ();
  if (WSAECONNREFUSED == i)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "SIP_ECONNREFUSED - No remote server.\n"));
      return 1;
    }
  if (WSAECONNRESET == i)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "SIP_ECONNREFUSED - No remote server.\n"));
      return 1;
    }
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			  "WSAGetLastError returned : %i.\n", i));
#endif

  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "udp_plugin: message sent to %s on port %i\n", host, port));

  osip_free (buf);
  return 0;
}
