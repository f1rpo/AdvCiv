// advc.003x: Cut from CvInfos.cpp

#include "CvGameCoreDLL.h"
#include "CvInfo_Build.h"
#include "CvXMLLoadUtility.h"


CvBuildInfo::CvBuildInfo() :
m_iTime(0),
m_iCost(0),
m_iTechPrereq(NO_TECH),
m_iImprovement(NO_IMPROVEMENT),
m_iRoute(NO_ROUTE),
m_iEntityEvent(NO_ENTITYEVENT),
m_iMissionType(NO_MISSION),
m_bKill(false),
m_paiFeatureTech(NULL),
m_paiFeatureTime(NULL),
m_paiFeatureProduction(NULL),
m_pabFeatureRemove(NULL)
{}

CvBuildInfo::~CvBuildInfo()
{
	SAFE_DELETE_ARRAY(m_paiFeatureTech);
	SAFE_DELETE_ARRAY(m_paiFeatureTime);
	SAFE_DELETE_ARRAY(m_paiFeatureProduction);
	SAFE_DELETE_ARRAY(m_pabFeatureRemove);
}

int CvBuildInfo::getEntityEvent() const
{
	return m_iEntityEvent;
}

int CvBuildInfo::getMissionType() const
{
	return m_iMissionType;
}

void CvBuildInfo::setMissionType(int iNewType)
{
	m_iMissionType = iNewType;
}

TechTypes CvBuildInfo::getFeatureTech(FeatureTypes eFeature) const
{
	FAssertEnumBounds(eFeature);
	return m_paiFeatureTech ? (TechTypes)m_paiFeatureTech[eFeature] : NO_TECH; // advc.003t
}

int CvBuildInfo::getFeatureTime(FeatureTypes eFeature) const
{
	FAssertEnumBounds(eFeature);
	return m_paiFeatureTime ? m_paiFeatureTime[eFeature] : 0; // advc.003t
}

int CvBuildInfo::getFeatureProduction(FeatureTypes eFeature) const
{
	FAssertEnumBounds(eFeature);
	return m_paiFeatureProduction ? m_paiFeatureProduction[eFeature] : 0; // advc.003t
}

// <!-- custom: fix the no feature assert failed in EnumTraits.h by adding a NO_FEATURE guard.
// It is tedious to do it in all callers, so a guard here is much easier, clean and easy, with the help of chatgpt 5 thanks, see known issue as of now 71 for details -->
// <!-- custom: for reference, error was: -->
// Assert Failed
// File: c:\program files (x86)\steam\steamapps\common\sid meier's civilization iv beyond the sword\beyond the sword\mods\advciv-sas\cvgamecoredll\EnumTraits.h
// Line: 193
// Func: enum_traits_detail::assertEnumBounds
// Expression: static_cast<int>(eIndex) >= static_cast<int>(0)
// Message: Index expected to be >= 0. (value: -1)
// Yep—patch isFeatureRemove itself to tolerate NO_FEATURE. Minimal, future-proof:
// That’s all you need. It prevents the EnumTraits assert and makes every existing call site safe (AI, UI, Python). No other changes required.
bool CvBuildInfo::isFeatureRemove(FeatureTypes eFeature) const
{
    // Allow callers to pass "no feature" safely
    if (eFeature == NO_FEATURE)
        return false;
	FAssertEnumBounds(eFeature);
	return m_pabFeatureRemove ? m_pabFeatureRemove[eFeature] : false;
}

bool CvBuildInfo::read(CvXMLLoadUtility* pXML)
{
	if (!base_t::read(pXML))
		return false;

	pXML->SetInfoIDFromChildXmlVal(m_iTechPrereq, "PrereqTech");

	pXML->GetChildXmlValByName(&m_iTime, "iTime");
	pXML->GetChildXmlValByName(&m_iCost, "iCost");
	pXML->GetChildXmlValByName(&m_bKill, "bKill");

	pXML->SetInfoIDFromChildXmlVal(m_iImprovement, "ImprovementType");
	pXML->SetInfoIDFromChildXmlVal(m_iRoute, "RouteType");
	pXML->SetInfoIDFromChildXmlVal(m_iEntityEvent, "EntityEvent");

	pXML->SetFeatureStruct(&m_paiFeatureTech, &m_paiFeatureTime, &m_paiFeatureProduction, &m_pabFeatureRemove);

	return true;
}
