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



#ifndef _PPL_SOCKET_H_
#define _PPL_SOCKET_H_

#include "ppl.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#define PPL_PF_UNIX     PF_UNIX
#define PPL_PF_LOCAL    PF_LOCAL
#define PPL_PF_INET     PF_INET
#define PPL_PF_INET6    PF_INET6
#define PPL_PF_PACKET   PF_PACKET

#define PPL_SOCK_STREAM     SOCK_STREAM
#define PPL_SOCK_DGRAM      SOCK_DGRAM
#define PPL_SOCK_SEQPACKET  SOCK_SEQPACKET


/**
 * @file ppl_socket.h
 * @brief PPL Socket Handling Routines
 */

/**
 * @defgroup PPL_SOCKET Socket Handling
 * @ingroup PPL
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Structure for storing a socket descriptor
 * @defvar ppl_socket_t
 */
  typedef int ppl_socket_t;

/**
 * Get New socket.
 */
  PPL_DECLARE (ppl_socket_t) ppl_socket (int domain, int protocol,
					 int type);

/**
 * Bind to a socket.
 */
  PPL_DECLARE (ppl_socket_t) ppl_socket_bind (ppl_socket_t sock,
					      struct sockaddr *addr,
					      int len);
  
/**
 * Close socket.
 */
  PPL_DECLARE (int) ppl_socket_close (ppl_socket_t sock);

/**
 * Write in a socket.
 */
  PPL_DECLARE (int) ppl_socket_write (ppl_socket_t sock, const void *buf,
				      size_t count);
  
/**
 * Read in a socket.
 */
  PPL_DECLARE (int) ppl_socket_read (ppl_socket_t sock, void *buf, size_t count);
       
/**
 * Read in a socket.
 */
  PPL_DECLARE (int) ppl_socket_recv (ppl_socket_t sock, void *buf,
				     size_t count, int flags,
				     struct sockaddr *from,
				     socklen_t * fromlen);

#ifdef __cplusplus
}
#endif
/** @} */
#endif
