# Microsoft Developer Studio Project File - Name="ppl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ppl - Win32 with TRE
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ppl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ppl.mak" CFG="ppl - Win32 with TRE"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ppl - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ppl - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ppl - Win32 with TRE" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ppl - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".libs"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PPL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\osip\include" /I "..\ppl\win32" /D "_WINDOWS" /D "_USRDLL" /D "PPL_EXPORTS" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "ENABLE_TRACE" /D "OSIP_MT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 msvcrt.lib Iphlpapi.lib Ws2_32.lib osipparser2.lib osip2.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /nodefaultlib /libpath:".libs" /libpath:"..\..\osip\platform\windows\.libs"

!ELSEIF  "$(CFG)" == "ppl - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".libs"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PPL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /I "..\..\osip\include" /I "..\ppl\win32" /D "_WINDOWS" /D "_USRDLL" /D "PPL_EXPORTS" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "ENABLE_TRACE" /D "OSIP_MT" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msvcrtd.lib Iphlpapi.lib Ws2_32.lib osipparser2.lib osip2.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /nodefaultlib /pdbtype:sept /libpath:".libs" /libpath:"..\..\osip\platform\windows\.libs"

!ELSEIF  "$(CFG)" == "ppl - Win32 with TRE"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ppl___Win32_with_TRE"
# PROP BASE Intermediate_Dir "ppl___Win32_with_TRE"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".libs"
# PROP Intermediate_Dir "ppl___Win32_with_TRE"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /I "..\ppl\win32" /D "_WINDOWS" /D "_USRDLL" /D "PPL_EXPORTS" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "ENABLE_TRACE" /D "ENABLE_DEBUG" /D "OSIP_MT" /D "USE_TMP_BUFFER" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /I "..\..\osip\include" /I "..\ppl\win32" /D "_WINDOWS" /D "_USRDLL" /D "PPL_EXPORTS" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "ENABLE_TRACE" /D "OSIP_MT" /D "TRE" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 Ws2_32.lib msvcrtd.lib osipparser2.lib osip2.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /nodefaultlib /pdbtype:sept /libpath:".libs"
# ADD LINK32 msvcrtd.lib Iphlpapi.lib Ws2_32.lib osipparser2.lib osip2.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /nodefaultlib /pdbtype:sept /libpath:".libs" /libpath:"..\..\osip\platform\windows\.libs"

!ENDIF 

# Begin Target

# Name "ppl - Win32 Release"
# Name "ppl - Win32 Debug"
# Name "ppl - Win32 with TRE"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\ppl\win32\ppldbm.c
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\ppldns.c
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\ppldso.c
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\pplgetopt.c
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\pplinit.c
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\pplmd5.c
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\pplpipe.c
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\pplsocket.c
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\ppltime.c
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\ppluinfo.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\ppl\win32\ppl\ppl.h
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\ppl\ppl_dbm.h
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\ppl\ppl_dns.h
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\ppl\ppl_dso.h
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\ppl\ppl_getopt.h
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\ppl\ppl_init.h
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\ppl\ppl_md5.h
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\ppl\ppl_pipe.h
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\ppl\ppl_socket.h
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\ppl\ppl_time.h
# End Source File
# Begin Source File

SOURCE=..\ppl\win32\ppl\ppl_uinfo.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
