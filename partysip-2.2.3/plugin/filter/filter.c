/*
  The filter plugin is a GPL plugin for partysip.
  Copyright (C) 2002,2003  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002,2003  Aymeric MOIZARD - <jack@atosc.org>
  
  The filter plugin is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  The filter plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <partysip/partysip.h>
#include <partysip/psp_utils.h>

#include <ppl/ppl_dns.h>

#include "filter.h"

filter_ctx_t *filter_context = NULL;

extern psp_plugin_t PPL_DECLARE_DATA filter_plugin;
extern char filter_name_config[50];

/*
  This plugin can be configured to:
  * 
  * mandate record-routing to stay on the path for the duration of the call.
       ("proxy-mode")

  */

/*
  Configuration sample:.
  
  <filter>

  mode          statefull
  # record-route  on

  # tel url definitions 
  forward    ^tel:07([0-9]+)|sip:0033%1@sip.no-ip.org;user=phone
  forward    ^tel:0[89]([0-9]+)|sip:713@192.168.1.83;user=phone
  forward    ^tel:(1[01])([0-9]+)|sip:%1%2@g1.france.com
  forward    ^tel:12([0-9]+)|sip:%1@g2.france.com
  forward    ^tel:13([0-9]+)|sip:g3.france.com
  forward    ^tel:([0-9]+)|sip:%s@world-operator.com

  </filter>
*/

#define INTERNAL_SCOPE      0x1000
#define EXTERNAL_SCOPE      0x0100
#define REDIRECT_MODE      0x0010
#define R_ROUTE_MODE       0x0001

#define ISSET_INTERNAL_FILTERSCOPE(flag) ((~flag|~INTERNAL_SCOPE)==~INTERNAL_SCOPE)
#define SET_INTERNAL_FILTERSCOPE(flag)   (flag=flag|INTERNAL_SCOPE)

#define ISSET_EXTERNAL_FILTERSCOPE(flag) ((~flag|~EXTERNAL_SCOPE)==~EXTERNAL_SCOPE)
#define SET_EXTERNAL_FILTERSCOPE(flag)   (flag=flag|EXTERNAL_SCOPE)

#define ISSET_REDIRECT_MODE(flag)    ((~flag|~REDIRECT_MODE)==~REDIRECT_MODE)
#define SET_REDIRECT_MODE(flag)      (flag=flag|REDIRECT_MODE)

#define ISSET_R_ROUTE_MODE(flag)     ((~flag|~R_ROUTE_MODE)==~R_ROUTE_MODE)
#define SET_R_ROUTE_MODE(flag)       (flag=flag|R_ROUTE_MODE)

#if !defined(WIN32) || defined(TRE)
static int filter_load_forward_config()
{
  int i;
  config_element_t *elem;
  config_element_t *next_elem;
  char *arg1;
  
  elem = psp_config_get_sub_element("forward", filter_name_config, NULL);
  
  while (elem!=NULL)
    {
      tel_rule_t *tel_rule;
      /*
	forward    ^tel:07([0-9]+)|sip:0033%1@sip.no-ip.org;user=phone
	forward    ^tel:0[89]([0-9]+)|sip:713@192.168.1.83;user=phone
	forward    ^tel:(1[01])([0-9]+)|sip:%1%2@g1.france.com
      */

      arg1 = strchr(elem->value, '|'); /* find beginning of prefix */
      if (arg1==NULL) return -1;
      arg1=arg1+1;

      /* only 100 length is allowed */
      if (arg1-elem->value-1>100) return -1;
      if (strlen(arg1)>100) return -1;

      tel_rule = (tel_rule_t*) osip_malloc(sizeof(tel_rule_t));
      memset(tel_rule, 0, sizeof(tel_rule_t));
      tel_rule->next   = NULL;
      tel_rule->parent = NULL;
      osip_strncpy(tel_rule->prefix, elem->value, arg1-elem->value-1);
      osip_strncpy(tel_rule->dnsresult, arg1, strlen(arg1));
      /* pre-compile the regex */
      i = regcomp(&tel_rule->cprefix, tel_rule->prefix,
		  REG_EXTENDED|REG_ICASE);
      if (i!=0)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
				"filter plugin: Error in regex pattern: %s!\n",
				tel_rule->prefix));
	  osip_free(tel_rule);
	  return -1;
	}

      ADD_ELEMENT(filter_context->tel_rules, tel_rule);

      next_elem = elem;
      elem = psp_config_get_sub_element("forward", filter_name_config, next_elem);
    }
  return 0;
}
#endif

int
filter_ctx_init()
{
  config_element_t *elem;
  int i;

#if defined(WIN32) && !defined(TRE)
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO4,NULL,
			"filter plugin: NOT IMPLEMENTED ON WIN32!\n"));
  return 0;
#else

  filter_context = (filter_ctx_t*)osip_malloc(sizeof(filter_ctx_t));
  if (filter_context==NULL) return -1;
  
  filter_context->tel_rules = NULL;

  filter_context->flag = 0;
  elem = psp_config_get_sub_element("mode", filter_name_config, NULL);
  if (elem==NULL||elem->value==NULL)
    { }
  else if (0==strcmp(elem->value, "redirect"))
    SET_REDIRECT_MODE(filter_context->flag);
  else if (0==strcmp(elem->value, "statefull"))
    { }
  else
    goto lsci_error1; /* error, bad option */

  elem = psp_config_get_sub_element("filter_scope", filter_name_config, NULL);
  if (elem==NULL||elem->value==NULL)
    {
      SET_INTERNAL_FILTERSCOPE(filter_context->flag);
      SET_EXTERNAL_FILTERSCOPE(filter_context->flag);
    }
  else if (0==strcmp(elem->value, "internal"))
    SET_INTERNAL_FILTERSCOPE(filter_context->flag);
  else if (0==strcmp(elem->value, "external"))
    SET_EXTERNAL_FILTERSCOPE(filter_context->flag);
  else if (0==strcmp(elem->value, "both"))
    {
      SET_INTERNAL_FILTERSCOPE(filter_context->flag);
      SET_EXTERNAL_FILTERSCOPE(filter_context->flag);
    }
  else
    goto lsci_error1; /* error, bad option */

  elem = psp_config_get_sub_element("record-route", filter_name_config, NULL);
  if (elem==NULL||elem->value==NULL)
    { }
  else if (0==strcmp(elem->value, "off"))
    { }
  else if (0==strcmp(elem->value, "on"))
    SET_R_ROUTE_MODE(filter_context->flag);
  else
    goto lsci_error1; /* error, bad option */  

  /* load the configuration behavior for INVITE */
  i = filter_load_forward_config();
  if (i!=0) goto lsci_error2;
  
  if (ISSET_R_ROUTE_MODE(filter_context->flag))
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
		  "filter plugin: configured to do record-routing!\n"));
    }

  if (ISSET_REDIRECT_MODE(filter_context->flag))
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
		  "filter plugin: configured in redirect mode!\n"));
    }

  if (ISSET_INTERNAL_FILTERSCOPE(filter_context->flag))
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
		  "filter plugin: configured to process url for local domain!\n"));
    }

  if (ISSET_EXTERNAL_FILTERSCOPE(filter_context->flag))
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
		  "filter plugin: configured to process url for non local domain!\n"));
    }

  return 0;

 lsci_error2:
 lsci_error1:
  osip_free(filter_context);
  filter_context = NULL;
  return -1;
#endif
}

void
filter_ctx_free()
{
  tel_rule_t *tel_rule;
  if (filter_context==NULL) return;

#if !defined(WIN32) || defined(TRE)
  for (tel_rule = filter_context->tel_rules;
       tel_rule!=NULL;
       tel_rule=filter_context->tel_rules)
  {
    REMOVE_ELEMENT(filter_context->tel_rules, tel_rule);
    regfree(&tel_rule->cprefix);
    osip_free(tel_rule);
  }

  osip_free(filter_context);
  filter_context = NULL;
#endif
}

#if !defined(WIN32) || defined(TRE)

static int
filter_match_rule(tel_rule_t *tel_rule, osip_message_t *request,
		    char *match1, char *match2)
{
  osip_uri_t *req_uri = request->req_uri;
  char *url;
  int i;
  regmatch_t pmatch[3];
  if (tel_rule->prefix=='\0') return 0; /* always match */

  if (req_uri==NULL)
    return -1;

  osip_uri_to_str_canonical(req_uri, &url);
  if (url==NULL)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
			    "filter plugin: Bad sip url in SIP request.\n",
			    tel_rule->prefix));
      return -1;
    }

  /* match one sip url */
  i = regexec(&tel_rule->cprefix, url, tel_rule->cprefix.re_nsub+1, pmatch, 0);
  if (i!=0)
    {
      if (i!=REG_NOMATCH)
	{
	  char error_buf[512];
	  regerror(i, &tel_rule->cprefix, error_buf, 512);
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
				"filter plugin: regexec failed(%i) for %s!\n",
				i, error_buf));
	}
      else
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO4,NULL,
				"filter plugin: No match for %s!\n", url));
	}
      osip_free(url);
      return -1;
    }
  
#ifdef FILTER_TEST
    fprintf(stdout, "tel_rule->cprefix.re_nsub+1: %i\n",
	    tel_rule->cprefix.re_nsub+1);
#endif

  for (i=0;pmatch[i].rm_so!=-1 && tel_rule->cprefix.re_nsub+1 != i ;i++)
    {
#ifdef FILTER_TEST
      fprintf(stdout, "result of regexec: %i %i\n",
	      pmatch[i].rm_so, pmatch[i].rm_eo);
      fprintf(stdout, "pmatch[i].rm_so: %s\n", url+pmatch[i].rm_so);
#endif
      /* reject this too long field */
      if (pmatch[i].rm_eo-pmatch[i].rm_so+1>255)
	{
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
				"filter plugin: url component is too long, I choose to reject it!\n"));
	  osip_free(url);
	  return -1;
	}
#ifdef WIN32
      if (i==1)
	_snprintf(match1, pmatch[i].rm_eo-pmatch[i].rm_so, /* -1 with TRE? */
		 "%s", url+pmatch[i].rm_so);
      else if (i==2)
	_snprintf(match2, pmatch[i].rm_eo-pmatch[i].rm_so, /* -1 with TRE? */
		 "%s", url+pmatch[i].rm_so);
#else
      if (i==1)
	snprintf(match1, pmatch[i].rm_eo-pmatch[i].rm_so+1,
		 "%s", url+pmatch[i].rm_so);
      else if (i==2)
	snprintf(match2, pmatch[i].rm_eo-pmatch[i].rm_so+1,
		 "%s", url+pmatch[i].rm_so);
#endif
    }
#ifdef FILTER_TEST
  fprintf(stdout, "match1: %s, match2: %s\n", match1, match2);
#endif
  osip_free(url);
  return 0;
}


static int
filter_build_dnsresult(tel_rule_t *tel_rule, osip_uri_t *req_uri, char *match1,
		       char *match2, char **dest)
{
  char *tmp;
  char *_tmp;
  char *_tmp2;
  *dest = NULL;

  if (req_uri->scheme==NULL)
    /* just in case */
    req_uri->scheme=osip_strdup("sip");
  
  /* find %1 in string */
  _tmp = strstr(tel_rule->dnsresult, "%1");
  /* find %2 in string */
  _tmp2 = strstr(tel_rule->dnsresult, "%2");

  /* this is always enough: (and match1 and match2 can be empty string '\0') */
  tmp = osip_malloc(strlen(tel_rule->dnsresult)
		+strlen(match1)+strlen(match2) + 3 );
  
  if (_tmp!=NULL) /* contains %s */
    {
      /* copy from the beginning to '%1' */
      osip_strncpy(tmp,tel_rule->dnsresult, _tmp-tel_rule->dnsresult);
      /* copy match1 from the current pos end of match1 */
      osip_strncpy(tmp + strlen(tmp), match1, strlen(match1));
      if (_tmp2==NULL) /* simple case */
	/* copy the rest up to the end */
	  osip_strncpy(tmp + strlen(tmp), _tmp+2, strlen(_tmp+2));
      else
	{
	  /* copy the rest up to %2 */
	  osip_strncpy(tmp + strlen(tmp), _tmp+2, _tmp2-_tmp-2);
	  /* copy match2 from the current pos end of match2 */
	  osip_strncpy(tmp + strlen(tmp), match2, strlen(match2));
	  /* copy the rest up to the end */
	  osip_strncpy(tmp + strlen(tmp), _tmp2+2, strlen(_tmp2+2));
	}
    }
  else if (_tmp2!=NULL)
    {
      /* copy from the beginning to '%2' */
      osip_strncpy(tmp,tel_rule->dnsresult, _tmp2-tel_rule->dnsresult);
      /* copy match1 from the current pos end of match2 */
      osip_strncpy(tmp + strlen(tmp), match2, strlen(match2));
      /* copy the rest up to the end */
      osip_strncpy(tmp + strlen(tmp), _tmp2+2, strlen(_tmp2+2));
    }
  else
    sprintf(tmp, tel_rule->dnsresult);

  if (tmp!=NULL)
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO4,NULL,
			    "filter plugin: Here is the resulted value: %s\n",
			    tmp));
    }

  *dest = tmp;
  return 0;
}

#endif

/* HOOK METHODS */

/*
  This method returns:
  -2 if plugin consider this request should be totally discarded!
  -1 on error
  0  nothing has been done
  1  things has been done on psp_req element
*/
int cb_filter_search_location(psp_request_t *psp_req)
{
#if defined(WIN32) && !defined(TRE)
  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO4,NULL,
	   "filter plugin: not implemented on WIN32\n"));
  psp_request_set_state(psp_req, PSP_PROPOSE);
  psp_request_set_uas_status(psp_req, 404);
  psp_request_set_mode(psp_req, PSP_UAS_MODE);
  return 0;
#else
  tel_rule_t *tel_rule;
  location_t *loc;
  osip_route_t *route;
  osip_uri_t *url;
  int i;
  osip_uri_param_t *psp_param;
  osip_message_t *request;
  request = psp_request_get_request(psp_req);

  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
			"filter plugin: filter_context %p!\n", filter_context));


  if (ISSET_INTERNAL_FILTERSCOPE(filter_context->flag))
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
		  "filter plugin: configured to process url for local domain!\n"));
    }

  if (ISSET_EXTERNAL_FILTERSCOPE(filter_context->flag))
    {
      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
		  "filter plugin: configured to process url for non local domain!\n"));
    }

  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
	   "filter plugin: entering cb_filter_search_location\n"));

		/* default OUTPUT */
  if (ISSET_R_ROUTE_MODE(filter_context->flag))
    psp_request_set_property(psp_req, PSP_STAY_ON_PATH);
  else
    psp_request_set_property(psp_req, 0);

  if (!ISSET_REDIRECT_MODE(filter_context->flag))
    /* state full or default mode */
    psp_request_set_mode(psp_req, PSP_SFULL_MODE);
  else
    {
      psp_request_set_uas_status(psp_req, 302);
      psp_request_set_mode(psp_req, PSP_UAS_MODE);
    }

  i=0;
  for (;!osip_list_eol(&request->routes, i);i++)
    {
      osip_message_get_route (request, i, &route);
      if (0 != psp_core_is_responsible_for_this_route(route->url))
	{
	  psp_request_set_mode (psp_req, PSP_SFULL_MODE);
	  psp_request_set_state (psp_req, PSP_MANDATE);
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
				  "filter plugin: mandate statefull handling for route.\n"));
	  return 0;
	}
    }

  psp_request_set_state(psp_req, PSP_MANDATE);

  if (i>1)
    {
      psp_request_set_uas_status(psp_req, 482); /* loop? */
      psp_request_set_mode(psp_req, PSP_UAS_MODE);
      return 0;
    }
  if (i==1)
    {
      osip_message_get_route(request, 0, &route); /* should be the first one */
      /* if this route header contains the "psp" parameter, it means
	 the element does not come from a pre-route-set header (in this
	 last case, we want to execute the plugin for the initial request) */
      /* NON compliant UAs (not returning this parameter) are guilty. */
      osip_uri_uparam_get_byname (route->url, "psp", &psp_param);
      if (psp_param!=NULL)
	{
	  psp_request_set_state(psp_req, PSP_MANDATE);
	  psp_request_set_mode (psp_req, PSP_SFULL_MODE);
	  /* got it, leave this plugin. */
	  return 0;
	}
    }

  if (ISSET_INTERNAL_FILTERSCOPE(filter_context->flag)
      && ISSET_EXTERNAL_FILTERSCOPE(filter_context->flag))
    { /* process request in all cases */ }
  else
    {
      if (0 == psp_core_is_responsible_for_this_domain (request->
							req_uri))
	{
	  if (!ISSET_INTERNAL_FILTERSCOPE(filter_context->flag))
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
				    "filter plugin: uri is for our local domain -> do not process it!\n"));
	      psp_request_set_state(psp_req, PSP_PROPOSE);
	      psp_request_set_uas_status(psp_req, 404);
	      psp_request_set_mode(psp_req, PSP_UAS_MODE);
	      return 0;
	    }
	}
      else
	{
	  if (!ISSET_EXTERNAL_FILTERSCOPE(filter_context->flag))
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
				    "filter plugin: uri is not for our local domain -> do not process it!\n"));
	      psp_request_set_state(psp_req, PSP_PROPOSE);
	      psp_request_set_uas_status(psp_req, 404);
	      psp_request_set_mode(psp_req, PSP_UAS_MODE);
	      return 0;
	    }
	}
    }
    

  for (tel_rule = filter_context->tel_rules;
       tel_rule!=NULL;
       tel_rule=tel_rule->next)
    {
      char match1[256];
      char match2[256];
      /* Add this for TRE */
      memset(match1, 0, sizeof(match1));
      memset(match2, 0, sizeof(match2));
      i = filter_match_rule(tel_rule, request, match1, match2);
      if (i!=-1)
	{
	  char *tmp;
	  filter_build_dnsresult(tel_rule, request->req_uri,
				   match1, match2, &tmp);
	  if (tmp==NULL)
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
				    "filter plugin: failed to build new destination url!\n"));
	      psp_request_set_uas_status(psp_req, 400);
	      psp_request_set_mode(psp_req, PSP_UAS_MODE);
	      psp_request_set_state(psp_req, PSP_MANDATE);
	      return -1;
	    }

	  i = osip_uri_init(&url);
	  i = osip_uri_parse(url, tmp);
	  osip_free(tmp);
	  if (i!=0)
	    {
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
				    "filter plugin: Could not clone request-uri!\n"));
	      psp_request_set_uas_status(psp_req, 400);
	      psp_request_set_mode(psp_req, PSP_UAS_MODE);
	      psp_request_set_state(psp_req, PSP_MANDATE);
	      osip_uri_free(url);
	      return -1;
	    }

	  i = location_init(&loc, url, 3600);
	  if (i!=0)
	    { /* This can only happen in case we don't have enough memory */
	      osip_uri_free(url);
	      OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_BUG,NULL,
				    "filter plugin: Could not create location info!\n"));
	      psp_request_set_uas_status(psp_req, 400);
	      psp_request_set_mode(psp_req, PSP_UAS_MODE);
	      psp_request_set_state(psp_req, PSP_MANDATE);
	      return -1;
	    }
	  ADD_ELEMENT(psp_req->locations, loc);
	  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
				"filter plugin: mandate statefull (or redirect) mode for request.\n"));
	  return 0;
	}
    }

  OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_INFO1,NULL,
	"filter plugin: Didn't do anything with this request?\n"));
  psp_request_set_state(psp_req, PSP_PROPOSE);
  psp_request_set_uas_status(psp_req, 404);
  psp_request_set_mode(psp_req, PSP_UAS_MODE);

  return 0;
#endif
}

