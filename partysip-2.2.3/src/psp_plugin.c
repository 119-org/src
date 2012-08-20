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


#include <psp_plug.h>

#ifndef WIN32
#define PSP_SERVER_INSTALL_DIR PSP_SERVER_PREFIX
#endif

const char PLUGIN_PATH[13] = "partysip";

#define MAX_FILENAME_LEN 255

/* WAIT HERE!!!
   This method DOES NOT DYNAMICLY ALLOCATE THE psp_plugin_t structure.
   The structure is an external variable of the plugin and retreived
   by dlsym() ??
*/
int
psp_plugin_load (psp_plugin_t ** plug, char *name, char *name_config)
{
  static int plug_id = 0;
  char *full_path;
  ppl_dso_handle_t *dso_handle;
  ppl_dso_handle_sym_t sym;
  int i;

  plug_id++;
  *plug = NULL;

  full_path = (char *) osip_malloc (MAX_FILENAME_LEN + 1);
#ifdef WIN32
  _snprintf (full_path, MAX_FILENAME_LEN, "./psp_%s.dll", name);
#else
  snprintf (full_path, MAX_FILENAME_LEN,
	    PSP_SERVER_INSTALL_DIR "/lib/%s/libpsp_%s.so", PLUGIN_PATH, name);
#endif
  i = ppl_dso_load (&dso_handle, full_path);
  if (i != 0)
    {
      printf ("ERROR: could not load plugin (%s): %s\n", full_path,
	      dso_handle->errormsg);
      osip_free (full_path);
      return -1;
    }

  /* get the %s_plugin element (this is an element of type psp_plugin_t) */
#ifdef WIN32
  _snprintf (full_path, MAX_FILENAME_LEN, "%s_plugin", name);
#else
  snprintf (full_path, MAX_FILENAME_LEN, "%s_plugin", name);
#endif
  i = ppl_dso_sym (&sym, dso_handle, full_path);
  osip_free (full_path);
  if (i != 0)
    {
      ppl_dso_unload (dso_handle);
      return -1;
    }
  *plug = (psp_plugin_t *) sym;	/* we got it! */
  (*plug)->dso_handle = dso_handle;

  i = ppl_dso_sym (&sym, dso_handle, "plugin_init");
  if (i != 0)
    {
      ppl_dso_unload (dso_handle);
      *plug = NULL;
      return -1;
    }
  (*plug)->plugin_init = sym;

  i = ppl_dso_sym (&sym, dso_handle, "plugin_start");
  if (i != 0)
    {
      ppl_dso_unload (dso_handle);
      *plug = NULL;
      return -1;
    }
  (*plug)->plugin_start = sym;

  i = ppl_dso_sym (&sym, dso_handle, "plugin_release");
  if (i != 0)
    {
      ppl_dso_unload (dso_handle);
      *plug = NULL;
      return -1;
    }
  (*plug)->plugin_release = sym;

  /* set the uique plugin id */
  (*plug)->plug_id = plug_id;
  /* it's time to autoconfigure the plugin by calling plugin_init()
   */
  i = (*plug)->plugin_init (name_config);
  if (i != 0)
    {
      ppl_dso_unload ((*plug)->dso_handle);
      *plug = NULL;
      return -1;
    }
  return 0;
}

int
psp_plugin_is_not_busy (psp_plugin_t * plug)
{
  if (plug == NULL)
    return -1;
  if (plug->owners == 0)
    return 0;
  return -1;
}

PPL_DECLARE (int) psp_plugin_take_ownership (psp_plugin_t * plug)
{
  if (plug == NULL)
    return -1;
  (plug->owners)++;
  return 0;
}

int
psp_plugin_release_ownership (psp_plugin_t * plug)
{
  if (plug == NULL)
    return -1;
  (plug->owners)--;
  return 0;
}

void
psp_plugin_free (psp_plugin_t * plug)
{
  ppl_dso_handle_sym_t sym;
  int i;
  if (plug == NULL)
    return;
  psp_plugin_release_ownership (plug);	/* is it called here? */
  if (0 != psp_plugin_is_not_busy (plug))
    {
      return;
    }
  /* there is no xxx_plugin related to this psp_plugin */

  /* relocation issue: recalculate */
  i = ppl_dso_sym (&sym, plug->dso_handle, "plugin_release");
  if (i != 0)
    {
      return ;
    }
  plug->plugin_release = sym;
  plug->plugin_release ();
#ifndef WIN32
  {
    ppl_dso_handle_t *dso_handle;
    dso_handle = plug->dso_handle;
    ppl_dso_unload (plug->dso_handle);
  }
#endif
  /* osip_free(plug); */
}

int
psp_plugin_set_plug_id (psp_plugin_t * plug, int plug_id)
{
  if (plug == NULL)
    return -1;
  plug->plug_id = plug_id;
  return 0;
}

int
psp_plugin_get_plug_id (psp_plugin_t * plug)
{
  if (plug == NULL)
    return -1;
  return plug->plug_id;
}

char *
psp_plugin_get_name (psp_plugin_t * plug)
{
  if (plug == NULL)
    return NULL;
  return plug->name;
}

char *
psp_plugin_get_version (psp_plugin_t * plug)
{
  if (plug == NULL)
    return NULL;
  return plug->version;
}

char *
psp_plugin_get_description (psp_plugin_t * plug)
{
  if (plug == NULL)
    return NULL;
  return plug->description;
}
