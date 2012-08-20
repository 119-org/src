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


#ifndef PPL_H
#define PPL_H

#include <windowsx.h>
#include <winsock2.h>
#include <Ws2tcpip.h>

#include <osipparser2/osip_port.h>

#define REMOVE_ELEMENT(first_element, element)   \
       if (element->parent==NULL)                \
	{ first_element = element->next;         \
          if (first_element!=NULL)               \
          first_element->parent = NULL; }        \
       else \
        { element->parent->next = element->next; \
          if (element->next!=NULL)               \
	element->next->parent = element->parent; \
	element->next = NULL;                    \
	element->parent = NULL; }

#define ADD_ELEMENT(first_element, element) \
   if (first_element==NULL)                 \
    {                                       \
      first_element   = element;            \
      element->next   = NULL;               \
      element->parent = NULL;               \
    }                                       \
  else                                      \
    {                                       \
      element->next   = first_element;      \
      element->parent = NULL;               \
      element->next->parent = element;      \
      first_element = element;              \
    }

#define APPEND_ELEMENT(type_of_element_t, first_element, element) \
  if (first_element==NULL)                            \
    { first_element = element;                        \
      element->next   = NULL; /* useless */           \
      element->parent = NULL; /* useless */ }         \
  else                                                \
    { type_of_element_t *f;                           \
      for (f=first_element; f->next!=NULL; f=f->next) \
         { }                                          \
      f->next    = element;                           \
      element->parent = f;                            \
      element->next   = NULL;                         \
    }

/* on win32, the compiler needs a special declaration */
#ifndef PSP_PLUGIN

#define PPL_DECLARE_DATA __declspec(dllexport)
#define PPL_DECLARE(type) __declspec(dllexport) type __stdcall

#define PSP_PLUGIN_DECLARE_DATA __declspec(dllimport)
#define PSP_PLUGIN_DECLARE(type) __declspec(dllimport) type __stdcall

#else

#define PPL_DECLARE_DATA __declspec(dllimport)
#define PPL_DECLARE(type) __declspec(dllimport) type __stdcall

#define PSP_PLUGIN_DECLARE_DATA __declspec(dllexport)
#define PSP_PLUGIN_DECLARE(type) __declspec(dllexport) type

#endif

typedef int ppl_status_t;

#define PPL_SUCCESS   0
/* APR ERROR VALUES */
/** 
 * @defgroup APRErrorValues Error Values
 * <PRE>
 * <b>APR ERROR VALUES</b>
 * APR_ENOSTAT      APR was unable to perform a stat on the file 
 * APR_ENOPOOL      APR was not provided a pool with which to allocate memory
 * APR_EBADDATE     APR was given an invalid date 
 * APR_EINVALSOCK   APR was given an invalid socket
 * APR_ENOPROC      APR was not given a process structure
 * APR_ENOTIME      APR was not given a time structure
 * APR_ENODIR       APR was not given a directory structure
 * APR_ENOLOCK      APR was not given a lock structure
 * APR_ENOPOLL      APR was not given a poll structure
 * APR_ENOSOCKET    APR was not given a socket
 * APR_ENOTHREAD    APR was not given a thread structure
 * APR_ENOTHDKEY    APR was not given a thread key structure
 * APR_ENOSHMAVAIL  There is no more shared memory available
 * APR_EDSOOPEN     APR was unable to open the dso object.  For more 
 *                  information call apr_dso_error().
 * APR_EGENERAL     General failure (specific information not available)
 * APR_EBADIP       The specified IP address is invalid
 * APR_EBADMASK     The specified netmask is invalid
 * </PRE>
 *
 * <PRE>
 * <b>APR STATUS VALUES</b>
 * APR_INCHILD        Program is currently executing in the child
 * APR_INPARENT       Program is currently executing in the parent
 * APR_DETACH         The thread is detached
 * APR_NOTDETACH      The thread is not detached
 * APR_CHILD_DONE     The child has finished executing
 * APR_CHILD_NOTDONE  The child has not finished executing
 * APR_TIMEUP         The operation did not finish before the timeout
 * APR_INCOMPLETE     The character conversion stopped because of an 
 *                    incomplete character or shift sequence at the end 
 *                    of the input buffer.
 * APR_BADCH          Getopt found an option not in the option string
 * APR_BADARG         Getopt found an option that is missing an argument 
 *                    and an argument was specified in the option string
 * APR_EOF            APR has encountered the end of the file
 * APR_NOTFOUND       APR was unable to find the socket in the poll structure
 * APR_ANONYMOUS      APR is using anonymous shared memory
 * APR_FILEBASED      APR is using a file name as the key to the shared memory
 * APR_KEYBASED       APR is using a shared key as the key to the shared memory
 * APR_EINIT          Ininitalizer value.  If no option has been found, but 
 *                    the status variable requires a value, this should be used
 * APR_ENOTIMPL       The APR function has not been implemented on this 
 *                    platform, either because nobody has gotten to it yet, 
 *                    or the function is impossible on this platform.
 * APR_EMISMATCH      Two passwords do not match.
 * APR_EABSOLUTE      The given path was absolute.
 * APR_ERELATIVE      The given path was relative.
 * APR_EINCOMPLETE    The given path was neither relative nor absolute.
 * APR_EABOVEROOT     The given path was above the root path.
 * APR_EBUSY          The given lock was busy.
 * </PRE>
 * @{
 */

#define PPL_START_ERRNO    200
#define PPL_START_STATUS   250

#define PPL_ENOSTAT        (PPL_START_ERRNO + 1)
#define PPL_ENOPOOL        (PPL_START_ERRNO + 2)
/* empty slot: +3 */
/** @see PPL_STATUS_IS_EBADDATE */
#define PPL_EBADDATE       (PPL_START_ERRNO + 4)
/** @see PPL_STATUS_IS_EINVALSOCK */
#define PPL_EINVALSOCK     (PPL_START_ERRNO + 5)
/** @see PPL_STATUS_IS_ENOPROC */
#define PPL_ENOPROC        (PPL_START_ERRNO + 6)
/** @see PPL_STATUS_IS_ENOTIME */
#define PPL_ENOTIME        (PPL_START_ERRNO + 7)
/** @see PPL_STATUS_IS_ENODIR */
#define PPL_ENODIR         (PPL_START_ERRNO + 8)
/** @see PPL_STATUS_IS_ENOLOCK */
#define PPL_ENOLOCK        (PPL_START_ERRNO + 9)
/** @see PPL_STATUS_IS_ENOPOLL */
#define PPL_ENOPOLL        (PPL_START_ERRNO + 10)
/** @see PPL_STATUS_IS_ENOSOCKET */
#define PPL_ENOSOCKET      (PPL_START_ERRNO + 11)
/** @see PPL_STATUS_IS_ENOTHREAD */
#define PPL_ENOTHREAD      (PPL_START_ERRNO + 12)
/** @see PPL_STATUS_IS_ENOTHDKEY */
#define PPL_ENOTHDKEY      (PPL_START_ERRNO + 13)
/** @see PPL_STATUS_IS_EGENERAL */
#define PPL_EGENERAL       (PPL_START_ERRNO + 14)
/** @see PPL_STATUS_IS_ENOSHMAVAIL */
#define PPL_ENOSHMAVAIL    (PPL_START_ERRNO + 15)
/** @see PPL_STATUS_IS_EBADIP */
#define PPL_EBADIP         (PPL_START_ERRNO + 16)
/** @see PPL_STATUS_IS_EBADMASK */
#define PPL_EBADMASK       (PPL_START_ERRNO + 17)
/* empty slot: +18 */
/** @see PPL_STATUS_IS_EDSOPEN */
#define PPL_EDSOOPEN       (PPL_START_ERRNO + 19)
/** @see PPL_STATUS_IS_EABSOLUTE */
#define PPL_EABSOLUTE      (PPL_START_ERRNO + 20)
/** @see PPL_STATUS_IS_ERELATIVE */
#define PPL_ERELATIVE      (PPL_START_ERRNO + 21)
/** @see PPL_STATUS_IS_EINCOMPLETE */
#define PPL_EINCOMPLETE    (PPL_START_ERRNO + 22)
/** @see PPL_STATUS_IS_EABOVEROOT */
#define PPL_EABOVEROOT     (PPL_START_ERRNO + 23)
/** @see PPL_STATUS_IS_EBADPATH */
#define PPL_EBADPATH       (PPL_START_ERRNO + 24)


/* PPL ERROR VALUE TESTS */
/** 
 * PPL was unable to perform a stat on the file 
 * @warning always use this test, as platform-specific variances may meet this
 * more than one error code 
 */
#define PPL_STATUS_IS_ENOSTAT(s)        ((s) == PPL_ENOSTAT)
/** 
 * PPL was not provided a pool with which to allocate memory 
 * @warning always use this test, as platform-specific variances may meet this
 * more than one error code 
 */
#define PPL_STATUS_IS_ENOPOOL(s)        ((s) == PPL_ENOPOOL)
/** PPL was given an invalid date  */
#define PPL_STATUS_IS_EBADDATE(s)       ((s) == PPL_EBADDATE)
/** PPL was given an invalid socket */
#define PPL_STATUS_IS_EINVALSOCK(s)     ((s) == PPL_EINVALSOCK)
/** PPL was not given a process structure */
#define PPL_STATUS_IS_ENOPROC(s)        ((s) == PPL_ENOPROC)
/** PPL was not given a time structure */
#define PPL_STATUS_IS_ENOTIME(s)        ((s) == PPL_ENOTIME)
/** PPL was not given a directory structure */
#define PPL_STATUS_IS_ENODIR(s)         ((s) == PPL_ENODIR)
/** PPL was not given a lock structure */
#define PPL_STATUS_IS_ENOLOCK(s)        ((s) == PPL_ENOLOCK)
/** PPL was not given a poll structure */
#define PPL_STATUS_IS_ENOPOLL(s)        ((s) == PPL_ENOPOLL)
/** PPL was not given a socket */
#define PPL_STATUS_IS_ENOSOCKET(s)      ((s) == PPL_ENOSOCKET)
/** PPL was not given a thread structure */
#define PPL_STATUS_IS_ENOTHREAD(s)      ((s) == PPL_ENOTHREAD)
/** PPL was not given a thread key structure */
#define PPL_STATUS_IS_ENOTHDKEY(s)      ((s) == PPL_ENOTHDKEY)
/** Generic Error which can not be put into another spot */
#define PPL_STATUS_IS_EGENERAL(s)       ((s) == PPL_EGENERAL)
/** There is no more shared memory available */
#define PPL_STATUS_IS_ENOSHMAVAIL(s)    ((s) == PPL_ENOSHMAVAIL)
/** The specified IP address is invalid */
#define PPL_STATUS_IS_EBADIP(s)         ((s) == PPL_EBADIP)
/** The specified netmask is invalid */
#define PPL_STATUS_IS_EBADMASK(s)       ((s) == PPL_EBADMASK)
/* empty slot: +18 */
/** 
 * PPL was unable to open the dso object.  
 * For more information call apr_dso_error().
 */
#define PPL_STATUS_IS_EDSOOPEN(s)       ((s) == PPL_EDSOOPEN)
/** The given path was absolute. */
#define PPL_STATUS_IS_EABSOLUTE(s)      ((s) == PPL_EABSOLUTE)
/** The given path was relative. */
#define PPL_STATUS_IS_ERELATIVE(s)      ((s) == PPL_ERELATIVE)
/** The given path was neither relative nor absolute. */
#define PPL_STATUS_IS_EINCOMPLETE(s)    ((s) == PPL_EINCOMPLETE)
/** The given path was above the root path. */
#define PPL_STATUS_IS_EABOVEROOT(s)     ((s) == PPL_EABOVEROOT)
/** The given path was bad. */
#define PPL_STATUS_IS_EBADPATH(s)       ((s) == PPL_EBADPATH)

/* PPL STATUS VALUES */
/** @see PPL_STATUS_IS_INCHILD */
#define PPL_INCHILD        (PPL_START_STATUS + 1)
/** @see PPL_STATUS_IS_INPARENT */
#define PPL_INPARENT       (PPL_START_STATUS + 2)
/** @see PPL_STATUS_IS_DETACH */
#define PPL_DETACH         (PPL_START_STATUS + 3)
/** @see PPL_STATUS_IS_NOTDETACH */
#define PPL_NOTDETACH      (PPL_START_STATUS + 4)
/** @see PPL_STATUS_IS_CHILD_DONE */
#define PPL_CHILD_DONE     (PPL_START_STATUS + 5)
/** @see PPL_STATUS_IS_CHILD_NOTDONE */
#define PPL_CHILD_NOTDONE  (PPL_START_STATUS + 6)
/** @see PPL_STATUS_IS_TIMEUP */
#define PPL_TIMEUP         (PPL_START_STATUS + 7)
/** @see PPL_STATUS_IS_INCOMPLETE */
#define PPL_INCOMPLETE     (PPL_START_STATUS + 8)
/* empty slot: +9 */
/* empty slot: +10 */
/* empty slot: +11 */
/** @see PPL_STATUS_IS_BADCH */
#define PPL_BADCH          (PPL_START_STATUS + 12)
/** @see PPL_STATUS_IS_BADARG */
#define PPL_BADARG         (PPL_START_STATUS + 13)
/** @see PPL_STATUS_IS_EOF */
#define PPL_EOF            (PPL_START_STATUS + 14)
/** @see PPL_STATUS_IS_NOTFOUND */
#define PPL_NOTFOUND       (PPL_START_STATUS + 15)
/* empty slot: +16 */
/* empty slot: +17 */
/* empty slot: +18 */
/** @see PPL_STATUS_IS_ANONYMOUS */
#define PPL_ANONYMOUS      (PPL_START_STATUS + 19)
/** @see PPL_STATUS_IS_FILEBASED */
#define PPL_FILEBASED      (PPL_START_STATUS + 20)
/** @see PPL_STATUS_IS_KEYBASED */
#define PPL_KEYBASED       (PPL_START_STATUS + 21)
/** @see PPL_STATUS_IS_EINIT */
#define PPL_EINIT          (PPL_START_STATUS + 22)
/** @see PPL_STATUS_IS_ENOTIMPL */
#define PPL_ENOTIMPL       (PPL_START_STATUS + 23)
/** @see PPL_STATUS_IS_EMISMATCH */
#define PPL_EMISMATCH      (PPL_START_STATUS + 24)
/** @see PPL_STATUS_IS_EBUSY */
#define PPL_EBUSY          (PPL_START_STATUS + 25)


#endif
