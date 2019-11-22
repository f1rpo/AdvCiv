#pragma once

#ifndef FASSERT_H
#define FASSERT_H

// Only compile in FAssert's if FASSERT_ENABLE is defined.  By default, however, let's key off of
// _DEBUG.  Sometimes, however, it's useful to enable asserts in release builds, and you can do that
// simply by changing the following lines to define FASSERT_ENABLE or using project settings to override
// advc.make (comment): The Makefile and project configuration take care of this
#ifdef _DEBUG
#define FASSERT_ENABLE
#endif
#ifdef FASSERT_ENABLE
/*  advc.make: Inlining functions with assertions could be a bad choice. Let's
	at least not force-inline. */
#define __forceinline inline

#ifdef WIN32

bool FAssertDlg( const char*, const char*, const char*, unsigned int,
		/*  advc.006f (from C2C): const char* param added. And changed the
			two locations below so that __FUNCTION__ is passed. */
		const char*, bool& );

#define FAssert( expr )	\
{ \
	static bool bIgnoreAlways = false; \
	if( !bIgnoreAlways && !(expr) ) \
{ \
	if( FAssertDlg( #expr, 0, __FILE__, __LINE__, __FUNCTION__, bIgnoreAlways ) ) \
{ _asm int 3 } \
} \
}

#define FAssertMsg( expr, msg ) \
{ \
	static bool bIgnoreAlways = false; \
	if( !bIgnoreAlways && !(expr) ) \
{ \
	if( FAssertDlg( #expr, msg, __FILE__, __LINE__, __FUNCTION__, bIgnoreAlways ) ) \
{ _asm int 3 } \
} \
}

#else // Non Win32 platforms--just use built-in FAssert
#define FAssert( expr )	FAssert( expr )
#define FAssertMsg( expr, msg )	FAssert( expr )

#endif

/*  <advc.make> Building with snprintf in the K-Mod code below works fine,
	but the VS2010 editor can't handle it. Conversely, the editor can handle
	_snprintf_s (which should be equivalent apart from the return value),
	but then building fails. Therefore define '_CODE_EDITOR' through the project file
	for the editor, but not in the Makefile.
	(BtS had originally used sprintf, which doesn't handle buffer overflows well.) */
#if _MSC_VER <= 1600 // Probably no problem in more recent versions
#ifdef _CODE_EDITOR
#define snprintf _snprintf_s
#endif
#endif // </advc.make>

// K-mod. moved the following macro from CvInitCore.h to here (and modified it)
#define FASSERT_BOUNDS(lower,upper,index,fnString)\
	if (index < lower)\
	{\
		char acOut[256];\
		snprintf(acOut, 256, "Index in %s expected to be >= %d. (value: %d)", fnString, lower, index);\
		FAssertMsg(index >= lower, acOut);\
	}\
	else if (index >= upper)\
	{\
		char acOut[256];\
		snprintf(acOut, 256, "Index in %s expected to be < %d. (value: %d)", fnString, upper, index);\
		FAssertMsg(index < upper, acOut);\
	}
// K-Mod end
#else
// FASSERT_ENABLE not defined		advc.006c: void(0) added
#define FAssert( expr ) (void)0
#define FAssertMsg( expr, msg ) (void)0
// K-Mod:
#define FASSERT_BOUNDS(lower,upper,index,fnString) (void)0

#endif

#endif // FASSERT_H
