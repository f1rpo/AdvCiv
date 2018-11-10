#pragma once

// cityAI.h

#ifndef CIV4_CITY_AI_H
#define CIV4_CITY_AI_H

#include "CvCity.h"

typedef std::vector<std::pair<UnitAITypes, int> > UnitTypeWeightArray;

class CvCityAI : public CvCity
{

public:

	CvCityAI();
	virtual ~CvCityAI();

	void AI_init();
	void AI_uninit();
	void AI_reset();

	void AI_doTurn();

	void AI_assignWorkingPlots();
	void AI_updateAssignWork();

	//bool AI_avoidGrowth(); // disabled by K-Mod
	bool AI_ignoreGrowth();
	//int AI_specialistValue(SpecialistTypes eSpecialist, bool bAvoidGrowth, bool bRemove, bool bIgnoreFood = false) const;
	int AI_specialistValue(SpecialistTypes eSpecialist, bool bRemove, bool bIgnoreFood = false, int iGrowthValue = -1) const; // K-Mod
	int AI_permanentSpecialistValue(SpecialistTypes eSpecialist) const; // K-Mod
	void AI_chooseProduction();

	UnitTypes AI_bestUnit(bool bAsync = false, AdvisorTypes eIgnoreAdvisor = NO_ADVISOR, UnitAITypes* peBestUnitAI = NULL);
	UnitTypes AI_bestUnitAI(UnitAITypes eUnitAI, bool bAsync = false, AdvisorTypes eIgnoreAdvisor = NO_ADVISOR);

	BuildingTypes AI_bestBuilding(int iFocusFlags = 0, int iMaxTurns = 0, bool bAsync = false, AdvisorTypes eIgnoreAdvisor = NO_ADVISOR);
	BuildingTypes AI_bestBuildingThreshold(int iFocusFlags = 0, int iMaxTurns = 0, int iMinThreshold = 0, bool bAsync = false, AdvisorTypes eIgnoreAdvisor = NO_ADVISOR);
	
	/* int AI_buildingValue(BuildingTypes eBuilding, int iFocusFlags = 0) const;
	int AI_buildingValueThreshold(BuildingTypes eBuilding, int iFocusFlags = 0, int iThreshold = 0) const; */
	int AI_buildingValue(BuildingTypes eBuilding, int iFocusFlags = 0, int iThreshold = 0, bool bConstCache = false, bool bAllowRecursion = true) const;

	ProjectTypes AI_bestProject(int* piBestValue = 0,
			bool bAsync = false); // advc.001n
	int AI_projectValue(ProjectTypes eProject);

	// K-Mod note, I've deleted the single-argument version of the following two functions. They were completely superfluous.
	ProcessTypes AI_bestProcess(CommerceTypes eCommerceType = NO_COMMERCE) const;
	int AI_processValue(ProcessTypes eProcess, CommerceTypes eCommerceType = NO_COMMERCE) const;

	int AI_neededSeaWorkers();

	bool AI_isDefended(int iExtra = 0);
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD							9/19/08		jdog5000		    */
/* 																			    */
/* 	Air AI																	    */
/********************************************************************************/
/* original BTS code
	bool AI_isAirDefended(int iExtra = 0);
*/
	bool AI_isAirDefended(bool bCountLand = false, int iExtra = 0);
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								    */
/********************************************************************************/
	bool AI_isDanger();

	int AI_neededDefenders(bool ignoreEvac = false); // advc.139: param added
	int AI_neededAirDefenders();
	int AI_minDefenders();
	// advc.139: Added param
	int AI_neededFloatingDefenders(bool ignoreEvac = false);
	void AI_updateNeededFloatingDefenders();
	// <advc.139>
	void updateSafety(double relativeCityVal);
	bool isEvacuating() const;
	bool isSafe() const;
	// </advc.139>
	int AI_getEmphasizeAvoidGrowthCount() const;
	bool AI_isEmphasizeAvoidGrowth() const;

	int AI_getEmphasizeGreatPeopleCount() const;
	bool AI_isEmphasizeGreatPeople() const;

	bool AI_isAssignWorkDirty() const;
	void AI_setAssignWorkDirty(bool bNewValue);

	bool AI_isChooseProductionDirty() const;
	void AI_setChooseProductionDirty(bool bNewValue);

	CvCity* AI_getRouteToCity() const;
	void AI_updateRouteToCity();

	int AI_getEmphasizeYieldCount(YieldTypes eIndex) const;
	bool AI_isEmphasizeYield(YieldTypes eIndex) const;

	int AI_getEmphasizeCommerceCount(CommerceTypes eIndex) const;
	bool AI_isEmphasizeCommerce(CommerceTypes eIndex) const;

	bool AI_isEmphasize(EmphasizeTypes eIndex) const;
	void AI_setEmphasize(EmphasizeTypes eIndex, bool bNewValue);
	void AI_forceEmphasizeCulture(bool bNewValue);

	int AI_getBestBuildValue(int iIndex);
	int AI_totalBestBuildValue(CvArea* pArea);

	int AI_clearFeatureValue(int iIndex);
	// K-Mod
	// note: some of the following functions existed in BBAI for debugging purposes. But the new K-Mod versions are an integral part of the AI.
	bool AI_isGoodPlot(int iPlot, int* aiYields = 0) const;
	int AI_countGoodPlots() const;
	int AI_countWorkedPoorPlots() const;
	int AI_getTargetPopulation() const;
	void AI_getYieldMultipliers(int &iFoodMultiplier, int &iProductionMultiplier, int &iCommerceMultiplier, int &iDesiredFoodChange) const;
		// advc.003: Made the plot param const
	int AI_getImprovementValue(CvPlot const& kPlot, ImprovementTypes eImprovement, int iFoodPriority, int iProductionPriority, int iCommercePriority, int iDesiredFoodChange, int iClearFeatureValue = 0, bool bEmphasizeIrrigation = false, BuildTypes* peBestBuild = 0) const;
	// K-Mod end
	BuildTypes AI_getBestBuild(int iIndex) const;
	int AI_countBestBuilds(CvArea* pArea) const;
	void AI_updateBestBuild();

	virtual int AI_cityValue() const;

	int AI_calculateWaterWorldPercent();

	int AI_getCityImportance(bool bEconomy, bool bMilitary);

	int AI_calculateMilitaryOutput() const; // K-Mod

	int AI_yieldMultiplier(YieldTypes eYield) const;
	void AI_updateSpecialYieldMultiplier();
	int AI_specialYieldMultiplier(YieldTypes eYield) const;
	int AI_getCultureWeight() const { return m_iCultureWeight; } // K-Mod
	void AI_setCultureWeight(int iWeight) { m_iCultureWeight = iWeight; } // K-Mod

	int AI_countNumBonuses(BonusTypes eBonus, bool bIncludeOurs, bool bIncludeNeutral, int iOtherCultureThreshold, bool bLand = true, bool bWater = true);
	int AI_countNumImprovableBonuses( bool bIncludeNeutral, TechTypes eExtraTech = NO_TECH, bool bLand = true, bool bWater = false ); // BBAI

	int AI_playerCloseness(PlayerTypes eIndex, int iMaxDistance);
	int AI_highestTeamCloseness(TeamTypes eTeam); // K-Mod
	bool AI_isFrontlineCity() const; // K-Mod
	int AI_cityThreat(bool bDangerPercent = false);

	int AI_getWorkersHave();
	int AI_getWorkersNeeded();
	void AI_changeWorkersHave(int iChange);
	BuildingTypes AI_bestAdvancedStartBuilding(int iPass);

	void read(FDataStreamBase* pStream);
	void write(FDataStreamBase* pStream);

	void AI_ClearConstructionValueCache(); // K-Mod
	bool isAwfulSite(PlayerTypes futureOwnerId) const; // advc.122

protected:

	int m_iEmphasizeAvoidGrowthCount;
	int m_iEmphasizeGreatPeopleCount;

	bool m_bAssignWorkDirty;
	bool m_bChooseProductionDirty;

	IDInfo m_routeToCity;

	int* m_aiEmphasizeYieldCount;
	int* m_aiEmphasizeCommerceCount;
	bool m_bForceEmphasizeCulture;

	int m_aiBestBuildValue[NUM_CITY_PLOTS];

	BuildTypes m_aeBestBuild[NUM_CITY_PLOTS];

	bool* m_pbEmphasize;

	int* m_aiSpecialYieldMultiplier;

	int m_iCachePlayerClosenessTurn;
	int m_iCachePlayerClosenessDistance;
	int* m_aiPlayerCloseness;

	int m_iNeededFloatingDefenders;
	int m_iNeededFloatingDefendersCacheTurn;
	// <advc.139>
	// Updated during AI turns; no need for serialization
	bool bEvacuate;
	bool bSafe; // </advc.139>

	int m_iWorkersNeeded;
	int m_iWorkersHave;

	std::vector<int> m_aiConstructionValue; // K-Mod. (cache)
	int m_iCultureWeight; // K-Mod

	void AI_doDraft(bool bForce = false);
	void AI_doHurry(bool bForce = false);
	void AI_doEmphasize();
	int AI_getHappyFromHurry(HurryTypes eHurry) const;
	int AI_getHappyFromHurry(HurryTypes eHurry, UnitTypes eUnit, bool bIgnoreNew) const;
	int AI_getHappyFromHurry(HurryTypes eHurry, BuildingTypes eBuilding, bool bIgnoreNew) const;
	int AI_getHappyFromHurry(int iHurryPopulation) const;
	bool AI_doPanic();
	//int AI_calculateCulturePressure(bool bGreatWork = false) const; // disabled by K-Mod

	bool AI_chooseUnit(UnitAITypes eUnitAI = NO_UNITAI, int iOdds = -1); // bbai added iOdds
	bool AI_chooseUnit(UnitTypes eUnit, UnitAITypes eUnitAI);
	
	bool AI_chooseDefender();
	bool AI_chooseLeastRepresentedUnit(UnitTypeWeightArray &allowedTypes, int iOdds = -1); // bbai added iOdds
	bool AI_chooseBuilding(int iFocusFlags = 0, int iMaxTurns = MAX_INT, int iMinThreshold = 0, int iOdds = -1); // bbai added iOdds.
	bool AI_chooseProject();
	bool AI_chooseProcess(CommerceTypes eCommerceType = NO_COMMERCE);

	bool AI_bestSpreadUnit(bool bMissionary, bool bExecutive, int iBaseChance, UnitTypes* eBestSpreadUnit, int* iBestSpreadUnitValue);
	bool AI_addBestCitizen(bool bWorkers, bool bSpecialists, int* piBestPlot = NULL, SpecialistTypes* peBestSpecialist = NULL);
	bool AI_removeWorstCitizen(SpecialistTypes eIgnoreSpecialist = NO_SPECIALIST);
	void AI_juggleCitizens();

	int AI_citizenSacrificeCost(int iCitLoss, int iHappyLevel = 0, int iNewAnger = 0, int iAngerTimer = 0); // K-Mod

	bool AI_potentialPlot(short* piYields) const;
	bool AI_foodAvailable(int iExtra = 0) const;
	//int AI_yieldValue(short* piYields, short* piCommerceYields, bool bAvoidGrowth, bool bRemove, bool bIgnoreFood = false, bool bIgnoreGrowth = false, bool bIgnoreStarvation = false, bool bWorkerOptimization = false) const;
	//int AI_plotValue(CvPlot* pPlot, bool bAvoidGrowth, bool bRemove, bool bIgnoreFood = false, bool bIgnoreGrowth = false, bool bIgnoreStarvation = false) const;
	// K-Mod. Note: iGrowthValue < 0 means "automatic". It will use AI_growthValuePerFood. iGrowthValue == 0 means "ignore growth".
	int AI_yieldValue(short* piYields, short* piCommerceYields, bool bRemove, bool bIgnoreFood, bool bIgnoreStarvation, bool bWorkerOptimization, int iGrowthValue) const;
	int AI_jobChangeValue(std::pair<bool, int> new_job, std::pair<bool, int> old_job, bool bIgnoreFood, bool bIgnoreStarvation, int iGrowthValue) const; // value gained by swapping jobs. (bIsSpecialist, iIndex) pairs.
		// advc.003: Made plot param const in these two functions
	int AI_plotValue(CvPlot const* pPlot, bool bRemove, bool bIgnoreFood, bool bIgnoreStarvation, int iGrowthValue) const;
	bool AI_finalImprovementYieldDifference(CvPlot const* pPlot, short* piYields) const; // difference between current yields and yields after plot improvement reaches final upgrade.
	bool AI_timeWeightedImprovementYields(CvPlot const* pPlot, ImprovementTypes eImprovement, int time_scale, std::vector<float>& weighted_yields) const; // time-weighted yields for improvements which have upgrades
	int AI_specialPlotImprovementValue(CvPlot* pPlot) const; // value for working a plot in addition to its yields
	int AI_growthValuePerFood() const;
	// K-mod end

	int AI_experienceWeight();
	int AI_buildUnitProb(bool draft = false); // advc.017: param added

	void AI_bestPlotBuild(CvPlot* pPlot, int* piBestValue, BuildTypes* peBestBuild, int iFoodPriority, int iProductionPriority, int iCommercePriority, bool bChop, int iHappyAdjust, int iHealthAdjust, int iDesiredFoodChange);

	void AI_buildGovernorChooseProduction();
	void AI_barbChooseProduction(); // K-Mod

	int AI_getYieldMagicValue(const int* piYieldsTimes100, bool bHealthy) const;
			// advc.003: Made plot param const
	int AI_getPlotMagicValue(CvPlot const& kPlot, bool bHealthy, bool bWorkerOptimization = false) const;
	int AI_countGoodTiles(bool bHealthy, bool bUnworkedOnly, int iThreshold = 50, bool bWorkerOptimization = false) const;
	int AI_countGoodSpecialists(bool bHealthy) const;
	//int AI_calculateTargetCulturePerTurn() const; // disabled by K-Mod

	void AI_stealPlots();

	//int AI_buildingSpecialYieldChangeValue(BuildingTypes kBuilding, YieldTypes eYield) const;
	int AI_buildingSeaYieldChangeWeight(BuildingTypes eBuilding, bool bGrowing = true) const; // K-Mod

	void AI_cachePlayerCloseness(int iMaxDistance);
	void AI_updateWorkersNeededHere();
	// advc.179:
	double estimateReligionBuildings(PlayerTypes civId, ReligionTypes eReligion,
			std::vector<BuildingTypes> const& buildings) const;
	// added so under cheat mode we can call protected functions for testing
	friend class CvGameTextMgr;
};

#endif
