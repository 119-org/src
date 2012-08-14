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
#include <wchar.h>

ppl_status_t
ppl_conv_utf8_osip_to_ucs2 (const char *in, size_t * inbytes,
		       ppl_wchar_t * out, size_t * outwords)
{
  /* what is a long long on win32? */
  /* ppl_int64_t newch, mask; */
  long newch, mask;
  size_t expect, eating;
  int ch;

  while (*inbytes && *outwords)
    {
      ch = (unsigned char) (*in++);
      if (!(ch & 0200))
	{
	  /* US-ASCII-7 plain text
	   */
	  --*inbytes;
	  --*outwords;
	  *(out++) = ch;
	}
      else
	{
	  if ((ch & 0300) != 0300)
	    {
	      /* Multibyte Continuation is out of place
	       */
	      return -1;
	    }
	  else
	    {
	      /* Multibyte Sequence Lead Character
	       *
	       * Compute the expected bytes while adjusting
	       * or lead byte and leading zeros mask.
	       */
	      mask = 0340;
	      expect = 1;
	      while ((ch & mask) == mask)
		{
		  mask |= mask >> 1;
		  if (++expect > 3)	/* (truly 5 for ucs-4) */
		    return -1;
		}
	      newch = ch & ~mask;
	      eating = expect + 1;
	      if (*inbytes <= expect)
		return -1;
	      /* Reject values of excessive leading 0 bits
	       * utf-8 _demands_ the shortest possible byte length
	       */
	      if (expect == 1)
		{
		  if (!(newch & 0036))
		    return -1;
		}
	      else
		{
		  /* Reject values of excessive leading 0 bits
		   */
		  if (!newch && !((unsigned char) *in & 0077 & (mask << 1)))
		    return -1;
		  if (expect == 2)
		    {
		      /* Reject values D800-DFFF when not utf16 encoded
		       * (may not be an appropriate restriction for ucs-4)
		       */
		      if (newch == 0015 && ((unsigned char) *in & 0040))
			return -1;
		    }
		  else if (expect == 3)
		    {
		      /* Short circuit values > 110000
		       */
		      if (newch > 4)
			return -1;
		      if (newch == 4 && ((unsigned char) *in & 0060))
			return -1;
		    }
		}
	      /* Where the boolean (expect > 2) is true, we will need
	       * an extra word for the output.
	       */
	      if (*outwords < (size_t) (expect > 2) + 1)
		break;		/* buffer full */
	      while (expect--)
		{
		  /* Multibyte Continuation must be legal */
		  if (((ch = (unsigned char) *(in++)) & 0300) != 0200)
		    return -1;
		  newch <<= 6;
		  newch |= (ch & 0077);
		}
	      *inbytes -= eating;
	      /* newch is now a true ucs-4 character
	       *
	       * now we need to fold to ucs-2
	       */
	      if (newch < 0x10000)
		{
		  --*outwords;
		  *(out++) = (ppl_wchar_t) newch;
		}
	      else
		{
		  *outwords -= 2;
		  newch -= 0x10000;
		  *(out++) = (ppl_wchar_t) (0xD800 | (newch >> 10));
		  *(out++) = (ppl_wchar_t) (0xDC00 | (newch & 0x03FF));
		}
	    }
	}
    }
  /* Buffer full 'errors' aren't errors, the client must inspect both
   * the inbytes and outwords values
   */
  return PPL_SUCCESS;
}


ppl_status_t
utf8_osip_to_unicode_path (ppl_wchar_t * retstr, size_t retlen, const char *srcstr)
{
  /* TODO: The computations could preconvert the string to determine
   * the true size of the retstr, but that's a memory over speed
   * tradeoff that isn't appropriate this early in development.
   *
   * Allocate the maximum string length based on leading 4 
   * characters of \\?\ (allowing nearly unlimited path lengths) 
   * plus the trailing null, then transform /'s into \\'s since
   * the \\?\ form doesn't allow '/' path seperators.
   *
   * Note that the \\?\ form only works for local drive paths, and
   * \\?\UNC\ is needed UNC paths.
   */
  int srcremains = strlen (srcstr) + 1;
  ppl_wchar_t *t = retstr;
  ppl_status_t rv;

  /* This is correct, we don't twist the filename if it is will
   * definately be shorter than MAX_PATH.  It merits some 
   * performance testing to see if this has any effect, but there
   * seem to be applications that get confused by the resulting
   * Unicode \\?\ style file names, especially if they use argv[0]
   * or call the Win32 API functions such as GetModuleName, etc.
   * Not every application is prepared to handle such names.
   *
   * Note that a utf-8 name can never result in more wide chars
   * than the original number of utf-8 narrow chars.
   */
  if (srcremains > MAX_PATH)
    {
      if (srcstr[1] == ':' && (srcstr[2] == '/' || srcstr[2] == '\\'))
	{
	  wcscpy (retstr, L"\\\\?\\");
	  retlen -= 4;
	  t += 4;
	}
      else if ((srcstr[0] == '/' || srcstr[0] == '\\')
	       && (srcstr[1] == '/' || srcstr[1] == '\\')
	       && (srcstr[2] != '?'))
	{
	  /* Skip the slashes */
	  srcstr += 2;
	  srcremains -= 2;
	  wcscpy (retstr, L"\\\\?\\UNC\\");
	  retlen -= 8;
	  t += 8;
	}
    }

  if (rv = ppl_conv_utf8_osip_to_ucs2 (srcstr, &srcremains, t, &retlen))
    {
      return (rv == -1) ? -1 : rv;
    }
  if (srcremains)
    {
      return -1;
    }
  for (; *t; ++t)
    if (*t == L'/')
      *t = L'\\';
  return PPL_SUCCESS;
}

PPL_DECLARE (ppl_status_t) ppl_dso_unload (ppl_dso_handle_t * dso)
{
  if (dso->handle != NULL && !FreeLibrary (dso->handle))
    {
      osip_free (dso);
      return -1;
    }
  osip_free (dso);
  dso->handle = NULL;

  return PPL_SUCCESS;
}


PPL_DECLARE (ppl_status_t)
ppl_dso_load (ppl_dso_handle_t ** dso_handle, const char *path)
{
  HINSTANCE os_handle;
  ppl_status_t rv;
  UINT em;

#if PPL_HAS_UNICODE_FS
  {
    ppl_wchar_t wpath[PPL_PATH_MAX];

    if ((rv = utf8_osip_to_unicode_path (wpath, sizeof (wpath)
				    / sizeof (ppl_wchar_t),
				    path)) != PPL_SUCCESS)
      {
	*dso_handle = osip_malloc (sizeof (**dso_handle));
	(*dso_handle)->errormsg = NULL;	/* no info */
	return -1;
      }
    /* Prevent ugly popups from killing our app */
    em = SetErrorMode (SEM_FAILCRITICALERRORS);
    os_handle = LoadLibraryExW (wpath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!os_handle)
      os_handle = LoadLibraryExW (wpath, NULL, 0);
    if (!os_handle)
      rv = -1;
    SetErrorMode (em);
  }
#else
/* back to the behavior for 95, 98 and Millenium */
/* I do not autodetect this! */
  {
    char fspec[PPL_PATH_MAX], *p = fspec;

    /* Must convert path from / to \ notation.
     * Per PR2555, the LoadLibraryEx function is very picky about slashes.
     * Debugging on NT 4 SP 6a reveals First Chance Exception within NTDLL.
     * LoadLibrary in the MS PSDK also reveals that it -explicitly- states
     * that backslashes must be used for the LoadLibrary family of calls.
     */
    strncpy (fspec, path, sizeof (fspec));
    while ((p = strchr (p, '/')) != NULL)
      *p = '\\';

    /* Prevent ugly popups from killing our app */
    em = SetErrorMode (SEM_FAILCRITICALERRORS);
    os_handle = LoadLibraryEx (path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!os_handle)
      os_handle = LoadLibraryEx (path, NULL, 0);
    if (!os_handle)
      rv = -1;
    else
      rv = PPL_SUCCESS;
    SetErrorMode (em);
  }
#endif
  *dso_handle = osip_malloc (sizeof (**dso_handle));

  if (rv)
    {
      (*dso_handle)->handle = NULL;
      (*dso_handle)->errormsg = NULL;	/* no info */
      return -1;
    }

  (*dso_handle)->handle = (void *) os_handle;
  (*dso_handle)->errormsg = NULL;

  return PPL_SUCCESS;
}

PPL_DECLARE (ppl_status_t)
ppl_dso_sym (ppl_dso_handle_sym_t * ressym,
	     struct ppl_dso_handle_t * handle, const char *symname)
{
  *ressym = (ppl_dso_handle_sym_t) GetProcAddress (handle->handle, symname);

  if (!*ressym)
    {
      return -1;
    }
  return PPL_SUCCESS;
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
  return "Error checking not implemented on win32";
}
