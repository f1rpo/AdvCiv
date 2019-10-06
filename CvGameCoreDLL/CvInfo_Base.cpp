// advc.003x: Cut from CvInfos.cpp

#include "CvGameCoreDLL.h"
#include "CvXMLLoadUtility.h"
#include "CvDLLXMLIFaceBase.h"

CvInfoBase::CvInfoBase() : m_bGraphicalOnly(false) {}

CvInfoBase::~CvInfoBase() {}
#if SERIALIZE_CVINFOS
void CvInfoBase::read(FDataStreamBase* pStream)
{
	reset();
	pStream->Read(&m_bGraphicalOnly);
	pStream->ReadString(m_szType);
	pStream->ReadString(m_szCivilopediaKey);
	pStream->ReadString(m_szHelpKey);
	pStream->ReadString(m_szStrategyKey);
	pStream->ReadString(m_szButton);
	pStream->ReadString(m_szTextKey);
}

void CvInfoBase::write(FDataStreamBase* pStream)
{
	pStream->Write(m_bGraphicalOnly);
	pStream->WriteString(m_szType);
	pStream->WriteString(m_szCivilopediaKey);
	pStream->WriteString(m_szHelpKey);
	pStream->WriteString(m_szStrategyKey);
	pStream->WriteString(m_szButton);
	pStream->WriteString(m_szTextKey);
}
#endif
void CvInfoBase::reset()
{
	//clear cache
	m_aCachedDescriptions.clear();
	m_szCachedText.clear();
	m_szCachedHelp.clear();
	m_szCachedStrategy.clear();
	m_szCachedCivilopedia.clear();
}

bool CvInfoBase::isGraphicalOnly() const
{
	return m_bGraphicalOnly;
}

const TCHAR* CvInfoBase::getType() const
{
	if (m_szType.empty())
		return NULL;

	return m_szType;
}

const TCHAR* CvInfoBase::getButton() const
{
	if (m_szButton.empty())
		return NULL;

	return m_szButton;
}

const wchar* CvInfoBase::getTextKeyWide() const
{
	return m_szTextKey;
}

const wchar* CvInfoBase::getDescription(uint uiForm) const
{
	while(m_aCachedDescriptions.size() <= uiForm)
	{
		m_aCachedDescriptions.push_back(gDLL->getObjectText(m_szTextKey,
			m_aCachedDescriptions.size()));
	}
	return m_aCachedDescriptions[uiForm];
}

const wchar* CvInfoBase::getText() const
{
	// used instead of getDescription for Info entries that are not objects
	// so they do not have gender/plurality/forms defined in the Translator system
	if(m_szCachedText.empty())
		m_szCachedText = gDLL->getText(m_szTextKey);
	return m_szCachedText;
}

const wchar* CvInfoBase::getCivilopedia() const
{
	if(m_szCachedCivilopedia.empty())
		m_szCachedCivilopedia = gDLL->getText(m_szCivilopediaKey);
	return m_szCachedCivilopedia;
}

const wchar*  CvInfoBase::getHelp() const
{
	if (m_szCachedHelp.empty())
		m_szCachedHelp = gDLL->getText(m_szHelpKey);
	return m_szCachedHelp;
}

const wchar* CvInfoBase::getStrategy() const
{
	if (m_szCachedStrategy.empty())
		m_szCachedStrategy = gDLL->getText(m_szStrategyKey);
	return m_szCachedStrategy;
}

bool CvInfoBase::isMatchForLink(std::wstring szLink, bool bKeysOnly) const
{
	if (szLink == CvWString(getType()).GetCString())
		return true;

	if (!bKeysOnly)
	{
		uint iNumForms = gDLL->getNumForms(getTextKeyWide());
		for (uint i = 0; i < iNumForms; i++)
		{
			if (szLink == getDescription(i))
				return true;
		}
	}
	return false;
}

// read from XML: TYPE, DESC, BUTTON
bool CvInfoBase::read(CvXMLLoadUtility* pXML)
{
	// Skip any comments and stop at the next value we might want
	if (!pXML->SkipToNextVal())
		return false;

	pXML->MapChildren(); // try to hash children for fast lookup by name

	pXML->GetChildXmlValByName(&m_bGraphicalOnly, "bGraphicalOnly", /* advc.006b: */ false);
	pXML->GetChildXmlValByName(m_szType, "Type", /* advc.006b: */ "");
	pXML->GetChildXmlValByName(m_szTextKey, "Description", /* advc.006b: */ L"");
	pXML->GetChildXmlValByName(m_szCivilopediaKey, "Civilopedia", /* advc.006b: */ L"");
	pXML->GetChildXmlValByName(m_szHelpKey, "Help", /* advc.006b: */ L"");
	pXML->GetChildXmlValByName(m_szStrategyKey, "Strategy", /* advc.006b: */ L"");
	pXML->GetChildXmlValByName(m_szButton, "Button", /* advc.006b: */ "");

	return true;
}


bool CvScalableInfo::read(CvXMLLoadUtility* pXML)
{
	float fScale;
	pXML->GetChildXmlValByName(&fScale, "fScale");
	setScale(fScale);
	pXML->GetChildXmlValByName(&fScale, "fInterfaceScale", 1.0f);
	setInterfaceScale(fScale);
	return true;
}

float CvScalableInfo::getScale() const
{
	return m_fScale;
}

void CvScalableInfo::setScale(float fScale)
{
	m_fScale = fScale;
}

float CvScalableInfo::getInterfaceScale() const
{
	return m_fInterfaceScale;
}

void CvScalableInfo::setInterfaceScale(float fInterfaceScale)
{
	m_fInterfaceScale = fInterfaceScale;
}

CvHotkeyInfo::CvHotkeyInfo() :
m_iActionInfoIndex(-1),
m_iHotKeyVal(-1),
m_iHotKeyPriority(-1),
m_iHotKeyValAlt(-1),
m_iHotKeyPriorityAlt(-1),
m_iOrderPriority(5), // advc.006b: Moved from CvHotkeyInfo::read
m_bAltDown(false),
m_bShiftDown(false),
m_bCtrlDown(false),
m_bAltDownAlt(false),
m_bShiftDownAlt(false),
m_bCtrlDownAlt(false)
{}

bool CvHotkeyInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvInfoBase::read(pXML))
		return false;

	int iVal;
	bool bVal;
	CvString szTextVal;

	// advc.006b: Default arguments added to GetChildXmlValByName calls

	if (pXML->GetChildXmlValByName(szTextVal, "HotKey", ""))
		setHotKey(szTextVal);

	setHotKeyVal(pXML->GetHotKeyInt(szTextVal));

	if (pXML->GetChildXmlValByName(&iVal, "iHotKeyPriority",  -1))
		setHotKeyPriority(iVal);

	if (pXML->GetChildXmlValByName(szTextVal, "HotKeyAlt", ""))
		setHotKeyValAlt(pXML->GetHotKeyInt(szTextVal));

	if (pXML->GetChildXmlValByName(&iVal, "iHotKeyPriorityAlt", -1))
		setHotKeyPriorityAlt(iVal);

	if (pXML->GetChildXmlValByName(&bVal, "bAltDown", false))
		setAltDown(bVal);

	if (pXML->GetChildXmlValByName(&bVal, "bShiftDown", false))
		setShiftDown(bVal);

	if (pXML->GetChildXmlValByName(&bVal, "bCtrlDown", false))
		setCtrlDown(bVal);

	if (pXML->GetChildXmlValByName(&bVal, "bAltDownAlt", false))
		setAltDownAlt(bVal);

	if (pXML->GetChildXmlValByName(&bVal, "bShiftDownAlt", false))
		setShiftDownAlt(bVal);

	if (pXML->GetChildXmlValByName(&bVal, "bCtrlDownAlt", false))
		setCtrlDownAlt(bVal);

	if (pXML->GetChildXmlValByName(&iVal, "iOrderPriority", 0))
		setOrderPriority(iVal);

	setHotKeyDescription(getTextKeyWide(), NULL, pXML->CreateHotKeyFromDescription(getHotKey(), m_bShiftDown, m_bAltDown, m_bCtrlDown));

	return true;
}
#if SERIALIZE_CVINFOS
void CvHotkeyInfo::read(FDataStreamBase* pStream)
{
	CvInfoBase::read(pStream);
	uint uiFlag=0;
	pStream->Read(&uiFlag);

	pStream->Read(&m_iHotKeyVal);
	pStream->Read(&m_iHotKeyPriority);
	pStream->Read(&m_iHotKeyValAlt);
	pStream->Read(&m_iHotKeyPriorityAlt);
	pStream->Read(&m_iOrderPriority);
	pStream->Read(&m_bAltDown);
	pStream->Read(&m_bShiftDown);
	pStream->Read(&m_bCtrlDown);
	pStream->Read(&m_bAltDownAlt);
	pStream->Read(&m_bShiftDownAlt);
	pStream->Read(&m_bCtrlDownAlt);
	pStream->ReadString(m_szHotKey);
	pStream->ReadString(m_szHotKeyDescriptionKey);
	pStream->ReadString(m_szHotKeyAltDescriptionKey);
	pStream->ReadString(m_szHotKeyString);
}

void CvHotkeyInfo::write(FDataStreamBase* pStream)
{
	CvInfoBase::write(pStream);
	uint uiFlag = 0;
	pStream->Write(uiFlag);

	pStream->Write(m_iHotKeyVal);
	pStream->Write(m_iHotKeyPriority);
	pStream->Write(m_iHotKeyValAlt);
	pStream->Write(m_iHotKeyPriorityAlt);
	pStream->Write(m_iOrderPriority);
	pStream->Write(m_bAltDown);
	pStream->Write(m_bShiftDown);
	pStream->Write(m_bCtrlDown);
	pStream->Write(m_bAltDownAlt);
	pStream->Write(m_bShiftDownAlt);
	pStream->Write(m_bCtrlDownAlt);
	pStream->WriteString(m_szHotKey);
	pStream->WriteString(m_szHotKeyDescriptionKey);
	pStream->WriteString(m_szHotKeyAltDescriptionKey);
	pStream->WriteString(m_szHotKeyString);
}
#endif
int CvHotkeyInfo::getActionInfoIndex() const
{
	return m_iActionInfoIndex;
}

void CvHotkeyInfo::setActionInfoIndex(int i)
{
	m_iActionInfoIndex = i;
}

int CvHotkeyInfo::getHotKeyVal() const
{
	return m_iHotKeyVal;
}

void CvHotkeyInfo::setHotKeyVal(int i)
{
	m_iHotKeyVal = i;
}

int CvHotkeyInfo::getHotKeyPriority() const
{
	return m_iHotKeyPriority;
}

void CvHotkeyInfo::setHotKeyPriority(int i)
{
	m_iHotKeyPriority = i;
}

int CvHotkeyInfo::getHotKeyValAlt() const
{
	return m_iHotKeyValAlt;
}

void CvHotkeyInfo::setHotKeyValAlt(int i)
{
	m_iHotKeyValAlt = i;
}

int CvHotkeyInfo::getHotKeyPriorityAlt() const
{
	return m_iHotKeyPriorityAlt;
}

void CvHotkeyInfo::setHotKeyPriorityAlt(int i)
{
	m_iHotKeyPriorityAlt = i;
}

int CvHotkeyInfo::getOrderPriority() const
{
	return m_iOrderPriority;
}

void CvHotkeyInfo::setOrderPriority(int i)
{
	m_iOrderPriority = i;
}

bool CvHotkeyInfo::isAltDown() const
{
	return m_bAltDown;
}

void CvHotkeyInfo::setAltDown(bool b)
{
	m_bAltDown = b;
}

bool CvHotkeyInfo::isShiftDown() const
{
	return m_bShiftDown;
}

void CvHotkeyInfo::setShiftDown(bool b)
{
	m_bShiftDown = b;
}

bool CvHotkeyInfo::isCtrlDown() const
{
	return m_bCtrlDown;
}

void CvHotkeyInfo::setCtrlDown(bool b)
{
	m_bCtrlDown = b;
}

bool CvHotkeyInfo::isAltDownAlt() const
{
	return m_bAltDownAlt;
}

void CvHotkeyInfo::setAltDownAlt(bool b)
{
	m_bAltDownAlt = b;
}

bool CvHotkeyInfo::isShiftDownAlt() const
{
	return m_bShiftDownAlt;
}

void CvHotkeyInfo::setShiftDownAlt(bool b)
{
	m_bShiftDownAlt = b;
}

bool CvHotkeyInfo::isCtrlDownAlt() const
{
	return m_bCtrlDownAlt;
}

void CvHotkeyInfo::setCtrlDownAlt(bool b)
{
	m_bCtrlDownAlt = b;
}

const TCHAR* CvHotkeyInfo::getHotKey() const
{
	return m_szHotKey;
}

void CvHotkeyInfo::setHotKey(const TCHAR* szVal)
{
	m_szHotKey = szVal;
}

std::wstring CvHotkeyInfo::getHotKeyDescription() const
{
	CvWString szTemptext;
	if (!m_szHotKeyAltDescriptionKey.empty())
	{
		szTemptext.Format(L"%s (%s)", gDLL->getObjectText(m_szHotKeyAltDescriptionKey, 0).GetCString(),
			gDLL->getObjectText(m_szHotKeyDescriptionKey, 0).GetCString());
	}
	else szTemptext = gDLL->getObjectText(m_szHotKeyDescriptionKey, 0);

	if (!m_szHotKeyString.empty())
		szTemptext += m_szHotKeyString;

	return szTemptext;
}

void CvHotkeyInfo::setHotKeyDescription(const wchar* szHotKeyDescKey, const wchar* szHotKeyAltDescKey, const wchar* szHotKeyString)
{
	m_szHotKeyDescriptionKey = szHotKeyDescKey;
	m_szHotKeyAltDescriptionKey = szHotKeyAltDescKey;
	m_szHotKeyString = szHotKeyString;
}

// <advc.tag>
CvInfoEnum::XMLElement::XMLElement(int iEnumValue, CvString szName) :
		m_iEnumValue(iEnumValue), m_szName(szName), m_bMandatory(true) {}

CvInfoEnum::XMLElement::XMLElement(int iEnumValue, CvString szName, bool bMandatory) :
		m_iEnumValue(iEnumValue), m_szName(szName), m_bMandatory(bMandatory) {}

int CvInfoEnum::XMLElement::getEnumValue() const { return m_iEnumValue; }

CvString CvInfoEnum::XMLElement::getName() const { return m_szName; }
	
bool CvInfoEnum::XMLElement::isMandatory() const { return m_bMandatory; }

CvInfoEnum::IntElement::IntElement(int iEnumValue, CvString szName) :
		XMLElement(iEnumValue, szName), m_iDefaultValue(0) {}

CvInfoEnum::IntElement::IntElement(int iEnumValue, CvString szName, int iDefault) :
		XMLElement(iEnumValue, szName, false), m_iDefaultValue(iDefault) {}

CvInfoEnum::ElementDataType CvInfoEnum::IntElement::getDataType() const
{
	return INT_ELEMENT;
}

int CvInfoEnum::IntElement::getDefaultValue() const { return m_iDefaultValue; }

CvInfoEnum::BoolElement::BoolElement(int iEnumValue, CvString szName) :
		XMLElement(iEnumValue, szName), m_bDefaultValue(false) {}

CvInfoEnum::BoolElement::BoolElement(int iEnumValue, CvString szName, bool bDefault) :
		XMLElement(iEnumValue, szName, false), m_bDefaultValue(bDefault) {}

CvInfoEnum::ElementDataType CvInfoEnum::BoolElement::getDataType() const
{
	return BOOL_ELEMENT;
}

int CvInfoEnum::BoolElement::getDefaultValue() const { return m_bDefaultValue; }

void CvInfoEnum::addElements(std::vector<XMLElement*>& r) const
{
	// Could add elements common to all info classes here
}

void CvInfoEnum::set(IntElementTypes e, int iNewValue)
{
	FASSERT_BOUNDS(0, (int)m_aiData.size(), e, "CvInfoBase::set(IntElementTypes,int)");
	m_aiData[e] = iNewValue;
}

void CvInfoEnum::set(BoolElementTypes e, bool bNewValue)
{
	FASSERT_BOUNDS(0, (int)m_abData.size(), e, "CvInfoBase::set(BoolElementTypes,bool)");
	m_abData[e] = bNewValue;
}

bool CvInfoEnum::read(CvXMLLoadUtility* pXML)
{
	CvInfoBase::read(pXML);

	std::vector<XMLElement*> apElements;
	addElements(apElements);
	{	// Allocate space in data vectors
		int iIntElements = 0;
		int iBoolElements = 0;
		for (size_t i = 0; i < apElements.size(); i++)
		{
			switch(apElements[i]->getDataType())
			{
			case INT_ELEMENT: iIntElements++; break;
			case BOOL_ELEMENT: iBoolElements++; break;
			default: FAssertMsg(false, "Data type misses element counting code");
			}
		}
		m_aiData.resize(iIntElements);
		m_abData.resize(iBoolElements);
	}
	for (size_t i = 0; i < apElements.size(); i++)
	{
		XMLElement& kElement = *apElements[i];
		int const iEnumValue = kElement.getEnumValue();
		CvString szName = kElement.getName();
		switch(kElement.getDataType())
		{
		case INT_ELEMENT:
			szName.insert(0, "i");
			int iTmp;
			if (kElement.isMandatory())
				pXML->GetChildXmlValByName(&iTmp, szName.GetCString());
			else
			{
				pXML->GetChildXmlValByName(&iTmp, szName.GetCString(),
						static_cast<IntElement&>(kElement).getDefaultValue());
			}
			FASSERT_BOUNDS(0, (int)m_aiData.size(), iEnumValue, "CvInfoBase::read");
			m_aiData[iEnumValue] = iTmp;
			break;
		case BOOL_ELEMENT:
			szName.insert(0, "b");
			bool bTmp;
			if (kElement.isMandatory())
				pXML->GetChildXmlValByName(&bTmp, szName.GetCString());
			else
			{
				pXML->GetChildXmlValByName(&bTmp, szName.GetCString(),
						static_cast<BoolElement&>(kElement).getDefaultValue());
			}
			FASSERT_BOUNDS(0, (int)m_abData.size(), iEnumValue, "CvInfoBase::read");
			m_abData[iEnumValue] = bTmp;
			break;
		default: FAssertMsg(false, "Data type misses XML loading code");
		}
		delete &kElement;
	}
	return true;
}

#if SERIALIZE_CVINFOS
void CvInfoEnum::read(FDataStreamBase* pStream)
{
	CvInfoBase::read(pStream);
	pStream->Read((int)m_aiData.size(), m_aiData.data());
	pStream->Read((int)m_abData.size(), m_abData.data());
}

void CvInfoEnum::write(FDataStreamBase* pStream)
{
	CvInfoBase::write(pStream);
	pStream->Write((int)m_aiData.size(), m_aiData.data());
	pStream->Write((int)m_abData.size(), m_abData.data());
}
#endif // </advc.tag>
