// advc.003x: Cut from CvInfos.cpp and reorganized a bit with a new base class

#include "CvGameCoreDLL.h"
#include "CvXMLLoadUtility.h"
#include "CvDLLXMLIFaceBase.h"


CvOrganizationInfo::CvOrganizationInfo() :
m_wcSymbol(0),
m_iTechPrereq(NO_TECH),
m_iFreeUnitClass(NO_UNITCLASS),
m_iSpreadFactor(0),
m_iMissionType(NO_MISSION)
{}

wchar CvOrganizationInfo::getChar() const
{
	return m_wcSymbol;
}

void CvOrganizationInfo::setChar(wchar wc)
{
	m_wcSymbol = wc;
}

int CvOrganizationInfo::getFreeUnitClass() const
{
	return m_iFreeUnitClass;
}

int CvOrganizationInfo::getMissionType() const
{
	return m_iMissionType;
}

void CvOrganizationInfo::setMissionType(int iNewType)
{
	m_iMissionType = iNewType;
}

const TCHAR* CvOrganizationInfo::getMovieFile() const
{
	return m_szMovieFile;
}

const TCHAR* CvOrganizationInfo::getMovieSound() const
{
	return m_szMovieSound;
}

const TCHAR* CvOrganizationInfo::getSound() const
{
	return m_szSound;
}

bool CvOrganizationInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvHotkeyInfo::read(pXML))
		return false;

	pXML->SetInfoIDFromChildXmlVal(m_iTechPrereq, "TechPrereq");
	pXML->SetInfoIDFromChildXmlVal(m_iFreeUnitClass, "FreeUnitClass");

	pXML->GetChildXmlValByName(&m_iSpreadFactor, "iSpreadFactor");

	pXML->GetChildXmlValByName(m_szMovieFile, "MovieFile");
	pXML->GetChildXmlValByName(m_szMovieSound, "MovieSound");
	pXML->GetChildXmlValByName(m_szSound, "Sound");

	return true;
}

CvReligionInfo::CvReligionInfo() :
m_iHolyCityChar(0),
m_iNumFreeUnits(0),
m_paiGlobalReligionCommerce(NULL),
m_paiHolyCityCommerce(NULL),
m_paiStateReligionCommerce(NULL)
{}

CvReligionInfo::~CvReligionInfo()
{
	SAFE_DELETE_ARRAY(m_paiGlobalReligionCommerce);
	SAFE_DELETE_ARRAY(m_paiHolyCityCommerce);
	SAFE_DELETE_ARRAY(m_paiStateReligionCommerce);
}

int CvReligionInfo::getHolyCityChar() const
{
	return m_iHolyCityChar;
}

void CvReligionInfo::setHolyCityChar(int i)
{
	m_iHolyCityChar = i;
}

int CvReligionInfo::getNumFreeUnits() const
{
	return m_iNumFreeUnits;
}

const TCHAR* CvReligionInfo::getTechButton() const
{
	return m_szTechButton;
}

const TCHAR* CvReligionInfo::getGenericTechButton() const
{
	return m_szGenericTechButton;
}

const TCHAR* CvReligionInfo::getButtonDisabled() const
{
	static TCHAR szDisabled[512];

	szDisabled[0] = '\0';

	if (getButton() && strlen(getButton()) > 4)
	{
		strncpy(szDisabled, getButton(), strlen(getButton()) - 4);
		szDisabled[strlen(getButton()) - 4] = '\0';
		strcat(szDisabled, "_D.dds");
	}

	return szDisabled;
}

const wchar* CvReligionInfo::getAdjectiveKey() const
{
	return m_szAdjectiveKey;
}

int CvReligionInfo::getGlobalReligionCommerce(int i) const
{
	FAssertBounds(0, NUM_COMMERCE_TYPES, i);
	return m_paiGlobalReligionCommerce ? m_paiGlobalReligionCommerce[i] : 0; // advc.003t
}

int* CvReligionInfo::getGlobalReligionCommerceArray() const
{
	return m_paiGlobalReligionCommerce;
}

int CvReligionInfo::getHolyCityCommerce(int i) const
{
	FAssertBounds(0, NUM_COMMERCE_TYPES, i);
	return m_paiHolyCityCommerce ? m_paiHolyCityCommerce[i] : 0; // advc.003t
}

int* CvReligionInfo::getHolyCityCommerceArray() const
{
	return m_paiHolyCityCommerce;
}

int CvReligionInfo::getStateReligionCommerce(int i) const
{
	FAssertBounds(0, NUM_COMMERCE_TYPES, i);
	return m_paiStateReligionCommerce ? m_paiStateReligionCommerce[i] : 0; // advc.003t
}

int* CvReligionInfo::getStateReligionCommerceArray() const
{
	return m_paiStateReligionCommerce;
}

bool CvReligionInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvOrganizationInfo::read(pXML))
		return false;

	pXML->GetChildXmlValByName(&m_iNumFreeUnits, "iFreeUnits");

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"GlobalReligionCommerces"))
	{
		pXML->SetCommerce(&m_paiGlobalReligionCommerce);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_paiGlobalReligionCommerce, NUM_COMMERCE_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"HolyCityCommerces"))
	{
		pXML->SetCommerce(&m_paiHolyCityCommerce);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_paiHolyCityCommerce, NUM_COMMERCE_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"StateReligionCommerces"))
	{
		pXML->SetCommerce(&m_paiStateReligionCommerce);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_paiStateReligionCommerce, NUM_COMMERCE_TYPES);

	pXML->GetChildXmlValByName(m_szTechButton, "TechButton");
	pXML->GetChildXmlValByName(m_szGenericTechButton, "GenericTechButton");
	pXML->GetChildXmlValByName(m_szAdjectiveKey, "Adjective");

	return true;
}
// advc.003w:
bool CvReligionInfo::isReligionTech(TechTypes eTech)
{
	FOR_EACH_ENUM(Religion)
	{
		if (GC.getInfo(eLoopReligion).getTechPrereq() == eTech)
			return true;
	}
	return false;
}

CvCorporationInfo::CvCorporationInfo() :
m_iHeadquarterChar(0),
m_iSpreadCost(0),
m_iMaintenance(0),
m_eBonusProduced(NO_BONUS),
m_paiHeadquarterCommerce(NULL),
m_paiCommerceProduced(NULL),
m_paiYieldProduced(NULL)
{}

CvCorporationInfo::~CvCorporationInfo()
{
	SAFE_DELETE_ARRAY(m_paiHeadquarterCommerce);
	SAFE_DELETE_ARRAY(m_paiCommerceProduced);
	SAFE_DELETE_ARRAY(m_paiYieldProduced);
}

int CvCorporationInfo::getHeadquarterChar() const
{
	return m_iHeadquarterChar;
}

void CvCorporationInfo::setHeadquarterChar(int i)
{
	m_iHeadquarterChar = i;
}
// advc.003t: Calls from Python aren't going to respect the bounds
int CvCorporationInfo::py_getPrereqBonus(int i) const
{
	if (i < 0 || i >= getNumPrereqBonuses())
		return NO_BONUS;
	return m_aePrereqBonuses[i];
}

int CvCorporationInfo::getHeadquarterCommerce(int i) const
{
	FAssertBounds(0, NUM_COMMERCE_TYPES, i);
	return m_paiHeadquarterCommerce ? m_paiHeadquarterCommerce[i] : 0; // advc.003t
}

int* CvCorporationInfo::getHeadquarterCommerceArray() const
{
	return m_paiHeadquarterCommerce;
}

int CvCorporationInfo::getCommerceProduced(int i) const
{
	FAssertBounds(0, NUM_COMMERCE_TYPES, i);
	return m_paiCommerceProduced ? m_paiCommerceProduced[i] : 0; // advc.003t
}

int* CvCorporationInfo::getCommerceProducedArray() const
{
	return m_paiCommerceProduced;
}

int CvCorporationInfo::getYieldProduced(int i) const
{
	FAssertBounds(0, NUM_YIELD_TYPES, i);
	return m_paiYieldProduced ? m_paiYieldProduced[i] : 0; // advc.003t
}

int* CvCorporationInfo::getYieldProducedArray() const
{
	return m_paiYieldProduced;
}

bool CvCorporationInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvOrganizationInfo::read(pXML))
		return false;

	pXML->GetChildXmlValByName(&m_iSpreadCost, "iSpreadCost");
	pXML->GetChildXmlValByName(&m_iMaintenance, "iMaintenance");

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"HeadquarterCommerces"))
	{
		pXML->SetCommerce(&m_paiHeadquarterCommerce);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_paiHeadquarterCommerce, NUM_COMMERCE_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "CommercesProduced"))
	{
		pXML->SetCommerce(&m_paiCommerceProduced);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_paiCommerceProduced, NUM_COMMERCE_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "YieldsProduced"))
	{
		pXML->SetYields(&m_paiYieldProduced);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_paiYieldProduced, NUM_YIELD_TYPES);

	CvString szTextVal;

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "PrereqBonuses"))
	{
		if (pXML->SkipToNextVal())
		{
			int const iNumSibs = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
			if (iNumSibs > 0 && pXML->GetChildXmlVal(szTextVal))
			{
				FAssert(iNumSibs <= GC.getDefineINT(CvGlobals::NUM_CORPORATION_PREREQ_BONUSES));
				for (int j = 0; j < iNumSibs; j++)
				{
					BonusTypes eBonus = (BonusTypes)pXML->FindInInfoClass(szTextVal);
					if (eBonus != NO_BONUS)
						m_aePrereqBonuses.push_back(eBonus);
					if (!pXML->GetNextXmlVal(szTextVal))
						break;
				}
				gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
			}
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}

	pXML->GetChildXmlValByName(szTextVal, "BonusProduced");
	m_eBonusProduced = (BonusTypes)pXML->FindInInfoClass(szTextVal);

	return true;
}
// advc.003w:
bool isCorporationTech(TechTypes eTech)
{
	FOR_EACH_ENUM(Corporation)
	{
		if (GC.getInfo(eLoopCorporation).getTechPrereq() == eTech)
			return true;
	}
	return false;
}
