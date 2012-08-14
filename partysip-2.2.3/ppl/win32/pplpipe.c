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
#include <ppl/ppl_socket.h>
#include <osipparser2/osip_port.h>


#if 1

PPL_DECLARE (ppl_pipe_t *) ppl_pipe ()
{
  ppl_socket_t s = 0;
  int timeout = 0;
  static int aport = 10500;
  struct sockaddr_in raddr;
  int j;

  ppl_pipe_t *my_pipe = (ppl_pipe_t *) osip_malloc (sizeof (ppl_pipe_t));

//  my_pipe->pipes[0] = (int) ppl_socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    s = (int) ppl_socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (0 > s)
    {
      osip_free (my_pipe);
      return NULL;
    }
//  my_pipe->pipes[1] = (int) ppl_socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    my_pipe->pipes[1] = (int) ppl_socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (0 > my_pipe->pipes[1])
    {
      ppl_socket_close (s);
      osip_free (my_pipe);
      return NULL;
    }

  /* we should set the local ip address */
  raddr.sin_addr.s_addr = inet_addr ("127.0.0.1");
  /* we should set the port of my_pipe[1] */
  raddr.sin_family = AF_INET;

  j = 50;
  while (aport++ && j-- > 0)
    {
      raddr.sin_port = htons ((short) aport);
      if (ppl_socket_bind
	  (s, (struct sockaddr *) &raddr, sizeof (raddr)) < 0)
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_WARNING, NULL,
				  "Failed to bind one local socket %i!\n",
				  aport));
	}
      else
	break;
    }

  if (j == 0)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "Failed to bind a local socket, aborting!\n"));
      ppl_socket_close (s);
      ppl_socket_close (my_pipe->pipes[1]);
      osip_free (my_pipe);
      exit (0);
    }

  j = listen(s,1);
  if (j != 0)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "Failed to listen on a local socket, aborting!\n"));
      ppl_socket_close (s);
      ppl_socket_close (my_pipe->pipes[1]);
      osip_free (my_pipe);
      exit (0);
    }

    j = setsockopt (my_pipe->pipes[1],
		      SOL_SOCKET,
		      SO_RCVTIMEO, (const char*) &timeout, sizeof (timeout));
    if (j != NO_ERROR)
      {
	/* failed for some reason... */
		OSIP_TRACE (osip_trace
			(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"udp plugin; cannot set O_NONBLOCK to the file desciptor!\n"));
		ppl_socket_close (s);
		ppl_socket_close (my_pipe->pipes[1]);
		osip_free (my_pipe);
		exit (0);
	}

/* already set:
  raddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  raddr.sin_port        = htons((short)aport);
  raddr.sin_family      = AF_INET; */

  /* pipe[1] is prepared to send data to pipe[0] */
  connect (my_pipe->pipes[1], (struct sockaddr *) &raddr, sizeof (raddr));

  my_pipe->pipes[0] = accept (s, NULL, NULL);

  if (my_pipe->pipes[0]<=0)
  {
		OSIP_TRACE (osip_trace
			(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"udp plugin; Failed to call accept!\n"));
		ppl_socket_close (s);
		ppl_socket_close (my_pipe->pipes[1]);
		osip_free (my_pipe);
		exit (0);

  }

  return my_pipe;
}

PPL_DECLARE (int) ppl_pipe_close (ppl_pipe_t * apipe)
{
  if (apipe == NULL)
    return -1;
  ppl_socket_close (apipe->pipes[0]);
  ppl_socket_close (apipe->pipes[1]);
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
  return send (apipe->pipes[1], buf, count, 0);
}

/**
 * Read in a pipe.
 */
PPL_DECLARE (int) ppl_pipe_read (ppl_pipe_t * apipe, void *buf, size_t count)
{
  if (apipe == NULL)
    return -1;
  return recv (apipe->pipes[0], buf, count, 0 /* MSG_DONTWAIT */ );	/* BUG?? */
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

#else

PPL_DECLARE (ppl_pipe_t *) ppl_pipe ()
{
	ppl_pipe_t *my_pipe;
	int i;

	my_pipe = (ppl_pipe_t *) osip_malloc (sizeof (ppl_pipe_t));
	if (my_pipe==NULL)
	{
		OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			"Failed to allocate pipe!\n"));
		return NULL;
	}

	i = CreatePipe(my_pipe->pipes[0], my_pipe->pipes[1], NULL, 0);
	if (i!=0)
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				  "Failed to create pipe!\n"));
      osip_free (my_pipe);
	  return NULL;
	}
	return my_pipe;
}

PPL_DECLARE (int) ppl_pipe_close (ppl_pipe_t * apipe)
{
  if (apipe == NULL)
    return -1;
  DisconnectNamedPipe (apipe->pipes[0]);
  DisconnectNamedPipe (apipe->pipes[1]);
  CloseHandle(apipe->pipes[0]);
  CloseHandle(apipe->pipes[1]);
  osip_free (apipe);
  return 0;
}


/**
 * Write in a pipe.
 */
PPL_DECLARE (int)
ppl_pipe_write (ppl_pipe_t * apipe, const void *buf, size_t count)
{
  int nBytesWritten;
  if (apipe == NULL)
    return -1;

  return WriteFile (apipe->pipes[1], buf, count, &nBytesWritten, NULL);
}

/**
 * Read in a pipe.
 */
PPL_DECLARE (int) ppl_pipe_read (ppl_pipe_t * apipe, void *buf, size_t count)
{
  int nBytesRead;
  int i;
  if (apipe == NULL)
    return -1;

  i = ReadFile(apipe->pipes[0], buf, count
	  , &nBytesRead, NULL);
  if (i!=0)
  {
	return -1;
  }
  return nBytesRead;
}

/**
 * Get descriptor of reading pipe.
 */
PPL_DECLARE (int) ppl_pipe_get_read_descr (ppl_pipe_t * apipe)
{
  if (apipe == NULL)
    return -1;

  /* How to return a descriptor??? IMPOSIBLE? */
  return apipe->pipes[0];
}

#endif
