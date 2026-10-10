#pragma once

#ifndef CIV4_CITY_AI_H
#define CIV4_CITY_AI_H

#include "CvCity.h"
#include "AIStrategies.h" // advc.enum

class CvCityAI : public CvCity
{
public:

	CvCityAI();
	~CvCityAI();
	// advc.003u: Override replacing AI_init. Parameter list copied from CvCity::init.
	void init(int iID, PlayerTypes eOwner, int iX, int iY, bool bBumpUnits, bool bUpdatePlotGroups, int iOccupationTimer = 0); // advc.ctr

	void AI_doTurn();
	void AI_assignWorkingPlots(/* advc.131d: */ bool bEmphasize = false);
	void AI_updateAssignWork();

	//bool AI_avoidGrowth(); // disabled by K-Mod
	//bool AI_ignoreGrowth(); // advc.003j: K-Mod's replacement is also no longer used
	//int AI_specialistValue(SpecialistTypes eSpecialist, bool bAvoidGrowth, bool bRemove, bool bIgnoreFood = false) const;
	int AI_specialistValue(SpecialistTypes eSpecialist, bool bRemove, bool bIgnoreFood = false, int iGrowthValue = -1) const; // K-Mod
	// <!-- custom: Optional component outputs let log-gated Great Person weight diagnostics explain a winning Join City value without duplicating this valuation. (GPT-5.5 + ChatGPT-5.5) -->
	int AI_permanentSpecialistValue(SpecialistTypes eSpecialist, int* piYieldValue = NULL, int* paiCommerceValues = NULL) const; // K-Mod
	void AI_chooseProduction();
	bool SAS_AI_findBestFallbackUnit(UnitTypes& ePickUnit, UnitAITypes& ePickUnitAI, bool bOffenseDefaultUnitAIsOnly, bool bDefenseDefaultUnitAIsOnly, int iMaxCost, bool bAllowSiege, bool bAllowTrebuchetsLike, int iCapNonTrebuchetsLikeSiegesAll, int iCapTrebsLike, int iSiegesAllNonTrebuchetsLike, int iSiegesAllTrebuchetsLike, UnitTypes eSkipUnit, bool bAllowOverallFallback, bool bAllowCheapestFallback) const;

	UnitTypes AI_bestUnit(bool bAsync = false, AdvisorTypes eIgnoreAdvisor = NO_ADVISOR, UnitAITypes* peBestUnitAI = NULL) const;
	// <!-- custom: Optionally return the same-pass ranked concrete candidates so a KI#53 siege veto can try the next unit without rerunning randomized valuation. See KI#53.3. (GPT-5.6-Sol) -->
	UnitTypes AI_bestUnitAI(UnitAITypes eUnitAI, bool bAsync = false, AdvisorTypes eIgnoreAdvisor = NO_ADVISOR, std::vector<UnitTypes>* paeRankedUnits = NULL) const;

	BuildingTypes AI_bestBuilding(int iFocusFlags = 0, int iMaxTurns = 0, bool bAsync = false, AdvisorTypes eIgnoreAdvisor = NO_ADVISOR) const;
	// <!-- custom: bRandomize preserves the inherited random wonder bonus and final 100-124% building-value multiplier by default.
	// Pass false only for deterministic diagnostics/behavioral gates that must not consume ASyncRand or synchronized game RNG.
	// The optional output returns the winning same-pass post-turn selection score so a pre-gated realized-choice recorder does not rerun randomized valuation. See KI#197.13. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	BuildingTypes AI_bestBuildingThreshold(int iFocusFlags = 0, int iMaxTurns = 0, int iMinThreshold = 0, bool bAsync = false, AdvisorTypes eIgnoreAdvisor = NO_ADVISOR, bool bRandomize = true, int* piBestValue = NULL) const;

	/* int AI_buildingValue(BuildingTypes eBuilding, int iFocusFlags = 0) const;
	int AI_buildingValueThreshold(BuildingTypes eBuilding, int iFocusFlags = 0, int iThreshold = 0) const; */
	// advc.121b
	// <!-- custom: hoisted from multiline signature between `bIgnoreSpecialists` and `bObsolete` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	// <!-- custom: eAssumeTech distinguishes current-state valuation from AI_techBuildingValue's candidate technology and guaranteed-prerequisite context. See KI#48.14. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	int AI_buildingValue(BuildingTypes eBuilding, int iFocusFlags = 0, int iThreshold = 0, bool bConstCache = false, bool bAllowRecursion = true, bool bIgnoreSpecialists = false, bool bObsolete = false, TechTypes eAssumeTech = NO_TECH) const; // advc.004c
	// <advc.179>
	scaled AI_estimateReligionBuildings(PlayerTypes ePlayer, ReligionTypes eReligion, std::vector<BuildingTypes> const& aeBuildings) const; // </advc.179>
	ProjectTypes AI_bestProject(int* piBestValue = 0, /* advc.001n: */ bool bAsync = false) const;
	int AI_projectValue(ProjectTypes eProject) const;

	/*  K-Mod note, I've deleted the single-argument version of the following two functions.
		They were completely superfluous. */
	ProcessTypes AI_bestProcess(CommerceTypes eCommerceType = NO_COMMERCE) const;
	int AI_processValue(ProcessTypes eProcess, CommerceTypes eCommerceType = NO_COMMERCE) const;

	int AI_neededSeaWorkers() const;

	// <advc.110>
	bool AI_isDefended(int iExtra = 0) const { return (AI_countExcessDefenders() + iExtra >= 0); }
	int AI_countExcessDefenders() const; // </advc.110>
	//bool AI_isAirDefended(int iExtra = 0);
	// BETTER_BTS_AI_MOD, Air AI, 9/19/08, jdog5000:
	bool AI_isAirDefended(bool bCountLand = false, int iExtra = 0) const;
	bool AI_isDanger() const;
	int AI_neededDefenders(/* advc.139: */ bool bIgnoreEvac = false, bool bConstCache = false) const; // advc.001n
	int AI_neededAirDefenders(/* advc.001n: */ bool bConstCache = false) const;
	int AI_minDefenders() const;
	int AI_neededFloatingDefenders(/* advc.139: */ bool bIgnoreEvac = false, /* <advc.001n> */ bool bConstCache = false, bool bIgnoreCulture = false) const; // advc.099c
	// was void AI_updateNeededFloatingDefenders()
	// </advc.001n>
	// <!-- custom: hoisted from multiline signature between `bConstCache` and `bIgnoreCulture` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	int AI_calculateNeededFloatingDefenders(bool bConstCache, /* <advc.099c> */ bool bIgnoreCulture = false) const;
	int AI_neededCultureDefenders() const; // </advc.099c>
	// <advc.139>
	void AI_updateSafety(bool bUpdatePerfectSafety = true);
	CitySafetyTypes AI_getSafety() const { return m_eSafety; }
	bool AI_isEvacuating() const { return (m_eSafety == CITYSAFETY_EVACUATING); }
	bool AI_isSafe() const { return (m_eSafety >= CITYSAFETY_SAFE); }
	void AI_setCityValPercent(int iValue);
	int AI_getCityValPercent() const { return m_iCityValPercent; }
	// </advc.139>

	// advc: Moved from CvCity b/c it's part of the AI
	int AI_culturePressureFactor() const; // K-Mod
	// <!-- custom: Return the direct Artist culture percent for clean, secure, non-cultural-victory AI cities while preserving full value for local culture emergencies. (ChatGPT-5.5 + GPT-5.5 review) -->
	int SAS_AI_directArtistCultureValuePercent() const;
	int AI_getEmphasizeAvoidGrowthCount() const { return m_iEmphasizeAvoidGrowthCount; }
	bool AI_isEmphasizeAvoidGrowth() const { return (AI_getEmphasizeAvoidGrowthCount() > 0); }

	int AI_getEmphasizeGreatPeopleCount() const { return m_iEmphasizeGreatPeopleCount; }
	bool AI_isEmphasizeGreatPeople() const { return (AI_getEmphasizeGreatPeopleCount() > 0); }

	bool AI_isAssignWorkDirty() const { return m_bAssignWorkDirty; }
	void AI_setAssignWorkDirty(bool bNewValue) { m_bAssignWorkDirty = bNewValue; }

	//bool AI_isChooseProductionDirty() const; // advc.003u: Moved to CvCity
	//void AI_setChooseProductionDirty(bool bNewValue);

	CvCityAI* AI_getRouteToCity() const;
	void AI_updateRouteToCity();

	int AI_getEmphasizeYieldCount(YieldTypes eIndex) const;
	bool AI_isEmphasizeYield(YieldTypes eIndex) const;

	int AI_getEmphasizeCommerceCount(CommerceTypes eIndex) const;
	bool AI_isEmphasizeCommerce(CommerceTypes eIndex) const;

	bool AI_isEmphasize(EmphasizeTypes eIndex) const;
	void AI_setEmphasize(EmphasizeTypes eIndex, bool bNewValue);
	// <advc.131d>
	bool AI_isStrongEmphasis() const { return m_bStrongEmphasis; }
	void AI_setStrongEmphasis(bool bStrongEmphasis); // </advc.131d>
	//void AI_forceEmphasizeCulture(bool bNewValue); // advc.003j

	int AI_getBestBuildValue(/* advc.enum: */ CityPlotTypes ePlot) const { FAssertEnumBounds(ePlot); return m_aiBestBuildValue[ePlot]; }
	int AI_totalBestBuildValue(CvArea const& kArea) const;

	int AI_clearFeatureValue(CityPlotTypes ePlot) const;
	// K-Mod
	// note: some of the following functions existed in BBAI for debugging purposes. But the new K-Mod versions are an integral part of the AI.
	bool AI_isGoodPlot(CityPlotTypes ePlot, int* aiYields = 0) const;
	int AI_countGoodPlots() const;
	int AI_countWorkedPoorPlots() const;
	int AI_getTargetPopulation() const;
	void AI_getYieldMultipliers(int &iFoodMultiplier, int &iProductionMultiplier, int &iCommerceMultiplier, int &iDesiredFoodChange) const;
	int AI_getImprovementValue(CvPlot const& kPlot, ImprovementTypes eImprovement, int iFoodPriority, int iProductionPriority, int iCommercePriority, int iDesiredFoodChange, int iClearFeatureValue = 0, bool bEmphasizeIrrigation = false, BuildTypes* peBestBuild = 0) const;
	// K-Mod end
	BuildTypes AI_getBestBuild(CityPlotTypes ePlot) const;
	int AI_countBestBuilds(CvArea const& kArea) const;
	void AI_updateBestBuild();
	int AI_countOvergrownBonuses(FeatureTypes eFeature) const; // advc.129

	int AI_splitEmpireValue() const; // advc.ctr: Renamed from AI_cityValue
	// <!-- custom: inherited sea-throughput valuation: logically read-only; const lets building valuation reuse the existing water-world demand estimate directly for production and free-experience context. See KI#48.11 and KI#48.12. (ChatGPT-5.6-Sol) -->
	int AI_calculateWaterWorldPercent() const;
	int AI_getCityImportance(bool bEconomy, bool bMilitary);

	int AI_yieldMultiplier(YieldTypes eYield) const;
	void AI_updateSpecialYieldMultiplier();
	int AI_specialYieldMultiplier(YieldTypes eYield) const;
	int AI_commerceToProductionMultiplierShift() const; // advc
	int AI_getCultureWeight() const { return m_iCultureWeight; } // K-Mod
	void AI_setCultureWeight(int iWeight) { m_iCultureWeight = iWeight; } // K-Mod
	// <!-- custom: BEGIN transient culture-victory city-state accessors for empire ranking and local process evaluation. (GPT-5.5) -->
	int AI_getCultureVictoryRank() const { return m_iCultureVictoryRank; }
	void AI_setCultureVictoryRank(int iRank) { m_iCultureVictoryRank = iRank; }
	int AI_getCultureVictoryInvestmentPercent() const { return m_iCultureVictoryInvestmentPercent; }
	void AI_setCultureVictoryInvestmentPercent(int iPercent) { m_iCultureVictoryInvestmentPercent = iPercent; }
	// <!-- custom: END transient culture-victory city-state accessors. (GPT-5.5) -->
	bool AI_needsCultureToWorkFullRadius() const; // advc

	int AI_countNumBonuses(BonusTypes eBonus, bool bIncludeOurs, bool bIncludeNeutral, int iOtherCultureThreshold, bool bLand = true, bool bWater = true) const;
	int AI_countNumImprovableBonuses(bool bIncludeNeutral, TechTypes eExtraTech = NO_TECH, bool bLand = true, bool bWater = false) const; // BBAI

	int AI_playerCloseness(PlayerTypes eIndex, int iMaxDistance /* advc: */ = DEFAULT_PLAYER_CLOSENESS, bool bConstCache = false) const; // advc.001n
	// K-Mod
	// <!-- custom: hoisted from multiline signature between `eTeam` and `bConstCache` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	int AI_highestTeamCloseness(TeamTypes eTeam, bool bConstCache) const; // advc.001n
	//bool AI_isFrontlineCity() const; // K-Mod // advc.003j: unused
	int AI_calculateMilitaryOutput() const; // K-Mod
	int AI_cityThreat(/*bool bDangerPercent = false*/) const; // advc: param was (and has always been) unused
	int AI_defianceAngerCost(ReligionTypes eVSReligion) const; // advc.118b

	int AI_getWorkersHave() const;
	int AI_getWorkersNeeded() const;
	void AI_changeWorkersHave(int iChange);
	BuildingTypes AI_bestAdvancedStartBuilding(int iPass) const;

	void AI_ClearConstructionValueCache(); // K-Mod

	void read(FDataStreamBase* pStream);
	void write(FDataStreamBase* pStream);

	static CvCityAI* fromIDInfo(IDInfo id); // advc

protected:

	int m_iCultureWeight; // K-Mod
	// <!-- custom: BEGIN transient culture-victory city state. (GPT-5.5) -->
	// <!-- custom: Transient projected culture-victory rank, refreshed with commerce weights and intentionally not serialized.
	// It lets local process evaluation distinguish required Legendary candidates from unrelated cities without repeating the empire-wide ranking pass. (GPT-5.5) -->
	int m_iCultureVictoryRank;
	// <!-- custom: Transient victory-specific culture investment percentage after candidate balancing, also intentionally not serialized.
	// Zero prevents a far-ahead candidate from using Culture process while preserving independent base/pressure culture value. (GPT-5.5) -->
	int m_iCultureVictoryInvestmentPercent;
	// <!-- custom: END transient culture-victory city state. (GPT-5.5) -->
	int m_iEmphasizeAvoidGrowthCount;
	int m_iEmphasizeGreatPeopleCount;
	int m_iWorkersNeeded;
	int m_iWorkersHave;
	int m_iCityValPercent; // advc.139

	bool m_bAssignWorkDirty;
	CitySafetyTypes m_eSafety; // advc.139

	IDInfo m_routeToCity;

	int* m_aiEmphasizeYieldCount;
	int* m_aiEmphasizeCommerceCount;
	bool m_bForceEmphasizeCulture; // advc.003j (comment): unused
	bool m_bStrongEmphasis; // advc.131d
	bool* m_pbEmphasize;

	BuildTypes* m_aeBestBuild;
	BuildTypes m_eBestBuild; // advc.opt

	int* m_aiSpecialYieldMultiplier;
	int* m_aiBestBuildValue;
	int* m_aiPlayerCloseness;
	// <advc> Made mutable (and made the cache accessor functions const)
	mutable int* m_aiCachePlayerClosenessTurn;
	mutable int* m_aiCachePlayerClosenessDistance;
	mutable int m_iNeededFloatingDefenders;
	mutable int m_iNeededFloatingDefendersCacheTurn;
	mutable std::vector<int> m_aiConstructionValue; // K-Mod. (cache)
	// </advc>

	void AI_doDraft(bool bForce = false);
	void AI_doHurry(bool bForce = false);
	void AI_doEmphasize();
	int AI_getHappyFromHurry(HurryTypes eHurry) const;
	int AI_getHappyFromHurry(HurryTypes eHurry, UnitTypes eUnit, bool bIgnoreNew) const;
	int AI_getHappyFromHurry(HurryTypes eHurry, BuildingTypes eBuilding, bool bIgnoreNew) const;
	int AI_getHappyFromHurry(int iHurryPopulation) const;
	void AI_doPanic();
	//int AI_calculateCulturePressure(bool bGreatWork = false) const; // disabled by K-Mod

	bool AI_bestSpreadUnit(bool bMissionary, bool bExecutive, int iBaseChance, UnitTypes* eBestSpreadUnit, int* iBestSpreadUnitValue);
	bool AI_chooseUnit(UnitAITypes eUnitAI = NO_UNITAI, int iOdds = -1); // BETTER_BTS_AI_MOD, 01/09/10, jdog5000: City AI
	// <!-- custom: The optional result distinguishes a KI#53 siege veto from other terminal concrete-unit failures, allowing only the former to try another unit of the same role. See KI#53.3. (GPT-5.6-Sol) -->
	bool AI_chooseUnit(UnitTypes eUnit, UnitAITypes eUnitAI, bool* pbRetrySameRole = NULL);
	bool AI_chooseDefender();
	bool AI_chooseLeastRepresentedUnit(UnitAIWeightMap const& kWeights, int iOdds = -1); // BBAI
	bool AI_chooseBuilding(int iFocusFlags = 0, int iMaxTurns = MAX_INT, int iMinThreshold = 0, int iOdds = -1); // BBAI
	//bool AI_chooseProject(); // advc.003j
	bool AI_chooseProcess(CommerceTypes eCommerceType = NO_COMMERCE);

	bool AI_addBestCitizen(bool bWorkers, bool bSpecialists, CityPlotTypes* peBestPlot = NULL, SpecialistTypes* peBestSpecialist = NULL);
	bool AI_removeWorstCitizen(SpecialistTypes eIgnoreSpecialist = NO_SPECIALIST);
	void AI_juggleCitizens(/* advc.131d: */ bool bEmphasize = false);
	int AI_citizenSacrificeCost(int iCitLoss, int iHappyLevel = 0, int iNewAnger = 0, int iAngerTimer = 0); // K-Mod
	int AI_citizenValue() const; // advc
	// advc: Both unused
	/*bool AI_potentialPlot(CvPlot const& kPlot) const; // advc.enum: param was 'short* piYields'
	bool AI_foodAvailable(int iExtra = 0) const;*/
	//int AI_yieldValue(short* piYields, short* piCommerceYields, bool bAvoidGrowth, bool bRemove, bool bIgnoreFood = false, bool bIgnoreGrowth = false, bool bIgnoreStarvation = false, bool bWorkerOptimization = false) const;
	//int AI_plotValue(CvPlot* pPlot, bool bAvoidGrowth, bool bRemove, bool bIgnoreFood = false, bool bIgnoreGrowth = false, bool bIgnoreStarvation = false) const;
	/*	K-Mod. Note: iGrowthValue < 0 means "automatic".
		It will use AI_growthValuePerFood. iGrowthValue == 0 means "ignore growth". */
	int AI_yieldValue(int* piYields, int* piCommerceYields, bool bRemove, bool bIgnoreFood, bool bIgnoreStarvation, bool bWorkerOptimization, int iGrowthValue) const;
	// value gained by swapping jobs. (bIsSpecialist, iIndex) pairs.
	int AI_jobChangeValue(std::pair<bool, int> new_job, std::pair<bool, int> old_job, bool bIgnoreFood, bool bIgnoreStarvation, int iGrowthValue) const;
	int AI_plotValue(CvPlot const& pPlot, bool bRemove, bool bIgnoreFood, bool bIgnoreStarvation, int iGrowthValue) const;
	// difference between current yields and yields after plot improvement reaches final upgrade.
	bool AI_finalImprovementYieldDifference(CvPlot const& kPlot, int* piYields) const;
	// time-weighted yields for improvements which have upgrades
	bool AI_timeWeightedImprovementYields(CvPlot const& kPlot, ImprovementTypes eImprovement, int iTimeScale, EagerEnumMap<YieldTypes, scaled>& kWeightedYields) const;
	// value for working a plot in addition to its yields
	int AI_specialPlotImprovementValue(CvPlot const& kPlot) const;
	int AI_growthValuePerFood() const;
	// K-Mod end  // <advc.901>
	int AI_healthHappyImprovementValue(CvPlot const& kPlot, ImprovementTypes eImprovement, ImprovementTypes eFinalImprovement, bool bIgnoreFeature, bool bIgnoreOtherCities = true) const; // </advc.901>
	// <advc>
	int AI_defensiveBuildingValue(BuildingTypes eBuilding, bool bAreaAlone, bool bWarPlan, int iNumCities, int iNumCitiesInArea, bool bRemove, /* advc.004c: */ bool bObsolete) const; // </advc>

	// <!-- custom: logically read-only helpers made const so const building valuation can reuse the authoritative dynamic unit-production-demand estimate without a proxy or const_cast. See KI#48.10. (ChatGPT-5.6-Sol) -->
	int AI_experienceWeight() const;
	int AI_buildUnitProb(/* advc.017: */ bool bDraft = false) const;
	bool AI_emphasizeIrrigatingPlot(CvPlot const& kPlot) const; // advc
	void AI_bestPlotBuild(CvPlot const& kPlot, int* piBestValue, BuildTypes* peBestBuild, int iFoodPriority, int iProductionPriority, int iCommercePriority, bool bChop, int iHappyAdjust, int iHealthAdjust, int iDesiredFoodChange) const;

	void AI_buildGovernorChooseProduction();
	void AI_barbChooseProduction(); // K-Mod

	int AI_getYieldMagicValue(const int* piYieldsTimes100, bool bHealthy) const;
	int AI_getPlotMagicValue(CvPlot const& kPlot, bool bHealthy, bool bWorkerOptimization = false) const;
	int AI_countGoodTiles(bool bHealthy, bool bUnworkedOnly, int iThreshold = 50, bool bWorkerOptimization = false) const;
	int AI_countGoodSpecialists(bool bHealthy) const;
	//int AI_calculateTargetCulturePerTurn() const; // disabled by K-Mod
	int AI_nukeEplosionValue() const; // advc.650

	void AI_stealPlots();

	//int AI_buildingSpecialYieldChangeValue(BuildingTypes kBuilding, YieldTypes eYield) const;
	int AI_buildingSeaYieldChangeWeight(BuildingTypes eBuilding, bool bGrowing = true) const; // K-Mod
	// advc.opt: was void AI_cachePlayerCloseness(int)
	int AI_calculatePlayerCloseness(int iMaxDistance, PlayerTypes ePlayer, bool bConstCache) const; // advc.001n
	// <advc.031b>
	int AI_calculateSettlerPriority(int iAreaSites, int iBestAreaFoundValue, int iWaterAreaSites, int iBestWaterAreaFoundValue) const; // </advc.031b>
	void AI_updateWorkersHaveAndNeeded(); // advc.113b: Renamed from AI_updateWorkersNeededHere

	// added so under cheat mode we can call protected functions for testing
	friend class CvGameTextMgr;
	friend class CvCity; // advc.003u: So that protected functions can be called through CvCity::AI
};

#endif
