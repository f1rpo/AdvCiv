// advc.003x: Cut from CvInfos.cpp

#include "CvGameCoreDLL.h"
#include "CvInfo_Building.h"
#include "CvXMLLoadUtility.h"
#include "CvDLLXMLIFaceBase.h"

CvBuildingInfo::CvBuildingInfo() :
m_iBuildingClassType(NO_BUILDINGCLASS),
m_iVictoryPrereq(NO_VICTORY),
m_iFreeStartEra(NO_ERA),
m_iMaxStartEra(NO_ERA),
m_iObsoleteTech(NO_TECH),
m_iPrereqAndTech(NO_TECH),
m_iNoBonus(NO_BONUS),
m_iPowerBonus(NO_BONUS),
m_iFreeBonus(NO_BONUS),
m_iNumFreeBonuses(0),
m_iFreeBuildingClass(NO_BUILDINGCLASS),
m_iFreePromotion(NO_PROMOTION),
m_iCivicOption(NO_CIVICOPTION),
m_iAIWeight(0),
m_iProductionCost(0),
m_iHurryCostModifier(0),
m_iHurryAngerModifier(0),
m_iAdvancedStartCost(0),
m_iAdvancedStartCostIncrease(0),
m_iMinAreaSize(0),
m_iNumCitiesPrereq(0),
m_iNumTeamsPrereq(0),
m_iUnitLevelPrereq(0),
m_iMinLatitude(0),
m_iMaxLatitude(90),
m_iGreatPeopleRateModifier(0),
m_iGreatGeneralRateModifier(0),
m_iDomesticGreatGeneralRateModifier(0),
m_iGlobalGreatPeopleRateModifier(0),
m_iAnarchyModifier(0),
m_iGoldenAgeModifier(0),
m_iGlobalHurryModifier(0),
m_iFreeExperience(0),
m_iGlobalFreeExperience(0),
m_iFoodKept(0),
m_iAirlift(0),
m_iAirModifier(0),
m_iAirUnitCapacity(0),
m_iNukeModifier(0),
m_iNukeExplosionRand(0),
m_iFreeSpecialist(0),
m_iAreaFreeSpecialist(0),
m_iGlobalFreeSpecialist(0),
m_iHappiness(0),
m_iAreaHappiness(0),
m_iGlobalHappiness(0),
m_iStateReligionHappiness(0),
m_iWorkerSpeedModifier(0),
m_iMilitaryProductionModifier(0),
m_iSpaceProductionModifier(0),
m_iGlobalSpaceProductionModifier(0),
m_iTradeRoutes(0),
m_iCoastalTradeRoutes(0),
m_iAreaTradeRoutes(0), // advc.310
m_iTradeRouteModifier(0),
m_iForeignTradeRouteModifier(0),
m_iAssetValue(0),
m_iPowerValue(0),
m_iSpecialBuildingType(NO_SPECIALBUILDING),
m_iAdvisorType(NO_ADVISOR),
m_iHolyCity(NO_RELIGION),
m_iReligionType(NO_RELIGION),
m_iStateReligion(NO_RELIGION),
m_iPrereqReligion(NO_RELIGION),
m_iPrereqCorporation(NO_CORPORATION),
m_iFoundsCorporation(NO_CORPORATION),
m_iGlobalReligionCommerce(0),
m_iGlobalCorporationCommerce(0),
m_iPrereqAndBonus(NO_BONUS),
m_iGreatPeopleUnitClass(NO_UNITCLASS),
m_iGreatPeopleRateChange(0),
m_iConquestProbability(0),
m_iMaintenanceModifier(0),
m_iWarWearinessModifier(0),
m_iGlobalWarWearinessModifier(0),
m_iEnemyWarWearinessModifier(0),
m_iHealRateChange(0),
m_iHealth(0),
m_iAreaHealth(0),
m_iGlobalHealth(0),
m_iGlobalPopulationChange(0),
m_iFreeTechs(0),
m_iDefenseModifier(0),
m_iBombardDefenseModifier(0),
m_iAllCityDefenseModifier(0),
m_iEspionageDefenseModifier(0),
m_iMissionType(NO_MISSION),
m_iVoteSourceType(NO_VOTESOURCE),
m_fVisibilityPriority(0.0f),
m_bTeamShare(false),
m_bWater(false),
m_bRiver(false),
m_bPower(false),
m_bDirtyPower(false),
m_bAreaCleanPower(false),
m_bAreaBorderObstacle(false),
m_bForceTeamVoteEligible(false),
m_bCapital(false),
m_bGovernmentCenter(false),
m_bGoldenAge(false),
m_bMapCentering(false),
m_bNoUnhappiness(false),
//m_bNoUnhealthyPopulation(false),
m_iUnhealthyPopulationModifier(0), // K-Mod
m_bBuildingOnlyHealthy(false),
m_bNeverCapture(false),
m_bNukeImmune(false),
m_bPrereqReligion(false),
m_bCenterInCity(false),
m_bStateReligion(false),
m_bAllowsNukes(false),
m_piPrereqAndTechs(NULL),
m_piPrereqOrBonuses(NULL),
m_piProductionTraits(NULL),
m_piHappinessTraits(NULL),
m_piSeaPlotYieldChange(NULL),
m_piRiverPlotYieldChange(NULL),
m_piGlobalSeaPlotYieldChange(NULL),
m_piYieldChange(NULL),
m_piYieldModifier(NULL),
m_piPowerYieldModifier(NULL),
m_piAreaYieldModifier(NULL),
m_piGlobalYieldModifier(NULL),
m_piCommerceChange(NULL),
m_piObsoleteSafeCommerceChange(NULL),
m_piCommerceChangeDoubleTime(NULL),
m_piCommerceModifier(NULL),
m_piGlobalCommerceModifier(NULL),
m_piSpecialistExtraCommerce(NULL),
m_piStateReligionCommerce(NULL),
m_piCommerceHappiness(NULL),
m_piReligionChange(NULL),
m_piSpecialistCount(NULL),
m_piFreeSpecialistCount(NULL),
m_piBonusHealthChanges(NULL),
m_piBonusHappinessChanges(NULL),
m_piBonusProductionModifier(NULL),
m_piUnitCombatFreeExperience(NULL),
m_piDomainFreeExperience(NULL),
m_piDomainProductionModifier(NULL),
m_piBuildingHappinessChanges(NULL),
m_piPrereqNumOfBuildingClass(NULL),
m_piFlavorValue(NULL),
m_piImprovementFreeSpecialist(NULL),
m_pbCommerceFlexible(NULL),
m_pbCommerceChangeOriginalOwner(NULL),
m_pbBuildingClassNeededInCity(NULL),
m_ppaiSpecialistYieldChange(NULL),
m_ppaiBonusYieldModifier(NULL),
// UNOFFICIAL_PATCH, Efficiency, 06/27/10, Afforess & jdog5000: START
m_bAnySpecialistYieldChange(false),
m_bAnyBonusYieldModifier(false)
// UNOFFICIAL_PATCH: END
{}

CvBuildingInfo::~CvBuildingInfo()
{
	SAFE_DELETE_ARRAY(m_piPrereqAndTechs);
	SAFE_DELETE_ARRAY(m_piPrereqOrBonuses);
	SAFE_DELETE_ARRAY(m_piProductionTraits);
	SAFE_DELETE_ARRAY(m_piHappinessTraits);
	SAFE_DELETE_ARRAY(m_piSeaPlotYieldChange);
	SAFE_DELETE_ARRAY(m_piRiverPlotYieldChange);
	SAFE_DELETE_ARRAY(m_piGlobalSeaPlotYieldChange);
	SAFE_DELETE_ARRAY(m_piYieldChange);
	SAFE_DELETE_ARRAY(m_piYieldModifier);
	SAFE_DELETE_ARRAY(m_piPowerYieldModifier);
	SAFE_DELETE_ARRAY(m_piAreaYieldModifier);
	SAFE_DELETE_ARRAY(m_piGlobalYieldModifier);
	SAFE_DELETE_ARRAY(m_piCommerceChange);
	SAFE_DELETE_ARRAY(m_piObsoleteSafeCommerceChange);
	SAFE_DELETE_ARRAY(m_piCommerceChangeDoubleTime);
	SAFE_DELETE_ARRAY(m_piCommerceModifier);
	SAFE_DELETE_ARRAY(m_piGlobalCommerceModifier);
	SAFE_DELETE_ARRAY(m_piSpecialistExtraCommerce);
	SAFE_DELETE_ARRAY(m_piStateReligionCommerce);
	SAFE_DELETE_ARRAY(m_piCommerceHappiness);
	SAFE_DELETE_ARRAY(m_piReligionChange);
	SAFE_DELETE_ARRAY(m_piSpecialistCount);
	SAFE_DELETE_ARRAY(m_piFreeSpecialistCount);
	SAFE_DELETE_ARRAY(m_piBonusHealthChanges);
	SAFE_DELETE_ARRAY(m_piBonusHappinessChanges);
	SAFE_DELETE_ARRAY(m_piBonusProductionModifier);
	SAFE_DELETE_ARRAY(m_piUnitCombatFreeExperience);
	SAFE_DELETE_ARRAY(m_piDomainFreeExperience);
	SAFE_DELETE_ARRAY(m_piDomainProductionModifier);
	SAFE_DELETE_ARRAY(m_piBuildingHappinessChanges);
	SAFE_DELETE_ARRAY(m_piPrereqNumOfBuildingClass);
	SAFE_DELETE_ARRAY(m_piFlavorValue);
	SAFE_DELETE_ARRAY(m_piImprovementFreeSpecialist);
	SAFE_DELETE_ARRAY(m_pbCommerceFlexible);
	SAFE_DELETE_ARRAY(m_pbCommerceChangeOriginalOwner);
	SAFE_DELETE_ARRAY(m_pbBuildingClassNeededInCity);

	if (m_ppaiSpecialistYieldChange != NULL)
	{
		for(int i = 0; i < GC.getNumSpecialistInfos(); i++)
			SAFE_DELETE_ARRAY(m_ppaiSpecialistYieldChange[i]);
		SAFE_DELETE_ARRAY(m_ppaiSpecialistYieldChange);
	}
	if (m_ppaiBonusYieldModifier != NULL)
	{
		for(int i = 0;i < GC.getNumBonusInfos(); i++)
			SAFE_DELETE_ARRAY(m_ppaiBonusYieldModifier[i]);
		SAFE_DELETE_ARRAY(m_ppaiBonusYieldModifier);
	}
}

int CvBuildingInfo::getVictoryPrereq() const
{
	return m_iVictoryPrereq;
}

int CvBuildingInfo::getFreeStartEra() const
{
	return m_iFreeStartEra;
}

int CvBuildingInfo::getMaxStartEra() const
{
	return m_iMaxStartEra;
}

int CvBuildingInfo::getObsoleteTech() const
{
	return m_iObsoleteTech;
}

int CvBuildingInfo::getPrereqAndTech() const
{
	return m_iPrereqAndTech;
}

int CvBuildingInfo::getNoBonus() const
{
	return m_iNoBonus;
}

int CvBuildingInfo::getPowerBonus() const
{
	return m_iPowerBonus;
}

int CvBuildingInfo::getFreeBonus() const
{
	return m_iFreeBonus;
}

int CvBuildingInfo::getNumFreeBonuses() const
{
	return m_iNumFreeBonuses;
}

int CvBuildingInfo::getFreeBuildingClass() const
{
	return m_iFreeBuildingClass;
}

int CvBuildingInfo::getFreePromotion() const
{
	return m_iFreePromotion;
}

int CvBuildingInfo::getCivicOption() const
{
	return m_iCivicOption;
}

int CvBuildingInfo::getAIWeight() const
{
	return m_iAIWeight;
}

int CvBuildingInfo::getProductionCost() const
{
	return m_iProductionCost;
}

int CvBuildingInfo::getHurryCostModifier() const
{
	return m_iHurryCostModifier;
}

int CvBuildingInfo::getHurryAngerModifier() const
{
	return m_iHurryAngerModifier;
}

int CvBuildingInfo::getAdvancedStartCost() const
{
	return m_iAdvancedStartCost;
}

int CvBuildingInfo::getAdvancedStartCostIncrease() const
{
	return m_iAdvancedStartCostIncrease;
}

int CvBuildingInfo::getMinAreaSize() const
{
	return m_iMinAreaSize;
}

int CvBuildingInfo::getNumCitiesPrereq() const
{
	return m_iNumCitiesPrereq;
}

int CvBuildingInfo::getNumTeamsPrereq() const
{
	return m_iNumTeamsPrereq;
}

int CvBuildingInfo::getUnitLevelPrereq() const
{
	return m_iUnitLevelPrereq;
}

int CvBuildingInfo::getMinLatitude() const
{
	return m_iMinLatitude;
}

int CvBuildingInfo::getMaxLatitude() const
{
	return m_iMaxLatitude;
}

int CvBuildingInfo::getGreatPeopleRateModifier() const
{
	return m_iGreatPeopleRateModifier;
}

int CvBuildingInfo::getGreatGeneralRateModifier() const
{
	return m_iGreatGeneralRateModifier;
}

int CvBuildingInfo::getDomesticGreatGeneralRateModifier() const
{	// <advc.310>
	if(!m_bEnabledDomesticGreatGeneralRateModifier)
		return 0; // </advc.310>
	return m_iDomesticGreatGeneralRateModifier;
}

int CvBuildingInfo::getGlobalGreatPeopleRateModifier() const
{
	return m_iGlobalGreatPeopleRateModifier;
}

int CvBuildingInfo::getAnarchyModifier() const
{
	return m_iAnarchyModifier;
}

int CvBuildingInfo::getGoldenAgeModifier() const
{
	return m_iGoldenAgeModifier;
}

int CvBuildingInfo::getGlobalHurryModifier() const
{
	return m_iGlobalHurryModifier;
}

int CvBuildingInfo::getFreeExperience() const
{
	return m_iFreeExperience;
}

int CvBuildingInfo::getGlobalFreeExperience() const
{
	return m_iGlobalFreeExperience;
}

int CvBuildingInfo::getFoodKept() const
{
	return m_iFoodKept;
}

int CvBuildingInfo::getAirlift() const
{
	return m_iAirlift;
}

int CvBuildingInfo::getAirModifier() const
{
	return m_iAirModifier;
}

int CvBuildingInfo::getAirUnitCapacity() const
{
	return m_iAirUnitCapacity;
}

int CvBuildingInfo::getNukeModifier() const
{
	return m_iNukeModifier;
}

int CvBuildingInfo::getNukeExplosionRand() const
{
	return m_iNukeExplosionRand;
}

int CvBuildingInfo::getFreeSpecialist() const
{
	return m_iFreeSpecialist;
}

int CvBuildingInfo::getAreaFreeSpecialist() const
{
	return m_iAreaFreeSpecialist;
}

int CvBuildingInfo::getGlobalFreeSpecialist() const
{
	return m_iGlobalFreeSpecialist;
}

int CvBuildingInfo::getHappiness() const
{
	return m_iHappiness;
}

int CvBuildingInfo::getAreaHappiness() const
{
	return m_iAreaHappiness;
}

int CvBuildingInfo::getGlobalHappiness() const
{
	return m_iGlobalHappiness;
}

int CvBuildingInfo::getStateReligionHappiness() const
{
	return m_iStateReligionHappiness;
}

int CvBuildingInfo::getWorkerSpeedModifier() const
{
	return m_iWorkerSpeedModifier;
}

int CvBuildingInfo::getMilitaryProductionModifier() const
{
	return m_iMilitaryProductionModifier;
}

int CvBuildingInfo::getSpaceProductionModifier() const
{
	return m_iSpaceProductionModifier;
}

int CvBuildingInfo::getGlobalSpaceProductionModifier() const
{
	return m_iGlobalSpaceProductionModifier;
}

int CvBuildingInfo::getTradeRoutes() const
{
	return m_iTradeRoutes;
}

int CvBuildingInfo::getCoastalTradeRoutes() const
{
	return m_iCoastalTradeRoutes;
}
// <advc.310>
int CvBuildingInfo::getAreaTradeRoutes() const
{
	if(!m_bEnabledAreaTradeRoutes)
		return 0;
	return m_iAreaTradeRoutes;
} // </advc.310>

int CvBuildingInfo::getTradeRouteModifier() const
{
	return m_iTradeRouteModifier;
}

int CvBuildingInfo::getForeignTradeRouteModifier() const
{
	return m_iForeignTradeRouteModifier;
}

int CvBuildingInfo::getAssetValue() const
{
	return m_iAssetValue;
}

int CvBuildingInfo::getPowerValue() const
{
	return m_iPowerValue;
}

int CvBuildingInfo::getSpecialBuildingType() const
{
	return m_iSpecialBuildingType;
}

int CvBuildingInfo::getAdvisorType() const
{
	return m_iAdvisorType;
}

int CvBuildingInfo::getHolyCity() const
{
	return m_iHolyCity;
}

int CvBuildingInfo::getReligionType() const
{
	return m_iReligionType;
}

int CvBuildingInfo::getStateReligion() const
{
	return m_iStateReligion;
}

int CvBuildingInfo::getPrereqReligion() const
{
	return m_iPrereqReligion;
}

int CvBuildingInfo::getPrereqCorporation() const
{
	return m_iPrereqCorporation;
}

int CvBuildingInfo::getFoundsCorporation() const
{
	return m_iFoundsCorporation;
}

int CvBuildingInfo::getGlobalReligionCommerce() const
{
	return m_iGlobalReligionCommerce;
}

int CvBuildingInfo::getGlobalCorporationCommerce() const
{
	return m_iGlobalCorporationCommerce;
}

int CvBuildingInfo::getPrereqAndBonus() const
{
	return m_iPrereqAndBonus;
}

int CvBuildingInfo::getGreatPeopleUnitClass() const
{
	return m_iGreatPeopleUnitClass;
}

int CvBuildingInfo::getGreatPeopleRateChange() const
{
	return m_iGreatPeopleRateChange;
}

int CvBuildingInfo::getConquestProbability() const
{
	return m_iConquestProbability;
}

int CvBuildingInfo::getMaintenanceModifier() const
{
	return m_iMaintenanceModifier;
}

int CvBuildingInfo::getWarWearinessModifier() const
{
	return m_iWarWearinessModifier;
}

int CvBuildingInfo::getGlobalWarWearinessModifier() const
{
	return m_iGlobalWarWearinessModifier;
}

int CvBuildingInfo::getEnemyWarWearinessModifier() const
{
	return m_iEnemyWarWearinessModifier;
}

int CvBuildingInfo::getHealRateChange() const
{
	return m_iHealRateChange;
}

int CvBuildingInfo::getHealth() const
{
	return m_iHealth;
}

int CvBuildingInfo::getAreaHealth() const
{
	return m_iAreaHealth;
}

int CvBuildingInfo::getGlobalHealth() const
{
	return m_iGlobalHealth;
}

int CvBuildingInfo::getGlobalPopulationChange() const
{
	return m_iGlobalPopulationChange;
}

int CvBuildingInfo::getFreeTechs() const
{
	return m_iFreeTechs;
}

int CvBuildingInfo::getDefenseModifier() const
{
	return m_iDefenseModifier;
}

int CvBuildingInfo::getBombardDefenseModifier() const
{
	return m_iBombardDefenseModifier;
}

int CvBuildingInfo::getAllCityDefenseModifier() const
{
	return m_iAllCityDefenseModifier;
}

int CvBuildingInfo::getEspionageDefenseModifier() const
{
	return m_iEspionageDefenseModifier;
}

int CvBuildingInfo::getMissionType() const
{
	return m_iMissionType;
}

void CvBuildingInfo::setMissionType(int iNewType)
{
	m_iMissionType = iNewType;
}

int CvBuildingInfo::getVoteSourceType() const
{
	return m_iVoteSourceType;
}

float CvBuildingInfo::getVisibilityPriority() const
{
	return m_fVisibilityPriority;
}

bool CvBuildingInfo::isTeamShare() const
{
	return m_bTeamShare;
}

bool CvBuildingInfo::isWater() const
{
	return m_bWater;
}

bool CvBuildingInfo::isRiver() const
{
	return m_bRiver;
}

bool CvBuildingInfo::isPower() const
{
	return m_bPower;
}

bool CvBuildingInfo::isDirtyPower() const
{
	return m_bDirtyPower;
}

bool CvBuildingInfo::isAreaCleanPower() const
{
	return m_bAreaCleanPower;
}

bool CvBuildingInfo::isAreaBorderObstacle() const
{	// <advc.310>
	if(!m_bEnabledAreaBorderObstacle)
		return false; // </advc.310>
	return m_bAreaBorderObstacle;
}

bool CvBuildingInfo::isForceTeamVoteEligible() const
{
	return m_bForceTeamVoteEligible;
}

bool CvBuildingInfo::isCapital() const
{
	return m_bCapital;
}

bool CvBuildingInfo::isGovernmentCenter() const
{
	return m_bGovernmentCenter;
}

bool CvBuildingInfo::isGoldenAge() const
{
	return m_bGoldenAge;
}

bool CvBuildingInfo::isMapCentering() const
{
	return m_bMapCentering;
}

bool CvBuildingInfo::isNoUnhappiness() const
{
	return m_bNoUnhappiness;
}

/* original bts code
bool CvBuildingInfo::isNoUnhealthyPopulation() const
{
	return m_bNoUnhealthyPopulation;
}*/
/*  K-Mod, 27/dec/10, karadoc
	replace NoUnhealthyPopulation with UnhealthyPopulationModifier */
int CvBuildingInfo::getUnhealthyPopulationModifier() const
{
	return m_iUnhealthyPopulationModifier;
} // K-Mod end

bool CvBuildingInfo::isBuildingOnlyHealthy() const
{
	return m_bBuildingOnlyHealthy;
}

bool CvBuildingInfo::isNeverCapture() const
{
	return m_bNeverCapture;
}

bool CvBuildingInfo::isNukeImmune() const
{
	return m_bNukeImmune;
}

bool CvBuildingInfo::isPrereqReligion() const
{
	return m_bPrereqReligion;
}

bool CvBuildingInfo::isCenterInCity() const
{
	return m_bCenterInCity;
}

bool CvBuildingInfo::isStateReligion() const
{
	return m_bStateReligion;
}

bool CvBuildingInfo::isAllowsNukes() const
{
	return m_bAllowsNukes;
}

const TCHAR* CvBuildingInfo::getConstructSound() const
{
	return m_szConstructSound;
}

void CvBuildingInfo::setConstructSound(const TCHAR* szVal)
{
	m_szConstructSound = szVal;
}

const TCHAR* CvBuildingInfo::getArtDefineTag() const
{
	return m_szArtDefineTag;
}

void CvBuildingInfo::setArtDefineTag(const TCHAR* szVal)
{
	m_szArtDefineTag = szVal;
}

const TCHAR* CvBuildingInfo::getMovieDefineTag() const
{
	return m_szMovieDefineTag;
}

void CvBuildingInfo::setMovieDefineTag(const TCHAR* szVal)
{
	m_szMovieDefineTag = szVal;
}


int CvBuildingInfo::getYieldChange(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvBuildingInfo::getYieldChange");
	return m_piYieldChange ? m_piYieldChange[i] : 0; // advc.003t
}

int* CvBuildingInfo::getYieldChangeArray() const
{
	return m_piYieldChange;
}

int CvBuildingInfo::getYieldModifier(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvBuildingInfo::getYieldModifier");
	return m_piYieldModifier ? m_piYieldModifier[i] : 0; // advc003t
}

int* CvBuildingInfo::getYieldModifierArray() const
{
	return m_piYieldModifier;
}

int CvBuildingInfo::getPowerYieldModifier(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvBuildingInfo::getPowerYieldModifier");
	return m_piPowerYieldModifier ? m_piPowerYieldModifier[i] : 0; // advc.003t
}

int* CvBuildingInfo::getPowerYieldModifierArray() const
{
	return m_piPowerYieldModifier;
}

int CvBuildingInfo::getAreaYieldModifier(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvBuildingInfo::getAreaYieldModifier");
	return m_piAreaYieldModifier ? m_piAreaYieldModifier[i] : 0; // advc.003t
}

int* CvBuildingInfo::getAreaYieldModifierArray() const
{
	return m_piAreaYieldModifier;
}

int CvBuildingInfo::getGlobalYieldModifier(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvBuildingInfo::getGlobalYieldModifier");
	return m_piGlobalYieldModifier ? m_piGlobalYieldModifier[i] : 0; // advc.003t
}

int* CvBuildingInfo::getGlobalYieldModifierArray() const
{
	return m_piGlobalYieldModifier;
}

int CvBuildingInfo::getSeaPlotYieldChange(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvBuildingInfo::getSeaPlotYieldChange");
	return m_piSeaPlotYieldChange ? m_piSeaPlotYieldChange[i] : 0; // advc.003t
}

int* CvBuildingInfo::getSeaPlotYieldChangeArray() const
{
	return m_piSeaPlotYieldChange;
}

int CvBuildingInfo::getRiverPlotYieldChange(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvBuildingInfo::getRiverPlotYieldChange");
	return m_piRiverPlotYieldChange ? m_piRiverPlotYieldChange[i] : 0; // advc.003t
}

int* CvBuildingInfo::getRiverPlotYieldChangeArray() const
{
	return m_piRiverPlotYieldChange;
}

int CvBuildingInfo::getGlobalSeaPlotYieldChange(int i) const
{
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, i, "CvBuildingInfo::getGlobalSeaPlotYieldChange");
	return m_piGlobalSeaPlotYieldChange ? m_piGlobalSeaPlotYieldChange[i] : 0; // advc.003t
}

int* CvBuildingInfo::getGlobalSeaPlotYieldChangeArray() const
{
	return m_piGlobalSeaPlotYieldChange;
}

int CvBuildingInfo::getCommerceChange(int i) const
{
	FASSERT_BOUNDS(0, NUM_COMMERCE_TYPES, i, "CvBuildingInfo::getCommerceChange");
	return m_piCommerceChange ? m_piCommerceChange[i] : 0; // advc.003t
}

int* CvBuildingInfo::getCommerceChangeArray() const
{
	return m_piCommerceChange;
}

int CvBuildingInfo::getObsoleteSafeCommerceChange(int i) const
{
	FASSERT_BOUNDS(0, NUM_COMMERCE_TYPES, i, "CvBuildingInfo::getObsoleteSafeCommerceChange");
	return m_piObsoleteSafeCommerceChange ? m_piObsoleteSafeCommerceChange[i] : 0; // advc.003t
}

int* CvBuildingInfo::getObsoleteSafeCommerceChangeArray() const
{
	return m_piObsoleteSafeCommerceChange;
}

int CvBuildingInfo::getCommerceChangeDoubleTime(int i) const
{
	FASSERT_BOUNDS(0, NUM_COMMERCE_TYPES, i, "CvBuildingInfo::getCommerceChangeDoubleTime");
	return m_piCommerceChangeDoubleTime ? m_piCommerceChangeDoubleTime[i]
			: 0; // advc.003t: Was -1. 0 means infinity here.
}

int CvBuildingInfo::getCommerceModifier(int i) const
{
	FASSERT_BOUNDS(0, NUM_COMMERCE_TYPES, i, "CvBuildingInfo::getCommerceModifier");
	return m_piCommerceModifier ? m_piCommerceModifier[i] : 0; // advc.003t
}

int* CvBuildingInfo::getCommerceModifierArray() const
{
	return m_piCommerceModifier;
}

int CvBuildingInfo::getGlobalCommerceModifier(int i) const
{
	FASSERT_BOUNDS(0, NUM_COMMERCE_TYPES, i, "CvBuildingInfo::getGlobalCommerceModifier");
	return m_piGlobalCommerceModifier ? m_piGlobalCommerceModifier[i] : 0; // advc.003t
}

int* CvBuildingInfo::getGlobalCommerceModifierArray() const
{
	return m_piGlobalCommerceModifier;
}

int CvBuildingInfo::getSpecialistExtraCommerce(int i) const
{
	FASSERT_BOUNDS(0, NUM_COMMERCE_TYPES, i, "CvBuildingInfo::getSpecialistExtraCommerce");
	return m_piSpecialistExtraCommerce ? m_piSpecialistExtraCommerce[i] : 0; // advc.003t
}

int* CvBuildingInfo::getSpecialistExtraCommerceArray() const
{
	return m_piSpecialistExtraCommerce;
}

int CvBuildingInfo::getStateReligionCommerce(int i) const
{
	FASSERT_BOUNDS(0, NUM_COMMERCE_TYPES, i, "CvBuildingInfo::getStateReligionCommerce");
	return m_piStateReligionCommerce ? m_piStateReligionCommerce[i] : 0; // advc.003t
}

int* CvBuildingInfo::getStateReligionCommerceArray() const
{
	return m_piStateReligionCommerce;
}

int CvBuildingInfo::getCommerceHappiness(int i) const
{
	FASSERT_BOUNDS(0, NUM_COMMERCE_TYPES, i, "CvBuildingInfo::getCommerceHappiness");
	return m_piCommerceHappiness ? m_piCommerceHappiness[i] : 0; // advc.003t
}

int CvBuildingInfo::getReligionChange(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumReligionInfos(), i, "CvBuildingInfo::getReligionChange");
	return m_piReligionChange ? m_piReligionChange[i]
			: 0; // advc.003t: Was -1. This one acts as a boolean actually.
}

int CvBuildingInfo::getSpecialistCount(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumSpecialistInfos(), i, "CvBuildingInfo::getSpecialistCount");
	FAssertMsg(i < GC.getNumSpecialistInfos(), "Index out of bounds");
	FAssertMsg(i > -1, "Index out of bounds");
	return m_piSpecialistCount ? m_piSpecialistCount[i] : 0; // advc.003t
}

int CvBuildingInfo::getFreeSpecialistCount(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumSpecialistInfos(), i, "CvBuildingInfo::getFreeSpecialistCount");
	return m_piFreeSpecialistCount ? m_piFreeSpecialistCount[i] : 0; // advc.003t
}

int CvBuildingInfo::getBonusHealthChanges(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumBonusInfos(), i, "CvBuildingInfo::getBonusHealthChanges");
	return m_piBonusHealthChanges ? m_piBonusHealthChanges[i] : 0; // advc.003t
}

int CvBuildingInfo::getBonusHappinessChanges(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumBonusInfos(), i, "CvBuildingInfo::getBonusHappinessChanges");
	return m_piBonusHappinessChanges ? m_piBonusHappinessChanges[i] : 0; // advc.003t
}

int CvBuildingInfo::getBonusProductionModifier(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumBonusInfos(), i, "CvBuildingInfo::getBonusProductionModifier");
	return m_piBonusProductionModifier ? m_piBonusProductionModifier[i] : 0; // advc.003t
}

int CvBuildingInfo::getUnitCombatFreeExperience(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumUnitCombatInfos(), i, "CvBuildingInfo::getUnitCombatFreeExperience");
	return m_piUnitCombatFreeExperience ? m_piUnitCombatFreeExperience[i] : 0; // advc.003t
}

int CvBuildingInfo::getDomainFreeExperience(int i) const
{
	FASSERT_BOUNDS(0, NUM_DOMAIN_TYPES, i, "CvBuildingInfo::getDomainFreeExperience");
	return m_piDomainFreeExperience ? m_piDomainFreeExperience[i] : 0; // advc.003t
}

int CvBuildingInfo::getDomainProductionModifier(int i) const
{
	FASSERT_BOUNDS(0, NUM_DOMAIN_TYPES, i, "CvBuildingInfo::getDomainProductionModifier");
	return m_piDomainProductionModifier ? m_piDomainProductionModifier[i] : 0; // advc.003t
}

int CvBuildingInfo::getPrereqAndTechs(int i) const
{
	FASSERT_BOUNDS(0, GC.getNUM_BUILDING_AND_TECH_PREREQS(), i, "CvBuildingInfo::getPrereqAndTechs");
	return m_piPrereqAndTechs ? m_piPrereqAndTechs[i] : NO_TECH; // advc.003t
}

int CvBuildingInfo::getPrereqOrBonuses(int i) const
{
	FASSERT_BOUNDS(0, GC.getNUM_BUILDING_PREREQ_OR_BONUSES(), i, "CvBuildingInfo::getPrereqOrBonuses");
	return m_piPrereqOrBonuses ? m_piPrereqOrBonuses[i] : NO_TECH; // advc.003t
}

int CvBuildingInfo::getProductionTraits(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumTraitInfos(), i, "CvBuildingInfo::getProductionTraits");
	return m_piProductionTraits ? m_piProductionTraits[i]
			: 0; // advc.003t: Was -1. This is the production discount percentage.
}

int CvBuildingInfo::getHappinessTraits(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumTraitInfos(), i, "CvBuildingInfo::getHappinessTraits");
	return m_piHappinessTraits ? m_piHappinessTraits[i]
			: 0; // advc.003t: Was -1. This is the happiness from trait.
}

int CvBuildingInfo::getBuildingHappinessChanges(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumBuildingClassInfos(), i, "CvBuildingInfo::getBuildingHappinessChanges");
	return m_piBuildingHappinessChanges ? m_piBuildingHappinessChanges[i] : 0; // advc.003t
}

int CvBuildingInfo::getPrereqNumOfBuildingClass(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumBuildingClassInfos(), i, "CvBuildingInfo::getPrereqNumOfBuildingClass");
	return m_piPrereqNumOfBuildingClass ? m_piPrereqNumOfBuildingClass[i] : 0; // advc.003t
}

int CvBuildingInfo::getFlavorValue(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumFlavorTypes(), i, "CvBuildingInfo::getFlavorValue");
	return m_piFlavorValue ? m_piFlavorValue[i] : 0; // advc.003t
}

int CvBuildingInfo::getImprovementFreeSpecialist(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumImprovementInfos(), i, "CvBuildingInfo::getImprovementFreeSpecialist");
	return m_piImprovementFreeSpecialist ? m_piImprovementFreeSpecialist[i] : 0; // advc.003t
}

bool CvBuildingInfo::isCommerceFlexible(int i) const
{
	FASSERT_BOUNDS(0, NUM_COMMERCE_TYPES, i, "CvBuildingInfo::isCommerceFlexible");
	return m_pbCommerceFlexible ? m_pbCommerceFlexible[i] : false;
}

bool CvBuildingInfo::isCommerceChangeOriginalOwner(int i) const
{
	FASSERT_BOUNDS(0, NUM_COMMERCE_TYPES, i, "CvBuildingInfo::isCommerceChangeOriginalOwner");
	return m_pbCommerceChangeOriginalOwner ? m_pbCommerceChangeOriginalOwner[i] : false;
}

bool CvBuildingInfo::isBuildingClassNeededInCity(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumBuildingClassInfos(), i, "CvBuildingInfo::isBuildingClassNeededInCity");
	return m_pbBuildingClassNeededInCity ? m_pbBuildingClassNeededInCity[i] : false;
}

int CvBuildingInfo::getSpecialistYieldChange(int i, int j) const
{
	FASSERT_BOUNDS(0, GC.getNumSpecialistInfos(), i, "CvBuildingInfo::getSpecialistYieldChange");
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, j, "CvBuildingInfo::getSpecialistYieldChange");
	return m_ppaiSpecialistYieldChange ? m_ppaiSpecialistYieldChange[i][j] : 0; // advc.003t
}

int* CvBuildingInfo::getSpecialistYieldChangeArray(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumSpecialistInfos(), i, "CvBuildingInfo::getSpecialistYieldChangeArray");
	return m_ppaiSpecialistYieldChange[i];
}

int CvBuildingInfo::getBonusYieldModifier(int i, int j) const
{
	FASSERT_BOUNDS(0, GC.getNumBonusInfos(), i, "CvBuildingInfo::getBonusYieldModifier");
	FASSERT_BOUNDS(0, NUM_YIELD_TYPES, j, "CvBuildingInfo::getBonusYieldModifier");
	return m_ppaiBonusYieldModifier ? m_ppaiBonusYieldModifier[i][j] : 0; // advc.003t
}

int* CvBuildingInfo::getBonusYieldModifierArray(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumBonusInfos(), i, "CvBuildingInfo::getBonusYieldModifierArray");
	return m_ppaiBonusYieldModifier[i];
}

const TCHAR* CvBuildingInfo::getButton() const
{
	const CvArtInfoBuilding * pBuildingArtInfo;
	pBuildingArtInfo = getArtInfo();
	if (pBuildingArtInfo != NULL)
		return pBuildingArtInfo->getButton();

	return NULL;
}

const CvArtInfoBuilding* CvBuildingInfo::getArtInfo() const
{
	return ARTFILEMGR.getBuildingArtInfo(getArtDefineTag());
}

const CvArtInfoMovie* CvBuildingInfo::getMovieInfo() const
{
	const TCHAR* pcTag = getMovieDefineTag();
	if (pcTag != NULL && _tcscmp(pcTag, "NONE") != 0)
		return ARTFILEMGR.getMovieArtInfo(pcTag);
	
	return NULL;
}

const TCHAR* CvBuildingInfo::getMovie() const
{
	const CvArtInfoMovie* pArt;
	pArt = getMovieInfo();
	if (pArt != NULL)
		return pArt->getPath();

	return NULL;
}
#if SERIALIZE_CVINFOS
void CvBuildingInfo::read(FDataStreamBase* stream)
{
	CvHotkeyInfo::read(stream);
	uint uiFlag=0;
	stream->Read(&uiFlag);

	stream->Read(&m_iBuildingClassType);
	stream->Read(&m_iVictoryPrereq);
	stream->Read(&m_iFreeStartEra);
	stream->Read(&m_iMaxStartEra);
	stream->Read(&m_iObsoleteTech);
	stream->Read(&m_iPrereqAndTech);
	stream->Read(&m_iNoBonus);
	stream->Read(&m_iPowerBonus);
	stream->Read(&m_iFreeBonus);
	stream->Read(&m_iNumFreeBonuses);
	stream->Read(&m_iFreeBuildingClass);
	stream->Read(&m_iFreePromotion);
	stream->Read(&m_iCivicOption);
	stream->Read(&m_iAIWeight);
	stream->Read(&m_iProductionCost);
	stream->Read(&m_iHurryCostModifier);
	stream->Read(&m_iHurryAngerModifier);
	stream->Read(&m_iAdvancedStartCost);
	stream->Read(&m_iAdvancedStartCostIncrease);
	stream->Read(&m_iMinAreaSize);
	stream->Read(&m_iNumCitiesPrereq);
	stream->Read(&m_iNumTeamsPrereq);
	stream->Read(&m_iUnitLevelPrereq);
	stream->Read(&m_iMinLatitude);
	stream->Read(&m_iMaxLatitude);
	stream->Read(&m_iGreatPeopleRateModifier);
	stream->Read(&m_iGreatGeneralRateModifier);
	stream->Read(&m_iDomesticGreatGeneralRateModifier);
	stream->Read(&m_iGlobalGreatPeopleRateModifier);
	stream->Read(&m_iAnarchyModifier);
	stream->Read(&m_iGoldenAgeModifier);
	stream->Read(&m_iGlobalHurryModifier);
	stream->Read(&m_iFreeExperience);
	stream->Read(&m_iGlobalFreeExperience);
	stream->Read(&m_iFoodKept);
	stream->Read(&m_iAirlift);
	stream->Read(&m_iAirModifier);
	stream->Read(&m_iAirUnitCapacity);
	stream->Read(&m_iNukeModifier);
	stream->Read(&m_iNukeExplosionRand);
	stream->Read(&m_iFreeSpecialist);
	stream->Read(&m_iAreaFreeSpecialist);
	stream->Read(&m_iGlobalFreeSpecialist);
	stream->Read(&m_iHappiness);
	stream->Read(&m_iAreaHappiness);
	stream->Read(&m_iGlobalHappiness);
	stream->Read(&m_iStateReligionHappiness);
	stream->Read(&m_iWorkerSpeedModifier);
	stream->Read(&m_iMilitaryProductionModifier);
	stream->Read(&m_iSpaceProductionModifier);
	stream->Read(&m_iGlobalSpaceProductionModifier);
	stream->Read(&m_iTradeRoutes);
	stream->Read(&m_iCoastalTradeRoutes);
	stream->Read(&m_iAreaTradeRoutes); // advc.310
	stream->Read(&m_iTradeRouteModifier);
	stream->Read(&m_iForeignTradeRouteModifier);
	stream->Read(&m_iAssetValue);
	stream->Read(&m_iPowerValue);
	stream->Read(&m_iSpecialBuildingType);
	stream->Read(&m_iAdvisorType);
	stream->Read(&m_iHolyCity);
	stream->Read(&m_iReligionType);
	stream->Read(&m_iStateReligion);
	stream->Read(&m_iPrereqReligion);
	stream->Read(&m_iPrereqCorporation);
	stream->Read(&m_iFoundsCorporation);
	stream->Read(&m_iGlobalReligionCommerce);
	stream->Read(&m_iGlobalCorporationCommerce);
	stream->Read(&m_iPrereqAndBonus);
	stream->Read(&m_iGreatPeopleUnitClass);
	stream->Read(&m_iGreatPeopleRateChange);
	stream->Read(&m_iConquestProbability);
	stream->Read(&m_iMaintenanceModifier);
	stream->Read(&m_iWarWearinessModifier);
	stream->Read(&m_iGlobalWarWearinessModifier);
	stream->Read(&m_iEnemyWarWearinessModifier);
	stream->Read(&m_iHealRateChange);
	stream->Read(&m_iHealth);
	stream->Read(&m_iAreaHealth);
	stream->Read(&m_iGlobalHealth);
	stream->Read(&m_iGlobalPopulationChange);
	stream->Read(&m_iFreeTechs);
	stream->Read(&m_iDefenseModifier);
	stream->Read(&m_iBombardDefenseModifier);
	stream->Read(&m_iAllCityDefenseModifier);
	stream->Read(&m_iEspionageDefenseModifier);
	stream->Read(&m_iMissionType);
	stream->Read(&m_iVoteSourceType);
	stream->Read(&m_fVisibilityPriority);
	stream->Read(&m_bTeamShare);
	stream->Read(&m_bWater);
	stream->Read(&m_bRiver);
	stream->Read(&m_bPower);
	stream->Read(&m_bDirtyPower);
	stream->Read(&m_bAreaCleanPower);
	stream->Read(&m_bAreaBorderObstacle);
	stream->Read(&m_bForceTeamVoteEligible);
	stream->Read(&m_bCapital);
	stream->Read(&m_bGovernmentCenter);
	stream->Read(&m_bGoldenAge);
	stream->Read(&m_bMapCentering);
	stream->Read(&m_bNoUnhappiness);
	//stream->Read(&m_bNoUnhealthyPopulation);
	stream->Read(&m_iUnhealthyPopulationModifier); // K-Mod
	stream->Read(&m_bBuildingOnlyHealthy);
	stream->Read(&m_bNeverCapture);
	stream->Read(&m_bNukeImmune);
	stream->Read(&m_bPrereqReligion);
	stream->Read(&m_bCenterInCity);
	stream->Read(&m_bStateReligion);
	stream->Read(&m_bAllowsNukes);
	stream->ReadString(m_szConstructSound);
	stream->ReadString(m_szArtDefineTag);
	stream->ReadString(m_szMovieDefineTag);
	SAFE_DELETE_ARRAY(m_piPrereqAndTechs);
	m_piPrereqAndTechs = new int[GC.getNUM_BUILDING_AND_TECH_PREREQS()];
	stream->Read(GC.getNUM_BUILDING_AND_TECH_PREREQS(), m_piPrereqAndTechs);
	SAFE_DELETE_ARRAY(m_piPrereqOrBonuses);
	m_piPrereqOrBonuses = new int[GC.getNUM_BUILDING_PREREQ_OR_BONUSES()];
	stream->Read(GC.getNUM_BUILDING_PREREQ_OR_BONUSES(), m_piPrereqOrBonuses);
	SAFE_DELETE_ARRAY(m_piProductionTraits);
	m_piProductionTraits = new int[GC.getNumTraitInfos()];
	stream->Read(GC.getNumTraitInfos(), m_piProductionTraits);
	SAFE_DELETE_ARRAY(m_piHappinessTraits);
	m_piHappinessTraits = new int[GC.getNumTraitInfos()];
	stream->Read(GC.getNumTraitInfos(), m_piHappinessTraits);
	SAFE_DELETE_ARRAY(m_piSeaPlotYieldChange);
	m_piSeaPlotYieldChange = new int[NUM_YIELD_TYPES];
	stream->Read(NUM_YIELD_TYPES, m_piSeaPlotYieldChange);
	SAFE_DELETE_ARRAY(m_piRiverPlotYieldChange);
	m_piRiverPlotYieldChange = new int[NUM_YIELD_TYPES];
	stream->Read(NUM_YIELD_TYPES, m_piRiverPlotYieldChange);
	SAFE_DELETE_ARRAY(m_piGlobalSeaPlotYieldChange);
	m_piGlobalSeaPlotYieldChange = new int[NUM_YIELD_TYPES];
	stream->Read(NUM_YIELD_TYPES, m_piGlobalSeaPlotYieldChange);
	SAFE_DELETE_ARRAY(m_piYieldChange);
	m_piYieldChange = new int[NUM_YIELD_TYPES];
	stream->Read(NUM_YIELD_TYPES, m_piYieldChange);
	SAFE_DELETE_ARRAY(m_piYieldModifier);
	m_piYieldModifier = new int[NUM_YIELD_TYPES];
	stream->Read(NUM_YIELD_TYPES, m_piYieldModifier);
	SAFE_DELETE_ARRAY(m_piPowerYieldModifier);
	m_piPowerYieldModifier = new int[NUM_YIELD_TYPES];
	stream->Read(NUM_YIELD_TYPES, m_piPowerYieldModifier);
	SAFE_DELETE_ARRAY(m_piAreaYieldModifier);
	m_piAreaYieldModifier = new int[NUM_YIELD_TYPES];
	stream->Read(NUM_YIELD_TYPES, m_piAreaYieldModifier);
	SAFE_DELETE_ARRAY(m_piGlobalYieldModifier);
	m_piGlobalYieldModifier = new int[NUM_YIELD_TYPES];
	stream->Read(NUM_YIELD_TYPES, m_piGlobalYieldModifier);
	SAFE_DELETE_ARRAY(m_piCommerceChange);
	m_piCommerceChange = new int[NUM_COMMERCE_TYPES];
	stream->Read(NUM_COMMERCE_TYPES, m_piCommerceChange);
	SAFE_DELETE_ARRAY(m_piObsoleteSafeCommerceChange);
	m_piObsoleteSafeCommerceChange = new int[NUM_COMMERCE_TYPES];
	stream->Read(NUM_COMMERCE_TYPES, m_piObsoleteSafeCommerceChange);
	SAFE_DELETE_ARRAY(m_piCommerceChangeDoubleTime);
	m_piCommerceChangeDoubleTime = new int[NUM_COMMERCE_TYPES];
	stream->Read(NUM_COMMERCE_TYPES, m_piCommerceChangeDoubleTime);
	SAFE_DELETE_ARRAY(m_piCommerceModifier);
	m_piCommerceModifier = new int[NUM_COMMERCE_TYPES];
	stream->Read(NUM_COMMERCE_TYPES, m_piCommerceModifier);
	SAFE_DELETE_ARRAY(m_piGlobalCommerceModifier);
	m_piGlobalCommerceModifier = new int[NUM_COMMERCE_TYPES];
	stream->Read(NUM_COMMERCE_TYPES, m_piGlobalCommerceModifier);
	SAFE_DELETE_ARRAY(m_piSpecialistExtraCommerce);
	m_piSpecialistExtraCommerce = new int[NUM_COMMERCE_TYPES];
	stream->Read(NUM_COMMERCE_TYPES, m_piSpecialistExtraCommerce);
	SAFE_DELETE_ARRAY(m_piStateReligionCommerce);
	m_piStateReligionCommerce = new int[NUM_COMMERCE_TYPES];
	stream->Read(NUM_COMMERCE_TYPES, m_piStateReligionCommerce);
	SAFE_DELETE_ARRAY(m_piCommerceHappiness);
	m_piCommerceHappiness = new int[NUM_COMMERCE_TYPES];
	stream->Read(NUM_COMMERCE_TYPES, m_piCommerceHappiness);
	SAFE_DELETE_ARRAY(m_piReligionChange);
	m_piReligionChange = new int[GC.getNumReligionInfos()];
	stream->Read(GC.getNumReligionInfos(), m_piReligionChange);
	SAFE_DELETE_ARRAY(m_piSpecialistCount);
	m_piSpecialistCount = new int[GC.getNumSpecialistInfos()];
	stream->Read(GC.getNumSpecialistInfos(), m_piSpecialistCount);
	SAFE_DELETE_ARRAY(m_piFreeSpecialistCount);
	m_piFreeSpecialistCount = new int[GC.getNumSpecialistInfos()];
	stream->Read(GC.getNumSpecialistInfos(), m_piFreeSpecialistCount);
	SAFE_DELETE_ARRAY(m_piBonusHealthChanges);
	m_piBonusHealthChanges = new int[GC.getNumBonusInfos()];
	stream->Read(GC.getNumBonusInfos(), m_piBonusHealthChanges);
	SAFE_DELETE_ARRAY(m_piBonusHappinessChanges);
	m_piBonusHappinessChanges = new int[GC.getNumBonusInfos()];
	stream->Read(GC.getNumBonusInfos(), m_piBonusHappinessChanges);
	SAFE_DELETE_ARRAY(m_piBonusProductionModifier);
	m_piBonusProductionModifier = new int[GC.getNumBonusInfos()];
	stream->Read(GC.getNumBonusInfos(), m_piBonusProductionModifier);
	SAFE_DELETE_ARRAY(m_piUnitCombatFreeExperience);
	m_piUnitCombatFreeExperience = new int[GC.getNumUnitCombatInfos()];
	stream->Read(GC.getNumUnitCombatInfos(), m_piUnitCombatFreeExperience);
	SAFE_DELETE_ARRAY(m_piDomainFreeExperience);
	m_piDomainFreeExperience = new int[NUM_DOMAIN_TYPES];
	stream->Read(NUM_DOMAIN_TYPES, m_piDomainFreeExperience);
	SAFE_DELETE_ARRAY(m_piDomainProductionModifier);
	m_piDomainProductionModifier = new int[NUM_DOMAIN_TYPES];
	stream->Read(NUM_DOMAIN_TYPES, m_piDomainProductionModifier);
	SAFE_DELETE_ARRAY(m_piBuildingHappinessChanges);
	m_piBuildingHappinessChanges = new int[GC.getNumBuildingClassInfos()];
	stream->Read(GC.getNumBuildingClassInfos(), m_piBuildingHappinessChanges);
	SAFE_DELETE_ARRAY(m_piPrereqNumOfBuildingClass);
	m_piPrereqNumOfBuildingClass = new int[GC.getNumBuildingClassInfos()];
	stream->Read(GC.getNumBuildingClassInfos(), m_piPrereqNumOfBuildingClass);
	SAFE_DELETE_ARRAY(m_piFlavorValue);
	m_piFlavorValue = new int[GC.getNumFlavorTypes()];
	stream->Read(GC.getNumFlavorTypes(), m_piFlavorValue);
	SAFE_DELETE_ARRAY(m_piImprovementFreeSpecialist);
	m_piImprovementFreeSpecialist = new int[GC.getNumImprovementInfos()];
	stream->Read(GC.getNumImprovementInfos(), m_piImprovementFreeSpecialist);
	SAFE_DELETE_ARRAY(m_pbCommerceFlexible);
	m_pbCommerceFlexible = new bool[NUM_COMMERCE_TYPES];
	stream->Read(NUM_COMMERCE_TYPES, m_pbCommerceFlexible);
	SAFE_DELETE_ARRAY(m_pbCommerceChangeOriginalOwner);
	m_pbCommerceChangeOriginalOwner = new bool[NUM_COMMERCE_TYPES];
	stream->Read(NUM_COMMERCE_TYPES, m_pbCommerceChangeOriginalOwner);
	SAFE_DELETE_ARRAY(m_pbBuildingClassNeededInCity);
	m_pbBuildingClassNeededInCity = new bool[GC.getNumBuildingClassInfos()];
	stream->Read(GC.getNumBuildingClassInfos(), m_pbBuildingClassNeededInCity);
	if (m_ppaiSpecialistYieldChange != NULL)
	{
		for(int i = 0; i < GC.getNumSpecialistInfos(); i++)
			SAFE_DELETE_ARRAY(m_ppaiSpecialistYieldChange[i]);
		SAFE_DELETE_ARRAY(m_ppaiSpecialistYieldChange);
	}
	m_ppaiSpecialistYieldChange = new int*[GC.getNumSpecialistInfos()];
	for(int i = 0;i < GC.getNumSpecialistInfos(); i++)
	{
		m_ppaiSpecialistYieldChange[i]  = new int[NUM_YIELD_TYPES];
		stream->Read(NUM_YIELD_TYPES, m_ppaiSpecialistYieldChange[i]);
	} // UNOFFICIAL_PATCH, Efficiency, 06/27/10, Afforess & jdog5000: START
	m_bAnySpecialistYieldChange = false;
	for(int i = 0; !m_bAnySpecialistYieldChange && i < GC.getNumSpecialistInfos(); i++)
	{
		for(int j = 0; j < NUM_YIELD_TYPES; j++)
		{
			if (m_ppaiSpecialistYieldChange[i][j] != 0)
			{
				m_bAnySpecialistYieldChange = true;
				break;
			}
		}
	} // UNOFFICIAL_PATCH: END
	if (m_ppaiBonusYieldModifier != NULL)
	{
		for(int i = 0; i < GC.getNumBonusInfos(); i++)
			SAFE_DELETE_ARRAY(m_ppaiBonusYieldModifier[i]);
		SAFE_DELETE_ARRAY(m_ppaiBonusYieldModifier);
	}
	m_ppaiBonusYieldModifier = new int*[GC.getNumBonusInfos()];
	for(int i = 0; i < GC.getNumBonusInfos(); i++)
	{
		m_ppaiBonusYieldModifier[i]  = new int[NUM_YIELD_TYPES];
		stream->Read(NUM_YIELD_TYPES, m_ppaiBonusYieldModifier[i]);
	} // UNOFFICIAL_PATCH, Efficiency, 06/27/10, Afforess & jdog5000: START
	m_bAnyBonusYieldModifier = false;
	for(int i = 0; !m_bAnyBonusYieldModifier && i < GC.getNumBonusInfos(); i++)
	{
		for(int j=0; j < NUM_YIELD_TYPES; j++)
		{
			if (m_ppaiBonusYieldModifier[i][j] != 0)
			{
				m_bAnyBonusYieldModifier = true;
				break;
			}
		}
	} // UNOFFICIAL_PATCH: END
}

void CvBuildingInfo::write(FDataStreamBase* stream)
{
	CvHotkeyInfo::write(stream);
	uint uiFlag=0;
	stream->Write(uiFlag);

	stream->Write(m_iBuildingClassType);
	stream->Write(m_iVictoryPrereq);
	stream->Write(m_iFreeStartEra);
	stream->Write(m_iMaxStartEra);
	stream->Write(m_iObsoleteTech);
	stream->Write(m_iPrereqAndTech);
	stream->Write(m_iNoBonus);
	stream->Write(m_iPowerBonus);
	stream->Write(m_iFreeBonus);
	stream->Write(m_iNumFreeBonuses);
	stream->Write(m_iFreeBuildingClass);
	stream->Write(m_iFreePromotion);
	stream->Write(m_iCivicOption);
	stream->Write(m_iAIWeight);
	stream->Write(m_iProductionCost);
	stream->Write(m_iHurryCostModifier);
	stream->Write(m_iHurryAngerModifier);
	stream->Write(m_iAdvancedStartCost);
	stream->Write(m_iAdvancedStartCostIncrease);
	stream->Write(m_iMinAreaSize);
	stream->Write(m_iNumCitiesPrereq);
	stream->Write(m_iNumTeamsPrereq);
	stream->Write(m_iUnitLevelPrereq);
	stream->Write(m_iMinLatitude);
	stream->Write(m_iMaxLatitude);
	stream->Write(m_iGreatPeopleRateModifier);
	stream->Write(m_iGreatGeneralRateModifier);
	stream->Write(m_iDomesticGreatGeneralRateModifier);
	stream->Write(m_iGlobalGreatPeopleRateModifier);
	stream->Write(m_iAnarchyModifier);
	stream->Write(m_iGoldenAgeModifier);
	stream->Write(m_iGlobalHurryModifier);
	stream->Write(m_iFreeExperience);
	stream->Write(m_iGlobalFreeExperience);
	stream->Write(m_iFoodKept);
	stream->Write(m_iAirlift);
	stream->Write(m_iAirModifier);
	stream->Write(m_iAirUnitCapacity);
	stream->Write(m_iNukeModifier);
	stream->Write(m_iNukeExplosionRand);
	stream->Write(m_iFreeSpecialist);
	stream->Write(m_iAreaFreeSpecialist);
	stream->Write(m_iGlobalFreeSpecialist);
	stream->Write(m_iHappiness);
	stream->Write(m_iAreaHappiness);
	stream->Write(m_iGlobalHappiness);
	stream->Write(m_iStateReligionHappiness);
	stream->Write(m_iWorkerSpeedModifier);
	stream->Write(m_iMilitaryProductionModifier);
	stream->Write(m_iSpaceProductionModifier);
	stream->Write(m_iGlobalSpaceProductionModifier);
	stream->Write(m_iTradeRoutes);
	stream->Write(m_iCoastalTradeRoutes);
	stream->Write(m_iAreaTradeRoutes); // advc.310
	stream->Write(m_iTradeRouteModifier);
	stream->Write(m_iForeignTradeRouteModifier);
	stream->Write(m_iAssetValue);
	stream->Write(m_iPowerValue);
	stream->Write(m_iSpecialBuildingType);
	stream->Write(m_iAdvisorType);
	stream->Write(m_iHolyCity);
	stream->Write(m_iReligionType);
	stream->Write(m_iStateReligion);
	stream->Write(m_iPrereqReligion);
	stream->Write(m_iPrereqCorporation);
	stream->Write(m_iFoundsCorporation);
	stream->Write(m_iGlobalReligionCommerce);
	stream->Write(m_iGlobalCorporationCommerce);
	stream->Write(m_iPrereqAndBonus);
	stream->Write(m_iGreatPeopleUnitClass);
	stream->Write(m_iGreatPeopleRateChange);
	stream->Write(m_iConquestProbability);
	stream->Write(m_iMaintenanceModifier);
	stream->Write(m_iWarWearinessModifier);
	stream->Write(m_iGlobalWarWearinessModifier);
	stream->Write(m_iEnemyWarWearinessModifier);
	stream->Write(m_iHealRateChange);
	stream->Write(m_iHealth);
	stream->Write(m_iAreaHealth);
	stream->Write(m_iGlobalHealth);
	stream->Write(m_iGlobalPopulationChange);
	stream->Write(m_iFreeTechs);
	stream->Write(m_iDefenseModifier);
	stream->Write(m_iBombardDefenseModifier);
	stream->Write(m_iAllCityDefenseModifier);
	stream->Write(m_iEspionageDefenseModifier);
	stream->Write(m_iMissionType);
	stream->Write(m_iVoteSourceType);

	stream->Write(m_fVisibilityPriority);

	stream->Write(m_bTeamShare);
	stream->Write(m_bWater);
	stream->Write(m_bRiver);
	stream->Write(m_bPower);
	stream->Write(m_bDirtyPower);
	stream->Write(m_bAreaCleanPower);
	stream->Write(m_bAreaBorderObstacle);
	stream->Write(m_bForceTeamVoteEligible);
	stream->Write(m_bCapital);
	stream->Write(m_bGovernmentCenter);
	stream->Write(m_bGoldenAge);
	stream->Write(m_bMapCentering);
	stream->Write(m_bNoUnhappiness);
	//stream->Write(m_bNoUnhealthyPopulation);
	stream->Write(m_iUnhealthyPopulationModifier); // K-Mod
	stream->Write(m_bBuildingOnlyHealthy);
	stream->Write(m_bNeverCapture);
	stream->Write(m_bNukeImmune);
	stream->Write(m_bPrereqReligion);
	stream->Write(m_bCenterInCity);
	stream->Write(m_bStateReligion);
	stream->Write(m_bAllowsNukes);
	stream->WriteString(m_szConstructSound);
	stream->WriteString(m_szArtDefineTag);
	stream->WriteString(m_szMovieDefineTag);
	stream->Write(GC.getNUM_BUILDING_AND_TECH_PREREQS(), m_piPrereqAndTechs);
	stream->Write(GC.getNUM_BUILDING_PREREQ_OR_BONUSES(), m_piPrereqOrBonuses);
	stream->Write(GC.getNumTraitInfos(), m_piProductionTraits);
	stream->Write(GC.getNumTraitInfos(), m_piHappinessTraits);
	stream->Write(NUM_YIELD_TYPES, m_piSeaPlotYieldChange);
	stream->Write(NUM_YIELD_TYPES, m_piRiverPlotYieldChange);
	stream->Write(NUM_YIELD_TYPES, m_piGlobalSeaPlotYieldChange);
	stream->Write(NUM_YIELD_TYPES, m_piYieldChange);
	stream->Write(NUM_YIELD_TYPES, m_piYieldModifier);
	stream->Write(NUM_YIELD_TYPES, m_piPowerYieldModifier);
	stream->Write(NUM_YIELD_TYPES, m_piAreaYieldModifier);
	stream->Write(NUM_YIELD_TYPES, m_piGlobalYieldModifier);
	stream->Write(NUM_COMMERCE_TYPES, m_piCommerceChange);
	stream->Write(NUM_COMMERCE_TYPES, m_piObsoleteSafeCommerceChange);
	stream->Write(NUM_COMMERCE_TYPES, m_piCommerceChangeDoubleTime);
	stream->Write(NUM_COMMERCE_TYPES, m_piCommerceModifier);
	stream->Write(NUM_COMMERCE_TYPES, m_piGlobalCommerceModifier);
	stream->Write(NUM_COMMERCE_TYPES, m_piSpecialistExtraCommerce);
	stream->Write(NUM_COMMERCE_TYPES, m_piStateReligionCommerce);
	stream->Write(NUM_COMMERCE_TYPES, m_piCommerceHappiness);
	stream->Write(GC.getNumReligionInfos(), m_piReligionChange);
	stream->Write(GC.getNumSpecialistInfos(), m_piSpecialistCount);
	stream->Write(GC.getNumSpecialistInfos(), m_piFreeSpecialistCount);
	stream->Write(GC.getNumBonusInfos(), m_piBonusHealthChanges);
	stream->Write(GC.getNumBonusInfos(), m_piBonusHappinessChanges);
	stream->Write(GC.getNumBonusInfos(), m_piBonusProductionModifier);
	stream->Write(GC.getNumUnitCombatInfos(), m_piUnitCombatFreeExperience);
	stream->Write(NUM_DOMAIN_TYPES, m_piDomainFreeExperience);
	stream->Write(NUM_DOMAIN_TYPES, m_piDomainProductionModifier);
	stream->Write(GC.getNumBuildingClassInfos(), m_piBuildingHappinessChanges);
	stream->Write(GC.getNumBuildingClassInfos(), m_piPrereqNumOfBuildingClass);
	stream->Write(GC.getNumFlavorTypes(), m_piFlavorValue);
	stream->Write(GC.getNumImprovementInfos(), m_piImprovementFreeSpecialist);
	stream->Write(NUM_COMMERCE_TYPES, m_pbCommerceFlexible);
	stream->Write(NUM_COMMERCE_TYPES, m_pbCommerceChangeOriginalOwner);
	stream->Write(GC.getNumBuildingClassInfos(), m_pbBuildingClassNeededInCity);
	for(int i = 0 ;i < GC.getNumSpecialistInfos(); i++)
		stream->Write(NUM_YIELD_TYPES, m_ppaiSpecialistYieldChange[i]);
	for(int i = 0; i < GC.getNumBonusInfos(); i++)
		stream->Write(NUM_YIELD_TYPES, m_ppaiBonusYieldModifier[i]);
}
#endif

bool CvBuildingInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvHotkeyInfo::read(pXML))
		return false;

	CvString szTextVal;

	pXML->GetChildXmlValByName(szTextVal, "BuildingClass");
	m_iBuildingClassType = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "SpecialBuildingType");
	m_iSpecialBuildingType = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "Advisor");
	m_iAdvisorType = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "ArtDefineTag");
	setArtDefineTag(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "MovieDefineTag");
	setMovieDefineTag(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "HolyCity");
	m_iHolyCity = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "ReligionType");
	m_iReligionType = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "StateReligion");
	m_iStateReligion = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "PrereqReligion");
	m_iPrereqReligion = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "PrereqCorporation");
	m_iPrereqCorporation = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "FoundsCorporation");
	m_iFoundsCorporation = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "GlobalReligionCommerce");
	m_iGlobalReligionCommerce = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "GlobalCorporationCommerce");
	m_iGlobalCorporationCommerce = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "VictoryPrereq");
	m_iVictoryPrereq = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "FreeStartEra");
	m_iFreeStartEra = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "MaxStartEra");
	m_iMaxStartEra = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "ObsoleteTech");
	m_iObsoleteTech = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "PrereqTech");
	m_iPrereqAndTech = pXML->FindInInfoClass(szTextVal);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"TechTypes"))
	{
		if (pXML->SkipToNextVal())
		{
			int iNumSibs = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
			pXML->InitList(&m_piPrereqAndTechs, GC.getNUM_BUILDING_AND_TECH_PREREQS(), -1);
			bool bAnyReq = false; // advc.003t
			if (iNumSibs > 0)
			{
				if (pXML->GetChildXmlVal(szTextVal))
				{
					FAssertMsg((iNumSibs <= GC.getNUM_BUILDING_AND_TECH_PREREQS()),"For loop iterator is greater than array size");
					for (int j = 0; j < iNumSibs; j++)
					{
						m_piPrereqAndTechs[j] = pXML->FindInInfoClass(szTextVal);
						// <advc.003t>
						if (m_piPrereqAndTechs[j] != NO_TECH)
							bAnyReq = true; // </advc.003t>
						if (!pXML->GetNextXmlVal(szTextVal))
							break;
					}
					gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
				}
			} // <advc.003t>
			if (!bAnyReq)
				SAFE_DELETE_ARRAY(m_piPrereqAndTechs); // </advc.003t>
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}

	pXML->GetChildXmlValByName(szTextVal, "Bonus");
	m_iPrereqAndBonus = pXML->FindInInfoClass(szTextVal);
	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"PrereqBonuses"))
	{
		if (pXML->SkipToNextVal())
		{
			int iNumChildren = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
			pXML->InitList(&m_piPrereqOrBonuses, GC.getNUM_BUILDING_PREREQ_OR_BONUSES(), -1);
			bool bAnyReq = false; // advc.003t
			if (iNumChildren > 0)
			{
				if (pXML->GetChildXmlVal(szTextVal))
				{
					FAssertMsg((iNumChildren <= GC.getNUM_BUILDING_PREREQ_OR_BONUSES()),"For loop iterator is greater than array size");
					for (int j = 0; j < iNumChildren; j++)
					{
						m_piPrereqOrBonuses[j] = pXML->FindInInfoClass(szTextVal);
						// <advc.003t>
						if (m_piPrereqOrBonuses[j] != NO_BONUS)
							bAnyReq = true; // </advc.003t>
						if (!pXML->GetNextXmlVal(szTextVal))
							break;
					}
					gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
				}
			} // <advc.003t>
			if (!bAnyReq)
				SAFE_DELETE_ARRAY(m_piPrereqOrBonuses); // </advc.003t>
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}

	pXML->SetVariableListTagPair(&m_piProductionTraits, "ProductionTraits", GC.getNumTraitInfos());
	pXML->SetVariableListTagPair(&m_piHappinessTraits, "HappinessTraits", GC.getNumTraitInfos());

	pXML->GetChildXmlValByName(szTextVal, "NoBonus");
	m_iNoBonus = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "PowerBonus");
	m_iPowerBonus = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "FreeBonus");
	m_iFreeBonus = pXML->FindInInfoClass(szTextVal);
	pXML->GetChildXmlValByName(&m_iNumFreeBonuses, "iNumFreeBonuses");

	pXML->GetChildXmlValByName(szTextVal, "FreeBuilding");
	m_iFreeBuildingClass = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "FreePromotion");
	m_iFreePromotion = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "CivicOption");
	m_iCivicOption = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "GreatPeopleUnitClass");
	m_iGreatPeopleUnitClass = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "DiploVoteType");
	m_iVoteSourceType = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(&m_iGreatPeopleRateChange, "iGreatPeopleRateChange");
	pXML->GetChildXmlValByName(&m_bTeamShare, "bTeamShare");
	pXML->GetChildXmlValByName(&m_bWater, "bWater");
	pXML->GetChildXmlValByName(&m_bRiver, "bRiver");
	pXML->GetChildXmlValByName(&m_bPower, "bPower");
	pXML->GetChildXmlValByName(&m_bDirtyPower, "bDirtyPower");
	pXML->GetChildXmlValByName(&m_bAreaCleanPower, "bAreaCleanPower");
	pXML->GetChildXmlValByName(&m_bAreaBorderObstacle, "bBorderObstacle");
	pXML->GetChildXmlValByName(&m_bForceTeamVoteEligible, "bForceTeamVoteEligible");
	pXML->GetChildXmlValByName(&m_bCapital, "bCapital");
	pXML->GetChildXmlValByName(&m_bGovernmentCenter, "bGovernmentCenter");
	pXML->GetChildXmlValByName(&m_bGoldenAge, "bGoldenAge");
	pXML->GetChildXmlValByName(&m_bAllowsNukes, "bAllowsNukes");
	pXML->GetChildXmlValByName(&m_bMapCentering, "bMapCentering");
	pXML->GetChildXmlValByName(&m_bNoUnhappiness, "bNoUnhappiness");
	//pXML->GetChildXmlValByName(&m_bNoUnhealthyPopulation, "bNoUnhealthyPopulation");
	pXML->GetChildXmlValByName(&m_iUnhealthyPopulationModifier, "iUnhealthyPopulationModifier"); // K-Mod
	pXML->GetChildXmlValByName(&m_bBuildingOnlyHealthy, "bBuildingOnlyHealthy");
	pXML->GetChildXmlValByName(&m_bNeverCapture, "bNeverCapture");
	pXML->GetChildXmlValByName(&m_bNukeImmune, "bNukeImmune");
	pXML->GetChildXmlValByName(&m_bPrereqReligion, "bPrereqReligion");
	pXML->GetChildXmlValByName(&m_bCenterInCity, "bCenterInCity");
	pXML->GetChildXmlValByName(&m_bStateReligion, "bStateReligion");
	pXML->GetChildXmlValByName(&m_iAIWeight, "iAIWeight");
	pXML->GetChildXmlValByName(&m_iProductionCost, "iCost");
	pXML->GetChildXmlValByName(&m_iHurryCostModifier, "iHurryCostModifier");
	pXML->GetChildXmlValByName(&m_iHurryAngerModifier, "iHurryAngerModifier");
	pXML->GetChildXmlValByName(&m_iAdvancedStartCost, "iAdvancedStartCost");
	pXML->GetChildXmlValByName(&m_iAdvancedStartCostIncrease, "iAdvancedStartCostIncrease");
	pXML->GetChildXmlValByName(&m_iMinAreaSize, "iMinAreaSize");
	pXML->GetChildXmlValByName(&m_iConquestProbability, "iConquestProb");
	pXML->GetChildXmlValByName(&m_iNumCitiesPrereq, "iCitiesPrereq");
	pXML->GetChildXmlValByName(&m_iNumTeamsPrereq, "iTeamsPrereq");
	pXML->GetChildXmlValByName(&m_iUnitLevelPrereq, "iLevelPrereq");
	pXML->GetChildXmlValByName(&m_iMinLatitude, "iMinLatitude");
	pXML->GetChildXmlValByName(&m_iMaxLatitude, "iMaxLatitude", 90);
	pXML->GetChildXmlValByName(&m_iGreatPeopleRateModifier, "iGreatPeopleRateModifier");
	pXML->GetChildXmlValByName(&m_iGreatGeneralRateModifier, "iGreatGeneralRateModifier");
	pXML->GetChildXmlValByName(&m_iDomesticGreatGeneralRateModifier, "iDomesticGreatGeneralRateModifier");
	pXML->GetChildXmlValByName(&m_iGlobalGreatPeopleRateModifier, "iGlobalGreatPeopleRateModifier");
	pXML->GetChildXmlValByName(&m_iAnarchyModifier, "iAnarchyModifier");
	pXML->GetChildXmlValByName(&m_iGoldenAgeModifier, "iGoldenAgeModifier");
	pXML->GetChildXmlValByName(&m_iGlobalHurryModifier, "iGlobalHurryModifier");
	pXML->GetChildXmlValByName(&m_iFreeExperience, "iExperience");
	pXML->GetChildXmlValByName(&m_iGlobalFreeExperience, "iGlobalExperience");
	pXML->GetChildXmlValByName(&m_iFoodKept, "iFoodKept");
	pXML->GetChildXmlValByName(&m_iAirlift, "iAirlift");
	pXML->GetChildXmlValByName(&m_iAirModifier, "iAirModifier");
	pXML->GetChildXmlValByName(&m_iAirUnitCapacity, "iAirUnitCapacity");
	pXML->GetChildXmlValByName(&m_iNukeModifier, "iNukeModifier");
	pXML->GetChildXmlValByName(&m_iNukeExplosionRand, "iNukeExplosionRand");
	pXML->GetChildXmlValByName(&m_iFreeSpecialist, "iFreeSpecialist");
	pXML->GetChildXmlValByName(&m_iAreaFreeSpecialist, "iAreaFreeSpecialist");
	pXML->GetChildXmlValByName(&m_iGlobalFreeSpecialist, "iGlobalFreeSpecialist");
	pXML->GetChildXmlValByName(&m_iMaintenanceModifier, "iMaintenanceModifier");
	pXML->GetChildXmlValByName(&m_iWarWearinessModifier, "iWarWearinessModifier");
	pXML->GetChildXmlValByName(&m_iGlobalWarWearinessModifier, "iGlobalWarWearinessModifier");
	pXML->GetChildXmlValByName(&m_iEnemyWarWearinessModifier, "iEnemyWarWearinessModifier");
	pXML->GetChildXmlValByName(&m_iHealRateChange, "iHealRateChange");
	pXML->GetChildXmlValByName(&m_iHealth, "iHealth");
	pXML->GetChildXmlValByName(&m_iAreaHealth, "iAreaHealth");
	pXML->GetChildXmlValByName(&m_iGlobalHealth, "iGlobalHealth");
	pXML->GetChildXmlValByName(&m_iHappiness, "iHappiness");
	pXML->GetChildXmlValByName(&m_iAreaHappiness, "iAreaHappiness");
	pXML->GetChildXmlValByName(&m_iGlobalHappiness, "iGlobalHappiness");
	pXML->GetChildXmlValByName(&m_iStateReligionHappiness, "iStateReligionHappiness");
	pXML->GetChildXmlValByName(&m_iWorkerSpeedModifier, "iWorkerSpeedModifier");
	pXML->GetChildXmlValByName(&m_iMilitaryProductionModifier, "iMilitaryProductionModifier");
	pXML->GetChildXmlValByName(&m_iSpaceProductionModifier, "iSpaceProductionModifier");
	pXML->GetChildXmlValByName(&m_iGlobalSpaceProductionModifier, "iGlobalSpaceProductionModifier");
	pXML->GetChildXmlValByName(&m_iTradeRoutes, "iTradeRoutes");
	pXML->GetChildXmlValByName(&m_iCoastalTradeRoutes, "iCoastalTradeRoutes");
	pXML->GetChildXmlValByName(&m_iAreaTradeRoutes, "iAreaTradeRoutes"); // advc.310
	pXML->GetChildXmlValByName(&m_iTradeRouteModifier, "iTradeRouteModifier");
	pXML->GetChildXmlValByName(&m_iForeignTradeRouteModifier, "iForeignTradeRouteModifier");
	pXML->GetChildXmlValByName(&m_iGlobalPopulationChange, "iGlobalPopulationChange");
	pXML->GetChildXmlValByName(&m_iFreeTechs, "iFreeTechs");
	pXML->GetChildXmlValByName(&m_iDefenseModifier, "iDefense");
	pXML->GetChildXmlValByName(&m_iBombardDefenseModifier, "iBombardDefense");
	pXML->GetChildXmlValByName(&m_iAllCityDefenseModifier, "iAllCityDefense");
	pXML->GetChildXmlValByName(&m_iEspionageDefenseModifier, "iEspionageDefense");
	pXML->GetChildXmlValByName(&m_iAssetValue, "iAsset");
	pXML->GetChildXmlValByName(&m_iPowerValue, "iPower");
	pXML->GetChildXmlValByName(&m_fVisibilityPriority, "fVisibilityPriority");

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "SeaPlotYieldChanges"))
	{
		pXML->SetYields(&m_piSeaPlotYieldChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piSeaPlotYieldChange, NUM_YIELD_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "RiverPlotYieldChanges"))
	{
		pXML->SetYields(&m_piRiverPlotYieldChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piRiverPlotYieldChange, NUM_YIELD_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "GlobalSeaPlotYieldChanges"))
	{
		pXML->SetYields(&m_piGlobalSeaPlotYieldChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piGlobalSeaPlotYieldChange, NUM_YIELD_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "YieldChanges"))
	{
		pXML->SetYields(&m_piYieldChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piYieldChange, NUM_YIELD_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "YieldModifiers"))
	{
		pXML->SetYields(&m_piYieldModifier);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piYieldModifier, NUM_YIELD_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "PowerYieldModifiers"))
	{
		pXML->SetYields(&m_piPowerYieldModifier);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piPowerYieldModifier, NUM_YIELD_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(), "AreaYieldModifiers"))
	{
		pXML->SetYields(&m_piAreaYieldModifier);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piAreaYieldModifier, NUM_YIELD_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"GlobalYieldModifiers"))
	{
		pXML->SetYields(&m_piGlobalYieldModifier);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piGlobalYieldModifier, NUM_YIELD_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"CommerceChanges"))
	{
		pXML->SetCommerce(&m_piCommerceChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piCommerceChange, NUM_COMMERCE_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"ObsoleteSafeCommerceChanges"))
	{
		pXML->SetCommerce(&m_piObsoleteSafeCommerceChange);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piObsoleteSafeCommerceChange, NUM_COMMERCE_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"CommerceChangeDoubleTimes"))
	{
		pXML->SetCommerce(&m_piCommerceChangeDoubleTime);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piCommerceChangeDoubleTime, NUM_COMMERCE_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"CommerceModifiers"))
	{
		pXML->SetCommerce(&m_piCommerceModifier);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piCommerceModifier, NUM_COMMERCE_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"GlobalCommerceModifiers"))
	{
		pXML->SetCommerce(&m_piGlobalCommerceModifier);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piGlobalCommerceModifier, NUM_COMMERCE_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"SpecialistExtraCommerces"))
	{
		pXML->SetCommerce(&m_piSpecialistExtraCommerce);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piSpecialistExtraCommerce, NUM_COMMERCE_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"StateReligionCommerces"))
	{
		pXML->SetCommerce(&m_piStateReligionCommerce);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piStateReligionCommerce, NUM_COMMERCE_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"CommerceHappinesses"))
	{
		pXML->SetCommerce(&m_piCommerceHappiness);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_piCommerceHappiness, NUM_COMMERCE_TYPES);

	pXML->SetVariableListTagPair(&m_piReligionChange, "ReligionChanges", GC.getNumReligionInfos());

	pXML->SetVariableListTagPair(&m_piSpecialistCount, "SpecialistCounts", GC.getNumSpecialistInfos());
	pXML->SetVariableListTagPair(&m_piFreeSpecialistCount, "FreeSpecialistCounts", GC.getNumSpecialistInfos());

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"CommerceFlexibles"))
	{
		pXML->SetCommerce(&m_pbCommerceFlexible);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_pbCommerceFlexible, NUM_COMMERCE_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"CommerceChangeOriginalOwners"))
	{
		pXML->SetCommerce(&m_pbCommerceChangeOriginalOwner);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_pbCommerceChangeOriginalOwner, NUM_COMMERCE_TYPES);

	pXML->GetChildXmlValByName(szTextVal, "ConstructSound");
	setConstructSound(szTextVal);

	pXML->SetVariableListTagPair(&m_piBonusHealthChanges, "BonusHealthChanges", GC.getNumBonusInfos());
	pXML->SetVariableListTagPair(&m_piBonusHappinessChanges, "BonusHappinessChanges", GC.getNumBonusInfos());
	pXML->SetVariableListTagPair(&m_piBonusProductionModifier, "BonusProductionModifiers", GC.getNumBonusInfos());

	pXML->SetVariableListTagPair(&m_piUnitCombatFreeExperience, "UnitCombatFreeExperiences", GC.getNumUnitCombatInfos());

	pXML->SetVariableListTagPair(&m_piDomainFreeExperience, "DomainFreeExperiences", NUM_DOMAIN_TYPES);
	pXML->SetVariableListTagPair(&m_piDomainProductionModifier, "DomainProductionModifiers", NUM_DOMAIN_TYPES);

	pXML->SetVariableListTagPair(&m_piPrereqNumOfBuildingClass, "PrereqBuildingClasses", GC.getNumBuildingClassInfos());
	pXML->SetVariableListTagPair(&m_pbBuildingClassNeededInCity, "BuildingClassNeededs", GC.getNumBuildingClassInfos());

	// UNOFFICIAL_PATCH, Efficiency, 06/27/10, Afforess & jdog5000: START
	m_bAnySpecialistYieldChange = false;
	pXML->Init2DIntList(&m_ppaiSpecialistYieldChange, GC.getNumSpecialistInfos(), NUM_YIELD_TYPES);
	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"SpecialistYieldChanges"))
	{
		int iNumChildren = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
		if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"SpecialistYieldChange"))
		{
			for(int j = 0; j < iNumChildren; j++)
			{
				pXML->GetChildXmlValByName(szTextVal, "SpecialistType");
				int k = pXML->FindInInfoClass(szTextVal);
				if (k > -1)
				{
					SAFE_DELETE_ARRAY(m_ppaiSpecialistYieldChange[k]);
					if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"YieldChanges"))
					{
						pXML->SetYields(&m_ppaiSpecialistYieldChange[k]);
						gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
					}
					else pXML->InitList(&m_ppaiSpecialistYieldChange[k], NUM_YIELD_TYPES);
				}
				if (!gDLL->getXMLIFace()->NextSibling(pXML->GetXML()))
					break;
			}

			for(int ii = 0; !m_bAnySpecialistYieldChange && ii < GC.getNumSpecialistInfos(); ii++)
			{
				for(int ij = 0; ij < NUM_YIELD_TYPES; ij++)
				{
					if (m_ppaiSpecialistYieldChange[ii][ij] != 0)
					{
						m_bAnySpecialistYieldChange = true;
						break;
					}
				}
			}
			gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	m_bAnyBonusYieldModifier = false;
	pXML->Init2DIntList(&m_ppaiBonusYieldModifier, GC.getNumBonusInfos(), NUM_YIELD_TYPES);
	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"BonusYieldModifiers"))
	{
		int iNumChildren = gDLL->getXMLIFace()->GetNumChildren(pXML->GetXML());
		if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"BonusYieldModifier"))
		{
			for(int j = 0; j < iNumChildren; j++)
			{
				pXML->GetChildXmlValByName(szTextVal, "BonusType");
				int k = pXML->FindInInfoClass(szTextVal);
				if (k > -1)
				{
					SAFE_DELETE_ARRAY(m_ppaiBonusYieldModifier[k]);
					if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"YieldModifiers"))
					{
						pXML->SetYields(&m_ppaiBonusYieldModifier[k]);
						gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
					}
					else pXML->InitList(&m_ppaiBonusYieldModifier[k], NUM_YIELD_TYPES);

				}

				if (!gDLL->getXMLIFace()->NextSibling(pXML->GetXML()))
					break;
			}

			for(int ii = 0; !m_bAnyBonusYieldModifier && ii < GC.getNumBonusInfos(); ii++)
			{
				for(int ij=0; ij < NUM_YIELD_TYPES; ij++)
				{
					if (m_ppaiBonusYieldModifier[ii][ij] != 0)
					{
						m_bAnyBonusYieldModifier = true;
						break;
					}
				}
			}
			gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
		}
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	// UNOFFICIAL_PATCH: END

	pXML->SetVariableListTagPair(&m_piFlavorValue, "Flavors", GC.getNumFlavorTypes());
	pXML->SetVariableListTagPair(&m_piImprovementFreeSpecialist, "ImprovementFreeSpecialists", GC.getNumImprovementInfos());
	pXML->SetVariableListTagPair(&m_piBuildingHappinessChanges, "BuildingHappinessChanges", GC.getNumBuildingClassInfos());

	return true;
}
// <advc.310>
bool CvBuildingInfo::m_bEnabledDomesticGreatGeneralRateModifier = true;
bool CvBuildingInfo::m_bEnabledAreaTradeRoutes = true;
bool CvBuildingInfo::m_bEnabledAreaBorderObstacle = true;
void CvBuildingInfo::setDomesticGreatGeneralRateModifierEnabled(bool b) {
	m_bEnabledDomesticGreatGeneralRateModifier = b;
}
void CvBuildingInfo::setAreaTradeRoutesEnabled(bool b) {
	m_bEnabledAreaTradeRoutes = b;
}
void CvBuildingInfo::setAreaBorderObstacleEnabled(bool b) {
	m_bEnabledAreaBorderObstacle = b;
} // </advc.310>

CvBuildingClassInfo::CvBuildingClassInfo() :
m_iMaxGlobalInstances(0),
m_iMaxTeamInstances(0),
m_iMaxPlayerInstances(0),
m_iExtraPlayerInstances(0),
m_iDefaultBuildingIndex(NO_BUILDING),
m_bNoLimit(false),
m_bMonument(false),
m_piVictoryThreshold(NULL)
{}

CvBuildingClassInfo::~CvBuildingClassInfo()
{
	SAFE_DELETE_ARRAY(m_piVictoryThreshold);
}

int CvBuildingClassInfo::getMaxGlobalInstances() const
{
	return m_iMaxGlobalInstances;
}

int CvBuildingClassInfo::getMaxTeamInstances() const
{
	return m_iMaxTeamInstances;
}

int CvBuildingClassInfo::getMaxPlayerInstances() const
{
	return m_iMaxPlayerInstances;
}

int CvBuildingClassInfo::getExtraPlayerInstances() const
{
	return m_iExtraPlayerInstances;
}

int CvBuildingClassInfo::getDefaultBuildingIndex() const
{
	return m_iDefaultBuildingIndex;
}

bool CvBuildingClassInfo::isNoLimit() const
{
	return m_bNoLimit;
}

bool CvBuildingClassInfo::isMonument() const
{
	return m_bMonument;
}

/*  advc (comment): Unused in XML. Number of buildings of this class required for
	victory i as a necessary (not sufficient) condition. */
int CvBuildingClassInfo::getVictoryThreshold(int i) const
{
	FAssertMsg(i < GC.getNumVictoryInfos(), "Index out of bounds");
	FAssertMsg(i > -1, "Index out of bounds");
	return m_piVictoryThreshold ? m_piVictoryThreshold[i] : 0; // advc.003t
}

bool CvBuildingClassInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvInfoBase::read(pXML))
		return false;

	pXML->GetChildXmlValByName(&m_iMaxGlobalInstances, "iMaxGlobalInstances");
	pXML->GetChildXmlValByName(&m_iMaxTeamInstances, "iMaxTeamInstances");
	pXML->GetChildXmlValByName(&m_iMaxPlayerInstances, "iMaxPlayerInstances");
	pXML->GetChildXmlValByName(&m_iExtraPlayerInstances, "iExtraPlayerInstances");

	pXML->GetChildXmlValByName(&m_bNoLimit, "bNoLimit");
	pXML->GetChildXmlValByName(&m_bMonument, "bMonument");

	pXML->SetVariableListTagPair(&m_piVictoryThreshold, "VictoryThresholds", GC.getNumVictoryInfos());

	CvString szTextVal;
	pXML->GetChildXmlValByName(szTextVal, "DefaultBuilding");
	m_aszExtraXMLforPass3.push_back(szTextVal);

	return true;
}

bool CvBuildingClassInfo::readPass3()
{
	if (m_aszExtraXMLforPass3.size() < 1)
	{
		FAssert(false);
		return false;
	}

	m_iDefaultBuildingIndex = GC.getInfoTypeForString(m_aszExtraXMLforPass3[0]);
	m_aszExtraXMLforPass3.clear();

	return true;
}

CvSpecialBuildingInfo::CvSpecialBuildingInfo() :
m_iObsoleteTech(NO_TECH),
m_iTechPrereq(NO_TECH),
m_iTechPrereqAnyone(NO_TECH),
m_bValid(false),
m_piProductionTraits(NULL)
{}

CvSpecialBuildingInfo::~CvSpecialBuildingInfo()
{
	SAFE_DELETE_ARRAY(m_piProductionTraits);
}

int CvSpecialBuildingInfo::getObsoleteTech() const
{
	return m_iObsoleteTech;
}

int CvSpecialBuildingInfo::getTechPrereq() const
{
	return m_iTechPrereq;
}

int CvSpecialBuildingInfo::getTechPrereqAnyone() const
{
	return m_iTechPrereqAnyone;
}

bool CvSpecialBuildingInfo::isValid() const
{
	return m_bValid;
}

int CvSpecialBuildingInfo::getProductionTraits(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumTraitInfos(), i, "CvSpecialBuildingInfo::getProductionTraits");
	return m_piProductionTraits ? m_piProductionTraits[i]
			: 0; // advc.003t: Was -1. This is the production discount percentage.
}

bool CvSpecialBuildingInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvInfoBase::read(pXML))
		return false;

	CvString szTextVal;
	pXML->GetChildXmlValByName(szTextVal, "ObsoleteTech");
	m_iObsoleteTech = pXML->FindInInfoClass(szTextVal);
	pXML->GetChildXmlValByName(szTextVal, "TechPrereq");
	m_iTechPrereq = pXML->FindInInfoClass(szTextVal);
	pXML->GetChildXmlValByName(szTextVal, "TechPrereqAnyone");
	m_iTechPrereqAnyone = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(&m_bValid, "bValid");

	pXML->SetVariableListTagPair(&m_piProductionTraits, "ProductionTraits", GC.getNumTraitInfos());

	return true;
}

CvVoteSourceInfo::CvVoteSourceInfo() :
	m_iVoteInterval(0),
	m_iFreeSpecialist(NO_SPECIALIST),
	m_iCivic(NO_CIVIC),
	m_aiReligionYields(NULL),
	m_aiReligionCommerces(NULL)
{}

CvVoteSourceInfo::~CvVoteSourceInfo()
{
	SAFE_DELETE_ARRAY(m_aiReligionYields);
	SAFE_DELETE_ARRAY(m_aiReligionCommerces);
}

int CvVoteSourceInfo::getVoteInterval() const
{
	return m_iVoteInterval;
}

int CvVoteSourceInfo::getFreeSpecialist() const
{
	return m_iFreeSpecialist;
}

int CvVoteSourceInfo::getCivic() const
{
	return m_iCivic;
}

int CvVoteSourceInfo::getReligionYield(int i) const
{
	FAssert(i >= 0 && i < NUM_YIELD_TYPES);
	return m_aiReligionYields[i];
}

int CvVoteSourceInfo::getReligionCommerce(int i) const
{
	FAssert(i >= 0 && i < NUM_COMMERCE_TYPES);
	return m_aiReligionCommerces[i];
}

const CvWString CvVoteSourceInfo::getPopupText() const
{
	return gDLL->getText(m_szPopupText);
}

const CvWString CvVoteSourceInfo::getSecretaryGeneralText() const
{
	return gDLL->getText(m_szSecretaryGeneralText);
}

bool CvVoteSourceInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvInfoBase::read(pXML))
		return false;

	pXML->GetChildXmlValByName(&m_iVoteInterval, "iVoteInterval");
	pXML->GetChildXmlValByName(m_szPopupText, "PopupText");
	pXML->GetChildXmlValByName(m_szSecretaryGeneralText, "SecretaryGeneralText");

	CvString szTextVal;
	pXML->GetChildXmlValByName(szTextVal, "FreeSpecialist");
	m_iFreeSpecialist = GC.getInfoTypeForString(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "Civic");
	m_aszExtraXMLforPass3.push_back(szTextVal);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"ReligionYields"))
	{
		pXML->SetCommerce(&m_aiReligionYields);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_aiReligionYields, NUM_YIELD_TYPES);

	if (gDLL->getXMLIFace()->SetToChildByTagName(pXML->GetXML(),"ReligionCommerces"))
	{
		pXML->SetCommerce(&m_aiReligionCommerces);
		gDLL->getXMLIFace()->SetToParent(pXML->GetXML());
	}
	else pXML->InitList(&m_aiReligionCommerces, NUM_COMMERCE_TYPES);

	return true;
}

bool CvVoteSourceInfo::readPass3()
{
	if (m_aszExtraXMLforPass3.size() < 1)
	{
		FAssert(false);
		return false;
	}

	m_iCivic = GC.getInfoTypeForString(m_aszExtraXMLforPass3[0]);
	m_aszExtraXMLforPass3.clear();

	return true;
}

CvVoteInfo::CvVoteInfo() :
m_iPopulationThreshold(0),
m_iStateReligionVotePercent(0),
m_iTradeRoutes(0),
m_iMinVoters(0),
m_bSecretaryGeneral(false),
m_bVictory(false),
m_bFreeTrade(false),
m_bNoNukes(false),
m_bCityVoting(false),
m_bCivVoting(false),
m_bDefensivePact(false),
m_bOpenBorders(false),
m_bForcePeace(false),
m_bForceNoTrade(false),
m_bForceWar(false),
m_bAssignCity(false),
m_pbForceCivic(NULL),
m_abVoteSourceTypes(NULL)
{}

CvVoteInfo::~CvVoteInfo()
{
	SAFE_DELETE_ARRAY(m_pbForceCivic);
	SAFE_DELETE_ARRAY(m_abVoteSourceTypes);
}

int CvVoteInfo::getPopulationThreshold() const
{
	return m_iPopulationThreshold;
}

int CvVoteInfo::getStateReligionVotePercent() const
{
	return m_iStateReligionVotePercent;
}

int CvVoteInfo::getTradeRoutes() const
{
	return m_iTradeRoutes;
}

int CvVoteInfo::getMinVoters() const
{
	return m_iMinVoters;
}

bool CvVoteInfo::isSecretaryGeneral() const
{
	return m_bSecretaryGeneral;
}

bool CvVoteInfo::isVictory() const
{
	return m_bVictory;
}

bool CvVoteInfo::isFreeTrade() const
{
	return m_bFreeTrade;
}

bool CvVoteInfo::isNoNukes() const
{
	return m_bNoNukes;
}

bool CvVoteInfo::isCityVoting() const
{
	return m_bCityVoting;
}

bool CvVoteInfo::isCivVoting() const
{
	return m_bCivVoting;
}

bool CvVoteInfo::isDefensivePact() const
{
	return m_bDefensivePact;
}

bool CvVoteInfo::isOpenBorders() const
{
	return m_bOpenBorders;
}

bool CvVoteInfo::isForcePeace() const
{
	return m_bForcePeace;
}

bool CvVoteInfo::isForceNoTrade() const
{
	return m_bForceNoTrade;
}

bool CvVoteInfo::isForceWar() const
{
	return m_bForceWar;
}

bool CvVoteInfo::isAssignCity() const
{
	return m_bAssignCity;
}

bool CvVoteInfo::isForceCivic(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumCivicInfos(), i, "CvVoteInfo::isForceCivic");
	return m_pbForceCivic ? m_pbForceCivic[i] : false;
}

bool CvVoteInfo::isVoteSourceType(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumVoteSourceInfos(), i, "isVoteSourceType");
	return m_abVoteSourceTypes ? m_abVoteSourceTypes[i] : false;
}

bool CvVoteInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvInfoBase::read(pXML))
		return false;

	pXML->GetChildXmlValByName(&m_iPopulationThreshold, "iPopulationThreshold");
	pXML->GetChildXmlValByName(&m_iStateReligionVotePercent, "iStateReligionVotePercent");
	pXML->GetChildXmlValByName(&m_iTradeRoutes, "iTradeRoutes");
	pXML->GetChildXmlValByName(&m_iMinVoters, "iMinVoters");

	pXML->GetChildXmlValByName(&m_bSecretaryGeneral, "bSecretaryGeneral");
	pXML->GetChildXmlValByName(&m_bVictory, "bVictory");
	pXML->GetChildXmlValByName(&m_bFreeTrade, "bFreeTrade");
	pXML->GetChildXmlValByName(&m_bNoNukes, "bNoNukes");
	pXML->GetChildXmlValByName(&m_bCityVoting, "bCityVoting");
	pXML->GetChildXmlValByName(&m_bCivVoting, "bCivVoting");
	pXML->GetChildXmlValByName(&m_bDefensivePact, "bDefensivePact");
	pXML->GetChildXmlValByName(&m_bOpenBorders, "bOpenBorders");
	pXML->GetChildXmlValByName(&m_bForcePeace, "bForcePeace");
	pXML->GetChildXmlValByName(&m_bForceNoTrade, "bForceNoTrade");
	pXML->GetChildXmlValByName(&m_bForceWar, "bForceWar");
	pXML->GetChildXmlValByName(&m_bAssignCity, "bAssignCity");

	pXML->SetVariableListTagPair(&m_pbForceCivic, "ForceCivics", GC.getNumCivicInfos());
	pXML->SetVariableListTagPair(&m_abVoteSourceTypes, "DiploVotes", GC.getNumVoteSourceInfos());

	return true;
}

CvProjectInfo::CvProjectInfo() :
m_iVictoryPrereq(NO_VICTORY),
m_iTechPrereq(NO_TECH),
m_iAnyoneProjectPrereq(NO_PROJECT),
m_iMaxGlobalInstances(0),
m_iMaxTeamInstances(0),
m_iProductionCost(0),
m_iNukeInterception(0),
m_iTechShare(0),
m_iEveryoneSpecialUnit(NO_SPECIALUNIT),
m_iEveryoneSpecialBuilding(NO_SPECIALBUILDING),
m_iVictoryDelayPercent(0),
m_iSuccessRate(0),
m_bSpaceship(false),
m_bAllowsNukes(false),
m_piBonusProductionModifier(NULL),
m_piVictoryThreshold(NULL),
m_piVictoryMinThreshold(NULL),
m_piProjectsNeeded(NULL)
{}

CvProjectInfo::~CvProjectInfo()
{
	SAFE_DELETE_ARRAY(m_piBonusProductionModifier);
	SAFE_DELETE_ARRAY(m_piVictoryThreshold);
	SAFE_DELETE_ARRAY(m_piVictoryMinThreshold);
	SAFE_DELETE_ARRAY(m_piProjectsNeeded);
}

int CvProjectInfo::getVictoryPrereq() const
{
	return m_iVictoryPrereq;
}

int CvProjectInfo::getTechPrereq() const
{
	return m_iTechPrereq;
}

int CvProjectInfo::getAnyoneProjectPrereq() const
{
	return m_iAnyoneProjectPrereq;
}

int CvProjectInfo::getMaxGlobalInstances() const
{
	return m_iMaxGlobalInstances;
}

int CvProjectInfo::getMaxTeamInstances() const
{
	return m_iMaxTeamInstances;
}

int CvProjectInfo::getProductionCost() const
{
	return m_iProductionCost;
}

int CvProjectInfo::getNukeInterception() const
{
	return m_iNukeInterception;
}

int CvProjectInfo::getTechShare() const
{
	return m_iTechShare;
}

int CvProjectInfo::getEveryoneSpecialUnit() const
{
	return m_iEveryoneSpecialUnit;
}

int CvProjectInfo::getEveryoneSpecialBuilding() const
{
	return m_iEveryoneSpecialBuilding;
}

int CvProjectInfo::getVictoryDelayPercent() const
{
	return m_iVictoryDelayPercent;
}

int CvProjectInfo::getSuccessRate() const
{
	return m_iSuccessRate;
}

bool CvProjectInfo::isSpaceship() const
{
	return m_bSpaceship;
}

bool CvProjectInfo::isAllowsNukes() const
{
	return m_bAllowsNukes;
}

const char* CvProjectInfo::getMovieArtDef() const
{
	return m_szMovieArtDef;
}

const TCHAR* CvProjectInfo::getCreateSound() const
{
	return m_szCreateSound;
}

void CvProjectInfo::setCreateSound(const TCHAR* szVal)
{
	m_szCreateSound = szVal;
}

int CvProjectInfo::getBonusProductionModifier(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumBonusInfos(), i, "CvProjectInfo::getBonusProductionModifier");
	return m_piBonusProductionModifier ? m_piBonusProductionModifier[i] : 0; // advc.003t
}

int CvProjectInfo::getVictoryThreshold(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumVictoryInfos(), i, "CvProjectInfo::getVictoryThreshold");
	return m_piVictoryThreshold ? m_piVictoryThreshold[i] : 0; // advc.003t
}

int CvProjectInfo::getVictoryMinThreshold(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumVictoryInfos(), i, "CvProjectInfo::getVictoryMinThreshold");
	// <advc.003t>
	if (m_piVictoryMinThreshold == NULL)
		return 0;
	if (m_piVictoryMinThreshold[i] != 0) // </advc.003t>
		return m_piVictoryMinThreshold[i];
	return getVictoryThreshold(i);
}

int CvProjectInfo::getProjectsNeeded(int i) const
{
	FASSERT_BOUNDS(0, GC.getNumProjectInfos(), i, "CvProjectInfo::getProjectsNeeded");
	return m_piProjectsNeeded ? m_piProjectsNeeded[i] : 0; // advc.003t: was false
}

bool CvProjectInfo::read(CvXMLLoadUtility* pXML)
{
	if (!CvInfoBase::read(pXML))
		return false;

	CvString szTextVal;

	pXML->GetChildXmlValByName(szTextVal, "VictoryPrereq");
	m_iVictoryPrereq = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "TechPrereq");
	m_iTechPrereq = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(&m_iMaxGlobalInstances, "iMaxGlobalInstances");
	pXML->GetChildXmlValByName(&m_iMaxTeamInstances, "iMaxTeamInstances");
	pXML->GetChildXmlValByName(&m_iProductionCost, "iCost");
	pXML->GetChildXmlValByName(&m_iNukeInterception, "iNukeInterception");
	pXML->GetChildXmlValByName(&m_iTechShare, "iTechShare");

	pXML->GetChildXmlValByName(szTextVal, "EveryoneSpecialUnit");
	m_iEveryoneSpecialUnit = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(szTextVal, "EveryoneSpecialBuilding");
	m_iEveryoneSpecialBuilding = pXML->FindInInfoClass(szTextVal);

	pXML->GetChildXmlValByName(&m_bSpaceship, "bSpaceship");
	pXML->GetChildXmlValByName(&m_bAllowsNukes, "bAllowsNukes");
	pXML->GetChildXmlValByName(m_szMovieArtDef, "MovieDefineTag");

	pXML->SetVariableListTagPair(&m_piBonusProductionModifier, "BonusProductionModifiers", GC.getNumBonusInfos());
	pXML->SetVariableListTagPair(&m_piVictoryThreshold, "VictoryThresholds", GC.getNumVictoryInfos());
	pXML->SetVariableListTagPair(&m_piVictoryMinThreshold, "VictoryMinThresholds", GC.getNumVictoryInfos());
	pXML->GetChildXmlValByName(&m_iVictoryDelayPercent, "iVictoryDelayPercent");
	pXML->GetChildXmlValByName(&m_iSuccessRate, "iSuccessRate");

	pXML->GetChildXmlValByName(szTextVal, "CreateSound");
	setCreateSound(szTextVal);

	return true;
}

bool CvProjectInfo::readPass2(CvXMLLoadUtility* pXML)
{
	CvString szTextVal;

	pXML->SetVariableListTagPair(&m_piProjectsNeeded, "PrereqProjects", GC.getNumProjectInfos());

	pXML->GetChildXmlValByName(szTextVal, "AnyonePrereqProject");
	m_iAnyoneProjectPrereq = GC.getInfoTypeForString(szTextVal);

	return true;
}
