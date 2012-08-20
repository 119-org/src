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


#include <ppl/ppl_pipe.h>
#include <osipparser2/osip_port.h>


PPL_DECLARE (ppl_pipe_t *) ppl_pipe ()
{
  ppl_pipe_t *my_pipe = (ppl_pipe_t *) osip_malloc (sizeof (ppl_pipe_t));

  if (0 != pipe (my_pipe->pipes))
    {
      osip_free (my_pipe);
      return NULL;
    }
  return my_pipe;
}

PPL_DECLARE (int) ppl_pipe_close (ppl_pipe_t * apipe)
{
  if (apipe == NULL)
    return -1;
  close (apipe->pipes[0]);
  close (apipe->pipes[1]);
  osip_free (apipe);
  return 0;
}


/**
 * Write in a pipe.
 */
PPL_DECLARE (int)
ppl_pipe_write (ppl_pipe_t * apipe, const void *buf, size_t count)
{
  if (apipe == NULL)
    return -1;
  return write (apipe->pipes[1], buf, count);
}

/**
 * Read in a pipe.
 */
PPL_DECLARE (int) ppl_pipe_read (ppl_pipe_t * apipe, void *buf, size_t count)
{
  if (apipe == NULL)
    return -1;
  return read (apipe->pipes[0], buf, count);
}

/**
 * Get descriptor of reading pipe.
 */
PPL_DECLARE (int) ppl_pipe_get_read_descr (ppl_pipe_t * apipe)
{
  if (apipe == NULL)
    return -1;
  return apipe->pipes[0];
}
