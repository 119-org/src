/*
  The partysip program is a modular SIP proxy server (SIP -rfc3261-)
  Copyright (C) 2002  WellX Telecom   - <partysip@wellx.com>
  Copyright (C) 2002  Aymeric MOIZARD - <jack@atosc.org>
  
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
#include <partysip/psp_config.h>
#include <psp_utils.h>

#include <ppl/ppl_init.h>
#include <ppl/ppl_getopt.h>
#include <ppl/ppl_uinfo.h>
extern ppl_uinfo_t *user_infos;

#include <ppl/ppl_dbm.h>

#define PSP_SERVER_BASEARGS "u:f:d:vVHh?X"

#define PSP_SERVER_BASEVENDOR "WellX Telecom"
#define PSP_SERVER_BASEPRODUCT "Partysip"

#ifdef WIN32
#define PSP_SERVER_BASEREVISION PSP_VERSION
#define PSP_SERVER_BASEREVISION PSP_VERSION

#else
#define PSP_SERVER_BASEREVISION VERSION
#define PSP_SERVER_BASEREVISION VERSION
#endif

#define PSP_SERVER_BASEVERSION PSP_SERVER_BASEPRODUCT "/" PSP_SERVER_BASEREVISION
#define PSP_SERVER_VERSION  PSP_SERVER_BASEVERSION

#define PSP_SERVER_INSTALL_DIR PSP_SERVER_PREFIX

#if defined(__DATE__) && defined(__TIME__)
static const char server_built[] = __DATE__ " " __TIME__;
#else
static const char server_built[] = "unknown";
#endif

#define PSP_SERVER_WHO_AM_I PSP_SERVER_BASEVENDOR "/" PSP_SERVER_VERSION


#if defined(HAVE_GDBM_H) || defined(HAVE_GDBM_NDBM_H) || defined(HAVE_DB_H) || defined(HAVE_DB1_DB_H) || defined(HAVE_DB2_DB_H) || defined(HAVE_DB3_DB_H) || defined(HAVE_NDBM_H) || defined(HAVE_DBM_H)

/* use html */
int output_in_html = 0;

static void
usage (int code)
{
  printf ("\n\
usage:\n\
\n\
   psp_users [-vV] [-f config] [-u username]\n\
\n\
   [-u]                  print information on a specific user\n\
   [-h]                  print this help\n\
   [-f config]           configuration file for partysip.\n\
   [-d level]            be verbose. 0 -> no output. 6 -> all output .\n\
   [-v]                  print partysip info\n\
   [-V]                  print full partysip info with compile flags\n\n");
  exit (code);
}



static int
print_uinfo(ppl_uinfo_t *uinfo)
{
  binding_t *b;
  char *tmp;

  osip_uri_to_str (uinfo->aor->url, &tmp);
  fprintf(stdout, "aor  : %s\n", tmp);
  osip_free(tmp);
  
  for (b=uinfo->bindings; b!=NULL; b=b->next)
    {
      osip_contact_to_str(uinfo->bindings->contact, &tmp);
      fprintf(stdout,"\tlocation: %s\n",tmp );
      osip_free(tmp);
    }
  return 0;
}

static void
translate2html(char *tmp, char *buf)
{
  for (;*tmp!='\0';tmp++)
    {
      if (*tmp=='<')
	{
	  sprintf(buf, "&lt;");
	  buf = buf + 3;
	}
      else
	*buf = *tmp;
      buf++;
    }
  *buf='\0';
}

static int
print_my_uinfo_html(ppl_uinfo_t *uinfo)
{
  char *tmp;
  char buf[512];

  osip_uri_to_str (uinfo->aor->url, &tmp);
  translate2html(tmp, buf);

  fprintf(stdout, "<TR>\n<TD WIDTH=\"53\" HEIGHT=\"11\"><IMG SRC=\"img/transitionG.png\" HEIGHT=11 WIDTH=53></TD>\n");
  fprintf(stdout, "<TD WIDTH=\"230\"><IMG SRC=\"img/transitionD.png\" HEIGHT=11 WIDTH=500></TD>\n");
  fprintf(stdout, "</TR>\n");

  fprintf(stdout, "<TR>\n<TD WIDTH=\"53\" HEIGHT=\"40\"><IMG SRC=\"img/sip-empty.png\" HEIGHT=40 WIDTH=53></TD>\n");

  fprintf(stdout, "<TD VALIGN=BOTTOM><div align=right>%s",buf);

  if (uinfo->status==1)
    fprintf(stdout,"<IMG SRC=\"img/sip-online2.png\" HEIGHT=40 WIDTH=53>");
  else if (uinfo->status==2)
    fprintf(stdout,"<IMG SRC=\"img/sip-away2.png\" HEIGHT=40 WIDTH=53>");
  else if (uinfo->status==3)
    fprintf(stdout,"<IMG SRC=\"img/sip-busy2.png\" HEIGHT=40 WIDTH=53>");
  else if (uinfo->status==4)
    fprintf(stdout,"<IMG SRC=\"img/sip-bifm2.png\" HEIGHT=40 WIDTH=53>");
  else if (uinfo->status==0)
    fprintf(stdout,"<IMG SRC=\"img/sip-empty2.png\" HEIGHT=40 WIDTH=53>");

  osip_free(tmp);
  fprintf(stdout, "</div>\n</TD></TR>\n");

  fprintf(stdout, "<TR>\n<TD WIDTH=\"53\" HEIGHT=\"11\"><IMG SRC=\"img/transitionG.png\" HEIGHT=11 WIDTH=53></TD>\n");
  fprintf(stdout, "<TD WIDTH=\"230\"><IMG SRC=\"img/transitionD.png\" HEIGHT=11 WIDTH=500></TD>\n");
  fprintf(stdout, "</TR>\n");

  return 0;
}

static int
print_uinfo_html(ppl_uinfo_t *uinfo)
{
  char *tmp;
  char buf[512];

  osip_uri_to_str (uinfo->aor->url, &tmp);
  translate2html(tmp, buf);

  if (uinfo->status==1)
    fprintf(stdout, "<TR>\n<TD WIDTH=\"53\" HEIGHT=\"40\"><IMG SRC=\"img/sip-online.png\" HEIGHT=40 WIDTH=53></TD>\n");
  else if (uinfo->status==2)
    fprintf(stdout, "<TR>\n<TD WIDTH=\"53\" HEIGHT=\"40\"><IMG SRC=\"img/sip-away.png\" HEIGHT=40 WIDTH=53></TD>\n");
  else if (uinfo->status==3)
    fprintf(stdout, "<TR>\n<TD WIDTH=\"53\" HEIGHT=\"40\"><IMG SRC=\"img/sip-busy.png\" HEIGHT=40 WIDTH=53></TD>\n");
  else if (uinfo->status==4)
    fprintf(stdout, "<TR>\n<TD WIDTH=\"53\" HEIGHT=\"40\"><IMG SRC=\"img/sip-bifm.png\" HEIGHT=40 WIDTH=53></TD>\n");
  else if (uinfo->status==0)
    fprintf(stdout, "<TR>\n<TD WIDTH=\"53\" HEIGHT=\"40\"><IMG SRC=\"img/sip-empty.png\" HEIGHT=40 WIDTH=53></TD>\n");
  
  fprintf(stdout, "<TD VALIGN=BOTTOM BACKGROUND=\"img/sip-empty2.png\">%s",buf);
  osip_free(tmp);
  if (uinfo->status==1)
    fprintf(stdout, " (Online)");
  else if (uinfo->status==2)
    fprintf(stdout, " (Away)");
  else if (uinfo->status==3)
    fprintf(stdout, " (Busy)");
  else if (uinfo->status==4)
    fprintf(stdout, " (Back In a Few Minutes)");
  else if (uinfo->status==0)
    fprintf(stdout, " (Unkown)");

  /*
  for (b=uinfo->bindings; b!=NULL; b=b->next)
    {
      osip_contact_to_str(uinfo->bindings->contact, &tmp);
      translate2html(tmp, buf);
      fprintf(stdout," (%s)", buf);
      osip_free(tmp);
    }
  */
  fprintf(stdout, "</td></tr>\n");

  return 0;
}

static int ppl_dbm_print(const char *name)
{
  ppl_uinfo_t *uinfo = NULL;

  if (name!=NULL)
    {
      for (uinfo = user_infos; uinfo != NULL; uinfo = uinfo->next)
	{
	  if (0 == strcmp (name, uinfo->aor->url->username))
	    {
	      /* uinfo found */
	      break;
	    }
	}
      if (uinfo==NULL)
	{
	  fprintf(stderr, "psp_users: user \"%s\" not found\n", name);
	  return -1;
	}
    }

  if (output_in_html==1)
    {
      fprintf(stdout, "\
<!doctype html public \"-//w3c//dtd html 4.0 transitional//en\">\n\
<HTML>\n\
<HEAD>\n\
   <META http-equiv=\"Content-Type\" content=\"text/html\">\n\
</HEAD>\n\
<BODY text=\"#CCCCCC\" bgcolor=\"#FFFFCC\" link=\"#000066\" vlink=\"#FFFF99\" alink=\"#FFFF99\">\n\
<table BORDER=0 CELLSPACING=0 CELLPADDING=0 COLS=2 BGCOLOR=\"#7F7F7F\">\n\
<TR>\n\
<TD WIDTH=\"53\" HEIGHT=\"40\"><img SRC=\"img/angleHG.png\" height=59 width=53></TD>\n\
<TD WIDTH=\"230\"><img SRC=\"img/angleHD.png\" height=59 WIDTH=500></TD>\n\
</TR>\n\n");

    }

  /* print my local user info */
  if (uinfo!=NULL)
    print_my_uinfo_html(uinfo);

  /* print my friends' user info */
  for (uinfo = user_infos; uinfo != NULL; uinfo = uinfo->next)
    {
      if (name!=NULL
	  &&0 == strcmp (name, uinfo->aor->url->username))
	{
	  /*
	    if (output_in_html==0)
	    print_uinfo(uinfo);
	    else
	    print_uinfo_html(uinfo);
	  */
	}
      else
	{
	  if (output_in_html==0)
	    print_uinfo(uinfo);
	  else
	    print_uinfo_html(uinfo);
	}
    }

  if (output_in_html==1)
    {
      fprintf(stdout, "<TR>\n<TD WIDTH=\"53\" HEIGHT=\"40\"><IMG SRC=\"img/sip-empty.png\" HEIGHT=40 WIDTH=53></TD>\n\
<TD VALIGN=BOTTOM><div align=right><IMG SRC=\"img/sip-empty2.png\" HEIGHT=40 WIDTH=500>\
</div>\n</TD></TR>\n");
      
      fprintf(stdout, "<TR>\n\
<TD WIDTH=\"53\" HEIGHT=\"59\"><img SRC=\"img/angleBG.png\" HEIGHT=59 WIDTH=53></TD>\n\
<TD WIDTH=\"230\"><img SRC=\"img/angleBD.png\" HEIGHT=59 WIDTH=500></TD>\n\
</TR>\n\
</TABLE>\n\
\n\
</BODY>\n\
</HTML>\n");
    }

  return 0;
}

int
main (int argc, const char *const argv[])
{
  char c;
  int i;
  ppl_getopt_t *opt;
  ppl_status_t rv;
  const char *cf_config_file = CONFIG_DIR "/partysip.conf";
  int cf_debug_level = 0;
  const char *optarg;

  char *recovery_file;
  const char *username = NULL; /* ask for a specific user */

  /* ppl_initialize(); */
  ppl_init_open ( );

  if (argc > 1 && strlen (argv[1]) == 1 && 0 == strncmp (argv[1], "-", 2))
    usage (0);
  if (argc > 1 && strlen (argv[1]) >= 2 && 0 == strncmp (argv[1], "--", 2))
    usage (0);


  ppl_getopt_init (&opt, argc, argv);

  while ((rv =
	  ppl_getopt (opt, PSP_SERVER_BASEARGS, &c, &optarg)) == PPL_SUCCESS)
    {
      switch (c)
	{
	case 'u':
	  username = optarg;
	  break;
	case 'f':
	  cf_config_file = optarg;
	  break;
	case 'd':
	  /* to be replaced by strtol */
	  cf_debug_level = atoi (optarg);
	  break;
	case 'H':
	  output_in_html = 1;
	  break;
	case 'v':
	case 'V':
	  printf ("Server version:     %s\n", PSP_SERVER_BASEVERSION);
	  printf ("Server built:       %s\n", server_built);
	  exit (0);
	case 'h':
	case '?':
	  printf ("Server version:     %s\n", PSP_SERVER_BASEVERSION);
	  printf ("Server version:     %s\n", PSP_SERVER_BASEVERSION);
	  printf ("Server built:       %s\n", server_built);
	  usage (0);
	default:
	  /* bad cmdline option?  then we die */
	  usage (1);
	}
    }

  if (rv != PPL_EOF)
    {
      usage (1);
    }

  osip_free ((void *) (opt->argv));
  osip_free ((void *) opt);

  /*********************************/
  /*   Load Config File            */
  i = psp_config_load ((char *) cf_config_file);
  if (i != 0)
    {
      fprintf (stderr, "psp_users: cannot open config file (%s)\n", cf_config_file);
      usage (1);
    }

  {
    ppl_dbm_t *_dbm;
    recovery_file  = psp_config_get_element ("recovery_file");
    if (recovery_file==NULL)
      {
	fprintf (stderr, "psp_users: no recovery_file in configuration file!\n");
	exit(0);
      }

    i = ppl_dbm_open(&_dbm, recovery_file, O_RDONLY);
    if (i!=0)
      {
	fprintf (stderr, "psp_users: can't open recovery file!\n");
	exit(0);
      }

    
    if (username==NULL) /* all users */
      ppl_dbm_print(NULL);
    else
      ppl_dbm_print(username);

    ppl_dbm_close(_dbm);
    osip_free(_dbm);
  }

  psp_config_unload ();
  ppl_init_close ();
  exit(1);
  return 0;
}

#else

int
main (int argc, const char *const argv[])
{
  fprintf (stderr, "psp_users: partysip is compiled without DBM support. Please, recompile.\n");
  exit(0);
}

#endif

