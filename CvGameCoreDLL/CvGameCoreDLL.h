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
/*	<advc.make> That's Windows 2000, which is what Civ 4 requires anyway
	(according to the publisher's website). Enables some more macros;
	who knows, maybe also more optimized code. */
#define _WIN32_WINNT 0x0500
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

/*	advc (tbd.): Put all this primitive type stuff into a separate header,
	perhaps along with the contents of TypeChoice.h (advc.fract). */
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
/*inline DWORD FtoDW( float f ) { return *(DWORD*)&f; }
inline float DWtoF( dword n ) { return *(float*)&n; }
inline float MaxFloat() { return DWtoF(0x7f7fffff); }*/

/*	advc: Get the limits of an integer type of at most 4 bytes length
	(bool doesn't count either) -- since we don't have SIZE_MIN/MAX (cstdint),
	nor boost::integer_traits<T>::const_max.
	We do have std::numeric_limits<T>::min and max, but those are functions,
	so they can't e.g. be used in static assertions. std::numeric_limits<T>::is_signed
	is a constant, but let's nevertheless take care of that here too, so that
	integer_limits and numeric_limits won't have to be used side by side. */
template<typename T>
struct integer_limits;
template<>
struct integer_limits<char>
{
	static char const min = MIN_CHAR;
	static char const max = MAX_CHAR;
	static bool const is_signed = true;
};
template<>
struct integer_limits<byte>
{
	static byte const min = MIN_UNSIGNED_CHAR;
	static byte const max = MAX_UNSIGNED_CHAR;
	static bool const is_signed = false;
};
template<>
struct integer_limits<short>
{
	static short const min = MIN_SHORT;
	static short const max = MAX_SHORT;
	static bool const is_signed = true;
};
template<>
struct integer_limits<word>
{
	static word const min = MIN_UNSIGNED_SHORT;
	static word const max = MAX_UNSIGNED_SHORT;
	static bool const is_signed = false;
};
template<>
struct integer_limits<int>
{
	static int const min = MIN_INT;
	static int const max = MAX_INT;
	static bool const is_signed = true;
};
template<>
struct integer_limits<uint>
{
	static uint const min = MIN_UNSIGNED_INT;
	static uint const max = MAX_UNSIGNED_INT;
	static bool const is_signed = false;
};

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

//jason tests (advc.make: removed most of those)
#include "CvRandom.h"
#include "FProfiler.h"
#include "CvGameCoreUtils.h"
#include "ScaledNum.h" // Includes TypeChoice.h
#include "CvGlobals.h"
#include "EnumMap2D.h" // advc.enum: Includes EnumMap.h
#include "CvPythonCaller.h" // advc.003y
#include "CvDLLLogger.h" // advc.003t
// <advc.003x> Include only parts of the old CvInfos.h (caveat: the order of these matters)
#include "CvInfo_Base.h" // advc.enum: Includes CvInfo_EnumMap.h
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
