#pragma once

#ifndef CIV4_GAME_H
#define CIV4_GAME_H

class CvPlot;
class CvCity;
class CvReplayMessage;
class CvReplayInfo;
class CvArtInfoBuilding;
class CvArea;
class CvHallOfFameInfo; // advc.106i
class CvGameAI;
class CvDeal;
class CvCivilization; // advc.003w
class NormalizationTarget; // advc.027
class Shelf; // advc.300
class BarbarianWeightMap; // advc.304
class StartPointsAsHandicap; // advc.250b
class RiseFall; // advc.700

typedef std::vector<const CvReplayMessage*> ReplayMessageList;


class CvGame /* advc.003e: */ : private boost::noncopyable
{
public:

	CvGame();
	virtual ~CvGame();
protected: // advc.003u: Can't easily move these past AI_makeAssignWorkDirty (the EXE relies on the order)
	virtual void read(FDataStreamBase* pStream);
	virtual void write(FDataStreamBase* pStream);
	virtual void writeReplay(FDataStreamBase& stream, PlayerTypes ePlayer);
public:
	// advc.003u: Keep one pure virtual function so that this class is abstract
	virtual void AI_makeAssignWorkDirty() = 0;

	DllExport void init(HandicapTypes eHandicap);
	DllExport void reset(HandicapTypes eHandicap, bool bConstructorCall = false);

	DllExport void setInitialItems();
	// <advc.tsl>
	DllExport void regenerateMap() { regenerateMap(false); } // call from WorldBuilder
	void regenerateMap(bool bAutomated); // </advc.tsl>
	void showDawnOfMan(); // advc.004j
	DllExport void initDiplomacy();
	DllExport void initFreeUnits();

	/* <advc.108>: Three levels of start plot normalization:
	 1: low (weak starting plots on average, high variance); for single-player
	 2: medium (strong starting plots, low variance); for multi-player
	 3: high (very strong starting plots, low variance);  BtS/ K-Mod behavior
	 (the differences between all three aren't very great) */
	enum StartingPlotNormalizationLevel {
		NORMALIZE_DEFAULT, NORMALIZE_LOW, NORMALIZE_MEDIUM, NORMALIZE_HIGH };
	StartingPlotNormalizationLevel getStartingPlotNormalizationLevel() const;
	void setStartingPlotNormalizationLevel(StartingPlotNormalizationLevel eLevel = NORMALIZE_DEFAULT);
	// </advc.108>
	int getStartingPlotRange() const; // advc.opt (exposed to Python via CyPlayer)

	DllExport void update();
	void updateScore(bool bForce = false);
	// <advc.003y>
	int getScoreComponent(int iRawScore, int iInitial, int iMax, int iMultiplier, bool bExponential, bool bFinal, bool bVictory) const; // </advc.003y>
	int getDifficultyForEndScore() const; // advc.250 (exposed to Python; hence public)
	void updateAIHandicap(); // advc.127

	DllExport void updateColoredPlots();
	DllExport void updateBlockadedPlots();
	bool updateNukeAreaOfEffect(CvPlot const* pPlot = NULL) const; // advc.653
	void updateSeaPatrolColors(CvUnit const& kSelectedUnit); // advc.004k

	void updatePlotGroups();
	void updateBuildingCommerce();
	void updateCitySight(bool bIncrement);
	void updateTradeRoutes();
	void updateGwPercentAnger(); // K-Mod

	DllExport void updateSelectionList();
	DllExport void updateTestEndTurn();
	void autoSave(bool bInitial = false); // advc.106l
	DllExport void testExtendedGame();

	// advc
	DllExport CvUnit* getPlotUnit(const CvPlot* pPlot, int iIndex) const { return getPlotUnits(pPlot, NULL, iIndex); }
	// advc
	DllExport void getPlotUnits(const CvPlot *pPlot, std::vector<CvUnit*>& kPlotUnits) const { getPlotUnits(pPlot, &kPlotUnits, -1); }

	DllExport void cycleCities(bool bForward = true, bool bAdd = false) const;							// Exposed to Python
	// <advc.154>
	CvSelectionGroup* getNextGroupInCycle(bool bForward, bool bWorkers, bool& bWrap, CvUnit*& pCycleUnit, std::set<int>* pCycledGroups = NULL) const;
	CvUnit* getCycleButtonUnit(bool bForward, bool bWorkers) const; // </advc.154>						// Exposed to Python
	void cycleSelectionGroups(bool bClear, bool bForward = true, bool bWorkers = false); // Exposed to Python
	// K-Mod:
	void cycleSelectionGroups_delayed(int iDelay, bool bIncremental, bool bDelayOnly = false);
	DllExport bool cyclePlotUnits(CvPlot* pPlot, bool bForward = true, bool bAuto = false, int iCount = -1) const; // Exposed to Python
	DllExport bool selectCity(CvCity* pSelectCity, bool bCtrl, bool bAlt, bool bShift) const;

	DllExport void selectionListMove(CvPlot* pPlot, bool bAlt, bool bShift, bool bCtrl) const;												// Exposed to Python
	DllExport void selectionListGameNetMessage(int eMessage, int iData2 = -1, int iData3 = -1, int iData4 = -1, int iFlags = 0, bool bAlt = false, bool bShift = false) const; // Exposed to Python
	DllExport void selectedCitiesGameNetMessage(int eMessage, int iData2 = -1, int iData3 = -1, int iData4 = -1, bool bOption = false, bool bAlt = false, bool bShift = false, bool bCtrl = false) const; // Exposed to Python
	void cityPushOrder(CvCity* pCity, OrderTypes eOrder, int iData, bool bAlt = false, bool bShift = false, bool bCtrl = false) const; // Exposed to Python

	DllExport void selectUnit(CvUnit* pUnit, bool bClear, bool bToggle = false, bool bSound = false) const;
	DllExport void selectGroup(CvUnit* pUnit, bool bShift, bool bCtrl, bool bAlt) const;
	DllExport void selectAll(CvPlot* pPlot) const;

	DllExport bool selectionListIgnoreBuildingDefense() const;

	DllExport bool canHandleAction(int iAction, CvPlot* pPlot = NULL, bool bTestVisible = false, bool bUseCache = false) const;
	DllExport void setupActionCache() const;
	DllExport void handleAction(int iAction);

	bool canDoControl(ControlTypes eControl) const;
	void doControl(ControlTypes eControl);
	// <K-Mod>
	void retire();
	void enterWorldBuilder(); // </K-Mod>
	void exitToMenu(); // advc
	void setGlobeView(bool b); // advc

	DllExport void implementDeal(PlayerTypes eWho, PlayerTypes eOtherWho, CLinkList<TradeData>* pOurList, CLinkList<TradeData>* pTheirList, bool bForce = false);
	// advc (note): Not const; callee may perform in-place preprocessing.
	// <!-- custom: hoisted from multiline signature between `eOtherWho` and `kOurList` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	void implementDeal(PlayerTypes eWho, PlayerTypes eOtherWho, CLinkList<TradeData>& kOurList, CLinkList<TradeData>& kTheirList, bool bForce = false);
	// <advc.036>
	CvDeal* implementAndReturnDeal(PlayerTypes eWho, PlayerTypes eOtherWho, CLinkList<TradeData>& kOurList, CLinkList<TradeData>& kTheirList, bool bForce = false); // </advc.036>
	void verifyDeals();

	DllExport void getGlobeviewConfigurationParameters(TeamTypes eTeam, bool& bStarsVisible, bool& bWorldIsRound);

	int getSymbolID(int iSymbol) { return gDLL->getInterfaceIFace()->getSymbolID(iSymbol); } // Exposed to Python

	int getProductionPerPopulation(HurryTypes eHurry) const;											// Exposed to Python
	int getHurryAngerLength() const; // advc

	int getAdjustedPopulationPercent(VictoryTypes eVictory) const;										// Exposed to Python
	int getAdjustedLandPercent(VictoryTypes eVictory) const;											// Exposed to Python
	bool isDiploVictoryValid() const; // advc.178 (exposed to Python)
	bool isTeamVote(VoteTypes eVote) const;																// Exposed to Python
	bool isChooseElection(VoteTypes eVote) const;														// Exposed to Python
	bool isTeamVoteEligible(TeamTypes eTeam, VoteSourceTypes eVoteSource) const;						// Exposed to Python
	int countVote(const VoteTriggeredData& kData, PlayerVoteTypes eChoice) const;
	int countPossibleVote(VoteTypes eVote, VoteSourceTypes eVoteSource) const;							// Exposed to Python
	TeamTypes findHighestVoteTeam(const VoteTriggeredData& kData) const;
	int getVoteRequired(VoteTypes eVote, VoteSourceTypes eVoteSource) const;							// Exposed to Python
	TeamTypes getSecretaryGeneral(VoteSourceTypes eVoteSource) const;									// Exposed to Python
	bool canHaveSecretaryGeneral(VoteSourceTypes eVoteSource) const;									// Exposed to Python
	void clearSecretaryGeneral(VoteSourceTypes eVoteSource);
	void updateSecretaryGeneral();

	int countCivPlayersAlive() const;																	// Exposed to Python
	int countCivPlayersEverAlive() const;																// Exposed to Python
	int countCivTeamsAlive() const;																		// Exposed to Python
	int countCivTeamsEverAlive() const;																	// Exposed to Python
	int countHumanPlayersAlive() const;																	// Exposed to Python
	int countFreeTeamsAlive() const; // K-Mod
	// advc.137: Replaces getDefaultPlayers for most purposes
	int getRecommendedPlayers() const;
	int getSeaLevelChange() const; // advc.137, advc.140

	int countTotalCivPower() const;																		// Exposed to Python
	int countTotalNukeUnits() const;																	// Exposed to Python
	int countKnownTechNumTeams(TechTypes eTech) const;													// Exposed to Python
	int getNumFreeBonuses(BuildingTypes eBuilding) const;												// Exposed to Python

	int countReligionLevels(ReligionTypes eReligion) const;												// Exposed to Python
	int calculateReligionPercent(ReligionTypes eReligion, bool bIgnoreOtherReligions = false) const; // advc.115b; Exposed to Python
	int countCorporationLevels(CorporationTypes eCorporation) const;									// Exposed to Python
	void replaceCorporation(CorporationTypes eOldCorp, CorporationTypes eNewCorp);

	int goldenAgeLength() const;																		// Exposed to Python
	int victoryDelay(VictoryTypes eVictory) const;														// Exposed to Python
	int getImprovementUpgradeTime(ImprovementTypes eImprovement) const;									// Exposed to Python
	int getSpeedPercent() const; // advc.252

	// <!-- custom: compute mapname once per map load (new game, load save file) so we don't have to do it everytime (e.g. for each unit order and at each turn).
	// I don't know too much about these although it was my idea to do so, code provided with the help of chatgpt 5 thanks. -->
	// Fast, cached answers computed once per map/load
	bool isLandHeavyMapnameCached() const { return m_bLandHeavyMapname; }
	bool isNavalHeavyMapnameCached() const { return m_bNavalHeavyMapname; }

	bool canTrainNukes() const;																			// Exposed to Python
	EraTypes getCurrentEra() const;																		// Exposed to Python
	EraTypes getHighestEra() const; // advc
	scaled groundbreakingNormalizationModifier(TechTypes eTech) const; // advc.groundbr

	// advc.opt (cached)
	DllExport TeamTypes getActiveTeam() const { return GC.getInitCore().getActiveTeam(); } // Exposed to Python
	CivilizationTypes getActiveCivilizationType() const;												// Exposed to Python
	CvCivilization const* getActiveCivilization() const; // advc.003w

	DllExport bool isNetworkMultiPlayer() const { return GC.getInitCore().getMultiplayer(); } // Exposed to Python
	DllExport bool isGameMultiPlayer() const;															// Exposed to Python
	DllExport bool isTeamGame() const;																	// Exposed to Python

	bool isModem() const;
	void setModem(bool bModem);

	DllExport void reviveActivePlayer();																// Exposed to Python
	DllExport int getNumHumanPlayers() { return GC.getInitCore().getNumHumans(); } // Exposed to Python
	DllExport int getGameTurn() // <advc> Need a const version											// Exposed to Python
	{	CvGame const& kThis = *this;
		return kThis.getGameTurn();
	}
	int getGameTurn() const { return GC.getInitCore().getGameTurn(); } // </advc>
	void setGameTurn(int iNewValue);																	// Exposed to Python
	void incrementGameTurn();

	int getTurnYear(int iGameTurn) const;																// Exposed to Python
	int getGameTurnYear() const { return getTurnYear(getGameTurn()); } // Exposed to Python
	int getElapsedGameTurns() const { return m_iElapsedGameTurns; } // Exposed to Python
	void incrementElapsedGameTurns();

	// <!-- custom: Persist the source revision history of this game independently of the current runtime resolver, so later UI/logs can distinguish the creation source from subsequent loads under newer builds. (ChatGPT-5.6-Sol) -->
	int getNumSASVersionHistoryEntries() const { return (int)m_aSASVersionHistory.size(); }
	int getSASVersionHistoryTurn(int iIndex) const;
	char const* getSASVersionHistoryVersion(int iIndex) const;
	char const* getSASVersionHistoryCommitHash(int iIndex) const;
	int getSASVersionHistoryDirtyState(int iIndex) const;
	void initializeSASVersionHistoryForNewGame();
	// <!-- custom: Expose only the persisted checkpoint loaded from the current save; live saves are logged directly while serializing and do not mutate synchronized game state merely to remember diagnostic wall-clock metadata. (ChatGPT-5.6-Sol) -->
	bool hasSASLoadedSaveCheckpoint() const { return m_iSASLoadedSaveCheckpointTurn >= 0; }
	int getSASLoadedSaveCheckpointTurn() const { return m_iSASLoadedSaveCheckpointTurn; }
	int getSASLoadedSaveCheckpointElapsedTurn() const { return m_iSASLoadedSaveCheckpointElapsedTurn; }
	uint getSASLoadedSaveCheckpointMapRandState() const { return m_uiSASLoadedSaveCheckpointMapRandState; }
	uint getSASLoadedSaveCheckpointSyncRandState() const { return m_uiSASLoadedSaveCheckpointSyncRandState; }
	char const* getSASLoadedSaveCheckpointUtc() const { return m_szSASLoadedSaveCheckpointUtc.c_str(); }
	char const* getSASLoadedSaveCheckpointKind() const { return m_szSASLoadedSaveCheckpointKind.c_str(); }
	char const* getSASLoadedSaveCheckpointReason() const { return m_szSASLoadedSaveCheckpointReason.c_str(); }
	char const* getSASLoadedSaveCheckpointOriginalBasename() const { return m_szSASLoadedSaveCheckpointOriginalBasename.c_str(); }
	// <!-- custom: SASFastSave already owns its exact final target basename before calling CyGame.saveGame.
	// Carry that one-shot context into CvGame::write so the serialized checkpoint and later loaded-save record can identify the original SASFastSave without duplicating filename construction in C++. (ChatGPT-5.6-Sol) -->
	void setSASFastSaveContext(char const* szReason, char const* szOriginalBasename);
	void clearSASFastSaveContext();

	int AIHandicapAdjustment() const; // advc.251

	int getMaxTurns() const { return GC.getInitCore().getMaxTurns(); } // Exposed to Python
	void setMaxTurns(int iNewValue);																	// Exposed to Python
	void changeMaxTurns(int iChange);																	// Exposed to Python

	int getMaxCityElimination() const { return GC.getInitCore().getMaxCityElimination(); } // Exposed to Python
	void setMaxCityElimination(int iNewValue);															// Exposed to Python

	int getNumAdvancedStartPoints() const { return GC.getInitCore().getNumAdvancedStartPoints(); } // Exposed to Python
	void setNumAdvancedStartPoints(int iNewValue);														// Exposed to Python

	int getStartTurn() const { return m_iStartTurn; } // Exposed to Python
	void setStartTurn(int iNewValue);

	int getStartYear() const { return m_iStartYear; } // Exposed to Python
	void setStartYear(int iNewValue);																	// Exposed to Python

	int getEstimateEndTurn() const { return m_iEstimateEndTurn; } // Exposed to Python
	void setEstimateEndTurn(int iNewValue);																// Exposed to Python
	scaled gameTurnProgress(int iDelay = 0) const; // advc

	DllExport int getTurnSlice() const { return m_iTurnSlice; }											// Exposed to Python
	int getMinutesPlayed() const;																		// Exposed to Python
	void setTurnSlice(int iNewValue);
	void changeTurnSlice(int iChange);

	int getCutoffSlice() const;
	void setCutoffSlice(int iNewValue);
	void changeCutoffSlice(int iChange);
	DllExport int getTurnSlicesRemaining()
	// <advc> Need a const version
	{	CvGame const& kThis = *this;
		return kThis.getTurnSlicesRemaining();
	}
	int getTurnSlicesRemaining() const { return getCutoffSlice() - getTurnSlice(); };
	// </advc>
	void resetTurnTimer();
	void incrementTurnTimer(int iNumTurnSlices);
	int getMaxTurnLen();

	int getTargetScore() const { return GC.getInitCore().getTargetScore(); } // Exposed to Python
	void setTargetScore(int iNewValue);																	// Exposed to Python

	int getNumGameTurnActive() { return m_iNumGameTurnActive; }											// Exposed to Python
	DllExport int countNumHumanGameTurnActive() const;													// Exposed to Python
	void changeNumGameTurnActive(int iChange);

	int getNumCities() const { return m_iNumCities; } // Exposed to Python
	int getNumCivCities() const;																		// Exposed to Python
	void changeNumCities(int iChange);

	int getTotalPopulation() const { return m_iTotalPopulation; } // Exposed to Python
	void changeTotalPopulation(int iChange);

	int getTradeRoutes() const { return m_iTradeRoutes; } // Exposed to Python
	void changeTradeRoutes(int iChange);																// Exposed to Python

	int getFreeTradeCount() const { return m_iFreeTradeCount; } // Exposed to Python
	bool isFreeTrade() const { return (getFreeTradeCount() > 0); } // Exposed to Python
	void changeFreeTradeCount(int iChange);																// Exposed to Python

	int getNoNukesCount() const { return m_iNoNukesCount; } // Exposed to Python
	bool isNoNukes() const { return (getNoNukesCount() > 0); } // Exposed to Python
	void changeNoNukesCount(int iChange);																// Exposed to Python

	int getSecretaryGeneralTimer(VoteSourceTypes eVoteSource) const { return m_aiSecretaryGeneralTimer.get(eVoteSource); } // Exposed to Python
	void setSecretaryGeneralTimer(VoteSourceTypes eVoteSource, int iNewValue);
	void changeSecretaryGeneralTimer(VoteSourceTypes eVoteSource, int iChange);

	int getVoteTimer(VoteSourceTypes eVoteSource) const { return m_aiVoteTimer.get(eVoteSource); } // Exposed to Python
	void setVoteTimer(VoteSourceTypes eVoteSource, int iNewValue);
	void changeVoteTimer(VoteSourceTypes eVoteSource, int iChange);

	int getNukesExploded() const { return m_iNukesExploded; } // Exposed to Python
	void changeNukesExploded(int iChange);																// Exposed to Python

	int getMaxPopulation() const { return m_iMaxPopulation; }											// Exposed to Python
	int getMaxLand() const { return m_iMaxLand; }														// Exposed to Python
	int getMaxTech() const { return m_iMaxTech; }														// Exposed to Python
	int getMaxWonders() const { return m_iMaxWonders; }													// Exposed to Python
	int getInitPopulation() const { return m_iInitPopulation; }											// Exposed to Python
	int getInitLand() const { return m_iInitLand; }														// Exposed to Python
	int getInitTech() const { return m_iInitTech; }														// Exposed to Python
	int getInitWonders() const { return m_iInitWonders; }												// Exposed to Python
	/*	<advc> Asset score functions moved from CvGameCoreUtils. Could be static -
		but let's not commit to that. */
	int getPopulationAsset(int iPopulation) const { return iPopulation * 2; }
	int getLandPlotsAsset(int iLandPlots) const { return iLandPlots; }
	int getPopulationPower(int iPopulation) const { return iPopulation / 2; }
	int getPopulationScore(int iPopulation) const { return iPopulation; }
	int getLandPlotsScore(int iLandPlots) const { return iLandPlots; }
	int getTechScore(TechTypes eTech) const { return GC.getInfo(eTech).getEra() + 1; }
	int getWonderScore(BuildingClassTypes eWonderClass) const; // </advc>
	DllExport void initScoreCalculation();

	int getAIAutoPlay() const { return m_iAIAutoPlay; } // Exposed to Python
	// <advc.127>
	DllExport void setAIAutoPlay(int iNewValue) { setAIAutoPlay(iNewValue, true); } // Exposed to Python
	// <!-- custom: Added eEndCause so callers can preserve why automation stopped instead of forcing SASGameRecord to infer it from an ambiguous counter reset. See KI#203. (GPT-5.6-Sol) -->
	void setAIAutoPlay(int iNewValue, bool bChangePlayerStatus, SASAutoPlayEndCause eEndCause = SAS_AUTOPLAY_END_UNSPECIFIED); // </advc.127>
	void changeAIAutoPlay(int iChange, /* advc.127: */ bool bChangePlayerStatus = true, SASAutoPlayEndCause eEndCause = SAS_AUTOPLAY_END_UNSPECIFIED);
	// <advc.opt>
	int getCivPlayersEverAlive() const;
	void changeCivPlayersEverAlive(int iChange);
	int getCivTeamsEverAlive() const;
	void changeCivTeamsEverAlive(int iChange);
	// </advc.opt>
	// <K-Mod>
	int getGlobalWarmingIndex() const { return m_iGlobalWarmingIndex; }									// Exposed to Python
	void setGlobalWarmingIndex(int iNewValue);
	void changeGlobalWarmingIndex(int iChange);
	int getGlobalWarmingChances() const;																// Exposed to Python
	int getGwEventTally() const { return m_iGwEventTally; }												// Exposed to Python
	void setGwEventTally(int iNewValue);
	void changeGwEventTally(int iChange);
	int calculateGlobalPollution() const;																// Exposed to Python
	int calculateGwLandDefence(PlayerTypes ePlayer = NO_PLAYER) const;									// Exposed to Python
	int calculateGwSustainabilityThreshold(PlayerTypes ePlayer = NO_PLAYER) const;						// Exposed to Python
	int calculateGwSeverityRating() const;																// Exposed to Python
	// </K-Mod>
	unsigned int getInitialTime();
	DllExport void setInitialTime(unsigned int uiNewValue);

	bool isScoreDirty() const;																			// Exposed to Python
	void setScoreDirty(bool bNewValue);																	// Exposed to Python
	// <advc.003r> Akin to deferCall in BugUtil.py
	enum UpdateTimerTypes {
		UPDATE_COLLAPSE_SCORE_BOARD, // advc.085
		UPDATE_DIRTY_SCORE_HELP, // advc.085
		UPDATE_MOUSE_FOCUS, // advc.001w
		UPDATE_LOOK_AT_STARTING_PLOT, // advc.004j
		UPDATE_STORE_REPLAY_TEXTURE, // advc.106n
		UPDATE_REBUILD_PLOTS, // advc.001
		NUM_UPDATE_TIMER_TYPES
	};
	void setUpdateTimer(UpdateTimerTypes eTimerType, int iDelay);
	// Unused so far (x3)
	void cancelUpdateTimer(UpdateTimerTypes eTimerType) { setUpdateTimer(eTimerType, -1); }
	int getUpdateTimer(UpdateTimerTypes eTimerType) const;
	bool isUpdatePending(UpdateTimerTypes eTimerType) const { return (getUpdateTimer(eTimerType) >= 0); } // </advc.003r>

	bool isCircumnavigated() const { return m_bCircumnavigated; }										// Exposed to Python
	void makeCircumnavigated();																			// Exposed to Python
	bool circumnavigationAvailable() const;

	bool isDiploVote(VoteSourceTypes eVoteSource) const { return (getDiploVoteCount(eVoteSource) > 0); } // Exposed to Python
	int getDiploVoteCount(VoteSourceTypes eVoteSource) const { return m_aiDiploVote.get(eVoteSource); }
	void changeDiploVote(VoteSourceTypes eVoteSource, int iChange);																					// Exposed to Python
	bool canDoResolution(VoteSourceTypes eVoteSource, VoteSelectionSubData const& kData) const;
	bool isValidVoteSelection(VoteSourceTypes eVoteSource, VoteSelectionSubData const& kData) const;

	DllExport bool isDebugMode() const { return m_bDebugModeCache; } // Exposed to Python
	DllExport void toggleDebugMode();																	// Exposed to Python
	DllExport void updateDebugModeCache();
	bool isDebugToolsAllowed(bool bWB) const; // advc.135c
	bool isGameNameEnableDebugTools(CvWString const& kGameName) const; // advc.135c
	DllExport int getPitbossTurnTime() const;															// Exposed to Python
	DllExport void setPitbossTurnTime(int iHours);														// Exposed to Python

	DllExport bool isHotSeat() const { return GC.getInitCore().getHotseat(); } // Exposed to Python
	DllExport bool isPbem() const { return GC.getInitCore().getPbem(); } // Exposed to Python
	DllExport bool isPitboss() const { return GC.getInitCore().getPitboss(); } // Exposed to Python
	bool isSimultaneousTeamTurns() const;																// Exposed to Python

	DllExport bool isFinalInitialized() const { return m_bFinalInitialized; } // Exposed to Python
	DllExport void setFinalInitialized(bool bNewValue);
	// <advc.004x>
	void setDawnOfManShown(bool b);
	bool isAboutToShowDawnOfMan() const; // </advc.004x>
	// <advc.061>
	void setScreenDimensions(int iWidth, int iHeight); // (exposed to Python)
	int getScreenWidth() const;
	int getScreenHeight() const;
	// </advc.061>  <advc.004n>
	void changePlotListShift(int iChange) { m_iPlotListShift += iChange; }
	int getPlotListShift() const { return m_iPlotListShift; }
	void onCityScreenChange(); // </advc.004n>
	bool getPbemTurnSent() const;
	DllExport void setPbemTurnSent(bool bNewValue);

	DllExport bool getHotPbemBetweenTurns() const;
	void setHotPbemBetweenTurns(bool bNewValue);

	bool isPlayerOptionsSent() const;
	void sendPlayerOptions(bool bForce = false);

	DllExport PlayerTypes getActivePlayer() const { return GC.getInitCore().getActivePlayer(); } // Exposed to Python
	DllExport void setActivePlayer(PlayerTypes eNewValue, bool bForceHotSeat = false);					// Exposed to Python
	void updateActiveVisibility(); // advc.706
	DllExport void updateUnitEnemyGlow();
	/* <advc.106b> When a DLL function is called from the EXE, there is no (other)
	   way to determine whether it's during a human turn.
	   (Or would CvPlayer::isTurnActive work? But that's not as convenient ...) */
	// Also used for various other AdvCiv changes
	bool isInBetweenTurns() const { return m_bInBetweenTurns; }
	void setInBetweenTurns(bool b); // </advc.106b>

	HandicapTypes getHandicapType() const { return m_eHandicap; }										// Exposed to Python
	void setHandicapType(HandicapTypes eHandicap);
	HandicapTypes getAIHandicap() const { return m_eAIHandicap; } // advc.127 (advc.708: exposed to Python)

	DllExport PlayerTypes getPausePlayer() const;														// Exposed to Python
	DllExport bool isPaused() const;																									// Exposed to Python
	DllExport void setPausePlayer(PlayerTypes eNewValue);

	UnitTypes getBestLandUnit() const;																	// Exposed to Python
	int getBestLandUnitCombat() const;																	// Exposed to Python
	void setBestLandUnit(UnitTypes eNewValue);

	TeamTypes getWinner() const { return m_eWinner; } // Exposed to Python
	VictoryTypes getVictory() const { return m_eVictory; } // Exposed to Python
	void setWinner(TeamTypes eNewWinner, VictoryTypes eNewVictory);										// Exposed to Python

	DllExport GameStateTypes getGameState() const { return m_eGameState; } // Exposed to Python
	DllExport void setGameState(GameStateTypes eNewValue);

	// advc.106h:
	PlayerTypes getInitialActivePlayer() const { return m_eInitialActivePlayer; }
	EraTypes getStartEra() const { return GC.getInitCore().getEra(); } // Exposed to Python
	CalendarTypes getCalendar() const { return GC.getInitCore().getCalendar(); } // Exposed to Python
	GameSpeedTypes getGameSpeedType() const { return GC.getInitCore().getGameSpeed(); } // Exposed to Python

	PlayerTypes getRankPlayer(PlayerTypes eRank) const { return m_aeRankPlayer.get(eRank); } // Exposed to Python
	void setRankPlayer(PlayerTypes eRank, PlayerTypes ePlayer);
	// advc (note): The topmost rank is 0
	PlayerTypes getPlayerRank(PlayerTypes ePlayer) const { return m_aePlayerRank.get(ePlayer); } // Exposed to Python
	void setPlayerRank(PlayerTypes ePlayer, PlayerTypes eRank);
	DllExport int getPlayerScore(PlayerTypes ePlayer) const { return m_aiPlayerScore.get(ePlayer); } // Exposed to Python
	void setPlayerScore(PlayerTypes ePlayer, int iScore);
	TeamTypes getRankTeam(TeamTypes eRank) const { return m_aeRankTeam.get(eRank); } // Exposed to Python
	void setRankTeam(TeamTypes eRank, TeamTypes eTeam);
	int getTeamRank(TeamTypes eTeam) const { return m_aeTeamRank.get(eTeam); } // Exposed to Python
	void setTeamRank(TeamTypes eTeam, TeamTypes eRank);
	DllExport int getTeamScore(TeamTypes eTeam) const { return m_aiTeamScore.get(eTeam); } // Exposed to Python
	void setTeamScore(TeamTypes eTeam, int iScore);

	DllExport bool isOption(GameOptionTypes eIndex) const { return GC.getInitCore().getOption(eIndex); } // Exposed to Python
	void setOption(GameOptionTypes eIndex, bool bEnabled);												// Exposed to Python

	DllExport bool isMPOption(MultiplayerOptionTypes eIndex) const { return GC.getInitCore().getMPOption(eIndex); } // Exposed to Python
	void setMPOption(MultiplayerOptionTypes eIndex, bool bEnabled);

	bool isForcedControl(ForceControlTypes eIndex) const { return GC.getInitCore().getForceControl(eIndex); } // Exposed to Python
	void setForceControl(ForceControlTypes eIndex, bool bEnabled);
	// <advc>
	bool canConstruct(BuildingTypes eBuilding, bool bIgnoreCost, bool bTestVisible) const;
	bool canTrain(UnitTypes eUnit, bool bIgnoreCost, bool bTestVisible) const;
	// </advc>
	int getUnitCreatedCount(UnitTypes eUnit) const { return m_aiUnitCreatedCount.get(eUnit); } // Exposed to Python
	void incrementUnitCreatedCount(UnitTypes eUnit);
	int getUnitClassCreatedCount(UnitClassTypes eUnitClass) const { return m_aiUnitClassCreatedCount.get(eUnitClass); } // Exposed to Python
	bool isUnitClassMaxedOut(UnitClassTypes eUnitClass, int iExtra = 0) const;							// Exposed to Python
	void incrementUnitClassCreatedCount(UnitClassTypes eUnitClass);
	int getBuildingClassCreatedCount(BuildingClassTypes eBuildingClass) const { return m_aiBuildingClassCreatedCount.get(eBuildingClass); } // Exposed to Python
	bool isBuildingClassMaxedOut(BuildingClassTypes eBuildingClass, int iExtra = 0) const;				// Exposed to Python
	void incrementBuildingClassCreatedCount(BuildingClassTypes eBuildingClass);
	int getProjectCreatedCount(ProjectTypes eProject) const { return m_aiProjectCreatedCount.get(eProject); } // Exposed to Python
	bool isProjectMaxedOut(ProjectTypes eProject, int iExtra = 0) const;								// Exposed to Python
	void incrementProjectCreatedCount(ProjectTypes eProject, int iExtra = 1);

	int getForceCivicCount(CivicTypes eCivic) const { return m_aiForceCivicCount.get(eCivic); } // Exposed to Python
	bool isForceCivic(CivicTypes eCivic) const { return (getForceCivicCount(eCivic) > 0); } // Exposed to Python
	bool isForceCivicOption(CivicOptionTypes eCivicOption) const;										// Exposed to Python
	void changeForceCivicCount(CivicTypes eCivic, int iChange);
	int getMaxConscript(CivicTypes eCivic) const;

	PlayerVoteTypes getVoteOutcome(VoteTypes eVote) const { return m_aiVoteOutcome.get(eVote); } // Exposed to Python
	bool isVotePassed(VoteTypes eVote) const;															// Exposed to Python
	void setVoteOutcome(VoteTriggeredData const& kData, PlayerVoteTypes eNewValue);

	bool isVictoryValid(VictoryTypes eVictory) const { return GC.getInitCore().getVictory(eVictory); } // Exposed to Python
	void setVictoryValid(VictoryTypes eVictory, bool bValid); // (advc: Exposed to Python)

	bool isSpecialUnitValid(SpecialUnitTypes eSpecialUnit) const { return m_abSpecialUnitValid.get(eSpecialUnit); } // Exposed to Python
	void makeSpecialUnitValid(SpecialUnitTypes eSpecialUnit);											// Exposed to Python
	bool isSpecialBuildingValid(SpecialBuildingTypes eSpecialBuilding) const { return m_abSpecialBuildingValid.get(eSpecialBuilding); } // Exposed to Python
	void makeSpecialBuildingValid(SpecialBuildingTypes eSpecialBuilding, bool bAnnounce = false); // Exposed to Python

	bool isNukesValid() const { return m_bNukesValid; } // Exposed to Python
	void makeNukesValid(bool bValid = true);															// Exposed to Python

	bool isInAdvancedStart() const;																		// Exposed to Python

	void setVoteChosen(int iSelection, int iVoteId);

	int getReligionGameTurnFounded(ReligionTypes eReligion) const { return m_aiReligionGameTurnFounded.get(eReligion); } // Exposed to Python
	bool isReligionFounded(ReligionTypes eReligion) const { return (getReligionGameTurnFounded(eReligion) >= 0); } // Exposed to Python
	void makeReligionFounded(ReligionTypes eReligion, PlayerTypes ePlayer);
	bool isReligionSlotTaken(ReligionTypes eReligion) const { return m_abReligionSlotTaken.get(eReligion); } // Exposed to Python
	void setReligionSlotTaken(ReligionTypes eReligion, bool bTaken);
	CvCity* getHolyCity(ReligionTypes eReligion) const;													// Exposed to Python
	void setHolyCity(ReligionTypes eReligion, CvCity* pCity, bool bAnnounce);							// Exposed to Python

	int getCorporationGameTurnFounded(CorporationTypes eCorp) const { return m_aiCorporationGameTurnFounded.get(eCorp); } // Exposed to Python
	bool isCorporationFounded(CorporationTypes eCorp) const { return (getCorporationGameTurnFounded(eCorp) >= 0); } // Exposed to Python
	void makeCorporationFounded(CorporationTypes eCorp, PlayerTypes ePlayer);
	CvCity* getHeadquarters(CorporationTypes eCorp) const;												// Exposed to Python
	void setHeadquarters(CorporationTypes eCorp, CvCity* pCity, bool bAnnounce);						// Exposed to Python

	PlayerVoteTypes getPlayerVote(PlayerTypes eVoter, int iVote) const;									// Exposed to Python
	void setPlayerVote(PlayerTypes eVoter, int iVote, PlayerVoteTypes eNewValue);
	void castVote(PlayerTypes eVoter, int iVote, PlayerVoteTypes ePlayerVote);

	DllExport CvWString const& getName();
	void setName(TCHAR const* szName);

	// Script data needs to be a narrow string for pickling in Python
	std::string getScriptData() const;																	// Exposed to Python
	void setScriptData(std::string szNewValue);															// Exposed to Python

	// <!-- custom: Past-city lookup compares raw canonical keys/literals and their current rendered text. See KI#325. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	bool isPastCityName(CvWString const& szName) const; // advc.005c: was "isDestroyedCityName"
	void addPastCityName(CvWString const& szName); // advc.005c: was "addDestroyedCityName"

	bool isGreatPersonBorn(CvWString& szName) const;
	void addGreatPersonBornName(CvWString const& szName);

	DllExport int getIndexAfterLastDeal();																// Exposed to Python
	int getNumDeals() { return m_deals.getCount(); }													// Exposed to Python

	DllExport CvDeal* getDeal(int iID) { return m_deals.getAt(iID); } // Exposed to Python
	CvDeal const* getDeal(int iID) const { return m_deals.getAt(iID); } // advc: const version

	CvDeal* addDeal();
	void deleteDeal(int iID);
	// iteration (advc: const)
	//return (!bRev ? m_deals.beginIter(pIterIdx) : m_deals.endIter(pIterIdx));
	// advc.opt
	CvDeal* firstDeal(int *pIterIdx, bool bRev=false) const { FAssert(!bRev); return m_deals.beginIter(pIterIdx); } // Exposed to Python
	//return (!bRev ? m_deals.nextIter(pIterIdx) : m_deals.prevIter(pIterIdx));
	// advc.opt
	CvDeal* nextDeal(int *pIterIdx, bool bRev=false) const { return m_deals.nextIter(pIterIdx); } // Exposed to Python
	// <advc.072>
	CvDeal* nextCurrentDeal(PlayerTypes eGivePlayer, PlayerTypes eReceivePlayer, TradeableItems eItemType, int iData = -1, bool bWidget = false);
	// </advc.072>
	VoteSelectionData* getVoteSelection(int iID) const { return m_voteSelections.getAt(iID); }
	VoteSelectionData* addVoteSelection(VoteSourceTypes eVoteSource);
	void deleteVoteSelection(int iID);

	VoteTriggeredData* getVoteTriggered(int iID) const;
	VoteTriggeredData* addVoteTriggered(VoteSelectionData const& kData, int iChoice);
	VoteTriggeredData* addVoteTriggered(VoteSourceTypes eVoteSource, VoteSelectionSubData const& kOptionData);
	void deleteVoteTriggered(int iID);

	CvRandom& getMapRand() { return m_mapRand; } // Exposed to Python
	int getMapRandNum(int iNum, const char* pszLog) { return m_mapRand.get(iNum, pszLog); }
	CvRandom& getSorenRand() { return m_sorenRand; } // Exposed to Python
	//  Returns a value from the half-closed interval [0,iNum)
	int getSorenRandNum(int iNum, const char* pszLog, int iData1 = MIN_INT, int iData2 = MIN_INT) { return m_sorenRand.getInt(iNum, pszLog, /* advc.007: */ iData1, iData2); } // advc.007
	// (map rand, sync rand)
	std::pair<uint,uint> getInitialRandSeed() const; // advc.027b

	DllExport int calculateSyncChecksum();																// Exposed to Python
	DllExport int calculateOptionsChecksum();															// Exposed to Python
	bool checkInSync(); // advc.001n
	void doFPCheck(int iChecksum, PlayerTypes ePlayer); // advc.003g

	// <advc> Move coords to the end - or to the start.
	// <!-- custom: hoisted from multiline signature between `szText` and `eColor` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	// <!-- custom: Allow private replay events to retain their exact historical observer audience instead of becoming global when shown in Timeline. See KI#337. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	void addReplayMessage(ReplayMessageTypes eType = NO_REPLAY_MESSAGE, PlayerTypes ePlayer = NO_PLAYER, CvWString szText = L"", ColorTypes eColor = NO_COLOR, int iPlotX = INVALID_PLOT_COORD, int iPlotY = INVALID_PLOT_COORD, qword uiVisibilityMask = ~(qword)0);
	void addReplayMessage(CvPlot const& kPlot, ReplayMessageTypes eType = NO_REPLAY_MESSAGE, PlayerTypes ePlayer = NO_PLAYER, CvWString szText = L"", ColorTypes eColor = NO_COLOR); // </advc>
	void clearReplayMessageMap();
	int getReplayMessageTurn(uint i) const;
	ReplayMessageTypes getReplayMessageType(uint i) const;
	int getReplayMessagePlotX(uint i) const;
	int getReplayMessagePlotY(uint i) const;
	PlayerTypes getReplayMessagePlayer(uint i) const;
	LPCWSTR getReplayMessageText(uint i) const;
	uint getNumReplayMessages() const;
	ColorTypes getReplayMessageColor(uint i) const;
	// <!-- custom: Let observer-specific replay snapshots test the historical audience stored beside each message. See KI#337. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	bool isReplayMessageVisibleTo(uint i, PlayerTypes ePlayer) const;
	// <advc>
	void onAllGameDataRead();
	bool isAllGameDataRead() const { return m_bAllGameDataRead; }
	void onGraphicsInitialized(); // </advc>

	CvReplayInfo* getReplayInfo() const;
	DllExport void setReplayInfo(CvReplayInfo* pReplay);
	void saveReplay(PlayerTypes ePlayer);

	bool hasSkippedSaveChecksum() const;																// Exposed to Python

	void addPlayer(PlayerTypes eNewPlayer, LeaderHeadTypes eLeader, CivilizationTypes eCiv);			// Exposed to Python
	// BETTER_BTS_AI_MOD, Debug, 8/1/08, jdog5000:
	void changeHumanPlayer(PlayerTypes eNewHuman, /* advc: */ bool bSetTurnActive = false);

	bool testVictory(VictoryTypes eVictory, TeamTypes eTeam, bool* pbEndScore = NULL) const;

	bool isCompetingCorporation(CorporationTypes eCorp1, CorporationTypes eCorp2) const;
	// advc.enum: Cache this at CvCity instead (implementations deleted)
	/*int getShrineBuildingCount(ReligionTypes eReligion = NO_RELIGION);
	BuildingTypes getShrineBuilding(int eIndex, ReligionTypes eReligion = NO_RELIGION);
	void changeShrineBuilding(BuildingTypes eBuilding, ReligionTypes eReligion, bool bRemove = false);*/

	// (advc, note: Important not to call culturalVictoryNumCultureCities here.)
	bool culturalVictoryValid() const { return (m_iNumCultureVictoryCities > 0); }
	int culturalVictoryNumCultureCities() const;
	CultureLevelTypes culturalVictoryCultureLevel() const;
	int getCultureThreshold(CultureLevelTypes eLevel) const;
	int freeCityCultureFromTrait(TraitTypes eTrait) const; // advc.908b
	// advc.enum: Moved to CvMap
	/*int getPlotExtraYield(int iX, int iY, YieldTypes eYield) const;
	void setPlotExtraYield(int iX, int iY, YieldTypes eYield, int iCost);
	void removePlotExtraYield(int iX, int iY);
	int getPlotExtraCost(int iX, int iY) const;
	void changePlotExtraCost(int iX, int iY, int iCost);
	void removePlotExtraCost(int iX, int iY);*/

	ReligionTypes getVoteSourceReligion(VoteSourceTypes eVoteSource) const { return m_aeVoteSourceReligion.get(eVoteSource); } // Exposed to Python
	void setVoteSourceReligion(VoteSourceTypes eVoteSource, ReligionTypes eReligion, bool bAnnounce = false); // Exposed to Python

	bool isEventActive(EventTriggerTypes eTrigger) const												// Exposed to Python
	{
		FAssert(!isOption(GAMEOPTION_NO_EVENTS)); // advc.003v
		return !m_abInactiveTriggers.get(eTrigger);
	}
	DllExport void initEvents();
		// advc (note): The ..EverActive functions are currently only called from Python (Pedia)
	bool isCivEverActive(CivilizationTypes eCivilization) const;										// Exposed to Python
	bool isLeaderEverActive(LeaderHeadTypes eLeader) const;												// Exposed to Python
	bool isUnitEverActive(UnitTypes eUnit) const;														// Exposed to Python
	bool isBuildingEverActive(BuildingTypes eBuilding) const;											// Exposed to Python
	void processBuilding(BuildingTypes eBuilding, int iChange);
	//bool pythonIsBonusIgnoreLatitudes() const; // advc.003y: Moved to CvPythonCaller

	DllExport void getGlobeLayers(std::vector<CvGlobeLayerData>& aLayers) const;
	DllExport void startFlyoutMenu(CvPlot const* pPlot, std::vector<CvFlyoutMenuData>& aFlyoutItems) const;
	DllExport void applyFlyoutMenu(CvFlyoutMenuData const& kItem);
	DllExport CvPlot* getNewHighlightPlot() const;
	DllExport ColorTypes getPlotHighlightColor(CvPlot* pPlot) const;
	DllExport void cheatSpaceship() const;
	DllExport VictoryTypes getSpaceVictory() const;
	VictoryTypes getDominationVictory() const; // advc.115f
	DllExport void nextActivePlayer(bool bForward);

	// advc.003j (note): Isn't (and imo shouldn't be) used DLL-internally
	DllExport DomainTypes getUnitDomain(UnitTypes eUnit) const;
	DllExport CvArtInfoBuilding const* getBuildingArtInfo(BuildingTypes eBuilding) const;
	DllExport bool isWaterBuilding(BuildingTypes eBuilding) const;
	DllExport CivilopediaWidgetShowTypes getWidgetShow(BonusTypes eBonus) const;
	DllExport CivilopediaWidgetShowTypes getWidgetShow(ImprovementTypes eImprovement) const;

	DllExport void loadBuildQueue(const CvString& strItem) const;

	DllExport int getNextSoundtrack(EraTypes eLastEra, int iLastSoundtrack) const;
	DllExport int getSoundtrackSpace() const;
	DllExport bool isSoundtrackOverride(CvString& strSoundtrack) const;

	DllExport void initSelection() const;
	DllExport bool canDoPing(CvPlot* pPlot, PlayerTypes ePlayer) const;
	DllExport bool shouldDisplayReturn() const;
	DllExport bool shouldDisplayEndTurn() const;
	DllExport bool shouldDisplayWaitingOthers() const;
	DllExport bool shouldDisplayWaitingYou() const;
	DllExport bool shouldDisplayEndTurnButton() const;
	DllExport bool shouldDisplayFlag() const;
	DllExport bool shouldDisplayUnitModel() const;
	DllExport bool shouldShowResearchButtons() const;
	DllExport bool shouldCenterMinimap() const;
	DllExport EndTurnButtonStates getEndTurnState() const;

	void setCityBarWidth(bool bWide); // advc.095 (exposed to Python)
	DllExport void handleCityScreenPlotPicked(CvCity* pCity, CvPlot* pPlot, bool bAlt, bool bShift, bool bCtrl) const;
	DllExport void handleCityScreenPlotDoublePicked(CvCity* pCity, CvPlot* pPlot, bool bAlt, bool bShift, bool bCtrl) const;
	DllExport void handleCityScreenPlotRightPicked(CvCity* pCity, CvPlot* pPlot, bool bAlt, bool bShift, bool bCtrl) const;
	DllExport void handleCityPlotRightPicked(CvCity* pCity, CvPlot* pPlot, bool bAlt, bool bShift, bool bCtrl) const;
	DllExport void handleMiddleMouse(bool bCtrl, bool bAlt, bool bShift);
	DllExport void handleDiplomacySetAIComment(DiploCommentTypes eComment) const;
	void setHelpTextAreaWidth(float fWidth); // advc.092c (exposed to Python)

	scaled goodyHutEffectFactor(bool bSpeedAdjust = true) const; // advc.314
	// <advc.004m>
	GlobeLayerTypes getCurrentLayer() const;
	void reportCurrentLayer(GlobeLayerTypes eLayer);		// (exposed to Python)
	// </advc.004m>  <advc.052>
	bool isScenario() const { return m_bScenario; }			// (exposed to Python)
	void setScenario(bool b);

	// <!-- custom: compute mapname once per map load (new game, load save file) so we don't have to do it everytime (e.g. for each unit order and at each turn).
	// I don't know too much about these although it was my idea to do so, code provided with the help of chatgpt 5 thanks. -->
	// Add these two bools in the protected data block, next to the other booleans (a natural spot is right after m_bScenario / m_bAllGameDataRead group):
	bool m_bLandHeavyMapname;   // computed once after map gen / on load
	bool m_bNavalHeavyMapname;  // same

	// </advc.052>  <advc.127b>
	/*  Returns (-1,-1) if 'vs' doesn't exist in any city or (eObserver!=NO_TEAM)
		isn't revealed to eObserver */
	std::pair<int,int> getVoteSourceXY(VoteSourceTypes eVS, TeamTypes eObserver, bool bDebug = false) const;
	BuildingTypes getVoteSourceBuilding(VoteSourceTypes eVS) const;
	CvCity* getVoteSourceCity(VoteSourceTypes eVS, TeamTypes eObserver, bool bDebug = false) const;
	// </advc.127b>
	bool isFreeStartEraBuilding(BuildingTypes eBuilding) const; // advc
	/*  advc.250b: Used for exposing a StartPointsAsHandicap member function
		to Python. (Don't want to create a Python wrapper just for that one function.) */
	StartPointsAsHandicap const& startPointsAsHandicap() const;
	int getBarbarianStartTurn() const; // advc.300		(exposed to Python)
	bool isBarbarianCreationEra() const; // advc.307
	// <advc.304>
	BarbarianWeightMap& getBarbarianWeightMap() { return *m_pBarbarianWeightMap; }
	BarbarianWeightMap const& getBarbarianWeightMap() const { return *m_pBarbarianWeightMap; }
	// </advc.304>  <advc.703>
	RiseFall const& getRiseFall() const { return *m_pRiseFall; }
	RiseFall& getRiseFall() { return *m_pRiseFall; }
	// </advc.703>
	void setHallOfFame(CvHallOfFameInfo* pHallOfFame); // advc.106i
	std::set<int>& getActivePlayerCycledGroups(); // advc
	// <advc.003u>
	//return *static_cast<CvGameAI*>(const_cast<CvGame*>(this));
	/* The above won't work in an inline function b/c the compiler doesn't know that CvGameAI is derived from CvGame */
	CvGameAI& AI() { return *reinterpret_cast<CvGameAI*>(this); }
	//return *static_cast<CvGameAI const*>(this);
	CvGameAI const& AI() const { return *reinterpret_cast<CvGameAI const*>(this); } // </advc.003u>

protected:
	int m_iElapsedGameTurns;
	int m_iStartTurn;
	int m_iStartYear;
	int m_iEstimateEndTurn;
	int m_iTurnSlice;
	int m_iCutoffSlice;
	int m_iNumGameTurnActive;
	int m_iNumCities;
	int m_iTotalPopulation;
	int m_iTradeRoutes;
	int m_iFreeTradeCount;
	int m_iNoNukesCount;
	int m_iNukesExploded;
	int m_iMaxPopulation;
	int m_iMaxLand;
	int m_iMaxTech;
	int m_iMaxWonders;
	int m_iInitPopulation;
	int m_iInitLand;
	int m_iInitTech;
	int m_iInitWonders;
	int m_iAIAutoPlay;
	int m_iGlobalWarmingIndex;	// K-Mod
	int m_iGwEventTally;		// K-Mod
	int m_iTurnLoadedFromSave; // advc.044
	// <advc.opt>
	mutable int m_iStartingPlotRange;
	int m_iCivPlayersEverAlive;
	int m_iCivTeamsEverAlive;
	// </advc.opt>
	int m_iUnitUpdateAttempts; // advc.001y
	int m_iScreenWidth, m_iScreenHeight; // advc.061
	// <advc.004n>
	int m_iPlotListShift;
	bool m_bCityScreenUp; // </advc.004n>
	unsigned int m_uiInitialTime;
	unsigned int m_uiSaveFlag; // advc

	// <!-- custom: One compact persisted entry per distinct runtime source encountered by this game. Entry 0 is always the creation source; unavailable source metadata stays honestly empty with dirty=-1 rather than inventing provenance. (ChatGPT-5.6-Sol) -->
	struct SASVersionHistoryEntry
	{
		SASVersionHistoryEntry() : iTurn(-1), iDirtyState(-1) {}
		int iTurn;
		int iDirtyState;
		CvString szVersion;
		CvString szCommitHash;
	};
	std::vector<SASVersionHistoryEntry> m_aSASVersionHistory;
	// <!-- custom: Save-checkpoint provenance is embedded only in the serialized script-data copy and stripped again on load, so Python-visible game script data keeps its original contract while a loaded save can identify the exact serialization that produced it. (ChatGPT-5.6-Sol) -->
	int m_iSASLoadedSaveCheckpointTurn;
	int m_iSASLoadedSaveCheckpointElapsedTurn;
	uint m_uiSASLoadedSaveCheckpointMapRandState;
	uint m_uiSASLoadedSaveCheckpointSyncRandState;
	CvString m_szSASLoadedSaveCheckpointUtc;
	CvString m_szSASLoadedSaveCheckpointKind;
	CvString m_szSASLoadedSaveCheckpointReason;
	CvString m_szSASLoadedSaveCheckpointOriginalBasename;
	// <!-- custom: Transient only; Python sets these immediately around a SASFastSave call and they are never ordinary synchronized gameplay/save state. (ChatGPT-5.6-Sol) -->
	CvString m_szSASPendingFastSaveReason;
	CvString m_szSASPendingFastSaveOriginalBasename;

	bool m_bScoreDirty;
	bool m_bCircumnavigated;
	bool m_bDebugMode;
	bool m_bDebugModeCache;
	bool m_bFinalInitialized;
	bool m_bPbemTurnSent;
	bool m_bHotPbemBetweenTurns;
	bool m_bPlayerOptionsSent;
	bool m_bNukesValid;
	bool m_bInBetweenTurns; // advc.106b
	bool m_bFeignSP; // advc.135c
	bool m_bScenario; // advc.052
	bool m_bAllGameDataRead; // advc
	bool m_bDoMShown; // advc.004x
	bool m_bLayerFromSavegame; // advc.004m
	bool m_bFPTestDone; // advc.003g

	HandicapTypes m_eHandicap;
	HandicapTypes m_eAIHandicap; // advc.127
	PlayerTypes m_ePausePlayer;
	UnitTypes m_eBestLandUnit;
	TeamTypes m_eWinner;
	VictoryTypes m_eVictory;
	GameStateTypes m_eGameState;
	PlayerTypes m_eInitialActivePlayer; // advc.106h
	GlobeLayerTypes m_eCurrentLayer; // advc.004m
	//PlayerTypes m_eEventPlayer; // (advc: unused)
	StartingPlotNormalizationLevel m_eNormalizationLevel; // advc.108

	CvRandom m_mapRand;
	CvRandom m_sorenRand;

	CvString m_szScriptData;

	int m_aiUpdateTimers[NUM_UPDATE_TIMER_TYPES]; // advc.003r
	/*	<advc.enum> (NB: Mustn't use eager allocation for dynamic enum types
		b/c XML isn't loaded yet.) */
	EagerEnumMap<PlayerTypes,PlayerTypes> m_aeRankPlayer; // Ordered by rank
	EagerEnumMap<PlayerTypes,PlayerTypes> m_aePlayerRank;
	EagerEnumMap<PlayerTypes,int> m_aiPlayerScore;
	EagerEnumMap<TeamTypes,TeamTypes> m_aeRankTeam; // Ordered by rank
	EagerEnumMap<TeamTypes,TeamTypes> m_aeTeamRank;
	EagerEnumMap<TeamTypes,int> m_aiTeamScore;

	ArrayEnumMap<UnitTypes,int> m_aiUnitCreatedCount;
	ArrayEnumMap<UnitClassTypes,int> m_aiUnitClassCreatedCount;
	ArrayEnumMap<BuildingClassTypes,int> m_aiBuildingClassCreatedCount;
	ArrayEnumMap<ProjectTypes,int> m_aiProjectCreatedCount;
	ArrayEnumMap<CivicTypes,int,char> m_aiForceCivicCount;

	ArrayEnumMap<VoteTypes,PlayerVoteTypes> m_aiVoteOutcome;
	ArrayEnumMap<ReligionTypes,int,void*,-1> m_aiReligionGameTurnFounded;
	ArrayEnumMap<CorporationTypes,int,void*,-1> m_aiCorporationGameTurnFounded;
	ArrayEnumMap<VoteSourceTypes,int> m_aiSecretaryGeneralTimer;
	ArrayEnumMap<VoteSourceTypes,int> m_aiVoteTimer;
	ArrayEnumMap<VoteSourceTypes,int> m_aiDiploVote;
	/*	advc (note): Used to be used for ICBM, unused since BtS.
		SpecialBuildingValid is still used for Bomb Shelters. */
	ArrayEnumMap<SpecialUnitTypes,bool> m_abSpecialUnitValid;
	ArrayEnumMap<SpecialBuildingTypes,bool> m_abSpecialBuildingValid;
	ArrayEnumMap<ReligionTypes,bool> m_abReligionSlotTaken;

	ArrayEnumMap<ReligionTypes,PlotNumTypes> m_aeHolyCity;
	ArrayEnumMap<CorporationTypes,PlotNumTypes> m_aeHeadquarters;
	IDInfo* m_pLegacyOrgSeatData;
	//int** m_apaiPlayerVote; // obsoleted by BtS
	// </advc.enum>
	// advc.005c: Renamed from "DestroyedCities"
	std::vector<CvWString> m_aszPastCities;
	std::vector<CvWString> m_aszGreatPeopleBorn;

	FFreeListTrashArray<VoteSelectionData> m_voteSelections;
	FFreeListTrashArray<VoteTriggeredData> m_votesTriggered;
	FFreeListTrashArray<CvDeal> m_deals;
	/*  <advc.072> Not serialized. One for use by CvPlayer::getItemTradeString,
		the other for CvDLLWidgetData::parseTradeItem. */
	CLinkList<DealItemData> m_currentDeals;
	CLinkList<DealItemData> m_currentDealsWidget;
	mutable bool m_bShowingCurrentDeals;
	// </advc.072>

	// <advc.027b>
	struct InitialRandSeed
	{
		uint uiMap;
		uint uiSync;
	} m_initialRandSeed;
	// </advc.027b>
	ReplayMessageList m_listReplayMessages;
	// <!-- custom: Parallel to m_listReplayMessages; each bit records a player who could see that message when it occurred. See KI#337. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	std::vector<qword> m_auiReplayMessageVisibility;
	CvReplayInfo* m_pReplayInfo;
	int m_iNumSessions;
	int m_iMapRegens; // advc.tsl
	CvHallOfFameInfo* m_pHallOfFame; // advc.106i

	BarbarianWeightMap* m_pBarbarianWeightMap; // advc.304 (serialized by CvMap)
	// <advc.enum> Replacing hash_map, vector
	ArrayEnumMap<VoteSourceTypes,ReligionTypes> m_aeVoteSourceReligion;
	ArrayEnumMap<EventTriggerTypes,bool> m_abInactiveTriggers; // </advc.enum>

	/*  K-Mod. This is used to track which groups have been cycled through in the current turn.
		Note: it does not need to be kept in sync for multiplayer games. */
	std::set<int> m_aiActivePlayerCycledGroups; // advc: Was public; public getter added.

	// cache some frequently used values
	/*int m_iShrineBuildingCount;
	int* m_aiShrineBuilding;
	int* m_aiShrineReligion;*/ // advc.enum: Handled directly by CvCity now
	int m_iNumCultureVictoryCities;
	CultureLevelTypes m_eCultureVictoryCultureLevel;

	StartPointsAsHandicap* m_pSpah; // advc.250b
	RiseFall* m_pRiseFall; // advc.700

	void uninit();
	void setStartTurnYear(int iTurn = 0); // advc.250c
	// <!-- custom: Append current cached ModName version/SHA only when that revision differs from the last persisted entry; older save layouts are intentionally unsupported rather than migrated. (ChatGPT-5.6-Sol) -->
	void updateSASVersionHistoryAfterLoad();
	void appendCurrentSASVersionHistoryIfChanged();
	void initScenario(); // advc.051

	// <!-- custom: compute mapname once per map load (new game, load save file) so we don't have to do it everytime (e.g. for each unit order and at each turn).
	// I don't know too much about these although it was my idea to do so, code provided with the help of chatgpt 5 thanks. -->
	void recomputeMapnameHeaviness();

	void setPlayerColors(); // advc.002i
	void initGameHandicap(); // advc.127
	void initFreeState();
	void initFreeCivState(); // advc.tsl
	/* <advc.027> */ NormalizationTarget* /* </advc.027> */ assignStartingPlots();
	void normalizeStartingPlots(/* advc.027: */ NormalizationTarget const* pTarget = NULL);
	void updateStartingPlotRange() const; // advc.opt
	void applyOptionEffects(bool bEnableAll = false); // advc.310
	void doTurn();
	void doDeals();
	void doGlobalWarming();
	CvPlot* getRandGWPlot(int iPool); // K-Mod
	void doHolyCity();
	// <advc.138>
	int religionPriority(TeamTypes eTeam, ReligionTypes eReligion) const;
	int religionPriority(PlayerTypes ePlayer, ReligionTypes eReligion) const;
	// </advc.138>
	void doHeadquarters();
	void doDiploVote();
	void doVoteResults();
	void doVoteSelection();

	void createBarbarianCities();
	void createBarbarianUnits();
	void createAnimals();
	// <advc.300>
	void createBarbarianCity(bool bNoCivCities, int iProbModifierPercent = 100);
	int numBarbariansToCreate(int iTilesPerUnit, int iTiles, int iUnowned, int iUnitsPresent);
	int createBarbarianUnits(int iUnitsToCreate, int iUnitsPresent, CvArea& kArea, Shelf* pShelf = NULL, bool bCargoAllowed = false, bool bOnlyCargo = false);
	CvPlot* randomBarbarianPlot(int& iValid, CvArea const& kArea, Shelf const* pShelf = NULL);
	bool killBarbarian(int iUnitsPresent, int iTiles, int iPop, CvArea& kArea, Shelf* pShelf = NULL);
	UnitTypes randomBarbarianUnit(UnitAITypes eUnitAI, CvPlot const& kPlot);
	scaled barbarianPeakLandRatio() const;
	// </advc.300>

	void verifyCivics();

	void updateUnprofiled(); // advc.256
	void updateWar();
	void updateMoves();
	void updateTimers();
	void updateTurnTimer();

	void testAlive();
	void testVictory();
	void showEndGameSequence();
	int FPChecksum() const; // advc.003g
	void handleUpdateTimer(UpdateTimerTypes eTimerType); // advc.003r
	bool isValidReplayIndex(uint i) const; // advc

	void processVote(const VoteTriggeredData& kData, int iChange);

	//void normalizeStartingPlotLocations();
	// <advc.108b>
	void rearrangeTeamStarts(bool bOnlyWithinArea = false, scaled rInertia = 0); // advc.027
	void applyStartingLocHandicaps(/* advc.027: */ NormalizationTarget const* pStartValues);
	template<class Agent>
	void sortByStartingLocHandicap(std::vector<std::pair<Agent*, int> > const& kStartingLocPercentPerAgent, std::vector<Agent*>& kResult); // </advc.108b>
	// <advc>
	// <!-- custom: hoisted from multiline signature before `kDistances` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
	int getTeamClosenessScore(ArrayEnumMap2D<PlayerTypes, PlayerTypes, int> const& kDistances, std::vector<PlayerTypes> const& kStartingLocs); // </advc>
	void normalizeAddRiver();
	void normalizeRemovePeaks();
	void normalizeAddLakes();
	void normalizeRemoveBadFeatures();
	void normalizeRemoveBadTerrain();
	void normalizeAddFoodBonuses(/* advc.027: */ NormalizationTarget const* pTarget = NULL);
	void normalizeAddGoodTerrain();
	void normalizeAddExtras(/* advc.027: */ NormalizationTarget const* pTarget = NULL);
	// <advc>
	bool placeExtraBonus(PlayerTypes eStartPlayer, CvPlot& kPlot, bool bCheckCanPlace, bool bIgnoreLatitude, bool bRemoveFeature, bool bNoFood); // advc.108
	bool isNormalizationBonus(BonusTypes eBonus, PlayerTypes eStartPlayer, CvPlot const& kPlot, bool bCheckCanPlace, bool bIgnoreLatitude) const; // </advc>
	CvPlot* normalizeFindLakePlot(PlayerTypes ePlayer);
	// <advc.108>
	bool normalizeCanAddLakeTo(CvPlot const& kPlot) const;
	bool skipDuplicateNormalizationBonus(CvPlot const& kStartPlot, CvPlot const& kPlot, BonusTypes eBonus, bool bSecondPass = false);
	bool isPowerfulStartingBonus(CvPlot const& kPlot, PlayerTypes eStartPlayer) const;
	bool isWeakStartingFoodBonus(CvPlot const& kPlot, PlayerTypes eStartPlayer) const;
	// </advc.108>  <advc>
	void doUpdateCacheOnTurn();
	CvUnit* getPlotUnits(CvPlot const* pPlot, std::vector<CvUnit*>* pPlotUnits, int iIndex = -1) const; // </advc>

private: // advc.003u: (See comments in the private section of CvPlayer.h)
	//virtual void AI_initExternal();
	virtual void AI_resetExternal();
	virtual void AI_makeAssignWorkDirtyExternal();
	virtual void AI_updateAssignWorkExternal();
	virtual int AI_combatValueExternal(UnitTypes eUnit);
};

/*	<advc.007c> Some macros to make the RNG functions of the singleton
	CvGame instance less tedious to use */

// Implementation files can re-define this to use a different CvGame instance
#define CVGAME_INSTANCE_FOR_RNG GC.getGame()

inline CvRandom& syncRand() { return CVGAME_INSTANCE_FOR_RNG.getSorenRand(); }
inline CvRandom& mapRand() { return CVGAME_INSTANCE_FOR_RNG.getMapRand(); }
// These have to be macros to let CALL_LOC_STR expand to the proper code location
#define SyncRandNum(iNumOutcomes) \
	CVGAME_INSTANCE_FOR_RNG.getSorenRandNum((iNumOutcomes), CALL_LOC_STR)
#define SyncRandFract(ScaledNumType) \
	ScaledNumType::rand(syncRand(), CALL_LOC_STR)
#define SyncRandSuccess(rSuccessProb) \
	(rSuccessProb).randSuccess(syncRand(), CALL_LOC_STR)
/*	(Implementing this through conversion to ScaledNum would lead to
	precision problems or unnecessary arithmetic operations.) */
#define SyncRandSuccessRatio(iNumerator, iDenominator) \
	((iNumerator) > 0 && ((iNumerator) >= (iDenominator) || (iNumerator) > SyncRandNum(iDenominator)))
#define SyncRandSuccess100(iSuccessPercent) \
	SyncRandSuccessRatio(iSuccessPercent, 100)
#define SyncRandSuccess1000(iSuccessPermille) \
	SyncRandSuccessRatio(iSuccessPermille, 1000)
#define SyncRandSuccess10000(iSuccessPermyriad) \
	SyncRandSuccessRatio(iSuccessPermyriad, 10000)
// Not sure what to name this one. Success has 1 chance in iNumLots.
#define SyncRandOneChanceIn(iNumLots) \
	(SyncRandNum(iNumLots) == 0)

// Same as above, replacing "Sync" with "Map".
#define MapRandNum(iNumOutcomes) \
	CVGAME_INSTANCE_FOR_RNG.getMapRandNum((iNumOutcomes), CALL_LOC_STR)
#define MapRandFract(ScaledNumType) \
	ScaledNumType::rand(mapRand(), CALL_LOC_STR)
#define MapRandSuccess(rSuccessProb) \
	(rSuccessProb).randSuccess(mapRand(), CALL_LOC_STR)
#define MapRandSuccessRatio(iNumerator, iDenominator) \
	((iNumerator) > 0 && ((iNumerator) >= (iDenominator) || (iNumerator) > MapRandNum(iDenominator)))
#define MapRandSuccess100(iSuccessPercent) \
	MapRandSuccessRatio(iSuccessPercent, 100)
#define MapRandSuccess1000(iSuccessPermille) \
	MapRandSuccessRatio(iSuccessPermille, 1000)
#define MapRandSuccess10000(iSuccessPermyriad) \
	MapRandSuccessRatio(iSuccessPermyriad, 10000)
#define MapRandOneChanceIn(iNumLots) \
	(MapRandNum(iNumLots) == 0)
// </advc.007c>

#endif
