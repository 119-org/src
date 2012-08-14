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

#include <partysip.h>
#include <psp_request.h>
#include "proxyfsm.h"

int
psp_request_init (psp_request_t ** req, osip_transaction_t * inc_tr, osip_message_t *ack)
{
  if (ack==NULL)
    {
      if (inc_tr==NULL)
	return -1;
      if (inc_tr->orig_request==NULL)
	return -1;
    }
  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "Allocating psp_req ressource!\n"));

  *req = (psp_request_t *) osip_malloc (sizeof (psp_request_t));
  if (*req == NULL)
    return -1;
  memset(*req, 0, sizeof(psp_request_t));

  if (ack==NULL)
    psp_core_sfp_generate_branch_for_request (inc_tr->orig_request,
					      (*req)->branch);
  else
    psp_core_sfp_generate_branch_for_request (ack, (*req)->branch);
  if ((*req)->branch[0] == '\0') { osip_free(*req); return -1; }

  (*req)->owners = 0;
  (*req)->state = PROXY_PRE_CALLING;
  (*req)->start = time (NULL);
  (*req)->flag = 0;
  (*req)->uas_status = 0;

  (*req)->timer_noanswer_length = TIMEOUT_FOR_NO_ANSWER * 1000;
  gettimeofday(&(*req)->timer_noanswer_start, NULL);
  add_gettimeofday(&(*req)->timer_noanswer_start, (*req)->timer_noanswer_length);

  (*req)->timer_nofinalanswer_length = TIMEOUT_FOR_NONINVITE * 1000;
  if (ack==NULL && inc_tr->ist_context!=NULL)
    (*req)->timer_nofinalanswer_length = TIMEOUT_FOR_INVITE * 1000;

  (*req)->timer_nofinalanswer_start.tv_sec = -1;

  (*req)->timer_close_length = 32 * 1000;
  if (ack==NULL && inc_tr->ist_context!=NULL)
    (*req)->timer_close_length = 32 * 1000;

  (*req)->timer_close_start.tv_sec = -1;

  (*req)->locations = NULL;
  (*req)->fallback_locations = NULL;
  (*req)->inc_tr = inc_tr;
  (*req)->branch_index = 0;
  (*req)->branch_notanswered = NULL;
  (*req)->branch_answered    = NULL;
  (*req)->branch_completed   = NULL;
  (*req)->branch_cancelled   = NULL;
  (*req)->out_response = NULL;
  (*req)->inc_ack = ack;
  (*req)->next = NULL;
  (*req)->parent = NULL;
  return 0;
}

void
psp_request_free (psp_request_t * req)
{
  /*  plug_ctx_t *c; */
  location_t *loc;
  sfp_branch_t *br;

  if (req == NULL)
    return;

  if (req->owners != 0)
    return;			/* delete later */

  if (req->inc_tr == NULL)
    {
    }
  else if (req->inc_tr->state == IST_TERMINATED
	   || req->inc_tr->state == NIST_TERMINATED)
    {
      osip_transaction_free2 (req->inc_tr);
    }
  else
    {
      osip_transaction_free (req->inc_tr);
    }

  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO1, NULL,
	       "free psp_req ressouce!\n"));
  for (loc = req->locations; loc != NULL; loc = req->locations)
    {
      REMOVE_ELEMENT (req->locations, loc);
      location_free (loc);
    }

  for (loc = req->fallback_locations; loc != NULL; loc = req->fallback_locations)
    {
      REMOVE_ELEMENT (req->fallback_locations, loc);
      location_free (loc);
    }

  for (br = req->branch_notanswered; br != NULL; br = req->branch_notanswered)
    {
      REMOVE_ELEMENT (req->branch_notanswered, br);
      sfp_branch_free(br);
    }
  for (br = req->branch_answered; br != NULL; br = req->branch_answered)
    {
      REMOVE_ELEMENT (req->branch_answered, br);
      sfp_branch_free(br);
    }
  for (br = req->branch_completed; br != NULL; br = req->branch_completed)
    {
      REMOVE_ELEMENT (req->branch_completed, br);
      sfp_branch_free(br);
    }
  for (br = req->branch_cancelled; br != NULL; br = req->branch_cancelled)
    {
      REMOVE_ELEMENT (req->branch_cancelled, br);
      sfp_branch_free(br);
    }
  osip_message_free(req->inc_ack);
  osip_free(req);
}

PPL_DECLARE (int) psp_request_set_mode (psp_request_t * req, int mode)
{
  if (req == NULL)
    return -1;
  req->flag = (req->flag & 0xFF0) | mode;	/* so we delete the first 4 bits */
  return 0;
}

PPL_DECLARE (int) psp_request_set_state (psp_request_t * req, int state)
{
  if (req == NULL)
    return -1;
  req->flag = (req->flag & 0xF0F) | state;	/* so we keep only the first 4 bits */
  return 0;
}

PPL_DECLARE (int) psp_request_set_property (psp_request_t * req, int property)
{
  if (req == NULL)
    return -1;
  req->flag = (req->flag & 0x0FF) | property;	/* so we delete the first 4 bits */
  return 0;
}

PPL_DECLARE (int) psp_request_set_uas_status (psp_request_t * req, int status)
{
  if (req == NULL)
    return -1;
  req->uas_status = status;
  return 0;
}

PPL_DECLARE (int) psp_request_get_uas_status (psp_request_t * req)
{
  if (req == NULL)
    return -1;
  return req->uas_status;
}

int
psp_request_take_ownership (psp_request_t * req)
{
  if (req == NULL)
    return -1;
  req->owners++;
  return 0;
}

int
psp_request_release_ownership (psp_request_t * req)
{
  if (req == NULL)
    return -1;
  req->owners--;
  return 0;
}


PPL_DECLARE (osip_message_t*) psp_request_get_request(psp_request_t * req)
{
  if (req==NULL)
    return NULL;
  if (req->inc_ack!=NULL)
    return req->inc_ack;
  if (req->inc_tr!=NULL)
    return req->inc_tr->orig_request;
  return NULL;
}

PPL_DECLARE (int) location_init (location_t ** loc, osip_uri_t * url, int expire)
{
  *loc = (location_t *) osip_malloc (sizeof (location_t));
  if (*loc == NULL)
    return -1;
  (*loc)->url = url;
  (*loc)->path = NULL;
  (*loc)->expire = expire;
  (*loc)->next = NULL;
  (*loc)->parent = NULL;
  return 0;
}

PPL_DECLARE (void) location_free (location_t * location)
{
  osip_uri_free (location->url);
  osip_free (location->path);
  osip_free (location);
}

PPL_DECLARE (int) location_set_url (location_t * location, osip_uri_t * url)
{
  if (location == NULL)
    return -1;
  location->url = url;
  return 0;
}

PPL_DECLARE (int) location_set_expires (location_t * location, int expire)
{
  if (location == NULL)
    return -1;
  location->expire = expire;
  return 0;
}

PPL_DECLARE (int) location_set_path (location_t * location, char *path)
{
  if (location == NULL)
    return -1;
  location->path = path;
  return 0;
}

PPL_DECLARE (int) location_clone (location_t * location, location_t ** loc)
{
  int i;
  osip_uri_t *url;

  *loc = NULL;
  if (location == NULL)
    return -1;
  i = osip_uri_clone (location->url, &url);
  if (i != 0)
    return -1;
  i = location_init (loc, url, location->expire);
  if (i != 0)
    goto lc_error1;
  return 0;
lc_error1:
  osip_uri_free (url);
  *loc = NULL;
  return -1;
}
