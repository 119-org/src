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

/* MD5.H - header file for MD5C.C
   This file is 
   "derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm"
   Changes by Aymeric MOIZARD <jack@atosc.org>
*/

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */

#ifndef _PPL_MD5_H
#define _PPL_MD5_H

#include "ppl.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

/* GLOBAL.H - RSAREF types and constants
 */

/* PROTOTYPES should be set to one if and only if the compiler supports
  function argument prototyping.
  The following makes PROTOTYPES default to 0 if it has not already
  been defined with C compiler flags.
 */

/* modified for oSIP: GCC supports this feature */
#define PROTOTYPES 1

#ifndef PROTOTYPES
#define PROTOTYPES 0
#endif

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

/* UINT4 defines a four byte word */
typedef unsigned long int UINT4;

/* PROTO_LIST is defined depending on how PROTOTYPES is defined above.
If using PROTOTYPES, then PROTO_LIST returns the list, otherwise it
  returns an empty list.
 */
#if PROTOTYPES
#define PROTO_LIST(list) list
#else
#define PROTO_LIST(list) ()
#endif

/* End of GLOBAL.H */

/* MD5 context. */
typedef struct
{
  UINT4 state[4];		/* state (ABCD) */
  UINT4 count[2];		/* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];	/* input buffer */
}
MD5_CTX;

PPL_DECLARE (void)
     ppl_MD5Init PROTO_LIST ((MD5_CTX *));
PPL_DECLARE (void)
     ppl_MD5Update PROTO_LIST ((MD5_CTX *, unsigned char *, unsigned int));
PPL_DECLARE (void)
     ppl_MD5Final PROTO_LIST ((unsigned char[16], MD5_CTX *));


#define HASHLEN 16
     typedef char HASH[HASHLEN];

#define HASHHEXLEN 32
     typedef char HASHHEX[HASHHEXLEN + 1];

#define IN
#define OUT


#ifdef __cplusplus
     extern "C"
     {
#endif

/* calculate H(A1) as per HTTP Digest spec */
       PPL_DECLARE (void) ppl_md5_DigestCalcHA1 (IN char *pszAlg,
						 IN char *pszUserName,
						 IN char *pszRealm,
						 IN char *pszPassword,
						 IN char *pszNonce,
						 IN char *pszCNonce,
						 OUT HASHHEX SessionKey);

/* calculate request-digest/response-digest as per HTTP Digest spec */
       PPL_DECLARE (void) ppl_md5_DigestCalcResponse (IN HASHHEX HA1,	/* H(A1) */
						      IN char *pszNonce,	/* nonce from server */
						      IN char *pszNonceCount,	/* 8 hex digits */
						      IN char *pszCNonce,	/* client nonce */
						      IN char *pszQop,	/* qop-value: "", "auth", "auth-int" */
						      IN char *pszMethod,	/* method from the request */
						      IN char *pszDigestUri,	/* requested URL */
						      IN HASHHEX HEntity,	/* H(entity body) if qop="auth-int" */
						      OUT HASHHEX Response	/* request-digest or response-digest */
						      );



       /* End of MD5.h Copyright RSA  */

       #define ppl_md5_hash_osip_to_hex ppl_md5_hash_to_hex 
       PPL_DECLARE (void) ppl_md5_hash_to_hex (HASH Bin, HASHHEX Hex);



#ifdef __cplusplus
     }
#endif

#endif
