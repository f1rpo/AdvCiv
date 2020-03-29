// advc.003x: Cut from CvInfos.cpp

#include "CvGameCoreDLL.h"
#include "CvXMLLoadUtility.h"
#include "CvDLLXMLIFaceBase.h"


CvCivilizationInfo::CvCivilizationInfo() :
m_iDefaultPlayerColor(NO_PLAYERCOLOR),
m_iArtStyleType(NO_ARTSTYLE),
m_iUnitArtStyleType(NO_UNIT_ARTSTYLE),
m_iNumCityNames(0),
m_iNumLeaders(0),
m_iSelectionSoundScriptId(0),
m_iActionSoundScriptId(0),
m_iDerivativeCiv(NO_CIVILIZATION),
m_bPlayable(false),
m_bAIPlayable(false),
m_piCivilizationBuildings(NULL),
m_piCivilizationUnits(NULL),
m_piCivilizationFreeUnitsClass(NULL),
m_piCivilizationInitialCivics(NULL),
m_pbLeaders(NULL),
m_pbCivilizationFreeBuildingClass(NULL),
m_pbCivilizationFreeTechs(NULL),
m_pbCivilizationDisableTechs(NULL),
m_paszCityNames(NULL)
{}

CvCivilizationInfo::~CvCivilizationInfo()
{
	SAFE_DELETE_ARRAY(m_piCivilizationBuildings);
	SAFE_DELETE_ARRAY(m_piCivilizationUnits);
	SAFE_DELETE_ARRAY(m_piCivilizationFreeUnitsClass);
	SAFE_DELETE_ARRAY(m_piCivilizationInitialCivics);
	SAFE_DELETE_ARRAY(m_pbLeaders);
	SAFE_DELETE_ARRAY(m_pbCivilizationFreeBuildingClass);
	SAFE_DELETE_ARRAY(m_pbCivilizationFreeTechs);
	SAFE_DELETE_ARRAY(m_pbCivilizationDisableTechs);
	SAFE_DELETE_ARRAY(m_paszCityNames);
}

void CvCivilizationInfo::reset()
{
	CvInfoBase::reset();

	m_aszAdjective.clear();
	m_aszShortDescription.clear();
}

int CvCivilizationInfo::getDerivativeCiv() const
{
	return m_iDerivativeCiv;
}

int CvCivilizationInfo::getDefaultPlayerColor() const
{
	return m_iDefaultPlayerColor;
}

int CvCivilizationInfo::getArtStyleType() const
{
	return m_iArtStyleType;
}

int CvCivilizationInfo::getUnitArtStyleType() const
{
	return m_iUnitArtStyleType;
}

int CvCivilizationInfo::getNumCityNames() const
{
	return m_iNumCityNames;
}

int CvCivilizationInfo::getNumLeaders() const
{
	return m_iNumLeaders;
}

int CvCivilizationInfo::getSelectionSoundScriptId() const
{
	return m_iSelectionSoundScriptId;
}

int CvCivilizationInfo::getActionSoundScriptId() const
{
	return m_iActionSoundScriptId;
}

bool CvCivilizationInfo::isAIPlayable() const
{
	return m_bAIPlayable;
}

bool CvCivilizationInfo::isPlayable() const
{
	return m_bPlayable;
}

const wchar* CvCivilizationInfo::getShortDescription(uint uiForm)
{
	while(m_aszShortDescription.size() <= uiForm)
	{
		m_aszShortDescription.push_back(gDLL->getObjectText(m_szShortDescriptionKey,
			m_aszShortDescription.size()));
	}
	return m_aszShortDescription[uiForm];
}

const wchar* CvCivilizationInfo::getShortDescriptionKey() const
{
	return m_szShortDescriptionKey;
}

const wchar* CvCivilizationInfo::getAdjective(uint uiForm)
{
	while(m_aszAdjective.size() <= uiForm)
		m_aszAdjective.push_back(gDLL->getObjectText(m_szAdjectiveKey, m_aszAdjective.size()));

	return m_aszAdjective[uiForm];
}

const wchar* CvCivilizationInfo::getAdjectiveKey() const
{
	return m_szAdjectiveKey;
}

const TCHAR* CvCivilizationInfo::getFlagTexture() const
{
	return ARTFILEMGR.getCivilizationArtInfo(getArtDefineTag())->getPath();
}

const TCHAR* CvCivilizationInfo::getArtDefineTag() const
{
	return m_szArtDefineTag;
}

void CvCivilizationInfo::setArtDefineTag(const TCHAR* szVal)
{
	m_szArtDefineTag = szVal;
}

BuildingTypes CvCivilizationInfo::getCivilizationBuildings(int i) const
{
	FAssertBounds(0, GC.getNumBuildingClassInfos(), i);
	return m_piCivilizationBuildings ? (BuildingTypes)m_piCivilizationBuildings[i] : NO_BUILDING; // advc.003t
}

UnitTypes CvCivilizationInfo::getCivilizationUnits(int i) const
{
	FAssertBounds(0, GC.getNumUnitClassInfos(), i);
	return m_piCivilizationUnits ? (UnitTypes)m_piCivilizationUnits[i] : NO_UNIT; // advc.003t
}

int CvCivilizationInfo::getCivilizationFreeUnitsClass(int i) const
{
	FAssertBounds(0, GC.getNumUnitClassInfos(), i);
	return m_piCivilizationFreeUnitsClass ? m_piCivilizationFreeUnitsClass[i] : NO_UNITCLASS; // advc.003t
}

int CvCivilizationInfo::getCivilizationInitialCivics(int i) const
{
	FAssertBounds(0, GC.getNumCivicOptionInfos(), i);
	return m_piCivilizationInitialCivics ? m_piCivilizationInitialCivics[i] : NO_CIVIC; // advc.003t
}

bool CvCivilizationInfo::isLeaders(int i) const
{
	FAssertBounds(0, GC.getNumLeaderHeadInfos(), i);
	return m_pbLeaders ? m_pbLeaders[i] : false;
}

bool CvCivilizationInfo::isCivilizationFreeBuildingClass(int i) const
{
	FAssertBounds(0, GC.getNumBuildingClassInfos(), i);
	return m_pbCivilizationFreeBuildingClass ? m_pbCivilizationFreeBuildingClass[i] : false;
}

bool CvCivilizationInfo::isCivilizationFreeTechs(int i) const
{
	FAssertBounds(0, GC.getNumTechInfos(), i);
	return m_pbCivilizationFreeTechs ? m_pbCivilizationFreeTechs[i] : false;
}

bool CvCivilizationInfo::isCivilizationDisableTechs(int i) const
{
	FAssertBounds(0, GC.getNumTechInfos(), i);
	return m_pbCivilizationDisableTechs ? m_pbCivilizationDisableTechs[i] : false;
}

const CvArtInfoCivilization* CvCivilizationInfo::getArtInfo() const
{
	return ARTFILEMGR.getCivilizationArtInfo(getArtDefineTag());
}

const TCHAR* CvCivilizationInfo::getButton() const
{
	return getArtInfo()->getButton();
}

std::string CvCivilizationInfo::getCityNames(int i) const
{
	FAssertMsg(i < getNumCityNames(), "Index out of bounds");
	FAssertMsg(i > -1, "Index out of bounds");
	return m_paszCityNames[i];
}

#if SERIALIZE_CVINFOS
void CvCivilizationInfo::read(FDataStreamBase* stream)
{
	CvInfoBase::read(stream);
	uint uiFlag=0;
	stream->Read(&uiFlag);

	stream->Read(&m_iDefaultPlayerColor);
	stream->Read(&m_iArtStyleType);
	stream->Read(&m_iUnitArtStyleType); // FlavorUnits by Impaler[WrG]
	stream->Read(&m_iNumCityNames);
	stream->Read(&m_iNumLeaders);
	stream->Read(&m_iSelectionSoundScriptId);
	stream->Read(&m_iActionSoundScriptId);
	stream->Read(&m_iDerivativeCiv);
	stream->Read(&m_bAIPlayable);
	stream->Read(&m_bPlayable);
	stream->ReadString(m_szArtDefineTag);
	stream->ReadString(m_szShortDescriptionKey);
	stream->ReadString(m_szAdjectiveKey);
	SAFE_DELETE_ARRAY(m_piCivilizationBuildings);
	m_piCivilizationBuildings = new int[GC.getNumBuildingClassInfos()];
	stream->Read(GC.getNumBuildingClassInfos(), m_piCivilizationBuildings);
	SAFE_DELETE_ARRAY(m_piCivilizationUnits);
	m_piCivilizationUnits = new int[GC.getNumUnitClassInfos()];
	stream->Read(GC.getNumUnitClassInfos(), m_piCivilizationUnits);
	SAFE_DELETE_ARRAY(m_piCivilizationFreeUnitsClass);
	m_piCivilizationFreeUnitsClass = new int[GC.getNumUnitClassInfos()];
	stream->Read(GC.getNumUnitClassInfos(), m_piCivilizationFreeUnitsClass);
	SAFE_DELETE_ARRAY(m_piCivilizationInitialCivics);
	m_piCivilizationInitialCivics = new int[GC.getNumCivicOptionInfos()];
	stream->Read(GC.getNumCivicOptionInfos(), m_piCivilizationInitialCivics);
	SAFE_DELETE_ARRAY(m_pbLeaders);
	m_pbLeaders = new bool[GC.getNumLeaderHeadInfos()];
	stream->Read(GC.getNumLeaderHeadInfos(), m_pbLeaders);
	SAFE_DELETE_ARRAY(m_pbCivilizationFreeBuildingClass);
	m_pbCivilizationFreeBuildingClass = new bool[GC.getNumBuildingClassInfos()];
	stream->Read(GC.getNumBuildingClassInfos(), m_pbCivilizationFreeBuildingClass);
	SAFE_DELETE_ARRAY(m_pbCivilizationFreeTechs);
	m_pbCivilizationFreeTechs = new bool[GC.getNumTechInfos()];
	stream->Read(GC.getNumTechInfos(), m_pbCivilizationFreeTechs);
	SAFE_DELETE_ARRAY(m_pbCivilizationDisableTechs);
	m_pbCivilizationDisableTechs = new bool[GC.getNumTechInfos()];
	stream->Read(GC.getNumTechInfos(), m_pbCivilizationDisableTechs);
	SAFE_DELETE_ARRAY(m_paszCityNames);
	m_paszCityNames = new CvString[m_iNumCityNames];
	stream->ReadString(m_iNumCityNames, m_paszCityNames);
}

void CvCivilizationInfo::write(FDataStreamBase* stream)
{
	CvInfoBase::write(stream);
	uint uiFlag=0;
	stream->Write(uiFlag);

	stream->Write(m_iDefaultPlayerColor);
	stream->Write(m_iArtStyleType);
	stream->Write(m_iUnitArtStyleType);
	stream->Write(m_iNumCityNames);
	stream->Write(m_iNumLeaders);
	stream->Write(m_iSelectionSoundScriptId);
	stream->Write(m_iActionSoundScriptId);
	stream->Write(m_iDerivativeCiv);
	stream->Write(m_bAIPlayable);
	stream->Write(m_bPlayable);
	stream->WriteString(m_szArtDefineTag);
	stream->WriteString(m_szShortDescriptionKey);
	stream->WriteString(m_szAdjectiveKey);
	stream->Write(GC.getNumBuildingClassInfos(), m_piCivilizationBuildings);
	stream->Write(GC.getNumUnitClassInfos(), m_piCivilizationUnits);
	stream->Write(GC.getNumUnitClassInfos(), m_piCivilizationFreeUnitsClass);
	stream->Write(GC.getNumCivicOptionInfos(), m_piCivilizationInitialCivics);
	stream->Write(GC.getNumLeaderHeadInfos(), m_pbLeaders);
	stream->Write(GC.getNumBuildingClassInfos(), m_pbCivilizationFreeBuildingClass);
	stream->Write(GC.getNumTechInfos(), m_pbCivilizationFreeTechs);
	stream->Write(GC.getNumTechInfos(), m_pbCivilizationDisableTechs);
	stream->WriteString(m_iNumCityNames, m_paszCityNames);
}
#endif
bool CvCivilizationInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvInfoBase::read(pXML))
		return false;

	char szClassVal[256]; // holds the text value of the relevant classinfo
	CvString szTextVal;

	// Get the Text from Text/Civ4GameTextXML.xml
	pXML->GetChildXmlValByName(m_szShortDescriptionKey, "ShortDescription");
	pXML->GetChildXmlValByName(m_szAdjectiveKey, "Adjective");

	pXML->GetChildXmlValByName(szTextVal, "DefaultPlayerColor");
	m_iDefaultPlayerColor = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "ArtDefineTag");
	setArtDefineTag(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "ArtStyleType");
	m_iArtStyleType = GC.getTypesEnum(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "UnitArtStyleType",
			""); // advc.006b: Barbarians and Minor Civs don't have one
	m_iUnitArtStyleType = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "CivilizationSelectionSound");
	m_iSelectionSoundScriptId = (szTextVal.GetLength() > 0) ? gDLL->getAudioTagIndex(szTextVal.GetCString(), AUDIOTAG_3DSCRIPT) : -1;
	pXML->GetChildXmlValByName(szTextVal, "CivilizationActionSound");
	m_iActionSoundScriptId = (szTextVal.GetLength() > 0) ? gDLL->getAudioTagIndex(szTextVal.GetCString(), AUDIOTAG_3DSCRIPT) : -1;

	pXML->GetChildXmlValByName(&m_bPlayable, "bPlayable");
	pXML->GetChildXmlValByName(&m_bAIPlayable, "bAIPlayable");
	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "Cities"))
	{
		pXML->SetStringList(&m_paszCityNames, &m_iNumCityNames);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	// advc.003: Reduce code duplication
	for (int i = 0; i < 2; i++)
	{
		bool bBuildings = (i == 0);
		if (!gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),
			bBuildings ? "Buildings" : "Units"))
		{
			continue;
		}
		if (pXML->SkipToNextVal())
		{
			int** piUniques;
			if (bBuildings)
			{
				piUniques = &m_piCivilizationBuildings;
				pXML->InitBuildingDefaults(piUniques);
			}
			else
			{
				piUniques = &m_piCivilizationUnits;
				pXML->InitUnitDefaults(piUniques);
			}
			int iNumSibs = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
			if (iNumSibs > 0 && gDLL->getXMLIFace()->SetToChild(pXML->GetXML()))
			{
				FAssert(iNumSibs <= (bBuildings ? GC.getNumBuildingClassInfos() : GC.getNumUnitClassInfos()));
				for (int j = 0; j < iNumSibs; j++)
				{
					if (pXML->GetChildXmlVal(szClassVal))
					{
						int iClassIndex = pXML->FindInInfoClass(szClassVal);
						if (iClassIndex > -1)
						{
							pXML->GetNextXmlVal( szTextVal);
							(*piUniques)[iClassIndex] = pXML->FindInInfoClass(szTextVal);
						}
						else FAssert(iClassIndex > -1);
						gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
					}
					if (!gDLL->getXMLIFace()->NextSibling(pXML->GetXML()))
						break;
				}
				gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
			}
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}

	pXML->SetVariableListTagPair(&m_piCivilizationFreeUnitsClass, "FreeUnitClasses", GC.getNumUnitClassInfos());
	pXML->SetVariableListTagPair(&m_pbCivilizationFreeBuildingClass, "FreeBuildingClasses", GC.getNumBuildingClassInfos());
	pXML->SetVariableListTagPair(&m_pbCivilizationFreeTechs, "FreeTechs", GC.getNumTechInfos());
	pXML->SetVariableListTagPair(&m_pbCivilizationDisableTechs, "DisableTechs", GC.getNumTechInfos());

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"InitialCivics"))
	{
		if (pXML->SkipToNextVal())
		{
			int iNumSibs = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
			pXML->InitList(&m_piCivilizationInitialCivics, GC.getNumCivicOptionInfos());
			if (iNumSibs > 0)
			{
				if (pXML->GetChildXmlVal(szTextVal))
				{
					FAssert(iNumSibs <= GC.getNumCivicOptionInfos());
					for (int j = 0; j < iNumSibs; j++)
					{
						m_piCivilizationInitialCivics[j] = pXML->FindInInfoClass(szTextVal);
						if (!pXML->GetNextXmlVal(szTextVal))
							break;
					}
					gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
				}
			}
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}

	pXML->SetVariableListTagPair(&m_pbLeaders, "Leaders", GC.getNumLeaderHeadInfos());
	pXML->GetChildXmlValByName(szTextVal, "CivilizationSelectionSound");

	return true;
}

bool CvCivilizationInfo::readPass2(CvXMLLoadUtility* pXML)
{
	CvString szTextVal;
	pXML->GetChildXmlValByName(szTextVal, "DerivativeCiv");
	m_iDerivativeCiv = GC.getInfoTypeForString(szTextVal);

	return true;
}

CvLeaderHeadInfo::CvLeaderHeadInfo() :
m_iWonderConstructRand(0),
m_iBaseAttitude(0),
m_iBasePeaceWeight(0),
m_iPeaceWeightRand(0),
m_iWarmongerRespect(0),
m_iEspionageWeight(0),
m_iRefuseToTalkWarThreshold(0),
m_iNoTechTradeThreshold(0),
m_iTechTradeKnownPercent(0),
m_iMaxGoldTradePercent(0),
m_iMaxGoldPerTurnTradePercent(0),
// BETTER_BTS_AI_MOD, Victory Strategy AI, 03/21/10, jdog5000: START
m_iCultureVictoryWeight(0),
m_iSpaceVictoryWeight(0),
m_iConquestVictoryWeight(0),
m_iDominationVictoryWeight(0),
m_iDiplomacyVictoryWeight(0),
// BETTER_BTS_AI_MOD: END
m_iMaxWarRand(0),
m_iMaxWarNearbyPowerRatio(0),
m_iMaxWarDistantPowerRatio(0),
m_iMaxWarMinAdjacentLandPercent(0),
m_iLimitedWarRand(0),
m_iLimitedWarPowerRatio(0),
m_iDogpileWarRand(0),
m_iMakePeaceRand(0),
m_iDeclareWarTradeRand(0),
m_iDemandRebukedSneakProb(0),
m_iDemandRebukedWarProb(0),
m_iRazeCityProb(0),
m_iBuildUnitProb(0),
m_iBaseAttackOddsChange(0),
m_iAttackOddsChangeRand(0),
m_iWorseRankDifferenceAttitudeChange(0),
m_iBetterRankDifferenceAttitudeChange(0),
m_iCloseBordersAttitudeChange(0),
m_iLostWarAttitudeChange(0),
m_iAtWarAttitudeDivisor(0),
m_iAtWarAttitudeChangeLimit(0),
m_iAtPeaceAttitudeDivisor(0),
m_iAtPeaceAttitudeChangeLimit(0),
m_iSameReligionAttitudeChange(0),
m_iSameReligionAttitudeDivisor(0),
m_iSameReligionAttitudeChangeLimit(0),
m_iDifferentReligionAttitudeChange(0),
m_iDifferentReligionAttitudeDivisor(0),
m_iDifferentReligionAttitudeChangeLimit(0),
m_iBonusTradeAttitudeDivisor(0),
m_iBonusTradeAttitudeChangeLimit(0),
m_iOpenBordersAttitudeDivisor(0),
m_iOpenBordersAttitudeChangeLimit(0),
m_iDefensivePactAttitudeDivisor(0),
m_iDefensivePactAttitudeChangeLimit(0),
m_iShareWarAttitudeChange(0),
m_iShareWarAttitudeDivisor(0),
m_iShareWarAttitudeChangeLimit(0),
m_iFavoriteCivicAttitudeChange(0),
m_iFavoriteCivicAttitudeDivisor(0),
m_iFavoriteCivicAttitudeChangeLimit(0),
m_iDemandTributeAttitudeThreshold(NO_ATTITUDE),
m_iNoGiveHelpAttitudeThreshold(NO_ATTITUDE),
m_iTechRefuseAttitudeThreshold(NO_ATTITUDE),
// <advc.ctr> Default values
m_iCityRefuseAttitudeThreshold(ATTITUDE_CAUTIOUS),
m_iNativeCityRefuseAttitudeThreshold(ATTITUDE_PLEASED), // </advc.ctr>
m_iStrategicBonusRefuseAttitudeThreshold(NO_ATTITUDE),
m_iHappinessBonusRefuseAttitudeThreshold(NO_ATTITUDE),
m_iHealthBonusRefuseAttitudeThreshold(NO_ATTITUDE),
m_iMapRefuseAttitudeThreshold(NO_ATTITUDE),
m_iDeclareWarRefuseAttitudeThreshold(NO_ATTITUDE),
m_iDeclareWarThemRefuseAttitudeThreshold(NO_ATTITUDE),
m_iStopTradingRefuseAttitudeThreshold(NO_ATTITUDE),
m_iStopTradingThemRefuseAttitudeThreshold(NO_ATTITUDE),
m_iAdoptCivicRefuseAttitudeThreshold(NO_ATTITUDE),
m_iConvertReligionRefuseAttitudeThreshold(NO_ATTITUDE),
m_iOpenBordersRefuseAttitudeThreshold(NO_ATTITUDE),
m_iDefensivePactRefuseAttitudeThreshold(NO_ATTITUDE),
m_iPermanentAllianceRefuseAttitudeThreshold(NO_ATTITUDE),
m_iVassalRefuseAttitudeThreshold(NO_ATTITUDE),
m_iVassalPowerModifier(0),
m_iFreedomAppreciation(0),
m_iFavoriteCivic(NO_CIVIC),
m_iFavoriteReligion(NO_RELIGION),
m_pbTraits(NULL),
m_piFlavorValue(NULL),
m_piContactRand(NULL),
m_piContactDelay(NULL),
m_piMemoryDecayRand(NULL),
m_piMemoryAttitudePercent(NULL),
m_piNoWarAttitudeProb(NULL),
m_piUnitAIWeightModifier(NULL),
m_piImprovementWeightModifier(NULL),
m_piDiploPeaceIntroMusicScriptIds(NULL),
m_piDiploPeaceMusicScriptIds(NULL),
m_piDiploWarIntroMusicScriptIds(NULL),
m_piDiploWarMusicScriptIds(NULL)
{}

CvLeaderHeadInfo::~CvLeaderHeadInfo()
{
	SAFE_DELETE_ARRAY(m_pbTraits);
	SAFE_DELETE_ARRAY(m_piFlavorValue);
	SAFE_DELETE_ARRAY(m_piContactRand);
	SAFE_DELETE_ARRAY(m_piContactDelay);
	SAFE_DELETE_ARRAY(m_piMemoryDecayRand);
	SAFE_DELETE_ARRAY(m_piMemoryAttitudePercent);
	SAFE_DELETE_ARRAY(m_piNoWarAttitudeProb);
	SAFE_DELETE_ARRAY(m_piUnitAIWeightModifier);
	SAFE_DELETE_ARRAY(m_piImprovementWeightModifier);
	SAFE_DELETE_ARRAY(m_piDiploPeaceIntroMusicScriptIds);
	SAFE_DELETE_ARRAY(m_piDiploPeaceMusicScriptIds);
	SAFE_DELETE_ARRAY(m_piDiploWarIntroMusicScriptIds);
	SAFE_DELETE_ARRAY(m_piDiploWarMusicScriptIds);
}

const TCHAR* CvLeaderHeadInfo::getButton() const
{
	const CvArtInfoLeaderhead * pLeaderheadArtInfo;
	pLeaderheadArtInfo = getArtInfo();
	if (pLeaderheadArtInfo != NULL)
		return pLeaderheadArtInfo->getButton();

	else return NULL;
}

int CvLeaderHeadInfo::getWonderConstructRand() const
{
	return m_iWonderConstructRand;
}

int CvLeaderHeadInfo::getBaseAttitude() const
{
	return m_iBaseAttitude;
}

int CvLeaderHeadInfo::getBasePeaceWeight() const
{
	return m_iBasePeaceWeight;
}

int CvLeaderHeadInfo::getPeaceWeightRand() const
{
	return m_iPeaceWeightRand;
}

int CvLeaderHeadInfo::getWarmongerRespect() const
{
	return m_iWarmongerRespect;
}

int CvLeaderHeadInfo::getEspionageWeight() const
{
	return m_iEspionageWeight;
}

int CvLeaderHeadInfo::getRefuseToTalkWarThreshold() const
{
	return m_iRefuseToTalkWarThreshold;
}

int CvLeaderHeadInfo::getNoTechTradeThreshold() const
{
	return m_iNoTechTradeThreshold;
}

int CvLeaderHeadInfo::getTechTradeKnownPercent() const
{
	return m_iTechTradeKnownPercent;
}

int CvLeaderHeadInfo::getMaxGoldTradePercent() const
{
	return m_iMaxGoldTradePercent;
}

int CvLeaderHeadInfo::getMaxGoldPerTurnTradePercent() const
{
	return m_iMaxGoldPerTurnTradePercent;
}
// BETTER_BTS_AI_MOD, Victory Strategy AI, 03/21/10, jdog5000: START
int CvLeaderHeadInfo::getCultureVictoryWeight() const
{
	return m_iCultureVictoryWeight;
}

int CvLeaderHeadInfo::getSpaceVictoryWeight() const
{
	return m_iSpaceVictoryWeight;
}

int CvLeaderHeadInfo::getConquestVictoryWeight() const
{
	return m_iConquestVictoryWeight;
}

int CvLeaderHeadInfo::getDominationVictoryWeight() const
{
	return m_iDominationVictoryWeight;
}

int CvLeaderHeadInfo::getDiplomacyVictoryWeight() const
{
	return m_iDiplomacyVictoryWeight;
}
// BETTER_BTS_AI_MOD: END
int CvLeaderHeadInfo::getMaxWarRand() const
{
	return m_iMaxWarRand;
}

int CvLeaderHeadInfo::getMaxWarNearbyPowerRatio() const
{
	return m_iMaxWarNearbyPowerRatio;
}

int CvLeaderHeadInfo::getMaxWarDistantPowerRatio() const
{
	return m_iMaxWarDistantPowerRatio;
}

int CvLeaderHeadInfo::getMaxWarMinAdjacentLandPercent() const
{
	return m_iMaxWarMinAdjacentLandPercent;
}

int CvLeaderHeadInfo::getLimitedWarRand() const
{
	return m_iLimitedWarRand;
}

int CvLeaderHeadInfo::getLimitedWarPowerRatio() const
{
	return m_iLimitedWarPowerRatio;
}

int CvLeaderHeadInfo::getDogpileWarRand() const
{
	return m_iDogpileWarRand;
}

int CvLeaderHeadInfo::getMakePeaceRand() const
{
	return m_iMakePeaceRand;
}

int CvLeaderHeadInfo::getDeclareWarTradeRand() const
{
	return m_iDeclareWarTradeRand;
}

int CvLeaderHeadInfo::getDemandRebukedSneakProb() const
{
	return m_iDemandRebukedSneakProb;
}

int CvLeaderHeadInfo::getDemandRebukedWarProb() const
{
	return m_iDemandRebukedWarProb;
}

int CvLeaderHeadInfo::getRazeCityProb() const
{
	return m_iRazeCityProb;
}

int CvLeaderHeadInfo::getBaseAttackOddsChange() const
{
	return m_iBaseAttackOddsChange;
}

int CvLeaderHeadInfo::getAttackOddsChangeRand() const
{
	return m_iAttackOddsChangeRand;
}

int CvLeaderHeadInfo::getWorseRankDifferenceAttitudeChange() const
{
	return m_iWorseRankDifferenceAttitudeChange;
}

int CvLeaderHeadInfo::getBetterRankDifferenceAttitudeChange() const
{
	return m_iBetterRankDifferenceAttitudeChange;
}

int CvLeaderHeadInfo::getCloseBordersAttitudeChange() const
{
	return m_iCloseBordersAttitudeChange;
}

int CvLeaderHeadInfo::getLostWarAttitudeChange() const
{
	return m_iLostWarAttitudeChange;
}

int CvLeaderHeadInfo::getAtWarAttitudeDivisor() const
{
	return m_iAtWarAttitudeDivisor;
}

int CvLeaderHeadInfo::getAtWarAttitudeChangeLimit() const
{
	return m_iAtWarAttitudeChangeLimit;
}

int CvLeaderHeadInfo::getAtPeaceAttitudeDivisor() const
{
	return m_iAtPeaceAttitudeDivisor;
}

int CvLeaderHeadInfo::getAtPeaceAttitudeChangeLimit() const
{
	return m_iAtPeaceAttitudeChangeLimit;
}

int CvLeaderHeadInfo::getSameReligionAttitudeChange() const
{
	return m_iSameReligionAttitudeChange;
}

int CvLeaderHeadInfo::getSameReligionAttitudeDivisor() const
{
	return m_iSameReligionAttitudeDivisor;
}

int CvLeaderHeadInfo::getSameReligionAttitudeChangeLimit() const
{
	return m_iSameReligionAttitudeChangeLimit;
}

int CvLeaderHeadInfo::getDifferentReligionAttitudeChange() const
{
	return m_iDifferentReligionAttitudeChange;
}

int CvLeaderHeadInfo::getDifferentReligionAttitudeDivisor() const
{
	return m_iDifferentReligionAttitudeDivisor;
}

int CvLeaderHeadInfo::getDifferentReligionAttitudeChangeLimit() const
{
	return m_iDifferentReligionAttitudeChangeLimit;
}

int CvLeaderHeadInfo::getBonusTradeAttitudeDivisor() const
{
	return m_iBonusTradeAttitudeDivisor;
}

int CvLeaderHeadInfo::getBonusTradeAttitudeChangeLimit() const
{
	return m_iBonusTradeAttitudeChangeLimit;
}

int CvLeaderHeadInfo::getOpenBordersAttitudeDivisor() const
{
	return m_iOpenBordersAttitudeDivisor;
}

int CvLeaderHeadInfo::getOpenBordersAttitudeChangeLimit() const
{
	return m_iOpenBordersAttitudeChangeLimit;
}

int CvLeaderHeadInfo::getDefensivePactAttitudeDivisor() const
{
	return m_iDefensivePactAttitudeDivisor;
}

int CvLeaderHeadInfo::getDefensivePactAttitudeChangeLimit() const
{
	return m_iDefensivePactAttitudeChangeLimit;
}

int CvLeaderHeadInfo::getShareWarAttitudeChange() const
{
	return m_iShareWarAttitudeChange;
}

int CvLeaderHeadInfo::getShareWarAttitudeDivisor() const
{
	return m_iShareWarAttitudeDivisor;
}

int CvLeaderHeadInfo::getShareWarAttitudeChangeLimit() const
{
	return m_iShareWarAttitudeChangeLimit;
}

int CvLeaderHeadInfo::getFavoriteCivicAttitudeChange() const
{
	return m_iFavoriteCivicAttitudeChange;
}

int CvLeaderHeadInfo::getFavoriteCivicAttitudeDivisor() const
{
	return m_iFavoriteCivicAttitudeDivisor;
}

int CvLeaderHeadInfo::getFavoriteCivicAttitudeChangeLimit() const
{
	return m_iFavoriteCivicAttitudeChangeLimit;
}

int CvLeaderHeadInfo::getDemandTributeAttitudeThreshold() const
{
	return m_iDemandTributeAttitudeThreshold;
}

int CvLeaderHeadInfo::getNoGiveHelpAttitudeThreshold() const
{
	return m_iNoGiveHelpAttitudeThreshold;
}

int CvLeaderHeadInfo::getTechRefuseAttitudeThreshold() const
{
	return m_iTechRefuseAttitudeThreshold;
}
// <advc.ctr>
int CvLeaderHeadInfo::getCityRefuseAttitudeThreshold() const
{
	return m_iCityRefuseAttitudeThreshold;
}

int CvLeaderHeadInfo::getNativeCityRefuseAttitudeThreshold() const
{
	return m_iNativeCityRefuseAttitudeThreshold;
} // </advc.ctr>

int CvLeaderHeadInfo::getStrategicBonusRefuseAttitudeThreshold() const
{
	return m_iStrategicBonusRefuseAttitudeThreshold;
}

int CvLeaderHeadInfo::getHappinessBonusRefuseAttitudeThreshold() const
{
	return m_iHappinessBonusRefuseAttitudeThreshold;
}

int CvLeaderHeadInfo::getHealthBonusRefuseAttitudeThreshold() const
{
	return m_iHealthBonusRefuseAttitudeThreshold;
}

int CvLeaderHeadInfo::getMapRefuseAttitudeThreshold() const
{
	return m_iMapRefuseAttitudeThreshold;
}

int CvLeaderHeadInfo::getDeclareWarRefuseAttitudeThreshold() const
{
	return m_iDeclareWarRefuseAttitudeThreshold;
}

int CvLeaderHeadInfo::getDeclareWarThemRefuseAttitudeThreshold() const
{
	return m_iDeclareWarThemRefuseAttitudeThreshold;
}

int CvLeaderHeadInfo::getStopTradingRefuseAttitudeThreshold() const
{
	return m_iStopTradingRefuseAttitudeThreshold;
}

int CvLeaderHeadInfo::getStopTradingThemRefuseAttitudeThreshold() const
{
	return m_iStopTradingThemRefuseAttitudeThreshold;
}

int CvLeaderHeadInfo::getAdoptCivicRefuseAttitudeThreshold() const
{
	return m_iAdoptCivicRefuseAttitudeThreshold;
}

int CvLeaderHeadInfo::getConvertReligionRefuseAttitudeThreshold() const
{
	return m_iConvertReligionRefuseAttitudeThreshold;
}

int CvLeaderHeadInfo::getOpenBordersRefuseAttitudeThreshold() const
{
	return m_iOpenBordersRefuseAttitudeThreshold;
}

int CvLeaderHeadInfo::getDefensivePactRefuseAttitudeThreshold() const
{
	return m_iDefensivePactRefuseAttitudeThreshold;
}

int CvLeaderHeadInfo::getPermanentAllianceRefuseAttitudeThreshold() const
{
	return m_iPermanentAllianceRefuseAttitudeThreshold;
}

int CvLeaderHeadInfo::getVassalRefuseAttitudeThreshold() const
{
	return m_iVassalRefuseAttitudeThreshold;
}

int CvLeaderHeadInfo::getVassalPowerModifier() const
{
	return m_iVassalPowerModifier;
}

int CvLeaderHeadInfo::getFavoriteCivic() const
{
	return m_iFavoriteCivic;
}

int CvLeaderHeadInfo::getFavoriteReligion() const
{
	return m_iFavoriteReligion;
}

int CvLeaderHeadInfo::getFreedomAppreciation() const
{
	return m_iFreedomAppreciation;
}

const TCHAR* CvLeaderHeadInfo::getArtDefineTag() const
{
	return m_szArtDefineTag;
}

void CvLeaderHeadInfo::setArtDefineTag(const TCHAR* szVal)
{
	m_szArtDefineTag = szVal;
}

bool CvLeaderHeadInfo::hasTrait(int i) const
{
	FAssertBounds(0, GC.getNumTraitInfos(), i);
	return m_pbTraits ? m_pbTraits[i] : false;
}

int CvLeaderHeadInfo::getFlavorValue(int i) const
{
	FAssertBounds(0, GC.getNumFlavorTypes(), i);
	return m_piFlavorValue ? m_piFlavorValue[i] : 0; // advc.003t
}

int CvLeaderHeadInfo::getContactRand(int i) const
{
	FAssertBounds(0, NUM_CONTACT_TYPES, i);
	return m_piContactRand ? m_piContactRand[i] : 0; // advc.003t: Never contact by default
}

int CvLeaderHeadInfo::getContactDelay(int i) const
{
	FAssertBounds(0, NUM_CONTACT_TYPES, i);
	return m_piContactDelay ? m_piContactDelay[i] : 0; // advc.003t: No delay by default
}

int CvLeaderHeadInfo::getMemoryDecayRand(int i) const
{
	FAssertBounds(0, NUM_MEMORY_TYPES, i);
	// <advc.104i>
	if(m_piMemoryDecayRand == NULL)
		return -1;
	// The "clean" approach would be to set this 52 times in LeaderHead XML
	if(i == MEMORY_DECLARED_WAR_RECENT && m_piMemoryDecayRand[i] == 0)
		return 11;
	return m_piMemoryDecayRand[i]; // </advc.104i>
}

int CvLeaderHeadInfo::getMemoryAttitudePercent(int i) const
{
	FAssertBounds(0, NUM_MEMORY_TYPES, i);
	return m_piMemoryAttitudePercent ? m_piMemoryAttitudePercent[i] : 0; // advc.003t
}

int CvLeaderHeadInfo::getNoWarAttitudeProb(int i) const
{
	FAssertBounds(0, NUM_ATTITUDE_TYPES, i);
	return m_piNoWarAttitudeProb ? m_piNoWarAttitudeProb[i] : 0; // advc.003t
}

int CvLeaderHeadInfo::getUnitAIWeightModifier(int i) const
{
	FAssertBounds(0, NUM_UNITAI_TYPES, i);
	return m_piUnitAIWeightModifier ? m_piUnitAIWeightModifier[i] : 0; // advc.003t
}

int CvLeaderHeadInfo::getImprovementWeightModifier(int i) const
{
	FAssertBounds(0, GC.getNumImprovementInfos(), i);
	return m_piImprovementWeightModifier ? m_piImprovementWeightModifier[i] : 0; // advc.003t
}

int CvLeaderHeadInfo::getDiploPeaceIntroMusicScriptIds(int i) const
{
	FAssertBounds(0, GC.getNumEraInfos(), i);
	return m_piDiploPeaceIntroMusicScriptIds ? m_piDiploPeaceIntroMusicScriptIds[i]
			// advc.003t: CvLeaderHeadInfo::read sets the music script ids to 0 by default, so 0 works.
			: 0;
}

int CvLeaderHeadInfo::getDiploPeaceMusicScriptIds(int i) const
{
	FAssertBounds(0, GC.getNumEraInfos(), i);
	return m_piDiploPeaceMusicScriptIds ? m_piDiploPeaceMusicScriptIds[i] : 0; // advc.003t
}

int CvLeaderHeadInfo::getDiploWarIntroMusicScriptIds(int i) const
{
	FAssertBounds(0, GC.getNumEraInfos(), i);
	return m_piDiploWarIntroMusicScriptIds ? m_piDiploWarIntroMusicScriptIds[i] : 0; // advc.003t
}

int CvLeaderHeadInfo::getDiploWarMusicScriptIds(int i) const
{
	FAssertBounds(0, GC.getNumEraInfos(), i);
	return m_piDiploWarMusicScriptIds ? m_piDiploWarMusicScriptIds[i] : 0; // advc.003t
}

const TCHAR* CvLeaderHeadInfo::getLeaderHead() const
{
	const CvArtInfoLeaderhead * pLeaderheadArtInfo;
	pLeaderheadArtInfo = getArtInfo();
	if (pLeaderheadArtInfo != NULL)
		return pLeaderheadArtInfo->getNIF();

	return NULL;
}
#if SERIALIZE_CVINFOS
void CvLeaderHeadInfo::read(FDataStreamBase* stream)
{
	CvInfoBase::read(stream);
	uint uiFlag=0;
	stream->Read(&uiFlag);

	stream->Read(&m_iWonderConstructRand);
	stream->Read(&m_iBaseAttitude);
	stream->Read(&m_iBasePeaceWeight);
	stream->Read(&m_iPeaceWeightRand);
	stream->Read(&m_iWarmongerRespect);
	stream->Read(&m_iEspionageWeight);
	stream->Read(&m_iRefuseToTalkWarThreshold);
	stream->Read(&m_iNoTechTradeThreshold);
	stream->Read(&m_iTechTradeKnownPercent);
	stream->Read(&m_iMaxGoldTradePercent);
	stream->Read(&m_iMaxGoldPerTurnTradePercent);
	// BETTER_BTS_AI_MOD, Victory Strategy AI, 03/21/10, jdog5000: START
	if (uiFlag > 0)
	{
		stream->Read(&m_iCultureVictoryWeight);
		stream->Read(&m_iSpaceVictoryWeight);
		stream->Read(&m_iConquestVictoryWeight);
		stream->Read(&m_iDominationVictoryWeight);
		stream->Read(&m_iDiplomacyVictoryWeight);
	} // BETTER_BTS_AI_MOD: END
	stream->Read(&m_iMaxWarRand);
	stream->Read(&m_iMaxWarNearbyPowerRatio);
	stream->Read(&m_iMaxWarDistantPowerRatio);
	stream->Read(&m_iMaxWarMinAdjacentLandPercent);
	stream->Read(&m_iLimitedWarRand);
	stream->Read(&m_iLimitedWarPowerRatio);
	stream->Read(&m_iDogpileWarRand);
	stream->Read(&m_iMakePeaceRand);
	stream->Read(&m_iDeclareWarTradeRand);
	stream->Read(&m_iDemandRebukedSneakProb);
	stream->Read(&m_iDemandRebukedWarProb);
	stream->Read(&m_iRazeCityProb);
	stream->Read(&m_iBuildUnitProb);
	stream->Read(&m_iBaseAttackOddsChange);
	stream->Read(&m_iAttackOddsChangeRand);
	stream->Read(&m_iWorseRankDifferenceAttitudeChange);
	stream->Read(&m_iBetterRankDifferenceAttitudeChange);
	stream->Read(&m_iCloseBordersAttitudeChange);
	stream->Read(&m_iLostWarAttitudeChange);
	stream->Read(&m_iAtWarAttitudeDivisor);
	stream->Read(&m_iAtWarAttitudeChangeLimit);
	stream->Read(&m_iAtPeaceAttitudeDivisor);
	stream->Read(&m_iAtPeaceAttitudeChangeLimit);
	stream->Read(&m_iSameReligionAttitudeChange);
	stream->Read(&m_iSameReligionAttitudeDivisor);
	stream->Read(&m_iSameReligionAttitudeChangeLimit);
	stream->Read(&m_iDifferentReligionAttitudeChange);
	stream->Read(&m_iDifferentReligionAttitudeDivisor);
	stream->Read(&m_iDifferentReligionAttitudeChangeLimit);
	stream->Read(&m_iBonusTradeAttitudeDivisor);
	stream->Read(&m_iBonusTradeAttitudeChangeLimit);
	stream->Read(&m_iOpenBordersAttitudeDivisor);
	stream->Read(&m_iOpenBordersAttitudeChangeLimit);
	stream->Read(&m_iDefensivePactAttitudeDivisor);
	stream->Read(&m_iDefensivePactAttitudeChangeLimit);
	stream->Read(&m_iShareWarAttitudeChange);
	stream->Read(&m_iShareWarAttitudeDivisor);
	stream->Read(&m_iShareWarAttitudeChangeLimit);
	stream->Read(&m_iFavoriteCivicAttitudeChange);
	stream->Read(&m_iFavoriteCivicAttitudeDivisor);
	stream->Read(&m_iFavoriteCivicAttitudeChangeLimit);
	stream->Read(&m_iDemandTributeAttitudeThreshold);
	stream->Read(&m_iNoGiveHelpAttitudeThreshold);
	stream->Read(&m_iTechRefuseAttitudeThreshold);
	// <advc.ctr>
	stream->Read(&m_iCityRefuseAttitudeThreshold);
	stream->Read(&m_iNativeCityRefuseAttitudeThreshold); // </advc.ctr>
	stream->Read(&m_iStrategicBonusRefuseAttitudeThreshold);
	stream->Read(&m_iHappinessBonusRefuseAttitudeThreshold);
	stream->Read(&m_iHealthBonusRefuseAttitudeThreshold);
	stream->Read(&m_iMapRefuseAttitudeThreshold);
	stream->Read(&m_iDeclareWarRefuseAttitudeThreshold);
	stream->Read(&m_iDeclareWarThemRefuseAttitudeThreshold);
	stream->Read(&m_iStopTradingRefuseAttitudeThreshold);
	stream->Read(&m_iStopTradingThemRefuseAttitudeThreshold);
	stream->Read(&m_iAdoptCivicRefuseAttitudeThreshold);
	stream->Read(&m_iConvertReligionRefuseAttitudeThreshold);
	stream->Read(&m_iOpenBordersRefuseAttitudeThreshold);
	stream->Read(&m_iDefensivePactRefuseAttitudeThreshold);
	stream->Read(&m_iPermanentAllianceRefuseAttitudeThreshold);
	stream->Read(&m_iVassalRefuseAttitudeThreshold);
	stream->Read(&m_iVassalPowerModifier);
	stream->Read(&m_iFreedomAppreciation);
	stream->Read(&m_iFavoriteCivic);
	stream->Read(&m_iFavoriteReligion);
	stream->ReadString(m_szArtDefineTag);
	SAFE_DELETE_ARRAY(m_pbTraits);
	m_pbTraits = new bool[GC.getNumTraitInfos()];
	stream->Read(GC.getNumTraitInfos(), m_pbTraits);
	SAFE_DELETE_ARRAY(m_piFlavorValue);
	m_piFlavorValue = new int[GC.getNumFlavorTypes()];
	stream->Read(GC.getNumFlavorTypes(), m_piFlavorValue);
	SAFE_DELETE_ARRAY(m_piContactRand);
	m_piContactRand = new int[NUM_CONTACT_TYPES];
	stream->Read(NUM_CONTACT_TYPES, m_piContactRand);
	SAFE_DELETE_ARRAY(m_piContactDelay);
	m_piContactDelay = new int[NUM_CONTACT_TYPES];
	stream->Read(NUM_CONTACT_TYPES, m_piContactDelay);
	SAFE_DELETE_ARRAY(m_piMemoryDecayRand);
	m_piMemoryDecayRand = new int[NUM_MEMORY_TYPES];
	stream->Read(NUM_MEMORY_TYPES, m_piMemoryDecayRand);
	SAFE_DELETE_ARRAY(m_piMemoryAttitudePercent);
	m_piMemoryAttitudePercent = new int[NUM_MEMORY_TYPES];
	// <advc.104i>
	int iNumMemoryTypesToRead = NUM_MEMORY_TYPES;
	if(uiFlag < 2)
	{
		iNumMemoryTypesToRead--;
		m_piMemoryAttitudePercent[iNumMemoryTypesToRead] = 0;
	}
	stream->Read(iNumMemoryTypesToRead, m_piMemoryAttitudePercent); // </advc.104i>
	SAFE_DELETE_ARRAY(m_piNoWarAttitudeProb);
	m_piNoWarAttitudeProb = new int[NUM_ATTITUDE_TYPES];
	stream->Read(NUM_ATTITUDE_TYPES, m_piNoWarAttitudeProb);
	SAFE_DELETE_ARRAY(m_piUnitAIWeightModifier);
	m_piUnitAIWeightModifier = new int[NUM_UNITAI_TYPES];
	stream->Read(NUM_UNITAI_TYPES, m_piUnitAIWeightModifier);
	SAFE_DELETE_ARRAY(m_piImprovementWeightModifier);
	m_piImprovementWeightModifier = new int[GC.getNumImprovementInfos()];
	stream->Read(GC.getNumImprovementInfos(), m_piImprovementWeightModifier);
	SAFE_DELETE_ARRAY(m_piDiploPeaceIntroMusicScriptIds);
	m_piDiploPeaceIntroMusicScriptIds = new int[GC.getNumEraInfos()];
	stream->Read(GC.getNumEraInfos(), m_piDiploPeaceIntroMusicScriptIds);
	SAFE_DELETE_ARRAY(m_piDiploPeaceMusicScriptIds);
	m_piDiploPeaceMusicScriptIds = new int[GC.getNumEraInfos()];
	stream->Read(GC.getNumEraInfos(), m_piDiploPeaceMusicScriptIds);
	SAFE_DELETE_ARRAY(m_piDiploWarIntroMusicScriptIds);
	m_piDiploWarIntroMusicScriptIds = new int[GC.getNumEraInfos()];
	stream->Read(GC.getNumEraInfos(), m_piDiploWarIntroMusicScriptIds);
	SAFE_DELETE_ARRAY(m_piDiploWarMusicScriptIds);
	m_piDiploWarMusicScriptIds = new int[GC.getNumEraInfos()];
	stream->Read(GC.getNumEraInfos(), m_piDiploWarMusicScriptIds);
}

void CvLeaderHeadInfo::write(FDataStreamBase* stream)
{
	CvInfoBase::write(stream);
	uint uiFlag=1; // BETTER_BTS_AI_MOD, 03/21/10, jdog5000
	uiFlag = 2; // advc.104i
	stream->Write(uiFlag);

	stream->Write(m_iWonderConstructRand);
	stream->Write(m_iBaseAttitude);
	stream->Write(m_iBasePeaceWeight);
	stream->Write(m_iPeaceWeightRand);
	stream->Write(m_iWarmongerRespect);
	stream->Write(m_iEspionageWeight);
	stream->Write(m_iRefuseToTalkWarThreshold);
	stream->Write(m_iNoTechTradeThreshold);
	stream->Write(m_iTechTradeKnownPercent);
	stream->Write(m_iMaxGoldTradePercent);
	stream->Write(m_iMaxGoldPerTurnTradePercent);
	// BETTER_BTS_AI_MOD, Victory Strategy AI, 03/21/10, jdog5000: START
	stream->Write(m_iCultureVictoryWeight);
	stream->Write(m_iSpaceVictoryWeight);
	stream->Write(m_iConquestVictoryWeight);
	stream->Write(m_iDominationVictoryWeight);
	stream->Write(m_iDiplomacyVictoryWeight);
	// BETTER_BTS_AI_MOD: END
	stream->Write(m_iMaxWarRand);
	stream->Write(m_iMaxWarNearbyPowerRatio);
	stream->Write(m_iMaxWarDistantPowerRatio);
	stream->Write(m_iMaxWarMinAdjacentLandPercent);
	stream->Write(m_iLimitedWarRand);
	stream->Write(m_iLimitedWarPowerRatio);
	stream->Write(m_iDogpileWarRand);
	stream->Write(m_iMakePeaceRand);
	stream->Write(m_iDeclareWarTradeRand);
	stream->Write(m_iDemandRebukedSneakProb);
	stream->Write(m_iDemandRebukedWarProb);
	stream->Write(m_iRazeCityProb);
	stream->Write(m_iBuildUnitProb);
	stream->Write(m_iBaseAttackOddsChange);
	stream->Write(m_iAttackOddsChangeRand);
	stream->Write(m_iWorseRankDifferenceAttitudeChange);
	stream->Write(m_iBetterRankDifferenceAttitudeChange);
	stream->Write(m_iCloseBordersAttitudeChange);
	stream->Write(m_iLostWarAttitudeChange);
	stream->Write(m_iAtWarAttitudeDivisor);
	stream->Write(m_iAtWarAttitudeChangeLimit);
	stream->Write(m_iAtPeaceAttitudeDivisor);
	stream->Write(m_iAtPeaceAttitudeChangeLimit);
	stream->Write(m_iSameReligionAttitudeChange);
	stream->Write(m_iSameReligionAttitudeDivisor);
	stream->Write(m_iSameReligionAttitudeChangeLimit);
	stream->Write(m_iDifferentReligionAttitudeChange);
	stream->Write(m_iDifferentReligionAttitudeDivisor);
	stream->Write(m_iDifferentReligionAttitudeChangeLimit);
	stream->Write(m_iBonusTradeAttitudeDivisor);
	stream->Write(m_iBonusTradeAttitudeChangeLimit);
	stream->Write(m_iOpenBordersAttitudeDivisor);
	stream->Write(m_iOpenBordersAttitudeChangeLimit);
	stream->Write(m_iDefensivePactAttitudeDivisor);
	stream->Write(m_iDefensivePactAttitudeChangeLimit);
	stream->Write(m_iShareWarAttitudeChange);
	stream->Write(m_iShareWarAttitudeDivisor);
	stream->Write(m_iShareWarAttitudeChangeLimit);
	stream->Write(m_iFavoriteCivicAttitudeChange);
	stream->Write(m_iFavoriteCivicAttitudeDivisor);
	stream->Write(m_iFavoriteCivicAttitudeChangeLimit);
	stream->Write(m_iDemandTributeAttitudeThreshold);
	stream->Write(m_iNoGiveHelpAttitudeThreshold);
	stream->Write(m_iTechRefuseAttitudeThreshold);
	// <advc.ctr>
	stream->Write(m_iCityRefuseAttitudeThreshold);
	stream->Write(m_iNativeCityRefuseAttitudeThreshold); // </advc.ctr>
	stream->Write(m_iStrategicBonusRefuseAttitudeThreshold);
	stream->Write(m_iHappinessBonusRefuseAttitudeThreshold);
	stream->Write(m_iHealthBonusRefuseAttitudeThreshold);
	stream->Write(m_iMapRefuseAttitudeThreshold);
	stream->Write(m_iDeclareWarRefuseAttitudeThreshold);
	stream->Write(m_iDeclareWarThemRefuseAttitudeThreshold);
	stream->Write(m_iStopTradingRefuseAttitudeThreshold);
	stream->Write(m_iStopTradingThemRefuseAttitudeThreshold);
	stream->Write(m_iAdoptCivicRefuseAttitudeThreshold);
	stream->Write(m_iConvertReligionRefuseAttitudeThreshold);
	stream->Write(m_iOpenBordersRefuseAttitudeThreshold);
	stream->Write(m_iDefensivePactRefuseAttitudeThreshold);
	stream->Write(m_iPermanentAllianceRefuseAttitudeThreshold);
	stream->Write(m_iVassalRefuseAttitudeThreshold);
	stream->Write(m_iVassalPowerModifier);
	stream->Write(m_iFreedomAppreciation);
	stream->Write(m_iFavoriteCivic);
	stream->Write(m_iFavoriteReligion);
	stream->WriteString(m_szArtDefineTag);
	stream->Write(GC.getNumTraitInfos(), m_pbTraits);
	stream->Write(GC.getNumFlavorTypes(), m_piFlavorValue);
	stream->Write(NUM_CONTACT_TYPES, m_piContactRand);
	stream->Write(NUM_CONTACT_TYPES, m_piContactDelay);
	stream->Write(NUM_MEMORY_TYPES, m_piMemoryDecayRand);
	stream->Write(NUM_MEMORY_TYPES, m_piMemoryAttitudePercent);
	stream->Write(NUM_ATTITUDE_TYPES, m_piNoWarAttitudeProb);
	stream->Write(NUM_UNITAI_TYPES, m_piUnitAIWeightModifier);
	stream->Write(GC.getNumImprovementInfos(), m_piImprovementWeightModifier);
	stream->Write(GC.getNumEraInfos(), m_piDiploPeaceIntroMusicScriptIds);
	stream->Write(GC.getNumEraInfos(), m_piDiploPeaceMusicScriptIds);
	stream->Write(GC.getNumEraInfos(), m_piDiploWarIntroMusicScriptIds);
	stream->Write(GC.getNumEraInfos(), m_piDiploWarMusicScriptIds);
}
#endif
const CvArtInfoLeaderhead* CvLeaderHeadInfo::getArtInfo() const
{
	return ARTFILEMGR.getLeaderheadArtInfo( getArtDefineTag());
}

bool CvLeaderHeadInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvInfoBase::read(pXML))
		return false;

	CvString szTextVal;

	pXML->GetChildXmlValByName(szTextVal, "ArtDefineTag");
	setArtDefineTag(szTextVal);

	pXML->GetChildXmlValByName(&m_iWonderConstructRand, "iWonderConstructRand");
	pXML->GetChildXmlValByName(&m_iBaseAttitude, "iBaseAttitude");
	pXML->GetChildXmlValByName(&m_iBasePeaceWeight, "iBasePeaceWeight");
	pXML->GetChildXmlValByName(&m_iPeaceWeightRand, "iPeaceWeightRand");
	pXML->GetChildXmlValByName(&m_iWarmongerRespect, "iWarmongerRespect");
	pXML->GetChildXmlValByName(&m_iEspionageWeight, "iEspionageWeight");
	pXML->GetChildXmlValByName(&m_iRefuseToTalkWarThreshold, "iRefuseToTalkWarThreshold");
	pXML->GetChildXmlValByName(&m_iNoTechTradeThreshold, "iNoTechTradeThreshold");
	pXML->GetChildXmlValByName(&m_iTechTradeKnownPercent, "iTechTradeKnownPercent");
	pXML->GetChildXmlValByName(&m_iMaxGoldTradePercent, "iMaxGoldTradePercent");
	pXML->GetChildXmlValByName(&m_iMaxGoldPerTurnTradePercent, "iMaxGoldPerTurnTradePercent");
	// BETTER_BTS_AI_MOD, Victory Strategy AI, 03/21/10, jdog5000: START
	pXML->GetChildXmlValByName(&m_iCultureVictoryWeight, "iCultureVictoryWeight", 0);
	pXML->GetChildXmlValByName(&m_iSpaceVictoryWeight, "iSpaceVictoryWeight", 0);
	pXML->GetChildXmlValByName(&m_iConquestVictoryWeight, "iConquestVictoryWeight", 0);
	pXML->GetChildXmlValByName(&m_iDominationVictoryWeight, "iDominationVictoryWeight", 0);
	pXML->GetChildXmlValByName(&m_iDiplomacyVictoryWeight, "iDiplomacyVictoryWeight", 0);
	// BETTER_BTS_AI_MOD: END
	pXML->GetChildXmlValByName(&m_iMaxWarRand, "iMaxWarRand");
	pXML->GetChildXmlValByName(&m_iMaxWarNearbyPowerRatio, "iMaxWarNearbyPowerRatio");
	pXML->GetChildXmlValByName(&m_iMaxWarDistantPowerRatio, "iMaxWarDistantPowerRatio");
	pXML->GetChildXmlValByName(&m_iMaxWarMinAdjacentLandPercent, "iMaxWarMinAdjacentLandPercent");
	pXML->GetChildXmlValByName(&m_iLimitedWarRand, "iLimitedWarRand");
	pXML->GetChildXmlValByName(&m_iLimitedWarPowerRatio, "iLimitedWarPowerRatio");
	pXML->GetChildXmlValByName(&m_iDogpileWarRand, "iDogpileWarRand");
	pXML->GetChildXmlValByName(&m_iMakePeaceRand, "iMakePeaceRand");
	pXML->GetChildXmlValByName(&m_iDeclareWarTradeRand, "iDeclareWarTradeRand");
	pXML->GetChildXmlValByName(&m_iDemandRebukedSneakProb, "iDemandRebukedSneakProb");
	pXML->GetChildXmlValByName(&m_iDemandRebukedWarProb, "iDemandRebukedWarProb");
	pXML->GetChildXmlValByName(&m_iRazeCityProb, "iRazeCityProb");
	pXML->GetChildXmlValByName(&m_iBuildUnitProb, "iBuildUnitProb");
	pXML->GetChildXmlValByName(&m_iBaseAttackOddsChange, "iBaseAttackOddsChange");
	pXML->GetChildXmlValByName(&m_iAttackOddsChangeRand, "iAttackOddsChangeRand");
	pXML->GetChildXmlValByName(&m_iWorseRankDifferenceAttitudeChange, "iWorseRankDifferenceAttitudeChange");
	pXML->GetChildXmlValByName(&m_iBetterRankDifferenceAttitudeChange, "iBetterRankDifferenceAttitudeChange");
	pXML->GetChildXmlValByName(&m_iCloseBordersAttitudeChange, "iCloseBordersAttitudeChange");
	pXML->GetChildXmlValByName(&m_iLostWarAttitudeChange, "iLostWarAttitudeChange");
	pXML->GetChildXmlValByName(&m_iAtWarAttitudeDivisor, "iAtWarAttitudeDivisor");
	pXML->GetChildXmlValByName(&m_iAtWarAttitudeChangeLimit, "iAtWarAttitudeChangeLimit");
	pXML->GetChildXmlValByName(&m_iAtPeaceAttitudeDivisor, "iAtPeaceAttitudeDivisor");
	pXML->GetChildXmlValByName(&m_iAtPeaceAttitudeChangeLimit, "iAtPeaceAttitudeChangeLimit");
	pXML->GetChildXmlValByName(&m_iSameReligionAttitudeChange, "iSameReligionAttitudeChange");
	pXML->GetChildXmlValByName(&m_iSameReligionAttitudeDivisor, "iSameReligionAttitudeDivisor");
	pXML->GetChildXmlValByName(&m_iSameReligionAttitudeChangeLimit, "iSameReligionAttitudeChangeLimit");
	pXML->GetChildXmlValByName(&m_iDifferentReligionAttitudeChange, "iDifferentReligionAttitudeChange");
	pXML->GetChildXmlValByName(&m_iDifferentReligionAttitudeDivisor, "iDifferentReligionAttitudeDivisor");
	pXML->GetChildXmlValByName(&m_iDifferentReligionAttitudeChangeLimit, "iDifferentReligionAttitudeChangeLimit");
	pXML->GetChildXmlValByName(&m_iBonusTradeAttitudeDivisor, "iBonusTradeAttitudeDivisor");
	pXML->GetChildXmlValByName(&m_iBonusTradeAttitudeChangeLimit, "iBonusTradeAttitudeChangeLimit");
	pXML->GetChildXmlValByName(&m_iOpenBordersAttitudeDivisor, "iOpenBordersAttitudeDivisor");
	pXML->GetChildXmlValByName(&m_iOpenBordersAttitudeChangeLimit, "iOpenBordersAttitudeChangeLimit");
	pXML->GetChildXmlValByName(&m_iDefensivePactAttitudeDivisor, "iDefensivePactAttitudeDivisor");
	pXML->GetChildXmlValByName(&m_iDefensivePactAttitudeChangeLimit, "iDefensivePactAttitudeChangeLimit");
	pXML->GetChildXmlValByName(&m_iShareWarAttitudeChange, "iShareWarAttitudeChange");
	pXML->GetChildXmlValByName(&m_iShareWarAttitudeDivisor, "iShareWarAttitudeDivisor");
	pXML->GetChildXmlValByName(&m_iShareWarAttitudeChangeLimit, "iShareWarAttitudeChangeLimit");
	pXML->GetChildXmlValByName(&m_iFavoriteCivicAttitudeChange, "iFavoriteCivicAttitudeChange");
	pXML->GetChildXmlValByName(&m_iFavoriteCivicAttitudeDivisor, "iFavoriteCivicAttitudeDivisor");
	pXML->GetChildXmlValByName(&m_iFavoriteCivicAttitudeChangeLimit, "iFavoriteCivicAttitudeChangeLimit");
	pXML->GetChildXmlValByName(&m_iVassalPowerModifier, "iVassalPowerModifier");
	pXML->GetChildXmlValByName(&m_iFreedomAppreciation, "iFreedomAppreciation");

	pXML->GetChildXmlValByName(szTextVal, "DemandTributeAttitudeThreshold");
	m_iDemandTributeAttitudeThreshold = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "NoGiveHelpAttitudeThreshold");
	m_iNoGiveHelpAttitudeThreshold = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "TechRefuseAttitudeThreshold");
	m_iTechRefuseAttitudeThreshold = pXML->FindInInfoClass(szTextVal);
	// <advc.ctr>
	pXML->GetChildXmlValByName(szTextVal, "CityRefuseAttitudeThreshold", "");
	if (!szTextVal.empty())
		m_iCityRefuseAttitudeThreshold = pXML->FindInInfoClass(szTextVal);
	// (Keep initial value otherwise)
	pXML->GetChildXmlValByName(szTextVal, "NativeCityRefuseAttitudeThreshold", "");
	if (!szTextVal.empty())
		m_iNativeCityRefuseAttitudeThreshold = pXML->FindInInfoClass(szTextVal);
	// </advc.ctr>
	pXML->GetChildXmlValByName(szTextVal, "StrategicBonusRefuseAttitudeThreshold");
	m_iStrategicBonusRefuseAttitudeThreshold = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "HappinessBonusRefuseAttitudeThreshold");
	m_iHappinessBonusRefuseAttitudeThreshold = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "HealthBonusRefuseAttitudeThreshold");
	m_iHealthBonusRefuseAttitudeThreshold = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "MapRefuseAttitudeThreshold");
	m_iMapRefuseAttitudeThreshold = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "DeclareWarRefuseAttitudeThreshold");
	m_iDeclareWarRefuseAttitudeThreshold = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "DeclareWarThemRefuseAttitudeThreshold");
	m_iDeclareWarThemRefuseAttitudeThreshold = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "StopTradingRefuseAttitudeThreshold");
	m_iStopTradingRefuseAttitudeThreshold = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "StopTradingThemRefuseAttitudeThreshold");
	m_iStopTradingThemRefuseAttitudeThreshold = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "AdoptCivicRefuseAttitudeThreshold");
	m_iAdoptCivicRefuseAttitudeThreshold = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "ConvertReligionRefuseAttitudeThreshold");
	m_iConvertReligionRefuseAttitudeThreshold = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "OpenBordersRefuseAttitudeThreshold");
	m_iOpenBordersRefuseAttitudeThreshold = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "DefensivePactRefuseAttitudeThreshold");
	m_iDefensivePactRefuseAttitudeThreshold = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "PermanentAllianceRefuseAttitudeThreshold");
	m_iPermanentAllianceRefuseAttitudeThreshold = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "VassalRefuseAttitudeThreshold");
	m_iVassalRefuseAttitudeThreshold = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "FavoriteCivic");
	m_iFavoriteCivic = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "FavoriteReligion");
	m_iFavoriteReligion = pXML->FindInInfoClass(szTextVal);

	pXML->SetVariableListTagPair(&m_pbTraits, "Traits", GC.getNumTraitInfos());

	pXML->SetVariableListTagPair(&m_piFlavorValue, "Flavors", GC.getNumFlavorTypes());
	pXML->SetVariableListTagPair(&m_piContactRand, "ContactRands", NUM_CONTACT_TYPES);
	pXML->SetVariableListTagPair(&m_piContactDelay, "ContactDelays", NUM_CONTACT_TYPES);
	pXML->SetVariableListTagPair(&m_piMemoryDecayRand, "MemoryDecays", NUM_MEMORY_TYPES);
	pXML->SetVariableListTagPair(&m_piMemoryAttitudePercent, "MemoryAttitudePercents", NUM_MEMORY_TYPES);
	pXML->SetVariableListTagPair(&m_piNoWarAttitudeProb, "NoWarAttitudeProbs", NUM_ATTITUDE_TYPES);
	pXML->SetVariableListTagPair(&m_piUnitAIWeightModifier, "UnitAIWeightModifiers", NUM_UNITAI_TYPES);
	pXML->SetVariableListTagPair(&m_piImprovementWeightModifier, "ImprovementWeightModifiers", GC.getNumImprovementInfos());
	pXML->SetVariableListTagPairForAudioScripts(&m_piDiploPeaceIntroMusicScriptIds, "DiplomacyIntroMusicPeace", GC.getNumEraInfos());
	pXML->SetVariableListTagPairForAudioScripts(&m_piDiploPeaceMusicScriptIds, "DiplomacyMusicPeace", GC.getNumEraInfos());
	pXML->SetVariableListTagPairForAudioScripts(&m_piDiploWarIntroMusicScriptIds, "DiplomacyIntroMusicWar", GC.getNumEraInfos());
	pXML->SetVariableListTagPairForAudioScripts(&m_piDiploWarMusicScriptIds, "DiplomacyMusicWar", GC.getNumEraInfos());

	return true;
}

CvTraitInfo::CvTraitInfo() :
m_iHealth(0),
m_iHappiness(0),
m_iMaxAnarchy(0),
m_iUpkeepModifier(0),
m_iLevelExperienceModifier(0),
m_iGreatPeopleRateModifier(0),
m_iGreatGeneralRateModifier(0),
m_iDomesticGreatGeneralRateModifier(0),
m_iMaxGlobalBuildingProductionModifier(0),
m_iMaxTeamBuildingProductionModifier(0),
m_iMaxPlayerBuildingProductionModifier(0),
m_paiExtraYieldThreshold(NULL),
m_paiTradeYieldModifier(NULL),
m_paiCommerceChange(NULL),
m_paiCommerceModifier(NULL),
m_pabFreePromotionUnitCombat(NULL),
m_pabFreePromotion(NULL)
{}

CvTraitInfo::~CvTraitInfo()
{
	SAFE_DELETE_ARRAY(m_paiExtraYieldThreshold);
	SAFE_DELETE_ARRAY(m_paiTradeYieldModifier);
	SAFE_DELETE_ARRAY(m_paiCommerceChange);
	SAFE_DELETE_ARRAY(m_paiCommerceModifier);
	SAFE_DELETE_ARRAY(m_pabFreePromotionUnitCombat);
	SAFE_DELETE_ARRAY(m_pabFreePromotion);
}

int CvTraitInfo::getHealth() const
{
	return m_iHealth;
}

int CvTraitInfo::getHappiness() const
{
	return m_iHappiness;
}

int CvTraitInfo::getMaxAnarchy() const
{
	return m_iMaxAnarchy;
}

int CvTraitInfo::getUpkeepModifier() const
{
	return m_iUpkeepModifier;
}

int CvTraitInfo::getLevelExperienceModifier() const
{
	return m_iLevelExperienceModifier;
}

int CvTraitInfo::getGreatPeopleRateModifier() const
{
	return m_iGreatPeopleRateModifier;
}

int CvTraitInfo::getGreatGeneralRateModifier() const
{
	return m_iGreatGeneralRateModifier;
}

int CvTraitInfo::getDomesticGreatGeneralRateModifier() const
{
	return m_iDomesticGreatGeneralRateModifier;
}

int CvTraitInfo::getMaxGlobalBuildingProductionModifier() const
{
	return m_iMaxGlobalBuildingProductionModifier;
}

int CvTraitInfo::getMaxTeamBuildingProductionModifier() const
{
	return m_iMaxTeamBuildingProductionModifier;
}

int CvTraitInfo::getMaxPlayerBuildingProductionModifier() const
{
	return m_iMaxPlayerBuildingProductionModifier;
}

const TCHAR* CvTraitInfo::getShortDescription() const
{
	return m_szShortDescription;
}

void CvTraitInfo::setShortDescription(const TCHAR* szVal)
{
	m_szShortDescription = szVal;
}

// Arrays  // advc.003t: FAssertBounds calls added

int CvTraitInfo::getExtraYieldThreshold(int i) const
{
	FAssertBounds(0, NUM_YIELD_TYPES, i);
	return m_paiExtraYieldThreshold ? m_paiExtraYieldThreshold[i] : 0; // advc.003t
}

int CvTraitInfo::getTradeYieldModifier(int i) const
{
	FAssertBounds(0, NUM_YIELD_TYPES, i);
	return m_paiTradeYieldModifier ? m_paiTradeYieldModifier[i] : 0; // advc.003t
}

int CvTraitInfo::getCommerceChange(int i) const
{
	FAssertBounds(0, NUM_COMMERCE_TYPES, i);
	return m_paiCommerceChange ? m_paiCommerceChange[i] : 0; // advc.003t
}

int CvTraitInfo::getCommerceModifier(int i) const
{
	FAssertBounds(0, NUM_COMMERCE_TYPES, i);
	return m_paiCommerceModifier ? m_paiCommerceModifier[i] : 0; // advc.003t
}

bool CvTraitInfo::isFreePromotion(int i) const // advc.003t: Return type was int
{
	FAssertBounds(0, GC.getNumPromotionInfos(), i);
	return m_pabFreePromotion ? m_pabFreePromotion[i] : false;
}

bool CvTraitInfo::isFreePromotionUnitCombat(int i) const // advc.003t: Return type was int
{
	FAssertBounds(0, GC.getNumUnitCombatInfos(), i);
	return m_pabFreePromotionUnitCombat ? m_pabFreePromotionUnitCombat[i] : false;
}

bool CvTraitInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvInfoBase::read(pXML))
		return false;

	CvString szTextVal;

	pXML->GetChildXmlValByName(szTextVal, "ShortDescription");
	setShortDescription(szTextVal);

	pXML->GetChildXmlValByName(&m_iHealth, "iHealth");
	pXML->GetChildXmlValByName(&m_iHappiness, "iHappiness");
	pXML->GetChildXmlValByName(&m_iMaxAnarchy, "iMaxAnarchy");
	pXML->GetChildXmlValByName(&m_iUpkeepModifier, "iUpkeepModifier");
	pXML->GetChildXmlValByName(&m_iLevelExperienceModifier, "iLevelExperienceModifier");
	pXML->GetChildXmlValByName(&m_iGreatPeopleRateModifier, "iGreatPeopleRateModifier");
	pXML->GetChildXmlValByName(&m_iGreatGeneralRateModifier, "iGreatGeneralRateModifier");
	pXML->GetChildXmlValByName(&m_iDomesticGreatGeneralRateModifier, "iDomesticGreatGeneralRateModifier");
	pXML->GetChildXmlValByName(&m_iMaxGlobalBuildingProductionModifier, "iMaxGlobalBuildingProductionModifier");
	pXML->GetChildXmlValByName(&m_iMaxTeamBuildingProductionModifier, "iMaxTeamBuildingProductionModifier");
	pXML->GetChildXmlValByName(&m_iMaxPlayerBuildingProductionModifier, "iMaxPlayerBuildingProductionModifier");

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "ExtraYieldThresholds"))
	{
		pXML->SetYields(&m_paiExtraYieldThreshold);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_paiExtraYieldThreshold, NUM_YIELD_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "TradeYieldModifiers"))
	{
		pXML->SetYields(&m_paiTradeYieldModifier);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_paiTradeYieldModifier, NUM_YIELD_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "CommerceChanges"))
	{
		pXML->SetCommerce(&m_paiCommerceChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_paiCommerceChange, NUM_COMMERCE_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "CommerceModifiers"))
	{
		pXML->SetCommerce(&m_paiCommerceModifier);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_paiCommerceModifier, NUM_COMMERCE_TYPES);

	pXML->SetVariableListTagPair(&m_pabFreePromotion, "FreePromotions", GC.getNumPromotionInfos());

	pXML->SetVariableListTagPair(&m_pabFreePromotionUnitCombat, "FreePromotionUnitCombats", GC.getNumUnitCombatInfos());

	return true;
}

CvDiplomacyResponse::CvDiplomacyResponse() :
m_iNumDiplomacyText(0),
m_pbCivilizationTypes(NULL),
m_pbLeaderHeadTypes(NULL),
m_pbAttitudeTypes(NULL),
m_pbDiplomacyPowerTypes(NULL),
m_paszDiplomacyText(NULL)
{}

CvDiplomacyResponse::~CvDiplomacyResponse()
{
	SAFE_DELETE_ARRAY(m_pbCivilizationTypes);
	SAFE_DELETE_ARRAY(m_pbLeaderHeadTypes);
	SAFE_DELETE_ARRAY(m_pbAttitudeTypes);
	SAFE_DELETE_ARRAY(m_pbDiplomacyPowerTypes);
	SAFE_DELETE_ARRAY(m_paszDiplomacyText);
}

int CvDiplomacyResponse::getNumDiplomacyText() const
{
	return m_iNumDiplomacyText;
}

/*void CvDiplomacyResponse::setNumDiplomacyText(int i)
{
	m_iNumDiplomacyText = i;
}*/

bool CvDiplomacyResponse::getCivilizationTypes(int i) const
{
	FAssertBounds(0, GC.getNumCivilizationInfos(), i);
	return (m_pbCivilizationTypes != NULL ? m_pbCivilizationTypes[i] : false); // advc.003t
}

/*bool* CvDiplomacyResponse::getCivilizationTypes() const
{
	return m_pbCivilizationTypes;
}*/

/*void CvDiplomacyResponse::setCivilizationTypes(int i, bool bVal)
{
	FAssertMsg(i < GC.getNumCivilizationInfos(), "Index out of bounds");
	FAssertMsg(i > -1, "Index out of bounds");
	m_pbCivilizationTypes[i] = bVal;
}*/

bool CvDiplomacyResponse::getLeaderHeadTypes(int i) const
{
	FAssertBounds(0, GC.getNumLeaderHeadInfos(), i);
	return (m_pbLeaderHeadTypes != NULL ? m_pbLeaderHeadTypes[i] : false); // advc.003t
}

/*bool* CvDiplomacyResponse::getLeaderHeadTypes() const
{
	return m_pbLeaderHeadTypes;
}*/

/*void CvDiplomacyResponse::setLeaderHeadTypes(int i, bool bVal)
{
	FAssertMsg(i < GC.getNumLeaderHeadInfos(), "Index out of bounds");
	FAssertMsg(i > -1, "Index out of bounds");
	m_pbLeaderHeadTypes[i] = bVal;
}*/

bool CvDiplomacyResponse::getAttitudeTypes(int i) const
{
	FAssertBounds(0, NUM_ATTITUDE_TYPES, i);
	return (m_pbAttitudeTypes != NULL ? m_pbAttitudeTypes[i] : false); // advc.003t
}

/*bool* CvDiplomacyResponse::getAttitudeTypes() const
{
	return m_pbAttitudeTypes;
}*/

/*void CvDiplomacyResponse::setAttitudeTypes(int i, bool bVal)
{
	FAssertMsg(i < NUM_ATTITUDE_TYPES, "Index out of bounds");
	FAssertMsg(i > -1, "Index out of bounds");
	m_pbAttitudeTypes[i] = bVal;
}*/

bool CvDiplomacyResponse::getDiplomacyPowerTypes(int i) const
{
	FAssertBounds(0, NUM_DIPLOMACYPOWER_TYPES, i);
	return (m_pbDiplomacyPowerTypes != NULL ? m_pbDiplomacyPowerTypes[i] : false); // advc.003t
}

/*bool* CvDiplomacyResponse::getDiplomacyPowerTypes() const
{
	return m_pbDiplomacyPowerTypes;
}*/

/*void CvDiplomacyResponse::setDiplomacyPowerTypes(int i, bool bVal)
{
	FAssertMsg(i < NUM_DIPLOMACYPOWER_TYPES, "Index out of bounds");
	FAssertMsg(i > -1, "Index out of bounds");
	m_pbDiplomacyPowerTypes[i] = bVal;
}*/

const TCHAR* CvDiplomacyResponse::getDiplomacyText(int i) const
{
	return m_paszDiplomacyText[i];
}

const CvString* CvDiplomacyResponse::getDiplomacyText() const
{
	return m_paszDiplomacyText;
}

void CvDiplomacyResponse::setDiplomacyText(int i, CvString szText)
{
	FAssertBounds(0, getNumDiplomacyText(), i);
	m_paszDiplomacyText[i] = szText;
}
/*#if SERIALIZE_CVINFOS
void CvDiplomacyResponse::read(FDataStreamBase* stream)
{
	uint uiFlag=0;
	stream->Read(&uiFlag);

	stream->Read(&m_iNumDiplomacyText);
	SAFE_DELETE_ARRAY(m_pbCivilizationTypes);
	m_pbCivilizationTypes = new bool[GC.getNumCivilizationInfos()];
	stream->Read(GC.getNumCivilizationInfos(), m_pbCivilizationTypes);
	SAFE_DELETE_ARRAY(m_pbLeaderHeadTypes);
	m_pbLeaderHeadTypes = new bool[GC.getNumLeaderHeadInfos()];
	stream->Read(GC.getNumLeaderHeadInfos(), m_pbLeaderHeadTypes);
	SAFE_DELETE_ARRAY(m_pbAttitudeTypes);
	m_pbAttitudeTypes = new bool[NUM_ATTITUDE_TYPES];
	stream->Read(NUM_ATTITUDE_TYPES, m_pbAttitudeTypes);
	SAFE_DELETE_ARRAY(m_pbDiplomacyPowerTypes);
	m_pbDiplomacyPowerTypes = new bool[NUM_DIPLOMACYPOWER_TYPES];
	stream->Read(NUM_DIPLOMACYPOWER_TYPES, m_pbDiplomacyPowerTypes);
	SAFE_DELETE_ARRAY(m_paszDiplomacyText);
	m_paszDiplomacyText = new CvString[m_iNumDiplomacyText];
	stream->ReadString(m_iNumDiplomacyText, m_paszDiplomacyText);
}

void CvDiplomacyResponse::write(FDataStreamBase* stream)
{
	uint uiFlag=0;
	stream->Write(uiFlag);

	stream->Write(m_iNumDiplomacyText);
	stream->Write(GC.getNumCivilizationInfos(), m_pbCivilizationTypes);
	stream->Write(GC.getNumLeaderHeadInfos(), m_pbLeaderHeadTypes);
	stream->Write(NUM_ATTITUDE_TYPES, m_pbAttitudeTypes);
	stream->Write(NUM_DIPLOMACYPOWER_TYPES, m_pbDiplomacyPowerTypes);
	stream->WriteString(m_iNumDiplomacyText, m_paszDiplomacyText);
}*/
//#endif
bool CvDiplomacyResponse::read(CvXMLLoadUtility* pXML)
{
	pXML->SetVariableListTagPair(&m_pbCivilizationTypes, "Civilizations", GC.getNumCivilizationInfos());
	pXML->SetVariableListTagPair(&m_pbLeaderHeadTypes, "Leaders", GC.getNumLeaderHeadInfos());
	pXML->SetVariableListTagPair(&m_pbAttitudeTypes, "Attitudes", NUM_ATTITUDE_TYPES);
	pXML->SetVariableListTagPair(&m_pbDiplomacyPowerTypes, "DiplomacyPowers", NUM_DIPLOMACYPOWER_TYPES);
	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "DiplomacyText"))
	{
		pXML->SetStringList(&m_paszDiplomacyText, &m_iNumDiplomacyText);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	return true;
}

CvDiplomacyInfo::~CvDiplomacyInfo()
{
	// note - Response member vars allocated by CvXmlLoadUtility
	for (std::vector<CvDiplomacyResponse*>::iterator it = m_pResponses.begin(); it != m_pResponses.end(); ++it)
		SAFE_DELETE(*it);
	m_pResponses.clear();
}

CvDiplomacyResponse const& CvDiplomacyInfo::getResponse(int iNum) const
{
	return *m_pResponses[iNum];
}
// <advc.705>
CvDiplomacyResponse& CvDiplomacyInfo::getResponse_(int iNum)
{
	return *m_pResponses[iNum];
} // </advc.705>

int CvDiplomacyInfo::getNumResponses() const
{
	return m_pResponses.size();
}

bool CvDiplomacyInfo::getCivilizationTypes(int i, int j) const
{
	FAssertBounds(0, getNumResponses(), i);
	FAssertBounds(0, GC.getNumCivilizationInfos(), j);
	return m_pResponses[i]->getCivilizationTypes(j);
}

bool CvDiplomacyInfo::getLeaderHeadTypes(int i, int j) const
{
	FAssertBounds(0, getNumResponses(), i);
	FAssertBounds(0, GC.getNumLeaderHeadInfos(), j);
	return m_pResponses[i]->getLeaderHeadTypes(j);
}

bool CvDiplomacyInfo::getAttitudeTypes(int i, int j) const
{
	FAssertBounds(0, getNumResponses(), i);
	FAssertBounds(0, NUM_ATTITUDE_TYPES, j);
	return m_pResponses[i]->getAttitudeTypes(j);
}

bool CvDiplomacyInfo::getDiplomacyPowerTypes(int i, int j) const
{
	FAssertBounds(0, getNumResponses(), i);
	FAssertBounds(0, NUM_DIPLOMACYPOWER_TYPES, j);
	return m_pResponses[i]->getDiplomacyPowerTypes(j);
}

int CvDiplomacyInfo::getNumDiplomacyText(int i) const
{
	FAssertBounds(0, getNumResponses(), i);
	return m_pResponses[i]->getNumDiplomacyText();
}

const TCHAR* CvDiplomacyInfo::getDiplomacyText(int i, int j) const
{
	FAssertBounds(0, getNumResponses(), i);
	FAssertBounds(0, getNumDiplomacyText(i), j);
	return m_pResponses[i]->getDiplomacyText(j);
}
/*#if SERIALIZE_CVINFOS
void CvDiplomacyInfo::read(FDataStreamBase* stream)
{
	CvInfoBase::read(stream);
	uint uiFlag=0;
	stream->Read(&uiFlag);

	int iNumResponses;
	stream->Read(&iNumResponses);
	for (int uiIndex = 0; uiIndex < iNumResponses; ++uiIndex)
	{
		CvDiplomacyResponse* pResponse = new CvDiplomacyResponse;
		pResponse->read(stream);
		m_pResponses.push_back(pResponse);
	}
}

void CvDiplomacyInfo::write(FDataStreamBase* stream)
{
	CvInfoBase::write(stream);
	uint uiFlag=0;
	stream->Write(uiFlag);

	int iNumResponses = m_pResponses.size();
	stream->Write(iNumResponses);
	for (int uiIndex = 0; uiIndex < iNumResponses; ++uiIndex)
		m_pResponses[uiIndex]->write(stream);
}*/
//#endif
bool CvDiplomacyInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvInfoBase::read(pXML))
		return false;

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"Responses"))
	{
		int iNewResponses = gDLL->getXMLIFace()->NumOfChildrenByTagName(pXML->GetXML(), "Response");
		// advc.006b: GetChildXmlValByName can't handle comments
		FAssertMsg(iNewResponses > 0, "XML comment inside Responses?");
		gDLL->getXMLIFace()->SetToChild(pXML->GetXML());

		for (int i = 0; i < iNewResponses; i++)
		{
			CvDiplomacyResponse* pNewResponse = new CvDiplomacyResponse;
			pNewResponse->read(pXML);
			m_pResponses.push_back(pNewResponse);

			if (!gDLL->getXMLIFace()->NextSibling(pXML->GetXML()))
				break;
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	gDLL->getXMLIFace()->SetToParent(pXML->GetXML());

	return true;
}
