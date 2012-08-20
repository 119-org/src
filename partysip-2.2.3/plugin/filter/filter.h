/*
  The filter plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The filter plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The filter plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _FILTER_H_
#define _FILTER_H_

#include <ppl/ppl.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if !defined(WIN32) || defined(TRE) /* no regex.h on win32? */
#include <regex.h>
#endif

/* rules for tel urls */
typedef struct tel_rule_t tel_rule_t;

struct tel_rule_t {
  char prefix[101];         /* prefix for matching rules.  */
#if !defined(WIN32) || defined(TRE) /* no regex.h on win32? */
  regex_t cprefix;        /* compiled rule */
#endif
  char dnsresult[101];     /* static route -empty when DNS lookup will be used */

  tel_rule_t *next;
  tel_rule_t *parent;
};

typedef struct filter_ctx_t {

  int   flag;    /* reflect the configuration given in partysip.conf */
  tel_rule_t *tel_rules;

} filter_ctx_t;

int  filter_ctx_init(void);
void filter_ctx_free(void);

/* hook for filter plugin */
int  cb_filter_search_location(psp_request_t *psp_req);


#endif
