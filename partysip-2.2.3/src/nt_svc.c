/*
 * Copyright 1998-2002 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/*
  Copyright 1998-2002 The OpenLDAP Foundation
  All rights reserved.
  
  Redistribution and use in source and binary forms, with or without
  modification, are permitted only as authorized by the OpenLDAP
  Public License.  A copy of this license is available at
  http://www.OpenLDAP.org/license.html or in file LICENSE in the
  top-level directory of the distribution.

  OpenLDAP is a registered trademark of the OpenLDAP Foundation.
  
  Individual files and/or contributed packages may be copyright by
  other parties and subject to additional restrictions.
  
  This work is derived from the University of Michigan LDAP v3.3
  distribution.  Information concerning this software is available
  at: http://www.umich.edu/~dirsvcs/ldap/
  
  This work also contains materials derived from public sources.
  
  Additional information about OpenLDAP can be obtained at:
  http://www.openldap.org/
  
  ---
  
  Portions Copyright (c) 1992-1996 Regents of the University of Michigan.
  All rights reserved.
  
  Redistribution and use in source and binary forms are permitted
  provided that this notice is preserved and that due credit is given
  to the University of Michigan at Ann Arbor. The name of the University
  may not be used to endorse or promote products derived from this
  software without specific prior written permission. This software
  is provided ``as is'' without express or implied warranty.
*/

#include <partysip.h>


#if defined(WIN32) && defined(HAVE_NT_SERVICE_MANAGER)

/* in main.c */
void WINAPI ServiceMain (DWORD argc, LPTSTR * argv);

/* in ntservice.c */
int srv_install (char *service, char *displayName, char *filename,
		 BOOL auosip_to_start);
int srv_remove (char *service, char *filename);
DWORD svc_installed (LPTSTR lpszServiceName, LPTSTR lpszBinaryPathName);
DWORD svc_running (LPTSTR lpszServiceName);


void partysip_exit (int process_exit_value);

int
main (int argc, LPTSTR * argv)
{
  int length;
  char filename[MAX_PATH], *fname_start;

  /*
   * Because the service was registered as SERVICE_WIN32_OWN_PROCESS,
   * the lpServiceName element of the SERVICE_TABLE_ENTRY will be
   * ignored. Since we don't even know the name of the service at
   * this point (since it could have been installed under a name
   * different than SERVICE_NAME), we might as well just provide
   * the parameter as "".
   */

  SERVICE_TABLE_ENTRY DispatchTable[] = {
    {"Partysip Proxy Server", (LPSERVICE_MAIN_FUNCTION) ServiceMain},
    {NULL, NULL}
  };

  /*
    set the service's current directory to being the
    installation directory for the service.
   */
  GetModuleFileName (NULL, filename, sizeof (filename));
  fname_start = strrchr (filename, '\\');

  *fname_start = '\0';
  SetCurrentDirectory (filename);

  if (argc > 1)
    {
/* change to link with MSVCRTD.lib */
      if (_stricmp ("install", argv[1]) == 0)
	{
	  char *svcName = "Partysip Proxy Server";
	  char *displayName = "Partysip Proxy Server";
	  BOOL auosip_to_start = FALSE;

	  if ((argc > 2) && (argv[2] != NULL))
	    svcName = argv[2];

	  if (argc > 3 && argv[3])
	    displayName = argv[3];

	  if (argc > 4 && _stricmp (argv[4], "auto") == 0)
	    auosip_to_start = TRUE;

	  if ((length =
	       GetModuleFileName (NULL, filename, sizeof (filename))) == 0)
	    {
	      fputs ("unable to retrieve file name for the service.\n",
		     stderr);
	      return EXIT_FAILURE;
	    }
	  if (!srv_install (svcName, displayName, filename, auosip_to_start))
	    {
	      fputs ("service failed installation ...\n", stderr);
	      return EXIT_FAILURE;
	    }
	  fputs ("service has been installed ...\n", stderr);
	  return EXIT_SUCCESS;
	}

      if (_stricmp ("remove", argv[1]) == 0)
	{
	  char *svcName = "Partysip Proxy Server";
	  if ((argc > 2) && (argv[2] != NULL))
	    svcName = argv[2];
	  if ((length =
	       GetModuleFileName (NULL, filename, sizeof (filename))) == 0)
	    {
	      fputs ("unable to retrieve file name for the service.\n",
		     stderr);
	      return EXIT_FAILURE;
	    }
	  if (!srv_remove (svcName, filename))
	    {
	      fputs ("failed to remove the service ...\n", stderr);
	      return EXIT_FAILURE;
	    }
	  fputs ("service has been removed ...\n", stderr);
	  return EXIT_SUCCESS;
	}
    }

  if (svc_installed ("Partysip Proxy Server", NULL) != 0
      || svc_running ("Partysip Proxy Server") == 1
      || StartServiceCtrlDispatcher (DispatchTable) == 0)
    {
      /* This application may not be run as a service?
	 In this cas we can run the method ourself?
	 ServiceMain (argc, argv); */
    }

  return EXIT_SUCCESS;
}

#endif
