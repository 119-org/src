/*
  The partysip program is a modular SIP proxy server (SIP -rfc3261-)
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  Partysip is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  Partysip is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <partysip/partysip.h>
#include <partysip/psp_request.h>
#include <partysip/osip_msg.h>

#include <ppl/ppl_md5.h>
#include <ppl/ppl_dns.h>

psp_core_t *core = NULL;

int psp_core_parse_servername (char *buf);
static int psp_core_parse_serveripv4 (char *buf);
static int psp_core_parse_serveripv6 (char *buf);

int
psp_core_init ()
{
  char *tmp;
  int i;

  core = (psp_core_t *) osip_malloc (sizeof (psp_core_t));
  if (core == NULL)
    return -1;

  memset (core, 0, sizeof (psp_core_t));

  tmp = psp_config_get_element ("banner");
#ifdef VERSION
  if (tmp!=NULL && strlen(tmp)>1)
    snprintf(core->banner, 128, "%s/%s", tmp, VERSION);
#elif defined(WIN32)
  if (tmp!=NULL && strlen(tmp)>1)
    _snprintf(core->banner, 128, "%s/X.X.X", tmp);
#endif

#ifdef DEFAULT_T1
  i = psp_osip_init (&(core->psp_osip), DEFAULT_T1);
#else
  i = psp_osip_init (&(core->psp_osip), 500);
#endif
  if (i != 0)
    goto mci_error1;

  core->ipv6_enable = 0;
  tmp = psp_config_get_element ("ipv6_enable");
  if (tmp!=NULL && 0 == osip_strncasecmp (tmp, "on",2))
    core->ipv6_enable = 1;

  tmp = psp_config_get_element ("recovery_delay");
  if (tmp != NULL)
    core->recovery_delay = osip_atoi(tmp);
  else
    core->recovery_delay = -1;

  i = pspm_resolv_init (&(core->resolv));
  if (i != 0)
    goto mci_error2;

  i = pspm_tlp_init (&(core->tlp));
  if (i != 0)
    goto mci_error3;

  i = pspm_sfp_init (&(core->sfp));
  if (i != 0)
    goto mci_error5;

  if (core->ipv6_enable == 1)
    {
      tmp = psp_config_get_element ("serverip6");
      if (tmp == NULL)
	goto mci_error6;
      if (psp_core_parse_serveripv6 (tmp) != 0)
	goto mci_error6;
    }

  tmp = psp_config_get_element ("serverip");
  if (tmp == NULL)
    goto mci_error6;
  if (psp_core_parse_serveripv4 (tmp) != 0)
    goto mci_error6;

  tmp = psp_config_get_element ("servername");
  if (tmp == NULL)
    goto mci_error7;
  if (psp_core_parse_servername (tmp) != 0)
    goto mci_error7;

  core->iptables_dynamic_natrule = 0;	/* only allowed on linux */
  core->ext_ip = NULL;
  core->ext_fqdn = NULL;
  core->ext_mask = NULL;
  core->lan_ip = NULL;
  core->lan_fqdn = NULL;
  core->lan_mask = NULL;
  for (i=0;i<PSP_NAT_MAX_CALL;i++)
    {
      *(core->fw_entries[i].hash) = '\0';
      *(core->fw_entries[i].lan_ip) = '\0';
      *(core->fw_entries[i].lan_port) = '\0';
      *(core->fw_entries[i].remove_rule) = '\0';
      core->fw_entries[i].birth_date = 0;
      core->fw_entries[i].call_length = 0;
    }

  core->masquerade_via = 0;
  tmp = psp_config_get_element ("masquerade_via");
  if (tmp != NULL && 0 == strcmp (tmp, "on"))
    {
      core->masquerade_via = 1;
    }

  core->masquerade_sdp = 0;
  tmp = psp_config_get_element ("masquerade_sdp");
  if (tmp != NULL && 0 == strcmp (tmp, "on"))
    {
      core->masquerade_sdp = 1;
    }

  core->remote_natip = NULL;

  tmp = psp_config_get_element ("remote_natip");
  core->remote_natip = tmp;
  if ((core->masquerade_via==1
       || core->masquerade_sdp==1)
      && tmp != NULL)
    {
      int i;
      struct addrinfo *nataddr;
      i = ppl_dns_get_addrinfo (&nataddr,
			       core->remote_natip, 5060);
      freeaddrinfo(nataddr);
      if (i!=0)
	{
	  /* masquerading is disabled */
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       " SIP masquerading is disabled: (could not resolv: %s)\n",
		       core->remote_natip));
	  core->remote_natip = NULL;
	  core->masquerade_via = 0;
	  core->masquerade_sdp = 0;
	}
    }

  tmp = psp_config_get_element ("iptables_dynamic_natrule");
  if (tmp != NULL && 0 == strcmp (tmp, "on"))
    {
      core->iptables_dynamic_natrule = 1;	/* enable NAT feature */

      tmp = psp_config_get_element ("iptables_server");
      if (tmp == NULL)
	goto mci_error8;
      core->ipt_server = tmp;

      tmp = psp_config_get_element ("iptables_port");
      if (tmp == NULL)
	goto mci_error8;
      core->ipt_port = atoi(tmp);

      i = ppl_dns_get_addrinfo (&(core->ipt_addr),
			       core->ipt_server, osip_atoi(tmp));
      if (i!=0)
	{
	  /* masquerading is disabled */
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "dynamic NAT rule is disabled: (could not resolv: %s)\n",
		       core->remote_natip));
	  core->ipt_server = NULL;
	  core->iptables_dynamic_natrule = 0;
	}
    }

  if (core->iptables_dynamic_natrule == 1
      ||core->masquerade_via == 1
      ||core->masquerade_sdp == 1)
    { /* so we need all masks and ips */
      tmp = psp_config_get_element ("if_extip");
      if (tmp != NULL)
	{	
	  core->ext_fqdn = NULL;
	  core->ext_ip = osip_strdup(tmp);
	  tmp = psp_config_get_element ("if_extmask");
	  if (tmp == NULL)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_ERROR, NULL,
			   "Please specify a if_extmask parameter in the config file\n"));
	      osip_free(core->ext_ip);
	      goto mci_error8;
	    }
	  core->ext_mask = osip_strdup(tmp);
	  
	  core->lan_fqdn = NULL;
	  tmp = psp_config_get_element ("if_lanip");
	  if (tmp == NULL)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_ERROR, NULL,
			   "Please specify a if_lanip parameter in the config file\n"));
	      osip_free(core->ext_ip);
	      osip_free(core->lan_mask);
	      goto mci_error8;
	    }
	  core->lan_ip = osip_strdup(tmp);
	  
	  tmp = psp_config_get_element ("if_lanmask");
	  if (tmp == NULL)
	    {
	      OSIP_TRACE (osip_trace
			  (__FILE__, __LINE__, OSIP_ERROR, NULL,
			   "Please specify a if_lanmask parameter in the config file\n"));
	      osip_free(core->ext_ip);
	      osip_free(core->lan_mask);
	      osip_free(core->lan_ip);
	      goto mci_error8;
	    }
	  core->lan_mask = osip_strdup(tmp);
	}
      else
	{
#ifdef __linux
	  char *tmp_name;
	  char *tmp_ip;
	  char *tmp_mask;
	  tmp = psp_config_get_element ("if_ext");
	  if (tmp == NULL)
	    goto mci_error8;
	  if (0 == ppl_dns_get_local_fqdn (&tmp_name, &tmp_ip, &tmp_mask, tmp, 0, AF_INET))
	    { }
	  else if (0 != ppl_dns_get_local_fqdn (&tmp_name, &tmp_ip, &tmp_mask, tmp, 0, AF_INET6))
	    goto mci_error8;
	  core->ext_ip = tmp_ip;
	  core->ext_fqdn = tmp_name;
	  core->ext_mask = tmp_mask;
	  if (tmp_mask == NULL)
	    goto mci_error9;
	  tmp = psp_config_get_element ("if_lan");
	  if (tmp == NULL)
	    goto mci_error9;
	  if (0 == ppl_dns_get_local_fqdn (&tmp_name, &tmp_ip, &tmp_mask, tmp, 0, AF_INET))
	    {}
	  else if (0 != ppl_dns_get_local_fqdn (&tmp_name, &tmp_ip, &tmp_mask, tmp, 0, AF_INET6))
	    goto mci_error9;
	  core->lan_ip = tmp_ip;
	  core->lan_fqdn = tmp_name;
	  core->lan_mask = tmp_mask;
	  if (tmp_mask == NULL)
	    goto mci_error10;
#else
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "Define if_lanip, if_lanmask, if_extip, if_extmask in the config file to enable SIP masquerading\n"));
	    goto mci_error10;
#endif
	}
      
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
			      "NAT enabled %s(public)/%s -> %s(internal)/%s\n",
			      core->ext_ip, core->ext_mask,
			      core->lan_ip, core->lan_mask));
    }


  tmp = psp_config_get_element ("serverport_udp");
  if (tmp == NULL)
    core->port = 5060;
  else
    {
      char *end;
      end = strchr (tmp, ' ');
      if (end != NULL)
	*end = '\0';		/* discard any additionnal element */
      core->port = osip_atoi (tmp);
    }

  tmp = psp_config_get_element ("disable_check_for_osip_to_tag_in_cancel");
  if (tmp != NULL && 0 == strcmp (tmp, "on"))
    core->disable_check_for_osip_to_tag_in_cancel = 1;

  tmp = psp_config_get_element ("rfc3327");
  if (tmp != NULL && 0 == strcmp (tmp, "on"))
    core->rfc3327 = 1;

  core->gl_lock = osip_mutex_init ();
  if (core->gl_lock == NULL)
    goto mci_error10;

  return 0;

mci_error10:
  osip_free (core->lan_ip);
  osip_free (core->lan_fqdn);
  osip_free (core->lan_mask);
#ifdef __linux
mci_error9:
#endif
  osip_free (core->ext_ip);
  osip_free (core->ext_fqdn);
  osip_free (core->ext_mask);
mci_error8:
mci_error7:
  for (i = 0; i < MAX_SERVER_IDENTIFIERS && core->servername[i] != NULL; i++)
    {
      osip_free (core->servername[i]);
      core->servername[i] = NULL;
    }
mci_error6:
  for (i = 0; i < MAX_SERVER_IDENTIFIERS && core->serverip[i] != NULL; i++)
    {
      osip_free (core->serverip[i]);
      core->serverip[i] = NULL;
    }
  for (i = 0; i < MAX_SERVER_IDENTIFIERS && core->serverip6[i] != NULL; i++)
    {
      osip_free (core->serverip6[i]);
      core->serverip6[i] = NULL;
    }
  pspm_sfp_free (core->sfp);
mci_error5:
  pspm_tlp_free (core->tlp);
mci_error3:
  pspm_resolv_free (core->resolv);
mci_error2:
  psp_osip_free (core->psp_osip);
mci_error1:
  osip_free (core);
  core = NULL;
  return -1;
}

int
psp_core_load_all_ipv4()
{
  int cond;
  int i;
  char allips[255];
  char *tmp;
  
  char *servername;
  char *serverip;
  char *netmask;
  char def_gateway[50];
  
  serverip = psp_config_get_element ("serverip");
  if (serverip!=NULL)
    return 0; /* should not be defined for win32 and linux */
  
  memset(&allips,'\0',sizeof(allips));
  
  memset(&def_gateway, '\0',sizeof(def_gateway));
  i = ppl_dns_default_gateway(AF_INET, def_gateway, 49);
  if (i!=0)
    {
      fprintf(stdout, "Default Gateway Interface detection failed. Please define \"serverip\" in the config file\n");
      return -1;
    }
  osip_strncpy(allips, def_gateway, strlen(def_gateway));
  
  /* the MAIN ip address is used in Via header
	It MUST always be added at the beginning of serverip values. */
  
  tmp = allips + strlen(allips);
  
  cond = 0;
  for (i=1;cond==0;i++) /* index of interface start at "1" */
    {
      cond = ppl_dns_get_local_fqdn (&servername, &serverip, &netmask, NULL, i, AF_INET);
      if (cond==0)
	{
	  if (0!=strncmp(serverip, allips, strlen(serverip)))
	    { /* do not add the default gateway */
	      osip_strncpy(tmp, ",", 1);
	      tmp++;
	      osip_strncpy(tmp, serverip, strlen(serverip));
	      tmp = tmp + strlen(tmp);
	    }

	  fprintf(stdout, "Found new address : %s/%s/%s\n",
		  servername, serverip, netmask);
	  osip_free(servername);
	  osip_free(serverip);
	  osip_free(netmask);
	}
    }

  /*
    OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
    "Here is the detected list of ip address: %s\n",
    allips));
  */

  /* finally add the list of ordered ip address */
  psp_config_add_element (osip_strdup ("serverip"),
			  osip_strdup (allips));

  return 0;
}

int
psp_core_load_all_ipv6()
{
  int cond;
  int i;
  char allips[255];
  char *tmp;
  
  char *servername;
  char *serverip;
  char *netmask;
  char def_gateway[50];
  
  serverip = psp_config_get_element ("serverip6");
  if (serverip!=NULL)
    return 0; /* should not be defined for win32 and linux */
  
  memset(&allips,'\0',sizeof(allips));
  
  memset(&def_gateway, '\0',sizeof(def_gateway));
  i = ppl_dns_default_gateway(AF_INET6, def_gateway, 49);
  if (i!=0)
    {
      fprintf(stdout, "Default Gateway Interface detection failed. Please define \"serverip\" in the config file\n");
      return -1;
    }
  osip_strncpy(allips, def_gateway, strlen(def_gateway));
  
  /* the MAIN ip address is used in Via header
	It MUST always be added at the beginning of serverip values. */
  
  tmp = allips + strlen(allips);
  
  cond = 0;
  for (i=1;cond==0;i++) /* index of interface start at "1" */
    {
      cond = ppl_dns_get_local_fqdn (&servername, &serverip, &netmask, NULL, i, AF_INET6);
      if (cond==0)
	{
	  if (0!=strncmp(serverip, allips, strlen(serverip)))
	    { /* do not add the default gateway */
	      osip_strncpy(tmp, ",", 1);
	      tmp++;
	      osip_strncpy(tmp, serverip, strlen(serverip));
	      tmp = tmp + strlen(tmp);
	    }

	  fprintf(stdout, "Found new address : %s/%s/%s\n",
		  servername, serverip, netmask);
	  osip_free(servername);
	  osip_free(serverip);
	  osip_free(netmask);
	}
    }

  /*
    OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
    "Here is the detected list of ip address: %s\n",
    allips));
  */

  /* finally add the list of ordered ip address */
  psp_config_add_element (osip_strdup ("serverip6"),
			  osip_strdup (allips));

  return 0;
}


int
psp_core_parse_servername (char *buf)
{
  char *c;
  size_t token_len = 0;
  int n = 0;

  c = buf;
  while (*c != '\0')
    {
      token_len = strcspn (c, " ,");
      if (token_len == 0)
	{
	  /* the token is only separators
	   * (example, buf = 'foo.example.com, bar.example.com'
	   *  and we are now here : ---------^
	   */
	  c++;
	}
      else
	{
	  if (n <= MAX_SERVER_IDENTIFIERS)
	    {
	      int need_bytes = token_len + 1;

	      /* now we need to check if there are any trailing dots
	       * that we should strip.
	       */
	      while (*(c + need_bytes - 2) == '.')
		need_bytes--;

	      /* XXX check if this token resolves? */

	      core->servername[n] = osip_malloc (need_bytes);
	      if (core->servername[n] == NULL)
		return -1;

	      osip_strncpy ((char *) core->servername[n], c, need_bytes - 1);
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
				      "servername token %i: '%s'\n", n,
				      core->servername[n]));
	      c += token_len;
	    }
	  else
	    {
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				      "more than %i servernames\n",
				      MAX_SERVER_IDENTIFIERS));
	      return -1;
	    }
	  n++;
	}
    }
  return 0;			/* OK */
}

static int
psp_core_parse_serveripv4 (char *buf)
{
  char *c;
  size_t token_len = 0;
  int n = 0;

  c = buf;
  while (*c != '\0')
    {
      token_len = strcspn (c, " ,");
      if (token_len == 0)
	{
	  /* the token is only separators, just advance past it */
	  c++;
	}
      else
	{
	  if (n <= MAX_SERVER_IDENTIFIERS)
	    {
	      int need_bytes = token_len + 1;

	      core->serverip[n] = osip_malloc (need_bytes);
	      if (core->serverip[n] == NULL)
		return -1;

	      osip_strncpy ((char *) core->serverip[n], c, need_bytes - 1);
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
				      "serverip token %i: '%s'\n", n,
				      core->serverip[n]));
	      c += token_len;

	      if (inet_addr (core->serverip[n]) == -1)
		{
		  OSIP_TRACE (osip_trace
			      (__FILE__, __LINE__, OSIP_ERROR, NULL,
			       "serverip '%s' is not a valid IP address\n",
			       core->serverip[n]));
		  return -1;
		}
	    }
	  else
	    {
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				      "more than %i serverips\n",
				      MAX_SERVER_IDENTIFIERS));
	      return -1;
	    }
	  n++;
	}
    }
  return 0;			/* OK */
}

static int
psp_core_parse_serveripv6 (char *buf)
{
  char *c;
  size_t token_len = 0;
  int n = 0;

  c = buf;
  while (*c != '\0')
    {
      token_len = strcspn (c, " ,");
      if (token_len == 0)
	{
	  /* the token is only separators, just advance past it */
	  c++;
	}
      else
	{
	  struct in6_addr addr;
	  if (n <= MAX_SERVER_IDENTIFIERS)
	    {
	      int need_bytes = token_len + 1;

	      core->serverip6[n] = osip_malloc (need_bytes);
	      if (core->serverip6[n] == NULL)
		return -1;

	      osip_strncpy ((char *) core->serverip6[n], c, need_bytes - 1);
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
				      "serverip6 token %i: '%s'\n", n,
				      core->serverip6[n]));
	      c += token_len;

	      if (ppl_inet_pton ((const char *)core->serverip6[n], (void *)&addr) <= 0)
		{
		  OSIP_TRACE (osip_trace
			      (__FILE__, __LINE__, OSIP_ERROR, NULL,
			       "serverip6 '%s' is not a valid IP address\n",
			       core->serverip6[n]));
		  return -1;
		}
	    }
	  else
	    {
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				      "more than %i serverip6s\n",
				      MAX_SERVER_IDENTIFIERS));
	      return -1;
	    }
	  n++;
	}
    }
  return 0;			/* OK */
}

static void *
mythread_resolv (pspm_resolv_t * resolv)
{
  int i;

  i = pspm_resolv_execute (resolv, 5, 0, -1);	/* infinite loop */
  if (i != 0)
    return NULL;
  return NULL;
}

static void *
mythread_tlp (pspm_tlp_t * tlp)
{
  int i;

  i = pspm_tlp_execute (tlp, 5, 0, -1);	/* infinite loop */
  if (i != 0)
    return NULL;
  return NULL;
}

static void *
mythread_sfp (pspm_sfp_t * sfp)
{
  int i;

  i = pspm_sfp_execute (sfp, 5, 0, -1);	/* infinite loop */
  if (i != 0)
    return NULL;
  return NULL;
}

int
psp_core_start (int use_this_thread)
{
  int i;

  i = pspm_resolv_start (core->resolv, (void *(*)(void *)) mythread_resolv,
			 core->resolv);
  if (i != 0)
    return -1;

  i = pspm_tlp_start (core->tlp, (void *(*)(void *)) mythread_tlp, core->tlp);
  if (i != 0)
    return -1;

  if (use_this_thread == 1) /* interactive mode: start a new thread as usual */
    {
      i =
	pspm_sfp_start (core->sfp, (void *(*)(void *)) mythread_sfp,
			core->sfp);
      if (i != 0)
	return -1;
    }
  else
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
			      "starting the sfp module with the main thread\n"));
      mythread_sfp (core->sfp);
      /* will never return until partysip is closed */
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "shuting down partysip\n"));
    }

  return 0;
}

int
psp_core_execute ()
{
  int i;

  i = pspm_resolv_execute (core->resolv, 0, 0, 1);
  if (i != 0)
    goto pce_error1;

  i = pspm_tlp_execute (core->tlp, 0, 0, 1);
  if (i != 0)
    goto pce_error1;

  i = pspm_sfp_execute (core->sfp, 0, 0, 1);
  if (i != 0)
    goto pce_error1;

  return 0;
pce_error1:

  return -1;
}

void
psp_core_free ()
{
  int i;

  if (core == NULL)
    return;
  /* first: shutdown threads */
  psp_osip_release (core->psp_osip);	/* all transaction threads will die */
  pspm_resolv_release (core->resolv);	/* The resolver thread will die */
  pspm_tlp_release (core->tlp);	/* The tlp thread will die */

  if (core->sfp == NULL || core->sfp->module == NULL)
    return;
  if (core->sfp->module->thread == NULL)	/* this is not a thread! */
    {
      char q[2] = "q";
      i = ppl_pipe_write (core->sfp->module->wakeup, &q, 1);
      if (i != 1)
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_ERROR, NULL,
		       "could not write in pipe!\n"));
	  return;
	}
      osip_usleep (3000000);	/* as it's not a thread, we don't know
				   when this module will exit */
    }
  else
    {
      pspm_sfp_release (core->sfp);	/* The sfp thread will die */
    }

  /* done */

  /* here NO thread should be active */

  /* unregister other plugins and their methods */
  psp_core_remove_all_hook ();
  /* remove all plugins */
  psp_core_remove_all_plugins ();

  pspm_resolv_free (core->resolv);

  pspm_tlp_free (core->tlp);

  pspm_sfp_free (core->sfp);

  psp_osip_free (core->psp_osip);

  for (i = 0; i < MAX_SERVER_IDENTIFIERS && core->servername[i] != NULL; i++)
    {
      osip_free (core->servername[i]);
      core->servername[i] = NULL;
    }
  for (i = 0; i < MAX_SERVER_IDENTIFIERS && core->serverip[i] != NULL; i++)
    {
      osip_free (core->serverip[i]);
      core->serverip[i] = NULL;
    }

  osip_free (core->lan_ip);
  osip_free (core->lan_fqdn);
  osip_free (core->lan_mask);

  osip_free (core->ext_ip);
  osip_free (core->ext_fqdn);
  osip_free (core->ext_mask);

  osip_mutex_destroy (core->gl_lock);

  osip_free (core);
}

int
psp_core_lock ()
{
  return osip_mutex_lock (core->gl_lock);
}

int
psp_core_unlock ()
{
  return osip_mutex_unlock (core->gl_lock);
}

void
psp_core_remove_all_plugins ()
{
  {
    tlp_plugin_t *p;

    for (p = core->tlp->tlp_plugins; p != NULL; p = core->tlp->tlp_plugins)
      {
	core->tlp->tlp_plugins = p->next;
	tlp_plugin_free (p);
      }
  }
  {
    sfp_plugin_t *p;

    for (p = core->sfp->sfp_plugins; p != NULL; p = core->sfp->sfp_plugins)
      {
	REMOVE_ELEMENT (core->sfp->sfp_plugins, p);
	sfp_plugin_free (p);
      }
  }
}

void
psp_core_remove_all_hook ()
{
  /* remove sfp_inc hooks */
  sfp_inc_func_tab_free (core->sfp->rcv_invites);
  sfp_inc_func_tab_free (core->sfp->rcv_registers);
  sfp_inc_func_tab_free (core->sfp->rcv_byes);
  sfp_inc_func_tab_free (core->sfp->rcv_optionss);
  sfp_inc_func_tab_free (core->sfp->rcv_infos);
  sfp_inc_func_tab_free (core->sfp->rcv_cancels);
  sfp_inc_func_tab_free (core->sfp->rcv_notifys);
  sfp_inc_func_tab_free (core->sfp->rcv_subscribes);
  sfp_inc_func_tab_free (core->sfp->rcv_unknowns);

  core->sfp->rcv_invites = NULL;
  core->sfp->rcv_registers = NULL;
  core->sfp->rcv_byes = NULL;
  core->sfp->rcv_optionss = NULL;
  core->sfp->rcv_infos = NULL;
  core->sfp->rcv_cancels = NULL;
  core->sfp->rcv_notifys = NULL;
  core->sfp->rcv_subscribes = NULL;
  core->sfp->rcv_unknowns = NULL;

  sfp_fwd_func_tab_free (core->sfp->fwd_invites);
  sfp_fwd_func_tab_free (core->sfp->fwd_registers);
  sfp_fwd_func_tab_free (core->sfp->fwd_byes);
  sfp_fwd_func_tab_free (core->sfp->fwd_optionss);
  sfp_fwd_func_tab_free (core->sfp->fwd_infos);
  sfp_fwd_func_tab_free (core->sfp->fwd_cancels);
  sfp_fwd_func_tab_free (core->sfp->fwd_notifys);
  sfp_fwd_func_tab_free (core->sfp->fwd_subscribes);
  sfp_fwd_func_tab_free (core->sfp->fwd_unknowns);

  core->sfp->fwd_invites = NULL;
  core->sfp->fwd_registers = NULL;
  core->sfp->fwd_byes = NULL;
  core->sfp->fwd_optionss = NULL;
  core->sfp->fwd_infos = NULL;
  core->sfp->fwd_cancels = NULL;
  core->sfp->fwd_notifys = NULL;
  core->sfp->fwd_subscribes = NULL;
  core->sfp->fwd_unknowns = NULL;

  sfp_rcv_func_tab_free (core->sfp->rcv_1xxs);
  sfp_rcv_func_tab_free (core->sfp->rcv_2xxs);
  sfp_rcv_func_tab_free (core->sfp->rcv_3xxs);
  sfp_rcv_func_tab_free (core->sfp->rcv_4xxs);
  sfp_rcv_func_tab_free (core->sfp->rcv_5xxs);
  sfp_rcv_func_tab_free (core->sfp->rcv_6xxs);

  core->sfp->rcv_1xxs = NULL;
  core->sfp->rcv_2xxs = NULL;
  core->sfp->rcv_3xxs = NULL;
  core->sfp->rcv_4xxs = NULL;
  core->sfp->rcv_5xxs = NULL;
  core->sfp->rcv_6xxs = NULL;

  sfp_snd_func_tab_free (core->sfp->snd_1xxs);
  sfp_snd_func_tab_free (core->sfp->snd_2xxs);
  sfp_snd_func_tab_free (core->sfp->snd_3xxs);
  sfp_snd_func_tab_free (core->sfp->snd_4xxs);
  sfp_snd_func_tab_free (core->sfp->snd_5xxs);
  sfp_snd_func_tab_free (core->sfp->snd_6xxs);

  core->sfp->snd_1xxs = NULL;
  core->sfp->snd_2xxs = NULL;
  core->sfp->snd_3xxs = NULL;
  core->sfp->snd_4xxs = NULL;
  core->sfp->snd_5xxs = NULL;
  core->sfp->snd_6xxs = NULL;

}

int
psp_core_sfp_generate_branch_for_request (osip_message_t * request, char *branch)
{
  psp_core_default_generate_branch_for_request (request, branch);
  if (branch[0] == '\0')
    return -1;
  branch[7] = 'f';
  return 0;
}

int
psp_core_default_generate_branch_for_request (osip_message_t * request, char *branch)
{
  osip_via_t *via;
  osip_generic_param_t *b;
  via = osip_list_get (&request->vias, 0);
  osip_via_param_get_byname (via, "branch", &b);
  if (b != NULL && 0 == strncmp ("z9hG4bK", b->gvalue, 7))
    {
      /* compute hash with branchid */
      MD5_CTX Md5Ctx;
      HASH HA1;

      ppl_MD5Init (&Md5Ctx);
      ppl_MD5Update (&Md5Ctx, (unsigned char *) b->gvalue,
		     strlen (b->gvalue));
      ppl_MD5Final ((unsigned char *) HA1, &Md5Ctx);

      osip_strncpy (branch, "z9hG4bKl", 8);
      ppl_md5_hash_osip_to_hex (HA1, branch + 8);
    }
  else
    {				/* not compliant...: hash is more complex */
      char *topmostvia;
      char *cid;
      char *req_uri;
      int i;
      osip_generic_param_t *tag;
      MD5_CTX Md5Ctx;
      HASH HA1;

      ppl_MD5Init (&Md5Ctx);

      i = osip_via_to_str (via, &topmostvia);
      ppl_MD5Update (&Md5Ctx, (unsigned char *) topmostvia,
		     strlen (topmostvia));
      osip_free (topmostvia);
      ppl_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);

      osip_to_get_tag (request->to, &tag);
      if (tag != NULL)
	{
	  ppl_MD5Update (&Md5Ctx, (unsigned char *) tag->gvalue,
			 strlen (tag->gvalue));
	}
      ppl_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);

      osip_from_get_tag (request->from, &tag);
      if (tag != NULL)
	{
	  ppl_MD5Update (&Md5Ctx, (unsigned char *) tag->gvalue,
			 strlen (tag->gvalue));
	}
      else
	{
	  OSIP_TRACE (osip_trace
		      (__FILE__, __LINE__, OSIP_WARNING, NULL,
		       "remote UA is not compliant: no tag in the from\n"));
	}
      ppl_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);

      osip_call_id_to_str (request->call_id, &cid);
      ppl_MD5Update (&Md5Ctx, (unsigned char *) cid, strlen (cid));
      osip_free (cid);
      ppl_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);

      ppl_MD5Update (&Md5Ctx, (unsigned char *) request->cseq->number,
		     strlen (request->cseq->number));
      ppl_MD5Update (&Md5Ctx, (unsigned char *) ":", 1);

      osip_uri_to_str (request->req_uri, &req_uri);
      ppl_MD5Update (&Md5Ctx, (unsigned char *) req_uri, strlen (req_uri));
      osip_free (req_uri);
      ppl_MD5Final ((unsigned char *) HA1, &Md5Ctx);

      osip_strncpy (branch, "z9hG4bKl", 8);
      ppl_md5_hash_osip_to_hex (HA1, branch + 8);
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO4, NULL,
		   "Branch hash (remote client is not compliant): %s\n",
		   branch));
    }
  return 0;
}

PPL_DECLARE (char *) psp_core_get_ip_of_local_proxy ()
{
  char def_gateway[50];
  char *dyn_ip;

  if (core->ipv6_enable != 1)
    {
      /* only implemented for IPv4 */

      dyn_ip = psp_config_get_element ("dynamic_ip");
      if (dyn_ip!=NULL && 0==osip_strcasecmp(dyn_ip, "on"))
	{
	  int i;
	  memset(&def_gateway, '\0',sizeof(def_gateway));
	  i = ppl_dns_default_gateway(AF_INET, def_gateway, 49);
	  if (i!=0)
	    {
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
				      "Failed to re-detect default interface keep '%s'\n",
				      core->serverip[0]));
	    }
	  else
	    {
	      char *tmp = core->serverip[0];
	      core->serverip[0] = osip_malloc (50);
	      if (core->serverip[0] == NULL)
		core->serverip[0] = tmp; /* back to previous ip */
	      else
		{
		  osip_free(tmp);
		  osip_strncpy ((char *) core->serverip[0],
				def_gateway,
				strlen(def_gateway));
		  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO2, NULL,
					  "New default interface detected '%s'\n",
					  core->serverip[0]));
		}
	    }
	}
    }

  if (core->ipv6_enable == 1)
    {
      struct in6_addr addr;
      if (ppl_inet_pton ((const char *)core->serverip6[0], (void *)&addr) <= 0)
	{
	  if (ppl_inet_pton ((const char *)core->servername[0], (void *)&addr) <= 0)
	    return core->serverip6[0];
	  return core->servername[0];
	}
      return core->serverip6[0];
    }
  else
    {
      unsigned long int one_inet_addr;
      
      if ((int) (one_inet_addr = inet_addr (core->serverip[0])) == -1)
	{
	  if ((int) (one_inet_addr = inet_addr (core->servername[0])) == -1)
	    return core->serverip[0];
	  return core->servername[0];
	}
      return core->serverip[0];
    }
}

PPL_DECLARE (int) psp_core_get_port_of_local_proxy ()
{
  return core->port;
}

PPL_DECLARE (int) psp_core_is_responsible_for_this_domain (osip_uri_t * url)
{
  return psp_core_is_responsible_for_this_request_uri (url);
}

PPL_DECLARE (int) psp_core_is_responsible_for_this_request_uri (osip_uri_t * url)
{
  int port, is_an_ip, i;
  int match = 0;

  if (url == NULL)
    return -1;

  if (0 != strcmp (url->scheme, "sip") && 0 != strcmp (url->scheme, "sips"))
    return -1;			/* only sip url are allowed here */

  if (core->ipv6_enable == 1)
    {
      struct in6_addr addr;
      is_an_ip = ppl_inet_pton ((const char *)url->host, (void *)&addr);
      if (is_an_ip <= 0)
	is_an_ip = -1;
    }
  else
    is_an_ip = inet_addr (url->host);

  if (is_an_ip == -1)
    {
      /* is a hostname, search our list of names for this machine */
      
      for (i = 0; i < MAX_SERVER_IDENTIFIERS && core->servername[i] != NULL;
	   i++)
	{
	  if (!osip_strcasecmp (url->host, core->servername[i]))
	    {
	      match = 1;
	      break;
	    }
	}
    }
  else
    {
      /* is an IP address, search our list of IP addresses for this machine */

      for (i = 0; i < MAX_SERVER_IDENTIFIERS && core->serverip[i] != NULL;
	   i++)
	{
	  if (!strcmp (url->host, core->serverip[i]))
	    {
	      match = 1;
	      break;
	    }
	}
      if (match!=1)
	{
	  for (i = 0; i < MAX_SERVER_IDENTIFIERS && core->serverip6[i] != NULL;
	       i++)
	    {
	      if (!strcmp (url->host, core->serverip6[i]))
		{
		  match = 1;
		  break;
		}
	    }
	}
    }

  if (match)
    {
      /* If more than one proxy runs on this host, we have to check
         for the port. */

      /*  if the req_uri has not port specified
         then this is a request for this domain */
      if (url->port == NULL)
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
				  "psp_core_is_responsible_for_this_domain() '%s'\n",
				  url->host));
	  return 0;
	}

      if (core->port == 5060)
	{
	  /* in this case, the port MUST be the same than the one
	     used by the proxy */
	  if (url->port == NULL)
	    {
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
				      "psp_core_is_responsible_for_this_domain() '%s'\n",
				      url->host));
	      return 0;
	    }
	  port = osip_atoi (url->port);
	  if (5060 == port)
	    {
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
				      "psp_core_is_responsible_for_this_domain() '%s'\n",
				      url->host));
	      return 0;
	    }
	  return -1;
	}
      else
	{
	  /* if the port is specified in the domain, it can
	     be the same. */
	  port = osip_atoi (url->port);
	  if (core->port == port)
	    {
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
				      "psp_core_is_responsible_for_this_domain() '%s'\n",
				      url->host));
	      return 0;
	    }
	  return -1;
	}
      return -1;
    }

  return -1;
  /*  return psp_core_is_responsible_for_this_route (url); */
}

PPL_DECLARE (int) psp_core_is_responsible_for_this_route (osip_uri_t * url)
{
  int port, is_an_ip, i;
  int match = 0;

  if (url == NULL)
    return -1;

  if (0 != strcmp (url->scheme, "sip") && 0 != strcmp (url->scheme, "sips"))
    return -1;			/* only sip url are allowed here */

  if (core->ipv6_enable == 1)
    {
      struct in6_addr addr;
      is_an_ip = ppl_inet_pton ((const char *)url->host, (void *)&addr);
      if (is_an_ip <= 0)
	is_an_ip = -1;
    }
  else
    is_an_ip = inet_addr (url->host);

  if (is_an_ip == -1)
    {
      /* is a hostname, search our list of names for this machine */

      for (i = 0; i < MAX_SERVER_IDENTIFIERS && core->servername[i] != NULL;
	   i++)
	{
	  if (!osip_strcasecmp (url->host, core->servername[i]))
	    {
	      match = 1;
	      break;
	    }
	}
    }
  else
    {
      /* is an IP address, search our list of IP addresses for this machine */

      for (i = 0; i < MAX_SERVER_IDENTIFIERS && core->serverip[i] != NULL;
	   i++)
	{
	  if (!strcmp (url->host, core->serverip[i]))
	    {
	      match = 1;
	      break;
	    }
	}
      if (match!=1)
	{
	  for (i = 0; i < MAX_SERVER_IDENTIFIERS && core->serverip6[i] != NULL;
	       i++)
	    {
	      if (!strcmp (url->host, core->serverip6[i]))
		{
		  match = 1;
		  break;
		}
	    }
	}
    }

  if (match)
    {
      /* If more than one proxy runs on this host, we have to check
         for the port. */
      if (url->port != NULL)
	port = osip_atoi (url->port);
      else
	port = 5060;
      if (core->port == port)
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO4, NULL,
				  "psp_core_is_responsible_for_this_route() '%s'\n",
				  url->host));
	  return 0;
	}
      return -1;
    }
  return -1;
}


PPL_DECLARE (int) psp_core_find_osip_transaction_and_add_event (osip_event_t * evt)
{
  return osip_find_transaction_and_add_event (core->psp_osip->osip, evt);
}

/*
osip_transaction_t *
psp_core_osip_find_transaction(osip_event_t *evt)
{
  return osip_find_transaction(core->psp_osip->osip, evt);
}
*/

int
psp_core_cb_snd_message (osip_transaction_t * tr, osip_message_t * sip, char *host,
			 int port, int out_socket)
{
  if (MSG_IS_RESPONSE (sip) && host == NULL)
    {				/* automatic search of destination */
      /* if (reliable_transport) -> use existing connection
         .                     else use received
         .                     else use sent-by
         elseif (maddr is found) use the maddr(+use the indicated ttl if multicast)
         elseif (unreliable protocol) use received
         .                       else use sent-by        
       */
      osip_via_t *via;
      osip_generic_param_t *maddr;
      osip_generic_param_t *received;
      osip_generic_param_t *rport;
      via = osip_list_get (&sip->vias, 0);
      if (via == NULL)
	return -2;
      osip_via_param_get_byname (via, "maddr", &maddr);
      osip_via_param_get_byname (via, "received", &received);
      osip_via_param_get_byname (via, "rport", &rport);
      /* 1: skip the TCP implementation, partysip don't support it by now */
      /* 2: check maddr and multicast usage */
      if (maddr != NULL)
	host = maddr->gvalue;
      /* we should check if this is a multicast address and use
         set the "ttl" in this case. (this must be done in the
         UDP message (not at the SIP layer) */
      else if (received != NULL)
	host = received->gvalue;
      else
	host = via->host;

      if (rport == NULL || rport->gvalue == NULL)
	{
	  if (via->port != NULL)
	    port = osip_atoi (via->port);
	  else
	    port = 5060;
	}
      else
	port = osip_atoi (rport->gvalue);
    }
  return pspm_tlp_send_message (core->tlp, tr, sip, host, port, out_socket);
}


  /* the strict router issue: TODO:
     "The proxy MUST inspect the Request-URI of the request.  If the
     Request-URI of the request contains a value this proxy previously
     placed into a Record-Route header field (see Section 16.6 item 4),
     the proxy MUST replace the Request-URI in the request with the last
     value from the Route header field, and remove that value from the
     Route header field. The proxy MUST then proceed as if it received
     this modified request.
     ...
     This requirement does not obligate a proxy to keep state in
     order to detect URIs it previously placed in Record-Route
     header fields. Instead, a proxy need only place enough
     information in those URIs to recognize them as values it
     provided when they later appear."
   */

PPL_DECLARE (int) psp_core_fix_strict_router_issue (osip_event_t * evt)
{
  osip_uri_param_t *psp_param;

  if (MSG_IS_RESPONSE (evt->sip))
    return 0;

  osip_uri_uparam_get_byname (evt->sip->req_uri, "psp", &psp_param);
  if (psp_param != NULL && !osip_list_eol (&evt->sip->routes, 0))
    {				/* !! strict rooter detected! (compliant with old draft...)
				   We have to rewrite the request-uri and routes */
      osip_route_t *route;
      char *magicstring;
      int pos;

      magicstring = psp_config_get_element ("magicstring2");	/* Who I am? */
      if (0 == strcmp (magicstring, psp_param->gvalue))
	{			/* Yes, it's mine */
	  /* replace the rq-uri with the last route header */
	  int i;
	  osip_uri_t *oldurl;

	  pos = 0;
	  while (!osip_list_eol (&evt->sip->routes, pos))
	    pos++;
	  pos--;
	  osip_message_get_route (evt->sip, pos, &route);
	  osip_list_remove (&evt->sip->routes, pos);
	  oldurl = evt->sip->req_uri;

	  evt->sip->req_uri = route->url;
	  route->url = NULL;	/* avoid double free */
	  osip_route_free (route);

	  i = osip_route_init (&route);
	  if (i != 0)
	    return -1;
	  route->url = oldurl;
	  osip_list_add (&evt->sip->routes, route, 0);
	}
    }
  /* request is now compliant to the latest draft :-) */

  /* now we should check if the request-uri has an maddr paramater...
     (I won't receive such request until I add maddr and partysip-0.5.0 don't!)
   */
  return 0;
}

int
psp_core_handle_late_answer (osip_message_t * answer)
{
  int i;
  /*  int port;
     osip_via_t *via; */
  osip_message_t *answerxxx;

  i = osip_msg_sfp_build_response_osip_to_forward (&answerxxx, answer);
  if (i != 0)
    {
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_ERROR, NULL,
		   "xxx module: Can't build the late answer to forward!\n"));
      return -1;
    }

  /*
     via = osip_list_get (answerxxx->vias, 0);
     if (via == NULL)
     {
     osip_message_free (answerxxx);
     return -1;
     }
     if (via->port == NULL)
     port = 5060;
     else
     port = atoi (via->port);
     psp_core_cb_snd_message (NULL, answerxxx, via->host, port, -1);
   */
  psp_core_cb_snd_message (NULL, answerxxx, NULL, 5060, -1);

  osip_message_free (answerxxx);

  return 0;
}
