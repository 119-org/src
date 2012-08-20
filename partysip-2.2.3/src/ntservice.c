/* $OpenLDAP: pkg/ldap/libraries/liblutil/ntservice.c,v 1.21.2.1 2002/07/28 19:18:14 kurt Exp $ */
/*
 * Copyright 1998-2002 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

/*
 * NT Service manager utilities for OpenLDAP services
 *	these should NOT be slapd specific, but are
 */

#if defined(WIN32) && defined(HAVE_NT_SERVICE_MANAGER)

#include "partysip.h"
#include <osip2/osip_mt.h>

#include <windows.h>
#include <winsvc.h>

/*
 * The whole debug implementation is a bit of a hack.
 * We have to define this LDAP_SLAPD_V macro here since the slap.h
 * header file isn't included (and really shouldn't be).
 * TODO: re-implement debug functions so that the debug level can
 * be passed around instead of havi+
 
   ng to count on the existence of
 * ldap_debug, slap_debug, etc.
 */

#define SCM_NOTIFICATION_INTERVAL	5000
#define THIRTY_SECONDS				(30 * 1000)

int is_NT_Service = 1;		/* assume this is an NT service until determined that */
							/* startup was from the command line */
HANDLE started_event, stopped_event;
HANDLE start_status_tid, stop_status_tid;

SERVICE_STATUS SLAPDServiceStatus;
SERVICE_STATUS_HANDLE hSLAPDServiceStatus;


void (*stopfunc) (int);

static char *GetLastErrorString (void);

VOID SvcDebugOut(LPSTR String, DWORD Status) 
{ 
    CHAR  Buffer[1024]; 
    if (strlen(String) < 1000) 
    { 
        sprintf(Buffer, String, Status); 
        OutputDebugStringA(Buffer); 
    } 
}

int
srv_install (LPCTSTR lpszServiceName, LPCTSTR lpszDisplayName,
	     LPCTSTR lpszBinaryPathName, BOOL auosip_to_start)
{
  HKEY hKey;
  DWORD dwValue, dwDisposition;
  SC_HANDLE schSCManager, schService;

  fprintf (stderr, "The install path is %s.\n", lpszBinaryPathName);
  if ((schSCManager =
       OpenSCManager (NULL, NULL,
		      SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE)) !=
      NULL)
    {
      if ((schService = CreateService (schSCManager,
				       lpszServiceName,
				       lpszDisplayName,
				       SERVICE_ALL_ACCESS,
				       SERVICE_WIN32_OWN_PROCESS,
				       auosip_to_start ? SERVICE_AUTO_START :
				       SERVICE_DEMAND_START,
				       SERVICE_ERROR_NORMAL,
				       lpszBinaryPathName, NULL, NULL, NULL,
				       NULL, NULL)) != NULL)
	{
	  char regpath[132];
	  CloseServiceHandle (schService);
	  CloseServiceHandle (schSCManager);

	  _snprintf (regpath, sizeof regpath,
		     "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\%s",
		     lpszServiceName);
	  /* Create the registry key for event logging to the Windows NT event log. */
	  if (RegCreateKeyEx (HKEY_LOCAL_MACHINE,
			      regpath, 0,
			      "REG_SZ", REG_OPTION_NON_VOLATILE,
			      KEY_ALL_ACCESS, NULL, &hKey,
			      &dwDisposition) != ERROR_SUCCESS)
	    {
	      fprintf (stderr,
		       "RegCreateKeyEx() failed. GetLastError=%lu (%s)\n",
		       GetLastError (), GetLastErrorString ());
	      RegCloseKey (hKey);
	      return (0);
	    }
	  if (RegSetValueEx
	      (hKey, "EventMessageFile", 0, REG_EXPAND_SZ, lpszBinaryPathName,
	       strlen (lpszBinaryPathName) + 1) != ERROR_SUCCESS)
	    {
	      fprintf (stderr,
		       "RegSetValueEx(EventMessageFile) failed. GetLastError=%lu (%s)\n",
		       GetLastError (), GetLastErrorString ());
	      RegCloseKey (hKey);
	      return (0);
	    }

	  dwValue =
	    EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE |
	    EVENTLOG_INFORMATION_TYPE;
	  if (RegSetValueEx
	      (hKey, "TypesSupported", 0, REG_DWORD, (LPBYTE) & dwValue,
	       sizeof (DWORD)) != ERROR_SUCCESS)
	    {
	      fprintf (stderr,
		       "RegCreateKeyEx(TypesSupported) failed. GetLastError=%lu (%s)\n",
		       GetLastError (), GetLastErrorString ());
	      RegCloseKey (hKey);
	      return (0);
	    }
	  RegCloseKey (hKey);
	  return (1);
	}
      else
	{
	  fprintf (stderr, "CreateService() failed. GetLastError=%lu (%s)\n",
		   GetLastError (), GetLastErrorString ());
	  CloseServiceHandle (schSCManager);
	  return (0);
	}
    }
  else
    fprintf (stderr, "OpenSCManager() failed. GetLastError=%lu (%s)\n",
	     GetLastError (), GetLastErrorString ());
  return (0);
}


int
srv_remove (LPCTSTR lpszServiceName, LPCTSTR lpszBinaryPathName)
{
  SC_HANDLE schSCManager, schService;

  fprintf (stderr, "The installed path is %s.\n", lpszBinaryPathName);
  if ((schSCManager =
       OpenSCManager (NULL, NULL,
		      SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE)) !=
      NULL)
    {
      if ((schService =
	   OpenService (schSCManager, lpszServiceName, DELETE)) != NULL)
	{
	  if (DeleteService (schService) == TRUE)
	    {
	      CloseServiceHandle (schService);
	      CloseServiceHandle (schSCManager);
	      return (1);
	    }
	  else
	    {
	      fprintf (stderr,
		       "DeleteService() failed. GetLastError=%lu (%s)\n",
		       GetLastError (), GetLastErrorString ());
	      fprintf (stderr, "The %s service has not been removed.\n",
		       lpszBinaryPathName);
	      CloseServiceHandle (schService);
	      CloseServiceHandle (schSCManager);
	      return (0);
	    }
	}
      else
	{
	  fprintf (stderr, "OpenService() failed. GetLastError=%lu (%s)\n",
		   GetLastError (), GetLastErrorString ());
	  CloseServiceHandle (schSCManager);
	  return (0);
	}
    }
  else
    fprintf (stderr, "OpenSCManager() failed. GetLastError=%lu (%s)\n",
	     GetLastError (), GetLastErrorString ());
  return (0);
}


DWORD
svc_installed (LPTSTR lpszServiceName, LPTSTR lpszBinaryPathName)
{
  char buf[256];
  HKEY key;
  DWORD rc;
  DWORD type;
  long len;

  strcpy (buf, TEXT ("SYSTEM\\CurrentControlSet\\Services\\"));
  strcat (buf, lpszServiceName);
  if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, buf, 0, KEY_QUERY_VALUE, &key) !=
      ERROR_SUCCESS)
    return (-1);

  rc = 0;
  if (lpszBinaryPathName)
    {
      len = sizeof (buf);
      if (RegQueryValueEx (key, "ImagePath", NULL, &type, buf, &len) ==
	  ERROR_SUCCESS)
	{
	  if (strcmp (lpszBinaryPathName, buf))
	    rc = -1;
	}
    }
  RegCloseKey (key);
  return (rc);
}


DWORD
svc_running (LPTSTR lpszServiceName)
{
  SC_HANDLE service;
  SC_HANDLE scm;
  DWORD rc;
  SERVICE_STATUS ss;

  if (!(scm = OpenSCManager (NULL, NULL, GENERIC_READ)))
    return (GetLastError ());

  rc = 1;
  service = OpenService (scm, lpszServiceName, SERVICE_QUERY_STATUS);
  if (service)
    {
      if (!QueryServiceStatus (service, &ss))
	rc = GetLastError ();
      else if (ss.dwCurrentState != SERVICE_STOPPED)
	rc = 0;
      CloseServiceHandle (service);
    }
  CloseServiceHandle (scm);
  return (rc);
}


static void *
start_status_routine (void *ptr)
{
  DWORD wait_result;
  int done = 0;

  while (!done)
    {
      wait_result =
	WaitForSingleObject (started_event, SCM_NOTIFICATION_INTERVAL);
      switch (wait_result)
	{
	case WAIT_ABANDONED:
	case WAIT_OBJECT_0:
	  /* the object that we were waiting for has been destroyed (ABANDONED) or
	   * signalled (TIMEOUT_0). We can assume that the startup process is
	   * complete and tell the Service Control Manager that we are now runnng */
	  SLAPDServiceStatus.dwCurrentState = SERVICE_RUNNING;
	  SLAPDServiceStatus.dwWin32ExitCode = NO_ERROR;
	  SLAPDServiceStatus.dwCheckPoint++;
	  SLAPDServiceStatus.dwWaitHint = 1000;
	  SetServiceStatus (hSLAPDServiceStatus, &SLAPDServiceStatus);
#if 0
	  SvcDebugOut(" [PARTYSIP] Service is running %d\n",0);
#endif
			done = 1;
	  break;
	case WAIT_TIMEOUT:
	  /* We've waited for the required time, so send an update to the Service Control 
	   * Manager saying to wait again. */
	  SLAPDServiceStatus.dwCheckPoint++;
	  SLAPDServiceStatus.dwWaitHint = SCM_NOTIFICATION_INTERVAL * 2;
	  SetServiceStatus (hSLAPDServiceStatus, &SLAPDServiceStatus);
	  break;
	case WAIT_FAILED:
	  /* theres been some problem with WaitForSingleObject so tell the Service
	   * Control Manager to wait 30 seconds before deploying its assasin and 
	   * then leave the thread. */
	  SLAPDServiceStatus.dwCheckPoint++;
	  SLAPDServiceStatus.dwWaitHint = THIRTY_SECONDS;
	  SetServiceStatus (hSLAPDServiceStatus, &SLAPDServiceStatus);
	  done = 1;
	  break;
	}
    }
#if 0
  SvcDebugOut(" [PARTYSIP] Exit from start_status_routing.%d\n",0);
#endif
  osip_thread_exit ();
  return NULL;
}



static void *
stop_status_routine (void *ptr)
{
  DWORD wait_result;
  int done = 0;

  while (!done)
    {
      wait_result =
	WaitForSingleObject (stopped_event, SCM_NOTIFICATION_INTERVAL);
      switch (wait_result)
	{
	case WAIT_ABANDONED:
	case WAIT_OBJECT_0:
	  /* the object that we were waiting for has been destroyed (ABANDONED) or
	   * signalled (TIMEOUT_0). The shutting down process is therefore complete 
	   * and the final SERVICE_STOPPED message will be sent to the service control
	   * manager prior to the process terminating. */
		SvcDebugOut(" [PARTYSIP] The service has terminated %d\n",0);
	  done = 1;
	  break;
	case WAIT_TIMEOUT:
	  /* We've waited for the required time, so send an update to the Service Control 
	   * Manager saying to wait again. */
	  SLAPDServiceStatus.dwCheckPoint++;
	  SLAPDServiceStatus.dwWaitHint = SCM_NOTIFICATION_INTERVAL * 2;
	  SetServiceStatus (hSLAPDServiceStatus, &SLAPDServiceStatus);
	  break;
	case WAIT_FAILED:
	  /* theres been some problem with WaitForSingleObject so tell the Service
	   * Control Manager to wait 30 seconds before deploying its assasin and 
	   * then leave the thread. */
	  SLAPDServiceStatus.dwCheckPoint++;
	  SLAPDServiceStatus.dwWaitHint = THIRTY_SECONDS;
	  SetServiceStatus (hSLAPDServiceStatus, &SLAPDServiceStatus);
	  done = 1;
	  break;
	default:
	  break;

	}
    }
  osip_thread_exit ();
  return NULL;
}



void WINAPI
SLAPDServiceCtrlHandler (IN DWORD Opcode)
{
  switch (Opcode)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:

      OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_INFO1, NULL,
			      "Service Shutdown ordered\n"));
      SLAPDServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
      SLAPDServiceStatus.dwCheckPoint++;
      SLAPDServiceStatus.dwWaitHint = SCM_NOTIFICATION_INTERVAL * 2;
      SetServiceStatus (hSLAPDServiceStatus, &SLAPDServiceStatus);

      stopped_event = CreateEvent (NULL, FALSE, FALSE, NULL);
      if (stopped_event == NULL)
	{
	  /* the event was not created. We will ask the service control manager for 30
	   * seconds to shutdown */
	  SLAPDServiceStatus.dwCheckPoint++;
	  SLAPDServiceStatus.dwWaitHint = THIRTY_SECONDS;
	  SetServiceStatus (hSLAPDServiceStatus, &SLAPDServiceStatus);
	}
      else
	{
	  /* start a thread to report the progress to the service control manager 
	   * until the stopped_event is fired. */
	  unsigned tid;
	  stop_status_tid =
	    _beginthreadex (NULL, 0, stop_status_routine, NULL, 0, &tid);
	  if (stop_status_tid != NULL)
	    {

	    }
	  else
	    {
	      /* failed to create the thread that tells the Service Control Manager that the
	       * service stopping is proceeding. 
	       * tell the Service Control Manager to wait another 30 seconds before deploying its
	       * assasin.  */
	      SLAPDServiceStatus.dwCheckPoint++;
	      SLAPDServiceStatus.dwWaitHint = THIRTY_SECONDS;
	      SetServiceStatus (hSLAPDServiceStatus, &SLAPDServiceStatus);
	    }
	}
      stopfunc (1);
      break;

    case SERVICE_CONTROL_INTERROGATE:
      SetServiceStatus (hSLAPDServiceStatus, &SLAPDServiceStatus);
#if 0
      SvcDebugOut(" [PARTYSIP] service control interoggate.%d\n",SLAPDServiceStatus.dwCurrentState);
#endif
      break;
    }
  return;
}


void
CommenceStartupProcessing (LPCTSTR lpszServiceName, void (*stopper) (int))
{
  hSLAPDServiceStatus =
    RegisterServiceCtrlHandler (lpszServiceName,
				(LPHANDLER_FUNCTION) SLAPDServiceCtrlHandler);
  if (hSLAPDServiceStatus == (SERVICE_STATUS_HANDLE)0)
  {
	  SvcDebugOut(" [PARTYSIP] Failed to get a status handle! %d\n",
		  GetLastError());
	return;
  }

  stopfunc = stopper;

  /* initialize the Service Status structure */
  SLAPDServiceStatus.dwServiceType = SERVICE_WIN32; /* _OWN_PROCESS; */
  SLAPDServiceStatus.dwCurrentState = SERVICE_START_PENDING;
  SLAPDServiceStatus.dwControlsAccepted =
    SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
  SLAPDServiceStatus.dwWin32ExitCode = NO_ERROR;
  SLAPDServiceStatus.dwServiceSpecificExitCode = 0;
  SLAPDServiceStatus.dwCheckPoint = 0;
  SLAPDServiceStatus.dwWaitHint = SCM_NOTIFICATION_INTERVAL * 5;

  SetServiceStatus (hSLAPDServiceStatus, &SLAPDServiceStatus);

  /* start up a thread to keep sending SERVICE_START_PENDING to the Service Control Manager
   * until the slapd listener is completed and listening. Only then should we send 
   * SERVICE_RUNNING to the Service Control Manager. */
  started_event = CreateEvent (NULL, FALSE, FALSE, NULL);
  if (started_event == NULL)
    {
      /* failed to create the event to determine when the startup process is complete so
       * tell the Service Control Manager to wait another 30 seconds before deploying its
       * assasin  */
      SLAPDServiceStatus.dwCheckPoint++;
      SLAPDServiceStatus.dwWaitHint = THIRTY_SECONDS;
      SetServiceStatus (hSLAPDServiceStatus, &SLAPDServiceStatus);
    }
  else
    {
      /* start a thread to report the progress to the service control manager 
       * until the started_event is fired.  */
      unsigned tid;
      start_status_tid =
	_beginthreadex (NULL, 0, start_status_routine, NULL, 0, &tid);
      if (start_status_tid != NULL)
	{
	}
      else
	{
	  /* failed to create the thread that tells the Service Control Manager that the
	   * service startup is proceeding. 
	   * tell the Service Control Manager to wait another 30 seconds before deploying its
	   * assasin.  */
	  SLAPDServiceStatus.dwCheckPoint++;
	  SLAPDServiceStatus.dwWaitHint = THIRTY_SECONDS;
	  SetServiceStatus (hSLAPDServiceStatus, &SLAPDServiceStatus);
	  SvcDebugOut(" [PARTYSIP] Failed to start the service checker! %d\n",
		  GetLastError());
	}
    }
}


void
ReportPartysipShutdownComplete ()
{
  DWORD status;
  /* stop sending SERVICE_STOP_PENDING messages
     to the Service Control Manager */
  SetEvent (stopped_event);
  CloseHandle (stopped_event);

  /* wait for the thread sending the SERVICE_STOP_PENDING
     messages to the Service Control Manager to die.
     if the wait fails then put ourselves to sleep for
     half the Service Control Manager update interval */
  status = WaitForSingleObject ((HANDLE) stop_status_tid, INFINITE);
  if (status == WAIT_FAILED)
    Sleep (SCM_NOTIFICATION_INTERVAL / 2);

  SLAPDServiceStatus.dwCurrentState = SERVICE_STOPPED;
  SLAPDServiceStatus.dwCheckPoint++;
  SLAPDServiceStatus.dwWaitHint = SCM_NOTIFICATION_INTERVAL;
  SetServiceStatus (hSLAPDServiceStatus, &SLAPDServiceStatus);
}


static char *
GetErrorString (int err)
{
  static char msgBuf[1024];

  FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM,
		 NULL,
		 err, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
		 msgBuf, 1024, NULL);

  return msgBuf;
}

static char *
GetLastErrorString (void)
{
  return GetErrorString (GetLastError ());
}
#endif
