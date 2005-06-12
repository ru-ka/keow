# Microsoft Developer Studio Project File - Name="KeowKernel" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=KeowKernel - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "KeowKernel.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "KeowKernel.mak" CFG="KeowKernel - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "KeowKernel - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "KeowKernel - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "KeowKernel - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /I "../kernel-headers-2.4.24-1/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x1409 /d "NDEBUG"
# ADD RSC /l 0x1409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib advapi32.lib ole32.lib shell32.lib shlwapi.lib uuid.lib /nologo /version:0.1 /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "KeowKernel - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I "../kernel-headers-2.4.24-1/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x1409 /d "_DEBUG"
# ADD RSC /l 0x1409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib advapi32.lib ole32.lib shell32.lib shlwapi.lib uuid.lib /nologo /version:0.1 /subsystem:windows /map /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ENDIF 

# Begin Target

# Name "KeowKernel - Win32 Release"
# Name "KeowKernel - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\core.cpp
# End Source File
# Begin Source File

SOURCE=.\filesys.cpp
# End Source File
# Begin Source File

SOURCE=.\forkexec.cpp
# End Source File
# Begin Source File

SOURCE=.\fs_keow.cpp
# End Source File
# Begin Source File

SOURCE=.\fs_proc.cpp
# End Source File
# Begin Source File

SOURCE=.\ioh.cpp
# End Source File
# Begin Source File

SOURCE=.\ioh_console.cpp
# End Source File
# Begin Source File

SOURCE=.\ioh_devnull.cpp
# End Source File
# Begin Source File

SOURCE=.\ioh_devzero.cpp
# End Source File
# Begin Source File

SOURCE=.\ioh_file.cpp
# End Source File
# Begin Source File

SOURCE=.\ioh_pipe.cpp
# End Source File
# Begin Source File

SOURCE=.\ioh_proc.cpp
# End Source File
# Begin Source File

SOURCE=.\kernel.cpp
# End Source File
# Begin Source File

SOURCE=.\loadelf.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\memory.cpp
# End Source File
# Begin Source File

SOURCE=.\path.cpp
# End Source File
# Begin Source File

SOURCE=.\shortcuts.cpp
# End Source File
# Begin Source File

SOURCE=.\signals.cpp
# End Source File
# Begin Source File

SOURCE=.\sys_io.cpp
# End Source File
# Begin Source File

SOURCE=.\sys_mem.cpp
# End Source File
# Begin Source File

SOURCE=.\sys_mount.cpp
# End Source File
# Begin Source File

SOURCE=.\sys_net.cpp
# End Source File
# Begin Source File

SOURCE=.\sys_none.cpp
# End Source File
# Begin Source File

SOURCE=.\sys_perms.cpp
# End Source File
# Begin Source File

SOURCE=.\sys_process.cpp
# End Source File
# Begin Source File

SOURCE=.\sys_rtsig.cpp
# End Source File
# Begin Source File

SOURCE=.\sys_sys.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\filesys.h
# End Source File
# Begin Source File

SOURCE=.\forkexec.h
# End Source File
# Begin Source File

SOURCE=.\iohandler.h
# End Source File
# Begin Source File

SOURCE=.\kernel.h
# End Source File
# Begin Source File

SOURCE=.\linux_includes.h
# End Source File
# Begin Source File

SOURCE=.\loadelf.h
# End Source File
# Begin Source File

SOURCE=.\memory.h
# End Source File
# Begin Source File

SOURCE=.\path.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\tib.h
# End Source File
# Begin Source File

SOURCE=.\types.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Resources.rc
# End Source File
# Begin Source File

SOURCE=.\Tux.ico
# End Source File
# End Group
# End Target
# End Project
