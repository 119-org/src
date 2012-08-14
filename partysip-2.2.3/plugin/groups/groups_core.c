/*
  The groups plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The groups plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The groups plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <partysip/partysip.h>

#include "groups.h"

/* this structure is retreived by the core application
   with dlsym */
psp_plugin_t PSP_PLUGIN_DECLARE_DATA groups_plugin;

/* element shared with the core application layer */
/* all xxx_plugin elements MUST be DYNAMICLY ALLOCATED
   as the core application will assume they are */
sfp_plugin_t *groups_plug;
char groups_name_config[50];

extern groups_ctx_t *groups_context;

/* this is called by the core application when the module
   is loaded (the address of this method is located in
   the structure groups_plugin->plugin_init() */
PSP_PLUGIN_DECLARE(int) plugin_init(char *name_config)
{
  sfp_inc_func_t *fn;
  int i;
  /* plugin MUST create their own structure and give them
     back to the core by calling:
     psp_core_load_xxx_plugin();
     where xxx is the module name related to the plugin.
     psp_plugins can have more than one xxx_plugins attached
  */
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
	      "groups plugin: plugin_init()!\n"));

  /* default config name */
#ifndef WIN32
  if (name_config==NULL)
    snprintf(groups_name_config, 49, "groups");
  else
    snprintf(groups_name_config, 49, name_config);
#else
  if (name_config==NULL)
    _snprintf(groups_name_config, 49, "groups");
  else
    _snprintf(groups_name_config, 49, name_config);
#endif

  i = groups_ctx_init();
  if (i!=0)
    goto pi_error1;

  psp_plugin_take_ownership(&groups_plugin);
  i = psp_core_load_sfp_plugin(&groups_plug, &groups_plugin);
  if (i!=0)
    goto pi_error2;

  /* INIT HOOK METHOD */
  i = sfp_inc_func_init(&fn, &cb_groups_search_location,
		    groups_plugin.plug_id);
  if (i!=0) goto pi_error3;
  i = psp_core_add_sfp_inc_invite_hook(fn, PSP_HOOK_FIRST);
  if (i!=0) goto pi_error3;

  i = sfp_inc_func_init(&fn, &cb_groups_search_location,
		    groups_plugin.plug_id);
  if (i!=0) goto pi_error3;
  i = psp_core_add_sfp_inc_ack_hook(fn, PSP_HOOK_FIRST);
  if (i!=0) goto pi_error3;

  i = sfp_inc_func_init(&fn, &cb_groups_search_location,
		    groups_plugin.plug_id);
  if (i!=0) goto pi_error3;
  i = psp_core_add_sfp_inc_register_hook(fn, PSP_HOOK_FIRST);
  if (i!=0) goto pi_error3;

  i = sfp_inc_func_init(&fn, &cb_groups_search_location,
		    groups_plugin.plug_id);
  if (i!=0) goto pi_error3;
  i = psp_core_add_sfp_inc_bye_hook(fn, PSP_HOOK_FIRST);
  if (i!=0) goto pi_error3;

  i = sfp_inc_func_init(&fn, &cb_groups_search_location,
		    groups_plugin.plug_id);
  if (i!=0) goto pi_error3;
  i = psp_core_add_sfp_inc_options_hook(fn, PSP_HOOK_FIRST);
  if (i!=0) goto pi_error3;

  i = sfp_inc_func_init(&fn, &cb_groups_search_location,
		    groups_plugin.plug_id);
  if (i!=0) goto pi_error3;
  i = psp_core_add_sfp_inc_info_hook(fn, PSP_HOOK_FIRST);
  if (i!=0) goto pi_error3;

  i = sfp_inc_func_init(&fn, &cb_groups_search_location,
		    groups_plugin.plug_id);
  if (i!=0) goto pi_error3;
  i = psp_core_add_sfp_inc_cancel_hook(fn, PSP_HOOK_FIRST);
  if (i!=0) goto pi_error3;

  i = sfp_inc_func_init(&fn, &cb_groups_search_location,
		    groups_plugin.plug_id);
  if (i!=0) goto pi_error3;
  i = psp_core_add_sfp_inc_notify_hook(fn, PSP_HOOK_FIRST);
  if (i!=0) goto pi_error3;

  i = sfp_inc_func_init(&fn, &cb_groups_search_location,
		    groups_plugin.plug_id);
  if (i!=0) goto pi_error3;
  i = psp_core_add_sfp_inc_subscribe_hook(fn, PSP_HOOK_FIRST);
  if (i!=0) goto pi_error3;

  i = sfp_inc_func_init(&fn, &cb_groups_search_location,
		    groups_plugin.plug_id);
  if (i!=0) goto pi_error3;
  i = psp_core_add_sfp_inc_unknown_hook(fn, PSP_HOOK_FIRST);
  if (i!=0) goto pi_error3;

  return 0;

 pi_error3:
 pi_error2:
  groups_ctx_free();
 pi_error1:
  return -1;
}

PSP_PLUGIN_DECLARE(int) plugin_start()
{
  /*  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
      "groups plugin: plugin_start()!\n"));
  */
  return -1;
}

PSP_PLUGIN_DECLARE(int) plugin_release()
{
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
	      "groups plugin: plugin_release()!\n"));
  groups_ctx_free();
  groups_context=NULL;
  return 0;
}

psp_plugin_t PSP_PLUGIN_DECLARE_DATA groups_plugin = 
{
  0,  /* uninitialized */
  "Plugin for hunting groups definitions.",
  "0.5.2",
  "This plugin can be used to define urls used to call several members.",
  PLUGIN_SFP,
  0,     /* number of owners is always 0 at the begining */
  NULL,  /* future place for the dso_handle */
  &plugin_init,
  &plugin_start,
  &plugin_release
};
