// advc.003x: Cut from CvInfos.cpp

#include "CvGameCoreDLL.h"
#include "CvInfo_Terrain.h"
#include "CvXMLLoadUtility.h"
#include "CvDLLXMLIFaceBase.h"


CvTerrainInfo::CvTerrainInfo() :
m_iMovementCost(0),
m_iSeeFromLevel(0),
m_iSeeThroughLevel(0),
m_iBuildModifier(0),
m_iDefenseModifier(0),
m_bWater(false),
m_bImpassable(false),
m_bFound(false),
m_bFoundCoast(false),
m_bFoundFreshWater(false),
m_iWorldSoundscapeScriptId(0),
m_piYields(NULL),
m_piRiverYieldChange(NULL),
m_piHillsYieldChange(NULL),
m_pi3DAudioScriptFootstepIndex(NULL)
{}

CvTerrainInfo::~CvTerrainInfo()
{
	SAFE_DELETE_ARRAY(m_piYields);
	SAFE_DELETE_ARRAY(m_piRiverYieldChange);
	SAFE_DELETE_ARRAY(m_piHillsYieldChange);
	SAFE_DELETE_ARRAY(m_pi3DAudioScriptFootstepIndex);
}

const TCHAR* CvTerrainInfo::getArtDefineTag() const
{
	return m_szArtDefineTag;
}

void CvTerrainInfo::setArtDefineTag(const TCHAR* szTag)
{
	m_szArtDefineTag = szTag;
}

int CvTerrainInfo::getWorldSoundscapeScriptId() const
{
	return m_iWorldSoundscapeScriptId;
}

int CvTerrainInfo::getYield(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvTerrainInfo::getYield");
	return m_piYields ? m_piYields[i] : 0; // advc.003t
}

int CvTerrainInfo::getRiverYieldChange(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvTerrainInfo::getRiverYieldChange");
	return m_piRiverYieldChange ? m_piRiverYieldChange[i] : 0; // advc.003t
}

int CvTerrainInfo::getHillsYieldChange(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvTerrainInfo::getHillsYieldChange");
	return m_piHillsYieldChange ? m_piHillsYieldChange[i] : 0; // advc.003t
}

int CvTerrainInfo::get3DAudioScriptFootstepIndex(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumFootstepAudioTypes(), i, "CvTerrainInfo::get3DAudioScriptFootstepIndex"); // advc: Check for upper bound added
	return m_pi3DAudioScriptFootstepIndex ? m_pi3DAudioScriptFootstepIndex[i]
			: 0; // advc.003t: see get3DAudioScriptFootstepIndex
}

bool CvTerrainInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvInfoBase::read(pXML))
		return false;

	CvString szTextVal;
	pXML->GetChildXmlValByName( szTextVal, "ArtDefineTag");
	setArtDefineTag(szTextVal);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"Yields"))
	{
		pXML->SetYields(&m_piYields);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piYields, NUM_YIELD_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"RiverYieldChange"))
	{
		pXML->SetYields(&m_piRiverYieldChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piRiverYieldChange, NUM_YIELD_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"HillsYieldChange"))
	{
		pXML->SetYields(&m_piHillsYieldChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piHillsYieldChange, NUM_YIELD_TYPES);

	pXML->GetChildXmlValByName(&m_bWater, "bWater");
	pXML->GetChildXmlValByName(&m_bImpassable, "bImpassable");
	pXML->GetChildXmlValByName(&m_bFound, "bFound");
	pXML->GetChildXmlValByName(&m_bFoundCoast, "bFoundCoast");
	pXML->GetChildXmlValByName(&m_bFoundFreshWater, "bFoundFreshWater");

	pXML->GetChildXmlValByName(&m_iMovementCost, "iMovement");
	pXML->GetChildXmlValByName(&m_iSeeFromLevel, "iSeeFrom");
	pXML->GetChildXmlValByName(&m_iSeeThroughLevel, "iSeeThrough");
	pXML->GetChildXmlValByName(&m_iBuildModifier, "iBuildModifier");
	pXML->GetChildXmlValByName(&m_iDefenseModifier, "iDefense");

	pXML->SetVariableListTagPairForAudioScripts(&m_pi3DAudioScriptFootstepIndex, "FootstepSounds", GC.getNumFootstepAudioTypes());

	pXML->GetChildXmlValByName(szTextVal, "WorldSoundscapeAudioScript", /* advc.006b: */ "");
	if (szTextVal.GetLength() > 0)
		m_iWorldSoundscapeScriptId = gDLL->getAudioTagIndex(szTextVal.GetCString(), AUDIOTAG_SOUNDSCAPE);
	else m_iWorldSoundscapeScriptId = -1;

	return true;
}

const TCHAR* CvTerrainInfo::getButton() const
{
	const CvArtInfoTerrain* pTerrainArtInfo = getArtInfo();
	if (pTerrainArtInfo != NULL)
		return pTerrainArtInfo->getButton();

	return NULL;
}

const CvArtInfoTerrain* CvTerrainInfo::getArtInfo() const
{
	return ARTFILEMGR.getTerrainArtInfo(getArtDefineTag());
}

CvFeatureInfo::CvFeatureInfo() :
m_iMovementCost(0),
m_iSeeThroughChange(0),
m_iHealthPercent(0),
m_iAppearanceProbability(0),
m_iDisappearanceProbability(0),
m_iGrowthProbability(0),
m_iDefenseModifier(0),
m_iRivalDefenseModifier(0), // advc.012
m_iAdvancedStartRemoveCost(0),
m_iTurnDamage(0),
m_iWarmingDefense(0), //GWMod
m_bNoCoast(false),
m_bNoRiver(false),
m_bNoAdjacent(false),
m_bRequiresFlatlands(false),
m_bRequiresRiver(false),
m_bRequiresRiverSide(false), // advc.129b
m_bAddsFreshWater(false),
m_bImpassable(false),
m_bNoCity(false),
m_bNoImprovement(false),
m_bVisibleAlways(false),
m_bNukeImmune(false),
m_iWorldSoundscapeScriptId(0),
m_iEffectProbability(0),
m_piYieldChange(NULL),
m_piRiverYieldChange(NULL),
m_piHillsYieldChange(NULL),
m_pi3DAudioScriptFootstepIndex(NULL),
m_pbTerrain(NULL)
{}

CvFeatureInfo::~CvFeatureInfo()
{
	SAFE_DELETE_ARRAY(m_piYieldChange);
	SAFE_DELETE_ARRAY(m_piRiverYieldChange);
	SAFE_DELETE_ARRAY(m_piHillsYieldChange);
	SAFE_DELETE_ARRAY(m_pi3DAudioScriptFootstepIndex);
	SAFE_DELETE_ARRAY(m_pbTerrain);
}

int CvFeatureInfo::getAppearanceProbability() const
{
	return m_iAppearanceProbability;
}

int CvFeatureInfo::getDisappearanceProbability() const
{
	return m_iDisappearanceProbability;
}

int CvFeatureInfo::getGrowthProbability() const
{
	return m_iGrowthProbability;
}

int CvFeatureInfo::getAdvancedStartRemoveCost() const
{
	return m_iAdvancedStartRemoveCost;
}

int CvFeatureInfo::getTurnDamage() const
{
	return m_iTurnDamage;
}

int CvFeatureInfo::getWarmingDefense() const //GWMod
{
	return m_iWarmingDefense;
}

bool CvFeatureInfo::isNoCoast() const
{
	return m_bNoCoast;
}

bool CvFeatureInfo::isNoRiver() const
{
	return m_bNoRiver;
}

bool CvFeatureInfo::isNoAdjacent() const
{
	return m_bNoAdjacent;
}

bool CvFeatureInfo::isRequiresFlatlands() const
{
	return m_bRequiresFlatlands;
}

bool CvFeatureInfo::isRequiresRiver() const
{
	return m_bRequiresRiver;
}
// <advc.129b>
bool CvFeatureInfo::isRequiresRiverSide() const
{
	return m_bRequiresRiverSide;
} // </advc.129b>

bool CvFeatureInfo::isAddsFreshWater() const
{
	return m_bAddsFreshWater;
}

bool CvFeatureInfo::isNoCity() const
{
	return m_bNoCity;
}

bool CvFeatureInfo::isVisibleAlways() const
{
	return m_bVisibleAlways;
}

bool CvFeatureInfo::isNukeImmune() const
{
	return m_bNukeImmune;
}

const TCHAR* CvFeatureInfo::getOnUnitChangeTo() const
{
	return m_szOnUnitChangeTo;
}

const TCHAR* CvFeatureInfo::getArtDefineTag() const
{
	return m_szArtDefineTag;
}

void CvFeatureInfo::setArtDefineTag(const TCHAR* szTag)
{
	m_szArtDefineTag = szTag;
}

int CvFeatureInfo::getWorldSoundscapeScriptId() const
{
	return m_iWorldSoundscapeScriptId;
}

const TCHAR* CvFeatureInfo::getEffectType() const
{
	return m_szEffectType;
}

int CvFeatureInfo::getEffectProbability() const
{
	return m_iEffectProbability;
}

int CvFeatureInfo::get3DAudioScriptFootstepIndex(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumFootstepAudioTypes(), i, "CvTerrainInfo::get3DAudioScriptFootstepIndex"); // advc: Check for upper bound added
	return m_pi3DAudioScriptFootstepIndex ? m_pi3DAudioScriptFootstepIndex[i]
			// advc.003t: Was -1. CvTerrainInfo::read sets 0 as the default, so 0 works apparently.
			: 0;
}

bool CvFeatureInfo::isTerrain(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumTerrainInfos(), i, "CvFeatureInfo::isTerrain");
	return m_pbTerrain ? m_pbTerrain[i] : false;
}

int CvFeatureInfo::getNumVarieties() const
{
	return getArtInfo()->getNumVarieties();
}

const TCHAR* CvFeatureInfo::getButton() const
{
	const CvArtInfoFeature* pFeatureArtInfo = getArtInfo();
	if (pFeatureArtInfo != NULL)
		return pFeatureArtInfo->getButton();

	else return NULL;
}

const CvArtInfoFeature* CvFeatureInfo::getArtInfo() const
{
	return ARTFILEMGR.getFeatureArtInfo( getArtDefineTag());
}

bool CvFeatureInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvInfoBase::read(pXML))
		return false;

	CvString szTextVal;
	pXML->GetChildXmlValByName( szTextVal, "ArtDefineTag");
	setArtDefineTag(szTextVal);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"YieldChanges"))
	{
		pXML->SetYields(&m_piYieldChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else
	{
		pXML->InitList(&m_piYieldChange, NUM_YIELD_TYPES);
	}

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"RiverYieldChange"))
	{
		pXML->SetYields(&m_piRiverYieldChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else
	{
		pXML->InitList(&m_piRiverYieldChange, NUM_YIELD_TYPES);
	}

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"HillsYieldChange"))
	{
		pXML->SetYields(&m_piHillsYieldChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piHillsYieldChange, NUM_YIELD_TYPES);

	pXML->GetChildXmlValByName(&m_iMovementCost, "iMovement");
	pXML->GetChildXmlValByName(&m_iSeeThroughChange, "iSeeThrough");
	pXML->GetChildXmlValByName(&m_iHealthPercent, "iHealthPercent");
	pXML->GetChildXmlValByName(&m_iDefenseModifier, "iDefense");
	// advc.012:
	pXML->GetChildXmlValByName(&m_iRivalDefenseModifier, "iRivalDefense");
	pXML->GetChildXmlValByName(&m_iAdvancedStartRemoveCost, "iAdvancedStartRemoveCost");
	pXML->GetChildXmlValByName(&m_iTurnDamage, "iTurnDamage");
	pXML->GetChildXmlValByName(&m_iWarmingDefense, "iWarmingDefense"); //GWMod new xml field M.A.
	pXML->GetChildXmlValByName(&m_iAppearanceProbability, "iAppearance");
	pXML->GetChildXmlValByName(&m_iDisappearanceProbability, "iDisappearance");
	pXML->GetChildXmlValByName(&m_iGrowthProbability, "iGrowth");
	pXML->GetChildXmlValByName(&m_bNoCoast, "bNoCoast");
	pXML->GetChildXmlValByName(&m_bNoRiver, "bNoRiver");
	pXML->GetChildXmlValByName(&m_bNoAdjacent, "bNoAdjacent");
	pXML->GetChildXmlValByName(&m_bRequiresFlatlands, "bRequiresFlatlands");
	pXML->GetChildXmlValByName(&m_bRequiresRiver, "bRequiresRiver");
	// advc.129b:
	pXML->GetChildXmlValByName(&m_bRequiresRiverSide, "bRequiresRiverSide");
	pXML->GetChildXmlValByName(&m_bAddsFreshWater, "bAddsFreshWater");
	pXML->GetChildXmlValByName(&m_bImpassable, "bImpassable");
	pXML->GetChildXmlValByName(&m_bNoCity, "bNoCity");
	pXML->GetChildXmlValByName(&m_bNoImprovement, "bNoImprovement");
	pXML->GetChildXmlValByName(&m_bVisibleAlways, "bVisibleAlways");
	pXML->GetChildXmlValByName(&m_bNukeImmune, "bNukeImmune");
	pXML->GetChildXmlValByName(m_szOnUnitChangeTo, "OnUnitChangeTo");

	pXML->SetVariableListTagPairForAudioScripts(&m_pi3DAudioScriptFootstepIndex, "FootstepSounds", GC.getNumFootstepAudioTypes());

	pXML->GetChildXmlValByName(szTextVal, "WorldSoundscapeAudioScript", /* advc.006b: */ "");
	if (szTextVal.GetLength() > 0)
		m_iWorldSoundscapeScriptId = gDLL->getAudioTagIndex(szTextVal.GetCString(), AUDIOTAG_SOUNDSCAPE);
	else m_iWorldSoundscapeScriptId = -1;

	pXML->GetChildXmlValByName(m_szEffectType, "EffectType");
	pXML->GetChildXmlValByName(&m_iEffectProbability, "iEffectProbability");

	pXML->SetVariableListTagPair(&m_pbTerrain, "TerrainBooleans", GC.getNumTerrainInfos());

	return true;
}

CvBonusInfo::CvBonusInfo() :
m_iBonusClassType(NO_BONUSCLASS),
m_iChar(0),
m_iTechReveal(0),
m_iTechCityTrade(0),
m_iTechObsolete(0),
m_iAITradeModifier(0),
m_iAIObjective(0),
m_iHealth(0),
m_iHappiness(0),
m_iMinAreaSize(0),
m_iMinLatitude(0),
m_iMaxLatitude(90),
m_iPlacementOrder(0),
m_iConstAppearance(0),
m_iRandAppearance1(0),
m_iRandAppearance2(0),
m_iRandAppearance3(0),
m_iRandAppearance4(0),
m_iPercentPerPlayer(0),
m_iTilesPer(0),
m_iMinLandPercent(0),
m_iUniqueRange(0),
m_iGroupRange(0),
m_iGroupRand(0),
m_bOneArea(false),
m_bHills(false),
m_bFlatlands(false),
m_bNoRiverSide(false),
m_bNormalize(false),
m_piYieldChange(NULL),
m_pbTerrain(NULL),
m_pbFeature(NULL),
m_pbFeatureTerrain(NULL)
{}

CvBonusInfo::~CvBonusInfo()
{
	SAFE_DELETE_ARRAY(m_piYieldChange);
	SAFE_DELETE_ARRAY(m_pbTerrain);
	SAFE_DELETE_ARRAY(m_pbFeature);
	SAFE_DELETE_ARRAY(m_pbFeatureTerrain); // free memory - MT
}

int CvBonusInfo::getBonusClassType() const
{
	return m_iBonusClassType;
}

int CvBonusInfo::getChar() const
{
	return m_iChar;
}

void CvBonusInfo::setChar(int i)
{
	m_iChar = i;
}

int CvBonusInfo::getAITradeModifier() const
{
	return m_iAITradeModifier;
}

int CvBonusInfo::getAIObjective() const
{
	return m_iAIObjective;
}

int CvBonusInfo::getMinAreaSize() const
{
	return m_iMinAreaSize;
}

int CvBonusInfo::getMinLatitude() const
{
	return m_iMinLatitude;
}

int CvBonusInfo::getMaxLatitude() const
{
	return m_iMaxLatitude;
}

int CvBonusInfo::getPlacementOrder() const
{
	return m_iPlacementOrder;
}

int CvBonusInfo::getConstAppearance() const
{
	return m_iConstAppearance;
}

int CvBonusInfo::getRandAppearance1() const
{
	return m_iRandAppearance1;
}

int CvBonusInfo::getRandAppearance2() const
{
	return m_iRandAppearance2;
}

int CvBonusInfo::getRandAppearance3() const
{
	return m_iRandAppearance3;
}

int CvBonusInfo::getRandAppearance4() const
{
	return m_iRandAppearance4;
}

int CvBonusInfo::getPercentPerPlayer() const
{
	return m_iPercentPerPlayer;
}

int CvBonusInfo::getTilesPer() const
{
	return m_iTilesPer;
}

int CvBonusInfo::getMinLandPercent() const
{
	return m_iMinLandPercent;
}

int CvBonusInfo::getUniqueRange() const
{
	return m_iUniqueRange;
}

int CvBonusInfo::getGroupRange() const
{
	return m_iGroupRange;
}

int CvBonusInfo::getGroupRand() const
{
	return m_iGroupRand;
}

bool CvBonusInfo::isOneArea() const
{
	return m_bOneArea;
}

bool CvBonusInfo::isHills() const
{
	return m_bHills;
}

bool CvBonusInfo::isFlatlands() const
{
	return m_bFlatlands;
}

bool CvBonusInfo::isNoRiverSide() const
{
	return m_bNoRiverSide;
}

bool CvBonusInfo::isNormalize() const
{
	return m_bNormalize;
}

const TCHAR* CvBonusInfo::getArtDefineTag() const
{
	return m_szArtDefineTag;
}

void CvBonusInfo::setArtDefineTag(const TCHAR* szVal)
{
	m_szArtDefineTag = szVal;
}

const CvArtInfoBonus* CvBonusInfo::getArtInfo() const
{
	return ARTFILEMGR.getBonusArtInfo( getArtDefineTag());
}

int CvBonusInfo::getYieldChange(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvBonusInfo::getYieldChange");
	return m_piYieldChange ? m_piYieldChange[i] : 0; // advc.003t
}

int* CvBonusInfo::getYieldChangeArray()
{
	return m_piYieldChange;
}

bool CvBonusInfo::isTerrain(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumTerrainInfos(), i, "CvBonusInfo::isTerrain");
	return m_pbTerrain ? m_pbTerrain[i] : false;
}

bool CvBonusInfo::isFeature(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumFeatureInfos(), i, "CvBonusInfo::isFeature");
	return m_pbFeature ? m_pbFeature[i] : false;
}

bool CvBonusInfo::isFeatureTerrain(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumTerrainInfos(), i, "CvBonusInfo::isFeatureTerrain");
	return m_pbFeatureTerrain ?	m_pbFeatureTerrain[i] : false;
}

const TCHAR* CvBonusInfo::getButton() const
{
	const CvArtInfoBonus* pBonusArtInfo = getArtInfo();
	if (pBonusArtInfo != NULL)
		return pBonusArtInfo->getButton();

	return NULL;
}
#if SERIALIZE_CVINFOS
void CvBonusInfo::read(FDataStreamBase* stream)
{
	CvInfoBase::read(stream);
	uint uiFlag=0;
	stream->Read(&uiFlag);

	stream->Read(&m_iBonusClassType);
	stream->Read(&m_iChar);
	stream->Read(&m_iTechReveal);
	stream->Read(&m_iTechCityTrade);
	stream->Read(&m_iTechObsolete);
	stream->Read(&m_iAITradeModifier);
	stream->Read(&m_iAIObjective);
	stream->Read(&m_iHealth);
	stream->Read(&m_iHappiness);
	stream->Read(&m_iMinAreaSize);
	stream->Read(&m_iMinLatitude);
	stream->Read(&m_iMaxLatitude);
	stream->Read(&m_iPlacementOrder);
	stream->Read(&m_iConstAppearance);
	stream->Read(&m_iRandAppearance1);
	stream->Read(&m_iRandAppearance2);
	stream->Read(&m_iRandAppearance3);
	stream->Read(&m_iRandAppearance4);
	stream->Read(&m_iPercentPerPlayer);
	stream->Read(&m_iTilesPer);
	stream->Read(&m_iMinLandPercent);
	stream->Read(&m_iUniqueRange);
	stream->Read(&m_iGroupRange);
	stream->Read(&m_iGroupRand);
	stream->Read(&m_bOneArea);
	stream->Read(&m_bHills);
	stream->Read(&m_bFlatlands);
	stream->Read(&m_bNoRiverSide);
	stream->Read(&m_bNormalize);
	stream->ReadString(m_szArtDefineTag);
	SAFE_DELETE_ARRAY(m_piYieldChange);
	m_piYieldChange = new int[NUM_YIELD_TYPES];
	stream->Read(NUM_YIELD_TYPES, m_piYieldChange);
	SAFE_DELETE_ARRAY(m_pbTerrain);
	m_pbTerrain = new bool[GC.getNumTerrainInfos()];
	stream->Read(GC.getNumTerrainInfos(), m_pbTerrain);
	SAFE_DELETE_ARRAY(m_pbFeature);
	m_pbFeature = new bool[GC.getNumFeatureInfos()];
	stream->Read(GC.getNumFeatureInfos(), m_pbFeature);
	SAFE_DELETE_ARRAY(m_pbFeatureTerrain);
	m_pbFeatureTerrain = new bool[GC.getNumTerrainInfos()];
	stream->Read(GC.getNumTerrainInfos(), m_pbFeatureTerrain);
}

void CvBonusInfo::write(FDataStreamBase* stream)
{
	CvInfoBase::write(stream);
	uint uiFlag=0;
	stream->Write(uiFlag);

	stream->Write(m_iBonusClassType);
	stream->Write(m_iChar);
	stream->Write(m_iTechReveal);
	stream->Write(m_iTechCityTrade);
	stream->Write(m_iTechObsolete);
	stream->Write(m_iAITradeModifier);
	stream->Write(m_iAIObjective);
	stream->Write(m_iHealth);
	stream->Write(m_iHappiness);
	stream->Write(m_iMinAreaSize);
	stream->Write(m_iMinLatitude);
	stream->Write(m_iMaxLatitude);
	stream->Write(m_iPlacementOrder);
	stream->Write(m_iConstAppearance);
	stream->Write(m_iRandAppearance1);
	stream->Write(m_iRandAppearance2);
	stream->Write(m_iRandAppearance3);
	stream->Write(m_iRandAppearance4);
	stream->Write(m_iPercentPerPlayer);
	stream->Write(m_iTilesPer);
	stream->Write(m_iMinLandPercent);
	stream->Write(m_iUniqueRange);
	stream->Write(m_iGroupRange);
	stream->Write(m_iGroupRand);
	stream->Write(m_bOneArea);
	stream->Write(m_bHills);
	stream->Write(m_bFlatlands);
	stream->Write(m_bNoRiverSide);
	stream->Write(m_bNormalize);
	stream->WriteString(m_szArtDefineTag);
	stream->Write(NUM_YIELD_TYPES, m_piYieldChange);
	stream->Write(GC.getNumTerrainInfos(), m_pbTerrain);
	stream->Write(GC.getNumFeatureInfos(), m_pbFeature);
	stream->Write(GC.getNumTerrainInfos(), m_pbFeatureTerrain);
}
#endif
bool CvBonusInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvInfoBase::read(pXML))
		return false;

	CvString szTextVal;

	pXML->GetChildXmlValByName(szTextVal, "BonusClassType");
	m_iBonusClassType = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "ArtDefineTag");
	setArtDefineTag(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "TechReveal");
	m_iTechReveal = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "TechCityTrade");
	m_iTechCityTrade = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "TechObsolete");
	m_iTechObsolete = pXML->FindInInfoClass(szTextVal);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"YieldChanges"))
	{
		pXML->SetYields(&m_piYieldChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piYieldChange, NUM_YIELD_TYPES);

	pXML->GetChildXmlValByName(&m_iAITradeModifier, "iAITradeModifier");
	pXML->GetChildXmlValByName(&m_iAIObjective, "iAIObjective");
	pXML->GetChildXmlValByName(&m_iHealth, "iHealth");
	pXML->GetChildXmlValByName(&m_iHappiness, "iHappiness");
	pXML->GetChildXmlValByName(&m_iMinAreaSize, "iMinAreaSize");
	pXML->GetChildXmlValByName(&m_iMinLatitude, "iMinLatitude");
	pXML->GetChildXmlValByName(&m_iMaxLatitude, "iMaxLatitude", 90);
	pXML->GetChildXmlValByName(&m_iPlacementOrder, "iPlacementOrder");
	pXML->GetChildXmlValByName(&m_iConstAppearance, "iConstAppearance");

	// if we can set the current xml node to it's next sibling
	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"Rands"))
	{
		pXML->GetChildXmlValByName(&m_iRandAppearance1, "iRandApp1");
		pXML->GetChildXmlValByName(&m_iRandAppearance2, "iRandApp2");
		pXML->GetChildXmlValByName(&m_iRandAppearance3, "iRandApp3");
		pXML->GetChildXmlValByName(&m_iRandAppearance4, "iRandApp4");

		// set the current xml node to it's parent node
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}

	pXML->GetChildXmlValByName(&m_iPercentPerPlayer, "iPlayer");
	pXML->GetChildXmlValByName(&m_iTilesPer, "iTilesPer");
	pXML->GetChildXmlValByName(&m_iMinLandPercent, "iMinLandPercent");
	pXML->GetChildXmlValByName(&m_iUniqueRange, "iUnique");
	pXML->GetChildXmlValByName(&m_iGroupRange, "iGroupRange");
	pXML->GetChildXmlValByName(&m_iGroupRand, "iGroupRand");
	pXML->GetChildXmlValByName(&m_bOneArea, "bArea");
	pXML->GetChildXmlValByName(&m_bHills, "bHills");
	pXML->GetChildXmlValByName(&m_bFlatlands, "bFlatlands");
	pXML->GetChildXmlValByName(&m_bNoRiverSide, "bNoRiverSide");
	pXML->GetChildXmlValByName(&m_bNormalize, "bNormalize");

	pXML->SetVariableListTagPair(&m_pbTerrain, "TerrainBooleans", GC.getNumTerrainInfos());
	pXML->SetVariableListTagPair(&m_pbFeature, "FeatureBooleans", GC.getNumFeatureInfos());
	pXML->SetVariableListTagPair(&m_pbFeatureTerrain, "FeatureTerrainBooleans", GC.getNumTerrainInfos());

	return true;
}

CvBonusClassInfo::CvBonusClassInfo() : m_iUniqueRange(0) {}


int CvBonusClassInfo::getUniqueRange() const
{
	return m_iUniqueRange;
}

bool CvBonusClassInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvInfoBase::read(pXML))
		return false;
	pXML->GetChildXmlValByName(&m_iUniqueRange, "iUnique");
	return true;
}

CvRouteInfo::CvRouteInfo() :
m_iAdvancedStartCost(0),
m_iAdvancedStartCostIncrease(0),
m_iValue(0),
m_iMovementCost(0),
m_iFlatMovementCost(0),
m_iPrereqBonus(NO_BONUS),
m_piYieldChange(NULL),
m_piTechMovementChange(NULL),
m_piPrereqOrBonuses(NULL)
{}

CvRouteInfo::~CvRouteInfo()
{
	SAFE_DELETE_ARRAY(m_piYieldChange);
	SAFE_DELETE_ARRAY(m_piTechMovementChange);
	SAFE_DELETE_ARRAY(m_piPrereqOrBonuses);
}

int CvRouteInfo::getAdvancedStartCost() const
{
	return m_iAdvancedStartCost;
}

int CvRouteInfo::getAdvancedStartCostIncrease() const
{
	return m_iAdvancedStartCostIncrease;
}

int CvRouteInfo::getValue() const
{
	return m_iValue;
}

int CvRouteInfo::getYieldChange(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvRouteInfo::getYieldChange");
	return m_piYieldChange ? m_piYieldChange[i] : 0; // advc.003t
}

int CvRouteInfo::getTechMovementChange(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumTechInfos(), i, "CvRouteInfo::getTechMovementChange");
	return m_piTechMovementChange ? m_piTechMovementChange[i] : 0; // advc.003t
}

int CvRouteInfo::getPrereqOrBonus(int i) const
{
	return m_piPrereqOrBonuses ? m_piPrereqOrBonuses[i] : NO_BONUS; // advc.003t
}

bool CvRouteInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvInfoBase::read(pXML))
		return false;

	CvString szTextVal;

	pXML->GetChildXmlValByName(&m_iAdvancedStartCost, "iAdvancedStartCost");
	pXML->GetChildXmlValByName(&m_iAdvancedStartCostIncrease, "iAdvancedStartCostIncrease");

	pXML->GetChildXmlValByName(&m_iValue, "iValue");
	pXML->GetChildXmlValByName(&m_iMovementCost, "iMovement");
	pXML->GetChildXmlValByName(&m_iFlatMovementCost, "iFlatMovement");

	pXML->GetChildXmlValByName(szTextVal, "BonusType");
	m_iPrereqBonus = pXML->FindInInfoClass(szTextVal);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"Yields"))
	{
		pXML->SetYields(&m_piYieldChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piYieldChange, NUM_YIELD_TYPES);

	pXML->SetVariableListTagPair(&m_piTechMovementChange, "TechMovementChanges", GC.getNumTechInfos());

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"PrereqOrBonuses"))
	{
		if (pXML->SkipToNextVal())
		{
			int iNumSibs = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
			FAssertMsg(GC.getNUM_ROUTE_PREREQ_OR_BONUSES() > 0, "Allocating zero or less memory in SetGlobalUnitInfo");
			pXML->InitList(&m_piPrereqOrBonuses, GC.getNUM_ROUTE_PREREQ_OR_BONUSES(), -1);

			if (iNumSibs > 0)
			{
				if (pXML->GetChildXmlVal(szTextVal))
				{
					FAssertMsg((iNumSibs <= GC.getNUM_ROUTE_PREREQ_OR_BONUSES()) ,"There are more siblings than memory allocated for them in SetGlobalUnitInfo");
					for (int j = 0; j < iNumSibs; j++)
					{
						m_piPrereqOrBonuses[j] = pXML->FindInInfoClass(szTextVal);
						if (!pXML->GetNextXmlVal(szTextVal))
							break;
					}
					gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
				}
			}
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}

	return true;
}

CvImprovementInfo::CvImprovementInfo() :
m_iAdvancedStartCost(0),
m_iAdvancedStartCostIncrease(0),
m_iTilesPerGoody(0),
m_iGoodyUniqueRange(0),
m_iFeatureGrowthProbability(0),
m_iUpgradeTime(0),
m_iAirBombDefense(0),
m_iDefenseModifier(0),
m_iHappiness(0),
m_iPillageGold(0),
m_iImprovementPillage(NO_IMPROVEMENT),
m_iImprovementUpgrade(NO_IMPROVEMENT),
m_bActsAsCity(false), // advc: was true
m_bHillsMakesValid(false),
m_bFreshWaterMakesValid(false),
m_bRiverSideMakesValid(false),
m_bNoFreshWater(false),
m_bRequiresFlatlands(false),
m_bRequiresRiverSide(false),
m_bRequiresIrrigation(false),
m_bCarriesIrrigation(false),
m_bRequiresFeature(false),
m_bWater(false),
m_bGoody(false),
m_bPermanent(false),
m_bOutsideBorders(false),
m_iWorldSoundscapeScriptId(0),
m_piPrereqNatureYield(NULL),
m_piYieldChange(NULL),
m_piRiverSideYieldChange(NULL),
m_piHillsYieldChange(NULL),
m_piIrrigatedChange(NULL),
m_pbTerrainMakesValid(NULL),
m_pbFeatureMakesValid(NULL),
m_ppiTechYieldChanges(NULL),
m_ppiRouteYieldChanges(NULL),
m_paImprovementBonus(NULL)
{}

CvImprovementInfo::~CvImprovementInfo()
{
	SAFE_DELETE_ARRAY(m_piPrereqNatureYield);
	SAFE_DELETE_ARRAY(m_piYieldChange);
	SAFE_DELETE_ARRAY(m_piRiverSideYieldChange);
	SAFE_DELETE_ARRAY(m_piHillsYieldChange);
	SAFE_DELETE_ARRAY(m_piIrrigatedChange);
	SAFE_DELETE_ARRAY(m_pbTerrainMakesValid);
	SAFE_DELETE_ARRAY(m_pbFeatureMakesValid);

	if (m_paImprovementBonus != NULL)
	{
		SAFE_DELETE_ARRAY(m_paImprovementBonus); // XXX make sure this isn't leaking memory...
		// ^advc: Should be fine; ~CvImprovementBonusInfo gets called.
	}
	if (m_ppiTechYieldChanges != NULL)
	{
		for (int iI = 0; iI < GC.getNumTechInfos(); iI++)
			SAFE_DELETE_ARRAY(m_ppiTechYieldChanges[iI]);
		SAFE_DELETE_ARRAY(m_ppiTechYieldChanges);
	}

	if (m_ppiRouteYieldChanges != NULL)
	{
		for (int iI = 0; iI < GC.getNumRouteInfos(); iI++)
			SAFE_DELETE_ARRAY(m_ppiRouteYieldChanges[iI]);
		SAFE_DELETE_ARRAY(m_ppiRouteYieldChanges);
	}
}

int CvImprovementInfo::getAdvancedStartCost() const
{
	return m_iAdvancedStartCost;
}

int CvImprovementInfo::getAdvancedStartCostIncrease() const
{
	return m_iAdvancedStartCostIncrease;
}

int CvImprovementInfo::getTilesPerGoody() const
{
	return m_iTilesPerGoody;
}

int CvImprovementInfo::getGoodyUniqueRange() const
{
	return m_iGoodyUniqueRange;
}

int CvImprovementInfo::getFeatureGrowthProbability() const
{
	return m_iFeatureGrowthProbability;
}

int CvImprovementInfo::getUpgradeTime() const
{
	return m_iUpgradeTime;
}

int CvImprovementInfo::getPillageGold() const
{
	return m_iPillageGold;
}

int CvImprovementInfo::getImprovementPillage() const
{
	return m_iImprovementPillage;
}

int CvImprovementInfo::getImprovementUpgrade() const
{
	return m_iImprovementUpgrade;
}

const TCHAR* CvImprovementInfo::getArtDefineTag() const
{
	return m_szArtDefineTag;
}

void CvImprovementInfo::setArtDefineTag(const TCHAR* szVal)
{
	m_szArtDefineTag = szVal;
}

int CvImprovementInfo::getWorldSoundscapeScriptId() const
{
	return m_iWorldSoundscapeScriptId;
}

int CvImprovementInfo::getPrereqNatureYield(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvImprovementInfo::getPrereqNatureYield");
	return m_piPrereqNatureYield ? m_piPrereqNatureYield[i] : 0; // advc.003t
}

int* CvImprovementInfo::getPrereqNatureYieldArray()
{
	return m_piPrereqNatureYield;
}

int CvImprovementInfo::getYieldChange(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvImprovementInfo::getYieldChange");
	return m_piYieldChange ? m_piYieldChange[i] : 0; // advc.003t
}

int* CvImprovementInfo::getYieldChangeArray()
{
	return m_piYieldChange;
}

int CvImprovementInfo::getRiverSideYieldChange(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvImprovementInfo::getRiverSideYieldChange");
	return m_piRiverSideYieldChange ? m_piRiverSideYieldChange[i] : 0; // advc.003t
}

int* CvImprovementInfo::getRiverSideYieldChangeArray()
{
	return m_piRiverSideYieldChange;
}

int CvImprovementInfo::getHillsYieldChange(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvImprovementInfo::getHillsYieldChange");
	return m_piHillsYieldChange ? m_piHillsYieldChange[i] : 0; // advc.003t
}

int* CvImprovementInfo::getHillsYieldChangeArray()
{
	return m_piHillsYieldChange;
}

int CvImprovementInfo::getIrrigatedYieldChange(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvImprovementInfo::getIrrigatedYieldChange");
	return m_piIrrigatedChange ? m_piIrrigatedChange[i] : 0; // advc.003t
}

int* CvImprovementInfo::getIrrigatedYieldChangeArray()
{
	return m_piIrrigatedChange;
}

bool CvImprovementInfo::getTerrainMakesValid(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumTerrainInfos(), i, "CvImprovementInfo::getTerrainMakesValid");
	return m_pbTerrainMakesValid ? m_pbTerrainMakesValid[i] : false;
}

bool CvImprovementInfo::getFeatureMakesValid(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumFeatureInfos(), i, "CvImprovementInfo::getFeatureMakesValid");
	return m_pbFeatureMakesValid ? m_pbFeatureMakesValid[i] : false;
}

int CvImprovementInfo::getTechYieldChanges(int i, int j) const
{
	FASSERT_BOUNDS(0, GC.getNumTechInfos(), i, "CvImprovementInfo::getTechYieldChanges");
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, j, "CvImprovementInfo::getTechYieldChanges");
	return m_ppiTechYieldChanges[i][j];
}

int* CvImprovementInfo::getTechYieldChangesArray(int i) const
{
	return m_ppiTechYieldChanges[i];
}

int CvImprovementInfo::getRouteYieldChanges(int i, int j) const
{
	FASSERT_BOUNDS(0, GC.getNumRouteInfos(), i, "CvImprovementInfo::getRouteYieldChanges");
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, j, "CvImprovementInfo::getRouteYieldChanges");
	return m_ppiRouteYieldChanges[i][j];
}

int* CvImprovementInfo::getRouteYieldChangesArray(int i) const
{
	return m_ppiRouteYieldChanges[i];
}

int CvImprovementInfo::getImprovementBonusYield(int i, int j) const
{
	FASSERT_BOUNDS(0, GC.getNumBonusInfos(), i, "CvImprovementInfo::getImprovementBonusYield");
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, j, "CvImprovementInfo::getImprovementBonusYield");
	return m_paImprovementBonus[i].m_piYieldChange ? m_paImprovementBonus[i].getYieldChange(j) : 0; // advc.003t
}

bool CvImprovementInfo::isImprovementBonusMakesValid(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumBonusInfos(), i, "CvImprovementInfo::isImprovementBonusMakesValid");
	return m_paImprovementBonus[i].m_bBonusMakesValid;
}

bool CvImprovementInfo::isImprovementBonusTrade(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumBonusInfos(), i, "CvImprovementInfo::isImprovementBonusTrade");
	return m_paImprovementBonus[i].m_bBonusTrade;
}

int CvImprovementInfo::getImprovementBonusDiscoverRand(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumBonusInfos(), i, "CvImprovementInfo::getImprovementBonusDiscoverRand");
	return m_paImprovementBonus[i].m_iDiscoverRand;
}

const TCHAR* CvImprovementInfo::getButton() const
{
	const CvArtInfoImprovement* pImprovementArtInfo = getArtInfo();
	if (pImprovementArtInfo != NULL)
		return pImprovementArtInfo->getButton();

	return NULL;
}

const CvArtInfoImprovement* CvImprovementInfo::getArtInfo() const
{
	return ARTFILEMGR.getImprovementArtInfo(getArtDefineTag());
}

const TCHAR* CvArtInfoImprovement::getShaderNIF() const
{
	return m_szShaderNIF;
}
void CvArtInfoImprovement::setShaderNIF(const TCHAR* szDesc)
{
	m_szShaderNIF = szDesc;
}
#if SERIALIZE_CVINFOS
void CvImprovementInfo::read(FDataStreamBase* stream)
{
	CvXMLInfo::read(stream); // advc.tag
	uint uiFlag=0;
	stream->Read(&uiFlag);

	stream->Read(&m_iAdvancedStartCost);
	stream->Read(&m_iAdvancedStartCostIncrease);
	stream->Read(&m_iTilesPerGoody);
	stream->Read(&m_iGoodyUniqueRange);
	stream->Read(&m_iFeatureGrowthProbability);
	stream->Read(&m_iUpgradeTime);
	stream->Read(&m_iAirBombDefense);
	stream->Read(&m_iDefenseModifier);
	stream->Read(&m_iHappiness);
	stream->Read(&m_iPillageGold);
	stream->Read(&m_iImprovementPillage);
	stream->Read(&m_iImprovementUpgrade);
	stream->Read(&m_bActsAsCity);
	stream->Read(&m_bHillsMakesValid);
	stream->Read(&m_bFreshWaterMakesValid);
	stream->Read(&m_bRiverSideMakesValid);
	stream->Read(&m_bNoFreshWater);
	stream->Read(&m_bRequiresFlatlands);
	stream->Read(&m_bRequiresRiverSide);
	stream->Read(&m_bRequiresIrrigation);
	stream->Read(&m_bCarriesIrrigation);
	stream->Read(&m_bRequiresFeature);
	stream->Read(&m_bWater);
	stream->Read(&m_bGoody);
	stream->Read(&m_bPermanent);
	stream->Read(&m_bOutsideBorders);
	stream->ReadString(m_szArtDefineTag);
	stream->Read(&m_iWorldSoundscapeScriptId);
	SAFE_DELETE_ARRAY(m_piPrereqNatureYield);
	m_piPrereqNatureYield = new int[NUM_YIELD_TYPES];
	stream->Read(NUM_YIELD_TYPES, m_piPrereqNatureYield);
	SAFE_DELETE_ARRAY(m_piYieldChange);
	m_piYieldChange = new int[NUM_YIELD_TYPES];
	stream->Read(NUM_YIELD_TYPES, m_piYieldChange);
	SAFE_DELETE_ARRAY(m_piRiverSideYieldChange);
	m_piRiverSideYieldChange = new int[NUM_YIELD_TYPES];
	stream->Read(NUM_YIELD_TYPES, m_piRiverSideYieldChange);
	SAFE_DELETE_ARRAY(m_piHillsYieldChange);
	m_piHillsYieldChange = new int[NUM_YIELD_TYPES];
	stream->Read(NUM_YIELD_TYPES, m_piHillsYieldChange);
	SAFE_DELETE_ARRAY(m_piIrrigatedChange);
	m_piIrrigatedChange = new int[NUM_YIELD_TYPES];
	stream->Read(NUM_YIELD_TYPES, m_piIrrigatedChange);
	SAFE_DELETE_ARRAY(m_pbTerrainMakesValid);
	m_pbTerrainMakesValid = new bool[GC.getNumTerrainInfos()];
	stream->Read(GC.getNumTerrainInfos(), m_pbTerrainMakesValid);
	SAFE_DELETE_ARRAY(m_pbFeatureMakesValid);
	m_pbFeatureMakesValid = new bool[GC.getNumFeatureInfos()];
	stream->Read(GC.getNumFeatureInfos(), m_pbFeatureMakesValid);
	SAFE_DELETE_ARRAY(m_paImprovementBonus);
	m_paImprovementBonus = new CvImprovementBonusInfo[GC.getNumBonusInfos()];
	for (int i = 0; i < GC.getNumBonusInfos(); i++)
		m_paImprovementBonus[i].read(stream);
	if (m_ppiTechYieldChanges != NULL)
	{
		for(int i = 0; i < GC.getNumTechInfos(); i++)
			SAFE_DELETE_ARRAY(m_ppiTechYieldChanges[i]);
		SAFE_DELETE_ARRAY(m_ppiTechYieldChanges);
	}
	m_ppiTechYieldChanges = new int*[GC.getNumTechInfos()];
	for(int i = 0; i < GC.getNumTechInfos(); i++)
	{
		m_ppiTechYieldChanges[i]  = new int[NUM_YIELD_TYPES];
		stream->Read(NUM_YIELD_TYPES, m_ppiTechYieldChanges[i]);
	}
	if (m_ppiRouteYieldChanges != NULL)
	{
		for(int i = 0; i < GC.getNumRouteInfos(); i++)
		{
			SAFE_DELETE_ARRAY(m_ppiRouteYieldChanges[i]);
		}
		SAFE_DELETE_ARRAY(m_ppiRouteYieldChanges);
	}
	m_ppiRouteYieldChanges = new int*[GC.getNumRouteInfos()];
	for(int i = 0; i < GC.getNumRouteInfos(); i++)
	{
		m_ppiRouteYieldChanges[i]  = new int[NUM_YIELD_TYPES];
		stream->Read(NUM_YIELD_TYPES, m_ppiRouteYieldChanges[i]);
	}
}

void CvImprovementInfo::write(FDataStreamBase* stream)
{
	CvXMLInfo::write(stream); // advc.tag
	uint uiFlag=0;
	stream->Write(uiFlag);

	stream->Write(m_iAdvancedStartCost);
	stream->Write(m_iAdvancedStartCostIncrease);
	stream->Write(m_iTilesPerGoody);
	stream->Write(m_iGoodyUniqueRange);
	stream->Write(m_iFeatureGrowthProbability);
	stream->Write(m_iUpgradeTime);
	stream->Write(m_iAirBombDefense);
	stream->Write(m_iDefenseModifier);
	stream->Write(m_iHappiness);
	stream->Write(m_iPillageGold);
	stream->Write(m_iImprovementPillage);
	stream->Write(m_iImprovementUpgrade);
	stream->Write(m_bActsAsCity);
	stream->Write(m_bHillsMakesValid);
	stream->Write(m_bFreshWaterMakesValid);
	stream->Write(m_bRiverSideMakesValid);
	stream->Write(m_bNoFreshWater);
	stream->Write(m_bRequiresFlatlands);
	stream->Write(m_bRequiresRiverSide);
	stream->Write(m_bRequiresIrrigation);
	stream->Write(m_bCarriesIrrigation);
	stream->Write(m_bRequiresFeature);
	stream->Write(m_bWater);
	stream->Write(m_bGoody);
	stream->Write(m_bPermanent);
	stream->Write(m_bOutsideBorders);
	stream->WriteString(m_szArtDefineTag);
	stream->Write(m_iWorldSoundscapeScriptId);
	stream->Write(NUM_YIELD_TYPES, m_piPrereqNatureYield);
	stream->Write(NUM_YIELD_TYPES, m_piYieldChange);
	stream->Write(NUM_YIELD_TYPES, m_piRiverSideYieldChange);
	stream->Write(NUM_YIELD_TYPES, m_piHillsYieldChange);
	stream->Write(NUM_YIELD_TYPES, m_piIrrigatedChange);
	stream->Write(GC.getNumTerrainInfos(), m_pbTerrainMakesValid);
	stream->Write(GC.getNumFeatureInfos(), m_pbFeatureMakesValid);
	for (int i = 0; i < GC.getNumBonusInfos(); i++)
		m_paImprovementBonus[i].write(stream);
	for(int i = 0; i < GC.getNumTechInfos(); i++)
		stream->Write(NUM_YIELD_TYPES, m_ppiTechYieldChanges[i]);
	for(int i = 0; i < GC.getNumRouteInfos(); i++)
		stream->Write(NUM_YIELD_TYPES, m_ppiRouteYieldChanges[i]);
}
#endif
// <advc.tag>
void CvImprovementInfo::addElements(std::vector<XMLElement*>& r) const
{
	CvXMLInfo::addElements(r);
	r.push_back(new IntElement(HealthPercent, "HealthPercent", 0)); // advc.901
} // </advc.tag>

bool CvImprovementInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvXMLInfo::read(pXML)) // advc.tag
		return false;

	CvString szTextVal;

	pXML->GetChildXmlValByName(szTextVal, "ArtDefineTag");
	setArtDefineTag(szTextVal);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"PrereqNatureYields"))
	{
		pXML->SetYields(&m_piPrereqNatureYield);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piPrereqNatureYield, NUM_YIELD_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"YieldChanges"))
	{
		pXML->SetYields(&m_piYieldChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piYieldChange, NUM_YIELD_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"RiverSideYieldChange"))
	{
		pXML->SetYields(&m_piRiverSideYieldChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piRiverSideYieldChange, NUM_YIELD_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"HillsYieldChange"))
	{
		pXML->SetYields(&m_piHillsYieldChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piHillsYieldChange, NUM_YIELD_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"IrrigatedYieldChange"))
	{
		pXML->SetYields(&m_piIrrigatedChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piIrrigatedChange, NUM_YIELD_TYPES);

	pXML->GetChildXmlValByName(&m_iAdvancedStartCost, "iAdvancedStartCost");
	pXML->GetChildXmlValByName(&m_iAdvancedStartCostIncrease, "iAdvancedStartCostIncrease");
	pXML->GetChildXmlValByName(&m_bActsAsCity, "bActsAsCity");
	pXML->GetChildXmlValByName(&m_bHillsMakesValid, "bHillsMakesValid");
	pXML->GetChildXmlValByName(&m_bFreshWaterMakesValid, "bFreshWaterMakesValid");
	pXML->GetChildXmlValByName(&m_bRiverSideMakesValid, "bRiverSideMakesValid");
	pXML->GetChildXmlValByName(&m_bNoFreshWater, "bNoFreshWater");
	pXML->GetChildXmlValByName(&m_bRequiresFlatlands, "bRequiresFlatlands");
	pXML->GetChildXmlValByName(&m_bRequiresRiverSide, "bRequiresRiverSide");
	pXML->GetChildXmlValByName(&m_bRequiresIrrigation, "bRequiresIrrigation");
	pXML->GetChildXmlValByName(&m_bCarriesIrrigation, "bCarriesIrrigation");
	pXML->GetChildXmlValByName(&m_bRequiresFeature, "bRequiresFeature");
	pXML->GetChildXmlValByName(&m_bWater, "bWater");
	pXML->GetChildXmlValByName(&m_bGoody, "bGoody");
	pXML->GetChildXmlValByName(&m_bPermanent, "bPermanent");
	pXML->GetChildXmlValByName(&m_iTilesPerGoody, "iTilesPerGoody");
	pXML->GetChildXmlValByName(&m_iGoodyUniqueRange, "iGoodyRange");
	pXML->GetChildXmlValByName(&m_iFeatureGrowthProbability, "iFeatureGrowth");
	pXML->GetChildXmlValByName(&m_iUpgradeTime, "iUpgradeTime");
	pXML->GetChildXmlValByName(&m_iAirBombDefense, "iAirBombDefense");
	pXML->GetChildXmlValByName(&m_iDefenseModifier, "iDefenseModifier");
	pXML->GetChildXmlValByName(&m_iHappiness, "iHappiness");
	pXML->GetChildXmlValByName(&m_iPillageGold, "iPillageGold");
	pXML->GetChildXmlValByName(&m_bOutsideBorders, "bOutsideBorders");

	pXML->SetVariableListTagPair(&m_pbTerrainMakesValid, "TerrainMakesValids", GC.getNumTerrainInfos());
	pXML->SetVariableListTagPair(&m_pbFeatureMakesValid, "FeatureMakesValids", GC.getNumFeatureInfos());

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"BonusTypeStructs"))
	{
		pXML->SetImprovementBonuses(&m_paImprovementBonus);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitImprovementBonusList(&m_paImprovementBonus, GC.getNumBonusInfos());

	for (int i = 0; i < 2; i++) // advc: Reduce code duplication
	{
		bool bTech = (i == 0);
		int iNumInfos = (bTech ? GC.getNumTechInfos() : GC.getNumRouteInfos());
		FAssert(iNumInfos > 0);
		int*** pppiYieldChanges = &(bTech ? m_ppiTechYieldChanges : m_ppiRouteYieldChanges);
		pXML->Init2DIntList(pppiYieldChanges, iNumInfos, NUM_YIELD_TYPES);
		if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), bTech ?
			"TechYieldChanges" : "RouteYieldChanges"))
		{
			if (pXML->SkipToNextVal())
			{
				int iNumSibs = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
				if (gDLL->getXMLIFace()->SetToChild(pXML->GetXML()))
				{
					if (iNumSibs > 0)
					{
						for (int j = 0; j < iNumSibs; j++)
						{
							pXML->GetChildXmlValByName(szTextVal, bTech ?
									"PrereqTech" : "RouteType");
							int iIndex = pXML->FindInInfoClass(szTextVal);
							if (iIndex > -1)
							{
								SAFE_DELETE_ARRAY((*pppiYieldChanges)[iIndex]);
								if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), bTech ?
									"TechYields": "RouteYields"))
								{
									pXML->SetYields(&(*pppiYieldChanges)[iIndex]);
									gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
								}
								else pXML->InitList(&(*pppiYieldChanges)[iIndex], NUM_YIELD_TYPES);
							}
							if (!gDLL->getXMLIFace()->NextSibling(pXML->GetXML()))
								break;
						}
					}
					gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
				}
			}
			gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
		}
	}

	pXML->GetChildXmlValByName(szTextVal, "WorldSoundscapeAudioScript", /* advc.006b: */ "");
	if (szTextVal.GetLength() > 0)
		m_iWorldSoundscapeScriptId = gDLL->getAudioTagIndex(szTextVal.GetCString(), AUDIOTAG_SOUNDSCAPE);
	else m_iWorldSoundscapeScriptId = -1;

	return true;
}

bool CvImprovementInfo::readPass2(CvXMLLoadUtility* pXML)
{
	CvString szTextVal;

	pXML->GetChildXmlValByName(szTextVal, "ImprovementPillage");
	m_iImprovementPillage = GC.getInfoTypeForString(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "ImprovementUpgrade");
	m_iImprovementUpgrade = GC.getInfoTypeForString(szTextVal);

	return true;
}

CvImprovementBonusInfo::CvImprovementBonusInfo() :
m_iDiscoverRand(0),
m_bBonusMakesValid(false),
m_bBonusTrade(false),
m_piYieldChange(NULL)
{}

CvImprovementBonusInfo::~CvImprovementBonusInfo()
{
	SAFE_DELETE_ARRAY(m_piYieldChange);
}

int CvImprovementBonusInfo::getDiscoverRand() const
{
	return m_iDiscoverRand;
}

bool CvImprovementBonusInfo::isBonusMakesValid() const
{
	return m_bBonusMakesValid;
}

bool CvImprovementBonusInfo::isBonusTrade() const
{
	return m_bBonusTrade;
}

int CvImprovementBonusInfo::getYieldChange(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvImprovementBonusInfo::getYieldChange");
	return m_piYieldChange ? m_piYieldChange[i] : 0; // advc.003t
}
#if SERIALIZE_CVINFOS
void CvImprovementBonusInfo::read(FDataStreamBase* stream)
{
	CvInfoBase::read(stream);
	uint uiFlag=0;
	stream->Read(&uiFlag);
	stream->Read(&m_iDiscoverRand);
	stream->Read(&m_bBonusMakesValid);
	stream->Read(&m_bBonusTrade);
	SAFE_DELETE_ARRAY(m_piYieldChange);
	m_piYieldChange = new int[NUM_YIELD_TYPES];
	stream->Read(NUM_YIELD_TYPES, m_piYieldChange);
}

void CvImprovementBonusInfo::write(FDataStreamBase* stream)
{
	CvInfoBase::write(stream);
	uint uiFlag=0;
	stream->Write(uiFlag);
	stream->Write(m_iDiscoverRand);
	stream->Write(m_bBonusMakesValid);
	stream->Write(m_bBonusTrade);
	stream->Write(NUM_YIELD_TYPES, m_piYieldChange);
}
#endif

CvGoodyInfo::CvGoodyInfo() :
m_iGold(0),
m_iGoldRand1(0),
m_iGoldRand2(0),
m_iMapOffset(0),
m_iMapRange(0),
m_iMapProb(0),
m_iExperience(0),
m_iHealing(0),
m_iDamagePrereq(0),
m_iBarbarianUnitProb(0),
m_iMinBarbarians(0),
m_iUnitClassType(NO_UNITCLASS),
m_iBarbarianUnitClass(NO_UNITCLASS),
m_bTech(false),
m_bBad(false)
{}

int CvGoodyInfo::getGold() const
{
	return m_iGold;
}

int CvGoodyInfo::getGoldRand1() const
{
	return m_iGoldRand1;
}

int CvGoodyInfo::getGoldRand2() const
{
	return m_iGoldRand2;
}

int CvGoodyInfo::getMapOffset() const
{
	return m_iMapOffset;
}

int CvGoodyInfo::getMapRange() const
{
	return m_iMapRange;
}

int CvGoodyInfo::getMapProb() const
{
	return m_iMapProb;
}

int CvGoodyInfo::getExperience() const
{
	return m_iExperience;
}

int CvGoodyInfo::getHealing() const
{
	return m_iHealing;
}

int CvGoodyInfo::getDamagePrereq() const
{
	return m_iDamagePrereq;
}

int CvGoodyInfo::getBarbarianUnitProb() const
{
	return m_iBarbarianUnitProb;
}

int CvGoodyInfo::getMinBarbarians() const
{
	return m_iMinBarbarians;
}

int CvGoodyInfo::getUnitClassType() const
{
	return m_iUnitClassType;
}

int CvGoodyInfo::getBarbarianUnitClass() const
{
	return m_iBarbarianUnitClass;
}

bool CvGoodyInfo::isTech() const
{
	return m_bTech;
}

bool CvGoodyInfo::isBad() const
{
	return m_bBad;
}

const TCHAR* CvGoodyInfo::getSound() const
{
	return m_szSound;
}

void CvGoodyInfo::setSound(const TCHAR* szVal)
{
	m_szSound=szVal;
}

bool CvGoodyInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvInfoBase::read(pXML))
		return false;

	CvString szTextVal;
	pXML->GetChildXmlValByName(szTextVal, "Sound");
	setSound(szTextVal);

	pXML->GetChildXmlValByName(&m_iGold, "iGold");
	pXML->GetChildXmlValByName(&m_iGoldRand1, "iGoldRand1");
	pXML->GetChildXmlValByName(&m_iGoldRand2, "iGoldRand2");
	pXML->GetChildXmlValByName(&m_iMapOffset, "iMapOffset");
	pXML->GetChildXmlValByName(&m_iMapRange, "iMapRange");
	pXML->GetChildXmlValByName(&m_iMapProb, "iMapProb");
	pXML->GetChildXmlValByName(&m_iExperience, "iExperience");
	pXML->GetChildXmlValByName(&m_iHealing, "iHealing");
	pXML->GetChildXmlValByName(&m_iDamagePrereq, "iDamagePrereq");
	pXML->GetChildXmlValByName(&m_bTech, "bTech");
	pXML->GetChildXmlValByName(&m_bBad, "bBad");

	pXML->GetChildXmlValByName(szTextVal, "UnitClass");
	m_iUnitClassType = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "BarbarianClass");
	m_iBarbarianUnitClass = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(&m_iBarbarianUnitProb, "iBarbarianUnitProb");
	pXML->GetChildXmlValByName(&m_iMinBarbarians, "iMinBarbarians");

	return true;
}
