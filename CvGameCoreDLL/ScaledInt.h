#pragma once

#ifndef SCALED_INT_H
#define SCALED_INT_H

// advc.fract: New file

// Large lookup table, but ScaledInt.h will eventually be precompiled.
#include "FixedPointPowTables.h"

/*  class ScaledInt: Approximates a fractional number as an integer times a scale factor.
	For fixed-point arithmetic that can't lead to network sync issues.
	Performance: Comparable to double (see ScaledIntTest.cpp) so long as the scale factor
	is a power of 2.
	Overloads commonly used arithmetic operators and offers some conveniences that the
	built-in types don't have, e.g. abs, clamp, approxEquals, bernoulliSuccess (coin flip).
	Compile-time converter from double: macro 'fixp'
	Conversion from percentage: macro 'per100' (also 'per1000', 'per10000')
	scaled_int typedef for default precision.

	In code that uses Hungarian notation, I propose the prefix 'r' for
	ScaledInt variables, or more generally for any types that represent
	rational numbers without a floating point.

	The difference between ScaledInt and boost::rational is that the latter allows
	the denominator to change at runtime, which allows for greater accuracy but
	isn't as fast. */

/*  SCALE is the factor by which integer numbers are multiplied when converted
	to a ScaledInt (see constructor from int) and thus determines the precision
	of fractional numbers and the numeric limits (MAX, MIN) - the higher SCALE,
	the greater the precision and the tighter the limits.

	INT is the type of the underlying integer variable. Has to be an integral type.
	Both parameters are mostly internal to the implementation of ScaledInt. The public
	interface assumes that the client code works mostly with int, with types that
	can be cast implicitly to int and with double literals (see fixp macro).
	There are no operators allowing ScaledInt instances of different SCALE values or
	different INT types to be mixed. That said, there is a non-explicit constructor
	for conversion, so some of the existing operators will work for operands with
	differing template parameters.

	For unsigned INT types, internal integer divisions are rounded to the nearest INT
	in order to improve precision. For signed INT types, only getInt rounds to the
	nearest int; the internal divisions round toward 0 in order to avoid a sign check
	(branching). That extra precision is important for the pow function. If it's
	important in client code, then the typedef scaled_uint can be used and unsigned int
	const expressions can be passed to the per100 macro.

	Tbd.: Perhaps overload more operators for mixed signed/unsigned INT operands.
	If exactly one operand is signed, then the return type should be signed as well.
	Otherwise, the operand with the larger range (sizeof) should win out. */
template<int SCALE, class INT = int> // (uint SCALE leads to trouble w/ signed/unsigned conversion)
class ScaledInt
{
public:
	static INT const MAX;
	static INT const MIN;

	/*	Factory function for creating fractions (with wrapper macros per100).
		Numerator and denominator as template parameters ensure
		that the conversion to SCALE happens at compile time, so that
		floating-point math can be used for maximal accuracy.
		When the denominator isn't known at compile time, use ctor(int,int). */
	template<int iNUM, int iDEN>
	static inline ScaledInt<SCALE,INT> fromRational()
	{
		return fromDouble(iNUM / static_cast<double>(iDEN));
	}
	template<int iDEN>
	static inline ScaledInt<SCALE,INT> fromRational(int iNum)
	{
		ScaledInt<iDEN,INT> rRational;
		rRational.m_i = iNum;
		return rRational;
	}

	__forceinline static ScaledInt<SCALE,INT> max(
		ScaledInt<SCALE,INT> r1, ScaledInt<SCALE,INT> r2)
	{
		return std::max(r1, r2);
	}
	__forceinline static ScaledInt<SCALE,INT> min(
		ScaledInt<SCALE,INT> r1, ScaledInt<SCALE,INT> r2)
	{
		return std::min(r1, r2);
	}

	__forceinline ScaledInt() : m_i(0) {}
	__forceinline ScaledInt(int i) : m_i(SCALE * i)
	{
		FAssertBounds(MIN / SCALE, MAX / SCALE + 1, i);
	}
	__forceinline ScaledInt(uint u) : m_i(SCALE * u)
	{
		FAssert(u <= MAX / SCALE);
	}
	// Conversion between scales and int types
	__forceinline ScaledInt(int iNum, int iDen)
	{
		m_i = toScale(iNum, iDen);
	}
	template<int FROM_SCALE, class FROM_T>
	__forceinline ScaledInt(ScaledInt<FROM_SCALE,FROM_T> rOther)
	{
		if (FROM_SCALE == SCALE)
			m_i = static_cast<INT>(rOther.m_i);
		else
		{
			FAssertBounds(MIN / SCALE, MAX / SCALE + 1, rOther.m_i);
			m_i = static_cast<INT>((rOther.m_i * SCALE +
				// Only round to nearest when unsigned (avoid branching)
				(bSIGNED ? 0/*(FROM_SCALE / (rOther.m_i > 0 ? 2 : -2))*/ :
				FROM_SCALE / 2)) / FROM_SCALE);
		}
	}

	int getInt() const
	{
		// Assumed to be used less frequently than scale conversion. Take the time to round.
		return (m_i + SCALE / (!bSIGNED || m_i > 0 ? 2 : -2)) / SCALE;
	}
	__forceinline round() const // Alias
	{
		return getInt();
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
		return toScaleRound(m_i, SCALE, 100);
	}
	__forceinline int getPermille() const
	{
		return toScaleRound(m_i, SCALE, 1000);
	}
	__forceinline int roundToMultiple(int iMultiple) const
	{
		return toScaleRound(m_i, SCALE * iMultiple, 1) * iMultiple;
	}
	__forceinline double getDouble() const
	{
		return m_i / static_cast<double>(SCALE);
	}
	__forceinline float getFloat() const
	{
		return m_i / static_cast<float>(SCALE);
	}
	char const* str(int iDen = SCALE)
	{
		if (iDen == 1)
			_snprintf(szBuf, 32, "%s%d", isInt() ? "" : "ca. ", getInt());
		else if (iDen == 100)
			_snprintf(szBuf, 32, "%d percent", getPercent());
		else if (iDen == 1000)
			_snprintf(szBuf, 32, "%d permille", getPermille());
		else _snprintf(szBuf, 32, "%d/%d", toScale(m_i, SCALE, iDen), iDen);
		return szBuf;
	}

	__forceinline void mulDiv(int iMultiplier, int iDivisor)
	{
		m_i = toScale(m_i, iDivisor, iMultiplier);
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
		return (kRand.getInt(SCALE, szLog, iLogData1, iLogData2) < m_i);
	}

	ScaledInt<SCALE,INT> pow(int iExp) const
	{
		if (iExp < 0)
			return 1 / powNonNegative(-iExp);
		return powNonNegative(iExp);
	}
	ScaledInt<SCALE,INT> pow(ScaledInt<SCALE,INT> rExp) const
	{
		FAssert(!isNegative());
		if (rExp.bSIGNED && rExp.isNegative())
			return 1 / powNonNegative(-rExp);
		return powNonNegative(rExp);
	}
	__forceinline ScaledInt<SCALE,INT> sqrt() const
	{
		FAssert(!isNegative());
		return powNonNegative(fromRational<1,2>());
	}

	__forceinline ScaledInt<SCALE,INT> abs() const
	{
		ScaledInt<SCALE,INT> r;
		r.m_i = std::abs(m_i);
		return r;
	}

	template<class LoType,class HiType>
	__forceinline void clamp(LoType lo, HiType hi)
	{
		FAssert(lo <= hi);
		increaseTo(lo);
		decreaseTo(hi);
	}
	template<class LoType>
	__forceinline void increaseTo(LoType lo)
	{
		// (std::max doesn't allow differing types)
		if (*this < lo)
			*this = lo;
	}
	template<class HiType>
	__forceinline void decreaseTo(HiType hi)
	{
		if (*this > hi)
			*this = hi;
	}
	template<class LoType,class HiType>
	__forceinline ScaledInt<SCALE,INT> clamped(LoType lo, HiType hi) const
	{
		ScaledInt<SCALE,INT> rCopy(*this);
		rCopy.clamp(lo, hi);
		return rCopy;
	}
	template<class LoType>
	__forceinline ScaledInt<SCALE,INT> increasedTo(LoType lo) const
	{
		ScaledInt<SCALE,INT> rCopy(*this);
		rCopy.increaseTo(lo);
		return rCopy;
	}
	template<class HiType>
	__forceinline ScaledInt<SCALE,INT> decreasedTo(HiType hi) const
	{
		ScaledInt<SCALE,INT> rCopy(*this);
		rCopy.decreaseTo(hi);
		return rCopy;
	}

	template<class NumType, class Epsilon>
	__forceinline bool approxEquals(NumType num, Epsilon e) const
	{
		// Can't be allowed for floating point types; will have to use fixp to wrap.
		BOOST_STATIC_ASSERT(!std::numeric_limits<int>::has_infinity);
		return ((*this - num).abs() <= e);
	}

	__forceinline bool isPositive() const { return (m_i > 0); }
	__forceinline bool isNegative() const { return (bSIGNED && m_i < 0); }

	__forceinline ScaledInt<SCALE,INT> operator-() { return ScaledInt<SCALE,INT>(-m_i); }

	__forceinline bool operator<(ScaledInt<SCALE,INT> rOther) const
	{
		return (m_i < rOther.m_i);
	}
	__forceinline bool operator>(ScaledInt<SCALE,INT> rOther) const
	{
		return (m_i > rOther.m_i);
	}
    __forceinline bool operator==(ScaledInt<SCALE,INT> rOther) const
	{
		return (m_i == rOther.m_i);
	}
	__forceinline bool operator!=(ScaledInt<SCALE,INT> rOther) const
	{
		return (m_i != rOther.m_i);
	}
	__forceinline bool operator<=(ScaledInt<SCALE,INT> rOther) const
	{
		return (m_i <= rOther.m_i);
	}
	__forceinline bool operator>=(ScaledInt<SCALE,INT> rOther) const
	{
		return (m_i >= rOther.m_i);
	}

	/*	Make comparisons with int exact - to be consistent with int-float comparisons.
		(The alternative would be to compare i with this->getInt()). */
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

	__forceinline ScaledInt<SCALE,INT>& operator+=(ScaledInt<SCALE,INT> rOther)
	{
		// Maybe uncomment this for some special occasion
		/*FAssert(rOther <= 0 || m_i <= MAX - rOther.m_i);
		FAssert(rOther >= 0 || m_i >= MIN + rOther.m_i);*/
		m_i += rOther.m_i;
		return *this;
	}
	__forceinline ScaledInt<SCALE,INT>& operator-=(ScaledInt<SCALE,INT> rOther)
	{
		/*FAssert(rOther >= 0 || m_i <= MAX + rOther.m_i);
		FAssert(rOther <= 0 || m_i >= MIN - rOther.m_i);*/
		m_i -= rOther.m_i;
		return *this;
	}
	__forceinline ScaledInt<SCALE,INT>& operator*=(ScaledInt<SCALE,INT> rOther)
	{
		if (std::numeric_limits<INT>::digits <= 32)
		{
			/*	According to my tests, MulDiv is slower than spelling the 32b/64b
				conversion out. Perhaps MulDiv loses some time checking for overflow?
				I'll check in an assertion, but if the 64b-to-32b conversion fails,
				that's really the caller's responsibility. */
			/*int iNum = MulDiv(m_i, rOther.m_i, SCALE);
			//	-1 is how MulDiv indicates overflow, but could also be the legit result
			//	of -1/SCALE times 1/SCALE.
			FAssert(iNum != -1 || (m_i * rOther.m_i) / SCALE == -1);
			m_i = iNum;*/
			long long lNum = (m_i * rOther.m_i + (bSIGNED ? 0 : SCALE / 2)) / SCALE;
			FAssert(lNum >= MIN && lNum <= MAX);
			m_i = static_cast<INT>(lNum);
		}
		else
		{
			/*	If INT has more than 32b, then this might be fine.
				For int32_t and SCALE=1024, it would overflow already
				when computing 46*46. */
			FAssertBounds(MIN / rOther.m_i, MAX / rOther.m_i + 1, m_i);
			m_i *= rOther.m_i;
			if (!bSIGNED) // Round to nearest when sign doesn't need to be checked
				m_i += SCALE / 2;
			m_i /= SCALE;
		}
		/*	Another approach (when SCALE is a power of 2) would be to look for the
			least significant set bit in m_i and rOther.m_i in order to reduce
			(m_i*rOther.m_i)/SCALE. See comment about _BitScan elsewhere in this file. */
		/*	Another thought: Could add a bool template parameter to ScaledInt that
			enables the else branch above regardless of the size of INT --
			for client code where speed is essential and overflow impossible. */
		return *this;
	}
	__forceinline ScaledInt<SCALE,INT>& operator/=(ScaledInt<SCALE,INT> rOther)
	{
		FAssertBounds(MIN / SCALE, MAX / SCALE + 1, m_i);
		m_i *= SCALE;
		if (!bSIGNED) // Round to nearest when sign doesn't need to be checked
			m_i += rOther.m_i / 2;
		// (For signed rounding, see ROUND_DIVIDE in CvGameCoreUtils.h)
		m_i /= rOther.m_i;
		return *this;
	}

	__forceinline ScaledInt<SCALE,INT>& operator++()
	{
		(*this) += 1;
		return *this;
	}
	__forceinline ScaledInt<SCALE,INT>& operator--()
	{
		(*this) -= 1;
		return *this;
	}
	__forceinline ScaledInt<SCALE,INT> operator++(int)
	{
		ScaledInt<SCALE,INT> rCopy(*this);
		(*this) += 1;
		return rCopy;
	}
	__forceinline ScaledInt<SCALE,INT> operator--(int)
	{
		ScaledInt<SCALE,INT> rCopy(*this);
		(*this) -= 1;
		return rCopy;
	}

	__forceinline ScaledInt<SCALE,INT>& operator+=(int i)
	{
		(*this) += ScaledInt<SCALE,INT>(i);
		return (*this);
	}
	__forceinline ScaledInt<SCALE,INT>& operator-=(int i)
	{
		(*this) -= ScaledInt<SCALE,INT>(i);
		return (*this);
	}
	__forceinline ScaledInt<SCALE,INT>& operator*=(int i)
	{
		(*this) *= ScaledInt<SCALE,INT>(i);
		return (*this);
	}
	__forceinline ScaledInt<SCALE,INT>& operator/=(int i)
	{
		(*this) /= ScaledInt<SCALE,INT>(i);
		return (*this);
	}
	__forceinline ScaledInt<SCALE,INT>& operator+=(uint u)
	{
		(*this) += ScaledInt<SCALE,INT>(u);
		return (*this);
	}
	__forceinline ScaledInt<SCALE,INT>& operator-=(uint u)
	{
		(*this) -= ScaledInt<SCALE,INT>(u);
		return (*this);
	}
	__forceinline ScaledInt<SCALE,INT>& operator*=(uint u)
	{
		(*this) *= ScaledInt<SCALE,INT>(u);
		return (*this);
	}
	__forceinline ScaledInt<SCALE,INT>& operator/=(uint u)
	{
		(*this) /= ScaledInt<SCALE,INT>(u);
		return (*this);
	}

	__forceinline ScaledInt<SCALE,INT> operator+(int i) const
	{
		ScaledInt<SCALE,INT> rCopy(*this);
		rCopy += i;
		return rCopy;
	}
	__forceinline ScaledInt<SCALE,INT> operator-(int i) const
	{
		ScaledInt<SCALE,INT> rCopy(*this);
		rCopy -= i;
		return rCopy;
	}
	__forceinline ScaledInt<SCALE,INT> operator*(int i) const
	{
		ScaledInt<SCALE,INT> rCopy(*this);
		rCopy *= i;
		return rCopy;
	}
	__forceinline ScaledInt<SCALE,INT> operator/(int i) const
	{
		ScaledInt<SCALE,INT> rCopy(*this);
		rCopy /= i;
		return rCopy;
	}
	__forceinline ScaledInt<SCALE,INT> operator+(uint u) const
	{
		ScaledInt<SCALE,INT> rCopy(*this);
		rCopy += u;
		return rCopy;
	}
	__forceinline ScaledInt<SCALE,INT> operator-(uint u) const
	{
		ScaledInt<SCALE,INT> rCopy(*this);
		rCopy -= u;
		return rCopy;
	}
	__forceinline ScaledInt<SCALE,INT> operator*(uint u) const
	{
		ScaledInt<SCALE,INT> rCopy(*this);
		rCopy *= u;
		return rCopy;
	}
	__forceinline ScaledInt<SCALE,INT> operator/(uint u) const
	{
		ScaledInt<SCALE,INT> rCopy(*this);
		rCopy /= u;
		return rCopy;
	}

private:
	INT m_i;

	static char szBuf[32]; // for str function

	template<int OTHER_SCALE,class OTHER_INT>
	friend class ScaledInt;

	__forceinline INT toScale(int iNum, int iFromScale, int iToScale = SCALE) const
	{
		// Akin to code in ctor(ScaledInt) and operator*=(ScaledInt)
		long long lNum = iNum * iToScale;
		if (!bSIGNED)
			lNum += iFromScale / 2;
		lNum /= iFromScale;
		FAssert(lNum >= MIN && lNum <= MAX);
		return static_cast<INT>(lNum);
	}
	int toScaleRound(int iNum, int iFromScale, int iToScale = SCALE) const
	{
		long long lNum = iNum * iToScale;
		if (!bSIGNED)
			lNum += iFromScale / 2;
		else lNum += iFromScale / (lNum >= 0 ? 2 : -2);
		lNum /= iFromScale;
		FAssert(lNum >= MIN && lNum <= MAX);
		return static_cast<int>(lNum);
	}

	ScaledInt<SCALE,INT> powNonNegative(int iExp) const
	{
		ScaledInt<SCALE,INT> rCopy(*this);
		/*  This can be done faster in general by squaring.
			However, I doubt that it would be faster for
			the small exponents I'm expecting to deal with*/
		ScaledInt<SCALE,INT> r = 1;
		for (int i = 0; i < iExp; i++)
			r *= rCopy;
		return r;
	}
	/*	Custom algorithm.
		There is a reasonably recent paper "A Division-Free Algorithm for Fixed-Point
		Power Exponential Function in Embedded System" [sic] based on Newton's method.
		That's probably faster and more accurate, but an implementation isn't
		spelled out. Perhaps tbd. */
	ScaledInt<SCALE,INT> powNonNegative(ScaledInt<SCALE,INT> rExp) const
	{
		/*	Base 0 or too close to it to make a difference given the precision of the algorithm.
			Fixme: rExp could also be close to 0. Should somehow use x=y*z => b^x = (b^y)^z. */
		if (m_i < SCALE / 64)
			return 0;
		/*	Recall that: If x=y+z, then b^x=(b^y)*(b^z).
						 If b=a*c, then b^x=(a^x)*(c^x). */
		// Split rExp into the sum of an integer and a (scaled) fraction between 0 and 1
		// Running example: 5.2^2.1 at SCALE 1024, i.e. (5325/1024)^(2150/1024)
		INT expInt = rExp.m_i / SCALE; // 2 in the example
		// Use uint in all local ScaledInt variables for more accurate rounding
		ScaledInt<128,uint> rExpFrac(rExp - expInt); // Ex.: 13/128
		/*	Factorize the base into powers of 2 and, as the last factor, the base divided
			by the product of the 2-bases. */
		ScaledInt<SCALE,uint> rProductOfPowersOfTwo(1);
		INT baseDiv = 1;
		// Look up approximate result of 2^rExpFrac in precomputed table
		FAssertBounds(0, 128, rExpFrac.m_i); // advc.tmp: Don't keep this assert permanently
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
		FAssertBounds(0, 64+1, iLastBaseTimes64); // advc.tmp: Don't keep this assert permanently
		if (rExpFrac.m_i != 0 && iLastBaseTimes64 != 64)
		{
			// Could be prone to cache misses :(
			rLastFactor.m_i = FixedPointPowTables::powersUnitInterval_256
					[iLastBaseTimes64-1][rExpFrac.m_i-1] + 1; // Table and values are shifted by 1
			// Ex.: Position [41][12] is 244, i.e. rLastFactor=245/256. Approximation of (5.2/8)^0.1
		}
		ScaledInt<SCALE,INT> r(ScaledInt<SCALE,uint>(pow(expInt)) *
				rProductOfPowersOfTwo * ScaledInt<SCALE,uint>(rLastFactor));
		return r;
		/*	Ex.: First factor is 27691/1024, approximating 5.2^2,
			second factor: 1270/1024, approximating (2^0.1)^3,
			last factor: 980/1024, approximating (5.2/8)^0.1.
			Result: 32867/1024, which is ca. 32.097, whereas 5.2^2.1 is ca. 31.887. */
	}

	static __forceinline long long scaleForComparison(int i)
	{
		// If long long is too slow, we'd have to return an int after checking:
		//FAssertBounds(MIN_INT / SCALE, MAX_INT / SCALE + 1, i);
		return i * SCALE;
	}

	static __forceinline ScaledInt<SCALE,INT> fromDouble(double d)
	{
		ScaledInt<SCALE,INT> r;
		r.m_i = static_cast<INT>(d * SCALE + (d > 0 ? 0.5 : -0.5));
		return r;
	}
	static bool const bSIGNED = std::numeric_limits<INT>::is_signed;
};

template<int SCALE, class INT>
INT const ScaledInt<SCALE,INT>::MAX = std::numeric_limits<INT>::max();
template<int SCALE, class INT>
INT const ScaledInt<SCALE,INT>::MIN = std::numeric_limits<INT>::min();
/*	MAX and MIN aren't compile-time constants (they would be in C++11).
	We also don't have SIZE_MAX (cstdint) and
	no boost::integer_traits<INT>::const_max. */
//BOOST_STATIC_ASSERT(SCALE*SCALE < MAX);

template<int SCALE, class INT>
char ScaledInt<SCALE,INT>::szBuf[] = {};

template<int SCALE, class INT>
__forceinline ScaledInt<SCALE,INT> operator+(
	ScaledInt<SCALE,INT> rLeft, ScaledInt<SCALE,INT> rRight)
{
	rLeft += rRight;
	return rLeft;
}
template<int SCALE, class INT>
__forceinline ScaledInt<SCALE,INT> operator-(
	ScaledInt<SCALE,INT> rLeft, ScaledInt<SCALE,INT> rRight)
{
	rLeft -= rRight;
	return rLeft;
}
template<int SCALE, class INT>
__forceinline ScaledInt<SCALE,INT> operator*(
	ScaledInt<SCALE,INT> rLeft, ScaledInt<SCALE,INT> rRight)
{
	rLeft *= rRight;
	return rLeft;
}
template<int SCALE, class INT>
__forceinline ScaledInt<SCALE,INT> operator/(
	ScaledInt<SCALE,INT> rLeft, ScaledInt<SCALE,INT> rRight)
{
	rLeft /= rRight;
	return rLeft;
}

/*	Commutativity
	Tbd.: Try using boost/operators.hpp instead:
	equality_comparable with itself, int and uint (both ways); incrementable; decrementable;
	addable to int, uint, double; int and uint addable to ScaledInt; same for
	subtractable, divisible, multipliable.
	However, boost uses reference parameters for everything. */
template<int SCALE, class INT>
__forceinline ScaledInt<SCALE,INT> operator+(int i, ScaledInt<SCALE,INT> r)
{
	return r + i;
}
/*	As we don't implement an int cast operator, assignment to int
	should be forbidden as well. (No implicit getInt.) */
/*template<int SCALE, class INT>
__forceinline int& operator+=(int& i, ScaledInt<SCALE,INT> r)
{
	i = (r + i).getInt();
	return i;
}*/
template<int SCALE, class INT>
__forceinline ScaledInt<SCALE,INT> operator-(int i, ScaledInt<SCALE,INT> r)
{
	return ScaledInt<SCALE,INT>(i) - r;
}
/*template<int SCALE, class INT>
__forceinline int& operator-=(int& i, ScaledInt<SCALE,INT> r)
{
	i = (ScaledInt<SCALE,INT>(i) - r).getInt();
	return i;
}*/
template<int SCALE, class INT>
__forceinline ScaledInt<SCALE,INT> operator*(int i, ScaledInt<SCALE,INT> r)
{
	return r * i;
}
/*template<int SCALE, class INT>
__forceinline int& operator*=(int& i, ScaledInt<SCALE,INT> r)
{
	i = (r * i).getInt();
	return i;
}*/
template<int SCALE, class INT>
__forceinline ScaledInt<SCALE,INT> operator/(int i, ScaledInt<SCALE,INT> r)
{
	return ScaledInt<SCALE,INT>(i) / r;
}
/*template<int SCALE, class INT>
__forceinline int& operator/=(int& i, ScaledInt<SCALE,INT> r)
{
	i = (ScaledInt<SCALE,INT>(i) / r).getInt();
	return i;
}*/
template<int SCALE, class INT>
__forceinline ScaledInt<SCALE,INT> operator+(uint u, ScaledInt<SCALE,INT> r)
{
	return r + u;
}
template<int SCALE, class INT>
__forceinline ScaledInt<SCALE,INT> operator-(uint u, ScaledInt<SCALE,INT> r)
{
	return r - u;
}
template<int SCALE, class INT>
__forceinline ScaledInt<SCALE,INT> operator*(uint ui, ScaledInt<SCALE,INT> r)
{
	return r * ui;
}
template<int SCALE, class INT>
__forceinline ScaledInt<SCALE,INT> operator/(uint u, ScaledInt<SCALE,INT> r)
{
	return r / u;
}
template<int SCALE, class INT>
 __forceinline bool operator<(int i, ScaledInt<SCALE,INT> r)
{
	return (r > i);
}
template<int SCALE, class INT>
__forceinline bool operator>(int i, ScaledInt<SCALE,INT> r)
{
	return (r < i);
}
template<int SCALE, class INT>
 __forceinline bool operator==(int i, ScaledInt<SCALE,INT> r)
{
	return (r == i);
}
 template<int SCALE, class INT>
 __forceinline bool operator!=(int i, ScaledInt<SCALE,INT> r)
{
	return (r != i);
}
template<int SCALE, class INT>
__forceinline bool operator<=(int i, ScaledInt<SCALE,INT> r)
{
	return (r >= i);
}
template<int SCALE, class INT>
 __forceinline bool operator>=(int i, ScaledInt<SCALE,INT> r)
{
	return (r <= i);
}
template<int SCALE, class INT>
__forceinline bool operator<(double d, ScaledInt<SCALE,INT> r)
{
	return (r > d);
}
template<int SCALE, class INT>
__forceinline bool operator>(double d, ScaledInt<SCALE,INT> r)
{
	return (r < d);
}

// 1024 isn't very precise at all - but at least better than the percent scale normally used by BtS.
typedef ScaledInt<1024,int> scaled_int;
typedef ScaledInt<1024,uint> scaled_uint;

/*	The uint versions will, unfortunately, not be called when
	the caller passes a positive signed int literal. */
__forceinline scaled_uint per100(uint iNum)
{
	return scaled_uint::fromRational<100>(iNum);
}
__forceinline scaled_int per100(int iNum)
{
	return scaled_int::fromRational<100>(iNum);
}
__forceinline scaled_uint per1000(uint iNum)
{
	return scaled_uint::fromRational<1000>(iNum);
}
__forceinline scaled_int per1000(int iNum)
{
	return scaled_int::fromRational<1000>(iNum);
}
__forceinline scaled_uint per10000(uint iNum)
{
	return scaled_uint::fromRational<10000>(iNum);
}
__forceinline scaled_int per10000(int iNum)
{
	return scaled_int::fromRational<10000>(iNum);
}
/*	For double, only const expressions are allowed. Can only
	make sure of that through a macro. The macro can't use
	(dConstExpr) >= 0 ? scaled_uint::fromRational<...> : scaled_int::fromRational<...>
	b/c the ternary-? operator has to have compatible and unambigious operands types. */
#define fixp(dConstExpr) \
		((dConstExpr) >= ((int)MAX_INT) / 10000 - 1 || \
		(dConstExpr) <= ((int)MIN_INT) / 10000 + 1 ? \
		scaled_int(-1) : \
		scaled_int::fromRational<(int)( \
		(dConstExpr) * 10000 + ((dConstExpr) > 0 ? 0.5 : -0.5)), 10000>())

#endif
