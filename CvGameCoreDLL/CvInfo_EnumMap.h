#pragma once

#ifndef CV_INFO_ENUM_MAP_H
#define CV_INFO_ENUM_MAP_H

class FDataStreamBase;

/*	advc.003t: Enum maps for storing key-value pairs loaded from XML,
	the key being of an enum type and the value of some integral type
	(float will also work I suppose). These maps, unlike those defined in EnumMap.h,
	never need to be modified after XML loading. Moreover, XML assigns a
	value only to a small subset of possible keys, and a default value -
	normally 0 - is assumed for the rest. Therefore, it's usually more efficient
	(cache performance, iteration over non-default values only) to store the map
	as a list of pairs in a ListEnumMap. For mappings with relatively
	few default values - or frequent random access -, there is ArrayEnumMap.
	The two maps have the same interface but no shared base class, i.e. the choice
	between them needs to be made at compile time. Would be nice to let the
	program decide based on the data loaded from XML, but that would add a lot of
	overhead for virtual function calls.
	ArrayEnumMap and ListEnumMap have counterparts EnumMap (from the
	"We the People" mod) and SparseEnumMap, both defined in EnumMap.h, for data
	that changes continuously. */

// Let's start, to motivate the iteration interface, with the for-each macro:
#define iANON_NON_DEFAULT_INDEX CONCATVARNAME(iAnonNonDefaultIndex_, __LINE__)
#define FOR_EACH_NON_DEFAULT_INFO_PAIR(kEnumMapSeq, EnumPrefix, ValueType) \
	int iANON_NON_DEFAULT_INDEX = 0; \
	for (std::pair<EnumPrefix##Types,ValueType> per##EnumPrefix##Val = \
		kEnumMapSeq.nextNonDefault(static_cast<EnumPrefix##Types>(-1), 0); \
		per##EnumPrefix##Val.first != non_default_enum_map::end; \
		per##EnumPrefix##Val = kEnumMapSeq.nextNonDefault( \
		per##EnumPrefix##Val.first, ++iANON_NON_DEFAULT_INDEX))
/*	Example:
FOR_EACH_NON_DEFAULT_INFO_PAIR(kBuilding.getUnitCombatFreeXP(), UnitCombat, int)
	expands to
int iAnonNonDefaultIndex_3947 = 0;
for (std::pair<UnitCombatTypes,int> perUnitCombatVal =
	kBuilding.getUnitCombatFreeXP().nextNonDefault(static_cast<UnitCombatTypes>(-1), 0);
	perUnitCombatVal.first != non_default_enum_map::end;
	perUnitCombatVal = kBuilding.getUnitCombatFreeXP().nextNonDefault(
	perUnitCombatVal.first, ++iAnonNonDefaultIndex_3947))*/
/*	(A sadness that the pair types can't be inferred from the map instance,
	but that's how it is. An iterator class wouldn't help with this problem either.) */
/*	Mainly for boolean value type. Could make this more efficient by adding
	nextNonDefaultKey functions (returning only the enum key). */
#define FOR_EACH_NON_DEFAULT_KEY(kEnumMapSeq, EnumPrefix) \
	int iANON_NON_DEFAULT_INDEX = 0; \
	for (EnumPrefix##Types eLoop##EnumPrefix = \
		kEnumMapSeq.nextNonDefault(static_cast<EnumPrefix##Types>(-1), 0).first; \
		eLoop##EnumPrefix != non_default_enum_map::end; \
		eLoop##EnumPrefix = kEnumMapSeq.nextNonDefault( \
		eLoop##EnumPrefix, ++iANON_NON_DEFAULT_INDEX).first)

// For the CvInfo_... header files
#define DEF_INFO_ENUM_MAP_HELPER(TagName, EnumPrefix, ValueType, \
		InfoMapTempl, vDefault, getterPrefix, PyValueType) \
	/* for the for-each macro above */ \
	InfoMapTempl<EnumPrefix##Types,ValueType,vDefault> const& getterPrefix##TagName() const \
	{ \
		return m_map##TagName; \
	} \
	private: \
	/* Non-const accessor for XML loading. */ \
	/* Can't have the same name as the public version above b/c the */ \
	/* compiler will assume that the (inaccessible) private version is meant */ \
	/* when the callee is non-const. */ \
	InfoMapTempl<EnumPrefix##Types,ValueType,vDefault>& TagName() \
	{ \
		return m_map##TagName; \
	} \
	/* data member declaration */ \
	InfoMapTempl<EnumPrefix##Types,ValueType,vDefault> m_map##TagName; \
	/* Python export (will have to prepend "py_" in CyInfoInterface*.cpp) */ \
	PY_WRAP(PyValueType, getterPrefix##TagName, EnumPrefix); \
	public: \
	/* random access */ \
	ValueType getterPrefix##TagName(EnumPrefix##Types e##EnumPrefix) const \
	{ \
		return m_map##TagName.get(e##EnumPrefix); \
	}
#define DEF_INFO_ENUM_MAP_DEFAULT(TagName, EnumPrefix, ValueType, InfoMapTempl, vDefault) \
	DEF_INFO_ENUM_MAP_HELPER(TagName, EnumPrefix, ValueType, InfoMapTempl, vDefault, get, int)
#define DEF_INFO_ENUM_MAP(TagName, EnumPrefix, ValueType, InfoMapTempl) \
	DEF_INFO_ENUM_MAP_DEFAULT(TagName, EnumPrefix, ValueType, InfoMapTempl, 0)
// Separate macros just so that the getter functions get named "is..."
#define DEF_INFO_ENUM_MAP_BOOL_DEFAULT(TagName, EnumPrefix, InfoMapTempl, vDefault) \
	DEF_INFO_ENUM_MAP_HELPER(TagName, EnumPrefix, bool, InfoMapTempl, vDefault, is, bool)
#define DEF_INFO_ENUM_MAP_BOOL(TagName, EnumPrefix, InfoMapTempl) \
	DEF_INFO_ENUM_MAP_BOOL_DEFAULT(TagName, EnumPrefix, InfoMapTempl, 0)

// Can't use the same macro for yield and commerce maps b/c those aren't class templates
#define DEF_SHORT_INFO_ENUM_MAP(TagName, EnumPrefix, ShortMapType) \
	/* for game text */ \
	ShortMapType const& get##TagName() const \
	{ \
		return m_map##TagName; \
	} \
	private: \
	ShortMapType& TagName() \
	{ \
		return m_map##TagName; \
	} \
	ShortMapType m_map##TagName; \
	PY_WRAP(int, get##TagName, EnumPrefix); \
	public: \
	ShortMapType::value_t get##TagName(EnumPrefix##Types e##EnumPrefix) const \
	{ \
		return m_map##TagName.get(e##EnumPrefix); \
	}

// Mappings to short (yield or commerce) maps
#define DEF_INFO_ENUM2SHORT_MAP(TagName, LongEnumPrefix, ShortEnumPrefix, \
		ShortMapType, InfoMapTempl) \
	/* for iteration over short maps that have non-default values */ \
	InfoMapTempl<LongEnumPrefix##Types,ShortMapType::enc_t> const& get##TagName() const \
	{ \
		return m_map##TagName; \
	} \
	private: \
	InfoMapTempl<LongEnumPrefix##Types,ShortMapType::enc_t>& TagName() \
	{ \
		return m_map##TagName; \
	} \
	InfoMapTempl<LongEnumPrefix##Types,ShortMapType::enc_t> m_map##TagName; \
	public: \
	ShortMapType get##TagName(LongEnumPrefix##Types e##LongEnumPrefix) const \
	{ \
		return ShortMapType(m_map##TagName.get(e##LongEnumPrefix)); \
	} \
	private: \
	PY_WRAP_2D(ShortMapType::value_t, get##TagName, LongEnumPrefix, ShortEnumPrefix) \
	public: \
	ShortMapType::value_t get##TagName( \
		LongEnumPrefix##Types e##LongEnumPrefix, \
		ShortEnumPrefix##Types e##ShortEnumPrefix \
		) const \
	{ \
		return ShortMapType(m_map##TagName.get(e##LongEnumPrefix)). \
				get(e##ShortEnumPrefix); \
	}


namespace non_default_enum_map
{	/*	-1 won't work here; only a high value avoids a comparison when
		searching for an enum key in a list. */
	int const end = MAX_SHORT;
}

/*	Abstract base class for dealing with XML loading.
	Sadly, this will add 4 byte for the vtable pointer to the derived classes.
	Just one template param for compatibility with CvXMLLoadUtility::
	SetVariableListTagPair - don't want to move that into a header file
	(currently relies on explicit instantiation). */
template<typename V>
class CvInfoMap
{
public:
	virtual ~CvInfoMap() {}
	virtual void insert(int iKey, V vValue)=0;
	virtual void finalizeInsertions() {}
	virtual V getDefault() const=0;
	virtual int numKeys() const=0;
	/*	<advc.003i> These functions are only for the defunct XML cache ...
		Not tested - b/c the cache isn't working. */
	void write(FDataStreamBase* pStream) const
	{
		_write<V>(pStream);
	}

	void read(FDataStreamBase* pStream)
	{
		_read<V>(pStream);
	}

	/*	For ShortEnumMap encoded as the primitive type V.
		T is then the true (unencoded) type - with its own
		serialization functions. */
	template<class T>
	void writeEncoded(FDataStreamBase* pStream) const
	{
		for (int i = 0; i < numKeys(); i++)
		{
			T innerMap(vget(i));
			innerMap.write(pStream);
		}
	}

	template<class T>
	void readEncoded(FDataStreamBase* pStream)
	{
		for (int i = 0; i < numKeys(); i++)
		{
			T innerMap;
			innerMap.read(pStream);
			insert(i, innerMap.encode());
		}
	}

private:
	/*	To remain compatible with the BtS cache format
		(although I don't think that'll ever matter),
		store and read data only as int or bool. */
	template<typename T>
	void _write(FDataStreamBase* pStream) const
	{
		for (int i = 0; i < numKeys(); i++)
		{
			pStream->Write(static_cast<int>(vget(i)));
		}
	}

	template<>
	void _write<bool>(FDataStreamBase* pStream) const
	{
		for (int i = 0; i < numKeys(); i++)
		{
			pStream->Write(vget(i));
		}
	}

	template<typename T>
	void _read(FDataStreamBase* pStream)
	{
		for (int i = 0; i < numKeys(); i++)
		{
			int iVal;
			pStream->Read(&iVal);
			if (sizeof(V) == 1)
				insert(i, static_cast<V>(toChar(iVal)));
			else if (sizeof(V) == 2)
				insert(i, static_cast<V>(toShort(iVal)));
			else insert(i, static_cast<V>(iVal));
		}
	}

	template<>
	void _read<bool>(FDataStreamBase* pStream)
	{
		for (int i = 0; i < numKeys(); i++)
		{
			bool bVal;
			pStream->Read(&bVal);
			insert(i, bVal);
		}
	}

protected:
	virtual V vget(int iKey) const=0; // </advc.003i>

	typedef unsigned __int32 boolX32; // for specializations
};


template<typename E, typename V, V vDEFAULT>
class CvInfoEnumMap : public CvInfoMap<V>
{
	/*	Note: Somehow, concrete classes derived from this abstract class
		have to declare their default constructor explicitly. */
public:

	V getDefault() const // override
	{
		return vDEFAULT;
	}

	int numKeys() const // override
	{
		return getEnumLength(static_cast<E>(0));
	}
};


template<typename E, typename V, V vDEFAULT = static_cast<V>(0)>
class ListEnumMap : public CvInfoEnumMap<E, V, vDEFAULT>
{
	std::vector<std::pair<E,V> > m_pairs;
	/*	This results in three move instructions.
		Not worth the memory savings from using <short,short> I think(?). */
	/*static std::pair<E,V> expand(std::pair<short,short> iiPair)
	{
		return std::pair<E,V>(static_cast<E>(iiPair.first), static_cast<V>(iiPair.second));
	}*/
public:
	ListEnumMap() {}

	void finalizeInsertions() // override
	{
		FAssert(m_pairs.empty() || m_pairs.back().first != non_default_enum_map::end);
		/*	End of list marker. Will save us one comparison per execution
			of the get loop. Worth the extra 8 byte? */
		m_pairs.push_back(std::make_pair(
				static_cast<E>(non_default_enum_map::end), vDEFAULT));
	}

	void insert(int iKey, V vValue) // override
	{
		/*	finalizeInsertions shouldn't have been called.
			But don't want to check this every time ... */
		//FAssert(m_pairs.empty() || m_pairs.back().first != non_default_enum_map::end);
		if (vValue != vDEFAULT)
			m_pairs.push_back(std::make_pair(static_cast<E>(iKey), vValue));
	}

	V get(E eKey) const
	{
		FAssertEnumBounds(eKey);
		return getUnsafe(eKey);
	}

	V getUnsafe(E eKey) const
	{
		/*	Typically, we'll only have a couple of pairs, or none.
			Not worth doing int divisions and extra comparisons for interpolation. */
		E eNonDefaultKey;
		for (size_t i = 0; (eNonDefaultKey = m_pairs[i].first) <= eKey; i++)
		{
			if (eNonDefaultKey == eKey)
				return m_pairs[i].second;
		}
		return vDEFAULT;
	}

	// eDummy param for interchangeability with ArrayEnumMap
	std::pair<E,V> nextNonDefault(E eDummy, int iPairIndex) const
	{
		FAssertBounds(0, m_pairs.size(), iPairIndex);
		return m_pairs[iPairIndex];
	}

	bool isAnyNonDefault() const
	{
		return (m_pairs.size() > 1); // disregard end-of-list marker
	}

	// <advc.xmldefault> (Not tested)
	ListEnumMap(ListEnumMap const& kOther)
	{
		assign(kOther);
	}

	ListEnumMap& operator=(ListEnumMap const& kOther)
	{
		assign(kOther);
		return *this;
	}

private:
	void assign(ListEnumMap const& kOther)
	{
		m_pairs = kOther.m_pairs;
	} // </advc.xmldefault>
// <advc.003i>
protected:
	V vget(int iKey) const // override
	{
		return getUnsafe(static_cast<E>(iKey));
	} // </advc.003i>
};

// Specialize for bool
template<typename E, bool bDEFAULT>
class ListEnumMap<E, bool, bDEFAULT> : public CvInfoEnumMap<E, bool, bDEFAULT>
{
	// (With the WtP type setup, we could even use char here for many E types.)
	std::vector<short> m_nonDefaultKeys;

public:
	ListEnumMap() {}

	void finalizeInsertions() // override
	{
		FAssert(m_nonDefaultKeys.empty() || m_nonDefaultKeys.back() != non_default_enum_map::end);
		m_nonDefaultKeys.push_back(non_default_enum_map::end);
	}

	void insert(int iKey, bool bValue) // override
	{
		//FAssert(m_nonDefaultKeys.empty() || m_nonDefaultKeys.back() != non_default_enum_map::end);
		if (bValue != bDEFAULT)
			m_nonDefaultKeys.push_back(toShort(iKey));
	}

	bool get(E eKey) const
	{
		FAssertEnumBounds(eKey);
		return getUnsafe(eKey);
	}

	bool getUnsafe(E eKey) const
	{
		short const iKey = static_cast<short>(eKey);
		short iNonDefaultKey;
		for (size_t i = 0; (iNonDefaultKey = m_nonDefaultKeys[i]) <= iKey; i++)
		{
			if (iNonDefaultKey == iKey)
				return !bDEFAULT;
		}
		return bDEFAULT;
	}

	std::pair<E,bool> nextNonDefault(E eDummy, int iPairIndex) const
	{
		FAssertBounds(0, m_nonDefaultKeys.size(), iPairIndex);
		return std::make_pair(static_cast<E>(m_nonDefaultKeys[iPairIndex]), !bDEFAULT);
	}

	bool isAnyNonDefault() const
	{
		return (m_nonDefaultKeys.size() > 1);
	}

	// <advc.xmldefault> (Not tested)
	ListEnumMap(ListEnumMap const& kOther)
	{
		assign(kOther);
	}

	ListEnumMap& operator=(ListEnumMap const& kOther)
	{
		assign(kOther);
		return *this;
	}

private:
	void assign(ListEnumMap const& kOther)
	{
		m_nonDefaultKeys = kOther.m_nonDefaultKeys;
	} // </advc.xmldefault>
// <advc.003i>
protected:
	bool vget(int iKey) const // override
	{
		return getUnsafe(static_cast<E>(iKey));
	} // </advc.003i>
};

#define WTP_ARRAY_ENUM_MAP 0
#if (WTP_ARRAY_ENUM_MAP == 0)

template<typename E, typename V, V vDEFAULT = static_cast<V>(0)>
class ArrayEnumMap : public CvInfoEnumMap<E, V, vDEFAULT>
{
	/*	A vector would take up 12 byte. We don't need to store the size b/c
		it's equal to the length of E. */
	union
	{
		V* m_values;
		boolX32* m_blocks; // V=bool
	};

public:
	ArrayEnumMap() : m_values(NULL) {}

	~ArrayEnumMap()
	{
		uninit();
	}

	void finalizeInsertions() // override
	{
		if (!isAllocated())
			allocate(true);
	}

	void insert(int iKey, V vValue) // override
	{
		if (!isAllocated())
			allocate(true);
		/*	ArrayEnumMap is for mappings with many non-default values, but that'll
			probably not be the case for all instances. I think greatly speeding up
			iteration over all-default instances will be worth storing an extra value. */
		if (vValue != vDEFAULT)
			setAllDefault(false);
		set(static_cast<E>(iKey), vValue);
	}

	V get(E eKey) const
	{
		FAssertEnumBounds(eKey);
		return getUnsafe(eKey);
	}

	V getUnsafe(E eKey) const
	{
		return _getUnsafe<V>(eKey);
	}

	// iDummy param for interchangeability with ListEnumMap
	std::pair<E,V> nextNonDefault(E eLastKey, int iDummy) const
	{
		FAssert(eLastKey >= -1);
		if (!isAllDefault())
		{
			int const iValues = numValues();
			for (int i = eLastKey + 1; i < iValues; i++)
			{
				V vValue = getUnsafe(static_cast<E>(i));
				if (vValue != vDEFAULT)
					return std::make_pair(static_cast<E>(i), vValue);
			}
		}
		return std::make_pair(static_cast<E>(non_default_enum_map::end), vDEFAULT);
	}

	bool isAnyNonDefault() const
	{
		return !isAllDefault();
	}

	// <advc.xmldefault> (Not tested)
	ArrayEnumMap(ArrayEnumMap const& kOther)
	{
		assign(kOther);
	}

	ArrayEnumMap& operator=(ArrayEnumMap const& kOther)
	{
		assign(kOther);
		return *this;
	}

private:
	void assign(ArrayEnumMap const& kOther)
	{
		if (isAllocated() && !kOther.isAllocated())
		{
			uninit();
			return;
		}
		if (!isAllocated() && kOther.isAllocated())
			allocate();
		_assign<V>(kOther);
	}

	template<typename T>
	void _assign(ArrayEnumMap const& kOther)
	{
		std::memcpy(m_values, kOther.m_values, numValues() * sizeof(m_values[0]));
	}
	
	template<>
	void _assign<bool>(ArrayEnumMap const& kOther)
	{
		std::memcpy(m_blocks, kOther.m_blocks, numBlocks() * sizeof(m_blocks[0]));
	} // </advc.xmldefault>

	void uninit()
	{
		SAFE_DELETE_ARRAY(m_values);
	}

	bool isAllocated() const
	{
		return (m_values != NULL);
	}

	void allocate(bool bInitialize = false)
	{
		FAssert(!isAllocated());
		_allocate<V>(bInitialize);
		if (bInitialize)
			setAllDefault(true);
	}

	template<typename T>
	void _allocate(bool bInitialize)
	{
		m_values = new T[fullLength()];
		if (bInitialize)
			std::fill_n(m_values, numValues(), vDEFAULT);
	}

	template<>
	void _allocate<bool>(bool bInitialize)
	{
		m_blocks = new boolX32[numBlocks()];
		if (bInitialize)
			std::fill_n(m_blocks, numBlocks(), vDEFAULT ? MAX_UNSIGNED_INT : 0);
	}

	// (The inherited numKeys can't be inlined b/c it's virtual)
	__inline E numValues() const
	{
		return getEnumLength(static_cast<E>(0));
	}

	int fullLength() const
	{
		// +1 for the all-defaults flag
		return numValues() + 1;
	}

	bool isAllDefault() const
	{
		return (getUnsafe(numValues()) != static_cast<V>(0));
	}

	void setAllDefault(bool bAllDefault)
	{
		set(numValues(), static_cast<V>(bAllDefault ? 1 : 0));
	}

	void set(E eKey, V vValue)
	{
		FAssertBounds(0, fullLength(), eKey);
		setUnsafe(eKey, vValue);
	}

	void setUnsafe(E eKey, V vValue)
	{
		_setUnsafe<V>(eKey, vValue);
	}

	template<typename T>
	void _setUnsafe(E eKey, T tValue)
	{
		m_values[eKey] = tValue;
	}

	template<>
	void _setUnsafe<bool>(E eKey, bool bValue)
	{
		BitUtil::SetBit(m_blocks[getBlock(eKey)], getIndexInBlock(eKey), bValue);
	}

	template<typename T>
	T _getUnsafe(E eKey) const
	{
		return m_values[eKey];
	}

	template<>
	bool _getUnsafe<bool>(E eKey) const
	{
		return BitUtil::HasBit(m_blocks[getBlock(eKey)], getIndexInBlock(eKey));
	}

	/*	From the WtP enum map (EnumMap.h) ...  (Should only be called if V==bool,
		but I'm not going to bother safeguarding that requirement.) */
	int getBlock(int i) const
	{
		return i / 32;
	}

	int getIndexInBlock(int i) const
	{
		return i & 0x1F;
	}

	int numBlocks() const
	{
		return intdiv::uceil(fullLength(), 32);
	}
// <advc.003i>
protected:
	V vget(int iKey) const // override
	{
		return getUnsafe(static_cast<E>(iKey));
	} // </advc.003i>
};

#else
/*	EnumMap makes an isAllocated check in each get call.
	I don't think this is going to be beneficial on the bottom line,
	but it would worth a test (once ArrayEnumMap is more widely used).
	On the plus side, EnumMap handles enums known to be short
	more efficiently. */
template<typename E, typename V, V vDEFAULT = static_cast<V>(0)>
class ArrayEnumMap
:	public EnumMapBase<E, V, static_cast<int>(vDEFAULT)>,
	public CvInfoEnumMap<E, V, vDEFAULT>
{
public:
	ArrayEnumMap() {}

	void insert(int iKey, V vValue) // override
	{
		set(static_cast<E>(iKey), vValue);
	}

	// iDummy param for interchangeability with ListEnumMap
	std::pair<E,V> nextNonDefault(E eLastKey, int iDummy) const
	{
		FAssert(eLastKey >= -1);
		if (isAllocated())
		{
			int const iValues = numValues();
			for (int i = eLastKey + 1; i < iValues; i++)
			{
				E key = static_cast<E>(i);
				V val = get(key);
				if (val != vDEFAULT)
					return std::make_pair(key, val);
			}
		}
		return std::make_pair(static_cast<E>(non_default_enum_map::end), vDEFAULT);
	}

	V getUnsafe(E eKey) const
	{	// (Can't bypass the bounds assertions in a derived class)
		return get(eKey);
	}

	bool isAnyNonDefault() const
	{
		return isAllocated();
	}

	// <advc.xmldefault> (Not tested)
	ArrayEnumMap(ArrayEnumMap const& kOther)
	{
		assign(kOther);
	}

	ArrayEnumMap& operator=(ArrayEnumMap const& kOther)
	{
		assign(kOther);
		return *this;
	}

private:
	void assign(ArrayEnumMap const& kOther)
	{
		for (int i = 0; i < numValues(); i++)
			set(static_cast<E>(i), kOther.get(static_cast<E>(i)));
	} // </advc.xmldefault>

	int numValues() const
	{
		return getEnumLength(static_cast<E>(0));
	}
// <advc.003i>
protected:
	V vget(int iKey) const // override
	{
		return getUnsafe(static_cast<E>(iKey));
	} // </advc.003i>
};
#endif

template<typename E, typename V, V vDEFAULT = static_cast<V>(0)>
class ListAndArrayEnumMap : public CvInfoEnumMap<E, V, vDEFAULT>
{
	ListEnumMap<E,V,vDEFAULT> m_list;
	ArrayEnumMap<E,V,vDEFAULT> m_array;

public:
	ListAndArrayEnumMap() {}

	void finalizeInsertions() // override
	{
		m_list.finalizeInsertions();
		m_array.finalizeInsertions();
	}

	void insert(int iKey, V vValue) // override
	{
		m_list.insert(iKey, vValue);
		m_array.insert(iKey, vValue);
	}

	V get(E eKey) const
	{
		return m_array.get(eKey);
	}

	V getUnsafe(E eKey) const
	{
		return m_array.getUnsafe(eKey);
	}

	std::pair<E,V> nextNonDefault(E eLastKey, int iPairIndex) const
	{
		return m_list.nextNonDefault(eLastKey, iPairIndex);
	}

	// <advc.xmldefault> (Not tested)
	ListAndArrayEnumMap(ListAndArrayEnumMap const& kOther)
	{
		assign(kOther);
	}

	ListAndArrayEnumMap& operator=(ListAndArrayEnumMap const& kOther)
	{
		assign(kOther);
		return *this;
	}

private:
	void assign(ListAndArrayEnumMap const& kOther)
	{
		m_list = kOther.m_list;
		m_array = kOther.m_array;
	} // </advc.xmldefault>

protected:
	V vget(int iKey) const // override
	{
		return getUnsafe(static_cast<E>(iKey));
	}
};


#pragma pack(push, 1)
// Similar to the WtP EnumMap with bINLINE_1_BYTE (V=char) or bINLINE_2_BYTE (V=short)
template<typename E, typename V, V vDEFAULT = 0, int iCAPACITY = 4>
class ShortEnumMap : public CvInfoEnumMap<E, V, vDEFAULT>
{
	BOOST_STATIC_ASSERT(sizeof(V) <= 2);

protected: // To allow quick init from 32-bit and 64-bit int in derived classes
	V m_values[iCAPACITY];
	static const m_iCAPACITY = iCAPACITY;
	/*	Making ShortEnumMap the Value type of another CvInfoMap runs into
		too many hurdles. Will instead treat m_values as a primitive type.
		Caveat: Should then only use the default-default value, i.e. 0, for
		both map types involved: the outer map only knows how to zero the inner map. */
	typedef unsigned __int32 charX4;
	typedef unsigned __int64 shortX4;

public:
	typedef V value_t; // expose for DEF_INFO_... macro

	ShortEnumMap()
	{
		FAssert(numValues() <= iCAPACITY);
		resetAll();
	}

	void resetAll()
	{
		std::fill_n(m_values, iCAPACITY, vDEFAULT);
	}

	void insert(int* piArray)
	{
		for (int i = 0; i < numValues(); i++)
		{
			int iVal = piArray[i];
			/*	There are a bunch of assertions in the code below;
				faster to do a default check upfront. */
			#ifdef FASSERT_ENABLE
			if (iVal == static_cast<int>(vDEFAULT))
				continue;
			#endif
			V vVal;
			if (sizeof(V) == 1)
				vVal = static_cast<V>(toChar(iVal));
			else vVal = static_cast<V>(toShort(iVal));
			insert(i, vVal);
		}
	}

	void insert(int iKey, V vValue) // override
	{
		set(static_cast<E>(iKey), vValue);
	}

	void set(E eKey, V vValue)
	{
		FAssertEnumBounds(eKey);
		setUnsafe(eKey, vValue);
	}

	void setUnsafe(E eKey, V vValue)
	{
		m_values[eKey] = vValue;
	}

	V get(E eKey) const
	{
		FAssertEnumBounds(eKey);
		return getUnsafe(eKey);
	}

	V getUnsafe(E eKey) const
	{
		return m_values[eKey];
	}

	V operator[](E eKey) const
	{
		return getUnsafe(eKey);
	}

	V& operator[](E eKey)
	{
		return m_values[eKey];
	}

	// Handling non-defaults at the call site is going to be faster and easier to read
	/*std::pair<E,V> nextNonDefault(E eLastKey, int iDummy) const
	{
		FAssert(eLastKey >= -1);
		int const iValues = numValues();
		for (int i = eLastKey + 1; i < iValues; i++)
		{
			E eKey = static_cast<E>(i);
			V vValue = getUnsafe(eKey);
			if (vValue != vDEFAULT)
				return std::make_pair(eKey, vValue);
		}
		return std::make_pair(static_cast<E>(non_default_enum_map::end), vDEFAULT);
	}*/

	void write(FDataStreamBase* pStream, bool bAsInt = true) const
	{
		if (bAsInt)
		{
			write(pStream);
			return;
		}
		// Efficient write
		pStream->Write(iCAPACITY, m_values);
	}

	void read(FDataStreamBase* pStream, bool bAsInt = true)
	{
		if (bAsInt)
		{
			read(pStream);
			return;
		}
		// Read efficient format
		pStream->Read(iCAPACITY, m_values);
	}

	// <advc.xmldefault> (Not tested)
	ShortEnumMap(ShortEnumMap const& kOther)
	{
		assign(kOther);
	}

	ShortEnumMap& operator=(ShortEnumMap const& kOther)
	{
		assign(kOther);
		return *this;
	}

private:
	void assign(ShortEnumMap const& kOther)
	{
		std::memcpy(m_values, kOther.m_values, numValues() * sizeof(m_values[0]));
	} // </advc.xmldefault>

	int numValues() const
	{
		return getEnumLength(static_cast<E>(0));
	}
	// <advc.003i>
protected:
	V vget(int iKey) const // override
	{
		return getUnsafe(static_cast<E>(iKey));
	} // </advc.003i>
};

template<typename V, V vDEFAULT = 0>
class YieldMap : public ShortEnumMap<YieldTypes, V, vDEFAULT> {};

class YieldChangeMap : public YieldMap<char>
{
public:
	typedef charX4 enc_t;

	YieldChangeMap() {}

	YieldChangeMap(enc_t ui)
	{
		BOOST_STATIC_ASSERT(sizeof(char) * m_iCAPACITY == sizeof(ui));
		((enc_t*)m_values)[0] = ui;
	}

	enc_t encode() const
	{
		return ((enc_t*)m_values)[0];
	}
};

//template<short iDEFAULT = 0>
class YieldPercentMap : public YieldMap<short, /*iDEFAULT*/0>
{
public:
	typedef shortX4 enc_t;

	YieldPercentMap() {}

	YieldPercentMap(enc_t ui)
	{
		BOOST_STATIC_ASSERT(sizeof(short) * m_iCAPACITY == sizeof(ui));
		((enc_t*)m_values)[0] = ui;
	}

	enc_t encode() const
	{
		return ((enc_t*)m_values)[0];
	}
};

/*class YieldModifierMap : public YieldPercentMap<0>
{
public:
	YieldModifierMap() {}
	YieldModifierMap(shortX4 ui) : YieldPercentMap<0>(ui) {}
};

class YieldMultiplierMap : public YieldPercentMap<100>
{
public:
	YieldMultiplierMap() {}
	YieldMultiplierMap(shortX4 ui) : YieldPercentMap<100>(ui) {}
};*/ // Nonzero default values are too difficult to support

/*	Exact same thing for CommerceTypes ...
	Macro? Maybe once it has matured a bit; no proper IDE support then. */

template<typename V, V vDEFAULT = 0>
class CommerceMap : public ShortEnumMap<CommerceTypes, V, vDEFAULT> {};

class CommerceChangeMap : public CommerceMap<char>
{
public:
	typedef charX4 enc_t;

	CommerceChangeMap() {}

	CommerceChangeMap(enc_t ui)
	{
		BOOST_STATIC_ASSERT(sizeof(char) * m_iCAPACITY == sizeof(ui));
		((enc_t*)m_values)[0] = ui;
	}

	enc_t encode() const
	{
		return ((enc_t*)m_values)[0];
	}
};

//template<short iDEFAULT>
class CommercePercentMap : public CommerceMap<short, /*iDEFAULT*/0>
{
public:
	typedef shortX4 enc_t;

	CommercePercentMap() {}

	CommercePercentMap(enc_t ui)
	{
		BOOST_STATIC_ASSERT(sizeof(short) * m_iCAPACITY == sizeof(ui));
		((enc_t*)m_values)[0] = ui;
	}

	enc_t encode() const
	{
		return ((enc_t*)m_values)[0];
	}
};

/*class CommerceModifierMap : public CommercePercentMap<0>
{
public:
	CommerceModifierMap() {}
	CommerceModifierMap(shortX4 ui) : CommercePercentMap<0>(ui) {}
};

class CommerceMultiplierMap : public CommercePercentMap<100>
{
public:
	CommerceMultiplierMap() {}
	CommerceMultiplierMap(shortX4 ui) : CommercePercentMap<100>(ui) {}
};*/

#pragma pack(pop)

#endif
