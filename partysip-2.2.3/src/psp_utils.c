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

#include <partysip/psp_config.h>
#include <partysip/psp_utils.h>
#include <ppl/ppl_uinfo.h>
#include <partysip.h>
#include "proxyfsm.h"

#if defined(WIN32) || defined(NEED_GETTIMEOFDAY)

#include <time.h>
#include <sys/timeb.h>

int gettimeofday(struct timeval *tp, void *tz)
{
	struct _timeb timebuffer;   

	_ftime( &timebuffer );
	tp->tv_sec  = timebuffer.time;
	tp->tv_usec = timebuffer.millitm * 1000;
	return 0;
}

#endif

PPL_DECLARE (int)
psp_util_get_and_set_next_token (char **dest, char *buf, char **next)
{
  char *end;
  char *start;

  *next = NULL;

  /* find first non space and tab element */
  start = buf;
  while (((*start == ' ') || (*start == '\t')) && (*start != '\0')
	 && (*start != '\r') && (*start != '\n'))
    start++;
  end = start;
  while ((*end != '\0') && (*end != '\r') && (*end != '\n') && (*end != ' ')
	 && (*end != '\t'))
    end++;
  if ((*end == '\r') || (*end == '\n'))
    /* we should continue normally only if this is the separator asked! */
    return -1;
  if (end == start)
    return -1;			/* empty value (or several space!) */

  *dest = osip_malloc (end - (start) + 1);
  osip_strncpy (*dest, start, end - start);

  *next = end + 1;		/* return the position right after the separator */
  return 0;
}

static int
user_info_add (char *buf)
{
  ppl_uinfo_t *uinfo;
  osip_uri_t *user_url;
  osip_contact_t *contact;
  char *next;
  char *result;
  char *login;
  char *passwd;
  int i;


  i = osip_uri_init (&user_url);
  if (i != 0)
    return -1;

  i = psp_util_get_and_set_next_token (&result, buf, &next);
  if (i != 0)
    goto uia_error1;
  osip_clrspace (result);

  i = osip_uri_parse (user_url, result);
  osip_free (result);
  if (i != 0)
    goto uia_error2;

  buf = next;
  i = psp_util_get_and_set_next_token (&login, buf, &next);
  if (i != 0)
    goto uia_error3;
  osip_clrspace (login);

  buf = next;
  i = psp_util_get_and_set_next_token (&passwd, buf, &next);
  if (i != 0)
    goto uia_error4;
  osip_clrspace (passwd);

  /* these are registration with no password */
  if (0 == strcmp (login, "none"))
    {
      osip_free (login);
      osip_free (passwd);
      login = NULL;
      passwd = NULL;
    }

  uinfo = ppl_uinfo_create (user_url, login, passwd);
  if (uinfo == NULL)
    goto uia_error5;
  osip_uri_free (user_url);

  if (next == NULL || strlen (next) < 5)
    return 0;

  /* add one static routes if it exist! */
  i = osip_contact_init (&contact);
  if (i != 0)
    goto uia_error6;

  osip_clrspace (next);
  i = osip_contact_parse (contact, next);
  if (i != 0)
    {
      osip_contact_free (contact);
      goto uia_error6;
    }

  i = ppl_uinfo_add_binding_with_path (uinfo, contact, NULL, NULL);
  osip_contact_free (contact);
  if (i != 0)
    goto uia_error6;

  return 0;

uia_error6:
  return 0;

uia_error5:
  osip_free (passwd);
uia_error4:
  osip_free (login);
uia_error3:
uia_error2:
uia_error1:
  osip_uri_free (user_url);
  return -1;
}

int
psp_utils_load_users ()
{
  int number_of_user_entry = 0;
  int number_of_auth_rules = 0;
  config_element_t *previous = NULL;
  config_element_t *elem;
  int i;

  /*
     elem = psp_config_get_sub_element("auth", "userinfo", previous);
     while (elem!=NULL)
     {
     i = auth_entry_init(elem->value);
     if (i!=0)
     {
     OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,
     "registrar plugin: Bad auth line in config file!\n%s\n", elem->value));
     return -1;
     }
     number_of_auth_rules++;
     previous = elem;
     elem = psp_config_get_sub_element("auth", "userinfo", previous);
     }
   */
  elem = psp_config_get_sub_element ("user", "userinfo", NULL);
  while (elem != NULL)
    {
      i = user_info_add (elem->value);
      if (i != 0)
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				  "registrar plugin: Bad user line in config file!\n%s\n"));
	  return -1;
	}
      number_of_user_entry++;
      previous = elem;
      elem = psp_config_get_sub_element ("user", "userinfo", previous);
    }
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO3, NULL,
			  "registrar plugin: Number of auth rules: %i. Number of user entries: %i\n",
			  number_of_auth_rules, number_of_user_entry));


  return 0;
}
