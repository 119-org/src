/*
  The ls_static plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The ls_static plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The ls_static plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _LS_STATIC_H_
#define _LS_STATIC_H_

#include <ppl/ppl.h>

typedef struct ls_static_ctx_t
{

  int flag;			/* reflect the configuration given in partysip.conf */

  config_element_t *elem_forward;
  config_element_t *elem_reject;

}
ls_static_ctx_t;

int ls_static_ctx_init (void);
void ls_static_ctx_free (void);

/* hook for ls_static plugin */
int cb_ls_static_search_location (psp_request_t * psp_req);


#endif
