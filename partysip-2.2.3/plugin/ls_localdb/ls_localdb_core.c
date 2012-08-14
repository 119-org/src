/*
  The ls_localdb plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The ls_localdb plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The ls_localdb plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <partysip/partysip.h>

#include "ls_localdb.h"

/* this structure is retreived by the core application
   with dlsym */
psp_plugin_t PSP_PLUGIN_DECLARE_DATA ls_localdb_plugin;

/* element shared with the core application layer */
/* all xxx_plugin elements MUST be DYNAMICLY ALLOCATED
   as the core application will assume they are */
sfp_plugin_t *localdb_plug;
char localdb_name_config[50];

extern ls_localdb_ctx_t *ls_localdb_context;

/* this is called by the core application when the module
   is loaded (the address of this method is located in
   the structure ls_localdb_plugin->plugin_init() */
PSP_PLUGIN_DECLARE (int)
plugin_init (char *name_config)
{
  sfp_inc_func_t *fn;
  int i;

  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
			  "ls_localdb plugin: plugin_init()!\n"));

#ifndef WIN32
  if (name_config==NULL)
    snprintf(localdb_name_config, 49, "localdb");
  else
    snprintf(localdb_name_config, 49, name_config);
#else
  if (name_config==NULL)
    _snprintf(localdb_name_config, 49, "localdb");
  else
    _snprintf(localdb_name_config, 49, name_config);
#endif

  i = ls_localdb_ctx_init ();
  if (i != 0)
    goto pi_error1;

  psp_plugin_take_ownership (&ls_localdb_plugin);
  i = psp_core_load_sfp_plugin (&localdb_plug, &ls_localdb_plugin);
  if (i != 0)
    goto pi_error2;

  /* INIT HOOK METHOD */
  i = sfp_inc_func_init (&fn, &cb_ls_localdb_search_user_location,
		     ls_localdb_plugin.plug_id);
  if (i != 0)
    goto pi_error3;
  i = psp_core_add_sfp_inc_invite_hook (fn, PSP_HOOK_LAST);
  if (i != 0)
    goto pi_error3;

  i = sfp_inc_func_init (&fn, &cb_ls_localdb_search_user_location,
		     ls_localdb_plugin.plug_id);
  if (i != 0)
    goto pi_error3;
  i = psp_core_add_sfp_inc_ack_hook (fn, PSP_HOOK_LAST);
  if (i != 0)
    goto pi_error3;

  i = sfp_inc_func_init (&fn, &cb_ls_localdb_search_user_location,
		     ls_localdb_plugin.plug_id);
  if (i != 0)
    goto pi_error3;
  i = psp_core_add_sfp_inc_register_hook (fn, PSP_HOOK_LAST);
  if (i != 0)
    goto pi_error3;

  i = sfp_inc_func_init (&fn, &cb_ls_localdb_search_user_location,
		     ls_localdb_plugin.plug_id);
  if (i != 0)
    goto pi_error3;
  i = psp_core_add_sfp_inc_bye_hook (fn, PSP_HOOK_LAST);
  if (i != 0)
    goto pi_error3;

  i = sfp_inc_func_init (&fn, &cb_ls_localdb_search_user_location,
		     ls_localdb_plugin.plug_id);
  if (i != 0)
    goto pi_error3;
  i = psp_core_add_sfp_inc_options_hook (fn, PSP_HOOK_LAST);
  if (i != 0)
    goto pi_error3;

  i = sfp_inc_func_init (&fn, &cb_ls_localdb_search_user_location,
		     ls_localdb_plugin.plug_id);
  if (i != 0)
    goto pi_error3;
  i = psp_core_add_sfp_inc_info_hook (fn, PSP_HOOK_LAST);
  if (i != 0)
    goto pi_error3;

  i = sfp_inc_func_init (&fn, &cb_ls_localdb_search_user_location,
		     ls_localdb_plugin.plug_id);
  if (i != 0)
    goto pi_error3;
  i = psp_core_add_sfp_inc_cancel_hook (fn, PSP_HOOK_LAST);
  if (i != 0)
    goto pi_error3;

  i = sfp_inc_func_init (&fn, &cb_ls_localdb_search_user_location,
		     ls_localdb_plugin.plug_id);
  if (i != 0)
    goto pi_error3;
  i = psp_core_add_sfp_inc_notify_hook (fn, PSP_HOOK_LAST);
  if (i != 0)
    goto pi_error3;

  i = sfp_inc_func_init (&fn, &cb_ls_localdb_search_user_location,
		     ls_localdb_plugin.plug_id);
  if (i != 0)
    goto pi_error3;
  i = psp_core_add_sfp_inc_subscribe_hook (fn, PSP_HOOK_LAST);
  if (i != 0)
    goto pi_error3;

  i = sfp_inc_func_init (&fn, &cb_ls_localdb_search_user_location,
		     ls_localdb_plugin.plug_id);
  if (i != 0)
    goto pi_error3;
  i = psp_core_add_sfp_inc_unknown_hook (fn, PSP_HOOK_LAST);
  if (i != 0)
    goto pi_error3;

  return 0;


  /* TODO */
pi_error3:
pi_error2:
  ls_localdb_ctx_free ();
pi_error1:
  return -1;
}

PSP_PLUGIN_DECLARE (int) plugin_start ()
{
  /*  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
     "ls_localdb plugin: plugin_start()!\n"));
   */
  return -1;
}

PSP_PLUGIN_DECLARE (int) plugin_release ()
{
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
			  "ls_localdb plugin: plugin_release()!\n"));
  ls_localdb_ctx_free ();
  ls_localdb_context = NULL;
  return 0;
}

psp_plugin_t PSP_PLUGIN_DECLARE_DATA ls_localdb_plugin = {
  0,				/* uninitialized */
  "Location search plugin",
  "0.5.1",
  "plugin for location search in local cache.",
  PLUGIN_SFP,
  0,				/* number of owners is always 0 at the begining */
  NULL,				/* future place for the dso_handle */
  &plugin_init,
  &plugin_start,
  &plugin_release
};
