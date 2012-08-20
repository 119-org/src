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


#ifndef _PARTYSIP_H_
#define _PARTYSIP_H_

#ifndef WIN32
#include "config.h"
#endif

#include <ppl/ppl_time.h>
#include <ppl/ppl_dso.h>
#include <ppl/ppl_socket.h>
#include <ppl/ppl_pipe.h>

#include <osip2/osip.h>
#include <osipparser2/osip_parser.h>
#include <osip2/osip_mt.h>

#include <partysip/psp_macros.h>
#include <partysip/psp_request.h>
#include <partysip/psp_plug.h>
#include <partysip/psp_config.h>

#include <partysip/psp_nat.h>

#ifndef INVITE_TIMEOUT
#define INVITE_TIMEOUT   (60)
#endif

#ifdef IPV6_SUPPORT
#define PARTYSIP_CONF "partysip6.conf"
#else
#define PARTYSIP_CONF "partysip.conf"
#endif


/* hidden (from user-point of view) config */
typedef struct _psp_osip_t
{
  osip_t *osip;			/* osip main element                         */
  osip_fifo_t *osip_tr_terminated;	/* osip transactions in the terminated state */
  int delay;			/* delay between update (should be 500ms)    */
  int exit_flag;		/* quick hack to shutdown the timers thread  */
  struct osip_thread *timers;		/* internal osip timer handler               */

  ppl_pipe_t *wakeup;

  /* basic statistics */
  int total_ict;
  int successfull_ict;
  int total_nict;
  int successfull_nict;
  int total_ist;
  int successfull_ist;
  int total_nist;
  int successfull_nist;

}
psp_osip_t;

int psp_osip_init (psp_osip_t ** psp_osip, int delay);
void psp_osip_free (psp_osip_t * psp_osip);
int psp_osip_wakeup (psp_osip_t * psp_osip);
int psp_osip_release (psp_osip_t * psp_osip);
int psp_osip_set_delay (psp_osip_t * psp_osip, int delay);


typedef struct _module_t
{

#define MOD_THREAD     1	/* start module_execute() once */
  int flag;			/* MOD_THREAD */
  struct osip_thread *thread;		/* modules are initiated in a thread (optionnaly) */
  ppl_pipe_t *wakeup;		/* descriptor used to wake up the thread */
  char *filename;
}
module_t;

int module_init (module_t ** module, int flag, char *filename);
void module_free (module_t * module);
int module_release (module_t * module);
int module_start (module_t * module, void *(*func_start) (void *), void *arg);
int module_set_flag (module_t * module, int flag);
int module_wakeup (module_t * module);

/********************************************************************/
/**************************   resolver    ***************************/

typedef struct _pspm_resolv_t
{
  module_t *module;

  /* no plugin */
}
pspm_resolv_t;

int pspm_resolv_init (pspm_resolv_t ** resolv);
void pspm_resolv_free (pspm_resolv_t * resolv);
int pspm_resolv_release (pspm_resolv_t * resolv);
int pspm_resolv_start (pspm_resolv_t * resolv, void *(*func_start) (void *),
		       void *arg);
int pspm_resolv_execute (pspm_resolv_t * resolv, int sec_max, int usec_max,
			 int max);
void pspm_resolv_handle_dns_query (pspm_resolv_t * resolv, char *address);


/********************************************************************/
/*****************************   tlp    *****************************/

typedef struct _pspm_tlp_t
{
  module_t *module;

  tlp_plugin_t *tlp_plugins;	/* list of tlp_plugins */

}
pspm_tlp_t;

int pspm_tlp_init (pspm_tlp_t ** pspm);
void pspm_tlp_free (pspm_tlp_t * pspm);
int pspm_tlp_execute (pspm_tlp_t * pspm, int sec_max,
		      int usec_max, int max_analysed);
int pspm_tlp_start (pspm_tlp_t * pspm, void *(*func_start) (void *),
		    void *arg);
int pspm_tlp_release (pspm_tlp_t * pspm);

/* int  pspm_tlp_add_plugin(pspm_tlp_t *m, psp_plugin_t* plugin) */
int pspm_tlp_send_message (pspm_tlp_t * pspm, osip_transaction_t * tr,
			   osip_message_t * sip, char *host, int port, int out_socket);

/********************************************************************/
/*****************************   sfp   *****************************/

typedef struct _pspm_sfp_t
{
  /* internal use only */
  module_t *module;

  sfp_plugin_t *sfp_plugins;

  sfp_inc_func_tab_t *rcv_invites;   /* reception of INVITE    */
  sfp_inc_func_tab_t *rcv_acks;	  /* reception of ACK       */
  sfp_inc_func_tab_t *rcv_registers; /*              REGISTER  */
  sfp_inc_func_tab_t *rcv_byes;      /*              BYE       */
  sfp_inc_func_tab_t *rcv_optionss;  /*              OPTIONS   */
  sfp_inc_func_tab_t *rcv_infos;     /*              INFO      */
  sfp_inc_func_tab_t *rcv_cancels;   /*              CANCEL    */
  sfp_inc_func_tab_t *rcv_notifys;   /*              NOTIFY    */
  sfp_inc_func_tab_t *rcv_subscribes;/*              SUBSCRIBE */
  sfp_inc_func_tab_t *rcv_unknowns;  /*              UNKNOWN   */

  osip_fifo_t *sip_ack;              /* osip_message_t ACK messages */
  osip_fifo_t *osip_message_traffic; /* New osip_transaction_t element. */

  sfp_fwd_func_tab_t *fwd_invites;	/* forward      INVITE    */
  sfp_fwd_func_tab_t *fwd_acks;	/* forward      ACK       */
  sfp_fwd_func_tab_t *fwd_registers;	/*              REGISTER  */
  sfp_fwd_func_tab_t *fwd_byes;	/*              BYE       */
  sfp_fwd_func_tab_t *fwd_optionss;	/*              OPTIONS   */
  sfp_fwd_func_tab_t *fwd_infos;	/*              INFO      */
  sfp_fwd_func_tab_t *fwd_cancels;	/*              CANCEL    */
  sfp_fwd_func_tab_t *fwd_notifys;	/*              NOTIFY    */
  sfp_fwd_func_tab_t *fwd_subscribes;	/*              SUBSCRIBE */
  sfp_fwd_func_tab_t *fwd_unknowns;	/*              UNKNOWN   */

  sfp_rcv_func_tab_t *rcv_1xxs;	/* receive      1xx    */
  sfp_rcv_func_tab_t *rcv_2xxs;	/*              2xx    */
  sfp_rcv_func_tab_t *rcv_3xxs;	/*              3xx    */
  sfp_rcv_func_tab_t *rcv_4xxs;	/*              4xx    */
  sfp_rcv_func_tab_t *rcv_5xxs;	/*              5xx    */
  sfp_rcv_func_tab_t *rcv_6xxs;	/*              6xx    */

  sfp_snd_func_tab_t *snd_1xxs;	/* forward      1xx    */
  sfp_snd_func_tab_t *snd_2xxs;	/*              2xx    */
  sfp_snd_func_tab_t *snd_3xxs;	/*              3xx    */
  sfp_snd_func_tab_t *snd_4xxs;	/*              4xx    */
  sfp_snd_func_tab_t *snd_5xxs;	/*              5xx    */
  sfp_snd_func_tab_t *snd_6xxs;	/*              6xx    */

  osip_fifo_t *sfull_request;	/* psp_request_t ELEMENT in STATEFULL_MODE */
  osip_fifo_t *sfull_cancels;	/* osip_message_t ELEMENT */

  osip_list_t *broken_transactions;
  /* Definitions of several STATEs for psp_req. Each defines the conditions
     required and the actions to be taken.

     1: waiting for new location to try
     CONDITIONS: req->locations->url->host is an ip address.
     or ppl_dns_get_result (&dns_result, loc->url->host); give a result.
     ACTION: create an outgoing transaction with the new location.
     2: waiting for informative answers in outgoing transactions
     CONDITIONS: psp_req->rsp_ctx->branches->transaction->state==XICT_PROCEEDING
     && psp_req->rsp_ctx->branches->last_status <
     atoi(psp_req->rsp_ctx->branches->transaction->last_response->status_code)))
     ACTION:  discard status==100
     ACTION2: forward response.
     3: waiting for final answer:
     CONDITIONS: psp_req->rsp_ctx->branches->transaction->state
     ==XICT_COMPLETED||==XICT_TERMINATED||
     && 199 <
     atoi(psp_req->rsp_ctx->branches->transaction->last_response->status_code)))
     ACTION: if response is 200 ==> forward response AND cancel other branches.
     ACTION2: if response is >199 AND no other branch => forward response.
     3: waiting for transaction timeout:
     CONDITIONS: psp_req->rsp_ctx->branches->transaction->state==XICT_PROCEEDING
     && psp_req->rsp_ctx->branches is expired
     ACTION: if response is 100 mark branch as deleted.
     ACTION2: if response is 101 to 199, cancel branch.
   */
  /*  struct osip_mutex *mut_psp_requests; */
  psp_request_t *psp_requests;
}
pspm_sfp_t;


int pspm_sfp_init (pspm_sfp_t ** pspm);
void pspm_sfp_free (pspm_sfp_t * pspm);
int pspm_sfp_execute (pspm_sfp_t * pspm, int sec_max, int usec_max, int max);
int pspm_sfp_start (pspm_sfp_t * pspm, void *(*func_start) (void *),
		    void *arg);
int pspm_sfp_release (pspm_sfp_t * pspm);


PPL_DECLARE (int) psp_core_event_add_sip_message (osip_event_t * evt);

int pspm_sfp_inc_dispatch_ack (pspm_sfp_t * pspm, osip_message_t * ack);
int pspm_sfp_inc_dispatch_traffic (pspm_sfp_t * pspm, osip_transaction_t * inc_tr);
int pspm_sfp_inc_call_plugins (pspm_sfp_t * pspm, psp_request_t * req);
int pspm_sfp_inc_dispatch_psp_request (pspm_sfp_t * pspm, psp_request_t * req);



int pspm_sfp_handle_new_cancel (pspm_sfp_t * pspm, osip_transaction_t * tr_cancel);
int pspm_sfp_handle_new_request (pspm_sfp_t * pspm, psp_request_t * req);
int pspm_sfp_handle_new_location (pspm_sfp_t * pspm, psp_request_t * req);
int pspm_sfp_delete_broken_location (pspm_sfp_t * pspm, psp_request_t * req);
int pspm_sfp_delete_completed_branchs (pspm_sfp_t * pspm, psp_request_t * req);
int pspm_sfp_delete_expired_branchs (pspm_sfp_t * pspm, psp_request_t * req);

int pspm_all_branch_are_terminated (pspm_sfp_t * pspm, psp_request_t * req);
int pspm_sfp_forward_200ok_final_response (pspm_sfp_t * pspm,
					   psp_request_t * req);
int pspm_sfp_forward_provisionnal_response (pspm_sfp_t * pspm,
					    psp_request_t * req);
int pspm_all_request_are_terminated (pspm_sfp_t * pspm, psp_request_t * req);
int pspm_sfp_wait_for_final_response (pspm_sfp_t * pspm, psp_request_t * req);
int pspm_sfp_answer_request_and_cancel_branches (pspm_sfp_t * pspm,
						 psp_request_t * req, int code);

int pspm_sfp_cancel_all_pending_branches (pspm_sfp_t * pspm, psp_request_t * req);
int pspm_sfp_delete_broken_transactions (pspm_sfp_t * pspm);

/* int pspm_sfp_reply_best_answer_or_404 (pspm_sfp_t * pspm, psp_request_t * req); */


int pspm_sfp_call_plugins_for_request (pspm_sfp_t * pspm, psp_request_t * req,
				       sfp_branch_t * sfp_branch);
int pspm_sfp_call_plugins_for_rcv_response (pspm_sfp_t * pspm,
					    psp_request_t * req,
					    sfp_branch_t * branch);
int pspm_sfp_call_plugins_for_snd_response (pspm_sfp_t * pspm,
					    psp_request_t * req,
					    osip_message_t *response);

#ifndef MAX_SERVER_IDENTIFIERS
#define MAX_SERVER_IDENTIFIERS	32
#endif

typedef struct _psp_core_t
{

  psp_osip_t *psp_osip;		/* osip related params */

  /* core elements */
  pspm_resolv_t *resolv;	/* module for Address Resolutio (no plugin) */
  pspm_tlp_t *tlp;		/* module for Transport layer Processing:   */
  pspm_sfp_t *sfp;		/* module for State Full Processing         */

  int   iptables_dynamic_natrule; /* NAT feature enabled if == 1 */

  char *ipt_server;		/* port for iptable_server     */
  int   ipt_port;		/* ip for iptable_port         */
  struct addrinfo *ipt_addr;

  int   masquerade_sdp;
  int   masquerade_via;
  char *remote_natip;

  char *lan_ip;
  char *lan_fqdn;
  char *lan_mask;
  char *ext_ip;
  char *ext_fqdn;
  char *ext_mask;
  int   recovery_delay;

  char *serverip[MAX_SERVER_IDENTIFIERS];	/* IP addresses for this proxy */
  char *servername[MAX_SERVER_IDENTIFIERS];	/* Hostnames for this proxy
						 * (should be ALL valid hostnames)
						 */
  int   ipv6_enable;
  char *serverip6[MAX_SERVER_IDENTIFIERS];	/* IP addresses for this proxy */

  int port;			/* this proxy runs on this port.             */
  struct osip_mutex *gl_lock;

  firewall_entry_t fw_entries[PSP_NAT_MAX_CALL];

  /* rfc3337.txt
     Extension Header Field for Registering Non-Adjacent Contacts */
  int rfc3327; /* if set: add a Path header in outgoing REGISTER messages */

  /* compatibility issues */
  /* this is for compatibility with some
     Uncompliant UAs (ATA v2.15 ata18x (020927a)) */
  int disable_check_for_osip_to_tag_in_cancel;	/* 0 inactive, 1 active */

  char banner[128];
}
psp_core_t;

int psp_core_init (void);
void psp_core_free (void);
int psp_core_load_all_ipv4(void);
int psp_core_load_all_ipv6(void);

int psp_core_lock (void);
int psp_core_unlock (void);
int psp_core_start (int use_this_thread);
int psp_core_execute (void);
void psp_core_remove_all_hook (void);
void psp_core_remove_all_plugins (void);
PPL_DECLARE (char *)
psp_core_get_ip_of_local_proxy (void);
PPL_DECLARE (int)
psp_core_get_port_of_local_proxy (void);
PPL_DECLARE (int)
psp_core_is_responsible_for_this_domain (osip_uri_t * url);
PPL_DECLARE (int)
psp_core_is_responsible_for_this_request_uri (osip_uri_t * url);
PPL_DECLARE (int)
psp_core_is_responsible_for_this_route (osip_uri_t * url);
int psp_core_sfp_generate_branch_for_request (osip_message_t * request, char *branch);
int psp_core_default_generate_branch_for_request (osip_message_t * request, char *branch);
psp_request_t *psp_core_sfp_find_req (int osip_id);
PPL_DECLARE (int)
psp_core_find_osip_transaction_and_add_event (osip_event_t * evt);
int psp_core_switch_osip_to_uas_mode (psp_request_t * req, int status);
PPL_DECLARE (int)
psp_core_fix_last_via_header (osip_event_t * evt, char *ip_addr, int port);
PPL_DECLARE (int)
psp_core_fix_strict_router_issue (osip_event_t * evt);

/* this method can handle late answer without calling plugins, this
   could lead to a response forwarded that does not match the
   previous ones. (in the case, some plugins act as if partysip
   is a B2BUA and for example modify the SDP body.) */
     int psp_core_handle_late_answer (osip_message_t * answer);

/* NETWORK related callbacks for osip */
/* Note: out_socket is not always set! For example, with UDP, we could
   always use the same global socket to send message. For TCP, we
   should create a new socket when a new transaction is created and
   this is the correct place to store the socket. In this case, you
   must also take care of closing the socket... */
     int psp_core_cb_snd_message (osip_transaction_t * tr, osip_message_t * sip, char *host,
				  int port, int out_socket);

/* UAS related callbacks for osip */
     void psp_core_cb_ist_kill_transaction (int type, osip_transaction_t * tr);
     void psp_core_cb_nist_kill_transaction (int type, osip_transaction_t * tr);
     void psp_core_cb_sndresp_retransmission (int type, osip_transaction_t * tr,
					      osip_message_t * sip);
     void psp_core_cb_rcvreq_retransmission (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_rcvinvite (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_rcvack (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_rcvack2 (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_rcvregister (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_rcvbye (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_rcvcancel (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_rcvinfo (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_rcvoptions (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_rcvprack (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_rcvunkrequest (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_snd1xx (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_snd2xx (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_snd3xx (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_snd4xx (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_snd5xx (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_snd6xx (int type, osip_transaction_t * tr, osip_message_t * sip);

/* UAC related callbacks for osip */
     void psp_core_cb_ict_kill_transaction (int type, osip_transaction_t * tr);
     void psp_core_cb_nict_kill_transaction (int type, osip_transaction_t * tr);
     void psp_core_cb_transport_error (int type, osip_transaction_t * tr, int error);
     void psp_core_cb_sndreq_retransmission (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_rcvresp_retransmission (int type, osip_transaction_t * tr,
					      osip_message_t * sip);
     void psp_core_cb_sndinvite (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_sndack (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_sndbye (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_sndcancel (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_sndinfo (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_sndoptions (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_sndregister (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_sndprack (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_sndunkrequest (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_rcv1xx (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_rcv2xx (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_rcv3xx (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_rcv4xx (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_rcv5xx (int type, osip_transaction_t * tr, osip_message_t * sip);
     void psp_core_cb_rcv6xx (int type, osip_transaction_t * tr, osip_message_t * sip);

/* this API must be available to plugins */
/* config step */
     int psp_core_load_psp_plugin (char *name, psp_plugin_t ** plug);

PPL_DECLARE (int)
psp_core_load_tlp_plugin (tlp_plugin_t ** p, psp_plugin_t * psp, int flag);
PPL_DECLARE (int)
psp_core_add_tlp_plugin (tlp_plugin_t * plug);

PPL_DECLARE (int)
psp_core_load_sfp_plugin (sfp_plugin_t ** p, psp_plugin_t * psp);
PPL_DECLARE (int)
psp_core_add_sfp_plugin (sfp_plugin_t * plug);

/***IMP***/
PPL_DECLARE (int)
psp_core_add_sfp_inc_invite_hook (sfp_inc_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_inc_ack_hook (sfp_inc_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_inc_register_hook (sfp_inc_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_inc_bye_hook (sfp_inc_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_inc_options_hook (sfp_inc_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_inc_info_hook (sfp_inc_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_inc_cancel_hook (sfp_inc_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_inc_notify_hook (sfp_inc_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_inc_subscribe_hook (sfp_inc_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_inc_unknown_hook (sfp_inc_func_t * fn, int hookflg);


/***SFP***/
PPL_DECLARE (int)
psp_core_add_sfp_fwd_invite_hook (sfp_fwd_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_fwd_ack_hook (sfp_fwd_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_fwd_register_hook (sfp_fwd_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_fwd_bye_hook (sfp_fwd_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_fwd_options_hook (sfp_fwd_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_fwd_info_hook (sfp_fwd_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_fwd_cancel_hook (sfp_fwd_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_fwd_notify_hook (sfp_fwd_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_fwd_subscribe_hook (sfp_fwd_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_fwd_unknown_hook (sfp_fwd_func_t * fn, int hookflg);

PPL_DECLARE (int)
psp_core_add_sfp_rcv_1xx_hook (sfp_rcv_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_rcv_2xx_hook (sfp_rcv_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_rcv_3xx_hook (sfp_rcv_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_rcv_4xx_hook (sfp_rcv_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_rcv_5xx_hook (sfp_rcv_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_rcv_6xx_hook (sfp_rcv_func_t * fn, int hookflg);

PPL_DECLARE (int)
psp_core_add_sfp_snd_1xx_hook (sfp_snd_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_snd_2xx_hook (sfp_snd_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_snd_3xx_hook (sfp_snd_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_snd_4xx_hook (sfp_snd_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_snd_5xx_hook (sfp_snd_func_t * fn, int hookflg);
PPL_DECLARE (int)
psp_core_add_sfp_snd_6xx_hook (sfp_snd_func_t * fn, int hookflg);

/* run time step */
PPL_DECLARE (int)
psp_core_dns_add_domain_error (char *address);
PPL_DECLARE (int)
psp_core_dns_add_domain_result (ppl_dns_entry_t * dns);
PPL_DECLARE (int)
psp_core_event_resolv_add_query (char *address);

/* for all events coming from the UDP listener */
PPL_DECLARE (int)
psp_core_event_add_sfp_inc_message (osip_event_t * evt);
PPL_DECLARE (int)
psp_core_event_add_sfp_inc_ack (osip_message_t * ack);
PPL_DECLARE (int)
psp_core_event_add_sfp_inc_traffic (osip_transaction_t * inc_tr);

PPL_DECLARE (int)
psp_core_event_add_sfull_request (psp_request_t * req);
PPL_DECLARE (int)
psp_core_event_add_sfull_cancel (osip_transaction_t * tr_cancel);

#endif
