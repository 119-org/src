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

extern psp_core_t *core;


/* FIREWALL issue:

ISSUE1: incompatibility with forking capability. (even if not done by partysip)

   As stated in the rfc3261, one INVITE transaction may establish to
   several sessions. (some proxy may fork and several 200ok can be
   received.

   Partysip has no means to be aware of the number of call established
   when a transaction has been accepted. This is not an issue for incoming
   calls as most of the time, partysip won't fork requests. But for
   outgoing calls, a request may be forked more often.

   Here is the situation of an outgoing call where the REQUEST has
   forked and 2 sessions are established.

   ua1 -> INVITE1 -> partysip -> INVITE2 -> ua2
   .                 partysip -> INVITE2 -> ua3
   ua1 <- 200 OK  <- partysip <- 200 OK  <- ua2
   ua1 <- 200 OK  <- partysip <- 200 OK  <- ua3

   In this scenario, the proxy has modified the INVITE1'sdp so that
   the ua2 and ua3 will send media to partysip's port. (Those media
   will be forwarded thanks to the new 'iptables rule' added to the
   firewall. The firewall will forward both RTP stream coming to the
   same port.

   After receiving 2 times a 200 ok, a UA should close at least one
   dialog with a BYE. This BYE will contains the callid information
   that were located in the initial INVITE and partysip will remove
   the rule previously added for this callid. Once the rule is
   deleted, the call that remains active will not receive media
   any more.

   What can we do to make it work without adding complexity in
   partysip.

   ISSUE2: retransmissions of LATE answers.

   If a 200 ok (or any response) is modified, then all retransmissions
   MUST be also modified. For 2OO ok to INVITE, from the second
   retransmissions, the message is received but won't match a
   transaction in osip. -this is because UA does not handle 200 Ok
   for INVITE like proxy.-. Forwarding a late answer is easy without
   maintaining any state in partysip. With this new feature, a state
   have to be maintained.
   
   
*/

int
firewall_entry_remove_rule (char *uniquehash)
{
  static char *iptable_server = IPTABLE_SERVER_IP;
  static struct sockaddr_in6 raddr6;
  static struct sockaddr_in raddr;
  static int cli_socket = 0;
  char *host;
  int iptable_srv_port = IPTABLE_SERVER_PORT;
  int i;
  int index;

  if (core->iptables_dynamic_natrule == 0)
    {
      /* Dynamic insertion of rules is diabled */
      /* but masquerading_sdp is on */
      return 0;
    }

  for (index=0;index<PSP_NAT_MAX_CALL;index++)
    {
      if (*(core->fw_entries[index].hash) == '\0')
	{}
      else if (0==strcmp(core->fw_entries[index].hash, uniquehash))
	break;
    }

  if (*(core->fw_entries[index].hash) == '\0'
      || 0!=strcmp(core->fw_entries[index].hash, uniquehash))
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "firewall module: No rule to delete found for callid %s\n",
		   uniquehash));
      return 0; /* continue anyway... */
    }

  if (cli_socket == 0)		/* only once */
    {
      if (core->ipt_port == 0)	/* port is unspecified, use default */
	iptable_srv_port = IPTABLE_SERVER_PORT;
      else
	iptable_srv_port = core->ipt_port;
      if (core->ipt_server == NULL)
	host = iptable_server;	/* IPTABLE_SERVER_IP; */
      else
	host = core->ipt_server;

      /* the arguments line will be received through a socket */
      cli_socket = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (cli_socket < 0)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "firewall module: unable to create socket\n"));
	  cli_socket = 0;
	  goto ferr_end;
	}

      if (core->ipv6_enable == 1)
	{
	  ppl_inet_pton ((const char *)host, (void *)&raddr6.sin6_addr);
	  raddr6.sin6_port = htons ((short) iptable_srv_port);
	  raddr6.sin6_family = AF_INET6;
	  i = connect (cli_socket, (struct sockaddr *) &raddr6, sizeof (raddr6));
	}
      else
	{
	  raddr.sin_addr.s_addr = inet_addr (host);
	  raddr.sin_port = htons ((short) iptable_srv_port);
	  raddr.sin_family = AF_INET;
	  i = connect (cli_socket, (struct sockaddr *) &raddr, sizeof (raddr));
	}

      if (i < 0)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "firewall module: failed to connect on %s:%i\n",
		       host, iptable_srv_port));
	  ppl_socket_close (cli_socket);
	  cli_socket = 0;
	  goto ferr_end;
	}
    }

  i = send (cli_socket, core->fw_entries[index].remove_rule,
	    strlen (core->fw_entries[index].remove_rule), 0);
  if (i <= 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "firewall module: failed to send iptable rule to server.\n"));
    }
  *(core->fw_entries[index].hash) = '\0';
  *(core->fw_entries[index].lan_ip) = '\0';
  *(core->fw_entries[index].lan_port) = '\0';
  *(core->fw_entries[index].remove_rule) = '\0';
  core->fw_entries[index].birth_date = 0;
  core->fw_entries[index].call_length = 0;

  for (index++;index<PSP_NAT_MAX_CALL;index++)
    {
      if (*(core->fw_entries[index].hash) == '\0')
	{}
      else if (0==strcmp(core->fw_entries[index].hash, uniquehash))
	{
	  /* delete all streams related to a call-id */
	  i = send (cli_socket, core->fw_entries[index].remove_rule,
		    strlen (core->fw_entries[index].remove_rule), 0);
	  if (i <= 0)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_ERROR, NULL,
			   "firewall module: failed to send iptable rule to server.\n"));
	    }
	  *(core->fw_entries[index].hash) = '\0';
	  *(core->fw_entries[index].lan_ip) = '\0';
	  *(core->fw_entries[index].lan_port) = '\0';
	  *(core->fw_entries[index].remove_rule) = '\0';
	  core->fw_entries[index].birth_date = 0;
	  core->fw_entries[index].call_length = 0;
	}
    }

  ppl_socket_close (cli_socket);
  cli_socket = 0;
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "firewall module: Rule removed from the firewall.\n"));
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "firewall module: %s.\n",core->fw_entries[index].remove_rule));
  return 0;

 ferr_end:
  /* continue on current index */
  for (;index<PSP_NAT_MAX_CALL;index++)
    {
      if (*(core->fw_entries[index].hash) == '\0')
	{}
      else if (0==strcmp(core->fw_entries[index].hash, uniquehash))
	{
	  *(core->fw_entries[index].hash) = '\0';
	  *(core->fw_entries[index].lan_ip) = '\0';
	  *(core->fw_entries[index].lan_port) = '\0';
	  *(core->fw_entries[index].remove_rule) = '\0';
	  core->fw_entries[index].birth_date = 0;
	  core->fw_entries[index].call_length = 0;
	}
    }
  return 0;
}

int
firewall_entry_add_rule (char *uniquehash, char *inc_port,
			 char *lan_ip, char *lan_port)
{
  static char *iptable_server = IPTABLE_SERVER_IP;
  /*  static char *rule = "-t nat -A PREROUTING -p udp --destination-port inc_port -j DNAT --to-destination lan_ip:lan_port" */
  static char *rule =
    "-t nat -A PREROUTING -p udp --destination-port %i -j DNAT --to-destination %s:%s";
  static char *remove_rule =
    "-t nat -D PREROUTING -p udp --destination-port %i -j DNAT --to-destination %s:%s";
  static struct sockaddr_in6 raddr6;
  static struct sockaddr_in raddr;
  static int cli_socket = 0;
  static int start_at_port = 30000;
  char buf[150];
  char buf2[150];
  char *host;
  int iptable_srv_port;
  int index;
  int i;

  if (core->iptables_dynamic_natrule == 0)
    {
      /* Dynamic insertion of rules is diabled */
      /* but masquerading_sdp is on */
      return osip_atoi(lan_port);
    }

  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO2, NULL,
	       "firewall module: Start connection to firewall server\n"));

  for (index=0;index<PSP_NAT_MAX_CALL;index++)
    {
      if (*(core->fw_entries[index].hash) == '\0')
	break;
      else if (0==strcmp(core->fw_entries[index].hash,uniquehash)
	       &&0==strcmp(core->fw_entries[index].lan_ip,lan_ip)
	       &&0==strcmp(core->fw_entries[index].lan_port,lan_port))
	/* rule already exist in firewall: probably a RE-INVITE? */
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO2, NULL,
		       "firewall module: rule already exist!\n"));
	  return 0;
	}
    }

  if (PSP_NAT_MAX_CALL==index)
    /* reach the limit of allowed call!
       This should be verified earlier in the process. */
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "firewall module: Maximum number of call reached!\n"));
      return -1;
    }
  
  start_at_port++;
  sprintf (buf, rule, start_at_port, lan_ip, lan_port);
  sprintf (buf2, remove_rule, start_at_port, lan_ip, lan_port);

  if (cli_socket == 0)		/* only once */
    {
      if (core->ipt_port == 0)	/* port is unspecified, use default */
	iptable_srv_port = IPTABLE_SERVER_PORT;
      else
	iptable_srv_port = core->ipt_port;
      if (core->ipt_server == NULL)
	host = iptable_server;	/* IPTABLE_SERVER_IP; */
      else
	host = core->ipt_server;

      /* the arguments line will be received through a socket */
      cli_socket = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (cli_socket < 0)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "firewall module: unable to create socket\n"));
	  cli_socket = 0;
	  return -1;
	}

      if (core->ipv6_enable == 1)
	{
	  ppl_inet_pton ((const char *)host, (void *)&raddr6.sin6_addr);
	  raddr6.sin6_port = htons ((short) iptable_srv_port);
	  raddr6.sin6_family = AF_INET6;
	  i = connect (cli_socket, (struct sockaddr *) &raddr6, sizeof (raddr6));
	}
      else
	{
	  raddr.sin_addr.s_addr = inet_addr (host);
	  raddr.sin_port = htons ((short) iptable_srv_port);
	  raddr.sin_family = AF_INET;
	  i = connect (cli_socket, (struct sockaddr *) &raddr, sizeof (raddr));
	}

      if (i < 0)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "firewall module: failed to connect on %s:%i\n",
		       host, iptable_srv_port));
	  ppl_socket_close (cli_socket);
	  cli_socket = 0;
	  return -1;
	}
    }
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO4, NULL,
	       "firewall module: new rule: %s\n", buf));

  i = send (cli_socket, buf, strlen (buf), 0);
  if (i <= 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "firewall module: failed to send iptable rule to server.\n"));
      ppl_socket_close (cli_socket);
      cli_socket = 0;
      return -1;
    }
  ppl_socket_close (cli_socket);
  cli_socket = 0;

  { /* find first free firewall entry and add it */
    sprintf(core->fw_entries[index].hash, uniquehash);
    sprintf(core->fw_entries[index].remove_rule, buf2);
    sprintf(core->fw_entries[index].lan_ip, lan_ip);
    sprintf(core->fw_entries[index].lan_port, lan_port);
    core->fw_entries[index].birth_date = time(NULL);
    core->fw_entries[index].call_length = 3600;

    OSIP_TRACE (osip_trace
		(__FILE__, __LINE__, OSIP_INFO3, NULL,
		 "firewall module: New rule description:\n"));
    OSIP_TRACE (osip_trace
		(__FILE__, __LINE__, OSIP_INFO3, NULL,
		 "firewall module: hash: %s, i: %i\n",
		 core->fw_entries[index].hash,
		 core->fw_entries[index].birth_date));
    OSIP_TRACE (osip_trace
		(__FILE__, __LINE__, OSIP_INFO3, NULL,
		 "firewall module: removal cmd: %s\n",
		 core->fw_entries[index].remove_rule));
  }

  /* should check if evrything was fine. */
  /*
   * RTP proxy: get ready and start forwarding
   * using an iptables rule.

   usefull comand:
   Print the filter rules:
   iptables -D PREROUTING 2
   Print the NAT rules:
   iptables -t nat -D PREROUTING 2

   Rule to apply to forward a RTP stream from the outside to a host in the lan
   iptables -t nat -A PREROUTING -p udp --destination-port lan_port -j DNAT
   __                      --to-destination lan_ip:lan_port

   To delete an entry in the table.
   iptables -t nat -D PREROUTING 2

   */
  return start_at_port;
}


/*
  This method check if the remote_ip is a host on our LAN
*/
int
check_subnet (int lan_ip, int lan_mask, int remote_ip)
{
  if ((lan_ip & lan_mask) == (remote_ip & lan_mask))
    return 0;
  return 1;
}

int
firewall_modify_osip_body_validate_condition1 (sdp_message_t * sdp)
{
  sdp_connection_t *conn;
  int remote_ip;
  int lan_ip;
  int lan_mask;
  /* find the first c= address in 200ok */
  conn = sdp->c_connection;
  if (conn == NULL || conn->c_addr == NULL)	/* no global connection info */
    {
      conn = sdp_message_connection_get (sdp, 0, 0);
      if (conn == NULL || conn->c_addr == NULL)	/* connection info is missing */
	/* do nothing... */
	{
	  return 1;
	}
    }
  /* Is this 'connection info' a host on the LAN subnet? */

  remote_ip = inet_addr (conn->c_addr);
  if (remote_ip == INADDR_NONE)
    return 1;			/* can this be an fqdn? (probably
				   external, so it should be ok) */
  lan_ip = inet_addr (core->lan_ip);
  if (lan_ip == INADDR_NONE)
    return -1;			/* this should never happen! */

  lan_mask = inet_addr (core->lan_mask);
  if (lan_mask == INADDR_NONE)
    return -1;			/* this should never happen! */

  if (0 != check_subnet (lan_ip, lan_mask, remote_ip))
    /* This stream is not to be received on our LAN. */
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "firewall module: callee is NOT the LAN!\n"));
      return 1;
    }
  return 0;
}

int
firewall_modify_200ok_validate_condition2 (osip_message_t * response, sdp_message_t * sdp)
{
  /*     
     Special case:(TODO)
     The caller is inside the same network but is calling somebody
     inside through an external proxy. The caller will appear to
     be outside with this algo. It's weird. We can detect this if
     we check all the Via header and find out that another was
     inserted by me...
   */
  int remote_ip;
  int lan_ip;
  int lan_mask;
  char *host;
  osip_via_t *via;
  osip_generic_param_t *maddr;
  osip_generic_param_t *received;
  osip_generic_param_t *rport;
  via = osip_list_get (&response->vias, 0);
  if (via == NULL)
    return -1;
  osip_via_param_get_byname (via, "maddr", &maddr);
  osip_via_param_get_byname (via, "received", &received);
  osip_via_param_get_byname (via, "rport", &rport);
  /* 1: skip the TCP implementation, partysip don't support it by now */
  /* 2: check maddr and multicast usage */
  if (maddr != NULL)
    host = maddr->gvalue;
  /* we should check if this is a multicast address and use
     set the "ttl" in this case. (this must be done in the
     UDP message (not at the SIP layer) */
  else if (received != NULL)
    host = received->gvalue;
  else
    host = via->host;

  remote_ip = inet_addr (host);
  if (remote_ip == INADDR_NONE)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_BUG, NULL,
		   "firewall module: can't analyse previous hop ip!\n"));
      return -1;		/* this is never a fqdn?? what happened */
    }

  lan_ip = inet_addr (core->lan_ip);
  if (lan_ip == INADDR_NONE)
    return -1;			/* this should never happen! */

  lan_mask = inet_addr (core->lan_mask);
  if (lan_mask == INADDR_NONE)
    return -1;			/* this should never happen! */

  if (0 == check_subnet (lan_ip, lan_mask, remote_ip))
    /* The original request cames from the LAN */
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "firewall module: caller is calling from the LAN\n"));
      return 1;			/* no need to change sdp */
    }
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO2, NULL,
	       "firewall module: caller is NOT calling from the LAN\n"));
  return 0;
}

int
firewall_modify_invite_validate_condition2 (osip_message_t * invite, sdp_message_t * sdp,
					    char *target_host)
{
  /*     
     An INVITE to be sent inside our LAN does not need to be
     modified.
   */
  int remote_ip;
  int lan_ip;
  int lan_mask;
    
  /* check destination info. */
  remote_ip = inet_addr (target_host);
  if (remote_ip == INADDR_NONE)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_BUG, NULL,
		   "firewall module: can't analyse previous hop ip!\n"));
      return -1;		/* this is never a fqdn?? what happened */
    }

  lan_ip = inet_addr (core->lan_ip);
  if (lan_ip == INADDR_NONE)
    return -1;			/* this should never happen! */

  lan_mask = inet_addr (core->lan_mask);
  if (lan_mask == INADDR_NONE)
    return -1;			/* this should never happen! */

  if (0 == check_subnet (lan_ip, lan_mask, remote_ip))
    /* The original request is going to the LAN */
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "firewall module: callee is probably inside the LAN (target info:%s)\n", target_host));
      return 1;			/* no need to change sdp */
    }
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO2, NULL,
	       "firewall module: callee is NOT inside the LAN\n"));
  return 0;
}

int
firewall_fix_200ok_osip_to_forward (osip_message_t * response)
{
  char *hash_value;
  int pos_body;
  osip_body_t *body;
  sdp_message_t *sdp;
  char *tmp;
  int i;

  sdp_media_t *med;
  char *lan_ip;

  if (response == NULL)
    return -1;
  if (MSG_IS_REQUEST (response))
    return 1;
  if (!MSG_IS_STATUS_2XX (response) && !MSG_IS_STATUS_1XX (response))
    return 1;

  i = -1;
  for (pos_body = 0; !osip_list_eol (&response->bodies, pos_body); pos_body++)
    {
      sdp = NULL;
      i = osip_message_get_body (response, pos_body, &body);
      if (i == 0)
	{
	  sdp_message_init (&sdp);
	  i = sdp_message_parse (sdp, body->body);
	  if (i != 0)
	    {
	      sdp_message_free (sdp);
	      sdp = NULL;
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_ERROR, NULL,
			   "firewall module: no SDP body in response!\n"));
	    }
	}
      if (i == 0)
	break;			/* sdp body found at pos_body */
    }

  if (i != 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_WARNING, NULL,
		   "firewall module: no sdp body in response!\n"));
      return 1;
    }


  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO2, NULL,
	       "firewall module: encode body in 200 response!\n"));

  /* remove any content lenth so it will be calculated again. */
  osip_content_length_free (response->content_length);
  response->content_length = NULL;

  /* is callee on the lan? */
  i = firewall_modify_osip_body_validate_condition1 (sdp);
  if (i != 0)
    {
      sdp_message_free (sdp);
      return i;			/* i can be (1) or (-1) */
    }

  /* is caller NOT on the lan? */
  i = firewall_modify_200ok_validate_condition2 (response, sdp);
  if (i != 0)
    {
      sdp_message_free (sdp);
      /* i can be (1) or (-1) */
      return i;
    }


  /* Yeap! we need NAT feature! */
  /* condition1: callee on LAN is valid */
  /* condition2: caller NOT on LAN is valid */

  /* From this point, we know for sure, this response comes from the LAN. */
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO2, NULL,
	       "firewall module: NAT translation required for 200ok!\n"));

  i = osip_call_id_to_str(response->call_id, &hash_value);
  if (i != 0)
    {
      sdp_message_free (sdp);
      return -1;
    }

  lan_ip = NULL;
  if (sdp->c_connection != NULL && sdp->c_connection->c_addr != NULL)
    {
      lan_ip = sdp->c_connection->c_addr;
      sdp->c_connection->c_addr = osip_strdup (core->ext_ip);
    }

  for (i = 0; !osip_list_eol (&sdp->m_medias, i); i++)
    {
      med = (sdp_media_t *) osip_list_get (&sdp->m_medias, i);
      if (med->m_media != NULL)
	{
	  sdp_connection_t *conn;
	  char *fw_port;
	  conn = (sdp_connection_t *) osip_list_get (&sdp->m_medias, i);
	  fw_port = med->m_port;
	  if (conn != NULL && conn->c_addr!=NULL)
	    {
	      /* TODO: should open the number of port specified */
	      int _fw_port;
	      _fw_port = firewall_entry_add_rule (hash_value, fw_port,
						  conn->c_addr, med->m_port);
	      if (_fw_port<0)
		break;
	      osip_free(conn->c_addr);
	      conn->c_addr = osip_strdup (core->ext_ip);	/* rewrite address */
	      osip_free(med->m_port);
	      med->m_port = osip_malloc(10);
	      sprintf(med->m_port, "%i", _fw_port);
	    }
	  else if (lan_ip != NULL)
	    {
	      int _fw_port;
	      _fw_port = firewall_entry_add_rule (hash_value, fw_port,
						  lan_ip, med->m_port);
	      if (_fw_port<0)
		break;
	      osip_free(med->m_port);
	      med->m_port = osip_malloc(10);
	      sprintf(med->m_port, "%i", _fw_port);
	    }
	}
    }
  osip_free(lan_ip);
  osip_free(hash_value);

  /* now we MUST replace the sdp body with the modified one.
     We know the sdp was at pos_body. */
  i = osip_message_get_body (response, pos_body, &body);
  if (body == NULL)
    {
      sdp_message_free (sdp);
      return -1;			/* but we detect it earlier? = BUG */
    }

  i = sdp_message_to_str (sdp, &tmp);
  sdp_message_free (sdp);
  if (i != 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "firewall module: Cannot replace new sdp body in response.\n"));
      return -1;
    }
  osip_free (body->body);
  body->body = tmp;
  /* Is there any content-length header in mime message? */
  response->message_property = 2;

  return 0;
}


int
firewall_fix_request_osip_to_forward (osip_message_t * invite, char *target_host)
{
  char *hash_value;
  int pos_body;
  osip_body_t *body;
  sdp_message_t *sdp;
  char *tmp;
  int i;

  sdp_media_t *med;
  char *lan_ip;

  if (invite == NULL)
    return -1;
  if (MSG_IS_RESPONSE (invite))
    return 1;

  i = -1;
  for (pos_body = 0; !osip_list_eol (&invite->bodies, pos_body); pos_body++)
    {
      sdp = NULL;
      i = osip_message_get_body (invite, pos_body, &body);
      if (i == 0)
	{
	  sdp_message_init (&sdp);
	  i = sdp_message_parse (sdp, body->body);
	  if (i != 0)
	    {
	      sdp_message_free (sdp);
	      sdp = NULL;
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO4, NULL,
			   "firewall module: not a SDP body!\n"));
	    }
	}
      if (i == 0)
	break;			/* sdp body found at pos_body */
    }

  if (i != 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_WARNING, NULL,
		   "firewall module: no sdp body in invite!\n"));
      return 1;
    }


  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO2, NULL,
	       "firewall module: encode body in invite!\n"));

  /* remove any content lenth so it will be calculated again. */
  osip_content_length_free (invite->content_length);
  invite->content_length = NULL;


  /* is caller on the lan? */
  i = firewall_modify_osip_body_validate_condition1 (sdp);
  if (i != 0)
    {
      sdp_message_free (sdp);
      return i;			/* i is (1) NOT (-1) */
    }

  /* is callee NOT on the lan? */
  i = firewall_modify_invite_validate_condition2 (invite, sdp, target_host);
  if (i != 0)
    {
      sdp_message_free (sdp);
      return i;			/* i is (1) NOT (-1) */
    }

  /* Yeap! we need NAT feature! */
  /* condition1: callee on LAN is valid */
  /* condition2: caller NOT on LAN is valid */

  /* From this point, we know for sure, this invite comes from the LAN. */
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO2, NULL,
	       "firewall module: NAT translation required for invite!\n"));

  i = osip_call_id_to_str(invite->call_id, &hash_value);
  if (i != 0)
    {
      sdp_message_free (sdp);
      return -1;
    }

  lan_ip = NULL;
  if (sdp->c_connection != NULL && sdp->c_connection->c_addr != NULL)
    {
      lan_ip = sdp->c_connection->c_addr;
      sdp->c_connection->c_addr = osip_strdup (core->ext_ip);
    }

  for (i = 0; !osip_list_eol (&sdp->m_medias, i); i++)
    {
      med = (sdp_media_t *) osip_list_get (&sdp->m_medias, i);
      if (med->m_media != NULL)	{
	  sdp_connection_t *conn;
	  char *fw_port;
	  conn = (sdp_connection_t *) osip_list_get (&sdp->m_medias, i);
	  fw_port = med->m_port;
	  if (conn != NULL && conn->c_addr!=NULL)
	    {
	      /* TODO: should open the number of port specified */
	      int _fw_port;
	      _fw_port = firewall_entry_add_rule (hash_value, fw_port,
						  conn->c_addr, med->m_port);
	      osip_free(conn->c_addr);	      
	      conn->c_addr = osip_strdup (core->ext_ip);	/* rewrite address */
	      osip_free(med->m_port);
	      med->m_port = osip_malloc(10);
	      sprintf(med->m_port, "%i", _fw_port);
	    }
	  else if (lan_ip != NULL)
	    {
	      /* no address to for this media rewrite address */
	      int _fw_port;
	      _fw_port = firewall_entry_add_rule (hash_value, fw_port,
						  lan_ip, med->m_port);
	      osip_free(med->m_port);
	      med->m_port = osip_malloc(10);
	      sprintf(med->m_port, "%i", _fw_port);
	    }
	}
    }
  osip_free(lan_ip);
  osip_free(hash_value);

  /* now we MUST replace the sdp body with the modified one.
     We know the sdp was at pos_body. */
  i = osip_message_get_body (invite, pos_body, &body);
  if (body == NULL)
    {
      sdp_message_free (sdp);
      return -1;			/* but we detect it earlier? = BUG */
    }

  i = sdp_message_to_str (sdp, &tmp);
  sdp_message_free (sdp);
  if (i != 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "firewall module: Cannot replace new sdp body in invite.\n"));
      return -1;
    }
  osip_free (body->body);
  body->body = tmp;
  /* Is there any content-length header in mime message? */
  invite->message_property = 2;

  return 0;
}

