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
module_init (module_t ** module, int flag, char *filename)
{
  (*module) = (module_t *) osip_malloc (sizeof (module_t));
  if ((*module) == NULL)
    return -1;
  (*module)->flag = flag;
  (*module)->thread = NULL;
  (*module)->filename = osip_strdup (filename);
  (*module)->wakeup = ppl_pipe ();
  if ((*module)->wakeup == NULL)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "plug_tpl cannot create pipe!\n"));
      osip_free ((*module)->filename);
      osip_free (*module);
      *module = NULL;
      return -1;
    }
  return 0;
}

void
module_free (module_t * module)
{
  int i;

  if (module == NULL)
    return;
  if (module->thread != NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_BUG, NULL,
		   "You must call module_release() before calling module_free()!\n"));
      return;
    }
  osip_free (module->filename);
  module->filename = NULL;
  i = ppl_pipe_close (module->wakeup);
  if (i == -1)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "could not close pipe!\n"));
    }
  module->wakeup = NULL;
  osip_free (module);
}

int
module_release (module_t * module)
{
  char q[2] = "q";
  int i;

  if (module == NULL)
    return -1;
  if ((~(module->flag) | ~MOD_THREAD) != ~MOD_THREAD)
    return 0;			/* not a thread */
  if (module->wakeup == NULL)
    return 0;			/* not yet started */

  i = ppl_pipe_write (module->wakeup, &q, 1);
  if (i != 1)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "could not write in pipe!\n"));
      return -1;
    }

  if (module->thread == NULL)
    return 0;			/* already stopped? */

  i = osip_thread_join (module->thread);
  if (i != 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "could not shutdown thread!\n"));
      return -1;
    }
  osip_free (module->thread);
  module->thread = NULL;
  return 0;
}

int
module_start (module_t * module, void *(*func_start) (void *), void *arg)
{
  if (module == NULL)
    return -1;
  if (func_start == NULL)
    return -1;
  if ((~(module->flag) | ~MOD_THREAD) == ~MOD_THREAD)
    {
      module->thread = osip_thread_create (20000, func_start, arg);
      if (module->thread == NULL)
	return -1;
    }
  return 0;
}

int
module_set_flag (module_t * module, int flag)
{
  if (module == NULL)
    return -1;
  module->flag = flag;
  return 0;
}

int
module_wakeup (module_t * module)
{
  int i;
  char q[2] = "w";

  if (module == NULL)
    return -1;
  i = ppl_pipe_write (module->wakeup, &q, 1);
  if (i != 1)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "could not write in pipe!\n"));
      perror ("error while writing in pipe");
      return -1;
    }
  return 0;
}
