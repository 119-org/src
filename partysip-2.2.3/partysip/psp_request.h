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


#ifndef _PSP_REQ_H
#define _PSP_REQ_H

#ifndef WIN32
#include "config.h"
#endif

#include <ppl/ppl_time.h>
#include <ppl/ppl_dso.h>
#include <ppl/ppl_dns.h>

#include <osip2/osip.h>
#include <osipparser2/osip_parser.h>
#include <osip2/osip_mt.h>

#include <partysip/psp_macros.h>

/* psp_req_t is
   1: created by the imp module upon reception of a new request
   2: given to the plugins sequentially
   3: updated by plugins
   4: given in the fifo of ups, slr or sfp
   Final Information is used to answer or forward the transaction
*/


typedef struct sfp_branch_t sfp_branch_t;

struct sfp_branch_t
{
  int branchid; /* unique id */
  char branch[45];
  ppl_time_t ctx_start;
  int timer_c;
  int ctx_timeout;
  osip_transaction_t *out_tr;
  osip_uri_t *url;
  ppl_dns_ip_t *fallbacks;
  osip_message_t *request;
  osip_message_t *response;
  int last_code_forwarded;
  int already_cancelled;
  sfp_branch_t *next;
  sfp_branch_t *parent;
};

int sfp_branch_init (sfp_branch_t ** sfp_branch, char *branch);
void sfp_branch_free (sfp_branch_t * branch);
int sfp_branch_get_osip_id (sfp_branch_t * branch);
int sfp_branch_set_transaction (sfp_branch_t * branch,
				osip_transaction_t * out_tr);
int sfp_branch_set_timeout (sfp_branch_t * branch, int delay);
int sfp_branch_is_expired (sfp_branch_t * branch);
int sfp_branch_set_url (sfp_branch_t * branch, osip_uri_t * url);
int sfp_branch_set_request (sfp_branch_t * branch, osip_message_t * req);
int sfp_branch_set_response (sfp_branch_t * branch, osip_message_t * resp);
int sfp_branch_get_last_code_forwarded (sfp_branch_t * sfp);
int sfp_branch_set_last_code_forwarded (sfp_branch_t * sfp, int lastcode);
int sfp_branch_clone_and_set_fallbacks (sfp_branch_t * branch,
					ppl_dns_ip_t * ips);

typedef struct location_t location_t;

struct location_t
{
  osip_uri_t *url;			/* THIS SHOULD BE CHANGED TO A CONTACT HEADER!!! */
  char  *path;			/* add a path information (see rfc3327.txt) */
  int expire;
  location_t *next;
  location_t *parent;
};

PPL_DECLARE (int)
location_init (location_t ** location, osip_uri_t * url, int expire);
PPL_DECLARE (void)
location_free (location_t * location);
PPL_DECLARE (int)
location_set_url (location_t * location, osip_uri_t * url);
PPL_DECLARE (int)
location_set_expires (location_t * location, int expire);
PPL_DECLARE (int)
location_set_path (location_t * location, char *path);
PPL_DECLARE (int)
location_clone (location_t * loc, location_t ** loc2);


/* extra stuff for dialog between core layer and plugins.
   plugin id: unique number for plugin.
   request id: transaction id(?) that triggered this request.
*/
/*
#define CTX_READY           0x01
#define CTX_NOT_READY       0x02
#define CTX_PARTIAL         0x04 */	/* in later step? */

typedef struct _psp_request psp_request_t;

struct _psp_request
{
  int owners;
#define PROXY_PRE_CALLING 0
#define PROXY_CALLING     1
#define PROXY_PROCEEDING  2
#define PROXY_CLOSING     3
#define PROXY_CLOSED      4
  int state;
  int start;
#define PSP_UAS_MODE        0x001
#define PSP_SFULL_MODE      0x008

#define PSP_SEQ_MODE        0x002
#define PSP_FORK_MODE       0x008
  
  /* state flag */
#define PSP_CONTINUE        0x010
#define PSP_PROPOSE         0x020
#define PSP_MANDATE         0x040
#define PSP_STOP            0x080

  /* mode properties */
#define PSP_STAY_ON_PATH    0x100
  
#define __IS_PSP_UAS_MODE(flag)   ((~flag | ~PSP_UAS_MODE ) == ~PSP_UAS_MODE)
#define __IS_PSP_SFULL_MODE(flag) ((~flag | ~PSP_SFULL_MODE ) == ~PSP_SFULL_MODE)
#define __IS_PSP_FORK_MODE(flag) ((~flag | ~PSP_FORK_MODE ) == ~PSP_FORK_MODE)
#define __IS_PSP_SEQ_MODE(flag) ((~flag | ~PSP_SEQ_MODE ) == ~PSP_SEQ_MODE)
  
#define __SET_UAS_MODE(flag)      (flag = ((flag&0xFF0)|PSP_UAS_MODE))
#define __SET_SFULL_MODE(flag)    (flag = ((flag&0xFF0)|PSP_SFULL_MODE))
#define __SET_FORK_MODE(flag)    (flag = ((flag&0xFF0)|PSP_FORK_MODE))
#define __SET_SEQ_MODE(flag)    (flag = ((flag&0xFF0)|PSP_SEQ_MODE))
  
#define __IS_PSP_CONTINUE(flag)   ((~flag | ~PSP_CONTINUE ) == ~PSP_CONTINUE)
#define __IS_PSP_PROPOSE(flag)    ((~flag | ~PSP_PROPOSE  ) == ~PSP_PROPOSE)
#define __IS_PSP_MANDATE(flag)    ((~flag | ~PSP_MANDATE  ) == ~PSP_MANDATE)
#define __IS_PSP_STOP(flag)       ((~flag | ~PSP_STOP  ) == ~PSP_STOP)
  
#define __SET_CONTINUE(flag)      (flag = ((flag&0xF0F)|PSP_CONTINUE))
#define __SET_PROPOSE(flag)       (flag = ((flag&0xF0F)|PSP_PROPOSE))
#define __SET_MANDATE(flag)       (flag = ((flag&0xF0F)|PSP_MANDATE))
#define __SET_STOP(flag)          (flag = ((flag&0xF0F)|PSP_STOP))

#define __SET_STAY_ON_PATH(flag)    (flag = ((flag&0x0FF)|PSP_STAY_ON_PATH))
#define __IS_PSP_STAY_ON_PATH(flag) ((~flag|~PSP_STAY_ON_PATH)==~PSP_STAY_ON_PATH)
  int flag; 		/* contains state and mode flags */
  int uas_status;	/* UAS return code proposed or mandated */
#ifndef TIMEOUT_FOR_NO_ANSWER
#define TIMEOUT_FOR_NO_ANSWER 15
#endif
  int timer_noanswer_length;
  struct timeval timer_noanswer_start;
#ifndef TIMEOUT_FOR_INVITE
#define TIMEOUT_FOR_INVITE 120
#endif
#ifndef TIMEOUT_FOR_NONINVITE
#define TIMEOUT_FOR_NONINVITE 32
#endif
  int timer_nofinalanswer_length;
  struct timeval timer_nofinalanswer_start;
  int timer_close_length;
  struct timeval timer_close_start;

  location_t     *locations;
  location_t     *fallback_locations;
  osip_transaction_t *inc_tr;
  int             branch_index;
  char            branch[45];
  sfp_branch_t   *branch_notanswered;
  sfp_branch_t   *branch_answered;
  sfp_branch_t   *branch_completed;
  sfp_branch_t   *branch_cancelled;
  osip_message_t *out_response;
  osip_message_t *inc_ack;
  struct _psp_request *parent;
  struct _psp_request *next;
};


int psp_request_init (psp_request_t ** req, osip_transaction_t * inc_tr,
		      osip_message_t *ack);
void psp_request_free (psp_request_t * req);
PPL_DECLARE (int)
psp_request_set_state (psp_request_t * req, int state);
PPL_DECLARE (int)
psp_request_set_mode (psp_request_t * req, int mode);
PPL_DECLARE (int)
psp_request_set_property (psp_request_t * req, int property);
PPL_DECLARE (int)
psp_request_set_uas_status (psp_request_t * req, int status);

int psp_request_take_ownership (psp_request_t * req);
int psp_request_release_ownership (psp_request_t * req);

PPL_DECLARE (int)
psp_request_get_uas_status (psp_request_t * req);

PPL_DECLARE (osip_message_t*)
psp_request_get_request(psp_request_t * req);

#endif
