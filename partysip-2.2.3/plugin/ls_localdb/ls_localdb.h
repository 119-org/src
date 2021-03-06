/*
  The ls_localdb plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The ls_localdb plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The ls_localdb plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef _LS_LOCALDB_H_
#define _LS_LOCALDB_H_


#include <ppl/ppl.h>

typedef struct ls_localdb_ctx_t
{

  int flag;

}
ls_localdb_ctx_t;

int ls_localdb_ctx_init (void);
void ls_localdb_ctx_free (void);

/* hook for route_static plugin */
int cb_ls_localdb_search_user_location (psp_request_t * psp_req);


#endif
