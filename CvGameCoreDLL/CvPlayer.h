#pragma once

// player.h

#ifndef CIV4_PLAYER_H
#define CIV4_PLAYER_H

#include "CvPlotGroup.h"

class CvTalkingHeadMessage;
class CvDiploParameters;
class CvPopupInfo;
class CvEventTriggerInfo;
class CvPlayerRecord; // K-Mod
class AdvCiv4lert; // advc.210
class CvArea;
class CvPlayerAI;
// <advc.003u> For FFreeListTrashArrays
class CvCity; class CvCityAI;
class CvUnit; class CvUnitAI;
class CvSelectionGroup; class CvSelectionGroupAI;
// </advc.003u>
class CvCivilization; // advc.003w
typedef std::list<CvTalkingHeadMessage> CvMessageQueue;
typedef std::list<CvPopupInfo*> CvPopupQueue;
typedef std::list<CvDiploParameters*> CvDiploQueue;
typedef stdext::hash_map<int, int> CvTurnScoreMap;
typedef stdext::hash_map<EventTypes, EventTriggeredData> CvEventMap;
typedef std::vector< std::pair<UnitCombatTypes, PromotionTypes> > UnitCombatPromotionArray;
typedef std::vector< std::pair<UnitClassTypes, PromotionTypes> > UnitClassPromotionArray;
typedef std::vector< std::pair<CivilizationTypes, LeaderHeadTypes> > CivLeaderArray;

// <advc.003u>
#ifndef GET_PLAYER // Prefer the definition in CvPlayerAI.h
#define GET_PLAYER(x) CvPlayer::getPlayer(x)
#endif // </advc.003u>

class CvPlayer /* advc.003e: */ : private boost::noncopyable
{
public:
	// <advc.003u>
	static inline CvPlayer& getPlayer(PlayerTypes ePlayer)
	{
		FAssertBounds(0, MAX_PLAYERS, ePlayer);
		// Needs to be inline and I don't want to include CvPlayerAI.h here
		return *reinterpret_cast<CvPlayer*>(m_aPlayers[ePlayer]);
	}
	// static functions moved from CvPlayerAI:
	static void initStatics();
	static void freeStatics();
	static bool areStaticsInitialized() { return (m_aPlayers != NULL); }
	// </advc.003u>

	explicit CvPlayer(PlayerTypes eID);
	virtual ~CvPlayer();
protected: // advc.003u: Can't easily move these past AI_makeAssignWorkDirty (the EXE relies on the order)
	virtual void read(FDataStreamBase* pStream);
	virtual void write(FDataStreamBase* pStream);
public:
	// advc.003u: Keep one pure virtual function so that this class is abstract
	virtual void AI_makeAssignWorkDirty() = 0;

	DllExport void init(PlayerTypes eID);
	DllExport void setupGraphical();
	DllExport void reset(PlayerTypes eID = NO_PLAYER, bool bConstructorCall = false);

	void initFreeState();
	void initFreeUnits();
	void addFreeUnitAI(UnitAITypes eUnitAI, int iCount);
	void addFreeUnit(UnitTypes eUnit, UnitAITypes eUnitAI = NO_UNITAI);

	int startingPlotRange() const;																																									// Exposed to Python
	bool startingPlotWithinRange(CvPlot const& kPlot, PlayerTypes ePlayer, int iRange, int iPass) const;									// Exposed to Python
	int startingPlotDistanceFactor(CvPlot const& kPlot, PlayerTypes ePlayer, int iRange) const;
	//int findStartingArea() const;
	std::vector<std::pair<int,int> > findStartingAreas() const; // dlph.35
	// advc.027: New auxiliary function (public only for Debug mode info)
	int coastRiverStartingAreaScore(CvArea const& a) const;
	CvPlot* findStartingPlot(bool bRandomize = false);																																									// Exposed to Python

	CvPlotGroup* initPlotGroup(CvPlot* pPlot);

	CvCity* initCity(int iX, int iY, bool bBumpUnits, bool bUpdatePlotGroups,																																// Exposed to Python
			int iOccupationTimer = 0); // advc.ctr
	void acquireCity(CvCity* pCity, bool bConquest, bool bTrade, bool bUpdatePlotGroups,																							// Exposed to Python
			bool bPeaceDeal = false); // advc.ctr
	void killCities();													// Exposed to Python
	CvWString getNewCityName() const;																																								// Exposed to Python
	void getCivilizationCityName(CvWString& szBuffer, CivilizationTypes eCivilization) const;
	bool isCityNameValid(CvWString& szName, bool bTestDestroyed = true) const;

	CvUnit* initUnit(UnitTypes eUnit, int iX, int iY, UnitAITypes eUnitAI = NO_UNITAI,								// Exposed to Python
			// advc.003u: Set this default here rather than in CvUnit::init
			DirectionTypes eFacingDirection = DIRECTION_SOUTH);
	void disbandUnit(bool bAnnounce);																																					// Exposed to Python
	void killUnits();																																													// Exposed to Python

	CvSelectionGroup* cycleSelectionGroups(CvUnit* pUnit, bool bForward, bool bWorkers, bool* pbWrap);

	bool hasTrait(TraitTypes eTrait) const;																																			// Exposed to Python
	// BETTER_BTS_AI_MOD, 12/30/08, jdog5000: START
	void initInGame(PlayerTypes eID);
	void resetPlotAndCityData();
	// BETTER_BTS_AI_MOD: END
	// CHANGE_PLAYER, 12/30/08, jdog5000: START
	/*void clearTraitBonuses();
	void addTraitBonuses();*/
	void processTraits(int iChange); // advc.003q: Replacing the above
	void changePersonalityType();
	void resetCivTypeEffects(/* advc.003q: */ bool bInit);
	void changeLeader(LeaderHeadTypes eNewLeader);
	void changeCiv(CivilizationTypes eNewCiv);
	void setIsHuman(bool bNewValue);
	// CHANGE_PLAYER: END
	// AI_AUTO_PLAY_MOD, 07/09/08, jdog5000: START
	void setHumanDisabled(bool bNewVal);
	bool isHumanDisabled() /* advc.127: */ const;	// and exposed to Python
	bool isSpectator() const; // advc.127
	bool isAutoPlayJustEnded() const;		// advc.127: exposed to Python
	// AI_AUTO_PLAY_MOD: END
	// <advc.inl>
	DllExport inline bool isHuman() const { return m_bHuman; }																																							// Exposed to Python
	DllExport inline bool isBarbarian() const { return (m_eID == BARBARIAN_PLAYER); }																																		// Exposed to Python
	// </advc.inl>
	DllExport void updateHuman();

	/*  K-Mod note: I've changed getName, getCivilizationDescription,
		and getCivilizationShortDescription to only give accurate information
		if the active player has met this player.
		The "key" versions of those functions are unchanged. This is important
		because getNameKey and so on are used to create messages for the replay. */
	DllExport wchar const* getName(uint uiForm = 0) const																											// Exposed to Python
	{	// <advc.007> debugName function for logging
		return getName(false, uiForm);
	}
	wchar const* getName(bool bForceReveal, uint uiForm) const;
	wchar const* debugName() const { return getName(true, 0); } // </advc.007>
	// K-Mod. Player name to be used in replay
	wchar const* getReplayName(uint uiForm = 0) const;
	DllExport wchar const* getNameKey() const;																																	// Exposed to Python
	DllExport wchar const* getCivilizationDescription(uint uiForm = 0) const;																		// Exposed to Python
	wchar const* getCivilizationDescriptionKey() const;																								// Exposed to Python
	wchar const* getCivilizationShortDescription(uint uiForm = 0) const															// Exposed to Python
	{	// <advc.007> debugCivDescr function for logging
		return getCivilizationShortDescription(false, uiForm);
	}
	wchar const* getCivilizationShortDescription(bool bForceReveal, uint uiForm) const;
	wchar const* debugCivDescr() const { return getCivilizationShortDescription(true, 0); }
	// </advc.007>
	wchar const* getCivilizationShortDescriptionKey() const;																					// Exposed to Python
	wchar const* getCivilizationAdjective(uint uiForm = 0) const;																			// Exposed to Python
	wchar const* getCivilizationAdjectiveKey() const;																									// Exposed to Python
	DllExport CvWString getFlagDecal() const;																																		// Exposed to Python
	DllExport bool isWhiteFlag() const;																																					// Exposed to Python
	void setFlagDecal(CvWString const& szFlagDecal, bool bUpdate); // advc.127c
	const wchar* getStateReligionName(uint uiForm = 0) const;																					// Exposed to Python
	const wchar* getStateReligionKey() const;																													// Exposed to Python
	const CvWString getBestAttackUnitName(uint uiForm = 0) const;																								// Exposed to Python
	const CvWString getWorstEnemyName() const;																																	// Exposed to Python
	const wchar* getBestAttackUnitKey() const;																																	// Exposed to Python
	DllExport ArtStyleTypes getArtStyleType() const;																														// Exposed to Python
	const TCHAR* getUnitButton(UnitTypes eUnit) const;																														// Exposed to Python

	void doTurn();
	void doTurnUnits();

	void verifyCivics();
	void verifyStateReligion(); // dlph.10
	void verifyCityProduction(); // advc.064d
	void updatePlotGroups();

	void updateYield();
	void updateMaintenance();
	void updatePowerHealth();
	void updateExtraBuildingHappiness();
	void updateExtraBuildingHealth();
	void updateFeatureHappiness();
	void updateReligionHappiness();
	void updateExtraSpecialistYield();
	void updateCommerce(CommerceTypes eCommerce);
	void updateCommerce();
	void updateBuildingCommerce();
	void updateReligionCommerce();
	void updateCorporation();
	//void updateCityPlotYield(); // advc.003j
	void updateCitySight(bool bIncrement, bool bUpdatePlotGroups);
	void updateTradeRoutes();
	void updatePlunder(int iChange, bool bUpdatePlotGroups);

	void updateTimers();

	bool hasReadyUnit(bool bAny = false) const;
	bool hasAutoUnit() const;
	DllExport bool hasBusyUnit() const;

	// K-Mod
	bool isChoosingFreeTech() const { return m_iChoosingFreeTechCount > 0; }
	void changeChoosingFreeTechCount(int iChange) { m_iChoosingFreeTechCount += iChange; }
	// K-Mod end

	void chooseTech(int iDiscover = 0, CvWString szText = "", bool bFront = false);				// Exposed to Python

	int calculateScore(bool bFinal = false, bool bVictory = false) const;

	int findBestFoundValue() const;																																				// Exposed to Python

	int upgradeAllPrice(UnitTypes eUpgradeUnit, UnitTypes eFromUnit) const; // advc: const
	// advc.080:
	int upgradeAllXPChange(UnitTypes eUpgradeUnit, UnitTypes eFromUnit) const;

	// note: bbai added bIncludeTraining to the following two functions
	int countReligionSpreadUnits(CvArea const* pArea,																// Exposed to Python
			ReligionTypes eReligion, bool bIncludeTraining = false) const;
	int countCorporationSpreadUnits(CvArea const* pArea,															// Exposed to Python
			CorporationTypes eCorporation, bool bIncludeTraining = false) const;

	int countNumCoastalCities() const;																																		// Exposed to Python
	int countNumCoastalCitiesByArea(CvArea const& kArea) const;																									// Exposed to Python
	int countTotalCulture() const;																																				// Exposed to Python

	// advc.042: countUnimprovedBonuses, countOwnedBonuses moved to CvPlayerAI
	int countCityFeatures(FeatureTypes eFeature) const;																										// Exposed to Python
	int countNumBuildings(BuildingTypes eBuilding) const;																									// Exposed to Python
	int countNumCitiesConnectedToCapital() const;																								// Exposed to Python
	/* int countPotentialForeignTradeCities(CvArea* pIgnoreArea = NULL) const;
	int countPotentialForeignTradeCitiesConnected() const; */ // K-Mod: These functions were used exclusively for AI.  I've moved them to CvPlayerAI.
	bool doesImprovementConnectBonus(ImprovementTypes eImprovement, BonusTypes eBonus) const; // K-Mod

	DllExport bool canContact(PlayerTypes ePlayer) const																									// Exposed to Python
	{	// <advc> To match CvTeam::canContact
		return canContact(ePlayer, false);
	}
	bool canContact(PlayerTypes ePlayer, // </advc>
			bool bCheckWillingness) const; // K-Mod. this checks willingness to talk on both sides
	void contact(PlayerTypes ePlayer);																															// Exposed to Python
	DllExport void handleDiploEvent(DiploEventTypes eDiploEvent, PlayerTypes ePlayer, int iData1, int iData2);
	bool canTradeWith(PlayerTypes eWhoTo) const;																													// Exposed to Python
	bool canReceiveTradeCity() const;
	DllExport bool canTradeItem(PlayerTypes eWhoTo, TradeData item, bool bTestDenial = false) const;			// Exposed to Python
	bool canPossiblyTradeItem(PlayerTypes eWhoTo, TradeableItems eItemType) const; // advc.opt
	DllExport DenialTypes getTradeDenial(PlayerTypes eWhoTo, TradeData item) const;												// Exposed to Python
	bool canTradeNetworkWith(PlayerTypes ePlayer) const;																									// Exposed to Python
	int getNumAvailableBonuses(BonusTypes eBonus) const;																									// Exposed to Python
	int getNumTradeableBonuses(BonusTypes eBonus) const;																				// Exposed to Python
	int getNumTradeBonusImports(PlayerTypes eFromPlayer) const;																								// Exposed to Python
	bool hasBonus(BonusTypes eBonus) const;									// Exposed to Python
	// advc: Said "IncludeCancelable", but actually does the opposite.
	bool isTradingWithTeam(TeamTypes eTeam, bool bIncludeUncancelable) const;
	bool canStopTradingWithTeam(TeamTypes eTeam, bool bContinueNotTrading = false) const;																										// Exposed to Python
	void stopTradingWithTeam(TeamTypes eTeam,																										// Exposed to Python
			// <advc.130f>
			bool bDiploPenalty = true);
	bool isAnyDealTooRecentToCancel(TeamTypes eTeam) const; // </advc.130f>
	void killAllDeals();																																						// Exposed to Python

	void findNewCapital();																																					// Exposed to Python
	int getNumGovernmentCenters() const;																												// Exposed to Python

	bool canRaze(CvCity const& kCity) const;																													// Exposed to Python
	void raze(CvCity& kCity);																																				// Exposed to Python
	void disband(CvCity& kCity);																																		// Exposed to Python

	bool canReceiveGoody(CvPlot* pPlot, GoodyTypes eGoody, CvUnit* pUnit) const;													// Exposed to Python
	void receiveGoody(CvPlot* pPlot, GoodyTypes eGoody, CvUnit* pUnit,															// Exposed to Python
			bool bNoRecursion = false); // advc.314
	void doGoody(CvPlot* pPlot, CvUnit* pUnit,																											// Exposed to Python
			// advc.314: Set this when rolling an additional outcome
			GoodyTypes eTaboo = NO_GOODY);

	DllExport bool canFound(int iX, int iY, bool bTestVisible = false) const;															// Exposed to Python
	void found(int iX, int iY);																																			// Exposed to Python

	bool canTrain(UnitTypes eUnit, bool bContinue = false, bool bTestVisible = false, bool bIgnoreCost = false) const;										// Exposed to Python
	bool canConstruct(BuildingTypes eBuilding, bool bContinue = false, bool bTestVisible = false, bool bIgnoreCost = false, bool bIgnoreTech = false) const; // Exposed to Python, K-Mod added bIgnoreTech
	bool canCreate(ProjectTypes eProject, bool bContinue = false, bool bTestVisible = false) const;							// Exposed to Python
	bool canMaintain(ProcessTypes eProcess, bool bContinue = false) const;																			// Exposed to Python
	bool isProductionMaxedUnitClass(UnitClassTypes eUnitClass) const;																						// Exposed to Python
	bool isProductionMaxedBuildingClass(BuildingClassTypes eBuildingClass, bool bAcquireCity = false) const;		// Exposed to Python
	bool isProductionMaxedProject(ProjectTypes eProject) const;																									// Exposed to Python
	int getProductionNeeded(UnitTypes eUnit) const;																										// Exposed to Python
	int getProductionNeeded(BuildingTypes eBuilding) const;																						// Exposed to Python
	int getProductionNeeded(ProjectTypes eProject) const;																							// Exposed to Python
	int getProductionModifier(UnitTypes eUnit) const;
	int getProductionModifier(BuildingTypes eBuilding) const;
	int getProductionModifier(ProjectTypes eProject) const;
	double trainingModifierFromHandicap(bool bWorldClass = false) const;

	int getBuildingClassPrereqBuilding(BuildingTypes eBuilding, BuildingClassTypes ePrereqBuildingClass, int iExtra = 0) const;	// Exposed to Python
	void removeBuildingClass(BuildingClassTypes eBuildingClass);																		// Exposed to Python
	void processBuilding(BuildingTypes eBuilding, int iChange, CvArea& kArea);

	int getBuildCost(const CvPlot* pPlot, BuildTypes eBuild) const;
	bool canBuild(const CvPlot* pPlot, BuildTypes eBuild, bool bTestEra = false, bool bTestVisible = false) const;	// Exposed to Python
	RouteTypes getBestRoute(const CvPlot* pPlot = NULL) const;																						// Exposed to Python
	int getImprovementUpgradeRate() const;																													// Exposed to Python

	int calculateTotalYield(YieldTypes eYield) const;																											// Exposed to Python
	int calculateTotalExports(YieldTypes eYield) const;																										// Exposed to Python
	int calculateTotalImports(YieldTypes eYield) const;																										// Exposed to Python

	int calculateTotalCityHappiness() const;																															// Exposed to Python
	int calculateTotalCityUnhappiness() const;																														// Exposed to Python

	int calculateTotalCityHealthiness() const;																														// Exposed to Python
	int calculateTotalCityUnhealthiness() const;																													// Exposed to Python

	int calculatePollution(int iTypes = POLLUTION_ALL) const; // K-Mod, Exposed to Python
	int getGwPercentAnger() const; // K-Mod, Exposed to Python
	void setGwPercentAnger(int iNewValue); // K-Mod

	int getUnitCostMultiplier() const; // K-Mod
	int calculateUnitCost(int& iFreeUnits, int& iFreeMilitaryUnits, int& iPaidUnits,
			// K-Mod: changed iBaseUnitCost to iUnitCost
			int& iPaidMilitaryUnits, int& iUnitCost, int& iMilitaryCost, int& iExtraCost,
			int iExtraPop = 0, int iExtraUnits = 0) const; // advc.004b
	int calculateUnitCost(																																				// Exposed to Python
			int iExtraPop = 0, int iExtraUnits = 0) const; // advc.004b
	int calculateUnitSupply(int& iPaidUnits, int& iBaseSupplyCost,																// Exposed to Python
			int iExtraOutsideUnits = 0) const; // advc.004b
	int calculateUnitSupply(																																			// Exposed to Python
			int iExtraOutsideUnits = 0) const; // advc.004b
	int calculatePreInflatedCosts() const;																																// Exposed to Python
	int calculateInflationRate() const;																																		// Exposed to Python
	int calculateInflatedCosts() const;																																		// Exposed to Python

	//int calculateBaseNetGold() const; // disabled by K-Mod
	//int calculateBaseNetResearch(TechTypes eTech = NO_TECH) const;   // disabled by K-Mod (was exposed to Python)
	int calculateResearchModifier(TechTypes eTech,										   // Exposed to Python
			// <advc.910>
			int* piFromOtherKnown = NULL, int* piFromPaths = NULL,
			int* piFromTeam = NULL) const; // </advc.910>
	int calculateGoldRate() const;																																				// Exposed to Python
	int calculateResearchRate(TechTypes eTech = NO_TECH) const;																						// Exposed to Python
	int calculateTotalCommerce() const;

	bool isResearch() const;																																							// Exposed to Python
	bool canEverResearch(TechTypes eTech) const;																								// Exposed to Python
	TechTypes getDiscoveryTech(UnitTypes eUnit) const; // advc: Moved from CvGameCoreUtils
	bool canResearch(TechTypes eTech, bool bTrade = false,
			bool bFree = false, // (K-Mod, added bFree.) Exposed to Python
			// advc.126: Disables the isHasTech check
			bool bCouldResearchAgain = false) const;
	TechTypes getCurrentResearch() const;																												// Exposed to Python
	bool isCurrentResearchRepeat() const;																																	// Exposed to Python
	bool isNoResearchAvailable() const;																																		// Exposed to Python
	int getResearchTurnsLeft(TechTypes eTech, bool bOverflow) const;														// Exposed to Python
	bool canSeeResearch(PlayerTypes ePlayer, // K-Mod, Exposed to Python
			bool bCheckPoints = true) const; // advc.085
	bool canSeeDemographics(PlayerTypes ePlayer, // K-Mod, Exposed to Python
			bool bCheckPoints = true) const; // advc.085
	// advc.085:
	int espionageNeededToSee(PlayerTypes ePlayer, bool bDemographics) const;
	// advc.550e; also need it for advc.314
	bool isSignificantDiscovery(TechTypes eTech) const;

	bool isCivic(CivicTypes eCivic) const;																																// Exposed to Python
	bool canDoCivics(CivicTypes eCivic) const;																														// Exposed to Python
	bool canRevolution(CivicTypes* paeNewCivics) const;																					// Exposed to Python
	void revolution(CivicTypes* paeNewCivics, bool bForce = false);												// Exposed to Python
	int getCivicPercentAnger(CivicTypes eCivic, bool bIgnore = false) const;																										// Exposed to Python

	bool canDoReligion(ReligionTypes eReligion) const;																										// Exposed to Python
	bool canChangeReligion() const;																																				// Exposed to Python
	bool canConvert(ReligionTypes eReligion) const;																							// Exposed to Python
	void convert(ReligionTypes eReligion, bool bForce = false); // advc.001v																						// Exposed to Python
	bool hasHolyCity(ReligionTypes eReligion) const;																											// Exposed to Python
	int countHolyCities() const;																																					// Exposed to Python
	void foundReligion(ReligionTypes eReligion, ReligionTypes eSlotReligion, bool bAward);																										// Exposed to Python

	bool hasHeadquarters(CorporationTypes eCorporation) const;																											// Exposed to Python
	int countHeadquarters() const;																																					// Exposed to Python
	int countCorporations(CorporationTypes eCorporation,																// Exposed to Python
			CvArea const* pArea = 0) const; // K-Mod
	void foundCorporation(CorporationTypes eCorporation);																										// Exposed to Python

	int getCivicAnarchyLength(CivicTypes* paeNewCivics,																	// Exposed to Python
			bool bIgnoreGoldenAge = false) const; // advc.132
	int getReligionAnarchyLength(																												// Exposed to Python
			bool bIgnoreGoldenAge = false) const; // advc.132
	int unitsRequiredForGoldenAge() const;																											// Exposed to Python
	int unitsGoldenAgeCapable() const;																																		// Exposed to Python
	int unitsGoldenAgeReady() const;																														// Exposed to Python
	void killGoldenAgeUnits(CvUnit* pUnitAlive);

	int greatPeopleThreshold(bool bMilitary = false) const;																														// Exposed to Python
	int specialistYield(SpecialistTypes eSpecialist, YieldTypes eYield) const;														// Exposed to Python
	int specialistCommerce(SpecialistTypes eSpecialist, CommerceTypes eCommerce) const;										// Exposed to Python

	CvPlot* getStartingPlot() const;																																			// Exposed to Python
	void setStartingPlot(CvPlot* pNewValue, bool bUpdateStartDist);												// Exposed to Python

	int getTotalPopulation() const;																															// Exposed to Python
	int getAveragePopulation() const;																																			// Exposed to Python
	void changeTotalPopulation(int iChange);
	long getRealPopulation() const;																																				// Exposed to Python
	int getReligionPopulation(ReligionTypes eReligion) const;

	int getTotalLand() const;																																							// Exposed to Python
	void changeTotalLand(int iChange);

	int getTotalLandScored() const;																																				// Exposed to Python
	void changeTotalLandScored(int iChange);

	int getGold() const;																																				// Exposed to Python
	DllExport void setGold(int iNewValue);																													// Exposed to Python
	DllExport void changeGold(int iChange);																													// Exposed to Python

	int getGoldPerTurn() const;																																						// Exposed to Python

	int getAdvancedStartPoints() const;																																				// Exposed to Python
	void setAdvancedStartPoints(int iNewValue);																													// Exposed to Python
	void changeAdvancedStartPoints(int iChange);																													// Exposed to Python

	int getEspionageSpending(TeamTypes eAgainstTeam) const;																								// Exposed to Python
	bool canDoEspionageMission(EspionageMissionTypes eMission, PlayerTypes eTargetPlayer, const CvPlot* pPlot, int iExtraData, const CvUnit* pUnit,		// Exposed to Python
			bool bCheckPoints = true) const; // advc.085
	int getEspionageMissionBaseCost(EspionageMissionTypes eMission, PlayerTypes eTargetPlayer, const CvPlot* pPlot, int iExtraData, const CvUnit* pSpyUnit) const;
	int getEspionageMissionCost(EspionageMissionTypes eMission, PlayerTypes eTargetPlayer, const CvPlot* pPlot = NULL, int iExtraData = -1, const CvUnit* pSpyUnit = NULL) const;		// Exposed to Python
	// dlph.33/advc:
	int adjustMissionCostToTeamSize(int iBaseCost, PlayerTypes eTargetPlayer) const;
	// <advc.120d> New functions, all exposed to Python
	TechTypes getStealCostTech(PlayerTypes eTargetPlayer) const;
	bool canSeeTech(PlayerTypes eOther) const;
	bool canSpy() const;
	// </advc.120d>
	int getEspionageMissionCostModifier(EspionageMissionTypes eMission, PlayerTypes eTargetPlayer, const CvPlot* pPlot = NULL, int iExtraData = -1, const CvUnit* pSpyUnit = NULL) const;
	bool doEspionageMission(EspionageMissionTypes eMission, PlayerTypes eTargetPlayer, CvPlot* pPlot, int iExtraData, CvUnit* pUnit);

	int getEspionageSpendingWeightAgainstTeam(TeamTypes eIndex) const;																							// Exposed to Python
	void setEspionageSpendingWeightAgainstTeam(TeamTypes eIndex, int iValue);																		// Exposed to Python
	void changeEspionageSpendingWeightAgainstTeam(TeamTypes eIndex, int iChange);																// Exposed to Python

	bool canStealTech(PlayerTypes eTarget, TechTypes eTech) const;
	bool canForceCivics(PlayerTypes eTarget, CivicTypes eCivic) const;
	bool canForceReligion(PlayerTypes eTarget, ReligionTypes eReligion) const;
		// advc: 2x const CvUnit&
	bool canSpyDestroyUnit(PlayerTypes eTarget, CvUnit const& kUnit) const;
	bool canSpyBribeUnit(PlayerTypes eTarget, CvUnit const& kUnit) const;
	bool canSpyDestroyBuilding(PlayerTypes eTarget, BuildingTypes eBuilding) const;
	bool canSpyDestroyProject(PlayerTypes eTarget, ProjectTypes eProject) const;
	// advc.120d: Exposed to Python
	int getEspionageGoldQuantity(EspionageMissionTypes eMission, PlayerTypes eTargetPlayer, const CvCity* pCity) const; // K-Mod

	void doAdvancedStartAction(AdvancedStartActionTypes eAction, int iX, int iY, int iData, bool bAdd,
			int iData2 = -1); // advc.250c
	// advc: Made all the pointer params const
	int getAdvancedStartUnitCost(UnitTypes eUnit, bool bAdd, CvPlot const* pPlot = NULL) const;																													// Exposed to Python
	int getAdvancedStartCityCost(bool bAdd, CvPlot const* pPlot = NULL) const;																													// Exposed to Python
	int getAdvancedStartPopCost(bool bAdd, CvCity const* pCity = NULL) const;																													// Exposed to Python
	int getAdvancedStartCultureCost(bool bAdd, CvCity const* pCity = NULL) const;																													// Exposed to Python
	int getAdvancedStartBuildingCost(BuildingTypes eBuilding, bool bAdd, CvCity const* pCity = NULL) const;																													// Exposed to Python
	int getAdvancedStartImprovementCost(ImprovementTypes eImprovement, bool bAdd, CvPlot const* pPlot = NULL) const;																													// Exposed to Python
	int getAdvancedStartRouteCost(RouteTypes eRoute, bool bAdd, CvPlot const* pPlot = NULL) const;																													// Exposed to Python
	int getAdvancedStartTechCost(TechTypes eTech, bool bAdd) const;																													// Exposed to Python
	int getAdvancedStartVisibilityCost(bool bAdd, CvPlot const* pPlot = NULL) const;																													// Exposed to Python

	int getGoldenAgeTurns() const;																															// Exposed to Python
	bool isGoldenAge() const;																																		// Exposed to Python
	void changeGoldenAgeTurns(int iChange);																													// Exposed to Python
	int getGoldenAgeLength() const;

	int getNumUnitGoldenAges() const;																																			// Exposed to Python
	void changeNumUnitGoldenAges(int iChange);																											// Exposed to Python

	int getAnarchyTurns() const;																																					// Exposed to Python
	bool isAnarchy() const;																																			// Exposed to Python
	void changeAnarchyTurns(int iChange);																														// Exposed to Python

	int getStrikeTurns() const;																																						// Exposed to Python
	void changeStrikeTurns(int iChange);

	int getMaxAnarchyTurns() const;																																				// Exposed to Python
	void updateMaxAnarchyTurns();

	int getAnarchyModifier() const;																																				// Exposed to Python
	void changeAnarchyModifier(int iChange);

	int getGoldenAgeModifier() const;																																				// Exposed to Python
	void changeGoldenAgeModifier(int iChange);

	int getHurryModifier() const;																																					// Exposed to Python
	void changeHurryModifier(int iChange);

	void createGreatPeople(UnitTypes eGreatPersonUnit, bool bIncrementThreshold, bool bIncrementExperience, int iX, int iY);

	int getGreatPeopleCreated() const;																																		// Exposed to Python
	void incrementGreatPeopleCreated();

	int getGreatGeneralsCreated() const;																																		// Exposed to Python
	void incrementGreatGeneralsCreated();

	int getGreatPeopleThresholdModifier() const;																													// Exposed to Python
	void changeGreatPeopleThresholdModifier(int iChange);

	int getGreatGeneralsThresholdModifier() const;																													// Exposed to Python
	void changeGreatGeneralsThresholdModifier(int iChange);

	int getGreatPeopleRateModifier() const;																																// Exposed to Python
	void changeGreatPeopleRateModifier(int iChange);

	int getGreatGeneralRateModifier() const;																																// Exposed to Python
	void changeGreatGeneralRateModifier(int iChange);

	int getDomesticGreatGeneralRateModifier() const;																																// Exposed to Python
	void changeDomesticGreatGeneralRateModifier(int iChange);

	int getStateReligionGreatPeopleRateModifier() const;																									// Exposed to Python
	void changeStateReligionGreatPeopleRateModifier(int iChange);

	int getMaxGlobalBuildingProductionModifier() const;																										// Exposed to Python
	void changeMaxGlobalBuildingProductionModifier(int iChange);

	int getMaxTeamBuildingProductionModifier() const;																											// Exposed to Python
	void changeMaxTeamBuildingProductionModifier(int iChange);

	int getMaxPlayerBuildingProductionModifier() const;																										// Exposed to Python
	void changeMaxPlayerBuildingProductionModifier(int iChange);

	int getFreeExperience() const;																																				// Exposed to Python
	void changeFreeExperience(int iChange);

	int getFeatureProductionModifier() const;																															// Exposed to Python
	void changeFeatureProductionModifier(int iChange);

	int getWorkerSpeedModifier() const;																																		// Exposed to Python
	void changeWorkerSpeedModifier(int iChange);
	int getWorkRate(BuildTypes eBuild) const; // advc.011c

	int getImprovementUpgradeRateModifier() const;																									// Exposed to Python
	void changeImprovementUpgradeRateModifier(int iChange);

	int getMilitaryProductionModifier() const;																											// Exposed to Python
	void changeMilitaryProductionModifier(int iChange);

	int getSpaceProductionModifier() const;																																// Exposed to Python
	void changeSpaceProductionModifier(int iChange);

	int getCityDefenseModifier() const;																																		// Exposed to Python
	void changeCityDefenseModifier(int iChange);

	int getNumNukeUnits() const;																																					// Exposed to Python
	void changeNumNukeUnits(int iChange);

	int getNumOutsideUnits() const;																																				// Exposed to Python
	void changeNumOutsideUnits(int iChange);

	int getBaseFreeUnits() const;																																					// Exposed to Python
	void changeBaseFreeUnits(int iChange);

	int getBaseFreeMilitaryUnits() const;																																	// Exposed to Python
	void changeBaseFreeMilitaryUnits(int iChange);

	int getFreeUnitsPopulationPercent() const;																														// Exposed to Python
	void changeFreeUnitsPopulationPercent(int iChange);

	int getFreeMilitaryUnitsPopulationPercent() const;																										// Exposed to Python
	void changeFreeMilitaryUnitsPopulationPercent(int iChange);

	int getTypicalUnitValue(UnitAITypes eUnitAI, DomainTypes eDomain = NO_DOMAIN) const; // K-Mod

	// K-Mod note: GoldPerUnit and GoldPerMilitaryUnit are now in units of 1/100 gold.
	int getGoldPerUnit() const;																																								// Exposed to Python
	void changeGoldPerUnit(int iChange);

	int getGoldPerMilitaryUnit() const;																																				// Exposed to Python
	void changeGoldPerMilitaryUnit(int iChange);

	int getExtraUnitCost() const;																																							// Exposed to Python
	void changeExtraUnitCost(int iChange);

	int getNumMilitaryUnits() const;																																					// Exposed to Python
	void changeNumMilitaryUnits(int iChange);

	int getHappyPerMilitaryUnit() const;																																			// Exposed to Python
	void changeHappyPerMilitaryUnit(int iChange);
	// <advc.912c>
	int getLuxuryModifier() const;																																			// Exposed to Python
	void changeLuxuryModifier(int iChange); // </advc.912c>
	int getMilitaryFoodProductionCount() const;
	bool isMilitaryFoodProduction() const;																																		// Exposed to Python
	void changeMilitaryFoodProductionCount(int iChange);

	int getHighestUnitLevel() const;																																					// Exposed to Python
	void setHighestUnitLevel(int iNewValue);

	int getConscriptCount() const;																																						// Exposed to Python
	void setConscriptCount(int iNewValue);																															// Exposed to Python
	void changeConscriptCount(int iChange);																															// Exposed to Python

	int getMaxConscript() const;																																		// Exposed to Python
	void changeMaxConscript(int iChange);

	int getOverflowResearch() const;																																// Exposed to Python
	void setOverflowResearch(int iNewValue);																														// Exposed to Python
	void changeOverflowResearch(int iChange);																														// Exposed to Python

	/*int getNoUnhealthyPopulationCount() const;
	bool isNoUnhealthyPopulation() const; // Exposed to Python
	void changeNoUnhealthyPopulationCount(int iChange);*/ // BtS
	// K-Mod, 27/dec/10: replaced with UnhealthyPopulationModifier
	int getUnhealthyPopulationModifier() const; // Exposed to Python
	void changeUnhealthyPopulationModifier(int iChange);
	// K-Mod end

	int getExpInBorderModifier() const;
	void changeExpInBorderModifier(int iChange);

	int getBuildingOnlyHealthyCount() const;
	bool isBuildingOnlyHealthy() const;																																				// Exposed to Python
	void changeBuildingOnlyHealthyCount(int iChange);

	int getDistanceMaintenanceModifier() const;																																// Exposed to Python
	void changeDistanceMaintenanceModifier(int iChange);

	int getNumCitiesMaintenanceModifier() const;																															// Exposed to Python
	void changeNumCitiesMaintenanceModifier(int iChange);

	int getCorporationMaintenanceModifier() const;																															// Exposed to Python
	void changeCorporationMaintenanceModifier(int iChange);

	int getTotalMaintenance() const;																																					// Exposed to Python
	// advc.004b: Need the exact value (new getter function)
	int getTotalMaintenanceTimes100() const;
	void changeTotalMaintenance(int iChange);

	int getUpkeepModifier() const;																																						// Exposed to Python
	void changeUpkeepModifier(int iChange);

	int getLevelExperienceModifier() const;																																						// Exposed to Python
	void changeLevelExperienceModifier(int iChange);

	int getExtraHealth() const;																																			// Exposed to Python
	void changeExtraHealth(int iChange);

	int getBuildingGoodHealth() const;																																				// Exposed to Python
	void changeBuildingGoodHealth(int iChange);

	int getBuildingBadHealth() const;																																					// Exposed to Python
	void changeBuildingBadHealth(int iChange);

	int getExtraHappiness() const;																																						// Exposed to Python
	void changeExtraHappiness(int iChange);

	int getBuildingHappiness() const;																																					// Exposed to Python
	void changeBuildingHappiness(int iChange);

	int getLargestCityHappiness() const;																																			// Exposed to Python
	void changeLargestCityHappiness(int iChange);

	int getWarWearinessPercentAnger() const;																																	// Exposed to Python
	void updateWarWearinessPercentAnger();
	int getModifiedWarWearinessPercentAnger(int iWarWearinessPercentAnger) const;

	int getWarWearinessModifier() const;																																			// Exposed to Python
	void changeWarWearinessModifier(int iChange);

	int getFreeSpecialist() const;																																						// Exposed to Python
	void changeFreeSpecialist(int iChange);

	int getNoForeignTradeCount() const;
	bool isNoForeignTrade() const;																																						// Exposed to Python
	void changeNoForeignTradeCount(int iChange);

	int getNoCorporationsCount() const;
	bool isNoCorporations() const;																																						// Exposed to Python
	void changeNoCorporationsCount(int iChange);

	int getNoForeignCorporationsCount() const;
	bool isNoForeignCorporations() const;																																						// Exposed to Python
	void changeNoForeignCorporationsCount(int iChange);

	int getCoastalTradeRoutes() const;																																				// Exposed to Python
	void changeCoastalTradeRoutes(int iChange);																													// Exposed to Python

	int getTradeRoutes() const;																																								// Exposed to Python
	void changeTradeRoutes(int iChange);																																// Exposed to Python

	int getRevolutionTimer() const;																																	// Exposed to Python
	void setRevolutionTimer(int iNewValue);
	void changeRevolutionTimer(int iChange);

	int getConversionTimer() const;																																						// Exposed to Python
	void setConversionTimer(int iNewValue);
	void changeConversionTimer(int iChange);

	int getStateReligionCount() const;
	bool isStateReligion() const;																																							// Exposed to Python
	void changeStateReligionCount(int iChange);

	int getNoNonStateReligionSpreadCount() const;
	bool isNoNonStateReligionSpread() const;																												// Exposed to Python
	void changeNoNonStateReligionSpreadCount(int iChange);

	int getStateReligionHappiness() const;																													// Exposed to Python
	void changeStateReligionHappiness(int iChange);

	int getNonStateReligionHappiness() const;																																	// Exposed to Python
	void changeNonStateReligionHappiness(int iChange);

	int getStateReligionUnitProductionModifier() const;																												// Exposed to Python
	void changeStateReligionUnitProductionModifier(int iChange);

	int getStateReligionBuildingProductionModifier() const;																										// Exposed to Python
	void changeStateReligionBuildingProductionModifier(int iChange);																		// Exposed to Python

	int getStateReligionFreeExperience() const;																																// Exposed to Python
	void changeStateReligionFreeExperience(int iChange);

	DllExport inline CvCity* getCapitalCity() const // advc.inl																							// Exposed to Python
	{
		return getCity(m_iCapitalCityID);
	}
	void setCapitalCity(CvCity* pNewCapitalCity);
	// <advc.127b> -1 if no capital or (eObserver!=NO_TEAM) unrevealed to eObserver
	int getCapitalX(TeamTypes eObserver, bool bDebug = false) const;
	int getCapitalY(TeamTypes eObserver, bool bDebug = false) const;
	int getCapitalX(PlayerTypes eObserver, bool bDebug = false) const;
	int getCapitalY(PlayerTypes eObserver, bool bDebug = false) const;
	// </advc.127b>

	int getCitiesLost() const;																																								// Exposed to Python
	void changeCitiesLost(int iChange);

	int getWinsVsBarbs() const;																																								// Exposed to Python
	void changeWinsVsBarbs(int iChange);

	int getAssets() const;																																					// Exposed to Python
	void changeAssets(int iChange);																																			// Exposed to Python

	int getPower() const;																																						// Exposed to Python
	void changePower(int iChange);

	int getPopScore(bool bCheckVassal = true) const;																																				// Exposed to Python
	void changePopScore(int iChange);																																		// Exposed to Python
	int getLandScore(bool bCheckVassal = true) const;																																				// Exposed to Python
	void changeLandScore(int iChange);																																	// Exposed to Python
	int getTechScore() const;																																				// Exposed to Python
	void changeTechScore(int iChange);																																	// Exposed to Python
	int getWondersScore() const;																																		// Exposed to Python
	void changeWondersScore(int iChange);	// Exposed to Python

	int getCombatExperience() const; 	// Exposed to Python
	void setCombatExperience(int iExperience);   // Exposed to Python
	void changeCombatExperience(int iChange);   // Exposed to Python

	bool isConnected() const;
	DllExport int getNetID() const;
	DllExport void setNetID(int iNetID);
	//void sendReminder(); // advc.003y: Moved to CvPythonCaller (sendEmailReminder)

	uint getStartTime() const;
	DllExport void setStartTime(uint uiStartTime);
	uint getTotalTimePlayed() const;																																// Exposed to Python

	bool isMinorCiv() const;																																									// Exposed to Python
	bool isMajorCiv() const { return (!isBarbarian() && !isMinorCiv()); } // advc

	DllExport bool isAlive() const { return m_bAlive; } // advc.inl																																	// Exposed to Python
	bool isEverAlive() const { return m_bEverAlive; }; // advc.inl																															// Exposed to Python
	void setAlive(bool bNewValue);
	void verifyAlive();

	DllExport bool isTurnActive() const;
	DllExport void setTurnActive(bool bNewValue, bool bDoTurn = true);
	void onTurnLogging() const; // K-Mod

	bool isAutoMoves() const;
	void setAutoMoves(bool bNewValue);
	DllExport void setTurnActiveForPbem(bool bActive);

	DllExport bool isPbemNewTurn() const;
	DllExport void setPbemNewTurn(bool bNew);

	bool isEndTurn() const;
	DllExport void setEndTurn(bool bNewValue);

	DllExport bool isTurnDone() const;

	bool isExtendedGame() const;																																			// Exposed to Python
	void makeExtendedGame();

	bool isFoundedFirstCity() const;																																	// Exposed to Python
	void setFoundedFirstCity(bool bNewValue);
	// <advc.078>
	bool isAnyGPPEver() const;		// (Exposed to Python)
	void reportFirstGPP(); // </advc.078>

	bool isStrike() const;																																	// Exposed to Python
	void setStrike(bool bNewValue);

	DllExport PlayerTypes getID() const { return m_eID; } // advc.inl																								// Exposed to Python

	DllExport HandicapTypes getHandicapType() const;																									// Exposed to Python

	DllExport CivilizationTypes getCivilizationType() const;																					// Exposed to Python
	// <advc.003u>
	void setCivilization(CivilizationTypes eCivilization);
	inline CvCivilization const& getCivilization() const
	{
		FAssertMsg(m_pCivilization != NULL, "Player has no civilization type");
		return *m_pCivilization;
	} // </advc.003u>
	DllExport LeaderHeadTypes getLeaderType() const;																									// Exposed to Python
	LeaderHeadTypes getPersonalityType() const;																												// Exposed to Python
	void setPersonalityType(LeaderHeadTypes eNewValue);																					// Exposed to Python
	// advc.opt: Inlined
	inline DllExport EraTypes getCurrentEra() const { return m_eCurrentEra; }																										// Exposed to Python
	void setCurrentEra(EraTypes eNewValue);

	ReligionTypes getLastStateReligion() const;
	ReligionTypes getStateReligion() const;																									// Exposed to Python
	void setLastStateReligion(ReligionTypes eNewValue);																					// Exposed to Python

	PlayerTypes getParent() const;
	void setParent(PlayerTypes eParent);
	// <advc> Convenient to have these team-level functions directly at CvPlayer
	TeamTypes getMasterTeam() const;
	bool isAVassal() const; // </advc>
	DllExport inline TeamTypes getTeam() const { return m_eTeamType; } // advc.inl																				// Exposed to Python
	void setTeam(TeamTypes eTeam);
	void updateTeamType();

	DllExport PlayerColorTypes getPlayerColor() const;																								// Exposed to Python
	DllExport int getPlayerTextColorR() const;																												// Exposed to Python
	DllExport int getPlayerTextColorG() const;																												// Exposed to Python
	DllExport int getPlayerTextColorB() const;																												// Exposed to Python
	int getPlayerTextColorA() const;																												// Exposed to Python
	ColorTypes getPlayerTextColor() const; // advc.106

	int getSeaPlotYield(YieldTypes eIndex) const;																											// Exposed to Python
	void changeSeaPlotYield(YieldTypes eIndex, int iChange);

	int getYieldRateModifier(YieldTypes eIndex) const;																								// Exposed to Python
	void changeYieldRateModifier(YieldTypes eIndex, int iChange);

	int getCapitalYieldRateModifier(YieldTypes eIndex) const;																					// Exposed to Python
	void changeCapitalYieldRateModifier(YieldTypes eIndex, int iChange);

	int getExtraYieldThreshold(YieldTypes eIndex) const;																							// Exposed to Python
	void updateExtraYieldThreshold(YieldTypes eIndex);

	int getTradeYieldModifier(YieldTypes eIndex) const;																								// Exposed to Python
	void changeTradeYieldModifier(YieldTypes eIndex, int iChange);

	int getFreeCityCommerce(CommerceTypes eIndex) const;																							// Exposed to Python
	void changeFreeCityCommerce(CommerceTypes eIndex, int iChange);

	int getCommercePercent(CommerceTypes eIndex) const;																								// Exposed to Python
	/* void setCommercePercent(CommerceTypes eIndex, int iNewValue); // Exposed to Python
	DllExport void changeCommercePercent(CommerceTypes eIndex, int iChange); */ // Exposed to Python
	// K-Mod. these functions now return false if the value is not changed.
	bool setCommercePercent(CommerceTypes eIndex, int iNewValue, bool bForce = false); // Exposed to Python
	bool changeCommercePercent(CommerceTypes eIndex, int iChange); // Exposed to Python
	// K-Mod end

	int getCommerceRate(CommerceTypes eIndex) const;																									// Exposed to Python
	void changeCommerceRate(CommerceTypes eIndex, int iChange);

	int getCommerceRateModifier(CommerceTypes eIndex) const;																					// Exposed to Python
	void changeCommerceRateModifier(CommerceTypes eIndex, int iChange);

	int getCapitalCommerceRateModifier(CommerceTypes eIndex) const;																		// Exposed to Python
	void changeCapitalCommerceRateModifier(CommerceTypes eIndex, int iChange);

	int getStateReligionBuildingCommerce(CommerceTypes eIndex) const;																	// Exposed to Python
	void changeStateReligionBuildingCommerce(CommerceTypes eIndex, int iChange);

	int getSpecialistExtraCommerce(CommerceTypes eIndex) const;																				// Exposed to Python
	void changeSpecialistExtraCommerce(CommerceTypes eIndex, int iChange);

	int getCommerceFlexibleCount(CommerceTypes eIndex) const;
	bool isCommerceFlexible(CommerceTypes eIndex) const;																							// Exposed to Python
	void changeCommerceFlexibleCount(CommerceTypes eIndex, int iChange);

	int getGoldPerTurnByPlayer(PlayerTypes eIndex) const;																							// Exposed to Python
	void changeGoldPerTurnByPlayer(PlayerTypes eIndex, int iChange);

	bool isFeatAccomplished(FeatTypes eIndex) const;																									// Exposed to Python
	void setFeatAccomplished(FeatTypes eIndex, bool bNewValue);																	// Exposed to Python

	DllExport bool isOption(PlayerOptionTypes eIndex) const;																		// Exposed to Python
	DllExport void setOption(PlayerOptionTypes eIndex, bool bNewValue);													// Exposed to Python

	bool isLoyalMember(VoteSourceTypes eVoteSource) const;																		// Exposed to Python
	void setLoyalMember(VoteSourceTypes eVoteSource, bool bNewValue);													// Exposed to Python

	bool isPlayable() const;
	void setPlayable(bool bNewValue);

	int getBonusExport(BonusTypes eIndex) const;																											// Exposed to Python
	void changeBonusExport(BonusTypes eIndex, int iChange);

	int getBonusImport(BonusTypes eIndex) const;																											// Exposed to Python
	void changeBonusImport(BonusTypes eIndex, int iChange);

	int getImprovementCount(ImprovementTypes eIndex) const;																						// Exposed to Python
	void changeImprovementCount(ImprovementTypes eIndex, int iChange);

	int getFreeBuildingCount(BuildingTypes eIndex) const;
	bool isBuildingFree(BuildingTypes eIndex) const;																									// Exposed to Python
	void changeFreeBuildingCount(BuildingTypes eIndex, int iChange);

	int getExtraBuildingHappiness(BuildingTypes eIndex) const;																				// Exposed to Python
	void changeExtraBuildingHappiness(BuildingTypes eIndex, int iChange);
	int getExtraBuildingHealth(BuildingTypes eIndex) const;																				// Exposed to Python
	void changeExtraBuildingHealth(BuildingTypes eIndex, int iChange);

	int getFeatureHappiness(FeatureTypes eIndex) const;																								// Exposed to Python
	void changeFeatureHappiness(FeatureTypes eIndex, int iChange);

	int getUnitClassCount(UnitClassTypes eIndex) const;																								// Exposed to Python
	bool isUnitClassMaxedOut(UnitClassTypes eIndex, int iExtra = 0) const;														// Exposed to Python
	void changeUnitClassCount(UnitClassTypes eIndex, int iChange);
	int getUnitClassMaking(UnitClassTypes eIndex) const;																							// Exposed to Python
	void changeUnitClassMaking(UnitClassTypes eIndex, int iChange);
	int getUnitClassCountPlusMaking(UnitClassTypes eIndex) const;																			// Exposed to Python

	int getBuildingClassCount(BuildingClassTypes eIndex) const;																				// Exposed to Python
	bool isBuildingClassMaxedOut(BuildingClassTypes eIndex, int iExtra = 0) const;										// Exposed to Python
	void changeBuildingClassCount(BuildingClassTypes eIndex, int iChange);
	int getBuildingClassMaking(BuildingClassTypes eIndex) const;																			// Exposed to Python
	void changeBuildingClassMaking(BuildingClassTypes eIndex, int iChange);
	int getBuildingClassCountPlusMaking(BuildingClassTypes eIndex) const;															// Exposed to Python

	int getHurryCount(HurryTypes eIndex) const;																												// Exposed to Python
	bool canHurry(HurryTypes eIndex) const;																									// Exposed to Python
	bool canPopRush() const;
	bool canGoldRush() const; // advc.064b
	void changeHurryCount(HurryTypes eIndex, int iChange);

	int getSpecialBuildingNotRequiredCount(SpecialBuildingTypes eIndex) const;												// Exposed to Python
	bool isSpecialBuildingNotRequired(SpecialBuildingTypes eIndex) const;															// Exposed to Python
	void changeSpecialBuildingNotRequiredCount(SpecialBuildingTypes eIndex, int iChange);

	int getHasCivicOptionCount(CivicOptionTypes eIndex) const;
	bool isHasCivicOption(CivicOptionTypes eIndex) const;																							// Exposed to Python
	void changeHasCivicOptionCount(CivicOptionTypes eIndex, int iChange);

	int getNoCivicUpkeepCount(CivicOptionTypes eIndex) const;
	bool isNoCivicUpkeep(CivicOptionTypes eIndex) const;																							// Exposed to Python
	void changeNoCivicUpkeepCount(CivicOptionTypes eIndex, int iChange);

	int getHasReligionCount(ReligionTypes eIndex) const;																							// Exposed to Python
	int countTotalHasReligion() const;																																// Exposed to Python
	int findHighestHasReligionCount() const;																													// Exposed to Python
	void changeHasReligionCount(ReligionTypes eIndex, int iChange);
	// advc.132: No longer just an AI concept b/c spies can only switch to major now
	bool isMajorReligion(ReligionTypes eReligion) const;

	int getHasCorporationCount(CorporationTypes eIndex) const;																							// Exposed to Python
	int countTotalHasCorporation() const;																																// Exposed to Python
	void changeHasCorporationCount(CorporationTypes eIndex, int iChange);
	bool isActiveCorporation(CorporationTypes eIndex) const;

	int getUpkeepCount(UpkeepTypes eIndex) const;																											// Exposed to Python
	void changeUpkeepCount(UpkeepTypes eIndex, int iChange);

	int getSpecialistValidCount(SpecialistTypes eIndex) const;
	bool isSpecialistValid(SpecialistTypes eIndex) const;																		// Exposed to Python
	void changeSpecialistValidCount(SpecialistTypes eIndex, int iChange);

	bool isResearchingTech(TechTypes eIndex) const;																					// Exposed to Python
	void setResearchingTech(TechTypes eIndex, bool bNewValue);

	CivicTypes getCivics(CivicOptionTypes eIndex) const;																		// Exposed to Python
	int getSingleCivicUpkeep(CivicTypes eCivic, bool bIgnoreAnarchy = false,													// Exposed to Python
			int iExtraCities = 0) const; // advc.004b
	int getCivicUpkeep(CivicTypes* paeCivics = NULL, bool bIgnoreAnarchy = false, 													// Exposed to Python
			int iExtraCities = 0) const; // advc.004b
	void setCivics(CivicOptionTypes eIndex, CivicTypes eNewValue);															// Exposed to Python

	int getSpecialistExtraYield(SpecialistTypes eIndex1, YieldTypes eIndex2) const;										// Exposed to Python
	void changeSpecialistExtraYield(SpecialistTypes eIndex1, YieldTypes eIndex2, int iChange);

	int getImprovementYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2) const;								// Exposed to Python
	void changeImprovementYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2, int iChange);

	//void updateGroupCycle(CvUnit* pUnit);
	void updateGroupCycle(CvSelectionGroup const& kGroup); // K-Mod
	void removeGroupCycle(int iID);
	void refreshGroupCycleList(); // K-Mod
	CLLNode<int>* deleteGroupCycleNode(CLLNode<int>* pNode);
	CLLNode<int>* nextGroupCycleNode(CLLNode<int>* pNode) const;
	CLLNode<int>* previousGroupCycleNode(CLLNode<int>* pNode) const;
	CLLNode<int>* headGroupCycleNode() const;
	CLLNode<int>* tailGroupCycleNode() const;

	int findPathLength(TechTypes eTech, bool bCost = true) const;																			// Exposed to Python
	int getQueuePosition(TechTypes eTech) const;																											// Exposed to Python
	void clearResearchQueue();																												// Exposed to Python
	bool pushResearch(TechTypes eTech, bool bClear = false,																					// Exposed to Python
			bool bKillPopup = true); // advc.004x
	void popResearch(TechTypes eTech);																													// Exposed to Python
	int getLengthResearchQueue() const;																																// Exposed to Python
	CLLNode<TechTypes>* nextResearchQueueNode(CLLNode<TechTypes>* pNode) const;
	CLLNode<TechTypes>* headResearchQueueNode() const;
	CLLNode<TechTypes>* tailResearchQueueNode() const;

	void addCityName(const CvWString& szName);																									// Exposed to Python
	int getNumCityNames() const;																																// Exposed to Python
	CvWString getCityName(int iIndex) const;																										// Exposed to Python
	CLLNode<CvWString>* nextCityNameNode(CLLNode<CvWString>* pNode) const;
	CLLNode<CvWString>* headCityNameNode() const;

	// plot groups iteration
	// advc.inl: Inline most of these. Remove unused bRev param to avoid branching.
	inline CvPlotGroup* firstPlotGroup(int *pIterIdx/*, bool bRev=false*/) const
	{
		return /*bRev ? m_plotGroups.endIter(pIterIdx) :*/ m_plotGroups.beginIter(pIterIdx);
	}
	inline CvPlotGroup* nextPlotGroup(int *pIterIdx/*, bool bRev=false*/) const
	{
		return /*bRev ? m_plotGroups.prevIter(pIterIdx) :*/ m_plotGroups.nextIter(pIterIdx);
	}
	inline int getNumPlotGroups() const { return m_plotGroups.getCount(); }
	inline CvPlotGroup* getPlotGroup(int iID) const
	{
		return (CvPlotGroup*)m_plotGroups.getAt(iID);
	}
	CvPlotGroup* addPlotGroup();
	inline void deletePlotGroup(int iID)
	{
		m_plotGroups.removeAt(iID);
	}

	// city iteration (advc.inl: inlined)
	DllExport inline CvCity* firstCity(int *pIterIdx, bool bRev=false) const										// Exposed to Python
	{	//return (!bRev ? m_cities.beginIter(pIterIdx) : m_cities.endIter(pIterIdx));
		FAssert(!bRev);
		return m_cities.beginIter(pIterIdx); // advc.opt
	}
	DllExport inline CvCity* nextCity(int *pIterIdx, bool bRev=false) const											// Exposed to Python
	{	//return (!bRev ? m_cities.nextIter(pIterIdx) : m_cities.prevIter(pIterIdx));
		return m_cities.nextIter(pIterIdx); // advc.opt
	}
	DllExport int getNumCities() const																				// Exposed to Python
	{
		return m_cities.getCount();
	}
	DllExport inline CvCity* getCity(int iID) const																	// Exposed to Python
	{
		return m_cities.getAt(iID);
	}
	//CvCity* addCity(); // advc.003u: removed
	void deleteCity(int iID);

	// unit iteration (advc.inl: inlined)
	DllExport inline CvUnit* firstUnit(int *pIterIdx, bool bRev=false) const										// Exposed to Python
	{	//return (!bRev ? m_units.beginIter(pIterIdx) : m_units.endIter(pIterIdx));
		FAssert(!bRev);
		return m_units.beginIter(pIterIdx); // advc.opt
	}
	DllExport inline CvUnit* nextUnit(int *pIterIdx, bool bRev=false) const											// Exposed to Python
	{	//return (!bRev ? m_units.nextIter(pIterIdx) : m_units.prevIter(pIterIdx));
		return m_units.nextIter(pIterIdx);
	}
	DllExport int getNumUnits() const																				// Exposed to Python
	{
		return m_units.getCount();
	}
	inline CvUnit* getUnit(int iID) const																			// Exposed to Python
	{
		return m_units.getAt(iID);
	}
	//CvUnit* addUnit(); // advc.003u: removed
	void deleteUnit(int iID);

	// selection groups iteration (advc.inl: inlined)
	inline CvSelectionGroup* firstSelectionGroup(int *pIterIdx, bool bRev=false) const								// Exposed to Python
	{	//return (!bRev ? m_selectionGroups.beginIter(pIterIdx) : m_selectionGroups.endIter(pIterIdx));
		FAssert(!bRev);
		return m_selectionGroups.beginIter(pIterIdx);
	}
	inline CvSelectionGroup* nextSelectionGroup(int *pIterIdx, bool bRev=false) const								// Exposed to Python
	{	//return (!bRev ? m_selectionGroups.nextIter(pIterIdx) : m_selectionGroups.prevIter(pIterIdx));
		return m_selectionGroups.nextIter(pIterIdx);
	}
	int getNumSelectionGroups() const																				// Exposed to Python
	{
		return m_selectionGroups.getCount();
	}
	inline CvSelectionGroup* getSelectionGroup(int iID) const														// Exposed to Python
	{
		return m_selectionGroups.getAt(iID);
	}
	CvSelectionGroup* addSelectionGroup();
	void deleteSelectionGroup(int iID);

	// pending triggers iteration (advc.003j - unused)
	EventTriggeredData* firstEventTriggered(int *pIterIdx, bool bRev=false) const;
	EventTriggeredData* nextEventTriggered(int *pIterIdx, bool bRev=false) const;
	int getNumEventsTriggered() const;

	EventTriggeredData* getEventTriggered(int iID) const;								// Exposed to Python
	EventTriggeredData* addEventTriggered();
	void deleteEventTriggered(int iID);
	EventTriggeredData* initTriggeredData(EventTriggerTypes eEventTrigger,				// Exposed to Python
			bool bFire = false, int iCityId = -1, int iPlotX = INVALID_PLOT_COORD,
			int iPlotY = INVALID_PLOT_COORD, PlayerTypes eOtherPlayer = NO_PLAYER,
			int iOtherPlayerCityId = -1, ReligionTypes eReligion = NO_RELIGION,
			CorporationTypes eCorporation = NO_CORPORATION, int iUnitId = -1,
			BuildingTypes eBuilding = NO_BUILDING);
	int getEventTriggerWeight(EventTriggerTypes eTrigger) const;						// Exposed to python

	DllExport void addMessage(const CvTalkingHeadMessage& message);
	void showMissedMessages();
	void clearMessages();
	DllExport const CvMessageQueue& getGameMessages() const;
	void expireMessages();
	DllExport void addPopup(CvPopupInfo* pInfo, bool bFront = false);
	void clearPopups();
	DllExport CvPopupInfo* popFrontPopup();
	DllExport const CvPopupQueue& getPopups() const;
	void killAll(ButtonPopupTypes ePopupType, int iData1 = -1); // advc.004x
	DllExport void addDiplomacy(CvDiploParameters* pDiplo);
	void clearDiplomacy();
	DllExport const CvDiploQueue& getDiplomacy() const;
	DllExport CvDiploParameters* popFrontDiplomacy();
	void validateDiplomacy(); // advc.001e
	DllExport void showSpaceShip();
	DllExport void clearSpaceShipPopups();
	void doChangeCivicsPopup(CivicTypes eCivic); // advc.004x

	int getScoreHistory(int iTurn) const;																								// Exposed to Python
	void updateScoreHistory(int iTurn, int iBestScore);

	int getEconomyHistory(int iTurn) const;																							// Exposed to Python
	void updateEconomyHistory(int iTurn, int iBestEconomy);
	int getIndustryHistory(int iTurn) const;																						// Exposed to Python
	void updateIndustryHistory(int iTurn, int iBestIndustry);
	int getAgricultureHistory(int iTurn) const;																					// Exposed to Python
	void updateAgricultureHistory(int iTurn, int iBestAgriculture);
	int getPowerHistory(int iTurn) const;																								// Exposed to Python
	void updatePowerHistory(int iTurn, int iBestPower);
	int getCultureHistory(int iTurn) const;																							// Exposed to Python
	void updateCultureHistory(int iTurn, int iBestCulture);
	int getEspionageHistory(int iTurn) const;																							// Exposed to Python
	void updateEspionageHistory(int iTurn, int iBestEspionage);
	// advc.004s:
	void updateHistoryMovingAvg(CvTurnScoreMap& history, int iGameTurn, int iNewSample);
	const CvPlayerRecord* getPlayerRecord() const; // K-Mod

	// Script data needs to be a narrow string for pickling in Python
	std::string getScriptData() const;																									// Exposed to Python
	void setScriptData(std::string szNewValue);																					// Exposed to Python

	DllExport const CvString getPbemEmailAddress() const;
	DllExport void setPbemEmailAddress(const char* szAddress);
	DllExport const CvString getSmtpHost() const;
	DllExport void setSmtpHost(const char* szHost);

	const EventTriggeredData* getEventOccured(EventTypes eEvent) const;				// Exposed to python
	bool isTriggerFired(EventTriggerTypes eEventTrigger) const;
	void setEventOccured(EventTypes eEvent, const EventTriggeredData& kEventTriggered, bool bOthers = true);
	void resetEventOccured(EventTypes eEvent, bool bAnnounce = true);													// Exposed to Python
	void setTriggerFired(const EventTriggeredData& kTriggeredData, bool bOthers = true, bool bAnnounce = true);
	void resetTriggerFired(EventTriggerTypes eEventTrigger);
	void trigger(EventTriggerTypes eEventTrigger);													// Exposed to Python
	void trigger(const EventTriggeredData& kData);
	void applyEvent(EventTypes eEvent, int iTriggeredId, bool bUpdateTrigger = true);
	bool canDoEvent(EventTypes eEvent, const EventTriggeredData& kTriggeredData) const;
	TechTypes getBestEventTech(EventTypes eEvent, PlayerTypes eOtherPlayer) const;
	int getEventCost(EventTypes eEvent, PlayerTypes eOtherPlayer, bool bRandom) const;
	bool canTrigger(EventTriggerTypes eTrigger, PlayerTypes ePlayer, ReligionTypes eReligion) const;
	const EventTriggeredData* getEventCountdown(EventTypes eEvent) const;
	void setEventCountdown(EventTypes eEvent, const EventTriggeredData& kEventTriggered);
	void resetEventCountdown(EventTypes eEvent);

	bool isFreePromotion(UnitCombatTypes eUnitCombat, PromotionTypes ePromotion) const;
	void setFreePromotion(UnitCombatTypes eUnitCombat, PromotionTypes ePromotion, bool bFree);
	bool isFreePromotion(UnitClassTypes eUnitCombat, PromotionTypes ePromotion) const;
	void setFreePromotion(UnitClassTypes eUnitCombat, PromotionTypes ePromotion, bool bFree);

	PlayerVoteTypes getVote(int iId) const;
	void setVote(int iId, PlayerVoteTypes ePlayerVote);

	int getUnitExtraCost(UnitClassTypes eUnitClass) const;
	void setUnitExtraCost(UnitClassTypes eUnitClass, int iCost);

	bool splitEmpire(int iArea); // advc: Keep this one around for CvMessageData
	bool splitEmpire(CvArea& kArea);
	bool canSplitEmpire() const;
	bool canSplitArea(CvArea const& kArea) const;
	PlayerTypes getSplitEmpirePlayer(CvArea const& kArea) const;
	bool getSplitEmpireLeaders(CivLeaderArray& aLeaders) const;

	void launch(VictoryTypes victoryType);

	bool hasShrine(ReligionTypes eReligion);
	int getVotes(VoteTypes eVote, VoteSourceTypes eVoteSource) const;   // Exposed to Python
	void processVoteSource(VoteSourceTypes eVoteSource, bool bActive);
	bool canDoResolution(VoteSourceTypes eVoteSource, const VoteSelectionSubData& kData) const;
	bool canDefyResolution(VoteSourceTypes eVoteSource, const VoteSelectionSubData& kData) const;
	void setDefiedResolution(VoteSourceTypes eVoteSource, const VoteSelectionSubData& kData);
	void setEndorsedResolution(VoteSourceTypes eVoteSource, const VoteSelectionSubData& kData);
	bool isFullMember(VoteSourceTypes eVoteSource) const;		// Exposed to Python
	bool isVotingMember(VoteSourceTypes eVoteSource) const;		// Exposed to Python

	void invalidatePopulationRankCache();
	void invalidateYieldRankCache(YieldTypes eYield = NO_YIELD);
	void invalidateCommerceRankCache(CommerceTypes eCommerce = NO_COMMERCE);

	PlayerTypes pickConqueredCityOwner(const CvCity& kCity) const;
	bool canHaveTradeRoutesWith(PlayerTypes ePlayer) const;

	void forcePeace(PlayerTypes ePlayer);		// exposed to Python
	// advc.032: True iff a treaty was found and its turnsToCancel reset
	bool resetPeaceTreaty(PlayerTypes ePlayer);
	bool canSpiesEnterBorders(PlayerTypes ePlayer) const;
	int getNewCityProductionValue() const;

	int getGrowthThreshold(int iPopulation) const;

	void verifyUnitStacksValid();
	UnitTypes getTechFreeUnit(TechTypes eTech) const;
	// BULL - Trade Hover - start
	void calculateTradeTotals(YieldTypes eIndex, int& iDomesticYield,
			int& iDomesticRoutes, int& iForeignYield, int& iForeignRoutes,
			PlayerTypes eWithPlayer = NO_PLAYER) const;
	// BULL - Trade Hover - end
	void checkAlert(int iAlertID, bool bSilent); // advc.210 (exposed to Python)
	// advc.104, advc.038, advc.132; exposed to Python.
	double estimateYieldRate(YieldTypes eYield, int iSamples = 5) const;
	void setSavingReplay(bool b); // advc.106i
	// <advc.085> (both exposed to Python)
	void setScoreboardExpanded(bool b);
	bool isScoreboardExpanded() const;
	// </advc.085>
	DllExport void buildTradeTable(PlayerTypes eOtherPlayer, CLinkList<TradeData>& ourList) const;
	DllExport bool getHeadingTradeString(PlayerTypes eOtherPlayer, TradeableItems eItem, CvWString& szString, CvString& szIcon) const;
	DllExport bool getItemTradeString(PlayerTypes eOtherPlayer, bool bOffer, bool bShowingCurrent, const TradeData& zTradeData, CvWString& szString, CvString& szIcon) const;
	DllExport void updateTradeList(PlayerTypes eOtherPlayer, CLinkList<TradeData>& ourInventory, const CLinkList<TradeData>& ourOffer, const CLinkList<TradeData>& theirOffer) const;
	void markTradeOffers(CLinkList<TradeData>& ourInventory, const CLinkList<TradeData>& ourOffer) const; // K-Mod
	DllExport int getIntroMusicScriptId(PlayerTypes eForPlayer) const;
	DllExport int getMusicScriptId(PlayerTypes eForPlayer) const;
	DllExport void getGlobeLayerColors(GlobeLayerTypes eGlobeLayerType, int iOption, std::vector<NiColorA>& aColors, std::vector<CvPlotIndicatorData>& aIndicators) const;
	void setBonusHelpDirty(); // advc.003p
	DllExport void cheat(bool bCtrl, bool bAlt, bool bShift);
	DllExport const CvArtInfoUnit* getUnitArtInfo(UnitTypes eUnit, int iMeshGroup = 0) const;
	DllExport bool hasSpaceshipArrived() const;
	// <advc.003u>
	__forceinline CvPlayerAI& AI()
	{	//return *static_cast<CvPlayerAI*>(const_cast<CvPlayer*>(this));
		/*  The above won't work in an inline function b/c the compiler doesn't know
			that CvPlayerAI is derived from CvPlayer */
		return *reinterpret_cast<CvPlayerAI*>(this);
	}
	__forceinline CvPlayerAI const& AI() const
	{	//return *static_cast<CvPlayerAI const*>(this);
		return *reinterpret_cast<CvPlayerAI const*>(this);
	} // </advc.003u>	

protected:  // <advc.210>
	void initAlerts(bool bSilentCheck = false);
	void uninitAlerts(); // </advc.210>

	PlayerTypes m_eID; // advc: Moved up for easier access in the debugger

	static CvPlayerAI** m_aPlayers; // advc.003u: Moved from CvPlayerAI.h; and store only pointers.

	int m_iStartingX;
	int m_iStartingY;
	int m_iTotalPopulation;
	int m_iTotalLand;
	int m_iTotalLandScored;
	int m_iGold;
	int m_iGoldPerTurn;
	int m_iAdvancedStartPoints;
	int m_iGoldenAgeTurns;
	int m_iNumUnitGoldenAges;
	int m_iStrikeTurns;
	int m_iAnarchyTurns;
	int m_iMaxAnarchyTurns;
	int m_iAnarchyModifier;
	int m_iGoldenAgeModifier;
	int m_iGlobalHurryModifier;
	int m_iGreatPeopleCreated;
	int m_iGreatGeneralsCreated;
	int m_iGreatPeopleThresholdModifier;
	int m_iGreatGeneralsThresholdModifier;
	int m_iGreatPeopleRateModifier;
	int m_iGreatGeneralRateModifier;
	int m_iDomesticGreatGeneralRateModifier;
	int m_iStateReligionGreatPeopleRateModifier;
	int m_iMaxGlobalBuildingProductionModifier;
	int m_iMaxTeamBuildingProductionModifier;
	int m_iMaxPlayerBuildingProductionModifier;
	int m_iFreeExperience;
	int m_iFeatureProductionModifier;
	int m_iWorkerSpeedModifier;
	int m_iImprovementUpgradeRateModifier;
	int m_iMilitaryProductionModifier;
	int m_iSpaceProductionModifier;
	int m_iCityDefenseModifier;
	int m_iNumNukeUnits;
	int m_iNumOutsideUnits;
	int m_iBaseFreeUnits;
	int m_iBaseFreeMilitaryUnits;
	int m_iFreeUnitsPopulationPercent;
	int m_iFreeMilitaryUnitsPopulationPercent;
	int m_iGoldPerUnit;
	int m_iGoldPerMilitaryUnit;
	int m_iExtraUnitCost;
	int m_iNumMilitaryUnits;
	int m_iHappyPerMilitaryUnit;
	int m_iLuxuryModifier; // advc.912c
	int m_iMilitaryFoodProductionCount;
	int m_iConscriptCount;
	int m_iMaxConscript;
	int m_iHighestUnitLevel;
	int m_iOverflowResearch;
	//int m_iNoUnhealthyPopulationCount;
	int m_iUnhealthyPopulationModifier; // K-Mod
	int m_iExpInBorderModifier;
	int m_iBuildingOnlyHealthyCount;
	int m_iDistanceMaintenanceModifier;
	int m_iNumCitiesMaintenanceModifier;
	int m_iCorporationMaintenanceModifier;
	int m_iTotalMaintenance;
	int m_iUpkeepModifier;
	int m_iLevelExperienceModifier;
	int m_iExtraHealth;
	int m_iBuildingGoodHealth;
	int m_iBuildingBadHealth;
	int m_iExtraHappiness;
	int m_iBuildingHappiness;
	int m_iLargestCityHappiness;
	int m_iWarWearinessPercentAnger;
	int m_iWarWearinessModifier;
	int m_iGwPercentAnger; // K-Mod
	int m_iFreeSpecialist;
	int m_iNoForeignTradeCount;
	int m_iNoCorporationsCount;
	int m_iNoForeignCorporationsCount;
	int m_iCoastalTradeRoutes;
	int m_iTradeRoutes;
	int m_iRevolutionTimer;
	int m_iConversionTimer;
	int m_iStateReligionCount;
	int m_iNoNonStateReligionSpreadCount;
	int m_iStateReligionHappiness;
	int m_iNonStateReligionHappiness;
	int m_iStateReligionUnitProductionModifier;
	int m_iStateReligionBuildingProductionModifier;
	int m_iStateReligionFreeExperience;
	int m_iCapitalCityID;
	int m_iCitiesLost;
	int m_iWinsVsBarbs;
	int m_iAssets;
	int m_iPower;
	int m_iPopulationScore;
	int m_iLandScore;
	int m_iTechScore;
	int m_iWondersScore;
	int m_iCombatExperience;
	int m_iPopRushHurryCount;
	int m_iGoldRushHurryCount; // advc.064b
	int m_iInflationModifier;
	int m_iChoosingFreeTechCount; // K-Mod (based on the 'Unofficial Patch'

	uint m_uiStartTime;  // XXX save these?

	bool m_bAlive;
	bool m_bEverAlive;
	bool m_bTurnActive;
	bool m_bAutoMoves;
	bool m_bEndTurn;
	bool m_bPbemNewTurn;
	bool m_bExtendedGame;
	bool m_bFoundedFirstCity;
	bool m_bAnyGPPEver; // advc.078
	bool m_bStrike;
	bool m_bHuman;
	// AI_AUTO_PLAY_MOD, 09/01/07, MRGENIE: START
	bool m_bDisableHuman;		// Set to true to disable isHuman() check
	bool m_bAutoPlayJustEnded; // advc.127
	// AI_AUTO_PLAY_MOD: END
	bool m_bSavingReplay; // advc.106i
	bool m_bScoreboardExpanded; // advc.085

	LeaderHeadTypes m_ePersonalityType;
	EraTypes m_eCurrentEra;
	ReligionTypes m_eLastStateReligion;
	PlayerTypes m_eParent;
	TeamTypes m_eTeamType;
	CvCivilization* m_pCivilization; // advc.003u

	int* m_aiSeaPlotYield;
	int* m_aiYieldRateModifier;
	int* m_aiCapitalYieldRateModifier;
	int* m_aiExtraYieldThreshold;
	int* m_aiTradeYieldModifier;
	int* m_aiFreeCityCommerce;
	int* m_aiCommercePercent;
	int* m_aiCommerceRate;
	int* m_aiCommerceRateModifier;
	int* m_aiCapitalCommerceRateModifier;
	int* m_aiStateReligionBuildingCommerce;
	int* m_aiSpecialistExtraCommerce;
	int* m_aiCommerceFlexibleCount;
	int* m_aiGoldPerTurnByPlayer;
	int* m_aiEspionageSpendingWeightAgainstTeam;

	bool* m_abFeatAccomplished;
	bool* m_abOptions;

	CvString m_szScriptData;

	int* m_paiBonusExport;
	int* m_paiBonusImport;
	int* m_paiImprovementCount;
	int* m_paiFreeBuildingCount;
	int* m_paiExtraBuildingHappiness;
	int* m_paiExtraBuildingHealth;
	/*int** m_paiExtraBuildingYield;
	int** m_paiExtraBuildingCommerce;*/ // advc: unused
	int* m_paiFeatureHappiness;
	int* m_paiUnitClassCount;
	int* m_paiUnitClassMaking;
	int* m_paiBuildingClassCount;
	int* m_paiBuildingClassMaking;
	int* m_paiHurryCount;
	int* m_paiSpecialBuildingNotRequiredCount;
	int* m_paiHasCivicOptionCount;
	int* m_paiNoCivicUpkeepCount;
	int* m_paiHasReligionCount;
	int* m_paiHasCorporationCount;
	int* m_paiUpkeepCount; // advc (comment): unused (but accessible)
	int* m_paiSpecialistValidCount;

	bool* m_pabResearchingTech;
	bool* m_pabLoyalMember;

	std::vector<EventTriggerTypes> m_triggersFired;
	std::vector<AdvCiv4lert*> m_paAlerts; // advc.210
	// <advc.106b>
	std::vector<CvTalkingHeadMessage*> m_aMajorMsgs;
	int m_iNewMessages; // </advc.106b>
	CLinkList<std::pair<PlayerTypes,BonusTypes> > m_cancelingExport; // advc.074

	CivicTypes* m_paeCivics;

	int** m_ppaaiSpecialistExtraYield;
	int** m_ppaaiImprovementYieldChange;

	CLinkList<int> m_groupCycle;
	CLinkList<TechTypes> m_researchQueue;
	CLinkList<CvWString> m_cityNames;

	FFreeListTrashArray<CvPlotGroup> m_plotGroups;
	FFreeListTrashArray<CvCity,CvCityAI> m_cities;
	FFreeListTrashArray<CvUnit,CvUnitAI> m_units;
	FFreeListTrashArray<CvSelectionGroup,CvSelectionGroupAI> m_selectionGroups;
	FFreeListTrashArray<EventTriggeredData> m_eventsTriggered;

	CvEventMap m_mapEventsOccured;
	CvEventMap m_mapEventCountdown;
	UnitCombatPromotionArray m_aFreeUnitCombatPromotions;
	UnitClassPromotionArray m_aFreeUnitClassPromotions;

	std::vector< std::pair<int, PlayerVoteTypes> > m_aVote;
	std::vector< std::pair<UnitClassTypes, int> > m_aUnitExtraCosts;

	CvMessageQueue m_listGameMessages;
	CvPopupQueue m_listPopups;
	CvDiploQueue m_listDiplomacy;

	CivicTypes m_eReminderPending; // advc.004x
	CvWString** m_aszBonusHelp; // advc.003p  (not serialized)
	CvTurnScoreMap m_mapScoreHistory;
	CvTurnScoreMap m_mapEconomyHistory;
	CvTurnScoreMap m_mapIndustryHistory;
	CvTurnScoreMap m_mapAgricultureHistory;
	CvTurnScoreMap m_mapPowerHistory;
	CvTurnScoreMap m_mapCultureHistory;
	CvTurnScoreMap m_mapEspionageHistory;

	void uninit();
	void initContainers();
	bool initOtherData();

	void doGold();
	void doResearch();
	void doEspionagePoints();
	void doWarnings();
	void doEvents();
	// <advc.106b>
	void postProcessMessages();
	int getStartOfTurnMessageLimit() const;
	// </advc.106b>
	void decayBuildProgress(); // advc.011
	void showForeignPromoGlow(bool b); // advc.002e
	// <advc.ctr>
	int cultureConvertedUponCityTrade(CvPlot const& kCityPlot, CvPlot const& kPlot,
			PlayerTypes eOldOwner) const; // </advc.ctr>
	// <advc.314>
	bool isGoodyTech(TechTypes techId, bool bProgress) const;
	void addGoodyMsg(CvWString s, CvPlot const& p, TCHAR const* sound);
	void promoteFreeUnit(CvUnit& u, double pr);
	// </advc.314>
	static int adjustAdvStartPtsToSpeed(int iPoints); // advc.250c
	// advc.120f:
	void announceEspionageToThirdParties(EspionageMissionTypes eMission, PlayerTypes eTarget);
	bool checkExpireEvent(EventTypes eEvent, const EventTriggeredData& kTriggeredData) const;
	void expireEvent(EventTypes eEvent, const EventTriggeredData& kTriggeredData, bool bFail);
	bool isValidTriggerReligion(const CvEventTriggerInfo& kTrigger, CvCity* pCity, ReligionTypes eReligion) const;
	bool isValidTriggerCorporation(const CvEventTriggerInfo& kTrigger, CvCity* pCity, CorporationTypes eCorporation) const;
	CvCity* pickTriggerCity(EventTriggerTypes eTrigger) const;
	CvUnit* pickTriggerUnit(EventTriggerTypes eTrigger, CvPlot* pPlot, bool bPickPlot) const;
	bool isValidEventTech(TechTypes eTech, EventTypes eEvent, PlayerTypes eOtherPlayer) const;

	void verifyGoldCommercePercent();
	int doCaptureGold(CvCity const& kOldCity); // advc.003y
	void processCivics(CivicTypes eCivic, int iChange);

	//void doUpdateCacheOnTurn(); // advc: unused
	int getResearchTurnsLeftTimes100(TechTypes eTech, bool bOverflow) const;
	int groundbreakingPenalty(TechTypes eTech) const; // advc.groundbr

	void getTradeLayerColors(std::vector<NiColorA>& aColors, std::vector<CvPlotIndicatorData>& aIndicators) const;  // used by Globeview trade layer
	void getUnitLayerColors(GlobeLayerUnitOptionTypes eOption, std::vector<NiColorA>& aColors, std::vector<CvPlotIndicatorData>& aIndicators) const;  // used by Globeview unit layer
	void getResourceLayerColors(GlobeLayerResourceOptionTypes eOption, std::vector<NiColorA>& aColors, std::vector<CvPlotIndicatorData>& aIndicators) const;  // used by Globeview resource layer
	void getReligionLayerColors(ReligionTypes eSelectedReligion, std::vector<NiColorA>& aColors, std::vector<CvPlotIndicatorData>& aIndicators) const;  // used by Globeview religion layer
	void getCultureLayerColors(std::vector<NiColorA>& aColors, std::vector<CvPlotIndicatorData>& aIndicators) const;  // used by Globeview culture layer

private:
	/*  advc.003u: The remaining virtual functions should not be called within
		the DLL. (Call the respective CvPlayerAI functions instead.)
		The signatures have to be preserved for the EXE though, and in this exact
		order. Additional virtual functions for use within the DLL can be added
		in the public and protected sections above, but, for each function added,
		another external function needs to be removed. (I've already removed three
		in order to reposition CvPlayer::read, write and AI_makeAssignWorkDirty.)
		The first few should be safe to remove; I don't think the EXE ever calls them.
		I've removed all const qualifiers. They're not enforced at runtime and thus
		irrelevant on functions called only from the EXE (so long as it remains closed-source).
		The default arguments are important though. Changing them should be fine (if needs be).
	/*  K-Mod note: Adding new virtual functions to this list seems to cause unpredictable behaviour during the initialization of the game.
		So beware! */
	// ^advc: Right - messing things up here will mess things up at runtime.
	/*virtual void AI_initExternal();
	virtual void AI_resetExternal(bool bConstructor);
	virtual void AI_doTurnPreExternal();*/
	virtual void AI_doTurnPostExternal();
	virtual void AI_doTurnUnitsPreExternal();
	virtual void AI_doTurnUnitsPostExternal();
	virtual void AI_updateFoundValuesExternal(
			bool bStartingLoc = false);
	virtual void AI_unitUpdateExternal();
	virtual void AI_makeAssignWorkDirtyExternal();
	virtual void AI_assignWorkingPlotsExternal();
	virtual void AI_updateAssignWorkExternal();
	virtual void AI_makeProductionDirtyExternal();
	virtual void AI_conquerCityExternal(CvCity* pCity);
	virtual int AI_foundValueExternal(int iX, int iY,
			int iMinUnitRange = -1, bool bStartingLoc = false);
	virtual bool AI_isCommercePlotExternal(CvPlot* pPlot);
	virtual int AI_getPlotDangerExternal(CvPlot* pPlot,
			int iRange = -1, bool bTestMoves = true);
	virtual bool AI_isFinancialTroubleExternal();
	virtual TechTypes AI_bestTechExternal(
			int iMaxPathLength = 1, bool bIgnoreCost = false, bool bAsync = false, TechTypes eIgnoreTech = NO_TECH, AdvisorTypes eIgnoreAdvisor = NO_ADVISOR);
	virtual void AI_chooseFreeTechExternal();
	virtual void AI_chooseResearchExternal();
	virtual bool AI_isWillingToTalkExternal(PlayerTypes ePlayer);
	virtual bool AI_demandRebukedSneakExternal(PlayerTypes ePlayer);
	virtual bool AI_demandRebukedWarExternal(PlayerTypes ePlayer);
	virtual AttitudeTypes AI_getAttitudeExternal(PlayerTypes ePlayer,
			bool bForced = true);
	virtual PlayerVoteTypes AI_diploVoteExternal(VoteSelectionSubData& kVoteData, VoteSourceTypes eVoteSource, bool bPropose);
	virtual int AI_dealValExternal(PlayerTypes ePlayer, CLinkList<TradeData>* pList,
			bool bIgnoreAnnual = false, int iExtra = 1);
	virtual bool AI_considerOfferExternal(PlayerTypes ePlayer, CLinkList<TradeData>* pTheirList, CLinkList<TradeData>* pOurList,
			int iChange = 1);
	virtual bool AI_counterProposeExternal(PlayerTypes ePlayer, CLinkList<TradeData>* pTheirList, CLinkList<TradeData>* pOurList, CLinkList<TradeData>* pTheirInventory, CLinkList<TradeData>* pOurInventory, CLinkList<TradeData>* pTheirCounter, CLinkList<TradeData>* pOurCounter);
	virtual int AI_bonusValExternal(BonusTypes eBonus, int iChange);
	virtual int AI_bonusTradeValExternal(BonusTypes eBonus, PlayerTypes ePlayer, int iChange);
	virtual DenialTypes AI_bonusTradeExternal(BonusTypes eBonus, PlayerTypes ePlayer);
	virtual int AI_cityTradeValExternal(CvCity* pCity);
	virtual DenialTypes AI_cityTradeExternal(CvCity* pCity, PlayerTypes ePlayer);
	virtual DenialTypes AI_stopTradingTradeExternal(TeamTypes eTradeTeam, PlayerTypes ePlayer);
	virtual DenialTypes AI_civicTradeExternal(CivicTypes eCivic, PlayerTypes ePlayer);
	virtual DenialTypes AI_religionTradeExternal(ReligionTypes eReligion, PlayerTypes ePlayer);
	virtual int AI_unitValueExternal(UnitTypes eUnit, UnitAITypes eUnitAI, CvArea* pArea);
	virtual int AI_totalUnitAIsExternal(UnitAITypes eUnitAI);
	virtual int AI_totalAreaUnitAIsExternal(CvArea* pArea, UnitAITypes eUnitAI);
	virtual int AI_totalWaterAreaUnitAIsExternal(CvArea* pArea, UnitAITypes eUnitAI);
	virtual int AI_plotTargetMissionAIsExternal(CvPlot* pPlot, MissionAITypes eMissionAI,
			CvSelectionGroup* pSkipSelectionGroup = NULL, int iRange = 0);
	virtual int AI_unitTargetMissionAIsExternal(CvUnit* pUnit, MissionAITypes eMissionAI,
			CvSelectionGroup* pSkipSelectionGroup = NULL);
	virtual int AI_civicValueExternal(CivicTypes eCivic);
	virtual int AI_getNumAIUnitsExternal(UnitAITypes eIndex);
	virtual void AI_changePeacetimeTradeValueExternal(PlayerTypes eIndex, int iChange);
	virtual void AI_changePeacetimeGrantValueExternal(PlayerTypes eIndex, int iChange);
	virtual int AI_getAttitudeExtraExternal(PlayerTypes eIndex);
	virtual void AI_setAttitudeExtraExternal(PlayerTypes eIndex, int iNewValue);
	virtual void AI_changeAttitudeExtraExternal(PlayerTypes eIndex, int iChange);
	virtual void AI_setFirstContactExternal(PlayerTypes eIndex, bool bNewValue);
	virtual int AI_getMemoryCountExternal(PlayerTypes eIndex1, MemoryTypes eIndex2);
	virtual void AI_changeMemoryCountExternal(PlayerTypes eIndex1, MemoryTypes eIndex2, int iChange);
	virtual void AI_doCommerceExternal();
	virtual EventTypes AI_chooseEventExternal(int iTriggeredId);
	virtual void AI_launchExternal(VictoryTypes eVictory);
	virtual void AI_doAdvancedStartExternal(bool bNoExit = false);
	virtual void AI_updateBonusValueExternal();
	virtual void AI_updateBonusValueExternal(BonusTypes eBonus);
	virtual ReligionTypes AI_chooseReligionExternal();
	virtual int AI_getExtraGoldTargetExternal();
	virtual void AI_setExtraGoldTargetExternal(int iNewValue);
	virtual int AI_maxGoldPerTurnTradeExternal(PlayerTypes ePlayer);
	virtual int AI_maxGoldTradeExternal(PlayerTypes ePlayer);
	virtual void readExternal(FDataStreamBase* pStream);
	virtual void writeExternal(FDataStreamBase* pStream);
};

/*	<advc.opt> Moved from CvGameCoreUtils for inlining.
	NO_PLAYER checks removed (cf. IDInfo constructor). */
DllExport inline CvCity* getCity(IDInfo city)												// Exposed to Python
{
	FAssertBounds(0, MAX_PLAYERS, city.eOwner);
	return GET_PLAYER(city.eOwner).getCity(city.iID);
}
DllExport inline CvUnit* getUnit(IDInfo unit)												// Exposed to Python
{
	FAssertBounds(0, MAX_PLAYERS, unit.eOwner);
	return GET_PLAYER(unit.eOwner).getUnit(unit.iID);
} // </advc.opt>

#endif
