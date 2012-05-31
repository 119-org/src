#include "libpcinfo.h"

int Initialize()
{
	//Initialize Network
	WSADATA wsaData;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if(ret != NO_ERROR)
	{
		printf("Error at WSAStartup(): %ld\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
}
DWORD GetMemoryByPID(DWORD dwProcessID)
{
	PROCESS_MEMORY_COUNTERS pmc;
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID);
	if(hProcess == NULL)
	{
		return -1;
	}
	if(GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
	{
		return pmc.WorkingSetSize;
	}
	return 0;
}
DWORD GetMemoryRate()
{
	MEMORYSTATUS mem;
	GlobalMemoryStatus(&mem);
	return mem.dwMemoryLoad;
}

DWORD GetMemoryTotal()
{
	MEMORYSTATUS mem;
	GlobalMemoryStatus(&mem);
	return mem.dwTotalPhys;	//Bytes
}
DWORD GetMemoryUsed()
{
	MEMORYSTATUS mem;
	GlobalMemoryStatus(&mem);
	return (mem.dwTotalPhys - mem.dwAvailPhys);	//Bytes
}

DWORD GetCpuByProcess(DWORD dwProcessID)
{
	return 0;
}

DWORD GetCpuTotal()
{
	PROCNTQSI   NtQuerySystemInformation;
	NtQuerySystemInformation = (PROCNTQSI)GetProcAddress(GetModuleHandle("ntdll"), "NtQuerySystemInformation");
	if(!NtQuerySystemInformation)
	{
		return -1;
	}
//	NtQuerySystemInformation(SystemBasicInformation, &SysBaseInfo, sizeof(SysBaseInfo), NULL);
}

void DisplayStruct(int i, LPNETRESOURCE lpnrLocal)
{
	printf("NETRESOURCE[%d] Scope: ", i);
	switch (lpnrLocal->dwScope) {
	case (RESOURCE_CONNECTED):
		printf("connected\n");
		break;
	case (RESOURCE_GLOBALNET):
		printf("all resources\n");
		break;
	case (RESOURCE_REMEMBERED):
		printf("remembered\n");
		break;
	default:
		printf("unknown scope %d\n", lpnrLocal->dwScope);
		break;
	}

	printf("NETRESOURCE[%d] Type: ", i);
	switch (lpnrLocal->dwType) {
	case (RESOURCETYPE_ANY):
		printf("any\n");
		break;
	case (RESOURCETYPE_DISK):
		printf("disk\n");
		break;
	case (RESOURCETYPE_PRINT):
		printf("print\n");
		break;
	default:
		printf("unknown type %d\n", lpnrLocal->dwType);
		break;
	}

	printf("NETRESOURCE[%d] DisplayType: ", i);
	switch (lpnrLocal->dwDisplayType) {
	case (RESOURCEDISPLAYTYPE_GENERIC):
		printf("generic\n");
		break;
	case (RESOURCEDISPLAYTYPE_DOMAIN):
		printf("domain\n");
		break;
	case (RESOURCEDISPLAYTYPE_SERVER):
		printf("server\n");
		break;
	case (RESOURCEDISPLAYTYPE_SHARE):
		printf("share\n");
		break;
	case (RESOURCEDISPLAYTYPE_FILE):
		printf("file\n");
		break;
	case (RESOURCEDISPLAYTYPE_GROUP):
		printf("group\n");
		break;
	case (RESOURCEDISPLAYTYPE_NETWORK):
		printf("network\n");
		break;
	default:
		printf("unknown display type %d\n", lpnrLocal->dwDisplayType);
		break;
	}

	printf("NETRESOURCE[%d] Usage: 0x%x = ", i, lpnrLocal->dwUsage);
	if (lpnrLocal->dwUsage & RESOURCEUSAGE_CONNECTABLE)
		printf("connectable ");
	if (lpnrLocal->dwUsage & RESOURCEUSAGE_CONTAINER)
		printf("container ");
	printf("\n");

	printf("NETRESOURCE[%d] Localname: %s\n", i, (char*)lpnrLocal->lpLocalName);
	printf("NETRESOURCE[%d] Remotename: %s\n", i, (char*)lpnrLocal->lpRemoteName);
	printf("NETRESOURCE[%d] Comment: %s\n", i, (char*)lpnrLocal->lpComment);
	printf("NETRESOURCE[%d] Provider: %s\n", i, (char*)lpnrLocal->lpProvider);
	printf("\n");
}
int NetEnumerate(NETRESOURCE* NetResource)
{
	HANDLE hEnum;

	int ret = WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_ANY, NULL, NetResource, &hEnum);
	if(ret != NO_ERROR)
	{
		printf("WnetOpenEnum failed with error %d\n", ret);
		return FALSE;
	}

	DWORD dwCount = -1;
	DWORD dwBufferSize = 16 * 1024;
	LPVOID lpBuffer = new char[dwBufferSize];
	NetResource = (NETRESOURCE*)lpBuffer;
	do
	{
		memset(lpBuffer, 0, dwBufferSize);
		ret = WNetEnumResource(hEnum, &dwCount, NetResource, &dwBufferSize);
		if(ret == NO_ERROR)
		{
			for(int i = 0; i < dwCount; i++)
			{
				DisplayStruct(i, NetResource);
				if((NetResource[i].dwUsage & RESOURCEUSAGE_CONTAINER) != RESOURCEUSAGE_CONTAINER)
				{
					break;
				}
				if(!NetEnumerate(&NetResource[i]))
				{
					string str = NetResource[i].lpRemoteName;
					int pos = str.find("\\\\");
					if(pos != -1)
					{
						string substr = str.substr(pos + 2, str.length());
						hostent* host = gethostbyname(substr.c_str());
						if(host == NULL)
							continue;
						char* localhost = inet_ntoa(*(in_addr*)*host->h_addr_list);
						printf("ip:%s\n", localhost);
					}
				}
			}
		}
		else if(ret == ERROR_NO_MORE_ITEMS)
		{
			printf("WNetEnumResource failed with error %d\n", ret);
			break;
		}
	}while(ret == ERROR_NO_MORE_ITEMS);
	
	ret = WNetCloseEnum(hEnum);
	delete[] lpBuffer;
	return 0;
}
int GetLanInfo()
{
	LPNETRESOURCE lpNetResource = NULL;
	NetEnumerate(lpNetResource);
	return 0;
}
int GetMacAddr()
{
	DWORD dwRetVal;
	IPAddr DestIp = 0;
	IPAddr SrcIp = 0;       /* default for src ip */
	ULONG MacAddr[2];       /* for 6-byte hardware addresses */
	ULONG PhysAddrLen = 6;  /* default to length of six bytes */

	char *DestIpString = NULL;"www.baidu.com";
	char *SrcIpString = NULL;
	BYTE *bPhysAddr;

	DestIp = inet_addr(DestIpString);

	memset(&MacAddr, 0xff, sizeof (MacAddr));

	printf("Sending ARP request for IP address: %s\n", DestIpString);

	dwRetVal = SendARP(DestIp, SrcIp, &MacAddr, &PhysAddrLen);

	if (dwRetVal == NO_ERROR) {
		bPhysAddr = (BYTE *) & MacAddr;
		if (PhysAddrLen) {
			for (int i = 0; i < (int) PhysAddrLen; i++) {
				if (i == (PhysAddrLen - 1))
					printf("%.2X\n", (int) bPhysAddr[i]);
				else
					printf("%.2X-", (int) bPhysAddr[i]);
			}
		} else
			printf
			("Warning: SendArp completed successfully, but returned length=0\n");

	} else {
		printf("Error: SendArp failed with error: %d", dwRetVal);
		switch (dwRetVal) {
		case ERROR_GEN_FAILURE:
			printf(" (ERROR_GEN_FAILURE)\n");
			break;
		case ERROR_INVALID_PARAMETER:
			printf(" (ERROR_INVALID_PARAMETER)\n");
			break;
		case ERROR_INVALID_USER_BUFFER:
			printf(" (ERROR_INVALID_USER_BUFFER)\n");
			break;
		case ERROR_BAD_NET_NAME:
			printf(" (ERROR_GEN_FAILURE)\n");
			break;
		case ERROR_BUFFER_OVERFLOW:
			printf(" (ERROR_BUFFER_OVERFLOW)\n");
			break;
		case ERROR_NOT_FOUND:
			printf(" (ERROR_NOT_FOUND)\n");
			break;
		default:
			printf("\n");
			break;
		}
	}

	return 0;

}

int GetCpuTemperature()
{
	CoInitializeEx(0,COINIT_MULTITHREADED);
	try 
	{
		if(SUCCEEDED(CoInitializeSecurity(NULL,-1,NULL,NULL,RPC_C_AUTHN_LEVEL_DEFAULT,RPC_C_IMP_LEVEL_IMPERSONATE,NULL,EOAC_NONE,NULL))) 
		{
			IWbemLocator *pLoc=NULL;
			if(SUCCEEDED(CoCreateInstance(CLSID_WbemLocator,0,CLSCTX_INPROC_SERVER,IID_IWbemLocator,(LPVOID *)&pLoc)))
			{
				IWbemServices *pSvc=NULL;
				if(SUCCEEDED(pLoc->ConnectServer(_bstr_t(L"ROOT\\WMI"),NULL,NULL,0,NULL,0,0,&pSvc)))
				{
					if(SUCCEEDED(CoSetProxyBlanket(pSvc,RPC_C_AUTHN_WINNT,RPC_C_AUTHZ_NONE,NULL,RPC_C_AUTHN_LEVEL_CALL,RPC_C_IMP_LEVEL_IMPERSONATE,NULL,EOAC_NONE)))
					{
						IEnumWbemClassObject* pEnumerator=NULL;
						if(SUCCEEDED(pSvc->ExecQuery(bstr_t("WQL"),bstr_t("SELECT * FROM MSAcpi_ThermalZoneTemperature"),WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY,NULL,&pEnumerator)))
						{
							IWbemClassObject *pclsObj;
							ULONG uReturn=0;
							while(pEnumerator)
							{
								pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn); 
								if(0==uReturn)
									break;
								VARIANT vtProp;
								VariantInit(&vtProp);
								pclsObj->Get(L"CurrentTemperature", 0, &vtProp, 0, 0);
								wcout << "Current CPU Temperature : " << (vtProp.intVal - 2732)/10.0 << endl;
								VariantClear(&vtProp);
								pclsObj->Release();
							}
						}
					}
					pSvc->Release();
				}
				pLoc->Release();
			}
		}
	}
	catch (_com_error err)
	{
	}
	CoUninitialize();
	return 0; 
}
DWORD GetCPUID()
{
	unsigned long s1,s2;
	unsigned long high;
	unsigned long middle;
	unsigned long low;
	unsigned char vendor_id[]="------------";//CPU提供商ID
	char szCPUID[128] = {0};
	char szVendorID[128] = {0};
	// 以下为获得CPU ID的汇编语言指令
	_asm    // 得到CPU提供商信息 
	{
		xor eax,eax   // 将eax清0
		cpuid         // 获取CPUID的指令
		mov dword ptr vendor_id,ebx
		mov dword ptr vendor_id[+4],edx
		mov dword ptr vendor_id[+8],ecx  
	}
	wsprintf(szVendorID, "%s", vendor_id);
	_asm    // 得到CPU ID的高32位 
	{
		mov eax,01h
		xor edx,edx
		cpuid
		mov s2,eax
	}
	high = s2;
	_asm    // 得到CPU ID的低64位
	{ 
		mov eax,03h
		xor ecx,ecx
		xor edx,edx
		cpuid
		mov s1,edx
		mov s2,ecx
	}
	middle = s1;
	low = s2;
	wsprintf(szCPUID, "%08X-%08X-%08X", high, middle, low);//str

	printf("Vendor:%s\n", szVendorID);
	printf("CPUID:%s\n", szCPUID);
	return 0;
}
int main(int argc, char** argv)
{
	Initialize();
	//printf("GetMemoryRate = %6d%%\n", GetMemoryRate());
	//printf("GetMemoryTotal = %6d\n", GetMemoryTotal());
	//printf("GetMemoryUsed = %6d\n", GetMemoryUsed());
	//printf("GetMemoryByPID = %6d\n", GetMemoryByPID(GetCurrentProcessId())/1024);
	//GetLanInfo();
	//GetMacAddr();
	//GetCpuTemperature();
	GetCPUID();
	return 0;
}