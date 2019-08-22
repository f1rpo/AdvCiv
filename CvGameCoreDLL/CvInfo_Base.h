#pragma once

#ifndef CV_INFO_BASE_H
#define CV_INFO_BASE_H

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
	DllExport virtual ~CvInfoBase();

	virtual void reset();

	bool isGraphicalOnly() const;

	DllExport const TCHAR* getType() const;
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
		FAssertMsg(false, "readPass2 has not been overridden");
		return false;
	}
	virtual bool readPass3()
	{
		FAssertMsg(false, "readPass3 has not been overridden");
		return false;
	}

protected:
	bool doneReadingXML(CvXMLLoadUtility* pXML);
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
	void setScale(float fScale);

	// the scale of the unit appearing in the interface screens
	DllExport float getInterfaceScale() const; // Exposed to Python
	void setInterfaceScale(float fInterfaceScale);

	bool read(CvXMLLoadUtility* pXML);

protected:
	float m_fScale;
	float m_fInterfaceScale; 
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  class : CvHotkeyInfo
//  holds the hotkey info for an info class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvHotkeyInfo : public CvInfoBase
{
public:
	CvHotkeyInfo();

	bool read(CvXMLLoadUtility* pXML);
	#if SERIALIZE_CVINFOS
	virtual void read(FDataStreamBase* pStream);
	virtual void write(FDataStreamBase* pStream);
	#endif
	int getActionInfoIndex() const;
	void setActionInfoIndex(int i);

	int getHotKeyVal() const;
	void setHotKeyVal(int i);
	int getHotKeyPriority() const;
	void setHotKeyPriority(int i);
	int getHotKeyValAlt() const;
	void setHotKeyValAlt(int i);
	int getHotKeyPriorityAlt() const;
	void setHotKeyPriorityAlt(int i);
	int getOrderPriority() const;
	void setOrderPriority(int i);

	bool isAltDown() const;
	void setAltDown(bool b);
	bool isShiftDown() const;
	void setShiftDown(bool b);
	bool isCtrlDown() const;
	void setCtrlDown(bool b);
	bool isAltDownAlt() const;
	void setAltDownAlt(bool b);
	bool isShiftDownAlt() const;
	void setShiftDownAlt(bool b);
	bool isCtrlDownAlt() const;
	void setCtrlDownAlt(bool b);

	const TCHAR* getHotKey() const; // Exposed to Python
	void setHotKey(const TCHAR* szVal);

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
};

#endif
