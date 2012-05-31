#ifndef _LIBPCINFO_H_
#define _LIBPCINFO_H_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <windows.h>

//LAN Info
#include <Psapi.h>
#include <Iphlpapi.h>
#pragma comment(lib, "Psapi.lib")
#pragma comment( lib,"Ws2_32.lib" )
#pragma comment( lib,"Mpr.lib" )
#pragma comment( lib,"iphlpapi.lib" )

//CPU ID
#define _WIN32_DCOM
#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>
# pragma comment(lib, "wbemuuid.lib")

using namespace std;

int Initialize();
DWORD GetMemoryByProcess(DWORD dwProcessID);
DWORD GetMemoryRate();
DWORD GetMemoryUsed();
DWORD GetMemoryTotal();

typedef LONG (WINAPI *PROCNTQSI)(UINT, PVOID, ULONG, PULONG);
DWORD GetCpuByProcess(DWORD dwProcessID);
DWORD GetCpuUsed();
DWORD GetCpuTotal();
DWORD GetCPUID();

int GetLanInfo();
int GetMacAddr();

int GetCpuTemperature();
int GetBoardTemperature();
int GetHardDiskTemperature();
int GetCpuFanSpeed();


LPTSTR GetProgramDir();
DWORD GetCapacityByDisk();
DWORD GetOSVersion();













#endif