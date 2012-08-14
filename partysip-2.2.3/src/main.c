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

#include <partysip/psp_config.h>
#include <psp_utils.h>

#include <ppl/ppl_init.h>
#include <ppl/ppl_getopt.h>
#include <ppl/ppl_uinfo.h>
#include <ppl/ppl_md5.h>

#ifdef HAVE_SETRLIMIT
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

void partysip_exit (int process_exit_value);

#define PSP_SERVER_BASEARGS "f:l:d:vVilph?X"

#define PSP_SERVER_BASEVENDOR "WellX Telecom"
#define PSP_SERVER_BASEPRODUCT "Partysip"

#ifdef WIN32
#define PSP_SERVER_BASEREVISION PSP_VERSION
#define PSP_SERVER_BASEREVISION PSP_VERSION

#if defined(WIN32) && defined(HAVE_NT_SERVICE_MANAGER)
extern HANDLE started_event, stopped_event;
void ReportPartysipShutdownComplete ();
void CommenceStartupProcessing (LPCTSTR lpszServiceName,
				void (*stopper) (int));

#endif

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

#define PSP_PLUGIN_API "O.1"

/* verify the syntax of the server header in SIP ... */
#define PSP_SERVER_HEADER   "WellX/partysip-" PSP_SERVER_BASEREVISION

FILE *log_file = NULL;


/* Developper can grab the compile flag here... */
static void
show_compile_set_tings ()
{
  printf ("Server header:      %s\n", PSP_SERVER_HEADER);
  printf ("Server version:     %s\n", PSP_SERVER_BASEVERSION);
  printf ("Global Config File  %s/%s\n", CONFIG_DIR, PARTYSIP_CONF);
  printf ("Server built:       %s\n", server_built);
  printf ("Server's interface: %s\n", PSP_PLUGIN_API);
  printf ("Architecture:       %ld-bit\n", 8 * (long) sizeof (void *));
  printf ("Server compiled with....\n");
#ifdef STDC_HEADERS
  printf (" -DSTDC_HEADERS\n");
#endif
#ifdef HAVE_ASSERT_H
  printf (" -DHAVE_ASSERT_H\n");
#endif
#ifdef HAVE_CTYPE_H
  printf (" -DHAVE_CTYPE_H\n");
#endif
#ifdef HAVE_DL_H
  printf (" -DHAVE_DL_H\n");
#endif
#if HAVE_DLFCN_H
  printf (" -DHAVE_DLFCN_H\n");
#endif
#if HAVE_FCNTL_H
  printf (" -DHAVE_FCNTL_H\n");
#endif
#if HAVE_MALLOC_H
  printf (" -DHAVE_MALLOC_H\n");
#endif
#ifdef HAVE_PTH_PTHREAD_H
  printf (" -DHAVE_PTH_PTHREAD_H\n");
#endif
#ifdef HAVE_PTHREAD_H
  printf (" -DHAVE_PTHREAD_H\n");
#endif
#if HAVE_SEMAPHORE_H
  printf (" -DHAVE_SEMAPHORE_H\n");
#endif
#if HAVE_SIGNAL_H
  printf (" -DHAVE_SIGNAL_H\n");
#endif
#if HAVE_STDARG_H
  printf (" -DHAVE_STDARG_H\n");
#endif
#if HAVE_STDDEF_H
  printf (" -DHAVE_STDDEF_H\n");
#endif
#if HAVE_STDIO_H
  printf (" -DHAVE_STDIO_H\n");
#endif
#if HAVE_STDLIB_H
  printf (" -DHAVE_STDLIB_H\n");
#endif
#if HAVE_STRING_H
  printf (" -DHAVE_STRING_H\n");
#endif
#ifdef HAVE_STRINGS_H
  printf (" -DHAVE_STRINGS_H\n");
#endif
#if HAVE_SYS_SEM_H
  printf (" -DHAVE_SYS_SEM_H\n");
#endif
#ifdef HAVE_SYS_SIGNAL_H
  printf (" -DHAVE_SYS_SIGNAL_H\n");
#endif
#ifdef HAVE_SYS_SOCKET_H
  printf (" -DHAVE_SYS_SOCKET_H\n");
#endif
#ifdef HAVE_SYS_TIME_H
  printf (" -DHAVE_SYS_TIME_H\n");
#endif
#if HAVE_SYS_TYPES_H
  printf (" -DHAVE_SYS_TYPES_H\n");
#endif
#ifdef HAVE_TIME_H
  printf (" -DHAVE_TIME_H\n");
#endif
#ifdef HAVE_UNISTD_H
  printf (" -DHAVE_UNISTD_H\n");
#endif
#ifdef HAVE_VARARGS_H
  printf (" -DHAVE_VARARGS_H\n");
#endif
  printf (" -DPSP_SERVER_INSTALL_DIR=%s\n", PSP_SERVER_INSTALL_DIR);
}

static void
usage (int code)
{
  printf ("\n\
usage:\n\
\n\
   partysip [-vV] [-f config] [-d level -l logfile]\n\
\n\
   [-h]                  print this help\n\
   [-i]                  interactive mode\n\
   [-f config]           configuration file for partysip.\n\
   [-d level]            be verbose. 0 -> no output. 6 -> all output .\n\
   [-l logfile]          specify the log file.\n\
   [-v]                  print partysip info\n\
   [-V]                  print full partysip info with compile flags\n\n");
  exit (code);
}

void
partysip_exit (int process_exit_value)
{

  psp_core_free ();
  psp_config_unload ();

  ppl_init_close ();

#if defined(WIN32) && defined(HAVE_NT_SERVICE_MANAGER)
  ReportPartysipShutdownComplete ();
  osip_usleep (1000000); /* is it still needed? */
#endif
  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			  "program has terminated.\n"));

  if (log_file != NULL && log_file != stdout)
    fclose (log_file);


#if defined(WIN32) && defined(HAVE_NT_SERVICE_MANAGER)
  return;
#else
  /* in case of services, the method MUST return */
  exit (process_exit_value);
#endif
}

#if 0
static void
partysip_level (char *tmp)
{
  if (0 == strncmp (tmp, "0", 1))
    {
      TRACE_ENABLE_LEVEL (0);
    }
  if (0 == strncmp (tmp, "1", 1))
    {
      TRACE_ENABLE_LEVEL (1);
    }
  if (0 == strncmp (tmp, "2", 1))
    {
      TRACE_ENABLE_LEVEL (2);
    }
  if (0 == strncmp (tmp, "3", 1))
    {
      TRACE_ENABLE_LEVEL (3);
    }
  if (0 == strncmp (tmp, "4", 1))
    {
      TRACE_ENABLE_LEVEL (4);
    }
  if (0 == strncmp (tmp, "5", 1))
    {
      TRACE_ENABLE_LEVEL (5);
    }
  if (0 == strncmp (tmp, "d", 1))
    {
      TRACE_DISABLE_LEVEL (0);
      TRACE_DISABLE_LEVEL (1);
      TRACE_DISABLE_LEVEL (2);
      TRACE_DISABLE_LEVEL (3);
      TRACE_DISABLE_LEVEL (4);
      TRACE_DISABLE_LEVEL (5);
    }
}
#endif

#ifdef WIN32
char *
simple_readline (int descr)
{
  int i = 0;
  char *tmp = (char *) osip_malloc (201);

  fgets (tmp, 200, stdin);
  osip_clrspace (tmp);
  return tmp;
}
#else
static char *
simple_readline (int descr)
{
  int ret;
  fd_set fset;

  FD_ZERO (&fset);
  FD_SET (descr, &fset);
  ret = select (descr + 1, &fset, NULL, NULL, NULL);

  if (FD_ISSET (descr, &fset))
    {
      char *tmp;
      int i;

      tmp = (char *) osip_malloc (201);
      i = read (descr, tmp, 200);
      tmp[i] = '\0';
      if (i > 0)
	osip_clrspace (tmp);
      return tmp;
    }
  return NULL;
}
#endif


static void
main_run ()
{
  char *tmp;

  while (1)
    {
      printf ("(partysip)");
      fflush (stdout);
      tmp = simple_readline (1);
      if (tmp != NULL)
	{
	  if (strlen (tmp) >= 6 && 0 == strncmp (tmp, "plugin", 6))
	    ;
	  else if (strlen (tmp) >= 4 && 0 == strncmp (tmp, "help", 4))
	    ;
#ifdef ENABLE_MPATROL
	  else if (strlen (tmp) == 1 && 0 == strncmp (tmp, "s", 1))
	    {
	      static int on=0;
	      if (on==0)
		{
		  __mp_clearleaktable();
		  __mp_startleaktable();
		  on = 1;
		}
	      else
		{
		  __mp_stopleaktable();
		  __mp_leaktable(0, MP_LT_UNFREED, 0);
		  on = 0;
		}
	    }
#endif
	  else if (strlen (tmp) == 1 && 0 == strncmp (tmp, "q", 1))
	    {
	      osip_free (tmp);
	      partysip_exit (0);
	    }
	  else if (strlen (tmp) == 4 && 0 == strncmp (tmp, "quit", 4))
	    {
	      osip_free (tmp);
	      partysip_exit (0);
	    }
	  else
	    printf ("error: %s: command not found!\n", tmp);
	  osip_free (tmp);
	}
      else
	partysip_exit (1);
    }

}

static int
main_load_plugins ()
{
  int i;
  psp_plugin_t *psp_plugin;
  char *plugins;
  char *plugins_config;
  char *next;
  char *next_config;
  char *result;
  char *result_config;
  /*
     The config file MUST contain the list of plugins as in:
     plugins = udp syntax filter auth rgstrar ls_localdb ls_sfull
   */
  plugins = psp_config_get_element ("plugins");
  if (plugins == NULL)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "Could not read 'plugins' line in config file.\n"));
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "You've got to add this in config file:\n"));
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "plugins = udp syntax filter auth rgstrar ls_localdb ls_sfull\n"));
      return -1;
    }

  plugins_config = psp_config_get_element ("plugins_config");
  if (plugins_config == NULL)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO3, NULL,
			      "Could not read optionnal 'plugins_config' line in config file.\n"));
      result_config = NULL;
    }
    

  i = psp_util_get_and_set_next_token (&result, plugins, &next);
  if (i != 0)
    {
      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
			      "failed to load plugin\n"));
      return -1;
    }
  if (plugins_config!=NULL)
    {
      i = psp_util_get_and_set_next_token (&result_config, plugins_config, &next_config);
      if (i != 0)
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				  "failed to read plugins_config\n"));
	  return -1;
	}
    }
  for (; result != NULL;)
    {
      if (result_config!=NULL)
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
				  "loading plugin: %s (%s)\n", result, result_config));
	}
      else
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
				  "loading plugin: %s\n", result));
	}
      i = psp_plugin_load (&psp_plugin, result, result_config);
      if (i != 0)
	{
	  OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				  "failed to load plugin %s\n", result));
	  return -1;
	}
      if (plugins_config!=NULL)
	{
	  osip_free (result_config);
	  plugins_config = next_config;
	}
      osip_free (result);
      plugins = next;
      i = psp_util_get_and_set_next_token (&result, plugins, &next);
      if (i != 0)
	break;
      if (plugins_config!=NULL)
	{
	  i = psp_util_get_and_set_next_token (&result_config, plugins_config, &next_config);
	  if (i != 0)
	    {
	      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL,
				      "failed to read plugins_config line\n"));
	      return -1;
	    }
	}
    }

  return 0;
}

#if defined(WIN32) && defined(HAVE_NT_SERVICE_MANAGER)

int WINAPI
ServiceMain (DWORD argc, LPTSTR * argv)
#else

int
main (int argc, const char *const argv[])
#endif
{
  char c;
  int i;
  ppl_getopt_t *opt;
  ppl_status_t rv;
  const char *cf_config_file = CONFIG_DIR "/" PARTYSIP_CONF;
  int interactive_mode = 0;
  int cf_debug_level = 0;
  const char *cf_log_file = NULL;
  const char *optarg;

#if defined(WIN32) && defined(HAVE_NT_SERVICE_MANAGER)
  interactive_mode = 2;
  CommenceStartupProcessing ("Partysip Proxy Server", partysip_exit);
  cf_debug_level = 6;
#endif

#ifdef HAVE_SETRLIMIT
  {
    struct rlimit rlimit;
    rlimit.rlim_cur = 2097151;
    rlimit.rlim_max = 2097151;
    if (setrlimit (RLIMIT_CORE,&rlimit) == -1)
      {
	perror("Could not reset new core size (2097151) with setrlimit");
	exit(1);
      }
  }
#endif

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
	case 'f':
	  cf_config_file = optarg;
	  break;
	case 'd':
	  /* to be replaced by strtol */
	  cf_debug_level = atoi (optarg);
	  break;
	case 'l':
	  cf_log_file = optarg;
	  break;
	case 'v':
	  printf ("Server version:     %s\n", PSP_SERVER_BASEVERSION);
	  printf ("Server built:       %s\n", server_built);
	  partysip_exit (0);
	case 'V':
	  show_compile_set_tings ();
	  partysip_exit (0);
	case 'i':
	  interactive_mode = 1;
	  break;
	case 'p':
	  printf ("Show compiled in modules -NOT IMPLEMENTED-\n");
	  /*      psp_core_show_compiled_in_plugins(); */
	  partysip_exit (0);
	case 'h':
	case '?':
	  printf ("\nServer version:     %s\n", PSP_SERVER_BASEVERSION);
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


  /*********************************/
  /*   Load Config File            */

  i = psp_config_load ((char *) cf_config_file);
  if (i != 0)
    {
      printf ("Config name:        %s\n", cf_config_file);
      perror ("ERROR: Could not open config file");
      usage (1);
    }

#if defined(__linux) || defined (WIN32) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__APPLE_CC__)
  psp_core_load_all_ipv4();
  psp_core_load_all_ipv6();
#endif

  /* check some mandatory element in the config file */
  {
    char *serverip = psp_config_get_element ("serverip");
    char *servername = psp_config_get_element ("servername");
    char *serverrealm = psp_config_get_element ("serverrealm");
    /*    char *magicstring = psp_config_get_element ("magicstring"); */

    if (serverip == NULL)
      {
	printf ("Bad configuration: \"serverip\" is mandatory\n");
	partysip_exit (1);
      }
    if (servername == NULL)
      {
	printf ("Bad configuration: \"servername\" is mandatory\n");
	partysip_exit (1);
      }
    /*    if (magicstring == NULL || strlen (magicstring) < 10) { */
    {
      char *magicstring;
      MD5_CTX Md5Ctx;
      HASH HA1;
      char *tmp;
      magicstring = (char *) osip_malloc (36);
      
      ppl_MD5Init (&Md5Ctx);
      ppl_MD5Update (&Md5Ctx, (unsigned char *) serverip,
		     strlen (serverip));
      ppl_MD5Update (&Md5Ctx, (unsigned char *) servername,
		     strlen (servername));
      tmp = psp_config_get_element ("serverport_udp");
      if (tmp==NULL)
	ppl_MD5Update (&Md5Ctx, (unsigned char *) "5060",
		       strlen ("5060"));
      else
	ppl_MD5Update (&Md5Ctx, (unsigned char *) tmp,
		       strlen (tmp));
      
      tmp = psp_config_get_element ("magicstring");
      if (tmp==NULL)
	{
	  /* add a random element */
	  char *buf = (char *) osip_malloc (33);
	  unsigned int number = osip_build_random_number ();
	  sprintf (buf, "%u", number);
	  ppl_MD5Update (&Md5Ctx, (unsigned char *) buf,
			 strlen (buf));
	  osip_free(buf);
	}
      else
	ppl_MD5Update (&Md5Ctx, (unsigned char *) tmp,
		       strlen (tmp));
      
      ppl_MD5Final ((unsigned char *) HA1, &Md5Ctx);
      
      ppl_md5_hash_osip_to_hex (HA1, magicstring);
      psp_config_add_element (osip_strdup ("magicstring2"),
			      magicstring);
    }

    if (serverrealm == NULL)
      {
	printf ("Bad configuration: \"serverrealm\" is mandatory.\n");
	partysip_exit (1);
      }

  }

  {
    char *atmp = psp_config_get_element ("loglevel");
    if (atmp!=NULL)
      {
	cf_debug_level = strtol(atmp, (char **)NULL, 10);
	if (cf_debug_level<0 || cf_debug_level>8)
	  cf_debug_level = 6;
      }
  }

  if (cf_debug_level > 0 && cf_log_file == NULL)
    {
      cf_log_file = psp_config_get_element ("log");
    }

  if (cf_debug_level > 0)
    {
      char *atmp;

      printf ("Server:             %s\n", PSP_SERVER_WHO_AM_I);
      printf ("Debug level:        %i\n", cf_debug_level);
      printf ("Config name:        %s\n", cf_config_file);
      atmp = psp_config_get_element ("serverip");
      printf ("ServerIP: (IPv4)    %s\n", atmp);
      atmp = psp_config_get_element ("serverip6");
      if (atmp!=NULL)
	printf ("ServerIP: (IPv6)    %s\n", atmp);
      atmp = psp_config_get_element ("servername");
      printf ("ServerName:         %s\n", atmp);
      atmp = psp_config_get_element ("serverrealm");
      printf ("ServerRealm:        '%s'\n", atmp);
      if (cf_log_file == NULL)
	printf ("Log name:           Standard output\n");
      else
	printf ("Log name:           %s\n", cf_log_file);
    }


  /*********************************/
  /* INIT Log File and Log LEVEL   */

  if (cf_debug_level > 0)
    {
#ifdef ENABLE_SYSLOG
      osip_trace_initialize_syslog (cf_debug_level, "partysip");
#else
      if (cf_log_file != NULL)
	{
	  log_file = fopen (cf_log_file, "w+");
	  if (NULL == log_file)
	    {
	      printf ("Log name:           %s\n", cf_log_file);
	      perror ("ERROR: Could not open log file");
	      exit (1);
	    }
	}
      else
	log_file = NULL;
      TRACE_INITIALIZE (cf_debug_level, log_file);
#endif
    }

  osip_free ((void *) (opt->argv));
  osip_free ((void *) opt);

  {
    char *recovery_file;
    recovery_file  = psp_config_get_element ("recovery_file");
    if (recovery_file!=NULL)
      {
	i = ppl_uinfo_set_dbm(recovery_file);
	if (i!=0)
	  {
	    fprintf (stderr, "ERROR: can't open recovery file!\n");
	  }
      }
  }

  i = psp_core_init ();
  if (i != 0)
    {
      fprintf (stderr, "ERROR: Could not initialize partysip!\n");
      goto main_error;
    }

  /* load general user configuration */
  i = psp_utils_load_users ();
  if (i != 0)
    {
      fprintf (stderr, "ERROR: Could not load user config!\n");
      partysip_exit (0);
    }

  /* from here, let's load plugins */
  i = main_load_plugins ();
  if (i != 0)
    {
      fprintf (stderr, "ERROR: Could not load plugins!\n");
      partysip_exit (0);
    }

#if defined(WIN32) && defined(HAVE_NT_SERVICE_MANAGER)
  SetEvent (started_event);
#endif

  /* start all modules that we want as thread */
  i = psp_core_start (interactive_mode);	/* block if interactive_mode==0 */
  if (i != 0)
    {
      fprintf (stderr, "ERROR: Could not start modules of partysip!\n");
      goto main_error;
    }

  if (interactive_mode == 1)
    main_run ();
  /* Now, it's true for all platforms.
     #ifndef WIN32
     else
     select (0, NULL, NULL, NULL, NULL);
     #endif
   */

  if (interactive_mode == 1)
    partysip_exit (0);		/* else, this is done somewhere else */

  return 0;
main_error:
  partysip_exit (0);
  return -1;
}
