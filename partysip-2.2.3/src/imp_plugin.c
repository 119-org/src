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

PPL_DECLARE (int)
sfp_inc_func_init (sfp_inc_func_t ** func, int (*fn) (psp_request_t *), int plug_id)
{
  *func = (sfp_inc_func_t *) osip_malloc (sizeof (sfp_inc_func_t));
  if (*func == NULL)
    return -1;

  (*func)->plug_id = plug_id;
  (*func)->cb_rcv_func = fn;
  (*func)->next = NULL;
  (*func)->parent = NULL;
  return 0;
}

PPL_DECLARE (void) sfp_inc_func_free (sfp_inc_func_t * func)
{
  if (func == NULL)
    return;
  osip_free(func);
}

int
sfp_inc_func_tab_init (sfp_inc_func_tab_t ** tab)
{
  *tab = (sfp_inc_func_tab_t *) osip_malloc (sizeof (sfp_inc_func_tab_t));
  if (*tab == NULL)
    return -1;

  (*tab)->func_hook_really_first = NULL;
  (*tab)->func_hook_first = NULL;
  (*tab)->func_hook_middle = NULL;
  (*tab)->func_hook_last = NULL;
  (*tab)->func_hook_really_last = NULL;
  (*tab)->func_hook_final = NULL;

  return 0;
}

void
sfp_inc_func_tab_free (sfp_inc_func_tab_t * tab)
{
  sfp_inc_func_t *f;

  if (tab == NULL)
    return;
  for (f = tab->func_hook_really_first; f != NULL;
       f = tab->func_hook_really_first)
    {
      tab->func_hook_really_first = f->next;
      sfp_inc_func_free (f);
    }
  for (f = tab->func_hook_first; f != NULL; f = tab->func_hook_first)
    {
      tab->func_hook_first = f->next;
      sfp_inc_func_free (f);
    }
  for (f = tab->func_hook_middle; f != NULL; f = tab->func_hook_middle)
    {
      tab->func_hook_middle = f->next;
      sfp_inc_func_free (f);
    }
  for (f = tab->func_hook_last; f != NULL; f = tab->func_hook_last)
    {
      tab->func_hook_last = f->next;
      sfp_inc_func_free (f);
    }
  for (f = tab->func_hook_really_last; f != NULL;
       f = tab->func_hook_really_last)
    {
      tab->func_hook_really_last = f->next;
      sfp_inc_func_free (f);
    }
  for (f = tab->func_hook_final; f != NULL;
       f = tab->func_hook_final)
    {
      tab->func_hook_final = f->next;
      sfp_inc_func_free (f);
    }
  osip_free (tab);
}

int
sfp_inc_func_tab_add_hook_really_first (sfp_inc_func_tab_t * tab, sfp_inc_func_t * fn)
{
  if (tab == NULL)
    return -1;
  ADD_ELEMENT (tab->func_hook_really_first, fn);
  return 0;
}

int
sfp_inc_func_tab_add_hook_first (sfp_inc_func_tab_t * tab, sfp_inc_func_t * fn)
{
  if (tab == NULL)
    return -1;
  ADD_ELEMENT (tab->func_hook_first, fn);
  return 0;
}


int
sfp_inc_func_tab_add_hook_middle (sfp_inc_func_tab_t * tab, sfp_inc_func_t * fn)
{
  if (tab == NULL)
    return -1;
  ADD_ELEMENT (tab->func_hook_middle, fn);
  return 0;
}

int
sfp_inc_func_tab_add_hook_last (sfp_inc_func_tab_t * tab, sfp_inc_func_t * fn)
{
  if (tab == NULL)
    return -1;
  ADD_ELEMENT (tab->func_hook_last, fn);
  return 0;
}

int
sfp_inc_func_tab_add_hook_really_last (sfp_inc_func_tab_t * tab, sfp_inc_func_t * fn)
{
  if (tab == NULL)
    return -1;
  ADD_ELEMENT (tab->func_hook_really_last, fn);
  return 0;
}

int
sfp_inc_func_tab_add_hook_final (sfp_inc_func_tab_t * tab, sfp_inc_func_t * fn)
{
  if (tab == NULL)
    return -1;
  ADD_ELEMENT (tab->func_hook_final, fn);
  return 0;
}
