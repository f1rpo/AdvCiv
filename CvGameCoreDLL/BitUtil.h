#pragma once

#ifndef BIT_UTIL_H
#define BIT_UTIL_H

/*  advc.enum: Helper functions for bitwise operations. Copied from the
	"We the People" mod (all code by Nightinggale), which keeps these functions
	directly in CvGameCoreDLL.h. */

namespace BitUtil // advc: Don't want these in the global namespace (for now)
{
	template <typename T>
	inline bool HasBit(const T x, const int y)
	{
		return (x & ((T)1U << y)) != 0;
	}

	template <typename T>
	inline T SetBit(T &x, const int y)
	{
		return x = (T)(x | ((T)1U << y));
	}

	// use one of the previous functions instead of hardcoding bValue
	// this function is only for cases where a bit can be turned both on and off
	template <typename T>
	inline T SetBit(T &x, const int y, const bool bValue)
	{
		if (bValue)
		{
			return x = (T)(x | ((T)1U << y));
		}
		else
		{
			return x = (T)(x & ~((T)1U << y));
		}
	}
// advc: The rest are unused for now
#if 0
	template <typename T>
	inline T ClrBit(T &x, const int y)
	{
		return x = (T)(x & ~((T)1U << y));
	}

	// use both has and get prefix as both fits and remembering both appears to be a pain
	template <typename T>
	inline T GetBits(T &x, const int iIndex, const int iNumBits)
	{
		return (x >> iIndex) & (((T)1u << iNumBits) - 1);
	}

	template <typename T>
	inline T HasBits(T &x, const int iIndex, const int iNumBits)
	{
		return GetBits(x, iIndex, iNumBits);
	}

	template <typename T>
	inline T SetBits(T &x, const int iIndex, const int iNumBits, const T iValue)
	{
		x &= ~(((1 << iNumBits) - 1) << iIndex);
		x |= (iValue & ((1 << iNumBits) - 1)) << iIndex;
		return x;
	}
#endif
};

// advc.enum (not from WtP): Macro for (somewhat) type-safe bitmasks
#define OVERLOAD_BITWISE_OPERATORS(EnumType) \
	__forceinline EnumType operator|(EnumType eLeft, EnumType eRight) \
	{ \
		int iLeft = eLeft, iRight = eRight; \
		return static_cast<EnumType>(iLeft | iRight); \
	} \
	__forceinline EnumType operator|=(EnumType& eLeft, EnumType eRight) \
	{ \
		return eLeft = (eLeft | eRight); \
	} \
	__forceinline EnumType operator&(EnumType eLeft, EnumType eRight) \
	{ \
		int iLeft = eLeft, iRight = eRight; \
		return static_cast<EnumType>(iLeft & iRight); \
	} \
	__forceinline EnumType operator&=(EnumType& eLeft, EnumType eRight) \
	{ \
		return eLeft = (eLeft & eRight); \
	} \
	__forceinline EnumType operator^(EnumType eLeft, EnumType eRight) \
	{ \
		int iLeft = eLeft, iRight = eRight; \
		return static_cast<EnumType>(iLeft ^ iRight); \
	} \
	__forceinline EnumType operator^=(EnumType& eLeft, EnumType eRight) \
	{ \
		return eLeft = (eLeft ^ eRight); \
	} \
	__forceinline EnumType operator~(EnumType e) \
	{ \
		int i = e; \
		return static_cast<EnumType>(~i); \
	}

#endif
