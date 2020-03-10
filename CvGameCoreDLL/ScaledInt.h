#pragma once

#ifndef SCALED_INT_H
#define SCALED_INT_H

/*	advc.fract: Header-only class template for fixed-point fractional numbers.
	Working -- but still work in progress. */

#include "FixedPointPowTables.h" // Large lookup table, but ScaledInt.h gets precompiled.
#include "TypeChoice.h"
/*	Other non-BtS dependencies: ROUND_DIVIDE and round in CvGameCoreUtils.h.
	(Tbd.: Move those global functions here.)
	For inclusion in PCH, one may have to define NOMINMAX before including windows.h;
	see CvGameCoreDLL.h.
	May want to define __forceinline as inline if FASSERT_ENABLE is defined;
	see FAssert.h.
	The bernoulliSuccess function assumes that CvRandom can include two integer values
	in its log messages; see comment in bernoulliSuccess. */

// For members shared by all instantiations of ScaledInt
template<typename Dummy> // Just so that static data members can be defined in the header
class ScaledIntBase
{
protected:
	static CvString szBuf;
	/*	(Could also be global, but sizeof(OtherIntType) <= 4 shouldn't be assumed
		in other contexts.) */
	template<typename OtherIntType>
	static __forceinline int safeToInt(OtherIntType n)
	{
		BOOST_STATIC_ASSERT(sizeof(OtherIntType) <= 4);
		// uint is the only problematic OtherIntType
		if (!std::numeric_limits<OtherIntType>::is_signed &&
			sizeof(int) == sizeof(OtherIntType))
		{
			FAssert(n <= static_cast<OtherIntType>(MAX_INT));
		}
		return static_cast<int>(n);
	}
};
template<typename Dummy>
CvString ScaledIntBase<Dummy>::szBuf = "";

/*	See comment below about EnumType parameter of ScaledInt. This assertion
	is a pretty crude way to implement that. */
#define STATIC_ASSERT_COMPATIBLE(EnumType1,EnumType2) \
	BOOST_STATIC_ASSERT( \
			(is_same_type<EnumType1,EnumType2>::value) || \
			(is_same_type<EnumType1,int>::value) || \
			(is_same_type<int,EnumType2>::value));

/*  class ScaledInt: Approximates a fractional number as an integer multiplied by a
	scale factor. For fixed-point arithmetic that can't lead to network sync issues.
	Performance: Generally comparable to floating-point types when the scale factor is
	a power of 2; see ScaledIntTest.cpp.
	Overloads commonly used arithmetic operators and offers some conveniences that the
	built-in types don't have, e.g. abs, clamp, approxEquals, bernoulliSuccess (coin flip).
	Compile-time converter from double: macro 'fixp'
	Conversion from percentage: macro 'per100' (also 'per1000', 'per10000')
	'scaled' and 'uscaled' typedefs for default precision.

	In code that uses Hungarian notation, I propose the prefix 'r' for
	ScaledInt variables, or more generally for any types that represent
	rational numbers without a floating point.

	The difference between ScaledInt and boost::rational is that the latter allows
	the denominator to change at runtime, which allows for greater accuracy but
	isn't as fast. */

/*  iSCALE is the factor by which integer numbers are multiplied when converted
	to a ScaledInt (see constructor from int) and thus determines the precision
	of fractional numbers and affects the numeric limits (MAX, MIN) - the higher
	iSCALE, the greater the precision and the tighter the limits.

	IntType is the type of the underlying integer variable. Has to be an integral type.
	__int64 isn't currently supported. For unsigned IntType, internal integer
	divisions are rounded to the nearest IntType value in order to improve precision.
	For signed INT types, this isn't guaranteed. (Known issue: The unsigned rounding
	operations can lead to overflow.) Using an unsigned IntType also speeds up
	multiplication.

	ScaledInt instances of different iSCALE values or different IntTypes can be mixed.
	Multiplications, divisions and comparisons on differing scales will internally scale
	both operands up to the product of the two scales (using 64 bit if necessary) in order
	to minimize rounding errors in multiplication and division and to make comparisons exact.
	The return type of the arithmetic operators is a ScaledInt with iSCALE equal to the
	maximum of the two scales and IntType equal to the common type of the two integer types
	(see choose_int_type in TypeChoice.h; should be equivalent to std::common_type in C++11).

	EnumType (optional): If an enum type is given, the resulting ScaledInt type will
	be incompatible with ScaledInt types that use a different enum type.
	See usage example (MovementPts) in ScaledIntTest.
	Tbd.: The compiler errors from STATIC_ASSERT_COMPATIBLE are difficult to read.
	Is there some way to define the respective function templates only for arguments
	with compatible enum types? Or else, using something like
	{ EnumType eDummy = (OtherEnumType)0; }
	instead of BOOST_STATIC_ASSERT would already help a bit - but that would fail when
	EnumType is int and OtherEnumType isn't.

	Also tbd. (in addition to "tbd." and "fixme" comments throughout this file):
	- Move large function definitions out of the class definition; specialize.
	- Add logarithm function.
	- Add Natvis file.
	- Test whether the pragma pack is a good idea.
	For background, see the replies and "To be done" in the initial post here:
	forums.civfanatics.com/threads/class-for-fixed-point-arithmetic.655037  */
#pragma pack(push, 1)
template<int iSCALE, typename IntType = int, typename EnumType = int>
class ScaledInt : ScaledIntBase<void> // Tbd.: Rename to ScaledNum. What's being scaled isn't necessarily an integer.
{
	BOOST_STATIC_ASSERT(sizeof(IntType) <= 4);
	/*	Workaround for MSVC bug with dependent template argument in friend declaration:
		Make the scale parameter an int but cast it to IntType internally. This way,
		iSCALE can also precede IntType in the parameter list. */
	static IntType const SCALE = static_cast<IntType>(iSCALE);
	template<int iOTHER_SCALE, typename OtherIntType, typename OtherEnumType>
	friend class ScaledInt;

	static bool const bSIGNED = std::numeric_limits<IntType>::is_signed;
	/*	Limits of IntType. Set through std::numeric_limits, but can't do that in-line; and
		we don't have SIZE_MIN/MAX (cstdint), nor boost::integer_traits<IntType>::const_max.
		Therefore, can't use INTMIN, INTMAX in static assertions. */
	static IntType const INTMIN;
	static IntType const INTMAX;

	IntType m_i;

public:
	static IntType MAX() { return INTMAX / SCALE; }
	static IntType MIN() { return INTMIN / SCALE; }

	/*	Factory function for creating fractions (with wrapper macros per100).
		Numerator and denominator as template parameters ensure
		that the conversion to SCALE happens at compile time, so that
		floating-point math can be used for maximal accuracy. */
	template<int iNUM, int iDEN>
	static inline ScaledInt fromRational()
	{
		BOOST_STATIC_ASSERT(bSIGNED || (iDEN >= 0 && iNUM >= 0));
		return fromDouble(iNUM / static_cast<double>(iDEN));
	}

	__forceinline static ScaledInt max(ScaledInt r1, ScaledInt r2)
	{
		return std::max(r1, r2);
	}
	__forceinline static ScaledInt min(ScaledInt r1, ScaledInt r2)
	{
		return std::min(r1, r2);
	}
	template<typename LoType, typename HiType>
	static __forceinline ScaledInt clamp(ScaledInt r, LoType lo, HiType hi)
	{
		r.clamp(lo, hi);
		return r;
	}

	__forceinline ScaledInt() : m_i(static_cast<IntType>(0)) {}
	__forceinline ScaledInt(int i) : m_i(static_cast<IntType>(SCALE * i))
	{
		// (Tbd.: Not sure if this assertion should be kept permanently)
		FAssert(static_cast<IntType>(i) >= INTMIN / SCALE);
		FAssert(static_cast<IntType>(i) <= INTMAX / SCALE);
	}
	__forceinline ScaledInt(uint u) : m_i(static_cast<IntType>(SCALE * u))
	{
		FAssert(u <= INTMAX / SCALE);
	}
	// Construction from rational
	__forceinline ScaledInt(int iNum, int iDen)
	{
		m_i = safeCast(mulDiv(SCALE, iNum, iDen));
	}

	// Scale and integer type conversion constructor
	template<int iFROM_SCALE, typename OtherIntType, typename OtherEnumType>
	__forceinline ScaledInt(ScaledInt<iFROM_SCALE,OtherIntType,OtherEnumType> rOther)
	{
		STATIC_ASSERT_COMPATIBLE(EnumType,OtherEnumType);
		static OtherIntType const FROM_SCALE = ScaledInt<iFROM_SCALE,OtherIntType,OtherEnumType>::SCALE;
		if (FROM_SCALE == SCALE)
			m_i = safeCast(rOther.m_i);
		else
		{
			m_i = safeCast(ScaledInt<iFROM_SCALE,OtherIntType,OtherEnumType>::
					mulDivByScale(rOther.m_i, SCALE));
		}
	}

	/*	Explicit conversion to default EnumType
		(can't overload explicit cast operator in C++03) */
	__forceinline ScaledInt<iSCALE,IntType> convert() const
	{
		ScaledInt<iSCALE,IntType> r;
		r.m_i = m_i;
		return r;
	}

	__forceinline int getInt() const
	{
		// Conversion to int shouldn't be extremely frequent; take the time to round.
		return round();
	}
	int round() const
	{
		if (INTMAX < SCALE) // Wish I could BOOST_STATIC_ASSERT this
			FAssert(false);
		if (bSIGNED)
		{
			FAssert(m_i > 0 ?
					m_i <= static_cast<IntType>(INTMAX - SCALE / 2) :
					m_i >= static_cast<IntType>(INTMIN + SCALE / 2));
			return (m_i + SCALE / static_cast<IntType>(m_i >= 0 ? 2 : -2)) / SCALE;
		}
		// Tbd.: Use additional bits when a high SCALE makes overflow likely
		FAssert(m_i <= static_cast<IntType>(INTMAX - SCALE / 2u));
		FAssert(m_i >= static_cast<IntType>(INTMIN + SCALE / 2u));
		return (m_i + SCALE / 2u) / SCALE;
	}
	__forceinline int floor() const
	{
		return static_cast<int>(m_i / SCALE);
	}
	int ceil() const
	{
		int r = floor();
		return r + ((m_i >= 0 && m_i - r * SCALE > 0) ? 1 : 0);
	}
	// Cast operator - better require explicit calls to getInt.
	/*__forceinline operator int() const
	{
		return getInt();
	}*/
	bool isInt() const
	{
		return (m_i % SCALE == 0);
	}

	__forceinline int getPercent() const
	{
		return safeToInt(mulDivRound(m_i, 100, SCALE));
	}
	__forceinline int getPermille() const
	{
		return safeToInt(mulDivRound(m_i, 1000, SCALE));
	}
	__forceinline int roundToMultiple(int iMultiple) const
	{
		return mulDivRound(m_i, 1, SCALE * iMultiple) * iMultiple;
	}
	__forceinline double getDouble() const
	{
		return m_i / static_cast<double>(SCALE);
	}
	__forceinline float getFloat() const
	{
		return m_i / static_cast<float>(SCALE);
	}
	CvString const& str(int iDen = iSCALE)
	{
		if (iDen == 1)
			szBuf.Format("%s%d", isInt() ? "" : "ca. ", round());
		else if (iDen == 100)
			szBuf.Format("%d percent", getPercent());
		else if (iDen == 1000)
			szBuf.Format("%d permille", getPermille());
		else szBuf.Format("%d/%d", safeToInt(mulDivByScale(m_i, iDen)), iDen);
		return szBuf;
	}

	void write(FDataStreamBase* pStream) const
	{
		pStream->Write(m_i);
	}
	void read(FDataStreamBase* pStream)
	{
		pStream->Read(&m_i);
	}

	__forceinline void mulDiv(int iMultiplier, int iDivisor)
	{
		m_i = safeCast(mulDiv(m_i, iMultiplier, iDivisor));
	}

	// Bernoulli trial (coin flip) with success probability equal to m_i/SCALE
	bool bernoulliSuccess(CvRandom& kRand, char const* szLog,
		int iLogData1 = -1, int iLogData2 = -1) const
	{
		// Guards for better performance and to avoid unnecessary log output
		if (m_i <= 0)
			return false;
		if (m_i >= SCALE)
			return true;
		BOOST_STATIC_ASSERT(iSCALE <= USHRT_MAX);
		/*	When porting ScaledInt to another mod, you may want to use:
			return (kRand.get(static_cast<unsigned short>(SCALE), szLog) < m_i); */
		return (kRand.getInt(static_cast<unsigned short>(SCALE),
				szLog, iLogData1, iLogData2) < m_i);
	}

	ScaledInt pow(int iExp) const
	{
		if (iExp < 0)
			return 1 / powNonNegative(-iExp);
		return powNonNegative(iExp);
	}
	ScaledInt pow(ScaledInt rExp) const
	{
		FAssert(!isNegative());
		if (rExp.bSIGNED && rExp.isNegative())
			return 1 / powNonNegative(-rExp);
		return powNonNegative(rExp);
	}
	__forceinline ScaledInt sqrt() const
	{
		FAssert(!isNegative());
		return powNonNegative(fromRational<1,2>());
	}
	__forceinline ScaledInt exponentiate(ScaledInt rExp)
	{
		*this = pow(rExp);
	}

	__forceinline ScaledInt abs() const
	{
		ScaledInt r;
		r.m_i = std::abs(m_i);
		return r;
	}

	template<typename LoType, typename HiType>
	__forceinline void clamp(LoType lo, HiType hi)
	{
		FAssert(lo <= hi);
		increaseTo(lo);
		decreaseTo(hi);
	}
	template<typename LoType>
	__forceinline void increaseTo(LoType lo)
	{
		// (std::max doesn't allow differing types)
		if (*this < lo)
			*this = lo;
	}
	template<typename HiType>
	__forceinline void decreaseTo(HiType hi)
	{
		if (*this > hi)
			*this = hi;
	}
	// Too easy to use these by accident instead of the non-const functions above
	/*template<typename LoType>
	__forceinline ScaledInt increasedTo(LoType lo) const
	{
		ScaledInt rCopy(*this);
		rCopy.increaseTo(lo);
		return rCopy;
	}
	template<typename HiType>
	__forceinline ScaledInt decreasedTo(HiType hi) const
	{
		ScaledInt rCopy(*this);
		rCopy.decreaseTo(hi);
		return rCopy;
	}*/

	template<typename NumType, typename Epsilon>
	__forceinline bool approxEquals(NumType num, Epsilon e) const
	{
		// Can't be allowed for floating point types; will have to use fixp to wrap.
		BOOST_STATIC_ASSERT(!std::numeric_limits<NumType>::has_infinity);
		BOOST_STATIC_ASSERT(!std::numeric_limits<Epsilon>::has_infinity);
		return ((*this - num).abs() <= e);
	}

	__forceinline bool isPositive() const { return (m_i > 0); }
	__forceinline bool isNegative() const { return (bSIGNED && m_i < 0); }

	__forceinline ScaledInt operator-() { return ScaledInt(-m_i); }

	template<int iOTHER_SCALE, typename OtherIntType, typename OtherEnumType>
	__forceinline bool operator<(ScaledInt<iOTHER_SCALE,OtherIntType,OtherEnumType> rOther) const
	{
		STATIC_ASSERT_COMPATIBLE(EnumType,OtherEnumType);
		if (iOTHER_SCALE == iSCALE)
			return (m_i < rOther.m_i);
		return (ScaledInt<iOTHER_SCALE,OtherIntType,OtherEnumType>::scaleForComparison(m_i) <
				scaleForComparison(rOther.m_i));
	}
	template<int iOTHER_SCALE, typename OtherIntType, typename OtherEnumType>
	__forceinline bool operator>(ScaledInt<iOTHER_SCALE,OtherIntType,OtherEnumType> rOther) const
	{
		STATIC_ASSERT_COMPATIBLE(EnumType,OtherEnumType);
		if (iOTHER_SCALE == iSCALE)
			return (m_i > rOther.m_i);
		return (ScaledInt<iOTHER_SCALE,OtherIntType,OtherEnumType>::scaleForComparison(m_i) >
				scaleForComparison(rOther.m_i));
	}
	template<int iOTHER_SCALE, typename OtherIntType, typename OtherEnumType>
	__forceinline bool operator==(ScaledInt<iOTHER_SCALE,OtherIntType,OtherEnumType> rOther) const
	{
		STATIC_ASSERT_COMPATIBLE(EnumType,OtherEnumType);
		if (iOTHER_SCALE == iSCALE)
			return (m_i == rOther.m_i);
		return (ScaledInt<iOTHER_SCALE,OtherIntType,OtherEnumType>::scaleForComparison(m_i) ==
				scaleForComparison(rOther.m_i));
	}
	template<int iOTHER_SCALE, typename OtherIntType, typename OtherEnumType>
	__forceinline bool operator!=(ScaledInt<iOTHER_SCALE,OtherIntType,OtherEnumType> rOther) const
	{
		STATIC_ASSERT_COMPATIBLE(EnumType,OtherEnumType);
		if (iOTHER_SCALE == iSCALE)
			return (m_i != rOther.m_i);
		return (ScaledInt<iOTHER_SCALE,OtherIntType,OtherEnumType>::scaleForComparison(m_i) !=
				scaleForComparison(rOther.m_i));
	}
	template<int iOTHER_SCALE, typename OtherIntType, typename OtherEnumType>
	__forceinline bool operator<=(ScaledInt<iOTHER_SCALE,OtherIntType,OtherEnumType> rOther) const
	{
		STATIC_ASSERT_COMPATIBLE(EnumType,OtherEnumType);
		if (iOTHER_SCALE == iSCALE)
			return (m_i <= rOther.m_i);
		return (ScaledInt<iOTHER_SCALE,OtherIntType,OtherEnumType>::scaleForComparison(m_i) <=
				scaleForComparison(rOther.m_i));
	}
	template<int iOTHER_SCALE, typename OtherIntType, typename OtherEnumType>
	__forceinline bool operator>=(ScaledInt<iOTHER_SCALE,OtherIntType,OtherEnumType> rOther) const
	{
		STATIC_ASSERT_COMPATIBLE(EnumType,OtherEnumType);
		if (iOTHER_SCALE == iSCALE)
			return (m_i >= rOther.m_i);
		return (ScaledInt<iOTHER_SCALE,OtherIntType,OtherEnumType>::scaleForComparison(m_i) >=
				scaleForComparison(rOther.m_i));
	}

	// Exact comparisons with int - to be consistent with int-float comparisons.
	__forceinline bool operator<(int i) const
	{
		return (m_i < scaleForComparison(i));
	}
    __forceinline bool operator>(int i) const
	{
		return (m_i > scaleForComparison(i));
	}
    __forceinline bool operator==(int i) const
	{
		return (m_i == scaleForComparison(i));
	}
	__forceinline bool operator!=(int i) const
	{
		return (m_i != scaleForComparison(i));
	}
	__forceinline bool operator<=(int i) const
	{
		return (m_i <= scaleForComparison(i));
	}
    __forceinline bool operator>=(int i) const
	{
		return (m_i >= scaleForComparison(i));
	}
	__forceinline bool operator<(uint i) const
	{
		return (m_i < scaleForComparison(u));
	}
    __forceinline bool operator>(uint u) const
	{
		return (m_i > scaleForComparison(u));
	}
    __forceinline bool operator==(uint u) const
	{
		return (m_i == scaleForComparison(u));
	}
	__forceinline bool operator!=(uint u) const
	{
		return (m_i != scaleForComparison(i));
	}
	__forceinline bool operator<=(uint u) const
	{
		return (m_i <= scaleForComparison(u));
	}
    __forceinline bool operator>=(uint u) const
	{
		return (m_i >= scaleForComparison(u));
	}

	/*	Can't guarantee here that only const expressions are used.
		So floating-point operands will have to be wrapped in fixp. */
	/*__forceinline bool operator<(double d) const
	{
		return (getDouble() < d);
	}
    __forceinline bool operator>(double d) const
	{
		return (getDouble() > d);
	}*/

	// Operand on different scale: Let ctor implicitly convert it to ScaledInt
	__forceinline ScaledInt& operator+=(ScaledInt rOther)
	{
		// Maybe uncomment this for some special occasion
		/*FAssert(rOther <= 0 || m_i <= INTMAX - rOther.m_i);
		FAssert(rOther >= 0 || m_i >= INTMIN + rOther.m_i);*/
		m_i += rOther.m_i;
		return *this;
	}
	__forceinline ScaledInt& operator-=(ScaledInt rOther)
	{
		/*FAssert(rOther >= 0 || m_i <= INTMAX + rOther.m_i);
		FAssert(rOther <= 0 || m_i >= INTMIN - rOther.m_i);*/
		m_i -= rOther.m_i;
		return *this;
	}

	template<int iOTHER_SCALE, typename OtherIntType, typename OtherEnumType>
	__forceinline ScaledInt& operator*=(ScaledInt<iOTHER_SCALE,OtherIntType,OtherEnumType> rOther)
	{
		STATIC_ASSERT_COMPATIBLE(EnumType,OtherEnumType);
		m_i = safeCast(ScaledInt<iOTHER_SCALE,OtherIntType,OtherEnumType>::mulDivByScale(m_i, rOther.m_i));
		return *this;
	}
	template<int iOTHER_SCALE, typename OtherIntType, typename OtherEnumType>
	__forceinline ScaledInt& operator/=(ScaledInt<iOTHER_SCALE,OtherIntType,OtherEnumType> rOther)
	{
		STATIC_ASSERT_COMPATIBLE(EnumType,OtherEnumType);
		m_i = safeCast(mulDiv(m_i, ScaledInt<iOTHER_SCALE,OtherIntType,OtherEnumType>::SCALE, rOther.m_i));
		return *this;
	}

	__forceinline ScaledInt& operator++()
	{
		(*this) += 1;
		return *this;
	}
	__forceinline ScaledInt& operator--()
	{
		(*this) -= 1;
		return *this;
	}
	__forceinline ScaledInt operator++(int)
	{
		ScaledInt rCopy(*this);
		(*this) += 1;
		return rCopy;
	}
	__forceinline ScaledInt operator--(int)
	{
		ScaledInt rCopy(*this);
		(*this) -= 1;
		return rCopy;
	}

	__forceinline ScaledInt& operator+=(int i)
	{
		(*this) += ScaledInt(i);
		return (*this);
	}
	__forceinline ScaledInt& operator-=(int i)
	{
		(*this) -= ScaledInt(i);
		return (*this);
	}
	__forceinline ScaledInt& operator*=(int i)
	{
		(*this) *= ScaledInt(i);
		return (*this);
	}
	__forceinline ScaledInt& operator/=(int i)
	{
		(*this) /= ScaledInt(i);
		return (*this);
	}
	__forceinline ScaledInt& operator+=(uint u)
	{
		(*this) += ScaledInt(u);
		return (*this);
	}
	__forceinline ScaledInt& operator-=(uint u)
	{
		(*this) -= ScaledInt(u);
		return (*this);
	}
	__forceinline ScaledInt& operator*=(uint u)
	{
		(*this) *= ScaledInt(u);
		return (*this);
	}
	__forceinline ScaledInt& operator/=(uint u)
	{
		(*this) /= ScaledInt(u);
		return (*this);
	}

	__forceinline ScaledInt operator+(int i) const
	{
		ScaledInt rCopy(*this);
		rCopy += i;
		return rCopy;
	}
	__forceinline ScaledInt operator-(int i) const
	{
		ScaledInt rCopy(*this);
		rCopy -= i;
		return rCopy;
	}
	__forceinline ScaledInt operator*(int i) const
	{
		ScaledInt rCopy(*this);
		rCopy *= i;
		return rCopy;
	}
	__forceinline ScaledInt operator/(int i) const
	{
		ScaledInt rCopy(*this);
		rCopy /= i;
		return rCopy;
	}
	__forceinline ScaledInt operator+(uint u) const
	{
		ScaledInt rCopy(*this);
		rCopy += u;
		return rCopy;
	}
	__forceinline ScaledInt operator-(uint u) const
	{
		ScaledInt rCopy(*this);
		rCopy -= u;
		return rCopy;
	}
	__forceinline ScaledInt operator*(uint u) const
	{
		ScaledInt rCopy(*this);
		rCopy *= u;
		return rCopy;
	}
	__forceinline ScaledInt operator/(uint u) const
	{
		ScaledInt rCopy(*this);
		rCopy /= u;
		return rCopy;
	}

private:
	/*	Doesn't depend on any ScaledInt template param. Make it global? Would have to be in
		a namespace in order to avoid confusion with MulDiv (WinBase.h). */
	template<typename MultiplicandType, typename MultiplierType, typename DivisorType>
	static __forceinline
	typename choose_int_type
		< typename choose_int_type<MultiplicandType,MultiplierType>, DivisorType >::type
	mulDiv(MultiplicandType multiplicand, MultiplierType multiplier, DivisorType divisor)
	{
		typedef typename choose_int_type
				< typename choose_int_type<MultiplicandType,MultiplierType>, DivisorType >::type
				ReturnType;
		BOOST_STATIC_ASSERT(sizeof(MultiplierType) <= 4);
		BOOST_STATIC_ASSERT(sizeof(DivisorType) <= 4);
		if (bSIGNED)
		{
			int i;
			if (sizeof(MultiplierType) == 4 || sizeof(MultiplicandType) == 4)
			{
				/*	For multiplying signed int, MulDiv (WinBase.h) is fastest.
					NB: rounds to nearest. */
				i = MulDiv(static_cast<int>(multiplicand),
						static_cast<int>(multiplier),
						static_cast<int>(divisor));
			}
			else
			{
				// For smaller signed types, int can't overflow.
				i = multiplicand;
				i *= multiplier;
				/*	Rounding to nearest here would add a branch instruction.
					To force rounding, call mulDivRound. */
				i /= divisor;
			}
			return static_cast<ReturnType>(i);
		}
		else
		{
			typedef typename choose_type<
					(sizeof(MultiplicandType) >= 4 || sizeof(MultiplierType) >= 4),
					unsigned __int64, unsigned int>::type ProductType;
			ProductType n = multiplicand;
			n *= multiplier;
			/*	Rounding to nearest is almost free
				but (fixme): can overflow */
			n += divisor / 2u;
			n /= divisor;
			return static_cast<ReturnType>(n);
		}
	}

	template<typename MultiplicandType, typename MultiplierType>
	static __forceinline
	typename choose_int_type
		< typename choose_int_type<IntType,MultiplierType>, MultiplicandType >::type
	mulDivByScale(MultiplicandType multiplicand, MultiplierType multiplier)
	{
		/*	For now, forwarding is sufficient. Tbd.: Try using SSE2 intrinsics
			when SCALE is a power of 2, i.e. when SCALE & (SCALE - 1) == 0.
			(Wouldn't want to check this when the divisor isn't known at compile time.) */
		return mulDiv(multiplicand, multiplier, SCALE);
	}

	template<typename MultiplierType, typename DivisorType>
	static __forceinline
	typename choose_int_type
		< typename choose_int_type<IntType,MultiplierType>, DivisorType >::type
	mulDivRound(IntType multiplicand, MultiplierType multiplier, DivisorType divisor)
	{
		typedef typename choose_int_type
				< typename choose_int_type<IntType,MultiplierType>, DivisorType >::type
				ReturnType;
		BOOST_STATIC_ASSERT(sizeof(MultiplierType) <= 4);
		BOOST_STATIC_ASSERT(sizeof(DivisorType) <= 4);	
		if (bSIGNED && sizeof(MultiplierType) < 4 && sizeof(IntType) < 4)
		{
			int i = multiplicand;
			i *= multiplier;
			i = ROUND_DIVIDE(i, divisor);
			i /= divisor;
			return static_cast<ReturnType>(i);
		} // In all other cases, mulDiv rounds too.
		return mulDiv(multiplicand, multiplier, divisor);
	}

	template<typename OtherIntType>
	static __forceinline IntType safeCast(OtherIntType n)
	{
		if (std::numeric_limits<OtherIntType>::is_signed != bSIGNED ||
			sizeof(IntType) < sizeof(OtherIntType))
		{
			if (!bSIGNED && std::numeric_limits<OtherIntType>::is_signed)
				FAssert(n >= 0);
			if (sizeof(IntType) < sizeof(OtherIntType) ||
				(sizeof(IntType) == sizeof(OtherIntType) &&
				bSIGNED && !std::numeric_limits<OtherIntType>::is_signed))
			{
				/*	(No static_cast b/c it needs to compile even when IntType is bigger
					than OtherIntType, i.e. when the conditions above are false.
					Tbd.: Solve this problem through choose_type and specialization?) */
				FAssert(n <= (OtherIntType)MAXINT);
				if (bSIGNED && std::numeric_limits<OtherIntType>::is_signed)
					FAssert(n >= (OtherIntType)MININT);
			}
		}
		return static_cast<IntType>(n);
	}

	ScaledInt powNonNegative(int iExp) const
	{
		ScaledInt rCopy(*this);
		/*  This can be done faster in general by squaring.
			However, I doubt that it would be faster for
			the small exponents I'm expecting to deal with. */
		ScaledInt r = 1;
		for (int i = 0; i < iExp; i++)
			r *= rCopy;
		return r;
	}
	/*	Custom algorithm.
		There is a reasonably recent paper "A Division-Free Algorithm for Fixed-Point
		Power Exponential Function in Embedded System" [sic] based on Newton's method.
		That's probably faster and more accurate, but an implementation isn't
		spelled out. Perhaps tbd. */
	ScaledInt powNonNegative(ScaledInt rExp) const
	{
		/*	Base 0 or too close to it to make a difference given the precision of the algorithm.
			Fixme: rExp could also be close to 0. Should somehow use x=y*z => b^x = (b^y)^z. */
		if (m_i < SCALE / 64)
			return 0;
		/*	Recall that: If x=y+z, then b^x=(b^y)*(b^z).
						 If b=a*c, then b^x=(a^x)*(c^x). */
		// Split rExp into the sum of an integer and a (scaled) fraction between 0 and 1
		// Running example: 5.2^2.1 at SCALE 1024, i.e. (5325/1024)^(2150/1024)
		IntType expInt = rExp.m_i / SCALE; // 2 in the example
		// Use uint in all local ScaledInt variables for more accurate rounding
		ScaledInt<128,uint> rExpFrac(rExp - expInt); // Ex.: 13/128
		/*	Factorize the base into powers of 2 and, as the last factor, the base divided
			by the product of the 2-bases. */
		ScaledInt<iSCALE,uint> rProductOfPowersOfTwo(1);
		IntType baseDiv = 1;
		// Look up approximate result of 2^rExpFrac in precomputed table
		FAssertBounds(0, 128, rExpFrac.m_i); // Tbd.: Don't keep this assert permanently
		ScaledInt<256,uint> rPowOfTwo; // Ex.: Array position [13] is 19, so rPowOfTwo=19/256
		rPowOfTwo.m_i = FixedPointPowTables::powersOfTwoNormalized_256[rExpFrac.m_i];
		++rPowOfTwo; // Denormalize (Ex.: 275/256; approximating 2^0.1)
		/*	Tbd.: Try replacing this loop with _BitScanReverse (using the /EHsc compiler flag).
			Or perhaps not available in MSVC03? See: github.com/danaj/Math-Prime-Util/pull/10/
			*/
		while (baseDiv < *this)
		{
			baseDiv *= 2;
			rProductOfPowersOfTwo *= rPowOfTwo;
		} // Ex.: baseDiv=8 and rProductOfPowersOfTwo=1270/1024, approximating (2^0.1)^3.
		ScaledInt<256,uint> rLastFactor(1);
		// Look up approximate result of ((*this)/baseDiv)^rExpFrac in precomputed table
		int iLastBaseTimes64 = (ScaledInt<64,uint>(*this / baseDiv)).m_i; // Ex.: 42/64 approximating 5.2/8
		FAssertBounds(0, 64+1, iLastBaseTimes64); // Tbd.: Don't keep this assert permanently
		if (rExpFrac.m_i != 0 && iLastBaseTimes64 != 64)
		{
			// Could be prone to cache misses :(
			rLastFactor.m_i = FixedPointPowTables::powersUnitInterval_256
					[iLastBaseTimes64-1][rExpFrac.m_i-1] + 1; // Table and values are shifted by 1
			// Ex.: Position [41][12] is 244, i.e. rLastFactor=245/256. Approximation of (5.2/8)^0.1
		}
		ScaledInt r(ScaledInt<iSCALE,uint>(pow(expInt)) *
				rProductOfPowersOfTwo * ScaledInt<iSCALE,uint>(rLastFactor));
		return r;
		/*	Ex.: First factor is 27691/1024, approximating 5.2^2,
			second factor: 1270/1024, approximating (2^0.1)^3,
			last factor: 980/1024, approximating (5.2/8)^0.1.
			Result: 32867/1024, which is ca. 32.097, whereas 5.2^2.1 is ca. 31.887. */
	}

	#define SIZE_OF_EITHER_GEQ(T1,T2,iSize) \
		/*(sizeof(T1) >= 4 || sizeof(T2) >= iSize)*/ \
		/* ^Not fit for my heroic idiom. j/k -- compiler crash. Workaround: */ \
		(sizeof(typename choose_bigger_int_type<T1,T2>::type) >= iSize)
	template<typename OtherIntType>
	static __forceinline
	typename choose_type
		< (std::numeric_limits<OtherIntType>::is_signed),
		typename choose_type
		< SIZE_OF_EITHER_GEQ(IntType,OtherIntType,4), __int64, int >::type,
		typename choose_type
		< SIZE_OF_EITHER_GEQ(IntType,OtherIntType,4), unsigned __int64, uint >::type
		>::type
	scaleForComparison(OtherIntType n)
	{
		// Tbd.: Perhaps some intrinsic function could do this faster (on the caller's side)?
		// The return type (copy-paste from above):
		typedef typename choose_type
			< (std::numeric_limits<OtherIntType>::is_signed),
			typename choose_type
			< SIZE_OF_EITHER_GEQ(IntType,OtherIntType,4), __int64, int >::type,
			typename choose_type
			< SIZE_OF_EITHER_GEQ(IntType,OtherIntType,4), unsigned __int64, uint >::type
			>::type LongType;
		LongType lNum = n;
		return lNum * SCALE;
	}
	#undef SIZE_OF_EITHER_GEQ

	static __forceinline ScaledInt fromDouble(double d)
	{
		ScaledInt r;
		r.m_i = safeCast(::round(d * SCALE));
		return r;
	}
};
#pragma pack(pop)

/*	To unclutter template parameter lists and make it easier to add more parameters.
	Un-defined at the end of the file. */
#define ScaledInt_PARAMS int iSCALE, typename IntType, typename EnumType
#define ScaledInt_T ScaledInt<iSCALE,IntType,EnumType>

template<ScaledInt_PARAMS>
IntType const ScaledInt_T::INTMAX = std::numeric_limits<IntType>::max();
		//- iSCALE / 2; // Margin for rounding? Can currently overflow unnotices; fixme.
template<ScaledInt_PARAMS>
IntType const ScaledInt_T::INTMIN = std::numeric_limits<IntType>::min();
		//+ (std::numeric_limits<IntType>::is_signed  ? iSCALE / 2 : 0);

#define COMMON_SCALED_INT \
	typename choose_type<(iLEFT_SCALE >= iRIGHT_SCALE), \
		ScaledInt< \
			iLEFT_SCALE, \
			typename choose_int_type<LeftIntType,RightIntType>::type, \
			typename choose_type<is_same_type<LeftEnumType,int>::value,RightEnumType,LeftEnumType>::type \
		>, \
		ScaledInt< \
			iRIGHT_SCALE, \
			typename choose_int_type<LeftIntType,RightIntType>::type, \
			typename choose_type<is_same_type<LeftEnumType,int>::value,RightEnumType,LeftEnumType>::type \
		> \
	>::type
/*	Simpler, but crashes the compiler (i.e. the above is a workaround).
#define COMMON_SCALED_INT \
	ScaledInt<(iLeft > iRight ? iLeft : iRight), \
	typename choose_int_type<LeftIntType,RightIntType>::type, \
	typename choose_type<is_same_type<LeftEnumType,int>::value,RightEnumType,LeftEnumType>::type > */

template<int iLEFT_SCALE,  typename LeftIntType,  typename LeftEnumType,
		 int iRIGHT_SCALE, typename RightIntType, typename RightEnumType>
__forceinline COMMON_SCALED_INT
operator+(
	ScaledInt<iLEFT_SCALE,  LeftIntType,  LeftEnumType>  rLeft,
	ScaledInt<iRIGHT_SCALE, RightIntType, RightEnumType> rRight)
{
	STATIC_ASSERT_COMPATIBLE(LeftEnumType,RightEnumType);
	/*	Note: No accuracy would be gained by scaling both operands up to
		iLEFT_SCALE*iRIGHT_SCALE before adding them. */
	COMMON_SCALED_INT r(rLeft);
	r += rRight;
	return r;
}
template<int iLEFT_SCALE,  typename LeftIntType,  typename LeftEnumType,
		 int iRIGHT_SCALE, typename RightIntType, typename RightEnumType>
__forceinline COMMON_SCALED_INT
operator-(
	ScaledInt<iLEFT_SCALE,  LeftIntType,  LeftEnumType>  rLeft,
	ScaledInt<iRIGHT_SCALE, RightIntType, RightEnumType> rRight)
{
	STATIC_ASSERT_COMPATIBLE(LeftEnumType,RightEnumType);
	COMMON_SCALED_INT r(rLeft);
	r -= rRight;
	return r;
}
template<int iLEFT_SCALE,  typename LeftIntType,  typename LeftEnumType,
		 int iRIGHT_SCALE, typename RightIntType, typename RightEnumType>
__forceinline COMMON_SCALED_INT
operator*(
	ScaledInt<iLEFT_SCALE,  LeftIntType,  LeftEnumType>  rLeft,
	ScaledInt<iRIGHT_SCALE, RightIntType, RightEnumType> rRight)
{
	STATIC_ASSERT_COMPATIBLE(LeftEnumType,RightEnumType);
	if (iLEFT_SCALE >= iRIGHT_SCALE)
	{
		COMMON_SCALED_INT r(rLeft);
		r *= rRight;
		return r;
	}
	else
	{
		COMMON_SCALED_INT r(rRight);
		r *= rLeft;
		return r;
	}
}
template<int iLEFT_SCALE,  typename LeftIntType,  typename LeftEnumType,
		 int iRIGHT_SCALE, typename RightIntType, typename RightEnumType>
__forceinline COMMON_SCALED_INT
operator/(
	ScaledInt<iLEFT_SCALE,  LeftIntType,  LeftEnumType>  rLeft,
	ScaledInt<iRIGHT_SCALE, RightIntType, RightEnumType> rRight)
{
	STATIC_ASSERT_COMPATIBLE(LeftEnumType,RightEnumType);
	if (iLEFT_SCALE >= iRIGHT_SCALE)
	{
		COMMON_SCALED_INT r(rLeft);
		r /= rRight;
		return r;
	}
	else
	{
		COMMON_SCALED_INT r(rRight);
		r /= rLeft;
		return r;
	}
}

// Commutativity ...

template<ScaledInt_PARAMS>
__forceinline ScaledInt_T operator+(int i, ScaledInt_T r)
{
	return r + i;
}
/*	As we don't implement an int cast operator, assignment to int
	should be forbidden as well. (No implicit getInt.) */
/*template<ScaledInt_PARAMS>
__forceinline int& operator+=(int& i, ScaledInt_T r)
{
	i = (r + i).getInt();
	return i;
}*/
template<ScaledInt_PARAMS>
__forceinline ScaledInt_T operator-(int i, ScaledInt_T r)
{
	return ScaledInt_T(i) - r;
}
/*template<ScaledInt_PARAMS>
__forceinline int& operator-=(int& i, ScaledInt_T r)
{
	i = (ScaledInt_T(i) - r).getInt();
	return i;
}*/
template<ScaledInt_PARAMS>
__forceinline ScaledInt_T operator*(int i, ScaledInt_T r)
{
	return r * i;
}
/*template<ScaledInt_PARAMS>
__forceinline int& operator*=(int& i, ScaledInt_T r)
{
	i = (r * i).getInt();
	return i;
}*/
template<ScaledInt_PARAMS>
__forceinline ScaledInt_T operator/(int i, ScaledInt_T r)
{
	return ScaledInt_T(i) / r;
}
/*template<ScaledInt_PARAMS>
__forceinline int& operator/=(int& i, ScaledInt_T r)
{
	i = (ScaledInt_T(i) / r).getInt();
	return i;
}*/
template<ScaledInt_PARAMS>
__forceinline ScaledInt_T operator+(uint u, ScaledInt_T r)
{
	return r + u;
}
template<ScaledInt_PARAMS>
__forceinline ScaledInt_T operator-(uint u, ScaledInt_T r)
{
	return r - u;
}
template<ScaledInt_PARAMS>
__forceinline ScaledInt_T operator*(uint ui, ScaledInt_T r)
{
	return r * ui;
}
template<ScaledInt_PARAMS>
__forceinline ScaledInt_T operator/(uint u, ScaledInt_T r)
{
	return r / u;
}
template<ScaledInt_PARAMS>
 __forceinline bool operator<(int i, ScaledInt_T r)
{
	return (r > i);
}
template<ScaledInt_PARAMS>
__forceinline bool operator>(int i, ScaledInt_T r)
{
	return (r < i);
}
template<ScaledInt_PARAMS>
 __forceinline bool operator==(int i, ScaledInt_T r)
{
	return (r == i);
}
template<ScaledInt_PARAMS>
 __forceinline bool operator!=(int i, ScaledInt_T r)
{
	return (r != i);
}
template<ScaledInt_PARAMS>
__forceinline bool operator<=(int i, ScaledInt_T r)
{
	return (r >= i);
}
template<ScaledInt_PARAMS>
 __forceinline bool operator>=(int i, ScaledInt_T r)
{
	return (r <= i);
}
template<ScaledInt_PARAMS>
__forceinline bool operator<(double d, ScaledInt_T r)
{
	return (r > d);
}
template<ScaledInt_PARAMS>
__forceinline bool operator>(double d, ScaledInt_T r)
{
	return (r < d);
}

// Scale 2048 leads to INTMAX=1048575, i.e. 1024*1024-1.
typedef ScaledInt<2048,int> scaled;
typedef ScaledInt<2048,uint> uscaled;

#define TYPEDEF_SCALED_ENUM(iScale,IntType,TypeName) \
	enum TypeName##Types {}; /* Not really supposed to be used anywhere else */ \
	typedef ScaledInt<iScale,IntType,TypeName##Types> TypeName;

/*	Note that the uint versions will not be called when
	the caller passes a positive signed int literal (e.g. 5);
	will have to be an unsigned literal (e.g. 5u). */
__forceinline uscaled per100(uint uiNum)
{
	return uscaled(uiNum, 100);
}
__forceinline scaled per100(int iNum)
{
	return scaled(iNum, 100);
}
__forceinline uscaled per1000(uint uiNum)
{
	return uscaled(uiNum, 1000);
}
__forceinline scaled per1000(int iNum)
{
	return scaled(iNum, 1000);
}
__forceinline uscaled per10000(uint uiNum)
{
	return uscaled(uiNum, 10000);
}
__forceinline scaled per10000(int iNum)
{
	return scaled(iNum, 10000);
}
/*	'scaled' construction from double. Only const expressions are allowed.
	Can only make sure of that through a macro. Tbd.: Could return a uscaled
	when the double expression is non-negative:
	choose_type<(dConstExpr) >= 0,uscaled,scaled>::type::fromRational
	Arithmetic operations are faster on uscaled, but mixing the two types
	isn't going to be helpful. So perhaps create a separate ufixp macro instead(?). */
#define fixp(dConstExpr) \
		((dConstExpr) >= ((int)MAX_INT) / 10000 - 1 || \
		(dConstExpr) <= ((int)MIN_INT) / 10000 + 1 ? \
		scaled(-1) : \
		scaled::fromRational<(int)( \
		(dConstExpr) * 10000 + ((dConstExpr) > 0 ? 0.5 : -0.5)), 10000>())

#undef ScaledInt_PARAMS
#undef ScaledInt_T
#undef INT_TYPE_CHOICE

#endif
