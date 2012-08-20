/*
  The auth plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The auth plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The auth plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <partysip/partysip.h>

#include "auth.h"

/* this structure is retreived by the core application
   with dlsym */
psp_plugin_t PSP_PLUGIN_DECLARE_DATA auth_plugin;

/* element shared with the core application layer */
/* all xxx_plugin elements MUST be DYNAMICLY ALLOCATED
   as the core application will assume they are */
sfp_plugin_t *auth_plugin2;

extern auth_ctx_t *auth_context;

/* this is called by the core application when the module
   is loaded (the address of this method is located in
   the structure auth_plugin->plugin_init() */
PSP_PLUGIN_DECLARE (int)
plugin_init (char *name_config)
{
  char *noauth;
  sfp_inc_func_t *fn;
  sfp_snd_func_t *fn2;
  int i;

  /* plugin MUST create their own structure and give them
     back to the core by calling:
     psp_core_load_xxx_plugin();
     where xxx is the module name related to the plugin.
     psp_plugins can have more than one xxx_plugins attached
   */
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
			  "auth plugin: plugin_init()!\n"));

  i = auth_ctx_init ();
  if (i != 0)
    goto pi_error1;

  /* verify we actually want to do authentication */
  noauth = psp_config_get_element ("authentication");
  if (noauth == NULL)
    {
    }
  else if (0 == strcmp (noauth, "off"))
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "auth plugin: Authentication is turned off!\n"));
      psp_plugin_take_ownership (&auth_plugin);
      i = psp_core_load_sfp_plugin (&auth_plugin2, &auth_plugin);
      if (i != 0)
	goto pi_error4;
      return 0;
    }
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			  "auth plugin: Authentication is turned on!\n"));

  psp_plugin_take_ownership (&auth_plugin);
  i = psp_core_load_sfp_plugin (&auth_plugin2, &auth_plugin);
  if (i != 0)
    goto pi_error4;

  /* INIT HOOK METHOD FOR IMP */
  i = sfp_inc_func_init (&fn, &cb_auth_validate_credentials, auth_plugin.plug_id);
  if (i != 0)
    goto pi_error5;
  i = psp_core_add_sfp_inc_invite_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error5;

  i = sfp_inc_func_init (&fn, &cb_auth_validate_credentials, auth_plugin.plug_id);
  if (i != 0)
    goto pi_error5;
  i = psp_core_add_sfp_inc_register_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error5;

  i = sfp_inc_func_init (&fn, &cb_auth_validate_credentials, auth_plugin.plug_id);
  if (i != 0)
    goto pi_error5;
  i = psp_core_add_sfp_inc_ack_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error5;

  i = sfp_inc_func_init (&fn, &cb_auth_validate_credentials, auth_plugin.plug_id);
  if (i != 0)
    goto pi_error5;
  i = psp_core_add_sfp_inc_bye_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error5;

  i = sfp_inc_func_init (&fn, &cb_auth_validate_credentials, auth_plugin.plug_id);
  if (i != 0)
    goto pi_error5;
  i = psp_core_add_sfp_inc_options_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error5;

  i = sfp_inc_func_init (&fn, &cb_auth_validate_credentials, auth_plugin.plug_id);
  if (i != 0)
    goto pi_error5;
  i = psp_core_add_sfp_inc_info_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error5;

  i = sfp_inc_func_init (&fn, &cb_auth_validate_credentials, auth_plugin.plug_id);
  if (i != 0)
    goto pi_error5;
  i = psp_core_add_sfp_inc_cancel_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error5;

  i = sfp_inc_func_init (&fn, &cb_auth_validate_credentials, auth_plugin.plug_id);
  if (i != 0)
    goto pi_error5;
  i = psp_core_add_sfp_inc_notify_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error5;

  i = sfp_inc_func_init (&fn, &cb_auth_validate_credentials, auth_plugin.plug_id);
  if (i != 0)
    goto pi_error5;
  i = psp_core_add_sfp_inc_subscribe_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error5;

  i = sfp_inc_func_init (&fn, &cb_auth_validate_credentials, auth_plugin.plug_id);
  if (i != 0)
    goto pi_error5;
  i = psp_core_add_sfp_inc_unknown_hook (fn, PSP_HOOK_REALLY_FIRST);
  if (i != 0)
    goto pi_error5;


  /* INIT HOOK METHOD FOR SFP */
  i = sfp_snd_func_init (&fn2, &cb_auth_add_credentials, auth_plugin.plug_id);
  if (i != 0)
    goto pi_error5;
  i = psp_core_add_sfp_snd_4xx_hook (fn2, PSP_HOOK_MIDDLE);
  if (i != 0)
    goto pi_error5;

  return 0;


  /* TODO */
pi_error5:
pi_error4:
  auth_ctx_free ();
  auth_context = NULL;
pi_error1:
  return -1;
}

PSP_PLUGIN_DECLARE (int) plugin_start ()
{
  /*  Really useless!
     OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
     "auth plugin: plugin_start()!\n"));
   */

  return -1;
}

PSP_PLUGIN_DECLARE (int) plugin_release ()
{
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			  "auth plugin: plugin_release()!\n"));
  auth_ctx_free ();
  auth_context = NULL;
  return 0;
}

psp_plugin_t PSP_PLUGIN_DECLARE_DATA auth_plugin = {
  0,				/* uninitialized */
  "Auth plugin",
  "0.5.1",
  "plugin with authentication capabilities",
  PLUGIN_SFP,
  0,				/* number of owners is always 0 at the begining */
  NULL,				/* future place for the dso_handle */
  &plugin_init,
  &plugin_start,
  &plugin_release
};
