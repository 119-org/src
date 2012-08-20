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

#ifndef _PSP_CONFIG_H_
#define _PSP_CONFIG_H_

#include <partysip/psp_macros.h>

typedef struct config_element_t config_element_t;

struct config_element_t
{
  char *sub_config;
  char *name;
  char *value;
  config_element_t *next;
  config_element_t *parent;
};

/* external API */
int psp_config_load (char *filename);
void psp_config_unload (void);

PPL_DECLARE (int)
psp_config_add_element (char *name, char *value);
PPL_DECLARE (char *)
psp_config_get_element (char *name);
PPL_DECLARE (int)
psp_config_set_element (char *sub_config, char *string);
PPL_DECLARE (config_element_t *) psp_config_get_sub_element (char *name,
							     char *sub_config,
							     config_element_t
							     * start);

#endif
