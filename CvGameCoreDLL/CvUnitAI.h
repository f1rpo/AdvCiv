#pragma once

// unitAI.h

#ifndef CIV4_UNIT_AI_H
#define CIV4_UNIT_AI_H

#include "CvUnit.h"
#include <utility> // advc.033

class CvCity;

class CvUnitAI : public CvUnit
{

public:

	CvUnitAI();
	virtual ~CvUnitAI();

	void AI_init(UnitAITypes eUnitAI);
	void AI_uninit();
	void AI_reset(UnitAITypes eUnitAI = NO_UNITAI);

	bool AI_update();
	//bool AI_follow();
	bool AI_follow(bool bFirst = true); // K-Mod
	bool AI_load(UnitAITypes eUnitAI, MissionAITypes eMissionAI, UnitAITypes eTransportedUnitAI = NO_UNITAI, int iMinCargo = -1, int iMinCargoSpace = -1, int iMaxCargoSpace = -1, int iMaxCargoOurUnitAI = -1, int iFlags = 0, int iMaxPath = MAX_INT, int iMaxTransportPath = MAX_INT); // bbai / bts

	void AI_upgrade();

	void AI_promote();

	int AI_groupFirstVal();
	int AI_groupSecondVal();

	int AI_attackOdds(const CvPlot* pPlot, bool bPotentialEnemy) const;

	bool AI_bestCityBuild(CvCity* pCity, CvPlot** ppBestPlot = NULL, BuildTypes* peBestBuild = NULL, CvPlot* pIgnorePlot = NULL, CvUnit* pUnit = NULL);

	bool AI_isCityAIType() const;

	inline int AI_getBirthmark() const { return m_iBirthmark; }
	void AI_setBirthmark(int iNewValue);

	UnitAITypes AI_getUnitAIType() const;
	void AI_setUnitAIType(UnitAITypes eNewValue);

	int AI_sacrificeValue(const CvPlot* pPlot) const;

	void read(FDataStreamBase* pStream);
	void write(FDataStreamBase* pStream);

protected:

	int m_iBirthmark;

	UnitAITypes m_eUnitAIType;

	int m_iAutomatedAbortTurn;
	int m_iSearchRangeRandPercent; // advc.128
	/* advc.117, advc.121: Need this value repeatedly within a Worker move and
	   it might be costly to recompute. No need for serialization b/c it all
	   happens within the same AI move. */
	int m_iNeededWorkers;

	bool AI_considerDOW(CvPlot* pPlot); // K-Mod
	bool AI_considerPathDOW(CvPlot* pPlot, int iFlags); // K-Mod
	// K-Mod
	CvUnit* AI_findTransport(UnitAITypes eUnitAI, int iFlags = 0,
			int iMaxPath = MAX_INT, UnitAITypes eTransportedUnitAI = NO_UNITAI,
			int iMinCargo = -1, int iMinCargoSpace = -1, int iMaxCargoSpace = -1,
			int iMaxCargoOurUnitAI = -1);
	// K-Mod end
	void AI_animalMove();
	void AI_settleMove();
	void AI_workerMove();
	void AI_barbAttackMove();
	void AI_attackMove();
	void AI_attackCityMove();
	void AI_attackCityLemmingMove();
	void AI_collateralMove();
	void AI_pillageMove();
	void AI_reserveMove();
	void AI_counterMove();
	void AI_paratrooperMove();
	void AI_cityDefenseMove();
	void AI_cityDefenseExtraMove();
	void AI_exploreMove();
	void AI_missionaryMove();
	void AI_prophetMove();
	void AI_artistMove();
	void AI_scientistMove();
	void AI_generalMove();
	void AI_merchantMove();
	void AI_engineerMove();
	void AI_greatSpyMove(); // K-Mod
	bool AI_greatPersonMove(); // K-Mod
	void AI_spyMove();
	void AI_ICBMMove();
	void AI_workerSeaMove();
	void AI_barbAttackSeaMove();
	void AI_pirateSeaMove();
	void AI_attackSeaMove();
	void AI_reserveSeaMove();
	void AI_escortSeaMove();
	void AI_exploreSeaMove();
	void AI_assaultSeaMove();
	void AI_settlerSeaMove();
	void AI_missionarySeaMove();
	void AI_spySeaMove();
	void AI_carrierSeaMove();
	void AI_missileCarrierSeaMove();
	void AI_attackAirMove();
	void AI_defenseAirMove();
	void AI_carrierAirMove();
	void AI_missileAirMove();

	void AI_networkAutomated();
	void AI_cityAutomated();

	int AI_promotionValue(PromotionTypes ePromotion);

	bool AI_shadow(UnitAITypes eUnitAI, int iMax = -1, int iMaxRatio = -1, bool bWithCargoOnly = true,
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/01/10                                jdog5000      */
/* Unit AI                                                                                      */
/************************************************************************************************/
			bool bOutsideCityOnly = false, int iMaxPath = MAX_INT);
	// K-Mod. I've created AI_omniGroup with the intention of using it to phase out AI_group and AI_groupMergeRange.
	bool AI_omniGroup(UnitAITypes eUnitAI, int iMaxGroup = -1, int iMaxOwnUnitAI = -1, bool bStackOfDoom = false, int iFlags = 0, int iMaxPath = -1, bool bMergeGroups = true, bool bSafeOnly = true, bool bIgnoreFaster = false, bool bIgnoreOwnUnitType = false, bool bBiggerOnly = true, int iMinUnitAI = -1, bool bWithCargoOnly = false, bool bIgnoreBusyTransports = false);
	bool AI_group(UnitAITypes eUnitAI, int iMaxGroup = -1, int iMaxOwnUnitAI = -1, int iMinUnitAI = -1, bool bIgnoreFaster = false, bool bIgnoreOwnUnitType = false, bool bStackOfDoom = false, int iMaxPath = MAX_INT, bool bAllowRegrouping = false, bool bWithCargoOnly = false, bool bInCityOnly = false, MissionAITypes eIgnoreMissionAIType = NO_MISSIONAI);
	//bool AI_load(UnitAITypes eUnitAI, MissionAITypes eMissionAI, UnitAITypes eTransportedUnitAI = NO_UNITAI, int iMinCargo = -1, int iMinCargoSpace = -1, int iMaxCargoSpace = -1, int iMaxCargoOurUnitAI = -1, int iFlags = 0, int iMaxPath = MAX_INT, int iMaxTransportPath = MAX_INT);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	bool AI_guardCityBestDefender();
	bool AI_guardCityOnlyDefender(); // K-Mod
	bool AI_guardCityMinDefender(bool bSearch = true);
	bool AI_guardCity(bool bLeave = false, bool bSearch = false, int iMaxPath = MAX_INT, int iFlags = 0);
	bool AI_guardCityAirlift();
	bool AI_guardCoast(bool bPrimaryOnly = false, int iFlags = 0, int iMaxPath = -1); // K-Mod
	bool AI_guardBonus(int iMinValue = 0);
	bool AI_guardYield(); // advc.300
	bool AI_barbAmphibiousCapture(); // advc.306
	int AI_getPlotDefendersNeeded(CvPlot* pPlot, int iExtra);
	bool AI_guardFort(bool bSearch = true);
	bool AI_guardCitySite();
	bool AI_guardSpy(int iRandomPercent);
	bool AI_destroySpy();
	bool AI_sabotageSpy();
	bool AI_pickupTargetSpy();
	bool AI_chokeDefend();
	bool AI_heal(int iDamagePercent = 0, int iMaxPath = MAX_INT);
	bool AI_isThreatenedFromLand() const; // advc.139
	bool AI_afterAttack();
	bool AI_goldenAge();
	bool AI_spreadReligion();
	bool AI_spreadCorporation();
	bool AI_spreadReligionAirlift();
	bool AI_spreadCorporationAirlift();
	bool AI_discover(bool bThisTurnOnly = false, bool bFirstResearchOnly = false);
	bool AI_lead(std::vector<UnitAITypes>& aeAIUnitTypes);
	bool AI_join(int iMaxCount = MAX_INT);
	bool AI_construct(int iMaxCount = MAX_INT, int iMaxSingleBuildingCount = MAX_INT, int iThreshold = 15);
	/*bool AI_switchHurry(); // advc.003j
	bool AI_hurry();*/
	//bool AI_greatWork(); // disabled by K-Mod
	bool AI_offensiveAirlift();
	bool AI_paradrop(int iRange);
	#if 0 // advc.003: unused
	bool AI_protect(int iOddsThreshold, int iFlags = 0, int iMaxPathTurns = MAX_INT);
	#endif
	bool AI_patrol();
	bool AI_defend();
	bool AI_safety();
	bool AI_hide();
	bool AI_goody(int iRange);
	bool AI_explore();
	bool AI_exploreRange(int iRange);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/29/10                                jdog5000      */
/* War tactics AI                                                                               */
/************************************************************************************************/
	CvCity* AI_pickTargetCity(int iFlags = 0, int iMaxPathTurns = MAX_INT, bool bHuntBarbs = false);
	bool AI_goToTargetCity(int iFlags = 0, int iMaxPathTurns = MAX_INT, CvCity* pTargetCity = NULL);
	//bool AI_goToTargetBarbCity(int iMaxPathTurns = 10); // disabled by K-Mod. (duplicate code ftl)
	bool AI_pillageAroundCity(CvCity* pTargetCity, int iBonusValueThreshold = 0, int iFlags = 0, int iMaxPathTurns = MAX_INT);
	bool AI_bombardCity();
	// advc.003: iFlags for cityAttack is no longer used (not since K-Mod 1.15)
	bool AI_cityAttack(int iRange, int iOddsThreshold, int iFlags = 0, bool bFollow = false);
	bool AI_anyAttack(int iRange, int iOddsThreshold, int iFlags = 0, int iMinStack = 0, bool bAllowCities = true, bool bFollow = false);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	bool AI_rangeAttack(int iRange);
	bool AI_leaveAttack(int iRange, int iThreshold, int iStrengthThreshold);
	bool AI_defensiveCollateral(int iThreshold, int iSearchRange); // K-Mod
	bool AI_evacuateCity(); // advc.139
	bool AI_defendTerritory(int iThreshold, int iFlags, int iMaxPathTurns, bool bLocal = false); // K-Mod
	bool AI_stackVsStack(int iSearchRange, int iAttackThreshold, int iRiskThreshold, int iFlags); // K-Mod
	bool AI_blockade();
	bool AI_pirateBlockade();
	bool AI_seaBombardRange(int iMaxRange);
	bool AI_pillage(int iBonusValueThreshold = 0, int iFlags = 0);
	bool AI_pillageRange(int iRange, int iBonusValueThreshold = 0, int iFlags = 0);
	bool AI_found(int iFlags = MOVE_NO_ENEMY_TERRITORY); // K-Mod added iFlags
	//bool AI_foundRange(int iRange, bool bFollow = false); // disabled by K-Mod
	bool AI_foundFollow(); // K-Mod
	bool AI_assaultSeaTransport(bool bAttackBarbs = false, bool bLocal = false); // K-mod added bLocal
	bool AI_assaultSeaReinforce(bool bAttackBarbs = false); // BBAI
	bool AI_transportGoTo(CvPlot* pEndTurnPlot, CvPlot* pTargetPlot, int iFlags, MissionAITypes eMissionAI); // K-Mod

	bool AI_settlerSeaTransport();
	bool AI_settlerSeaFerry();
	bool AI_specialSeaTransportMissionary();
	bool AI_specialSeaTransportSpy();
	bool AI_carrierSeaTransport();
	bool AI_connectPlot(CvPlot* pPlot, int iRange = 0);
	bool AI_improveCity(CvCity* pCity);
	bool AI_improveLocalPlot(int iRange, CvCity* pIgnoreCity);
	bool AI_nextCityToImprove(CvCity* pCity);
	bool AI_nextCityToImproveAirlift();
	bool AI_irrigateTerritory();
	bool AI_fortTerritory(bool bCanal, bool bAirbase);
	//bool AI_improveBonus(int iMinValue = 0, CvPlot** ppBestPlot = NULL, BuildTypes* peBestBuild = NULL, int* piBestValue = NULL);
	bool AI_improveBonus(); // K-Mod
	bool AI_improvePlot(CvPlot* pPlot, BuildTypes eBuild);
	BuildTypes AI_betterPlotBuild(CvPlot* pPlot, BuildTypes eBuild);
	bool AI_connectBonus(bool bTestTrade = true);
	bool AI_connectCity();
	bool AI_routeCity();
	bool AI_routeTerritory(bool bImprovementOnly = false);
	bool AI_travelToUpgradeCity();
	bool AI_retreatToCity(bool bPrimary = false, bool bPrioritiseAirlift = false, int iMaxPath = MAX_INT);
	bool AI_handleStranded(int iFlags = 0); // K-mod
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/15/09                                jdog5000      */
/* Naval AI                                                                                     */
/************************************************************************************************/
	bool AI_pickup(UnitAITypes eUnitAI, bool bCountProduction = false, int iMaxPath = MAX_INT);
	bool AI_pickupStranded(UnitAITypes eUnitAI = NO_UNITAI, int iMaxPath = MAX_INT);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	bool AI_airOffensiveCity();
	bool AI_airDefensiveCity();
	bool AI_airCarrier();
	bool AI_missileLoad(UnitAITypes eTargetUnitAI, int iMaxOwnUnitAI = -1, bool bStealthOnly = false);
	bool AI_airStrike(int iThreshold = 0); // K-Mod note. this function now handles bombing defences, and defensive strikes.
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						9/26/08				jdog5000	    */
/* 	Air AI																	    */
/********************************************************************************/
	int AI_airOffenseBaseValue( CvPlot* pPlot );
	//bool AI_defensiveAirStrike(); // disabled by K-Mod
	bool AI_defendBaseAirStrike();
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								    */
/********************************************************************************/
	bool AI_airBombPlots();
	//bool AI_airBombDefenses(); // disabled by K-Mod
	bool AI_exploreAir();
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/12/09                                jdog5000      */
/* Player Interface                                                                             */
/************************************************************************************************/
	int AI_exploreAirPlotValue( CvPlot* pPlot );
	bool AI_exploreAir2();
	void AI_exploreAirMove();
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	bool AI_nuke();
	bool AI_nukeRange(int iRange);
	//bool AI_trade(int iValueThreshold); // deleted by K-Mod
	int AI_tradeMissionValue(CvPlot*& pBestPlot, int iThreshold = 0); // K-Mod
	bool AI_doTradeMission(CvPlot* pTradePlot); // K-Mod
	int AI_greatWorkValue(CvPlot*& pBestPlot, int iThreshold = 0); // K-Mod
	bool AI_doGreatWork(CvPlot* pCulturePlot); // K-Mod
	bool AI_infiltrate();
	bool AI_reconSpy(int iRange);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/20/09                                jdog5000      */
/* Espionage AI                                                                                 */
/************************************************************************************************/
	bool AI_revoltCitySpy();
	bool AI_bonusOffenseSpy(int iMaxPath);
	bool AI_cityOffenseSpy(int iRange, CvCity* pSkipCity = NULL);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	bool AI_espionageSpy();
	EspionageMissionTypes AI_bestPlotEspionage(PlayerTypes& eTargetPlayer, CvPlot*& pPlot, int& iData) const; // K-Mod
	bool AI_moveToStagingCity();
	bool AI_seaRetreatFromCityDanger();
	bool AI_airRetreatFromCityDanger();
	bool AI_airAttackDamagedSkip();

	bool AI_followBombard();

	bool AI_potentialEnemy(TeamTypes eTeam, const CvPlot* pPlot = NULL);
	// <advc.033>
	std::pair<int,int> AI_countPiracyTargets(CvPlot const& p,
			bool bStopIfAnyTarget = false) const;
	bool AI_isAnyPiracyTarget(CvPlot const& p) const;
	// </advc.033>
	bool AI_defendPlot(CvPlot* pPlot);
	int AI_pillageValue(CvPlot* pPlot, int iBonusValueThreshold = 0);
	//int AI_nukeValue(CvCity* pCity);
	int AI_nukeValue(CvPlot* pCenterPlot, int iSearchRange, CvPlot*& pBestTarget, int iCivilianTargetWeight = 50) const; // K-Mod
	//bool AI_canPillage(CvPlot& kPlot) const; // advc.003j

	int AI_searchRange(int iRange);
	bool AI_plotValid(CvPlot* pPlot);

	//int AI_finalOddsThreshold(CvPlot* pPlot, int iOddsThreshold);
	int AI_getWeightedOdds(CvPlot* pPlot, bool bPotentialEnemy = false); // K-Mod
	unsigned AI_unitBirthmarkHash(int iExtra = 0) const; // K-Mod
	unsigned AI_unitPlotHash(const CvPlot* pPlot, int iExtra = 0) const; // K-Mod

	int AI_stackOfDoomExtra() const;

	//bool AI_stackAttackCity(int iRange, int iPowerThreshold, bool bFollow = true);
	// K-Mod. used for adjacent cities only. Negative threshold means 'automatic'.
	bool AI_stackAttackCity(int iPowerThreshold = -1);
	bool AI_moveIntoCity(int iRange);

	bool AI_groupMergeRange(UnitAITypes eUnitAI, int iRange, bool bBiggerOnly = true, bool bAllowRegrouping = false, bool bIgnoreFaster = false);
	
	//bool AI_artistCultureVictoryMove(); // disabled by K-Mod
	bool AI_poach();
	bool AI_choke(int iRange = 1, bool bDefensive = false, int iFlags = 0);
	// advc.012: Current plot if p=NULL; cf. CvTeamAI::plotDefense
	int AI_plotDefense(CvPlot const* p = NULL) const;
	bool AI_solveBlockageProblem(CvPlot* pDestPlot, bool bDeclareWar);
	
	int AI_calculatePlotWorkersNeeded(CvPlot* pPlot, BuildTypes eBuild);

	//int AI_getEspionageTargetValue(CvPlot* pPlot, int iMaxPath);
	int AI_getEspionageTargetValue(CvPlot* pPlot); // K-Mod

	bool AI_canGroupWithAIType(UnitAITypes eUnitAI) const;
	bool AI_allowGroup(const CvUnit* pUnit, UnitAITypes eUnitAI) const;
	bool AI_moveSettlerToCoast(int iMaxPathTurns = 5); // advc.040

	// added so under cheat mode we can call protected functions for testing
	friend class CvGameTextMgr;

// Lead From Behind by UncutDragon. (edited for K-Mod)
public:
	void LFBgetBetterAttacker(CvUnit** ppAttacker, const CvPlot* pPlot, bool bPotentialEnemy, int& iAIAttackOdds, int& iAttackerValue);
};

#endif
