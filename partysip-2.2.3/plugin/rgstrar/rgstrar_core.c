/*
  The rgstrar plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The rgstrar plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The rgstrar plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <partysip/partysip.h>

#include "rgstrar.h"

/* this structure is retreived by the core application
   with dlsym */
psp_plugin_t PSP_PLUGIN_DECLARE_DATA rgstrar_plugin;

/* element shared with the core application layer */
/* all xxx_plugin elements MUST be DYNAMICLY ALLOCATED
   as the core application will assume they are */
sfp_plugin_t *rgstrar_plugin2;

extern rgstrar_ctx_t *rgstrar_context;

/* this is called by the core application when the module
   is loaded (the address of this method is located in
   the structure rgstrar_plugin->plugin_init() */
PSP_PLUGIN_DECLARE (int)
plugin_init (char *name_config)
{
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
			  "rgstrar plugin: plugin_init()!\n"));

  i = rgstrar_ctx_init ();
  if (i != 0)
    goto pi_error1;

  psp_plugin_take_ownership (&rgstrar_plugin);
  i = psp_core_load_sfp_plugin (&rgstrar_plugin2, &rgstrar_plugin);
  if (i != 0)
    goto pi_error3;

  /* INIT HOOK METHOD FOR IMP */
  i = sfp_inc_func_init (&fn, &cb_rgstrar_update_contact_list,
			 rgstrar_plugin.plug_id);
  if (i != 0)
    goto pi_error4;
  i = psp_core_add_sfp_inc_register_hook (fn, PSP_HOOK_FIRST);
  if (i != 0)
    goto pi_error4;

  /* INIT HOOK METHOD FOR SFP */
  i = sfp_snd_func_init (&fn2, &cb_rgstrar_add_contacts_in_register,
			 rgstrar_plugin.plug_id);
  if (i != 0)
    goto pi_error4;
  i = psp_core_add_sfp_snd_2xx_hook (fn2, PSP_HOOK_MIDDLE);
  if (i != 0)
    goto pi_error4;

  /* from this point, partysip can hook from this plugin */
  return 0;


  /* TODO */
pi_error4:
pi_error3:
  rgstrar_ctx_free ();
  rgstrar_context = NULL;
pi_error1:
  return -1;
}

PSP_PLUGIN_DECLARE (int) plugin_start ()
{
  /*  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO2,NULL,
     "rgstrar plugin: plugin_start()!\n"));
   */
  return -1;
}

PSP_PLUGIN_DECLARE (int) plugin_release ()
{
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
			  "rgstrar plugin: plugin_release()!\n"));
  rgstrar_ctx_free ();
  rgstrar_context = NULL;
  return 0;
}

psp_plugin_t PSP_PLUGIN_DECLARE_DATA rgstrar_plugin = {
  0,				/* uninitialized */
  "Rgstrar plugin",
  "0.5.1",
  "plugin with registration capabilities",
  PLUGIN_SFP,
  0,				/* number of owners is always 0 at the begining */
  NULL,				/* future place for the dso_handle */
  &plugin_init,
  &plugin_start,
  &plugin_release
};
