#pragma once

#ifndef CvGameCoreDLL_h
#define CvGameCoreDLL_h

// includes (pch) for gamecore dll files
// Author - Mustafa Thamer

// <advc.make>
#include "PragmaWarnings.h" // Customize compiler warnings
#pragma warning(push, 3) // Don't check win, std and boost headers with /W4
// </advc.make>

// WINDOWS ...
#define WIN32_LEAN_AND_MEAN
// <advc.fract> Otherwise, classes in the PCH can't have members named "max" and "min".
#ifndef NOMINMAX
	#define NOMINMAX
#endif // </advc.fract>
#include <windows.h>

// advc.fract: Commented out, originally in CvGameCoreUtils.h.
//#undef max
//#undef min
#include <MMSystem.h>
#if defined _DEBUG && !defined USE_MEMMANAGER
	#define USE_MEMMANAGER
	#include <crtdbg.h>
#endif
#include <vector>
#include <list>
#include <tchar.h>
//#include <math.h>
//#include <assert.h>
// <advc>
#include <cmath>
#include <cassert>
#include <iterator>
#include <sstream> // </advc>
#include <map>
#include <hash_map>
// K-Mod
#include <set>
#include <utility>
#include <algorithm>
#include <queue> // k146
#include <stack> // advc.030
// K-Mod end

#define DllExport   __declspec( dllexport )

typedef unsigned char		byte;
// advc (note): A little strange to me, but consistent with WORD in winnt.h.
typedef unsigned short		word;
typedef unsigned int		uint;
typedef unsigned long		dword;
typedef unsigned __int64	qword;
typedef wchar_t				wchar;

/*	advc.001q: Put minus operators into the negative constants, otherwise,
	if there's only a literal, it can get treated as an unsigned value. */
#define MAX_CHAR							(0x7f)
//#define MIN_CHAR							(0x80)
#define MIN_CHAR							(-MAX_CHAR - 1)
#define MAX_SHORT							(0x7fff)
//#define MIN_SHORT							(0x8000)
#define MIN_SHORT							(-MAX_SHORT - 1)
#define MAX_INT								(0x7fffffff)
//#define MIN_INT							(0x80000000)
#define MIN_INT								(-MAX_INT - 1)
#define MAX_UNSIGNED_CHAR					(0xff)
#define MIN_UNSIGNED_CHAR					(0x00)
#define MAX_UNSIGNED_SHORT					(0xffff)
#define MIN_UNSIGNED_SHORT					(0x0000)
#define MAX_UNSIGNED_INT					(0xffffffff)
#define MIN_UNSIGNED_INT					(0x00000000)
/*	advc: These are unused. FLT_MAX and FLT_MIN are used in a few places,
	so let's keep using those exclusively. */
/*__forceinline DWORD FtoDW( float f ) { return *(DWORD*)&f; }
__forceinline float DWtoF( dword n ) { return *(float*)&n; }
__forceinline float MaxFloat() { return DWtoF(0x7f7fffff); }*/

// (advc.make: Some macros moved into new header Trigonometry.h)

// <advc.003s> For generating variable names. (The layer of indirection is necessary.)
#define CONCATVARNAME_IMPL(prefix, lineNum) prefix##lineNum
#define CONCATVARNAME(prefix, suffix) CONCATVARNAME_IMPL(prefix, suffix) // </advc.003s>

// <advc> Stuff moved into separate headers
#include "GameBryo.h"
#include "CvMemoryManager.h"
#include "BoostPythonPCH.h" // </advc>
#pragma warning(pop) // advc.make: Restore project warning level
#include "FAssert.h"
#include "CvGameCoreDLLDefNew.h"
#include "FDataStreamBase.h"
#include "FFreeListTrashArray.h" // advc.003s: includes FreeListTraversal.h
#include "LinkedList.h"
#include "CvString.h"
#include "BitUtil.h" // advc.enum
#include "CvDefines.h"
#include "CvEnums.h" // includes CvInfoEnum.h
/*  advc: Smaller numbers may already crash the EXE; the DLL assumes in some places
	that player ids fit in a single byte. */
BOOST_STATIC_ASSERT(MAX_PLAYERS < MAX_CHAR && MAX_TEAMS < MAX_CHAR);
#include "CvStructs.h"
#include "CvDLLUtilityIFaceBase.h"

//jason tests (advc.make: removed most of these)
#include "CvRandom.h"
#include "CvGameCoreUtils.h"
#include "ScaledNum.h"
#include "CvGlobals.h"
#include "EnumMap2D.h" // advc.enum: Includes EnumMap.h
#include "CvPythonCaller.h" // advc.003y
#include "CvDLLLogger.h" // advc.003t
#include "FProfiler.h"
// <advc.003x> Include only parts of the old CvInfos.h (caveat: the order of these matters)
#include "CvInfo_Base.h"
#include "CvInfo_Asset.h"
#include "CvInfo_Tech.h"
#include "CvInfo_Civilization.h"
#include "CvInfo_Organization.h"
#include "CvInfo_Symbol.h"
#include "CvInfo_RandomEvent.h"
// For inlining in CvUnit.h. It's also big and needed rather frequently.
#include "CvInfo_Unit.h" // </advc.003x>
/*  advc.make: These I had removed (not _that_ frequently included),
	but decided to add them back. */
#include "CyGlobalContext.h" // Includes CvArtFileMgr.h
#include "CyCity.h"
#include "CvDLLEntityIFaceBase.h"
// advc.make: New additions
#include "CvDLLInterfaceIFaceBase.h"
#include "CvDLLFAStarIFaceBase.h"
#include "CvDLLEngineIFaceBase.h"
#include "CvInitCore.h" // Rarely changes and useful to have for inlining
#include "CvEventReporter.h" // Includes CvStatistics.h and CvDllPythonEvents.h
#include "CyArgsList.h"
#include "CyPlot.h"
#include "CyUnit.h"

// Undefine OutputDebugString in release builds // advc.make: in non-debug builds
#ifdef _DEBUG
	/*	<advc.wine> Wrap a macro around OutputDebugString that prints to both the VS console
		(as before through WinBase.h) and to a regular console e.g. for Wine. */
	// Caveat: szMsg has to be zero-terminated -- no fixed-size char buffers!
	#define printToConsole(szMsg) \
		OutputDebugString(szMsg); \
		printf("OutputDebugString: %s", szMsg);
#else
	#define printToConsole(szMsg)
#endif // </advc.wine>

#endif	// CvGameCoreDLL_h
