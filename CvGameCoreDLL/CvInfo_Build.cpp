// advc.003x: Cut from CvInfos.cpp

#include "CvGameCoreDLL.h"
#include "CvInfo_Build.h"
#include "CvXMLLoadUtility.h"
#include "CvDLLXMLIFaceBase.h"


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

int CvBuildInfo::getFeatureTech(int i) const
{
	FAssertBounds(0, GC.getNumFeatureInfos(), i);
	return m_paiFeatureTech ? m_paiFeatureTech[i] : NO_TECH; // advc.003t
}

int CvBuildInfo::getFeatureTime(int i) const
{
	FAssertBounds(0, GC.getNumFeatureInfos(), i);
	return m_paiFeatureTime ? m_paiFeatureTime[i] : 0; // advc.003t
}

int CvBuildInfo::getFeatureProduction(int i) const
{
	FAssertBounds(0, GC.getNumFeatureInfos(), i);
	return m_paiFeatureProduction ? m_paiFeatureProduction[i] : 0; // advc.003t
}

bool CvBuildInfo::isFeatureRemove(int i) const
{
	FAssertBounds(0, GC.getNumFeatureInfos(), i);
	return m_pabFeatureRemove ? m_pabFeatureRemove[i] : false;
}

bool CvBuildInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvHotkeyInfo::read(pXML))
		return false;

	CvString szTextVal;

	pXML->GetChildXmlValByName(szTextVal, "PrereqTech");
	m_iTechPrereq = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(&m_iTime, "iTime");
	pXML->GetChildXmlValByName(&m_iCost, "iCost");
	pXML->GetChildXmlValByName(&m_bKill, "bKill");

	pXML->GetChildXmlValByName(szTextVal, "ImprovementType");
	m_iImprovement = pXML->FindInInfoClass(szTextVal);
	pXML->GetChildXmlValByName(szTextVal, "RouteType");
	m_iRoute = pXML->FindInInfoClass(szTextVal);
	pXML->GetChildXmlValByName(szTextVal, "EntityEvent");
	m_iEntityEvent = pXML->FindInInfoClass(szTextVal);

	pXML->SetFeatureStruct(&m_paiFeatureTech, &m_paiFeatureTime, &m_paiFeatureProduction, &m_pabFeatureRemove);

	return true;
}
