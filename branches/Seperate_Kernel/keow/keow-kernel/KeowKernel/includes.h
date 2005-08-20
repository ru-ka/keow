// common includes

// include windows first so we catch #define redefinitions
// in linux includes and then we can fix them

//windows stuff
#include "WinFiles.h"


//linux stuff
#include "linux_includes.h"

//safe _after_ the linux stuff (else strcpy etc in linux includes won't compile)
#include <strsafe.h>



//STL
//We'd like to use STL, but it causes lots of std c stuff to be included
//that conflicts with the linux stuff (eg. errno.h)
//So we let Keow use the Win32 API and define our own helper classes
//that don't cause a conflict

#include "List.h"
#include "String.h"


//keow stuff
#include "ConstantMapping.h"
#include "Path.h"
#include "Device.h"
#include "Filesystem.h"
#include "FilesystemKeow.h"
#include "FilesystemProc.h"
#include "StubResourceInfo.h"
#include "Process.h"
#include "MountPoint.h"
#include "KernelTable.h"
#include "MemoryHelper.h"
#include "Utils.h"

