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

#include <psp_request.h>


int
sfp_branch_init (sfp_branch_t ** sfp_branch, char *branch)
{
  static int id = 0;
  *sfp_branch = (sfp_branch_t *) osip_malloc (sizeof (sfp_branch_t));
  if (*sfp_branch == NULL)
    return -1;
  memset(*sfp_branch, 0, sizeof(sfp_branch_t));
  (*sfp_branch)->already_cancelled = -1;
  (*sfp_branch)->branchid = id;
  (*sfp_branch)->ctx_start = time (NULL);
  (*sfp_branch)->timer_c = 180;
  (*sfp_branch)->ctx_timeout = TIMEOUT_FOR_NO_ANSWER; /* this is the default delay? */
  return 0;
}

void
sfp_branch_free (sfp_branch_t * branch)
{
  ppl_dns_ip_t *f;
  ppl_dns_ip_t *fnext;

  if (branch == NULL)
    return;

  osip_uri_free (branch->url);

  for (f = branch->fallbacks; f != NULL; f = fnext)
    {
      fnext = f->next;
      REMOVE_ELEMENT (branch->fallbacks, f);
      ppl_dns_ip_free (f);
    }

  if (branch->out_tr == NULL)
    {
    }
  else if (branch->out_tr->state == ICT_TERMINATED
	   || branch->out_tr->state == NICT_TERMINATED)
    {
      osip_transaction_free2 (branch->out_tr);
    }
  else
    {
      osip_transaction_free (branch->out_tr);
    }

  /* This is not a copy of the original request...
     osip_message_free (branch->request);
   */
  /*
     osip_message_free (branch->response);
   */
  osip_free(branch);
}

int
sfp_branch_get_last_code_forwarded (sfp_branch_t * sfp)
{
  if (sfp == NULL)
    return -1;
  return sfp->last_code_forwarded;
}

int
sfp_branch_set_last_code_forwarded (sfp_branch_t * sfp, int lastcode)
{
  if (sfp == NULL)
    return -1;
  sfp->last_code_forwarded = lastcode;
  return 0;
}

int
sfp_branch_get_osip_id (sfp_branch_t * branch)
{
  if (branch == NULL || branch->out_tr == NULL)
    return -1;
  return branch->out_tr->transactionid;
}

int
sfp_branch_set_transaction (sfp_branch_t * branch, osip_transaction_t * out_tr)
{
  if (branch == NULL)
    return -1;
  branch->out_tr = out_tr;
  return 0;
}

int
sfp_branch_set_url (sfp_branch_t * branch, osip_uri_t * url)
{
  if (branch == NULL)
    return -1;
  branch->url = url;
  return 0;
}

int
sfp_branch_set_timeout (sfp_branch_t * branch, int delay)
{
  if (branch == NULL)
    return -1;
  branch->ctx_timeout = delay;
  return 0;
}

int
sfp_branch_is_expired (sfp_branch_t * branch)
{
  ppl_time_t now;

  if (branch == NULL)
    return -1;
  now = ppl_time ();
  if (now - branch->ctx_start >= branch->ctx_timeout)
    return 0;
  return -1;
}

int
sfp_branch_clone_and_set_fallbacks (sfp_branch_t * branch, ppl_dns_ip_t * ips)
{
  ppl_dns_ip_t *f;
  ppl_dns_ip_t *ip;
  int i;

  if (branch == NULL)
    return -1;
  for (ip = ips; ip != NULL; ip = ip->next)
    {
      i = ppl_dns_ip_clone (ip, &f);
      if (i == 0)
	{
	  ADD_ELEMENT (branch->fallbacks, f);
	}
    }
  return 0;
}

int
sfp_branch_set_request (sfp_branch_t * branch, osip_message_t * req)
{
  if (branch == NULL)
    return -1;
  branch->request = req;
  return 0;
}

int
sfp_branch_set_response (sfp_branch_t * branch, osip_message_t * resp)
{
  if (branch == NULL)
    return -1;
  branch->response = resp;
  return 0;
}

