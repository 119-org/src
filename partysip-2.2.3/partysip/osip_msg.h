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

/* API forbidden for codes between 101 and 199 */
int osip_msg_build_response (osip_message_t ** dest, int status, osip_message_t * request);

int osip_msg_default_build_request_osip_to_forward (osip_message_t ** fwd, osip_uri_t * req_uri,
					       char *path,   osip_message_t * req,
					       int stayonpath);
int osip_msg_sfp_build_request_osip_to_forward (psp_request_t * req, osip_message_t ** fwd,
					   osip_uri_t * req_uri,   char *path,
					   osip_message_t * request, int stayonpath);
int osip_msg_sfp_build_response_osip_to_forward (osip_message_t ** dest, osip_message_t * response);

int osip_msg_modify_ack_osip_to_be_forwarded (osip_message_t * ack, int stayonpath);
int osip_msg_build_cancel (osip_message_t ** dest, osip_message_t * request_cancelled);
