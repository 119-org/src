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

/*
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *
 *  this file is a modification of ./httpd-2.0.32/srclib/apr/dso/unix/dso.c
 */

/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#include <ppl/ppl_dso.h>
#include <osipparser2/osip_port.h>

#if !defined(DSO_USE_DLFCN) && !defined(DSO_USE_SHL) && !defined(DSO_USE_DYLD)
#error No DSO implementation specified.
#endif

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>		/* malloc(), free() */
#endif
#if HAVE_STRING_H
#include <string.h>		/* for strerror() on HP-UX */
#endif

ppl_status_t
ppl_dso_unload (ppl_dso_handle_t * thedso)
{
  ppl_dso_handle_t *dso = thedso;

  if (dso->handle == NULL)
    {
      osip_free (dso);
      return PPL_SUCCESS;
    }
#if defined(DSO_USE_SHL)
  shl_unload ((shl_t) dso->handle);
#elif defined(DSO_USE_DYLD)
  NSUnLinkModule (dso->handle, FALSE);
#elif defined(DSO_USE_DLFCN)
  if (dlclose (dso->handle) != 0)
    {
      osip_free (dso);
      return PPL_EINIT;
    }
#endif
  dso->handle = NULL;
  osip_free (dso);
  return PPL_SUCCESS;
}


ppl_status_t
ppl_dso_load (ppl_dso_handle_t ** dso_handle, const char *path)
{
#if defined(DSO_USE_SHL)
  shl_t os_handle =
    shl_load (path, BIND_IMMEDIATE | BIND_VERBOSE | BIND_NOSTART, 0L);

#elif defined(DSO_USE_DYLD)
  NSObjectFileImage image;
  NSModule os_handle = NULL;
  char *err_msg = NULL;

  if (NSCreateObjectFileImageFromFile (path, &image) !=
      NSObjectFileImageSuccess)
    {
      err_msg = "cannot create object file image";
    }
  else
    {
#ifdef NSLINKMODULE_OPTION_PRIVATE
      os_handle = NSLinkModule (image, path,
				NSLINKMODULE_OPTION_PRIVATE |
				NSLINKMODULE_OPTION_RETURN_ON_ERROR);
#else
      os_handle = NSLinkModule (image, path, TRUE);
#endif
      NSDestroyObjectFileImage (image);
    }
#elif defined(DSO_USE_DLFCN)
#if defined(OSF1) || defined(SEQUENT) || defined(SNI) ||\
    (defined(__FreeBSD_version) && (__FreeBSD_version >= 220000))
  void *os_handle = dlopen ((char *) path, RTLD_NOW | RTLD_GLOBAL);

#else
  int flags = RTLD_NOW | RTLD_GLOBAL;
  void *os_handle;

#ifdef _AIX
  if (strchr (path + 1, '(') && path[strlen (path) - 1] == ')')
    {
      /* This special archive.a(dso.so) syntax is required for
       * the way libtool likes to build shared libraries on AIX.
       * dlopen() support for such a library requires that the
       * RTLD_MEMBER flag be enabled.
       */
      flags |= RTLD_MEMBER;
    }
#endif

  os_handle = dlopen (path, flags);
#endif
#endif /* DSO_USE_x */

  *dso_handle = (ppl_dso_handle_t *) osip_malloc (sizeof (ppl_dso_handle_t));

  if (os_handle == NULL)
    {
#if defined(DSO_USE_SHL)
      (*dso_handle)->errormsg = strerror (errno);
      return errno;
#elif defined(DSO_USE_DYLD)
      (*dso_handle)->errormsg = (err_msg) ? err_msg : "link failed";
      return PPL_EDSOOPEN;
#elif defined(DSO_USE_DLFCN)
      (*dso_handle)->errormsg = dlerror ();
      return PPL_EDSOOPEN;
#endif
    }

  (*dso_handle)->handle = (void *) os_handle;
  (*dso_handle)->errormsg = NULL;

  return PPL_SUCCESS;
}


PPL_DECLARE (ppl_status_t)
ppl_dso_sym (ppl_dso_handle_sym_t * ressym,
	     ppl_dso_handle_t * handle, const char *symname)
{
#if defined(DSO_USE_SHL)
  void *symaddr = NULL;
  int status;

  errno = 0;
  status =
    shl_findsym ((shl_t *) & handle->handle, symname, TYPE_PROCEDURE,
		 &symaddr);
  if (status == -1 && errno == 0)	/* try TYPE_DATA instead */
    status =
      shl_findsym ((shl_t *) & handle->handle, symname, TYPE_DATA, &symaddr);
  if (status == -1)
    return errno;
  *ressym = symaddr;
  return PPL_SUCCESS;

#elif defined(DSO_USE_DYLD)
  void *retval = NULL;
  NSSymbol symbol;
  char *symname2 = (char *) malloc (sizeof (char) * (strlen (symname) + 2));

  sprintf (symname2, "_%s", symname);
#ifdef NSLINKMODULE_OPTION_PRIVATE
  symbol = NSLookupSymbolInModule ((NSModule) handle->handle, symname2);
#else
  symbol = NSLookupAndBindSymbol (symname2);
#endif
  free (symname2);
  if (symbol == NULL)
    {
      handle->errormsg = "undefined symbol";
      return PPL_EINIT;
    }
  retval = NSAddressOfSymbol (symbol);
  if (retval == NULL)
    {
      handle->errormsg = "cannot resolve symbol";
      return PPL_EINIT;
    }
  *ressym = retval;
  return PPL_SUCCESS;
#elif defined(DSO_USE_DLFCN)

#if defined(DLSYM_NEEDS_UNDERSCORE)
  void *retval;
  char *symbol = (char *) malloc (sizeof (char) * (strlen (symname) + 2));

  sprintf (symbol, "_%s", symname);
  retval = dlsym (handle->handle, symbol);
  free (symbol);
#elif defined(SEQUENT) || defined(SNI)
  void *retval = dlsym (handle->handle, (char *) symname);
#else
  void *retval = dlsym (handle->handle, symname);
#endif /* DLSYM_NEEDS_UNDERSCORE */

  if (retval == NULL)
    {
      handle->errormsg = dlerror ();
      return PPL_EINIT;
    }

  *ressym = retval;

  return PPL_SUCCESS;
#endif /* DSO_USE_x */
}


PPL_DECLARE (const char *)
ppl_dso_error (ppl_dso_handle_t * dso, char *buffer, int buflen)
{
  if (dso->errormsg)
    {
      int len = strlen (dso->errormsg);

      if (len < buflen)
	buflen = len;
      strncpy (buffer, dso->errormsg, buflen);
      return dso->errormsg;
    }
  return "No Error";
}
