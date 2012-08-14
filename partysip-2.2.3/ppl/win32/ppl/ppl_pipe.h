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



#ifndef _PPL_PIPE_H_
#define _PPL_PIPE_H_

#include "ppl.h"


/**
 * @file ppl_pipe.h
 * @brief PPL Pipe Handling Routines
 */

/**
 * @defgroup PPL_PIPE Pipe Handling
 * @ingroup PPL
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Structure for storing a pipe descriptor
 * @defvar ppl_pipe_t
 */
  typedef struct ppl_pipe_t ppl_pipe_t;

  struct ppl_pipe_t
  {
#if 1
    int pipes[2];
#else
	HANDLE pipes[2];
#endif
  };

/**
 * Get New pipe pair.
 */
    PPL_DECLARE (ppl_pipe_t *) ppl_pipe (void);

/**
 * Close pipe
 */
    PPL_DECLARE (int) ppl_pipe_close (ppl_pipe_t * apipe);

/**
 * Write in a pipe.
 */
    PPL_DECLARE (int) ppl_pipe_write (ppl_pipe_t * pipe, const void *buf,
				      size_t count);

/**
 * Read in a pipe.
 */
    PPL_DECLARE (int) ppl_pipe_read (ppl_pipe_t * pipe, void *buf,
				     size_t count);

/**
 * Get descriptor of reading pipe.
 */
    PPL_DECLARE (int) ppl_pipe_get_read_descr (ppl_pipe_t * pipe);

#ifdef __cplusplus
}
#endif
/** @} */
#endif
