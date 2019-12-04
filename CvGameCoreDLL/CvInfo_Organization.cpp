// advc.003x: Cut from CvInfos.cpp and reorganized a bit with a new base class

#include "CvGameCoreDLL.h"
#include "CvXMLLoadUtility.h"
#include "CvDLLXMLIFaceBase.h"


CvOrganizationInfo::CvOrganizationInfo() :
m_iChar(0),
m_iTechPrereq(NO_TECH),
m_iFreeUnitClass(NO_UNITCLASS),
m_iSpreadFactor(0),
m_iMissionType(NO_MISSION)
{}

int CvOrganizationInfo::getChar() const
{
	return m_iChar;
}

void CvOrganizationInfo::setChar(int i)
{
	m_iChar = i;
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

void CvOrganizationInfo::setMovieFile(const TCHAR* szVal)
{
	m_szMovieFile = szVal;
}

const TCHAR* CvOrganizationInfo::getMovieSound() const
{
	return m_szMovieSound;
}

void CvOrganizationInfo::setMovieSound(const TCHAR* szVal)
{
	m_szMovieSound = szVal;
}

const TCHAR* CvOrganizationInfo::getSound() const
{
	return m_szSound;
}

void CvOrganizationInfo::setSound(const TCHAR* szVal)
{
	m_szSound=szVal;
}

bool CvOrganizationInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvHotkeyInfo::read(pXML))
		return false;

	CvString szTextVal;

	pXML->GetChildXmlValByName(szTextVal, "TechPrereq");
	m_iTechPrereq = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "FreeUnitClass");
	m_iFreeUnitClass = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(&m_iSpreadFactor, "iSpreadFactor");

	pXML->GetChildXmlValByName(szTextVal, "MovieFile");
	setMovieFile(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "MovieSound");
	setMovieSound(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "Sound");
	setSound(szTextVal);

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

void CvReligionInfo::setTechButton(const TCHAR* szVal)
{
	m_szTechButton=szVal;
}

const TCHAR* CvReligionInfo::getGenericTechButton() const
{
	return m_szGenericTechButton;
}

void CvReligionInfo::setGenericTechButton(const TCHAR* szVal)
{
	m_szGenericTechButton = szVal;
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

void CvReligionInfo::setAdjectiveKey(const TCHAR* szVal)
{
	m_szAdjectiveKey = szVal;
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

	CvString szTextVal;

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

	pXML->GetChildXmlValByName(szTextVal, "TechButton");
	setTechButton(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "GenericTechButton");
	setGenericTechButton(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "Adjective");
	setAdjectiveKey(szTextVal);

	return true;
}

CvCorporationInfo::CvCorporationInfo() :
m_iHeadquarterChar(0),
m_iSpreadCost(0),
m_iMaintenance(0),
m_iBonusProduced(NO_BONUS),
m_paiPrereqBonuses(NULL),
m_paiHeadquarterCommerce(NULL),
m_paiCommerceProduced(NULL),
m_paiYieldProduced(NULL)
{}

CvCorporationInfo::~CvCorporationInfo()
{
	SAFE_DELETE_ARRAY(m_paiPrereqBonuses);
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

int CvCorporationInfo::getSpreadCost() const
{
	return m_iSpreadCost;
}

int CvCorporationInfo::getMaintenance() const
{
	return m_iMaintenance;
}

int CvCorporationInfo::getBonusProduced() const
{
	return m_iBonusProduced;
}

int CvCorporationInfo::getPrereqBonus(int i) const
{
	FAssertBounds(0, GC.getNUM_CORPORATION_PREREQ_BONUSES(), i);
	return (m_paiPrereqBonuses != NULL ? m_paiPrereqBonuses[i] : NO_BONUS); // advc.003t
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

	CvString szTextVal;

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

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "PrereqBonuses"))
	{
		if (pXML->SkipToNextVal())
		{
			int iNumSibs = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
			FAssertMsg(GC.getNUM_CORPORATION_PREREQ_BONUSES() > 0, "Allocating zero or less memory in CvCorporationInfo::read");
			pXML->InitList(&m_paiPrereqBonuses, GC.getNUM_CORPORATION_PREREQ_BONUSES(), -1);

			if (iNumSibs > 0)
			{
				if (pXML->GetChildXmlVal(szTextVal))
				{
					FAssert(iNumSibs <= GC.getNUM_CORPORATION_PREREQ_BONUSES());
					for (int j = 0; j < iNumSibs; j++)
					{
						m_paiPrereqBonuses[j] = pXML->FindInInfoClass(szTextVal);
						if (!pXML->GetNextXmlVal(szTextVal))
							break;
					}
					gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
				}
			}
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}

	pXML->GetChildXmlValByName(szTextVal, "BonusProduced");
	m_iBonusProduced = pXML->FindInInfoClass(szTextVal);

	return true;
}
