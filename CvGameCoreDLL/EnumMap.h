/*  advc.enum: From the "We the People" (WtP) mod for Civ4Col, original author: Nightinggale,
	who is still working on the EnumMap classes. This version is from 8 Oct 2019.
	I have -for now- omitted the WtP serialization functions, and uncoupled the
	code from the Perl-generated enums that WtP uses. Instead of defining
	ArrayLength functions, the getEnumLength functions that AdvCiv defines in
	CvEnums.h and CvGlobals.h are used.
	Formatting: linebreaks added before scope resolution operators. */

#ifndef ENUM_MAP_H
#define ENUM_MAP_H
#pragma once

//
// EnumMap is a special case of map where there is a <key,value> pair for each key in an enum.
// https://docs.oracle.com/en/java/javase/13/docs/api/java.base/java/util/EnumMap.html
//
// Put in civ4 terms, it's an array with a fixed length and automated memory management.
// The length is either the length of an xml file (like UnitTypes) or a fixed value (like MAX_PLAYERS)
//
// The array is set up with template parameters as this allows "arguments" to the default constructor.
// Often an array of arrays can only use the default constructor for the inner array, making flexible default
// constructors important.
//
// In most cases only the first two parameters need to be set. The last two only benefit from non-default values
// in very special cases.
//
// Template parameters:
//    LengthType: the type, which is used for index and array size.
//       For instance PlayerTypes means get will require a PlayerTypes argument and the array size is MAX_PLAYERS.
//
//    T: the data stored in the array. This is usually of the int family, but it can also be enums.
//
//    DEFAULT: (optional) sets the value the array will init to
//      If not mentioned, the value will be set by which type T is. 0 for int family and -1 (NO_) for enums
//      Note: using default and calling setAll() is preferred. See below.
//
//    T_SUBSET: (optional) Sets the range of the array if a subset is needed.
//      If you have an LengthType with a length of 100, but you only need 70-75, setting a nondefault here,
//      can make the array contain 6 items, which is then accessed with indexes 70-75
//      Default value will use LengthType, meaning it goes from 0 to 
//
//
// Try to keep the different types of parameters to a minimum. Each time a new set is used, a new set is compiled.
// Example:
//   EnumMap<PlayerTypes, int> A;
//   EnumMap<PlayerTypes, int, 1> B;
// This will create two different functions, compile and include both into the DLL file.
//   EnumMap<PlayerTypes, int> A;
//   EnumMap<PlayerTypes, int> B;
//   B.setAll(1);
// Same result, but since they now share the same parameters, the compiler will only make one set, which they will both call.
// It's not a major issue to make multiple, partly because most calls are inline anyway, but it should be mentioned.

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
class EnumMapBase
{
public:
	EnumMapBase();
	~EnumMapBase();

	// const values (per class)
	T getDefault() const;
	LengthType First() const;
	LengthType getLength() const;
	LengthType numElements() const;

	// array access
	T get(LengthType eIndex) const;
	void set(LengthType eIndex, T eValue);
	void add(LengthType eIndex, T eValue);

	// add bound checks. Ignore call if out of bound index
	void safeSet(LengthType eIndex, T eValue);
	void safeAdd(LengthType eIndex, T eValue);

	// add a number to all indexes
	void addAll(T eValue);
	
	// Check if there is non-default contents.
	// isAllocated() test for a null pointer while hasContent() will loop the array to test each index for default value.
	// Useful to avoid looping all 0 arrays and when creating savegames.
	// Note: hasContent() can release memory if it doesn't alter what get() will return.
	bool isAllocated() const;
	bool hasContent() const;
	
	T getMin() const;
	T getMax() const;

	void keepMin(LengthType eIndex, T eValue);
	void keepMax(LengthType eIndex, T eValue);
	
	// memory allocation and freeing
	void reset();
	void setAll(T eValue);
private:
	void allocate();

public:
	void Read(/* advc: */ FDataStreamBase* pStream);
	void Write(/* advc: */ FDataStreamBase* pStream) const;

	// operator overload
	EnumMapBase& operator=(const EnumMapBase &rhs);
	
	template<class T2, int DEFAULT2, int SIZE2, int SIZE_OF_T2>
	EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>& operator  = (const EnumMapBase<LengthType, T2, DEFAULT2, T_SUBSET, SIZE2, SIZE_OF_T2> &rhs);
	template<class T2, int DEFAULT2, int SIZE2, int SIZE_OF_T2>
	EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>& operator += (const EnumMapBase<LengthType, T2, DEFAULT2, T_SUBSET, SIZE2, SIZE_OF_T2> &rhs);
	template<class T2, int DEFAULT2, int SIZE2, int SIZE_OF_T2>
	EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>& operator -= (const EnumMapBase<LengthType, T2, DEFAULT2, T_SUBSET, SIZE2, SIZE_OF_T2> &rhs);
	template<class T2, int DEFAULT2, int SIZE2, int SIZE_OF_T2>
	bool operator == (const EnumMapBase<LengthType, T2, DEFAULT2, T_SUBSET, SIZE2, SIZE_OF_T2> &rhs) const;
	template<class T2, int DEFAULT2, int SIZE2, int SIZE_OF_T2>
	bool operator != (const EnumMapBase<LengthType, T2, DEFAULT2, T_SUBSET, SIZE2, SIZE_OF_T2> &rhs) const;
	

private:

	union
	{
		T     * m_pArrayFull;
		short * m_pArrayShort;
		char  * m_pArrayChar;
	};

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
		LengthType first;
		LengthType last;

		interval()
		{
			first = (LengthType)0;
			last = (LengthType)0;
		}
	};
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


template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::EnumMapBase() : m_pArrayFull(NULL)
{
	FAssertMsg(sizeof(*this) == 4, "EnumMap is supposed to only contain a pointer");
	FAssertMsg(getLength() >= 0 && getLength() <= getEnumLength((LengthType)0), "Custom length out of range");
	FAssertMsg(First() >= 0 && First() <= getLength(), "Custom length out of range");
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::~EnumMapBase()
{
	reset();
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
__forceinline T EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::getDefault() const
{
	return (T)DEFAULT;
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
__forceinline LengthType EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::First() const
{
	//return ArrayStart((T_SUBSET)0);
	return (T_SUBSET)0; // advc: Get rid of the ArrayStart function
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
__forceinline LengthType EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::getLength() const
{
	return getEnumLength((T_SUBSET)0);
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
__forceinline LengthType EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::numElements() const
{
	// apparently subtracting two LengthTypes results in int, not LengthType
	return (LengthType)(getLength() - First());
}


template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline T EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::get(LengthType eIndex) const
{
	FAssert(eIndex >= First() && eIndex < getLength());

	switch (SIZE)
	{
	case 1:  return (T)(m_pArrayChar  ? m_pArrayChar [eIndex - First()] : DEFAULT);
	case 2:  return (T)(m_pArrayShort ? m_pArrayShort[eIndex - First()] : DEFAULT);
	default: return (T)(m_pArrayFull  ? m_pArrayFull [eIndex - First()] : DEFAULT);
	}
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline void EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::set(LengthType eIndex, T eValue)
{
	FAssert(eIndex >= First() && eIndex < getLength());
	if (m_pArrayFull == NULL)
	{
		if (eValue == DEFAULT) 
		{
			return;
		}
		allocate();
	}

	switch (SIZE)
	{
	case 1:  m_pArrayChar [eIndex - First()] = (char )eValue; break;
	case 2:  m_pArrayShort[eIndex - First()] = (short)eValue; break;
	default: m_pArrayFull [eIndex - First()] =        eValue; break;
	}
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline void EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::add(LengthType eIndex, T eValue)
{
	FAssert(eIndex >= First() && eIndex < getLength());
	if (eValue != 0)
	{
		set(eIndex, eValue + get(eIndex));
	}
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline void EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::safeSet(LengthType eIndex, T eValue)
{
	if (eIndex >= First() && eIndex < getLength())
	{
		set(eIndex, eValue);
	}
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline void EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::safeAdd(LengthType eIndex, T eValue)
{
	if (eIndex >= First() && eIndex < getLength())
	{
		add(eIndex, eValue);
	}
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline void EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::addAll(T eValue)
{
	if (eValue != 0)
	{
		for (LengthType eIndex = First(); eIndex < getLength(); ++eIndex)
		{
			add(eIndex, (T)eValue);
		}
	}
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline bool EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::isAllocated() const
{
	return m_pArrayFull != NULL;
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline bool EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::hasContent() const
{
	if (m_pArrayFull != NULL)
	{
		for (LengthType eIndex = (LengthType)0; eIndex < numElements(); ++eIndex)
		{
			if (get(eIndex) != DEFAULT)
			{
				return true;
			}
		}
		// now we cheat and alter data despite being const.
		// We just detected all data to be of the default value, meaning the array is not used.
		// Release the data to save memory. It won't change how the outside world view the EnumMap.
		(const_cast <EnumMapBase*> (this))->reset();
	}
	return false;
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline T EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::getMin() const
{
	if (m_pArray == NULL)
	{
		return DEFAULT;
	}
	return (T)(*std::min_element(m_pArray, m_pArray + numElements()));
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline T EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::getMax() const
{
	if (m_pArray == NULL)
	{
		return DEFAULT;
	}
	return (T)(*std::max_element(m_pArray, m_pArray + numElements()));
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline void EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::keepMin(LengthType eIndex, T eValue)
{
	FAssert(eIndex >= First() && eIndex < getLength());
	if (get(eIndex) > eValue)
	{
		set(eIndex, eValue);
	}
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline void EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::keepMax(LengthType eIndex, T eValue)
{
	FAssert(eIndex >= First() && eIndex < getLength());
	if (get(eIndex) < eValue)
	{
		set(eIndex, eValue);
	}
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline void EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::reset()
{
	// doesn't matter which one we free. They all point to the same memory address, which is what matters here.
	SAFE_DELETE_ARRAY(m_pArrayFull);
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline void EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::setAll(T eValue)
{
	if (m_pArrayChar == NULL)
	{
		if (eValue == DEFAULT)
		{
			return;
		}
		m_pArrayChar = new char[numElements() * SIZE_OF_T];
	}

	if (SIZE_OF_T == 1 || eValue == 0)
	{
		memset(m_pArrayFull, eValue, numElements() * SIZE_OF_T);
	}
	else if (SIZE == 2)
	{
		std::fill_n(m_pArrayShort, numElements(), eValue);
	}
	else
	{
		std::fill_n(m_pArrayFull, numElements(), eValue);
	}
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline void EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::allocate()
{
	FAssert(m_pArrayChar == NULL);

	m_pArrayChar = new char[numElements() * SIZE_OF_T];

	// memset is a whole lot faster. However it only works on bytes.
	// Optimize for the default case where default is 0.
	int const iElements = numElements(); // advc
	if (SIZE_OF_T == 1 || DEFAULT == 0)
	{
		memset(m_pArrayFull, DEFAULT, iElements * SIZE_OF_T);
	}
	else if (SIZE == 2)
	{
		std::fill_n(m_pArrayShort, iElements, (T)DEFAULT);
	}
	else
	{
		std::fill_n(m_pArrayFull, iElements, (T)DEFAULT);
	}
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline void EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::Read(/* <advc> */ FDataStreamBase* pStream)
{
	for (LengthType eIndex = First(); eIndex < getLength(); ++eIndex)
	{
		int iTmp;
		pStream->Read(&iTmp);
		set(eIndex, static_cast<T>(iTmp));
	} // </advc>
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline void EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::Write(/* <advc> */ FDataStreamBase* pStream) const
{
	for (LengthType eIndex = First(); eIndex < getLength(); ++eIndex)
		pStream->Write(static_cast<int>(get(eIndex))); // </advc>
}


//
// operator overloads
//

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T>
inline EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>& EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::operator=(const EnumMapBase &rhs)
{
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

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T> template<class T2, int DEFAULT2, int SIZE2, int SIZE_OF_T2>
inline EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>& EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::operator=(const EnumMapBase<LengthType, T2, DEFAULT2, T_SUBSET, SIZE2, SIZE_OF_T2> &rhs)
{
	if (rhs.isAllocated())
	{
		for (LengthType eIndex = First(); eIndex < getLength(); ++eIndex)
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

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T> template<class T2, int DEFAULT2, int SIZE2, int SIZE_OF_T2>
inline EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>& EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::operator+=(const EnumMapBase<LengthType, T2, DEFAULT2, T_SUBSET, SIZE2, SIZE_OF_T2> &rhs)
{
	if (rhs.isAllocated())
	{
		for (LengthType eIndex = First(); eIndex < getLength(); ++eIndex)
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

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T> template<class T2, int DEFAULT2, int SIZE2, int SIZE_OF_T2>
inline EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>& EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::operator-=(const EnumMapBase<LengthType, T2, DEFAULT2, T_SUBSET, SIZE2, SIZE_OF_T2> &rhs)
{
	if (rhs.isAllocated())
	{
		for (LengthType eIndex = First(); eIndex < getLength(); ++eIndex)
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

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T> template<class T2, int DEFAULT2, int SIZE2, int SIZE_OF_T2>
inline bool EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::operator==(const EnumMapBase<LengthType, T2, DEFAULT2, T_SUBSET, SIZE2, SIZE_OF_T2> &rhs) const
{
	if (!rhs.isAllocated() && !isAllocated())
	{
		return DEFAULT == DEFAULT2;
	}

	if (SIZE == SIZE2 && SIZE_OF_T == SIZE_OF_T2 && rhs.isAllocated() && isAllocated())
	{
		return memcmp(m_pArrayChar, rhs.m_pArrayChar, getLength() * SIZE_OF_T) == 0;
	}

	for (LengthType eIndex = First(); eIndex < getLength(); ++eIndex)
	{
		if (get(eIndex) != rhs.get(eIndex))
		{
			return false;
		}
	}
	return true;
}

template<class LengthType, class T, int DEFAULT, class T_SUBSET, int SIZE, int SIZE_OF_T> template<class T2, int DEFAULT2, int SIZE2, int SIZE_OF_T2>
inline bool EnumMapBase<LengthType, T, DEFAULT, T_SUBSET, SIZE, SIZE_OF_T>
::operator!=(const EnumMapBase<LengthType, T2, DEFAULT2, T_SUBSET, SIZE2, SIZE_OF_T2> &rhs) const
{
	if (!rhs.isAllocated() && !isAllocated())
	{
		return DEFAULT != DEFAULT2;
	}

	if (SIZE == SIZE2 && SIZE_OF_T == SIZE_OF_T2 && rhs.isAllocated() && isAllocated())
	{
		return memcmp(m_pArrayChar, rhs.m_pArrayChar, getLength() * SIZE_OF_T) != 0;
	}

	for (LengthType eIndex = First(); eIndex < getLength(); ++eIndex)
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
namespace // advc: anonymous namespace
{

template <class T>
struct EnumMapGetDefault
{
};

#define SET_ARRAY_DEFAULT( X ) \
__forceinline X ArrayDefault( X var) { return 0; } \
template <> struct EnumMapGetDefault<X> \
{ \
	enum { value = 0, SIZE = 0, SIZE_OF_T = sizeof(X) }; \
};

SET_ARRAY_DEFAULT(int);
SET_ARRAY_DEFAULT(short);
SET_ARRAY_DEFAULT(char);
SET_ARRAY_DEFAULT(unsigned int);
SET_ARRAY_DEFAULT(unsigned short);
SET_ARRAY_DEFAULT(byte);

/*  advc: VAR_SIZE param removed. Since NUM_TYPES is known at compile-time,
	SIZE can be derived from that. JITarrayType accessor, ArrayStart and ArrayLength
	functions removed. Renamed the macro from SET_ARRAY_XML_ENUM to SET_XML_ENUM_SIZE. */
#define SET_XML_ENUM_SIZE(Name, NUM_TYPES) \
template <> struct EnumMapGetDefault<Name> \
{ \
	enum { value = -1, SIZE = (NUM_TYPES < 128 ? 1 : 2), SIZE_OF_T = SIZE }; \
};

// Byte size is set in enums
// If the length isn't known at compile time, 2 is assumed.
// Setting the byte size means say PlayerTypes will use 1 byte instead of 4. Works because MAX_PLAYERS <= 0x7F

// (advc: Most lengths aren't known at compile time)
SET_XML_ENUM_SIZE(AreaAITypes, NUM_AREAAI_TYPES)
SET_XML_ENUM_SIZE(UnitAITypes, NUM_UNITAI_TYPES)
SET_XML_ENUM_SIZE(MemoryTypes, NUM_MEMORY_TYPES)
SET_XML_ENUM_SIZE(WorldSizeTypes, NUM_WORLDSIZE_TYPES)
SET_XML_ENUM_SIZE(YieldTypes, NUM_YIELD_TYPES)
SET_XML_ENUM_SIZE(CommerceTypes, NUM_COMMERCE_TYPES)
SET_XML_ENUM_SIZE(PlayerTypes, MAX_PLAYERS)
SET_XML_ENUM_SIZE(TeamTypes, MAX_TEAMS)

/*  <advc> Although the lengths aren't known to the compiler, I know that many types
	have fewer than 128 values and no reasonable XML change will change that. */
#define SET_XML_ENUM_SIZE1(Name) \
	template <> struct EnumMapGetDefault<Name##Types> \
	{ \
		enum { value = -1, SIZE = 1, SIZE_OF_T = SIZE }; \
	};
SET_XML_ENUM_SIZE1(SpecialBuilding)
SET_XML_ENUM_SIZE1(Civic)
SET_XML_ENUM_SIZE1(CivicOption)
SET_XML_ENUM_SIZE1(Climate)
SET_XML_ENUM_SIZE1(CultureLevel)
SET_XML_ENUM_SIZE1(Era)
SET_XML_ENUM_SIZE1(Emphasize)
SET_XML_ENUM_SIZE1(Feature)
SET_XML_ENUM_SIZE1(GameOption)
SET_XML_ENUM_SIZE1(GameSpeed)
SET_XML_ENUM_SIZE1(Goody)
SET_XML_ENUM_SIZE1(Handicap)
SET_XML_ENUM_SIZE1(Hurry)
SET_XML_ENUM_SIZE1(Improvement)
SET_XML_ENUM_SIZE1(Invisible)
SET_XML_ENUM_SIZE1(PlayerOption)
SET_XML_ENUM_SIZE1(Route)
SET_XML_ENUM_SIZE1(SeaLevel)
SET_XML_ENUM_SIZE1(Terrain)
SET_XML_ENUM_SIZE1(Trait)
SET_XML_ENUM_SIZE1(UnitCombat)
SET_XML_ENUM_SIZE1(SpecialUnit)
SET_XML_ENUM_SIZE1(Victory)

/*  2 being the default apparently does not mean that these can be omitted
	(Tbd.: There should be some way to get rid of SET_XML_ENUM_SIZE2.) */
#define SET_XML_ENUM_SIZE2(Name) \
	template <> struct EnumMapGetDefault<Name##Types> \
	{ \
		enum { value = -1, SIZE = 2, SIZE_OF_T = SIZE }; \
	};
SET_XML_ENUM_SIZE2(Bonus)
SET_XML_ENUM_SIZE2(LeaderHead)
SET_XML_ENUM_SIZE2(Event)
SET_XML_ENUM_SIZE2(EventTrigger)
SET_XML_ENUM_SIZE2(Build)
SET_XML_ENUM_SIZE2(Building)
SET_XML_ENUM_SIZE2(BuildingClass)
SET_XML_ENUM_SIZE2(Civilization)
SET_XML_ENUM_SIZE2(Color)
SET_XML_ENUM_SIZE2(PlayerColor)
SET_XML_ENUM_SIZE2(Promotion)
SET_XML_ENUM_SIZE2(Unit)
SET_XML_ENUM_SIZE2(UnitClass)
SET_XML_ENUM_SIZE2(Tech)

#define SET_NONXML_ENUM_LENGTH(TypeName, eLength) \
	__forceinline TypeName getEnumLength(TypeName) { return eLength; }
/*  Don't want to set these in CvEnums.h or anywhere in the global namespace b/c
	the FOR_EACH_ENUM macro shouldn't be used for them */
SET_NONXML_ENUM_LENGTH(PlayerTypes, (PlayerTypes)MAX_PLAYERS)
SET_NONXML_ENUM_LENGTH(TeamTypes, (TeamTypes)MAX_TEAMS)
} // End of anonymous namespace
// </advc>


//
// List of various types of EnumMaps
// In most cases it's not nice code to include all parameters from EnumMapBase.
// Adding other classes, which always sets the default makes it easier to add EnumMaps as arguments to functions etc.
//

template<class LengthType, class T, int DEFAULT>
class EnumMapDefault : public EnumMapBase <LengthType, T, DEFAULT, LengthType, EnumMapGetDefault<T>::SIZE, EnumMapGetDefault<T>::SIZE_OF_T > {};

template<class LengthType, class T>
class EnumMap : public EnumMapBase <LengthType, T, EnumMapGetDefault<T>::value, LengthType, EnumMapGetDefault<T>::SIZE, EnumMapGetDefault<T>::SIZE_OF_T > {};

#endif
