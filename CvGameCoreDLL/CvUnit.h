#pragma once

#ifndef CIV4_UNIT_H
#define CIV4_UNIT_H

#include "CvDLLEntity.h"

class CvPlot;
class CvArea;
class CvUnitInfo;
class CvSelectionGroup;
class CvArtInfoUnit;
class GroupPathFinder;
class CvUnitAI; // advc.003u
struct CombatDetails;

class CvUnit : public CvDLLUnitEntity
{
public:
	virtual ~CvUnit();

	void setupGraphical();
	void reloadEntity();

	void convert(CvUnit* pUnit);																			// Exposed to Python
	void kill(bool bDelay, PlayerTypes ePlayer = NO_PLAYER);												// Exposed to Python

	DllExport void NotifyEntity(MissionTypes eMission);

	void doTurn();
	void doTurnPost(); // advc.029
	void updateCombat(bool bQuick = false, /* advc.004c: */ bool* pbIntercepted = NULL, bool bSeaPatrol = false); // advc.004k
	void updateAirCombat(bool bQuick = false);
	bool updateAirStrike(CvPlot& kPlot, bool bQuick, bool bFinish);

	bool isActionRecommended(int iAction);
	void onActiveSelection(); // advc
	void updateFoundingBorder(bool bForceClear = false) const; // advc.004h

	bool isUnowned() const; // advc.061

	bool canDoCommand(CommandTypes eCommand, int iData1, int iData2, bool bTestVisible = false, bool bTestBusy = true) const; // Exposed to Python
	void doCommand(CommandTypes eCommand, int iData1, int iData2);											// Exposed to Python

	//FAStarNode* getPathLastNode() const; // disabled by K-Mod
	CvPlot& getPathEndTurnPlot() const;																		// Exposed to Python
	// K-Mod
	// <!-- custom: hoisted from multiline signature between `iMaxPath` and `bUseTempFinder` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	bool generatePath(CvPlot const& kTo, MovementFlags eFlags = NO_MOVEMENT_FLAGS, bool bReuse = false, int* piPathTurns = NULL, int iMaxPath = -1, bool bUseTempFinder = false) const; // advc.128; Exposed to Python
	GroupPathFinder& getPathFinder() const; // K-Mod
	// <advc>
	void pushGroupMoveTo(CvPlot const& kTo, MovementFlags eFlags = NO_MOVEMENT_FLAGS, bool bAppend = false, bool bManual = false, MissionAITypes eMissionAI = NO_MISSIONAI, CvPlot const* pMissionAIPlot = NULL, CvUnit const* pMissionAIUnit = NULL, bool bModified = false); // </advc>

	bool canEnterTerritory(TeamTypes eTeam, bool bIgnoreRightOfPassage = false, CvArea const* pArea = NULL) const; // advc: canEnterArea merged into canEnterTerritory; Exposed to Python
	TeamTypes getDeclareWarMove(const CvPlot* pPlot) const;													// Exposed to Python
	// K-Mod
	// <!-- custom: hoisted from multiline signature between `bAssumeVisible` and `bDangerCheck` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	bool canMoveInto(CvPlot const& kPlot, bool bAttack = false, bool bDeclareWar = false, bool bIgnoreLoad = false, bool bAssumeVisible = true, bool bDangerCheck = false) const; // advc.001k; Exposed to Python
	bool canMoveOrAttackInto(CvPlot const& kPlot, bool bDeclareWar = false, bool bDangerCheck = false) const; // advc.001k; Exposed to Python
	// bool canMoveThrough(const CvPlot* pPlot, bool bDeclareWar = false) const; // disabled by K-Mod (was exposed to Python)
	bool canEnterArea(CvArea const& kArea) const; // advc.030
	// <advc>
	bool isEnemyCity(CvPlot const& kPlot) const;											// Exposed to Python /via CyPlot)
	// was CvPlot::isValidDomainForLocation
	bool isValidDomain(CvPlot const& kPlot) const;											// Exposed to Python (via CyPlot)
	bool isRevealedValidDomain(CvPlot const& kPlot) const;
	// was CvPlot::isValidDomainForAction
	bool isValidDomain(bool bWater) const													// Exposed to Python (via CyPlot)
	{
		switch (getDomainType())
		{
			case DOMAIN_SEA: return (bWater || canMoveAllTerrain());
			case DOMAIN_AIR: return false;
			case DOMAIN_LAND: // fall through
			case DOMAIN_IMMOBILE: return (!bWater || canMoveAllTerrain());
			default: FAssert(false); return false;
		}
	}
	// Replacing CvPlot::isFriendlyCity
	bool isPlotValid(CvPlot const& kPlot) const;											// Exposed to Python (via CyPlot::isFriendlyCity)
	bool isRevealedPlotValid(CvPlot const& kPlot) const; // </advc>
	bool isInvasionMove(CvPlot const& kFrom, CvPlot const& kTo) const; // advc.162
	void attack(CvPlot* pPlot, bool bQuick, /* advc.004c: */ bool* pbIntercepted = NULL, bool bSeaPatrol = false); // advc
	void attackForDamage(CvUnit *pDefender, int attackerDamageChange, int defenderDamageChange);
	void fightInterceptor(CvPlot const& kPlot, bool bQuick);
	// advc: 1st param was CvPlot* (not const b/c of possible feature change)
	// <!-- custom: hoisted from multiline signature between `bShow` and `bJump` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	void move(CvPlot& kPlot, bool bShow, bool bJump = false, bool bGroup = true); // advc.163
	// K-Mod added bForceMove and bGroup
	bool jumpToNearestValidPlot(bool bGroup = false, bool bForceMove = false, bool bFreeMove = false); // advc.163; Exposed to Python

	bool canAutomate(AutomateTypes eAutomate) const;														// Exposed to Python
	void automate(AutomateTypes eAutomate);

	bool canScrap() const;																					// Exposed to Python
	// <!-- custom: Inherited cleanup callers could validly treat scrap() as successful under Base AdvCiv's permissive non-combat contract.
	// SAS made canScrap restrictive for AI preservation, so report success and let those callers distinguish the new vetoes from removal. See KI#331. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	bool scrap();
	// <!-- custom: New caller-vetted forced-scrap path bypasses routine SAS preservation; its mandatory-game-rule mode can also override the absolute AI-scrap preference. See KI#331. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	bool scrapForced(bool bMandatoryGameRule = false);

	bool canGift(bool bTestVisible = false, bool bTestTransport = true) const;								// Exposed to Python
	void gift(bool bTestTransport = true);

	bool canLoadOnto(CvUnit const& kUnit, CvPlot const& kPlot, bool bCheckMoves = false) const; // advc.123c; Exposed to Python
	void loadOnto(CvUnit& kUnit);
	bool canLoadOntoAnyUnit(CvPlot const& kPlot, bool bCheckMoves = false) const; // advc.123c; Exposed to Python
	void load();
	bool shouldLoadOnMove(const CvPlot* pPlot) const;

	bool canUnload() const;																					// Exposed to Python
	void unload();

	bool canUnloadAll() const;																				// Exposed to Python
	void unloadAll();

	bool canHold(const CvPlot* pPlot) const { return true; }												// Exposed to Python
	bool canSleep(const CvPlot* pPlot) const;																// Exposed to Python
	bool canFortify(const CvPlot* pPlot) const;																// Exposed to Python
	bool canAirPatrol(const CvPlot* pPlot) const;															// Exposed to Python
	void airCircle(bool bStart);

	bool canSeaPatrol(CvPlot const* pPlot = NULL, bool bCheckActivity = false) const; // advc; Exposed to Python
	bool isSeaPatrolling() const; // advc
	bool canReachBySeaPatrol(CvPlot const& kDest, CvPlot const* pFrom = NULL) const; // advc.004k

	bool canHeal(const CvPlot* pPlot) const;																// Exposed to Python
	bool canSentryHeal(const CvPlot* pPlot) const; // advc.004l
	bool canSentry(const CvPlot* pPlot) const;																// Exposed to Python

	int healRate(/* K-Mod: */ bool bLocation = true, bool bUnits = true, CvPlot const* pAt /* advc: */ = NULL) const;
	int healTurns(CvPlot const* pAt /* advc: */ = NULL) const;
	void doHeal();

		// advc (tbd.): Change the iX,iY params to a CvPlot const& kTarget (x10)
	bool canAirlift(const CvPlot* pPlot) const;																// Exposed to Python
	bool canAirliftAt(const CvPlot* pPlot, int iX, int iY) const;											// Exposed to Python
	bool airlift(int iX, int iY);

	bool isNukeVictim(const CvPlot* pPlot, TeamTypes eTeam, TeamTypes eObs = NO_TEAM) const; // kekm.7 (advc); Exposed to Python
	bool canNuke(CvPlot const* pFrom) const { return (nukeRange() != -1); }									// Exposed to Python
	bool canNukeAt(CvPlot const& kFrom, int iX, int iY, TeamTypes eObs = NO_TEAM) const; // kekm.7 (advc); Exposed to Python
	bool nuke(int iX, int iY);
	// <advc.650>
	int nukeInterceptionChance(CvPlot const& kTarget, TeamTypes eObs = NO_TEAM, TeamTypes* pBestTeam = NULL, EagerEnumMap<TeamTypes, bool> const* pTeamsAffected = NULL) const;
	// <advc.650>
	bool canRecon(const CvPlot* pPlot) const;																// Exposed to Python
	bool canReconAt(const CvPlot* pPlot, int iX, int iY) const;												// Exposed to Python
	bool recon(int iX, int iY);
	// <advc> Param lists changed for these two
	bool canAirBomb(CvPlot const* pFrom = NULL) const;														// Exposed to Python
	bool canAirBombAt(CvPlot const& kTarget, CvPlot const* pFrom = NULL) const; // </advc>					// Exposed to Python
	// <advc.255>
	enum StructureTypes { NO_STRUCTURE, STRUCTURE_IMPROVEMENT, STRUCTURE_ROUTE };
	// </advc.255>
	// <!-- custom: hoisted from multiline signature between `bTestVisibility` and `bForceImprovement` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	StructureTypes getDestructibleStructureAt(CvPlot const& kTarget, bool bTestVisibility, bool bForceImprovement = false) const; // advc.111
	int airBombDefenseDamage(CvCity const& kCity) const; // advc
	bool airBomb(CvPlot& kTarget, /* advc.004c: */ bool* pbIntercepted = NULL, bool bForceImprovement = false); // advc.111

	bool canAirStrike(CvPlot const& kPlot) const; // (advc.004c: was protected)

	CvCity* bombardTarget(CvPlot const& kFrom) const;														// Exposed to Python
	bool canBombard(CvPlot const& kFrom) const;																// Exposed to Python
	int damageToBombardTarget(CvPlot const& kFrom) const; // advc
	bool bombard();

	bool canParadrop(const CvPlot* pPlot) const;															// Exposed to Python
	bool canParadropAt(const CvPlot* pPlot, int iX, int iY) const;											// Exposed to Python
	bool paradrop(int iX, int iY, /* advc.004c: */ IDInfo* pInterceptor = NULL);

	bool canPillage(CvPlot const& kPlot) const;																// Exposed to Python
	bool pillage(/* advc.111: */ bool bForceImprovement = false);

	bool canPlunder(CvPlot const& kPlot, bool bTestVisible = false) const;									// Exposed to Python
	bool plunder();
	void updatePlunder(int iChange, bool bUpdatePlotGroups);
	// advc
	// <!-- custom: hoisted from multiline signature between `iExtra` and `bCheckCanPlunder` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	void blockadeRange(std::vector<CvPlot*>& r, int iExtra = 0, bool bCheckCanPlunder = true) const; // advc.033

	int sabotageCost(const CvPlot* pPlot) const;															// Exposed to Python
	int sabotageProb(const CvPlot* pPlot, ProbabilityTypes eProbStyle = PROBABILITY_REAL) const;			// Exposed to Python
	bool canSabotage(const CvPlot* pPlot, bool bTestVisible = false) const;									// Exposed to Python
	bool sabotage();

	int destroyCost(const CvPlot* pPlot) const;																// Exposed to Python
	int destroyProb(const CvPlot* pPlot, ProbabilityTypes eProbStyle = PROBABILITY_REAL) const;				// Exposed to Python
	bool canDestroy(const CvPlot* pPlot, bool bTestVisible = false) const;									// Exposed to Python
	bool destroy();

	int stealPlansCost(const CvPlot* pPlot) const;															// Exposed to Python
	int stealPlansProb(const CvPlot* pPlot, ProbabilityTypes eProbStyle = PROBABILITY_REAL) const;			// Exposed to Python
	bool canStealPlans(const CvPlot* pPlot, bool bTestVisible = false) const;								// Exposed to Python
	bool stealPlans();

	bool canFound(const CvPlot* pPlot, bool bTestVisible = false) const;									// Exposed to Python
	bool found();

	bool canSpread(const CvPlot* pPlot, ReligionTypes eReligion, bool bTestVisible = false) const;			// Exposed to Python
	bool spread(ReligionTypes eReligion);

	bool canSpreadCorporation(const CvPlot* pPlot, CorporationTypes eCorporation, bool bTestVisible = false) const; // Exposed to Python
	bool spreadCorporation(CorporationTypes eCorporation);
	int spreadCorporationCost(CorporationTypes eCorporation, CvCity const* pCity) const;

	bool canJoin(const CvPlot* pPlot, SpecialistTypes eSpecialist) const;									// Exposed to Python
	bool join(SpecialistTypes eSpecialist);

	bool canConstruct(const CvPlot* pPlot, BuildingTypes eBuilding, bool bTestVisible = false) const;		// Exposed to Python
	bool construct(BuildingTypes eBuilding);

	TechTypes getDiscoveryTech() const;																		// Exposed to Python
	int getDiscoverResearch(TechTypes eTech) const;															// Exposed to Python
	bool canDiscover(const CvPlot* pPlot) const;															// Exposed to Python
	bool discover();

	int getMaxHurryProduction(CvCity const* pCity) const;													// Exposed to Python
	int getHurryProduction(const CvPlot* pPlot) const;														// Exposed to Python
	bool canHurry(const CvPlot* pPlot, bool bTestVisible = false) const;									// Exposed to Python
	bool hurry();

	int getTradeGold(const CvPlot* pPlot) const;															// Exposed to Python
	bool canTrade(const CvPlot* pPlot, bool bTestVisible = false) const;									// Exposed to Python
	bool trade();

	int getGreatWorkCulture(const CvPlot* pPlot, int* piPerEra = NULL) const; // advc.251; Exposed to Python
	bool canGreatWork(const CvPlot* pPlot) const;															// Exposed to Python
	bool greatWork();

	int getEspionagePoints(const CvPlot* pPlot) const;														// Exposed to Python
	bool canInfiltrate(const CvPlot* pPlot, bool bTestVisible = false) const;								// Exposed to Python
	bool infiltrate();

	bool canEspionage(const CvPlot* pPlot, bool bTestVisible = false) const;
	bool espionage(EspionageMissionTypes eMission, int iData);
	// <!-- custom: Mission-phase interception carries the intended espionage mission and pre-mission target context so caught Spies remain attributable.
	// Ordinary travel interception leaves these optional fields empty. (ChatGPT-5.6-Sol) -->
	bool testSpyIntercepted(PlayerTypes eTargetPlayer, bool bMission, int iModifier, char const* szSummaryPhase, EspionageMissionTypes eMission = NO_ESPIONAGEMISSION, int iData = -1, ImprovementTypes eTargetImprovement = NO_IMPROVEMENT, RouteTypes eTargetRoute = NO_ROUTE, UnitTypes eTargetUnit = NO_UNIT); // (K-Mod added bMission)
	int getSpyInterceptPercent(TeamTypes eTargetTeam, bool bMission) const; // (K-Mod added bMission)
	bool isIntruding() const;

	bool canGoldenAge(const CvPlot* pPlot, bool bTestVisible = false) const;								// Exposed to Python
	bool goldenAge();

	bool canBuild(CvPlot const& pPlot, BuildTypes eBuild, bool bTestVisible = false, /* advc.181: */ bool bIgnoreFoW = true) const; // Exposed to Python
	bool build(BuildTypes eBuild);

	bool canPromote(PromotionTypes ePromotion, int iLeaderUnitId /* advc: */ = FFreeList::INVALID_INDEX) const; // Exposed to Python
	void promote(PromotionTypes ePromotion, int iLeaderUnitId /* advc: */ = FFreeList::INVALID_INDEX); // Exposed to Python
	int promotionHeal(PromotionTypes ePromotion = NO_PROMOTION) const; // advc

	int canLead(const CvPlot* pPlot, int iUnitId) const;
	bool lead(int iUnitId);

	int canGiveExperience(const CvPlot* pPlot) const;														// Exposed to Python
	bool giveExperience();																					// Exposed to Python
	int getStackExperienceToGive(int iNumUnits) const;

	int upgradePrice(UnitTypes eUnit) const;																// Exposed to Python
	int upgradeXPChange(UnitTypes eUnit = NO_UNIT) const; // advc.080, advc.131e
	bool upgradeAvailable(UnitTypes eFromUnit, UnitClassTypes eToUnitClass, int iCount = 0) const;			// Exposed to Python
	// <!-- custom: Optional known-city flag lets AI routing validate economic eligibility after it has already found a suitable upgrade city, without incorrectly retesting only the nearest city. See KI#188.3.3. (GPT-5.6-Sol) -->
	bool canUpgrade(UnitTypes eUnit, bool bTestVisible = false, bool bUpgradeCityKnown = false) const;			// Exposed to Python
	bool isReadyForUpgrade() const;
	/*	has upgrade is used to determine if an upgrade is possible,
		it specifically does not check whether the unit can move, whether the current plot is owned, enough gold
		those are checked in canUpgrade()
		does not search all cities, only checks the closest one */
	bool hasUpgrade(bool bSearch = false) const { return (getUpgradeCity(bSearch) != NULL); } // Exposed to Python
	bool hasUpgrade(UnitTypes eUnit, bool bSearch = false) const { return (getUpgradeCity(eUnit, bSearch) != NULL); }
	// <!-- custom: Optional output identifies the upgrade selected by the city search so unit AI can validate it without repeating the expensive unit-type and city loops. See KI#188.3.3. (GPT-5.6-Sol) -->
	CvCity* getUpgradeCity(bool bSearch = false, UnitTypes* peUpgradeUnit = NULL) const;
	CvCity* getUpgradeCity(UnitTypes eUnit, bool bSearch = false, int* iSearchValue = NULL) const;
	//void upgrade(UnitTypes eUnit);
	CvUnit* upgrade(UnitTypes eUnit); // K-Mod

	HandicapTypes getHandicapType() const;																	// Exposed to Python
	CivilizationTypes getCivilizationType() const;															// Exposed to Python
	const wchar* getVisualCivAdjective(TeamTypes eForTeam) const;
	SpecialUnitTypes getSpecialUnitType() const { return m_pUnitInfo->getSpecialUnitType(); } // Exposed to Python
	UnitTypes getCaptureUnitType(CivilizationTypes eCivilization) const;									// Exposed to Python
	int getCaptureOdds(CvUnit const& kDefender) const; // advc.010
	UnitCombatTypes getUnitCombatType() const { return m_pUnitInfo->getUnitCombatType(); } // Exposed to Python
	DllExport DomainTypes getDomainType() const { return m_pUnitInfo->getDomainType(); } // Exposed to Python
	InvisibleTypes getInvisibleType() const { return m_pUnitInfo->getInvisibleType(); } // Exposed to Python
	int getNumSeeInvisibleTypes() const { return m_pUnitInfo->getNumSeeInvisibleTypes(); } // Exposed to Python
	InvisibleTypes getSeeInvisibleType(int i) const { return m_pUnitInfo->getSeeInvisibleType(i); } // Exposed to Python

	int flavorValue(FlavorTypes eFlavor) const { return m_pUnitInfo->getFlavorValue(eFlavor); } // Exposed to Python

	bool isBarbarian() const;																				// Exposed to Python
	// advc.313:
	bool isKnownSeaBarbarian() const
	{
		return (isBarbarian() && getDomainType() == DOMAIN_SEA &&
				// Future-proof for Barbarian Privateers
				!m_pUnitInfo->isHiddenNationality());
	}
	bool isHuman() const;																					// Exposed to Python

	int visibilityRange() const;																			// Exposed to Python

	int baseMoves() const;																					// Exposed to Python
	int maxMoves() const { return (baseMoves() * GC.getMOVE_DENOMINATOR()); } // Exposed to Python
	int movesLeft() const { return std::max(0, maxMoves() - getMoves()); } // Exposed to Python
	DllExport bool canMove() const;																			// Exposed to Python
	DllExport bool hasMoved() const { return (getMoves() > 0); } // Exposed to Python

	int airRange() const { return (m_pUnitInfo->getAirRange() + getExtraAirRange()); } // Exposed to Python
	int nukeRange() const { return m_pUnitInfo->getNukeRange(); } // Exposed to Python
	bool isNuke() const { return m_pUnitInfo->isNuke(); } // advc

	bool canBuildRoute() const;																				// Exposed to Python
	DllExport BuildTypes getBuildType() const;																// Exposed to Python
	int workRate(bool bMax) const;																			// Exposed to Python

	bool isAnimal() const { return m_pUnitInfo->isAnimal(); } // Exposed to Python
	bool isNoBadGoodies() const { return m_pUnitInfo->isNoBadGoodies(); } // Exposed to Python
	bool isOnlyDefensive() const { return m_pUnitInfo->isOnlyDefensive(); } // Exposed to Python
	bool isNoCityCapture() const;																			// Exposed to Python
	bool isNoUnitCapture() const; // advc.315b
	bool isRivalTerritory() const { return m_pUnitInfo->isRivalTerritory(); } // Exposed to Python
	bool isMilitaryHappiness() const;																		// Exposed to Python
	bool isGarrisonInTeamCity() const; // advc.184
	int garrisonStrength() const; // advc.101
	bool isInvestigate() const { return m_pUnitInfo->isInvestigate(); } // Exposed to Python
	bool isCounterSpy() const { return m_pUnitInfo->isCounterSpy(); } // Exposed to Python
	bool isSpy() const { return m_pUnitInfo->isSpy(); }
	bool isFound() const { return m_pUnitInfo->isFound(); } // Exposed to Python
	bool isGoldenAge() const;																				// Exposed to Python
	bool canCoexistWithEnemyUnit(TeamTypes eTeam) const;													// Exposed to Python

	DllExport bool isFighting() const { return (getCombatUnit() != NULL); } // Exposed to Python
	DllExport bool isAttacking() const;																		// Exposed to Python
	DllExport bool isDefending() const;																		// Exposed to Python
	bool isInCombat() const;																				// Exposed to Python

	DllExport int maxHitPoints() const { return GC.getMAX_HIT_POINTS(); } // Exposed to Python
	int currHitPoints() const { return (maxHitPoints() - getDamage()); } // Exposed to Python
	bool isHurt() const { return (getDamage() > 0); } // Exposed to Python
	DllExport bool isDead() const { return (getDamage() >= maxHitPoints()); } // Exposed to Python
	// advc:
	bool isLethalDamage(int iDamage) { return (currHitPoints() - iDamage <= 0); }

	void setBaseCombatStr(int iCombat);																		// Exposed to Python
	int baseCombatStr() const { return m_iBaseCombat; } // Exposed to Python // advc: Default values - to make clear that these can be NULL.
	int maxCombatStr(CvPlot const* pPlot = NULL, CvUnit const* pAttacker = NULL, CombatDetails* pCombatDetails = NULL, bool bGarrisonStrength = false) const; // advc.500b; Exposed to Python
	// Exposed to Python
	// <!-- custom: hoisted from multiline signature between `pAttacker` and `pCombatDetails` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	int currCombatStr(CvPlot const* pPlot = NULL, CvUnit const* pAttacker = NULL, CombatDetails* pCombatDetails = NULL) const { return ((maxCombatStr(pPlot, pAttacker, pCombatDetails) * currHitPoints()) / maxHitPoints()); }
	int currFirepower(const CvPlot* pPlot = NULL, const CvUnit* pAttacker = NULL) const { return ((maxCombatStr(pPlot, pAttacker) + currCombatStr(pPlot, pAttacker) + 1) / 2); } // Exposed to Python
	int currEffectiveStr(CvPlot const* pPlot = NULL, CvUnit const* pAttacker = NULL, CombatDetails* pCombatDetails = NULL, int iCurrentHP = -1) const; // advc.139
	DllExport float maxCombatStrFloat(const CvPlot* pPlot, const CvUnit* pAttacker) const;					// Exposed to Python
	DllExport float currCombatStrFloat(const CvPlot* pPlot, const CvUnit* pAttacker) const;					// Exposed to Python
	// advc: was protected
	void getDefenderCombatValues(CvUnit const& kDefender, const CvPlot* pPlot, int iOurStrength, int iOurFirepower, int& iTheirOdds, int& iTheirStrength, int& iOurDamage, int& iTheirDamage, CombatDetails* pTheirDetails = NULL) const;

	DllExport bool canFight() const { return (baseCombatStr() > 0); } // Exposed to Python
	bool canSiege(TeamTypes eTeam) const;																	// Exposed to Python
	bool canCombat() const; // kekm.8
	bool canAttack() const;																					// Exposed to Python
	bool canAttack(const CvUnit& kDefender) const;
	bool canDefend(const CvPlot* pPlot = NULL) const;														// Exposed to Python
	// <advc>
	bool canBeAttackedBy(PlayerTypes eAttackingPlayer, CvUnit const* pAttacker, bool bTestEnemy, bool bTestPotentialEnemy, bool bTestVisible, bool bTestCanAttack) const; // </advc>
	bool isBetterDefenderThan(const CvUnit* pDefender, const CvUnit* pAttacker) const;						// Exposed to Python

	int airBaseCombatStr() const { return m_pUnitInfo->getAirCombat(); } // Exposed to Python
	int airMaxCombatStr(const CvUnit* pOther) const;														// Exposed to Python
	int airCurrCombatStr(const CvUnit* pOther) const { return ((airMaxCombatStr(pOther) * currHitPoints()) / maxHitPoints()); } // Exposed to Python
	DllExport float airMaxCombatStrFloat(const CvUnit* pOther) const;										// Exposed to Python
	DllExport float airCurrCombatStrFloat(const CvUnit* pOther) const;										// Exposed to Python
	int combatLimit() const { return m_pUnitInfo->getCombatLimit(); } // Exposed to Python
	int airCombatLimit() const { return m_pUnitInfo->getAirCombatLimit(); } // Exposed to Python
	DllExport bool canAirAttack() const { return (airBaseCombatStr() > 0); } // Exposed to Python
	bool canAirDefend(CvPlot const* pPlot = NULL) const;													// Exposed to Python
	int airCombatDamage(const CvUnit* pDefender) const;														// Exposed to Python
	int rangeCombatDamage(const CvUnit* pDefender) const;													// Exposed to Python
	CvUnit* bestInterceptor(CvPlot const& kPlot, bool bOdds = false) const; // advc.004c; Exposed to Python
	CvUnit* bestSeaPillageInterceptor(CvUnit* pPillager, int iMinOdds) const;								// Exposed to Python

	bool isAutomated() const;																				// Exposed to Python
	DllExport bool isWaiting() const;																		// Exposed to Python
	bool isFortifyable() const;																				// Exposed to Python
	int fortifyModifier() const;																			// Exposed to Python

	int experienceNeeded() const;																			// Exposed to Python
	int attackXPValue() const;																				// Exposed to Python
	int defenseXPValue() const;																				// Exposed to Python
	int maxXPValue() const;																					// Exposed to Python

	int firstStrikes() const { return std::max(0, m_pUnitInfo->getFirstStrikes() + getExtraFirstStrikes()); } // Exposed to Python
	int chanceFirstStrikes() const { return std::max(0, m_pUnitInfo->getChanceFirstStrikes() + getExtraChanceFirstStrikes()); } // Exposed to Python
	int maxFirstStrikes() const { return firstStrikes() + chanceFirstStrikes(); }							// Exposed to Python
	DllExport bool isRanged() const;																		// Exposed to Python

	bool alwaysInvisible() const { return m_pUnitInfo->isInvisible(); } // Exposed to Python
	bool immuneToFirstStrikes() const;																		// Exposed to Python
	bool noDefensiveBonus() const { return m_pUnitInfo->isNoDefensiveBonus(); } // Exposed to Python
	bool ignoreBuildingDefense() const { return m_pUnitInfo->isIgnoreBuildingDefense(); } // Exposed to Python
	// <!-- custom: add these so we can show TXT_KEY_UNIT_CANNOT_ENTER_SHORT_MAP_VIEW ingame in map view of a unit. -->
	bool isAnyTerrainImpassable() const { return m_pUnitInfo->isAnyTerrainImpassable(); }
	bool isAnyFeatureImpassable() const { return m_pUnitInfo->isAnyFeatureImpassable(); }
	// <!-- custom: also add the getters to list all terrain(s) and feature(s) impassables and according to chatgpt's explanation. -->
	bool getTerrainImpassable(TerrainTypes eTerrain) const { return m_pUnitInfo->getTerrainImpassable(eTerrain); }
	bool getFeatureImpassable(FeatureTypes eFeature) const { return m_pUnitInfo->getFeatureImpassable(eFeature); }
	bool canMoveImpassable() const { return m_pUnitInfo->canMoveImpassable(); } // Exposed to Python
	bool canMoveAllTerrain() const { return m_pUnitInfo->isCanMoveAllTerrain(); } // Exposed to Python
	// advc.opt: Now also true for all air units
	bool flatMovementCost() const { return m_bFlatMovement; } // Exposed to Python
	bool ignoreTerrainCost() const { return m_pUnitInfo->isIgnoreTerrainCost(); } // Exposed to Python
	bool isNeverInvisible() const;																			// Exposed to Python
	DllExport bool isInvisible(TeamTypes eTeam, bool bDebug, bool bCheckCargo = true) const;				// Exposed to Python
	bool isNukeImmune() const { return m_pUnitInfo->isNukeImmune(); } // Exposed to Python

	int maxInterceptionProbability() const;																	// Exposed to Python
	int currInterceptionProbability() const;																// Exposed to Python
	int evasionProbability() const { return std::max(0, m_pUnitInfo->getEvasionProbability() + getExtraEvasion()); } // Exposed to Python
	int withdrawalProbability() const;																		// Exposed to Python

	// advc.159 (note): getExtraCollateralDamage works multiplicatively since BtS 3.17
	int collateralDamage() const { return std::max(0, m_pUnitInfo->getCollateralDamage()); } // Exposed to Python
	int collateralDamageLimit() const { return std::max(0, m_pUnitInfo->getCollateralDamageLimit() * GC.getMAX_HIT_POINTS() / 100); } // Exposed to Python
	int collateralDamageMaxUnits() const																	// Exposed to Python
	{	// advc: Never negative in XML
		return /*std::max(0,*/m_pUnitInfo->getCollateralDamageMaxUnits();
	}

	int cityAttackModifier() const { return (m_pUnitInfo->getCityAttackModifier() + getExtraCityAttackPercent()); } // Exposed to Python
	int cityDefenseModifier() const { return (m_pUnitInfo->getCityDefenseModifier() + getExtraCityDefensePercent()); } // Exposed to Python
	int animalCombatModifier() const;																		// Exposed to Python
	int barbarianCombatModifier() const; // advc.315c
	int hillsAttackModifier() const;																		// Exposed to Python
	int hillsDefenseModifier() const;																		// Exposed to Python
	int terrainAttackModifier(TerrainTypes eTerrain) const;													// Exposed to Python
	int terrainDefenseModifier(TerrainTypes eTerrain) const;												// Exposed to Python
	int featureAttackModifier(FeatureTypes eFeature) const;													// Exposed to Python
	int featureDefenseModifier(FeatureTypes eFeature) const;												// Exposed to Python
	int unitClassAttackModifier(UnitClassTypes eUnitClass) const;											// Exposed to Python
	int unitClassDefenseModifier(UnitClassTypes eUnitClass) const;											// Exposed to Python
	int unitCombatModifier(UnitCombatTypes eUnitCombat) const;												// Exposed to Python
	int domainModifier(DomainTypes eDomain) const;															// Exposed to Python

	int bombardRate() const;																				// Exposed to Python
	int airBombBaseRate() const;																			// Exposed to Python
	int airBombCurrRate() const;																			// Exposed to Python

	SpecialUnitTypes specialCargo() const;																	// Exposed to Python
	DomainTypes domainCargo() const;																		// Exposed to Python
	int cargoSpace() const { return m_iCargoCapacity; }														// Exposed to Python
	void changeCargoSpace(int iChange);																		// Exposed to Python
	bool isFull() const { return (getCargo() >= cargoSpace()); }											// Exposed to Python
	int cargoSpaceAvailable(SpecialUnitTypes eSpecialCargo = NO_SPECIALUNIT, DomainTypes eDomainCargo = NO_DOMAIN) const; // Exposed to Python
	bool hasCargo() const { return (getCargo() > 0); }														// Exposed to Python
	//bool canCargoAllMove() const; // disabled by K-Mod (was exposed to Python)
	bool canCargoEnterTerritory(TeamTypes eTeam, bool bIgnoreRightOfPassage, CvArea const& kArea) const;
	int getUnitAICargo(UnitAITypes eUnitAI) const;															// Exposed to Python

	static CvUnit* fromIDInfo(IDInfo id); // advc
	DllExport int getID() const { return m_iID; }															// Exposed to Python
	int getIndex() const { return (getID() & FLTA_INDEX_MASK); }
	DllExport IDInfo getIDInfo() const { return IDInfo(getOwner(), getID()); }
	void setID(int iID);

	int getGroupID() const { return m_iGroupID; }															// Exposed to Python
	// advc: I don't think a unit is ever supposed to not be in a group
	//bool isInGroup() const; // Exposed to Python ( advc: still available to Python; see CyUnit.cpp.)
	bool isGroupHead() const;																				// Exposed to Python
	CvSelectionGroup* getGroupExternal() const; // advc.003s: exported through .def file
	// <advc.003s> Should make loops over units in a group less hazardous
	CvSelectionGroup const* getGroup() const;
	CvSelectionGroup* getGroup();																			// Exposed to Python
	// </advc.003s>
	bool isBeforeUnitCycle(CvUnit const& kOther) const; // advc: Moved from CvGameCoreUtils
	bool canJoinGroup(const CvPlot* pPlot, CvSelectionGroup const* pSelectionGroup) const;
	void joinGroup(CvSelectionGroup* pSelectionGroup, bool bRemoveSelected = false, bool bRejoin = true);

	DllExport int getHotKeyNumber();																													// Exposed to Python
	void setHotKeyNumber(int iNewValue);																											// Exposed to Python

	DllExport int getX() const { return m_iX; } // advc.inl: was "getX_INLINE"								// Exposed to Python
	DllExport int getY() const { return m_iY; } // advc.inl: was "getY_INLINE"								// Exposed to Python
	void setXY(int iX, int iY, bool bGroup = false, bool bUpdate = true, bool bShow = false, bool bCheckPlotVisible = false); // Exposed to Python

	bool at(int iX, int iY) const { return (getX() == iX && getY() == iY); }								// Exposed to Python
	bool at(CvPlot const& kPlot) const { return atPlot(&kPlot); }
	DllExport bool atPlot(const CvPlot* pPlot) const { return (plot() == pPlot); }							// Exposed to Python
	DllExport CvPlot* plot() const { return m_pPlot; } // advc.opt: cached									// Exposed to Python
	CvPlot& getPlot() const { return *m_pPlot; } // advc
	void updatePlot(); // advc.opt
	//int getArea() const;																					// Exposed to Python
	// <advc>
	CvArea& getArea() const { return *m_pArea; }
	CvArea* area() const { return m_pArea; }																// Exposed to Python
	bool isArea(CvArea const& kArea) const { return (area() == &kArea); }
	bool sameArea(CvUnit const& kOther) const { return (area() == kOther.area()); }
	void updateArea();
	// </advc>
	int getLastMoveTurn() const;
	void setLastMoveTurn(int iNewValue);

	CvPlot* getReconPlot() const;																			// Exposed to Python
	void setReconPlot(CvPlot* pNewValue);																	// Exposed to Python

	int getGameTurnCreated() const { return m_iGameTurnCreated; }											// Exposed to Python
	void setGameTurnCreated(int iNewValue);

	DllExport int getDamage() const { return m_iDamage; }													// Exposed to Python
	void setDamage(int iNewValue, PlayerTypes ePlayer = NO_PLAYER, bool bNotifyEntity = true);				// Exposed to Python
	void changeDamage(int iChange, PlayerTypes ePlayer = NO_PLAYER);										// Exposed to Python

	int getMoves() const { return m_iMoves; } // advc (note): Moves spent, not remaining.					// Exposed to Python
	void setMoves(int iNewValue);																			// Exposed to Python
	void changeMoves(int iChange);																			// Exposed to Python
	void finishMoves();																						// Exposed to Python

	int getExperience() const { return m_iExperience; }														// Exposed to Python
	void setExperience(int iNewValue, int iMax = -1);														// Exposed to Python
	// bool bUpdateGlobal = false
	// <!-- custom: hoisted from multiline signature between `bInBorders` and `iGlobalPercent` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	void changeExperience(int iChange, int iMax = -1, bool bFromCombat = false, bool bInBorders = false, int iGlobalPercent = 0); // advc.312; Exposed to Python
	int getGlobalXPPercent() const; // advc.312

	int getLevel() const { return m_iLevel; }																// Exposed to Python
	void setLevel(int iNewValue);
	void changeLevel(int iChange);

	int getCargo() const { return m_iCargo; }																// Exposed to Python
	void changeCargo(int iChange);
	void getCargoUnits(std::vector<CvUnit*>& aUnits) const;

	CvPlot* getAttackPlot() const;
	void setAttackPlot(const CvPlot* pNewValue, bool bAirCombat);
	bool isInAirCombat() const;

	DllExport int getCombatTimer() const;
	void setCombatTimer(int iNewValue);
	void changeCombatTimer(int iChange);

	int getCombatFirstStrikes() const;
	void setCombatFirstStrikes(int iNewValue);
	void changeCombatFirstStrikes(int iChange);

	int getFortifyTurns() const;																			// Exposed to Python
	void setFortifyTurns(int iNewValue);
	void changeFortifyTurns(int iChange);

	int getBlitzCount() const { return m_iBlitzCount; }
	// advc.164: was > 0
	bool isBlitz() const { return (getBlitzCount() != 0); } // Exposed to Python
	void changeBlitzCount(int iChange);

	int getAmphibCount() const { return m_iAmphibCount; }
	bool isAmphib() const { return (getAmphibCount() > 0); }												// Exposed to Python
	void changeAmphibCount(int iChange);

	int getRiverCount() const { return m_iRiverCount; }
	bool isRiver() const { return (getRiverCount() > 0); }													// Exposed to Python
	void changeRiverCount(int iChange);

	int getEnemyRouteCount() const { return m_iEnemyRouteCount; }
	bool isEnemyRoute() const { return (getEnemyRouteCount() > 0); }										// Exposed to Python
	void changeEnemyRouteCount(int iChange);

	int getAlwaysHealCount() const { return m_iAlwaysHealCount; }
	bool isAlwaysHeal() const { return (getAlwaysHealCount() > 0); }										// Exposed to Python
	void changeAlwaysHealCount(int iChange);

	int getHillsDoubleMoveCount() const { return m_iHillsDoubleMoveCount; }
	bool isHillsDoubleMove() const { return (getHillsDoubleMoveCount() > 0); }								// Exposed to Python
	void changeHillsDoubleMoveCount(int iChange);

	int getImmuneToFirstStrikesCount() const { return m_iImmuneToFirstStrikesCount; }
	void changeImmuneToFirstStrikesCount(int iChange);

	int getExtraVisibilityRange() const { return m_iExtraVisibilityRange; }									// Exposed to Python
	void changeExtraVisibilityRange(int iChange);

	int getExtraMoves() const { return m_iExtraMoves; }														// Exposed to Python
	void changeExtraMoves(int iChange);

	int getExtraMoveDiscount() const { return m_iExtraMoveDiscount; }										// Exposed to Python
	void changeExtraMoveDiscount(int iChange);

	int getExtraAirRange() const;																			// Exposed to Python
	void changeExtraAirRange(int iChange);

	int getExtraIntercept() const;																			// Exposed to Python
	void changeExtraIntercept(int iChange);

	int getExtraEvasion() const;																			// Exposed to Python
	void changeExtraEvasion(int iChange);

	int getExtraFirstStrikes() const { return m_iExtraFirstStrikes; }										// Exposed to Python
	void changeExtraFirstStrikes(int iChange);

	int getExtraChanceFirstStrikes() const { return m_iExtraChanceFirstStrikes; }							// Exposed to Python
	void changeExtraChanceFirstStrikes(int iChange);

	int getExtraWithdrawal() const { return m_iExtraWithdrawal; }											// Exposed to Python
	void changeExtraWithdrawal(int iChange);

	int getExtraCollateralDamage() const { return m_iExtraCollateralDamage; }								// Exposed to Python
	void changeExtraCollateralDamage(int iChange);

	int getExtraBombardRate() const { return m_iExtraBombardRate; }											// Exposed to Python
	void changeExtraBombardRate(int iChange);

	int getExtraEnemyHeal() const;																			// Exposed to Python
	void changeExtraEnemyHeal(int iChange);

	int getExtraNeutralHeal() const;																		// Exposed to Python
	void changeExtraNeutralHeal(int iChange);

	int getExtraFriendlyHeal() const;																		// Exposed to Python
	void changeExtraFriendlyHeal(int iChange);

	int getSameTileHeal() const;																			// Exposed to Python
	void changeSameTileHeal(int iChange);

	int getAdjacentTileHeal() const;																		// Exposed to Python
	void changeAdjacentTileHeal(int iChange);

	int getExtraCombatPercent() const { return m_iExtraCombatPercent; }										// Exposed to Python
	void changeExtraCombatPercent(int iChange);

	int getExtraCityAttackPercent() const { return m_iExtraCityAttackPercent; }								// Exposed to Python
	void changeExtraCityAttackPercent(int iChange);

	int getExtraCityDefensePercent() const { return m_iExtraCityDefensePercent; }							// Exposed to Python
	void changeExtraCityDefensePercent(int iChange);

	int getExtraHillsAttackPercent() const { return m_iExtraHillsAttackPercent; }							// Exposed to Python
	void changeExtraHillsAttackPercent(int iChange);

	int getExtraHillsDefensePercent() const { return m_iExtraHillsDefensePercent; }							// Exposed to Python
	void changeExtraHillsDefensePercent(int iChange);

	int getRevoltProtection() const;																		// Exposed to Python
	void changeRevoltProtection(int iChange);

	int getCollateralDamageProtection() const;																// Exposed to Python
	void changeCollateralDamageProtection(int iChange);

	int getPillageChange() const;																			// Exposed to Python
	void changePillageChange(int iChange);

	int getUpgradeDiscount() const;																			// Exposed to Python
	void changeUpgradeDiscount(int iChange);

	int getExperiencePercent() const;																		// Exposed to Python
	void changeExperiencePercent(int iChange);

	int getKamikazePercent() const;																			// Exposed to Python
	void changeKamikazePercent(int iChange);

	DllExport DirectionTypes getFacingDirection(bool bCheckLineOfSightProperty) const;
	void setFacingDirection(DirectionTypes facingDirection);
	void rotateFacingDirectionClockwise();
	void rotateFacingDirectionCounterClockwise();

	DllExport bool isSuicide() const;																		// Exposed to Python
	int getDropRange() const { return m_pUnitInfo->getDropRange(); }

	//return m_bMadeAttack;
	// advc.164: Keep the boolean interface in place
	bool isMadeAttack() const { return (m_iMadeAttacks > 0); } // Exposed to Python
	void setMadeAttack(bool bNewValue);																		// Exposed to Python
	bool isMadeAllAttacks() const; // advc.164

	bool isMadeInterception() const { return m_bMadeInterception; } // Exposed to Python
	void setMadeInterception(bool bNewValue);																// Exposed to Python

	bool isPromotionReadyExternal() const; // advc.002e: exported through .def file
	bool isPromotionReady() const { return m_bPromotionReady; }												// Exposed to Python
	void setPromotionReady(bool bNewValue);																	// Exposed to Python
	void testPromotionReady();

	bool isDelayedDeath() const { return m_bDeathDelay; }
	void startDelayedDeath();
	bool doDelayedDeath();

	bool isCombatFocus() const;

	DllExport bool isInfoBarDirty() const;
	DllExport void setInfoBarDirty(bool bNewValue);

	bool isBlockading() const { return m_bBlockading; }
	void setBlockading(bool bNewValue);
	void collectBlockadeGold();

	DllExport PlayerTypes getOwner() const { return m_eOwner; } // advc.inl: was "getOwnerINLINE"								// Exposed to Python
	DllExport PlayerTypes getVisualOwner(TeamTypes eForTeam = NO_TEAM) const;								// Exposed to Python
	// advc.inl: Split this function up so that part of it can be inlined
	PlayerTypes getCombatOwner(TeamTypes eForTeam, CvPlot const& kPlot) const { return (isAlwaysHostile() ? getCombatOwner_bulk(eForTeam, kPlot) : getOwner()); } // Exposed to Python
	// advc (for convenience)
	PlayerTypes getCombatOwner(TeamTypes eForTeam) const { return getCombatOwner(eForTeam, getPlot()); }
	DllExport TeamTypes getTeam() const;																	// Exposed to Python
	// <advc>
	bool isActiveOwned() const { return (GC.getInitCore().getActivePlayer() == getOwner()); }
	bool isActiveTeam() const { return (GC.getInitCore().getActiveTeam() == getTeam()); } // </advc>

	PlayerTypes getCapturingPlayer() const;
	void setCapturingPlayer(PlayerTypes eNewValue);

	DllExport const UnitTypes getUnitType() const { return m_eUnitType; }									// Exposed to Python
	CvUnitInfo& getUnitInfo() const { return *m_pUnitInfo; }
	UnitClassTypes getUnitClassType() const { return m_pUnitInfo->getUnitClassType(); } // Exposed to Python

	DllExport const UnitTypes getLeaderUnitType() const { return m_eLeaderUnitType; }
	void setLeaderUnitType(UnitTypes leaderUnitType);

	DllExport CvUnit* getCombatUnit() const;
	void setCombatUnit(CvUnit* pUnit, bool bAttacking = false);
	bool showSiegeTower(CvUnit* pDefender) const; // K-Mod

	CvUnit const* getTransportUnit() const;																	// Exposed to Python
	CvUnit* getTransportUnit(); // advc
	// advc.test: (Should perhaps simply turn m_transportUnit into a CvUnit pointer.)
	//FAssert((getTransportUnit() == NULL) == (m_transportUnit.iID == NO_PLAYER));
	// advc.opt: avoid ::getUnit call
	bool isCargo() const { return (m_transportUnit.iID != NO_PLAYER); } // Exposed to Python
	void setTransportUnit(CvUnit* pTransportUnit);															// Exposed to Python

	int getExtraDomainModifier(DomainTypes eDomain) const { return m_aiExtraDomainModifier.get(eDomain); } // Exposed to Python
	void changeExtraDomainModifier(DomainTypes eDomain, int iChange);

	DllExport const CvWString getName(uint uiForm = 0) const;												// Exposed to Python
	CvWString const getReplayName() const; // advc.106
	const wchar* getNameKey() const;																		// Exposed to Python
	wchar const* getNameKeyNoGG() const; // advc.004u
	const CvWString& getNameNoDesc() const;																	// Exposed to Python
	void setName(const CvWString szNewValue);																// Exposed to Python

	// Script data needs to be a narrow string for pickling in Python
	std::string getScriptData() const;																		// Exposed to Python
	void setScriptData(std::string szNewValue);																// Exposed to Python

	int getTerrainDoubleMoveCount(TerrainTypes eTerrain) const { return m_aiTerrainDoubleMoveCount.get(eTerrain); }
	bool isTerrainDoubleMove(TerrainTypes eTerrain) const { return (getTerrainDoubleMoveCount(eTerrain) > 0); } // Exposed to Python
	void changeTerrainDoubleMoveCount(TerrainTypes eTerrain, int iChange);

	int getFeatureDoubleMoveCount(FeatureTypes eFeature) const { return m_aiFeatureDoubleMoveCount.get(eFeature); }
	bool isFeatureDoubleMove(FeatureTypes eFeature) const { return (getFeatureDoubleMoveCount(eFeature) > 0); } // Exposed to Python
	void changeFeatureDoubleMoveCount(FeatureTypes eFeature, int iChange);

	int getExtraTerrainAttackPercent(TerrainTypes eTerrain) const { return m_aiExtraTerrainAttackPercent.get(eTerrain); } // Exposed to Python
	void changeExtraTerrainAttackPercent(TerrainTypes eTerrain, int iChange);
	int getExtraTerrainDefensePercent(TerrainTypes eTerrain) const { return m_aiExtraTerrainDefensePercent.get(eTerrain); } // Exposed to Python
	void changeExtraTerrainDefensePercent(TerrainTypes eTerrain, int iChange);
	int getExtraFeatureAttackPercent(FeatureTypes eFeature) const { return m_aiExtraFeatureAttackPercent.get(eFeature); } // Exposed to Python
	void changeExtraFeatureAttackPercent(FeatureTypes eFeature, int iChange);
	int getExtraFeatureDefensePercent(FeatureTypes eFeature) const { return m_aiExtraFeatureDefensePercent.get(eFeature); } // Exposed to Python
	void changeExtraFeatureDefensePercent(FeatureTypes eFeature, int iChange);

	int getExtraUnitCombatModifier(UnitCombatTypes eUnitCombat) const { return m_aiExtraUnitCombatModifier.get(eUnitCombat); } // Exposed to Python
	void changeExtraUnitCombatModifier(UnitCombatTypes eUnitCombat, int iChange);

	bool canAcquirePromotion(PromotionTypes ePromotion) const;												// Exposed to Python
	bool canAcquirePromotionAny() const;																	// Exposed to Python
	bool isPromotionValid(PromotionTypes ePromotion) const;													// Exposed to Python
	bool isHasPromotion(PromotionTypes ePromotion) const { return m_abHasPromotion.get(ePromotion); } // Exposed to Python
	void setHasPromotion(PromotionTypes ePromotion, bool bNewValue);										// Exposed to Python

	int getSubUnitCount() const;
	DllExport int getSubUnitsAlive() const;
	int getSubUnitsAlive(int iDamage) const;

	bool isTargetOf(CvUnit const& kAttacker) const;
	bool isEnemy(TeamTypes eTeam, CvPlot const& kPlot) const;
	// advc.opt: Instead of allowing pPlot==NULL
	__inline bool isEnemy(TeamTypes eTeam) const { return isEnemy(eTeam, getPlot()); }
	// advc.opt: Instead of allowing eTeam==NO_TEAM above
	bool isEnemy(CvPlot const& kPlot) const;
	// (advc: isPotentialEnemy moved to CvUnitAI)

	bool canRangeStrike() const;
	bool canRangeStrikeAt(const CvPlot* pPlot, int iX, int iY) const;
	bool rangeStrike(int iX, int iY);

	int getTriggerValue(EventTriggerTypes eTrigger, const CvPlot* pPlot, bool bCheckPlot) const;
	bool canApplyEvent(EventTypes eEvent) const;
	void applyEvent(EventTypes eEvent);

	int getImmobileTimer() const;																			// Exposed to Python
	void setImmobileTimer(int iNewValue);																	// Exposed to Python
	void changeImmobileTimer(int iChange);

	// (advc: potentialWarAction has become CvUnitAI::AI_mayAttack(CvPlot const&))
	bool willRevealAnyPlotFrom(CvPlot const& kFrom) const;
	bool isAlwaysHostile(CvPlot const& kPlot) const;
	// advc.opt: Faster version for unknown plot
	bool isAlwaysHostile() const { return m_pUnitInfo->isAlwaysHostile(); }

	bool verifyStackValid();

	DllExport const CvArtInfoUnit* getArtInfo(int i, EraTypes eEra) const;									// Exposed to Python
	DllExport const TCHAR* getButton() const;																// Exposed to Python
	DllExport int getGroupSize() const;
	DllExport int getGroupDefinitions() const;
	DllExport int getUnitGroupRequired(int i) const;
	DllExport bool isRenderAlways() const;
	DllExport float getAnimationMaxSpeed() const;
	DllExport float getAnimationPadTime() const;
	DllExport const char* getFormationType() const;
	DllExport bool isMechUnit() const;
	DllExport bool isRenderBelowWater() const;
	DllExport int getRenderPriority(UnitSubEntityTypes eUnitSubEntity, int iMeshGroupType, int UNIT_MAX_SUB_TYPES) const;

	DllExport bool shouldShowEnemyGlow(TeamTypes eForTeam) const;
	DllExport bool shouldShowFoundBorders() const;

	DllExport void cheat(bool bCtrl, bool bAlt, bool bShift);
	DllExport float getHealthBarModifier() const;
	DllExport void getLayerAnimationPaths(std::vector<AnimationPathTypes>& aAnimationPaths) const;
	DllExport int getSelectionSoundScript() const;

	bool isWorker() const; // advc.154  (Exposed to Python)

	// Lead From Behind (UncutDragon, edited for K-Mod): START
	// <!-- custom: hoisted from multiline signature between `pAttacker` and `pBestDefenderRank` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	bool isBetterDefenderThan(const CvUnit* pDefender, const CvUnit* pAttacker, int* pBestDefenderRank, bool bPreferUnowned = false) const; // advc.061
	int LFBgetAttackerRank(const CvUnit* pDefender, int& iUnadjustedRank) const;
	int LFBgetDefenderRank(const CvUnit* pAttacker) const;
	// unprotected by K-Mod. (I want to use the LFB value for some AI stuff)
	int LFBgetDefenderOdds(const CvUnit* pAttacker) const;
	int LFBgetValueAdjustedOdds(int iOdds, bool bDefender) const;
	int LFBgetRelativeValueRating() const;
	int LFGgetDefensiveValueAdjustment() const; // K-Mod
	bool LFBisBetterDefenderThan(const CvUnit* pDefender, const CvUnit* pAttacker, int* pBestDefenderRank) const;
	// (advc: LFBgetDefenderCombatOdds deleted, use 1000 minus calculateCombatOdds instead.)
	// Lead From Behind: END

	// <advc.003u>
	// virtual for FFreeListTrashArray
	virtual void read(FDataStreamBase* pStream);
	virtual void write(FDataStreamBase* pStream);
	//return *static_cast<CvUnitAI*>(const_cast<CvUnit*>(this));
	/* The above won't work in an inline function b/c the compiler doesn't know that CvUnitAI is derived from CvUnit */
	CvUnitAI& AI() { return *reinterpret_cast<CvUnitAI*>(this); }
	//return *static_cast<CvUnitAI const*>(this);
	CvUnitAI const& AI() const { return *reinterpret_cast<CvUnitAI const*>(this); }
	/*  Keep one pure virtual function to make the class abstract; remove all
		the others - the EXE doesn't call them. */ // </advc.003u>
	virtual UnitAITypes AI_getUnitAIType() const = 0;

protected:
	// <advc.003u>
	CvUnit();
	/*  Subclasses need to call these two init functions; not called by base.
		May also want to override them. */
	virtual void init(int iID, UnitTypes eUnit, PlayerTypes eOwner, int iX, int iY, DirectionTypes eFacingDirection);
	virtual void finalizeInit(); // </advc.003u>
	// <!-- custom: Centralize successful routine and forced scrap logging/deletion.
	// Forced callers must establish their narrow game-rule cleanup and safety conditions before bypassing generic SAS preservation. See KI#331. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	void scrapInternal();

	int m_iID;
	int m_iGroupID;
	// <advc> Moved up for easier access in debugger
	PlayerTypes m_eOwner;
	CvUnitInfo *m_pUnitInfo; // </advc>
	int m_iX;
	int m_iY;
	int m_iLastMoveTurn;
	int m_iReconX;
	int m_iReconY;
	int m_iLastReconTurn; // advc.029
	int m_iGameTurnCreated;
	int m_iHotKeyNumber;
	int m_iDamage;
	int m_iMoves;
	int m_iExperience;
	int m_iLevel;
	int m_iCargo;
	int m_iCargoCapacity;
	int m_iAttackPlotX;
	int m_iAttackPlotY;
	// <advc.048c>
	int m_iAttackOdds;
	int m_iPreCombatHP; // </advc.048c>
	int m_iCombatTimer;
	int m_iCombatFirstStrikes;
	//int m_iCombatDamage; // advc.003j: unused
	int m_iFortifyTurns;
	int m_iBlitzCount;
	int m_iAmphibCount;
	int m_iRiverCount;
	int m_iEnemyRouteCount;
	int m_iAlwaysHealCount;
	int m_iHillsDoubleMoveCount;
	int m_iImmuneToFirstStrikesCount;
	int m_iExtraVisibilityRange;
	int m_iExtraMoves;
	int m_iExtraMoveDiscount;
	int m_iExtraAirRange;
	int m_iExtraIntercept;
	int m_iExtraEvasion;
	int m_iExtraFirstStrikes;
	int m_iExtraChanceFirstStrikes;
	int m_iExtraWithdrawal;
	int m_iExtraCollateralDamage;
	int m_iExtraBombardRate;
	int m_iExtraEnemyHeal;
	int m_iExtraNeutralHeal;
	int m_iExtraFriendlyHeal;
	int m_iSameTileHeal;
	int m_iAdjacentTileHeal;
	int m_iExtraCombatPercent;
	int m_iExtraCityAttackPercent;
	int m_iExtraCityDefensePercent;
	int m_iExtraHillsAttackPercent;
	int m_iExtraHillsDefensePercent;
	int m_iRevoltProtection;
	int m_iCollateralDamageProtection;
	int m_iPillageChange;
	int m_iUpgradeDiscount;
	int m_iExperiencePercent;
	int m_iKamikazePercent;
	int m_iBaseCombat;
	DirectionTypes m_eFacingDirection;
	int m_iImmobileTimer;

	//bool m_bMadeAttack;
	int m_iMadeAttacks; // advc.164
	// advc.opt: Bitfields - since we have exactly 8 booleans ...
	bool m_bMadeInterception:1;
	bool m_bPromotionReady:1;
	bool m_bDeathDelay:1;
	bool m_bCombatFocus:1;
	bool m_bInfoBarDirty:1;
	bool m_bBlockading:1;
	bool m_bAirCombat:1;
	bool m_bFlatMovement:1; // advc.opt

	PlayerTypes m_eCapturingPlayer;
	UnitTypes m_eUnitType;
	UnitTypes m_eLeaderUnitType;
	// <advc.opt>
	CvArea* m_pArea;
	CvPlot* m_pPlot; // </advc.opt>
	// advc (note): These aren't CvUnit pointers b/c of the order of deserialization
	IDInfo m_combatUnit;
	IDInfo m_transportUnit;
	/*	advc.opt (tbd.): string objects take up 28 byte each; replace with pointers.
		May also want to replace most of the ints with short. That would cut
		the total size of this class in half. */
	CvWString m_szName;
	CvString m_szScriptData;

	// <advc.enum>
	ArrayEnumMap<PromotionTypes,bool> m_abHasPromotion;
	ArrayEnumMap<DomainTypes,int,short> m_aiExtraDomainModifier;
	ArrayEnumMap<TerrainTypes,int,short> m_aiTerrainDoubleMoveCount;
	ArrayEnumMap<TerrainTypes,int,short> m_aiExtraTerrainAttackPercent;
	ArrayEnumMap<TerrainTypes,int,short> m_aiExtraTerrainDefensePercent;
	ArrayEnumMap<FeatureTypes,int,short> m_aiFeatureDoubleMoveCount;
	ArrayEnumMap<FeatureTypes,int,short> m_aiExtraFeatureAttackPercent;
	ArrayEnumMap<FeatureTypes,int,short> m_aiExtraFeatureDefensePercent;
	ArrayEnumMap<UnitCombatTypes,int,short> m_aiExtraUnitCombatModifier;
	// </advc.enum>

	PlayerTypes getCombatOwner_bulk(TeamTypes eForTeam, CvPlot const& kPlot) const; // advc
	// <advc.111>
	bool pillageImprovement();
	bool pillageRoute();
	// </advc.111>
	bool canAdvance(const CvPlot* pPlot, int iThreshold) const;
	void collateralCombat(const CvPlot* pPlot, CvUnit const* pSkipUnit = NULL);
	void flankingStrikeCombat(const CvPlot* pPlot, int iAttackerStrength, int iAttackerFirepower, int iDefenderOdds, int iDefenderDamage, CvUnit const* pSkipUnit = NULL);

	bool interceptTest(CvPlot const& kPlot, /* advc.004c: */ IDInfo* pInterceptorID = NULL);
	CvUnit* airStrikeTarget(CvPlot const& kPlot) const;
	bool airStrike(CvPlot& kPlot, /* advc.004c: */ bool* pbIntercepted = NULL);

	int planBattle(CvBattleDefinition& kBattle, const std::vector<int>& combat_log) const; // K-Mod
	int computeUnitsToDie(const CvBattleDefinition & kDefinition, bool bRanged, BattleUnitTypes iUnit) const;
	bool verifyRoundsValid(const CvBattleDefinition & battleDefinition) const;
	void increaseBattleRounds(CvBattleDefinition & battleDefinition) const;
	int computeWaveSize(bool bRangedRound, int iAttackerMax, int iDefenderMax) const;
	bool isCombatVisible(const CvUnit* pDefender, bool bSeaPatrol = false) const; // advc.004k
	//void resolveCombat(CvUnit* pDefender, CvPlot* pPlot, CvBattleDefinition& kBattle);
	void resolveCombat(CvUnit* pDefender, CvPlot* pPlot, bool bVisible); // K-Mod
	void addAttackSuccessMessages(CvUnit const& kDefender, bool bFought) const; // advc.010
	void addDefenseSuccessMessages(CvUnit const& kDefender) const; // advc
	void addWithdrawalMessages(CvUnit const& kDefender) const; // advc
	// <advc.048c>
	void setHasBeenDefendedAgainstMessage(CvWString& kBuffer, CvUnit const& kDefender, int iAttackSuccess) const; // </advc.048c>
	bool suppressStackAttackSound(CvUnit const& kDefender) const; // advc.002l
	void resolveAirCombat(CvUnit* pInterceptor, CvPlot* pPlot, CvAirMissionDefinition& kBattle);
	void checkRemoveSelectionAfterAttack();
	void updateFlatMovement();
// <advc.003u>
private:
	void uninitEntity(); // I don't think subclasses should ever call this
	// </advc.003u>
};

// advc: Moved from the beginning of the file
struct CombatDetails											// Exposed to Python
{
	int iExtraCombatPercent;
	/*	advc.313 (note): Not sure what these suffixes are supposed to abbreviate.
		I'll be assuming "T" for "This", meaning the defender, and then:
		"TA" = "This is Animal", "AA" = "Attacker is Animal",
		"TB" = "This is Barbarian", "AB" = "Attacker is Barbarian". */
	int iAnimalCombatModifierTA;
	int iAIAnimalCombatModifierTA;
	int iAnimalCombatModifierAA;
	int iAIAnimalCombatModifierAA;
	int iBarbarianCombatModifierTB;
	int iAIBarbarianCombatModifierTB;
	int iBarbarianCombatModifierAB;
	int iAIBarbarianCombatModifierAB;
	// <advc.313>
	int iBarbarianCityAttackModifier;
	int iSeaBarbarianModifierTB;
	int iSeaBarbarianModifierAB; // </advc.313>
	int iPlotDefenseModifier;
	int iFortifyModifier;
	int iCityDefenseModifier;
	int iHillsAttackModifier;
	int iHillsDefenseModifier;
	int iFeatureAttackModifier;
	int iFeatureDefenseModifier;
	int iTerrainAttackModifier;
	int iTerrainDefenseModifier;
	int iCityAttackModifier;
	int iDomainDefenseModifier;
	//int iCityBarbarianDefenseModifier; // advc.313: replaced above
	int iClassDefenseModifier;
	int iClassAttackModifier;
	int iCombatModifierT;
	int iCombatModifierA;
	int iDomainModifierA;
	int iDomainModifierT;
	int iAnimalCombatModifierA;
	int iAnimalCombatModifierT;
	int iRiverAttackModifier;
	int iAmphibAttackModifier;
	int iKamikazeModifier;
	int iModifierTotal;
	int iBaseCombatStr;
	int iCombat;
	int iMaxCombatStr;
	int iCurrHitPoints;
	int iMaxHitPoints;
	int iCurrCombatStr;
	PlayerTypes eOwner;
	PlayerTypes eVisualOwner;
	std::wstring sUnitName;
	// advc:
	void reset(PlayerTypes eOwner, PlayerTypes eVisualOwner, std::wstring sUnitName)
	{
		/*	Not nice - but we mustn't zero the string, and we don't want to
			repeat all the int members. */
		ZeroMemory(this, 41 * sizeof(int));
		BOOST_STATIC_ASSERT(sizeof(CombatDetails) == 41 * sizeof(int) + 2 * sizeof(PlayerTypes) + sizeof(std::wstring));
		this->eOwner = eOwner;
		this->eVisualOwner = eVisualOwner;
		this->sUnitName = sUnitName;
	}
};

#endif
