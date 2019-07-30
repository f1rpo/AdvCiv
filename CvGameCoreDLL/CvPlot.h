#pragma once

// CvPlot.h

#ifndef CIV4_PLOT_H
#define CIV4_PLOT_H

// needs to have dll-interface to be used by clients of class
#pragma warning( disable: 4251 )

class CvArea;
class CvMap;
class CvPlotBuilder;
class CvRoute;
class CvRiver;
class CvCity;
class CvPlotGroup;
class CvFeature;
class CvUnit;
class CvSymbol;
class CvFlagEntity;
class DefenderSelector; // advco.defr

typedef bool (*ConstPlotUnitFunc)( const CvUnit* pUnit, int iData1, int iData2);
typedef bool (*PlotUnitFunc)(CvUnit* pUnit, int iData1, int iData2);

class CvPlot
{

public:
	CvPlot();
	virtual ~CvPlot();

	void init(int iX, int iY);
	void uninit();
	void reset(int iX = 0, int iY = 0, bool bConstructorCall=false);
	void setupGraphical();
	void updateGraphicEra();

	void erase();																																								// Exposed to Python

	DllExport float getPointX() const;
	DllExport float getPointY() const;
	DllExport NiPoint3 getPoint() const;																																	// Exposed to Python

	float getSymbolSize() const;
	DllExport float getSymbolOffsetX(int iID) const;
	DllExport float getSymbolOffsetY(int iID) const;

	TeamTypes getTeam() const;																																	// Exposed to Python

	void doTurn();

	void doImprovement();

	void updateCulture(bool bBumpUnits, bool bUpdatePlotGroups);

	void updateFog();
	void updateVisibility();

	void updateSymbolDisplay();
	void updateSymbolVisibility();
	void updateSymbols();

	void updateMinimapColor();

	void updateCenterUnit();

	void verifyUnitValidPlot();
	void forceBumpUnits(); // K-Mod

	void nukeExplosion(int iRange, CvUnit* pNukeUnit = NULL,
			bool bBomb = true); //  K-Mod added bBomb, Exposed to Python

	bool isConnectedTo( const CvCity* pCity) const;																												// Exposed to Python
	bool isConnectedToCapital(PlayerTypes ePlayer = NO_PLAYER) const;																			// Exposed to Python
	int getPlotGroupConnectedBonus(PlayerTypes ePlayer, BonusTypes eBonus) const;													// Exposed to Python
	bool isPlotGroupConnectedBonus(PlayerTypes ePlayer, BonusTypes eBonus) const;								// Exposed to Python
	bool isAdjacentPlotGroupConnectedBonus(PlayerTypes ePlayer, BonusTypes eBonus) const;				// Exposed to Python
	void updatePlotGroupBonus(bool bAdd);

	bool isAdjacentToArea(int iAreaID) const;
	bool isAdjacentToArea(const CvArea* pArea) const;																						// Exposed to Python
	bool shareAdjacentArea( const CvPlot* pPlot) const;																					// Exposed to Python
	bool isAdjacentToLand() const;																															// Exposed to Python
	bool isCoastalLand(int iMinWaterSize = -1) const;																																	// Exposed to Python

	bool isVisibleWorked() const;
	bool isWithinTeamCityRadius(TeamTypes eTeam, PlayerTypes eIgnorePlayer = NO_PLAYER) const;	// Exposed to Python

	DllExport bool isLake() const;																															// Exposed to Python
	bool isFreshWater() const;																												// Exposed to Python
	bool isPotentialIrrigation() const;																													// Exposed to Python
	bool canHavePotentialIrrigation() const;																										// Exposed to Python
	DllExport bool isIrrigationAvailable(bool bIgnoreSelf = false) const;												// Exposed to Python

	bool isRiverMask() const;
	DllExport bool isRiverCrossingFlowClockwise(DirectionTypes eDirection) const;
	bool isRiverSide() const;																																		// Exposed to Python
	bool isRiver() const;																																				// Exposed to Python
	bool isRiverConnection(DirectionTypes eDirection) const;																		// Exposed to Python
	// advc.500:
	bool isConnectRiverSegments() const;
	// advc.121: A kind of canal detection
	bool isConnectSea() const;

	CvPlot* getNearestLandPlotInternal(int iDistance) const;
	int getNearestLandArea() const;																															// Exposed to Python
	CvPlot* getNearestLandPlot() const;																													// Exposed to Python

	int seeFromLevel(TeamTypes eTeam) const;																										// Exposed to Python
	int seeThroughLevel() const;																																// Exposed to Python
	void changeAdjacentSight(TeamTypes eTeam, int iRange, bool bIncrement,
			CvUnit* pUnit, bool bUpdatePlotGroups);
	bool canSeePlot(CvPlot *plot, TeamTypes eTeam, int iRange, DirectionTypes eFacingDirection) const;
	bool canSeeDisplacementPlot(TeamTypes eTeam, int dx, int dy,
			int originalDX, int originalDY, bool firstPlot, bool outerRing) const;
	bool shouldProcessDisplacementPlot(int dx, int dy, int range, DirectionTypes eFacingDirection) const;
	void updateSight(bool bIncrement, bool bUpdatePlotGroups);
	void updateSeeFromSight(bool bIncrement, bool bUpdatePlotGroups);

	bool canHaveBonus(BonusTypes eBonus, bool bIgnoreLatitude = false,																						// Exposed to Python
			bool bIgnoreFeature = false) const; // advc.129
	bool canHaveImprovement(ImprovementTypes eImprovement,														// Exposed to Python
			TeamTypes eTeam = NO_TEAM, bool bPotential = false,
			BuildTypes eBuild = NO_BUILD, bool bAnyBuild = true) const; // dlph.9
	bool canBuild(BuildTypes eBuild, PlayerTypes ePlayer = NO_PLAYER, bool bTestVisible = false) const;														// Exposed to Python
	int getBuildTime(BuildTypes eBuild,																																										// Exposed to Python
			PlayerTypes ePlayer) const; // advc.251
	int getBuildTurnsLeft(BuildTypes eBuild, /* advc.251: */ PlayerTypes ePlayer,
			int iNowExtra, int iThenExtra,																			// Exposed to Python
			// <advc.011c>
			bool bIncludeUnits = true) const;
	int getBuildTurnsLeft(BuildTypes eBuild, PlayerTypes ePlayer) const;
	// </advc.011c>
	int getFeatureProduction(BuildTypes eBuild, TeamTypes eTeam, CvCity** ppCity) const;																// Exposed to Python

	DllExport CvUnit* getBestDefender(PlayerTypes eOwner,													// Exposed to Python
			PlayerTypes eAttackingPlayer = NO_PLAYER, const CvUnit* pAttacker = NULL,
			bool bTestAtWar = false, bool bTestPotentialEnemy = false,
			bool bTestCanMove = false) const { // <advc.028>
		return getBestDefender(eOwner, eAttackingPlayer, pAttacker, bTestAtWar,
				bTestPotentialEnemy, bTestCanMove, false); }
	CvUnit* getBestDefender(PlayerTypes eOwner,
			PlayerTypes eAttackingPlayer, CvUnit const* pAttacker,
			bool bTestAtWar, bool bTestPotentialEnemy, bool bTestCanMove,
			bool bTestVisible) const; // </advc.028>
	// advco.defr:
	void unavailableDefendersVsActivePlayer(std::set<CvUnit const*>& r) const;
	// BETTER_BTS_AI_MOD, Lead From Behind (UncutDragon), 02/21/10, jdog5000:
	bool hasDefender(bool bCheckCanAttack, PlayerTypes eOwner, PlayerTypes eAttackingPlayer = NO_PLAYER, const CvUnit* pAttacker = NULL, bool bTestAtWar = false, bool bTestPotentialEnemy = false, bool bTestCanMove = false) const;
	// disabled by K-Mod:
	//int AI_sumStrength(PlayerTypes eOwner, PlayerTypes eAttackingPlayer = NO_PLAYER, DomainTypes eDomainType = NO_DOMAIN, bool bDefensiveBonuses = true, bool bTestAtWar = false, bool bTestPotentialEnemy = false) const;
	CvUnit* getSelectedUnit() const;																																// Exposed to Python
	int getUnitPower(PlayerTypes eOwner = NO_PLAYER) const;																					// Exposed to Python

	int defenseModifier(TeamTypes eDefender, bool bIgnoreBuilding,
		/*  advc.012: NO_TEAM means rival defense applies; moved bHelp to the
			end b/c that parameter is rarely set */
			TeamTypes eAttacker = NO_TEAM, bool bHelp = false) const;									// Exposed to Python
	int movementCost(const CvUnit* pUnit, const CvPlot* pFromPlot,														// Exposed to Python
			bool bAssumeRevealed = true) const; // advc.001i

	int getExtraMovePathCost() const;																																// Exposed to Python
	void changeExtraMovePathCost(int iChange);																																// Exposed to Python

	bool isAdjacentOwned() const;																																		// Exposed to Python
	bool isAdjacentPlayer(PlayerTypes ePlayer, bool bLandOnly = false) const;												// Exposed to Python
	bool isAdjacentTeam(TeamTypes eTeam, bool bLandOnly = false) const;															// Exposed to Python
	bool isWithinCultureRange(PlayerTypes ePlayer) const;																						// Exposed to Python
	int getNumCultureRangeCities(PlayerTypes ePlayer) const;																				// Exposed to Python

	// BETTER_BTS_AI_MOD, General AI, 11/30/08, jdog5000: START
			// advc.003: const qualifier added to these two
	bool isHasPathToEnemyCity(TeamTypes eAttackerTeam, bool bIgnoreBarb = true) const;
	bool isHasPathToPlayerCity(TeamTypes eMoveTeam, PlayerTypes eOtherPlayer = NO_PLAYER) const;
	int calculatePathDistanceToPlot(TeamTypes eTeam, CvPlot* pTargetPlot,
			// <advc.104b>
			TeamTypes eTargetTeam = NO_TEAM, DomainTypes eDomain = NO_DOMAIN,
			int iMaxPath = -1); // </advc.104b>
	// BETTER_BTS_AI_MOD: END
	// BETTER_BTS_AI_MOD, Efficiency, 08/21/09, jdog5000: START
	// Plot danger cache (rewritten for K-Mod to fix bugs and improvement performance)
	inline int getActivePlayerSafeRangeCache() const { return m_iActivePlayerSafeRangeCache; }
	inline void setActivePlayerSafeRangeCache(int range) { m_iActivePlayerSafeRangeCache = range; }
	inline bool getBorderDangerCache(TeamTypes eTeam) const { return m_abBorderDangerCache[eTeam]; }
	inline void setBorderDangerCache(TeamTypes eTeam, bool bNewValue) { m_abBorderDangerCache[eTeam] = bNewValue; }
	void invalidateBorderDangerCache();
	// BETTER_BTS_AI_MOD: END
	PlayerTypes calculateCulturalOwner(
			bool bIgnoreCultureRange = false, // advc.099c
			bool bOwnExclusiveRadius = false) const; // advc.035

	void plotAction(PlotUnitFunc func, int iData1 = -1, int iData2 = -1, PlayerTypes eOwner = NO_PLAYER, TeamTypes eTeam = NO_TEAM);
	int plotCount(ConstPlotUnitFunc funcA, int iData1A = -1, int iData2A = -1, PlayerTypes eOwner = NO_PLAYER, TeamTypes eTeam = NO_TEAM, ConstPlotUnitFunc funcB = NULL, int iData1B = -1, int iData2B = -1) const;
	CvUnit* plotCheck(ConstPlotUnitFunc funcA, int iData1A = -1, int iData2A = -1, PlayerTypes eOwner = NO_PLAYER, TeamTypes eTeam = NO_TEAM, ConstPlotUnitFunc funcB = NULL, int iData1B = -1, int iData2B = -1) const;

	bool isOwned() const;																																							// Exposed to Python
	bool isBarbarian() const;																																					// Exposed to Python
	bool isRevealedBarbarian() const;																																	// Exposed to Python

	bool isVisible(TeamTypes eTeam, bool bDebug) const;																			// Exposed to Python
	DllExport bool isActiveVisible(bool bDebug) const;																								// Exposed to Python
	bool isVisibleToCivTeam() const;																																// Exposed to Python
	// <advc.706>
	static bool isAllFog();
	static void setAllFog(bool b); // </advc.706>
	// <advc.300>
	bool isCivUnitNearby(int iRadius) const;
	void getAdjacentLandAreaIds(std::set<int>& r) const;
	CvPlot const* nearestInvisiblePlot(bool bOnlyLand, int iMaxPlotDist, TeamTypes eObserver) const;
	// </advc.300>
	bool isVisibleToWatchingHuman() const;																														// Exposed to Python
	bool isAdjacentVisible(TeamTypes eTeam, bool bDebug) const;																				// Exposed to Python
	bool isAdjacentNonvisible(TeamTypes eTeam) const;																				// Exposed to Python

	DllExport bool isGoody(TeamTypes eTeam = NO_TEAM) const;																					// Exposed to Python
	bool isRevealedGoody(TeamTypes eTeam = NO_TEAM) const;																						// Exposed to Python
	void removeGoody();																																								// Exposed to Python

	DllExport bool isCity(bool bCheckImprovement = false, TeamTypes eForTeam = NO_TEAM) const;																																		// Exposed to Python
	bool isFriendlyCity(const CvUnit& kUnit, bool bCheckImprovement) const;																												// Exposed to Python
	bool isEnemyCity(const CvUnit& kUnit) const;																													// Exposed to Python

	bool isOccupation() const;																																				// Exposed to Python
	bool isBeingWorked() const;																															// Exposed to Python

	bool isUnit() const;																																							// Exposed to Python
	bool isInvestigate(TeamTypes eTeam) const;																												// Exposed to Python
	bool isVisibleEnemyDefender(const CvUnit* pUnit) const;																						// Exposed to Python
	CvUnit *getVisibleEnemyDefender(PlayerTypes ePlayer) const;
	int getNumDefenders(PlayerTypes ePlayer) const;																										// Exposed to Python
	int getNumVisibleEnemyDefenders(const CvUnit* pUnit) const;																				// Exposed to Python
	int getNumVisiblePotentialEnemyDefenders(const CvUnit* pUnit) const;															// Exposed to Python
	DllExport bool isVisibleEnemyUnit(PlayerTypes ePlayer) const;																			// Exposed to Python
	// advc.122:
	bool isVisibleEnemyCityAttacker(PlayerTypes eDefender, TeamTypes eAssumePeace = NO_TEAM) const;
	bool isVisiblePotentialEnemyUnit(PlayerTypes ePlayer) const; // K-Mod
	DllExport int getNumVisibleUnits(PlayerTypes ePlayer) const;
	bool isVisibleEnemyUnit(const CvUnit* pUnit) const;
	// advc.004l:
	bool isVisibleEnemyUnit(CvUnit const* pUnit, CvUnit const* pPotentialEnemy) const;
	bool isVisibleOtherUnit(PlayerTypes ePlayer) const;																								// Exposed to Python
	DllExport bool isFighting() const;																																// Exposed to Python

	bool canHaveFeature(FeatureTypes eFeature) const;																				// Exposed to Python

	DllExport bool isRoute() const;																																		// Exposed to Python
	bool isValidRoute(const CvUnit* pUnit,																											// Exposed to Python
			bool bAssumeRevealed) const; // advc.001i
	bool isTradeNetworkImpassable(TeamTypes eTeam) const;																														// Exposed to Python
	bool isNetworkTerrain(TeamTypes eTeam) const;																											// Exposed to Python
	bool isBonusNetwork(TeamTypes eTeam) const;																												// Exposed to Python
	bool isTradeNetwork(TeamTypes eTeam) const;																												// Exposed to Python
	bool isTradeNetworkConnected(const CvPlot * pPlot, TeamTypes eTeam) const;												// Exposed to Python
	bool isRiverNetwork(TeamTypes eTeam) const;

	bool isValidDomainForLocation(const CvUnit& unit) const;																					// Exposed to Python
	bool isValidDomainForAction(const CvUnit& unit) const;																						// Exposed to Python
	bool isImpassable() const;																															// Exposed to Python

	int getXExternal() const; // advc.003f: Exported through .def file																					// Exposed to Python
	#ifdef _USRDLL
	inline int getX() const { return m_iX; } // advc.003f: Renamed from getX_INLINE
	#endif
	int getYExternal() const; // advc.003f: Exported through .def file																					// Exposed to Python
	#ifdef _USRDLL
	inline int getY() const { return m_iY; } // advc.003f: Renamed from getY_INLINE
	#endif
	bool at(int iX, int iY) const;																																		// Exposed to Python
	int getLatitude() const;																																					// Exposed to Python
	void setLatitude(int iLatitude); // advc.tsl	(exposed to Python)
	int getFOWIndex() const;

	CvArea* area() const;																																							// Exposed to Python
	CvArea* waterArea(
			// BETTER_BTS_AI_MOD, General AI, 01/02/09, jdog5000
			bool bNoImpassable = false) const;
	CvArea* secondWaterArea() const;
	int getArea() const;																																		// Exposed to Python
	void setArea(int iNewValue, /* advc.310: */ bool bProcess = true);

	DllExport int getFeatureVariety() const;																													// Exposed to Python

	int getOwnershipDuration() const;																																	// Exposed to Python
	bool isOwnershipScore() const;																																		// Exposed to Python
	void setOwnershipDuration(int iNewValue);																													// Exposed to Python
	void changeOwnershipDuration(int iChange);																												// Exposed to Python

	int getImprovementDuration() const;																																// Exposed to Python
	void setImprovementDuration(int iNewValue);																												// Exposed to Python
	void changeImprovementDuration(int iChange);																											// Exposed to Python

	int getUpgradeProgress() const;																													// Exposed to Python
	int getUpgradeTimeLeft(ImprovementTypes eImprovement, PlayerTypes ePlayer) const;				// Exposed to Python
	void setUpgradeProgress(int iNewValue);																														// Exposed to Python
	void changeUpgradeProgress(int iChange);																													// Exposed to Python

	int getForceUnownedTimer() const;																																	// Exposed to Python
	bool isForceUnowned() const;																																			// Exposed to Python
	void setForceUnownedTimer(int iNewValue);																													// Exposed to Python
	void changeForceUnownedTimer(int iChange);																												// Exposed to Python

	int getCityRadiusCount() const;																																		// Exposed to Python
	bool isCityRadius() const; // Exposed to Python (K-Mod changed to bool)
	void changeCityRadiusCount(int iChange);

	bool isStartingPlot() const;																																			// Exposed to Python
	void setStartingPlot(bool bNewValue);																															// Exposed to Python

	DllExport bool isNOfRiver() const;																																// Exposed to Python
	void setNOfRiver(bool bNewValue, CardinalDirectionTypes eRiverDir);											// Exposed to Python

	DllExport bool isWOfRiver() const;																																// Exposed to Python
	void setWOfRiver(bool bNewValue, CardinalDirectionTypes eRiverDir);											// Exposed to Python

	DllExport CardinalDirectionTypes getRiverNSDirection() const;																			// Exposed to Python
	DllExport CardinalDirectionTypes getRiverWEDirection() const;																			// Exposed to Python

	CvPlot* getInlandCorner() const;																																	// Exposed to Python
	bool hasCoastAtSECorner() const;

	bool isIrrigated() const;																																					// Exposed to Python
	void setIrrigated(bool bNewValue);
	void updateIrrigated();

	bool isPotentialCityWork() const;																																						// Exposed to Python
	bool isPotentialCityWorkForArea(CvArea* pArea) const;																												// Exposed to Python
	void updatePotentialCityWork();

	bool isShowCitySymbols() const;
	void updateShowCitySymbols();

	bool isFlagDirty() const;																																										// Exposed to Python
	void setFlagDirty(bool bNewValue);																																					// Exposed to Python

	PlayerTypes getOwnerExternal() const; // advc.003f: Exported through .def file																					// Exposed to Python
	#ifdef _USRDLL
	inline PlayerTypes getOwner() const // advc.003f: Renamed from getOwnerINLINE
	{
		return (PlayerTypes)m_eOwner;
	}
	#endif
	void setOwner(PlayerTypes eNewValue, bool bCheckUnits, bool bUpdatePlotGroup);
	/*  <advc.035> The returned player becomes the owner if war/peace changes
		between that player and the current owner */
	PlayerTypes getSecondOwner() const;
	void setSecondOwner(PlayerTypes eNewValue);
	bool isContestedByRival(PlayerTypes eRival = NO_PLAYER) const;
	// </advc.035>
	PlotTypes getPlotType() const;																																			// Exposed to Python
	DllExport bool isWater() const;																																								// Exposed to Python
	bool isFlatlands() const;																																											// Exposed to Python
	DllExport bool isHills() const;																																								// Exposed to Python
	DllExport bool isPeak() const;																																								// Exposed to Python
	void setPlotType(PlotTypes eNewValue, bool bRecalculate = true, bool bRebuildGraphics = true);			// Exposed to Python

	DllExport TerrainTypes getTerrainType() const;																																	// Exposed to Python
	void setTerrainType(TerrainTypes eNewValue, bool bRecalculate = true, bool bRebuildGraphics = true);	// Exposed to Python

	DllExport FeatureTypes getFeatureType() const;																																	// Exposed to Python
	void setFeatureType(FeatureTypes eNewValue, int iVariety = -1);																				// Exposed to Python
	void setFeatureDummyVisibility(const char *dummyTag, bool show);																				// Exposed to Python
	void addFeatureDummyModel(const char *dummyTag, const char *modelTag);
	void setFeatureDummyTexture(const char *dummyTag, const char *textureTag);
	CvString pickFeatureDummyTag(int mouseX, int mouseY);
	void resetFeatureModel();

	DllExport BonusTypes getBonusType(TeamTypes eTeam = NO_TEAM) const;																							// Exposed to Python
	BonusTypes getNonObsoleteBonusType(TeamTypes eTeam = NO_TEAM, bool bCheckConnected = false) const;																	// Exposed to Python
	void setBonusType(BonusTypes eNewValue);																															// Exposed to Python

	DllExport ImprovementTypes getImprovementType() const;																													// Exposed to Python
	void setImprovementType(ImprovementTypes eNewValue);																									// Exposed to Python

	RouteTypes getRouteType() const;																																			// Exposed to Python
	void setRouteType(RouteTypes eNewValue, bool bUpdatePlotGroup);																															// Exposed to Python
	void updateCityRoute(bool bUpdatePlotGroup);

	DllExport CvCity* getPlotCity() const;																																					// Exposed to Python
	void setPlotCity(CvCity* pNewValue);
	void setRuinsName(const CvWString& szName); // advc.005c
	const wchar* getRuinsName() const; // advc.005c

	CvCity* getWorkingCity() const;																																				// Exposed to Python
	void updateWorkingCity();

	CvCity* getWorkingCityOverride() const;																															// Exposed to Python
	void setWorkingCityOverride( const CvCity* pNewValue);

	int getRiverID() const;																																							// Exposed to Python
	void setRiverID(int iNewValue);																																			// Exposed to Python

	int getMinOriginalStartDist() const;																																// Exposed to Python
	void setMinOriginalStartDist(int iNewValue);

	int getReconCount() const;																																					// Exposed to Python
	void changeReconCount(int iChange);

	int getRiverCrossingCount() const;																																	// Exposed to Python
	void changeRiverCrossingCount(int iChange);

	bool isHabitable(bool bIgnoreSea = false) const; // advc.300
	short* getYield();
	DllExport int getYield(YieldTypes eIndex) const;																										// Exposed to Python
	int calculateNatureYield(YieldTypes eIndex, TeamTypes eTeam, bool bIgnoreFeature = false) const;		// Exposed to Python
	int calculateBestNatureYield(YieldTypes eIndex, TeamTypes eTeam) const;															// Exposed to Python
	int calculateTotalBestNatureYield(TeamTypes eTeam) const;																						// Exposed to Python
	// BETTER_BTS_AI_MOD, City AI, 10/06/09, jdog5000:
	int calculateImprovementYieldChange(ImprovementTypes eImprovement, YieldTypes eYield, PlayerTypes ePlayer, bool bOptimal = false, bool bBestRoute = false) const;	// Exposed to Python

	int calculateYield(YieldTypes eIndex, bool bDisplay = false) const;												// Exposed to Python
	bool hasYield() const;																																		// Exposed to Python
	void updateYield();
	// int calculateMaxYield(YieldTypes eYield) const; // disabled by K-Mod
	int getYieldWithBuild(BuildTypes eBuild, YieldTypes eYield, bool bWithUpgrade) const;

	int getCulture(PlayerTypes eIndex) const;																									// Exposed to Python
		// advc.003b: Replaced by getTotalCulture
		private: int countTotalCulture() const; public:
	int getTotalCulture() const;
	int countFriendlyCulture(TeamTypes eTeam) const;
	TeamTypes findHighestCultureTeam() const;																														// Exposed to Python
	PlayerTypes findHighestCulturePlayer(
			bool bAlive = false) const; // advc.035
	int calculateCulturePercent(PlayerTypes eIndex) const;																		// Exposed to Python
	int calculateTeamCulturePercent(TeamTypes eIndex) const;																						// Exposed to Python
	void setCulture(PlayerTypes eIndex, int iNewValue, bool bUpdate, bool bUpdatePlotGroups);																		// Exposed to Python
	void changeCulture(PlayerTypes eIndex, int iChange, bool bUpdate);																	// Exposed to Python

	int countNumAirUnits(TeamTypes eTeam) const;																					// Exposed to Python
	int airUnitSpaceAvailable(TeamTypes eTeam) const;
	// <advc.081>
	int countAreaHostileUnits(PlayerTypes ePlayer, CvArea* pArea, bool bPlayer,
			bool bTeam, bool bNeutral, bool bHostile) const; // </advc.081>
	int getFoundValue(PlayerTypes eIndex,												// Exposed to Python
			bool bRandomize = false) const; // advc.052
	bool isBestAdjacentFound(PlayerTypes eIndex);										// Exposed to Python
	void setFoundValue(PlayerTypes eIndex, short iNewValue);
	// K-Mod: I've changed iNewValue to be 'short' instead of 'int', so that it matches the cache.

	int getPlayerCityRadiusCount(PlayerTypes eIndex) const;																							// Exposed to Python
	bool isPlayerCityRadius(PlayerTypes eIndex) const;																									// Exposed to Python
	void changePlayerCityRadiusCount(PlayerTypes eIndex, int iChange);

	CvPlotGroup* getPlotGroup(PlayerTypes ePlayer) const;
	CvPlotGroup* getOwnerPlotGroup() const;
	void setPlotGroup(PlayerTypes ePlayer, CvPlotGroup* pNewValue);
	void updatePlotGroup();
	void updatePlotGroup(PlayerTypes ePlayer, bool bRecalculate = true);

	int getVisibilityCount(TeamTypes eTeam) const;																											// Exposed to Python
	void changeVisibilityCount(TeamTypes eTeam, int iChange,												// Exposed to Python
			InvisibleTypes eSeeInvisible, bool bUpdatePlotGroups,
			CvUnit* pUnit = NULL); // advc.071

	int getStolenVisibilityCount(TeamTypes eTeam) const;																								// Exposed to Python
	void changeStolenVisibilityCount(TeamTypes eTeam, int iChange);

	int getBlockadedCount(TeamTypes eTeam) const;																								// Exposed to Python
	void changeBlockadedCount(TeamTypes eTeam, int iChange);

	DllExport PlayerTypes getRevealedOwner(TeamTypes eTeam, bool bDebug) const;													// Exposed to Python
	TeamTypes getRevealedTeam(TeamTypes eTeam, bool bDebug) const;														// Exposed to Python
	void setRevealedOwner(TeamTypes eTeam, PlayerTypes eNewValue);
	void updateRevealedOwner(TeamTypes eTeam);

	DllExport bool isRiverCrossing(DirectionTypes eIndex) const;																				// Exposed to Python
	void updateRiverCrossing(DirectionTypes eIndex);
	void updateRiverCrossing();

	DllExport bool isRevealed(TeamTypes eTeam, bool bDebug) const;																								// Exposed to Python
	void setRevealed(TeamTypes eTeam, bool bNewValue, bool bTerrainOnly,									// Exposed to Python
			TeamTypes eFromTeam, bool bUpdatePlotGroup);
	bool isAdjacentRevealed(TeamTypes eTeam,																// Exposed to Python
			bool bSkipOcean = false) const; // advc.250c
	bool isAdjacentNonrevealed(TeamTypes eTeam) const;																				// Exposed to Python

	ImprovementTypes getRevealedImprovementType(TeamTypes eTeam, bool bDebug) const;					// Exposed to Python
	void setRevealedImprovementType(TeamTypes eTeam, ImprovementTypes eNewValue);

	RouteTypes getRevealedRouteType(TeamTypes eTeam, bool bDebug) const;											// Exposed to Python
	void setRevealedRouteType(TeamTypes eTeam, RouteTypes eNewValue);

	int getBuildProgress(BuildTypes eBuild) const;																											// Exposed to Python
	bool changeBuildProgress(BuildTypes eBuild, int iChange,									// Exposed to Python
			//TeamTypes eTeam = NO_TEAM
			PlayerTypes ePlayer); // advc.251
	bool decayBuildProgress(bool bTest = false); // advc.011

	void updateFeatureSymbolVisibility();
	void updateFeatureSymbol(bool bForce = false);
	/*  The plot layout contains bonuses and improvements ---
		it is, like the city layout, passively computed by LSystems */
	DllExport bool isLayoutDirty() const;
	DllExport void setLayoutDirty(bool bDirty);
	DllExport bool isLayoutStateDifferent() const;
	DllExport void setLayoutStateToCurrent();
	bool updatePlotBuilder();

	DllExport void getVisibleImprovementState(ImprovementTypes& eType, bool& bWorked);				// determines how the improvement state is shown in the engine
	DllExport void getVisibleBonusState(BonusTypes& eType, bool& bImproved, bool& bWorked);		// determines how the bonus state is shown in the engine
	bool shouldUsePlotBuilder();
	CvPlotBuilder* getPlotBuilder() { return m_pPlotBuilder; }

	DllExport CvRoute* getRouteSymbol() const;
	void updateRouteSymbol(bool bForce = false, bool bAdjacent = false);

	DllExport CvRiver* getRiverSymbol() const;
	void updateRiverSymbol(bool bForce = false, bool bAdjacent = false);
	void updateRiverSymbolArt(bool bAdjacent = true);

	CvFeature* getFeatureSymbol() const;

	DllExport CvFlagEntity* getFlagSymbol() const;
	CvFlagEntity* getFlagSymbolOffset() const;
	DllExport void updateFlagSymbol();

	DllExport CvUnit* getCenterUnit() const;
	DllExport CvUnit* getDebugCenterUnit() const;
	void setCenterUnit(CvUnit* pNewValue);

	int getCultureRangeCities(PlayerTypes eOwnerIndex, int iRangeIndex) const;														// Exposed to Python
	bool isCultureRangeCity(PlayerTypes eOwnerIndex, int iRangeIndex) const;															// Exposed to Python
	void changeCultureRangeCities(PlayerTypes eOwnerIndex, int iRangeIndex, int iChange, bool bUpdatePlotGroups);

	int getInvisibleVisibilityCount(TeamTypes eTeam, InvisibleTypes eInvisible) const;										// Exposed to Python
	bool isInvisibleVisible(TeamTypes eTeam, InvisibleTypes eInvisible) const;														// Exposed to Python
	void changeInvisibleVisibilityCount(TeamTypes eTeam, InvisibleTypes eInvisible, int iChange);					// Exposed to Python

	int getNumUnits() const;																																		// Exposed to Python
	CvUnit* getUnitByIndex(int iIndex) const;																													// Exposed to Python
	void addUnit(CvUnit* pUnit, bool bUpdate = true);
	void removeUnit(CvUnit* pUnit, bool bUpdate = true);
	DllExport CLLNode<IDInfo>* nextUnitNode(CLLNode<IDInfo>* pNode) const;
	CLLNode<IDInfo>* prevUnitNode(CLLNode<IDInfo>* pNode) const;
	DllExport CLLNode<IDInfo>* headUnitNode() const;
	CLLNode<IDInfo>* tailUnitNode() const;

	int getNumSymbols() const;
	CvSymbol* getSymbol(int iID) const;
	CvSymbol* addSymbol();

	void deleteSymbol(int iID);
	void deleteAllSymbols();

	// Script data needs to be a narrow string for pickling in Python
	CvString getScriptData() const;																											// Exposed to Python
	void setScriptData(const char* szNewValue);																					// Exposed to Python

	bool canTrigger(EventTriggerTypes eTrigger, PlayerTypes ePlayer) const;
	bool canApplyEvent(EventTypes eEvent) const;
	void applyEvent(EventTypes eEvent);

	bool canTrain(UnitTypes eUnit, bool bContinue, bool bTestVisible,
			bool bCheckAirUnitCap = true, // advc.001b
			BonusTypes eAssumeAvailable = NO_BONUS) const; // advc.001u
	bool isEspionageCounterSpy(TeamTypes eTeam) const;

	DllExport int getAreaIdForGreatWall() const;
	DllExport int getSoundScriptId() const;
	DllExport int get3DAudioScriptFootstepIndex(int iFootstepTag) const;
	DllExport float getAqueductSourceWeight() const;  // used to place aqueducts on the map
	DllExport bool shouldDisplayBridge(CvPlot* pToPlot, PlayerTypes ePlayer) const;
	DllExport bool checkLateEra() const;
	void killRandomUnit(PlayerTypes eOwner, DomainTypes eDomain); // advc.300

	void read(FDataStreamBase* pStream);
	void write(FDataStreamBase* pStream);
	// advc.003h: Adopted from We The People mod (devolution)
	static void setMaxVisibilityRangeCache();

protected:

	short m_iX;
	short m_iY;
	int m_iArea;
	mutable CvArea *m_pPlotArea;
	short m_iFeatureVariety;
	short m_iOwnershipDuration;
	short m_iImprovementDuration;
	short m_iUpgradeProgress;
	short m_iForceUnownedTimer;
	short m_iCityRadiusCount;
	int m_iRiverID;
	short m_iMinOriginalStartDist;
	short m_iReconCount;
	short m_iRiverCrossingCount;
	short m_iLatitude; // advc.tsl

	bool m_bStartingPlot:1;
	bool m_bNOfRiver:1;
	bool m_bWOfRiver:1;
	bool m_bIrrigated:1;
	bool m_bPotentialCityWork:1;
	bool m_bShowCitySymbols:1;
	bool m_bFlagDirty:1;
	bool m_bPlotLayoutDirty:1;
	bool m_bLayoutStateWorked:1;

	char /*PlayerTypes*/ m_eOwner;
	short /*PlotTypes*/ m_ePlotType;
	short /*TerrainTypes*/ m_eTerrainType;
	short /*FeatureTypes*/ m_eFeatureType;
	short /*BonusTypes*/ m_eBonusType;
	short /*ImprovementTypes*/ m_eImprovementType;
	short /*RouteTypes*/ m_eRouteType;
	char /*CardinalDirectionTypes*/ m_eRiverNSDirection;
	char /*CardinalDirectionTypes*/ m_eRiverWEDirection;
	char /*PlayerTypes*/ m_eSecondOwner; // advc.035
	IDInfo m_plotCity;
	IDInfo m_workingCity;
	IDInfo m_workingCityOverride;
	// BETTER_BTS_AI_MOD, Efficiency (plot danger cache), 08/21/09, jdog5000: START
	//bool m_bActivePlayerNoDangerCache;
	int m_iActivePlayerSafeRangeCache; // K-Mod (the bbai implementation was flawed)
	bool* m_abBorderDangerCache;
	// BETTER_BTS_AI_MOD: END

	short* m_aiYield;
	int* m_aiCulture;
	short* m_aiFoundValue;
	char* m_aiPlayerCityRadiusCount;
	int* m_aiPlotGroup;			// IDs - keep as int
	short* m_aiVisibilityCount;
	short* m_aiStolenVisibilityCount;
	short* m_aiBlockadedCount;
	char* m_aiRevealedOwner;

	bool* m_abRiverCrossing;	// bit vector
	bool* m_abRevealed;

	short* /*ImprovementTypes*/ m_aeRevealedImprovementType;
	short* /*RouteTypes*/ m_aeRevealedRouteType;

	char* m_szScriptData;

	short* m_paiBuildProgress;
	int m_iTurnsBuildsInterrupted; // advc.011
	CvWString m_szMostRecentCityName; // advc.005c
	int m_iTotalCulture; // advc.003b

	CvFeature* m_pFeatureSymbol;
	CvRoute* m_pRouteSymbol;
	CvRiver* m_pRiverSymbol;
	CvFlagEntity* m_pFlagSymbol;
	CvFlagEntity* m_pFlagSymbolOffset;
	CvUnit* m_pCenterUnit;

	CvPlotBuilder* m_pPlotBuilder;		// builds bonuses and improvements

	char** m_apaiCultureRangeCities;
	short** m_apaiInvisibleVisibilityCount;

	CLinkList<IDInfo> m_units;
	DefenderSelector* m_pDefenderSelector; // advco.defr

	std::vector<CvSymbol*> m_symbols;

	static bool m_bAllFog; // advc.706

	void doFeature();
	void doCulture();

	void processArea(CvArea* pArea, int iChange);
	int calculateLatitude() const; // advc.tsl
	void doImprovementUpgrade();
	// <advc.099b>
	void doCultureDecay();
	int exclusiveRadius(PlayerTypes ePlayer) const;
	// </advc.099b>
	ColorTypes plotMinimapColor();

	static int iMaxVisibilityRangeCache; // advc.003h

	// added so under cheat mode we can access protected stuff
	friend class CvGameTextMgr;
};

#endif
