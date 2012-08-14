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


#ifndef _PPL_DNS_H_
#define _PPL_DNS_H_

#include "ppl.h"
#include <ppl/ppl_time.h>

/* compiler happyness: avoid ``empty compilation unit'' problem */
#define COMPILER_HAPPYNESS(name) \
    int __##name##_unit = 0;

#ifdef HAVE_ERRNO_H
#include <stdlib.h>
#endif

#ifdef HAVE_ERRNO_H
#include <stdio.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_ARPA_NAMESER_H
#include <arpa/nameser.h>
#endif

#ifdef HAVE_RESOLV8_COMPAT_H
#include <nameser8_compat.h>
#include <resolv8_compat.h>
#elif defined(HAVE_RESOLV_H) || defined(OpenBSD) || defined(FreeBSD) || defined(NetBSD)
#include <resolv.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif


/**
 * @file ppl_dns.h
 * @brief PPL DNS Handling Routines
 */

/**
 * @defgroup PPL_DNS DNS Handling
 * @ingroup PPL
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

  /* #define DEFINE_SOCKADDR_STORAGE */
#ifdef DEFINE_SOCKADDR_STORAGE

#if ULONG_MAX > 0xffffffff
# define __ss_aligntype __uint64_t
#else
# define __ss_aligntype __uint32_t
#endif
#define _SS_SIZE        128
#define _SS_PADSIZE     (_SS_SIZE - (2 * sizeof (__ss_aligntype)))

struct sockaddr_storage
{
    sa_family_t ss_family;      /* Address family */
    __ss_aligntype __ss_align;  /* Force desired alignment.  */
    char __ss_padding[_SS_PADSIZE];
};

#endif

/**
 * Structure for referencing an ip address.
 * @defvar ppl_dns_ip_t
 */
  typedef struct ppl_dns_ip_t ppl_dns_ip_t;

  struct ppl_dns_ip_t
  {
#define PSP_SRV_LOOKUP 0
#define PSP_NS_LOOKUP  1

    int srv_ns_flag;		/* distinguish NS and SRV lookup */
    unsigned int pref;
    char *name;
    unsigned int port;
    long ttl;
    unsigned int weight;
    unsigned int rweight;
#ifdef HAVE_GETADDRINFO
    struct addrinfo *addrinfo;
#else
    struct sockaddr_in sin;
#endif
    ppl_dns_ip_t *next;
    ppl_dns_ip_t *parent;
  };

/**
 * Structure for referencing an error.
 * @defvar ppl_dns_ip_t
 */
  typedef struct ppl_dns_error_t ppl_dns_error_t;

  struct ppl_dns_error_t
  {
    char *domain;
    /* char *error; futur use */
    int expires;
    ppl_dns_error_t *next;
    ppl_dns_error_t *parent;
  };

/**
 * Structure for referencing dns entry
 * @defvar ppl_dns_entry_t
 */
  typedef struct ppl_dns_entry_t ppl_dns_entry_t;

  struct ppl_dns_entry_t
  {
    char *name;
    char *protocol;
    ppl_time_t date;
    ppl_dns_ip_t *dns_ips;

    int ref;			/* counter for reference */
    ppl_dns_entry_t *next;
    ppl_dns_entry_t *parent;
  };


/* the biggest packet we'll send and receive */
#if PACKETSZ > 1024
#define	MAXPACKET PACKETSZ
#else
#define	MAXPACKET 1024
#endif

/* and what we send and receive */
  typedef union
  {
    HEADER hdr;
    u_char buf[MAXPACKET];
  }
  querybuf;

#ifndef T_SRV
#define T_SRV		33
#endif

/**
 * Init the DNS library.
 */
    PPL_DECLARE (ppl_status_t) ppl_dns_init (void);

/**
 * Free ressources in the DNS library.
 */
    PPL_DECLARE (ppl_status_t) ppl_dns_close (void);

/**
 * Delete and remove an entry in the cache.
 */
    PPL_DECLARE (void) ppl_dns_entry_free (ppl_dns_entry_t * dns);

/**
 * Initialize a ppl_dns_ip_t element.
 * WARNING: You MUST set the sin element before partysip can use it.
 */
    PPL_DECLARE (int) ppl_dns_ip_init (ppl_dns_ip_t ** ip);

/**
 * Free a ppl_dns_ip_t element.
 */
    PPL_DECLARE (int) ppl_dns_ip_free (ppl_dns_ip_t * ip);

/**
 * Clone a ppl_dns_ip_t element.
 */
    PPL_DECLARE (int) ppl_dns_ip_clone (ppl_dns_ip_t * ip,
					ppl_dns_ip_t ** dest);

/**
 * Make a query.
 * <BR>THIS IS NOT A REENTRANT METHOD!
 * @param domain Domain to search
 * @param protocol Protol to search for. (should be tcp or udp) 
 */
    PPL_DECLARE (ppl_status_t) ppl_dns_query (ppl_dns_entry_t ** dns,
					      char *domain, char *protocol);

/**
 * Get next query from the fifo of query.
 */
    PPL_DECLARE (char *) ppl_dns_get_next_query (void);

/**
 * Get next query from the fifo of query. Return NULL if no element is found.
 */
    PPL_DECLARE (char *) ppl_dns_tryget_next_query (void);

/**
 * Add a ERROR result in the list of known error results.
 * @param dns the the dns result to add.
 */
    PPL_DECLARE (ppl_status_t) ppl_dns_error_add (char *address);
/**
 * Get informations about dns_error.
 * @param dns the the dns result to add.
 */
    PPL_DECLARE (ppl_status_t) ppl_dns_get_error (ppl_dns_error_t **
						  dns_error, char *domain);
/**
 * Add a result in the list of known result
 * @param dns the the dns result to add.
 */
    PPL_DECLARE (ppl_status_t) ppl_dns_add_domain_result (ppl_dns_entry_t *
							  dns);

/**
 * Add a domain query to the resolver.
 * @param domain Domain to search
 */
    PPL_DECLARE (ppl_status_t) ppl_dns_add_domain_query (char *domain);

/**
 * Get a DNS result.
 * @param res_handle Location to store new handle for the DNS.
 * @param domain Domain to search
 */
    PPL_DECLARE (ppl_status_t) ppl_dns_get_result (ppl_dns_entry_t **
						   dns_result, char *domain);

/**
 * Remove the entry from local cache.
 * @param entry The entry to remove.
 */
    PPL_DECLARE (void) ppl_dns_remove_entry (ppl_dns_entry_t * dns_entry);

#define GETHOSTBYNAME_BUFLEN 512

/**
 * Thread safe address resolution asking for 1 address only.
 */
#ifdef HAVE_GETADDRINFO
    PPL_DECLARE (char *) ppl_inet_ntop (struct sockaddr *sockaddr);
    PPL_DECLARE (int)    ppl_inet_pton (const char *src, void *dst);

    PPL_DECLARE (ppl_status_t) ppl_dns_get_addrinfo (struct addrinfo **addrinfo,
						    char *hostname, int port);
#else
    PPL_DECLARE (int) ppl_dns_get_hostbyname (struct sockaddr_in *sin,
					     char *hostname, int port);
#endif

/**
 * Thread safe address resolution which create a dns entry with many ip.
 */
    PPL_DECLARE (int) ppl_dns_query_host (ppl_dns_entry_t ** dns,
					  char *hostname, int port);

/**
 * Automatic lookup of default gateway.
 */
    PPL_DECLARE (int) ppl_dns_default_gateway (int familiy, char *address, int size);

/**
 * Automatic detection of ip address and mask address.
 * On WIN32, you have to specify the WIN32_interface
 * argument. The UNIX_interface one is not used.
 * @param servername Name of interface.
 * @param serverip Ip address found for WIN32_interface.
 * @param netmask  Mask found for WIN32_interface.
 * @param UNIX_interface Name of interface.
 * @param WIN32_interface Index of interface to look for.
 */
  PPL_DECLARE (int) ppl_dns_get_local_fqdn (char **servername, char **serverip,
					    char **netmask, char *UNIX_interface,
					    unsigned int interface, int family);

  PPL_DECLARE (void) ppl_dns_entry_add_ref (ppl_dns_entry_t * dns);

  PPL_DECLARE (void) ppl_dns_entry_remove_ref (ppl_dns_entry_t * dns);

#ifdef __cplusplus
}
#endif
/** @} */

#endif
