#pragma once

// teamAI.h

#ifndef CIV4_TEAM_AI_H
#define CIV4_TEAM_AI_H

#include "CvTeam.h"
#include "WarAndPeaceAI.h"  // advc.104


class CvTeamAI : public CvTeam
{

public:

	CvTeamAI();
	virtual ~CvTeamAI();

#ifdef _USRDLL
	// inlined for performance reasons, only in the dll
	static CvTeamAI& getTeam(TeamTypes eTeam)
	{
		FAssertMsg(eTeam != NO_TEAM, "eTeam is not assigned a valid value");
		FAssertMsg(eTeam < MAX_TEAMS, "eTeam is not assigned a valid value");
		return m_aTeams[eTeam]; 
	}
#endif
	DllExport static CvTeamAI& getTeamNonInl(TeamTypes eTeam);

	static void initStatics();
	static void freeStatics();

	void AI_init();
	void AI_initMemory(); // K-Mod. (needs game map to be initialized first)
	void AI_uninit();
	void AI_reset(bool bConstructor);

	void AI_doTurnPre();
	void AI_doTurnPost();

	void AI_makeAssignWorkDirty();

	void AI_updateAreaStrategies(bool bTargets = true); // advc.003: "Stragies"->"Strategies"
	void AI_updateAreaTargets();

	int AI_countFinancialTrouble() const; // addvc.003j (comment): unused
	int AI_countMilitaryWeight(CvArea* pArea) const;

	int AI_estimateTotalYieldRate(YieldTypes eYield) const; // K-Mod

	bool AI_deduceCitySite(const CvCity* pCity) const; // K-Mod

	bool AI_isAnyCapitalAreaAlone() const;
	bool AI_isPrimaryArea(CvArea* pArea) const;
	bool AI_hasCitiesInPrimaryArea(TeamTypes eTeam) const;
	bool AI_hasSharedPrimaryArea(TeamTypes eTeam) const; // K-Mod
	AreaAITypes AI_calculateAreaAIType(CvArea* pArea, bool bPreparingTotal = false) const;
	inline bool AI_isLonely() const { return m_bLonely; } // advc.109

	int AI_calculateAdjacentLandPlots(TeamTypes eTeam) const;
	int AI_calculateCapitalProximity(TeamTypes eTeam) const;
	int AI_calculatePlotWarValue(TeamTypes eTeam) const;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      07/10/08                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
	int AI_calculateBonusWarValue(TeamTypes eTeam) const;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	bool AI_haveSeenCities(TeamTypes eTeam, bool bPrimaryAreaOnly = false, int iMinimum = 1) const; // K-Mod
	bool AI_isWarPossible() const;
	bool AI_isLandTarget(TeamTypes eTeam) const;
	bool AI_isAllyLandTarget(TeamTypes eTeam) const;
	bool AI_shareWar(TeamTypes eTeam) const;
	 // advc.003, advc.130e:
	void AI_updateAttitudeCache(TeamTypes eTeam, bool bUpdateWorstEnemy = true);
	AttitudeTypes AI_getAttitude(TeamTypes eTeam, bool bForced = true) const;
	int AI_getAttitudeVal(TeamTypes eTeam, bool bForced = true) const;
	int AI_getMemoryCount(TeamTypes eTeam, MemoryTypes eMemory) const;

	int AI_chooseElection(const VoteSelectionData& kVoteSelectionData) const;

	// K-Mod
	int AI_warSpoilsValue(TeamTypes eTarget, WarPlanTypes eWarPlan,
			bool bConstCache) const; // advc.001n
	int AI_warCommitmentCost(TeamTypes eTarget, WarPlanTypes eWarPlan,
			bool bConstCache) const; // advc.001n
	int AI_warDiplomacyCost(TeamTypes eTarget) const;
	// K-Mod end

	//int AI_startWarVal(TeamTypes eTeam) const;
	int AI_startWarVal(TeamTypes eTarget, WarPlanTypes eWarPlan, // K-Mod
			bool bConstCache = false) const; // advc.001n
	int AI_endWarVal(TeamTypes eTeam) const;

	int CvTeamAI::AI_knownTechValModifier(TechTypes eTech) const; // K-Mod

	int AI_techTradeVal(TechTypes eTech, TeamTypes eTeam,
			bool bIgnoreDiscount = false, // advc.550a
			bool bPeaceDeal = false) const; // advc.140h
	DenialTypes AI_techTrade(TechTypes eTech, TeamTypes eTeam) const;

	int AI_mapTradeVal(TeamTypes eTeam) const;
	DenialTypes AI_mapTrade(TeamTypes eTeam) const;

	int AI_vassalTradeVal(TeamTypes eTeam) const;
	DenialTypes AI_vassalTrade(TeamTypes eTeam) const;

	int AI_surrenderTradeVal(TeamTypes eTeam) const;
	DenialTypes AI_surrenderTrade(TeamTypes eTeam, int iPowerMultiplier = 100,
			bool bCheckAccept = true) const; // advc.104o
	/*  advc.104o: Previously a magic number in CvPlayer::getTradeDenial; needed
		in additional places now. */
	static int const VASSAL_POWER_MOD_SURRENDER = 140;

	int AI_getLowestVictoryCountdown() const;
	int AI_countMembersWithStrategy(int iStrategy) const; // K-Mod
	// bbai start
	bool AI_isAnyMemberDoVictoryStrategy(int iVictoryStrategy) const;
	bool AI_isAnyMemberDoVictoryStrategyLevel4() const;
	bool AI_isAnyMemberDoVictoryStrategyLevel3() const;

	int AI_getWarSuccessRating() const; // K-Mod

	int AI_getEnemyPowerPercent( bool bConsiderOthers = false ) const;
	int AI_getAirPower() const; // K-Mod
	int AI_getRivalAirPower( ) const;
	// K-Mod. (refuse peace when we need war for conquest victory.)
	bool AI_refusePeace(TeamTypes ePeaceTeam) const;
	// K-Mod. (is war an acceptable side effect for event choices, vassal deals, etc)
	bool AI_refuseWar(TeamTypes eWarTeam) const;
	bool AI_acceptSurrender( TeamTypes eSurrenderTeam ) const;
	bool AI_isOkayVassalTarget( TeamTypes eTeam ) const;

	void AI_getWarRands( int &iMaxWarRand, int &iLimitedWarRand, int &iDogpileWarRand ) const;
	void AI_getWarThresholds( int &iMaxWarThreshold, int &iLimitedWarThreshold, int &iDogpileWarThreshold ) const;
	int AI_getTotalWarOddsTimes100( ) const;
	// bbai end
	/*  <advc.115b>
		advc.104: NO_VOTESOURCE if none built yet, AP if AP built but not UN;
		otherwise UN */
	VoteSourceTypes AI_getLatestVictoryVoteSource() const;
	bool AI_isAnyCloseToReligiousVictory() const; // </advc.115b>

	int AI_makePeaceTradeVal(TeamTypes ePeaceTeam, TeamTypes eTeam) const;
	DenialTypes AI_makePeaceTrade(TeamTypes ePeaceTeam, TeamTypes eTeam) const;

	int AI_declareWarTradeVal(TeamTypes eWarTeam, TeamTypes eTeam) const;
	DenialTypes AI_declareWarTrade(TeamTypes eWarTeam, TeamTypes eTeam, bool bConsiderPower = true) const;

	int AI_openBordersTradeVal(TeamTypes eTeam) const;
	DenialTypes AI_openBordersTrade(TeamTypes eTeam) const;

	int AI_defensivePactTradeVal(TeamTypes eTeam) const;
	DenialTypes AI_defensivePactTrade(TeamTypes eTeam) const;

	DenialTypes AI_permanentAllianceTrade(TeamTypes eTeam) const;

	int AI_roundTradeVal(int iVal) const; // advc.104k
	// <advc.130y>
	void AI_forgiveEnemy(TeamTypes eEnemyTeam, bool bCapitulated, bool bFreed);
	void AI_thankLiberator(TeamTypes eLiberator);
	// </advc.130y>
	TeamTypes AI_getWorstEnemy() const;
	void AI_updateWorstEnemy(
			bool bUpdateRivalTrade = true); // advc.130p
	// <advc.130p> 0 or less if tId isn't an enemy at all
	int AI_enmityValue(TeamTypes tId) const;
	double AI_getDiploDecay() const;
	double AI_recentlyMetMultiplier(TeamTypes tId) const;
	// </advc.130p>
	// advc.130k: Public visibility b/c CvPlayerAI needs it too
	int AI_randomCounterChange(int cap = -1, double pr = 0.5) const;
	int AI_getWarPlanStateCounter(TeamTypes eIndex) const;
	void AI_setWarPlanStateCounter(TeamTypes eIndex, int iNewValue);
	void AI_changeWarPlanStateCounter(TeamTypes eIndex, int iChange);

	int AI_getAtWarCounter(TeamTypes eIndex) const;
	void AI_setAtWarCounter(TeamTypes eIndex, int iNewValue);
	void AI_changeAtWarCounter(TeamTypes eIndex, int iChange);

	int AI_getAtPeaceCounter(TeamTypes eIndex) const;
	void AI_setAtPeaceCounter(TeamTypes eIndex, int iNewValue);
	void AI_changeAtPeaceCounter(TeamTypes eIndex, int iChange);

	int AI_getHasMetCounter(TeamTypes eIndex) const;
	void AI_setHasMetCounter(TeamTypes eIndex, int iNewValue);
	void AI_changeHasMetCounter(TeamTypes eIndex, int iChange);

	int AI_getOpenBordersCounter(TeamTypes eIndex) const;
	void AI_setOpenBordersCounter(TeamTypes eIndex, int iNewValue);
	void AI_changeOpenBordersCounter(TeamTypes eIndex, int iChange);

	int AI_getDefensivePactCounter(TeamTypes eIndex) const;
	void AI_setDefensivePactCounter(TeamTypes eIndex, int iNewValue);
	void AI_changeDefensivePactCounter(TeamTypes eIndex, int iChange);

	int AI_getShareWarCounter(TeamTypes eIndex) const;
	void AI_setShareWarCounter(TeamTypes eIndex, int iNewValue);
	void AI_changeShareWarCounter(TeamTypes eIndex, int iChange);

	int AI_getWarSuccess(TeamTypes eIndex) const;
	void AI_setWarSuccess(TeamTypes eIndex, int iNewValue);
	void AI_changeWarSuccess(TeamTypes eIndex, int iChange);
	// <advc.130m>
	void AI_reportSharedWarSuccess(int iIntensity, TeamTypes agentId, TeamTypes enemyId,
			bool bIgnoreDistress = false);
	int AI_getSharedWarSuccess(TeamTypes allyId) const;
	void AI_setSharedWarSuccess(TeamTypes allyId, int sws); // </advc.130m>
	// <advc.130n>
	int AI_getReligionKnownSince(ReligionTypes eReligion) const; 
	void AI_reportNewReligion(ReligionTypes eReligion);
	// </advc.130n>
	int AI_getEnemyPeacetimeTradeValue(TeamTypes eIndex) const;
	void AI_setEnemyPeacetimeTradeValue(TeamTypes eIndex, int iNewValue);
	void AI_changeEnemyPeacetimeTradeValue(TeamTypes eIndex, int iChange);

	int AI_getEnemyPeacetimeGrantValue(TeamTypes eIndex) const;
	void AI_setEnemyPeacetimeGrantValue(TeamTypes eIndex, int iNewValue);
	void AI_changeEnemyPeacetimeGrantValue(TeamTypes eIndex, int iChange);

	WarPlanTypes AI_getWarPlan(TeamTypes eIndex) const;
	bool AI_isChosenWar(TeamTypes eIndex) const;
	bool isAnyChosenWar() const; // advc.105
	bool AI_isSneakAttackPreparing(TeamTypes eIndex) const;
	bool AI_isSneakAttackReady(TeamTypes eIndex) const;
	bool AI_isSneakAttackReady() const; // K-Mod (any team)
	void AI_setWarPlan(TeamTypes eIndex, WarPlanTypes eNewValue, bool bWar = true);
	// advc.104:
	void AI_setWarPlanNoUpdate(TeamTypes eIndex, WarPlanTypes eNewValue);
	int AI_teamCloseness(TeamTypes eIndex, int iMaxDistance = -1,
			bool bConsiderLandTarget = false, // advc.104o
			bool bConstCache = false) const; // advc.001n

	// <advc.104>
	WarAndPeaceAI::Team& warAndPeaceAI(); 
	WarAndPeaceAI::Team const& warAndPeaceAI() const;
	// </advc.104>

	// <advc.104> These were protected
	int AI_maxWarRand() const;
	int AI_maxWarNearbyPowerRatio() const;
	int AI_maxWarDistantPowerRatio() const;
	int AI_maxWarMinAdjacentLandPercent() const;
	int AI_limitedWarRand() const;
	int AI_limitedWarPowerRatio() const;
	int AI_dogpileWarRand() const;
	int AI_makePeaceRand() const;
	int AI_noWarAttitudeProb(AttitudeTypes eAttitude) const;
	// </advc.104><advc.104y>
	int AI_noWarProbAdjusted(TeamTypes tId) const;
	bool AI_isAvoidWar(TeamTypes tId) const; // </advc.104y>
	bool AI_performNoWarRolls(TeamTypes eTeam);
	// advc.012:
	int AI_plotDefense(CvPlot const& p, bool bIgnoreBuilding = false) const;
	
	int AI_getAttitudeWeight(TeamTypes eTeam) const;
	
	int AI_getTechMonopolyValue(TechTypes eTech, TeamTypes eTeam) const;
	
	bool AI_isWaterAreaRelevant(CvArea* pArea);
	
	virtual void read(FDataStreamBase* pStream);
	virtual void write(FDataStreamBase* pStream);
	

	// K-Mod. Strength Memory - a very basic and rough reminder-map of how strong the enemy presence is on each plot.
	int AI_getStrengthMemory(int x, int y) const;
	void AI_setStrengthMemory(int x, int y, int value);
	// <advc.make> No longer inlined. To avoid including CvPlot.h.
	int AI_getStrengthMemory(const CvPlot* pPlot);
	void AI_setStrengthMemory(const CvPlot* pPlot, int value); // </advc.make>

protected:

	std::vector<int> m_aiStrengthMemory;
	// exponentially dimishes memory, and clears obviously obsolete memory.
	void AI_updateStrengthMemory();
	// K-Mod end

	static CvTeamAI* m_aTeams;

	TeamTypes m_eWorstEnemy;

	int* m_aiWarPlanStateCounter;
	int* m_aiAtWarCounter;
	int* m_aiAtPeaceCounter;
	int* m_aiHasMetCounter;
	int* m_aiOpenBordersCounter;
	int* m_aiDefensivePactCounter;
	int* m_aiShareWarCounter;
	int* m_aiWarSuccess;
	int* m_aiSharedWarSuccess; // advc.130m
	std::map<ReligionTypes,int> m_religionKnownSince; // advc.130n
	int* m_aiEnemyPeacetimeTradeValue;
	int* m_aiEnemyPeacetimeGrantValue;
	WarPlanTypes* m_aeWarPlan;

	bool m_bLonely; // advc.109

	WarAndPeaceAI::Team* m_pWpai; // advc.104

	int AI_noTechTradeThreshold() const;
	int AI_techTradeKnownPercent() const;
	
	void AI_doCounter();
	void AI_doWar();

	// advc.003: Chunk of code that occured twice in doWar
	void AI_abandonWarPlanIfTimedOut(int iAbandonTimeModifier, TeamTypes t,
			bool bLimited, int iEnemyPowerPercent);
	// advc.104o:
	int AI_declareWarTradeValLegacy(TeamTypes eWarTeam, TeamTypes eTeam) const;
	int AI_getOpenBordersAttitudeDivisor() const; // advc.130i
	double AI_OpenBordersCounterIncrement(TeamTypes tId) const; // advc.130z
	bool AI_isPursuingCircumnavigation() const; // advc.136a


	// added so under cheat mode we can call protected functions for testing
	friend class CvGameTextMgr;
	friend class CvDLLWidgetData;
};

// helper for accessing static functions
#ifdef _USRDLL
#define GET_TEAM CvTeamAI::getTeam
#else
#define GET_TEAM CvTeamAI::getTeamNonInl
#endif

/*  <advc.003> To facilitate access to and usage of team-level functions when
	given 'PlayerTypes' values. */
#define TEAMID(civId) GET_PLAYER(civId).getTeam()
#define TEAMREF(civId) GET_TEAM(TEAMID(civId))
// </advc.003>

#endif
