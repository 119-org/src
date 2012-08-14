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

#include <ppl/ppl_socket.h>

PPL_DECLARE (ppl_socket_t) ppl_socket (int domain, int protocol, int type)
{
  return socket (domain, protocol, type);
}

PPL_DECLARE (ppl_socket_t)
ppl_socket_bind (ppl_socket_t sock, struct sockaddr * addr, int len)
{
  return bind (sock, addr, len);
}

PPL_DECLARE (int) ppl_socket_close (ppl_socket_t sock)
{
  return close (sock);
}

PPL_DECLARE (int)
ppl_socket_write (ppl_socket_t sock, const void *buf, size_t count)
{
  return write (sock, buf, count);
}

PPL_DECLARE (int)
ppl_socket_read (ppl_socket_t sock, void *buf, size_t count)
{
  return read (sock, buf, count);
}

PPL_DECLARE (int)
ppl_socket_recv (ppl_socket_t sock, void *buf, size_t max, int flags,
		 struct sockaddr *from, socklen_t * fromlen)
{
  return recvfrom (sock, buf, max, flags, from, fromlen);
}
