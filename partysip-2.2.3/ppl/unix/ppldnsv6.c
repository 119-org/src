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


#include <ppl/ppl_dns.h>
#include <ppl/ppl_socket.h>

#include <osipparser2/osip_port.h>
#include <osip2/osip_fifo.h>

osip_fifo_t *dns_entries;		/* list of domain or FQDN strings to resolv */
struct osip_mutex *m_dns_result;
ppl_dns_entry_t *dns_results;
ppl_dns_error_t *dns_errors;

static int
ppl_dns_lock_result_access (void)
{
  return osip_mutex_lock (m_dns_result);
}

static int
ppl_dns_unlock_result_access (void)
{
  return osip_mutex_unlock (m_dns_result);
}

static int
compare (const void *a, const void *b)
{
  ppl_dns_ip_t *aa, *bb;

  if (!a)
    return 1;
  if (!b)
    return -1;

  aa = (ppl_dns_ip_t *) * (int *) a;
  bb = (ppl_dns_ip_t *) * (int *) b;

  if (aa->pref > bb->pref)
    return 1;
  if (aa->pref < bb->pref)
    return -1;

  if (aa->rweight > bb->rweight)
    return -1;
  if (aa->rweight < bb->rweight)
    return 1;

  return 0;
}

PPL_DECLARE (char *) ppl_dns_get_next_query ()
{
  return osip_fifo_get (dns_entries);
}

PPL_DECLARE (char *) ppl_dns_tryget_next_query ()
{
  return osip_fifo_tryget (dns_entries);
}

PPL_DECLARE (ppl_status_t) ppl_dns_init ()
{
  dns_results = NULL;
  dns_errors = NULL;
  m_dns_result = osip_mutex_init ();
  if (m_dns_result == NULL)
    return -1;
  dns_entries = (osip_fifo_t *) osip_malloc (sizeof (osip_fifo_t));
  if (dns_entries == NULL)
    {
      osip_mutex_destroy (m_dns_result);
      return -1;
    }
  osip_fifo_init (dns_entries);
  return 0;
}

PPL_DECLARE (int) ppl_dns_ip_init (ppl_dns_ip_t ** ip)
{
  *ip = (ppl_dns_ip_t *) osip_malloc (sizeof (ppl_dns_ip_t));
  if (*ip == NULL)
    return -1;
  (*ip)->srv_ns_flag = PSP_NS_LOOKUP;	/* default is a NS lookup result */
  (*ip)->ttl = 60;
  (*ip)->pref = 0;
  (*ip)->name = NULL;
  (*ip)->port = 0;
  (*ip)->weight = 0;
  (*ip)->rweight = 0;
  (*ip)->next = NULL;
  (*ip)->parent = NULL;
  return 0;
}


PPL_DECLARE (int) ppl_dns_ip_free (ppl_dns_ip_t * ip)
{
  if (ip == NULL)
    return -1;
  osip_free (ip->name);
  if (ip->addrinfo!=NULL)
    {
      freeaddrinfo(ip->addrinfo);
    }
  osip_free(ip);
  return 0;
}

PPL_DECLARE (int) ppl_dns_ip_clone (ppl_dns_ip_t * ip, ppl_dns_ip_t ** dest)
{
  ppl_dns_ip_t *tmp;
  int i;
  int len;

  *dest = NULL;
  if (ip == NULL || ip->name == NULL)
    return -1;
  i = ppl_dns_ip_init (&tmp);
  if (i != 0)
    return -1;
  tmp->srv_ns_flag = ip->srv_ns_flag;
  tmp->ttl = ip->ttl;
  tmp->name = osip_strdup (ip->name);

  if (ip->addrinfo->ai_canonname==NULL) /* no cannonname */
    len = sizeof(struct addrinfo)
      + sizeof (struct sockaddr_in);
  else
    len = sizeof(struct addrinfo)
      + sizeof (struct sockaddr_in)
      + strlen(ip->name) +1;
  
  tmp->addrinfo = (struct addrinfo *) osip_malloc(len);

  memcpy(tmp->addrinfo, ip->addrinfo, len); /* only the first one */
  /* FIXME: We have to loop on all addrinfo! */

  tmp->pref = ip->pref;
  tmp->port = ip->port;
  tmp->weight = ip->weight;
  tmp->rweight = ip->rweight;
  tmp->next = NULL;
  tmp->parent = NULL;
  *dest = tmp;
  return 0;
}


PPL_DECLARE (void) ppl_dns_entry_free (ppl_dns_entry_t * dns)
{
  ppl_dns_ip_t *ip;

  if (dns == NULL)
    return;
  osip_free (dns->name);
  osip_free (dns->protocol);
  for (ip = dns->dns_ips; ip != NULL; ip = dns->dns_ips)
    {
      REMOVE_ELEMENT (dns->dns_ips, ip);
      if (ip->addrinfo!=NULL)
	{
	  freeaddrinfo(ip->addrinfo);
	}
      osip_free (ip->name);
      osip_free (ip);
    }
  osip_free (dns);
  return;
}


PPL_DECLARE (void) ppl_dns_entry_add_ref (ppl_dns_entry_t * dns)
{
  if (dns == NULL)
    return;
  dns->ref++;
}

PPL_DECLARE (void) ppl_dns_entry_remove_ref (ppl_dns_entry_t * dns)
{
  if (dns == NULL)
    return;
  dns->ref--;
}

PPL_DECLARE (ppl_status_t) ppl_dns_close ()
{
  ppl_dns_entry_t *dns;
  ppl_dns_error_t *err;

  for (dns = dns_results; dns != NULL; dns = dns_results)
    {
      REMOVE_ELEMENT (dns_results, dns);
      ppl_dns_entry_free (dns);
    }
  for (err = dns_errors; err != NULL; err = dns_errors)
    {
      REMOVE_ELEMENT (dns_errors, err);
      osip_free (err->domain);
      osip_free (err);
    }
  osip_mutex_destroy (m_dns_result);

  osip_fifo_free (dns_entries);

  /* endhostent(); */
  return 0;
}


PPL_DECLARE (void) ppl_dns_remove_entry (ppl_dns_entry_t * dns)
{
  ppl_dns_ip_t *d;

  if (dns->ref == 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO4, NULL,
		   "DNS cache entry removed: %s \n", dns->name));
      REMOVE_ELEMENT (dns_results, dns);
      osip_free (dns->name);
      osip_free (dns->protocol);
      for (d = dns->dns_ips; d != NULL; d = dns->dns_ips)
	{
	  REMOVE_ELEMENT (dns->dns_ips, d);
	  if (d->addrinfo!=NULL)
	    {
	      freeaddrinfo(d->addrinfo);
	    }
	  osip_free (d->name);
	  osip_free (d);
	}
      osip_free (dns);
    }
  else
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_WARNING, NULL,
		   "DNS cache entry not removed: %s\n", dns->name));
    }
}


static ppl_status_t
ppl_dns_resolv (ppl_dns_ip_t ** dns_ips, querybuf * answer, int n)
{
  ppl_dns_ip_t **array;
  int ancount, qdcount;		/* answer count and query count */
  HEADER *hp;			/* answer buffer header */
  char hostbuf[256];
  unsigned char *msg, *eom, *cp;	/* answer buffer positions */
  int dlen, type, aclass, pref, weight, port;
  long ttl;
  int answerno;

  *dns_ips = NULL;

  hp = (HEADER *) answer;
  qdcount = ntohs (hp->qdcount);
  ancount = ntohs (hp->ancount);

  msg = (unsigned char *) answer;
  eom = (unsigned char *) answer + n;
  cp = (unsigned char *) answer + sizeof (HEADER);

  while (qdcount-- > 0 && cp < eom)
    {
      n = dn_expand (msg, eom, cp, (char *) hostbuf, 256);
      if (n < 0)
	return -1;
      cp += n + QFIXEDSZ;
    }

  array = (ppl_dns_ip_t **) malloc (ancount * sizeof (ppl_dns_ip_t *));
  for (n = 0; n < ancount; n++)
    array[n] = NULL;
  answerno = 0;

  /* loop through the answer buffer and extract SRV records */
  while (ancount-- > 0 && cp < eom)
    {
      n = dn_expand (msg, eom, cp, (char *) hostbuf, 256);
      if (n < 0)
	{
	  for (n = 0; n < answerno; n++)
	    {
	      osip_free (array[n]->name);
	      (void) free (array[n]);
	    }
	  (void) free (array);
	  return -1;
	}

      cp += n;


#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
    defined(OLD_NAMESER) || defined(__FreeBSD__)
      type = _get_short (cp);
      cp += sizeof (u_short);
#elif defined(__APPLE_CC__)
      GETSHORT(type, cp);
#else
      NS_GET16 (type, cp);
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
    defined(OLD_NAMESER) || defined(__FreeBSD__)
      aclass = _get_short (cp);
      cp += sizeof (u_short);
#elif defined(__APPLE_CC__)
      GETSHORT(aclass, cp);
#else
      NS_GET16 (aclass, cp);
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
    defined(OLD_NAMESER) || defined(__FreeBSD__)
      ttl = _get_long (cp);
      cp += sizeof (u_long);
#elif defined(__APPLE_CC__)
      GETLONG(ttl, cp);
#else
      NS_GET32 (ttl, cp);
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
    defined(OLD_NAMESER) || defined(__FreeBSD__)
      dlen = _get_short (cp);
      cp += sizeof (u_short);
#elif defined(__APPLE_CC__)
      GETSHORT(dlen, cp);
#else
      NS_GET16 (dlen, cp);
#endif

      if (type != T_SRV)
	{
	  cp += dlen;
	  continue;
	}
#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
    defined(OLD_NAMESER) || defined(__FreeBSD__)
      pref = _get_short (cp);
      cp += sizeof (u_short);
#elif defined(__APPLE_CC__)
      GETSHORT(pref, cp);
#else
      NS_GET16 (pref, cp);
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
    defined(OLD_NAMESER) || defined(__FreeBSD__)
      weight = _get_short (cp);
      cp += sizeof (u_short);
#elif defined(__APPLE_CC__)
      GETSHORT(weight, cp);
#else
      NS_GET16 (weight, cp);
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__) ||\
    defined(OLD_NAMESER) || defined(__FreeBSD__)
      port = _get_short (cp);
      cp += sizeof (u_short);
#elif defined(__APPLE_CC__)
      GETSHORT(port, cp);
#else
      NS_GET16 (port, cp);
#endif

      n = dn_expand (msg, eom, cp, (char *) hostbuf, 256);
      if (n < 0)
	break;
      cp += n;

      array[answerno] = (ppl_dns_ip_t *) osip_malloc (sizeof (ppl_dns_ip_t));
      array[answerno]->srv_ns_flag = PSP_SRV_LOOKUP;
      array[answerno]->ttl = ttl;
      array[answerno]->pref = pref;
      array[answerno]->weight = weight;
      if (weight)
	array[answerno]->rweight = 1 + random () % (10000 * weight);
      else
	array[answerno]->rweight = 0;
      array[answerno]->port = port;
      array[answerno]->next = NULL;
      array[answerno]->name = osip_strdup ((char *) hostbuf);

      answerno++;
    }

  if (answerno == 0)
    {
      return -1;
    }

  qsort (array, answerno, sizeof (ppl_dns_ip_t *), compare);

  /* Recreate a linked list from the sorted array... */
  array[0]->parent = NULL;
  for (n = 0; n < answerno; n++)
    {
      if (n != 0)
	array[n]->parent = array[n - 1];
      array[n]->next = array[n + 1];
    }

  array[answerno - 1]->next = NULL;

  *dns_ips = array[0];
  (void) free (array);
  return 0;
}


PPL_DECLARE (ppl_status_t) ppl_dns_error_add (char *address)
{
  ppl_dns_error_t *error;

  if (address == NULL)
    return -1;
  error = (ppl_dns_error_t *) osip_malloc (sizeof (ppl_dns_error_t));
  error->domain = address;
  error->expires = time(NULL);
  error->next = NULL;
  error->parent = NULL;
  ppl_dns_lock_result_access ();
  ADD_ELEMENT (dns_errors, error);
  ppl_dns_unlock_result_access ();
  return 0;
}

PPL_DECLARE (ppl_status_t)
ppl_dns_get_error (ppl_dns_error_t ** dns_error, char *domain)
{
  ppl_dns_error_t *error;
  *dns_error = NULL;
  ppl_dns_lock_result_access ();

  for (error = dns_errors;
       error != NULL && 0 != strcmp (error->domain, domain);
       error = error->next)
    {
    }
  if (error == NULL)
    {
      ppl_dns_unlock_result_access ();
      return -1;
    }
  /* REMOVE_ELEMENT(dns_results, res); We could keep it for a while? */
  ppl_dns_unlock_result_access ();
  *dns_error = error;
  return 0;
}

/**
 * Add a result in the list of known result
 * @param dns the the dns result to add.
 */
PPL_DECLARE (ppl_status_t) ppl_dns_add_domain_result (ppl_dns_entry_t * dns)
{
  if (dns == NULL)
    return -1;
  ppl_dns_lock_result_access ();
  ADD_ELEMENT (dns_results, dns);
  ppl_dns_unlock_result_access ();
  return 0;
}

/**
 * A an entry in the list of known result
 * @param domain Domain to search
 */
PPL_DECLARE (ppl_status_t) ppl_dns_add_domain_query (char *domain)
{
  if (domain == NULL)
    return -1;
  osip_fifo_add (dns_entries, domain);
  return 0;
}

PPL_DECLARE (ppl_status_t)
ppl_dns_get_result (ppl_dns_entry_t ** dns, char *domain)
{
  ppl_dns_entry_t *res;
  ppl_dns_entry_t *resnext;
  ppl_time_t now;

  now = ppl_time ();
  *dns = NULL;
  ppl_dns_lock_result_access ();

  res = dns_results;
  resnext = res;
  { /* free some unusfull memory */
    ppl_dns_error_t *error;
    ppl_dns_error_t *enext;
    int now;
    now = time(NULL);
    for (error = dns_errors;
	 error != NULL;
	 error = enext)
      {
	enext = error->next;
	if ( now-error->expires > 120 )
	  {
	    OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
				    "release old bad resolution info for '%s'\n",
				    error->domain));
	    REMOVE_ELEMENT (dns_errors, error);
	    osip_free(error->domain);
	    osip_free(error);
	  }
      }
  }
  
  for (; res != NULL && 0 != strcmp (res->name, domain); res = resnext)
    {
      resnext = res->next;
      if (res->dns_ips != NULL)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO4, NULL,
		       "time to live: %li, date: %i\n",
		       res->dns_ips->ttl, res->date));
	  if (now - res->date > res->dns_ips->ttl)	/* expired entry! */
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_INFO4, NULL,
			   "ENTRY REMOVED1\n"));
	      ppl_dns_remove_entry (res);
	    }
	}
    }
  if (res == NULL)
    {
      ppl_dns_unlock_result_access ();
      return -1;
    }

  if (res->dns_ips != NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO4, NULL,
		   "time to live: %li, date: %i\n",
		   res->dns_ips->ttl, res->date));
      if (now - res->date > res->dns_ips->ttl)	/* expired entry! */
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_INFO4, NULL,
		       "ENTRY REMOVED2\n"));
	  ppl_dns_remove_entry (res);
	  res = NULL;
	  ppl_dns_unlock_result_access ();
	  return -1;
	}
    }

  /*  REMOVE_ELEMENT(dns_results, res); We could keep it for a while? */
  ppl_dns_unlock_result_access ();
  *dns = res;
  return 0;
}


PPL_DECLARE (ppl_status_t)
ppl_dns_query (ppl_dns_entry_t ** dest, char *domain, char *protocol)
{
  ppl_dns_entry_t *dns;
  ppl_dns_ip_t *next;
  ppl_dns_ip_t *ip;
  querybuf answer;		/* answer buffer from nameserver */
  int n;
  char *zone;

  *dest = NULL;

  if (!domain || !*domain || !protocol || !*protocol)
    return -1;

  zone = (char *) malloc (strlen (domain) + 5 + strlen (protocol) + 20);
  if (zone == NULL)
    return -1;

  *zone = '\0';

  strcat (zone, "_sip");
  strcat (zone, ".");
  strcat (zone, "_");
  strcat (zone, protocol);
  strcat (zone, ".");
  strcat (zone, domain);

  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO2, NULL,
	       "About to ask for '%s IN SRV'\n", zone));

  n =
    res_query (zone, C_IN, T_SRV, (unsigned char *) &answer, sizeof (answer));

  if (n < (int) sizeof (HEADER))
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "Did not get a valid response for query '%s IN SRV'\n",
		   zone));
      (void) free (zone);
      return -1;
    }

  OSIP_TRACE (osip_trace
	      (__FILE__, __LINE__, OSIP_INFO2, NULL,
	       "Got valid response for query '%s IN SRV'\n", zone));

  (void) free (zone);

  dns = (ppl_dns_entry_t *) osip_malloc (sizeof (ppl_dns_entry_t));

  dns->name = domain;
  dns->protocol = osip_strdup(protocol);
  dns->date = ppl_time ();
  dns->dns_ips = NULL;
  dns->ref = 0;
  dns->next = NULL;
  dns->parent = NULL;

  n = ppl_dns_resolv (&(dns->dns_ips), &answer, n);
  if (n != 0)
    {
      dns->name = NULL; /* fix bug: keep the name allocated after a failure. */
      ppl_dns_entry_free (dns);
      return -1;
    }

  for (ip = dns->dns_ips; ip != NULL; ip = ip->next)
    {
      n = ppl_dns_get_addrinfo (&(ip->addrinfo), ip->name, ip->port);
      if (n != PPL_SUCCESS)
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
				  "Hostname '%s' does not resolve",
				  ip->name));
	  if (ip->addrinfo!=NULL)
	    {
	      freeaddrinfo(ip->addrinfo);
	    }
	  osip_free (ip->name);
	  ip->name = NULL;	/* mark for deletion */
	}
    }

  /* delete ppl_dns_ip_t elements marked for deletion */

  for (ip = dns->dns_ips; ip != NULL; ip = next)
    {
      if (ip->name == NULL)
	{
	  next = ip->next;
	  REMOVE_ELEMENT (dns->dns_ips, ip);
	  osip_free (ip);
	}
      else
	{
	  next = ip->next;
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
				  "Hostname '%s' resolved to '%s'\n",
				  ip->name, ip->addrinfo->ai_canonname));
	}
    }
  *dest = dns;

  return 0;
}

#define BUFLEN 512
PPL_DECLARE (char *)
ppl_inet_ntop (struct sockaddr *sockaddr)
{
  char buf[BUFLEN];
  char *ptr = NULL;
  switch (sockaddr->sa_family) {
  case AF_INET:
    ptr = (char*)inet_ntop(sockaddr->sa_family,
			   &((struct sockaddr_in *)sockaddr)->sin_addr,
			   buf, BUFLEN-1);
    break;
  case AF_INET6:
    ptr = (char*)inet_ntop(sockaddr->sa_family,
			   &((struct sockaddr_in6 *)sockaddr)->sin6_addr,
			   buf, BUFLEN-1);
    break;
  default:
    return NULL;
  }
  if (ptr==NULL)
    return NULL;
  return osip_strdup(buf);
}

PPL_DECLARE (int)
ppl_inet_pton (const char *src, void *dst)
{
  if (strchr (src, ':')) /* possible IPv6 address */
	return (inet_pton(AF_INET6, src, dst));
  else if (strchr (src, '.')) /* possible IPv4 address */
	return (inet_pton(AF_INET, src, dst));
  else /* Impossibly a valid ip address */
	return INADDR_NONE;
}

PPL_DECLARE (ppl_status_t)
ppl_dns_get_addrinfo (struct addrinfo **addrinfo, char *hostname, int service)
{
  struct in_addr addr;
  struct in6_addr addrv6;
  struct addrinfo hints;
  int error;
  char portbuf[10];
  if (service!=0)
    snprintf(portbuf, sizeof(portbuf), "%d", service);

  memset (&hints, 0, sizeof (hints));
#if 0
  if (inet_pton(AF_INET, hostname, &addr)>0)
    {
      /* ipv4 address detected */
      struct addrinfo *_addrinfo;
      _addrinfo = (struct addrinfo *)osip_malloc(sizeof(struct addrinfo)
						 + sizeof (struct sockaddr_in)
						 + 0); /* no cannonname */
      _addrinfo->ai_flags = AI_NUMERICHOST;
      _addrinfo->ai_family = AF_INET;
      _addrinfo->ai_socktype = SOCK_DGRAM;
      _addrinfo->ai_protocol = IPPROTO_UDP;
      _addrinfo->ai_addrlen = sizeof(struct sockaddr_in);
      _addrinfo->ai_addr = (void *) (_addrinfo) + sizeof (struct addrinfo);
      
      memset(_addrinfo->ai_addr, 0, sizeof(struct sockaddr_in));
      ((struct sockaddr_in*)_addrinfo->ai_addr)->sin_family = AF_INET;
      ((struct sockaddr_in*)_addrinfo->ai_addr)->sin_addr.s_addr = inet_addr(hostname);
      if (service==0)
	((struct sockaddr_in*)_addrinfo->ai_addr)->sin_port   = htons (5060);
      else
	((struct sockaddr_in*)_addrinfo->ai_addr)->sin_port   = htons (service);
      _addrinfo->ai_canonname = NULL;
      _addrinfo->ai_next = NULL;
      
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "No DNS resolution needed for %s:%i\n", hostname, service));
      *addrinfo = _addrinfo;
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "IPv4 address detected: %s\n", hostname));
      return 0;
    }
  else if (inet_pton(AF_INET6, hostname, &addrv6)>0)
    {
      hints.ai_flags = AI_CANONNAME;
      hints.ai_family = PF_INET6; /* ipv6 support */
    }
  else
#endif
    {
      hints.ai_flags = AI_CANONNAME;
      hints.ai_family = PF_UNSPEC; /* ipv4 & ipv6 support */
#if 0
  hints.ai_family = PF_UNSPEC; /* ipv4 & ipv6 support */
#endif
    }

  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  if (service==0)
    {
      error = getaddrinfo (hostname, "sip", &hints, addrinfo);
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "SRV resolution with udp-sip-%s\n", hostname));
    }
  else
    {
      error = getaddrinfo (hostname, portbuf, &hints, addrinfo);
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "DNS resolution with %s:%i\n", hostname, service));
    }
  if (error || *addrinfo == NULL)
    { 
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "getaddrinfo failure. %s:%i\n", hostname, service));
     return -1;
    }
  /*
    fprintf (stdout, "canonnical name is: %s\n", (*addrinfo)->ai_canonname);
    fprintf (stdout, "ai_addrlen is: %i\n", (*addrinfo)->ai_addrlen);
  */
  /* memcpy (sin, res0->ai_addr, res0->ai_addrlen); */
  /* freeaddrinfo (res0); */

  return PPL_SUCCESS;
}

PPL_DECLARE (int)
ppl_dns_query_host (ppl_dns_entry_t ** dest, char *hostname, int port)
{
  struct addrinfo *addr;
  ppl_dns_ip_t *dns_ip;
  ppl_dns_entry_t *dns;
  int my_error;
  char portbuf[10];

  *dest = NULL;

  snprintf(portbuf, sizeof(portbuf), "%d", port);

  my_error = ppl_dns_get_addrinfo (&addr, hostname, port);
  if (my_error)
    return my_error;

  dns_ip = (ppl_dns_ip_t *) osip_malloc (sizeof (ppl_dns_ip_t));

  dns_ip->srv_ns_flag = PSP_NS_LOOKUP;
  dns_ip->ttl = 60;
  dns_ip->pref = 10;
  dns_ip->weight = 0;
  dns_ip->rweight = 0;
  dns_ip->port = port;
  dns_ip->next = NULL;
  dns_ip->parent = NULL;

  /*  dns_ip->sin.sin_family = addr.sin_family; */
  /*  dns_ip->sin.sin_addr = addr.sin_addr; */
  /*  dns_ip->sin.sin_port = htons (port); */
  dns_ip->addrinfo = addr;
  /* dns_ip->name = osip_strdup(addr->ai_canonname); */
  dns_ip->name = ppl_inet_ntop ((struct sockaddr *)(addr->ai_addr));

  if (dns_ip->name==NULL)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "getaddrinfo failure. could not get printable version\n"));
	  return -1;
    }
    
  dns = (ppl_dns_entry_t *) osip_malloc (sizeof (ppl_dns_entry_t));
  if (dns == NULL)
    {
      /* MLEAK! I have to free first_ip and following dns_ip!! */
      if (dns_ip->addrinfo!=NULL)
	{
	  freeaddrinfo(dns_ip->addrinfo);
	}
      osip_free (dns_ip->name);
      osip_free (dns_ip);
      return -1;
    }
  dns->name = hostname;
  dns->protocol = NULL;
  dns->date = ppl_time ();
  dns->dns_ips = dns_ip;
  dns->ref = 0;
  dns->next = NULL;
  dns->parent = NULL;

  *dest = dns;
  return 0;

}


#include <sys/ioctl.h>
#include <net/if.h>

static int ppl_dns_default_gateway_ipv4 (char *address, int size);
static int ppl_dns_default_gateway_ipv6 (char *address, int size);

int
ppl_dns_default_gateway (int familiy, char *address, int size)
{
  if (familiy==AF_INET6)
    {
      return ppl_dns_default_gateway_ipv6 (address, size);
    }
  else
    {
      return ppl_dns_default_gateway_ipv4 (address, size);
    }
}

/* This is a portable way to find the default gateway.
 * The ip of the default interface is returned.
 */
static int
ppl_dns_default_gateway_ipv4 (char *address, int size)
{
  unsigned int len;
  int sock_rt, on=1;
  struct sockaddr_in iface_out;
  struct sockaddr_in remote;
  
  memset(&remote, 0, sizeof(struct sockaddr_in));

  remote.sin_family = AF_INET;
  remote.sin_addr.s_addr = inet_addr("217.12.3.11");
  remote.sin_port = htons(11111);
  
  memset(&iface_out, 0, sizeof(iface_out));
  sock_rt = socket(AF_INET, SOCK_DGRAM, 0 );
  
  if (setsockopt(sock_rt, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on))
      == -1) {
    perror("DEBUG: [get_output_if] setsockopt(SOL_SOCKET, SO_BROADCAST");
    close(sock_rt);
    return -1;
  }
  
  if (connect(sock_rt, (struct sockaddr*)&remote, sizeof(struct sockaddr_in))
      == -1 ) {
    perror("DEBUG: [get_output_if] connect");
    close(sock_rt);
    return -1;
  }
  
  len = sizeof(iface_out);
  if (getsockname(sock_rt, (struct sockaddr *)&iface_out, &len) == -1 ) {
    perror("DEBUG: [get_output_if] getsockname");
    close(sock_rt);
    return -1;
  }
  close(sock_rt);
  if (iface_out.sin_addr.s_addr == 0)
    { /* what is this case?? */
      return -1;
    }
  osip_strncpy(address, inet_ntoa(iface_out.sin_addr), size-1);
  return 0;
}


/* This is a portable way to find the default gateway.
 * The ip of the default interface is returned.
 */
static int
ppl_dns_default_gateway_ipv6 (char *address, int size)
{
  unsigned int len;
  int sock_rt, on=1;
  struct sockaddr_in6 iface_out;
  struct sockaddr_in6 remote;
  
  memset(&remote, 0, sizeof(struct sockaddr_in));

  remote.sin6_family = AF_INET6;
  inet_pton(AF_INET6, "2001:638:500:101:2e0:81ff:fe24:37c6",
	    &remote.sin6_addr);
  remote.sin6_port = htons(11111);
  
  memset(&iface_out, 0, sizeof(iface_out));
  sock_rt = socket(AF_INET6, SOCK_DGRAM, 0 );
  
  if (setsockopt(sock_rt, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on))
      == -1) {
    perror("DEBUG: [get_output_if] setsockopt(SOL_SOCKET, SO_BROADCAST");
    close(sock_rt);
    return -1;
  }
  
  if (connect(sock_rt, (struct sockaddr*)&remote, sizeof(struct sockaddr_in6))
      == -1 ) {
    perror("DEBUG: [get_output_if] connect");
    close(sock_rt);
    return -1;
  }
  
  len = sizeof(iface_out);
  if (getsockname(sock_rt, (struct sockaddr *)&iface_out, &len) == -1 ) {
    perror("DEBUG: [get_output_if] getsockname");
    close(sock_rt);
    return -1;
  }
  close(sock_rt);

  if (iface_out.sin6_addr.s6_addr == 0)
    { /* what is this case?? */
      return -1;
    }
  inet_ntop(AF_INET6, (const void*) &iface_out.sin6_addr, address, size-1);
  return 0;
}

static int ppl_dns_get_local_fqdn_ipv6 (char **servername, char **serverip,
					char **netmask, char *interface,
					unsigned int pos_interface);
static int ppl_dns_get_local_fqdn_ipv4 (char **servername, char **serverip,
					char **netmask, char *interface,
					unsigned int pos_interface);

PPL_DECLARE (int)
ppl_dns_get_local_fqdn (char **servername, char **serverip,
			char **netmask, char *interface,
			unsigned int pos_interface, int family)
{
  if (family==AF_INET6)
    {
      return ppl_dns_get_local_fqdn_ipv6(servername, serverip, netmask,
					 interface, pos_interface);
    }
  else
    {
      return ppl_dns_get_local_fqdn_ipv4(servername, serverip, netmask,
					 interface, pos_interface);
    }
}

#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__APPLE_CC__) || defined (__linux)

#include <ifaddrs.h>


static int
ppl_dns_get_local_fqdn_ipv4 (char **servername, char **serverip,
			char **netmask, char *interface,
			unsigned int pos_interface)
{
  int  i = 0;

  struct ifaddrs *ifap, *ifa;
  *servername = NULL;
  *serverip = NULL;
  *netmask = NULL;

  if (getifaddrs(&ifap))
    return -1; /* cannot fetch interfaces */

  if (pos_interface>0) /* The method must return the interface at pos_interface	*/
    {
      i = 0;
      for (ifa = ifap; ifa; ifa=ifa->ifa_next)
	{
	  if (ifa->ifa_addr==NULL || ifa->ifa_addr->sa_family!=AF_INET)
	    {
	      /* skip non AF_INET interface */
	      pos_interface++;
	    }
	  else
	    {
	      if (i+1==pos_interface)
		{
		  char *atmp;
		  atmp = inet_ntoa (((struct sockaddr_in *) ifa->ifa_addr)->sin_addr);
		  *servername = osip_strdup (atmp);
		  *serverip = osip_strdup (atmp);
		  atmp = inet_ntoa (((struct sockaddr_in *) ifa->ifa_netmask)->sin_addr);
		  *netmask = osip_strdup (atmp);
		  freeifaddrs(ifap);
		  return 0;
		}
	    }
	  i++;
	}
      freeifaddrs(ifap);
      return -1;
    }

  if (interface!=NULL) /* The method must return the interface named interface */
    {
      for (ifa = ifap; ifa; ifa=ifa->ifa_next)
	{
	  if (ifa->ifa_addr!=NULL && ifa->ifa_addr->sa_family==AF_INET
	      && 0==strcmp(ifa->ifa_name, interface))
	    {
	      char *atmp;
	      atmp = inet_ntoa (((struct sockaddr_in *) ifa->ifa_addr)->sin_addr);
	      *servername = osip_strdup (atmp);
	      *serverip = osip_strdup (atmp);
	      atmp = inet_ntoa (((struct sockaddr_in *) ifa->ifa_netmask)->sin_addr);
	      *netmask = osip_strdup (atmp);
	      freeifaddrs(ifap);
	      return 0;
	    }
	}
      freeifaddrs(ifap);
      return -1;
    }

  *serverip = (char*)malloc(50);
  ppl_dns_default_gateway_ipv4 (*serverip, 49);
  if (*serverip==NULL)
    {
      freeifaddrs(ifap);
      return -1;
    }
  *servername = osip_strdup(*serverip);

  /* look for the mask */
  for (ifa = ifap; ifa; ifa=ifa->ifa_next)
    {
      if (ifa->ifa_addr!=NULL && ifa->ifa_addr->sa_family==AF_INET)
	{
	  char *atmp;
	  atmp = inet_ntoa (((struct sockaddr_in *) ifa->ifa_addr)->sin_addr);
	  if (0==strcmp(*serverip, atmp))
	    {
	      atmp = inet_ntoa (((struct sockaddr_in *) ifa->ifa_netmask)->sin_addr);
	      *netmask = osip_strdup (atmp);
	      freeifaddrs(ifap);
	      return 0;
	    }
	}
    }

  /* We might not need the mask? */
  freeifaddrs(ifap);
  return 0;
}

static int
ppl_dns_get_local_fqdn_ipv6 (char **servername, char **serverip,
			     char **netmask, char *interface,
			     unsigned int pos_interface)
{
  int  i = 0;

  struct ifaddrs *ifap, *ifa;
  *servername = NULL;
  *serverip = NULL;
  *netmask = NULL;

  if (getifaddrs(&ifap))
    return -1; /* cannot fetch interfaces */

  if (pos_interface>0) /* The method must return the interface at pos_interface	*/
    {
      i = 0;
      for (ifa = ifap; ifa; ifa=ifa->ifa_next)
	{
	  if (ifa->ifa_addr==NULL || ifa->ifa_addr->sa_family!=AF_INET6)
	    {
	      /* skip non AF_INET interface */
	      pos_interface++;
	    }
	  else
	    {
	      if (i+1==pos_interface)
		{
		  *servername = (char*) osip_malloc(50);
		  inet_ntop(AF_INET6, (const void*) &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr, (*servername), 49);

		  *serverip = osip_strdup (*servername);
		  *netmask = (char*) osip_malloc(50);
		  inet_ntop(AF_INET6, (const void*) &((struct sockaddr_in6 *)ifa->ifa_netmask)->sin6_addr, (*netmask), 49);
		  freeifaddrs(ifap);
		  return 0;
		}
	    }
	  i++;
	}
      freeifaddrs(ifap);
      return -1;
    }

  if (interface!=NULL) /* The method must return the interface named interface */
    {
      for (ifa = ifap; ifa; ifa=ifa->ifa_next)
	{
	  if (ifa->ifa_addr!=NULL && ifa->ifa_addr->sa_family==AF_INET6
	      && 0==strcmp(ifa->ifa_name, interface))
	    {
	      *servername = (char*) osip_malloc(50);
	      inet_ntop(AF_INET6, (const void*) &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr, (*servername), 49);
			
	      *serverip = osip_strdup (*servername);
	      *netmask = (char*) osip_malloc(50);
	      inet_ntop(AF_INET6, (const void*) &((struct sockaddr_in6 *)ifa->ifa_netmask)->sin6_addr, (*netmask), 49);
	      freeifaddrs(ifap);
	      return 0;
	    }
	}
      freeifaddrs(ifap);
      return -1;
    }

  *serverip = (char*)malloc(50);
  ppl_dns_default_gateway_ipv6 (*serverip, 49);
  if (*serverip==NULL)
    {
      freeifaddrs(ifap);
      return -1;
    }
  *servername = osip_strdup(*serverip);

  /* look for the mask */
  for (ifa = ifap; ifa; ifa=ifa->ifa_next)
    {
      if (ifa->ifa_addr!=NULL && ifa->ifa_addr->sa_family==AF_INET6)
	{
	  char atmp[50];
	  inet_ntop(AF_INET6, (const void*) &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr, atmp, 49);
	  if (0==strcmp(*serverip, atmp))
	    {
	      *netmask = (char*) osip_malloc(50);
	      inet_ntop(AF_INET6, (const void*) &((struct sockaddr_in6 *)ifa->ifa_netmask)->sin6_addr, *netmask, 49);
	      freeifaddrs(ifap);
	      return 0;
	    }
	}
    }

  /* We might not need the mask? */
  freeifaddrs(ifap);
  return 0;
}

#else

#if defined(__sun__)
#include <sys/sockio.h>
#endif

static int ppl_dns_get_local_fqdn_ipv4 (char **servername, char **serverip,
					char **netmask, char *interface,
					unsigned int pos_interface)
{
  struct ifconf netconf;
  int sock, err, if_count, i, j = 0;
  char tmp[160];
  char fqdn_proposal[10][21];
  char ip_proposal[10][21];
  char def_gateway[50];

  *servername = NULL;
  *serverip = NULL;
  *netmask = NULL;

  netconf.ifc_len = 160;
  netconf.ifc_buf = tmp;

  sock = ppl_socket (PF_INET, SOCK_DGRAM, 0);
  err = ioctl (sock, SIOCGIFCONF, &netconf);
  if (err < 0)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_WARNING, NULL,
			      "ioctl failed! cannot detect the ip address!\n"));
      ppl_socket_close (sock);
      return -1;
    }
  ppl_socket_close (sock);
  if_count = netconf.ifc_len / 32;

  if (pos_interface>0) /* The method must return the interface at pos_interface	*/
    {
      for (i = 0; i < if_count; i++)
	{
	  struct ifreq *ifr;
	  ifr = &(netconf.ifc_req[i]);
	  if (i+1==pos_interface)
	    {
	      char *atmp;
	      atmp = inet_ntoa (((struct sockaddr_in *) (&ifr->ifr_addr))->sin_addr);
	      *servername = osip_strdup (atmp);
	      *serverip = osip_strdup (atmp);
#ifdef __linux
	      {
		sock = ppl_socket (PF_INET, SOCK_DGRAM, 0);
		if (sock <= 0)
		  {
		    OSIP_TRACE (osip_trace
				(__FILE__, __LINE__, OSIP_WARNING, NULL,
				 "ioctl failed! cannot detect the netmask for %s!\n",
				 *serverip));
		    return 0; /* we probably don't need the mask? */
		  }
		else if (ioctl (sock, SIOCGIFNETMASK, /*&if_data */ &(netconf.ifc_req[i])) < 0)
		  {
		    OSIP_TRACE (osip_trace
				(__FILE__, __LINE__, OSIP_WARNING, NULL,
				 "ioctl failed! cannot detect the netmask for %s!\n",
				 *serverip));
		  }
		else
		  {
		    struct in_addr in;
		    in.s_addr =
		      *((unsigned long *) &netconf.ifc_req[i].ifr_netmask.sa_data[2]);
		    /* *((unsigned long *) &if_data.ifr_netmask.sa_data[2]); */
		    *netmask = osip_strdup (inet_ntoa (in));
		  }
		ppl_socket_close (sock);
	      }
#endif
	      return 0; /* even if the mask detection has failed? */
	    }
	}
      return -1; /* even if the mask detection has failed? */
    }


  for (i = 0; i < if_count; i++)
    {
      if (interface != NULL
	  && strcmp (netconf.ifc_req[i].ifr_name, interface) == 0)
	{
	  char *atmp;
	  atmp = inet_ntoa (((struct sockaddr_in *) (&netconf.ifc_req[i].
						     ifr_addr))->sin_addr);
	  strncpy (fqdn_proposal[0], netconf.ifc_req[i].ifr_name, 30);
	  strncpy (ip_proposal[0], atmp, 30);
	  *servername = osip_strdup (ip_proposal[0]);
	  *serverip = osip_strdup (ip_proposal[0]);
#ifdef __linux
	  {
	    struct ifreq if_data;
	    strcpy (if_data.ifr_name, interface);
	    sock = ppl_socket (PF_INET, SOCK_DGRAM, 0);
	    if (sock <= 0)
	      {
		OSIP_TRACE (osip_trace
			    (__FILE__, __LINE__, OSIP_WARNING, NULL,
			     "ioctl failed! cannot detect the netmask for %s!\n",
			     atmp));
	      }
	    else if (ioctl (sock, SIOCGIFNETMASK, &if_data) < 0)
	      {
		OSIP_TRACE (osip_trace
			    (__FILE__, __LINE__, OSIP_WARNING, NULL,
			     "ioctl failed! cannot detect the netmask for %s!\n",
			     atmp));
	      }
	    else
	      {
		struct in_addr in;
		in.s_addr =
		  *((unsigned long *) &if_data.ifr_netmask.sa_data[2]);
		*netmask = osip_strdup (inet_ntoa (in));
	      }
	    ppl_socket_close (sock);
	  }
#endif
	  return 0;
	}
      if (interface == NULL &&
	  strcmp (netconf.ifc_req[i].ifr_name, "lo") != 0)
	{
	  char *atmp;

	  atmp =
	    inet_ntoa (((struct sockaddr_in *) (&netconf.ifc_req[i].
						ifr_addr))->sin_addr);
	  strncpy (fqdn_proposal[j], netconf.ifc_req[i].ifr_name, 20);
	  strncpy (ip_proposal[j], atmp, 20);
	  j++;
	}
    }
  if (interface != NULL)
    return -1;

  if (j == 1)
    {
      *servername = osip_strdup (ip_proposal[0]);
      *serverip = osip_strdup (ip_proposal[0]);
#ifdef __linux
      {
	struct ifreq if_data;
	strcpy (if_data.ifr_name, fqdn_proposal[0]);
	sock = ppl_socket (PF_INET, SOCK_DGRAM, 0);
	if (sock <= 0)
	  {
	    OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_WARNING, NULL,
				    "ioctl failed! cannot detect the netmask for %s!\n",
				    fqdn_proposal[0]));
	  }
	else if (ioctl (sock, SIOCGIFNETMASK, &if_data) < 0)
	  {
	    OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_WARNING, NULL,
				    "ioctl failed! cannot detect the netmask for %s!\n",
				    fqdn_proposal[0]));
	  }
	else
	  {
	    struct in_addr in;
	    in.s_addr = *((unsigned long *) &if_data.ifr_netmask.sa_data[2]);
	    *netmask = osip_strdup (inet_ntoa (in));
	  }
	ppl_socket_close (sock);
      }
#endif
      return PPL_SUCCESS;
    }
  else
    {
      ppl_dns_default_gateway_ipv4 (def_gateway, 49);
      if (def_gateway != NULL)
	{
	  for (i = 0; i < j; i++)
	    {
	      if (strcmp (ip_proposal[i], def_gateway) == 0)
		{
		  *servername = osip_strdup (ip_proposal[i]);
		  *serverip = osip_strdup (ip_proposal[i]);
#ifdef __linux
		  {
		    struct ifreq if_data;
		    strcpy (if_data.ifr_name, fqdn_proposal[i]);
		    sock = ppl_socket (PF_INET, SOCK_DGRAM, 0);
		    if (sock <= 0)
		      {
			OSIP_TRACE (osip_trace
				    (__FILE__, __LINE__, OSIP_WARNING, NULL,
				     "ioctl failed! cannot detect the netmask for %s!\n",
				     fqdn_proposal[i]));
		      }
		    else if (ioctl (sock, SIOCGIFNETMASK, &if_data) < 0)
		      {
			OSIP_TRACE (osip_trace
				    (__FILE__, __LINE__, OSIP_WARNING, NULL,
				     "ioctl failed! cannot detect the netmask for %s!\n",
				     fqdn_proposal[i]));
			perror ("socket failed");
		      }
		    else
		      {
			struct in_addr in;
			in.s_addr = *((unsigned long *)
				      &if_data.ifr_netmask.sa_data[2]);
			*netmask = osip_strdup (inet_ntoa (in));
		      }
		    ppl_socket_close (sock);
		  }
#endif
		  return PPL_SUCCESS;
		}
	    }
	}
    }
  return -1;
}

static int ppl_dns_get_local_fqdn_ipv6 (char **servername, char **serverip,
					char **netmask, char *interface,
					unsigned int pos_interface)
{
  *servername = NULL;
  *serverip = NULL;
  *netmask = NULL;

  /* TODO */
  return -1;
}
#endif
