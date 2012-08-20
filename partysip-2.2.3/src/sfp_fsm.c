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

#include <partysip/osip_msg.h>

#include <ppl/ppl_dns.h>
#include "proxyfsm.h"

typedef struct psp_statemachine psp_statemachine_t;

struct psp_statemachine
{
  osip_list_t *transitions;
};


psp_event_t *__psp_event_new (int type)
{
  psp_event_t *event = (psp_event_t *) osip_malloc(sizeof(psp_event_t));
  if (event==NULL)
    return NULL;
  memset(event, 0, sizeof(psp_event_t));
  event->type = type;
  return event;
}

void
__psp_event_set_response(psp_event_t *evt, int branchid)
{
  if (evt==NULL) return;
  evt->branchid = branchid;
}

typedef struct _transition_t
{
  state_t state;
  type_t type;
  void (*method) (void *, void *);
}
transition_t;

psp_statemachine_t *proxy_fsm;

static transition_t *
proxy_fsm_findmethod (type_t type, state_t state)
{
  int pos;

  pos = 0;
  while (!osip_list_eol (proxy_fsm->transitions, pos))
    {
      transition_t *transition;
      transition = (transition_t *) osip_list_get (proxy_fsm->transitions, pos);
      if (transition->type == type && transition->state == state)
	return transition;
      pos++;
    }
  return NULL;
}


/* call the right execution method.          */
/*   return -1 when event must be discarded  */
static int
proxy_fsm_callmethod (type_t type, void *proxyevent, psp_request_t *psp_req)
{
  transition_t *transition;

  transition = proxy_fsm_findmethod (type, psp_req->state);
  if (transition == NULL)
    {
      /* No transition found for this event */
      return -1;		/* error */
    }
  transition->method (psp_req, proxyevent);
  return 0;			/* ok */
}


int
psp_request_execute (psp_request_t *psp_request, psp_event_t * evt)
{
  /* to kill the process, simply send this type of event. */
  if (EVT_IS_KILL_TRANSACTION (evt))
    {
      /* MAJOR CHANGE!
         TRANSACTION MUST NOW BE RELEASED BY END-USER:
         So Any usefull data can be save and re-used */
      /* osip_transaction_free(transaction);
         osip_free(transaction); */
      osip_free (evt);
      return 0;
    }

  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO4, NULL,
	       "partysipevent psp_request: %x\n", psp_request));
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO4, NULL,
	       "partysipevent psp_request->state: %i\n", psp_request->state));
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO4, NULL,
	       "partysipevent evt->type: %i\n", evt->type));
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO4, NULL,
	       "partysipevent evt->branchid: %i\n", evt->branchid));

  if (-1 == proxy_fsm_callmethod (evt->type,
			    evt,
			    psp_request))
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO3, NULL, "USELESS event!\n"));
      /* message is useless. */
    }
  else
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO4, NULL,
		   "sipevent evt: method called!\n"));
    }
  osip_free (evt);  /* this is the ONLY place for freeing event!! */
  return 1;
}

/************************/
/* FSM  ---- > PROXY    */
/************************/

void proxy_snd_all_request (psp_request_t * pspm, psp_event_t * evt);
void proxy_start_fallback_locations (psp_request_t * pspm, psp_event_t * evt);
void proxy_timeout_try_new_location (psp_request_t * pspm, psp_event_t * evt);
void proxy_timeout_noanswer (psp_request_t * pspm, psp_event_t * evt);
void proxy_timeout_nofinalanswer (psp_request_t * pspm, psp_event_t * evt);
void proxy_timeout_close (psp_request_t * req, psp_event_t * evt);
void proxy_rcv_1xx (psp_request_t * pspm, psp_event_t * evt);
void proxy_rcv_2xx (psp_request_t * pspm, psp_event_t * evt);
void proxy_rcv_3456xx (psp_request_t * pspm, psp_event_t * evt);
void proxy_rcv_cancel (psp_request_t * pspm, psp_event_t * evt);

extern psp_core_t *core;

void
__proxy_unload_fsm ()
{
  transition_t *transition;

  while (!osip_list_eol (proxy_fsm->transitions, 0))
    {
      transition = (transition_t *) osip_list_get (proxy_fsm->transitions, 0);
      osip_list_remove (proxy_fsm->transitions, 0);
      osip_free (transition);
    }
  osip_free (proxy_fsm->transitions);
  osip_free (proxy_fsm);
}


void
__proxy_load_fsm ()
{
  transition_t *transition;

  proxy_fsm = (psp_statemachine_t *) osip_malloc (sizeof (psp_statemachine_t));
  proxy_fsm->transitions = (osip_list_t *) osip_malloc (sizeof (osip_list_t));
  osip_list_init (proxy_fsm->transitions);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_PRE_CALLING;
  transition->type = EVT_RCV_REQUEST;
  transition->method = (void (*)(void *, void *)) &proxy_snd_all_request;
  osip_list_add (proxy_fsm->transitions, transition, -1);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_CALLING;
  transition->type = EVT_NEW_LOCATION;
  transition->method = (void (*)(void *, void *)) &proxy_timeout_try_new_location;
  osip_list_add (proxy_fsm->transitions, transition, -1);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_PROCEEDING;
  transition->type = EVT_RCV_CANCEL;
  transition->method = (void (*)(void *, void *)) &proxy_rcv_cancel;
  osip_list_add (proxy_fsm->transitions, transition, -1);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_PROCEEDING;
  transition->type = EVT_NEW_LOCATION;
  transition->method = (void (*)(void *, void *)) &proxy_timeout_try_new_location;
  osip_list_add (proxy_fsm->transitions, transition, -1);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_PRE_CALLING;
  transition->type = TIMEOUT_NOANSWER;
  transition->method = (void (*)(void *, void *)) &proxy_timeout_noanswer;
  osip_list_add (proxy_fsm->transitions, transition, -1);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_CALLING;
  transition->type = TIMEOUT_NOANSWER;
  transition->method = (void (*)(void *, void *)) &proxy_timeout_noanswer;
  osip_list_add (proxy_fsm->transitions, transition, -1);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_PROCEEDING;
  transition->type = TIMEOUT_NOFINALANSWER;
  transition->method = (void (*)(void *, void *)) &proxy_timeout_nofinalanswer;
  osip_list_add (proxy_fsm->transitions, transition, -1);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_CALLING;
  transition->type = EVT_RCV_STATUS_1XX;
  transition->method = (void (*)(void *, void *)) &proxy_rcv_1xx;
  osip_list_add (proxy_fsm->transitions, transition, -1);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_PROCEEDING;
  transition->type = EVT_RCV_STATUS_1XX;
  transition->method = (void (*)(void *, void *)) &proxy_rcv_1xx;
  osip_list_add (proxy_fsm->transitions, transition, -1);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_CALLING;
  transition->type = EVT_RCV_STATUS_3456XX;
  transition->method = (void (*)(void *, void *)) &proxy_rcv_3456xx;
  osip_list_add (proxy_fsm->transitions, transition, -1);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_PROCEEDING;
  transition->type = EVT_RCV_STATUS_3456XX;
  transition->method = (void (*)(void *, void *)) &proxy_rcv_3456xx;
  osip_list_add (proxy_fsm->transitions, transition, -1);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_CALLING;
  transition->type = EVT_RCV_STATUS_2XX;
  transition->method = (void (*)(void *, void *)) &proxy_rcv_2xx;
  osip_list_add (proxy_fsm->transitions, transition, -1);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_PROCEEDING;
  transition->type = EVT_RCV_STATUS_2XX;
  transition->method = (void (*)(void *, void *)) &proxy_rcv_2xx;
  osip_list_add (proxy_fsm->transitions, transition, -1);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_PROCEEDING;
  transition->type = EVT_ALL_BRANCH_ANSWERED;
  transition->method = (void (*)(void *, void *)) &proxy_rcv_2xx;
  osip_list_add (proxy_fsm->transitions, transition, -1);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_PROCEEDING;
  transition->type = TIMEOUT_CLOSE;
  transition->method = (void (*)(void *, void *)) &proxy_timeout_close;
  osip_list_add (proxy_fsm->transitions, transition, -1);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_CLOSING;
  transition->type = TIMEOUT_CLOSE;
  transition->method = (void (*)(void *, void *)) &proxy_timeout_close;
  osip_list_add (proxy_fsm->transitions, transition, -1);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_CALLING;
  transition->type = EVT_FALLBACK_LOCATION;
  transition->method = (void (*)(void *, void *)) &proxy_start_fallback_locations;
  osip_list_add (proxy_fsm->transitions, transition, -1);

  transition = (transition_t *) osip_malloc (sizeof (transition_t));
  transition->state = PROXY_PROCEEDING;
  transition->type = EVT_FALLBACK_LOCATION;
  transition->method = (void (*)(void *, void *)) &proxy_start_fallback_locations;
  osip_list_add (proxy_fsm->transitions, transition, -1);

}


void proxy_snd_all_request (psp_request_t * req, psp_event_t * evt)
{
  int i;
  location_t *loc;
  location_t *ptr;
  osip_message_t *request = psp_request_get_request(req);

  if (__IS_PSP_UAS_MODE (req->flag))
    {
      sfp_answer_request(req);
      req->state = PROXY_CLOSING;
      return;
    }

  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "proxyfsm: proxy_snd_all_request evt=%x\n", evt));

  /* refuse plugin who gives non sip/sips-urls. */
  for (loc = req->locations; loc != NULL; loc = ptr)
    {
      ptr = loc->next;
      if (0 != strcmp (loc->url->scheme, "sip")
	  && 0 != strcmp (loc->url->scheme, "sips"))
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_WARNING, NULL,
		       "sfp module: Location should be a sip url!\n"));
	  REMOVE_ELEMENT (req->locations, loc);
	  location_free (loc);
	}
    }
  
  /* If the plugins don't give any location, the usual process
     is made. */
  if (req->locations == NULL)
    {
      osip_route_t *route;
      osip_route_t *route2;
      osip_uri_t *dest_url;
      osip_uri_t *url;
      osip_message_get_route (request, 0, &route);
      osip_message_get_route (request, 1, &route2);
      if (route == NULL)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "sfp module: no valid location given?\n"));
	  req->inc_tr = NULL;
	  req->state = PROXY_CLOSING;
	  return;
	}
      else if (0 != psp_core_is_responsible_for_this_route (route->url))
	dest_url = route->url;
      else if (route2 != NULL)
	dest_url = route2->url;
      else
	dest_url = request->req_uri;
      
      if (0 == psp_core_is_responsible_for_this_route (dest_url))
	{ /* too much route pointing on me, may be some kind of attack? */
	  req->inc_tr = NULL;
	  req->state = PROXY_CLOSING;
	  return;
	}

      if (0 == psp_core_is_responsible_for_this_route (dest_url))
	{ /* too much route pointing on me, may be some kind of attack? */
	  req->inc_tr = NULL;
	  req->state = PROXY_CLOSING;
	  return ;
	}
      i = osip_uri_clone (dest_url, &url);
      if (i != 0)
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				  "sfp module: Could not clone request-uri!\n"));
	  req->inc_tr = NULL;
	  req->state = PROXY_CLOSING;
	  return ;
	}
      i = location_init (&loc, url, 3600);
      if (i != 0)
	{
	  osip_uri_free (url);
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_BUG, NULL,
				  "sfp module: Could not create location info!\n"));
	  req->inc_tr = NULL;
	  req->state = PROXY_CLOSING;
	  return ;
	}
      ADD_ELEMENT (req->locations, loc);
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
			      "sfp module: location guessed from route headers!\n"));
    }

  if (MSG_IS_ACK (request))
    {
      sfp_forward_ack (req);
      return;
    }

  /* create a new branch for all possible locations */
  for (loc = req->locations; loc != NULL; loc = ptr)
    {
      int is_an_ip;
      struct in6_addr addr;
      ptr = loc->next;
      is_an_ip = ppl_inet_pton ((const char *)loc->url->host, (void *)&addr);
      if (is_an_ip == -1)
	{
	  /* This case should never occur? */
	  REMOVE_ELEMENT (req->locations, loc);
	  location_free(loc);
  	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO1, NULL,
		       "sfp module: Address family not supported?\n"));
	}
      else if (is_an_ip == 0)
	{
	  psp_core_event_resolv_add_query (osip_strdup (loc->url->host));
	}
      else
	{
	  i = sfp_create_branch(req, loc, NULL);
	  if (i!=0)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_ERROR, NULL,
			   "module sfp: Failed to create branch\n"));
	    }
	  REMOVE_ELEMENT (req->locations, loc);
	  location_free (loc);
	}
    }
  req->state = PROXY_CALLING;
}

void proxy_timeout_try_new_location (psp_request_t * req, psp_event_t * evt)
{
  location_t *loc;
  location_t *ptr;

  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "proxyfsm: proxy_timeout_try_new_location evt=%x\n", evt));

  /* create a new branch for all possible locations */
  for (loc = req->locations; loc != NULL; loc = ptr)
    {
      int is_an_ip;
      struct in6_addr addr;
      ptr = loc->next;
      is_an_ip = ppl_inet_pton ((const char *)loc->url->host, (void *)&addr);
      if (is_an_ip == -1)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_BUG, NULL,
		       "module sfp: The bad location should be already removed!\n"));
	  REMOVE_ELEMENT (req->locations, loc);
	  location_free(loc);
	}
      else if (is_an_ip == 0)
	{
	  ppl_dns_entry_t *dns_result;
	  int i;
	  i = ppl_dns_get_result (&dns_result, loc->url->host);
	  if (i != 0)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO3, NULL,
			   "module sfp: Keep waiting for the resolution to succeed...!\n"));
	      
	    }
	  else
	    {
	      /* The resolver can provide several IPs for the same location.
	         This is a major issue here: What should I do: starting a
	         transactions for each IP does not seems to be reasonnable.
	         (forking should be done on several locations, not
	         on one location that resolved to many IPs. In the case
	         where a host is resolved to many IPs, I sould try the
	         first one and use the other ones as fallbacks.
	      */
	      int i = sfp_create_branch(req, loc, dns_result->dns_ips);
	      /* in all cases, (failure or success), this location
	         has been tried and will never be tried again */
	      REMOVE_ELEMENT (req->locations, loc);
	      location_free (loc);
	      if (i!=0)
		{
		  OSIP_TRACE (osip_trace
			      (__FILE__, __LINE__, OSIP_ERROR, NULL,
			       "module sfp: Failed to create branch\n"));
		}
	    }
	}
      else
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_BUG, NULL,
		       "module sfp: The correct location should be already removed!\n"));
	  REMOVE_ELEMENT (req->locations, loc);
	  location_free (loc);
	}
    }  
}

void proxy_start_fallback_locations (psp_request_t * req, psp_event_t * evt)
{
  location_t *loc;
  location_t *ptr;
  int i;

  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "proxyfsm: proxy_start_fallback_locations evt=%x\n", evt));

#if 0
  /* TODO: it woud be better to cancel all previous transaction */

  /* cancel all answered pending branch */
  for (br = req->branch_answered;
       br != NULL;
       br = ptr)
    {
      ptr = br->next;
      if (br->out_tr==NULL)
	{}
      else if (br->out_tr->last_response==NULL)
	{}
      else if (br->out_tr->last_response->status_code<200)
	{
	  sfp_cancel_pending_branch(req, br);
	}
    }
#endif
    
  /* We may have other location to try: */
  for (loc = req->fallback_locations; loc != NULL; loc = ptr)
    {
      int is_an_ip;
      struct in6_addr addr;
      ptr = loc->next;
      is_an_ip = ppl_inet_pton ((const char *)loc->url->host, (void *)&addr);
      if (is_an_ip == -1)
	{
	  /* This case should never occur? */
	  REMOVE_ELEMENT (req->fallback_locations, loc);
	  location_free(loc);
  	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO1, NULL,
		       "sfp module: Address family not supported?\n"));
	}
      else if (is_an_ip == 0)
	{
	  psp_core_event_resolv_add_query (osip_strdup (loc->url->host));
	}
      else
	{
	  i = sfp_create_branch(req, loc, NULL);
	  if (i!=0)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_ERROR, NULL,
			   "module sfp: Failed to create branch\n"));
	    }
	  REMOVE_ELEMENT (req->fallback_locations, loc);
	  location_free (loc);
	}
    }

  /* return to PROXY_CALLING state */
  req->state = PROXY_CALLING;

  /* restart the timer */
  gettimeofday(&req->timer_noanswer_start, NULL);
  add_gettimeofday(&req->timer_noanswer_start, req->timer_noanswer_length);
  
  /* stop the timer for no final answer */
  req->timer_nofinalanswer_start.tv_sec=-1;
  
  /* stop the timer for no final answer */
  req->timer_close_start.tv_sec=-1;
}

void proxy_timeout_noanswer (psp_request_t * req, psp_event_t * evt)
{
  sfp_branch_t *tmp = NULL;
  sfp_branch_t *br;
  sfp_branch_t *ptr;
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "proxyfsm: proxy_timeout_noanswer evt=%x\n", evt));
  
  /* If there was some fallback address, then some final answers may
       have been received. */
  for (br = req->branch_answered;
       br != NULL;
       br = ptr)
    {
      ptr = br->next;
      if (br->out_tr==NULL)
	{}
	else if (br->out_tr->last_response==NULL)
	  {}
      else if (br->out_tr->last_response->status_code<299)
	{}
      else if (br->out_tr->last_response->status_code>br->last_code_forwarded)
	{
	  /* try to forward the new response */
	  br->last_code_forwarded = br->out_tr->last_response->status_code;
	  if (tmp==NULL ||
	      br->out_tr->last_response->status_code
	      <tmp->out_tr->last_response->status_code)
	    tmp = br;
	  REMOVE_ELEMENT(req->branch_answered, br);
	  ADD_ELEMENT (req->branch_cancelled, br);
	}
    }

  /* cancel all answered pending branch in all cases */
  for (br = req->branch_answered;
       br != NULL;
       br = ptr)
    {
      ptr = br->next;
      if (br->out_tr==NULL)
	  {}
      else if (br->out_tr->last_response==NULL)
	{}
      else if (br->out_tr->last_response->status_code<200)
	{
	  sfp_cancel_pending_branch(req, br);
	  REMOVE_ELEMENT(req->branch_answered, br);
	  ADD_ELEMENT (req->branch_cancelled, br);
	}
    }

  /* stop & start the timers */
  req->timer_noanswer_start.tv_sec=-1;
  req->timer_nofinalanswer_start.tv_sec=-1;
  
  gettimeofday(&req->timer_close_start, NULL);
  add_gettimeofday(&req->timer_close_start, req->timer_close_length);
  
  if (tmp!=NULL) /* one transaction was previously answered */
  {
    sfp_forward_response (req, tmp);
  
    req->state = PROXY_CLOSING;
    return;
  }

  /* no final answer has been received for any transaction */
  req->uas_status = 408; /* Timeout */
  sfp_answer_request(req);
  req->state = PROXY_CLOSING;
}

void proxy_timeout_nofinalanswer (psp_request_t * req, psp_event_t * evt)
{
  sfp_branch_t *br;
  sfp_branch_t *ptr;
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "proxyfsm: proxy_timeout_nofinalanswer evt=%x\n", evt));

  /* cancel all answered pending branch, and wait for a final answer */
  for (br = req->branch_answered;
       br != NULL;
       br = ptr)
    {
      ptr = br->next;
      if (br->out_tr==NULL)
	{}
      else if (br->out_tr->last_response==NULL)
	{}
      else if (br->out_tr->last_response->status_code<200)
	{
	  sfp_cancel_pending_branch(req, br);
	}
    }

  /* stop the timer for no answer */
  req->timer_noanswer_start.tv_sec=-1;

  /* stop the timer for no answer */
  req->timer_nofinalanswer_start.tv_sec=-1;

  /* start the timer for closing the response context */
  gettimeofday(&req->timer_close_start, NULL);
  add_gettimeofday(&req->timer_close_start, req->timer_close_length);
}

void proxy_timeout_close (psp_request_t * req, psp_event_t * evt)
{
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "proxyfsm: proxy_timeout_close evt=%x\n", evt));

  if (req->state==PROXY_PROCEEDING)
    {
      /* no final answer (but CANCEL was sent!) */
      req->uas_status = 408; /* Timeout */
      sfp_answer_request(req);
      
      /* stop & start the timers */
      req->timer_noanswer_start.tv_sec=-1;
      req->timer_nofinalanswer_start.tv_sec=-1;
      
      gettimeofday(&req->timer_close_start, NULL);
      add_gettimeofday(&req->timer_close_start, req->timer_close_length);
      
      req->state = PROXY_CLOSING;
    }
  else
    {
      /* time to close the request */
      req->state = PROXY_CLOSED;  
    }
}

void proxy_rcv_1xx (psp_request_t * req, psp_event_t * evt)
{
  sfp_branch_t *br;
  sfp_branch_t *ptr;
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "proxyfsm: proxy_rcv_1xx evt=%x\n", evt));

  for (br = req->branch_answered;
       br != NULL;
       br = ptr)
    {
      ptr = br->next;
      if (br->out_tr==NULL)
	{}
      else if (br->out_tr->last_response==NULL)
	{}
      else if (!MSG_IS_STATUS_1XX(br->out_tr->last_response))
	{}
      else if (br->out_tr->last_response->status_code>br->last_code_forwarded)
	{
	  /* try to forward the new response */
	  br->last_code_forwarded = br->out_tr->last_response->status_code;
	  if (br->last_code_forwarded!=100) /* do not forward 100 Trying */
	    sfp_forward_response (req, br);
	}
    }

  for (br = req->branch_notanswered;
       br != NULL;
       br = ptr)
    {
      ptr = br->next;
      if (br->out_tr==NULL)
	{}
      else if (br->out_tr->last_response==NULL)
	{}
      else if (!MSG_IS_STATUS_1XX(br->out_tr->last_response))
	{}
      else if (br->out_tr->last_response->status_code>br->last_code_forwarded)
	{
	  /* try to forward the new response */
	  br->last_code_forwarded = br->out_tr->last_response->status_code;
	  if (br->last_code_forwarded!=100) /* do not forward 100 Trying */
	    sfp_forward_response (req, br);
	  REMOVE_ELEMENT(req->branch_notanswered, br);
	  ADD_ELEMENT (req->branch_answered, br);
	}
    }

  /* start the timer for no final answer */
  gettimeofday(&req->timer_nofinalanswer_start, NULL);
  add_gettimeofday(&req->timer_nofinalanswer_start, req->timer_nofinalanswer_length);
  /* stop the timer for no answer */
  req->timer_noanswer_start.tv_sec=-1;

  req->state = PROXY_PROCEEDING;
}

void proxy_rcv_2xx (psp_request_t * req, psp_event_t * evt)
{
  sfp_branch_t *br;
  sfp_branch_t *ptr;
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "proxyfsm: proxy_rcv_2xx evt=%x\n", evt));

  for (br = req->branch_answered;
       br != NULL;
       br = ptr)
    {
      ptr = br->next;
      if (br->out_tr==NULL)
	{}
      else if (br->out_tr->last_response==NULL)
	{}
      else if (!MSG_IS_STATUS_2XX(br->out_tr->last_response))
	{}
      else if (br->out_tr->last_response->status_code>br->last_code_forwarded)
	{
	  /* try to forward the new response */
	  br->last_code_forwarded = br->out_tr->last_response->status_code;
	  sfp_forward_response (req, br);
	  REMOVE_ELEMENT(req->branch_answered, br);
	  ADD_ELEMENT (req->branch_completed, br);
	}
    }

  for (br = req->branch_notanswered;
       br != NULL;
       br = ptr)
    {
      ptr = br->next;
      if (br->out_tr==NULL)
	{}
      else if (br->out_tr->last_response==NULL)
	{}
      else if (!MSG_IS_STATUS_2XX(br->out_tr->last_response))
	{}
      else if (br->out_tr->last_response->status_code>br->last_code_forwarded)
	{
	  /* try to forward the new response */
	  br->last_code_forwarded = br->out_tr->last_response->status_code;
	  sfp_forward_response (req, br);
	  REMOVE_ELEMENT(req->branch_notanswered, br);
	  ADD_ELEMENT (req->branch_completed, br);
	}
    }

  /* cancel all answered pending branch */
  for (br = req->branch_answered;
       br != NULL;
       br = ptr)
    {
      ptr = br->next;
      if (br->out_tr==NULL)
	{}
      else if (br->out_tr->last_response==NULL)
	{}
      else if (br->out_tr->last_response->status_code<200)
	{
	  sfp_cancel_pending_branch(req, br);
	  REMOVE_ELEMENT(req->branch_answered, br);
	  ADD_ELEMENT (req->branch_cancelled, br);
	}
    }

  /* stop & start the timers */
  req->timer_noanswer_start.tv_sec=-1;
  req->timer_nofinalanswer_start.tv_sec=-1;
  
  gettimeofday(&req->timer_close_start, NULL);
  add_gettimeofday(&req->timer_close_start, req->timer_close_length);

  req->state = PROXY_CLOSING;
}

void proxy_rcv_3456xx (psp_request_t * req, psp_event_t * evt)
{
  sfp_branch_t *tmp = NULL;
  sfp_branch_t *br;
  sfp_branch_t *ptr;
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "proxyfsm: proxy_rcv_3456xx evt=%x\n", evt));

  if (req->branch_completed!=NULL) /* a 2xx has been aready sent */
    {
      /* 200 ok already sent for another branch */
      return;
    }

  for (br = req->branch_answered;
       br != NULL;
       br = ptr)
    {
      ptr = br->next;
      if (br->out_tr==NULL)
	{}
      else if (br->out_tr->last_response==NULL)
	{}
      else if (br->out_tr->last_response->status_code<299)
	{}
      else if (br->out_tr->last_response->status_code>br->last_code_forwarded)
	{
	  /* try to forward the new response */
	  br->last_code_forwarded = br->out_tr->last_response->status_code;
	  if (tmp==NULL ||
	      br->out_tr->last_response->status_code
	      <tmp->out_tr->last_response->status_code)
	    tmp = br;
	  REMOVE_ELEMENT(req->branch_answered, br);
	  ADD_ELEMENT (req->branch_cancelled, br);
	}
    }
  
  for (br = req->branch_notanswered;
       br != NULL;
       br = ptr)
    {
      ptr = br->next;
      if (br->out_tr==NULL)
	{}
      else if (br->out_tr->last_response==NULL)
	{}
      else if (br->out_tr->last_response->status_code<299)
	{}
      else if (br->out_tr->last_response->status_code>br->last_code_forwarded)
	{
	  /* try to forward the new response */
	  br->last_code_forwarded = br->out_tr->last_response->status_code;
	  if (tmp==NULL ||
	      br->out_tr->last_response->status_code
	      <tmp->out_tr->last_response->status_code)
	    tmp = br;
	  REMOVE_ELEMENT(req->branch_notanswered, br);
	  ADD_ELEMENT (req->branch_cancelled, br);
	}
    }
  
  sfp_forward_response (req, tmp);

  /* cancel all answered pending branch */
  for (br = req->branch_answered;
       br != NULL;
       br = ptr)
    {
      ptr = br->next;
      if (br->out_tr==NULL)
	{}
      else if (br->out_tr->last_response==NULL)
	{}
      else if (br->out_tr->last_response->status_code<200)
	{
	  sfp_cancel_pending_branch(req, br);
	  REMOVE_ELEMENT(req->branch_answered, br);
	  ADD_ELEMENT (req->branch_cancelled, br);
	}
    }

  /* stop & start the timers */
  req->timer_noanswer_start.tv_sec=-1;
  req->timer_nofinalanswer_start.tv_sec=-1;
  
  gettimeofday(&req->timer_close_start, NULL);
  add_gettimeofday(&req->timer_close_start, req->timer_close_length);

  req->state = PROXY_CLOSING;
}

void proxy_rcv_cancel (psp_request_t * req, psp_event_t * evt)
{
  sfp_branch_t *br;
  sfp_branch_t *ptr;
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "proxyfsm: proxy_rcv_cancel evt=%x\n", evt));

  /* cancel all answered pending branch, and wait for a final answer */
  for (br = req->branch_answered;
       br != NULL;
       br = ptr)
    {
      ptr = br->next;
      if (br->out_tr==NULL)
	{}
      else if (br->out_tr->last_response==NULL)
	{}
      else if (br->out_tr->last_response->status_code<200)
	{
	  sfp_cancel_pending_branch(req, br);
	}
    }

  /* stop the timer for no answer */
  req->timer_noanswer_start.tv_sec=-1;

  /* stop the timer for no answer */
  req->timer_nofinalanswer_start.tv_sec=-1;

  /* start the timer for closing the response context */
  gettimeofday(&req->timer_close_start, NULL);
  add_gettimeofday(&req->timer_close_start, req->timer_close_length);
}
