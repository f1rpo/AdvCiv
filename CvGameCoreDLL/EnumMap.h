/*  advc.enum: From the "We the People" (WtP) mod for Civ4Col, original author: Nightinggale,
	who may still be working on the EnumMap classes. This version is from 3 Nov 2019.
	I have -for now- omitted the WtP serialization functions, and uncoupled the
	code from the Perl-generated enums that WtP uses. Instead of defining
	ArrayLength functions, the getEnumLength functions that AdvCiv defines in
	CvEnums.h and CvGlobals.h are used.
	Formatting: linebreaks added before scope resolution operators.
	advc.fract: Disabled the INLINE_NATIVE representation for "small" enum types
	in order to allow T=ScaledNum. (As suggested to me by Nightinggale.)
	Functions for bitwise operations moved into BitUtil.h (included in PCH);
	WtP defines them directly in the PCH. */

#pragma once

#ifndef ENUM_MAP_H
#define ENUM_MAP_H

// advc: Moved up; (VS2010) IntelliSense needs it here.
template <class T> struct EnumMapGetDefault {};


// EnumMap is a special case of map where there is a <key,value> pair for each key in an enum.
// docs.oracle.com/en/java/javase/13/docs/api/java.base/java/util/EnumMap.html
//
// Put in civ4 terms, it's an array with a fixed length and automated memory management.
// Memory is allocated using a lazy approach and occasionally freed as well if it doesn't matter outside the class.
// This means, like say std::vector, it can be used without considering memory allocations at all and it can't leak.
//
// The array is set up with template parameters as this allows "arguments" to the default constructor.
// Often an array of arrays can only use the default constructor for the inner array, making flexible default
// constructors important.
//
// Strict with types in arguments and assert checks (both compile and runtime) in order to catch bugs.
// Highly optimized for performance (particularly with hardcoded xml lengths) and low memory usage.
//
// Example usage:
// EnumMap<YieldTypes,bool>
// EnumMap<BuildingTypes,int>
// EnumMap<TeamTypes, PlayerTypes>
//
// See the end of the file for (often unneeded) additionaly features, like disabling type checks and altering default values.
// DEFAULT is 0, except if the second parameter is an enum, in which case the default is -1 (like NO_PLAYER)

enum
{
	ENUMMAP_SIZE_NATIVE,
	ENUMMAP_SIZE_1_BYTE,
	ENUMMAP_SIZE_2_BYTE,
	ENUMMAP_SIZE_BOOL,

	// max bytes used by class instance
	// used for determining if an array should be inside the class instance instead of a pointer.
	// this saves memory I/O, but with the tradeoff of making the instance bigger, which is not always wanted.
	// This is particularly useful if the array is no bigger than a pointer, like a char array of length 3-4.
	//
	// Note: the compiler will use 4 byte alignment, meaning this works best if set to a multiple of 4
	ENUMMAP_MAX_BYTES = 4,

	// set how many bits can be inlined in the class itself
	// if a bool array has <= this number of bits, then, instead of allocating memory elsewhere,
	// the data will be placed in the class itself
	ENUMMAP_MAX_INLINE_BOOL = 64,

	// bitmasks to get the bits, which gives the indexes to store 8 or 32 bit.
	// modulo is slow at runtime, binary AND is fast. They give the same result in this case.
	// Maybe the compiler will optimize to a binary and, but explicitly writing it means we are certain it will optimize.
	// In fact this way it's "optimized" even in debug builds.
	//ENUMMAP_BITMASK_8_BIT = 7, // advc: unused for now
	ENUMMAP_BITMASK_32_BIT = 0x1F,
};

template<class IndexType, class T, int DEFAULT, class T_SUBSET = IndexType, class LengthType = IndexType>
class EnumMapBase
{
public:
	EnumMapBase();
	~EnumMapBase();

	// const values (per class)
	T getDefault() const;
	IndexType First() const;
	IndexType getLength() const;
	IndexType numElements() const;

	// array access
	T get(IndexType eIndex) const;
	void set(IndexType eIndex, T tValue);
	// advc: Allow individual elements to be reset
	__forceinline void reset(IndexType eIndex) { set(eIndex, (T)DEFAULT); }
	void add(IndexType eIndex, T tValue);
	void multiply(IndexType eIndex, T tMultiplier); // advc

	// add bound checks. Ignore call if out of bound index
	void safeSet(IndexType eIndex, T tValue);
	void safeAdd(IndexType eIndex, T tValue);

	// add a number to all indexes
	void addAll(T tValue);

	// get the sum of all elements // advc.fract: return type was int
	T getTotal() const;
	int getSupportSz() const; // advc.fract: Since the above is no longer usable for bool

	// Check if there is non-default contents.
	// isAllocated() test for a null pointer while hasContent() will loop the array to test each index for default value.
	// Useful to avoid looping all 0 arrays and when creating savegames.
	// Note: hasContent() can release memory if it doesn't alter what get() will return.
	bool isAllocated() const;
	bool hasContent() const;

	T getMin() const;
	T getMax() const;

	void keepMin(IndexType eIndex, T tValue);
	void keepMax(IndexType eIndex, T tValue);

	// memory allocation and freeing
	void reset();
	void setAll(T tValue);

	/*  <advc> (note): These new parameters are a mess (the read/Write functions ignore them
		in some circumstances). It's temporary measure until adopting the
		SavegameReader/Writer classes from WtP. */
	void Read(FDataStreamBase* pStream, bool bAsInt = true, bool bAsShort = false,
			bool bAsFloat = false, bool bAsDouble = false/*, bool bSkipLast = false*/);
	void ReadFloat(FDataStreamBase* pStream, bool bAsDouble = true);
	// For converting from MAX_CIV_PLAYERS/TEAMS to MAX_PLAYERS/TEAMS. Not needed after all (so far).
	/*void ReadButOne(FDataStreamBase* pStream, bool bAsInt = false, bool bAsFloat = false,
			bool bAsDouble = false);*/
	void Write(FDataStreamBase* pStream, bool bAsInt = true, bool bAsFloat = false) const; // </advc>

	////
	//// End of functions
	//// There is no need to keep reading this class declaration unless you are interested in the internal implementation
	////
// Compile-time constants
	// advc: Renamed from "LENGTH"; can't guarantee a length for types loaded from XML.
	static const int MAX_LENGTH = EnumMapGetDefault<LengthType>::MAX_LENGTH;
private: // advc (Maybe some of these should indeed be public, but probably not all.)
	static const int SIZE = EnumMapGetDefault<T>::SIZE;
	static const int SIZE_OF_T = EnumMapGetDefault<T>::SIZE_OF_T;
	/*static const bool bINLINE_NATIVE = (SIZE == ENUMMAP_SIZE_NATIVE && (MAX_LENGTH * SIZE_OF_T) <= ENUMMAP_MAX_BYTES);
	static const int NUM_NATIVE_BLOCKS = (bINLINE_NATIVE ? MAX_LENGTH : 1);*/ // advc.fract

	static const bool bINLINE_1_BYTE = (SIZE == ENUMMAP_SIZE_1_BYTE && (MAX_LENGTH * SIZE_OF_T) <= ENUMMAP_MAX_BYTES);
	static const int NUM_1_BYTE_BLOCKS = (bINLINE_1_BYTE ? MAX_LENGTH : 1);

	static const bool bINLINE_2_BYTE = (SIZE == ENUMMAP_SIZE_2_BYTE && (MAX_LENGTH * SIZE_OF_T) <= ENUMMAP_MAX_BYTES);
	static const int NUM_2_BYTE_BLOCKS = (bINLINE_2_BYTE ? MAX_LENGTH : 1);

	static const bool bINLINE_BOOL = (SIZE == ENUMMAP_SIZE_BOOL && MAX_LENGTH <= ENUMMAP_MAX_INLINE_BOOL);
	static const int NUM_BOOL_BLOCKS = bINLINE_BOOL ? (MAX_LENGTH + 31) / 32 : 1;
	static const unsigned int BOOL_BLOCK_DEFAULT = DEFAULT ? MAX_UNSIGNED_INT : 0;

	static const bool bINLINE = (/*bINLINE_NATIVE ||*/ // advc.fract
			bINLINE_1_BYTE || bINLINE_2_BYTE || bINLINE_BOOL);

private: // advc: Since these aren't implemented (yet), make them private.
	// operator overload
	EnumMapBase& operator=(const EnumMapBase &rhs);

	template<class T2, int DEFAULT2>
	EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>&
			operator=(const EnumMapBase<IndexType, T2, DEFAULT2, T_SUBSET, LengthType> &rhs);
	template<class T2, int DEFAULT2>
	EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>&
			operator+=(const EnumMapBase<IndexType, T2, DEFAULT2, T_SUBSET, LengthType> &rhs);
	template<class T2, int DEFAULT2>
	EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>&
			operator-=(const EnumMapBase<IndexType, T2, DEFAULT2, T_SUBSET, LengthType> &rhs);
	template<class T2, int DEFAULT2>
	bool operator==(const EnumMapBase<IndexType, T2, DEFAULT2, T_SUBSET, LengthType> &rhs) const;
	template<class T2, int DEFAULT2>
	bool operator!=(const EnumMapBase<IndexType, T2, DEFAULT2, T_SUBSET, LengthType> &rhs) const;


private:

	// the actual memory in this class
	// since it's a union, the goal is to keep all of them to 4 bytes
	// the exception is when bINLINE is set and is allowed to use more than 4 bytes
	// an example of this is <PlayerTypes, bool> has a bool for each player, hence 8 bytes
	union
	{
		T* m_pArrayFull;
		short* m_pArrayShort;
		char* m_pArrayChar;
		unsigned int* m_pArrayBool;
		/*	advc.fract: This wouldn't work for ScaledNum
			b/c C++03 allows only POD types in unions. */
		//T m_InlineNative[NUM_NATIVE_BLOCKS];
		char m_Inline_1_byte[NUM_1_BYTE_BLOCKS];
		short m_Inline_2_byte[NUM_2_BYTE_BLOCKS];
    	unsigned int m_InlineBoolArray[NUM_BOOL_BLOCKS];
	};

	// the code will technically still work if this fails, but it will waste memory
	BOOST_STATIC_ASSERT(sizeof(T) <= 4 /*|| bINLINE_NATIVE*/); // advc.fract
	enum
	{
		SAVE_ARRAY_MULTI_BYTE,
		SAVE_ARRAY_LAST_TOKEN,
		SAVE_ARRAY_INDEX_OFFSET,
		SAVE_ARRAY_EMPTY_BYTE = 0xFF,
		SAVE_ARRAY_EMPTY_SHORT = 0xFFFF,
	};

	class interval
	{
	public:
		IndexType first;
		IndexType last;

		interval() : first((IndexType)0), last((IndexType)0) {}
	};

	// bool helpers
	int getBoolArrayBlock(int iIndex) const
	{
		if (bINLINE_BOOL && NUM_BOOL_BLOCKS == 1)
		{
			// hardcode only using first block in arrays hardcoded to only use one block
			// with a bit of luck the compiler can optimize the array code away completely
			FAssert(iIndex < 32);
			return 0;
		}
		else return iIndex / 32;
	}
	// advc: This one had no inline keyword
	__forceinline int getBoolArrayIndexInBlock(int iIndex) const
	{
		return iIndex & ENUMMAP_BITMASK_32_BIT;
	}

	////
	//// Specialized functions
	////
	//// The idea is that the "outside world" calls a non-template function,
	//// which then calls a template function for the same purpose.
	//// This allows the outside world to stay simple (get/set etc) while at the same time
	//// allowing specialized functions for each.
	//// This makes it easier for the compiler to optimize and allows more code.
	//// The key difference is that before the compiler would compile for all cases and then hopefully
	//// discard unused cases. Now it only compiles the specialized case matching the class templates in question.
	//// This allows code, which calls overloaded functions, which doesn't support all cases of all template classes.
	////
	//// Written inside the class declaration due to a C++ limitation.
	//// According to the standard, partial specialized functions aren't allowed and
	//// specialized functions can't be used for unspecialized template classes.
	//// However written inside the class declaration, the compiler treats the class as specialized,
	//// hence allowing specialized functions
	////

	// declarations
	// to avoid being mixed with what the outside world calls, all of them are private and have a _ prefix

	template <bool bInline, int iSize>
	T _get(int iIndex) const;

	template <bool bInline, int iSize>
	void _set(int iIndex, T tValue);

	template <bool bInline, int iSize>
	void _allocate(T tValue = (T)DEFAULT);

	template <bool bInline, int iSize>
	void _setAll(T val);

	template <bool bInline>
	unsigned int _getNumBoolBlocks() const;

	template <int iSize>
	void _Read(/* <advc> */ FDataStreamBase* pStream, bool bAsInt = true, bool bAsShort = false,
			bool bAsFloat = false, bool bAsDouble = false/*, bool bSkipLast = false*/); // </advc>

	template <int iSize>
	void _Write(/* advc: */ FDataStreamBase* pStream, bool bAsInt = true, bool bAsFloat = false) const;

	//
	// The actual specialized impletation of the functions
	//

	// get  (advc: were all force-inlined)
	template<>
	T _get<false, ENUMMAP_SIZE_NATIVE>(int iIndex) const
	{
		return (T)(m_pArrayFull ? m_pArrayFull[iIndex] : DEFAULT);
	}
	template<>
	T _get<false, ENUMMAP_SIZE_1_BYTE>(int iIndex) const
	{
		return (T)(m_pArrayChar ? m_pArrayChar[iIndex] : DEFAULT);
	}
	template<>
	T _get<false, ENUMMAP_SIZE_2_BYTE>(int iIndex) const
	{
		return (T)(m_pArrayShort ? m_pArrayShort[iIndex] : DEFAULT);
	}
	template<>
	T _get<false, ENUMMAP_SIZE_BOOL>(int iIndex) const
	{
		return m_pArrayBool ? BitUtil::HasBit(m_pArrayBool[getBoolArrayBlock(iIndex)], getBoolArrayIndexInBlock(iIndex)) : DEFAULT;
	}
	/*template<>
	__forceinline T _get<true, ENUMMAP_SIZE_NATIVE>(int iIndex) const
	{
		return m_InlineNative[iIndex];
	}*/ // advc.fract
	template<>
	__forceinline T _get<true, ENUMMAP_SIZE_1_BYTE>(int iIndex) const
	{
		return (T)m_Inline_1_byte[iIndex];
	}
	template<>
	__forceinline T _get<true, ENUMMAP_SIZE_2_BYTE>(int iIndex) const
	{
		return (T)m_Inline_2_byte[iIndex];
	}
	template<>
	inline T _get<true, ENUMMAP_SIZE_BOOL>(int iIndex) const
	{
		return BitUtil::HasBit(m_InlineBoolArray[getBoolArrayBlock(iIndex)], getBoolArrayIndexInBlock(iIndex));
	}

	// set
	template<>
	__forceinline void _set<false, ENUMMAP_SIZE_NATIVE>(int iIndex, T tValue)
	{
		m_pArrayFull[iIndex] = tValue;
	}
	template<>
	__forceinline void _set<false, ENUMMAP_SIZE_1_BYTE>(int iIndex, T tValue)
	{	// advc: Cast added to conform with /W4
		m_pArrayChar[iIndex] = static_cast<char>(tValue);
	}
	template<>
	__forceinline void _set<false, ENUMMAP_SIZE_2_BYTE>(int iIndex, T tValue)
	{
		m_pArrayShort[iIndex] = static_cast<short>(tValue); // advc: cast (see above)
	}
	template<>
	inline void _set<false, ENUMMAP_SIZE_BOOL>(int iIndex, T tValue)
	{
		BitUtil::SetBit(m_pArrayBool[getBoolArrayBlock(iIndex)],
				getBoolArrayIndexInBlock(iIndex), tValue ? 1 : 0);
	}
	/*template<>
	__forceinline void _set<true, ENUMMAP_SIZE_NATIVE>(int iIndex, T tValue)
	{
		m_InlineNative[iIndex] = tValue;
	}*/ // advc.fract
	template<>
	__forceinline void _set<true, ENUMMAP_SIZE_1_BYTE>(int iIndex, T tValue)
	{
		m_Inline_1_byte[iIndex] = tValue;
	}
	template<>
	__forceinline void _set<true, ENUMMAP_SIZE_2_BYTE>(int iIndex, T tValue)
	{
		m_Inline_2_byte[iIndex] = tValue;
	}
	template<>
	inline void _set<true, ENUMMAP_SIZE_BOOL>(int iIndex, T tValue)
	{
		BitUtil::SetBit(m_InlineBoolArray[getBoolArrayBlock(iIndex)], getBoolArrayIndexInBlock(iIndex), tValue ? 1 : 0);
	}

	// setAll
	template<>
	__forceinline void _setAll<false, ENUMMAP_SIZE_NATIVE>(T tValue)
	{
		std::fill_n(m_pArrayFull, numElements(), tValue);
	}
	template<>
	__forceinline void _setAll<false, ENUMMAP_SIZE_1_BYTE>(T tValue)
	{
		std::fill_n(m_pArrayChar, numElements(), tValue);
	}
	template<>
	__forceinline void _setAll<false, ENUMMAP_SIZE_2_BYTE>(T tValue)
	{
		std::fill_n(m_pArrayShort, numElements(), tValue);
	}
	template<>
	__forceinline void _setAll<false, ENUMMAP_SIZE_BOOL>(T tValue)
	{
		std::fill_n(m_pArrayBool, _getNumBoolBlocks<bINLINE_BOOL>(), tValue ? MAX_UNSIGNED_INT : 0);
	}
	/*template<>
	__forceinline void _setAll<true, ENUMMAP_SIZE_NATIVE>(T tValue)
	{
		std::fill_n(&m_InlineNative[0], numElements(), tValue);
	}*/ // advc.fract
	template<>
	__forceinline void _setAll<true, ENUMMAP_SIZE_1_BYTE>(T tValue)
	{
		std::fill_n(&m_Inline_1_byte[0], numElements(), tValue);
	}
	template<>
	__forceinline void _setAll<true, ENUMMAP_SIZE_2_BYTE>(T tValue)
	{
		std::fill_n(&m_Inline_2_byte[0], numElements(), tValue);
	}
	template<>
	__forceinline void _setAll<true, ENUMMAP_SIZE_BOOL>(T tValue)
	{
		std::fill_n(&m_InlineBoolArray[0], _getNumBoolBlocks<bINLINE>(), tValue ? MAX_UNSIGNED_INT : 0);
	}

	// allocate
	template<>
	void _allocate<false, ENUMMAP_SIZE_NATIVE>(T tValue)
	{
		FAssert(m_pArrayFull == NULL);
		m_pArrayFull = new T[numElements()];
		_setAll<bINLINE, SIZE>(tValue);
	}
	template<>
	void _allocate<false, ENUMMAP_SIZE_1_BYTE>(T tValue)
	{
		FAssert(m_pArrayChar == NULL);
		m_pArrayChar = new char[numElements()];
		_setAll<bINLINE, SIZE>(tValue);
	}
	template<>
	void _allocate<false, ENUMMAP_SIZE_2_BYTE>(T tValue)
	{
		FAssert(m_pArrayShort == NULL);
		m_pArrayShort = new short[numElements()];
		_setAll<bINLINE, SIZE>(tValue);
	}
	template<>
	void _allocate<false, ENUMMAP_SIZE_BOOL>(T tValue)
	{
		FAssert(m_pArrayBool == NULL);
		m_pArrayBool = new unsigned int[_getNumBoolBlocks<bINLINE>()];
		_setAll<bINLINE, SIZE>(tValue);
	}
	/*template<>
	void _allocate<true, ENUMMAP_SIZE_NATIVE>(T tValue)
	{
		FErrorMsg("EnumMap::_allocate shouldn't be called for classes with inline memory");
	}*/ // advc.fract
	template<>
	void _allocate<true, ENUMMAP_SIZE_1_BYTE>(T tValue)
	{
		FErrorMsg("EnumMap::_allocate shouldn't be called for classes with inline memory");
	}
	template<>
	void _allocate<true, ENUMMAP_SIZE_2_BYTE>(T tValue)
	{
		FErrorMsg("EnumMap::_allocate shouldn't be called for classes with inline memory");
	}
	template<>
	void _allocate<true, ENUMMAP_SIZE_BOOL>(T tValue)
	{
		FErrorMsg("EnumMap::_allocate shouldn't be called for classes with inline memory");
	}

	////
	//// bool specialization
	////

	template <>
	__forceinline unsigned int _getNumBoolBlocks<false>() const
	{
		return (numElements() + 31) / 32;
	}

	template <>
	__forceinline unsigned int _getNumBoolBlocks<true>() const
	{
		return NUM_BOOL_BLOCKS;
	}

	template<>
	void _Read<ENUMMAP_SIZE_BOOL>(/* <advc> */ FDataStreamBase* pStream, bool bAsInt, bool bAsShort,
		bool bAsFloat, bool bAsDouble/*, bool bSkipLast*/)
	{
		// (ignore bAs...)
		//IndexType const eLength = (IndexType)(getLength() - bSkipLast ? 1 : 0);
		for (IndexType eIndex = First(); eIndex < getLength(); ++eIndex)
		{
			bool bTmp;
			pStream->Read(&bTmp);
			set(eIndex, bTmp);
		} // </advc>
	}

	template<>
	void _Write<ENUMMAP_SIZE_BOOL>(/* <advc> */ FDataStreamBase* pStream, bool bAsInt,
		bool bAsFloat) const
	{
		// (ignore bAs...)
		for (IndexType eIndex = First(); eIndex < getLength(); ++eIndex)
			pStream->Write(get(eIndex));
		// </advc>
	}
};

//
// Functions
//
// They are all inline, though that doesn't mean they will get inlined in the resulting dll file as it's only a recommendation.
// The keyword inline serves two purposes. One is inlining small functions in the caller code,
// while the other is telling the linker that a function can be present in multiple object files.
// If the linker detects two identical inlined functions, it will merge them into one in the resulting dll file,
// like the code had been written in a cpp file and only present in one object file.
//
// The result is that the template functions are all compiled while we don't have to consider if they are compiled more than once.
// Maybe they will get inlined, but more likely some of them (particularly savegame code) are too big and will not be inlined.
//
// To actually force the compiler to inline, the keyword __forceinline can be used, but this one should really be used with care.
// Actually inlining functions can slow down the code and inline is usually only good for very small functions, like get variable.
//
/*  advc: MSVC03 seems to treat member functions as implicitly inline when it comes
	to the one-definition rule - even if the functions aren't defined within the
	class definition. So I've removed some inline keywords b/c I don't want to
	recommend any function with branching for inline expansion. I've also
	downgraded some __forceinline keywords to inline. */


template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
inline EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::EnumMapBase() : m_pArrayFull(NULL)
{
	// bools can only default to 0 or 1
	BOOST_STATIC_ASSERT(SIZE != ENUMMAP_SIZE_BOOL || DEFAULT == 0 || DEFAULT == 1);
	FAssertMsg(bINLINE_BOOL || sizeof(*this) == 4, "EnumMap is supposed to only contain a pointer");
	FAssertMsg(getLength() >= 0 && getLength() <= getEnumLength((LengthType)0), "Custom length out of range");
	FAssertMsg(First() >= 0 && First() <= getLength(), "Custom length out of range");

	if (bINLINE)
	{
		this->setAll((T)DEFAULT);
	}
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
inline EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::~EnumMapBase()
{
	reset();
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
__forceinline T EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::getDefault() const
{
	return (T)DEFAULT;
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
__forceinline IndexType EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::First() const
{
	//return ArrayStart((T_SUBSET)0);
	return (IndexType)0; // advc: Get rid of the ArrayStart function
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
__forceinline IndexType EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::getLength() const
{
	// advc: getEnumLength(T_SUBSET) returns a T_SUBSET value, so a cast is needed.
	return (IndexType)getEnumLength((T_SUBSET)0);
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
__forceinline IndexType EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::numElements() const
{
	// apparently subtracting two IndexTypes results in int, not IndexType
	return (IndexType)(getLength() - First());
}


template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
__forceinline T EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::get(IndexType eIndex) const
{
	FAssert(eIndex >= First() && eIndex < getLength());
	return _get<bINLINE, SIZE>(eIndex - First());
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
// advc: was inline
void EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::set(IndexType eIndex, T tValue)
{
	FAssert(eIndex >= First() && eIndex < getLength());
	if (!bINLINE && m_pArrayFull == NULL)
	{
		if (tValue == DEFAULT)
		{
			return;
		}
		_allocate<bINLINE_BOOL, SIZE>();
	}
	_set<bINLINE, SIZE>(eIndex - First(), tValue);
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
// advc: was inline
void EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::add(IndexType eIndex, T tValue)
{
	BOOST_STATIC_ASSERT(SIZE != ENUMMAP_SIZE_BOOL);
	FAssert(eIndex >= First() && eIndex < getLength());
	if (tValue != 0)
	{
		set(eIndex, tValue + get(eIndex));
	}
}

// advc:
template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
inline void EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::multiply(IndexType eIndex, T tMultiplier)
{
	/*  In 'add', I expect that branching on tValue is better than risking a cache miss,
		but tMultiplier==1 in 'multiply' seems far less likely than tValue==0 in 'add'. */
	//if (tMultiplier != 1)
	set(eIndex, tMultiplier * get(eIndex));
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
void EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
// advc: was inline
::safeSet(IndexType eIndex, T tValue)
{
	BOOST_STATIC_ASSERT(SIZE != ENUMMAP_SIZE_BOOL);
	if (eIndex >= First() && eIndex < getLength())
	{
		set(eIndex, tValue);
	}
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
void EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
// advc: was inline
::safeAdd(IndexType eIndex, T tValue)
{
	BOOST_STATIC_ASSERT(SIZE != ENUMMAP_SIZE_BOOL);
	if (eIndex >= First() && eIndex < getLength())
	{
		add(eIndex, tValue);
	}
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
// advc: was inline
void EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::addAll(T tValue)
{
	if (tValue != 0)
	{
		for (IndexType eIndex = First(); eIndex < getLength(); ++eIndex)
		{
			add(eIndex, (T)tValue);
		}
	}
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
// advc: was inline; advc.fract: return type was int
T EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>::getTotal() const
{
	// bINLINE is set at compile time and if true, isAllocated will always be true
	// used here to tell the compiler that the true statement (not allocated) can be declared unreachable at compile time
	if (!bINLINE && !isAllocated())
	{
		// no need to loop through unallocated memory
		return DEFAULT * getLength();
	}
	T tReturnVal = 0;
	const int iLength = getLength();
	for (IndexType eIndex = First(); eIndex < iLength; ++eIndex)
	{
		tReturnVal = tReturnVal + get(eIndex);
	}
	return tReturnVal;
}

/*	advc.fract: Replacing getTotal for bool; might also have other uses.
	The support of a vector is the set of nonzero elements.
	This function returns the cardinality of that set. */
template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
int EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>::getSupportSz() const
{
	if (!bINLINE && !isAllocated())
		return (DEFAULT == 0 ? 0 : getLength());
	int iReturnVal = 0;
	const int iLength = getLength();
	for (IndexType eIndex = First(); eIndex < iLength; ++eIndex)
	{
		if (get(eIndex) != 0)
			iReturnVal++;
	}
	return iReturnVal;
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
inline bool EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::isAllocated() const
{
	return bINLINE || m_pArrayFull != NULL;
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
// advc: inline removed
bool EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::hasContent() const
{
	if (bINLINE_BOOL)
	{
		for (int i = 0; i < NUM_BOOL_BLOCKS; ++i)
		{
			if (m_InlineBoolArray[i] != BOOL_BLOCK_DEFAULT)
			{
				return true;
			}
		}
		return false;
	}
	if (bINLINE || m_pArrayFull != NULL)
	{
		if (SIZE == ENUMMAP_SIZE_BOOL)
		{
			FAssertMsg(m_pArrayBool != NULL, "reset not called?"); // advc
			const int iNumBlocks = _getNumBoolBlocks<bINLINE_BOOL>();
			for (int i = 0; i < iNumBlocks; ++i)
			{	// advc: was m_pArrayBool
				if (m_InlineBoolArray[i] != BOOL_BLOCK_DEFAULT)
				{
					return true;
				}
			}
		}
		else
		{
			for (IndexType eIndex = (IndexType)0; eIndex < numElements(); ++eIndex)
			{
				if (get(eIndex) != DEFAULT)
				{
					return true;
				}
			}
		}
		// now we cheat and alter data despite being const.
		// We just detected all data to be of the default value, meaning the array is not used.
		// Release the data to save memory. It won't change how the outside world view the EnumMap.
		if (!bINLINE)
		{
			(const_cast <EnumMapBase*> (this))->reset();
		}
	}
	return false;
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
// advc: was inline
T EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::getMin() const
{
	BOOST_STATIC_ASSERT(false); // implementation isn't generic yet
	BOOST_STATIC_ASSERT(SIZE != ENUMMAP_SIZE_BOOL);
	if (m_pArray == NULL)
	{
		return DEFAULT;
	}
	return (T)(*std::min_element(m_pArray, m_pArray + numElements()));
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
// advc: was inline
T EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::getMax() const
{
	BOOST_STATIC_ASSERT(false); // implementation isn't generic yet
	BOOST_STATIC_ASSERT(SIZE != ENUMMAP_SIZE_BOOL);
	if (m_pArray == NULL)
	{
		return DEFAULT;
	}
	return (T)(*std::max_element(m_pArray, m_pArray + numElements()));
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
void EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::keepMin(IndexType eIndex, T tValue)
{
	BOOST_STATIC_ASSERT(SIZE != ENUMMAP_SIZE_BOOL);
	FAssert(eIndex >= First() && eIndex < getLength());
	if (get(eIndex) > tValue)
	{
		set(eIndex, tValue);
	}
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
// advc: was inline
void EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::keepMax(IndexType eIndex, T tValue)
{
	BOOST_STATIC_ASSERT(SIZE != ENUMMAP_SIZE_BOOL);
	FAssert(eIndex >= First() && eIndex < getLength());
	if (get(eIndex) < tValue)
	{
		set(eIndex, tValue);
	}
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
// advc: was inline
void EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::reset()
{
	if (bINLINE)
	{
		// can't free inlined memory. Set it all to default instead
		setAll((T)DEFAULT);
	}
	else
	{
		// doesn't matter which one we free. They all point to the same memory address, which is what matters here.
		SAFE_DELETE_ARRAY(m_pArrayFull);
	}
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
// advc: was inline
void EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::setAll(T tValue)
{
	if (!bINLINE && m_pArrayChar == NULL)
	{
		if (tValue == DEFAULT)
		{
			return;
		}
		_allocate<bINLINE, SIZE>(tValue);
	}
	else
	{
		_setAll<bINLINE, SIZE>(tValue);
	}
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
inline void EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::Read(/* <advc> */ FDataStreamBase* pStream, bool bAsInt, bool bAsShort,
	bool bAsFloat, bool bAsDouble/*, bool bSkipLast*/)
{
	_Read<SIZE>(pStream, bAsInt, bAsShort, bAsFloat, bAsDouble/*, bSkipLast*/);
}

/*template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
inline void EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::ReadButOne(FDataStreamBase* pStream, bool bAsInt, bool bAsFloat, bool bAsDouble)
{
	Read(pStream, bAsInt, false, bAsFloat, bAsDouble, true);
}*/

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
inline void EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::ReadFloat(FDataStreamBase* pStream, bool bAsDouble)
{
	Read(pStream, false, false, true, bAsDouble);
} // </advc>

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
inline void EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::Write(/* <advc> */ FDataStreamBase* pStream, bool bAsInt, bool bAsFloat) const
{
	_Write<SIZE>(pStream, bAsInt, bAsFloat); // </advc>
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
template<int SIZE2>
// advc: was inline
void EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::_Read(/* <advc> */ FDataStreamBase* pStream, bool bAsInt, bool bAsShort, bool bAsFloat,
	bool bAsDouble/*, bool bSkipLast*/) // </advc>
{
	BOOST_STATIC_ASSERT(SIZE == SIZE2);
	BOOST_STATIC_ASSERT(SIZE != ENUMMAP_SIZE_BOOL);
	// <advc>
	//IndexType const eLength = (IndexType)(getLength() - bSkipLast ? 1 : 0);
	if (bAsDouble)
	{
		for (IndexType eIndex = First(); eIndex < getLength(); ++eIndex)
		{
			double dTmp;
			pStream->Read(&dTmp);
			float fTmp = static_cast<float>(dTmp);
			/*  Seems to be the only way to get the compiler to accept this cast.
				bAsDouble and bAsFloat must of course only be used when T=float.
				The problem will go away when adopting the WtP SavegameReader. */
			set(eIndex, *(T*)((void*)&fTmp));
		}
		return;
	}
	if (bAsFloat)
	{
		for (IndexType eIndex = First(); eIndex < getLength(); ++eIndex)
		{
			float fTmp;
			pStream->Read(&fTmp);
			set(eIndex, *(T*)((void*)&fTmp));
		}
		return;
	}
	if (!bAsInt)
	{
		if (!bAsShort && (SIZE == ENUMMAP_SIZE_1_BYTE || (SIZE == ENUMMAP_SIZE_NATIVE &&
			sizeof(T) == 1)))
		{
			for (IndexType eIndex = First(); eIndex < getLength(); ++eIndex)
			{
				char cTmp;
				pStream->Read(&cTmp);
				set(eIndex, static_cast<T>(cTmp));
			}
			return;
		}
		if (bAsShort || SIZE == ENUMMAP_SIZE_2_BYTE || (SIZE == ENUMMAP_SIZE_NATIVE &&
			sizeof(T) == 2))
		{
			for (IndexType eIndex = First(); eIndex < getLength(); ++eIndex)
			{
				short sTmp;
				pStream->Read(&sTmp);
				set(eIndex, static_cast<T>(sTmp));
			}
			return;
		}
	}
	for (IndexType eIndex = First(); eIndex < getLength(); ++eIndex)
	{
		int iTmp;
		pStream->Read(&iTmp);
		set(eIndex, static_cast<T>(iTmp));
	} // <advc>
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
template<int SIZE2>
// advc: was inline
void EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::_Write(/* advc: */ FDataStreamBase* pStream, bool bAsInt, bool bAsFloat) const
{
	BOOST_STATIC_ASSERT(SIZE == SIZE2);
	BOOST_STATIC_ASSERT(SIZE != ENUMMAP_SIZE_BOOL);
	// <advc>
	if (bAsFloat)
	{
		for (IndexType eIndex = First(); eIndex < getLength(); ++eIndex)
			pStream->Write(static_cast<float>(get(eIndex)));
		return;
	}
	if (!bAsInt)
	{
		if (SIZE == ENUMMAP_SIZE_1_BYTE || (SIZE == ENUMMAP_SIZE_NATIVE &&
			sizeof(T) == 1))
		{
			for (IndexType eIndex = First(); eIndex < getLength(); ++eIndex)
				pStream->Write(static_cast<char>(get(eIndex)));
			return;
		}
		if (SIZE == ENUMMAP_SIZE_2_BYTE || (SIZE == ENUMMAP_SIZE_NATIVE &&
			sizeof(T) == 2))
		{
			for (IndexType eIndex = First(); eIndex < getLength(); ++eIndex)
				pStream->Write(static_cast<short>(get(eIndex)));
			return;
		}
	}
	for (IndexType eIndex = First(); eIndex < getLength(); ++eIndex)
		pStream->Write(static_cast<int>(get(eIndex))); // </advc>
}

//
// operator overloads
//

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
// advc: was inline
EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>&
EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::operator=(const EnumMapBase &rhs)
{
	BOOST_STATIC_ASSERT(false); // implementation isn't generic yet
	BOOST_STATIC_ASSERT(SIZE != ENUMMAP_SIZE_BOOL);
	if (rhs.isAllocated())
	{
		if (m_pArrayFull == NULL) allocate();
		memcpy(m_pArrayFull, rhs.m_pArrayFull, numElements() * SIZE_OF_T);
	}
	else
	{
		reset();
	}

	return *this;
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
template<class T2, int DEFAULT2>
// advc: was inline
EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>&
EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::operator=(const EnumMapBase<IndexType, T2, DEFAULT2, T_SUBSET, LengthType> &rhs)
{
	BOOST_STATIC_ASSERT(false); // implementation isn't generic yet
	BOOST_STATIC_ASSERT(SIZE != ENUMMAP_SIZE_BOOL);
	if (rhs.isAllocated())
	{
		for (IndexType eIndex = First(); eIndex < getLength(); ++eIndex)
		{
			set(eIndex, rhs.get(eIndex));
		}
	}
	else
	{
		assignAll(DEFAULT2);
	}

	return *this;
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
template<class T2, int DEFAULT2>
// advc: was inline
EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>&
EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::operator+=(const EnumMapBase<IndexType, T2, DEFAULT2, T_SUBSET, LengthType> &rhs)
{
	BOOST_STATIC_ASSERT(false); // implementation isn't generic yet
	BOOST_STATIC_ASSERT(SIZE != ENUMMAP_SIZE_BOOL);
	if (rhs.isAllocated())
	{
		for (IndexType eIndex = First(); eIndex < getLength(); ++eIndex)
		{
			add(eIndex, rhs.get(eIndex));
		}
	}
	else if (DEFAULT2 != 0)
	{
		addAll(DEFAULT2);
	}

	return *this;
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
template<class T2, int DEFAULT2>
// advc: was inline
EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>&
EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::operator-=(const EnumMapBase<IndexType, T2, DEFAULT2, T_SUBSET, LengthType> &rhs)
{
	BOOST_STATIC_ASSERT(false); // implementation isn't generic yet
	BOOST_STATIC_ASSERT(SIZE != ENUMMAP_SIZE_BOOL);
	if (rhs.isAllocated())
	{
		for (IndexType eIndex = First(); eIndex < getLength(); ++eIndex)
		{
			add(eIndex, -rhs.get(eIndex));
		}
	}
	else if (DEFAULT2 != 0)
	{
		addAll(-DEFAULT2);
	}

	return *this;
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
template<class T2, int DEFAULT2>
// advc: was inline
bool EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::operator==(const EnumMapBase<IndexType, T2, DEFAULT2, T_SUBSET, LengthType> &rhs) const
{
	BOOST_STATIC_ASSERT(false); // implementation isn't generic yet
	BOOST_STATIC_ASSERT(SIZE != ENUMMAP_SIZE_BOOL);
	if (!rhs.isAllocated() && !isAllocated())
	{
		return DEFAULT == DEFAULT2;
	}

	if (SIZE == SIZE2 && SIZE_OF_T == SIZE_OF_T2 && rhs.isAllocated() && isAllocated())
	{
		return memcmp(m_pArrayChar, rhs.m_pArrayChar, getLength() * SIZE_OF_T) == 0;
	}

	for (IndexType eIndex = First(); eIndex < getLength(); ++eIndex)
	{
		if (get(eIndex) != rhs.get(eIndex))
		{
			return false;
		}
	}
	return true;
}

template<class IndexType, class T, int DEFAULT, class T_SUBSET, class LengthType>
template<class T2, int DEFAULT2>
// advc: was inline
bool EnumMapBase<IndexType, T, DEFAULT, T_SUBSET, LengthType>
::operator!=(const EnumMapBase<IndexType, T2, DEFAULT2, T_SUBSET, LengthType> &rhs) const
{
	BOOST_STATIC_ASSERT(false); // implementation isn't generic yet
	BOOST_STATIC_ASSERT(SIZE != ENUMMAP_SIZE_BOOL);
	if (!rhs.isAllocated() && !isAllocated())
	{
		return DEFAULT != DEFAULT2;
	}

	if (SIZE == SIZE2 && SIZE_OF_T == SIZE_OF_T2 && rhs.isAllocated() && isAllocated())
	{
		return memcmp(m_pArrayChar, rhs.m_pArrayChar, getLength() * SIZE_OF_T) != 0;
	}

	for (IndexType eIndex = First(); eIndex < getLength(); ++eIndex)
	{
		if (get(eIndex) == rhs.get(eIndex))
		{
			return false;
		}
	}
	return true;
}


//
//
// type setup
// most of this aims at being set up at compile time
//
//

__forceinline bool ArrayDefault(bool var) { return 0; } \
template <> struct EnumMapGetDefault<bool> \
{ \
enum { DEFAULT_VALUE = 0, SIZE = ENUMMAP_SIZE_BOOL, SIZE_OF_T = sizeof(char) }; \
};

#define SET_ARRAY_DEFAULT( X ) \
__forceinline X ArrayDefault( X var) { return 0; } \
template <> struct EnumMapGetDefault< X > \
{ \
	enum { DEFAULT_VALUE = 0, SIZE = ENUMMAP_SIZE_NATIVE, SIZE_OF_T = sizeof(X) }; \
};

SET_ARRAY_DEFAULT(int);
SET_ARRAY_DEFAULT(short);
SET_ARRAY_DEFAULT(char);
SET_ARRAY_DEFAULT(unsigned int);
SET_ARRAY_DEFAULT(unsigned short);
SET_ARRAY_DEFAULT(byte);
SET_ARRAY_DEFAULT(float); // advc
// <advc.fract> (Can't pass template params into SET_ARRAY_DEFAULT)
template<int iSCALE, typename IntType, typename EnumType>
__forceinline ScaledNum<iSCALE,IntType,EnumType> ArrayDefault(ScaledNum<iSCALE,IntType,EnumType> var) { return 0; }
template<int iSCALE, typename IntType, typename EnumType>
struct EnumMapGetDefault<ScaledNum<iSCALE,IntType,EnumType> >
{
	enum { DEFAULT_VALUE = 0, SIZE = ENUMMAP_SIZE_NATIVE, SIZE_OF_T = sizeof(ScaledNum<iSCALE,IntType,EnumType>) };
}; // </advc.fract>

/*  advc: COMPILE_NUM_TYPES param removed; use NUM_ENUM_TYPES macro instead.
	JITarrayType accessor, ArrayStart and ArrayLength functions removed.
	Renamed the macro from SET_ARRAY_XML_ENUM to SET_XML_ENUM_SIZE.
	And some minor changes for compatibility with the DO_FOR_EACH... macro. */
#define SET_XML_ENUM_SIZE(Name, INFIX) \
	template <> struct EnumMapGetDefault<Name##Types> \
{ \
	enum { \
		DEFAULT_VALUE = -1, \
		SIZE = NUM_ENUM_TYPES(INFIX) > MAX_CHAR ? ENUMMAP_SIZE_2_BYTE : ENUMMAP_SIZE_1_BYTE, \
		SIZE_OF_T = SIZE, \
		MAX_LENGTH = NUM_ENUM_TYPES(INFIX), \
	}; \
};

// Byte size is set in enums
// If the length isn't known at compile time, 2 is assumed.
// Setting the byte size means say PlayerTypes will use 1 byte instead of 4. Works because MAX_PLAYERS <= 0x7F
// <advc>
DO_FOR_EACH_STATIC_INFO_TYPE(SET_XML_ENUM_SIZE)
SET_XML_ENUM_SIZE(AreaAI, AREAAI)

#define SET_XML_ENUM_SIZE1(Name, Dummy) \
	template <> struct EnumMapGetDefault<Name##Types> \
	{ \
		enum \
		{ \
			DEFAULT_VALUE = -1, SIZE = ENUMMAP_SIZE_1_BYTE, SIZE_OF_T = SIZE, \
			MAX_LENGTH = MAX_CHAR \
		}; \
	};
DO_FOR_EACH_SMALL_DYN_INFO_TYPE(SET_XML_ENUM_SIZE1)
// Not included in the list above:
SET_XML_ENUM_SIZE1(WorldSize, Dummy)
SET_XML_ENUM_SIZE1(Flavor, Dummy)
SET_XML_ENUM_SIZE1(Direction, Dummy)
SET_XML_ENUM_SIZE1(WarPlan, Dummy)
SET_XML_ENUM_SIZE1(CityPlot, Dummy)
SET_XML_ENUM_SIZE1(ArtStyle, Dummy)
SET_XML_ENUM_SIZE1(Feat, Dummy)
SET_XML_ENUM_SIZE1(PlayerVote, Dummy)

/*  2 being the default apparently does not mean that these can be omitted
	(Tbd.: There should be some way to get rid of SET_XML_ENUM_SIZE2.) */
#define SET_XML_ENUM_SIZE2(Name, Dummy) \
	template <> struct EnumMapGetDefault<Name##Types> \
	{ \
		enum \
		{ \
			DEFAULT_VALUE = -1, SIZE = ENUMMAP_SIZE_2_BYTE, SIZE_OF_T = SIZE, \
			MAX_LENGTH = MAX_SHORT \
		}; \
	};
DO_FOR_EACH_BIG_DYN_INFO_TYPE(SET_XML_ENUM_SIZE2)
/*	The corresponding getEnumLength function is inlined in CvMap.h.
	2 byte allows for at most 256*128 tiles (or 181*181).
	Larger maps really aren't playable, but let's allow them when
	the player limit has been increased. */
#if MAX_CIV_PLAYERS <= 18
SET_XML_ENUM_SIZE2(PlotNum, Dummy)
#else
template<> struct EnumMapGetDefault<PlotNumTypes> {
	enum {
		DEFAULT_VALUE = -1, SIZE = ENUMMAP_SIZE_NATIVE, SIZE_OF_T = SIZE,
		MAX_LENGTH = MAX_SHORT
	};
};
#endif

// The other getEnumLength functions are generated through macros in CvEnums.h
#define SET_NONXML_ENUM_LENGTH(TypeName, eLength) \
	__forceinline TypeName getEnumLength(TypeName) { return eLength; } \
	template <> struct EnumMapGetDefault<TypeName> \
	{ \
		enum { \
			DEFAULT_VALUE = -1, \
			SIZE = eLength > MAX_CHAR ? ENUMMAP_SIZE_2_BYTE : ENUMMAP_SIZE_1_BYTE, \
			SIZE_OF_T = SIZE, \
			MAX_LENGTH = eLength, \
		}; \
	};

SET_NONXML_ENUM_LENGTH(PlayerTypes, MAX_PLAYERS)
SET_NONXML_ENUM_LENGTH(TeamTypes, MAX_TEAMS)
// For enum maps that exclude the Barbarians
enum CivPlayerTypes {
	NUM_CIV_PLAYER_TYPES = MAX_CIV_PLAYERS
};
enum CivTeamTypes {
	NUM_CIV_TEAM_TYPES = MAX_CIV_TEAMS,
};
SET_NONXML_ENUM_LENGTH(CivPlayerTypes, NUM_CIV_PLAYER_TYPES)
SET_NONXML_ENUM_LENGTH(CivTeamTypes, NUM_CIV_TEAM_TYPES)

// Not to be used outside of this header
#undef SET_XML_ENUM_SIZE
#undef SET_XML_ENUM_SIZE1
#undef SET_XML_ENUM_SIZE2
#undef SET_ARRAY_DEFAULT
#undef SET_NONXML_ENUM_LENGTH
// </advc>


//
// List of various types of EnumMaps
// In most cases it's not nice code to include all parameters from EnumMapBase.
// Adding other classes, which always sets the default makes it easier to add EnumMaps as arguments to functions etc.
//

// The different classes:
// EnumMap:
///  the default, which takes a length (xml file) and type to store. Use this one as much as possible.
// EnumMapDefault:
///  same as EnumMap, but you can set what the default value should be (read: the value assigned by constructor/reset)
///  Note: indexes at default value aren't saved, hence saving an EnumMap with lots of default values will take less file space
// EnumMapInt:
///  Allows the index to be set by int instead of the enum type
///  Do not use if it can be avoided. The tradeoff of easier coding is that the compiler can no longer catch bugs.
///  The index arguments are consistently set to require the correct enum types specifically to catch bugs where arguments are switched etc.
///  Using EnumMapInt can easily end up with the compiler accepting swapped arguments etc.
///  For this reason, only use this if you know you have to typecast anyway for each call,
///  like when using CityPlotTypes for length [advc: EnumMap<IndexType,CityPlotTypes> shouldn't require casts in AdvCiv].

template<class IndexType, class T, int DEFAULT>
class EnumMapDefault : public EnumMapBase <IndexType, T, DEFAULT> {};

template<class IndexType, class T>
class EnumMap : public EnumMapBase <IndexType, T, EnumMapGetDefault<T>::DEFAULT_VALUE> {};

template<class IndexType, class T, int DEFAULT = 0>
class EnumMapInt : public EnumMapBase <int, T, DEFAULT, IndexType, IndexType> {};

// <advc> (Will have to explicitly set DEFAULT=-1 when T is an enum type)
template<class T, int DEFAULT = 0>
class CivPlayerMap : public EnumMapBase <PlayerTypes, T, DEFAULT, CivPlayerTypes, CivPlayerTypes> {};
template<class T, int DEFAULT = 0>
class CivTeamMap : public EnumMapBase <TeamTypes, T, DEFAULT, CivTeamTypes, CivTeamTypes> {};
// </advc>

typedef EnumMap<CivicOptionTypes,CivicTypes> CivicMap; // advc: Needed rather frequently

#endif
