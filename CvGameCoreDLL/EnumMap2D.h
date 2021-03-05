// advc.enum: From the "We the People" mod (Nightinggale). See EnumMap.h

#pragma once

#ifndef ENUM_MAP_2D_H
#define ENUM_MAP_2D_H

#include "EnumMap.h"


template<class OuterArrayType, class InnerArrayType, class T, int DEFAULT>
class EnumMap2DDefault
{
public:
	EnumMap2DDefault();
	~EnumMap2DDefault();

	OuterArrayType First() const;
	OuterArrayType Length() const;

	T get(OuterArrayType eArray, InnerArrayType eIndex) const;
	void set(OuterArrayType eArray, InnerArrayType eIndex, T tValue);
	// advc: Allow individual elements to be reset
	__forceinline void reset(OuterArrayType eArray, InnerArrayType eIndex)
	{
		set(eArray, eIndex, (T)DEFAULT);
	}
	void add(OuterArrayType eArray, InnerArrayType eIndex, T tValue);

	bool hasContent() const;

	void reset();

	void Read(/* <advc> */ FDataStreamBase* pStream, bool bAsInt = true, bool bLazy = false);
	void Write(/* <advc> */ FDataStreamBase* pStream, bool bAsInt = true, bool bLazy = false) const;

private:
	void allocate();

	EnumMapDefault<InnerArrayType, T, DEFAULT> * m_pOuterArray;

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
		OuterArrayType first;
		OuterArrayType last;

		interval() : first((OuterArrayType)0), last((OuterArrayType)0) {}
	};
};

template<class OuterArrayType, class InnerArrayType, class T, int DEFAULT>
EnumMap2DDefault<OuterArrayType, InnerArrayType, T, DEFAULT>
::EnumMap2DDefault() : m_pOuterArray(NULL) {}

template<class OuterArrayType, class InnerArrayType, class T, int DEFAULT>
EnumMap2DDefault<OuterArrayType, InnerArrayType, T, DEFAULT>
::~EnumMap2DDefault()
{
	reset();
}

template<class OuterArrayType, class InnerArrayType, class T, int DEFAULT>
__forceinline OuterArrayType EnumMap2DDefault<OuterArrayType, InnerArrayType, T, DEFAULT>
::First() const
{
	return (OuterArrayType)0;
}

template<class OuterArrayType, class InnerArrayType, class T, int DEFAULT>
__forceinline OuterArrayType EnumMap2DDefault<OuterArrayType, InnerArrayType, T, DEFAULT>
::Length() const
{
	return getEnumLength((OuterArrayType)0); // advc.enum: was ArrayLength
}

template<class OuterArrayType, class InnerArrayType, class T, int DEFAULT>
inline T EnumMap2DDefault<OuterArrayType, InnerArrayType, T, DEFAULT>
::get(OuterArrayType eArray, InnerArrayType eIndex) const
{
	FAssert(eArray >= 0 && eArray < Length());
	return m_pOuterArray ? m_pOuterArray[eArray].get(eIndex) : (T)DEFAULT;
}

template<class OuterArrayType, class InnerArrayType, class T, int DEFAULT>
inline void EnumMap2DDefault<OuterArrayType, InnerArrayType, T, DEFAULT>
::set(OuterArrayType eArray, InnerArrayType eIndex, T tValue)
{
	FAssert(eArray >= 0 && eArray < Length());
	if (m_pOuterArray == NULL)
	{
		if (tValue == DEFAULT) return;
		allocate();
	}
	m_pOuterArray[eArray].set(eIndex, tValue);
}

template<class OuterArrayType, class InnerArrayType, class T, int DEFAULT>
inline void EnumMap2DDefault<OuterArrayType, InnerArrayType, T, DEFAULT>
::add(OuterArrayType eArray, InnerArrayType eIndex, T tValue)
{
	FAssert(eArray >= 0 && eArray < Length());
	if (tValue == 0)
	{
		return;
	}
	
	if (m_pOuterArray == NULL)
	{
		allocate();
	}
	m_pOuterArray[eArray].add(eIndex, tValue);
}

template<class OuterArrayType, class InnerArrayType, class T, int DEFAULT>
inline bool EnumMap2DDefault<OuterArrayType, InnerArrayType, T, DEFAULT>
::hasContent() const
{
	if (m_pOuterArray)
	{
		for (OuterArrayType eArray = First(); eArray < Length(); ++eArray)
		{
			if (m_pOuterArray[eArray].hasContent())
			{
				return true;
			}
		}
		// now we cheat and alter data despite being const.
		// We just detected all data to be of the default value, meaning the array is not used.
		// Release the data to save memory. It won't change how the outside world view the EnumMap.
		(const_cast <EnumMap2DDefault*> (this))->reset();
	}

	return false;
}

template<class OuterArrayType, class InnerArrayType, class T, int DEFAULT>
inline void EnumMap2DDefault<OuterArrayType, InnerArrayType, T, DEFAULT>
::reset()
{
	SAFE_DELETE_ARRAY(m_pOuterArray);
}

template<class OuterArrayType, class InnerArrayType, class T, int DEFAULT>
inline void EnumMap2DDefault<OuterArrayType, InnerArrayType, T, DEFAULT>
::allocate()
{
	FAssert(m_pOuterArray == NULL);
	m_pOuterArray = new EnumMapDefault<InnerArrayType, T, DEFAULT>[Length()];
}

template<class OuterArrayType, class InnerArrayType, class T, int DEFAULT>
inline void EnumMap2DDefault<OuterArrayType, InnerArrayType, T, DEFAULT>
::Read(/* <advc> */ FDataStreamBase* pStream, bool bAsInt, bool bLazy)
{
	if (m_pOuterArray == NULL)
		allocate();
	for (OuterArrayType eArray = First(); eArray < Length(); ++eArray)
	{
		if (bLazy)
		{
			int iCount;
			pStream->Read(&iCount);
			if (iCount <= 0)
				continue;
		}
		m_pOuterArray[eArray].Read(pStream, bAsInt);
	}
	hasContent(); // </advc>
}


template<class OuterArrayType, class InnerArrayType, class T, int DEFAULT>
inline void EnumMap2DDefault<OuterArrayType, InnerArrayType, T, DEFAULT>
::Write(/* <advc> */ FDataStreamBase* pStream, bool bAsInt, bool bLazy) const
{
	if (m_pOuterArray == NULL)
		const_cast<EnumMap2DDefault*>(this)->allocate();
	for (OuterArrayType eArray = First(); eArray < Length(); ++eArray)
	{
		if (bLazy)
		{
			if(!m_pOuterArray[eArray].hasContent())
			{
				pStream->Write(0);
				continue;
			}
			else pStream->Write(static_cast<int>(m_pOuterArray[eArray].getLength()));
		}
		m_pOuterArray[eArray].Write(pStream, bAsInt);
	}
	hasContent(); // </advc>
}


template<class OuterArrayType, class InnerArrayType, class T>
class EnumMap2D : public EnumMap2DDefault < OuterArrayType, InnerArrayType, T, EnumMapGetDefault<T>::DEFAULT_VALUE > {};

#endif
