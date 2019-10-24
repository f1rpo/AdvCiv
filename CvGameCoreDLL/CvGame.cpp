// game.cpp

#include "CvGameCoreDLL.h"
#include "CvGame.h"
#include "CvGamePlay.h"
#include "CvMap.h"
#include "CvInitCore.h"
#include "CvMapGenerator.h"
#include "CvArtFileMgr.h"
#include "CvDiploParameters.h"
#include "CvReplayMessage.h"
#include "CyArgsList.h"
#include "CvInfos.h"
#include "CvPopupInfo.h"
#include "CvReplayInfo.h"
#include "CvGameTextMgr.h"
#include "CvEventReporter.h"
#include "CvMessageControl.h"
#include "StartPointsAsHandicap.h" // advc.250b
#include "RiseFall.h" // advc.700
#include "CvHallOfFameInfo.h" // advc.106i

// interface uses
#include "CvDLLInterfaceIFaceBase.h"
#include "CvDLLEngineIFaceBase.h"
#include "CvDLLPythonIFaceBase.h"

#include "BBAILog.h" // BBAI
#include "CvBugOptions.h" // K-Mod

// Public Functions...

CvGame::CvGame()
{
	m_aiRankPlayer = new int[MAX_PLAYERS];        // Ordered by rank...
	m_aiPlayerRank = new int[MAX_PLAYERS];        // Ordered by player ID...
	m_aiPlayerScore = new int[MAX_PLAYERS];       // Ordered by player ID...
	m_aiRankTeam = new int[MAX_TEAMS];						// Ordered by rank...
	m_aiTeamRank = new int[MAX_TEAMS];						// Ordered by team ID...
	m_aiTeamScore = new int[MAX_TEAMS];						// Ordered by team ID...

	m_pSpah = new StartPointsAsHandicap(); // advc.250b
	m_pRiseFall = new RiseFall(); // advc.700

	m_paiUnitCreatedCount = NULL;
	m_paiUnitClassCreatedCount = NULL;
	m_paiBuildingClassCreatedCount = NULL;
	m_paiProjectCreatedCount = NULL;
	m_paiForceCivicCount = NULL;
	m_paiVoteOutcome = NULL;
	m_paiReligionGameTurnFounded = NULL;
	m_paiCorporationGameTurnFounded = NULL;
	m_aiSecretaryGeneralTimer = NULL;
	m_aiVoteTimer = NULL;
	m_aiDiploVote = NULL;

	m_pabSpecialUnitValid = NULL;
	m_pabSpecialBuildingValid = NULL;
	m_abReligionSlotTaken = NULL;

	m_paHolyCity = NULL;
	m_paHeadquarters = NULL;

	m_pReplayInfo = NULL;
	m_pHallOfFame = NULL; // advc.106i

	m_aiShrineBuilding = NULL;
	m_aiShrineReligion = NULL;

	reset(NO_HANDICAP, true);
}


CvGame::~CvGame()
{
	uninit();

	SAFE_DELETE_ARRAY(m_aiRankPlayer);
	SAFE_DELETE_ARRAY(m_aiPlayerRank);
	SAFE_DELETE_ARRAY(m_aiPlayerScore);
	SAFE_DELETE_ARRAY(m_aiRankTeam);
	SAFE_DELETE_ARRAY(m_aiTeamRank);
	SAFE_DELETE_ARRAY(m_aiTeamScore);

	SAFE_DELETE(m_pSpah); // advc.250b
	SAFE_DELETE(m_pRiseFall); // advc.700
}

void CvGame::init(HandicapTypes eHandicap)
{
	int iI;
	CvInitCore& ic = GC.getInitCore();

	//--------------------------------
	// Init saved data
	reset(eHandicap);

	//--------------------------------
	// Init containers
	m_deals.init();
	m_voteSelections.init();
	m_votesTriggered.init();

	m_mapRand.init(ic.getMapRandSeed() % 73637381);
	m_sorenRand.init(ic.getSyncRandSeed() % 52319761);

	//--------------------------------
	// Init non-saved data

	m_bAllGameDataRead = true; // advc.003: Not loading from savegame
	// <advc.108>
	m_iNormalizationLevel = GC.getDefineINT("NORMALIZE_STARTPLOTS_AGGRESSIVELY") > 0 ?
			3 : 1;
	if(m_iNormalizationLevel == 1 && isGameMultiPlayer())
		m_iNormalizationLevel = 2;
	// </advc.108>

	//--------------------------------
	// Init other game data

	// Turn off all MP options if it's a single player game
	if (ic.getType() == GAME_SP_NEW || ic.getType() == GAME_SP_SCENARIO)
	{
		for (iI = 0; iI < NUM_MPOPTION_TYPES; ++iI)
		{
			setMPOption((MultiplayerOptionTypes)iI, false);
		}
	}

	// If this is a hot seat game, simultaneous turns is always off
	if (isHotSeat() || isPbem())
	{
		setMPOption(MPOPTION_SIMULTANEOUS_TURNS, false);
	}
	// If we didn't set a time in the Pitboss, turn timer off
	if (isPitboss() && getPitbossTurnTime() == 0)
	{
		setMPOption(MPOPTION_TURN_TIMER, false);
	}

	if (isMPOption(MPOPTION_SHUFFLE_TEAMS))
	{
		int aiTeams[MAX_CIV_PLAYERS];
		int iNumPlayers = 0;
		for (int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			if (ic.getSlotStatus((PlayerTypes)i) == SS_TAKEN)
			{
				aiTeams[iNumPlayers] = ic.getTeam((PlayerTypes)i);
				++iNumPlayers;
			}
		}

		for (int i = 0; i < iNumPlayers; i++)
		{
			int j = (getSorenRand().get(iNumPlayers - i, NULL) + i);

			if (i != j)
			{
				int iTemp = aiTeams[i];
				aiTeams[i] = aiTeams[j];
				aiTeams[j] = iTemp;
			}
		}

		iNumPlayers = 0;
		for (int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			if (ic.getSlotStatus((PlayerTypes)i) == SS_TAKEN)
			{
				ic.setTeam((PlayerTypes)i, (TeamTypes)aiTeams[iNumPlayers]);
				++iNumPlayers;
			}
		}
	}

	if (isOption(GAMEOPTION_LOCK_MODS))
	{
		if (isGameMultiPlayer())
		{
			setOption(GAMEOPTION_LOCK_MODS, false);
		}
		else
		{
			static const int iPasswordSize = 8;
			char szRandomPassword[iPasswordSize];
			for (int i = 0; i < iPasswordSize-1; i++)
			{
				szRandomPassword[i] = getSorenRandNum(128, NULL);
			}
			szRandomPassword[iPasswordSize-1] = 0;

			ic.setAdminPassword(szRandomPassword);
		}
	}

	/*  advc.250c: So far, no points from Advanced Start have been assigned.
		I want the start turn to be a function of the start points.
		I'm assigning the start turn preliminarily here to avoid problems with
		start turn being undefined (not sure if this would be an issue),
		and overwrite the value later.
		To this end, I've moved the start turn and start year computation
		into a new function: */
	setStartTurnYear();

	for (iI = 0; iI < GC.getNumSpecialUnitInfos(); iI++)
	{
		if (GC.getSpecialUnitInfo((SpecialUnitTypes)iI).isValid())
		{
			makeSpecialUnitValid((SpecialUnitTypes)iI);
		}
	}

	for (iI = 0; iI < GC.getNumSpecialBuildingInfos(); iI++)
	{
		if (GC.getSpecialBuildingInfo((SpecialBuildingTypes)iI).isValid())
		{
			makeSpecialBuildingValid((SpecialBuildingTypes)iI);
		}
	}

	AI_init();

	doUpdateCacheOnTurn();
}

//
// Set initial items (units, techs, etc...)
//
void CvGame::setInitialItems()
{
	PROFILE_FUNC();

	// advc.003g: Want to set this as soon as CvGame knows the GameType
	b_mFPTestDone = !isNetworkMultiPlayer();

	int iAI = 0; // advc.250b: Just for disabling SPaH in game w/o any AI civs
	// K-Mod: Adjust the game handicap level to be the average of all the human player's handicap.
	// (Note: in the original bts rules, it would always set to Noble if the humans had different handicaps)
	//if (isGameMultiPlayer()) // advc.250b: Check moved down
	int iHumanPlayers = 0;
	int iTotal = 0;
	for (PlayerTypes i = (PlayerTypes)0; i < MAX_PLAYERS; i=(PlayerTypes)(i+1))
	{
		if (GET_PLAYER(i).isHuman())
		{
			iHumanPlayers++;
			iTotal += GC.getHandicapInfo(GET_PLAYER(i).getHandicapType()).
					getDifficulty(); // advc.250a
		}
		// <advc.250b>
		else if(GET_PLAYER(i).isAlive() && i != BARBARIAN_PLAYER &&
				!GET_PLAYER(i).isMinorCiv())
			iAI++; // </advc.250b>
	}
	if (isGameMultiPlayer()) {
		if (iHumanPlayers > 0) {
			/*  advc.250a: Relies on no strange new handicaps being placed
				between Settler and Deity. Same in CvTeam::getHandicapType. */
				setHandicapType((HandicapTypes)
				::round // dlph.22
				(iTotal / (10.0 * iHumanPlayers)));
		}
		else // advc.003: Moved K-Mod comment into AssertMsg.
			FAssertMsg(false, "All-AI game. Not necessarily wrong, but unexpected.");
	}
	// K-Mod end

	initFreeState();
	assignStartingPlots();
	normalizeStartingPlots();
	// <advc.030> Now that ice has been placed and normalization is through
	if(GC.getDefineINT("PASSABLE_AREAS") > 0)
		GC.getMap().recalculateAreas();
	// </advc.030>
	initFreeUnits();
	setAIHandicap(); // advc.127
	// <advc.250b>
	if(!isOption(GAMEOPTION_ADVANCED_START) || iAI == 0)
		setOption(GAMEOPTION_SPAH, false);
	if(isOption(GAMEOPTION_SPAH))
		// Reassigns start plots and start points
		m_pSpah->setInitialItems(); // </advc.250b>
	int iStartTurn = getStartTurn(); // advc.250c, advc.251
	// <advc.250c>
	if(getStartEra() == 0 && GC.getDefineINT("INCREASE_START_TURN") > 0) {
		std::vector<double> distr;
		for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
			CvPlayer const& civ = GET_PLAYER((PlayerTypes)i);
			if(civ.isAlive())
				distr.push_back(civ.getAdvancedStartPoints());
		}
		iStartTurn = getStartTurn();
		double maxMean = (::dMax(distr) + ::dMean(distr)) / 2.0;
		if(maxMean > 370) {
			iStartTurn += ::roundToMultiple(std::pow(std::max(0.0, maxMean - 325),
					0.58), 5);
		}
	} // </advc.250c>
	// <advc.251> Also set a later start turn if handicap grants lots of AI freebies
	if(!isOption(GAMEOPTION_ADVANCED_START) && getNumHumanPlayers() <
			countCivPlayersAlive()) {
		CvHandicapInfo& gameHandicap = GC.getHandicapInfo(getHandicapType());
		iStartTurn += ((gameHandicap.getAIStartingUnitMultiplier() * 10 +
				gameHandicap.getAIStartingWorkerUnits() * 10) *
				GC.getGameSpeedInfo(getGameSpeedType()).getGrowthPercent()) / 100;
	} // <advc.250c>
	if(getStartTurn() != iStartTurn && GC.getDefineINT("INCREASE_START_TURN") > 0) {
		setStartTurnYear(iStartTurn);
		/*  initDiplomacy is called from outside the DLL between the first
			setStartTurnYear call and setInitialItems. The second setStartTurnYear
			causes any initial "universal" peace treaties to end after 1 turn.
			Need to inform all CvDeal objects about the changed start turn: */
		CvDeal* d; int foo;
		for(d = firstDeal(&foo); d != NULL; d = nextDeal(&foo))
			d->setInitialGameTurn(getGameTurn());
	} // </advc.250c>
	// </advc.251>
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)i);
		if (kPlayer.isAlive())
			kPlayer.AI_updateFoundValues();
	}
}


void CvGame::regenerateMap()
{
	if (GC.getInitCore().getWBMapScript())
		return;

	int iI;
	CvMap& m = GC.getMap(); // advc.003
	/*  <advc.004j> Not sure if the unmodded game or any mod included with AdvCiv
		uses script data, but can't hurt to reset it. CvDLLButtonPopup::
		launchMainMenuPopup wants to disallow map regeneration once script data
		has been set. */
	setScriptData("");
	for(int i = 0; i < m.numPlots(); ++i) {
		CvPlot* p = m.plotByIndex(i);
		if(p != NULL) {
			p->setScriptData("");
			/*  advc.021b: Otherwise, assignStartingPlots runs into trouble upon
				map regeneration when a script calls allowDefaultImpl after
				assigning starting plots. */
			p->setStartingPlot(false);
		}
	} // </advc.004j>

	setFinalInitialized(false);
	setDawnOfManShown(false); // advc.004x

	for (iI = 0; iI < MAX_PLAYERS; iI++)
		GET_PLAYER((PlayerTypes)iI).killUnits();

	for (iI = 0; iI < MAX_PLAYERS; iI++)
		GET_PLAYER((PlayerTypes)iI).killCities();

	for (iI = 0; iI < MAX_PLAYERS; iI++)
		GET_PLAYER((PlayerTypes)iI).killAllDeals();

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		GET_PLAYER((PlayerTypes)iI).setFoundedFirstCity(false);
		GET_PLAYER((PlayerTypes)iI).setStartingPlot(NULL, false);
		// <advc.004x>
		if(GET_PLAYER((PlayerTypes)iI).isHuman())
			GET_PLAYER((PlayerTypes)iI).killAll(BUTTONPOPUP_CHOOSETECH);
		// </advc.004x>
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
		m.setRevealedPlots(((TeamTypes)iI), false);

	gDLL->getEngineIFace()->clearSigns();

	m.erasePlots();

	CvMapGenerator::GetInstance().generateRandomMap();
	CvMapGenerator::GetInstance().addGameElements();

	gDLL->getEngineIFace()->RebuildAllPlots();
	// <advc.251>
	setGameTurn(0);
	setStartTurn(0);
	setStartTurnYear();
	m_iElapsedGameTurns = 0;
	// </advc.251>
	setTurnSlice(0); // advc.001: Reset minutesPlayed to 0
	CvEventReporter::getInstance().resetStatistics();

	setInitialItems();

	initScoreCalculation();
	setFinalInitialized(true);

	m.setupGraphical();
	gDLL->getEngineIFace()->SetDirty(GlobeTexture_DIRTY_BIT, true);
	gDLL->getEngineIFace()->SetDirty(MinimapTexture_DIRTY_BIT, true);
	gDLL->getInterfaceIFace()->setDirty(ColoredPlots_DIRTY_BIT, true);

	cycleSelectionGroups_delayed(1, false);
	// <advc.004j>
	bool bShowDawn = (GC.getDefineINT("SHOW_DAWN_AFTER_REGEN") > 0 &&
			// Somehow doesn't work with Adv. Start; Dawn screen doesn't appear.
			(!isOption(GAMEOPTION_ADVANCED_START) || isOption(GAMEOPTION_SPAH)));
	// </advc.004j>
	// <advc.700>
	if(isOption(GAMEOPTION_RISE_FALL)) {
		m_pRiseFall->reset();
		m_pRiseFall->init();
		bShowDawn = false;
	}
	else { // </advc.700>
		autoSave(true); // advc.106l
	} // <advc.004j>
	if(bShowDawn)
		showDawnOfMan(); // </advc.004j>
	if (NO_PLAYER != getActivePlayer())
	{
		CvPlot* pPlot = GET_PLAYER(getActivePlayer()).getStartingPlot();
		if (NULL != pPlot)
		{
			//gDLL->getInterfaceIFace()->lookAt(pPlot->getPoint(), CAMERALOOKAT_NORMAL);
			/*  <advc.004j> ^Comment by EmperorFool (from BULL):
				"This doesn't work until after the game has had time to update.
				 Centering on the starting location is now done by MapFinder using
				 BugUtil.delayCall()."
				I'm going to use setUpdateTimer instead. Unless the Dawn screen
				is shown b/c that focuses the camera anyway. */
			if(!bShowDawn)
				setUpdateTimer(UPDATE_LOOK_AT_STARTING_PLOT, 5); // </advc.004j>
		}
	}
}

// <advc.004j>
void CvGame::showDawnOfMan() {

	if(getActivePlayer() == NO_PLAYER)
		return;
	// Based on CvAllErasDawnOfManScreenEventManager.py
	CvPopupInfo* dom = new CvPopupInfo();
	dom->setButtonPopupType(BUTTONPOPUP_PYTHON_SCREEN);
	dom->setText(L"showDawnOfMan");
	GET_PLAYER(getActivePlayer()).addPopup(dom);
	setDawnOfManShown(true); // advc.004x
} // </advc.004j>


void CvGame::uninit()
{
	SAFE_DELETE_ARRAY(m_aiShrineBuilding);
	SAFE_DELETE_ARRAY(m_aiShrineReligion);
	SAFE_DELETE_ARRAY(m_paiUnitCreatedCount);
	SAFE_DELETE_ARRAY(m_paiUnitClassCreatedCount);
	SAFE_DELETE_ARRAY(m_paiBuildingClassCreatedCount);
	SAFE_DELETE_ARRAY(m_paiProjectCreatedCount);
	SAFE_DELETE_ARRAY(m_paiForceCivicCount);
	SAFE_DELETE_ARRAY(m_paiVoteOutcome);
	SAFE_DELETE_ARRAY(m_paiReligionGameTurnFounded);
	SAFE_DELETE_ARRAY(m_paiCorporationGameTurnFounded);
	SAFE_DELETE_ARRAY(m_aiSecretaryGeneralTimer);
	SAFE_DELETE_ARRAY(m_aiVoteTimer);
	SAFE_DELETE_ARRAY(m_aiDiploVote);

	SAFE_DELETE_ARRAY(m_pabSpecialUnitValid);
	SAFE_DELETE_ARRAY(m_pabSpecialBuildingValid);
	SAFE_DELETE_ARRAY(m_abReligionSlotTaken);

	SAFE_DELETE_ARRAY(m_paHolyCity);
	SAFE_DELETE_ARRAY(m_paHeadquarters);

	m_aszDestroyedCities.clear();
	m_aszGreatPeopleBorn.clear();

	m_deals.uninit();
	// <advc.072> This also frees dynamically allocated memory
	m_currentDeals.clear();
	m_currentDealsWidget.clear();
	// </advc.072>
	m_voteSelections.uninit();
	m_votesTriggered.uninit();

	m_mapRand.uninit();
	m_sorenRand.uninit();

	clearReplayMessageMap();
	SAFE_DELETE(m_pReplayInfo);

	m_aPlotExtraYields.clear();
	m_aPlotExtraCosts.clear();
	m_mapVoteSourceReligions.clear();
	m_aeInactiveTriggers.clear();
	applyOptionEffects(true); // advc.310
	/*  advc.700: Need to call this explicitly due to the unusual way that
		RiseFall is initialized (from updateBlockadedPlots) */
	m_pRiseFall->reset();
}

// advc.250c: Function body cut from CvGame::init. Changes marked in-line.
void CvGame::setStartTurnYear(int iTurn) {

	int iI;
	CvGameSpeedInfo const& kSpeed = GC.getGameSpeedInfo(getGameSpeedType()); // advc.003
	// <advc.250c>
	if(iTurn > 0)
		setGameTurn(iTurn);
	else // </advc.250c>
		if (getGameTurn() == 0)
	{
		int iStartTurn = 0;
		for (iI = 0; iI < kSpeed.getNumTurnIncrements(); iI++)
			iStartTurn += kSpeed.getGameTurnInfo(iI).iNumGameTurnsPerIncrement;
		iStartTurn *= GC.getEraInfo(getStartEra()).getStartPercent();
		iStartTurn /= 100;
		setGameTurn(iStartTurn);
	}

	setStartTurn(getGameTurn());

	if (getMaxTurns() == 0 /* advc.250c: */ || iTurn > 0)
	{
		int iEstimateEndTurn = 0;
		for (iI = 0; iI < kSpeed.getNumTurnIncrements(); iI++)
			iEstimateEndTurn += kSpeed.getGameTurnInfo(iI).iNumGameTurnsPerIncrement;
		setEstimateEndTurn(iEstimateEndTurn);

		if (getEstimateEndTurn() > getGameTurn())
		{
			bool bValid = false;
			for (iI = 0; iI < GC.getNumVictoryInfos(); iI++)
			{
				if (isVictoryValid((VictoryTypes)iI))
				{
					if (GC.getVictoryInfo((VictoryTypes)iI).isEndScore())
					{
						bValid = true;
						break;
					}
				}
			}

			if (bValid)
				setMaxTurns(getEstimateEndTurn() - getGameTurn());
		}
	}
	else setEstimateEndTurn(getGameTurn() + getMaxTurns());

	setStartYear(GC.getDefineINT("START_YEAR"));
}

// Initialize data members that are serialized.
void CvGame::reset(HandicapTypes eHandicap, bool bConstructorCall)
{
	int iI;

	//--------------------------------
	// Uninit class
	uninit();
	m_bAllGameDataRead = false; // advc.003;
	// <advc.106i>
	if(m_pHallOfFame != NULL)
		m_pHallOfFame->uninit(); // Don't keep HoF in memory indefinitely
	// </advc.106i>
	m_iElapsedGameTurns = 0;
	m_iStartTurn = 0;
	m_iStartYear = 0;
	m_iEstimateEndTurn = 0;
	m_iTurnSlice = 0;
	m_iCutoffSlice = 0;
	m_iNumGameTurnActive = 0;
	m_iNumCities = 0;
	m_iTotalPopulation = 0;
	m_iTradeRoutes = 0;
	m_iFreeTradeCount = 0;
	m_iNoNukesCount = 0;
	m_iNukesExploded = 0;
	m_iMaxPopulation = 0;
	m_iMaxLand = 0;
	m_iMaxTech = 0;
	m_iMaxWonders = 0;
	m_iInitPopulation = 0;
	m_iInitLand = 0;
	m_iInitTech = 0;
	m_iInitWonders = 0;
	m_iAIAutoPlay = 0;
	m_iGlobalWarmingIndex = 0;// K-Mod
	m_iGwEventTally = -1; // K-Mod (-1 means Gw tally has not been activated yet)
	// <advc.003b>
	m_iCivPlayersEverAlive = 0;
	m_iCivTeamsEverAlive = 0;
	// </advc.003b>
	m_uiInitialTime = 0;

	m_bScoreDirty = false;
	m_bCircumnavigated = false;
	m_bDebugMode = false;
	m_bDebugModeCache = false;
	m_bFinalInitialized = false;
	m_bPbemTurnSent = false;
	m_bHotPbemBetweenTurns = false;
	m_bPlayerOptionsSent = false;
	m_bNukesValid = false;
	m_bShowingCurrentDeals = false; // advc.072  (not serialized)
	m_iScreenWidth = m_iScreenHeight = 0; // advc.061

	m_eHandicap = eHandicap;
	// advc.127: (XML not loaded when constructor called)
	m_eAIHandicap = bConstructorCall ? NO_HANDICAP : (HandicapTypes)GC.getDefineINT("STANDARD_HANDICAP");
	m_ePausePlayer = NO_PLAYER;
	m_eBestLandUnit = NO_UNIT;
	m_eWinner = NO_TEAM;
	m_eVictory = NO_VICTORY;
	m_eGameState = GAMESTATE_ON;
	m_eInitialActivePlayer = NO_PLAYER; // advc.106h
	m_szScriptData = "";

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_aiRankPlayer[iI] = 0;
		m_aiPlayerRank[iI] = 0;
		m_aiPlayerScore[iI] = 0;
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		m_aiRankTeam[iI] = 0;
		m_aiTeamRank[iI] = 0;
		m_aiTeamScore[iI] = 0;
	}

	if (!bConstructorCall)
	{
		FAssertMsg(m_paiUnitCreatedCount==NULL, "about to leak memory, CvGame::m_paiUnitCreatedCount");
		m_paiUnitCreatedCount = new int[GC.getNumUnitInfos()];
		for (iI = 0; iI < GC.getNumUnitInfos(); iI++)
		{
			m_paiUnitCreatedCount[iI] = 0;
		}

		FAssertMsg(m_paiUnitClassCreatedCount==NULL, "about to leak memory, CvGame::m_paiUnitClassCreatedCount");
		m_paiUnitClassCreatedCount = new int[GC.getNumUnitClassInfos()];
		for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
		{
			m_paiUnitClassCreatedCount[iI] = 0;
		}

		FAssertMsg(m_paiBuildingClassCreatedCount==NULL, "about to leak memory, CvGame::m_paiBuildingClassCreatedCount");
		m_paiBuildingClassCreatedCount = new int[GC.getNumBuildingClassInfos()];
		for (iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
		{
			m_paiBuildingClassCreatedCount[iI] = 0;
		}

		FAssertMsg(m_paiProjectCreatedCount==NULL, "about to leak memory, CvGame::m_paiProjectCreatedCount");
		m_paiProjectCreatedCount = new int[GC.getNumProjectInfos()];
		for (iI = 0; iI < GC.getNumProjectInfos(); iI++)
		{
			m_paiProjectCreatedCount[iI] = 0;
		}

		FAssertMsg(m_paiForceCivicCount==NULL, "about to leak memory, CvGame::m_paiForceCivicCount");
		m_paiForceCivicCount = new int[GC.getNumCivicInfos()];
		for (iI = 0; iI < GC.getNumCivicInfos(); iI++)
		{
			m_paiForceCivicCount[iI] = 0;
		}

		FAssertMsg(0 < GC.getNumVoteInfos(), "GC.getNumVoteInfos() is not greater than zero in CvGame::reset");
		FAssertMsg(m_paiVoteOutcome==NULL, "about to leak memory, CvGame::m_paiVoteOutcome");
		m_paiVoteOutcome = new PlayerVoteTypes[GC.getNumVoteInfos()];
		for (iI = 0; iI < GC.getNumVoteInfos(); iI++)
		{
			m_paiVoteOutcome[iI] = NO_PLAYER_VOTE;
		}

		FAssertMsg(0 < GC.getNumVoteSourceInfos(), "GC.getNumVoteSourceInfos() is not greater than zero in CvGame::reset");
		FAssertMsg(m_aiDiploVote==NULL, "about to leak memory, CvGame::m_aiDiploVote");
		m_aiDiploVote = new int[GC.getNumVoteSourceInfos()];
		for (iI = 0; iI < GC.getNumVoteSourceInfos(); iI++)
		{
			m_aiDiploVote[iI] = 0;
		}

		FAssertMsg(m_pabSpecialUnitValid==NULL, "about to leak memory, CvGame::m_pabSpecialUnitValid");
		m_pabSpecialUnitValid = new bool[GC.getNumSpecialUnitInfos()];
		for (iI = 0; iI < GC.getNumSpecialUnitInfos(); iI++)
		{
			m_pabSpecialUnitValid[iI] = false;
		}

		FAssertMsg(m_pabSpecialBuildingValid==NULL, "about to leak memory, CvGame::m_pabSpecialBuildingValid");
		m_pabSpecialBuildingValid = new bool[GC.getNumSpecialBuildingInfos()];
		for (iI = 0; iI < GC.getNumSpecialBuildingInfos(); iI++)
		{
			m_pabSpecialBuildingValid[iI] = false;
		}

		FAssertMsg(m_paiReligionGameTurnFounded==NULL, "about to leak memory, CvGame::m_paiReligionGameTurnFounded");
		m_paiReligionGameTurnFounded = new int[GC.getNumReligionInfos()];
		FAssertMsg(m_abReligionSlotTaken==NULL, "about to leak memory, CvGame::m_abReligionSlotTaken");
		m_abReligionSlotTaken = new bool[GC.getNumReligionInfos()];
		FAssertMsg(m_paHolyCity==NULL, "about to leak memory, CvGame::m_paHolyCity");
		m_paHolyCity = new IDInfo[GC.getNumReligionInfos()];
		for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
		{
			m_paiReligionGameTurnFounded[iI] = -1;
			m_paHolyCity[iI].reset();
			m_abReligionSlotTaken[iI] = false;
		}

		FAssertMsg(m_paiCorporationGameTurnFounded==NULL, "about to leak memory, CvGame::m_paiCorporationGameTurnFounded");
		m_paiCorporationGameTurnFounded = new int[GC.getNumCorporationInfos()];
		m_paHeadquarters = new IDInfo[GC.getNumCorporationInfos()];
		for (iI = 0; iI < GC.getNumCorporationInfos(); iI++)
		{
			m_paiCorporationGameTurnFounded[iI] = -1;
			m_paHeadquarters[iI].reset();
		}

		FAssertMsg(m_aiShrineBuilding==NULL, "about to leak memory, CvGame::m_aiShrineBuilding");
		FAssertMsg(m_aiShrineReligion==NULL, "about to leak memory, CvGame::m_aiShrineReligion");
		m_aiShrineBuilding = new int[GC.getNumBuildingInfos()];
		m_aiShrineReligion = new int[GC.getNumBuildingInfos()];
		for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
		{
			m_aiShrineBuilding[iI] = (int) NO_BUILDING;
			m_aiShrineReligion[iI] = (int) NO_RELIGION;
		}

		FAssertMsg(m_aiSecretaryGeneralTimer==NULL, "about to leak memory, CvGame::m_aiSecretaryGeneralTimer");
		FAssertMsg(m_aiVoteTimer==NULL, "about to leak memory, CvGame::m_aiVoteTimer");
		m_aiSecretaryGeneralTimer = new int[GC.getNumVoteSourceInfos()];
		m_aiVoteTimer = new int[GC.getNumVoteSourceInfos()];
		for (iI = 0; iI < GC.getNumVoteSourceInfos(); iI++)
		{
			m_aiSecretaryGeneralTimer[iI] = 0;
			m_aiVoteTimer[iI] = 0;
		}
	}

	m_deals.removeAll();
	m_voteSelections.removeAll();
	m_votesTriggered.removeAll();

	m_mapRand.reset();
	m_sorenRand.reset();

	m_iNumSessions = 1;

	m_iShrineBuildingCount = 0;
	m_iNumCultureVictoryCities = 0;
	m_eCultureVictoryCultureLevel = NO_CULTURELEVEL;
	m_bScenario = false; // advc.052
	if (!bConstructorCall)
	{
		AI_reset();
	}
	m_ActivePlayerCycledGroups.clear(); // K-Mod
	m_bInBetweenTurns = false; // advc.106b
	m_iTurnLoadedFromSave = -1; // advc.044
	// <advc.004m>
	m_eCurrentLayer = GLOBE_LAYER_UNKNOWN;
	m_bLayerFromSavegame = false; // </advc.004m>
	m_bFeignSP = false; // advc.135c
	m_bDoMShown = false; // advc.004x
	b_mFPTestDone = false; // advc.003g
	// <advc.003r>
	for(int i = 0; i < NUM_UPDATE_TIMER_TYPES; i++)
		m_aiUpdateTimers[i] = -1; // </advc.003r>
}


void CvGame::initDiplomacy()
{
	PROFILE_FUNC();

	for(int i = 0; i < MAX_TEAMS; i++) {  // advc.003: style changes
		CvTeam& t = GET_TEAM((TeamTypes)i);
		t.meet(t.getID(), false);
		if(i == BARBARIAN_TEAM || t.isMinorCiv()) {
			for(int j = 0; j < MAX_CIV_TEAMS; j++) {
				CvTeam& kTarget = GET_TEAM((TeamTypes)j);
				if(kTarget.isAlive() && i != j) // advc.003m: Alive check added
					t.declareWar(kTarget.getID(), false, NO_WARPLAN);
			}
		}
	}

	// Forced peace at the beginning of Advanced starts
	if (isOption(GAMEOPTION_ADVANCED_START)
			/*  advc.250b: No need to protect the AI from the player when human
				start is advanced, and AI won't start wars within 10 turns anyway. */
			&& !isOption(GAMEOPTION_SPAH))
	{
		CLinkList<TradeData> player1List;
		CLinkList<TradeData> player2List;
		TradeData kTradeData;
		setTradeItem(&kTradeData, TRADE_PEACE_TREATY);
		player1List.insertAtEnd(kTradeData);
		player2List.insertAtEnd(kTradeData);

		for (int iPlayer1 = 0; iPlayer1 < MAX_CIV_PLAYERS; ++iPlayer1)
		{
			CvPlayer& kLoopPlayer1 = GET_PLAYER((PlayerTypes)iPlayer1);

			if (kLoopPlayer1.isAlive())
			{
				for (int iPlayer2 = iPlayer1 + 1; iPlayer2 < MAX_CIV_PLAYERS; ++iPlayer2)
				{
					CvPlayer& kLoopPlayer2 = GET_PLAYER((PlayerTypes)iPlayer2);

					if (kLoopPlayer2.isAlive())
					{
						if (GET_TEAM(kLoopPlayer1.getTeam()).canChangeWarPeace(kLoopPlayer2.getTeam()))
						{
							implementDeal((PlayerTypes)iPlayer1, (PlayerTypes)iPlayer2, &player1List, &player2List);
						}
					}
				}
			}
		}
	}
}

// <advc.127>
void CvGame::setAIHandicap() {

	// Set m_eAIHandicap to the average of AI handicaps
	int iHandicapSum = 0;
	int iDiv = 0;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayer& civ = GET_PLAYER((PlayerTypes)i);
		if(civ.isAlive() && !civ.isHuman() && !civ.isMinorCiv()) {
			iHandicapSum += civ.getHandicapType();
			iDiv++;
		}
	}
	if(iDiv > 0) // Leaves it at STANDARD_HANDICAP in all-human games
		m_eAIHandicap = (HandicapTypes)::round(iHandicapSum / (double)iDiv);
} // </advc.127>


void CvGame::initFreeState()
{
	if(GC.getInitCore().isScenario()) {
		setScenario(true); // advc.052
		AI().AI_initScenario(); // advc.104u
	}
	else { // advc.051: (Moved up.) Don't force 0 gold in scenarios.
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
				GET_PLAYER((PlayerTypes)iI).initFreeState();
		}
	}
	applyOptionEffects(); // advc.310
	for (int iI = 0; iI < GC.getNumTechInfos(); iI++)
	{
		for (int iJ = 0; iJ < MAX_TEAMS; iJ++)
		{
			if(!GET_TEAM((TeamTypes)iJ).isAlive())
				continue; // advc.003
			bool bValid = false;
			if (//(GC.getHandicapInfo(getHandicapType()).isFreeTechs(iI)) || // disabled by K-Mod. (moved & changed. See below)
					(!GET_TEAM((TeamTypes)iJ).isHuman() && GC.getHandicapInfo(getHandicapType()).isAIFreeTechs(iI)
					// advc.001: Barbarians receiving free AI tech might be a bug
					&& iJ != BARBARIAN_TEAM
					&& !isOption(GAMEOPTION_ADVANCED_START)) || // advc.250c
					GC.getTechInfo((TechTypes)iI).getEra() < getStartEra())
				bValid = true;
			if (!bValid) {
				for (int iK = 0; iK < MAX_PLAYERS; iK++)
				{
					CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iK); // K-Mod
					if(!kLoopPlayer.isAlive() || kLoopPlayer.getTeam() != iJ)
						continue; // advc.003
					/*  <advc.250b> <advc.250c> Always grant civ-specific tech,
						but not tech from handicap if Advanced Start except to
						human civs that don't actually start Advanced (SPaH option). */
					if (GC.getCivilizationInfo(kLoopPlayer.getCivilizationType()).isCivilizationFreeTechs(iI))
					{
						bValid = true;
						break;
					}
					if (!bValid &&
							// K-Mod (give techs based on player handicap, not game handicap.)
							GC.getHandicapInfo(kLoopPlayer.getHandicapType()).isFreeTechs(iI)
							&& (!isOption(GAMEOPTION_ADVANCED_START) ||
							(isOption(GAMEOPTION_SPAH) &&
							GET_TEAM((TeamTypes)iJ).isHuman())))
							// </advc.250b> </advc.250c>
					{
						bValid = true;
						break;
					}
				}
			}
			// <advc.126> Later-era free tech only for later-era starts.
			if(bValid && GC.getTechInfo((TechTypes)iI).getEra() > getStartEra())
				bValid = false; // </advc.126>
			if(bValid) // advc.051: Don't take away techs granted by the scenario
				GET_TEAM((TeamTypes)iJ).setHasTech((TechTypes)iI, true, NO_PLAYER, false, false);
			if (bValid && GC.getTechInfo((TechTypes)iI).isMapVisible())
				GC.getMap().setRevealedPlots((TeamTypes)iJ, true, true);
		}
	} // <advc.051>
	if(isScenario() && getStartEra() <= 0) { // Set start era based on player era
		//int iMinEra = NO_ERA;
		int iEraSum = 0; // Better use the mean
		int iMajorCivs = 0;
		for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
			CvPlayer const& civ = GET_PLAYER((PlayerTypes)i);
			if(civ.isAlive() && !civ.isMinorCiv()) {
				int iEra = civ.getCurrentEra();
				/*if(iMinEra == NO_ERA)
					iMinEra = iEra;
				else iMinEra = std::min(iMinEra, iEra);*/
				iEraSum += iEra;
				iMajorCivs++;
			}
		}
		int iStartEra = iEraSum / std::max(iMajorCivs, 1);//=iMinEra
		if(iStartEra > getStartEra())
			GC.getInitCore().setEra((EraTypes)iStartEra);
	}
	m_eInitialActivePlayer = getActivePlayer(); // advc.106h
}


void CvGame::initScenario() {

	setAIHandicap(); // advc.127
	initFreeState(); // Tech from handicap
	// <advc.030>
	if(GC.getDefineINT("PASSABLE_AREAS") > 0) {
		/*  recalculateAreas can't handle preplaced cities. Or perhaps it can
			(Barbarian cities are fine in most cases), but there's going to
			be other stuff, like free units, that causes problems. */
		for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
			if(GET_PLAYER((PlayerTypes)i).getNumCities() > 0)
				return;
		}
		GC.getMap().recalculateAreas();
	} // </advc.030>
}

void CvGame::initFreeUnits() {

	bool bScenario = GC.getInitCore().isScenario();
	/*  In scenarios, neither setInitialItems nor initFreeState is called; the
		EXE only calls initFreeUnits, so the initialization of freebies needs to
		happen here. */
	if(bScenario)
		initScenario();
	initFreeUnits_bulk(); // (also sets Advanced Start points)
	if(!bScenario)
		return;
	/*  <advc.250b> Advanced Start is always visible on the Custom Scenario screen,
		but doesn't work properly unless Advanced Start is the scenario's
		default setting. Verify that start points have been assigned, or else
		disable Advanced Start. */
	bool bValid = false;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayer const& civ = GET_PLAYER((PlayerTypes)i);
		if(civ.isAlive() && civ.getAdvancedStartPoints() > 0) {
			bValid = true;
			break;
		}
	}
	if(!bValid) {
		setOption(GAMEOPTION_SPAH, false);
		setOption(GAMEOPTION_ADVANCED_START, false);
	} // </advc.250b>
}

void CvGame::initFreeUnits_bulk() { // </advc.051>

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if ((GET_PLAYER((PlayerTypes)iI).getNumUnits() == 0) && (GET_PLAYER((PlayerTypes)iI).getNumCities() == 0))
			{
				GET_PLAYER((PlayerTypes)iI).initFreeUnits();
			}
		}
	}
}

/*  <advc.310> For building (or other) effects that only apply when certain
	game options are set. */
void CvGame::applyOptionEffects(bool bEnableAll) {

	CvBuildingInfo::setDomesticGreatGeneralRateModifierEnabled(bEnableAll ||
			isOption(GAMEOPTION_RAGING_BARBARIANS) || isOption(GAMEOPTION_NO_BARBARIANS));
	CvBuildingInfo::setAreaTradeRoutesEnabled(bEnableAll ||
			!isOption(GAMEOPTION_RAGING_BARBARIANS) || isOption(GAMEOPTION_NO_BARBARIANS));
	CvBuildingInfo::setAreaBorderObstacleEnabled(bEnableAll ||
			!isOption(GAMEOPTION_NO_BARBARIANS));
} // </advc.310>


void CvGame::assignStartingPlots()
{
	PROFILE_FUNC();

	// (original bts code deleted) // advc.003
	// K-Mod. Same functionality, but much faster and easier to read.
	//
	// First, make a list of all the pre-marked starting plots on the map.
	std::vector<CvPlot*> starting_plots;
	for (int i = 0; i < GC.getMap().numPlots(); i++)
	{
		gDLL->callUpdater();	// allow window updates during launch

		CvPlot* pLoopPlot = GC.getMap().plotByIndex(i);
		if (pLoopPlot->isStartingPlot())
			starting_plots.push_back(pLoopPlot);
	}
	// Now, randomly assign a starting plot to each player.
	for (PlayerTypes i = (PlayerTypes)0; starting_plots.size() > 0 && i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
	{
		CvPlayer& kLoopPlayer = GET_PLAYER(i);
		if (kLoopPlayer.isAlive() && kLoopPlayer.getStartingPlot() == NULL)
		{
			int iRandOffset = getSorenRandNum(starting_plots.size(), "Starting Plot");
			kLoopPlayer.setStartingPlot(starting_plots[iRandOffset], true);
			// remove this plot from the list.
			starting_plots[iRandOffset] = starting_plots[starting_plots.size()-1];
			starting_plots.pop_back();
		}
	}
	// K-Mod end

	if (gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "assignStartingPlots"))
	{
		if (!gDLL->getPythonIFace()->pythonUsingDefaultImpl())
		{
			// Python override
			return;
		}
	}
	std::vector<PlayerTypes> playerOrder; // advc.003: was <int>
	std::vector<bool> newPlotFound(MAX_CIV_PLAYERS, false); // advc.108b
	if (isTeamGame())
	{	/*  advc (comment): This assignment is just a starting point for
			normalizeStartingPlotLocations */
		for (int iPass = 0; iPass < 2 * MAX_PLAYERS; ++iPass)
		{
			bool bStartFound = false;
			int iRandOffset = getSorenRandNum(countCivTeamsAlive(), "Team Starting Plot");

			for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
			{
				int iLoopTeam = ((iI + iRandOffset) % MAX_CIV_TEAMS);
				if (!GET_TEAM((TeamTypes)iLoopTeam).isAlive())
					continue;

				for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
				{
					CvPlayer& kMember = GET_PLAYER((PlayerTypes)iJ);
					if(!kMember.isAlive())
						continue; // advc.003
					if (kMember.getTeam() == iLoopTeam
							// <advc.108b>
							&& !newPlotFound[iJ]) {
						if(kMember.getStartingPlot() == NULL)
							kMember.setStartingPlot(kMember.findStartingPlot(), true);
						if(kMember.getStartingPlot() != NULL) {
							playerOrder.push_back(kMember.getID());
							bStartFound = true;
							newPlotFound[kMember.getID()] = true;
							break;
						}
					} // </advc.108b>
				}
			}

			if (!bStartFound)
				break;
		}

		//check all players have starting plots
		for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
		{
			FAssertMsg(!GET_PLAYER((PlayerTypes)iJ).isAlive() ||
					(GET_PLAYER((PlayerTypes)iJ).getStartingPlot() != NULL
					&& newPlotFound[iJ]), // advc.108b
					"Player has no starting plot");
		}
	} /* advc.108b: Replace all this. Don't want handicaps to be ignored in
		 multiplayer, and the BtS random assignment of human starts doesn't
		 actually work - favors player 0 when humans are in slots 0, 1 ... */
	/*else if (isGameMultiPlayer()) {
		int iRandOffset = getSorenRandNum(countCivPlayersAlive(), "Player Starting Plot");
		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++) {
			int iLoopPlayer = ((iI + iRandOffset) % MAX_CIV_PLAYERS);
			if (GET_PLAYER((PlayerTypes)iLoopPlayer).isAlive()) {
				if (GET_PLAYER((PlayerTypes)iLoopPlayer).isHuman()) {
					if (GET_PLAYER((PlayerTypes)iLoopPlayer).getStartingPlot() == NULL) {
						GET_PLAYER((PlayerTypes)iLoopPlayer).setStartingPlot(GET_PLAYER((PlayerTypes)iLoopPlayer).findStartingPlot(), true);
						playerOrder.push_back(iLoopPlayer);
					}
				}
			}
		}
		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++) {
			if (GET_PLAYER((PlayerTypes)iI).isAlive()) {
				if (!(GET_PLAYER((PlayerTypes)iI).isHuman())) {
					if (GET_PLAYER((PlayerTypes)iI).getStartingPlot() == NULL) {
						GET_PLAYER((PlayerTypes)iI).setStartingPlot(GET_PLAYER((PlayerTypes)iI).findStartingPlot(), true);
						playerOrder.push_back(iI);
					}
				}
			}
		}
	}
	else
	{	// advc.003 (Comment): The minus 1 prevents humans from getting the worst plot
		int const upperBound = countCivPlayersAlive() - 1;
		int iHumanSlot = range(((upperBound * GC.getHandicapInfo(getHandicapType()).
				getStartingLocationPercent()) / 100), 0, upperBound);
		for (int iI = 0; iI < iHumanSlot; iI++) {
			if (GET_PLAYER((PlayerTypes)iI).isAlive()) {
				if (!(GET_PLAYER((PlayerTypes)iI).isHuman())) {
					if (GET_PLAYER((PlayerTypes)iI).getStartingPlot() == NULL) {
						GET_PLAYER((PlayerTypes)iI).setStartingPlot(GET_PLAYER((PlayerTypes)iI).findStartingPlot(), true);
						playerOrder.push_back(iI);
					}
				}
			}
		}
		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++) {
			if (GET_PLAYER((PlayerTypes)iI).isAlive()) {
				if (GET_PLAYER((PlayerTypes)iI).isHuman()) {
					if (GET_PLAYER((PlayerTypes)iI).getStartingPlot() == NULL) {
						GET_PLAYER((PlayerTypes)iI).setStartingPlot(GET_PLAYER((PlayerTypes)iI).findStartingPlot(), true);
						playerOrder.push_back(iI);
					}
				}
			}
		}
		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++) {
			if (GET_PLAYER((PlayerTypes)iI).isAlive()) {
				if (GET_PLAYER((PlayerTypes)iI).getStartingPlot() == NULL) {
					GET_PLAYER((PlayerTypes)iI).setStartingPlot(GET_PLAYER((PlayerTypes)iI).findStartingPlot(), true);
					playerOrder.push_back(iI);
				}
			}
		}
	}
	//Now iterate over the player starts in the original order and re-place them.
	//std::vector<int>::iterator playerOrderIter;
	for (playerOrderIter = playerOrder.begin(); playerOrderIter != playerOrder.end(); ++playerOrderIter)
		GET_PLAYER((PlayerTypes)(*playerOrderIter)).setStartingPlot(GET_PLAYER((PlayerTypes)(*playerOrderIter)).findStartingPlot(), true);*/
	// <advc.108b>
	else {
		int const iAlive = countCivPlayersAlive();
		for(int i = 0; i < iAlive; i++)
			playerOrder.push_back(NO_PLAYER);
		for(int iPass = 0; iPass < 2; iPass++) {
			bool bHuman = (iPass == 0);
			int iCivs = countHumanPlayersAlive();
			if(!bHuman)
				iCivs = iAlive - iCivs;
			int iRandOffset = getSorenRandNum(iCivs, "advc.108b");
			int iSkipped = 0;
			for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
				CvPlayer& civ = GET_PLAYER((PlayerTypes)i);
				if(civ.isAlive() && civ.isHuman() == bHuman) {
					if(iSkipped < iRandOffset) {
						iSkipped++;
						continue;
					}
					/*  This sets iRandOffset to the id of a random human civ
						in the first pass, and a random AI civ in the second. */
					iRandOffset = i;
					break;
				}
			}
			for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
				CvPlayer& civ = GET_PLAYER((PlayerTypes)((i + iRandOffset) %
						MAX_CIV_PLAYERS));
				if(!civ.isAlive() || civ.isHuman() != bHuman)
					continue;
				FAssert(!newPlotFound[civ.getID()]);
				// If the map script hasn't set a plot, find one.
				if(civ.getStartingPlot() == NULL)
					civ.setStartingPlot(civ.findStartingPlot(), true);
				if(civ.getStartingPlot() == NULL) {
					FAssertMsg(false, "No starting plot found");
					continue;
				}
				int iPos = ::range((iAlive *
						GC.getHandicapInfo(civ.getHandicapType()).
						getStartingLocationPercent()) / 100, 0, iAlive - 1);
				if(playerOrder[iPos] != NO_PLAYER) { // Pos already taken
					for(int j = 1; j < std::max(iPos + 1, iAlive - iPos); j++) {
						// Alternate between better and worse positions
						if(iPos + j < iAlive && playerOrder[iPos + j] == NO_PLAYER) {
							iPos += j;
							break;
						}
						if(iPos - j >= 0 && playerOrder[iPos - j] == NO_PLAYER) {
							iPos -= j;
							break;
						}
					}
					FAssert(playerOrder[iPos] == NO_PLAYER);
				}
				playerOrder[iPos] = civ.getID();
				newPlotFound[civ.getID()] = true;
			}
		}
	}
	std::vector<std::pair<int,CvPlot*> > startPlots;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayer& civ = GET_PLAYER((PlayerTypes)i);
		if(!civ.isAlive())
			continue;
		CvPlot* p = civ.getStartingPlot();
		if(p == NULL) {
			FAssertMsg(p != NULL, "Player has no starting plot");
			civ.setStartingPlot(civ.findStartingPlot(), true);
		}
		if(p == NULL)
			continue;
		/*  p->getFoundValue(civ.getID()) would be faster, but
			CvPlot::setFoundValue may not have been called
			(and then it returns 0) */
		int val = civ.AI_foundValue(p->getX(), p->getY(), -1, true);
		FAssertMsg(val > 0, "Bad starting position");
		// minus val for descending order
		startPlots.push_back(std::make_pair(-val, p));
	}
	FAssert(startPlots.size() == playerOrder.size());
	std::sort(startPlots.begin(), startPlots.end());
	for(size_t i = 0; i < playerOrder.size(); i++) {
		if(playerOrder[i] == NO_PLAYER) {
			FAssert(playerOrder[i] != NO_PLAYER);
			continue;
		}
		GET_PLAYER(playerOrder[i]).setStartingPlot(
				startPlots[i].second, true);
	} // </advc.108b>
}

// Swaps starting locations until we have reached the optimal closeness between teams
// (caveat: this isn't quite "optimal" because we could get stuck in local minima, but it's pretty good)
void CvGame::normalizeStartingPlotLocations()
{	// <advc.003b> This function is only for team games
	if(!isTeamGame())
		return; // </advc.003b>
	CvPlot* apNewStartPlots[MAX_CIV_PLAYERS];
	int* aaiDistances[MAX_CIV_PLAYERS];
	int aiStartingLocs[MAX_CIV_PLAYERS];
	int iI, iJ;

	// Precalculate distances between all starting positions:
	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			gDLL->callUpdater();	// allow window to update during launch
			aaiDistances[iI] = new int[iI];
			for (iJ = 0; iJ < iI; iJ++)
			{
				aaiDistances[iI][iJ] = 0;
			}
			CvPlot *pPlotI = GET_PLAYER((PlayerTypes)iI).getStartingPlot();
			if (pPlotI != NULL)
			{
				for (iJ = 0; iJ < iI; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isAlive())
					{
						CvPlot *pPlotJ = GET_PLAYER((PlayerTypes)iJ).getStartingPlot();
						if (pPlotJ != NULL)
						{
							int iDist = GC.getMap().calculatePathDistance(pPlotI, pPlotJ);
							if (iDist == -1)
							{
								// 5x penalty for not being on the same area, or having no passable route
								iDist = 5*plotDistance(pPlotI->getX(), pPlotI->getY(), pPlotJ->getX(), pPlotJ->getY());
							}
							aaiDistances[iI][iJ] = iDist;
						}
					}
				}
			}
		}
		else
		{
			aaiDistances[iI] = NULL;
		}
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		aiStartingLocs[iI] = iI; // each player starting in own location
	}

	int iBestScore = getTeamClosenessScore(aaiDistances, aiStartingLocs);
	bool bFoundSwap = true;
	while (bFoundSwap)
	{
		bFoundSwap = false;
		for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				for (iJ = 0; iJ < iI; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isAlive())
					{
						int iTemp = aiStartingLocs[iI];
						aiStartingLocs[iI] = aiStartingLocs[iJ];
						aiStartingLocs[iJ] = iTemp;
						int iScore = getTeamClosenessScore(aaiDistances, aiStartingLocs);
						if (iScore < iBestScore)
						{
							iBestScore = iScore;
							bFoundSwap = true;
						}
						else
						{
							// Swap them back:
							iTemp = aiStartingLocs[iI];
							aiStartingLocs[iI] = aiStartingLocs[iJ];
							aiStartingLocs[iJ] = iTemp;
						}
					}
				}
			}
		}
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		apNewStartPlots[iI] = NULL;
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (aiStartingLocs[iI] != iI)
			{
				apNewStartPlots[iI] = GET_PLAYER((PlayerTypes)aiStartingLocs[iI]).getStartingPlot();
			}
		}
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (apNewStartPlots[iI] != NULL)
			{
				GET_PLAYER((PlayerTypes)iI).setStartingPlot(apNewStartPlots[iI], false);
			}
		}
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		SAFE_DELETE_ARRAY(aaiDistances[iI]);
	}
}

/* <advc.108>: Three levels of start plot normalization:
	 1: low (weak starting plots on average, high variance); for single-player
	 2: high (strong starting plots, low variance); for multi-player
	 3: very high (very strong starting plots, low variance);  BtS/ K-Mod behavior
	 (the differences between all three aren't great) */
int CvGame::getNormalizationLevel() const {

	return m_iNormalizationLevel;
} // </advc.108


void CvGame::normalizeAddRiver()  // advc.003: style changes
{
	CvMap const& m = GC.getMap();
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer const& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (!kLoopPlayer.isAlive())
			continue;

		CvPlot* pStartingPlot = kLoopPlayer.getStartingPlot();
		if (pStartingPlot == NULL)
			continue;
		if (pStartingPlot->isFreshWater())
			continue;

		// if we will be able to add a lake, then use old river code
		if (normalizeFindLakePlot(kLoopPlayer.getID()) != NULL)
		{
			//CvMapGenerator::GetInstance().doRiver(pStartingPlot);
			// K-Mod. If we can have a lake then we don't always need a river.
			// Also, the river shouldn't always start on the SE corner of our site.
			if (getSorenRandNum(10, "normalize add river") < (pStartingPlot->isCoastalLand() ? 5 : 7))
			{
				CvPlot* pRiverPlot = pStartingPlot->getInlandCorner();
				if (pRiverPlot)
					CvMapGenerator::GetInstance().doRiver(pRiverPlot);
			} // K-Mod end.
		} // otherwise, use new river code which is much more likely to succeed
		else CvMapGenerator::GetInstance().addRiver(pStartingPlot);

		// add floodplains to any desert tiles the new river passes through
		for (int iJ = 0; iJ < m.numPlots(); iJ++)
		{
			CvPlot* pPlot = m.plotByIndex(iJ);
			for (int iK = 0; iK < GC.getNumFeatureInfos(); iK++)
			{
				FeatureTypes eLoopFeature = (FeatureTypes)iK;
				if (!GC.getFeatureInfo(eLoopFeature).isRequiresRiver() ||
						!pPlot->canHaveFeature(eLoopFeature))
					continue;

				if (GC.getFeatureInfo(eLoopFeature).getAppearanceProbability() == 10000)
				{
					if (pPlot->getBonusType() != NO_BONUS)
						pPlot->setBonusType(NO_BONUS);
					pPlot->setFeatureType(eLoopFeature);
					break;
				}
			}
		}
	}
}


void CvGame::normalizeRemovePeaks()  // advc.003: style changes
{
	// <advc.108>
	double prRemoval = 1;
	if(m_iNormalizationLevel <= 1)
		prRemoval = GC.getDefineINT("REMOVAL_CHANCE_PEAK") / 100.0;
	// </advc.108>

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer const& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (!kLoopPlayer.isAlive())
			continue;

		CvPlot* pStartingPlot = GET_PLAYER((PlayerTypes)iI).getStartingPlot();
		if (pStartingPlot == NULL)
			continue;

		int const iRange = 3;
		for (int iDX = -(iRange); iDX <= iRange; iDX++)
		{
			for (int iDY = -(iRange); iDY <= iRange; iDY++)
			{
				CvPlot*pLoopPlot = plotXY(pStartingPlot->getX(),
						pStartingPlot->getY(), iDX, iDY);
				if (pLoopPlot == NULL)
					continue;
				if (pLoopPlot->isPeak()
						&& ::bernoulliSuccess(prRemoval, "advc.108")) // advc.108
					pLoopPlot->setPlotType(PLOT_HILLS);
			}
		}
	}
}

void CvGame::normalizeAddLakes()
{
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			CvPlot* pLakePlot = normalizeFindLakePlot((PlayerTypes)iI);
			if (pLakePlot != NULL)
			{
				pLakePlot->setPlotType(PLOT_OCEAN);
			}
		}
	}
}

CvPlot* CvGame::normalizeFindLakePlot(PlayerTypes ePlayer)  // advc.003: style changes
{
	if (!GET_PLAYER(ePlayer).isAlive())
		return NULL;

	CvPlot* pStartingPlot = GET_PLAYER(ePlayer).getStartingPlot();
	if (pStartingPlot == NULL || pStartingPlot->isFreshWater())
		return NULL;

	// K-Mod. Shuffle the order that plots are checked.
	int aiShuffle[NUM_CITY_PLOTS];
	shuffleArray(aiShuffle, NUM_CITY_PLOTS, getMapRand());
	// K-Mod end
	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = plotCity(pStartingPlot->getX(), pStartingPlot->getY(),
				aiShuffle[iI]); // K-Mod

		if (pLoopPlot == NULL || pLoopPlot->isWater() ||
				pLoopPlot->isCoastalLand() || pLoopPlot->isRiver() ||
				pLoopPlot->getBonusType() != NO_BONUS)
			continue;

		bool bStartingPlot = false;
		for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
		{
			CvPlayer const& kLoopPlayer = GET_PLAYER((PlayerTypes)iJ);
			if (!kLoopPlayer.isAlive())
				continue;
			if (kLoopPlayer.getStartingPlot() == pLoopPlot)
			{
				bStartingPlot = true;
				break;
			}
		}

		if (!bStartingPlot)
			return pLoopPlot;
	}
	return NULL;
}


void CvGame::normalizeRemoveBadFeatures()  // advc.003: style changes
{
	// advc.108
	int const iThreshBadFeatPerCity = GC.getDefineINT("THRESH-BAD-FEAT-PER-CITY");

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayerAI const& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if(!kLoopPlayer.isAlive())
			continue;
		CvPlot* pStartingPlot = kLoopPlayer.getStartingPlot();
		if(pStartingPlot == NULL)
			continue;
		// <advc.108>
		int iBadFeatures = 0;
		for(int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++) {
			CvPlot* p = plotCity(pStartingPlot->getX(), pStartingPlot->getY(), iJ);
			// Disregard inner ring
			if(p == NULL || ::plotDistance(p, pStartingPlot) < 2 ||
					p->getFeatureType() == NO_FEATURE)
				continue;
			if(GC.getFeatureInfo(p->getFeatureType()).getYieldChange(YIELD_FOOD) <= 0 &&
					GC.getFeatureInfo(p->getFeatureType()).getYieldChange(YIELD_PRODUCTION) <= 0)
				iBadFeatures++;
		}
		double prRemoval = 0;
		if(iBadFeatures > iThreshBadFeatPerCity) {
			prRemoval = 1.0 - m_iNormalizationLevel *
					(iThreshBadFeatPerCity / (double)iBadFeatures);
		}
		if(m_iNormalizationLevel >= 3)
			prRemoval = 1;
		// </advc.108>
		for(int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++) {
			CvPlot* pLoopPlot = plotCity(pStartingPlot->getX(),
					pStartingPlot->getY(), iJ);
			if(pLoopPlot != NULL && pLoopPlot->getFeatureType() != NO_FEATURE) {
				if(GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_FOOD) <= 0 &&
						GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_PRODUCTION) <= 0) {
					// <advc.108>
					if(::plotDistance(pLoopPlot, pStartingPlot) < 2 ||
							(!isPowerfulStartingBonus(*pLoopPlot, kLoopPlayer.getID()) &&
							::bernoulliSuccess(prRemoval, "advc.108"))) // </advc.108>
						pLoopPlot->setFeatureType(NO_FEATURE);
				}
			}
		}

		int iCityRange = CITY_PLOTS_RADIUS;
		int iExtraRange = 2;
		int iMaxRange = iCityRange + iExtraRange;
		for (int iX = -iMaxRange; iX <= iMaxRange; iX++)
		{
			for (int iY = -iMaxRange; iY <= iMaxRange; iY++)
			{
				CvPlot* pLoopPlot = plotXY(pStartingPlot->getX(),
						pStartingPlot->getY(), iX, iY);
				if(pLoopPlot == NULL)
					continue;
				int iDistance = plotDistance(pStartingPlot->getX(),
						pStartingPlot->getY(),
						pLoopPlot->getX(), pLoopPlot->getY());
				if(iDistance <= iMaxRange &&
						pLoopPlot->getFeatureType() != NO_FEATURE &&
						GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_FOOD) <= 0 &&
						GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_PRODUCTION) <= 0)
				{
					if (pLoopPlot->isWater())
					{
						if (pLoopPlot->isAdjacentToLand() ||
								(iDistance != iMaxRange &&
								getSorenRandNum(2, "Remove Bad Feature") == 0))
							pLoopPlot->setFeatureType(NO_FEATURE);
					}
					else if (iDistance != iMaxRange)
					{
						// <advc.108> Plots outside the city range: reduced chance of removal
						if((m_iNormalizationLevel > 2 &&
								getSorenRandNum((2 + ((pLoopPlot->getBonusType() == NO_BONUS) ? 0 : 2)), "Remove Bad Feature") == 0) || // original check
								(m_iNormalizationLevel <= 2 &&
								getSorenRandNum((3 - ((pLoopPlot->getBonusType() == NO_BONUS) ? 1 : 0)), "advc.108") != 0))
						// </advc.108>
							pLoopPlot->setFeatureType(NO_FEATURE);
					}
				}
			}
		}
	}
}


void CvGame::normalizeRemoveBadTerrain()  // advc.003: style changes
{
	// <advc.108>
	double prKeep = 0;
	if(m_iNormalizationLevel <= 1)
		prKeep = 1 - GC.getDefineINT("REMOVAL_CHANCE_BAD_TERRAIN") / 100.0;
	// </advc.108>

	int iCityRange = CITY_PLOTS_RADIUS;
	int iExtraRange = 1;
	int iMaxRange = iCityRange + iExtraRange;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayerAI const& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if(!kLoopPlayer.isAlive())
			continue;
		CvPlot* pStartingPlot = kLoopPlayer.getStartingPlot();
		if(pStartingPlot == NULL)
			continue;
		for (int iX = -iMaxRange; iX <= iMaxRange; iX++)
		{
			for (int iY = -iMaxRange; iY <= iMaxRange; iY++)
			{
				CvPlot* pLoopPlot = plotXY(pStartingPlot->getX(),
						pStartingPlot->getY(), iX, iY);
				if(pLoopPlot == NULL)
					continue;
				int iDistance = plotDistance(pStartingPlot->getX(), pStartingPlot->getY(),
						pLoopPlot->getX(), pLoopPlot->getY());
				if(iDistance > iMaxRange)
					continue;
				if(!pLoopPlot->isWater() && (iDistance <= iCityRange ||
						pLoopPlot->isCoastalLand() || getSorenRandNum(
						1 + iDistance - iCityRange, "Map Upgrade Terrain Food") == 0))
				{
					CvTerrainInfo const& kTerrain = GC.getTerrainInfo(pLoopPlot->getTerrainType());
					int iPlotFood = kTerrain.getYield(YIELD_FOOD);
					int iPlotProduction = kTerrain.getYield(YIELD_PRODUCTION);
					if (iPlotFood + iPlotProduction > 1)
						continue;
					// <advc.108>
					if(isPowerfulStartingBonus(*pLoopPlot, kLoopPlayer.getID()))
						continue;
					/*  I think the BtS code ends up replacing Desert with Desert when
						there's a feature, but let's rather handle Desert features explicitly. */
					if(pLoopPlot->getFeatureType() != NO_FEATURE &&
							GC.getFeatureInfo(pLoopPlot->getFeatureType()).
							getYieldChange(YIELD_FOOD) + iPlotFood >= 2)
						continue;
					if(::bernoulliSuccess(prKeep, "advc.108")) {
						if(iPlotFood > 0 ||
							/*  advc.129b: Two chances of removal for Snow river
								(BuildModifier=50), but not for Desert river. */
								(pLoopPlot->isRiver() && kTerrain.getBuildModifier() < 30) ||
								::bernoulliSuccess(prKeep, "advc.108"))
							continue;
					} // </advc.108>
					int const iTargetTotal = 2;
					int iTargetFood = 1;
					if (pLoopPlot->getBonusType(kLoopPlayer.getTeam()) != NO_BONUS)
						iTargetFood = 1;
					else if (iPlotFood == 1 || iDistance <= iCityRange)
						iTargetFood = 1 + getSorenRandNum(2, "Map Upgrade Terrain Food");
					else iTargetFood = pLoopPlot->isCoastalLand() ? 2 : 1;

					for (int iK = 0; iK < GC.getNumTerrainInfos(); iK++)
					{
						CvTerrainInfo const& kRepl = GC.getTerrainInfo((TerrainTypes)iK);
						if (kRepl.isWater())
							continue;
						if (kRepl.getYield(YIELD_FOOD) >= iTargetFood &&
								kRepl.getYield(YIELD_FOOD) +
								kRepl.getYield(YIELD_PRODUCTION) == iTargetTotal)
						{
							if (pLoopPlot->getFeatureType() == NO_FEATURE ||
									GC.getFeatureInfo(pLoopPlot->getFeatureType()).
									isTerrain(iK))
								pLoopPlot->setTerrainType((TerrainTypes)iK);
						}
					}
				}
			}
		}
	}
}


void CvGame::normalizeAddFoodBonuses()  // advc.003: style changes
{
	bool bIgnoreLatitude = pythonIsBonusIgnoreLatitudes();
	int iFoodPerPop = GC.getFOOD_CONSUMPTION_PER_POPULATION(); // K-Mod

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		const CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI); // K-Mod
		if (!kLoopPlayer.isAlive())
			continue;

		CvPlot* pStartingPlot = kLoopPlayer.getStartingPlot();
		if (pStartingPlot == NULL)
			continue;

		int iFoodBonus = 0;
		int iGoodNatureTileCount = 0;
		//for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
		for (int iJ = 1; iJ < NUM_CITY_PLOTS; iJ++) // K-Mod. Don't count the city plot.
		{
			CvPlot* pLoopPlot = plotCity(pStartingPlot->getX(), pStartingPlot->getY(), iJ);
			if (pLoopPlot == NULL)
				continue;

			BonusTypes eBonus = pLoopPlot->getBonusType(kLoopPlayer.getTeam());
			if (eBonus != NO_BONUS)
			{
				CvBonusInfo const& kBonus = GC.getBonusInfo(eBonus);
				if (kBonus.getYieldChange(YIELD_FOOD) > 0)
				{
					if (kBonus.getTechCityTrade() == NO_TECH || GC.getTechInfo((TechTypes)
							kBonus.getTechCityTrade()).getEra() <= getStartEra())
					{
						if (pLoopPlot->isWater())
							iFoodBonus += 2;
						else
						{
							//iFoodBonus += 3;
							// K-Mod. Bonus which only give 3 food with their improvement should not be worth 3 points. (ie. plains-cow should not be the only food resource.)
							/* first attempt - this doesn't work, because "max yield" essentially means +2 food on any plot. That isn't what we want.
							if (pLoopPlot->calculateMaxYield(YIELD_FOOD) >= 2*iFoodPerPop) // ie. >= 4
								iFoodBonus += 3;
							else iFoodBonus += 2; */
							int iNaturalFood = pLoopPlot->calculateBestNatureYield(YIELD_FOOD,
									kLoopPlayer.getTeam());
							int iHighFoodThreshold = 2*iFoodPerPop; // ie. 4 food.
							bool bHighFood = iNaturalFood + 1 >= iHighFoodThreshold; // (+1 just as a shortcut to save time for obvious cases.)

							for (ImprovementTypes eImp = (ImprovementTypes)0; !bHighFood && eImp < GC.getNumImprovementInfos(); eImp=(ImprovementTypes)(eImp+1))
							{
								if (GC.getImprovementInfo(eImp).isImprovementBonusTrade(eBonus))
								{
									bHighFood = iNaturalFood + pLoopPlot->calculateImprovementYieldChange(
											eImp, YIELD_FOOD, kLoopPlayer.getID(), false, false) >=
											iHighFoodThreshold;
								}
							}
							iFoodBonus += bHighFood ? 3 : 2;
							// K-Mod end
						}
					}
				}
				else if (pLoopPlot->calculateBestNatureYield(YIELD_FOOD, kLoopPlayer.getTeam()) >= iFoodPerPop)
					iGoodNatureTileCount++;
			}
			else if (pLoopPlot->calculateBestNatureYield(YIELD_FOOD, kLoopPlayer.getTeam()) >= iFoodPerPop+1)
				iGoodNatureTileCount++;
		}

		int iTargetFoodBonusCount = 3;
		// advc.108: (Don't do this after all:)
		//int iTargetFoodBonusCount = m_iNormalizationLevel;
		iTargetFoodBonusCount += std::max(0, 2-iGoodNatureTileCount); // K-Mod

		// K-Mod. I've rearranged a couple of things to make it a bit more efficient and easier to read.
		for (int iJ = 1; iJ < NUM_CITY_PLOTS; iJ++)
		{
			if (iFoodBonus >= iTargetFoodBonusCount)
				break;

			CvPlot* pLoopPlot = plotCity(pStartingPlot->getX(), pStartingPlot->getY(), iJ);
			if (pLoopPlot == NULL || pLoopPlot->getBonusType() != NO_BONUS)
				continue;

			for (int iK = 0; iK < GC.getNumBonusInfos(); iK++)
			{
				const CvBonusInfo& kLoopBonus = GC.getBonusInfo((BonusTypes)iK);
				if (!kLoopBonus.isNormalize() || kLoopBonus.getYieldChange(YIELD_FOOD) <= 0)
					continue;

				if (kLoopBonus.getTechCityTrade() != NO_TECH &&
						GC.getTechInfo((TechTypes)kLoopBonus.getTechCityTrade()).
						getEra() > getStartEra())
					continue;
				if (!GET_TEAM(kLoopPlayer.getTeam()).isHasTech((TechTypes)kLoopBonus.getTechReveal()))
					continue;
				// <advc.108> Don't place the food resource on a bad feature
				FeatureTypes eFeature = pLoopPlot->getFeatureType();
				bool bValid = true;
				if(eFeature != NO_FEATURE) {
					CvFeatureInfo& kFeature = GC.getFeatureInfo(eFeature);
					bValid = false;
					if(m_iNormalizationLevel >= 3 || kFeature.getYieldChange(YIELD_FOOD) > 0 ||
							kFeature.getYieldChange(YIELD_PRODUCTION) > 0)
						bValid = true;
				}
				if(!bValid)
					continue; // </advc.108>
				if (!pLoopPlot->canHaveBonus((BonusTypes)iK, bIgnoreLatitude))
					continue;

				pLoopPlot->setBonusType((BonusTypes)iK);
				if (pLoopPlot->isWater())
					iFoodBonus += 2;
				else
				{
					//iFoodBonus += 3;
					// K-Mod
					int iNaturalFood = pLoopPlot->calculateBestNatureYield(YIELD_FOOD, kLoopPlayer.getTeam());
					int iHighFoodThreshold = 2*iFoodPerPop; // ie. 4 food.
					bool bHighFood = iNaturalFood + 1 >= iHighFoodThreshold; // (+1 just as a shortcut to save time for obvious cases.)

					for (ImprovementTypes eImp = (ImprovementTypes)0; !bHighFood && eImp < GC.getNumImprovementInfos(); eImp=(ImprovementTypes)(eImp+1))
					{
						if (GC.getImprovementInfo(eImp).isImprovementBonusTrade((BonusTypes)iK))
						{
							bHighFood = iNaturalFood + pLoopPlot->calculateImprovementYieldChange(
									eImp, YIELD_FOOD, (PlayerTypes)iI, false, false) >= iHighFoodThreshold;
						}
					}
					iFoodBonus += bHighFood ? 3 : 2;
					// K-Mod end
				}
				break;
			}
		}
	}
}


void CvGame::normalizeAddGoodTerrain()
{
	// <advc.108>
	if(m_iNormalizationLevel <= 1)
		return; // </advc.108>
	CvPlot* pStartingPlot;
	CvPlot* pLoopPlot;
	bool bChanged;
	int iGoodPlot;
	int iI, iJ, iK;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			pStartingPlot = GET_PLAYER((PlayerTypes)iI).getStartingPlot();

			if (pStartingPlot != NULL)
			{
				iGoodPlot = 0;

				for (iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
				{
					pLoopPlot = plotCity(pStartingPlot->getX(), pStartingPlot->getY(), iJ);

					if (pLoopPlot != NULL)
					{
						if (pLoopPlot != pStartingPlot)
						{
							if ((pLoopPlot->calculateNatureYield(YIELD_FOOD, GET_PLAYER((PlayerTypes)iI).getTeam()) >= GC.getFOOD_CONSUMPTION_PER_POPULATION()) &&
								  (pLoopPlot->calculateNatureYield(YIELD_PRODUCTION, GET_PLAYER((PlayerTypes)iI).getTeam()) > 0))
							{
								iGoodPlot++;
							}
						}
					}
				}

				for (iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
				{
					if (iGoodPlot >= 4)
					{
						break;
					}

					pLoopPlot = plotCity(pStartingPlot->getX(), pStartingPlot->getY(), iJ);

					if (pLoopPlot != NULL)
					{
						if (pLoopPlot != pStartingPlot)
						{
							if (!(pLoopPlot->isWater()))
							{
								if (!(pLoopPlot->isHills()))
								{
									if (pLoopPlot->getBonusType() == NO_BONUS)
									{
										bChanged = false;

										if (pLoopPlot->calculateNatureYield(YIELD_FOOD, GET_PLAYER((PlayerTypes)iI).getTeam()) < GC.getFOOD_CONSUMPTION_PER_POPULATION())
										{
											for (iK = 0; iK < GC.getNumTerrainInfos(); iK++)
											{
												if (!(GC.getTerrainInfo((TerrainTypes)iK).isWater()))
												{
													if (GC.getTerrainInfo((TerrainTypes)iK).getYield(YIELD_FOOD) >= GC.getFOOD_CONSUMPTION_PER_POPULATION())
													{
														pLoopPlot->setTerrainType((TerrainTypes)iK);
														bChanged = true;
														break;
													}
												}
											}
										}

										if (pLoopPlot->calculateNatureYield(YIELD_PRODUCTION, GET_PLAYER((PlayerTypes)iI).getTeam()) == 0)
										{
											for (iK = 0; iK < GC.getNumFeatureInfos(); iK++)
											{
												if ((GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_FOOD) >= 0) &&
													  (GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_PRODUCTION) > 0))
												{
													if (GC.getFeatureInfo((FeatureTypes)iK).isTerrain(pLoopPlot->getTerrainType()))
													{
														pLoopPlot->setFeatureType((FeatureTypes)iK);
														bChanged = true;
														break;
													}
												}
											}
										}

										if (bChanged)
										{
											iGoodPlot++;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}


void CvGame::normalizeAddExtras()  // advc.003: Some changes to reduce indentation
{
	bool bIgnoreLatitude = pythonIsBonusIgnoreLatitudes();
	int iTotalValue = 0;
	int iPlayerCount = 0;
	int iBestValue = 0;
	int iWorstValue = MAX_INT;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayerAI const& kPlayer = GET_PLAYER((PlayerTypes)iI);
		if (!kPlayer.isAlive())
			continue;

		CvPlot* pStartingPlot = kPlayer.getStartingPlot();
		if (pStartingPlot == NULL)
			continue;

		int iValue = kPlayer.AI_foundValue(pStartingPlot->getX(), pStartingPlot->getY(), -1, true);
		iTotalValue += iValue;
		iPlayerCount++;
		iBestValue = std::max(iValue, iBestValue);
		iWorstValue = std::min(iValue, iWorstValue);
	}

	//iTargetValue = (iTotalValue + iBestValue) / (iPlayerCount + 1);
	int iTargetValue = (iBestValue * 4) / 5;
	// <advc.108>
	if(m_iNormalizationLevel <= 1)
		iTargetValue = GC.getDefineINT("STARTVAL_LOWER_BOUND-PERCENT") * iBestValue / 100;
	// </advc.108>
	logBBAI("Adding extras to normalize starting positions. (target value: %d)", iTargetValue); // K-Mod

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI); // K-Mod
		// K-Mod note: The following two 'continue' conditions were originally enourmous if blocks. I just changed it for readability.

		if (!kLoopPlayer.isAlive())
			continue;

		CvPlot* pStartingPlot = kLoopPlayer.getStartingPlot();
		if (pStartingPlot == NULL)
			continue;

		gDLL->callUpdater();	// allow window to update during launch

		int iCount = 0;
		int iFeatureCount = 0;
		int aiShuffle[NUM_CITY_PLOTS];
		shuffleArray(aiShuffle, NUM_CITY_PLOTS, getMapRand());
		for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
		{
			if (kLoopPlayer.AI_foundValue(pStartingPlot->getX(), pStartingPlot->getY(), -1, true) >= iTargetValue)
			{
				if (gMapLogLevel > 0)
					logBBAI("    Player %d doesn't need any more features.", iI); // K-Mod
				break;
			}
			if (getSorenRandNum(iCount + 2, "Setting Feature Type") <= 1)
			{
				CvPlot* pLoopPlot = plotCity(pStartingPlot->getX(), pStartingPlot->getY(), aiShuffle[iJ]);
				if (pLoopPlot == NULL || pLoopPlot == pStartingPlot ||
						pLoopPlot->getBonusType() != NO_BONUS)
					continue;

				if (pLoopPlot->getFeatureType() == NO_FEATURE)
				{
					for (int iK = 0; iK < GC.getNumFeatureInfos(); iK++)
					{
						if ((GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_FOOD) +
								GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_PRODUCTION)) > 0)
						{
							if (pLoopPlot->canHaveFeature((FeatureTypes)iK))
							{
								if (gMapLogLevel > 0)
									logBBAI("    Adding %S for player %d.", GC.getFeatureInfo((FeatureTypes)iK).getDescription(), iI); // K-Mod
								pLoopPlot->setFeatureType((FeatureTypes)iK);
								iCount++;
								break;
							}
						}
					}
					iFeatureCount += (pLoopPlot->getFeatureType() != NO_FEATURE) ? 1 : 0;
				}
			}
		}

		int iCoastFoodCount = 0;
		int iOceanFoodCount = 0;
		int iOtherCount = 0;
		int iWaterCount = 0;
		for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
		{
			CvPlot* pLoopPlot = plotCity(pStartingPlot->getX(), pStartingPlot->getY(), iJ);
			if (pLoopPlot == NULL || pLoopPlot == pStartingPlot)
				continue;

			if (pLoopPlot->isWater())
			{
				iWaterCount++;
				if (pLoopPlot->getBonusType() != NO_BONUS)
				{
					if (pLoopPlot->isAdjacentToLand())
						iCoastFoodCount++;
					else iOceanFoodCount++;
				}
			}
			else if (pLoopPlot->getBonusType( // <advc.108> Don't count unrevealed bonuses
					m_iNormalizationLevel > 1 ?
					NO_TEAM : kLoopPlayer.getTeam()) /* </advc.108> */ != NO_BONUS)
				iOtherCount++;
		}

		bool bLandBias = (iWaterCount > NUM_CITY_PLOTS / 2);
		shuffleArray(aiShuffle, NUM_CITY_PLOTS, getMapRand());
		for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
		{
			CvPlot* pLoopPlot = plotCity(pStartingPlot->getX(), pStartingPlot->getY(), aiShuffle[iJ]);
			if (pLoopPlot == NULL || pLoopPlot == pStartingPlot)
				continue;

			if (getSorenRandNum((bLandBias && pLoopPlot->isWater()) ? 2 : 1, "Placing Bonuses") == 0)
			{
				if (iOtherCount * 3 + iOceanFoodCount * 2 + iCoastFoodCount * 2 >= 12)
					break;

				if (kLoopPlayer.AI_foundValue(pStartingPlot->getX(), pStartingPlot->getY(), -1, true) >= iTargetValue)
				{
					if (gMapLogLevel > 0)
						logBBAI("    Player %d doesn't need any more bonuses.", iI); // K-Mod
					break;
				}
				bool bCoast = (pLoopPlot->isWater() && pLoopPlot->isAdjacentToLand());
				bool bOcean = (pLoopPlot->isWater() && !bCoast);
				if ((pLoopPlot != pStartingPlot)
						&& !(bCoast && iCoastFoodCount >= 2) // advc.108: was >2
						&& !(bOcean && iOceanFoodCount >= 2) // advc.108: was >2
						// advc.108: At most 3 sea food
						&& !((bOcean || bCoast) && iOceanFoodCount + iCoastFoodCount >= 3))
				{
					for (int iPass = 0; iPass < 2; iPass++)
					{
						if (pLoopPlot->getBonusType() != NO_BONUS)
							continue;

						for (int iK = 0; iK < GC.getNumBonusInfos(); iK++)
						{	// advc.003: Checks moved into auxiliary function
							if(!isValidExtraBonus((BonusTypes)iK, kLoopPlayer.getID(),
									*pLoopPlot, iPass == 0, bIgnoreLatitude))
								continue;
							if (gMapLogLevel > 0)
								logBBAI("    Adding %S for player %d.", GC.getBonusInfo((BonusTypes)iK).getDescription(), iI); // K-Mod
							pLoopPlot->setBonusType((BonusTypes)iK);
							iCoastFoodCount += bCoast ? 1 : 0;
							iOceanFoodCount += bOcean ? 1 : 0;
							iOtherCount += !(bCoast || bOcean) ? 1 : 0;
							break;
						}
						if (!bLandBias || pLoopPlot->isWater() || pLoopPlot->getBonusType() != NO_BONUS)
							continue;

						if ((iFeatureCount > 4 && pLoopPlot->getFeatureType() != NO_FEATURE)
								&& iCoastFoodCount + iOceanFoodCount > 2 &&
								getSorenRandNum(2, "Clear feature to add bonus") == 0)
						{
							if (gMapLogLevel > 0)
								logBBAI("    Removing %S to place bonus for player %d.", GC.getFeatureInfo(pLoopPlot->getFeatureType()).getDescription(), iI); // K-Mod

							pLoopPlot->setFeatureType(NO_FEATURE);
							for (int iK = 0; iK < GC.getNumBonusInfos(); iK++)
							{	// advc.003: Checks moved into auxiliary function
								if(!isValidExtraBonus((BonusTypes)iK, kLoopPlayer.getID(),
										*pLoopPlot, iPass == 0, bIgnoreLatitude))
									continue;

								if (gMapLogLevel > 0)
									logBBAI("    Adding %S for player %d.", GC.getBonusInfo((BonusTypes)iK).getDescription(), iI); // K-Mod
								pLoopPlot->setBonusType((BonusTypes)iK);
								iOtherCount++;
								break;
							}
						}
					}
				}
			}
		}

		shuffleArray(aiShuffle, NUM_CITY_PLOTS, getMapRand());
		for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
		{
			if (kLoopPlayer.AI_foundValue(pStartingPlot->getX(), pStartingPlot->getY(), -1, true) >= iTargetValue)
			{
				if (gMapLogLevel > 0)
					logBBAI("    Player %d doesn't need any more features (2).", iI); // K-Mod
				break;
			}
			CvPlot* pLoopPlot = plotCity(pStartingPlot->getX(), pStartingPlot->getY(), aiShuffle[iJ]);

			if (pLoopPlot == NULL || pLoopPlot == pStartingPlot ||
					pLoopPlot->getBonusType() != NO_BONUS || pLoopPlot->getFeatureType() != NO_FEATURE)
				continue;

			for (int iK = 0; iK < GC.getNumFeatureInfos(); iK++)
			{
				if ((GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_FOOD) + GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_PRODUCTION)) > 0)
				{
					if (pLoopPlot->canHaveFeature((FeatureTypes)iK))
					{
						if (gMapLogLevel > 0)
							logBBAI("    Adding %S for player %d.", GC.getFeatureInfo((FeatureTypes)iK).getDescription(), iI); // K-Mod
						pLoopPlot->setFeatureType((FeatureTypes)iK);
						break;
					}
				}
			}
		}
		int iHillsCount = 0;
		for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
		{
			CvPlot* pLoopPlot = plotCity(pStartingPlot->getX(), pStartingPlot->getY(), iJ);
			if (pLoopPlot == NULL)
				continue;

			if (pLoopPlot->isHills())
				iHillsCount++;
		}
		shuffleArray(aiShuffle, NUM_CITY_PLOTS, getMapRand());
		for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
		{
			if (iHillsCount >= 3)
				break;

			CvPlot* pLoopPlot = plotCity(pStartingPlot->getX(), pStartingPlot->getY(), aiShuffle[iJ]);
			if (pLoopPlot == NULL || pLoopPlot->isWater() || pLoopPlot->isHills())
				continue;

			if (pLoopPlot->getFeatureType() == NO_FEATURE ||
					!GC.getFeatureInfo(pLoopPlot->getFeatureType()).isRequiresFlatlands())
			{
				if ((pLoopPlot->getBonusType() == NO_BONUS) ||
						GC.getBonusInfo(pLoopPlot->getBonusType()).isHills())
				{
					if (gMapLogLevel > 0)
						logBBAI("    Adding hills for player %d.", iI); // K-Mod
					pLoopPlot->setPlotType(PLOT_HILLS, false, true);
					iHillsCount++;
				}
			}
		}
		if (gMapLogLevel > 0)
			logBBAI("    Player %d final value: %d", iI, kLoopPlayer.AI_foundValue(pStartingPlot->getX(), pStartingPlot->getY(), -1, true)); // K-Mod
	}
	if (gMapLogLevel > 0)
		logBBAI("normalizeAddExtras() complete"); // K-Mod
}


void CvGame::normalizeStartingPlots()
{
	PROFILE_FUNC();

	if (!GC.getInitCore().getWBMapScript() || GC.getInitCore().getWBMapNoPlayers())
	{
		if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeStartingPlotLocations", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
		{
			normalizeStartingPlotLocations();
		}
	}

	if (GC.getInitCore().getWBMapScript())
	{
		return;
	}

	if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeAddRiver", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
	{
		normalizeAddRiver();
	}

	if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeRemovePeaks", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
	{
		normalizeRemovePeaks();
	}

	if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeAddLakes", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
	{
		normalizeAddLakes();
	}

	if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeRemoveBadFeatures", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
	{
		normalizeRemoveBadFeatures();
	}

	if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeRemoveBadTerrain", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
	{
		normalizeRemoveBadTerrain();
	}

	if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeAddFoodBonuses", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
	{
		normalizeAddFoodBonuses();
	}

	if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeAddGoodTerrain", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
	{
		normalizeAddGoodTerrain();
	}

	if (!gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "normalizeAddExtras", NULL)  || gDLL->getPythonIFace()->pythonUsingDefaultImpl())
	{
		normalizeAddExtras();
	}
}

// <advc.003> Cut, pasted, refactored from normalizeAddExtras
bool CvGame::isValidExtraBonus(BonusTypes eBonus, PlayerTypes eStartPlayer,
		CvPlot const& kStartPlot, bool bCheckCanPlace, bool bIgnoreLatitude) const {

	CvBonusInfo const& kBonus = GC.getBonusInfo(eBonus);
	if (!kBonus.isNormalize())
		return false;

	if (kBonus.getYieldChange(YIELD_FOOD) < 0 ||
			kBonus.getYieldChange(YIELD_PRODUCTION) < 0)
		return false;

	if (kBonus.getTechCityTrade() != NO_TECH &&
			GC.getTechInfo((TechTypes)(kBonus.getTechCityTrade())).getEra() > getStartEra())
		return false;
	/*  advc: BtS had checked this only for seafood; doesn't really matter though
		b/c all of the isNormalize resources are revealed from the start. */
	if (!TEAMREF(eStartPlayer).isHasTech((TechTypes)kBonus.getTechReveal()))
		return false;

	if (bCheckCanPlace ? CvMapGenerator::GetInstance().
			canPlaceBonusAt(eBonus, kStartPlot.getX(), kStartPlot.getY(), bIgnoreLatitude) :
			kStartPlot.canHaveBonus(eBonus, bIgnoreLatitude))
		return true;

	return false;
} // </advc.003>

// <advc.108>
bool CvGame::isPowerfulStartingBonus(CvPlot const& kStartPlot, PlayerTypes eStartPlayer) const {

	if(getStartEra() > 0)
		return false;
	BonusTypes eBonus = kStartPlot.getBonusType(TEAMID(eStartPlayer));
	if(eBonus == NO_BONUS)
		return false;
	return (GC.getBonusInfo(eBonus).getBonusClassType() ==
			GC.getInfoTypeForString("BONUSCLASS_PRECIOUS"));
} // </advc.108>

// For each of n teams, let the closeness score for that team be the average distance of an edge between two players on that team.
// This function calculates the closeness score for each team and returns the sum of those n scores.
// The lower the result, the better "clumped" the players' starting locations are.
//
// Note: for the purposes of this function, player i will be assumed to start in the location of player aiStartingLocs[i]

int CvGame::getTeamClosenessScore(int** aaiDistances, int* aiStartingLocs)
{
	int iScore = 0;

	for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
	{
		if (GET_TEAM((TeamTypes)iTeam).isAlive())
		{
			int iTeamTotalDist = 0;
			int iNumEdges = 0;
			for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
			{
				if (GET_PLAYER((PlayerTypes)iPlayer).isAlive())
				{
					if (GET_PLAYER((PlayerTypes)iPlayer).getTeam() == (TeamTypes)iTeam)
					{
						for (int iOtherPlayer = 0; iOtherPlayer < iPlayer; iOtherPlayer++)
						{
							if (GET_PLAYER((PlayerTypes)iOtherPlayer).getTeam() == (TeamTypes)iTeam)
							{
								// Add the edge between these two players that are on the same team
								iNumEdges++;
								int iPlayerStart = aiStartingLocs[iPlayer];
								int iOtherPlayerStart = aiStartingLocs[iOtherPlayer];

								if (iPlayerStart < iOtherPlayerStart) // Make sure that iPlayerStart > iOtherPlayerStart
								{
									int iTemp = iPlayerStart;
									iPlayerStart = iOtherPlayerStart;
									iOtherPlayerStart = iTemp;
								}
								else if (iPlayerStart == iOtherPlayerStart)
								{
									FAssertMsg(false, "Two players are (hypothetically) assigned to the same starting location!");
								}
								iTeamTotalDist += aaiDistances[iPlayerStart][iOtherPlayerStart];
							}
						}
					}
				}
			}

			int iTeamScore;
			if (iNumEdges == 0)
			{
				iTeamScore = 0;
			}
			else
			{
				iTeamScore = iTeamTotalDist/iNumEdges; // the avg distance between team edges is the team score
			}

			iScore += iTeamScore;
		}
	}
	return iScore;
}


void CvGame::update()
{
	startProfilingDLL(false);
	PROFILE_BEGIN("CvGame::update");

	if (!gDLL->GetWorldBuilderMode() || isInAdvancedStart())
	{
		sendPlayerOptions();

		// sample generic event
		CyArgsList pyArgs;
		pyArgs.add(getTurnSlice());
		/*  advc.210: To prevent BUG alerts from being checked at the start of a
			game turn. I've tried doing that through BugEventManager.py, but soon
			gave up. Tagging advc.706 b/c it's especially important to supress
			the update when R&F is enabled. */
		if(!isInBetweenTurns()) {
			CvEventReporter::getInstance().genericEvent("gameUpdate", pyArgs.makeFunctionArgs());
			// <advc.003r>
			for(int i = 0; i < NUM_UPDATE_TIMER_TYPES; i++)
				handleUpdateTimer((UpdateTimerTypes)i); // </advc.003r>
		}
		if (getTurnSlice() == 0)
		{	// <advc.700> Delay initial auto-save until RiseFall is initialized
			bool bStartTurn = (getGameTurn() == getStartTurn()); // advc.004m
			// I guess TurnSlice==0 already implies that it's the start turn (?)
			FAssert(bStartTurn);
			if((!bStartTurn || !isOption(GAMEOPTION_RISE_FALL)) // </advc.700>
					&& m_iTurnLoadedFromSave != m_iElapsedGameTurns) // advc.044
				autoSave(true); // advc.106l
			/* <advc.004m> This seems to be the earliest place where bubbles can
			   be enabled w/o crashing. */
			if(bStartTurn && getBugOptionBOOL("MainInterface__StartWithResourceIcons", true))
				gDLL->getEngineIFace()->setResourceLayer(true);
			// </advc.004m>
		}
		if (getNumGameTurnActive() == 0)
		{
			if (!isPbem() || !getPbemTurnSent())
			{
				doTurn();
			}
		}

		updateScore();

		updateWar();

		updateMoves();

		updateTimers();

		updateTurnTimer();

		AI_updateAssignWork();

		testAlive();
		AI().warAndPeaceAI().invalidateUICache(); // advc.104l
		if (getAIAutoPlay() == 0 && !gDLL->GetAutorun() && GAMESTATE_EXTENDED != getGameState())
		{
			if (countHumanPlayersAlive() == 0
					&& !isOption(GAMEOPTION_RISE_FALL)) // advc.707
				setGameState(GAMESTATE_OVER);
		}

		changeTurnSlice(1);

		if (NO_PLAYER != getActivePlayer() && GET_PLAYER(getActivePlayer()).getAdvancedStartPoints() >= 0 && !gDLL->getInterfaceIFace()->isInAdvancedStart())
		{
			gDLL->getInterfaceIFace()->setInAdvancedStart(true);
			gDLL->getInterfaceIFace()->setWorldBuilder(true);
		} // <advc.705>
		if(isOption(GAMEOPTION_RISE_FALL))
			m_pRiseFall->restoreDiploText(); // </advc.705>
	}
	PROFILE_END();
	stopProfilingDLL(false);
}


void CvGame::updateScore(bool bForce)
{
	int iI, iJ, iK;

	if(!isScoreDirty() && !bForce)
		return;
	setScoreDirty(false);

	bool abPlayerScored[MAX_CIV_PLAYERS] = { false };
	std::vector<PlayerTypes> aeUpdateAttitude; // advc.001
	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		int iBestScore = MIN_INT;
		PlayerTypes eBestPlayer = NO_PLAYER;

		for (iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
		{
			if (!abPlayerScored[iJ])
			{
				int iScore = GET_PLAYER((PlayerTypes)iJ).calculateScore(false);

				if (iScore >= iBestScore)
				{
					iBestScore = iScore;
					eBestPlayer = (PlayerTypes)iJ;
				}
			}
		}
		// <advc.003>
		if(eBestPlayer == NO_PLAYER) {
			FAssert(eBestPlayer != NO_PLAYER);
			continue;
		} // </advc.003>
		abPlayerScored[eBestPlayer] = true;
		// <advc.001>
		if(iI != getPlayerRank(eBestPlayer))
			aeUpdateAttitude.push_back(eBestPlayer); // </advc.001>
		setRankPlayer(iI, eBestPlayer);
		setPlayerRank(eBestPlayer, iI);
		setPlayerScore(eBestPlayer, iBestScore);
		GET_PLAYER(eBestPlayer).updateScoreHistory(getGameTurn(), iBestScore);
	} // <advc.001>
	/*for(size_t i = 0; i < updateAttitude.size(); i++)
		GET_PLAYER(updateAttitude[i]).AI_updateAttitudeCache();*/
	/*  The above isn't enough; the attitudes of those outside updateAttitude
		toward those inside could also change. */
	if(!aeUpdateAttitude.empty()) {
		for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
			CvPlayerAI& civ = GET_PLAYER((PlayerTypes)i);
			if(civ.isAlive() && !civ.isMinorCiv())
				civ.AI_updateAttitudeCache();
		}
	} // </advc.001>

	bool abTeamScored[MAX_CIV_TEAMS] = { false };
	for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		int iBestScore = MIN_INT;
		TeamTypes eBestTeam = NO_TEAM;

		for (iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
		{
			if (!abTeamScored[iJ])
			{
				int iScore = 0;

				for (iK = 0; iK < MAX_CIV_PLAYERS; iK++)
				{
					if (GET_PLAYER((PlayerTypes)iK).getTeam() == iJ)
					{
						iScore += getPlayerScore((PlayerTypes)iK);
					}
				}

				if (iScore >= iBestScore)
				{
					iBestScore = iScore;
					eBestTeam = (TeamTypes)iJ;
				}
			}
		}

		abTeamScored[eBestTeam] = true;

		setRankTeam(iI, eBestTeam);
		setTeamRank(eBestTeam, iI);
		setTeamScore(eBestTeam, iBestScore);
	}
}

void CvGame::updatePlotGroups()
{
	PROFILE_FUNC();

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).updatePlotGroups();
		}
	}
}


void CvGame::updateBuildingCommerce()
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).updateBuildingCommerce();
		}
	}
}


void CvGame::updateCitySight(bool bIncrement)
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).updateCitySight(bIncrement, false);
		}
	}

	updatePlotGroups();
}


void CvGame::updateTradeRoutes()
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).updateTradeRoutes();
		}
	}
}

/*  K-Mod
	calculate unhappiness due to the state of global warming */
void CvGame::updateGwPercentAnger()
{
	int iGlobalPollution;
	int iGwSeverityRating;
	int iGlobalDefence;

	int iGwIndex = getGlobalWarmingIndex();

	if (iGwIndex > 0)
	{
		iGlobalPollution = calculateGlobalPollution();
		iGwSeverityRating = calculateGwSeverityRating();
		iGlobalDefence = calculateGwLandDefence(NO_PLAYER);
	} // advc.003: Ensure initialization
	else iGlobalPollution = iGwSeverityRating = iGlobalDefence = -1;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)iI);
		int iAngerPercent = 0;
		if (iGwIndex > 0 && kPlayer.isAlive() && !kPlayer.isMinorCiv())
		{
			// player unhappiness = base rate * severity rating * responsibility factor

			int iLocalDefence = calculateGwLandDefence((PlayerTypes)iI);
			int iResponsibilityFactor =	100*(kPlayer.calculatePollution() - iLocalDefence);

			iResponsibilityFactor /= std::max(1, calculateGwSustainabilityThreshold((PlayerTypes)iI));
			iResponsibilityFactor *= calculateGwSustainabilityThreshold();
			iResponsibilityFactor /= std::max(1, iGlobalPollution - iGlobalDefence);
			// amplify the affects of responsibility
			iResponsibilityFactor = std::max(0, 2*iResponsibilityFactor-100);

			iAngerPercent = GC.getDefineINT("GLOBAL_WARMING_BASE_ANGER_PERCENT") * iGwSeverityRating * iResponsibilityFactor;
			iAngerPercent = ROUND_DIVIDE(iAngerPercent, 10000);// div, 100 * 100
		}
		kPlayer.setGwPercentAnger(iAngerPercent);
	}
} // K-Mod end

/*  <advc.106l> Wrapper that reports the event. Everyone should call this
	instead of calling the CvEngine function directly. */
void CvGame::autoSave(bool bInitial) {
	/*  <advc.135c> Avoid overlapping auto-saves in test games played on a
		single machine. Don't know how to check this properly. */
	if(isNetworkMultiPlayer() && isDebugToolsAllowed(false) && getActivePlayer() % 2 == 0)
		return; // </advc.135c>
	CvEventReporter::getInstance().preAutoSave();
	gDLL->getEngineIFace()->AutoSave(bInitial);
	// BULL - AutoSave - start
	if(bInitial)
		gDLL->getPythonIFace()->callFunction(PYCivModule, "gameStartSave");
	// BULL - AutoSave - end
} // </advc.106l>

void CvGame::testExtendedGame()
{
	if (getGameState() != GAMESTATE_OVER)
	{
		return;
	}

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).isHuman())
			{
				if (GET_PLAYER((PlayerTypes)iI).isExtendedGame())
				{
					setGameState(GAMESTATE_EXTENDED);
					break;
				}
			}
		}
	}
}


void CvGame::cityPushOrder(CvCity* pCity, OrderTypes eOrder, int iData, bool bAlt, bool bShift, bool bCtrl) const
{
	if (pCity->getProduction() > 0)
	{
		//CvMessageControl::getInstance().sendPushOrder(pCity->getID(), eOrder, iData, bAlt, bShift, !bShift);
		CvMessageControl::getInstance().sendPushOrder(pCity->getID(), eOrder, iData, bAlt, false, bShift ? -1 : 0);
	}
	else if ((eOrder == ORDER_TRAIN) && (pCity->getProductionUnit() == iData))
	{
		//CvMessageControl::getInstance().sendPushOrder(pCity->getID(), eOrder, iData, bAlt, !bCtrl, bCtrl);
		CvMessageControl::getInstance().sendPushOrder(pCity->getID(), eOrder, iData, bAlt, false, bCtrl ? 0 : -1);
	}
	else
	{
		//CvMessageControl::getInstance().sendPushOrder(pCity->getID(), eOrder, iData, bAlt, bShift, bCtrl);
		CvMessageControl::getInstance().sendPushOrder(pCity->getID(), eOrder, iData, bAlt, !(bShift || bCtrl), bShift ? -1 : 0);
	}
}


void CvGame::selectUnit(CvUnit* pUnit, bool bClear, bool bToggle, bool bSound) const
{
	PROFILE_FUNC();
	/* original bts code
	if (gDLL->getInterfaceIFace()->getHeadSelectedUnit() == NULL)
		bSelectGroup = true;
	else if (gDLL->getInterfaceIFace()->getHeadSelectedUnit()->getGroup() != pUnit->getGroup())
		bSelectGroup = true;
	else if (pUnit->IsSelected() && !(gDLL->getInterfaceIFace()->mirrorsSelectionGroup()))
		bSelectGroup = !bToggle;
	else bSelectGroup = false;*/
	// K-Mod. Redesigned to make selection more sensible and predictable
	// In 'simple mode', shift always groups and always targets only a single unit.
	// advc.001: Option id was SimpleSelectionMode here but SimpleSelection in XML
	bool bSimpleMode = getBugOptionBOOL("MainInterface__SimpleSelection", true);

	bool bExplicitDeselect = false;
	bool bSelectGroup = false;
	if (gDLL->getInterfaceIFace()->getHeadSelectedUnit() == NULL)
		bSelectGroup = true;
	else if (bToggle)
	{
		if (pUnit->IsSelected())
		{
			bExplicitDeselect = true;
			bSelectGroup = false;
		}
		else
		{
			bSelectGroup = bSimpleMode ? false : gDLL->getInterfaceIFace()->mirrorsSelectionGroup();
		}
	}
	else
	{
		bSelectGroup = gDLL->getInterfaceIFace()->mirrorsSelectionGroup()
			? gDLL->getInterfaceIFace()->getHeadSelectedUnit()->getGroup() != pUnit->getGroup()
			: pUnit->IsSelected();
	} // K-Mod end

	gDLL->getInterfaceIFace()->clearSelectedCities();
	bool bGroup = false;
	if (bClear)
	{
		gDLL->getInterfaceIFace()->clearSelectionList();
		bGroup = false;
	}
	else
	{	//bGroup = gDLL->getInterfaceIFace()->mirrorsSelectionGroup();
		// K-Mod. If there is only one unit selected, and it is to be toggled, just degroup it rather than unselecting it.
		if (bExplicitDeselect && gDLL->getInterfaceIFace()->getLengthSelectionList() == 1)
		{
			CvMessageControl::getInstance().sendJoinGroup(pUnit->getID(), FFreeList::INVALID_INDEX);
			return; // that's all.
		}
		bGroup = gDLL->getInterfaceIFace()->mirrorsSelectionGroup();
		// Note: bGroup will not clear away unselected units of the group.
		// so if we want to do that, we'll have to do it explicitly.
		if (!bGroup && bSimpleMode && bToggle)
		{	// 'toggle' should be seen as explicitly adding / removing units from a group.
			// so lets explicitly reform the group.
			selectionListGameNetMessage(GAMEMESSAGE_JOIN_GROUP);
			// note: setting bGroup = true doesn't work here either,
			// because the internals of insertIntoSelectionList apparently wants to go out of its way to make our lives difficult.
			// (stuffed if I know what it actually does. Maybe it only sends the group signal if the units aren't already grouped or something.
			//  in any case, we have to do it explicitly or it won't work.)
			CvUnit* pSelectionHead = gDLL->getInterfaceIFace()->getHeadSelectedUnit();
			if (pSelectionHead)
				CvMessageControl::getInstance().sendJoinGroup(pUnit->getID(), pSelectionHead->getID());
		} // K-Mod end
	}
	if (bSelectGroup)
	{
		CvSelectionGroup* pSelectionGroup = pUnit->getGroup();

		gDLL->getInterfaceIFace()->selectionListPreChange();

		CLLNode<IDInfo>* pEntityNode = pSelectionGroup->headUnitNode();
		while (pEntityNode != NULL)
		{
			FAssertMsg(::getUnit(pEntityNode->m_data), "null entity in selection group");
			gDLL->getInterfaceIFace()->insertIntoSelectionList(::getUnit(pEntityNode->m_data),
					false, bToggle, bGroup, bSound, true);
			pEntityNode = pSelectionGroup->nextUnitNode(pEntityNode);
		}
		gDLL->getInterfaceIFace()->selectionListPostChange();
	}
	else
	{
		gDLL->getInterfaceIFace()->insertIntoSelectionList(pUnit, false, bToggle, bGroup, bSound);
		// K-Mod. Unfortunately, removing units from the group is not correctly handled by the interface functions.
		// so we need to do it explicitly.
		if (bExplicitDeselect && bGroup)
			CvMessageControl::getInstance().sendJoinGroup(pUnit->getID(), FFreeList::INVALID_INDEX);
		// K-Mod end
	}
	gDLL->getInterfaceIFace()->makeSelectionListDirty();
}


// K-Mod. I've made an ugly hack to change the functionality of double-click from select-all to wake-all. Here's how it works:
// if this function is called with only bAlt == true, but without the alt key actually down, then wake-all is triggered rather than select-all.
// To achieve the select-all functionality without the alt key, call the function with bCtrl && bAlt.
void CvGame::selectGroup(CvUnit* pUnit, bool bShift, bool bCtrl, bool bAlt) const
{
	PROFILE_FUNC();

	FAssertMsg(pUnit != NULL, "pUnit == NULL unexpectedly");
	// <advc.002e> Show glow (only) on selected unit
	if(!getBugOptionBOOL("PLE__ShowPromotionGlow", false)) {
		CvPlayer const& kOwner = GET_PLAYER(pUnit->getOwner());
		FOR_EACH_UNIT(u, kOwner) {
			gDLL->getEntityIFace()->showPromotionGlow(u->getUnitEntity(),
					u->atPlot(pUnit->plot()) && u->isReadyForPromotion());
		}
	} // </advc.002e>
	// K-Mod. the hack (see above)
	if (bAlt && !bShift && !bCtrl && !GC.altKey() && !gDLL->altKey()) // (using gDLL->altKey, to better match the state of bAlt)
	{
		// the caller says alt is pressed, but the computer says otherwise. Lets assume this is a double-click.
		CvPlot* pUnitPlot = pUnit->plot();
		CLLNode<IDInfo>* pUnitNode = pUnitPlot->headUnitNode();
		while (pUnitNode != NULL)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = pUnitPlot->nextUnitNode(pUnitNode);

			if (pLoopUnit->getOwner() == getActivePlayer() && pLoopUnit->isGroupHead() && pLoopUnit->isWaiting())
			{
				CvMessageControl::getInstance().sendDoCommand(pLoopUnit->getID(), COMMAND_WAKE, -1, -1, false);
			}
		}
		gDLL->getInterfaceIFace()->selectUnit(pUnit, true, false, true);
		return;
	}
	// K-Mod end

	if (bAlt || bCtrl)
	{
		gDLL->getInterfaceIFace()->clearSelectedCities();

		CvPlot* pUnitPlot = pUnit->plot();
		DomainTypes eDomain = pUnit->getDomainType(); // K-Mod
		bool bCheckMoves = pUnit->canMove() || pUnit->IsSelected(); // K-Mod.
		// (Note: the IsSelected check is to stop selected units with no moves from make it hard to select moveable units by clicking on the map.)

		bool bGroup;
		if (!bShift)
		{
			gDLL->getInterfaceIFace()->clearSelectionList();
			bGroup = true;
		}
		else
		{
			//bGroup = gDLL->getInterfaceIFace()->mirrorsSelectionGroup();
			// K-Mod. Treat shift as meaning we should always form a group
			if (!gDLL->getInterfaceIFace()->mirrorsSelectionGroup())
				selectionListGameNetMessage(GAMEMESSAGE_JOIN_GROUP);
			bGroup = true; // note: sometimes this won't work. (see comments in CvGame::selectUnit.) Unfortunately, it's too fiddly to fix.
			// K-Mod end
		}

		CLLNode<IDInfo>* pUnitNode = pUnitPlot->headUnitNode();

		gDLL->getInterfaceIFace()->selectionListPreChange();

		while (pUnitNode != NULL)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = pUnitPlot->nextUnitNode(pUnitNode);

			if (pLoopUnit->getOwner() == getActivePlayer())
			{
				if (pLoopUnit->getDomainType() == eDomain && (!bCheckMoves || pLoopUnit->canMove())) // K-Mod added domain check and bCheckMoves.
				{
					//if (!isMPOption(MPOPTION_SIMULTANEOUS_TURNS) || getTurnSlice() - pLoopUnit->getLastMoveTurn() > GC.getDefineINT("MIN_TIMER_UNIT_DOUBLE_MOVES")) // disabled by K-Mod
					{
						if (bAlt || (pLoopUnit->getUnitType() == pUnit->getUnitType()))
						{
							gDLL->getInterfaceIFace()->insertIntoSelectionList(pLoopUnit, false, false, bGroup, false, true);
						}
					}
				}
			}
		}

		gDLL->getInterfaceIFace()->selectionListPostChange();
	}
	else
	{
		gDLL->getInterfaceIFace()->selectUnit(pUnit, !bShift, bShift, true);
	}
}


void CvGame::selectAll(CvPlot* pPlot) const
{
	CvUnit* pSelectUnit;
	CvUnit* pCenterUnit;

	pSelectUnit = NULL;

	if (pPlot != NULL)
	{
		pCenterUnit = pPlot->getDebugCenterUnit();

		if ((pCenterUnit != NULL) && (pCenterUnit->getOwner() == getActivePlayer()))
		{
			pSelectUnit = pCenterUnit;
		}
	}

	if (pSelectUnit != NULL)
	{
		//gDLL->getInterfaceIFace()->selectGroup(pSelectUnit, false, false, true);
		gDLL->getInterfaceIFace()->selectGroup(pSelectUnit, false, true, true); // K-Mod
	}
}


bool CvGame::selectionListIgnoreBuildingDefense() const
{
	PROFILE_FUNC();

	bool bIgnoreBuilding = false;
	bool bAttackLandUnit = false;

	CLLNode<IDInfo>* pSelectedUnitNode = gDLL->getInterfaceIFace()->headSelectionListNode();

	while (pSelectedUnitNode != NULL)
	{
		CvUnit* pSelectedUnit = ::getUnit(pSelectedUnitNode->m_data);
		pSelectedUnitNode = gDLL->getInterfaceIFace()->nextSelectionListNode(pSelectedUnitNode);

		if (pSelectedUnit != NULL)
		{
			if (pSelectedUnit->ignoreBuildingDefense())
			{
				bIgnoreBuilding = true;
			}

			if ((pSelectedUnit->getDomainType() == DOMAIN_LAND) && pSelectedUnit->canAttack())
			{
				bAttackLandUnit = true;
			}
		}
	}

	if (!bIgnoreBuilding && !bAttackLandUnit)
	{
		if (getBestLandUnit() != NO_UNIT)
		{
			bIgnoreBuilding = GC.getUnitInfo(getBestLandUnit()).isIgnoreBuildingDefense();
		}
	}

	return bIgnoreBuilding;
}


void CvGame::implementDeal(PlayerTypes eWho, PlayerTypes eOtherWho, CLinkList<TradeData>* pOurList, CLinkList<TradeData>* pTheirList, bool bForce)
{
	// <advc.036>
	implementAndReturnDeal(eWho, eOtherWho, pOurList, pTheirList, bForce);
}

CvDeal* CvGame::implementAndReturnDeal(PlayerTypes eWho, PlayerTypes eOtherWho,
		CLinkList<TradeData>* pOurList, CLinkList<TradeData>* pTheirList,
		bool bForce) { // </advc.036>

	FAssert(eWho != NO_PLAYER);
	FAssert(eOtherWho != NO_PLAYER);
	FAssert(eWho != eOtherWho);
	// <advc.032>
	if(TEAMREF(eWho).isForcePeace(TEAMID(eOtherWho))) {
		for(CLLNode<TradeData>* pNode = pOurList->head(); pNode != NULL;
				pNode = pOurList->next(pNode)) {
			if(pNode->m_data.m_eItemType == TRADE_PEACE_TREATY) {
				if(GET_PLAYER(eWho).resetPeaceTreaty(eOtherWho))
					return NULL; // advc.036
			}
		}
	} // </advc.032>
	CvDeal* pDeal = addDeal();
	pDeal->init(pDeal->getID(), eWho, eOtherWho);
	pDeal->addTrades(pOurList, pTheirList, !bForce);
	if ((pDeal->getLengthFirstTrades() == 0) && (pDeal->getLengthSecondTrades() == 0))
	{
		pDeal->kill();
		return NULL; // advc.036
	}
	return pDeal; // advc.036
}


void CvGame::verifyDeals()
{
	CvDeal* pLoopDeal;
	int iLoop;

	for(pLoopDeal = firstDeal(&iLoop); pLoopDeal != NULL; pLoopDeal = nextDeal(&iLoop))
	{
		pLoopDeal->verify();
	}
}


/* Globeview configuration control:
If bStarsVisible, then there will be stars visible behind the globe when it is on
If bWorldIsRound, then the world will bend into a globe; otherwise, it will show up as a plane  */
void CvGame::getGlobeviewConfigurationParameters(TeamTypes eTeam, bool& bStarsVisible, bool& bWorldIsRound)
{
	if(GET_TEAM(eTeam).isMapCentering() || isCircumnavigated())
	{
		bStarsVisible = true;
		bWorldIsRound = true;
	}
	else
	{
		bStarsVisible = false;
		bWorldIsRound = false;
	}
}


int CvGame::getSymbolID(int iSymbol)
{
	return gDLL->getInterfaceIFace()->getSymbolID(iSymbol);
}


int CvGame::getAdjustedPopulationPercent(VictoryTypes eVictory) const
{
	int iPopulation;
	int iBestPopulation;
	int iNextBestPopulation;
	int iI;

	if (GC.getVictoryInfo(eVictory).getPopulationPercentLead() == 0)
	{
		return 0;
	}

	if (getTotalPopulation() == 0)
	{
		return 100;
	}

	iBestPopulation = 0;
	iNextBestPopulation = 0;

	for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			iPopulation = GET_TEAM((TeamTypes)iI).getTotalPopulation();

			if (iPopulation > iBestPopulation)
			{
				iNextBestPopulation = iBestPopulation;
				iBestPopulation = iPopulation;
			}
			else if (iPopulation > iNextBestPopulation)
			{
				iNextBestPopulation = iPopulation;
			}
		}
	}

	return std::min(100, (((iNextBestPopulation * 100) / getTotalPopulation()) + GC.getVictoryInfo(eVictory).getPopulationPercentLead()));
}


int CvGame::getProductionPerPopulation(HurryTypes eHurry) const
{
	if (NO_HURRY == eHurry)
	{
		return 0;
	}
	return (GC.getHurryInfo(eHurry).getProductionPerPopulation() * 100) / std::max(1, GC.getGameSpeedInfo(getGameSpeedType()).getHurryPercent());
}


int CvGame::getAdjustedLandPercent(VictoryTypes eVictory) const
{
	int iPercent;

	if (GC.getVictoryInfo(eVictory).getLandPercent() == 0)
	{
		return 0;
	}

	iPercent = GC.getVictoryInfo(eVictory).getLandPercent();

	iPercent -= (getCivTeamsEverAlive() * 2);

	return std::max(iPercent, GC.getVictoryInfo(eVictory).getMinLandPercent());
}

// <advc.178> Mostly cut and pasted from CvPlayerAI::AI_calculateDiplomacyVictoryStage
bool CvGame::isDiploVictoryValid() const {

	for (int iI = 0; iI < GC.getNumVictoryInfos(); iI++)
	{
		if (isVictoryValid((VictoryTypes)iI))
		{
			CvVictoryInfo& kVictoryInfo = GC.getVictoryInfo((VictoryTypes)iI);
			if (kVictoryInfo.isDiploVote())
				return true;
		}
	}
	return false;
} // </advc.178>


bool CvGame::isTeamVote(VoteTypes eVote) const
{
	return (GC.getVoteInfo(eVote).isSecretaryGeneral() || GC.getVoteInfo(eVote).isVictory());
}


bool CvGame::isChooseElection(VoteTypes eVote) const
{
	return !(GC.getVoteInfo(eVote).isSecretaryGeneral());
}


bool CvGame::isTeamVoteEligible(TeamTypes eTeam, VoteSourceTypes eVoteSource) const
{
	CvTeam& kTeam = GET_TEAM(eTeam);

	if (kTeam.isForceTeamVoteEligible(eVoteSource))
	{
		return true;
	}

	if (!kTeam.isFullMember(eVoteSource))
	{
		return false;
	}

	int iCount = 0;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
		if (kLoopTeam.isAlive())
		{
			if (kLoopTeam.isForceTeamVoteEligible(eVoteSource))
			{
				++iCount;
			}
		}
	}

	int iExtraEligible = GC.getDefineINT("TEAM_VOTE_MIN_CANDIDATES") - iCount;
	if (iExtraEligible <= 0)
	{
		return false;
	}

	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (iI != eTeam)
		{
			CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
			if (kLoopTeam.isAlive())
			{
				if (!kLoopTeam.isForceTeamVoteEligible(eVoteSource))
				{
					if (kLoopTeam.isFullMember(eVoteSource))
					{
						int iLoopVotes = kLoopTeam.getVotes(NO_VOTE, eVoteSource);
						int iVotes = kTeam.getVotes(NO_VOTE, eVoteSource);
						// <advc.014>
						if(!kTeam.isCapitulated() || !kLoopTeam.isCapitulated()) {
							if(kTeam.isCapitulated())
								iVotes = 0;
							if(kLoopTeam.isCapitulated())
								iLoopVotes = 0;
						} // </advc.014>
						if (iLoopVotes > iVotes || (iLoopVotes == iVotes && iI < eTeam))
						{
							iExtraEligible--;
						}
					}
				}
			}
		}
	}

	return (iExtraEligible > 0);
}


int CvGame::countVote(const VoteTriggeredData& kData, PlayerVoteTypes eChoice) const
{
	int iCount = 0;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; ++iI)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (getPlayerVote(((PlayerTypes)iI), kData.getID()) == eChoice)
			{
				iCount += GET_PLAYER((PlayerTypes)iI).getVotes(kData.kVoteOption.eVote, kData.eVoteSource);
			}
		}
	}

	return iCount;
}


int CvGame::countPossibleVote(VoteTypes eVote, VoteSourceTypes eVoteSource) const
{
	int iCount;
	int iI;

	iCount = 0;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		iCount += GET_PLAYER((PlayerTypes)iI).getVotes(eVote, eVoteSource);
	}

	return iCount;
}



TeamTypes CvGame::findHighestVoteTeam(const VoteTriggeredData& kData) const
{
	TeamTypes eBestTeam = NO_TEAM;

	if (isTeamVote(kData.kVoteOption.eVote))
	{
		int iBestCount = 0;
		for (int iI = 0; iI < MAX_CIV_TEAMS; ++iI)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				int iCount = countVote(kData, (PlayerVoteTypes)iI);

				if (iCount > iBestCount)
				{
					iBestCount = iCount;
					eBestTeam = (TeamTypes)iI;
				}
			}
		}
	}

	return eBestTeam;
}


int CvGame::getVoteRequired(VoteTypes eVote, VoteSourceTypes eVoteSource) const
{
	return ((countPossibleVote(eVote, eVoteSource) * GC.getVoteInfo(eVote).getPopulationThreshold()) / 100);
}


TeamTypes CvGame::getSecretaryGeneral(VoteSourceTypes eVoteSource) const
{
	int iI;

	if (!canHaveSecretaryGeneral(eVoteSource))
	{
		for (int iBuilding = 0; iBuilding < GC.getNumBuildingInfos(); ++iBuilding)
		{
			if (GC.getBuildingInfo((BuildingTypes)iBuilding).getVoteSourceType() == eVoteSource)
			{
				for (iI = 0; iI < MAX_CIV_PLAYERS; ++iI)
				{
					CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
					if (kLoopPlayer.isAlive())
					{
						if (kLoopPlayer.getBuildingClassCount((BuildingClassTypes)GC.getBuildingInfo((BuildingTypes)iBuilding).getBuildingClassType()) > 0)
						{
							ReligionTypes eReligion = getVoteSourceReligion(eVoteSource);
							if (NO_RELIGION == eReligion || kLoopPlayer.getStateReligion() == eReligion)
							{
								return kLoopPlayer.getTeam();
							}
						}
					}
				}
			}
		}
	}
	else
	{
		for (iI = 0; iI < GC.getNumVoteInfos(); iI++)
		{
			if (GC.getVoteInfo((VoteTypes)iI).isVoteSourceType(eVoteSource))
			{
				if (GC.getVoteInfo((VoteTypes)iI).isSecretaryGeneral())
				{
					if (isVotePassed((VoteTypes)iI))
					{
						return ((TeamTypes)(getVoteOutcome((VoteTypes)iI)));
					}
				}
			}
		}
	}


	return NO_TEAM;
}

bool CvGame::canHaveSecretaryGeneral(VoteSourceTypes eVoteSource) const
{
	for (int iI = 0; iI < GC.getNumVoteInfos(); iI++)
	{
		if (GC.getVoteInfo((VoteTypes)iI).isVoteSourceType(eVoteSource))
		{
			if (GC.getVoteInfo((VoteTypes)iI).isSecretaryGeneral())
			{
				return true;
			}
		}
	}

	return false;
}

void CvGame::clearSecretaryGeneral(VoteSourceTypes eVoteSource)
{
	for (int j = 0; j < GC.getNumVoteInfos(); ++j)
	{
		CvVoteInfo& kVote = GC.getVoteInfo((VoteTypes)j);

		if (kVote.isVoteSourceType(eVoteSource))
		{
			if (kVote.isSecretaryGeneral())
			{
				VoteTriggeredData kData;
				kData.eVoteSource = eVoteSource;
				kData.kVoteOption.eVote = (VoteTypes)j;
				kData.kVoteOption.iCityId = -1;
				kData.kVoteOption.szText.clear(); // kmodx
				kData.kVoteOption.ePlayer = NO_PLAYER;
				setVoteOutcome(kData, NO_PLAYER_VOTE);
				setSecretaryGeneralTimer(eVoteSource, 0);
			}
		}
	}
}

void CvGame::updateSecretaryGeneral()
{
	for (int i = 0; i < GC.getNumVoteSourceInfos(); ++i)
	{
		TeamTypes eSecretaryGeneral = getSecretaryGeneral((VoteSourceTypes)i);
		if (NO_TEAM != eSecretaryGeneral && (!GET_TEAM(eSecretaryGeneral).isFullMember((VoteSourceTypes)i)
				|| GET_TEAM(eSecretaryGeneral).isCapitulated())) // advc.014
			clearSecretaryGeneral((VoteSourceTypes)i);
	}
}

int CvGame::countCivPlayersAlive() const
{
	int iCount = 0;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			iCount++;
		}
	}

	return iCount;
}


int CvGame::countCivPlayersEverAlive() const
{	// advc.003b:
	FAssertMsg(!m_bAllGameDataRead, "Should use getCivPlayersEverAlive instead");
	int iCount = 0;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iI);
		if (kPlayer.isEverAlive())
		{
			if (kPlayer.getParent() == NO_PLAYER)
			{
				iCount++;
			}
		}
	}

	return iCount;
}


int CvGame::countCivTeamsAlive() const
{
	int iCount = 0;

	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			iCount++;
		}
	}

	return iCount;
}


int CvGame::countCivTeamsEverAlive() const
{	// advc.003b:
	FAssertMsg(!m_bAllGameDataRead, "Should use getCivTeamsEverAlive instead");
	std::set<int> setTeamsEverAlive;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iI);
		if (kPlayer.isEverAlive())
		{
			if (kPlayer.getParent() == NO_PLAYER)
			{
				setTeamsEverAlive.insert(kPlayer.getTeam());
			}
		}
	}

	return setTeamsEverAlive.size();
}


int CvGame::countHumanPlayersAlive() const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iI);
		if (kPlayer.isAlive() && (kPlayer.isHuman() ||
				/*  advc.127: To prevent CvGame::update from concluding that the
					game is over when human is still disabled at the start of a round. */
				kPlayer.isHumanDisabled()))
			iCount++;
	}
	return iCount;
}

// K-Mod
int CvGame::countFreeTeamsAlive() const
{
	int iCount = 0;
	for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
	{
		const CvTeam& kLoopTeam = GET_TEAM(i);
		//if (kLoopTeam.isAlive() && !kLoopTeam.isCapitulated())
		if (kLoopTeam.isAlive() && !kLoopTeam.isAVassal()) // I'm in two minds about which of these to use here.
			iCount++;
	}
	return iCount;
}
// K-Mod end


// <advc.137>
int CvGame::getRecommendedPlayers() const {

	CvWorldInfo const& kWorld = GC.getWorldInfo(GC.getMap().getWorldSize());
	return ::range(((-4 * getSeaLevelChange() + 100) * kWorld.getDefaultPlayers()) / 100,
			2, MAX_CIV_PLAYERS);
}

// <advc.140>
int CvGame::getSeaLevelChange() const {

	int r = 0;
	SeaLevelTypes eSeaLevel = GC.getInitCore().getSeaLevel();
	if(eSeaLevel != NO_SEALEVEL)
		r = GC.getSeaLevelInfo(eSeaLevel).getSeaLevelChange();
	return r;
}
// </advc.140> </advc.137>


int CvGame::countTotalCivPower()
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iI);
		if (kPlayer.isAlive())
			iCount += kPlayer.getPower();
	}
	return iCount;
}


int CvGame::countTotalNukeUnits()
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kPlayer.isAlive())
			iCount += kPlayer.getNumNukeUnits();
	}
	return iCount;
}


int CvGame::countKnownTechNumTeams(TechTypes eTech)
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isEverAlive())
		{
			if (GET_TEAM((TeamTypes)iI).isHasTech(eTech))
				iCount++;
		}
	}
	return iCount;
}


int CvGame::getNumFreeBonuses(BuildingTypes eBuilding) const
{
	if (GC.getBuildingInfo(eBuilding).getNumFreeBonuses() == -1)
		return GC.getWorldInfo(GC.getMap().getWorldSize()).getNumFreeBuildingBonuses();
	else return GC.getBuildingInfo(eBuilding).getNumFreeBonuses();
}


int CvGame::countReligionLevels(ReligionTypes eReligion) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
			iCount += GET_PLAYER((PlayerTypes)iI).getHasReligionCount(eReligion);
	}
	return iCount;
}

int CvGame::countCorporationLevels(CorporationTypes eCorporation) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.isAlive())
			iCount += GET_PLAYER((PlayerTypes)iI).getHasCorporationCount(eCorporation);
	}
	return iCount;
}

void CvGame::replaceCorporation(CorporationTypes eCorporation1, CorporationTypes eCorporation2)
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.isAlive())
		{
			FOR_EACH_CITY(pCity, kLoopPlayer)
			{
				if (pCity->isHasCorporation(eCorporation1))
				{
					pCity->setHasCorporation(eCorporation1, false, false, false);
					pCity->setHasCorporation(eCorporation2, true, true);
				}
			}

			FOR_EACH_UNIT(pUnit, kLoopPlayer)
			{
				if (pUnit->getUnitInfo().getCorporationSpreads(eCorporation1) > 0)
				{
					pUnit->kill(false);
				}
			}
		}
	}
}


int CvGame::calculateReligionPercent(ReligionTypes eReligion,
		bool bIgnoreOtherReligions) const // advc.115b: Param added  // advc.003: style changes
{
	if (getTotalPopulation() == 0)
		return 0;

	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer const& kPlayer = GET_PLAYER((PlayerTypes)iI);
		if (!kPlayer.isAlive())
			continue;

		FOR_EACH_CITY(pLoopCity, kPlayer)
		{
			if (pLoopCity->isHasReligion(eReligion))
			{	// <advc.115b>
				if(bIgnoreOtherReligions)
					iCount += pLoopCity->getPopulation();
				else // </advc.115b>
				{
					iCount += (pLoopCity->getPopulation() +
							pLoopCity->getReligionCount() / 2) /
							pLoopCity->getReligionCount();
				}
			}
		}
	}
	return (iCount * 100) / getTotalPopulation();
}


int CvGame::goldenAgeLength() const
{
	int iLength = GC.getDefineINT("GOLDEN_AGE_LENGTH");

	iLength *= GC.getGameSpeedInfo(getGameSpeedType()).getGoldenAgePercent();
	iLength /= 100;

	return iLength;
}

int CvGame::victoryDelay(VictoryTypes eVictory) const
{
	FAssert(eVictory >= 0 && eVictory < GC.getNumVictoryInfos());

	int iLength = GC.getVictoryInfo(eVictory).getVictoryDelayTurns();

	iLength *= GC.getGameSpeedInfo(getGameSpeedType()).getVictoryDelayPercent();
	iLength /= 100;

	return iLength;
}



int CvGame::getImprovementUpgradeTime(ImprovementTypes eImprovement) const
{
	int iTime;

	iTime = GC.getImprovementInfo(eImprovement).getUpgradeTime();

	iTime *= GC.getGameSpeedInfo(getGameSpeedType()).getImprovementPercent();
	iTime /= 100;

	iTime *= GC.getEraInfo(getStartEra()).getImprovementPercent();
	iTime /= 100;

	return iTime;
}

/*  advc.003: 3 for Marathon, 0.67 for Quick. Based on VictoryDelay. For cases where
	there isn't a more specific game speed modifier that could be applied. (E.g.
	tech costs should be adjusted based on iResearchPercent, not on this function.) */
double CvGame::gameSpeedFactor() const {

	return GC.getGameSpeedInfo(getGameSpeedType()).getVictoryDelayPercent() / 100.0;
} // </advc.003>

bool CvGame::canTrainNukes() const
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kPlayer.isAlive())
		{
			for (int iJ = 0; iJ < GC.getNumUnitClassInfos(); iJ++)
			{
				UnitTypes eUnit = (UnitTypes)GC.getCivilizationInfo(kPlayer.getCivilizationType()).getCivilizationUnits((UnitClassTypes)iJ);

				if (NO_UNIT != eUnit)
				{
					if (-1 != GC.getUnitInfo(eUnit).getNukeRange())
					{
						if (kPlayer.canTrain(eUnit))
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}


EraTypes CvGame::getCurrentEra() const
{
	//PROFILE_FUNC(); // advc.003b: OK - negligble

	int iEra = 0;
	int iCount = 0;

	//for (iI = 0; iI < MAX_PLAYERS; iI++)
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++) // K-Mod (don't count the barbarians)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			iEra += GET_PLAYER((PlayerTypes)iI).getCurrentEra();
			iCount++;
		}
	}

	if (iCount > 0)
	{
		//return ((EraTypes)(iEra / iCount));
		return (EraTypes)::round(iEra / (double)iCount); // dlph.17
	}

	return NO_ERA;
}


TeamTypes CvGame::getActiveTeam() const
{
	if (getActivePlayer() == NO_PLAYER)
	{
		return NO_TEAM;
	}
	else
	{
		return (TeamTypes)GET_PLAYER(getActivePlayer()).getTeam();
	}
}


CivilizationTypes CvGame::getActiveCivilizationType() const
{
	if (getActivePlayer() == NO_PLAYER)
	{
		return NO_CIVILIZATION;
	}
	else
	{
		return (CivilizationTypes)GET_PLAYER(getActivePlayer()).getCivilizationType();
	}
}


bool CvGame::isNetworkMultiPlayer() const
{
	return GC.getInitCore().getMultiplayer();
}


bool CvGame::isGameMultiPlayer() const
{	// <advc.135c>
	if(m_bFeignSP)
		return false; // </advc.135c>
	return (isNetworkMultiPlayer() || isPbem() || isHotSeat());
}


bool CvGame::isTeamGame() const
{
	FAssert(countCivPlayersAlive() >= countCivTeamsAlive());
	return (countCivPlayersAlive() > countCivTeamsAlive());
}


bool CvGame::isModem() /* advc.003: */ const
{
	return gDLL->IsModem();
}
void CvGame::setModem(bool bModem)
{
	if (bModem)
	{
		gDLL->ChangeINIKeyValue("CONFIG", "Bandwidth", "modem");
	}
	else
	{
		gDLL->ChangeINIKeyValue("CONFIG", "Bandwidth", "broadband");
	}

	gDLL->SetModem(bModem);
}


void CvGame::reviveActivePlayer()
{
	if (!GET_PLAYER(getActivePlayer()).isAlive())
	{
		setAIAutoPlay(0, /* advc.127: */ false);

		GC.getInitCore().setSlotStatus(getActivePlayer(), SS_TAKEN);

		long lResult=0;
		CyArgsList argsList;
		argsList.add(getActivePlayer());
		gDLL->getPythonIFace()->callFunction(PYGameModule, "doReviveActivePlayer", argsList.makeFunctionArgs(), &lResult);
		if (lResult == 1)
			return;

		GET_PLAYER(getActivePlayer()).initUnit((UnitTypes)0, 0, 0);
	}
}


int CvGame::getNumHumanPlayers()
{
	return GC.getInitCore().getNumHumans();
}


int CvGame::getGameTurn() const {

	return GC.getInitCore().getGameTurn();
}


void CvGame::setGameTurn(int iNewValue)
{
	if (getGameTurn() != iNewValue)
	{
		GC.getInitCore().setGameTurn(iNewValue);
		FAssert(getGameTurn() >= 0);

		updateBuildingCommerce();

		setScoreDirty(true);

		gDLL->getInterfaceIFace()->setDirty(TurnTimer_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
	}
}


void CvGame::incrementGameTurn()
{
	setGameTurn(getGameTurn() + 1);
}


int CvGame::getTurnYear(int iGameTurn) /* advc.003: */ const
{
	// moved the body of this method to Game Core Utils so we have access for other games than the current one (replay screen in HOF)
	return getTurnYearForGame(iGameTurn, getStartYear(), getCalendar(), getGameSpeedType());
}


int CvGame::getGameTurnYear() const // advc.003: const
{
	//return getTurnYear(getGameTurn()); // To work aorund non-const getGameTurn
	return getTurnYear(getGameTurn());
}


int CvGame::getElapsedGameTurns() const
{
	return m_iElapsedGameTurns;
}


void CvGame::incrementElapsedGameTurns()
{
	m_iElapsedGameTurns++;
}

// <advc.251>
int CvGame::AIHandicapAdjustment() const {

	int iGameTurn = getGameTurn();
	int iVictoryDelayPercent = GC.getGameSpeedInfo(getGameSpeedType()).getVictoryDelayPercent();
	if(iVictoryDelayPercent > 0)
		iGameTurn = (iGameTurn * 100) / iVictoryDelayPercent;
	int iIncrementTurns = GC.getHandicapInfo(getHandicapType()).getAIHandicapIncrementTurns();
	if(iIncrementTurns == 0)
		return 0;
	/*  Flip sign b/c we're dealing with cost modifiers that are supposed to decrease.
		Only if a negative AIHandicapIncrement is set in XML, the modifiers are
		supposed to increase. */
	return -iGameTurn / iIncrementTurns;
} // </advc.251>


int CvGame::getMaxTurns() const
{
	return GC.getInitCore().getMaxTurns();
}


void CvGame::setMaxTurns(int iNewValue)
{
	GC.getInitCore().setMaxTurns(iNewValue);
	FAssert(getMaxTurns() >= 0);
}


void CvGame::changeMaxTurns(int iChange)
{
	setMaxTurns(getMaxTurns() + iChange);
}


int CvGame::getMaxCityElimination() const
{
	return GC.getInitCore().getMaxCityElimination();
}


void CvGame::setMaxCityElimination(int iNewValue)
{
	GC.getInitCore().setMaxCityElimination(iNewValue);
	FAssert(getMaxCityElimination() >= 0);
}

int CvGame::getNumAdvancedStartPoints() const
{
	return GC.getInitCore().getNumAdvancedStartPoints();
}


void CvGame::setNumAdvancedStartPoints(int iNewValue)
{
	GC.getInitCore().setNumAdvancedStartPoints(iNewValue);
	FAssert(getNumAdvancedStartPoints() >= 0);
}

int CvGame::getStartTurn() const
{
	return m_iStartTurn;
}


void CvGame::setStartTurn(int iNewValue)
{
	m_iStartTurn = iNewValue;
}


int CvGame::getStartYear() const
{
	return m_iStartYear;
}


void CvGame::setStartYear(int iNewValue)
{
	m_iStartYear = iNewValue;
}


int CvGame::getEstimateEndTurn() const
{
	return m_iEstimateEndTurn;
}


void CvGame::setEstimateEndTurn(int iNewValue)
{
	m_iEstimateEndTurn = iNewValue;
}

/*  <advc.003> Ratio of turns played to total estimated game length; between 0 and 1.
	iDelay is added to the number of turns played. */
double CvGame::gameTurnProgress(int iDelay) const {

	/*  Even with time victory disabled, we shouldn't expect the game to last
		beyond 2050. So, no need to check if it's disabled. */
	double gameLength = getEstimateEndTurn() - getStartTurn();
	return std::min(1.0, (getElapsedGameTurns() + iDelay) / gameLength);
} // </advc.003>

int CvGame::getTurnSlice() const
{
	return m_iTurnSlice;
}


int CvGame::getMinutesPlayed() const
{
	return (getTurnSlice() / gDLL->getTurnsPerMinute());
}


void CvGame::setTurnSlice(int iNewValue)
{
	m_iTurnSlice = iNewValue;
}


void CvGame::changeTurnSlice(int iChange)
{
	setTurnSlice(getTurnSlice() + iChange);
}


int CvGame::getCutoffSlice() const
{
	return m_iCutoffSlice;
}


void CvGame::setCutoffSlice(int iNewValue)
{
	m_iCutoffSlice = iNewValue;
}


void CvGame::changeCutoffSlice(int iChange)
{
	setCutoffSlice(getCutoffSlice() + iChange);
}


void CvGame::resetTurnTimer()
{
	// We should only use the turn timer if we are in multiplayer
	if (isMPOption(MPOPTION_TURN_TIMER))
	{
		if (getElapsedGameTurns() > 0 || !isOption(GAMEOPTION_ADVANCED_START))
		{
			// Determine how much time we should allow
			int iTurnLen = getMaxTurnLen();
			if (getElapsedGameTurns() == 0 && !isPitboss())
			{
				// Let's allow more time for the initial turn
				TurnTimerTypes eTurnTimer = GC.getInitCore().getTurnTimer();
				FAssertMsg(eTurnTimer >= 0 && eTurnTimer < GC.getNumTurnTimerInfos(), "Invalid TurnTimer selection in InitCore");
				iTurnLen = (iTurnLen * GC.getTurnTimerInfo(eTurnTimer).getFirstTurnMultiplier());
			}
			// Set the current turn slice to start the 'timer'
			setCutoffSlice(getTurnSlice() + iTurnLen);
		}
	}
}

void CvGame::incrementTurnTimer(int iNumTurnSlices)
{
	if (isMPOption(MPOPTION_TURN_TIMER))
	{
		// If the turn timer has expired, we shouldn't increment it as we've sent our turn complete message
		if (getTurnSlice() <= getCutoffSlice())
		{
			changeCutoffSlice(iNumTurnSlices);
		}
	}
}


int CvGame::getMaxTurnLen()
{
	if (isPitboss())
	{
		// Use the user provided input
		// Turn time is in hours
		return (getPitbossTurnTime() * 3600 * 4);
	}
	else
	{
		int iMaxUnits = 0;
		int iMaxCities = 0;

		// Find out who has the most units and who has the most cities
		// Calculate the max turn time based on the max number of units and cities
		for (int i = 0; i < MAX_CIV_PLAYERS; ++i)
		{
			if (GET_PLAYER((PlayerTypes)i).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)i).getNumUnits() > iMaxUnits)
				{
					iMaxUnits = GET_PLAYER((PlayerTypes)i).getNumUnits();
				}
				if (GET_PLAYER((PlayerTypes)i).getNumCities() > iMaxCities)
				{
					iMaxCities = GET_PLAYER((PlayerTypes)i).getNumCities();
				}
			}
		}

		// Now return turn len based on base len and unit and city bonuses
		TurnTimerTypes eTurnTimer = GC.getInitCore().getTurnTimer();
		FAssertMsg(eTurnTimer >= 0 && eTurnTimer < GC.getNumTurnTimerInfos(), "Invalid TurnTimer Selection in InitCore");
		return (GC.getTurnTimerInfo(eTurnTimer).getBaseTime() +
				(GC.getTurnTimerInfo(eTurnTimer).getCityBonus()*iMaxCities) +
				(GC.getTurnTimerInfo(eTurnTimer).getUnitBonus()*iMaxUnits));
	}
}


int CvGame::getTargetScore() const
{
	return GC.getInitCore().getTargetScore();
}


void CvGame::setTargetScore(int iNewValue)
{
	GC.getInitCore().setTargetScore(iNewValue);
	FAssert(getTargetScore() >= 0);
}


int CvGame::getNumGameTurnActive()
{
	return m_iNumGameTurnActive;
}


int CvGame::countNumHumanGameTurnActive() const
{
	int iCount;
	int iI;

	iCount = 0;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isHuman())
		{
			if (GET_PLAYER((PlayerTypes)iI).isTurnActive())
			{
				iCount++;
			}
		}
	}

	return iCount;
}


void CvGame::changeNumGameTurnActive(int iChange)
{
	m_iNumGameTurnActive = (m_iNumGameTurnActive + iChange);
	FAssert(getNumGameTurnActive() >= 0);
}


int CvGame::getNumCities() const
{
	return m_iNumCities;
}


int CvGame::getNumCivCities() const
{
	return (getNumCities() - GET_PLAYER(BARBARIAN_PLAYER).getNumCities());
}


void CvGame::changeNumCities(int iChange)
{
	m_iNumCities = (m_iNumCities + iChange);
	FAssert(getNumCities() >= 0);
}


int CvGame::getTotalPopulation() const
{
	return m_iTotalPopulation;
}


void CvGame::changeTotalPopulation(int iChange)
{
	m_iTotalPopulation = (m_iTotalPopulation + iChange);
	FAssert(getTotalPopulation() >= 0);
}


int CvGame::getTradeRoutes() const
{
	return m_iTradeRoutes;
}


void CvGame::changeTradeRoutes(int iChange)
{
	if (iChange != 0)
	{
		m_iTradeRoutes = (m_iTradeRoutes + iChange);
		FAssert(getTradeRoutes() >= 0);

		updateTradeRoutes();
	}
}


int CvGame::getFreeTradeCount() const
{
	return m_iFreeTradeCount;
}


bool CvGame::isFreeTrade() const
{
	return (getFreeTradeCount() > 0);
}


void CvGame::changeFreeTradeCount(int iChange)
{
	if(iChange == 0)
		return;

	bool bOldFreeTrade = isFreeTrade();

	m_iFreeTradeCount = (m_iFreeTradeCount + iChange);
	FAssert(getFreeTradeCount() >= 0);

	if (bOldFreeTrade != isFreeTrade())
	{
		updateTradeRoutes();
	}
}


int CvGame::getNoNukesCount() const
{
	return m_iNoNukesCount;
}


bool CvGame::isNoNukes() const
{
	return (getNoNukesCount() > 0);
}


void CvGame::changeNoNukesCount(int iChange)
{
	m_iNoNukesCount = (m_iNoNukesCount + iChange);
	FAssert(getNoNukesCount() >= 0);
}


int CvGame::getSecretaryGeneralTimer(VoteSourceTypes eVoteSource) const
{
	FAssert(eVoteSource >= 0);
	FAssert(eVoteSource < GC.getNumVoteSourceInfos());
	return m_aiSecretaryGeneralTimer[eVoteSource];
}


void CvGame::setSecretaryGeneralTimer(VoteSourceTypes eVoteSource, int iNewValue)
{
	FAssert(eVoteSource >= 0);
	FAssert(eVoteSource < GC.getNumVoteSourceInfos());
	m_aiSecretaryGeneralTimer[eVoteSource] = iNewValue;
	FAssert(getSecretaryGeneralTimer(eVoteSource) >= 0);
}


void CvGame::changeSecretaryGeneralTimer(VoteSourceTypes eVoteSource, int iChange)
{
	setSecretaryGeneralTimer(eVoteSource, getSecretaryGeneralTimer(eVoteSource) + iChange);
}


int CvGame::getVoteTimer(VoteSourceTypes eVoteSource) const
{
	FAssert(eVoteSource >= 0);
	FAssert(eVoteSource < GC.getNumVoteSourceInfos());
	return m_aiVoteTimer[eVoteSource];
}


void CvGame::setVoteTimer(VoteSourceTypes eVoteSource, int iNewValue)
{
	FAssert(eVoteSource >= 0);
	FAssert(eVoteSource < GC.getNumVoteSourceInfos());
	m_aiVoteTimer[eVoteSource] = iNewValue;
	FAssert(getVoteTimer(eVoteSource) >= 0);
}


void CvGame::changeVoteTimer(VoteSourceTypes eVoteSource, int iChange)
{
	setVoteTimer(eVoteSource, getVoteTimer(eVoteSource) + iChange);
}


int CvGame::getNukesExploded() const
{
	return m_iNukesExploded;
}


void CvGame::changeNukesExploded(int iChange)
{
	m_iNukesExploded = (m_iNukesExploded + iChange);
}


int CvGame::getMaxPopulation() const
{
	return m_iMaxPopulation;
}


int CvGame::getMaxLand() const
{
	return m_iMaxLand;
}


int CvGame::getMaxTech() const
{
	return m_iMaxTech;
}


int CvGame::getMaxWonders() const
{
	return m_iMaxWonders;
}


int CvGame::getInitPopulation() const
{
	return m_iInitPopulation;
}


int CvGame::getInitLand() const
{
	return m_iInitLand;
}


int CvGame::getInitTech() const
{
	return m_iInitTech;
}


int CvGame::getInitWonders() const
{
	return m_iInitWonders;
}


void CvGame::initScoreCalculation()
{
	// initialize score calculation
	int iMaxFood = 0;
	for (int i = 0; i < GC.getMap().numPlots(); i++)
	{
		CvPlot* pPlot = GC.getMap().plotByIndex(i);
		if (!pPlot->isWater() || pPlot->isAdjacentToLand())
		{
			iMaxFood += pPlot->calculateBestNatureYield(YIELD_FOOD, NO_TEAM);
		}
	}
	m_iMaxPopulation = getPopulationScore(iMaxFood / std::max(1, GC.getFOOD_CONSUMPTION_PER_POPULATION()));
	m_iMaxLand = getLandPlotsScore(GC.getMap().getLandPlots());
	m_iMaxTech = 0;
	for (int i = 0; i < GC.getNumTechInfos(); i++)
	{
		m_iMaxTech += getTechScore((TechTypes)i);
	}
	m_iMaxWonders = 0;
	for (int i = 0; i < GC.getNumBuildingClassInfos(); i++)
	{
		m_iMaxWonders += getWonderScore((BuildingClassTypes)i);
	}

	if (NO_ERA != getStartEra())
	{
		int iNumSettlers = GC.getEraInfo(getStartEra()).getStartingUnitMultiplier();
		m_iInitPopulation = getPopulationScore(iNumSettlers * (GC.getEraInfo(getStartEra()).getFreePopulation() + 1));
		m_iInitLand = getLandPlotsScore(iNumSettlers *  NUM_CITY_PLOTS);
	}
	else
	{
		m_iInitPopulation = 0;
		m_iInitLand = 0;
	}

	m_iInitTech = 0;
	for (int i = 0; i < GC.getNumTechInfos(); i++)
	{
		if (GC.getTechInfo((TechTypes)i).getEra() < getStartEra())
		{
			m_iInitTech += getTechScore((TechTypes)i);
		}
		else
		{
			// count all possible free techs as initial to lower the score from immediate retirement
			for (int iCiv = 0; iCiv < GC.getNumCivilizationInfos(); iCiv++)
			{
				if (GC.getCivilizationInfo((CivilizationTypes)iCiv).isPlayable())
				{
					if (GC.getCivilizationInfo((CivilizationTypes)iCiv).isCivilizationFreeTechs(i))
					{
						m_iInitTech += getTechScore((TechTypes)i);
						break;
					}
				}
			}
		}
	}
	m_iInitWonders = 0;
}


int CvGame::getAIAutoPlay() /* advc.003: */ const
{
	return m_iAIAutoPlay;
}


void CvGame::setAIAutoPlay(int iNewValue, /* <advc.127> */ bool bChangePlayerStatus)
{
	m_iAIAutoPlay = std::max(0, iNewValue);
	if(!bChangePlayerStatus)
		return; // </advc.127>
	// Erik <BM1>
	if (m_iAIAutoPlay == 0)
	{
		// Required by the benchmark to be informed when autoplay has completed
		CyArgsList pyArgs;
		pyArgs.add(getTurnSlice());
		CvEventReporter::getInstance().genericEvent("AutoPlayComplete", pyArgs.makeFunctionArgs());
	} // Erik </BM1>

	/*  AI_AUTO_PLAY_MOD, 07/09/08, jdog5000: START
		(Multiplayer compatibility idea from Jeckel) */
	// <advc.127> To make sure I'm not breaking anything in singleplayer
	if(!isGameMultiPlayer())
	{
		GET_PLAYER(getActivePlayer()).setHumanDisabled((getAIAutoPlay() != 0));
		return;
	} // </advc.127>
	for(int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if(kLoopPlayer.isHuman() || kLoopPlayer.isHumanDisabled())
		{	/*  advc.127: Was GET_PLAYER(getActivePlayer()).
				Tagging advc.001 because that was probably a bug. */
			kLoopPlayer.setHumanDisabled((getAIAutoPlay() != 0));
		}
	} // AI_AUTO_PLAY_MOD: END
}


void CvGame::changeAIAutoPlay(int iChange, /* advc.127: */ bool changePlayerStatus)
{
	setAIAutoPlay(getAIAutoPlay() + iChange, /* advc.127: */ changePlayerStatus);
}

// <advc.003b>
int CvGame::getCivPlayersEverAlive() const {

	// Could pose a savegame compatibility problem (uiFlag<4)
	FAssert(m_bAllGameDataRead); // But it's OK if uiFlag < 9 in CvPlayerAI
	if(!m_bAllGameDataRead)
		return countCivPlayersEverAlive();
	return m_iCivPlayersEverAlive;
}

void CvGame::changeCivPlayersEverAlive(int iChange) {

	m_iCivPlayersEverAlive += iChange;
	//FAssert(iChange >= 0);
	/*  iChange normally shouldn't be negative, but liberated civs aren't supposed
		to count, and they're set to 'alive' before getting marked as liberated
		(CvPlayer::setParent), so they need to be subtracted once setParent is called. */
	FASSERT_BOUNDS(0, MAX_CIV_PLAYERS + 1, m_iCivPlayersEverAlive, "CvGame::changeCivPlayersEverAlive");
}

int CvGame::getCivTeamsEverAlive() const {

	// Could pose a savegame compatibility problem (uiFlag<4)
	FAssert(m_bAllGameDataRead); // But it's OK if uiFlag < 9 in CvPlayerAI
	if(!m_bAllGameDataRead)
		return countCivTeamsEverAlive();
	return m_iCivTeamsEverAlive;
}

void CvGame::changeCivTeamsEverAlive(int iChange) {

	m_iCivTeamsEverAlive += iChange;
	FASSERT_BOUNDS(0, MAX_CIV_TEAMS + 1, m_iCivTeamsEverAlive, "CvGame::changeCivTeamsEverAlive");
} // </advc.003b>

/*  K-mod, 6/dec/10, karadoc
	18/dec/10 - added Gw calc functions */
int CvGame::getGlobalWarmingIndex() const
{
	return m_iGlobalWarmingIndex;
}

void CvGame::setGlobalWarmingIndex(int iNewValue)
{
	m_iGlobalWarmingIndex = std::max(0, iNewValue);
}

void CvGame::changeGlobalWarmingIndex(int iChange)
{
	setGlobalWarmingIndex(getGlobalWarmingIndex() + iChange);
}

int CvGame::getGlobalWarmingChances() const
{
	// Note: this is the number of chances global warming has to strike in the current turn
	// as you can see, I've scaled it by the number of turns in the game. The probability per chance is also scaled like this.
	// I estimate that the global warming index will actually be roughly proportional to the number of turns in the game
	// so by scaling the chances, and the probability per chance, I hope to get roughly the same number of actually events per game
	int iIndexPerChance = GC.getDefineINT("GLOBAL_WARMING_INDEX_PER_CHANCE");
	iIndexPerChance*=GC.getGameSpeedInfo(getGameSpeedType()).getVictoryDelayPercent();
	iIndexPerChance/=100;
	return ROUND_DIVIDE(getGlobalWarmingIndex(), std::max(1, iIndexPerChance));
}

int CvGame::getGwEventTally() const
{
	return m_iGwEventTally;
}

void CvGame::setGwEventTally(int iNewValue)
{
	m_iGwEventTally = iNewValue;
}

void CvGame::changeGwEventTally(int iChange)
{
	setGwEventTally(getGwEventTally() + iChange);
}

// worldwide pollution
int CvGame::calculateGlobalPollution() const
{
	int iGlobalPollution = 0;
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iPlayer);
		if (kPlayer.isAlive())
		{
			iGlobalPollution += kPlayer.calculatePollution();
		}
	}
	return iGlobalPollution;
}

// if ePlayer == NO_PLAYER, all features are counted. Otherwise, only count features owned by the specified player.
int CvGame::calculateGwLandDefence(PlayerTypes ePlayer) const
{
	int iTotal = 0;

	for (int i = 0; i < GC.getMap().numPlots(); ++i)
	{
		CvPlot* pPlot = GC.getMap().plotByIndex(i);

		if (pPlot->getFeatureType() != NO_FEATURE)
		{
			if (ePlayer == NO_PLAYER || ePlayer == pPlot->getOwner())
			{
				iTotal += GC.getFeatureInfo(pPlot->getFeatureType()).getWarmingDefense();
			}
		}
	}
	return iTotal;
}

// again, NO_PLAYER means everyone
int CvGame::calculateGwSustainabilityThreshold(PlayerTypes ePlayer) const
{
	// expect each pop to give ~10 pollution per turn at the time we cross the threshold, and ~1 pop per land tile...
	// so default resistance should be around 10 per tile.
	int iGlobalThreshold = GC.getMap().getLandPlots() * GC.getDefineINT("GLOBAL_WARMING_RESISTANCE");

	// maybe we should add some points for coastal tiles as well, so that watery maps don't get too much warming

	if (ePlayer == NO_PLAYER)
		return iGlobalThreshold;

	// I have a few possible threshold distribution systems in mind:
	// could be proportional to total natural food yield;
	// or a combination of population, land size, total completed research, per player, etc.
	// Currently, a player's share of the total threshold is just proportional to their land size (just like the threshold itself)
	CvPlayer& kPlayer = GET_PLAYER(ePlayer);
	if (kPlayer.isAlive())
	{
		return iGlobalThreshold * kPlayer.getTotalLand() / std::max(1, GC.getMap().getLandPlots());
	}
	else
		return 0;
}

int CvGame::calculateGwSeverityRating() const
{
	// Here are some of the properties I want from this function:
	// - the severity should be a number between 0 and 100 (ie. a percentage value)
	// - zero severity should mean zero global warming
	// - the function should asymptote towards 100
	//
	// - It should be a function of the index divided by (total land area * game length).

	// I recommend looking at the graph of this function to get a sense of how it works.

	const long x = GC.getDefineINT("GLOBAL_WARMING_PROB") * getGlobalWarmingIndex() / (std::max(1,4*GC.getGameSpeedInfo(getGameSpeedType()).getVictoryDelayPercent()*GC.getMap().getLandPlots()));
	const long b = 70; // shape parameter. Lower values result in the function being steeper earlier.
	return 100L - b*100L/(b+x*x);
}
/*
** K-mod end
*/

unsigned int CvGame::getInitialTime()
{
	return m_uiInitialTime;
}
/*  advc.003j (comment): Both unused since the BtS expansion, though the EXE still
	calls setInitialTime at the start of a game. */
void CvGame::setInitialTime(unsigned int uiNewValue)
{
	m_uiInitialTime = uiNewValue;
}


bool CvGame::isScoreDirty() const
{
	return m_bScoreDirty;
}


void CvGame::setScoreDirty(bool bNewValue)
{
	m_bScoreDirty = bNewValue;
}

// <advc.003r>
void CvGame::setUpdateTimer(UpdateTimerTypes eTimerType, int iDelay) {
	// <advc.001w>
	if(eTimerType == UPDATE_MOUSE_FOCUS && getBugOptionBOOL("MainInterface__RapidUnitCycling", false)) {
		// No need for this hack when there is no unit-cycling delay
		iDelay = -1;
	} // </advc.001w>
	m_aiUpdateTimers[eTimerType] = iDelay;
} // </advc.003r>


bool CvGame::isCircumnavigated() const
{
	return m_bCircumnavigated;
}


void CvGame::makeCircumnavigated()
{
	m_bCircumnavigated = true;
}

bool CvGame::circumnavigationAvailable() const
{
	if (isCircumnavigated())
	{
		return false;
	}

	if (GC.getDefineINT("CIRCUMNAVIGATE_FREE_MOVES") == 0)
	{
		return false;
	}

	CvMap& kMap = GC.getMap();

	if (!kMap.isWrapX() && !kMap.isWrapY())
	{
		return false;
	}

	if (kMap.getLandPlots() > (kMap.numPlots() * 2) / 3)
	{
		return false;
	}

	return true;
}

bool CvGame::isDiploVote(VoteSourceTypes eVoteSource) const
{
	return (getDiploVoteCount(eVoteSource) > 0);
}


int CvGame::getDiploVoteCount(VoteSourceTypes eVoteSource) const
{
	FAssert(eVoteSource >= 0 && eVoteSource < GC.getNumVoteSourceInfos());
	return m_aiDiploVote[eVoteSource];
}


void CvGame::changeDiploVote(VoteSourceTypes eVoteSource, int iChange)
{
	FAssert(eVoteSource >= 0 && eVoteSource < GC.getNumVoteSourceInfos());

	if (iChange != 0)
	{
		for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
		{
			GET_PLAYER((PlayerTypes)iPlayer).processVoteSourceBonus(eVoteSource, false);
		}

		m_aiDiploVote[eVoteSource] += iChange;
		FAssert(getDiploVoteCount(eVoteSource) >= 0);

		for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
		{
			GET_PLAYER((PlayerTypes)iPlayer).processVoteSourceBonus(eVoteSource, true);
		}
	}
}

bool CvGame::canDoResolution(VoteSourceTypes eVoteSource, const VoteSelectionSubData& kData) const
{
	CvVoteInfo const& kVote = GC.getVoteInfo(kData.eVote); // advc.003
	if (kVote.isVictory())
	{
		int iVotesRequired = getVoteRequired(kData.eVote, eVoteSource); // K-Mod
		for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
		{
			CvTeam& kTeam = GET_TEAM((TeamTypes)iTeam);

			if (kTeam.getVotes(kData.eVote, eVoteSource) >= iVotesRequired)
				return false; // K-Mod. same, but faster.
			/* original bts code
			if (kTeam.isVotingMember(eVoteSource)) {
				if (kTeam.getVotes(kData.eVote, eVoteSource) >= getVoteRequired(kData.eVote, eVoteSource)) {
					// Can't vote on a winner if one team already has all the votes necessary to win
					return false;
				}
			} */
		}
	}

	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);

		if (kPlayer.isVotingMember(eVoteSource))
		{	// <dlph.25/advc>
			if(kVote.isForceWar()) {
				if(GET_TEAM(kPlayer.getTeam()).isFullMember(eVoteSource) &&
						!kPlayer.canDoResolution(eVoteSource, kData))
					return false;
			}
			else // </dlph.25/advc>
			if (!kPlayer.canDoResolution(eVoteSource, kData))
			{
				return false;
			}
		}
		else if (kPlayer.isAlive() && !kPlayer.isBarbarian() && !kPlayer.isMinorCiv())
		{
			// all players need to be able to vote for a diplo victory
			if (kVote.isVictory())
				return false;
		}
	}

	return true;
}

bool CvGame::isValidVoteSelection(VoteSourceTypes eVoteSource, const VoteSelectionSubData& kData) const
{
	if (NO_PLAYER != kData.ePlayer)
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);
		if (!kPlayer.isAlive() || kPlayer.isBarbarian() || kPlayer.isMinorCiv())
		{
			return false;
		}
	}

	if (NO_PLAYER != kData.eOtherPlayer)
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.eOtherPlayer);
		if (!kPlayer.isAlive() || kPlayer.isBarbarian() || kPlayer.isMinorCiv())
		{
			return false;
		}
	}

	int iNumVoters = 0;
	for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
	{
		//if (GET_TEAM((TeamTypes)iTeam).isVotingMember(eVoteSource))
		// K-Mod. to prevent "AP cheese", only count full members for victory votes.
		if (GET_TEAM((TeamTypes)iTeam).isFullMember(eVoteSource) ||
			(!GC.getVoteInfo(kData.eVote).isVictory() && GET_TEAM((TeamTypes)iTeam).isVotingMember(eVoteSource)))
		// K-Mod end
		{
			++iNumVoters;
		}
	}
	if (iNumVoters  < GC.getVoteInfo(kData.eVote).getMinVoters())
	{
		return false;
	}

	if (GC.getVoteInfo(kData.eVote).isOpenBorders())
	{
		bool bOpenWithEveryone = true;
		for (int iTeam1 = 0; iTeam1 < MAX_CIV_TEAMS; ++iTeam1)
		{
			if (GET_TEAM((TeamTypes)iTeam1).isFullMember(eVoteSource))
			{
				for (int iTeam2 = iTeam1 + 1; iTeam2 < MAX_CIV_TEAMS; ++iTeam2)
				{
					CvTeam& kTeam2 = GET_TEAM((TeamTypes)iTeam2);

					if (kTeam2.isFullMember(eVoteSource))
					{
						if (!kTeam2.isOpenBorders((TeamTypes)iTeam1))
						{
							bOpenWithEveryone = false;
							break;
						}
					}
				}
			}
		}
		if (bOpenWithEveryone)
		{
			return false;
		}
	}
	else if (GC.getVoteInfo(kData.eVote).isDefensivePact())
	{
		bool bPactWithEveryone = true;
		for (int iTeam1 = 0; iTeam1 < MAX_CIV_TEAMS; ++iTeam1)
		{
			CvTeam& kTeam1 = GET_TEAM((TeamTypes)iTeam1);
			if (kTeam1.isFullMember(eVoteSource)
					&& !kTeam1.isAVassal()) // advc.001
			{
				for (int iTeam2 = iTeam1 + 1; iTeam2 < MAX_CIV_TEAMS; ++iTeam2)
				{
					CvTeam& kTeam2 = GET_TEAM((TeamTypes)iTeam2);
					if (kTeam2.isFullMember(eVoteSource)
							&& !kTeam2.isAVassal()) // advc.001
					{
						if (!kTeam2.isDefensivePact((TeamTypes)iTeam1))
						{
							bPactWithEveryone = false;
							break;
						}
					}
				}
			}
		}
		if (bPactWithEveryone)
		{
			return false;
		}
	}
	else if (GC.getVoteInfo(kData.eVote).isForcePeace())
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);

		if(kPlayer.isAVassal()) // advc.003
			return false;
		//if (!kPlayer.isFullMember(eVoteSource))
		// dlph.25: 'These are not necessarily the same.'
		if (!GET_TEAM(kPlayer.getTeam()).isFullMember(eVoteSource))
		{
			return false;
		}

		bool bValid = false;

		for (int iTeam2 = 0; iTeam2 < MAX_CIV_TEAMS; ++iTeam2)
		{
			if (atWar(kPlayer.getTeam(), (TeamTypes)iTeam2))
			{
				CvTeam& kTeam2 = GET_TEAM((TeamTypes)iTeam2);

				if (kTeam2.isVotingMember(eVoteSource))
				{
					bValid = true;
					break;
				}
			}
		}

		if (!bValid)
		{
			return false;
		}
	}
	else if (GC.getVoteInfo(kData.eVote).isForceNoTrade())
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);
		//if (kPlayer.isFullMember(eVoteSource))
		// dlph.25: 'These are not necessarily the same.'
		if (GET_TEAM(kPlayer.getTeam()).isFullMember(eVoteSource))
		{
			return false;
		}

		bool bNoTradeWithEveryone = true;
		for (int iPlayer2 = 0; iPlayer2 < MAX_CIV_PLAYERS; ++iPlayer2)
		{
			CvPlayer& kPlayer2 = GET_PLAYER((PlayerTypes)iPlayer2);
			if (kPlayer2.getTeam() != kPlayer.getTeam())
			{
				//if (kPlayer2.isFullMember(eVoteSource))
				// dlph.25: 'These are not necessarily the same.'
				if (GET_TEAM(kPlayer2.getTeam()).isFullMember(eVoteSource))
				{
					if (kPlayer2.canStopTradingWithTeam(kPlayer.getTeam()))
					{
						bNoTradeWithEveryone = false;
						break;
					}
				}
			}
		}
		// Not an option if already at war with everyone
		if (bNoTradeWithEveryone)
		{
			return false;
		}
	}
	else if (GC.getVoteInfo(kData.eVote).isForceWar())
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);
		CvTeam& kTeam = GET_TEAM(kPlayer.getTeam());

		if (kTeam.isAVassal())
		{
			return false;
		}
		//if (kPlayer.isFullMember(eVoteSource))
		// dlph.25: 'These are not necessarily the same.'
		if (GET_TEAM(kPlayer.getTeam()).isFullMember(eVoteSource))
		{
			return false;
		}

		bool bAtWarWithEveryone = true;
		for (int iTeam2 = 0; iTeam2 < MAX_CIV_TEAMS; ++iTeam2)
		{
			if (iTeam2 != kPlayer.getTeam())
			{
				CvTeam& kTeam2 = GET_TEAM((TeamTypes)iTeam2);
				if (kTeam2.isFullMember(eVoteSource))
				{
					if (!kTeam2.isAtWar(kPlayer.getTeam()) && kTeam2.canChangeWarPeace(kPlayer.getTeam()))
					{
						bAtWarWithEveryone = false;
						break;
					}
				}
			}
		}
		// Not an option if already at war with everyone
		if (bAtWarWithEveryone)
		{
			return false;
		}

		//if (!kPlayer.isVotingMember(eVoteSource))
		// dlph.25: Replacing the above
		if (!GET_TEAM(kPlayer.getTeam()).isFullMember(eVoteSource))
		{
			// Can be passed only if already at war with a member
			bool bValid = false;
			for (int iTeam2 = 0; iTeam2 < MAX_CIV_TEAMS; ++iTeam2)
			{
				if (atWar(kPlayer.getTeam(), (TeamTypes)iTeam2))
				{
					CvTeam& kTeam2 = GET_TEAM((TeamTypes)iTeam2);

					if (kTeam2.isFullMember(eVoteSource))
					{
						bValid = true;
						break;
					}
				}
			}

			if (!bValid)
			{
				return false;
			}
		}
	}
	else if (GC.getVoteInfo(kData.eVote).isAssignCity())
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);
		//if (kPlayer.isFullMember(eVoteSource) || !kPlayer.isVotingMember(eVoteSource))
		// dlph.25: 'These are not necessarily the same'
		if (GET_TEAM(kPlayer.getTeam()).isFullMember(eVoteSource) || !GET_TEAM(kPlayer.getTeam()).isVotingMember(eVoteSource))
		{
			return false;
		}

		CvCity* pCity = kPlayer.getCity(kData.iCityId);
		FAssert(NULL != pCity);
		if (NULL == pCity)
		{
			return false;
		}

		if (NO_PLAYER == kData.eOtherPlayer)
		{
			return false;
		}

		CvPlayer& kOtherPlayer = GET_PLAYER(kData.eOtherPlayer);
		if (kOtherPlayer.getTeam() == kPlayer.getTeam())
		{
			return false;
		}

		if (atWar(kPlayer.getTeam(), GET_PLAYER(kData.eOtherPlayer).getTeam()))
		{
			return false;
		}

		//if (!kOtherPlayer.isFullMember(eVoteSource))
		// dlph.25: 'These are not necessarily the same'
		if (!GET_TEAM(kOtherPlayer.getTeam()).isFullMember(eVoteSource))
		{
			return false;
		}

		if (kOtherPlayer.isHuman() && isOption(GAMEOPTION_ONE_CITY_CHALLENGE))
		{
			return false;
		}
	}

	if (!canDoResolution(eVoteSource, kData))
	{
		return false;
	}

	return true;
}


bool CvGame::isDebugMode() const
{
	return m_bDebugModeCache;
}


void CvGame::toggleDebugMode()
{	// <advc.135c>
	if(!m_bDebugMode && !isDebugToolsAllowed(false))
		return; // </advc.135c>
	m_bDebugMode = (m_bDebugMode ? false : true);
	updateDebugModeCache();

	GC.getMap().updateVisibility();
	GC.getMap().updateSymbols();
	GC.getMap().updateMinimapColor();
	GC.getMap().setFlagsDirty(); // K-Mod
	updateColoredPlots(); // K-Mod

	gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
	gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
	gDLL->getInterfaceIFace()->setDirty(MinimapSection_DIRTY_BIT, true);
	gDLL->getInterfaceIFace()->setDirty(UnitInfo_DIRTY_BIT, true);
	gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
	gDLL->getInterfaceIFace()->setDirty(GlobeLayer_DIRTY_BIT, true);

	//gDLL->getEngineIFace()->SetDirty(GlobeTexture_DIRTY_BIT, true);
	gDLL->getEngineIFace()->SetDirty(MinimapTexture_DIRTY_BIT, true);
	gDLL->getEngineIFace()->SetDirty(CultureBorders_DIRTY_BIT, true);

	if (m_bDebugMode)
	{
		gDLL->getEngineIFace()->PushFogOfWar(FOGOFWARMODE_OFF);
	}
	else
	{
		gDLL->getEngineIFace()->PopFogOfWar();
	}
	gDLL->getEngineIFace()->setFogOfWarFromStack();
}

void CvGame::updateDebugModeCache()
{
	//if ((gDLL->getChtLvl() > 0) || (gDLL->GetWorldBuilderMode()))
	/*  advc.135c: Replacing the above (should perhaps just remove the check
		b/c toggleDebugMode already checks isDebugToolsAllowed) */
	if(isDebugToolsAllowed(false))
		m_bDebugModeCache = m_bDebugMode;
	else m_bDebugModeCache = false;
}

// <advc.135c>
bool CvGame::isDebugToolsAllowed(bool bWB) const {

	if(gDLL->getInterfaceIFace()->isInAdvancedStart())
		return false;
	if(gDLL->GetWorldBuilderMode())
		return true;
	if(isGameMultiPlayer()) {
		if(GC.getDefineINT("ENABLE_DEBUG_TOOLS_MULTIPLAYER") <= 0)
			return false;
		if(isHotSeat())
			return true;
		// (CvGame::getName isn't const)
		CvWString const& szGameName = GC.getInitCore().getGameName();
		return (szGameName.compare(L"chipotle") == 0);
	}
	if(bWB) {
		// Cut and pasted from canDoControl (CvGameInterface.cpp)
		return GC.getInitCore().getAdminPassword().empty();
	}
	return gDLL->getChtLvl() > 0;
} // </advc.135c>


int CvGame::getPitbossTurnTime() const
{
	return GC.getInitCore().getPitbossTurnTime();
}

void CvGame::setPitbossTurnTime(int iHours)
{
	GC.getInitCore().setPitbossTurnTime(iHours);
}


bool CvGame::isHotSeat() const
{
	return (GC.getInitCore().getHotseat());
}

bool CvGame::isPbem() const
{
	return (GC.getInitCore().getPbem());
}



bool CvGame::isPitboss() const
{
	return (GC.getInitCore().getPitboss());
}

bool CvGame::isSimultaneousTeamTurns() const
{
	if (!isNetworkMultiPlayer())
	{
		return false;
	}

	if (isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
	{
		return false;
	}

	return true;
}


bool CvGame::isFinalInitialized() const
{
	return m_bFinalInitialized;
}


void CvGame::setFinalInitialized(bool bNewValue)
{
	PROFILE_FUNC();

	if (isFinalInitialized() == bNewValue)
		return; // advc.003
	m_bFinalInitialized = bNewValue;
	if (!isFinalInitialized())
		return;

	updatePlotGroups();
	GC.getMap().updateIrrigated();

	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		CvTeamAI& kTeam = GET_TEAM((TeamTypes)iI);
		kTeam.AI_initMemory(); // K-Mod
		if (kTeam.isAlive())
			kTeam.AI_updateAreaStrategies();
	}
}

// <advc.004x>
void CvGame::setDawnOfManShown(bool b) {

	m_bDoMShown = b;
}


bool CvGame::isAboutToShowDawnOfMan() const {

	return (!m_bDoMShown && getElapsedGameTurns() <= 0);
} // </advc.004x>

// <advc.061>
void CvGame::setScreenDimensions(int x, int y) {

	m_iScreenWidth = x;
	m_iScreenHeight = y;
}

int CvGame::getScreenWidth() const {

	return m_iScreenWidth;
}

int CvGame::getScreenHeight() const {

	return m_iScreenHeight;
} // </advc.061>


bool CvGame::getPbemTurnSent() const
{
	return m_bPbemTurnSent;
}


void CvGame::setPbemTurnSent(bool bNewValue)
{
	m_bPbemTurnSent = bNewValue;
}


bool CvGame::getHotPbemBetweenTurns() const
{
	return m_bHotPbemBetweenTurns;
}


void CvGame::setHotPbemBetweenTurns(bool bNewValue)
{
	m_bHotPbemBetweenTurns = bNewValue;
}


bool CvGame::isPlayerOptionsSent() const
{
	return m_bPlayerOptionsSent;
}


void CvGame::sendPlayerOptions(bool bForce)
{
	if (getActivePlayer() == NO_PLAYER)
	{
		return;
	}

	if (!isPlayerOptionsSent() || bForce)
	{
		m_bPlayerOptionsSent = true;

		for (int iI = 0; iI < NUM_PLAYEROPTION_TYPES; iI++)
		{
			gDLL->sendPlayerOption(((PlayerOptionTypes)iI), gDLL->getPlayerOption((PlayerOptionTypes)iI));
		}
	}
}


PlayerTypes CvGame::getActivePlayer() const
{
	return GC.getInitCore().getActivePlayer();
}


void CvGame::setActivePlayer(PlayerTypes eNewValue, bool bForceHotSeat)
{
	PlayerTypes eOldActivePlayer = getActivePlayer();
	if (eOldActivePlayer != eNewValue)
	{
		int iActiveNetId = ((NO_PLAYER != eOldActivePlayer) ? GET_PLAYER(eOldActivePlayer).getNetID() : -1);
		GC.getInitCore().setActivePlayer(eNewValue);

		//if (GET_PLAYER(eNewValue).isHuman() && (isHotSeat() || isPbem() || bForceHotSeat))
		if (eNewValue != NO_PLAYER && GET_PLAYER(eNewValue).isHuman() && (isHotSeat() || isPbem() || bForceHotSeat)) // K-Mod
		{
			gDLL->getPassword(eNewValue);
			setHotPbemBetweenTurns(false);
			gDLL->getInterfaceIFace()->dirtyTurnLog(eNewValue);

			if (NO_PLAYER != eOldActivePlayer)
			{
				int iInactiveNetId = GET_PLAYER(eNewValue).getNetID();
				GET_PLAYER(eNewValue).setNetID(iActiveNetId);
				GET_PLAYER(eOldActivePlayer).setNetID(iInactiveNetId);
			}

			GET_PLAYER(eNewValue).showMissedMessages();

			if (countHumanPlayersAlive() == 1 && isPbem())
			{
				// Nobody else left alive
				GC.getInitCore().setType(GAME_HOTSEAT_NEW);
			}

			if (isHotSeat() || bForceHotSeat)
			{
				sendPlayerOptions(true);
			}
		}
		updateActiveVisibility(); // advc.706: Moved into subroutine
	}
}

// advc.706: Cut and pasted from CvGame::setActivePlayer
void CvGame::updateActiveVisibility() {

		if(!GC.IsGraphicsInitialized())
			return;
		GC.getMap().updateFog();
		GC.getMap().updateVisibility();
		GC.getMap().updateSymbols();
		GC.getMap().updateMinimapColor();

		updateUnitEnemyGlow();

		gDLL->getInterfaceIFace()->setEndTurnMessage(false);

		gDLL->getInterfaceIFace()->clearSelectedCities();
		gDLL->getInterfaceIFace()->clearSelectionList();

		gDLL->getInterfaceIFace()->setDirty(PercentButtons_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(ResearchButtons_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(MinimapSection_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(UnitInfo_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(Flag_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(GlobeLayer_DIRTY_BIT, true);
		// <advc.003p>
		if(getActivePlayer() != NO_PLAYER)
			GET_PLAYER(getActivePlayer()).setBonusHelpDirty();
		FAssert(getActivePlayer() != NO_PLAYER);
		// </advc.003p>
		gDLL->getEngineIFace()->SetDirty(CultureBorders_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(BlockadedPlots_DIRTY_BIT, true);
}


void CvGame::updateUnitEnemyGlow()
{
	for(int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayer const& kPlayer = GET_PLAYER((PlayerTypes)i);
		if (!kPlayer.isAlive())
			continue; // advc.003b
		FOR_EACH_UNIT(pLoopUnit, kPlayer)
			gDLL->getEntityIFace()->updateEnemyGlow(pLoopUnit->getUnitEntity());
	}
}

HandicapTypes CvGame::getHandicapType() const
{
	return m_eHandicap;
}

void CvGame::setHandicapType(HandicapTypes eHandicap)
{
	m_eHandicap = eHandicap;
}

// <advc.127>
HandicapTypes CvGame::getAIHandicap() const {

	return m_eAIHandicap;
} // </advc.127>

/*  <advc.250> This was originally handled in CvUtils.py (getScoreComponent),
	but gets a bit more involved with SPaH. */
int CvGame::getDifficultyForEndScore() const {

	CvHandicapInfo& h = GC.getHandicapInfo(getHandicapType());
	int r = h.getDifficulty();
	if(isOption(GAMEOPTION_ONE_CITY_CHALLENGE))
		r += 30;
	if(!isOption(GAMEOPTION_SPAH))
		return r;
	std::vector<int> aiStartPointDistrib;
	m_pSpah->distribution(aiStartPointDistrib);
	std::vector<double> distr;
	for(size_t i = 0; i < aiStartPointDistrib.size(); i++)
		distr.push_back(aiStartPointDistrib[i]);
	return r + ::round((::dMax(distr) + ::dMean(distr)) /
			h.getAIAdvancedStartPercent());
} // </advc.250>

PlayerTypes CvGame::getPausePlayer() const
{
	return m_ePausePlayer;
}


bool CvGame::isPaused() const
{
	return (getPausePlayer() != NO_PLAYER);
}


void CvGame::setPausePlayer(PlayerTypes eNewValue)
{
	m_ePausePlayer = eNewValue;
}


UnitTypes CvGame::getBestLandUnit() const
{
	return m_eBestLandUnit;
}


int CvGame::getBestLandUnitCombat() const
{
	if (getBestLandUnit() == NO_UNIT)
	{
		return 1;
	}

	return std::max(1, GC.getUnitInfo(getBestLandUnit()).getCombat());
}


void CvGame::setBestLandUnit(UnitTypes eNewValue)
{
	if (getBestLandUnit() != eNewValue)
	{
		m_eBestLandUnit = eNewValue;

		gDLL->getInterfaceIFace()->setDirty(UnitInfo_DIRTY_BIT, true);
	}
}


TeamTypes CvGame::getWinner() const
{
	return m_eWinner;
}


VictoryTypes CvGame::getVictory() const
{
	return m_eVictory;
}


void CvGame::setWinner(TeamTypes eNewWinner, VictoryTypes eNewVictory)
{
	CvWString szBuffer;

	if ((getWinner() != eNewWinner) || (getVictory() != eNewVictory))
	{
		m_eWinner = eNewWinner;
		m_eVictory = eNewVictory;
		// advc.707: Handled by RiseFall::prepareForExtendedGame
		if(!isOption(GAMEOPTION_RISE_FALL))
			// AI_AUTO_PLAY_MOD, 07/09/08, jdog5000:
			CvEventReporter::getInstance().victory(eNewWinner, eNewVictory);

		if (getVictory() != NO_VICTORY)
		{
			if (getWinner() != NO_TEAM)
			{
				szBuffer = gDLL->getText("TXT_KEY_GAME_WON", GET_TEAM(getWinner()).getReplayName().GetCString(), GC.getVictoryInfo(getVictory()).getTextKeyWide());
				addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, GET_TEAM(getWinner()).getLeaderID(), szBuffer, -1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
			}

			if ((getAIAutoPlay() > 0 || gDLL->GetAutorun())
					&& !isOption(GAMEOPTION_RISE_FALL)) // advc.707
			{
				setGameState(GAMESTATE_EXTENDED);
			}
			else
			{
				setGameState(GAMESTATE_OVER);
			}
		}

		gDLL->getInterfaceIFace()->setDirty(Center_DIRTY_BIT, true);
		// AI_AUTO_PLAY_MOD, 07/09/08, jdog5000 (commented out)
		//CvEventReporter::getInstance().victory(eNewWinner, eNewVictory);
		gDLL->getInterfaceIFace()->setDirty(Soundtrack_DIRTY_BIT, true);
	}
}


GameStateTypes CvGame::getGameState() const
{
	return m_eGameState;
}


void CvGame::setGameState(GameStateTypes eNewValue)
{
	if (getGameState() == eNewValue)
		return; // advc.003

	m_eGameState = eNewValue;
	if (eNewValue == GAMESTATE_OVER)
	{
		CvEventReporter::getInstance().gameEnd();
		gDLL->getPythonIFace()->callFunction(PYCivModule, "gameEndSave"); // BULL - AutoSave
		// <advc.707>
		if(isOption(GAMEOPTION_RISE_FALL))
			m_pRiseFall->prepareForExtendedGame(); // </advc.707>
		showEndGameSequence();
		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isHuman())
			{	// One more turn?
				CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_EXTENDED_GAME);
				if (NULL != pInfo)
					GET_PLAYER((PlayerTypes)iI).addPopup(pInfo);
			}
		}
	}
	gDLL->getInterfaceIFace()->setDirty(Cursor_DIRTY_BIT, true);
}

// <advc.106h>
PlayerTypes CvGame::getInitialActivePlayer() const {

	return m_eInitialActivePlayer;
} // </advc.106h>


GameSpeedTypes CvGame::getGameSpeedType() const
{
	return GC.getInitCore().getGameSpeed();
}


EraTypes CvGame::getStartEra() const
{
	return GC.getInitCore().getEra();
}


CalendarTypes CvGame::getCalendar() const
{
	return GC.getInitCore().getCalendar();
}


PlayerTypes CvGame::getRankPlayer(int iRank) const
{
	FAssertMsg(iRank >= 0, "iRank is expected to be non-negative (invalid Rank)");
	FAssertMsg(iRank < MAX_PLAYERS, "iRank is expected to be within maximum bounds (invalid Rank)");
	return (PlayerTypes)m_aiRankPlayer[iRank];
}


void CvGame::setRankPlayer(int iRank, PlayerTypes ePlayer)
{
	FAssertMsg(iRank >= 0, "iRank is expected to be non-negative (invalid Rank)");
	FAssertMsg(iRank < MAX_PLAYERS, "iRank is expected to be within maximum bounds (invalid Rank)");

	if (getRankPlayer(iRank) != ePlayer)
	{
		m_aiRankPlayer[iRank] = ePlayer;

		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
	}
}


int CvGame::getPlayerRank(PlayerTypes ePlayer) const
{	// advc (comment): The topmost rank is 0
	FAssertMsg(ePlayer >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(ePlayer < MAX_PLAYERS, "ePlayer is expected to be within maximum bounds (invalid Index)");
	return m_aiPlayerRank[ePlayer];
}


void CvGame::setPlayerRank(PlayerTypes ePlayer, int iRank)
{
	FAssertMsg(ePlayer >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(ePlayer < MAX_PLAYERS, "ePlayer is expected to be within maximum bounds (invalid Index)");
	m_aiPlayerRank[ePlayer] = iRank;
	FAssert(getPlayerRank(ePlayer) >= 0);
}


int CvGame::getPlayerScore(PlayerTypes ePlayer)	const
{
	FAssertMsg(ePlayer >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(ePlayer < MAX_PLAYERS, "ePlayer is expected to be within maximum bounds (invalid Index)");
	return m_aiPlayerScore[ePlayer];
}


void CvGame::setPlayerScore(PlayerTypes ePlayer, int iScore)
{
	FAssertMsg(ePlayer >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(ePlayer < MAX_PLAYERS, "ePlayer is expected to be within maximum bounds (invalid Index)");

	if (getPlayerScore(ePlayer) != iScore)
	{
		m_aiPlayerScore[ePlayer] = iScore;
		FAssert(getPlayerScore(ePlayer) >= 0);

		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
	}
}


TeamTypes CvGame::getRankTeam(int iRank) const
{
	FAssertMsg(iRank >= 0, "iRank is expected to be non-negative (invalid Rank)");
	FAssertMsg(iRank < MAX_TEAMS, "iRank is expected to be within maximum bounds (invalid Index)");
	return (TeamTypes)m_aiRankTeam[iRank];
}


void CvGame::setRankTeam(int iRank, TeamTypes eTeam)
{
	FAssertMsg(iRank >= 0, "iRank is expected to be non-negative (invalid Rank)");
	FAssertMsg(iRank < MAX_TEAMS, "iRank is expected to be within maximum bounds (invalid Index)");

	if (getRankTeam(iRank) != eTeam)
	{
		m_aiRankTeam[iRank] = eTeam;

		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
	}
}


int CvGame::getTeamRank(TeamTypes eTeam) const
{
	FAssertMsg(eTeam >= 0, "eTeam is expected to be non-negative (invalid Index)");
	FAssertMsg(eTeam < MAX_TEAMS, "eTeam is expected to be within maximum bounds (invalid Index)");
	return m_aiTeamRank[eTeam];
}


void CvGame::setTeamRank(TeamTypes eTeam, int iRank)
{
	FAssertMsg(eTeam >= 0, "eTeam is expected to be non-negative (invalid Index)");
	FAssertMsg(eTeam < MAX_TEAMS, "eTeam is expected to be within maximum bounds (invalid Index)");
	m_aiTeamRank[eTeam] = iRank;
	FAssert(getTeamRank(eTeam) >= 0);
}


int CvGame::getTeamScore(TeamTypes eTeam) const
{
	FAssertMsg(eTeam >= 0, "eTeam is expected to be non-negative (invalid Index)");
	FAssertMsg(eTeam < MAX_TEAMS, "eTeam is expected to be within maximum bounds (invalid Index)");
	return m_aiTeamScore[eTeam];
}


void CvGame::setTeamScore(TeamTypes eTeam, int iScore)
{
	FAssertMsg(eTeam >= 0, "eTeam is expected to be non-negative (invalid Index)");
	FAssertMsg(eTeam < MAX_TEAMS, "eTeam is expected to be within maximum bounds (invalid Index)");
	m_aiTeamScore[eTeam] = iScore;
	FAssert(getTeamScore(eTeam) >= 0);
}


bool CvGame::isOption(GameOptionTypes eIndex) const
{	// <advc.003b>
	if(eIndex < 0 || eIndex >= NUM_GAMEOPTION_TYPES) {
		FASSERT_BOUNDS(0, NUM_GAMEOPTION_TYPES, eIndex, "No such game option");
		return false;
	} // Use inline functions. Probably doesn't matter, but feels better.
	return GC.getInitCore().getOptions()[eIndex]; // </advc.003b>
}


void CvGame::setOption(GameOptionTypes eIndex, bool bEnabled)
{
	GC.getInitCore().setOption(eIndex, bEnabled);
}


bool CvGame::isMPOption(MultiplayerOptionTypes eIndex) const
{
	return GC.getInitCore().getMPOption(eIndex);
}


void CvGame::setMPOption(MultiplayerOptionTypes eIndex, bool bEnabled)
{
	GC.getInitCore().setMPOption(eIndex, bEnabled);
}


bool CvGame::isForcedControl(ForceControlTypes eIndex) const
{
	return GC.getInitCore().getForceControl(eIndex);
}


void CvGame::setForceControl(ForceControlTypes eIndex, bool bEnabled)
{
	GC.getInitCore().setForceControl(eIndex, bEnabled);
}

// advc.003: Mostly cut from CvPlayer::canConstruct
bool CvGame::canConstruct(BuildingTypes eBuilding, bool bIgnoreCost, bool bTestVisible) const {

	if(!bIgnoreCost && GC.getBuildingInfo(eBuilding).getProductionCost() == -1)
		return false;

	if(getCivTeamsEverAlive() < GC.getBuildingInfo(eBuilding).getNumTeamsPrereq())
		return false;

	VictoryTypes ePrereqVict = (VictoryTypes)GC.getBuildingInfo(eBuilding).
			getVictoryPrereq();
	if(ePrereqVict != NO_VICTORY && !isVictoryValid(ePrereqVict))
		return false;

	int iMaxStartEra = GC.getBuildingInfo(eBuilding).getMaxStartEra();
	if(iMaxStartEra != NO_ERA && getStartEra() > iMaxStartEra)
		return false;

	if(isBuildingClassMaxedOut((BuildingClassTypes)
			(GC.getBuildingInfo(eBuilding).getBuildingClassType())))
		return false;

	if(bTestVisible)
		return true;

	if(isNoNukes() && GC.getBuildingInfo(eBuilding).isAllowsNukes()) {
		return false;
		// What the original code did:
		/*for(int i = 0; i < GC.getNumUnitInfos(); i++) {
			if (GC.getUnitInfo((UnitTypes)i).getNukeRange() != -1)
				return false;
		}*/
	}

	SpecialBuildingTypes eSpecial = (SpecialBuildingTypes)GC.getBuildingInfo(eBuilding).
			getSpecialBuildingType();
	if(eSpecial != NO_SPECIALBUILDING && !isSpecialBuildingValid(eSpecial))
		return false;

	if(getNumCities() < GC.getBuildingInfo(eBuilding).getNumCitiesPrereq())
		return false;

	return true;
}


int CvGame::getUnitCreatedCount(UnitTypes eIndex) const
{
	FASSERT_BOUNDS(0, GC.getNumUnitInfos(), eIndex, "CvGame::setUnitCreatedCount");
	return m_paiUnitCreatedCount[eIndex];
}


void CvGame::incrementUnitCreatedCount(UnitTypes eIndex)
{
	FASSERT_BOUNDS(0, GC.getNumUnitInfos(), eIndex, "CvGame::incrementUnitCreatedCount");
	m_paiUnitCreatedCount[eIndex]++;
}


int CvGame::getUnitClassCreatedCount(UnitClassTypes eIndex) const
{
	FASSERT_BOUNDS(0, GC.getNumUnitInfos(), eIndex, "CvGame::getUnitCreatedCount");
	return m_paiUnitClassCreatedCount[eIndex];
}


bool CvGame::isUnitClassMaxedOut(UnitClassTypes eIndex, int iExtra) const
{
	FASSERT_BOUNDS(0, GC.getNumUnitClassInfos(), eIndex, "CvGame::isUnitClassMaxedOut");

	if (!isWorldUnitClass(eIndex))
	{
		return false;
	}
	FASSERT_BOUNDS(0, GC.getUnitClassInfo(eIndex).getMaxGlobalInstances()+1, getUnitClassCreatedCount(eIndex), "CvGame::isUnitClassMaxedOut");


	return ((getUnitClassCreatedCount(eIndex) + iExtra) >= GC.getUnitClassInfo(eIndex).getMaxGlobalInstances());
}


void CvGame::incrementUnitClassCreatedCount(UnitClassTypes eIndex)
{
	FASSERT_BOUNDS(0, GC.getNumUnitClassInfos(), eIndex, "CvGame::incrementUnitClassCreatedCount");
	m_paiUnitClassCreatedCount[eIndex]++;
}


int CvGame::getBuildingClassCreatedCount(BuildingClassTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiBuildingClassCreatedCount[eIndex];
}


bool CvGame::isBuildingClassMaxedOut(BuildingClassTypes eIndex, int iExtra) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (!isWorldWonderClass(eIndex))
	{
		return false;
	}

	FAssertMsg(getBuildingClassCreatedCount(eIndex) <= GC.getBuildingClassInfo(eIndex).getMaxGlobalInstances(), "Index is expected to be within maximum bounds (invalid Index)");

	return ((getBuildingClassCreatedCount(eIndex) + iExtra) >= GC.getBuildingClassInfo(eIndex).getMaxGlobalInstances());
}


void CvGame::incrementBuildingClassCreatedCount(BuildingClassTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiBuildingClassCreatedCount[eIndex]++;
}


int CvGame::getProjectCreatedCount(ProjectTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiProjectCreatedCount[eIndex];
}


bool CvGame::isProjectMaxedOut(ProjectTypes eIndex, int iExtra) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (!isWorldProject(eIndex))
	{
		return false;
	}

	FAssertMsg(getProjectCreatedCount(eIndex) <= GC.getProjectInfo(eIndex).getMaxGlobalInstances(), "Index is expected to be within maximum bounds (invalid Index)");

	return ((getProjectCreatedCount(eIndex) + iExtra) >= GC.getProjectInfo(eIndex).getMaxGlobalInstances());
}


void CvGame::incrementProjectCreatedCount(ProjectTypes eIndex, int iExtra)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiProjectCreatedCount[eIndex] += iExtra;
}


int CvGame::getForceCivicCount(CivicTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCivicInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiForceCivicCount[eIndex];
}


bool CvGame::isForceCivic(CivicTypes eIndex) const
{
	return (getForceCivicCount(eIndex) > 0);
}


bool CvGame::isForceCivicOption(CivicOptionTypes eCivicOption) const
{
	int iI;

	for (iI = 0; iI < GC.getNumCivicInfos(); iI++)
	{
		if (GC.getCivicInfo((CivicTypes)iI).getCivicOptionType() == eCivicOption)
		{
			if (isForceCivic((CivicTypes)iI))
			{
				return true;
			}
		}
	}

	return false;
}


void CvGame::changeForceCivicCount(CivicTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCivicInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange == 0)
		return;

	bool bOldForceCivic = isForceCivic(eIndex);

	m_paiForceCivicCount[eIndex] += iChange;
	FAssert(getForceCivicCount(eIndex) >= 0);

	if (bOldForceCivic != isForceCivic(eIndex))
	{
		verifyCivics();
	}
}


PlayerVoteTypes CvGame::getVoteOutcome(VoteTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumVoteInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiVoteOutcome[eIndex];
}


bool CvGame::isVotePassed(VoteTypes eIndex) const
{
	PlayerVoteTypes ePlayerVote = getVoteOutcome(eIndex);

	if (isTeamVote(eIndex))
	{
		return (ePlayerVote >= 0 && ePlayerVote < MAX_CIV_TEAMS);
	}
	else
	{
		return (ePlayerVote == PLAYER_VOTE_YES);
	}
}


void CvGame::setVoteOutcome(const VoteTriggeredData& kData, PlayerVoteTypes eNewValue)
{
	VoteTypes eIndex = kData.kVoteOption.eVote;
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumVoteInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (getVoteOutcome(eIndex) != eNewValue)
	{
		bool bOldPassed = isVotePassed(eIndex);

		m_paiVoteOutcome[eIndex] = eNewValue;

		if (bOldPassed != isVotePassed(eIndex))
		{
			processVote(kData, ((isVotePassed(eIndex)) ? 1 : -1));
		}
	}

	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kPlayer.isAlive())
		{
			kPlayer.setVote(kData.getID(), NO_PLAYER_VOTE);
		}
	}
}


int CvGame::getReligionGameTurnFounded(ReligionTypes eIndex) const
{
	FASSERT_BOUNDS(0, GC.getNumReligionInfos(), eIndex, "CvGame::getReligionGameTurnFounded");
	return m_paiReligionGameTurnFounded[eIndex];
}


bool CvGame::isReligionFounded(ReligionTypes eIndex) const
{
	return (getReligionGameTurnFounded(eIndex) != -1);
}


void CvGame::makeReligionFounded(ReligionTypes eIndex, PlayerTypes ePlayer)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumReligionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (!isReligionFounded(eIndex))
	{
		FAssertMsg(getGameTurn() != -1, "getGameTurn() is not expected to be equal with -1");
		m_paiReligionGameTurnFounded[eIndex] = getGameTurn();

		CvEventReporter::getInstance().religionFounded(eIndex, ePlayer);
	}
}

bool CvGame::isReligionSlotTaken(ReligionTypes eReligion) const
{
	FAssertMsg(eReligion >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eReligion < GC.getNumReligionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_abReligionSlotTaken[eReligion];
}

void CvGame::setReligionSlotTaken(ReligionTypes eReligion, bool bTaken)
{
	FAssertMsg(eReligion >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eReligion < GC.getNumReligionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_abReligionSlotTaken[eReligion] = bTaken;
}


int CvGame::getCorporationGameTurnFounded(CorporationTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCorporationInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiCorporationGameTurnFounded[eIndex];
}


bool CvGame::isCorporationFounded(CorporationTypes eIndex) const
{
	return (getCorporationGameTurnFounded(eIndex) != -1);
}


void CvGame::makeCorporationFounded(CorporationTypes eIndex, PlayerTypes ePlayer)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCorporationInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (!isCorporationFounded(eIndex))
	{
		FAssertMsg(getGameTurn() != -1, "getGameTurn() is not expected to be equal with -1");
		m_paiCorporationGameTurnFounded[eIndex] = getGameTurn();

		CvEventReporter::getInstance().corporationFounded(eIndex, ePlayer);
	}
}

bool CvGame::isVictoryValid(VictoryTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumVictoryInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return GC.getInitCore().getVictory(eIndex);
}

void CvGame::setVictoryValid(VictoryTypes eIndex, bool bValid)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumVictoryInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	GC.getInitCore().setVictory(eIndex, bValid);
}


bool CvGame::isSpecialUnitValid(SpecialUnitTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumSpecialUnitInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_pabSpecialUnitValid[eIndex];
}


void CvGame::makeSpecialUnitValid(SpecialUnitTypes eIndex)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumSpecialUnitInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_pabSpecialUnitValid[eIndex] = true;
}


bool CvGame::isSpecialBuildingValid(SpecialBuildingTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumSpecialBuildingInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_pabSpecialBuildingValid[eIndex];
}


void CvGame::makeSpecialBuildingValid(SpecialBuildingTypes eIndex, bool bAnnounce)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumSpecialBuildingInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (!m_pabSpecialBuildingValid[eIndex])
	{
		m_pabSpecialBuildingValid[eIndex] = true;


		if (bAnnounce)
		{
			CvWString szBuffer = gDLL->getText("TXT_KEY_SPECIAL_BUILDING_VALID", GC.getSpecialBuildingInfo(eIndex).getTextKeyWide());

			for (int iI = 0; iI < MAX_PLAYERS; iI++)
			{
				if (GET_PLAYER((PlayerTypes)iI).isAlive())
				{
					gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_PROJECT_COMPLETED", MESSAGE_TYPE_MAJOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
				}
			}
		}
	}
}


bool CvGame::isNukesValid() const
{
	return m_bNukesValid;
}


void CvGame::makeNukesValid(bool bValid)
{
	m_bNukesValid = bValid;
}

bool CvGame::isInAdvancedStart() const
{
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		if ((GET_PLAYER((PlayerTypes)iPlayer).getAdvancedStartPoints() >= 0) && GET_PLAYER((PlayerTypes)iPlayer).isHuman())
		{
			return true;
		}
	}

	return false;
}

void CvGame::setVoteChosen(int iSelection, int iVoteId)
{
	VoteSelectionData* pVoteSelectionData = getVoteSelection(iVoteId);
	if (NULL != pVoteSelectionData)
	{
		addVoteTriggered(*pVoteSelectionData, iSelection);
	}

	deleteVoteSelection(iVoteId);
}


CvCity* CvGame::getHolyCity(ReligionTypes eIndex)
{
// K-Mod
	/* original bts code
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumReligionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	*/
	FASSERT_BOUNDS(0, GC.getNumReligionInfos(), eIndex, "CvGame::getHolyCity");
// K-Mod end
	return getCity(m_paHolyCity[eIndex]);
}


void CvGame::setHolyCity(ReligionTypes eIndex, CvCity* pNewValue, bool bAnnounce)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumReligionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	CvCity* pOldValue = getHolyCity(eIndex);

	if(pOldValue == pNewValue)
		return;

	// religion visibility now part of espionage
	//updateCitySight(false, true);

	if (pNewValue != NULL)
	{
		m_paHolyCity[eIndex] = pNewValue->getIDInfo();
	}
	else
	{
		m_paHolyCity[eIndex].reset();
	}

	// religion visibility now part of espionage
	//updateCitySight(true, true);

	if (pOldValue != NULL)
	{
		pOldValue->changeReligionInfluence(eIndex, -(GC.getDefineINT("HOLY_CITY_INFLUENCE")));

		pOldValue->updateReligionCommerce();

		pOldValue->setInfoDirty(true);
	}

	if (getHolyCity(eIndex) != NULL)
	{
		CvCity* pHolyCity = getHolyCity(eIndex);

		pHolyCity->setHasReligion(eIndex, true, bAnnounce, true);
		pHolyCity->changeReligionInfluence(eIndex, GC.getDefineINT("HOLY_CITY_INFLUENCE"));

		pHolyCity->updateReligionCommerce();

		pHolyCity->setInfoDirty(true);

		if (bAnnounce)
		{
			if (isFinalInitialized() && !(gDLL->GetWorldBuilderMode()))
			{
				CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_REL_FOUNDED", GC.getReligionInfo(eIndex).getTextKeyWide(), pHolyCity->getNameKey());
				addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, pHolyCity->getOwner(), szBuffer, pHolyCity->getX(), pHolyCity->getY());
						// advc.106: Reserve this color for treaties
						//(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));

				for (int iI = 0; iI < MAX_PLAYERS; iI++)
				{
					CvPlayer const& kObs = GET_PLAYER((PlayerTypes)iI);
					if (!kObs.isAlive())
						continue; // advc.003
					if (pHolyCity->isRevealed(kObs.getTeam(), false)
							|| kObs.isSpectator()) // advc.127
					{
						szBuffer = gDLL->getText("TXT_KEY_MISC_REL_FOUNDED", GC.getReligionInfo(eIndex).getTextKeyWide(), pHolyCity->getNameKey());
						gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(), false, GC.getDefineINT("EVENT_MESSAGE_TIME_LONG"), szBuffer, GC.getReligionInfo(eIndex).getSound(), MESSAGE_TYPE_MAJOR_EVENT, GC.getReligionInfo(eIndex).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"), pHolyCity->getX(), pHolyCity->getY(), false, true);
					}
					else
					{
						szBuffer = gDLL->getText("TXT_KEY_MISC_REL_FOUNDED_UNKNOWN", GC.getReligionInfo(eIndex).getTextKeyWide());
						gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(), false, GC.getDefineINT("EVENT_MESSAGE_TIME_LONG"), szBuffer, GC.getReligionInfo(eIndex).getSound(), MESSAGE_TYPE_MAJOR_EVENT, GC.getReligionInfo(eIndex).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
					}
				}
			}
		}
	}

	AI_makeAssignWorkDirty();
}


CvCity* CvGame::getHeadquarters(CorporationTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCorporationInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return getCity(m_paHeadquarters[eIndex]);
}


void CvGame::setHeadquarters(CorporationTypes eIndex, CvCity* pNewValue, bool bAnnounce)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCorporationInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	CvCity* pOldValue = getHeadquarters(eIndex);

	if (pOldValue != pNewValue)
	{
		if (pNewValue != NULL)
		{
			m_paHeadquarters[eIndex] = pNewValue->getIDInfo();
		}
		else
		{
			m_paHeadquarters[eIndex].reset();
		}

		if (pOldValue != NULL)
		{
			pOldValue->updateCorporation();

			pOldValue->setInfoDirty(true);
		}

		CvCity* pHeadquarters = getHeadquarters(eIndex);

		if (NULL != pHeadquarters)
		{
			pHeadquarters->setHasCorporation(eIndex, true, bAnnounce);
			pHeadquarters->updateCorporation();
			pHeadquarters->setInfoDirty(true);

			if (bAnnounce)
			{
				if (isFinalInitialized() && !(gDLL->GetWorldBuilderMode()))
				{
					CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_CORPORATION_FOUNDED", GC.getCorporationInfo(eIndex).getTextKeyWide(), pHeadquarters->getNameKey());
					addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, pHeadquarters->getOwner(), szBuffer, pHeadquarters->getX(), pHeadquarters->getY(), (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));

					for (int iI = 0; iI < MAX_PLAYERS; iI++)
					{
						if (GET_PLAYER((PlayerTypes)iI).isAlive())
						{
							if (pHeadquarters->isRevealed(GET_PLAYER((PlayerTypes)iI).getTeam(), false))
							{
								gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), false, GC.getDefineINT("EVENT_MESSAGE_TIME_LONG"), szBuffer, GC.getCorporationInfo(eIndex).getSound(), MESSAGE_TYPE_MAJOR_EVENT, GC.getCorporationInfo(eIndex).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"), pHeadquarters->getX(), pHeadquarters->getY(), false, true);
							}
							else
							{
								CvWString szBuffer2 = gDLL->getText("TXT_KEY_MISC_CORPORATION_FOUNDED_UNKNOWN", GC.getCorporationInfo(eIndex).getTextKeyWide());
								gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), false, GC.getDefineINT("EVENT_MESSAGE_TIME_LONG"), szBuffer2, GC.getCorporationInfo(eIndex).getSound(), MESSAGE_TYPE_MAJOR_EVENT, GC.getCorporationInfo(eIndex).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
							}
						}
					}
				}
			}
		}

		AI_makeAssignWorkDirty();
	}
}


PlayerVoteTypes CvGame::getPlayerVote(PlayerTypes eOwnerIndex, int iVoteId) const
{
	FAssert(eOwnerIndex >= 0);
	FAssert(eOwnerIndex < MAX_CIV_PLAYERS);
	FAssert(NULL != getVoteTriggered(iVoteId));

	return GET_PLAYER(eOwnerIndex).getVote(iVoteId);
}


void CvGame::setPlayerVote(PlayerTypes eOwnerIndex, int iVoteId, PlayerVoteTypes eNewValue)
{
	FAssert(eOwnerIndex >= 0);
	FAssert(eOwnerIndex < MAX_CIV_PLAYERS);
	FAssert(NULL != getVoteTriggered(iVoteId));

	GET_PLAYER(eOwnerIndex).setVote(iVoteId, eNewValue);
}


void CvGame::castVote(PlayerTypes eOwnerIndex, int iVoteId, PlayerVoteTypes ePlayerVote)
{
	VoteTriggeredData* pTriggeredData = getVoteTriggered(iVoteId);
	if (NULL != pTriggeredData)
	{
		CvVoteInfo& kVote = GC.getVoteInfo(pTriggeredData->kVoteOption.eVote);
		if (kVote.isAssignCity())
		{
			FAssert(pTriggeredData->kVoteOption.ePlayer != NO_PLAYER);
			CvPlayer& kCityPlayer = GET_PLAYER(pTriggeredData->kVoteOption.ePlayer);

			if (GET_PLAYER(eOwnerIndex).getTeam() != kCityPlayer.getTeam())
			{
				switch (ePlayerVote)
				{
				/*  advc.130j (comment): Leave these alone for now. Should be based
					on the number of votes cast. */
				case PLAYER_VOTE_YES:
					kCityPlayer.AI_changeMemoryCount(eOwnerIndex, MEMORY_VOTED_AGAINST_US, 1);
					break;
				case PLAYER_VOTE_NO:
					kCityPlayer.AI_changeMemoryCount(eOwnerIndex, MEMORY_VOTED_FOR_US, 1);
					break;
				default:
					break;
				}
			}
		}
		else if (isTeamVote(pTriggeredData->kVoteOption.eVote))
		{
			if ((PlayerVoteTypes)GET_PLAYER(eOwnerIndex).getTeam() != ePlayerVote)
			{
				for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
				{
					CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
					if (kLoopPlayer.isAlive())
					{
						if (kLoopPlayer.getTeam() != GET_PLAYER(eOwnerIndex).getTeam() && kLoopPlayer.getTeam() == (TeamTypes)ePlayerVote)
						{
							/*  advc.130j (comment): Should not happen if there was
								only one name on the ballot. (Tbd.) */
							kLoopPlayer.AI_changeMemoryCount(eOwnerIndex, MEMORY_VOTED_FOR_US, 1);
						}
					}
				}
			}
		}

		setPlayerVote(eOwnerIndex, iVoteId, ePlayerVote);
	}
}


std::string CvGame::getScriptData() const
{
	return m_szScriptData;
}


void CvGame::setScriptData(std::string szNewValue)
{
	m_szScriptData = szNewValue;
}

const CvWString & CvGame::getName()
{
	return GC.getInitCore().getGameName();
}


void CvGame::setName(const TCHAR* szName)
{
	GC.getInitCore().setGameName(szName);
}


bool CvGame::isDestroyedCityName(CvWString& szName) const
{
	std::vector<CvWString>::const_iterator it;

	for (it = m_aszDestroyedCities.begin(); it != m_aszDestroyedCities.end(); it++)
	{
		if (*it == szName)
		{
			return true;
		}
	}

	return false;
}

void CvGame::addDestroyedCityName(const CvWString& szName)
{
	m_aszDestroyedCities.push_back(szName);
}

bool CvGame::isGreatPersonBorn(CvWString& szName) const
{
	std::vector<CvWString>::const_iterator it;

	for (it = m_aszGreatPeopleBorn.begin(); it != m_aszGreatPeopleBorn.end(); it++)
	{
		if (*it == szName)
		{
			return true;
		}
	}

	return false;
}

void CvGame::addGreatPersonBornName(const CvWString& szName)
{
	m_aszGreatPeopleBorn.push_back(szName);
}


// Protected Functions...

// K-Mod note: I've made some unmarked style adjustments to this function.
void CvGame::doTurn()
{
	PROFILE_BEGIN("CvGame::doTurn()");

	// END OF TURN
	if(!CvPlot::isAllFog()) // advc.706: Suppress popups
		CvEventReporter::getInstance().beginGameTurn(getGameTurn());

	doUpdateCacheOnTurn();

	updateScore();

	doDeals();

	/* original bts code
	for (iI = 0; iI < MAX_TEAMS; iI++) {
		if (GET_TEAM((TeamTypes)iI).isAlive())
			GET_TEAM((TeamTypes)iI).doTurn();
	} */ // disabled by K-Mod. CvTeam::doTurn is now called at the the same time as CvPlayer::doTurn, to fix certain turn-order imbalances.

	GC.getMap().doTurn();

	createBarbarianCities();

	createBarbarianUnits();

	doGlobalWarming();

	doHolyCity();

	doHeadquarters();

	gDLL->getInterfaceIFace()->setEndTurnMessage(false);
	gDLL->getInterfaceIFace()->setHasMovedUnit(false);

	if (getAIAutoPlay() > 0)
	{	/*  <advc.127> Flag added: don't change player status when decrementing
			the counter at the start of a round. Let onEndPlayerTurn in AIAutoPlay.py
			handle it. (Because human control should resume right before the human
			turn, which is not necessarily at the beginning of a round.) */
		changeAIAutoPlay(-1, false);
		if(getAIAutoPlay() > 0)
			checkInSync(); // May set AutoPlay counter to 0
		// </advc.127>
		if (getAIAutoPlay() == 0)
			reviveActivePlayer();
	}

	CvEventReporter::getInstance().endGameTurn(getGameTurn());

	incrementGameTurn();
	incrementElapsedGameTurns();
	/*  advc.004: Already done in doDeals, but that's before incrementing the
		turn counter. Want to kill peace treaties asap. */
	verifyDeals();
	// <advc.700>
	if(isOption(GAMEOPTION_RISE_FALL))
		m_pRiseFall->atGameTurnStart(); // </advc.700>
	// advc.127: was right after doHeadquarters
	doDiploVote();

	if (isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
	{
		int aiShuffle[MAX_PLAYERS];

		shuffleArray(aiShuffle, MAX_PLAYERS, getSorenRand());
		std::set<TeamTypes> active_teams; // K-Mod.

		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			PlayerTypes eLoopPlayer = (PlayerTypes)aiShuffle[iI];

			CvPlayer& kLoopPlayer = GET_PLAYER(eLoopPlayer);
			if (kLoopPlayer.isAlive())
			{
				// K-Mod. call CvTeam::doTurn when the first player from each team is activated.
				if (active_teams.insert(kLoopPlayer.getTeam()).second)
					GET_TEAM(kLoopPlayer.getTeam()).doTurn();
				// K-Mod end
				kLoopPlayer.setTurnActive(true);
			}
		}
	}
	else if (isSimultaneousTeamTurns())
	{
		for (int iI = 0; iI < MAX_TEAMS; iI++)
		{
			CvTeam& kTeam = GET_TEAM((TeamTypes)iI);
			if (kTeam.isAlive())
			{
				kTeam.setTurnActive(true);
				FAssert(getNumGameTurnActive() == kTeam.getAliveCount());
				break;
			}
		}
	}
	else
	{
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (isPbem() && GET_PLAYER((PlayerTypes)iI).isHuman())
				{
					if (iI == getActivePlayer())
					{
						// Nobody else left alive
						GC.getInitCore().setType(GAME_HOTSEAT_NEW);
						GET_PLAYER((PlayerTypes)iI).setTurnActive(true);
					}
					else if (!getPbemTurnSent())
					{
						gDLL->sendPbemTurn((PlayerTypes)iI);
					}
				}
				else
				{
					GET_PLAYER((PlayerTypes)iI).setTurnActive(true);
					FAssert(getNumGameTurnActive() == 1);
				}

				break;
			}
		}
	}

	testVictory();

	gDLL->getEngineIFace()->SetDirty(GlobePartialTexture_DIRTY_BIT, true);
	gDLL->getEngineIFace()->DoTurn();

	PROFILE_END();

	stopProfilingDLL(true);
	// <advc.700>
	if(isOption(GAMEOPTION_RISE_FALL))
		m_pRiseFall->autoSave();
	else // </advc.700>
		autoSave(); // advc.106l
}

// <advc.106b>
bool CvGame::isInBetweenTurns() const {

	return m_bInBetweenTurns;
}

void CvGame::setInBetweenTurns(bool b) {

	m_bInBetweenTurns = b;
} // </advc.106b>


void CvGame::doDeals()
{
	CvDeal* pLoopDeal;
	int iLoop;

	verifyDeals();

	std::set<PlayerTypes> trade_players; // K-Mod. List of players involved in trades.
	for(pLoopDeal = firstDeal(&iLoop); pLoopDeal != NULL; pLoopDeal = nextDeal(&iLoop))
	{
		// K-Mod
		trade_players.insert(pLoopDeal->getFirstPlayer());
		trade_players.insert(pLoopDeal->getSecondPlayer());
		// K-Mod end
		pLoopDeal->doTurn();
	}

	// K-Mod. Update the attitude cache for all trade players
	for (std::set<PlayerTypes>::iterator it = trade_players.begin(); it != trade_players.end(); ++it)
	{
		FAssert(*it != NO_PLAYER);
		GET_PLAYER(*it).AI_updateAttitudeCache();
	}
	// K-Mod end
}

/*  K-Mod, 5/dec/10, karadoc
	complete rewrite of global warming, using some features from 'GWMod' by M.A. */
void CvGame::doGlobalWarming()
{
	PROFILE_FUNC();
	/*
	** Calculate change in GW index
	*/
	int iGlobalWarmingValue = calculateGlobalPollution();

	int iGlobalWarmingDefense = calculateGwSustainabilityThreshold(); // Natural global defence
	iGlobalWarmingDefense+= calculateGwLandDefence(); // defence from features (forests & jungles)

	changeGlobalWarmingIndex(iGlobalWarmingValue - iGlobalWarmingDefense);

	// check if GW has 'activated'.
	if (getGwEventTally() < 0 && getGlobalWarmingIndex() > 0)
	{
		setGwEventTally(0);

		// Send a message saying that the threshold has been passed
		CvWString szBuffer;

		szBuffer = gDLL->getText("TXT_KEY_MISC_GLOBAL_WARMING_ACTIVE");
		// add the message to the replay
		addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, NO_PLAYER, szBuffer, -1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));

		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_GLOBALWARMING", MESSAGE_TYPE_MAJOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
			}

			// Tell human players that the threshold has been reached
			if (GET_PLAYER((PlayerTypes)iI).isHuman() && !isNetworkMultiPlayer())
			{
				CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_TEXT);
				if (pInfo != NULL)
				{
					pInfo->setText(gDLL->getText("TXT_KEY_POPUP_ENVIRONMENTAL_ADVISOR"));
					gDLL->getInterfaceIFace()->addPopup(pInfo, (PlayerTypes)iI);
				}
			}
		}

	}

	/*
	** Apply the effects of GW
	*/
	int iGlobalWarmingRolls = getGlobalWarmingChances();

	TerrainTypes eWarmingTerrain = ((TerrainTypes)(GC.getDefineINT("GLOBAL_WARMING_TERRAIN")));
	TerrainTypes eFrozenTerrain = ((TerrainTypes)(GC.getDefineINT("FROZEN_TERRAIN")));
	TerrainTypes eColdTerrain = ((TerrainTypes)(GC.getDefineINT("COLD_TERRAIN")));
	TerrainTypes eTemperateTerrain = ((TerrainTypes)(GC.getDefineINT("TEMPERATE_TERRAIN")));
	TerrainTypes eDryTerrain = ((TerrainTypes)(GC.getDefineINT("DRY_TERRAIN")));
	TerrainTypes eBarrenTerrain = ((TerrainTypes)(GC.getDefineINT("BARREN_TERRAIN")));

	FeatureTypes eColdFeature = ((FeatureTypes)(GC.getDefineINT("COLD_FEATURE")));
	FeatureTypes eTemperateFeature = ((FeatureTypes)(GC.getDefineINT("TEMPERATE_FEATURE")));
	FeatureTypes eWarmFeature = ((FeatureTypes)(GC.getDefineINT("WARM_FEATURE")));
	FeatureTypes eFalloutFeature = ((FeatureTypes)(GC.getDefineINT("NUKE_FEATURE")));

	//Global Warming
	for (int iI = 0; iI < iGlobalWarmingRolls; iI++)
	{
		// note, warming prob out of 1000, not percent.
		int iLeftOdds = 10*GC.getGameSpeedInfo(getGameSpeedType()).getVictoryDelayPercent();
		if (getSorenRandNum(iLeftOdds, "Global Warming") < GC.getDefineINT("GLOBAL_WARMING_PROB"))
		{
			//CvPlot* pPlot = GC.getMap().syncRandPlot(RANDPLOT_LAND | RANDPLOT_NOT_CITY);

			// Global warming is no longer completely random. getRandGWPlot will get a weighted random plot for us to strike
			CvPlot* pPlot = getRandGWPlot(3);

			if (pPlot != NULL)
			{
				bool bChanged = false;
				/*
				** rewritten terrain changing code:
				*/
				// 1) Melt frozen terrain
				if (pPlot->getFeatureType() == eColdFeature)
				{
					pPlot->setFeatureType(NO_FEATURE);
					bChanged = true;
				}
				else if (pPlot->getTerrainType() == eFrozenTerrain)
				{
						pPlot->setTerrainType(eColdTerrain);
						bChanged = true;
				}
				else if (pPlot->getTerrainType() == eColdTerrain)
				{
					pPlot->setTerrainType(eTemperateTerrain);
					bChanged = true;
				}
				// 2) Forest -> Jungle
				// advc.055: Commented out
				/*else if (pPlot->getFeatureType() == eTemperateFeature) {
					pPlot->setFeatureType(eWarmFeature);
					bChanged = true;
				}*/
				// 3) Remove other features
				else if (pPlot->getFeatureType() != NO_FEATURE && pPlot->getFeatureType() != eFalloutFeature)
				{
					pPlot->setFeatureType(NO_FEATURE);
					bChanged = true;
				}
				// 4) Dry the terrain
				// Rising seas
				else if (pPlot->getTerrainType() == eTemperateTerrain)
				{
					pPlot->setTerrainType(eDryTerrain);
					bChanged = true;
				}
				else if (pPlot->getTerrainType() == eDryTerrain)
				{
					pPlot->setTerrainType(eBarrenTerrain);
					bChanged = true;
				}
				/* 5) Sink coastal desert (disabled)
				else if (pPlot->getTerrainType() == eBarrenTerrain) {
					if (isOption(GAMEOPTION_RISING_SEAS)) {
						if (pPlot->isCoastalLand()) {
							if (!pPlot->isHills() && !pPlot->isPeak()) {
								pPlot->forceBumpUnits();
								pPlot->setPlotType(PLOT_OCEAN);
								bChanged = true;
				} } } }*/

				if (bChanged)
				{
					// only destroy the improvement if the new terrain cannot support it
					if (!pPlot->canHaveImprovement(pPlot->getImprovementType()),
							NO_BUILD, false) // dlph.9
						pPlot->setImprovementType(NO_IMPROVEMENT);

					CvCity* pCity = GC.getMap().findCity(pPlot->getX(), pPlot->getY(), NO_PLAYER, NO_TEAM, false);
					if (pCity != NULL)
					{
						if (pPlot->isVisible(pCity->getTeam(), false))
						{
							CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_GLOBAL_WARMING_NEAR_CITY", pCity->getNameKey());
							gDLL->getInterfaceIFace()->addHumanMessage(pCity->getOwner(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_SQUISH", MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX(), pPlot->getY(), true, true);
						}
					}
					changeGwEventTally(1);
				}
			}
		}
	}
	updateGwPercentAnger();
	if (getGlobalWarmingIndex() > 0)
	{
		changeGlobalWarmingIndex(-getGlobalWarmingIndex()*GC.getDefineINT("GLOBAL_WARMING_RESTORATION_RATE", 0)/100);
	}
}

// Choose the best plot for global warming to strike from a set of iPool random plots
CvPlot* CvGame::getRandGWPlot(int iPool)
{
	CvPlot* pBestPlot = NULL;
	CvPlot* pTestPlot = NULL;
	TerrainTypes eTerrain = NO_TERRAIN;
	int iBestScore = -1; // higher score means better target plot
	int iTestScore;
	int i;

	const TerrainTypes eFrozenTerrain = ((TerrainTypes)(GC.getDefineINT("FROZEN_TERRAIN")));
	const TerrainTypes eColdTerrain = ((TerrainTypes)(GC.getDefineINT("COLD_TERRAIN")));
	const TerrainTypes eTemperateTerrain = ((TerrainTypes)(GC.getDefineINT("TEMPERATE_TERRAIN")));
	const TerrainTypes eDryTerrain = ((TerrainTypes)(GC.getDefineINT("DRY_TERRAIN")));

	const FeatureTypes eColdFeature = ((FeatureTypes)(GC.getDefineINT("COLD_FEATURE")));

	// Currently we just choose the coldest tile; but I may include other tests in future versions
	for (i = 0; i < iPool; i++)
	{
		// I want to be able to select a water tile with ice on it; so I can't just exclude water completely...
		//CvPlot* pTestPlot = GC.getMap().syncRandPlot(RANDPLOT_LAND | RANDPLOT_NOT_CITY);
		int j;
		for (j = 0; j < 100; j++)
		{
			pTestPlot = GC.getMap().syncRandPlot(RANDPLOT_NOT_CITY);

			if (pTestPlot == NULL)
				break; // already checked 100 plots in the syncRandPlot funciton, so just give up.

			// check for ice
			if (pTestPlot->getFeatureType() == eColdFeature)
			{
				// pretend it's frozen terrain
				eTerrain = eFrozenTerrain;
				break;
			}
			// check for ordinary land plots
			if (!pTestPlot->isWater() && !pTestPlot->isPeak())
			{
				eTerrain = pTestPlot->getTerrainType();
				break;
			}
			// not a suitable plot, try again.
		}

		if (pTestPlot == NULL || j == 100)
			continue;

		// if only I could do this with a switch...

		if (eTerrain == eFrozenTerrain)
			iTestScore = 4;
		else if (eTerrain == eColdTerrain)
			iTestScore = 3;
		else if (eTerrain == eTemperateTerrain)
			iTestScore = 2;
		else if (eTerrain == eDryTerrain)
			iTestScore = 1;
		else
			iTestScore = 0;

		if (iTestScore > iBestScore)
		{
			if (iBestScore > 0 || iTestScore >= 3)
				return pTestPlot; // lets not target the ice too much...

			pBestPlot = pTestPlot;
			iBestScore = iTestScore;
		}
	}
	return pBestPlot;
}
/*
** K-Mod end
*/


void CvGame::doHolyCity()
{
	PlayerTypes eBestPlayer;
	TeamTypes eBestTeam;
	int iValue;
	int iBestValue;
	int iI, iJ, iK;

	long lResult = 0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "doHolyCity", NULL, &lResult);
	if (lResult == 1)
	{
		return;
	}

	if (getElapsedGameTurns() < 5 && !isOption(GAMEOPTION_ADVANCED_START))
	{
		return;
	}

	int iRandOffset = getSorenRandNum(GC.getNumReligionInfos(), "Holy City religion offset");
	for (int iLoop = 0; iLoop < GC.getNumReligionInfos(); ++iLoop)
	{
		iI = ((iLoop + iRandOffset) % GC.getNumReligionInfos());

		if (!isReligionSlotTaken((ReligionTypes)iI))
		{
			iBestValue = MAX_INT;
			eBestTeam = NO_TEAM;

			/*  advc.001: Was MAX_TEAMS. Make sure barbs can't found a religion
				somehow. Adopted from Mongoose SDK ReligionMod. */
			for (iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
			{
				if (GET_TEAM((TeamTypes)iJ).isAlive())
				{
					if (GET_TEAM((TeamTypes)iJ).isHasTech((TechTypes)(GC.getReligionInfo((ReligionTypes)iI).getTechPrereq())))
					{
						if (GET_TEAM((TeamTypes)iJ).getNumCities() > 0)
						{
							iValue = getSorenRandNum(10, "Found Religion (Team)");

							for (iK = 0; iK < GC.getNumReligionInfos(); iK++)
							{
								int iReligionCount = GET_TEAM((TeamTypes)iJ).getHasReligionCount((ReligionTypes)iK);

								if (iReligionCount > 0)
								{
									iValue += iReligionCount * 20;
								}
							}
							// advc.138:
							iValue -= religionPriority((TeamTypes)iJ, (ReligionTypes)iI);

							if (iValue < iBestValue)
							{
								iBestValue = iValue;
								eBestTeam = ((TeamTypes)iJ);
							}
						}
					}
				}
			}

			if (eBestTeam != NO_TEAM)
			{
				iBestValue = MAX_INT;
				eBestPlayer = NO_PLAYER;

				for (iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isAlive())
					{
						if (GET_PLAYER((PlayerTypes)iJ).getTeam() == eBestTeam)
						{
							if (GET_PLAYER((PlayerTypes)iJ).getNumCities() > 0)
							{
								iValue = getSorenRandNum(10, "Found Religion (Player)");

								if (!GET_PLAYER((PlayerTypes)iJ).isHuman())
								{   // advc.138: Was 10. Need some x: 15 < x < 20.
									iValue += 18;
								}

								for (iK = 0; iK < GC.getNumReligionInfos(); iK++)
								{
									int iReligionCount = GET_PLAYER((PlayerTypes)iJ).getHasReligionCount((ReligionTypes)iK);

									if (iReligionCount > 0)
									{
										iValue += iReligionCount * 20;
									}
								}
								// advc.138:
								iValue -= religionPriority((PlayerTypes)iJ, (ReligionTypes)iI);

								if (iValue < iBestValue)
								{
									iBestValue = iValue;
									eBestPlayer = ((PlayerTypes)iJ);
								}
							}
						}
					}
				}

				if (eBestPlayer != NO_PLAYER)
				{
					ReligionTypes eReligion = (ReligionTypes)iI;

					if (isOption(GAMEOPTION_PICK_RELIGION))
					{
						eReligion = GET_PLAYER(eBestPlayer).AI_chooseReligion();
					}

					if (NO_RELIGION != eReligion)
					{
						GET_PLAYER(eBestPlayer).foundReligion(eReligion, (ReligionTypes)iI, false);
					}
				}
			}
		}
	}
}

// <advc.138>
int CvGame::religionPriority(TeamTypes eTeam, ReligionTypes eReligion) const {

	int iMembers = 0;
	int r = 0;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		if(i != eTeam || !GET_PLAYER((PlayerTypes)i).isAlive())
			continue;
		iMembers++;
		r += religionPriority(GET_PLAYER((PlayerTypes)i).getID(), eReligion);
	}
	if(iMembers == 0)
		return 0;
	return r / iMembers;
}


int CvGame::religionPriority(PlayerTypes ePlayer, ReligionTypes eReligion) const {

	int r = 0;
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	for(int i = 0; i < GC.getNumTraitInfos(); i++) {
		TraitTypes eNoAnarchyTrait = (TraitTypes)i;
		if(!kPlayer.hasTrait(eNoAnarchyTrait) ||
				GC.getTraitInfo(eNoAnarchyTrait).getMaxAnarchy() != 0)
			continue;
		r += 5;
		/*  Spiritual human should be sure to get a religion (so long as
			difficulty isn't above Noble). Not quite sure if my choice of
			numbers in this function and in doHolyCity accomplishes that. */
		if(kPlayer.isHuman())
			r += 6;
		break;
	}
	r += ((100 - GC.getHandicapInfo(kPlayer.getHandicapType()).
			getStartingLocationPercent()) * 31) / 100;
	// With the pick-rel option, eReligion will change later on anyway.
	if(!isOption(GAMEOPTION_PICK_RELIGION)) {
		/*  Not excluding human here means that choosing a leader with an early
			fav religion can make a difference in human getting a religion.
			Unexpected, as fav. religions are pretty obscure knowledge. On the
			other hand, it's a pity to assign human an arbitrary religion when
			e.g. Buddhism would fit so well for Ashoka.
			Don't use PersonalityType here; fav. religion is always a matter
			of LeaderType. */
		if(GC.getLeaderHeadInfo(kPlayer.getLeaderType()).getFavoriteReligion() == eReligion)
			r += 6;
	}
	return r;
}
// </advc.138>

void CvGame::doHeadquarters()
{
	long lResult = 0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "doHeadquarters", NULL, &lResult);
	if (lResult == 1)
	{
		return;
	}

	if (getElapsedGameTurns() < 5)
	{
		return;
	}

	for (int iI = 0; iI < GC.getNumCorporationInfos(); iI++)
	{
		CvCorporationInfo& kCorporation = GC.getCorporationInfo((CorporationTypes)iI);
		if (!isCorporationFounded((CorporationTypes)iI))
		{
			int iBestValue = MAX_INT;
			TeamTypes eBestTeam = NO_TEAM;

			for (int iJ = 0; iJ < MAX_TEAMS; iJ++)
			{
				CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iJ);
				if (kLoopTeam.isAlive())
				{
					if (NO_TECH != kCorporation.getTechPrereq() && kLoopTeam.isHasTech((TechTypes)(kCorporation.getTechPrereq())))
					{
						if (kLoopTeam.getNumCities() > 0)
						{
							bool bHasBonus = false;
							for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
							{
								if (NO_BONUS != kCorporation.getPrereqBonus(i) && kLoopTeam.hasBonus((BonusTypes)kCorporation.getPrereqBonus(i)))
								{
									bHasBonus = true;
									break;
								}
							}

							if (bHasBonus)
							{
								int iValue = getSorenRandNum(10, "Found Corporation (Team)");

								for (int iK = 0; iK < GC.getNumCorporationInfos(); iK++)
								{
									int iCorporationCount = GET_PLAYER((PlayerTypes)iJ).getHasCorporationCount((CorporationTypes)iK);

									if (iCorporationCount > 0)
									{
										iValue += iCorporationCount * 20;
									}
								}

								if (iValue < iBestValue)
								{
									iBestValue = iValue;
									eBestTeam = ((TeamTypes)iJ);
								}
							}
						}
					}
				}
			}

			if (eBestTeam != NO_TEAM)
			{
				iBestValue = MAX_INT;
				PlayerTypes eBestPlayer = NO_PLAYER;

				for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iJ);
					if (kLoopPlayer.isAlive())
					{
						if (kLoopPlayer.getTeam() == eBestTeam)
						{
							if (kLoopPlayer.getNumCities() > 0)
							{
								bool bHasBonus = false;
								for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
								{
									if (NO_BONUS != kCorporation.getPrereqBonus(i) && kLoopPlayer.hasBonus((BonusTypes)kCorporation.getPrereqBonus(i)))
									{
										bHasBonus = true;
										break;
									}
								}

								if (bHasBonus)
								{
									int iValue = getSorenRandNum(10, "Found Religion (Player)");

									if (!kLoopPlayer.isHuman())
									{
										iValue += 10;
									}

									for (int iK = 0; iK < GC.getNumCorporationInfos(); iK++)
									{
										int iCorporationCount = GET_PLAYER((PlayerTypes)iJ).getHasCorporationCount((CorporationTypes)iK);

										if (iCorporationCount > 0)
										{
											iValue += iCorporationCount * 20;
										}
									}

									if (iValue < iBestValue)
									{
										iBestValue = iValue;
										eBestPlayer = ((PlayerTypes)iJ);
									}
								}
							}
						}
					}
				}

				if (eBestPlayer != NO_PLAYER)
				{
					GET_PLAYER(eBestPlayer).foundCorporation((CorporationTypes)iI);
				}
			}
		}
	}
}


void CvGame::doDiploVote()
{
	doVoteResults();

	doVoteSelection();
}


void CvGame::createBarbarianCities()  // advc.003 some style changes
{
	if (getMaxCityElimination() > 0)
		return;

	if (isOption(GAMEOPTION_NO_BARBARIANS))
		return;

	long lResult = 0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "createBarbarianCities", NULL, &lResult);
	if (lResult == 1)
		return;

	if (GC.getEraInfo(getCurrentEra()).isNoBarbCities())
		return;

	CvHandicapInfo const& kGameHandicap = GC.getHandicapInfo(getHandicapType());
	if (kGameHandicap.getUnownedTilesPerBarbarianCity() <= 0)
		return;

	if (getNumCivCities() < countCivPlayersAlive() * 2)
			return;

	if (getElapsedGameTurns() <= ((kGameHandicap.getBarbarianCityCreationTurnsElapsed() *
			GC.getGameSpeedInfo(getGameSpeedType()).getBarbPercent()) / 100) /
			std::max(getStartEra() + 1, 1))
		return;

	/* <advc.300> Create up to two cities per turn, though at most one in an
	   area settled by a civ. Moved the rest of createBarbarianCities (plural)
	   into new function createBarbarianCity (singular). */
	createBarbarianCity(false);
	// A second city at full probability is too much; try 50%.
	createBarbarianCity(true, 50);
}


void CvGame::createBarbarianCity(bool bSkipCivAreas, int iProbModifierPercent) {

	int iCreationProb = GC.getHandicapInfo(getHandicapType()).getBarbarianCityCreationProb();
	/* No cities past Medieval, so it's either +0 (Ancient), +1 (Classical)
	   or +4 (Medieval). */
	int iEra = getCurrentEra();
	iCreationProb += iEra * iEra;
	iCreationProb *= iProbModifierPercent;
	iCreationProb /= 100;
	// Adjust creation prob to game speed
	CvGameSpeedInfo const& kSpeed = GC.getGameSpeedInfo(getGameSpeedType());
	iCreationProb *= kSpeed.getBarbPercent();
	iCreationProb /= 100;
	if(getSorenRandNum(100, "Barb City Creation") >= iCreationProb) // </advc.300>
		return;

	/*  advc (comment): This multiplier expresses how close the total number of
		Barbarian cities is to the global target. In contrast, iTargetCities
		in the loop is per area, not global.
		It's apparently a kind of percentage. If above 100, i.e. 100+x, it seems
		to mean that x% more barb cities are needed.
		The global target is between 20% and 40% of the number of civ cities,
		which is pretty ambitious. */
	int iTargetCitiesMultiplier = 100;
	{
		int iTargetBarbCities = (getNumCivCities() * 5 * GC.getHandicapInfo(getHandicapType()).getBarbarianCityCreationProb()) / 100;
		int iBarbCities = GET_PLAYER(BARBARIAN_PLAYER).getNumCities();
		if (iBarbCities < iTargetBarbCities)
			iTargetCitiesMultiplier += (300 * (iTargetBarbCities - iBarbCities)) / iTargetBarbCities;

		if (isOption(GAMEOPTION_RAGING_BARBARIANS))
		{
			iTargetCitiesMultiplier *= 3;
			iTargetCitiesMultiplier /= 2;
		}
	}

	CvPlayerAI::CvFoundSettings kFoundSet(GET_PLAYER(BARBARIAN_PLAYER), false); // K-Mod
	kFoundSet.iMinRivalRange = GC.getDefineINT("MIN_BARBARIAN_CITY_STARTING_DISTANCE");
	/* <advc.300> Randomize penalty on short inter-city distance for more variety
	   in Barbarian settling patterns. The expected value is 8, which is also the
	   value K-Mod uses. */
	kFoundSet.iBarbDiscouragedRange = 5 + getSorenRandNum(7, "advc.300 (discouraged range)");
	CvMap const& m = GC.getMap();
	// Precomputed for efficiency
	std::map<int,int> unownedPerArea; int foo=-1;
	for(CvArea* pArea = m.firstArea(&foo); pArea != NULL; pArea = m.nextArea(&foo)) {
		CvArea const& a = *pArea;
		/* Plots owned by Barbarians are counted in BtS, and I count them when
		   creating units because it makes some sense that Barbarians get fewer free
		   units once they have cities, but for cities, I'm not sure.
		   Keep counting them for now. */
		std::pair<int,int> iiOwnedUnowned = a.countOwnedUnownedHabitableTiles();
				//a.countOwnedUnownedHabitableTiles(true);
		int iUnowned = iiOwnedUnowned.second;
		std::vector<Shelf*> shelves;
		m.getShelves(a.getID(), shelves);
		for(size_t i = 0; i < shelves.size(); i++)
			iUnowned += shelves[i]->countUnownedPlots() / 2;
		unownedPerArea.insert(std::make_pair(a.getID(), iUnowned));
	}
	bool bRage = isOption(GAMEOPTION_RAGING_BARBARIANS);
	// </advc.300>

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	for (int iI = 0; iI < m.numPlots(); iI++)
	{
		CvPlot* pLoopPlot = m.plotByIndex(iI);
		if(pLoopPlot->isWater() || pLoopPlot->isVisibleToCivTeam())
			continue; // advc.003
		// <advc.300>
		CvArea& a = *pLoopPlot->area();
		int const iAreaSz = a.getNumTiles();
		bool bCivArea = (a.getNumCities() > a.getCitiesPerPlayer(BARBARIAN_PLAYER));
		if(bSkipCivAreas && bCivArea)
			continue;
		std::map<int,int>::const_iterator unowned = unownedPerArea.find(a.getID());
		FAssert(unowned != unownedPerArea.end());
		int iTargetCities = unowned->second;
		if(bRage) { // Didn't previously affect city density
			iTargetCities *= 7;
			iTargetCities /= 5;
		}
		if(!bCivArea) {
			/*  BtS triples iTargetCities here. Want to make it era-based.
				Important that the multiplier is rather small in the first four eras
				so that civs get a chance to settle small landmasses before
				Barbarians appear there. Once there is a Barbarian city on a
				small landmass, there may not be room for another city, and a
				naval attack on a Barbarian city is difficult to execute for the AI. */
			double mult = 0.5 + 0.88 * iEra;
			iTargetCities = ::round(mult * iTargetCities); // </advc.300>
		}
		int iUnownedTilesThreshold = GC.getHandicapInfo(getHandicapType()).getUnownedTilesPerBarbarianCity();
		if(iAreaSz < iUnownedTilesThreshold / 3) {
			iTargetCities *= iTargetCitiesMultiplier;
			iTargetCities /= 100;
		} // <advc.304>
		int iDestroyedCities = a.getBarbarianCitiesEverCreated() -
				a.getCitiesPerPlayer(BARBARIAN_PLAYER);
		FAssert(iDestroyedCities >= 0);
		iDestroyedCities = std::max(0, iDestroyedCities);
		iUnownedTilesThreshold += iDestroyedCities * 3; // </advc.304>
		iTargetCities /= std::max(1, iUnownedTilesThreshold);

		if (a.getCitiesPerPlayer(BARBARIAN_PLAYER) < iTargetCities)
		{
			//iValue = GET_PLAYER(BARBARIAN_PLAYER).AI_foundValue(pLoopPlot->getX(), pLoopPlot->getY(), GC.getDefineINT("MIN_BARBARIAN_CITY_STARTING_DISTANCE"));
			// K-Mod
			int iValue = GET_PLAYER(BARBARIAN_PLAYER).AI_foundValue_bulk(
					pLoopPlot->getX(), pLoopPlot->getY(), kFoundSet);
			if (iTargetCitiesMultiplier > 100)
			{/* <advc.300> This gives the area with the most owned tiles priority
				over other areas unless the global city target is reached (rare),
				or the most crowded area hasn't enough unowned tiles left.
				The idea of bSkipCivAreas is to settle terrae incognitae earlier,
				so I'm considerably reducing the impact in that case.
				Also, the first city placed in a previously uninhabited area is
				placed randomly b/c each found value gets multipied with 0,
				which is apparently a bug. Let's instead use the projected number
				of owned tiles after placing the city (by adding 9). */
				int iOwned = a.getNumOwnedTiles();
				if(bSkipCivAreas)
					iValue += iOwned;
				else iValue *= iOwned + 9; // advc.001 </advc.300>
			}
			/*  advc.300, advc.001: Looks like another bug; probably
				times 1 to 1.5 was intended. (Dividing by 100 doesn't affect
				pBestPlot, but I'd like to put iBestValue is on the scale of a
				regular found value - although it isn't used for anything.)
				This kind of randomization is mitigated by the fact that clusters
				of tiles tend to have similar found values. The effect is mostly
				local. I'm trying to get the Barbarians to also settle mediocre land
				occasionally by randomizing CvFoundSettings.iBarbDiscouragedRange above. */
			/*iValue += (100 + getSorenRandNum(50, "Barb City Found"));
			iValue /= 100;*/
			iValue *= (100 + getSorenRandNum(50, "Barb City Found")) / 100;
			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				pBestPlot = pLoopPlot;
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(iBestValue > 0); // advc.300
		GET_PLAYER(BARBARIAN_PLAYER).found(pBestPlot->getX(), pBestPlot->getY());
	}
}


void CvGame::createBarbarianUnits()
{
	if(isOption(GAMEOPTION_NO_BARBARIANS))
		return;

	long lResult = 0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "createBarbarianUnits", NULL, &lResult);
	if (lResult == 1)
		return;

	//if (GC.getEraInfo(getCurrentEra()).isNoBarbUnits()) ...
	bool bCreateBarbarians = isBarbarianCreationEra(); // advc.307 (checked later now)
	bool bAnimals = false;
	if (getNumCivCities() < (3 * countCivPlayersAlive()) / 2 &&
			!isOption(GAMEOPTION_ONE_CITY_CHALLENGE) &&
			/*  advc.300: No need to delay Barbarians (bAnimals=true) if they start
				slowly (PEAK_PERCENT>=35). For slow game speed settings, there is
				now a similar check in CvUnitAI::AI_barbAttackMove. */
			GC.getDefineINT("BARB_PEAK_PERCENT") < 35)
		bAnimals = true;
	// advc.300: Moved into new function
	if (getGameTurn() < getBarbarianStartTurn())
		bAnimals = true;

	if (bAnimals)
		createAnimals();

	// <advc.300>
	if(bAnimals)
		return;
	CvHandicapInfo const& kGameHandicap = GC.getHandicapInfo(getHandicapType());
	int iBaseTilesPerLandUnit = kGameHandicap.getUnownedTilesPerBarbarianUnit();
	// Divided by 10 b/c now only shelf water tiles count
	int iBaseTilesPerSeaUnit = kGameHandicap.getUnownedWaterTilesPerBarbarianUnit() / 8;
	// </advc.300>
	int iLoop;
	for(CvArea* pLoopArea = GC.getMap().firstArea(&iLoop); pLoopArea != NULL;
			pLoopArea = GC.getMap().nextArea(&iLoop)) {
		// <advc.300>
		CvArea& a = *pLoopArea;
		/*  For each land area, first spawn sea Barbarians for each shelf attached
			to that land area. Skip water areas entirely. Then spawn units in the
			land area. Shelves go first b/c units can now spawn in cargo;
			spawn fewer land units then. No units in unsettled areas.
			(Need to at least spawn a Barbarian city before that). */
		if(a.isWater() || a.getNumCities() == 0)
			continue;
		int iUnowned = 0, iTiles = 0;
		std::vector<Shelf*> shelves; GC.getMap().getShelves(a.getID(), shelves);
		for(size_t i = 0; i < shelves.size(); i++) {
			// Shelves also count for land Barbarians, ...
			iUnowned += shelves[i]->countUnownedPlots();
			iTiles += shelves[i]->size();
		}
		// ... but only half.
		iUnowned /= 2; iTiles /= 2;

		/*  For performance -- countOwnedUnownedHabitableTiles isn't cached;
			goes through the entire map for each land area, and archipelago-type maps
			can have a lot of those. */
		int iTotal = a.getNumTiles() + iTiles;
		int iUnownedTotal = a.getNumUnownedTiles() + iUnowned;
		if(iUnownedTotal >= iTotal)
			continue;

		/*  In the following, only care about "habitable" tiles, i.e. with a
			positive food yield (implied for shelf tiles).
			Should tiles visible to a civ count? Yes; it's not unrealistic that
			Barbarians originate in one (visible) place, and emerge as a threat
			in another (invisible) place. */
		std::pair<int,int> iiOwnedUnowned = a.countOwnedUnownedHabitableTiles();
		iUnowned += iiOwnedUnowned.second;
		iTiles += iiOwnedUnowned.first + iiOwnedUnowned.second;
		// NB: Animals are included in this count
		int iLandUnits = a.getUnitsPerPlayer(BARBARIAN_PLAYER);
		//  Kill a Barbarian unit if the area gets crowded
		if(killBarbarian(iLandUnits, iTiles, a.getPopulationPerPlayer(BARBARIAN_PLAYER),
				a, NULL))
			iLandUnits--;
		if(iUnownedTotal < iBaseTilesPerLandUnit / 2)
			continue;
		int iOwned = iTiles - iUnowned;
		int iBarbCities = a.getCitiesPerPlayer(BARBARIAN_PLAYER);
		int iNeededLand = numBarbariansToCreate(iBaseTilesPerLandUnit, iTiles,
				iUnowned, iLandUnits, iBarbCities);
		for(size_t i = 0; i < shelves.size(); i++) {
			int iShips = shelves[i]->countBarbarians();
			if(killBarbarian(iShips, shelves[i]->size(), a.getPopulationPerPlayer(BARBARIAN_PLAYER),
					a, shelves[i]))
				iShips--;
			if(!bCreateBarbarians)
				continue;
			int iNeededSea = numBarbariansToCreate(iBaseTilesPerSeaUnit,
					shelves[i]->size(), shelves[i]->countUnownedPlots(), iShips);
			/* 'BETTER_BTS_AI_MOD 9/25/08 jdog5000
				Limit construction of barb ships based on player navies' */
			// advc: BBAI code deleted -- sanity check based on Barbarian cities instead:
			if(iShips > iBarbCities + 2)
				iNeededSea = 0;
			iNeededLand -= createBarbarianUnits(iNeededSea, a, shelves[i],
					iNeededLand > 0); // advc.306
		}
		/*  Don't spawn Barbarian units on (or on shelves around) continents where
			civs don't outnumber Barbarians */
		int iCivCities = a.countCivCities();
		int iBarbarianCities = a.getNumCities() - iCivCities;
		FAssert(iBarbarianCities >= 0);
		if(iCivCities > iBarbarianCities && bCreateBarbarians)
			createBarbarianUnits(iNeededLand, a, NULL);
		/*  Rest of the creation code: moved into functions numBarbariansToCreate and
			createBarbarians */
		// </advc.300>
	}
	FOR_EACH_UNIT(pLoopUnit, GET_PLAYER(BARBARIAN_PLAYER))
	{
		if (pLoopUnit->isAnimal()
				// advc.309: Don't cull animals where there are no civ cities
				&& pLoopUnit->area()->countCivCities() > 0)
		{
			pLoopUnit->kill(false);
			break;
		}
	} // <advc.300>
	FOR_EACH_CITY(c, GET_PLAYER(BARBARIAN_PLAYER)) {
		/*  Large Barb congregations are only a problem if they have nothing
			to attack */
		if(c->area()->countCivCities() > 0)
			continue;
		int iUnits = c->plot()->getNumDefenders(BARBARIAN_PLAYER);
		double prKill = (iUnits - std::max(1.5 * c->getPopulation(), 4.0)) / 4.0;
		if(::bernoulliSuccess(prKill, "advc.300 (kill_1)"))
			c->plot()->killRandomUnit(BARBARIAN_PLAYER, DOMAIN_LAND);
	} // </advc.300>
}


void CvGame::createAnimals()  // advc.003: style changes
{
	if (GC.getEraInfo(getCurrentEra()).isNoAnimals()
			|| isOption(GAMEOPTION_NO_ANIMALS)) // advc.309
		return;

	CvHandicapInfo const& kGameHandicap = GC.getHandicapInfo(getHandicapType());
	if (kGameHandicap.getUnownedTilesPerGameAnimal() <= 0)
		return;

	if (getNumCivCities() < countCivPlayersAlive())
		return;

	if (getElapsedGameTurns() < 5)
		return;

	int iLoop;
	for(CvArea* pLoopArea = GC.getMap().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMap().nextArea(&iLoop))
	{
		if (pLoopArea->isWater())
			continue;

		int iNeededAnimals = pLoopArea->getNumUnownedTiles() /
				kGameHandicap.getUnownedTilesPerGameAnimal();
		/*  <advc.300> Will allow animals to survive longer on landmasses w/o
			civ cities. But only want a couple of animals there. */
		if(pLoopArea->countCivCities() == 0)
			iNeededAnimals /= 2; // </advc.300>
		iNeededAnimals -= pLoopArea->getUnitsPerPlayer(BARBARIAN_PLAYER);
		if (iNeededAnimals <= 0)
			continue;

		iNeededAnimals = (iNeededAnimals / 5) + 1;
		for (int iI = 0; iI < iNeededAnimals; iI++)
		{
			CvPlot* pPlot = GC.getMap().syncRandPlot(
					(RANDPLOT_NOT_VISIBLE_TO_CIV | RANDPLOT_PASSABLE
					| RANDPLOT_WATERSOURCE), // advc.300
					pLoopArea->getID(), GC.getDefineINT("MIN_ANIMAL_STARTING_DISTANCE"));
			if (pPlot == NULL)
				continue;

			UnitTypes eBestUnit = NO_UNIT;
			int iBestValue = 0;
			// advc (comment): This loop picks an animal that is suitable for pPlot
			for (int iJ = 0; iJ < GC.getNumUnitClassInfos(); iJ++)
			{
				UnitTypes eLoopUnit = (UnitTypes)(GC.getCivilizationInfo(GET_PLAYER(BARBARIAN_PLAYER).
						getCivilizationType()).getCivilizationUnits(iJ));
				if (eLoopUnit == NO_UNIT)
					continue;
				CvUnitInfo const& kUnit = GC.getUnitInfo(eLoopUnit);
				if (!kUnit.getUnitAIType(UNITAI_ANIMAL))
					continue;
				if (pPlot->getFeatureType() != NO_FEATURE ?
						kUnit.getFeatureNative(pPlot->getFeatureType()) :
						kUnit.getTerrainNative(pPlot->getTerrainType()))
				{
					int iValue = 1 + getSorenRandNum(1000, "Animal Unit Selection");
					if (iValue > iBestValue)
					{
						eBestUnit = eLoopUnit;
						iBestValue = iValue;
					}
				}
			}
			if (eBestUnit != NO_UNIT)
			{
				GET_PLAYER(BARBARIAN_PLAYER).initUnit(eBestUnit,
						pPlot->getX(), pPlot->getY(), UNITAI_ANIMAL);
			}
		}
	}
}

// <advc.307>
bool CvGame::isBarbarianCreationEra() const {

	if(isOption(GAMEOPTION_NO_BARBARIANS))
		return false;
	EraTypes eCurrentEra = getCurrentEra();
	return (!GC.getEraInfo(eCurrentEra).isNoBarbUnits() &&
			/*  Also stop spawning when Barbarian tech falls behind too much;
				may resume once they catch up. */
			eCurrentEra <= GET_PLAYER(BARBARIAN_PLAYER).getCurrentEra() + 1);
}

// <advc.300>
int CvGame::getBarbarianStartTurn() const {

	int iTargetElapsed = GC.getHandicapInfo(getHandicapType()).
		   getBarbarianCreationTurnsElapsed();
	iTargetElapsed *= GC.getGameSpeedInfo(getGameSpeedType()).getBarbPercent();
	int iDivisor = 100;
	/*  This term is new. Well, not entirely, it's also applied to
		BarbarianCityCreationTurnsElapsed. */
	iDivisor *= std::max(1, (int)getStartEra());
	iTargetElapsed /= iDivisor;
	int iStartTurn = getStartTurn();
	// Have Barbarians appear earlier in Ancient Advanced Start too
	if(isOption(GAMEOPTION_ADVANCED_START) && getStartEra() <= 0 &&
			// advc.250b: Earlier Barbarians only if humans start Advanced too
			!isOption(GAMEOPTION_SPAH))
		iStartTurn /= 2;
	return iStartTurn + iTargetElapsed;
}

// Based on code originally in createBarbarianUnits, but modified beyond recognition.
int CvGame::numBarbariansToCreate(int iTilesPerUnit, int iTiles, int iUnowned,
		int iUnitsPresent, int iBarbarianCities) {

	int iOwned = iTiles - iUnowned;
	int iPeakPercent = ::range(GC.getDefineINT("BARB_PEAK_PERCENT"), 0, 100);
	if(iOwned == 0 || iPeakPercent == 0)
		return 0;
	double peak = iPeakPercent / 100.0;
	double ownedRatio = iOwned / (double)iTiles;
	bool bPeakReached = (ownedRatio > peak);
	double divisor = iTilesPerUnit;
	double dividend = -1;
	if(bPeakReached) {
		divisor *= (1 - peak);
		dividend = iUnowned;
	}
	else {
		divisor *= peak;
		dividend = iOwned;
	}
	/*	For Rage, reduce divisor to 60% (50% in BtS), but
		<advc.307> reduces it further based on the game era. */
	if(isOption(GAMEOPTION_RAGING_BARBARIANS)) {
		int iCurrentEra = getCurrentEra();
		/*  Don't reduce divisor in start era (gets too tough on Classical
			and Medieval starts b/c the starting defenders are mere Archers). */
		if(iCurrentEra <= getStartEra())
			iCurrentEra = 0;
		double rageMultiplier = 0.6;
		rageMultiplier *= (8 - iCurrentEra) / 8.0;
		divisor = divisor * rageMultiplier;
		divisor = std::max(divisor, 10.0);
	} // </advc.307>
	else divisor = std::max(divisor, 14.0);
	double target = std::min(dividend / divisor,
			/*  Make sure that there's enough unowned land where the Barbarians
				could plausibly gather. */
			iUnowned / 6.0);
	double adjustment = GC.getDefineINT("BARB_ACTIVITY_ADJUSTMENT") + 100;
	adjustment /= 100;
	target *= adjustment;

	int iInitialDefenders = GC.getHandicapInfo(getHandicapType()).
			getBarbarianInitialDefenders();
	double r = target - std::max(0, iUnitsPresent
	/*  Don't count city defenders. Settled Barbarians being less aggressive makes
		sense, but cities also reduce the number of unowned tiles; that's enough.
		(Alt. idea: Subtract half the Barbarian population in this area.)
		Old Firaxis to-do comment on this subject: 'XXX eventually need to measure
		how many barbs of eBarbUnitAI we have in this area...' */
			- iBarbarianCities * std::max(0, iInitialDefenders));
	if (r < 1)
		return 0; // Avoid very small creation probabilities
	double creationRate = 0.25; // the BtS rate
	// Novel: adjusted to game speed
	creationRate /= (GC.getGameSpeedInfo(getGameSpeedType()).getBarbPercent() / 100.0);
	/*  BtS always created at least one unit, but, on Marathon, this could be too fast.
		Probabilistic instead. */
	r *= creationRate;
	if (r < 1)
	{
		if (::bernoulliSuccess(r, "advc.300 (numBarbariansToCreate)"))
			return 1;
		else return 0;
	}
	return ::round(r);
}

// Returns the number of land units spawned (possibly in cargo). The first half is new code.
int CvGame::createBarbarianUnits(int n, CvArea& a, Shelf* pShelf, bool bCargoAllowed) { // </advc.300>

	/* <advc.306> Spawn cargo load before ships. Othwerwise, the newly placed ship
	   would always be an eligible target and too many ships would carry cargo. */
	FAssert(!bCargoAllowed || pShelf != NULL);
	int r = 0;
	if(bCargoAllowed) {
		CvUnit* pTransport = pShelf->randomBarbarianCargoUnit();
		if(pTransport != NULL) {
			UnitAITypes eLoadAI = UNITAI_ATTACK;
			for(int i = 0; i < 2; i++)
			{
				UnitTypes eLoadUnit = randomBarbarianUnit(eLoadAI, a);
				if(eLoadUnit == NO_UNIT)
					break;
				CvUnit* pLoadUnit = GET_PLAYER(BARBARIAN_PLAYER).initUnit(
						eLoadUnit, pTransport->getX(), pTransport->getY(), eLoadAI);
				/*  Don't set pTransport to UNITAI_ASSAULT_SEA -- that's for
					medium-/large-scale invasions, and too laborious to adjust.
					Instead add an unload routine to CvUnitAI::barbAttackSeaMove. */
				if(pLoadUnit == NULL)
					break;
				pLoadUnit->setTransportUnit(pTransport);
				r++;
				/*  Only occasionally spawn two units at once. Prefer the natural
					way, i.e. a ship receiving a second passenger while travelling
					to its target through fog of war. (I don't think that happens
					often enough though ...) */
				if(pTransport->getCargo() > 1 || ::bernoulliSuccess(0.7, "advc.306"))
					break;
			}
		}
	} // </advc.306>
	// From here on, mostly cut and pasted from the oiginal createBarbarianUnits.

	for (int iI = 0; iI < n; iI++) { // <advc.300>
		CvPlot* pPlot = NULL;
		// Reroll twice if the tile has poor yield
		for(int i = 0; i < 3; i++)
		{
			pPlot = randomBarbarianPlot(a, pShelf);
			/*  If we can't find a plot once, we won't find one in a later
				iteration either. */
			if(pPlot == NULL)
				return r;
			int iTotalYield = 0;
			for(int j = 0; j < GC.getNUM_YIELD_TYPES(); j++)
				iTotalYield += pPlot->getYield((YieldTypes)j);
			// Want to re-roll flat Tundra Forest as well
			if(iTotalYield == 2 && pPlot->getImprovementType() == NO_IMPROVEMENT)
			{
				iTotalYield = 0;
				for(int j = 0; j < GC.getNUM_YIELD_TYPES(); j++)
				{
					iTotalYield += pPlot->calculateNatureYield((YieldTypes)j,
							NO_TEAM, /* bIgnoreFeature=*/ true);
				}
			}
			if(iTotalYield >= 2)
				break;
		}
		UnitAITypes eUnitAI = UNITAI_ATTACK;
		if(pShelf != NULL)
			eUnitAI = UNITAI_ATTACK_SEA;
		// Original code moved into new function:
		UnitTypes eUnitType = randomBarbarianUnit(eUnitAI, a);
		if(eUnitType == NO_UNIT)
			return r;
		CvUnit* pNewUnit = GET_PLAYER(BARBARIAN_PLAYER).initUnit(eUnitType,
				pPlot->getX(), pPlot->getY(), eUnitAI);
		if(pNewUnit != NULL && !pPlot->isWater())
			r++;
		// </advc.300>
		// K-Mod. Give a combat penalty to barbarian boats.
		if (pNewUnit && pPlot->isWater() &&
				 !pNewUnit->getUnitInfo().isHiddenNationality()) // dlph.12
		{	// find the "disorganized" promotion. (is there a better way to do this?)
			PromotionTypes eDisorganized = (PromotionTypes)GC.getInfoTypeForString("PROMOTION_DISORGANIZED", true);
			if (eDisorganized != NO_PROMOTION)
			{
				// sorry, barbarians. Free boats are just too dangerous for real civilizations to defend against.
				pNewUnit->setHasPromotion(eDisorganized, true);
			}
		} // K-Mod end
	}
	return r; // advc.306
}

// <advc.300>
CvPlot* CvGame::randomBarbarianPlot(CvArea const& a, Shelf* shelf) const {

	int restrictionFlags = RANDPLOT_NOT_VISIBLE_TO_CIV |
			/*  Shelves already ensure this and one-tile islands
				can't spawn Barbarians anyway. */
			//RANDPLOT_ADJACENT_LAND |
			RANDPLOT_PASSABLE |
			RANDPLOT_HABITABLE | // New flag
			RANDPLOT_UNOWNED;
	/*  Added the "unowned" flag to prevent spawning in Barbarian land.
		Could otherwise happen now b/c the visible flag and dist. restriction
		no longer apply to Barbarians previously spawned; see
		CvPlot::isVisibleToCivTeam, CvMap::isCivUnitNearby. */
	int iDist = GC.getDefineINT("MIN_BARBARIAN_STARTING_DISTANCE");
	// <advc.304> Sometimes don't pick a plot if there are few legal plots
	int iLegal = 0;
	CvPlot* r = NULL;
	if(shelf == NULL)
		r = GC.getMap().syncRandPlot(restrictionFlags, a.getID(), iDist, -1, &iLegal);
	else {
		r = shelf->randomPlot(restrictionFlags, iDist, &iLegal);
		if(r != NULL && iLegal * 100 < shelf->size())
			r = NULL;
	}
	if(r != NULL) {
		double prSkip = 0;
		if(iLegal > 0 && iLegal < 4)
			prSkip = 1 - 1.0 / (5 - iLegal);
		if(::bernoulliSuccess(prSkip, "advc.304"))
			r = NULL;
	}
	return r; // </advc.304>
}


bool CvGame::killBarbarian(int iPresent, int iTiles, int iBarbPop, CvArea& a, Shelf* shelf) {

	if(iPresent <= 5) // 5 is never a crowd
		return false;
	double divisor = 4 * iBarbPop;
	if(shelf != NULL)
		divisor += shelf->size();
	else divisor += iTiles; /*  Includes 50% shelf (given the way this function
								is currently used). */
	// Don't want large Barbarian continents crawling with units
	divisor = 5 * std::pow(divisor, 0.7);
	if(::bernoulliSuccess(iPresent / divisor, "advc.300 (kill_2)")) {
		if(shelf != NULL)
			return shelf->killBarbarian();
		/*  Tbd.: Be a bit more considerate about which unit to sacrifice.
			Currently, it's the same (arbitrary) method as for animal culling. */
		FOR_EACH_UNIT(pUnit, GET_PLAYER(BARBARIAN_PLAYER)) {
			CvUnit& u = *pUnit;
			if(u.isAnimal() || u.plot()->area()->getID() != a.getID() ||
					u.getUnitCombatType() == NO_UNITCOMBAT)
				continue;
			u.kill(false);
			return true;
		}
	}
	return false;
}

// Based on BtS code originally in createBarbarianUnits
UnitTypes CvGame::randomBarbarianUnit(UnitAITypes eUnitAI, CvArea const& a) {

	bool bSea;
	switch(eUnitAI) {
	case UNITAI_ATTACK_SEA: bSea = true; break;
	case UNITAI_ATTACK: bSea = false; break;
	default: return NO_UNIT;
	}
	UnitTypes r = NO_UNIT;
	int iBestValue = 0;
	for(int i = 0; i < GC.getNumUnitClassInfos(); i++) {
		UnitTypes eUnit = (UnitTypes)(GC.getCivilizationInfo(
				GET_PLAYER(BARBARIAN_PLAYER).getCivilizationType()).
				getCivilizationUnits(i));
		if(eUnit == NO_UNIT)
			continue;
		CvUnitInfo const& u = GC.getUnitInfo(eUnit);
		DomainTypes eDomain = (DomainTypes)u.getDomainType();
		if(u.getCombat() <= 0 || eDomain == DOMAIN_AIR ||
				u.isMostlyDefensive() || // advc.315
				(eDomain == DOMAIN_SEA) != bSea ||
				!GET_PLAYER(BARBARIAN_PLAYER).canTrain(eUnit))
			continue;
		// <advc.301>
		BonusTypes eAndBonus = (BonusTypes)u.getPrereqAndBonus();
		TechTypes eAndBonusTech = NO_TECH;
		if(eAndBonus != NO_BONUS) {
			eAndBonusTech = (TechTypes)GC.getBonusInfo(eAndBonus).getTechCityTrade();
			if((eAndBonusTech != NO_TECH && !GET_TEAM(BARBARIAN_TEAM).
					isHasTech(eAndBonusTech)) || !a.hasAnyAreaPlayerBonus(eAndBonus))
				continue;
		}
		/*  No units from more than 1 era ago (obsoletion too difficult to test).
			hasTech already tested by canTrain, but era shouldn't be
			tested there b/c it's OK for Barbarian cities to train outdated units
			(they only will if they can't train anything better). */
		TechTypes eAndTech = (TechTypes)u.getPrereqAndTech();
		int iUnitEra = 0;
		if(eAndTech != NO_TECH)
			iUnitEra = GC.getTechInfo(eAndTech).getEra();
		if(eAndBonusTech != NO_TECH)
			iUnitEra = std::max(iUnitEra, GC.getTechInfo(eAndBonusTech).getEra());
		if(iUnitEra + 1 < getCurrentEra())
			continue; // </advc.301>
		bool bFound = false;
		bool bRequires = false;
		for(int j = 0; j < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); j++) {
			BonusTypes eOrBonus = (BonusTypes)u.getPrereqOrBonuses(j);
			if(eOrBonus == NO_BONUS)
				continue;
			CvBonusInfo const& kOrBonus = GC.getBonusInfo(eOrBonus);
			TechTypes eOrBonusTech = (TechTypes)kOrBonus.getTechCityTrade();
			if(eOrBonusTech != NO_TECH) {
				bRequires = true;
				if(GET_TEAM(BARBARIAN_TEAM).isHasTech(eOrBonusTech)
						/*  advc.301: Also require the resource to be connected by
							someone on this continent; in particular, don't spawn
							Horse Archers on a horseless continent. */
						&& a.hasAnyAreaPlayerBonus(eOrBonus)) {
					bFound = true;
					break;
				}
			}
		}
		if(bRequires && !bFound)
			continue;
		/*  <advc.301>: The code above only checks if they can build the improvements
			necessary to obtain the required resources; it does not check if they
			can see/use the resource. This means that Spearmen often appear before
			Archers b/c they require only Hunting and Mining, and not Bronze Working.
			Correction: */
		if(!GET_TEAM(BARBARIAN_TEAM).canSeeReqBonuses(eUnit))
			continue; // </advc.301>
		int iValue = (1 + getSorenRandNum(1000, "Barb Unit Selection"));
		if(u.getUnitAIType(eUnitAI))
			iValue += 200;
		if(iValue > iBestValue) {
			r = eUnit;
			iBestValue = iValue;
		}
	}
	return r;
}
// </advc.300>

void CvGame::updateWar()
{
	int iI, iJ;

	if (isOption(GAMEOPTION_ALWAYS_WAR))
	{
		for (iI = 0; iI < MAX_TEAMS; iI++)
		{
			CvTeam& kTeam1 = GET_TEAM((TeamTypes)iI);
			if (kTeam1.isAlive() && kTeam1.isHuman())
			{
				for (iJ = 0; iJ < MAX_TEAMS; iJ++)
				{
					CvTeam& kTeam2 = GET_TEAM((TeamTypes)iJ);
					if (kTeam2.isAlive() && !kTeam2.isHuman())
					{
						FAssert(iI != iJ);

						if (kTeam1.isHasMet((TeamTypes)iJ))
						{
							if (!kTeam1.isAtWar((TeamTypes)iJ))
							{
								kTeam1.declareWar(((TeamTypes)iJ), false, NO_WARPLAN);
							}
						}
					}
				}
			}
		}
	}
}


void CvGame::updateMoves()
{
	int iI;

	int aiShuffle[MAX_PLAYERS];
	if (isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
	{
		shuffleArray(aiShuffle, MAX_PLAYERS, getSorenRand());
	}
	else
	{
		for (iI = 0; iI < MAX_PLAYERS; iI++)
		{
			aiShuffle[iI] = iI;
		}
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& player = GET_PLAYER((PlayerTypes)(aiShuffle[iI]));

		if (player.isAlive())
		{
			if (player.isTurnActive())
			{
				if (!player.isAutoMoves())
				{
					player.AI_unitUpdate();

					if (!player.isHuman())
					{
						if (!(player.hasBusyUnit()) && !(player.hasReadyUnit(true)))
						{
							player.setAutoMoves(true);
						}
					}
				}

				if (player.isAutoMoves())
				{
					FOR_EACH_GROUP(pGroup, player)
						pGroup->autoMission();
					// K-Mod. Here's where we do the AI for automated units.
					// Note, we can't do AI_update and autoMission in the same loop, because either one might delete the group - and thus cause the other to crash.
					if (player.isHuman())
					{
						FOR_EACH_GROUP(pGroup, player)
						{
							if (pGroup->AI_update())
							{
								FAssert(player.hasBusyUnit());
								break;
							}
						}
						// Refresh the group cycle for human players.
						// Non-human players can wait for their units to wake up, or regain moves - group cycle isn't very important for them anyway.
						player.refreshGroupCycleList();
					}
					// K-Mod end

					if (!player.hasBusyUnit())
					{
						player.setAutoMoves(false);
					}
				}
			}
		}
	}
}


void CvGame::verifyCivics()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).verifyCivics();
		}
	}
}


void CvGame::updateTimers()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).updateTimers();
		}
	}
}


void CvGame::updateTurnTimer()  // advc.003: style changes
{
	if (!isMPOption(MPOPTION_TURN_TIMER)) // Are we using a turn timer?
		return;

	if (getElapsedGameTurns() <= 0 && isOption(GAMEOPTION_ADVANCED_START))
		return;

	if (getTurnSlice() <= getCutoffSlice()) // Has the turn expired?
		return;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive() && GET_PLAYER((PlayerTypes)iI).isTurnActive())
		{
			GET_PLAYER((PlayerTypes)iI).setEndTurn(true);
			if (!isMPOption(MPOPTION_SIMULTANEOUS_TURNS) && !isSimultaneousTeamTurns())
				break;
		}
	}
}


void CvGame::testAlive()
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
		GET_PLAYER((PlayerTypes)iI).verifyAlive();
}

bool CvGame::testVictory(VictoryTypes eVictory, TeamTypes eTeam, bool* pbEndScore) const  // advc.003: simplified this function a bit
{
	FASSERT_BOUNDS(0, GC.getNumVictoryInfos(), eVictory, "CvGame::testVictory");
	FASSERT_BOUNDS(0, MAX_CIV_TEAMS, eTeam, "CvGame::testVictory");
	FAssert(GET_TEAM(eTeam).isAlive());

	if(pbEndScore != NULL)
		*pbEndScore = false;

	if(!isVictoryValid(eVictory))
		return false;

	CvVictoryInfo const& kVictory = GC.getVictoryInfo(eVictory);
	if (kVictory.isEndScore())
	{
		if (pbEndScore)
			*pbEndScore = true;

		if (getMaxTurns() == 0 || getElapsedGameTurns() < getMaxTurns())
			return false;

		for (int iK = 0; iK < MAX_CIV_TEAMS; iK++)
		{
			if (GET_TEAM((TeamTypes)iK).isAlive() && iK != eTeam &&
					getTeamScore((TeamTypes)iK) >= getTeamScore(eTeam))
				return false;
		}
	}
	if (kVictory.isTargetScore())
	{
		if (getTargetScore() == 0 || getTeamScore(eTeam) < getTargetScore())
			return false;

		for (int iK = 0; iK < MAX_CIV_TEAMS; iK++)
		{
			if (GET_TEAM((TeamTypes)iK).isAlive() && iK != eTeam &&
					getTeamScore((TeamTypes)iK) >= getTeamScore(eTeam))
				return false;
		}

	}
	CvTeam const& kTeam = GET_TEAM(eTeam);
	if (kVictory.isConquest())
	{
		if (kTeam.getNumCities() == 0)
			return false;

		int iEverAlive = 0; // advc.084
		for (int iK = 0; iK < MAX_CIV_TEAMS; iK++)
		{
			CvTeam const& kLoopTeam = GET_TEAM((TeamTypes)iK);
			if (kLoopTeam.isAlive() && iK != eTeam &&
					!kLoopTeam.isVassal(eTeam) && kLoopTeam.getNumCities() > 0)
				return false;
			// <advc.084>
			if(kLoopTeam.isEverAlive())
				iEverAlive++;
		}
		if(iEverAlive <= 1)
			return false; // </advc.084>
	}
	if (kVictory.isDiploVote())
	{
		bool bFound = false;
		for (int iK = 0; iK < GC.getNumVoteInfos(); iK++)
		{
			if (GC.getVoteInfo((VoteTypes)iK).isVictory() && getVoteOutcome((VoteTypes)iK) == eTeam)
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
			return false;
	}
	if (getAdjustedPopulationPercent(eVictory) > 0)
	{
		if (100 * kTeam.getTotalPopulation() < getTotalPopulation() *
				getAdjustedPopulationPercent(eVictory))
			return false;
	}
	if (getAdjustedLandPercent(eVictory) > 0)
	{
		if (100 * kTeam.getTotalLand() < GC.getMap().getLandPlots() *
				getAdjustedLandPercent(eVictory))
			return false;
	}
	if (kVictory.getReligionPercent() > 0)
	{
		if (getNumCivCities() <= countCivPlayersAlive() * 2)
			return false;

		bool bFound = false;
		for (int iK = 0; iK < GC.getNumReligionInfos(); iK++)
		{
			ReligionTypes eLoopReligion = (ReligionTypes)iK;
			if (kTeam.hasHolyCity(eLoopReligion) &&
					calculateReligionPercent(eLoopReligion) >=
					kVictory.getReligionPercent())
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
			return false;
	}
	if (kVictory.getCityCulture() != NO_CULTURELEVEL &&
			kVictory.getNumCultureCities() > 0)
	{
		int iCount = 0;
		for (int iK = 0; iK < MAX_CIV_PLAYERS; iK++)
		{
			CvPlayer const& kLoopPlayer = GET_PLAYER((PlayerTypes)iK);
			if (!kLoopPlayer.isAlive() || kLoopPlayer.getTeam() != eTeam)
				continue;
			FOR_EACH_CITY(pLoopCity, kLoopPlayer)
			{
				if (pLoopCity->getCultureLevel() >= kVictory.getCityCulture())
					iCount++;
			}
		}
		if (iCount < kVictory.getNumCultureCities())
			return false;
	}
	if (kVictory.getTotalCultureRatio() > 0)
	{
		int iThreshold = (kTeam.countTotalCulture() * 100) /
				kVictory.getTotalCultureRatio();
		for (int iK = 0; iK < MAX_CIV_TEAMS; iK++)
		{
			CvTeam const& kLoopTeam = GET_TEAM((TeamTypes)iK);
			if (kLoopTeam.isAlive() && iK != eTeam &&
					kLoopTeam.countTotalCulture() > iThreshold)
				return false;
		}
	}
	for (int iK = 0; iK < GC.getNumBuildingClassInfos(); iK++)
	{
		BuildingClassTypes eLoopClass = (BuildingClassTypes)iK;
		if (GC.getBuildingClassInfo(eLoopClass).getVictoryThreshold(eVictory) >
				kTeam.getBuildingClassCount(eLoopClass))
			return false;
	}
	for (int iK = 0; iK < GC.getNumProjectInfos(); iK++)
	{
		ProjectTypes eLoopProject = (ProjectTypes)iK;
		if (GC.getProjectInfo(eLoopProject).getVictoryMinThreshold(eVictory) >
				kTeam.getProjectCount(eLoopProject))
			return false;
	}
	long lResult = 1; // advc.003b: Disable this callback unless enabled through XML?
	CyArgsList argsList; argsList.add(eVictory);
	gDLL->getPythonIFace()->callFunction(PYGameModule, "isVictory", argsList.makeFunctionArgs(), &lResult);
	if (lResult == 0)
		return false;
	return true;
}

void CvGame::testVictory()
{
	bool bEndScore = false;

	if (getVictory() != NO_VICTORY)
	{
		return;
	}

	if (getGameState() == GAMESTATE_EXTENDED)
	{
		return;
	}

	updateScore();

	long lResult = 1; // advc (comment): This checks if 10 turns have elapsed
	gDLL->getPythonIFace()->callFunction(PYGameModule, "isVictoryTest", NULL, &lResult);
	if (lResult == 0)
		return;

	std::vector<std::vector<int> > aaiWinners;

	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
		if (kLoopTeam.isAlive())
		{
			if (!(kLoopTeam.isMinorCiv()))
			{
				for (int iJ = 0; iJ < GC.getNumVictoryInfos(); iJ++)
				{
					if (testVictory((VictoryTypes)iJ, (TeamTypes)iI, &bEndScore))
					{
						if (kLoopTeam.getVictoryCountdown((VictoryTypes)iJ) < 0)
						{
							if (kLoopTeam.getVictoryDelay((VictoryTypes)iJ) == 0)
							{
								kLoopTeam.setVictoryCountdown((VictoryTypes)iJ, 0);
							}
						}

						//update victory countdown
						if (kLoopTeam.getVictoryCountdown((VictoryTypes)iJ) > 0)
						{
							kLoopTeam.changeVictoryCountdown((VictoryTypes)iJ, -1);
						}

						if (kLoopTeam.getVictoryCountdown((VictoryTypes)iJ) == 0)
						{
							if (getSorenRandNum(100, "Victory Success") < kLoopTeam.getLaunchSuccessRate((VictoryTypes)iJ))
							{
								std::vector<int> aWinner;
								aWinner.push_back(iI);
								aWinner.push_back(iJ);
								aaiWinners.push_back(aWinner);
							}
							else
							{
								kLoopTeam.resetVictoryProgress();
							}
						}
					}
				}
			}
		}

	}

	if (aaiWinners.size() > 0)
	{
		int iWinner = getSorenRandNum(aaiWinners.size(), "Victory tie breaker");
		setWinner(((TeamTypes)aaiWinners[iWinner][0]), ((VictoryTypes)aaiWinners[iWinner][1]));
	}

	if (getVictory() == NO_VICTORY)
	{
		if (getMaxTurns() > 0)
		{
			if (getElapsedGameTurns() >= getMaxTurns())
			{
				if (!bEndScore)
				{
					if ((getAIAutoPlay() > 0) || gDLL->GetAutorun())
					{
						setGameState(GAMESTATE_EXTENDED);
					}
					else
					{
						setGameState(GAMESTATE_OVER);
					}
				}
			}
		}
	}
}


void CvGame::processVote(const VoteTriggeredData& kData, int iChange)
{
	CvVoteInfo& kVote = GC.getVoteInfo(kData.kVoteOption.eVote);

	changeTradeRoutes(kVote.getTradeRoutes() * iChange);
	changeFreeTradeCount(kVote.isFreeTrade() ? iChange : 0);
	changeNoNukesCount(kVote.isNoNukes() ? iChange : 0);

	for (int iI = 0; iI < GC.getNumCivicInfos(); iI++)
	{
		changeForceCivicCount((CivicTypes)iI, kVote.isForceCivic(iI) ? iChange : 0);
	}


	if (iChange > 0)
	{
		if (kVote.isOpenBorders())
		{
			for (int iTeam1 = 0; iTeam1 < MAX_CIV_PLAYERS; ++iTeam1)
			{
				CvTeam& kLoopTeam1 = GET_TEAM((TeamTypes)iTeam1);
				if (kLoopTeam1.isVotingMember(kData.eVoteSource))
				{
					for (int iTeam2 = iTeam1 + 1; iTeam2 < MAX_CIV_PLAYERS; ++iTeam2)
					{
						CvTeam& kLoopTeam2 = GET_TEAM((TeamTypes)iTeam2);
						if (kLoopTeam2.isVotingMember(kData.eVoteSource))
						{
							kLoopTeam1.signOpenBorders((TeamTypes)iTeam2);
						}
					}
				}
			}

			setVoteOutcome(kData, NO_PLAYER_VOTE);
		}
		else if (kVote.isDefensivePact())
		{
			for (int iTeam1 = 0; iTeam1 < MAX_CIV_PLAYERS; ++iTeam1)
			{
				CvTeam& kLoopTeam1 = GET_TEAM((TeamTypes)iTeam1);
				if (kLoopTeam1.isVotingMember(kData.eVoteSource))
				{
					for (int iTeam2 = iTeam1 + 1; iTeam2 < MAX_CIV_PLAYERS; ++iTeam2)
					{
						CvTeam& kLoopTeam2 = GET_TEAM((TeamTypes)iTeam2);
						if (kLoopTeam2.isVotingMember(kData.eVoteSource))
						{
							kLoopTeam1.signDefensivePact((TeamTypes)iTeam2);
						}
					}
				}
			}

			setVoteOutcome(kData, NO_PLAYER_VOTE);
		}
		else if (kVote.isForcePeace())
		{
			FAssert(NO_PLAYER != kData.kVoteOption.ePlayer);
			CvPlayer& kPlayer = GET_PLAYER(kData.kVoteOption.ePlayer);
			if (gTeamLogLevel >= 1) // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
				logBBAI("  Vote for forcing peace against team %d (%S) passes", kPlayer.getTeam(), kPlayer.getCivilizationDescription(0));
			// <dlph.25> 'Cancel defensive pacts with the attackers first'
			int foo=-1;
			for(CvDeal* pLoopDeal = firstDeal(&foo); pLoopDeal != NULL; pLoopDeal = nextDeal(&foo)) {
				bool bCancelDeal = false;
				if((TEAMID(pLoopDeal->getFirstPlayer()) == kPlayer.getTeam() &&
						TEAMREF(pLoopDeal->getSecondPlayer()).isVotingMember(
						kData.eVoteSource)) || (GET_PLAYER(pLoopDeal->
						getSecondPlayer()).getTeam() == kPlayer.getTeam() &&
						TEAMREF(pLoopDeal->getFirstPlayer()).isVotingMember(
						kData.eVoteSource))) {
					for(CLLNode<TradeData>* pNode = pLoopDeal->headFirstTradesNode();
							pNode != NULL; pNode = pLoopDeal->nextFirstTradesNode(pNode)) {
						if(pNode->m_data.m_eItemType == TRADE_DEFENSIVE_PACT) {
							bCancelDeal = true;
							break;
						}
					}
					if(!bCancelDeal) {
						for(CLLNode<TradeData>* pNode = pLoopDeal->headSecondTradesNode();
								pNode != NULL; pNode = pLoopDeal->nextSecondTradesNode(pNode)) {
							if(pNode->m_data.m_eItemType == TRADE_DEFENSIVE_PACT) {
								bCancelDeal = true;
								break;
							}
						}
					}
				}
				if(bCancelDeal)
					pLoopDeal->kill();
			} // </dlph.25>
			for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
			{
				CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
				if (kLoopPlayer.getTeam() != kPlayer.getTeam())
				{
					if (kLoopPlayer.isVotingMember(kData.eVoteSource))
					{
						kLoopPlayer.forcePeace(kData.kVoteOption.ePlayer);
					}
				}
			}

			setVoteOutcome(kData, NO_PLAYER_VOTE);
		}
		else if (kVote.isForceNoTrade())
		{
			FAssert(NO_PLAYER != kData.kVoteOption.ePlayer);
			CvPlayer& kPlayer = GET_PLAYER(kData.kVoteOption.ePlayer);

			for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
			{
				CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
				if (kLoopPlayer.isVotingMember(kData.eVoteSource))
				{
					if (kLoopPlayer.canStopTradingWithTeam(kPlayer.getTeam()))
					{
						kLoopPlayer.stopTradingWithTeam(kPlayer.getTeam());
					}
				}
			}

			setVoteOutcome(kData, NO_PLAYER_VOTE);
		}
		else if (kVote.isForceWar())
		{
			FAssert(NO_PLAYER != kData.kVoteOption.ePlayer);
			CvPlayer& kPlayer = GET_PLAYER(kData.kVoteOption.ePlayer);
			if (gTeamLogLevel >= 1) // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
				logBBAI("  Vote for war against team %d (%S) passes", kPlayer.getTeam(), kPlayer.getCivilizationDescription(0));

			for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
			{
				CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
				//if (kLoopPlayer.isVotingMember(kData.eVoteSource))
				// dlph.25/advc:
				if(GET_TEAM(kLoopPlayer.getTeam()).isFullMember(kData.eVoteSource))
				{
					if (GET_TEAM(kLoopPlayer.getTeam()).canChangeWarPeace(kPlayer.getTeam()))
					{
						//GET_TEAM(kLoopPlayer.getTeam()).declareWar(kPlayer.getTeam(), false, WARPLAN_DOGPILE);
						// <dlph.26>
						CvTeam::queueWar(kLoopPlayer.getTeam(), kPlayer.getTeam(),
								false, WARPLAN_DOGPILE); // </dlph.26>
						// advc.104i:
						GET_TEAM(kPlayer.getTeam()).makeUnwillingToTalk(kLoopPlayer.getTeam());
					}
				}
			}
			CvTeam::triggerWars(); // dlph.26
			setVoteOutcome(kData, NO_PLAYER_VOTE);
		}
		else if (kVote.isAssignCity())
		{
			FAssert(NO_PLAYER != kData.kVoteOption.ePlayer);
			CvPlayer& kPlayer = GET_PLAYER(kData.kVoteOption.ePlayer);
			CvCity* pCity = kPlayer.getCity(kData.kVoteOption.iCityId);
			FAssert(NULL != pCity);

			if (NULL != pCity)
			{
				if (NO_PLAYER != kData.kVoteOption.eOtherPlayer && kData.kVoteOption.eOtherPlayer != pCity->getOwner())
				{
					if (gTeamLogLevel >= 1) // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
						logBBAI("  Vote for assigning %S to %d (%S) passes", pCity->getName().GetCString(), GET_PLAYER(kData.kVoteOption.eOtherPlayer).getTeam(), GET_PLAYER(kData.kVoteOption.eOtherPlayer).getCivilizationDescription(0));
					GET_PLAYER(kData.kVoteOption.eOtherPlayer).acquireCity(pCity, false, true, true);
				}
			}
			setVoteOutcome(kData, NO_PLAYER_VOTE);
		}
	}
}


int CvGame::getIndexAfterLastDeal()
{
	return m_deals.getIndexAfterLast();
}


int CvGame::getNumDeals()
{
	return m_deals.getCount();
}


CvDeal* CvGame::addDeal()
{
	return m_deals.add();
}


 void CvGame::deleteDeal(int iID)
{
	m_deals.removeAt(iID);
	gDLL->getInterfaceIFace()->setDirty(Foreign_Screen_DIRTY_BIT, true);
}

CvDeal* CvGame::firstDeal(int *pIterIdx, bool bRev) const
{
	return !bRev ? m_deals.beginIter(pIterIdx) : m_deals.endIter(pIterIdx);
}


CvDeal* CvGame::nextDeal(int *pIterIdx, bool bRev) const
{
	return !bRev ? m_deals.nextIter(pIterIdx) : m_deals.prevIter(pIterIdx);
}

/*  <advc.072> All the FAssert(false) in this function mean that we're somehow
	out of step with the iteration that happens in the EXE. */
CvDeal* CvGame::nextCurrentDeal(PlayerTypes eGivePlayer, PlayerTypes eReceivePlayer,
		TradeableItems eItemType, int iData, bool bWidget) {

	if(!m_bShowingCurrentDeals) {
		m_currentDeals.clear(); // Probably not needed, but can't hurt to clear them.
		m_currentDealsWidget.clear();
		return NULL;
	}
	// Probably not needed:
	PlayerTypes eDiploPlayer = (PlayerTypes)gDLL->getDiplomacyPlayer();
	if(!((getActivePlayer() == eGivePlayer && eDiploPlayer == eReceivePlayer) ||
			(getActivePlayer() == eReceivePlayer && eDiploPlayer == eGivePlayer)))
		return NULL;
	CLinkList<DealItemData>& kCurrentDeals = (bWidget ? m_currentDealsWidget :
			m_currentDeals);
	if(kCurrentDeals.getLength() <= 0) {
		bool bFirstFound = false; int foo=-1;
		for(CvDeal* d = firstDeal(&foo); d != NULL; d = nextDeal(&foo)) {
			if(!d->isBetween(eGivePlayer, eReceivePlayer))
				continue;
			CLinkList<TradeData> const& kGiveList = *(d->getFirstPlayer() == eGivePlayer ?
					d->getFirstTrades() : d->getSecondTrades());
			for(CLLNode<TradeData>* pNode = kGiveList.head(); pNode != NULL;
					pNode = kGiveList.next(pNode)) {
				if(!CvDeal::isAnnual(pNode->m_data.m_eItemType) &&
						pNode->m_data.m_eItemType != TRADE_PEACE_TREATY)
					break;
				if(!bFirstFound) {
					if(pNode->m_data.m_eItemType != eItemType ||
							pNode->m_data.m_iData != iData) {
						FAssert(false);
						return NULL;
					}
					bFirstFound = true;
				}
				DealItemData data(eGivePlayer, eReceivePlayer, pNode->m_data.m_eItemType,
						pNode->m_data.m_iData, d->getID());
				kCurrentDeals.insertAtEnd(data);
			}
		}
	}
	if(kCurrentDeals.getLength() <= 0) {
		FAssert(false);
		return NULL;
	}
	CLLNode<DealItemData>* pNode = kCurrentDeals.head();
	DealItemData data = pNode->m_data;
	if(data.eGivePlayer != eGivePlayer || data.eReceivePlayer != eReceivePlayer ||
			data.iData != iData || data.eItemType != eItemType) {
		kCurrentDeals.clear();
		FAssert(false);
		return NULL;
	}
	CvDeal* r = getDeal(pNode->m_data.iDeal);
	kCurrentDeals.deleteNode(pNode);
	FAssert(r != NULL);
	return r;
} // </advc.072>


 CvRandom& CvGame::getMapRand()
{
	return m_mapRand;
}


int CvGame::getMapRandNum(int iNum, const char* pszLog)
{
	return m_mapRand.get(iNum, pszLog);
}


CvRandom& CvGame::getSorenRand()
{
	return m_sorenRand;
}


int CvGame::getSorenRandNum(int iNum, const char* pszLog,
		int iData1, int iData2) // advc.007
{
	return m_sorenRand.getInt(iNum, pszLog, /* advc.007: */ iData1, iData2);
}


int CvGame::calculateSyncChecksum()
{
	PROFILE_FUNC();
	// <advc.003b>
	if(!isNetworkMultiPlayer())
		return 0; // </advc.003b>

	int iValue = 0;
	iValue += getMapRand().getSeed();
	iValue += getSorenRand().getSeed();

	iValue += getNumCities();
	iValue += getTotalPopulation();
	iValue += getNumDeals();

	iValue += GC.getMap().getOwnedPlots();
	iValue += GC.getMap().getNumAreas();

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayerAI const& kPlayer = GET_PLAYER((PlayerTypes)iI);
		if (!kPlayer.isEverAlive())
			continue;

		int iJ=-1;
		int iMultiplier = getPlayerScore(kPlayer.getID());
		// <advc.001n>
		/*  Would like checkInSync to set this before calling CvDLLUtilityIFaceBase::
			GetSyncOOS, but that doesn't work b/c, apparently, GetSyncOOS returns
			the most recently computed checksum instead of calling calculateSyncChecksum. */
		bool const bFullOOSCheck = false;
		std::vector<long> aiMultipliers;
		int const iCases = 8; // Originally 4, K-Mod added another 4.
		for(int k = 0; k < (bFullOOSCheck ? iCases : 1); k++)
		{
			switch (bFullOOSCheck ? k : // </advc.001n>
				   (getTurnSlice() % iCases))
			{
			case 0:
				iMultiplier += (kPlayer.getTotalPopulation() * 543271);
				iMultiplier += (kPlayer.getTotalLand() * 327382);
				iMultiplier += (kPlayer.getGold() * 107564);
				iMultiplier += (kPlayer.getAssets() * 327455);
				iMultiplier += (kPlayer.getPower() * 135647);
				iMultiplier += (kPlayer.getNumCities() * 436432);
				iMultiplier += (kPlayer.getNumUnits() * 324111);
				iMultiplier += (kPlayer.getNumSelectionGroups() * 215356);
				break;

			case 1:
				for (iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
					iMultiplier += (kPlayer.calculateTotalYield((YieldTypes)iJ) * 432754);
				for (iJ = 0; iJ < NUM_COMMERCE_TYPES; iJ++)
					iMultiplier += (kPlayer.getCommerceRate((CommerceTypes)iJ) * 432789);
				break;

			case 2:
				for (iJ = 0; iJ < GC.getNumBonusInfos(); iJ++)
				{
					iMultiplier += (kPlayer.getNumAvailableBonuses((BonusTypes)iJ) * 945732);
					iMultiplier += (kPlayer.getBonusImport((BonusTypes)iJ) * 326443);
					iMultiplier += (kPlayer.getBonusExport((BonusTypes)iJ) * 932211);
				}
				for (iJ = 0; iJ < GC.getNumImprovementInfos(); iJ++)
					iMultiplier += (kPlayer.getImprovementCount((ImprovementTypes)iJ) * 883422);
				for (iJ = 0; iJ < GC.getNumBuildingClassInfos(); iJ++)
					iMultiplier += (kPlayer.getBuildingClassCountPlusMaking((BuildingClassTypes)iJ) * 954531);
				for (iJ = 0; iJ < GC.getNumUnitClassInfos(); iJ++)
					iMultiplier += (kPlayer.getUnitClassCountPlusMaking((UnitClassTypes)iJ) * 754843);
				for (iJ = 0; iJ < NUM_UNITAI_TYPES; iJ++)
					iMultiplier += (kPlayer.AI_totalUnitAIs((UnitAITypes)iJ) * 643383);
				break;

			case 3:
				FOR_EACH_UNIT(pLoopUnit, kPlayer)
				{
					iMultiplier += (pLoopUnit->getX() * 876543);
					iMultiplier += (pLoopUnit->getY() * 985310);
					iMultiplier += (pLoopUnit->getDamage() * 736373);
					iMultiplier += (pLoopUnit->getExperience() * 820622);
					iMultiplier += (pLoopUnit->getLevel() * 367291);
				}
				break;
			// K-Mod - new checks.
			case 4: // attitude cache
				// <advc.003n>
				if(iI == BARBARIAN_PLAYER)
					break; // </advc.003n>
				for (iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
				{
					if(iI != iJ) // advc.003: self-attitude should never matter
						iMultiplier += kPlayer.AI_getAttitudeVal((PlayerTypes)iJ, false) << iJ;
				}
				// strategy hash
				//iMultiplier += kPlayer.AI_getStrategyHash() * 367291;
				break;
			case 5: // city religions and corporations
				FOR_EACH_CITY(pLoopCity, kPlayer)
				{
					for (iJ = 0; iJ < GC.getNumReligionInfos(); iJ++)
					{
						if (pLoopCity->isHasReligion((ReligionTypes)iJ))
							iMultiplier += pLoopCity->getID() * (iJ+1);
					}
					for (iJ = 0; iJ < GC.getNumCorporationInfos(); iJ++)
					{
						if (pLoopCity->isHasCorporation((CorporationTypes)iJ))
							iMultiplier += (pLoopCity->getID()+1) * (iJ+1);
					}
				}
				break;
			case 6: // city production
				/*FOR_EACH_CITY(pLoopCity, kPlayer) {
					CLLNode<OrderData>* pOrderNode = pLoopCity->headOrderQueueNode();
					if (pOrderNode != NULL)
						iMultiplier += pLoopCity->getID()*(pOrderNode->m_data.eOrderType+2*pOrderNode->m_data.iData1+3*pOrderNode->m_data.iData2+6);
				} break;*/
				// city health and happiness
				FOR_EACH_CITY(pLoopCity, kPlayer)
				{
					iMultiplier += pLoopCity->goodHealth() * 876543;
					iMultiplier += pLoopCity->badHealth() * 985310;
					iMultiplier += pLoopCity->happyLevel() * 736373;
					iMultiplier += pLoopCity->unhappyLevel() * 820622;
					iMultiplier += pLoopCity->getFood() * 367291;
					/*  advc.001n: FloatingDefenders should be good enough as closeness
						factors into that */
					iMultiplier += pLoopCity->AI().AI_neededFloatingDefenders(false, true) * 324111;
					/*for(iJ = 0; iJ < MAX_PLAYERS; iJ++) {
						if(GET_PLAYER((PlayerTypes)iJ).isAlive()) {
							iMultiplier += (pLoopCity->AI().AI_playerCloseness(
									(PlayerTypes)iJ, DEFAULT_PLAYER_CLOSENESS, true) + 1) * (iJ + 1);
						}
					}*/
				}
				break;
			case 7: // city event history
				FOR_EACH_CITY(pLoopCity, kPlayer)
				{
					for (iJ = 0; iJ < GC.getNumEventInfos(); iJ++)
						iMultiplier += (iJ+1)*pLoopCity->isEventOccured((EventTypes)iJ);
				}
				break;
			// K-Mod end
			} // end TurnSlice switch
			// <advc.001n>
			aiMultipliers.push_back(iMultiplier);
		}
		if(bFullOOSCheck)
			iMultiplier = (int)(::hash(aiMultipliers) * MAX_INT);
		// </advc.001n>
		if (iMultiplier != 0)
			iValue *= iMultiplier;
	}
	return iValue;
}


int CvGame::calculateOptionsChecksum()
{
	PROFILE_FUNC();

	int iValue = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		for (int iJ = 0; iJ < NUM_PLAYEROPTION_TYPES; iJ++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isOption((PlayerOptionTypes)iJ))
			{
				iValue += (iI * 943097);
				iValue += (iJ * 281541);
			}
		}
	}
	return iValue;
}

// <advc.001n>
bool CvGame::checkInSync() {

	if(!isNetworkMultiPlayer())
		return true;

	int iSyncHash = gDLL->GetSyncOOS(GET_PLAYER(getActivePlayer()).getNetID());
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayer const& kOther = GET_PLAYER((PlayerTypes)i);
		if(kOther.isAlive() && kOther.getID() != getActivePlayer() &&
				(kOther.isHuman() || kOther.isHumanDisabled())) {
			int iOtherSyncHash = gDLL->GetSyncOOS(kOther.getNetID());
			if(iOtherSyncHash != iSyncHash) {
				FAssert(iOtherSyncHash == iSyncHash);
				setAIAutoPlay(0);
				return false;
			}
		}
	}
	return true;
} // </advc.001n>

/*  <advc.003g> Test the platform's floating point behavior (intermediate precision
	and/or rounding). */
int CvGame::FPChecksum() const {

	/*  I've used this together with the _controlfp call at the end to test the
		error message in doFPCheck (i.e. for testing the test) */
	/*if(getActivePlayer() == 0)
		_controlfp(_PC_64, _MCW_PC);*/

	// Test 1: based on https://stackoverflow.com/questions/14749929/c-float-operations-have-different-results-on-i386-and-arm-but-why
	float x = 4.80000019f;
	int result1 = (int)(38000 / x + 10000 / x);
	result1 -= 9995; // 4 or 5
	// Test 2: based on https://stackoverflow.com/questions/11832428/windows-intel-and-ios-arm-differences-in-floating-point-calculations
	x = (-5.241729736328125f * 94.37158203125f) - (-7.25933837890625f * 68.14253997802734f);
	int result2 = ::round(-10000.0 * x); // 5 or 6

	/*if(getActivePlayer()==0)
		_controlfp(_PC_24, _MCW_PC);*/

	return 10 * result1 + result2; // Fold them into a single return value
	// (I'm sure there are simpler and more reliable ways to test FP behavior ...)
}

void CvGame::doFPCheck(int iChecksum, PlayerTypes ePlayer) {

	if(b_mFPTestDone)
		return;
	b_mFPTestDone = true;
	if(iChecksum == FPChecksum())
		return; // Active player is able to reproduce checksum received over the net

	gDLL->getInterfaceIFace()->addHumanMessage(getActivePlayer(), true, GC.getEVENT_MESSAGE_TIME(),
			CvWString::format(L"Your machine's FP test computation has yielded a"
				 L" different result than that of %s. The game may frequently go"
				 L" out of sync due to floating point calculations in the AdvCiv mod.",
				 GET_PLAYER(ePlayer).getName()), NULL, MESSAGE_TYPE_MAJOR_EVENT, NULL,
			(ColorTypes)GC.getInfoTypeForString("COLOR_WARNING_TEXT"));
} // </advc.003g

// <advc.003r>
void CvGame::handleUpdateTimer(UpdateTimerTypes eTimerType) {

	if(m_aiUpdateTimers[eTimerType] < 0)
		return;

	if(m_aiUpdateTimers[eTimerType] == 0) {
		switch(eTimerType) {
		// <advc.085> See CvPlayer::setScoreboardExpanded
		case UPDATE_SCORE_BOARD_DIRTY:
			gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
			/*  For some strange reason, the HUD retains mouse focus after expanding
				the scoreboard, and this is the only remedy I was able to find
				(apart from CvInterface::makeInterfaceDirty, which results in flickering). */
			gDLL->getInterfaceIFace()->makeSelectionListDirty();
			break; // </advc.085>
		// <advc.001w>
		case UPDATE_MOUSE_FOCUS:
			gDLL->getInterfaceIFace()->makeSelectionListDirty();
			break; // </advc.001w>
		// <advc.004j>
		case UPDATE_LOOK_AT_STARTING_PLOT: {
			CvPlot* pStartingPlot = GET_PLAYER(getActivePlayer()).getStartingPlot();
			if(pStartingPlot != NULL)
				gDLL->getInterfaceIFace()->lookAt(pStartingPlot->getPoint(), CAMERALOOKAT_NORMAL);
			break;
			} // </advc.004j>
		default: FAssertMsg(false, "Unknown update timer type");
		}
	}
	m_aiUpdateTimers[eTimerType]--;
} // </advc.003r>

void CvGame::addReplayMessage(ReplayMessageTypes eType, PlayerTypes ePlayer, CvWString pszText, int iPlotX, int iPlotY, ColorTypes eColor)
{
	int iGameTurn = getGameTurn();
	CvReplayMessage* pMessage = new CvReplayMessage(iGameTurn, eType, ePlayer);
	if (NULL != pMessage)
	{
		pMessage->setPlot(iPlotX, iPlotY);
		pMessage->setText(pszText);
		if (NO_COLOR == eColor)
			eColor = (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE");
		pMessage->setColor(eColor);
		m_listReplayMessages.push_back(pMessage);
	}
}

void CvGame::clearReplayMessageMap()
{
	for (ReplayMessageList::const_iterator itList = m_listReplayMessages.begin(); itList != m_listReplayMessages.end(); itList++)
	{
		const CvReplayMessage* pMessage = *itList;
		SAFE_DELETE(pMessage);
	}
	m_listReplayMessages.clear();
}

int CvGame::getReplayMessageTurn(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (-1);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (-1);
	}
	return pMessage->getTurn();
}

ReplayMessageTypes CvGame::getReplayMessageType(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (NO_REPLAY_MESSAGE);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (NO_REPLAY_MESSAGE);
	}
	return pMessage->getType();
}

int CvGame::getReplayMessagePlotX(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (-1);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (-1);
	}
	return pMessage->getPlotX();
}

int CvGame::getReplayMessagePlotY(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (-1);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (-1);
	}
	return pMessage->getPlotY();
}

PlayerTypes CvGame::getReplayMessagePlayer(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (NO_PLAYER);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (NO_PLAYER);
	}
	return pMessage->getPlayer();
}

LPCWSTR CvGame::getReplayMessageText(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (NULL);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (NULL);
	}
	return pMessage->getText().GetCString();
}

ColorTypes CvGame::getReplayMessageColor(uint i) const
{
	if (i >= m_listReplayMessages.size())
	{
		return (NO_COLOR);
	}
	const CvReplayMessage* pMessage =  m_listReplayMessages[i];
	if (NULL == pMessage)
	{
		return (NO_COLOR);
	}
	return pMessage->getColor();
}


uint CvGame::getNumReplayMessages() const
{
	return m_listReplayMessages.size();
}

// Private Functions...

void CvGame::read(FDataStreamBase* pStream)
{
	int iI;

	reset(NO_HANDICAP);

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	if (uiFlag < 1)
	{
		int iEndTurnMessagesSent;
		pStream->Read(&iEndTurnMessagesSent);
	}
	pStream->Read(&m_iElapsedGameTurns);
	pStream->Read(&m_iStartTurn);
	pStream->Read(&m_iStartYear);
	pStream->Read(&m_iEstimateEndTurn);
	pStream->Read(&m_iTurnSlice);
	pStream->Read(&m_iCutoffSlice);
	pStream->Read(&m_iNumGameTurnActive);
	pStream->Read(&m_iNumCities);
	pStream->Read(&m_iTotalPopulation);
	pStream->Read(&m_iTradeRoutes);
	pStream->Read(&m_iFreeTradeCount);
	pStream->Read(&m_iNoNukesCount);
	pStream->Read(&m_iNukesExploded);
	pStream->Read(&m_iMaxPopulation);
	pStream->Read(&m_iMaxLand);
	pStream->Read(&m_iMaxTech);
	pStream->Read(&m_iMaxWonders);
	pStream->Read(&m_iInitPopulation);
	pStream->Read(&m_iInitLand);
	pStream->Read(&m_iInitTech);
	pStream->Read(&m_iInitWonders);
	pStream->Read(&m_iAIAutoPlay);
	/*  advc.127: m_iAIAutoPlay really shouldn't be stored in savegames.
		Auto Play is off when a savegame is loaded, even if it's an autosave
		created during Auto Play, so m_iAIAutoPlay needs to be 0. */
	m_iAIAutoPlay = 0;
	pStream->Read(&m_iGlobalWarmingIndex); // K-Mod
	pStream->Read(&m_iGwEventTally); // K-Mod
	// <advc.003b>
	if(uiFlag >= 4) {
		pStream->Read(&m_iCivPlayersEverAlive);
		pStream->Read(&m_iCivTeamsEverAlive);
	} /* The else case is handled in allGameDataRead - need to read the players and
		 teams first. */
	// </advc.003b>
	// m_uiInitialTime not saved

	pStream->Read(&m_bScoreDirty);
	pStream->Read(&m_bCircumnavigated);
	// m_bDebugMode not saved
	pStream->Read(&m_bFinalInitialized);
	// m_bPbemTurnSent not saved
	pStream->Read(&m_bHotPbemBetweenTurns);
	// m_bPlayerOptionsSent not saved
	pStream->Read(&m_bNukesValid);

	pStream->Read((int*)&m_eHandicap);
	pStream->Read((int*)&m_eAIHandicap); // advc.127
	pStream->Read((int*)&m_ePausePlayer);
	pStream->Read((int*)&m_eBestLandUnit);
	pStream->Read((int*)&m_eWinner);
	pStream->Read((int*)&m_eVictory);
	pStream->Read((int*)&m_eGameState);
	// <advc.106h>
	if(uiFlag >= 6)
		pStream->Read((int*)&m_eInitialActivePlayer);
	else m_eInitialActivePlayer = getActivePlayer(); // </advc.106h>
	// <advc.004m>
	if(uiFlag >= 5) {
		pStream->Read((int*)&m_eCurrentLayer);
		m_bLayerFromSavegame = true;
	} // </advc.004m>
	pStream->ReadString(m_szScriptData);

	if (uiFlag < 1)
	{
		std::vector<int> aiEndTurnMessagesReceived(MAX_PLAYERS);
		pStream->Read(MAX_PLAYERS, &aiEndTurnMessagesReceived[0]);
	}
	pStream->Read(MAX_PLAYERS, m_aiRankPlayer);
	pStream->Read(MAX_PLAYERS, m_aiPlayerRank);
	pStream->Read(MAX_PLAYERS, m_aiPlayerScore);
	pStream->Read(MAX_TEAMS, m_aiRankTeam);
	pStream->Read(MAX_TEAMS, m_aiTeamRank);
	pStream->Read(MAX_TEAMS, m_aiTeamScore);

	pStream->Read(GC.getNumUnitInfos(), m_paiUnitCreatedCount);
	pStream->Read(GC.getNumUnitClassInfos(), m_paiUnitClassCreatedCount);
	pStream->Read(GC.getNumBuildingClassInfos(), m_paiBuildingClassCreatedCount);
	pStream->Read(GC.getNumProjectInfos(), m_paiProjectCreatedCount);
	pStream->Read(GC.getNumCivicInfos(), m_paiForceCivicCount);
	pStream->Read(GC.getNumVoteInfos(), (int*)m_paiVoteOutcome);
	pStream->Read(GC.getNumReligionInfos(), m_paiReligionGameTurnFounded);
	pStream->Read(GC.getNumCorporationInfos(), m_paiCorporationGameTurnFounded);
	pStream->Read(GC.getNumVoteSourceInfos(), m_aiSecretaryGeneralTimer);
	pStream->Read(GC.getNumVoteSourceInfos(), m_aiVoteTimer);
	pStream->Read(GC.getNumVoteSourceInfos(), m_aiDiploVote);

	pStream->Read(GC.getNumSpecialUnitInfos(), m_pabSpecialUnitValid);
	pStream->Read(GC.getNumSpecialBuildingInfos(), m_pabSpecialBuildingValid);
	pStream->Read(GC.getNumReligionInfos(), m_abReligionSlotTaken);

	for (iI=0;iI<GC.getNumReligionInfos();iI++)
	{
		pStream->Read((int*)&m_paHolyCity[iI].eOwner);
		pStream->Read(&m_paHolyCity[iI].iID);
	}

	for (iI=0;iI<GC.getNumCorporationInfos();iI++)
	{
		pStream->Read((int*)&m_paHeadquarters[iI].eOwner);
		pStream->Read(&m_paHeadquarters[iI].iID);
	}

	{
		CvWString szBuffer;
		uint iSize;

		m_aszDestroyedCities.clear();
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			pStream->ReadString(szBuffer);
			m_aszDestroyedCities.push_back(szBuffer);
		}

		m_aszGreatPeopleBorn.clear();
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			pStream->ReadString(szBuffer);
			m_aszGreatPeopleBorn.push_back(szBuffer);
		}
	}

	ReadStreamableFFreeListTrashArray(m_deals, pStream);
	ReadStreamableFFreeListTrashArray(m_voteSelections, pStream);
	ReadStreamableFFreeListTrashArray(m_votesTriggered, pStream);

	m_mapRand.read(pStream);
	m_sorenRand.read(pStream);
	// <advc.250b>
	if(isOption(GAMEOPTION_SPAH))
		m_pSpah->read(pStream); // </advc.250b>
	// <advc.701>
	if(uiFlag >= 2) {
		if(isOption(GAMEOPTION_RISE_FALL))
			m_pRiseFall->read(pStream);
	}
	else { // Options have been shuffled around
		setOption(GAMEOPTION_NEW_RANDOM_SEED, isOption(GAMEOPTION_RISE_FALL));
		setOption(GAMEOPTION_RISE_FALL, false);
	} // </advc.701>
	{
		clearReplayMessageMap();
		ReplayMessageList::_Alloc::size_type iSize;
		pStream->Read(&iSize);
		for (ReplayMessageList::_Alloc::size_type i = 0; i < iSize; i++)
		{
			CvReplayMessage* pMessage = new CvReplayMessage(0);
			if (NULL != pMessage)
			{
				pMessage->read(*pStream);
			}
			m_listReplayMessages.push_back(pMessage);
		}
	}
	// m_pReplayInfo not saved

	pStream->Read(&m_iNumSessions);
	if (!isNetworkMultiPlayer())
	{
		++m_iNumSessions;
	}

	{
		int iSize;
		m_aPlotExtraYields.clear();
		pStream->Read(&iSize);
		for (int i = 0; i < iSize; ++i)
		{
			PlotExtraYield kPlotYield;
			kPlotYield.read(pStream);
			m_aPlotExtraYields.push_back(kPlotYield);
		}
	}

	{
		int iSize;
		m_aPlotExtraCosts.clear();
		pStream->Read(&iSize);
		for (int i = 0; i < iSize; ++i)
		{
			PlotExtraCost kPlotCost;
			kPlotCost.read(pStream);
			m_aPlotExtraCosts.push_back(kPlotCost);
		}
	}

	{
		int iSize;
		m_mapVoteSourceReligions.clear();
		pStream->Read(&iSize);
		for (int i = 0; i < iSize; ++i)
		{
			VoteSourceTypes eVoteSource;
			ReligionTypes eReligion;
			pStream->Read((int*)&eVoteSource);
			pStream->Read((int*)&eReligion);
			m_mapVoteSourceReligions[eVoteSource] = eReligion;
		}
	}

	{
		int iSize;
		m_aeInactiveTriggers.clear();
		pStream->Read(&iSize);
		for (int i = 0; i < iSize; ++i)
		{
			int iTrigger;
			pStream->Read(&iTrigger);
			m_aeInactiveTriggers.push_back((EventTriggerTypes)iTrigger);
		}
	}

	// Get the active player information from the initialization structure
	if (!isGameMultiPlayer())
	{
		for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isHuman())
			{
				setActivePlayer((PlayerTypes)iI);
				break;
			}
		}
		addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getActivePlayer(), gDLL->getText("TXT_KEY_MISC_RELOAD", m_iNumSessions));
	}

	if (isOption(GAMEOPTION_NEW_RANDOM_SEED))
	{
		if (!isNetworkMultiPlayer())
		{
			m_sorenRand.reseed(timeGetTime());
		}
	}

	pStream->Read(&m_iShrineBuildingCount);
	pStream->Read(GC.getNumBuildingInfos(), m_aiShrineBuilding);
	pStream->Read(GC.getNumBuildingInfos(), m_aiShrineReligion);
	pStream->Read(&m_iNumCultureVictoryCities);
	pStream->Read(&m_eCultureVictoryCultureLevel);
	// <advc.052>
	if(uiFlag >= 3)
		pStream->Read(&m_bScenario); // </advc.052>
	m_iTurnLoadedFromSave = m_iElapsedGameTurns; // advc.044
	applyOptionEffects(); // advc.310
	b_mFPTestDone = !isNetworkMultiPlayer(); // advc.003g
}


void CvGame::write(FDataStreamBase* pStream)
{
	int iI;

	uint uiFlag=1;
	uiFlag = 2; // advc.701: R&F option
	uiFlag = 3; // advc.052
	uiFlag = 4; // advc.003b: Civs and teams EverAlive tracked
	uiFlag = 5; // advc.004m
	uiFlag = 6; // advc.106h
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iElapsedGameTurns);
	pStream->Write(m_iStartTurn);
	pStream->Write(m_iStartYear);
	pStream->Write(m_iEstimateEndTurn);
	pStream->Write(m_iTurnSlice);
	pStream->Write(m_iCutoffSlice);
	pStream->Write(m_iNumGameTurnActive);
	pStream->Write(m_iNumCities);
	pStream->Write(m_iTotalPopulation);
	pStream->Write(m_iTradeRoutes);
	pStream->Write(m_iFreeTradeCount);
	pStream->Write(m_iNoNukesCount);
	pStream->Write(m_iNukesExploded);
	pStream->Write(m_iMaxPopulation);
	pStream->Write(m_iMaxLand);
	pStream->Write(m_iMaxTech);
	pStream->Write(m_iMaxWonders);
	pStream->Write(m_iInitPopulation);
	pStream->Write(m_iInitLand);
	pStream->Write(m_iInitTech);
	pStream->Write(m_iInitWonders);
	pStream->Write(m_iAIAutoPlay);
	pStream->Write(m_iGlobalWarmingIndex); // K-Mod
	pStream->Write(m_iGwEventTally); // K-Mod
	// <advc.003b>
	pStream->Write(m_iCivPlayersEverAlive);
	pStream->Write(m_iCivTeamsEverAlive);
	// </advc.003b>
	// m_uiInitialTime not saved

	pStream->Write(m_bScoreDirty);
	pStream->Write(m_bCircumnavigated);
	// m_bDebugMode not saved
	pStream->Write(m_bFinalInitialized);
	// m_bPbemTurnSent not saved
	pStream->Write(m_bHotPbemBetweenTurns);
	// m_bPlayerOptionsSent not saved
	pStream->Write(m_bNukesValid);

	pStream->Write(m_eHandicap);
	pStream->Write(m_eAIHandicap); // advc.127
	pStream->Write(m_ePausePlayer);
	pStream->Write(m_eBestLandUnit);
	pStream->Write(m_eWinner);
	pStream->Write(m_eVictory);
	pStream->Write(m_eGameState);
	pStream->Write(m_eInitialActivePlayer); // advc.106h
	pStream->Write(m_eCurrentLayer); // advc.004m

	pStream->WriteString(m_szScriptData);

	pStream->Write(MAX_PLAYERS, m_aiRankPlayer);
	pStream->Write(MAX_PLAYERS, m_aiPlayerRank);
	pStream->Write(MAX_PLAYERS, m_aiPlayerScore);
	pStream->Write(MAX_TEAMS, m_aiRankTeam);
	pStream->Write(MAX_TEAMS, m_aiTeamRank);
	pStream->Write(MAX_TEAMS, m_aiTeamScore);

	pStream->Write(GC.getNumUnitInfos(), m_paiUnitCreatedCount);
	pStream->Write(GC.getNumUnitClassInfos(), m_paiUnitClassCreatedCount);
	pStream->Write(GC.getNumBuildingClassInfos(), m_paiBuildingClassCreatedCount);
	pStream->Write(GC.getNumProjectInfos(), m_paiProjectCreatedCount);
	pStream->Write(GC.getNumCivicInfos(), m_paiForceCivicCount);
	pStream->Write(GC.getNumVoteInfos(), (int*)m_paiVoteOutcome);
	pStream->Write(GC.getNumReligionInfos(), m_paiReligionGameTurnFounded);
	pStream->Write(GC.getNumCorporationInfos(), m_paiCorporationGameTurnFounded);
	pStream->Write(GC.getNumVoteSourceInfos(), m_aiSecretaryGeneralTimer);
	pStream->Write(GC.getNumVoteSourceInfos(), m_aiVoteTimer);
	pStream->Write(GC.getNumVoteSourceInfos(), m_aiDiploVote);

	pStream->Write(GC.getNumSpecialUnitInfos(), m_pabSpecialUnitValid);
	pStream->Write(GC.getNumSpecialBuildingInfos(), m_pabSpecialBuildingValid);
	pStream->Write(GC.getNumReligionInfos(), m_abReligionSlotTaken);

	for (iI=0;iI<GC.getNumReligionInfos();iI++)
	{
		pStream->Write(m_paHolyCity[iI].eOwner);
		pStream->Write(m_paHolyCity[iI].iID);
	}

	for (iI=0;iI<GC.getNumCorporationInfos();iI++)
	{
		pStream->Write(m_paHeadquarters[iI].eOwner);
		pStream->Write(m_paHeadquarters[iI].iID);
	}

	{
		std::vector<CvWString>::iterator it;

		pStream->Write(m_aszDestroyedCities.size());
		for (it = m_aszDestroyedCities.begin(); it != m_aszDestroyedCities.end(); it++)
		{
			pStream->WriteString(*it);
		}

		pStream->Write(m_aszGreatPeopleBorn.size());
		for (it = m_aszGreatPeopleBorn.begin(); it != m_aszGreatPeopleBorn.end(); it++)
		{
			pStream->WriteString(*it);
		}
	}

	WriteStreamableFFreeListTrashArray(m_deals, pStream);
	WriteStreamableFFreeListTrashArray(m_voteSelections, pStream);
	WriteStreamableFFreeListTrashArray(m_votesTriggered, pStream);

	m_mapRand.write(pStream);
	m_sorenRand.write(pStream);
	// <advc.250b>
	if(isOption(GAMEOPTION_SPAH))
		m_pSpah->write(pStream); // </advc.250b>
	// <advc.701>
	if(isOption(GAMEOPTION_RISE_FALL))
		m_pRiseFall->write(pStream); // </advc.701>
	ReplayMessageList::_Alloc::size_type iSize = m_listReplayMessages.size();
	pStream->Write(iSize);
	for (ReplayMessageList::const_iterator it = m_listReplayMessages.begin(); it != m_listReplayMessages.end(); it++)
	{
		const CvReplayMessage* pMessage = *it;
		if (NULL != pMessage)
		{
			pMessage->write(*pStream);
		}
	}
	// m_pReplayInfo not saved

	pStream->Write(m_iNumSessions);

	pStream->Write(m_aPlotExtraYields.size());
	for (std::vector<PlotExtraYield>::iterator it = m_aPlotExtraYields.begin(); it != m_aPlotExtraYields.end(); ++it)
	{
		(*it).write(pStream);
	}

	pStream->Write(m_aPlotExtraCosts.size());
	for (std::vector<PlotExtraCost>::iterator it = m_aPlotExtraCosts.begin(); it != m_aPlotExtraCosts.end(); ++it)
	{
		(*it).write(pStream);
	}

	pStream->Write(m_mapVoteSourceReligions.size());
	for (stdext::hash_map<VoteSourceTypes, ReligionTypes>::iterator it = m_mapVoteSourceReligions.begin(); it != m_mapVoteSourceReligions.end(); ++it)
	{
		pStream->Write(it->first);
		pStream->Write(it->second);
	}

	pStream->Write(m_aeInactiveTriggers.size());
	for (std::vector<EventTriggerTypes>::iterator it = m_aeInactiveTriggers.begin(); it != m_aeInactiveTriggers.end(); ++it)
	{
		pStream->Write(*it);
	}

	pStream->Write(m_iShrineBuildingCount);
	pStream->Write(GC.getNumBuildingInfos(), m_aiShrineBuilding);
	pStream->Write(GC.getNumBuildingInfos(), m_aiShrineReligion);
	pStream->Write(m_iNumCultureVictoryCities);
	pStream->Write(m_eCultureVictoryCultureLevel);
	pStream->Write(m_bScenario); // advc.052
}

void CvGame::writeReplay(FDataStreamBase& stream, PlayerTypes ePlayer)
{
	GET_PLAYER(ePlayer).setSavingReplay(false); // advc.106i
	SAFE_DELETE(m_pReplayInfo);
	m_pReplayInfo = new CvReplayInfo();
	if (m_pReplayInfo)
	{
		m_pReplayInfo->createInfo(ePlayer);
		// <advc.707>
		if(isOption(GAMEOPTION_RISE_FALL))
			m_pReplayInfo->setFinalScore(m_pRiseFall->getFinalRiseScore());
		// </advc.707>
		m_pReplayInfo->write(stream);
	}
}

/*  <advc.003> When loading a savegame, this function is called once all
	read functions have been called. */
void CvGame::allGameDataRead() {

	// <advc.003b> Savegame compatibility (uiFlag<4)
	if(m_iCivPlayersEverAlive == 0)
		m_iCivPlayersEverAlive = countCivPlayersEverAlive();
	if(m_iCivTeamsEverAlive == 0)
		m_iCivTeamsEverAlive = countCivTeamsEverAlive();
	// </advc.003b>
	getWPAI.update(); // advc.104
	GET_PLAYER(getActivePlayer()).validateDiplomacy(); // advc.134a
	m_bAllGameDataRead = true;
}

// Called once the EXE signals that graphics have been initialized (w/e that means exactly)
void CvGame::onGraphicsInitialized() {

	// (Nothing to be done here currently)
}
// </advc.003>

void CvGame::saveReplay(PlayerTypes ePlayer)
{	// advc.106i: Hack to prepend sth. to the replay file name
	GET_PLAYER(ePlayer).setSavingReplay(true);
	gDLL->getEngineIFace()->SaveReplay(ePlayer);
	// advc.106i: Probably redundant b/c CvGame::writeReplay already sets it to false
	GET_PLAYER(ePlayer).setSavingReplay(false);
}


void CvGame::showEndGameSequence()
{
	CvPopupInfo* pInfo;
	CvWString szBuffer;
	int iI;

	long iHours = getMinutesPlayed() / 60;
	long iMinutes = getMinutesPlayed() % 60;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& player = GET_PLAYER((PlayerTypes)iI);
		if (player.isHuman())
		{
			addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, (PlayerTypes)iI, gDLL->getText("TXT_KEY_MISC_TIME_SPENT", iHours, iMinutes));

			pInfo = new CvPopupInfo(BUTTONPOPUP_TEXT);
			if (NULL != pInfo)
			{
				if ((getWinner() != NO_TEAM) && (getVictory() != NO_VICTORY))
				{
					pInfo->setText(gDLL->getText("TXT_KEY_GAME_WON", GET_TEAM(getWinner()).getName().GetCString(), GC.getVictoryInfo(getVictory()).getTextKeyWide()));
				}
				else
				{
					pInfo->setText(gDLL->getText("TXT_KEY_MISC_DEFEAT"));
				}
				player.addPopup(pInfo);
			}

			if (getWinner() == player.getTeam())
			{
				if (!CvString(GC.getVictoryInfo(getVictory()).getMovie()).empty())
				{
					// show movie
					pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN);
					if (NULL != pInfo)
					{
						pInfo->setText(L"showVictoryMovie");
						pInfo->setData1((int)getVictory());
						player.addPopup(pInfo);
					}
				}
				else if (GC.getVictoryInfo(getVictory()).isDiploVote())
				{
					pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN);
					if (NULL != pInfo)
					{
						pInfo->setText(L"showUnVictoryScreen");
						player.addPopup(pInfo);
					}
				}
			}

			// show replay
			pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN);
			if (NULL != pInfo)
			{
				pInfo->setText(L"showReplay");
				pInfo->setData1(iI);
				/*  advc.106i (comment): The BtS comment below means that the HoF
					is not shown right after the player exits from the Replay screen.
					That it still gets shown later on is intentional (see a few
					lines below). */
				pInfo->setOption1(false); // don't go to HOF on exit
				player.addPopup(pInfo);
			}

			// show top cities / stats
			pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN);
			if (NULL != pInfo)
			{
				pInfo->setText(L"showInfoScreen");
				pInfo->setData1(0);
				pInfo->setData2(1);
				player.addPopup(pInfo);
			}

			// show Dan
			pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN);
			if (NULL != pInfo)
			{
				pInfo->setText(L"showDanQuayleScreen");
				player.addPopup(pInfo);
			}

			// show Hall of Fame
			pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN);
			if (NULL != pInfo)
			{
				pInfo->setText(L"showHallOfFame");
				player.addPopup(pInfo);
			}
		}
	}
}

CvReplayInfo* CvGame::getReplayInfo() const
{
	return m_pReplayInfo;
}

void CvGame::setReplayInfo(CvReplayInfo* pReplay)
{
	SAFE_DELETE(m_pReplayInfo);
	m_pReplayInfo = pReplay;
}

bool CvGame::hasSkippedSaveChecksum() const
{
	return gDLL->hasSkippedSaveChecksum();
}

void CvGame::addPlayer(PlayerTypes eNewPlayer, LeaderHeadTypes eLeader, CivilizationTypes eCiv)
{
	// UNOFFICIAL_PATCH Start
	// * Fixed bug with colonies who occupy recycled player slots showing the old leader or civ names.
	CvWString szEmptyString = L"";
	LeaderHeadTypes eOldLeader = GET_PLAYER(eNewPlayer).getLeaderType();
	if (eOldLeader != NO_LEADER && eOldLeader != eLeader)
	{
		GC.getInitCore().setLeaderName(eNewPlayer, szEmptyString);
	}
	CivilizationTypes eOldCiv = GET_PLAYER(eNewPlayer).getCivilizationType();
	if (eOldCiv != NO_CIVILIZATION && eOldCiv != eCiv)
	{
		GC.getInitCore().setCivAdjective(eNewPlayer, szEmptyString);
		GC.getInitCore().setCivDescription(eNewPlayer, szEmptyString);
		GC.getInitCore().setCivShortDesc(eNewPlayer, szEmptyString);
	}
	// UNOFFICIAL_PATCH End
	PlayerColorTypes eColor = (PlayerColorTypes)GC.getCivilizationInfo(eCiv).getDefaultPlayerColor();

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (eColor == NO_PLAYERCOLOR || (GET_PLAYER((PlayerTypes)iI).getPlayerColor() == eColor
				/*  UNOFFICIAL_PATCH, Bugfix, 12/30/08, jdog5000:
					Don't invalidate color choice if it's taken by this player */
				&& (PlayerTypes)iI != eNewPlayer))
		{
			for (int iK = 0; iK < GC.getNumPlayerColorInfos(); iK++)
			{
				if (iK != GC.getCivilizationInfo((CivilizationTypes)GC.getDefineINT("BARBARIAN_CIVILIZATION")).getDefaultPlayerColor())
				{
					bool bValid = true;

					for (int iL = 0; iL < MAX_CIV_PLAYERS; iL++)
					{
						if (GET_PLAYER((PlayerTypes)iL).getPlayerColor() == iK)
						{
							bValid = false;
							break;
						}
					}

					if (bValid)
					{
						eColor = (PlayerColorTypes)iK;
						iI = MAX_CIV_PLAYERS;
						break;
					}
				}
			}
		}
	}

	TeamTypes eTeam = GET_PLAYER(eNewPlayer).getTeam();
	GC.getInitCore().setLeader(eNewPlayer, eLeader);
	GC.getInitCore().setCiv(eNewPlayer, eCiv);
	GC.getInitCore().setSlotStatus(eNewPlayer, SS_COMPUTER);
	GC.getInitCore().setColor(eNewPlayer, eColor);
	// BETTER_BTS_AI_MOD, Bugfix, 12/30/08, jdog5000: START
	// Team init now handled when appropriate in player initInGame
	/*GET_TEAM(eTeam).init(eTeam);
	GET_PLAYER(eNewPlayer).init(eNewPlayer);*/
	// Standard player init is written for beginning of game, it resets global random events for this player only among other flaws
	GET_PLAYER(eNewPlayer).initInGame(eNewPlayer);
	// BETTER_BTS_AI_MOD: END
}

//	BETTER_BTS_AI_MOD, Debug, 8/1/08, jdog5000: START
void CvGame::changeHumanPlayer(PlayerTypes eNewHuman)
{
	PlayerTypes eCurHuman = getActivePlayer();
	/*  <advc.127> Rearranged code b/c of a change in CvPlayer::isOption.
		Important for advc.706. */
	if(eNewHuman == eCurHuman) {
		if(getActivePlayer() != eNewHuman)
			setActivePlayer(eNewHuman, false);
		GET_PLAYER(eNewHuman).setIsHuman(true);
		GET_PLAYER(eNewHuman).updateHuman();
		return;
	}
	GET_PLAYER(eCurHuman).setIsHuman(true);
	GET_PLAYER(eNewHuman).setIsHuman(true);
	GET_PLAYER(eCurHuman).updateHuman();
	GET_PLAYER(eNewHuman).updateHuman();
	for (int iI = 0; iI < NUM_PLAYEROPTION_TYPES; iI++)
	{
		GET_PLAYER(eNewHuman).setOption((PlayerOptionTypes)iI,
				GET_PLAYER(eCurHuman).isOption((PlayerOptionTypes)iI));
	}
	for (int iI = 0; iI < NUM_PLAYEROPTION_TYPES; iI++)
	{
		gDLL->sendPlayerOption(((PlayerOptionTypes)iI),
				GET_PLAYER(eNewHuman).isOption((PlayerOptionTypes)iI));
	} // </advc.127>
	setActivePlayer(eNewHuman, false);

	GET_PLAYER(eCurHuman).setIsHuman(false);
	GET_PLAYER(eCurHuman).updateHuman(); // advc.127
} // BETTER_BTS_AI_MOD: END


bool CvGame::isCompetingCorporation(CorporationTypes eCorporation1, CorporationTypes eCorporation2) const
{
	// K-Mod
	if (eCorporation1 == eCorporation2)
		return false;
	// K-Mod end

	bool bShareResources = false;

	for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES() && !bShareResources; ++i)
	{
		if (GC.getCorporationInfo(eCorporation1).getPrereqBonus(i) != NO_BONUS)
		{
			for (int j = 0; j < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++j)
			{
				if (GC.getCorporationInfo(eCorporation2).getPrereqBonus(j) != NO_BONUS)
				{
					if (GC.getCorporationInfo(eCorporation1).getPrereqBonus(i) == GC.getCorporationInfo(eCorporation2).getPrereqBonus(j))
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

int CvGame::getPlotExtraYield(int iX, int iY, YieldTypes eYield) const
{
	for (std::vector<PlotExtraYield>::const_iterator it = m_aPlotExtraYields.begin(); it != m_aPlotExtraYields.end(); ++it)
	{
		if ((*it).m_iX == iX && (*it).m_iY == iY)
		{
			return (*it).m_aeExtraYield[eYield];
		}
	}

	return 0;
}

void CvGame::setPlotExtraYield(int iX, int iY, YieldTypes eYield, int iExtraYield)
{
	bool bFound = false;

	for (std::vector<PlotExtraYield>::iterator it = m_aPlotExtraYields.begin(); it != m_aPlotExtraYields.end(); ++it)
	{
		if ((*it).m_iX == iX && (*it).m_iY == iY)
		{
			(*it).m_aeExtraYield[eYield] += iExtraYield;
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		PlotExtraYield kExtraYield;
		kExtraYield.m_iX = iX;
		kExtraYield.m_iY = iY;
		for (int i = 0; i < NUM_YIELD_TYPES; ++i)
		{
			if (eYield == i)
			{
				kExtraYield.m_aeExtraYield.push_back(iExtraYield);
			}
			else
			{
				kExtraYield.m_aeExtraYield.push_back(0);
			}
		}
		m_aPlotExtraYields.push_back(kExtraYield);
	}

	CvPlot* pPlot = GC.getMap().plot(iX, iY);
	if (NULL != pPlot)
	{
		pPlot->updateYield();
	}
}

void CvGame::removePlotExtraYield(int iX, int iY)
{
	for (std::vector<PlotExtraYield>::iterator it = m_aPlotExtraYields.begin(); it != m_aPlotExtraYields.end(); ++it)
	{
		if ((*it).m_iX == iX && (*it).m_iY == iY)
		{
			m_aPlotExtraYields.erase(it);
			break;
		}
	}

	CvPlot* pPlot = GC.getMap().plot(iX, iY);
	if (NULL != pPlot)
	{
		pPlot->updateYield();
	}
}

int CvGame::getPlotExtraCost(int iX, int iY) const
{
	for (std::vector<PlotExtraCost>::const_iterator it = m_aPlotExtraCosts.begin(); it != m_aPlotExtraCosts.end(); ++it)
	{
		if ((*it).m_iX == iX && (*it).m_iY == iY)
		{
			return (*it).m_iCost;
		}
	}

	return 0;
}

void CvGame::changePlotExtraCost(int iX, int iY, int iCost)
{
	bool bFound = false;

	for (std::vector<PlotExtraCost>::iterator it = m_aPlotExtraCosts.begin(); it != m_aPlotExtraCosts.end(); ++it)
	{
		if ((*it).m_iX == iX && (*it).m_iY == iY)
		{
			(*it).m_iCost += iCost;
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		PlotExtraCost kExtraCost;
		kExtraCost.m_iX = iX;
		kExtraCost.m_iY = iY;
		kExtraCost.m_iCost = iCost;
		m_aPlotExtraCosts.push_back(kExtraCost);
	}
}

void CvGame::removePlotExtraCost(int iX, int iY)
{
	for (std::vector<PlotExtraCost>::iterator it = m_aPlotExtraCosts.begin(); it != m_aPlotExtraCosts.end(); ++it)
	{
		if ((*it).m_iX == iX && (*it).m_iY == iY)
		{
			m_aPlotExtraCosts.erase(it);
			break;
		}
	}
}

ReligionTypes CvGame::getVoteSourceReligion(VoteSourceTypes eVoteSource) const
{
	stdext::hash_map<VoteSourceTypes, ReligionTypes>::const_iterator it;

	it = m_mapVoteSourceReligions.find(eVoteSource);
	if (it == m_mapVoteSourceReligions.end())
	{
		return NO_RELIGION;
	}

	return it->second;
}

void CvGame::setVoteSourceReligion(VoteSourceTypes eVoteSource, ReligionTypes eReligion, bool bAnnounce)
{
	m_mapVoteSourceReligions[eVoteSource] = eReligion;

	if (bAnnounce)
	{
		if (NO_RELIGION != eReligion)
		{
			CvWString szBuffer = gDLL->getText("TXT_KEY_VOTE_SOURCE_RELIGION", GC.getReligionInfo(eReligion).getTextKeyWide(), GC.getReligionInfo(eReligion).getAdjectiveKey(), GC.getVoteSourceInfo(eVoteSource).getTextKeyWide());

			for (int iI = 0; iI < MAX_PLAYERS; iI++)
			{
				PlayerTypes ePlayer = (PlayerTypes)iI;
				if (GET_PLAYER(ePlayer).isAlive())
				{	// <advc.127b>
					std::pair<int,int> xy = getVoteSourceXY(eVoteSource,
							TEAMID(ePlayer), true); // </advc.127>
					gDLL->getInterfaceIFace()->addHumanMessage(ePlayer, false,
							GC.getEVENT_MESSAGE_TIME(), szBuffer,
							GC.getReligionInfo(eReligion).getSound(),
							MESSAGE_TYPE_MAJOR_EVENT, NULL,
							(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
							xy.first, xy.second); // advc.127b
				}
			}
		}
	}
}


int CvGame::getShrineBuildingCount(ReligionTypes eReligion)
{
	int	iShrineBuildingCount = 0;

	if (eReligion == NO_RELIGION)
		iShrineBuildingCount = m_iShrineBuildingCount;
	else for (int iI = 0; iI < m_iShrineBuildingCount; iI++)
		if (m_aiShrineReligion[iI] == eReligion)
			iShrineBuildingCount++;

	return iShrineBuildingCount;
}

BuildingTypes CvGame::getShrineBuilding(int eIndex, ReligionTypes eReligion)
{
	FAssertMsg(eIndex >= 0 && eIndex < m_iShrineBuildingCount, "invalid index to CvGame::getShrineBuilding");

	BuildingTypes eBuilding = NO_BUILDING;

	if (eIndex >= 0 && eIndex < m_iShrineBuildingCount)
	{
		if (eReligion == NO_RELIGION)
			eBuilding = (BuildingTypes) m_aiShrineBuilding[eIndex];
		else for (int iI = 0, iReligiousBuilding = 0; iI < m_iShrineBuildingCount; iI++)
			if (m_aiShrineReligion[iI] == (int) eReligion)
			{
				if (iReligiousBuilding == eIndex)
				{
					// found it
					eBuilding = (BuildingTypes) m_aiShrineBuilding[iI];
					break;
				}

				iReligiousBuilding++;
			}
	}

	return eBuilding;
}

void CvGame::changeShrineBuilding(BuildingTypes eBuilding, ReligionTypes eReligion, bool bRemove)
{
	FAssertMsg(eBuilding >= 0 && eBuilding < GC.getNumBuildingInfos(), "invalid index to CvGame::changeShrineBuilding");
	FAssertMsg(bRemove || m_iShrineBuildingCount < GC.getNumBuildingInfos(), "trying to add too many buildings to CvGame::changeShrineBuilding");

	if (bRemove)
	{
		bool bFound = false;

		for (int iI = 0; iI < m_iShrineBuildingCount; iI++)
		{
			if (!bFound)
			{
				// note, eReligion is not important if we removing, since each building is always one religion
				if (m_aiShrineBuilding[iI] == (int) eBuilding)
					bFound = true;
			}

			if (bFound)
			{
				int iToMove = iI + 1;
				if (iToMove < m_iShrineBuildingCount)
				{
					m_aiShrineBuilding[iI] = m_aiShrineBuilding[iToMove];
					m_aiShrineReligion[iI] = m_aiShrineReligion[iToMove];
				}
				else
				{
					m_aiShrineBuilding[iI] = (int) NO_BUILDING;
					m_aiShrineReligion[iI] = (int) NO_RELIGION;
				}
			}

		if (bFound)
			m_iShrineBuildingCount--;

		}
	}
	else if (m_iShrineBuildingCount < GC.getNumBuildingInfos())
	{
		// add this item to the end
		m_aiShrineBuilding[m_iShrineBuildingCount] = eBuilding;
		m_aiShrineReligion[m_iShrineBuildingCount] = eReligion;
		m_iShrineBuildingCount++;
	}

}

bool CvGame::culturalVictoryValid() /* advc.003: */ const
{
	return (m_iNumCultureVictoryCities > 0);
}

int CvGame::culturalVictoryNumCultureCities() /* advc.003: */ const
{
	return m_iNumCultureVictoryCities;
}

CultureLevelTypes CvGame::culturalVictoryCultureLevel() /* advc.003: */  const
{
	if (m_iNumCultureVictoryCities > 0)
	{
		return (CultureLevelTypes)m_eCultureVictoryCultureLevel;
	}

	return NO_CULTURELEVEL;
}

int CvGame::getCultureThreshold(CultureLevelTypes eLevel) const
{
	int iThreshold = GC.getCultureLevelInfo(eLevel).getSpeedThreshold(getGameSpeedType());
	if (isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		iThreshold *= 100 + GC.getDefineINT("NO_ESPIONAGE_CULTURE_LEVEL_MODIFIER");
		iThreshold /= 100;
	} // <advc.126>
	int const iExempt = 50; // Don't adjust thresholds below "developing"
	if(iThreshold >= iExempt)
	{
		iThreshold *= GC.getEraInfo(getStartEra()).getCulturePercent();
		iThreshold /= 100; // </advc.126>
		// <advc.251>
		iThreshold *= GC.getHandicapInfo(getHandicapType()).getCultureLevelPercent();
		iThreshold /= 100;
		iThreshold = std::max(iThreshold, iExempt);
	} // </advc.251>
	return iThreshold;
}

void CvGame::doUpdateCacheOnTurn()
{
	int	iI;

	// reset shrine count
	m_iShrineBuildingCount = 0;

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		CvBuildingInfo&	kBuildingInfo = GC.getBuildingInfo((BuildingTypes) iI);

		// if it is for holy city, then its a shrine-thing, add it
		if (kBuildingInfo.getHolyCity() != NO_RELIGION)
		{
			changeShrineBuilding((BuildingTypes) iI, (ReligionTypes) kBuildingInfo.getReligionType());
		}
	}

	// reset cultural victories
	m_iNumCultureVictoryCities = 0;
	for (iI = 0; iI < GC.getNumVictoryInfos(); iI++)
	{
		if (isVictoryValid((VictoryTypes) iI))
		{
			CvVictoryInfo& kVictoryInfo = GC.getVictoryInfo((VictoryTypes) iI);
			if (kVictoryInfo.getCityCulture() > 0)
			{
				int iNumCultureCities = kVictoryInfo.getNumCultureCities();
				if (iNumCultureCities > m_iNumCultureVictoryCities)
				{
					m_iNumCultureVictoryCities = iNumCultureCities;
					m_eCultureVictoryCultureLevel = kVictoryInfo.getCityCulture();
				}
			}
		}
	}

	// K-Mod. (todo: move all of that stuff above somewhere else. That doesn't need to be updated every turn!)
	CvSelectionGroup::path_finder.Reset(); // (one of the few manual resets we need)
	m_ActivePlayerCycledGroups.clear();
	// K-Mod end
}

VoteSelectionData* CvGame::getVoteSelection(int iID) const
{
	return ((VoteSelectionData*)(m_voteSelections.getAt(iID)));
}

VoteSelectionData* CvGame::addVoteSelection(VoteSourceTypes eVoteSource)
{
	VoteSelectionData* pData = ((VoteSelectionData*)(m_voteSelections.add()));

	if  (NULL != pData)
	{
		pData->eVoteSource = eVoteSource;

		for (int iI = 0; iI < GC.getNumVoteInfos(); iI++)
		{
			if (GC.getVoteInfo((VoteTypes)iI).isVoteSourceType(eVoteSource))
			{
				if (isChooseElection((VoteTypes)iI))
				{
					VoteSelectionSubData kData;
					kData.eVote = (VoteTypes)iI;
					kData.iCityId = -1;
					kData.ePlayer = NO_PLAYER;
					kData.eOtherPlayer = NO_PLAYER;

					if (GC.getVoteInfo(kData.eVote).isOpenBorders())
					{
						if (isValidVoteSelection(eVoteSource, kData))
						{
							kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_OPEN_BORDERS", getVoteRequired(kData.eVote, eVoteSource), countPossibleVote(kData.eVote, eVoteSource));
							pData->aVoteOptions.push_back(kData);
						}
					}
					else if (GC.getVoteInfo(kData.eVote).isDefensivePact())
					{
						if (isValidVoteSelection(eVoteSource, kData))
						{
							kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_DEFENSIVE_PACT", getVoteRequired(kData.eVote, eVoteSource), countPossibleVote(kData.eVote, eVoteSource));
							pData->aVoteOptions.push_back(kData);
						}
					}
					else if (GC.getVoteInfo(kData.eVote).isForcePeace())
					{
						for (int iTeam1 = 0; iTeam1 < MAX_CIV_TEAMS; ++iTeam1)
						{
							CvTeam& kTeam1 = GET_TEAM((TeamTypes)iTeam1);

							if (kTeam1.isAlive())
							{
								kData.ePlayer = kTeam1.getLeaderID();

								if (isValidVoteSelection(eVoteSource, kData))
								{
									kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_FORCE_PEACE", kTeam1.getName().GetCString(), getVoteRequired(kData.eVote, eVoteSource), countPossibleVote(kData.eVote, eVoteSource));
									pData->aVoteOptions.push_back(kData);
								}
							}
						}
					}
					else if (GC.getVoteInfo(kData.eVote).isForceNoTrade())
					{
						for (int iTeam1 = 0; iTeam1 < MAX_CIV_TEAMS; ++iTeam1)
						{
							CvTeam& kTeam1 = GET_TEAM((TeamTypes)iTeam1);

							if (kTeam1.isAlive())
							{
								kData.ePlayer = kTeam1.getLeaderID();

								if (isValidVoteSelection(eVoteSource, kData))
								{
									kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_FORCE_NO_TRADE", kTeam1.getName().GetCString(), getVoteRequired(kData.eVote, eVoteSource), countPossibleVote(kData.eVote, eVoteSource));
									pData->aVoteOptions.push_back(kData);
								}
							}
						}
					}
					else if (GC.getVoteInfo(kData.eVote).isForceWar())
					{
						for (int iTeam1 = 0; iTeam1 < MAX_CIV_TEAMS; ++iTeam1)
						{
							CvTeam& kTeam1 = GET_TEAM((TeamTypes)iTeam1);

							if (kTeam1.isAlive())
							{
								kData.ePlayer = kTeam1.getLeaderID();

								if (isValidVoteSelection(eVoteSource, kData))
								{
									kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_FORCE_WAR", kTeam1.getName().GetCString(), getVoteRequired(kData.eVote, eVoteSource), countPossibleVote(kData.eVote, eVoteSource));
									pData->aVoteOptions.push_back(kData);
								}
							}
						}
					}
					else if (GC.getVoteInfo(kData.eVote).isAssignCity())
					{
						for (int iPlayer1 = 0; iPlayer1 < MAX_CIV_PLAYERS; ++iPlayer1)
						{
							CvPlayer& kPlayer1 = GET_PLAYER((PlayerTypes)iPlayer1);
							FOR_EACH_CITY(pLoopCity, kPlayer1)
							{
								PlayerTypes eNewOwner = pLoopCity->plot()->findHighestCulturePlayer();
								if (eNewOwner != NO_PLAYER
								/*  advc.099: No longer implied by findHighestCulturePlayer;
									mustn't return cities to dead civs. */
										&& GET_PLAYER(eNewOwner).isAlive())
								{
									kData.ePlayer = (PlayerTypes)iPlayer1;
									kData.iCityId =	pLoopCity->getID();
									kData.eOtherPlayer = eNewOwner;

									if (isValidVoteSelection(eVoteSource, kData))
									{
										kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_ASSIGN_CITY", kPlayer1.getCivilizationAdjectiveKey(), pLoopCity->getNameKey(), GET_PLAYER(eNewOwner).getNameKey(), getVoteRequired(kData.eVote, eVoteSource), countPossibleVote(kData.eVote, eVoteSource));
										pData->aVoteOptions.push_back(kData);
									}
								}
							}
						}
					}
					else
					{
						kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_OPTION", GC.getVoteInfo(kData.eVote).getTextKeyWide(), getVoteRequired(kData.eVote, eVoteSource), countPossibleVote(kData.eVote, eVoteSource));
						if (isVotePassed(kData.eVote))
						{
							kData.szText += gDLL->getText("TXT_KEY_POPUP_PASSED");
						}

						//if (canDoResolution(eVoteSource, kData))
						if (isValidVoteSelection(eVoteSource, kData)) // K-Mod (zomg!)
						{
							pData->aVoteOptions.push_back(kData);
						}
					}
				}
			}
		}

		if (pData->aVoteOptions.size() == 0)
		{
			deleteVoteSelection(pData->getID());
			pData = NULL;
		}
	}

	return pData;
}

void CvGame::deleteVoteSelection(int iID)
{
	m_voteSelections.removeAt(iID);
}

VoteTriggeredData* CvGame::getVoteTriggered(int iID) const
{
	return ((VoteTriggeredData*)(m_votesTriggered.getAt(iID)));
}

VoteTriggeredData* CvGame::addVoteTriggered(const VoteSelectionData& kData, int iChoice)
{
	if (-1 == iChoice || iChoice >= (int)kData.aVoteOptions.size())
	{
		return NULL;
	}

	return addVoteTriggered(kData.eVoteSource, kData.aVoteOptions[iChoice]);
}

VoteTriggeredData* CvGame::addVoteTriggered(VoteSourceTypes eVoteSource, const VoteSelectionSubData& kOptionData)
{
	VoteTriggeredData* pData = ((VoteTriggeredData*)(m_votesTriggered.add()));

	if (NULL != pData)
	{
		pData->eVoteSource = eVoteSource;
		pData->kVoteOption = kOptionData;

		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iI);
			if (kPlayer.isVotingMember(eVoteSource))
			{
				if (kPlayer.isHuman())
				{	// <dlph.25>
					bool bForced = false;
					if (isTeamVote(kOptionData.eVote))
					{
						for (int iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
						{
							if(!GET_TEAM((TeamTypes)iJ).isAlive())
								continue;
							if (isTeamVoteEligible((TeamTypes)iJ, eVoteSource))
							{
								if (GET_TEAM(kPlayer.getTeam()).isVassal((TeamTypes)iJ)
										// advc:
										&& GET_TEAM(kPlayer.getTeam()).isCapitulated())
								{
									if (!isTeamVoteEligible(kPlayer.getTeam(), eVoteSource))
									{
										castVote((PlayerTypes)iI, pData->getID(),
												GET_PLAYER((PlayerTypes)iI).
												AI_diploVote(kOptionData, eVoteSource, false));
										bForced = true;
										break;
									}
								}
							}
						}
					}
					if (!bForced) // </dlph.25>
 					{
						CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_DIPLOVOTE);
						if (NULL != pInfo)
						{
							pInfo->setData1(pData->getID());
							gDLL->getInterfaceIFace()->addPopup(pInfo, (PlayerTypes)iI);
						}
 					}
 				}
				else
				{
					castVote(((PlayerTypes)iI), pData->getID(), GET_PLAYER((PlayerTypes)iI).AI_diploVote(kOptionData, eVoteSource, false));
				}
			}
		}
	}

	return pData;
}

void CvGame::deleteVoteTriggered(int iID)
{
	m_votesTriggered.removeAt(iID);
}

void CvGame::doVoteResults()
{
	// advc.150b: To make sure it doesn't go out of scope
	static CvWString szTargetCityName;
	int iLoop=-1;
	for (VoteTriggeredData* pVoteTriggered = m_votesTriggered.beginIter(&iLoop); NULL != pVoteTriggered; pVoteTriggered = m_votesTriggered.nextIter(&iLoop))
	{
		CvWString szBuffer;
		CvWString szMessage;
		VoteSelectionSubData subdata = pVoteTriggered->kVoteOption; // advc.003
		VoteTypes eVote = subdata.eVote;
		VoteSourceTypes eVoteSource = pVoteTriggered->eVoteSource;
		bool bPassed = false;

		if (!canDoResolution(eVoteSource, subdata))
		{
			for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
			{
				CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
				if (kPlayer.isVotingMember(eVoteSource))
				{
					szMessage.clear();
					szMessage.Format(L"%s: %s", gDLL->getText("TXT_KEY_ELECTION_CANCELLED").GetCString(), GC.getVoteInfo(eVote).getDescription());
					// advc.127b:
					std::pair<int,int> xy = getVoteSourceXY(eVoteSource, kPlayer.getTeam());
					gDLL->getInterfaceIFace()->addHumanMessage(kPlayer.getID(),
							false, GC.getEVENT_MESSAGE_TIME(), szMessage,
							"AS2D_NEW_ERA", MESSAGE_TYPE_INFO, NULL, (ColorTypes)
							GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
							xy.first, xy.second); // advc.127b
				}
			}
		}
		else
		{
			bool bAllVoted = true;
			for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
			{
				PlayerTypes ePlayer = (PlayerTypes) iJ;
				if (GET_PLAYER(ePlayer).isVotingMember(eVoteSource))
				{
					if (getPlayerVote(ePlayer, pVoteTriggered->getID()) == NO_PLAYER_VOTE)
					{
						//give player one more turn to submit vote
						setPlayerVote(ePlayer, pVoteTriggered->getID(), NO_PLAYER_VOTE_CHECKED);
						bAllVoted = false;
						break;
					}
					else if (getPlayerVote(ePlayer, pVoteTriggered->getID()) == NO_PLAYER_VOTE_CHECKED)
					{
						//default player vote to abstain
						setPlayerVote(ePlayer, pVoteTriggered->getID(), PLAYER_VOTE_ABSTAIN);
					}
				}
			}

			if (!bAllVoted)
				continue;
			// <advc.150b>
			szTargetCityName = "";
			int iVotes = -1; // </advc.150b>
			if (isTeamVote(eVote))
			{
				TeamTypes eTeam = findHighestVoteTeam(*pVoteTriggered);

				if (NO_TEAM != eTeam)
				{	// <advc.150b> Store vote count for later
					iVotes = countVote(*pVoteTriggered, (PlayerVoteTypes)eTeam);
					bPassed = (iVotes >= getVoteRequired(eVote, eVoteSource));
				}	// </advc.150b>

				szBuffer = GC.getVoteInfo(eVote).getDescription();

				if (eTeam != NO_TEAM)
				{
					szBuffer += NEWLINE + gDLL->getText("TXT_KEY_POPUP_DIPLOMATIC_VOTING_VICTORY",
							GET_TEAM(eTeam).getName().GetCString(),
							countVote(*pVoteTriggered, (PlayerVoteTypes)eTeam),
							getVoteRequired(eVote, eVoteSource),
							countPossibleVote(eVote, eVoteSource));
				}

				for (int iI = MAX_CIV_TEAMS; iI >= 0; --iI)
				{
					for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
					{
						if (GET_PLAYER((PlayerTypes)iJ).isVotingMember(eVoteSource))
						{
							if (getPlayerVote(((PlayerTypes)iJ), pVoteTriggered->getID()) == (PlayerVoteTypes)iI)
							{
								szBuffer += NEWLINE + gDLL->getText("TXT_KEY_POPUP_VOTES_FOR",
										GET_PLAYER((PlayerTypes)iJ).getNameKey(),
										GET_TEAM((TeamTypes)iI).getName().GetCString(),
										GET_PLAYER((PlayerTypes)iJ).getVotes(eVote, eVoteSource));
							}
						}
					}
				}

				for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isVotingMember(eVoteSource))
					{
						if (getPlayerVote(((PlayerTypes)iJ), pVoteTriggered->getID()) == PLAYER_VOTE_ABSTAIN)
						{
							szBuffer += NEWLINE + gDLL->getText("TXT_KEY_POPUP_ABSTAINS",
									GET_PLAYER((PlayerTypes)iJ).getNameKey(),
									GET_PLAYER((PlayerTypes)iJ).getVotes(eVote, eVoteSource));
						}
					}
				}

				if (NO_TEAM != eTeam && bPassed)
				{
					setVoteOutcome(*pVoteTriggered, (PlayerVoteTypes)eTeam);
				}
				else
				{
					setVoteOutcome(*pVoteTriggered, PLAYER_VOTE_ABSTAIN);
				}
			}
			else
			{	// <advc.150b>
				if(subdata.ePlayer != NO_PLAYER && subdata.iCityId >= 0) {
					CvCity* pTargetCity = GET_PLAYER(subdata.ePlayer).getCity(subdata.iCityId);
					if(pTargetCity != NULL)
						szTargetCityName = pTargetCity->getNameKey();
				}
				iVotes = countVote(*pVoteTriggered, PLAYER_VOTE_YES);
				bPassed = (iVotes >= getVoteRequired(eVote, eVoteSource));
				// </advc.150b>
				// Defying resolution
				if (bPassed)
				{
					for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
					{
						if (getPlayerVote((PlayerTypes)iJ, pVoteTriggered->getID()) == PLAYER_VOTE_NEVER)
						{
							bPassed = false;

							GET_PLAYER((PlayerTypes)iJ).setDefiedResolution(eVoteSource, subdata);
						}
					}
				}

				if (bPassed)
				{
					for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
					{
						if (GET_PLAYER((PlayerTypes)iJ).isVotingMember(eVoteSource))
						{
							if (getPlayerVote(((PlayerTypes)iJ), pVoteTriggered->getID()) == PLAYER_VOTE_YES)
							{
								GET_PLAYER((PlayerTypes)iJ).setEndorsedResolution(eVoteSource, subdata);
							}
						}
					}
				}

				szBuffer += NEWLINE + gDLL->getText((bPassed ? "TXT_KEY_POPUP_DIPLOMATIC_VOTING_SUCCEEDS" : "TXT_KEY_POPUP_DIPLOMATIC_VOTING_FAILURE"), GC.getVoteInfo(eVote).getTextKeyWide(), countVote(*pVoteTriggered, PLAYER_VOTE_YES), getVoteRequired(eVote, eVoteSource), countPossibleVote(eVote, eVoteSource));

				for (int iI = PLAYER_VOTE_NEVER; iI <= PLAYER_VOTE_YES; ++iI)
				{
					for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
					{
						if (GET_PLAYER((PlayerTypes)iJ).isVotingMember(eVoteSource))
						{
							if (getPlayerVote(((PlayerTypes)iJ), pVoteTriggered->getID()) == (PlayerVoteTypes)iI)
							{
								switch ((PlayerVoteTypes)iI)
								{
								case PLAYER_VOTE_ABSTAIN:
									szBuffer += NEWLINE + gDLL->getText("TXT_KEY_POPUP_ABSTAINS", GET_PLAYER((PlayerTypes)iJ).getNameKey(), GET_PLAYER((PlayerTypes)iJ).getVotes(eVote, eVoteSource));
									break;
								case PLAYER_VOTE_NEVER:
									szBuffer += NEWLINE + gDLL->getText("TXT_KEY_POPUP_VOTES_YES_NO", GET_PLAYER((PlayerTypes)iJ).getNameKey(), L"TXT_KEY_POPUP_VOTE_NEVER", GET_PLAYER((PlayerTypes)iJ).getVotes(eVote, eVoteSource));
									break;
								case PLAYER_VOTE_NO:
									szBuffer += NEWLINE + gDLL->getText("TXT_KEY_POPUP_VOTES_YES_NO", GET_PLAYER((PlayerTypes)iJ).getNameKey(), L"TXT_KEY_POPUP_NO", GET_PLAYER((PlayerTypes)iJ).getVotes(eVote, eVoteSource));
									break;
								case PLAYER_VOTE_YES:
									szBuffer += NEWLINE + gDLL->getText("TXT_KEY_POPUP_VOTES_YES_NO", GET_PLAYER((PlayerTypes)iJ).getNameKey(), L"TXT_KEY_POPUP_YES", GET_PLAYER((PlayerTypes)iJ).getVotes(eVote, eVoteSource));
									break;
								default:
									FAssert(false);
									break;
								}
							}
						}
					}
				}

				setVoteOutcome(*pVoteTriggered, bPassed ? PLAYER_VOTE_YES : PLAYER_VOTE_NO);
			}
			// <advc.150b>
			CvVoteInfo& kVote = GC.getVoteInfo(eVote);
			if(bPassed && !kVote.isSecretaryGeneral()) {
				CvWString szResolution;
				// Special treatment for resolutions with targets
				if(subdata.ePlayer != NO_PLAYER) {
					CvWString szKey;
					if(kVote.isForcePeace())
						szKey = L"TXT_KEY_POPUP_ELECTION_FORCE_PEACE";
					else if(kVote.isForceNoTrade())
						szKey = L"TXT_KEY_POPUP_ELECTION_FORCE_NO_TRADE";
					else if(kVote.isForceWar())
						szKey = L"TXT_KEY_POPUP_ELECTION_FORCE_WAR";
					if(!szKey.empty()) {
						szResolution = gDLL->getText(szKey, GET_PLAYER(subdata.ePlayer).
								getReplayName(), 0, 0);
					}
					else if(kVote.isAssignCity() && !szTargetCityName.empty() &&
							subdata.eOtherPlayer != NO_PLAYER) {
						szResolution = gDLL->getText("TXT_KEY_POPUP_ELECTION_ASSIGN_CITY",
								GET_PLAYER(subdata.ePlayer).getCivilizationAdjectiveKey(),
								szTargetCityName.GetCString(),
								GET_PLAYER(subdata.eOtherPlayer).getReplayName(), 0, 0);
					}
				}
				if(szResolution.empty()) {
					szResolution = kVote.getDescription();
					/*  This is e.g.
						"U.N. Resolution #1284 (Nuclear Non-Proliferation Treaty - Cannot Build Nuclear Weapons)
						Only want "Nuclear Non-Proliferation Treaty". */
					size_t pos1 = szResolution.find(L"(");
					if(pos1 != CvWString::npos && pos1 + 1 < szResolution.length()) {
						bool bForceCivic = false;
						// Mustn't remove the stuff after the dash if bForceCivic
						for(int i = 0; i < GC.getNumCivicInfos(); i++) {
							if(kVote.isForceCivic(i)) {
								bForceCivic = true;
								break;
							}
						}
						size_t pos2 = std::min((bForceCivic ? CvWString::npos :
								szResolution.find(L" -")),
								szResolution.find(L")"));
						if(pos2 > pos1)
							szResolution = szResolution.substr(pos1 + 1, pos2 - pos1 - 1);
					}
				}
				else {
					/*  Throw out stuff in parentheses, e.g.
						"Stop the war against Napoleon (Requires 0 of 0 Total Votes)" */
					szResolution = szResolution.substr(0, szResolution.find(L"(") - 1);
				}
				TeamTypes eSecrGen = getSecretaryGeneral(eVoteSource);
				szMessage = gDLL->getText("TXT_KEY_REPLAY_RESOLUTION_PASSED",
						GC.getVoteSourceInfo(eVoteSource).getTextKeyWide(),
						(eSecrGen == NO_TEAM ?
						gDLL->getText("TXT_KEY_TOPCIVS_UNKNOWN").GetCString() :
						GET_TEAM(eSecrGen).getReplayName().GetCString()),
						iVotes, // Don't show the required votes after all
						//getVoteRequired(eVote, eVoteSource),
						countPossibleVote(eVote, eVoteSource),
						szResolution.GetCString());
				addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, NO_PLAYER, szMessage,
						-1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
			} // </advc.150b>
			for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
			{
				CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iI);
				bool bShow = kPlayer.isVotingMember(pVoteTriggered->eVoteSource);
				if (bShow /* advc.127: */ && kPlayer.isHuman())
				{
					CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_TEXT);
					if (NULL != pInfo)
					{
						pInfo->setText(szBuffer);
						gDLL->getInterfaceIFace()->addPopup(pInfo, (PlayerTypes)iI);
					}
				} // <advc.003>
				if(!bShow && iI == subdata.ePlayer && GET_PLAYER(subdata.ePlayer).
						isVotingMember(pVoteTriggered->eVoteSource))
					bShow = true;
				if(!bShow && iI == subdata.eOtherPlayer && GET_PLAYER(subdata.eOtherPlayer).
						isVotingMember(pVoteTriggered->eVoteSource))
					bShow = true; // </advc.003>
				if (bPassed && (bShow // <advc.127>
						|| kPlayer.isSpectator()))
				{
					if(bShow || szMessage.empty() || kVote.isSecretaryGeneral())
					{	// </advc.127>
						szMessage = gDLL->getText("TXT_KEY_VOTE_RESULTS",
								GC.getVoteSourceInfo(eVoteSource).getTextKeyWide(),
								subdata.szText.GetCString());
						// Else use the replay msg
					}
					// <advc.127b>
					BuildingTypes eVSBuilding = getVoteSourceBuilding(eVoteSource);
					std::pair<int,int> xy = getVoteSourceXY(eVoteSource,
							kPlayer.getTeam(), true);
					// </advc.127b>
					gDLL->getInterfaceIFace()->addHumanMessage(kPlayer.getID(),
							false, GC.getEVENT_MESSAGE_TIME(), szMessage, "AS2D_NEW_ERA",
							// <advc.127> was always MINOR
							kVote.isSecretaryGeneral() ? MESSAGE_TYPE_MINOR_EVENT :
							MESSAGE_TYPE_MAJOR_EVENT, // </advc.127>
							// <advc.127b>
							eVSBuilding == NO_BUILDING ? NULL :
							GC.getBuildingInfo(eVSBuilding).getButton(),
							// </advc.127b>
							(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
							xy.first, xy.second); // advc.127b
				}
			}
		}

		if (!bPassed && GC.getVoteInfo(eVote).isSecretaryGeneral())
			setSecretaryGeneralTimer(eVoteSource, 0);

		deleteVoteTriggered(pVoteTriggered->getID());
	}
}

void CvGame::doVoteSelection()
{
	for (int iI = 0; iI < GC.getNumVoteSourceInfos(); ++iI)
	{
		VoteSourceTypes eVoteSource = (VoteSourceTypes)iI;

		if (isDiploVote(eVoteSource))
		{
			if (getVoteTimer(eVoteSource) > 0)
			{
				changeVoteTimer(eVoteSource, -1);
			}
			else
			{
				setVoteTimer(eVoteSource, (GC.getVoteSourceInfo(eVoteSource).getVoteInterval() * GC.getGameSpeedInfo(getGameSpeedType()).getVictoryDelayPercent()) / 100);

				for (int iTeam1 = 0; iTeam1 < MAX_CIV_TEAMS; ++iTeam1)
				{
					CvTeam& kTeam1 = GET_TEAM((TeamTypes)iTeam1);

					if (kTeam1.isAlive() && kTeam1.isVotingMember(eVoteSource))
					{
						for (int iTeam2 = iTeam1 + 1; iTeam2 < MAX_CIV_TEAMS; ++iTeam2)
						{
							CvTeam& kTeam2 = GET_TEAM((TeamTypes)iTeam2);

							if (kTeam2.isAlive() && kTeam2.isVotingMember(eVoteSource))
							{	//kTeam1.meet((TeamTypes)iTeam2, true);
								// <advc.071> Check isHasMet b/c getVoteSourceCity is a bit slow
								if(!kTeam1.isHasMet(kTeam2.getID())) {
									CvCity* pSrcCity = getVoteSourceCity(eVoteSource, NO_TEAM);
									if(pSrcCity == NULL)
										kTeam1.meet((TeamTypes)iTeam2, true, NULL);
									else {
										FirstContactData fcData(pSrcCity->plot());
										kTeam1.meet((TeamTypes)iTeam2, true, &fcData);
									} // </advc.071>
								}
							}
						}
					}
				}

				TeamTypes eSecretaryGeneral = getSecretaryGeneral(eVoteSource);
				PlayerTypes eSecretaryPlayer;

				if (eSecretaryGeneral != NO_TEAM)
				{
					eSecretaryPlayer = GET_TEAM(eSecretaryGeneral).getSecretaryID();
				}
				else
				{
					eSecretaryPlayer = NO_PLAYER;
				}

				bool bSecretaryGeneralVote = false;
				if (canHaveSecretaryGeneral(eVoteSource))
				{
					if (getSecretaryGeneralTimer(eVoteSource) > 0)
					{
						changeSecretaryGeneralTimer(eVoteSource, -1);
					}
					else
					{
						setSecretaryGeneralTimer(eVoteSource, GC.getDefineINT("DIPLO_VOTE_SECRETARY_GENERAL_INTERVAL"));

						for (int iJ = 0; iJ < GC.getNumVoteInfos(); iJ++)
						{
							if (GC.getVoteInfo((VoteTypes)iJ).isSecretaryGeneral() && GC.getVoteInfo((VoteTypes)iJ).isVoteSourceType(iI))
							{
								VoteSelectionSubData kOptionData;
								kOptionData.iCityId = -1;
								kOptionData.ePlayer = NO_PLAYER;
								kOptionData.eOtherPlayer = NO_PLAYER; // kmodx: Missing initialization
								kOptionData.eVote = (VoteTypes)iJ;
								kOptionData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_OPTION", GC.getVoteInfo((VoteTypes)iJ).getTextKeyWide(), getVoteRequired((VoteTypes)iJ, eVoteSource), countPossibleVote((VoteTypes)iJ, eVoteSource));
								addVoteTriggered(eVoteSource, kOptionData);
								bSecretaryGeneralVote = true;
								break;
							}
						}
					}
				}

				if (!bSecretaryGeneralVote && eSecretaryGeneral != NO_TEAM && eSecretaryPlayer != NO_PLAYER)
				{
					VoteSelectionData* pData = addVoteSelection(eVoteSource);
					if (NULL != pData)
					{
						if (GET_PLAYER(eSecretaryPlayer).isHuman())
						{
							CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_CHOOSEELECTION);
							if (NULL != pInfo)
							{
								pInfo->setData1(pData->getID());
								gDLL->getInterfaceIFace()->addPopup(pInfo, eSecretaryPlayer);
							}
						}
						else
						{
							setVoteChosen(GET_TEAM(eSecretaryGeneral).AI_chooseElection(*pData), pData->getID());
						}
					}
					else
					{
						setVoteTimer(eVoteSource, 0);
					}
				}
			}
		}
	}
}

bool CvGame::isEventActive(EventTriggerTypes eTrigger) const
{
	for (std::vector<EventTriggerTypes>::const_iterator it = m_aeInactiveTriggers.begin(); it != m_aeInactiveTriggers.end(); ++it)
	{
		if (*it == eTrigger)
		{
			return false;
		}
	}

	return true;
}

void CvGame::initEvents()
{
	for (int iTrigger = 0; iTrigger < GC.getNumEventTriggerInfos(); ++iTrigger)
	{
		if (isOption(GAMEOPTION_NO_EVENTS) || getSorenRandNum(100, "Event Active?") >= GC.getEventTriggerInfo((EventTriggerTypes)iTrigger).getPercentGamesActive())
		{
			m_aeInactiveTriggers.push_back((EventTriggerTypes)iTrigger);
		}
	}
}

bool CvGame::isCivEverActive(CivilizationTypes eCivilization) const
{
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kLoopPlayer.isEverAlive())
		{
			if (kLoopPlayer.getCivilizationType() == eCivilization)
			{
				return true;
			}
		}
	}

	return false;
}

bool CvGame::isLeaderEverActive(LeaderHeadTypes eLeader) const
{
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kLoopPlayer.isEverAlive())
		{
			if (kLoopPlayer.getLeaderType() == eLeader)
			{
				return true;
			}
		}
	}

	return false;
}

bool CvGame::isUnitEverActive(UnitTypes eUnit) const
{
	for (int iCiv = 0; iCiv < GC.getNumCivilizationInfos(); ++iCiv)
	{
		if (isCivEverActive((CivilizationTypes)iCiv))
		{
			if (eUnit == GC.getCivilizationInfo((CivilizationTypes)iCiv).getCivilizationUnits(GC.getUnitInfo(eUnit).getUnitClassType()))
			{
				return true;
			}
		}
	}

	return false;
}

bool CvGame::isBuildingEverActive(BuildingTypes eBuilding) const
{
	for (int iCiv = 0; iCiv < GC.getNumCivilizationInfos(); ++iCiv)
	{
		if (isCivEverActive((CivilizationTypes)iCiv))
		{
			if (eBuilding == GC.getCivilizationInfo((CivilizationTypes)iCiv).getCivilizationBuildings(GC.getBuildingInfo(eBuilding).getBuildingClassType()))
			{
				return true;
			}
		}
	}

	return false;
}

void CvGame::processBuilding(BuildingTypes eBuilding, int iChange)
{
	for (int iI = 0; iI < GC.getNumVoteSourceInfos(); ++iI)
	{
		if (GC.getBuildingInfo(eBuilding).getVoteSourceType() == (VoteSourceTypes)iI)
		{
			changeDiploVote((VoteSourceTypes)iI, iChange);
		}
	}
}

bool CvGame::pythonIsBonusIgnoreLatitudes() const
{
	long lResult = -1;
	if (gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "isBonusIgnoreLatitude", NULL, &lResult))
	{
		if (!gDLL->getPythonIFace()->pythonUsingDefaultImpl() && lResult != -1)
		{
			return (lResult != 0);
		}
	}

	return false;
}

// <advc.314> Between 0 and GOODY_BUFF_PEAK_MULTIPLIER, depending on game turn.
double CvGame::goodyHutEffectFactor(
		/*  Use true when a goody hut effect is supposed to increase with
			the game speed. When set to false, the turn numbers in this
			function are still game-speed adjusted. */
		bool bSpeedAdjust) const {

	CvGameSpeedInfo& kSpeed = GC.getGameSpeedInfo(getGameSpeedType());
	double speedMultTurns = kSpeed.getGrowthPercent() / 100.0;
	int const iWorldSzPercent = 100;
		// Not sure if map-size adjustment is a good idea
		//=GC.getWorldInfo(GC.getMap().getWorldSize()).getResearchPercent();
	double speedMultFinal = (bSpeedAdjust ?
			kSpeed.getTrainPercent() * iWorldSzPercent / 10000.0 : 1);
	double startTurn = std::max(0.0,
			GC.getDefineINT("GOODY_BUFF_START_TURN") * speedMultTurns);
	double peakTurn = std::max(startTurn,
			GC.getDefineINT("GOODY_BUFF_PEAK_TURN") * speedMultTurns);
	double peakMult = std::max(1, GC.getDefineINT("GOODY_BUFF_PEAK_MULTIPLIER"));
	/*  Exponent for power-law function; aiming for a function shape that
		resembles the graphs on the Info tab. */
	double exponent = 1.25;
	// (or rather: the inverse of the gradient)
	double gradient = std::pow(peakTurn - startTurn, exponent) / (peakMult - 1);
	gradient = ::dRange(gradient, 1.0, 500.0);
	double t = getGameTurn();
	/*  Function through (startTurn, 1) and (peakTurn, peakMult)
		[^that's assuming speedAdjust=false] */
	double r = speedMultFinal * std::min(peakMult,
			(gradient + std::pow(std::max(0.0, t - startTurn), exponent)) / gradient);
	return r;
} // </advc.314>

// <advc.004m>
GlobeLayerTypes CvGame::getCurrentLayer() const {

	return m_eCurrentLayer;
}

// Used by CvMainInterface.py to tell the DLL which layer is active
void CvGame::reportCurrentLayer(GlobeLayerTypes eLayer) {

	if(m_bLayerFromSavegame && eLayer != m_eCurrentLayer) {
		m_bLayerFromSavegame = false;
		/*  Can only enable the Resource and Unit layer from the DLL. That should
			suffice as the others are only available in Globe view and we're not
			in Globe view right after loading.
			(Could call CyGlobeLayerManager.setCurrentLayer via Python to set a
			Globe-only layer. Would have to write a Python function that first uses
			CyGlobeLayerManager to figure out the proper layer id based on m_eCurrentLayer.) */
		gDLL->getEngineIFace()->setResourceLayer(m_eCurrentLayer == GLOBE_LAYER_RESOURCE);
		if(m_eCurrentLayer == GLOBE_LAYER_UNIT || eLayer == GLOBE_LAYER_UNIT)
			gDLL->getEngineIFace()->toggleUnitLayer(); // No setter available
	}
	else m_eCurrentLayer = eLayer;
} // </advc.004m>

// <advc.127b>
std::pair<int,int> CvGame::getVoteSourceXY(VoteSourceTypes eVS, TeamTypes eObserver,
		bool bDebug) const {

	CvCity* pVSCity = getVoteSourceCity(eVS, eObserver, bDebug);
	std::pair<int,int> r = std::make_pair(-1,-1);
	if(pVSCity == NULL)
		return r;
	r.first = pVSCity->getX();
	r.second = pVSCity->getY();
	return r;
}

CvCity* CvGame::getVoteSourceCity(VoteSourceTypes eVS, TeamTypes eObserver, bool bDebug) const {

	BuildingTypes eVSBuilding = getVoteSourceBuilding(eVS);
	if(eVSBuilding == NO_BUILDING)
		return NULL;
	for(int i = 0; i < MAX_PLAYERS; i++) {
		CvPlayer const& kOwner = GET_PLAYER((PlayerTypes)i);
		if(!kOwner.isAlive())
			continue;
		FOR_EACH_CITY(c, kOwner) {
			if(eObserver != NO_TEAM && !c->isRevealed(eObserver, bDebug))
				continue;
			if(c->getNumBuilding(eVSBuilding) > 0)
				return c;
		}
	}
	return NULL;
}

// <advc.003> Used in several places and I want to make a small change
bool CvGame::isFreeStartEraBuilding(BuildingTypes eBuilding) const {

	CvBuildingInfo const& kBuilding = GC.getBuildingInfo(eBuilding);
	return (kBuilding.getFreeStartEra() != NO_ERA &&
			getStartEra() >= kBuilding.getFreeStartEra() &&
			// <advc.126>
			(kBuilding.getMaxStartEra() == NO_ERA ||
			kBuilding.getMaxStartEra() >= getStartEra())); // </advc.126>
} // </advc.003>


BuildingTypes CvGame::getVoteSourceBuilding(VoteSourceTypes eVS) const {

	for(int i = 0; i < GC.getNumBuildingInfos(); i++) {
		BuildingTypes eBuilding = (BuildingTypes)i;
		if(GC.getBuildingInfo(eBuilding).getVoteSourceType() == eVS)
			return eBuilding;
	}
	return NO_BUILDING;
} // </advc.127b>

// <advc.052>
bool CvGame::isScenario() const {

	return m_bScenario;
}

void CvGame::setScenario(bool b) {

	m_bScenario = b;
} // </advc.052>

// advc.250b:
StartPointsAsHandicap& CvGame::startPointsAsHandicap() {

	return *m_pSpah;
}

// <advc.703>
RiseFall const& CvGame::getRiseFall() const {

	return *m_pRiseFall;
}
RiseFall& CvGame::getRiseFall() {

	return *m_pRiseFall;
} // </advc.703>

// <advc.106i>
void CvGame::setHallOfFame(CvHallOfFameInfo* pHallOfFame) {

	m_pHallOfFame = pHallOfFame;
} // </advc.106i>

// <advc.003>
std::set<int>& CvGame::getActivePlayerCycledGroups() {

	return m_ActivePlayerCycledGroups; // Was public; now protected.
} // </advc.003>
