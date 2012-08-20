/*
  The syntax plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The syntax plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The syntax plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <partysip/partysip.h>

#include "syntax.h"

/* this structure is retreived by the core application
   with dlsym */
psp_plugin_t PSP_PLUGIN_DECLARE_DATA syntax_plugin;

char supported_schemes[200];

/* element shared with the core application layer */
/* all xxx_plugin elements MUST be DYNAMICLY ALLOCATED
   as the core application will assume they are */
sfp_plugin_t *syntax_sfp_plug;

/* this is called by the core application when the module
   is loaded (the address of this method is located in
   the structure syntax_plugin->plugin_init() */
PSP_PLUGIN_DECLARE (int)
plugin_init (char *name_config)
{
  sfp_inc_func_t *fn;
  sfp_snd_func_t *fn2;
  int i;

  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
			  "syntax plugin: plugin_init()!\n"));

  psp_plugin_take_ownership (&syntax_plugin);
  i = psp_core_load_sfp_plugin (&syntax_sfp_plug, &syntax_plugin);
  if (i != 0)
    return -1;

  {
    config_element_t *elem;
    elem = psp_config_get_sub_element ("allowed_schemes", "syntax", NULL);
    memset (supported_schemes, 0, 200);
    if (elem == NULL || elem->value == NULL || strlen (elem->value) > 199)
      {
	osip_strncpy (supported_schemes, "sip,sips", 9);
      }
    else
      {
	osip_strncpy (supported_schemes, elem->value, strlen (elem->value));
      }
  }

  /* INIT HOOK METHOD */
  i = sfp_inc_func_init (&fn, &cb_check_syntax_in_request, syntax_plugin.plug_id);
  if (i != 0)
    goto pi_error1;
  i = psp_core_add_sfp_inc_invite_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error1;

  i = sfp_inc_func_init (&fn, &cb_check_syntax_in_request, syntax_plugin.plug_id);
  if (i != 0)
    goto pi_error1;
  i = psp_core_add_sfp_inc_ack_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error1;

  i = sfp_inc_func_init (&fn, &cb_check_syntax_in_request, syntax_plugin.plug_id);
  if (i != 0)
    goto pi_error1;
  i = psp_core_add_sfp_inc_register_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error1;

  i = sfp_inc_func_init (&fn, &cb_check_syntax_in_request, syntax_plugin.plug_id);
  if (i != 0)
    goto pi_error1;
  i = psp_core_add_sfp_inc_bye_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error1;

  i = sfp_inc_func_init (&fn, &cb_check_syntax_in_request, syntax_plugin.plug_id);
  if (i != 0)
    goto pi_error1;
  i = psp_core_add_sfp_inc_options_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error1;

  i = sfp_inc_func_init (&fn, &cb_check_syntax_in_request, syntax_plugin.plug_id);
  if (i != 0)
    goto pi_error1;
  i = psp_core_add_sfp_inc_info_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error1;

  i = sfp_inc_func_init (&fn, &cb_check_syntax_in_request, syntax_plugin.plug_id);
  if (i != 0)
    goto pi_error1;
  i = psp_core_add_sfp_inc_cancel_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error1;

  i = sfp_inc_func_init (&fn, &cb_check_syntax_in_request, syntax_plugin.plug_id);
  if (i != 0)
    goto pi_error1;
  i = psp_core_add_sfp_inc_notify_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error1;

  i = sfp_inc_func_init (&fn, &cb_check_syntax_in_request, syntax_plugin.plug_id);
  if (i != 0)
    goto pi_error1;
  i = psp_core_add_sfp_inc_subscribe_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error1;

  i = sfp_inc_func_init (&fn, &cb_check_syntax_in_request, syntax_plugin.plug_id);
  if (i != 0)
    goto pi_error1;
  i = psp_core_add_sfp_inc_unknown_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error1;

  /* SFP hooks */
  i = sfp_snd_func_init (&fn2, &cb_complete_answer_on_4xx, syntax_plugin.plug_id);
  if (i != 0)
    goto pi_error1;
  i = psp_core_add_sfp_snd_4xx_hook (fn2, PSP_HOOK_FIRST);
  if (i != 0)
    goto pi_error1;

  return 0;

pi_error1:
  return -1;
}

PSP_PLUGIN_DECLARE (int) plugin_start ()
{
  /* OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
     "syntax plugin: plugin_start()!\n")); */

  return -1;
}

PSP_PLUGIN_DECLARE (int) plugin_release ()
{
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
			  "syntax plugin: plugin_release()!\n"));
  return 0;
}

psp_plugin_t PSP_PLUGIN_DECLARE_DATA syntax_plugin = {
  0,				/* uninitialized */
  "Syntax analyser plugin",
  "0.5.1",
  "plugin with syntax checking capabilities",
  PLUGIN_SFP,
  0,				/* number of owners is always 0 at the begining */
  NULL,				/* future place for the dso_handle */
  &plugin_init,
  &plugin_start,
  &plugin_release
};
