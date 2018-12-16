// game.cpp

#include "CvGameCoreDLL.h"
#include "CvGameCoreUtils.h"
#include "CvGame.h"
#include "CvMap.h"
#include "CvPlot.h"
#include "CvPlayerAI.h"
#include "CvRandom.h"
#include "CvTeamAI.h"
#include "CvGlobals.h"
#include "CvInitCore.h"
#include "CvMapGenerator.h"
#include "CvArtFileMgr.h"
#include "CvDiploParameters.h"
#include "CvReplayMessage.h"
#include "CyArgsList.h"
#include "CvInfos.h"
#include "CvPopupInfo.h"
#include "FProfiler.h"
#include "CvReplayInfo.h"
#include "CvGameTextMgr.h"
#include <set>
#include "CvEventReporter.h"
#include "CvMessageControl.h"

// interface uses
#include "CvDLLInterfaceIFaceBase.h"
#include "CvDLLEngineIFaceBase.h"
#include "CvDLLPythonIFaceBase.h"

#include "BetterBTSAI.h" // bbai
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
}

void CvGame::init(HandicapTypes eHandicap)
{
	int iI;

	//--------------------------------
	// Init saved data
	reset(eHandicap);

	//--------------------------------
	// Init containers
	m_deals.init();
	m_voteSelections.init();
	m_votesTriggered.init();

	m_mapRand.init(GC.getInitCore().getMapRandSeed() % 73637381);
	m_sorenRand.init(GC.getInitCore().getSyncRandSeed() % 52319761);

	//--------------------------------
	// Init non-saved data

	// <advc.108>
	m_iNormalizationLevel = GC.getDefineINT("NORMALIZE_STARTPLOTS_AGGRESSIVELY") > 0 ?
			3 : 1;
	if(m_iNormalizationLevel == 1 && isGameMultiPlayer())
		m_iNormalizationLevel = 2;
	// </advc.108>

	//--------------------------------
	// Init other game data

	// Turn off all MP options if it's a single player game
	if (GC.getInitCore().getType() == GAME_SP_NEW ||
		GC.getInitCore().getType() == GAME_SP_SCENARIO)
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
			if (GC.getInitCore().getSlotStatus((PlayerTypes)i) == SS_TAKEN)
			{
				aiTeams[iNumPlayers] = GC.getInitCore().getTeam((PlayerTypes)i);
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
			if (GC.getInitCore().getSlotStatus((PlayerTypes)i) == SS_TAKEN)
			{
				GC.getInitCore().setTeam((PlayerTypes)i, (TeamTypes)aiTeams[iNumPlayers]);
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

			GC.getInitCore().setAdminPassword(szRandomPassword);
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
	int nAI = 0; // advc.250b: Just for disabling SPaH in game w/o any AI civs
	// K-Mod: Adjust the game handicap level to be the average of all the human player's handicap.
	// (Note: in the original bts rules, it would always set to Nobel if the humans had different handicaps)
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
			nAI++; // </advc.250b>
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
	/*  <advc.127> Set m_eAIHandicap to the average of AI handicaps. Scenarios can
		assign unequal AI handicaps.
		(Then again, scenarios don't call  setInitialItems, so this loop is
		really superfluous.) */
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
	// </advc.127>
	// <advc.250b>
	if(!isOption(GAMEOPTION_ADVANCED_START) || nAI == 0)
		setOption(GAMEOPTION_SPAH, false);
	if(isOption(GAMEOPTION_SPAH))
		// Reassigns start plots and start points
		spah.setInitialItems(); // </advc.250b>
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
		double maxMean = (::max(distr) + ::mean(distr)) / 2.0;
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
        CvDeal* d; int dummy;
        for(d = firstDeal(&dummy); d != NULL; d = nextDeal(&dummy))
            d->setInitialGameTurn(getGameTurn());
	} // </advc.250c>
	// </advc.251>
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)i);
		if (kPlayer.isAlive())
		{
			kPlayer.AI_updateFoundValues();
		}
	}
}


void CvGame::regenerateMap()
{
	int iI;

	if (GC.getInitCore().getWBMapScript())
	{
		return;
	}

	/*  <advc.004j> Not sure if the unmodded game or any mod included with AdvCiv
		uses script data, but can't hurt to reset it. CvDLLButtonPopup::
		launchMainMenuPopup wants to disallow map regeneration once script data
		has been set. */
	setScriptData("");
	for(int i = 0; i < GC.getMapINLINE().numPlots(); ++i) {
		CvPlot* p = GC.getMapINLINE().plotByIndexINLINE(i);
		if(p != NULL) {
			p->setScriptData("");
			/*  advc.021b: Otherwise, assignStartingPlots runs into trouble upon
				map regeneration when a script calls allowDefaultImpl after
				assigning starting plots. */
			p->setStartingPlot(false);
		}
	} // </advc.004j>

	setFinalInitialized(false);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		GET_PLAYER((PlayerTypes)iI).killUnits();
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		GET_PLAYER((PlayerTypes)iI).killCities();
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		GET_PLAYER((PlayerTypes)iI).killAllDeals();
	}
	
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
	{
		GC.getMapINLINE().setRevealedPlots(((TeamTypes)iI), false);
	}

	gDLL->getEngineIFace()->clearSigns();

	GC.getMapINLINE().erasePlots();

	CvMapGenerator::GetInstance().generateRandomMap();
	CvMapGenerator::GetInstance().addGameElements();

	gDLL->getEngineIFace()->RebuildAllPlots();
	// <advc.251>
	setGameTurn(0);
	setStartTurn(0);
	setStartTurnYear();
	m_iElapsedGameTurns = 0;
	// </advc.251>
	CvEventReporter::getInstance().resetStatistics();

	setInitialItems();

	initScoreCalculation();
	setFinalInitialized(true);

	GC.getMapINLINE().setupGraphical();
	gDLL->getEngineIFace()->SetDirty(GlobeTexture_DIRTY_BIT, true);
	gDLL->getEngineIFace()->SetDirty(MinimapTexture_DIRTY_BIT, true);
	gDLL->getInterfaceIFace()->setDirty(ColoredPlots_DIRTY_BIT, true);

	cycleSelectionGroups_delayed(1, false);
	// <advc.700>
	if(isOption(GAMEOPTION_RISE_FALL)) {
		riseFall.reset();
		riseFall.init();
	}
	else { // </advc.700>
		gDLL->getEngineIFace()->AutoSave(true);
		// <advc.004j> Somehow doesn't work with Adv. Start; DoM screen doesn't appear.
		if(!isOption(GAMEOPTION_ADVANCED_START))
			showDawnOfMan();
	} // </advc.004j>
	if (NO_PLAYER != getActivePlayer())
	{
		CvPlot* pPlot = GET_PLAYER(getActivePlayer()).getStartingPlot();

		if (NULL != pPlot)
		{
			/*  advc.003 (comment): This appears to have no effect. Perhaps the
				camera can't be moved at this point. Would have to be done at some
				later point I guess. Setting SelectionCamera_DIRTY_BIT doesn't
				seem to help either. */
			gDLL->getInterfaceIFace()->lookAt(pPlot->getPoint(), CAMERALOOKAT_NORMAL);
		}
	}
}

// <advc.004j>
void CvGame::showDawnOfMan() {

	if(getActivePlayer() == NO_PLAYER)
		return;
	// This appears to require an argument, and I've no clue which
	//gDLL->getPythonIFace()->callFunction("CvScreensInterface", "showDawnOfMan");
	// Instead (based on CvAllErasDawnOfManScreenEventManager.py):
	CvPopupInfo* dom = new CvPopupInfo();
	dom->setButtonPopupType(BUTTONPOPUP_PYTHON_SCREEN);
	dom->setText(L"showDawnOfMan");
	GET_PLAYER(getActivePlayer()).addPopup(dom);
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
	/*  advc.700: Need to call this explicitly due to the unusual way that
		RiseFall is initialized (from updateBlockadedPlots) */
	riseFall.reset();
}

// <advc.250c> Function body cut from CvGame::init. Changes marked in-line.
void CvGame::setStartTurnYear(int iTurn) {

    int iI;

    if (getGameTurn() == 0
            || iTurn > 0) // advc.250c
    {
        int iStartTurn = 0;

        for (iI = 0; iI < GC.getGameSpeedInfo(getGameSpeedType()).getNumTurnIncrements(); iI++)
        {
            iStartTurn += GC.getGameSpeedInfo(getGameSpeedType()).getGameTurnInfo(iI).iNumGameTurnsPerIncrement;
        }

        iStartTurn *= GC.getEraInfo(getStartEra()).getStartPercent();
        iStartTurn /= 100;
        setGameTurn(
			// advc.250c: Overwrite game turn calculation if iTurn set
            iTurn > 0 ? iTurn :
            iStartTurn);
    }

    setStartTurn(getGameTurn());

    if (getMaxTurns() == 0
            || iTurn > 0) // advc.250c
    {
        int iEstimateEndTurn = 0;

        for (iI = 0; iI < GC.getGameSpeedInfo(getGameSpeedType()).getNumTurnIncrements(); iI++)
        {
            iEstimateEndTurn += GC.getGameSpeedInfo(getGameSpeedType()).getGameTurnInfo(iI).iNumGameTurnsPerIncrement;
        }

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
            {
                setMaxTurns(getEstimateEndTurn() - getGameTurn());
            }
        }
    }
    else
    {
        setEstimateEndTurn(getGameTurn() + getMaxTurns());
    }

    setStartYear(GC.getDefineINT("START_YEAR"));
} // </advc.250c>

// FUNCTION: reset()
// Initializes data members that are serialized.
void CvGame::reset(HandicapTypes eHandicap, bool bConstructorCall)
{
	int iI;

	//--------------------------------
	// Uninit class
	uninit();

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
	m_iScreenWidth = m_iScreenHeight = 0; // advc.061

	m_eHandicap = eHandicap;
	// advc.127: (XML not loaded when constructor called)
	m_eAIHandicap = bConstructorCall ? NO_HANDICAP : (HandicapTypes)GC.getDefineINT("STANDARD_HANDICAP");
	m_ePausePlayer = NO_PLAYER;
	m_eBestLandUnit = NO_UNIT;
	m_eWinner = NO_TEAM;
	m_eVictory = NO_VICTORY;
	m_eGameState = GAMESTATE_ON;

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
	m_bAITurn = false; // advc.106b
	m_iTurnLoadedFromSave = -1; // advc.044
	// <advc.004m>
	if(bConstructorCall)
		m_bResourceLayer = true;
	else if(GC.getDefineINT("SHOW_RESOURCE_BUBBLES_AT_GAME_START") <= 0) {
		/*  This causes the resource layer to be disabled when returning to the
			main menu. (Rather than remembering the latest status.) */
		m_bResourceLayer = false;
	} // </advc.004m>
	m_bResourceLayerSet = false; // advc.003d
	m_bFeignSP = false; // advc.135c
}


void CvGame::initDiplomacy()
{
	PROFILE_FUNC();

	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		GET_TEAM((TeamTypes)iI).meet(((TeamTypes)iI), false);

		if (GET_TEAM((TeamTypes)iI).isBarbarian() || GET_TEAM((TeamTypes)iI).isMinorCiv())
		{
			for (int iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
			{
				if (iI != iJ)
				{
					GET_TEAM((TeamTypes)iI).declareWar(((TeamTypes)iJ), false, NO_WARPLAN);
				}
			}
		}
	}

	// Forced peace at the beginning of Advanced starts
	if (isOption(GAMEOPTION_ADVANCED_START)
			/*  advc.250b: No need to protect the AI from the player when human
				start is advanced, and AI won't start wars within 10 turns
				anyway. */
			&&!isOption(GAMEOPTION_SPAH)
		)
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


void CvGame::initFreeState()
{
	int iI, iJ, iK;
	if(GC.getInitCore().isScenario()) {
		setScenario(true); // advc.052
		AI().AI_initScenario(); // advc.104u
	}
	else { // advc.051: (Moved up.) Don't force 0 gold in scenarios.
		for (iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				GET_PLAYER((PlayerTypes)iI).initFreeState();
			}
		}
	}
	for (iI = 0; iI < GC.getNumTechInfos(); iI++)
	{
		for (iJ = 0; iJ < MAX_TEAMS; iJ++)
		{	// <advc.003>
			if(!GET_TEAM((TeamTypes)iJ).isAlive())
				continue; // </advc.003>
			bool bValid = false;
			if (//(GC.getHandicapInfo(getHandicapType()).isFreeTechs(iI)) || // disabled by K-Mod. (moved & changed. See below)
					(!(GET_TEAM((TeamTypes)iJ).isHuman()) && GC.getHandicapInfo(getHandicapType()).isAIFreeTechs(iI)
				/*  advc.001: Barbarians receiving free AI tech might be a bug.
					If all AI civs start with the same tech, barbarians will
					get that tech soon either way, but with this fix at least
					not immediately. */
					&& iJ != (int)BARBARIAN_TEAM
					// advc.250c:
					&& !isOption(GAMEOPTION_ADVANCED_START)) ||
					(GC.getTechInfo((TechTypes)iI).getEra() < getStartEra()))
				bValid = true;
			if (!bValid) {
				for (iK = 0; iK < MAX_PLAYERS; iK++)
				{
					CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iK); // K-Mod
					// <advc.003>
					if(!kLoopPlayer.isAlive() || kLoopPlayer.getTeam() != iJ)
						continue; // </advc.003>
					/*  <advc.250b><advc.250c> Always grant civ-specific tech,
						but not tech from handicap if Advanced Start
						except to human civs that don't actually start
						Advanced (SPaH option). */
					if (GC.getCivilizationInfo(kLoopPlayer.getCivilizationType()).isCivilizationFreeTechs(iI))
					{
						bValid = true;
						break;
					}
					if (!bValid &&
						GC.getHandicapInfo(kLoopPlayer.getHandicapType()).isFreeTechs(iI)
						&& (!isOption(GAMEOPTION_ADVANCED_START) ||
						(isOption(GAMEOPTION_SPAH) &&
						GET_TEAM((TeamTypes)iJ).isHuman()))
						// </advc.250b></advc.250c>
						) // K-Mod (give techs based on player handicap, not game handicap.)
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
			{ 
				GC.getMapINLINE().setRevealedPlots((TeamTypes)iJ, true, true); 
			}
		}
	}
}

// <advc.051>
void CvGame::initScenario() {

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
	}
	// </advc.030>
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

void CvGame::assignStartingPlots()
{
	PROFILE_FUNC();

	// (original bts code deleted) // advc.003	
	// K-Mod. Same functionality, but much faster and easier to read.
	//
	// First, make a list of all the pre-marked starting plots on the map.
	std::vector<CvPlot*> starting_plots;
	for (int i = 0; i < GC.getMapINLINE().numPlotsINLINE(); i++)
	{
		gDLL->callUpdater();	// allow window updates during launch

		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(i);
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
	{	/*  advc.003 (comment): This assignment is just a starting point for
			normalizeStartingPlotLocations */
		for (int iPass = 0; iPass < 2 * MAX_PLAYERS; ++iPass)
		{
			bool bStartFound = false;
			int iRandOffset = getSorenRandNum(countCivTeamsAlive(), "Team Starting Plot");

			for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
			{
				int iLoopTeam = ((iI + iRandOffset) % MAX_CIV_TEAMS);

				if (GET_TEAM((TeamTypes)iLoopTeam).isAlive())
				{
					for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
					{	// <advc.003>
						CvPlayer& member = GET_PLAYER((PlayerTypes)iJ);
						if(!member.isAlive())
							continue; // </advc.003>
						if (member.getTeam() == iLoopTeam
								// <advc.108b>
								&& !newPlotFound[iJ]) {
							if(member.getStartingPlot() == NULL)
								member.setStartingPlot(member.findStartingPlot(), true);
							if(member.getStartingPlot() != NULL) {
								playerOrder.push_back(member.getID());
								bStartFound = true;
								newPlotFound[member.getID()] = true;
								break;
							}
						} // </advc.108b>
					}
				}
			}

			if (!bStartFound)
			{
				break;
			}
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
	/*else if (isGameMultiPlayer())
	{
		int iRandOffset = getSorenRandNum(countCivPlayersAlive(), "Player Starting Plot");

		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			int iLoopPlayer = ((iI + iRandOffset) % MAX_CIV_PLAYERS);

			if (GET_PLAYER((PlayerTypes)iLoopPlayer).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iLoopPlayer).isHuman())
				{
					if (GET_PLAYER((PlayerTypes)iLoopPlayer).getStartingPlot() == NULL)
					{
						GET_PLAYER((PlayerTypes)iLoopPlayer).setStartingPlot(GET_PLAYER((PlayerTypes)iLoopPlayer).findStartingPlot(), true);
						playerOrder.push_back(iLoopPlayer);
					}
				}
			}
		}

		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (!(GET_PLAYER((PlayerTypes)iI).isHuman()))
				{
					if (GET_PLAYER((PlayerTypes)iI).getStartingPlot() == NULL)
					{
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

		for (int iI = 0; iI < iHumanSlot; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (!(GET_PLAYER((PlayerTypes)iI).isHuman()))
				{
					if (GET_PLAYER((PlayerTypes)iI).getStartingPlot() == NULL)
					{
						GET_PLAYER((PlayerTypes)iI).setStartingPlot(GET_PLAYER((PlayerTypes)iI).findStartingPlot(), true);
						playerOrder.push_back(iI);
					}
				}
			}
		}

		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).isHuman())
				{
					if (GET_PLAYER((PlayerTypes)iI).getStartingPlot() == NULL)
					{
						GET_PLAYER((PlayerTypes)iI).setStartingPlot(GET_PLAYER((PlayerTypes)iI).findStartingPlot(), true);
						playerOrder.push_back(iI);
					}
				}
			}
		}

		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).getStartingPlot() == NULL)
				{
					GET_PLAYER((PlayerTypes)iI).setStartingPlot(GET_PLAYER((PlayerTypes)iI).findStartingPlot(), true);
					playerOrder.push_back(iI);
				}
			}
		}
	}
	
	//Now iterate over the player starts in the original order and re-place them.
	//std::vector<int>::iterator playerOrderIter;
	for (playerOrderIter = playerOrder.begin(); playerOrderIter != playerOrder.end(); ++playerOrderIter)
	{
		GET_PLAYER((PlayerTypes)(*playerOrderIter)).setStartingPlot(GET_PLAYER((PlayerTypes)(*playerOrderIter)).findStartingPlot(), true);
	}*/
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
		int val = civ.AI_foundValue(p->getX_INLINE(), p->getY_INLINE(), -1, true);
		FAssertMsg(val > 0, "Bad starting position");
		// minus val for descending order
		startPlots.push_back(std::make_pair<int,CvPlot*>(-val, p));
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
							int iDist = GC.getMapINLINE().calculatePathDistance(pPlotI, pPlotJ);
							if (iDist == -1)
							{
								// 5x penalty for not being on the same area, or having no passable route
								iDist = 5*plotDistance(pPlotI->getX_INLINE(), pPlotI->getY_INLINE(), pPlotJ->getX_INLINE(), pPlotJ->getY_INLINE());
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
	 3: very high (very strong starting plots, low variance);  BtS/ K-Mod behavior */
int CvGame::getNormalizationLevel() const {

	return m_iNormalizationLevel;
} // </advc.108


void CvGame::normalizeAddRiver()
{
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			CvPlot* pStartingPlot = GET_PLAYER((PlayerTypes)iI).getStartingPlot();

			if (pStartingPlot != NULL)
			{
				if (!pStartingPlot->isFreshWater())
				{
					// if we will be able to add a lake, then use old river code
					if (normalizeFindLakePlot((PlayerTypes)iI) != NULL)
					{
						//CvMapGenerator::GetInstance().doRiver(pStartingPlot);
						// K-Mod. If we can have a lake then we don't always need a river.
						// Also, the river shouldn't always start on the SE corner of our site.
						if (getSorenRandNum(10, "normalize add river") < (pStartingPlot->isCoastalLand() ? 5 : 7))
						{
							CvPlot* pRiverPlot = pStartingPlot->getInlandCorner();
							if (pRiverPlot)
								CvMapGenerator::GetInstance().doRiver(pRiverPlot);
						}
						// K-Mod end.
					}
					// otherwise, use new river code which is much more likely to succeed
					else
					{
						CvMapGenerator::GetInstance().addRiver(pStartingPlot);
					}
					// add floodplains to any desert tiles the new river passes through
					for (int iK = 0; iK < GC.getMapINLINE().numPlotsINLINE(); iK++)
					{
						CvPlot* pPlot = GC.getMapINLINE().plotByIndexINLINE(iK);
						FAssert(pPlot != NULL);

						for (int iJ = 0; iJ < GC.getNumFeatureInfos(); iJ++)
						{
							if (GC.getFeatureInfo((FeatureTypes)iJ).isRequiresRiver())
							{
								if (pPlot->canHaveFeature((FeatureTypes)iJ))
								{
									if (GC.getFeatureInfo((FeatureTypes)iJ).getAppearanceProbability() == 10000)
									{
										if (pPlot->getBonusType() != NO_BONUS)
										{
											pPlot->setBonusType(NO_BONUS);
										}
										pPlot->setFeatureType((FeatureTypes)iJ);
										break;
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


void CvGame::normalizeRemovePeaks()
{
	// <advc.108>
	double prRemoval = 1;
	if(m_iNormalizationLevel <= 1)
		prRemoval = GC.getDefineINT("REMOVAL_CHANCE_PEAK") / 100.0;
	// </advc.108>

	CvPlot* pStartingPlot;
	CvPlot* pLoopPlot;
	int iRange;
	int iDX, iDY;
	int iI;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			pStartingPlot = GET_PLAYER((PlayerTypes)iI).getStartingPlot();

			if (pStartingPlot != NULL)
			{
				iRange = 3;

				for (iDX = -(iRange); iDX <= iRange; iDX++)
				{
					for (iDY = -(iRange); iDY <= iRange; iDY++)
					{
						pLoopPlot = plotXY(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iDX, iDY);

						if (pLoopPlot != NULL)
						{
							if (pLoopPlot->isPeak()
								// advc.108:
								&& ::bernoulliSuccess(prRemoval, "advc.108")
								)
							{
								pLoopPlot->setPlotType(PLOT_HILLS);
							}
						}
					}
				}
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

CvPlot* CvGame::normalizeFindLakePlot(PlayerTypes ePlayer)
{
	if (!GET_PLAYER(ePlayer).isAlive())
	{
		return NULL;
	}

	CvPlot* pStartingPlot = GET_PLAYER(ePlayer).getStartingPlot();
	if (pStartingPlot != NULL)
	{
		if (!(pStartingPlot->isFreshWater()))
		{
			// K-Mod. Shuffle the order that plots are checked.
			int aiShuffle[NUM_CITY_PLOTS];
			shuffleArray(aiShuffle, NUM_CITY_PLOTS, getMapRand());
			// K-Mod end
			for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
			{
				//CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iJ);
				CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), aiShuffle[iJ]); // K-Mod

				if (pLoopPlot != NULL)
				{
					if (!(pLoopPlot->isWater()))
					{
						if (!(pLoopPlot->isCoastalLand()))
						{
							if (!(pLoopPlot->isRiver()))
							{
								if (pLoopPlot->getBonusType() == NO_BONUS)
								{
									bool bStartingPlot = false;

									for (int iK = 0; iK < MAX_CIV_PLAYERS; iK++)
									{
										if (GET_PLAYER((PlayerTypes)iK).isAlive())
										{
											if (GET_PLAYER((PlayerTypes)iK).getStartingPlot() == pLoopPlot)
											{
												bStartingPlot = true;
												break;
											}
										}
									}

									if (!bStartingPlot)
									{
										return pLoopPlot;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return NULL;
}

// advc.003: Refactored
void CvGame::normalizeRemoveBadFeatures()
{
	// advc.108
	int const iThreshBadFeatPerCity = GC.getDefineINT("THRESH-BAD-FEAT-PER-CITY");

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayerAI const& civ = GET_PLAYER((PlayerTypes)iI);
		if(!civ.isAlive())
			continue;
		CvPlot* pStartingPlot = civ.getStartingPlot();
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
			CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(),
					pStartingPlot->getY_INLINE(), iJ);
			if(pLoopPlot != NULL && pLoopPlot->getFeatureType() != NO_FEATURE) {
				if(GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_FOOD) <= 0 &&
						GC.getFeatureInfo(pLoopPlot->getFeatureType()).getYieldChange(YIELD_PRODUCTION) <= 0) {
					// <advc.108>
					if(::plotDistance(pLoopPlot, pStartingPlot) < 2 ||
							::bernoulliSuccess(prRemoval, "advc.108")) // </advc.108
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
				CvPlot* pLoopPlot = plotXY(pStartingPlot->getX_INLINE(),
						pStartingPlot->getY_INLINE(), iX, iY);
				if(pLoopPlot == NULL)
					continue;
				int iDistance = plotDistance(pStartingPlot->getX_INLINE(),
						pStartingPlot->getY_INLINE(),
						pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
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


void CvGame::normalizeRemoveBadTerrain()
{
	// <advc.108>
	double prKeep = 0;
	if(m_iNormalizationLevel <= 1)
		prKeep = 1 - GC.getDefineINT("REMOVAL_CHANCE_BAD_TERRAIN") / 100.0;
	// </advc.108>

	CvPlot* pLoopPlot;
	int iI, iK;
	int iX, iY;

	int iCityRange = CITY_PLOTS_RADIUS;
	int iExtraRange = 1;
	int iMaxRange = iCityRange + iExtraRange;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++) // advc.003: Some refactoring in this loop
	{
		CvPlayerAI const& civ = GET_PLAYER((PlayerTypes)iI);
		if(!civ.isAlive())
			continue;
		CvPlot* pStartingPlot = civ.getStartingPlot();
		if(pStartingPlot == NULL)
			continue;
		for (iX = -iMaxRange; iX <= iMaxRange; iX++)
		{
			for (iY = -iMaxRange; iY <= iMaxRange; iY++)
			{
				pLoopPlot = plotXY(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iX, iY);
				if(pLoopPlot == NULL)
					continue;
				int iDistance = plotDistance(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
				if(iDistance > iMaxRange)
					continue;
				if(!pLoopPlot->isWater() && (iDistance <= iCityRange ||
						pLoopPlot->isCoastalLand() || getSorenRandNum(
						1 + iDistance - iCityRange, "Map Upgrade Terrain Food") == 0))
				{
					CvTerrainInfo const& ti = GC.getTerrainInfo(pLoopPlot->getTerrainType());
					int iPlotFood = ti.getYield(YIELD_FOOD);
					int iPlotProduction = ti.getYield(YIELD_PRODUCTION);
					if (iPlotFood + iPlotProduction > 1)
						continue;
					/*  <advc.003> I think the BtS code ends up replacing
						Desert with Desert when there's a feature, but let's
						rather handle Desert features explicitly. */
					if(pLoopPlot->getFeatureType() != NO_FEATURE &&
							GC.getFeatureInfo(pLoopPlot->getFeatureType()).
							getYieldChange(YIELD_FOOD) + iPlotFood >= 2)
						continue; // </advc.003>
					// <advc.108>
					if(::bernoulliSuccess(prKeep, "advc.108")) {
						if(iPlotFood > 0 ||
							/*  advc.129b: Two chances of removal for Snow river
								(BuildModifier=50), but not for Desert river. */
								(pLoopPlot->isRiver() && ti.getBuildModifier() < 30) ||
								::bernoulliSuccess(prKeep, "advc.108"))
							continue;
					} // </advc.108>
					int const iTargetTotal = 2;
					int iTargetFood = 1;
					if (pLoopPlot->getBonusType(civ.getTeam()) != NO_BONUS)
					{
						iTargetFood = 1;
					}
					else if (iPlotFood == 1 || iDistance <= iCityRange)
					{
						iTargetFood = 1 + getSorenRandNum(2, "Map Upgrade Terrain Food");
					}
					else
					{
						iTargetFood = pLoopPlot->isCoastalLand() ? 2 : 1;
					}
					for (iK = 0; iK < GC.getNumTerrainInfos(); iK++)
					{
						CvTerrainInfo const& repl = GC.getTerrainInfo((TerrainTypes)iK);
						if (repl.isWater())
							continue;
						if (repl.getYield(YIELD_FOOD) >= iTargetFood &&
								repl.getYield(YIELD_FOOD) +
								repl.getYield(YIELD_PRODUCTION) == iTargetTotal)
						{
							if (pLoopPlot->getFeatureType() == NO_FEATURE ||
									GC.getFeatureInfo(pLoopPlot->getFeatureType()).
									isTerrain(iK))
							{
								pLoopPlot->setTerrainType((TerrainTypes)iK);
							}
						}
					}
				}
			}
		}
	}
}


void CvGame::normalizeAddFoodBonuses()
{
	bool bIgnoreLatitude = pythonIsBonusIgnoreLatitudes();
	int iFoodPerPop = GC.getFOOD_CONSUMPTION_PER_POPULATION(); // K-Mod

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		const CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI); // K-Mod

		if (kLoopPlayer.isAlive())
		{
			CvPlot* pStartingPlot = kLoopPlayer.getStartingPlot();

			if (pStartingPlot != NULL)
			{
				int iFoodBonus = 0;
				int iGoodNatureTileCount = 0;

				//for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
				for (int iJ = 1; iJ < NUM_CITY_PLOTS; iJ++) // K-Mod. Don't count the city plot.
				{
					CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iJ);

					if (pLoopPlot != NULL)
					{
						BonusTypes eBonus = pLoopPlot->getBonusType(kLoopPlayer.getTeam());

						if (eBonus != NO_BONUS)
						{
							if (GC.getBonusInfo(eBonus).getYieldChange(YIELD_FOOD) > 0)
							{
								if ((GC.getBonusInfo(eBonus).getTechCityTrade() == NO_TECH) || (GC.getTechInfo((TechTypes)(GC.getBonusInfo(eBonus).getTechCityTrade())).getEra() <= getStartEra()))
								{
									if (pLoopPlot->isWater())
									{
										iFoodBonus += 2;
									}
									else
									{
										//iFoodBonus += 3;

										// K-Mod. Bonus which only give 3 food with their improvement should not be worth 3 points. (ie. plains-cow should not be the only food resource.)
										/* first attempt - this doesn't work, because "max yield" essentially means +2 food on any plot. That isn't what we want.
										if (pLoopPlot->calculateMaxYield(YIELD_FOOD) >= 2*iFoodPerPop) // ie. >= 4
											iFoodBonus += 3;
										else
											iFoodBonus += 2; */
										int iNaturalFood = pLoopPlot->calculateBestNatureYield(YIELD_FOOD, kLoopPlayer.getTeam());
										int iHighFoodThreshold = 2*iFoodPerPop; // ie. 4 food.
										bool bHighFood = iNaturalFood + 1 >= iHighFoodThreshold; // (+1 just as a shortcut to save time for obvious cases.)

										for (ImprovementTypes eImp = (ImprovementTypes)0; !bHighFood && eImp < GC.getNumImprovementInfos(); eImp=(ImprovementTypes)(eImp+1))
										{
											if (GC.getImprovementInfo(eImp).isImprovementBonusTrade(eBonus))
											{
												bHighFood = iNaturalFood + pLoopPlot->calculateImprovementYieldChange(eImp, YIELD_FOOD, (PlayerTypes)iI, false, false) >= iHighFoodThreshold;
											}
										}
										iFoodBonus += bHighFood ? 3 : 2;
										// K-Mod end
									}
								}
							}
							else if (pLoopPlot->calculateBestNatureYield(YIELD_FOOD, kLoopPlayer.getTeam()) >= iFoodPerPop)
						    {
						        iGoodNatureTileCount++;
						    }
						}
						else
						{
                            if (pLoopPlot->calculateBestNatureYield(YIELD_FOOD, kLoopPlayer.getTeam()) >= iFoodPerPop+1)
						    {
						        iGoodNatureTileCount++;
						    }
						}
					}
				}
				
				int iTargetFoodBonusCount = 3;
				// advc.108: (Don't do this after all:)
				//int iTargetFoodBonusCount = m_iNormalizationLevel;
				iTargetFoodBonusCount += std::max(0, 2-iGoodNatureTileCount); // K-Mod

				// K-Mod. I've rearranged a couple of things to make it a bit more efficient and easier to read.
				for (int iJ = 1; iJ < NUM_CITY_PLOTS; iJ++)
				{
					if (iFoodBonus >= iTargetFoodBonusCount)
					{
						break;
					}

					CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iJ);

					if (pLoopPlot && pLoopPlot->getBonusType() == NO_BONUS)
					{
						for (int iK = 0; iK < GC.getNumBonusInfos(); iK++)
						{
							const CvBonusInfo& kLoopBonus = GC.getBonusInfo((BonusTypes)iK);
							if (kLoopBonus.isNormalize() && kLoopBonus.getYieldChange(YIELD_FOOD) > 0)
							{
								if ((kLoopBonus.getTechCityTrade() == NO_TECH) || (GC.getTechInfo((TechTypes)(kLoopBonus.getTechCityTrade())).getEra() <= getStartEra()))
								{
									if (GET_TEAM(kLoopPlayer.getTeam()).isHasTech((TechTypes)kLoopBonus.getTechReveal()))
									{
								      // <advc.108> Don't place the food resource on a bad feature
									  FeatureTypes ft = pLoopPlot->getFeatureType();
									  bool skip = false;
									  if(ft != NO_FEATURE) {
										CvFeatureInfo& feat = GC.getFeatureInfo(ft);
										skip = true;
										if(m_iNormalizationLevel >= 3 || feat.getYieldChange(YIELD_FOOD) > 0 ||
										    feat.getYieldChange(YIELD_PRODUCTION) > 0)
										  skip = false;
									  }
									  if(!skip) // </advc.108>
										if (pLoopPlot->canHaveBonus(((BonusTypes)iK), bIgnoreLatitude))
										{
											pLoopPlot->setBonusType((BonusTypes)iK);
											if (pLoopPlot->isWater())
											{
												iFoodBonus += 2;
											}
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
														bHighFood = iNaturalFood + pLoopPlot->calculateImprovementYieldChange(eImp, YIELD_FOOD, (PlayerTypes)iI, false, false) >= iHighFoodThreshold;
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
						}
					}
				}
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
					pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iJ);

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

					pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iJ);

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


void CvGame::normalizeAddExtras()
{
	bool bIgnoreLatitude = pythonIsBonusIgnoreLatitudes();

	int iTotalValue = 0;
	int iPlayerCount = 0;
	int iBestValue = 0;
	int iWorstValue = MAX_INT;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			CvPlot* pStartingPlot = GET_PLAYER((PlayerTypes)iI).getStartingPlot();

			if (pStartingPlot != NULL)
			{
				int iValue = GET_PLAYER((PlayerTypes)iI).AI_foundValue(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), -1, true);
				iTotalValue += iValue;
                iPlayerCount++;
                
                iBestValue = std::max(iValue, iBestValue);
                iWorstValue = std::min(iValue, iWorstValue);
			}
		}
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
			if (kLoopPlayer.AI_foundValue(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), -1, true) >= iTargetValue)
			{
				if (gMapLogLevel > 0)
					logBBAI("    Player %d doesn't need any more features.", iI); // K-Mod
				break;
			}
			if (getSorenRandNum((iCount + 2), "Setting Feature Type") <= 1)
			{
				CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), aiShuffle[iJ]);

				if (pLoopPlot != NULL)
				{
					if (pLoopPlot != pStartingPlot)
					{
						if (pLoopPlot->getBonusType() == NO_BONUS)
						{
							if (pLoopPlot->getFeatureType() == NO_FEATURE)
							{
								for (int iK = 0; iK < GC.getNumFeatureInfos(); iK++)
								{
									if ((GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_FOOD) + GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(YIELD_PRODUCTION)) > 0)
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
							}

							iFeatureCount += (pLoopPlot->getFeatureType() != NO_FEATURE) ? 1 : 0;
						}
					}
				}
			}
		}

		int iCoastFoodCount = 0;
		int iOceanFoodCount = 0;
		int iOtherCount = 0;
		int iWaterCount = 0;
		for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
		{
			CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iJ);
			if (pLoopPlot != NULL)
			{
				if (pLoopPlot != pStartingPlot)
				{
					if (pLoopPlot->isWater())
					{
						iWaterCount++;
						if (pLoopPlot->getBonusType() != NO_BONUS)
						{
							if (pLoopPlot->isAdjacentToLand())
							{
								iCoastFoodCount++;
							}
							else
							{
								iOceanFoodCount++;
							}
						}
					}
					else if (pLoopPlot->getBonusType(
								// <advc.108> Don't count unrevealed bonuses
								m_iNormalizationLevel > 1 ?
								NO_TEAM : kLoopPlayer.getTeam()) // </advc.108>
								!= NO_BONUS)
							iOtherCount++;
				}
			}
		}

		bool bLandBias = (iWaterCount > NUM_CITY_PLOTS / 2);

		shuffleArray(aiShuffle, NUM_CITY_PLOTS, getMapRand());

		for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
		{
			CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), aiShuffle[iJ]);

			if ((pLoopPlot != NULL) && (pLoopPlot != pStartingPlot))
			{
				if (getSorenRandNum(((bLandBias && pLoopPlot->isWater()) ? 2 : 1), "Placing Bonuses") == 0)
				{
					if ((iOtherCount * 3 + iOceanFoodCount * 2 + iCoastFoodCount * 2) >= 12)
					{
						break;
					}

					if (kLoopPlayer.AI_foundValue(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), -1, true) >= iTargetValue)
					{
						if (gMapLogLevel > 0)
							logBBAI("    Player %d doesn't need any more bonuses.", iI); // K-Mod
						break;
					}

					bool bCoast = (pLoopPlot->isWater() && pLoopPlot->isAdjacentToLand());
					bool bOcean = (pLoopPlot->isWater() && !bCoast);
					if ((pLoopPlot != pStartingPlot)
						&& !(bCoast && (iCoastFoodCount >= 2)) // advc.108: was >2
						&& !(bOcean && (iOceanFoodCount >= 2)) // advc.108: was >2
						// advc.108: At most 3 sea food
						&& !((bOcean || bCoast) && iOceanFoodCount + iCoastFoodCount >= 3))
					{
						for (int iPass = 0; iPass < 2; iPass++)
						{
							if (pLoopPlot->getBonusType() == NO_BONUS)
							{
								for (int iK = 0; iK < GC.getNumBonusInfos(); iK++)
								{
									if (GC.getBonusInfo((BonusTypes)iK).isNormalize())
									{
										//???no bonuses with negative yields?
										if ((GC.getBonusInfo((BonusTypes)iK).getYieldChange(YIELD_FOOD) >= 0) &&
											(GC.getBonusInfo((BonusTypes)iK).getYieldChange(YIELD_PRODUCTION) >= 0))
										{
											if ((GC.getBonusInfo((BonusTypes)iK).getTechCityTrade() == NO_TECH) || (GC.getTechInfo((TechTypes)(GC.getBonusInfo((BonusTypes)iK).getTechCityTrade())).getEra() <= getStartEra()))
											{
												if (GET_TEAM(kLoopPlayer.getTeam()).isHasTech((TechTypes)(GC.getBonusInfo((BonusTypes)iK).getTechReveal())))
												{
													if ((iPass == 0) ? CvMapGenerator::GetInstance().canPlaceBonusAt(((BonusTypes)iK), pLoopPlot->getX(), pLoopPlot->getY(), bIgnoreLatitude) : pLoopPlot->canHaveBonus(((BonusTypes)iK), bIgnoreLatitude))
													{
														/*  advc.108 (comment): Don't need to check whether the bonus is
															revealed b/c all initially unrevealed bonuses have
															bNormalize=0 in CvBonusInfos.xml. */
														if (gMapLogLevel > 0)
															logBBAI("    Adding %S for player %d.", GC.getBonusInfo((BonusTypes)iK).getDescription(), iI); // K-Mod
														pLoopPlot->setBonusType((BonusTypes)iK);
														iCoastFoodCount += bCoast ? 1 : 0;
														iOceanFoodCount += bOcean ? 1 : 0;
														iOtherCount += !(bCoast || bOcean) ? 1 : 0;
														break;
													}
												}
											}
										}
									}
								}

								if (bLandBias && !pLoopPlot->isWater() && pLoopPlot->getBonusType() == NO_BONUS)
								{
									if (((iFeatureCount > 4) && (pLoopPlot->getFeatureType() != NO_FEATURE))
										&& ((iCoastFoodCount + iOceanFoodCount) > 2))
									{
										if (getSorenRandNum(2, "Clear feature to add bonus") == 0)
										{
											if (gMapLogLevel > 0)
												logBBAI("    Removing %S to place bonus for player %d.", GC.getFeatureInfo(pLoopPlot->getFeatureType()).getDescription(), iI); // K-Mod
											pLoopPlot->setFeatureType(NO_FEATURE);

											for (int iK = 0; iK < GC.getNumBonusInfos(); iK++)
											{
												if (GC.getBonusInfo((BonusTypes)iK).isNormalize())
												{
													//???no bonuses with negative yields?
													if ((GC.getBonusInfo((BonusTypes)iK).getYieldChange(YIELD_FOOD) >= 0) &&
														(GC.getBonusInfo((BonusTypes)iK).getYieldChange(YIELD_PRODUCTION) >= 0))
													{
														if ((GC.getBonusInfo((BonusTypes)iK).getTechCityTrade() == NO_TECH) || (GC.getTechInfo((TechTypes)(GC.getBonusInfo((BonusTypes)iK).getTechCityTrade())).getEra() <= getStartEra()))
														{
															if ((iPass == 0) ? CvMapGenerator::GetInstance().canPlaceBonusAt(((BonusTypes)iK), pLoopPlot->getX(), pLoopPlot->getY(), bIgnoreLatitude) : pLoopPlot->canHaveBonus(((BonusTypes)iK), bIgnoreLatitude))
															{
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
									}
								}
							}
						}
					}
				}
			}
		}

		shuffleArray(aiShuffle, NUM_CITY_PLOTS, getMapRand());

		for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
		{
			if (kLoopPlayer.AI_foundValue(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), -1, true) >= iTargetValue)
			{
				if (gMapLogLevel > 0)
					logBBAI("    Player %d doesn't need any more features (2).", iI); // K-Mod
				break;
			}

			CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), aiShuffle[iJ]);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot != pStartingPlot)
				{
					if (pLoopPlot->getBonusType() == NO_BONUS)
					{
						if (pLoopPlot->getFeatureType() == NO_FEATURE)
						{
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
					}
				}
			}
		}

		int iHillsCount = 0;

		for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
		{
			CvPlot* pLoopPlot =plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), iJ);
			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->isHills())
				{
					iHillsCount++;
				}
			}
		}
		shuffleArray(aiShuffle, NUM_CITY_PLOTS, getMapRand());
		for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
		{
			if (iHillsCount >= 3)
			{
				break;
			}
			CvPlot* pLoopPlot = plotCity(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), aiShuffle[iJ]);
			if (pLoopPlot != NULL)
			{
				if (!pLoopPlot->isWater())
				{
					if (!pLoopPlot->isHills())
					{
						if ((pLoopPlot->getFeatureType() == NO_FEATURE) ||
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
				}
			}
		}
		if (gMapLogLevel > 0)
			logBBAI("    Player %d final value: %d", iI, kLoopPlayer.AI_foundValue(pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE(), -1, true)); // K-Mod
	}
	if (gMapLogLevel > 0)
		logBBAI("normalizeAddExtras() complete"); // K-Mod
}


void CvGame::normalizeStartingPlots()
{
	PROFILE_FUNC();

	if (!(GC.getInitCore().getWBMapScript()) || GC.getInitCore().getWBMapNoPlayers())
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
			game turn. I've tried doing that through BugEventManager.py, but
			eventually gave up. Tagging advc.706 b/c it's especially important
			to supress the update when R&F is enabled. */
		if(!isAITurn())
			CvEventReporter::getInstance().genericEvent("gameUpdate", pyArgs.makeFunctionArgs());

		if (getTurnSlice() == 0)
		{	// <advc.700> Delay initial auto-save until RiseFall is initialized
			bool isStartTurn = (getGameTurn() == getStartTurn()); // advc.004m
			// I guess TurnSlice==0 already implies that it's the start turn (?)
			if((!isStartTurn || !isOption(GAMEOPTION_RISE_FALL)) // </advc.700>
					&& m_iTurnLoadedFromSave != m_iElapsedGameTurns) // advc.044
				gDLL->getEngineIFace()->AutoSave(true);
			/* <advc.004m> This seems to be the earliest place where bubbles can
			   be enabled w/o crashing. */
			if(isStartTurn && GC.getDefineINT("SHOW_RESOURCE_BUBBLES_AT_GAME_START") > 0)
				gDLL->getEngineIFace()->setResourceLayer(true); // </advc.004m>
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

		if ((getAIAutoPlay() == 0) && !(gDLL->GetAutorun()) && GAMESTATE_EXTENDED != getGameState())
		{
			if (countHumanPlayersAlive() == 0
					&& !isOption(GAMEOPTION_RISE_FALL)) // advc.707
			{
				setGameState(GAMESTATE_OVER);
			}
		}

		changeTurnSlice(1);

		if (NO_PLAYER != getActivePlayer() && GET_PLAYER(getActivePlayer()).getAdvancedStartPoints() >= 0 && !gDLL->getInterfaceIFace()->isInAdvancedStart())
		{
			gDLL->getInterfaceIFace()->setInAdvancedStart(true);
			gDLL->getInterfaceIFace()->setWorldBuilder(true);
		} // <advc.705>
		if(isOption(GAMEOPTION_RISE_FALL))
			riseFall.restoreDiploText(); // </advc.705>
		// <advc.003d>
		if(!m_bResourceLayerSet) {
			gDLL->getEngineIFace()->setResourceLayer(isResourceLayer());
			/*  This flag is only for performance - who knows how long the
				line above takes (even if bOn already equals bResourceLayer). */
			m_bResourceLayerSet = true;
		} // </advc.003d>
	}
	PROFILE_END();
	stopProfilingDLL(false);
}


void CvGame::updateScore(bool bForce)
{
	bool abPlayerScored[MAX_CIV_PLAYERS];
	bool abTeamScored[MAX_CIV_TEAMS];
	int iScore;
	int iBestScore;
	PlayerTypes eBestPlayer;
	TeamTypes eBestTeam;
	int iI, iJ, iK;

	if (!isScoreDirty() && !bForce)
	{
		return;
	}

	setScoreDirty(false);

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		abPlayerScored[iI] = false;
	}
	std::vector<PlayerTypes> updateAttitude; // advc.001
	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		iBestScore = MIN_INT;
		eBestPlayer = NO_PLAYER;

		for (iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
		{
			if (!abPlayerScored[iJ])
			{
				iScore = GET_PLAYER((PlayerTypes)iJ).calculateScore(false);

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
			updateAttitude.push_back(eBestPlayer); // </advc.001>
		setRankPlayer(iI, eBestPlayer);
		setPlayerRank(eBestPlayer, iI);
		setPlayerScore(eBestPlayer, iBestScore);
		GET_PLAYER(eBestPlayer).updateScoreHistory(getGameTurn(), iBestScore);
	} // <advc.001>
	/*for(size_t i = 0; i < updateAttitude.size(); i++)
		GET_PLAYER(updateAttitude[i]).AI_updateAttitudeCache();*/
	/*  The above isn't enough; the attitudes of those outside updateAttitude
		toward those inside could also change. */
	if(!updateAttitude.empty()) {
		for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
			CvPlayerAI& civ = GET_PLAYER((PlayerTypes)i);
			if(civ.isAlive() && !civ.isMinorCiv())
				civ.AI_updateAttitudeCache();
		}
	} // </advc.001>
	for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		abTeamScored[iI] = false;
	}

	for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		iBestScore = MIN_INT;
		eBestTeam = NO_TEAM;

		for (iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
		{
			if (!abTeamScored[iJ])
			{
				iScore = 0;

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

	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).updatePlotGroups();
		}
	}
}


void CvGame::updateBuildingCommerce()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).updateBuildingCommerce();
		}
	}
}


void CvGame::updateCitySight(bool bIncrement)
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
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
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			GET_PLAYER((PlayerTypes)iI).updateTradeRoutes();
		}
	}
}

/*
** K-Mod
** calculate unhappiness due to the state of global warming
*/
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
	}

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
}
/*
** K-Mod end
*/

void CvGame::testExtendedGame()
{
	int iI;

	if (getGameState() != GAMESTATE_OVER)
	{
		return;
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
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

	bool bSelectGroup;
	bool bGroup;

	/* original bts code
	if (gDLL->getInterfaceIFace()->getHeadSelectedUnit() == NULL)
	{
		bSelectGroup = true;
	}
	else if (gDLL->getInterfaceIFace()->getHeadSelectedUnit()->getGroup() != pUnit->getGroup())
	{
		bSelectGroup = true;
	}
	else if (pUnit->IsSelected() && !(gDLL->getInterfaceIFace()->mirrorsSelectionGroup()))
	{
		bSelectGroup = !bToggle;
	}
	else
	{
		bSelectGroup = false;
	} */
	// K-Mod. Redesigned to make selection more sensible and predictable
	// In 'simple mode', shift always groups and always targets a only a single unit.
	bool bSimpleMode = getBugOptionBOOL("MainInterface__SimpleSelectionMode", false, "SIMPLE_SELECTION_MODE");

	bool bExplicitDeselect = false;

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
	}
	// K-Mod end

	gDLL->getInterfaceIFace()->clearSelectedCities();

	if (bClear)
	{
		gDLL->getInterfaceIFace()->clearSelectionList();
		bGroup = false;
	}
	else
	{
		//bGroup = gDLL->getInterfaceIFace()->mirrorsSelectionGroup();

		// K-Mod. If there is only one unit selected, and it is it to be toggled, just degroup it rather than unselecting it.
		if (bExplicitDeselect && gDLL->getInterfaceIFace()->getLengthSelectionList() == 1)
		{
			CvMessageControl::getInstance().sendJoinGroup(pUnit->getID(), FFreeList::INVALID_INDEX);
			return; // that's all.
		}

		bGroup = gDLL->getInterfaceIFace()->mirrorsSelectionGroup();
		// Note: bGroup will not clear away unselected units of the group.
		// so if we want to do that, we'll have to do it explicitly.
		if (!bGroup && bSimpleMode && bToggle)
		{
			// 'toggle' should be seen as explicitly adding / removing units from a group.
			// so lets explicitly reform the group.
			selectionListGameNetMessage(GAMEMESSAGE_JOIN_GROUP);
			// note: setting bGroup = true doesn't work here either,
			// because the internals of insertIntoSelectionList apparently wants to go out of its way to make our lives difficult.
			// (stuffed if I know what it actually does. Maybe it only sends the group signal if the units aren't already grouped or something.
			//  in any case, we have to do it explicitly or it won't work.)
			CvUnit* pSelectionHead = gDLL->getInterfaceIFace()->getHeadSelectedUnit();
			if (pSelectionHead)
				CvMessageControl::getInstance().sendJoinGroup(pUnit->getID(), pSelectionHead->getID());
		}
		// K-Mod end
	}

	if (bSelectGroup)
	{
		CvSelectionGroup* pSelectionGroup = pUnit->getGroup();

		gDLL->getInterfaceIFace()->selectionListPreChange();

		CLLNode<IDInfo>* pEntityNode = pSelectionGroup->headUnitNode();
		while (pEntityNode != NULL)
		{
			FAssertMsg(::getUnit(pEntityNode->m_data), "null entity in selection group");
			gDLL->getInterfaceIFace()->insertIntoSelectionList(::getUnit(pEntityNode->m_data), false, bToggle, bGroup, bSound, true);

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
	if(GC.getDefineINT("SHOW_PROMOTION_GLOW") <= 0) {
		CvPlayer const& owner = GET_PLAYER(pUnit->getOwnerINLINE());
		int dummy;
		for(CvUnit* u = owner.firstUnit(&dummy); u != NULL; u = owner.nextUnit(&dummy)) {
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

			if (pLoopUnit->getOwnerINLINE() == getActivePlayer() && pLoopUnit->isGroupHead() && pLoopUnit->isWaiting())
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

			if (pLoopUnit->getOwnerINLINE() == getActivePlayer())
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

		if ((pCenterUnit != NULL) && (pCenterUnit->getOwnerINLINE() == getActivePlayer()))
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

	// <advc.003> Minor refactoring
	FAssert(eWho != NO_PLAYER);
	FAssert(eOtherWho != NO_PLAYER);
	FAssert(eWho != eOtherWho); // </advc.003>
	// <advc.032>
	if(TEAMREF(eWho).isForcePeace(TEAMID(eOtherWho))) {
		for(CLLNode<TradeData>* item = pOurList->head(); item != NULL;
				item = pOurList->next(item)) {
			if(item->m_data.m_eItemType == TRADE_PEACE_TREATY) {
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

	iPercent -= (countCivTeamsEverAlive() * 2);

	return std::max(iPercent, GC.getVictoryInfo(eVictory).getMinLandPercent());
}

// <advc.178> Mostly cut and pasted from CvPlayerAI::AI_calculateDiplomacyVictoryStage
bool CvGame::isDiploVictoryValid() const {

	std::vector<VictoryTypes> veDiplomacy;
	for (int iI = 0; iI < GC.getNumVictoryInfos(); iI++)
	{
		if (isVictoryValid((VictoryTypes) iI))
		{
			CvVictoryInfo& kVictoryInfo = GC.getVictoryInfo((VictoryTypes) iI);
			if( kVictoryInfo.isDiploVote() )
			{
				veDiplomacy.push_back((VictoryTypes)iI);
			}
		}
	}
	return (veDiplomacy.size() > 0);
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
							if(kTeam.isCapitulated()) iVotes = 0;
							if(kLoopTeam.isCapitulated()) iLoopVotes = 0;
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
		{
			clearSecretaryGeneral((VoteSourceTypes)i);
		}
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
{
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
{
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
					game is over when human is still disabled at the start of a
					round. */
				kPlayer.isHumanDisabled()))
		{
			iCount++;
		}
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
		{
			iCount++;
		}
	}

	return iCount;
}
// K-Mod end


// <advc.137>
int CvGame::getRecommendedPlayers() const {

	CvWorldInfo const& wi = GC.getWorldInfo(GC.getMapINLINE().getWorldSize());
	return ::range(((-5 * getSeaLevelChange() + 100) * wi.getDefaultPlayers()) / 100,
			2, MAX_CIV_PLAYERS);
}

// <advc.140>
int CvGame::getSeaLevelChange() const {

	int r = 0;
	SeaLevelTypes sea = GC.getInitCore().getSeaLevel();
	if(sea != NO_SEALEVEL)
		r = GC.getSeaLevelInfo(sea).getSeaLevelChange();
	return r;
}
// </advc.140></advc.137>


int CvGame::countTotalCivPower()
{
	int iCount = 0;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iI);
		if (kPlayer.isAlive())
		{
			iCount += kPlayer.getPower();
		}
	}

	return iCount;
}


int CvGame::countTotalNukeUnits()
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iI);
		if (kPlayer.isAlive())
		{
			iCount += kPlayer.getNumNukeUnits();
		}
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
			{
				iCount++;
			}
		}
	}

	return iCount;
}


int CvGame::getNumFreeBonuses(BuildingTypes eBuilding)
{
	if (GC.getBuildingInfo(eBuilding).getNumFreeBonuses() == -1)
	{
		return GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getNumFreeBuildingBonuses();
	}
	else
	{
		return GC.getBuildingInfo(eBuilding).getNumFreeBonuses();
	}
}


int CvGame::countReligionLevels(ReligionTypes eReligion)
{
	int iCount;
	int iI;

	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			iCount += GET_PLAYER((PlayerTypes)iI).getHasReligionCount(eReligion);
		}
	}

	return iCount;
}

int CvGame::countCorporationLevels(CorporationTypes eCorporation)
{
	int iCount = 0;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.isAlive())
		{
			iCount += GET_PLAYER((PlayerTypes)iI).getHasCorporationCount(eCorporation);
		}
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
			int iIter;
			for (CvCity* pCity = kLoopPlayer.firstCity(&iIter); NULL != pCity; pCity = kLoopPlayer.nextCity(&iIter))
			{
				if (pCity->isHasCorporation(eCorporation1))
				{
					pCity->setHasCorporation(eCorporation1, false, false, false);
					pCity->setHasCorporation(eCorporation2, true, true);
				}
			}

			for (CvUnit* pUnit = kLoopPlayer.firstUnit(&iIter); NULL != pUnit; pUnit = kLoopPlayer.nextUnit(&iIter))
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
		bool ignoreOtherReligions) const // advc.115b: Param added
{
	CvCity* pLoopCity;
	int iCount;
	int iLoop;
	int iI;

	if (getTotalPopulation() == 0)
	{
		return 0;
	}

	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
			{
				if (pLoopCity->isHasReligion(eReligion))
				{	// <advc.115b>
					if(ignoreOtherReligions)
						iCount += pLoopCity->getPopulation();
					else // </advc.115b> // <advc.003> Removed some parentheses
					iCount += (pLoopCity->getPopulation() +
							pLoopCity->getReligionCount() / 2) /
							pLoopCity->getReligionCount(); // </advc.003>
				}
			}
		}
	}

	return ((iCount * 100) / getTotalPopulation());
}


int CvGame::goldenAgeLength() const
{
	int iLength;

	iLength = GC.getDefineINT("GOLDEN_AGE_LENGTH");

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


bool CvGame::isModem() const // advc.003: const
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
	if (!(GET_PLAYER(getActivePlayer()).isAlive()))
	{
		setAIAutoPlayBulk(0, false); // advc.127: false flag added

		GC.getInitCore().setSlotStatus(getActivePlayer(), SS_TAKEN);
		
		// Let Python handle it
		long lResult=0;
		CyArgsList argsList;
		argsList.add(getActivePlayer());

		gDLL->getPythonIFace()->callFunction(PYGameModule, "doReviveActivePlayer", argsList.makeFunctionArgs(), &lResult);
		if (lResult == 1)
		{
			return;
		}

		GET_PLAYER(getActivePlayer()).initUnit(((UnitTypes)0), 0, 0);
	}
}


int CvGame::getNumHumanPlayers()
{
	return GC.getInitCore().getNumHumans();
}

int CvGame::getGameTurn()
{
	return GC.getInitCore().getGameTurn();
} // <advc.003> const replacement
int CvGame::gameTurn() const {
	return GC.getInitCore().getGameTurn();
} // </advc.003>


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


int CvGame::getTurnYear(int iGameTurn) const // advc.003: const
{
	// moved the body of this method to Game Core Utils so we have access for other games than the current one (replay screen in HOF)
	return getTurnYearForGame(iGameTurn, getStartYear(), getCalendar(), getGameSpeedType());
}


int CvGame::getGameTurnYear() const // advc.003: const
{
	//return getTurnYear(getGameTurn()); // To work aorund non-const getGameTurn
	return getTurnYear(gameTurn());
}


int CvGame::getElapsedGameTurns() const
{
	return m_iElapsedGameTurns;
}


void CvGame::incrementElapsedGameTurns()
{
	m_iElapsedGameTurns++;
}


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

/*  advc.003: Ratio of turns played to total estimated game length; between 0 and 1.
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


int CvGame::getTurnSlicesRemaining()
{
	return (getCutoffSlice() - getTurnSlice());
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
		return ( getPitbossTurnTime() * 3600 * 4);
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
		return ( GC.getTurnTimerInfo(eTurnTimer).getBaseTime() + 
			    (GC.getTurnTimerInfo(eTurnTimer).getCityBonus()*iMaxCities) +
				(GC.getTurnTimerInfo(eTurnTimer).getUnitBonus()*iMaxUnits) );
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
	for (int i = 0; i < GC.getMapINLINE().numPlotsINLINE(); i++)
	{
		CvPlot* pPlot = GC.getMapINLINE().plotByIndexINLINE(i);
		if (!pPlot->isWater() || pPlot->isAdjacentToLand())
		{
			iMaxFood += pPlot->calculateBestNatureYield(YIELD_FOOD, NO_TEAM);
		}
	}
	m_iMaxPopulation = getPopulationScore(iMaxFood / std::max(1, GC.getFOOD_CONSUMPTION_PER_POPULATION()));
	m_iMaxLand = getLandPlotsScore(GC.getMapINLINE().getLandPlots());
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


int CvGame::getAIAutoPlay() const // advc.003: made const
{
	return m_iAIAutoPlay;
}

void CvGame::setAIAutoPlay(int iNewValue) {
	// <advc.127>
	setAIAutoPlayBulk(iNewValue);
}
void CvGame::setAIAutoPlayBulk(int iNewValue, bool changePlayerStatus)
{	// advc.127
	m_iAIAutoPlay = std::max(0, iNewValue);
	// <advc.127>
	if(!changePlayerStatus)
		return; // </advc.127>
/************************************************************************************************/
/* AI_AUTO_PLAY_MOD                           07/09/08                            jdog5000      */
/*                                                                                              */
/*                                                                                              */
/************************************************************************************************/
// Multiplayer compatibility idea from Jeckel
	// <advc.127> To make sure I'm not breaking anything in singleplayer
	if(!isGameMultiPlayer()) {
		GET_PLAYER(getActivePlayer()).setHumanDisabled((getAIAutoPlay() != 0));
		return;
	} // </advc.127>
	for( int iI = 0; iI < MAX_CIV_PLAYERS; iI++ )
	{
		if( GET_PLAYER((PlayerTypes)iI).isHuman() || GET_PLAYER((PlayerTypes)iI).isHumanDisabled() )
		{	/*  advc.127: Was GET_PLAYER(getActivePlayer()).
				Tagging advc.001 because that was probably a bug. */
			GET_PLAYER((PlayerTypes)iI).setHumanDisabled((getAIAutoPlay() != 0));
		}
	}
/************************************************************************************************/
/* AI_AUTO_PLAY_MOD                            END                                              */
/************************************************************************************************/
}


void CvGame::changeAIAutoPlay(int iChange
		, bool changePlayerStatus) // advc.127
{
	setAIAutoPlayBulk(getAIAutoPlay() + iChange,
			changePlayerStatus); // advc.127
}

/*
** K-mod, 6/dec/10, karadoc
** 18/dec/10 - added Gw calc functions
*/
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

	for (int i = 0; i < GC.getMapINLINE().numPlotsINLINE(); ++i)
	{
		CvPlot* pPlot = GC.getMapINLINE().plotByIndexINLINE(i);

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
	int iGlobalThreshold = GC.getMapINLINE().getLandPlots() * GC.getDefineINT("GLOBAL_WARMING_RESISTANCE");
	
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
		return iGlobalThreshold * kPlayer.getTotalLand() / std::max(1, GC.getMapINLINE().getLandPlots());
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

	const long x = GC.getDefineINT("GLOBAL_WARMING_PROB") * getGlobalWarmingIndex() / (std::max(1,4*GC.getGameSpeedInfo(getGameSpeedType()).getVictoryDelayPercent()*GC.getMapINLINE().getLandPlots()));
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

	CvMap& kMap = GC.getMapINLINE();

	if (!(kMap.isWrapXINLINE()) && !(kMap.isWrapYINLINE()))
	{
		return false;
	}

	if (kMap.getLandPlots() > ((kMap.numPlotsINLINE() * 2) / 3))
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

	if (0 != iChange)
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
	CvVoteInfo const& vi = GC.getVoteInfo(kData.eVote); // advc.003
	if (vi.isVictory())
	{
		int iVotesRequired = getVoteRequired(kData.eVote, eVoteSource); // K-Mod
		for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
		{
			CvTeam& kTeam = GET_TEAM((TeamTypes)iTeam);

			if (kTeam.getVotes(kData.eVote, eVoteSource) >= iVotesRequired)
				return false; // K-Mod. same, but faster.
			/* original bts code
			if (kTeam.isVotingMember(eVoteSource))
			{
				if (kTeam.getVotes(kData.eVote, eVoteSource) >= getVoteRequired(kData.eVote, eVoteSource))
				{
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
			if(vi.isForceWar()) {
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
			if (vi.isVictory())
			{
				return false;
			}
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
	m_bDebugMode = ((m_bDebugMode) ? false : true);
	updateDebugModeCache();

	GC.getMapINLINE().updateVisibility();
	GC.getMapINLINE().updateSymbols();
	GC.getMapINLINE().updateMinimapColor();
	GC.getMapINLINE().setFlagsDirty(); // K-Mod
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
	{
		m_bDebugModeCache = m_bDebugMode;
	}
	else
	{
		m_bDebugModeCache = false;
	}
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
		CvWString const& gameName = GC.getInitCore().getGameName();
		return (gameName.compare(L"chipotle") == 0);
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

	if (isFinalInitialized() != bNewValue)
	{
		m_bFinalInitialized = bNewValue;

		if (isFinalInitialized())
		{
			updatePlotGroups();

			GC.getMapINLINE().updateIrrigated();

			for (int iI = 0; iI < MAX_TEAMS; iI++)
			{
				GET_TEAM((TeamTypes)iI).AI_initMemory(); // K-Mod
				if (GET_TEAM((TeamTypes)iI).isAlive())
				{
					GET_TEAM((TeamTypes)iI).AI_updateAreaStrategies();
				}
			}
		}
	}
}

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

// <advc.706> Cut and pasted from CvGame::setActivePlayer
void CvGame::updateActiveVisibility() {

		if(!GC.IsGraphicsInitialized())
			return;
		GC.getMapINLINE().updateFog();
		GC.getMapINLINE().updateVisibility();
		GC.getMapINLINE().updateSymbols();
		GC.getMapINLINE().updateMinimapColor();

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

		gDLL->getEngineIFace()->SetDirty(CultureBorders_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(BlockadedPlots_DIRTY_BIT, true);
}
// </advc.706>

void CvGame::updateUnitEnemyGlow()
{
	//update unit enemy glow
	for(int i=0;i<MAX_PLAYERS;i++)
	{
		PlayerTypes playerType = (PlayerTypes) i;
		int iLoop;
		for(CvUnit *pLoopUnit = GET_PLAYER(playerType).firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = GET_PLAYER(playerType).nextUnit(&iLoop))
		{
			//update glow
			gDLL->getEntityIFace()->updateEnemyGlow(pLoopUnit->getUnitEntity());
		}
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
	spah.distribution(aiStartPointDistrib);
	std::vector<double> distr;
	for(size_t i = 0; i < aiStartPointDistrib.size(); i++)
		distr.push_back(aiStartPointDistrib[i]);
	return r + ::round((::max(distr) + ::mean(distr)) /
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
/************************************************************************************************/
/* AI_AUTO_PLAY_MOD                        07/09/08                                jdog5000      */
/*                                                                                              */
/*                                                                                              */
/************************************************************************************************/
			CvEventReporter::getInstance().victory(eNewWinner, eNewVictory);
/************************************************************************************************/
/* AI_AUTO_PLAY_MOD                        END                                                  */
/************************************************************************************************/

		if (getVictory() != NO_VICTORY)
		{
			if (getWinner() != NO_TEAM)
			{
				szBuffer = gDLL->getText("TXT_KEY_GAME_WON", GET_TEAM(getWinner()).getReplayName().GetCString(), GC.getVictoryInfo(getVictory()).getTextKeyWide());
				addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, GET_TEAM(getWinner()).getLeaderID(), szBuffer, -1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
			}

			if ((getAIAutoPlay() > 0) || gDLL->GetAutorun())
			{
				setGameState(GAMESTATE_EXTENDED);
			}
			else
			{
				setGameState(GAMESTATE_OVER);
			}
		}

		gDLL->getInterfaceIFace()->setDirty(Center_DIRTY_BIT, true);

/************************************************************************************************/
/* AI_AUTO_PLAY_MOD                        07/09/08                                jdog5000      */
/*                                                                                              */
/*                                                                                              */
/************************************************************************************************/
/* original code
		CvEventReporter::getInstance().victory(eNewWinner, eNewVictory);
*/
/************************************************************************************************/
/* AI_AUTO_PLAY_MOD                        END                                                  */
/************************************************************************************************/

		gDLL->getInterfaceIFace()->setDirty(Soundtrack_DIRTY_BIT, true);
	}
}


GameStateTypes CvGame::getGameState() const
{
	return m_eGameState;
}


void CvGame::setGameState(GameStateTypes eNewValue)
{
	if (getGameState() != eNewValue)
	{
		m_eGameState = eNewValue;

		if (eNewValue == GAMESTATE_OVER)
		{
			CvEventReporter::getInstance().gameEnd();
			// <advc.707>
			if(isOption(GAMEOPTION_RISE_FALL))
				riseFall.prepareForExtendedGame(); // </advc.707>
			showEndGameSequence();
			for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
			{
				if (GET_PLAYER((PlayerTypes)iI).isHuman())
				{
					// One more turn?
					CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_EXTENDED_GAME);
					if (NULL != pInfo)
					{
						GET_PLAYER((PlayerTypes)iI).addPopup(pInfo);
					}
				}
			}
		}

		gDLL->getInterfaceIFace()->setDirty(Cursor_DIRTY_BIT, true);
	}
}


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
{	// advc.003 (comment): The topmost rank is 0
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
{
	return GC.getInitCore().getOption(eIndex);
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


bool CvGame::isSpecialUnitValid(SpecialUnitTypes eIndex)
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


bool CvGame::isSpecialBuildingValid(SpecialBuildingTypes eIndex)
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
				addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, pHolyCity->getOwnerINLINE(), szBuffer, pHolyCity->getX_INLINE(), pHolyCity->getY_INLINE());
						// advc.106: Reserve this color for treaties
						//(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));

				for (int iI = 0; iI < MAX_PLAYERS; iI++)
				{	// advc.003:
					CvPlayer const& civ = GET_PLAYER((PlayerTypes)iI);
					if (civ.isAlive())
					{
						if (pHolyCity->isRevealed(civ.getTeam(), false)
								|| civ.isSpectator()) // advc.127
						{
							szBuffer = gDLL->getText("TXT_KEY_MISC_REL_FOUNDED", GC.getReligionInfo(eIndex).getTextKeyWide(), pHolyCity->getNameKey());
							gDLL->getInterfaceIFace()->addHumanMessage(civ.getID(), false, GC.getDefineINT("EVENT_MESSAGE_TIME_LONG"), szBuffer, GC.getReligionInfo(eIndex).getSound(), MESSAGE_TYPE_MAJOR_EVENT, GC.getReligionInfo(eIndex).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"), pHolyCity->getX_INLINE(), pHolyCity->getY_INLINE(), false, true);
						}
						else
						{
							szBuffer = gDLL->getText("TXT_KEY_MISC_REL_FOUNDED_UNKNOWN", GC.getReligionInfo(eIndex).getTextKeyWide());
							gDLL->getInterfaceIFace()->addHumanMessage(civ.getID(), false, GC.getDefineINT("EVENT_MESSAGE_TIME_LONG"), szBuffer, GC.getReligionInfo(eIndex).getSound(), MESSAGE_TYPE_MAJOR_EVENT, GC.getReligionInfo(eIndex).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
						}
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
					addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, pHeadquarters->getOwnerINLINE(), szBuffer, pHeadquarters->getX_INLINE(), pHeadquarters->getY_INLINE(), (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));

					for (int iI = 0; iI < MAX_PLAYERS; iI++)
					{
						if (GET_PLAYER((PlayerTypes)iI).isAlive())
						{
							if (pHeadquarters->isRevealed(GET_PLAYER((PlayerTypes)iI).getTeam(), false))
							{
								gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), false, GC.getDefineINT("EVENT_MESSAGE_TIME_LONG"), szBuffer, GC.getCorporationInfo(eIndex).getSound(), MESSAGE_TYPE_MAJOR_EVENT, GC.getCorporationInfo(eIndex).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"), pHeadquarters->getX_INLINE(), pHeadquarters->getY_INLINE(), false, true);
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
		CvEventReporter::getInstance().beginGameTurn( getGameTurn() );

	doUpdateCacheOnTurn();

	updateScore();

	doDeals();

	/* original bts code
	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			GET_TEAM((TeamTypes)iI).doTurn();
		}
	} */ // disabled by K-Mod. CvTeam::doTurn is now called at the the same time as CvPlayer::doTurn, to fix certain turn-order imbalances.

	GC.getMapINLINE().doTurn();

	createBarbarianCities();

	createBarbarianUnits();

	doGlobalWarming();

	doHolyCity();

	doHeadquarters();

	gDLL->getInterfaceIFace()->setEndTurnMessage(false);
	gDLL->getInterfaceIFace()->setHasMovedUnit(false);

	if (getAIAutoPlay() > 0)
	{	/*  <advc.127> Flag added: don't change player status when decrementing
			the counter at the start of a round. Let AIAutoPlay.py::onEndPlayerTurn
			handle it. (Because human control should resume right before the human
			turn, which is not necessarily at the beginning of a round.) */
		changeAIAutoPlay(-1, false);
		if(isNetworkMultiPlayer()) { // Stop when OOS
			int syncHash = gDLL->GetSyncOOS(GET_PLAYER(getActivePlayer()).getNetID());
			for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
				CvPlayer const& other = GET_PLAYER((PlayerTypes)i);
				if(!other.isAlive() || other.getID() == getActivePlayer() ||
						!other.isHumanDisabled())
					continue;
				if(gDLL->GetSyncOOS(other.getNetID()) != syncHash)
					setAIAutoPlay(0);
			}
		} // </advc.127>
		if (getAIAutoPlay() == 0)
		{
			reviveActivePlayer();
		}
	}

	CvEventReporter::getInstance().endGameTurn(getGameTurn());

	incrementGameTurn();
	incrementElapsedGameTurns();
	/*  advc.004: Already done in doDeals, but that's before incrementing the
		turn counter. Want to kill peace treaties asap. */
	verifyDeals();
	// <advc.700>
	if(isOption(GAMEOPTION_RISE_FALL))
		riseFall.atGameTurnStart(); // </advc.700>
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
		riseFall.autoSave();
	else {// </advc.700>
		/*  <advc.127> Avoid overlapping auto-saves in test games played on a
			single machine. Don't know how to check this properly. */
		if(!isNetworkMultiPlayer() || getAIAutoPlay() <= 0 || getActivePlayer() == NO_PLAYER ||
				getActivePlayer() % 2 == 0) // </advc.127>
			gDLL->getEngineIFace()->AutoSave();
	}
}

// <advc.106b>
bool CvGame::isAITurn() const {

	return m_bAITurn;
}

void CvGame::setAITurn(bool b) {

	m_bAITurn = b;
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

/*
** original bts code
**
void CvGame::doGlobalWarming()
{
	int iGlobalWarmingDefense = 0;
	for (int i = 0; i < GC.getMapINLINE().numPlotsINLINE(); ++i)
	{
		CvPlot* pPlot = GC.getMapINLINE().plotByIndexINLINE(i);

		if (!pPlot->isWater())
		{
			if (pPlot->getFeatureType() != NO_FEATURE)
			{
				if (GC.getFeatureInfo(pPlot->getFeatureType()).getGrowthProbability() > 0) // hack, but we don't want to add new XML field in the patch just for this
				{
					++iGlobalWarmingDefense;
				}
			}
		}
	}
	iGlobalWarmingDefense = iGlobalWarmingDefense * GC.getDefineINT("GLOBAL_WARMING_FOREST") / std::max(1, GC.getMapINLINE().getLandPlots());

	int iUnhealthWeight = GC.getDefineINT("GLOBAL_WARMING_UNHEALTH_WEIGHT");
	int iGlobalWarmingValue = 0;
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iPlayer);
		if (kPlayer.isAlive())
		{
			int iLoop;
			for (CvCity* pCity = kPlayer.firstCity(&iLoop); pCity != NULL; pCity = kPlayer.nextCity(&iLoop))
			{
				iGlobalWarmingValue -= pCity->getBuildingBadHealth() * iUnhealthWeight;
			}
		}
	}
	iGlobalWarmingValue /= GC.getMapINLINE().numPlotsINLINE();

	iGlobalWarmingValue += getNukesExploded() * GC.getDefineINT("GLOBAL_WARMING_NUKE_WEIGHT") / 100;

	TerrainTypes eWarmingTerrain = ((TerrainTypes)(GC.getDefineINT("GLOBAL_WARMING_TERRAIN")));

	for (int iI = 0; iI < iGlobalWarmingValue; iI++)
	{
		if (getSorenRandNum(100, "Global Warming") + iGlobalWarmingDefense < GC.getDefineINT("GLOBAL_WARMING_PROB"))
		{
			CvPlot* pPlot = GC.getMapINLINE().syncRandPlot(RANDPLOT_LAND | RANDPLOT_NOT_CITY);

			if (pPlot != NULL)
			{
				bool bChanged = false;

				if (pPlot->getFeatureType() != NO_FEATURE)
				{
					if (pPlot->getFeatureType() != GC.getDefineINT("NUKE_FEATURE"))
					{
						pPlot->setFeatureType(NO_FEATURE);
						bChanged = true;
					}
				}
				else if (pPlot->getTerrainType() != eWarmingTerrain)
				{
					if (pPlot->calculateTotalBestNatureYield(NO_TEAM) > 1)
					{
						pPlot->setTerrainType(eWarmingTerrain);
						bChanged = true;
					}
				}

				if (bChanged)
				{
					pPlot->setImprovementType(NO_IMPROVEMENT);

					CvCity* pCity = GC.getMapINLINE().findCity(pPlot->getX_INLINE(), pPlot->getY_INLINE());
					if (pCity != NULL)
					{
						if (pPlot->isVisible(pCity->getTeam(), false))
						{
							CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_GLOBAL_WARMING_NEAR_CITY", pCity->getNameKey());
							gDLL->getInterfaceIFace()->addHumanMessage(pCity->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_GLOBALWARMING", MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true, true);
						}
					}
				}
			}
		}
	}
}
** end original bts code
**/

/*
** K-Mod, 5/dec/10, karadoc
** complete rewrite of global warming, using some features from 'GWMod' by M.A.
*/
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
			//CvPlot* pPlot = GC.getMapINLINE().syncRandPlot(RANDPLOT_LAND | RANDPLOT_NOT_CITY);

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
				else if (pPlot->getFeatureType() == eTemperateFeature)
				{
					pPlot->setFeatureType(eWarmFeature);
					bChanged = true;
				}
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
				else if (pPlot->getTerrainType() == eBarrenTerrain)
				{
					if (isOption(GAMEOPTION_RISING_SEAS))
					{
						if (pPlot->isCoastalLand())
						{
							if (!pPlot->isHills() && !pPlot->isPeak())
							{
								pPlot->forceBumpUnits();
								pPlot->setPlotType(PLOT_OCEAN);
								bChanged = true;
							}
						}
					}
				}*/

				if (bChanged)
				{
					// only destroy the improvement if the new terrain cannot support it
					if (!pPlot->canHaveImprovement(pPlot->getImprovementType()),
							NO_BUILD, false) // dlph.9
						pPlot->setImprovementType(NO_IMPROVEMENT);

					CvCity* pCity = GC.getMapINLINE().findCity(pPlot->getX_INLINE(), pPlot->getY_INLINE(), NO_PLAYER, NO_TEAM, false);
					if (pCity != NULL)
					{
						if (pPlot->isVisible(pCity->getTeam(), false))
						{
							CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_GLOBAL_WARMING_NEAR_CITY", pCity->getNameKey());
							gDLL->getInterfaceIFace()->addHumanMessage(pCity->getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_SQUISH", MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true, true);
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
		//CvPlot* pTestPlot = GC.getMapINLINE().syncRandPlot(RANDPLOT_LAND | RANDPLOT_NOT_CITY);
		int j;
		for (j = 0; j < 100; j++)
		{
			pTestPlot = GC.getMapINLINE().syncRandPlot(RANDPLOT_NOT_CITY);

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
							iValue -= religionPriority((TeamTypes)iJ,
									(ReligionTypes)iI);

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

								if (!(GET_PLAYER((PlayerTypes)iJ).isHuman()))
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
								iValue -= religionPriority((PlayerTypes)iJ,
										(ReligionTypes)iI);

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
int CvGame::religionPriority(TeamTypes teamId, ReligionTypes relId) const {

	int teamSize = 0;
	int r = 0;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayer const& civ = GET_PLAYER((PlayerTypes)i);
		if(civ.isMinorCiv() || civ.getTeam() != teamId) continue;
		teamSize++;
		r += religionPriority(civ.getID(), relId);
	}
	if(teamSize == 0)
		return 0;
	return r / teamSize;
}

int CvGame::religionPriority(PlayerTypes civId, ReligionTypes relId) const {

	int r = 0;
	CvPlayer const& civ = GET_PLAYER(civId);
	for(int i = 0; i < GC.getNumTraitInfos(); i++) {
		if(civ.hasTrait((TraitTypes)i)) {
			if(GC.getTraitInfo((TraitTypes)i).getMaxAnarchy() == 0) {
				r += 5;
				/*  Spiritual human should be sure to get a religion (so long as
					difficulty isn't above Noble). Not quite sure if my choice of
					numbers in this function and in doHolyCity accomplishes that. */
				if(GET_PLAYER(civId).isHuman())
					r += 6;
				break;
			}
		}
	}
	r += ((100 - GC.getHandicapInfo(civ.getHandicapType()).
			getStartingLocationPercent()) * 31) / 100;
	// With the pick-rel option, relId will change later on anyway
	if(!isOption(GAMEOPTION_PICK_RELIGION)) {
		/*  Not excluding human here means that choosing a leader with an
			early fav religion can make a difference in human getting
			a religion. Unexpected, as fav religions are pretty obscure
			knowledge. On the other hand, it's a pity to assign human
			an arbitrary religion when e.g. Buddhism would fit so well for
			Ashoka.
			Don't use PersonalityType here; fav. religion is always a matter
			of LeaderType (only matters when playing with shuffled
			personalities). */
		if(GC.getLeaderHeadInfo(civ.getLeaderType()).getFavoriteReligion() ==
				(int)relId)
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


void CvGame::createBarbarianCities()
{
	/*CvPlot* pBestPlot; advc.300: Moved to initialization
	long lResult;
	int iTargetCities;
	int iBestValue;*/

	if (getMaxCityElimination() > 0)
	{
		return;
	}

	if (isOption(GAMEOPTION_NO_BARBARIANS))
	{
		return;
	}

	long lResult = 0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "createBarbarianCities", NULL, &lResult);
	if (lResult == 1)
	{
		return;
	}

	if (GC.getEraInfo(getCurrentEra()).isNoBarbCities())
	{
		return;
	}

	if (GC.getHandicapInfo(getHandicapType()).getUnownedTilesPerBarbarianCity() <= 0)
	{
		return;
	}

	if (getNumCivCities() < (countCivPlayersAlive() * 2))
	{
		return;
	}

	if (getElapsedGameTurns() <= (((GC.getHandicapInfo(getHandicapType()).getBarbarianCityCreationTurnsElapsed() * GC.getGameSpeedInfo(getGameSpeedType()).getBarbPercent()) / 100) / std::max(getStartEra() + 1, 1)))
	{
		return;
	}

	/* <advc.300> Create up to two cities per turn, though at most one in an
	   area settled by a civ. Rest of createBarbarianCities (plural) moved into
	   function createBarbCity (singular). */
	createBarbCity(false);
	// A second city at full probability is too much; try halved probability
	createBarbCity(true, 0.5);
}


void CvGame::createBarbCity(bool bSkipCivAreas, float prMod) {

	float cp = (float)GC.getHandicapInfo(getHandicapType()).getBarbarianCityCreationProb();
	/* No cities past Medieval, so it's either +0 (Ancient), +1 (Classical)
	   or +4 (Medieval). */
	cp += std::pow((float)getCurrentEra(), 2);
	if(bSkipCivAreas)
		cp *= prMod;
	// Adjust creation prob to game speed
	CvGameSpeedInfo& gsi = GC.getGameSpeedInfo(getGameSpeedType());
	/*  Time to build a Settler depends on TrainPercent, but overall slow-down
		(captured by BarbPercent) means less time is available for
		training Settlers. Use the mean as a compromise. */
	int adjPercent = (gsi.getTrainPercent() + gsi.getBarbPercent()) / 2;
	cp *= adjPercent / 100.0f;
	if(getSorenRandNum(100, "Barb City Creation") >= ::round(cp)) // </advc.300>
		return;
	EraTypes const gameEra = getCurrentEra(); // advc.003

	int iBestValue = 0;
	CvPlot* pBestPlot = NULL;
	
	/*  advc.003: (comment) This multiplier expresses how close the total
		number of barb cities is to the global target. In contrast, iTargetCities
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
		{
			iTargetCitiesMultiplier += (300 * (iTargetBarbCities - iBarbCities)) / iTargetBarbCities;
		}
		
		if (isOption(GAMEOPTION_RAGING_BARBARIANS))
		{
			iTargetCitiesMultiplier *= 3;
			iTargetCitiesMultiplier /= 2;
		}
	}

	CvPlayerAI::CvFoundSettings kFoundSet(GET_PLAYER(BARBARIAN_PLAYER), false); // K-Mod
	kFoundSet.iMinRivalRange = GC.getDefineINT("MIN_BARBARIAN_CITY_STARTING_DISTANCE");
	/* <advc.300> Randomize penalty on short inter-city distance for more variety
	   in barb settling patterns. The expected value is 8, which is also the
	   value K-Mod uses. */
	kFoundSet.iBarbDiscouragedRange = 5 + getSorenRandNum(7, "advc.300");

	// Precomputed for efficiency
	std::map<int,int> unownedPerArea; int foo=-1;
	for(CvArea* ap = GC.getMap().firstArea(&foo); ap != NULL; ap = GC.getMap().nextArea(&foo)) {
		CvArea const& a = *ap;
		/* Plots owned by barbs are counted in BtS, and I count them when
		   creating units because it makes some sense that barbs get fewer free
		   units once they have cities, but for cities, I'm not sure.
		   Keep counting them for now. */
		std::pair<int,int> ownedUnowned = a.countOwnedUnownedHabitableTiles();
				//a.countOwnedUnownedHabitableTiles(true);
		int iUnowned = ownedUnowned.second;
		std::vector<Shelf*> shelves; GC.getMap().getShelves(a.getID(), shelves);
		for(size_t i = 0; i < shelves.size(); i++)
			iUnowned += shelves[i]->countUnownedPlots() / 2;
		unownedPerArea.insert(std::make_pair(a.getID(), iUnowned));
	}
	bool bRage = isOption(GAMEOPTION_RAGING_BARBARIANS);
	// </advc.300>

	for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
		// <advc.003>
		if(pLoopPlot->isWater() || pLoopPlot->isVisibleToCivTeam())
			continue; // </advc.003>
		// <advc.300>
		int const iAreaSz = pLoopPlot->area()->getNumTiles();
		bool isCivCities = (pLoopPlot->area()->getNumCities() >
				pLoopPlot->area()->getCitiesPerPlayer(BARBARIAN_PLAYER));
		if(bSkipCivAreas && isCivCities)
			continue;
		std::map<int,int>::const_iterator unowned = unownedPerArea.find(
				pLoopPlot->area()->getID());
		FAssert(unowned != unownedPerArea.end());
		int iTargetCities = unowned->second;
		if(bRage) { // Didn't previously affect city density
			iTargetCities *= 7;
			iTargetCities /= 5;
		}
		if(!isCivCities) {
			/*  BtS triples iTargetCities here. Want to make it era-based.
				Important that the multiplier is small in the first two eras
				so that civs get a chance to settle small landmasses before
				barbs appear there. Once there is a barb city on a small landmass,
				there may not be room for another city, and a naval attack on a
				barb city is difficult to execute for the AI (even impossible,
				I think, if the city is landlocked). */
			double mult = std::min(6.0, std::pow(gameEra + 1.0, 2.0) / 3);
			iTargetCities = ::round(mult * iTargetCities); // </advc.300>
		}				
		int iUnownedTilesThreshold = GC.getHandicapInfo(getHandicapType()).getUnownedTilesPerBarbarianCity();
		if(iAreaSz < iUnownedTilesThreshold / 3) {
			iTargetCities *= iTargetCitiesMultiplier;
			iTargetCities /= 100;
		} // <advc.304>
		CvArea& a = *pLoopPlot->area();
		int iDestroyedCities = a.getBarbarianCitiesEverCreated() -
				a.getCitiesPerPlayer(BARBARIAN_PLAYER);
		FAssert(iDestroyedCities >= 0);
		iDestroyedCities = std::max(0, iDestroyedCities);
		iUnownedTilesThreshold += iDestroyedCities * 3; // </advc.304>
		iTargetCities /= std::max(1, iUnownedTilesThreshold);

		if (pLoopPlot->area()->getCitiesPerPlayer(BARBARIAN_PLAYER) < iTargetCities)
		{
			//iValue = GET_PLAYER(BARBARIAN_PLAYER).AI_foundValue(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), GC.getDefineINT("MIN_BARBARIAN_CITY_STARTING_DISTANCE"));
			// K-Mod
			int iValue = GET_PLAYER(BARBARIAN_PLAYER).AI_foundValue_bulk(
					pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), kFoundSet); 
			if (iTargetCitiesMultiplier > 100)
			{/* <advc.300> This gives the area with the most owned tiles priority
				over other areas unless the global city target is reached (rare),
				or the most crowded area hasn't enough unowned tiles left.
				The idea of skipCivAreas is to settle terra incognita earlier,
				so I'm considerably reducing the impact in that case.
				Also, the first city placed in a previously uninhabited area is
				placed randomly b/c each found value gets multipied with 0,
				which is apparently a bug. Let's instead use the projected number
				of owned tiles after placing the city (by adding 9). */
				int iOwned = pLoopPlot->area()->getNumOwnedTiles();
				if(bSkipCivAreas)
					iValue += iOwned;
				else iValue *= iOwned + 9; // advc.001 </advc.300>
			}
			/*  advc.300, advc.001: Looks like another bug.
				Good spots have found values in the thousands; adding between 0
				and 50 is negligible. The division by 100 suggests that times
				1 to 1.5 was intended. The division is pointless in any case b/c
				it applies to all found values alike, and thus doesn't affect
				pBestPlot.
				This kind of randomization is mitigated by the fact that clusters
				of tiles tend to have similar found values. The effect is mostly
				local. I'm trying to get the barbs to also settle mediocre land
				occasionally by randomizing CvFoundSettings.iBarbDiscouragedRange. */
			//iValue += (100 + getSorenRandNum(50, "Barb City Found"));
			//iValue /= 100;
			iValue *= 100 + getSorenRandNum(50, "Barb City Found");
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
		GET_PLAYER(BARBARIAN_PLAYER).found(pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE());
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

	CvUnit* pLoopUnit;
	CvArea* pLoopArea;
	int iLoop;
	bool bAnimals = false;

	// advc.300: Checked later now
	//if (GC.getEraInfo(getCurrentEra()).isNoBarbUnits()) ...
	bool bSpawn = isBarbarianCreationEra(); // advc.307

	if (getNumCivCities() < ((countCivPlayersAlive() * 3) / 2) && !isOption(GAMEOPTION_ONE_CITY_CHALLENGE))
	{
		/*  <advc.300> No need to delay barbs if they start slowly.
			However, added a similar check to CvUnitAI::AI_barbAttackMove
			for slow game speed settings. */
		double crowdedness = countCivPlayersEverAlive();
		crowdedness /= getRecommendedPlayers();
		if(GC.getDefineINT("BARB_PEAK_PERCENT") < 35 || crowdedness > 1.25)
			bAnimals = true; // </advc.300>
	}

	// advc.300: Moved into new function
	if (getGameTurn() < getBarbarianStartTurn())
	{
		bAnimals = true;
	}

	if (bAnimals)
	{
		createAnimals();
	}

	// <advc.300>
	if(bAnimals)
		return;
	CvHandicapInfo const& hci = GC.getHandicapInfo(getHandicapType());
	int baseTilesPerLandUnit = hci.getUnownedTilesPerBarbarianUnit();
	// Divided by 10 b/c now only shelf water tiles count
	int baseTilesPerSeaUnit = hci.getUnownedWaterTilesPerBarbarianUnit() / 8;
	// </advc.300>
	for(pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL;
			pLoopArea = GC.getMapINLINE().nextArea(&iLoop)) {
		// <advc.300> 
		CvArea& a = *pLoopArea;
		/*  For each land area, first spawn sea barbarians for each shelf attached
			to that land area. Skip water areas entirely. Then spawn units on the
			land area. Sea areas go first b/c units can now spawn in cargo;
			spawn fewer land units then.
			No units in unsettled areas. (Need to at least spawn a barbarian city
			before that). */
		if(a.isWater() || a.getNumCities() == 0)
			continue;
		int nUnowned = 0, nTiles = 0;
		std::vector<Shelf*> shelves; GC.getMap().getShelves(a.getID(), shelves);
		for(size_t i = 0; i < shelves.size(); i++) {
			// Shelves also count for land barbarians,
			nUnowned += shelves[i]->countUnownedPlots();
			nTiles += shelves[i]->size();
		}
		// ... but only half.
		nUnowned /= 2; nTiles /= 2;
		/*  For efficiency -- countOwnedUnownedHabitableTiles isn't cached;
			goes through the entire map for each land area, and archipelago-type maps
			can have a lot of those. */
		int nTotal = a.getNumTiles() + nTiles;
		int nUnownedTotal = a.getNumUnownedTiles() + nUnowned;
		if(nUnownedTotal >= nTotal)
			continue;
		/* In the following, only care about "habitable" tiles, i.e. with a
		   positive food yield (implied for shelf tiles).
		   Should tiles visible to a civ count? Yes; it's not unrealistic that
		   barbs originate in one (visible) place, and emerge as a threat
		   in another (invisible) place. */
		std::pair<int,int> ownedUnowned = a.countOwnedUnownedHabitableTiles();
		nUnowned += ownedUnowned.second;
		nTiles += ownedUnowned.first + ownedUnowned.second;
		// NB: Animals are included in this count
		int nLandUnits = a.getUnitsPerPlayer(BARBARIAN_PLAYER);
		//  Kill a barb if the area gets crowded.
		if(killBarb(nLandUnits, nTiles, a.getPopulationPerPlayer(BARBARIAN_PLAYER),
				a, NULL))
			nLandUnits--;
		if(nUnownedTotal < baseTilesPerLandUnit / 2)
			continue;
		int nOwned = nTiles - nUnowned;
		int nBarbCities = a.getCitiesPerPlayer(BARBARIAN_PLAYER);
		int neededLand = numBarbariansToSpawn(baseTilesPerLandUnit, nTiles,
				nUnowned, nLandUnits, nBarbCities);
		for(unsigned int i = 0; i < shelves.size(); i++) {
			int nShips = shelves[i]->countBarbarians();
			if(killBarb(nShips, shelves[i]->size(),
					a.getPopulationPerPlayer(BARBARIAN_PLAYER), a, shelves[i]))
				nShips--;
			if(!bSpawn)
				continue;
			int neededSea = numBarbariansToSpawn(baseTilesPerSeaUnit,
					shelves[i]->size(),
                    shelves[i]->countUnownedPlots(),
                    nShips);
			// (Deleted) Use a different sanity check, based on barb cities.
			/* ``BETTER_BTS_AI_MOD 9/25/08 jdog5000
				 Limit construction of barb ships based on player navies [...]´´ */
			if(nShips > nBarbCities + 2)
				neededSea = 0;
			neededLand -= spawnBarbarians(neededSea, a, shelves[i],
					neededLand > 0); // advc.306
		}
		/*  Don't spawn barb. units on (or on shelves around) continents where
			civs don't outnumber barbs */
		int cc = a.countCivCities(); int bc = a.getNumCities() - cc; FAssert(bc >= 0);
		if(cc > bc && bSpawn)
			spawnBarbarians(neededLand, a, NULL);
		/*  Rest of the creation code: moved into functions numBarbariansToSpawn and
			spawnBarbarians */
		// </advc.300>
	}
	for (pLoopUnit = GET_PLAYER(BARBARIAN_PLAYER).firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = GET_PLAYER(BARBARIAN_PLAYER).nextUnit(&iLoop))
	{
		if (pLoopUnit->isAnimal()
				// advc.309: Don't cull animals where there are no civ cities
				&& pLoopUnit->area()->countCivCities() > 0
			)
		{
			pLoopUnit->kill(false);
			break;
		}
	} // <advc.300>
	for(CvCity* c = GET_PLAYER(BARBARIAN_PLAYER).firstCity(&iLoop); c != NULL;
			c = GET_PLAYER(BARBARIAN_PLAYER).nextCity(&iLoop)) {
		/*  Large Barb congregations are only a problem if they have nothing
			to attack */
		if(c->area()->countCivCities() > 0)
			continue;
		int unitCount = c->plot()->getNumDefenders(BARBARIAN_PLAYER);
		double pr = (unitCount - std::max(1.5 * c->getPopulation(), 4.0)) / 4.0;
		if(::bernoulliSuccess(pr, "advc.300"))
			c->plot()->killRandomUnit(BARBARIAN_PLAYER, DOMAIN_LAND);
	} // </advc.300>
}


void CvGame::createAnimals()
{
	CvArea* pLoopArea;
	CvPlot* pPlot;
	UnitTypes eBestUnit;
	UnitTypes eLoopUnit;
	int iNeededAnimals;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI, iJ;

	if (GC.getEraInfo(getCurrentEra()).isNoAnimals()
			|| isOption(GAMEOPTION_NO_ANIMALS)) // advc.309
		return;

	if (GC.getHandicapInfo(getHandicapType()).getUnownedTilesPerGameAnimal() <= 0)
	{
		return;
	}

	if (getNumCivCities() < countCivPlayersAlive())
	{
		return;
	}

	if (getElapsedGameTurns() < 5)
	{
		return;
	}


	for(pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		if (!(pLoopArea->isWater()))
		{
			iNeededAnimals = (pLoopArea->getNumUnownedTiles() / GC.getHandicapInfo(getHandicapType()).getUnownedTilesPerGameAnimal());
			/* advc.300: Place a couple of "bisons".
			   Culling kicks in when a civ city is founded. */
			if(pLoopArea->countCivCities() == 0) iNeededAnimals /= 2;
			iNeededAnimals -= pLoopArea->getUnitsPerPlayer(BARBARIAN_PLAYER);

			if (iNeededAnimals > 0)
			{
				iNeededAnimals = ((iNeededAnimals / 5) + 1);

				for (iI = 0; iI < iNeededAnimals; iI++)
				{
					pPlot = GC.getMapINLINE().syncRandPlot(
							(RANDPLOT_NOT_VISIBLE_TO_CIV | RANDPLOT_PASSABLE
							| RANDPLOT_WATERSOURCE), // advc.300
							pLoopArea->getID(), GC.getDefineINT("MIN_ANIMAL_STARTING_DISTANCE"));

					if (pPlot != NULL)
					{
						eBestUnit = NO_UNIT;
						iBestValue = 0;

						// advc.003 (comment): Picks an animal that is suitable for pPlot
						for (iJ = 0; iJ < GC.getNumUnitClassInfos(); iJ++)
						{
							eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(GET_PLAYER(BARBARIAN_PLAYER).getCivilizationType()).getCivilizationUnits(iJ)));

							if (eLoopUnit != NO_UNIT)
							{
								if (GC.getUnitInfo(eLoopUnit).getUnitAIType(UNITAI_ANIMAL))
								{
									if ((pPlot->getFeatureType() != NO_FEATURE) ? GC.getUnitInfo(eLoopUnit).getFeatureNative(pPlot->getFeatureType()) : GC.getUnitInfo(eLoopUnit).getTerrainNative(pPlot->getTerrainType()))
									{
										iValue = (1 + getSorenRandNum(1000, "Animal Unit Selection"));

										if (iValue > iBestValue)
										{
											eBestUnit = eLoopUnit;
											iBestValue = iValue;
										}
									}
								}
							}
						}

						if (eBestUnit != NO_UNIT)
						{
							GET_PLAYER(BARBARIAN_PLAYER).initUnit(eBestUnit, pPlot->getX_INLINE(), pPlot->getY_INLINE(), UNITAI_ANIMAL);
						}
					}
				}
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
			/*  Also stop spawning when barb tech falls behind too much;
				may resume once Barb tech catches up. */
			eCurrentEra <= GET_PLAYER(BARBARIAN_PLAYER).getCurrentEra() + 1);
}

// <advc.300>
int CvGame::getBarbarianStartTurn() const {

	int targetElapsed = GC.getHandicapInfo(getHandicapType()).
		   getBarbarianCreationTurnsElapsed();
	targetElapsed *= GC.getGameSpeedInfo(getGameSpeedType()).getBarbPercent();
	int divisor = 100;
	/*  This term is new. Well, not entirely, it's also applied to
		BarbarianCityCreationTurnsElapsed. */
	divisor *= std::max(1, (int)getStartEra());
	targetElapsed /= divisor;
	int startTurn = getStartTurn();
	// Have barbs appear earlier in Ancient Advanced Start too.
	if(isOption(GAMEOPTION_ADVANCED_START) && getStartEra() <= 0 &&
			// advc.250b: Earlier barbs only if humans start advanced.
			!isOption(GAMEOPTION_SPAH))
		startTurn /= 2;
	return startTurn + targetElapsed;
}

// Based on code originally in createBarbarianUnits, but modded beyond recognition.
int CvGame::numBarbariansToSpawn(int iTilesPerUnit, int iTiles, int iUnowned,
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
		divisor = ::round(divisor * rageMultiplier);
	} // </advc.307>
	double target = std::min(dividend / divisor,
			/* Make sure that there's enough unowned land where the barbs could
			   plausibly gather. */
			iUnowned / 6.0);
	double adjustment = GC.getDefineINT("BARB_ACTIVITY_ADJUSTMENT") + 100;
	adjustment /= 100;
	target *= adjustment;
	
	int iInitialDefenders = GC.getHandicapInfo(getHandicapType()).
			getBarbarianInitialDefenders();
	int iNeeded = (int)((target - iUnitsPresent)
	/*  The (unmodded) term above counts city defenders when determining
		how many more Barbarians should be placed. That means, Barbarian cities can
		decrease Barbarian aggressiveness in two ways: By reducing the number of
		unowned tiles, and by shifting 2 units (standard size of a city garrison)
		per city to defensive behavior. While settled Barbarians being less
		aggressive is plausible, this goes a bit over the top. Also don't want
		units produced in Barbarian cities to proportionally reduce the number of
		spawned barbarians.
		Subtract the defenders. (Alt. idea: Subtract half the Barbarian population in
		this area.)
		Old Firaxis to-do comment on this subject:
		``XXX eventually need to measure how many barbs of eBarbUnitAI we have
		  in this area...´´ */
		+ iBarbarianCities * std::max(0, iInitialDefenders));
	if(iNeeded <= 0)
		return 0;
	double spawnRate = 0.25; // the BtS rate
	// Novel: adjusted to game speed
	CvGameSpeedInfo const& speed = GC.getGameSpeedInfo(getGameSpeedType());
	/*  See comment in createBarbarianCities about using the mean of
		TrainPercent and BarbPercent. */
	spawnRate /= ((speed.getTrainPercent() + speed.getBarbPercent()) / 200.0);
	double r = iNeeded * spawnRate;
	/* BtS always spawns at least one unit, but on Marathon, this could be too fast.
		Probabilistic instead. */
	if(r < 1) {
		if(::bernoulliSuccess(r, "advc.300"))
			return 1;
		else return 0;
	}
	return ::round(r);
}

// Returns the number of land units spawned (possibly in cargo)
int CvGame::spawnBarbarians(int n, CvArea& a, Shelf* shelf,
		bool bCargoAllowed) {

  // </advc.300>
	/* <advc.306> Spawn cargo load before ships. Othwerwise, the newly placed ship
	   would always be an eligible target, and too many ships would carry cargo. */
	FAssert(!bCargoAllowed || shelf != NULL);
	int r = 0;
	if(bCargoAllowed) {
		CvUnit* cargo = shelf->randomBarbCargoUnit();
		if(cargo != NULL) {
			UnitAITypes loadAI = UNITAI_ATTACK;
			for(int i = 0; i < 2; i++) {
				UnitTypes lut = randomBarbUnit(loadAI, a);
				if(lut == NO_UNIT)
					break;
				CvUnit* load = GET_PLAYER(BARBARIAN_PLAYER).initUnit(
						lut, cargo->getX(), cargo->getY(), loadAI);
				/*  Don't set cargo to UNITAI_ASSAULT_SEA - that's for medium-/
					large-scale invasions, and too laborious to adjust. Instead
					add an unload routine to CvUnitAI::barbAttackSeaMove. */
				if(load == NULL)
					break;
				load->setTransportUnit(cargo);
				r++;
				/*  Only occasionally spawn two units at once. Prefer the natural
					way, i.e. a ship receiving a second passenger while travelling
					to its target through fog of war. I don't think that happens
					often enough though. */
				if(cargo->getCargo() > 1 || ::bernoulliSuccess(0.7, "advc.306"))
					break;
			}
		}
	}
	// From here on, mostly cut and pasted from createBarbarianUnits. </advc.306>

	CvPlot* pPlot=NULL;
	for (int iI = 0; iI < n; iI++)
	{
        // <advc.300>
		// Reroll twice if the tile has poor yield
		for(int i = 0; i < 3; i++) {
			pPlot = randomBarbPlot(a, shelf);
			/*  If we can't find a plot once, we won't find one in a later
				iteration either. */
			if(pPlot == NULL)
				return r;
			int iTotalYield = 0;
			for(int j = 0; j < GC.getNUM_YIELD_TYPES(); j++)
				iTotalYield += pPlot->getYield((YieldTypes)j);
			// Want to re-roll flat Tundra Forest as well
			if(iTotalYield == 2 && pPlot->getImprovementType() == NO_IMPROVEMENT) {
				iTotalYield = 0;
				for(int j = 0; j < GC.getNUM_YIELD_TYPES(); j++)
					iTotalYield += pPlot->calculateNatureYield((YieldTypes)j,
							NO_TEAM, true); // Ignore feature
			}
			if(iTotalYield >= 2)
				break;
		}
		UnitAITypes ai = UNITAI_ATTACK;
		if(shelf != NULL)
			ai = UNITAI_ATTACK_SEA;
		// Original code moved into new function:
		UnitTypes ut = randomBarbUnit(ai, a);
		if(ut == NO_UNIT)
			return r;
		CvUnit* pNewUnit = GET_PLAYER(BARBARIAN_PLAYER).initUnit(ut,
				pPlot->getX(), pPlot->getY(), ai);
		if(pNewUnit != NULL && !pPlot->isWater()) r++;
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
CvPlot* CvGame::randomBarbPlot(CvArea const& a, Shelf* shelf) const {

	int restrictionFlags = RANDPLOT_NOT_VISIBLE_TO_CIV |
			/*  Shelves already ensure this and one-tile islands
				can't spawn barbs anyway. */
			//RANDPLOT_ADJACENT_LAND |
			RANDPLOT_PASSABLE |
			RANDPLOT_HABITABLE | // New flag
			RANDPLOT_UNOWNED;
	/*  Added the "unowned" flag to prevent spawning in barbarian land.
		Could otherwise happen now b/c the visible flag and dist. restriction
		no longer apply to barbarians previously spawned; see
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


bool CvGame::killBarb(int iPresent, int iTiles, int iBarbPop, CvArea& a, Shelf* shelf) {

	if(iPresent <= 5) // 5 is never a crowd
		return false;
	double divisor = 4 * iBarbPop;
	if(shelf != NULL)
		divisor += shelf->size();
	else divisor += iTiles; /*  Includes 50% shelf (given the way this function
								is currently used). */
	// Don't want large barb continents crawling with units
	divisor = 5 * std::pow(divisor, 0.7);
	if(::bernoulliSuccess(iPresent / divisor, "advc.300 (kill)")) {
		if(shelf != NULL)
			return shelf->killBarb();
		/*  Tbd.: Be a bit more considerate about which unit to sacrifice.
			Currently, it's the same (arbitrary) method as for animal culling. */
		int foo=-1;
		for(CvUnit* up = GET_PLAYER(BARBARIAN_PLAYER).firstUnit(&foo);
				up != NULL; up = GET_PLAYER(BARBARIAN_PLAYER).nextUnit(&foo)) {
			CvUnit& u = *up;
			if(u.isAnimal() || u.plot()->area()->getID() != a.getID() ||
					u.getUnitCombatType() == NO_UNITCOMBAT)
				continue;
			u.kill(false);
			return true;
		}
	}
	return false;
}


// Code cut from createBarbarianUnits and refactored
UnitTypes CvGame::randomBarbUnit(UnitAITypes ai, CvArea const& a) {

	bool sea;
	switch(ai) {
	case UNITAI_ATTACK_SEA: sea = true; break;
	case UNITAI_ATTACK: sea = false; break;
	default: return NO_UNIT; }
	UnitTypes r = NO_UNIT;
	int bestVal = 0;
	for(int i = 0; i < GC.getNumUnitClassInfos(); i++) {
		UnitTypes ut = (UnitTypes)(GC.getCivilizationInfo(
				GET_PLAYER(BARBARIAN_PLAYER).getCivilizationType()).
				getCivilizationUnits(i));
		if(ut == NO_UNIT)
			continue;
		CvUnitInfo& u = GC.getUnitInfo(ut);
		DomainTypes dom = (DomainTypes)u.getDomainType();
		if(u.getCombat() <= 0 || dom == DOMAIN_AIR ||
				::isMostlyDefensive(u) || // advc.315
				(dom == DOMAIN_SEA) != sea ||
				!GET_PLAYER(BARBARIAN_PLAYER).canTrain(ut))
			continue;
		// <advc.301>
		BonusTypes andReq = (BonusTypes)u.getPrereqAndBonus();
		TechTypes andReqTech = NO_TECH;
		if(andReq != NO_BONUS) {
			andReqTech = (TechTypes)GC.getBonusInfo(andReq).getTechCityTrade();
			if((andReqTech != NO_TECH && !GET_TEAM(BARBARIAN_TEAM).
					isHasTech(andReqTech)) || !a.hasAnyAreaPlayerBonus(andReq))
				continue;
		}
		/*  No units from more than 1 era ago (obsoletion too difficult to test).
			hasTech already tested by canTrain, but era shouldn't be
			tested there b/c it's OK for Barbarian cities to train outdated units
			(they only will if they can't train anything better). */
		TechTypes reqTech = (TechTypes)u.getPrereqAndTech();
		int unitEra = 0;
		if(reqTech != NO_TECH)
			unitEra = GC.getTechInfo(reqTech).getEra();
		if(andReqTech != NO_TECH)
			unitEra = std::max(unitEra, GC.getTechInfo(andReqTech).getEra());
		if(unitEra + 1 < getCurrentEra())
			continue; // </advc.301>
		bool bFound = false;
		bool bRequires = false;
		for(int j = 0; j < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); j++) {
			BonusTypes orReq = (BonusTypes)u.getPrereqOrBonuses(j);
			if(orReq == NO_BONUS)
				continue;
			CvBonusInfo& orReqInf = GC.getBonusInfo(orReq);
			TechTypes eTech = (TechTypes)orReqInf.getTechCityTrade();
			if(eTech != NO_TECH) {
				bRequires = true;
				if(GET_TEAM(BARBARIAN_TEAM).isHasTech(eTech)
						/*  advc.301: Also require the resource to be connected by
							someone on this continent; in particular, don't spawn
							Horse Archers on a horseless continent. */
						&& a.hasAnyAreaPlayerBonus(orReq)) {
					bFound = true;
					break;
				}
			}
		}
		if(bRequires && !bFound)
			continue;
		/*  <advc.301>: The code above only checks if they can build the
			improvements necessary to obtain the required bonus resources;
			it does not check if they can see/use the resource. This means
			that Spearmen often appear before Archers b/c they require only
			Hunting and Mining, and not Bronze Working. Correction: */
		if(!GET_TEAM(BARBARIAN_TEAM).canSeeReqBonuses(ut))
			continue; // </advc.301>
		int val = (1 + getSorenRandNum(1000, "Barb Unit Selection"));
		if(u.getUnitAIType(ai))
			val += 200;
		if(val > bestVal) {
			r = ut;
			bestVal = val;
		}
	}
	return r;
} // </advc.300>


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
	CvSelectionGroup* pLoopSelectionGroup;
	int aiShuffle[MAX_PLAYERS];
	int iLoop;
	int iI;

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
					for(pLoopSelectionGroup = player.firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = player.nextSelectionGroup(&iLoop))
					{
						pLoopSelectionGroup->autoMission();
					}
					// K-Mod. Here's where we do the AI for automated units.
					// Note, we can't do AI_update and autoMission in the same loop, because either one might delete the group - and thus cause the other to crash.
					if (player.isHuman())
					{
						for (pLoopSelectionGroup = player.firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = player.nextSelectionGroup(&iLoop))
						{
							if (pLoopSelectionGroup->AI_update())
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

					if (!(player.hasBusyUnit()))
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


void CvGame::updateTurnTimer()
{
	int iI;

	// Are we using a turn timer?
	if (isMPOption(MPOPTION_TURN_TIMER))
	{
		if (getElapsedGameTurns() > 0 || !isOption(GAMEOPTION_ADVANCED_START))
		{
			// Has the turn expired?
			if (getTurnSlice() > getCutoffSlice())
			{
				for (iI = 0; iI < MAX_PLAYERS; iI++)
				{
					if (GET_PLAYER((PlayerTypes)iI).isAlive() && GET_PLAYER((PlayerTypes)iI).isTurnActive())
					{
						GET_PLAYER((PlayerTypes)iI).setEndTurn(true);

						if (!isMPOption(MPOPTION_SIMULTANEOUS_TURNS) && !isSimultaneousTeamTurns())
						{
							break;
						}
					}
				}
			}
		}
	}
}


void CvGame::testAlive()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		GET_PLAYER((PlayerTypes)iI).verifyAlive();
	}
}

bool CvGame::testVictory(VictoryTypes eVictory, TeamTypes eTeam, bool* pbEndScore) const
{
	FAssert(eVictory >= 0 && eVictory < GC.getNumVictoryInfos());
	FAssert(eTeam >=0 && eTeam < MAX_CIV_TEAMS);
	FAssert(GET_TEAM(eTeam).isAlive());

	bool bValid = isVictoryValid(eVictory);
	if (pbEndScore)
	{
		*pbEndScore = false;
	}

	if (bValid)
	{
		if (GC.getVictoryInfo(eVictory).isEndScore())
		{
			if (pbEndScore)
			{
				*pbEndScore = true;
			}

			if (getMaxTurns() == 0)
			{
				bValid = false;
			}
			else if (getElapsedGameTurns() < getMaxTurns())
			{
				bValid = false;
			}
			else
			{
				bool bFound = false;

				for (int iK = 0; iK < MAX_CIV_TEAMS; iK++)
				{
					if (GET_TEAM((TeamTypes)iK).isAlive())
					{
						if (iK != eTeam)
						{
							if (getTeamScore((TeamTypes)iK) >= getTeamScore(eTeam))
							{
								bFound = true;
								break;
							}
						}
					}
				}

				if (bFound)
				{
					bValid = false;
				}
			}
		}
	}

	if (bValid)
	{
		if (GC.getVictoryInfo(eVictory).isTargetScore())
		{
			if (getTargetScore() == 0)
			{
				bValid = false;
			}
			else if (getTeamScore(eTeam) < getTargetScore())
			{
				bValid = false;
			}
			else
			{
				bool bFound = false;

				for (int iK = 0; iK < MAX_CIV_TEAMS; iK++)
				{
					if (GET_TEAM((TeamTypes)iK).isAlive())
					{
						if (iK != eTeam)
						{
							if (getTeamScore((TeamTypes)iK) >= getTeamScore(eTeam))
							{
								bFound = true;
								break;
							}
						}
					}
				}

				if (bFound)
				{
					bValid = false;
				}
			}
		}
	}

	if (bValid)
	{
		if (GC.getVictoryInfo(eVictory).isConquest())
		{
			if (GET_TEAM(eTeam).getNumCities() == 0)
			{
				bValid = false;
			}
			else
			{
				bool bFound = false;

				for (int iK = 0; iK < MAX_CIV_TEAMS; iK++)
				{
					if (GET_TEAM((TeamTypes)iK).isAlive())
					{
						if (iK != eTeam && !GET_TEAM((TeamTypes)iK).isVassal(eTeam))
						{
							if (GET_TEAM((TeamTypes)iK).getNumCities() > 0)
							{
								bFound = true;
								break;
							}
						}
					}
				}

				if (bFound)
				{
					bValid = false;
				}
			}
		}
	}

	if (bValid)
	{
		if (GC.getVictoryInfo(eVictory).isDiploVote())
		{
			bool bFound = false;

			for (int iK = 0; iK < GC.getNumVoteInfos(); iK++)
			{
				if (GC.getVoteInfo((VoteTypes)iK).isVictory())
				{
					if (getVoteOutcome((VoteTypes)iK) == eTeam)
					{
						bFound = true;
						break;
					}
				}
			}

			if (!bFound)
			{
				bValid = false;
			}
		}
	}

	if (bValid)
	{
		if (getAdjustedPopulationPercent(eVictory) > 0)
		{
			if (100 * GET_TEAM(eTeam).getTotalPopulation() < getTotalPopulation() * getAdjustedPopulationPercent(eVictory))
			{
				bValid = false;
			}
		}
	}

	if (bValid)
	{
		if (getAdjustedLandPercent(eVictory) > 0)
		{
			if (100 * GET_TEAM(eTeam).getTotalLand() < GC.getMapINLINE().getLandPlots() * getAdjustedLandPercent(eVictory))
			{
				bValid = false;
			}
		}
	}

	if (bValid)
	{
		if (GC.getVictoryInfo(eVictory).getReligionPercent() > 0)
		{
			bool bFound = false;

			if (getNumCivCities() > (countCivPlayersAlive() * 2))
			{
				for (int iK = 0; iK < GC.getNumReligionInfos(); iK++)
				{
					if (GET_TEAM(eTeam).hasHolyCity((ReligionTypes)iK))
					{
						if (calculateReligionPercent((ReligionTypes)iK) >= GC.getVictoryInfo(eVictory).getReligionPercent())
						{
							bFound = true;
							break;
						}
					}

					if (bFound)
					{
						break;
					}
				}
			}

			if (!bFound)
			{
				bValid = false;
			}
		}
	}

	if (bValid)
	{
		if ((GC.getVictoryInfo(eVictory).getCityCulture() != NO_CULTURELEVEL) && (GC.getVictoryInfo(eVictory).getNumCultureCities() > 0))
		{
			int iCount = 0;

			for (int iK = 0; iK < MAX_CIV_PLAYERS; iK++)
			{
				if (GET_PLAYER((PlayerTypes)iK).isAlive())
				{
					if (GET_PLAYER((PlayerTypes)iK).getTeam() == eTeam)
					{
						int iLoop;
						for (CvCity* pLoopCity = GET_PLAYER((PlayerTypes)iK).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iK).nextCity(&iLoop))
						{
							if (pLoopCity->getCultureLevel() >= GC.getVictoryInfo(eVictory).getCityCulture())
							{
								iCount++;
							}
						}
					}
				}
			}

			if (iCount < GC.getVictoryInfo(eVictory).getNumCultureCities())
			{
				bValid = false;
			}
		}
	}

	if (bValid)
	{
		if (GC.getVictoryInfo(eVictory).getTotalCultureRatio() > 0)
		{
			int iThreshold = ((GET_TEAM(eTeam).countTotalCulture() * 100) / GC.getVictoryInfo(eVictory).getTotalCultureRatio());

			bool bFound = false;

			for (int iK = 0; iK < MAX_CIV_TEAMS; iK++)
			{
				if (GET_TEAM((TeamTypes)iK).isAlive())
				{
					if (iK != eTeam)
					{
						if (GET_TEAM((TeamTypes)iK).countTotalCulture() > iThreshold)
						{
							bFound = true;
							break;
						}
					}
				}
			}

			if (bFound)
			{
				bValid = false;
			}
		}
	}

	if (bValid)
	{
		for (int iK = 0; iK < GC.getNumBuildingClassInfos(); iK++)
		{
			if (GC.getBuildingClassInfo((BuildingClassTypes) iK).getVictoryThreshold(eVictory) > GET_TEAM(eTeam).getBuildingClassCount((BuildingClassTypes)iK))
			{
				bValid = false;
				break;
			}
		}
	}

	if (bValid)
	{
		for (int iK = 0; iK < GC.getNumProjectInfos(); iK++)
		{
			if (GC.getProjectInfo((ProjectTypes) iK).getVictoryMinThreshold(eVictory) > GET_TEAM(eTeam).getProjectCount((ProjectTypes)iK))
			{
				bValid = false;
				break;
			}
		}
	}

	if (bValid)
	{
		long lResult = 1;
		CyArgsList argsList;
		argsList.add(eVictory);
		gDLL->getPythonIFace()->callFunction(PYGameModule, "isVictory", argsList.makeFunctionArgs(), &lResult);
		if (0 == lResult)
		{
			bValid = false;
		}
	}

	return bValid;
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

	long lResult = 1; // advc.003 (comment): This checks if 10 turns have elapsed
	gDLL->getPythonIFace()->callFunction(PYGameModule, "isVictoryTest", NULL, &lResult);
	if (lResult == 0)
	{
		return;
	}

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

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/02/09                                jdog5000      */
/*                                                                                              */
/* AI logging                                                                                   */
/************************************************************************************************/
			if( gTeamLogLevel >= 1 )
			{
				logBBAI("  Vote for forcing peace against team %d (%S) passes", kPlayer.getTeam(), kPlayer.getCivilizationDescription(0) );
			}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
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

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/02/09                                jdog5000      */
/*                                                                                              */
/* AI logging                                                                                   */
/************************************************************************************************/
			if( gTeamLogLevel >= 1 )
			{
				logBBAI("  Vote for war against team %d (%S) passes", kPlayer.getTeam(), kPlayer.getCivilizationDescription(0) );
			}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

			for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
			{
				CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
				//if (kLoopPlayer.isVotingMember(kData.eVoteSource))
				// dlph.25/advc:
				if(GET_TEAM(kLoopPlayer.getTeam()).isFullMember(kData.eVoteSource))
				{
					if (GET_TEAM(kLoopPlayer.getTeam()).canChangeWarPeace(kPlayer.getTeam()))
					{
						GET_TEAM(kLoopPlayer.getTeam()).declareWar(kPlayer.getTeam(), false, WARPLAN_DOGPILE);
						// advc.104i:
						GET_TEAM(kPlayer.getTeam()).makeUnwillingToTalk(kLoopPlayer.getTeam());
					}
				}
			}

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
				if (NO_PLAYER != kData.kVoteOption.eOtherPlayer && kData.kVoteOption.eOtherPlayer != pCity->getOwnerINLINE())
				{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/02/09                                jdog5000      */
/*                                                                                              */
/* AI logging                                                                                   */
/************************************************************************************************/
					if( gTeamLogLevel >= 1 )
					{
						logBBAI("  Vote for assigning %S to %d (%S) passes", pCity->getName().GetCString(), GET_PLAYER(kData.kVoteOption.eOtherPlayer).getTeam(), GET_PLAYER(kData.kVoteOption.eOtherPlayer).getCivilizationDescription(0) );
					}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
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

// <advc.003>
 CvDeal* CvGame::getDeal(int iID)																		
{
	return m_deals.getAt(iID);
}


CvDeal* CvGame::addDeal()
{
	return m_deals.add();
}
// </advc.003>

 void CvGame::deleteDeal(int iID)
{
	m_deals.removeAt(iID);
	gDLL->getInterfaceIFace()->setDirty(Foreign_Screen_DIRTY_BIT, true);
}

CvDeal* CvGame::firstDeal(int *pIterIdx, bool bRev)
{
	return !bRev ? m_deals.beginIter(pIterIdx) : m_deals.endIter(pIterIdx);
}


CvDeal* CvGame::nextDeal(int *pIterIdx, bool bRev)
{
	return !bRev ? m_deals.nextIter(pIterIdx) : m_deals.prevIter(pIterIdx);
}


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


int CvGame::getSorenRandNum(int iNum, const char* pszLog)
{
	return m_sorenRand.get(iNum, pszLog);
}


int CvGame::calculateSyncChecksum()
{
	PROFILE_FUNC();

	int iMultiplier;
	int iValue;
	int iI, iJ;

	iValue = 0;

	iValue += getMapRand().getSeed();
	iValue += getSorenRand().getSeed();

	iValue += getNumCities();
	iValue += getTotalPopulation();
	iValue += getNumDeals();

	iValue += GC.getMapINLINE().getOwnedPlots();
	iValue += GC.getMapINLINE().getNumAreas();

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isEverAlive())
		{
			iMultiplier = getPlayerScore((PlayerTypes)iI);

			//switch (getTurnSlice() % 4)
			switch (getTurnSlice() % 8) // K-Mod
			{
			case 0:
				iMultiplier += (GET_PLAYER((PlayerTypes)iI).getTotalPopulation() * 543271);
				iMultiplier += (GET_PLAYER((PlayerTypes)iI).getTotalLand() * 327382);
				iMultiplier += (GET_PLAYER((PlayerTypes)iI).getGold() * 107564);
				iMultiplier += (GET_PLAYER((PlayerTypes)iI).getAssets() * 327455);
				iMultiplier += (GET_PLAYER((PlayerTypes)iI).getPower() * 135647);
				iMultiplier += (GET_PLAYER((PlayerTypes)iI).getNumCities() * 436432);
				iMultiplier += (GET_PLAYER((PlayerTypes)iI).getNumUnits() * 324111);
				iMultiplier += (GET_PLAYER((PlayerTypes)iI).getNumSelectionGroups() * 215356);
				break;

			case 1:
				for (iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
				{
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).calculateTotalYield((YieldTypes)iJ) * 432754);
				}

				for (iJ = 0; iJ < NUM_COMMERCE_TYPES; iJ++)
				{
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).getCommerceRate((CommerceTypes)iJ) * 432789);
				}
				break;

			case 2:
				for (iJ = 0; iJ < GC.getNumBonusInfos(); iJ++)
				{
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).getNumAvailableBonuses((BonusTypes)iJ) * 945732);
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).getBonusImport((BonusTypes)iJ) * 326443);
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).getBonusExport((BonusTypes)iJ) * 932211);
				}

				for (iJ = 0; iJ < GC.getNumImprovementInfos(); iJ++)
				{
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).getImprovementCount((ImprovementTypes)iJ) * 883422);
				}

				for (iJ = 0; iJ < GC.getNumBuildingClassInfos(); iJ++)
				{
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).getBuildingClassCountPlusMaking((BuildingClassTypes)iJ) * 954531);
				}

				for (iJ = 0; iJ < GC.getNumUnitClassInfos(); iJ++)
				{
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).getUnitClassCountPlusMaking((UnitClassTypes)iJ) * 754843);
				}

				for (iJ = 0; iJ < NUM_UNITAI_TYPES; iJ++)
				{
					iMultiplier += (GET_PLAYER((PlayerTypes)iI).AI_totalUnitAIs((UnitAITypes)iJ) * 643383);
				}
				break;

			case 3:
			{
				CvUnit* pLoopUnit;
				int iLoop;

				for (pLoopUnit = GET_PLAYER((PlayerTypes)iI).firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = GET_PLAYER((PlayerTypes)iI).nextUnit(&iLoop))
				{
					iMultiplier += (pLoopUnit->getX_INLINE() * 876543);
					iMultiplier += (pLoopUnit->getY_INLINE() * 985310);
					iMultiplier += (pLoopUnit->getDamage() * 736373);
					iMultiplier += (pLoopUnit->getExperience() * 820622);
					iMultiplier += (pLoopUnit->getLevel() * 367291);
				}
				break;
			}
			// K-Mod - new checks.
			case 4:
				// attitude cache
				for (iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					if(iI!=iJ) // advc.003: self-attitude should never matter
						iMultiplier += GET_PLAYER((PlayerTypes)iI).AI_getAttitudeVal((PlayerTypes)iJ, false) << iJ;
				}
				// strategy hash
				//iMultiplier += GET_PLAYER((PlayerTypes)iI).AI_getStrategyHash() * 367291;
				break;
			case 5:
			{
				// city religions and corporations
				CvCity* pLoopCity;
				int iLoop;

				for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
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
			}
			case 6:
			{
				// city production
				/* CvCity* pLoopCity;
				int iLoop;

				for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
				{
					CLLNode<OrderData>* pOrderNode = pLoopCity->headOrderQueueNode();
					if (pOrderNode != NULL)
					{
						iMultiplier += pLoopCity->getID()*(pOrderNode->m_data.eOrderType+2*pOrderNode->m_data.iData1+3*pOrderNode->m_data.iData2+6);
					}
				}
				break; */
				// city health and happiness
				CvCity* pLoopCity;
				int iLoop;

				for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
				{
					iMultiplier += pLoopCity->goodHealth() * 876543;
					iMultiplier += pLoopCity->badHealth() * 985310;
					iMultiplier += pLoopCity->happyLevel() * 736373;
					iMultiplier += pLoopCity->unhappyLevel() * 820622;
					iMultiplier += pLoopCity->getFood() * 367291;
				}
				break;
			}
			case 7:
			{
				// city event history
				CvCity* pLoopCity;
				int iLoop;

				for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
				{
					for (iJ = 0; iJ < GC.getNumEventInfos(); iJ++)
					{
						iMultiplier += (iJ+1)*pLoopCity->isEventOccured((EventTypes)iJ);
					}
				}
				break;
			}
			// K-Mod end
			} // end TimeSlice switch

			if (iMultiplier != 0)
			{
				iValue *= iMultiplier;
			}
		}
	}

	return iValue;
}


int CvGame::calculateOptionsChecksum()
{
	PROFILE_FUNC();

	int iValue;
	int iI, iJ;

	iValue = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		for (iJ = 0; iJ < NUM_PLAYEROPTION_TYPES; iJ++)
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


void CvGame::addReplayMessage(ReplayMessageTypes eType, PlayerTypes ePlayer, CvWString pszText, int iPlotX, int iPlotY, ColorTypes eColor)
{
	int iGameTurn = getGameTurn();
	CvReplayMessage* pMessage = new CvReplayMessage(iGameTurn, eType, ePlayer);
	if (NULL != pMessage)
	{
		pMessage->setPlot(iPlotX, iPlotY);
		pMessage->setText(pszText);
		if (NO_COLOR == eColor)
		{
			eColor = (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE");
		}
		pMessage->setColor(eColor);
		m_listReplayMessages.push_back(pMessage);
	}
}

void CvGame::clearReplayMessageMap()
{
	for (ReplayMessageList::const_iterator itList = m_listReplayMessages.begin(); itList != m_listReplayMessages.end(); itList++)
	{
		const CvReplayMessage* pMessage = *itList;
		if (NULL != pMessage)
		{
			delete pMessage;
		}
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
		spah.read(pStream); // </advc.250b><advc.701>
	if(uiFlag >= 2) {
		if(isOption(GAMEOPTION_RISE_FALL))
			riseFall.read(pStream);
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
}


void CvGame::write(FDataStreamBase* pStream)
{
	int iI;

	uint uiFlag=1;
	uiFlag = 2; // advc.701: R&F option
	uiFlag = 3; // advc.052
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
		spah.write(pStream); // </advc.250b><advc.701>
	if(isOption(GAMEOPTION_RISE_FALL))
		riseFall.write(pStream); // </advc.701>
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
			m_pReplayInfo->setFinalScore(riseFall.getFinalRiseScore());
		// </advc.707>
		m_pReplayInfo->write(stream);
	}
}

void CvGame::saveReplay(PlayerTypes ePlayer)
{	// advc.106i: Hack to prepend sth. to the replay file name
	GET_PLAYER(ePlayer).setSavingReplay(true);
	gDLL->getEngineIFace()->SaveReplay(ePlayer);
	/*  advc.106i: Probably redundant b/c CvGame::writeReplay already sets it
		to false */
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
	if ( (eOldLeader != NO_LEADER) && (eOldLeader != eLeader) ) 
	{
		GC.getInitCore().setLeaderName(eNewPlayer, szEmptyString);
	}
	CivilizationTypes eOldCiv = GET_PLAYER(eNewPlayer).getCivilizationType();
	if ( (eOldCiv != NO_CIVILIZATION) && (eOldCiv != eCiv) ) 
	{
		GC.getInitCore().setCivAdjective(eNewPlayer, szEmptyString);
		GC.getInitCore().setCivDescription(eNewPlayer, szEmptyString);
		GC.getInitCore().setCivShortDesc(eNewPlayer, szEmptyString);
	}
	// UNOFFICIAL_PATCH End
	PlayerColorTypes eColor = (PlayerColorTypes)GC.getCivilizationInfo(eCiv).getDefaultPlayerColor();

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       12/30/08                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original bts code
		if (eColor == NO_PLAYERCOLOR || GET_PLAYER((PlayerTypes)iI).getPlayerColor() == eColor)
*/
		// Don't invalidate color choice if it's taken by this player
		if (eColor == NO_PLAYERCOLOR || (GET_PLAYER((PlayerTypes)iI).getPlayerColor() == eColor && (PlayerTypes)iI != eNewPlayer) )
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
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
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						12/30/08            jdog5000    */
/* 																			*/
/* 	Bugfix																	*/
/********************************************************************************/
/* original BTS code
	GET_TEAM(eTeam).init(eTeam);
	GET_PLAYER(eNewPlayer).init(eNewPlayer);
*/
	// Team init now handled when appropriate in player initInGame
	// Standard player init is written for beginning of game, it resets global random events for this player only among other flaws
	GET_PLAYER(eNewPlayer).initInGame(eNewPlayer);
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/
}

/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						8/1/08				jdog5000	*/
/* 																			*/
/* 	Debug																	*/
/********************************************************************************/
void CvGame::changeHumanPlayer( PlayerTypes eNewHuman )
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
		GET_PLAYER(eNewHuman).setOption( (PlayerOptionTypes)iI, GET_PLAYER(eCurHuman).isOption((PlayerOptionTypes)iI) );
	}
	for (int iI = 0; iI < NUM_PLAYEROPTION_TYPES; iI++)
	{
		gDLL->sendPlayerOption(((PlayerOptionTypes)iI), GET_PLAYER(eNewHuman).isOption((PlayerOptionTypes)iI));
	} // </advc.127>
	setActivePlayer(eNewHuman, false);

	GET_PLAYER(eCurHuman).setIsHuman(false);
	GET_PLAYER(eCurHuman).updateHuman(); // advc.127
}
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/

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

	CvPlot* pPlot = GC.getMapINLINE().plot(iX, iY);
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

	CvPlot* pPlot = GC.getMapINLINE().plot(iX, iY);
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
				if (GET_PLAYER((PlayerTypes)iI).isAlive())
				{	// advc.127b:
					std::pair<int,int> xy = getVoteSourceXY(eVoteSource);
					gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, GC.getReligionInfo(eReligion).getSound(), MESSAGE_TYPE_MAJOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
								xy.first, xy.second); // advc.127b
				}
			}
		}
	}
}


// CACHE: cache frequently used values
///////////////////////////////////////


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

bool CvGame::culturalVictoryValid() const // advc.003: const added
{
	if (m_iNumCultureVictoryCities > 0)
	{
		return true;
	}

	return false;
}

int CvGame::culturalVictoryNumCultureCities() const // advc.003: const added
{
	return m_iNumCultureVictoryCities;
}

CultureLevelTypes CvGame::culturalVictoryCultureLevel() const // advc.003: const added
{
	if (m_iNumCultureVictoryCities > 0)
	{
		return (CultureLevelTypes) m_eCultureVictoryCultureLevel;
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
	}
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

							int iLoop;
							for (CvCity* pLoopCity = kPlayer1.firstCity(&iLoop); NULL != pLoopCity; pLoopCity = kPlayer1.nextCity(&iLoop))
							{
								PlayerTypes eNewOwner = pLoopCity->plot()->findHighestCulturePlayer();
								if (NO_PLAYER != eNewOwner
										/* advc.099: No longer implied by
										   findHighestCulturePlayer, and mustn't
										   return cities to dead civs. */
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

		if (0 == pData->aVoteOptions.size())
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
	static CvWString targetCityName;
	int iLoop=-1;
	for (VoteTriggeredData* pVoteTriggered = m_votesTriggered.beginIter(&iLoop); NULL != pVoteTriggered; pVoteTriggered = m_votesTriggered.nextIter(&iLoop))
	{
		CvWString szBuffer;
		CvWString szMessage;
		VoteSelectionSubData subd = pVoteTriggered->kVoteOption; // advc.003
		VoteTypes eVote = subd.eVote;
		VoteSourceTypes eVoteSource = pVoteTriggered->eVoteSource;
		bool bPassed = false;

		if (!canDoResolution(eVoteSource, subd))
		{
			for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
			{
				CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iPlayer);
				if (kPlayer.isVotingMember(eVoteSource))
				{
					szMessage.clear();
					szMessage.Format(L"%s: %s", gDLL->getText("TXT_KEY_ELECTION_CANCELLED").GetCString(), GC.getVoteInfo(eVote).getDescription());
					// advc.127b:
					std::pair<int,int> xy = getVoteSourceXY(eVoteSource);
					gDLL->getInterfaceIFace()->addHumanMessage((PlayerTypes)iPlayer, false, GC.getEVENT_MESSAGE_TIME(), szMessage, "AS2D_NEW_ERA", MESSAGE_TYPE_INFO, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
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
			{
				continue;
			} // <advc.150b>
			targetCityName = "";
			int voteCount = -1; // </advc.150b>
			if (isTeamVote(eVote))
			{
				TeamTypes eTeam = findHighestVoteTeam(*pVoteTriggered);

				if (NO_TEAM != eTeam)
				{	// <advc.150b> Store voteCount for later
					voteCount = countVote(*pVoteTriggered, (PlayerVoteTypes)eTeam);
					bPassed = (voteCount >= getVoteRequired(eVote, eVoteSource));
				}	// </advc.150b>

				szBuffer = GC.getVoteInfo(eVote).getDescription();

				if (eTeam != NO_TEAM)
				{
					szBuffer += NEWLINE + gDLL->getText("TXT_KEY_POPUP_DIPLOMATIC_VOTING_VICTORY", GET_TEAM(eTeam).getName().GetCString(), countVote(*pVoteTriggered, (PlayerVoteTypes)eTeam), getVoteRequired(eVote, eVoteSource), countPossibleVote(eVote, eVoteSource));
				}

				for (int iI = MAX_CIV_TEAMS; iI >= 0; --iI)
				{
					for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
					{
						if (GET_PLAYER((PlayerTypes)iJ).isVotingMember(eVoteSource))
						{
							if (getPlayerVote(((PlayerTypes)iJ), pVoteTriggered->getID()) == (PlayerVoteTypes)iI)
							{
								szBuffer += NEWLINE + gDLL->getText("TXT_KEY_POPUP_VOTES_FOR", GET_PLAYER((PlayerTypes)iJ).getNameKey(), GET_TEAM((TeamTypes)iI).getName().GetCString(), GET_PLAYER((PlayerTypes)iJ).getVotes(eVote, eVoteSource));
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
							szBuffer += NEWLINE + gDLL->getText("TXT_KEY_POPUP_ABSTAINS", GET_PLAYER((PlayerTypes)iJ).getNameKey(), GET_PLAYER((PlayerTypes)iJ).getVotes(eVote, eVoteSource));
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
				if(subd.ePlayer != NO_PLAYER && subd.iCityId >= 0) {
					CvCity* targetCity = GET_PLAYER(subd.ePlayer).getCity(subd.iCityId);
					if(targetCity != NULL)
						targetCityName = targetCity->getNameKey();
				}
				voteCount = countVote(*pVoteTriggered, PLAYER_VOTE_YES);
				bPassed = (voteCount >= getVoteRequired(eVote, eVoteSource));
				// </advc.150b>
				// Defying resolution
				if (bPassed)
				{
					for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
					{
						if (getPlayerVote((PlayerTypes)iJ, pVoteTriggered->getID()) == PLAYER_VOTE_NEVER)
						{
							bPassed = false;

							GET_PLAYER((PlayerTypes)iJ).setDefiedResolution(eVoteSource, subd);
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
								GET_PLAYER((PlayerTypes)iJ).setEndorsedResolution(eVoteSource, subd);
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
				CvWString resolutionStr;
				// Special treatment for resolutions with targets
				if(subd.ePlayer != NO_PLAYER) {
					CvWString key;
					if(kVote.isForcePeace())
						key = L"TXT_KEY_POPUP_ELECTION_FORCE_PEACE";
					else if(kVote.isForceNoTrade())
						key = L"TXT_KEY_POPUP_ELECTION_FORCE_NO_TRADE";
					else if(kVote.isForceWar())
						key = L"TXT_KEY_POPUP_ELECTION_FORCE_WAR";
					if(!key.empty()) {
						resolutionStr = gDLL->getText(key, GET_PLAYER(subd.ePlayer).
								getReplayName(), 0, 0);
					}
					else if(kVote.isAssignCity() && !targetCityName.empty() &&
							subd.eOtherPlayer != NO_PLAYER) {
						resolutionStr = gDLL->getText("TXT_KEY_POPUP_ELECTION_ASSIGN_CITY",
								GET_PLAYER(subd.ePlayer).getCivilizationAdjectiveKey(),
								targetCityName.GetCString(),
								GET_PLAYER(subd.eOtherPlayer).getReplayName(), 0, 0);
					}
				}
				if(resolutionStr.empty()) {
					resolutionStr = kVote.getDescription();
					/*  This is e.g.
						"U.N. Resolution #1284 (Nuclear Non-Proliferation Treaty - Cannot Build Nuclear Weapons)
						Only want "Nuclear Non-Proliferation Treaty". */
					size_t pos1 = resolutionStr.find(L"(");
					if(pos1 != CvWString::npos && pos1 + 1 < resolutionStr.length()) {
						bool bForceCivic = false;
						// Mustn't remove the stuff after the dash if bForceCivic
						for(int i = 0; i < GC.getNumCivicInfos(); i++) {
							if(kVote.isForceCivic(i)) {
								bForceCivic = true;
								break;
							}
						}
						size_t pos2 = std::min((bForceCivic ? CvWString::npos :
								resolutionStr.find(L" -")),
								resolutionStr.find(L")"));
						if(pos2 > pos1)
							resolutionStr = resolutionStr.substr(pos1 + 1, pos2 - pos1 - 1);
					}
				}
				else {
					/*  Throw out stuff in parentheses, e.g.
						"Stop the war against Napoleon (Requires 0 of 0 Total Votes)" */
					resolutionStr = resolutionStr.substr(0, resolutionStr.find(L"(") - 1);
				}
				TeamTypes secGen = getSecretaryGeneral(eVoteSource);
				szMessage = gDLL->getText("TXT_KEY_REPLAY_RESOLUTION_PASSED",
						GC.getVoteSourceInfo(eVoteSource).getTextKeyWide(),
						(secGen == NO_TEAM ?
						gDLL->getText("TXT_KEY_TOPCIVS_UNKNOWN").GetCString() :
						GET_TEAM(secGen).getReplayName().GetCString()),
						voteCount, // Don't show the required votes after all
						//getVoteRequired(eVote, eVoteSource),
						countPossibleVote(eVote, eVoteSource),
						resolutionStr.GetCString());
				addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, NO_PLAYER, szMessage,
						-1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
			} // </advc.150b>
			for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
			{
				CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iI);
				bool bShow = kPlayer.isVotingMember(pVoteTriggered->eVoteSource);
				if (bShow
						&& kPlayer.isHuman()) // advc.127
				{
					CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_TEXT);
					if (NULL != pInfo)
					{
						pInfo->setText(szBuffer);
						gDLL->getInterfaceIFace()->addPopup(pInfo, (PlayerTypes)iI);
					}
				} // <advc.003>
				if(!bShow && iI == subd.ePlayer && GET_PLAYER(subd.ePlayer).
						isVotingMember(pVoteTriggered->eVoteSource))
					bShow = true;
				if(!bShow && iI == subd.eOtherPlayer && GET_PLAYER(subd.eOtherPlayer).
						isVotingMember(pVoteTriggered->eVoteSource))
					bShow = true; // </advc.003>
				if (bPassed && (bShow // <advc.127>
						|| kPlayer.isSpectator()))
				{	
					if(bShow || szMessage.empty() || kVote.isSecretaryGeneral()) {
						// </advc.127>
						szMessage = gDLL->getText("TXT_KEY_VOTE_RESULTS",
								GC.getVoteSourceInfo(eVoteSource).getTextKeyWide(),
								subd.szText.GetCString());
						// Else use the replay msg
					}
					// <advc.127b>
					BuildingTypes vsBuilding = getVoteSourceBuilding(eVoteSource);
					std::pair<int,int> xy = getVoteSourceXY(eVoteSource);
					// </advc.127b>
					gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), szMessage, "AS2D_NEW_ERA",
							// <advc.127> was always MINOR
							kVote.isSecretaryGeneral() ? MESSAGE_TYPE_MINOR_EVENT :
							MESSAGE_TYPE_MAJOR_EVENT, // </advc.127>
							// <advc.127b>
							vsBuilding == NO_BUILDING ? NULL :
							GC.getBuildingInfo(vsBuilding).getButton(),
							// </advc.127b>
							(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
							xy.first, xy.second); // advc.127b
				}
			}
		}

		if (!bPassed && GC.getVoteInfo(eVote).isSecretaryGeneral())
		{
			setSecretaryGeneralTimer(eVoteSource, 0);
		}

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
							{
								kTeam1.meet((TeamTypes)iTeam2, true);
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

	CvGameSpeedInfo& sp = GC.getGameSpeedInfo(getGameSpeedType());
	double speedMultTurns = sp.getGrowthPercent() / 100.0;
	int const iWorldSzPercent = 100;
		// Not sure if map-size adjustment is a good idea
		//=GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getResearchPercent();
	double speedMultFinal = (bSpeedAdjust ?
			sp.getTrainPercent() * iWorldSzPercent / 10000.0 : 1);
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
	double t = gameTurn();
	/*  Function through (startTurn, 1) and (peakTurn, peakMult)
		[^that's assuming speedAdjust=false] */
	double r = speedMultFinal * std::min(peakMult,
			(gradient + std::pow(std::max(0.0, t - startTurn), exponent)) / gradient);
	return r;
} // </advc.314>

// <advc.004m>
bool CvGame::isResourceLayer() const {

	return m_bResourceLayer;
}
void CvGame::reportResourceLayerToggled() {

	m_bResourceLayer = !m_bResourceLayer;
}
// </advc.004m>

// <advc.127b>
std::pair<int,int> CvGame::getVoteSourceXY(VoteSourceTypes vs) const {

	CvCity* vsCity = getVoteSourceCity(vs);
	std::pair<int,int> r = std::make_pair<int,int>(-1,-1);
	if(vsCity == NULL)
		return r;
	r.first = vsCity->getX_INLINE();
	r.second = vsCity->getY_INLINE();
	return r;
}

CvCity* CvGame::getVoteSourceCity(VoteSourceTypes vs) const {

	BuildingTypes vsBuilding = getVoteSourceBuilding(vs);
	if(vsBuilding == NO_BUILDING)
		return NULL;
	for(int i = 0; i < MAX_PLAYERS; i++) { int foo=-1;
		CvPlayer const& pl = GET_PLAYER((PlayerTypes)i);
		if(!pl.isAlive())
			continue;
		for(CvCity* c = pl.firstCity(&foo); c != NULL; c = pl.nextCity(&foo)) {
			if(c->getNumBuilding(vsBuilding) > 0)
				return c;
		}
	}
	return NULL;
}

// <advc.003> Used in several places and I want to make a small change
bool CvGame::isFreeStartEraBuilding(BuildingTypes eBuilding) const {

	CvBuildingInfo const& bi = GC.getBuildingInfo(eBuilding);
	return (bi.getFreeStartEra() != NO_ERA &&
			getStartEra() >= bi.getFreeStartEra() &&
			// <advc.126>
			(bi.getMaxStartEra() == NO_ERA ||
			bi.getMaxStartEra() >= getStartEra())); // </advc.126>
} // </advc.003>


BuildingTypes CvGame::getVoteSourceBuilding(VoteSourceTypes vs) const {

	for(int i = 0; i < GC.getNumBuildingInfos(); i++) {
		BuildingTypes bt = (BuildingTypes)i;
		if(GC.getBuildingInfo(bt).getVoteSourceType() == vs)
			return bt;
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

	return spah;
}

// <advc.703>
RiseFall const& CvGame::getRiseFall() const {

	return riseFall;
}

RiseFall& CvGame::getRiseFall() {

	return riseFall;
} // </advc.703>
