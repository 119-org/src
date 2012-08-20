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

#ifndef WIN32

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

#endif

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
  osip_free (ip);
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
      osip_free (ip->name);
      osip_free (ip);
    }

  osip_free(dns);
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
  /* Not sure this is the correct method to call?
     Elements allocated by gethsotbyname_r still exists! */
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
	  osip_free (d->name);
	  osip_free (d);
	}
      osip_free (dns);
    }
  else
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO4, NULL,
		   "DNS cache entry removed: %s \n", dns->name));
    }
}


/* I DON'T KNOW HOW TO SUPPORT SRV RECORD ON WINDOWS, TODO */

PPL_DECLARE (ppl_status_t)
ppl_dns_resolv (ppl_dns_ip_t ** dns_ips, querybuf * answer, int n)
{
  *dns_ips = NULL;
  return -1;
}

PPL_DECLARE (ppl_status_t)
ppl_dns_query (ppl_dns_entry_t ** dest, char *domain, char *protocol)
{
  *dest = NULL;
  return -1;
}

#define BUFLEN 512
PPL_DECLARE (char *)
ppl_inet_ntop (struct sockaddr *sockaddr)
{
  char *ptr = NULL;
  switch (sockaddr->sa_family) {
  case AF_INET:
	  ptr = inet_ntoa (((struct sockaddr_in *)sockaddr)->sin_addr);
	  break;
  case AF_INET6:
	  ptr = NULL;
      break;
  default:
    return NULL;
  }
  if (ptr==NULL)
    return NULL;
  return osip_strdup(ptr);
}

PPL_DECLARE (int)
ppl_inet_pton (const char *src, void *dst)
{
  if (strchr (src, ':')) /* possible IPv6 address */
	return -1; /* (inet_pton(AF_INET6, src, dst)); */
  else if (strchr (src, '.')) /* possible IPv4 address */
  {
	  ((struct sockaddr_in *)dst)->sin_port = 0;	/* to N. byte order */
	  ((struct sockaddr_in *)dst)->sin_family = AF_INET;
	  ((struct sockaddr_in *)dst)->sin_addr.s_addr = inet_addr(src);	/* already in N. byte order */

	  if (((struct sockaddr_in *)dst)->sin_addr.s_addr == INADDR_NONE)
		return 0;
	  return 1; /* (inet_pton(AF_INET, src, dst)); */
  }
  else /* Impossibly a valid ip address */
	return INADDR_NONE;
}

PPL_DECLARE (ppl_status_t) ppl_dns_error_add (char *address)
{
  ppl_dns_error_t *error;

  if (address == NULL)
    return -1;
  error = (ppl_dns_error_t *) osip_malloc (sizeof (ppl_dns_error_t));
  error->domain = address;
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
ppl_dns_get_addrinfo (struct addrinfo **addrinfo, char *hostname, int service)
{
  unsigned long int one_inet_addr;
  struct addrinfo hints;
  int error;
  char portbuf[10];
  if (service!=0)
    _snprintf(portbuf, sizeof(portbuf), "%d", service);

  memset (&hints, 0, sizeof (hints));
  if ((int) (one_inet_addr = inet_addr (hostname)) == -1)
    hints.ai_flags = AI_CANONNAME;
  else
    {
#if 0
      struct addrinfo *_addrinfo;
      _addrinfo = (struct addrinfo *)osip_malloc(sizeof(struct addrinfo)
					     + sizeof (struct sockaddr_in)
					     + 0); /* no cannonname */
      memset(_addrinfo, 0, sizeof(struct addrinfo) + sizeof (struct sockaddr_in) + 0);
      _addrinfo->ai_flags = AI_NUMERICHOST;
      _addrinfo->ai_family = AF_INET;
      _addrinfo->ai_socktype = SOCK_DGRAM;
      _addrinfo->ai_protocol = IPPROTO_UDP;
      _addrinfo->ai_addrlen = sizeof(struct sockaddr_in);
      _addrinfo->ai_addr = (void *) ((_addrinfo) + sizeof (struct addrinfo));

      ((struct sockaddr_in*)_addrinfo->ai_addr)->sin_family = AF_INET;
      ((struct sockaddr_in*)_addrinfo->ai_addr)->sin_addr.s_addr = one_inet_addr;
      if (service==0)
	((struct sockaddr_in*)_addrinfo->ai_addr)->sin_port   = htons (5060);
      else
	((struct sockaddr_in*)_addrinfo->ai_addr)->sin_port   = htons ((unsigned short)service);
      _addrinfo->ai_canonname = NULL;
      _addrinfo->ai_next = NULL;

      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "No DNS resolution needed for %s:%i\n", hostname, service));
      *addrinfo = _addrinfo;
      return 0;
#else
    hints.ai_flags = AI_NUMERICHOST;
#endif
  }

  hints.ai_family = PF_INET;
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

  _snprintf(portbuf, sizeof(portbuf), "%d", port);

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

static int ppl_dns_default_gateway_ipv4 (char *address, int size);
static int ppl_dns_default_gateway_ipv6 (char *address, int size);

PPL_DECLARE (int)
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

int
ppl_dns_default_gateway_ipv4 (char *address, int size)
{
	/* w2000 and W95/98 */
	unsigned long  best_interface_index;
	DWORD hr;

	/* NT4 (sp4 only?) */
	PMIB_IPFORWARDTABLE ipfwdt;
	DWORD siz_ipfwd_table = 0;
	unsigned int ipf_cnt;

	memset(address, '\0', size);

	best_interface_index = -1;
	/* w2000 and W95/98 only */
	hr = GetBestInterface(inet_addr("217.12.3.11"),&best_interface_index);
	if (hr)
	{
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			hr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR) &lpMsgBuf, 0, NULL);

		OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
					 "GetBestInterface: %s\r\n", lpMsgBuf));
		best_interface_index = -1;
	}

	if (best_interface_index != -1)
	{ /* probably W2000 or W95/W98 */
		char *servername;
		char *serverip;
		char *netmask;
		OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
					 "Default Interface found %i\r\n", best_interface_index));

		if (0 == ppl_dns_get_local_fqdn(&servername, &serverip, &netmask,
				NULL, best_interface_index, AF_INET))
		{
			osip_strncpy(address, serverip, size-1);
			osip_free(servername);
			osip_free(serverip);
			osip_free(netmask);
			return 0;
		}
		return -1;
	}


	if (!GetIpForwardTable(NULL, &siz_ipfwd_table, FALSE) == ERROR_INSUFFICIENT_BUFFER
		|| !(ipfwdt = (PMIB_IPFORWARDTABLE) alloca (siz_ipfwd_table)))
	{
		OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
			"Allocation error\r\n"));
		return -1;
	}


	/* NT4 (sp4 support only?) */
	if (!GetIpForwardTable(ipfwdt, &siz_ipfwd_table, FALSE))
	{
		for (ipf_cnt = 0; ipf_cnt < ipfwdt->dwNumEntries; ++ipf_cnt) 
		{
			if (ipfwdt->table[ipf_cnt].dwForwardDest == 0)
			{ /* default gateway found */
				char *servername;
				char *serverip;
				char *netmask;
				OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
					"Default Interface found %i\r\n", ipfwdt->table[ipf_cnt].dwForwardIfIndex));

				if (0 ==  ppl_dns_get_local_fqdn(&servername, &serverip, &netmask,
						NULL, ipfwdt->table[ipf_cnt].dwForwardIfIndex, AF_INET))
				{
					osip_strncpy(address, serverip, size-1);
					osip_free(servername);
					osip_free(serverip);
					osip_free(netmask);
					return 0;
				}
				return -1;
			}
		}

	}
	/* no default gateway interface found */
	return -1;
}

int
ppl_dns_default_gateway_ipv6 (char *address, int size)
{
	memset(address, '\0', size);
	return -1;
}

static int ppl_dns_get_local_fqdn_ipv6 (char **servername, char **serverip,
					char **netmask, char *_interface,
					unsigned int pos_interface);
static int ppl_dns_get_local_fqdn_ipv4 (char **servername, char **serverip,
					char **netmask, char *_interface,
					unsigned int pos_interface);

PPL_DECLARE (int)
ppl_dns_get_local_fqdn (char **servername, char **serverip,
			char **netmask, char *_interface,
			unsigned int pos_interface, int family)
{
  if (family==AF_INET6)
    {
      return ppl_dns_get_local_fqdn_ipv6(servername, serverip, netmask,
					 _interface, pos_interface);
    }
  else
    {
      return ppl_dns_get_local_fqdn_ipv4(servername, serverip, netmask,
					 _interface, pos_interface);
    }
}

static int
ppl_dns_get_local_fqdn_ipv4 (char **servername, char **serverip,
			char **netmask, char *UNIX_interface, unsigned int WIN32_interface)
{
	unsigned int pos;

	*servername = NULL; /* no name on win32? */
	*serverip   = NULL;
	*netmask    = NULL;

	/* First, try to get the interface where we should listen */
	{
		DWORD size_of_iptable = 0;
		PMIB_IPADDRTABLE ipt;
		PMIB_IFROW ifrow;

		if (GetIpAddrTable(NULL, &size_of_iptable, TRUE) == ERROR_INSUFFICIENT_BUFFER)
		{
			ifrow = (PMIB_IFROW) _alloca (sizeof(MIB_IFROW));
			ipt = (PMIB_IPADDRTABLE) _alloca (size_of_iptable);
			if (ifrow==NULL || ipt==NULL)
			{
				/* not very usefull to continue */
				OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
					"ERROR alloca failed\r\n"));
				return -1;
			}

			if (!GetIpAddrTable(ipt, &size_of_iptable, TRUE))
			{
				/* look for the best public interface */

				for (pos=0; pos < ipt->dwNumEntries && *netmask==NULL ; ++pos)
				{
					/* index is */
					struct in_addr addr;
					struct in_addr mask;
					ifrow->dwIndex = ipt->table[pos].dwIndex;
					if (GetIfEntry(ifrow) == NO_ERROR)
					{
						switch(ifrow->dwType)
						{
						case MIB_IF_TYPE_LOOPBACK:
						//	break;
						case MIB_IF_TYPE_ETHERNET:
						default:
							addr.s_addr = ipt->table[pos].dwAddr;
							mask.s_addr = ipt->table[pos].dwMask;
							if (ipt->table[pos].dwIndex == WIN32_interface)
							{
								*servername = NULL; /* no name on win32? */
								*serverip   = osip_strdup(inet_ntoa(addr));
								*netmask    = osip_strdup(inet_ntoa(mask));
								OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
									"Interface ethernet: %s/%s\r\n", *serverip, *netmask));
								break;
							}
						}
					}
				}
			}
		}
	}

	if (*serverip==NULL || *netmask==NULL)
	{
		OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_WARNING, NULL,
			"ERROR No network interface found\r\n"));
		return -1;
	}

	return 0;
}

static int
ppl_dns_get_local_fqdn_ipv6 (char **servername, char **serverip,
			char **netmask, char *UNIX_interface, unsigned int WIN32_interface)
{
	*servername = NULL;
	*serverip = NULL;
	*netmask = NULL;
	return -1;
}
