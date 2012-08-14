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


#ifndef _PSP_PLUG_H
#define _PSP_PLUG_H

#include <ppl/ppl_dso.h>

#include <partysip/psp_macros.h>
#include <partysip/psp_request.h>

/* How to call plugins:
   1: detect which method or response code is used
   2: call the list of method in the PSP_HOOK_REALLY_FIRST list
   3: call the list of method in the PSP_HOOK_FIRST
   4: ...
   5: ...

   So: For performance reason, this list should be ready to use:
   table[0] for any REQUEST (other than INVITE)
   table[1] for INVITE
   table[2] for REGISTER
   table[3] for BYE
   ...

   table[1] contains all the hook related to INVITE:
   for irp:
*/

    /* Hook orderings */
/** run this hook first, before ANYTHING */
#define PSP_HOOK_REALLY_FIRST	(-10)
/** run this hook first */
#define PSP_HOOK_FIRST		0
/** run this hook somewhere */
#define PSP_HOOK_MIDDLE		10
/** run this hook after every other hook which is defined*/
#define PSP_HOOK_LAST		20
/** run this hook last, after EVERYTHING */
#define PSP_HOOK_REALLY_LAST	30

#define PSP_HOOK_FINAL		40


/********************************************************************/
/***************************** general  *****************************/

/* HOW PLUGINS ARE IMPLEMENTED:
   1: The core layer is asked to load a plugin named "$name"
   2: Do a dlopen of the shared library
   3: Do dlsym to access some methods:
       plugin_configure
       -> used to init internal state of plugin
       -> plugin store its "func_tab" in the *_plugin structure
       ....
*/
typedef struct psp_plugin_t psp_plugin_t;

struct psp_plugin_t
{
  int plug_id;
  char *name;
  char *version;
  char *description;

#define PLUGIN_TLP 0x01
#define PLUGIN_IMP 0x02
#define PLUGIN_UAP 0x04
#define PLUGIN_SFP 0x16
  int flag;
  /*... */

  int owners;			/* number of attached plugins */
  ppl_dso_handle_t *dso_handle;
  int (*plugin_init) ();
  int (*plugin_start) ();
  int (*plugin_release) ();
};

int psp_plugin_load (psp_plugin_t ** plugin, char *name, char *name_config);
void psp_plugin_free (psp_plugin_t * plugin);
PPL_DECLARE (int)
psp_plugin_take_ownership (psp_plugin_t * plugin);
     int psp_plugin_release_ownership (psp_plugin_t * plugin);
     int psp_plugin_is_not_busy (psp_plugin_t * plugin);
     int psp_plugin_set_plug_id (psp_plugin_t * plugin, int plug_id);

     int psp_plugin_get_plug_id (psp_plugin_t * plugin);
     char *psp_plugin_get_name (psp_plugin_t * plugin);
     char *psp_plugin_get_version (psp_plugin_t * plugin);
     char *psp_plugin_get_description (psp_plugin_t * plugin);


/********************************************************************/
/*****************************   tlp    *****************************/


     typedef struct tlp_rcv_func_t
     {
       int plug_id;		/* plug_id of the plugin */
       int (*cb_rcv_func) (int);	/* callback */
     }
tlp_rcv_func_t;

     typedef struct tlp_snd_func_t
     {
       int plug_id;		/* plug_id of the plugin */
       int (*cb_snd_func) (osip_transaction_t *, osip_message_t *, char *, int, int);
     }
tlp_snd_func_t;

PPL_DECLARE (int)
tlp_rcv_func_init (tlp_rcv_func_t ** elt, int (*cb_rcv_func) (int), int plug_id);	/* number of element to get */

PPL_DECLARE (int)
tlp_snd_func_init (tlp_snd_func_t ** elt, int (*cb_snd_func) (osip_transaction_t *,	/* read-only */
							      osip_message_t *,	/* read-only? */
							      char *,	/* destination host or NULL */
							      int,	/* destination port */
							      int), int plug_id);	/* socket to use (or -1) */

     typedef struct tlp_plugin_t tlp_plugin_t;

     struct tlp_plugin_t
     {
       psp_plugin_t *psp_plugin;
       int plug_id;
       int in_socket;
       int out_socket;
       int mcast_socket;

#define TLP_UDP        0x01
#define TLP_TCP        0x02
#define TLP_RELIABLE   0x04
#define TLP_UNRELIABLE 0x08
#define TLP_STCP       0x10

       int flag;		/* PLUG_{ TCP | UDP | RELIABLE | UNRELIABLE} */
       tlp_rcv_func_t *rcv_func;	/* methods to receive SIP MESSAGE (optionnal) */
       tlp_snd_func_t *snd_func;	/* methods to send SIP MESSAGE    (mandatory) */
       tlp_plugin_t *next;
       tlp_plugin_t *parent;
     };

     int tlp_plugin_init (tlp_plugin_t ** plugin, psp_plugin_t * psp_plugin,
			  int flag);
     void tlp_plugin_free (tlp_plugin_t * plugin);
PPL_DECLARE (int)
tlp_plugin_set_input_socket (tlp_plugin_t * plug, int socket);
PPL_DECLARE (int)
tlp_plugin_set_output_socket (tlp_plugin_t * plug, int socket);
PPL_DECLARE (int)
tlp_plugin_set_mcast_socket (tlp_plugin_t * plug, int socket);
PPL_DECLARE (int)
tlp_plugin_set_rcv_hook (tlp_plugin_t * plug, tlp_rcv_func_t * fn);
PPL_DECLARE (int)
tlp_plugin_set_snd_hook (tlp_plugin_t * plug, tlp_snd_func_t * fn);


/********************************************************************/
/*****************************   sfp   *****************************/

     typedef struct sfp_inc_func sfp_inc_func_t;

     struct sfp_inc_func
     {
       int plug_id;		/* plug_id of the plugin */
       int (*cb_rcv_func) (psp_request_t *);	/* callback */
       sfp_inc_func_t *parent;
       sfp_inc_func_t *next;
     };

PPL_DECLARE (int)
sfp_inc_func_init (sfp_inc_func_t ** func_tab, int (*func) (psp_request_t *),
		   int plug_id);
PPL_DECLARE (void)
sfp_inc_func_free (sfp_inc_func_t * func_tab);
     int sfp_inc_func_append_tab (sfp_inc_func_t * tab1, sfp_inc_func_t * tab2);

     typedef struct _sfp_inc_func_tab_t
     {
       sfp_inc_func_t *func_hook_really_first;
       sfp_inc_func_t *func_hook_first;
       sfp_inc_func_t *func_hook_middle;
       sfp_inc_func_t *func_hook_last;
       sfp_inc_func_t *func_hook_really_last;
       sfp_inc_func_t *func_hook_final;
     } sfp_inc_func_tab_t;

     int sfp_inc_func_tab_init (sfp_inc_func_tab_t ** func_tab);
     void sfp_inc_func_tab_free (sfp_inc_func_tab_t * func_tab);
     int sfp_inc_func_tab_register_hook (sfp_inc_func_tab_t * tab, sfp_inc_func_t * func,
					 int hook_flag);
     int sfp_inc_func_tab_add_hook_really_first (sfp_inc_func_tab_t * tab,
						 sfp_inc_func_t * fn);
     int sfp_inc_func_tab_add_hook_first (sfp_inc_func_tab_t * tab, sfp_inc_func_t * fn);
     int sfp_inc_func_tab_add_hook_middle (sfp_inc_func_tab_t * tab, sfp_inc_func_t * fn);
     int sfp_inc_func_tab_add_hook_really_last (sfp_inc_func_tab_t * tab,
						sfp_inc_func_t * fn);
     int sfp_inc_func_tab_add_hook_last (sfp_inc_func_tab_t * tab, sfp_inc_func_t * fn);
     int sfp_inc_func_tab_add_hook_final (sfp_inc_func_tab_t * tab, sfp_inc_func_t * fn);


     typedef struct sfp_fwd_func_t sfp_fwd_func_t;

     struct sfp_fwd_func_t
     {
       int plug_id;		/* plug_id of the plugin */
       int (*cb_fwd_request_func) (psp_request_t *);	/* callback */
       sfp_fwd_func_t *parent;
       sfp_fwd_func_t *next;
     };

     typedef struct sfp_rcv_func_t sfp_rcv_func_t;

     struct sfp_rcv_func_t
     {
       int plug_id;		/* plug_id of the plugin */
       int (*cb_rcv_answer_func) (psp_request_t *);	/* callback */
       sfp_rcv_func_t *parent;
       sfp_rcv_func_t *next;
     };

     typedef struct sfp_snd_func_t sfp_snd_func_t;

     struct sfp_snd_func_t
     {
       int plug_id;		/* plug_id of the plugin */
       int (*cb_snd_answer_func) (psp_request_t *, osip_message_t*); /* callback */
       sfp_snd_func_t *parent;
       sfp_snd_func_t *next;
     };

PPL_DECLARE (int)
sfp_fwd_func_init (sfp_fwd_func_t ** func_tab,
		   int (*fn) (psp_request_t *), int plug_id);
PPL_DECLARE (void)
sfp_fwd_func_free (sfp_fwd_func_t * func_tab);
     int sfp_fwd_func_append_tab (sfp_fwd_func_t * tab1,
				  sfp_fwd_func_t * tab2);

PPL_DECLARE (int)
sfp_rcv_func_init (sfp_rcv_func_t ** func_tab,
		   int (*fn) (psp_request_t *), int plug_id);
PPL_DECLARE (void)
sfp_rcv_func_free (sfp_rcv_func_t * func_tab);
     int sfp_rcv_func_append_tab (sfp_rcv_func_t * tab1,
				  sfp_rcv_func_t * tab2);

PPL_DECLARE (int)
sfp_snd_func_init (sfp_snd_func_t ** func_tab,
		   int (*fn) (psp_request_t *, osip_message_t*),
		   int plug_id);
PPL_DECLARE (void)
sfp_snd_func_free (sfp_snd_func_t * func_tab);
     int sfp_snd_func_append_tab (sfp_snd_func_t * tab1,
				  sfp_snd_func_t * tab2);

     typedef struct _sfp_fwd_func_tab_t
     {
       sfp_fwd_func_t *func_hook_really_first;
       sfp_fwd_func_t *func_hook_first;
       sfp_fwd_func_t *func_hook_middle;
       sfp_fwd_func_t *func_hook_last;
       sfp_fwd_func_t *func_hook_really_last;
     }
sfp_fwd_func_tab_t;

     typedef struct _sfp_rcv_func_tab_t
     {
       sfp_rcv_func_t *func_hook_really_first;
       sfp_rcv_func_t *func_hook_first;
       sfp_rcv_func_t *func_hook_middle;
       sfp_rcv_func_t *func_hook_last;
       sfp_rcv_func_t *func_hook_really_last;
     }
sfp_rcv_func_tab_t;

     typedef struct _sfp_snd_func_tab_t
     {
       sfp_snd_func_t *func_hook_really_first;
       sfp_snd_func_t *func_hook_first;
       sfp_snd_func_t *func_hook_middle;
       sfp_snd_func_t *func_hook_last;
       sfp_snd_func_t *func_hook_really_last;
     }
sfp_snd_func_tab_t;

     int sfp_fwd_func_tab_init (sfp_fwd_func_tab_t ** func_tab);
     void sfp_fwd_func_tab_free (sfp_fwd_func_tab_t * func_tab);
     int sfp_fwd_func_tab_register_hook (sfp_fwd_func_tab_t * tab,
					 sfp_fwd_func_t * func,
					 int hook_flag);

     int sfp_rcv_func_tab_init (sfp_rcv_func_tab_t ** func_tab);
     void sfp_rcv_func_tab_free (sfp_rcv_func_tab_t * func_tab);
     int sfp_rcv_func_tab_register_hook (sfp_rcv_func_tab_t * tab,
					 sfp_rcv_func_t * func,
					 int hook_flag);

     int sfp_snd_func_tab_init (sfp_snd_func_tab_t ** func_tab);
     void sfp_snd_func_tab_free (sfp_snd_func_tab_t * func_tab);
     int sfp_snd_func_tab_register_hook (sfp_snd_func_tab_t * tab,
					 sfp_snd_func_t * func,
					 int hook_flag);

     typedef struct sfp_plugin_t sfp_plugin_t;

     struct sfp_plugin_t
     {
       psp_plugin_t *psp_plugin;
       sfp_plugin_t *next;
       sfp_plugin_t *parent;
     };

     int sfp_plugin_init (sfp_plugin_t ** plugin, psp_plugin_t * psp_plugin);
     void sfp_plugin_free (sfp_plugin_t * plugin);

#endif
