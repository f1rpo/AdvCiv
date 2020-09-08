#pragma once

#ifndef CV_INFO_BASE_H
#define CV_INFO_BASE_H

#include "CyInfoWrapper.h" // advc.003x

/*  advc.003x: Cut from CvInfos.h; to be precompiled.
	CvInfoBase, CvScalableInfo, CvHotkeyInfo */

class CvXMLLoadUtility;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvInfoBase
//
//  The base class for all info classes to inherit from.  This gives us
//	the base description and type strings
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvInfoBase /* advc.003e: */ : private boost::noncopyable
{
public: // All the const functions are exposed to Python
	CvInfoBase();
	CvInfoBase(CvInfoBase const& kOther); // advc.xmldefault
	DllExport virtual ~CvInfoBase();

	virtual void reset();

	bool isGraphicalOnly() const;

	DllExport const TCHAR* getType() const;
	bool isDefaultsType() const; // advc.xmldefault
	virtual const TCHAR* getButton() const;

	// for python wide string handling
	std::wstring pyGetTextKey() { return getTextKeyWide(); }
	std::wstring pyGetDescription() { return getDescription(0); }
	std::wstring pyGetDescriptionForm(uint uiForm) { return getDescription(uiForm); }
	std::wstring pyGetText() { return getText(); }
	std::wstring pyGetCivilopedia() { return getCivilopedia(); }
	std::wstring pyGetHelp() { return getHelp(); }
	std::wstring pyGetStrategy() { return getStrategy(); }

	DllExport const wchar* getTextKeyWide() const;
	DllExport const wchar* getDescription(uint uiForm = 0) const;
	DllExport const wchar* getText() const;
	const wchar* getCivilopedia() const;
	DllExport const wchar* getHelp() const;
	const wchar* getStrategy() const;

	bool isMatchForLink(std::wstring szLink, bool bKeysOnly) const;
	#if SERIALIZE_CVINFOS
	virtual void read(FDataStreamBase* pStream);
	virtual void write(FDataStreamBase* pStream);
	#endif
	// FUNCTION:    read
	//! \brief      Reads in a CvAnimationPathInfo definition from XML
	//! \param      pXML Pointer to the XML loading object
	//! \retval     true if the definition was read successfully, false otherwise
	// (advc: Cut this comment from CvAnimationPathInfo)
	virtual bool read(CvXMLLoadUtility* pXML);
	virtual bool readPass2(CvXMLLoadUtility* pXML)
	{
		FErrorMsg("readPass2 has not been overridden");
		return false;
	}
	virtual bool readPass3()
	{
		FErrorMsg("readPass3 has not been overridden");
		return false;
	}

protected:
	//bool doneReadingXML(CvXMLLoadUtility* pXML); // advc.003j: Never had an implementation

	bool m_bGraphicalOnly;

	CvString m_szType;
	CvString m_szButton; // Used for Infos that don't require an ArtAssetInfo
	CvWString m_szTextKey;
	CvWString m_szCivilopediaKey;
	CvWString m_szHelpKey;
	CvWString m_szStrategyKey;

	// translated text
	std::vector<CvString> m_aszExtraXMLforPass3;
	mutable std::vector<CvWString> m_aCachedDescriptions;
	mutable CvWString m_szCachedText;
	mutable CvWString m_szCachedHelp;
	mutable CvWString m_szCachedStrategy;
	mutable CvWString m_szCachedCivilopedia;
};

// holds the scale for scalable objects
class CvScalableInfo /* advc.003e: */ : private boost::noncopyable
{
public:
	CvScalableInfo() : m_fScale(1.0f), m_fInterfaceScale(1.0f) {}

	DllExport float getScale() const; // Exposed to Python

	// the scale of the unit appearing in the interface screens
	DllExport float getInterfaceScale() const; // Exposed to Python

	bool read(CvXMLLoadUtility* pXML);

protected:
	float m_fScale;
	float m_fInterfaceScale; 
};

// <advc.tag> Abstract class that allows XML elements to be added and accessed through enum values
class CvXMLInfo : public CvInfoBase
{
public:
	enum IntElementTypes // To be extended by derived classes (see CvImprovementInfo for an example)
	{
		NUM_INT_ELEMENT_TYPES
	};
	enum BoolElementTypes
	{
		NUM_BOOL_ELEMENT_TYPES
	};
	__forceinline int get(IntElementTypes e) const
	{
		FAssertBounds(0, m_aiData.size(), e);
		return m_aiData[e];
	}
	__forceinline int get(BoolElementTypes e) const
	{
		FAssertBounds(0, m_abData.size(), e);
		return m_abData[e];
	}
	bool read(CvXMLLoadUtility* pXML);
	#if SERIALIZE_CVINFOS
	void read(FDataStreamBase* pStream);
	void write(FDataStreamBase* pStream);
	#endif

protected:
	enum ElementDataType
	{
		INT_ELEMENT,
		BOOL_ELEMENT,
	};
	class XMLElement
	{
	public:
		XMLElement(int iEnumValue, CvString szName);
		XMLElement(int iEnumValue, CvString szName, bool bMandatory);
		virtual ~XMLElement() {};
		virtual ElementDataType getDataType() const = 0;
		int getEnumValue() const;
		CvString getName() const;
		bool isMandatory() const;
	private:
		CvString m_szName;
		int m_iEnumValue;
		bool m_bMandatory;
	};
	class IntElement : public XMLElement
	{
	public:
		IntElement(int iEnumValue, CvString szName);
		IntElement(int iEnumValue, CvString szName, int iDefault);
		ElementDataType getDataType() const;
		int getDefaultValue() const;
	private:
		int m_iDefaultValue;
	};
	class BoolElement : public XMLElement
	{
	public:
		BoolElement(int iEnumValue, CvString szName);
		BoolElement(int iEnumValue, CvString szName, bool bDefault);
		ElementDataType getDataType() const;
		int getDefaultValue() const;
	private:
		int m_bDefaultValue;
	};
	/*  Derived classes that extend any of the element type enums need to override this.
		The overridden function needs to call the base function and then append its
		own elements to r. */
	virtual void addElements(std::vector<XMLElement*>& r) const;
	// Allow derived classes to overwrite data (but don't expose the data structures)
	void set(IntElementTypes e, int iNewValue);
	void set(BoolElementTypes e, bool bNewValue);

private:
	std::vector<int> m_aiData;
	std::vector<bool> m_abData;
}; // </advc.tag>

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvHotkeyInfo
//  holds the hotkey info for an info class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvHotkeyInfo : public /* advc.tag: */ CvXMLInfo
{
public:
	CvHotkeyInfo();
	// <advc.tag>
	enum IntElementTypes // unused so far
	{
		NUM_INT_ELEMENT_TYPES = CvXMLInfo::NUM_BOOL_ELEMENT_TYPES
	};
	enum BoolElementTypes // unused so far
	{
		NUM_BOOL_ELEMENT_TYPES = CvXMLInfo::NUM_BOOL_ELEMENT_TYPES
	};
	using CvXMLInfo::get; // unhide
	__forceinline int get(IntElementTypes e) const
	{
		return get(static_cast<CvXMLInfo::IntElementTypes>(e));
	}
	__forceinline int get(BoolElementTypes e) const
	{
		return get(static_cast<CvXMLInfo::BoolElementTypes>(e));
	} // </advc.tag>

	bool read(CvXMLLoadUtility* pXML);
	#if SERIALIZE_CVINFOS
	virtual void read(FDataStreamBase* pStream);
	virtual void write(FDataStreamBase* pStream);
	#endif
	int getActionInfoIndex() const;
	void setActionInfoIndex(int i);

	int getHotKeyVal() const;
	int getHotKeyPriority() const;
	int getHotKeyValAlt() const;
	int getHotKeyPriorityAlt() const;
	int getOrderPriority() const;

	bool isAltDown() const;
	bool isShiftDown() const;
	bool isCtrlDown() const;
	bool isAltDownAlt() const;
	bool isShiftDownAlt() const;
	bool isCtrlDownAlt() const;

	const TCHAR* getHotKey() const; // Exposed to Python

	std::wstring getHotKeyDescription() const;
	void setHotKeyDescription(const wchar* szHotKeyDescKey, const wchar* szHotKeyAltDescKey,
			const wchar* szHotKeyString);

protected:
	int m_iActionInfoIndex;
	int m_iHotKeyVal;
	int m_iHotKeyPriority;
	int m_iHotKeyValAlt;
	int m_iHotKeyPriorityAlt;
	int m_iOrderPriority;

	bool m_bAltDown;
	bool m_bShiftDown;
	bool m_bCtrlDown;
	bool m_bAltDownAlt;
	bool m_bShiftDownAlt;
	bool m_bCtrlDownAlt;

	CvString m_szHotKey;
	CvWString m_szHotKeyDescriptionKey;
	CvWString m_szHotKeyAltDescriptionKey;
	CvWString m_szHotKeyString;

	void addElements(std::vector<XMLElement*>& r) const; // advc.tag
};

#endif
