#pragma once

#ifndef FASSERT_H
#define FASSERT_H

// Only compile in FAssert's if FASSERT_ENABLE is defined.  By default, however, let's key off of
// _DEBUG.  Sometimes, however, it's useful to enable asserts in release builds, and you can do that
// simply by changing the following lines to define FASSERT_ENABLE or using project settings to override
// advc.test: No change currently
#ifdef _DEBUG
#define FASSERT_ENABLE
#endif

#ifdef FASSERT_ENABLE

#ifdef WIN32

bool FAssertDlg( const char*, const char*, const char*, unsigned int, bool& );

#define FAssert( expr )	\
{ \
	static bool bIgnoreAlways = false; \
	if( !bIgnoreAlways && !(expr) ) \
{ \
	if( FAssertDlg( #expr, 0, __FILE__, __LINE__, bIgnoreAlways ) ) \
{ _asm int 3 } \
} \
}

#define FAssertMsg( expr, msg ) \
{ \
	static bool bIgnoreAlways = false; \
	if( !bIgnoreAlways && !(expr) ) \
{ \
	if( FAssertDlg( #expr, msg, __FILE__, __LINE__, bIgnoreAlways ) ) \
{ _asm int 3 } \
} \
}

#else
// Non Win32 platforms--just use built-in FAssert
#define FAssert( expr )	FAssert( expr )
#define FAssertMsg( expr, msg )	FAssert( expr )

#endif

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
// FASSERT_ENABLE not defined
#define FAssert( expr )
#define FAssertMsg( expr, msg )
#define FASSERT_BOUNDS(lower,upper,index,fnString) // K-Mod

#endif

#endif // FASSERT_H
