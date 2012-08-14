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
 *  this file is a modification of ./httpd-2.0.32/srclib/apr/include/apr_dso.h
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

#ifndef _PPL_DSO_H_
#define _PPL_DSO_H_

#include "ppl.h"

#ifdef HAVE_MACH_O_DYLD_H
#include <mach-o/dyld.h>

#elif defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

#ifdef HAVE_DL_H
#include <dl.h>
#endif

#ifndef RTLD_NOW
#define RTLD_NOW 1
#endif

#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif

#if (defined(__FreeBSD__) ||\
     defined(__OpenBSD__) ||\
     defined(__NetBSD__)     ) && !defined(__ELF__)
#define DLSYM_NEEDS_UNDERSCORE
#endif

struct ppl_dso_handle_t
{
  void *handle;
  const char *errormsg;
};

/**
 * @file ppl_dso.h
 * @brief PPL Dynamic Object Handling Routines
 */

/**
 * @defgroup PPL_DSO Dynamic Object Handling
 * @ingroup PPL
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Structure for referencing dynamic objects
 * @defvar ppl_dso_handle_t
 */
  typedef struct ppl_dso_handle_t ppl_dso_handle_t;

/**
 * Structure for referencing symbols from dynamic objects
 * @defvar ppl_dso_handle_sym_t
 */
  typedef void *ppl_dso_handle_sym_t;

/**
 * Load a DSO library.
 * @param res_handle Location to store new handle for the DSO.
 * @param path Path to the DSO library
 */
    PPL_DECLARE (ppl_status_t) ppl_dso_load (ppl_dso_handle_t ** res_handle,
					     const char *path);

/**
 * Close a DSO library.
 * @param handle handle to close.
 */
    PPL_DECLARE (ppl_status_t) ppl_dso_unload (ppl_dso_handle_t * handle);

/**
 * Load a symbol from a DSO handle.
 * @param ressym Location to store the loaded symbol
 * @param handle handle to load the symbol from.
 * @param symname Name of the symbol to load.
 */
    PPL_DECLARE (ppl_status_t) ppl_dso_sym (ppl_dso_handle_sym_t * ressym,
					    ppl_dso_handle_t * handle,
					    const char *symname);

/**
 * Report more information when a DSO function fails.
 * @param dso The dso handle that has been opened
 * @param buf Location to store the dso error
 * @param bufsize The size of the provided buffer
 */
    PPL_DECLARE (const char *) ppl_dso_error (ppl_dso_handle_t * dso,
					      char *buf, int bufsize);

#ifdef __cplusplus
}
#endif
/** @} */
#endif
