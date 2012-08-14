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

#include <ppl/ppl_time.h>

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>		/* malloc(), free() */
#endif
#if HAVE_STRING_H
#include <string.h>		/* for strerror() on HP-UX */
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>		/* for strerror() on HP-UX */
#endif
#if HAVE_TIME_H
#include <time.h>		/* for strerror() on HP-UX */
#endif

PPL_DECLARE (ppl_time_t) ppl_time ()
{
  return time (NULL);
}
