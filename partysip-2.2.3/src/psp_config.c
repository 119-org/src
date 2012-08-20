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


#include <osipparser2/osip_port.h>
#include <psp_config.h>

static config_element_t *elements = NULL;
static config_element_t *sub_elements = NULL;

int
psp_config_load (char *filename)
{
  FILE *file;
  char *s;
  char sub_config_name[31];

  sub_config_name[0] = '\0';

  file = fopen (filename, "r");
  if (file == NULL)
    return -1;

  s = (char *) osip_malloc (1024 * sizeof (char));
  while (NULL != fgets (s, 1024, file))
    {
      int length;

      osip_clrspace (s);
      length = strlen (s);
      if ((strlen (s) <= 1) || (0 == strncmp (s, "#", 1)))
	;			/* skip it */
      else if (s[0] == '<' && (length < 30))
	{
	  if (s[1] != '/')
	    osip_strncpy (sub_config_name, s + 1, length - 2);
	  else
	    sub_config_name[0] = '\0';
	}
      else
	{
	  if (0 != psp_config_set_element (sub_config_name, s))
	    {
	      osip_free (s);
	      fclose (file);
	      return -1;
	    }
	}
    }
  osip_free (s);
  fclose (file);

  return 0;			/* ok */
}

PPL_DECLARE (int) psp_config_add_element (char *name, char *value)
{
  config_element_t *configelt;

  configelt = (config_element_t *) osip_malloc (sizeof (config_element_t));
  configelt->sub_config = NULL;
  configelt->next = NULL;
  configelt->parent = NULL;
  configelt->name = name;
  configelt->value = value;
  ADD_ELEMENT (elements, configelt);
  return 0;
}

PPL_DECLARE (int) psp_config_set_element (char *sub_config, char *string)
{
  config_element_t *configelt;
  char *tmp;

  configelt = (config_element_t *) osip_malloc (sizeof (config_element_t));
  configelt->next = NULL;
  configelt->parent = NULL;

  if (sub_config[0] == '\0')	/* not a sub config */
    {
      tmp = strchr (string, '=');	/* search "=" 61 */
      if (tmp == NULL)		/* I do not allow this */
	{
	  printf ("This is a bad configuration: broken line is:\nerror:%s\n",
		  string);
	  osip_free (configelt);
	  return -1;
	}
      configelt->sub_config = NULL;
      configelt->name = (char *) osip_malloc (tmp - string + 1);
      configelt->value = (char *) osip_malloc (string + strlen (string) - tmp);
      osip_strncpy (configelt->name, string, tmp - string);
      osip_strncpy (configelt->value, tmp + 1,
		string + strlen (string) - tmp - 1);
      osip_clrspace (configelt->name);
      osip_clrspace (configelt->value);
      ADD_ELEMENT (elements, configelt);
    }
  else
    {				/* in a sub config */
      configelt->sub_config = osip_strdup (sub_config);
      for (tmp = string; *tmp != '\0' && (*tmp != ' ' && *tmp != '\t'); tmp++)
	{
	}
      if (*tmp == '\0')
	{
	  printf ("This is a bad configuration: broken line is:\nerror:%s\n",
		  string);
	  osip_free (configelt->sub_config);
	  osip_free (configelt);
	  return -1;
	}
      configelt->name = (char *) osip_malloc (tmp - string + 1);
      configelt->value = osip_strdup (tmp + 1);
      osip_strncpy (configelt->name, string, tmp - string);
      osip_clrspace (configelt->name);
      osip_clrspace (configelt->value);
      ADD_ELEMENT (sub_elements, configelt);
    }

  return 0;
}

PPL_DECLARE (char *) psp_config_get_element (char *name)
{
  config_element_t *tmp;

  for (tmp = elements; tmp != NULL; tmp = tmp->next)
    {
      if (tmp->sub_config == NULL && 0 == strcmp (tmp->name, name))
	return tmp->value;
    }
  return NULL;
}

PPL_DECLARE (config_element_t *)
psp_config_get_sub_element (char *name, char *sub_config,
			    config_element_t * start)
{
  config_element_t *tmp;

  if (start == NULL)
    start = sub_elements;
  else
    start = start->next;
  for (tmp = start; tmp != NULL; tmp = tmp->next)
    {
      if (tmp->sub_config != NULL
	  && 0 == strcmp (tmp->name, name)
	  && 0 == strcmp (tmp->sub_config, sub_config))
	return tmp;
    }
  return NULL;
}

void
psp_config_unload ()
{
  config_element_t *tmp;

  for (tmp = elements; tmp != NULL; tmp = elements)
    {
      REMOVE_ELEMENT (elements, tmp);
      osip_free (tmp->sub_config);
      osip_free (tmp->name);
      osip_free (tmp->value);
      osip_free (tmp);
    }
  elements = NULL;
  for (tmp = sub_elements; tmp != NULL; tmp = sub_elements)
    {
      REMOVE_ELEMENT (sub_elements, tmp);
      osip_free (tmp->sub_config);
      osip_free (tmp->name);
      osip_free (tmp->value);
      osip_free (tmp);
    }
  sub_elements = NULL;
  return;
}
