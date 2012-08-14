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

extern psp_core_t *core;

/************ PLUGIN REGISTRATION API *******************/


PPL_DECLARE (int)
psp_core_load_tlp_plugin (tlp_plugin_t ** p, psp_plugin_t * psp, int flag)
{
  tlp_plugin_t *plug;
  int i;

  *p = NULL;
  i = tlp_plugin_init (&plug, psp, flag);
  if (i != 0)
    return -1;
  *p = plug;
  psp_core_add_tlp_plugin (plug);
  return 0;
}

PPL_DECLARE (int) psp_core_add_tlp_plugin (tlp_plugin_t * plug)
{
  ADD_ELEMENT (core->tlp->tlp_plugins, plug);
  return 0;
}


PPL_DECLARE (int)
psp_core_load_sfp_plugin (sfp_plugin_t ** p, psp_plugin_t * psp)
{
  sfp_plugin_t *plug;
  int i;

  *p = NULL;
  i = sfp_plugin_init (&plug, psp);
  if (i != 0)
    return -1;
  *p = plug;
  psp_core_add_sfp_plugin (plug);
  return 0;
}

PPL_DECLARE (int) psp_core_add_sfp_plugin (sfp_plugin_t * plug)
{
  ADD_ELEMENT (core->sfp->sfp_plugins, plug);
  return 0;
}

/************ HOOK REGISTRATION API *********************/

static int
psp_core_add_sfp_inc_hook (sfp_inc_func_tab_t * tab, sfp_inc_func_t * fn, int hookflg)
{
  switch (hookflg)
    {
    case PSP_HOOK_REALLY_FIRST:
      APPEND_ELEMENT (sfp_inc_func_t, tab->func_hook_really_first, fn);
      break;
    case PSP_HOOK_FIRST:
      APPEND_ELEMENT (sfp_inc_func_t, tab->func_hook_first, fn);
      break;
    case PSP_HOOK_LAST:
      APPEND_ELEMENT (sfp_inc_func_t, tab->func_hook_last, fn);
      break;
    case PSP_HOOK_REALLY_LAST:
      APPEND_ELEMENT (sfp_inc_func_t, tab->func_hook_really_last, fn);
      break;
    case PSP_HOOK_FINAL:
      APPEND_ELEMENT (sfp_inc_func_t, tab->func_hook_final, fn);
      break;
    case PSP_HOOK_MIDDLE:
    default:
      APPEND_ELEMENT (sfp_inc_func_t, tab->func_hook_middle, fn);
      break;
    }
  return 0;
}


PPL_DECLARE (int) psp_core_add_sfp_inc_invite_hook (sfp_inc_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_inc_hook (core->sfp->rcv_invites, fn, hookflg);
}

PPL_DECLARE (int) psp_core_add_sfp_inc_ack_hook (sfp_inc_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_inc_hook (core->sfp->rcv_acks, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_inc_register_hook (sfp_inc_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_inc_hook (core->sfp->rcv_registers, fn, hookflg);
}

PPL_DECLARE (int) psp_core_add_sfp_inc_bye_hook (sfp_inc_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_inc_hook (core->sfp->rcv_byes, fn, hookflg);
}

PPL_DECLARE (int) psp_core_add_sfp_inc_options_hook (sfp_inc_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_inc_hook (core->sfp->rcv_optionss, fn, hookflg);
}

PPL_DECLARE (int) psp_core_add_sfp_inc_info_hook (sfp_inc_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_inc_hook (core->sfp->rcv_infos, fn, hookflg);
}

PPL_DECLARE (int) psp_core_add_sfp_inc_cancel_hook (sfp_inc_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_inc_hook (core->sfp->rcv_cancels, fn, hookflg);
}

PPL_DECLARE (int) psp_core_add_sfp_inc_notify_hook (sfp_inc_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_inc_hook (core->sfp->rcv_notifys, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_inc_subscribe_hook (sfp_inc_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_inc_hook (core->sfp->rcv_subscribes, fn, hookflg);
}

PPL_DECLARE (int) psp_core_add_sfp_inc_unknown_hook (sfp_inc_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_inc_hook (core->sfp->rcv_unknowns, fn, hookflg);
}

/***SFP***/
static int
psp_core_add_sfp_fwd_hook (sfp_fwd_func_tab_t * tab, sfp_fwd_func_t * fn,
			   int hookflg)
{
  switch (hookflg)
    {
    case PSP_HOOK_REALLY_FIRST:
      APPEND_ELEMENT (sfp_fwd_func_t, tab->func_hook_really_first, fn);
      break;
    case PSP_HOOK_FIRST:
      APPEND_ELEMENT (sfp_fwd_func_t, tab->func_hook_first, fn);
      break;
    case PSP_HOOK_LAST:
      APPEND_ELEMENT (sfp_fwd_func_t, tab->func_hook_last, fn);
      break;
    case PSP_HOOK_REALLY_LAST:
      APPEND_ELEMENT (sfp_fwd_func_t, tab->func_hook_really_last, fn);
      break;
    case PSP_HOOK_MIDDLE:
    default:
      APPEND_ELEMENT (sfp_fwd_func_t, tab->func_hook_middle, fn);
      break;
    }
  return 0;
}

static int
psp_core_add_sfp_rcv_hook (sfp_rcv_func_tab_t * tab, sfp_rcv_func_t * fn,
			   int hookflg)
{
  switch (hookflg)
    {
    case PSP_HOOK_REALLY_FIRST:
      APPEND_ELEMENT (sfp_rcv_func_t, tab->func_hook_really_first, fn);
      break;
    case PSP_HOOK_FIRST:
      APPEND_ELEMENT (sfp_rcv_func_t, tab->func_hook_first, fn);
      break;
    case PSP_HOOK_LAST:
      APPEND_ELEMENT (sfp_rcv_func_t, tab->func_hook_last, fn);
      break;
    case PSP_HOOK_REALLY_LAST:
      APPEND_ELEMENT (sfp_rcv_func_t, tab->func_hook_really_last, fn);
      break;
    case PSP_HOOK_MIDDLE:
    default:
      APPEND_ELEMENT (sfp_rcv_func_t, tab->func_hook_middle, fn);
      break;
    }
  return 0;
}

static int
psp_core_add_sfp_snd_hook (sfp_snd_func_tab_t * tab, sfp_snd_func_t * fn,
			   int hookflg)
{
  switch (hookflg)
    {
    case PSP_HOOK_REALLY_FIRST:
      APPEND_ELEMENT (sfp_snd_func_t, tab->func_hook_really_first, fn);
      break;
    case PSP_HOOK_FIRST:
      APPEND_ELEMENT (sfp_snd_func_t, tab->func_hook_first, fn);
      break;
    case PSP_HOOK_LAST:
      APPEND_ELEMENT (sfp_snd_func_t, tab->func_hook_last, fn);
      break;
    case PSP_HOOK_REALLY_LAST:
      APPEND_ELEMENT (sfp_snd_func_t, tab->func_hook_really_last, fn);
      break;
    case PSP_HOOK_MIDDLE:
    default:
      APPEND_ELEMENT (sfp_snd_func_t, tab->func_hook_middle, fn);
      break;
    }
  return 0;
}

PPL_DECLARE (int)
psp_core_add_sfp_fwd_invite_hook (sfp_fwd_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_fwd_hook (core->sfp->fwd_invites, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_fwd_ack_hook (sfp_fwd_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_fwd_hook (core->sfp->fwd_acks, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_fwd_register_hook (sfp_fwd_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_fwd_hook (core->sfp->fwd_registers, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_fwd_bye_hook (sfp_fwd_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_fwd_hook (core->sfp->fwd_byes, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_fwd_options_hook (sfp_fwd_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_fwd_hook (core->sfp->fwd_optionss, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_fwd_info_hook (sfp_fwd_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_fwd_hook (core->sfp->fwd_infos, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_fwd_cancel_hook (sfp_fwd_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_fwd_hook (core->sfp->fwd_cancels, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_fwd_notify_hook (sfp_fwd_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_fwd_hook (core->sfp->fwd_notifys, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_fwd_subscribe_hook (sfp_fwd_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_fwd_hook (core->sfp->fwd_subscribes, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_fwd_unknown_hook (sfp_fwd_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_fwd_hook (core->sfp->fwd_unknowns, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_rcv_1xx_hook (sfp_rcv_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_rcv_hook (core->sfp->rcv_1xxs, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_rcv_2xx_hook (sfp_rcv_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_rcv_hook (core->sfp->rcv_2xxs, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_rcv_3xx_hook (sfp_rcv_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_rcv_hook (core->sfp->rcv_3xxs, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_rcv_4xx_hook (sfp_rcv_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_rcv_hook (core->sfp->rcv_4xxs, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_rcv_5xx_hook (sfp_rcv_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_rcv_hook (core->sfp->rcv_5xxs, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_rcv_6xx_hook (sfp_rcv_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_rcv_hook (core->sfp->rcv_6xxs, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_snd_1xx_hook (sfp_snd_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_snd_hook (core->sfp->snd_1xxs, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_snd_2xx_hook (sfp_snd_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_snd_hook (core->sfp->snd_2xxs, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_snd_3xx_hook (sfp_snd_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_snd_hook (core->sfp->snd_3xxs, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_snd_4xx_hook (sfp_snd_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_snd_hook (core->sfp->snd_4xxs, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_snd_5xx_hook (sfp_snd_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_snd_hook (core->sfp->snd_5xxs, fn, hookflg);
}

PPL_DECLARE (int)
psp_core_add_sfp_snd_6xx_hook (sfp_snd_func_t * fn, int hookflg)
{
  return psp_core_add_sfp_snd_hook (core->sfp->snd_6xxs, fn, hookflg);
}

