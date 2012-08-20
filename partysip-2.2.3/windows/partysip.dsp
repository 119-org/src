# Microsoft Developer Studio Project File - Name="partysip" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=partysip - Win32 with TRE
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "partysip.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "partysip.mak" CFG="partysip - Win32 with TRE"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "partysip - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "partysip - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "partysip - Win32 with TRE" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "partysip - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\partysip" /I ".." /I "..\..\osip\include" /I "..\ppl\win32" /D "_CONSOLE" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "ENABLE_TRACE" /D "OSIP_MT" /YX /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 ppl.lib Ws2_32.lib msvcrt.lib osipparser2.lib osip2.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /nodefaultlib /libpath:".libs" /libpath:"..\..\osip\platform\windows\.libs"

!ELSEIF  "$(CFG)" == "partysip - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "partysip___Win32_Debug"
# PROP BASE Intermediate_Dir "partysip___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".libs"
# PROP Intermediate_Dir "partysip___Win32_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /I "..\partysip" /I ".." /I "..\..\osip\include" /I "..\ppl\win32" /D "_CONSOLE" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "ENABLE_TRACE" /D "OSIP_MT" /FR /YX /FD /D /D /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ppl.lib Ws2_32.lib msvcrtd.lib osipparser2.lib osip2.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib /pdbtype:sept /libpath:".libs" /libpath:"..\..\osip\platform\windows\.libs"

!ELSEIF  "$(CFG)" == "partysip - Win32 with TRE"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "partysip___Win32_with_TRE"
# PROP BASE Intermediate_Dir "partysip___Win32_with_TRE"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".libs"
# PROP Intermediate_Dir "partysip___Win32_with_TRE"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /I "..\ppl\win32" /I "..\partysip" /I ".." /D "_CONSOLE" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "ENABLE_TRACE" /D "ENABLE_DEBUG" /D "OSIP_MT" /D "USE_TMP_BUFFER" /FR /YX /FD /D /D /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /I "..\partysip" /I ".." /I "..\..\osip\include" /I "..\ppl\win32" /D "HAVE_NT_SERVICE_MANAGER" /D "_CONSOLE" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "ENABLE_TRACE" /D "OSIP_MT" /D "TRE" /FR /YX /FD /D /D /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ppl.lib Ws2_32.lib msvcrtd.lib osipparser2.lib osip2.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib /pdbtype:sept /libpath:".libs"
# ADD LINK32 ppl.lib Ws2_32.lib msvcrtd.lib osipparser2.lib osip2.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib /pdbtype:sept /libpath:".libs" /libpath:"..\..\osip\platform\windows\.libs"

!ENDIF 

# Begin Target

# Name "partysip - Win32 Release"
# Name "partysip - Win32 Debug"
# Name "partysip - Win32 with TRE"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\imp.c
# End Source File
# Begin Source File

SOURCE=..\src\imp_plugin.c
# End Source File
# Begin Source File

SOURCE=..\src\main.c
# End Source File
# Begin Source File

SOURCE=..\src\nt_svc.c
# End Source File
# Begin Source File

SOURCE=..\src\ntservice.c
# End Source File
# Begin Source File

SOURCE=..\src\osip_msg.c
# End Source File
# Begin Source File

SOURCE=..\src\psp_config.c
# End Source File
# Begin Source File

SOURCE=..\src\psp_core.c
# End Source File
# Begin Source File

SOURCE=..\src\psp_core2.c
# End Source File
# Begin Source File

SOURCE=..\src\psp_core3.c
# End Source File
# Begin Source File

SOURCE=..\src\psp_core4.c
# End Source File
# Begin Source File

SOURCE=..\src\psp_core5.c
# End Source File
# Begin Source File

SOURCE=..\src\psp_module.c
# End Source File
# Begin Source File

SOURCE=..\src\psp_nat.c
# End Source File
# Begin Source File

SOURCE=..\src\psp_osip.c
# End Source File
# Begin Source File

SOURCE=..\src\psp_plugin.c
# End Source File
# Begin Source File

SOURCE=..\src\psp_request.c
# End Source File
# Begin Source File

SOURCE=..\src\psp_resolv.c
# End Source File
# Begin Source File

SOURCE=..\src\psp_utils.c
# End Source File
# Begin Source File

SOURCE=..\src\sfp.c
# End Source File
# Begin Source File

SOURCE=..\src\sfp_branch.c
# End Source File
# Begin Source File

SOURCE=..\src\sfp_fsm.c
# End Source File
# Begin Source File

SOURCE=..\src\sfp_fsm2.c
# End Source File
# Begin Source File

SOURCE=..\src\sfp_plugin.c
# End Source File
# Begin Source File

SOURCE=..\src\tlp.c
# End Source File
# Begin Source File

SOURCE=..\src\tlp_plugin.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\partysip\osip_msg.h
# End Source File
# Begin Source File

SOURCE=..\partysip\partysip.h
# End Source File
# Begin Source File

SOURCE=..\src\proxyfsm.h
# End Source File
# Begin Source File

SOURCE=..\partysip\psp_config.h
# End Source File
# Begin Source File

SOURCE=..\partysip\psp_macros.h
# End Source File
# Begin Source File

SOURCE=..\partysip\psp_plug.h
# End Source File
# Begin Source File

SOURCE=..\partysip\psp_request.h
# End Source File
# Begin Source File

SOURCE=..\partysip\psp_utils.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
