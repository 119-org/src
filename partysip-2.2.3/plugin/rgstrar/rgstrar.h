/*
  The rgstrar plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The rgstrar plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The rgstrar plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef _REGISTRAR_H_
#define _REGISTRAR_H_

#include <ppl/ppl.h>
#include <ppl/ppl_md5.h>

typedef struct rgstrar_ctx_t
{

  int avoid_warning_at_compilation;

}
rgstrar_ctx_t;

int rgstrar_ctx_init (void);
void rgstrar_ctx_free (void);


/* hook for registrar/authentication plugin */
/* IMP hooks */
int cb_rgstrar_update_contact_list (psp_request_t * psp_req);	/* IMP HOOK_FIRST */

/* SFP hooks */
int cb_rgstrar_add_contacts_in_register (psp_request_t * psp_req, osip_message_t *response);	/* SFP HOOK_MIDDLE */

#endif
