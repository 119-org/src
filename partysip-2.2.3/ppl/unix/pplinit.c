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


#include <ppl/ppl_init.h>
#include <ppl/ppl_uinfo.h>
#include <ppl/ppl_dns.h>

/**
 * Init the ppl library.
 */
PPL_DECLARE (int) ppl_init_open ()
{
  int i;

  i = ppl_uinfo_init ();
  if (i != 0)
    return -1;
  i = ppl_dns_init ();
  if (i != 0)
    return -1;
  return 0;
}

/**
 * Free ressource used by the ppl library.
 */
PPL_DECLARE (void) ppl_init_close ()
{
  ppl_uinfo_close_dbm ();
  ppl_uinfo_free_all ();
  ppl_dns_close ();
  return;
}
