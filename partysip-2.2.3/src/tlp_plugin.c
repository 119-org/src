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

int
tlp_plugin_init (tlp_plugin_t ** plug, psp_plugin_t * psp, int flag)
{
  *plug = (tlp_plugin_t *) osip_malloc (sizeof (tlp_plugin_t));
  if (*plug == NULL)
    return -1;

  (*plug)->psp_plugin = psp;
  (*plug)->plug_id = psp->plug_id;
  (*plug)->in_socket = -1;
  (*plug)->out_socket = -1;
  (*plug)->mcast_socket = -1;
  (*plug)->flag = flag;		/* PLUG_TCP, PLUG_UDP, PLUG_RELIABLE, PLUG_UNRELIABLE */

  (*plug)->rcv_func = NULL;
  (*plug)->snd_func = NULL;

  (*plug)->next = NULL;
  (*plug)->parent = NULL;
  return 0;
}

void
tlp_plugin_free (tlp_plugin_t * plug)
{
  if (plug == NULL)
    return;
  psp_plugin_free (plug->psp_plugin);
  /*  THIS IS NOT A DYMAMIC ELEMENT!!  + THIS IS ATTACHED TO SEVERAL
     PSP_PLUGIN... so the next line will never be called
     osip_free(plug->psp_plugin); */
  /*  Already closed by the plugin itself.
    if (plug->in_socket != -1)
    ppl_socket_close (plug->in_socket);
  */
  plug->in_socket = -1;
  osip_free (plug->rcv_func);
  osip_free (plug->snd_func);
  osip_free (plug);
}


PPL_DECLARE (int)
tlp_plugin_set_input_socket (tlp_plugin_t * plug, int socket)
{
  if (plug == NULL)
    return -1;
  plug->in_socket = socket;
  return 0;
}

PPL_DECLARE (int)
tlp_plugin_set_output_socket (tlp_plugin_t * plug, int socket)
{
  if (plug == NULL)
    return -1;
  plug->out_socket = socket;
  return 0;
}

PPL_DECLARE (int)
tlp_plugin_set_mcast_socket (tlp_plugin_t * plug, int socket)
{
  if (plug == NULL)
    return -1;
  plug->mcast_socket = socket;
  return 0;
}

PPL_DECLARE (int)
tlp_plugin_set_rcv_hook (tlp_plugin_t * plug, tlp_rcv_func_t * fn)
{
  if (plug == NULL)
    return -1;
  plug->rcv_func = fn;
  return 0;
}

PPL_DECLARE (int)
tlp_plugin_set_snd_hook (tlp_plugin_t * plug, tlp_snd_func_t * fn)
{
  if (plug == NULL)
    return -1;
  plug->snd_func = fn;
  return 0;
}


PPL_DECLARE (int) tlp_rcv_func_init (tlp_rcv_func_t ** elt, int (*cb_rcv_func) (int), int plug_id)	/* number of element to get */
{
  *elt = (tlp_rcv_func_t *) osip_malloc (sizeof (tlp_rcv_func_t));
  if (*elt == NULL)
    return -1;
  (*elt)->cb_rcv_func = cb_rcv_func;
  (*elt)->plug_id = plug_id;
  return 0;
}

PPL_DECLARE (int) tlp_snd_func_init (tlp_snd_func_t ** elt,
				     int (*cb_snd_func) (osip_transaction_t *,	/* read-only */
							 osip_message_t *,	/* read-only? */
							 char *,	/* destination host or NULL */
							 int,	/* destination port */
							 int), int plug_id)	/* socket to use (or -1) */
{
  *elt = (tlp_snd_func_t *) osip_malloc (sizeof (tlp_snd_func_t));
  if (*elt == NULL)
    return -1;
  (*elt)->cb_snd_func = cb_snd_func;
  (*elt)->plug_id = plug_id;
  return 0;
}
