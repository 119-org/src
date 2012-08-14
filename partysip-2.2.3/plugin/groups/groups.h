/*
  The groups plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The groups plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The groups plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _GROUPS_H_
#define _GROUPS_H_

#include <ppl/ppl.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef MAX_MEMBERS
#define MAX_MEMBERS 50
#endif

#ifndef MAX_GROUPS
#define MAX_GROUPS 20
#endif

/* rules for tel urls */
typedef struct grp grp_t;

struct grp {
  char group[255];                 /* groupname of the group */
  char domain[255];                /* domain of the group   */

  char members[MAX_MEMBERS][255];  /* space for 50 members  */

  int  flag;    /* reflect the configuration given in partysip.conf */
};

typedef struct groups_ctx_t {

  int   flag;    /* reflect the configuration given in partysip.conf */
  grp_t grps[MAX_GROUPS];

} groups_ctx_t;

int  groups_ctx_init(void);
void groups_ctx_free(void);

/* hook for groups plugin */
int  cb_groups_search_location(psp_request_t *psp_req);


#endif
