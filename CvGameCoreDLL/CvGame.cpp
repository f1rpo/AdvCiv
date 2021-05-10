#include "CvGameCoreDLL.h"
#include "CvGame.h"
#include "CvDeal.h"
#include "CvAgents.h" // advc.agent
#include "CoreAI.h"
#include "CvCityAI.h"
#include "CvUnit.h"
#include "CvSelectionGroupAI.h"
#include "CitySiteEvaluator.h"
#include "PlotRange.h"
#include "CvArea.h"
#include "CvMapGenerator.h"
#include "CvDiploParameters.h"
#include "CvReplayMessage.h"
#include "CvInfo_City.h"
#include "CvInfo_Terrain.h"
#include "CvInfo_GameOption.h"
#include "CvInfo_Civics.h"
#include "CvPopupInfo.h"
#include "CvReplayInfo.h"
#include "CvGameTextMgr.h"
#include "CvMessageControl.h"
#include "StartingPositionIteration.h" // advc.027
#include "StartPointsAsHandicap.h" // advc.250b
#include "RiseFall.h" // advc.700
#include "CvHallOfFameInfo.h" // advc.106i
#include "BBAILog.h" // BBAI
#include "CvBugOptions.h" // K-Mod

/*	<advc.007b> Use this CvGame instance instead of GC.getGame() for RNG calls.
	(Won't matter so long as CvGame is a singleton class.) */
#undef CVGAME_INSTANCE_FOR_RNG
#define CVGAME_INSTANCE_FOR_RNG (*this) // </advc.007b>


CvGame::CvGame() :
	m_pRiseFall(new RiseFall()), // advc.700
	m_pSpah(new StartPointsAsHandicap()) // advc.250b
{
	m_aiRankPlayer = new int[MAX_PLAYERS];        // Ordered by rank...
	m_aiPlayerRank = new int[MAX_PLAYERS];        // Ordered by player ID...
	m_aiPlayerScore = new int[MAX_PLAYERS];       // Ordered by player ID...
	m_aiRankTeam = new int[MAX_TEAMS];						// Ordered by rank...
	m_aiTeamRank = new int[MAX_TEAMS];						// Ordered by team ID...
	m_aiTeamScore = new int[MAX_TEAMS];						// Ordered by team ID...

	m_paiUnitCreatedCount = NULL;
	m_paiUnitClassCreatedCount = NULL;
	m_paiBuildingClassCreatedCount = NULL;
	m_paiProjectCreatedCount = NULL;
	m_paiForceCivicCount = NULL;
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
	CvInitCore& ic = GC.getInitCore();

	reset(eHandicap); // Reset serialized data

	// Init containers ...

	m_voteSelections.init();
	m_votesTriggered.init();

	/*m_mapRand.init(ic.getMapRandSeed() % 73637381);
	m_sorenRand.init(ic.getSyncRandSeed() % 52319761);*/
	/*	advc.027b: The modulo reductions get in the way of reproducing
		regenerated maps. It seems that, unless positive seeds are set
		in CivilizationIV.ini, CvInitCore receives the same seed for
		both RNGs. The modulo doesn't reliably prevent that. Shouldn't
		really matter, and actually makes it easier to reproduce maps
		as only one seed has to be read off the screen. */
	getMapRand().init(ic.getMapRandSeed());
	getSRand().init(ic.getSyncRandSeed());
	m_initialRandSeed.uiMap = getMapRand().getSeed();
	m_initialRandSeed.uiSync = getSRand().getSeed(); // <advc.027b>

	// Init non-serialized data ...

	m_bAllGameDataRead = true; // advc: Not loading from savegame

	// Turn off all MP options if it's a single player game
	if (ic.getType() == GAME_SP_NEW || ic.getType() == GAME_SP_SCENARIO)
	{
		FOR_EACH_ENUM(MPOption)
			setMPOption(eLoopMPOption, false);
	}

	// If this is a hot seat game, simultaneous turns is always off.
	if (isHotSeat() || isPbem())
		setMPOption(MPOPTION_SIMULTANEOUS_TURNS, false);
	// If we didn't set a time in the Pitboss, turn timer off.
	if (isPitboss() && getPitbossTurnTime() == 0)
		setMPOption(MPOPTION_TURN_TIMER, false);

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
			setOption(GAMEOPTION_LOCK_MODS, false);
		else
		{
			static const int iPasswordSize = 8;
			char szRandomPassword[iPasswordSize];
			for (int i = 0; i < iPasswordSize-1; i++)
			{
				szRandomPassword[i] = /* advc: */ toChar(
						getSorenRandNum(CHAR_MAX + 1, NULL));
			}
			szRandomPassword[iPasswordSize-1] = 0;
			ic.setAdminPassword(szRandomPassword);
		}
	}

	/*  advc.250c: So far, no points from Advanced Start have been assigned.
		I want the start turn to be a function of the start points.
		I'm assigning the start turn preliminarily here to avoid problems with
		start turn being undefined (not sure if this would be an issue),
		and overwrite the value later. To this end, I've moved the
		start turn and start year computation into a new function: */
	setStartTurnYear();

	FOR_EACH_ENUM(SpecialUnit)
	{
		if (GC.getInfo(eLoopSpecialUnit).isValid())
			makeSpecialUnitValid(eLoopSpecialUnit);
	}
	FOR_EACH_ENUM(SpecialBuilding)
	{
		if (GC.getInfo(eLoopSpecialBuilding).isValid())
			makeSpecialBuildingValid(eLoopSpecialBuilding);
	}

	AI().AI_init();

	doUpdateCacheOnTurn();
}

// Set initial items (units, techs, etc...)
void CvGame::setInitialItems()
{
	PROFILE_FUNC();

	initFreeState();
	// <advc.027> Keep data from starting plot assignment for normalization
	NormalizationTarget* pNormalizationTarget = assignStartingPlots();
	normalizeStartingPlots(pNormalizationTarget);
	SAFE_DELETE(pNormalizationTarget); // </advc.027>
	CvMap& kMap = GC.getMap();
	// <advc> River ids shouldn't be used after map generation
	for (int i = 0; i < kMap.numPlots(); i++)
		kMap.plotByIndex(i)->setRiverID(-1); // </advc>
	// <advc.030> Now that ice has been placed and normalization is through
	if(GC.getDefineBOOL("PASSABLE_AREAS"))
		kMap.recalculateAreas(false);
	// </advc.030>
	initFreeUnits();
	// <advc.250c>
	if (GC.getDefineBOOL("INCREASE_START_TURN") && getStartEra() == 0)
	{
		int iStartTurn = getStartTurn();
		bool bAllHuman = (PlayerIter<HUMAN>::count() >= PlayerIter<CIV_ALIVE>::count());
		CvHandicapInfo const& kGameHandicap = GC.getInfo(getHandicapType());
		if (isOption(GAMEOPTION_ADVANCED_START))
		{
			std::vector<scaled> distr;
			for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
			{
				if (!it->isHuman() || !isOption(GAMEOPTION_SPAH) || bAllHuman)
					distr.push_back(it->getAdvancedStartPoints());
			}
			/*	Start turn 0 is calibrated to the AI freebies from difficulty, which
				increase with every level (well, every level after Prince). */
			scaled rDifficultyVal = kGameHandicap.getDifficulty() * fixp(6.25);
			rDifficultyVal.increaseTo(175);
			scaled rMaxMean = (stats::max(distr) + stats::mean(distr)) / 2;
			if (rMaxMean > rDifficultyVal)
				iStartTurn += (rMaxMean - rDifficultyVal).pow(fixp(0.58)).roundToMultiple(5);
		} // </advc.250c>
		// <advc.251> Also set a later start turn if handicap grants lots of AI freebies
		else if (!bAllHuman)
		{
			iStartTurn += ((kGameHandicap.getAIStartingUnitMultiplier() * 10 +
					kGameHandicap.getAIStartingWorkerUnits() * 10) *
					GC.getInfo(getGameSpeedType()).getGrowthPercent()) / 100;
		} // <advc.250c>
		if (getStartTurn() != iStartTurn)
		{
			setStartTurnYear(iStartTurn);
			/*  initDiplomacy is called from outside the DLL between the first
				setStartTurnYear call and setInitialItems. The second setStartTurnYear
				causes any initial "universal" peace treaties to end after 1 turn.
				Need to inform all CvDeal objects about the changed start turn: */
			FOR_EACH_DEAL_VAR(d)
				d->setInitialGameTurn(getGameTurn());
		} // </advc.251>
	} // </advc.250c>
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
		it->AI_updateFoundValues();
}


void CvGame::regenerateMap()
{
	if (GC.getInitCore().getWBMapScript())
		return;
	CvMap& kMap = GC.getMap();
	/*  <advc.004j> Not sure if the unmodded game or any mod included with AdvCiv
		uses script data, but can't hurt to reset it. CvDLLButtonPopup::
		launchMainMenuPopup wants to disallow map regeneration once script data
		has been set. */
	setScriptData("");
	for(int i = 0; i < kMap.numPlots(); ++i)
	{
		CvPlot* p = kMap.plotByIndex(i);
		if(p != NULL)
		{
			p->setScriptData("");
			/*  advc.021b: Otherwise, assignStartingPlots runs into trouble upon
				map regeneration when a script calls allowDefaultImpl after
				assigning starting plots. */
			p->setStartingPlot(false);
		}
	} // </advc.004j>

	setFinalInitialized(false);
	setDawnOfManShown(false); // advc.004x

	for (PlayerIter<> it; it.hasNext(); ++it)
		it->killUnits();
	for (PlayerIter<> it; it.hasNext(); ++it)
		it->killCities();
	for (PlayerIter<> it; it.hasNext(); ++it)
		it->killAllDeals();
	for (PlayerIter<> it; it.hasNext(); ++it)
	{
		it->setFoundedFirstCity(false);
		it->setStartingPlot(NULL);
		// <advc.004x>
		if (it->isHuman())
			it->killAll(BUTTONPOPUP_CHOOSETECH); // </advc.004x>
	}
	for (TeamIter<> it; it.hasNext(); ++it)
		kMap.setRevealedPlots(it->getID(), false);

	gDLL->getEngineIFace()->clearSigns();
	m_aPlotExtraYields.clear(); // advc.004j
	kMap.erasePlots();

	/*	advc.027b: Mustn't overwrite m_initialRandSeed.uiSync if we want to
		preserve the civ selection when reproducing a map after a restart.
		normalizeStartingPlots now uses MapRand exclusively. */
	m_initialRandSeed.uiMap = getMapRand().getSeed();
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

	kMap.setupGraphical();
	gDLL->getEngineIFace()->SetDirty(GlobeTexture_DIRTY_BIT, true);
	gDLL->getEngineIFace()->SetDirty(MinimapTexture_DIRTY_BIT, true);
	gDLL->UI().setDirty(ColoredPlots_DIRTY_BIT, true);

	cycleSelectionGroups_delayed(1, false);
	// <advc.004j>
	bool bShowDawn = (GC.getDefineBOOL("SHOW_DAWN_AFTER_REGEN") &&
			// Somehow doesn't work with Adv. Start; Dawn screen doesn't appear.
			(!isOption(GAMEOPTION_ADVANCED_START) || isOption(GAMEOPTION_SPAH)));
	// </advc.004j>
	// <advc.700>
	if(isOption(GAMEOPTION_RISE_FALL))
	{
		m_pRiseFall->reset();
		m_pRiseFall->init();
		bShowDawn = false;
	}
	else // </advc.700>
		autoSave(true); // advc.106l
	// <advc.004j>
	if(bShowDawn)
		showDawnOfMan(); // </advc.004j>
	if (getActivePlayer()!= NO_PLAYER)
	{
		CvPlot* pPlot = GET_PLAYER(getActivePlayer()).getStartingPlot();
		if (pPlot != NULL)
		{
			//gDLL->UI().lookAt(pPlot->getPoint(), CAMERALOOKAT_NORMAL);
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

// advc.004j:
void CvGame::showDawnOfMan()
{
	if(getActivePlayer() == NO_PLAYER)
		return;
	// Based on CvAllErasDawnOfManScreenEventManager.py
	CvPopupInfo* pDummyPopup = new CvPopupInfo();
	pDummyPopup->setButtonPopupType(BUTTONPOPUP_PYTHON_SCREEN);
	pDummyPopup->setText(L"showDawnOfMan");
	GET_PLAYER(getActivePlayer()).addPopup(pDummyPopup);
	setDawnOfManShown(true); // advc.004x
}


void CvGame::uninit()
{
	SAFE_DELETE_ARRAY(m_aiShrineBuilding);
	SAFE_DELETE_ARRAY(m_aiShrineReligion);
	SAFE_DELETE_ARRAY(m_paiUnitCreatedCount);
	SAFE_DELETE_ARRAY(m_paiUnitClassCreatedCount);
	SAFE_DELETE_ARRAY(m_paiBuildingClassCreatedCount);
	SAFE_DELETE_ARRAY(m_paiProjectCreatedCount);
	SAFE_DELETE_ARRAY(m_paiForceCivicCount);
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
	// advc: Removed CvRandom::uninit; there was nothing to be done.
	/*m_mapRand.uninit();
	m_sorenRand.uninit();*/

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

// advc: Cut from CvGame::init
void CvGame::setStartTurnYear(int iTurn)
{
	CvGameSpeedInfo const& kSpeed = GC.getInfo(getGameSpeedType()); // advc
	// <advc.250c>
	if(iTurn > 0)
		setGameTurn(iTurn);
	else // </advc.250c>
		if (getGameTurn() == 0)
	{
		int iStartTurn = 0;
		for (int i = 0; i < kSpeed.getNumTurnIncrements(); i++)
			iStartTurn += kSpeed.getGameTurnInfo(i).iNumGameTurnsPerIncrement;
		iStartTurn *= GC.getInfo(getStartEra()).getStartPercent();
		iStartTurn /= 100;
		setGameTurn(iStartTurn);
	}

	setStartTurn(getGameTurn());

	if (getMaxTurns() == 0 /* advc.250c: */ || iTurn > 0)
	{
		int iEstimateEndTurn = 0;
		for (int i = 0; i < kSpeed.getNumTurnIncrements(); i++)
			iEstimateEndTurn += kSpeed.getGameTurnInfo(i).iNumGameTurnsPerIncrement;
		setEstimateEndTurn(iEstimateEndTurn);

		if (getEstimateEndTurn() > getGameTurn())
		{
			bool bValid = false;
			FOR_EACH_ENUM(Victory)
			{
				if (isVictoryValid(eLoopVictory) &&
					GC.getInfo(eLoopVictory).isEndScore())
				{
					bValid = true;
					break;
				}
			}
			if (bValid)
				setMaxTurns(getEstimateEndTurn() - getGameTurn());
		}
	}
	else setEstimateEndTurn(getGameTurn() + getMaxTurns());

	setStartYear(GC.getDefineINT("START_YEAR"));
}

/*  Initialize data members that are serialized. (advc: I'm also initializing some
	non-serialized variables here. Most of these don't _have to_ be reset when
	loading a savegame, but it seems safer to do so.) */
void CvGame::reset(HandicapTypes eHandicap, bool bConstructorCall)
{
	int iI;

	uninit();

	m_bAllGameDataRead = false; // advc;
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
	// <advc.opt>
	m_iStartingPlotRange = 0; // (not serialized)
	m_iCivPlayersEverAlive = 0;
	m_iCivTeamsEverAlive = 0;
	// </advc.opt>
	m_uiInitialTime = 0;
	m_uiSaveFlag = 0; // advc

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
	// <advc.127>
	m_eAIHandicap = (bConstructorCall ? NO_HANDICAP :
			// (XML not loaded when constructor called)
			(HandicapTypes)GC.getDefineINT("STANDARD_HANDICAP")); // </advc.127>
	m_ePausePlayer = NO_PLAYER;
	m_eBestLandUnit = NO_UNIT;
	m_eWinner = NO_TEAM;
	m_eVictory = NO_VICTORY;
	m_eGameState = GAMESTATE_ON;
	m_eInitialActivePlayer = NO_PLAYER; // advc.106h
	m_eNormalizationLevel = NORMALIZE_DEFAULT; // advc.108
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

		m_aiVoteOutcome.reset();

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
	m_initialRandSeed.uiMap = m_initialRandSeed.uiSync = 0; // advc.027b

	m_iNumSessions = 1;

	m_iShrineBuildingCount = 0;
	m_iNumCultureVictoryCities = 0;
	m_eCultureVictoryCultureLevel = NO_CULTURELEVEL;
	m_bScenario = false; // advc.052

	if (!bConstructorCall)
		AI().AI_reset();

	m_ActivePlayerCycledGroups.clear(); // K-Mod
	m_bInBetweenTurns = false; // advc.106b
	m_iUnitUpdateAttempts = 0; // advc.001y
	m_iTurnLoadedFromSave = -1; // advc.044
	// <advc.004m>
	m_eCurrentLayer = GLOBE_LAYER_UNKNOWN;
	m_bLayerFromSavegame = false; // </advc.004m>
	m_bFeignSP = false; // advc.135c
	m_bDoMShown = false; // advc.004x
	m_bFPTestDone = false; // advc.003g
	// <advc.003r>
	for(int i = 0; i < NUM_UPDATE_TIMER_TYPES; i++)
		m_aiUpdateTimers[i] = -1; // </advc.003r>
	/*  <advc.003v> No need to read data from a savegame first; CvInitCore
		is responsible for the game options and is loaded before CvGame. */
	if (!bConstructorCall)
		CvGlobals::getInstance().loadOptionalXMLInfo(); // </advc.003v>
}

/*	The EXE calls this after generating the map but before initFreeState
	(i.e. also before assigning starting plots). Seems like a good place
	for various initializations as it gets called for all game types
	(unlike setInitialItems, which isn't called for scenarios with fixed
	civs and leaders). */
void CvGame::initDiplomacy()
{
	PROFILE_FUNC();

	GC.getAgents().gameStart(false); // advc.agent
	m_bFPTestDone = !isNetworkMultiPlayer(); // advc.003g
	// <advc.108>
	// Don't overwrite "Balanced" custom map option
	if (m_eNormalizationLevel != NORMALIZE_HIGH)
		setStartingPlotNormalizationLevel(); // </advc.108>
	setPlayerColors(); // advc.002i

	for (TeamIter<> itTeam; itTeam.hasNext(); ++itTeam)
	{
		itTeam->meet(itTeam->getID(), false);
		if (!itTeam->isMajorCiv())
		{
			for (TeamIter<CIV_ALIVE> itCiv; itCiv.hasNext(); ++itCiv) // advc.003m: ALIVE
			{
				if(itTeam->getID() != itCiv->getID())
					itTeam->declareWar(itCiv->getID(), false, NO_WARPLAN);
			}
		}
	}

	// Forced peace at the beginning of Advanced starts
	if (isOption(GAMEOPTION_ADVANCED_START) &&
		/*  advc.250b: No need to protect the AI from the player when human
			start is advanced, and AI won't start wars within 10 turns anyway. */
		!isOption(GAMEOPTION_SPAH))
	{
		CLinkList<TradeData> items1;
		CLinkList<TradeData> items2;
		{
			TradeData peaceTreaty(TRADE_PEACE_TREATY);
			items1.insertAtEnd(peaceTreaty);
			items2.insertAtEnd(peaceTreaty);
		}
		// advc.001: I don't think this should happen for minor civs
		for (PlayerIter<MAJOR_CIV> itFirst; itFirst.hasNext(); ++itFirst)
		{
			for (PlayerIter<MAJOR_CIV> itSecond; itSecond.hasNext(); ++itSecond)
			{
				if (itFirst->getID() <= itSecond->getID())
					continue;
				if (GET_TEAM(itFirst->getTeam()).canChangeWarPeace(itSecond->getTeam()))
					implementDeal(itFirst->getID(), itSecond->getID(), items2, items2);
			}
		}
	}
}

/*	advc.002i: Assign unique player colors in games where multiple players
	have the same civ type */
void CvGame::setPlayerColors()
{
	std::vector<std::pair<PlayerTypes,int> > aeiReassign;
	EnumMap<CivilizationTypes,int> aiPlayersPerCiv;
	for (PlayerIter<ALIVE> it; it.hasNext(); ++it)
	{
		CivilizationTypes eCiv = it->getCivilizationType();
		aiPlayersPerCiv.add(eCiv, 1);
		int iPlayers = aiPlayersPerCiv.get(eCiv);
		if (iPlayers > 1)
			aeiReassign.push_back(std::make_pair(it->getID(), iPlayers));
	}
	for (size_t i = 0; i < aeiReassign.size(); i++)
	{
		PlayerTypes const eReassignPlayer = aeiReassign[i].first;
		bool const bMatchSecondary = (aeiReassign[i].second == 2);
		bool const bRandomize = (aeiReassign[i].second > 3);
		CivilizationTypes eBestCiv = NO_CIVILIZATION;
		float fSmallestDiff = FLT_MAX;
		CivilizationTypes const eCiv = GET_PLAYER(eReassignPlayer).
				getCivilizationType();
		CvPlayerColorInfo const& kDefaultColor = GC.getInfo(
				(PlayerColorTypes)GC.getInfo(eCiv).getDefaultPlayerColor());
		/*	Look for a color similar to the secondary color of eCiv.
			(If we try to match the primary color, it'll be difficult
			to distinguish from the first player of eCiv.) */
		NiColorA const& kTargetColor = GC.getInfo(kDefaultColor.
				getColorTypeSecondary()).getColor();
		/*	For the third player per civ, try matching a mix of primary
			and secondary color. For subsequent players, pick a random color. */
		NiColorA const* pTargetColor2 = (bMatchSecondary || bRandomize) ? NULL :
				&GC.getInfo(kDefaultColor.getColorTypePrimary()).getColor();
		FOR_EACH_ENUM2(Civilization, eLoopCiv)
		{
			if (aiPlayersPerCiv.get(eLoopCiv) > 0)
				continue;
			float fDiffValue = 0;
			if (bRandomize)
			{
				/*	Since RGB values are stored as float, it's conceivable
					that different colors could get chosen on different machines
					in multiplayer. That should be OK, but I'd very much prefer for
					everyone to use the same colors, so I'm not going to use
					GC.getASyncRand. */
				fDiffValue += getSorenRandNum(10000, "setPlayerColors");
			}
			else
			{
				// Candidate color: primary color of an unused civ
				NiColorA const& kLoopColor = GC.getInfo(GC.getInfo(
						(PlayerColorTypes)GC.getInfo(eLoopCiv).
						getDefaultPlayerColor()).getColorTypePrimary()).getColor();
				fDiffValue += ::colorDifference(kTargetColor, kLoopColor);
				if (pTargetColor2 != NULL)
					fDiffValue += ::colorDifference(*pTargetColor2, kLoopColor);
			}
			if (fDiffValue < fSmallestDiff)
			{
				fSmallestDiff = fDiffValue;
				eBestCiv = eLoopCiv;
			}
		}
		FAssert(eBestCiv != NO_CIVILIZATION || MAX_PLAYERS > GC.getNumCivilizationInfos());
		if (eBestCiv != NO_CIVILIZATION)
		{
			GC.getInitCore().setColor(eReassignPlayer, (PlayerColorTypes)
					GC.getInfo(eBestCiv).getDefaultPlayerColor());
			aiPlayersPerCiv.add(eBestCiv, 1);
		}
		/*	(Else keep the colors assigned by the EXE. They're picked from the back
			of Civ4PlayerColorInfos.xml. Not guaranteed to be unique.) */
	}
}

// advc.127:
void CvGame::initGameHandicap()
{
	/*	K-Mod: Adjust the game handicap level to be the average
		of all the human player's handicap. (Note: in the original bts rules,
		it would always set to Noble if the humans had different handicaps) */
	// advc: Moved from setInitialItems b/c that function isn't called in scenarios
	if (isGameMultiPlayer())
	{
		int iSum = 0;
		PlayerIter<HUMAN> it;
		for (; it.hasNext(); ++it)
			iSum += GC.getInfo(it->getHandicapType())./* advc.250a: */getDifficulty();
		int const iDiv = it.nextIndex();
		if (iDiv > 0)
		{
			/*  advc.250a: Relies on no strange new handicaps being placed
				between Settler and Deity. Same in CvTeam::getHandicapType. */
				setHandicapType((HandicapTypes) /* kekm.22: */ intdiv::round(
						iSum, 10 * iDiv));
		}
		FAssertMsg(iDiv > 0, "All-AI game. Not necessarily wrong, but unexpected.");
	} // K-Mod end

	// Set m_eAIHandicap to the average of AI handicaps
	int iHandicapSum = 0;
	int iDiv = 0;
	for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
	{
		if (!it->isHuman())
		{
			iHandicapSum += it->getHandicapType();
			iDiv++;
		}
	}
	if(iDiv > 0) // Leaves it at STANDARD_HANDICAP in all-human games
		m_eAIHandicap = (HandicapTypes)intdiv::uround(iHandicapSum, iDiv);
}


void CvGame::initFreeState()
{
	initGameHandicap(); // advc.127
	// <advc.084>
	if (getCivTeamsEverAlive() == 1)
	{
		FOR_EACH_ENUM(Victory)
		{
			CvVictoryInfo const& kLoopVictory = GC.getInfo(eLoopVictory);
			if (kLoopVictory.isConquest() || kLoopVictory.isDiploVote()
				/*	Domination victory isn't going to be fun, but let's
					let the player decide whether to disable it. */
				/*|| kLoopVictory.getPopulationPercentLead() > 0*/)
			{
				setVictoryValid(eLoopVictory, false);
			}
		}
	} // </advc.084>
	// <advc.250b>
	if(!isOption(GAMEOPTION_ADVANCED_START) ||
		PlayerIter<HUMAN>::count() == PlayerIter<CIV_ALIVE>::count())
	{
		setOption(GAMEOPTION_SPAH, false);
	}
	if(isOption(GAMEOPTION_SPAH))
		// Reassigns start plots and start points
		m_pSpah->setInitialItems(); // </advc.250b>
	if (GC.getInitCore().isScenario())
	{
		setScenario(true); // advc.052
		AI().AI_initScenario(); // advc.104u
	}
	else // advc.051: (Moved up.) Don't force 0 gold in scenarios
	{
		for (PlayerIter<ALIVE> it; it.hasNext(); ++it)
			it->initFreeState();
	}
	applyOptionEffects(); // advc.310
	FOR_EACH_ENUM(Tech)
	{
		for (TeamIter<ALIVE> itTeam; itTeam.hasNext(); ++itTeam)
		{
			CvTeam& kTeam = *itTeam;
			bool bValid = false;
			// disabled by K-Mod. (moved & changed. See below)
			if (//(GC.getInfo(getHandicapType()).isFreeTechs(eLoopTech)) ||
				(!kTeam.isHuman() && GC.getInfo(getHandicapType()).isAIFreeTechs(eLoopTech) &&
				// advc.001: Barbarians receiving free AI tech might be a bug
				!kTeam.isBarbarian() /* advc.250c: */ && !isOption(GAMEOPTION_ADVANCED_START)) ||
				GC.getInfo(eLoopTech).getEra() < getStartEra())
			{
				bValid = true;
			}
			if (!bValid)
			{
				for (MemberIter itMember(kTeam.getID()); itMember.hasNext(); ++itMember)
				{
					CvPlayer& kMember = *itMember;
					/*  <advc.250b> <advc.250c> Always grant civ-specific tech,
						but not tech from handicap if Advanced Start except to
						human civs that don't actually start Advanced (SPaH option). */
					if (GC.getInfo(kMember.getCivilizationType()).isCivilizationFreeTechs(eLoopTech))
					{
						bValid = true;
						break;
					}
					if (!bValid &&
						// K-Mod (give techs based on player handicap, not game handicap.)
						GC.getInfo(kMember.getHandicapType()).isFreeTechs(eLoopTech)
						&& (!isOption(GAMEOPTION_ADVANCED_START) ||
						(isOption(GAMEOPTION_SPAH) && kTeam.isHuman())))
						// </advc.250b> </advc.250c>
					{
						bValid = true;
						break;
					}
				}
			}
			if (!bValid)
				continue;
			// <advc.126> Later-era free tech only for later-era starts.
			if(GC.getInfo(eLoopTech).getEra() > getStartEra())
				continue; // </advc.126>
			// (advc.051: Don't take away techs granted by the scenario)
			kTeam.setHasTech(eLoopTech, true, NO_PLAYER, false, false);
			if (GC.getInfo(eLoopTech).isMapVisible())
				GC.getMap().setRevealedPlots(kTeam.getID(), true, true);
		}
	}  // <advc.051>
	if(isScenario() && getStartEra() <= 0) // Set start era based on player era
	{
		int iEraSum = 0;
		PlayerIter<MAJOR_CIV> it;
		for (; it.hasNext(); ++it)
			iEraSum += it->getCurrentEra();
		int iStartEra = iEraSum / std::max(it.nextIndex(), 1);
		if(iStartEra > getStartEra())
			GC.getInitCore().setEra((EraTypes)iStartEra);
	}
	m_eInitialActivePlayer = getActivePlayer(); // advc.106h
}


void CvGame::initScenario()
{
	initFreeState(); // Tech from handicap
	// <advc.030>
	if(GC.getDefineBOOL("PASSABLE_AREAS"))
	{
		/*  recalculateAreas can't handle preplaced cities. Or perhaps it can
			(Barbarian cities are fine in most cases), but there's going to
			be other stuff, like free units, that causes problems. */
		for(int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			if(GET_PLAYER((PlayerTypes)i).getNumCities() > 0)
				return;
		}
		GC.getMap().recalculateAreas();
	} // </advc.030>
}

void CvGame::initFreeUnits()
{
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
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayer const& kPlayer = GET_PLAYER((PlayerTypes)i);
		if(kPlayer.isAlive() && kPlayer.getAdvancedStartPoints() > 0)
		{
			bValid = true;
			break;
		}
	}
	if(!bValid)
	{
		setOption(GAMEOPTION_SPAH, false);
		setOption(GAMEOPTION_ADVANCED_START, false);
	} // </advc.250b>
}


void CvGame::initFreeUnits_bulk() // </advc.051>
{
	// kekm.28: exclude Barbarians
	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		if (itPlayer->getNumUnits() == 0 && itPlayer->getNumCities() == 0)
			itPlayer->initFreeUnits();
	}
}

/*  advc.310: For building (or other) effects that only apply when certain
	game options are set. */
void CvGame::applyOptionEffects(bool bEnableAll)
{
	CvBuildingInfo::setDomesticGreatGeneralRateModifierEnabled(bEnableAll ||
			isOption(GAMEOPTION_RAGING_BARBARIANS) || isOption(GAMEOPTION_NO_BARBARIANS));
	CvBuildingInfo::setAreaTradeRoutesEnabled(bEnableAll ||
			!isOption(GAMEOPTION_RAGING_BARBARIANS) || isOption(GAMEOPTION_NO_BARBARIANS));
	CvBuildingInfo::setAreaBorderObstacleEnabled(bEnableAll ||
			!isOption(GAMEOPTION_NO_BARBARIANS));
}

// advc.027: Return value added; to be (safe-)deleted by caller.
NormalizationTarget* CvGame::assignStartingPlots()
{
	PROFILE_FUNC();
	CvMap const& kMap = GC.getMap();
	bool const bScenario = GC.getInitCore().getWBMapScript();
	/*	(advc: BtS code for handling scenarios deleted)
		K-Mod. Same functionality, but much faster and easier to read. */
	{
		/*	First, make a list of all the pre-marked starting plots on the map.
			advc (note): Normally, only scenarios can have such marked plots. */
		std::vector<CvPlot*> apStartingPlots;
		for (int i = 0; i < kMap.numPlots(); i++)
		{	// advc.opt: Shouldn't be necessary; the loop body is very fast.
			//gDLL->callUpdater(); // allow window updates during launch
			CvPlot* pLoopPlot = kMap.plotByIndex(i);
			if (pLoopPlot->isStartingPlot())
				apStartingPlots.push_back(pLoopPlot);
		}
		// Now, randomly assign a starting plot to each player.
		for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext() &&
			apStartingPlots.size() > 0; ++itPlayer)
		{
			if (itPlayer->getStartingPlot() != NULL || // Already got one
				/*	advc.027: Don't assign one of the marked plots when the
					scenario has set the RandomStartLocation flag for the player */
				itPlayer->isRandomWBStart())
			{
				continue;
			}
			// advc.027b: was getSorenRandNum
			int iRandOffset = getMapRandNum(apStartingPlots.size(), "Starting Plot");
			itPlayer->setStartingPlot(apStartingPlots[iRandOffset]/*, true*/); // advc.opt
			// remove this plot from the list.
			apStartingPlots[iRandOffset] = apStartingPlots[apStartingPlots.size() - 1];
			apStartingPlots.pop_back();
		}
	} // K-Mod end

	if (GC.getPythonCaller()->callMapFunction("assignStartingPlots"))
		return /* <advc.027> */ NULL;

	NormalizationTarget* pNormalizationTarget = NULL;
	/*	If the map script allows it, StartingPositionIteration will
		set starting sites that the code below may then reassign among the civs. */
	StartingPositionIteration spi;
	pNormalizationTarget = spi.createNormalizationTarget();
	// Apply SPI team assignment (no effect if SPI hasn't actually been executed)
	std::vector<CvPlot*> apStartsPerPlayer(MAX_CIV_PLAYERS);
	for (TeamIter<CIV_ALIVE> itTeam; itTeam.hasNext(); ++itTeam)
	{
		for (MemberIter itMember(itTeam->getID()); itMember.hasNext(); ++itMember)
		{
			apStartsPerPlayer[itMember->getID()] = spi.getTeamSite(
					itTeam->getID(), itMember.nextIndex() - 1);
		}
	}
	bool const bTeamGame = isTeamGame();
	bool bTeamAssignmentDone = bTeamGame;
	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		if (apStartsPerPlayer[itPlayer->getID()] != NULL)
			itPlayer->setStartingPlot(apStartsPerPlayer[itPlayer->getID()]);
		else
		{
			bTeamAssignmentDone = false;
			// SPI should do all the work or none
			FAssert(itPlayer.nextIndex() == 1 && pNormalizationTarget == NULL);
			break;
		}
	}
	// Reassignment of starting sites makes debugging harder
	if (pNormalizationTarget != NULL && spi.isDebug())
		return pNormalizationTarget; // </advc.027>
	if (bTeamGame)
	{
		if (!bTeamAssignmentDone) // advc.027
		{
			EnumMap<PlayerTypes,bool> abPlayerDone; // advc
			// BtS team assignment
			for (int iPass = 0; iPass < 2 * MAX_PLAYERS; iPass++)
			{
				bool bStartFound = false;
				// advc.027b: was getSorenRandNum
				int const iRandOffset = getMapRandNum(countCivTeamsAlive(),
						"Team Starting Plot");
				gDLL->callUpdater(); // advc
				for (int i = 0; i < MAX_CIV_TEAMS; i++)
				{
					TeamTypes eLoopTeam = (TeamTypes)((i + iRandOffset) % MAX_CIV_TEAMS);
					if (!GET_TEAM(eLoopTeam).isAlive())
						continue;
					for (MemberIter itMember(eLoopTeam); itMember.hasNext(); ++itMember)
					{
						if (abPlayerDone.get(itMember->getID()))
							continue;
						if (itMember->getStartingPlot() == NULL)
						{
							itMember->setStartingPlot(itMember->findStartingPlot()/*, true*/); // advc.opt
							abPlayerDone.set(itMember->getID(), true);
						}
						if (itMember->getStartingPlot() != NULL)
						{
							bStartFound = true;
							break;
						}
					}
				}
				if (!bStartFound)
					break;
			}
			#ifdef FASSERT_ENABLE
			for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
			{
				FAssert(it->getStartingPlot() != NULL);
			}
			#endif
		}
		bool bRearrange = !bTeamAssignmentDone;
		/*	<advc.027> Run the BtS rearrangement code on top of SPI
			(though not across areas) when the continents are relatively large.
			(On archipelagic maps, I think the BtS code would do more harm than good
			b/c it doesn't take into account shallow-water connections.) */
		if (!bRearrange)
		{
			std::set<int> startingAreas;
			PlayerIter<CIV_ALIVE> itPlayer;
			for (; itPlayer.hasNext(); ++itPlayer)
				startingAreas.insert(itPlayer->getStartingPlot()->getArea().getID());
			if (2 * itPlayer.nextIndex() >= 7 * (int)startingAreas.size())
				bRearrange = true;
		} // </advc.027>
		if (bRearrange)
		{
			/*	advc: assignStartingPlots shouldn't get called for scenarios with
				fixed civs and leaders. (As far as I can tell, a scenario can only
				have fixed civs and leaders for all players, or for none.) */
			FAssert(!bScenario || GC.getInitCore().getWBMapNoPlayers());
			// advc.108b: Moved from normalizeStartingPlots
			if (!GC.getPythonCaller()->callMapFunction("normalizeStartingPlotLocations"))
			{
				rearrangeTeamStarts(
						// advc.027:
						bTeamAssignmentDone, bTeamAssignmentDone ? per100(15) : 0);
			}
		}
	}
	else // If map script [advc.027: and SPI] haven't set a plot
	{
		for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
		{
			if (itPlayer->getStartingPlot() == NULL)
			{
				gDLL->callUpdater();
				itPlayer->setStartingPlot(itPlayer->findStartingPlot()/*, true*/); // advc.opt
			}
			if (itPlayer->getStartingPlot() == NULL)
			{
				FErrorMsg("No starting plot found");
				continue;
			}
		}
	}
	/*	advc.108b: Code moved into subroutine.
		advc.027 (note): Would be better to do this _after_ normalization.
		But that would require a NormalizationTarget member function for
		updating the start values. */
	applyStartingLocHandicaps(pNormalizationTarget);
	return (bScenario ? NULL : pNormalizationTarget); // advc.027
}

/*	advc.108b: Based on code cut from assignStartingPlots.
	Easier to implement this in a comparator.
	For sorting players in descending order of their team start values
	and, within a team, in descending order of the player start values. */
namespace
{
	class StartingSiteComparator
	{
	public:
		StartingSiteComparator(NormalizationTarget const* pStartValues)
		:	m_pStartValues(pStartValues)
		{}
		bool operator()(PlayerTypes ePlayer1, PlayerTypes ePlayer2) const
		{
			TeamTypes const eTeam1 = TEAMID(ePlayer1);
			TeamTypes const eTeam2 = TEAMID(ePlayer2);
			if (eTeam1 == eTeam2)
			{
				scaled rValue1 = playerStartValue(ePlayer1);
				scaled rValue2 = playerStartValue(ePlayer2);
				if (rValue1 == rValue2)
					return (ePlayer1 < ePlayer2);
				return (rValue1 > rValue2);
			}
			scaled rValue1 = teamStartValue(eTeam1);
			scaled rValue2 = teamStartValue(eTeam2);
			if (rValue1 == rValue2)
				return (eTeam1 < eTeam2);
			return (rValue1 > rValue2);
		}
	private:
		NormalizationTarget const* m_pStartValues;
		scaled playerStartValue(PlayerTypes ePlayer) const
		{
			CvPlot const* pStartPlot = GET_PLAYER(ePlayer).getStartingPlot();
			scaled r = -1;
			if (pStartPlot == NULL)
				return r;
			if (m_pStartValues == NULL)
			{
				/*	pStartPlot->getFoundValue(ePlayer) would be faster,
					but CvPlot::setFoundValue may not have been called
					(and then it returns 0). */
				scaled r = GET_PLAYER(ePlayer).AI_foundValue(
						pStartPlot->getX(), pStartPlot->getY(), -1,
						false, true); // advc.027: bNormalize instead of bStartingLoc
				FAssertMsg(r.isPositive(), "Starting site likely unplayable");
				return r;
			}
			return m_pStartValues->getStartValue(*pStartPlot); // advc.027
		}
		scaled teamStartValue(TeamTypes eTeam) const
		{
			scaled r;
			for (MemberIter itMember(eTeam); itMember.hasNext(); ++itMember)
			{
				r += playerStartValue(itMember->getID());
			}
			return r;
		}
	};
}

// advc.108b: Cut from assignStartingPlots
void CvGame::applyStartingLocHandicaps(
	NormalizationTarget const* pStartValues) // advc.027
{
	/*	Replace all this. Don't want to ignore StartingLocPercent
		in multiplayer games, and the BtS random assignment of human starts
		didn't really work - had favored player 0 when humans are in slots 0, 1 ... */
	/*else if (isGameMultiPlayer()) {
		int iRandOffset = getMapRandNum(PlayerIter<CIV_ALIVE>::count(), "Player Starting Plot");
		// ... (deleted on 14 June 2020)
	}
	else
	{	// advc (comment): The minus 1 prevents humans from getting the worst plot
		int const iUpperBound = PlayerIter<CIV_ALIVE>::count() - 1;
		// ...
	}
	//std::vector<PlayerTypes>::iterator itPlayerOrder; //iterate over the player starts in the original order and re-place them.
	for (itPlayerOrder = aePlayerOrder.begin(); itPlayerOrder != aePlayerOrder.end(); ++itPlayerOrder)
		GET_PLAYER(*itPlayerOrder).setStartingPlot(GET_PLAYER(*playerOrderIter).findStartingPlot(), true);*/

	/*	The basic approach, as in K-Mod, is to sort the players by handicap,
		starting with the least handicapped player, and to sort the starting sites
		by value, starting with the best site. In the end, the sorted sites are
		assigned to the sorted players. */
	std::vector<PlayerTypes> aePlayersByHandicap; // ("PlayerOrder" in K-Mod)
	/*	First, order the teams according to handicap, then the members of each team.
		This way, the same algorithm works for team games and non-team games.
		(BtS had not taken StartingLoc handicaps into account in team games.) */
	std::vector<CvTeam*> apTeamsByHandicap;
	std::vector<std::pair<CvTeam*,int> > aStartingLocPercentPerTeam;
	// I don't think my approach can work when the teams don't have a uniform size
	int iTeamSize = 0;
	for (TeamIter<CIV_ALIVE> itTeam; itTeam.hasNext(); ++itTeam)
	{
		if (iTeamSize == 0)
			iTeamSize = itTeam->getNumMembers();
		else if (iTeamSize != itTeam->getNumMembers())
			return;
		int iStartingLocPercent = 0;
		MemberIter itMember(itTeam->getID());
		for (; itMember.hasNext(); ++itMember)
		{
			iStartingLocPercent += GC.getInfo(itMember->getHandicapType()).
					getStartingLocationPercent();
		}
		aStartingLocPercentPerTeam.push_back(std::make_pair(
				&*itTeam, intdiv::round(iStartingLocPercent, itMember.nextIndex())));
	}
	sortByStartingLocHandicap(aStartingLocPercentPerTeam, apTeamsByHandicap);
	for (size_t i = 0; i < apTeamsByHandicap.size(); i++)
	{
		std::vector<CvPlayer*> apMembersByHandicap;
		std::vector<std::pair<CvPlayer*,int> > aStartingLocPercentPerMember;
		for (MemberIter itMember(apTeamsByHandicap[i]->getID());
			itMember.hasNext(); ++itMember)
		{
			aStartingLocPercentPerMember.push_back(std::make_pair(
					&*itMember, GC.getInfo(itMember->getHandicapType()).
					getStartingLocationPercent()));
		}
		sortByStartingLocHandicap(aStartingLocPercentPerMember, apMembersByHandicap);
		for (size_t j = 0; j < apMembersByHandicap.size(); j++)
		{
			aePlayersByHandicap.push_back(apMembersByHandicap[j]->getID());
		}
	}

	std::vector<PlayerTypes> aePlayersByStartValue;
	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
		aePlayersByStartValue.push_back(itPlayer->getID());
	if (aePlayersByStartValue.size() != aePlayersByHandicap.size())
	{
		FAssert(aePlayersByStartValue.size() == aePlayersByHandicap.size());
		return;
	}
	{
		StartingSiteComparator comp(/* advc.027: */ pStartValues);
		std::sort(aePlayersByStartValue.begin(), aePlayersByStartValue.end(), comp);
	}
	std::vector<CvPlot*> apStartingSitesByValue;
	apStartingSitesByValue.reserve(aePlayersByStartValue.size());
	for (size_t i = 0; i < aePlayersByStartValue.size(); i++)
	{
		apStartingSitesByValue.push_back(GET_PLAYER(aePlayersByStartValue[i]).
				getStartingPlot());
	}

	// <advc.027> Try to avoid giving human players high-volatility starts
	if (pStartValues != NULL && aePlayersByHandicap.size() > 5u &&
		!isTeamGame())
	{
		for (size_t i = 0; i < aePlayersByHandicap.size(); i++)
		{
			PlayerTypes const ePlayer = aePlayersByHandicap[i];
			if (ePlayer == NO_PLAYER || !GET_PLAYER(ePlayer).isHuman())
				continue;
			CvPlot const* pStart = GET_PLAYER(ePlayer).getStartingPlot();
			if (pStart == NULL)
				continue;
			scaled rVolatility = pStartValues->getVolatilityValue(*pStart);
			if (rVolatility < fixp(0.2))
				continue;
			/*	Don't want to undercut the handicap bias too much. Hence look for
				a less volatile start only one up and one down in aePlayersByHandicap. */
			std::vector<std::pair<int,PlayerTypes> > aieSwapPlayers;
			if (i > 0)
			{
				aieSwapPlayers.push_back(std::make_pair((int)
						(i - 1), aePlayersByHandicap[i - 1]));
			}
			if (i < aePlayersByHandicap.size() - 1)
			{
				aieSwapPlayers.push_back(std::make_pair((int)
						(i + 1), aePlayersByHandicap[i + 1]));
			}
			int iBestSwapIndex = -1;
			// Tiny improvements in volatility aren't worth swapping for
			scaled rBestSwapVal = fixp(0.1);
			for (size_t j = 0; j < aieSwapPlayers.size(); j++)
			{
				PlayerTypes const eSwapPlayer = aieSwapPlayers[j].second;
				if (eSwapPlayer == NO_PLAYER || GET_PLAYER(eSwapPlayer).isHuman())
					continue;
				int const iSwapPlayerIndex = aieSwapPlayers[j].first;		
				CvPlot const* pSwapStart = apStartingSitesByValue[iSwapPlayerIndex];
				if (pSwapStart == NULL)
					continue;
				scaled rSwapVolatility = pStartValues->getVolatilityValue(*pSwapStart);
				scaled rSwapVal = rVolatility - rSwapVolatility;
				if (rSwapVal > rBestSwapVal)
				{
					rBestSwapVal = rSwapVal;
					iBestSwapIndex = iSwapPlayerIndex;
				}
			}
			if (iBestSwapIndex >= 0)
			{
				std::swap(aePlayersByHandicap[i], aePlayersByHandicap[iBestSwapIndex]);
				if (iBestSwapIndex == i + 1)
					i++; // Skip next iteration to make sure not to swap again
			}
		}
	} // </advc.027>
	for (size_t i = 0; i < aePlayersByHandicap.size(); i++)
	{
		if (aePlayersByHandicap[i] == NO_PLAYER ||
			apStartingSitesByValue[i] == NULL)
		{
			FErrorMsg("Failed to apply StartingLoc handicap");
			continue;
		}
		GET_PLAYER(aePlayersByHandicap[i]).setStartingPlot(apStartingSitesByValue[i]);
	}
}

/*	advc.108b: Based on BtS code cut from assignStartingPlots.
	The Agent type can be either CvPlayer or CvTeam. The agents have to be
	alive and non-Barbarian. kResult should be empty before the call. */
template<class Agent>
CvGame::sortByStartingLocHandicap(
	std::vector<std::pair<Agent*,int> > const& kStartingLocPercentPerAgent,
	std::vector<Agent*>& kResult)
{
	int const iAgents = kStartingLocPercentPerAgent.size();
	int iHumanAgents = 0;
	for (int i = 0; i < iAgents; i++)
	{
		if (kStartingLocPercentPerAgent[i].first->isHuman())
			iHumanAgents++;
	}
	int const iAIAgents = iAgents - iHumanAgents;
	FAssert(kResult.empty());
	kResult.resize(iAgents, NULL);
	for (int iPass = 0; iPass < 2; iPass++)
	{
		bool bHuman = (iPass == 0);
		int iLoopAgents = (bHuman ? iHumanAgents : iAIAgents);
		int iRandOffset = getMapRandNum(iLoopAgents, "sortByStartingLocHandicap");
		int iSkipped = 0;
		for (int i = 0; i < iAgents; i++)
		{
			if (kStartingLocPercentPerAgent[i].first->isHuman() == bHuman)
			{
				if (iSkipped < iRandOffset)
				{
					iSkipped++;
					continue;
				}
				/*  This sets iRandOffset to the index of a random human agent
					in the first pass, and a random AI agent in the second. */
				iRandOffset = i;
				break;
			}
		}
		for (int i = 0; i < iAgents; i++)
		{
			std::pair<Agent*,int> piLoopPair = kStartingLocPercentPerAgent[i];
			Agent& kAgent = *piLoopPair.first;
			if (kAgent.isHuman() != bHuman)
				continue;
			int iPos = ::range((iAgents * piLoopPair.second) / 100, 0, iAgents - 1);
			if (kResult[iPos] != NULL) // If pos already taken
			{
				for (int j = 1; j < std::max(iPos + 1, iAgents - iPos); j++)
				{
					// Alternate between better and worse positions
					if (iPos + j < iAgents && kResult[iPos + j] == NULL)
					{
						iPos += j;
						break;
					}
					if (iPos - j >= 0 && kResult[iPos - j] == NULL)
					{
						iPos -= j;
						break;
					}
				}
				FAssert(kResult[iPos] == NULL);
			}
			kResult[iPos] = &kAgent;
		}
	}
}

/*	Swaps starting locations until we have reached the
	optimal closeness between teams. (caveat: this isn't quite "optimal"
	because we could get stuck in local minima, but it's pretty good.)
	advc: Renamed from "normalizeStartingPlotLocations". No longer part of the
	normalization step. Refactored, but the code structure is still as in BtS. */
void CvGame::rearrangeTeamStarts(/* advc.027: */ bool bOnlyWithinArea, scaled rInertia)
{	// <advc.opt> This function is only for team games
	if(!isTeamGame())
		return; // </advc.opt>
	CvMap const& kMap = GC.getMap();
	// Precompute distances between all starting sites
	EnumMap2D<PlayerTypes,PlayerTypes,int> aaiDistances;
	for (PlayerIter<CIV_ALIVE> itFirstPlayer; itFirstPlayer.hasNext(); ++itFirstPlayer)
	{
		gDLL->callUpdater(); // allow window to update during launch
		CvPlot* pFirstStart = itFirstPlayer->getStartingPlot();
		if (pFirstStart == NULL)
			continue;
		PlayerTypes const eFirst = itFirstPlayer->getID();
		for (PlayerIter<CIV_ALIVE> itSecondPlayer; itSecondPlayer.hasNext();
			++itSecondPlayer)
		{
			CvPlot* pSecondStart = itSecondPlayer->getStartingPlot();
			if (pSecondStart == NULL)
				continue;
			PlayerTypes const eSecond = itSecondPlayer->getID();
			if (eSecond < eFirst)
				continue;
			int iDist = kMap.calculatePathDistance(pFirstStart, pSecondStart);
			if (iDist == -1)
			{
				iDist = kMap.plotDistance(pFirstStart, pSecondStart);
				/*	advc.027: The BtS penalty below is (presumably) mainly intended
					as an incentive for putting sites of the same team in the same area.
					But it also provides a strong incentive for minimizing distances
					between team sites in different areas when it's not possible
					to put them all in one area. (Especially) don't want this
					side-effect when swaps between areas aren't allowed. */
				if (!bOnlyWithinArea)
					iDist *= 5;
			}
			aaiDistances.set(eFirst, eSecond, iDist);
			aaiDistances.set(eSecond, eFirst, iDist);
		}
	}

	std::vector<PlayerTypes> aeStartingLocs(MAX_CIV_PLAYERS);
	// each player starting in own location
	std11::iota(aeStartingLocs.begin(), aeStartingLocs.end(), (PlayerTypes)0);

	int iBestScore = getTeamClosenessScore(aaiDistances, aeStartingLocs);
	bool bFoundSwap = true;
	/*	<advc.027> I worry that going through the players in turn order
		can lead to biases toward or against the (human) team of player 0.
		(PlayerIter unfortunately only knows how to use SRand; I want MapRand here.) */
	int aiPlayersShuffled[MAX_CIV_PLAYERS];
	::shuffleArray(aiPlayersShuffled, MAX_CIV_PLAYERS, getMapRand()); // </advc.027>
	while (bFoundSwap)
	{
		bFoundSwap = false;
		for (int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			CvPlayer& kFirst = GET_PLAYER((PlayerTypes)aiPlayersShuffled[i]); // advc.027
			if (!kFirst.isAlive())
				continue;
			for (int j = 0; j < /* advc.027: */ i; j++)
			{
				CvPlayer& kSecond = GET_PLAYER((PlayerTypes)aiPlayersShuffled[j]); // advc.027
				if (!kSecond.isAlive())
					continue;
				// <advc.027>
				if (bOnlyWithinArea &&
					// (The rest of this function also tolerates NULL starts)
					kFirst.getStartingPlot() != NULL && kSecond.getStartingPlot() != NULL &&
					!kFirst.getStartingPlot()->sameArea(*kSecond.getStartingPlot()))
				{
					continue;
				} // </advc.027>
				std::swap(aeStartingLocs[kFirst.getID()],
						aeStartingLocs[kSecond.getID()]);
				int iScore = getTeamClosenessScore(aaiDistances, aeStartingLocs);
				if (iScore * (1 + rInertia) < iBestScore) // advc.027: inertia
				{
					iBestScore = iScore;
					bFoundSwap = true;
				}
				else
				{	// undo
					std::swap(aeStartingLocs[kSecond.getID()],
							aeStartingLocs[kFirst.getID()]);
				}
			}
		}
	}

	std::vector<CvPlot*> apNewStartPlots(MAX_CIV_PLAYERS, NULL);
	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		PlayerTypes const ePlayer = itPlayer->getID();
		if (aeStartingLocs[ePlayer] != ePlayer)
		{
			apNewStartPlots[ePlayer] = GET_PLAYER(aeStartingLocs[ePlayer]).
					getStartingPlot();
		}
	}
	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		PlayerTypes const ePlayer = itPlayer->getID();
		if (apNewStartPlots[ePlayer] != NULL)
			GET_PLAYER(ePlayer).setStartingPlot(apNewStartPlots[ePlayer]);
	}
}

/*	For each of n teams, let the closeness score for that team be the
	average distance of an edge between two players on that team.
	This function calculates the closeness score for each team and
	returns the sum of those n scores. The lower the result, the better "clumped"
	the players' starting locations are.
	Note: for the purposes of this function, player i will be assumed to start
	in the location of player kStartingLocs[i] */
int CvGame::getTeamClosenessScore(  // advc: params used to be arrays
	EnumMap2D<PlayerTypes,PlayerTypes,int> const& kDistances,
	std::vector<PlayerTypes> const& kStartingLocs)
{
	int iScore = 0;
	for (TeamIter<CIV_ALIVE> itTeam; itTeam.hasNext(); ++itTeam)
	{
		int iTeamTotalDist = 0;
		int iNumEdges = 0;
		for (MemberIter itFirstMember(itTeam->getID()); itFirstMember.hasNext();
			++itFirstMember)
		{
			for (MemberIter itSecondMember(itTeam->getID()); itSecondMember.hasNext();
				++itSecondMember)
			{
				PlayerTypes const eFirst = itFirstMember->getID();
				PlayerTypes const eSecond = itSecondMember->getID();
				if (eFirst <= eSecond)
					continue;
				// Add the edge between these two players that are on the same team
				iNumEdges++;
				PlayerTypes eFirstStart = kStartingLocs.at(eFirst);
				PlayerTypes eSecondStart = kStartingLocs.at(eSecond);
				if (eFirstStart < eSecondStart) // Ensure eFirstStart > eSecondStart
					std::swap(eFirstStart, eSecondStart);
				else FAssertMsg(eFirstStart != eSecondStart,
						"Two players are (hypothetically) assigned to the same starting location!");
				FAssertMsg(kDistances.get(eFirstStart, eSecondStart) > 0, "Distance not computed?"); // advc
				iTeamTotalDist += kDistances.get(eFirstStart, eSecondStart);
			}
		}
		int iTeamScore;
		if (iNumEdges == 0)
			iTeamScore = 0;
		else
		{
			// The avg distance between team edges is the team score
			iTeamScore = intdiv::uround(iTeamTotalDist, iNumEdges);
		}
		iScore += iTeamScore;
	}
	return iScore;
}

// <advc.108>
void CvGame::setStartingPlotNormalizationLevel(StartingPlotNormalizationLevel eLevel)
{
	if (eLevel == NORMALIZE_DEFAULT)
	{
		eLevel = (GC.getDefineBOOL("NORMALIZE_STARTPLOTS_AGGRESSIVELY") ?
				NORMALIZE_HIGH : NORMALIZE_LOW);
		if (eLevel == NORMALIZE_LOW && isGameMultiPlayer() &&
			TeamIter<HUMAN>::count() > 1)
		{
			eLevel = NORMALIZE_MEDIUM;
		}
	}
	m_eNormalizationLevel = eLevel;
}

/*	(Note: Only for external callers.
	Within CvGame, m_eNormalizationLevel gets accessed directly.) */
CvGame::StartingPlotNormalizationLevel CvGame::getStartingPlotNormalizationLevel() const
{
	return m_eNormalizationLevel;
} // </advc.108

// advc.opt: Replacing CvPlayer::startingPlotRange. Now cached.
int CvGame::getStartingPlotRange() const
{
	if (m_iStartingPlotRange <= 0)
		updateStartingPlotRange();
	return m_iStartingPlotRange;
}


void CvGame::normalizeAddRiver()
{
	CvMap const& kMap = GC.getMap();
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		CvPlayer const& kLoopPlayer = *it;
		CvPlot* pStartingPlot = kLoopPlayer.getStartingPlot();
		if (pStartingPlot == NULL)
			continue;
		if (pStartingPlot->isFreshWater() ||
			// <advc.108>
			(m_eNormalizationLevel <= NORMALIZE_LOW &&
			pStartingPlot->isAdjacentFreshWater())) // </advc.108>
		{
			continue;
		}
		// if we will be able to add a lake, then use old river code
		if (normalizeFindLakePlot(kLoopPlayer.getID()) != NULL)
		{
			//CvMapGenerator::GetInstance().doRiver(pStartingPlot);
			/*	K-Mod. If we can have a lake then we don't always need a river.
				Also, the river shouldn't always start on the SE corner of our site. */
			if (//getSorenRandNum(10,"...") < (pStartingPlot->isCoastalLand() ? 5 : 7))
				/*	<advc.108> The above is a 50% lake chance when coastal, 30% otherwise.
					(Will also get a lake when no river possible or a lake already happens
					to be present.) Note that a coastal river normally has only one segment.
					Want to reduce the lake chance and coastal bias a bit. */
				(pStartingPlot->isCoastalLand() ? fixp(0.62) : fixp(0.74)).
				bernoulliSuccess(GC.getGame().getMapRand(), "normalize add river"))
			{	// </advc.108>
				CvPlot* pRiverPlot = pStartingPlot->getInlandCorner();
				if (pRiverPlot != NULL)
					CvMapGenerator::GetInstance().doRiver(pRiverPlot);
			} // K-Mod end.
		} // otherwise, use new river code which is much more likely to succeed
		else CvMapGenerator::GetInstance().addRiver(pStartingPlot);

		// add floodplains to any desert tiles the new river passes through
		for (int i = 0; i < kMap.numPlots(); i++)
		{
			CvPlot& kPlot = kMap.getPlotByIndex(i);
			// advc.108: Can't hurt to randomize the order
			FOR_EACH_ENUM_RAND(Feature, getMapRand())
			{
				if (!GC.getInfo(eLoopFeature).isRequiresRiver() ||
					!kPlot.canHaveFeature(eLoopFeature))
				{
					continue;
				}
				//if (GC.getInfo(eLoopFeature).getAppearanceProbability() == 10000)
				// <advc.108> Cleaner to do the proper dice roll
				if (scaled(GC.getInfo(eLoopFeature).getAppearanceProbability(), 10000).
					bernoulliSuccess(getMapRand(), "normalize add river feature"))
				{	// </advc.108>
					if (kPlot.getBonusType() != NO_BONUS)
						kPlot.setBonusType(NO_BONUS);
					kPlot.setFeatureType(eLoopFeature);
					break;
				}
			}
		}
	}
}


void CvGame::normalizeRemovePeaks()
{
	// <advc.108>
	scaled rRemovalProb = 1;
	if(m_eNormalizationLevel <= NORMALIZE_LOW)
		rRemovalProb = per100(GC.getDefineINT("REMOVAL_CHANCE_PEAK"));
	// </advc.108>

	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		CvPlot* pStartingPlot = itPlayer->getStartingPlot();
		if (pStartingPlot == NULL)
			continue;
		// advc 027 (note): Range corresponds to AIFoundValue::adjustToLandAreaBoundary
		for (SquareIter itPlot(*pStartingPlot, 3); itPlot.hasNext(); ++itPlot)
		{
			if (itPlot->isPeak() &&
				rRemovalProb.bernoulliSuccess(getMapRand(), "advc.108")) // advc.108
			{
				itPlot->setPlotType(PLOT_HILLS);
			}
		}
	}
}


void CvGame::normalizeAddLakes()
{
	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{	// <advc> (Moved out of normalizeFindLakePlot)
		CvPlot* pStartingPlot = itPlayer->getStartingPlot();
		if (pStartingPlot == NULL || pStartingPlot->isFreshWater() || // </advc>
			// <advc.108>
			(m_eNormalizationLevel <= NORMALIZE_LOW &&
			pStartingPlot->isAdjacentFreshWater())) // </advc.108>
		{
			continue; 
		}
		CvPlot* pLakePlot = normalizeFindLakePlot(itPlayer->getID());
		if (pLakePlot != NULL)
			pLakePlot->setPlotType(PLOT_OCEAN);
	}
}

/*	K-Mod: Shuffle the plots - advc.108: Randomize, yes,
	but the inner ring has to take precedence. Rewritten.
	(The K-Mod behavior was intentional though according to a CFC post.) */
CvPlot* CvGame::normalizeFindLakePlot(PlayerTypes ePlayer)
{
	CvPlot const& kStart = *GET_PLAYER(ePlayer).getStartingPlot();
	FOR_EACH_ADJ_PLOT_VAR_RAND(kStart, getMapRand())
	{
		if (normalizeCanAddLakeTo(*pAdj))
			return pAdj;
	}
	if (kStart.isAdjacentFreshWater())
		return NULL;
	for (CityPlotRandIter itPlot(kStart, getMapRand(), false);
		itPlot.hasNext(); ++itPlot)
	{
		if (itPlot.currID() < NUM_INNER_PLOTS)
			continue;
		if (normalizeCanAddLakeTo(*itPlot))
			return &*itPlot;
	}
	return NULL;
}

// advc.108: Cut from normalizeFindLakePlot
bool CvGame::normalizeCanAddLakeTo(CvPlot const& kPlot) const
{
	if (kPlot.isWater() || kPlot.isCoastalLand() ||
		kPlot.isRiver() || kPlot.getBonusType() != NO_BONUS)
	{
		return false;
	}
	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		if (itPlayer->getStartingPlot() == &kPlot)
			return false;
	}
	return true;
}


void CvGame::normalizeRemoveBadFeatures()
{
	// advc.108
	int const iThreshBadFeatPerCity = GC.getDefineINT("THRESH-BAD-FEAT-PER-CITY");

	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		CvPlot* pStartingPlot = itPlayer->getStartingPlot();
		if (pStartingPlot == NULL)
			continue;
		// <advc.108>
		int iBadFeatures = 0;
		for (CityPlotIter itPlot(*pStartingPlot); itPlot.hasNext(); ++itPlot)
		{
			// Disregard inner ring later
			if (itPlot.currID() < NUM_INNER_PLOTS || !itPlot->isFeature())
				continue;
			if(GC.getInfo(itPlot->getFeatureType()).getYieldChange(YIELD_FOOD) <= 0 &&
				GC.getInfo(itPlot->getFeatureType()).getYieldChange(YIELD_PRODUCTION) <= 0)
			{
				iBadFeatures++;
			}
		}
		{
			scaled rRemovalProb;
			if (iBadFeatures > iThreshBadFeatPerCity)
			{
				rRemovalProb = 1 - m_eNormalizationLevel *
						scaled(iThreshBadFeatPerCity, iBadFeatures);
			}
			if (m_eNormalizationLevel >= NORMALIZE_HIGH)
				rRemovalProb = 1;
			// </advc.108>
			for (CityPlotIter itPlot(*pStartingPlot); itPlot.hasNext(); ++itPlot)
			{
				CvPlot& p = *itPlot;
				if (!p.isFeature())
					continue;
				if (GC.getInfo(p.getFeatureType()).getYieldChange(YIELD_FOOD) <= 0 &&
					GC.getInfo(p.getFeatureType()).getYieldChange(YIELD_PRODUCTION) <= 0)
				{
					// <advc.108>
					if (itPlot.currID() < NUM_INNER_PLOTS ||
						(!isPowerfulStartingBonus(p, itPlayer->getID()) &&
						(rRemovalProb.bernoulliSuccess(getMapRand(), "Remove Bad Feature 1") ||
						isWeakStartingFoodBonus(p, itPlayer->getID()))))
					{	// </advc.108>
						p.setFeatureType(NO_FEATURE);
					}
				}
			}
		}
		int const iCityRange = CITY_PLOTS_RADIUS;
		for (PlotCircleIter itPlot(*pStartingPlot, iCityRange + 2);
			itPlot.hasNext(); ++itPlot)
		{
			CvPlot& p = *itPlot;
			int iDistance = itPlot.currPlotDist();
			if (p.isFeature() &&
				GC.getInfo(p.getFeatureType()).getYieldChange(YIELD_FOOD) <= 0 &&
				GC.getInfo(p.getFeatureType()).getYieldChange(YIELD_PRODUCTION) <= 0)
			{
				if (p.isWater())
				{
					if (p.isAdjacentToLand() || (iDistance <= iCityRange + 1 &&
						// advc.027b: was getSorenRandNum
						fixp(0.5).bernoulliSuccess(getMapRand(), "Remove Bad Feature 2")))
					{
						p.setFeatureType(NO_FEATURE);
					}
				}
				else if (iDistance <= iCityRange + 1)
				{
					scaled prRemoval(1, 2);
					if (m_eNormalizationLevel > NORMALIZE_MEDIUM) // advc.108
					{	/*	Smaller pr when there is a resource. I wonder if that's
							really what the BtS programmer meant to do. */
						//getSorenRandNum(2 + (p.getBonusType() == NO_BONUS ? 0 : 2),"...") == 0)
						if (p.getBonusType() != NO_BONUS)
							prRemoval = scaled(1, 4);
					}
					// <advc.108>
					else if (p.getBonusType() == NO_BONUS)
						prRemoval = scaled(1, 3); // </advc.108>
					// advc.027b: was getSorenRandNum
					if (prRemoval.bernoulliSuccess(getMapRand(), "Remove Bad Feature 3"))
						p.setFeatureType(NO_FEATURE);
				}
			}
		}
	}
}


void CvGame::normalizeRemoveBadTerrain()
{
	// <advc.108>
	scaled rKeepProb;
	if(m_eNormalizationLevel <= NORMALIZE_LOW)
		rKeepProb = 1 - per100(GC.getDefineINT("REMOVAL_CHANCE_BAD_TERRAIN"));
	// </advc.108>
	int const iCityRange = CITY_PLOTS_RADIUS;
	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		CvPlot* pStartingPlot = itPlayer->getStartingPlot();
		if (pStartingPlot == NULL)
			continue;
		for (PlotCircleIter itPlot(*pStartingPlot, iCityRange + 1);
			itPlot.hasNext(); ++itPlot)
		{
			CvPlot& p = *itPlot;
			int iDistance = itPlot.currPlotDist();
			if (!p.isWater() && (iDistance <= iCityRange ||
				p.isCoastalLand() || scaled(1, 1 + iDistance - iCityRange).
				bernoulliSuccess(getMapRand(), "Map Upgrade Terrain Food 1")))
			{
				CvTerrainInfo const& kTerrain = GC.getInfo(p.getTerrainType());
				int iPlotFood = kTerrain.getYield(YIELD_FOOD);
				int iPlotProduction = kTerrain.getYield(YIELD_PRODUCTION);
				if (iPlotFood + iPlotProduction > 1)
					continue;
				// <advc.108>
				if (isPowerfulStartingBonus(p, itPlayer->getID()))
					continue;
				/*  I think the BtS code ends up replacing Desert with Desert when
					there's a feature, but let's rather handle Desert features explicitly. */
				if (p.isFeature() &&
					GC.getInfo(p.getFeatureType()).
					getYieldChange(YIELD_FOOD) + iPlotFood >= 2)
				{
					continue;
				}
				if (rKeepProb.bernoulliSuccess(getMapRand(), "Map Upgrade Terrain Food 2"))
				{
					if (iPlotFood > 0 ||
					/*  advc.129b: Two chances of removal for Snow river
						(BuildModifier=50), but not for Desert river. */
						(p.isRiver() && kTerrain.getBuildModifier() < 30) ||
						rKeepProb.bernoulliSuccess(getMapRand(), "Map Upgrade Terrain Food 3"))
					{
						if (!isWeakStartingFoodBonus(p, itPlayer->getID()))
							continue;
					}
				} // </advc.108>
				int const iTargetTotal = 2;
				int iTargetFood = 1;
				if (p.getBonusType(itPlayer->getTeam()) != NO_BONUS)
					iTargetFood = 1;
				else if (iPlotFood == 1 || iDistance <= iCityRange)
				{	// advc.027b: was getSorenRandNum
					iTargetFood = 1 + getMapRandNum(2, "Map Upgrade Terrain Food 4");
				}
				else iTargetFood = (p.isCoastalLand() ? 2 : 1);

				FOR_EACH_ENUM(Terrain)
				{
					CvTerrainInfo const& kRepl = GC.getInfo(eLoopTerrain);
					if (kRepl.isWater())
						continue;
					if (kRepl.getYield(YIELD_FOOD) >= iTargetFood &&
						kRepl.getYield(YIELD_FOOD) +
						kRepl.getYield(YIELD_PRODUCTION) == iTargetTotal)
					{
						if (!p.isFeature() ||
							GC.getInfo(p.getFeatureType()).isTerrain(eLoopTerrain))
						{
							p.setTerrainType(eLoopTerrain);
						}
					}
				}
			}
		}
	}
}


void CvGame::normalizeAddFoodBonuses(/* advc.027: */ NormalizationTarget const* pTarget)
{
	bool const bIgnoreLatitude = GC.getPythonCaller()->isBonusIgnoreLatitude();
	int const iFoodPerPop = GC.getFOOD_CONSUMPTION_PER_POPULATION(); // K-Mod

	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		CvPlayer const& kPlayer = *itPlayer;
		CvPlot const* pStartingPlot = kPlayer.getStartingPlot();
		if (pStartingPlot == NULL)
			continue;

		int iFoodBonus = 0;
		int iGoodNatureTileCount = 0;
		// K-Mod. Don't count the city plot.
		for (CityPlotIter itPlot(*pStartingPlot, false); itPlot.hasNext(); ++itPlot)
		{
			CvPlot& p = *itPlot;
			BonusTypes eBonus = p.getBonusType(kPlayer.getTeam());
			if (eBonus == NO_BONUS)
			{
				if (p.calculateBestNatureYield(YIELD_FOOD, kPlayer.getTeam()) >=
					iFoodPerPop + 1)
				{
					iGoodNatureTileCount++;
				}
				continue;
			}
			CvBonusInfo const& kBonus = GC.getInfo(eBonus);
			if (kBonus.getYieldChange(YIELD_FOOD) <= 0)
			{
				if (p.calculateBestNatureYield(YIELD_FOOD, kPlayer.getTeam()) >=
					iFoodPerPop)
				{
					iGoodNatureTileCount++;
				}
				continue;
			}
			if (kBonus.getTechCityTrade() != NO_TECH &&
				GC.getInfo(kBonus.getTechCityTrade()).getEra() > getStartEra())
			{
				continue;
			}
			// <advc.001> Whale
			TechTypes eTechImprove = kBonus.getTechImprove(p.isWater());
			if (eTechImprove != NO_TECH &&
				GC.getInfo(eTechImprove).getEra() > getStartEra())
			{
				continue;
			} // </advc.001>
			if (p.isWater())
			{
				iFoodBonus += 2;
				// <advc.108>
				if (p.isAdjacentToLand())
					iFoodBonus++; // </advc.108>
				continue;
			}
			//iFoodBonus += 3;
			/*	K-Mod. Bonus which only give 3 food with their improvement
				should not be worth 3 points.
				(ie. plains-cow should not be the only food resource.) */
			/* first attempt - this doesn't work, because "max yield"
				essentially means +2 food on any plot. That isn't what we want. */
			/*if (p.calculateMaxYield(YIELD_FOOD) >= 2*iFoodPerPop)
				iFoodBonus += 3;
			else iFoodBonus += 2;*/
			int const iHighFoodThreshold = 2 * iFoodPerPop;
			int const iNaturalFood = p.calculateBestNatureYield(YIELD_FOOD, kPlayer.getTeam());
			// (+1 just as a shortcut to save time for obvious cases.)
			bool bHighFood = iNaturalFood + 1 >= iHighFoodThreshold;
			for (ImprovementTypes eImp = (ImprovementTypes)0;
				!bHighFood && eImp < GC.getNumImprovementInfos();
				eImp=(ImprovementTypes)(eImp+1))
			{
				if (GC.getInfo(eImp).isImprovementBonusTrade(eBonus))
				{
					bHighFood = (iNaturalFood + p.calculateImprovementYieldChange(
							eImp, YIELD_FOOD, kPlayer.getID(), false, false) >=
							iHighFoodThreshold);
				}
			}
			iFoodBonus += (bHighFood ? 3 : 2);
			// K-Mod end
		}

		int iTargetFoodBonusCount = 3;
		// <advc.027>
		bool bTargetReached = (pTarget != NULL &&
				pTarget->isReached(*pStartingPlot)); // </advc.027>
		iTargetFoodBonusCount += std::max(0, 2 - iGoodNatureTileCount); // K-Mod
		// <advc.108> Randomize order of traversal but (much) prefer the inner ring
		for (int iOuterPass = 0; iOuterPass < 2; iOuterPass++)
		{
			bool const bInnerRingBias = (iOuterPass == 0);
			for (CityPlotRandIter itPlot(*pStartingPlot, getMapRand(), false);
				itPlot.hasNext() && // </advc.108>
			/*	K-Mod. I've rearranged a couple of things to make it a bit
				more efficient and easier to read. */
				iFoodBonus < iTargetFoodBonusCount - /* advc.027: */ (bTargetReached ? 1 : 0);
				++itPlot)
			{	// <advc.108>
				if (bInnerRingBias && itPlot.currID() >= NUM_INNER_PLOTS &&
					fixp(0.63).bernoulliSuccess(getMapRand(), "inner ring bias food"))
				{
					continue;
				} // </advc.108>
				CvPlot& p = *itPlot;
				if (p.getBonusType() != NO_BONUS || /* advc.004z: */ p.isGoody() ||
					// advc.108 (from PerfectWorld 2)
					(!p.sameArea(*pStartingPlot) && !p.isWater()))
				{
					continue;
				}
				// <adcv.108>
				for (int iPass = 0; iPass < 2; iPass++)
				{
					bool const bInitialPass = (iPass == 0); // </advc.108>
					// advc.129: Randomize the order in which resources are considered
					FOR_EACH_ENUM_RAND(Bonus, getMapRand())
					{
						if (GC.getInfo(eLoopBonus).getYieldChange(YIELD_FOOD) <= 0)
							continue;
						/*	advc.108: Let map generator check canPlaceBonusAt in the
							initial pass (same as in normalizeAddExtras) */
						if (!isNormalizationBonus(eLoopBonus, kPlayer.getID(), p,
							bInitialPass, !bInitialPass || bIgnoreLatitude))
						{
							continue;
						}
						// <advc.108>
						// Don't place the food resource on a bad feature
						FeatureTypes const eFeature = p.getFeatureType();
						bool bValid = true;
						if(eFeature != NO_FEATURE)
						{
							CvFeatureInfo const& kFeature = GC.getInfo(eFeature);
							bValid = false;
							if(m_eNormalizationLevel >= NORMALIZE_HIGH ||
								kFeature.getYieldChange(YIELD_FOOD) > 0 ||
								kFeature.getYieldChange(YIELD_PRODUCTION) > 0)
							{
								bValid = true;
							}
						}
						if(!bValid)
							continue;
						if (bInitialPass &&
							skipDuplicateNormalizationBonus(*pStartingPlot, p, eLoopBonus))
						{	// </advc.108>
							continue;
						}
						p.setBonusType(eLoopBonus);
						if (gMapLogLevel > 0) logBBAI("    Adding food bonus %S for player %d", GC.getInfo(eLoopBonus).getDescription(), itPlayer->getID()); // advc
						if (p.isWater())
							iFoodBonus += 2;
						else
						{
							//iFoodBonus += 3;
							// K-Mod
							int const iNaturalFood = p.calculateBestNatureYield(
									YIELD_FOOD, kPlayer.getTeam());
							int const iHighFoodThreshold = 2 * iFoodPerPop;
							// (+1 just as a shortcut to save time for obvious cases.)
							bool bHighFood = (iNaturalFood + 1 >= iHighFoodThreshold);
							FOR_EACH_ENUM(Improvement)
							{
								if (GC.getInfo(eLoopImprovement).
									isImprovementBonusTrade(eLoopBonus))
								{
									bHighFood = (iNaturalFood +
											p.calculateImprovementYieldChange(eLoopImprovement,
											YIELD_FOOD, kPlayer.getID(), false, false) >=
											iHighFoodThreshold);
								}
							}
							iFoodBonus += (bHighFood ? 3 : 2);
						} // K-Mod end
						// advc.027:
						bTargetReached = (pTarget != NULL && pTarget->isReached(*pStartingPlot));
						break;
					}  // <advc.108> Don't do 2nd pass if 1st pass has succeeded
					if (p.getBonusType() != NO_BONUS)
						break; // </advc.108>
				}
			}
		}
	}
}


void CvGame::normalizeAddGoodTerrain()
{
	// <advc.108>
	if(m_eNormalizationLevel <= NORMALIZE_LOW)
		return; // </advc.108>

	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		CvPlayer const& kPlayer = *itPlayer;
		CvPlot* pStartingPlot = kPlayer.getStartingPlot();
		if (pStartingPlot == NULL)
			continue;

		int iGoodPlot = 0;
		for (CityPlotIter itPlot(*pStartingPlot, false); itPlot.hasNext(); ++itPlot)
		{
			if ((itPlot->calculateNatureYield(YIELD_FOOD, kPlayer.getTeam()) >=
				GC.getFOOD_CONSUMPTION_PER_POPULATION()) &&
				(itPlot->calculateNatureYield(YIELD_PRODUCTION, kPlayer.getTeam()) > 0))
			{
				iGoodPlot++;
			}
		}
		for (CityPlotIter itPlot(*pStartingPlot, false); itPlot.hasNext() &&
			iGoodPlot < 4; ++itPlot)
		{
			CvPlot& kPlot = *itPlot;
			if (kPlot.isWater() || kPlot.isHills() || kPlot.getBonusType() != NO_BONUS)
				continue;

			bool bChanged = false;
			if (kPlot.calculateNatureYield(YIELD_FOOD, kPlayer.getTeam()) <
				GC.getFOOD_CONSUMPTION_PER_POPULATION())
			{
				FOR_EACH_ENUM(Terrain)
				{
					CvTerrainInfo const& kLoopTerrain = GC.getInfo(eLoopTerrain);
					if (!kLoopTerrain.isWater() && kLoopTerrain.getYield(YIELD_FOOD) >=
						GC.getFOOD_CONSUMPTION_PER_POPULATION())
					{
						kPlot.setTerrainType(eLoopTerrain);
						bChanged = true;
						break;
					}
				}
			}
			if (kPlot.calculateNatureYield(YIELD_PRODUCTION, kPlayer.getTeam()) == 0)
			{
				FOR_EACH_ENUM(Feature)
				{
					CvFeatureInfo const& kLoopFeature = GC.getInfo(eLoopFeature);
					if (kLoopFeature.getYieldChange(YIELD_FOOD) >= 0 &&
						kLoopFeature.getYieldChange(YIELD_PRODUCTION) > 0 &&
						kLoopFeature.isTerrain(kPlot.getTerrainType()))
					{
						kPlot.setFeatureType(eLoopFeature);
						bChanged = true;
						break;
					}
				}
			}
			if (bChanged)
				iGoodPlot++;
		}
	}
}


void CvGame::normalizeAddExtras(/* advc.027: */ NormalizationTarget const* pTarget)
{
	bool const bIgnoreLatitude = GC.getPythonCaller()->isBonusIgnoreLatitude();

	/*	advc.108: Moved up so that the code dependent on found value
		already takes the extra hills into account */
	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		CvPlayerAI const& kPlayer = *itPlayer;
		CvPlot* pStartingPlot = kPlayer.getStartingPlot();
		if (pStartingPlot == NULL)
			continue;
		int iHills = 0;
		for (CityPlotIter itPlot(*pStartingPlot); itPlot.hasNext(); ++itPlot)
		{
			if (itPlot->isHills())
				iHills++;
		}
		int iHillsAdded = 0; // advc.108
		// advc (comment): Starting plot not excluded. I guess that's OK.
		for (CityPlotRandIter it(*pStartingPlot, getMapRand(), true);
			iHills < 3 && /* advc.108: */ iHillsAdded < 2 &&
			it.hasNext(); ++it)
		{
			CvPlot& p = *it;
			if (p.isWater() || p.isHills() ||
				!p.sameArea(*pStartingPlot)) // advc.108 (from Perfect World 2)
			{
				continue;
			}
			if (!p.isFeature() ||
				!GC.getInfo(p.getFeatureType()).isRequiresFlatlands())
			{
				if (p.getBonusType() == NO_BONUS ||
					GC.getInfo(p.getBonusType()).isHills())
				{
					if (gMapLogLevel > 0) logBBAI("    Adding hills for player %d.", kPlayer.getID()); // K-Mod
					p.setPlotType(PLOT_HILLS, false, true);
					iHills++;
					// <advc.108>
					if (it.currID() != CITY_HOME_PLOT)
						iHillsAdded++; // </advc.108>
				}
			}
		}
	}

	scaled rTargetValue;
	if (pTarget == NULL) // advc.027
	{
		int iTotalValue = 0;
		int iBestValue = 0;
		int iWorstValue = MAX_INT;
		for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
		{
			CvPlayerAI const& kPlayer = *itPlayer;
			CvPlot const* pStartingPlot = kPlayer.getStartingPlot();
			if (pStartingPlot == NULL)
				continue;
			int iValue = kPlayer.AI_foundValue(pStartingPlot->getX(), pStartingPlot->getY(),
					-1, /* advc.031e: */ false, true);
			iTotalValue += iValue;
			iBestValue = std::max(iValue, iBestValue);
			iWorstValue = std::min(iValue, iWorstValue);
		}
		//iTargetValue = (iTotalValue + iBestValue) / (itPlayer.nextIndex() + 1);
		rTargetValue = fixp(0.8) * iBestValue;
		// <advc.108>
		if(m_eNormalizationLevel <= NORMALIZE_LOW)
			rTargetValue = fixp(0.75) * iBestValue; // </advc.108>
		logBBAI("Adding extras to normalize starting positions. (target value: %d)", rTargetValue.round()); // K-Mod
	}

	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		CvPlayerAI const& kPlayer = *itPlayer;
		CvPlot* pStartingPlot = kPlayer.getStartingPlot();
		if (pStartingPlot == NULL)
			continue;

		gDLL->callUpdater(); // allow window to update during launch
		CitySiteEvaluator citySiteEval(kPlayer, -1, false, true);
		// <advc.031c>
		if (gFoundLogLevel > 0 && pTarget == NULL)
			citySiteEval.log(*pStartingPlot);
		// </advc.031c>
		// <advc.108> Treat desert features and forest separately
		int iFoodFeatures = 0;
		int iProductionFeatures = 0;
		for (CityPlotIter itPlot(*pStartingPlot, false); itPlot.hasNext(); ++itPlot)
		{
			if (itPlot->isFeature())
			{
				CvFeatureInfo const& kFeature = GC.getFeatureInfo(itPlot->getFeatureType());
				if (kFeature.getYieldChange(YIELD_FOOD) > 0)
					iFoodFeatures++;
				if (kFeature.getYieldChange(YIELD_PRODUCTION) > 0)
					iProductionFeatures++;
			}
		}
		bool bProductionFeatureDone = false;
		bool bFoodFeatureDone = false; // </advc.108>
		{
			for (CityPlotRandIter itPlot(*pStartingPlot, getMapRand(), false);
				itPlot.hasNext(); ++itPlot)
			{
				CvPlot& kLoopPlot = *itPlot;
				if (kLoopPlot.getBonusType() != NO_BONUS || kLoopPlot.isFeature())
					continue;
				/*if (getSRandNum(iCount + 2, "Setting Feature Type") > 1)
					continue;*/ // advc.108: Replaced below
				/*	advc.129: Randomize - for mod-mods (for forest, oasis, flood plains
					the order doesn't matter b/c they have mutually exclusive prereqs) */
				FOR_EACH_ENUM_RAND(Feature, getMapRand())
				{
					if (!kLoopPlot.canHaveFeature(eLoopFeature))
						continue;
					// <advc.108> (Partly duplicated in the second extra feature loop)
					CvFeatureInfo const& kFeature = GC.getFeatureInfo(eLoopFeature);
					bool bFood = (kFeature.getYieldChange(YIELD_FOOD) > 0);
					bool bProduction = (kFeature.getYieldChange(YIELD_PRODUCTION) > 0);
					if ((!bFood && !bProduction) ||
						(bFood && (iFoodFeatures >= 4 || bFoodFeatureDone)) ||
						bProduction && (iProductionFeatures >= 6 || bProductionFeatureDone))
					{
						continue;
					} // </advc.108>
					// advc.opt: Moved down (do all other checks first)
					if (pTarget != NULL ? pTarget->isReached(*pStartingPlot) : // advc.027
						citySiteEval.evaluate(*pStartingPlot) >= rTargetValue)
					{
						if (gMapLogLevel > 0) logBBAI("    Player %d doesn't need any more features.", kPlayer.getID()); // K-Mod
						goto next_player; // advc
					}
					if (gMapLogLevel > 0) logBBAI("    Adding %S for player %d.", GC.getInfo(eLoopFeature).getDescription(), kPlayer.getID()); // K-Mod
					kLoopPlot.setFeatureType(eLoopFeature);
					// <advc.108>
					if (bFood)
					{
						iFoodFeatures++;
						// Add at most 1 food feature at first
						bFoodFeatureDone = true;;
					}
					else
					{
						iProductionFeatures++;
						// Replacing a BtS clause higher up
						if (!scaled(1, std::max(iProductionFeatures - 2, 1)).
							bernoulliSuccess(getSRand(), "Place Production Feature (1)"))
						{
							bProductionFeatureDone = true;
						}
					}
					break;
				}
			}
			// For 2nd extra feature loop
			bProductionFeatureDone = false;
			bFoodFeatureDone = false; // </advc.108>
		}

		int iCoastBonus = 0;
		int iOceanBonus = 0;
		int iLandBonus = 0;
		int iLandFood = 0; // advc.108
		int iWater = 0;
		for (CityPlotIter itPlot(*pStartingPlot, false); itPlot.hasNext(); ++itPlot)
		{
			CvPlot const& p = *itPlot;
			BonusTypes eLoopBonus = p.getBonusType(
					// <advc.108> Don't count unrevealed bonuses
					m_eNormalizationLevel > NORMALIZE_LOW ? NO_TEAM :
					kPlayer.getTeam()); /* </advc.108> */
			if (p.isWater())
			{
				iWater++;
				if (eLoopBonus != NO_BONUS)
				{
					if (p.isAdjacentToLand())
						iCoastBonus++;
					else iOceanBonus++;
				}
			}
			else if (eLoopBonus != NO_BONUS)
			{
				iLandBonus++;
				// <advc.108>
				if (GC.getInfo(eLoopBonus).getYieldChange(YIELD_FOOD) > 0)
					iLandFood++; // </advc.108>
			}
		}
		// <advc.108>
		for (int iOuterPass = 0; iOuterPass < 2; iOuterPass++)
		{
			bool const bInnerRingBias = (iOuterPass == 0); // </advc.108>
			bool const bLandBias = (iWater > NUM_CITY_PLOTS / 2);
			for (CityPlotRandIter itPlot(*pStartingPlot, getMapRand(), false);
				itPlot.hasNext() &&
				//iLandBonus * 3 +
				iLandFood * 4 + (iLandBonus - iLandFood) * 3 + // advc.108
				iOceanBonus * 2 + iCoastBonus * 3 < 12; ++itPlot) // advc.108: iCoastBonus multiplier was 2
			{	// <advc.108>
				if (bInnerRingBias && itPlot.currID() >= NUM_INNER_PLOTS &&
					fixp(0.47).bernoulliSuccess(getMapRand(), "inner ring bias extras"))
				{
					continue;
				} // </advc.108>
				CvPlot& p = *itPlot;
				//if (getSorenRandNum(bLandBias && p.isWater() ? 2 : 1,"...") == 0) {
				// advc.027b: (note that getSorenRandNum(1) is always 0)
				if (bLandBias && p.isWater() && fixp(0.5).bernoulliSuccess(getMapRand(), "Placing Bonuses"))
					continue;
				if (pTarget != NULL ? pTarget->isNearlyReached(*pStartingPlot) : // advc.027
					citySiteEval.evaluate(*pStartingPlot) >= rTargetValue)
				{
					if (gMapLogLevel > 0) logBBAI("    Player %d doesn't need any more bonuses.", kPlayer.getID()); // K-Mod
					goto done_placing_resources;
				}
				bool const bCoast = (p.isWater() && p.isAdjacentToLand());
				bool const bOcean = (p.isWater() && !bCoast);
				if (!(bCoast && iCoastBonus >= 2) && // advc.108: was >2
					!(bOcean && iOceanBonus >= 2) &&// advc.108: was >2
					// advc.108: At most 3 sea food
					!((bOcean || bCoast) && iOceanBonus + iCoastBonus >= 3))
				{
					for (int iPass = 0; iPass < 2; iPass++)
					{
						if (p.getBonusType() != NO_BONUS)
							continue;
						int iFoodBonuses = iLandFood + iCoastBonus + iOceanBonus; // advc.108
						// advc: Selection and placement moved into auxiliary function
						if (placeExtraBonus(kPlayer.getID(), p,
							iPass == 0, bIgnoreLatitude, false,
							iFoodBonuses > 2 - iPass)) // advc.108
						{
							if (p.isWater())
							{
								if (bCoast)
									iCoastBonus++;
								else if (bOcean)
									iOceanBonus++;
							}
							else
							{
								iLandBonus++;
								// <advc.108>
								if (GC.getInfo(p.getBonusType()).
									getYieldChange(YIELD_FOOD) > 0)
								{
									iLandFood++;
								} // </advc.108>
							}
							break;
						}
						if (!bLandBias || p.isWater() ||
							p.getBonusType() != NO_BONUS)
						{
							continue;
						}
						if (p.isFeature() &&
							//iFeatureCount > 4 &&
							// <advc.108> Don't clear food features
							GC.getFeatureInfo(p.getFeatureType()).
							  getYieldChange(YIELD_FOOD) <= 0 &&
							(GC.getFeatureInfo(p.getFeatureType()).
							  getYieldChange(YIELD_PRODUCTION) <= 0 ||
							// Don't clear production features if they're scarce
							iProductionFeatures >= 4) && // </advc.108>
							iCoastBonus + iOceanBonus > 2 &&
							fixp(0.5).bernoulliSuccess(getMapRand(), "Clear feature to add bonus"))
						{
							// advc: Selection, clearing of feature and placement moved into auxiliary function.
							if (placeExtraBonus(kPlayer.getID(), p, iPass == 0,
								bIgnoreLatitude, true, iFoodBonuses >= 2))
							{
								iLandBonus++;
								// <advc.108>
								if (GC.getInfo(p.getBonusType()).getYieldChange(YIELD_FOOD) > 0)
									iLandFood++; // </advc.108>
							}
						}
					}
				}
			}
		} done_placing_resources:
		for (CityPlotRandIter itPlot(*pStartingPlot, getMapRand(), false);
			itPlot.hasNext(); ++itPlot)
		{
			CvPlot& p = *itPlot;
			if (p.getBonusType() != NO_BONUS || p.isFeature())
				continue;
			// advc.opt: Moved down
			if (pTarget != NULL ? pTarget->isReached(*pStartingPlot) : // advc.027
				citySiteEval.evaluate(*pStartingPlot) >= rTargetValue)
			{
				if (gMapLogLevel > 0) logBBAI("    Player %d doesn't need any more features (2).", kPlayer.getID()); // K-Mod
				break;
			}
			FOR_EACH_ENUM_RAND(Feature, getMapRand()) // advc.129: randomize
			{
				if (!p.canHaveFeature(eLoopFeature))
					continue;
				// <advc.108> (Similar to the first place-feature loop)
				CvFeatureInfo const& kFeature = GC.getFeatureInfo(eLoopFeature);
				bool bFood = (kFeature.getYieldChange(YIELD_FOOD) > 0);
				bool bProduction = (kFeature.getYieldChange(YIELD_PRODUCTION) > 0);
				if ((!bFood && !bProduction) ||
					(bFood && (iFoodFeatures >= 4 || bFoodFeatureDone)) ||
					(bProduction && bProductionFeatureDone))
				{
					continue;
				}
				// Too many river forests are unhelpful; they block improvements.
				if (p.isRiver() &&
					GC.getInfo(eLoopFeature).getRiverYieldChange(YIELD_COMMERCE) <
					GC.getInfo(p.getTerrainType()).getRiverYieldChange(YIELD_COMMERCE))
				{
					continue;
				}
				/*	BtS had placed features everywhere (unless found value became
					high enough - unlikely to happen) */
				if (bFood)
				{
					iFoodFeatures++;
					if (!scaled(1, std::max(iFoodFeatures, 1)).
						bernoulliSuccess(getSRand(), "Place Food Feature"))
					{
						bFoodFeatureDone = true;
					}
				}
				else
				{
					iProductionFeatures++;
					if (!scaled(1, std::max(iProductionFeatures - 2, 1)).
						bernoulliSuccess(getSRand(), "Place Production Feature (2)"))
					{
						bProductionFeatureDone = true;
					}
				} // </advc.108>
				if (gMapLogLevel > 0) logBBAI("    Adding %S for player %d.", GC.getInfo(eLoopFeature).getDescription(), kPlayer.getID()); // K-Mod
				p.setFeatureType(eLoopFeature);
				break;
			}
		}
		next_player: continue; // advc
	}
}


void CvGame::normalizeStartingPlots(NormalizationTarget const* pTarget)
{
	PROFILE_FUNC();

	CvPythonCaller const& py = *GC.getPythonCaller(); // advc.003y

	// advc.108b: Team starts now reassigned already in assignStartingPlots

	if (GC.getInitCore().getWBMapScript())
		return;

	if (!py.callMapFunction("normalizeAddRiver"))
		normalizeAddRiver();

	if (!py.callMapFunction("normalizeRemovePeaks"))
		normalizeRemovePeaks();

	if (!py.callMapFunction("normalizeAddLakes"))
		normalizeAddLakes();

	if (!py.callMapFunction("normalizeRemoveBadFeatures"))
		normalizeRemoveBadFeatures();

	if (!py.callMapFunction("normalizeRemoveBadTerrain"))
		normalizeRemoveBadTerrain();

	if (!py.callMapFunction("normalizeAddFoodBonuses"))
		normalizeAddFoodBonuses(/* advc.027: */ pTarget);

	if (!py.callMapFunction("normalizeAddGoodTerrain"))
		normalizeAddGoodTerrain();

	if (!py.callMapFunction("normalizeAddExtras"))
		normalizeAddExtras(/* advc.027: */ pTarget);
	// <advc> K-Mod logging code moved out of normalizeAddExtras
	if (gMapLogLevel > 0)
	{
		for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
		{
			if (it->getStartingPlot() != NULL)
				logBBAI("    Player %d final value: %d", it->getID(), it->AI_foundValue(it->getStartingPlot()->getX(), it->getStartingPlot()->getY(), -1, false, true));
		}
		logBBAI("normalizeAddExtras() complete");
	} // </advc>
}

/*  advc.opt: Body cut from CvPlayer::startingPlotRange. Not player-dependent,
	and there's no need to recompute it for every prospective starting plot.
	const b/c it only updates a mutable cache. */
void CvGame::updateStartingPlotRange() const
{
	CvMap const& kMap = GC.getMap();
	int iRange = kMap.maxStepDistance() + 10;
	iRange *= GC.getDefineINT("STARTING_DISTANCE_PERCENT");
	iRange /= 100;
	int const iAlive = countCivPlayersAlive();
	int const iLand = kMap.getLandPlots();
	iRange *= iLand / (std::max(GC.getInfo(kMap.getWorldSize()).
			getTargetNumCities(), 1) * iAlive);
	iRange /= NUM_CITY_PLOTS;
	// <advc.031> Replacing kMap.getNumAreas(). Tiny islands shouldn't matter.
	int iMajorAreas = 0;
	FOR_EACH_AREA(pArea)
	{
		if (pArea->getNumTiles() * iAlive > iLand)
			iMajorAreas++;
	}
	if (iMajorAreas == 0)
		iMajorAreas = iAlive;
	// </advc.031>
	iRange += std::min((iMajorAreas + 1) / 2, iAlive);
	iRange *= 100 + GC.getPythonCaller()->minStartingDistanceMod();
	iRange /= 100;
	m_iStartingPlotRange = std::max(iRange, GC.getDefineINT("MIN_CIV_STARTING_DISTANCE"));
}

// advc: Cut, refactored from normalizeAddExtras
bool CvGame::placeExtraBonus(PlayerTypes eStartPlayer, CvPlot& kPlot,
		bool bCheckCanPlace, bool bIgnoreLatitude, bool bRemoveFeature,
		bool bNoFood) // advc.108
{
	CvPlot const& kStartPlot = *GET_PLAYER(eStartPlayer).getStartingPlot();
	// <advc.108>
	if (bCheckCanPlace &&
		((!kPlot.sameArea(kStartPlot) && !kPlot.isWater()) || // </advc.108>
		kPlot.isGoody())) // advc.004z
	{
		return false;
	}
	if (bRemoveFeature && kPlot.isFeature())
	{
		if (gMapLogLevel > 0) logBBAI("    Removing %S to place bonus for player %d", GC.getInfo(kPlot.getFeatureType()).getDescription(), eStartPlayer); // K-Mod
		kPlot.setFeatureType(NO_FEATURE);
	}
	// advc.129: Try the resources in a random order
	FOR_EACH_ENUM_RAND(Bonus, getMapRand())
	{
		CvBonusInfo const& kLoopBonus = GC.getInfo(eLoopBonus);
		if (bNoFood && kLoopBonus.getYieldChange(YIELD_FOOD) > 0 || // advc.108
			kPlot.getBonusType() != NO_BONUS)
		{
			continue;
		}
		if (!isNormalizationBonus(eLoopBonus, eStartPlayer, kPlot, bCheckCanPlace, bIgnoreLatitude) ||
			skipDuplicateNormalizationBonus(kStartPlot, kPlot, eLoopBonus, !bCheckCanPlace)) // advc.108
		{
			continue;
		}
		// <advc.004z>
		if (kPlot.isGoody())
			kPlot.setImprovementType(NO_IMPROVEMENT); // </advc.004z>
		if (gMapLogLevel > 0) logBBAI("    Adding %S for player %d", kLoopBonus.getDescription(), eStartPlayer); // K-Mod
		kPlot.setBonusType(eLoopBonus);
		return true;
	}
	return false;
}

/*	advc.108: May probabilistically return false when there is already a resource
	of type eBonus near kStartPlot */
bool CvGame::skipDuplicateNormalizationBonus(CvPlot const& kStartPlot, CvPlot const& kPlot,
	BonusTypes eBonus, bool bSecondPass)
{
	scaled rSkipPr = fixp(1/3.);
	CvBonusInfo const& kBonus = GC.getInfo(eBonus);
	if (kBonus.getGroupRange() <= 0)
		rSkipPr *= 2;
	if (bSecondPass)
		rSkipPr /= 2;
	for (CityPlotIter it(kStartPlot); it.hasNext(); ++it)
	{
		if (it->getBonusType() != eBonus)
			continue;
		// Adjacent duplicates look especially ugly
		int iDist = stepDistance(&*it, &kPlot);
		if ((2 * rSkipPr / iDist).bernoulliSuccess(
			getMapRand(), "Avoid double extra bonus"))
		{
			return true;
		}
	}
	return false;
}

// advc: Cut, pasted, refactored from normalizeAddExtras
bool CvGame::isNormalizationBonus(BonusTypes eBonus, PlayerTypes eStartPlayer,
	CvPlot const& kPlot, bool bCheckCanPlace, bool bIgnoreLatitude) const
{
	CvBonusInfo const& kBonus = GC.getInfo(eBonus);
	if (!kBonus.isNormalize())
		return false;

	if (kBonus.getYieldChange(YIELD_FOOD) < 0 ||
		kBonus.getYieldChange(YIELD_PRODUCTION) < 0)
	{
		return false;
	}
	if (kBonus.getTechCityTrade() != NO_TECH &&
		GC.getInfo(kBonus.getTechCityTrade()).getEra() > getStartEra())
	{
		return false;
	}
	/*  advc: BtS had checked this only for seafood; doesn't really matter though
		b/c all of the isNormalize resources are revealed from the start. */
	if (!GET_TEAM(eStartPlayer).isHasTech((TechTypes)kBonus.getTechReveal()))
		return false;
	if (kPlot.getBonusType() == eBonus)
	{
		FAssert(!bCheckCanPlace);
		return true;
	}
	return (bCheckCanPlace ? CvMapGenerator::GetInstance().
			canPlaceBonusAt(eBonus, kPlot.getX(), kPlot.getY(), bIgnoreLatitude) :
			kPlot.canHaveBonus(eBonus, bIgnoreLatitude));
}

// <advc.108>
bool CvGame::isPowerfulStartingBonus(CvPlot const& kPlot, PlayerTypes eStartPlayer) const
{
	if(getStartEra() > 0)
		return false;
	BonusTypes eBonus = kPlot.getBonusType(TEAMID(eStartPlayer));
	if(eBonus == NO_BONUS)
		return false;
	return (GC.getInfo(eBonus).getBonusClassType() ==
			GC.getInfoTypeForString("BONUSCLASS_PRECIOUS"));
}

// Tailored for Tundra Deer, dry Jungle Rice
bool CvGame::isWeakStartingFoodBonus(CvPlot const& kPlot, PlayerTypes eStartPlayer) const
{
	BonusTypes eBonus = kPlot.getBonusType(TEAMID(eStartPlayer));
	if (eBonus == NO_BONUS ||
		// To filter out resources that normalizeAddFood doesn't care about
		!isNormalizationBonus(eBonus, eStartPlayer, kPlot, false, true))
	{
		return false;
	}
	int iBaseFood = GC.getInfo(eBonus).getYieldChange(YIELD_FOOD);
	if (iBaseFood <= 0)
		return false;
	 iBaseFood += GC.getInfo(kPlot.getTerrainType()).getYield(YIELD_FOOD);
	// (Some overlap with AIFoundValue::getBonusImprovement)
	int iBestImprovFood = 0;
	FOR_EACH_ENUM(Build)
	{
		CvBuildInfo const& kBuild = GC.getInfo(eLoopBuild);
		ImprovementTypes eImprov = kBuild.getImprovement();
		if (eImprov == NO_IMPROVEMENT)
			continue;
		CvImprovementInfo const& kImprov = GC.getInfo(eImprov);
		if (eImprov == NO_IMPROVEMENT ||
			!kImprov.isImprovementBonusMakesValid(eBonus) ||
			!kImprov.isImprovementBonusTrade(eBonus))
		{
			continue;
		}
		int iImprovFood = kPlot.calculateImprovementYieldChange(
				eImprov, YIELD_FOOD, eStartPlayer);
		iBestImprovFood = std::max(iBestImprovFood, iImprovFood);
	}
	return (iBaseFood + iBestImprovFood <= 4 &&
			// Not really a food resource if the improvement doesn't add food
			iBestImprovFood > 0);
} // </advc.108>


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
		if(!isInBetweenTurns())
		{
			CvEventReporter::getInstance().genericEvent("gameUpdate", pyArgs.makeFunctionArgs());
			// <advc.003r>
			for(int i = 0; i < NUM_UPDATE_TIMER_TYPES; i++)
				handleUpdateTimer((UpdateTimerTypes)i); // </advc.003r>
		}
		if (getTurnSlice() == 0) // advc (note): Implies 0 elapsed game turns
		{	// <advc.700> Delay initial auto-save until RiseFall is initialized
			if (!isOption(GAMEOPTION_RISE_FALL) &&// </advc.700>
				m_iTurnLoadedFromSave != m_iElapsedGameTurns) // advc.044
			{
				autoSave(true); // advc.106l
			}
			/* <advc.004m> This seems to be the earliest place where bubbles can
			   be enabled w/o crashing. */
			if (BUGOption::isEnabled("MainInterface__StartWithResourceIcons", true))
				gDLL->getEngineIFace()->setResourceLayer(true);
			// </advc.004m>
		}
		if (getNumGameTurnActive() == 0)
		{
			if (!isPbem() || !getPbemTurnSent())
				doTurn();
		}
		updateScore();
		updateWar();
		updateMoves();
		updateTimers();
		updateTurnTimer();
		AI().AI_updateAssignWork();
		testAlive();
		if (getAIAutoPlay() == 0 && !gDLL->GetAutorun() && GAMESTATE_EXTENDED != getGameState())
		{
			if (countHumanPlayersAlive() == 0 &&
				!isOption(GAMEOPTION_RISE_FALL)) // advc.707
			{
				setGameState(GAMESTATE_OVER);
			}
		}
		changeTurnSlice(1);
		/*	<advc.104l> Make sure that UI (specifically willing-to-talk indicator)
			doesn't become outdated throughout a turn. Or at least not for long.
			advc.085: Invalidating on every turn slice might slightly hurt
			responsiveness when hovering over the scoreboard. (Not sure.) */
		if (getTurnSlice() % 4 == 0)
			AI().uwai().invalidateUICache(); // </advc.104l>

		if (getActivePlayer() != NO_PLAYER && GET_PLAYER(getActivePlayer()).getAdvancedStartPoints() >= 0 &&
			!gDLL->UI().isInAdvancedStart())
		{
			gDLL->UI().setInAdvancedStart(true);
			gDLL->UI().setWorldBuilder(true);
		} // <advc.705>
		if(isOption(GAMEOPTION_RISE_FALL))
			m_pRiseFall->restoreDiploText(); // </advc.705>
	}
	PROFILE_END();
	stopProfilingDLL(false);
}


void CvGame::updateScore(bool bForce)
{
	if(!isScoreDirty() && !bForce)
		return;
	setScoreDirty(false);

	bool abPlayerScored[MAX_CIV_PLAYERS] = { false };
	std::vector<PlayerTypes> aeUpdateAttitude; // advc.001
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		int iBestScore = MIN_INT;
		PlayerTypes eBestPlayer = NO_PLAYER;

		for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
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
		// <advc>
		if(eBestPlayer == NO_PLAYER)
		{
			FAssert(eBestPlayer != NO_PLAYER);
			continue;
		} // </advc>
		abPlayerScored[eBestPlayer] = true;
		// <advc.001>
		if(iI != getPlayerRank(eBestPlayer))
			aeUpdateAttitude.push_back(eBestPlayer); // </advc.001>
		setRankPlayer(iI, eBestPlayer);
		setPlayerRank(eBestPlayer, iI);
		setPlayerScore(eBestPlayer, iBestScore);
		// advc.004s:
		GET_PLAYER(eBestPlayer).updateHistory(PLAYER_HISTORY_SCORE, getGameTurn());
	}
	/*for(size_t i = 0; i < updateAttitude.size(); i++)
		GET_PLAYER(updateAttitude[i]).AI_updateAttitude();*/
	// <advc.001>
	/*  The above isn't enough; the attitudes of those outside updateAttitude
		toward those inside could also change. */
	if(!aeUpdateAttitude.empty())
	{
		for(int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
			if(kLoopPlayer.isAlive() && !kLoopPlayer.isMinorCiv())
				kLoopPlayer.AI_updateAttitude();
		}
	} // </advc.001>

	bool abTeamScored[MAX_CIV_TEAMS] = { false };
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		int iBestScore = MIN_INT;
		TeamTypes eBestTeam = NO_TEAM;

		for (int iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
		{
			if (!abTeamScored[iJ])
			{
				int iScore = 0;

				for (int iK = 0; iK < MAX_CIV_PLAYERS; iK++)
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

// advc.003y: Ported from CvUtil.py
int CvGame::getScoreComponent(int iRawScore, int iInitial, int iMax,
	int iMultiplier, bool bExponential, bool bFinal, bool bVictory) const
{
	if (getEstimateEndTurn() <= 0)
		return 0;

	static scaled const rSCORE_FREE_PERCENT = per100(GC.getDefineINT(
			"SCORE_FREE_PERCENT"));
	static scaled const rSCORE_VICTORY_PERCENT = per100(GC.getDefineINT(
			"SCORE_VICTORY_PERCENT"));
	static scaled const rSCORE_HANDICAP_PERCENT_OFFSET = per100(GC.getDefineINT(
			"SCORE_HANDICAP_PERCENT_OFFSET"));
	static scaled const rSCORE_HANDICAP_PERCENT_PER = per100(GC.getDefineINT(
			"SCORE_HANDICAP_PERCENT_PER"));

	scaled rMax = iMax;
	if (bFinal && bVictory) // Not synchronized; floating point math is fine here.
	{
		scaled rTurnRatio(getGameTurn(), getEstimateEndTurn());
		if (bExponential && iInitial > 0)
			rMax = iInitial * (rMax / iInitial).pow(rTurnRatio);
		else rMax = iInitial + rTurnRatio * (rMax - iInitial);
	}
	scaled rFreeScore = rSCORE_FREE_PERCENT * rMax;
	scaled rScore = iMultiplier;
	scaled rDiv = rFreeScore + rMax;
	if (rDiv >= 1)
		rScore = (iRawScore + rFreeScore) * (iMultiplier / rDiv);
	if (!bVictory && !bFinal)
		return rScore.round();
	if (bVictory)
		rScore *= 1 + rSCORE_VICTORY_PERCENT;
	if (bFinal)
	{	// <advc.250a>
		rScore *= 1 + rSCORE_HANDICAP_PERCENT_OFFSET +
				scaled(getDifficultyForEndScore(), 10) * rSCORE_HANDICAP_PERCENT_PER;
		// </advc.250a>
	}
	return rScore.round();
}


void CvGame::updatePlotGroups()
{
	PROFILE_FUNC();

	for (PlayerIter<ALIVE> it; it.hasNext(); ++it)
		it->updatePlotGroups();
}


void CvGame::updateBuildingCommerce()
{
	for (PlayerIter<ALIVE> it; it.hasNext(); ++it)
		it->updateBuildingCommerce();
}


void CvGame::updateCitySight(bool bIncrement)
{
	for (PlayerIter<ALIVE> it; it.hasNext(); ++it)
		it->updateCitySight(bIncrement, false);
	updatePlotGroups();
}


void CvGame::updateTradeRoutes()
{
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it) // advc: exclude Barbarians
		it->updateTradeRoutes();
}

// K-Mod: calculate unhappiness due to the state of global warming
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
	} // advc: Ensure initialization
	else iGlobalPollution = iGwSeverityRating = iGlobalDefence = -1;
	for (PlayerIter<MAJOR_CIV> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		int iAngerPercent = 0;
		if (iGwIndex > 0)
		{
			// player unhappiness = base rate * severity rating * responsibility factor

			int iLocalDefence = calculateGwLandDefence(itPlayer->getID());
			int iResponsibilityFactor =	100*(itPlayer->calculatePollution() - iLocalDefence);

			iResponsibilityFactor /= std::max(1,
					calculateGwSustainabilityThreshold(itPlayer->getID()));
			iResponsibilityFactor *= calculateGwSustainabilityThreshold();
			iResponsibilityFactor /= std::max(1, iGlobalPollution - iGlobalDefence);
			// amplify the affects of responsibility
			iResponsibilityFactor = std::max(0, 2*iResponsibilityFactor-100);

			static int const iGLOBAL_WARMING_BASE_ANGER_PERCENT = GC.getDefineINT("GLOBAL_WARMING_BASE_ANGER_PERCENT"); // advc.opt
			iAngerPercent = iGLOBAL_WARMING_BASE_ANGER_PERCENT * iGwSeverityRating * iResponsibilityFactor;
			iAngerPercent = intdiv::round(iAngerPercent, 100 * 100);
		}
		itPlayer->setGwPercentAnger(iAngerPercent);
	}
}

/*  advc.106l: Wrapper that reports the event. Everyone should call this
	instead of calling the CvEngine function directly. */
void CvGame::autoSave(bool bInitial)
{
	/*  <advc.135c> Avoid overlapping auto-saves in test games played on a
		single machine. Don't know how to check this properly. */
	if(isNetworkMultiPlayer() && isDebugToolsAllowed(false) && getActivePlayer() % 2 == 0)
		return; // </advc.135c>
	CvEventReporter::getInstance().preAutoSave();
	gDLL->getEngineIFace()->AutoSave(bInitial);
	// BULL - AutoSave - start
	if(bInitial && BUGOption::isEnabled("AutoSave__CreateStartSave", false))
		GC.getPythonCaller()->call("gameStartSave", PYCivModule);
	// BULL - AutoSave - end
}

void CvGame::testExtendedGame()
{
	if (getGameState() != GAMESTATE_OVER)
		return;

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
	/*if (gDLL->UI().getHeadSelectedUnit() == NULL)
		bSelectGroup = true;
	else if (gDLL->UI().getHeadSelectedUnit()->getGroup() != pUnit->getGroup())
		bSelectGroup = true;
	else if (pUnit->IsSelected() && !(gDLL->UI().mirrorsSelectionGroup()))
		bSelectGroup = !bToggle;
	else bSelectGroup = false;*/ // BtS
	// K-Mod. Redesigned to make selection more sensible and predictable
	// In 'simple mode', shift always groups and always targets only a single unit.
	// advc.001: Option id was SimpleSelectionMode here but SimpleSelection in XML
	bool bSimpleMode = BUGOption::isEnabled("MainInterface__SimpleSelection", true);

	bool bExplicitDeselect = false;
	bool bSelectGroup = false;
	if (gDLL->UI().getHeadSelectedUnit() == NULL)
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
			bSelectGroup = bSimpleMode ? false : gDLL->UI().mirrorsSelectionGroup();
		}
	}
	else
	{
		bSelectGroup = gDLL->UI().mirrorsSelectionGroup()
			? gDLL->UI().getHeadSelectedUnit()->getGroup() != pUnit->getGroup()
			: pUnit->IsSelected();
	} // K-Mod end

	gDLL->UI().clearSelectedCities();
	bool bGroup = false;
	if (bClear)
	{
		gDLL->UI().clearSelectionList();
		bGroup = false;
	}
	else
	{	//bGroup = gDLL->UI().mirrorsSelectionGroup();
		// K-Mod. If there is only one unit selected, and it is to be toggled, just degroup it rather than unselecting it.
		if (bExplicitDeselect && gDLL->UI().getLengthSelectionList() == 1)
		{
			CvMessageControl::getInstance().sendJoinGroup(pUnit->getID(), FFreeList::INVALID_INDEX);
			return; // that's all.
		}
		bGroup = gDLL->UI().mirrorsSelectionGroup();
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
			CvUnit* pSelectionHead = gDLL->UI().getHeadSelectedUnit();
			if (pSelectionHead)
				CvMessageControl::getInstance().sendJoinGroup(pUnit->getID(), pSelectionHead->getID());
		} // K-Mod end
	}
	if (bSelectGroup)
	{
		gDLL->UI().selectionListPreChange();
		FOR_EACH_UNIT_VAR_IN(pUnit, *pUnit->getGroup())
		{
			FAssertMsg(pUnit != NULL, "null entity in selection group");
			gDLL->UI().insertIntoSelectionList(
					pUnit, false, bToggle, bGroup, bSound, true);
		}
		gDLL->UI().selectionListPostChange();
	}
	else
	{
		gDLL->UI().insertIntoSelectionList(pUnit, false, bToggle, bGroup, bSound);
		// K-Mod. Unfortunately, removing units from the group is not correctly handled by the interface functions.
		// so we need to do it explicitly.
		if (bExplicitDeselect && bGroup)
			CvMessageControl::getInstance().sendJoinGroup(pUnit->getID(), FFreeList::INVALID_INDEX);
		// K-Mod end
	}
	gDLL->UI().makeSelectionListDirty();
}

/*	K-Mod. I've made an ugly hack to change the functionality of double-click
	from select-all to wake-all. Here's how it works:
	If this function is called with only bAlt==true, but without the Alt key actually down,
	then wake-all is triggered rather than select-all.
	To achieve the select-all functionality without the Alt key,
	call the function with bCtrl && bAlt. */
void CvGame::selectGroup(CvUnit* pUnit, bool bShift, bool bCtrl, bool bAlt) const
{
	PROFILE_FUNC();

	FAssertMsg(pUnit != NULL, "pUnit == NULL unexpectedly");
	// <advc.002e> Show glow (only) on selected unit
	if(!BUGOption::isEnabled("PLE__ShowPromotionGlow", false))
	{
		CvPlayer const& kOwner = GET_PLAYER(pUnit->getOwner());
		FOR_EACH_UNIT_VAR(u, kOwner)
		{
			gDLL->getEntityIFace()->showPromotionGlow(u->getEntity(),
					u->atPlot(pUnit->plot()) && u->isPromotionReady());
		}
	} // </advc.002e>
	// K-Mod. the hack (see above)
	if (bAlt && !bShift && !bCtrl && !GC.altKey() &&
		!gDLL->altKey()) // (using gDLL->altKey, to better match the state of bAlt)
	{
		/*	The caller says alt is pressed, but the computer says otherwise.
			Lets assume this is a double-click. */
		CvPlot const& kUnitPlot = pUnit->getPlot();
		FOR_EACH_UNIT_VAR_IN(pLoopUnit, kUnitPlot)
		{
			if (pLoopUnit->getOwner() == getActivePlayer() &&
				pLoopUnit->isGroupHead() && pLoopUnit->isWaiting())
			{
				CvMessageControl::getInstance().sendDoCommand(
						pLoopUnit->getID(), COMMAND_WAKE, -1, -1, false);
			}
		}
		gDLL->UI().selectUnit(pUnit, true, false, true);
		return;
	}
	// K-Mod end

	if (bAlt || bCtrl)
	{
		gDLL->UI().clearSelectedCities();
		DomainTypes const eDomain = pUnit->getDomainType(); // K-Mod
		bool const bCheckMoves = pUnit->canMove() || pUnit->IsSelected(); // K-Mod.
		/*	(Note: The IsSelected check is to stop selected units with no moves
			from making it hard to select moveable units by clicking on the map.) */

		bool bGroup;
		if (!bShift)
		{
			gDLL->UI().clearSelectionList();
			bGroup = true;
		}
		else
		{
			//bGroup = gDLL->UI().mirrorsSelectionGroup();
			// K-Mod. Treat shift as meaning we should always form a group
			if (!gDLL->UI().mirrorsSelectionGroup())
				selectionListGameNetMessage(GAMEMESSAGE_JOIN_GROUP);
			/*	note: sometimes this won't work. (see comments in CvGame::selectUnit.)
				Unfortunately, it's too fiddly to fix. */
			bGroup = true;
			// K-Mod end
		}
		gDLL->getInterfaceIFace()->selectionListPreChange();
		CvPlot const& kUnitPlot = pUnit->getPlot();
		FOR_EACH_UNIT_VAR_IN(pLoopUnit, kUnitPlot)
		{
			if (pLoopUnit->getOwner() != getActivePlayer())
				continue;
			// K-Mod: added domain check and bCheckMoves
			if (pLoopUnit->getDomainType() == eDomain &&
				(!bCheckMoves || pLoopUnit->canMove()))
			{
				// disabled by K-Mod:
				//if (!isMPOption(MPOPTION_SIMULTANEOUS_TURNS) || getTurnSlice() - pLoopUnit->getLastMoveTurn() > GC.getDefineINT("MIN_TIMER_UNIT_DOUBLE_MOVES"))
				if (bAlt || pLoopUnit->getUnitType() == pUnit->getUnitType())
				{
					gDLL->getInterfaceIFace()->insertIntoSelectionList(
							pLoopUnit, false, false, bGroup, false, true);
				}
			}
		}

		gDLL->getInterfaceIFace()->selectionListPostChange();
	}
	else gDLL->getInterfaceIFace()->selectUnit(pUnit, !bShift, bShift, true);
}


void CvGame::selectAll(CvPlot* pPlot) const
{
	CvUnit* pSelectUnit = NULL;
	if (pPlot != NULL)
	{
		CvUnit* pCenterUnit = pPlot->getDebugCenterUnit();
		if (pCenterUnit != NULL && pCenterUnit->getOwner() == getActivePlayer())
			pSelectUnit = pCenterUnit;
	}
	if (pSelectUnit != NULL)
	{
		gDLL->getInterfaceIFace()->selectGroup(pSelectUnit,
				//false, false, true);
				false, true, true); // K-Mod
	}
}


bool CvGame::selectionListIgnoreBuildingDefense() const
{
	//PROFILE_FUNC(); // advc.003o

	bool bIgnoreBuilding = false;
	bool bAttackLandUnit = false;

	for (CLLNode<IDInfo> const* pNode = gDLL->UI().headSelectionListNode();
		pNode != NULL; pNode = gDLL->UI().nextSelectionListNode(pNode))
	{
		CvUnit* pSelectedUnit = ::getUnit(pNode->m_data);
		if (pSelectedUnit == NULL)
			continue;
		if (pSelectedUnit->ignoreBuildingDefense())
			bIgnoreBuilding = true;
		if (pSelectedUnit->getDomainType() == DOMAIN_LAND &&
			pSelectedUnit->canAttack())
		{
			bAttackLandUnit = true;
		}
	}

	if (!bIgnoreBuilding && !bAttackLandUnit &&
		getBestLandUnit() != NO_UNIT)
	{
		bIgnoreBuilding = GC.getInfo(getBestLandUnit()).isIgnoreBuildingDefense();
	}

	return bIgnoreBuilding;
}


void CvGame::implementDeal(PlayerTypes eWho, PlayerTypes eOtherWho,
	CLinkList<TradeData>* pOurList, CLinkList<TradeData>* pTheirList,
	bool bForce)
{
	// <advc> Not sure if the EXE ever calls implementDeal with a NULL list
	CLinkList<TradeData> emptyList;
	implementDeal(eWho, eOtherWho,
			pOurList == NULL ? emptyList : *pOurList,
			pTheirList == NULL ? emptyList : *pTheirList,
			bForce);
}


void CvGame::implementDeal(PlayerTypes eWho, PlayerTypes eOtherWho,
	CLinkList<TradeData>& kOurList, CLinkList<TradeData>& kTheirList,
	bool bForce)
{
	// </advc>  <advc.036>
	implementAndReturnDeal(eWho, eOtherWho, kOurList, kTheirList, bForce);
}


CvDeal* CvGame::implementAndReturnDeal(PlayerTypes eWho, PlayerTypes eOtherWho,
	CLinkList<TradeData>& kOurList, CLinkList<TradeData>& kTheirList,
	bool bForce) // </advc.036>
{
	FAssert(eWho != NO_PLAYER);
	FAssert(eOtherWho != NO_PLAYER);
	FAssert(eWho != eOtherWho);

	CvDeal* pDeal = addDeal();
	pDeal->init(pDeal->getID(), eWho, eOtherWho);
	pDeal->addTradeItems(kOurList, kTheirList, !bForce);
	if (pDeal->getLengthFirst() <= 0 && pDeal->getLengthSecond() <= 0)
	{
		pDeal->kill();
		return NULL; // advc.036
	}
	return pDeal; // advc.036
}


void CvGame::verifyDeals()
{
	FOR_EACH_DEAL_VAR(pLoopDeal)
		pLoopDeal->verify();
}

/*	Globeview configuration control:
	If bStarsVisible, then there will be stars visible behind the globe when it is on.
	If bWorldIsRound, then the world will bend into a globe;
	otherwise, it will show up as a plane.  */
void CvGame::getGlobeviewConfigurationParameters(TeamTypes eTeam,
	bool& bStarsVisible, bool& bWorldIsRound)
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
	if (GC.getInfo(eVictory).getPopulationPercentLead() == 0)
		return 0;
	if (getTotalPopulation() == 0)
		return 100;

	int iBestPopulation = 0;
	int iNextBestPopulation = 0;
	for (TeamIter<CIV_ALIVE> itTeam; itTeam.hasNext(); ++itTeam)
	{
		int iPopulation = itTeam->getTotalPopulation();
		if (iPopulation > iBestPopulation)
		{
			iNextBestPopulation = iBestPopulation;
			iBestPopulation = iPopulation;
		}
		else if (iPopulation > iNextBestPopulation)
			iNextBestPopulation = iPopulation;
	}
	return std::min(100, ((iNextBestPopulation * 100) / getTotalPopulation()) +
			GC.getInfo(eVictory).getPopulationPercentLead());
}


int CvGame::getProductionPerPopulation(HurryTypes eHurry) const
{
	if (eHurry == NO_HURRY)
		return 0;
	return (GC.getInfo(eHurry).getProductionPerPopulation() * 100) /
			std::max(1, GC.getInfo(getGameSpeedType()).getHurryPercent());
}


int CvGame::getAdjustedLandPercent(VictoryTypes eVictory) const
{
	if (GC.getInfo(eVictory).getLandPercent() == 0)
		return 0;

	int iPercent = GC.getInfo(eVictory).getLandPercent();
	iPercent -= (getCivTeamsEverAlive() * 2);
	return std::max(iPercent, GC.getInfo(eVictory).getMinLandPercent());
}

// advc.178: Cut from CvPlayerAI::AI_calculateDiplomacyVictoryStage
bool CvGame::isDiploVictoryValid() const
{
	FOR_EACH_ENUM(Victory)
	{
		if (isVictoryValid(eLoopVictory) && GC.getInfo(eLoopVictory).isDiploVote())
			return true;
	}
	return false;
}


bool CvGame::isTeamVote(VoteTypes eVote) const
{
	return (GC.getInfo(eVote).isSecretaryGeneral() || GC.getInfo(eVote).isVictory());
}


bool CvGame::isChooseElection(VoteTypes eVote) const
{
	return !GC.getInfo(eVote).isSecretaryGeneral();
}


bool CvGame::isTeamVoteEligible(TeamTypes eTeam, VoteSourceTypes eVoteSource) const
{
	CvTeam const& kTeam = GET_TEAM(eTeam);
	if (kTeam.isForceTeamVoteEligible(eVoteSource))
		return true;
	if (!kTeam.isFullMember(eVoteSource))
		return false;

	int iCount = 0;
	for (TeamIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		if (it->isForceTeamVoteEligible(eVoteSource))
			iCount++;
	}
	int iExtraEligible = GC.getDefineINT("TEAM_VOTE_MIN_CANDIDATES") - iCount;
	if (iExtraEligible <= 0)
		return false;
	for (TeamIter<CIV_ALIVE,NOT_SAME_TEAM_AS> it(eTeam); it.hasNext(); ++it)
	{
		CvTeam const& kLoopTeam = *it;
		if (!kLoopTeam.isForceTeamVoteEligible(eVoteSource) &&
			kLoopTeam.isFullMember(eVoteSource))
		{
			int iLoopVotes = kLoopTeam.getVotes(NO_VOTE, eVoteSource);
			int iVotes = kTeam.getVotes(NO_VOTE, eVoteSource);
			// <advc.014>
			if(!kTeam.isCapitulated() || !kLoopTeam.isCapitulated())
			{
				if(kTeam.isCapitulated())
					iVotes = 0;
				if(kLoopTeam.isCapitulated())
					iLoopVotes = 0;
			} // </advc.014>
			if (iLoopVotes > iVotes ||
				(iLoopVotes == iVotes && kLoopTeam.getID() < eTeam))
			{
				iExtraEligible--;
			}
		}
	}
	return (iExtraEligible > 0);
}


int CvGame::countVote(VoteTriggeredData const& kData, PlayerVoteTypes eChoice) const
{
	int iCount = 0;
	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		if (getPlayerVote(itPlayer->getID(), kData.getID()) == eChoice)
			iCount += itPlayer->getVotes(kData.kVoteOption.eVote, kData.eVoteSource);
	}
	return iCount;
}


int CvGame::countPossibleVote(VoteTypes eVote, VoteSourceTypes eVoteSource) const
{
	int iCount = 0;
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
		iCount += it->getVotes(eVote, eVoteSource);
	return iCount;
}



TeamTypes CvGame::findHighestVoteTeam(const VoteTriggeredData& kData) const
{
	if (!isTeamVote(kData.kVoteOption.eVote))
		return NO_TEAM;
	TeamTypes eBestTeam = NO_TEAM;
	int iBestCount = 0;
	for (TeamIter<CIV_ALIVE> itTeam; itTeam.hasNext(); ++itTeam)
	{
		int iCount = countVote(kData, (PlayerVoteTypes)itTeam->getID());
		if (iCount > iBestCount)
		{
			iBestCount = iCount;
			eBestTeam = itTeam->getID();
		}
	}
	return eBestTeam;
}


int CvGame::getVoteRequired(VoteTypes eVote, VoteSourceTypes eVoteSource) const
{
	return (countPossibleVote(eVote, eVoteSource) *
			GC.getInfo(eVote).getPopulationThreshold()) / 100;
}


TeamTypes CvGame::getSecretaryGeneral(VoteSourceTypes eVoteSource) const
{
	if (!canHaveSecretaryGeneral(eVoteSource))
	{
		FOR_EACH_ENUM(Building)
		{
			if (GC.getInfo(eLoopBuilding).getVoteSourceType() != eVoteSource)
				continue;
			for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
			{
				if (itPlayer->getBuildingClassCount(
					GC.getInfo(eLoopBuilding).getBuildingClassType()) > 0)
				{
					ReligionTypes eReligion = getVoteSourceReligion(eVoteSource);
					if (eReligion == NO_RELIGION ||
						itPlayer->getStateReligion() == eReligion)
					{
						return itPlayer->getTeam();
					}
				}
			}
		}
		return NO_TEAM; // advc
	}
	FOR_EACH_ENUM(Vote)
	{
		if (GC.getInfo(eLoopVote).isVoteSourceType(eVoteSource) &&
			GC.getInfo(eLoopVote).isSecretaryGeneral() &&
			isVotePassed(eLoopVote))
		{
			TeamTypes eSecretary = (TeamTypes)getVoteOutcome(eLoopVote);
			if (GET_TEAM(eSecretary).isAlive()) // advc.001
				return eSecretary;
		}
	}
	return NO_TEAM;
}

bool CvGame::canHaveSecretaryGeneral(VoteSourceTypes eVoteSource) const
{
	FOR_EACH_ENUM(Vote)
	{
		if (GC.getInfo(eLoopVote).isVoteSourceType(eVoteSource) &&
			GC.getInfo(eLoopVote).isSecretaryGeneral())
		{
			return true;
		}
	}
	return false;
}

void CvGame::clearSecretaryGeneral(VoteSourceTypes eVoteSource)
{
	FOR_EACH_ENUM2(Vote, eVote)
	{
		CvVoteInfo& kVote = GC.getInfo(eVote);
		if (kVote.isVoteSourceType(eVoteSource) &&
			kVote.isSecretaryGeneral())
		{
			VoteTriggeredData kData;
			kData.eVoteSource = eVoteSource;
			kData.kVoteOption.eVote = eVote;
			kData.kVoteOption.iCityId = -1;
			kData.kVoteOption.szText.clear(); // kmodx
			kData.kVoteOption.ePlayer = NO_PLAYER;
			setVoteOutcome(kData, NO_PLAYER_VOTE);
			setSecretaryGeneralTimer(eVoteSource, 0);
		}
	}
}

void CvGame::updateSecretaryGeneral()
{
	FOR_EACH_ENUM(VoteSource)
	{
		TeamTypes eSecretary = getSecretaryGeneral(eLoopVoteSource);
		if (eSecretary != NO_TEAM &&
			(!GET_TEAM(eSecretary).isFullMember(eLoopVoteSource) ||
			GET_TEAM(eSecretary).isCapitulated())) // advc.014
		{
			clearSecretaryGeneral(eLoopVoteSource);
		}
	}
}

int CvGame::countCivPlayersAlive() const
{
	int iCount = 0;
	for (int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		if (GET_PLAYER((PlayerTypes)i).isAlive())
			iCount++;
	}
	return iCount;
}


int CvGame::countCivPlayersEverAlive() const
{	// advc.opt:
	FAssertMsg(!m_bAllGameDataRead, "Should use getCivPlayersEverAlive instead");
	int iCount = 0;
	for (int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayer const& kPlayer = GET_PLAYER((PlayerTypes)i);
		if (kPlayer.isEverAlive())
		{
			if (kPlayer.getParent() == NO_PLAYER)
				iCount++;
		}
	}
	return iCount;
}


int CvGame::countCivTeamsAlive() const
{
	int iCount = 0;
	for (int i = 0; i < MAX_CIV_TEAMS; i++)
	{
		if (GET_TEAM((TeamTypes)i).isAlive())
			iCount++;
	}
	return iCount;
}


int CvGame::countCivTeamsEverAlive() const
{	// advc.opt:
	FAssertMsg(!m_bAllGameDataRead, "Should use getCivTeamsEverAlive instead");
	std::set<int> teamsEverAlive;
	for (int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)i);
		if (kPlayer.isEverAlive() &&
			kPlayer.getParent() == NO_PLAYER)
		{
			teamsEverAlive.insert(kPlayer.getTeam());
		}
	}
	return teamsEverAlive.size();
}


int CvGame::countHumanPlayersAlive() const
{
	int iCount = 0;
	for (int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)i);
		if (kPlayer.isAlive() && (kPlayer.isHuman() ||
			/*  advc.127: To prevent CvGame::update from concluding that the
				game is over when human is still disabled at the start of a round. */
			kPlayer.isHumanDisabled()))
		{
			iCount++;
		}
	}
	return iCount;
}

// K-Mod:
int CvGame::countFreeTeamsAlive() const
{
	int iCount = 0;
	for (int i = 0; i < MAX_CIV_TEAMS; i++)
	{
		CvTeam const& kLoopTeam = GET_TEAM((TeamTypes)i);
		//if (kLoopTeam.isAlive() && !kLoopTeam.isCapitulated())
		// I'm in two minds about which of these to use here
		// advc.agent (note): Keep this as in K-Mod. Vassals aren't free.
		if (kLoopTeam.isAlive() && !kLoopTeam.isAVassal())
			iCount++;
	}
	return iCount;
}

// <advc.137>
int CvGame::getRecommendedPlayers() const
{
	CvWorldInfo const& kWorld = GC.getInfo(GC.getMap().getWorldSize());
	scaled r = kWorld.getDefaultPlayers();
	r *= per100(100 - 4 * getSeaLevelChange());
	r.clamp(2, PlayerIter<>::count());
	return r.round();
}

// advc.140:
int CvGame::getSeaLevelChange() const
{
	int r = 0;
	SeaLevelTypes eSeaLevel = GC.getInitCore().getSeaLevel();
	if(eSeaLevel != NO_SEALEVEL)
		r = GC.getInfo(eSeaLevel).getSeaLevelChange();
	return r;
} // </advc.137>


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
	if (GC.getInfo(eBuilding).getNumFreeBonuses() == -1)
		return GC.getInfo(GC.getMap().getWorldSize()).getNumFreeBuildingBonuses();
	return GC.getInfo(eBuilding).getNumFreeBonuses();
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
			FOR_EACH_CITY_VAR(pCity, kLoopPlayer)
			{
				if (pCity->isHasCorporation(eCorporation1))
				{
					pCity->setHasCorporation(eCorporation1, false, false, false);
					pCity->setHasCorporation(eCorporation2, true, true);
				}
			}

			FOR_EACH_UNIT_VAR(pUnit, kLoopPlayer)
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
	bool bIgnoreOtherReligions) const // advc.115b: Param added
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
	static int const iGOLDEN_AGE_LENGTH = GC.getDefineINT("GOLDEN_AGE_LENGTH"); // advc.opt
	int iLength = iGOLDEN_AGE_LENGTH;

	iLength *= GC.getInfo(getGameSpeedType()).getGoldenAgePercent();
	iLength /= 100;

	return iLength;
}


int CvGame::victoryDelay(VictoryTypes eVictory) const
{
	FAssert(eVictory >= 0 && eVictory < GC.getNumVictoryInfos());

	int iLength = GC.getInfo(eVictory).getVictoryDelayTurns();

	iLength *= GC.getInfo(getGameSpeedType()).getVictoryDelayPercent();
	iLength /= 100;

	return iLength;
}


int CvGame::getImprovementUpgradeTime(ImprovementTypes eImprovement) const
{
	int iTime = GC.getInfo(eImprovement).getUpgradeTime();

	iTime *= GC.getInfo(getGameSpeedType()).getImprovementPercent();
	iTime /= 100;

	iTime *= GC.getInfo(getStartEra()).getImprovementPercent();
	iTime /= 100;

	return iTime;
}

/*  advc: 3 for Marathon, 0.67 for Quick. Based on VictoryDelay. For cases where
	there isn't a more specific game speed modifier that could be applied. (E.g.
	tech costs should be adjusted based on iResearchPercent, not on this function.) */
scaled CvGame::gameSpeedMultiplier() const
{
	return per100(GC.getInfo(getGameSpeedType()).getVictoryDelayPercent());
}

bool CvGame::canTrainNukes() const
{
	for (PlayerIter<ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		CvCivilization const& kCiv = itPlayer->getCivilization(); // advc.003w
		for (int i = 0; i < kCiv.getNumUnits(); i++)
		{
			UnitTypes eUnit = kCiv.unitAt(i);
			if (GC.getInfo(eUnit).getNukeRange() >= 0)
			{
				if (itPlayer->canTrain(eUnit))
					return true;
			}
		}
	}
	return false;
}


EraTypes CvGame::getCurrentEra() const
{
	/*	advc.opt: OK - negligble. NB: Some call locations changed
		to CvGameAI::AI_getCurrEraFactor (computing time also negligible). */
	//PROFILE_FUNC();

	int iEra = 0;
	// K-Mod: don't count the barbarians
	PlayerIter<CIV_ALIVE> it;
	for (; it.hasNext(); ++it)
		iEra += it->getCurrentEra();
	int const iCount = it.nextIndex();
	if (iCount > 0)
	{
		//return (EraTypes)(iEra / iCount);
		return (EraTypes)intdiv::uround(iEra, iCount); // kekm.17
	}
	FAssert(iCount > 0); // advc
	return NO_ERA;
}

// advc:
EraTypes CvGame::getHighestEra() const
{
	EraTypes eMax = NO_ERA;
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
		eMax = std::max(eMax, it->getCurrentEra());
	return eMax;
}

// advc.groundbr: Normalize tech costs when groundbreaking penalties are enabled
scaled CvGame::groundbreakingNormalizationModifier(TechTypes eTech) const
{
	if (!GC.getDefineBOOL(CvGlobals::AI_GROUNDBREAKING_PENALTY_ENABLE))
		return 0;
	EraTypes const eTechEra = GC.getInfo(eTech).getEra();
	if (eTechEra <= GC.getGame().getStartEra())
		return 0;
	return -per100(GC.getInfo(eTechEra).get(CvEraInfo::AIMaxGroundbreakingPenalty)) / 4;
}


TeamTypes CvGame::getActiveTeam() const
{
	if (getActivePlayer() == NO_PLAYER)
		return NO_TEAM;
	return (TeamTypes)GET_PLAYER(getActivePlayer()).getTeam();
}


CivilizationTypes CvGame::getActiveCivilizationType() const
{
	if (getActivePlayer() == NO_PLAYER)
		return NO_CIVILIZATION;
	return (CivilizationTypes)GET_PLAYER(getActivePlayer()).getCivilizationType();
}

// advc.003w:
CvCivilization const* CvGame::getActiveCivilization() const
{
	PlayerTypes eActivePlayer = getActivePlayer();
	if (eActivePlayer == NO_PLAYER)
		return NULL;
	return &GET_PLAYER(eActivePlayer).getCivilization();
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


bool CvGame::isModem() /* advc: */ const
{
	return gDLL->IsModem();
}


void CvGame::setModem(bool bModem)
{
	if (bModem)
		gDLL->ChangeINIKeyValue("CONFIG", "Bandwidth", "modem");
	else gDLL->ChangeINIKeyValue("CONFIG", "Bandwidth", "broadband");
	gDLL->SetModem(bModem);
}


void CvGame::reviveActivePlayer()
{
	if (GET_PLAYER(getActivePlayer()).isAlive())
		return;

	setAIAutoPlay(0, /* advc.127: */ false);
	GC.getInitCore().setSlotStatus(getActivePlayer(), SS_TAKEN);
	if (GC.getPythonCaller()->doReviveActivePlayer())
		return;
	GET_PLAYER(getActivePlayer()).initUnit((UnitTypes)0, 0, 0);
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


int CvGame::getTurnYear(int iGameTurn) /* advc: */ const
{
	// moved the body of this method to Game Core Utils so we have access for other games than the current one (replay screen in HOF)
	return getTurnYearForGame(iGameTurn, getStartYear(), getCalendar(), getGameSpeedType());
}


int CvGame::getGameTurnYear() const // advc: const
{
	//return getTurnYear(getGameTurn()); // To work aorund non-const getGameTurn
	return getTurnYear(getGameTurn());
}


void CvGame::incrementElapsedGameTurns()
{
	m_iElapsedGameTurns++;
}

// advc.251:
int CvGame::AIHandicapAdjustment() const
{
	int iGameTurn = getGameTurn();
	int iVictoryDelayPercent = GC.getInfo(getGameSpeedType()).getVictoryDelayPercent();
	if(iVictoryDelayPercent > 0)
		iGameTurn = (iGameTurn * 100) / iVictoryDelayPercent;
	int iIncrementTurns = GC.getInfo(getHandicapType()).getAIHandicapIncrementTurns();
	if(iIncrementTurns == 0)
		return 0;
	/*  Flip sign b/c we're dealing with cost modifiers that are supposed to decrease.
		Only if a negative AIHandicapIncrement is set in XML, the modifiers are
		supposed to increase. */
	return -iGameTurn / iIncrementTurns;
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


void CvGame::setMaxCityElimination(int iNewValue)
{
	GC.getInitCore().setMaxCityElimination(iNewValue);
	FAssert(getMaxCityElimination() >= 0);
}


void CvGame::setNumAdvancedStartPoints(int iNewValue)
{
	GC.getInitCore().setNumAdvancedStartPoints(iNewValue);
	FAssert(getNumAdvancedStartPoints() >= 0);
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

/*  advc: Ratio of turns played to total estimated game length; between 0 and 1.
	iDelay is added to the number of turns played. */
double CvGame::gameTurnProgress(int iDelay) const
{
	/*  Even with time victory disabled, we shouldn't expect the game to last
		beyond 2050. So, no need to check if it's disabled. */
	double gameLength = getEstimateEndTurn() - getStartTurn();
	return std::min(1.0, (getElapsedGameTurns() + iDelay) / gameLength);
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
				iTurnLen = (iTurnLen * GC.getInfo(eTurnTimer).getFirstTurnMultiplier());
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
		return (GC.getInfo(eTurnTimer).getBaseTime() +
				(GC.getInfo(eTurnTimer).getCityBonus()*iMaxCities) +
				(GC.getInfo(eTurnTimer).getUnitBonus()*iMaxUnits));
	}
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
	int iCount = 0;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isHuman())
		{
			if (GET_PLAYER((PlayerTypes)iI).isTurnActive())
				iCount++;
		}
	}
	return iCount;
}


void CvGame::changeNumGameTurnActive(int iChange)
{
	m_iNumGameTurnActive = (m_iNumGameTurnActive + iChange);
	FAssert(getNumGameTurnActive() >= 0);
	m_iUnitUpdateAttempts = 0; // advc.001y
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

// advc: Moved from CvGameCoreUtils
int CvGame::getWonderScore(BuildingClassTypes eWonderClass) const
{
	if (GC.getInfo(eWonderClass).isLimited())
		return 5;
	return 0;
}

// initialize score calculation
void CvGame::initScoreCalculation()
{
	int iMaxFood = 0;
	CvMap const& kMap = GC.getMap();
	for (int i = 0; i < kMap.numPlots(); i++)
	{
		CvPlot const& kPlot = kMap.getPlotByIndex(i);
		if (!kPlot.isWater() || kPlot.isAdjacentToLand())
			iMaxFood += kPlot.calculateBestNatureYield(YIELD_FOOD, NO_TEAM);
	}
	m_iMaxPopulation = getPopulationScore(iMaxFood / std::max(1,
			GC.getFOOD_CONSUMPTION_PER_POPULATION()));
	m_iMaxLand = getLandPlotsScore(GC.getMap().getLandPlots());
	m_iMaxTech = 0;
	FOR_EACH_ENUM(Tech)
	{
		m_iMaxTech += getTechScore(eLoopTech);
	}
	m_iMaxWonders = 0;
	FOR_EACH_ENUM(BuildingClass)
	{
		m_iMaxWonders += getWonderScore(eLoopBuildingClass);
	}

	if (getStartEra() != NO_ERA)
	{
		int iNumSettlers = GC.getInfo(getStartEra()).getStartingUnitMultiplier();
		m_iInitPopulation = getPopulationScore(iNumSettlers *
				(GC.getInfo(getStartEra()).getFreePopulation() + 1));
		m_iInitLand = getLandPlotsScore(iNumSettlers *  NUM_CITY_PLOTS);
	}
	else
	{
		m_iInitPopulation = 0;
		m_iInitLand = 0;
	}

	m_iInitTech = 0;
	FOR_EACH_ENUM(Tech)
	{
		if (GC.getInfo(eLoopTech).getEra() < getStartEra())
			m_iInitTech += getTechScore(eLoopTech);
		else
		{
			/*	count all possible free techs as initial to lower the score
				from immediate retirement */
			FOR_EACH_ENUM(Civilization)
			{
				if (GC.getInfo(eLoopCivilization).isPlayable())
				{
					if (GC.getInfo(eLoopCivilization).isCivilizationFreeTechs(
						eLoopCivilization))
					{
						m_iInitTech += getTechScore(eLoopTech);
						break;
					}
				}
			}
		}
	}
	m_iInitWonders = 0;
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
	if (!isGameMultiPlayer())
	{
		GET_PLAYER(getActivePlayer()).setHumanDisabled((getAIAutoPlay() != 0));
		return;
	} // </advc.127>
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.isHuman() || kLoopPlayer.isHumanDisabled())
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

// <advc.opt>
int CvGame::getCivPlayersEverAlive() const
{
	// Could pose a savegame compatibility problem (uiFlag<4)
	FAssert(m_bAllGameDataRead); // But it's OK if uiFlag < 9 in CvPlayerAI
	if(!m_bAllGameDataRead)
		return countCivPlayersEverAlive();
	return m_iCivPlayersEverAlive;
}

void CvGame::changeCivPlayersEverAlive(int iChange)
{
	m_iCivPlayersEverAlive += iChange;
	//FAssert(iChange >= 0);
	/*  iChange normally shouldn't be negative, but liberated civs aren't supposed
		to count, and they're set to 'alive' before getting marked as liberated
		(CvPlayer::setParent), so they need to be subtracted once setParent is called. */
	FAssertBounds(0, MAX_CIV_PLAYERS + 1, m_iCivPlayersEverAlive);
}

int CvGame::getCivTeamsEverAlive() const
{
	// Could pose a savegame compatibility problem (uiFlag<4)
	FAssert(m_bAllGameDataRead); // But it's OK if uiFlag < 9 in CvPlayerAI
	if(!m_bAllGameDataRead)
		return countCivTeamsEverAlive();
	return m_iCivTeamsEverAlive;
}

void CvGame::changeCivTeamsEverAlive(int iChange)
{
	m_iCivTeamsEverAlive += iChange;
	FAssertBounds(0, MAX_CIV_TEAMS + 1, m_iCivTeamsEverAlive);
} // </advc.opt>

/*  K-mod, 6/dec/10:
	(18/dec/10 - added Gw calc functions) */
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
	/*	Note: this is the number of chances global warming has to strike
		in the current turn. As you can see, I've scaled it by the game length.
		The probability per chance is also scaled like this. I estimate that
		the global warming index will actually be roughly proportional to the
		number of turns in the game, so, by scaling the chances, and the
		probability per chance, I hope to get roughly the same number of
		actual events per game. */
	int iIndexPerChance = GC.getDefineINT("GLOBAL_WARMING_INDEX_PER_CHANCE");
	iIndexPerChance *= GC.getInfo(getGameSpeedType()).getVictoryDelayPercent();
	iIndexPerChance /= 100;
	return intdiv::round(getGlobalWarmingIndex(), std::max(1, iIndexPerChance));
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
	CvMap const& kMap = GC.getMap();
	for (int i = 0; i < kMap.numPlots(); ++i)
	{
		CvPlot const& kPlot = kMap.getPlotByIndex(i);
		if (kPlot.isFeature())
		{
			if (ePlayer == NO_PLAYER || ePlayer == kPlot.getOwner())
				iTotal += GC.getInfo(kPlot.getFeatureType()).getWarmingDefense();
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
		return iGlobalThreshold * kPlayer.getTotalLand() /
				std::max(1, GC.getMap().getLandPlots());
	}
	return 0;
}

int CvGame::calculateGwSeverityRating() const
{
	/*	Here are some of the properties I want from this function:
		- the severity should be a number between 0 and 100 (ie. a percentage value)
		- zero severity should mean zero global warming
		- the function should asymptote towards 100
		- It should be a function of the index divided by (total land area * game length).
	I recommend looking at the graph of this function to get a sense of how it works. */
	/*	advc: Was long, which is equivalent to int. Could use long long,
		but it looks like 32 bit should suffice. */
	int const x = GC.getDefineINT("GLOBAL_WARMING_PROB") * getGlobalWarmingIndex() /
			std::max(1, GC.getMap().getLandPlots() * 4 *
			GC.getInfo(getGameSpeedType()).getVictoryDelayPercent());
	// shape parameter. Lower values result in the function being steeper earlier.
	int const b = 70;
	return 100 - (b * 100) / (b + SQR(x));
} // K-Mod end

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
void CvGame::setUpdateTimer(UpdateTimerTypes eTimerType, int iDelay)
{
	FAssertBounds(0, NUM_UPDATE_TIMER_TYPES, eTimerType);
	// <advc.001w>
	if(eTimerType == UPDATE_MOUSE_FOCUS && BUGOption::isEnabled("MainInterface__RapidUnitCycling", false)) {
		// No need for this hack when there is no unit-cycling delay
		iDelay = -1;
	} // </advc.001w>
	m_aiUpdateTimers[eTimerType] = iDelay;
}


int CvGame::getUpdateTimer(UpdateTimerTypes eTimerType) const
{
	FAssertBounds(0, NUM_UPDATE_TIMER_TYPES, eTimerType);
	return m_aiUpdateTimers[eTimerType];
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
	static bool const bCIRCUMNAVIGATE_FREE_MOVES = GC.getDefineBOOL("CIRCUMNAVIGATE_FREE_MOVES"); // advc.opt
	if (!bCIRCUMNAVIGATE_FREE_MOVES)
		return false;

	if (isCircumnavigated())
		return false;

	CvMap& kMap = GC.getMap();

	if (!kMap.isWrapX() && !kMap.isWrapY())
		return false;

	if (kMap.getLandPlots() > (kMap.numPlots() * 2) / 3)
		return false;

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
			GET_PLAYER((PlayerTypes)iPlayer).processVoteSource(eVoteSource, false);
		}

		m_aiDiploVote[eVoteSource] += iChange;
		FAssert(getDiploVoteCount(eVoteSource) >= 0);

		for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
		{
			GET_PLAYER((PlayerTypes)iPlayer).processVoteSource(eVoteSource, true);
		}
	}
}

bool CvGame::canDoResolution(VoteSourceTypes eVoteSource, const VoteSelectionSubData& kData) const
{
	CvVoteInfo const& kVote = GC.getInfo(kData.eVote); // advc
	if (kVote.isVictory())
	{
		int iVotesRequired = getVoteRequired(kData.eVote, eVoteSource); // K-Mod
		for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
		{
			CvTeam& kTeam = GET_TEAM((TeamTypes)iTeam);

			if (kTeam.getVotes(kData.eVote, eVoteSource) >= iVotesRequired)
				return false; // K-Mod. same, but faster.
			/*if (kTeam.isVotingMember(eVoteSource)) {
				if (kTeam.getVotes(kData.eVote, eVoteSource) >= getVoteRequired(kData.eVote, eVoteSource)) {
					// Can't vote on a winner if one team already has all the votes necessary to win
					return false;
				}
			}*/ // BtS
		}
	}

	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);

		if (kPlayer.isVotingMember(eVoteSource))
		{	// <kekm.25/advc>
			if(kVote.isForceWar())
			{
				if(GET_TEAM(kPlayer.getTeam()).isFullMember(eVoteSource) &&
					!kPlayer.canDoResolution(eVoteSource, kData))
				{
					return false;
				}
			}
			else // </kekm.25/advc>
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

// advc (note): Needs to be consistent with processVote
bool CvGame::isValidVoteSelection(VoteSourceTypes eVoteSource, VoteSelectionSubData const& kData) const
{
	if (kData.ePlayer!= NO_PLAYER)
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);
		if (!kPlayer.isAlive() || kPlayer.isBarbarian() || kPlayer.isMinorCiv())
			return false;
	}

	if (kData.eOtherPlayer != NO_PLAYER)
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.eOtherPlayer);
		if (!kPlayer.isAlive() || kPlayer.isBarbarian() || kPlayer.isMinorCiv())
			return false;
	}

	int iNumVoters = 0;
	for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
	{
		//if (GET_TEAM((TeamTypes)iTeam).isVotingMember(eVoteSource))
		// K-Mod. to prevent "AP cheese", only count full members for victory votes.
		if (GET_TEAM((TeamTypes)iTeam).isFullMember(eVoteSource) ||
			(!GC.getInfo(kData.eVote).isVictory() &&
			GET_TEAM((TeamTypes)iTeam).isVotingMember(eVoteSource)))
		// K-Mod end
		{
			++iNumVoters;
		}
	}
	if (iNumVoters  < GC.getInfo(kData.eVote).getMinVoters())
		return false;

	if (GC.getInfo(kData.eVote).isOpenBorders())
	{
		bool bOpenBordersWithEveryone = true;
		for (TeamIter<MAJOR_CIV> itFirst; itFirst.hasNext(); ++itFirst)
		{
			/*	advc (note): Applies to all members, but becomes unavailable if
				all full members already have OB. Not sure if that's sensible. */
			if (!itFirst->isFullMember(eVoteSource))
				continue;
			for (TeamIter<MAJOR_CIV> itSecond; itSecond.hasNext(); ++itSecond)
			{
				if (!itSecond->isFullMember(eVoteSource))
					continue;
				if (itSecond->getID() > itFirst->getID())
				{
					if (!itSecond->isOpenBorders(itFirst->getID()))
					{
						bOpenBordersWithEveryone = false;
						break;
					}
				}
			}
		}
		if (bOpenBordersWithEveryone)
			return false;
	}
	else if (GC.getInfo(kData.eVote).isDefensivePact())
	{
		bool bDefensivePactWithEveryone = true;
		// advc.001: Exclude vassals
		for (TeamIter<FREE_MAJOR_CIV> itFirst; itFirst.hasNext(); ++itFirst)
		{
			/*	advc (note): Applies to all members, but becomes unavailable if
				all full members already have DP. Not sure if that's sensible. */
			if (!itFirst->isFullMember(eVoteSource))
				continue;
			// advc.001: Exclude vassals
			for (TeamIter<FREE_MAJOR_CIV> itSecond; itSecond.hasNext(); ++itSecond)
			{
				if (!itSecond->isFullMember(eVoteSource))
					continue;
				if (itSecond->getID() > itFirst->getID())
				{
					if (!itFirst->isDefensivePact(itSecond->getID()))
					{
						bDefensivePactWithEveryone = false;
						break;
					}
				}
			}
		}
		if (bDefensivePactWithEveryone)
			return false;
	}
	else if (GC.getInfo(kData.eVote).isForcePeace())
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);

		if(kPlayer.isAVassal()) // advc
			return false;
		//if (!kPlayer.isFullMember(eVoteSource))
		// kekm.25: 'These are not necessarily the same.'
		if (!GET_TEAM(kPlayer.getTeam()).isFullMember(eVoteSource))
			return false;

		bool bValid = false;
		// advc.178: Exclude vassals
		for (TeamIter<FREE_MAJOR_CIV,ENEMY_OF> it(kPlayer.getTeam()); it.hasNext(); ++it)
		{
			if (it->isVotingMember(eVoteSource))
			{
				bValid = true;
				break;
			}
		}

		if (!bValid)
			return false;
	}
	else if (GC.getInfo(kData.eVote).isForceNoTrade())
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);
		//if (kPlayer.isFullMember(eVoteSource))
		// kekm.25: 'These are not necessarily the same.'
		if (GET_TEAM(kPlayer.getTeam()).isFullMember(eVoteSource))
			return false;

		bool bNoTradeWithEveryone = true;
		for (int iPlayer2 = 0; iPlayer2 < MAX_CIV_PLAYERS; ++iPlayer2)
		{
			CvPlayer& kPlayer2 = GET_PLAYER((PlayerTypes)iPlayer2);
			if (kPlayer2.getTeam() != kPlayer.getTeam())
			{
				//if (kPlayer2.isFullMember(eVoteSource))
				// kekm.25: 'These are not necessarily the same.'
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
			return false;
	}
	else if (GC.getInfo(kData.eVote).isForceWar())
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);
		CvTeam& kTeam = GET_TEAM(kPlayer.getTeam());

		if (kTeam.isAVassal())
			return false;
		//if (kPlayer.isFullMember(eVoteSource))
		// kekm.25: 'These are not necessarily the same.'
		if (GET_TEAM(kPlayer.getTeam()).isFullMember(eVoteSource))
			return false;

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
			return false;

		//if (!kPlayer.isVotingMember(eVoteSource))
		// kekm.25: Replacing the above
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
				return false;
		}
	}
	else if (GC.getInfo(kData.eVote).isAssignCity())
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);
		//if (kPlayer.isFullMember(eVoteSource) || !kPlayer.isVotingMember(eVoteSource))
		// kekm.25: 'These are not necessarily the same'
		if (GET_TEAM(kPlayer.getTeam()).isFullMember(eVoteSource) || !GET_TEAM(kPlayer.getTeam()).isVotingMember(eVoteSource))
			return false;

		CvCity* pCity = kPlayer.getCity(kData.iCityId);
		if (pCity == NULL)
		{
			FAssert(pCity != NULL);
			return false;
		}

		if (kData.eOtherPlayer == NO_PLAYER)
			return false;

		CvPlayer& kOtherPlayer = GET_PLAYER(kData.eOtherPlayer);
		if (kOtherPlayer.getTeam() == kPlayer.getTeam())
			return false;

		if (atWar(kPlayer.getTeam(), TEAMID(kData.eOtherPlayer)))
			return false;

		//if (!kOtherPlayer.isFullMember(eVoteSource))
		// kekm.25: 'These are not necessarily the same'
		if (!GET_TEAM(kOtherPlayer.getTeam()).isFullMember(eVoteSource))
			return false;

		if (kOtherPlayer.isHuman() && isOption(GAMEOPTION_ONE_CITY_CHALLENGE))
			return false;
	}

	if (!canDoResolution(eVoteSource, kData))
		return false;

	return true;
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
	/*	advc.001w (note): updateVisibility does this more selectively
		-- too selectively, unfortunately, in the special case of foreign spies. */
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
		gDLL->getEngineIFace()->PushFogOfWar(FOGOFWARMODE_OFF);
	else gDLL->getEngineIFace()->PopFogOfWar();

	gDLL->getEngineIFace()->setFogOfWarFromStack();
	// <advc.045>
	// (Seems to add a noticeable delay; not worth it I think.)
	/*for (PlayerIter<ALIVE,NOT_SAME_TEAM_AS> itPlayer(getActiveTeam());
		itPlayer.hasNext(); ++itPlayer)
	{
		FOR_EACH_CITY_VAR(pCity, *itPlayer)
			pCity->setLayoutDirty(true);
	}*/ // </advc.045>
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

// advc.135c:
bool CvGame::isDebugToolsAllowed(bool bWB) const
{
	if(gDLL->getInterfaceIFace()->isInAdvancedStart())
		return false;
	if(gDLL->GetWorldBuilderMode())
		return true;
	if(isGameMultiPlayer())
	{
		if(!GC.getDefineBOOL(CvGlobals::ENABLE_DEBUG_TOOLS_MULTIPLAYER))
			return false;
		if(isHotSeat())
			return true;
		return isGameNameEnableDebugTools(GC.getInitCore().getGameName());
	}
	if(bWB)
	{
		// Cut and pasted from canDoControl (CvGameInterface.cpp)
		return GC.getInitCore().getAdminPassword().empty();
	}
	return gDLL->getChtLvl() > 0;
}

// advc.135c:
bool CvGame::isGameNameEnableDebugTools(CvWString const& kGameName) const
{
	return (kGameName.compare(L"chipotle") == 0);
}


int CvGame::getPitbossTurnTime() const
{
	return GC.getInitCore().getPitbossTurnTime();
}


void CvGame::setPitbossTurnTime(int iHours)
{
	GC.getInitCore().setPitbossTurnTime(iHours);
}


bool CvGame::isSimultaneousTeamTurns() const
{
	if (!isNetworkMultiPlayer())
		return false;

	if (isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
		return false;

	return true;
}


void CvGame::setFinalInitialized(bool bNewValue)
{
	PROFILE_FUNC();

	if (isFinalInitialized() == bNewValue)
		return; // advc
	m_bFinalInitialized = bNewValue;
	if (!isFinalInitialized())
		return;

	updatePlotGroups();
	GC.getMap().updateIrrigated();
	// <advc.158> Replacing K-Mod code. If spawned later, CvPlayer::initInGame will handle it.
	for (TeamIter<ALIVE> itTeam; itTeam.hasNext(); ++itTeam)
	{
		itTeam->AI_strengthMemory().init(
				GC.getMap().numPlots(), itTeam->getID());
	} // </advc.158>
	for (TeamIter<ALIVE> itTeam; itTeam.hasNext(); ++itTeam)
		itTeam->AI_updateAreaStrategies();
}

// <advc.004x>
void CvGame::setDawnOfManShown(bool b)
{
	m_bDoMShown = b;
}


bool CvGame::isAboutToShowDawnOfMan() const
{
	return (!m_bDoMShown && getElapsedGameTurns() <= 0);
} // </advc.004x>

// <advc.061>
void CvGame::setScreenDimensions(int x, int y)
{
	m_iScreenWidth = x;
	m_iScreenHeight = y;
}

int CvGame::getScreenWidth() const
{
	return m_iScreenWidth;
}

int CvGame::getScreenHeight() const
{
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
		return;

	if (!isPlayerOptionsSent() || bForce)
	{
		m_bPlayerOptionsSent = true;
		for (int iI = 0; iI < NUM_PLAYEROPTION_TYPES; iI++)
		{
			gDLL->sendPlayerOption(((PlayerOptionTypes)iI), gDLL->getPlayerOption((PlayerOptionTypes)iI));
		}
	}
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
				sendPlayerOptions(true);
		}
		updateActiveVisibility(); // advc.706: Moved into subroutine
	}
}

// advc.706: Cut and pasted from CvGame::setActivePlayer
void CvGame::updateActiveVisibility()
{
	if(!GC.IsGraphicsInitialized())
		return;
	/*  <advc.001> Moved up - clear selection lists before
		updating the center unit in updateVisibility */
	gDLL->getInterfaceIFace()->clearSelectedCities();
	gDLL->getInterfaceIFace()->clearSelectionList(); // </advc.001>
	GC.getMap().updateFog();
	GC.getMap().updateVisibility();
	GC.getMap().updateSymbols();
	GC.getMap().updateMinimapColor();

	updateUnitEnemyGlow();
	gDLL->getInterfaceIFace()->setEndTurnMessage(false);

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
			continue; // advc.opt
		FOR_EACH_UNIT_VAR(pLoopUnit, kPlayer)
			gDLL->getEntityIFace()->updateEnemyGlow(pLoopUnit->getEntity());
	}
}

void CvGame::setHandicapType(HandicapTypes eHandicap)
{
	m_eHandicap = eHandicap;
}

// advc.250: This was originally a one-liner in CvUtils.py
int CvGame::getDifficultyForEndScore() const
{
	CvHandicapInfo const& kGameHandicap = GC.getInfo(getHandicapType());
	scaled r = kGameHandicap.getDifficulty();
	// <advc.084>
	if (getCivTeamsEverAlive() == 1)
		r /= 4; // </advc.084>
	if(isOption(GAMEOPTION_ONE_CITY_CHALLENGE))
		r += 30;
	if(!isOption(GAMEOPTION_SPAH))
		return r.round();
	std::vector<int> aiStartPointDistrib;
	m_pSpah->distribution(aiStartPointDistrib);
	std::vector<scaled> arDistr;
	for(size_t i = 0; i < aiStartPointDistrib.size(); i++)
		arDistr.push_back(aiStartPointDistrib[i]);
	return (r + (stats::max(arDistr) + stats::mean(arDistr)) /
			kGameHandicap.getAIAdvancedStartPercent()).round();
}

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
		return 1;
	return std::max(1, GC.getInfo(getBestLandUnit()).getCombat());
}


void CvGame::setBestLandUnit(UnitTypes eNewValue)
{
	if (getBestLandUnit() != eNewValue)
	{
		m_eBestLandUnit = eNewValue;

		gDLL->getInterfaceIFace()->setDirty(UnitInfo_DIRTY_BIT, true);
	}
}


void CvGame::setWinner(TeamTypes eNewWinner, VictoryTypes eNewVictory)
{
	CvWString szBuffer;

	if (getWinner() != eNewWinner || getVictory() != eNewVictory)
	{
		m_eWinner = eNewWinner;
		m_eVictory = eNewVictory;
		// advc.707: Handled by RiseFall::prepareForExtendedGame
		if (!isOption(GAMEOPTION_RISE_FALL))
		{
			// AI_AUTO_PLAY_MOD, 07/09/08, jdog5000:
			CvEventReporter::getInstance().victory(eNewWinner, eNewVictory);
		}
		if (getVictory() != NO_VICTORY)
		{
			if (getWinner() != NO_TEAM)
			{
				szBuffer = gDLL->getText("TXT_KEY_GAME_WON",
						GET_TEAM(getWinner()).getReplayName().GetCString(),
						GC.getInfo(getVictory()).getTextKeyWide());
				addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, GET_TEAM(getWinner()).getLeaderID(), szBuffer,
						-1, -1, GC.getColorType("HIGHLIGHT_TEXT"));
			}
			if ((getAIAutoPlay() > 0 || gDLL->GetAutorun()) &&
				!isOption(GAMEOPTION_RISE_FALL)) // advc.707
			{
				setGameState(GAMESTATE_EXTENDED);
			}
			else setGameState(GAMESTATE_OVER);
		}

		gDLL->UI().setDirty(Center_DIRTY_BIT, true);
		// AI_AUTO_PLAY_MOD, 07/09/08, jdog5000 (commented out)
		//CvEventReporter::getInstance().victory(eNewWinner, eNewVictory);
		gDLL->UI().setDirty(Soundtrack_DIRTY_BIT, true);
	}
}


void CvGame::setGameState(GameStateTypes eNewValue)
{
	if (getGameState() == eNewValue)
		return; // advc

	m_eGameState = eNewValue;
	if (eNewValue == GAMESTATE_OVER)
	{
		CvEventReporter::getInstance().gameEnd();
		// BULL - AutoSave - start
		if (BUGOption::isEnabled("AutoSave__CreateEndSave", false))
			GC.getPythonCaller()->call("gameEndSave", PYCivModule);
		// BULL - AutoSave - end
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
	gDLL->UI().setDirty(Cursor_DIRTY_BIT, true);
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

void CvGame::setOption(GameOptionTypes eIndex, bool bEnabled)
{
	GC.getInitCore().setOption(eIndex, bEnabled);
}


void CvGame::setMPOption(MultiplayerOptionTypes eIndex, bool bEnabled)
{
	GC.getInitCore().setMPOption(eIndex, bEnabled);
}


void CvGame::setForceControl(ForceControlTypes eIndex, bool bEnabled)
{
	GC.getInitCore().setForceControl(eIndex, bEnabled);
}

// advc: Mostly cut from CvPlayer::canConstruct
bool CvGame::canConstruct(BuildingTypes eBuilding, bool bIgnoreCost, bool bTestVisible) const
{
	CvBuildingInfo const& kBuilding = GC.getInfo(eBuilding);

	if(!bIgnoreCost && kBuilding.getProductionCost() == -1)
		return false;

	if(getCivTeamsEverAlive() < kBuilding.getNumTeamsPrereq())
		return false;
	{
		VictoryTypes ePrereqVict = kBuilding.getVictoryPrereq();
		if(ePrereqVict != NO_VICTORY && !isVictoryValid(ePrereqVict))
			return false;
	}
	{
		int iMaxStartEra = kBuilding.getMaxStartEra();
		if(iMaxStartEra != NO_ERA && getStartEra() > iMaxStartEra)
			return false;
	}
	if(isBuildingClassMaxedOut(kBuilding.getBuildingClassType()))
		return false;

	if(bTestVisible)
		return true;

	if(isNoNukes() && kBuilding.isAllowsNukes())
	{
		return false;
		// What the original code did:
		/*for(int i = 0; i < GC.getNumUnitInfos(); i++) {
			if (GC.getInfo((UnitTypes)i).getNukeRange() != -1)
				return false;
		}*/
	}
	{
		SpecialBuildingTypes eSpecial = GC.getInfo(eBuilding).getSpecialBuildingType();
		if(eSpecial != NO_SPECIALBUILDING && !isSpecialBuildingValid(eSpecial))
			return false;
	}
	if(getNumCities() < GC.getInfo(eBuilding).getNumCitiesPrereq())
		return false;
	{
		CorporationTypes eFoundCorp = kBuilding.getFoundsCorporation();
		if (eFoundCorp != NO_CORPORATION && isCorporationFounded(eFoundCorp))
			return false;
	}
	return true;
}

// advc: Cut from CvPlayer::canTrain
bool CvGame::canTrain(UnitTypes eUnit, bool bIgnoreCost, bool bTestVisible) const
{
	CvUnitInfo const& kUnit = GC.getInfo(eUnit);

	if (!bIgnoreCost && kUnit.getProductionCost() == -1)
		return false;

	if (isOption(GAMEOPTION_NO_ESPIONAGE) && (kUnit.isSpy() || kUnit.getEspionagePoints() > 0))
		return false;

	if (isUnitClassMaxedOut(kUnit.getUnitClassType()))
		return false;

	if (bTestVisible)
		return true;

	if ((isNoNukes() || !isNukesValid()) && kUnit.getNukeRange() != -1)
		return false;

	SpecialUnitTypes eSpecialUnit = kUnit.getSpecialUnitType();
	if (eSpecialUnit != NO_SPECIALUNIT && !isSpecialUnitValid(eSpecialUnit))
		return false;

	return true;
}

int CvGame::getUnitCreatedCount(UnitTypes eIndex) const
{
	FAssertBounds(0, GC.getNumUnitInfos(), eIndex);
	return m_paiUnitCreatedCount[eIndex];
}


void CvGame::incrementUnitCreatedCount(UnitTypes eIndex)
{
	FAssertBounds(0, GC.getNumUnitInfos(), eIndex);
	m_paiUnitCreatedCount[eIndex]++;
}


int CvGame::getUnitClassCreatedCount(UnitClassTypes eIndex) const
{
	FAssertBounds(0, GC.getNumUnitInfos(), eIndex);
	return m_paiUnitClassCreatedCount[eIndex];
}


bool CvGame::isUnitClassMaxedOut(UnitClassTypes eIndex, int iExtra) const
{
	FAssertBounds(0, GC.getNumUnitClassInfos(), eIndex);
	if (!GC.getInfo(eIndex).isWorldUnit())
		return false;
	FAssertBounds(0, GC.getInfo(eIndex).getMaxGlobalInstances() + 1, getUnitClassCreatedCount(eIndex));
	return (getUnitClassCreatedCount(eIndex) + iExtra >= GC.getInfo(eIndex).getMaxGlobalInstances());
}


void CvGame::incrementUnitClassCreatedCount(UnitClassTypes eIndex)
{
	FAssertBounds(0, GC.getNumUnitClassInfos(), eIndex);
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

	if (!GC.getInfo(eIndex).isWorldWonder())
		return false;

	FAssert(getBuildingClassCreatedCount(eIndex) <= GC.getInfo(eIndex).getMaxGlobalInstances());

	return ((getBuildingClassCreatedCount(eIndex) + iExtra) >= GC.getInfo(eIndex).getMaxGlobalInstances());
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

	if (!GC.getInfo(eIndex).isWorldProject())
		return false;

	FAssertMsg(getProjectCreatedCount(eIndex) <= GC.getInfo(eIndex).getMaxGlobalInstances(), "Index is expected to be within maximum bounds (invalid Index)");

	return ((getProjectCreatedCount(eIndex) + iExtra) >= GC.getInfo(eIndex).getMaxGlobalInstances());
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
	FOR_EACH_ENUM(Civic)
	{
		if (GC.getInfo(eLoopCivic).getCivicOptionType() == eCivicOption)
		{
			if (isForceCivic(eLoopCivic))
				return true;
		}
	}
	return false;
}

/*	advc: Moved from CvGameCoreUtils; renamed from "getWorldSizeMaxConscript".
	(A few CvGame functions that call CvMap::getWorldSize should perhaps be
	moved to CvMap - or perhaps a new class CvWorld should be created so that
	CvMap can deal primarily with plot-related functions.) */
int CvGame::getMaxConscript(CivicTypes eCivic) const
{
	int iMaxConscript = GC.getInfo(eCivic).getMaxConscript();
	iMaxConscript *= std::max(0, GC.getInfo(GC.getMap().getWorldSize()).
			getMaxConscriptModifier() + 100);
	iMaxConscript /= 100;
	return iMaxConscript;
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
		verifyCivics();
}


PlayerVoteTypes CvGame::getVoteOutcome(VoteTypes eIndex) const
{
	return m_aiVoteOutcome.get(eIndex);
}


bool CvGame::isVotePassed(VoteTypes eIndex) const
{
	PlayerVoteTypes ePlayerVote = getVoteOutcome(eIndex);

	if (isTeamVote(eIndex))
		return (ePlayerVote >= 0 && ePlayerVote < MAX_CIV_TEAMS);
	return (ePlayerVote == PLAYER_VOTE_YES);
}


void CvGame::setVoteOutcome(VoteTriggeredData const& kData, PlayerVoteTypes eNewValue)
{
	VoteTypes eVote = kData.kVoteOption.eVote;
	FAssertEnumBounds(eVote);
	if (getVoteOutcome(eVote) != eNewValue)
	{
		bool bOldPassed = isVotePassed(eVote);
		m_aiVoteOutcome.set(eVote, eNewValue);
		if (bOldPassed != isVotePassed(eVote))
			processVote(kData, isVotePassed(eVote) ? 1 : -1);
	}
	for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		itPlayer->setVote(kData.getID(), NO_PLAYER_VOTE);
	}
}


int CvGame::getReligionGameTurnFounded(ReligionTypes eIndex) const
{
	FAssertBounds(0, GC.getNumReligionInfos(), eIndex);
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
	// <advc.096> Update the tech-religion icons now shown on the tech tree
	if (bTaken && !isReligionSlotTaken(eReligion))
	{	/*	advc.001: This might be a minor bugfix -- tech-religion icons have
			always been used on the main interface. */
		gDLL->getInterfaceIFace()->setDirty(ResearchButtons_DIRTY_BIT, true);
		/*	Disabled this again - don't use tech-religion icons there.
			Not worth having to redraw the whole tech tree. */
		//gDLL->getInterfaceIFace()->setDirty(Tech_Screen_DIRTY_BIT, true);
	} // </advc.096>
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


void CvGame::setVictoryValid(VictoryTypes eVict, bool bValid)
{
	GC.getInitCore().setVictory(eVict, bValid);
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
			CvWString szBuffer = gDLL->getText("TXT_KEY_SPECIAL_BUILDING_VALID",
					GC.getInfo(eIndex).getTextKeyWide());
			for (int iI = 0; iI < MAX_PLAYERS; iI++)
			{
				if (GET_PLAYER((PlayerTypes)iI).isAlive())
				{
					gDLL->UI().addMessage((PlayerTypes)iI, false, -1, szBuffer,
							"AS2D_PROJECT_COMPLETED", MESSAGE_TYPE_MAJOR_EVENT, NULL,
							GC.getColorType("HIGHLIGHT_TEXT"));
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
	if (pVoteSelectionData != NULL)
		addVoteTriggered(*pVoteSelectionData, iSelection);
	deleteVoteSelection(iVoteId);
}


CvCity* CvGame::getHolyCity(ReligionTypes eIndex) const
{
	FAssertBounds(0, GC.getNumReligionInfos(), eIndex); // K-Mod
	return getCity(m_paHolyCity[eIndex]);
}

// advc (note): Almost the same as setHeadquarters
void CvGame::setHolyCity(ReligionTypes eIndex, CvCity* pNewValue, bool bAnnounce)
{
	FAssertBounds(0, GC.getNumReligionInfos(), eIndex);

	CvCity* pOldValue = getHolyCity(eIndex);
	if(pOldValue == pNewValue)
		return;
	// religion visibility now part of espionage
	//updateCitySight(false, true);
	if (pNewValue != NULL)
		m_paHolyCity[eIndex] = pNewValue->getIDInfo();
	else m_paHolyCity[eIndex].reset();
	// religion visibility now part of espionage
	//updateCitySight(true, true);
	if (pOldValue != NULL)
	{
		pOldValue->changeReligionInfluence(eIndex, -GC.getDefineINT("HOLY_CITY_INFLUENCE"));
		pOldValue->updateReligionCommerce();
		pOldValue->setInfoDirty(true);
	}
	AI_makeAssignWorkDirty();// advc: Was done at the very end; hope it's OK up here.
	if (getHolyCity(eIndex) == NULL)
		return;

	CvCity* pHolyCity = getHolyCity(eIndex);
	pHolyCity->setHasReligion(eIndex, true, bAnnounce, true);
	pHolyCity->changeReligionInfluence(eIndex, GC.getDefineINT("HOLY_CITY_INFLUENCE"));
	pHolyCity->updateReligionCommerce();
	pHolyCity->setInfoDirty(true);
	if (!bAnnounce || !isFinalInitialized() || gDLL->GetWorldBuilderMode())
		return;

	CvWString szMsgRevealed(gDLL->getText("TXT_KEY_MISC_REL_FOUNDED",
			GC.getInfo(eIndex).getTextKeyWide(), pHolyCity->getNameKey()));
	CvWString szMsgUnknown(gDLL->getText("TXT_KEY_MISC_REL_FOUNDED_UNKNOWN",
			GC.getInfo(eIndex).getTextKeyWide()));
	addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, pHolyCity->getOwner(),
			szMsgRevealed, pHolyCity->getX(), pHolyCity->getY());
			// advc.106: Reserve this color for treaties
			//(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayer const& kObs = GET_PLAYER((PlayerTypes)i);
		if (!kObs.isAlive())
			continue;
		bool bRevealed = (pHolyCity->isRevealed(kObs.getTeam()) ||
				kObs.isSpectator()); // advc.127
		gDLL->UI().addMessage(kObs.getID(), false,
				-1, // advc.106: was MESSAGE_TIME_LONG
				bRevealed ? szMsgRevealed : szMsgUnknown,
				GC.getInfo(eIndex).getSound(), MESSAGE_TYPE_MAJOR_EVENT,
				GC.getInfo(eIndex).getButton(),
				GC.getColorType("HIGHLIGHT_TEXT"),
				bRevealed ? pHolyCity->getX() : -1, bRevealed ? pHolyCity->getY() : -1,
				false, bRevealed);
	}
}


CvCity* CvGame::getHeadquarters(CorporationTypes eIndex) const
{
	FAssertBounds(0, GC.getNumCorporationInfos(), eIndex);
	return getCity(m_paHeadquarters[eIndex]);
}

// advc (note): Almost the same as setHolyCity
void CvGame::setHeadquarters(CorporationTypes eIndex, CvCity* pNewValue, bool bAnnounce)
{
	FAssertBounds(0, GC.getNumCorporationInfos(), eIndex);

	CvCity* pOldValue = getHeadquarters(eIndex);
	if (pOldValue == pNewValue)
		return;

	if (pNewValue != NULL)
		m_paHeadquarters[eIndex] = pNewValue->getIDInfo();
	else m_paHeadquarters[eIndex].reset();

	if (pOldValue != NULL)
	{
		pOldValue->updateCorporation();
		pOldValue->setInfoDirty(true);
	}
	AI_makeAssignWorkDirty(); // advc: Moved up; see setHolyCity.
	CvCity* pHeadquarters = getHeadquarters(eIndex);
	if (pHeadquarters == NULL)
		return;

	pHeadquarters->setHasCorporation(eIndex, true, bAnnounce);
	pHeadquarters->updateCorporation();
	pHeadquarters->setInfoDirty(true);
	if (!bAnnounce || !isFinalInitialized() || gDLL->GetWorldBuilderMode())
		return;

	CvWString szMsgRevealed(gDLL->getText("TXT_KEY_MISC_CORPORATION_FOUNDED",
			GC.getInfo(eIndex).getTextKeyWide(), pHeadquarters->getNameKey()));
	CvWString szMsgUnknown(gDLL->getText("TXT_KEY_MISC_CORPORATION_FOUNDED_UNKNOWN",
			GC.getInfo(eIndex).getTextKeyWide()));
	addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, pHeadquarters->getOwner(),
			szMsgRevealed, pHeadquarters->getX(), pHeadquarters->getY());
			// advc.106: Reserve this color for treaties
			//(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayer const& kObs = GET_PLAYER((PlayerTypes)i);
		if (!kObs.isAlive())
			continue;
		bool bRevealed = (pHeadquarters->isRevealed(kObs.getTeam()) ||
				kObs.isSpectator()); // advc.127
		gDLL->UI().addMessage(kObs.getID(), false,
				-1, // advc.106: was MESASAGE_TIME_LONG
				bRevealed ? szMsgRevealed : szMsgUnknown,
				GC.getInfo(eIndex).getSound(), MESSAGE_TYPE_MAJOR_EVENT,
				GC.getInfo(eIndex).getButton(),
				GC.getColorType("HIGHLIGHT_TEXT"),
				bRevealed ? pHeadquarters->getX() : -1, bRevealed ? pHeadquarters->getY() : -1,
				false, bRevealed);
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
		CvVoteInfo& kVote = GC.getInfo(pTriggeredData->kVoteOption.eVote);
		if (kVote.isAssignCity())
		{
			FAssert(pTriggeredData->kVoteOption.ePlayer != NO_PLAYER);
			CvPlayerAI& kCityPlayer = GET_PLAYER(pTriggeredData->kVoteOption.ePlayer);

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
					CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
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

CvWString const& CvGame::getName()
{
	return GC.getInitCore().getGameName();
}


void CvGame::setName(TCHAR const* szName)
{
	GC.getInitCore().setGameName(szName);
}


bool CvGame::isDestroyedCityName(CvWString& szName) const
{
	std::vector<CvWString>::const_iterator it;

	for (it = m_aszDestroyedCities.begin(); it != m_aszDestroyedCities.end(); ++it)
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

	for (it = m_aszGreatPeopleBorn.begin(); it != m_aszGreatPeopleBorn.end(); ++it)
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

	/*for (iI = 0; iI < MAX_TEAMS; iI++) {
		if (GET_TEAM((TeamTypes)iI).isAlive())
			GET_TEAM((TeamTypes)iI).doTurn();
	}*/ // BtS
	/*	Disabled by K-Mod. CvTeam::doTurn is now called at the the same time
		as CvPlayer::doTurn, to fix certain turn-order imbalances. */

	GC.getMap().doTurn();

	createBarbarianCities();
	createBarbarianUnits();

	doGlobalWarming();

	doHolyCity();
	doHeadquarters();

	gDLL->getInterfaceIFace()->setEndTurnMessage(false);
	gDLL->getInterfaceIFace()->setHasMovedUnit(false);

	CvEventReporter::getInstance().endGameTurn(getGameTurn());

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
		::shuffleArray(aiShuffle, MAX_PLAYERS, getSorenRand());
		std::set<TeamTypes> activeTeams; // K-Mod
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)aiShuffle[i]);
			if (kLoopPlayer.isAlive())
			{
				// K-Mod. call CvTeam::doTurn when the first player from each team is activated.
				if (activeTeams.insert(kLoopPlayer.getTeam()).second)
					GET_TEAM(kLoopPlayer.getTeam()).doTurn();
				// K-Mod end
				kLoopPlayer.setTurnActive(true);
			}
		}
	}
	else if (isSimultaneousTeamTurns())
	{
		for (int i = 0; i < MAX_TEAMS; i++)
		{
			CvTeam& kTeam = GET_TEAM((TeamTypes)i);
			if (kTeam.isAlive())
			{
				kTeam.setTurnActive(true);
				FAssert(getNumGameTurnActive() == kTeam.getAliveCount());
				/*	UNOFFICIAL_PATCH (bugfix), 06/10/10, snarko & jdog5000:
					Break only after first found alive player */
				break;
			}
		}
	}
	else
	{
		for (PlayerIter<ALIVE> it; it.hasNext(); ++it)
		{
			CvPlayer& kPlayer = *it;
			if (isPbem() && kPlayer.isHuman())
			{
				if (kPlayer.getID() == getActivePlayer())
				{
					// Nobody else left alive
					GC.getInitCore().setType(GAME_HOTSEAT_NEW);
					kPlayer.setTurnActive(true);
				}
				else if (!getPbemTurnSent())
					gDLL->sendPbemTurn(kPlayer.getID());
			}
			else
			{
				kPlayer.setTurnActive(true);
				FAssert(getNumGameTurnActive() == 1);
			}
			break;
		}
	}

	testVictory();

	gDLL->getEngineIFace()->SetDirty(GlobePartialTexture_DIRTY_BIT, true);
	gDLL->getEngineIFace()->DoTurn();

	PROFILE_END();

	stopProfilingDLL(true);
	// <advc.044>
	if (isMPOption(MPOPTION_SIMULTANEOUS_TURNS) || isHotSeat())
		autoSave();
	else
	{
		CvPlayer const& kActivePlayer = GET_PLAYER(getActivePlayer());
		if (!kActivePlayer.isAlive())
		{
			FAssert(kActivePlayer.isHumanDisabled());
			autoSave();
		}
	} // (Otherwise, autosave in CvPlayer::setTurnActive.)
	// </advc.044>
}

// <advc.106b>
bool CvGame::isInBetweenTurns() const
{
	return m_bInBetweenTurns;
}


void CvGame::setInBetweenTurns(bool b)
{
	m_bInBetweenTurns = b;
} // </advc.106b>


void CvGame::doDeals()
{
	verifyDeals();

	std::set<PlayerTypes> trade_players; // K-Mod. List of players involved in trades.
	FOR_EACH_DEAL_VAR(pLoopDeal)
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
		GET_PLAYER(*it).AI_updateAttitude();
	}
	// K-Mod end
}

// advc.055: For doGlobalWarming (needs to have external linkage)
namespace
{
	struct PreGWPlot
	{
		PreGWPlot(CvPlot const* pPlot, // No reference b/c need copy-ctor
			TerrainTypes eTerrain, FeatureTypes eFeature, ImprovementTypes eImprovement)
		:	pPlot(pPlot),
			eTerrain(eTerrain), eFeature(eFeature), eImprovement(eImprovement)
		{}
		CvPlot const* pPlot;
		TerrainTypes eTerrain;
		FeatureTypes eFeature;
		ImprovementTypes eImprovement;
	};
}

/*  K-Mod, 5/dec/10, karadoc
	complete rewrite of global warming, using some features from 'GWMod' by M.A. */
void CvGame::doGlobalWarming()
{
	PROFILE_FUNC();

	// Calculate change in GW index
	int iGlobalWarmingValue = calculateGlobalPollution();
	int iGlobalWarmingDefense = calculateGwSustainabilityThreshold(); // Natural global defence
	iGlobalWarmingDefense += calculateGwLandDefence(); // defence from features (forests & jungles)
	changeGlobalWarmingIndex(iGlobalWarmingValue - iGlobalWarmingDefense);

	// check if GW has 'activated'.
	if (getGwEventTally() < 0 && getGlobalWarmingIndex() > 0)
	{
		setGwEventTally(0);

		// Send a message saying that the threshold has been passed
		CvWString szBuffer;

		szBuffer = gDLL->getText("TXT_KEY_MISC_GLOBAL_WARMING_ACTIVE");
		// add the message to the replay
		addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, NO_PLAYER, szBuffer,
				-1, -1, GC.getColorType("HIGHLIGHT_TEXT"));

		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			if (GET_PLAYER((PlayerTypes)i).isAlive())
			{
				gDLL->UI().addMessage((PlayerTypes)i, false, -1, szBuffer,
						"AS2D_GLOBALWARMING", MESSAGE_TYPE_MAJOR_EVENT, NULL,
						GC.getColorType("HIGHLIGHT_TEXT"));
			}

			// Tell human players that the threshold has been reached
			if (GET_PLAYER((PlayerTypes)i).isHuman() && !isNetworkMultiPlayer())
			{
				CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_TEXT);
				if (pInfo != NULL)
				{
					pInfo->setText(gDLL->getText("TXT_KEY_POPUP_ENVIRONMENTAL_ADVISOR"));
					gDLL->getInterfaceIFace()->addPopup(pInfo, (PlayerTypes)i);
				}
			}
		}

	}

	int iGlobalWarmingRolls = getGlobalWarmingChances();

	// Apply the effects of GW

	// advc.opt: Can't hurt to make these static
	static TerrainTypes const eWarmingTerrain = (TerrainTypes)GC.getDefineINT("GLOBAL_WARMING_TERRAIN");
	static TerrainTypes const eFrozenTerrain = (TerrainTypes)GC.getDefineINT("FROZEN_TERRAIN");
	static TerrainTypes const eColdTerrain = (TerrainTypes)GC.getDefineINT("COLD_TERRAIN");
	static TerrainTypes const eTemperateTerrain = (TerrainTypes)GC.getDefineINT("TEMPERATE_TERRAIN");
	static TerrainTypes const eDryTerrain = (TerrainTypes)GC.getDefineINT("DRY_TERRAIN");
	static TerrainTypes const eBarrenTerrain = (TerrainTypes)GC.getDefineINT("BARREN_TERRAIN");

	static FeatureTypes const eColdFeature = (FeatureTypes)GC.getDefineINT("COLD_FEATURE");
	static FeatureTypes const eTemperateFeature = (FeatureTypes)GC.getDefineINT("TEMPERATE_FEATURE");
	static FeatureTypes const eWarmFeature = (FeatureTypes)GC.getDefineINT("WARM_FEATURE");
	static FeatureTypes const eFalloutFeature = (FeatureTypes)GC.getDefineINT("NUKE_FEATURE");
	// advc.055:
	static bool const bPROTECT_FEATURE_ON_NON_DRY_TERRAIN = GC.getDefineBOOL("PROTECT_FEATURE_ON_NON_DRY_TERRAIN");

	// advc.055:
	std::vector<PreGWPlot> aChangedPlots;
	bool bSoundPlayed = false; // advc.002l
	for (int i = 0; i < iGlobalWarmingRolls; i++)
	{
		// note, warming prob out of 1000, not percent.
		int iLeftOdds = 10 * GC.getInfo(getGameSpeedType()).getVictoryDelayPercent();
		if (getSorenRandNum(iLeftOdds, "Global Warming") >= GC.getDefineINT("GLOBAL_WARMING_PROB"))
			continue;
		//CvPlot* pPlot = GC.getMap().syncRandPlot(RANDPLOT_LAND | RANDPLOT_NOT_CITY);
		/*	Global warming is no longer completely random.
			getRandGWPlot will get a weighted random plot for us to strike. */
		// advc.055: Arg was 3. The higher the value, the greater the preference for cold terrain.
		CvPlot* pPlot = getRandGWPlot(2);
		if (pPlot == NULL)
			continue;
		// <advc.055>
		TerrainTypes const eTerrain = pPlot->getTerrainType();
		FeatureTypes const eFeature = pPlot->getFeatureType();
		ImprovementTypes const eImprov = pPlot->getImprovementType();
		// Just for the announcements
		PreGWPlot preGWPlot(pPlot, eTerrain, eFeature, eImprov);
		bool bProtectFeature = false;
		CvFeatureInfo const* pProtectedFeature = NULL;
		if (eImprov != NO_IMPROVEMENT && eFeature != NO_FEATURE)
		{
			if (per100(GC.getInfo(eImprov).get(
				CvImprovementInfo::GWFeatureProtection)).bernoulliSuccess(
				getSRand(), "GWFeatureProtection"))
			{
				bProtectFeature = true;
				pProtectedFeature = &GC.getInfo(eFeature);
			}
		} // </advc.055>
		bool bChanged = false;
		// rewritten terrain changing code:
		// 1) Melt frozen terrain
		if (eFeature == eColdFeature /* advc.055: */ && !bProtectFeature)
		{
			pPlot->setFeatureType(NO_FEATURE);
			bChanged = true;
		}
		else if (eTerrain == eFrozenTerrain &&
			(!bProtectFeature || pProtectedFeature->isTerrain(eColdTerrain))) // advc.055
		{
			pPlot->setTerrainType(eColdTerrain);
			bChanged = true;
		}
		else if (eTerrain == eColdTerrain &&
			(!bProtectFeature || pProtectedFeature->isTerrain(eDryTerrain))) // advc.055
		{
			pPlot->setTerrainType(eDryTerrain); // advc.055: was eTemperateTerrain
			bChanged = true;
		}
		// 2) Forest -> Jungle
		// advc.055: Commented out
		/*else if (eFeature == eTemperateFeature) {
			pPlot->setFeatureType(eWarmFeature);
			bChanged = true;
		}*/
		// 3) Remove other features
		else if (eFeature != NO_FEATURE && eFeature != eFalloutFeature &&
			/* <advc.055> */ !bProtectFeature &&
			(!bPROTECT_FEATURE_ON_NON_DRY_TERRAIN ||
			(eFeature != eTemperateFeature && eFeature != eWarmFeature) ||
			eTerrain == eDryTerrain)) // </advc.055>
		{
			pPlot->setFeatureType(NO_FEATURE);
			bChanged = true;
		}
		// 4) Dry the terrain
		else if (eTerrain == eTemperateTerrain &&
			(!bProtectFeature || pProtectedFeature->isTerrain(eDryTerrain))) // advc.055
		{
			pPlot->setTerrainType(eDryTerrain);
			bChanged = true;
		}
		else if (eTerrain == eDryTerrain &&
			(!bProtectFeature || pProtectedFeature->isTerrain(eBarrenTerrain))) // advc.055
		{	// <advc.055> Don't desertify (very) cold dry terrain
			int iColdScore = 0;
			FOR_EACH_ADJ_PLOT(*pPlot)
			{
				int iLoopColdScore = 0;
				if (pAdj->getTerrainType() == eFrozenTerrain ||
					pAdj->getFeatureType() == eColdFeature)
				{
					iLoopColdScore += 2;
				}
				else if (pAdj->getTerrainType() == eColdTerrain)
					iLoopColdScore++;
				if (plotDistance(pPlot, pAdj) <= 1)
					iLoopColdScore *= 2;
				iColdScore += iLoopColdScore;
			}
			if (iColdScore < 3) // </advc.055>
			{
				pPlot->setTerrainType(eBarrenTerrain);
				bChanged = true;
			}
		}
		/* 5) Sink coastal desert (disabled)
		else if (eTerrain == eBarrenTerrain) {
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
			if (!pPlot->canHaveImprovement(eImprov),
				NO_BUILD, false) // kekm.9
			{
				pPlot->setImprovementType(NO_IMPROVEMENT, /* advc.055: */ true);
			}  // <advc.055>
			if (!pPlot->canHaveFeature(eFeature, true))
			{
				pPlot->setFeatureType(NO_FEATURE);
				FAssert(!bProtectFeature);
			}
			aChangedPlots.push_back(preGWPlot);
			// Do the announcements in a separate loop
			// </advc.055>
			changeGwEventTally(1);
		}
	}
	updateGwPercentAnger();
	if (getGlobalWarmingIndex() > 0)
	{
		changeGlobalWarmingIndex(-getGlobalWarmingIndex() *
				GC.getDefineINT("GLOBAL_WARMING_RESTORATION_RATE", 0)/100);
	}
	// <advc.055>
	/*	advc.706 (note): These will be shortlived INFO-type messages,
		no need to store those at AI players.  */
	for (PlayerIter<HUMAN> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		std::vector<std::pair<CvCity const*,PreGWPlot> >
				aAffectedTeamCities, aAffectedRivalCities;
		int iMelted = 0;
		for (size_t i = 0; i < aChangedPlots.size(); i++)
		{
			CvPlot const& kPlot = *aChangedPlots[i].pPlot;
			if (!kPlot.isRevealed(itPlayer->getTeam()))
				continue;
			CvCity const* pCity = GC.getMap().findCity(kPlot.getX(), kPlot.getY(),
					NO_PLAYER, NO_TEAM, false); // (as in K-Mod)
			{
				bool bNearCity = (pCity != NULL &&
						(kPlot.getRevealedOwner(itPlayer->getTeam()) != NO_PLAYER ||
						plotDistance(&kPlot, pCity->plot()) <= CITY_PLOTS_RADIUS + 1));
				if (!bNearCity && aChangedPlots[i].eFeature == eColdFeature &&
					//aChangedPlots[i].eTerrain == GC.getWATER_TERRAIN(false)
					// Better for maps that might have Antarctica as a landmass
					kPlot.isWater())
				{
					iMelted++;
					continue;
				}
			}
			if (pCity == NULL) // Can't describe the affected tile w/o a city
			{
				FAssert(itPlayer->getNumCities() == 0);
				continue;
			}
			(pCity->getTeam() == itPlayer->getTeam() ?
					aAffectedTeamCities : aAffectedRivalCities).push_back(
					std::make_pair(pCity, aChangedPlots[i]));
		}
		int const iMaxRivalCityAnnouncements = 1;
		for (int iPass = 0; iPass < 2; iPass++)
		{
			bool const bTeamCities = (iPass == 0);
			std::vector<std::pair<CvCity const*,PreGWPlot> >&
					kAffectedCities = (bTeamCities ?
					aAffectedTeamCities : aAffectedRivalCities);
			if (!bTeamCities && kAffectedCities.size() > iMaxRivalCityAnnouncements)
				continue;
			for (size_t i = 0; i < kAffectedCities.size(); i++)
			{
				CvPlot const& kPlot = *kAffectedCities[i].second.pPlot;
				CvCity const& kCity = *kAffectedCities[i].first;
				TerrainTypes const eOldTerrain = kAffectedCities[i].second.eTerrain;
				FeatureTypes const eOldFeature = kAffectedCities[i].second.eFeature;
				ImprovementTypes const eOldImprov = kAffectedCities[i].second.eImprovement;
				CvWString szBuffer = gDLL->getText(
						bTeamCities ?
						"TXT_KEY_MISC_GLOBAL_WARMING_NEAR_YOUR_CITY" :
						"TXT_KEY_MISC_GLOBAL_WARMING_NEAR_CITY",
						kCity.getNameKey());
				std::vector<CvWString> aszGWEffects;
				if (eOldTerrain != kPlot.getTerrainType())
				{
					aszGWEffects.push_back(gDLL->getText(
							"TXT_KEY_GW_TERRAIN_OR_FEATURE_CHANGED",
							GC.getInfo(eOldTerrain).getDescription(),
							GC.getInfo(kPlot.getTerrainType()).getDescription()));
				}
				FeatureTypes const eNewFeature = kPlot.getFeatureType();
				if (eOldFeature != eNewFeature)
				{
					if (eOldFeature != NO_FEATURE && eNewFeature != NO_FEATURE)
					{
						aszGWEffects.push_back(gDLL->getText(
								"TXT_KEY_GW_TERRAIN_OR_FEATURE_CHANGED",
								GC.getInfo(eOldFeature).getDescription(),
								GC.getInfo(eNewFeature).getDescription()));
					}
					else if (eNewFeature == NO_FEATURE)
					{
						aszGWEffects.push_back(gDLL->getText(
								"TXT_KEY_GW_DESTROYED",
								GC.getInfo(eOldFeature).getDescription()));
					}
					else // Can't currently happen
					{
						aszGWEffects.push_back(gDLL->getText(
								"TXT_KEY_GW_CREATED",
								GC.getInfo(eNewFeature).getDescription()));
					}
				}
				/*	Fog of war is currently not a concern because improvements
					only get destroyed when the new terrain type (not subject to
					fog of war) mandates it. */
				ImprovementTypes const eNewImprov = kPlot.getImprovementType();
				if (eOldImprov != eNewImprov && eNewImprov == NO_IMPROVEMENT)
				{
					aszGWEffects.push_back(gDLL->getText(
							"TXT_KEY_GW_DESTROYED",
							GC.getInfo(eOldImprov).getDescription()));
				}
				bool bFirst = true;
				for (size_t j = 0; j < aszGWEffects.size(); j++)
				{
					setListHelp(szBuffer, L" (", aszGWEffects[j].c_str(), L", ", bFirst);
				}
				if (!bFirst)
					szBuffer.append(L")");
				bool bPlaySound = (!bSoundPlayed && // advc.002l
						bTeamCities);
				gDLL->UI().addMessage(itPlayer->getID(),
						false, -1, szBuffer, kPlot, // (as in K-Mod)
						bPlaySound ? NULL : // advc.002l
						"AS2D_SQUISH");
				bSoundPlayed = bPlaySound; // advc.002l: Once is enough			
			}
		}
		// Too many events near rival cities to announce them separately
		if (aAffectedRivalCities.size() > iMaxRivalCityAnnouncements)
		{
			CvWString szBuffer = gDLL->getText(
					"TXT_KEY_MISC_GLOBAL_WARMING_NEAR_CITIES");
			bool bFirst = true;
			for (size_t i = 0; i < aAffectedRivalCities.size(); i++)
			{
				setListHelp(szBuffer, L" ", aAffectedRivalCities[i].first->getName(),
						L", ", bFirst);
				bFirst = false;
			}
			if (!bFirst)
				szBuffer.append(L".");
			gDLL->UI().addMessage(itPlayer->getID(), false, -1, szBuffer);
		}
		if (iMelted > 0)
		{
			CvWString szBuffer = gDLL->getText(
					"TXT_KEY_GW_MELTED",
					GC.getInfo(eColdFeature).getDescription(),
					iMelted,
					/*	It might be shallow water - but probably isn't,
						and calling it deep gets across that it's far away. */
					GC.getInfo(GC.getWATER_TERRAIN(false)).getDescription());
			gDLL->UI().addMessage(itPlayer->getID(), false, -1, szBuffer);
		}
	} // </advc.055>
}

// Choose the best plot for global warming to strike from a set of iPool random plots
CvPlot* CvGame::getRandGWPlot(int iPool)
{	// advc.opt: Can't hurt to make these static
	static const TerrainTypes eFrozenTerrain = (TerrainTypes)GC.getDefineINT("FROZEN_TERRAIN");
	static const TerrainTypes eColdTerrain = (TerrainTypes)GC.getDefineINT("COLD_TERRAIN");
	static const TerrainTypes eTemperateTerrain = (TerrainTypes)GC.getDefineINT("TEMPERATE_TERRAIN");
	static const TerrainTypes eDryTerrain = (TerrainTypes)GC.getDefineINT("DRY_TERRAIN");
	static const FeatureTypes eColdFeature = (FeatureTypes)GC.getDefineINT("COLD_FEATURE");

	/*	Currently we just choose the coldest tile;
		but I may include other tests in future versions. */
	CvPlot* pBestPlot = NULL;
	TerrainTypes eTerrain = NO_TERRAIN;
	int iBestScore = -1; // higher score means better target plot
	for (int i = 0; i < iPool; i++)
	{
		/*	I want to be able to select a water tile with ice on it;
			so I can't just exclude water completely... */
		//CvPlot* pTestPlot = GC.getMap().syncRandPlot(RANDPLOT_LAND | RANDPLOT_NOT_CITY);
		/*  advc (comment): Should arguably just create a new flag RANDPLOT_GLOBAL_WARMING
			and let syncRandPlot handle the randomized selection. */ 
		CvPlot* pTestPlot = NULL;
		/*  advc: Was < 100. If we want to be certain not to miss, then we should
			check the whole map. But a 1% failure chance is fine with me. */
		for (int j = 0; j < 25; j++)
		{
			pTestPlot = GC.getMap().syncRandPlot(RANDPLOT_NOT_CITY,
					/*	advc: iTimeout was 100 - don't need to draw that many cities
						to conclude that sth. is wrong. */
					NULL, -1, 20);
			if (pTestPlot == NULL)
			{
				FAssert(pTestPlot != NULL); // advc
				break; // give up
			}
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
		// advc: 2nd test unnecessary as I'm resetting pTestPlot in the outer loop
		if (pTestPlot == NULL/* || j == 100*/)
			continue;

		/*	if only I could do this with a switch...
			[advc: one cannot b/c the case labels need to be constant expressions] */
		int iTestScore = 0;
		if (eTerrain == eFrozenTerrain)
			iTestScore = 4;
		else if (eTerrain == eColdTerrain)
			iTestScore = 3;
		else if (eTerrain == eTemperateTerrain)
			iTestScore = 2;
		else if (eTerrain == eDryTerrain)
			iTestScore = 1;
		if (iTestScore > iBestScore)
		{
			if (iBestScore > 0 || iTestScore >= 3)
				return pTestPlot; // lets not target the ice too much...

			pBestPlot = pTestPlot;
			iBestScore = iTestScore;
		}
	}
	return pBestPlot;
} // K-Mod end


void CvGame::doHolyCity()
{
	if (GC.getPythonCaller()->doHolyCity())
		return;

	if (getElapsedGameTurns() < 5 && !isOption(GAMEOPTION_ADVANCED_START))
		return;

	int iRandOffset = getSorenRandNum(GC.getNumReligionInfos(), "Holy City religion offset");
	for (int iLoop = 0; iLoop < GC.getNumReligionInfos(); ++iLoop)
	{
		int iI = ((iLoop + iRandOffset) % GC.getNumReligionInfos());
		ReligionTypes eReligion = (ReligionTypes)iI;
		if (isReligionSlotTaken(eReligion))
			continue;

		TeamTypes eBestTeam = NO_TEAM;
		{ // scope for iBestValue
			int iBestValue = MAX_INT;
			/*  advc.001: Was MAX_TEAMS. Make sure Barbarians can't found a religion
				somehow. Adopted from Mongoose SDK ReligionMod. */
			for (int iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
			{
				CvTeam const& kTeam = GET_TEAM((TeamTypes)iJ);
				if (!kTeam.isAlive())
					continue;
				if (!kTeam.isHasTech((TechTypes)GC.getInfo(eReligion).getTechPrereq()))
					continue;
				if (kTeam.getNumCities() <= 0)
					continue;

				int iValue = getSorenRandNum(10, "Found Religion (Team)");
				for (int iK = 0; iK < GC.getNumReligionInfos(); iK++)
				{
					int iReligionCount = kTeam.getHasReligionCount((ReligionTypes)iK);
					if (iReligionCount > 0)
						iValue += iReligionCount * 20;
				}
				// advc.138:
				iValue -= religionPriority(kTeam.getID(), eReligion);
				if (iValue < iBestValue)
				{
					iBestValue = iValue;
					eBestTeam = kTeam.getID();
				}
			}
		}
		if (eBestTeam == NO_TEAM)
			continue;

		int iBestValue = MAX_INT;
		PlayerTypes eBestPlayer = NO_PLAYER;
		for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
		{
			CvPlayer const& kMember = GET_PLAYER((PlayerTypes)iJ);
			if (!kMember.isAlive() || kMember.getTeam() != eBestTeam || kMember.getNumCities() <= 0)
				continue;

			int iValue = getSorenRandNum(10, "Found Religion (Player)");
			if (!kMember.isHuman())
				iValue += 18; // advc.138: Was 10. Need some x: 15 < x < 20.
			for (int iK = 0; iK < GC.getNumReligionInfos(); iK++)
			{
				int iReligionCount = kMember.getHasReligionCount((ReligionTypes)iK);
				if (iReligionCount > 0)
					iValue += iReligionCount * 20;
			}
			iValue -= religionPriority(kMember.getID(), eReligion); // advc.138
			if (iValue < iBestValue)
			{
				iBestValue = iValue;
				eBestPlayer = kMember.getID();
			}
		}
		if (eBestPlayer == NO_PLAYER)
			continue;

		ReligionTypes eFoundReligion = eReligion;
		if (isOption(GAMEOPTION_PICK_RELIGION))
			eFoundReligion = GET_PLAYER(eBestPlayer).AI_chooseReligion();
		if (eFoundReligion != NO_RELIGION)
			GET_PLAYER(eBestPlayer).foundReligion(eFoundReligion, eReligion, false);
	}
}

// <advc.138>
int CvGame::religionPriority(TeamTypes eTeam, ReligionTypes eReligion) const {

	int iMembers = 0;
	int r = 0;
	for (int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayer const& kMember = GET_PLAYER((PlayerTypes)i);
		if(!kMember.isAlive() || kMember.getTeam() != eTeam)
			continue;
		iMembers++;
		r += religionPriority(kMember.getID(), eReligion);
	}
	if (iMembers <= 0)
		return 0;
	return r / iMembers;
}


int CvGame::religionPriority(PlayerTypes ePlayer, ReligionTypes eReligion) const
{
	int r = 0;
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	for(int i = 0; i < GC.getNumTraitInfos(); i++)
	{
		TraitTypes eNoAnarchyTrait = (TraitTypes)i;
		if (!kPlayer.hasTrait(eNoAnarchyTrait) ||
				GC.getInfo(eNoAnarchyTrait).getMaxAnarchy() != 0)
			continue;
		r += 5;
		/*  Spiritual human should be sure to get a religion (so long as
			difficulty isn't above Noble). Not quite sure if my choice of
			numbers in this function and in doHolyCity accomplishes that. */
		if (kPlayer.isHuman())
			r += 6;
		break;
	}
	r += ((100 - GC.getInfo(kPlayer.getHandicapType()).
			getStartingLocationPercent()) * 31) / 100;
	// With the pick-rel option, eReligion will change later on anyway.
	if (!isOption(GAMEOPTION_PICK_RELIGION))
	{
		/*  Not excluding human here means that choosing a leader with an early
			fav religion can make a difference in human getting a religion.
			Unexpected, as fav. religions are pretty obscure knowledge. On the
			other hand, it's a pity to assign human an arbitrary religion when
			e.g. Buddhism would fit so well for Ashoka.
			Don't use PersonalityType here; fav. religion is always a matter
			of LeaderType. */
		if (GC.getInfo(kPlayer.getLeaderType()).getFavoriteReligion() == eReligion)
			r += 6;
	}
	return r;
} // </advc.138>

/*  advc: Since none of the BtS corps have a prereq. tech, this function
	normally does nothing. It has clearly never been properly tested (I've found
	two errors while refactoring it). I'm tempted to remove it, but corporations
	that get founded through a tech could be interesting for XML modding. It remains
	mostly untested though, and shares some (duplicate) code with doHlyCity. */
void CvGame::doHeadquarters()
{
	// advc.003y: Call to nonexistent Python function "doHeadquarters" removed

	if (getElapsedGameTurns() < 5)
		return;

	FOR_EACH_ENUM2(Corporation, eCorp)
	{
		CvCorporationInfo& kCorp = GC.getInfo(eCorp);
		if (isCorporationFounded(eCorp))
			continue;

		TeamTypes eBestTeam = NO_TEAM;
		{ // Scope for iBestValue
			int iBestValue = MAX_INT;
			for (int iJ = 0; iJ < MAX_TEAMS; iJ++)
			{
				CvTeam& kTeam = GET_TEAM((TeamTypes)iJ);
				if (!kTeam.isAlive())
					continue;
				// advc (note): This is as far as execution gets in AdvCiv/BtS
				if (kCorp.getTechPrereq() == NO_TECH ||
						!kTeam.isHasTech((TechTypes)kCorp.getTechPrereq()))
					continue;
				if (kTeam.getNumCities() <= 0)
					continue;
				bool bHasBonus = false;
				for (int i = 0; i < kCorp.getNumPrereqBonuses(); i++)
				{
					if (kTeam.hasBonus(kCorp.getPrereqBonus(i)))
					{
						bHasBonus = true;
						break;
					}
				}
				if (!bHasBonus)
					continue;
				int iValue = getSorenRandNum(10, "Found Corporation (Team)");
				for (int iK = 0; iK < GC.getNumCorporationInfos(); iK++)
				{
					int iCorporationCount = //GET_PLAYER((PlayerTypes)iJ).getHasCorporationCount((CorporationTypes)iK);
							kTeam.getHasCorporationCount((CorporationTypes)iK); // advc.001: iJ isn't a player
					iValue += iCorporationCount * 20;
				}
				if (iValue < iBestValue)
				{
					iBestValue = iValue;
					eBestTeam = kTeam.getID();
				}
			} // advc.001: Team loop needs to end here
		}
		if (eBestTeam == NO_TEAM)
			continue;

		int iBestValue = MAX_INT;
		PlayerTypes eBestPlayer = NO_PLAYER;
		for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
		{
			CvPlayer& kMember = GET_PLAYER((PlayerTypes)iJ);
			if (!kMember.isAlive() || kMember.getTeam() != eBestTeam || kMember.getNumCities() <= 0)
				continue;

			bool bHasBonus = false;
			for (int i = 0; i < kCorp.getNumPrereqBonuses(); i++)
			{
				if (kMember.hasBonus(kCorp.getPrereqBonus(i)))
				{
					bHasBonus = true;
					break;
				}
			}
			if (!bHasBonus)
				continue;

			int iValue = getSorenRandNum(10, "Found Corporation (Player)");
			if (!kMember.isHuman())
				iValue += 10;
			FOR_EACH_ENUM(Corporation)
			{
				int iCorporationCount = kMember.getHasCorporationCount(eLoopCorporation);
				if (iCorporationCount > 0)
					iValue += iCorporationCount * 20;
			}
			if (iValue < iBestValue)
			{
				iBestValue = iValue;
				eBestPlayer = kMember.getID();
			}
		}
		if (eBestPlayer != NO_PLAYER)
			GET_PLAYER(eBestPlayer).foundCorporation(eCorp);
	}
}


void CvGame::doDiploVote()
{
	doVoteResults();
	doVoteSelection();
}


void CvGame::createBarbarianCities()
{
	if (getMaxCityElimination() > 0)
		return;

	if (isOption(GAMEOPTION_NO_BARBARIANS))
		return;

	if (GC.getPythonCaller()->createBarbarianCities())
		return;

	if (GC.getInfo(getCurrentEra()).isNoBarbCities())
		return;

	CvHandicapInfo const& kGameHandicap = GC.getInfo(getHandicapType());
	if (kGameHandicap.getUnownedTilesPerBarbarianCity() <= 0)
		return;

	if (getNumCivCities() < countCivPlayersAlive() * 2)
			return;

	if (getElapsedGameTurns() <= ((kGameHandicap.getBarbarianCityCreationTurnsElapsed() *
			GC.getInfo(getGameSpeedType()).getBarbPercent()) / 100) /
			std::max(getStartEra() + 1, 1))
		return;

	/* <advc.300> Create up to two cities per turn, though at most one in an
	   area settled by a civ. Moved the rest of createBarbarianCities (plural)
	   into new function createBarbarianCity (singular). */
	createBarbarianCity(false);
	// A second city at full probability is too much; try 50%.
	createBarbarianCity(true, 50);
}


void CvGame::createBarbarianCity(bool bSkipCivAreas, int iProbModifierPercent)
{
	int iCreationProb = GC.getInfo(getHandicapType()).getBarbarianCityCreationProb();
	/* No cities past Medieval, so it's either +0 (Ancient), +1 (Classical)
	   or +4 (Medieval). */
	int iEra = getCurrentEra();
	iCreationProb += iEra * iEra;
	iCreationProb *= iProbModifierPercent;
	iCreationProb /= 100;
	// Adjust creation prob to game speed
	CvGameSpeedInfo const& kSpeed = GC.getInfo(getGameSpeedType());
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
		int iTargetBarbCities = (getNumCivCities() * 5 * GC.getInfo(getHandicapType()).getBarbarianCityCreationProb()) / 100;
		int iBarbCities = GET_PLAYER(BARBARIAN_PLAYER).getNumCities();
		if (iBarbCities < iTargetBarbCities)
			iTargetCitiesMultiplier += (300 * (iTargetBarbCities - iBarbCities)) / iTargetBarbCities;

		if (isOption(GAMEOPTION_RAGING_BARBARIANS))
		{
			iTargetCitiesMultiplier *= 3;
			iTargetCitiesMultiplier /= 2;
		}
	}

	CitySiteEvaluator citySiteEval(GET_PLAYER(BARBARIAN_PLAYER),
			GC.getDefineINT("MIN_BARBARIAN_CITY_STARTING_DISTANCE"));
	/* <advc.300> Randomize penalty on short inter-city distance for more variety
	   in Barbarian settling patterns. The expected value is 8, which is also the
	   value K-Mod uses. */
	citySiteEval.discourageBarbarians(5 +
			getSorenRandNum(7, "advc.300 (discouraged range)"));
	CvMap const& m = GC.getMap();
	std::map<int,int> unownedPerArea; // Precomputed for efficiency
	FOR_EACH_AREA(pArea)
	{
		/* Plots owned by Barbarians are counted in BtS, and I count them when
		   creating units because it makes some sense that Barbarians get fewer free
		   units once they have cities, but for cities, I'm not sure.
		   Keep counting them for now. */
		std::pair<int,int> iiOwnedUnowned = pArea->countOwnedUnownedHabitableTiles();
				//a.countOwnedUnownedHabitableTiles(true);
		int iUnowned = iiOwnedUnowned.second;
		std::vector<Shelf*> shelves;
		m.getShelves(*pArea, shelves);
		for(size_t i = 0; i < shelves.size(); i++)
			iUnowned += shelves[i]->countUnownedPlots() / 2;
		unownedPerArea.insert(std::make_pair(pArea->getID(), iUnowned));
	}
	bool bRage = isOption(GAMEOPTION_RAGING_BARBARIANS);
	// </advc.300>

	CvPlot const* pBestPlot = NULL;
	int iBestValue = 0;
	for (int iI = 0; iI < m.numPlots(); iI++)
	{
		CvPlot& kPlot = m.getPlotByIndex(iI);
		if (kPlot.isWater() || kPlot.isVisibleToCivTeam())
			continue;
		// <advc.300>
		CvArea& a = kPlot.getArea();
		int const iAreaSz = a.getNumTiles();
		bool bCivArea = (a.getNumCities() > a.getCitiesPerPlayer(BARBARIAN_PLAYER));
		if (bSkipCivAreas && bCivArea)
			continue;
		std::map<int,int>::const_iterator unowned = unownedPerArea.find(a.getID());
		FAssert(unowned != unownedPerArea.end());
		int iTargetCities = unowned->second;
		if (bRage) // Didn't previously affect city density
		{
			iTargetCities *= 7;
			iTargetCities /= 5;
		}
		if(!bCivArea)
		{
			/*  BtS triples iTargetCities here. Want to make it era-based.
				Important that the multiplier is rather small in the first four eras
				so that civs get a chance to settle small landmasses before
				Barbarians appear there. Once there is a Barbarian city on a
				small landmass, there may not be room for another city, and a
				naval attack on a Barbarian city is difficult to execute for the AI. */
			scaled rMult = per100(50) + per100(88) * iEra;
			iTargetCities = (rMult * iTargetCities).round(); // </advc.300>
		}
		int iUnownedTilesThreshold = GC.getInfo(getHandicapType()).getUnownedTilesPerBarbarianCity();
		if (iAreaSz < iUnownedTilesThreshold / 3)
		{
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
			int iValue = citySiteEval.evaluate(kPlot);
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
				pBestPlot = &kPlot;
			}
		}
	}

	if (pBestPlot != NULL)
	{
		FAssert(iBestValue > 0); // advc.300
		GET_PLAYER(BARBARIAN_PLAYER).found(pBestPlot->getX(), pBestPlot->getY());
		logBBAI("Barbarian city created at plot %d, %d", pBestPlot->getX(), pBestPlot->getY()); // advc.300 (from MNAI)
	}
}


void CvGame::createBarbarianUnits()
{
	if(isOption(GAMEOPTION_NO_BARBARIANS))
		return;

	if (GC.getPythonCaller()->createBarbarianUnits())
		return;

	//if (GC.getInfo(getCurrentEra()).isNoBarbUnits()) ...
	bool bCreateBarbarians = isBarbarianCreationEra(); // advc.307 (checked later now)
	bool bAnimals = false;
	if (getNumCivCities() < (3 * countCivPlayersAlive()) / 2 &&
		!isOption(GAMEOPTION_ONE_CITY_CHALLENGE) &&
		/*  advc.300: No need to delay Barbarians (bAnimals=true) if they start
			slowly (PEAK_PERCENT>=35). For slow game speed settings, there is
			now a similar check in CvUnitAI::AI_barbAttackMove. */
		barbarianPeakLandRatio() < per100(35))
	{
		bAnimals = true;
	}
	// advc.300: Moved into new function
	if (getGameTurn() < getBarbarianStartTurn())
		bAnimals = true;

	if (bAnimals)
		createAnimals();
	// <advc.300>
	if(bAnimals)
		return;
	CvHandicapInfo const& kGameHandicap = GC.getInfo(getHandicapType());
	int iBaseTilesPerLandUnit = kGameHandicap.getUnownedTilesPerBarbarianUnit();
	// Divided by 10 b/c now only shelf water tiles count
	int iBaseTilesPerSeaUnit = kGameHandicap.getUnownedWaterTilesPerBarbarianUnit() / 8;
	// </advc.300>
	FOR_EACH_AREA_VAR(pLoopArea)
	{
		// <advc.300>
		CvArea& a = *pLoopArea;
		/*  For each land area, first spawn sea Barbarians for each shelf attached
			to that land area. Skip water areas entirely. Then spawn units in the
			land area. Shelves go first b/c units can now spawn in cargo;
			spawn fewer land units then. No units in unsettled areas.
			(Need to at least spawn a Barbarian city before that). */
		if (a.isWater() || a.getNumCities() == 0)
			continue;
		int iUnowned = 0, iTiles = 0;
		std::vector<Shelf*> shelves; GC.getMap().getShelves(a, shelves);
		for (size_t i = 0; i < shelves.size(); i++)
		{
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
		if (iUnownedTotal >= iTotal)
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
		if(killBarbarian(iLandUnits, iTiles,
			a.getPopulationPerPlayer(BARBARIAN_PLAYER), a, NULL))
		{
			iLandUnits--;
		}
		if(iUnownedTotal < iBaseTilesPerLandUnit / 2)
			continue;
		int iBarbCities = a.getCitiesPerPlayer(BARBARIAN_PLAYER);
		int iNeededLand = numBarbariansToCreate(iBaseTilesPerLandUnit, iTiles,
				iUnowned, iLandUnits, iBarbCities);
		for (size_t i = 0; i < shelves.size(); i++)
		{
			int iShips = shelves[i]->countBarbarians();
			if (killBarbarian(iShips, shelves[i]->size(), a.getPopulationPerPlayer(BARBARIAN_PLAYER),
				a, shelves[i]))
			{
				iShips--;
			}
			if (!bCreateBarbarians)
				continue;
			int iNeededSea = numBarbariansToCreate(iBaseTilesPerSeaUnit,
					shelves[i]->size(), shelves[i]->countUnownedPlots(), iShips);
			/* 'BETTER_BTS_AI_MOD 9/25/08 jdog5000
				Limit construction of barb ships based on player navies' */
			// advc: BBAI code deleted -- sanity check based on Barbarian cities instead:
			if (iShips > iBarbCities + 2)
				iNeededSea = 0;
			// <advc.306> Keep spawning units on ships
			if (iNeededSea <= 0 && iNeededLand <= 0 && a.getNumCivCities() > 0)
				createBarbarianUnits(1, a, shelves[i], true, true); // </advc.306>
			else iNeededLand -= createBarbarianUnits(iNeededSea, a, shelves[i],
					iNeededLand > 0); // advc.306
		}
		/*  Don't spawn Barbarian units on (or on shelves around) continents where
			civs don't outnumber Barbarians */
		int const iCivCities = a.getNumCivCities();
		int const iBarbarianCities = a.getCitiesPerPlayer(BARBARIAN_PLAYER);
		FAssert(iBarbarianCities >= 0);
		if (iCivCities > iBarbarianCities && bCreateBarbarians)
			createBarbarianUnits(iNeededLand, a, NULL);
		/*  Rest of the creation code: moved into functions numBarbariansToCreate and
			createBarbarians */
		// </advc.300>
	}
	FOR_EACH_UNIT_VAR(pLoopUnit, GET_PLAYER(BARBARIAN_PLAYER))
	{
		if (pLoopUnit->isAnimal() &&
			// advc.309: Don't cull animals where there are no civ cities
			pLoopUnit->getArea().getNumCivCities() > 0)
		{
			pLoopUnit->kill(false);
			break;
		} // <advc.300>
	}
	FOR_EACH_CITY(c, GET_PLAYER(BARBARIAN_PLAYER))
	{
		/*  Large Barb congregations are only a problem if they have nothing
			to attack */
		if(c->getArea().getNumCivCities() > 0)
			continue;
		int iUnits = c->getPlot().getNumDefenders(BARBARIAN_PLAYER);
		double prKill = (iUnits - std::max(1.5 * c->getPopulation(), 4.0)) / 4.0;
		if(::bernoulliSuccess(prKill, "advc.300 (kill_1)"))
			c->getPlot().killRandomUnit(BARBARIAN_PLAYER, DOMAIN_LAND);
	} // </advc.300>
}


void CvGame::createAnimals()
{
	if (GC.getInfo(getCurrentEra()).isNoAnimals() ||
		isOption(GAMEOPTION_NO_ANIMALS)) // advc.309
	{
		return;
	}
	CvHandicapInfo const& kGameHandicap = GC.getInfo(getHandicapType());
	if (kGameHandicap.getUnownedTilesPerGameAnimal() <= 0)
		return;

	if (getNumCivCities() < countCivPlayersAlive())
		return;

	if (getElapsedGameTurns() < 5)
		return;

	int const iMinAnimalStartingDist = GC.getDefineINT("MIN_ANIMAL_STARTING_DISTANCE"); // advc.300
	FOR_EACH_AREA(pLoopArea)
	{
		if (pLoopArea->isWater())
			continue;

		int iNeededAnimals = pLoopArea->getNumUnownedTiles() /
				kGameHandicap.getUnownedTilesPerGameAnimal();
		/*  <advc.300> Will allow animals to survive longer on landmasses w/o
			civ cities. But only want a couple of animals there. */
		if(pLoopArea->getNumCivCities() <= 0)
			iNeededAnimals /= 2; // </advc.300>
		iNeededAnimals -= pLoopArea->getUnitsPerPlayer(BARBARIAN_PLAYER);
		if (iNeededAnimals <= 0)
			continue;

		iNeededAnimals = (iNeededAnimals / 5) + 1;
		for (int iI = 0; iI < iNeededAnimals; iI++)
		{
			CvPlot* pPlot = GC.getMap().syncRandPlot((RANDPLOT_NOT_VISIBLE_TO_CIV | RANDPLOT_PASSABLE
					| RANDPLOT_WATERSOURCE), // advc.300: Also use no iTimeout (try all plots)
					pLoopArea, iMinAnimalStartingDist);
			if (pPlot == NULL)
				continue;

			UnitTypes eBestUnit = NO_UNIT;
			int iBestValue = 0;
			// advc (comment): This loop picks an animal that is suitable for pPlot
			CvCivilization const& kCiv = GET_PLAYER(BARBARIAN_PLAYER).getCivilization(); // advc.003w
			for (int i = 0; i < kCiv.getNumUnits(); i++)
			{
				UnitTypes eLoopUnit = kCiv.unitAt(i);
				CvUnitInfo const& kUnit = GC.getInfo(eLoopUnit);
				if (!kUnit.getUnitAIType(UNITAI_ANIMAL))
					continue;
				if (pPlot->isFeature() ?
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

// advc.307:
bool CvGame::isBarbarianCreationEra() const
{
	if(isOption(GAMEOPTION_NO_BARBARIANS))
		return false;
	EraTypes eCurrentEra = getCurrentEra();
	return (!GC.getInfo(eCurrentEra).isNoBarbUnits() &&
			/*  Also stop spawning when Barbarian tech falls behind too much;
				may resume once they catch up. */
			eCurrentEra <= GET_PLAYER(BARBARIAN_PLAYER).getCurrentEra() + 1);
}

// <advc.300>
int CvGame::getBarbarianStartTurn() const
{
	int iTargetElapsed = GC.getInfo(getHandicapType()).
		   getBarbarianCreationTurnsElapsed();
	iTargetElapsed *= GC.getInfo(getGameSpeedType()).getBarbPercent();
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
	{
		iStartTurn /= 2;
	}
	return iStartTurn + iTargetElapsed;
}

// Based on code originally in createBarbarianUnits, but modified beyond recognition.
int CvGame::numBarbariansToCreate(int iTilesPerUnit, int iTiles, int iUnowned,
		int iUnitsPresent, int iBarbarianCities)
{
	int const iOwned = iTiles - iUnowned;
	scaled const rPeakRatio = barbarianPeakLandRatio();
	if (iOwned == 0 || rPeakRatio == 0)
		return 0;
	scaled rDivisor = iTilesPerUnit;
	scaled rDividend;
	{
		scaled rOwnedRatio(iOwned, iTiles);
		bool bPeakReached = (rOwnedRatio >= rPeakRatio);
		if (bPeakReached)
		{
			rDivisor *= (1 - rPeakRatio);
			rDividend = iUnowned;
		}
		else
		{
			rDivisor *= rPeakRatio;
			rDividend = iOwned;
		}
	}
	/*	For Rage, reduce divisor to 60% (50% in BtS), but
		<advc.307> reduces it further based on the game era. */
	if (isOption(GAMEOPTION_RAGING_BARBARIANS))
	{
		int iCurrentEra = getCurrentEra();
		/*  Don't reduce divisor in start era (gets too tough on Classical
			and Medieval starts b/c the starting defenders are mere Archers). */
		if (iCurrentEra <= getStartEra())
			iCurrentEra = 0;
		scaled rRageMultiplier = fixp(0.6);
		rRageMultiplier.mulDiv(8 - iCurrentEra, 8);
		rDivisor *= rRageMultiplier;
		rDivisor.increaseTo(10);
	} // </advc.307>
	else rDivisor.increaseTo(14);
	scaled rTarget = rDividend / rDivisor;
	/*  Make sure that there's enough unowned land where the Barbarians
		could plausibly gather. */
	rTarget.decreaseTo(scaled(iUnowned, 6));
	static scaled const rAdjustment = 1 + per100(GC.getDefineINT(
			"BARB_ACTIVITY_ADJUSTMENT"));
	rTarget *= rAdjustment;

	int iInitialDefenders = GC.getInfo(getHandicapType()).
			getBarbarianInitialDefenders();
	scaled r = rTarget - std::max(0, iUnitsPresent
	/*  Don't count city defenders. Settled Barbarians being less aggressive makes
		sense, but cities also reduce the number of unowned tiles; that's enough.
		(Alt. idea: Subtract half the Barbarian population in this area.)
		Old Firaxis to-do comment on this subject: 'XXX eventually need to measure
		how many barbs of eBarbUnitAI we have in this area...' */
			- iBarbarianCities * std::max(0, iInitialDefenders));
	if (r < 1)
		return 0; // Avoid very small creation probabilities
	scaled rCreationRate = fixp(0.25); // the BtS rate
	// Novel: adjusted to game speed
	rCreationRate /= per100(GC.getInfo(getGameSpeedType()).getBarbPercent());
	r *= rCreationRate;
	/*  BtS always created at least one unit, but, on Marathon, this could be too fast.
		Probabilistic instead. */
	if (r < 1)
	{
		if (r.bernoulliSuccess(getSRand(), "numBarbariansToCreate"))
			return 1;
		return 0;
	}
	return r.round();
}

// Returns the number of land units spawned (possibly in cargo). The first half is new code.
int CvGame::createBarbarianUnits(int n, CvArea& a, Shelf* pShelf, bool bCargoAllowed,
	bool bOnlyCargo) // </advc.300>
{
	/* <advc.306> Spawn cargo load before ships. Otherwise, the newly placed ship
	   would always be an eligible target and too many ships would carry cargo. */
	FAssert(!bCargoAllowed || pShelf != NULL);
	FAssert(!bOnlyCargo || bCargoAllowed);
	int r = 0;
	if(bCargoAllowed)
	{
		CvUnit* pTransport = pShelf->randomBarbarianTransport();
		if (pTransport != NULL)
		{
			UnitAITypes eLoadAI = UNITAI_ATTACK;
			for (int i = 0; i < 2; i++)
			{
				UnitTypes eLoadUnit = randomBarbarianUnit(eLoadAI, a);
				if (eLoadUnit == NO_UNIT)
					break;
				CvUnit* pLoadUnit = GET_PLAYER(BARBARIAN_PLAYER).initUnit(
						eLoadUnit, pTransport->getX(), pTransport->getY(), eLoadAI);
				/*  Don't set pTransport to UNITAI_ASSAULT_SEA -- that's for
					medium-/large-scale invasions, and too laborious to adjust.
					Instead add an unload routine to CvUnitAI::barbAttackSeaMove. */
				if (pLoadUnit == NULL)
					break;
				pLoadUnit->setTransportUnit(pTransport);
				r++;
				/*  Only occasionally spawn two units at once. Prefer the natural
					way, i.e. a ship receiving a second passenger while travelling
					to its target through fog of war. (I don't think that happens
					often enough though ...) */
				if (pTransport->getCargo() > 1 || ::bernoulliSuccess(0.7, "advc.306"))
					break;
			}
		}
		if (bOnlyCargo)
			return r;
	} // </advc.306>

	for (int iI = 0; iI < n; iI++) 
	{
		// <advc.300>
		CvPlot* pPlot = NULL;
		// Reroll twice if the tile has poor yield
		for(int i = 0; i < 3; i++)
		{
			pPlot = randomBarbarianPlot(a, pShelf);
			/*  If we can't find a plot once, we won't find one in a later
				iteration either. */
			if (pPlot == NULL)
				return r;
			int iTotalYield = 0;
			FOR_EACH_ENUM(Yield)
				iTotalYield += pPlot->getYield(eLoopYield);
			// Want to re-roll flat Tundra Forest as well
			if (iTotalYield == 2 && !pPlot->isImproved())
			{
				iTotalYield = 0;
				FOR_EACH_ENUM(Yield)
				{
					iTotalYield += pPlot->calculateNatureYield(eLoopYield, NO_TEAM,
							/* bIgnoreFeature=*/true);
				}
			}
			if (iTotalYield >= 2)
				break;
		}
		UnitAITypes eUnitAI = UNITAI_ATTACK;
		if (pShelf != NULL)
			eUnitAI = UNITAI_ATTACK_SEA;
		// Original code moved into new function:
		UnitTypes eUnitType = randomBarbarianUnit(eUnitAI, a);
		if (eUnitType == NO_UNIT)
			return r;
		CvUnit* pNewUnit = GET_PLAYER(BARBARIAN_PLAYER).initUnit(eUnitType,
				pPlot->getX(), pPlot->getY(), eUnitAI);
		if (pNewUnit != NULL && !pPlot->isWater())
			r++;
		// </advc.300>
		// K-Mod. Give a combat penalty to barbarian boats.
		if (pNewUnit && pPlot->isWater() &&
				!pNewUnit->getUnitInfo().isHiddenNationality()) // kekm.12
		{	// find the "disorganized" promotion. (is there a better way to do this?)
			PromotionTypes eDisorganized = (PromotionTypes)
					GC.getInfoTypeForString("PROMOTION_DISORGANIZED", true);
			if (eDisorganized != NO_PROMOTION)
			{	// sorry, barbarians. Free boats are just too dangerous for real civilizations to defend against.
				pNewUnit->setHasPromotion(eDisorganized, true);
			}
		} // K-Mod end
	}
	return r; // advc.306
}

// <advc.300>
CvPlot* CvGame::randomBarbarianPlot(CvArea const& a, Shelf const* pShelf)
{
	RandPlotFlags const eFlags = (RANDPLOT_NOT_VISIBLE_TO_CIV |
			/*  Shelves already ensure this and one-tile islands
				can't spawn Barbarians anyway. */
			//RANDPLOT_ADJACENT_LAND |
			RANDPLOT_PASSABLE |
			RANDPLOT_HABITABLE | // new
			RANDPLOT_UNOWNED);
	/*  Added the "unowned" flag to prevent spawning in Barbarian land.
		Could otherwise happen now b/c the visible flag and dist. restriction
		no longer apply to Barbarians previously spawned; see
		CvPlot::isVisibleToCivTeam, CvMap::isCivUnitNearby. */
	static int const iDist = GC.getDefineINT("MIN_BARBARIAN_STARTING_DISTANCE");
	// <advc.304> Sometimes don't pick a plot if there are few legal plots
	int iLegal = 0;
	CvPlot* pRandPlot = NULL;
	if (pShelf == NULL)
		pRandPlot = GC.getMap().syncRandPlot(eFlags, &a, iDist, -1, &iLegal);
	else
	{
		pRandPlot = pShelf->randomPlot(eFlags, iDist, &iLegal);
		if(pRandPlot != NULL && iLegal * 100 < pShelf->size())
			pRandPlot = NULL;
	}
	if (pRandPlot != NULL && iLegal > 0 && iLegal < 4)
	{
		// Tbd.: Should perhaps be based on a.getNumTiles()
		scaled rSkipProb = 1 - scaled(1, 5 - iLegal);
		if(rSkipProb.bernoulliSuccess(getSRand(), "randomBarbarianPlot"))
		pRandPlot = NULL;
	}
	return pRandPlot; // </advc.304>
}


bool CvGame::killBarbarian(int iUnitsPresent, int iTiles, int iPop, CvArea& a, Shelf* pShelf)
{
	if (iUnitsPresent <= 5) // 5 is never a crowd
		return false;
	scaled rDivisor = std::max(1, 4 * iPop);
	if (pShelf != NULL)
		rDivisor += pShelf->size();
	else rDivisor += iTiles; /*  Includes 50% shelf (given the way this function
								is currently used). */
	// Don't want large Barbarian continents crawling with units
	rDivisor.exponentiate(fixp(0.7));
	rDivisor *= 5;
	if ((iUnitsPresent / rDivisor).bernoulliSuccess(getSRand(), "killBarbarian2"))
	{
		if(pShelf != NULL)
			return pShelf->killBarbarian();
		/*  The same method as for animal culling. Should result
			in first-in-first-out behavior; fair enough. */
		FOR_EACH_UNIT_VAR(pUnit, GET_PLAYER(BARBARIAN_PLAYER))
		{
			CvUnit& u = *pUnit;
			if (u.isAnimal() || !u.isArea(a) ||
				u.getUnitCombatType() == NO_UNITCOMBAT)
			{
				continue;
			}
			u.kill(false);
			return true;
		}
	}
	return false;
}

// Based on BtS code originally in createBarbarianUnits
UnitTypes CvGame::randomBarbarianUnit(UnitAITypes eUnitAI, CvArea const& a)
{
	bool bSea;
	switch(eUnitAI)
	{
	case UNITAI_ATTACK_SEA: bSea = true; break;
	case UNITAI_ATTACK: bSea = false; break;
	default: return NO_UNIT;
	}
	UnitTypes r = NO_UNIT;
	int iBestValue = 0;
	CvCivilization const& kCiv = GET_PLAYER(BARBARIAN_PLAYER).getCivilization(); // advc.003w
	for (int i = 0; i < kCiv.getNumUnits(); i++)
	{
		UnitTypes eUnit = kCiv.unitAt(i);
		CvUnitInfo const& kUnit = GC.getInfo(eUnit);
		DomainTypes eDomain = kUnit.getDomainType();
		if(kUnit.getCombat() <= 0 || eDomain == DOMAIN_AIR ||
			kUnit.isMostlyDefensive() || // advc.315
			(eDomain == DOMAIN_SEA) != bSea ||
			!GET_PLAYER(BARBARIAN_PLAYER).canTrain(eUnit))
		{
			continue;
		}  // <advc.301>
		BonusTypes const eAndBonus = kUnit.getPrereqAndBonus();
		std::vector<TechTypes> aeAndBonusTechs(2, NO_TECH);
		if (eAndBonus != NO_BONUS)
		{
			CvBonusInfo const& kAndBonus = GC.getInfo(eAndBonus);
			aeAndBonusTechs.push_back(kAndBonus.getTechCityTrade()); // (as in BtS)
			aeAndBonusTechs.push_back(kAndBonus.getTechReveal());
			bool bValid = true;
			for (size_t j = 0; j < aeAndBonusTechs.size(); j++)
			{
				if (aeAndBonusTechs[j] != NO_TECH &&
					!GET_TEAM(BARBARIAN_TEAM).isHasTech(aeAndBonusTechs[j]))
				{
					bValid = false;
					break;
				}
			}
			if (!bValid || !a.hasAnyAreaPlayerBonus(eAndBonus))
				continue;
		}
		/*  No units from more than 1 era ago (obsoletion too difficult to test).
			hasTech already tested by canTrain, but era shouldn't be
			tested there b/c it's OK for Barbarian cities to train outdated units
			(they only will if they can't train anything better). */
		TechTypes const eAndTech = kUnit.getPrereqAndTech();
		int iUnitEra = 0;
		if (eAndTech != NO_TECH)
			iUnitEra = GC.getInfo(eAndTech).getEra();
		for (size_t j = 0; j < aeAndBonusTechs.size(); j++)
		{
			if (aeAndBonusTechs[j] != NO_TECH)
			{
				iUnitEra = std::max<int>(iUnitEra,
						GC.getInfo(aeAndBonusTechs[j]).getEra());
			}
		}
		if (iUnitEra + 1 < getCurrentEra())
			continue; // </advc.301>
		bool bFound = false;
		bool bRequires = false;
		for (int j = 0; j < kUnit.getNumPrereqOrBonuses() &&
			!bFound; j++) // advc.301
		{
			BonusTypes const eOrBonus = kUnit.getPrereqOrBonuses(j);
			CvBonusInfo const& kOrBonus = GC.getInfo(eOrBonus);
			// <advc.301>
			std::vector<TechTypes> aeOrBonusTechs(2, NO_TECH);
			aeOrBonusTechs.push_back(kOrBonus.getTechCityTrade()); // (as in BtS)
			aeOrBonusTechs.push_back(kOrBonus.getTechReveal());
			for (size_t k = 0; k < aeOrBonusTechs.size(); k++)
			{
				if (aeOrBonusTechs[k] == NO_TECH)
					continue;
				bRequires = true;
				if (GET_TEAM(BARBARIAN_TEAM).isHasTech(aeOrBonusTechs[k]) &&
					a.hasAnyAreaPlayerBonus(eOrBonus)) // </advc.301>
				{
					bFound = true;
					break;
				}
			}
		}
		if (bRequires && !bFound)
			continue;
		int iValue = (1 + getSorenRandNum(1000, "Barb Unit Selection"));
		if (kUnit.getUnitAIType(eUnitAI))
			iValue += 200;
		if (iValue > iBestValue)
		{
			r = eUnit;
			iBestValue = iValue;
		}
	}
	return r;
}

// (See documentation in XML)
scaled CvGame::barbarianPeakLandRatio() const
{
	scaled r;
	if (isOption(GAMEOPTION_RAGING_BARBARIANS))
	{
		static scaled const rRagingRatio = per100(GC.getDefineINT(
				"BARB_RAGE_PEAK_PERCENT"));
		r = rRagingRatio;
	}
	else
	{
		static scaled const rNormalRatio = per100(GC.getDefineINT(
				"BARB_PEAK_PERCENT"));
		r = rNormalRatio;
	}
	r.clamp(0, 1);
	return r;
}
// </advc.300>

void CvGame::updateWar()
{
	if (isOption(GAMEOPTION_ALWAYS_WAR))
	{
		for (int iI = 0; iI < MAX_TEAMS; iI++)
		{
			CvTeam& kTeam1 = GET_TEAM((TeamTypes)iI);
			if (kTeam1.isAlive() && kTeam1.isHuman())
			{
				for (int iJ = 0; iJ < MAX_TEAMS; iJ++)
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
	int aiShuffle[MAX_PLAYERS];
	if (isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
		shuffleArray(aiShuffle, MAX_PLAYERS, getSorenRand());
	else
	{
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			aiShuffle[iI] = iI;
		}
	} // <advc.001y>
	int const iMaxUnitUpdateAttempts = 18;
	FAssertMsg(m_iUnitUpdateAttempts != iMaxUnitUpdateAttempts - 5, "Unit stuck in a loop");
	// </advc.001y>
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)aiShuffle[iI]);
		if (!kPlayer.isAlive() || !kPlayer.isTurnActive())
			continue;

		if (!kPlayer.isAutoMoves())
		{
			kPlayer.AI_unitUpdate();
			if (!kPlayer.isHuman() && !kPlayer.hasBusyUnit())
			{
				/*  advc.001y: Safety measure against infinite loop
					(Complementing Vanilla Civ 4 code in CvSelectionGroupAI::AI_update.
					The attempt counter there won't work when CvUnitAI::AI_update
					joins a different selection group.) */
				if (m_iUnitUpdateAttempts > iMaxUnitUpdateAttempts ||
					!kPlayer.hasReadyUnit(true))
				{
					kPlayer.setAutoMoves(true);
				}
				else m_iUnitUpdateAttempts++; // advc.001y
			}
		}
		if (kPlayer.isAutoMoves())
		{
			FOR_EACH_GROUP_VAR(pGroup, kPlayer)
				pGroup->autoMission();
			/*	K-Mod. Here's where we do the AI for automated units.
				Note, we can't do AI_update and autoMission in the same loop, because
				either one might delete the group - and thus cause the other to crash. */
			if (kPlayer.isHuman())
			{
				FOR_EACH_GROUPAI_VAR(pGroup, kPlayer)
				{
					if (pGroup->AI_update())
					{
						FAssert(kPlayer.hasBusyUnit());
						break;
					}
				}
				/*	Refresh the group cycle for human players.
					Non-human players can wait for their units to wake up, or regain moves -
					group cycle isn't very important for them anyway. */
				kPlayer.refreshGroupCycleList();
			} // K-Mod end
			if (!kPlayer.hasBusyUnit())
				kPlayer.setAutoMoves(false);
		}
	}
}


void CvGame::verifyCivics()
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).verifyCivics();
		}
	}
}


void CvGame::updateTimers()
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).updateTimers();
		}
	}
}


void CvGame::updateTurnTimer()
{
	if (!isMPOption(MPOPTION_TURN_TIMER)) // Are we using a turn timer?
		return;

	if (getElapsedGameTurns() <= 0 && isOption(GAMEOPTION_ADVANCED_START))
		return;

	if (getTurnSlice() <= getCutoffSlice()) // Has the turn expired?
		return;

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (GET_PLAYER((PlayerTypes)i).isAlive() && GET_PLAYER((PlayerTypes)i).isTurnActive())
		{
			GET_PLAYER((PlayerTypes)i).setEndTurn(true);
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

bool CvGame::testVictory(VictoryTypes eVictory, TeamTypes eTeam, bool* pbEndScore) const
{
	FAssertEnumBounds(eVictory);
	FAssertBounds(0, MAX_CIV_TEAMS, eTeam);
	FAssert(GET_TEAM(eTeam).isAlive());

	if(pbEndScore != NULL)
		*pbEndScore = false;

	if(!isVictoryValid(eVictory))
		return false;

	CvVictoryInfo const& kVictory = GC.getInfo(eVictory);
	if (kVictory.isEndScore())
	{
		if (pbEndScore)
			*pbEndScore = true;

		if (getMaxTurns() == 0 || getElapsedGameTurns() < getMaxTurns())
			return false;
	}
	if (kVictory.isTargetScore())
	{
		if (getTargetScore() == 0 || getTeamScore(eTeam) < getTargetScore())
			return false;
	}
	if (kVictory.isEndScore() || kVictory.isTargetScore())
	{
		int const iTestScore = getTeamScore(eTeam);
		for (TeamIter<CIV_ALIVE,NOT_SAME_TEAM_AS> itOther(eTeam);
			itOther.hasNext(); ++itOther)
		{
			if (getTeamScore(itOther->getID()) >= iTestScore)
				return false;
		}
	}
	CvTeam const& kTeam = GET_TEAM(eTeam);
	if (kVictory.isConquest())
	{
		if (kTeam.getNumCities() <= 0)
			return false;
		for (TeamIter<CIV_ALIVE,NOT_SAME_TEAM_AS> itOther(eTeam);
			itOther.hasNext(); ++itOther)
		{
			if (!itOther->isVassal(eTeam) && itOther->getNumCities() > 0)
				return false;
		}
	}
	if (kVictory.isDiploVote())
	{
		bool bFound = false;
		FOR_EACH_ENUM(Vote)
		{
			if (GC.getInfo(eLoopVote).isVictory() &&
				getVoteOutcome(eLoopVote) == eTeam)
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
		{
			return false;
		}
	}
	if (getAdjustedLandPercent(eVictory) > 0)
	{
		if (100 * kTeam.getTotalLand() < GC.getMap().getLandPlots() *
			getAdjustedLandPercent(eVictory))
		{
			return false;
		}
	}
	if (kVictory.getReligionPercent() > 0)
	{
		if (getNumCivCities() <= countCivPlayersAlive() * 2)
			return false;

		bool bFound = false;
		FOR_EACH_ENUM(Religion)
		{
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
		for (MemberIter itMember(eTeam); itMember.hasNext(); ++itMember)
		{
			FOR_EACH_CITY(pCity, *itMember)
			{
				if (pCity->getCultureLevel() >= kVictory.getCityCulture())
					iCount++;
			}
		}
		if (iCount < kVictory.getNumCultureCities())
			return false;
	}
	if (kVictory.getTotalCultureRatio() > 0)
	{
		int const iThreshold = (kTeam.countTotalCulture() * 100) /
				kVictory.getTotalCultureRatio();
		for (TeamIter<CIV_ALIVE,NOT_SAME_TEAM_AS> itOther(eTeam);
			itOther.hasNext(); ++itOther)
		{
			if (itOther->countTotalCulture() > iThreshold)
				return false;
		}
	}
	FOR_EACH_ENUM(BuildingClass)
	{
		if (GC.getInfo(eLoopBuildingClass).getVictoryThreshold(eVictory) >
			kTeam.getBuildingClassCount(eLoopBuildingClass))
		{
			return false;
		}
	}
	FOR_EACH_ENUM(Project)
	{
		if (GC.getInfo(eLoopProject).getVictoryMinThreshold(eVictory) >
			kTeam.getProjectCount(eLoopProject))
		{
			return false;
		}
	}
	if (!GC.getPythonCaller()->isVictory(eVictory))
		return false;

	return true;
}


void CvGame::testVictory()
{
	bool bEndScore = false;

	if (getVictory() != NO_VICTORY)
		return;

	if (getGameState() == GAMESTATE_EXTENDED)
		return;

	updateScore();
	// <advc.003y> Replace Python callback (which is now disabled by default)
	if (getElapsedGameTurns() <= 10 || !GC.getPythonCaller()->isVictoryPossible())
		return; // </advc.003y>

	std::vector<std::pair<TeamTypes,VictoryTypes> > aeeWinners; // advc: was vector<vector<int> >
	for (TeamIter<MAJOR_CIV> itTeam; itTeam.hasNext(); ++itTeam)
	{
		CvTeam& kTeam = *itTeam;
		FOR_EACH_ENUM2(Victory, eVictory)
		{
			if (testVictory(eVictory, kTeam.getID(), &bEndScore))
			{
				if (kTeam.getVictoryCountdown(eVictory) < 0)
				{
					if (kTeam.getVictoryDelay(eVictory) == 0)
						kTeam.setVictoryCountdown(eVictory, 0);
				}
				//update victory countdown
				if (kTeam.getVictoryCountdown(eVictory) > 0)
					kTeam.changeVictoryCountdown(eVictory, -1);
				if (kTeam.getVictoryCountdown(eVictory) == 0)
				{
					if (getSorenRandNum(100, "Victory Success") <
						kTeam.getLaunchSuccessRate(eVictory))
					{
						aeeWinners.push_back(std::make_pair(kTeam.getID(), eVictory));
					}
					else kTeam.resetVictoryProgress();
				}
			}
		}
	}
	if (aeeWinners.size() > 0)
	{
		int iWinner = getSorenRandNum(aeeWinners.size(), "Victory tie breaker");
		setWinner(aeeWinners[iWinner].first, aeeWinners[iWinner].second);
	}
	if (getVictory() == NO_VICTORY && !bEndScore && getMaxTurns() > 0 &&
		getElapsedGameTurns() >= getMaxTurns())
	{
		if (getAIAutoPlay() > 0 || gDLL->GetAutorun())
			setGameState(GAMESTATE_EXTENDED);
		else setGameState(GAMESTATE_OVER);
	}
}


void CvGame::processVote(const VoteTriggeredData& kData, int iChange)
{
	FAssert(iChange != 0); // advc
	CvVoteInfo const& kVote = GC.getInfo(kData.kVoteOption.eVote);
	changeTradeRoutes(kVote.getTradeRoutes() * iChange);
	if (kVote.isFreeTrade())
		changeFreeTradeCount(iChange);
	if (kVote.isNoNukes())
		changeNoNukesCount(iChange);
	FOR_EACH_ENUM(Civic)
	{
		if (kVote.isForceCivic(eLoopCivic))
			changeForceCivicCount(eLoopCivic, iChange);
	}
	if (iChange <= 0)
		return;

	if (kVote.isOpenBorders() || kVote.isDefensivePact()) // advc: Merged these two handlers
	{
		for (TeamIter<MAJOR_CIV> itFirst; itFirst.hasNext(); ++itFirst)
		{
			if (!itFirst->isVotingMember(kData.eVoteSource))
				continue;
			for (TeamIter<MAJOR_CIV> itSecond; itSecond.hasNext(); ++itSecond)
			{
				if (!itSecond->isVotingMember(kData.eVoteSource))
					continue;
				if (itSecond->getID() > itFirst->getID())
				{
					if (kVote.isOpenBorders())
						itFirst->signOpenBorders(itSecond->getID(), /* advc.032: */ true);
					if (kVote.isDefensivePact())
						itFirst->signDefensivePact(itSecond->getID(), /* advc.032: */ true);
				}
			}
		}
		setVoteOutcome(kData, NO_PLAYER_VOTE);
	}
	else if (kVote.isForcePeace())
	{
		FAssert(NO_PLAYER != kData.kVoteOption.ePlayer);
		CvPlayer& kPlayer = GET_PLAYER(kData.kVoteOption.ePlayer);
		if (gTeamLogLevel >= 1) logBBAI("  Vote for forcing peace against team %d (%S) passes", kPlayer.getTeam(), kPlayer.getCivilizationDescription(0)); // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
		// <kekm.25> 'Cancel defensive pacts with the attackers first'
		FOR_EACH_DEAL_VAR(pLoopDeal)
		{
			if ((TEAMID(pLoopDeal->getFirstPlayer()) == kPlayer.getTeam() &&
				GET_TEAM(pLoopDeal->getSecondPlayer()).
				isVotingMember(kData.eVoteSource)) ||
				(TEAMID(pLoopDeal->getSecondPlayer()) == kPlayer.getTeam() &&
				GET_TEAM(pLoopDeal->getFirstPlayer()).
				isVotingMember(kData.eVoteSource)))
			{
				FOR_EACH_TRADE_ITEM(pLoopDeal->getFirstList())
				{
					if(pItem->m_eItemType == TRADE_DEFENSIVE_PACT)
					{
						pLoopDeal->kill();
						break;
					}
				} // advc: Don't bother with SecondTrades; DPs are dual.
			}
		} // </kekm.25>
		for (PlayerIter<MAJOR_CIV,NOT_SAME_TEAM_AS> itLoopPlayer(kPlayer.getTeam());
			itLoopPlayer.hasNext(); ++itLoopPlayer)
		{
			if (!itLoopPlayer->isVotingMember(kData.eVoteSource))
				continue;
			/*	advc.130v (note): Not replaced with CvTeam::signPeaceTreaty
				because I want vassals to get their own peace treaties here. */
			itLoopPlayer->forcePeace(kData.kVoteOption.ePlayer);
		}
		setVoteOutcome(kData, NO_PLAYER_VOTE);
	}
	else if (kVote.isForceNoTrade())
	{
		FAssert(NO_PLAYER != kData.kVoteOption.ePlayer);
		CvPlayer& kPlayer = GET_PLAYER(kData.kVoteOption.ePlayer);
		for (PlayerIter<MAJOR_CIV> itLoopPlayer; itLoopPlayer.hasNext(); ++itLoopPlayer)
		{
			if (!itLoopPlayer->isVotingMember(kData.eVoteSource))
				continue;
			if (itLoopPlayer->canStopTradingWithTeam(kPlayer.getTeam()))
				itLoopPlayer->stopTradingWithTeam(kPlayer.getTeam());
		}
		setVoteOutcome(kData, NO_PLAYER_VOTE);
	}
	else if (kVote.isForceWar())
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.kVoteOption.ePlayer);
		if (gTeamLogLevel >= 1) logBBAI("  Vote for war against team %d (%S) passes", kPlayer.getTeam(), kPlayer.getCivilizationDescription(0)); // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
		CvTeamAI& kTeam = GET_TEAM(kPlayer.getTeam());
		// advc.001: Exclude dead civs
		for (PlayerIter<MAJOR_CIV,POTENTIAL_ENEMY_OF> it(kTeam.getID());
			it.hasNext(); ++it)
		{
			CvTeam& kFullMember = GET_TEAM(it->getTeam());
			// kekm.25/advc: was isVotingMember
			if (!kFullMember.isFullMember(kData.eVoteSource))
				continue;
			if (kFullMember.canChangeWarPeace(kTeam.getID()))
			{
				// <kekm.26>
				CvTeam::queueWar(kFullMember.getID(), kTeam.getID(),
						false, WARPLAN_DOGPILE); // </kekm.26>
				kTeam.AI_makeUnwillingToTalk(kFullMember.getID()); // advc.104i
			}
		}
		CvTeam::triggerWars(); // kekm.26
		setVoteOutcome(kData, NO_PLAYER_VOTE);
	}
	else if (kVote.isAssignCity())
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.kVoteOption.ePlayer);
		CvCity* pCity = kPlayer.getCity(kData.kVoteOption.iCityId);
		if (pCity != NULL &&
			kData.kVoteOption.eOtherPlayer != NO_PLAYER)
		{
			CvPlayer& kNewOwner = GET_PLAYER(kData.kVoteOption.eOtherPlayer);
			if (kNewOwner.getID() != pCity->getOwner())
			{
				if (gTeamLogLevel >= 1) logBBAI("  Vote for assigning %S to %d (%S) passes", pCity->getName().GetCString(), kNewOwner.getTeam(), kNewOwner.getCivilizationDescription(0)); // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
				kNewOwner.acquireCity(pCity, false, true, true);
			}
		}
		// advc (note): Otherwise, the city has changed owner while the votes were cast.
		setVoteOutcome(kData, NO_PLAYER_VOTE);
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

/*  advc.072: All the FAssert(false) in this function mean that we're somehow
	out of step with the iteration that happens in the EXE. */
CvDeal* CvGame::nextCurrentDeal(PlayerTypes eGivePlayer, PlayerTypes eReceivePlayer,
		TradeableItems eItemType, int iData, bool bWidget)
{
	if(!m_bShowingCurrentDeals)
	{
		m_currentDeals.clear(); // Probably not needed, but can't hurt to clear them.
		m_currentDealsWidget.clear();
		return NULL;
	}
	// Probably not needed:
	PlayerTypes eDiploPlayer = (PlayerTypes)gDLL->getDiplomacyPlayer();
	if(!((getActivePlayer() == eGivePlayer && eDiploPlayer == eReceivePlayer) ||
		(getActivePlayer() == eReceivePlayer && eDiploPlayer == eGivePlayer)))
	{
		return NULL;
	}
	CLinkList<DealItemData>& kCurrentDeals = (bWidget ? m_currentDealsWidget :
			m_currentDeals);
	if(kCurrentDeals.getLength() <= 0)
	{
		bool bFirstFound = false;
		FOR_EACH_DEAL(d)
		{
			if(!d->isBetween(eGivePlayer, eReceivePlayer))
				continue;
			CLinkList<TradeData> const& kGiveList = (d->getFirstPlayer() == eGivePlayer ?
					d->getFirstList() : d->getSecondList());
			FOR_EACH_TRADE_ITEM(kGiveList)
			{
				if(!CvDeal::isAnnual(pItem->m_eItemType) &&
					pItem->m_eItemType != TRADE_PEACE_TREATY)
				{
					break;
				}
				if(!bFirstFound)
				{
					if(pItem->m_eItemType != eItemType ||
						pItem->m_iData != iData)
					{
						FAssert(false);
						return NULL;
					}
					bFirstFound = true;
				}
				DealItemData data(eGivePlayer, eReceivePlayer, pItem->m_eItemType,
						pItem->m_iData, d->getID());
				kCurrentDeals.insertAtEnd(data);
			}
		}
	}
	if(kCurrentDeals.getLength() <= 0)
	{
		FAssert(false);
		return NULL;
	}
	CLLNode<DealItemData>* pNode = kCurrentDeals.head();
	DealItemData data = pNode->m_data;
	if(data.eGivePlayer != eGivePlayer || data.eReceivePlayer != eReceivePlayer ||
		data.iData != iData || data.eItemType != eItemType)
	{
		kCurrentDeals.clear();
		FAssert(false);
		return NULL;
	}
	CvDeal* r = getDeal(pNode->m_data.iDeal);
	kCurrentDeals.deleteNode(pNode);
	FAssert(r != NULL);
	return r;
}

// advc.027b:
std::pair<uint,uint> CvGame::getInitialRandSeed() const
{
	return std::make_pair(m_initialRandSeed.uiMap, m_initialRandSeed.uiSync);
}


int CvGame::calculateSyncChecksum()
{
	//PROFILE_FUNC(); // advc.003o
	// <advc.opt>
	if(!isNetworkMultiPlayer())
		return 0; // </advc.opt>

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
		std::vector<int> aiMultipliers;
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
					if(iI != iJ) // advc: self-attitude should never matter
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
				FOR_EACH_CITYAI(pLoopCity, kPlayer)
				{
					iMultiplier += pLoopCity->goodHealth() * 876543;
					iMultiplier += pLoopCity->badHealth() * 985310;
					iMultiplier += pLoopCity->happyLevel() * 736373;
					iMultiplier += pLoopCity->unhappyLevel() * 820622;
					iMultiplier += pLoopCity->getFood() * 367291;
					/*for(iJ = 0; iJ < MAX_PLAYERS; iJ++) {
						if(GET_PLAYER((PlayerTypes)iJ).isAlive()) {
							iMultiplier += (pLoopCity->AI_playerCloseness(
									(PlayerTypes)iJ, DEFAULT_PLAYER_CLOSENESS, true) + 1) * (iJ + 1);
						}
					}*/
					/*  <advc.001n> FloatingDefenders should be good enough as closeness
						factors into that */
					if (!kPlayer.isBarbarian())
						iMultiplier += pLoopCity->AI_neededFloatingDefenders(false, true) * 324111;
					// </advc.001n>
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
	//PROFILE_FUNC(); // advc.003o

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

// advc.001n:
bool CvGame::checkInSync()
{
	if(!isNetworkMultiPlayer())
		return true;

	int iSyncHash = gDLL->GetSyncOOS(GET_PLAYER(getActivePlayer()).getNetID());
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayer const& kOther = GET_PLAYER((PlayerTypes)i);
		if(kOther.isAlive() && kOther.getID() != getActivePlayer() &&
			(kOther.isHuman() || kOther.isHumanDisabled()))
		{
			int iOtherSyncHash = gDLL->GetSyncOOS(kOther.getNetID());
			if(iOtherSyncHash != iSyncHash)
			{
				FAssert(iOtherSyncHash == iSyncHash);
				setAIAutoPlay(0);
				return false;
			}
		}
	}
	return true;
}

/*  <advc.003g> Test the platform's floating point behavior (intermediate precision
	and/or rounding). */
int CvGame::FPChecksum() const
{
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

void CvGame::doFPCheck(int iChecksum, PlayerTypes ePlayer)
{
	if(m_bFPTestDone)
		return;
	m_bFPTestDone = true;
	if(iChecksum == FPChecksum())
		return; // Active player is able to reproduce checksum received over the net

	gDLL->UI().addMessage(getActivePlayer(), true, -1,
			CvWString::format(L"Your machine's FP test computation has yielded a"
			L" different result than that of %s. The game may frequently go"
			L" out of sync due to floating point calculations in the AdvCiv mod.",
			GET_PLAYER(ePlayer).getName()), NULL, MESSAGE_TYPE_MAJOR_EVENT, NULL,
			GC.getColorType("WARNING_TEXT"));
} // </advc.003g

// <advc.003r>
void CvGame::handleUpdateTimer(UpdateTimerTypes eTimerType)
{
	if(m_aiUpdateTimers[eTimerType] < 0)
		return;

	if(m_aiUpdateTimers[eTimerType] == 0)
	{
		switch(eTimerType)
		{
		// <advc.085>
		case UPDATE_COLLAPSE_SCORE_BOARD:
			GET_PLAYER(getActivePlayer()).setScoreboardExpanded(false);
			break;
		case UPDATE_DIRTY_SCORE_BOARD:
			gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
			break; // </advc.085>
		// <advc.001w>
		case UPDATE_MOUSE_FOCUS:
			gDLL->getInterfaceIFace()->makeSelectionListDirty();
			break; // </advc.001w>
		// <advc.004j>
		case UPDATE_LOOK_AT_STARTING_PLOT:
		{
			CvPlot* pStartingPlot = GET_PLAYER(getActivePlayer()).getStartingPlot();
			if(pStartingPlot != NULL)
				gDLL->getInterfaceIFace()->lookAt(pStartingPlot->getPoint(), CAMERALOOKAT_NORMAL);
			break;
		} // </advc.004j>
		// advc.106n:
		case UPDATE_STORE_REPLAY_TEXTURE: GC.getMap().updateReplayTexture(); break;
		default: FErrorMsg("Unknown update timer type");
		}
	}
	m_aiUpdateTimers[eTimerType]--;
} // </advc.003r>


void CvGame::addReplayMessage(ReplayMessageTypes eType, PlayerTypes ePlayer,
	CvWString pszText, int iPlotX, int iPlotY, ColorTypes eColor)
{
	int iGameTurn = getGameTurn();
	CvReplayMessage* pMessage = new CvReplayMessage(iGameTurn, eType, ePlayer);
	pMessage->setPlot(iPlotX, iPlotY);
	pMessage->setText(pszText);
	if (NO_COLOR == eColor)
		eColor = GC.getColorType("WHITE");
	pMessage->setColor(eColor);
	m_listReplayMessages.push_back(pMessage);
}


void CvGame::clearReplayMessageMap()
{
	for (ReplayMessageList::iterator itList = m_listReplayMessages.begin();
		itList != m_listReplayMessages.end(); ++itList)
	{
		SAFE_DELETE(*itList);
	}
	m_listReplayMessages.clear();
}


// advc: To get rid of some duplicate code
bool CvGame::isValidReplayIndex(uint i) const
{
	if (i >= m_listReplayMessages.size())
		return false;
	CvReplayMessage const* pMessage = m_listReplayMessages[i];
	if (pMessage == NULL)
		return false;
	return true;
}


int CvGame::getReplayMessageTurn(uint i) const
{
	return (isValidReplayIndex(i) ? m_listReplayMessages[i]->getTurn() : -1);
}


ReplayMessageTypes CvGame::getReplayMessageType(uint i) const
{
	return (isValidReplayIndex(i) ? m_listReplayMessages[i]->getType() : NO_REPLAY_MESSAGE);
}

int CvGame::getReplayMessagePlotX(uint i) const
{
	return (isValidReplayIndex(i) ? m_listReplayMessages[i]->getPlotX() : INVALID_PLOT_COORD);
}


int CvGame::getReplayMessagePlotY(uint i) const
{
	return (isValidReplayIndex(i) ? m_listReplayMessages[i]->getPlotY() : INVALID_PLOT_COORD);
}


PlayerTypes CvGame::getReplayMessagePlayer(uint i) const
{
	return (isValidReplayIndex(i) ? m_listReplayMessages[i]->getPlayer() : NO_PLAYER);
}


LPCWSTR CvGame::getReplayMessageText(uint i) const
{
	return (isValidReplayIndex(i) ? m_listReplayMessages[i]->getText().GetCString() : NULL);
}


ColorTypes CvGame::getReplayMessageColor(uint i) const
{
	return (isValidReplayIndex(i) ? m_listReplayMessages[i]->getColor() : NO_COLOR);
}


uint CvGame::getNumReplayMessages() const
{
	return m_listReplayMessages.size();
}


void CvGame::read(FDataStreamBase* pStream)
{
	reset(NO_HANDICAP);

	uint uiFlag=0;
	pStream->Read(&uiFlag);
	/*	advc: So that onAllGameDataRead can perform some final updates
		based on the save version */
	m_uiSaveFlag = uiFlag;
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
	m_iAIAutoPlay = -1; // advc.127: allGameDataRead will handle it properly
	pStream->Read(&m_iGlobalWarmingIndex); // K-Mod
	pStream->Read(&m_iGwEventTally); // K-Mod
	// <advc.opt>
	if(uiFlag >= 4)
	{
		pStream->Read(&m_iCivPlayersEverAlive);
		pStream->Read(&m_iCivTeamsEverAlive);
	} /* The else case is handled in allGameDataRead - need to read the players and
		 teams first. */
	// </advc.opt>
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
	if(uiFlag >= 5)
	{
		pStream->Read((int*)&m_eCurrentLayer);
		/*	Initial autosave doesn't contain valid info about the globe layers
				b/c it gets created before Python calls reportCurrentLayer */
		if (getTurnSlice() > 0)
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
	m_aiVoteOutcome.Read(pStream);
	pStream->Read(GC.getNumReligionInfos(), m_paiReligionGameTurnFounded);
	pStream->Read(GC.getNumCorporationInfos(), m_paiCorporationGameTurnFounded);
	pStream->Read(GC.getNumVoteSourceInfos(), m_aiSecretaryGeneralTimer);
	pStream->Read(GC.getNumVoteSourceInfos(), m_aiVoteTimer);
	pStream->Read(GC.getNumVoteSourceInfos(), m_aiDiploVote);

	pStream->Read(GC.getNumSpecialUnitInfos(), m_pabSpecialUnitValid);
	pStream->Read(GC.getNumSpecialBuildingInfos(), m_pabSpecialBuildingValid);
	pStream->Read(GC.getNumReligionInfos(), m_abReligionSlotTaken);

	FOR_EACH_ENUM(Religion)
	{
		pStream->Read((int*)&m_paHolyCity[eLoopReligion].eOwner);
		pStream->Read(&m_paHolyCity[eLoopReligion].iID);
		m_paHolyCity[eLoopReligion].validateOwner(); // advc.opt
	}
	FOR_EACH_ENUM(Corporation)
	{
		pStream->Read((int*)&m_paHeadquarters[eLoopCorporation].eOwner);
		pStream->Read(&m_paHeadquarters[eLoopCorporation].iID);
		m_paHeadquarters[eLoopCorporation].validateOwner();
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
	// <advc.027b>
	if (uiFlag >= 7)
	{
		pStream->Read(&m_initialRandSeed.uiMap);
		pStream->Read(&m_initialRandSeed.uiSync);
	} // </advc.027b>
	// <advc.250b>
	if(isOption(GAMEOPTION_SPAH))
		m_pSpah->read(pStream); // </advc.250b>
	// <advc.701>
	if(uiFlag >= 2)
	{
		if(isOption(GAMEOPTION_RISE_FALL))
			m_pRiseFall->read(pStream);
	}
	else // Options have been shuffled around
	{
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
			if (pMessage != NULL)
				pMessage->read(*pStream);
			m_listReplayMessages.push_back(pMessage);
		}
	}
	// m_pReplayInfo not saved

	pStream->Read(&m_iNumSessions);
	if (!isNetworkMultiPlayer())
		m_iNumSessions++;

	{
		int iSize;
		m_aPlotExtraYields.clear();
		pStream->Read(&iSize);
		for (int i = 0; i < iSize; i++)
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
		for (int i = 0; i < iSize; i++)
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
		for (int i = 0; i < iSize; i++)
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
		for (int i = 0; i < iSize; i++)
		{
			int iTrigger;
			pStream->Read(&iTrigger);
			m_aeInactiveTriggers.push_back((EventTriggerTypes)iTrigger);
		}
	}

	// Get the active player information from the initialization structure
	if (!isGameMultiPlayer())
	{
		for (int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			if (GET_PLAYER((PlayerTypes)i).isHuman())
			{
				setActivePlayer((PlayerTypes)i);
				break;
			}
		}
		addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getActivePlayer(),
				gDLL->getText("TXT_KEY_MISC_RELOAD", m_iNumSessions));
	}

	if (isOption(GAMEOPTION_NEW_RANDOM_SEED))
	{
		if (!isNetworkMultiPlayer() &&
			!GC.getDefineBOOL("IGNORE_NEW_RANDOM_SEED_OPTION")) // advc
		{
			m_sorenRand.reseed(timeGetTime());
		}
	}

	pStream->Read(&m_iShrineBuildingCount);
	pStream->Read(GC.getNumBuildingInfos(), m_aiShrineBuilding);
	pStream->Read(GC.getNumBuildingInfos(), m_aiShrineReligion);
	pStream->Read(&m_iNumCultureVictoryCities);
	pStream->Read((int*)&m_eCultureVictoryCultureLevel);
	// <advc.052>
	if(uiFlag >= 3)
		pStream->Read(&m_bScenario); // </advc.052>
	m_iTurnLoadedFromSave = m_iElapsedGameTurns; // advc.044
	applyOptionEffects(); // advc.310
	m_bFPTestDone = !isNetworkMultiPlayer(); // advc.003g
}


void CvGame::write(FDataStreamBase* pStream)
{
	PROFILE_FUNC(); // advc
	uint uiFlag;
	//uiFlag = 1; // BtS
	//uiFlag = 2; // advc.701: R&F option
	//uiFlag = 3; // advc.052
	//uiFlag = 4; // advc.opt: Players and teams ever alive
	//uiFlag = 5; // advc.004m
	//uiFlag = 6; // advc.106h
	//uiFlag = 7; // advc.027b
	uiFlag = 8; // advc.172
	pStream->Write(uiFlag);
	REPRO_TEST_BEGIN_WRITE("Game pt1");
	pStream->Write(m_iElapsedGameTurns);
	pStream->Write(m_iStartTurn);
	pStream->Write(m_iStartYear);
	pStream->Write(m_iEstimateEndTurn);
	REPRO_TEST_END_WRITE(); // Skip TurnSlice
	pStream->Write(m_iTurnSlice);
	pStream->Write(m_iCutoffSlice);
	pStream->Write(m_iNumGameTurnActive);
	REPRO_TEST_BEGIN_WRITE("Game pt2");
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
	// <advc.opt>
	pStream->Write(m_iCivPlayersEverAlive);
	pStream->Write(m_iCivTeamsEverAlive);
	// </advc.opt>
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
	m_aiVoteOutcome.Write(pStream);
	pStream->Write(GC.getNumReligionInfos(), m_paiReligionGameTurnFounded);
	pStream->Write(GC.getNumCorporationInfos(), m_paiCorporationGameTurnFounded);
	pStream->Write(GC.getNumVoteSourceInfos(), m_aiSecretaryGeneralTimer);
	pStream->Write(GC.getNumVoteSourceInfos(), m_aiVoteTimer);
	pStream->Write(GC.getNumVoteSourceInfos(), m_aiDiploVote);

	pStream->Write(GC.getNumSpecialUnitInfos(), m_pabSpecialUnitValid);
	pStream->Write(GC.getNumSpecialBuildingInfos(), m_pabSpecialBuildingValid);
	pStream->Write(GC.getNumReligionInfos(), m_abReligionSlotTaken);

	FOR_EACH_ENUM(Religion)
	{
		pStream->Write(m_paHolyCity[eLoopReligion].eOwner);
		pStream->Write(m_paHolyCity[eLoopReligion].iID);
	}
	FOR_EACH_ENUM(Corporation)
	{
		pStream->Write(m_paHeadquarters[eLoopCorporation].eOwner);
		pStream->Write(m_paHeadquarters[eLoopCorporation].iID);
	}

	{
		std::vector<CvWString>::iterator it;
		pStream->Write(m_aszDestroyedCities.size());
		for (it = m_aszDestroyedCities.begin(); it != m_aszDestroyedCities.end(); ++it)
		{
			pStream->WriteString(*it);
		}

		pStream->Write(m_aszGreatPeopleBorn.size());
		for (it = m_aszGreatPeopleBorn.begin(); it != m_aszGreatPeopleBorn.end(); ++it)
		{
			pStream->WriteString(*it);
		}
	}
	REPRO_TEST_END_WRITE();
	WriteStreamableFFreeListTrashArray(m_deals, pStream);
	REPRO_TEST_BEGIN_WRITE("Game pt3");
	WriteStreamableFFreeListTrashArray(m_voteSelections, pStream);
	WriteStreamableFFreeListTrashArray(m_votesTriggered, pStream);
	REPRO_TEST_END_WRITE();
	REPRO_TEST_BEGIN_WRITE("SyncRNGs");
	m_mapRand.write(pStream);
	m_sorenRand.write(pStream);
	// <advc.027b>
	pStream->Write(m_initialRandSeed.uiMap);
	pStream->Write(m_initialRandSeed.uiSync); // </advc.027b>
	REPRO_TEST_END_WRITE();
	// <advc.250b>
	if(isOption(GAMEOPTION_SPAH))
		m_pSpah->write(pStream); // </advc.250b>
	// <advc.701>
	if(isOption(GAMEOPTION_RISE_FALL))
		m_pRiseFall->write(pStream); // </advc.701>
	ReplayMessageList::_Alloc::size_type iSize = m_listReplayMessages.size();
	pStream->Write(iSize);
	for (ReplayMessageList::const_iterator it = m_listReplayMessages.begin();
		it != m_listReplayMessages.end(); ++it)
	{
		const CvReplayMessage* pMessage = *it;
		if (pMessage != NULL)
			pMessage->write(*pStream);
	}
	// m_pReplayInfo not saved
	pStream->Write(m_iNumSessions);
	REPRO_TEST_BEGIN_WRITE("Game pt3"); // (skip replay messages, sessions)
	pStream->Write(m_aPlotExtraYields.size());
	for (std::vector<PlotExtraYield>::iterator it = m_aPlotExtraYields.begin();
		it != m_aPlotExtraYields.end(); ++it)
	{
		it->write(pStream);
	}

	pStream->Write(m_aPlotExtraCosts.size());
	for (std::vector<PlotExtraCost>::iterator it = m_aPlotExtraCosts.begin();
		it != m_aPlotExtraCosts.end(); ++it)
	{
		it->write(pStream);
	}

	pStream->Write(m_mapVoteSourceReligions.size());
	for (stdext::hash_map<VoteSourceTypes,ReligionTypes>::iterator it = m_mapVoteSourceReligions.begin();
		it != m_mapVoteSourceReligions.end(); ++it)
	{
		pStream->Write(it->first);
		pStream->Write(it->second);
	}

	pStream->Write(m_aeInactiveTriggers.size());
	for (std::vector<EventTriggerTypes>::iterator it = m_aeInactiveTriggers.begin();
		it != m_aeInactiveTriggers.end(); ++it)
	{
		pStream->Write(*it);
	}

	pStream->Write(m_iShrineBuildingCount);
	pStream->Write(GC.getNumBuildingInfos(), m_aiShrineBuilding);
	pStream->Write(GC.getNumBuildingInfos(), m_aiShrineReligion);
	pStream->Write(m_iNumCultureVictoryCities);
	pStream->Write(m_eCultureVictoryCultureLevel);
	pStream->Write(m_bScenario); // advc.052
	REPRO_TEST_END_WRITE();
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

/*  advc: When loading a savegame, this function is called once all
	read functions have been called. */
void CvGame::onAllGameDataRead()
{
	// <advc.opt> Savegame compatibility (uiFlag<4)
	if(m_iCivPlayersEverAlive == 0)
		m_iCivPlayersEverAlive = countCivPlayersEverAlive();
	if(m_iCivTeamsEverAlive == 0)
		m_iCivTeamsEverAlive = countCivTeamsEverAlive();
	// </advc.opt>
	GC.getAgents().gameStart(true); // advc.agent
	// <advc.003m>
	for (TeamIter<> it; it.hasNext(); ++it)
	{
		if (it->getNumWars() < 0 || it->getNumWars(false, false) < 0 ||
			it->getNumWars(true, true) < 0)
		{
			it->finalizeInit();
		}
	} // </advc.003m>  <advc.opt>
	for (TeamIter<> it; it.hasNext(); ++it)
	{
		FOR_EACH_ENUM(WarPlan)
		{
			if (it->AI_getNumWarPlans(eLoopWarPlan) < 0)
			{
				it->AI_finalizeInit();
				break;
			}
		}
	} // </advc.opt>
	m_bAllGameDataRead = true;
	for (PlayerIter<HUMAN> it; it.hasNext(); ++it)
	{
		CvPlayerAI& kActive = *it;
		if (!kActive.isTurnActive())
			continue;
		/*	Bad idea (by me). The cache data has to be serialized instead.
			It gets updated at the start of a turn, yes, but if we rely on
			that, then it won't be safe to access data from another player's
			cache (who may not have taken a turn since a savegame was loaded).
			Could update all caches after loading, but that would result in
			more recent data after loading than upon saving. */
		//kActive.AI_updateCacheData();
		kActive.validateDiplomacy(); // advc.134a
	}
	// <advc.127> Save created during AI Auto Play
	if (m_iAIAutoPlay != 0 && !isNetworkMultiPlayer())
	{
		for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
			it->AI_updateAttitude();
	}
	m_iAIAutoPlay = 0; // </advc.127>
	// <advc.172>
	if (m_uiSaveFlag < 8)
	{
		for (PlayerIter<ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
		{
			FOR_EACH_CITY_VAR(pCity, *itPlayer)
				pCity->updateReligionCommerce();
		}
	} // </advc.172>
}

/*	advc: Called once the EXE signals that graphics have been initialized
	(w/e that means exactly) */
void CvGame::onGraphicsInitialized()
{
	// advc.095:
	setCityBarWidth(BUGOption::isEnabled("MainInterface__WideCityBars", false));
	/*	<advc.001> After loading, the camera tries to center on some unit
		(apparently; I don't know where that's implemented). If there is
		none, it seems to center on some random(?) unrevealed tile. */
	if (GET_PLAYER(getActivePlayer()).getNumUnits() == 0)
		setUpdateTimer(UPDATE_LOOK_AT_STARTING_PLOT, 1);
	// </advc.001>
	GC.getPythonCaller()->callScreenFunction("updateCameraStartDistance"); // advc.004m
}


void CvGame::saveReplay(PlayerTypes ePlayer)
{	// advc.106i: Hack to prepend sth. to the replay file name
	GET_PLAYER(ePlayer).setSavingReplay(true);
	gDLL->getEngineIFace()->SaveReplay(ePlayer);
	// advc.106i: Probably redundant b/c CvGame::writeReplay already sets it to false
	GET_PLAYER(ePlayer).setSavingReplay(false);
}


void CvGame::showEndGameSequence()
{
	long iHours = getMinutesPlayed() / 60;
	long iMinutes = getMinutesPlayed() % 60;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& player = GET_PLAYER((PlayerTypes)iI);
		if (player.isHuman())
		{
			addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, (PlayerTypes)iI, gDLL->getText("TXT_KEY_MISC_TIME_SPENT", iHours, iMinutes));

			CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_TEXT);
			if (NULL != pInfo)
			{
				if ((getWinner() != NO_TEAM) && (getVictory() != NO_VICTORY))
				{
					pInfo->setText(gDLL->getText("TXT_KEY_GAME_WON", GET_TEAM(getWinner()).getName().GetCString(), GC.getInfo(getVictory()).getTextKeyWide()));
				}
				else
				{
					pInfo->setText(gDLL->getText("TXT_KEY_MISC_DEFEAT"));
				}
				player.addPopup(pInfo);
			}

			if (getWinner() == player.getTeam())
			{
				if (!CvString(GC.getInfo(getVictory()).getMovie()).empty())
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
				else if (GC.getInfo(getVictory()).isDiploVote())
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
				pInfo->setData1(0); // advc.003y: Disable replay buttons
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
	/*	UNOFFICIAL_PATCH Start: Fixed bug with colonies who occupy recycled player slots
		showing the old leader or civ names */
	CvWString szEmptyString = L"";
	LeaderHeadTypes const eOldLeader = GET_PLAYER(eNewPlayer).getLeaderType();
	CvInitCore& kInitCore = GC.getInitCore();
	if (eOldLeader != NO_LEADER && eOldLeader != eLeader)
		kInitCore.setLeaderName(eNewPlayer, szEmptyString);
	CivilizationTypes const eOldCiv = GET_PLAYER(eNewPlayer).getCivilizationType();
	if (eOldCiv != NO_CIVILIZATION && eOldCiv != eCiv)
	{
		kInitCore.setCivAdjective(eNewPlayer, szEmptyString);
		kInitCore.setCivDescription(eNewPlayer, szEmptyString);
		kInitCore.setCivShortDesc(eNewPlayer, szEmptyString);
	}
	// UNOFFICIAL_PATCH End
	PlayerColorTypes eColor = (PlayerColorTypes)GC.getInfo(eCiv).getDefaultPlayerColor();
	// <advc> Restructured these byzantine loops. Hope I got it right.
	bool bFindNewColor = (eColor == NO_PLAYERCOLOR);
	if (!bFindNewColor)
	{
		for (PlayerIter<EVER_ALIVE> itOther; itOther.hasNext(); ++itOther)
		{
			if (itOther->getID() != eNewPlayer &&
				/*  UNOFFICIAL_PATCH, Bugfix, 12/30/08, jdog5000:
				Don't invalidate color choice if it's taken by this player */
				itOther->getPlayerColor() == eColor)
			{
				bFindNewColor = true;
				break;
			}
		}
	}
	if (bFindNewColor)
	{
		PlayerColorTypes const eBarbarianColor = (PlayerColorTypes)GC.getInfo((CivilizationTypes)
				GC.getDefineINT("BARBARIAN_CIVILIZATION")).getDefaultPlayerColor();
		FOR_EACH_ENUM(PlayerColor)
		{
			if (eLoopPlayerColor == eBarbarianColor)
				continue;
			// advc: Better make sure we have _some_ color
			if (eColor == NO_PLAYERCOLOR)
				eColor = eLoopPlayerColor;

			bool bValid = true;
			for (PlayerIter<EVER_ALIVE> itOther; itOther.hasNext(); ++itOther)
			{
				if (itOther->getID() != eNewPlayer && // advc: Same as in the bugfix above
					itOther->getPlayerColor() == eLoopPlayerColor)
				{
					bValid = false;
					break;
				}
			}
			if (bValid)
			{
				eColor = eLoopPlayerColor;
				break;
			}
		}
	} // </advc>
	kInitCore.setLeader(eNewPlayer, eLeader);
	kInitCore.setCiv(eNewPlayer, eCiv);
	kInitCore.setSlotStatus(eNewPlayer, SS_COMPUTER);
	kInitCore.setColor(eNewPlayer, eColor);
	// advc.001: For RANDOM_PERSONALITIES option
	GET_PLAYER(eNewPlayer).changePersonalityType();
	/*GET_TEAM(eNewPlayer).init(eTeam);
	GET_PLAYER(eNewPlayer).init(eNewPlayer);*/
	// BETTER_BTS_AI_MOD, Bugfix, 12/30/08, jdog5000: START
	/*	Team init now handled when appropriate in player initInGame.
		Standard player init is written for beginning of game,
		it resets global random events for this player only among other flaws. */
	GET_PLAYER(eNewPlayer).initInGame(eNewPlayer);
	// BETTER_BTS_AI_MOD: END
}

//	BETTER_BTS_AI_MOD, Debug, 8/1/08, jdog5000: START
void CvGame::changeHumanPlayer(PlayerTypes eNewHuman)
{
	PlayerTypes eCurHuman = getActivePlayer();
	/*  <advc.127> Rearranged code b/c of a change in CvPlayer::isOption.
		Important for advc.706. */
	if(eNewHuman == eCurHuman)
	{
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
	// <advc.001> Otherwise, the selected unit will affect CvPlot::updateCenterUnit.
	gDLL->getInterfaceIFace()->clearSelectionList();
	gDLL->getInterfaceIFace()->clearSelectedCities(); // </advc.001>
	setActivePlayer(eNewHuman, false);

	GET_PLAYER(eCurHuman).setIsHuman(false, /* advc.127c: */ true);
	GET_PLAYER(eCurHuman).updateHuman(); // advc.127
} // BETTER_BTS_AI_MOD: END


bool CvGame::isCompetingCorporation(
	CorporationTypes eCorporation1, CorporationTypes eCorporation2) const
{
	// K-Mod
	if (eCorporation1 == eCorporation2)
		return false;
	// K-Mod end

	bool bShareResources = false;

	for (int i = 0; i < GC.getInfo(eCorporation1).getNumPrereqBonuses() &&
		!bShareResources; i++)
	{
		for (int j = 0; j < GC.getInfo(eCorporation2).getNumPrereqBonuses(); j++)
		{
			if (GC.getInfo(eCorporation1).getPrereqBonus(i) ==
				GC.getInfo(eCorporation2).getPrereqBonus(j))
			{
				return true;
			}
		}
	}

	return false;
}

int CvGame::getPlotExtraYield(int iX, int iY, YieldTypes eYield) const
{
	for (std::vector<PlotExtraYield>::const_iterator it = m_aPlotExtraYields.begin();
		it != m_aPlotExtraYields.end(); ++it)
	{
		if (it->m_iX == iX && it->m_iY == iY)
			return it->m_aeExtraYield[eYield];
	}
	return 0;
}

void CvGame::setPlotExtraYield(int iX, int iY, YieldTypes eYield, int iExtraYield)
{
	bool bFound = false;
	for (std::vector<PlotExtraYield>::iterator it = m_aPlotExtraYields.begin();
		it != m_aPlotExtraYields.end(); ++it)
	{
		if (it->m_iX == iX && it->m_iY == iY)
		{
			it->m_aeExtraYield[eYield] += iExtraYield;
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
				kExtraYield.m_aeExtraYield.push_back(iExtraYield);
			else kExtraYield.m_aeExtraYield.push_back(0);
		}
		m_aPlotExtraYields.push_back(kExtraYield);
	}
	CvPlot* pPlot = GC.getMap().plot(iX, iY);
	if (pPlot != NULL)
		pPlot->updateYield();
}

void CvGame::removePlotExtraYield(int iX, int iY)
{
	for (std::vector<PlotExtraYield>::iterator it = m_aPlotExtraYields.begin();
		it != m_aPlotExtraYields.end(); ++it)
	{
		if (it->m_iX == iX && it->m_iY == iY)
		{
			m_aPlotExtraYields.erase(it);
			break;
		}
	}
	CvPlot* pPlot = GC.getMap().plot(iX, iY);
	if (pPlot != NULL)
		pPlot->updateYield();
}

int CvGame::getPlotExtraCost(int iX, int iY) const
{
	for (std::vector<PlotExtraCost>::const_iterator it = m_aPlotExtraCosts.begin();
		it != m_aPlotExtraCosts.end(); ++it)
	{
		if (it->m_iX == iX && it->m_iY == iY)
		{
			return it->m_iCost;
		}
	}
	return 0;
}

void CvGame::changePlotExtraCost(int iX, int iY, int iCost)
{
	bool bFound = false;
	for (std::vector<PlotExtraCost>::iterator it = m_aPlotExtraCosts.begin();
		it != m_aPlotExtraCosts.end(); ++it)
	{
		if (it->m_iX == iX && it->m_iY == iY)
		{
			it->m_iCost += iCost;
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
	for (std::vector<PlotExtraCost>::iterator it = m_aPlotExtraCosts.begin();
		it != m_aPlotExtraCosts.end(); ++it)
	{
		if (it->m_iX == iX && it->m_iY == iY)
		{
			m_aPlotExtraCosts.erase(it);
			break;
		}
	}
}

ReligionTypes CvGame::getVoteSourceReligion(VoteSourceTypes eVoteSource) const
{
	stdext::hash_map<VoteSourceTypes,ReligionTypes>::const_iterator it;
	it = m_mapVoteSourceReligions.find(eVoteSource);
	if (it == m_mapVoteSourceReligions.end())
		return NO_RELIGION;
	return it->second;
}

void CvGame::setVoteSourceReligion(VoteSourceTypes eVoteSource,
	ReligionTypes eReligion, bool bAnnounce)
{
	m_mapVoteSourceReligions[eVoteSource] = eReligion;

	if (bAnnounce)
	{
		if (NO_RELIGION != eReligion)
		{
			CvWString szBuffer = gDLL->getText("TXT_KEY_VOTE_SOURCE_RELIGION",
					GC.getInfo(eReligion).getTextKeyWide(),
					GC.getInfo(eReligion).getAdjectiveKey(),
					GC.getInfo(eVoteSource).getTextKeyWide());

			for (int iI = 0; iI < MAX_PLAYERS; iI++)
			{
				PlayerTypes ePlayer = (PlayerTypes)iI;
				if (GET_PLAYER(ePlayer).isAlive())
				{	// <advc.127b>
					std::pair<int,int> xy = getVoteSourceXY(eVoteSource,
							TEAMID(ePlayer), true); // </advc.127>
					gDLL->UI().addMessage(ePlayer, false, -1, szBuffer,
							GC.getInfo(eReligion).getSound(), MESSAGE_TYPE_MAJOR_EVENT,
							NULL, GC.getColorType("HIGHLIGHT_TEXT"),
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
	else
	{
		for (int i = 0; i < m_iShrineBuildingCount; i++)
		{
			if (m_aiShrineReligion[i] == eReligion)
				iShrineBuildingCount++;
		}
	}
	return iShrineBuildingCount;
}

BuildingTypes CvGame::getShrineBuilding(int eIndex, ReligionTypes eReligion)
{
	FAssert(eIndex >= 0 && eIndex < m_iShrineBuildingCount);
	BuildingTypes eBuilding = NO_BUILDING;
	if (eIndex >= 0 && eIndex < m_iShrineBuildingCount)
	{
		if (eReligion == NO_RELIGION)
			eBuilding = (BuildingTypes) m_aiShrineBuilding[eIndex];
		else
		{
			for (int iI = 0, iReligiousBuilding = 0; iI < m_iShrineBuildingCount; iI++)
			{
				if (m_aiShrineReligion[iI] == eReligion)
				{
					if (iReligiousBuilding == eIndex) // found it
					{
						eBuilding = (BuildingTypes) m_aiShrineBuilding[iI];
						break;
					}
					iReligiousBuilding++;
				}
			}
		}
	}
	return eBuilding;
}

void CvGame::changeShrineBuilding(BuildingTypes eBuilding,
	ReligionTypes eReligion, bool bRemove)
{
	FAssert(eBuilding >= 0 && eBuilding < GC.getNumBuildingInfos());
	FAssert(bRemove || m_iShrineBuildingCount < GC.getNumBuildingInfos());
	if (bRemove)
	{
		bool bFound = false;
		for (int i = 0; i < m_iShrineBuildingCount; i++)
		{
			if (!bFound)
			{
				/*	note, eReligion is not important if we are removing,
					since each building is always one religion */
				if (m_aiShrineBuilding[i] == eBuilding)
					bFound = true;
			}
			if (bFound)
			{
				int iToMove = i + 1;
				if (iToMove < m_iShrineBuildingCount)
				{
					m_aiShrineBuilding[i] = m_aiShrineBuilding[iToMove];
					m_aiShrineReligion[i] = m_aiShrineReligion[iToMove];
				}
				else
				{
					m_aiShrineBuilding[i] = NO_BUILDING;
					m_aiShrineReligion[i] = NO_RELIGION;
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

bool CvGame::culturalVictoryValid() /* advc: */ const
{
	return (m_iNumCultureVictoryCities > 0);
}

int CvGame::culturalVictoryNumCultureCities() /* advc: */ const
{
	return m_iNumCultureVictoryCities;
}

CultureLevelTypes CvGame::culturalVictoryCultureLevel() /* advc: */  const
{
	if (m_iNumCultureVictoryCities > 0)
		return m_eCultureVictoryCultureLevel;
	return NO_CULTURELEVEL;
}

int CvGame::getCultureThreshold(CultureLevelTypes eLevel) const
{
	int iThreshold = GC.getInfo(eLevel).getSpeedThreshold(getGameSpeedType());
	if (isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		static int const iNO_ESPIONAGE_CULTURE_LEVEL_MODIFIER = GC.getDefineINT("NO_ESPIONAGE_CULTURE_LEVEL_MODIFIER"); // advc.opt
		iThreshold *= 100 + iNO_ESPIONAGE_CULTURE_LEVEL_MODIFIER;
		iThreshold /= 100;
	} // <advc.126>
	int const iExempt = 50; // Don't adjust thresholds below "developing"
	if(iThreshold >= iExempt)
	{
		iThreshold *= GC.getInfo(getStartEra()).getCulturePercent();
		iThreshold /= 100; // </advc.126>
		// <advc.251>
		int iMultiple = 1;
		if (iThreshold > 100000)
			iMultiple = 10000;
		else if (iThreshold > 50000)
			iMultiple = 5000;
		else if (iThreshold > 10000)
			iMultiple = 1000;
		else if (iThreshold > 1000)
			iMultiple = 100;
		else if (iThreshold > 100)
			iMultiple = 10;
		iThreshold *= GC.getInfo(getHandicapType()).getCultureLevelPercent();
		iThreshold = per100(iThreshold).roundToMultiple(iMultiple);
		iThreshold = std::max(iThreshold, iExempt);
	} // </advc.251>
	return iThreshold;
}

void CvGame::doUpdateCacheOnTurn()
{
	// reset shrine count
	m_iShrineBuildingCount = 0;
	FOR_EACH_ENUM(Building)
	{
		// if it is for holy city, then its a shrine-thing, add it
		if (GC.getInfo(eLoopBuilding).getHolyCity() != NO_RELIGION)
			changeShrineBuilding(eLoopBuilding, GC.getInfo(eLoopBuilding).getReligionType());
	}

	// reset cultural victories
	m_iNumCultureVictoryCities = 0;
	FOR_EACH_ENUM(Victory)
	{
		if (!isVictoryValid(eLoopVictory))
			continue;

		CvVictoryInfo const& kVictoryInfo = GC.getInfo(eLoopVictory);
		if (kVictoryInfo.getCityCulture() > 0)
		{
			int iNumCultureCities = kVictoryInfo.getNumCultureCities();
			if (iNumCultureCities > m_iNumCultureVictoryCities)
			{
				m_iNumCultureVictoryCities = iNumCultureCities;
				m_eCultureVictoryCultureLevel = (CultureLevelTypes)
						kVictoryInfo.getCityCulture();
			}
		}
	}
	// K-Mod. (todo: move all of that stuff above somewhere else. That doesn't need to be updated every turn!)
	if (isFinalInitialized()) // advc.pf: Else pathfinder may not have been created yet
		CvSelectionGroup::resetPath(); // (one of the few manual resets we need)
	m_ActivePlayerCycledGroups.clear();
	// K-Mod end
}

VoteSelectionData* CvGame::getVoteSelection(int iID) const
{
	return m_voteSelections.getAt(iID);
}

VoteSelectionData* CvGame::addVoteSelection(VoteSourceTypes eVoteSource)
{
	VoteSelectionData* pData = m_voteSelections.add();
	FAssert(pData != NULL); // advc
	pData->eVoteSource = eVoteSource;
	FOR_EACH_ENUM(Vote)  // advc.003n: Minor civs excluded from all loops
	{
		if (!GC.getInfo(eLoopVote).isVoteSourceType(eVoteSource) ||
			!isChooseElection(eLoopVote))
		{
			continue;
		}
		VoteSelectionSubData kData;
		kData.eVote = eLoopVote;
		kData.iCityId = -1;
		kData.ePlayer = NO_PLAYER;
		kData.eOtherPlayer = NO_PLAYER;
		if (GC.getInfo(kData.eVote).isOpenBorders())
		{
			if (isValidVoteSelection(eVoteSource, kData))
			{
				kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_OPEN_BORDERS",
						getVoteRequired(kData.eVote, eVoteSource),
						countPossibleVote(kData.eVote, eVoteSource));
				pData->aVoteOptions.push_back(kData);
			}
		}
		else if (GC.getInfo(kData.eVote).isDefensivePact())
		{
			if (isValidVoteSelection(eVoteSource, kData))
			{
				kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_DEFENSIVE_PACT",
						getVoteRequired(kData.eVote, eVoteSource),
						countPossibleVote(kData.eVote, eVoteSource));
				pData->aVoteOptions.push_back(kData);
			}
		}
		else if (GC.getInfo(kData.eVote).isForcePeace())
		{
			for (TeamIter<MAJOR_CIV> it; it.hasNext(); ++it)
			{
				kData.ePlayer = it->getLeaderID();
				if (isValidVoteSelection(eVoteSource, kData))
				{
					kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_FORCE_PEACE",
							it->getName().GetCString(),
							getVoteRequired(kData.eVote, eVoteSource),
							countPossibleVote(kData.eVote, eVoteSource));
					pData->aVoteOptions.push_back(kData);
				}
			}
		}
		else if (GC.getInfo(kData.eVote).isForceNoTrade())
		{
			for (TeamIter<MAJOR_CIV> it; it.hasNext(); ++it)
			{
				kData.ePlayer = it->getLeaderID();
				if (isValidVoteSelection(eVoteSource, kData))
				{
					kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_FORCE_NO_TRADE",
							it->getName().GetCString(),
							getVoteRequired(kData.eVote, eVoteSource),
							countPossibleVote(kData.eVote, eVoteSource));
					pData->aVoteOptions.push_back(kData);
				}
			}
		}
		else if (GC.getInfo(kData.eVote).isForceWar())
		{
			for (TeamIter<MAJOR_CIV> it; it.hasNext(); ++it)
			{
				kData.ePlayer = it->getLeaderID();
				if (isValidVoteSelection(eVoteSource, kData))
				{
					kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_FORCE_WAR",
							it->getName().GetCString(),
							getVoteRequired(kData.eVote, eVoteSource),
							countPossibleVote(kData.eVote, eVoteSource));
					pData->aVoteOptions.push_back(kData);
				}
			}
		}
		else if (GC.getInfo(kData.eVote).isAssignCity())
		{
			for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
			{
				FOR_EACH_CITY(pLoopCity, *it)
				{
					PlayerTypes eNewOwner = pLoopCity->getPlot().findHighestCulturePlayer();
					if (eNewOwner != NO_PLAYER &&
					/*  advc.099: No longer implied by findHighestCulturePlayer;
						mustn't return cities to dead civs. */
						GET_PLAYER(eNewOwner).isAlive())
					{
						kData.ePlayer = it->getID();
						kData.iCityId =	pLoopCity->getID();
						kData.eOtherPlayer = eNewOwner;
						if (isValidVoteSelection(eVoteSource, kData))
						{
							kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_ASSIGN_CITY",
									it->getCivilizationAdjectiveKey(), pLoopCity->getNameKey(),
									GET_PLAYER(eNewOwner).getNameKey(),
									getVoteRequired(kData.eVote, eVoteSource),
									countPossibleVote(kData.eVote, eVoteSource));
							pData->aVoteOptions.push_back(kData);
						}
					}
				}
			}
		}
		else
		{
			kData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_OPTION",
					GC.getInfo(kData.eVote).getTextKeyWide(),
					getVoteRequired(kData.eVote, eVoteSource),
					countPossibleVote(kData.eVote, eVoteSource));
			if (isVotePassed(kData.eVote))
				kData.szText += gDLL->getText("TXT_KEY_POPUP_PASSED");
			//if (canDoResolution(eVoteSource, kData))
			if (isValidVoteSelection(eVoteSource, kData)) // K-Mod (zomg!)
				pData->aVoteOptions.push_back(kData);
		}
	}

	if (pData->aVoteOptions.size() == 0)
	{
		deleteVoteSelection(pData->getID());
		pData = NULL;
	}
	return pData;
}

void CvGame::deleteVoteSelection(int iID)
{
	m_voteSelections.removeAt(iID);
}

VoteTriggeredData* CvGame::getVoteTriggered(int iID) const
{
	return m_votesTriggered.getAt(iID);
}

VoteTriggeredData* CvGame::addVoteTriggered(const VoteSelectionData& kData, int iChoice)
{
	if (iChoice == -1 || iChoice >= (int)kData.aVoteOptions.size())
		return NULL;
	return addVoteTriggered(kData.eVoteSource, kData.aVoteOptions[iChoice]);
}

VoteTriggeredData* CvGame::addVoteTriggered(VoteSourceTypes eVoteSource, const VoteSelectionSubData& kOptionData)
{
	VoteTriggeredData* pData = m_votesTriggered.add();
	FAssert(pData != NULL); // advc
	pData->eVoteSource = eVoteSource;
	pData->kVoteOption = kOptionData;
	for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
	{
		CvPlayerAI& kVoter = *it;
		if (!kVoter.isVotingMember(eVoteSource))
			continue;

		if (!kVoter.isHuman())
		{
			castVote(kVoter.getID(), pData->getID(),
					kVoter.AI_diploVote(kOptionData, eVoteSource, false));
			continue;
		}
		// <kekm.25> (advc: simplified)
		if (isTeamVote(kOptionData.eVote))
		{
			TeamTypes const eVoterMaster = kVoter.getMasterTeam();
			if (eVoterMaster != kVoter.getTeam() &&
				GET_TEAM(kVoter.getTeam()).isCapitulated() && // advc
				isTeamVoteEligible(eVoterMaster, eVoteSource) &&
				!isTeamVoteEligible(kVoter.getTeam(), eVoteSource))
			{
				castVote(kVoter.getID(), pData->getID(),
						kVoter.AI_diploVote(kOptionData, eVoteSource, false));
				continue;
			}
		} // </kekm.25>
		CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_DIPLOVOTE);
		pInfo->setData1(pData->getID());
		gDLL->getInterfaceIFace()->addPopup(pInfo, kVoter.getID());
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
	for (VoteTriggeredData* pVoteTriggered = m_votesTriggered.beginIter(&iLoop);
		NULL != pVoteTriggered; pVoteTriggered = m_votesTriggered.nextIter(&iLoop))
	{
		CvWString szBuffer;
		CvWString szMessage;
		VoteSelectionSubData subdata = pVoteTriggered->kVoteOption; // advc
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
					szMessage.Format(L"%s: %s", gDLL->getText("TXT_KEY_ELECTION_CANCELLED").GetCString(),
							GC.getInfo(eVote).getDescription());
					// advc.127b:
					std::pair<int,int> xy = getVoteSourceXY(eVoteSource, kPlayer.getTeam());
					gDLL->UI().addMessage(kPlayer.getID(), false, -1, szMessage,
							"AS2D_NEW_ERA", MESSAGE_TYPE_INFO, NULL,
							GC.getColorType("HIGHLIGHT_TEXT"),
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

				szBuffer = GC.getInfo(eVote).getDescription();

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
				if(subdata.ePlayer != NO_PLAYER && subdata.iCityId >= 0)
				{
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

				szBuffer += NEWLINE + gDLL->getText((bPassed ? "TXT_KEY_POPUP_DIPLOMATIC_VOTING_SUCCEEDS" : "TXT_KEY_POPUP_DIPLOMATIC_VOTING_FAILURE"), GC.getInfo(eVote).getTextKeyWide(), countVote(*pVoteTriggered, PLAYER_VOTE_YES), getVoteRequired(eVote, eVoteSource), countPossibleVote(eVote, eVoteSource));

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
			CvVoteInfo& kVote = GC.getInfo(eVote);
			if(bPassed && !kVote.isSecretaryGeneral())
			{
				CvWString szResolution;
				// Special treatment for resolutions with targets
				if(subdata.ePlayer != NO_PLAYER)
				{
					CvWString szKey;
					if(kVote.isForcePeace())
						szKey = L"TXT_KEY_POPUP_ELECTION_FORCE_PEACE";
					else if(kVote.isForceNoTrade())
						szKey = L"TXT_KEY_POPUP_ELECTION_FORCE_NO_TRADE";
					else if(kVote.isForceWar())
						szKey = L"TXT_KEY_POPUP_ELECTION_FORCE_WAR";
					if(!szKey.empty())
					{
						szResolution = gDLL->getText(szKey, GET_PLAYER(subdata.ePlayer).
								getReplayName(), 0, 0);
					}
					else if(kVote.isAssignCity() && !szTargetCityName.empty() &&
							subdata.eOtherPlayer != NO_PLAYER)
					{
						szResolution = gDLL->getText("TXT_KEY_POPUP_ELECTION_ASSIGN_CITY",
								GET_PLAYER(subdata.ePlayer).getCivilizationAdjectiveKey(),
								szTargetCityName.GetCString(),
								GET_PLAYER(subdata.eOtherPlayer).getReplayName(), 0, 0);
					}
				}
				if(szResolution.empty())
				{
					szResolution = kVote.getDescription();
					/*  This is e.g.
						"U.N. Resolution #1284 (Nuclear Non-Proliferation Treaty - Cannot Build Nuclear Weapons)
						Only want "Nuclear Non-Proliferation Treaty". */
					size_t pos1 = szResolution.find(L"(");
					if(pos1 != CvWString::npos && pos1 + 1 < szResolution.length())
					{
						bool bForceCivic = false;
						// Mustn't remove the stuff after the dash if bForceCivic
						for(int i = 0; i < GC.getNumCivicInfos(); i++)
						{
							if(kVote.isForceCivic(i))
							{
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
				else
				{
					/*  Throw out stuff in parentheses, e.g.
						"Stop the war against Napoleon (Requires 0 of 0 Total Votes)" */
					szResolution = szResolution.substr(0, szResolution.find(L"(") - 1);
				}
				TeamTypes eSecrGen = getSecretaryGeneral(eVoteSource);
				szMessage = gDLL->getText("TXT_KEY_REPLAY_RESOLUTION_PASSED",
						GC.getInfo(eVoteSource).getTextKeyWide(),
						(eSecrGen == NO_TEAM ?
						gDLL->getText("TXT_KEY_TOPCIVS_UNKNOWN").GetCString() :
						GET_TEAM(eSecrGen).getReplayName().GetCString()),
						iVotes, // Don't show the required votes after all
						//getVoteRequired(eVote, eVoteSource),
						countPossibleVote(eVote, eVoteSource),
						szResolution.GetCString());
				addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, NO_PLAYER, szMessage,
						-1, -1, GC.getColorType("HIGHLIGHT_TEXT"));
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
				} // <advc>
				if(!bShow && iI == subdata.ePlayer && GET_PLAYER(subdata.ePlayer).
						isVotingMember(pVoteTriggered->eVoteSource))
					bShow = true;
				if(!bShow && iI == subdata.eOtherPlayer && GET_PLAYER(subdata.eOtherPlayer).
						isVotingMember(pVoteTriggered->eVoteSource))
					bShow = true; // </advc>
				if (bPassed && (bShow // <advc.127>
						|| kPlayer.isSpectator()))
				{
					if(bShow || szMessage.empty() || kVote.isSecretaryGeneral())
					{	// </advc.127>
						szMessage = gDLL->getText("TXT_KEY_VOTE_RESULTS",
								GC.getInfo(eVoteSource).getTextKeyWide(),
								subdata.szText.GetCString());
						// Else use the replay msg
					}
					// <advc.127b>
					BuildingTypes eVSBuilding = getVoteSourceBuilding(eVoteSource);
					std::pair<int,int> xy = getVoteSourceXY(eVoteSource,
							kPlayer.getTeam(), true);
					// </advc.127b>
					gDLL->UI().addMessage(kPlayer.getID(), false, -1, szMessage, "AS2D_NEW_ERA",
							// <advc.127> was always MINOR
							kVote.isSecretaryGeneral() ? MESSAGE_TYPE_MINOR_EVENT :
							MESSAGE_TYPE_MAJOR_EVENT, // </advc.127>
							// <advc.127b>
							eVSBuilding == NO_BUILDING ? NULL :
							GC.getInfo(eVSBuilding).getButton(), // </advc.127b>
							GC.getColorType("HIGHLIGHT_TEXT"),
							xy.first, xy.second); // advc.127b
				}
			}
		}

		if (!bPassed && GC.getInfo(eVote).isSecretaryGeneral())
			setSecretaryGeneralTimer(eVoteSource, 0);

		deleteVoteTriggered(pVoteTriggered->getID());
	}
}

void CvGame::doVoteSelection()
{
	FOR_EACH_ENUM2(VoteSource, eVS)
	{
		if (!isDiploVote(eVS))
			continue;
		if (getVoteTimer(eVS) > 0)
		{
			changeVoteTimer(eVS, -1);
			continue;
		}
		setVoteTimer(eVS, (GC.getInfo(eVS).getVoteInterval() *
				GC.getInfo(getGameSpeedType()).getVictoryDelayPercent()) / 100);

		for (TeamIter<MAJOR_CIV> itTeam1; itTeam1.hasNext(); ++itTeam1)
		{
			if (!itTeam1->isVotingMember(eVS))
				continue;
			for (TeamIter<MAJOR_CIV> itTeam2; itTeam2.hasNext(); ++itTeam2)
			{
				if (!itTeam2->isVotingMember(eVS))
					continue;
				//itTeam1->meet(itTeam2->getID(), true);
				// <advc.071> Check isHasMet b/c getVoteSourceCity is a bit slow
				if(!itTeam1->isHasMet(itTeam2->getID()))
				{
					CvCity const* pSrcCity = getVoteSourceCity(eVS, NO_TEAM);
					if(pSrcCity == NULL)
						itTeam1->meet(itTeam2->getID(), true, NULL);
					else
					{
						FirstContactData fcData(pSrcCity->plot());
						itTeam1->meet(itTeam2->getID(), true, &fcData);
					} // </advc.071>
				}
			}
		}
		TeamTypes eSecretaryGeneral = getSecretaryGeneral(eVS);
		PlayerTypes eSecretaryPlayer = NO_PLAYER;
		if (eSecretaryGeneral != NO_TEAM)
			eSecretaryPlayer = GET_TEAM(eSecretaryGeneral).getSecretaryID();

		bool bSecretaryGeneralVote = false;
		if (canHaveSecretaryGeneral(eVS))
		{
			if (getSecretaryGeneralTimer(eVS) > 0)
				changeSecretaryGeneralTimer(eVS, -1);
			else
			{
				setSecretaryGeneralTimer(eVS, GC.getDefineINT("DIPLO_VOTE_SECRETARY_GENERAL_INTERVAL"));
				FOR_EACH_ENUM(Vote)
				{
					CvVoteInfo const& kLoopVote = GC.getInfo(eLoopVote);
					if (kLoopVote.isSecretaryGeneral() && kLoopVote.isVoteSourceType(eVS))
					{
						VoteSelectionSubData kOptionData;
						kOptionData.iCityId = -1;
						kOptionData.ePlayer = NO_PLAYER;
						kOptionData.eOtherPlayer = NO_PLAYER; // kmodx: Missing initialization
						kOptionData.eVote = eLoopVote;
						kOptionData.szText = gDLL->getText("TXT_KEY_POPUP_ELECTION_OPTION",
								kLoopVote.getTextKeyWide(), getVoteRequired(eLoopVote, eVS),
								countPossibleVote(eLoopVote, eVS));
						addVoteTriggered(eVS, kOptionData);
						bSecretaryGeneralVote = true;
						break;
					}
				}
			}
		}

		if (!bSecretaryGeneralVote &&
			eSecretaryGeneral != NO_TEAM && eSecretaryPlayer != NO_PLAYER)
		{
			VoteSelectionData* pData = addVoteSelection(eVS);
			if (pData != NULL)
			{
				if (GET_PLAYER(eSecretaryPlayer).isHuman())
				{
					CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_CHOOSEELECTION);
					if (pInfo != NULL)
					{
						pInfo->setData1(pData->getID());
						gDLL->getInterfaceIFace()->addPopup(pInfo, eSecretaryPlayer);
					}
				}
				else
				{
					setVoteChosen(GET_TEAM(eSecretaryGeneral).AI_chooseElection(*pData),
							pData->getID());
				}
			}
			else setVoteTimer(eVS, 0);
		}
	}
}

bool CvGame::isEventActive(EventTriggerTypes eTrigger) const
{
	return (std::find(m_aeInactiveTriggers.begin(), m_aeInactiveTriggers.end(), // advc: Use STL (or better make this a std::set?)
			eTrigger) == m_aeInactiveTriggers.end());
}

void CvGame::initEvents()
{
	for (int iTrigger = 0; iTrigger < GC.getNumEventTriggerInfos(); ++iTrigger)
	{
		if (isOption(GAMEOPTION_NO_EVENTS) ||
			getSorenRandNum(100, "Event Active?") >=
			GC.getInfo((EventTriggerTypes)iTrigger).getPercentGamesActive())
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
	FOR_EACH_ENUM(Civilization)
	{
		if (isCivEverActive(eLoopCivilization))
		{
			if (eUnit == GC.getInfo(eLoopCivilization).getCivilizationUnits(
				GC.getInfo(eUnit).getUnitClassType()))
			{
				return true;
			}
		}
	}
	return false;
}

bool CvGame::isBuildingEverActive(BuildingTypes eBuilding) const
{
	FOR_EACH_ENUM(Civilization)
	{
		if (isCivEverActive(eLoopCivilization))
		{
			if (eBuilding == GC.getInfo(eLoopCivilization).getCivilizationBuildings(
				GC.getInfo(eBuilding).getBuildingClassType()))
			{
				return true;
			}
		}
	}
	return false;
}

void CvGame::processBuilding(BuildingTypes eBuilding, int iChange)
{
	FOR_EACH_ENUM(VoteSource)
	{
		if (GC.getInfo(eBuilding).getVoteSourceType() == eLoopVoteSource)
			changeDiploVote(eLoopVoteSource, iChange);
	}
}

// advc.314: Between 0 and GOODY_BUFF_PEAK_MULTIPLIER, depending on game turn.
scaled CvGame::goodyHutEffectFactor(
	/*  Use true when a goody hut effect is supposed to increase with
		the game speed. When set to false, the turn numbers in this
		function are still game-speed adjusted. */
	bool bSpeedAdjust) const
{
	static int const iGOODY_BUFF_START_TURN = GC.getDefineINT("GOODY_BUFF_START_TURN");
	static int const iGOODY_BUFF_PEAK_TURN = GC.getDefineINT("GOODY_BUFF_PEAK_TURN");
	static int const iGOODY_BUFF_PEAK_MULTIPLIER = GC.getDefineINT("GOODY_BUFF_PEAK_MULTIPLIER");
	CvGameSpeedInfo const& kSpeed = GC.getInfo(getGameSpeedType());
	scaled rTurnsSpeedFactor = per100(kSpeed.getGrowthPercent());
	scaled rWorldFactor = 1;
		// Not sure if map-size adjustment is a good idea
		//=per100(GC.getInfo(GC.getMap().getWorldSize()).getResearchPercent());
	scaled rFinalSpeedFactor = (bSpeedAdjust ?
			per100(kSpeed.getTrainPercent()) * rWorldFactor : 1);
	scaled rStartTurn = scaled::max(0, iGOODY_BUFF_START_TURN * rTurnsSpeedFactor);
	scaled rPeakTurn = scaled::max(rStartTurn, iGOODY_BUFF_PEAK_TURN * rTurnsSpeedFactor);
	scaled rPeakMult = std::max(1, iGOODY_BUFF_PEAK_MULTIPLIER);
	/*  Exponent for power-law function; aiming for a function shape that
		resembles the graphs on the Info tab. */
	scaled rExp = fixp(1.25);
	// (or rather: the inverse of the gradient)
	scaled rGradient = (rPeakTurn - rStartTurn).pow(rExp) / (rPeakMult - 1);
	rGradient.clamp(1, 500);
	scaled t = getGameTurn();
	/*  Function through (rStartTurn, 1) and (rPeakTurn, rPeakMult)
		[^that's assuming bSpeedAdjust=false] */
	scaled r = rFinalSpeedFactor * std::min(rPeakMult,
			(rGradient + (scaled::max(0, t - rStartTurn).pow(rExp))) / rGradient);
	return r;
}

// <advc.004m>
GlobeLayerTypes CvGame::getCurrentLayer() const
{
	return m_eCurrentLayer;
}

// Used by CvMainInterface.py to tell the DLL which layer is active
void CvGame::reportCurrentLayer(GlobeLayerTypes eLayer)
{
	if(m_bLayerFromSavegame && eLayer != m_eCurrentLayer)
	{
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
		bool bDebug) const
{
	CvCity const* pVSCity = getVoteSourceCity(eVS, eObserver, bDebug);
	std::pair<int,int> r = std::make_pair(-1,-1);
	if(pVSCity == NULL)
		return r;
	r.first = pVSCity->getX();
	r.second = pVSCity->getY();
	return r;
}


CvCity* CvGame::getVoteSourceCity(VoteSourceTypes eVS, TeamTypes eObserver, bool bDebug) const
{
	BuildingTypes eVSBuilding = getVoteSourceBuilding(eVS);
	if(eVSBuilding == NO_BUILDING)
		return NULL;
	for(int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayer const& kOwner = GET_PLAYER((PlayerTypes)i);
		if(!kOwner.isAlive())
			continue;
		FOR_EACH_CITY_VAR(c, kOwner)
		{
			if(eObserver != NO_TEAM && !c->isRevealed(eObserver, bDebug))
				continue;
			if(c->getNumBuilding(eVSBuilding) > 0)
				return c;
		}
	}
	return NULL;
}

// advc: Used in several places and I want to make a small change
bool CvGame::isFreeStartEraBuilding(BuildingTypes eBuilding) const
{
	CvBuildingInfo const& kBuilding = GC.getInfo(eBuilding);
	return (kBuilding.getFreeStartEra() != NO_ERA &&
			getStartEra() >= kBuilding.getFreeStartEra() &&
			// <advc.126>
			(kBuilding.getMaxStartEra() == NO_ERA ||
			kBuilding.getMaxStartEra() >= getStartEra())); // </advc.126>
}


BuildingTypes CvGame::getVoteSourceBuilding(VoteSourceTypes eVS) const
{
	FOR_EACH_ENUM(Building)
	{
		if(GC.getInfo(eLoopBuilding).getVoteSourceType() == eVS)
			return eLoopBuilding;
	}
	return NO_BUILDING;
} // </advc.127b>

// advc.052:
void CvGame::setScenario(bool b)
{
	m_bScenario = b;
	/*  These two apparently check the same thing via the EXE,
		probably by checking the ending of the map script name,
		which is going to be somewhat slow. */
	FAssert(m_bScenario == gDLL->isWBMapScript());
	FAssert(m_bScenario == GC.getInitCore().getWBMapScript());
}

// advc.250b:
StartPointsAsHandicap const& CvGame::startPointsAsHandicap() const
{
	return *m_pSpah;
}

// advc.106i:
void CvGame::setHallOfFame(CvHallOfFameInfo* pHallOfFame)
{
	m_pHallOfFame = pHallOfFame;
}

// advc:
std::set<int>& CvGame::getActivePlayerCycledGroups()
{
	return m_ActivePlayerCycledGroups; // Was public; now protected.
}
