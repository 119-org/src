/*
  The oSIP library implements the Session Initiation Protocol (SIP -rfc3261-)
  Copyright (C) 2001,2002,2003  Aymeric MOIZARD jack@atosc.org
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _FSM_H_
#define _FSM_H_

#include <osipparser2/osip_message.h>
#include <osip2/osip.h>

void add_gettimeofday(struct timeval *atv, int ms);
void min_timercmp(struct timeval *tv1, struct timeval *tv2);

#if defined (WIN32) || defined(NEED_GETTIMEOFDAY)
int gettimeofday(struct timeval *tp, void *);
#endif

#define EVT_RCV_REQUEST                          0
#define TIMEOUT_NOANSWER                         1
#define TIMEOUT_NOFINALANSWER                    2
#define TIMEOUT_CLOSE                            3
#define EVT_RCV_STATUS_1XX                       4
#define EVT_RCV_STATUS_3456XX                    5
#define EVT_RCV_STATUS_2XX                       6
#define EVT_RCV_CANCEL                           7
#define EVT_NEW_LOCATION                         8
#define EVT_ALL_BRANCH_ANSWERED                  9
#define EVT_FALLBACK_LOCATION                   10

/**
 * Structure for sipevent handling.
 * A psp_event_t element will have a type and will be related
 * to a transaction. In the general case, it is used by the
 * application layer to give SIP messages to the oSIP finite
 * state machine.
 * @defvar psp_event_t
 */
  typedef struct psp_event psp_event_t;

  struct psp_event
  {
    int type;
    int transactionid;
    int branchid;
  };

/**
 * Allocate a sipevent.
 * NOTE: THIS IS AN INTERNAL METHOD ONLY
 */
psp_event_t *__psp_event_new (int type);
void __psp_event_set_response(psp_event_t *evt, int branchid);

void __proxy_load_fsm (void);
void __proxy_unload_fsm (void);
int psp_request_execute (psp_request_t *psp_request, psp_event_t * evt);

void dispatch_cancel_event(pspm_sfp_t *pspm, osip_transaction_t * tr_cancel);
psp_event_t *pspm_sfp_need_start_fallback_location(psp_request_t *req);
psp_event_t *pspm_sfp_new_location_received(psp_request_t *req);
psp_event_t *pspm_sfp_need_timeout_noanswer(psp_request_t *req);
psp_event_t *pspm_sfp_need_timeout_nofinalanswer(psp_request_t *req);
psp_event_t *pspm_sfp_need_timeout_close(psp_request_t *req);
psp_event_t *pspm_sfp_test_received_1xx(psp_request_t *req);
psp_event_t *pspm_sfp_test_received_2xx(psp_request_t *req);
psp_event_t *pspm_sfp_test_received_3456xx(psp_request_t *req);

int sfp_create_branch (psp_request_t * req, location_t *loc, ppl_dns_ip_t * ips);
int sfp_forward_response (psp_request_t * req, sfp_branch_t *br);
int sfp_cancel_pending_branch(psp_request_t *req, sfp_branch_t *br);
int sfp_forward_ack(psp_request_t *req);
int sfp_answer_request(psp_request_t * req);

#endif
