/*
  The udp plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The udp plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The udp plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <partysip/partysip.h>

#include "udp.h"

/* this structure is retreived by the core application
   with dlsym */
psp_plugin_t PSP_PLUGIN_DECLARE_DATA udp_plugin;

/* element shared with the core application layer */
/* all xxx_plugin elements MUST be DYNAMICLY ALLOCATED
   as the core application will assume they are */
tlp_plugin_t *udp_plug;

extern local_ctx_t *ctx;

/* this is called by the core application when the module
   is loaded (the address of this method is located in
   the structure upd_plugin->plugin_init() */
PSP_PLUGIN_DECLARE (int)
plugin_init (char *name_config)
{
  tlp_rcv_func_t *fn_rcv;
  tlp_snd_func_t *fn_snd;
  int flag = TLP_UDP;		/* ?? */
  int i;
  char *in_udp_port;

  /* plugin MUST create their own structure and give them
     back to the core by calling:
     psp_core_load_xxx_plugin();
     where xxx is the module name related to the plugin.
     psp_plugins can have more than one xxx_plugins attached
   */
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO3, NULL,
			  "udp plugin: plugin_init()!\n"));

  in_udp_port = psp_config_get_element ("serverport_udp");
  if (in_udp_port == NULL)
    i = local_ctx_init (5060, 5060);
  else
    {
      int p = atoi (in_udp_port);

      i = local_ctx_init (p, p);
    }
  if (i != 0)
    return -1;

  psp_plugin_take_ownership (&udp_plugin);
  i = psp_core_load_tlp_plugin (&udp_plug, &udp_plugin, flag);
  if (i != 0)
    goto pi_error1;		/* so the core application known initialization failed! */

  tlp_plugin_set_input_socket (udp_plug, ctx->in_socket);
  tlp_plugin_set_output_socket (udp_plug, ctx->out_socket);
  tlp_plugin_set_mcast_socket (udp_plug, ctx->mcast_socket);

  /* add hook */
  i = tlp_rcv_func_init (&fn_rcv, &cb_rcv_udp_message, udp_plugin.plug_id);
  if (i != 0)
    goto pi_error2;		/* so the core application known initialization failed! */
  i = tlp_snd_func_init (&fn_snd, &cb_snd_udp_message, udp_plugin.plug_id);
  if (i != 0)
    goto pi_error3;		/* so the core application known initialization failed! */

  i = tlp_plugin_set_rcv_hook (udp_plug, fn_rcv);
  if (i != 0)
    goto pi_error4;		/* so the core application known initialization failed! */
  i = tlp_plugin_set_snd_hook (udp_plug, fn_snd);
  if (i != 0)
    goto pi_error4;		/* so the core application known initialization failed! */

  /* from this point, partysip can hook from this plugin */
  return 0;

  /* TODO */
pi_error4:
pi_error3:
pi_error2:
pi_error1:
  local_ctx_free ();
  return -1;
}

PSP_PLUGIN_DECLARE (int) plugin_start ()
{
  /* OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO3,NULL,
     "udp plugin: plugin_start()!\n")); */

  return -1;
}

PSP_PLUGIN_DECLARE (int) plugin_release ()
{
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO3, NULL,
			  "udp plugin: plugin_release()!\n"));
  local_ctx_free ();
  ctx = NULL;
  return 0;
}

psp_plugin_t PSP_PLUGIN_DECLARE_DATA udp_plugin = {
  0,				/* uninitialized */
  "Udp plugin",
  "0.5.1",
  "plugin for receiving and sending UDP message",
  PLUGIN_TLP,
  0,				/* number of owners is always 0 at the begining */
  NULL,				/* future place for the dso_handle */
  &plugin_init,
  &plugin_start,
  &plugin_release
};
