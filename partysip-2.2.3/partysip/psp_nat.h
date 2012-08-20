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

#ifndef _PSP_NAT_H_
#define _PSP_NAT_H_

#include <osipparser2/sdp_message.h>

#ifndef PSP_NAT_MAX_CALL
#define PSP_NAT_MAX_CALL 50
#endif

#ifndef IPTABLE_SERVER_PORT
#define IPTABLE_SERVER_PORT 20530
#endif

#ifndef IPTABLE_SERVER_IP
#define IPTABLE_SERVER_IP   "127.0.0.1"
#endif


typedef struct firewall_entry_t firewall_entry_t;

struct firewall_entry_t
{
  /* hash = callid (**so forking is not allowed**) */
  char hash[100]; /* common info to match initial INVITE and final BYE. */
  char lan_ip[20];        /* lan_ip for user in LAN. */
  char lan_port[10];      /* lan_port for user in LAN. */
  char remove_rule[255];  /* string to send to delete the entry. */
  int  birth_date;        /* birth date of new call. */
  int  call_length;       /* max duration for call.  */
} ;


int check_subnet (int lan_ip, int lan_mask, int remote_ip);
int firewall_modify_osip_body_validate_condition1 (sdp_message_t * sdp);
int firewall_modify_200ok_validate_condition2 (osip_message_t * response, sdp_message_t * sdp);
int firewall_modify_invite_validate_condition2 (osip_message_t * invite, sdp_message_t * sdp,
						char *target_host);

int firewall_entry_remove_rule (char *uniquehash);
int firewall_entry_add_rule(char *uniquehash, char *inc_port,
			    char *lan_ip, char *lan_port);
int firewall_fix_200ok_osip_to_forward (osip_message_t * response);
int firewall_fix_invite_osip_to_forward (osip_message_t * invite, char *target_host);
int firewall_fix_request_osip_to_forward (osip_message_t * request, char *target_host);

#endif
