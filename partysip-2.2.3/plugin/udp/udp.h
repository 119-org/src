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


#ifndef _UDP_H_
#define _UDP_H_

#include <ppl/ppl.h>

typedef struct local_ctx_t
{
  int in_port;
  int in_socket;
  int mcast_socket;
  int out_port;
  int out_socket;
}
local_ctx_t;

int local_ctx_init (int in_port, int out_port);
void local_ctx_free (void);

/* callback for tlp plugin */
int cb_rcv_udp_message (int max);

int cb_snd_udp_message (osip_transaction_t * transaction,	/* read only element */
			osip_message_t * message,	/* message to send           */
			char *host,	/* proposed destination host */
			int port,	/* proposed destination port */
			int socket);	/* proposed socket (if any)  */

/* miscelaneous methods */
int udp_log_event (osip_event_t * evt);
int udp_process_message (char *buf, char *ip_addr, int port, int length);


#endif
