/*
  This is the ppl library. It provides a portable interface to usual OS features
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The ppl library free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The ppl library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with the ppl library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef _PPL_INIT_H_
#define _PPL_INIT_H_

#include "ppl.h"

/**
 * @file ppl_init.h
 * @brief PPL Init Handling Routines
 */

/**
 * @defgroup PPL_INIT init Handling
 * @ingroup PPL
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Init the ppl library.
 */
  PPL_DECLARE (int) ppl_init_open (void);

/**
 * Free ressource used by the ppl library.
 */
  PPL_DECLARE (void) ppl_init_close (void);
  
#ifdef __cplusplus
}
#endif
/** @} */
#endif
