// playerAI.cpp

#include "CvGameCoreDLL.h"
#include "CvPlayerAI.h"
#include "CvRandom.h"
#include "CvGlobals.h"
#include "CvGameCoreUtils.h"
#include "CvMap.h"
#include "CvArea.h"
#include "CvPlot.h"
#include "CvGameAI.h"
#include "CvTeamAI.h"
#include "CvGameCoreUtils.h"
#include "CvDiploParameters.h"
#include "CvInitCore.h"
#include "CyArgsList.h"
#include "CvDLLInterfaceIFaceBase.h"
#include "CvDLLEntityIFaceBase.h"
#include "CvDLLPythonIFaceBase.h"
#include "CvDLLEngineIFaceBase.h"
#include "CvInfos.h"
#include "CvPopupInfo.h"
#include "FProfiler.h"
#include "CvDLLFAStarIFaceBase.h"
#include "FAStarNode.h"
#include "CvEventReporter.h"

#include "BetterBTSAI.h"
#include <iterator> // advc.036
// <k146>
#include <vector>
#include <set>
#include <queue>
// </k146>
//#define GREATER_FOUND_RANGE			(5)
#define CIVIC_CHANGE_DELAY			(20) // was 25
#define RELIGION_CHANGE_DELAY		(15)


// statics

CvPlayerAI* CvPlayerAI::m_aPlayers = NULL;

void CvPlayerAI::initStatics()
{
	m_aPlayers = new CvPlayerAI[MAX_PLAYERS];
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_aPlayers[iI].m_eID = ((PlayerTypes)iI);
	}
}

void CvPlayerAI::freeStatics()
{
	SAFE_DELETE_ARRAY(m_aPlayers);
}

bool CvPlayerAI::areStaticsInitialized()
{
	if(m_aPlayers == NULL)
	{
		return false;
	}

	return true;
}

DllExport CvPlayerAI& CvPlayerAI::getPlayerNonInl(PlayerTypes ePlayer)
{
	return getPlayer(ePlayer);
}

// Public Functions...


/* <advc.109>: AIs who haven't met half their competitors
   (rounded down) are lonely. */
bool CvPlayerAI::isLonely() const {

	// Don't feel lonely right away
	if(getCurrentEra() == GC.getGame().getStartEra())
		return false;
    int yetToMeet = 0, hasMet = 0;
    CvTeam& myTeam = GET_TEAM(getTeam());
    for(int i = 0; i < MAX_CIV_TEAMS; i++) {
        TeamTypes et = (TeamTypes)i; CvTeam& t = GET_TEAM(et);
        if(et == getTeam())
            continue;
        if(myTeam.isHasMet(et))
            hasMet++;
        else if(t.isAlive() && !t.isMinorCiv())
            yetToMeet++;
    }
    return yetToMeet > hasMet + 1;
}
// Akin to AI_getTotalAreaCityThreat, but much coarser
bool CvPlayerAI::feelsSafe() const {

	CvTeamAI const& ourTeam = GET_TEAM(getTeam());
	if(ourTeam.getAnyWarPlanCount(false) > 0)
		return false;
	CvGame& g = GC.getGameINLINE();
	EraTypes gameEra = g.getCurrentEra();
	/*  >=3: Anyone could attack across the sea, and can't fully trust friends
		anymore either as the game progresses */
	if(getCapitalCity() == NULL || gameEra >= 3 ||
			(((gameEra <= 2 && gameEra > 0) ||
			(gameEra > 2 && gameEra == g.getStartEra())) &&
			g.isOption(GAMEOPTION_RAGING_BARBARIANS)) ||
			GET_TEAM(BARBARIAN_TEAM).countNumCitiesByArea(getCapitalCity()->area()) >
			::round(getNumCities() / 3.0) || isThreatFromMinorCiv())
		return false;
	for(int i = 0; i < MAX_CIV_TEAMS; i++) {
		CvTeamAI const& t = GET_TEAM((TeamTypes)i);
		if(!t.isAlive() || !ourTeam.isHasMet(t.getID()) || t.isMinorCiv())
			continue;
		if(t.isHuman())
			return false;
		CvLeaderHeadInfo& lh = GC.getLeaderHeadInfo(GET_PLAYER(t.getLeaderID()).
				getPersonalityType());
		if(lh.getNoWarAttitudeProb(t.AI_getAttitude(getTeam())) < 100 &&
				3 * t.getPower(true) > 2 * ourTeam.getPower(false) &&
				(t.getCurrentEra() >= 3 || // seafaring
				t.AI_isPrimaryArea(getCapitalCity()->area())))
				// Check this instead? Might be a bit slow ...
				// ourTeam.AI_hasCitiesInPrimaryArea(t.getID())
			return false;
	}
	return true;
}

bool CvPlayerAI::isThreatFromMinorCiv() const {

	for(int i = 0; i < MAX_PLAYERS; i++) {
		CvPlayerAI const& p = GET_PLAYER((PlayerTypes)i);
		if(p.isMinorCiv() && p.isAlive() && 2 * p.getPower() > getPower() &&
				(getCapitalCity() == NULL ||
				p.AI_isPrimaryArea(getCapitalCity()->area())))
			return true;
	}
	return false;
} // </advc.109>

// <dlph.16> (Though DarkLunaPhantom didn't put this in a separate function)
int CvPlayerAI::nukeDangerDivisor() const {

	if(GC.getGameINLINE().isNoNukes())
		return 15;
	// "we're going to cheat a little bit, by counting nukes that we probably shouldn't know about."
	bool bDanger = false;
	bool bRemoteDanger = false;
	CvLeaderHeadInfo& ourPers = GC.getLeaderHeadInfo(getPersonalityType());
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayerAI const& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
		// Vassals can't have nukes b/c of change advc.143b
		if(!kLoopPlayer.isAlive() || kLoopPlayer.isAVassal() ||
				!GET_TEAM(getTeam()).isHasMet(kLoopPlayer.getTeam()) ||
				kLoopPlayer.getTeam() == getTeam())
			continue;
		/*  advc: Avoid building shelters against friendly
			nuclear powers. This is mostly role-playing. */
		CvLeaderHeadInfo& theirPers = GC.getLeaderHeadInfo(
				kLoopPlayer.getPersonalityType());
		AttitudeTypes towardThem = AI_getAttitude(kLoopPlayer.getID());
		if(ourPers.getNoWarAttitudeProb(towardThem) >= 100 &&
				(kLoopPlayer.isHuman() ?
				towardThem >= ATTITUDE_FRIENDLY :
				theirPers.getNoWarAttitudeProb(kLoopPlayer.
				AI_getAttitude(getID())) >= 100))
			continue;
		if(kLoopPlayer.getNumNukeUnits() > 0)
			return 1; // Greatest danger, smallest divisor.
		if(kLoopPlayer.getCurrentEra() >= 5)
			bRemoteDanger = true;
	}
	if(bRemoteDanger)
		return 10;
	return 20;
} // </dlph.16>

// <advc.001> Mostly copy-pasted from the homonymous CvTeamAI function
bool CvPlayerAI::AI_hasSharedPrimaryArea(PlayerTypes pId) const {

	FAssert(pId != getID());
	CvPlayerAI& p = GET_PLAYER(pId);
	int i;
	for(CvArea* a = GC.getMapINLINE().firstArea(&i); a != NULL;
			a = GC.getMapINLINE().nextArea(&i)) {
		if (AI_isPrimaryArea(a) && p.AI_isPrimaryArea(a))
			return true;
	}
	return false;
}
// </advc.001>

CvPlayerAI::CvPlayerAI()
{
	m_aiNumTrainAIUnits = new int[NUM_UNITAI_TYPES];
	m_aiNumAIUnits = new int[NUM_UNITAI_TYPES];
	m_aiSameReligionCounter = new int[MAX_PLAYERS];
	m_aiDifferentReligionCounter = new int[MAX_PLAYERS];
	m_aiFavoriteCivicCounter = new int[MAX_PLAYERS];
	m_aiBonusTradeCounter = new int[MAX_PLAYERS];
	m_aiPeacetimeTradeValue = new int[MAX_PLAYERS];
	m_aiPeacetimeGrantValue = new int[MAX_PLAYERS];
	m_aiGoldTradedTo = new int[MAX_PLAYERS];
	m_aiAttitudeExtra = new int[MAX_PLAYERS];

	m_abFirstContact = new bool[MAX_PLAYERS];

	m_aaiContactTimer = new int*[MAX_PLAYERS];
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		m_aaiContactTimer[i] = new int[NUM_CONTACT_TYPES];
	}

	m_aaiMemoryCount = new int*[MAX_PLAYERS];
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		m_aaiMemoryCount[i] = new int[NUM_MEMORY_TYPES];
	}

	m_aiAverageYieldMultiplier = new int[NUM_YIELD_TYPES];
	m_aiAverageCommerceMultiplier = new int[NUM_COMMERCE_TYPES];
	m_aiAverageCommerceExchange = new int[NUM_COMMERCE_TYPES];
	
	m_aiBonusValue = NULL;
	m_aiBonusValueTrade = NULL; // advc.036
	m_aiUnitClassWeights = NULL;
	m_aiUnitCombatWeights = NULL;
	//m_aiCloseBordersAttitudeCache = new int[MAX_PLAYERS];
	m_aiCloseBordersAttitudeCache.resize(MAX_PLAYERS); // K-Mod

	m_aiAttitudeCache.resize(MAX_PLAYERS); // K-Mod

	AI_reset(true);
}


CvPlayerAI::~CvPlayerAI()
{
	AI_uninit();

	SAFE_DELETE_ARRAY(m_aiNumTrainAIUnits);
	SAFE_DELETE_ARRAY(m_aiNumAIUnits);
	SAFE_DELETE_ARRAY(m_aiSameReligionCounter);
	SAFE_DELETE_ARRAY(m_aiDifferentReligionCounter);
	SAFE_DELETE_ARRAY(m_aiFavoriteCivicCounter);
	SAFE_DELETE_ARRAY(m_aiBonusTradeCounter);
	SAFE_DELETE_ARRAY(m_aiPeacetimeTradeValue);
	SAFE_DELETE_ARRAY(m_aiPeacetimeGrantValue);
	SAFE_DELETE_ARRAY(m_aiGoldTradedTo);
	SAFE_DELETE_ARRAY(m_aiAttitudeExtra);
	SAFE_DELETE_ARRAY(m_abFirstContact);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		SAFE_DELETE_ARRAY(m_aaiContactTimer[i]);
	}
	SAFE_DELETE_ARRAY(m_aaiContactTimer);

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		SAFE_DELETE_ARRAY(m_aaiMemoryCount[i]);
	}
	SAFE_DELETE_ARRAY(m_aaiMemoryCount);
	
	SAFE_DELETE_ARRAY(m_aiAverageYieldMultiplier);
	SAFE_DELETE_ARRAY(m_aiAverageCommerceMultiplier);
	SAFE_DELETE_ARRAY(m_aiAverageCommerceExchange);
	//SAFE_DELETE_ARRAY(m_aiCloseBordersAttitudeCache); // disabled by K-Mod
}


void CvPlayerAI::AI_init()
{
	AI_reset(false);

	//--------------------------------
	// Init other game data
	if ((GC.getInitCore().getSlotStatus(getID()) == SS_TAKEN) || (GC.getInitCore().getSlotStatus(getID()) == SS_COMPUTER))
	{
		FAssert(getPersonalityType() != NO_LEADER);
		AI_setPeaceWeight(GC.getLeaderHeadInfo(getPersonalityType()).getBasePeaceWeight() + GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getPeaceWeightRand(), "AI Peace Weight"));
		//AI_setEspionageWeight(GC.getLeaderHeadInfo(getPersonalityType()).getEspionageWeight());
		AI_setEspionageWeight(GC.getLeaderHeadInfo(getPersonalityType()).getEspionageWeight()*GC.getCommerceInfo(COMMERCE_ESPIONAGE).getAIWeightPercent()/100); // K-Mod. (I've changed the meaning of this value)
		//AI_setCivicTimer(((getMaxAnarchyTurns() == 0) ? (GC.getDefineINT("MIN_REVOLUTION_TURNS") * 2) : CIVIC_CHANGE_DELAY) / 2);
		AI_setReligionTimer(1);
		AI_setCivicTimer((getMaxAnarchyTurns() == 0) ? 1 : 2);
		AI_initStrategyRand(); // K-Mod
		updateCacheData(); // K-Mod
		// <advc.104>
		if(isEverAlive() && !isBarbarian() && !isMinorCiv())
			wpai.init(getID()); // </advc.104>
	}
}


void CvPlayerAI::AI_uninit()
{
	SAFE_DELETE_ARRAY(m_aiBonusValue);
	SAFE_DELETE_ARRAY(m_aiBonusValueTrade); // advc.036
	SAFE_DELETE_ARRAY(m_aiUnitClassWeights);
	SAFE_DELETE_ARRAY(m_aiUnitCombatWeights);
}


void CvPlayerAI::AI_reset(bool bConstructor)
{
	int iI;

	AI_uninit();

	m_iPeaceWeight = 0;
	m_iEspionageWeight = 0;
	m_iAttackOddsChange = 0;
	m_iCivicTimer = 0;
	m_iReligionTimer = 0;
	m_iExtraGoldTarget = 0;
	m_iCityTargetTimer = 0; // K-Mod

/************************************************************************************************/
/* CHANGE_PLAYER                         06/08/09                                 jdog5000      */
/*                                                                                              */
/*                                                                                              */
/************************************************************************************************/
	if( bConstructor || getNumUnits() == 0 )
	{
		for (iI = 0; iI < NUM_UNITAI_TYPES; iI++)
		{
			m_aiNumTrainAIUnits[iI] = 0;
			m_aiNumAIUnits[iI] = 0;
		}
	}
/************************************************************************************************/
/* CHANGE_PLAYER                           END                                                  */
/************************************************************************************************/

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_aiSameReligionCounter[iI] = 0;
		m_aiDifferentReligionCounter[iI] = 0;
		m_aiFavoriteCivicCounter[iI] = 0;
		m_aiBonusTradeCounter[iI] = 0;
		m_aiPeacetimeTradeValue[iI] = 0;
		m_aiPeacetimeGrantValue[iI] = 0;
		m_aiGoldTradedTo[iI] = 0;
		m_aiAttitudeExtra[iI] = 0;
		m_abFirstContact[iI] = false;
		for (int iJ = 0; iJ < NUM_CONTACT_TYPES; iJ++)
		{
			m_aaiContactTimer[iI][iJ] = 0;
		}
		for (int iJ = 0; iJ < NUM_MEMORY_TYPES; iJ++)
		{
			m_aaiMemoryCount[iI][iJ] = 0;
		}

		if (!bConstructor && getID() != NO_PLAYER)
		{
			PlayerTypes eLoopPlayer = (PlayerTypes) iI;
			CvPlayerAI& kLoopPlayer = GET_PLAYER(eLoopPlayer);
			kLoopPlayer.m_aiSameReligionCounter[getID()] = 0;
			kLoopPlayer.m_aiDifferentReligionCounter[getID()] = 0;
			kLoopPlayer.m_aiFavoriteCivicCounter[getID()] = 0;
			kLoopPlayer.m_aiBonusTradeCounter[getID()] = 0;
			kLoopPlayer.m_aiPeacetimeTradeValue[getID()] = 0;
			kLoopPlayer.m_aiPeacetimeGrantValue[getID()] = 0;
			kLoopPlayer.m_aiGoldTradedTo[getID()] = 0;
			kLoopPlayer.m_aiAttitudeExtra[getID()] = 0;
			kLoopPlayer.m_abFirstContact[getID()] = false;
			for (int iJ = 0; iJ < NUM_CONTACT_TYPES; iJ++)
			{
				kLoopPlayer.m_aaiContactTimer[getID()][iJ] = 0;
			}
			for (int iJ = 0; iJ < NUM_MEMORY_TYPES; iJ++)
			{
				kLoopPlayer.m_aaiMemoryCount[getID()][iJ] = 0;
			}
		}
	}
	
	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		m_aiAverageYieldMultiplier[iI] = 0;
	}
	for (iI = 0; iI< NUM_COMMERCE_TYPES; iI++)
	{
		m_aiAverageCommerceMultiplier[iI] = 0;
		m_aiAverageCommerceExchange[iI] = 0;
	}
	m_iAverageGreatPeopleMultiplier = 0;
	m_iAverageCulturePressure = 0;
	//m_iAveragesCacheTurn = -1;
	
	m_iStrategyHash = 0;
	//m_iStrategyHashCacheTurn = -1;

// BBAI
	m_iStrategyRand = UINT_MAX; // was 0 (K-Mod)
	m_iVictoryStrategyHash = 0;
	//m_iVictoryStrategyHashCacheTurn = -1;
// BBAI end

	m_bWasFinancialTrouble = false;
	m_iTurnLastProductionDirty = -1;

	//m_iUpgradeUnitsCacheTurn = -1;
	//m_iUpgradeUnitsCachedExpThreshold = 0;
	m_iUpgradeUnitsCachedGold = 0;
	m_iAvailableIncome = 0; // K-Mod

	m_aiAICitySites.clear();
	
	FAssert(m_aiBonusValue == NULL);
	m_aiBonusValue = new int[GC.getNumBonusInfos()];
	m_aiBonusValueTrade = new int[GC.getNumBonusInfos()]; // advc.036
	for (iI = 0; iI < GC.getNumBonusInfos(); iI++)
	{
		m_aiBonusValue[iI] = -1;
		m_aiBonusValueTrade[iI] = -1; // advc.036
	}
	
	FAssert(m_aiUnitClassWeights == NULL);
	m_aiUnitClassWeights = new int[GC.getNumUnitClassInfos()];
	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		m_aiUnitClassWeights[iI] = 0;		
	}

	FAssert(m_aiUnitCombatWeights == NULL);
	m_aiUnitCombatWeights = new int[GC.getNumUnitCombatInfos()];
	for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
	{
		m_aiUnitCombatWeights[iI] = 0;		
	}

	/* original bts code
	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_aiCloseBordersAttitudeCache[iI] = 0;

		if (!bConstructor && getID() != NO_PLAYER)
		{
			GET_PLAYER((PlayerTypes) iI).m_aiCloseBordersAttitudeCache[getID()] = 0;
		}
	} */
	// K-Mod
	m_aiCloseBordersAttitudeCache.assign(MAX_PLAYERS, 0);
	m_aiAttitudeCache.assign(MAX_PLAYERS, 0);
	// K-Mod end
	dangerFromSubs = false; // advc.651
}


int CvPlayerAI::AI_getFlavorValue(FlavorTypes eFlavor) const
{
	if (isHuman())
		return 0; // K-Mod
	/* <advc.109> Lonely civs need to focus on science so that they can
	   meet partners */
	int r = GC.getLeaderHeadInfo(getPersonalityType()).getFlavorValue(eFlavor);
	if(isLonely()) {
		if(eFlavor == FLAVOR_SCIENCE) {
			r = std::max(r, 1);
			r *= 2;
		}
		else r /= 2;
	}
	return r; // </advc.109>
	//return GC.getLeaderHeadInfo(getPersonalityType()).getFlavorValue(eFlavor);
}

// K-Mod
void CvPlayerAI::updateCacheData()
{
	if (isAlive())
	{
		// AI_updateCloseBorderAttitudeCache();
		// AI_updateAttitudeCache(); // attitude of this player is relevant to other players too, so this needs to be done elsewhere.
		AI_updateNeededExplorers(); // advc.003b
		AI_calculateAverages();
		AI_updateVictoryStrategyHash();
		//if (!isHuman()) // advc.104: Human strategies are interesting to know
						  //           for UWAI.
		{
			AI_updateStrategyHash();
			//AI_updateGoldToUpgradeAllUnits();
		}
		// note. total upgrade gold is currently used in AI_hurry, which is used by production-automated.
		// Therefore, we need to get total upgrade gold for human players as well as AI players.
		AI_updateGoldToUpgradeAllUnits();
		AI_updateAvailableIncome(); // K-Mod
		checkDangerFromSubmarines(); // advc.651
		// <advc.139>
		if(isBarbarian())
			return;
		std::vector<double> cityValues; int i;
		for(CvCity* c = firstCity(&i); c != NULL; c = nextCity(&i))
			cityValues.push_back(c->AI_cityValue());
		// i appears to be unreliable, skips numbers
		int j = 0;
		for(CvCity* c = firstCity(&i); c != NULL; c = nextCity(&i), j++)
			c->updateEvacuating(1 - ::percentileRank(cityValues, cityValues[j]));
		// </advc.139>
	}
}
// K-Mod end

void CvPlayerAI::AI_doTurnPre()
{
	PROFILE_FUNC();

	FAssertMsg(getPersonalityType() != NO_LEADER, "getPersonalityType() is not expected to be equal with NO_LEADER");
	FAssertMsg(getLeaderType() != NO_LEADER, "getLeaderType() is not expected to be equal with NO_LEADER");
	FAssertMsg(getCivilizationType() != NO_CIVILIZATION, "getCivilizationType() is not expected to be equal with NO_CIVILIZATION");
	CvGame& g = GC.getGameINLINE(); // advc.003
	// <advc.104>
	if(getID() == 0 && g.getElapsedGameTurns() <= 0 && GC.getInitCore().isScenario())
		g.initScenario(); // </advc.104>
	//AI_invalidateCloseBordersAttitudeCache();

	AI_doCounter();

	// K-Mod note: attitude cache is not refreshed here because there are still some attitude-affecting changes to come, in CvTeamAI::AI_doCounter
	//   for efficiency, I've put AI_updateCloseBorderAttitudeCache in CvTeam::doTurn rather than here.

	AI_updateBonusValue();
	// K-Mod. Update commerce weight before calculating great person weight
	AI_updateCommerceWeights(); // (perhaps this should be done again after AI_doCommerce, or when sliders change?)
	// GP weights can take a little bit of time, so lets only do it once every 3 turns.
	if ((g.getGameTurn() + AI_getStrategyRand(0))%3 == 0)
		AI_updateGreatPersonWeights();
	// K-Mod end
	
	AI_doEnemyUnitData();

	// K-Mod: clear construction value cache
	AI_ClearConstructionValueCache();
	
	if (isHuman())
	{
		return;
	}

	AI_doResearch();

	AI_doCommerce();

	AI_doMilitary();

	AI_doCivics();

	AI_doReligion();

	AI_doCheckFinancialTrouble();
}


void CvPlayerAI::AI_doTurnPost()
{
	PROFILE_FUNC();

	if (isHuman())
	{
		return;
	}

	if (isBarbarian())
	{
		return;
	}

	if (isMinorCiv())
	{
		return;
	}

	AI_doDiplo();
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       06/16/09                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
	// Moved per alexman's suggestion
	//AI_doSplit();	
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/

	for (int i = 0; i < GC.getNumVictoryInfos(); ++i)
	{
		AI_launch((VictoryTypes)i);
	}
}


void CvPlayerAI::AI_doTurnUnitsPre()
{
	PROFILE_FUNC();

	AI_updateFoundValues();

	if (AI_getCityTargetTimer() == 0 && GC.getGameINLINE().getSorenRandNum(8, "AI Update Area Targets") == 0) // K-Mod added timer check.
	{
		AI_updateAreaTargets();
	} // <advc.102>
	int dummy;
	for(CvUnit* u = firstUnit(&dummy); u != NULL; u = nextUnit(&dummy)) {
		u->setInitiallyVisible(u->plot()->isVisibleToWatchingHuman());
	} // </advc.102>
	if (isHuman())
	{
		return;
	}

	if (isBarbarian())
	{
		return;
	}
	
	// Uncommented by K-Mod, and added warplan condition.
	if (AI_isDoStrategy(AI_STRATEGY_CRUSH) && isFocusWar()) // advc.105
			//GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0)
	{
		AI_convertUnitAITypesForCrush();		
	}
}


void CvPlayerAI::AI_doTurnUnitsPost()
{
	PROFILE_FUNC();

	CvUnit* pLoopUnit;
	CvPlot* pUnitPlot;
	bool bValid;
	int iPass;
	int iLoop;

	if (!isHuman() || isOption(PLAYEROPTION_AUTO_PROMOTION))
	{
		for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
		{
			pLoopUnit->AI_promote();
		}
	}

	if (isHuman()
		/*  advc.300: Disbanding now handled in CvGame::killBarb.
			No more barb upgrades. */
		|| isBarbarian()
		)
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/24/10                                jdog5000      */
/*                                                                                              */
/* Gold AI                                                                                      */
/************************************************************************************************/
	//bool bAnyWar = (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0);
	int iStartingGold = getGold();
	/* BBAI code
	int iTargetGold = AI_goldTarget();
	int iUpgradeBudget = (AI_getGoldToUpgradeAllUnits() / (bAnyWar ? 1 : 2));
	iUpgradeBudget = std::min(iUpgradeBudget, iStartingGold - ((iTargetGold > iUpgradeBudget) ? (iTargetGold - iUpgradeBudget) : iStartingGold/2)); */
	// K-Mod. Note: AI_getGoldToUpgradeAllUnits() is actually one of the components of AI_goldTarget()
	int iCostPerMil = AI_unitCostPerMil(); // used for scrap decisions.
	int iUpgradeBudget = 0;
	if(isFocusWar()) // advc.105
	//if(GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0)
	{
		// when at war, upgrades get priority
		iUpgradeBudget = std::min(getGold(), AI_goldTarget(true));
	}
	else
	{
		int iMaxBudget = AI_goldTarget(true);
		iUpgradeBudget = std::min(iMaxBudget, getGold() * iMaxBudget / std::max(1, AI_goldTarget(false)));
	}
	// K-Mod end

	// Always willing to upgrade 1 unit if we have the money
	iUpgradeBudget = std::max(iUpgradeBudget,1);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	CvPlot* pLastUpgradePlot = NULL;
	for (iPass = 0; iPass < 4; iPass++)
	{
		for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
		{
			bool bNoDisband = false;
			bValid = false;

			switch (iPass)
			{
			case 0:
				// BBAI note:  Effectively only for galleys, triremes, and ironclads ... unit types which are limited in
				// what terrain they can operate in
				if (AI_unitImpassableCount(pLoopUnit->getUnitType()) > 0)
				{
					bValid = true;
				}
				break;
			case 1:
				pUnitPlot = pLoopUnit->plot();
				if (pUnitPlot->isCity())
				{
					if (pUnitPlot->getBestDefender(getID()) == pLoopUnit)
					{
						bNoDisband = true;
						bValid = true;
						pLastUpgradePlot = pUnitPlot;
					}
					// <advc.650>
					if(GC.getUnitInfo(pLoopUnit->getUnitType()).getNukeRange() >= 0)
						bValid = false; // </advc.650>
					// try to upgrade units which are in danger... but don't get obsessed
					if (!bValid && pLastUpgradePlot != pUnitPlot && AI_getAnyPlotDanger(pUnitPlot, 1, false))
					{
						bNoDisband = true;
						bValid = true;
						pLastUpgradePlot = pUnitPlot;
					}
				}
				break;
			case 2:
				/* original BTS code
				if (pLoopUnit->cargoSpace() > 0)
				{
					bValid = true;
				} */

				// Only normal transports
				if (pLoopUnit->cargoSpace() > 0 && pLoopUnit->specialCargo() == NO_SPECIALUNIT)
				{
					bValid = iStartingGold - getGold() < iUpgradeBudget;
				}
				// Also upgrade escort ships
				if (pLoopUnit->AI_getUnitAIType() == UNITAI_ESCORT_SEA)
				{
					bValid = iStartingGold - getGold() < iUpgradeBudget;
				}
				
				break;
			case 3:
				/* original BTS code
				bValid = true; */
				bValid = iStartingGold - getGold() < iUpgradeBudget;
				break;
			default:
				FAssert(false);
				break;
			}

			if (bValid)
			{
				bool bKilled = false;
				if (!bNoDisband)
				{
					//if (pLoopUnit->canFight()) // original bts code
					if (pLoopUnit->getUnitCombatType() != NO_UNITCOMBAT) // K-Mod - bug fix for the rare case of a barb city spawning on top of an animal
					{
						int iExp = pLoopUnit->getExperience();
						CvCity* pPlotCity = pLoopUnit->plot()->getPlotCity();
						if (pPlotCity != NULL && pPlotCity->getOwnerINLINE() == getID())
						{
							int iCityExp = 0;
							iCityExp += pPlotCity->getFreeExperience();
							iCityExp += pPlotCity->getDomainFreeExperience(pLoopUnit->getDomainType());
							iCityExp += pPlotCity->getUnitCombatFreeExperience(pLoopUnit->getUnitCombatType());
							if (iCityExp > 0)
							{
								/*if ((iExp == 0) || (iExp < (iCityExp + 1) / 2))
								{
									/* original bts code
									if ((pLoopUnit->getDomainType() != DOMAIN_LAND) || pLoopUnit->plot()->plotCount(PUF_isMilitaryHappiness, -1, -1, getID()) > 1)
									{
										if ((calculateUnitCost() > 0) && (AI_getPlotDanger( pLoopUnit->plot(), 2, false) == 0))
										{										
											pLoopUnit->kill(false);
											bKilled = true;
											pLastUpgradePlot = NULL;
										}
									}
								} */
								// K-Mod
								if (iExp < iCityExp && GC.getGameINLINE().getGameTurn() - pLoopUnit->getGameTurnCreated() > 8)
								{
									int iDefenders = pLoopUnit->plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getID(), NO_TEAM, PUF_isCityAIType);
									if (iDefenders > pPlotCity->AI_minDefenders() && !AI_getAnyPlotDanger(pLoopUnit->plot(), 2, false))
									{
										if (iCostPerMil > AI_maxUnitCostPerMil(pLoopUnit->area(), 100) + 2*iExp + 4*std::max(0, pPlotCity->AI_neededDefenders() - iDefenders))
										{
											if (gUnitLogLevel > 2)
												logBBAI("    %S scraps %S, with %d exp, and %d / %d spending.", getCivilizationDescription(0), pLoopUnit->getName(0).GetCString(), iExp, iCostPerMil, AI_maxUnitCostPerMil(pLoopUnit->area(), 100));
											pLoopUnit->scrap();
											pLoopUnit->doDelayedDeath(); // I could have just done kill(), but who knows what extra work scrap() wants to do for us?
											bKilled = true;
											pLastUpgradePlot = NULL;
											iCostPerMil = AI_unitCostPerMil(); // recalculate
										}
									}
								}
								// K-Mod end
							}
						}
					}
				}
				if (!bKilled)
				{
					pLoopUnit->AI_upgrade(); // CAN DELETE UNIT!!!
				}
			}
		}
	}

	// advc.300: Moved up.
	/*if (isBarbarian())
	{
		return;
	}*/

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/24/10                                jdog5000      */
/*                                                                                              */
/* AI Logging                                                                                   */
/************************************************************************************************/
	if( gPlayerLogLevel > 2 )
	{
		if( iStartingGold - getGold() > 0 )
		{
			logBBAI("    %S spends %d on unit upgrades out of budget of %d, %d gold remaining", getCivilizationDescription(0), iStartingGold - getGold(), iUpgradeBudget, getGold());
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	// UF bugfix: AI_doSplit moved here per alexman's suggestion
	AI_doSplit();
	// K-Mod note: the reason for moving it here is that player turn ordering can get messed up if a
	// new player is created, recycling an old player number, while no one else's turn is 'active'.
	// I can't help but think there is a more natural solution to this problem...
}


void CvPlayerAI::AI_doPeace()
{
	PROFILE_FUNC();

	CLinkList<TradeData> ourList;
	CLinkList<TradeData> theirList;
	bool abContacted[MAX_TEAMS];
	TradeData item;
	int iOurValue;
	int iTheirValue;
	int iI;

	FAssert(!isHuman());
	FAssert(!isMinorCiv());
	FAssert(!isBarbarian());

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		abContacted[iI] = false;
	}

	/*  <advc.003> Moved checks into new function willOfferPeace for advc.134a,
		then realized I don't need that function after all. Doesn't hurt though,
		and I'm using it for an assertion in CvPlayer::setTurnActive now. */
	for(iI = 0; iI < MAX_CIV_PLAYERS; iI++) {
		PlayerTypes civId = (PlayerTypes)iI;
		CvPlayerAI const& civ = GET_PLAYER(civId);
		if(AI_getContactTimer(civId, CONTACT_PEACE_TREATY) != 0)
			continue;
		if(!willOfferPeace(civId))
			continue; // </advc.003>
		bool bOffered = false;
		setTradeItem(&item, TRADE_SURRENDER);

		if (canTradeItem(civId, item, true))
		{
			ourList.clear();
			theirList.clear();

			ourList.insertAtEnd(item);

			bOffered = true;

			if (civ.isHuman())
			{
				/*  <advc.134a> Can't get this diplo popup through the EXE,
					so better not to send it at all. */
				bOffered = false;
				//FAssertMsg(false, "AI sends surrender popup, but the EXE won't show it");
				// Commented out: </advc.134a>
				/*if (!(abContacted[TEAMID(civId)]))
				{
					AI_changeContactTimer(civId, CONTACT_PEACE_TREATY,
							GC.getLeaderHeadInfo(getPersonalityType()).
							getContactDelay(CONTACT_PEACE_TREATY));
					pDiplo = new CvDiploParameters(getID());
					FAssert(pDiplo != NULL);
					pDiplo->setDiploComment((DiploCommentTypes)
							GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_PEACE"));
					pDiplo->setAIContact(true);
					pDiplo->setOurOfferList(theirList);
					pDiplo->setTheirOfferList(ourList);
					gDLL->beginDiplomacy(pDiplo, civId);
					abContacted[TEAMID(civId)] = true;
				}*/
			}
			else
			{
				GC.getGameINLINE().implementDeal(getID(), civId, &ourList, &theirList);
			}
		}

		if(bOffered) return; // advc.003
		// advc.003 (comment): Non-capitulation peace offered with a 10 or 5% probability
		if(GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(
				getPersonalityType()).getContactRand(CONTACT_PEACE_TREATY)
				/ 2, // advc.134a: Make it 10-20% b/c the other conditions are so narrow
				"AI Diplo Peace Treaty") != 0)
			return;
		setTradeItem(&item, TRADE_PEACE_TREATY);
		if(!canTradeItem(civId, item, true) || !civ.canTradeItem(getID(), item, true))
			return; // advc.003
		iOurValue =  GET_TEAM(getTeam()).AI_endWarVal(TEAMID(civId));
		iTheirValue = TEAMREF(civId).AI_endWarVal(getTeam());
		// advc.134a: Human discount
		if(civ.isHuman()) {
			// advc.003: Moved up
			if(abContacted[TEAMID(civId)]) return;
			iOurValue = ::round(1.2 * iOurValue);
		}
		// advc.104h:
		abContacted[TEAMID(civId)] = negotiatePeace(civId, iTheirValue, iOurValue);
	}
}

/*  <advc.104h> Cut and pasted (and heavily refactored) from doPeace b/c UWAI needs
	this subroutine. While decisions of war and peace are made by teams, an
	individual member of the team has to pay for peace (or receive payment);
	therefore not a CvTeamAI function. */
bool CvPlayerAI::negotiatePeace(PlayerTypes civId, int theirBenefit, int ourBenefit) {

	CvPlayerAI& civ = GET_PLAYER(civId);
	/*  I've called the parameters "Benefit" to make clear who receives which part,
		but in the body, I'm using iTheirValue for the stuff they receive and
		iOurValue for what we receive, as in the original BtS code. */
	int iTheirValue = theirBenefit;
	int iOurValue = ourBenefit;
	TechTypes eBestReceiveTech = NO_TECH;
	TechTypes eBestGiveTech = NO_TECH;
	int iReceiveGold = 0;
	int iGiveGold = 0;
	CvCity* pBestReceiveCity = NULL;
	CvCity* pBestGiveCity = NULL;
	
	if(iTheirValue > iOurValue) // They need to give us sth.
		iOurValue += negotiatePeace(getID(), civId, iTheirValue - iOurValue,
				&iReceiveGold, &eBestReceiveTech, &pBestReceiveCity);
	else if(iOurValue > iTheirValue) // We need to give them sth.
		iTheirValue += negotiatePeace(civId, getID(), iOurValue - iTheirValue,
				&iGiveGold, &eBestGiveTech, &pBestGiveCity);
	// <advc.134a> The bugfix doesn't work if the AI pays the human
	if(civ.isHuman() && (eBestGiveTech != NO_TECH || iGiveGold != 0
			|| pBestGiveCity != NULL))
		return false; // </advc.134a>
	/*  K-Mod: "ratio of endWar values has to be somewhat evened out by reparations
		for peace to happen" */
	if(iTheirValue < iOurValue*3/5 ||
			(civ.isHuman() && iOurValue < iTheirValue*4/5) ||
			(!civ.isHuman() && iOurValue < iTheirValue*3/5))
		return false;
	CLinkList<TradeData> ourList;
	CLinkList<TradeData> theirList;
	TradeData item;
	setTradeItem(&item, TRADE_PEACE_TREATY);
	ourList.insertAtEnd(item);
	theirList.insertAtEnd(item);
	if(eBestGiveTech != NO_TECH) {
		setTradeItem(&item, TRADE_TECHNOLOGIES, eBestGiveTech);
		ourList.insertAtEnd(item);
	}
	if(eBestReceiveTech != NO_TECH) {
		setTradeItem(&item, TRADE_TECHNOLOGIES, eBestReceiveTech);
		theirList.insertAtEnd(item);
	}
	if(iGiveGold != 0) {
		setTradeItem(&item, TRADE_GOLD, iGiveGold);
		ourList.insertAtEnd(item);
	}
	if(iReceiveGold != 0) {
		setTradeItem(&item, TRADE_GOLD, iReceiveGold);
		theirList.insertAtEnd(item);
	}
	if(pBestGiveCity != NULL) {
		setTradeItem(&item, TRADE_CITIES, pBestGiveCity->getID());
		ourList.insertAtEnd(item);
	}
	if(pBestReceiveCity != NULL) {
		setTradeItem(&item, TRADE_CITIES, pBestReceiveCity->getID());
		theirList.insertAtEnd(item);
	}
	if(civ.isHuman()) {
		AI_changeContactTimer(civId, CONTACT_PEACE_TREATY,
				GC.getLeaderHeadInfo(getPersonalityType()).
				getContactDelay(CONTACT_PEACE_TREATY));
		CvDiploParameters* pDiplo = new CvDiploParameters(getID());
		FAssert(pDiplo != NULL);
		pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_PEACE"));
		pDiplo->setAIContact(true);
		pDiplo->setOurOfferList(theirList);
		pDiplo->setTheirOfferList(ourList);
		gDLL->beginDiplomacy(pDiplo, civId);
//advc.test, advc.134a:
FAssertMsg(getWPAI.isEnabled(), "AI sent peace offer; if it doesn't appear, it could be b/c of a change in circumstances, but more likely advc.134a isn't working as intended");
/*  Note: The diplo msg gets through w/o problems if the OfferLists are left empty.
	The diplo comment will still say to "end the bloodshed", and if the player
	accepts, a peace treaty is signed.
	But it's no use if the AI can't offer reparations. */
	}
	else GC.getGameINLINE().implementDeal(getID(), civId, &ourList, &theirList);
	return true;
}

// Auxiliary function to reduce the amount of duplicate or dual code
int CvPlayerAI::negotiatePeace(PlayerTypes receiverId, PlayerTypes giverId, int delta,
		int* iGold, TechTypes* eBestTech, CvCity** pBestCity) {

	int r = 0;
	CvPlayerAI& receiver = GET_PLAYER(receiverId);
	CvPlayerAI& giver = GET_PLAYER(giverId);
	TradeData item;
	// <advc.134a> Let human pay gold if peace vals are similar.
	if(giver.isHuman() && delta <= giver.AI_maxGoldTrade(receiverId) &&
			delta < 100 * giver.getCurrentEra()) {
		setTradeItem(&item, TRADE_GOLD, delta);
		if(giver.canTradeItem(receiverId, item, true)) {
			*iGold = delta;
			return delta;
		}
	} // </advc.134a>
	int iBestValue = INT_MIN;
	for(int j = 0; j < GC.getNumTechInfos(); j++) {
		setTradeItem(&item, TRADE_TECHNOLOGIES, j);
		if(giver.canTradeItem(receiverId, item, true)) {
			/*  advc.104h: Pick the best fit instead of a random tech.
				Random tech has the advantage that any tech is tried eventually;
				disadvantage: can take many attempts at peace negotiation until
				a deal is struck.
				Make the choice more and more random the longer the war lasts
				by adding a random number between 0 and war duration to the
				power of 1.65. */
			int iValue = -std::abs(GET_TEAM(receiver.getTeam()).AI_techTradeVal(
				(TechTypes)j, giver.getTeam(), true, true) - delta) +
				GC.getGameINLINE().getSorenRandNum(::round(
					std::pow((double)GET_TEAM(receiver.getTeam()).
					AI_getAtWarCounter(giver.getTeam()), 1.65)),
					"AI Peace Trading");
			// BtS code:
			//iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Peace Trading (Tech #1)"));
			if(iValue > iBestValue) {
				iBestValue = iValue;
				*eBestTech = (TechTypes)j;
			}
		}
	}
	if(*eBestTech != NO_TECH) {
		// advc.104h: techTradeVal reduced by passing peaceDeal=true
		r += GET_TEAM(receiver.getTeam()).AI_techTradeVal(*eBestTech,
				giver.getTeam(), true, true);
	}

	// K-Mod note: ideally we'd use AI_goldTradeValuePercent()
	// to work out the gold's value gained by the receiver and the gold's value lost by the giver.
	// Instead, we just pretend that the receiver gets 1 point per gold, and the giver loses nothing.
	// (effectly a value of 2)
	int tradeGold = std::min(delta - r, giver.AI_maxGoldTrade(receiverId));
	if(tradeGold > 0) {
		setTradeItem(&item, TRADE_GOLD, tradeGold);
		if(giver.canTradeItem(receiverId, item, true)) {
			*iGold = tradeGold;
			r += tradeGold;
		}
	}
	// advc.104h: Don't add a city to the trade if it's already almost square
	if(delta - r > 0.2 * delta) {
		int iBestValue = 0; int iLoop;
		for(CvCity* pLoopCity = giver.firstCity(&iLoop); pLoopCity != NULL;
				pLoopCity = giver.nextCity(&iLoop)) {
			setTradeItem(&item, TRADE_CITIES, pLoopCity->getID());
			if(giver.canTradeItem(receiverId, item, true)
					/*  advc.104h: BtS checks this (only if we're the ones giving
						away the city). Apply the check regardless of who owns the
						city, but make it less strict. */
					//&& (receiverId == getID() || receiver.AI_cityTradeVal(pLoopCity) <= delta - r)
					&& receiver.AI_cityTradeVal(pLoopCity) <= delta - r + 0.2 * delta
					) {
				int iValue = pLoopCity->plot()->calculateCulturePercent(receiverId);
				if(iValue > iBestValue) {
					iBestValue = iValue;
					*pBestCity = pLoopCity;
				}
			}
		}
		if(*pBestCity != NULL)
			r += receiver.AI_cityTradeVal(*pBestCity);
	}
	return r;
} // </advc.104h>

// <advc.003>
bool CvPlayerAI::willOfferPeace(PlayerTypes toId) const {

	CvPlayer const& to = GET_PLAYER(toId);
	CvTeamAI const& ourTeam = GET_TEAM(getTeam());
	if(!to.isAlive() || toId == getID() || !ourTeam.isAtWar(TEAMID(toId)) ||
			// Don't contact if ...
			!canContactAndTalk(toId) || // either side refuses to talk
			ourTeam.isHuman() || // a human on our team is doing the talking
			// a human on their team
			(!to.isHuman() && (TEAMREF(toId)).isHuman()) ||
			// they're human and we're not the team leader
			(to.isHuman() && ourTeam.getLeaderID() != getID()) ||
			// war less than 10 turns old.
			ourTeam.AI_getAtWarCounter(TEAMID(toId)) <= 10)
		return false;
	return true;
} // </advc.003>


void CvPlayerAI::AI_updateFoundValues(bool bStartingLoc)
{
	PROFILE_FUNC();

	// bool bCitySiteCalculations = (GC.getGame().getGameTurn() > GC.getGame().getStartTurn()); // disabled by K-Mod (unused)
	
	int iLoop;
	for (CvArea* pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		pLoopArea->setBestFoundValue(getID(), 0);
	}

	if (bStartingLoc)
	{
		for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
		{
			GC.getMapINLINE().plotByIndexINLINE(iI)->setFoundValue(getID(), -1);
		}
	}
	else
	{
		// K-Mod. (I've also changed some of the code style all through this function)
		CvFoundSettings kFoundSet(*this, false);
		//

		if (!isBarbarian())
		{
			//AI_invalidateCitySites(AI_getMinFoundValue());
			AI_invalidateCitySites(-1); // K-Mod
		}
		for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
		{
			CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

			if (pLoopPlot->isRevealed(getTeam(), false))// || AI_isPrimaryArea(pLoopPlot->area()))
			{
				long iValue = -1;
				if (GC.getUSE_GET_CITY_FOUND_VALUE_CALLBACK())
				{
					CyArgsList argsList;
					argsList.add((int)getID());
					argsList.add(pLoopPlot->getX());
					argsList.add(pLoopPlot->getY());
					gDLL->getPythonIFace()->callFunction(PYGameModule, "getCityFoundValue", argsList.makeFunctionArgs(), &iValue);
				}

				if (iValue == -1)
				{
					//iValue = AI_foundValue(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
					iValue = AI_foundValue_bulk(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), kFoundSet); // K-Mod
				}

				FAssert(iValue <= MAX_SHORT); // K-Mod. If this assert fails, the foundValue calculation may need to be changed.
				iValue = std::min((long)MAX_SHORT, iValue); // K-Mod
				pLoopPlot->setFoundValue(getID(), (short)iValue);

				if (iValue > pLoopPlot->area()->getBestFoundValue(getID()))
				{
					pLoopPlot->area()->setBestFoundValue(getID(), iValue);
				}
			}
			// K-Mod. Clear out any junk found values.
			// (I've seen legacy AI code which makes use of the found values of unrevealed plots.
			//  It shouldn't use those values at all, but if it does use them, I'd prefer them not to be junk!)
			else
			{
				pLoopPlot->setFoundValue(getID(), 0);
			}
			// K-Mod end
		}
		if (!isBarbarian())
		{
			//int iMaxCityCount = 4;
			int iMaxCityCount = isHuman() ? 6 : 4; // K-Mod - because humans don't always walk towards the AI's top picks..
			AI_updateCitySites(AI_getMinFoundValue(), iMaxCityCount);
		}
	}
}


void CvPlayerAI::AI_updateAreaTargets()
{
	CvArea* pLoopArea;
	int iLoop;
	bool bResetTimer = AI_getCityTargetTimer() > 4; // K-Mod. (reset the timer if it is above the minimum time)

	for(pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		if (!(pLoopArea->isWater()))
		{
			if (GC.getGameINLINE().getSorenRandNum(3, "AI Target City") == 0)
			{
				pLoopArea->setTargetCity(getID(), NULL);
			}
			else
			{
				pLoopArea->setTargetCity(getID(), AI_findTargetCity(pLoopArea));
			}
			bResetTimer = bResetTimer || pLoopArea->getTargetCity(getID()); // K-Mod
		}
	}
	// K-Mod. (guarantee a short amount of time before randomly updating again)
	if (bResetTimer)
		AI_setCityTargetTimer(4);
	// K-Mod end
}


// Returns priority for unit movement (lower values move first...)
// This function has been heavily edited for K-Mod
int CvPlayerAI::AI_movementPriority(CvSelectionGroup* pGroup) const
{
	// If the group is trying to do a stack attack, let it go first!
	// (this is required for amphibious assults; during which a low priority group can be ordered to attack before its turn.)
	if (pGroup->AI_isGroupAttack())
		return -1;

	const CvUnit* pHeadUnit = pGroup->getHeadUnit();

	if (pHeadUnit == NULL)
		return INT_MAX;

	if( pHeadUnit->isSpy() )
	{
		return 0;
	}

	if (pHeadUnit->getDomainType() != DOMAIN_LAND)
	{
		if (pHeadUnit->bombardRate() > 0)
			return 1;

		if (pHeadUnit->hasCargo())
		{
			if (pHeadUnit->specialCargo() != NO_SPECIALUNIT)
				return 2;
			else
				return 3;
		}

		if (pHeadUnit->getDomainType() == DOMAIN_AIR)
		{
			// give recon units top priority. Unfortunately, these are not easy to identify because there is no air_recon AI type.
			if (pHeadUnit->airBombBaseRate() == 0 && !pHeadUnit->canAirDefend())
				return 0;

			// non recon units (including nukes!)
			if (pHeadUnit->canAirDefend() || pHeadUnit->evasionProbability() > 10)
				return 4;
			else
				return 5;
		}

		if (pHeadUnit->canFight())
		{
			if (pHeadUnit->collateralDamage() > 0)
				return 6;
			else
				return 7;
		}
		else
			return 8;
	}

	FAssert(pHeadUnit->getDomainType() == DOMAIN_LAND);

	if (pHeadUnit->AI_getUnitAIType() == UNITAI_WORKER)
		return 9;

	if (pHeadUnit->AI_getUnitAIType() == UNITAI_EXPLORE)
		return 10;

	if (pHeadUnit->bombardRate() > 0)
		return 11;

	if (pHeadUnit->collateralDamage() > 0)
		return 12;

	if (pGroup->isStranded())
		return 505;

	if (pHeadUnit->canFight())
	{
		int iPriority = 65; // allow + or - 50

		iPriority += (GC.getGameINLINE().getBestLandUnitCombat()*100 - pHeadUnit->currCombatStr(NULL, NULL) + 10) / 20; // note: currCombatStr has a factor of 100 built in.

		if (pGroup->getNumUnits() > 1)
		{
			iPriority--;
			if (pGroup->getNumUnits() > 4)
				iPriority--;
		}
		else if (pGroup->AI_getMissionAIType() == MISSIONAI_GUARD_CITY)
			iPriority++;

		if (pHeadUnit->withdrawalProbability() > 0 || pHeadUnit->noDefensiveBonus())
		{
			iPriority--;
			if (pHeadUnit->withdrawalProbability() > 20)
				iPriority--;
		}

		switch (pHeadUnit->AI_getUnitAIType())
		{
		case UNITAI_ATTACK_CITY:
		case UNITAI_ATTACK:
			iPriority--;
		case UNITAI_COUNTER:
			iPriority--;
		case UNITAI_PILLAGE:
		case UNITAI_RESERVE:
			iPriority--;
		case UNITAI_CITY_DEFENSE:
		case UNITAI_CITY_COUNTER:
		case UNITAI_CITY_SPECIAL:
			break;

		case UNITAI_COLLATERAL:
			FAssertMsg(false, "AI_movementPriority: unit AI type doesn't seem to match traits.");
			break;

		default:
			break;
		}
		return iPriority;
	}
	/* old code
		if (pHeadUnit->canFight())
		{
			if (pHeadUnit->withdrawalProbability() > 20)
			{
				return 9;
			}

			if (pHeadUnit->withdrawalProbability() > 0)
			{
				return 10;
			}

			iCurrCombat = pHeadUnit->currCombatStr(NULL, NULL);
			iBestCombat = (GC.getGameINLINE().getBestLandUnitCombat() * 100);

			if (pHeadUnit->noDefensiveBonus())
			{
				iCurrCombat *= 3;
				iCurrCombat /= 2;
			}

			if (pHeadUnit->AI_isCityAIType())
			{
				iCurrCombat /= 2;
			}

			if (iCurrCombat > iBestCombat)
			{
				return 11;
			}
			else if (iCurrCombat > ((iBestCombat * 4) / 5))
			{
				return 12;
			}
			else if (iCurrCombat > ((iBestCombat * 3) / 5))
			{
				return 13;
			}
			else if (iCurrCombat > ((iBestCombat * 2) / 5))
			{
				return 14;
			}
			else if (iCurrCombat > ((iBestCombat * 1) / 5))
			{
				return 15;
			}
			else
			{
				return 16;
			}
		}
	*/

	return 200;
}


void CvPlayerAI::AI_unitUpdate()
{
	PROFILE_FUNC();

	CLLNode<int>* pCurrUnitNode;

	FAssert(m_groupCycle.getLength() == m_selectionGroups.getCount());

	if (!hasBusyUnit())
	{
		pCurrUnitNode = headGroupCycleNode();

		while (pCurrUnitNode != NULL)
		{
			CvSelectionGroup* pLoopSelectionGroup = getSelectionGroup(pCurrUnitNode->m_data);
			pCurrUnitNode = nextGroupCycleNode(pCurrUnitNode);

			if (pLoopSelectionGroup->AI_isForceSeparate())
			{
				// do not split groups that are in the midst of attacking
				if (pLoopSelectionGroup->isForceUpdate() || !pLoopSelectionGroup->AI_isGroupAttack())
				{
					pLoopSelectionGroup->AI_separate();	// pointers could become invalid...
				}
			}
		}

		/* original bts code
		if (isHuman())
		{
			pCurrUnitNode = headGroupCycleNode();

			while (pCurrUnitNode != NULL)
			{
				CvSelectionGroup* pLoopSelectionGroup = getSelectionGroup(pCurrUnitNode->m_data);
				pCurrUnitNode = nextGroupCycleNode(pCurrUnitNode);

				if (pLoopSelectionGroup->AI_update())
				{
					break; // pointers could become invalid...
				}
			}
		}
		else */
		if (!isHuman()) // K-Mod - automated actions are no longer done from this function. Instead, they are done in CvGame::updateMoves
		{
			/* original bts code
			tempGroupCycle.clear();
			finalGroupCycle.clear();

			pCurrUnitNode = headGroupCycleNode();

			while (pCurrUnitNode != NULL)
			{
				tempGroupCycle.insertAtEnd(pCurrUnitNode->m_data);
				pCurrUnitNode = nextGroupCycleNode(pCurrUnitNode);
			}

			iValue = 0;

			while (tempGroupCycle.getLength() > 0)
			{
				pCurrUnitNode = tempGroupCycle.head();

				while (pCurrUnitNode != NULL)
				{
					pLoopSelectionGroup = getSelectionGroup(pCurrUnitNode->m_data);
					FAssertMsg(pLoopSelectionGroup != NULL, "selection group node with NULL selection group");

					if (AI_movementPriority(pLoopSelectionGroup) <= iValue)
					{
						finalGroupCycle.insertAtEnd(pCurrUnitNode->m_data);
						pCurrUnitNode = tempGroupCycle.deleteNode(pCurrUnitNode);
					}
					else
					{
						pCurrUnitNode = tempGroupCycle.next(pCurrUnitNode);
					}
				}

				iValue++;
			}

			pCurrUnitNode = finalGroupCycle.head();

			while (pCurrUnitNode != NULL)
			{
				pLoopSelectionGroup = getSelectionGroup(pCurrUnitNode->m_data);

				if (NULL != pLoopSelectionGroup)  // group might have been killed by a previous group update
				{
					if (pLoopSelectionGroup->AI_update())
					{
						break; // pointers could become invalid...
					}
				}

				pCurrUnitNode = finalGroupCycle.next(pCurrUnitNode);
			} */
			// K-Mod. It seems to me that all we're trying to do is run AI_update
			// on the highest priority groups until one of them becomes busy.
			// ... if only they had used the STL, this would be a lot easier.
			bool bRepeat = true;
			do
			{
				std::vector<std::pair<int, int> > groupList;

				pCurrUnitNode = headGroupCycleNode();
				while (pCurrUnitNode != NULL)
				{
					CvSelectionGroup* pLoopSelectionGroup = getSelectionGroup(pCurrUnitNode->m_data);
					FAssert(pLoopSelectionGroup != NULL);

					int iPriority = AI_movementPriority(pLoopSelectionGroup);
					groupList.push_back(std::make_pair(iPriority, pCurrUnitNode->m_data));

					pCurrUnitNode = nextGroupCycleNode(pCurrUnitNode);
				}
				FAssert(groupList.size() == getNumSelectionGroups());

				std::sort(groupList.begin(), groupList.end());
				for (size_t i = 0; i < groupList.size(); i++)
				{
					CvSelectionGroup* pLoopSelectionGroup = getSelectionGroup(groupList[i].second);
					// I think it's probably a good idea to activate automissions here, so that the move priority is respected even for commands issued on the previous turn.
					// (This will allow reserve units to guard workers that have already moved, rather than trying to guard workers who are about to move to a different plot.)
					if (pLoopSelectionGroup && !pLoopSelectionGroup->autoMission())
					{
						if (pLoopSelectionGroup->isBusy() || pLoopSelectionGroup->isCargoBusy() || pLoopSelectionGroup->AI_update())
						{
							bRepeat = false;
							break;
						}
					}
				}

				// one last trick that might save us a bit of time...
				// if the number of selection groups has increased, then lets try to take care of the new groups right away.
				// (there might be a faster way to look for the new groups, but I don't know it.)
				bRepeat = bRepeat && m_groupCycle.getLength() > (int)groupList.size();
				// the repeat will do a stack of redundant checks,
				// but I still expect it to be much faster than waiting for the next turnslice.
				// Note: I use m_groupCycle rather than getNumSelectionGroups just in case there is a bug which causes the two to be out of sync.
				// (otherwise, if getNumSelectionGroups is bigger, it could cause an infinite loop.)
			} while (bRepeat);
			// K-Mod end
		}
	}
}


void CvPlayerAI::AI_makeAssignWorkDirty()
{
	CvCity* pLoopCity;
	int iLoop;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->AI_setAssignWorkDirty(true);
	}
}


void CvPlayerAI::AI_assignWorkingPlots()
{
	CvCity* pLoopCity;
	int iLoop;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->AI_assignWorkingPlots();
	}
}


void CvPlayerAI::AI_updateAssignWork()
{
	CvCity* pLoopCity;
	int iLoop;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->AI_updateAssignWork();
	}
}

void CvPlayerAI::AI_doCentralizedProduction()
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	int iLoop;
	int iI;

	if( isHuman() )
	{
		return;
	}

	if( isBarbarian() )
	{
		return;
	}

	// Choose which cities should build floating defenders
	// evaluate military cities score and rank.
	// choose military cities that aren't building anything, or wait for good military cities to finish building.
	/*
	bool bCrushStrategy = kPlayer.AI_isDoStrategy(AI_STRATEGY_CRUSH);
	int iNeededFloatingDefenders = (isBarbarian() || bCrushStrategy) ?  0 : kPlayer.AI_getTotalFloatingDefendersNeeded(pArea);
 	int iTotalFloatingDefenders = (isBarbarian() ? 0 : kPlayer.AI_getTotalFloatingDefenders(pArea));
	
	UnitTypeWeightArray floatingDefenderTypes;
	floatingDefenderTypes.push_back(std::make_pair(UNITAI_CITY_DEFENSE, 125));
	floatingDefenderTypes.push_back(std::make_pair(UNITAI_CITY_COUNTER, 100));
	//floatingDefenderTypes.push_back(std::make_pair(UNITAI_CITY_SPECIAL, 0));
	floatingDefenderTypes.push_back(std::make_pair(UNITAI_RESERVE, 100));
	floatingDefenderTypes.push_back(std::make_pair(UNITAI_COLLATERAL, 100));
	
	if (iTotalFloatingDefenders < ((iNeededFloatingDefenders + 1) / (bGetBetterUnits ? 3 : 2)))
	if (!bExempt && AI_chooseLeastRepresentedUnit(floatingDefenderTypes))
	{
		if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose floating defender 1", getName().GetCString());
		return;
	}
	*/

	// Cycle through all buildings looking for limited buildings (wonders)
	// Identify the cities that will benefit the most. If the best city is not busy, build the wonder.
	/*
	for (iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		if (!(GET_PLAYER(getOwnerINLINE()).isBuildingClassMaxedOut(((BuildingClassTypes)iI), GC.getBuildingClassInfo((BuildingClassTypes)iI).getExtraPlayerInstances())))
		{
			BuildingTypes eLoopBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI);
			bool bWorld = isWorldWonderClass((BuildingClassTypes)iI);
			bool bNational = isNationalWonderClass((BuildingClassTypes)iI);

						if (canConstruct(eLoopBuilding))
						{
							// city loop.
							iValue = AI_buildingValueThreshold(eLoopBuilding);

							if (GC.getBuildingInfo(eLoopBuilding).getFreeBuildingClass() != NO_BUILDINGCLASS)
							{
								BuildingTypes eFreeBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(GC.getBuildingInfo(eLoopBuilding).getFreeBuildingClass());
								if (NO_BUILDING != eFreeBuilding)
								{
									iValue += (AI_buildingValue(eFreeBuilding, iFocusFlags) * (GET_PLAYER(getOwnerINLINE()).getNumCities() - GET_PLAYER(getOwnerINLINE()).getBuildingClassCountPlusMaking((BuildingClassTypes)GC.getBuildingInfo(eLoopBuilding).getFreeBuildingClass())));
								}
							}

							if (iValue > 0)
							{
								iTurnsLeft = getProductionTurnsLeft(eLoopBuilding, 0);

								if (isWorldWonderClass((BuildingClassTypes)iI))
								{
									if (iProductionRank <= std::min(3, ((GET_PLAYER(getOwnerINLINE()).getNumCities() + 2) / 3)))
									{
										if (bAsync)
										{
											iTempValue = GC.getASyncRand().get(GC.getLeaderHeadInfo(getPersonalityType()).getWonderConstructRand(), "Wonder Construction Rand ASYNC");
										}
										else
										{
											iTempValue = GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getWonderConstructRand(), "Wonder Construction Rand");
										}
										
										if (bAreaAlone)
										{
											iTempValue *= 2;
										}
										iValue += iTempValue;
									}
								}

								if (bAsync)
								{
									iValue *= (GC.getASyncRand().get(25, "AI Best Building ASYNC") + 100);
									iValue /= 100;
								}
								else
								{
									iValue *= (GC.getGameINLINE().getSorenRandNum(25, "AI Best Building") + 100);
									iValue /= 100;
								}

								iValue += getBuildingProduction(eLoopBuilding);
								
								
								bool bValid = ((iMaxTurns <= 0) ? true : false);
								if (!bValid)
								{
									bValid = (iTurnsLeft <= GC.getGameINLINE().AI_turnsPercent(iMaxTurns, GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getConstructPercent()));
								}
								if (!bValid)
								{
									for (int iHurry = 0; iHurry < GC.getNumHurryInfos(); ++iHurry)
									{
										if (canHurryBuilding((HurryTypes)iHurry, eLoopBuilding, true))
										{
											if (AI_getHappyFromHurry((HurryTypes)iHurry, eLoopBuilding, true) > 0)
											{
												bValid = true;
												break;
											}
										}
									}
								}

								if (bValid)
								{
									FAssert((MAX_INT / 1000) > iValue);
									iValue *= 1000;
									iValue /= std::max(1, (iTurnsLeft + 3));

									iValue = std::max(1, iValue);

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										eBestBuilding = eLoopBuilding;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return eBestBuilding;
	*/


// K-Mod end, the rest is some unfinished code from BBAI

	// BBAI TODO: Temp testing
	//if( getID() % 2 == 1 )
	//{
		return;
	//}
	
	// Determine number of cities player can use building wonders currently
	int iMaxNumWonderCities = 1 + getNumCities()/5;
	bool bIndustrious = (getMaxPlayerBuildingProductionModifier() > 0);
	bool bAtWar = (GET_TEAM(getTeam()).getAtWarCount(true) > 0);

	if( bIndustrious )
	{
		iMaxNumWonderCities += 1;
	}

	// Dagger?
	// Power?
	// Research?

	if( bAtWar )
	{
		int iWarSuccessRating = GET_TEAM(getTeam()).AI_getWarSuccessRating();
		if( iWarSuccessRating < -90 )
		{
			iMaxNumWonderCities = 0;
		}
		else 
		{
			if( iWarSuccessRating < 30 )
			{
				iMaxNumWonderCities -= 1;
			}
			if( iWarSuccessRating < -50 )
			{
				iMaxNumWonderCities /= 2;
			}
		}
	}

	if( isMinorCiv() && (GET_TEAM(getTeam()).getHasMetCivCount(false) > 1) )
	{
		iMaxNumWonderCities /= 2;
	}

	iMaxNumWonderCities = std::min(iMaxNumWonderCities, getNumCities());

	// Gather city statistics
	// Could rank cities based on gold, production here, could be O(n) instead of O(n^2)
	int iWorldWonderCities = 0;
	int iLimitedWonderCities = 0;
	int iNumDangerCities = 0;
	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if( pLoopCity->isProductionBuilding() )
		{
			if( isLimitedWonderClass((BuildingClassTypes)(GC.getBuildingInfo(pLoopCity->getProductionBuilding()).getBuildingClassType())))
			{
				iLimitedWonderCities++;

				if (isWorldWonderClass((BuildingClassTypes)(GC.getBuildingInfo(pLoopCity->getProductionBuilding()).getBuildingClassType())))
				{
					iWorldWonderCities++;
				}
			}
		}

		if( pLoopCity->isProductionProject() )
		{
			if( isLimitedProject(pLoopCity->getProductionProject()))
			{
				iLimitedWonderCities++;
				if( isWorldProject(pLoopCity->getProductionProject()))
				{
					iWorldWonderCities++;
				}
			}
		}
	}

	// Check for any global wonders to build
	for (iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		if (isWorldWonderClass((BuildingClassTypes)iI))
		{

			//canConstruct(
		}
	}

	// Check for any projects to build
	for (iI = 0; iI < GC.getNumProjectInfos(); iI++)
	{
		
	}

	// Check for any national/team wonders to build
	
}

void CvPlayerAI::AI_makeProductionDirty()
{
	CvCity* pLoopCity;
	int iLoop;

	FAssertMsg(!isHuman(), "isHuman did not return false as expected");

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->AI_setChooseProductionDirty(true);
	}
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/16/10                              jdog5000        */
/*                                                                                              */
/* War tactics AI                                                                               */
/************************************************************************************************/
void CvPlayerAI::AI_conquerCity(CvCity* pCity)
{
	bool bRaze = false;
	bool cultureVict = false; // advc.116
	if (canRaze(pCity))
	{
	    int iRazeValue = 0;
		int iCloseness = pCity->AI_playerCloseness(getID());

		// Reasons to always raze
		if( 2*pCity->getCulture(pCity->getPreviousOwner()) > pCity->getCultureThreshold(GC.getGameINLINE().culturalVictoryCultureLevel()) )
		{
			CvCity* pLoopCity;
			int iLoop;
			int iHighCultureCount = 1;

			for( pLoopCity = GET_PLAYER(pCity->getPreviousOwner()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(pCity->getPreviousOwner()).nextCity(&iLoop) )
			{
				if( 2*pLoopCity->getCulture(pCity->getPreviousOwner()) > pLoopCity->getCultureThreshold(GC.getGameINLINE().culturalVictoryCultureLevel()) )
				{
					iHighCultureCount++;
			// <advc.116> Count them all before deciding what to do
				}
			}
			int victTarget = GC.getGameINLINE().culturalVictoryNumCultureCities();
			// Razing won't help if they have many high-culture cities
			if(iHighCultureCount == victTarget || iHighCultureCount == victTarget + 1) {
				cultureVict = true;
				CvTeam const& prevTeam = TEAMREF(pCity->getPreviousOwner());
				// Don't raze if they're unlikely to reconquer it
				double powRatio = prevTeam.getPower(true) /
						(0.1 + GET_TEAM(getTeam()).getPower(false));
				if(powRatio > 0.80)
					bRaze = true;
				if(!bRaze) {
					int attStr = AI_localAttackStrength(pCity->plot(),
							prevTeam.getID(), DOMAIN_LAND, 4);
					int defStr = AI_localDefenceStrength(pCity->plot(), getTeam(),
							DOMAIN_LAND, 3);
					if(5 * attStr > 4 * defStr)
						bRaze = true;
					if(bRaze)
						logBBAI( "  Razing enemy cultural victory city" );
					}
				} // </advc.116>
			}
		// <advc.122>
		if(!isBarbarian() && !pCity->isHolyCity() && !pCity->isEverOwned(getID()) &&
				!pCity->hasActiveWorldWonder() && pCity->isAwfulSite(getID()))
			bRaze = true; // </advc.122>
		if( !bRaze )
		{
			// Reasons to not raze
			if(!cultureVict) { // advc.116
				if( (getNumCities() <= 1) || (getNumCities() < 5 && iCloseness > 0) )
				{
					if( gPlayerLogLevel >= 1 )
					{
						logBBAI("    Player %d (%S) decides not to raze %S because they have few cities", getID(), getCivilizationDescription(0), pCity->getName().GetCString() );
					}
				}
				else if( AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION3) && GET_TEAM(getTeam()).AI_isPrimaryArea(pCity->area()) )
				{
					// Do not raze, going for domination
					if( gPlayerLogLevel >= 1 )
					{
						logBBAI("    Player %d (%S) decides not to raze %S because they're going for domination", getID(), getCivilizationDescription(0), pCity->getName().GetCString() );
					}
				} // <advc.116>
			}
			//else // </advc.116>
			if( isBarbarian() )
			{
				if ( !(pCity->isHolyCity()) && !(pCity->hasActiveWorldWonder()))
				{
					if( (pCity->getPreviousOwner() != BARBARIAN_PLAYER) && (pCity->getOriginalOwner() != BARBARIAN_PLAYER) )
					{
						iRazeValue += GC.getLeaderHeadInfo(getPersonalityType()).getRazeCityProb();
						// <advc.300> Commented out
						//iRazeValue -= iCloseness;
						int diffEraPop = 1 + std::max(3, (int)
								GET_PLAYER(pCity->getOwnerINLINE()).
								getCurrentEra()) - pCity->getPopulation();
						iRazeValue *= diffEraPop;
						// The BtS raze roll; now used exclusively for barbs
						if(GC.getGameINLINE().getSorenRandNum(100, "advc.300") <
								iRazeValue)
							bRaze = true; // </advc.300>
					}
				}
			}
			else
			{
				bool bFinancialTrouble = AI_isFinancialTrouble();
				bool bBarbCity = (pCity->getPreviousOwner() == BARBARIAN_PLAYER) && (pCity->getOriginalOwner() == BARBARIAN_PLAYER);
				bool bPrevOwnerBarb = (pCity->getPreviousOwner() == BARBARIAN_PLAYER);
				bool bTotalWar = GET_TEAM(getTeam()).AI_getWarPlan(GET_PLAYER(pCity->getPreviousOwner()).getTeam()) == WARPLAN_TOTAL; // K-Mod
				
				/* original code
				if (GET_TEAM(getTeam()).countNumCitiesByArea(pCity->area()) == 0)
				{
					// Conquered city in new continent/island
					int iBestValue;

					if( pCity->area()->getNumCities() == 1 && AI_getNumAreaCitySites(pCity->area()->getID(), iBestValue) == 0 )
					{
						// Probably small island
						if( iCloseness == 0 )
						{
							// Safe to raze these now that AI can do pick up ...
							iRazeValue += GC.getLeaderHeadInfo(getPersonalityType()).getRazeCityProb();
						}
					}
					else
					{
						// At least medium sized island
						if( iCloseness < 10 )
						{
							if( bFinancialTrouble )
							{
								// Raze if we might start incuring colony maintenance
								iRazeValue = 100;
							}
							else
							{
								if (pCity->getPreviousOwner() != NO_PLAYER && !bPrevOwnerBarb)
								{
									if (GET_TEAM(GET_PLAYER(pCity->getPreviousOwner()).getTeam()).countNumCitiesByArea(pCity->area()) > 3)
									{
										iRazeValue += GC.getLeaderHeadInfo(getPersonalityType()).getRazeCityProb();
									}
								}
							}
						}
					}
				}
				else */ // K-Mod disabled. I don't see the point treating the above as a special case.
				{
					// <advc.116> Why raze a small island?
					// K-Mod (moved this from above)
					/*int iUnused;
					if (pCity->area()->getNumCities() == 1 && AI_getNumAreaCitySites(pCity->area()->getID(), iUnused) == 0)
					{
						// Probably small island
						if (iCloseness == 0)
							iRazeValue += 20;
					} */
					// K-Mod end //</advc.116>

					// Distance related aspects
					if (iCloseness > 0)
					{
						iRazeValue -= iCloseness;
					}
					else
					{
						iRazeValue += 15; // advc.116: was 40
						// <cdtw.1>
						CvTeam const& ourTeam = GET_TEAM(getTeam());
						if(ourTeam.isAVassal()) {
							iRazeValue -= 5;
							if(ourTeam.isCapitulated())
								iRazeValue -= 10;
						} // </cdtw.1>
						CvCity* pNearestTeamAreaCity = GC.getMapINLINE().findCity(pCity->getX_INLINE(), pCity->getY_INLINE(), NO_PLAYER, getTeam(), true, false, NO_TEAM, NO_DIRECTION, pCity);

						/*if( pNearestTeamAreaCity == NULL )
						{
							// Shouldn't happen
							iRazeValue += 30;
						} */
						// K-Mod
						if (pNearestTeamAreaCity == NULL
								/*  <advc.300> The +15 is bad enough if we don't have to worry about
									attacks against the city */
									&& pCity->getPreviousOwner() != BARBARIAN_PLAYER
									&& pCity->area()->getNumCities() > 1)
									// </advc.300>
						{
							if (bTotalWar && GET_TEAM(GET_PLAYER(pCity->getPreviousOwner()).getTeam()).AI_isPrimaryArea(pCity->area()))
								iRazeValue += 5;
							else
								iRazeValue += 30;
						}
						// K-Mod end
						else if(pNearestTeamAreaCity != NULL) // advc.300
						{
							int iDistance = plotDistance(pCity->getX_INLINE(), pCity->getY_INLINE(), pNearestTeamAreaCity->getX_INLINE(), pNearestTeamAreaCity->getY_INLINE());
							iDistance -= DEFAULT_PLAYER_CLOSENESS + 2;
							// <advc.116> World size adjustment, upper cap
							iDistance *= 60;
							iDistance /= GC.getMapINLINE().maxPlotDistance();
							iDistance = std::min(20, iDistance); // </advc.116>
							if ( iDistance > 0 )
							{
								iRazeValue += iDistance * (bBarbCity ? 3 : 2); // advc.116: Was 8 : 5
							}
						}
					}

					// <advc.116>
					if(bFinancialTrouble) {
						iRazeValue += (pCity->calculateDistanceMaintenanceTimes100(getID()) +
								pCity->calculateDistanceMaintenanceTimes100(getID())) /
								100; // Replacing:
								//std::max(0, (70 - 15 * pCity->getPopulation()));
					}
					iRazeValue -= 3 * pCity->getPopulation();
					// </advc.116>

					// Scale down distance/maintenance effects for organized. (disabled by K-Mod)
					/*
					if( iRazeValue > 0 )
					{
						for (iI = 0; iI < GC.getNumTraitInfos(); iI++)
						{
							if (hasTrait((TraitTypes)iI))
							{
								iRazeValue *= (100 - (GC.getTraitInfo((TraitTypes)iI).getUpkeepModifier()));
								iRazeValue /= 100;

								if( (GC.getTraitInfo((TraitTypes)iI).getUpkeepModifier() > 0) && gPlayerLogLevel >= 1 )
								{
									logBBAI("      Reduction for upkeep modifier %d", (GC.getTraitInfo((TraitTypes)iI).getUpkeepModifier()) );
								}
							}
						}
					} */

					// Non-distance related aspects
					iRazeValue += GC.getLeaderHeadInfo(getPersonalityType()).getRazeCityProb()
							/ 5; // advc.116

					// K-Mod
					iRazeValue -= AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION2) ? 20 : 0;
					// K-Mod end
		                
					if (getStateReligion() != NO_RELIGION)
					{
						if (pCity->isHasReligion(getStateReligion()))
						{
							if (GET_TEAM(getTeam()).hasShrine(getStateReligion()))
							{
								iRazeValue -= 50;

								if( gPlayerLogLevel >= 1 )
								{
									logBBAI("      Reduction for state religion with shrine" );
								}
							}
							else
							{
								iRazeValue -= 10;

								if( gPlayerLogLevel >= 1 )
								{
									logBBAI("      Reduction for state religion" );
								}
							}
						}
					}
					// K-Mod
					else
					{
						iRazeValue -= 5; // Free religion does not mean we hate everyone equally...
					}
					// K-Mod end
				}


				for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
				{
					if (pCity->isHolyCity((ReligionTypes)iI))
					{
						logBBAI("      Reduction for holy city" );
						if( getStateReligion() == iI )
						{
							iRazeValue -= 150;
						}
						else
						{
							iRazeValue -= 5 + GC.getGameINLINE().calculateReligionPercent((ReligionTypes)iI);
						}
					}
				}

				// K-Mod
				// corp HQ value.
				for (int iI = 0; iI < GC.getNumCorporationInfos(); iI++)
				{
					if (pCity->isHeadquarters((CorporationTypes)iI))
					{
						logBBAI("      Reduction for corp headquarters");
						iRazeValue -= 10 + 100 * GC.getGameINLINE().countCorporationLevels((CorporationTypes)iI) / GC.getGameINLINE().getNumCities();
					}
				}
				// great people
				iRazeValue -= 5 * pCity->getNumGreatPeople(); // advc.116: was 2 * ...
				iRazeValue += bBarbCity ? 5 : 0;
				// K-Mod end

				iRazeValue -= 15 * pCity->getNumActiveWorldWonders();

				CvPlot* pLoopPlot = NULL;
				for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
				{
					pLoopPlot = plotCity(pCity->getX_INLINE(), pCity->getY_INLINE(), iI);

					if (pLoopPlot != NULL)
					{
						if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
						{
							iRazeValue -= std::max(2, AI_bonusVal(pLoopPlot->getBonusType(getTeam()), 1, true)/2);
						}
					}
				}

				// <advc.116>
				PlayerTypes culturalOwnerId = pCity->findHighestCulture();
				if(culturalOwnerId != NO_PLAYER) {
					CvPlayer& culturalOwner = GET_PLAYER(culturalOwnerId);
					if(!culturalOwner.isBarbarian() && !atWar(getTeam(),
							culturalOwner.getTeam())) {
						if(culturalOwnerId == getID())
							iRazeValue -= 15;
						else {
							// Avoid penalty to relations (and may want to liberate)
							int multiplier = std::max(0, AI_getAttitude(culturalOwnerId)
									- ATTITUDE_ANNOYED);
							// 10 for Cautious, 20 for Pleased, 30 for Friendly
							iRazeValue -= 10 * multiplier;
						}
					}
				}
				// Don't raze remote cities conquered in early game
				int targetCities = GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).
						getTargetNumCities();
				if(targetCities * 0.75 >= (double)getNumCities())
					iRazeValue -= (targetCities - getNumCities()) * 5;
				// </advc.116>

				// More inclined to raze if we're unlikely to hold it
				if( GET_TEAM(getTeam()).getPower(false)*10 < GET_TEAM(GET_PLAYER(pCity->getPreviousOwner()).getTeam()).getPower(true)*8 )
				{
					int iTempValue = 20;
					iTempValue *= (GET_TEAM(GET_PLAYER(pCity->getPreviousOwner()).getTeam()).getPower(true) - GET_TEAM(getTeam()).getPower(false));
					iTempValue /= std::max( 100, GET_TEAM(getTeam()).getPower(false) );

					logBBAI("      Low power, so boost raze odds by %d", std::min( 75, iTempValue ) );

					iRazeValue += std::min( 75, iTempValue );
				}

				if( gPlayerLogLevel >= 1 )
				{
					if( bBarbCity ) logBBAI("      %S is a barb city", pCity->getName().GetCString() );
					if( bPrevOwnerBarb ) logBBAI("      %S was last owned by barbs", pCity->getName().GetCString() );
					logBBAI("      %S has area cities %d, closeness %d, bFinTrouble %d", pCity->getName().GetCString(), GET_TEAM(getTeam()).countNumCitiesByArea(pCity->area()), iCloseness, bFinancialTrouble );
				}

				// advc.116: Replacing the dice roll below. That's hardly any randomness.
				iRazeValue += GC.getGameINLINE().getSorenRandNum(6, "");
			}

			if( gPlayerLogLevel >= 1 )
			{
				//logBBAI("    Player %d (%S) has odds %d to raze city %S", getID(), getCivilizationDescription(0), iRazeValue, pCity->getName().GetCString() );
				// advc.116: Replacing the above
				if(!bRaze) logBBAI("    Player %d (%S) has raze value %d for city %S", getID(), getCivilizationDescription(0), iRazeValue, pCity->getName().GetCString() );
			}
						
			if (iRazeValue > 0)
			{
				// advc.116: Commented out
				//if (GC.getGameINLINE().getSorenRandNum(100, "AI Raze City") < iRazeValue)
				//{
					bRaze = true;
				//}
			}
		}
	}

	if( bRaze )
	{
		logBBAI("    Player %d (%S) decides to to raze city %S!!!", getID(), getCivilizationDescription(0), pCity->getName().GetCString() );
		pCity->doTask(TASK_RAZE);
		//logBBAI("    Player %d (%S) decides to to raze city %S!!!", getID(), getCivilizationDescription(0), pCity->getName().GetCString() );
		// K-Mod moved the log message up - otherwise it will crash due to pCity being deleted!
	}
	else
	{
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       06/14/09                       Maniac & jdog5000      */
/*                                                                                              */
/*                                                                                              */
/************************************************************************************************/
/* original bts code
		CvEventReporter::getInstance().cityAcquiredAndKept(GC.getGameINLINE().getActivePlayer(), pCity);
*/
		CvEventReporter::getInstance().cityAcquiredAndKept(getID(), pCity);
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
	}
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
// <advc.130q> About 7 or 8 is high (important city), below 1 is low
double CvPlayerAI::razeAngerRating(CvCity const& c) const {

	if(c.getCultureLevel() == NO_CULTURELEVEL)
		return 0;
	double relativeTileCulture = c.plot()->calculateCulturePercent(getID()) / 100.0;
	return relativeTileCulture * c.getCultureLevel() * c.getPopulation() /
			std::sqrt((double)getTotalPopulation());
} // </advc.130q>

bool CvPlayerAI::AI_acceptUnit(CvUnit* pUnit) const
{
	if (isHuman())
	{
		return true;
	}

	if (AI_isFinancialTrouble())
	{
		if (pUnit->AI_getUnitAIType() == UNITAI_WORKER)
		{
			if (AI_neededWorkers(pUnit->area()) > 0)
			{
				return true;
			}
		}

		if (pUnit->AI_getUnitAIType() == UNITAI_WORKER_SEA)
		{
			return true;
		}

		if (pUnit->AI_getUnitAIType() == UNITAI_MISSIONARY)
		{
			return true; //XXX
		}

		// K-Mod
		switch (pUnit->AI_getUnitAIType())
		{
		case UNITAI_PROPHET:
		case UNITAI_ARTIST:
		case UNITAI_SCIENTIST:
		case UNITAI_GENERAL:
		case UNITAI_MERCHANT:
		case UNITAI_ENGINEER:
		case UNITAI_GREAT_SPY:
			return true;
		default:
			break;
		}
		// K-Mod end
		return false;
	}

	return true;
}


bool CvPlayerAI::AI_captureUnit(UnitTypes eUnit, CvPlot* pPlot) const
{
	CvCity* pNearestCity;

	FAssert(!isHuman());

	if (pPlot->getTeam() == getTeam())
	{
		return true;
	}

	pNearestCity = GC.getMapINLINE().findCity(pPlot->getX_INLINE(), pPlot->getY_INLINE(), NO_PLAYER, getTeam());

	if (pNearestCity != NULL)
	{
		if (plotDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE()) <= 4)
		{
			return true;
		}
	}

	return false;
}


DomainTypes CvPlayerAI::AI_unitAIDomainType(UnitAITypes eUnitAI) const
{
	switch (eUnitAI)
	{
	case UNITAI_UNKNOWN:
		return NO_DOMAIN;
		break;

	case UNITAI_ANIMAL:
	case UNITAI_SETTLE:
	case UNITAI_WORKER:
	case UNITAI_ATTACK:
	case UNITAI_ATTACK_CITY:
	case UNITAI_COLLATERAL:
	case UNITAI_PILLAGE:
	case UNITAI_RESERVE:
	case UNITAI_COUNTER:
	case UNITAI_PARADROP:
	case UNITAI_CITY_DEFENSE:
	case UNITAI_CITY_COUNTER:
	case UNITAI_CITY_SPECIAL:
	case UNITAI_EXPLORE:
	case UNITAI_MISSIONARY:
	case UNITAI_PROPHET:
	case UNITAI_ARTIST:
	case UNITAI_SCIENTIST:
	case UNITAI_GENERAL:
	case UNITAI_MERCHANT:
	case UNITAI_ENGINEER:
	case UNITAI_GREAT_SPY: // K-Mod
	case UNITAI_SPY:
	case UNITAI_ATTACK_CITY_LEMMING:
		return DOMAIN_LAND;
		break;

	case UNITAI_ICBM:
		return DOMAIN_IMMOBILE;
		break;

	case UNITAI_WORKER_SEA:
	case UNITAI_ATTACK_SEA:
	case UNITAI_RESERVE_SEA:
	case UNITAI_ESCORT_SEA:
	case UNITAI_EXPLORE_SEA:
	case UNITAI_ASSAULT_SEA:
	case UNITAI_SETTLER_SEA:
	case UNITAI_MISSIONARY_SEA:
	case UNITAI_SPY_SEA:
	case UNITAI_CARRIER_SEA:
	case UNITAI_MISSILE_CARRIER_SEA:
	case UNITAI_PIRATE_SEA:
		return DOMAIN_SEA;
		break;

	case UNITAI_ATTACK_AIR:
	case UNITAI_DEFENSE_AIR:
	case UNITAI_CARRIER_AIR:
	case UNITAI_MISSILE_AIR:
		return DOMAIN_AIR;
		break;

	default:
		FAssert(false);
		break;
	}

	return NO_DOMAIN;
}


int CvPlayerAI::AI_yieldWeight(YieldTypes eYield, const CvCity* pCity) const // K-Mod added city argument
{
	/* original bts code
	if (eYield == YIELD_PRODUCTION)
	{
		int iProductionModifier = 100 + (30 * std::max(0, GC.getGame().getCurrentEra() - 1) / std::max(1, (GC.getNumEraInfos() - 2)));
		return (GC.getYieldInfo(eYield).getAIWeightPercent() * iProductionModifier) / 100;
	}

	return GC.getYieldInfo(eYield).getAIWeightPercent(); */

	// K-Mod. In the past, this function was always bundled with some code to boost the value of production and food...
	// For simplicity and consistency, I've brought the adjustments into this function.
	PROFILE_FUNC();

	int iWeight = GC.getYieldInfo(eYield).getAIWeightPercent();
	// Note: to reduce confusion, I'd changed all of the xml yield AIWeightPercent values to be 100. (originally they were f:100, p:110, and c:80)
	// I've adjusted the weights here to compenstate for the changes to the xml weights.
	switch (eYield)
	{
	case YIELD_FOOD:
		if (pCity)
		{
			// was ? 3 : 2
			iWeight *= (pCity->happyLevel() - pCity->unhappyLevel(1) - pCity->foodDifference() >= 0) ?
					//370 : 250; (K-Mod)
					325 : 225; // advc.110: Replacing the above
			iWeight /= 100;
		}
		else
		{
			// was 2.5
			iWeight *= 275; // advc.110: 300 in K-Mod
			iWeight /= 100;
		}
		// <advc.110><advc.115b>
		if(AI_isDoVictoryStrategy(AI_VICTORY_DIPLOMACY4))
			iWeight += 25; // </advc.115b>
		// Gradually reduce weight of food in the second half of the game
		else iWeight -= ::round(60 * std::max(0.0,
				GC.getGameINLINE().gameTurnProgress() - 0.5));
		// </advc.110>
		break;
	case YIELD_PRODUCTION:
		// was 2
		iWeight *= 225; // advc.110: 270 in K-Mod
		iWeight /= 100;
		break;
	case YIELD_COMMERCE:
		if (AI_isFinancialTrouble())
		{
			iWeight *= 2;
		}
		break;
	}
	return iWeight;
	// K-Mod end
}


int CvPlayerAI::AI_commerceWeight(CommerceTypes eCommerce, const CvCity* pCity) const
{
	PROFILE_FUNC();
	int iWeight;

	iWeight = GC.getCommerceInfo(eCommerce).getAIWeightPercent();

	//XXX Add something for 100%/0% type situations
	switch (eCommerce)
	{
	case COMMERCE_RESEARCH:
		if (AI_avoidScience())
		{
			if (isNoResearchAvailable())
			{
				iWeight = 0;
			}
			else
			{
				iWeight /= 8;
			}
		}
		break;
	case COMMERCE_GOLD:
		if (getCommercePercent(COMMERCE_GOLD) >= 80) // originally == 100
		{
			//avoid strikes
			//if (getGoldPerTurn() < -getGold()/100)
			if (getGold()+80*calculateGoldRate() < 0) // k146
			{
				iWeight += 15;
			}
		}
		else if (getCommercePercent(COMMERCE_GOLD) < 25) // originally == 0 (bbai)
		{
			//put more money towards other commerce types
			int iGoldPerTurn = calculateGoldRate(); // K-Mod (the original code used getGoldPerTurn, which is faster, but the wrong number.)
			if (iGoldPerTurn > -getGold()/40)
			{
				iWeight -= 25 - getCommercePercent(COMMERCE_GOLD); // originally 15 (bbai)
				iWeight -= (iGoldPerTurn > 0 && getCommercePercent(COMMERCE_GOLD) == 0) ? 10 : 0; // K-Mod. just a bit extra. (I'd like to compare getGold to AI_goldTarget; but it's too expensive.)
			}
		}
		// K-Mod.
		// (note. This factor is helpful for assigning merchant specialists in the right places, but it causes problems
		// when building wealth. Really we dont' want to scale all gold commerce like this, we only want to scale it when
		// it is independant of the commerce slider.)
		if (pCity != NULL)
		{
			iWeight *= pCity->getTotalCommerceRateModifier(COMMERCE_GOLD);
			iWeight /= AI_averageCommerceMultiplier(COMMERCE_GOLD);
		}
		//
		break;
	case COMMERCE_CULTURE:
		if (pCity != NULL)
		{
			iWeight = pCity->AI_getCultureWeight(); // K-Mod
			FAssert(iWeight >= 0);
		}
		else
		{
			// weight multipliers changed for K-Mod
			if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) || getCommercePercent(COMMERCE_CULTURE) >= 90)
			{
				iWeight *= 4;
			}
			else if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2) || getCommercePercent(COMMERCE_CULTURE) >= 70)
			{
				iWeight *= 3;
			}
			else if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1) || getCommercePercent(COMMERCE_CULTURE) >= 50)
			{
				iWeight *= 2;
			}
			iWeight *= AI_averageCulturePressure();
			iWeight /= 100;

			// K-Mod
			if(isFocusWar()) // advc.105
			//if(GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0)
			{
				if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1))
				{
					iWeight *= 2;
					iWeight /= 3;
				}
				else
				{
					iWeight /= 2;
				}
			}
			// K-Mod end
		}
		break;
	case COMMERCE_ESPIONAGE:
		{
			iWeight = AI_getEspionageWeight(); // K-Mod. (Note: AI_getEspionageWeight use to mean something else.)
		}
		break;
		
	default:
		break;
	}

	return iWeight;
}

// K-Mod.
// Commerce weight is used a lot for AI decisions, and some commerce weights calculations are quite long.
// Some I'm going to cache some of the values and update them at the start of each turn.
// Currently this function caches culture weight for each city, and espionage weight for the player.
void CvPlayerAI::AI_updateCommerceWeights()
{
	PROFILE_FUNC();

	//
	// City culture weight.
	//
	int iLegendaryCulture = GC.getGame().getCultureThreshold((CultureLevelTypes)(GC.getNumCultureLevelInfos() - 1));
	int iVictoryCities = GC.getGameINLINE().culturalVictoryNumCultureCities();

	// Use culture slider to decide whether a human player is going for cultural victory
	bool bUseCultureRank = AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2) || getCommercePercent(COMMERCE_CULTURE) >= 40;
	bool bC3 = AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) || getCommercePercent(COMMERCE_CULTURE) >= 70;
	bool bWarPlans = isFocusWar(); // advc.105
			//GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0;

	std::vector<std::pair<int,int> > city_countdown_list; // (turns, city id)
	{
		int iGoldCommercePercent = bUseCultureRank ? AI_estimateBreakEvenGoldPercent() : getCommercePercent(COMMERCE_GOLD);
		// Note: we only need the gold commerce percent if bUseCultureRank;
		// but I figure that I might as well put in a useful placeholder value just in case.
	
		int iLoop;
		for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			int iCountdown = -1;
			if (bUseCultureRank)
			{
				// K-Mod. Try to tell us what the culture would be like if we were to turn up the slider. (cf. AI_calculateCultureVictoryStage)
				// (current culture rate, minus what we're current getting from raw commerce, plus what we would be getting from raw commerce at maximum percentage.)
				int iEstimatedRate = pLoopCity->getCommerceRate(COMMERCE_CULTURE);
				iEstimatedRate += (100 - iGoldCommercePercent - getCommercePercent(COMMERCE_CULTURE)) * pLoopCity->getYieldRate(YIELD_COMMERCE) * pLoopCity->getTotalCommerceRateModifier(COMMERCE_CULTURE) / 10000;
				iCountdown = (iLegendaryCulture - pLoopCity->getCulture(getID())) / std::max(1, iEstimatedRate);
			}
			city_countdown_list.push_back(std::make_pair(iCountdown, pLoopCity->getID()));
		}
	}
	std::sort(city_countdown_list.begin(), city_countdown_list.end());

	FAssert(city_countdown_list.size() == getNumCities());
	for (size_t i = 0; i < city_countdown_list.size(); i++)
	{
		CvCity* pCity = getCity(city_countdown_list[i].second);

		// COMMERCE_CULTURE AIWeightPercent is set to 30% in the current xml.
		int iWeight = GC.getCommerceInfo(COMMERCE_CULTURE).getAIWeightPercent();

		int iPressureFactor = pCity->AI_culturePressureFactor();
		if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2))
			iPressureFactor = std::min(300, iPressureFactor); // don't let culture pressure dominate our decision making about where to put our culture.
		int iPressureWeight = iWeight * (iPressureFactor-100) / 100;

		if (pCity->getCulture(getID()) >= iLegendaryCulture)
		{
			iWeight /= 10;
		}
		else if (bUseCultureRank)
		{
			int iCultureRateRank = i+1; // an alias, used for clarity.

			// if one of the currently best cities, then focus hard
			if (iCultureRateRank <= iVictoryCities)
			{
				if (bC3)
				{
					// culture3
					iWeight *= 2 + 2*iCultureRateRank + (iCultureRateRank == iVictoryCities ? 1 : 0);
				}
				else
				{
					// culture2
					iWeight *= 6 + iCultureRateRank;
					iWeight /= 2;
				}
				// scale the weight, just in case we're playing on a mod with a different number of culture cities.
				iWeight *= 3;
				iWeight /= iVictoryCities;
			}
			// if one of the 3 close to the top, then still emphasize culture some
			else if (iCultureRateRank <= iVictoryCities + 3)
			{
				//iWeight *= bC3 ? 4 : 3;
				iWeight *= 3;
			}
			else
			{
				iWeight *= 2;
			}
		}
		else if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1) || getCommercePercent(COMMERCE_CULTURE) >= 30)
		{
			iWeight *= 2;
		}
		// K-Mod. Devalue culture in cities which really have no use for it.
		else if (iPressureFactor < 110)
		{
			// if we're not running a culture victory strategy
			// and if we're well into the game, and this city has ample culture, and we don't have any culture pressure...
			// then culture probably isn't worth much to us at all.

			// don't reduce the value if we are still on-track for a potential cultural victory.
			if (getCurrentEra() > GC.getNumEraInfos()/2 && pCity->getCultureLevel() >= 3)
			{
				int iLegendaryCulture = GC.getGame().getCultureThreshold((CultureLevelTypes)(GC.getNumCultureLevelInfos() - 1));
				int iCultureProgress = pCity->getCultureTimes100(getID()) / std::max(1, iLegendaryCulture);
				int iTimeProgress = 100*GC.getGameINLINE().getGameTurn() / GC.getGameINLINE().getEstimateEndTurn();
				iTimeProgress *= iTimeProgress;
				iTimeProgress /= 100;

				if (iTimeProgress > iCultureProgress)
				{
					int iReductionFactor = 100 + std::min(10, 110 - iPressureFactor) * (iTimeProgress - iCultureProgress) / 2;
					FAssert(iReductionFactor >= 100 && iReductionFactor <= 100 + 10 * 100/2);
					iWeight *= 100;
					iWeight /= iReductionFactor;
				}
			}
		}

		if (bWarPlans)
		{
			if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1))
			{
				iWeight *= 2;
				iWeight /= 3;
			}
			else
			{
				iWeight /= 2;
			}
		}

		iWeight += iPressureWeight;

		// Note: value bonus for the first few points of culture is determined elsewhere
		// so that we don't screw up the evaluation of limited culture bonuses such as national wonders.
		pCity->AI_setCultureWeight(iWeight);
	} // end culture

	//
	// Espionage weight
	//
	{	// <k146>
		int iWeightThreshold = 0; // For human players, what amount of espionage weight indicates that we care about a civ?
		if (isHuman())
		{
			int iTotalWeight = 0;
			int iTeamCount = 0;
			for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
			{
				if (GET_TEAM(getTeam()).isHasMet((TeamTypes)iTeam) && GET_TEAM((TeamTypes)iTeam).isAlive())
				{
					iTotalWeight += getEspionageSpendingWeightAgainstTeam((TeamTypes)iTeam);
					iTeamCount++;
				}
			}

			if (iTeamCount > 0)
			{
				iWeightThreshold = iTotalWeight / (iTeamCount +  8); // pretty arbitrary. But as an example, 90 points on 1 player out of 10 -> threshold is 5.
			}
		} // </k146>

		int iWeight = GC.getCommerceInfo(COMMERCE_ESPIONAGE).getAIWeightPercent();

		int iEspBehindWeight = 0;
		int iEspAttackWeight = 0;
		int iAllTeamTotalPoints = 0;
		int iTeamCount = 0;
		int iLocalTeamCount = 0;
		int iTotalUnspent = 0;
		for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
		{
			CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iTeam);

			if (kLoopTeam.isAlive() && iTeam != getTeam() && GET_TEAM(getTeam()).isHasMet((TeamTypes)iTeam) && !kLoopTeam.isVassal(getTeam()) && !GET_TEAM(getTeam()).isVassal((TeamTypes)iTeam))
			{
				iAllTeamTotalPoints += kLoopTeam.getEspionagePointsEver();
				iTeamCount++;

				int iTheirPoints = kLoopTeam.getEspionagePointsAgainstTeam(getTeam());
				int iOurPoints = GET_TEAM(getTeam()).getEspionagePointsAgainstTeam((TeamTypes)iTeam);
				iTotalUnspent += iOurPoints;
				int iAttitude = range(GET_TEAM(getTeam()).AI_getAttitudeVal((TeamTypes)iTeam), -12, 12);
				iTheirPoints -= (iTheirPoints*iAttitude)/(2*12);

				if (iTheirPoints > iOurPoints && (!isHuman() ||
						getEspionageSpendingWeightAgainstTeam((TeamTypes)iTeam) >
						iWeightThreshold)) // k146
				{
					iEspBehindWeight += 1;
					if (kLoopTeam.AI_getAttitude(getTeam()) <= ATTITUDE_CAUTIOUS
						&& GET_TEAM(getTeam()).AI_hasCitiesInPrimaryArea((TeamTypes)iTeam)
						// advc.120: Don't worry so much about espionage while at war
						&& !GET_TEAM(getTeam()).isAtWar(kLoopTeam.getID()))
					{
						iEspBehindWeight += 1;
					}
				}
				if (GET_TEAM(getTeam()).AI_hasCitiesInPrimaryArea((TeamTypes)iTeam))
				{
					iLocalTeamCount++;
					if (GET_TEAM(getTeam()).AI_getAttitude((TeamTypes)iTeam) <= ATTITUDE_ANNOYED
						|| GET_TEAM(getTeam()).AI_getWarPlan((TeamTypes)iTeam) != NO_WARPLAN)
					{
						iEspAttackWeight += 1;
					}
					if (AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE) && GET_PLAYER(kLoopTeam.getLeaderID()).getTechScore() > getTechScore())
					{
						iEspAttackWeight += 1;
					}
				}
			}
		}
		if (iTeamCount == 0)
		{
			iWeight /= 10; // can't put points on anyone - but accumulating "total points ever" is still better than nothing.
		}
		else
		{
			iAllTeamTotalPoints /= std::max(1, iTeamCount); // Get the average total points
			iWeight *= std::min(GET_TEAM(getTeam()).getEspionagePointsEver() + 3*iAllTeamTotalPoints + 16*GC.getGame().getGameTurn(), 5*iAllTeamTotalPoints);
			iWeight /= std::max(1, 4 * iAllTeamTotalPoints);
			// lower weight if we have spent less than a third of our total points
			if (getCommercePercent(COMMERCE_ESPIONAGE) == 0) // ...only if we aren't explicitly trying to get espionage.
			{
				iWeight *= 100 - 270*std::max(iTotalUnspent - std::max(2*GET_TEAM(getTeam()).getEspionagePointsEver()/3, 8*GC.getGame().getGameTurn()), 0)/std::max(1, GET_TEAM(getTeam()).getEspionagePointsEver());
				iWeight /= 100;
			}
			//
			if (AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE))
			{
				iWeight *= 3;
				iWeight /= 2;
			}
			iWeight *= 2*(iEspBehindWeight+iEspAttackWeight) + 3*iTeamCount/4 + 2;

			iWeight *= isHuman() ? 100 : GC.getLeaderHeadInfo(getPersonalityType()).getEspionageWeight();
			iWeight /= (iLocalTeamCount + iTeamCount)/2 + 2;
			iWeight /= 100;

			if (AI_isDoStrategy(AI_STRATEGY_ESPIONAGE_ECONOMY) || (isHuman() && getCommercePercent(COMMERCE_ESPIONAGE) >= 60))
			{
				iWeight = std::max(iWeight, GC.getCommerceInfo(COMMERCE_RESEARCH).getAIWeightPercent());
			}
			else
			{
				iWeight *= 100 + 2*getCommercePercent(COMMERCE_ESPIONAGE);
				iWeight /= 110;
			}
		}
		AI_setEspionageWeight(iWeight);
	} // end espionage
}
// K-Mod

// K-Mod: I've moved the bulk of this function into AI_foundValue_bulk...
short CvPlayerAI::AI_foundValue(int iX, int iY, int iMinRivalRange, bool bStartingLoc) const
{
	CvFoundSettings kSet(*this, bStartingLoc);
	kSet.iMinRivalRange = iMinRivalRange;
	return AI_foundValue_bulk(iX, iY, kSet);
}

CvPlayerAI::CvFoundSettings::CvFoundSettings(const CvPlayerAI& kPlayer, bool bStartingLoc) : bStartingLoc(bStartingLoc)
{
	iMinRivalRange = -1;
	iGreed = 120;
	bEasyCulture = bStartingLoc;
	bAmbitious = false;
	bFinancial = false;
	bDefensive = false;
	bSeafaring = false;
	bExpansive = false;
	bAllSeeing = bStartingLoc || kPlayer.isBarbarian();
	barbDiscouragedRange = 8; // advc.303

	if (!bStartingLoc)
	{
		for (int iI = 0; iI < GC.getNumTraitInfos(); iI++)
		{
			if (kPlayer.hasTrait((TraitTypes)iI))
			{
				if (GC.getTraitInfo((TraitTypes)iI).getCommerceChange(COMMERCE_CULTURE) > 0)
				{
					bEasyCulture = true;
					if (GC.getLeaderHeadInfo(kPlayer.getPersonalityType()).getBasePeaceWeight() <= 5)
						bAmbitious = true;
					iGreed += 15 * GC.getTraitInfo((TraitTypes)iI).getCommerceChange(COMMERCE_CULTURE);
				}
				if (GC.getTraitInfo((TraitTypes)iI).getExtraYieldThreshold(YIELD_COMMERCE) > 0)
				{
					bFinancial = true;
				}

				for (int iJ = 0; iJ < GC.getNumPromotionInfos(); iJ++)
				{
					if (GC.getTraitInfo((TraitTypes)iI).isFreePromotion(iJ))
					{
						// aggressive, protective... it doesn't really matter to me.
						if (GC.getLeaderHeadInfo(kPlayer.getPersonalityType()).getBasePeaceWeight() >= 5)
						{
							bDefensive = true;
						}
					}
				}

				for (int iJ = 0; iJ < GC.getNumUnitInfos(); iJ++)
				{
					if (GC.getUnitInfo((UnitTypes)iJ).isFound() &&
						GC.getUnitInfo((UnitTypes)iJ).getProductionTraits(iI) &&
						kPlayer.canTrain((UnitTypes)iJ))
					{
						iGreed += 20;
						if (GC.getLeaderHeadInfo(kPlayer.getPersonalityType()).getMaxWarRand() <= 150)
							bAmbitious = true;
					}
				}
			}
		}
		// seafaring test for unique unit and unique building
		if (kPlayer.getCoastalTradeRoutes() > 0)
			bSeafaring = true;
		if (!bSeafaring)
		{
			for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
			{
				UnitTypes eCivUnit = (UnitTypes)GC.getCivilizationInfo(kPlayer.getCivilizationType()).getCivilizationUnits(iI);
				UnitTypes eDefaultUnit = (UnitTypes)GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex();
				if (eCivUnit != NO_UNIT && eCivUnit != eDefaultUnit)
				{
					if (GC.getUnitInfo(eCivUnit).getDomainType() == DOMAIN_SEA)
					{
						bSeafaring = true;
						break;
					}
				}
			}
		}
		if (!bSeafaring)
		{
			for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
			{
				BuildingTypes eCivBuilding = (BuildingTypes)GC.getCivilizationInfo(kPlayer.getCivilizationType()).getCivilizationBuildings(iI);
				BuildingTypes eDefaultBuilding = (BuildingTypes)GC.getBuildingClassInfo((BuildingClassTypes)iI).getDefaultBuildingIndex();
				if (eCivBuilding !=
						NO_BUILDING // kmodx: was NO_UNIT
					&& eCivBuilding != eDefaultBuilding)
				{
					if (GC.getBuildingInfo(eCivBuilding).isWater())
					{
						bSeafaring = true;
						break;
					}
				}
			}
		}
		// culture building process
		if (!bEasyCulture)
		{
			for (int iJ = 0; iJ < GC.getNumProcessInfos(); iJ++)
			{
				if (GET_TEAM(kPlayer.getTeam()).isHasTech((TechTypes)GC.getProcessInfo((ProcessTypes)iJ).getTechPrereq()) &&
					GC.getProcessInfo((ProcessTypes)iJ).getProductionToCommerceModifier(COMMERCE_CULTURE) > 0)
				{
					bEasyCulture = true;
					break;
				}
			}
		}
		// free culture building
		if (!bEasyCulture)
		{
			for (int iJ = 0; iJ < GC.getNumBuildingInfos(); iJ++)
			{
				if (kPlayer.isBuildingFree((BuildingTypes)iJ) && GC.getBuildingInfo((BuildingTypes)iJ).getObsoleteSafeCommerceChange(COMMERCE_CULTURE) > 0)
				{
					bEasyCulture = true;
					break;
				}
			}
		}
		// easy artists
		if (!bEasyCulture)
		{
			for (int iJ = 0; iJ < GC.getNumSpecialistInfos(); iJ++)
			{
				if (kPlayer.isSpecialistValid((SpecialistTypes)iJ) && kPlayer.specialistCommerce((SpecialistTypes)iJ, COMMERCE_CULTURE) > 0)
				{
					bEasyCulture = true;
					break;
				}
			}
		}
	}

	if (kPlayer.AI_getFlavorValue(FLAVOR_GROWTH) > 0)
		bExpansive = true;

	if (kPlayer.getAdvancedStartPoints() >= 0)
	{
		iGreed = 200; // overruling previous value;
	}

	if (kPlayer.isHuman())
	{
		// don't use personality based traits for human players.
		bAmbitious = false;
		bDefensive = false;
		bExpansive = false;
	}

	iClaimThreshold = 100; // this will be converted into culture units at the end
	if (!bStartingLoc)
	{
		int iCitiesTarget = std::max(1, GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities());
		iClaimThreshold = 100 + 100 * kPlayer.getCurrentEra() / std::max(1, GC.getNumEraInfos()-1);
		iClaimThreshold += 80 * std::max(0, iCitiesTarget - kPlayer.getNumCities()) / iCitiesTarget;

		iClaimThreshold *= bEasyCulture ? (kPlayer.getCurrentEra() < 2 ? 200 : 150) : 100;
		iClaimThreshold *= bAmbitious ? 150 : 100;
		iClaimThreshold /= 10000;
	}
    iClaimThreshold *= 2 * GC.getGameINLINE().getCultureThreshold((CultureLevelTypes)std::min(2, GC.getNumCultureLevelInfos() - 1));
	// note, plot culture is roughly 10x city culture. (cf. CvCity::doPlotCultureTimes100)
}

// Heavily edited for K-Mod (some changes marked, others not.)
// note, this function is called for every revealed plot for every player at the start of every turn.
// try to not make it too slow!
short CvPlayerAI::AI_foundValue_bulk(int iX, int iY, const CvFoundSettings& kSet) const
{
	PROFILE_FUNC();

	int iResourceValue = 0;
	int iSpecialFood = 0;
	int iSpecialFoodPlus = 0;
	int iSpecialFoodMinus = 0;
	int iSpecialProduction = 0;
	int iSpecialCommerce = 0;
	// advc.031: int->double
	double baseProduction = 0; // K-Mod. (used to devalue cities which are unable to get any production.)
	double specials = 0; // advc.031: Number of tiles with special yield

	bool bNeutralTerritory = true;
	bool firstColony = false; // advc.040

	if (!canFound(iX, iY))
	{
		return 0;
	}
	CvGame& g = GC.getGameINLINE(); // advc.003
	CvPlot* pPlot = GC.getMapINLINE().plotINLINE(iX, iY);
	CvArea* pArea = pPlot->area();
	bool bIsCoastal = pPlot->isCoastalLand(GC.getMIN_WATER_SIZE_FOR_OCEAN());
	int iNumAreaCities = pArea->getCitiesPerPlayer(getID());
	// <advc.040>
	if(bIsCoastal && iNumAreaCities <= 0 && !isBarbarian() &&
			(pPlot->isFreshWater() ||
			/*  Don't apply first-colony logic to Tundra and Ice b/c these are
				likely surrounded by more (unrevealed) Tundra and Ice.
				(Non-river Desert also excluded, perhaps unfortunately.) */
			pPlot->calculateNatureYield(YIELD_FOOD, getTeam(), true) +
			pPlot->calculateNatureYield(YIELD_PRODUCTION, getTeam(), true) > 1))
		firstColony = true; // </advc.040>
	bool bAdvancedStart = (getAdvancedStartPoints() >= 0);

	if (!kSet.bStartingLoc && !bAdvancedStart)
	{
		if (!bIsCoastal && iNumAreaCities == 0)
		{
			return 0;
		}
	}

	if (bAdvancedStart)
	{
		//FAssert(!bStartingLoc);
		FAssert(g.isOption(GAMEOPTION_ADVANCED_START));
		if (kSet.bStartingLoc)
		{
			bAdvancedStart = false;
		}
	}

	// K-Mod. city site radius check used to be here. I've moved it down a bit.
	// ... and also the bonus vector initializtion

	if (kSet.iMinRivalRange != -1)
	{
		for (int iDX = -(kSet.iMinRivalRange); iDX <= kSet.iMinRivalRange; iDX++)
		{
			for (int iDY = -(kSet.iMinRivalRange); iDY <= kSet.iMinRivalRange; iDY++)
			{
				CvPlot* pLoopPlot = plotXY(iX, iY, iDX, iDY);

				if (pLoopPlot != NULL)
				{
					if (pLoopPlot->plotCheck(PUF_isOtherTeam, getID()) != NULL)
					{
						return 0;
					}
				}
			}
		}
	}

	if (kSet.bStartingLoc)
	{
		if (pPlot->isGoody())
		{
			return 0;
		}

		for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			CvPlot* pLoopPlot = plotCity(iX, iY, iI);

			if (pLoopPlot == NULL)
			{
				return 0;
			}
		}
	}

	// (K-Mod this site radius check code was moved from higher up)
	//Explaination of city site adjustment:
	//Any plot which is otherwise within the radius of a city site
	//is basically treated as if it's within an existing city radius
	std::vector<bool> abCitySiteRadius(NUM_CITY_PLOTS, false);
	/*  <advc.035>
		Need to distinguish tiles within the radius of one of our team's cities
		from those within just any city radius. */
	std::vector<bool> abOwnCityRadius(NUM_CITY_PLOTS, false);
	// Whether the tile flips to us once we settle near it
	std::vector<bool> abFlip(NUM_CITY_PLOTS, false);
	// K-Mod. bug fixes etc. (original code deleted)
	if (!kSet.bStartingLoc)
	{
		for (int iJ = 0; iJ < AI_getNumCitySites(); iJ++)
		{
			CvPlot* pCitySitePlot = AI_getCitySite(iJ);
			if (pCitySitePlot == pPlot)
				continue;
			FAssert(pCitySitePlot != NULL);

			if (plotDistance(iX, iY, pCitySitePlot->getX_INLINE(), pCitySitePlot->getY_INLINE()) <= GC.getMIN_CITY_RANGE() &&
				pPlot->area() == pCitySitePlot->area())
			{
				// this tile is too close to one of the sites we've already chosen.
				return 0; // we can't settle here.
			}
			for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
			{
				CvPlot* pLoopPlot = plotCity(iX, iY, iI);
				if (pLoopPlot != NULL)
				{
					if (plotDistance(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), pCitySitePlot->getX_INLINE(), pCitySitePlot->getY_INLINE()) <= CITY_PLOTS_RADIUS)
					{
						//Plot is inside the radius of a city site
						abCitySiteRadius[iI] = true;
					}
				}
			}
		} // <advc.035>
		if(GC.getOWN_EXCLUSIVE_RADIUS() > 0) {
			int dummy=-1;
			for(CvCity* c = firstCity(&dummy); c != NULL; c = nextCity(&dummy)) {
				for(int i = 0; i < NUM_CITY_PLOTS; i++) {
					CvPlot* p = plotCity(iX, iY, i);
					if(p != NULL && plotDistance(p->getX_INLINE(), p->getY_INLINE(),
							c->getX_INLINE(), c->getY_INLINE()) <= CITY_PLOTS_RADIUS)
						abOwnCityRadius[i] = true;
				}
			}
			for(int i = 0; i < NUM_CITY_PLOTS; i++) {
				CvPlot* p = plotCity(iX, iY, i);
				abFlip[i] = (!abOwnCityRadius[i] && p != NULL &&
						p->isOwned() && p->isCityRadius() &&
						p->getTeam() != getTeam() &&
						(p->getSecondOwner() == getID() ||
						/*  The above is enough b/c the tile may not be within the
							culture range of one of our cities; but it will be once
							the new city is founded. */
						p->findHighestCulturePlayer(true) == getID()));
			}
		} // </advc.035>
	}
	// K-Mod end
	// advc.035: Moved the owned-tile counting down b/c I need abFlip for it
	int iOwnedTiles = 0;
	for(int i = 0; i < NUM_CITY_PLOTS; i++) {
		CvPlot* pLoopPlot = plotCity(iX, iY, i);
		if (pLoopPlot == NULL || (pLoopPlot->isOwned() && pLoopPlot->getTeam() != getTeam()
				&& !abFlip[i])) // advc.035
			iOwnedTiles++;
	}
	if(iOwnedTiles > NUM_CITY_PLOTS / 3)
		return 0;

	std::vector<int> viBonusCount(GC.getNumBonusInfos(), 0);

	int iBadTile = 0;
	int unrev = 0, revDecentLand = 0; // advc.040
	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iI);
		// <advc.303>
		if(isBarbarian() && !::isInnerRing(pLoopPlot, pPlot)) {
			/* Rational barbs wouldn't mind settling one off the coast,
			   but human players do mind, and some really hate this.
			   Therefore, count the outer ring coast as bad if !bIsCoastal. */
			if(pLoopPlot != NULL && pLoopPlot->isWater() && !bIsCoastal &&
					pLoopPlot->calculateBestNatureYield(YIELD_FOOD, getTeam()) <= 1)
				iBadTile++;
			continue;
		} // </advc.303>

		if (iI != CITY_HOME_PLOT)
		{
			/*  <advc.031> NULL and impassable count as 2 bad tiles (as in BtS),
				but don't cheat with unrevealed tiles. */
			if(pLoopPlot == NULL) {
				iBadTile += 2;
				continue; 
			}
			if(!kSet.bAllSeeing && !pLoopPlot->isRevealed(getTeam(), false)) {
				// <advc.040>
				if(firstColony)
					unrev++; // </advc.040>
				continue;
			}
			if(pLoopPlot->isImpassable()) {
				iBadTile += 2;
				continue;
			} // </advc.031>
			// K-Mod (original code deleted)
			if (//!pLoopPlot->isFreshWater() &&
			/*  advc.031: Flood plains have high nature yield anyway,
				so this is about snow. Perhaps snow river should be
				just 1 bad tile(?). Bonus check added (Incense). */
				pLoopPlot->getBonusType() == NO_BONUS &&
				(pLoopPlot->calculateBestNatureYield(YIELD_FOOD, getTeam()) == 0 || pLoopPlot->calculateTotalBestNatureYield(getTeam()) <= 1))
			{	// <advc.031>
				iBadTile++; /*  Snow hills had previously not been counted as bad,
				but they are. Worthless even unless the city is deperate for
				production or has plenty of food. */
				if(!pLoopPlot->isHills() || (baseProduction >= 10 &&
						iSpecialFoodPlus - iSpecialFoodMinus < 5))
					iBadTile++; // </advc.031>
			}
			else if (pLoopPlot->isWater() && pLoopPlot->calculateBestNatureYield(YIELD_FOOD, getTeam()) <= 1)
			{	/* <advc.031> Removed the bIsCoastal check from the
				   condition above b/c I want to count ocean tiles as
				   half bad even when the city is at the coast. */
				if(!kSet.bSeafaring && pLoopPlot->calculateBestNatureYield(
						YIELD_COMMERCE, getTeam()) <= 1)
					iBadTile++;
				if(!bIsCoastal)
					iBadTile++; // </advc.031>
			}
			else if (pLoopPlot->isOwned())
			{
				if(!abFlip[iI]) { // advc.035
					if (pLoopPlot->getTeam() != getTeam() || pLoopPlot->isBeingWorked())
					{
						iBadTile++;
					}
					// note: this final condition is... not something I intend to keep permanently.
					else if (pLoopPlot->isCityRadius() || abCitySiteRadius[iI])
					{
						iBadTile++;
					}
				} // advc.035
			}
			// K-Mod end
			// <advc.040>
			else if(firstColony)
				revDecentLand++; // </advc.040>
		}
	} /* <advc.040> Make sure we do naval exploration near the spot before
		 sending a Settler */
	if(revDecentLand < 4)
		firstColony = false; // </advc.040>
	iBadTile /= 2;

	if (!kSet.bStartingLoc)
	{
		if ((iBadTile > (NUM_CITY_PLOTS / 2)) || (pArea->getNumTiles() <= 2)
			|| (isBarbarian() && iBadTile > 3) // advc.303
			)
		{
			bool bHasGoodBonus = false;
			int freshw = (pPlot->isFreshWater() ? 1 : 0); // advc.031
			for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
			{
				CvPlot* pLoopPlot = plotCity(iX, iY, iI);
				// <advc.303>
				if(isBarbarian() && !::isInnerRing(pLoopPlot, pPlot))
					continue; // </advc.303>

				if (pLoopPlot != NULL && (kSet.bAllSeeing || pLoopPlot->isRevealed(getTeam(), false)))
				{
					if (!(pLoopPlot->isOwned()))
					{
						if (pLoopPlot->isWater() || (pLoopPlot->area() == pArea) || (pLoopPlot->area()->getCitiesPerPlayer(getID()) > 0))
						{
							//BonusTypes eBonus = pLoopPlot->getBonusType(getTeam());
							BonusTypes eBonus = pLoopPlot->getNonObsoleteBonusType(getTeam()); // K-Mod

							if (eBonus != NO_BONUS)
							{
								if ((getNumTradeableBonuses(eBonus) == 0) || (AI_bonusVal(eBonus, 1, true) > 10)
									|| (GC.getBonusInfo(eBonus).getYieldChange(YIELD_FOOD) > 0))
								{
									bHasGoodBonus = true;
									break;
								}
							} // <advc.031>
							if(pLoopPlot->isFreshWater())
								freshw++; // </advc.031>
						}
					}
				}
			}

			if (!bHasGoodBonus
					&& freshw < 3 // advc.031
					&& !firstColony) // advc.040
			{
				return 0;
			}
		}
	}

	int iTakenTiles = 0;
	int iTeammateTakenTiles = 0;
	int iHealth = 0;
	// <advc.031>
	int iRiver = 0;
	int iGreen  = 0; // </advc.031>
	// advc.031: was 800 in K-Mod and 1000 before K-Mod
	int iValue = 600;

	// <advc.040>
	if(firstColony)
		iValue += 55 * std::min(5, unrev); // </advc.040>
	int iYieldLostHere = 0;
	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iI);
		// <advc.303>
		if(isBarbarian() && !::isInnerRing(pLoopPlot, pPlot))
			continue; // </advc.303>
		if (pLoopPlot == NULL)
		{
			iTakenTiles++;
		}
		//else if (pLoopPlot->isCityRadius() || abCitySiteRadius[iI])
		else if (iI != CITY_HOME_PLOT && (pLoopPlot->isCityRadius() || abCitySiteRadius[iI]) // K-Mod
				&& !abFlip[iI]) // advc.035
		{
			iTakenTiles++;
			if (abCitySiteRadius[iI])
			{
				iTeammateTakenTiles++;
			}
		}
		// K-Mod Note: it kind of sucks that no value is counted for taken tiles. Tile sharing / stealing should be allowed.
		else if (kSet.bAllSeeing || pLoopPlot->isRevealed(getTeam(), false))
		{
			int iPlotValue = 0; // K-Mod note. this use to be called iTempValue. I've renamed it throughout this section to improve clarity.

			FeatureTypes eFeature = pLoopPlot->getFeatureType();
			BonusTypes eBonus = pLoopPlot->getBonusType((kSet.bStartingLoc
				  // advc.108: Don't factor in unrevealed bonuses
				  && g.getNormalizationLevel() > 1)
				? NO_TEAM : getTeam());
			ImprovementTypes eBonusImprovement = NO_IMPROVEMENT;
			CvTeamAI const& kTeam = GET_TEAM(getTeam()); // advc.003
			// K-Mod
			bool bRemoveableFeature = false;
			bool bEventuallyRemoveableFeature = false;
			if (eFeature != NO_FEATURE)
			{
				for (int i = 0; i < GC.getNumBuildInfos(); ++i)
				{
					if (GC.getBuildInfo((BuildTypes)i).isFeatureRemove(eFeature))
					{
						bEventuallyRemoveableFeature = true;
						if (kTeam.isHasTech((TechTypes)GC.getBuildInfo((BuildTypes)i).getTechPrereq())
								// <advc.031> bugfix (tagging advc.001)
								&& kTeam.isHasTech((TechTypes)GC.getBuildInfo((BuildTypes)i).
								getFeatureTech(eFeature))) // </advc.031>
						{
							bRemoveableFeature = true;
							break;
						}
					}
				} // advc.003:
				int featHealth = GC.getFeatureInfo(eFeature).getHealthPercent();
				if (iI != CITY_HOME_PLOT && (!bRemoveableFeature || featHealth > 0))
					iHealth += featHealth; // note, this will be reduced by some factor before being added to the total value.
			}
			// K-Mod end

			// K-Mod note: iClaimThreshold is bigger for bEasyCulture and bAmbitious civs.
			// Also note, if the multiplier was to be properly used for unowned plots, it would need
			// to take into account the proximity of foreign cities and so on.
			// (similar to the calculation at the end of this function!)
			int iCultureMultiplier;
			if (!pLoopPlot->isOwned() || pLoopPlot->getOwnerINLINE() == getID()
					|| abFlip[iI]) // advc.035
				iCultureMultiplier = 100;
			else
			{
				bNeutralTerritory = false;

				int iOurCulture = pLoopPlot->getCulture(getID());
				PlayerTypes const loopPlotOwnerId = pLoopPlot->getOwnerINLINE();
				int iOtherCulture = std::max(1, pLoopPlot->getCulture(loopPlotOwnerId));
				/*  <advc.031> AI cities near OCC capital are annoying and don't
					fare well */
				if(GET_PLAYER(loopPlotOwnerId).isHuman() &&
						g.isOption(GAMEOPTION_ONE_CITY_CHALLENGE))
					iOtherCulture = (3 * iOtherCulture) / 2; // </advc.031>
				iCultureMultiplier = 100 * (iOurCulture + kSet.iClaimThreshold);
				iCultureMultiplier /= (iOtherCulture + kSet.iClaimThreshold
						/*  advc.035: The above is OK if cities own their
							exclusive radius ... */
						+ (GC.getOWN_EXCLUSIVE_RADIUS() ? 0 :
						// advc.031: but w/o that rule, it's too optimistic.
						::round(0.72 * (iOurCulture + kSet.iClaimThreshold))));
				iCultureMultiplier = std::min(100, iCultureMultiplier);
				// <advc.035> Increase it a bit?
				/*if(GC.getOWN_EXCLUSIVE_RADIUS())
					iCultureMultiplier = (2 * iCultureMultiplier + 100) / 3;*/
				// </advc.035> Moved up into the else branch:
				if (iCultureMultiplier < ((iNumAreaCities > 0) ? 25 : 50))
				{
					//discourage hopeless cases, especially on other continents.
					iTakenTiles += (iNumAreaCities > 0) ? 1 : 2;
				}
				// <advc.099b>
				double exclRadiusWeight = exclusiveRadiusWeight(
						::plotDistance(pLoopPlot, pPlot));
				iCultureMultiplier = std::min(100, ::round(iCultureMultiplier *
						(1 + exclRadiusWeight))); // </advc.099b>
				/*  <advc.035> The discouragement above is still useful for avoiding
				revolts */
				if(GC.getOWN_EXCLUSIVE_RADIUS())
					iCultureMultiplier = std::max(iCultureMultiplier, 65);
				// </advc.035>
			}

			//
			if (eBonus != NO_BONUS /* <advc.031> A Fort doesn't help if it
									  doesn't actually connect the bonus. */
					&& GET_TEAM(getTeam()).isHasTech((TechTypes)GC.getBonusInfo(
					eBonus).getTechCityTrade())) // </advc.031>
			{	/*  advc.031 (comment): Should not set eBonusImprovement to Fort
					if there is an improvement that increases the tile yield.
					Since Fort comes last in the ImprovementInfos XML, it should
					be OK as it is. */
				for (int iImprovement = 0; iImprovement < GC.getNumImprovementInfos(); ++iImprovement)
				{
					CvImprovementInfo& kImprovement = GC.getImprovementInfo((ImprovementTypes)iImprovement);

					//if (kImprovement.isImprovementBonusMakesValid(eBonus))
					if (kImprovement.isImprovementBonusTrade(eBonus)) // K-Mod. (!!)
					{
						eBonusImprovement = (ImprovementTypes)iImprovement;
						break;
					}
				}
			}

			int aiYield[NUM_YIELD_TYPES];

			for (int iYieldType = 0; iYieldType < NUM_YIELD_TYPES; ++iYieldType)
			{
				YieldTypes eYield = (YieldTypes)iYieldType;
				//aiYield[eYield] = pLoopPlot->getYield(eYield);
				aiYield[eYield] = pLoopPlot->calculateNatureYield(eYield, getTeam(), bEventuallyRemoveableFeature); // K-Mod

				if (iI == CITY_HOME_PLOT)
				{
					// (this section has been rewritten for K-Mod. The original code was bork.)
					int iBasePlotYield = aiYield[eYield];

					if (eFeature != NO_FEATURE && !bEventuallyRemoveableFeature) // note: if the feature is removable, was ignored already
						aiYield[eYield] -= GC.getFeatureInfo(eFeature).getYieldChange(eYield);

					aiYield[eYield] += GC.getYieldInfo(eYield).getCityChange();

					aiYield[eYield] = std::max(aiYield[eYield], GC.getYieldInfo(eYield).getMinCity());

					// K-Mod. Before we make special adjustments, there are some things we need to do with the true values.
					if (eYield == YIELD_PRODUCTION)
						baseProduction += aiYield[YIELD_PRODUCTION];
					else if (eYield == YIELD_FOOD)
						iSpecialFoodPlus += std::max(0, aiYield[YIELD_FOOD] - GC.getFOOD_CONSUMPTION_PER_POPULATION());
					//

					// Subtract the special yield we'd get from a bonus improvement, because we'd miss out on that by settling a city here.
					// Exception: the improvement might be something dud which we wouldn't normally build.
					// eg. +1 food from a plantation should not be counted, because a farm would be just as good.
					// But +1 hammers from a mine should be counted, because we'd build the mine anyway. I haven't thought of a good way to deal with this issue.
					if (eBonusImprovement != NO_IMPROVEMENT)
					{
						aiYield[eYield] -= GC.getImprovementInfo(eBonusImprovement).getImprovementBonusYield(eBonus, eYield);
					}

					// and subtract the base yield, so as to emphasise city plots which add yield rather than remove it. (eg. plains-hill vs floodplain)
					aiYield[eYield] -= iBasePlotYield;
				}
				else if (bEventuallyRemoveableFeature) // (not city tile). adjust for removable features
				{
					const CvFeatureInfo& kFeature = GC.getFeatureInfo(eFeature);

					if (bRemoveableFeature)
					{
						iPlotValue += 10 * kFeature.getYieldChange(eYield);
					}
					else
					{
						if (kFeature.getYieldChange(eYield) < 0)
						{
							iPlotValue -= (eBonus != NO_BONUS ? 25 : 5);
							// advc.031: was 30 *...
							iPlotValue += 25 * kFeature.getYieldChange(eYield);
						}
					} // <advc.031>
					if(aiYield[YIELD_FOOD] >= 2)
						iGreen++; // </advc.031>
				}
				// K-Mod end
				// <advc.031>
				else if(getExtraYieldThreshold(eYield) > 0 &&
						aiYield[eYield] >= getExtraYieldThreshold(eYield))
					aiYield[eYield] += GC.getEXTRA_YIELD(); // </advc.031>
			}
			// K-Mod. add non city plot production to the base production count. (city plot has already been counted)
			if (iI != CITY_HOME_PLOT) {
				baseProduction += aiYield[YIELD_PRODUCTION]; //
				// <advc.031>
				if(iPlotValue > 25 && pLoopPlot->isRiver())
					iRiver++; // </advc.031>
			}
			// (note: these numbers have been adjusted for K-Mod)
			if ((iI == CITY_HOME_PLOT || aiYield[YIELD_FOOD] >= GC.getFOOD_CONSUMPTION_PER_POPULATION())
					/*  advc.031: Lighthouse isn't yet taken into account here,
						but seafood tiles do satisfy the conditions above, and
						are being a bit overrated. */
					&& !pLoopPlot->isWater())
			{
				iPlotValue += 10;
				iPlotValue += aiYield[YIELD_FOOD] * 40;
				iPlotValue += aiYield[YIELD_PRODUCTION] * 30;
				iPlotValue += aiYield[YIELD_COMMERCE] * 20;

				/* original bts code
				if (kSet.bStartingLoc)
				{
					iPlotValue *= 2;
				} */
			}
			else if (aiYield[YIELD_FOOD] == GC.getFOOD_CONSUMPTION_PER_POPULATION() - 1)
			{
				iPlotValue += aiYield[YIELD_FOOD] * 30;
				iPlotValue += aiYield[YIELD_PRODUCTION] * 25;
				iPlotValue += aiYield[YIELD_COMMERCE] * 12;
			}
			else
			{
				iPlotValue += aiYield[YIELD_FOOD] * 15;
				iPlotValue += aiYield[YIELD_PRODUCTION] * 15;
				iPlotValue += aiYield[YIELD_COMMERCE] * 8;
			}

			if (pLoopPlot->isWater())
			{
				// K-Mod. kludge to account for lighthouse and lack of improvements.
				iPlotValue /= (bIsCoastal ? 2 : 3);
				iPlotValue += bIsCoastal ? 8*(aiYield[YIELD_COMMERCE]+aiYield[YIELD_PRODUCTION]) : 0;
				// (K-Mod note, I've moved the iSpecialFoodPlus adjustment elsewhere.)

				//if (kSet.bStartingLoc && !pPlot->isStartingPlot())
				if (kSet.bStartingLoc && getStartingPlot() != pPlot) // K-Mod
				{
					// I'm pretty much forbidding starting 1 tile inland non-coastal.
					// with more than a few coast tiles.
					iPlotValue += bIsCoastal ? 0 : -120; // was -400 (reduced by K-Mod)
				}
			}
			else // is land
			{
				if (iI != CITY_HOME_PLOT) {
					//iBaseProduction += pLoopPlot->isHills() ? 2 : 1;
					/*  The above is pretty bad. We're not going to build
						Workshops everywhere. Doesn't check for Peak or Desert. */
					FeatureTypes ft = pLoopPlot->getFeatureType();
					/*  If not event. removeable, then production from feature
						is already counted above */
					if(bEventuallyRemoveableFeature && ft != NO_FEATURE &&
							GC.getFeatureInfo(ft).getYieldChange(
							YIELD_PRODUCTION) > 0) {
						baseProduction += GC.getFeatureInfo(ft).
								/*  For chopping or Lumbermill.
									(I shouldn't hardcode it like this, but, god,
									is this stuff awkward to implement.) */
								getYieldChange(YIELD_PRODUCTION) + 0.5;
					}
					else if(getNumCities() <= 2) { // Fair enough early on
						if(pLoopPlot->isHills())
							baseProduction += 2;
					}
					else { for(int j = 0; j < GC.getNumImprovementInfos(); j++) {
						ImprovementTypes imprId = (ImprovementTypes)j;
						/*  Not a perfectly safe way to check if we can
							build the improvement, but fast. */
						if(getImprovementCount(imprId) <= 0)
							continue;
						CvImprovementInfo& impr = GC.getImprovementInfo(imprId);
						int yc = impr.getYieldChange(YIELD_PRODUCTION) +
								kTeam.getImprovementYieldChange(imprId,
								YIELD_PRODUCTION);
						/*  Note that any additional yield from putting impr
							on a bonus resource is already counted as
							iSpecialProduction */
						for(int k = 0; k < NUM_YIELD_TYPES && k != YIELD_PRODUCTION; k++) {
							int otherYield = impr.getYieldChange((YieldTypes)k);
							if(otherYield < 0) // Will be less inclined to build it then
								yc += otherYield;
						}
						/*  I'm not bothering with civics and routes here.
							Regarding routes, I guess it's OK to undercount
							b/c more production is needed by the time railroads
							become available.
							A Workshop may also remove a Forest; in this case,
							production is overcounted. */
						// Not fast; loops through all builds.
						if(yc <= 0 || !pLoopPlot->canHaveImprovement(imprId, getTeam()))
							continue;
						FAssertMsg(yc <= 3, "is this much production possible?");
						baseProduction += yc;
					} } // </advc.031>
				}

				if (pLoopPlot->isRiver())
				{
					//iPlotValue += 10; // (original)
					// K-Mod
					//iPlotValue += (kSet.bFinancial || kSet.bStartingLoc) ? 30 : 10;
					//iPlotValue += (pPlot->isRiver() ? 15 : 0);
					// <advc.908a> Replacing the above
					/*  Changed b/c of change to Financial trait, but also b/c
						advc.031 already appreciates high-yield tiles more,
						and I don't want the AI to overvalue tundra and snow
						rivers. */
					int riverPlotVal = (kSet.bStartingLoc ? 25 : 7);
					if(kSet.bFinancial && aiYield[YIELD_FOOD] +
							aiYield[YIELD_PRODUCTION] >= 1)
						riverPlotVal += 11;
					riverPlotVal = std::max(30, riverPlotVal);
					/*  Is this just to steer the AI toward settling at rivers
						rather than trying to make all river plots workable?
						Other than that, I can only think of Levee. */
					if(pPlot->isRiver())
						riverPlotVal += (kSet.bStartingLoc ? 12 : 6);
					iPlotValue += riverPlotVal; // </advc.908a>
				}
				if (pLoopPlot->canHavePotentialIrrigation())
				{
					// in addition to the river bonus
					iPlotValue += 5 + (pLoopPlot->isFreshWater() ? 5 : 0);
				}
			} /* <advc.030> Details chosen so that iPlotValue=150 is a fixpoint;
				 150 seems like a pretty average plot value to me. */
			iPlotValue = ::round(std::max(iPlotValue/3.0,
					std::pow(std::max(0.0, iPlotValue - 35.0), 1.25) / 2.5));
			// </advc.130>
			// K-Mod version (original code deleted)
			if (kSet.bEasyCulture)
			{
				// 5/4 * 21 ~= 9 * 1.5 + 12 * 1;
				iPlotValue *= 5;
				iPlotValue /= 4;
			}
			else
			{
				if ((pLoopPlot->getOwnerINLINE() == getID()) || (stepDistance(iX, iY, pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()) <= 1))
				{
					iPlotValue *= 3;
					iPlotValue /= 2;
				}
			}
			iPlotValue *= kSet.iGreed;
			iPlotValue /= 100;
			// K-Mod end

			iPlotValue *= iCultureMultiplier;
			iPlotValue /= 100;

			iValue += iPlotValue;
			// advc.031: Was 33, but now computed differently.
			if (iCultureMultiplier > 20 //ignore hopelessly entrenched tiles.
					|| GC.getOWN_EXCLUSIVE_RADIUS() > 0) // advc.035
			{

				if (eBonus != NO_BONUS && // K-Mod added water case (!!)
					((pLoopPlot->isWater() && bIsCoastal) || pLoopPlot->area() == pPlot->area() || pLoopPlot->area()->getCitiesPerPlayer(getID()) > 0))
				{
					//iBonusValue = AI_bonusVal(eBonus, 1, true) * ((!kSet.bStartingLoc && (getNumTradeableBonuses(eBonus) == 0) && (paiBonusCount[eBonus] == 1)) ? 80 : 20);
					// K-Mod
					//int iCount = getNumTradeableBonuses(eBonus) == 0 + viBonusCount[eBonus];
					//int iBonusValue = AI_bonusVal(eBonus, 0, true) * 80 / (1 + 2*iCount);
					/*  <advc.031> The "==0" looks like an error. Rather than correct
						this, I'm going to let AI_bonusVal handle the number of
						bonuses already connected. Just doing a division by iCount
						here won't work well for strategic resources. */
					/*  Don't assume that the bonus is enabled.
						And the coefficient was 80, reduced a little. */
					int iBonusValue = AI_bonusVal(eBonus, 1) * 70 / (1 + viBonusCount[eBonus]);
					// Note: 1. the value of starting bonuses is reduced later.
					//       2. iTempValue use to be used throughout this section. I've replaced all references with iBonusValue, for clarity.
					viBonusCount[eBonus]++; // (this use to be above the iBonusValue initialization)
					FAssert(viBonusCount[eBonus] > 0);

					iBonusValue *= (kSet.bStartingLoc ? 100 :
						/*  advc.031: Greed perhaps shouldn't matter here at all.
							Civs that like founding lots of cities are going to
							have lots of resources anyway; that's one thing they
							don't need to be greedy for.
							In BtS, Greed had the role of bEasyCulture; old comment:
							"Greedy founding means getting the best possible sites -
							fitting maximum resources into the fat cross."
							In K-Mod, it seems to be more about founding many
							cities, although Creative trait still factors in ...
							Just throw out Greed entirely? */
							(kSet.iGreed + 200) / 3);
					iBonusValue /= 100;
					// <advc.031>
					if(pLoopPlot->getOwnerINLINE() == getID())
						iBonusValue = 0; // We've already got it
					/*  Stop the AI from securing more than one Oil
						source when Oil is revealed but not yet workable */
					if(eBonusImprovement == NO_IMPROVEMENT &&
							getNumAvailableBonuses(eBonus) <= 0) { int dummy=-1;
						/*  CvPlayer::countOwnedBonuses is too expensive
							I think, but I'm copying a bit of code from there. */
						for(CvCity* c = firstCity(&dummy); c != NULL; c = nextCity(&dummy)) {
							if(c->AI_countNumBonuses(eBonus, true, true, -1) > 0) {
								/*  AI_bonusVal already reduces its result when the resource
									can't be worked, but strategic resources can still have
									bonus values of 500 and more, so this needs to be reduced
									much more. */
								iBonusValue = std::min(125, ::round(iBonusValue * 0.25));
								break;
							}
						}
					}
					// </advc.031>
					if (!kSet.bStartingLoc)
					{
						// K-Mod. (original code deleted)
						if (iI != CITY_HOME_PLOT)
						{	/*  advc.031: Why halve the value of water bonuses?
								Perhaps because they're costly to improve. But
								that's only true in the early game ... */
							if (pLoopPlot->isWater()/*) {
								//iBonusValue /= 2;*/
									&& getCurrentEra() < 3) {
								iBonusValue -= (3 - getCurrentEra()) * 20;
								iBonusValue = std::max(iBonusValue, 0);
							} // </advc.031>
							if (pLoopPlot->getOwnerINLINE() != getID() && stepDistance(pPlot->getX_INLINE(),pPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()) > 1)
							{
								if (!kSet.bEasyCulture)
									iBonusValue = iBonusValue * 3/4;
							}
							FAssert(iCultureMultiplier <= 100);
							iBonusValue = iBonusValue * (kSet.bAmbitious ? 110 : iCultureMultiplier) / 100;
						}
						else if (kSet.bAmbitious)
							iBonusValue = iBonusValue * 110 / 100;
						// K-Mod end
					}

					//iValue += (iBonusValue + 10);
					iResourceValue += iBonusValue; // K-Mod

					if (iI != CITY_HOME_PLOT)
					{
						if (eBonusImprovement != NO_IMPROVEMENT)
						{
							int iSpecialFoodTemp;
							iSpecialFoodTemp = pLoopPlot->calculateBestNatureYield(YIELD_FOOD, getTeam()) + GC.getImprovementInfo(eBonusImprovement).getImprovementBonusYield(eBonus, YIELD_FOOD);

							iSpecialFood += iSpecialFoodTemp;

							iSpecialFoodTemp -= GC.getFOOD_CONSUMPTION_PER_POPULATION();

							iSpecialFoodPlus += std::max(0,iSpecialFoodTemp);
							iSpecialFoodMinus -= std::min(0,iSpecialFoodTemp);
							// <advc.031>
							int iSpecialProdTemp = pLoopPlot->calculateBestNatureYield(
									YIELD_PRODUCTION, getTeam()) +
									GC.getImprovementInfo(eBonusImprovement).
									getImprovementBonusYield(eBonus, YIELD_PRODUCTION);
							iSpecialProduction += iSpecialProdTemp;
							int iSpecialCommTemp = pLoopPlot->calculateBestNatureYield(
									YIELD_COMMERCE, getTeam()) +
									GC.getImprovementInfo(eBonusImprovement).
									getImprovementBonusYield(eBonus, YIELD_COMMERCE);
							iSpecialCommerce += iSpecialCommTemp;
							// No functional change above. Now count the tile as special.
							if(iSpecialFoodTemp > 0 || iSpecialCommTemp + iSpecialProdTemp > 0)
								specials++;
							// </advc.031>
						}

						/*if (pLoopPlot->isWater())
							iValue += (bIsCoastal ? 0 : -800);*/ // (was ? 100 : -800)
						// <advc.031> Replacing the above
						if(pLoopPlot->isWater() && !bIsCoastal)
							iValue -= 100; // </advc.031>
					}
				} // end if usable bonus
				if (eBonusImprovement == NO_IMPROVEMENT && iI != CITY_HOME_PLOT
						/*  <advc.031> Need to account for foreign culture
							somehow, and I don't want to divide the
							special food plus, which is usually just 1 or 2. */
						&& iCultureMultiplier >
							(GC.getOWN_EXCLUSIVE_RADIUS() ? 40 : // advc.035
							55)) // </advc.031>
				{
					// non bonus related special food. (Note: the city plot is counted elsewhere.)
					int iEffectiveFood = aiYield[YIELD_FOOD];

					if (bIsCoastal && pLoopPlot->isWater() && aiYield[YIELD_COMMERCE] > 1) // lighthouse kludge.
						iEffectiveFood += 1;

					iSpecialFoodPlus += std::max(0, iEffectiveFood - GC.getFOOD_CONSUMPTION_PER_POPULATION());
				}
			}
		}
	}

	/* original bts code
	iResourceValue += iSpecialFood * 50;
	iResourceValue += iSpecialProduction * 50;
	iResourceValue += iSpecialCommerce * 50;
	if (kSet.bStartingLoc)
	{
		iResourceValue /= 2;
	} */
	// K-mod. It's tricky to get this right. Special commerce is great in the early game, but not so great later on.
	//        Food is always great - unless we already have too much; and food already affects a bunch of other parts of the site evaluation...
	if (kSet.bStartingLoc)
		iResourceValue /= 4; // try not to make the value of strategic resources too overwhelming. (note: I removed a bigger value reduction from the original code higher up.)
	// Note: iSpecialFood is whatever food happens to be associated with bonuses. Don't value it highly, because it's also counted in a bunch of other ways.
	// <advc.031>
	// Preserve this for later
	int nonYieldResourceVal = std::max(0, iResourceValue);
	if(specials > 0.01) {
		double perSpecial[NUM_YIELD_TYPES] = {0.0};
		perSpecial[YIELD_FOOD] = iSpecialFood / specials;
		perSpecial[YIELD_PRODUCTION] = iSpecialProduction / specials;
		perSpecial[YIELD_COMMERCE] = iSpecialCommerce / specials;
		double coefficients[NUM_YIELD_TYPES] = {0.24, 0.36, 0.32};
		double fromSpecial = 0;
		for(int i = 0; i < NUM_YIELD_TYPES; i++)
			fromSpecial += perSpecial[i] * coefficients[i];
		if(fromSpecial > 0)
			fromSpecial = std::pow(fromSpecial, 1.9) * 75 * specials;
		// advc.test: The K-Mod value for comparison in debugger
		int fromSpecialKmod=iSpecialFood*20+iSpecialProduction*40+iSpecialCommerce*35;
		iResourceValue += ::round(fromSpecial); // </advc.031>
	}
	iValue += std::max(0, iResourceValue);

	if (((iTakenTiles > (NUM_CITY_PLOTS / 3)
		|| (isBarbarian() && iTakenTiles > 2)) // advc.303
		&& iResourceValue < 250) // <advc.031>
		|| (iTakenTiles >= (2 * NUM_CITY_PLOTS) / 3 &&
		iResourceValue < 800)) // </advc.031>
	{
		return 0;
	}
	/*  <advc.031> There's already a food surplus factor toward the end of
		the function, but that one doesn't punish cities with low food much;
		not sure if it's intended to do that. */
	if(!kSet.bStartingLoc && getCurrentEra() < 4) {
		double lowFoodFactor = (7 + iGreen + iSpecialFoodPlus -
				iSpecialFoodMinus) / 10.0;
		lowFoodFactor = ::dRange(lowFoodFactor, 0.45, 1.0);
		// Don't apply this to the value from happiness/health/strategic
		iValue = ::round((iValue - nonYieldResourceVal) * lowFoodFactor)
				+ nonYieldResourceVal;
	} // </advc.031>
	/* original bts code (K-Mod: just go look at what "iTeammateTakenTiles" actually is...)
	if (iTeammateTakenTiles > 1)
	{
		return 0;
	}*/
	/*  <advc.031> Bad health in the early game needs to be discouraged more
		rigorously. Treat that case toward the end of this function. */
	if(iHealth > 0 || getCurrentEra() > 1) {
		iValue += std::min(iHealth, 350) / 6;
		// A quick and dirty treatment of chopping opportunities
		if(getCurrentEra() > 0 && iHealth > 200)
			iValue += (iHealth - 200) / (getCurrentEra() + 2);
	} // </advc.031>
	if (bIsCoastal)
	{
		if (!kSet.bStartingLoc)
		{
			if (pArea->getCitiesPerPlayer(getID()) == 0)
			{
				if (bNeutralTerritory)
				{
					//iValue += (iResourceValue > 0) ? 800 : 100; // (BtS code)
					// K-Mod replacement:
					//iValue += iResourceValue > 0 ? (kSet.bSeafaring ? 600 : 400) : 100;
					// advc.040: Mostly handled by the firstColony code now
					if(iResourceValue > 0 && kSet.bSeafaring)
						iValue += 250;
				}
			}
			else
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/03/09                                jdog5000      */
/*                                                                                              */
/* Settler AI           (edited by K-Mod)                                                       */
/************************************************************************************************/
				//iValue += 200;
				int iSeaValue = 0; // advc.031: was 50; instead:
				iValue += 50;
				// Push players to get more coastal cities so they can build navies
				CvArea* pWaterArea = pPlot->waterArea(true);
				if( pWaterArea != NULL )
				{	// advc.031: Replacing the line below
					iSeaValue += (kSet.bSeafaring ? 140 : 80);
					//120 + (kSet.bSeafaring ? 160 : 0);
					if( GET_TEAM(getTeam()).AI_isWaterAreaRelevant(pWaterArea)
							/*  advc.031: Don't worry about coastal production if we
								already have many coastal cities. */
							&& countNumCoastalCities() <= getNumCities() / 3)
					{	// advc.031: Replacing the line below
						iSeaValue += (kSet.bSeafaring ? 240 : 125);
						//iSeaValue += 120 + (kSet.bSeafaring ? 160 : 0);
						//if( (countNumCoastalCities() < (getNumCities()/4)) || (countNumCoastalCitiesByArea(pPlot->area()) == 0) )
						if (
							// advc.031: Disabled this clause
							//countNumCoastalCities() < getNumCities()/4 ||
							(pPlot->area()->getCitiesPerPlayer(getID()) > 0 && countNumCoastalCitiesByArea(pPlot->area()) == 0))
						{
							iSeaValue += 200;
						}
					}
				} /* <advc.031> Apply greed only half. (Why is settling at the coast
					 greedy at all? B/c it leaves more room for inland cities). */
				int halfGreed = 100 + (kSet.iGreed - 100) / 2;
				if(halfGreed > 100) {
					iSeaValue *= halfGreed;
					iSeaValue /= 100;
				}
				// Modify based on production (since the point is to build a navy)
				double mult = (baseProduction + iSpecialProduction) / 18.0;
				mult = 0.7 * ::dRange(mult, 0.1, 2.0);
				// That's the name of the .py file; not language-dependent.
				if(GC.getInitCore().getMapScriptName().compare(L"Pangaea") == 0)
					mult /= 2;
				iSeaValue = ::round(iSeaValue * mult);
				// </advc.031>
				iValue += iSeaValue;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			}
		}
		else
		{
			//let other penalties bring this down.
			iValue += 500; // was 600
			//if (!pPlot->isStartingPlot())
			if (getStartingPlot() != pPlot) // K-Mod
			{
				if (pArea->getNumStartingPlots() == 0)
				{
					iValue += 600; // was 1000
				}
			}
		}
	}

	if (pPlot->isHills())
	{
		// iValue += 200;
		// K-Mod
		iValue += 100 + (kSet.bDefensive ? 100 : 0);
	}

	if (pPlot->isFreshWater())
	{
		// iValue += 40; // K-Mod (commented this out, compensated by the river bonuses I added.)
		iValue += (GC.getDefineINT("FRESH_WATER_HEALTH_CHANGE") * 30);
	}

	if (kSet.bStartingLoc)
	{
		//int iRange = GREATER_FOUND_RANGE;
		int iRange = 6; // K-Mod (originally was 5)
		int iGreaterBadTile = 0;

		for (int iDX = -(iRange); iDX <= iRange; iDX++)
		{
			for (int iDY = -(iRange); iDY <= iRange; iDY++)
			{
				CvPlot* pLoopPlot = plotXY(iX, iY, iDX, iDY);

				if (pLoopPlot != NULL)
				{
					if (pLoopPlot->isWater() || (pLoopPlot->area() == pArea)) // K-Mod
					//if (pLoopPlot->isWater() || (pLoopPlot->area() == pArea))
					{
						if (plotDistance(iX, iY, pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()) <= iRange)
						{
							/* original bts code
							int iTempValue = 0;
							iTempValue += (pLoopPlot->getYield(YIELD_FOOD) * 15);
							iTempValue += (pLoopPlot->getYield(YIELD_PRODUCTION) * 11);
							iTempValue += (pLoopPlot->getYield(YIELD_COMMERCE) * 5);
							iValue += iTempValue;
							if (iTempValue < 21)
							{
								iGreaterBadTile += 2;
								if (pLoopPlot->getFeatureType() != NO_FEATURE)
								{
									if (pLoopPlot->calculateBestNatureYield(YIELD_FOOD,getTeam()) > 1)
									{
										iGreaterBadTile--;
									}
								}
							} */
							// K-Mod
							int iTempValue = 0;
							iTempValue += pLoopPlot->getYield(YIELD_FOOD) * 9;
							iTempValue += pLoopPlot->getYield(YIELD_PRODUCTION) * 5;
							iTempValue += pLoopPlot->getYield(YIELD_COMMERCE) * 3;
							iTempValue += pLoopPlot->isRiver() ? 1 : 0;
							iTempValue += pLoopPlot->isWater() ? -2 : 0;
							if (iTempValue < 13)
							{
								// 3 points for unworkable plots (desert, ice, far-ocean)
								// 2 points for bad plots (ocean, tundra)
								// 1 point for fixable bad plots (jungle)
								iGreaterBadTile++;
								if (pLoopPlot->calculateBestNatureYield(YIELD_FOOD,getTeam()) < 2)
								{
									iGreaterBadTile++;
									if (iTempValue <= 0)
										iGreaterBadTile++;
								}
							}
							if (pLoopPlot->isWater() || pLoopPlot->area() == pArea)
								iValue += iTempValue;
							else if (iTempValue >= 13)
								iGreaterBadTile++; // add at least 1 badness point for other islands.
							// K-Mod end
						}
					}
				}
			}
		}

		//if (!pPlot->isStartingPlot())
		if (getStartingPlot() != pPlot) // K-Mod
		{
			/* original bts code
			iGreaterBadTile /= 2;
			if (iGreaterBadTile > 12)
			{
				iValue *= 11;
				iValue /= iGreaterBadTile;
			} */
			// K-Mod. note: the range has been extended, and the 'bad' counting has been rescaled.
			iGreaterBadTile /= 3;
			int iGreaterRangePlots = 2*(iRange*iRange + iRange) + 1;
			if (iGreaterBadTile > iGreaterRangePlots/6)
			{
				iValue *= iGreaterRangePlots/6;
				iValue /= iGreaterBadTile;
			}

			// Maybe we can make a value adjustment based on the resources and players currently in this area
			// (wip)
			int iBonuses = 0;
			int iTypes = 0;
			int iPlayers = pArea->getNumStartingPlots();
			for (BonusTypes i = (BonusTypes)0; i < GC.getNumBonusInfos(); i=(BonusTypes)(i+1))
			{
				if (pArea->getNumBonuses(i) > 0)
				{
					iBonuses += 100 * pArea->getNumBonuses(i) / std::max(2, iPlayers + 1);
					iTypes += std::min(100, 100 * pArea->getNumBonuses(i) / std::max(2, iPlayers + 1));
				}
			}
			// bonus for resources per player on the continent. (capped to avoid overflow)
			iValue = iValue * std::min(400, 100 + 2 * iBonuses/100 + 8 * iTypes/100) / 130;

			// penality for a solo start. bonus for pairing with an existing solo start. penality for highly populated islands
			iValue = iValue * (10 + std::max(-2, iPlayers ? 2 - iPlayers : -2)) / 10;
			/* if (iPlayers == 0)
				iValue = iValue * 85 / 100;
			else if (iPlayers == 1)
				iValue = iValue * 110 / 100; */

			// K-Mod end
		}

		/* original bts code
		int iWaterCount = 0;

		for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			CvPlot* pLoopPlot = plotCity(iX, iY, iI);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->isWater())
				{
					iWaterCount ++;
					if (pLoopPlot->getYield(YIELD_FOOD) <= 1)
					{
						iWaterCount++;
					}
				}
			}
		}
		iWaterCount /= 2;

		int iLandCount = (NUM_CITY_PLOTS - iWaterCount);

		if (iLandCount < (NUM_CITY_PLOTS / 2))
		{
			//discourage very water-heavy starts.
			iValue *= 1 + iLandCount;
			iValue /= (1 + (NUM_CITY_PLOTS / 2));
		} */ // disabled by K-Mod (water starts are discouraged in other ways)
	/*}
	// advc.003: Same condition as the previous block
	if (kSet.bStartingLoc)
	{*/
		/* original bts code
		if (pPlot->getMinOriginalStartDist() == -1)
		{
			iValue += (GC.getMapINLINE().maxStepDistance() * 100);
		}
		else
		{
			iValue *= (1 + 4 * pPlot->getMinOriginalStartDist());
			iValue /= (1 + 2 * GC.getMapINLINE().maxStepDistance());
		} */
		// K-Mod. In the original code, getMinOriginalStartDist was always zero once the
		// starting positions had been assigned; and so this factor didn't work correctly.
		// I've fixed it (see change in updateMinOriginalStartDist.)
		// But I've now disabled it completely because this stuff is handled elsewhere anyway.
		/* int iMinRange = startingPlotRange();
		{
			int iScale = std::min(4*iMinRange, GC.getMapINLINE().maxStepDistance());
			int iExistingDist = pPlot->getMinOriginalStartDist() > 0
				? std::min(pPlot->getMinOriginalStartDist(), iScale)
				: iScale;
			FAssert(iScale > 2 && iExistingDist > 2); // sanity check
			iValue *= (1 + iScale + 4 * iExistingDist);
			iValue /= (1 + 3 * iScale);
		} */
		// K-Mod end

		//nice hacky way to avoid this messing with normalizer, use elsewhere?
		//if (!pPlot->isStartingPlot())
		if (getStartingPlot() != pPlot) // K-Mod. 'isStartingPlot' is not automatically set.
		{
			int iMinDistanceFactor = MAX_INT;
			int iMinRange = startingPlotRange();

			//iValue *= 100; // (disabled by K-Mod to prevent int overflow)
			for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
			{
				if (GET_PLAYER((PlayerTypes)iJ).isAlive())
				{
					if (iJ != getID())
					{
						int iClosenessFactor = GET_PLAYER((PlayerTypes)iJ).startingPlotDistanceFactor(pPlot, getID(), iMinRange);
						iMinDistanceFactor = std::min(iClosenessFactor, iMinDistanceFactor);

						if (iClosenessFactor < 1000)
						{
							iValue *= 2000 + iClosenessFactor;
							iValue /= 3000;
						}
					}
				}
			}

			if (iMinDistanceFactor > 1000)
			{
				//give a maximum boost of 25% for somewhat distant locations, don't go overboard.
				iMinDistanceFactor = std::min(1500, iMinDistanceFactor);
				iValue *= (1000 + iMinDistanceFactor);				
				iValue /= 2000;
			}
			else if (iMinDistanceFactor < 1000)
			{
				//this is too close so penalize again.
				iValue *= iMinDistanceFactor;
				iValue /= 1000;
				iValue *= iMinDistanceFactor;
				iValue /= 1000;
			}

			if (pPlot->getBonusType() != NO_BONUS)
			{
				iValue /= 2;
			}
		}
	}

	if (bAdvancedStart)
	{
		if (pPlot->getBonusType() != NO_BONUS)
		{
			iValue *= 70;
			iValue /= 100;
		}
	}

	// K-Mod. reduce value of cities which will struggle to get any productivity.
	{
		baseProduction += iSpecialProduction;
		// <advc.040>
		baseProduction += std::max(unrev / 2, ::round(
				baseProduction * (unrev / (double)NUM_CITY_PLOTS)));
		// </advc.040>
		/*  advc.031: The assertion below can fail when there are gems under
			jungle. Something's not quite correct in the baseProduction
			computation, but it isn't worth fretting over. */
		baseProduction = std::max(baseProduction, (double)GC.getYieldInfo(YIELD_PRODUCTION).getMinCity());
		//FAssert(!pPlot->isRevealed(getTeam(), false) || baseProduction >= GC.getYieldInfo(YIELD_PRODUCTION).getMinCity());
		int iThreshold = 9; // pretty arbitrary
		// <advc.303> Can't expect that much production from just the inner ring.
		if(isBarbarian())
			iThreshold = 4; // </advc.303>
		if (baseProduction < iThreshold)
		{	// advc.031: Replacing the two lines below
			iValue = ::round(iValue * baseProduction / iThreshold);
			/*iValue *= iBaseProduction;
			iValue /= iThreshold;*/
		}
	}
	// K-Mod end

	//CvCity* pNearestCity = GC.getMapINLINE().findCity(iX, iY, ((isBarbarian()) ? NO_PLAYER : getID()));
	// K-Mod. Adjust based on proximity to other players, and the shape of our empire.
	if (isBarbarian())
	{
		CvCity* pNearestCity = GC.getMapINLINE().findCity(iX, iY, NO_PLAYER);
		/* <advc.303>, advc.300. Now that the outer ring isn't counted, I worry
		   that an absolute penalty would reduce iValue to 0 too often. Also want
		   to discourage touching borders a bit more, which the relative penalty
		   should accomplish better.
		   Don't see the need for special treatment of nearest city in a
		   different area. CvGame avoids settling uninhabited areas anyway.
		   (maybe I'm missing sth. here) */
		if(pNearestCity == NULL)
			pNearestCity = GC.getMapINLINE().findCity(iX, iY, NO_PLAYER, NO_TEAM,
					false);
		if(pNearestCity != NULL) {
			iValue *= std::min(kSet.barbDiscouragedRange, plotDistance(iX, iY,
					pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE()));
			iValue /= kSet.barbDiscouragedRange;
		}
		/*if (pNearestCity)
			iValue -= (std::max(0, (8 - plotDistance(iX, iY, pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE()))) * 200);
		else
		{
			pNearestCity = GC.getMapINLINE().findCity(iX, iY, NO_PLAYER, NO_TEAM, false);
			if (pNearestCity != NULL)
			{
				int iDistance = plotDistance(iX, iY, pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE());
				iValue -= std::min(500 * iDistance, (8000 * iDistance) / GC.getMapINLINE().maxPlotDistance());
			}
		}*/ // </advc.303>
	}
	else if (!kSet.bStartingLoc)
	{
		int iForeignProximity = 0;
		int iOurProximity = 0;
		CvCity* pNearestCity = 0;
		CvCity* pCapital = getCapitalCity();
		int iMaxDistanceFromCapital = 0;

		for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i = (PlayerTypes)(i+1))
		{
			const CvPlayer& kLoopPlayer = GET_PLAYER(i);
			if (pArea->getCitiesPerPlayer(i) > 0 && GET_TEAM(getTeam()).isHasMet(kLoopPlayer.getTeam()) && !GET_TEAM(kLoopPlayer.getTeam()).isVassal(getTeam()))
			{
				int iProximity = 0;

				int iLoop;
				for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
				{
					if (i == getID())
					{
						if (pCapital)
							iMaxDistanceFromCapital = std::max(iMaxDistanceFromCapital, plotDistance(pCapital->plot(), pLoopCity->plot()));
					}

					if (pLoopCity->getArea() == pArea->getID())
					{
						int iDistance = plotDistance(iX, iY, pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE());

						if (i == getID())
						{
							if (!pNearestCity || iDistance < plotDistance(iX, iY, pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE()))
								pNearestCity = pLoopCity;
						}

						int iCultureRange = pLoopCity->getCultureLevel() + 3;
						if (iDistance <= iCultureRange && AI_deduceCitySite(pLoopCity))
						{
							// cf. culture distribution in CvCity::doPlotCultureTimes100
							iProximity += 90*(iDistance-iCultureRange)*(iDistance-iCultureRange)/(iCultureRange*iCultureRange) + 10;
						}
					}
				}
				if (kLoopPlayer.getTeam() == getTeam())
					iOurProximity = std::max(iOurProximity, iProximity);
				else
					iForeignProximity = std::max(iForeignProximity, iProximity);
			}
		}
		// Reduce the value if we are going to get squeezed out by culture.
		// Increase the value if we are hoping to block the other player!
		if (iForeignProximity > 0)
		{
			// As a rough guide of scale, settling 3 steps from a level 2 city in isolation would give a proximity of 24.
			// 4 steps from a level 2 city = 13
			// 4 steps from a level 3 city = 20
			int iDelta = iForeignProximity - iOurProximity;

			if (iDelta > 50)
				return 0; // we'd be crushed and eventually flipped if we settled here.

			if (iDelta > -20 && iDelta <= (kSet.bAmbitious ? 10 : 0) * (kSet.bEasyCulture ? 2 : 1))
			{
				int iTmp = iValue; // advc.031
				// we want to get this spot before our opponents do. The lower our advantage, the more urgent the site is.
				iValue *= 120 + iDelta/2 + (kSet.bAmbitious ? 5 : 0);
				iValue /= 100;
				/*  <advc.031> Don't rush to settle marginal spots (which might
					not even make the MinFoundValue cut w/o the boost above). */
				if(iTmp < 2000) {
					iValue *= iTmp;
					iValue /= 2000;
					iValue = std::max(iTmp, iValue);
				} // </advc.031>
			}
			iDelta -= kSet.bEasyCulture ? 20 : 10;
			if (iDelta > 0)
			{
				iValue *= 100 - iDelta*3/2;
				iValue /= 100;
			}
		}
	// K-Mod end (the rest of this block existed in the original code - but I've made some edits...)

		if (pNearestCity != NULL)
		{
			int iDistance = plotDistance(iX, iY, pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE());
			int iNumCities = getNumCities();
			/* original bts code
			if (iDistance > 5)
			{
				iValue -= (iDistance - 5) * 500;
			}
			else if (iDistance < 4)
			{
				iValue -= (4 - iDistance) * 2000;
			}
			iValue *= (8 + iNumCities * 4);
			iValue /= (2 + (iNumCities * 4) + iDistance);

			if (pNearestCity->isCapital())
			{
				iValue *= 150;
				iValue /= 100;
			}
			else if (getCapitalCity() != NULL) */
			// K-Mod.
			// Close cities are penalised in other ways
			int iTargetRange = (kSet.bExpansive ? 6 : 5);
			if (iDistance > iTargetRange)
			{
				iValue -= std::min(5, iDistance - iTargetRange) * 400; // with that max distance, we could fit a city in the middle!
			}
			iValue *= 8 + 4*iNumCities;
			iValue /= 2 + 4*iNumCities + std::max(5, iDistance); // 5, not iTargetRange, because 5 is better.

			if (!pNearestCity->isCapital() && pCapital)
			// K-Mod end
			{
				//Provide up to a 50% boost to value (80% for adv.start)
				//for city sites which are relatively close to the core
				//compared with the most distance city from the core
				//(having a boost rather than distance penalty avoids some distortion)

				//This is not primarly about maitenance but more about empire 
				//shape as such forbidden palace/state property are not big deal.
				int iDistanceToCapital = plotDistance(pCapital->plot(), pPlot);

				FAssert(iMaxDistanceFromCapital > 0);
				/* original bts code
				iValue *= 100 + (((bAdvancedStart ? 80 : 50) * std::max(0, (iMaxDistanceFromCapital - iDistance))) / iMaxDistanceFromCapital);
				iValue /= 100; */
				// K-Mod. just a touch of flavour. (note, for a long time this adjustment used iDistance instead of iDistanceToCaptial; and so I've reduced the scale to compensate)
				int iShapeWeight = bAdvancedStart ? 50 : (kSet.bAmbitious ? 15 : 30);
				iValue *= 100 + iShapeWeight * std::max(0, iMaxDistanceFromCapital - iDistanceToCapital) / iMaxDistanceFromCapital;
				iValue /= 100 + iShapeWeight;
				// K-Mod end
			}
		}
		else
		{
			pNearestCity = GC.getMapINLINE().findCity(iX, iY, getID(), getTeam(), false);
			if (pNearestCity != NULL)
			{
				int iDistance = // advc.031:
						std::min(GC.getMapINLINE().maxMaintenanceDistance(),
						plotDistance(iX, iY, pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE()));
				iValue -= std::min(500 * iDistance, (//8000
						std::max(8000 - getCurrentEra() * 1000, 4000) // advc.031
						* iDistance) / GC.getMapINLINE().maxPlotDistance());
			}
		}
	}

	if (iValue <= 0)
	{
		return 1;
	}
  if(!GET_TEAM(getTeam()).isCapitulated()) { // advc.130v
	if (pArea->countCivCities() == 0) // advc.031: Had been counting barb cities
	{
		//iValue *= 2;
		// K-Mod: presumably this is meant to be a bonus for being the first on a new continent.
		// But I don't want it to be a bonus for settling on tiny islands, so I'm changing it.
		iValue *= range(100 * (pArea->//getNumTiles()
				getNumRevealedTiles(getTeam()) // advc.031: Don't cheat
				- 15) / //15
				20 // req. 40 revealed tiles for the full bonus
				, 100, 200);
		iValue /= 100;
		// K-Mod end
	}
	else
	{
		int iTeamAreaCities = GET_TEAM(getTeam()).countNumCitiesByArea(pArea);
		/*  advc.040: Don't let a single rival city make all the difference.
			Btw, the AI shouldn't magically know iTeamAreaCities. This entire
			"stick to your continent" block should perhaps be removed. */
		if(10*pArea->getNumCities() > 7*iTeamAreaCities)
		//if (pArea->getNumCities() == iTeamAreaCities)
		{	// advc.040: Why care about barb cities? And times 3/2 is huge.
			/*iValue *= 3;
			iValue /= 2;
		}
		else if (pArea->getNumCities() == (iTeamAreaCities + GET_TEAM(BARBARIAN_TEAM).countNumCitiesByArea(pArea)))
		{*/
			iValue *= 5; // advc.031: was *=4 and /=3
			iValue /= 4;
		}
		else if (iTeamAreaCities > 0
				/*  advc.040: One city isn't a good enough reason for
					discriminating against new colonies */
				&& AI_isPrimaryArea(pArea))
		{
			//iValue *= 5;
			//iValue /= 4;
			; // advc.031: Instead reduce the value in the else branch
		}
		else iValue = ::round(iValue * 0.8); // advc.031
	}
  } // </advc.130v>
	if (!kSet.bStartingLoc)
	{
		int iFoodSurplus = std::max(0, iSpecialFoodPlus - iSpecialFoodMinus);
		int iFoodDeficit = std::max(0, iSpecialFoodMinus - iSpecialFoodPlus);

		/* original bts code
		iValue *= 100 + 20 * std::max(0, std::min(iFoodSurplus, 2 * GC.getFOOD_CONSUMPTION_PER_POPULATION()));
		iValue /= 100 + 20 * std::max(0, iFoodDeficit); */
		// K-Mod. (note that iFoodSurplus and iFoodDeficit already have the "max(0, x)" built in.
		iValue *= 100 + (kSet.bExpansive ? 20 : 15) * std::min(
				(iFoodSurplus + iSpecialFoodPlus)/2,
				2 * GC.getFOOD_CONSUMPTION_PER_POPULATION());
		iValue /= 100 + (kSet.bExpansive ? 20 : 15) * iFoodDeficit;
		// K-Mod end
	}

	if (!kSet.bStartingLoc && getNumCities() > 0)
	{
		int iBonusCount = 0;
		int iUniqueBonusCount = 0;
		for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
		{
			iBonusCount += viBonusCount[iI];
			iUniqueBonusCount += (viBonusCount[iI] > 0) ? 1 : 0;
		}
		if (iBonusCount > 4)
		{
			iValue *= 5;
			iValue /= (1 + iBonusCount);
		}
		else if (iUniqueBonusCount > 2)
		{
			iValue *= 5;
			iValue /= (3 + iUniqueBonusCount);
		} // <advc.031> When there's nothing special about it 
		else if(iResourceValue <= 0 && iSpecialFoodPlus <= 0 && iRiver < 4 &&
				unrev < 5) // advc.040
			iValue = ::round(0.62 * iValue); // </advc.031>
	}

	if (!kSet.bStartingLoc)
	{
		int iDeadLockCount = AI_countDeadlockedBonuses(pPlot);
		if (bAdvancedStart && (iDeadLockCount > 0))
		{
			iDeadLockCount += 2;
		}
		iValue /= (1 + iDeadLockCount);
	}

	int subtr = (isBarbarian() ? 2 : (NUM_CITY_PLOTS / 4)); // advc.303
	iBadTile += unrev / 2; // advc.040
	// <advc.031>
	iBadTile -= subtr;
	if(iBadTile > 0) {
		iValue -= ::round(std::pow((double)iBadTile, 1.33) * 275);
		iValue = std::max(0, iValue);
		// Earlier attempt: division by root
		/*double div = std::pow((double)std::max(0, iBadTile), 0.67) + 3;
		iValue = ::round(iValue / div);*/
	} // </advc.031>
	/* original bts code
	if (kSet.bStartingLoc)
	{
		int iDifferentAreaTile = 0;

		for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			CvPlot* pLoopPlot = plotCity(iX, iY, iI);

			if ((pLoopPlot == NULL) || !(pLoopPlot->isWater() || pLoopPlot->area() == pArea))
			{
				iDifferentAreaTile++;
			}
		}

		iValue /= (std::max(0, (iDifferentAreaTile - ((NUM_CITY_PLOTS * 2) / 3))) + 2);
	} */ // disabled by K-Mod. This kind of stuff is already taken into account.
	// <advc.031>
	int bonusHealth = 0;
	if(getCapitalCity() != NULL)
		bonusHealth += getCapitalCity()->getBonusGoodHealth();
	int iBadHealth = -iHealth/100 - bonusHealth -
			GC.getHandicapInfo(getHandicapType()).getHealthBonus();
	if(iBadHealth >= -1) // I.e. can only grow once
		iValue /= std::max(1, (3 - getCurrentEra() + iBadHealth));
	// </advc.031>
	// K-Mod. Note: iValue is an int, but this function only return a short - so we need to be careful.
	FAssert(iValue >= 0);
	FAssert(iValue < MAX_SHORT);
	iValue = std::min(iValue, MAX_SHORT);
	// K-Mod end
	return std::max(1, iValue);
}


bool CvPlayerAI::AI_isAreaAlone(CvArea* pArea) const
{
	return ((pArea->getNumCities() - GET_TEAM(BARBARIAN_TEAM).countNumCitiesByArea(pArea)) == GET_TEAM(getTeam()).countNumCitiesByArea(pArea));
}


bool CvPlayerAI::AI_isCapitalAreaAlone() const
{
	CvCity* pCapitalCity;

	pCapitalCity = getCapitalCity();

	if (pCapitalCity != NULL)
	{
		return AI_isAreaAlone(pCapitalCity->area());
	}

	return false;
}


bool CvPlayerAI::AI_isPrimaryArea(CvArea* pArea) const
{
	if (pArea->isWater())
	{
		return false;
	}

	if (pArea->getCitiesPerPlayer(getID()) > 2)
	{
		return true;
	}

	CvCity* pCapitalCity = getCapitalCity(); // advc.003

	if (pCapitalCity != NULL)
	{
		if (pCapitalCity->area() == pArea)
		{
			return true;
		}
	}

	return false;
}


int CvPlayerAI::AI_militaryWeight(CvArea* pArea) const
{
	//return (pArea->getPopulationPerPlayer(getID()) + pArea->getCitiesPerPlayer(getID()) + 1);
	// K-Mod. If pArea == NULL, count all areas.
	return pArea
		? pArea->getPopulationPerPlayer(getID()) + pArea->getCitiesPerPlayer(getID()) + 1
		: getTotalPopulation() + getNumCities() + 1;
	// K-Mod end
}

// This function has been edited by Mongoose, then by jdog5000, and then by me (karadoc). Some changes are marked, others are not.
int CvPlayerAI::AI_targetCityValue(CvCity* pCity, bool bRandomize, bool bIgnoreAttackers) const
{
	PROFILE_FUNC();

	FAssertMsg(pCity != NULL, "City is not assigned a valid value");

	//int iValue = 1 + pCity->getPopulation() * (50 + pCity->calculateCulturePercent(getID())) / 100;
	int iValue = 5 + pCity->getPopulation() * (100 + pCity->calculateCulturePercent(getID())) / 150; // K-Mod (to dilute the other effects)

	const CvPlayerAI& kOwner = GET_PLAYER(pCity->getOwnerINLINE());

	if (pCity->isCoastal())
	{
		iValue += 2;
	}

	// <advc.104d> Replacing the BBAI code below (essentially with K-Mod code)
	iValue += cityWonderVal(pCity);
	/*iValue += 4*pCity->getNumActiveWorldWonders();

	for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		if (pCity->isHolyCity((ReligionTypes)iI))
		{
			iValue += 2 + ((GC.getGameINLINE().calculateReligionPercent((ReligionTypes)iI)) / 5);

			if (kOwner.getStateReligion() == iI)
			{
				iValue += 2;
			}
			if( getStateReligion() == iI )
			{
				iValue += 8;
			}
		}
	}*/ // </advc.104d>
	// <cdtw.2>
	if(pCity->plot()->defenseModifier(pCity->getTeam(), false) <=
			GC.getCITY_DEFENSE_DAMAGE_HEAL_RATE()) {
		if(AI_isDoStrategy(AI_STRATEGY_AIR_BLITZ) || AI_isDoStrategy(AI_STRATEGY_LAND_BLITZ))
			iValue += 8;
		else if(AI_isDoStrategy(AI_STRATEGY_FASTMOVERS))
			iValue += 3;
		else iValue++;
	} // </cdtw.2>
	if (pCity->isEverOwned(getID()))
	{
		iValue += 4;

		if( pCity->getOriginalOwner() == getID() )
		{
			iValue += 2;
		}
	}

	if (!bIgnoreAttackers)
	{
		iValue += std::min( 8, (AI_adjacentPotentialAttackers(pCity->plot()) + 2)/3 );
	}

	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = plotCity(pCity->getX_INLINE(), pCity->getY_INLINE(), iI);

		if (pLoopPlot != NULL)
		{
			if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
			{
				iValue += std::max(1, AI_bonusVal(pLoopPlot->getBonusType(getTeam()), 1, true)/5);
			}

			if (pLoopPlot->getOwnerINLINE() == getID())
			{
				iValue++;
			}

			if (pLoopPlot->isAdjacentPlayer(getID(), true))
			{
				iValue++;
			}
		}
	}

	if( kOwner.AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) )
	{
		if( pCity->getCultureLevel() >= (GC.getGameINLINE().culturalVictoryCultureLevel() - 1) )
		{
			iValue += 15;
			
			if( kOwner.AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4) )
			{
				iValue += 25;

				 if (pCity->getCultureLevel() >= (GC.getGameINLINE().culturalVictoryCultureLevel()) ||
					 pCity->findCommerceRateRank(COMMERCE_CULTURE) <= GC.getGameINLINE().culturalVictoryNumCultureCities()) // K-Mod
				{
					iValue += 60; // was 10
				}
			}
		}
	}

	if( kOwner.AI_isDoVictoryStrategy(AI_VICTORY_SPACE3) )
	{
		if( pCity->isCapital() )
		{
			iValue += 10;

			if( kOwner.AI_isDoVictoryStrategy(AI_VICTORY_SPACE4) )
			{
				iValue += 10; // was 20

				if( GET_TEAM(pCity->getTeam()).getVictoryCountdown(GC.getGameINLINE().getSpaceVictory()) >= 0 )
				{
					iValue += 100; // was 30
				}
			}
		}
	}

	CvCity* pNearestCity = GC.getMapINLINE().findCity(pCity->getX_INLINE(), pCity->getY_INLINE(), getID());

	if (pNearestCity != NULL)
	{
		// Now scales sensibly with map size, on large maps this term was incredibly dominant in magnitude
		int iTempValue = 30;
		iTempValue *= std::max(1, ((GC.getMapINLINE().maxStepDistance() * 2) - GC.getMapINLINE().calculatePathDistance(pNearestCity->plot(), pCity->plot())));
		iTempValue /= std::max(1, (GC.getMapINLINE().maxStepDistance() * 2));

		iValue += iTempValue;
	}

	if (bRandomize)
	{
		iValue += GC.getGameINLINE().getSorenRandNum(((pCity->getPopulation() / 2) + 1), "AI Target City Value");
	}

	// K-Mod
	if (pCity->getHighestPopulation() < 1)
	{
		// Usually this means the city would be auto-razed.
		// (We can't use isAutoRaze for this, because that assumes the city is already captured.)
		iValue = (iValue +2)/3;
	}
	// K-Mod end

	return iValue;
}

/*  <advc.104d> Adopted from K-Mod's AI_warSpoilsValue. I'm not calling
	cityWonderVal from there though b/c advc.104 bypasses startWarVal,
	i.e. AI_warSpoilsValue is obsolete. Also, AI_warSpoilsVal is a team-level
	function, but I need this to be a civ-level function so that the
	city owner's state religion can be factored in. */
int CvPlayerAI::cityWonderVal(CvCity* cp) const {

	if(cp == NULL) return 0;
	CvCity const& c = *cp;
	CvGame& g = GC.getGameINLINE();
	int r = 0;
	for(int i = 0; i < GC.getNumReligionInfos(); i++) {
		ReligionTypes rel = (ReligionTypes)i;
		if(c.isHolyCity(rel)) {
			int nRelCities = g.countReligionLevels(rel);
			/*  Account for cities we may still convert to our state religion.
				Hard to estimate. If we have a lot of cities, some of them may
				still not have the religion. Or if they do, we can use these
				cities to build missionaries to convert foreign cities. */
			if(getStateReligion() == rel)
				nRelCities += ::round(getNumCities() / 2.5);
			/* K-Mod comment: "the -4 at the end is mostly there to offset the
				'wonder' value that will be added later. I don't want to double-count
				the value of the shrine, and the religion [the holy city?]
				without the shrine isn't worth much anyway." */
			r += std::max(0, nRelCities / (c.hasShrine(rel) ? 1 : 2) - 4);
		}
	}
	for(int i = 0; i < GC.getNumCorporationInfos(); i++) {
		CorporationTypes corp = (CorporationTypes)i;
		if(c.isHeadquarters(corp))
			r += std::max(0, 2 * g.countCorporationLevels(corp) - 4);
	}
	r += 4 * c.getNumActiveWorldWonders(getID());
	return r;
}// </advc.104d>

CvCity* CvPlayerAI::AI_findTargetCity(CvArea* pArea) const
{
	CvCity* pLoopCity;
	CvCity* pBestCity;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;

	iBestValue = 0;
	pBestCity = NULL;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (isPotentialEnemy(getTeam(), GET_PLAYER((PlayerTypes)iI).getTeam()))
			{
				for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
				{
					if (pLoopCity->area() == pArea)
					{
						iValue = AI_targetCityValue(pLoopCity, true);

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestCity = pLoopCity;
						}
					}
				}
			}
		}
	}

	return pBestCity;
}


bool CvPlayerAI::AI_isCommercePlot(CvPlot* pPlot) const
{
	return (pPlot->getYield(YIELD_FOOD) >= GC.getFOOD_CONSUMPTION_PER_POPULATION());
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* General AI, Efficiency                                                                       */
/************************************************************************************************/
// Plot danger cache

// The vast majority of checks for plot danger are boolean checks during path planning for non-combat
// units like workers, settlers, and GP.  Since these are simple checks for any danger they can be 
// cutoff early if danger is found.  To this end, the two caches tracked are for whether a given plot
// is either known to be safe for the player who is currently moving, or for whether the plot is
// known to be a plot bordering an enemy of this team and therefore unsafe.
//
// The safe plot cache is only for the active moving player and is only set if this is not a
// multiplayer game with simultaneous turns.  The safety cache for all plots is reset when the active
// player changes or a new game is loaded.
// 
// The border cache is done by team and works for all game types.  The border cache is reset for all
// plots when war or peace are declared, and reset over a limited range whenever a ownership over a plot
// changes.

// K-Mod. The cache also needs to be reset when routes are destroyed, because distance 2 border danger only counts when there is a route.
// Actually, the cache doesn't need to be cleared when war is declared; because false negatives have no impact with this cache.
// The safe plot cache can be invalid if we kill an enemy unit. Currently this is unaccounted for, and so the cache doesn't always match the true state.
// In general, I think this cache is a poorly planned idea. It's prone to subtle bugs if there are rule changes in seemingly independent parts of the games.
//
// I've done a bit of speed profiling and found that although the safe plot cache does shortcut around 50% of calls to AI_getAnyPlotDanger,
// that only ends up saving a few milliseconds each turn anyway. I don't really think that's worth risking of getting problems from bad cache.
// So even though I've put a bit of work into make the cache work better, I'm just going to disable it.

bool CvPlayerAI::isSafeRangeCacheValid() const
{
	return false; // Cache disabled. See comments above.
	//return isTurnActive() && !GC.getGameINLINE().isMPOption(MPOPTION_SIMULTANEOUS_TURNS) && GC.getGameINLINE().getNumGameTurnActive() == 1;
}

bool CvPlayerAI::AI_getAnyPlotDanger(CvPlot* pPlot, int iRange, bool bTestMoves, bool bCheckBorder) const
{
	PROFILE_FUNC();

	if (iRange == -1)
	{
		iRange = DANGER_RANGE;
	}

	/* bbai if( bTestMoves && isTurnActive() )
	{
		if( (iRange <= DANGER_RANGE) && pPlot->getActivePlayerNoDangerCache() )
		{
			return false;
		}
	} */
	// K-Mod
	if (bTestMoves && isSafeRangeCacheValid() && iRange <= pPlot->getActivePlayerSafeRangeCache())
		return false;
	// K-Mod end

	TeamTypes eTeam = getTeam();
	//bool bCheckBorder = (!isHuman() && !pPlot->isCity());
	// K-Mod. I don't want auto-workers on the frontline. Cities need to be excluded for some legacy AI code. (cf. condition in AI_getPlotDanger)
	bCheckBorder = bCheckBorder && !pPlot->isCity() && (!isHuman() || pPlot->plotCount(PUF_canDefend, -1, -1, getID(), NO_TEAM) == 0);
	// K-Mod end

	
	if( bCheckBorder )
	{
		//if( (iRange >= DANGER_RANGE) && pPlot->isTeamBorderCache(eTeam) )
		if (iRange >= BORDER_DANGER_RANGE && pPlot->getBorderDangerCache(eTeam)) // K-Mod. border danger doesn't count anything further than range 2.
		{
			return true;
		}
	}

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pLoopPlot;
	int iDistance;
	int iDX, iDY;
	CvArea *pPlotArea = pPlot->area();
	int iDangerRange;

	for (iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (iDY = -(iRange); iDY <= iRange; iDY++)
		{
			pLoopPlot	= plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{	// <advc.030>
				CvArea const& fromArea = *pLoopPlot->area();
				if(pPlotArea->canBeEntered(fromArea)) // Replacing:
				//if (pLoopPlot->area() == pPlotArea) // </advc.030>
				{
				    iDistance = stepDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
				    if( bCheckBorder
							// advc.030: For CheckBorder, it's a helpful precondition.
							&& pLoopPlot->area() == pPlotArea)
					{
						if (atWar(pLoopPlot->getTeam(), eTeam))
						{
							/* original bbai code
							// Border cache is reversible, set for both team and enemy.
							if (iDistance == 1)
							{
								pPlot->setBorderDangerCache(eTeam, true);
								pPlot->setBorderDangerCache(pLoopPlot->getTeam(), true);
								pLoopPlot->setBorderDangerCache(eTeam, true);
								pLoopPlot->setBorderDangerCache(pLoopPlot->getTeam(), true);
								return true;
							}
							else if ((iDistance == 2) && (pLoopPlot->isRoute()))
							{
								pPlot->setBorderDangerCache(eTeam, true);
								pPlot->setBorderDangerCache(pLoopPlot->getTeam(), true);
								pLoopPlot->setBorderDangerCache(eTeam, true);
								pLoopPlot->setBorderDangerCache(pLoopPlot->getTeam(), true);
								return true;
							} */
							// K-Mod. reversible my arse.
							if (iDistance == 1)
							{
								pPlot->setBorderDangerCache(eTeam, true);
								pLoopPlot->setBorderDangerCache(eTeam, true); // pLoopPlot is in enemy territory, so this is fine.
								if (pPlot->getTeam() == eTeam) // only set the cache for the pLoopPlot team if pPlot is owned by us! (ie. owned by their enemy)
								{
									pLoopPlot->setBorderDangerCache(pLoopPlot->getTeam(), true);
									pPlot->setBorderDangerCache(pLoopPlot->getTeam(), true);
								}
								return true;
							}
							else if (iDistance == 2 && pLoopPlot->isRoute())
							{
								pPlot->setBorderDangerCache(eTeam, true);
								pLoopPlot->setBorderDangerCache(eTeam, true); // owned by our enemy
								if (pPlot->isRoute() && pPlot->getTeam() == eTeam)
								{
									pLoopPlot->setBorderDangerCache(pLoopPlot->getTeam(), true);
									pPlot->setBorderDangerCache(pLoopPlot->getTeam(), true);
								}
								return true;
							}
							// K-Mod end
						}
					}

					pUnitNode = pLoopPlot->headUnitNode();

					while (pUnitNode != NULL)
					{
						pLoopUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

						// No need to loop over tiles full of our own units
						if( pLoopUnit->getTeam() == eTeam )
						{
							if( !(pLoopUnit->alwaysInvisible()) && (pLoopUnit->getInvisibleType() == NO_INVISIBLE) )
							{
								break;
							}
						}

						if (pLoopUnit->isEnemy(eTeam))
						{
							if (pLoopUnit->canAttack())
							{
								if (!(pLoopUnit->isInvisible(eTeam, false)))
								{ /* advc.030, advc.001k: Replacing the line below
									 (which calls isMadeAttack) */
									if(pPlotArea->canBeEntered(fromArea, pLoopUnit))
								    //if (pLoopUnit->canMoveOrAttackInto(pPlot))
									{
                                        if (!bTestMoves)
                                        {
                                            return true;
                                        }
                                        else
                                        {
                                            iDangerRange = pLoopUnit->baseMoves();
                                            iDangerRange += ((pLoopPlot->isValidRoute(pLoopUnit)) ? 1 : 0);
                                            if (iDangerRange >= iDistance)
											{
												return true;
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


	// The test moves case is a strict subset of the more general case,
	// either is appropriate for setting the cache.  However, since the test moves
	// case is called far more frequently, it is more important and the cache 
	// value being true is only assumed to mean that the plot is safe in the
	// test moves case.
	//if( bTestMoves )
	/* bbai code
	{
		if( isTurnActive() )
		{
			if( !(GC.getGameINLINE().isMPOption(MPOPTION_SIMULTANEOUS_TURNS)) && (GC.getGameINLINE().getNumGameTurnActive() == 1) )
			{
				pPlot->setActivePlayerNoDangerCache(true);
			}
		}
	} */
	// K-Mod. The above bbai code is flawed in that it flags the plot as safe regardless
	// of what iRange is and then reports that the plot is safe for any iRange <= DANGER_RANGE.
	if (isSafeRangeCacheValid() && iRange > pPlot->getActivePlayerSafeRangeCache())
		pPlot->setActivePlayerSafeRangeCache(iRange);
	// K-Mod end

	return false;
}

// <advc.104> For sorting in AI_getPlotDanger
bool byDamage(CvUnit* left, CvUnit* right) {
	return left->getDamage() < right->getDamage();
} // </advc.104>

int CvPlayerAI::AI_getPlotDanger(CvPlot* pPlot, int iRange, bool bTestMoves
	// advc.104:
	, bool bCheckBorder, int* lowHealth, int hpLimit, int limitCount, PlayerTypes enemyId
	) const
{
	PROFILE_FUNC();
	// <advc.104>
	if(limitCount == 0 || hpLimit <= 0)
		return 0;
	if(lowHealth != NULL)
		*lowHealth = 0; // </advc.104>
	// <advc.003> Initialization added or definition moved
	CLLNode<IDInfo>* pUnitNode=NULL;
	CvUnit* pLoopUnit=NULL;
	CvPlot* pLoopPlot=NULL;
	int iCount=0;
	int iBorderDanger=0;
	int iDX=-1, iDY=-1; // </advc.003>
	CvArea *pPlotArea = pPlot->area();
	TeamTypes eTeam = getTeam();

	if (iRange == -1)
	{
		iRange = DANGER_RANGE;
	}

	/* bbai
	if( bTestMoves && isTurnActive() )
	{
		if( (iRange <= DANGER_RANGE) && pPlot->getActivePlayerNoDangerCache() )
		{
			return 0;
		}
	} */
	// K-Mod
	if (bTestMoves && isSafeRangeCacheValid() && iRange <= pPlot->getActivePlayerSafeRangeCache())
		return 0;
	// K-Mod end

	for (iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (iDY = -(iRange); iDY <= iRange; iDY++)
		{
			pLoopPlot	= plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{	// <advc.030>
				CvArea const& fromArea = *pLoopPlot->area();
				if(pPlotArea->canBeEntered(fromArea)) // Replacing:
				//if (pLoopPlot->area() == pPlotArea) // </advc.030>
				{	// <avc.030> Moved up
					int iDistance = stepDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(),
							pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
					// </advc.030>
					if(bCheckBorder // advc.104
							&& pLoopPlot->area() == pPlotArea) { // advc.030
						if (atWar(pLoopPlot->getTeam(), eTeam))
						{
							if (iDistance == 1)
							{
								iBorderDanger++;
							}
							else if ((iDistance == 2) && (pLoopPlot->isRoute()))
							{
								iBorderDanger++;
							}
						}
					} // advc.104

					pUnitNode = pLoopPlot->headUnitNode();
					std::vector<CvUnit*> plotUnits; // advc.104
					while (pUnitNode != NULL)
					{
						pLoopUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);
						// No need to loop over tiles full of our own units
						if( pLoopUnit->getTeam() == eTeam )
						{
							if( !(pLoopUnit->alwaysInvisible()) && (pLoopUnit->getInvisibleType() == NO_INVISIBLE) )
							{
								break;
							}
						}
						// <advc.003> Some indentation removed
						if (pLoopUnit->isEnemy(eTeam) && pLoopUnit->canAttack() &&
								!pLoopUnit->isInvisible(eTeam, false)) {
							// </advc.003>
							/*  advc.030, advc.001k: Replacing the line below
								(which calls isMadeAttack) */
							if(pPlotArea->canBeEntered(fromArea, pLoopUnit))
							//if (pLoopUnit->canMoveOrAttackInto(pPlot))
							{	// <advc.104>
								if(enemyId == NO_PLAYER || pLoopUnit->getOwnerINLINE() == enemyId)
									plotUnits.push_back(pLoopUnit);
							}
						}
					}
					// Don't waste time with this otherwise
					if(lowHealth != NULL && !plotUnits.empty())
						std::sort(plotUnits.begin(), plotUnits.end(), byDamage);
					for(size_t i = 0; i < plotUnits.size(); i++) {
						pLoopUnit = plotUnits[i];
						bool countIncreased = false; // </advc.104>
						if (!bTestMoves)
						{
							iCount++;
							countIncreased = true; // advc.104
						}
						else
						{
							int iDangerRange = pLoopUnit->baseMoves();
							iDangerRange += ((pLoopPlot->isValidRoute(pLoopUnit)) ? 1 : 0);
							if (iDangerRange >= iDistance)
							{
								iCount++;
								countIncreased = true; // advc.104
							}
						}
						// <advc.104>
						if(countIncreased) {
							if(lowHealth != NULL && pLoopUnit->maxHitPoints() -
									pLoopUnit->getDamage() <= hpLimit)
								(*lowHealth)++;
							if(limitCount > 0 && iCount >= limitCount)
								return iCount;
						} // </advc.104>
					}
				}
			}
		}
	}
	//}}}} // advc.104: These closing braces have moved up
	// K-Mod
	if (iCount == 0 && isSafeRangeCacheValid() && iRange > pPlot->getActivePlayerSafeRangeCache())
		pPlot->setActivePlayerSafeRangeCache(iRange);
	// K-Mod end

	if (iBorderDanger > 0)
	{
	    /* original bts code
		if (!isHuman() && !pPlot->isCity())
	    {
            iCount += iBorderDanger;
	    } */
		// K-Mod. I don't want auto-workers on the frontline. So count border danger for humans too, unless the plot is defended.
		// but on the other hand, I don't think two border tiles are really more dangerous than one border tile.
		// (cf. condition used in AI_anyPlotDanger. Note that here we still count border danger in cities - because I want it for AI_cityThreat)
		if (!isHuman() || pPlot->plotCount(PUF_canDefend, -1, -1, getID(), NO_TEAM) == 0)
			iCount++;
		// K-Mod end
	}

	return iCount;
}

// Never used ...
/*
int CvPlayerAI::AI_getUnitDanger(CvUnit* pUnit, int iRange, bool bTestMoves, bool bAnyDanger) const
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pLoopPlot;
	int iCount;
	int iDistance;
	int iBorderDanger;
	int iDX, iDY;

    CvPlot* pPlot = pUnit->plot();
	iCount = 0;
	iBorderDanger = 0;

	if (iRange == -1)
	{
		iRange = DANGER_RANGE;
	}
	
	for (iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (iDY = -(iRange); iDY <= iRange; iDY++)
		{
			pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->area() == pPlot->area())
				{
				    iDistance = stepDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
				    if (atWar(pLoopPlot->getTeam(), getTeam()))
				    {
				        if (iDistance == 1)
				        {
				            iBorderDanger++;
				        }
				        else if ((iDistance == 2) && (pLoopPlot->isRoute()))
				        {
				            iBorderDanger++;
				        }
				    }


					pUnitNode = pLoopPlot->headUnitNode();

					while (pUnitNode != NULL)
					{
						pLoopUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

						if (atWar(pLoopUnit->getTeam(), getTeam()))
						{
							if (pLoopUnit->canAttack())
							{
								if (!(pLoopUnit->isInvisible(getTeam(), false)))
								{
								    if (pLoopUnit->canMoveOrAttackInto(pPlot))
								    {
                                        if (!bTestMoves)
                                        {
                                            iCount++;
                                        }
                                        else
                                        {
                                            iDangerRange = pLoopUnit->baseMoves();
                                            iDangerRange += ((pLoopPlot->isValidRoute(pLoopUnit)) ? 1 : 0);
                                            if (iDangerRange >= iDistance)
                                            {
                                                iCount++;
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

	if (iBorderDanger > 0)
	{
	    if (!isHuman() || pUnit->isAutomated())
	    {
            iCount += iBorderDanger;
	    }
	}

	return iCount;
}
*/
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


int CvPlayerAI::AI_getWaterDanger(CvPlot* pPlot, int iRange, bool bTestMoves) const
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pLoopPlot;
	int iCount;
	int iDX, iDY;

	iCount = 0;

	if (iRange == -1)
	{
		iRange = DANGER_RANGE;
	}
	
	CvArea* pWaterArea = pPlot->waterArea();

	for (iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (iDY = -(iRange); iDY <= iRange; iDY++)
		{
			pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->isWater())
				{
					if (pPlot->isAdjacentToArea(pLoopPlot->getArea()))
					{
						pUnitNode = pLoopPlot->headUnitNode();

						while (pUnitNode != NULL)
						{
							pLoopUnit = ::getUnit(pUnitNode->m_data);
							pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

							if (pLoopUnit->isEnemy(getTeam()))
							{
								if (pLoopUnit->canAttack())
								{
									if (!(pLoopUnit->isInvisible(getTeam(), false)))
									{
										iCount++;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	
	return iCount;
}

// rewritten for K-Mod
bool CvPlayerAI::AI_avoidScience() const
{
	if (isCurrentResearchRepeat() && (!isHuman() || getCommercePercent(COMMERCE_RESEARCH) <= 20))
		return true;

	if (isNoResearchAvailable())
		return true;

	return false;
}


// XXX
bool CvPlayerAI::AI_isFinancialTrouble() const
{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/12/09                                jdog5000      */
/*                                                                                              */
/* Barbarian AI                                                                                 */
/************************************************************************************************/
	if( isBarbarian() )
	{
		return false;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	//if (getCommercePercent(COMMERCE_GOLD) > 50)
	{
		//int iNetCommerce = 1 + getCommerceRate(COMMERCE_GOLD) + getCommerceRate(COMMERCE_RESEARCH) + std::max(0, getGoldPerTurn());
		//int iNetExpenses = calculateInflatedCosts() + std::min(0, getGoldPerTurn());
		int iNetCommerce = AI_getAvailableIncome(); // K-Mod
		int iNetExpenses = calculateInflatedCosts() + std::max(0, -getGoldPerTurn()); // unofficial patch
		
		int iFundedPercent = (100 * (iNetCommerce - iNetExpenses)) / std::max(1, iNetCommerce);
		int iSafePercent = 35; // was 40

		// <advc.110>
        /* Increase surplus required for a stable/ "safe" economy, and base
           the threshold on the current era (K-Mod: 35% for all eras). */
        iSafePercent = 65; // Ancient, Classical era
        int e = getCurrentEra();
        if(e > 1)
            iSafePercent -= 10 * e; // 55, 45, ... --> Medieval, Renaissance, ...
		// </advc.110>
		
		if (AI_avoidScience())
		{
			iSafePercent -= 8;
		}
		
		if(isFocusWar()) // advc.105
		//if (GET_TEAM(getTeam()).getAnyWarPlanCount(true))
		{
			iSafePercent -= 10; // was 12
		}
		
		if (isCurrentResearchRepeat())
		{
			iSafePercent -= 10;
		}

		// K-Mod
		int iCitiesTarget = std::max(1, GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities());
		if (getNumCities() < iCitiesTarget)
		{
			// note: I'd like to use (AI_getNumCitySites() > 0) as well, but that could potentially cause the site selection to oscillate.
			iSafePercent -= 14 * (iCitiesTarget - getNumCities()) / iCitiesTarget;
		}
		// K-Mod end
		
		if (iFundedPercent < iSafePercent)
		{
			return true;
		}
	}

	return false;
}

// This function has been heavily edited for K-Mod
int CvPlayerAI::AI_goldTarget(bool bUpgradeBudgetOnly) const
{
	int iGold = 0;

	// K-Mod.
	int iUpgradeBudget = AI_getGoldToUpgradeAllUnits();

	if (iUpgradeBudget > 0)
	{
		const CvTeamAI& kTeam = GET_TEAM(getTeam());
		bool bAnyWar = isFocusWar(); // advc.105
				//kTeam.getAnyWarPlanCount(true) > 0;

		if (!bAnyWar)
		{
			iUpgradeBudget /= AI_isFinancialTrouble() || AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4) ? 10 : 4;
		}
		else
		{
			int iSuccessRating = kTeam.AI_getWarSuccessRating();
			if (iSuccessRating < -10 || (iSuccessRating < 10 && kTeam.getWarPlanCount(WARPLAN_ATTACKED_RECENT, true) > 0))
				iUpgradeBudget *= 2; // cf. iTargetTurns in AI_doCommerce
			else if (iSuccessRating > 50 || AI_isFinancialTrouble())
			{
				iUpgradeBudget /= 2;
			}
		}

		if (bUpgradeBudgetOnly)
			return iUpgradeBudget;

		iGold += iUpgradeBudget;
	}
	// K-Mod end

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       02/24/10                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                      */
/************************************************************************************************/
/* original bts code
	if (GC.getGameINLINE().getElapsedGameTurns() >= 40)
*/
	if (GC.getGameINLINE().getElapsedGameTurns() >= 40 || getNumCities() > 3)
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
	{
		/* original bts code
		int iMultiplier = 0;
		iMultiplier += GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getResearchPercent();
		iMultiplier += GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getTrainPercent();
		iMultiplier += GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getConstructPercent();
		iMultiplier /= 3;

		iGold += ((getNumCities() * 3) + (getTotalPopulation() / 3));

		iGold += (GC.getGameINLINE().getElapsedGameTurns() / 2);*/
		// K-mod. Does slower research mean we need to keep more gold? Does slower building?
		// Surely the raw turn count is the one that needs to be adjusted for speed!
		if (!AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4))
		{
			int iStockPile = 3*std::min(8, getNumCities()) + std::min(120, getTotalPopulation())/3;
			iStockPile += 100*GC.getGameINLINE().getElapsedGameTurns() / (2*GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getResearchPercent());
			if (AI_getFlavorValue(FLAVOR_GOLD) > 0)
			{
				iStockPile *= 10 + AI_getFlavorValue(FLAVOR_GOLD);
				iStockPile /= 8;
			}
			// note: currently the highest flavor_gold is 5.
			iGold += iStockPile;
		}
		// K-Mod end

		/*iGold *= iMultiplier;
		iGold /= 100;*/

		/* original bts code
		if (bAnyWar)
		{
			iGold *= 3;
			iGold /= 2;
		} */ // K-Mod. I don't think we need this anymore.

		if (AI_avoidScience())
		{
			iGold *= 3; // was 10
		}

		//iGold += (AI_getGoldToUpgradeAllUnits() / (bAnyWar ? 1 : 2)); // obsolete (K-Mod)

		/* original bts code
		CorporationTypes eActiveCorporation = NO_CORPORATION;
		for (int iI = 0; iI < GC.getNumCorporationInfos(); iI++)
		{
			if (getHasCorporationCount((CorporationTypes)iI) > 0)
			{
				eActiveCorporation = (CorporationTypes)iI;
				break;
			}
		}
		if (eActiveCorporation != NO_CORPORATION)
		{
			int iSpreadCost = std::max(0, GC.getCorporationInfo(eActiveCorporation).getSpreadCost() * (100 + calculateInflationRate()));
			iSpreadCost /= 50;
			iGold += iSpreadCost;
		} */
		// K-Mod
		int iSpreadCost = 0;

		if (!isNoCorporations())
		{
			for (CorporationTypes eCorp = (CorporationTypes)0; eCorp < GC.getNumCorporationInfos(); eCorp = (CorporationTypes)(eCorp+1))
			{
				if (getHasCorporationCount(eCorp) > 0)
				{
					int iExecs = countCorporationSpreadUnits(NULL, eCorp, true);
					if (iExecs > 0)
					{
						int iTempCost = GC.getCorporationInfo(eCorp).getSpreadCost();
						iTempCost *= iExecs + 1; // execs + 1 because actual spread cost can be higher than the base cost.
						iSpreadCost = std::max(iSpreadCost, iTempCost);
					}
				}
			}
		}
		if (iSpreadCost > 0)
		{
			iSpreadCost *= 100 + calculateInflationRate();
			iSpreadCost /= 100;
			iGold += iSpreadCost;
		}
		// K-Mod end
	}

	return iGold + AI_getExtraGoldTarget();
}

// <k146>
// Functors used by AI_bestTech. (I wish we had lambdas.)
template <typename A, typename B>
struct PairSecondEq : public std::binary_function<std::pair<A, B>,std::pair<A, B>,bool>
{
	PairSecondEq(B t) : _target(t) {}

	bool operator()(const std::pair<A, B>& o1)
	{
		return o1.second == _target;
	}
private:
	B _target;
};
template <typename A, typename B>
struct PairFirstLess : public std::binary_function<std::pair<A, B>,std::pair<A, B>,bool>
{
	bool operator()(const std::pair<A, B>& o1, const std::pair<A, B>& o2)
	{
		return o1.first < o2.first;
	}
}; // </k146>

// edited by K-Mod and BBAI
TechTypes CvPlayerAI::AI_bestTech(int iMaxPathLength, bool bFreeTech, bool bAsync, TechTypes eIgnoreTech, AdvisorTypes eIgnoreAdvisor) const
{
	PROFILE("CvPlayerAI::AI_bestTech");

	CvTeam& kTeam = GET_TEAM(getTeam());
	
	std::vector<int> viBonusClassRevealed(GC.getNumBonusClassInfos(), 0);
	std::vector<int> viBonusClassUnrevealed(GC.getNumBonusClassInfos(), 0);
	std::vector<int> viBonusClassHave(GC.getNumBonusClassInfos(), 0);
	// k146 (comment): Find make lists of which bonuses we have / don't have / can see. This is used for tech evaluation
	for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
	{
	    TechTypes eRevealTech = (TechTypes)GC.getBonusInfo((BonusTypes)iI).getTechReveal();
	    BonusClassTypes eBonusClass = (BonusClassTypes)GC.getBonusInfo((BonusTypes)iI).getBonusClassType();
	    if (eRevealTech != NO_TECH)
	    {
	        if ((kTeam.isHasTech(eRevealTech)))
	        {
	            viBonusClassRevealed[eBonusClass]++;
	        }
	        else
	        {
	            viBonusClassUnrevealed[eBonusClass]++;
	        }

            if (getNumAvailableBonuses((BonusTypes)iI) > 0)
            {
                viBonusClassHave[eBonusClass]++;                
            }
            else if (countOwnedBonuses((BonusTypes)iI) > 0)
            {
                viBonusClassHave[eBonusClass]++;
            }
	    }
	}

#ifdef DEBUG_TECH_CHOICES
	CvWString szPlayerName = getName();
	DEBUGLOG("AI_bestTech:%S\n", szPlayerName.GetCString());
#endif
	// k146: Commented out
	/*for (int iI = 0; iI < GC.getNumTechInfos(); iI++)
	{
		if ((eIgnoreTech == NO_TECH) || (iI != eIgnoreTech))
		{
			if ((eIgnoreAdvisor == NO_ADVISOR) || (GC.getTechInfo((TechTypes)iI).getAdvisorType() != eIgnoreAdvisor))
			{
				if (canEverResearch((TechTypes)iI))
				{
					if (!(kTeam.isHasTech((TechTypes)iI)))
					{
						if (GC.getTechInfo((TechTypes)iI).getEra() <= (getCurrentEra() + 1))
						{
							iPathLength = findPathLength(((TechTypes)iI), false);

							if (iPathLength <= iMaxPathLength)
							{
								iValue = AI_techValue( (TechTypes)iI, iPathLength, bIgnoreCost, bAsync, viBonusClassRevealed, viBonusClassUnrevealed, viBonusClassHave );

								if( gPlayerLogLevel >= 3 )
								{
									logBBAI("      Player %d (%S) consider tech %S with value %d", getID(), getCivilizationDescription(0), GC.getTechInfo((TechTypes)iI).getDescription(), iValue );
								}

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									eBestTech = ((TechTypes)iI);
								}
							}
						}
					}
				}
			}
		}
	}*/
	// <k146>
	/*  Instead of choosing a tech anywhere inside the max path length with no
		adjustments for how deep the tech is, we'll evaluate all techs inside
		the max path length; but instead of just picking the highest value one
		irrespective of depth, we'll use the evaluatations to add value to the
		prereq techs, and then choose the best depth 1 tech at the end. */
	std::vector<std::pair<int, TechTypes> > techs; // (value, tech) pairs)
	//std::vector<TechTypes> techs;
	//std::vector<int> values;
	// cumulative number of techs for each depth of the search. (techs_to_depth[0] == 0)
	std::vector<int> techs_to_depth;

	int iTechCount = 0;
	for (int iDepth = 0; iDepth < iMaxPathLength; ++iDepth)
	{
		techs_to_depth.push_back(iTechCount);

		for (TechTypes eTech = (TechTypes)0; eTech < GC.getNumTechInfos();
				eTech=(TechTypes)(eTech+1))
		{
			const CvTechInfo& kTech = GC.getTechInfo(eTech);
			const std::vector<std::pair<int,TechTypes> >::iterator
					// Evaluated techs before the current depth
					tech_search_end = techs.begin()+techs_to_depth[iDepth];
			if (eTech == eIgnoreTech)
				continue;
			if (eIgnoreAdvisor != NO_ADVISOR && kTech.getAdvisorType() == eIgnoreAdvisor)
				continue;
			if (!canEverResearch(eTech))
				continue;
			if (kTeam.isHasTech(eTech))
				continue;

			if (GC.getTechInfo(eTech).getEra() > (getCurrentEra() + 1))
				continue; // too far in the future to consider. (This condition is only for efficiency.)

			if (std::find_if(techs.begin(), tech_search_end, PairSecondEq<int, TechTypes>(eTech)) != tech_search_end)
				continue; // already evaluated

			// Check "or" prereqs
			bool bMissingPrereq = false;
			for (int p = 0; p < GC.getNUM_OR_TECH_PREREQS(); ++p)
			{
				TechTypes ePrereq = (TechTypes)kTech.getPrereqOrTechs(p);
				if (ePrereq != NO_TECH)
				{
					if (kTeam.isHasTech(ePrereq) || std::find_if(techs.begin(), tech_search_end, PairSecondEq<int, TechTypes>(ePrereq)) != tech_search_end)
					{
						bMissingPrereq = false; // we have a prereq
						break;
					}
					bMissingPrereq = true; // A prereq exists, and we don't have it.
				}
			}
			if (bMissingPrereq)
				continue; // We don't have any of the "or" prereqs

			// Check "and" prereqs
			for (int p = 0; p < GC.getNUM_AND_TECH_PREREQS(); ++p)
			{
				TechTypes ePrereq = (TechTypes)kTech.getPrereqAndTechs(p);
				if (ePrereq != NO_TECH)
				{
					if (!GET_TEAM(getTeam()).isHasTech(ePrereq) && std::find_if(techs.begin(), tech_search_end, PairSecondEq<int, TechTypes>(ePrereq)) == tech_search_end)
					{
						bMissingPrereq = true;
						break;
					}
				}
			}
			if (bMissingPrereq)
				continue; // We're missing at least one "and" prereq
			//

			// Otherwise, all the prereqs are either researched, or on our list from lower depths.
			// We're ready to evaluate this tech and add it to the list.
			int iValue = AI_techValue(eTech, iDepth+1, iDepth == 0 && bFreeTech, bAsync, viBonusClassRevealed, viBonusClassUnrevealed, viBonusClassHave);

			techs.push_back(std::make_pair(iValue, eTech));
			//techs.push_back(eTech);
			//values.push_back(iValue);
			++iTechCount;

			if (iDepth == 0 && gPlayerLogLevel >= 3)
			{
				logBBAI("      Player %d (%S) consider tech %S with value %d", getID(), getCivilizationDescription(0), GC.getTechInfo(eTech).getDescription(), iValue );
			}
		}
	}
	techs_to_depth.push_back(iTechCount); // We need this to ensure techs_to_depth[1] exists.

	FAssert(techs_to_depth.size() == iMaxPathLength+1);
	//FAssert(techs.size() == values.size());

#ifdef USE_OLD_TECH_STUFF
	bool bPathways = false && getID() < GC.getGameINLINE().countCivPlayersEverAlive()/2; // testing (temp)
	bool bNewWays = true || getID() < GC.getGameINLINE().countCivPlayersEverAlive()/2; // testing (temp)

	if (!bPathways)
	{
	/*  Ok. All techs have been evaluated up to the given search depth.
		Now we just have to add a percentage the deep tech values to their
		prereqs. First, lets calculate what the percentage should be!
		Note: the fraction compounds for each depth level.
		eg. 1, 1/3, 1/9, 1/27, etc. */
	if (iMaxPathLength > 1 && iTechCount > techs_to_depth[1])
	{
		int iPrereqPercent = bNewWays ? 50 : 0;
		iPrereqPercent += (AI_getFlavorValue(FLAVOR_SCIENCE) > 0) ? 5 +
				AI_getFlavorValue(FLAVOR_SCIENCE) : 0;
		iPrereqPercent += AI_isDoStrategy(AI_STRATEGY_ECONOMY_FOCUS) ? 10 : 0;
		iPrereqPercent += AI_isDoVictoryStrategy(AI_VICTORY_SPACE1) ? 5 : 0;
		iPrereqPercent += AI_isDoVictoryStrategy(AI_VICTORY_SPACE2) ? 10 : 0;
		iPrereqPercent += AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE) ? -5 : 0;
		iPrereqPercent += kTeam.getAnyWarPlanCount(true) > 0 ? -10 : 0;
		// more modifiers to come?

		iPrereqPercent = range(iPrereqPercent, 0, 80);

		/*  I figure that if I go through the techs in reverse order to add
			value to their prereqs, I don't double-count or miss anything.
			Is that correct? */
		int iDepth = iMaxPathLength-1;
		for (int i = iTechCount-1; i >= techs_to_depth[1]; --i)
		{
			const CvTechInfo& kTech = GC.getTechInfo(techs[i].second);

			if (i < techs_to_depth[iDepth])
			{
				--iDepth;
			}
			FAssert(iDepth > 0);

			/*  We only want to award points to the techs directly below this
				level. We don't want, for example, Chemistry getting points
				from Biology when we haven't researched Scientific Method. */
			const std::vector<std::pair<int,TechTypes> >::iterator
					prereq_search_begin = techs.begin()+techs_to_depth[iDepth-1];
			const std::vector<std::pair<int,TechTypes> >::iterator
					prereq_search_end = techs.begin()+techs_to_depth[iDepth];
			/*  Also; for the time being, I only want to add value to prereqs
				from the best following tech, rather than from all following
				techs. (The logic is that we will only research one thing at a
				time anyway; so although opening lots of options is good, we
				shouldn't overvalue it.) */
			std::vector<int> prereq_bonus(techs.size(), 0);

			FAssert(techs[i].first*iPrereqPercent/100 > 0 &&
					techs[i].first*iPrereqPercent/100 < 100000);

			for (int p = 0; p < GC.getNUM_OR_TECH_PREREQS(); ++p)
			{
				TechTypes ePrereq = (TechTypes)kTech.getPrereqOrTechs(p);
				if (ePrereq != NO_TECH)
				{
					std::vector<std::pair<int,TechTypes> >::iterator tech_it =
							std::find_if(prereq_search_begin, prereq_search_end,
							PairSecondEq<int,TechTypes>(ePrereq));
					if (tech_it != prereq_search_end)
					{
						const size_t prereq_i = tech_it - techs.begin();
						//values[index] += values[i]*iPrereqPercent/100;
						prereq_bonus[prereq_i] = std::max(prereq_bonus[prereq_i],
								techs[i].first*iPrereqPercent/100);

						if (gPlayerLogLevel >= 3)
						{
							logBBAI("      %S adds %d to %S (depth %d)",
									GC.getTechInfo(techs[i].second).
									getDescription(),
									techs[i].first*iPrereqPercent/100,
									GC.getTechInfo(techs[prereq_i].second).
									getDescription(), iDepth-1);
						}
					}
				}
			}
			for (int p = 0; p < GC.getNUM_AND_TECH_PREREQS(); ++p)
			{
				TechTypes ePrereq = (TechTypes)kTech.getPrereqAndTechs(p);
				if (ePrereq != NO_TECH)
				{
					std::vector<std::pair<int,TechTypes> >::iterator tech_it =
							std::find_if(prereq_search_begin, prereq_search_end,
							PairSecondEq<int, TechTypes>(ePrereq));
					if (tech_it != prereq_search_end)
					{
						const size_t prereq_i = tech_it - techs.begin();

						//values[prereq_i] += values[i]*iPrereqPercent/100;
						prereq_bonus[prereq_i] = std::max(prereq_bonus[prereq_i],
								techs[i].first*iPrereqPercent/100);
						if (gPlayerLogLevel >= 3)
						{
							logBBAI("      %S adds %d to %S (depth %d)",
									GC.getTechInfo(techs[i].second).
									getDescription(),
									techs[i].first*iPrereqPercent/100,
									GC.getTechInfo(techs[prereq_i].second).
									getDescription(), iDepth-1);
						}
					}
				}
			}

			// Apply the prereq_bonuses
			for (size_t p = 0; p < prereq_bonus.size(); ++p)
			{
				/*  Kludge: Under this system, dead-end techs (such as rifling)
					can be avoided due to the high value of follow-on techs.
					So here's what we're going to do... */
				techs[p].first += std::max(prereq_bonus[p],
						(techs[p].first - 80)*iPrereqPercent*3/400);
				/*  Note, the "-80" is meant to represent removing the
					random value bonus. cf. AI_techValue (divided by ~5 turns).
					(bonus is 0-80*cities) */
			}
		}
	}
	// All the evaluations are now complete. Now we just have to find the best tech.
	std::vector<std::pair<int, TechTypes> >::iterator tech_it = std::max_element(techs.begin(), (bNewWays ? techs.begin()+techs_to_depth[1] : techs.end()),PairFirstLess<int, TechTypes>());
	if (tech_it == (bNewWays ? techs.begin()+techs_to_depth[1] : techs.end()))
	{
		FAssert(iTechCount == 0);
		return NO_TECH;
	}
	TechTypes eBestTech = tech_it->second;
	FAssert(canResearch(eBestTech));

	if (gPlayerLogLevel >= 1)
	{
		logBBAI("  Player %d (%S) selects tech %S with value %d", getID(), getCivilizationDescription(0), GC.getTechInfo(eBestTech).getDescription(), tech_it->first );
	}

	return eBestTech;
	}
#endif

	/*  Yet another version!
		pathways version
		We've evaluated all the techs up to the given depth. Now we want to
		choose the highest value pathway. eg. suppose iMaxPathLength = 3; we
		will then look for the best three techs to research, in order.
		It could be three techs for which we already have all prereqs, or it
		could be techs leading to new techs.

		Algorithm:
		* Build list of techs at each depth.
		* Sort lists by value at each depth.
		We don't want to consider every possible set of three techs.
		Many combos can be disregarded easily.
		For explanation purposes, assume a depth of 3.
		* The 3rd highest value at depth = 0 is a threshold for the next depth.
		  No techs lower than the threshold need to be considered in any combo.
		* The max of the old threshold and the (max_depth-cur_depth)th value
		  becomes the new threshold. eg. at depth=1, the 2nd highest value
		  becomes the new threshold (if it is higher than the old).
		* All techs above the threshold are viable end points.
		* For each end point, pick the highest value prereqs which allow us to
		  reach the endpoint.
		* If there that doesn't fill all the full path, pick the highest value
		  available techs. (eg. if our end-point is not a the max depth, we can
		  pick an arbitrary tech at depth=0) */

	// Sort the techs at each depth:
	FAssert(techs_to_depth[0] == 0); // No techs before depth 0.
	// max depth is after all techs
	FAssert(techs.size() == techs_to_depth[techs_to_depth.size()-1]);
	for (size_t i = 1; i < techs_to_depth.size(); ++i)
	{
		std::sort(techs.begin()+techs_to_depth[i-1], techs.begin()+techs_to_depth[i], std::greater<std::pair<int, TechTypes> >());
	}

	// First deal with the trivial cases...
	// no Techs
	if (techs.empty())
		return NO_TECH;
	// path length of 1.
	if (iMaxPathLength < 2)
	{
		FAssert(techs.size() > 0);
		return techs[0].second;
	}
	// ... and the case where there are not enough techs in the list.
	if ((int)techs.size() < iMaxPathLength)
	{
		return techs[0].second;
	}

	// Create a list of possible tech paths.
	std::vector<std::pair<int, std::vector<int> > > tech_paths; // (total_value, path)
	// Note: paths are a vector of indices referring to `techs`.
	/*  Paths are in reverse order, for convinence in constructive them.
		(ie. the first tech to research is at the end of the list.) */

	// Initial threshold
	FAssert(techs_to_depth.size() > 1);
	int iThreshold = techs[std::min(iMaxPathLength-1,
			(int)techs.size()-1)].first;
	// Note: this works even if depth=0 isn't big enough.

	double fDepthRate = 0.8;

	for (int end_depth = 0; end_depth < iMaxPathLength; ++end_depth)
	{
		// Note: at depth == 0, there are no prereqs, so we only need to consider the best option.
		for (int i = (end_depth == 0? iMaxPathLength-1 :
				techs_to_depth[end_depth]); i < techs_to_depth[end_depth+1]; ++i)
		{
			if (techs[i].first < iThreshold)
				break; /* Note: the techs are sorted, so if we're below the
						  threshold, we're done. */

			// This is a valid end point. So start a new tech path.
			tech_paths.push_back(std::make_pair(techs[i].first,
					std::vector<int>()));
			tech_paths.back().second.push_back(i);
			// A set of techs that will be in our path
			std::set<TechTypes> techs_in_path;
			// A queue of techs that we still need to check prereqs for
			std::queue<TechTypes> techs_to_check;

			techs_in_path.insert(techs[i].second);
			if (end_depth != 0)
			{
				techs_to_check.push(techs[i].second);
			}

			while (!techs_to_check.empty() && (int)techs_in_path.size() <
					iMaxPathLength)
			{
				bool bMissingPrereq = false;

				// AndTech prereqs:
				for (int p = 0; p < GC.getNUM_AND_TECH_PREREQS() &&
						!bMissingPrereq; ++p)
				{
					TechTypes ePrereq = (TechTypes)GC.getTechInfo(
							techs_to_check.front()).getPrereqAndTechs(p);
					if (!kTeam.isHasTech(ePrereq) &&
							techs_in_path.find(ePrereq) == techs_in_path.end())
					{
						bMissingPrereq = true;
						// find the tech. (Lambda would be nice...)
						//std::find_if(techs.begin(), techs.end(),[](std::pair<int, TechTypes> &t){return t.second == ePrereq;});
						/*  really we should use current depth instead of end_depth;
							but that's harder... */
						for (int j = 0; j < techs_to_depth[end_depth]; ++j)
						{
							if (techs[j].second == ePrereq)
							{
								// add it to the path.
								tech_paths.back().first = (int)(fDepthRate *
										tech_paths.back().first);
								tech_paths.back().first += techs[j].first;
								tech_paths.back().second.push_back(i);
								techs_in_path.insert(ePrereq);
								techs_to_check.push(ePrereq);
								bMissingPrereq = false;
								break;
							}
						}
					}
				}
				// OrTechs:
				int iBestOrIndex = -1;
				int iBestOrValue = -1;
				for (int p = 0; p < GC.getNUM_OR_TECH_PREREQS(); ++p)
				{
					TechTypes ePrereq = (TechTypes)GC.getTechInfo(
							techs_to_check.front()).getPrereqOrTechs(p);
					if (ePrereq == NO_TECH)
						continue;

					if (!kTeam.isHasTech(ePrereq) &&
							techs_in_path.find(ePrereq) == techs_in_path.end())
					{
						bMissingPrereq = true;
						// find the tech.
						for (int j = 0; j < techs_to_depth[end_depth]; ++j)
						{
							if (techs[j].second == ePrereq)
							{
								if (techs[j].first > iBestOrValue)
								{
									iBestOrIndex = j;
									iBestOrValue = techs[j].first;
								}
							}
						}
					}
					else
					{
						// We have one of the orPreqs.
						iBestOrIndex = -1;
						iBestOrValue = -1;
						bMissingPrereq = false;
						break;
					}
				}
				// Add the best OrPrereq to the path
				if (iBestOrIndex >= 0)
				{
					FAssert(bMissingPrereq);

					tech_paths.back().first = (int)(fDepthRate *
							tech_paths.back().first);
					tech_paths.back().first += techs[iBestOrIndex].first;
					tech_paths.back().second.push_back(iBestOrIndex);
					techs_in_path.insert(techs[iBestOrIndex].second);
					techs_to_check.push(techs[iBestOrIndex].second);
					bMissingPrereq = false;
				}

				if (bMissingPrereq)
				{
					break; // failured to add prereqs to the path
				}
				else
				{
					techs_to_check.pop(); // prereqs are satisfied
				}
			} // end techs_to_check (prereqs loop)

			/*  If we couldn't add all the prereqs (eg. too many),
				abort the path. */
			if ((int)techs_in_path.size() > iMaxPathLength ||
					!techs_to_check.empty())
			{
				tech_paths.pop_back();
				continue;
			}

			/*  If we haven't already filled the path with prereqs,
				fill the remaining slots with the highest value unused techs. */
			if (((int)techs_in_path.size() < iMaxPathLength))
			{
				/*  todo: consider backfilling the list with deeper techs if
					we've matched their prereqs already. */
				while ((int)techs_in_path.size() < iMaxPathLength)
				{
					for (int j = 0; j < techs_to_depth[1] &&
							(int)techs_in_path.size() < iMaxPathLength; ++j)
					{
						if (techs_in_path.count(techs[j].second) == 0)
						{
							techs_in_path.insert(techs[j].second);
							/*  Note: since this tech isn't a prereqs,
								it can go anywhere in our path. Try to research
								highest values first. */
							int k = 0;
							for (; k < (int)tech_paths.back().
									second.size(); ++k)
							{
								if (techs[j].first < techs[tech_paths.back().
										second[k]].first)
								{
									// Note: we'll need to recalculate the total value.
									tech_paths.back().second.insert(
											tech_paths.back().second.begin()+k, j);
									break;
								}
							}
							if (k == tech_paths.back().second.size())
							{
								// haven't added it yet
								tech_paths.back().second.push_back(j);
							}
						}
					}
				}
				// Recalculate total value;
				tech_paths.back().first = 0;
				for (int k = 0; k < (int)tech_paths.back().second.size(); ++k)
				{
					tech_paths.back().first = (int)(fDepthRate *
							tech_paths.back().first);
					tech_paths.back().first +=
							techs[tech_paths.back().second[k]].first;
				}
			}
		} // end loop through techs at given end depth

		/*  TODO: at this point we should update the threshold for the
			next depth... But I don't want to do that until the back-fill stage
			is fixed to consider deeper techs. */
	} // end loop through possible end depths

	/*  Return the tech corresponding to the back (first step) of the tech path
		with the highest value. */
	if (tech_paths.empty())
	{
		FAssertMsg(0, "Failed to create any tech paths");
		return NO_TECH;
	}
	std::vector<std::pair<int,std::vector<int> > >::iterator best_path_it =
			std::max_element(tech_paths.begin(), tech_paths.end(),
			PairFirstLess<int, std::vector<int> >());
	TechTypes eBestTech = techs[best_path_it->second.back()].second;
	if( gPlayerLogLevel >= 1)
	{
		logBBAI("  Player %d (%S) selects tech %S with value %d. (Aiming for %S)",
				getID(), getCivilizationDescription(0),
				GC.getTechInfo(eBestTech).getDescription(),
				techs[best_path_it->second.back()].first,
				GC.getTechInfo(techs[best_path_it->second.front()].second).
				getDescription());
	}
	FAssert(canResearchBulk(eBestTech, false, bFreeTech));
	// </k146>
	return eBestTech;
}

// This function has been mostly rewritten for K-Mod.
// Note: many of the values used in this function are arbitrary; but adjusted them all to get closer to having a common scale.
// The scale before research time is taken into account is roughly 4 = 1 commerce per turn. Afterwards it is arbitrary.
// (Compared to the original numbers, this is * 1/100 * 7 * 4. 28/100)
int CvPlayerAI::AI_techValue(TechTypes eTech, int iPathLength, bool bFreeTech, bool bAsync, const std::vector<int>& viBonusClassRevealed, const std::vector<int>& viBonusClassUnrevealed, const std::vector<int>& viBonusClassHave) const
{
	PROFILE_FUNC();
	FAssert(viBonusClassRevealed.size() == GC.getNumBonusClassInfos());
	FAssert(viBonusClassUnrevealed.size() == GC.getNumBonusClassInfos());
	FAssert(viBonusClassHave.size() == GC.getNumBonusClassInfos());

	long iValue = 1; // K-Mod. (the int was overflowing in parts of the calculation)

	CvCity* pCapitalCity = getCapitalCity();
	const CvTeamAI& kTeam = GET_TEAM(getTeam());
	const CvTechInfo& kTechInfo = GC.getTechInfo(eTech); // K-Mod

	bool bCapitalAlone = (GC.getGameINLINE().getElapsedGameTurns() > 0) ? AI_isCapitalAreaAlone() : false;
	bool bFinancialTrouble = AI_isFinancialTrouble();
	bool bAdvancedStart = getAdvancedStartPoints() >= 0;

	int iHasMetCount = kTeam.getHasMetCivCount(true);
	int iCoastalCities = countNumCoastalCities();

	int iCityCount = getNumCities();
	int iCityTarget = GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities();
	// <k146>
	int iRandomFactor = 0; // Amount of random value in the answer.
	int iRandomMax = 0;   // Max random value. (These randomness trackers aren't actually used, and may not even be accurate.)
	//if (iPathLength <= 1) // Don't include random bonus for follow-on tech values.
	{ // </k146>
		iRandomFactor = ((bAsync) ? GC.getASyncRand().get(80*iCityCount, "AI Research ASYNC") : GC.getGameINLINE().getSorenRandNum(80*iCityCount, "AI Research"));
		iRandomMax = 80*iCityCount;
		iValue += iRandomFactor;
	}
	if(!bFreeTech) // k146
		iValue += kTeam.getResearchProgress(eTech)/4;

	// Map stuff
	if (kTechInfo.isExtraWaterSeeFrom())
	{
		if (iCoastalCities > 0)
		{
			iValue += 28;

			if (bCapitalAlone)
			{
				iValue += 112;
			}
		}
	}

	if (kTechInfo.isMapCentering())
	{
		// K-Mod. The benefits of this are marginal at best.
	}

	if (kTechInfo.isMapVisible())
	{
		iValue += (3*GC.getMapINLINE().getLandPlots() + GC.getMapINLINE().numPlots())/400; // (3 * 1100 + 4400)/400 = 14. ~3 commerce/turn
		// Note, the world is usually thoroughly explored by the time of satellites. So this is low value.
		// If we wanted to evaluate this properly, we'd need to calculate at how much of the world is still unexplored.
	}

	// Expand trading options
	//if (kTechInfo.isMapTrading())
	if (kTechInfo.isMapTrading() && !kTeam.isMapTrading()) // K-Mod
	{
		// K-Mod. increase the bonus for each known civ that we can't already tech trade with
		int iNewTrade = 0;
		int iExistingTrade = 0;
		for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i = (TeamTypes)(i+1))
		{
			if (i == getTeam() || !kTeam.isHasMet(i))
				continue;
			const CvTeamAI& kLoopTeam = GET_TEAM(i);
			if (!kLoopTeam.isMapTrading())
			{
				if (kLoopTeam.AI_mapTrade(getTeam()) == NO_DENIAL && kTeam.AI_mapTrade(i) == NO_DENIAL)
					iNewTrade += kLoopTeam.getAliveCount();
			}
			else
				iExistingTrade += kLoopTeam.getAliveCount();
		}
		// The value could be scaled based on how much map we're missing; but I don't want to waste time calculating that.
		int iBase = (3*GC.getMapINLINE().getLandPlots() + GC.getMapINLINE().numPlots()) / 300; // ~ (3 * 1100 + 4400). ~ 4.5 commerce/turn
		iValue += iBase/2;
		if (iNewTrade > 0)
		{
			if (bCapitalAlone) // (or rather, have we met anyone from overseas)
			{
				iValue += 5*iBase; // a stronger chance of getting the map for a different island
			}

			if (iExistingTrade == 0 && iNewTrade > 1)
			{
				iValue += 3*iBase; // we have the possibility of being a map broker.
			}
			iValue += iBase*(iNewTrade + 1);
		}
		// K-Mod end
	}

	//if (kTechInfo.isTechTrading() && !GC.getGameINLINE().isOption(GAMEOPTION_NO_TECH_TRADING))
	if (kTechInfo.isTechTrading() && !GC.getGameINLINE().isOption(GAMEOPTION_NO_TECH_TRADING) && !kTeam.isTechTrading()) // K-Mod
	{
		// K-Mod. increase the bonus for each known civ that we can't already tech trade with
		int iBaseValue = getTotalPopulation() * 3;

		int iNewTrade = 0;
		int iExistingTrade = 0;
		for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i = (TeamTypes)(i+1))
		{
			if (i == getTeam() || !kTeam.isHasMet(i))
				continue;
			const CvTeamAI& kLoopTeam = GET_TEAM(i);
			if (!kLoopTeam.isTechTrading())
			{
				if (kLoopTeam.AI_techTrade(NO_TECH, getTeam()) == NO_DENIAL && kTeam.AI_techTrade(NO_TECH, i) == NO_DENIAL)
					iNewTrade += kLoopTeam.getAliveCount();
			}
			else
				iExistingTrade += kLoopTeam.getAliveCount();
		}
		iValue += iBaseValue * std::max(0, 3*iNewTrade - iExistingTrade);
		// K-Mod end
	}

	if (kTechInfo.isGoldTrading() && !kTeam.isGoldTrading())
	{
		// K-Mod. The key value of gold trading is to facilitate tech trades.
		int iBaseValue =  (!GC.getGameINLINE().isOption(GAMEOPTION_NO_TECH_TRADING) && !kTeam.isTechTrading())
			// <k146>
			? (getTotalPopulation() + 10)
			: (getTotalPopulation()/3 + 3); // </k146>
		int iNewTrade = 0;
		int iExistingTrade = 0;
		for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i = (TeamTypes)(i+1))
		{
			if (i == getTeam() || !kTeam.isHasMet(i))
				continue;
			const CvTeamAI& kLoopTeam = GET_TEAM(i);
			if (!kLoopTeam.isGoldTrading())
			{
				if (kLoopTeam.AI_techTrade(NO_TECH, getTeam()) == NO_DENIAL && kTeam.AI_techTrade(NO_TECH, i) == NO_DENIAL)
					iNewTrade += kLoopTeam.getAliveCount();
			}
			else
				iExistingTrade += kLoopTeam.getAliveCount();
		}
		iValue += iBaseValue * std::max(
				iNewTrade, // k146: was 0
				3*iNewTrade - iExistingTrade);
	}

	if (kTechInfo.isOpenBordersTrading())
	{
		// (Todo: copy the NewTrade / ExistingTrade method from above.)
		if (iHasMetCount > 0)
		{
			iValue += 12 * iCityCount;

			if (iCoastalCities > 0)
			{
				iValue += 4 * iCityCount;
			}
		}
	}

	// K-Mod. Value pact trading based on how many civs are willing, and on how much we think we need it!
	if (kTechInfo.isDefensivePactTrading() && !kTeam.isDefensivePactTrading())
	{
		int iNewTrade = 0;
		int iExistingTrade = 0;
		for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i = (TeamTypes)(i+1))
		{
			if (i == getTeam() || !kTeam.isHasMet(i))
				continue;
			const CvTeamAI& kLoopTeam = GET_TEAM(i);
			if (!kLoopTeam.isDefensivePactTrading())
			{
				if (kLoopTeam.AI_defensivePactTrade(getTeam()) == NO_DENIAL && kTeam.AI_defensivePactTrade(i) == NO_DENIAL)
					iNewTrade += kLoopTeam.getAliveCount();
			}
			else
				iExistingTrade += kLoopTeam.getAliveCount();
		}
		if (iNewTrade > 0)
		{
			int iPactValue = 3;
			if (AI_isDoStrategy(AI_STRATEGY_ALERT1))
				iPactValue += 1;
			if (AI_isDoStrategy(AI_STRATEGY_ALERT2))
				iPactValue += 1;
			/*  advc.115b: Commented out. Diplo victory isn't that peaceful, and
				pact improves relations with one civ, but worsens them with others. */
			/*if (AI_isDoVictoryStrategy(AI_VICTORY_DIPLOMACY2))
				iPactValue += 2;*/
			if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3 | AI_VICTORY_SPACE3 | AI_VICTORY_DIPLOMACY3))
				iPactValue += 1;

			iValue += iNewTrade * iPactValue * 28;
		}
	}
	// K-Mod end

	if (kTechInfo.isPermanentAllianceTrading() && (GC.getGameINLINE().isOption(GAMEOPTION_PERMANENT_ALLIANCES)))
	{
		//iValue += 56;
		iValue += 8*iCityTarget; // k146: Replacing the above
		// todo: check for friendly civs
	}

	if (kTechInfo.isVassalStateTrading() && !(GC.getGameINLINE().isOption(GAMEOPTION_NO_VASSAL_STATES)))
	{
		//iValue += 56;
		iValue += 8*iCityTarget; // k146: Replacing the above
		// todo: check anyone is small enough to vassalate. Check for conquest / domination strategies.
	}

	// Tile improvement abilities
	if (kTechInfo.isBridgeBuilding())
	{
		iValue += 6 * iCityCount;
	}

	if (kTechInfo.isIrrigation())
	{
		iValue += 16 * iCityCount;
	}

	if (kTechInfo.isIgnoreIrrigation())
	{
		iValue += 6 * iCityCount; // K-Mod. Ignore irrigation isn't so great.
	}

	if (kTechInfo.isWaterWork())
	{
		iValue += 6 * iCoastalCities * getAveragePopulation();
	}

	//iValue += (kTechInfo.getFeatureProductionModifier() * 2);
	//iValue += (kTechInfo.getWorkerSpeedModifier() * 4);
	iValue += kTechInfo.getFeatureProductionModifier() * 2 * std::max(iCityCount, iCityTarget) / 25;
	iValue += kTechInfo.getWorkerSpeedModifier() * 2 * std::max(iCityCount, iCityTarget/2) / 25;

	//iValue += (kTechInfo.getTradeRoutes() * (std::max((getNumCities() + 2), iConnectedForeignCities) + 1) * ((bFinancialTrouble) ? 200 : 100));
	// K-Mod. A very rough estimate assuming each city has ~2 trade routes; new local trade routes worth ~2 commerce, and foreign worth ~6.
	if (kTechInfo.getTradeRoutes() != 0)
	{
		int iConnectedForeignCities = AI_countPotentialForeignTradeCities(true, AI_getFlavorValue(FLAVOR_GOLD) == 0);
		int iAddedCommerce = (2*iCityCount*kTechInfo.getTradeRoutes() + 4*range(iConnectedForeignCities-2*getNumCities(), 0, iCityCount*kTechInfo.getTradeRoutes()));
		iValue += iAddedCommerce * (bFinancialTrouble ? 6 : 4) * AI_averageYieldMultiplier(YIELD_COMMERCE)/100;
		//iValue += (2*iCityCount*kTechInfo.getTradeRoutes() + 4*range(iConnectedForeignCities-2*getNumCities(), 0, iCityCount*kTechInfo.getTradeRoutes())) * (bFinancialTrouble ? 6 : 4);
	}
	// K-Mod end


	if (kTechInfo.getHealth() != 0)
	{
		iValue += 24 * iCityCount * AI_getHealthWeight(kTechInfo.getHealth(), 1) / 100;
	}
	if (kTechInfo.getHappiness() != 0)
	{
		// (this part of the evaluation was completely missing from the original bts code)
		iValue += 40 * iCityCount * AI_getHappinessWeight(kTechInfo.getHappiness(), 1) / 100;
	}

	for (int iJ = 0; iJ < GC.getNumRouteInfos(); iJ++)
	{
		//iValue += -(GC.getRouteInfo((RouteTypes)iJ).getTechMovementChange(eTech) * 100);
		// K-Mod. Note: techMovementChange is O(10)
		if (GC.getRouteInfo((RouteTypes)iJ).getTechMovementChange(eTech))
		{
			int iTemp = 4 * (std::max(iCityCount, iCityTarget) - iCoastalCities/2);
			if (bCapitalAlone)
				iTemp = iTemp * 2/3;
			if(isFocusWar()) // advc.105
			//if (kTeam.getAnyWarPlanCount(true))
				iTemp *= 2;
			iValue -= GC.getRouteInfo((RouteTypes)iJ).getTechMovementChange(eTech) * iTemp;
		}
		// K-Mod end
	}

	for (int iJ = 0; iJ < NUM_DOMAIN_TYPES; iJ++)
	{
		iValue += kTechInfo.getDomainExtraMoves(iJ) * (iJ == DOMAIN_LAND ? 16 : 6) * iCityCount;
	}

	// K-Mod. Extra specialist commerce. (Based on my civic evaluation code)
	bool bSpecialistCommerce = false;
	int iTotalBonusSpecialists = -1;
	int iTotalCurrentSpecialists = -1;
	for (CommerceTypes i = (CommerceTypes)0; i < NUM_COMMERCE_TYPES; i=(CommerceTypes)(i+1))
	{
		bSpecialistCommerce = kTechInfo.getSpecialistExtraCommerce(i) != 0;
	}

	if (bSpecialistCommerce)
	{
		// If there are any bonuses, we need to count our specialists.
 		// (The value from the bonuses will be applied later.)
		iTotalBonusSpecialists = iTotalCurrentSpecialists = 0;

		int iLoop;
		CvCity* pLoopCity;
		for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			iTotalBonusSpecialists += pLoopCity->getNumGreatPeople();
			iTotalBonusSpecialists += pLoopCity->totalFreeSpecialists();

			iTotalCurrentSpecialists += pLoopCity->getNumGreatPeople();
			iTotalCurrentSpecialists += pLoopCity->getSpecialistPopulation();
		}
	}
	// K-Mod end
	// <k146>
	for (CommerceTypes i = (CommerceTypes)0; i < NUM_COMMERCE_TYPES; i=(CommerceTypes)(i+1))
 	{
 		int iCommerceValue = 0;
 
 		// Commerce for specialists
 		if (bSpecialistCommerce)
 		{
			// If there are any bonuses, we need to count our specialists.
 			// (The value from the bonuses will be applied later.)
			iTotalBonusSpecialists = iTotalCurrentSpecialists = 0;
 			iCommerceValue += 4*AI_averageCommerceMultiplier(i)*
					(kTechInfo.getSpecialistExtraCommerce(i) *
					std::max((getTotalPopulation()+12*iTotalBonusSpecialists) / 
					12, iTotalCurrentSpecialists));
 		}
 
 		// Flexible commerce. (This is difficult to evaluate accurately without checking a lot of things - which I'm not going to do right now.)
 		if (kTechInfo.isCommerceFlexible(i))
 		{
			iCommerceValue += 40 + 4 * iCityCount;
 			/* original
 			iValue += 4 * iCityCount * (3*AI_averageCulturePressure()-200) / 100;
 			if (i == COMMERCE_CULTURE && AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2))
 			{
 				iValue += 280;
 			} */
 		}
 
 		if (iCommerceValue)
 		{
 			iCommerceValue *= AI_commerceWeight(i);
 			iCommerceValue /= 100;
 
 			iValue += iCommerceValue;
 		}
	} // </k146>
	for (int iJ = 0; iJ < GC.getNumTerrainInfos(); iJ++)
	{
		if (kTechInfo.isTerrainTrade(iJ))
		{
			if (GC.getTerrainInfo((TerrainTypes)iJ).isWater())
			{
				if (pCapitalCity != NULL)
				{
					//iValue += (countPotentialForeignTradeCities(pCapitalCity->area()) * 100);
					// K-Mod. Note: this could be coastal trade or ocean trade.
					// AI_countPotentialForeignTradeCities doesn't count civs we haven't met. Hopefully that will protect us from overestimating new routes.
					int iCurrentForeignRoutes = AI_countPotentialForeignTradeCities(true, AI_getFlavorValue(FLAVOR_GOLD) == 0);
					int iNewForeignRoutes = AI_countPotentialForeignTradeCities(false, AI_getFlavorValue(FLAVOR_GOLD) == 0) - iCurrentForeignRoutes;

					int iNewOverseasRoutes = std::max(0, AI_countPotentialForeignTradeCities(false, AI_getFlavorValue(FLAVOR_GOLD) == 0, pCapitalCity->area()) - iCurrentForeignRoutes);
					int iLocalRoutes = std::max(0, iCityCount*3 - iCurrentForeignRoutes);

					// TODO: multiply by average commerce multiplier?

					// 4 for upgrading local to foreign. 2 for upgrading to overseas. Divide by 3 if we don't have coastal cities.
					iValue += (std::min(iLocalRoutes, iNewForeignRoutes) * 16  + std::min(iNewOverseasRoutes, iCityCount*3) * 8) / (iCoastalCities > 0 ? 1 : 3);
					// K-Mod end
				}

				iValue += 8;
			}
			else
			{
				iValue += 280; // ??? (what could it be?)
			}
		}
	}

	if (kTechInfo.isRiverTrade())
	{
		iValue += (bCapitalAlone || iHasMetCount == 0 ? 2 : 4) * (std::max(iCityCount, iCityTarget/2) - iCoastalCities/2); // K-Mod.
		// Note: the value used here is low, because it's effectively double-counting what we already got from coastal trade.
	}

	/* ------------------ Tile Improvement Value  ------------------ */
	for (int iJ = 0; iJ < GC.getNumImprovementInfos(); iJ++)
	{
		for (int iK = 0; iK < NUM_YIELD_TYPES; iK++)
		{
			int iTempValue = 0;

			/* original code
			iTempValue += (GC.getImprovementInfo((ImprovementTypes)iJ).getTechYieldChanges(eTech, iK) * getImprovementCount((ImprovementTypes)iJ) * 50); */
			// Often, an improvement only becomes viable after it gets the tech bonus.
			// So it's silly to score the bonus proportionally to how many of the improvements we already have.
			iTempValue += GC.getImprovementInfo((ImprovementTypes)iJ).getTechYieldChanges(eTech, iK)
					* std::max(getImprovementCount((ImprovementTypes)iJ)
					+iCityCount, // k146
					3*iCityCount/2) * 4;
			// This new version is still bork, but at least it won't be worthless.
			iTempValue *= AI_averageYieldMultiplier((YieldTypes)iK);
			iTempValue /= 100;

			iTempValue *= AI_yieldWeight((YieldTypes)iK);
			iTempValue /= 100;

			iValue += iTempValue;
		}
	}

	int iBuildValue = 0;
	for (BuildTypes eLoopBuild = (BuildTypes)0; eLoopBuild < GC.getNumBuildInfos(); eLoopBuild=(BuildTypes)(eLoopBuild+1))
	{
		if (GC.getBuildInfo(eLoopBuild).getTechPrereq() == eTech)
		{
			ImprovementTypes eBuildImprovement = (ImprovementTypes)(GC.getBuildInfo(eLoopBuild).getImprovement());
			ImprovementTypes eFinalImprovement = finalImprovementUpgrade(eBuildImprovement);
			// Note: finalImprovementUpgrade returns -1 for looping improvements; and that isn't handled well here.

			if (eBuildImprovement == NO_IMPROVEMENT)
			{
				iBuildValue += 8*iCityCount;
			}
			else
			{
				int iAccessibility = 0;
				int iBestFeatureValue = 0; // O(100)

				const CvImprovementInfo& kBuildImprovement = GC.getImprovementInfo(eBuildImprovement);
				{
					// iAccessibility represents the number of this improvement we expect to have per city.
					iAccessibility += ((kBuildImprovement.isHillsMakesValid()) ? 150 : 0);
					iAccessibility += ((kBuildImprovement.isFreshWaterMakesValid()) ? 150 : 0);
					iAccessibility += ((kBuildImprovement.isRiverSideMakesValid()) ? 150 : 0);

					for (int iK = 0; iK < GC.getNumTerrainInfos(); iK++)
					{
						iAccessibility += (kBuildImprovement.getTerrainMakesValid(iK) ? 75 : 0);
					}

					for (int iK = 0; iK < GC.getNumFeatureInfos(); iK++)
					{
						if (kBuildImprovement.getFeatureMakesValid(iK))
						{
							iAccessibility += 75;
							int iTempValue = 0;
							for (YieldTypes i = (YieldTypes)0; i < NUM_YIELD_TYPES; i=(YieldTypes)(i+1))
							{
								iTempValue += GC.getFeatureInfo((FeatureTypes)iK).getYieldChange(i) * AI_yieldWeight(i) * AI_averageYieldMultiplier(i) / 100;
							}
							if (iTempValue > iBestFeatureValue)
								iBestFeatureValue = iTempValue;
						}
					}
				}

				int iYieldValue = 0; // O(100)
				if (eFinalImprovement == NO_IMPROVEMENT)
					eFinalImprovement = eBuildImprovement;
				const CvImprovementInfo& kFinalImprovement = GC.getImprovementInfo(eFinalImprovement);
				if (iAccessibility > 0)
				{
					iYieldValue += iBestFeatureValue;

					for (int iK = 0; iK < NUM_YIELD_TYPES; iK++)
					{
						int iTempValue = 0;

						// use average of final and initial improvements
						iTempValue += kBuildImprovement.getYieldChange(iK) * 100;
						iTempValue += kBuildImprovement.getRiverSideYieldChange(iK) * 50;
						iTempValue += kBuildImprovement.getHillsYieldChange(iK) * 75;
						iTempValue += kBuildImprovement.getIrrigatedYieldChange(iK) * 80;

						iTempValue += kFinalImprovement.getYieldChange(iK) * 100;
						iTempValue += kFinalImprovement.getRiverSideYieldChange(iK) * 50;
						iTempValue += kFinalImprovement.getHillsYieldChange(iK) * 75;
						iTempValue += kFinalImprovement.getIrrigatedYieldChange(iK) * 80;

						iTempValue /= 2;

						iTempValue *= AI_averageYieldMultiplier((YieldTypes)iK);
						iTempValue /= 100;

						iTempValue *= AI_yieldWeight((YieldTypes)iK);
						iTempValue /= 100;

						iYieldValue += iTempValue;
					}

					// The following are not yields, but close enough.
					iYieldValue += kFinalImprovement.isActsAsCity() ? 100 : 0;
					iYieldValue += kFinalImprovement.isCarriesIrrigation() ? 100 : 0;

					if (getCurrentEra() > GC.getGameINLINE().getStartEra())
						iYieldValue -= 100; // compare to a hypothetical low-value improvement

					iBuildValue += std::max(0, iYieldValue) * iAccessibility * (kFinalImprovement.isWater() ? iCoastalCities : std::max(2, iCityCount)) / 2500;
					// Note: we're converting from O(100)*O(100) to O(4)
				}

				for (int iK = 0; iK < GC.getNumBonusInfos(); iK++)
				{
					const CvBonusInfo& kBonusInfo = GC.getBonusInfo((BonusTypes)iK);

					if (!kFinalImprovement.isImprovementBonusMakesValid(iK) && !kFinalImprovement.isImprovementBonusTrade(iK))
						continue;

					bool bRevealed = kTeam.isBonusRevealed((BonusTypes)iK);

					int iNumBonuses = bRevealed
						? countOwnedBonuses((BonusTypes)iK) // actual count
						: std::max(1, 2*getNumCities() / std::max(1, 3*iCityTarget)); // a guess

					//iNumBonuses += std::max(0, (iCityTarget - iCityCount)*(kFinalImprovement.isWater() ? 2 : 3)/8); // future expansion

					if (iNumBonuses > 0 && (bRevealed || kBonusInfo.getTechReveal() == eTech))
					{
						int iBonusValue = 0;

						TechTypes eConnectTech = (TechTypes)kBonusInfo.getTechCityTrade();
						if ((kTeam.isHasTech(eConnectTech) || eConnectTech == eTech) && !kTeam.isBonusObsolete((BonusTypes)iK) && kBonusInfo.getTechObsolete() != eTech)
						{
							// note: this is in addition to the getTechCityTrade evaluation lower in this function.
							iBonusValue += AI_bonusVal((BonusTypes)iK, 1, true) * iCityCount;
							if (bRevealed)
								iBonusValue += (iNumBonuses-1) * iBonusValue / 10;
							else
								iBonusValue /= 2;
						}

						int iYieldValue = 0;
						for (YieldTypes y = (YieldTypes)0; y < NUM_YIELD_TYPES; y=(YieldTypes)(y+1))
						{
							int iTempValue = 0;

							iTempValue += kFinalImprovement.getImprovementBonusYield(iK, y) * 100;
							iTempValue += kFinalImprovement.getIrrigatedYieldChange(y) * 75;

							iTempValue *= AI_yieldWeight(y);
							iTempValue /= 100;
							iTempValue *= AI_averageYieldMultiplier(y);
							iTempValue /= 100;

							iYieldValue += iTempValue;
						}

						if (getCurrentEra() > GC.getGameINLINE().getStartEra())
							iYieldValue -= 100; // compare to a hypothetical low-value improvement

						// Bonuses might be outside of our city borders.
						iYieldValue *= 2*iNumBonuses;
						iYieldValue /= (bRevealed ? 3 : 4);

						if (kFinalImprovement.isWater())
							iYieldValue = iYieldValue * 2/3;

						// Convert to O(4)
						iYieldValue /= 25;

						iBuildValue += iBonusValue + std::max(0, iYieldValue);
					}
				}
			}

			RouteTypes eRoute = (RouteTypes)GC.getBuildInfo(eLoopBuild).getRoute();

			if (eRoute != NO_ROUTE)
			{
				int iRouteValue = 4 * (iCityCount - iCoastalCities/3) + (bAdvancedStart ? 2 : 0);
				if (getBestRoute() == NO_ROUTE)
					iRouteValue *= 4;
				else if (GC.getRouteInfo(getBestRoute()).getValue() < GC.getRouteInfo(eRoute).getValue())
					iRouteValue *= 2;

				for (int iK = 0; iK < NUM_YIELD_TYPES; iK++)
				{
					int iTempValue = 0;


					iTempValue += getNumCities() * GC.getRouteInfo(eRoute).getYieldChange(iK) * 32; // epic bonus for epic effect
					for (ImprovementTypes i = (ImprovementTypes)0; i < GC.getNumImprovementInfos(); i=(ImprovementTypes)(i+1))
					{
						iTempValue += GC.getImprovementInfo(i).getRouteYieldChanges(eRoute, iK) * std::max(getImprovementCount(i), 3*getNumCities()/2) * 4;
					}
					iTempValue *= AI_averageYieldMultiplier((YieldTypes)iK);
					iTempValue /= 100;

					iTempValue *= AI_yieldWeight((YieldTypes)iK);
					iTempValue /= 100;

					//iBuildValue += iTempValue;
					iRouteValue += iTempValue; // K-Mod
				}
				// K-Mod. Devalue it if we don't have the resources. (based on my code for unit evaluation)
				bool bMaybeMissing = false; // if we don't know if we have the bonus or not
				bool bDefinitelyMissing = false; // if we can see the bonuses, and we know we don't have any.

				for (int iK = 0; iK < GC.getNUM_ROUTE_PREREQ_OR_BONUSES(); ++iK)
				{
					BonusTypes ePrereqBonus = (BonusTypes)GC.getRouteInfo(eRoute).getPrereqOrBonus(iK);
					if (ePrereqBonus != NO_BONUS)
					{
						if (hasBonus(ePrereqBonus))
						{
							bDefinitelyMissing = false;
							bMaybeMissing = false;
							break;
						}
						else
						{
							if (kTeam.isBonusRevealed(ePrereqBonus) && countOwnedBonuses(ePrereqBonus) == 0)
							{
								bDefinitelyMissing = true;
							}
							else
							{
								bMaybeMissing = true;
							}
						}
					}
				}
				BonusTypes ePrereqBonus = (BonusTypes)GC.getRouteInfo(eRoute).getPrereqBonus();
				if (ePrereqBonus != NO_BONUS && !hasBonus(ePrereqBonus))
				{
					if ((kTeam.isHasTech((TechTypes)(GC.getBonusInfo(ePrereqBonus).getTechReveal())) || kTeam.isForceRevealedBonus(ePrereqBonus)) &&
						countOwnedBonuses(ePrereqBonus) == 0)
					{							
						bDefinitelyMissing = true;
					}
					else
					{							
						bMaybeMissing = true;
					}
				}
				if (bDefinitelyMissing)
				{
					iRouteValue /= 3;
				}
				else if (bMaybeMissing)
				{
					iRouteValue /= 2;
				}
				iBuildValue += iRouteValue;
				// K-Mod end
			}
		}
	}

    //the way feature-remove is done in XML is pretty weird
    //I believe this code needs to be outside the general BuildTypes loop
    //to ensure the feature-remove is only counted once rather than once per build
	//which could be a lot since nearly every build clears jungle...

	for (int iJ = 0; iJ < GC.getNumFeatureInfos(); iJ++)
	{
		bool bIsFeatureRemove = false;
		int iChopProduction = 0;
		for (int iK = 0; iK < GC.getNumBuildInfos(); iK++)
		{
			if (GC.getBuildInfo((BuildTypes)iK).getFeatureTech(iJ) == eTech)
			{
				bIsFeatureRemove = true;
				iChopProduction = GC.getBuildInfo((BuildTypes)iK).getFeatureProduction(iJ); // I'll assume it's the same for all builds
				break;
			}
		}

		if (bIsFeatureRemove)
		{
			int iChopValue = iChopProduction / 4; // k146: was 8

			if ((GC.getFeatureInfo(FeatureTypes(iJ)).getHealthPercent() < 0) ||
				((GC.getFeatureInfo(FeatureTypes(iJ)).getYieldChange(YIELD_FOOD) + GC.getFeatureInfo(FeatureTypes(iJ)).getYieldChange(YIELD_PRODUCTION) + GC.getFeatureInfo(FeatureTypes(iJ)).getYieldChange(YIELD_COMMERCE)) < 0))
			{
				iChopValue += 8; // k146: was 6
				/*  k146: Want to add value for the new city sites which become
					available due to this tech - but it isn't easy to evaluate.
					Perhaps we should go as far as counting features at our
					planned sites - but even that isn't exactly what we want. */
				iBuildValue += 40;
			}
			iBuildValue += 4 + iChopValue * (countCityFeatures((FeatureTypes)iJ) + 4);
		}
	}

	iValue += iBuildValue;

	// does tech reveal bonus resources
	for (int iJ = 0; iJ < GC.getNumBonusInfos(); iJ++)
	{
		if (GC.getBonusInfo((BonusTypes)iJ).getTechReveal() == eTech)
		{
			int iRevealValue = 8;
			iRevealValue += AI_bonusVal((BonusTypes)iJ, 1, true) * iCityCount * 2/3;
		
			BonusClassTypes eBonusClass = (BonusClassTypes)GC.getBonusInfo((BonusTypes)iJ).getBonusClassType();
			int iBonusClassTotal = (viBonusClassRevealed[eBonusClass] + viBonusClassUnrevealed[eBonusClass]);
		
			//iMultiplier is basically a desperation value
			//it gets larger as the AI runs out of options
			//Copper after failing to get horses is +66%
			//Iron after failing to get horse or copper is +200%
			//but with either copper or horse, Iron is only +25%
			int iMultiplier = 0;
			if (iBonusClassTotal > 0)
			{
                iMultiplier = (viBonusClassRevealed[eBonusClass] - viBonusClassHave[eBonusClass]);
                iMultiplier *= 100;
                iMultiplier /= iBonusClassTotal;
                
                iMultiplier *= (viBonusClassRevealed[eBonusClass] + 1);
                iMultiplier /= ((viBonusClassHave[eBonusClass] * iBonusClassTotal) + 1);
			}
		
			iMultiplier *= std::min(3, getNumCities());
			iMultiplier /= 3;
		
			iRevealValue *= 100 + iMultiplier;
			iRevealValue /= 100;

			// K-Mod
			// If we don't yet have the 'enable' tech, reduce the value of the reveal.
			if (GC.getBonusInfo((BonusTypes)iJ).getTechCityTrade() != eTech &&
				!kTeam.isHasTech((TechTypes)(GC.getBonusInfo((BonusTypes)iJ).getTechCityTrade())))
			{
				iRevealValue /= 3;
			}
			// K-Mod end

			iValue += iRevealValue;
		}
		// K-Mod: Value for enabling resources that are already revealed
		else if (GC.getBonusInfo((BonusTypes)iJ).getTechCityTrade() == eTech &&
			(kTeam.isHasTech((TechTypes)GC.getBonusInfo((BonusTypes)iJ).getTechReveal()) || kTeam.isForceRevealedBonus((BonusTypes)iJ)))
		{
			int iOwned = countOwnedBonuses((BonusTypes)iJ);
			if (iOwned > 0)
			{
				int iEnableValue = 4;
				iEnableValue += AI_bonusVal((BonusTypes)iJ, 1, true) * iCityCount;
				iEnableValue *= (iOwned > 1) ? 120 : 100;
				iEnableValue /= 100;

				iValue += iEnableValue;
			}
		}
		// K-Mod end
	}

	/* ------------------ Unit Value  ------------------ */
	bool bEnablesUnitWonder;
	iValue += AI_techUnitValue( eTech, iPathLength, bEnablesUnitWonder );
	
	if (bEnablesUnitWonder
			&& getTotalPopulation() > 5) // k146
	{	// <k146>
		const int iBaseRand = std::max(10, 110-30*iPathLength); // 80, 50, 20, 10
		int iWonderRandom = (bAsync ?
				GC.getASyncRand().get(iBaseRand, "AI Research Wonder Unit ASYNC") :
				GC.getGameINLINE().getSorenRandNum(iBaseRand, "AI Research Wonder Unit"));
		int iFactor = 100 * std::min(iCityCount, iCityTarget) /
				std::max(1, iCityTarget);
		iValue += (iWonderRandom + (bCapitalAlone ? 50 : 0)) * iFactor / 100;
		iRandomMax += iBaseRand * iFactor / 100;
		// </k146>
		iRandomFactor += iWonderRandom * iFactor / 100;
	}


	/* ------------------ Building Value  ------------------ */
	bool bEnablesWonder
			/*  advc.001n: AI_techBuildingValue now guaranteed to assign a value,
				but wasn't previously, and caller shouldn't rely on it. */
			= false;
	iValue += AI_techBuildingValue(eTech, bAsync, bEnablesWonder); // changed by K-Mod
	iValue -= AI_obsoleteBuildingPenalty(eTech, bAsync); // K-Mod!

	// K-Mod. Scale the random wonder bonus based on leader personality.
	/*  k146 (note): the value of the building itself was already counted by
		AI_techBuildingValue. This extra value is just because we like wonders. */
	if (bEnablesWonder && getTotalPopulation() > 5)
	{
		const int iBaseRand = std::max(10, 110-30*iPathLength); // 80, 50, 20, 10 (was 300)
		int iWonderRandom = ((bAsync) ? GC.getASyncRand().get(iBaseRand, "AI Research Wonder Building ASYNC") : GC.getGameINLINE().getSorenRandNum(iBaseRand, "AI Research Wonder Building"));
		int iFactor = 10 + GC.getLeaderHeadInfo(getPersonalityType()).getWonderConstructRand(); // note: highest value of iWonderConstructRand 50 in the default xml.
		iFactor += AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1) ? 15 : 0;
		iFactor += AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2) ? 10 : 0;
		iFactor /= bAdvancedStart ? 4 : 1;
		iFactor = iFactor * std::min(iCityCount, iCityTarget) / std::max(1, iCityTarget);
		iFactor += 50; // k146: This puts iFactor around 100, roughly.
		iValue += (40 // k146: was 200
				+ iWonderRandom) * iFactor / 100;
		iRandomMax += iBaseRand * iFactor / 100;
		iRandomFactor += iWonderRandom * iFactor / 100;
	}
	// K-Mod end

	bool bEnablesProjectWonder = false;
	// k146: Code moved into auxiliary function
	iValue += AI_techProjectValue(eTech, iPathLength, bEnablesProjectWonder);
	if (bEnablesProjectWonder)
	{
		int iWonderRandom = ((bAsync) ? GC.getASyncRand().get(56, "AI Research Wonder Project ASYNC") : GC.getGameINLINE().getSorenRandNum(56, "AI Research Wonder Project"));
		iValue += iWonderRandom;

		iRandomMax += 56;
		iRandomFactor += iWonderRandom;
	}


	/* ------------------ Process Value  ------------------ */
	bool bIsGoodProcess = false;
	for (int iJ = 0; iJ < GC.getNumProcessInfos(); iJ++)
	{
		if (GC.getProcessInfo((ProcessTypes)iJ).getTechPrereq() == eTech)
		{
			iValue += 28;

			for (int iK = 0; iK < NUM_COMMERCE_TYPES; iK++)
			{
				int iTempValue = (GC.getProcessInfo((ProcessTypes)iJ).getProductionToCommerceModifier(iK) * 4);

				// K-Mod. (check out what would happen to "bIsGoodProcess" without this bit.  "oops.")
				if (iTempValue <= 0)
					continue;
				// K-Mod end

				iTempValue *= AI_commerceWeight((CommerceTypes)iK);
				iTempValue /= 100;

				if (iK == COMMERCE_GOLD || iK == COMMERCE_RESEARCH)
				{
					bIsGoodProcess = true;
				}
				else if ((iK == COMMERCE_CULTURE) && AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1))
				{
					iTempValue *= 2; // k146: was 3
				}

				iValue += iTempValue * iCityCount / 100;
			}
		}
	}
	// <cdtw.3>
	bool bHighUnitCost = false;
	// After some testing and tweaking, this still doesn't seem useful:
	/*if(bIsGoodProcess && !bFinancialTrouble && !isFocusWar() && getCurrentEra() < 2 &&
			GET_TEAM(getTeam()).AI_getTotalWarOddsTimes100() <= 400) {
		int iNetCommerce = 1 + getCommerceRate(COMMERCE_GOLD) +
				getCommerceRate(COMMERCE_RESEARCH) + std::max(0, getGoldPerTurn());
		int iUnitCost = calculateUnitCost();
		if(iUnitCost * 20 > iNetCommerce)
			bHighUnitCost = true;
	}*/ // </cdtw.3>
	if (bIsGoodProcess && (bFinancialTrouble
			|| bHighUnitCost)) // cdtw.3
	{
		bool bHaveGoodProcess = false;
		for (int iJ = 0; iJ < GC.getNumProcessInfos(); iJ++)
		{
			if (kTeam.isHasTech((TechTypes)GC.getProcessInfo((ProcessTypes)iJ).getTechPrereq()))
			{
				bHaveGoodProcess = (GC.getProcessInfo((ProcessTypes)iJ).getProductionToCommerceModifier(COMMERCE_GOLD) + GC.getProcessInfo((ProcessTypes)iJ).getProductionToCommerceModifier(COMMERCE_RESEARCH)) > 0;
				if (bHaveGoodProcess)
				{
					break;
				}
			}
		}
		if (!bHaveGoodProcess)
		{
			iValue += (8*iCityCount // k146: was 6*...
				/ (bHighUnitCost ? 2 : 1)); // cdtw.3
		}
	}
	/* ------------------ Civic Value  ------------------ */
	for (int iJ = 0; iJ < GC.getNumCivicInfos(); iJ++)
	{
		if (GC.getCivicInfo((CivicTypes)iJ).getTechPrereq() == eTech
				 && !canDoCivics((CivicTypes)iJ)) // k146
		{
			CivicTypes eCivic = getCivics((CivicOptionTypes)(GC.getCivicInfo((CivicTypes)iJ).getCivicOptionType()));

			int iCurrentCivicValue = AI_civicValue(eCivic); // Note: scale of AI_civicValue is 1 = commerce/turn
			int iNewCivicValue = AI_civicValue((CivicTypes)iJ);

			if (iNewCivicValue > iCurrentCivicValue)
			{
				//iValue += std::min(2400, (2400 * (iNewCivicValue - iCurrentCivicValue)) / std::max(1, iCurrentCivicValue));
				/*  <k146> This should be 4 *, but situational values like civics
					are more interesting than static values */
				iValue += 5 * (iNewCivicValue - iCurrentCivicValue);
				/*  just a little something extra for the early game...
					to indicate that we may have undervalued the civic's
					long term appeal. */
				iValue += 4 * std::min(iCityCount, iCityTarget); // </k146>
			}

			if (eCivic == GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic()) {
				/*  k146: favourite civic is already taken into account in the
					civic evaluation above. */
				iValue += 6*iCityCount;
			}
			else
				iValue += 4*iCityCount;
		}
	}

	if (iPathLength <= 2)
	{
		bool bFirst = GC.getGameINLINE().countKnownTechNumTeams(eTech) == 0; // K-Mod

		// K-Mod. Make a list of corporation HQs that this tech enables.
		// (note: for convenience, I've assumed that each corp has at most one type of HQ building.)
		std::vector<BuildingTypes> corpHQs(GC.getNumCorporationInfos(), NO_BUILDING);

		for (int iB = 0; iB < GC.getNumBuildingInfos(); iB++)
		{
			const CvBuildingInfo& kBuildingInfo = GC.getBuildingInfo((BuildingTypes)iB);
			if (kBuildingInfo.getFoundsCorporation() != NO_CORPORATION)
			{
				if (isTechRequiredForBuilding(eTech, (BuildingTypes)iB))
				{
					FAssert(kBuildingInfo.getFoundsCorporation() < (int)corpHQs.size());
					corpHQs[kBuildingInfo.getFoundsCorporation()] = (BuildingTypes)iB;
				}
			}
		}
		// K-Mod end (the corpHQs map is used in the next section)

		// note: K-mod has moved this section from out of the first-discoverer block
		for (int iJ = 0; iJ < GC.getNumCorporationInfos(); iJ++)
		{
			/* original bts code
			if (GC.getCorporationInfo((CorporationTypes)iJ).getTechPrereq() == eTech)
			{
				if (!(GC.getGameINLINE().isCorporationFounded((CorporationTypes)iJ)))
				{
					iValue += 100 + ((bAsync) ? GC.getASyncRand().get(2400, "AI Research Corporation ASYNC") : GC.getGameINLINE().getSorenRandNum(2400, "AI Research Corporation"));
				}
			} */
			// K-Mod
			if (!GC.getGameINLINE().isCorporationFounded((CorporationTypes)iJ))
			{
				int iMissingTechs = 0;
				bool bCorpTech = false;

				if (GC.getCorporationInfo((CorporationTypes)iJ).getTechPrereq() == eTech)
				{
					bCorpTech = bFirst; // only good if we are the first to get the tech
					// missing tech stays at 0, because this is a special case. (ie. no great person required)
				}
				else if (corpHQs[iJ] != NO_BUILDING)
				{
					const CvBuildingInfo& kBuildingInfo = GC.getBuildingInfo(corpHQs[iJ]);
					if (kBuildingInfo.getPrereqAndTech() == eTech ||
						kTeam.isHasTech((TechTypes)kBuildingInfo.getPrereqAndTech()) ||
						canResearch((TechTypes)kBuildingInfo.getPrereqAndTech()))
					{
						bCorpTech = true;
						// Count the required techs. (cf. isTechRequiredForBuilding)

						iMissingTechs += !kTeam.isHasTech((TechTypes)kBuildingInfo.getPrereqAndTech()) ? 1 : 0;

						for (int iP = 0; iP < GC.getNUM_BUILDING_AND_TECH_PREREQS(); iP++)
						{
							iMissingTechs += !kTeam.isHasTech((TechTypes)kBuildingInfo.getPrereqAndTechs(iP)) ? 1 : 0;
						}

						SpecialBuildingTypes eSpecial = (SpecialBuildingTypes)kBuildingInfo.getSpecialBuildingType();
						iMissingTechs += eSpecial != NO_SPECIALBUILDING && !kTeam.isHasTech((TechTypes)GC.getSpecialBuildingInfo(eSpecial).getTechPrereq()) ? 1 : 0;

						FAssert(iMissingTechs > 0);
					}
				}
				if (bCorpTech)
				{
					int iCorpValue = AI_corporationValue((CorporationTypes)iJ); // roughly 100x commerce / turn / city.
					iCorpValue = iCorpValue * iCityCount * 2 / 100; // rescaled
					if (iMissingTechs == 0)
						iCorpValue = 3 * iCorpValue / 2; // this won't happen in standard BtS - but it might in some mods.
					else
						iCorpValue /= iMissingTechs;

					if (iCorpValue > 0)
					{
						if (isNoCorporations())
							iCorpValue /= 4;

						if (AI_getFlavorValue(FLAVOR_GOLD) > 0)
						{
							iCorpValue *= 20 + AI_getFlavorValue(FLAVOR_GOLD);
							iCorpValue /= 20;
							iValue += iCorpValue/2;
							iValue += ((bAsync) ? GC.getASyncRand().get(iCorpValue, "AI Research Corporation ASYNC") : GC.getGameINLINE().getSorenRandNum(iCorpValue, "AI Research Corporation"));
							iRandomMax += iCorpValue;
						}
						else
						{
							iValue += iCorpValue/3;
							iValue += ((bAsync) ? GC.getASyncRand().get(4*iCorpValue/3, "AI Research Corporation ASYNC") : GC.getGameINLINE().getSorenRandNum(4*iCorpValue/3, "AI Research Corporation"));
							iRandomMax += 4*iCorpValue/3;
						}
					}
				}
			}
			// K-Mod end
		}

		//if (GC.getGameINLINE().countKnownTechNumTeams(eTech) == 0)
		// K-Mod
		if (bFirst)
		{
			int iRaceModifier = 0; // 100 means very likely we will be first, -100 means very unlikely. 0 is 'unknown'.
			{
				int iTotalPlayers = 0;
				int iCount = 0;
				for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i = (PlayerTypes)(i+1))
				{
					const CvPlayer& kLoopPlayer = GET_PLAYER(i);
					if (kLoopPlayer.getTeam() != getTeam() && kLoopPlayer.isAlive())
					{
						iTotalPlayers++; // count players even if we haven't met them. (we know they're out there...)
						if (kTeam.isHasMet(kLoopPlayer.getTeam()) && (iPathLength <= 1 != kLoopPlayer.canResearch(eTech)))
						{
							iCount++; // if path is <= 1, count civs who can't research it. if path > 1, count civs who can research it.
						}
					}
				}
				iRaceModifier = (iPathLength <= 1 ? 100 : -100) * iCount / std::max(1, iTotalPlayers);
			}
			FAssert(iRaceModifier >= -100 && iRaceModifier <= 100);
		// K-Mod end
			int iReligionValue = 0;
			int iPotentialReligions = 0;
			int iAvailableReligions = 0; // K-Mod
			for (int iJ = 0; iJ < GC.getNumReligionInfos(); iJ++)
			{
				TechTypes eReligionTech = (TechTypes)GC.getReligionInfo((ReligionTypes)iJ).getTechPrereq();
				/* original bts code
				if (kTeam.isHasTech(eReligionTech))
				{
					if (!(GC.getGameINLINE().isReligionSlotTaken((ReligionTypes)iJ)))
					{
						iPotentialReligions++;
					}
				} */
				// K-Mod. iPotentialReligions will only be non-zero during the first few turns of advanced start games. Otherwise it is always zero.
				// Presumably that's what the original developers intended... so I'm going to leave that alone, and create a new value: iAvailableReligions.
				if (!GC.getGameINLINE().isReligionSlotTaken((ReligionTypes)iJ))
				{
					iAvailableReligions++;
					if (kTeam.isHasTech(eReligionTech))
						iPotentialReligions++;
				}
				// K-Mod end

				if (eReligionTech == eTech)
				{
					if (!(GC.getGameINLINE().isReligionSlotTaken((ReligionTypes)iJ)))
					{
						int iRoll = 150; // k146: was 400

						if (!GC.getGame().isOption(GAMEOPTION_PICK_RELIGION))
						{
							ReligionTypes eFavorite = (ReligionTypes)GC.getLeaderHeadInfo(getLeaderType()).getFavoriteReligion();
							if (eFavorite != NO_RELIGION)
							{
								if (iJ == eFavorite)
									iRoll = iRoll * 3/2;
								else
									iRoll = iRoll * 2/3;
							}
						}

						iRoll *= 200 + iRaceModifier;
						iRoll /= 200;
						if (iRaceModifier > 10 && AI_getFlavorValue(FLAVOR_RELIGION) > 0)
							iReligionValue += iRoll * (iRaceModifier-10) / 300;

						iReligionValue += bAsync ? GC.getASyncRand().get(iRoll, "AI Research Religion ASYNC") : GC.getGameINLINE().getSorenRandNum(iRoll, "AI Research Religion");
						// Note: relation value will be scaled down by other factors in the next section.
						iRandomMax += iRoll; // (Note: this doesn't include factors used later.)
					}
				}
			}
			
			if (iReligionValue > 0)
			{
				if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1))
				{
					iReligionValue += 50; // k146: was 100

					if (countHolyCities() < 1)
					{
						//iReligionValue += 200;
						// k146: Replacing the line above
						iReligionValue += (iCityCount > 1 ? 100 : 0);
					}
				}
				else
				{
					iReligionValue /= (1 + countHolyCities() + ((iPotentialReligions > 0) ? 1 : 0));
				}

				if ((countTotalHasReligion() == 0) && (iPotentialReligions == 0))
				{
					bool bNeighbouringReligions = false;
					for (PlayerTypes i = (PlayerTypes)0; !bNeighbouringReligions && i < MAX_CIV_PLAYERS; i = (PlayerTypes)(i+1))
					{
						const CvPlayer& kLoopPlayer = GET_PLAYER(i);
						if (kLoopPlayer.isAlive() && kTeam.isHasMet(kLoopPlayer.getTeam()) &&
							kLoopPlayer.getStateReligion() != NO_RELIGION &&
							// <advc.001>
							((getTeam() != kLoopPlayer.getTeam() && kTeam.AI_hasSharedPrimaryArea(kLoopPlayer.getTeam())) ||
							(getTeam() == kLoopPlayer.getTeam() && getID() != kLoopPlayer.getID() && AI_hasSharedPrimaryArea(kLoopPlayer.getID()))))
							/* The case where both civs are on the same team previously led
							   to a failed assertion in CvTeamAI::hasSharedPrimaryArea.
							   Added a new clause that checks if the civs (not their
							   teams) share a primary area. Rationale: If a team member
							   on the same continent has already founded a religion,
							   we should be less inclined to found another (will soon
							   spread to us). </advc.001> */
						{
							bNeighbouringReligions = true;
						}
					}
					if (!bNeighbouringReligions)
					{
						iReligionValue += 20;
						if (AI_getFlavorValue(FLAVOR_RELIGION))
							iReligionValue += 28 + 4 * AI_getFlavorValue(FLAVOR_RELIGION);
						if (GC.getGameINLINE().getElapsedGameTurns() >= 32 * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getResearchPercent() / 100)
							iReligionValue += 60;
						if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1))
							iReligionValue += 84;
					}

					if (iAvailableReligions <= 4 || AI_getFlavorValue(FLAVOR_RELIGION) > 0)
					{
						iReligionValue *= 2;
						iReligionValue += 56 + std::max(0, 6 - iAvailableReligions)*28;
					}
					else
					{
						iReligionValue = iReligionValue*3/2;
						iReligionValue += 28;
					}
				}

				if (AI_isDoStrategy(AI_STRATEGY_DAGGER))
				{
					iReligionValue /= 2;
				}
				iValue += iReligionValue * std::min(iCityCount, iCityTarget) / std::max(1, iCityTarget);
			}

			// K-Mod note: I've moved corporation value outside of this block. (because you don't need to be first to the tech to get the corp!)

			if (getTechFreeUnit(eTech) != NO_UNIT)
			{
				// K-Mod
				int iRoll = 2 * (100 + AI_getGreatPersonWeight((UnitClassTypes)GC.getTechInfo(eTech).getFirstFreeUnitClass()));
				// I've diluted the weight because free great people doesn't have the negative effect of making it harder to get more great people
				iRoll *= 200 + iRaceModifier;
				iRoll /= 200;
				if (iRaceModifier > 20 && AI_getFlavorValue(FLAVOR_SCIENCE) + AI_getFlavorValue(FLAVOR_GROWTH) > 0)
					iValue += iRoll * (iRaceModifier-10) / 400;
				iValue += bAsync ? GC.getASyncRand().get(iRoll, "AI Research Great People ASYNC") : GC.getGameINLINE().getSorenRandNum(iRoll, "AI Research Great People");
				iRandomMax += iRoll;
				// K-Mod end
			}

			//iValue += (kTechInfo.getFirstFreeTechs() * (200 + ((bCapitalAlone) ? 400 : 0) + ((bAsync) ? GC.getASyncRand().get(3200, "AI Research Free Tech ASYNC") : GC.getGameINLINE().getSorenRandNum(3200, "AI Research Free Tech"))));
			// K-Mod. Very rough evaluation of free tech.
			if (kTechInfo.getFirstFreeTechs() > 0)
			{	// <k146>
				//int iRoll = 1600; // previous base value - which we'll now calculate based on costs of currently researchable tech
				int iBase = 100;

				{ // cf. free tech in AI_buildingValue
					int iTotalTechCost = 0;
					int iMaxTechCost = 0;
					int iTechCount = 0;

					for (TechTypes iI = (TechTypes)0; iI < GC.getNumTechInfos(); iI=(TechTypes)(iI+1))
					{
						if (canResearchBulk(iI, false, true))
						{
							int iTechCost = kTeam.getResearchCost(iI);
							iTotalTechCost += iTechCost;
							iTechCount++;
							iMaxTechCost = std::max(iMaxTechCost, iTechCost);
						}
					}
					if (iTechCount > 0)
					{
						int iTechValue =  ((iTotalTechCost / iTechCount) + iMaxTechCost)/2;

						iBase += iTechValue * 20 / GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getResearchPercent();
					}
				} // I'm expecting iRoll to be ~600.
				iBase *= 200 + iRaceModifier + (AI_getFlavorValue(FLAVOR_SCIENCE) ? 50 : 0);
				iBase /= 200;
				int iTempValue = iBase; // value for each free tech
				if (iRaceModifier > 20 && iRaceModifier < 100) // save the free tech if we have no competition!
					iTempValue += iBase * (iRaceModifier-20) / 200;
				//int iTempValue = (iRaceModifier >= 0 ? 196 : 98) + (bCapitalAlone ? 28 : 0); // some value regardless of race or random.
				iTempValue += bAsync ? GC.getASyncRand().get(iBase, "AI Research Free Tech ASYNC") : GC.getGameINLINE().getSorenRandNum(iBase, "AI Research Free Tech");
				// </k146>
				iValue += iTempValue * kTechInfo.getFirstFreeTechs();
				iRandomMax += iBase * kTechInfo.getFirstFreeTechs();
			}
			// K-Mod end
		}
	}

	iValue += kTechInfo.getAIWeight();

	if (!isHuman())
	{
		int iFlavorValue = 0;
		for (int iJ = 0; iJ < GC.getNumFlavorTypes(); iJ++)
		{
			iFlavorValue += AI_getFlavorValue((FlavorTypes)iJ) * kTechInfo.getFlavorValue(iJ) * 5;
		}
		iValue += iFlavorValue * std::min(iCityCount, iCityTarget) / std::max(1, iCityTarget);
	}

	if (kTechInfo.isRepeat())
	{
		iValue /= 4;
	}
	// <k146>
	if (bFreeTech)
	{
		iValue += getResearchTurnsLeft(eTech, false) / 5;
		iValue *= 1000; // roughly normalise with usual value. (cf. code below)
		iValue /= 10 * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).
				getResearchPercent();
	} // </k146>
	else
	{
		if (iValue > 0)
		{
			//this stops quick speed messing up.... might want to adjust by other things too...
			int iAdjustment = GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getResearchPercent();
	
			// Shouldn't run this during anarchy
			int iTurnsLeft = getResearchTurnsLeftTimes100((eTech), false);
			bool bCheapBooster = ((iTurnsLeft < (2 * iAdjustment)) && (0 == ((bAsync) ? GC.getASyncRand().get(5, "AI Choose Cheap Tech") : GC.getGameINLINE().getSorenRandNum(5, "AI Choose Cheap Tech"))));
			
			
			//iValue *= 100000;
			iValue *= 1000; // k146
			
            iValue /= (iTurnsLeft + (bCheapBooster ? 1 : 5) * iAdjustment);
		}
	}
	
	//Tech Groundbreaker
	//if (!GC.getGameINLINE().isOption(GAMEOPTION_NO_TECH_TRADING))
	if (!GC.getGameINLINE().isOption(GAMEOPTION_NO_TECH_TRADING) && !isBarbarian() // K-Mod
			 && iPathLength <= 1) // k146
	{
		if (kTechInfo.isTechTrading() || kTeam.isTechTrading())
		{
			// K-Mod. We should either consider 'tech ground breaking' for all techs this turn, or none at all - otherwise it will just mess things up.
			// Also, if the value adjustment isn't too big, it should be ok to do this most of the time.
			if (AI_getStrategyRand(GC.getGameINLINE().getGameTurn()) % std::max(1, GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_TRADE_TECH)) == 0)
			{
				int iAlreadyKnown = 0;
				int iPotentialTrade = 0;
				for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i = (TeamTypes)(i+1))
				{
					const CvTeamAI& kLoopTeam = GET_TEAM(i);
					if (i != getTeam() && kLoopTeam.isAlive() && kTeam.isHasMet(i))
					{
						if (kLoopTeam.isHasTech(eTech) || (canSeeResearch(kLoopTeam.getLeaderID()) && GET_PLAYER(kLoopTeam.getLeaderID()).getCurrentResearch() == eTech))
							iAlreadyKnown++;
						else if (!kTeam.isAtWar(i) && kLoopTeam.AI_techTrade(NO_TECH, getTeam()) == NO_DENIAL && kTeam.AI_techTrade(NO_TECH, i) == NO_DENIAL)
							iPotentialTrade++;
					}
				}

				int iTradeModifier = iPotentialTrade * (GC.getGameINLINE().isOption(GAMEOPTION_NO_TECH_BROKERING) ?
						30 : 15); // k146: was 20:12
				iTradeModifier *= 200 - GC.getLeaderHeadInfo(getPersonalityType()).getTechTradeKnownPercent();
				iTradeModifier /= 150;
				iTradeModifier /= 1 + 2 * iAlreadyKnown;
				iValue *= 100 + std::min(100, iTradeModifier);
				iValue /= 100;
			}
			// K-Mod end
		}
	}

	if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3))
	{
		int iCVValue = AI_cultureVictoryTechValue(eTech);
		/* original bts code
		iValue *= (iCVValue + 10);
		iValue /= ((iCVValue < 100) ? 400 : 100); */
		// K-Mod. I don't think AI_cultureVictoryTechValue is accurate enough to play such a large role.
		iValue *= (iCVValue + 100);
		iValue /= 200;
		// K-Mod
	}

/***
**** K-Mod, 12/sep/10, Karadoc
**** Use a random _factor_ at the end.
***/
	iRandomFactor = ((bAsync) ? GC.getASyncRand().get(
			200, "AI Research factor ASYNC") : // k146: was 100
			GC.getGameINLINE().getSorenRandNum(
			200, "AI Research factor")); // k146: was 100
	// k146: was 950+...
	iValue *= (900 + iRandomFactor); // between 90% and 110%
	iValue /= 1000;
/***
**** END
***/

	FAssert(iValue < INT_MAX);
	iValue = range(iValue, 0, INT_MAX);

	return iValue;
}

// K-mod. This function returns the (positive) value of the buildings we will lose by researching eTech.
// (I think it's crazy that this stuff wasn't taken into account in original BtS)
int CvPlayerAI::AI_obsoleteBuildingPenalty(TechTypes eTech, bool bConstCache) const
{
	int iTotalPenalty = 0;
	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingTypes eLoopBuilding = ((BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI)));

		if (eLoopBuilding != NO_BUILDING)
		{
			bool bObsolete = GC.getBuildingInfo(eLoopBuilding).getObsoleteTech() == eTech
				|| (GC.getBuildingInfo(eLoopBuilding).getSpecialBuildingType() != NO_SPECIALBUILDING
				&& GC.getSpecialBuildingInfo((SpecialBuildingTypes)GC.getBuildingInfo(eLoopBuilding).getSpecialBuildingType()).getObsoleteTech() == eTech);

			if (bObsolete && getBuildingClassCount((BuildingClassTypes)iI) > 0)
			{
				int iLoop;
				for (const CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
				{
					int n = pLoopCity->getNumActiveBuilding(eLoopBuilding);
					if (n > 0)
					{
						iTotalPenalty += n * pLoopCity->AI_buildingValue(eLoopBuilding, 0, 0, bConstCache);
					}
				}
			}
		}
	}
	// iScale is set more or less arbitrarily based on trial and error.
	int iScale = 50;

	if (AI_getFlavorValue(FLAVOR_SCIENCE) > 0)
		iScale -= 10 + AI_getFlavorValue(FLAVOR_SCIENCE);
	iScale = std::max(10, iScale);

	iTotalPenalty *= iScale;
	iTotalPenalty /= 100;

	return iTotalPenalty;
}

// K-Mod. The original code had some vague / ad-hoc calculations to evalute the buildings enabled by techs.
// It was completely separate from CvCity::AI_buildingValue, and it ignored _most_ building functionality.
// I've decided it would be better to use the thorough calculations already present in AI_buildingValue.
// This should make the AI a bit smarter and the code more easy to update. But it might run slightly slower.
//
// Note: There are some cases where this new kind of evaluation is significantly flawed. For example,
// Assembly Line enables factories and coal power plants; since these two buildings are highly dependant on
// one another, their individual evaluation will greatly undervalue them. Another example: catherals can't
// be built in every city, but this function will evaluate them as if they could, thus overvaluing them.
// But still, the original code was far worse - so I think I'll just tolerate such flaws for now.
// The scale is roughly 4 = 1 commerce per turn.
int CvPlayerAI::AI_techBuildingValue(TechTypes eTech, bool bConstCache, bool& bEnablesWonder) const
{
	PROFILE_FUNC();
	FAssertMsg(!isAnarchy()
		|| isHuman(), /* advc.001: If a human-controlled civ needs to choose
		new research while in anarchy, the AI needs to recommend a tech. An
		inaccurate recommendation is better than a failed assertion at runtime. */
		"AI_techBuildingValue should not be used while in anarchy. Results will be inaccurate.");
	/*  advc.001n: Don't rely on caller to initialize this. Initialization was
		still present in the BBAI code, must've been lost in the K-Mod rewrite. */
	bEnablesWonder = false;
	int iTotalValue = 0;
	std::vector<const CvCity*> relevant_cities; // (this will be populated when we find a building that needs to be evaluated)

	for (BuildingClassTypes eClass = (BuildingClassTypes)0; eClass < GC.getNumBuildingClassInfos(); eClass=(BuildingClassTypes)(eClass+1))
	{
		BuildingTypes eLoopBuilding = ((BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(eClass));

		if (eLoopBuilding == NO_BUILDING || !isTechRequiredForBuilding(eTech, eLoopBuilding))
			continue; // this building class is not relevent

		const CvBuildingInfo& kLoopBuilding = GC.getBuildingInfo(eLoopBuilding);

		if (GET_TEAM(getTeam()).isObsoleteBuilding(eLoopBuilding))
			continue; // already obsolete

		if (isWorldWonderClass(eClass))
		{
			if (GC.getGameINLINE().isBuildingClassMaxedOut(eClass) || kLoopBuilding.getProductionCost() < 0)
				continue; // either maxed out, or it's a special building that we don't want to evaluate here.

			if (kLoopBuilding.getPrereqAndTech() == eTech)
				bEnablesWonder = true; // a buildable world wonder
		}

		bool bLimitedBuilding = isLimitedWonderClass(eClass) || kLoopBuilding.getProductionCost() < 0;

		// Populate the relevant_cities list if we haven't done so already.
		if (relevant_cities.empty())
		{
			int iEarliestTurn = INT_MAX;

			int iLoop;
			for (const CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
			{
				iEarliestTurn = std::min(iEarliestTurn, pLoopCity->getGameTurnAcquired());
			}

			int iCutoffTurn = (GC.getGameINLINE().getGameTurn() + iEarliestTurn) / 2 + GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getVictoryDelayPercent() * 30 / 100;
			// iCutoffTurn corresponds 50% of the time since our first city was aquired, with a 30 turn (scaled) buffer.

			for (const CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
			{
				if (pLoopCity->getGameTurnAcquired() < iCutoffTurn || pLoopCity->isCapital())
					relevant_cities.push_back(pLoopCity);
			}

			if (relevant_cities.empty())
			{
				FAssertMsg(isBarbarian(), "No revelent cities in AI_techBuildingValue");
				return 0;
			}
		}
		//

		// Perform the evaluation.
		int iBuildingValue = 0;
		for (size_t i = 0; i < relevant_cities.size(); i++)
		{
			if (relevant_cities[i]->canConstruct(eLoopBuilding, false, false, true, true) ||
				isNationalWonderClass(eClass)) // (ad-hoc). National wonders often require something which is unlocked by the same tech. So I'll disregard the construction requirements.
			{
				if (bLimitedBuilding) // TODO: don't assume 'limited' means 'only one'.
					iBuildingValue = std::max(iBuildingValue, relevant_cities[i]->AI_buildingValue(eLoopBuilding, 0, 0, bConstCache));
				else
					iBuildingValue += relevant_cities[i]->AI_buildingValue(eLoopBuilding, 0, 0, bConstCache);
			}
		}
		if (iBuildingValue > 0)
		{
			// Scale the value based on production cost and special production multipliers.
			if (kLoopBuilding.getProductionCost() > 0)
			{
				int iMultiplier = 0;
				for (TraitTypes i = (TraitTypes)0; i < GC.getNumTraitInfos(); i=(TraitTypes)(i+1))
				{
					if (hasTrait(i))
					{
						iMultiplier += kLoopBuilding.getProductionTraits(i);

						if (kLoopBuilding.getSpecialBuildingType() != NO_SPECIALBUILDING)
						{
							iMultiplier += GC.getSpecialBuildingInfo((SpecialBuildingTypes)kLoopBuilding.getSpecialBuildingType()).getProductionTraits(i);
						}
					}
				}

				for (BonusTypes i = (BonusTypes)0; i < GC.getNumBonusInfos(); i=(BonusTypes)(i+1))
				{
					if (hasBonus(i))
						iMultiplier += kLoopBuilding.getBonusProductionModifier(i);
				}
				int iScale = 15 * (3 + getCurrentEra()); // hammers (ideally this would be based on the average city yield or something like that.)
				iScale += AI_isCapitalAreaAlone() ? 30 : 0; // can afford to spend more on infrastructure if we are alone.
				// decrease when at war
				if(isFocusWar()) // advc.105
				//if (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0)
					iScale = iScale * 2/3;
				// increase scale for limited wonders, because they don't need to be built in every city.
				if (isLimitedWonderClass(eClass))
					iScale *= std::min((int)relevant_cities.size(), GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities());
				// adjust for game speed
				iScale = iScale * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getBuildPercent() / 100;
				// use the multiplier we calculated earlier
				iScale = iScale * (100 + iMultiplier) / 100;
				//
				iBuildingValue *= 100;
				iBuildingValue /= std::max(33, 100 * kLoopBuilding.getProductionCost() / std::max(1, iScale));
			}
			//
			iTotalValue += iBuildingValue;
		}
	}

	int iScale = AI_isDoStrategy(AI_STRATEGY_ECONOMY_FOCUS) ? 180 : 100;

	if (getNumCities() == 1 && getCurrentEra() == GC.getGameINLINE().getStartEra())
		iScale/=2; // I expect we'll want to be building mostly units until we get a second city.

	iTotalValue *= iScale;
	iTotalValue /= 120;

	return iTotalValue;
}
// K-Mod end

// This function has been mostly rewritten for K-Mod
// The final scale is roughly 4 = 1 commerce per turn.
int CvPlayerAI::AI_techUnitValue(TechTypes eTech, int iPathLength, bool& bEnablesUnitWonder) const
{
	PROFILE_FUNC();
	const CvTeamAI& kTeam = GET_TEAM(getTeam()); // K-Mod

	bool bWarPlan = isFocusWar(); // advc.105
			//(kTeam.getAnyWarPlanCount(true) > 0);
	if( !bWarPlan )
	{
		// Aggressive players will stick with war techs
		if (kTeam.AI_getTotalWarOddsTimes100() > 400)
		{
			bWarPlan = true;
		}
	}
	// <k146> Get some basic info.
	bool bLandWar = false;
	bool bIsAnyAssault = false;
	{
		int iLoop;
		for (CvArea* pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
		{
			if (AI_isPrimaryArea(pLoopArea))
			{
				switch (pLoopArea->getAreaAIType(getTeam()))
				{
					case AREAAI_OFFENSIVE:
					case AREAAI_DEFENSIVE:
					case AREAAI_MASSING:
						bLandWar = true;
						break;
					case AREAAI_ASSAULT:
					case AREAAI_ASSAULT_MASSING:
					case AREAAI_ASSAULT_ASSIST:
						bIsAnyAssault = true;
						break;
					default:
						break;
				};
			}
		}
	} // </k146>
	bool bCapitalAlone = (GC.getGameINLINE().getElapsedGameTurns() > 0) ? AI_isCapitalAreaAlone() : false;
	int iHasMetCount = kTeam.getHasMetCivCount(true);
	int iCoastalCities = countNumCoastalCities();
	CvCity* pCapitalCity = getCapitalCity();

	int iValue = 0;

	bEnablesUnitWonder = false;
	for (UnitClassTypes eLoopClass = (UnitClassTypes)0; eLoopClass < GC.getNumUnitClassInfos(); eLoopClass = (UnitClassTypes)(eLoopClass+1))
	{
		UnitTypes eLoopUnit = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(eLoopClass);

		if (eLoopUnit == NO_UNIT || !isTechRequiredForUnit(eTech, eLoopUnit))
			continue;

		CvUnitInfo& kLoopUnit = GC.getUnitInfo(eLoopUnit);
		/*  <k146> Raw value moved here, so that it is adjusted by missing
			techs / resources */
		//iValue += 200;
		int iTotalUnitValue = 200; // </k146>

		int iNavalValue = 0;
		int iOffenceValue = 0;
		int iDefenceValue = 0;
		/*  k146: Total military value. Offence and defence are added to this
			after all roles have been considered. */
		int iMilitaryValue = 0;
		int iUtilityValue = 0;

		if (GC.getUnitClassInfo(eLoopClass).getDefaultUnitIndex() != GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(eLoopClass))
		{
			//UU
			iTotalUnitValue += 600;
		}

		if (kLoopUnit.getPrereqAndTech() == eTech || kTeam.isHasTech((TechTypes)kLoopUnit.getPrereqAndTech()) || canResearch((TechTypes)kLoopUnit.getPrereqAndTech()))
		{
			// (note, we already checked that this tech is required for the unit.)
			for (UnitAITypes eAI = (UnitAITypes)0; eAI < NUM_UNITAI_TYPES; eAI = (UnitAITypes)(eAI+1))
			{
				int iWeight = 0;
				if (eAI == kLoopUnit.getDefaultUnitAIType())
				{
					// Score the default AI type as a direct competitor to the current best.
					iWeight = std::min(250, 70 * // k146
							AI_unitValue(eLoopUnit, eAI, 0) /
							std::max(1, AI_bestAreaUnitAIValue(eAI, 0)));
				}
				else if (kLoopUnit.getUnitAIType(eAI)) // only consider types which are flagged in the xml. (??)
				{
					// For the other AI types, only score based on the improvement over the current best.
					int iTypeValue = AI_unitValue(eLoopUnit, eAI, 0);
					if (iTypeValue > 0)
					{
						int iOldValue = AI_bestAreaUnitAIValue(eAI, 0);
						iWeight = std::min(150, // k146
								100 * (iTypeValue - iOldValue) / std::max(1, iOldValue));
					}
				}
				if (iWeight <= 0)
					continue;
				FAssert(iWeight <= 250); // k146

				switch (eAI)
				{
				case UNITAI_UNKNOWN:
				case UNITAI_ANIMAL:
					break;

				case UNITAI_SETTLE:
					iUtilityValue = std::max(iUtilityValue, 12*iWeight);
					break;

				case UNITAI_WORKER:
					iUtilityValue = std::max(iUtilityValue, 8*iWeight);
					break;

				case UNITAI_ATTACK:
					iOffenceValue = std::max(iOffenceValue, (bWarPlan ? 7 : 4)*iWeight + (AI_isDoStrategy(AI_STRATEGY_DAGGER) ? 5*iWeight : 0));
					iMilitaryValue += (bWarPlan ? 3 : 1)*iWeight; // k146
					break;

				case UNITAI_ATTACK_CITY:
					iOffenceValue = std::max(iOffenceValue, (bWarPlan ? 8 : 4)*iWeight + (AI_isDoStrategy(AI_STRATEGY_DAGGER) ? 6*iWeight : 0));
					iMilitaryValue += (bWarPlan ? 1 : 0)*iWeight; // k146
					break;

				case UNITAI_COLLATERAL:
					iDefenceValue = std::max(iDefenceValue, (bWarPlan ? 6 :
							3) // k146: was 4
							*iWeight + (AI_isDoStrategy(AI_STRATEGY_ALERT1) ? 2 : 0)*iWeight);
					// k146:
					iMilitaryValue += (bWarPlan ? 1 : 0)*iWeight + (bLandWar ? 2 : 0)*iWeight;
					break;

				case UNITAI_PILLAGE:
					iOffenceValue = std::max(iOffenceValue, (bWarPlan ?
							4 : // k146: was 2
							1)*iWeight);
					// <k146>
					iMilitaryValue += (bWarPlan ? 2 : 0)*iWeight +
							(bLandWar ? 2 : 0)*iWeight; // </k146>
					break;

				case UNITAI_RESERVE:
					iDefenceValue = std::max(iDefenceValue, (bWarPlan ?
							4 : 3 // k146: was 2:1
							)*iWeight);
					// <k146>
					iMilitaryValue += (iHasMetCount > 0 ? 2 : 1)*iWeight +
							(bWarPlan ? 2 : 0); // </k146>
					break;

				case UNITAI_COUNTER:
					iDefenceValue = std::max(iDefenceValue, (bWarPlan ?
							6 // k146: was 5
							: 3)*iWeight
							// k146:
							+ (AI_isDoStrategy(AI_STRATEGY_ALERT1) ? 2 : 0)*iWeight);
					iOffenceValue = std::max(iOffenceValue, (bWarPlan ?
							4 : // k146: was 2
							1)*iWeight + (AI_isDoStrategy(AI_STRATEGY_DAGGER) ? 3*iWeight : 0));
					// <k146>
					iMilitaryValue += (AI_isDoStrategy(AI_STRATEGY_ALERT1) ||
							bWarPlan ? 1 : 0)*iWeight + (bLandWar ? 1 : 0)*iWeight;
					// </k146>
					break;

				case UNITAI_PARADROP:
					iOffenceValue = std::max(iOffenceValue, (bWarPlan ? 6 : 3)*iWeight);
					break;

				case UNITAI_CITY_DEFENSE:
					// k146:
					iDefenceValue = std::max(iDefenceValue, (bWarPlan ? 8 : 6)*iWeight + (bCapitalAlone ? 0 : 2)*iWeight);
					// <k146>
					iMilitaryValue += (iHasMetCount > 0 ? 4 : 1)*iWeight +
							(AI_isDoStrategy(AI_STRATEGY_ALERT1) ||
							bWarPlan ? 3 : 0)*iWeight; // </k146>
					break;

				case UNITAI_CITY_COUNTER:
					iDefenceValue = std::max(iDefenceValue, (bWarPlan ? 8 :
							5)*iWeight); // k146: was 4)*...
					break;

				case UNITAI_CITY_SPECIAL:
					iDefenceValue = std::max(iDefenceValue, (bWarPlan ? 8 : 4)*iWeight);
					break;

				case UNITAI_EXPLORE:
					iUtilityValue = std::max(iUtilityValue, (bCapitalAlone ? 1 : 2)*iWeight);
					break;

				case UNITAI_MISSIONARY:
					//iTotalUnitValue += (getStateReligion() != NO_RELIGION ? 6 : 3)*iWeight;
					// K-Mod. There might be one missionary for each religion. Be careful not to overvalue them!
					{
						for (ReligionTypes i = (ReligionTypes)0; i < GC.getNumReligionInfos(); i = (ReligionTypes)(i+1))
						{
							if (kLoopUnit.getReligionSpreads(i) && getHasReligionCount(i) > 0)
							{
								iUtilityValue = std::max(iUtilityValue, (getStateReligion() == i ? 6 : 3)*iWeight);
							}
						}
					}
					// K-Mod end
					break;

				case UNITAI_PROPHET:
				case UNITAI_ARTIST:
				case UNITAI_SCIENTIST:
				case UNITAI_GENERAL:
				case UNITAI_MERCHANT:
				case UNITAI_ENGINEER:
				case UNITAI_GREAT_SPY: // K-Mod
					break;

				case UNITAI_SPY:
					//iMilitaryValue += (bWarPlan ? 100 : 50);
					// K-Mod
					if (iHasMetCount > 0) {
						iUtilityValue = std::max(iUtilityValue, (bCapitalAlone ? 1 : 2)*iWeight/2
								// <k146>
								+ (AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE) ?
								2 : 0)* iWeight); // </k146>
					}
					// K-Mod end
					break;

				case UNITAI_ICBM:
					//iMilitaryValue += ((bWarPlan) ? 200 : 100);
					// K-Mod
					if (!GC.getGameINLINE().isNoNukes())
					{
						iOffenceValue = std::max(iOffenceValue, (bWarPlan ? 2 : 1)*iWeight + (GC.getGameINLINE().isNukesValid() ? 2*AI_nukeWeight() : 0)*iWeight/100);
						FAssert(!GC.getGameINLINE().isNukesValid() || AI_nukeWeight() > 0);
					}
					// K-Mod end
					break;

				case UNITAI_WORKER_SEA:
					if (iCoastalCities > 0)
					{
						// note, workboat improvements are already counted in the improvement section
					}
					break;

				case UNITAI_ATTACK_SEA:
					// BBAI TODO: Boost value for maps where Barb ships are pestering us
					if (iCoastalCities > 0)
					{
						//iMilitaryValue += ((bWarPlan) ? 200 : 100);
						iOffenceValue = std::max(iOffenceValue, (bWarPlan ? 2 : 1) * (100 + kLoopUnit.getCollateralDamage()/2) * iWeight / 100);// K-Mod
					}
					iNavalValue = std::max(iNavalValue, 1*iWeight);
					break;

				case UNITAI_RESERVE_SEA:
					if (iCoastalCities > 0)
					{
						//iMilitaryValue += ((bWarPlan) ? 100 : 50);
						iOffenceValue = std::max(iOffenceValue, (bWarPlan ? 10 : 5) * (10 + kLoopUnit.getCollateralDamage()/20) * iWeight / 100);// K-Mod
						// K-Mod note: this naval value stuff seems a bit flakey...
					}
					iNavalValue = std::max(iNavalValue, 1*iWeight);
					break;

				case UNITAI_ESCORT_SEA:
					if (iCoastalCities > 0)
					{
						iOffenceValue = std::max(iOffenceValue, (bWarPlan ? 2 : 1)*iWeight/2);
					}
					iNavalValue = std::max(iNavalValue, 1*iWeight);
					break;

				case UNITAI_EXPLORE_SEA:
					if (iCoastalCities > 0)
					{
						//iTotalUnitValue += (bCapitalAlone ? 18 : 6)*iWeight;
						iUtilityValue = std::max(iUtilityValue,
								(4 + // k146: was 6+
								(bCapitalAlone ? 4 : 0) + (iHasMetCount > 0 ? 0 :
								4))*iWeight); // k146: was 6))...
					}
					break;

				case UNITAI_ASSAULT_SEA:
					if (iCoastalCities > 0)
					{
						iOffenceValue = std::max(iOffenceValue, (bWarPlan || bCapitalAlone ? 4 : 2)*iWeight);
					}
					iNavalValue = std::max(iNavalValue, 2*iWeight);
					break;

				case UNITAI_SETTLER_SEA:
					if (iCoastalCities > 0)
					{
						iUtilityValue = std::max(iUtilityValue,
								// k146:
								(bWarPlan ? 0 : 1)*iWeight + (bCapitalAlone ? 1 : 0)*iWeight);
					}
					iNavalValue = std::max(iNavalValue, 2*iWeight);
					break;

				case UNITAI_MISSIONARY_SEA:
					if (iCoastalCities > 0)
					{
						iUtilityValue = std::max(iUtilityValue, 1*iWeight);
					}
					break;

				case UNITAI_SPY_SEA:
					if (iCoastalCities > 0)
					{
						iUtilityValue = std::max(iUtilityValue, 1*iWeight);
					}
					break;

				case UNITAI_CARRIER_SEA:
					if (iCoastalCities > 0)
					{
						iOffenceValue = std::max(iOffenceValue, (bWarPlan ? 2 : 1)*iWeight/2);
					}
					break;

				case UNITAI_MISSILE_CARRIER_SEA:
					if (iCoastalCities > 0)
					{
						iOffenceValue = std::max(iOffenceValue, (bWarPlan ? 2 : 1)*iWeight/2);
					}
					break;

				case UNITAI_PIRATE_SEA:
					if (iCoastalCities > 0)
					{
						iOffenceValue = std::max(iOffenceValue, 1*iWeight);
					}
					iNavalValue = std::max(iNavalValue, 1*iWeight);
					break;

				case UNITAI_ATTACK_AIR:
					//iMilitaryValue += ((bWarPlan) ? 1200 : 800);
					// K-Mod
					iOffenceValue = std::max(iOffenceValue, (bWarPlan ? 10 : 5) * (100 + kLoopUnit.getCollateralDamage()*std::min(5, kLoopUnit.getCollateralDamageMaxUnits())/5) * iWeight / 100);
					break;

				case UNITAI_DEFENSE_AIR:
					//iMilitaryValue += ((bWarPlan) ? 1200 : 800);
					iDefenceValue = std::max(iDefenceValue, (bWarPlan ? 10 : 6)*iWeight + (bCapitalAlone ? 3 : 0)*iWeight);
					break;

				case UNITAI_CARRIER_AIR:
					if (iCoastalCities > 0)
					{
						iOffenceValue = std::max(iOffenceValue, (bWarPlan ? 2 : 1)*iWeight);
					}
					iNavalValue = std::max(iNavalValue, 4*iWeight);
					break;

				case UNITAI_MISSILE_AIR:
					iOffenceValue = std::max(iOffenceValue, (bWarPlan ? 2 : 1)*iWeight);
					break;

				default:
					FAssertMsg(false, "Missing UNITAI type in AI_techUnitValue");
					break;
				}
			}
			iTotalUnitValue += iUtilityValue;
			// <k146>
			iMilitaryValue += iOffenceValue + iDefenceValue;
			if (kLoopUnit.getBombardRate() > 0 &&
					!AI_isDoStrategy(AI_STRATEGY_ECONOMY_FOCUS))
			{	// block moved from UNITAI_ATTACK_CITY:
				iMilitaryValue += std::min(iOffenceValue, 100); // was straight 200
				// </k146>
				if (AI_calculateTotalBombard(DOMAIN_LAND) == 0)
				{
					iMilitaryValue += 600; // k146: was 800
					if (AI_isDoStrategy(AI_STRATEGY_DAGGER))
					{
						iMilitaryValue += 400; // was 1000
					}
				}
			}

			if (kLoopUnit.getUnitAIType(UNITAI_ASSAULT_SEA) && iCoastalCities > 0)
			{
				int iAssaultValue = 0;
				UnitTypes eExistingUnit = NO_UNIT;
				if (AI_bestAreaUnitAIValue(UNITAI_ASSAULT_SEA, NULL, &eExistingUnit) == 0)
				{
					iAssaultValue += 100; // k146: was 250
				}
				else if( eExistingUnit != NO_UNIT )
				{
					iAssaultValue += 500 * // k146: was 1000*
							std::max(0, AI_unitImpassableCount(eLoopUnit) - AI_unitImpassableCount(eExistingUnit));

					int iNewCapacity = kLoopUnit.getMoves() * kLoopUnit.getCargoSpace();
					int iOldCapacity = GC.getUnitInfo(eExistingUnit).getMoves() * GC.getUnitInfo(eExistingUnit).getCargoSpace();

					iAssaultValue += (200 * // k146: was 800*
							(iNewCapacity - iOldCapacity)) / std::max(1, iOldCapacity);
				}

				if (iAssaultValue > 0)
				{
					// k146: (bIsAnyAssault calculation removed; now done earlier)
					if (bIsAnyAssault)
					{
						iTotalUnitValue += iAssaultValue * 4;
					}
					else
					{
						iTotalUnitValue += iAssaultValue;
					}
				}
			}

			if (iNavalValue > 0)
			{
				if (getCapitalCity() != NULL)
				{
					// BBAI TODO: A little odd ... naval value is 0 if have no colonies.
					iNavalValue *= 2 * (getNumCities() - getCapitalCity()->area()->getCitiesPerPlayer(getID()));
					iNavalValue /= getNumCities();

					iTotalUnitValue += iNavalValue;
				}
			}
			// k146: Disabled
			/*if (AI_totalUnitAIs((UnitAITypes)kLoopUnit.getDefaultUnitAIType()) == 0)
			{
				// do not give bonus to seagoing units if they are worthless
				if (iTotalUnitValue > 0)
				{
					iTotalUnitValue *= 2;
				}

				if (kLoopUnit.getDefaultUnitAIType() == UNITAI_EXPLORE)
				{
					if (pCapitalCity != NULL)
					{
						iTotalUnitValue += (AI_neededExplorers(pCapitalCity->area()) * 400);
					}
				}

				if (kLoopUnit.getDefaultUnitAIType() == UNITAI_EXPLORE_SEA)
				{
					iTotalUnitValue += 400;
					iTotalUnitValue += ((GC.getGameINLINE().countCivTeamsAlive() - iHasMetCount) * 200);
				}
			}*/

			if (kLoopUnit.getUnitAIType(UNITAI_SETTLER_SEA))
			{
				if (getCapitalCity() != NULL)
				{
					UnitTypes eExistingUnit = NO_UNIT;
					int iBestAreaValue = 0;
					AI_getNumAreaCitySites(getCapitalCity()->getArea(), iBestAreaValue);

					//Early Expansion by sea
					if (AI_bestAreaUnitAIValue(UNITAI_SETTLER_SEA, NULL, &eExistingUnit) == 0)
					{
						CvArea* pWaterArea = getCapitalCity()->waterArea();
						if (pWaterArea != NULL)
						{
							int iBestOtherValue = 0;
							AI_getNumAdjacentAreaCitySites(pWaterArea->getID(), getCapitalCity()->getArea(), iBestOtherValue);

							if (iBestAreaValue == 0
									// k146: Give us a chance to see our land first.
									&& GC.getGameINLINE().getElapsedGameTurns() > 20)
							{
								iTotalUnitValue += 2000;
							}
							else if (iBestAreaValue < iBestOtherValue)
							{
								iTotalUnitValue += 1000;
							}
							else if (iBestOtherValue > 0)
							{
								iTotalUnitValue += 500;
							}
						}
					}
					// Landlocked expansion over ocean
					else if( eExistingUnit != NO_UNIT )
					{
						if( AI_unitImpassableCount(eLoopUnit) < AI_unitImpassableCount(eExistingUnit) )
						{
							if( iBestAreaValue < AI_getMinFoundValue() )
							{
								iTotalUnitValue += (AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION2) ? 2000 : 500);
							}
						}
					}
				}
			}

			if (iMilitaryValue > 0)
			{
				if (iHasMetCount == 0 || AI_isDoStrategy(AI_STRATEGY_ECONOMY_FOCUS))
				{
					iMilitaryValue /= 2;
				}

				// K-Mod
				if (AI_isDoStrategy(AI_STRATEGY_GET_BETTER_UNITS) && kLoopUnit.getDomainType() == DOMAIN_LAND)
				{
					iMilitaryValue += 3 * GC.getGameINLINE().AI_combatValue(eLoopUnit);
					iMilitaryValue *= 3;
					iMilitaryValue /= 2;
				}

				// This multiplier stuff is basically my version of the BBAI code I disabled further up.
				int iMultiplier = 100;
				if (AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST1 | AI_VICTORY_DOMINATION2) || AI_isDoStrategy(AI_STRATEGY_ALERT1))
				{
					iMultiplier += 25;
					if (AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST2 | AI_VICTORY_DOMINATION3))
					{
						iMultiplier += 25;
						if (AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3 | AI_VICTORY_DOMINATION4))
						{
							iMultiplier += 25;
						}
					}
				}
				if (AI_isDoStrategy(AI_STRATEGY_ALERT1 | AI_STRATEGY_TURTLE)
					&& (kLoopUnit.getUnitAIType(UNITAI_CITY_DEFENSE)
					|| kLoopUnit.getUnitAIType(UNITAI_CITY_SPECIAL)
					|| kLoopUnit.getUnitAIType(UNITAI_CITY_COUNTER)
					|| kLoopUnit.getUnitAIType(UNITAI_COLLATERAL)))
				{
					iMultiplier += AI_isDoStrategy(AI_STRATEGY_ALERT2) ? 70 : 30;
					if (iPathLength <= 1 && AI_isDoStrategy(AI_STRATEGY_TURTLE))
					{
						iMultiplier += 75;
					}
				}
				iMilitaryValue *= iMultiplier;
				iMilitaryValue /= 100;

				if (bCapitalAlone && iMultiplier <= 100) // I've moved this block from above, and added the multiplier condition.
				{
					iMilitaryValue *= 2;
					iMilitaryValue /= 3;
				}
				// K-Mod end

				iTotalUnitValue += iMilitaryValue;
			}
		}
		// <k146>
		if(isWorldUnitClass(eLoopClass) &&
				!GC.getGameINLINE().isUnitClassMaxedOut(eLoopClass))
			bEnablesUnitWonder = true; // </k146>
		// K-Mod
		// Decrease the value if we are missing other prereqs.
		{
			int iMissingTechs = 0;
			if (!kTeam.isHasTech((TechTypes)kLoopUnit.getPrereqAndTech()))
				iMissingTechs++;

			for (int iI = 0; iI < GC.getNUM_UNIT_AND_TECH_PREREQS(); iI++)
			{
				if (!kTeam.isHasTech((TechTypes)kLoopUnit.getPrereqAndTechs(iI)))
				{
					iMissingTechs++;
					if (!canResearch((TechTypes)kLoopUnit.getPrereqAndTechs(iI)))
						iMissingTechs++;
				}
			}
			FAssert(iMissingTechs > 0);
			iTotalUnitValue /= std::max(1, iMissingTechs);
		}

		// Decrease the value if we don't have the resources
		bool bMaybeMissing = false; // if we don't know if we have the bonus or not
		bool bDefinitelyMissing = false; // if we can see the bonuses, and we know we don't have any.
		// k146: if the 'maybe missing' resource is revealed by eTech
		bool bWillReveal = false;

		for (int iI = 0; iI < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); ++iI)
		{
			BonusTypes ePrereqBonus = (BonusTypes)kLoopUnit.getPrereqOrBonuses(iI);
			if (ePrereqBonus != NO_BONUS)
			{
				if (hasBonus(ePrereqBonus))
				{
					bDefinitelyMissing = false;
					bMaybeMissing = false;
					break;
				}
				else
				{
					if ((kTeam.isHasTech((TechTypes)(GC.getBonusInfo(ePrereqBonus).getTechReveal())) || kTeam.isForceRevealedBonus(ePrereqBonus)) && countOwnedBonuses(ePrereqBonus) == 0)
					{
						bDefinitelyMissing = true;
					}
					else
					{
						bMaybeMissing = true;
						// <k146>
						bDefinitelyMissing = false;
						if (GC.getBonusInfo(ePrereqBonus).getTechReveal() == eTech)
						{
							bWillReveal = true;
						} // </k146>
					}
				}
			}
		}
		BonusTypes ePrereqBonus = (BonusTypes)kLoopUnit.getPrereqAndBonus();
		if (ePrereqBonus != NO_BONUS && !hasBonus(ePrereqBonus))
		{
			if ((kTeam.isHasTech((TechTypes)(GC.getBonusInfo(ePrereqBonus).getTechReveal())) || kTeam.isForceRevealedBonus(ePrereqBonus)) &&
				countOwnedBonuses(ePrereqBonus) == 0)
			{
				bDefinitelyMissing = true;
			}
			else
			{
				bMaybeMissing = true;
				// <k146>
				if (GC.getBonusInfo(ePrereqBonus).getTechReveal() == eTech)
				{
					// assuming that we aren't also missing an "or" prereq.
					bWillReveal = true;
				} // </k146>
			}
		}
		if (bDefinitelyMissing)
		{
			iTotalUnitValue /= 4; // advc.131: was /=3
		}
		else if (bMaybeMissing)
		{	// <k146>
			if (bWillReveal)
			{
				// We're quite optimistic... mostly because otherwise we'd risk undervaluing axemen in the early game! (Kludge, sorry.)
				iTotalUnitValue *= 4;
				iTotalUnitValue /= 5;
			}
			else
			{
				//iTotalUnitValue = 2*iTotalUnitValue/3;
				iTotalUnitValue = iTotalUnitValue/2; // better get the reveal tech first
			} // </k146>
		}
		// K-Mod end

		iValue += iTotalUnitValue;
	}

	// K-Mod. Rescale to match AI_techValue
	iValue *= (4 * (getNumCities()
			+ 2)); // k146
	// k146: was /=100. This entire calculation is quite arbitrary. :(
	iValue /= 120;
	//
	return iValue;
}

/*  <k146> A (very rough) estimate of the value of projects enabled by eTech.
	Note: not a lot of thought has gone into this. I've basically just copied
	the original code [from AI_techValue] and tweaked it a little bit.
	The scale is roughly 4 = 1 commerce per turn. */
int CvPlayerAI::AI_techProjectValue(TechTypes eTech, int iPathLength, bool& bEnablesProjectWonder) const
{
	int iValue = 0;
	bEnablesProjectWonder = false;
	for (int iJ = 0; iJ < GC.getNumProjectInfos(); iJ++)
	{
		const CvProjectInfo& kProjectInfo = GC.getProjectInfo((ProjectTypes)iJ);
		// <advc.003> Compacted
		if(kProjectInfo.getTechPrereq() != eTech)
			continue;
		if(getTotalPopulation() > 5 &&  isWorldProject((ProjectTypes)iJ) &&
				!GC.getGameINLINE().isProjectMaxedOut((ProjectTypes)iJ))
			bEnablesProjectWonder = true;
		iValue += 280;
		if((VictoryTypes)kProjectInfo.getVictoryPrereq() == NO_VICTORY)
			continue; // </advc.003>
		// Victory condition values need to be scaled by number of cities to compete with other tech.
		// Total value with be roughly iBaseValue * cities.
		int iBaseValue = 0;
		if( !kProjectInfo.isSpaceship() )
		{
			// Apollo
			iBaseValue += (AI_isDoVictoryStrategy(AI_VICTORY_SPACE2) ? 50 : 2);
		}
		else
		{
			// Space ship parts (changed by K-Mod)
			// Note: ideally this would take into account the production cost of each item,
			//       and the total number / production of a completed space-ship, and a
			//       bunch of other things. But I think this is good enough for now.
			if (AI_isDoVictoryStrategy(AI_VICTORY_SPACE2))
			{
				iBaseValue += 40;
				if (AI_isDoVictoryStrategy(AI_VICTORY_SPACE3))
				{
					iBaseValue += 40;
					if (kProjectInfo.getMaxTeamInstances() > 0)
					{
						iBaseValue += (kProjectInfo.
								getMaxTeamInstances()-1) * 10;
					}
				}
			}
		}
		if (iBaseValue > 0)
		{
			iValue += iBaseValue * (3 + std::max(getNumCities(),
					GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).
					getTargetNumCities()));
		}
	}
	return iValue;
} // </k146>

int CvPlayerAI::AI_cultureVictoryTechValue(TechTypes eTech) const
{
	if (eTech == NO_TECH)
	{
		return 0;
	}

	int iValue = 0;

	if (GC.getTechInfo(eTech).isDefensivePactTrading())
	{
		iValue += 50;
	}

	if (GC.getTechInfo(eTech).isCommerceFlexible(COMMERCE_CULTURE))
	{
		iValue += 100;
	}

	//units
	bool bAnyWarplan = isFocusWar(); // advc.105
			//(GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0);
	int iBestUnitValue = 0;
	for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		UnitTypes eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

		if (eLoopUnit != NO_UNIT)
		{
			if (isTechRequiredForUnit((eTech), eLoopUnit))
			{
				int iTempValue = (GC.getUnitInfo(eLoopUnit).getCombat() * 100) / std::max(1, (GC.getGame().getBestLandUnitCombat()));
				iTempValue *= bAnyWarplan ? 2 : 1;

				iValue += iTempValue / 3;
				iBestUnitValue = std::max(iBestUnitValue, iTempValue);
			}
		}
	}
	iValue += std::max(0, iBestUnitValue - 15);

	//cultural things
	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingTypes eLoopBuilding = ((BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI)));

		if (eLoopBuilding != NO_BUILDING)
		{
			if (isTechRequiredForBuilding((eTech), eLoopBuilding))
			{
				CvBuildingInfo& kLoopBuilding = GC.getBuildingInfo(eLoopBuilding);

				if ((GC.getBuildingClassInfo((BuildingClassTypes)iI).getDefaultBuildingIndex()) != (GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI)))
				{
					//UB
					iValue += 100;
				}

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       05/25/10                          Fuyu & jdog5000     */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original bts code
				iValue += (150 * kLoopBuilding.getCommerceChange(COMMERCE_CULTURE)) * 20;
*/
				iValue += (150 * (kLoopBuilding.getCommerceChange(COMMERCE_CULTURE) + kLoopBuilding.getObsoleteSafeCommerceChange(COMMERCE_CULTURE))) / 20;
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
				iValue += kLoopBuilding.getCommerceModifier(COMMERCE_CULTURE) * 2;
			}
		}
	}

	//important civics
	for (int iI = 0; iI < GC.getNumCivicInfos(); iI++)
	{
		if (GC.getCivicInfo((CivicTypes)iI).getTechPrereq() == eTech)
		{
			iValue += GC.getCivicInfo((CivicTypes)iI).getCommerceModifier(COMMERCE_CULTURE) * 2;
		}
	}

	return iValue;
}

void CvPlayerAI::AI_chooseFreeTech()
{
	TechTypes eBestTech = NO_TECH;

	clearResearchQueue();

	if (GC.getUSE_AI_CHOOSE_TECH_CALLBACK()) // K-Mod. block unused python callbacks
	{
		CyArgsList argsList;
		long lResult;
		argsList.add(getID());
		argsList.add(true);
		lResult = -1;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "AI_chooseTech", argsList.makeFunctionArgs(), &lResult);
		eBestTech = ((TechTypes)lResult);
	}

	if (eBestTech == NO_TECH)
	{
		eBestTech = AI_bestTech(1, true);
	}

	if (eBestTech != NO_TECH)
	{
		GET_TEAM(getTeam()).setHasTech(eBestTech, true, getID(), true, true);
	}
}


void CvPlayerAI::AI_chooseResearch()
{
	//TechTypes eBestTech;
	//int iI;

	clearResearchQueue();

	if (getCurrentResearch() == NO_TECH)
	{
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if ((iI != getID()) && (GET_PLAYER((PlayerTypes)iI).getTeam() == getTeam()))
				{
					if (GET_PLAYER((PlayerTypes)iI).getCurrentResearch() != NO_TECH)
					{
						if (canResearch(GET_PLAYER((PlayerTypes)iI).getCurrentResearch()))
						{
							pushResearch(GET_PLAYER((PlayerTypes)iI).getCurrentResearch());
						}
					}
				}
			}
		}
	}

	if (getCurrentResearch() == NO_TECH)
	{
		TechTypes eBestTech = NO_TECH; // K-Mod
		if (GC.getUSE_AI_CHOOSE_TECH_CALLBACK()) // K-Mod. block unused python callbacks
		{
			CyArgsList argsList;
			long lResult;
			argsList.add(getID());
			argsList.add(false);
			lResult = -1;
			gDLL->getPythonIFace()->callFunction(PYGameModule, "AI_chooseTech", argsList.makeFunctionArgs(), &lResult);
			eBestTech = ((TechTypes)lResult);
		}

		if (eBestTech == NO_TECH)
		{	// <k146>
			int iResearchDepth = (isHuman() || isBarbarian() || AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) || AI_isDoStrategy(AI_STRATEGY_ESPIONAGE_ECONOMY))
				? 1
				: 3;
			eBestTech = AI_bestTech(iResearchDepth); // </k146>
		}

		if (eBestTech != NO_TECH)
		{
			pushResearch(eBestTech);
		}
	}
}

DiploCommentTypes CvPlayerAI::AI_getGreeting(PlayerTypes ePlayer) const
{
	TeamTypes eWorstEnemy;

	if (GET_PLAYER(ePlayer).getTeam() != getTeam())
	{
		eWorstEnemy = GET_TEAM(getTeam()).AI_getWorstEnemy();

		if ((eWorstEnemy != NO_TEAM) && (eWorstEnemy != GET_PLAYER(ePlayer).getTeam()) && GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isHasMet(eWorstEnemy) && (GC.getASyncRand().get(4) == 0))
		{
			if (GET_PLAYER(ePlayer).AI_hasTradedWithTeam(eWorstEnemy) && !atWar(GET_PLAYER(ePlayer).getTeam(), eWorstEnemy))
			{
				return (DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_WORST_ENEMY_TRADING");
			}
			else
			{
				return (DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_WORST_ENEMY");
			}
		}
		else if ((getNumNukeUnits() > 0) && (GC.getASyncRand().get(4) == 0))
		{
			return (DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_NUKES");
		}
		else if ((GET_PLAYER(ePlayer).getPower() < getPower()) && AI_getAttitude(ePlayer) < ATTITUDE_PLEASED && (GC.getASyncRand().get(4) == 0))
		{
			return (DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_UNIT_BRAG");
		}
	}

	return (DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_GREETINGS");
}

// return true if we are willing to talk to ePlayer
bool CvPlayerAI::AI_isWillingToTalk(PlayerTypes ePlayer) const
{
	FAssertMsg(getPersonalityType() != NO_LEADER, "getPersonalityType() is not expected to be equal with NO_LEADER");
	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");

	const CvTeamAI& kOurTeam = GET_TEAM(getTeam()); // K-Mod
	const CvTeam& kTheirTeam = GET_TEAM(GET_PLAYER(ePlayer).getTeam()); // K-Mod

	if (GET_PLAYER(ePlayer).getTeam() == getTeam()
		|| kTheirTeam.isVassal(getTeam())
		|| kOurTeam.isVassal(GET_PLAYER(ePlayer).getTeam()))
	{
		return true;
	}

	/* original bts code
	if (GET_TEAM(getTeam()).isHuman())
	{
		return false;
	} */ // disabled by K-Mod
	// K-Mod
	if (isHuman())
		return true; // humans will speak to anyone, apparently.
	// K-Mod end

	/* advc.134a: Switched arguments to avoid calling isAtWar on the human team
	   when an AI offers peace to a human. */
	// advc.104i: Moved the !atWar branch up
	if (!atWar(GET_PLAYER(ePlayer).getTeam(), getTeam()))
	{
		if (AI_getMemoryCount(ePlayer, MEMORY_STOPPED_TRADING_RECENT) > 0)
		{
			return false;
		}
		return true;
	}
	// advc.104i: Moved up
	if (kOurTeam.isAVassal()
			|| kTheirTeam.isAVassal()) // advc.104i: Get this out of the way
	{
		return false;
	}

	// K-Mod
	if (kOurTeam.isHuman()) // ie. we are an AI player, but our team is human
		return false; // let the human speak for us.
	// K-Mod end

	// <advc.104i>
	int atWarCounter = kOurTeam.AI_getAtWarCounter(TEAMID(ePlayer));
	if(getWPAI.isEnabled() && AI_getMemoryCount(ePlayer,
			MEMORY_STOPPED_TRADING_RECENT) <= 0) { // else RTT as in BtS
		if(GET_TEAM(getTeam()).AI_surrenderTrade(TEAMID(ePlayer)) == NO_DENIAL)
			return true;
		// 1 turn RTT and let the team leader handle peace negotiation
		if(atWarCounter <= 1 || GET_TEAM(getTeam()).getLeaderID() != getID())
			return false;
		if(!GET_PLAYER(ePlayer).isHuman())
			return true;
		/*  The EXE keeps calling this function when the diplo screen is
			already open, and isPeaceDealPossible is an expensive function */
		if(gDLL->isDiplomacy())
			return true;
		return kOurTeam.AI_surrenderTrade(TEAMID(ePlayer)) == NO_DENIAL ||
				warAndPeaceAI().isPeaceDealPossible(ePlayer);
	}
	if(!getWPAI.isEnabled()) // </advc.104i>
		// K-Mod
		if (kOurTeam.AI_refusePeace(GET_PLAYER(ePlayer).getTeam()))
		{
			TradeData item;
			setTradeItem(&item, TRADE_SURRENDER);

			if (!GET_PLAYER(ePlayer).canTradeItem(getID(), item, true))
				return false; // if they can't (or won't) capitulate, don't talk to them.
		}
		// K-Mod end

	int iRefuseDuration = (GC.getLeaderHeadInfo(getPersonalityType()).getRefuseToTalkWarThreshold() * ((kOurTeam.AI_isChosenWar(GET_PLAYER(ePlayer).getTeam())) ? 2 : 1));
		
	int iOurSuccess = 1 + kOurTeam.AI_getWarSuccess(GET_PLAYER(ePlayer).getTeam());
	int iTheirSuccess = 1 + kTheirTeam.AI_getWarSuccess(getTeam());
	if (iTheirSuccess > iOurSuccess * 2
		// <advc.001>
		&& iTheirSuccess >= GC.getWAR_SUCCESS_CITY_CAPTURING()
		/*  Otherwise, killing a single stray unit can be enough to lower
			the refuse duration to three turns (ratio 5:1). </advc.001> */
		)
	{
		iRefuseDuration *= 20 + ((80 * iOurSuccess * 2) / iTheirSuccess);
		iRefuseDuration /= 100;
	}

	if(atWarCounter < iRefuseDuration)
		return false;

	return true;
}


// XXX what if already at war???
// Returns true if the AI wants to sneak attack...
/*  advc.003 (comment) ... which is to say: set WARPLAN_LIMITED w/o prior preparation,
	but don't declare war until units reach the border (perhaps they never do if no
	proper attack stack ready); whereas demandRebukedWar immediately declares war
	(which also sets WARPLAN_LIMITED). */
bool CvPlayerAI::AI_demandRebukedSneak(PlayerTypes ePlayer) const
{
	FAssertMsg(!isHuman(), "isHuman did not return false as expected");
	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");

	FAssert(!(GET_TEAM(getTeam()).isAVassal()));
	FAssert(!(GET_TEAM(getTeam()).isHuman()));

	if (GC.getGameINLINE().getSorenRandNum(100, "AI Demand Rebuked") < GC.getLeaderHeadInfo(getPersonalityType()).getDemandRebukedSneakProb())
	{
		// <advc.104m>
		if(getWPAI.isEnabled()) {
			GET_TEAM(getTeam()).warAndPeaceAI().respondToRebuke(TEAMID(ePlayer), true);
			return false; // respondToRebuke handles any changes to war plans
		} // </advc.104m>
		//if (GET_TEAM(getTeam()).getPower(true) > GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getDefensivePower(getTeam()))
		// K-Mod. Don't start a war if we're already busy; and use AI_startWarVal to evaluate, rather than just power.
		// The 50 value is arbitrary. zero would probably be fine. 50 war rating is also arbitrary, but zero would be too low!
		const CvTeamAI& kTeam = GET_TEAM(getTeam());

		if (kTeam.AI_getWarPlan(GET_PLAYER(ePlayer).getTeam()) == NO_WARPLAN  &&
			(kTeam.getAnyWarPlanCount(true) == 0 || kTeam.AI_getWarSuccessRating() > 50) &&
			kTeam.AI_startWarVal(GET_PLAYER(ePlayer).getTeam(), WARPLAN_LIMITED) > 50)
		// K-Mod end
		{
			return true;
		}
	}

	return false;
}


// XXX what if already at war???
// Returns true if the AI wants to declare war...
bool CvPlayerAI::AI_demandRebukedWar(PlayerTypes ePlayer) const
{
	FAssertMsg(!isHuman(), "isHuman did not return false as expected");
	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");

	FAssert(!(GET_TEAM(getTeam()).isAVassal()));
	FAssert(!(GET_TEAM(getTeam()).isHuman()));

	// needs to be async because it only happens on the computer of the player who is in diplomacy...
	if (GC.getASyncRand().get(100, "AI Demand Rebuked ASYNC") < GC.getLeaderHeadInfo(getPersonalityType()).getDemandRebukedWarProb())
	{
		// <advc.104m>
		if(getWPAI.isEnabled()) {
			GET_TEAM(getTeam()).warAndPeaceAI().respondToRebuke(TEAMID(ePlayer), false);
			return false; // respondToRebuke handles any changes to war plans
		} // </advc.104m>
		if (GET_TEAM(getTeam()).getPower(true) > GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getDefensivePower(getTeam()))
		{
			if (GET_TEAM(getTeam()).AI_isAllyLandTarget(GET_PLAYER(ePlayer).getTeam()))
			{
				return true;
			}
		}
	}

	return false;
}


// XXX maybe make this a little looser (by time...)
bool CvPlayerAI::AI_hasTradedWithTeam(TeamTypes eTeam) const
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == eTeam)
			{
				if ((AI_getPeacetimeGrantValue((PlayerTypes)iI) + AI_getPeacetimeTradeValue((PlayerTypes)iI)) > 0)
				{
					return true;
				}
			}
		}
	}

	return false;
}

// static
AttitudeTypes CvPlayerAI::AI_getAttitudeFromValue(int iAttitudeVal)
{
	if (iAttitudeVal >= 10)
	{
		return ATTITUDE_FRIENDLY;
	}
	else if (iAttitudeVal >= 4) // advc.148: was 3
	{
		return ATTITUDE_PLEASED;
	}
	else if (iAttitudeVal <= -8) // advc.148: was -10
	{
		return ATTITUDE_FURIOUS;
	}
	else if (iAttitudeVal <= -2) // advc.148: was -3
	{
		return ATTITUDE_ANNOYED;
	}
	else
	{
		return ATTITUDE_CAUTIOUS;
	}
}

// K-Mod
void CvPlayerAI::AI_updateAttitudeCache()
{
	for (PlayerTypes i = (PlayerTypes)0; i < MAX_PLAYERS; i=(PlayerTypes)(i+1))
	{
		AI_updateAttitudeCache(i
			, false // advc.130e: Enough to upd. worst enemy once in the end
			);
	}
	GET_TEAM(getTeam()).AI_updateWorstEnemy(); // advc.130e
}

// note: most of this function has been moved from CvPlayerAI::AI_getAttitudeVal
void CvPlayerAI::AI_updateAttitudeCache(PlayerTypes ePlayer
	, bool updateWorstEnemy // advc.130e
	)
{
	PROFILE_FUNC();

	FAssert(ePlayer >= 0 && ePlayer < MAX_PLAYERS);

	const CvPlayerAI& kPlayer = GET_PLAYER(ePlayer);

	if (!GC.getGameINLINE().isFinalInitialized() || ePlayer == getID() ||
		!isAlive() || !kPlayer.isAlive() || !GET_TEAM(getTeam()).isHasMet(kPlayer.getTeam()))
	{
		m_aiAttitudeCache[ePlayer] = 0;
		return;
	}
	// <advc.sha> Now computed in subroutines.
	int iAttitude = AI_getFirstImpressionAttitude(ePlayer);
	iAttitude += AI_getTeamSizeAttitude(ePlayer);
	//iAttitude += AI_getLostWarAttitude(ePlayer);
	iAttitude += AI_getRankDifferenceAttitude(ePlayer);
	// </advc.sha>
	// <advc.130c>
	/* Disabling rank bonus for civs in the bottom half of the leaderboard.
	   Caveat in case this code fragment is re-activated: ranks start at 0. */
    /*if ((GC.getGameINLINE().getPlayerRank(getID()) >= (GC.getGameINLINE().countCivPlayersEverAlive() / 2)) &&
          (GC.getGameINLINE().getPlayerRank(ePlayer) >= (GC.getGameINLINE().countCivPlayersEverAlive() / 2)))
    {
        iAttitude++;
    }*/ // </advc.130c>
	iAttitude += AI_getCloseBordersAttitude(ePlayer);
	iAttitude += AI_getPeaceAttitude(ePlayer);
	iAttitude += AI_getSameReligionAttitude(ePlayer);
	iAttitude += AI_getDifferentReligionAttitude(ePlayer);
	iAttitude += AI_getBonusTradeAttitude(ePlayer);
	iAttitude += AI_getOpenBordersAttitude(ePlayer);
	iAttitude += AI_getDefensivePactAttitude(ePlayer);
	iAttitude += AI_getRivalDefensivePactAttitude(ePlayer);
	iAttitude += AI_getRivalVassalAttitude(ePlayer);
	iAttitude += AI_getExpansionistAttitude(ePlayer); // advc.130w
	iAttitude += AI_getShareWarAttitude(ePlayer);
	iAttitude += AI_getFavoriteCivicAttitude(ePlayer);
	iAttitude += AI_getTradeAttitude(ePlayer);
	iAttitude += AI_getRivalTradeAttitude(ePlayer);

	for (int iI = 0; iI < NUM_MEMORY_TYPES; iI++)
	{
		iAttitude += AI_getMemoryAttitude(ePlayer, ((MemoryTypes)iI));
	}

	//iAttitude += AI_getColonyAttitude(ePlayer); // advc.130r
	iAttitude += AI_getAttitudeExtra(ePlayer);
	/*  advc.sha: Moved to the end and added parameter; the partial result for
		iAttitude now factors in */
	iAttitude += AI_getWarAttitude(ePlayer, iAttitude);

	m_aiAttitudeCache[ePlayer] = range(iAttitude, -100, 100);
	if(updateWorstEnemy) GET_TEAM(getTeam()).AI_updateWorstEnemy(); // advc.130e
}

// for making minor adjustments
void CvPlayerAI::AI_changeCachedAttitude(PlayerTypes ePlayer, int iChange)
{
	FAssert(ePlayer >= 0 && ePlayer < MAX_PLAYERS);
	m_aiAttitudeCache[ePlayer] += iChange;
}
// K-Mod end

AttitudeTypes CvPlayerAI::AI_getAttitude(PlayerTypes ePlayer, bool bForced) const
{
	PROFILE_FUNC();

	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");

	return (AI_getAttitudeFromValue(AI_getAttitudeVal(ePlayer, bForced)));
}


// K-Mod note: the bulk of this function has been moved into CvPlayerAI::AI_updateAttitudeCache.
int CvPlayerAI::AI_getAttitudeVal(PlayerTypes ePlayer, bool bForced) const
{
	// <advc.006> All other functions that access attitude check this too
	if(getID() == ePlayer) {
		FAssert(getID() != ePlayer);
		return 100;
	} // </advc.006>
	if (bForced)
	{
		if (getTeam() == GET_PLAYER(ePlayer).getTeam() || (GET_TEAM(getTeam()).isVassal(GET_PLAYER(ePlayer).getTeam())
			//&& !GET_TEAM(getTeam()).isCapitulated())) // advc.130v: commented out
			))
		{
			return 100;
		}
		// <advc.130v>
		CvTeamAI const& ourTeam = GET_TEAM(getTeam());
		if(ourTeam.isCapitulated()) // Same val as master, but at most Cautious.
			return std::min(0, GET_TEAM(ourTeam.getMasterTeam()).
					AI_getAttitudeVal(TEAMID(ePlayer)));
		if(TEAMREF(ePlayer).isCapitulated()) {
			if(TEAMREF(ePlayer).getMasterTeam() == ourTeam.getMasterTeam())
				return 5; // Pleased
			return ourTeam.AI_getAttitudeVal(GET_PLAYER(ePlayer).getMasterTeam());
		}
		// </advc.130v>
		if (isBarbarian() || GET_PLAYER(ePlayer).isBarbarian())
		{
			return -100;
		}
	}
	if(isHuman()) return 0; // advc.130u
	return m_aiAttitudeCache[ePlayer];
}


int CvPlayerAI::AI_calculateStolenCityRadiusPlots(PlayerTypes ePlayer,
		bool onlyNonWorkable) const // advc.147
{
	PROFILE_FUNC(); // <advc.003>
	FAssert(ePlayer != getID());
	int iCount = 0; // </advc.003>
	// <advc.147> Replacing the code below
	/*  Note that this function is only called once per turn. And I'm not even
		sure that the new code is slower. */
	int dummy=-1;
	for(CvCity* cp = firstCity(&dummy); cp != NULL; cp = nextCity(&dummy)) {
		CvCity const& c = *cp;
		std::vector<CvPlot*> radius;
		::fatCross(*c.plot(), radius);
		int perCityCount = 0;
		int upperBound = std::max(6,
				(c.getPopulation() + c.getHighestPopulation()) / 2);
		for(size_t i = 1; i < radius.size(); i++) {
			CvPlot const* pp = radius[i];
			if(pp == NULL)
				continue;
			CvPlot const& p = *pp;
			if(p.getOwnerINLINE() == ePlayer
					// advc.147:
					&& (!onlyNonWorkable || p.getCityRadiusCount() <= 1)) {
				perCityCount++;
				if(perCityCount >= upperBound)
					break;
			}
		}
		iCount += perCityCount;
	} // </advc.147>
	/*for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (pLoopPlot->getOwnerINLINE() == ePlayer)
		{
			if (pLoopPlot->isPlayerCityRadius(getID()))
			{
				iCount++;
			}
		}
	}*/
	return iCount;
}

// K-Mod
void CvPlayerAI::AI_updateCloseBorderAttitudeCache()
{	// advc.001: was MAX_PLAYERS, but don't need this for barbs
	for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
	{
		AI_updateCloseBorderAttitudeCache(i);
	}
}

// Most of this function has been moved directly from AI_getCloseBorderAttitude
void CvPlayerAI::AI_updateCloseBorderAttitudeCache(PlayerTypes ePlayer)
{
	PROFILE_FUNC();

	const CvTeamAI& kOurTeam = GET_TEAM(getTeam());
	TeamTypes eTheirTeam = GET_PLAYER(ePlayer).getTeam();

	if (!isAlive() || !GET_PLAYER(ePlayer).isAlive() ||
		getTeam() == eTheirTeam || kOurTeam.isVassal(eTheirTeam) || GET_TEAM(eTheirTeam).isVassal(getTeam()))
	{
		m_aiCloseBordersAttitudeCache[ePlayer] = 0;
		return;
	}
	// <advc.130v>
	int limit = GC.getLeaderHeadInfo(getPersonalityType()).
			getCloseBordersAttitudeChange();
	if(limit == 0) {
		m_aiCloseBordersAttitudeCache[ePlayer] = 0;
		return;
	} // </advc.130v>
	// <advc.035>
	int weOwnCulturally = 0;
	bool bWar = kOurTeam.isAtWar(eTheirTeam);
	if(!bWar) {
		std::vector<CvPlot*> contested;
		::contestedPlots(contested, kOurTeam.getID(), eTheirTeam);
		for(size_t i = 0; i < contested.size(); i++)
			if(contested[i]->getSecondOwner() == getID())
				weOwnCulturally++;
	}
	int theySteal_weight = 3;
	int weSteal_weight = 0; // advc.147
	if(GC.getOWN_EXCLUSIVE_RADIUS() > 0) {
		if(!bWar)
			theySteal_weight = 6;
		else theySteal_weight = 4;
	} // </advc.035>
	// <advc.147>
	else weSteal_weight = 1;
	int theySteal = AI_calculateStolenCityRadiusPlots(ePlayer);
	int weSteal = 0;
	if(weSteal_weight > 0) { // A little costly; don't do it if we don't have to.
		weSteal = GET_PLAYER(ePlayer).
				AI_calculateStolenCityRadiusPlots(getID(), true);
	} // </advc.147>
	int iPercent = std::min(60, theySteal *
			theySteal_weight + 3 * weOwnCulturally // advc.035
			+ weSteal_weight * weSteal); // advc.147
	// K-Mod. I've rewritten AI_isLandTarget. The condition I'm using here is equivalent to the original function.
	/*if (kOurTeam.AI_hasCitiesInPrimaryArea(eTheirTeam) && kOurTeam.AI_calculateAdjacentLandPlots(eTheirTeam) >= 8) // K-Mod end
		iPercent += 40;*/
	// <advc.147> Replacing the above
	if (kOurTeam.AI_hasCitiesInPrimaryArea(eTheirTeam)) {
		int adjLand = kOurTeam.AI_calculateAdjacentLandPlots(eTheirTeam);
		if(adjLand > 5) {
			int targetCities = GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).
					getTargetNumCities();
			double adjWeight = ::dRange((5 * targetCities) /
					std::sqrt((double)getTotalLand()), 2.0, 4.0);
			iPercent += std::min(40, ::round(adjLand * adjWeight));
		}
	} // </advc.147>
	// <advc.130v>
	if(TEAMREF(ePlayer).isCapitulated())
		iPercent /= 2; // </advc.130v>
	/*  A bit messy to access other entries of the BordersAttitudeCache while
		updating, but shouldn't have any noticeable side-effect. */
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayerAI const& civ = GET_PLAYER((PlayerTypes)i);
		if(civ.isAlive() && !civ.isMinorCiv() && civ.getTeam() != getTeam() &&
				GET_TEAM(civ.getTeam()).isVassal(TEAMID(ePlayer)) &&
				GET_TEAM(civ.getTeam()).isCapitulated()) {
			int fromVassal = (m_aiCloseBordersAttitudeCache[civ.getID()] * 100) / limit;
			if(fromVassal > 0)
				iPercent += fromVassal;
		}
	}
	iPercent = std::min(iPercent, 100); // </advc.130v>
	// bbai
	if( AI_isDoStrategy(AI_VICTORY_CONQUEST3) )
	{
		iPercent = std::min( 120, (3 * iPercent)/2 );
	}
	// bbai end
	m_aiCloseBordersAttitudeCache[ePlayer] = (limit * iPercent) / 100;
}
// K-Mod end

// K-Mod note: the bulk of this function has been moved to AI_updateCloseBorderAttitudeCache
int CvPlayerAI::AI_getCloseBordersAttitude(PlayerTypes ePlayer) const
{
	return m_aiCloseBordersAttitudeCache[ePlayer];
}

// <advc.sha>
int CvPlayerAI::warSuccessAttitudeDivisor() const {

	return GC.getWAR_SUCCESS_CITY_CAPTURING() + 2 * getCurrentEra();
} // </advc.sha>

int CvPlayerAI::AI_getWarAttitude(PlayerTypes ePlayer,
		int partialSum) const // advc.sha
{
	int iAttitudeChange;
	int iAttitude;

	iAttitude = 0;

	if(!atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
		return 0; // advc.003
	// advc.130g: Was hardcoded as iAttitude-=3; no functional change.
	iAttitude += GC.getDefineINT("AT_WAR_ATTITUDE_CHANGE");

	if (GC.getLeaderHeadInfo(getPersonalityType()).getAtWarAttitudeDivisor() != 0)
	{
		iAttitudeChange = (GET_TEAM(getTeam()).AI_getAtWarCounter(GET_PLAYER(ePlayer).getTeam()) / GC.getLeaderHeadInfo(getPersonalityType()).getAtWarAttitudeDivisor());
		// <advc.sha> Factor war success into WarAttitude
		iAttitudeChange = ((-iAttitudeChange) +
				// Mean of time based penalty and a penalty based on success and era
				TEAMREF(ePlayer).AI_getWarSuccess(getTeam()) /
				warSuccessAttitudeDivisor()) / -2; // </advc.sha>
		int limit = abs(GC.getLeaderHeadInfo(getPersonalityType()).
				getAtWarAttitudeChangeLimit()); // advc.003: refactored
		iAttitude += range(iAttitudeChange, -limit, limit);
	}
	// <advc.sha> Bring it down to 0 at most
	if(partialSum > INT_MIN && partialSum + iAttitude >= 0)
		iAttitude -= partialSum + iAttitude + 1; // </advc.sha>
	return iAttitude;
}


int CvPlayerAI::AI_getPeaceAttitude(PlayerTypes ePlayer) const
{
	int iAttitudeChange;

	if (GC.getLeaderHeadInfo(getPersonalityType()).getAtPeaceAttitudeDivisor() != 0)
	{
		/*  <advc.130a> Use the minimum of peace counter and has-met counter, but
			reduce the dvisisor based on game progress, meaning that civs who meet
			late have to wait only 30 turns instead of 60. */
		// As in BtS:
		int metAndAtPeace = GET_TEAM(getTeam()).AI_getAtPeaceCounter(TEAMID(ePlayer));
		metAndAtPeace = std::min(metAndAtPeace, GET_TEAM(getTeam()).AI_getHasMetCounter(TEAMID(ePlayer)));
		// As in BtS:
		int divisor = GC.getLeaderHeadInfo(getPersonalityType()).getAtPeaceAttitudeDivisor();
		divisor = ::round(divisor * std::max(0.5, 1 -
				GC.getGameINLINE().gameTurnProgress()));
		/*  Rounded down as in BtS; at least 'divisor' turns need to pass before
			diplo improves. */
		iAttitudeChange = metAndAtPeace / divisor;
		// </advc.130a>
		return range(iAttitudeChange, -(abs(GC.getLeaderHeadInfo(getPersonalityType()).getAtPeaceAttitudeChangeLimit())), abs(GC.getLeaderHeadInfo(getPersonalityType()).getAtPeaceAttitudeChangeLimit()));
	}

	return 0;
}


int CvPlayerAI::AI_getSameReligionAttitude(PlayerTypes ePlayer) const
{
	int iAttitudeChange;
	int iAttitude;

	iAttitude = 0;

	if ((getStateReligion() != NO_RELIGION) && (getStateReligion() == GET_PLAYER(ePlayer).getStateReligion()))
	{
		iAttitude += GC.getLeaderHeadInfo(getPersonalityType()).getSameReligionAttitudeChange();

		if (hasHolyCity(getStateReligion()))
		{
			iAttitude++;
		}

		if (GC.getLeaderHeadInfo(getPersonalityType()).getSameReligionAttitudeDivisor() != 0)
		{
			iAttitudeChange = (AI_getSameReligionCounter(ePlayer) / GC.getLeaderHeadInfo(getPersonalityType()).getSameReligionAttitudeDivisor());
			// <advc.130x>
			int limit = ideologyDiploLimit(ePlayer, 0);
			iAttitude += range(iAttitudeChange, -limit, limit); // </advc.130x>
		}
	}

	return iAttitude;
}


int CvPlayerAI::AI_getDifferentReligionAttitude(PlayerTypes ePlayer) const
{
	// <advc.130n> Up to the timeKnown part, only refactoring
	CvPlayer const& p = GET_PLAYER(ePlayer);
	if(getStateReligion() == NO_RELIGION || p.getStateReligion() == NO_RELIGION ||
			getStateReligion() == p.getStateReligion())
		return 0;
	CvLeaderHeadInfo& lh = GC.getLeaderHeadInfo(getPersonalityType());
	int r = lh.getDifferentReligionAttitudeChange();
	int div = lh.getDifferentReligionAttitudeDivisor();
	if(div != 0) {
		int limit = ideologyDiploLimit(ePlayer, 1); // advc.130x
		r += range(AI_getDifferentReligionCounter(ePlayer) / div, -limit, limit);
	}
	int timeKnown = GET_TEAM(getTeam()).getReligionKnownSince(p.getStateReligion());
	if(timeKnown < 0)
		return 0;
	timeKnown = GC.getGameINLINE().getGameTurn() - timeKnown;
	FAssert(timeKnown >= 0);
	r = std::max(r, (2 * timeKnown) / (3 * div)); // max b/c they're negative values
	if(r < 0) {
		// No change to this, but apply it after the timeKnown cap:
		if(hasHolyCity(getStateReligion()))
			r--;
	} // </advc.130n>
	return r;
}


int CvPlayerAI::AI_getBonusTradeAttitude(PlayerTypes ePlayer) const
{
	int iAttitudeChange;

	if (!atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		if (GC.getLeaderHeadInfo(getPersonalityType()).getBonusTradeAttitudeDivisor() != 0)
		{
			iAttitudeChange = (AI_getBonusTradeCounter(ePlayer) / GC.getLeaderHeadInfo(getPersonalityType()).getBonusTradeAttitudeDivisor());
			return range(iAttitudeChange, -(abs(GC.getLeaderHeadInfo(getPersonalityType()).getBonusTradeAttitudeChangeLimit())), abs(GC.getLeaderHeadInfo(getPersonalityType()).getBonusTradeAttitudeChangeLimit()));
		}
	}

	return 0;
}


int CvPlayerAI::AI_getOpenBordersAttitude(PlayerTypes ePlayer) const
{
	int iAttitudeChange;

	if (!atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		if (GC.getLeaderHeadInfo(getPersonalityType()).getOpenBordersAttitudeDivisor() != 0)
		{
			iAttitudeChange = (GET_TEAM(getTeam()).AI_getOpenBordersCounter(GET_PLAYER(ePlayer).getTeam()) / GC.getLeaderHeadInfo(getPersonalityType()).getOpenBordersAttitudeDivisor());
			return range(iAttitudeChange, -(abs(GC.getLeaderHeadInfo(getPersonalityType()).getOpenBordersAttitudeChangeLimit())), abs(GC.getLeaderHeadInfo(getPersonalityType()).getOpenBordersAttitudeChangeLimit()));
		}
	}

	return 0;
}


int CvPlayerAI::AI_getDefensivePactAttitude(PlayerTypes ePlayer) const
{
	int iAttitudeChange;

	if (getTeam() != GET_PLAYER(ePlayer).getTeam() && (GET_TEAM(getTeam()).isVassal(GET_PLAYER(ePlayer).getTeam()) || GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isVassal(getTeam())))
	{
		// advc.130m:
		if(!GET_TEAM(getTeam()).isCapitulated() && !TEAMREF(ePlayer).isCapitulated())
			return GC.getLeaderHeadInfo(getPersonalityType()).getDefensivePactAttitudeChangeLimit();
	}

	if (!atWar(getTeam(), GET_PLAYER(ePlayer).getTeam())
		// advc.130p: Only apply the bonus when currently allied
		&& GET_TEAM(getTeam()).isDefensivePact(TEAMID(ePlayer))
		)
	{
		if (GC.getLeaderHeadInfo(getPersonalityType()).getDefensivePactAttitudeDivisor() != 0)
		{
			iAttitudeChange = (GET_TEAM(getTeam()).AI_getDefensivePactCounter(GET_PLAYER(ePlayer).getTeam()) / GC.getLeaderHeadInfo(getPersonalityType()).getDefensivePactAttitudeDivisor());
			return range(iAttitudeChange, -(abs(GC.getLeaderHeadInfo(getPersonalityType()).getDefensivePactAttitudeChangeLimit())), abs(GC.getLeaderHeadInfo(getPersonalityType()).getDefensivePactAttitudeChangeLimit()));
		}
	}

	return 0;
}


int CvPlayerAI::AI_getRivalDefensivePactAttitude(PlayerTypes ePlayer) const
{
	// advc.130t:
	return defensivePactAttitude(ePlayer, false);
	//BtS formula (refactored):
	/*if(!TEAMREF(ePlayer).isDefensivePact(getTeam()))
		return (-4 * TEAMREF(ePlayer).getDefensivePactCount(TEAMID(ePlayer))) /
				std::max(1, (GC.getGameINLINE().countCivTeamsAlive() - 2)); */
}

// <advc.130w>
int CvPlayerAI::AI_getExpansionistAttitude(PlayerTypes ePlayer) const {

	CvPlayer const& they = GET_PLAYER(ePlayer);
	double foreignCities = 0; int dummy;
	for(CvCity* cp = they.firstCity(&dummy); cp != NULL; cp = they.nextCity(&dummy)) {
		if(cp == NULL) continue; CvCity const& c = *cp;
		if(!c.isRevealed(getTeam(), false))
			continue;
		TeamTypes highestCulture = c.plot()->findHighestCultureTeam();
		if(highestCulture != TEAMID(ePlayer)) {
			double plus = 1;
			/*  Doesn't capture the case where they raze our city to make room
				for one of theirs, but that's covered by raze memory attitude. */
			if(highestCulture == getTeam() && c.isEverOwned(getID()))
				plus += 2;
			// Weighted by 2 times the difference between highest and ePlayer's
			foreignCities += dRange(0.02 *
					(c.plot()->calculateTeamCulturePercent(highestCulture)
					- c.plot()->calculateTeamCulturePercent(TEAMID(ePlayer))),
					0.0, 1.0) * plus;
		}
	}
	/*  1 city could just be one placed a bit too close to a foreign capital
		in the early game */
	if(foreignCities <= 1)
		return 0;
	int everAlive = 0;
	for(int i = 0; i < MAX_CIV_TEAMS; i++) {
		CvTeam const& t = GET_TEAM((TeamTypes)i);
		if(!t.isMinorCiv() && t.isEverAlive())
			everAlive++;
	}
	int civCities = GC.getGameINLINE().getNumCivCities() -
			GET_TEAM(BARBARIAN_TEAM).getNumCities();
	double citiesPerCiv = std::max((double)
			GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities(),
			civCities / (double)everAlive);
	EraTypes era = GC.getGameINLINE().getCurrentEra();
	if(era <= 2)
		citiesPerCiv = std::min(citiesPerCiv, 3.0 * (1 + era));
	double h = expansionistHate(ePlayer);
	return -std::min(4, ::round(h * 2.4 * foreignCities / citiesPerCiv));
}

int CvPlayerAI::AI_getRivalVassalAttitude(PlayerTypes ePlayer) const {

	if(GET_PLAYER(ePlayer).getMasterTeam() == getMasterTeam())
		return 0;
	CvGame const& g = GC.getGameINLINE();
	int everAlive = g.countCivTeamsEverAlive();
	double avgCities = g.getNumCities() / (double)everAlive;
	double capVass = 0;
	for(int i = 0; i < MAX_CIV_TEAMS; i++) {
		CvTeam const& t = GET_TEAM((TeamTypes)i);
		if(!t.isMinorCiv() && t.isAlive() && t.isVassal(TEAMID(ePlayer)) &&
				t.isCapitulated())
			capVass += ::dRange(2.5 * t.getNumCities() / avgCities, 0.5, 1.5);
	}
	return defensivePactAttitude(ePlayer, true) // advc.130t
			- ::round(expansionistHate(ePlayer) * 5 * capVass /
			std::sqrt((double)everAlive));
	//BtS formula (refactored):
	/*if(TEAMREF(ePlayer).getVassalCount(getTeam()) > 0)
		return (-6 * TEAMREF(ePlayer).getPower(true)) /
				std::max(1, GC.getGameINLINE().countTotalCivPower());*/
	// </advc.130w>
}

// <advc.130t>
int CvPlayerAI::defensivePactAttitude(PlayerTypes ePlayer, bool vassalPacts) const {

	if(GET_PLAYER(ePlayer).getMasterTeam() == getMasterTeam())
		return 0;
	CvTeamAI const& ourTeam = GET_TEAM(getTeam());
	double score = 0;
	int total = 0;
	for(int i = 0; i < MAX_CIV_TEAMS; i++) {
		CvTeam const& t = GET_TEAM((TeamTypes)i);
		if(!t.isAlive() || t.isMinorCiv() || !ourTeam.isHasMet(t.getID()) ||
				// If we're at war, then apparently the DP isn't deterring us
				ourTeam.isAtWar(t.getID()) ||
				(vassalPacts ? t.isCapitulated() : t.isAVassal()) ||
				t.getID() == getTeam() || t.getID() == TEAMID(ePlayer))
			continue;
		total++;
		if((vassalPacts && t.isVassal(TEAMID(ePlayer))) ||
				(!vassalPacts && TEAMREF(ePlayer).isDefensivePact(t.getID()) &&
				!ourTeam.isDefensivePact(t.getID()) &&
				ourTeam.getPower(true) / (double)t.getPower(true) > 0.7)) {
			// Score up to -4 per pact - too much? Cap at ATTITUDE_FRIENDLY - 2?
			score += std::max(0, std::min((int)ATTITUDE_FRIENDLY - 1,
					(GC.getLeaderHeadInfo(getPersonalityType()).
					getDeclareWarThemRefuseAttitudeThreshold() + 1) -
					/*  Calling AI_getAttitude doesn't lead to infinite recursion
						because the result is read from cache. */
					ourTeam.AI_getAttitude(t.getID())));
			if(ourTeam.AI_getWorstEnemy() == t.getID())
				score += 1;
		}
	}
	if(total == 0)
		return 0;
	// Moderate impact of the number of civs: only one over sqrt(total)
	return -std::min(5, ::round((2 * score) / std::sqrt((double)total)));
} // </advc.130t>

// <advc.130w>
double CvPlayerAI::expansionistHate(PlayerTypes civId) const {

	CvLeaderHeadInfo& lh = GC.getLeaderHeadInfo(getPersonalityType());
	double r = (5 + AI_getPeaceWeight() - lh.getWarmongerRespect()) / 12.0;
	double powRatio = GET_TEAM(getTeam()).getPower(true) / (double)
			TEAMREF(civId).getPower(true);
	r = std::min(r, powRatio);
	return dRange(r, 0.0, 1.0);
} // </advc.130w>

int CvPlayerAI::AI_getShareWarAttitude(PlayerTypes ePlayer) const
{
	// <advc.130m> Ended up rewriting this function entirely
	CvTeamAI const& ourTeam = GET_TEAM(getTeam());
	if(ourTeam.isCapitulated() || TEAMREF(ePlayer).isCapitulated() ||
			atWar(getTeam(), TEAMID(ePlayer)))
		return 0;
	CvLeaderHeadInfo& lh = GC.getLeaderHeadInfo(getPersonalityType());
	int div = lh.getShareWarAttitudeDivisor();
	if(div == 0)
		return 0;
	bool isShare = ourTeam.AI_shareWar(GET_PLAYER(ePlayer).getTeam());
	if(!isShare && ourTeam.getAtWarCount() > 0) {
		/*  Only suspend the relations bonus from past shared war if the current war
			is at least 5 turns old, and only if we could use help. */
		int wsThresh = ::round((3.0 * warSuccessAttitudeDivisor()) / 4);
		for(int i = 0; i < MAX_CIV_TEAMS; i++) {
			CvTeamAI const& t = GET_TEAM((TeamTypes)i);
			/*  AtWarCount is the number of teams we're at war with, whereas
				AtWarCounter is the number of turns we've been at war. Oh my! */
			if(t.isAlive() && !t.isMinorCiv() &&
					ourTeam.AI_getAtWarCounter(t.getID()) >= 5 &&
					ourTeam.AI_getWarSuccessRating() < 80 &&
					(ourTeam.AI_getWarPlan(t.getID()) != WARPLAN_DOGPILE ||
					ourTeam.AI_getAtWarCounter(t.getID()) >= 15))
				return 0;
		}
	}
	int r = 0;
	if(isShare) // No functional change here
		r += lh.getShareWarAttitudeChange();
	int turnsShared = ourTeam.AI_getShareWarCounter(TEAMID(ePlayer));
	int theirContribution = ourTeam.getSharedWarSuccess(TEAMID(ePlayer));
	int limit = abs(lh.getShareWarAttitudeChangeLimit());
	limit = std::min(limit, 1 + turnsShared / div);
	r += range((int)(theirContribution /
			// This divisor seems to produce roughly the result I have in mind
			(3.5 * GC.getWAR_SUCCESS_CITY_CAPTURING() * div)), 0, limit);
	return r;
	// </advc.130m>
}


int CvPlayerAI::AI_getFavoriteCivicAttitude(PlayerTypes ePlayer) const
{
	int iAttitudeChange;

	int iAttitude;

	iAttitude = 0;

	if (GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic() != NO_CIVIC)
	{
		if (isCivic((CivicTypes)(GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic())) && GET_PLAYER(ePlayer).isCivic((CivicTypes)(GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic())))
		{
			iAttitude += GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivicAttitudeChange();

			if (GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivicAttitudeDivisor() != 0)
			{
				iAttitudeChange = (AI_getFavoriteCivicCounter(ePlayer) / GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivicAttitudeDivisor());
				// <advc.130x>
				int limit = ideologyDiploLimit(ePlayer, 2);
				iAttitude += range(iAttitudeChange, -limit, limit); // </advc.130x>
			}
		}
	}

	return iAttitude;
}

// <advc.130p> Not much BtS code left here
int CvPlayerAI::AI_getTradeAttitude(PlayerTypes ePlayer) const {

	int r = AI_getPeacetimeGrantValue(ePlayer)
		+ std::max(0, (AI_getPeacetimeTradeValue(ePlayer)
		- GET_PLAYER(ePlayer).AI_getPeacetimeTradeValue(getID())));
	/*  Adjustment to game progress now happens when recording grant and trade values;
		decay happens explicitly in AI_doCounter. */
	return range(::round(r * TEAMREF(ePlayer).recentlyMetMultiplier(getTeam()))
			/ peacetimeTradeValDivisor, 0, peacetimeTradeRelationsLimit);
}


int CvPlayerAI::AI_getRivalTradeAttitude(PlayerTypes ePlayer) const {

	CvTeamAI const& ourTeam = GET_TEAM(getTeam());
	TeamTypes theirId = TEAMID(ePlayer);
	int r = ourTeam.AI_getEnemyPeacetimeGrantValue(theirId) +
			(ourTeam.AI_getEnemyPeacetimeTradeValue(theirId) / 2);
	// OB hurt our feelings also
	TeamTypes enemyId = ourTeam.AI_getWorstEnemy();
	/*  Checking enemyValue here should - hopefully - fix an oscillation problem,
		but I don't quite know what I'm doing ...
		(Checking this in CvTeamAI::AI_getWorstEnemy would be bad for performance.) */
	if(ourTeam.enemyValue(enemyId) <= 0)
		enemyId = NO_TEAM;
	double dualDealCounter = 0; // cf. CvDeal::isDual
	if(enemyId != NO_TEAM) {
		if(GET_TEAM(theirId).isOpenBorders(enemyId)) {
			dualDealCounter += std::min(20,
					std::min(ourTeam.AI_getHasMetCounter(theirId),
					GET_TEAM(theirId).AI_getOpenBordersCounter(enemyId)));
			if(ourTeam.isAtWar(enemyId))
				/*  Don't like that enemy units can move through the borders of
					ePlayer and heal there, but only moderate increase b/c we
					can't well afford to make further enemies when already in a war. */
				dualDealCounter *= 1.5;
		}
		/*  Resource trades are handled by CvDeal::doTurn (I wrote the code below
			before realizing that) */
		/*dualDealCounter += std::min(20.0,
				std::min((double)ourTeam.AI_getHasMetCounter(theirId),
				GET_PLAYER(ePlayer).AI_getBonusTradeCounter(enemyId) / 2.0));*/
		// <advc.130v> Blame it on the master instead
		if(TEAMREF(ePlayer).isCapitulated())
			dualDealCounter /= 2; // </advc.130v>
		/*  Our enemy could be liked by everyone else, but we can't afford to hate
			everyone. */
		int const counterBound = 15;
		if(dualDealCounter > counterBound) {
			int nOB = 0;
			int total = 0;
			/*  Vassals have their own OB agreements and can agree to embargos;
				count them fully here. */
			for(int i = 0; i < MAX_CIV_TEAMS; i++) {
				CvTeam const& t = GET_TEAM((TeamTypes)i);
				if(t.isAlive() && t.getID() != enemyId && t.isHasMet(getTeam())) {
					total++;
					if(t.isOpenBorders(enemyId))
						nOB++;
				}
			}
			double nonOBRatio = (total - nOB) / ((double)total);
			dualDealCounter = std::max((double)counterBound, nonOBRatio);
		}
		// <advc.130v>
		int vassalDealCounter = 0;
		for(int i = 0; i < MAX_CIV_TEAMS; i++) {
			CvTeamAI const& t = GET_TEAM((TeamTypes)i);
			if(t.isCapitulated() && t.getID() != getTeam() &&
					enemyId != TEAMID(ePlayer) && // Master mustn't be the enemy
					t.isVassal(TEAMID(ePlayer)) && t.isAlive() && !t.isMinorCiv() &&
					t.isOpenBorders(enemyId)) {
				int fromVassal = t.AI_getOpenBordersCounter(enemyId) / 2;
				if(fromVassal > 0)
					vassalDealCounter += fromVassal;
			}
		}
		dualDealCounter += std::min(vassalDealCounter, 10);
		// </advc.130v>
	}
	/*  Adjustment to game progress now happens when recording grant and trade values;
		decay happens explicitly in AI_doCounter. */
	r = (::round(r * TEAMREF(ePlayer).recentlyMetMultiplier(getTeam())) +
			75 * ::round(dualDealCounter)) / peacetimeTradeValDivisor;
	return -range(r, 0, peacetimeTradeRelationsLimit);
}

int CvPlayerAI::AI_getBonusTradeCounter(TeamTypes tId) const {

	int r = 0;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayerAI const& civ = GET_PLAYER((PlayerTypes)i);
		if(civ.isAlive() && civ.getTeam() == tId)
			r += AI_getBonusTradeCounter(civ.getID());
	}
	return r;
} // </advc.130p>


int CvPlayerAI::AI_getMemoryAttitude(PlayerTypes ePlayer, MemoryTypes eMemory) const
{
	// <advc.130s>
	if(eMemory == MEMORY_ACCEPTED_JOIN_WAR) {
		CvTeam const& ourTeam = GET_TEAM(getTeam());
		if(!GC.isJOIN_WAR_DIPLO_BONUS() || (ourTeam.getAtWarCount() > 0 &&
				!ourTeam.anyWarShared(TEAMID(ePlayer))) ||
				ourTeam.isAtWar(TEAMID(ePlayer)))
			return 0;
	} // </advc.130s>
	// <advc.145>
	// Fav. civic and religion are based on LeaderType, not PersonalityType.
	CvLeaderHeadInfo const& lh = GC.getLeaderHeadInfo(getLeaderType());
	if(eMemory == MEMORY_ACCEPTED_CIVIC) {
		CivicTypes fav = (CivicTypes)lh.getFavoriteCivic();
		if(fav != NO_CIVIC && (!GET_PLAYER(ePlayer).isCivic(fav) ||
				!isCivic(fav)))
			return 0;
	}
	if(eMemory == MEMORY_ACCEPTED_RELIGION) {
		ReligionTypes fav = (ReligionTypes)lh.getFavoriteReligion();
		if(fav != NO_RELIGION && (GET_PLAYER(ePlayer).getStateReligion() != fav ||
				getStateReligion() != fav))
			return 0;
	}
	if(eMemory == MEMORY_DENIED_CIVIC) {
		CivicTypes fav = (CivicTypes)lh.getFavoriteCivic();
		if(fav == NO_CIVIC || GET_PLAYER(ePlayer).isCivic(fav) || !isCivic(fav))
			return 0;
	}
	if(eMemory == MEMORY_DENIED_RELIGION) {
		ReligionTypes fav = (ReligionTypes)lh.getFavoriteReligion();
		if(fav == NO_RELIGION || GET_PLAYER(ePlayer).getStateReligion() == fav ||
				getStateReligion() != fav)
			return 0;
	} // </advc.145>
	/* <advc.130j> Was 100. Effect halved b/c diplo actions now counted twice.
		195 (instead of 200) to make sure that 0.5 gets rounded up. BtS rounded down,
		but this rarely mattered. Now rounding down would often result in no effect
		on relations. */
	double div = 195; // Finer granularity for DoW:
	if(eMemory == MEMORY_DECLARED_WAR)
		div = 295; // </advc.130j>
	return ::round((AI_getMemoryCount(ePlayer, eMemory) *
			GC.getLeaderHeadInfo(getPersonalityType()).
			getMemoryAttitudePercent(eMemory)) / div); 
}


int CvPlayerAI::AI_getColonyAttitude(PlayerTypes ePlayer) const
{
	int iAttitude = 0;

	if (getParent() == ePlayer)
	{
		iAttitude += GC.getLeaderHeadInfo(getPersonalityType()).getFreedomAppreciation();
	}

	return iAttitude;
}

// <advc.sha> Some changes to DaveMcW's code.
// BEGIN: Show Hidden Attitude Mod 01/22/2009
int CvPlayerAI::AI_getFirstImpressionAttitude(PlayerTypes ePlayer) const {

	CvPlayerAI& kPlayer = GET_PLAYER(ePlayer);
	CvHandicapInfo& h = GC.getHandicapInfo(kPlayer.getHandicapType());
	CvLeaderHeadInfo& lh = GC.getLeaderHeadInfo(getPersonalityType());
	int iAttitude = lh.getBaseAttitude();
	iAttitude += h.getAttitudeChange();
	if(!kPlayer.isHuman()) {
		// <advc.130b>
		// Original peace weight modifier
		int peaceWeightModifier = 4 - abs(AI_getPeaceWeight() -
				kPlayer.AI_getPeaceWeight());
		double personalityModifier = peaceWeightModifier * 0.45;
		// Original warmonger respect modifier
		int respectModifier = std::min(lh.getWarmongerRespect(),
				GC.getLeaderHeadInfo(kPlayer.getPersonalityType()).
				getWarmongerRespect());
		personalityModifier += respectModifier * 0.75;
		iAttitude += ::round(personalityModifier);
		iAttitude = ::range(iAttitude, -3, 3); // </advc.130b>
		iAttitude += h.getAIAttitudeChange(); // advc.148
	}
    return iAttitude;
}

int CvPlayerAI::AI_getTeamSizeAttitude(PlayerTypes ePlayer) const {

	return -std::max(0, (TEAMREF(ePlayer).getNumMembers() -
			GET_TEAM(getTeam()).getNumMembers()));
}

// <advc.130c>
int CvPlayerAI::AI_getRankDifferenceAttitude(PlayerTypes ePlayer) const {

	CvGame& g = GC.getGameINLINE();
	// No hate if they're 150% ahead
	if(g.getPlayerScore(ePlayer) * 10 > g.getPlayerScore(getID()) * 15)
		return 0;
	// Don't count ranks of unknown civs
	int iRankDifference = knownRankDifference(ePlayer);
	CvLeaderHeadInfo& lh = GC.getLeaderHeadInfo(getPersonalityType());
	/*  This was "+ 1" in BtS, which is arguably a bug.
		Continue using CivPlayersEverAlive although iRankDifference is now based
		only on known civs. */
	double maxRankDifference = g.countCivPlayersEverAlive() - 1;
	int base = 0;
	double multiplier = 0;
	// If we're ranked worse than they are:
	if(iRankDifference > 0) {
		base = lh.getWorseRankDifferenceAttitudeChange();
		/* Want multiplier to be 1 when the rank difference is 35% of nCivs, and
		   near 0 when greater than 50% of nCivs. */
		multiplier = 1 - (2.0 * std::abs(
				iRankDifference - 0.35 * maxRankDifference) / maxRankDifference);
		if(multiplier < 0)
			multiplier = 0;
	}
	/* If we're ranked better, as in BtS, the modifier is proportional
	   to the relative rank difference. */
	else {
		multiplier = -iRankDifference / maxRankDifference;
		base = lh.getBetterRankDifferenceAttitudeChange();
	}
	int r = ::round(base * multiplier);
	// Don't hate them if they're still in the first era
	if(r < 0 && (GET_PLAYER(ePlayer).getCurrentEra() <= g.getStartEra() ||
			/*  nor if the score difference is small (otherwise attitude can
				change too frequently) */
			g.getPlayerScore(ePlayer) - g.getPlayerScore(getID()) < 25))
		return 0;
	// Don't like them if we're still in the first era
	if(r > 0 && (getCurrentEra() <= g.getStartEra() ||
			GET_PLAYER(ePlayer).AI_isDoVictoryStrategyLevel3()))
		return 0;
	if(r < 0 && AI_isDoVictoryStrategyLevel3() && !GET_PLAYER(ePlayer).AI_isDoVictoryStrategyLevel4())
		return 0;
	return r;
} // </advc.130c>

int CvPlayerAI::AI_getLostWarAttitude(PlayerTypes ePlayer) const {
	// (not called anymore)
	if(TEAMREF(ePlayer).AI_getWarSuccess(getTeam()) >
			GET_TEAM(getTeam()).AI_getWarSuccess(TEAMID(ePlayer)))
		return GC.getLeaderHeadInfo(getPersonalityType()).getLostWarAttitudeChange();
    return 0;
}
// END: Show Hidden Attitude Mod </advc.sha>

// <advc.130c>
int CvPlayerAI::knownRankDifference(PlayerTypes otherId) const {

	CvGame& g = GC.getGameINLINE();
	int ourRank = g.getPlayerRank(getID());
	int theirRank = g.getPlayerRank(otherId);
	int delta = ourRank - theirRank;
	// We know who's ranked better, but (perhaps) not the exact rank difference
	if(delta < -1) delta = -1;
	if(delta > 1) delta = 1;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		PlayerTypes thirdId = (PlayerTypes)i;
		CvPlayer const& thirdCiv = GET_PLAYER(thirdId);
		if(!thirdCiv.isAlive() || thirdCiv.isBarbarian() || thirdCiv.isMinorCiv() ||
				!GET_TEAM(getTeam()).isHasMet(TEAMID(thirdId)))
			continue;
		int thirdRank = g.getPlayerRank(thirdId);
		if(delta < 0 && thirdRank > ourRank && thirdRank < theirRank)
			delta--;
		else if(delta > 0 && thirdRank < ourRank && thirdRank > theirRank)
			delta++;
	}
	return delta;
} // </advc.130c>

PlayerVoteTypes CvPlayerAI::AI_diploVote(const VoteSelectionSubData& kVoteData, VoteSourceTypes eVoteSource, bool bPropose)
{
	PROFILE_FUNC();
	// <advc.003> Moved the other declarations
	int iValue=0;
	int iI=0;
	// Replacing getGame calls throughout this function
	CvGame& g = GC.getGame(); // </advc.003>
	VoteTypes eVote = kVoteData.eVote;

	CvTeamAI& kOurTeam = GET_TEAM(getTeam()); // K-Mod

	if (g.isTeamVote(eVote))
	{
		if (g.isTeamVoteEligible(getTeam(), eVoteSource))
		{
			return (PlayerVoteTypes)getTeam();
		} // <advc.148>
		int iBestValue = 0;
		if(GC.getVoteInfo(eVote).isVictory())
			iBestValue = 9; // was 7 </advc.148>
		PlayerVoteTypes eBestTeam = PLAYER_VOTE_ABSTAIN;
		int secondBestVal = iBestValue; // advc.115b
		for (iI = 0; iI < MAX_TEAMS; iI++)
		{	// <advc.003>
			if(!GET_TEAM((TeamTypes)iI).isAlive() ||
					!g.isTeamVoteEligible((TeamTypes)iI, eVoteSource))
				continue; // </advc.003>
			if (kOurTeam.isVassal((TeamTypes)iI))
			{
				return (PlayerVoteTypes)iI;
			}

			iValue = kOurTeam.AI_getAttitudeVal((TeamTypes)iI);
			/*  <advc.130v> Capitulated vassals don't just vote for
				their master, but also for civs that the master likes. */
			if(kOurTeam.isCapitulated()) {
				iValue = GET_TEAM(kOurTeam.getMasterTeam()).
						AI_getAttitudeVal((TeamTypes)iI);
			} // </advc.130v>
			if(iValue > secondBestVal) { // advc.115b
				if (iValue > iBestValue)
				{
					secondBestVal = iBestValue; // advc.115b
					iBestValue = iValue;
					eBestTeam = (PlayerVoteTypes)iI;
				}
				else secondBestVal = iValue; // advc.115b
			}
		} // <advc.115b>
		if(iBestValue == secondBestVal)
			return PLAYER_VOTE_ABSTAIN; // </advc.115b>
		return eBestTeam;
	}
	else
	{
		TeamTypes eSecretaryGeneral = g.getSecretaryGeneral(eVoteSource);

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
/* original BTS code
		if (!bPropose)
		{
			if (eSecretaryGeneral != NO_TEAM)
			{
				if (eSecretaryGeneral == getTeam() ||(GET_TEAM(getTeam()).AI_getAttitude(eSecretaryGeneral) == ATTITUDE_FRIENDLY))
				{
					return PLAYER_VOTE_YES;
				}
			}
		}
*/
		// Remove blanket auto approval for friendly secretary
		bool bFriendlyToSecretary = false;
		if (!bPropose)
		{
			if (eSecretaryGeneral != NO_TEAM)
			{
				if (eSecretaryGeneral == getTeam())
				{
					return PLAYER_VOTE_YES;
				}
				else
				{
					bFriendlyToSecretary = (kOurTeam.AI_getAttitude(eSecretaryGeneral) == ATTITUDE_FRIENDLY);
				}
			}
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		
		bool bDefy = false;

		bool bValid = true;

		if (bValid)
		{
			for (iI = 0; iI < GC.getNumCivicInfos(); iI++)
			{
				if (GC.getVoteInfo(eVote).isForceCivic(iI))
				{
					if (!isCivic((CivicTypes)iI))
					{
						CivicTypes eBestCivic = AI_bestCivic((CivicOptionTypes)(GC.getCivicInfo((CivicTypes)iI).getCivicOptionType()));

						if (eBestCivic != NO_CIVIC)
						{
							if (eBestCivic != ((CivicTypes)iI))
							{
								int iBestCivicValue = AI_civicValue(eBestCivic);
								int iNewCivicValue = AI_civicValue((CivicTypes)iI);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
/* original BTS code
								if (iBestCivicValue > ((iNewCivicValue * 120) / 100))
								{
									bValid = false;
									if (iBestCivicValue > ((iNewCivicValue * (140 + (GC.getGame().getSorenRandNum(120, "AI Erratic Defiance (Force Civic)"))) / 100)))
*/
								// Increase threshold of voting for friend's proposal
								if( bFriendlyToSecretary )
								{
									iNewCivicValue *= 6;
									iNewCivicValue /= 5;
								}

								if (iBestCivicValue > ((iNewCivicValue * 120) / 100))
								{
									bValid = false;

									// Increase odds of defiance, particularly on AggressiveAI
									if (iBestCivicValue > ((iNewCivicValue * (140 + (g.getSorenRandNum((g.isOption(GAMEOPTION_AGGRESSIVE_AI) ? 60 : 80), "AI Erratic Defiance (Force Civic)"))) / 100)))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
									{
										bDefy = true;										
									}
									break;
								}
							}
						}
					}
				}
			}
		}

		if (bValid)
		{
			if (GC.getVoteInfo(eVote).getTradeRoutes() > 0)
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
				if( bFriendlyToSecretary )
				{
					return PLAYER_VOTE_YES;
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				if (getNumCities() > ((g.getNumCities() * 2) / (g.countCivPlayersAlive() + 1)))
				{
					bValid = false;
				}
			}
		}

		if (bValid)
		{
			if (GC.getVoteInfo(eVote).isNoNukes())
			{
				int iVoteBanThreshold = 0;
				iVoteBanThreshold += kOurTeam.getNukeInterception() / 3;
				iVoteBanThreshold += GC.getLeaderHeadInfo(getPersonalityType()).getBuildUnitProb();
				iVoteBanThreshold *= std::max(1, GC.getLeaderHeadInfo(getPersonalityType()).getWarmongerRespect());
				if (g.isOption(GAMEOPTION_AGGRESSIVE_AI))
				{
					iVoteBanThreshold *= 2;
				}

				bool bAnyHasSdi = false;
				for (iI = 0; iI < MAX_TEAMS; iI++)
				{
					if (GET_TEAM((TeamTypes)iI).isAlive() && iI != getTeam())
					{
						if (GET_TEAM((TeamTypes)iI).getNukeInterception() > 0)
						{
							bAnyHasSdi = true;
							break;
						}
					}
				}

				if (!bAnyHasSdi && kOurTeam.getNukeInterception() > 0 && kOurTeam.getNumNukeUnits() > 0)
				{
					iVoteBanThreshold *= 2;
				}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
				if( bFriendlyToSecretary )
				{
					iVoteBanThreshold *= 2;
					iVoteBanThreshold /= 3;
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

				bValid = (g.getSorenRandNum(100, "AI nuke ban vote") > iVoteBanThreshold);
				
				if (AI_isDoStrategy(AI_STRATEGY_OWABWNW))
				{
					bValid = false;
				}
				else if ((kOurTeam.getNumNukeUnits() / std::max(1, kOurTeam.getNumMembers())) < (g.countTotalNukeUnits() / std::max(1, g.countCivPlayersAlive())))
				{
					bValid = false;
				}
				if (!bValid && AI_getNumTrainAIUnits(UNITAI_ICBM) > 0)
				{
					if (g.getSorenRandNum(AI_isDoStrategy(AI_STRATEGY_OWABWNW) ? 2 : 3, "AI Erratic Defiance (No Nukes)") == 0)
					{
						bDefy = true;
					}
				}
			}
		}

		if (bValid)
		{
			if (GC.getVoteInfo(eVote).isFreeTrade())
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
				if( bFriendlyToSecretary )
				{
					return PLAYER_VOTE_YES;
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

				int iOpenCount = 0;
				int iClosedCount = 0;

				for (iI = 0; iI < MAX_TEAMS; iI++)
				{
					if (GET_TEAM((TeamTypes)iI).isAlive())
					{
						if (iI != getTeam())
						{
							if (kOurTeam.isOpenBorders((TeamTypes)iI))
							{
								iOpenCount += GET_TEAM((TeamTypes)iI).getNumCities();
							}
							else
							{
								iClosedCount += GET_TEAM((TeamTypes)iI).getNumCities();
							}
						}
					}
				}

				if (iOpenCount >= (getNumCities() * getTradeRoutes()))
				{
					bValid = false;
				}

				if (iClosedCount == 0)
				{
					bValid = false;
				}
			}
		}

		if (bValid)
		{
			if (GC.getVoteInfo(eVote).isOpenBorders())
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
				if( bFriendlyToSecretary )
				{
					return PLAYER_VOTE_YES;
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

				bValid = true;

				for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
				{
					if (iI != getTeam())
					{
						if (GET_TEAM((TeamTypes)iI).isVotingMember(eVoteSource))
						{
							if (NO_DENIAL != kOurTeam.AI_openBordersTrade((TeamTypes)iI))
							{
								bValid = false;
								break;
							}
						}
					}
				}
			}
			else if (GC.getVoteInfo(eVote).isDefensivePact())
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
				if( bFriendlyToSecretary )
				{
					return PLAYER_VOTE_YES;
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

				bValid = true;

				for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
				{
					if (iI != getTeam())
					{
						if (GET_TEAM((TeamTypes)iI).isVotingMember(eVoteSource))
						{
							if (NO_DENIAL != kOurTeam.AI_defensivePactTrade((TeamTypes)iI))
							{
								bValid = false;
								break;
							}
						}
					}
				}
			}
			else if (GC.getVoteInfo(eVote).isForcePeace())
			{
				FAssert(kVoteData.ePlayer != NO_PLAYER);
				TeamTypes ePeaceTeam = GET_PLAYER(kVoteData.ePlayer).getTeam();
				
				int iWarsWinning = 0;
				int iWarsLosing = 0;
				int iChosenWar = 0;
// K-Mod version of end-war vote decision.
// Note: this code is based on BBAI code. I'm not sure which bits are from BBAI and which are from BtS.
// So I haven't marked individual changes or preserved the original code. The way "winning" and "losing" are calculated are mine.
				bool bLosingBig = false;
				bool bWinningBig = false;
				bool bThisPlayerWinning = false;

				int iSuccessScale = GET_PLAYER(kVoteData.ePlayer).getNumMilitaryUnits() * GC.getDefineINT("WAR_SUCCESS_ATTACKING") / 5;

				bool bAggressiveAI = g.isOption(GAMEOPTION_AGGRESSIVE_AI);
				if (bAggressiveAI)
				{
					iSuccessScale *= 3;
					iSuccessScale /= 2;
				}

				// Is ePeaceTeam winning wars?
				int iPeaceTeamPower = GET_TEAM(ePeaceTeam).getPower(true);

				for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
				{
					if (GET_TEAM((TeamTypes)iI).isAlive())
					{
						if (iI != ePeaceTeam)
						{
							if (!GET_TEAM((TeamTypes)iI).isAVassal() && GET_TEAM((TeamTypes)iI).isAtWar(ePeaceTeam))
							{
								int iPeaceTeamSuccess = GET_TEAM(ePeaceTeam).AI_getWarSuccess((TeamTypes)iI);
								int iOtherTeamSuccess = GET_TEAM((TeamTypes)iI).AI_getWarSuccess(ePeaceTeam);

								int iOtherTeamPower = GET_TEAM((TeamTypes)iI).getPower(true);

								if (iPeaceTeamSuccess * iPeaceTeamPower > (iOtherTeamSuccess + iSuccessScale) * iOtherTeamPower)
								{
									// Have to be ahead by at least a few victories to count as win
									++iWarsWinning;

									if (iPeaceTeamSuccess * iPeaceTeamPower / std::max(1, (iOtherTeamSuccess + 2*iSuccessScale) * iOtherTeamPower) >= 2)
									{
										bWinningBig = true;
									}
								}
								else if (iOtherTeamSuccess * iOtherTeamPower > (iPeaceTeamSuccess + iSuccessScale) * iPeaceTeamPower)
								{
									// Have to have non-trivial loses
									++iWarsLosing;

									if (iOtherTeamSuccess * iOtherTeamPower / std::max(1, (iPeaceTeamSuccess + 2*iSuccessScale) * iPeaceTeamPower) >= 2)
									{
										bLosingBig = true;
									}

									if (iI == getTeam())
									{
										bThisPlayerWinning = true;
									}
								}
								else if (GET_TEAM(ePeaceTeam).AI_getAtWarCounter((TeamTypes)iI) < 10)
								{
									// Not winning, just recently attacked, and in multiple wars, be pessimistic
									// Counts ties from no actual battles
									if ((GET_TEAM(ePeaceTeam).getAtWarCount(true) > 1) && !(GET_TEAM(ePeaceTeam).AI_isChosenWar((TeamTypes)iI)))
									{
										++iWarsLosing;
									}
								}

								if (GET_TEAM(ePeaceTeam).AI_isChosenWar((TeamTypes)iI))
								{
									++iChosenWar;									
								}
							}
						}
					}
				}
				// <advc.104n> Overwrite the counts and flags set by BBAI/K-Mod
				if(getWPAI.isEnabled()) {
					bLosingBig = bWinningBig = false;
					// Shouldn't matter if chosen; trust the war utility computation
					iChosenWar = 0;
					/*  ... except when we're considering to propose peace with ourselves.
						If we've just started a war, and want to continue that war,
						the proposal can still be advantageous when it allows us
						to end some other war. In this case, we should not propose
						peace with ourselves, but peace with that other team that we
						don't want to be at war with.
						The proper solution would be to pick the best proposal
						instead of a random "valid" one. This is just a temporary fix: */
					if(getTeam() == ePeaceTeam && bPropose) {
						for(int i = 0; i < MAX_CIV_TEAMS; i++) {
							CvTeamAI const& enemy = GET_TEAM((TeamTypes)i);
							if(enemy.isAlive() && enemy.isAtWar(ePeaceTeam) &&
									GET_TEAM(ePeaceTeam).
									AI_getAtWarCounter(enemy.getID()) < 5 &&
									!enemy.AI_isChosenWar(ePeaceTeam))
								return PLAYER_VOTE_NO;
						}
					}
					int u = -GET_TEAM(ePeaceTeam).warAndPeaceAI().uEndAllWars(eVoteSource);
					if(u < -10) { iWarsWinning = 0; iWarsLosing = 1; }
					else if(u > 10) { iWarsWinning = 1; iWarsLosing = 0; }
					else iWarsWinning = iWarsLosing = 0;
					if(u > 75) bWinningBig = true;
					else if(u < -75) bLosingBig = true;
				} // </advc.104n>
				
				if (ePeaceTeam == getTeam())
				{
					int iPeaceRand = GC.getLeaderHeadInfo(getPersonalityType()).getBasePeaceWeight();
					// Note, base peace weight ranges between 0 and 10.
					iPeaceRand /= (bAggressiveAI ? 2 : 1);
					iPeaceRand /= (AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST2) ? 2 : 1);
					// <advc.104n>
					double prPeace = 0;
					if(iPeaceRand > 0)
						prPeace = 1 - (1.0 / iPeaceRand); // </advc.104n>
					if(bLosingBig &&
						/*  advc.104n: The BtS code can have us randomly vote
							against our own proposal. */
						::hash(g.getGameTurn(), getID()) < prPeace)
						//(GC.getGame().getSorenRandNum(iPeaceRand, "AI Force Peace to avoid loss") || bPropose) )
					{
						// Non-warmongers want peace to escape loss
						bValid = true;
					}
					//else if ( !bLosingBig && (iChosenWar > iWarsLosing) )
					else if ( !bLosingBig && (iChosenWar > iWarsLosing ||
							(AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3) // K-Mod
							// advc.104n: Military victory is covered by war utility
							&& !getWPAI.isEnabled())))
					{
						// If chosen to be in most wars, keep it going
						bValid = false;
					}
					else
					{
						// If losing most wars, vote for peace
						bValid = (iWarsLosing > iWarsWinning);
					}

					if (!bValid && !bLosingBig && bWinningBig)
					{
						// Can we continue this war with defiance penalties?
						if( !AI_isFinancialTrouble() )
						{
							if (!g.getSorenRandNum(iPeaceRand, "AI defy Force Peace!"))
							{
								bDefy = true;
							}
						}
					}
				}
				else if (eSecretaryGeneral == getTeam() && !bPropose)
				{
					bValid = true;
				}
				else if (GET_TEAM(ePeaceTeam).isAtWar(getTeam()))
				{
					bool bWantsToEndWar = ((kOurTeam.AI_endWarVal(ePeaceTeam) > (3*GET_TEAM(ePeaceTeam).AI_endWarVal(getTeam()))/2));
					/*  <advc.118> If we don't want to negotiate, then apparently
						we don't want to end the war */
					if(!AI_isWillingToTalk(GET_TEAM(ePeaceTeam).getLeaderID()))
						bWantsToEndWar = false; // </advc.118>

					bValid = bWantsToEndWar;

					if( bValid )
					{
						bValid = bWinningBig || (iWarsWinning > iWarsLosing) || (kOurTeam.getAtWarCount(true, true) > 1);
					}

					if (!bValid && bThisPlayerWinning && (iWarsLosing >= iWarsWinning) && !bPropose )
					{
						if( !kOurTeam.isAVassal() )
						{
							if( (kOurTeam.getAtWarCount(true) == 1) || bLosingBig )
							{
								// Can we continue this war with defiance penalties?
								if( !AI_isFinancialTrouble() )
								{
									int iDefyRand = GC.getLeaderHeadInfo(getPersonalityType()).getBasePeaceWeight();
									iDefyRand /= (bAggressiveAI ? 2 : 1);

									if (g.getSorenRandNum(iDefyRand, "AI Erratic Defiance (Force Peace)") == 0)
									{
										bDefy = true;
									}
								}
							}
						}
					}

					// <advc.104n> Can do better than the above
					if(getWPAI.isEnabled()) {
						int u = -GET_TEAM(getTeam()).warAndPeaceAI().uEndWar(ePeaceTeam);
						bValid = (u < 0);
						bDefy = false;
						if(!AI_isFinancialTrouble()) {
							double pr = (u - 50 - 5 * GC.getLeaderHeadInfo(getPersonalityType()).
									getBasePeaceWeight()) / 100.0;
							if(::bernoulliSuccess(pr, "advc.104n"))
								bDefy = true;
						}
					} // </advc.104n>

					if( !bValid && !bDefy && !bPropose )
					{
						if((kOurTeam.AI_getAttitude(eSecretaryGeneral) > GC.getLeaderHeadInfo(getPersonalityType()).getVassalRefuseAttitudeThreshold()) )
						{
							// Influence by secretary
							if( NO_DENIAL == kOurTeam.AI_makePeaceTrade(ePeaceTeam, eSecretaryGeneral) )
							{
								bValid = true;
							}
							else if( eSecretaryGeneral != NO_TEAM && kOurTeam.isVassal(eSecretaryGeneral) )
							{
								bValid = true;
							}
						}
					}
				}
				else
				{
					if( kOurTeam.AI_getWarPlan(ePeaceTeam) != NO_WARPLAN )
					{
						// Keep planned enemy occupied
						bValid = false;
					}
					else if( kOurTeam.AI_shareWar(ePeaceTeam)  && !(kOurTeam.isVassal(ePeaceTeam)) )
					{
						// Keep ePeaceTeam at war with our common enemies
						bValid = false;
					}
					else if(iWarsLosing > iWarsWinning)
					{
						// Feel pity for team that is losing (if like them enough to not declare war on them)
						bValid = (kOurTeam.AI_getAttitude(ePeaceTeam)
								> /* advc.118, advc.001: Was ">=", but that's not how
								     DeclareWarThemRefuseAttitudeThreshold works */
								GC.getLeaderHeadInfo(getPersonalityType()).getDeclareWarThemRefuseAttitudeThreshold());
					}
					else
					  if(iWarsLosing < iWarsWinning) // advc.118
					{
						// Stop a team that is winning (if don't like them enough to join them in war)
						bValid = (kOurTeam.AI_getAttitude(ePeaceTeam) < GC.getLeaderHeadInfo(getPersonalityType()).getDeclareWarRefuseAttitudeThreshold());
					}

					/* <advc.118> Also vote for peace if neither side is winning,
				       and we like the peace team and all its adversaries.
					   Need to like them so much that we couldn't be brought
					   to declare war on them. */
				    else {
					  AttitudeTypes dwtr = (AttitudeTypes)GC.getLeaderHeadInfo(
				          getPersonalityType()).
					      getDeclareWarThemRefuseAttitudeThreshold();
					  if(!bValid && kOurTeam.AI_getAttitude(ePeaceTeam) > dwtr) {
					    bValid = true;
					    for(int i = 0; i < MAX_CIV_TEAMS; i++) {
						  TeamTypes tt = (TeamTypes)i; CvTeam& t = GET_TEAM(tt);
					      if(t.isAlive() && tt != ePeaceTeam &&
						      !t.isAVassal() && t.isAtWar(ePeaceTeam) &&
							  kOurTeam.AI_getAttitude(tt) <= dwtr) {
						    bValid = false;
						    break;
						  }
					    }
					  }
					}
				    // </advc.118>

					if( !bValid )
					{
						if( bFriendlyToSecretary && !kOurTeam.isVassal(ePeaceTeam) )
						{
							// Influence by secretary
							bValid = true;
						}
					}
				}
// K-Mod end
			}
			else if (GC.getVoteInfo(eVote).isForceNoTrade())
			{
				FAssert(kVoteData.ePlayer != NO_PLAYER);
				TeamTypes eEmbargoTeam = GET_PLAYER(kVoteData.ePlayer).getTeam();

				if (eSecretaryGeneral == getTeam() && !bPropose)
				{
					bValid = true;
				}
				else if (eEmbargoTeam == getTeam())
				{
					bValid = false;
					if (!isNoForeignTrade())
					{
						bDefy = true;
					}
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
				else
				{
					if( bFriendlyToSecretary )
					{
						return PLAYER_VOTE_YES;
					}
					else if( canStopTradingWithTeam(eEmbargoTeam) )
					{
						bValid = (NO_DENIAL == AI_stopTradingTrade(eEmbargoTeam, kVoteData.ePlayer));
						if (bValid)
						{
							bValid = (kOurTeam.AI_getAttitude(eEmbargoTeam) <= ATTITUDE_CAUTIOUS);
						}
					}
					else
					{
						bValid = (kOurTeam.AI_getAttitude(eEmbargoTeam) < ATTITUDE_CAUTIOUS);
					}
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			}
			else if (GC.getVoteInfo(eVote).isForceWar())
			{
				FAssert(kVoteData.ePlayer != NO_PLAYER);
				TeamTypes eWarTeam = GET_PLAYER(kVoteData.ePlayer).getTeam();

				if (eSecretaryGeneral == getTeam() && !bPropose)
				{
					bValid = true;
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
/* original BTS code
				else if (eWarTeam == getTeam())
				{
					bValid = false;
				}
				else if (GET_TEAM(eWarTeam).isAtWar(getTeam()))
*/
				else if (eWarTeam == getTeam() || kOurTeam.isVassal(eWarTeam))
				{
					// Explicit rejection by all who will definitely be attacked
					bValid = false;
				}
				else if ( kOurTeam.AI_getWarPlan(eWarTeam) != NO_WARPLAN )
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				{
					bValid = true;
				}
				else
				{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      07/20/09                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
/* original BTS code
					bValid = (bPropose || NO_DENIAL == GET_TEAM(getTeam()).AI_declareWarTrade(eWarTeam, eSecretaryGeneral));
					if (bValid)
					{
						bValid = (GET_TEAM(getTeam()).AI_getAttitude(eWarTeam) < ATTITUDE_CAUTIOUS);
					}
*/
					if( !bPropose && kOurTeam.isAVassal() )
					{
						// Vassals always deny war trade requests and thus previously always voted no
						bValid = false;

						if( kOurTeam.getAnyWarPlanCount(true) == 0 )
						{
							if( eSecretaryGeneral == NO_TEAM || (kOurTeam.AI_getAttitude(eSecretaryGeneral) > GC.getLeaderHeadInfo(getPersonalityType()).getDeclareWarRefuseAttitudeThreshold()) )
							{
								if( eSecretaryGeneral != NO_TEAM && kOurTeam.isVassal(eSecretaryGeneral) )
								{
									bValid = true;
								}
								//else if( (GET_TEAM(getTeam()).isAVassal() ? GET_TEAM(getTeam()).getCurrentMasterPower(true) : GET_TEAM(getTeam()).getPower(true)) > GET_TEAM(eWarTeam).getDefensivePower() )
								else if (GET_TEAM(kOurTeam.getMasterTeam()).getPower(true) > GET_TEAM(eWarTeam).getDefensivePower())
								{
									bValid = true;
								}
							}
						}
					}
					else
					{
						bValid = (bPropose || NO_DENIAL == kOurTeam.AI_declareWarTrade(eWarTeam, eSecretaryGeneral));
					}

					if (bValid ||
							getWPAI.isEnabled()) // advc.104n
					{
						if(!getWPAI.isEnabled()) { // advc.104n
							int iNoWarOdds = GC.getLeaderHeadInfo(getPersonalityType()).getNoWarAttitudeProb((kOurTeam.AI_getAttitude(eWarTeam)));
							bValid = ((iNoWarOdds < 30) || (g.getSorenRandNum(100, "AI War Vote Attitude Check (Force War)") > iNoWarOdds));						
						// <advc.104n>
						} 
						if(getWPAI.isEnabled()) {
							int u = GET_TEAM(getTeam()).warAndPeaceAI().
										uJointWar(eWarTeam, eVoteSource);
							bValid = (bValid && u > 0);
							bDefy = false;
							/*  The war will be trouble too, but checking fin.
								trouble is a safeguard against defying repeatedly. */
							if(!AI_isFinancialTrouble()) {
								double pr = (-u - 125 + 5 * GC.getLeaderHeadInfo(getPersonalityType()).
										getBasePeaceWeight()) / 100.0;
								if(::bernoulliSuccess(pr, "advc.104n"))
									bDefy = true;
							}
						} // </advc.104n>
					}
					/*
					else
					{
						// Consider defying resolution
						if( !GET_TEAM(getTeam()).isAVassal() )
						{
							if( eSecretaryGeneral == NO_TEAM || GET_TEAM(getTeam()).AI_getAttitude(eWarTeam) > GET_TEAM(getTeam()).AI_getAttitude(eSecretaryGeneral) )
							{
								if( GET_TEAM(getTeam()).AI_getAttitude(eWarTeam) > GC.getLeaderHeadInfo(getPersonalityType()).getDefensivePactRefuseAttitudeThreshold() )
								{
									int iDefyRand = GC.getLeaderHeadInfo(getPersonalityType()).getBasePeaceWeight();
									iDefyRand /= (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 2 : 1);

									if (GC.getGame().getSorenRandNum(iDefyRand, "AI Erratic Defiance (Force War)") > 0)
									{
										bDefy = true;
									}
								}
							}
						}
					}
					*/
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				}
			}
			else if (GC.getVoteInfo(eVote).isAssignCity())
			{
				bValid = false;

				FAssert(kVoteData.ePlayer != NO_PLAYER);
				CvPlayer& kPlayer = GET_PLAYER(kVoteData.ePlayer);
				CvCity* pCity = kPlayer.getCity(kVoteData.iCityId);
				if (NULL != pCity)
				{
					if (NO_PLAYER != kVoteData.eOtherPlayer && kVoteData.eOtherPlayer != pCity->getOwnerINLINE())
					{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/03/09                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
						if ((!bPropose && eSecretaryGeneral == getTeam()) || GET_PLAYER(kVoteData.eOtherPlayer).getTeam() == getTeam())
						{
							bValid = true;
						}
						else if (kPlayer.getTeam() == getTeam())
						{
							bValid = false;
							// BBAI TODO: Wonders, holy city, aggressive AI?
							if (g.getSorenRandNum(3, "AI Erratic Defiance (Assign City)") == 0)
							{
								bDefy = true;
							}
						}
						else
						{
							bValid = (AI_getAttitude(kVoteData.ePlayer) < AI_getAttitude(kVoteData.eOtherPlayer));
						}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
					}
				}
			}
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
/* original BTS code
		if (bDefy && canDefyResolution(eVoteSource, kVoteData))
*/
		// Don't defy resolutions from friends
		if( bDefy && !bFriendlyToSecretary && canDefyResolution(eVoteSource, kVoteData))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		{
			return PLAYER_VOTE_NEVER;
		}

		return (bValid ? PLAYER_VOTE_YES : PLAYER_VOTE_NO);
	}

}

int CvPlayerAI::AI_dealVal(PlayerTypes ePlayer, const CLinkList<TradeData>* pList, bool bIgnoreAnnual, int iChange,
		bool ignoreDiscount) const // advc.550a
{
	CLLNode<TradeData>* pNode;
	CvCity* pCity;
	int iValue;

	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");

	iValue = 0;

	if (atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{	// <advc.104i>
		if(getWPAI.isEnabled())
			iValue += GET_TEAM(getTeam()).warAndPeaceAI().endWarVal(TEAMID(ePlayer));
		else // </advc.104>
		iValue += GET_TEAM(getTeam()).AI_endWarVal(GET_PLAYER(ePlayer).getTeam());
	}

	// advc.104o: Can't be hired for more than one war at a time
	int wars = 0;
	for (pNode = pList->head(); pNode; pNode = pList->next(pNode))
	{
		FAssertMsg(!(pNode->m_data.m_bHidden), "(pNode->m_data.m_bHidden) did not return false as expected");
		/*  <advc.130p> Replacing the checks in the individual cases. Previously,
			OB and DP were not counted as annual, but GPT and resources were;
			now it's the other way around. */
		TradeableItems ti = pNode->m_data.m_eItemType;
		if(bIgnoreAnnual && CvDeal::isAnnual(ti) && ti != TRADE_GOLD_PER_TURN &&
				ti != TRADE_RESOURCES)
			continue;
		switch(ti) // </advc.130p>
		{
		case TRADE_TECHNOLOGIES:
			iValue += GET_TEAM(getTeam()).AI_techTradeVal((TechTypes)(pNode->m_data.m_iData), GET_PLAYER(ePlayer).getTeam()
					, ignoreDiscount); // advc.550a
			break;
		case TRADE_RESOURCES:
			iValue += AI_bonusTradeVal(((BonusTypes)(pNode->m_data.m_iData)), ePlayer, iChange);
			break;
		case TRADE_CITIES:
			pCity = GET_PLAYER(ePlayer).getCity(pNode->m_data.m_iData);
			if (pCity != NULL)
			{
				iValue += AI_cityTradeVal(pCity);
			}
			break;
		case TRADE_GOLD:
			iValue += (pNode->m_data.m_iData * AI_goldTradeValuePercent()) / 100;
			break;
		case TRADE_GOLD_PER_TURN:
			iValue += AI_goldPerTurnTradeVal(pNode->m_data.m_iData);
			break;
		case TRADE_MAPS:
			iValue += GET_TEAM(getTeam()).AI_mapTradeVal(GET_PLAYER(ePlayer).getTeam());
			break;
		case TRADE_SURRENDER:
			iValue += GET_TEAM(getTeam()).AI_surrenderTradeVal(GET_PLAYER(ePlayer).getTeam());
			break;
		case TRADE_VASSAL:
			iValue += GET_TEAM(getTeam()).AI_vassalTradeVal(GET_PLAYER(ePlayer).getTeam());
			break;
		case TRADE_OPEN_BORDERS:
			iValue += GET_TEAM(getTeam()).AI_openBordersTradeVal(GET_PLAYER(ePlayer).getTeam());
			break;
		case TRADE_DEFENSIVE_PACT:
			iValue += GET_TEAM(getTeam()).AI_defensivePactTradeVal(GET_PLAYER(ePlayer).getTeam());
			break;
		case TRADE_PEACE:
			iValue += GET_TEAM(getTeam()).AI_makePeaceTradeVal(((TeamTypes)(pNode->m_data.m_iData)), GET_PLAYER(ePlayer).getTeam());
			break;
		case TRADE_WAR:
			// <advc.104o>
			if(getWPAI.isEnabled()) {
				wars++;
				if(wars > 1)
					iValue += 100000; // More than they can pay
			} // </advc.104o>
			iValue += GET_TEAM(getTeam()).AI_declareWarTradeVal(((TeamTypes)(pNode->m_data.m_iData)), GET_PLAYER(ePlayer).getTeam());
			break;
		case TRADE_EMBARGO:
			iValue += AI_stopTradingTradeVal(((TeamTypes)(pNode->m_data.m_iData)), ePlayer);
			break;
		case TRADE_CIVIC:
			iValue += AI_civicTradeVal(((CivicTypes)(pNode->m_data.m_iData)), ePlayer);
			break;
		case TRADE_RELIGION:
			iValue += AI_religionTradeVal(((ReligionTypes)(pNode->m_data.m_iData)), ePlayer);
			break;
		}
	}

	return iValue;
}


bool CvPlayerAI::AI_goldDeal(const CLinkList<TradeData>* pList) const
{
	CLLNode<TradeData>* pNode;

	for (pNode = pList->head(); pNode; pNode = pList->next(pNode))
	{
		FAssert(!(pNode->m_data.m_bHidden));

		switch (pNode->m_data.m_eItemType)
		{
		case TRADE_GOLD:
		case TRADE_GOLD_PER_TURN:
			return true;
			break;
		}
	}

	return false;
}

// <advc.705>
bool CvPlayerAI::isAnnualDeal(CLinkList<TradeData> const& itemList) const {

	for(CLLNode<TradeData>* node = itemList.head(); node != NULL;
			node = itemList.next(node)) {
		if(!CvDeal::isAnnual(node->m_data.m_eItemType))
			return false;
	}
	return true;
} // </advc.705>

/// \brief AI decision making on a proposal it is given
///
/// In this function the AI considers whether or not to accept another player's proposal.  This is used when
/// considering proposals from the human player made in the diplomacy window as well as a couple other places.
// advc.130o: Removed const qualifier
bool CvPlayerAI::AI_considerOffer(PlayerTypes ePlayer, const CLinkList<TradeData>* pTheirList, const CLinkList<TradeData>* pOurList, int iChange)
{
	const CvTeamAI& kOurTeam = GET_TEAM(getTeam()); // K-Mod
	// <advc.003> Plugged in throughout this function (or sometimes TEAMREF instead)
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	bool bHuman = kPlayer.isHuman(); // </advc.003>
	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");

	if (AI_goldDeal(pTheirList) && AI_goldDeal(pOurList))
	{
		return false;
	}

	if (iChange > -1)
	{
		for (CLLNode<TradeData>* pNode = pOurList->head(); pNode; pNode = pOurList->next(pNode))
		{
			TradeData tdata = pNode->m_data; // advc.003
			if (getTradeDenial(ePlayer, tdata) != NO_DENIAL)
			{
				return false;
			} /* <advc.036> Denial from the human pov should also prevent
				 resource trades. Matters for "Care to negotiate". */
			if(tdata.m_eItemType == TRADE_RESOURCES && bHuman &&
					kPlayer.getTradeDenial(getID(), tdata) != NO_DENIAL)
				return false; // </advc.036>
		}
	}
	/*if (kPlayer.getTeam() == getTeam())
	{
		return true;
	}*/
	// advc.155: Replacing the above
	bool bSameTeam = (getTeam() == kPlayer.getTeam());
	// K-Mod. Refuse all war-time offers unless it's part of a peace deal
	if (atWar(getTeam(), kPlayer.getTeam()))
	{
		/*  <advc.003> Could treat Cease Fire here. The K-Mod code below always
			refuses. */
		/*if(pTheirList->getLength() + pOurList->getLength() <= 0)
			;*/ // </advc.003>
		bool bEndWar = false;
		CLLNode<TradeData>* pNode;
		for (pNode = pTheirList->head(); !bEndWar && pNode; pNode = pTheirList->next(pNode))
		{
			if (CvDeal::isEndWar(pNode->m_data.m_eItemType))
				bEndWar = true;
		}
		for (pNode = pOurList->head(); !bEndWar && pNode; pNode = pOurList->next(pNode))
		{
			if (CvDeal::isEndWar(pNode->m_data.m_eItemType))
				bEndWar = true;
		}

		if (!bEndWar)
			return false;
	}
	// K-Mod end

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/23/09                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
	// Don't always accept giving deals, TRADE_VASSAL and TRADE_SURRENDER come with strings attached
	bool bVassalTrade = false;
	if (iChange > -1) // K-Mod
	{
		for (CLLNode<TradeData>* pNode = pTheirList->head(); pNode; pNode = pTheirList->next(pNode))
		{
			if( pNode->m_data.m_eItemType == TRADE_VASSAL )
			{
				bVassalTrade = true;

				for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
				{
					if (GET_TEAM((TeamTypes)iTeam).isAlive())
					{
						if (iTeam != getTeam() && iTeam != kPlayer.getTeam() && atWar(kPlayer.getTeam(), (TeamTypes)iTeam) && !atWar(getTeam(), (TeamTypes)iTeam))
						{
							if (kOurTeam.AI_declareWarTrade((TeamTypes)iTeam, kPlayer.getTeam(), false) != NO_DENIAL)
							{
								return false;
							}
						}
					}
				}
			}
			else if( pNode->m_data.m_eItemType == TRADE_SURRENDER )
			{
				bVassalTrade = true;

				if( !(kOurTeam.AI_acceptSurrender(kPlayer.getTeam())) )
				{
					return false;
				}
			}
		}
	}
	// K-Mod. This is our opportunity for canceling a vassal deal.
	else
	{
		for (CLLNode<TradeData>* pNode = pOurList->head(); pNode; pNode = pOurList->next(pNode))
		{
			if (pNode->m_data.m_eItemType == TRADE_VASSAL || pNode->m_data.m_eItemType == TRADE_SURRENDER)
			{
				bVassalTrade = true;

				// The trade denial calculation for vassal trades is actually a bit nuanced.
				// Rather than trying to restructure it, or write new code and risk inconsistencies, I'm just going use it.
				if (kOurTeam.AI_surrenderTrade(kPlayer.getTeam(), pNode->m_data.m_eItemType == TRADE_SURRENDER ? 125
						/*  advc.112, advc.143: Changed iPowerMultiplier from 75
							to 100, which is the default. Multiplier for cancelation
							now set inside AI_surrenderTrade. */
						: 100) != NO_DENIAL)
					return false;
				// note: AI_vassalTrade calls AI_surrenderTrade after doing a bunch of war checks and so on. So we don't need that.
				// CvPlayer::getTradeDenial, unfortunately, will reject any vassal deal by an AI player on a human team - we don't want that here.
				// (regarding the power multiplier, cf. values used in getTradeDenial)
			}
		}
	}
	// K-Mod end
	// <advc.705>
	CvGame& g = GC.getGameINLINE();
	bool possibleCollusion = g.isOption(GAMEOPTION_RISE_FALL) &&
			bHuman && g.getRiseFall().
			isCooperationRestricted(getID()); // </advc.705>
	if( !bVassalTrade )
	{
		if ((pOurList->getLength() == 0) && (pTheirList->getLength() > 0))
		{	// <advc.705>
			if(possibleCollusion) {
				g.getRiseFall().substituteDiploText(true);
				return false;
			}
			else // </advc.705>
				return true;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	// K-Mod
	if (iChange < 0)
	{
		// There are only a few cases where the AI will allow ongoing trade with its worst enemy...
		if (kOurTeam.AI_getWorstEnemy() == kPlayer.getTeam() && !kOurTeam.isVassal(kPlayer.getTeam()))
		{
			if (pOurList->getLength() > 1 || pOurList->head()->m_data.m_eItemType != TRADE_GOLD_PER_TURN)
			{
				// this trade isn't one of the special cases.
				return false;
			}
		}
	}
	// K-Mod end

	/*  <advc.132> Don't agree to accept two civics of the same column.
		Would be better to rule this out through denial (e.g. "must be joking"),
		but the trade screen isn't mod-able. */
	if(!checkCivicReligionConsistency(pOurList) ||
			!checkCivicReligionConsistency(pTheirList)) 
		return false; // </advc.132>
	/*  advc.003: iOurValue is the value of what we give; how much they value what
		we give, more specifically.
		(Elsewhere in this class, it's the other way around.) */
	int iOurValue = kPlayer.AI_dealVal(getID(), pOurList, false, iChange);
	// <advc.705>
	int pessimisticVal = (possibleCollusion ? g.getRiseFall().pessimisticDealVal(
			getID(), iOurValue, *pOurList) : -1); // </advc.705>
	int iTheirValue = AI_dealVal(ePlayer, pTheirList, false, iChange);
	int iThreshold = -1; // advc.155: Declaration moved up
	if (iOurValue > 0 && 0 == pTheirList->getLength() && 0 == iTheirValue)
	{	// <advc.130v> Vassal mustn't force a peace treaty on its master
		if(kOurTeam.isAVassal() && !kOurTeam.isVassal(TEAMID(ePlayer)))
			return false; // </advc.130v>
		// K-Mod
		// Don't give any gifts to civs that you are about to go to war with.
		if (kOurTeam.AI_getWarPlan(kPlayer.getTeam()) != NO_WARPLAN)
			return false;
		//Don't cancel gift deals to vassals that you like, unless you need the gift back.
		if (iChange < 0 && TEAMREF(ePlayer).isVassal(getTeam()))
		{
			/*if (iOurValue > AI_dealVal(ePlayer, pOurList, false, 1))
				return true;
			else
				return false; */
			// Simply comparing deal values doesn't work because the value from voluntary vassals gets halved.
			// Will have to use a kludge:
			if (pOurList->getLength() == 1 && pOurList->head()->m_data.m_eItemType == TRADE_RESOURCES)
			{
				BonusTypes eBonus = (BonusTypes)pOurList->head()->m_data.m_iData;
				if (kPlayer.AI_bonusVal(eBonus, -1) > AI_bonusVal(eBonus, 1))
					return true;
				else
					return false;
			}
		}
		// K-Mod end

		/*  <advc.133> isVassalTributeDeal can't distinguish between freebies from
			a granted help request and those from vassal tribute. That's fine,
			cancel gifts too. */
		if(kOurTeam.getMasterTeam() != TEAMID(ePlayer) &&
				CvDeal::isVassalTributeDeal(pOurList) && iChange <= 0)
			return false; // </advc.133>
		// <advc.130o>
		bool demand = false;
		bool accept = true; // </advc.130o>
		if (kOurTeam.isVassal(kPlayer.getTeam()) && CvDeal::isVassalTributeDeal(pOurList))
		{
			if (AI_getAttitude(ePlayer, false) <= GC.getLeaderHeadInfo(getPersonalityType()).getVassalRefuseAttitudeThreshold()
				//&& kOurTeam.getAtWarCount(true) == 0
				&& !isFocusWar() // advc.105: Replacing the above
				&& TEAMREF(ePlayer).getDefensivePactCount() == 0)
			{
				iOurValue *= (kOurTeam.getPower(false) + 10);
				iOurValue /= (TEAMREF(ePlayer).getPower(false) + 10);
			}
			else
			{
				return true;
			}
		}
		else
		{
			// <advc.130o> Moved up
			int const mc = AI_getMemoryCount(ePlayer, MEMORY_MADE_DEMAND_RECENT);
			if(mc > 0)
				return false; // </advc.130o>
			/*  <advc.130o> Was hard-coded, but the XML value is CAUTIOUS for all
				leaders, so, no functional change. */
			AttitudeTypes const noGiveHelpThresh = (AttitudeTypes)GC.getLeaderHeadInfo(
					getPersonalityType()).getNoGiveHelpAttitudeThreshold();
			demand = AI_getAttitude(ePlayer) <= noGiveHelpThresh;
			if (demand) // </advc.130o>
			{
				FAssert(bHuman);
				if (kOurTeam.getPower(false) > ((TEAMREF(ePlayer).getPower(false) * 4) / 3))
				{	// advc.130o: Don't return just yet
					accept = false;
				}
			} // <advc.130o>
			// </advc.130o> <advc.144>
			else if(!bSameTeam && // advc.155
					::hash(GC.getGameINLINE().getGameTurn(), getID()) < prDenyHelp())
				return false; // </advc.144>
			// <advc.104m>
			if(accept && demand && getWPAI.isEnabled() &&
					!bSameTeam) // advc.155
				accept = warAndPeaceAI().considerDemand(ePlayer, iOurValue);
			// </advc.104m>
		}
		// advc.130o: Do this only if UWAI hasn't already handled the offer
		if(!demand || (!getWPAI.isEnabled() && accept)) {
			iThreshold = (kOurTeam.AI_getHasMetCounter(kPlayer.getTeam()) + 50);
			iThreshold *= 2;
			// <advc.155>
			if(bSameTeam)
				iThreshold *= 5;
			else { // </advc.155>
				if (TEAMREF(ePlayer).AI_isLandTarget(getTeam()))
				{
					iThreshold *= 3;
				} 
				iThreshold *= (TEAMREF(ePlayer).getPower(false) + 100);
				iThreshold /= (kOurTeam.getPower(false) + 100);
			} // advc.155
			iThreshold -= kPlayer.AI_getPeacetimeGrantValue(getID());

			accept = (iOurValue < iThreshold); // advc.130o: Don't return yet
			// <advc.144>
			if(accept && !demand && getWPAI.isEnabled())
				accept = warAndPeaceAI().considerGiftRequest(ePlayer, iOurValue);
			// </advc.144>
		}
		// <advc.130o>
		if(demand) {
			/*  Moved here from CvPlayer::handleDiploEvent b/c that handler
				can't distinguish whether the AI accepts a human demand */
			if(accept) {
				// ePlayer is human, but still let the proxy AI remember.
				GET_PLAYER(ePlayer).AI_rememberEvent(getID(), MEMORY_ACCEPT_DEMAND);
				AI_rememberEvent(ePlayer, MEMORY_MADE_DEMAND);
			}
			else {
				// Again, not important.
				GET_PLAYER(ePlayer).AI_rememberEvent(getID(), MEMORY_REJECTED_DEMAND);
				/*  Here's where we don't increase MEMORY_MADE_DEMAND b/c the
					demand isn't accepted. CvPlayer::handleDiploEvent deals
					with the MADE_DEMAND_RECENT memory. */
			}
		} // <advc.155> Previously handled by CvPlayer::handleDiploEvent
		else if(accept && bSameTeam)
			AI_changeMemoryCount(ePlayer, MEMORY_MADE_DEMAND_RECENT, 1);
		// </advc.155>
		return accept; // </advc.130o>
	} // <advc.036>
	if(!checkResourceLimits(pOurList, pTheirList, ePlayer, iChange))
		return false; // </advc.036>
	// <advc.705>
	if(g.isOption(GAMEOPTION_RISE_FALL) && bHuman) {
		if(possibleCollusion) {
			double thresh = g.getRiseFall().dealThresh(isAnnualDeal(*pOurList));
			if(iChange < 0)
				thresh = std::min(thresh, std::max(0.4, thresh - 0.15));
			if(pessimisticVal / (iTheirValue + 0.01) < thresh)
				return g.getRiseFall().isSquareDeal(*pOurList, *pTheirList, getID());
		}
		if(!kOurTeam.isGoldTrading() && !TEAMREF(ePlayer).isGoldTrading())
			iOurValue = ::round(0.9 * iOurValue);
	} // </advc.705>
	if (iChange < 0)
	{
		//return (iTheirValue * 110 >= iOurValue * 100);
		// <advc.133> Always 125 in K-Mod
		int tolerance = (bHuman ? 130 : 140);
		// <advc.155>
		if(bSameTeam)
			tolerance = 150; // </advc.155>
		return (tolerance * // </advc.133>
				iTheirValue >= iOurValue * 100 // K-Mod
				// advc.155:
				|| (bSameTeam && iOurValue < iThreshold));
	}
	// <advc.136b>
	else {
		if(iTheirValue < 2 * GC.getDIPLOMACY_VALUE_REMAINDER() &&
				!AI_goldDeal(pTheirList) && (pTheirList->getLength() <= 0 ||
				!CvDeal::isDual(pTheirList->head()->m_data.m_eItemType)))
			return false;
	} // </advc.136b>

	return (iTheirValue >= iOurValue);
}

double CvPlayerAI::prDenyHelp() const {

	int contactRandGiveHelp = GC.getLeaderHeadInfo(getPersonalityType()).
			getContactRand(CONTACT_GIVE_HELP);
	double r = std::min(std::sqrt((double)contactRandGiveHelp) / 100.0, 0.5);
	return r;
}

// K-Mod. Helper fuction for AI_counterPropose. (lambas would be really nice here, but we can't have nice things.)
bool maxValueCompare(const std::pair<TradeData*, int>& a, const std::pair<TradeData*, int>& b)
{
	return a.second > b.second;
}

bool CvPlayerAI::AI_counterPropose(PlayerTypes ePlayer, const CLinkList<TradeData>* pTheirList, const CLinkList<TradeData>* pOurList, CLinkList<TradeData>* pTheirInventory, CLinkList<TradeData>* pOurInventory, CLinkList<TradeData>* pTheirCounter, CLinkList<TradeData>* pOurCounter) const
{
	//PROFILE_FUNC(); // advc.003b: Did a profiler run; neglible.
	// advc.705: Content moved into counterProposeBulk
	return counterProposeBulk(ePlayer, pTheirList, pOurList, pTheirInventory, pOurInventory, pTheirCounter, pOurCounter, 1);
}
// advc.705: Body cut and pasted from AI_counterPropose
bool CvPlayerAI::counterProposeBulk(PlayerTypes ePlayer, const CLinkList<TradeData>* pTheirList, const CLinkList<TradeData>* pOurList, CLinkList<TradeData>* pTheirInventory, CLinkList<TradeData>* pOurInventory, CLinkList<TradeData>* pTheirCounter, CLinkList<TradeData>* pOurCounter,
		double leniency) const // advc.705: Apply this to all assignments to iTheirValue
{
	bool bTheirGoldDeal = AI_goldDeal(pTheirList);
	bool bOurGoldDeal = AI_goldDeal(pOurList);

	if (bOurGoldDeal && bTheirGoldDeal)
	{
		return false;
	}

	/* original bts code
	iHumanDealWeight = AI_dealVal(ePlayer, pTheirList);
	iAIDealWeight = GET_PLAYER(ePlayer).AI_dealVal(getID(), pOurList); */
	// K-Mod note: the original code had the human and AI weights the wrong way around.
	// I found this confusing, so I've corrected it throughout this function.
	// (Under normal usage of this fuction, the AI player counters the human proposal - so "they" are human, not us.)
	int iValueForUs = AI_dealVal(ePlayer, pTheirList);
	int iValueForThem =
			::round(leniency * // advc.705
			GET_PLAYER(ePlayer).AI_dealVal(getID(), pOurList));
	/*  advc.001l: Moved into balanceDeal; has to be called on whichever side
		receives gold. */
	//int iGoldValuePercent = AI_goldTradeValuePercent();
	
	pTheirCounter->clear();
	pOurCounter->clear();

	// K-Mod. Refuse all war-time offers unless it's part of a peace deal
	if (atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		// Check to see if there is already an end-war item on the table
		bool bEndWar = false;
		for (CLLNode<TradeData>* pNode = pTheirList->head(); !bEndWar && pNode; pNode = pTheirList->next(pNode))
		{
			if (CvDeal::isEndWar(pNode->m_data.m_eItemType))
				bEndWar = true;
		}
		for (CLLNode<TradeData>* pNode = pOurList->head(); !bEndWar && pNode; pNode = pOurList->next(pNode))
		{
			if (CvDeal::isEndWar(pNode->m_data.m_eItemType))
				bEndWar = true;
		}

		// see if someone is willing to surrender
		if (!bEndWar)
		{
			TradeData item;
			setTradeItem(&item, TRADE_SURRENDER);
			if (canTradeItem(ePlayer, item, true))
			{
				pOurCounter->insertAtEnd(item);
				bEndWar = true;
			}
			else if (GET_PLAYER(ePlayer).canTradeItem(getID(), item, true))
			{
				pTheirCounter->insertAtEnd(item);
				bEndWar = true;
			}
		}
		// last chance: try a peace treaty.
		if (!bEndWar)
		{
			TradeData item;
			setTradeItem(&item, TRADE_PEACE_TREATY);
			if (canTradeItem(ePlayer, item, true) && GET_PLAYER(ePlayer).canTradeItem(getID(), item, true))
			{
				//pOurCounter->insertAtEnd(item);
				//pTheirCounter->insertAtEnd(item);
				// unfortunately, there is some weirdness in the game engine which causes it to flip its wig
				// if trade items are added to both sides of a peace deal... so we have to do it like this:
				if (iValueForThem > iValueForUs)
					pTheirCounter->insertAtEnd(item);
				else
					pOurCounter->insertAtEnd(item);
				bEndWar = true;
			}
			else
			{
				return false; // if there is no peace, there will be no trade
			}
		}
	}
	// <advc.132> Don't accept two civics of the same column.
	if(!checkCivicReligionConsistency(pOurList) ||
			!checkCivicReligionConsistency(pTheirList)) 
		return false; // </advc.132>
	// <advc.036>
	if(!checkResourceLimits(pOurList, pTheirList, ePlayer, 1)) 
		return false;
	/*  The above makes sure that not too many non-surplus resources are already
		in either list, but we also need to know how many more can be added to
		the list that needs further items. */
	CLinkList<TradeData> const* giverList = (iValueForThem < iValueForUs ?
			pOurList : pTheirList);
	CvPlayer const& giver = (iValueForThem < iValueForUs ? *this : GET_PLAYER(ePlayer));
	CvPlayer const& recipient = (giver.getID() == ePlayer ? *this : GET_PLAYER(ePlayer));
	// Count how many more can be traded
	int happyLeft, healthLeft;
	happyLeft = healthLeft = 2;
	if(GET_TEAM(giver.getTeam()).isCapitulated())
		happyLeft = healthLeft = INT_MAX;
	else {
		for(CLLNode<TradeData>* tdn = giverList->head(); tdn != NULL;
				tdn = giverList->next(tdn)) {
			if(tdn->m_data.m_eItemType != TRADE_RESOURCES)
				continue;
			BonusTypes eBonus = (BonusTypes)tdn->m_data.m_iData;
			CvBonusInfo& bi = GC.getBonusInfo(eBonus);
			if(!recipient.isHuman()) {
				/*  AI mustn't accept too many resources of the same kind
					(happy or health) at once */
				if(bi.getHappiness() > 0)
					happyLeft--;
				if(bi.getHealth() > 0)
					healthLeft--;
			}
			else {
				/*  Non-capitulated AI mustn't give too many non-surplus resources
					of the same kind (happy or health) at once */
				if(giver.getNumAvailableBonuses(eBonus) <= 1) {
					if(bi.getHappiness() > 0)
						happyLeft--;
					if(bi.getHealth() > 0)
						healthLeft--;
				}
			}
		}
		happyLeft = std::max(0, happyLeft);
		healthLeft = std::max(0, healthLeft);
	} // </advc.036>
	// <advc.136b>
	if(!isHuman() && iValueForUs > iValueForThem &&
			iValueForUs < 2 * GC.getDIPLOMACY_VALUE_REMAINDER())
		return false; // </advc.136b>
	// When counterProposing, we want balance the deal - but if we can't balance it, we want to make sure it favours us; not them.
	// So if their value is greater, we don't mind suggesting items which take them past balance.
	// But if our value is greater, we will never suggest adding items which overshoot the balance.
	if (iValueForThem > iValueForUs)
	{	// <advc.003> Moved into auxiliary function
		balanceDeal(bOurGoldDeal, pTheirInventory, ePlayer, iValueForThem,
			iValueForUs, pTheirCounter, pOurList, 1, true, happyLeft,
			healthLeft); // </advc.003>
	}
	else if (iValueForUs > iValueForThem)
	{
		// <advc.003> Moved into auxiliary function
		GET_PLAYER(ePlayer).balanceDeal(bTheirGoldDeal, pOurInventory, getID(),
				iValueForUs, iValueForThem, pOurCounter, pTheirList,
				leniency, false, happyLeft, healthLeft);
		// </advc.003>
	}
	/* original bts code
	return ((iValueForThem <= iValueForUs) && ((pOurList->getLength() > 0) || (pOurCounter->getLength() > 0) || (pTheirCounter->getLength() > 0))); */

	// K-Mod. This function now needs to handle AI - AI deals, and human auto-counters to AI suggested deals.
	if (pOurList->getLength() == 0 && pOurCounter->getLength() == 0 && pTheirCounter->getLength() == 0)
		return false;

	bool bDeal = false;

	if (GET_PLAYER(ePlayer).isHuman())
	{
		// AI counting a human proposal:
		// do not compromise the AI's value, and don't even consider the human's value.
		bDeal = iValueForUs >= iValueForThem;
		FAssert(!isHuman());
	}
	else
	{
		if (isHuman())
		{
			// Human civ auto-negotiating an AI proposal before it is put to the player for confirmation:
			// let the AI show a little bit of leniency
			// don't bother putting it to the player if it is really bad value
			bDeal = //4*iValueForUs >= iValueForThem && // advc.550b
				100*iValueForThem >= GET_PLAYER(ePlayer).AI_tradeAcceptabilityThreshold(getID())*iValueForUs;
			/*  <advc.550b> About the really bad deals: 4 to 1 is OK
				when the human doesn't give away tech. */
			bool bTech = false;
			for(CLLNode<TradeData>* node = pOurCounter->head(); node != NULL;
					node = pOurCounter->next(node)) {
				if(node->m_data.m_eItemType == TRADE_TECHNOLOGIES) {
					bTech = true;
					break;
				}
			}
			if(!bTech && 4 * iValueForUs < iValueForThem)
				bDeal = false;
			if(bTech  && 3 * iValueForUs < iValueForThem * 2)
				bDeal = false;
			// </advc.550b>
		}
		else
		{
			// Negotiations between two AIs:
			bDeal = 100*iValueForUs >= AI_tradeAcceptabilityThreshold(ePlayer)*iValueForThem
				&& 100*iValueForThem >= GET_PLAYER(ePlayer).AI_tradeAcceptabilityThreshold(getID())*iValueForUs;

		}
	} // <advc.705>
	CvGame const& g = GC.getGameINLINE();
	if(g.isOption(GAMEOPTION_RISE_FALL) && GET_PLAYER(ePlayer).isHuman()) {
		if(g.getRiseFall().isCooperationRestricted(getID())) {
			double thresh = g.getRiseFall().dealThresh(isAnnualDeal(*pOurList));
			if(bDeal && g.getRiseFall().pessimisticDealVal(
					getID(), iValueForThem, *pOurList) / (iValueForUs + 0.01) <
					thresh)
				bDeal = (g.getRiseFall().isSquareDeal(*pOurList, *pTheirList, getID()) &&
						 g.getRiseFall().isSquareDeal(*pOurCounter, *pTheirCounter, getID()));
		}
		double const secondAttempt = 0.9;
		if(!bDeal && !GET_TEAM(getTeam()).isGoldTrading() &&
				!TEAMREF(ePlayer).isGoldTrading() &&
				std::abs(leniency - secondAttempt) > 0.01)
			return counterProposeBulk(ePlayer, pTheirList, pOurList,
					pTheirInventory, pOurInventory, pTheirCounter, pOurCounter,
					secondAttempt);
	} // </advc.705>
	return bDeal;
	// K-Mod end
}

/*  <advc.003> Moved and refactored from AI_counterProposeBulk in order to reduce
	code duplication. 
	ePlayer is the owner of pInventory, the one who needs to give more.
	The callee is the one who needs to receive more.
	bGenerous means that the callee is willing to accept an unfavorable trade.
	The comments in the function body are from K-Mod. */
void CvPlayerAI::balanceDeal(bool bGoldDeal, CLinkList<TradeData> const* pInventory,
		PlayerTypes ePlayer, int& iGreaterVal, int& iSmallerVal,
		CLinkList<TradeData>* pCounter, CLinkList<TradeData> const* pList,
		double leniency, // advc.705
		bool bGenerous,
		int happyLeft, int healthLeft) const { // advc.036

	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	// First, try to make up the difference with gold.
	int iGoldValuePercent = kPlayer.AI_goldTradeValuePercent(); // advc.001l
	CLLNode<TradeData>* pGoldPerTurnNode = NULL;
	CLLNode<TradeData>* pGoldNode = NULL;
	if(!bGoldDeal) {
		for(CLLNode<TradeData>* pNode = pInventory->head(); pNode != NULL;
				pNode = pInventory->next(pNode)) {
			TradeData data = pNode->m_data;
			if(data.m_bOffering || data.m_bHidden)
				continue;
			/*  <advc.104> Avoid a potentially costly TradeDenial check for
				items that aren't covered by the switch block anyway. */
			TradeableItems ti = data.m_eItemType;
			if(ti != TRADE_GOLD && ti != TRADE_GOLD_PER_TURN)
				continue; // </advc.104>
			if (kPlayer.getTradeDenial(getID(), data) != NO_DENIAL)
				continue;
			FAssert(kPlayer.canTradeItem(getID(), data));
			switch(ti) {
			case TRADE_GOLD:
				pGoldNode = pNode;
				break;
			case TRADE_GOLD_PER_TURN:
				pGoldPerTurnNode = pNode;
				break;
			}
		}
	}
	if(pGoldNode != NULL) {
		int iGoldData = ((iGreaterVal - iSmallerVal) * 100 +
				// bGenerous: round up or down
				(bGenerous ? (iGoldValuePercent - 1) : 0)) /
				iGoldValuePercent;
		// <advc.026>
		if(isHuman() && !bGenerous)
			iGoldData -= (iGoldData % GC.getDIPLOMACY_VALUE_REMAINDER());
		int iMaxGold = ((isHuman() && bGenerous) ?
				kPlayer.maxGoldTradeGenerous(getID()) :
				kPlayer.AI_maxGoldTrade(getID())); 
		if(iMaxGold >= iGoldData) { // Replacing: </advc.026>
						// if(kPlayer.AI_maxGoldTrade(getID()) >= iGoldData) {
			pGoldNode->m_data.m_iData = iGoldData;
			iSmallerVal +=
				::round((leniency * // advc.705
				(iGoldData * iGoldValuePercent)) / 100);
			pCounter->insertAtEnd(pGoldNode->m_data);
			pGoldNode = NULL;
		}
	}
	/*  <advc.036> Try gpt before resources when trading with a human with
		negative income. Based on the "try one more time" code chunk below. */
	if(isHuman() && pGoldPerTurnNode != NULL && iGreaterVal > iSmallerVal &&
			calculateGoldRate() < 0) {
		int iGoldData = 0;
		int iGoldAvailable = kPlayer.AI_maxGoldPerTurnTrade(getID());
		while(AI_goldPerTurnTradeVal(iGoldData) < iGreaterVal - iSmallerVal &&
				iGoldData < iGoldAvailable)
			iGoldData++;
		int gptTradeVal = AI_goldPerTurnTradeVal(iGoldData);
		if(gptTradeVal >= iGreaterVal - iSmallerVal) {
			if(!bGenerous && gptTradeVal > iGreaterVal - iSmallerVal) {
				iGoldData--;
				gptTradeVal = AI_goldPerTurnTradeVal(iGoldData);
			}
			if(iGoldData > 0) {
				pGoldPerTurnNode->m_data.m_iData = iGoldData;
				iSmallerVal +=
						::round(leniency * // advc.705
						gptTradeVal);
				pCounter->insertAtEnd(pGoldPerTurnNode->m_data);
				pGoldPerTurnNode = NULL;
			}
		}
	} // </advc.036>
	std::pair<TradeData*, int> final_item(NULL, 0); // An item we may or may not use to finalise the deal. (See later)
	// advc.036:
	std::vector<std::pair<TradeData*, int> > nonsurplusItems;
	if(iGreaterVal > iSmallerVal) {
		// We were unable to balance the trade with just gold. So lets look at all the other items.
		// Exclude bonuses that we've already put on the table
		std::vector<bool> vbBonusDeal(GC.getNumBonusInfos(), false);
		for(CLLNode<TradeData>* pNode = pList->head(); pNode != NULL;
				pNode = pList->next(pNode)) {
			TradeData data = pNode->m_data;
			FAssert(!data.m_bHidden);
			if(data.m_eItemType == TRADE_RESOURCES)
				vbBonusDeal[pNode->m_data.m_iData] = true;
		}
		// We're only going to allow one city on the list. (For flavour reasons.)
		int iBestCityValue = 0;
		int iBestCityWeight = 0;
		CLLNode<TradeData>* pBestCityNode = NULL;
		// Evaluate everything they're willing to trade.
		std::vector<std::pair<TradeData*, int> > item_value_list; // (item*, value)
		CvGame& g = GC.getGameINLINE();
		for(CLLNode<TradeData>* pNode = pInventory->head(); pNode != NULL;
				pNode = pInventory->next(pNode)) {
			TradeData data = pNode->m_data;
			if(data.m_bOffering || data.m_bHidden)
				continue;
			/*  <advc.104> Avoid a potentially costly TradeDenial check for
				items that aren't covered by the switch block anyway. */
			TradeableItems ti = data.m_eItemType;
			if(ti != TRADE_MAPS && ti != TRADE_TECHNOLOGIES &&
					ti != TRADE_RESOURCES && ti != TRADE_CITIES)
				continue; // </advc.104>
			if(kPlayer.getTradeDenial(getID(), data) != NO_DENIAL)
				continue;
			FAssert(kPlayer.canTradeItem(getID(), data));
			bool bNonsurplus = false; // advc.036
			int iItemValue = 0;
			switch(ti) {
			case TRADE_MAPS:
				iItemValue = GET_TEAM(getTeam()).AI_mapTradeVal(kPlayer.getTeam());
				break;
			case TRADE_TECHNOLOGIES:
				iItemValue = GET_TEAM(getTeam()).AI_techTradeVal((TechTypes)
						data.m_iData, kPlayer.getTeam());
				break;
			case TRADE_RESOURCES:
				if(!vbBonusDeal[data.m_iData]) {
					// Don't ask for the last of a resource, or corporation resources; because we're not going to evaluate losses.
					// <advc.036> AI_bonusTradeVal evaluates losses now; moved up:
					BonusTypes eBonus = (BonusTypes)data.m_iData;
					iItemValue = AI_bonusTradeVal(eBonus, ePlayer, 1);
					/*  Bias AI-AI trades against resources that are of low value
						to the recipient; should rather try to trade these to
						someone else. */
					if(!isHuman() && !kPlayer.isHuman() &&
							// Don't skip items that fill the gap very well
							iGreaterVal - iSmallerVal - iItemValue >= 10) {
						/*  I think randomness in this function could lead to
							OOS problems */
						std::vector<long> hashInputs;
						hashInputs.push_back(g.getGameTurn());
						hashInputs.push_back(eBonus);
						if(::hash(hashInputs, getID()) > iItemValue / 100.0)
							continue;
					}
					vbBonusDeal[data.m_iData] = true;
					bNonsurplus = true;
					if((kPlayer.getNumTradeableBonuses(eBonus) > 1 && 
							kPlayer.AI_corporationBonusVal(eBonus) == 0)
							/*  Prefer to give non-surplus resources
								over gpt in AI-AI trades (i.e. no special
								treatment of nonsurplus resources needed).
								If a human is involved, gpt needs to be preferred
								b/c we don't want humans to minmax the gpt
								through trial and error. */
							|| (!isHuman() && !kPlayer.isHuman()))
						bNonsurplus = false;
					CvBonusInfo& bi = GC.getBonusInfo(eBonus);
					/*  Would be better to decrement the -left counters only once
						eBonus is added to pCounter, but that's complicated to
						implement. The important thing is to enforce the limits. */
					if(!kPlayer.isHuman()) {
						/*  AI mustn't give too many non-surplus resources of a
							kind (happy, health) at once */
						if(kPlayer.getNumAvailableBonuses(eBonus) <= 1) {
							if(bi.getHappiness() > 0 && happyLeft <= 0)
								continue;
							else happyLeft--;
							if(bi.getHealth() > 0 && healthLeft <= 0)
								continue;
							else healthLeft--;
						}
					} else { /* AI mustn't accept too many resources of a kind
								(happy, health) at once */
						if(bi.getHappiness() > 0 && happyLeft <= 0)
							continue;
						else happyLeft--;
						if(bi.getHealth() > 0 && healthLeft <= 0)
							continue;
						else healthLeft--;
					} // </advc.036>
				}
				break;
			case TRADE_CITIES:
				if(::atWar(kPlayer.getTeam(), getTeam())) {
					CvCity* pCity = kPlayer.getCity(data.m_iData);
					if(pCity != NULL) {
						int iWeight = AI_targetCityValue(pCity, false);
						if(iWeight > iBestCityWeight) {
							int iValue = AI_cityTradeVal(pCity);
							if(iValue > 0) {
								iBestCityValue = iValue;
								iBestCityWeight = iWeight;
								pBestCityNode = pNode;
							}
						}
					}
				}
				break;
			}
			if(iItemValue > 0 && (bGenerous ||
					iGreaterVal >= (iSmallerVal + iItemValue))) {
				// <advc.036>
				std::pair<TradeData*,int> itemValue = std::make_pair(
						&pNode->m_data, iItemValue);
				if(bNonsurplus)
					nonsurplusItems.push_back(itemValue);
				else item_value_list.push_back(itemValue); // </advc.036>
			}
		}
		if(pBestCityNode != NULL) {
			FAssert(bGenerous || iGreaterVal >= (iSmallerVal + iBestCityValue));
			item_value_list.push_back(std::make_pair(&pBestCityNode->m_data,
					iBestCityValue));
		}
		if(bGenerous) {
			// We want to get as close as we can to a balanced trade - but ensure that the deal favours us!
			// Find the best item, add it to the list; and repeat until we've closed the game in the trade values.
			while(iGreaterVal > iSmallerVal && !item_value_list.empty()) {
				int value_gap = iGreaterVal - iSmallerVal;
				// Find the best item to put us ahead, but as close to fair as possible.
				// Note: We're not doing this for perfect balance. We're counter-proposing so that the deal favours us!
				//   If we wanted to get closer to a balanced deal, we just remove that first condition.
				//   (Maybe that's what we should be doing for AI-AI trades; but there are still flavour considersations...)
				std::vector<std::pair<TradeData*, int> >::iterator it, best_it;
				for(best_it = it = item_value_list.begin();
						it != item_value_list.end(); it++) {
					if((it->second > value_gap && best_it->second < value_gap) ||
							std::abs(it->second - value_gap) <
							std::abs(best_it->second - value_gap))
						best_it = it;
				}
				// Only add the item if it will get us closer to balance.
				if(best_it->second <= 2*(iGreaterVal - iSmallerVal)) {
					pCounter->insertAtEnd(*best_it->first);
					iSmallerVal += best_it->second;
					item_value_list.erase(best_it);
				}
				else {
					// If nothing on the list can bring us closer to balance; we'll try to balance it with gold.
					// But if that doesn't work, we may need to add this last item. So lets bookmark it.
					final_item = *best_it;
					break;
				}
			}
		}
		else {
			// Sort the values from largest to smallest
			std::sort(item_value_list.begin(), item_value_list.end(), maxValueCompare);
			// Use the list to balance the trade.
			for (std::vector<std::pair<TradeData*, int> >::iterator it =
					item_value_list.begin(); it != item_value_list.end() &&
					iGreaterVal > iSmallerVal; it++) {
				int iItemValue = it->second;
				if(iGreaterVal >= iSmallerVal + iItemValue) {
					pCounter->insertAtEnd(*it->first);
					iSmallerVal +=
							::round(leniency * // advc.705
							iItemValue);
				}
			}
		}
	}
	// If their value is still higher, try one more time to make up the difference with gold.
	// If we're counter-proposing an AI deal, just get as close to the right value as we can.
	// But for humans, if they don't have enough gold then ask for one final item, to favour us.
	bool bAddFinalItem = false;
	if(iGreaterVal > iSmallerVal && (bGenerous ||
			// Consider the special case of a human player auto-counter-proposing a deal from an AI.
			// Humans are picky with trades. They turn down most AI deals. So to increase the chance of the human
			// ultimately accepting, lets see if the AI would allow the deal without any added gold. If the AI will
			// accept, then we won't add the gold. And this will be a rare example of the AI favouring the human.
			!kPlayer.isHuman() || 100*iSmallerVal >=
			AI_tradeAcceptabilityThreshold(ePlayer) * iGreaterVal)) {
		if(pGoldNode != NULL) {
			int iGoldData = ((iGreaterVal - iSmallerVal) * 100 +
					// bGenerous: round up or down
					(bGenerous ? (iGoldValuePercent - 1) : 0)) /
					iGoldValuePercent;
			//int iGoldAvailable = kPlayer.AI_maxGoldTrade(getID());
			// <advc.026> Replacing the above
			int iGoldAvailable = ((bGenerous && isHuman()) ?
					kPlayer.maxGoldTradeGenerous(getID()) :
					kPlayer.AI_maxGoldTrade(getID())); // </advc.026>
			if(bGenerous && kPlayer.isHuman() && iGoldData > iGoldAvailable)
				bAddFinalItem = true;
			else {
				iGoldData = std::min(iGoldData, iGoldAvailable);
				if(iGoldData > 0) {
					pGoldNode->m_data.m_iData = iGoldData;
					iSmallerVal +=
							::round(leniency * // advc.705
							((iGoldData * iGoldValuePercent) / 100));
					pCounter->insertAtEnd(pGoldNode->m_data);
					pGoldNode = NULL;
				}
			}
		}
		/*  <advc.001> If human asks for gold, then pGoldNode is NULL here,
			and the AI won't ask e.g. for a tech in exchange */
		else if(bGenerous && kPlayer.isHuman() && iGreaterVal > iSmallerVal &&
				pList->getLength() > 0 &&
				!CvDeal::isAnnual(pList->head()->m_data.m_eItemType))
			bAddFinalItem = true; // </advc.001>
	}
	if(iGreaterVal > iSmallerVal) {
		if(pGoldPerTurnNode != NULL) {
			int iGoldData = 0;
			while(AI_goldPerTurnTradeVal(iGoldData) < iGreaterVal - iSmallerVal)
				iGoldData++;
			if(!bGenerous && AI_goldPerTurnTradeVal(iGoldData) >
					iGreaterVal - iSmallerVal)
				iGoldData--;
			//int iGoldAvailable = kPlayer.AI_maxGoldPerTurnTrade(getID());
			// <advc.026> Replacing the above
			int iMaxGPT = ((isHuman() && bGenerous) ?
					kPlayer.maxGoldPerTurnTradeGenerous(getID()) :
					kPlayer.AI_maxGoldPerTurnTrade(getID()));
			int iGoldAvailable = iMaxGPT; // </advc.026>
			if(bGenerous && kPlayer.isHuman() && iGoldData > iGoldAvailable)
				bAddFinalItem = true;
			else {
				iGoldData = std::min(iGoldData, iGoldAvailable);
				if(iGoldData > 0) {
					pGoldPerTurnNode->m_data.m_iData = iGoldData;
					iSmallerVal +=
							::round(leniency * // advc.705
							AI_goldPerTurnTradeVal(pGoldPerTurnNode->
							m_data.m_iData));
					pCounter->insertAtEnd(pGoldPerTurnNode->m_data);
					pGoldPerTurnNode = NULL;
				}
			}
		}
		// <advc.001> See above at if(pGoldNode)...else
		else if(bGenerous && kPlayer.isHuman() && iGreaterVal > iSmallerVal &&
				pList->getLength() > 0 &&
				CvDeal::isAnnual(pList->head()->m_data.m_eItemType))
			bAddFinalItem = true; // </advc.001>
	} // <advc.036> A mix of the two K-Mod algorithms for item selection
	if(iGreaterVal > iSmallerVal && final_item.first == NULL) {
		while(iGreaterVal > iSmallerVal && !nonsurplusItems.empty()) {
			int value_gap = iGreaterVal - iSmallerVal;
			std::vector<std::pair<TradeData*, int> >::iterator it, best_it;
			for(best_it = it = nonsurplusItems.begin();
					it != nonsurplusItems.end(); it++) {
				if(!bGenerous && iSmallerVal + it->second > iGreaterVal)
					continue;
				if((bGenerous && it->second > value_gap &&
						best_it->second < value_gap) ||
						std::abs(it->second - value_gap) <
						std::abs(best_it->second - value_gap))
					best_it = it;
			}
			if(best_it->second <= 2 * (iGreaterVal - iSmallerVal) ||
					bGenerous) {
				pCounter->insertAtEnd(*best_it->first);
				iSmallerVal += best_it->second;
			}
			nonsurplusItems.erase(best_it);
		}
	} // </advc.036>
	// When counter proposing a suggestion from a human, the AI will insist on having the better value.
	// So lets add the cheapest item still on our list.
	// We would have added the item already if it was going to be 'fair'. So we can be sure will favour us.
	if(bAddFinalItem && final_item.first != NULL) {
		FAssert(iGreaterVal > iSmallerVal && kPlayer.isHuman());
		pCounter->insertAtEnd(*final_item.first);
		iSmallerVal += final_item.second;
		FAssert(iSmallerVal >= iGreaterVal);
	}
} // </advc.003>

// K-Mod. Minimum percentage of trade value that this player will accept.
// ie. Accept trades if 100 * value_for_us >= residual * value_for_them .
int CvPlayerAI::AI_tradeAcceptabilityThreshold(PlayerTypes eTrader) const
{
	if (isHuman())
		return 75;

	int iDiscount = 25;
	iDiscount += 10 * AI_getAttitudeWeight(eTrader) / 100;
	// This puts us between 15 (furious) and 35 (friendly)
	// in some later version, I might make this personality based.

	// adjust for team rank.
	int iRankDelta = GC.getGameINLINE().getTeamRank(GET_PLAYER(eTrader).getTeam()) - GC.getGameINLINE().getTeamRank(getTeam());
	iDiscount += 10 * iRankDelta / std::max(6, GC.getGameINLINE().countCivTeamsAlive());

	if (GET_PLAYER(eTrader).isHuman())
	{
		// note. humans get no discount for trades that they propose.
		// The discount here only applies to deals that the AI offers to the human.
		iDiscount = iDiscount*2/3;
		// <advc.134b> Change disabled again:
			/*  No discount if recently begged or demanded
				advc.130j: Can now check >1 instead of >0, which seems like a better
				balance. AI doesn't offer discounts if *very* recently begged. */
			//if(AI_getMemoryCount(eTrader, MEMORY_MADE_DEMAND_RECENT) > 1)
			//	iDiscount = 0;
		// </advc.134b>
	}
	return 100 - iDiscount;
}
// K-Mod end

// <advc.132>
bool CvPlayerAI::checkCivicReligionConsistency(CLinkList<TradeData> const* tradeItems) const {

	std::set<int> civicOptions;
	int religionChanges = 0;
	for(CLLNode<TradeData>* node = tradeItems->head(); node != NULL;
			node = tradeItems->next(node)) {
		if(node->m_data.m_eItemType == TRADE_CIVIC) {
			int opt = GC.getCivicInfo((CivicTypes)node->m_data.m_iData).
					getCivicOptionType();
			if(civicOptions.find(opt) != civicOptions.end())
				return false;
			civicOptions.insert(opt);
		}
		else if(node->m_data.m_eItemType == TRADE_RELIGION) {
			religionChanges++;
			if(religionChanges > 1)
				return false;
		}
	}
	return true;
} // </advc.132>

// <advc.036>
bool CvPlayerAI::checkResourceLimits(CLinkList<TradeData> const* weGive,
		CLinkList<TradeData> const* theyGive, PlayerTypes theyId, int iChange)
		const {

	/*  Resources in lists mustn't overlap. TradeDenial should already ensure
		this, but I don't think that's totally reliable. */
	std::set<int> resources1;
	std::set<int> resources2;
	std::set<int>* resources[] = { &resources1, &resources2 };
	CLinkList<TradeData> const* tradeItems[] = {weGive, theyGive};
	CvPlayer const* giver[] = {this,&GET_PLAYER(theyId)};
	for(int i = 0; i < 2; i++) {

		int nonSurplusHappy = 0;
		int nonSurplusHealth = 0;
		int happy, health;
		happy = health = 0;
		for(CLLNode<TradeData>* tdn = tradeItems[i]->head(); tdn != NULL;
				tdn = tradeItems[i]->next(tdn)) {
			if(tdn->m_data.m_eItemType != TRADE_RESOURCES)
				continue;
			BonusTypes eBonus = (BonusTypes)tdn->m_data.m_iData;
			resources[i]->insert(eBonus);
			CvBonusInfo& bi = GC.getBonusInfo(eBonus);
			bool bAvail = (giver[i]->getNumAvailableBonuses(eBonus) - iChange > 0);
			if(bi.getHappiness() > 0) {
				happy++;
				if(!bAvail)
					nonSurplusHappy++;
			}
			if(bi.getHealth() > 0) {
				health++;
				if(!bAvail)
					nonSurplusHealth++;
			}
		}
		/*  AI mustn't trade away too many non-surplus resources of a kind
			(happy, health) at once */
		if(!giver[i]->isHuman() && !GET_TEAM(giver[i]->getTeam()).isCapitulated() &&
				(nonSurplusHappy > 2 || nonSurplusHealth > 2))
			return false;
		/*  giver[(i + 1) % 2] is the recipient. AI mustn't accept too many
			resource of a kind (happy, health) at once. */
		if(!giver[(i + 1) % 2]->isHuman() && (happy > 2 || health > 2))
			return false;
	}
	if(resources1.empty() || resources2.empty()) // for speed
		return true;
	std::vector<int> intersection;
	std::set_intersection(resources1.begin(), resources1.end(),
			resources2.begin(), resources2.end(), std::back_inserter(
			intersection));
	return intersection.empty();
} // </advc.036>


int CvPlayerAI::AI_maxGoldTrade(PlayerTypes ePlayer) const
{
	PROFILE_FUNC();

	int iMaxGold;

	FAssert(ePlayer != getID());
	// <advc.104w>
	if(getWPAI.isEnabled() && TEAMREF(ePlayer).isAtWar(getTeam())) {
		int r = std::max(0, getGold());
		// Don't tell them exactly how much we can afford
		return r - (r % (2 * GC.getDIPLOMACY_VALUE_REMAINDER()));
	} // </advc.104w>
	if (isHuman() || (GET_PLAYER(ePlayer).getTeam() == getTeam()))
	{
		iMaxGold = getGold();
		// <advc.003>
		return std::max(0, iMaxGold);
	}
	/*else
	{*/ // </advc.003>
	/* original bts code
	iMaxGold = getTotalPopulation();
	iMaxGold *= (GET_TEAM(getTeam()).AI_getHasMetCounter(GET_PLAYER(ePlayer).getTeam()) + 10);
	iMaxGold *= GC.getLeaderHeadInfo(getPersonalityType()).getMaxGoldTradePercent();
	iMaxGold /= 100;
	iMaxGold -= AI_getGoldTradedTo(ePlayer);
	iResearchBuffer = -calculateGoldRate() * 12;
	iResearchBuffer *= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getResearchPercent();
	iResearchBuffer /= 100;
	iMaxGold = std::min(iMaxGold, getGold() - iResearchBuffer);
	iMaxGold = std::min(iMaxGold, getGold());
	iMaxGold -= (iMaxGold % GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER")); */
	
	// K-Mod. Similar, but with more personality, and with better handling of situations where the AI has lots of spare gold.
	int iTradePercent = GC.getLeaderHeadInfo(getPersonalityType()).getMaxGoldTradePercent();
	iMaxGold = getTotalPopulation();

	iMaxGold *= (GET_TEAM(getTeam()).AI_getHasMetCounter(GET_PLAYER(ePlayer).getTeam()) + 10);

	iMaxGold *= iTradePercent;
	iMaxGold /= 100;

	/*iMaxGold -= AI_getGoldTradedTo(ePlayer);
	iMaxGold = std::max(0, iMaxGold);*/
	//int iGoldRate = calculateGoldRate();
	// <advc.036> Replacing the above
	int iGoldRate = (AI_getAvailableIncome() - calculateInflatedCosts() -
			std::max(0, -getGoldPerTurn())) / 5; // </advc.036>
	if (iGoldRate > 0)
	{
		PROFILE("AI_maxGoldTrade: gold rate adjustment");
		int iTarget = AI_goldTarget();
		if (getGold() < iTarget)
		{
			iGoldRate -= (iTarget - getGold())/3;
			iGoldRate = std::max(0, iGoldRate);
		}
		else
		{
			iMaxGold += (getGold() - iTarget) * iTradePercent / 100;
		}
		iMaxGold += iGoldRate * 2 * iTradePercent / 100;
	}
	else
	{
		int iAdjustment = iGoldRate * 12;
		iAdjustment *= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getResearchPercent();
		iAdjustment /= 100;

		iMaxGold += iAdjustment;
	} // <advc.036>
	iMaxGold = adjustTradeGoldToDiplo(iMaxGold, ePlayer);
	// Moved from above the gold rate adjustment
	iMaxGold -= AI_getGoldTradedTo(ePlayer); // </advc.036>
	iMaxGold = range(iMaxGold, 0, getGold());
	// <advc.550f>
	if(GET_PLAYER(ePlayer).isHuman() && GET_TEAM(getTeam()).isTechTrading() ||
			TEAMREF(ePlayer).isTechTrading()) {
		TechTypes cr = getCurrentResearch();
		double techProgr = (cr == NO_TECH ? 0 : GET_TEAM(getTeam()).
				getResearchProgress(cr) / (double)std::max(1,
				GET_TEAM(getTeam()).getResearchCost(cr)));
		double goldRatio = std::min(1.0, 1.4 *
				(1 - std::min(std::abs(techProgr - 0.75),
				std::abs(techProgr + 0.25))) - 0.33);
		iMaxGold = (int)(iMaxGold * goldRatio);
	} // </advc.550f>
	iMaxGold -= (iMaxGold % GC.getDIPLOMACY_VALUE_REMAINDER());
	// K-Mod end
	return std::max(0, iMaxGold);
}

// <advc.036>
int CvPlayerAI::adjustTradeGoldToDiplo(int gold, PlayerTypes civId) const {

	double mult = 1;
	if(!TEAMREF(civId).isAtWar(getTeam()) &&
			!GET_TEAM(getTeam()).isCapitulated()) {
		AttitudeTypes att = AI_getAttitude(civId);
		switch(att) {
		case ATTITUDE_FURIOUS: mult = 0.33; break;
		case ATTITUDE_ANNOYED: mult = 0.67; break;
		case ATTITUDE_PLEASED: mult = 1.2; break;
		case ATTITUDE_FRIENDLY: mult = 1.4; break;
		}
	}
	return ::round(gold * mult);
} // </advc.036>

int CvPlayerAI::AI_maxGoldPerTurnTrade(PlayerTypes ePlayer) const {

	FAssert(ePlayer != getID()); // advc.003: Some refactoring in this function
	/*if (isHuman() || (GET_PLAYER(ePlayer).getTeam() == getTeam()))
	{
		iMaxGoldPerTurn = (calculateGoldRate() + (getGold() / GC.getDefineINT("PEACE_TREATY_LENGTH")));
	}*/
	// <advc.036>
	// Replacing the above:
	if(isHuman())
		return std::max(0, calculateGoldRate());
	// Don't pay gold to our capitulated vassal
	if(TEAMREF(ePlayer).isVassal(getTeam()) && TEAMREF(ePlayer).isCapitulated())
		return 0;
	/*  BtS caps the return value at calculateGoldRate, so the getGold()...
		part had no effect. The AI shouldn't make assumptions about human
		finances anyway. Let human use the gold slider to communicate how much
		gpt the AI can ask for in trade proposals. */
	int availableGold = (AI_getAvailableIncome() - getGoldPerTurn() -
			calculateInflatedCosts()) / 3;
	// Included in AvailableIncome, but don't want to divide it by 3.
	availableGold += getGoldPerTurn();
	availableGold = std::max(calculateGoldRate(), availableGold); // </advc.036>
	//availableGold = calculateGoldRate(); // BtS behavior
	// <advc.104w>
	if(getWPAI.isEnabled() && TEAMREF(ePlayer).isAtWar(getTeam())) {
		int r = std::max(0, availableGold);
		return r - (r % 5);
	} // </advc.104w>
	int iMaxGoldPerTurn = getTotalPopulation();
	iMaxGoldPerTurn *= 4; // advc.036
	iMaxGoldPerTurn *= GC.getLeaderHeadInfo(getPersonalityType()).
			getMaxGoldPerTurnTradePercent();
	iMaxGoldPerTurn /= 100;
	iMaxGoldPerTurn += std::min(0, getGoldPerTurnByPlayer(ePlayer));
	// <advc.036>
	iMaxGoldPerTurn = adjustTradeGoldToDiplo(iMaxGoldPerTurn, ePlayer);
	int r = ::range(iMaxGoldPerTurn, 0, std::max(0, availableGold));
	// This will only be relevant in the late game
	if(r > 10 * GC.getDIPLOMACY_VALUE_REMAINDER())
		r -= (r % GC.getDIPLOMACY_VALUE_REMAINDER());
	return r; // </advc.036>
}


int CvPlayerAI::AI_goldPerTurnTradeVal(int iGoldPerTurn) const
{
	int iValue = iGoldPerTurn * GC.getDefineINT("PEACE_TREATY_LENGTH");
	iValue *= AI_goldTradeValuePercent();
	iValue /= 100;
	
	return iValue;
}

// <advc.026>
int CvPlayerAI::maxGoldTradeGenerous(PlayerTypes theyId) const {

	double dr = AI_maxGoldTrade(theyId);
	double multiplier = 1 + GC.getDefineINT("AI_OFFER_EXTRA_GOLD_PERCENT") / 100.0;
	dr *= multiplier;
	int r = ::round(dr);
	r = std::min(getGold(), r);
	return std::max(0, r - (r % GC.getDIPLOMACY_VALUE_REMAINDER()));
}

int CvPlayerAI::maxGoldPerTurnTradeGenerous(PlayerTypes theyId) const {

	double r = AI_maxGoldPerTurnTrade(theyId);
	double multiplier = 1 + GC.getDefineINT("AI_OFFER_EXTRA_GOLD_PERCENT") / 100.0;
	r *= multiplier;
	return std::max(0, ::round(r));
}// </advc.026>

// (very roughly 4x gold / turn / city)
int CvPlayerAI::AI_bonusVal(BonusTypes eBonus, int iChange, bool bAssumeEnabled,
		bool bTrade) const // advc.036
{
	int iValue = 0;
	int iBonusCount = getNumAvailableBonuses(eBonus);
	// <advc.036>
	int tradeVal = 0;
	// More valuable if we have few resources for trade
	int surplus = 0;
	for(int i = 0; i < GC.getNumBonusInfos(); i++)
		surplus += std::max(0, getNumTradeableBonuses((BonusTypes)i) - 1);
	if(bTrade) /* Want surplus to matter mostly for buildValue and
				  foundValue, and not so much for prices charged in trade. */
		surplus = std::max(2, surplus);
	tradeVal = ::round(4 / std::sqrt((double)std::max(1, std::max(surplus,
			2 * (iBonusCount + iChange))))); // </advc.036>
	if ((iChange == 0) || ((iChange == 1) && (iBonusCount == 0)) || ((iChange == -1) && (iBonusCount == 1))
			// advc.036: Cover all strange cases here
			|| iChange + iBonusCount < 1)
	{
		//This is assuming the none-to-one or one-to-none case.
		iValue += AI_baseBonusVal(eBonus,
				bTrade); // advc.036
		iValue += AI_corporationBonusVal(eBonus);
		iValue = std::max(iValue, tradeVal); // advc.036
		// K-Mod.
		if (!bAssumeEnabled)
		{
			// Decrease the value if there is some tech reason for not having the bonus..
			const CvTeam& kTeam = GET_TEAM(getTeam());
			//if (!kTeam.isBonusRevealed(eBonus))
			// note. the tech is used here as a kind of proxy for the civ's readiness to use the bonus.
			if (!kTeam.isHasTech((TechTypes)GC.getBonusInfo(eBonus).getTechReveal()))
				iValue /= 2;
			if (!kTeam.isHasTech((TechTypes)GC.getBonusInfo(eBonus).getTechCityTrade()))
				iValue /= 2;
		}
		// K-Mod end
	}
	else
	{
		iValue += AI_corporationBonusVal(eBonus);
		//This is basically the marginal value of an additional instance of a bonus.
		//iValue += AI_baseBonusVal(eBonus) / 5;
		/*  advc.036: The potential for trades isn't that marginal, and the
			base value (for the first copy of a resource) is unhelpful. */
		iValue = std::max(iValue, tradeVal);
	}
	return iValue;
}

//Value sans corporation
// (K-Mod note: very vague units. roughly 4x gold / turn / city.)
int CvPlayerAI::AI_baseBonusVal(BonusTypes eBonus,
		bool bTrade) const // advc.036
{
	// advc.003: Reduced indentation throughout this function
	//recalculate if not defined
	if(!bTrade && // advc.036
			m_aiBonusValue[eBonus] != -1)
		return m_aiBonusValue[eBonus];
	// <advc.036>
	if(bTrade && m_aiBonusValueTrade[eBonus] != -1)
		return m_aiBonusValueTrade[eBonus]; // </advc.036>
	if(GET_TEAM(getTeam()).isBonusObsolete(eBonus)) {
		m_aiBonusValue[eBonus] = 0;
		m_aiBonusValueTrade[eBonus] = 0; // advc.036
		return m_aiBonusValue[eBonus];
	}
	PROFILE("CvPlayerAI::AI_baseBonusVal::recalculate");
	// <advc.036>
	double dValue = 0; // was int
	int happy = GC.getBonusInfo(eBonus).getHappiness();
	int health = GC.getBonusInfo(eBonus).getHealth();
	int buildingHappy = 0;
	int buildingHealth = 0;
	for(int i = 0; i < GC.getNumBuildingClassInfos(); i++) {
		BuildingClassTypes bct = (BuildingClassTypes)i;
		int bcc = getBuildingClassCount(bct);
		if(bcc <= 0)
			continue;
		BuildingTypes bt = (BuildingTypes)GC.getCivilizationInfo(
				getCivilizationType()).getCivilizationBuildings(bct);
		if(bt == NO_BUILDING)
			continue;
		buildingHappy += bcc * GC.getBuildingInfo(bt).getBonusHappinessChanges(eBonus);
		buildingHealth += bcc * GC.getBuildingInfo(bt).getBonusHealthChanges(eBonus);
	}
	if(buildingHappy > 0.55 * getNumCities())
		happy++;
	if(buildingHealth > 0.55 * getNumCities())
		health++;
	// What BtS did:
	/*iValue += (happy * 100);
	iValue += (health * 100);*/
	/*  Weight appears to be 100 when a city needs extra happiness or health
		to grow, a bit more when citizens are already angry or food is being
		lost. Generally lower than 100 because not all cities will need
		happiness or health for growth.
		Resource tiles can generally be worked without extra happiness or
		health. Being able to work an extra non-resource tile should be worth
		some 10 gpt per city. Improvement and building yields increase over the
		course of the game, but the service life of a citizen becomes shorter,
		the food needed for growth increases and the natural tile yields tend to
		decrease (worst tiles saved for last). Also, when a city near the
		happiness cap receives another luxury, it may still be unable to grow
		for lack of health. Tbd.: Could check whether health or happiness is
		the bottleneck and adjust BonusVal accordingly.
		5 gpt per 1 happiness could be reasonable; can still reduce that further
		in AI_bonusTradeVal. There's a division by 10 at the end of this function,
		and it's supposed to return 4 times the gpt per city, so everything still
		needs to be multiplied by 2: */
	double const scaleFactor = 2;
	/*  The weight functions always assign some value to happiness and health
		beyond what is needed for the current population, but the AI should
		secure bonuses some time before they're needed so that CvCityAI can
		tell when there is room for growth and prioritize food accordingly.
		2 is a bit much though, and 1 too little, so take the mean of the two. */
	int iExtraPop1 = 1, iExtraPop2 = 2;
	if(!bTrade) // Look ahead farther then (trades are fickle)
		iExtraPop1 = iExtraPop2;
	// Don't want iExtraPop to increase resource prices on the whole
	double extraPopFactor = std::max(0.5, 0.1 * (10 - 0.5 * (iExtraPop1 + iExtraPop2)));
	bool bAvailable = (getNumAvailableBonuses(eBonus) > 0);
	if(happy > 0) {
		/*  advc.912c: Better ignore getLuxuryModifier; don't want civs with a
			luxury modifier to trade for more luxuries, and don't want them to
			pay more more for luxuries. */
		double const civicsMod = 0;//getLuxuryModifier() / 200.0;
		// Considering to cancel a trade
		if(bTrade && bAvailable) { /* HappinessWeight can't handle iHappy=0;
									  increase extra pop instead */
			iExtraPop1 += happy;
			iExtraPop2 += happy;
		}
		dValue += scaleFactor * (0.5 + civicsMod) *
				(AI_getHappinessWeight(happy, iExtraPop1) +
				AI_getHappinessWeight(happy, iExtraPop2));
	}
	if(health > 0) {
		if(bTrade && bAvailable) {
			iExtraPop1 += health;
			iExtraPop2 += health;
		}
		// Lower multiplier b/c bad health is not as painful as anger
		dValue += (scaleFactor - 0.5) * 0.5 * (AI_getHealthWeight(health, iExtraPop1) +
				AI_getHealthWeight(health, iExtraPop2));
	}
	dValue *= extraPopFactor;
	// </advc.036>
	CvTeam& kTeam = GET_TEAM(getTeam());
	CvCity* pCapital = getCapitalCity();
	int iCityCount = getNumCities();
	int iCoastalCityCount = countNumCoastalCities();

	// find the first coastal city
	CvCity* pCoastalCity = NULL;
	CvCity* pUnconnectedCoastalCity = NULL;
	if (iCoastalCityCount > 0)
	{
		int iLoop;
		for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			if(!pLoopCity->isCoastal())
				continue;
			if (pLoopCity->isConnectedToCapital(getID()))
			{
				pCoastalCity = pLoopCity;
				break;
			}
			else if (pUnconnectedCoastalCity == NULL)
			{
				pUnconnectedCoastalCity = pLoopCity;
			}
		}
	}
	if (pCoastalCity == NULL && pUnconnectedCoastalCity != NULL)
	{
		pCoastalCity = pUnconnectedCoastalCity;
	}


	for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		UnitTypes eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

		if(eLoopUnit == NO_UNIT)
			continue;
		CvUnitInfo& kLoopUnit = GC.getUnitInfo(eLoopUnit);
		/* original bts code
		... */ // advc.003: deleted
		// K-Mod. Similar, but much better. (maybe)
		const int iBaseValue = //30;
			/*  <advc.031> This value gets multiplied by the number of cities
				when converted to gold per turn, but AI_foundValue does not do
				this, and a civ that never expands beyond 6 cities should still
				value access to e.g. nukes more than to Swordsmen, if only
				because gold is more readily available in the late game. */
				std::max(10,
				::round(12 * std::pow((double)kLoopUnit.getPowerValue(), 0.5)));
				// </advc.031>
		int iUnitValue = 0;
		if (kLoopUnit.getPrereqAndBonus() == eBonus)
		{
			iUnitValue = iBaseValue;
		}
		else
		{
			int iOrBonuses = 0;
			int iOrBonusesWeHave = 0; // excluding eBonus itself. (disabled for now. See comments below.)
			bool bIsOrBonus = false; // is eBonus one of the OrBonuses for this unit.

			for (int iJ = 0; iJ < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); iJ++)
			{
				BonusTypes ePrereqBonus = (BonusTypes)kLoopUnit.getPrereqOrBonuses(iJ);

				if (ePrereqBonus != NO_BONUS)
				{
					iOrBonuses++;
					//iOrBonusesWeHave += (ePrereqBonus != eBonus && getNumAvailableBonuses(ePrereqBonus)) ? 1 : 0;
					// @*#!  It occurs to me that using state-dependant stuff such as NumAvailableBonuses here could result in OOS errors.
					// This is because the code here can be trigged by local UI events, and then the value could be cached...
					// It's very frustrating - because including the effect from iOrBonusesWeHave was going to be a big improvment.
					// The only way I can think of working around this is to add a 'bConstCache' argument to this function...
					bIsOrBonus = bIsOrBonus || ePrereqBonus == eBonus;
				}
			}
			if (bIsOrBonus)
			{
				// 1: 1, 2: 2/3, 3: 1/2, ...
				iUnitValue = iBaseValue * 2 / (1+iOrBonuses+2*iOrBonusesWeHave);
			}
		}
		bool bCanTrain = false;
		if (iUnitValue > 0)
		{
			// devalue the unit if we wouldn't be able to build it anyway
			if (canTrain(eLoopUnit))
			{
				// is it a water unit and no coastal cities or our coastal city cannot build because its obsolete
				if ((kLoopUnit.getDomainType() == DOMAIN_SEA && (pCoastalCity == NULL || pCoastalCity->allUpgradesAvailable(eLoopUnit) != NO_UNIT)) ||
					// or our capital cannot build because its obsolete (we can already build all its upgrades)
					(pCapital != NULL && pCapital->allUpgradesAvailable(eLoopUnit) != NO_UNIT))
				{
					// its worthless
					iUnitValue = 0;
				}
				bCanTrain = true;
			}
			else
			{
				// there is some other reason why we can't build it. (maybe the unit is maxed out, or maybe we don't have the techs)
				iUnitValue /= 2;
				/*  <advc.036> Evaluation for trade should be short-term; if we
					can't use it right away, we shouldn't trade for it. */
				if(bTrade) {
					/*  This makes sure that we're not very close to the unit.
						Otherwise, a human could e.g. trade for Ivory one turn
						before finishing Construction. */
					TechTypes mainReq = (TechTypes)kLoopUnit.getPrereqAndTech();
					if(mainReq != NO_TECH && !kTeam.isHasTech(mainReq) &&
							kTeam.getResearchProgress(mainReq) <= 0)
						iUnitValue /= 3;
				} // </advc.036>
			}
		}
		if (iUnitValue > 0)
		{
			// devalue units for which we already have a better replacement.
			UnitAITypes eDefaultAI = (UnitAITypes)kLoopUnit.getDefaultUnitAIType();
			int iNewTypeValue = AI_unitValue(eLoopUnit, eDefaultAI, 0);
			int iBestTypeValue = AI_bestAreaUnitAIValue(eDefaultAI, 0);
			if (iBestTypeValue > 0)
			{
				int iNewValue = AI_unitValue(eLoopUnit, eDefaultAI, 0);
				iUnitValue = iUnitValue * std::max(0, std::min(100, 120*iNewValue / iBestTypeValue - 20)) / 100;
			}

			// devalue units which are related to our current era. (but not if it is still our best unit!)
			if (kLoopUnit.getPrereqAndTech() != NO_TECH)
			{
				int iDiff = GC.getTechInfo((TechTypes)(kLoopUnit.getPrereqAndTech())).getEra() - getCurrentEra();
				if (iDiff > 0
					//|| !bCanTrain || iNewTypeValue < iBestTypeValue) {
					/*  advc.036: The assignment below doesn't do anything
						if iDiff is 0, so alternative conditions have no effect.
						"but not if it is still our best unit!" sounds like it
						should be an AND. !bCanTrain is sufficiently penalized
						above. */
						&& iNewTypeValue < iBestTypeValue) {
					/*  advc.031: Replacing the line below b/c Horse was valued
						too highly in the late game */
					iUnitValue = iUnitValue/(1 + std::abs(iDiff));
					//iUnitValue = iUnitValue * 2/(2 + std::abs(iDiff));
				}
			}
			dValue += iUnitValue;
		}
		// K-Mod end
	}

	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingTypes eLoopBuilding = ((BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI)));

		if(eLoopBuilding == NO_BUILDING)
			continue;
		CvBuildingInfo& kLoopBuilding = GC.getBuildingInfo(eLoopBuilding);

		int iTempValue = 0;

		if (kLoopBuilding.getPrereqAndBonus() == eBonus)
		{
			iTempValue += 25; // advc.036: was 30
		}

		for (int iJ = 0; iJ < GC.getNUM_BUILDING_PREREQ_OR_BONUSES(); iJ++)
		{
			if (kLoopBuilding.getPrereqOrBonuses(iJ) == eBonus)
			{
				iTempValue += 15; // advc.036: was 20
			}
		}

		iTempValue += kLoopBuilding.getBonusProductionModifier(eBonus) / 10;

		if (kLoopBuilding.getPowerBonus() == eBonus)
		{
			iTempValue += 40; // advc.036: was 60
		}

		for (int iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
		{
			iTempValue += kLoopBuilding.getBonusYieldModifier(eBonus, iJ) / 2;
			if (kLoopBuilding.getPowerBonus() == eBonus)
			{
				iTempValue += kLoopBuilding.getPowerYieldModifier(iJ);
			}
		}

		{
			// determine whether we have the tech for this building
			bool bHasTechForBuilding = true;
			if (!(kTeam.isHasTech((TechTypes)(kLoopBuilding.getPrereqAndTech()))))
			{
				bHasTechForBuilding = false;
			}
			for (int iPrereqIndex = 0; bHasTechForBuilding && iPrereqIndex < GC.getNUM_BUILDING_AND_TECH_PREREQS(); iPrereqIndex++)
			{
				if (kLoopBuilding.getPrereqAndTechs(iPrereqIndex) != NO_TECH)
				{
					if (!(kTeam.isHasTech((TechTypes)(kLoopBuilding.getPrereqAndTechs(iPrereqIndex)))))
					{
						bHasTechForBuilding = false;
					}
				}
			}

			bool bIsStateReligion = (((ReligionTypes) kLoopBuilding.getStateReligion()) != NO_RELIGION);

			//check if function call is cached
			bool bCanConstruct = canConstruct(eLoopBuilding, false, /*bTestVisible*/ true, /*bIgnoreCost*/ true);

			// bCanNeverBuild when true is accurate, it may be false in some cases where we will never be able to build 
			bool bCanNeverBuild = (bHasTechForBuilding && !bCanConstruct && !bIsStateReligion);

			// if we can never build this, it is worthless
			if (bCanNeverBuild)
			{
				iTempValue = 0;
			}
			// double value if we can build it right now
			else if (bCanConstruct)
			{
				if(!bTrade) // advc.036
					iTempValue *= 2;
				/*  advc.036: Even if we can start building right now, the
					bonus can be taken away again during construction. */
				else iTempValue = ::round(iTempValue * 1.5);
			} // <advc.036> Don't trade for the bonus until we need it
			if(bTrade && !bCanConstruct)
				iTempValue = 0; // </advc.036>
			// if non-limited water building, weight by coastal cities
			if (kLoopBuilding.isWater() && !isLimitedWonderClass((BuildingClassTypes)(kLoopBuilding.getBuildingClassType())))
			{
				iTempValue *= iCoastalCityCount;
				iTempValue /= std::max(1, iCityCount/2);
			}

			if (kLoopBuilding.getPrereqAndTech() != NO_TECH)
			{
				int iDiff = abs(GC.getTechInfo((TechTypes)(kLoopBuilding.getPrereqAndTech())).getEra() - getCurrentEra());

				if (iDiff == 0)
				{
					iTempValue *= 3;
					iTempValue /= 2;
				}
				else
				{
					iTempValue /= iDiff;
					// <advc.036>
					if(bTrade)
						iTempValue = 0; // </advc.036>
				}
			}
			dValue += iTempValue;
		}
	}

	for (int iI = 0; iI < GC.getNumProjectInfos(); iI++)
	{
		ProjectTypes eProject = (ProjectTypes) iI;
		CvProjectInfo& kLoopProject = GC.getProjectInfo(eProject);
		int iTempValue = 0;

		iTempValue += kLoopProject.getBonusProductionModifier(eBonus) / 10;

		if(iTempValue <= 0)
			continue;
		bool bMaxedOut = (GC.getGameINLINE().isProjectMaxedOut(eProject) || kTeam.isProjectMaxedOut(eProject));

		if (bMaxedOut)
		{
			// project worthless
			iTempValue = 0;
		}
		else if (canCreate(eProject))
		{
			iTempValue *= 2;
		}

		if (kLoopProject.getTechPrereq() != NO_TECH)
		{
			int iDiff = abs(GC.getTechInfo((TechTypes)(kLoopProject.getTechPrereq())).getEra() - getCurrentEra());

			if (iDiff == 0)
			{
				iTempValue *= 3;
				iTempValue /= 2;
			}
			else
			{
				iTempValue /= iDiff;
				// <advc.036>
				if(bTrade)
					iTempValue = 0; // <advc.036>
			}
		}

		dValue += iTempValue;
	}

	RouteTypes eBestRoute = getBestRoute();
	for (int iI = 0; iI < GC.getNumBuildInfos(); iI++)
	{
		RouteTypes eRoute = (RouteTypes)(GC.getBuildInfo((BuildTypes)iI).getRoute());
		if(eRoute == NO_ROUTE)
			continue;
		int iTempValue = 0;
		if (GC.getRouteInfo(eRoute).getPrereqBonus() == eBonus)
		{
			iTempValue += 80;
		}
		for (int iJ = 0; iJ < GC.getNUM_ROUTE_PREREQ_OR_BONUSES(); iJ++)
		{
			if (GC.getRouteInfo(eRoute).getPrereqOrBonus(iJ) == eBonus)
			{
				iTempValue += 40;
			}
		}
		if ((eBestRoute != NO_ROUTE) && (GC.getRouteInfo(getBestRoute()).getValue() <= GC.getRouteInfo(eRoute).getValue()))
		{
			dValue += iTempValue;
		}
		else
		{
			dValue += iTempValue / 2;
		} // <advc.036> The usual tech checks
		TechTypes tech = (TechTypes)GC.getBuildInfo((BuildTypes)iI).getTechPrereq();
		if(tech != NO_TECH && !GET_TEAM(getTeam()).isHasTech(tech)) {
			iTempValue /= 2;
			EraTypes techEra = (EraTypes)GC.getTechInfo(tech).getEra();
			int eraDiff = techEra - getCurrentEra();
			if(eraDiff > 0)
				iTempValue /= eraDiff;
			if(bTrade)
				iTempValue = 0;
		} // </advc.036>
	}

	//	int iCorporationValue = AI_corporationBonusVal(eBonus);
	//	iValue += iCorporationValue;
	//
	//	if (iCorporationValue <= 0 && getNumAvailableBonuses(eBonus) > 0)
	//	{
	//		iValue /= 3;
	//	}

	dValue /= 10;
	// <advc.036>
	int iValue = ::round(dValue);
	iValue = std::max(0, iValue);
	if(!bTrade) {
		m_aiBonusValue[eBonus] = iValue;
		return iValue;
	}
	m_aiBonusValueTrade[eBonus] = iValue;
	return iValue; // </advc.036>
}

int CvPlayerAI::AI_corporationBonusVal(BonusTypes eBonus) const
{
	int iValue = 0;
	int iCityCount = getNumCities();
	iCityCount += iCityCount / 6 + 1;

	for (int iCorporation = 0; iCorporation < GC.getNumCorporationInfos(); ++iCorporation)
	{
		int iCorpCount = getHasCorporationCount((CorporationTypes)iCorporation);
		if (iCorpCount > 0)
		{
			int iNumCorpBonuses = 0;
			iCorpCount += getNumCities() / 6 + 1;
			CvCorporationInfo& kCorp = GC.getCorporationInfo((CorporationTypes)iCorporation);
			for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
			{
				if (eBonus == kCorp.getPrereqBonus(i))
				{
					iValue += (50 * kCorp.getYieldProduced(YIELD_FOOD) * iCorpCount) / iCityCount;
					iValue += (50 * kCorp.getYieldProduced(YIELD_PRODUCTION) * iCorpCount) / iCityCount;
					iValue += (30 * kCorp.getYieldProduced(YIELD_COMMERCE) * iCorpCount) / iCityCount;

					iValue += (30 * kCorp.getCommerceProduced(COMMERCE_GOLD) * iCorpCount) / iCityCount;
					iValue += (30 * kCorp.getCommerceProduced(COMMERCE_RESEARCH) * iCorpCount) / iCityCount;
					//iValue += (12 * kCorp.getCommerceProduced(COMMERCE_CULTURE) * iCorpCount) / iCityCount;
					// K-Mod, I'd love to calculate this stuff properly, but because of the way trade currently operates...
					iValue += (20 * kCorp.getCommerceProduced(COMMERCE_CULTURE) * iCorpCount) / iCityCount;
					iValue += (20 * kCorp.getCommerceProduced(COMMERCE_ESPIONAGE) * iCorpCount) / iCityCount;
					
					//Disabled since you can't found/spread a corp unless there is already a bonus,
					//and that bonus will provide the entirity of the bonusProduced benefit.

					/*if (NO_BONUS != kCorp.getBonusProduced())
					{
						if (getNumAvailableBonuses((BonusTypes)kCorp.getBonusProduced()) == 0)
						{
							iBonusValue += (1000 * iCorpCount * AI_baseBonusVal((BonusTypes)kCorp.getBonusProduced())) / (10 * iCityCount);
					}
					}*/
				}
			}
		}
	}

	iValue /= 100;	//percent
	iValue /= 10;	//match AI_baseBonusVal

	return iValue;
}

// <advc.036> Rewritten
int CvPlayerAI::AI_bonusTradeVal(BonusTypes eBonus, PlayerTypes ePlayer,
		int iChange) const {

	PROFILE_FUNC();
	FAssert(ePlayer != getID());
	bool useOurBonusVal = true;
	if(isHuman()) {
		/*  If this CvPlayer is human and ePlayer an AI, then this function
			needs to say how much the human values eBonus in ePlayer's
			estimation. Can't just use this->AI_bonusVal b/c this uses
			information that ePlayer might not have. */
		CvBonusInfo& bi = GC.getBonusInfo(eBonus);
		// I can live with cheating when it comes to strategic bonuses
		useOurBonusVal = (bi.getHappiness() + bi.getHealth() == 0);
		// Also don't worry about the value of additional copies (for corps)
		if(!useOurBonusVal && ((getNumAvailableBonuses(eBonus) > 0 && iChange > 0) ||
				(getNumAvailableBonuses(eBonus) - iChange > 1)))
			useOurBonusVal = true;
		// Otherwise ePlayer needs to know most of the human's territory
		if(!useOurBonusVal) {
			int revThresh = 2 * getNumCities() / 3;
			int revCount = 0;
			TeamTypes eTeam = TEAMID(ePlayer); int dummy=-1;
			for(CvCity* c = firstCity(&dummy); c != NULL; c = nextCity(&dummy)) {
				if(c->isRevealed(eTeam, false)) {
					revCount++;
					if(revCount >= revThresh) {
						useOurBonusVal = true;
						break;
					}
				}
			}
		}
	}
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	double ourVal = useOurBonusVal ? AI_bonusVal(eBonus, iChange, false, true) :
			// Use ePlayer's value as a substitute
			kPlayer.AI_bonusVal(eBonus, 0, false, true);
	ourVal *= getNumCities(); // bonusVal is per city
	/*  Don't pay fully b/c trade doesn't give us permanent access to the
		resource, and b/c it tends to be (and should be) a buyer's market. */
	ourVal = std::pow(ourVal, 0.7);
	/*  What else could ePlayer do with the resource? Trade it to somebody else.
		I'm assuming that trade denial has already filtered out resources that
		ePlayer (really) needs idomestically. */
	int cityCount = 0;
	int knownCivs = 0;
	int otherTakers = 0;
	/*  Can't check here if eBonus is strategic; use the strategic thresh minus 1
		as a compromise. */
	int refuseThresh = std::max(0, GC.getLeaderHeadInfo(kPlayer.getPersonalityType()).
			getStrategicBonusRefuseAttitudeThreshold() - 1);
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayerAI const& civ = GET_PLAYER((PlayerTypes)i);
		if(!civ.isAlive() || !GET_TEAM(getTeam()).isHasMet(civ.getTeam()) ||
				!TEAMREF(ePlayer).isHasMet(civ.getTeam()))
			continue;
		/*  The trade partners don't necessarily know all those cities, but the
			city count wouldn't be difficult to estimate. */
		cityCount += civ.getNumCities();
		knownCivs++;
		if(civ.getID() == ePlayer || civ.getID() == getID())
			continue;
		/*  Based on a profiler test, too slow:
			(maybe b/c of the is-strategic check in AI_bonusTrade) */
		/*TradeData item;
		item.m_eItemType = TRADE_RESOURCES;
		item.m_iData = eBonus;
		if(kPlayer.canTradeItem(civ.getID(), item, true))
			otherTakers++;*/
		// Also a little slow b/c of advc.124
		//if(!kPlayer.canTradeNetworkWith(civ.getID())
		// Checking just the essentials is fine though; it's only a heuristic.
		if(!civ.getCapitalCity()->isConnectedToCapital(ePlayer) ||
				civ.getNumAvailableBonuses(eBonus) > 0)
			continue;
		if(!kPlayer.isHuman() && kPlayer.AI_getAttitude(civ.getID()) <= refuseThresh)
			continue;
		if(!civ.isHuman() && civ.AI_getAttitude(ePlayer) <= std::max(0,
				GC.getLeaderHeadInfo(civ.getPersonalityType()).
				getStrategicBonusRefuseAttitudeThreshold() - 1))
			continue;
		otherTakers++;
	}
	int avail = kPlayer.getNumAvailableBonuses(eBonus);
	// Decrease marketVal if we have multiple spare copies
	otherTakers = ::range(otherTakers - avail + 2, 0, otherTakers);
	FAssert(knownCivs >= 2);
	/*  Hard to say how much a third party needs eBonus, but it'd certainly
		factor in its number of cities. */
	double marketVal = (4.0 * cityCount) / (3.0 * knownCivs) +
			std::sqrt((double)otherTakers);
	double const marketWeight = 2/3.0;
	/*  Want luxuries and food resources to be cheap, but not necessarily
		strategic resources. At 1/3 weight for ourVal, crucial strategics
		are still going to be expensive. */
	double r = marketWeight * marketVal + (1 - marketWeight) * ourVal;
	/*  Trade denial already ensures that ePlayer doesn't really need the
		resource. Don't want to factor in ePlayer's AI_bonusVal; just a little
		nudge to make sure that the AI doesn't offer unneeded nonsurplus resources at
		the same price as surplus resources. Exception for human b/c I don't
		want humans to keep their surplus resources and sell unneeded nonsurplus
		resources instead for a slightly higher price. */
	if(!kPlayer.isHuman() && (avail - iChange == 0 ||
			avail == 0 || // when considering cancellation
			kPlayer.AI_corporationBonusVal(eBonus) > 0))
		r++;
	if(!isHuman()) // Never pay more than it's worth to us
		r = std::min(r, ourVal);
	r *= std::max(0, GC.getBonusInfo(eBonus).getAITradeModifier() + 100) / 100.0;
	if(TEAMREF(ePlayer).isVassal(getTeam()) && !TEAMREF(ePlayer).isCapitulated())
		r *= 0.67; // advc.037: 0.5 in BtS
	int const treatyLength = GC.getDefineINT("PEACE_TREATY_LENGTH");
	int ir = ::round(r);
	/*  To make resource vs. resource trades more compatible. A multiple of 5
		would lead to a rounding error when gold is paid for a resource b/c
		2 gpt correspond to 1 tradeVal. */
	if(r >= 3 && !GET_TEAM(getTeam()).isGoldTrading() &&
			!TEAMREF(ePlayer).isGoldTrading())
		ir = ::roundToMultiple(ir, 4);
	return ir * treatyLength;
}  // </advc.036>

DenialTypes CvPlayerAI::AI_bonusTrade(BonusTypes eBonus, PlayerTypes ePlayer,
		int iChange) const // advc.133
{
	PROFILE_FUNC();

	AttitudeTypes eAttitude;
	bool bStrategic;
	int iI, iJ;
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer); // advc.003
	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");

	if (isHuman() && kPlayer.isHuman())
	{
		return NO_DENIAL;
	}
	/*  <advc.036> No wheeling and dealing, not even when it involves
		vassal/master or team mates. */
	if(kPlayer.getBonusExport(eBonus) + getBonusImport(eBonus) > 0) {
		/*  So that AI_buildTradeTable filters these out (b/c it's not easy
			to see when the AI is already exporting a resource; can be puzzling) */
		return DENIAL_JOKING;
	} // </advc.036>
	// advc.133:
	int iAvailThem = kPlayer.getNumAvailableBonuses(eBonus) + iChange;
	// advc.036: Moved this clause up
	if (iAvailThem > 0 && kPlayer.AI_corporationBonusVal(eBonus) <= 0)
	{
		return DENIAL_JOKING;
	} // <advc.037> This used to apply to all vassals
	bool bVassal = GET_TEAM(getTeam()).isVassal(TEAMID(ePlayer));
	if(bVassal && GET_TEAM(getTeam()).isCapitulated()) // </advc.037>
		return NO_DENIAL;

	if (atWar(getTeam(), TEAMID(ePlayer)))
	{	/*  advc.036 (comment): Doesn't help b/c there is no trade network while
			at war, and no feasible way to ensure that it will exist after
			making peace. */
		return NO_DENIAL;
	}

	//if (GET_PLAYER(ePlayer).getTeam() == getTeam())
	// advc.155: Commented out
	/*if (TEAMID(ePlayer) == getTeam() && kPlayer.isHuman()) // K-Mod
	{
		return NO_DENIAL;
	}*/
	// K-Mod (The above case should be tested for humans trying to give stuff to AI teammates
	// - otherwise the human won't know if the AI can actually use the resource.)
	/*if (TEAMID(ePlayer) == getTeam())
		return NO_DENIAL;*/
	// K-Mod end
	// <advc.036>
	int const tradeValThresh = 3;
	int valueForThem = -1;
	/*  advc.133: Don't bother checking the value-based clauses when considering
		cancelation. AI_considerOffer handles that. */
	if(iChange >= 0) {
		valueForThem = kPlayer.AI_bonusVal(eBonus, 0); // </advc.036>
		if (isHuman()
				/*  advc.036: No deal if they (AI) don't need the resource, but
					check attitude before giving the human that info. */
				&& valueForThem >= tradeValThresh)
		{
			return NO_DENIAL;
		}
	}
	if (GET_TEAM(getTeam()).AI_getWorstEnemy() == TEAMID(ePlayer))
	{
		return DENIAL_WORST_ENEMY;
	}
	// advc.036: Commented out
	/*if (AI_corporationBonusVal(eBonus) > 0)
	{
		return DENIAL_JOKING;
	}*/

	bStrategic = false;
	bool crucialStrategic = false; // advc.036
// <advc.001>
/************************************************************************************************/
/* Fuyu                    Start		 22.07.2010                                             */
/*                                                                                              */
/*  Better AI: Strategic For Current Era                                                        */
/************************************************************************************************/
	//disregard obsolete units
	CvCity* pCapitalCity = getCapitalCity();
	for (iI = 0; iI < GC.getNumUnitInfos(); iI++)
	{
		if (pCapitalCity != NULL && pCapitalCity->allUpgradesAvailable(
				(UnitTypes)iI) != NO_UNIT)
		{
			continue;
		}
/************************************************************************************************/
/* Fuyu                          END                                                            */
/************************************************************************************************/
// </advc.001>
		if (GC.getUnitInfo((UnitTypes) iI).getPrereqAndBonus() == eBonus)
		{
			bStrategic = true;
			crucialStrategic = true; // advc.036
		}
		for (iJ = 0; iJ < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); iJ++)
		{
			if (GC.getUnitInfo((UnitTypes)iI).getPrereqOrBonuses(iJ) == eBonus)
			{
				bStrategic = true;
				/*  Could check if we have one of the alternative OR-prereqs
					(getCapitalCity()->hasBonus(...), but then a human could
					try to trade for both OR-prereqs at once (e.g. Bronze and
					Iron, leaving us unable to train Axemen). */
				crucialStrategic = true;
			}
		}
	}

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		if (GC.getBuildingInfo((BuildingTypes) iI).getPrereqAndBonus() == eBonus)
		{
			bStrategic = true;
		}

		for (iJ = 0; iJ < GC.getNUM_BUILDING_PREREQ_OR_BONUSES(); iJ++)
		{
			if (GC.getBuildingInfo((BuildingTypes) iI).getPrereqOrBonuses(iJ) == eBonus)
			{
				bStrategic = true;
			}
		}
	}

	// XXX marble and stone???

	if(!isHuman() // advc.036: No longer guaranteed (b/c of the tradeValThresh clause)
			&& !bVassal) { // advc.037
		eAttitude = AI_getAttitude(ePlayer);
		if (bStrategic)
		{	// <advc.036>
			int iRefusalThresh = GC.getLeaderHeadInfo(getPersonalityType()).
					getStrategicBonusRefuseAttitudeThreshold();
			if(iAvailThem - kPlayer.getBonusImport(eBonus) > 0) 
				iRefusalThresh--;
			if(eAttitude <= iRefusalThresh) // </advc.036>
			{
				return DENIAL_ATTITUDE;
			}
		}
		if (GC.getBonusInfo(eBonus).getHappiness() > 0)
		{	// advc.036: Treat Ivory as non-crucial
			crucialStrategic = false;
			if (eAttitude <= GC.getLeaderHeadInfo(getPersonalityType()).getHappinessBonusRefuseAttitudeThreshold())
			{
				return DENIAL_ATTITUDE;
			}
		}

		if (GC.getBonusInfo(eBonus).getHealth() > 0)
		{
			crucialStrategic = false; // advc.036
			if (eAttitude <= GC.getLeaderHeadInfo(getPersonalityType()).getHealthBonusRefuseAttitudeThreshold())
			{
				return DENIAL_ATTITUDE;
			}
		}
	} // <advc.036>
	int iAvailUs = getNumAvailableBonuses(eBonus) - iChange;
	/*  Doesn't seem necessary after all; the valueForUs clauses rule out such
		trades anyway. By returning DENIAL_JOKING, crucial strategic resources
		could be excluded from the trade table (see changes in buildTradeTable),
		but it's perhaps confusing to show some non-surplus resources on the
		trade table and not others. */
	/*if(!isHuman() && crucialStrategic && iAvailUs <= 1 && (!bVassal ||
			iAvailThem > 0))
		return DENIAL_JOKING;
	*/// </advc.036>
	// <advc.133> See the previous 133-comment
	if(iChange < 0)
		return NO_DENIAL; // </advc.133>
	// <advc.036>
	if(!kPlayer.isHuman()) {
		/*  Allow human to import resources that they probably don't need,
			but make sure that the AI doesn't. */
		if(valueForThem < tradeValThresh)
			return DENIAL_NO_GAIN;
		if(isHuman())
			return NO_DENIAL;
	}
	int valueForUs = AI_corporationBonusVal(eBonus);
	if(iAvailUs - getBonusImport(eBonus) == 1)
		valueForUs = AI_bonusVal(eBonus, 0);
	/*  Don't want city counts to matter much b/c large civs are supposed to
		export to small (tall) ones. But selling to tiny civs often doesn't make
		sense; could get a much better deal from some larger buyer. */
	valueForUs += std::min(2 * valueForUs,
			getNumCities() / std::max(1, kPlayer.getNumCities()));
	// <advc.037>
	if(bVassal) {
		if(valueForUs >= 2 * (kPlayer.isHuman() ? tradeValThresh : valueForThem))
			return DENIAL_NO_GAIN;
		else return NO_DENIAL;
	} // </advc.037>
	// Replacing the JOKING clause further up
	if((kPlayer.isHuman()) ?
			// Don't presume value for human
			(valueForUs >= tradeValThresh +
			/*  bonusVal gives every city equal weight, but early on,
				it's mostly about the capital, which can grow fast. */
			std::min(2, getNumCities() / 2)) :
			(3 * valueForUs >= 2 * valueForThem ||
			valueForThem - valueForUs < tradeValThresh ||
			valueForUs > tradeValThresh + 2))
		return DENIAL_NO_GAIN;
	// </advc.036>
	return NO_DENIAL;
}

// K-Mod note: the way this function is currently used is that it actually represents
// the how much the current owner values _not giving the city to this player_.
//
// For example, if this player currently controls most of the city's culture,
// the value should be _lower_ rather than higher, so that the current owner
// is more likely to give up the city.
//
// Ideally the value of receiving the city and the cost of giving the city away would be
// separate things; but that's currently not how trades are made.
int CvPlayerAI::AI_cityTradeVal(CvCity* pCity) const
{
	FAssert(pCity->getOwnerINLINE() != getID());

	int iValue = 300;

	//iValue += (pCity->getPopulation() * 50);
	iValue += pCity->getPopulation()*20 + pCity->getHighestPopulation()*30; // K-Mod

	iValue += (pCity->getCultureLevel() * 200);
	iValue += (200 * pCity->getCultureLevel() * pCity->getCulture(getID())) /
			std::max(1, pCity->getCulture(pCity->getOwnerINLINE())
			/*  advc.001: I think karadoc's intention was to add at most
				200 * CultureLevel, namely when the city owner has 0 culture,
				but without the addition below, the increment would actually be
				200 * CultureLevel * this player's city culture, i.e. millions. */
			+ pCity->getCulture(getID()));

	//iValue += (((((pCity->getPopulation() * 50) + GC.getGameINLINE().getElapsedGameTurns() + 100) * 4) * pCity->plot()->calculateCulturePercent(pCity->getOwnerINLINE())) / 100);
	// K-Mod
	int iCityTurns = GC.getGameINLINE().getGameTurn() - (pCity->getGameTurnFounded() + pCity->getGameTurnAcquired())/2;
	iCityTurns = iCityTurns * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getVictoryDelayPercent() / 100;
	iValue += ((pCity->getPopulation()*20 + pCity->getHighestPopulation()*30 + iCityTurns*3/2 + 80) * 4 * (pCity->plot()->calculateCulturePercent(pCity->getOwnerINLINE())+10)) / 110;
	// K-Mod end

	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = plotCity(pCity->getX_INLINE(), pCity->getY_INLINE(), iI);

		if (pLoopPlot != NULL)
		{
			/* original bts code
			if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
			{
				iValue += (AI_bonusVal(pLoopPlot->getBonusType(getTeam()), 1, true) * 10);
			} */
			// K-Mod. Use average of our value for gaining the bonus, and their value for losing it.
			int iBonusValue = 0;

			if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
				iBonusValue += AI_bonusVal(pLoopPlot->getBonusType(getTeam()), 1, true);
			if (pLoopPlot->getBonusType(pCity->getTeam()) != NO_BONUS)
				iBonusValue += GET_PLAYER(pCity->getOwnerINLINE()).AI_bonusVal(pLoopPlot->getBonusType(pCity->getTeam()), -1, true);

			iBonusValue *= plotDistance(pLoopPlot, pCity->plot()) <= 1 ? 5 : 4;
			iValue += iBonusValue;
			// K-Mod end
		}
	}

	if (!(pCity->isEverOwned(getID())))
	{
		iValue *= 3;
		iValue /= 2;
	}

	/*  advc.104d: Moved the K-Mod code here into a new function
		cityWonderVal (with some tweaks). */
	iValue += cityWonderVal(pCity) * 10;
	// <advc.139>
	if(pCity->isEvacuating())
		iValue = ::round(0.5 * iValue); // </advc.139>
	return GET_TEAM(getTeam()).roundTradeVal(iValue); // advc.104k
}

// <advc.003> Refactored
DenialTypes CvPlayerAI::AI_cityTrade(CvCity* pCity, PlayerTypes ePlayer) const {

	FAssert(pCity->getOwnerINLINE() == getID());
	if(pCity->getLiberationPlayer(false) == ePlayer)
		return NO_DENIAL;
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	CvPlot const& p = *pCity->plot();
	if(!kPlayer.isHuman() && kPlayer.getTeam() != getTeam() && 
			!pCity->isEverOwned(ePlayer)) {
		// <advc.122> "We don't want to trade this" seems appropriate
		if(pCity->isAwfulSite(ePlayer))
			return DENIAL_UNKNOWN; // </advc.122>
		if(kPlayer.getNumCities() > 3 &&
			/*  advc.122 (comment): Now covered by CvPlayer::canTradeItem,
				but threshold there can be disabled through XML. */
				p.calculateCulturePercent(ePlayer) == 0) {
			if(kPlayer.AI_isFinancialTrouble())
				return DENIAL_UNKNOWN;
			CvCity* pNearestCity = GC.getMapINLINE().findCity(
					p.getX_INLINE(), p.getY_INLINE(), ePlayer, NO_TEAM, true, false,
					NO_TEAM, NO_DIRECTION, pCity);
			if(pNearestCity == NULL || plotDistance(p.getX_INLINE(), p.getY_INLINE(),
					pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE()) > 9)
				return DENIAL_UNKNOWN;
		}
	}
	if(isHuman() || atWar(getTeam(), kPlayer.getTeam()))
		return NO_DENIAL;
	if(kPlayer.getTeam() != getTeam())
		return DENIAL_NEVER;
	if(pCity->calculateCulturePercent(getID()) > 50)
		return DENIAL_TOO_MUCH;
	return NO_DENIAL;
} // </advc.003>


int CvPlayerAI::AI_stopTradingTradeVal(TeamTypes eTradeTeam, PlayerTypes ePlayer) const
{
	CvDeal* pLoopDeal;
	int iModifier;
	int iValue;
	int iLoop;

	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(GET_PLAYER(ePlayer).getTeam() != getTeam(), "shouldn't call this function on ourselves");
	FAssertMsg(eTradeTeam != getTeam(), "shouldn't call this function on ourselves"); // advc.003: The next two asserts had nonsensical messages (copy-paste error apparently)
	FAssertMsg(GET_TEAM(eTradeTeam).isAlive(), "GET_TEAM(eTradeTeam).isAlive is expected to be true");
	FAssertMsg(!atWar(eTradeTeam, GET_PLAYER(ePlayer).getTeam()), "ePlayer should be at peace with eTradeTeam");
	// <advc.130f>
	if(TEAMREF(ePlayer).isCapitulated() && TEAMREF(ePlayer).isVassal(getTeam()))
		return 0; // </advc.130f>
	iValue = (50 + (GC.getGameINLINE().getGameTurn() / 2));
	iValue += (GET_TEAM(eTradeTeam).getNumCities() * 5);

	iModifier = 0;

	switch (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).AI_getAttitude(eTradeTeam))
	{
	case ATTITUDE_FURIOUS:
		break;

	case ATTITUDE_ANNOYED:
		iModifier += 25;
		break;

	case ATTITUDE_CAUTIOUS:
		iModifier += 50;
		break;

	case ATTITUDE_PLEASED:
		iModifier += 100;
		break;

	case ATTITUDE_FRIENDLY:
		iModifier += 200;
		break;

	default:
		FAssert(false);
		break;
	} // <advc.130f>
	// towardUs - i.e. toward the civ who pays for the embargo
	AttitudeTypes towardUs = GET_PLAYER(ePlayer).AI_getAttitude(getID());
	if(towardUs >= ATTITUDE_PLEASED)
		iModifier -= 25;
	if(towardUs >= ATTITUDE_FRIENDLY)
		iModifier -= 25;
	CvTeam const& kTeam = TEAMREF(ePlayer); // The team that'll have to cancel OB
	if(kTeam.isOpenBorders(eTradeTeam)) {
		if(kTeam.getAtWarCount() > 0 && GET_PLAYER(ePlayer).isFocusWar() &&
				!kTeam.allWarsShared(getTeam(), false))
			iModifier += towardUs < ATTITUDE_FRIENDLY ? 150 : 100;
		else iModifier += 50;
	} // Lower bound was 0% in BtS, now 50%
	iValue *= std::max(50, iModifier + 100); // </advc.130f>
	iValue /= 100;
	// advc.130f: Factored into iModifier now
	/*if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isOpenBorders(eTradeTeam))
	{
		iValue *= 2;
	}*/

	if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isDefensivePact(eTradeTeam))
	{
		iValue *= 3;
	}

	for(pLoopDeal = GC.getGameINLINE().firstDeal(&iLoop); pLoopDeal != NULL; pLoopDeal = GC.getGameINLINE().nextDeal(&iLoop))
	{
		if (pLoopDeal->isCancelable(getID()) && !(pLoopDeal->isPeaceDeal()))
		{
			if (GET_PLAYER(pLoopDeal->getFirstPlayer()).getTeam() == GET_PLAYER(ePlayer).getTeam())
			{
				if (pLoopDeal->getLengthSecondTrades() > 0)
				{
					iValue += (GET_PLAYER(pLoopDeal->getFirstPlayer()).AI_dealVal(pLoopDeal->getSecondPlayer(), pLoopDeal->getSecondTrades()) * ((pLoopDeal->getLengthFirstTrades() == 0) ? 2 : 1));
				}
			}

			if (GET_PLAYER(pLoopDeal->getSecondPlayer()).getTeam() == GET_PLAYER(ePlayer).getTeam())
			{
				if (pLoopDeal->getLengthFirstTrades() > 0)
				{
					iValue += (GET_PLAYER(pLoopDeal->getSecondPlayer()).AI_dealVal(pLoopDeal->getFirstPlayer(), pLoopDeal->getFirstTrades()) * ((pLoopDeal->getLengthSecondTrades() == 0) ? 2 : 1));
				}
			}
		}
	}

	if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isVassal(getTeam()))
	{
		iValue /= 2;
	}
	return GET_TEAM(getTeam()).roundTradeVal(iValue); // advc.104k
}


DenialTypes CvPlayerAI::AI_stopTradingTrade(TeamTypes eTradeTeam, PlayerTypes ePlayer) const
{
	AttitudeTypes eAttitude;
	AttitudeTypes eAttitudeThem;
	int iI;

	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(GET_PLAYER(ePlayer).getTeam() != getTeam(), "shouldn't call this function on ourselves");
	FAssertMsg(eTradeTeam != getTeam(), "shouldn't call this function on ourselves");
	FAssertMsg(GET_TEAM(eTradeTeam).isAlive(), "GET_TEAM(eTradeTeam).isAlive is expected to be true");
	FAssertMsg(!atWar(getTeam(), eTradeTeam), "should be at peace with eTradeTeam");

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (GET_TEAM(getTeam()).isVassal(GET_PLAYER(ePlayer).getTeam()))
	{
		return NO_DENIAL;
	}

	if (GET_TEAM(getTeam()).isVassal(eTradeTeam))
	{
		return DENIAL_POWER_THEM;
	}
	// <advc.130f>
	// "It's out of our hands" b/c the vassal treaty can't be canceled
	if(GET_TEAM(eTradeTeam).isVassal(getTeam()))
		return DENIAL_POWER_THEM;
	if(GET_TEAM(getTeam()).isAtWar(TEAMID(ePlayer)))
		return NO_DENIAL; // </advc.130f>
	eAttitude = GET_TEAM(getTeam()).AI_getAttitude(GET_PLAYER(ePlayer).getTeam());

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getTeam())
			{
				if (eAttitude <= GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getStopTradingRefuseAttitudeThreshold())
				{
					return DENIAL_ATTITUDE;
				}
			}
		}
	}

	eAttitudeThem = GET_TEAM(getTeam()).AI_getAttitude(eTradeTeam);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getTeam())
			{
				if (eAttitudeThem > GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getStopTradingThemRefuseAttitudeThreshold())
				{
					return DENIAL_ATTITUDE_THEM;
				}
			}
		}
	}

	return NO_DENIAL;
}


/* advc.003: Clarification: This CvPlayer asks ePlayer to switch to eCivic.
   What price should this CvPlayer be willing to pay for that? */
int CvPlayerAI::AI_civicTradeVal(CivicTypes eCivic, PlayerTypes ePlayer) const
{	// <advc.003>
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	int iValue = 0;
	/*CivicTypes eBestCivic;
	int iValue;*/ // </advc.003>
	//int iValue = (2 * (getTotalPopulation() + GET_PLAYER(ePlayer).getTotalPopulation())); // XXX
	// advc.132: Replacing the above
	int anarchyCost = kPlayer.anarchyTradeVal(eCivic);

	CivicTypes eBestCivic = kPlayer.AI_bestCivic((CivicOptionTypes)(GC.getCivicInfo(eCivic).getCivicOptionType()));

	if (eBestCivic != NO_CIVIC)
	{
		if (eBestCivic != eCivic)
		{
			iValue += //std::max(0, // advc.132: Handle that below
					2 * (kPlayer.AI_civicValue(eBestCivic) - kPlayer.AI_civicValue(eCivic));
		}
	}
	/*  advc.132: ePlayer charges less than the anarchyCost if they were going to
		switch anyway. */
	iValue = std::max(iValue + anarchyCost, anarchyCost / 2);
	if(TEAMREF(ePlayer).isVassal(getTeam())) // advc.003
		iValue /= 2;
	// <advc.132>
	if(!isCivic(eCivic))
		iValue *= 2; // </advc.132>
	return GET_TEAM(getTeam()).roundTradeVal(iValue); // advc.104k
}


DenialTypes CvPlayerAI::AI_civicTrade(CivicTypes eCivic, PlayerTypes ePlayer) const
{
	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (GET_TEAM(getTeam()).isVassal(GET_PLAYER(ePlayer).getTeam()))
	{
		return NO_DENIAL;
	}

	if (atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		return NO_DENIAL;
	}

	if (GET_PLAYER(ePlayer).getTeam() == getTeam())
	{
		return NO_DENIAL;
	}

	if (getCivicPercentAnger(getCivics((CivicOptionTypes)(GC.getCivicInfo(eCivic).getCivicOptionType())),true) > getCivicPercentAnger(eCivic))
	{
		return DENIAL_ANGER_CIVIC;
	}

	CivicTypes eFavoriteCivic = (CivicTypes)GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic();
	if (eFavoriteCivic != NO_CIVIC)
	{
		if (isCivic(eFavoriteCivic) && (GC.getCivicInfo(eCivic).getCivicOptionType() == GC.getCivicInfo(eFavoriteCivic).getCivicOptionType()))
		{
			return DENIAL_FAVORITE_CIVIC;
		}
	}

	if (GC.getCivilizationInfo(getCivilizationType()).getCivilizationInitialCivics(GC.getCivicInfo(eCivic).getCivicOptionType()) == eCivic)
	{
		return DENIAL_JOKING;
	}

	if (AI_getAttitude(ePlayer) <= GC.getLeaderHeadInfo(getPersonalityType()).getAdoptCivicRefuseAttitudeThreshold())
	{
		return DENIAL_ATTITUDE;
	}

	return NO_DENIAL;
}


int CvPlayerAI::AI_religionTradeVal(ReligionTypes eReligion, PlayerTypes ePlayer) const
{	// <advc.003>
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	/*ReligionTypes eBestReligion;
	int iValue;*/ // </advc.003>
	//int iValue = (3 * (getTotalPopulation() + GET_PLAYER(ePlayer).getTotalPopulation())); // XXX
	// advc.132: Replacing the above
	int iValue = kPlayer.anarchyTradeVal();

	ReligionTypes eBestReligion = GET_PLAYER(ePlayer).AI_bestReligion();

	if (eBestReligion != NO_RELIGION)
	{
		if (eBestReligion != eReligion)
		{
			//iValue += std::max(0, (GET_PLAYER(ePlayer).AI_religionValue(eBestReligion) - GET_PLAYER(ePlayer).AI_religionValue(eReligion)));
			// K-Mod. AI_religionValue has arbitrary units.
			// We need to give it some kind of scale for it to be meaningful in this function.
			iValue *= 100 + std::min(100, 100 * GET_PLAYER(ePlayer).AI_religionValue(eBestReligion) / std::max(1, GET_PLAYER(ePlayer).AI_religionValue(eReligion)));
			iValue /= 100;
			// K-Mod end
		}
	}

	if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isVassal(getTeam()))
	{
		iValue /= 2;
	}
	// <advc.132>
	if(getStateReligion() != eReligion)
		iValue *= 2; // </advc.132>
	return GET_TEAM(getTeam()).roundTradeVal(iValue); // advc.104k
}


DenialTypes CvPlayerAI::AI_religionTrade(ReligionTypes eReligion, PlayerTypes ePlayer) const
{
	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (GET_TEAM(getTeam()).isVassal(GET_PLAYER(ePlayer).getTeam()))
	{
		return NO_DENIAL;
	}

	if (atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		return NO_DENIAL;
	}

	if (GET_PLAYER(ePlayer).getTeam() == getTeam())
	{
		return NO_DENIAL;
	}

	if (getStateReligion() != NO_RELIGION)
	{
		// advc.132: Original code moved into isMinorityReligion
		if(isMinorityReligion(eReligion))
		{
			return DENIAL_MINORITY_RELIGION;
		}
	}

	if (AI_getAttitude(ePlayer) <= GC.getLeaderHeadInfo(getPersonalityType()).getConvertReligionRefuseAttitudeThreshold())
	{
		return DENIAL_ATTITUDE;
	}

	return NO_DENIAL;
}


int CvPlayerAI::AI_unitImpassableCount(UnitTypes eUnit) const
{
	int iCount = 0;
	for (int iI = 0; iI < GC.getNumTerrainInfos(); iI++)
	{
		if (GC.getUnitInfo(eUnit).getTerrainImpassable(iI))
		{
			TechTypes eTech = (TechTypes)GC.getUnitInfo(eUnit).getTerrainPassableTech(iI);
			if (NO_TECH == eTech || !GET_TEAM(getTeam()).isHasTech(eTech))
			{
				iCount++;
			}
		}
	}

	for (int iI = 0; iI < GC.getNumFeatureInfos(); iI++)
	{
		if (GC.getUnitInfo(eUnit).getFeatureImpassable(iI))
		{
			TechTypes eTech = (TechTypes)GC.getUnitInfo(eUnit).getFeaturePassableTech(iI);
			if (NO_TECH == eTech || !GET_TEAM(getTeam()).isHasTech(eTech))
			{
				iCount++;
			}
		}
	}

	return iCount;
}

// K-Mod note: currently, unit promotions are considered in CvCityAI::AI_bestUnitAI rather than here.
int CvPlayerAI::AI_unitValue(UnitTypes eUnit, UnitAITypes eUnitAI, CvArea* pArea) const
{
	PROFILE_FUNC();

	bool bValid;
	int iNeededMissionaries;
	int iCombatValue;
	int iValue;
	int iTempValue;
	int iI;

	FAssertMsg(eUnit != NO_UNIT, "Unit is not assigned a valid value");
	FAssertMsg(eUnitAI != NO_UNITAI, "UnitAI is not assigned a valid value");
	CvUnitInfo const& u = GC.getUnitInfo(eUnit); // advc.003
	if (u.getDomainType() != AI_unitAIDomainType(eUnitAI))
	{
		if (eUnitAI != UNITAI_ICBM)//XXX
		{
			return 0;
		}
	}

	if (u.getNotUnitAIType(eUnitAI))
	{
		return 0;
	}

	bValid = u.getUnitAIType(eUnitAI);

	if (!bValid)
	{
		switch (eUnitAI)
		{
		case UNITAI_UNKNOWN:
			break;

		case UNITAI_ANIMAL:
			if (u.isAnimal())
			{
				bValid = true;
			}
			break;

		case UNITAI_SETTLE:
			if (u.isFound())
			{
				bValid = true;
			}
			break;

		case UNITAI_WORKER:
			for (iI = 0; iI < GC.getNumBuildInfos(); iI++)
			{
				if (u.getBuilds(iI))
				{
					bValid = true;
					break;
				}
			}
			break;

		case UNITAI_ATTACK:
			if (u.getCombat() > 0)
			{
				if (!u.isOnlyDefensive())
				{
					bValid = true;
				}
			}
			break;

		case UNITAI_ATTACK_CITY:
			if (u.getCombat() > 0)
			{
				if (!u.isOnlyDefensive())
				{
					if (!u.isNoCapture())
					{
						bValid = true;
					}
				}
			}
			break;

		case UNITAI_COLLATERAL:
			if (u.getCombat() > 0)
			{
				if (!u.isOnlyDefensive())
				{
					if (u.getCollateralDamage() > 0)
					{
						bValid = true;
					}
				}
			}
			break;

		case UNITAI_PILLAGE:
			if (u.getCombat() > 0)
			{
				if (!u.isOnlyDefensive())
				{
					bValid = true;
				}
			}
			break;

		case UNITAI_RESERVE:
			if (u.getCombat() > 0)
			{
				if (!u.isOnlyDefensive())
				{
						bValid = true;
					}
				}
			break;

		case UNITAI_COUNTER:
			if (u.getCombat() > 0)
			{
				if (!u.isOnlyDefensive())
				{
					if (u.getInterceptionProbability() > 0)
					{
						bValid = true;
					}

					if (!bValid)
					{
						for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
						{
							if (u.getUnitClassAttackModifier(iI) > 0)
							{
								bValid = true;
								break;
							}

							if (u.getTargetUnitClass(iI))
							{
								bValid = true;
								break;
							}
						}
					}

					if (!bValid)
					{
						for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
						{
							if (u.getUnitCombatModifier(iI) > 0)
							{
								bValid = true;
								break;
							}

							if (u.getTargetUnitCombat(iI))
							{
								bValid = true;
								break;
							}
						}
					}

					if (!bValid)
					{

						for (iI = 0; iI < GC.getNumUnitInfos(); iI++)
						{
							int iUnitClass = u.getUnitClassType();
							if (NO_UNITCLASS != iUnitClass && GC.getUnitInfo((UnitTypes)iI).getDefenderUnitClass(iUnitClass))
							{
								bValid = true;
								break;
							}

							int iUnitCombat = u.getUnitCombatType();
							if (NO_UNITCOMBAT !=  iUnitCombat && GC.getUnitInfo((UnitTypes)iI).getDefenderUnitCombat(iUnitCombat))
							{
								bValid = true;
								break;
							}
						}
					}
				}
			}
			break;

		case UNITAI_CITY_DEFENSE:
			if (u.getCombat() > 0)
			{
				if (!u.isNoDefensiveBonus())
				{
					if (u.getCityDefenseModifier() > 0)
					{
						bValid = true;
					}
				}
			}
			break;

		case UNITAI_CITY_COUNTER:
			if (u.getCombat() > 0)
			{
				if (!u.isNoDefensiveBonus())
				{
					if (u.getInterceptionProbability() > 0)
					{
						bValid = true;
					}

					if (!bValid)
					{
						for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
						{
							if (u.getUnitClassDefenseModifier(iI) > 0)
							{
								bValid = true;
								break;
							}
						}
					}

					if (!bValid)
					{
						for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
						{
							if (u.getUnitCombatModifier(iI) > 0)
							{
								bValid = true;
								break;
							}
						}
					}
				}
			}
			break;

		case UNITAI_CITY_SPECIAL:
			break;

		case UNITAI_PARADROP:
			if (u.getDropRange() > 0)
			{
				bValid = true;
			}
			break;

		case UNITAI_EXPLORE:
			// if (u.getCombat() > 0)
			if (!u.isNoRevealMap() && (u.getCombat() > 0 || u.isInvisible())) // K-Mod
			{
				if (0 == AI_unitImpassableCount(eUnit))
				{
					bValid = true;
				}
			}
			break;

		case UNITAI_MISSIONARY:
			if (pArea != NULL)
			{
				for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
				{
					if (u.getReligionSpreads((ReligionTypes)iI) > 0)
					{
						iNeededMissionaries = AI_neededMissionaries(pArea, ((ReligionTypes)iI));

						if (iNeededMissionaries > 0)
						{
							if (iNeededMissionaries > countReligionSpreadUnits(pArea, ((ReligionTypes)iI)))
							{
								bValid = true;
								break;
							}
						}
					}
				}

				for (iI = 0; iI < GC.getNumCorporationInfos(); iI++)
				{
					if (u.getCorporationSpreads((CorporationTypes)iI) > 0)
					{
						iNeededMissionaries = AI_neededExecutives(pArea, ((CorporationTypes)iI));

						if (iNeededMissionaries > 0)
						{
							if (iNeededMissionaries > countCorporationSpreadUnits(pArea, ((CorporationTypes)iI)))
							{
								bValid = true;
								break;
							}
						}
					}
				}
			}
			break;

		case UNITAI_PROPHET:
		case UNITAI_ARTIST:
		case UNITAI_SCIENTIST:
		case UNITAI_GENERAL:
		case UNITAI_MERCHANT:
		case UNITAI_ENGINEER:
		case UNITAI_GREAT_SPY: // K-Mod
		case UNITAI_SPY:
			break;

		case UNITAI_ICBM:
			if (u.getNukeRange() != -1)
			{
				bValid = true;
			}
			break;

		case UNITAI_WORKER_SEA:
			for (iI = 0; iI < GC.getNumBuildInfos(); iI++)
			{
				if (u.getBuilds(iI))
				{
					bValid = true;
					break;
				}
			}
			break;

		case UNITAI_ATTACK_SEA:
			if (u.getCombat() > 0)
			{
				if (u.getCargoSpace() == 0)
				{
					if (!(u.isInvisible()) && (u.getInvisibleType() == NO_INVISIBLE))
					{
						bValid = true;
					}
				}
			}
			break;

		case UNITAI_RESERVE_SEA:
			if (u.getCombat() > 0)
			{
				if (u.getCargoSpace() == 0)
				{
					bValid = true;
				}
			}
			break;

		case UNITAI_ESCORT_SEA:
			if (u.getCombat() > 0)
			{
				if (u.getCargoSpace() == 0)
				{
					if (0 == AI_unitImpassableCount(eUnit))
					{
						bValid = true;
					}
				}
			}
			break;

		case UNITAI_EXPLORE_SEA:
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/09/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
			if (u.getCargoSpace() <= 1 && !(u.isNoRevealMap()))
			{
				bValid = true;
			}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			break;

		case UNITAI_ASSAULT_SEA:
		case UNITAI_SETTLER_SEA:
			if (u.getCargoSpace() > 0)
			{
				if (u.getSpecialCargo() == NO_SPECIALUNIT)
				{
					bValid = true;
				}
			}
			break;

		case UNITAI_MISSIONARY_SEA:
		case UNITAI_SPY_SEA:
		case UNITAI_CARRIER_SEA:
		case UNITAI_MISSILE_CARRIER_SEA:
			if (u.getCargoSpace() > 0)
			{
				if (u.getSpecialCargo() != NO_SPECIALUNIT)
				{
					for (int i = 0; i < NUM_UNITAI_TYPES; ++i)
					{
						if (GC.getSpecialUnitInfo((SpecialUnitTypes)u.getSpecialCargo()).isCarrierUnitAIType(eUnitAI))
						{
							bValid = true;
							break;
						}
					}
				}
			}
			break;

		case UNITAI_PIRATE_SEA:
			if (u.isAlwaysHostile() && u.isHiddenNationality())
			{
				bValid = true;
			}
			break;

		case UNITAI_ATTACK_AIR:
			if (u.getAirCombat() > 0)
			{
				if (!u.isSuicide())
				{
					bValid = true;
				}
			}
			break;

		case UNITAI_DEFENSE_AIR:
			if (u.getInterceptionProbability() > 0)
			{
				bValid = true;
			}
			break;

		case UNITAI_CARRIER_AIR:
			if (u.getAirCombat() > 0)
			{
				if (u.getInterceptionProbability() > 0)
				{
					bValid = true;
				}
			}
			break;

		case UNITAI_MISSILE_AIR:
			if (u.getAirCombat() > 0)
			{
				if (u.isSuicide())
				{
					bValid = true;
				}
			}
			break;

		case UNITAI_ATTACK_CITY_LEMMING:
			bValid = false;
			break;

		default:
			FAssert(false);
			break;
		}
	}

	if (!bValid)
	{
		return 0;
	}

	iCombatValue = GC.getGameINLINE().AI_combatValue(eUnit);

	iValue = 1;

	iValue += u.getAIWeight();
	
	switch (eUnitAI)
	{
	case UNITAI_UNKNOWN:
	case UNITAI_ANIMAL:
		break;

	case UNITAI_SETTLE:
		iValue += (u.getMoves() * 100);
		break;

	case UNITAI_WORKER:
		for (iI = 0; iI < GC.getNumBuildInfos(); iI++)
		{
			if (u.getBuilds(iI))
			{
				iValue += 50;
			}
		}
		iValue += (u.getMoves() * 100);
		break;

	case UNITAI_ATTACK:
		
		iValue += iCombatValue;

		iValue += ((iCombatValue * u.getWithdrawalProbability()) / 100);
		if (u.getCombatLimit() < 100)
		{
			iValue -= (iCombatValue * (125 - u.getCombatLimit())) / 100;
		}

		// K-Mod
		if (u.getMoves() > 1)
		{
			// (the bts / bbai bonus was too high)
			int iFastMoverMultiplier = AI_isDoStrategy(AI_STRATEGY_FASTMOVERS) ? 3 : 1;
			iValue += iCombatValue * iFastMoverMultiplier * u.getMoves() / 8;
		}

		if (u.isNoCapture())
		{
			iValue -= iCombatValue * 30 / 100;
		}
		// K-Mod end
		
		break;

	case UNITAI_ATTACK_CITY:
	{
		PROFILE("AI_unitValue, UNITAI_ATTACK_CITY evaluation");
		// Effect army composition to have more collateral/bombard units
		
		iTempValue = ((iCombatValue * iCombatValue) / 75) + (iCombatValue / 2);
		iValue += iTempValue;
		if (u.isNoDefensiveBonus())
		{
			//iValue -= iTempValue / 2;
			iValue -= iTempValue / 3; // K-Mod
		}
		if (u.getDropRange() > 0)
		{
			//iValue -= iTempValue / 2;
			// disabled by K-Mod (how is drop range a disadvantage?)
		}
		if (u.isFirstStrikeImmune())
		{
			iValue += (iTempValue * 8) / 100;
		}
		iValue += ((iCombatValue * u.getCityAttackModifier()) / 75); // bbai (was 100).
		// iValue += ((iCombatValue * u.getCollateralDamage()) / 400); // (moved)
		iValue += ((iCombatValue * u.getWithdrawalProbability()) / 150); // K-Mod (was 100)
		// iValue += ((iCombatValue * u.getMoves() * iFastMoverMultiplier) / 4);
		// K-Mod
		if (u.getMoves() > 1)
		{
			int iFastMoverMultiplier = AI_isDoStrategy(AI_STRATEGY_FASTMOVERS) ? 4 : 1;
			iValue += iCombatValue * iFastMoverMultiplier * u.getMoves() / 10;
		}
		// K-Mod end


		/* if (!AI_isDoStrategy(AI_STRATEGY_AIR_BLITZ))
		{
			int iBombardValue = u.getBombardRate() * 8;
			//int iBombardValue = u.getBombardRate() * (u.isIgnoreBuildingDefense()?12 :8);
			if (iBombardValue > 0)
			{
				int iGoalTotalBombardRate = (AI_totalUnitAIs(UNITAI_ATTACK) + AI_totalUnitAIs(UNITAI_ATTACK_CITY)) * (getCurrentEra()+3) / 2;

				// Decrease the bombard target if we own every city in the area, or if we are fighting an overseas war
				if (pArea &&
					(pArea->getNumCities() == pArea->getCitiesPerPlayer(getID()) ||
					(pArea->getAreaAIType(getTeam()) != AREAAI_NEUTRAL && !AI_isLandWar(pArea))))
				{
					iGoalTotalBombardRate *= 2;
					iGoalTotalBombardRate /= 3;
				}

				// Note: this also counts UNITAI_COLLATERAL units, which only play defense
				int iTotalBombardRate = AI_calculateTotalBombard(DOMAIN_LAND);

				if (iTotalBombardRate <= iGoalTotalBombardRate)
				{
					iBombardValue *= (5*iGoalTotalBombardRate - 4*iTotalBombardRate);
					iBombardValue /= std::max(1, iGoalTotalBombardRate);
				}
				else
				{
					iBombardValue *= 3*iGoalTotalBombardRate+iTotalBombardRate;
					iBombardValue /= 4*std::max(1, iTotalBombardRate);
				}

				iValue += iBombardValue;
			}
		} */
		// K-Mod. Bombard rate and collateral damage are both very powerful, but they have diminishing returns wrt the number of such units.
		// Units with these traits tend to also have a 'combat limit' of less than 100%. It is bad to have an army with a high proportion of
		// combat-limited units, but it is fine to have some. So as an ad hoc mechanism for evaluating the tradeoff between collateral damage
		// & bombard vs. combat limit, I'm going to estimate the number of combat limited attack units we already have and use this to adjust
		// the value of this unit. - The value of bombard is particularly inflated, to make sure at least _some_ siege units are built.
		// Note: The original bts bombard evaluation has been deleted.
		// The commented code above is K-Mod code from before the more recent changes; kept for comparison.
		int iSiegeValue = 0;
		iSiegeValue += iCombatValue * u.getCollateralDamage() * (4+u.getCollateralDamageMaxUnits()) / 600;
		if (u.getBombardRate() > 0 && !AI_isDoStrategy(AI_STRATEGY_AIR_BLITZ))
		{
			int iBombardValue = (u.getBombardRate()+3) * 6;
			// Decrease the bombard value if we own every city in the area, or if we are fighting an overseas war
			if (pArea &&
				(pArea->getNumCities() == pArea->getCitiesPerPlayer(getID()) ||
				(pArea->getAreaAIType(getTeam()) != AREAAI_NEUTRAL && !AI_isLandWar(pArea))) &&
				AI_calculateTotalBombard(DOMAIN_SEA) > 0)
			{
				iBombardValue /= 2;
			}
			iSiegeValue += iBombardValue;
		}
		if (u.getCombatLimit() < 100)
		{
			PROFILE("AI_unitValue, UNITAI_ATTACK_CITY combat limit adjustment");
			// count the number of existing combat-limited units.
			int iLimitedUnits = 0;

			// Unfortunately, when counting units like this we can't distiguish between attack unit and collateral defensive units.
			// Most of the time, unitai_collateral units will be combat limited, and so we should subtract them from out limited unit tally.
			// But in some situations there are collateral damage units without combat limits (eg. Cho-Ko-Nu). When such units are in use,
			// we should not assume all unitai_collateral are limited. -- This whole business is an ugly kludge... I hope it works.
			int iNoLimitCollateral = 0;

			for (UnitClassTypes i = (UnitClassTypes)0; i < GC.getNumUnitClassInfos(); i=(UnitClassTypes)(i+1))
			{
				UnitTypes eLoopUnit = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(i);
				if (eLoopUnit == NO_UNIT)
					continue;
				const CvUnitInfo& kLoopInfo = GC.getUnitInfo(eLoopUnit);

				if (kLoopInfo.getDomainType() == DOMAIN_LAND && kLoopInfo.getCombat() > 0 && !kLoopInfo.isOnlyDefensive())
				{
					if (kLoopInfo.getCombatLimit() < 100)
						iLimitedUnits += getUnitClassCount(i);
					else if (kLoopInfo.getCollateralDamage() > 0)
						iNoLimitCollateral = getUnitClassCount(i);
				}
			}

			iLimitedUnits -= range(AI_totalUnitAIs(UNITAI_COLLATERAL) - iNoLimitCollateral / 2, 0, iLimitedUnits);
			FAssert(iLimitedUnits >= 0);
			int iAttackUnits = std::max(1, AI_totalUnitAIs(UNITAI_ATTACK) + AI_totalUnitAIs(UNITAI_ATTACK_CITY)); // floor value is just to avoid divide-by-zero
			FAssert(iAttackUnits >= iLimitedUnits || iLimitedUnits <= 3); // this is not strictly guarenteed, but I expect it to always be true under normal playing conditions.

			iValue *= std::max(1, iAttackUnits - iLimitedUnits);
			iValue /= iAttackUnits;

			iSiegeValue *= std::max(1, iAttackUnits - iLimitedUnits);
			iSiegeValue /= iAttackUnits + 2 * iLimitedUnits;
		}
		iValue += iSiegeValue;

		break;
	}

	case UNITAI_COLLATERAL:
		iValue += iCombatValue;
		//iValue += ((iCombatValue * u.getCollateralDamage()) / 50);
		iValue += iCombatValue * u.getCollateralDamage() * (1 + u.getCollateralDamageMaxUnits()) / 350; // K-Mod (max units is 6-8 in the current xml)
		iValue += ((iCombatValue * u.getMoves()) / 4);
		iValue += ((iCombatValue * u.getWithdrawalProbability()) / 25);
		iValue -= ((iCombatValue * u.getCityAttackModifier()) / 100);
		break;

	case UNITAI_PILLAGE:
		iValue += iCombatValue;
		//iValue += (iCombatValue * u.getMoves());
		iValue += iCombatValue * (u.getMoves()-1) / 2;
		break;

	case UNITAI_RESERVE:
		iValue += iCombatValue;
		// iValue -= ((iCombatValue * u.getCollateralDamage()) / 200); // disabled by K-Mod (collateral damage is never 'bad')
		for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
		{
//			int iCombatModifier = u.getUnitCombatModifier(iI);
//			iCombatModifier = (iCombatModifier < 40) ? iCombatModifier : (40 + (iCombatModifier - 40) / 2);
//			iValue += ((iCombatValue * iCombatModifier) / 100);
			iValue += ((iCombatValue * u.getUnitCombatModifier(iI) * AI_getUnitCombatWeight((UnitCombatTypes)iI)) / 12000);
		}
		//iValue += ((iCombatValue * u.getMoves()) / 2);
		// K-Mod
		if (u.getMoves() > 1)
		{
			iValue += iCombatValue * u.getMoves() / (u.isNoDefensiveBonus() ? 7 : 5);
		}

		iValue -= (u.isNoDefensiveBonus() ? iCombatValue / 4 : 0);

		for (UnitClassTypes i = (UnitClassTypes)0; i < GC.getNumUnitClassInfos(); i=(UnitClassTypes)(i+1))
		{
			iValue += iCombatValue * u.getFlankingStrikeUnitClass(i) * AI_getUnitClassWeight(i) / 20000; // (this is pretty small)
		}
		// K-Mod end
		break;

	case UNITAI_COUNTER:
		iValue += (iCombatValue / 2);
		for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
		{
			iValue += ((iCombatValue * u.getUnitClassAttackModifier(iI) * AI_getUnitClassWeight((UnitClassTypes)iI)) /
					10000); // k146: was 7500
			// <k146>
			iValue += ((iCombatValue * u.getUnitClassDefenseModifier(iI) *
					AI_getUnitClassWeight((UnitClassTypes)iI)) / 10000);
			// </k146>
			iValue += ((iCombatValue * (u.getTargetUnitClass(iI) ? 50 : 0)) / 100);
		}
		for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
		{
//			int iCombatModifier = u.getUnitCombatModifier(iI);
//			iCombatModifier = (iCombatModifier < 40) ? iCombatModifier : (40 + (iCombatModifier - 40) / 2);
//			iValue += ((iCombatValue * iCombatModifier) / 100);
			iValue += ((iCombatValue * u.getUnitCombatModifier(iI) * AI_getUnitCombatWeight((UnitCombatTypes)iI)) /
					7500); // k146: was 10000
			iValue += ((iCombatValue * (u.getTargetUnitCombat(iI) ? 50 : 0)) / 100);
		}
		for (iI = 0; iI < GC.getNumUnitInfos(); iI++)
		{
			int eUnitClass = u.getUnitClassType();
			if (NO_UNITCLASS != eUnitClass && GC.getUnitInfo((UnitTypes)iI).getDefenderUnitClass(eUnitClass))
			{
				iValue += (50 * iCombatValue) / 100;
			}

			int eUnitCombat = u.getUnitCombatType();
			if (NO_UNITCOMBAT != eUnitCombat && GC.getUnitInfo((UnitTypes)iI).getDefenderUnitCombat(eUnitCombat))
			{
				iValue += (50 * iCombatValue) / 100;
			}
		}

		//iValue += ((iCombatValue * u.getMoves()) / 2);
		// K-Mod
		if (u.getMoves() > 1)
		{
			iValue += iCombatValue * u.getMoves() /
					8; // k146: was 6
		}
		// K-Mod end

		iValue += ((iCombatValue * u.getWithdrawalProbability()) / 100);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/20/10                                jdog5000      */
/*                                                                                              */
/* War strategy AI                                                                              */
/************************************************************************************************/
		//iValue += (u.getInterceptionProbability() * 2);
		if( u.getInterceptionProbability() > 0 )
		{
			int iTempValue = u.getInterceptionProbability();

			iTempValue *= (25 + std::min(175, GET_TEAM(getTeam()).AI_getRivalAirPower()));
			iTempValue /= 100;

			iValue += iTempValue;
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/	
		break;

	case UNITAI_CITY_DEFENSE:
		iValue += ((iCombatValue * 2) / 3);
		iValue += ((iCombatValue * u.getCityDefenseModifier()) / 75);
		// K-Mod. Value for collateral immunity
		for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
		{
			if (u.getUnitCombatCollateralImmune(iI))
			{
				iValue += iCombatValue*30/100;
				break;
			}
		}
		// k146: Other bonuses
		iValue += iCombatValue * u.getHillsDefenseModifier() / 200;
		// K-Mod end
		break;

	case UNITAI_CITY_COUNTER:
	case UNITAI_CITY_SPECIAL:
	case UNITAI_PARADROP:
		iValue += (iCombatValue / 2);
		iValue += ((iCombatValue * u.getCityDefenseModifier()) / 100);
		for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
		{
			iValue += ((iCombatValue * u.getUnitClassAttackModifier(iI) * AI_getUnitClassWeight((UnitClassTypes)iI)) / 10000);
			iValue += ((iCombatValue * (u.getDefenderUnitClass(iI) ? 50 : 0)) / 100);
		}
		for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
		{
			iValue += ((iCombatValue * u.getUnitCombatModifier(iI) * AI_getUnitCombatWeight((UnitCombatTypes)iI)) / 10000);
			iValue += ((iCombatValue * (u.getDefenderUnitCombat(iI) ? 50 : 0)) / 100);
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/20/10                                jdog5000      */
/*                                                                                              */
/* War strategy AI                                                                              */
/************************************************************************************************/
		//iValue += (u.getInterceptionProbability() * 3);
		if( u.getInterceptionProbability() > 0 )
		{
			int iTempValue = u.getInterceptionProbability();

			iTempValue *= (25 + std::min(125, GET_TEAM(getTeam()).AI_getRivalAirPower()));
			iTempValue /= 50;

			iValue += iTempValue;
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/	
		break;

	case UNITAI_EXPLORE:
		iValue += (iCombatValue / 2);
		iValue += (u.getMoves() * 200);
		if (u.isNoBadGoodies())
		{
			iValue += 100;
		}
		// K-Mod note: spies are valid explorers, but the AI currently doesn't know to convert them to
		// UNITAI_SPY when the exploring is finished. Hence I won't yet give a value bonus for invisibility.
		break;

	case UNITAI_MISSIONARY:
		iValue += (u.getMoves() * 100);
		if (getStateReligion() != NO_RELIGION)
		{
			if (u.getReligionSpreads(getStateReligion()) > 0)
			{
				iValue += (5 * u.getReligionSpreads(getStateReligion())) / 2;
			}
		}
		for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
		{
			if (u.getReligionSpreads((ReligionTypes)iI) && hasHolyCity((ReligionTypes)iI))
			{
				iValue += 80;
				break;
			}
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/08/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
		if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		{
		    int iTempValue = 0;
		    for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
		    {
                if (u.getReligionSpreads((ReligionTypes)iI))
                {
                    iTempValue += (50 * getNumCities()) / (1 + getHasReligionCount((ReligionTypes)iI));
                }
		    }
		    iValue += iTempValue;		    
		}
		for (iI = 0; iI < GC.getNumCorporationInfos(); ++iI)
		{
			if (hasHeadquarters((CorporationTypes)iI))
			{
				if (u.getCorporationSpreads(iI) > 0)
				{
					iValue += (5 * u.getCorporationSpreads(iI)) / 2;
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       06/03/09                                jdog5000      */
/*                                                                                              */
/* Bugfix				                                                                         */
/************************************************************************************************/
					// Fix potential crash, probably would only happen in mods
					if( pArea != NULL )
					{
						iValue += 300 / std::max(1, pArea->countHasCorporation((CorporationTypes)iI, getID()));
					}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
				}
			}
		}
		break;

	case UNITAI_PROPHET:
	case UNITAI_ARTIST:
	case UNITAI_SCIENTIST:
	case UNITAI_GENERAL:
	case UNITAI_MERCHANT:
	case UNITAI_ENGINEER:
	case UNITAI_GREAT_SPY: // K-Mod
		break;

	case UNITAI_SPY:
		iValue += (u.getMoves() * 100);
		if (u.isSabotage())
		{
			iValue += 50;
		}
		if (u.isDestroy())
		{
			iValue += 50;
		}
		if (u.isCounterSpy())
		{
			iValue += 100;
		}
		break;

	case UNITAI_ICBM:
		if (u.getNukeRange() != -1)
		{
			/* original bts code
			iTempValue = 40 + (u.getNukeRange() * 40);
			if (u.getAirRange() == 0)
			{
				iValue += iTempValue;
			}
			else
			{
				iValue += (iTempValue * std::min(10, u.getAirRange())) / 10;
			}
			iValue += (iTempValue * (60 + u.getEvasionProbability())) / 100; */
			// K-Mod
			iTempValue = 100 + (u.getNukeRange() * 40);
			if (u.getAirRange() > 0)
			{
				iTempValue = iTempValue * std::min(8, u.getAirRange()) / 10;
			}
			// estimate the expected loss from being shot down.
			{
				bool bWar = pArea && pArea->getAreaAIType(getTeam()) != AREAAI_NEUTRAL;
				const CvTeamAI& kTeam = GET_TEAM(getTeam());
				int iInterceptTally = 0;
				int iPowerTally = 0;
				for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i = (TeamTypes)(i+1))
				{
					const CvTeam& kLoopTeam = GET_TEAM(i);
					if (kLoopTeam.isAlive() && kTeam.isHasMet(i) && (!bWar || kTeam.AI_getWarPlan(i) != NO_WARPLAN))
					{
						int iPower = kLoopTeam.getPower(false);
						iPowerTally += iPower;
						iInterceptTally += kLoopTeam.getNukeInterception() * iPower;
					}
				}
				if (iInterceptTally > 0)
				{
					FAssert(iPowerTally > 0);
					iTempValue -= iTempValue * (iInterceptTally / iPowerTally) * (100 - u.getEvasionProbability()) / 10000;
				}
			}
			iValue += iTempValue;
			// K-Mod end
		}
		break;

	case UNITAI_WORKER_SEA:
		for (iI = 0; iI < GC.getNumBuildInfos(); iI++)
		{
			if (u.getBuilds(iI))
			{
				iValue += 50;
			}
		}
		iValue += (u.getMoves() * 100);
		break;

	case UNITAI_ATTACK_SEA:
		iValue += iCombatValue;
		iValue += ((iCombatValue * u.getMoves()) / 2);
		iValue += (u.getBombardRate() * 4);
		break;

	case UNITAI_RESERVE_SEA:
		iValue += iCombatValue;
		iValue += (iCombatValue * u.getMoves());
		break;

	case UNITAI_ESCORT_SEA:
		iValue += iCombatValue;
		iValue += (iCombatValue * u.getMoves());
		iValue += (u.getInterceptionProbability() * 3);
		if (u.getNumSeeInvisibleTypes() > 0)
		{
			iValue += 200;
		}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       06/03/09                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
		// Boats which can't be seen don't play defense, don't make good escorts
		/*  <advc.028> They can defend now. Stats of subs and Stealth Destroyer
			aren't great for escorting, but there's other code to check this. */
		/*if (u.getInvisibleType() != NO_INVISIBLE)
		{
			iValue /= 2;
		}*/ // </advc.028>
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
		break;

	case UNITAI_EXPLORE_SEA:
		{
			int iExploreValue = 100;
			if (pArea != NULL)
			{
				if (pArea->isWater())
				{
					if (pArea->getUnitsPerPlayer(BARBARIAN_PLAYER) > 0)
					{
						iExploreValue += (2 * iCombatValue);
					}
				}
			}
			iValue += (u.getMoves() * iExploreValue);
			if (u.isAlwaysHostile())
			{
				iValue /= 2;
			}
			iValue /= (1 + AI_unitImpassableCount(eUnit));
		}
		break;

	case UNITAI_ASSAULT_SEA:
	case UNITAI_SETTLER_SEA:
	case UNITAI_MISSIONARY_SEA:
	case UNITAI_SPY_SEA:
		iValue += (iCombatValue / 2);
		iValue += (u.getMoves() * 200);
		iValue += (u.getCargoSpace() * 300);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/18/09                                jdog5000      */
/*                                                                                              */
/* City AI                                                                                      */
/************************************************************************************************/
		// Never build galley transports when ocean faring ones exist (issue mainly for Carracks)
		iValue /= (1 + AI_unitImpassableCount(eUnit));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		break;

	case UNITAI_CARRIER_SEA:
		iValue += iCombatValue;
		iValue += (u.getMoves() * 50);
		iValue += (u.getCargoSpace() * 400);
		break;

	case UNITAI_MISSILE_CARRIER_SEA:
		iValue += iCombatValue * u.getMoves();
		iValue += (25 + iCombatValue) * (3 + (u.getCargoSpace()));
		break;

	case UNITAI_PIRATE_SEA:
		iValue += iCombatValue;
		iValue += (iCombatValue * u.getMoves());
		break;

	case UNITAI_ATTACK_AIR:
		iValue += iCombatValue;
		iValue += (u.getCollateralDamage() * iCombatValue) / 100;
		iValue += 4 * u.getBombRate();
		iValue += (iCombatValue * (100 + 2 * u.getCollateralDamage()) * u.getAirRange()) / 100;
		break;

	case UNITAI_DEFENSE_AIR:
		iValue += iCombatValue;
		iValue += (u.getInterceptionProbability() * 3);
		iValue += (u.getAirRange() * iCombatValue);
		break;

	case UNITAI_CARRIER_AIR:
		iValue += (iCombatValue);
		iValue += (u.getInterceptionProbability() * 2);
		iValue += (u.getAirRange() * iCombatValue);
		break;

	case UNITAI_MISSILE_AIR:
		iValue += iCombatValue;
		iValue += 4 * u.getBombRate();
		iValue += u.getAirRange() * iCombatValue;
		break;

	case UNITAI_ATTACK_CITY_LEMMING:
		iValue += iCombatValue;
		break;

	default:
		FAssert(false);
		break;
	}
	
	if ((iCombatValue > 0) && ((eUnitAI == UNITAI_ATTACK) || (eUnitAI == UNITAI_ATTACK_CITY)))
	{
		if (pArea != NULL)
		{
			AreaAITypes eAreaAI = pArea->getAreaAIType(getTeam());
			if (eAreaAI == AREAAI_ASSAULT || eAreaAI == AREAAI_ASSAULT_MASSING)
			{
				for (int iI = 0; iI < GC.getNumPromotionInfos(); iI++)
				{
					if (u.getFreePromotions(iI))
					{
						if (GC.getPromotionInfo((PromotionTypes)iI).isAmphib())
						{
							iValue *= 133;
							iValue /= 100;
							break;
						}
					}
				}
			}
		}
	}

	return std::max(0, iValue);
}


int CvPlayerAI::AI_totalUnitAIs(UnitAITypes eUnitAI) const
{
	return (AI_getNumTrainAIUnits(eUnitAI) + AI_getNumAIUnits(eUnitAI));
}


int CvPlayerAI::AI_totalAreaUnitAIs(CvArea* pArea, UnitAITypes eUnitAI) const
{
	return (pArea->getNumTrainAIUnits(getID(), eUnitAI) + pArea->getNumAIUnits(getID(), eUnitAI));
}


int CvPlayerAI::AI_totalWaterAreaUnitAIs(CvArea* pArea, UnitAITypes eUnitAI) const
{
	CvCity* pLoopCity;
	int iCount;
	int iLoop;
	int iI;

	iCount = AI_totalAreaUnitAIs(pArea, eUnitAI);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
			{
				if (pLoopCity->waterArea() == pArea)
				{
					iCount += pLoopCity->plot()->plotCount(PUF_isUnitAIType, eUnitAI, -1, getID());

					if (pLoopCity->getOwnerINLINE() == getID())
					{
						iCount += pLoopCity->getNumTrainUnitAI(eUnitAI);
					}
				}
			}
		}
	}


	return iCount;
}


int CvPlayerAI::AI_countCargoSpace(UnitAITypes eUnitAI) const
{
	CvUnit* pLoopUnit;
	int iCount;
	int iLoop;

	iCount = 0;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		if (pLoopUnit->AI_getUnitAIType() == eUnitAI)
		{
			iCount += pLoopUnit->cargoSpace();
		}
	}

	return iCount;
}


int CvPlayerAI::AI_neededExplorers(CvArea* pArea) const
{	// <advc.003b> Body moved into AI_neededExplorersBulk
	if(pArea == NULL) {
		FAssert(pArea != NULL);
		return 0;
	}
	std::map<int,int>::const_iterator r = neededExplorersByArea.find(pArea->getID());
	if(r == neededExplorersByArea.end())
		return 0; // Small area w/o cached value; no problem.
	return r->second;
}

void CvPlayerAI::AI_updateNeededExplorers() {

	neededExplorersByArea.clear();
	CvMap& m = GC.getMapINLINE(); int dummy;
	for(CvArea* a = m.firstArea(&dummy); a != NULL; a = m.nextArea(&dummy)) {
		int id = a->getID();
		// That threshold should also be OK for land areas
		if(a->getNumTiles() > GC.getLAKE_MAX_AREA_SIZE())
			neededExplorersByArea[a->getID()] = AI_neededExplorersBulk(a);
		// Nothing stored for small areas; AI_neededExplorers will return 0.
	}
}
// Body cut and pasted from AI_neededExplorers
int CvPlayerAI::AI_neededExplorersBulk(CvArea const* pArea) const {

	// advc.305: Be sure not to build Workboats for exploration
	if(isBarbarian()) return 0;

	int iNeeded = 0;

	if (pArea->isWater())
	{
		iNeeded = std::min(iNeeded + (pArea->getNumUnrevealedTiles(getTeam()) / 400), std::min(2, ((getNumCities() / 2) + 1)));
	}
	else
	{
		iNeeded = std::min(iNeeded + (pArea->getNumUnrevealedTiles(getTeam()) / //150
				// 040: Exploration becomes ever cheaper as the game progresses
				std::max(150 - 20 * getCurrentEra(), 50)
				), std::min(3, ((getNumCities() / 3) + 2)));
	}

	if (0 == iNeeded)
	{
		if ((GC.getGameINLINE().countCivTeamsAlive() - 1) > GET_TEAM(getTeam()).getHasMetCivCount(true))
		{
			if (pArea->isWater())
			{
				if (GC.getMap().findBiggestArea(true) == pArea)
				{
					iNeeded++;
				}
			}
			else
			{
			    if (getCapitalCity() != NULL && pArea->getID() == getCapitalCity()->getArea())
			    {
                    for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
                    {
                        CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
                        if (kPlayer.isAlive() && kPlayer.getTeam() != getTeam())
                        {
                            if (!GET_TEAM(getTeam()).isHasMet(kPlayer.getTeam()))
                            {
                                if (pArea->getCitiesPerPlayer(kPlayer.getID()) > 0)
                                {
                                    iNeeded++;
                                    break;
                                }
                            }
                        }
                    }
			    }
			}
		}
	}
	return iNeeded;
} // </advc.003b>


int CvPlayerAI::AI_neededWorkers(CvArea* pArea) const
{
	/*  advc.003b: This function is called each time a Worker moves or a city
		chooses production, and countUnimprovedBonuses loops over all plots.
		In the single profiler run I did, the computing time was completely
		negligible though. */
	PROFILE_FUNC();

	CvCity* pLoopCity;
	int iCount;
	int iLoop;

	iCount = countUnimprovedBonuses(pArea) * 2;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (pLoopCity->getArea() == pArea->getID())
		{
			iCount += pLoopCity->AI_getWorkersNeeded() * 3;
		}
	}
	
	if (iCount == 0)
	{
		return 0;
	}

	// K-Mod. Some additional workers if for 'growth' flavour AIs who are still growing...
	if (
		// AI_getFlavorValue(FLAVOR_GROWTH) > 0 && // advc.113: All civs need those extra workers
		AI_isPrimaryArea(pArea))
	{
		int iDummy;
		int iExtraCities = std::min(GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities()*4/3 - getNumCities(), AI_getNumAreaCitySites(pArea->getID(), iDummy));
		iExtraCities = range(iExtraCities, 0, getNumCities()*2/3);
		/* <advc.113> Used to be 3 indiscriminately. Factoring in Growth flavor
		   here now, but only to a minor effect b/c Growth is mostly about
		   tall cities, which doesn't actually fit well with additional Workers. */
		iCount += iExtraCities * (AI_getFlavorValue(FLAVOR_GROWTH) > 0 ? 2 : 1);
		// </advc.113>
	}
	// K-Mod end

	if (getBestRoute() != NO_ROUTE)
	{
		iCount += pArea->getCitiesPerPlayer(getID()) / 2;
	}


	iCount += 1;
	iCount /= 3;

	/* <advc.113> The AI built too few workers. More specifically, the calculation
	   of needed workers focuses on immediate tasks, and, hence, lags behind
	   the true workload when new tasks come up, e.g. when founding a city.
	   The 25% increase (new XML parameter) leads to extra workers being trained
	   ahead of time. (Probably too many workers in the later eras this way, but
	   that's a minor concern.)
	   Then, the std::min bounds: I made the first one stricter,
	   i.e. at most 2.5 workers per city instead of 3, and removed
	   the second one. Current population should have no bearing on workers. */
	iCount = (iCount * (100 + GC.getDefineINT("WORKER-RESERVE_PERCENT"))) / 100;

	iCount = std::min(iCount, (5 * pArea->getCitiesPerPlayer(getID())) / 2);
	//iCount = std::min(iCount, (1 + getTotalPopulation()) / 2); // </advc.113>

	return std::max(1, iCount);

}


int CvPlayerAI::AI_neededMissionaries(CvArea* pArea, ReligionTypes eReligion) const
{
    PROFILE_FUNC();
	int iCount;
	bool bHoly, bState, bHolyState;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/08/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
	bool bCultureVictory = AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	bHoly = hasHolyCity(eReligion);
	bState = (getStateReligion() == eReligion);
	bHolyState = ((getStateReligion() != NO_RELIGION) && hasHolyCity(getStateReligion()));

	iCount = 0;

    //internal spread.
    if (bCultureVictory || bState || bHoly)
    {
        iCount = std::max(iCount, (pArea->getCitiesPerPlayer(getID()) - pArea->countHasReligion(eReligion, getID())));
        if (iCount > 0)
        {
            if (!bCultureVictory)
	{
                iCount = std::max(1, iCount / (bHoly ? 2 : 4));
            }
            return iCount;
        }
	}

    //external spread.
    if ((bHoly && bState) || (bHoly && !bHolyState && (getStateReligion() != NO_RELIGION)))
    {
        iCount += ((pArea->getNumCities() * 2) - (pArea->countHasReligion(eReligion) * 3));
        iCount /= 8;

        iCount = std::max(0, iCount);

		if (AI_isPrimaryArea(pArea))
		{
			iCount++;
		}
    }


	return iCount;
}


int CvPlayerAI::AI_neededExecutives(CvArea* pArea, CorporationTypes eCorporation) const
{
	if (!hasHeadquarters(eCorporation))
	{
		return 0;
	}

	int iCount = ((pArea->getCitiesPerPlayer(getID()) - pArea->countHasCorporation(eCorporation, getID())) * 2);
	iCount += (pArea->getNumCities() - pArea->countHasCorporation(eCorporation));

	iCount /= 3;

	if (AI_isPrimaryArea(pArea))
	{
		++iCount;
	}

	return iCount;
}

// K-Mod. This function is used to replace the old (broken) "unit cost percentage" calculation used by the AI
int CvPlayerAI::AI_unitCostPerMil() const
{
	// original "cost percentage" = calculateUnitCost() * 100 / std::max(1, calculatePreInflatedCosts());
	// If iUnitCostPercentage is calculated as above, decreasing maintenance will actually decrease the max units.
	// If a builds a courthouse or switches to state property, it would then think it needs to get rid of units!
	// It makes no sense, and civs with a surplus of cash still won't want to build units. So lets try it another way...
	int iUnitCost = calculateUnitCost() * std::max(0, calculateInflationRate() + 100) / 100;
	if (iUnitCost <= getNumCities()/2) // cf with the final line
		return 0;

	int iTotalRaw = calculateTotalYield(YIELD_COMMERCE);

	int iFunds = iTotalRaw * AI_averageCommerceMultiplier(COMMERCE_GOLD) / 100;
	iFunds += getGoldPerTurn() - calculateInflatedCosts();
	iFunds += getCommerceRate(COMMERCE_GOLD) - iTotalRaw * AI_averageCommerceMultiplier(COMMERCE_GOLD) * getCommercePercent(COMMERCE_GOLD) / 10000;
	return std::max(0, iUnitCost-getNumCities()/2) * 1000 / std::max(1, iFunds); // # cities is there to offset early-game distortion.
}

// This function gives an approximate / recommended maximum on our unit spending. Note though that it isn't a hard cap.
// we might go as high has 20 point above the "maximum"; and of course, the maximum might later go down.
// So this should only be used as a guide.
int CvPlayerAI::AI_maxUnitCostPerMil(CvArea* pArea, int iBuildProb) const
{
	if (isBarbarian())
		return 500;

	if (GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE))
		return 20; // ??

	if (iBuildProb < 0)
		iBuildProb = GC.getLeaderHeadInfo(getPersonalityType()).getBuildUnitProb() + 6; // a rough estimate.

	bool bTotalWar = GET_TEAM(getTeam()).getWarPlanCount(WARPLAN_TOTAL, true);
	bool bAggressiveAI = GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI);

	int iMaxUnitSpending = (bAggressiveAI ? 30 : 20) + iBuildProb*4/3;

	if (AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST4))
	{
		iMaxUnitSpending += 30;
	}
	else if (AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3 | AI_VICTORY_DOMINATION3))
	{
		iMaxUnitSpending += 20;
	}
	else if (AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST1))
	{
		iMaxUnitSpending += 10;
	}

	if (!bTotalWar)
	{
		iMaxUnitSpending += AI_isDoStrategy(AI_STRATEGY_ALERT1) ? 15 + iBuildProb / 3 : 0;
		iMaxUnitSpending += AI_isDoStrategy(AI_STRATEGY_ALERT2) ? 15 + iBuildProb / 3 : 0;
		// note. the boost from alert1 + alert2 matches the boost from total war. (see below).
	}

	if (AI_isDoStrategy(AI_STRATEGY_FINAL_WAR))
	{
		iMaxUnitSpending += 300;
	}
	else
	{
		iMaxUnitSpending += bTotalWar ? 30 + iBuildProb*2/3 : 0;
		if (pArea)
		{
			switch (pArea->getAreaAIType(getTeam()))
			{
			case AREAAI_OFFENSIVE:
				iMaxUnitSpending += 40;
				break;

			case AREAAI_DEFENSIVE:
				iMaxUnitSpending += 75;
				break;

			case AREAAI_MASSING:
				iMaxUnitSpending += 75;
				break;

			case AREAAI_ASSAULT:
				iMaxUnitSpending += 40;
				break;

			case AREAAI_ASSAULT_MASSING:
				iMaxUnitSpending += 70;
				break;

			case AREAAI_ASSAULT_ASSIST:
				iMaxUnitSpending += 35;
				break;

			case AREAAI_NEUTRAL:
				// think of 'dagger' as being prep for total war.
				FAssert(!bTotalWar);
				iMaxUnitSpending += AI_isDoStrategy(AI_STRATEGY_DAGGER) ? 20 + iBuildProb*2/3 : 0;
				break;
			default:
				FAssert(false);
			}
		}
		else
		{
			if(isFocusWar()) // advc.105
			//if (GET_TEAM(getTeam()).getAnyWarPlanCount(true))
				iMaxUnitSpending += 55;
			else
			{
				FAssert(!bTotalWar);
				iMaxUnitSpending += AI_isDoStrategy(AI_STRATEGY_DAGGER) ? 20 + iBuildProb*2/3 : 0;
			}
		}
	}
	return iMaxUnitSpending;
}

// When nukes are enabled, this function returns a percentage factor of how keen this player is to build nukes.
// The starting value is around 100, which corresponds to quite a low tendency to build nukes.
int CvPlayerAI::AI_nukeWeight() const
{
	PROFILE_FUNC();

	if (!GC.getGameINLINE().isNukesValid() || GC.getGameINLINE().isNoNukes())
		return 0;

	const CvTeamAI& kTeam = GET_TEAM(getTeam());
	// <advc.143b>
	if(kTeam.isCapitulated())
		return 0; // </advc.143b>
	int iNukeWeight = 100;

	// Increase the weight based on how many nukes the world has made & used so far.
	int iHistory = 2*GC.getGameINLINE().getNukesExploded() + GC.getGameINLINE().countTotalNukeUnits() - GC.getLeaderHeadInfo(getPersonalityType()).getBasePeaceWeight();
	iHistory *= 35; // 5% for each nuke, 10% for each exploded
	iHistory /= std::max(1,
			GC.getGameINLINE().getRecommendedPlayers() // advc.137
			//GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getDefaultPlayers()
			);
	iHistory = std::min(iHistory, 300);
	if (iHistory > 0)
	{
		iHistory *= 90 + GC.getLeaderHeadInfo(getPersonalityType()).getConquestVictoryWeight();
		iHistory /= 100;
	}

	iNukeWeight += iHistory;

	// increase the weight if we were the team who enabled nukes. (look for projects only. buildings can get stuffed.)
	for (ProjectTypes i = (ProjectTypes)0; i < GC.getNumProjectInfos(); i = (ProjectTypes)(i+1))
	{
		if (GC.getProjectInfo(i).isAllowsNukes() && kTeam.getProjectCount(i) > 0)
		{
			iNukeWeight += std::max(0, 150 - iHistory/2);
			break;
		}
	}

	// increase the weight for total war, or for the home-stretch to victory, or for losing wars.
	if (kTeam.AI_isAnyMemberDoVictoryStrategyLevel4())
	{
		iNukeWeight = iNukeWeight*3/2;
	}
	else if (kTeam.getWarPlanCount(WARPLAN_TOTAL, true) > 0 || kTeam.AI_getWarSuccessRating() < -20)
	{
		iNukeWeight = iNukeWeight*4/3;
	}

	return iNukeWeight;
}

bool CvPlayerAI::AI_isLandWar(CvArea* pArea) const
{
	switch(pArea->getAreaAIType(getTeam()))
	{
	case AREAAI_OFFENSIVE:
	case AREAAI_MASSING:
	case AREAAI_DEFENSIVE:
		return true;
	default:
		return false;
	}
}
// K-Mod end

// <advc.105>
bool CvPlayerAI::isFocusWar(CvArea* ap) const {

	CvTeamAI const& t = GET_TEAM(getTeam());
	CvCity* capital = getCapitalCity();
	if(capital == NULL || capital->area() == NULL)
		return false;
	CvArea const& a = (ap == NULL ? *capital->area() : *ap);
	/*  Chosen wars (ongoing or in preparation) are always worth focusing on;
		others only when on the defensive. */
	return (t.isAnyChosenWar() || a.getAreaAIType(getTeam()) == AREAAI_DEFENSIVE ||
			AI_isDoStrategy(AI_STRATEGY_ALERT2));
} // </advc.105>

int CvPlayerAI::AI_adjacentPotentialAttackers(CvPlot* pPlot, bool bTestCanMove) const
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pLoopPlot;
	int iCount;
	int iI;

	iCount = 0;
	CvArea const& plotArea = *pPlot->area(); // advc.030
	for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		pLoopPlot = plotDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), ((DirectionTypes)iI));

		if (pLoopPlot != NULL)
		{	// <advc.030>
			CvArea const& fromArea = *pLoopPlot->area();
			if(plotArea.canBeEntered(fromArea)) // Replacing:
			//if (pLoopPlot->area() == pPlot->area()) // </advc.030>
			{
				pUnitNode = pLoopPlot->headUnitNode();

				while (pUnitNode != NULL)
				{
					pLoopUnit = ::getUnit(pUnitNode->m_data);
					pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

					if (pLoopUnit->getOwnerINLINE() == getID())
					{	// advc.030: Replacing the line below
						if(plotArea.canBeEntered(fromArea, pLoopUnit))
						//if (pLoopUnit->getDomainType() == ((pPlot->isWater()) ? DOMAIN_SEA : DOMAIN_LAND))
						{
							if (pLoopUnit->canAttack())
							{
								if (!bTestCanMove || pLoopUnit->canMove())
								{
									if (!(pLoopUnit->AI_isCityAIType()))
									{
										iCount++;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return iCount;
}


int CvPlayerAI::AI_totalMissionAIs(MissionAITypes eMissionAI, CvSelectionGroup* pSkipSelectionGroup) const
{
	PROFILE_FUNC();

	CvSelectionGroup* pLoopSelectionGroup;
	int iCount;
	int iLoop;

	iCount = 0;

	for(pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup != pSkipSelectionGroup)
		{
			if (pLoopSelectionGroup->AI_getMissionAIType() == eMissionAI)
			{
				iCount += pLoopSelectionGroup->getNumUnits();
			}
		}
	}

	return iCount;
}

int CvPlayerAI::AI_missionaryValue(CvArea* pArea, ReligionTypes eReligion, PlayerTypes* peBestPlayer) const
{
	CvTeam& kTeam = GET_TEAM(getTeam());
	CvGame& kGame = GC.getGame();
	
	int iSpreadInternalValue = 100;
	int iSpreadExternalValue = 0;

	// Obvious copy & paste bug
	if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1))
	{
		iSpreadInternalValue += 500;
		if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2))
		{
			iSpreadInternalValue += 1500;
			if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3))
			{
				iSpreadInternalValue += 3000;
			}
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/03/09                                jdog5000      */
/*                                                                                              */
/* Missionary AI                                                                                */
/************************************************************************************************/
	// In free religion, treat all religions like state religions
	bool bStateReligion = (getStateReligion() == eReligion);
	
	if (!isStateReligion())
	{
		// Free religion
		iSpreadInternalValue += 500;
		bStateReligion = true;
	}
	else if(bStateReligion)
	{
		iSpreadInternalValue += 1000;
	}
	else
	{
		iSpreadInternalValue += (500 * getHasReligionCount(eReligion)) / std::max(1, getNumCities());
	}
	
	int iGoldValue = 0;
	if (kTeam.hasHolyCity(eReligion))
	{
		iSpreadInternalValue += bStateReligion ? 1000 : 300;
		iSpreadExternalValue += bStateReligion ? 1000 : 150;
		if (kTeam.hasShrine(eReligion))
		{
			iSpreadInternalValue += bStateReligion ? 500 : 300;
			iSpreadExternalValue += bStateReligion ? 300 : 200;
			int iGoldMultiplier = kGame.getHolyCity(eReligion)->getTotalCommerceRateModifier(COMMERCE_GOLD);
			iGoldValue = 6 * iGoldMultiplier;
			// K-Mod. todo: use GC.getReligionInfo(eReligion).getGlobalReligionCommerce((CommerceTypes)iJ)
		}
	}
	
	int iOurCitiesHave = 0;
	int iOurCitiesCount = 0;
	
	if (NULL == pArea)
	{
		iOurCitiesHave = kTeam.getHasReligionCount(eReligion);
		iOurCitiesCount = kTeam.getNumCities();
	}
	else
	{
		iOurCitiesHave = pArea->countHasReligion(eReligion, getID()) + countReligionSpreadUnits(pArea, eReligion,true);
		iOurCitiesCount = pArea->getCitiesPerPlayer(getID());
	}
	
	if (iOurCitiesHave < iOurCitiesCount)
	{
		iSpreadInternalValue *= 30 + ((100 * (iOurCitiesCount - iOurCitiesHave))/ iOurCitiesCount);
		iSpreadInternalValue /= 100;
		iSpreadInternalValue += iGoldValue;
	}
	else
	{
		iSpreadInternalValue = 0;
	}
	
	if (iSpreadExternalValue > 0)
	{
		int iBestPlayer = NO_PLAYER;
		int iBestValue = 0;
		for (int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
		{
			if (iPlayer != getID())
			{
				CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
				if (kLoopPlayer.isAlive() && kLoopPlayer.getTeam() != getTeam() && kLoopPlayer.getNumCities() > 0)
				{
					if (GET_TEAM(kLoopPlayer.getTeam()).isOpenBorders(getTeam()) ||
							// advc.001i:
							GET_TEAM(getTeam()).isFriendlyTerritory(kLoopPlayer.getTeam()))
					{
						int iCitiesCount = 0;
						int iCitiesHave = 0;
						int iMultiplier = AI_isDoStrategy(AI_STRATEGY_MISSIONARY) ? 60 : 25;
						if (!kLoopPlayer.isNoNonStateReligionSpread() || (kLoopPlayer.getStateReligion() == eReligion))
						{
							if (NULL == pArea)
							{
								iCitiesCount += 1 + (kLoopPlayer.getNumCities() * 75) / 100;
								iCitiesHave += std::min(iCitiesCount, kLoopPlayer.getHasReligionCount(eReligion));
							}
							else
							{
								int iPlayerSpreadPercent = (100 * kLoopPlayer.getHasReligionCount(eReligion)) / kLoopPlayer.getNumCities();
								iCitiesCount += pArea->getCitiesPerPlayer((PlayerTypes)iPlayer);
								iCitiesHave += std::min(iCitiesCount, (iCitiesCount * iPlayerSpreadPercent) / 75);
							}
						}
						
						if (kLoopPlayer.getStateReligion() == NO_RELIGION)
						{
							// Paganism counts as a state religion civic, that's what's caught below
							if (kLoopPlayer.getStateReligionCount() > 0)
							{
								int iTotalReligions = kLoopPlayer.countTotalHasReligion();
								iMultiplier += 100 * std::max(0, kLoopPlayer.getNumCities() - iTotalReligions);
								iMultiplier += (iTotalReligions == 0) ? 100 : 0;
							}
						}

						int iValue = (iMultiplier * iSpreadExternalValue * (iCitiesCount - iCitiesHave)) / std::max(1, iCitiesCount);
						iValue /= 100;
						iValue += iGoldValue;

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							iBestPlayer = iPlayer;
						}
					}
				}
			}
		}

		if (iBestValue > iSpreadInternalValue)
		{
			if (NULL != peBestPlayer)
			{
				*peBestPlayer = (PlayerTypes)iBestPlayer;
			}
			return iBestValue;
		}

	}

	if (NULL != peBestPlayer)
	{
		*peBestPlayer = getID();
	}

	return iSpreadInternalValue;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
}

// K-Mod note: the original BtS code for this was totally inconsistant with the calculation in AI_missionaryValue
// -- which is bad news since the results are compared directly.
// I've rewritten most of this function so that it is more sane and more compariable to the missionary value.
// The original code is deleted.
// Currently, the return value has units of roughly (and somewhat arbitrarily) 1000 * commerce per turn.
int CvPlayerAI::AI_executiveValue(CvArea* pArea, CorporationTypes eCorporation, PlayerTypes* peBestPlayer, bool bSpreadOnly) const
{
	PROFILE_FUNC();
	CvGame& kGame = GC.getGame();
	CvCorporationInfo& kCorp = GC.getCorporationInfo(eCorporation);

	int iSpreadExternalValue = 0;
	int iExistingExecs = 0;
	if (pArea)
	{
		iExistingExecs += bSpreadOnly ? 0 : std::max(0, countCorporationSpreadUnits(pArea, eCorporation, true) - 1);
		// K-Mod note, -1 just so that we can build a spare, perhaps to airlift to another area. ("-1" execs is ok.)
		// bSpreadOnly means that we are not calculating the value of building a new exec. Just the value of spreading.
	}

	if (GET_TEAM(getTeam()).hasHeadquarters(eCorporation))
	{
		int iHqValue = 0;
		CvCity* kHqCity = kGame.getHeadquarters(eCorporation);
		for (CommerceTypes i = (CommerceTypes)0; i < NUM_COMMERCE_TYPES; i=(CommerceTypes)(i+1))
		{
			if (kCorp.getHeadquarterCommerce(i))
				iHqValue += kCorp.getHeadquarterCommerce(i) * kHqCity->getTotalCommerceRateModifier(i) * AI_commerceWeight(i)/100;
		}

		iSpreadExternalValue += iHqValue;
	}

	int iBestPlayer = NO_PLAYER;
	int iBestValue = 0;
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
	{
		const CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		int iNumCities = pArea ? pArea->getCitiesPerPlayer((PlayerTypes)iPlayer) : kLoopPlayer.getNumCities();
		if (kLoopPlayer.isAlive() && iNumCities > 0)
		{
			if (GET_TEAM(getTeam()).isFriendlyTerritory(kLoopPlayer.getTeam()) || GET_TEAM(kLoopPlayer.getTeam()).isOpenBorders(getTeam()))
			{
				if (!kLoopPlayer.isNoCorporations() && (iPlayer == getID() || !kLoopPlayer.isNoForeignCorporations()))
				{
					int iAttitudeWeight;

					if (kLoopPlayer.getTeam() == getTeam())
						iAttitudeWeight = 100;
					else if (GET_TEAM(kLoopPlayer.getTeam()).isVassal(getTeam()))
						iAttitudeWeight = 50;
					else
						iAttitudeWeight = AI_getAttitudeWeight((PlayerTypes)iPlayer) - (isHuman() ? 100 : 75);
					// note: this is to discourage automated human units from spreading to the AI, not AI to human.

					// a rough check to save us some time.
					if (iAttitudeWeight <= 0 && iSpreadExternalValue <= 0)
						continue;

					int iCitiesHave = kLoopPlayer.countCorporations(eCorporation, pArea);

					if (iCitiesHave + iExistingExecs >= iNumCities)
						continue;

					int iCorpValue = kLoopPlayer.AI_corporationValue(eCorporation);
					int iValue = iSpreadExternalValue;
					iValue += (iCorpValue * iAttitudeWeight)/100;
					if (iValue > 0 && iCorpValue > 0 && iCitiesHave == 0 && iExistingExecs == 0)
					{
						// if the player will spread the corp themselves, then that's good for us.
						if (iAttitudeWeight >= 50)
						{
							// estimate spread to 2/3 of total cities.
							iValue *= (2*kLoopPlayer.getNumCities()+1)/3;
						}
						else
						{
							// estimate spread to 1/4 of total cities, rounded up.
							iValue *= (kLoopPlayer.getNumCities()+3)/4;
						}
					}
					if (iValue > iBestValue)
					{
						for (int iCorp = 0; iCorp < GC.getNumCorporationInfos(); iCorp++)
						{
							if (kGame.isCorporationFounded((CorporationTypes)iCorp) && kGame.isCompetingCorporation(eCorporation, (CorporationTypes)iCorp))
							{
								int iExtra = kLoopPlayer.countCorporations((CorporationTypes)iCorp, pArea);

								if (iExtra > 0 && iCorpValue > kLoopPlayer.AI_corporationValue((CorporationTypes)iCorp)*4/3) // (ideally, hq should be considered too)
									iExtra /= 2;

								iCitiesHave += iExtra;
							}
						}

						if (iCitiesHave + iExistingExecs >= iNumCities)
							continue;

						iBestValue = iValue;
						iBestPlayer = iPlayer;
					}
				}
			}
		}
	}

	if (NULL != peBestPlayer)
	{
		*peBestPlayer = (PlayerTypes)iBestPlayer;
	}
	// I'm putting in a fudge-factor of 10 just to bring the value up to scale with AI_missionaryValue.
	// This isn't something that i'm happy about, but it's easier than rewriting AI_missionaryValue.
	return 10 * iBestValue;
}

// This function has been completely rewriten for K-Mod. The original code has been deleted. (it was junk)
// Returns approximately 100 x gpt value of the corporation, for one city.
int CvPlayerAI::AI_corporationValue(CorporationTypes eCorporation, const CvCity* pCity) const
{
	const CvTeamAI& kTeam = GET_TEAM(getTeam());
	CvCorporationInfo& kCorp = GC.getCorporationInfo(eCorporation);
	int iValue = 0;
	int iMaintenance = 0;

	int iBonuses = 0;
	for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
	{
		BonusTypes eBonus = (BonusTypes)kCorp.getPrereqBonus(i);
		if (NO_BONUS != eBonus)
		{
			if (pCity == NULL)
				iBonuses += countOwnedBonuses(eBonus);
			else
				iBonuses += pCity->getNumBonuses(eBonus);
			// maybe use getNumAvailableBonuses ?
			if (!kTeam.isHasTech((TechTypes)GC.getBonusInfo(eBonus).getTechReveal()) && !kTeam.isForceRevealedBonus(eBonus))
			{
				iBonuses++; // expect that we'll get one of each unrevealed resource
			}
		}
	}
	if (!GC.getGameINLINE().isCorporationFounded(eCorporation))
	{
		// expect that we'll be able to trade for at least 1 of the bonuses.
		iBonuses++;
	}

	for (int iI = (CommerceTypes)0; iI < NUM_COMMERCE_TYPES; ++iI)
	{
		int iTempValue = kCorp.getCommerceProduced((CommerceTypes)iI) * iBonuses;

		if (iTempValue != 0)
		{
			if (pCity == NULL)
				iTempValue *= AI_averageCommerceMultiplier((CommerceTypes)iI);
			else
				iTempValue *= pCity->getTotalCommerceRateModifier((CommerceTypes)iI);
			// I'd feel more comfortable if they named it "multiplier" when it included the base 100%.
			iTempValue /= 100;

			iTempValue *= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getCorporationMaintenancePercent();
			iTempValue /= 100;

			iTempValue *= AI_commerceWeight((CommerceTypes)iI, pCity);
			iTempValue /= 100;

			iValue += iTempValue;
		}

		iMaintenance += 100 * kCorp.getHeadquarterCommerce(iI);
	}

	for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
	{
		int iTempValue = kCorp.getYieldProduced((YieldTypes)iI) * iBonuses;
		if (iTempValue != 0)
		{
			if (pCity == NULL)
				iTempValue *= AI_averageYieldMultiplier((YieldTypes)iI);
			else
				iTempValue *= pCity->getBaseYieldRateModifier((YieldTypes)iI);

			iTempValue /= 100;
			iTempValue *= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getCorporationMaintenancePercent();
			iTempValue /= 100;

			iTempValue *= AI_yieldWeight((YieldTypes)iI);
			iTempValue /= 100;

			iValue += iTempValue;
		}
	}

	// maintenance cost
	int iTempValue = kCorp.getMaintenance() * iBonuses;
	iTempValue *= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getCorporationMaintenancePercent();
	iTempValue /= 100;
	iTempValue += iMaintenance;
	// Inflation, population, and maintenance modifiers... lets just approximate them like this:
	iTempValue *= 2;
	iTempValue /= 3;

	iTempValue *= AI_commerceWeight(COMMERCE_GOLD);
	iTempValue /= 100;

	iValue -= iTempValue;

	// bonus produced by the corp
	BonusTypes eBonusProduced = (BonusTypes)kCorp.getBonusProduced();
	if (eBonusProduced != NO_BONUS)
	{
		//int iBonuses = getNumAvailableBonuses((BonusTypes)kCorp.getBonusProduced());
		int iBonuses = pCity ? pCity->getNumBonuses(eBonusProduced) : countOwnedBonuses(eBonusProduced);
		// pretend we have 1 bonus if it is not yet revealed. (so that we don't overvalue the corp before the resource gets revealed)
		iBonuses += !kTeam.isHasTech((TechTypes)GC.getBonusInfo(eBonusProduced).getTechReveal()) ? 1 : 0;
		iValue += AI_baseBonusVal(eBonusProduced) * 25 / (1 + 2 * iBonuses * (iBonuses+3));
	}

	return iValue;
}

int CvPlayerAI::AI_areaMissionAIs(CvArea* pArea, MissionAITypes eMissionAI, CvSelectionGroup* pSkipSelectionGroup) const
{
	PROFILE_FUNC();

	CvSelectionGroup* pLoopSelectionGroup;
	CvPlot* pMissionPlot;
	int iCount;
	int iLoop;

	iCount = 0;

	for(pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup != pSkipSelectionGroup)
		{
			if (pLoopSelectionGroup->AI_getMissionAIType() == eMissionAI)
			{
				pMissionPlot = pLoopSelectionGroup->AI_getMissionAIPlot();

				if (pMissionPlot != NULL)
				{
					if (pMissionPlot->area() == pArea)
					{
						iCount += pLoopSelectionGroup->getNumUnits();
					}
				}
			}
		}
	}

	return iCount;
}


int CvPlayerAI::AI_plotTargetMissionAIs(CvPlot* pPlot, MissionAITypes eMissionAI, CvSelectionGroup* pSkipSelectionGroup, int iRange) const
{
	int iClosestTargetRange;
	return AI_plotTargetMissionAIs(pPlot, &eMissionAI, 1, iClosestTargetRange, pSkipSelectionGroup, iRange);
}

int CvPlayerAI::AI_plotTargetMissionAIs(CvPlot* pPlot, MissionAITypes eMissionAI, int& iClosestTargetRange, CvSelectionGroup* pSkipSelectionGroup, int iRange) const
{
	return AI_plotTargetMissionAIs(pPlot, &eMissionAI, 1, iClosestTargetRange, pSkipSelectionGroup, iRange);
}

int CvPlayerAI::AI_plotTargetMissionAIs(CvPlot* pPlot, MissionAITypes* aeMissionAI, int iMissionAICount, int& iClosestTargetRange, CvSelectionGroup* pSkipSelectionGroup, int iRange) const
{
	PROFILE_FUNC();

	int iCount = 0;
	iClosestTargetRange = MAX_INT;

	int iLoop;
	for(CvSelectionGroup* pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup != pSkipSelectionGroup)
		{
			CvPlot* pMissionPlot = pLoopSelectionGroup->AI_getMissionAIPlot();

			if (pMissionPlot != NULL)
			{
				MissionAITypes eGroupMissionAI = pLoopSelectionGroup->AI_getMissionAIType();
				int iDistance = stepDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pMissionPlot->getX_INLINE(), pMissionPlot->getY_INLINE());

				if (iDistance <= iRange)
				{
					for (int iMissionAIIndex = 0; iMissionAIIndex < iMissionAICount; iMissionAIIndex++)
					{
						if (eGroupMissionAI == aeMissionAI[iMissionAIIndex] || aeMissionAI[iMissionAIIndex] == NO_MISSIONAI)
						{
							iCount += pLoopSelectionGroup->getNumUnits();

							if (iDistance < iClosestTargetRange)
							{
								iClosestTargetRange = iDistance;
							}
						}
					}
				}
			}
		}
	}

	return iCount;
}

// K-Mod

// Total defensive strength of units that can move iRange steps to reach pDefencePlot
int CvPlayerAI::AI_localDefenceStrength(const CvPlot* pDefencePlot, TeamTypes eDefenceTeam, DomainTypes eDomainType, int iRange, bool bAtTarget, bool bCheckMoves, bool bNoCache) const
{
	PROFILE_FUNC();

	int	iTotal = 0;

	FAssert(bAtTarget || !bCheckMoves); // it doesn't make much sense to check moves if the defenders are meant to stay put.

	for (int iDX = -iRange; iDX <= iRange; iDX++)
	{
		for (int iDY = -iRange; iDY <= iRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(pDefencePlot->getX_INLINE(), pDefencePlot->getY_INLINE(), iDX, iDY);
			if (pLoopPlot == NULL || !pLoopPlot->isVisible(getTeam(), false))
				continue;

			CLLNode<IDInfo>* pUnitNode = pLoopPlot->headUnitNode();

			int iPlotTotal = 0;
			while (pUnitNode != NULL)
			{
				CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

				if (pLoopUnit->getTeam() == eDefenceTeam
					|| (eDefenceTeam != NO_TEAM && GET_TEAM(pLoopUnit->getTeam()).isVassal(eDefenceTeam))
					|| (eDefenceTeam == NO_TEAM && isPotentialEnemy(getTeam(), pLoopUnit->getTeam())))
				{
					if (eDomainType == NO_DOMAIN || (pLoopUnit->getDomainType() == eDomainType))
					{
						if (bCheckMoves)
						{
							// unfortunately, we can't use the global pathfinder here
							// - because the calling function might be waiting to use some pathfinding results
							// So this check will have to be really rough. :(
							int iMoves = pLoopUnit->baseMoves();
							iMoves += pLoopPlot->isValidRoute(pLoopUnit) ? 1 : 0;
							int iDistance = std::max(std::abs(iDX), std::abs(iDY));
							if (iDistance > iMoves)
								continue; // can't make it. (maybe?)
						}
						int iUnitStr = pLoopUnit->currEffectiveStr(bAtTarget ? pDefencePlot : pLoopPlot, NULL);
						// first strikes are not counted in currEffectiveStr.
						// actually, the value of first strikes is non-trivial to calculate... but we should do /something/ to take them into account.
						iUnitStr *= 100 + 4 * pLoopUnit->firstStrikes() + 2 * pLoopUnit->chanceFirstStrikes();
						iUnitStr /= 100;
						// note. Most other parts of the code use 5% per first strike, but I figure we should go lower because this unit may get clobbered by collateral damage before fighting.
						iPlotTotal += iUnitStr;
					}
				}
			}
			if (!bNoCache && !isHuman() && eDefenceTeam == NO_TEAM && eDomainType == DOMAIN_LAND && !bCheckMoves && (!bAtTarget || pLoopPlot == pDefencePlot))
			{
				// while since we're here, we might as well update our memory.
				// (human players don't track strength memory)
				GET_TEAM(getTeam()).AI_setStrengthMemory(pLoopPlot, iPlotTotal);
				FAssert(isTurnActive());
			}
			iTotal += iPlotTotal;
		}
	}

	return iTotal;
}

// Total attack strength of units that can move iRange steps to reach pAttackPlot
int CvPlayerAI::AI_localAttackStrength(const CvPlot* pTargetPlot, TeamTypes eAttackTeam, DomainTypes eDomainType, int iRange, bool bUseTarget, bool bCheckMoves, bool bCheckCanAttack,
		int* attackerCount) const // advc.139
{
	PROFILE_FUNC();

	int iBaseCollateral = bUseTarget
		? estimateCollateralWeight(pTargetPlot, getTeam())
		: estimateCollateralWeight(0, getTeam());

	int	iTotal = 0;

	for (int iDX = -iRange; iDX <= iRange; iDX++)
	{
		for (int iDY = -iRange; iDY <= iRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(pTargetPlot->getX_INLINE(), pTargetPlot->getY_INLINE(), iDX, iDY);
			if (pLoopPlot == NULL || !pLoopPlot->isVisible(getTeam(), false))
				continue;

			CLLNode<IDInfo>* pUnitNode = pLoopPlot->headUnitNode();

			while (pUnitNode != NULL)
			{
				CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

				if (pLoopUnit->getTeam() == eAttackTeam	|| (eAttackTeam == NO_TEAM && atWar(getTeam(), pLoopUnit->getTeam())))
				{
					if (eDomainType == NO_DOMAIN || (pLoopUnit->getDomainType() == eDomainType))
					{
						if (!pLoopUnit->canAttack()) // bCheckCanAttack means something else.
							continue;

						if (bCheckMoves)
						{
							// unfortunately, we can't use the global pathfinder here
							// - because the calling function might be waiting to use some pathfinding results
							// So this check will have to be really rough. :(
							int iMoves = pLoopUnit->baseMoves();
							iMoves += pLoopPlot->isValidRoute(pLoopUnit) ? 1 : 0;
							int iDistance = std::max(std::abs(iDX), std::abs(iDY));
							if (iDistance > iMoves)
								continue; // can't make it. (maybe?)
						}
						if (bCheckCanAttack)
						{
							if (!pLoopUnit->canMove() || (pLoopUnit->isMadeAttack() && !pLoopUnit->isBlitz())
								|| (bUseTarget && pLoopUnit->combatLimit() < 100 && pLoopPlot->isWater() && !pTargetPlot->isWater()))
							{
								continue; // can't attack
							}
						}

						int iUnitStr = pLoopUnit->currEffectiveStr(bUseTarget ? pTargetPlot : NULL, bUseTarget ? pLoopUnit : NULL);
						// first strikes. (see comments in AI_localDefenceStrength... although, maybe 5% would be better here?)
						iUnitStr *= 100 + 4 * pLoopUnit->firstStrikes() + 2 * pLoopUnit->chanceFirstStrikes();
						iUnitStr /= 100;
						//
						if (pLoopUnit->collateralDamage() > 0)
						{
							int iPossibleTargets = std::min(bUseTarget ? pTargetPlot->getNumVisibleEnemyDefenders(pLoopUnit) - 1 : INT_MAX, pLoopUnit->collateralDamageMaxUnits());

							if (iPossibleTargets > 0)
							{
								// collateral damage is not trivial to calculate. This estimate is pretty rough. (cf with calculation in AI_sumStrength)
								// (Note: collateralDamage() and iBaseCollateral both include factors of 100.)
								iUnitStr += pLoopUnit->baseCombatStr() * iBaseCollateral * pLoopUnit->collateralDamage() * iPossibleTargets / 10000;
							}
						}
						iTotal += iUnitStr;
						// <advc.139>
						if(attackerCount != NULL && pLoopUnit->getDamage() < 30) {
							CvUnitInfo& u = GC.getUnitInfo(pLoopUnit->getUnitType());
							// 80 is Cannon; don't count the early siege units
							if(!u.isOnlyDefensive() && u.getCombatLimit() >= 80)
								(*attackerCount)++;
						} // </advc.139>
					}
				}
			}
		}
	}

	return iTotal;
}
// K-Mod end

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/03/10                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/

// K-Mod. I've changed this function to calculation the attack power of our groups, rather than just the number of units.
// I've also changed it to use a separate instance of the path finder, so that it doesn't clear the reset the existing path data.

//int CvPlayerAI::AI_cityTargetUnitsByPath(CvCity* pCity, CvSelectionGroup* pSkipSelectionGroup, int iMaxPathTurns) const
int CvPlayerAI::AI_cityTargetStrengthByPath(CvCity* pCity, CvSelectionGroup* pSkipSelectionGroup, int iMaxPathTurns) const
{
	PROFILE_FUNC();

	//int iCount = 0;
	int iTotalStrength = 0;
	KmodPathFinder temp_finder;

	int iLoop;
	//int iPathTurns;
	for(CvSelectionGroup* pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup != pSkipSelectionGroup && pLoopSelectionGroup->getNumUnits() > 0)
		{
			FAssert(pLoopSelectionGroup->plot() != NULL); // this use to be part of the above condition. I've turned it into an assert. K-Mod.
			CvPlot* pMissionPlot = pLoopSelectionGroup->AI_getMissionAIPlot();

			if (pMissionPlot != NULL)
			{
				int iDistance = stepDistance(pCity->getX_INLINE(), pCity->getY_INLINE(), pMissionPlot->getX_INLINE(), pMissionPlot->getY_INLINE());

				if (iDistance <= 1 && pLoopSelectionGroup->canFight())
				{
					/* original bbai code
					if( pLoopSelectionGroup->generatePath(pLoopSelectionGroup->plot(), pMissionPlot, 0, true, &iPathTurns, iMaxPathTurns) )
					{
						if( !(pLoopSelectionGroup->canAllMove()) )
						{
							iPathTurns++;
						}

						if( iPathTurns <= iMaxPathTurns )
						{
							iCount += pLoopSelectionGroup->getNumUnits();
						}
					} */
					temp_finder.SetSettings(CvPathSettings(pLoopSelectionGroup, 0, iMaxPathTurns, GC.getMOVE_DENOMINATOR()));
					if (temp_finder.GeneratePath(pMissionPlot))
					{
						iTotalStrength += pLoopSelectionGroup->AI_sumStrength(pCity->plot(), DOMAIN_LAND);
					}
				}
			}
		}
	}

	//return iCount;
	return iTotalStrength;
}

int CvPlayerAI::AI_unitTargetMissionAIs(CvUnit* pUnit, MissionAITypes eMissionAI, CvSelectionGroup* pSkipSelectionGroup) const
{
	return AI_unitTargetMissionAIs(pUnit, &eMissionAI, 1, pSkipSelectionGroup, -1);
}

int CvPlayerAI::AI_unitTargetMissionAIs(CvUnit* pUnit, MissionAITypes* aeMissionAI, int iMissionAICount, CvSelectionGroup* pSkipSelectionGroup) const
{
	return AI_unitTargetMissionAIs(pUnit, aeMissionAI, iMissionAICount, pSkipSelectionGroup, -1);
}

int CvPlayerAI::AI_unitTargetMissionAIs(CvUnit* pUnit, MissionAITypes* aeMissionAI, int iMissionAICount, CvSelectionGroup* pSkipSelectionGroup, int iMaxPathTurns) const
{
	PROFILE_FUNC();

	CvSelectionGroup* pLoopSelectionGroup;
	int iCount;
	int iLoop;

	iCount = 0;

	for(pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup != pSkipSelectionGroup)
		{
			if (pLoopSelectionGroup->AI_getMissionAIUnit() == pUnit)
			{
				MissionAITypes eGroupMissionAI = pLoopSelectionGroup->AI_getMissionAIType();
				int iPathTurns = MAX_INT;

				if( iMaxPathTurns >= 0 && (pUnit->plot() != NULL) && (pLoopSelectionGroup->plot() != NULL))
				{
					pLoopSelectionGroup->generatePath(pLoopSelectionGroup->plot(), pUnit->plot(), 0, false, &iPathTurns);
					if( !(pLoopSelectionGroup->canAllMove()) )
					{
						iPathTurns++;
					}
				}

				if ((iMaxPathTurns == -1) || (iPathTurns <= iMaxPathTurns))
				{
					for (int iMissionAIIndex = 0; iMissionAIIndex < iMissionAICount; iMissionAIIndex++)
					{
						if (eGroupMissionAI == aeMissionAI[iMissionAIIndex] || NO_MISSIONAI == aeMissionAI[iMissionAIIndex])
						{
							iCount += pLoopSelectionGroup->getNumUnits();
						}
					}
				}
			}
		}
	}

	return iCount;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/		

int CvPlayerAI::AI_enemyTargetMissionAIs(MissionAITypes eMissionAI, CvSelectionGroup* pSkipSelectionGroup) const
{
	return AI_enemyTargetMissionAIs(&eMissionAI, 1, pSkipSelectionGroup);
}

int CvPlayerAI::AI_enemyTargetMissionAIs(MissionAITypes* aeMissionAI, int iMissionAICount, CvSelectionGroup* pSkipSelectionGroup) const
{
	PROFILE_FUNC();

	int iCount = 0;

	int iLoop;
	for(CvSelectionGroup* pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup != pSkipSelectionGroup)
		{
			CvPlot* pMissionPlot = pLoopSelectionGroup->AI_getMissionAIPlot();

			if (NULL != pMissionPlot && pMissionPlot->isOwned())
			{
				MissionAITypes eGroupMissionAI = pLoopSelectionGroup->AI_getMissionAIType();
				for (int iMissionAIIndex = 0; iMissionAIIndex < iMissionAICount; iMissionAIIndex++)
				{
					if (eGroupMissionAI == aeMissionAI[iMissionAIIndex] || NO_MISSIONAI == aeMissionAI[iMissionAIIndex])
					{
						if (GET_TEAM(getTeam()).AI_isChosenWar(pMissionPlot->getTeam()))
						{
							iCount += pLoopSelectionGroup->getNumUnits();
							iCount += pLoopSelectionGroup->getCargo();
						}
					}
				}
			}
		}
	}

	return iCount;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/19/10                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
int CvPlayerAI::AI_enemyTargetMissions(TeamTypes eTargetTeam, CvSelectionGroup* pSkipSelectionGroup) const
{
	PROFILE_FUNC();

	int iCount = 0;

	int iLoop;
	for(CvSelectionGroup* pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup != pSkipSelectionGroup)
		{
			CvPlot* pMissionPlot = pLoopSelectionGroup->AI_getMissionAIPlot();

			if( pMissionPlot == NULL )
			{
				pMissionPlot = pLoopSelectionGroup->plot();
			}

			if (NULL != pMissionPlot )
			{
				if( pMissionPlot->isOwned() && pMissionPlot->getTeam() == eTargetTeam )
				{
					if (atWar(getTeam(),pMissionPlot->getTeam()) || pLoopSelectionGroup->AI_isDeclareWar(pMissionPlot))
					{
						iCount += pLoopSelectionGroup->getNumUnits();
						iCount += pLoopSelectionGroup->getCargo();
					}
				}
			}
		}
	}

	return iCount;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

int CvPlayerAI::AI_wakePlotTargetMissionAIs(CvPlot* pPlot, MissionAITypes eMissionAI, CvSelectionGroup* pSkipSelectionGroup) const
{
	PROFILE_FUNC();

	FAssert(pPlot != NULL);
	
	int iCount = 0;

	int iLoop;
	for(CvSelectionGroup* pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup != pSkipSelectionGroup)
		{
			MissionAITypes eGroupMissionAI = pLoopSelectionGroup->AI_getMissionAIType();
			if (eMissionAI == NO_MISSIONAI || eMissionAI == eGroupMissionAI)
			{
				CvPlot* pMissionPlot = pLoopSelectionGroup->AI_getMissionAIPlot();
				if (pMissionPlot != NULL && pMissionPlot == pPlot)
				{
					iCount += pLoopSelectionGroup->getNumUnits();
					pLoopSelectionGroup->setActivityType(ACTIVITY_AWAKE);
				}
			}
		}
	}

	return iCount;
}

// K-Mod, I've added piBestValue, and tidied up some stuff.
CivicTypes CvPlayerAI::AI_bestCivic(CivicOptionTypes eCivicOption, int* piBestValue) const
{
	CivicTypes eBestCivic = NO_CIVIC;
	int iBestValue = MIN_INT;

	for (int iI = 0; iI < GC.getNumCivicInfos(); iI++)
	{
		if (GC.getCivicInfo((CivicTypes)iI).getCivicOptionType() == eCivicOption)
		{
			if (canDoCivics((CivicTypes)iI))
			{
				int iValue = AI_civicValue((CivicTypes)iI);

				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					eBestCivic = (CivicTypes)iI;
				}
			}
		}
	}

	if (piBestValue)
		*piBestValue = iBestValue;

	return eBestCivic;
}

// The bulk of this function has been rewriten for K-Mod. (some original code deleted, some edited by BBAI)
// Note: the value is roughly in units of commerce per turn.
// Also, this function could probably be made a bit more accurate and perhaps even faster if it calculated effects on a city-by-city basis,
// rather than averaging effects across all cities. (certainly this would work better for happiness modifiers.)
int CvPlayerAI::AI_civicValue(CivicTypes eCivic) const
{
	PROFILE_FUNC();

	const CvTeamAI& kTeam = GET_TEAM(getTeam()); // K-Mod
	const CvGame& kGame = GC.getGameINLINE(); // K-Mod

	int iCities = getNumCities();

	FAssertMsg(eCivic < GC.getNumCivicInfos(), "eCivic is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(eCivic >= 0, "eCivic is expected to be non-negative (invalid Index)");

	// Circumvents crash bug in simultaneous turns MP games
	if( eCivic == NO_CIVIC )
	{
		return 1;
	}

	if( isBarbarian() )
	{
		return 1;
	}

	const CvCivicInfo& kCivic = GC.getCivicInfo(eCivic);

	int iS = isCivic(eCivic)?-1 :1;// K-Mod, sign for whether we should be considering gaining a bonus, or losing a bonus

	bool bWarPlan = (kTeam.getAnyWarPlanCount(true) > 0);
	if( bWarPlan )
	{
		bWarPlan = false;
		int iEnemyWarSuccess = 0;

		for( int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++ )
		{
			if( GET_TEAM((TeamTypes)iTeam).isAlive() && !GET_TEAM((TeamTypes)iTeam).isMinorCiv() )
			{
				if( kTeam.AI_getWarPlan((TeamTypes)iTeam) != NO_WARPLAN )
				{
					if( kTeam.AI_getWarPlan((TeamTypes)iTeam) == WARPLAN_TOTAL || kTeam.AI_getWarPlan((TeamTypes)iTeam) == WARPLAN_PREPARING_TOTAL )
					{
						bWarPlan = true;
						break;
					}

					if( kTeam.AI_isLandTarget((TeamTypes)iTeam) )
					{
						bWarPlan = true;
						break;
					}

					iEnemyWarSuccess += GET_TEAM((TeamTypes)iTeam).AI_getWarSuccess(getTeam());
				}
			}
		}

		if( !bWarPlan )
		{
			if( iEnemyWarSuccess > std::min(iCities, 4) * GC.getWAR_SUCCESS_CITY_CAPTURING() )
			{
				// Lots of fighting, so war is real
				bWarPlan = true;
			}
			else if( iEnemyWarSuccess > std::min(iCities, 2) * GC.getWAR_SUCCESS_CITY_CAPTURING() )
			{
				if( kTeam.AI_getEnemyPowerPercent() > 120 )
				{
					bWarPlan = true;
				}
			}
		}
	}

	if( !bWarPlan )
	{
		// Aggressive players will stick with war civics
		if( kTeam.AI_getTotalWarOddsTimes100() > 200 )
		{
			bWarPlan = true;
		}
	}

	//int iConnectedForeignCities = countPotentialForeignTradeCitiesConnected();
	int iTotalReligonCount = countTotalHasReligion();
	ReligionTypes eBestReligion = getStateReligion();
	int iBestReligionPopulation = 0;
	if (kCivic.isStateReligion())
	{
		ReligionTypes temp = AI_bestReligion(); // only calculate best religion if this is a religious civic
		if (temp != NO_RELIGION)
			eBestReligion = temp;

		if (eBestReligion != NO_RELIGION)
		{
			int iLoop;
			for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity; pLoopCity = nextCity(&iLoop))
			{
				iBestReligionPopulation += pLoopCity->isHasReligion(eBestReligion) ? pLoopCity->getPopulation() : 0;
			}
		}
	}
	int iBestReligionCities = eBestReligion == NO_RELIGION ? 0 : getHasReligionCount(eBestReligion);
	int iWarmongerFactor = 25000 / std::max(100, (100 + GC.getLeaderHeadInfo(getPersonalityType()).getMaxWarRand()));
	// K-Mod note. max war rand is between 50 and 400, so I've renamed the above number from iWarmongerPercent to iWarmongerFactor.
	// I don't know what it is meant to be a percentage of. It's a number roughly between 56 and 167.

	int iMaintenanceFactor =  AI_commerceWeight(COMMERCE_GOLD) * std::max(0, calculateInflationRate() + 100) / 100; // K-Mod

	int iValue = (iCities * 6);

	iValue += (GC.getCivicInfo(eCivic).getAIWeight() * iCities);

	// K-Mod: civic anger is counted somewhere else
	//iValue += (getCivicPercentAnger(eCivic) / 10);

	iValue += -(GC.getCivicInfo(eCivic).getAnarchyLength() * iCities);

	//iValue += -(getSingleCivicUpkeep(eCivic, true)*80)/100;
	iValue -= getSingleCivicUpkeep(eCivic, true) * iMaintenanceFactor / 100; // K-Mod. (note. upkeep modifiers are included in getSingleCivicUpkeep.)

	CvCity* pCapital = getCapitalCity();
	iValue += ((kCivic.getGreatPeopleRateModifier() * iCities) / 10);
	iValue += ((kCivic.getGreatGeneralRateModifier() * getNumMilitaryUnits()) / 50);
	iValue += ((kCivic.getDomesticGreatGeneralRateModifier() * getNumMilitaryUnits()) / 100);
	/* original bts code
	iValue += -((kCivic.getDistanceMaintenanceModifier() * std::max(0, (iCities - 3))) / 8);
	iValue += -((kCivic.getNumCitiesMaintenanceModifier() * std::max(0, (iCities - 3))) / 8); */
	// K-Mod. After looking at a couple of examples, it's plain to see that the above maintenance estimates are far too big.
	// Surprisingly, it actually doesn't take much time to calculate the precise magnitude of the maintenance change. So that's what I'll do!
	if (kCivic.getNumCitiesMaintenanceModifier() != 0)
	{
		PROFILE("civicValue: NumCitiesMaintenance");
		int iTemp = 0;
		int iLoop;
		for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			iTemp += pLoopCity->calculateNumCitiesMaintenanceTimes100() * (pLoopCity->getMaintenanceModifier() + 100) / 100;
		}
		iTemp *= 100;
		iTemp /= std::max(1, getNumCitiesMaintenanceModifier() + 100);

		iTemp *= iMaintenanceFactor;
		iTemp /= 100;

		iValue -= iTemp * kCivic.getNumCitiesMaintenanceModifier() / 10000;
	}
	if (kCivic.getDistanceMaintenanceModifier() != 0)
	{
		PROFILE("civicValue: DistanceMaintenance");
		int iTemp = 0;
		int iLoop;
		for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			iTemp += pLoopCity->calculateDistanceMaintenanceTimes100() * (pLoopCity->getMaintenanceModifier() + 100) / 100;
		}
		iTemp *= 100;
		iTemp /= std::max(1, getDistanceMaintenanceModifier() + 100);

		iTemp *= iMaintenanceFactor;
		iTemp /= 100;

		iValue -= iTemp * kCivic.getDistanceMaintenanceModifier() / 10000;
	}
	// K-Mod end

	/* original bts code
	iValue += ((kCivic.getWorkerSpeedModifier() * AI_getNumAIUnits(UNITAI_WORKER)) / 15);
	iValue += ((kCivic.getImprovementUpgradeRateModifier() * iCities) / 50);
	iValue += (kCivic.getMilitaryProductionModifier() * iCities * iWarmongerPercent) / (bWarPlan ? 300 : 500 ); 
	iValue += (kCivic.getBaseFreeUnits() / 2);
	iValue += (kCivic.getBaseFreeMilitaryUnits() / 2);
	iValue += ((kCivic.getFreeUnitsPopulationPercent() * getTotalPopulation()) / 200);
	iValue += ((kCivic.getFreeMilitaryUnitsPopulationPercent() * getTotalPopulation()) / 300);
	iValue += -(kCivic.getGoldPerUnit() * getNumUnits());
	iValue += -(kCivic.getGoldPerMilitaryUnit() * getNumMilitaryUnits() * iWarmongerPercent) / 200; */
	// K-Mod, just a bunch of minor accuracy improvements to these approximations.
	if (kCivic.getWorkerSpeedModifier() != 0)
	{
		int iWorkers = 0;
		int iLoop;
		for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			iWorkers += 2 * pLoopCity->AI_getWorkersNeeded();
		}
		iWorkers -= AI_getNumAIUnits(UNITAI_WORKER);
		if (iWorkers > 0)
		{
			iValue += kCivic.getWorkerSpeedModifier() * iWorkers / 15;
		}
	}
	iValue += kCivic.getImprovementUpgradeRateModifier() * iCities / 50; // this is a tough one... I'll leave it alone for now.
	// value for kCivic.getMilitaryProductionModifier() has been moved
	if (kCivic.getBaseFreeUnits() || kCivic.getBaseFreeMilitaryUnits() ||
		kCivic.getFreeUnitsPopulationPercent() || kCivic.getFreeMilitaryUnitsPopulationPercent() ||
		kCivic.getGoldPerUnit() || kCivic.getGoldPerMilitaryUnit())
	{
		int iFreeUnits = 0;
		int iFreeMilitaryUnits = 0;
		int iUnits = getNumUnits();
		int iMilitaryUnits = getNumMilitaryUnits();
		int iPaidUnits = iUnits;
		int iPaidMilitaryUnits = iMilitaryUnits;
		int iMilitaryCost = 0;
		int iUnitCost = 0;
		int iExtraCost = 0; // unused
		calculateUnitCost(iFreeUnits, iFreeMilitaryUnits, iPaidUnits, iPaidMilitaryUnits, iUnitCost, iMilitaryCost, iExtraCost);

		int iTempValue = 0;

		// units costs. (note goldPerUnit and goldPerMilitaryUnit are in units of 1/100 gold)
		int iCostPerUnit = (getGoldPerUnit() + (iS > 0 ? kCivic.getGoldPerUnit() : 0)) * getUnitCostMultiplier() / 100;
		int iFreeUnitDelta = iS * (std::min(iUnits, iFreeUnits + iS*(kCivic.getBaseFreeUnits() + kCivic.getFreeUnitsPopulationPercent() * getTotalPopulation()/100)) - std::min(iUnits, iFreeUnits));
		FAssert(iFreeUnitDelta >= 0);
		iTempValue += iFreeUnitDelta * iCostPerUnit * iMaintenanceFactor / 10000;
		iTempValue -= (iPaidUnits-iFreeUnitDelta) * kCivic.getGoldPerUnit() * iMaintenanceFactor / 10000;

		// military
		iCostPerUnit = getGoldPerMilitaryUnit() + (iS > 0 ? kCivic.getGoldPerMilitaryUnit() : 0);
		iFreeUnitDelta = iS * (std::min(iMilitaryUnits, iFreeMilitaryUnits + iS*(kCivic.getBaseFreeMilitaryUnits() + kCivic.getFreeMilitaryUnitsPopulationPercent() * getTotalPopulation()/100)) - std::min(iMilitaryUnits, iFreeMilitaryUnits));
		FAssert(iFreeUnitDelta >= 0);
		iTempValue += iFreeUnitDelta * iCostPerUnit * iMaintenanceFactor / 10000;
		iTempValue -= (iPaidMilitaryUnits-iFreeUnitDelta) * kCivic.getGoldPerMilitaryUnit() * iMaintenanceFactor / 10000;

		// adjust based on future expectations
		if (iTempValue < 0)
		{
			iTempValue *= 100 + iWarmongerFactor / (bWarPlan ? 3 : 6);
			iTempValue /= 100;
		}
		iValue += iTempValue;
	}
	// K-Mod end

	//iValue += ((kCivic.isMilitaryFoodProduction()) ? 0 : 0);
	// bbai
	int iMaxConscript = getWorldSizeMaxConscript(eCivic);
	if( iMaxConscript > 0 && (pCapital != NULL) )
	{
		UnitTypes eConscript = pCapital->getConscriptUnit();
		if( eConscript != NO_UNIT )
		{
			// Nationhood
			int iCombatValue = kGame.AI_combatValue(eConscript);
			if( iCombatValue > 33 )
			{
				int iTempValue = iCities + ((bWarPlan) ? 30 : 10);

				/* iTempValue *= range(kTeam.AI_getEnemyPowerPercent(), 50, 300);
				iTempValue /= 100; */ // disabled by K-Mod

				iTempValue *= iCombatValue;
				iTempValue /= 75;

				int iWarSuccessRating = kTeam.AI_getWarSuccessRating();
				if( iWarSuccessRating < -25 )
				{
					iTempValue *= 75 + range(-iWarSuccessRating, 25, 100);
					iTempValue /= 100;
				}
				// K-Mod. (What's with all the "losing means we need drafting" mentality in the BBAI code? It's... not the right way to look at it.)
				// (NOTE: "conscript_population_per_cost" is actually "production_per_conscript_population". The developers didn't know what "per" means.)
				int iConscriptPop = std::max(1, GC.getUnitInfo(eConscript).getProductionCost() / GC.getDefineINT("CONSCRIPT_POPULATION_PER_COST"));
				int iProductionFactor = 100 * GC.getUnitInfo(eConscript).getProductionCost();
				iProductionFactor /= iConscriptPop * GC.getDefineINT("CONSCRIPT_POPULATION_PER_COST");
				iTempValue *= iProductionFactor;
				iTempValue /= 100;
				iTempValue *= std::min(iCities, iMaxConscript*3);
				iTempValue /= iMaxConscript*3;
				// reduce the value if unit spending is already high.
				int iUnitSpending = AI_unitCostPerMil();
				int iMaxSpending = AI_maxUnitCostPerMil() + 5 - iS*5; // increase max by 1% if we're already on this civic, just for a bit of inertia.
				if (iUnitSpending > iMaxSpending)
				{
					iTempValue = std::max(0, iTempValue * (2 * iMaxSpending - iUnitSpending)/std::max(1, iMaxSpending));
				}
				else if (iProductionFactor >= 140) // cf. 'bGoodValue' in CvCityAI::AI_doDraft
				{
					// advc.017: was 2*iMaxSpending
					iTempValue *= ::round(1.35*iMaxSpending);
					iTempValue /= std::max(1, iMaxSpending + iUnitSpending);
				}
				// todo. put in something to do with how much happiness we can afford to lose.. or something like that.
				// K-Mod end

				iValue += iTempValue;
			}
		}
	}
	// bbai end
/*
** K-Mod.
** evaluation of my new unhealthiness modifier
*/
	iValue += (iCities * 6 * iS * AI_getHealthWeight(-iS*kCivic.getUnhealthyPopulationModifier(), 1, true)) / 100;
	// c.f	iValue += (iCities * 6 * AI_getHealthWeight(kCivic.getExtraHealth(), 1)) / 100;

	// If the GW threshold has been reached, increase the value based on GW anger
	if (kGame.getGlobalWarmingIndex() > 0)
	{
		// estimate the happiness boost...
		// suppose pop pollution is 1/3 of total, and current relative contribution is around 100%
		// and anger percent scales like 2* relative contribution...
		// anger percent reduction will then be around (kCivic.getUnhealthyPopulationModifier()*2/3)%

		// Unfortunately, since adopting this civic will lower the very same anger percent that is giving the civic value...
		// the civic will be valued lower as soon as it is adopted. :(
		// Fixing this problem (and others like it) is a bit tricky. For now, I'll just try to fudge around it.
		int iGwAnger = getGwPercentAnger();
		if (isCivic(eCivic)) // Fudge factor
		{
			iGwAnger *= 100;
			iGwAnger /= 100 - 2*kCivic.getUnhealthyPopulationModifier()/3;
			// Note, this fudge factor is actually pretty good at estimating what the GwAnger would have been.
		}
		int iCleanValue = (iCities * 12 * iS * AI_getHappinessWeight(iS*ROUND_DIVIDE(-kCivic.getUnhealthyPopulationModifier()*iGwAnger*2,300), 1, true)) / 100;
		// This isn't a big reduction; and it should be the only part of this evaluation.
		// Maybe I'll add more later; such as some flavour factors.

		iValue += iCleanValue;
	}
// K-Mod end
	if (bWarPlan)
	{
		iValue += ((kCivic.getExpInBorderModifier() * getNumMilitaryUnits()) / 200);
	}
	iValue += ((kCivic.isBuildingOnlyHealthy()) ? (iCities * 3) : 0);
	//iValue += -((kCivic.getWarWearinessModifier() * getNumCities()) / ((bWarPlan) ? 10 : 50));
	// <cdtw.4> Replacing the above
	if(kCivic.getWarWearinessModifier() != 0) {
		// comment Dave_uk:
		/*  lets take into account how bad war weariness actually is for us.
			this is actually double counting since we already account for how
			happiness will be affected below, however, if war weariness is
			really killing us we need to take action and switch */
		int iTempValue = -kCivic.getWarWearinessModifier() * getNumCities();
		iTempValue *= std::max(getWarWearinessPercentAnger(), 100);
		iTempValue /= 100;
		iTempValue /= ((bWarPlan) ? 10 : 50);
		iValue += iTempValue;
	} // </cdtw.4>
	//iValue += (kCivic.getFreeSpecialist() * iCities * 12);
	// K-Mod. A rough approximation is ok, but perhaps not quite /that/ rough.
	// Here's some code that I wrote for specialists in AI_buildingValue. (note. building value uses 4x commerce; but here we just use 1x commerce)
	if (kCivic.getFreeSpecialist() != 0)
	{
		int iSpecialistValue = 5 * 100; // rough base value
		// additional bonuses
		for (CommerceTypes i = (CommerceTypes)0; i < NUM_COMMERCE_TYPES; i = (CommerceTypes)(i+1))
		{
			int c = getSpecialistExtraCommerce(i) + kCivic.getSpecialistExtraCommerce(i);
			if (c)
				iSpecialistValue += c * AI_commerceWeight(i);
		}
		iSpecialistValue += 2*std::max(0, AI_averageGreatPeopleMultiplier()-100);
		iValue += iCities * iSpecialistValue / 100;
	}
	// K-Mod end

	/*
	iValue += (kCivic.getTradeRoutes() * (std::max(0, iConnectedForeignCities - iCities * 3) * 6 + (iCities * 2)));
	iValue += -((kCivic.isNoForeignTrade()) ? (iConnectedForeignCities * 3) : 0); */

	// K-Mod - take a few more things into account for trade routes.
	// NOTE: this calculation makes a bunch of assumptions about about the yield type and magnitude from trade routes.
	if (kCivic.getTradeRoutes() != 0 || kCivic.isNoForeignTrade())
	{
		PROFILE("civicValue: trade routes");
		// As a rough approximation, let each foreign trade route give base 3 commerce, and local trade routes give 1.
		// Our civ can have 1 connection to each foreign city.

		int iTempValue = 0;
		int iConnectedForeignCities = AI_countPotentialForeignTradeCities(true, AI_getFlavorValue(FLAVOR_GOLD) == 0);

		int iTotalTradeRoutes = iCities * 3; // I'll save the proper count for when the estimate of yield / route improves...
		/* int iLoop;
		for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			iTotalTradeRoutes += pLoopCity->getTradeRoutes();
		}
		if (iS < 0)
			iTotalTradeRoutes -= kCivic.getTradeRoutes() * iCities; */

		FAssert(iTotalTradeRoutes >= iCities);

		if (kCivic.isNoForeignTrade())
		{
			// We should attempt the block one-way trade which other civs might be getting from us.
			// count how many foreign trade cities we are not connected to...
			int iDisconnectedForeignTrade = AI_countPotentialForeignTradeCities(false, AI_getFlavorValue(FLAVOR_GOLD) == 0) - iConnectedForeignCities;
			FAssert(iDisconnectedForeignTrade >= 0);
			// and estimate the value of blocking foreign trade to these cities. (we don't get anything from this, but we're denying our potential rivals.)
			iTempValue += std::min(iDisconnectedForeignTrade, iCities); // just 1 point per city.

			// estimate the number of foreign cities which are immune to "no foreign trade".
			int iSafeOverseasTrade = 0;
			for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i = (TeamTypes)(i+1))
			{
				if (i == getTeam() || !kTeam.isFreeTrade(i))
					continue;
				if (kTeam.isVassal(i) || GET_TEAM(i).isVassal(getTeam()))
					iSafeOverseasTrade += GET_TEAM(i).getNumCities();
			}
			iSafeOverseasTrade = std::min(iSafeOverseasTrade, iConnectedForeignCities);

			iTempValue -= std::min(iConnectedForeignCities - iSafeOverseasTrade, iTotalTradeRoutes) * 3/2; // only reduce by 1.5 rather than 2, because it is good to deny our rivals the trade.
			iConnectedForeignCities = iSafeOverseasTrade;
		}

		// Unfortunately, it's not easy to tell whether or not we'll foreign trade when we switch to this civic.
		// for example, if we are considering a switch from mercantilism to free market, we'd currently have "noForeignTrade", but if we actually make the switch then we wouldn't.
		//  ... So for simplicity I'm going to just assume that there is only one civic with noForeignTrade, and therefore the value of iConnectedForeignCities is unchanged.
		//CvCity* pCapital = getCapitalCity(); // advc.003: Redundant
		if (pCapital)
		{
			// add an estimate of our own overseas cities (even though they are not really "foreign".
			iConnectedForeignCities += iCities - pCapital->area()->getCitiesPerPlayer(getID());
		}

		iTempValue += kCivic.getTradeRoutes() * std::max(0, iConnectedForeignCities - iTotalTradeRoutes) * 2 + iCities * 1;

		// Trade routes increase in value as cities grow, and build trade multipliers
		iTempValue *= 2*(getCurrentEra()+1) + GC.getNumEraInfos();
		iTempValue /= GC.getNumEraInfos();
		// commerce multipliers
		iTempValue *= AI_averageYieldMultiplier(YIELD_COMMERCE);
		iTempValue /= 100;
		iValue += iTempValue;
	}
	// K-Mod end

	// Corporations (K-Mod edition!)
	if (kCivic.isNoCorporations() || kCivic.isNoForeignCorporations() || kCivic.getCorporationMaintenanceModifier() != 0)
	{
		PROFILE("civicValue: corporation effects");

		int iPotentialCorpValue = 0; // an estimate of the value lost when "isNoCorporations" blocks us from founding a good corp.

		for (CorporationTypes eCorp = (CorporationTypes)0; eCorp < GC.getNumCorporationInfos(); eCorp=(CorporationTypes)(eCorp+1))
		{
			const CvCorporationInfo& kCorpInfo = GC.getCorporationInfo(eCorp);
			int iSpeculation = 0;

			if (!kGame.isCorporationFounded(eCorp))
			{
				// if this civic is going to block us from founding any new corporations
				// then we should try to estimate the cost of blocking such an opportunity.
				// (but we don't count corporations that are simply founded by a technology)
				if (AI_getFlavorValue(FLAVOR_GROWTH) + AI_getFlavorValue(FLAVOR_SCIENCE) > 0 && kCivic.isNoCorporations() && kCorpInfo.getTechPrereq() == NO_TECH)
				{
					// first, if this corp competes with a corp that we already have, then assume we don't want it.
					bool bConflict = false;
					for (CorporationTypes i = (CorporationTypes)0; i < GC.getNumCorporationInfos(); i=(CorporationTypes)(i+1))
					{
						if (kGame.isCompetingCorporation(eCorp, i) && kGame.isCorporationFounded(i) && countCorporations(i) > 0)
						{
							bConflict = true;
							break;
						}
					}

					if (!bConflict)
					{
						// Find the building that founds the corp, and check how many players have the prereq techs.
						// Note: I'm assuming that the conditions for this building are reasonable.
						// eg. if a great person is required, then it should be a great person that we are actually able to get!
						for (BuildingTypes i = (BuildingTypes)0; i < GC.getNumBuildingInfos(); i=(BuildingTypes)(i+1))
						{
							const CvBuildingInfo& kLoopBuilding = GC.getBuildingInfo(i);
							if (kLoopBuilding.getFoundsCorporation() == eCorp && kLoopBuilding.getProductionCost() < 0) // don't count buildings that can be constructed normally
							{
								bool bHasPrereq = true;

								if (canDoCivics(eCivic))
								{
									// Only check the tech prereqs for the corp if we already have the prereqs for this civic.
									// (This condition will help us research towards the corp tech rather than researching towards the civic that will block the corp.)
									bHasPrereq = kTeam.isHasTech((TechTypes)kLoopBuilding.getPrereqAndTech()) || canResearch((TechTypes)kLoopBuilding.getPrereqAndTech());

									for (int iI = 0; bHasPrereq && iI < GC.getNUM_BUILDING_AND_TECH_PREREQS(); iI++)
									{
										bHasPrereq = kTeam.isHasTech((TechTypes)kLoopBuilding.getPrereqAndTechs(iI)) || canResearch((TechTypes)kLoopBuilding.getPrereqAndTechs(iI));
									}
								}

								if (bHasPrereq)
								{
									iSpeculation = 2;
									// +1 for each other player we know with all the prereqs
									for (PlayerTypes j = (PlayerTypes)0; j < MAX_CIV_PLAYERS; j=(PlayerTypes)(j+1))
									{
										const CvTeam& kLoopTeam = GET_TEAM(GET_PLAYER(j).getTeam());
										if (kLoopTeam.getID() != getTeam() && kTeam.isHasMet(kLoopTeam.getID()))
										{
											bHasPrereq = kLoopTeam.isHasTech((TechTypes)kLoopBuilding.getPrereqAndTech());

											for (int iI = 0; bHasPrereq && iI < GC.getNUM_BUILDING_AND_TECH_PREREQS(); iI++)
											{
												bHasPrereq = kLoopTeam.isHasTech((TechTypes)kLoopBuilding.getPrereqAndTechs(iI));
											}
											if (bHasPrereq)
												iSpeculation++;
										}
									}
								}
								// assume there is only one building that founds the corporation
								break;
							}
						}
					}
				}

				if (iSpeculation == 0)
					continue; // no value from this corp, speculative or otherwise
			}

			bool bPlayerHQ = false;
			bool bTeamHQ = false;
			if (hasHeadquarters(eCorp) || iSpeculation > 0)
			{
				bPlayerHQ = true;
				bTeamHQ = true;
			}
			else if (kTeam.hasHeadquarters(eCorp))
			{
				bTeamHQ = true;
			}

			int iCorpValue = 0; // total civic value change due to the effect of this corporation.

			int iBonuses = 0;
			int iCorpCities = countCorporations(eCorp);
			int iMaintenance = 0;
			// If the HQ is ours, assume we will spread the corp. If it is not our, assume we don't care.
			// (Note: this doesn't take into account the posibility of competing corps. Sorry.)
			if (bTeamHQ)
			{
				iCorpCities = ((bPlayerHQ ?1 :2)*iCorpCities + iCities)/(bPlayerHQ ?2 :3);
			}

			for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
			{
				BonusTypes eBonus = (BonusTypes)kCorpInfo.getPrereqBonus(i);
				if (NO_BONUS != eBonus)
				{
					iBonuses +=	kTeam.isBonusRevealed(eBonus) ? countOwnedBonuses(eBonus) : 1; // expect that we'll get at least one of each unrevealed bonus.
					// maybe use getNumAvailableBonuses ?
				}
			}
			iBonuses += bPlayerHQ ? 1 : 0;

			for (CommerceTypes i = (CommerceTypes)0; i < NUM_COMMERCE_TYPES; i = (CommerceTypes)(i+1))
			{
				int iTempValue = 0;

				// loss of the headquarter bonus from our cities.
				if (bTeamHQ &&
					( (kCivic.isNoForeignCorporations() && !bPlayerHQ) ||
					kCivic.isNoCorporations() ))
				{
					CvCity* pHqCity = iSpeculation == 0 ? kGame.getHeadquarters(eCorp) : pCapital;
					if (pHqCity)
						iTempValue -= pHqCity->getCommerceRateModifier(i) * kCorpInfo.getHeadquarterCommerce(i) * iCorpCities / 100;
				}

				// loss of corp commerce bonuses
				if (kCivic.isNoCorporations() || (kCivic.isNoForeignCorporations() && !bPlayerHQ))
				{
					iTempValue -= iCorpCities * ((AI_averageCommerceMultiplier(i) * kCorpInfo.getCommerceProduced(i))/100 * iBonuses * GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getCorporationMaintenancePercent()) / 10000;
				}

				if (iTempValue != 0)
				{
					iTempValue *= AI_commerceWeight(i);
					iTempValue /= 100;

					iCorpValue += iTempValue;
				}

				iMaintenance += kCorpInfo.getHeadquarterCommerce(i) * iCorpCities;
			}

			if (kCivic.isNoCorporations() || (kCivic.isNoForeignCorporations() && !bPlayerHQ))
			{
				// loss of corp yield bonuses
				for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
				{
					int iTempValue = -(iCorpCities * kCorpInfo.getYieldProduced((YieldTypes)iI) * iBonuses);
					iTempValue *= AI_averageYieldMultiplier((YieldTypes)iI);
					iTempValue /= 100;
					iTempValue *= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getCorporationMaintenancePercent();
					iTempValue /= 10000; // getYieldProduced is x100.

					iTempValue *= AI_yieldWeight((YieldTypes)iI);
					iTempValue /= 100;

					iCorpValue += iTempValue;
				}

				// loss of corp resource
				if (kCorpInfo.getBonusProduced() != NO_BONUS)
				{
					iCorpValue -= AI_bonusVal((BonusTypes)kCorpInfo.getBonusProduced(), 1, false) / 4;
				}
			}

			// loss of maintenance cost (money saved)
			int iTempValue = kCorpInfo.getMaintenance() * iBonuses * iCorpCities;
			iTempValue *= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getCorporationMaintenancePercent();
			iTempValue /= 10000;
			iTempValue += iMaintenance;
			// population, and maintenance modifiers... lets just approximate them like this:
			iTempValue *= 2;
			iTempValue /= 3;

			// (note, corp maintenance is not amplified by inflation, and so we don't use iMaintenanceFactor here.)
			iTempValue *= AI_commerceWeight(COMMERCE_GOLD);
			iTempValue /= 100;

			if (kCivic.isNoCorporations() || (kCivic.isNoForeignCorporations() && !bPlayerHQ))
			{
				iCorpValue += iTempValue;
			}
			else
			{
				iCorpValue += (-kCivic.getCorporationMaintenanceModifier() * iTempValue)/100;
			}

			FAssert(iSpeculation == 0 || (!kGame.isCorporationFounded(eCorp) && kCivic.isNoCorporations()));
			iCorpValue;
			if (iSpeculation == 0)
				iValue += iCorpValue;
			else
			{
				// Note: when speculating about disabling unfounded corps, we only count the losses. (If the corp is negative value, then we wouldn't found it!)
				iCorpValue *= 2;
				iCorpValue /= iSpeculation + 1; // iSpeculation == 2 + # of other civs with the prereqs.
				if (iCorpValue < iPotentialCorpValue)
					iPotentialCorpValue = iCorpValue;
			}
		}
		FAssert(iPotentialCorpValue <= 0);
		iValue += iPotentialCorpValue;
	}
	// K-Mod end


	if (kCivic.getCivicPercentAnger() != 0)
	{
		// K-Mod version. (the original bts code was ridiculous - it is now deleted)
		// value for negation of unhappiness. Again, let me point out how _stupid_ it is that "percent_anger_divisor" is 1000, not 100.
		iValue += (iCities * 12 * iS * AI_getHappinessWeight(iS*getCivicPercentAnger(eCivic, true)*100/GC.getPERCENT_ANGER_DIVISOR(), 2, true)) / 100;
		// value for putting pressure on other civs. (this should probably take into account the civics of other civs)
		iValue += kCivic.getCivicPercentAnger() * (iCities * iCities - iCities) / (5*kGame.getNumCities()); // the 5* on the end is because "percent" is really per mil.
	}

	if (kCivic.getExtraHealth() != 0)
		iValue += (iCities * 6 * iS * AI_getHealthWeight(iS*kCivic.getExtraHealth(), 1)) / 100;

	if (kCivic.getExtraHappiness() != 0) // New K-Mod effect
		iValue += (iCities * 10 * iS * AI_getHappinessWeight(iS *
				/*  (zero extra citizens... as a kind of kludge to get the
					value closer to where I want it) */
				kCivic.getExtraHappiness(), 0)) / 100;

	if (kCivic.getHappyPerMilitaryUnit() != 0)
		iValue += (iCities * 9 * iS * AI_getHappinessWeight(iS*kCivic.getHappyPerMilitaryUnit() * 3, 1))
				// <advc.912c>
				/ 200; // divisor was 100
	if (kCivic.getLuxuryModifier() > 0 && pCapital != NULL) {
		int luxMod = kCivic.getLuxuryModifier();
		int happyChangeCapital = (pCapital->getBonusGoodHappiness(true) *
				luxMod) / 100;
		iValue += (iCities * 9 * iS * AI_getHappinessWeight(iS *
				happyChangeCapital, 1)) / 100;
	} // </advc.912c>
	if (kCivic.getLargestCityHappiness() != 0)
		iValue += (16 * std::min(iCities, GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities()) * iS * AI_getHappinessWeight(iS*kCivic.getLargestCityHappiness(), 1)) / 100; // Higher weight, because it affects the best cities (was 14)

	if (kCivic.getWarWearinessModifier() != 0) // K-Mod. (original code deleted)
		iValue += (12 * iCities * iS * AI_getHappinessWeight(iS*ROUND_DIVIDE(getWarWearinessPercentAnger() * -getWarWearinessModifier(), GC.getPERCENT_ANGER_DIVISOR()), 1, true)) / 100;

	//iValue += (kCivic.getNonStateReligionHappiness() * (iTotalReligonCount - iHighestReligionCount) * 5);
	// K-Mod
	if (kCivic.getNonStateReligionHappiness() != 0)
		iValue += 12 * iCities * iS * AI_getHappinessWeight(iS * kCivic.getNonStateReligionHappiness() * iTotalReligonCount / std::max(1, iCities), 0) / 100;
	// K-Mod end

	// K-Mod. Experience and production modifiers
	{
		// Roughly speaking these are the approximations used in this section:
		// each population produces 1 hammer.
		// percentage of hammers spent on units is BuildUnitProb + 40 if at war
		// experience points are worth a production boost of 8% each, multiplied by the warmonger factor, and componded with actual production multipliers
		int iProductionShareUnits = GC.getLeaderHeadInfo(getPersonalityType()).getBuildUnitProb();
		if (bWarPlan)
			iProductionShareUnits = (100 + iProductionShareUnits)/2;
		else if (AI_isDoStrategy(AI_STRATEGY_ECONOMY_FOCUS))
			iProductionShareUnits /= 2;

		int iProductionShareBuildings = 100 - iProductionShareUnits;

		int iTempValue = getTotalPopulation() * kCivic.getMilitaryProductionModifier() + iBestReligionPopulation * kCivic.getStateReligionUnitProductionModifier();

		int iExperience = getTotalPopulation() * kCivic.getFreeExperience() + iBestReligionPopulation * kCivic.getStateReligionFreeExperience();
		if (iExperience)
		{
			iExperience *= 8 * std::max(iWarmongerFactor, (bWarPlan ? 100 : 0));
			iExperience /= 100;
			iExperience *= AI_averageYieldMultiplier(YIELD_PRODUCTION);
			iExperience /= 100;

			iTempValue += iExperience;
		}

		iTempValue *= iProductionShareUnits;
		iTempValue /= 100;

		iTempValue += iBestReligionPopulation * kCivic.getStateReligionBuildingProductionModifier() * iProductionShareBuildings / 100;
		iTempValue *= AI_yieldWeight(YIELD_PRODUCTION);
		iTempValue /= 100;
		iValue += iTempValue / 100;

		/* old modifiers, (just for reference)
		iValue += (kCivic.getMilitaryProductionModifier() * iCities * iWarmongerFactor) / (bWarPlan ? 300 : 500 );
		if (kCivic.getFreeExperience() > 0)
		{
			// Free experience increases value of hammers spent on units, population is an okay measure of base hammer production
			int iTempValue = (kCivic.getFreeExperience() * getTotalPopulation() * (bWarPlan ? 30 : 12))/100;
			iTempValue *= AI_averageYieldMultiplier(YIELD_PRODUCTION);
			iTempValue /= 100;
			iTempValue *= iWarmongerFactor;
			iTempValue /= 100;
			iValue += iTempValue;
		}
		iValue += ((kCivic.getStateReligionUnitProductionModifier() * iBestReligionCities) / 4);
		iValue += ((kCivic.getStateReligionBuildingProductionModifier() * iBestReligionCities) / 3);
		iValue += (kCivic.getStateReligionFreeExperience() * iBestReligionCities * ((bWarPlan) ? 6 : 2)); */
	}
	// K-Mod end

	if (kCivic.isStateReligion())
	{
		if (iBestReligionCities > 0)
		{
			/* original bts code
			iValue += iHighestReligionCount;

			iValue += ((kCivic.isNoNonStateReligionSpread()) ? ((iCities - iHighestReligionCount) * 2) : 0);
			iValue += (kCivic.getStateReligionHappiness() * iHighestReligionCount * 4);
			iValue += ((kCivic.getStateReligionGreatPeopleRateModifier() * iHighestReligionCount) / 20);
			iValue += (kCivic.getStateReligionGreatPeopleRateModifier() / 4);
			iValue += ((kCivic.getStateReligionUnitProductionModifier() * iHighestReligionCount) / 4);
			iValue += ((kCivic.getStateReligionBuildingProductionModifier() * iHighestReligionCount) / 3);
			iValue += (kCivic.getStateReligionFreeExperience() * iHighestReligionCount * ((bWarPlan) ? 6 : 2)); */

			// K-Mod
			FAssert(eBestReligion != NO_RELIGION);
			iValue += (2 * iBestReligionPopulation - getTotalPopulation()) / 6;

			if (kCivic.isNoNonStateReligionSpread())
			{	// <advc.115b>
				if(GET_TEAM(getTeam()).isAVassal()) {
					CvTeamAI const& master = GET_TEAM(getMasterTeam());
					if(!master.isHuman() && master.isAnyCloseToReligiousVictory())
						return -1000;
				} // </advc.115b>
				if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2))
				{
					iValue -= 3 * std::max(0, iCities + iBestReligionCities - iTotalReligonCount);
				}
				else if (eBestReligion == GC.getLeaderHeadInfo(getLeaderType()).getFavoriteReligion()
					&& /*  advc.115b: Used to be a logical OR. Theocracy as the
					   standard civic from Medieval onwards hurts
					   religious strategies unduly. */
					(hasHolyCity(eBestReligion) && countHolyCities() == 1))
				{
					iValue += iCities * 2; // protection vs espionage? depriving foreign civs of religion revenue?
				}
			}

			// getStateReligionFreeExperience, getStateReligionBuildingProductionModifier, getStateReligionUnitProductionModifier; all moved elsewhere.

			if (kCivic.getStateReligionHappiness() != 0)
			{
				iValue += 10 * iCities * iS * AI_getHappinessWeight(iS * kCivic.getStateReligionHappiness(), 1) * iBestReligionPopulation / std::max(1, 100 * getTotalPopulation());
			}
			if (kCivic.getStateReligionGreatPeopleRateModifier() != 0)
			{
				// This is not going to be very good. I'm sorry about that. I can't think of a neat way to make it better.
				// (The best way would involve counting GGP, weighted by the AI value for great person type,
				// multiplied by the probability that the city will actually be able to produce a great person, and so on.
				// But I'm worried that would be too slow / complex.)

				std::vector<int> base_rates;
				int iLoop;
				for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity; pLoopCity = nextCity(&iLoop))
				{
					if (pLoopCity->isHasReligion(eBestReligion))
						base_rates.push_back(pLoopCity->getBaseGreatPeopleRate());
				}
				int iGpCities = std::min((int)base_rates.size(), 3);
				std::partial_sort(base_rates.begin(), base_rates.begin()+iGpCities, base_rates.end());

				int iTempValue = 0;
				for (int i = 0; i < iGpCities; i++)
					iTempValue += base_rates[i];

				iValue += 2 * kCivic.getStateReligionGreatPeopleRateModifier() * iTempValue / 100;
			}

			// apostolic palace
			for (VoteSourceTypes i = (VoteSourceTypes)0; i < GC.getNumVoteSourceInfos(); i = (VoteSourceTypes)(i+1))
			{
				if (kGame.isDiploVote(i))
				{
					if (kGame.getVoteSourceReligion(i) == eBestReligion && isLoyalMember(i))
					{
						int iTempValue = getTotalPopulation() * iBestReligionCities / std::max(1, iCities);

						if (getTeam() == kGame.getSecretaryGeneral(i))
							iTempValue *= 2;
						if (kTeam.isForceTeamVoteEligible(i))
							iTempValue /= 2;

						iValue += iTempValue;
					}
				}
			}

			// Value civic based on wonders granting state religion boosts
			for (CommerceTypes i = (CommerceTypes)0; i < NUM_COMMERCE_TYPES; i = (CommerceTypes)(i+1))
			{
				int iTempValue = iBestReligionCities * getStateReligionBuildingCommerce(i);
				if (iTempValue)
				{
					iTempValue *= AI_averageCommerceMultiplier(i);
					iTempValue /= 100;

					iTempValue *= AI_commerceWeight(i);
					iTempValue /= 100;

					iValue += iTempValue;
				}
			}
			// K-Mod end
		}
	}

	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		int iTempValue = 0;

		//iTempValue += ((kCivic.getYieldModifier(iI) * iCities) / 2);
		iTempValue += kCivic.getYieldModifier(iI) * iCities / 4; // K-Mod (Still bogus, but I'd rather assume 25 yield/turn average than 50.)
		
		if (pCapital) 
		{
			// Bureaucracy
			// K-Mod
			int iTemp = (kCivic.getCapitalYieldModifier(iI) * pCapital->getBaseYieldRate((YieldTypes)iI));
			if (iTemp != 0)
			{
				switch (iI)
				{
				case YIELD_PRODUCTION:
					// For production, we inflate the value a little to account for the fact that it may help us win wonder races.
					iTemp /= 80;
					break;
				case YIELD_COMMERCE:
					// For commerce, the multiplier is compounded by the multipliers on individual commerce types.
					iTemp *= pCapital->AI_yieldMultiplier((YieldTypes)iI);
					iTemp /= 100 * std::max(1, pCapital->getBaseYieldRateModifier((YieldTypes)iI));
					break;
				default:
					iTemp /= 100;
					break;
				}
				iTempValue += iTemp;
			}
			// K-Mod end
		}
		iTempValue += ((kCivic.getTradeYieldModifier(iI) * iCities) / 11);
		// (K-Mod note: that denominator is bogus, but since no civics currently have this modifier anyway, I'm just going to leave it.)

		for (int iJ = 0; iJ < GC.getNumImprovementInfos(); iJ++)
		{
			// Free Speech
			iTempValue += (AI_averageYieldMultiplier((YieldTypes)iI) * (kCivic.getImprovementYieldChanges(iJ, iI) * (getImprovementCount((ImprovementTypes)iJ) + iCities/2))) / 100;
		}

		/* original code
		if (iI == YIELD_FOOD)
		{ 
			iTempValue *= 3; 
		} 
		else if (iI == YIELD_PRODUCTION) 
		{ 
			iTempValue *= ((AI_avoidScience()) ? 6 : 2); 
		} 
		else if (iI == YIELD_COMMERCE) 
		{ 
			iTempValue *= ((AI_avoidScience()) ? 2 : 4);
			iTempValue /= 3;
		} */

		// K-Mod
		iTempValue *= AI_yieldWeight((YieldTypes)iI);
		iTempValue /= 100;
		// K-mod end

		iValue += iTempValue;
	}

	// K-Mod: count bonus specialists,
	// so that we can more accurately gauge the representation bonus
	int iTotalBonusSpecialists = -1;
	int iTotalCurrentSpecialists = -1;

	// only take the time to count them if the civic has a bonus for specialists
	bool bSpecialistCommerce = false;
	for (int iI = 0; !bSpecialistCommerce && iI < NUM_COMMERCE_TYPES; iI++)
	{
		bSpecialistCommerce = kCivic.getSpecialistExtraCommerce(iI) != 0;
	}

	if (bSpecialistCommerce)
	{
		iTotalBonusSpecialists = iTotalCurrentSpecialists = 0;

		int iLoop;
		CvCity* pLoopCity;
		for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			iTotalBonusSpecialists += pLoopCity->getNumGreatPeople();
			iTotalBonusSpecialists += pLoopCity->totalFreeSpecialists();

			iTotalCurrentSpecialists += pLoopCity->getNumGreatPeople();
			iTotalCurrentSpecialists += pLoopCity->getSpecialistPopulation();
		}
	}
	// K-Mod end

	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		int iTempValue = 0;

		// K-Mod
		iTempValue += kCivic.getCommerceModifier(iI) * 100*getCommerceRate((CommerceTypes)iI) / AI_averageCommerceMultiplier((CommerceTypes)iI);
		if (pCapital != NULL)
		{
			iTempValue += kCivic.getCapitalCommerceModifier(iI) * pCapital->getBaseCommerceRate((CommerceTypes)iI);
		}

		// Representation
		//iTempValue += ((kCivic.getSpecialistExtraCommerce(iI) * getTotalPopulation()) / 15);
		// K-Mod
		if (bSpecialistCommerce)
			iTempValue += AI_averageCommerceMultiplier((CommerceTypes)iI)*(kCivic.getSpecialistExtraCommerce(iI) * std::max((getTotalPopulation()+10*iTotalBonusSpecialists) / 10, iTotalCurrentSpecialists));
		// K-Mod end

		iTempValue /= 100; // (for the 3 things above)

		if (iTempValue)
		{
			iTempValue *= AI_commerceWeight((CommerceTypes)iI);
			iTempValue /= 100;

			iValue += iTempValue;
		}
	}

	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		int iTempValue = kCivic.getBuildingHappinessChanges(iI);
		/* original bts code
		if (iTempValue != 0)
		{
			// Nationalism
			if( !isNationalWonderClass((BuildingClassTypes)iI) )
			{
				iValue += (iTempValue * iCities)/2;
			}
			iValue += (iTempValue * getBuildingClassCountPlusMaking((BuildingClassTypes)iI) * 2);
		}*/
		// K-Mod
		if (iTempValue != 0)
		{
			int iExpectedBuildings = 0;
			if (canConstruct((BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI)))
			{
				iExpectedBuildings = (iCities + 2*getBuildingClassCountPlusMaking((BuildingClassTypes)iI))/3;
			}
			iValue += (10 * iExpectedBuildings * iS * AI_getHappinessWeight(iS * iTempValue, 1))/100;
		}
	}

	for (int iI = 0; iI < GC.getNumFeatureInfos(); iI++)
	{
		int iHappiness = kCivic.getFeatureHappinessChanges(iI);

		if (iHappiness != 0)
		{
			iValue += (iHappiness * countCityFeatures((FeatureTypes)iI) * 5);
		}
	}

	for (HurryTypes i = (HurryTypes)0; i < GC.getNumHurryInfos(); i=(HurryTypes)(i+1))
	{
		if (kCivic.isHurry(i))
		{
			/* original bts code
			int iTempValue = 0;

			if (GC.getHurryInfo((HurryTypes)iI).getGoldPerProduction() > 0)
			{
				iTempValue += ((((AI_avoidScience()) ? 50 : 25) * iCities) / GC.getHurryInfo((HurryTypes)iI).getGoldPerProduction());
			}
			iTempValue += (GC.getHurryInfo((HurryTypes)iI).getProductionPerPopulation() * iCities * (bWarPlan ? 2 : 1)) / 5;

			iValue += iTempValue; */

			// K-Mod. I'm not attempting to made an accurate estimate of the value here - I just want to make it a little bit more nuanced than it was.
			int iTempValue = 0;
			const CvHurryInfo& kHurryInfo = GC.getHurryInfo(i);

			if (kHurryInfo.getGoldPerProduction() > 0)
			{
				iTempValue = AI_averageCommerceMultiplier(COMMERCE_GOLD) * (AI_avoidScience() ? 2000 : 1000) * iCities / kHurryInfo.getGoldPerProduction();
				iTempValue /= std::max(1, (getHurryModifier() + 100) * AI_commerceWeight(COMMERCE_GOLD));
			}

			if (kHurryInfo.getProductionPerPopulation() > 0)
			{
				// <advc.121>
				int multiplier = 3;
				if(bWarPlan) multiplier++;
				if(pCapital != NULL && pCapital->area()->getAreaAIType(getTeam()) ==
						AREAAI_DEFENSIVE && isFocusWar())
					multiplier++; // </advc.121>
				// if we had easy access to averages for getMaxFoodKeptPercent and getHurryAngerModifier, then I'd use them. - but I don't want to calculate them here.
				iTempValue += //(bWarPlan ? 8 : 5)
					multiplier // advc.121: Replacing the above 8 : 5
					* iCities *
					kGame.getProductionPerPopulation(i) /
					std::max(1, getGrowthThreshold(getAveragePopulation()));
			}

			if (iTempValue > 0)
			{
				if (kHurryInfo.getProductionPerPopulation() && kHurryInfo.getGoldPerProduction())
					iTempValue /= 2;

				iValue += iTempValue;
			}
			// K-Mod end
		}
	}

	/* for (int iI = 0; iI < GC.getNumSpecialBuildingInfos(); iI++)
	{
		if (kCivic.isSpecialBuildingNotRequired(iI))
		{
			iValue += ((iCities / 2) + 1); // XXX
		}
	} */ // Disabled by K-Mod. This evaluation isn't accurate enough to be useful - but it does sometimes cause civs to switch to organized religion when they don't have a religion...

	for (int iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
	{
		int iTempValue = 0;
		if (kCivic.isSpecialistValid(iI))
		{
			// K-Mod todo: the current code sucks. Fix it.
			iTempValue += iCities * (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) ? 10 : 1) + 6;
		}
		iValue += (iTempValue / 2);
	}

	// K-Mod. When aiming for a diplomatic victory, consider the favourite civics of our friends!
	if (AI_isDoVictoryStrategy(AI_VICTORY_DIPLOMACY3))
	{
		for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
		{
			const CvPlayerAI& kLoopPlayer = GET_PLAYER(i);

			if (!kLoopPlayer.isAlive() || kLoopPlayer.getTeam() == getTeam() || !kTeam.isHasMet(kLoopPlayer.getTeam()))
				continue;

			if (kLoopPlayer.isHuman())
				continue; // human players don't care about favourite civics. The AI should understand this.

			AttitudeTypes eAttitude = AI_getAttitude(i, false);
			if (eAttitude >= ATTITUDE_PLEASED)
			{
				const CvLeaderHeadInfo& kPersonality = GC.getLeaderHeadInfo(kLoopPlayer.getPersonalityType());
				if (kPersonality.getFavoriteCivic() == eCivic && kLoopPlayer.isCivic(eCivic))
				{
					// (better to use getVotes; but that's more complex.)
					//iValue += kLoopPlayer.getTotalPopulation() * (2 + kPersonality.getFavoriteCivicAttitudeChangeLimit()) / 20;
					iValue += kLoopPlayer.getTotalPopulation() / 5; // lets keep it simple
				}
			}
		}
	}
	// K-Mod end

	if (GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic() == eCivic)
	{
		if (!kCivic.isStateReligion() || iBestReligionCities > 0)
		{
			iValue *= 5; 
			iValue /= 4; 
			iValue += 6 * iCities;
			iValue += 20; 
		}
	}

	/* if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2) && (GC.getCivicInfo(eCivic).isNoNonStateReligionSpread()))
	{
		iValue /= 10;
	} */ // what the lol...

	return iValue;
}

ReligionTypes CvPlayerAI::AI_bestReligion() const
{
	int iBestValue = 0;
	ReligionTypes eBestReligion = NO_RELIGION;

	ReligionTypes eFavorite = (ReligionTypes)GC.getLeaderHeadInfo(getLeaderType()).getFavoriteReligion();

	for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		if (canDoReligion((ReligionTypes)iI))
		{
			int iValue = AI_religionValue((ReligionTypes)iI);

			/* original bts code
			if (getStateReligion() == ((ReligionTypes)iI))
			{
				iValue *= 4;
				iValue /= 3;
			} */
			// K-Mod
			if (iI == getStateReligion() && getReligionAnarchyLength() > 0)
			{
				if (AI_isFirstTech(getCurrentResearch()) || 
						isFocusWar()) // advc.105
						//GET_TEAM(getTeam()).getAnyWarPlanCount(true))
					iValue = iValue*3/2;
				else
					iValue = iValue*4/3;
			}
			// K-Mod end

			if (eFavorite == iI)
			{
				iValue *= 5;
				iValue /= 4;
			}

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				eBestReligion = ((ReligionTypes)iI);
			}
		}
	}

	if ((NO_RELIGION == eBestReligion) || AI_isDoStrategy(AI_STRATEGY_MISSIONARY))
	{
		return eBestReligion;
	}

	/* int iBestCount = getHasReligionCount(eBestReligion);
	int iSpreadPercent = (iBestCount * 100) / std::max(1, getNumCities());
	int iPurityPercent = (iBestCount * 100) / std::max(1, countTotalHasReligion());

	if (iPurityPercent < 49)
	{
		if (iSpreadPercent > ((eBestReligion == eFavorite) ? 65 : 75))
		{
			if (iPurityPercent > ((eBestReligion == eFavorite) ? 25 : 32))
			{
				return eBestReligion;
			}
		}
		return NO_RELIGION;
	} */ // disabled by K-Mod
	// K-Mod. Don't instantly convert to the first religion avaiable, unless it is your own religion.
	int iSpread = getHasReligionCount(eBestReligion) * 100 / std::min(GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities()*3/2+1, getNumCities()+1);
	if (getStateReligion() == NO_RELIGION && iSpread < 29 - AI_getFlavorValue(FLAVOR_RELIGION)
		&& (GC.getGameINLINE().getHolyCity(eBestReligion) == NULL || GC.getGameINLINE().getHolyCity(eBestReligion)->getTeam() != getTeam()))
	{
		return NO_RELIGION;
	}
	// K-Mod end
	
	return eBestReligion;
}

// This function has been completely rewriten for K-Mod
int CvPlayerAI::AI_religionValue(ReligionTypes eReligion) const
{
	PROFILE_FUNC();

	int iValue = 0;

	if (getHasReligionCount(eReligion) == 0)
		return 0;

	int iReligionFlavor = AI_getFlavorValue(FLAVOR_RELIGION);

	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iValue += pLoopCity->getReligionGrip(eReligion) * pLoopCity->getPopulation();
		// note. this takes into account current state religion and number of religious buildings.
	}
	iValue *= getHasReligionCount(eReligion);
	iValue /= getNumCities();

	// holy city modifier
	CvCity* pHolyCity = GC.getGameINLINE().getHolyCity(eReligion);
	if (pHolyCity != NULL && pHolyCity->getTeam() == getTeam())
	{
		iValue *= 110 + 5 * iReligionFlavor + (10+iReligionFlavor)*GC.getGameINLINE().calculateReligionPercent(eReligion)/10;
		iValue /= 100;
	}

	// diplomatic modifier
	int iTotalCivs = 0; // x2
	int iLikedReligionCivs = 0; // x2 for friendly
	for (int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		const CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
		if (i != getID() && kLoopPlayer.isAlive() && GET_TEAM(getTeam()).isHasMet(kLoopPlayer.getTeam()) && !kLoopPlayer.isMinorCiv())
		{
			iTotalCivs += 2;
			if (kLoopPlayer.getStateReligion() == eReligion)
			{
				AttitudeTypes eAttitude = AI_getAttitude((PlayerTypes)i, false);
				if (eAttitude >= ATTITUDE_PLEASED)
				{
					iLikedReligionCivs++;
					if (eAttitude >= ATTITUDE_FRIENDLY)
						iLikedReligionCivs++;
				}
			}
		}
	}

	// * up to +100% boost for liked civs having this has their state religion. (depending on personality)
	// * less potential boost if we have aspirations of being a religious leader
	// * minimum boost for apostolic palace religion
	const CvLeaderHeadInfo& kPersonality = GC.getLeaderHeadInfo(getPersonalityType());
	int iDiplomaticBase = 50 + 5 * range(kPersonality.getSameReligionAttitudeChangeLimit() - kPersonality.getDifferentReligionAttitudeChange(), 0, 10);
	// note. with the default xml, the highest the above number can get is 50 + 5*9 (Zara Yaqob). For most leaders its roughly 50 + 5*5. No one gets 100.
	// also, most civs with a high base modifier also have iReligionFlavor > 0; and so their final modifier will be reduced.
	int iDiplomaticModifier = 10 * iDiplomaticBase * iLikedReligionCivs / std::max(1, iTotalCivs);
	iDiplomaticModifier /= 10 + iReligionFlavor;
	// advc.131: AP matter even if diplo modifier already high
	//if (iDiplomaticModifier < iDiplomaticBase/3)
	{
		for (VoteSourceTypes i = (VoteSourceTypes)0; i < GC.getNumVoteSourceInfos(); i = (VoteSourceTypes)(i+1))
		{
			if (
				/*  advc.131: Shouldn't check if we're a member now when deciding
					whether we should be one. */
				//isLoyalMember(i) &&
				GC.getGameINLINE().isDiploVote(i) && GC.getGameINLINE().getVoteSourceReligion(i) == eReligion)
			{
				// advc.131: += instead of =
				iDiplomaticModifier += iDiplomaticBase/3;
			}
		}
	}
	iValue *= 100 + iDiplomaticModifier;
	iValue /= 100;

	return iValue;
}

/// \brief Value of espionage mission at this plot.
///
/// Assigns value to espionage mission against ePlayer at pPlot, where iData can provide additional information about mission.
// K-Mod note: A rough rule of thumb for this evaluation is that depriving the enemy of 1 commerce is worth 1 point; gaining 1 commerce for ourself is worth 2 points.
int CvPlayerAI::AI_espionageVal(PlayerTypes eTargetPlayer, EspionageMissionTypes eMission, CvPlot* pPlot, int iData) const
{
	TeamTypes eTargetTeam = GET_PLAYER(eTargetPlayer).getTeam();

	if (eTargetPlayer == NO_PLAYER)
	{
		return 0;
	}

	//int iCost = getEspionageMissionCost(eMission, eTargetPlayer, pPlot, iData);

	if (!canDoEspionageMission(eMission, eTargetPlayer, pPlot, iData, NULL))
	{
		return 0;
	}

	bool bMalicious = AI_isMaliciousEspionageTarget(eTargetPlayer);

	int iValue = 0;
	if (bMalicious && GC.getEspionageMissionInfo(eMission).isDestroyImprovement())
	{
		if (NULL != pPlot)
		{
			if (pPlot->getOwnerINLINE() == eTargetPlayer)
			{
				ImprovementTypes eImprovement = pPlot->getImprovementType();
				if (eImprovement != NO_IMPROVEMENT)
				{
					BonusTypes eBonus = pPlot->getNonObsoleteBonusType(GET_PLAYER(eTargetPlayer).getTeam());
					if (NO_BONUS != eBonus)
					{
						iValue += 2*GET_PLAYER(eTargetPlayer).AI_bonusVal(eBonus, -1); // was 1*
						
						int iTempValue = 0;
						if (NULL != pPlot->getWorkingCity())
						{
							iTempValue += (pPlot->calculateImprovementYieldChange(eImprovement, YIELD_FOOD, pPlot->getOwnerINLINE()) * 2);
							iTempValue += (pPlot->calculateImprovementYieldChange(eImprovement, YIELD_PRODUCTION, pPlot->getOwnerINLINE()) * 1);
							iTempValue += (pPlot->calculateImprovementYieldChange(eImprovement, YIELD_COMMERCE, pPlot->getOwnerINLINE()) * 2);
							iTempValue += GC.getImprovementInfo(eImprovement).getUpgradeTime() / 2;
							iValue += iTempValue;
						}
					}
				}
			}
		}
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getDestroyBuildingCostFactor() > 0)
	{
		FAssert(iData >= 0 && iData < GC.getNumBuildingInfos());
		if (canSpyDestroyBuilding(eTargetPlayer, (BuildingTypes)iData))
		{
			if (NULL != pPlot)
			{
				CvCity* pCity = pPlot->getPlotCity();

				if (NULL != pCity)
				{
					if (pCity->getNumRealBuilding((BuildingTypes)iData) > 0)
					{
						CvBuildingInfo& kBuilding = GC.getBuildingInfo((BuildingTypes)iData);
						// K-Mod
						if (!pCity->isDisorder()) // disorder messes up the evaluation of production and of building value. That's the only reason for this condition.
						{
							// Note: I'm not allowing recursion in the building evaluation.
							// This may cause the cached value to be inaccurate, but it doesn't really matter, because the building is already built!
							// (AI_buildingValue gives units of 4x commerce/turn)
							iValue += kBuilding.getProductionCost() / 2;
							iValue += (2 + pCity->getProductionTurnsLeft((BuildingTypes)iData, 1)) * pCity->AI_buildingValue((BuildingTypes)iData, 0, 0, false, false) / 5;
						}
						// K-Mod end
					}
				}
			}
		}
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getDestroyProjectCostFactor() > 0)
	{
		FAssert(iData >= 0 && iData < GC.getNumProjectInfos());
		if (canSpyDestroyProject(eTargetPlayer, (ProjectTypes)iData))
		{
			CvProjectInfo& kProject = GC.getProjectInfo((ProjectTypes)iData);
			
			iValue += getProductionNeeded((ProjectTypes)iData) * (kProject.getMaxTeamInstances() == 1 ? 8 : 6); // was 3 : 2
		}
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getDestroyProductionCostFactor() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();
			FAssert(pCity != NULL);
			if (pCity != NULL)
			{
				int iTempValue = pCity->getProduction();
				if (iTempValue > 0)
				{
					if (pCity->getProductionProject() != NO_PROJECT)
					{
						CvProjectInfo& kProject = GC.getProjectInfo(pCity->getProductionProject());
						iValue += iTempValue * ((kProject.getMaxTeamInstances() == 1) ? 6 : 3);	// was 4 : 2
					}
					else if (pCity->getProductionBuilding() != NO_BUILDING)
					{
						CvBuildingInfo& kBuilding = GC.getBuildingInfo(pCity->getProductionBuilding());
						if (isWorldWonderClass((BuildingClassTypes)kBuilding.getBuildingClassType()))
						{
							iValue += 3 * iTempValue;
						}
						iValue += iTempValue;
					}
					else
					{
						iValue += iTempValue;
					}
				}
			}
		}
	}


	if (bMalicious && (GC.getEspionageMissionInfo(eMission).getDestroyUnitCostFactor() > 0 || GC.getEspionageMissionInfo(eMission).getBuyUnitCostFactor() > 0) )
	{
		if (NULL != pPlot)
		{
			CvUnit* pUnit = GET_PLAYER(eTargetPlayer).getUnit(iData);

			if (NULL != pUnit)
			{
				UnitTypes eUnit = pUnit->getUnitType();

				iValue += GET_PLAYER(eTargetPlayer).AI_unitValue(eUnit, (UnitAITypes)GC.getUnitInfo(eUnit).getDefaultUnitAIType(), pUnit->area());

				if (GC.getEspionageMissionInfo(eMission).getBuyUnitCostFactor() > 0)
				{
					/*if (!canTrain(eUnit) || getProductionNeeded(eUnit) > iCost)
					{
						iValue += AI_unitValue(eUnit, (UnitAITypes)GC.getUnitInfo(eUnit).getDefaultUnitAIType(), pUnit->area());
					}*/
					// K-Mod. AI_unitValue is a relative rating value. It shouldn't be used for this.
					// (The espionage mission is not enabled anyway, so I'm not going to put a lot of effort into it.)
					iValue += GC.getUnitInfo(eUnit).getProductionCost() * (canTrain(eUnit) ? 4 : 8); // kmodx: Parentheses added; not sure if needed
					//
				}
			}
		}
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getStealTreasuryTypes() > 0)
	{
		if( pPlot != NULL && pPlot->getPlotCity() != NULL )
		{
			/* int iGoldStolen = (GET_PLAYER(eTargetPlayer).getGold() * GC.getEspionageMissionInfo(eMission).getStealTreasuryTypes()) / 100;
			iGoldStolen *= pPlot->getPlotCity()->getPopulation();
			iGoldStolen /= std::max(1, GET_PLAYER(eTargetPlayer).getTotalPopulation());
			iValue += ((GET_PLAYER(eTargetPlayer).AI_isFinancialTrouble() || AI_isFinancialTrouble()) ? 4 : 2) * (2 * std::max(0, iGoldStolen - iCost));*/
			// K-Mod
			int iGoldStolen = getEspionageGoldQuantity(eMission, eTargetPlayer, pPlot->getPlotCity());
			iValue += (GET_PLAYER(eTargetPlayer).AI_isFinancialTrouble() || AI_isFinancialTrouble() ? 6 : 4) * iGoldStolen;
		}
	}

	if (GC.getEspionageMissionInfo(eMission).getCounterespionageNumTurns() > 0)
	{
		//iValue += 100 * GET_TEAM(getTeam()).AI_getAttitudeVal(GET_PLAYER(eTargetPlayer).getTeam());

		// K-Mod (I didn't comment that line out, btw.)
		const TeamTypes eTeam = GET_PLAYER(eTargetPlayer).getTeam();
		const int iEra = getCurrentEra();
		int iCounterValue = 5 + 3*iEra + (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3 || AI_VICTORY_SPACE3) ? 20 : 0);
		// this is pretty bogus. I'll try to come up with something better some other time.
		iCounterValue *= 50*iEra*(iEra+1)/2 + GET_TEAM(eTeam).getEspionagePointsAgainstTeam(getTeam());
		iCounterValue /= std::max(1, 50*iEra*(iEra+1) + GET_TEAM(getTeam()).getEspionagePointsAgainstTeam(eTeam))/2;
		iCounterValue *= 
				// advc.130j:
				(int)ceil(AI_getMemoryCount(eTargetPlayer, MEMORY_SPY_CAUGHT) / 2.0)
				+ (GET_TEAM(getTeam()).isAtWar(eTeam)?2 :0) + (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4 | AI_VICTORY_SPACE3)?2 : 0);
		iValue += iCounterValue;
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getBuyCityCostFactor() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				iValue += AI_cityTradeVal(pCity);
			}
		}
	}

	if (GC.getEspionageMissionInfo(eMission).getCityInsertCultureAmountFactor() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();
			if (NULL != pCity)
			{
				if (pCity->getOwner() != getID())
				{
					int iCultureAmount = GC.getEspionageMissionInfo(eMission).getCityInsertCultureAmountFactor() * pPlot->getCulture(getID());
					iCultureAmount /= 100;
					/* if (pCity->calculateCulturePercent(getID()) > 40)
					{
						iValue += iCultureAmount * 3;
					}*/
					// K-Mod - both offensive & defensive use of spread culture mission. (The first "if" is really just for effeciency.)
					if (pCity->calculateCulturePercent(getID()) >= 8)
					{
						const CvCity* pOurClosestCity = GC.getMap().findCity(pPlot->getX(), pPlot->getY(), getID());
						if (pOurClosestCity != NULL)
						{
							int iDistance = pCity->cultureDistance(xDistance(pPlot->getX(), pOurClosestCity->getX()), yDistance(pPlot->getY(), pOurClosestCity->getY()));
							if (iDistance < 6)
							{
								int iPressure = std::max(pCity->AI_culturePressureFactor() - 100, pOurClosestCity->AI_culturePressureFactor());
								int iMultiplier = std::min(2, (6 - iDistance) * iPressure / 500);
								iValue += iCultureAmount * iMultiplier;
							}
						}
					}
					// K-Mod end
				}
			}
		}
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getCityPoisonWaterCounter() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				int iCityHealth = pCity->goodHealth() - pCity->badHealth(false, 0);
				//int iBaseUnhealth = GC.getEspionageMissionInfo(eMission).getCityPoisonWaterCounter();
				int iBaseUnhealth = GC.getEspionageMissionInfo(eMission).getCityPoisonWaterCounter() - std::max(pCity->getOccupationTimer(), GET_PLAYER(eTargetPlayer).getAnarchyTurns());

				// K-Mod: fixing some "wtf".
				/*
				int iAvgFoodShortage = std::max(0, iBaseUnhealth - iCityHealth) - pCity->foodDifference();
				iAvgFoodShortage += std::max(0, iBaseUnhealth/2 - iCityHealth) - pCity->foodDifference();
				
				iAvgFoodShortage /= 2;
				
				if( iAvgFoodShortage > 0 )
				{
					iValue += 8 * iAvgFoodShortage * iAvgFoodShortage;
				}*/
				int iAvgFoodShortage = (std::max(0, iBaseUnhealth - iCityHealth) + std::max(0, -iCityHealth))/2 - pCity->foodDifference(true, true);

				if (iAvgFoodShortage > 0)
				{
					iValue += 3 * iAvgFoodShortage * iBaseUnhealth;
					/*  advc.160: Prefer large cities w/o Granaries.
						Too little impact? iValue culd be sth. like 100,
						threshold e.g. 30, food kept 12. */
					iValue += pCity->growthThreshold() - pCity->getFoodKept();
				}
				// K-Mod end
			}
		}
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getCityUnhappinessCounter() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				int iCityCurAngerLevel = pCity->happyLevel() - pCity->unhappyLevel(0);
				//int iBaseAnger = GC.getEspionageMissionInfo(eMission).getCityUnhappinessCounter();
				// K-Mod
				int iBaseAnger = GC.getEspionageMissionInfo(eMission).getCityUnhappinessCounter() - std::max(pCity->getOccupationTimer(), GET_PLAYER(eTargetPlayer).getAnarchyTurns());
				// K-Mod end
				int iAvgUnhappy = iCityCurAngerLevel - iBaseAnger/2;
				
				if (iAvgUnhappy < 0)
				{
					iValue += 7 * abs(iAvgUnhappy) * iBaseAnger;// down from 14
				}
			}
		}
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getCityRevoltCounter() > 0)
	{
		// Handled elsewhere
	}

	if (GC.getEspionageMissionInfo(eMission).getBuyTechCostFactor() > 0)
	{
		FAssert(iData >= 0 && iData < GC.getNumTechInfos());

		//if (iCost < GET_TEAM(getTeam()).getResearchLeft((TechTypes)iData) * 4 / 3)
		if (canStealTech(eTargetPlayer, (TechTypes)iData)) // K-Mod!
		{
			//int iTempValue = GET_TEAM(getTeam()).AI_techTradeVal((TechTypes)iData, GET_PLAYER(eTargetPlayer).getTeam());
			// K-Mod
			int iTempValue = GET_TEAM(getTeam()).AI_techTradeVal((TechTypes)iData, GET_PLAYER(eTargetPlayer).getTeam());
			iTempValue *= AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE) ? 6 : 5;
			iTempValue /= 3;
			// K-Mod end

			iValue += iTempValue;
		}
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getSwitchCivicCostFactor() > 0)
	{
		iValue += AI_civicTradeVal((CivicTypes)iData, eTargetPlayer);
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getSwitchReligionCostFactor() > 0)
	{
		iValue += AI_religionTradeVal((ReligionTypes)iData, eTargetPlayer);
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getPlayerAnarchyCounter() > 0)
	{
		// AI doesn't use Player Anarchy
	}

	return iValue;
}

// K-Mod
bool CvPlayerAI::AI_isMaliciousEspionageTarget(PlayerTypes eTarget) const
{
	if (GET_PLAYER(eTarget).getTeam() == getTeam())
		return false;
	return
		// <advc.120b>: Replaceing ...
		//AI_getAttitude(eTarget) <= (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? ATTITUDE_PLEASED : ATTITUDE_CAUTIOUS) ||
		// ... with:
		GC.getLeaderHeadInfo(getPersonalityType()).getNoWarAttitudeProb(std::min(
				AI_getAttitude(eTarget) + 1, NUM_ATTITUDE_TYPES - 1)) < 100 ||
		// </advc.120b>
		GET_TEAM(getTeam()).AI_getWarPlan(GET_PLAYER(eTarget).getTeam()) != NO_WARPLAN ||
		(AI_isDoVictoryStrategyLevel4() && GET_PLAYER(eTarget).AI_isDoVictoryStrategyLevel4() && !AI_isDoVictoryStrategy(AI_VICTORY_DIPLOMACY1));
}
// K-Mod end

int CvPlayerAI::AI_getPeaceWeight() const
{
	return m_iPeaceWeight;
}


void CvPlayerAI::AI_setPeaceWeight(int iNewValue)
{
	m_iPeaceWeight = iNewValue;
}

int CvPlayerAI::AI_getEspionageWeight() const
{
	if (GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		return 0;
	}
	return m_iEspionageWeight;
}

void CvPlayerAI::AI_setEspionageWeight(int iNewValue)
{
	m_iEspionageWeight = iNewValue;
}


int CvPlayerAI::AI_getAttackOddsChange() const
{
	return m_iAttackOddsChange;
}


void CvPlayerAI::AI_setAttackOddsChange(int iNewValue)
{
	m_iAttackOddsChange = iNewValue;
}


int CvPlayerAI::AI_getCivicTimer() const
{
	return m_iCivicTimer;
}


void CvPlayerAI::AI_setCivicTimer(int iNewValue)
{
	m_iCivicTimer = iNewValue;
	FAssert(AI_getCivicTimer() >= 0);
}


void CvPlayerAI::AI_changeCivicTimer(int iChange)
{
	AI_setCivicTimer(AI_getCivicTimer() + iChange);
}


int CvPlayerAI::AI_getReligionTimer() const
{
	return m_iReligionTimer;
}


void CvPlayerAI::AI_setReligionTimer(int iNewValue)
{
	m_iReligionTimer = iNewValue;
	FAssert(AI_getReligionTimer() >= 0);
}


void CvPlayerAI::AI_changeReligionTimer(int iChange)
{
	AI_setReligionTimer(AI_getReligionTimer() + iChange);
}

int CvPlayerAI::AI_getExtraGoldTarget() const
{
	return m_iExtraGoldTarget;
}

void CvPlayerAI::AI_setExtraGoldTarget(int iNewValue)
{
	m_iExtraGoldTarget = iNewValue;
}

int CvPlayerAI::AI_getNumTrainAIUnits(UnitAITypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_UNITAI_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiNumTrainAIUnits[eIndex];
}


void CvPlayerAI::AI_changeNumTrainAIUnits(UnitAITypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_UNITAI_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiNumTrainAIUnits[eIndex] = (m_aiNumTrainAIUnits[eIndex] + iChange);
	FAssert(AI_getNumTrainAIUnits(eIndex) >= 0);
}


int CvPlayerAI::AI_getNumAIUnits(UnitAITypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_UNITAI_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiNumAIUnits[eIndex];
}


void CvPlayerAI::AI_changeNumAIUnits(UnitAITypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_UNITAI_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiNumAIUnits[eIndex] = (m_aiNumAIUnits[eIndex] + iChange);
	FAssert(AI_getNumAIUnits(eIndex) >= 0);
}


int CvPlayerAI::AI_getSameReligionCounter(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiSameReligionCounter[eIndex];
}


void CvPlayerAI::AI_changeSameReligionCounter(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiSameReligionCounter[eIndex] = (m_aiSameReligionCounter[eIndex] + iChange);
	FAssert(AI_getSameReligionCounter(eIndex) >= 0);
}


int CvPlayerAI::AI_getDifferentReligionCounter(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiDifferentReligionCounter[eIndex];
}


void CvPlayerAI::AI_changeDifferentReligionCounter(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiDifferentReligionCounter[eIndex] = (m_aiDifferentReligionCounter[eIndex] + iChange);
	FAssert(AI_getDifferentReligionCounter(eIndex) >= 0);
}


int CvPlayerAI::AI_getFavoriteCivicCounter(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiFavoriteCivicCounter[eIndex];
}


void CvPlayerAI::AI_changeFavoriteCivicCounter(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiFavoriteCivicCounter[eIndex] = (m_aiFavoriteCivicCounter[eIndex] + iChange);
	FAssert(AI_getFavoriteCivicCounter(eIndex) >= 0);
}


int CvPlayerAI::AI_getBonusTradeCounter(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiBonusTradeCounter[eIndex];
}


void CvPlayerAI::AI_changeBonusTradeCounter(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiBonusTradeCounter[eIndex] = (m_aiBonusTradeCounter[eIndex] + iChange);
	FAssert(AI_getBonusTradeCounter(eIndex) >= 0);
}


int CvPlayerAI::AI_getPeacetimeTradeValue(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiPeacetimeTradeValue[eIndex];
}


int CvPlayerAI::AI_getPeacetimeGrantValue(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiPeacetimeGrantValue[eIndex];
}

// <advc.130p> Merged the following two functions
void CvPlayerAI::AI_changePeacetimeTradeValue(PlayerTypes eIndex, int iChange) {
	AI_changePeaceTimeValue(eIndex, iChange, false);
}
void CvPlayerAI::AI_changePeacetimeGrantValue(PlayerTypes eIndex, int iChange) {
	AI_changePeaceTimeValue(eIndex, iChange, true);
}

// Based on the old AI_changePeacetimeGrantValue
void CvPlayerAI::AI_changePeaceTimeValue(PlayerTypes eIndex, int iChange,
		bool isGrant) {

	PROFILE_FUNC(); FAssert(eIndex >= 0); FAssert(eIndex < MAX_PLAYERS);
	if(iChange == 0)
		return;
	int* aiPeacetimeValue = isGrant ? m_aiPeacetimeGrantValue : m_aiPeacetimeTradeValue;
	CvGame& g = GC.getGameINLINE();
	/*  Trade values are higher in slow games b/c everything is more expensive.
		Game scores aren't much affected by speed. Adjust trade value to
		speed to make trade value comparable with score. */
	double val = (100.0 * iChange) /
			GC.getGameSpeedInfo(g.getGameSpeedType()).getResearchPercent();
	/*  Times 1000 for increased precision, times 2 for an average of the player
		scores, times 2 b/c "fair trade" is twice as sensitive as "traded with
		worst enemy" (it's this way too in BtS) */
	int incr = ::round((val * 4000)
			/*  BtS adjusted trade and grant values to the game progress only when
				computing the effect on relations. Now that progress is measured
				by score, it's better to do it here, based on the scores at the time
				that the trade happens. */
			/ (g.getPlayerScore(eIndex) + g.getPlayerScore(getID())));
	// <advc.130v>
	PlayerTypes masterId = NO_PLAYER;
	if(TEAMREF(eIndex).isCapitulated()) {
		incr /= 2;
		masterId = GET_TEAM(GET_PLAYER(eIndex).getMasterTeam()).getLeaderID();
		aiPeacetimeValue[masterId] = aiPeacetimeValue[masterId] + incr;
	} // </advc.130v>
	aiPeacetimeValue[eIndex] = aiPeacetimeValue[eIndex] + incr;
	FAssert(aiPeacetimeValue[eIndex] >= 0 && iChange > 0);
	if(iChange <= 0 || TEAMID(eIndex) == getTeam())
		return;
	for(int iI = 0; iI < MAX_CIV_TEAMS; iI++) {
		TeamTypes tId = (TeamTypes)iI;
		CvTeamAI& t = GET_TEAM(tId);
		if(tId != TEAMID(eIndex) && (masterId == NO_TEAM || tId != TEAMID(masterId)) &&
				t.isAlive() && t.AI_getWorstEnemy() == getTeam() &&
				t.isHasMet(TEAMID(eIndex))) { // unoffical patch bugfix, by Sephi.
			double mult = 2000;
			// Avoid oscillation of worst enemy
			int delta = std::max(0, t.AI_getAttitudeVal(TEAMID(eIndex)) -
					t.AI_getAttitudeVal(t.AI_getWorstEnemy()));
			mult *= delta / 5.0;
			int enemyIncr = ::round((val * mult) /
					(g.getTeamScore(getTeam()) + g.getTeamScore(tId)));
			if(enemyIncr <= 0)
				continue;
			// <advc.130v>
			if(TEAMREF(eIndex).isCapitulated() && !isGrant) { // Vassals don't make gifts
				enemyIncr /= 2;
				t.AI_changeEnemyPeacetimeTradeValue(TEAMID(masterId), enemyIncr);
			} // </advc.130v>
			if(isGrant)
				t.AI_changeEnemyPeacetimeGrantValue(TEAMID(eIndex), enemyIncr);
			else t.AI_changeEnemyPeacetimeTradeValue(TEAMID(eIndex), enemyIncr);
		}
	}
}

void CvPlayerAI::AI_setPeacetimeTradeValue(PlayerTypes eIndex, int iVal) {

	m_aiPeacetimeTradeValue[eIndex] = iVal;
}

void CvPlayerAI::AI_setPeacetimeGrantValue(PlayerTypes eIndex, int iVal) {

	m_aiPeacetimeGrantValue[eIndex] = iVal;
} // </advc.130p>

// <advc.130k>
void CvPlayerAI::AI_setSameReligionCounter(PlayerTypes eIndex, int iValue) {
	m_aiSameReligionCounter[eIndex] = iValue;
}
void CvPlayerAI::AI_setDifferentReligionCounter(PlayerTypes eIndex, int iValue) {
	m_aiDifferentReligionCounter[eIndex] = iValue;
}
void CvPlayerAI::AI_setFavoriteCivicCounter(PlayerTypes eIndex, int iValue) {
	m_aiFavoriteCivicCounter[eIndex] = iValue;
}
void CvPlayerAI::AI_setBonusTradeCounter(PlayerTypes eIndex, int iValue) {
	m_aiBonusTradeCounter[eIndex] = iValue;
} // </advc.130k>

// <advc.130x> 0: same religion, 1 differing religion, 2 fav civic
int CvPlayerAI::ideologyDiploLimit(PlayerTypes theyId, int mode) const {

	if(mode < 0 && mode > 2) {
		FAssert(false);
		return -1;
	}
	CvLeaderHeadInfo& lh = GC.getLeaderHeadInfo(getPersonalityType());
	int lhLimit;
	switch(mode) {
	case 0: lhLimit = lh.getSameReligionAttitudeChangeLimit(); break;
	case 1: lhLimit = lh.getDifferentReligionAttitudeChangeLimit(); break;
	case 2: lhLimit = lh.getFavoriteCivicAttitudeChangeLimit(); break;
	default: FAssert(false); lhLimit = 0;
	}
	lhLimit = abs(lhLimit);
	// Don't further reduce a limit of 1
	if(lhLimit <= 1) return lhLimit;
	int total = 0;
	int count = 0;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayer const& civ = GET_PLAYER((PlayerTypes)i);
		if(!civ.isAlive() || civ.isMinorCiv() || civ.getID() == getID() ||
				civ.getID() == theyId || GET_TEAM(civ.getTeam()).isCapitulated() ||
				!GET_TEAM(getTeam()).isHasMet(civ.getTeam()))
			continue;
		total++;
		if(mode == 0 && getStateReligion() != NO_RELIGION &&
				getStateReligion() == civ.getStateReligion())
			count++;
		else if(mode == 1 && getStateReligion() != NO_RELIGION &&
				civ.getStateReligion() != NO_RELIGION &&
				getStateReligion() != civ.getStateReligion())
			count++;
		else if(mode == 2) {
			CivicTypes fav = (CivicTypes)lh.getFavoriteCivic();
			if(fav != NO_CIVIC && isCivic(fav) && civ.isCivic(fav))
				count++;
		}
	}
	double ratio = 0.5;
	/*  Since we're not counting us and them, the total can be 0. Treat this case
		as if 50% of the other civs were sharing the ideology. */
	if(total > 0)
		ratio = count / (double)total;
	// 0.51: To make sure we round up if the ratio is 100%
	return lhLimit - ::round(0.51 * ratio * lhLimit);
}
// </advc.130x>

int CvPlayerAI::AI_getGoldTradedTo(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiGoldTradedTo[eIndex];
}


void CvPlayerAI::AI_changeGoldTradedTo(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiGoldTradedTo[eIndex] = (m_aiGoldTradedTo[eIndex] + iChange);
	FAssert(AI_getGoldTradedTo(eIndex) >= 0);
}


int CvPlayerAI::AI_getAttitudeExtra(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiAttitudeExtra[eIndex];
}


void CvPlayerAI::AI_setAttitudeExtra(PlayerTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	int iChange = iNewValue - m_aiAttitudeExtra[eIndex]; // K-Mod
	m_aiAttitudeExtra[eIndex] = iNewValue;
	// K-Mod
	if (iChange)
		AI_changeCachedAttitude(eIndex, iChange);
	// K-Mod end
}


void CvPlayerAI::AI_changeAttitudeExtra(PlayerTypes eIndex, int iChange)
{
	AI_setAttitudeExtra(eIndex, (AI_getAttitudeExtra(eIndex) + iChange));
}


bool CvPlayerAI::AI_isFirstContact(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_abFirstContact[eIndex];
}


void CvPlayerAI::AI_setFirstContact(PlayerTypes eIndex, bool bNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_abFirstContact[eIndex] = bNewValue;
}


int CvPlayerAI::AI_getContactTimer(PlayerTypes eIndex1, ContactTypes eIndex2) const
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex1 < MAX_PLAYERS, "eIndex1 is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex2 < NUM_CONTACT_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");
	return m_aaiContactTimer[eIndex1][eIndex2];
}


void CvPlayerAI::AI_changeContactTimer(PlayerTypes eIndex1, ContactTypes eIndex2, int iChange)
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex1 < MAX_PLAYERS, "eIndex1 is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex2 < NUM_CONTACT_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");
	m_aaiContactTimer[eIndex1][eIndex2] = (AI_getContactTimer(eIndex1, eIndex2) + iChange);
	FAssert(AI_getContactTimer(eIndex1, eIndex2) >= 0);
}


int CvPlayerAI::AI_getMemoryCount(PlayerTypes eIndex1, MemoryTypes eIndex2) const
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex1 < MAX_PLAYERS, "eIndex1 is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex2 < NUM_MEMORY_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");
	return m_aaiMemoryCount[eIndex1][eIndex2];
}


void CvPlayerAI::AI_changeMemoryCount(PlayerTypes eIndex1, MemoryTypes eIndex2, int iChange)
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex1 < MAX_PLAYERS, "eIndex1 is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex2 < NUM_MEMORY_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");
	int iAttitude = AI_getMemoryAttitude(eIndex1, eIndex2); // K-Mod
	m_aaiMemoryCount[eIndex1][eIndex2] += iChange;
	FAssert(AI_getMemoryCount(eIndex1, eIndex2) >= 0);
	AI_changeCachedAttitude(eIndex1, AI_getMemoryAttitude(eIndex1, eIndex2) - iAttitude); // K-Mod
}

// <advc.130j>
void CvPlayerAI::AI_rememberEvent(PlayerTypes civId, MemoryTypes mem) {

	int sign = 0; // 0 means it's a neutral event, 1 is good, -1 bad
	/*  Random events should have the effects that the event dialogs say they have.
		(E.g. "... you gain +1 attitude with ...")
		Don't want votes for hostile civs to improve relations much, at least not
		until the AI can figure out when a vote matters (often it doesn't). */
	if(mem != MEMORY_EVENT_GOOD_TO_US &&  MEMORY_EVENT_BAD_TO_US &&
			mem != MEMORY_VOTED_FOR_US) {
		int memAtt = GC.getLeaderHeadInfo(getPersonalityType()).getMemoryAttitudePercent(mem);
		if(memAtt < 0) sign = -1;
		if(memAtt > 0) sign = 1;
	}
	int delta = 2;
	// Need a finer granularity for DoW
	if(mem == MEMORY_DECLARED_WAR)
		delta = 3;
// This disables the bulk of change 130j:
#if 0
	// We're surprised by the actions of civId
	if((sign < 0 && (AI_getAttitude(civId) >= ATTITUDE_FRIENDLY ||
			// <advc.130o> Surprised by war despite tribute
			(mem == MEMORY_DECLARED_WAR && !isHuman() && GET_PLAYER(civId).isHuman() &&
			AI_getMemoryCount(civId, MEMORY_MADE_DEMAND) > 0))) || // </advc.130o>
			(sign > 0 && AI_getAttitude(civId) <= ATTITUDE_ANNOYED))
		delta++;
	// Just what we expected from civId
	if((sign > 0 && AI_getAttitude(civId) >= ATTITUDE_FRIENDLY) ||
			(sign < 0 && AI_getAttitude(civId) <= ATTITUDE_ANNOYED))
		delta--;
#endif
	// <advc.130y> Cap DoW penalty for vassals
	if(mem == MEMORY_DECLARED_WAR && (GET_TEAM(getTeam()).isAVassal() ||
			TEAMREF(civId).isAVassal()))
		delta = std::min(3, delta); // </advc.130y>
	AI_changeMemoryCount(civId, mem, delta);
	// <advc.130l>
	if(GC.getDefineINT("ENABLE_130L") <= 0)
		return;
	int const nPairs = 6;
	MemoryTypes coupledRequests[nPairs][2] = {
		{MEMORY_GIVE_HELP, MEMORY_REFUSED_HELP},
		{MEMORY_ACCEPT_DEMAND, MEMORY_REJECTED_DEMAND},
		{MEMORY_ACCEPTED_RELIGION, MEMORY_DENIED_RELIGION},
		{MEMORY_ACCEPTED_CIVIC, MEMORY_DENIED_CIVIC},
		{MEMORY_ACCEPTED_JOIN_WAR, MEMORY_DENIED_JOIN_WAR},
		{MEMORY_ACCEPTED_STOP_TRADING, MEMORY_DENIED_STOP_TRADING},
		/*  All the others are AI requests, so, this one is odd, and
			it decays very quickly anyway. */
		//{MEMORY_VOTED_FOR_US, MEMORY_VOTED_AGAINST_US},
		/*  Could let a +2 event erase e.g. half of a remembered -2 event,
			but a +2 event is reported as two +1 events to this function;
			the current implementation can't tell the difference. */
		//{MEMORY_EVENT_GOOD_TO_US, MEMORY_EVENT_BAD_TO_US},
		// Not worth complicating things for:
		//{MEMORY_DECLARED_WAR, MEMORY_INDEPENDENCE},
		// !! Update nPairs when adding/removing pairs
	};
	for(int i = 0; i < nPairs; i++)
		for(int j = 0; j < 2; j++)
			if(mem == coupledRequests[i][j]) {
				MemoryTypes other = coupledRequests[i][(j + 1) % 2];
				AI_changeMemoryCount(civId, other, -std::min(
						1, AI_getMemoryCount(civId, other)));
			} // </advc.130l>
} // </advc.130j>


// K-Mod. Note, unlike some other timers, this timer is internally stored as the turn number that the timer will expire.
// With this system, the timer doesn't have to be decremented every turn.
int CvPlayerAI::AI_getCityTargetTimer() const
{
	return std::max(0, m_iCityTargetTimer - GC.getGameINLINE().getGameTurn());
}

void CvPlayerAI::AI_setCityTargetTimer(int iTurns)
{
	m_iCityTargetTimer = GC.getGameINLINE().getGameTurn() + iTurns;
}
// K-Mod end

int CvPlayerAI::AI_calculateGoldenAgeValue(bool bConsiderRevolution) const
{
    int iValue;
    int iTempValue;
    int iI;

    iValue = 0;
    for (iI = 0; iI <  NUM_YIELD_TYPES; ++iI)
    {
		/* original bts code
        iTempValue = (GC.getYieldInfo((YieldTypes)iI).getGoldenAgeYield() * AI_yieldWeight((YieldTypes)iI));
        iTempValue /= std::max(1, (1 + GC.getYieldInfo((YieldTypes)iI).getGoldenAgeYieldThreshold())); */
		// K-Mod
		iTempValue = GC.getYieldInfo((YieldTypes)iI).getGoldenAgeYield() * AI_yieldWeight((YieldTypes)iI) * AI_averageYieldMultiplier((YieldTypes)iI);
		iTempValue /= 1 + GC.getYieldInfo((YieldTypes)iI).getGoldenAgeYieldThreshold();
		//
        iValue += iTempValue;
    }

    /* original bts code
	iValue *= getTotalPopulation();
    iValue *= GC.getGameINLINE().goldenAgeLength(); // original BtS code
    iValue /= 100; */
	// K-Mod
	iValue *= getTotalPopulation();
	iValue *= getGoldenAgeLength();
    iValue /= 10000;

	// K-Mod. Add some value if we would use the opportunity to switch civics
	// Note: this first "if" isn't necessary. It just saves us checking civics when we don't need to.
	if (bConsiderRevolution && getMaxAnarchyTurns() != 0 && !isGoldenAge() && getAnarchyModifier() + 100 > 0)
	{
		std::vector<CivicTypes> aeBestCivics(GC.getNumCivicOptionInfos());
		for (int iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
		{
			aeBestCivics[iI] = getCivics((CivicOptionTypes)iI);
		}

		for (int iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
		{
			int iCurrentValue = AI_civicValue(aeBestCivics[iI]);
			int iBestValue;
			CivicTypes eNewCivic = AI_bestCivic((CivicOptionTypes)iI, &iBestValue);
			
			// using a 10 percent thresold. (cf the higher threshold used in AI_doCivics)
			if (aeBestCivics[iI] != NO_CIVIC && 100*iBestValue > 110*iCurrentValue)
			{
				aeBestCivics[iI] = eNewCivic;
				if (gPlayerLogLevel > 0) logBBAI("      %S wants a golden age to switch to %S (value: %d vs %d)", getCivilizationDescription(0), GC.getCivicInfo(eNewCivic).getDescription(0), iBestValue, iCurrentValue);
			}
		}

		int iAnarchyLength = getCivicAnarchyLength(&aeBestCivics[0]);
		if (iAnarchyLength > 0)
		{
			// we would switch; so what is the negation of anarchy worth?
			for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
			{
				iTempValue = getCommerceRate((CommerceTypes)iI) * iAnarchyLength;
				iTempValue *= AI_commerceWeight((CommerceTypes)iI);
				iTempValue /= 100;
				iValue += iTempValue;
			}
			// production and GGP matter too, but I don't really want to try to evaluate them properly. Sorry.
			iValue += getTotalPopulation() * iAnarchyLength;
			// On the other hand, I'm ignoring the negation of maintenance cost.
		}
	}
	// K-Mod end

    return iValue;
}

// Protected Functions...

void CvPlayerAI::AI_doCounter()
{
	int iI=0, iJ=0; // advc.003
	double mult = 1 - GET_TEAM(getTeam()).getDiploDecay(); // advc.130k
	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{	// <advc.130k>
		PlayerTypes civId = (PlayerTypes)iI;
		CvPlayerAI const& civ = GET_PLAYER(civId);
		if(!civ.isAlive() || civ.getTeam() == getTeam() ||
				!GET_TEAM(getTeam()).isHasMet(civ.getTeam()))
			continue;
		CvTeamAI const& t = GET_TEAM(getTeam());
		if(getStateReligion() != NO_RELIGION &&
				getStateReligion() == civ.getStateReligion())
			AI_changeSameReligionCounter(civId, t.randomCounterChange());
		else AI_setSameReligionCounter(civId, (int)(
				mult * AI_getSameReligionCounter(civId)));
		CvLeaderHeadInfo& lh = GC.getLeaderHeadInfo(getPersonalityType());
		if(getStateReligion() != NO_RELIGION &&
				civ.getStateReligion() != NO_RELIGION &&
				getStateReligion() != civ.getStateReligion() &&
				// Delay religion hate if just met
				t.AI_getHasMetCounter(civ.getTeam()) >
				// This is 10 (given the XML value in BtS)
				2 * -lh.getDifferentReligionAttitudeDivisor())
			AI_changeDifferentReligionCounter(civId, t.randomCounterChange());
		else AI_setDifferentReligionCounter(civId, (int)(
				mult * AI_getDifferentReligionCounter(civId)));
		if(lh.getFavoriteCivic() != NO_CIVIC) {
			CivicTypes favCivic = (CivicTypes)lh.getFavoriteCivic();
			if(isCivic(favCivic) && civ.isCivic(favCivic))
				AI_changeFavoriteCivicCounter(civId, t.randomCounterChange());
			else AI_setFavoriteCivicCounter(civId, (int)(
				mult * AI_getFavoriteCivicCounter(civId)));
		} // <advc.130p>
		AI_setPeacetimeGrantValue(civId, (int)(
				mult * AI_getPeacetimeGrantValue(civId)));
		AI_setPeacetimeTradeValue(civId, (int)(
				mult * AI_getPeacetimeTradeValue(civId)));
		// </advc.130p>
		// <advc.149>
		int attitudeDiv = lh.getBonusTradeAttitudeDivisor();
		if(attitudeDiv <= 0)
			continue;
		//int iBonusImports = getNumTradeBonusImports(civId);
		// Same scale as the above, but taking into account utility.
		double bonusVal = bonusImportValue(civId);
		int c = AI_getBonusTradeCounter(civId);
		if(bonusVal <= c / (1.5 * attitudeDiv)) {
			/*  BtS decreases the BonusTradeCounter by 1 + civ.getNumCities() / 4,
				but let's just do exponential decay instead. */
			AI_setBonusTradeCounter(civId, (int)(
					mult * AI_getBonusTradeCounter(civId)));
		}
		else {
			double incr = bonusVal;
			if(bonusVal > 0.01) {
				CvCity* capital = getCapitalCity();
				double capBonuses = 0;
				if(capital != NULL) {
					capBonuses = std::max(0.0,
							capital->countUniqueBonuses() - bonusVal);
				}
				double exportable = 0;
				for(int j = 0; j < GC.getNumBonusInfos(); j++) {
					BonusTypes bt = (BonusTypes)j;
					int avail = civ.getNumAvailableBonuses(bt)
							+ civ.getBonusExport(bt);
					if(avail > 1)
						exportable += std::min(avail - 1, 3);
				} /* Mean of capBonuses and a multiple of exportable, but
					 no more than 1.5 times capBonuses. */
				double weight1 = (capBonuses + std::min(capBonuses * 2,
						3.1 * std::max(bonusVal, exportable))) / 2;
				/*  Rather than changing attitudeDiv in XML for every leader,
					do the fine-tuning here. */
				double weight2 = 345.0 / attitudeDiv;
				if(weight1 >= weight2)
					incr = (bonusVal / weight1) * weight2;
			}
			AI_changeBonusTradeCounter(civId, t.randomCounterChange(::round(
					1.25 * attitudeDiv * lh.getBonusTradeAttitudeChangeLimit()),
					incr / 2)); // Halved b/c it's binomial distrib. w/ 2 trials
			// <advc.036>
			int oldGoldTraded = AI_getGoldTradedTo(civId);
			int newGoldTraded = (int)(mult * oldGoldTraded);
			AI_changeGoldTradedTo(civId, newGoldTraded - oldGoldTraded);
			// </advc.036>
		} // </advc.149></advc.130k>
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			for (iJ = 0; iJ < NUM_CONTACT_TYPES; iJ++)
			{
				if (AI_getContactTimer(((PlayerTypes)iI), ((ContactTypes)iJ)) > 0)
				{
					AI_changeContactTimer(((PlayerTypes)iI), ((ContactTypes)iJ), -1);
				}
			}
		}
	}

	int baseWarAtt = GC.getDefineINT("AT_WAR_ATTITUDE_CHANGE"); // advc.130g
	// <advc.003>
	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		PlayerTypes civId = (PlayerTypes)iI; 
		if(!GET_PLAYER(civId).isAlive())
			continue; 
		for (iJ = 0; iJ < NUM_MEMORY_TYPES; iJ++) {
			MemoryTypes mId = (MemoryTypes)iJ;
			int c = AI_getMemoryCount(civId, mId);
			if(c <= 0 || GC.getLeaderHeadInfo(getPersonalityType()).
					getMemoryDecayRand(mId) <= 0)
				continue; // </advc.003>
			// <advc.144> No decay of MADE_DEMAND_RECENT while peace treaty
			if(mId == MEMORY_MADE_DEMAND_RECENT &&
					GET_TEAM(getTeam()).isForcePeace(TEAMID(civId)))
				continue; // </advc.144>
			// <advc.130r>
			if(GET_TEAM(getTeam()).isAtWar(TEAMID(civId))) {
				// No decay while war ongoing
				if(mId == MEMORY_DECLARED_WAR)
					continue;
				// Limited decay while war ongoing
				if((mId == MEMORY_CANCELLED_OPEN_BORDERS ||
						mId == MEMORY_CANCELLED_DEFENSIVE_PACT ||
						mId == MEMORY_CANCELLED_VASSAL_AGREEMENT) &&
						c <= 1)
					continue;
			}
			if(mId == MEMORY_DECLARED_WAR_ON_FRIEND &&
					atWarWithPartner(TEAMID(civId)))
				continue;
			// </advc.130r><advc.130j>
			/*  Need to decay at least twice as fast b/c each
				request now counts twice (on average). */
			double div = 2;
			// Finer granularity for DoW
			if(mId == MEMORY_DECLARED_WAR)
				div = 3;
			// <advc.104m> Faster decay of memory about human response to AI demand
			if(getWPAI.isEnabled() && (mId == MEMORY_REJECTED_DEMAND ||
					mId == MEMORY_ACCEPT_DEMAND))
				div *= (10 / 6.0); // 60% faster decay // </advc.104m>
			// <advc.130r> Faster yet if multiple requests remembered
			if(c > 3) // c==3 could stem from a single request
				div = 2 * std::sqrt(c / 2.0); // </advc.130r>
			int decayRand = GC.getLeaderHeadInfo(getPersonalityType()).
					getMemoryDecayRand(mId);
			CvGame& g = GC.getGameINLINE();
			// <advc.130r> Moderate game-speed adjustment
			decayRand *= GC.getGameSpeedInfo(g.getGameSpeedType()).
					getGoldenAgePercent();
			decayRand = ::round(decayRand / (100 * div)); // </advc.130r>
			if(decayRand != 0 && /*  advc.130r: Interpret 0 as no decay. Otherwise, you'd
									 have to delete the whole MemoryDecay entry from the
									 Leaderhead XML to prevent decay via XML. */
					g.getSorenRandNum(decayRand, "Memory Decay") == 0) // </advc.130j>
				AI_changeMemoryCount(civId, mId, -1);
		}
		// <advc.130g>
		int memRebuke = AI_getMemoryCount(civId, MEMORY_REJECTED_DEMAND);
		if((AI_getWarAttitude(civId) < baseWarAtt - 1 && memRebuke > 0 &&
				GET_TEAM(getTeam()).AI_isChosenWar(TEAMID(civId))) ||
				GET_TEAM(getTeam()).isAVassal())
			AI_changeMemoryCount(civId, MEMORY_REJECTED_DEMAND, -memRebuke);
			// </advc.130g>
	}
}

// <advc.149> Based on CvPlayer::getNumTradeBonusImports
double CvPlayerAI::bonusImportValue(PlayerTypes fromId) const {

	double r = 0;
	CvGame& g = GC.getGameINLINE(); int dummy=-1;
	for(CvDeal* d = g.firstDeal(&dummy); d != NULL; d = g.nextDeal(&dummy)) {
		CLinkList<TradeData> const* list = NULL;
		if(d->getFirstPlayer() == getID() && d->getSecondPlayer() == fromId)
			list = d->getSecondTrades();
		else if(d->getSecondPlayer() == getID() && d->getFirstPlayer() == fromId)
			list = d->getFirstTrades();
		if(list == NULL || CvDeal::isVassalTributeDeal(list))
			continue;
		CLLNode<TradeData>* node=NULL;
		for(node = list->head(); node != NULL; node = list->next(node)) {
			if(node->m_data.m_eItemType == TRADE_RESOURCES) {
				r += GET_PLAYER(getID()).AI_bonusVal((BonusTypes)
						node->m_data.m_iData, -1);
			}
		}
	}
	// The divisor should be the typical value of a somewhat useful bonus
	return r / 2.5;
} // </advc.149>

void CvPlayerAI::AI_doMilitary()
{
	/* original bts code
	if (GET_TEAM(getTeam()).getAnyWarPlanCount(true) == 0)
	{
		while (AI_isFinancialTrouble() && (calculateUnitCost() > 0))
		{
			if (!AI_disbandUnit(1, false))
			{
				break;
			}
		}
	} */
	// K-Mod
	PROFILE_FUNC();

	if (!isAnarchy() && AI_isFinancialTrouble() && GET_TEAM(getTeam()).AI_getWarSuccessRating() > -30)
	{
		int iCost = AI_unitCostPerMil();
		const int iMaxCost = AI_maxUnitCostPerMil();

		if (iCost > 0 && (iCost > iMaxCost || calculateGoldRate() < 0))
		{
			std::vector<std::pair<int, int> > unit_values; // <value, id>
			int iLoop;
			for (CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
			{
				int iValue = AI_disbandValue(pLoopUnit);
				if (iValue < MAX_INT)
					unit_values.push_back(std::make_pair(iValue, pLoopUnit->getID()));
			}
			std::sort(unit_values.begin(), unit_values.end());

			size_t iDisbandCount = 0;
			do
			{
				if (unit_values.size() <= iDisbandCount)
					break;

				CvUnit* pDisbandUnit = getUnit(unit_values[iDisbandCount].second);
				FAssert(pDisbandUnit);
				if (gUnitLogLevel > 2)
				{
					CvWString aiTypeString;
					getUnitAIString(aiTypeString, pDisbandUnit->AI_getUnitAIType());
					logBBAI("    %S scraps '%S' %S, at (%d, %d), with value %d, due to financial trouble.", getCivilizationDescription(0), aiTypeString.GetCString(), pDisbandUnit->getName(0).GetCString(), pDisbandUnit->getX_INLINE(), pDisbandUnit->getY_INLINE(), unit_values[iDisbandCount].first);
				}

				pDisbandUnit->scrap();
				pDisbandUnit->doDelayedDeath();
				iDisbandCount++;

				iCost = AI_unitCostPerMil();
			} while (iCost > 0 && (iCost > iMaxCost || calculateGoldRate() < 0) && AI_isFinancialTrouble()
				&& iDisbandCount <= 2); // advc.110: Disband at most 2 units per turn.
		}
	}
	// K-Mod end

	AI_setAttackOddsChange(GC.getLeaderHeadInfo(getPersonalityType()).getBaseAttackOddsChange() +
		GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getAttackOddsChangeRand(), "AI Attack Odds Change #1") +
		GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getAttackOddsChangeRand(), "AI Attack Odds Change #2"));
}


void CvPlayerAI::AI_doResearch()
{
	FAssertMsg(!isHuman(), "isHuman did not return false as expected");

	//if (getCurrentResearch() == NO_TECH)
	if (getCurrentResearch() == NO_TECH && isResearch() && !isAnarchy()) // K-Mod
	{
		AI_chooseResearch();
		//AI_forceUpdateStrategies(); //to account for current research.
	}
}

// This function has been heavily edited for K-Mod. (Most changes are unmarked. Lots of code has been rearranged / deleted.
void CvPlayerAI::AI_doCommerce()
{
	FAssertMsg(!isHuman(), "isHuman did not return false as expected");

	if (isBarbarian())
	{
		return;
	}

	const static int iCommerceIncrement = GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS");
	CvTeamAI& kTeam = GET_TEAM(getTeam());

	int iGoldTarget = AI_goldTarget();
	/*  <advc.550f> Some extra gold for trade. Don't put this into AI_goldTarget
		for performance reasons, and b/c AI_maxGoldForTrade shouldn't take this
		extra budget into account. */
	if(!AI_isFinancialTrouble() && !AI_isDoVictoryStrategyLevel4()) {
		bool isTechTr = GET_TEAM(getTeam()).isTechTrading();
		double partnerScore = 0;
		int partners = 0;
		for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
			CvPlayerAI const& civ = GET_PLAYER((PlayerTypes)i);
			if(!civ.isAlive() || civ.getID() == getID() || !GET_TEAM(getTeam()).
					isHasMet(civ.getTeam()) || GET_TEAM(getTeam()).
					// Don't stockpile gold for reparations; pay gpt instead.
					isAtWar(civ.getTeam()))
				continue;
			if(!isTechTr && !GET_TEAM(civ.getTeam()).isTechTrading())
				continue;
			if(civ.getCurrentEra() < getCurrentEra() || (!civ.isHuman() && 
					civ.AI_getAttitude(getID()) <= GC.getLeaderHeadInfo(
					civ.getPersonalityType()).getTechRefuseAttitudeThreshold()))
				continue;
			partnerScore += std::sqrt((double)civ.getTotalPopulation()) *
					civ.getCurrentEra() * 10;
			partners++;
		}
		if(partners > 0) {
			int iScore = ::round(partnerScore / std::sqrt((double)partners));
			iGoldTarget = std::max(iGoldTarget, iScore);
		}
	} // </advc.550f>
	int iTargetTurns = 4 * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getResearchPercent();
	iTargetTurns /= 100;
	iTargetTurns = std::max(3, iTargetTurns);
	// K-Mod. make it faster on the way down
	bool bFinancialTrouble = AI_isFinancialTrouble();
	if (!bFinancialTrouble && getGold() > iGoldTarget)
		iTargetTurns = (iTargetTurns + 1)/2;
	// K-Mod end

	bool bReset = false;

	if (isCommerceFlexible(COMMERCE_CULTURE))
	{
		if (getCommercePercent(COMMERCE_CULTURE) > 0)
		{
			setCommercePercent(COMMERCE_CULTURE, 0);

			bReset = true;
		}
	}

	if (isCommerceFlexible(COMMERCE_ESPIONAGE))
	{
		if (getCommercePercent(COMMERCE_ESPIONAGE) > 0)
		{
			setCommercePercent(COMMERCE_ESPIONAGE, 0);

			bReset = true;
		}
	}

	if (bReset)
	{
		AI_assignWorkingPlots();
	}

	const bool bFirstTech = AI_isFirstTech(getCurrentResearch());
	const bool bNoResearch = isNoResearchAvailable();

	if (isCommerceFlexible(COMMERCE_CULTURE))
	{
		if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4))
		{
			setCommercePercent(COMMERCE_CULTURE, 100);
		}
		else if (getNumCities() > 0)
		{
			int iIdealPercent = 0;

			int iLoop;
			for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
			{
				if (pLoopCity->getCommerceHappinessPer(COMMERCE_CULTURE) > 0)
				{
					iIdealPercent += ((pLoopCity->angryPopulation() * 100) / pLoopCity->getCommerceHappinessPer(COMMERCE_CULTURE));
				}
			}

			iIdealPercent /= getNumCities();

			// K-Mod
			int iCap = 20;
			if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2) && !bFirstTech)
			{
				iIdealPercent+=5;
				iCap += 10;
			}
			if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) && !bFirstTech)
			{
				iIdealPercent+=5;
				iCap += 20;
			}
			iIdealPercent += (AI_averageCulturePressure() - 100)/10;
			iIdealPercent = std::max(iIdealPercent, (AI_averageCulturePressure() - 100)/5);
			if (AI_avoidScience())
			{
				iCap += 30;
				iIdealPercent *= 2;
			}
			iIdealPercent -= iIdealPercent % iCommerceIncrement;
			iIdealPercent = std::min(iIdealPercent, iCap);
			// K-Mod end

			setCommercePercent(COMMERCE_CULTURE, iIdealPercent);
		}
	}

	if (isCommerceFlexible(COMMERCE_RESEARCH) && !bNoResearch)
	{
		setCommercePercent(COMMERCE_RESEARCH, 100-getCommercePercent(COMMERCE_CULTURE));

		if (!AI_avoidScience() && !AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4))
		{
			// If we can afford to run full science for the rest of our current research,
			// then reduce our gold target so that we can burn through our gold.
			int iGoldRate = calculateGoldRate();
			if (iGoldRate < 0)
			{
				TechTypes eCurrentResearch = getCurrentResearch();
				if (eCurrentResearch != NO_TECH)
				{
					int iResearchTurnsLeft = getResearchTurnsLeft(eCurrentResearch, true);

					if (getGold() >= iResearchTurnsLeft * -iGoldRate)
						iGoldTarget /= 5;
				}
			}
		}
	}

	// Note, in the original code has gold marked as inflexible, but was effectly adjusted by changing the other commerce percentages. This is no longer the case.
	if (isCommerceFlexible(COMMERCE_GOLD))
	{
		// note: as we increase the gold rate, research will be reduced before culture.
		// (The order defined in CommerceTypes is the order that the types will be decreased to accomdate changes.)
		bool bValid = true;
		while (bValid && getCommercePercent(COMMERCE_GOLD) < 100 && getGold() + iTargetTurns * calculateGoldRate() < iGoldTarget)
		{
			bValid = changeCommercePercent(COMMERCE_GOLD, iCommerceIncrement);
		}
	}

	// K-Mod - evaluate potential espionage missions to determine espionage weights, even if espionage isn't flexible. (loosely based on bbai)
	if (!GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE) && kTeam.getHasMetCivCount(true) > 0 && (isCommerceFlexible(COMMERCE_ESPIONAGE) || getCommerceRate(COMMERCE_ESPIONAGE) > 0))
	{
		int iEspionageTargetRate = 0;
		std::vector<int> aiTarget(MAX_CIV_TEAMS, 0);
		std::vector<int> aiWeight(MAX_CIV_TEAMS, 0);
		int iHighestTarget = 0;
		int iMinModifier = INT_MAX;
		int iApproxTechCost = 0;
		TeamTypes eMinModTeam = NO_TEAM;
		bool bFocusEspionage = AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE) &&
				!isFocusWar(); // advc.105
				//kTeam.getAnyWarPlanCount(true) == 0;

		// For this part of the AI, I will assume there is only mission for each purpose.
		EspionageMissionTypes eSeeDemographicsMission = NO_ESPIONAGEMISSION;
		EspionageMissionTypes eSeeResearchMission = NO_ESPIONAGEMISSION;
		EspionageMissionTypes eStealTechMission = NO_ESPIONAGEMISSION;
		EspionageMissionTypes eCityRevoltMission = NO_ESPIONAGEMISSION;

		for (int iMission = 0; iMission < GC.getNumEspionageMissionInfos(); ++iMission)
		{
			CvEspionageMissionInfo& kMissionInfo = GC.getEspionageMissionInfo((EspionageMissionTypes)iMission);

			if (kMissionInfo.isSeeDemographics())
				eSeeDemographicsMission = (EspionageMissionTypes)iMission;

			if (kMissionInfo.isSeeResearch())
				eSeeResearchMission = (EspionageMissionTypes)iMission;

			if (kMissionInfo.getBuyTechCostFactor() != 0)
				eStealTechMission = (EspionageMissionTypes)iMission;

			if (kMissionInfo.getCityRevoltCounter() > 0)
				eCityRevoltMission = (EspionageMissionTypes)iMission;
		}

		for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
		{
			CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
			if (kLoopTeam.isAlive() && iTeam != getTeam() && kTeam.isHasMet((TeamTypes)iTeam) &&
				!kLoopTeam.isVassal(getTeam()) && !kTeam.isVassal((TeamTypes)iTeam))
			{
				int iTheirEspPoints = kLoopTeam.getEspionagePointsAgainstTeam(getTeam());
				int iOurEspPoints = kTeam.getEspionagePointsAgainstTeam((TeamTypes)iTeam);
				int iDesiredMissionPoints = 0;
				int iAttitude = range(kTeam.AI_getAttitudeVal((TeamTypes)iTeam), -12, 12);
				WarPlanTypes eWarPlan = kTeam.AI_getWarPlan((TeamTypes)iTeam);

				aiWeight[iTeam] = 6;
				int iRateDivisor = iTargetTurns*3;

				if (eWarPlan != NO_WARPLAN)
				{
					int iMissionCost = std::max(getEspionageMissionCost(eSeeResearchMission, kLoopTeam.getLeaderID()), getEspionageMissionCost(eSeeDemographicsMission, kLoopTeam.getLeaderID()));
					if (eWarPlan != WARPLAN_DOGPILE && AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE) && getCapitalCity())
					{
						CvCity* pTargetCity = getCapitalCity()->area()->getTargetCity(getID());
						if (pTargetCity && pTargetCity->getTeam() == iTeam && pTargetCity->area()->getAreaAIType(getTeam()) != AREAAI_DEFENSIVE && pTargetCity->area()->getNumAIUnits(getID(), UNITAI_SPY) > 0)
						{
							iMissionCost = std::max(iMissionCost, getEspionageMissionCost(eCityRevoltMission, pTargetCity->getOwnerINLINE(), pTargetCity->plot()));
							if (eWarPlan == WARPLAN_TOTAL || eWarPlan == WARPLAN_PREPARING_TOTAL)
							{
								iRateDivisor -= (iTargetTurns+2) / 4;
							}
						}
						else
							iMissionCost = std::max(iMissionCost, getEspionageMissionCost(eCityRevoltMission, kLoopTeam.getLeaderID()));
					}

					iMissionCost *= 12;
					iMissionCost /= 10;

					iDesiredMissionPoints = std::max(iDesiredMissionPoints, iMissionCost);

					iRateDivisor -= iTargetTurns/2;
					aiWeight[iTeam] += 6;

				}
				else
				{
					if (iAttitude <= -3)
					{
						int iMissionCost = std::max(getEspionageMissionCost(eSeeResearchMission, kLoopTeam.getLeaderID()), getEspionageMissionCost(eSeeDemographicsMission, kLoopTeam.getLeaderID()));
						iMissionCost *= 11;
						iMissionCost /= 10;
						iDesiredMissionPoints = std::max(iDesiredMissionPoints, iMissionCost);
					}
					else if (iAttitude < 3)
					{
						int iMissionCost = getEspionageMissionCost(eSeeDemographicsMission, kLoopTeam.getLeaderID());
						iMissionCost *= 11;
						iMissionCost /= 10;
						iDesiredMissionPoints = std::max(iDesiredMissionPoints, iMissionCost);
					}
				}

				iRateDivisor += 4*range(iAttitude, -8, 8)/(5*iTargetTurns); // + or - 1 point, with the standard target turns.
				iRateDivisor += AI_totalUnitAIs(UNITAI_SPY) == 0 ? 4 : 0;
				aiWeight[iTeam] -= (iAttitude/3);
				if (kTeam.AI_hasCitiesInPrimaryArea((TeamTypes)iTeam))
				{
					aiWeight[iTeam] *= 2;
					aiWeight[iTeam] = std::max(0, aiWeight[iTeam]);
					iRateDivisor -= iTargetTurns/2;
				}
				else
					iRateDivisor += iTargetTurns/2;

				// Individual player targeting
				if (bFocusEspionage)
				{
					for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
					{
						CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
						// <advc.003>
						if (kLoopPlayer.getTeam() != iTeam || kLoopPlayer.getNumCities() <= 0)
							continue; // </advc.003>
						std::vector<int> cityModifiers;
						CvCity* pLoopCity;
						int iLoop;
						int iTargetCities = 0;
						for (pLoopCity = kLoopPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kLoopPlayer.nextCity(&iLoop))
						{
							if (pLoopCity->isRevealed(getTeam(), false) && pLoopCity->area() != NULL && AI_isPrimaryArea(pLoopCity->area()))
							{
								cityModifiers.push_back(getEspionageMissionCostModifier(NO_ESPIONAGEMISSION, (PlayerTypes)iPlayer, pLoopCity->plot()));
							}
						}
						if (cityModifiers.size() > 0)
						{
							// Get the average of the lowest 3 cities.
							int iSampleSize = std::min(3, (int)cityModifiers.size());
							std::partial_sort(cityModifiers.begin(), cityModifiers.begin()+iSampleSize, cityModifiers.end());
							int iModifier = 0;
							for (std::vector<int>::iterator it = cityModifiers.begin(); it != cityModifiers.begin()+iSampleSize; ++it)
							{
								iModifier += *it;
							}
							iModifier /= iSampleSize;

							if (iModifier < iMinModifier ||
								(iModifier == iMinModifier && iAttitude < kTeam.AI_getAttitudeVal(eMinModTeam)))
							{
								// do they have any techs we can steal?
								bool bValid = false;
								for (int iT = 0; iT < GC.getNumTechInfos(); iT++)
								{
									if (canStealTech((PlayerTypes)iPlayer, (TechTypes)iT))
									{
										bValid = iApproxTechCost > 0; // don't set it true unless there are at least 2 stealable techs.
										// get a (very rough) approximation of how much it will cost to steal a tech.
										iApproxTechCost = (kTeam.getResearchCost((TechTypes)iT) + iApproxTechCost) / (iApproxTechCost != 0 ? 2 : 1);
										break;
									}
								}
								if (bValid)
								{
									iMinModifier = iModifier;
									eMinModTeam = (TeamTypes)iTeam;
								}
							}
						}
					}
				}
				if (iTheirEspPoints > iDesiredMissionPoints
					&& kTeam.AI_getAttitude((TeamTypes)iTeam) <= ATTITUDE_CAUTIOUS)
				{
					iRateDivisor += 1;
					// adjust their points based on our current relationship
					iTheirEspPoints *= 34 + (eWarPlan == NO_WARPLAN ? -iAttitude : 12);
					iTheirEspPoints /= 36; // note. the scale here set by the range of iAttitude. [-12, 12]

					// scale by esp points ever, so that we don't fall over ourselves trying to catch up with
					// a civ that happens to be running an espionage economy.
					int iOurTotal = std::max(4, kTeam.getEspionagePointsEver());
					int iTheirTotal = std::max(4, kLoopTeam.getEspionagePointsEver());

					iTheirEspPoints *= (3*iOurTotal + iTheirTotal);
					iTheirEspPoints /= (2*iOurTotal + 4*iTheirTotal);
					// That's a factor between 3/2 and 1/4, centered at 4/6
					iDesiredMissionPoints = std::max(iTheirEspPoints, iDesiredMissionPoints);
				}

				aiTarget[iTeam] = (iDesiredMissionPoints - iOurEspPoints)/std::max(3*iTargetTurns/2,iRateDivisor);

				iEspionageTargetRate += std::max(0, aiTarget[iTeam]);
			}
		}

		// now, the big question is whether or not we can steal techs more easilly than we can research them.
		bool bCheapTechSteal = false;
		if (eMinModTeam != NO_TEAM && !AI_avoidScience() && !bNoResearch && !AI_isDoVictoryStrategy(AI_VICTORY_SPACE3))
		{
			if (eStealTechMission != NO_ESPIONAGEMISSION)
			{
				iMinModifier *= 100 - GC.getDefineINT("MAX_FORTIFY_TURNS") * GC.getDefineINT("ESPIONAGE_EACH_TURN_UNIT_COST_DECREASE");
				iMinModifier /= 100;
				iMinModifier *= 100 + GC.getEspionageMissionInfo(eStealTechMission).getBuyTechCostFactor();
				iMinModifier /= 100;
				// This is the espionage cost modifier for stealing techs.

				// lets say "cheap" means 75% of the research cost.
				bCheapTechSteal = ((AI_isDoStrategy(AI_STRATEGY_ESPIONAGE_ECONOMY) ? 8000 : 7500) * AI_averageCommerceMultiplier(COMMERCE_ESPIONAGE) / std::max(1, iMinModifier) > AI_averageCommerceMultiplier(COMMERCE_RESEARCH)*calculateResearchModifier(getCurrentResearch()));
				if (bCheapTechSteal)
				{
					aiTarget[eMinModTeam] += iApproxTechCost / 10;
					iEspionageTargetRate += iApproxTechCost / 10;
					// I'm just using iApproxTechCost to get a rough sense of scale.
					// cf. (iDesiredEspPoints - iOurEspPoints)/std::max(6,iRateDivisor);
				}
			}
		}


		for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
		{
			if (aiTarget[iTeam] > 0)
			{
				//aiWeight[iTeam] += (150*aiTarget[iTeam])/std::max(4,iEspionageTargetRate);
				aiWeight[iTeam] += (100*aiTarget[iTeam])/std::max(4,iEspionageTargetRate);
			}
			else if (aiTarget[iTeam] < 0)
			{
				if (iTeam != eMinModTeam &&
					(!AI_isMaliciousEspionageTarget(GET_TEAM((TeamTypes)iTeam).getLeaderID()) || !kTeam.AI_hasCitiesInPrimaryArea((TeamTypes)iTeam)))
					aiWeight[iTeam] = 0;
			}
			if (iTeam == eMinModTeam)
			{
				aiWeight[iTeam] = std::max(1, aiWeight[iTeam]);
				aiWeight[iTeam] *= 3;
				aiWeight[iTeam] /= 2;
			}
			else if (eMinModTeam != NO_TEAM && bCheapTechSteal)
			{
				aiWeight[iTeam] /= 2; // we want to focus hard on the best target
			}
			// note. bounds checks are done the set weight function
			setEspionageSpendingWeightAgainstTeam((TeamTypes)iTeam, aiWeight[iTeam]);
			// one last ad-hoc correction to the target espionage rate...
			if (aiWeight[iTeam] <= 0 && aiTarget[iTeam] > 0)
				iEspionageTargetRate -= aiTarget[iTeam];
		}
		// K-Mod end

		// K-Mod. Only try to control espionage rate if we can also control research,
		// and if we don't need our commerce for more important things.
		if (isCommerceFlexible(COMMERCE_ESPIONAGE) && isCommerceFlexible(COMMERCE_RESEARCH)
			&& !bFinancialTrouble && !bFirstTech && !AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4))
		{
			if (!bCheapTechSteal) // K-Mod
			{
				iEspionageTargetRate *= (140 - getCommercePercent(COMMERCE_GOLD) * 2); // was 110 -
				iEspionageTargetRate /= 140; // was 110
			}
			/*  advc.001: was getLeaderType. Makes a difference when playing with
				random AI personalities. */
			iEspionageTargetRate *= GC.getLeaderHeadInfo(getPersonalityType()).getEspionageWeight();
			iEspionageTargetRate /= 100;

			int iCap = AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE) ? 20 : 10;
			iCap += bCheapTechSteal || AI_avoidScience() ? 50 : 0;

			FAssert(iCap <= 100);
			iCap = std::min(iCap, 100); // just in case.

			bool bValid = true;
			while (bValid && getCommerceRate(COMMERCE_ESPIONAGE) < iEspionageTargetRate && getCommercePercent(COMMERCE_ESPIONAGE) < iCap)
			{
				changeCommercePercent(COMMERCE_RESEARCH, -iCommerceIncrement);
				bValid = changeCommercePercent(COMMERCE_ESPIONAGE, iCommerceIncrement);

				if (getGold() + iTargetTurns * calculateGoldRate() < iGoldTarget)
				{
					break;
				}

				if (!AI_avoidScience() && !bCheapTechSteal)
				{
					if (getCommercePercent(COMMERCE_RESEARCH) * 2 <= (getCommercePercent(COMMERCE_ESPIONAGE) + iCommerceIncrement) * 3)
					{
						break;
					}
				}
			}
		}
	}
	// K-Mod. prevent the AI from stockpiling excessive amounts of gold while in avoidScience.
	if (AI_avoidScience() && isCommerceFlexible(COMMERCE_GOLD))
	{
		while (getCommercePercent(COMMERCE_GOLD) > 0 && getGold() + std::min(0, calculateGoldRate()) > iGoldTarget)
		{
			if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) && isCommerceFlexible(COMMERCE_CULTURE))
				changeCommercePercent(COMMERCE_CULTURE, iCommerceIncrement);
			else if (isCommerceFlexible(COMMERCE_ESPIONAGE))
				changeCommercePercent(COMMERCE_ESPIONAGE, iCommerceIncrement);
			else if (isCommerceFlexible(COMMERCE_RESEARCH) && !bNoResearch) // better than nothing...
				changeCommercePercent(COMMERCE_RESEARCH, iCommerceIncrement);
			else
				break;
		}
	}
	// K-Mod end
	
	//if (!bFirstTech && (getGold() < iGoldTarget) && getCommercePercent(COMMERCE_RESEARCH) > 40)
	if (!bFirstTech && isCommerceFlexible(COMMERCE_GOLD) && (getGold() < iGoldTarget) && getCommercePercent(COMMERCE_GOLD) < 50) // K-Mod
	{
		bool bHurryGold = false;
		for (int iHurry = 0; iHurry < GC.getNumHurryInfos(); iHurry++)
		{
			if ((GC.getHurryInfo((HurryTypes)iHurry).getGoldPerProduction() > 0) && canHurry((HurryTypes)iHurry))
			{
				bHurryGold = true;
				break;				
			}
		}
		if (bHurryGold)
		{
			if (getCommercePercent(COMMERCE_ESPIONAGE) > 0 && !AI_isDoStrategy(AI_STRATEGY_ESPIONAGE_ECONOMY) && isCommerceFlexible(COMMERCE_ESPIONAGE))
			{
				changeCommercePercent(COMMERCE_ESPIONAGE, -iCommerceIncrement);			
			}
			else if (isCommerceFlexible(COMMERCE_RESEARCH))
			{
				changeCommercePercent(COMMERCE_RESEARCH, -iCommerceIncrement);			
			}
			// note. gold is automatically increased when other types are decreased.
		}
	}
	
	// this is called on doTurn, so make sure our gold is high enough keep us above zero gold.
	verifyGoldCommercePercent();
}

// K-Mod. I've rewriten most of this function, based on edits from BBAI. I don't know what's original bts code and what's not.
// (the BBAI implementation had some bugs)
void CvPlayerAI::AI_doCivics()
{
	FAssertMsg(!isHuman(), "isHuman did not return false as expected");

	if( isBarbarian() )
	{
		return;
	}

	if (AI_getCivicTimer() > 0)
	{
		AI_changeCivicTimer(-1);
		if (getGoldenAgeTurns() != 1) // K-Mod. If its the last turn of a golden age, consider switching civics anyway.
		{
			return;
		}
	}

	if (!canRevolution(NULL))
	{
		return;
	}

	// FAssertMsg(AI_getCivicTimer() == 0, "AI Civic timer is expected to be 0"); // Disabled by K-Mod

	std::vector<CivicTypes> aeBestCivic(GC.getNumCivicOptionInfos());
	std::vector<int> aiCurrentValue(GC.getNumCivicOptionInfos());

	for (int iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
	{
		aeBestCivic[iI] = getCivics((CivicOptionTypes)iI);
		aiCurrentValue[iI] = AI_civicValue(aeBestCivic[iI]);
	}

	int iAnarchyLength = 0;
	bool bWillSwitch;
	bool bWantSwitch;
	bool bFirstPass = true;
	do
	{
		bWillSwitch = false;
		bWantSwitch = false;
		for (int iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
		{
			int iBestValue;
			CivicTypes eNewCivic = AI_bestCivic((CivicOptionTypes)iI, &iBestValue);

			int iTestAnarchy = getCivicAnarchyLength(&aeBestCivic[0]);
			// using ~30 percent as a rough estimate of revolution cost, and low threshold regardless of anarchy just for a bit of inertia.
			// reduced threshold if we are already going to have a revolution.
			// <advc.003> Only did away with the nested conditional operators.
			int iThreshold = 5;
			if(iTestAnarchy > iAnarchyLength)
				iThreshold += (!bFirstPass | bWantSwitch ? 20 : 30); // </advc.003>
			/*  <advc.132b> If vassal, increase threshold unless switching is
				(permanently) free. */
			if(getAnarchyModifier() > -100 && getMaxAnarchyTurns() != 0) {
				TeamTypes masterTeamId = getMasterTeam();
				if(getTeam() != masterTeamId && GET_TEAM(masterTeamId).isHuman())
					iThreshold += 15;
				/*  Earlier (working) code that applies inertia when the current
					vassal civic matches that of the master (regardless of whether
					the master is human): */
				/*CivicOptionTypes civicsColumn = (CivicOptionTypes)iI;
				CivicTypes currentCivic = getCivics(civicsColumn);
				if(masterTeamId != NO_PLAYER && currentCivic != NO_CIVIC) {
					int nMastersMatchingCurrent = 0;
					int nMasters = 0;
					for(int j = 0; j < MAX_CIV_PLAYERS; j++) {
						CvPlayer const& masterCiv = GET_PLAYER((PlayerTypes)j);
						if(!masterCiv.isAlive()
								|| masterCiv.getTeam() != masterTeamId)
							continue;
						nMasters++;
						if(masterCiv.isCivic(currentCivic))
							nMastersMatchingCurrent++;
					}
					if(nMastersMatchingCurrent > 0) {
						int masterBasedInertia = 25;
						masterBasedInertia *= nMastersMatchingCurrent;
						masterBasedInertia /= nMasters;
						iThreshold += masterBasedInertia;
					}
				}*/
			} // </advc.132b>

			if (100*iBestValue > (100+iThreshold)*aiCurrentValue[iI])
			{
				FAssert(aeBestCivic[iI] != NO_CIVIC);
				if (gPlayerLogLevel > 0) logBBAI("    %S decides to switch to %S (value: %d vs %d%S)", getCivilizationDescription(0), GC.getCivicInfo(eNewCivic).getDescription(0), iBestValue, aiCurrentValue[iI], bFirstPass?"" :", on recheck");
				iAnarchyLength = iTestAnarchy;
				aeBestCivic[iI] = eNewCivic;
				aiCurrentValue[iI] = iBestValue;
				bWillSwitch = true;
			}
			else
			{
				if (100*iBestValue > 120*aiCurrentValue[iI])
					bWantSwitch = true;
			}
		}
		bFirstPass = false;
	} while (bWillSwitch && bWantSwitch);
	// Recheck, just in case we can switch another good civic without adding more anarchy.


	// finally, if our current research would give us a new civic, consider waiting for that.
	if (iAnarchyLength > 0 && bWillSwitch)
	{
		TechTypes eResearch = getCurrentResearch();
		int iResearchTurns;
		if (eResearch != NO_TECH && (iResearchTurns = getResearchTurnsLeft(eResearch, true)) < 2*CIVIC_CHANGE_DELAY/3)
		{
			for (int iI = 0; iI < GC.getNumCivicInfos(); iI++)
			{
				const CvCivicInfo& kCivic = GC.getCivicInfo((CivicTypes)iI);
				if (kCivic.getTechPrereq() == eResearch && !canDoCivics((CivicTypes)iI))
				{
					CivicTypes eOtherCivic = aeBestCivic[kCivic.getCivicOptionType()];
					aeBestCivic[kCivic.getCivicOptionType()] = (CivicTypes)iI; // temporary switch just to test the anarchy length
					if (getCivicAnarchyLength(&aeBestCivic[0]) <= iAnarchyLength)
					{
						// if the anarchy length would be the same, consider waiting for the new civic..
						int iValue = AI_civicValue((CivicTypes)iI);
						if (100 * iValue > (102+2*iResearchTurns) * aiCurrentValue[kCivic.getCivicOptionType()])
						{
							if (gPlayerLogLevel > 0)
								logBBAI("    %S delays revolution to wait for %S (value: %d vs %d)", getCivilizationDescription(0), kCivic.getDescription(0), iValue, aiCurrentValue[kCivic.getCivicOptionType()]);
							AI_setCivicTimer(iResearchTurns*2/3);
							return;
						}
					}
					aeBestCivic[kCivic.getCivicOptionType()] = eOtherCivic;
				}
			}
		}
	}
	//

	if (canRevolution(&aeBestCivic[0]))
	{
		revolution(&aeBestCivic[0]);
		AI_setCivicTimer((getMaxAnarchyTurns() == 0) ? (GC.getDefineINT("MIN_REVOLUTION_TURNS") * 2) : CIVIC_CHANGE_DELAY);
	}
}

void CvPlayerAI::AI_doReligion()
{
	ReligionTypes eBestReligion;

	FAssertMsg(!isHuman(), "isHuman did not return false as expected");
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      07/20/09                                jdog5000      */
/*                                                                                              */
/* Barbarian AI, efficiency                                                                     */
/************************************************************************************************/
	if( isBarbarian() )
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_getReligionTimer() > 0)
	{
		AI_changeReligionTimer(-1);
		return;
	}

	if (!canChangeReligion())
	{
		return;
	}

	FAssertMsg(AI_getReligionTimer() == 0, "AI Religion timer is expected to be 0");

	eBestReligion = AI_bestReligion();

	if (eBestReligion == NO_RELIGION)
	{
		eBestReligion = getStateReligion();
	}
	if (canConvert(eBestReligion))
	{	// <advc.131>
		double pr = 1;
		if(getReligionAnarchyLength() > 0 && eBestReligion != NO_RELIGION &&
				getStateReligion() != NO_RELIGION)
			pr = 1 - AI_religionValue(getStateReligion()) /
					(double)AI_religionValue(eBestReligion);
		/*  AI_religionValue fluctuates a lot; not sure why. Squaring pr to
			reduce switching. */
		pr *= pr;
		if(::bernoulliSuccess(pr, "advc.131")) { // </advc.131>
			if (gPlayerLogLevel > 0)
			{
				logBBAI("    %S decides to convert to %S (value: %d vs %d)", getCivilizationDescription(0), GC.getReligionInfo(eBestReligion).getDescription(0),
					eBestReligion == NO_RELIGION ? 0 : AI_religionValue(eBestReligion), getStateReligion() == NO_RELIGION ? 0 : AI_religionValue(getStateReligion()));
			}
			convert(eBestReligion);
			AI_setReligionTimer((getMaxAnarchyTurns() == 0) ? (GC.getDefineINT("MIN_CONVERSION_TURNS") * 2) : RELIGION_CHANGE_DELAY);
		} // advc.131
	}
}
// <advc.003>
/*  advc.133: 1 means d should be canceled, -1 it shouldn't be canceled,
	0 it should be renegotiated. */
int CvPlayerAI::checkCancel(CvDeal const& d, PlayerTypes otherId, bool flip) {

	PlayerTypes c1 = (flip ? d.getSecondPlayer() : d.getFirstPlayer());
	PlayerTypes c2 = (flip ? d.getFirstPlayer() : d.getSecondPlayer());
	CLinkList<TradeData> const* trades1 = (flip ? d.getSecondTrades() :
			d.getFirstTrades());
	CLinkList<TradeData> const* trades2 = (flip ? d.getFirstTrades() :
			d.getSecondTrades());
	if(c1 != getID() || c2 != otherId)
		return -1;
	// Cut and pasted from AI_doDiplo:
	// K-Mod. getTradeDenial is not equipped to consider deal cancelation properly.
	if(!AI_considerOffer(otherId, trades2, trades1, -1)) // <advc.133>
		return 0;
	/*  @"not equipped"-comment above: Resource trades do need special treatment.
		For duals (Open Borders, Def. Pact, Perm. Alliance), getTradeDenial
		is all right. AI_considerOffer will not cancel these b/c the trade values
		of dual trades are always considered fair.
		Worst enmity is treated separately elsewhere. */
	if(trades1 == NULL)
		return -1;
	CLLNode<TradeData>* tdn = trades1->head();
	if(tdn != NULL && CvDeal::isDual(tdn->m_data.m_eItemType, true) &&
			getTradeDenial(otherId, tdn->m_data) != NO_DENIAL &&
			::bernoulliSuccess(0.2, "advc.133"))
		return 1;
	/*  getTradeDenial will always return DENIAL_JOKING. Instead, call
		AI_bonusTrade explicitly and tell it that this is about cancelation. */
	if(trades2 == NULL)
		return -1;
	for(CLLNode<TradeData>* tdn = trades1->head(); tdn != NULL;
			tdn = trades1->next(tdn)) {
		TradeData tdata = tdn->m_data;
		if(tdata.m_eItemType != TRADE_RESOURCES)
			continue;
		BonusTypes bId = (BonusTypes)tdata.m_iData;
		if(AI_bonusTrade(bId, otherId, -1) != NO_DENIAL)
			return 1;
	} // </advc.133>
	return -1;
} // </advc.003>

void CvPlayerAI::AI_doDiplo()
{
	PROFILE_FUNC();

	CLLNode<TradeData>* pNode;
	CvDiploParameters* pDiplo;
	CvDeal* pLoopDeal;
	CvCity* pLoopCity;
	CvPlot* pLoopPlot;
	CLinkList<TradeData> ourList;
	CLinkList<TradeData> theirList;
	bool abContacted[MAX_TEAMS];
	TradeData item;
	// advc.133: Moved into proposeResourceTrade
	//BonusTypes eBestReceiveBonus;
	BonusTypes eBestGiveBonus;
	TechTypes eBestReceiveTech;
	TechTypes eBestGiveTech;
	TeamTypes eBestTeam;
	//bool bCancelDeal; // advc.003: Moved into checkCancel
	int iReceiveGold;
	int iGiveGold;
	int iGold;
	//int iGoldData;
	//int iGoldWeight;
	int iGoldValuePercent;
	int iCount;
	int iPossibleCount;
	int iValue;
	int iBestValue;
	int iOurValue;
	int iTheirValue;
	int iPass;
	int iLoop;
	int iI, iJ;

	FAssert(!isHuman());
	FAssert(!isMinorCiv());
	FAssert(!isBarbarian());

	// allow python to handle it
	if (GC.getUSE_AI_DO_DIPLO_CALLBACK()) // K-Mod. block unused python callbacks
	{
		CyArgsList argsList;
		argsList.add(getID());
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "AI_doDiplo", argsList.makeFunctionArgs(), &lResult);
		if (lResult == 1)
		{
			return;
		}
	}
	iGoldValuePercent = AI_goldTradeValuePercent();

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		abContacted[iI] = false;
	} // <advc.003> Replaced everywhere
	CvGame& g = GC.getGame();
	CvTeamAI const& ourTeam = GET_TEAM(getTeam()); // </advc.003>
	// <advc.024>
	int contacts[MAX_CIV_PLAYERS];
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
		contacts[i] = i;
	::shuffleArray(contacts, MAX_CIV_PLAYERS, g.getSorenRand());
	// </advc.024>
	for (iPass = 0; iPass < 2; iPass++)
	{
		for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			/*  <advc.003> Some refactoring in the long body of this loop.
				Important for advc.024 that civId is used everywhere instead of
				(PlayerTypes)iI. */
			PlayerTypes civId = (PlayerTypes)contacts[iI]; // advc.024
			CvPlayerAI const& civ = GET_PLAYER(civId);
			if(!civ.isAlive() || civ.isHuman() != (iPass == 1) || civId == getID() ||
					civ.getNumCities() <= 0) // advc.701
				continue;
			/*  <advc.706> Don't want player to receive diplo msgs on the
				first turn of a chapter. */
			if(civ.isHuman() && g.isOption(GAMEOPTION_RISE_FALL) &&
					g.getRiseFall().isBlockPopups())
				continue;
			// </advc.706>
			//if (GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam()) // disabled by K-Mod
			for(pLoopDeal = g.firstDeal(&iLoop); pLoopDeal != NULL; pLoopDeal = g.nextDeal(&iLoop))
			{
				if(!pLoopDeal->isCancelable(getID()))
					continue;
				// if ((g.getGameTurn() - pLoopDeal->getInitialGameTurn()) >= (GC.getDefineINT("PEACE_TREATY_LENGTH") * 2)) // K-Mod disabled
				// (original bts code deleted)
				/*  advc.003: Cancelation checks moved into a subfunction
					to reduce code duplication */
				// <advc.133>
				int cancelCode1 = checkCancel(*pLoopDeal, civId, false);
				int cancelCode2 = checkCancel(*pLoopDeal, civId, true);
				if(cancelCode1 < 0 && cancelCode2 < 0)
					continue;
				bool renegotiate = ((cancelCode1 == 0 || cancelCode2 == 0) &&
						/*  Canceled AI-human deals bring up the trade table
							anyway (except when a trade becomes invalid);
							this is about AI-AI trades. */
						!isHuman() && !GET_PLAYER(civId).isHuman() &&
						// The rest is covered by proposeResourceTrade
						canContactAndTalk(civId)); // </advc.133>
				if (civ.isHuman() && canContactAndTalk(civId))
				{
					ourList.clear();
					theirList.clear();
					bool bVassalDeal = pLoopDeal->isVassalDeal(); // K-Mod

					for (pNode = pLoopDeal->headFirstTradesNode(); pNode != NULL; pNode = pLoopDeal->nextFirstTradesNode(pNode))
					{
						if (pLoopDeal->getFirstPlayer() == getID())
							ourList.insertAtEnd(pNode->m_data);
						else theirList.insertAtEnd(pNode->m_data);
					}
					for (pNode = pLoopDeal->headSecondTradesNode(); pNode != NULL; pNode = pLoopDeal->nextSecondTradesNode(pNode))
					{
						if (pLoopDeal->getSecondPlayer() == getID())
							ourList.insertAtEnd(pNode->m_data);
						else theirList.insertAtEnd(pNode->m_data);
					}
					pLoopDeal->kill(); // K-Mod. Kill the old deal first.

					pDiplo = new CvDiploParameters(getID());
					FAssert(pDiplo != NULL);

					if (bVassalDeal)
					{
						pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_NO_VASSAL"));
						pDiplo->setAIContact(true);
						gDLL->beginDiplomacy(pDiplo, civId);
					}
					else
					{
						pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_CANCEL_DEAL"));
						pDiplo->setAIContact(true);
						pDiplo->setOurOfferList(theirList);
						pDiplo->setTheirOfferList(ourList);
						gDLL->beginDiplomacy(pDiplo, civId);
					}
					abContacted[civ.getTeam()] = true;
				}
				// K-Mod.
				else {
					pLoopDeal->kill(); // K-Mod end
					// <advc.133>
					if(renegotiate)
						proposeResourceTrade(civId); // </advc.133>
				}
				//pLoopDeal->kill(); // XXX test this for AI...
				// K-Mod. I've rearranged stuff so that we can kill the deal before a diplomacy window.
			}

			if(!canContactAndTalk(civId))
				continue;
			if(civ.getTeam() == getTeam() || TEAMREF(civId).isVassal(getTeam()))
			{
				// XXX will it cancel this deal if it loses it's first resource???

				iBestValue = 0;
				eBestGiveBonus = NO_BONUS;

				for (iJ = 0; iJ < GC.getNumBonusInfos(); iJ++)
				{
					if (getNumTradeableBonuses((BonusTypes)iJ) > 1)
					{
						/* original bts code
						if ((GET_PLAYER((PlayerTypes)iI).AI_bonusTradeVal(((BonusTypes)iJ), getID(), 1) > 0)
							&& (GET_PLAYER((PlayerTypes)iI).AI_bonusVal((BonusTypes)iJ, 1) > AI_bonusVal((BonusTypes)iJ, -1))) */
						// K-Mod
						bool bHasBonus = civ.getNumAvailableBonuses((BonusTypes)iJ) > 0;
						if (civ.AI_bonusTradeVal(((BonusTypes)iJ), getID(), 1) > 0 &&
								2 * civ.AI_bonusVal((BonusTypes)iJ, 1) > (bHasBonus? 3 : 2) * AI_bonusVal((BonusTypes)iJ, -1))
						// K-mod end
						{
							setTradeItem(&item, TRADE_RESOURCES, iJ);

							if (canTradeItem(civId, item, true))
							{
								iValue = (1 + g.getSorenRandNum(10000, "AI Bonus Trading #1"));

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									eBestGiveBonus = ((BonusTypes)iJ);
								}
							}
						}
					}
				}

				if (eBestGiveBonus != NO_BONUS)
				{
					ourList.clear();
					theirList.clear();

					setTradeItem(&item, TRADE_RESOURCES, eBestGiveBonus);
					ourList.insertAtEnd(item);

					if (civ.isHuman())
					{
						if (!abContacted[civ.getTeam()])
						{
							pDiplo = new CvDiploParameters(getID());
							FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
							pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_GIVE_HELP"));
							pDiplo->setAIContact(true);
							pDiplo->setTheirOfferList(ourList);
							gDLL->beginDiplomacy(pDiplo, civId);
							abContacted[civ.getTeam()] = true;
						}
					}
					else g.implementDeal(getID(), civId, &ourList, &theirList);
				}
			}

			if (civ.getTeam() != getTeam() && TEAMREF(civId).isVassal(getTeam()))
			{
				iBestValue = 0;
				eBestGiveTech = NO_TECH;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/06/09                                jdog5000      */
/*                                                                                              */
/* Diplomacy                                                                                    */
/************************************************************************************************/
				// Don't give techs for free to advanced vassals ...
				//if( GET_PLAYER((PlayerTypes)iI).getTechScore()*10 < getTechScore()*9 )
				/*  <advc.112> Give tech only by and by probabilistically, especially to
					peace vassals. The longer they stay with us, the more we trust them. */
				int theirTechScore = 10 * civ.getTechScore();
				int ourTechScore = getTechScore() *
						(civ.isHuman() ? 7 : 9); // K-Mod
				double techScoreRatio = theirTechScore / (ourTechScore + 0.01);
				double pr = 1 - techScoreRatio;
				if(TEAMREF(civId).isCapitulated())
					pr = std::max(pr, 1/3.0);
				if(techScoreRatio <= 1 && ::bernoulliSuccess(pr, "advc.112"))
				{	// </advc.112>
					for (iJ = 0; iJ < GC.getNumTechInfos(); iJ++)
					{
						if (ourTeam.AI_techTrade((TechTypes)iJ, civ.getTeam()) == NO_DENIAL)
						{
							setTradeItem(&item, TRADE_TECHNOLOGIES, iJ);

							if (canTradeItem(civId, item, true))
							{
								iValue = (1 + g.getSorenRandNum(10000, "AI Vassal Tech gift"));

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									eBestGiveTech = ((TechTypes)iJ);
								}
							}
						}
					}
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

				if (eBestGiveTech != NO_TECH)
				{
					ourList.clear();
					theirList.clear();

					setTradeItem(&item, TRADE_TECHNOLOGIES, eBestGiveTech);
					ourList.insertAtEnd(item);

					if (civ.isHuman())
					{
						if (!abContacted[civ.getTeam()])
						{
							pDiplo = new CvDiploParameters(getID());
							FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
							pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_GIVE_HELP"));
							pDiplo->setAIContact(true);
							pDiplo->setTheirOfferList(ourList);
							gDLL->beginDiplomacy(pDiplo, civId);
							abContacted[civ.getTeam()] = true;
						}
					}
					else g.implementDeal(getID(), civId, &ourList, &theirList);
				}
			}

			if(civ.getTeam() == getTeam() || ourTeam.isHuman() ||
					(!civ.isHuman() && TEAMREF(civId).isHuman()) ||
					ourTeam.isAtWar(civ.getTeam()))
				continue;
			FAssert(!civ.isBarbarian());

			if (TEAMREF(civId).isVassal(getTeam()))
			{
				iBestValue = 0;
				eBestGiveBonus = NO_BONUS;

				for (iJ = 0; iJ < GC.getNumBonusInfos(); iJ++)
				{
					if (civ.getNumTradeableBonuses((BonusTypes)iJ) > 0 && getNumAvailableBonuses((BonusTypes)iJ) == 0)
					{
						iValue = AI_bonusTradeVal((BonusTypes)iJ, civId, 1);

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							eBestGiveBonus = ((BonusTypes)iJ);
						}
					}
				}

				if (eBestGiveBonus != NO_BONUS)
				{
					theirList.clear();
					ourList.clear();

					setTradeItem(&item, TRADE_RESOURCES, eBestGiveBonus);
					theirList.insertAtEnd(item);

					if (civ.isHuman())
					{
						if (!abContacted[civ.getTeam()])
						{
							CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_VASSAL_GRANT_TRIBUTE, getID(), eBestGiveBonus);
							if (pInfo)
							{
								gDLL->getInterfaceIFace()->addPopup(pInfo, civId);
								abContacted[civ.getTeam()] = true;
							}
						}
					}
					else g.implementDeal(getID(), civId, &ourList, &theirList);
				}
			}

			if (AI_getAttitude(civId) >= ATTITUDE_CAUTIOUS)
			{
				for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
				{
					if (pLoopCity->getPreviousOwner() != civId)
					{
						if (((pLoopCity->getGameTurnAcquired() + 4) % 20) == (g.getGameTurn() % 20))
						{
							iCount = 0;
							iPossibleCount = 0;

							for (iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
							{
								pLoopPlot = plotCity(pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), iJ);

								if (pLoopPlot != NULL)
								{
									if (pLoopPlot->getOwnerINLINE() == civId)
										iCount++;

									iPossibleCount++;
								}
							}

							if (iCount >= (iPossibleCount / 2))
							{
								setTradeItem(&item, TRADE_CITIES, pLoopCity->getID());

								if (canTradeItem(civId, item, true))
								{
									ourList.clear();
									ourList.insertAtEnd(item);

									if (civ.isHuman())
									{
										//if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()])) {
										pDiplo = new CvDiploParameters(getID());
										FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
										pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_CITY"));
										pDiplo->setAIContact(true);
										pDiplo->setTheirOfferList(ourList);
										gDLL->beginDiplomacy(pDiplo, civId);
										abContacted[civ.getTeam()] = true;
									}
									else g.implementDeal(getID(), civId, &ourList, NULL);
								}
							}
						}
					}
				}
			}

			if (ourTeam.getLeaderID() == getID() &&
					// advc.003b: Avoid costly TradeDenial check (in canTradeItem)
					!ourTeam.isAVassal())
			{
				if (AI_getContactTimer(civId, CONTACT_PERMANENT_ALLIANCE) == 0)
				{
					bool bOffered = false;
					if (g.getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_PERMANENT_ALLIANCE), "AI Diplo Alliance") == 0)
					{
						setTradeItem(&item, TRADE_PERMANENT_ALLIANCE);

						if (canTradeItem(civId, item, true) && civ.canTradeItem(getID(), item, true))
						{
							ourList.clear();
							theirList.clear();
							ourList.insertAtEnd(item);
							theirList.insertAtEnd(item);
							bOffered = true;

							if (civ.isHuman())
							{
								if (!abContacted[civ.getTeam()])
								{
									AI_changeContactTimer(civId, CONTACT_PERMANENT_ALLIANCE, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_PERMANENT_ALLIANCE));
									pDiplo = new CvDiploParameters(getID());
									FAssert(pDiplo != NULL);
									pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_DEAL"));
									pDiplo->setAIContact(true);
									pDiplo->setOurOfferList(theirList);
									pDiplo->setTheirOfferList(ourList);
									gDLL->beginDiplomacy(pDiplo, civId);
									abContacted[civ.getTeam()] = true;
								}
							}
							else
							{
								g.implementDeal(getID(), civId, &ourList, &theirList);
								break; // move on to next player since we are on the same team now
							}
						}
					}

					if(bOffered)
						continue;
					// <advc.112>
					int div = GC.getLeaderHeadInfo(getPersonalityType()).
							getContactRand(CONTACT_PERMANENT_ALLIANCE); // 80
					double prVassal = 0;
					if(div > 0) {
						prVassal = (8.0 / div) * ::dRange(g.getPlayerRank(civId) /
								(double)g.countCivPlayersAlive(), 0.25, 0.5);
						if(ourTeam.getAtWarCount() > 0)
							prVassal *= 4;
					}
					if(::bernoulliSuccess(prVassal, "advc.112")) { // </advc.112>
						setTradeItem(&item, TRADE_VASSAL);
						if (canTradeItem(civId, item, true))
						{
							ourList.clear();
							theirList.clear();
							ourList.insertAtEnd(item);

							if (civ.isHuman())
							{
								if (!abContacted[civ.getTeam()])
								{
									AI_changeContactTimer(civId, CONTACT_PERMANENT_ALLIANCE, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_PERMANENT_ALLIANCE));
									pDiplo = new CvDiploParameters(getID());
									FAssert(pDiplo != NULL);
									pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_VASSAL"));
									pDiplo->setAIContact(true);
									pDiplo->setOurOfferList(theirList);
									pDiplo->setTheirOfferList(ourList);
									gDLL->beginDiplomacy(pDiplo, civId);
									abContacted[civ.getTeam()] = true;
								}
							}
							else
							{
								bool bAccepted = true;
								/*  advc.104o: Already checked by canTradeItem
									(actually regardless of whether UWAI is enabled,
									I think) */
								if(!getWPAI.isEnabled()) {
									TeamTypes eMasterTeam = civ.getTeam();
									for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
									{	// <advc.003> Refactored
										TeamTypes tId = (TeamTypes)iTeam;
										if(!GET_TEAM(tId).isAlive() || tId == getTeam() ||
												tId == eMasterTeam || !atWar(getTeam(), tId) ||
												atWar(eMasterTeam, tId))
											continue; // </advc.003>
										if (GET_TEAM(eMasterTeam).AI_declareWarTrade((TeamTypes)iTeam, getTeam(), false) != NO_DENIAL)
										{
											bAccepted = false;
											break;
										}
									}
								}
								if(bAccepted)
									g.implementDeal(getID(), civId, &ourList, &theirList);
							}
						}
					} // advc.112
				}
			}

			if (civ.isHuman() && (ourTeam.getLeaderID() == getID()) && !ourTeam.isVassal(civ.getTeam()))
			{
				if (!abContacted[civ.getTeam()])
				{
					if (g.getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_RELIGION_PRESSURE), "AI Diplo Religion Pressure") == 0)
					{
						abContacted[civ.getTeam()] = contactReligion(civId);
					}
				}
				if (!abContacted[civ.getTeam()])
				{
					if (g.getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_CIVIC_PRESSURE), "AI Diplo Civic Pressure") == 0)
					{
						abContacted[civ.getTeam()] = contactCivics(civId);
					}
				}
			}

			if (civ.isHuman() && (ourTeam.getLeaderID() == getID()))
			{
				if(!abContacted[civ.getTeam()])
				{	/*  advc.130m: ContactRand check moved into proposeJointWar;
						now a bit more complicated. */
					abContacted[civ.getTeam()] = proposeJointWar(civId);
				}
			}

			if (civ.isHuman() && ourTeam.getLeaderID() == getID() && !ourTeam.isVassal(civ.getTeam()))
			{
				if (g.getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_STOP_TRADING), "AI Diplo Stop Trading") == 0)
				{
					// <advc.003> Moved into new function
					if(!abContacted[civ.getTeam()])
						abContacted[civ.getTeam()] = proposeEmbargo(civId);
					// </advc.003>
				}
			}

			if (civ.isHuman() && ourTeam.getLeaderID() == getID())
			{
				if (TEAMREF(civId).getAssets() < (ourTeam.getAssets() / 2))
				{
					if (AI_getAttitude(civId) > GC.getLeaderHeadInfo(
							/*  advc.001: NoGiveHelpAttitudeThreshold is the same for all leaders,
								so it doesn't really matter. On principle, the personality of the
								human leader shouldn't matter. */
							//GET_PLAYER((PlayerTypes)iI).
							getPersonalityType()).getNoGiveHelpAttitudeThreshold())
					{
						if (AI_getContactTimer(civId, CONTACT_GIVE_HELP) == 0)
						{
							if (g.getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_GIVE_HELP), "AI Diplo Give Help") == 0)
							{
								// XXX maybe do gold instead???

								iBestValue = 0;
								eBestGiveTech = NO_TECH;

								for (iJ = 0; iJ < GC.getNumTechInfos(); iJ++)
								{
									setTradeItem(&item, TRADE_TECHNOLOGIES, iJ);

									if (canTradeItem(civId, item, true))
									{
										iValue = (1 + g.getSorenRandNum(10000, "AI Giving Help"));

										if (iValue > iBestValue)
										{
											iBestValue = iValue;
											eBestGiveTech = ((TechTypes)iJ);
										}
									}
								}

								if (eBestGiveTech != NO_TECH)
								{
									ourList.clear();
									setTradeItem(&item, TRADE_TECHNOLOGIES, eBestGiveTech);
									ourList.insertAtEnd(item);

									if (!abContacted[civ.getTeam()])
									{
										AI_changeContactTimer(civId, CONTACT_GIVE_HELP, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_GIVE_HELP));
										pDiplo = new CvDiploParameters(getID());
										FAssert(pDiplo != NULL);
										pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_GIVE_HELP"));
										pDiplo->setAIContact(true);
										pDiplo->setTheirOfferList(ourList);
										gDLL->beginDiplomacy(pDiplo, civId);
										abContacted[civ.getTeam()] = true;
									}
								}
							}
						}
					}
				}
				
				if (!abContacted[civ.getTeam()] &&
						// <advc.130v> 
						(!ourTeam.isAVassal() || ourTeam.getMasterTeam() == civ.getTeam())) {
						// </advc.130v>
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/12/10                                jdog5000      */
/*                                                                                              */
/* Diplomacy                                                                                    */
/************************************************************************************************/
					int iRand = GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_ASK_FOR_HELP);
					int iTechPerc = ourTeam.getBestKnownTechScorePercent();
					if( iTechPerc < 90 )
					{
						iRand *= std::max(1, iTechPerc - 60);
						iRand /= 30;
					}
					
					if (g.getSorenRandNum(iRand, "AI Diplo Ask For Help") == 0)
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
					{
						abContacted[civ.getTeam()] = askHelp(civId);
					}
				}
/* original bts code
				if (ourTeam.canDeclareWar(GET_PLAYER((PlayerTypes)iI).getTeam()) && !ourTeam.AI_isSneakAttackPreparing(GET_PLAYER((PlayerTypes)iI).getTeam()))
*/
				// Bug fix: when team was sneak attack ready but hadn't declared, could demand tribute
				// If other team accepted, it blocked war declaration for 10 turns but AI didn't react.
				if (ourTeam.canDeclareWar(civ.getTeam()) && !ourTeam.AI_isChosenWar(civ.getTeam()))
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
				{
					if (TEAMREF(civId).getDefensivePower() < ourTeam.getPower(true)
							&& !isFocusWar()  // advc.105
							// advc.104g:
							&& (!getWPAI.isEnabled() || warAndPeaceAI().getCache().numReachableCities(civId) > 0))
					{
						if (!abContacted[civ.getTeam()])
						{	// Each type of tribute request uses the same probability
							for(int i = 0; i < 5; i++) {
								if(g.getSorenRandNum(
										GC.getLeaderHeadInfo(getPersonalityType()).
										getContactRand(CONTACT_DEMAND_TRIBUTE),
										"AI Diplo Demand Tribute") == 0) {
									abContacted[civ.getTeam()] = demandTribute(civId, i);
									break;
								}
							}
						}
					}
				}
			}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

			if(ourTeam.getLeaderID() == getID())
			{
				if (AI_getContactTimer(civId, CONTACT_OPEN_BORDERS) == 0)
				{
					if (g.getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_OPEN_BORDERS), "AI Diplo Open Borders") == 0)
					{
						setTradeItem(&item, TRADE_OPEN_BORDERS);

						if (canTradeItem(civId, item, true) && civ.canTradeItem(getID(), item, true))
						{
							ourList.clear();
							theirList.clear();
							ourList.insertAtEnd(item);
							theirList.insertAtEnd(item);

							if (civ.isHuman())
							{
								if (!abContacted[civ.getTeam()])
								{
									AI_changeContactTimer(civId, CONTACT_OPEN_BORDERS, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_OPEN_BORDERS));
									pDiplo = new CvDiploParameters(getID());
									FAssert(pDiplo != NULL);
									pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_DEAL"));
									pDiplo->setAIContact(true);
									pDiplo->setOurOfferList(theirList);
									pDiplo->setTheirOfferList(ourList);
									gDLL->beginDiplomacy(pDiplo, civId);
									abContacted[civ.getTeam()] = true;
								}
							}
							else g.implementDeal(getID(), civId, &ourList, &theirList);
						}
					}
				}

				if (AI_getContactTimer(civId, CONTACT_DEFENSIVE_PACT) == 0)
				{	// <cdtw.5>
					int iRand = GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_DEFENSIVE_PACT);
					if(AI_isDoStrategy(AI_STRATEGY_ALERT2) && !isFocusWar() &&
							!civ.isHuman()) {
						iRand = iRand / std::max(1, 10 - 2 *
								ourTeam.getDefensivePactCount());
					}
					if(g.getSorenRandNum(iRand, "AI Diplo Defensive Pact") == 0)
					// Replacing this: // </cdtw.5>
					//if (g.getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_DEFENSIVE_PACT), "AI Diplo Defensive Pact") == 0)
					{
						setTradeItem(&item, TRADE_DEFENSIVE_PACT);

						if (canTradeItem(civId, item, true) && civ.canTradeItem(getID(), item, true))
						{
							ourList.clear();
							theirList.clear();
							ourList.insertAtEnd(item);
							theirList.insertAtEnd(item);

							if (civ.isHuman())
							{
								if (!abContacted[civ.getTeam()])
								{
									AI_changeContactTimer(civId, CONTACT_DEFENSIVE_PACT, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_DEFENSIVE_PACT));
									pDiplo = new CvDiploParameters(getID());
									FAssert(pDiplo != NULL);
									pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_DEAL"));
									pDiplo->setAIContact(true);
									pDiplo->setOurOfferList(theirList);
									pDiplo->setTheirOfferList(ourList);
									gDLL->beginDiplomacy(pDiplo, civId);
									abContacted[civ.getTeam()] = true;
								}
							}
							else g.implementDeal(getID(), civId, &ourList, &theirList);
						}
					}
				}
			}

			if (!civ.isHuman() || ourTeam.getLeaderID() == getID())
			{
				if (AI_getContactTimer(civId, CONTACT_TRADE_TECH) == 0)
				{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/24/10                                jdog5000      */
/*                                                                                              */
/* Diplomacy                                                                                    */
/************************************************************************************************/
					int iRand = GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_TRADE_TECH);
					int iTechPerc = ourTeam.getBestKnownTechScorePercent();
					if( iTechPerc < 90 )
					{
						/* BBAI
						iRand *= std::max(1, iTechPerc - 60);
						iRand /= 30; */
						// K-Mod. not so extreme.
						iRand = (iRand * (10 + iTechPerc) + 50)/100;
						// K-Mod end
					}

					if( AI_isDoVictoryStrategy(AI_VICTORY_SPACE1) )
						iRand /= 2;
						
					iRand = std::max(1, iRand);
					if (g.getSorenRandNum(iRand, "AI Diplo Trade Tech") == 0)
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
					{
						iBestValue = 0;
						eBestReceiveTech = NO_TECH;
						/*  <advc.550f> This is normally going to be
							getCurrentResearch, but it doesn't hurt to check if
							we have more progress on some other tech. */
						TechTypes eBestProgressTech = NO_TECH;
						int iBestProgress = 0; // </advc.550f>
						for (iJ = 0; iJ < GC.getNumTechInfos(); iJ++)
						{
							setTradeItem(&item, TRADE_TECHNOLOGIES, iJ);
							TechTypes eTech = (TechTypes)iJ; // advc.003
							if (civ.canTradeItem(getID(), item, true))
							{
								/*  advc.001: Value can be 0 if tech already almost researched
									and iI is human. Will then ask human to impart the tech for
									nothing in return, which is pointless and confusing. */
								if(ourTeam.AI_techTradeVal(eTech, civ.getTeam()) == 0)
									continue;
								iValue = (1 + g.getSorenRandNum(10000, "AI Tech Trading #1"));
								// <advc.550f>
								int iProgress = ourTeam.getResearchProgress(eTech);
								if(iProgress > iBestProgress &&
										// This is only for inter-AI deals
										!civ.isHuman()) {
									iBestProgress = iProgress;
									eBestProgressTech = eTech;
								} // </advc.550f>
								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									eBestReceiveTech = ((TechTypes)iJ);
								}
							}
						}

						if (eBestReceiveTech != NO_TECH)
						{
							// K-Mod
							iOurValue = ourTeam.AI_techTradeVal(eBestReceiveTech, civ.getTeam());
							iTheirValue = 0;
							int iBestDelta = iOurValue;
							// K-Mod end
							//iBestValue = 0;
							eBestGiveTech = NO_TECH;

							for (iJ = 0; iJ < GC.getNumTechInfos(); iJ++)
							{
								setTradeItem(&item, TRADE_TECHNOLOGIES, iJ);

								if (canTradeItem(civId, item, true))
								{
									/* original bts code
									iValue = (1 + g.getSorenRandNum(10000, "AI Tech Trading #2"));
									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										eBestGiveTech = ((TechTypes)iJ);
									} */
									// K-Mod
									iValue = TEAMREF(civId).AI_techTradeVal((TechTypes)iJ, getTeam());
									int iDelta = std::abs(iOurValue - (90 + g.getSorenRandNum(21, "AI Tech Trading #2"))*iValue / 100);
									if (iDelta < iBestDelta) // aim to trade values as close as possible
									{
										iBestDelta = iDelta;
										iTheirValue = iValue;
										eBestGiveTech = ((TechTypes)iJ);
									}
									// K-Mod end
								}
							}

							// (original bts code deleted)

							ourList.clear();
							theirList.clear();

							if (eBestGiveTech != NO_TECH)
							{
								setTradeItem(&item, TRADE_TECHNOLOGIES, eBestGiveTech);
								ourList.insertAtEnd(item);
							}

							setTradeItem(&item, TRADE_TECHNOLOGIES, eBestReceiveTech);
							theirList.insertAtEnd(item);

							/* original bts code
							if (iGiveGold != 0)
							{
								setTradeItem(&item, TRADE_GOLD, iGiveGold);
								ourList.insertAtEnd(item);
							}

							if (iReceiveGold != 0)
							{
								setTradeItem(&item, TRADE_GOLD, iReceiveGold);
								theirList.insertAtEnd(item);
							} */
							// K-Mod
							/* bool bDeal = GET_PLAYER((PlayerTypes)iI).isHuman()
								? 6*iOurValue > 5*iTheirValue && 3*iTheirValue > 2*iOurValue
								: 3*iOurValue > 2*iTheirValue && 3*iTheirValue > 2*iOurValue; */
							bool bDeal = 100*iOurValue >= AI_tradeAcceptabilityThreshold(civId)*iTheirValue
									&& 100*iTheirValue >= civ.AI_tradeAcceptabilityThreshold(getID())*iOurValue;
							if (!bDeal
									/*  <advc.550b> Unlikely to lead to an
										attractive offer */
									&& (!civ.isHuman() || iTheirValue /
									(double)iOurValue > 0.5 ||
									::bernoulliSuccess(0.36, "advc.550b")))
									// </advc.550b>
							{
								PROFILE("AI_doDiplo self-counter-propose");
								// let the other team counter-propose (even if the other team is a human player)
								// unfortunately, the API is pretty clumsy for setting up this counter proposal.
								// please just bear with me.
								// (Note: this would be faster if we just built the lists directly, but by using the existing API, we are kind of future-proofing)
								CLinkList<TradeData> ourInventory, theirInventory, ourCounter, theirCounter;
								buildTradeTable(civId, ourInventory); // all tradeable items
								markTradeOffers(ourInventory, ourList); // K-Mod funciton - set m_bOffering on each item offered
								updateTradeList(civId, ourInventory, ourList, theirList); // hide what should be excluded
								civ.buildTradeTable(getID(), theirInventory); // same for the other player
								civ.markTradeOffers(theirInventory, theirList);
								civ.updateTradeList(getID(), theirInventory, theirList, ourList);

								// Note: AI_counterPropose tends to favour the caller, which in this case is the other player.
								// ie. we asked for the trade, but they set the final terms. (unless the original deal is accepted)
								if (civ.AI_counterPropose(getID(), &ourList, &theirList, &ourInventory, &theirInventory, &ourCounter, &theirCounter))
								{
									ourList.concatenate(ourCounter);
									theirList.concatenate(theirCounter);
									bDeal = true;
									// K-Mod note: the following log call will miss the second name if it is a non-standard name (for example, a human player name).
									// This is a problem in the way the getName function is implimented. Unfortunately, the heart of the problem is in the signature of
									// some of the dllexport functions - and so it can't easily be fixed.
									if (gTeamLogLevel >= 2) logBBAI("    %S makes a deal with %S using AI_counterPropose. (%d : %d)", getName(0), civ.getName(0), ourList.getLength(), theirList.getLength());
								}
							}
							if (bDeal)
							{
							// K-Mod end
								if (civ.isHuman())
								{
									if (!abContacted[civ.getTeam()])
									{
										AI_changeContactTimer(civId, CONTACT_TRADE_TECH, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_TRADE_TECH));
										pDiplo = new CvDiploParameters(getID());
										FAssert(pDiplo != NULL);
										pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_DEAL"));
										pDiplo->setAIContact(true);
										pDiplo->setOurOfferList(theirList);
										pDiplo->setTheirOfferList(ourList);
										gDLL->beginDiplomacy(pDiplo, civId);
										abContacted[civ.getTeam()] = true;
									}
								}
								else {
									g.implementDeal(getID(), civId, &ourList, &theirList);
									// advc.550f: I.e. we're done trading tech
									eBestProgressTech = NO_TECH;
								}
							}
						}
						// <advc.550f>
						if(eBestProgressTech != NO_TECH &&
							eBestProgressTech != eBestReceiveTech) {
							iOurValue = ourTeam.AI_techTradeVal(eBestProgressTech,
									civ.getTeam());
							int price = (100 * iOurValue) / civ.AI_goldTradeValuePercent();
							if(price <= AI_maxGoldTrade(civ.getID())) {
								setTradeItem(&item, TRADE_GOLD, price);
								if(canTradeItem(civ.getID(), item)) {
									ourList.clear();
									ourList.insertAtEnd(item);
									theirList.clear();
									setTradeItem(&item, TRADE_TECHNOLOGIES, eBestProgressTech);
									theirList.insertAtEnd(item);
									g.implementDeal(getID(), civId, &ourList, &theirList);
								}
							}
						} // </advc.550f>
					}
				}
			}
			/*  <advc.133> Moved the cheap abContacted check up, and the rest
				of the resource trade code into a new function. */
			if((!abContacted[civ.getTeam()] || !civ.isHuman()) &&
					g.getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).
					getContactRand(CONTACT_TRADE_BONUS),
					"AI Diplo Trade Bonus") == 0)
				abContacted[civ.getTeam()] = proposeResourceTrade(civId);
			// </advc.133>
			if (AI_getContactTimer(civId, CONTACT_TRADE_MAP) == 0)
			{
				if (g.getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_TRADE_MAP), "AI Diplo Trade Map") == 0)
				{
					setTradeItem(&item, TRADE_MAPS);
					if (civ.canTradeItem(getID(), item, true) && canTradeItem(civId, item, true))
					{
						// <advc.136b>
						int valForThem = TEAMREF(civId).AI_mapTradeVal(getTeam());
						int thresh = GC.getDIPLOMACY_VALUE_REMAINDER();
						if (!civ.isHuman() || (ourTeam.AI_mapTradeVal(civ.getTeam()) >= valForThem
								&& valForThem > thresh)) // <advc.136b>
						{
							ourList.clear();
							theirList.clear();
							setTradeItem(&item, TRADE_MAPS);
							ourList.insertAtEnd(item);
							setTradeItem(&item, TRADE_MAPS);
							theirList.insertAtEnd(item);

							if (civ.isHuman())
							{
								if (!abContacted[civ.getTeam()])
								{
									AI_changeContactTimer(civId, CONTACT_TRADE_MAP, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_TRADE_MAP));
									pDiplo = new CvDiploParameters(getID());
									FAssert(pDiplo != NULL);
									pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_DEAL"));
									pDiplo->setAIContact(true);
									pDiplo->setOurOfferList(theirList);
									pDiplo->setTheirOfferList(ourList);
									gDLL->beginDiplomacy(pDiplo, civId);
									abContacted[civ.getTeam()] = true;
								}
							}
							else g.implementDeal(getID(), civId, &ourList, &theirList);
						}
					}
				}
			}

			/*  advc.001: The only trade left is WarTrade. The AI doesn't offer
				war trades to humans. The BtS code kills such trades through
				canTradeItem, so it's not a real issue; just pointless. */
			if(civ.isHuman())// && abContacted[civ.getTeam()])
				continue;
			/*  <advc.130v> Capitulated vassals certainly shouldn't hire
				war allies. Peace vassals would be OK if it weren't for
				advc.146 which implements a peace treaty between the war allies;
				can't have a vassal force its master into a peace treaty. */
			if(ourTeam.isAVassal())
				continue; // </advc.130v>
			int iMinAtWarCounter = MAX_INT;
			for (iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
			{
				if (GET_TEAM((TeamTypes)iJ).isAlive())
				{
					if (atWar(((TeamTypes)iJ), getTeam()))
					{
						int iAtWarCounter = ourTeam.AI_getAtWarCounter((TeamTypes)iJ);
						if (ourTeam.AI_getWarPlan((TeamTypes)iJ) == WARPLAN_DOGPILE)
						{
							iAtWarCounter *= 2;
							iAtWarCounter += 5;
						}
						iMinAtWarCounter = std::min(iAtWarCounter, iMinAtWarCounter);
					}
				}
			}
			// 40 for all leaders
			int iDeclareWarTradeRand = GC.getLeaderHeadInfo(getPersonalityType()).getDeclareWarTradeRand();							
			if (iMinAtWarCounter < 10)
			{
				iDeclareWarTradeRand *= iMinAtWarCounter;
				iDeclareWarTradeRand /= 10;
				iDeclareWarTradeRand ++;
			}
										
			if (iMinAtWarCounter < 4)
			{
				iDeclareWarTradeRand /= 4;
				iDeclareWarTradeRand ++;
			}
			// <advc.104o>
			// Same probability as in BtS (so far)
			double pr = (iDeclareWarTradeRand <= 0 ? 1.0 :
					1.0 / iDeclareWarTradeRand);
			/*  pr is 1 if iMinAtWarCounter is 0, i.e. we've just declared war
				ourselves. That's a bit too predictable I think. */
			if(getWPAI.isEnabled())
				pr = std::min(pr, 0.75);
			// Commented out: BtS probability test
			//if(g.getSorenRandNum(iDeclareWarTradeRand, "AI Diplo Declare War Trade") != 0)
			if(!::bernoulliSuccess(pr, "advc.104o"))
				continue;
			int iBestTargetValue = 0; // </advc.104o>
			iBestValue = 0;
			eBestTeam = NO_TEAM;

			for (iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
			{
				if(!GET_TEAM((TeamTypes)iJ).isAlive() ||
						!atWar(((TeamTypes)iJ), getTeam()) ||
						atWar(((TeamTypes)iJ), civ.getTeam()))
					continue;
				// <advc.104o>
				if(getWPAI.isEnabled()) {
					CvTeam const& target = GET_TEAM((TeamTypes)iJ);
					/*  AI_declareWarTrade checks attitude toward the target;
						no war if the target is liked. Shouldn't matter for
						capitulated vassals, but an unpopular voluntary vassal
						should be a valid target even if the master would not be.
						Attitude toward the master still factors into
						tradeValJointWar. */
					if(target.isCapitulated())
						continue;
					setTradeItem(&item, TRADE_WAR, target.getID());
					if(!civ.canTradeItem(getID(), item))
						continue;
					/*  Test denial separately (not through canTradeItem) b/c
						the denial test is costlier and produces log output */
					if(TEAMREF(civId).AI_declareWarTrade(target.getID(), getTeam())
							!= NO_DENIAL)
						continue;
					iValue = ourTeam.warAndPeaceAI().tradeValJointWar(
							target.getID(), civ.getTeam());
					if(iValue <= iBestTargetValue)
						continue;
					/*  This call doesn't compute how much our team values the DoW
						by civ (that's iValue), it computes how much civ needs to be
						payed for the Dow. Has to work this way b/c AI_declareWarTradeVal
						is also used for human-AI war trades. */
					int theirPrice = ourTeam.AI_declareWarTradeVal(
							target.getID(), civ.getTeam());
					/*  Don't try to make the trade if the DoW by civId isn't
						nearly as valuable to us as what they'll charge */
					if(iValue >= (3 * theirPrice) / 4) {
						iBestTargetValue = iValue;
						eBestTeam = target.getID();
					}
				}
				else // </advc.104o>
				if (GET_TEAM((TeamTypes)iJ).getAtWarCount(true) < std::max(2, (g.countCivTeamsAlive() / 2)))
				{
					setTradeItem(&item, TRADE_WAR, iJ);

					if (civ.canTradeItem(getID(), item, true))
					{
						iValue = (1 + g.getSorenRandNum(1000, "AI Declare War Trading"));

						iValue *= (101 + GET_TEAM((TeamTypes)iJ).AI_getAttitudeWeight(getTeam()));
						iValue /= 100;

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							eBestTeam = ((TeamTypes)iJ);
						}
					}
				}
			}

			if (eBestTeam != NO_TEAM)
			{
				iBestValue = 0;
				eBestGiveTech = NO_TECH;

				for (iJ = 0; iJ < GC.getNumTechInfos(); iJ++)
				{
					setTradeItem(&item, TRADE_TECHNOLOGIES, iJ);

					if (canTradeItem(civId, item, true))
					{
						iValue = (1 + g.getSorenRandNum(100, "AI Tech Trading #2"));
														
						iValue *= GET_TEAM(eBestTeam).getResearchLeft((TechTypes)iJ);

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							eBestGiveTech = ((TechTypes)iJ);
						}
					}
				}

				iOurValue = ourTeam.AI_declareWarTradeVal(eBestTeam, civ.getTeam());
				if(eBestGiveTech != NO_TECH)
					iTheirValue = TEAMREF(civId).AI_techTradeVal(eBestGiveTech, getTeam());
				else iTheirValue = 0;

				int iBestValue2 = 0;
				TechTypes eBestGiveTech2 = NO_TECH;
												
				if ((iTheirValue < iOurValue) && (eBestGiveTech != NO_TECH))
				{
					for (iJ = 0; iJ < GC.getNumTechInfos(); iJ++)
					{
						if(iJ == eBestGiveTech)
							continue;
								
						setTradeItem(&item, TRADE_TECHNOLOGIES, iJ);

						if (canTradeItem(civId, item, true))
						{
							iValue = (1 + g.getSorenRandNum(100, "AI Tech Trading #2"));
																
							iValue *= GET_TEAM(eBestTeam).getResearchLeft((TechTypes)iJ);

							if (iValue > iBestValue)
							{
								iBestValue2 = iValue;
								eBestGiveTech2 = ((TechTypes)iJ);
							}
						}
					}
													
					if(eBestGiveTech2 != NO_TECH)
						iTheirValue += TEAMREF(civId).AI_techTradeVal(eBestGiveTech2, getTeam());
				}

				iReceiveGold = 0;
				iGiveGold = 0;

				if (iTheirValue > iOurValue &&
						/*  advc.104o: Don't worry about giving them more if it's
							still worth it for us. (No effect if UWAI is disabled
							b/c then iBestTargetValue remains 0.) */
						iTheirValue > iBestTargetValue)
				{
					iGold = std::min(((iTheirValue - iOurValue) * 100) / iGoldValuePercent, civ.AI_maxGoldTrade(getID()));

					if (iGold > 0)
					{
						setTradeItem(&item, TRADE_GOLD, iGold);

						if (civ.canTradeItem(getID(), item, true))
						{
							iReceiveGold = iGold;
							iOurValue += (iGold * iGoldValuePercent) / 100;
						}
					}
				}
				else if (iOurValue > iTheirValue)
				{
					iGold = std::min(((iOurValue - iTheirValue) * 100) / iGoldValuePercent, AI_maxGoldTrade(civId));

					if (iGold > 0)
					{
						setTradeItem(&item, TRADE_GOLD, iGold);

						if (canTradeItem(civId, item, true))
						{
							iGiveGold = iGold;
							iTheirValue += (iGold * iGoldValuePercent) / 100;
						}
					}
				}

				if (iTheirValue > (iOurValue * 3 / 4))
				{
					ourList.clear();
					theirList.clear();

					if (eBestGiveTech != NO_TECH)
					{
						setTradeItem(&item, TRADE_TECHNOLOGIES, eBestGiveTech);
						ourList.insertAtEnd(item);
					}
													
					if (eBestGiveTech2 != NO_TECH)
					{
						setTradeItem(&item, TRADE_TECHNOLOGIES, eBestGiveTech2);
						ourList.insertAtEnd(item);
					}

					setTradeItem(&item, TRADE_WAR, eBestTeam);
					theirList.insertAtEnd(item);

					if (iGiveGold != 0)
					{
						setTradeItem(&item, TRADE_GOLD, iGiveGold);
						ourList.insertAtEnd(item);
					}

					if (iReceiveGold != 0)
					{
						setTradeItem(&item, TRADE_GOLD, iReceiveGold);
						theirList.insertAtEnd(item);
					}

					g.implementDeal(getID(), civId, &ourList, &theirList);
				}
			}
		}
	}
}

// <advc.003> Refactored code from AI_doDiplo
/*  Caller ensures that human isn't us, not on our team, not a vassal
	and can be contacted. We're alive, not human, not a vassal, team leader. */
bool CvPlayerAI::proposeJointWar(PlayerTypes humanId) {

	if(AI_getContactTimer(humanId, CONTACT_JOIN_WAR) > 0)
		return false;
	CvTeamAI const& ourTeam = GET_TEAM(getTeam());
	if(ourTeam.getAtWarCount(true, true) == 0)
		return false;
	CvPlayerAI const& human = GET_PLAYER(humanId);
	CvGame& g = GC.getGameINLINE();
	// <advc.130m> Replacing this with an attitude check at the end:
		//AI_getMemoryCount(humanId, MEMORY_DECLARED_WAR) > 0 ||
		//AI_getMemoryCount(humanId, MEMORY_HIRED_WAR_ALLY) > 0
	/*  Want contact probability to increase with war duration, but we haven't
		decided on a war target yet. Use the average, and later pick the target
		based on war duration. */
	double totalAtWarTurns = 0;
	int nWars = 0;
	for(int i = 0; i < MAX_CIV_TEAMS; i++) {
		CvTeam const& t = GET_TEAM((TeamTypes)i);
		if(t.isAlive() && !t.isAVassal() && t.isAtWar(getTeam())) {
			double awt = std::min(20, ourTeam.AI_getAtWarCounter(t.getID()));
			/*  Already sharing a war with humanId makes us reluctant to ask them
				to join another war. */
			if(t.isAtWar(TEAMID(humanId)))
				awt *= -0.5;
			totalAtWarTurns += awt;
			nWars++;
		}
	}
	FAssert(nWars > 0);
	double avgAtWarTurns = std::max(1.0, totalAtWarTurns / nWars);
	if(g.getSorenRandNum(::round(GC.getLeaderHeadInfo(getPersonalityType()).
			/*  That's one tenth of the BtS probability on turn one of a war,
				the BtS probability after 10 turns, and then grows to twice that. */
			getContactRand(CONTACT_JOIN_WAR) * 10 / avgAtWarTurns), // </advc.130m>
			"AI Diplo Join War") != 0)
		return false;
	int bestTargetVal = -1;
	TeamTypes bestTarget = NO_TEAM;
	for(int i = 0; i < MAX_CIV_TEAMS; i++) {
		TeamTypes targetId = (TeamTypes)i;
		CvTeamAI const& target = GET_TEAM(targetId);
		if(!target.isAlive() || !atWar(targetId, getTeam()) ||
				atWar(targetId, TEAMID(humanId)) ||
				!TEAMREF(humanId).isHasMet(targetId) ||
				!TEAMREF(humanId).canDeclareWar(targetId) ||
				// <advc.130m>
				/*  Asking for a human-human war (or embargo) isn't smart;
					they could immediately reconcile */
				target.isHuman() ||
				// Give humanId some time to prepare before we press them
				ourTeam.AI_getAtWarCounter(targetId) < 5 ||
				target.isAVassal()) 
			continue;
		// Avoid asking if they've recently made peace
		double prSkip = 1 - (TEAMREF(humanId).AI_getAtPeaceCounter(targetId) -
				GC.getDefineINT("PEACE_TREATY_LENGTH")) * 0.067;
		if(::bernoulliSuccess(prSkip, "advc.130m (peace)"))
			continue; // </advc.130m>
		int targetVal = g.getSorenRandNum(10000, "AI Joining War");
		// advc.130m:
		targetVal += std::min(20, ourTeam.AI_getAtWarCounter(targetId)) * 1000;
		if(targetVal > bestTargetVal) {
			bestTargetVal = targetVal;
			bestTarget = targetId;
		}
	}
	// <advc.130m> Ask for embargo instead
	AttitudeTypes towardsHuman = AI_getAttitude(humanId);
	if(towardsHuman <= ATTITUDE_ANNOYED) {
		if(ourTeam.AI_getWorstEnemy() == bestTarget && towardsHuman == ATTITUDE_ANNOYED)
 			return proposeEmbargo(humanId);
		return false;
	}
	if(getWPAI.isEnabled() && bestTarget != NO_TEAM) {
		if(TEAMREF(humanId).warAndPeaceAI().
				declareWarTrade(bestTarget, getTeam()) != NO_DENIAL)
			return proposeEmbargo(humanId);
	} // </advc.130m>
	if(bestTarget == NO_TEAM)
		return false;
	AI_changeContactTimer(humanId, CONTACT_JOIN_WAR,
			GC.getLeaderHeadInfo(getPersonalityType()).
			getContactDelay(CONTACT_JOIN_WAR));
	CvDiploParameters* pDiplo = new CvDiploParameters(getID());
	/*  NB: The DLL never deletes CvDiploParameters objects; presumably
		handled by beginDiplomacy */
	pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString(
			"AI_DIPLOCOMMENT_JOIN_WAR"), GET_PLAYER(GET_TEAM(bestTarget).
			getLeaderID()).getCivilizationAdjectiveKey());
	pDiplo->setAIContact(true);
	pDiplo->setData(bestTarget);
	gDLL->beginDiplomacy(pDiplo, humanId);
	return true;
}

/*  Code cut-and-pasted from AI_doDiplo; now needed in an additional place.
	Same assumptions as for contactReligion, plus can't be between vassal + master. */
bool CvPlayerAI::proposeEmbargo(PlayerTypes humanId) {

	if(AI_getContactTimer(humanId, CONTACT_STOP_TRADING) > 0 ||
			AI_getAttitude(humanId) <= ATTITUDE_FURIOUS) // advc.130f
		return false;
	CvTeamAI const& ourTeam = GET_TEAM(getTeam());
	TeamTypes eBestTeam = ourTeam.AI_getWorstEnemy();
	if(eBestTeam == NO_TEAM || !TEAMREF(humanId).isHasMet(eBestTeam) ||
			GET_TEAM(eBestTeam).isVassal(TEAMID(humanId)) ||
			!GET_PLAYER(humanId).canStopTradingWithTeam(eBestTeam) ||
			// advc.104m:
			ourTeam.AI_isSneakAttackReady(TEAMID(humanId)) ||
			// <advc.130f>
			ourTeam.AI_getAttitudeVal(TEAMID(humanId)) -
			ourTeam.AI_getAttitudeVal(eBestTeam) < 2) // </advc.130f>
		return false;
	FAssert(!atWar(TEAMID(humanId), eBestTeam));
	FAssert(TEAMID(humanId) != eBestTeam);
	// <advc.130f>
	/*  During war preparation against human, ask to stop trading only if this
		will improve our attitude. */
	/*if(GET_TEAM(getTeam()).AI_getWarPlan(TEAMID(humanId)) != NO_WARPLAN &&
			AI_getAttitude(humanId) >= AI_getAttitudeFromValue(AI_getAttitudeVal(humanId))
			+ GC.getLeaderHeadInfo(getPersonalityType()).
			getMemoryAttitudePercent(MEMORY_ACCEPTED_STOP_TRADING) / 100)
		return false;*/
	/*  ^ Never mind; leave it up to the human to figure out if the AI might
		declare war anyway. */
	/*  Mustn't propose embargo if our deals can't be canceled.
		Perhaps this check should be in CvPlayer::canTradeItem instead? */
	CvGame& g = GC.getGameINLINE(); int dummy;
	for(CvDeal* d = g.firstDeal(&dummy); d != NULL; d = g.nextDeal(&dummy)) {
		if((d->getFirstPlayer() != getID() || TEAMID(d->getSecondPlayer()) != eBestTeam) &&
				(d->getSecondPlayer() != getID() || TEAMID(d->getFirstPlayer()) != eBestTeam))
			continue;
		if(!d->isCancelable(getID())) 
			return false;
	} // </advc.130f>
	AI_changeContactTimer(humanId, CONTACT_STOP_TRADING,
			GC.getLeaderHeadInfo(getPersonalityType()).
			getContactDelay(CONTACT_STOP_TRADING));
	CvDiploParameters* pDiplo = new CvDiploParameters(getID());
	pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString(
			"AI_DIPLOCOMMENT_STOP_TRADING"),
			GET_PLAYER(GET_TEAM(eBestTeam).getLeaderID()).
			getCivilizationAdjectiveKey());
	pDiplo->setAIContact(true);
	pDiplo->setData(eBestTeam);
	gDLL->beginDiplomacy(pDiplo, humanId);
	return true;
}


/*  Cut, pasted and refactored from AI_doDiplo; no functional change.
	Caller ensures isAlive, canContactAndTalk, not at war, not minor/ barb,
	!this->isHuman. If civId is human, !abContacted is ensured, but civId may
	also be an AI civ. */
bool CvPlayerAI::proposeResourceTrade(PlayerTypes civId) {

	if(AI_getContactTimer(civId, CONTACT_TRADE_BONUS) > 0)
		return false;
	CvPlayerAI const& civ = GET_PLAYER(civId);
	CvGame& g = GC.getGame();
	int iBestValue = 0;
	BonusTypes eBestReceiveBonus = NO_BONUS;
	TradeData item;
	for(int i = 0; i < GC.getNumBonusInfos(); i++) {
		BonusTypes eBonus = (BonusTypes)i;
		/*  <advc.036> For AI-AI trades the conditions below are better handled
			by AI_getTradeDenial. */
		if(civ.isHuman() && (civ.getNumTradeableBonuses(eBonus) <= 1 ||
				civ.AI_corporationBonusVal(eBonus) != 0))
				// || AI_bonusTradeVal(eBonus, civId, 1) <= 0)
			continue;
		int bias = AI_bonusTradeVal(eBonus, civId, 1) -
				civ.AI_bonusTradeVal(eBonus, getID(), -1);
		if(!::bernoulliSuccess((bias - 15) / 100.0, "advc.036"))
			continue; // </advc.036>
		setTradeItem(&item, TRADE_RESOURCES, eBonus);
		if(civ.canTradeItem(getID(), item, true)) {
			// <advc.036> Was completely random before
			int iValue = g.getSorenRandNum(25, "AI Bonus Trading #1")
					+ bias; // </advc.036>
			if(iValue > iBestValue) {
				iBestValue = iValue;
				eBestReceiveBonus = eBonus;
			}
		}
	}
	// <advc.036> Commented out
	/*if(eBestReceiveBonus != NO_BONUS) {
	iBestValue = 0;*/
	/*  If human doesn't have a surplus resource we want, then don't try to sell
		him/her something. */
	if(eBestReceiveBonus == NO_BONUS && civ.isHuman())
		return false;
	BonusTypes eBestGiveBonus = NO_BONUS;
	for(int i = 0; i < GC.getNumBonusInfos(); i++) {
		BonusTypes eBonus = (BonusTypes)i;
		// <advc.036> See comment about eBestReceiveBonus
		/*if(i == eBestReceiveBonus || getNumTradeableBonuses(eBonus) <= 1 ||
				civ.AI_bonusTradeVal(eBonus, getID(), 1) <= 0)
			continue;*/
		int bias = civ.AI_bonusTradeVal(eBonus, getID(), 1) -
				AI_bonusTradeVal(eBonus, civId, -1);
		if(!::bernoulliSuccess((bias - 15)/100.0, "advc.036"))
			continue; // </advc.036>
		setTradeItem(&item, TRADE_RESOURCES, eBonus);
		if(canTradeItem(civId, item, true)) {
			// <advc.036>
			int iValue = (g.getSorenRandNum(30, "AI Bonus Trading #2")
					+ bias); // </advc.036>
			if(iValue > iBestValue) {
				iBestValue = iValue;
				eBestGiveBonus = eBonus;
			}
		}
	}
	CLinkList<TradeData> ourList;
	CLinkList<TradeData> theirList;
	/*if(eBestGiveBonus != NO_BONUS && (!civ.isHuman() ||
			AI_bonusTradeVal(eBestReceiveBonus, civId, -1) >=
			civ.AI_bonusTradeVal(eBestGiveBonus, getID(), 1))) {
		ourList.clear();
		theirList.clear();
		setTradeItem(&item, TRADE_RESOURCES, eBestGiveBonus);
		ourList.insertAtEnd(item);
		setTradeItem(&item, TRADE_RESOURCES, eBestReceiveBonus);
		theirList.insertAtEnd(item);
	}*/
	/*  <advc.036> Replacing the above.
		Rather than trying to trade eBestGiveBonus against eBestReceiveBonus,
		pick the one with the higher trade value difference (iBestValue) and let
		AI_counterPropose produce an offer. */
	bool bDeal = false;
	if(eBestGiveBonus != NO_BONUS) {
		ourList.clear();
		theirList.clear();
		setTradeItem(&item, TRADE_RESOURCES, eBestGiveBonus);
		ourList.insertAtEnd(item);
		/*  Code (K-Mod) copied from the tech trade block above.
			Can't move this stuff into the AI_counterPropose b/c
			the lists need to be allocated here. */
		CLinkList<TradeData> ourInventory, theirInventory, ourCounter, theirCounter;
		// Leave our inventory empty; we're not giving anything else.
		civ.buildTradeTable(getID(), theirInventory);
		civ.markTradeOffers(theirInventory, theirList);
		civ.updateTradeList(getID(), theirInventory, theirList, ourList);
		if(AI_counterPropose(civId, &theirList, &ourList,
				&theirInventory, &ourInventory,
				&theirCounter, &ourCounter)) {
			theirList.concatenate(theirCounter);
			bDeal = true;
		}
	}
	if(!bDeal && eBestReceiveBonus != NO_BONUS) {
		ourList.clear();
		theirList.clear();
		setTradeItem(&item, TRADE_RESOURCES, eBestReceiveBonus);
		theirList.insertAtEnd(item);
		CLinkList<TradeData> ourInventory, theirInventory, ourCounter, theirCounter;
		// Leave their inventory empty
		buildTradeTable(civId, ourInventory);
		markTradeOffers(ourInventory, ourList);
		updateTradeList(civId, ourInventory, ourList, theirList);
		if(civ.AI_counterPropose(getID(), &ourList, &theirList,
				&ourInventory, &theirInventory,
				&ourCounter, &theirCounter)) {
			ourList.concatenate(ourCounter);
			bDeal = true;
		}
	}
	if(!bDeal)
		return false; // </advc.036>
	if(civ.isHuman()) {
		AI_changeContactTimer(civId, CONTACT_TRADE_BONUS, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_TRADE_BONUS));
		CvDiploParameters* pDiplo = new CvDiploParameters(getID());
		pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString(
				"AI_DIPLOCOMMENT_OFFER_DEAL"));
		pDiplo->setAIContact(true);
		pDiplo->setOurOfferList(theirList);
		pDiplo->setTheirOfferList(ourList);
		gDLL->beginDiplomacy(pDiplo, civId);
		return true;
	}
	g.implementDeal(getID(), civId, &ourList, &theirList);
	return false; // Only true when a human was contacted
} // </advc.133>

/*  Cut, pasted and refactored from AI_doDiplo; no functional change.
	Caller ensures isHuman/ !isHuman, isAlive, canContactAndTalk, 
	unequal team ids, not at war, not minor/ barb. */
bool CvPlayerAI::contactReligion(PlayerTypes humanId) {

	if(AI_getContactTimer(humanId, CONTACT_RELIGION_PRESSURE) > 0 ||
			getStateReligion() == NO_RELIGION)
		return false;
	CvPlayer const& human = GET_PLAYER(humanId);
	if(!human.canConvert(getStateReligion()))
		return false;
	AI_changeContactTimer(humanId, CONTACT_RELIGION_PRESSURE,
			GC.getLeaderHeadInfo(getPersonalityType()).
			getContactDelay(CONTACT_RELIGION_PRESSURE));
	CvDiploParameters* pDiplo = new CvDiploParameters(getID());
	pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString(
			"AI_DIPLOCOMMENT_RELIGION_PRESSURE"));
	pDiplo->setAIContact(true);
	gDLL->beginDiplomacy(pDiplo, humanId);
	return true;
}

// Same conditions as contactReligion
bool CvPlayerAI::contactCivics(PlayerTypes humanId) {

	if(AI_getContactTimer(humanId, CONTACT_CIVIC_PRESSURE) > 0)
		return false;
	CivicTypes eFavoriteCivic = (CivicTypes)(GC.getLeaderHeadInfo(getPersonalityType()).
			getFavoriteCivic());
	if(eFavoriteCivic == NO_CIVIC || !isCivic(eFavoriteCivic))
		return false;
	CvPlayer const& human = GET_PLAYER(humanId);
	if(!human.canDoCivics(eFavoriteCivic) || human.isCivic(eFavoriteCivic) ||
			!human.canRevolution(NULL))
		return false;
	AI_changeContactTimer(humanId, CONTACT_CIVIC_PRESSURE,
			GC.getLeaderHeadInfo(getPersonalityType()).
			getContactDelay(CONTACT_CIVIC_PRESSURE));
	CvDiploParameters* pDiplo = new CvDiploParameters(getID());
	pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString(
			"AI_DIPLOCOMMENT_CIVIC_PRESSURE"),
			GC.getCivicInfo(eFavoriteCivic).getTextKeyWide());
	pDiplo->setAIContact(true);
	gDLL->beginDiplomacy(pDiplo, humanId);
	return true;
}

/*  Same conditions as contactReligion (AI_doDiplo checks additional ones,
	but these aren't necessary for this function to work correctly) */
bool CvPlayerAI::askHelp(PlayerTypes humanId) {

	// <advc.104m>
	if(GET_TEAM(getTeam()).AI_isSneakAttackReady(TEAMID(humanId)))
		return false;
	if(getWPAI.isEnabled() && ::bernoulliSuccess(0.2, "advc.130m"))
		return false; // </advc.104m>
	// <advc.705>
	CvGame const& g = GC.getGameINLINE();
	if(g.isOption(GAMEOPTION_RISE_FALL) && g.getRiseFall().
			isCooperationRestricted(getID()) &&
			::bernoulliSuccess(0.5, "advc.705"))
		return false; // </advc.705>
	if(AI_getContactTimer(humanId, CONTACT_ASK_FOR_HELP) > 0 ||
			TEAMREF(humanId).getAssets() <= (GET_TEAM(getTeam()).getAssets() / 2))
		return false;
	int iBestValue = 0;
	TechTypes eBestReceiveTech = NO_TECH;
	TradeData item;
	CvPlayer const& human = GET_PLAYER(humanId);
	for(int i = 0; i < GC.getNumTechInfos(); i++) {	
		setTradeItem(&item, TRADE_TECHNOLOGIES, i);
		if(!human.canTradeItem(getID(), item, true))
			continue;
		int iValue = 1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Asking For Help");
		if(iValue > iBestValue) {
			iBestValue = iValue;
			eBestReceiveTech = (TechTypes)i;
		}
	}
	if(eBestReceiveTech == NO_TECH)
		return false;
	CLinkList<TradeData> theirList;
	setTradeItem(&item, TRADE_TECHNOLOGIES, eBestReceiveTech);
	theirList.insertAtEnd(item);
	AI_changeContactTimer(humanId, CONTACT_ASK_FOR_HELP,
			GC.getLeaderHeadInfo(getPersonalityType()).
			getContactDelay(CONTACT_ASK_FOR_HELP));
	CvDiploParameters* pDiplo = new CvDiploParameters(getID());
	pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString(
			"AI_DIPLOCOMMENT_ASK_FOR_HELP"));
	pDiplo->setAIContact(true);
	pDiplo->setOurOfferList(theirList);
	gDLL->beginDiplomacy(pDiplo, humanId);
	return true;
}

// Same necessary conditions as the others
bool CvPlayerAI::demandTribute(PlayerTypes humanId, int tributeType) {

	// <advc.104m>
	if(GET_TEAM(getTeam()).AI_isSneakAttackReady(TEAMID(humanId)))
		return false;
	if(getWPAI.isEnabled() && ::bernoulliSuccess(0.5, "advc.104m"))
		return false; // </advc.104m>
	if(AI_getContactTimer(humanId, CONTACT_DEMAND_TRIBUTE) > 0 ||
			AI_getAttitude(humanId) > GC.getLeaderHeadInfo(getPersonalityType()).
			getDemandTributeAttitudeThreshold())
		return false;
	CvPlayerAI const& human = GET_PLAYER(humanId);
	TradeData item;
	CLinkList<TradeData> theirList;
	switch(tributeType) {
	case 0: {
		int iReceiveGold = range(human.getGold() - 50, 0, human.AI_goldTarget());
		iReceiveGold -= (iReceiveGold % GC.getDIPLOMACY_VALUE_REMAINDER());
		if(iReceiveGold <= 50)
			break;
		setTradeItem(&item, TRADE_GOLD, iReceiveGold);
		/*  advc.001: BtS checks this only for some of the items, and it does so
			with bTestDenial, which seems pointless when the callee is human. */
		if(human.canTradeItem(getID(), item))
			theirList.insertAtEnd(item);
		break;
	} case 1: {
		setTradeItem(&item, TRADE_MAPS);
		if(human.canTradeItem(getID(), item)) { // advc.001
			if(GET_TEAM(getTeam()).AI_mapTradeVal(human.getTeam()) > 100)
				theirList.insertAtEnd(item);
		}
		break;
	} case 2: {
		int iBestValue = 0;
		TechTypes eBestReceiveTech = NO_TECH;
		for(int i = 0; i < GC.getNumTechInfos(); i++) {
			setTradeItem(&item, TRADE_TECHNOLOGIES, i);
			if(!human.canTradeItem(getID(), item))
				continue;
			TechTypes eTech = (TechTypes)i;
			if(GC.getGameINLINE().countKnownTechNumTeams(eTech) <= 1)
				continue;
			int iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000,
					"AI Demanding Tribute (Tech)"));
			if(iValue > iBestValue) {
				iBestValue = iValue;
				eBestReceiveTech = eTech;
			}
		}
		if(eBestReceiveTech != NO_TECH) {
			setTradeItem(&item, TRADE_TECHNOLOGIES, eBestReceiveTech);
			theirList.insertAtEnd(item);
		}
		break;
	} case 3: {
		int iBestValue = 0;
		BonusTypes eBestReceiveBonus = NO_BONUS;
		for(int i = 0; i < GC.getNumBonusInfos(); i++) {
			setTradeItem(&item, TRADE_RESOURCES, i);
			if(!human.canTradeItem(getID(), item))
				continue;
			BonusTypes eBonus = (BonusTypes)i;
			if(human.getNumTradeableBonuses(eBonus) <= 1 ||
					AI_bonusTradeVal(eBonus, humanId, 1) <= 0)
				continue;
			int iValue = 1 + GC.getGameINLINE().getSorenRandNum(10000,
					"AI Demanding Tribute (Bonus)");
			if(iValue > iBestValue) {
				iBestValue = iValue;
				eBestReceiveBonus = eBonus;
			}
		}
		if(eBestReceiveBonus != NO_BONUS) {
			setTradeItem(&item, TRADE_RESOURCES, eBestReceiveBonus);
			theirList.insertAtEnd(item);
		}
		break; // <advc.104m> New: demand gpt
	} case 4: {
		int gpt = (human.AI_getAvailableIncome() -
				human.calculateInflatedCosts()) / 10;
		int mod = GC.getDIPLOMACY_VALUE_REMAINDER();
		if(gpt >= mod) {
			gpt -= gpt % mod;
			setTradeItem(&item, TRADE_GOLD_PER_TURN, gpt);
			if(human.canTradeItem(getID(), item))
				theirList.insertAtEnd(item);
		}
		break; // </advc.104m>
	} default:
		return false;
	}
	if(theirList.getLength() <= 0)
		return false;
	// <advc.104m> Don't ask for too little
	// Unless difficulty is low
	if(GC.getHandicapInfo(human.getHandicapType()).getDifficulty() > 25) {
		int dealVal = AI_dealVal(humanId, &theirList, false, 1, true);
		if(dealVal < 50 * std::pow(2.0, getCurrentEra()))
			return false;
	} // </advc.104m>
	AI_changeContactTimer(humanId, CONTACT_DEMAND_TRIBUTE,
			GC.getLeaderHeadInfo(getPersonalityType()).
			getContactDelay(CONTACT_DEMAND_TRIBUTE));
	CvDiploParameters* pDiplo = new CvDiploParameters(getID());
	pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString(
			"AI_DIPLOCOMMENT_DEMAND_TRIBUTE"));
	pDiplo->setAIContact(true);
	pDiplo->setOurOfferList(theirList);
	gDLL->beginDiplomacy(pDiplo, humanId);
	return true;
} // </advc.003>

// <advc.104><advc.031>
/*  Assets like additional cities become less valuable over the course of a game
	b/c there are fewer and fewer turns to go. Not worth considering in the
	first half of the game, but e.g. after 300 turns (normal settings),
	the multiplier will have decreased to 0.8. The current implementation isn't
	specific to a CvPlayerAI, but this could change, i.e. one could estimate the
	end turn based on the knowledge of this CvPlayerAI.
	delay: When an asset is not immediately acquired. */
double CvPlayerAI::amortizationMultiplier(int delay) const {

	return ::dRange(2 * // Use this coefficient to fine-tune the effect
			(1 - GC.getGameINLINE().gameTurnProgress(std::max(0, delay))),
			0.15, 1.0);
} // </advc.104></advc.031>

//
// read object from a stream
// used during load
//
void CvPlayerAI::read(FDataStreamBase* pStream)
{
	CvPlayer::read(pStream);	// read base class data first

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iPeaceWeight);
	pStream->Read(&m_iEspionageWeight);
	pStream->Read(&m_iAttackOddsChange);
	pStream->Read(&m_iCivicTimer);
	pStream->Read(&m_iReligionTimer);
	pStream->Read(&m_iExtraGoldTarget);
	// K-Mod
	if (uiFlag >= 6)
		pStream->Read(&m_iCityTargetTimer);
	else
		AI_setCityTargetTimer(0);
	// K-Mod end

	pStream->Read(&m_iStrategyHash);
	//pStream->Read(&m_iStrategyHashCacheTurn); // disabled by K-Mod
// BBAI (edited for K-Mod)
	pStream->Read(&m_iStrategyRand);
	pStream->Read(&m_iVictoryStrategyHash);
// BBAI end
	//pStream->Read(&m_iAveragesCacheTurn); // disabled by K-Mod
	pStream->Read(&m_iAverageGreatPeopleMultiplier);
	pStream->Read(&m_iAverageCulturePressure); // K-Mod

	pStream->Read(NUM_YIELD_TYPES, m_aiAverageYieldMultiplier);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiAverageCommerceMultiplier);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiAverageCommerceExchange);

	//pStream->Read(&m_iUpgradeUnitsCacheTurn); // disabled by K-Mod
	//pStream->Read(&m_iUpgradeUnitsCachedExpThreshold); // disabled by K-Mod
	pStream->Read(&m_iUpgradeUnitsCachedGold);
	// K-Mod
	if (uiFlag >= 7)
		pStream->Read(&m_iAvailableIncome);
	// K-Mod end

	pStream->Read(NUM_UNITAI_TYPES, m_aiNumTrainAIUnits);
	pStream->Read(NUM_UNITAI_TYPES, m_aiNumAIUnits);

	pStream->Read(MAX_PLAYERS, m_aiSameReligionCounter);
	pStream->Read(MAX_PLAYERS, m_aiDifferentReligionCounter);
	pStream->Read(MAX_PLAYERS, m_aiFavoriteCivicCounter);
	pStream->Read(MAX_PLAYERS, m_aiBonusTradeCounter);
	pStream->Read(MAX_PLAYERS, m_aiPeacetimeTradeValue);
	pStream->Read(MAX_PLAYERS, m_aiPeacetimeGrantValue);
	pStream->Read(MAX_PLAYERS, m_aiGoldTradedTo);
	pStream->Read(MAX_PLAYERS, m_aiAttitudeExtra);
	// K-Mod. Load the attitude cache. (originally, in BBAI and the CAR Mod, this was not saved.
	// But there are rare situations in which it needs to be saved/read to avoid OOS errors.)
	if (uiFlag >= 4)
		pStream->Read(MAX_PLAYERS, &m_aiAttitudeCache[0]);
	// K-Mod end
	pStream->Read(MAX_PLAYERS, m_abFirstContact);

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		pStream->Read(NUM_CONTACT_TYPES, m_aaiContactTimer[i]);
	}
	for (int i = 0; i < MAX_PLAYERS; i++)
	{	
		pStream->Read(NUM_MEMORY_TYPES, m_aaiMemoryCount[i]);
	}

	pStream->Read(&m_bWasFinancialTrouble);
	pStream->Read(&m_iTurnLastProductionDirty);
	
	{
		m_aiAICitySites.clear();
		uint iSize;
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			int iCitySite;
			pStream->Read(&iCitySite);
			m_aiAICitySites.push_back(iCitySite);
		}
	}

	pStream->Read(GC.getNumBonusInfos(), m_aiBonusValue);
	// <advc.036>
	if(uiFlag >= 10)
		pStream->Read(GC.getNumBonusInfos(), m_aiBonusValueTrade); // </advc.036>
	pStream->Read(GC.getNumUnitClassInfos(), m_aiUnitClassWeights);
	pStream->Read(GC.getNumUnitCombatInfos(), m_aiUnitCombatWeights);
	// K-Mod
	m_GreatPersonWeights.clear();
	if (uiFlag >= 5)
	{
		int iItems;
		int iType, iWeight;
		pStream->Read(&iItems);
		for (int i = 0; i < iItems; i++)
		{
			pStream->Read(&iType);
			pStream->Read(&iWeight);
			m_GreatPersonWeights.insert(std::make_pair((UnitClassTypes)iType, iWeight));
		}
	}
	// K-Mod end
	//pStream->Read(MAX_PLAYERS, m_aiCloseBordersAttitudeCache);
	pStream->Read(MAX_PLAYERS, &m_aiCloseBordersAttitudeCache[0]); // K-Mod
	// <advc.003b>
	if(uiFlag >= 8) {
		int sz = 0;
		pStream->Read(&sz);
		for(int i = 0; i < sz; i++) {
			int first = -1, second = 0;
			pStream->Read(&first);
			pStream->Read(&second);
			FAssert(first >= 0);
			neededExplorersByArea[first] = second;
		}
	} else if(isAlive())
		AI_updateNeededExplorers();
	// </advc.003b>
	// <advc.104>
	if(isEverAlive() && !isBarbarian() && !isMinorCiv()) {
		wpai.read(pStream); 
		/*  Would be enough to do this once after all players have been
			loaded, but don't know which function, if any, the EXE calls
			after loading all players and before calling CvPlayerAI::isWillingToTalk,
			which requires WarAndPeaceAI to be up-to-date.
			Perhaps enough to do it once after loading BARBARIAN_PLAYER? */
		getWPAI.update();
	} // </advc.104>
	// <advc.148>
	if(uiFlag < 9 && getID() == MAX_PLAYERS - 1) {
		for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
			CvPlayerAI& civ = GET_PLAYER((PlayerTypes)i);
			if(civ.isAlive())
				civ.AI_updateAttitudeCache();
		}
	} // </advc.148>
}


//
// save object to a stream
// used during save
//
void CvPlayerAI::write(FDataStreamBase* pStream)
{
	CvPlayer::write(pStream);	// write base class data first

/*
	uint uiFlag=0;
*/
	// Flag for type of save
	uint uiFlag=7;
	uiFlag = 8; // advc.003b
	uiFlag = 9; // advc.148
	uiFlag = 10; // advc.036
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iPeaceWeight);
	pStream->Write(m_iEspionageWeight);
	pStream->Write(m_iAttackOddsChange);
	pStream->Write(m_iCivicTimer);
	pStream->Write(m_iReligionTimer);
	pStream->Write(m_iExtraGoldTarget);
	pStream->Write(m_iCityTargetTimer); // K-Mod. uiFlag >= 6

	pStream->Write(m_iStrategyHash);
	//pStream->Write(m_iStrategyHashCacheTurn); // disabled by K-Mod
// BBAI
	pStream->Write(m_iStrategyRand);
	pStream->Write(m_iVictoryStrategyHash);
// BBAI end
	//pStream->Write(m_iAveragesCacheTurn);
	pStream->Write(m_iAverageGreatPeopleMultiplier);
	pStream->Write(m_iAverageCulturePressure); // K-Mod

	pStream->Write(NUM_YIELD_TYPES, m_aiAverageYieldMultiplier);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiAverageCommerceMultiplier);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiAverageCommerceExchange);

	//pStream->Write(m_iUpgradeUnitsCacheTurn); // disabled by K-Mod
	//pStream->Write(m_iUpgradeUnitsCachedExpThreshold); // disabled by K-Mod
	pStream->Write(m_iUpgradeUnitsCachedGold);
	pStream->Write(m_iAvailableIncome); // K-Mod. uiFlag >= 7

	pStream->Write(NUM_UNITAI_TYPES, m_aiNumTrainAIUnits);
	pStream->Write(NUM_UNITAI_TYPES, m_aiNumAIUnits);
	pStream->Write(MAX_PLAYERS, m_aiSameReligionCounter);
	pStream->Write(MAX_PLAYERS, m_aiDifferentReligionCounter);
	pStream->Write(MAX_PLAYERS, m_aiFavoriteCivicCounter);
	pStream->Write(MAX_PLAYERS, m_aiBonusTradeCounter);
	pStream->Write(MAX_PLAYERS, m_aiPeacetimeTradeValue);
	pStream->Write(MAX_PLAYERS, m_aiPeacetimeGrantValue);
	pStream->Write(MAX_PLAYERS, m_aiGoldTradedTo);
	pStream->Write(MAX_PLAYERS, m_aiAttitudeExtra);
	// K-Mod. save the attitude cache. (to avoid OOS problems)
	pStream->Write(MAX_PLAYERS, &m_aiAttitudeCache[0]); // uiFlag >= 4
	// K-Mod end
	pStream->Write(MAX_PLAYERS, m_abFirstContact);

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		pStream->Write(NUM_CONTACT_TYPES, m_aaiContactTimer[i]);
	}
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		pStream->Write(NUM_MEMORY_TYPES, m_aaiMemoryCount[i]);
	}

	pStream->Write(m_bWasFinancialTrouble);
	pStream->Write(m_iTurnLastProductionDirty);
	
	{
		uint iSize = m_aiAICitySites.size();
		pStream->Write(iSize);
		std::vector<int>::iterator it;
		for (it = m_aiAICitySites.begin(); it != m_aiAICitySites.end(); ++it)
		{
			pStream->Write((*it));
		}
	}

	pStream->Write(GC.getNumBonusInfos(), m_aiBonusValue);
	pStream->Write(GC.getNumBonusInfos(), m_aiBonusValueTrade); // advc.036
	pStream->Write(GC.getNumUnitClassInfos(), m_aiUnitClassWeights);
	pStream->Write(GC.getNumUnitCombatInfos(), m_aiUnitCombatWeights);
	// K-Mod. save great person weights. (uiFlag >= 5)
	{
		int iItems = m_GreatPersonWeights.size();
		pStream->Write(iItems);
		std::map<UnitClassTypes, int>::const_iterator it;
		for (it = m_GreatPersonWeights.begin(); it != m_GreatPersonWeights.end(); ++it)
		{
			pStream->Write((int)it->first);
			pStream->Write(it->second);
		}
	}
	// K-Mod end
	//pStream->Write(MAX_PLAYERS, m_aiCloseBordersAttitudeCache);
	pStream->Write(MAX_PLAYERS, &m_aiCloseBordersAttitudeCache[0]); // K-Mod
	// <advc.003b>
	pStream->Write((int)neededExplorersByArea.size());
	for(std::map<int,int>::const_iterator it = neededExplorersByArea.begin();
			it != neededExplorersByArea.end(); it++) {
		pStream->Write(it->first);
		pStream->Write(it->second);
	} // </advc.003b>
	// <advc.104>
	if(isEverAlive() && !isBarbarian() && !isMinorCiv())
		wpai.write(pStream); // </advc.104>
}

// (K-Mod note: this should be roughly in units of commerce.)
int CvPlayerAI::AI_eventValue(EventTypes eEvent, const EventTriggeredData& kTriggeredData) const
{
	CvEventTriggerInfo& kTrigger = GC.getEventTriggerInfo(kTriggeredData.m_eTrigger);
	CvEventInfo& kEvent = GC.getEventInfo(eEvent);

	CvTeamAI& kTeam = GET_TEAM(getTeam()); // K-Mod

	int iNumCities = getNumCities();
	CvCity* pCity = getCity(kTriggeredData.m_iCityId);
	CvPlot* pPlot = GC.getMapINLINE().plot(kTriggeredData.m_iPlotX, kTriggeredData.m_iPlotY);
	CvUnit* pUnit = getUnit(kTriggeredData.m_iUnitId);

	int iHappy = 0;
	int iHealth = 0;
	int aiYields[NUM_YIELD_TYPES];
	int aiCommerceYields[NUM_COMMERCE_TYPES];
	
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		aiYields[iI] = 0;
	}
	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		aiCommerceYields[iI] = 0;
	}

	/* original bts code
	if (NO_PLAYER != kTriggeredData.m_eOtherPlayer)
	{
		if (kEvent.isDeclareWar())
		{
			switch (AI_getAttitude(kTriggeredData.m_eOtherPlayer))
			{
			case ATTITUDE_FURIOUS:
			case ATTITUDE_ANNOYED:
			case ATTITUDE_CAUTIOUS:
				if (GET_TEAM(getTeam()).getDefensivePower() < GET_TEAM(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam()).getPower(true))
				{
					return -MAX_INT + 1;
				}
				break;
			case ATTITUDE_PLEASED:
			case ATTITUDE_FRIENDLY:
				return -MAX_INT + 1;
				break;
			}
		}
	} */ // this is handled later.

	//Proportional to #turns in the game...
	//(AI evaluation will generally assume proper game speed scaling!)
	int iGameSpeedPercent = GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getVictoryDelayPercent();

	int iValue = GC.getGameINLINE().getSorenRandNum(kEvent.getAIValue(), "AI Event choice");
	iValue += (getEventCost(eEvent, kTriggeredData.m_eOtherPlayer, false) + getEventCost(eEvent, kTriggeredData.m_eOtherPlayer, true)) / 2;

	//iValue += kEvent.getEspionagePoints();
	iValue += kEvent.getEspionagePoints() * AI_commerceWeight(COMMERCE_ESPIONAGE) / 100; // K-Mod

	if (kEvent.getTech() != NO_TECH)
	{
		iValue += (kTeam.getResearchCost((TechTypes)kEvent.getTech()) * kEvent.getTechPercent()) / 100;
	}

	if (kEvent.getUnitClass() != NO_UNITCLASS)
	{
		UnitTypes eUnit = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(kEvent.getUnitClass());
		if (eUnit != NO_UNIT)
		{
			//Altough AI_unitValue compares well within units, the value is somewhat independent of cost
			int iUnitValue = GC.getUnitInfo(eUnit).getProductionCost();
			if (iUnitValue > 0)
			{
				iUnitValue *= 2;
			}
			else if (iUnitValue == -1)
			{
				iUnitValue = 2000; //Great Person?  // (was 200)
			}

			iUnitValue *= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getTrainPercent();
			iUnitValue /= 100; // K-Mod
			iValue += kEvent.getNumUnits() * iUnitValue;
		}
	}
	
	if (kEvent.isDisbandUnit())
	{
		CvUnit* pUnit = getUnit(kTriggeredData.m_iUnitId);
		if (NULL != pUnit)
		{
			int iUnitValue = pUnit->getUnitInfo().getProductionCost();
			if (iUnitValue > 0)
			{
				iUnitValue *= 2;
			}
			else if (iUnitValue == -1)
			{
				iUnitValue = 2000; //Great Person?  // (was 200)
			}

			iUnitValue *= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getTrainPercent();
			iUnitValue /= 100; // K-Mod
			iValue -= iUnitValue;
		}
	}

	if (kEvent.getBuildingClass() != NO_BUILDINGCLASS)
	{
		BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(kEvent.getBuildingClass());
		if (eBuilding != NO_BUILDING)
		{
			if (pCity)
			{
				//iValue += kEvent.getBuildingChange() * pCity->AI_buildingValue(eBuilding);
				int iBuildingValue = GC.getBuildingInfo(eBuilding).getProductionCost();
				if (iBuildingValue > 0)
				{
					iBuildingValue *= 2;
				}
				else if (iBuildingValue == -1)
				{
					iBuildingValue = 300; //Shrine?
				}

				iBuildingValue *= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getConstructPercent();
				iValue += kEvent.getBuildingChange() * iBuildingValue;
			}
		}
	}

	TechTypes eBestTech = NO_TECH;
	int iBestValue = 0;
	for (int iTech = 0; iTech < GC.getNumTechInfos(); ++iTech)
	{
		if (canResearch((TechTypes)iTech))
		{
			if (NO_PLAYER == kTriggeredData.m_eOtherPlayer || GET_TEAM(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam()).isHasTech((TechTypes)iTech))
			{
				int iValue = 0;
				for (int i = 0; i < GC.getNumFlavorTypes(); ++i)
				{
					iValue += kEvent.getTechFlavorValue(i) * GC.getTechInfo((TechTypes)iTech).getFlavorValue(i);
				}

				if (iValue > iBestValue)
				{
					eBestTech = (TechTypes)iTech;
					iBestValue = iValue;
				}
			}
		}
	}

	if (eBestTech != NO_TECH)
	{
		iValue += (kTeam.getResearchCost(eBestTech) * kEvent.getTechPercent()) / 100;
	}

	if (kEvent.isGoldenAge())
	{
		iValue += AI_calculateGoldenAgeValue();
	}

	{	//Yield and other changes
		if (kEvent.getNumBuildingYieldChanges() > 0)
		{
			for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
			{
				for (int iYield = 0; iYield < NUM_YIELD_TYPES; ++iYield)
				{
					aiYields[iYield] += kEvent.getBuildingYieldChange(iBuildingClass, iYield);
				}
			}
		}

		if (kEvent.getNumBuildingCommerceChanges() > 0)
		{
			for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
			{
				for (int iCommerce = 0; iCommerce < NUM_COMMERCE_TYPES; ++iCommerce)
				{
					aiCommerceYields[iCommerce] += kEvent.getBuildingCommerceChange(iBuildingClass, iCommerce);
				}
			}
		}

		if (kEvent.getNumBuildingHappyChanges() > 0)
		{
			for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
			{
				iHappy += kEvent.getBuildingHappyChange(iBuildingClass);
			}
		}

		if (kEvent.getNumBuildingHealthChanges() > 0)
		{
			for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
			{
				iHealth += kEvent.getBuildingHealthChange(iBuildingClass);
			}
		}
	}
	
	if (kEvent.isCityEffect())
	{
		int iCityPopulation = -1;
		int iCityTurnValue = 0;
		if (NULL != pCity)
		{
			iCityPopulation = pCity->getPopulation();
			for (int iSpecialist = 0; iSpecialist < GC.getNumSpecialistInfos(); ++iSpecialist)
			{
				if (kEvent.getFreeSpecialistCount(iSpecialist) > 0)
				{
					//iCityTurnValue += (pCity->AI_specialistValue((SpecialistTypes)iSpecialist, false, false) / 300);
					iCityTurnValue += pCity->AI_permanentSpecialistValue((SpecialistTypes)iSpecialist) / 100; // K-Mod
				}
			}
		}

		if (-1 == iCityPopulation)
		{
			//What is going on here?
			iCityPopulation = 5;
		}

		iCityTurnValue += ((iHappy + kEvent.getHappy()) * 8);
		iCityTurnValue += ((iHealth + kEvent.getHealth()) * 6);
		
		iCityTurnValue += aiYields[YIELD_FOOD] * 5;
		iCityTurnValue += aiYields[YIELD_PRODUCTION] * 5;
		iCityTurnValue += aiYields[YIELD_COMMERCE] * 3;
		
		iCityTurnValue += aiCommerceYields[COMMERCE_RESEARCH] * 3;
		iCityTurnValue += aiCommerceYields[COMMERCE_GOLD] * 3;
		iCityTurnValue += aiCommerceYields[COMMERCE_CULTURE] * 2; // was 1
		iCityTurnValue += aiCommerceYields[COMMERCE_ESPIONAGE] * 2;

		iValue += (iCityTurnValue * 20 * iGameSpeedPercent) / 100;

		iValue += kEvent.getFood();
		iValue += kEvent.getFoodPercent() / 4;
		iValue += kEvent.getPopulationChange() * 30;
		iValue -= kEvent.getRevoltTurns() * (12 + iCityPopulation * 16);
		iValue -= (kEvent.getHurryAnger() * 6 * GC.getDefineINT("HURRY_ANGER_DIVISOR") * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getHurryConscriptAngerPercent()) / 100;
		iValue += kEvent.getHappyTurns() * 10;
		iValue += kEvent.getCulture() / 2;
	}
	else if (!kEvent.isOtherPlayerCityEffect())
	{
		int iPerTurnValue = 0;
		iPerTurnValue += iNumCities * ((iHappy * 4) + (kEvent.getHappy() * 8));
		iPerTurnValue += iNumCities * ((iHealth * 3) + (kEvent.getHealth() * 6));
		
		iValue += (iPerTurnValue * 20 * iGameSpeedPercent) / 100;
		
		iValue += (kEvent.getFood() * iNumCities);
		iValue += (kEvent.getFoodPercent() * iNumCities) / 4;
		iValue += (kEvent.getPopulationChange() * iNumCities * 40);
		iValue -= (iNumCities * kEvent.getHurryAnger() * 6 * GC.getDefineINT("HURRY_ANGER_DIVISOR") * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getHurryConscriptAngerPercent()) / 100;
		iValue += iNumCities * kEvent.getHappyTurns() * 10;
		iValue += iNumCities * kEvent.getCulture() / 2;
	}
	
	int iBonusValue = 0;
	if (NO_BONUS != kEvent.getBonus())
	{
		//iBonusValue = AI_bonusVal((BonusTypes)kEvent.getBonus());
		iBonusValue = AI_bonusVal((BonusTypes)kEvent.getBonus(), 0, true); // K-Mod
	}

	if (NULL != pPlot)
	{
		if (kEvent.getFeatureChange() != 0)
		{
			int iOldFeatureValue = 0;
			int iNewFeatureValue = 0;
			if (pPlot->getFeatureType() != NO_FEATURE)
			{
				//*grumble* who tied feature production to builds rather than the feature...
				iOldFeatureValue = GC.getFeatureInfo(pPlot->getFeatureType()).getHealthPercent();

				if (kEvent.getFeatureChange() > 0)
				{
					FeatureTypes eFeature = (FeatureTypes)kEvent.getFeature();
					FAssert(eFeature != NO_FEATURE);
					if (eFeature != NO_FEATURE)
					{
						iNewFeatureValue = GC.getFeatureInfo(eFeature).getHealthPercent();
					}
				}

				iValue += ((iNewFeatureValue - iOldFeatureValue) * iGameSpeedPercent) / 100;
			}
		}

		if (kEvent.getImprovementChange() > 0)
		{
			iValue += (30 * iGameSpeedPercent) / 100;
		}
		else if (kEvent.getImprovementChange() < 0)
		{
			iValue -= (30 * iGameSpeedPercent) / 100;
		}

		if (kEvent.getRouteChange() > 0)
		{
			iValue += (10 * iGameSpeedPercent) / 100;
		}
		else if (kEvent.getRouteChange() < 0)
		{
			iValue -= (10 * iGameSpeedPercent) / 100;
		}

		if (kEvent.getBonusChange() > 0)
		{
			iValue += (iBonusValue * 15 * iGameSpeedPercent) / 100;
		}
		else if (kEvent.getBonusChange() < 0)
		{
			iValue -= (iBonusValue * 15 * iGameSpeedPercent) / 100;
		}
		
		for (int i = 0; i < NUM_YIELD_TYPES; ++i)
		{
			if (0 != kEvent.getPlotExtraYield(i))
			{
				if (pPlot->getWorkingCity() != NULL)
				{
					FAssertMsg(pPlot->getWorkingCity()->getOwner() == getID(), "Event creates a boni for another player?");
					aiYields[i] += kEvent.getPlotExtraYield(i);
				}
				else
				{
					iValue += (20 * 8 * kEvent.getPlotExtraYield(i) * iGameSpeedPercent) / 100;
				}
			}
		}
	}

	if (NO_BONUS != kEvent.getBonusRevealed())
	{
		iValue += (iBonusValue * 10 * iGameSpeedPercent) / 100;
	}

	if (NULL != pUnit)
	{
		iValue += (2 * pUnit->baseCombatStr() * kEvent.getUnitExperience() * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getTrainPercent()) / 100;

		iValue -= 10 * kEvent.getUnitImmobileTurns();
	}

	{
		int iPromotionValue = 0;
		
		for (int i = 0; i < GC.getNumUnitCombatInfos(); ++i)
		{
			if (NO_PROMOTION != kEvent.getUnitCombatPromotion(i))
			{
				int iLoop;
				for (CvUnit* pLoopUnit = firstUnit(&iLoop); NULL != pLoopUnit; pLoopUnit = nextUnit(&iLoop))
				{
					if (pLoopUnit->getUnitCombatType() == i)
					{
						if (!pLoopUnit->isHasPromotion((PromotionTypes)kEvent.getUnitCombatPromotion(i)))
						{
							iPromotionValue += 5 * pLoopUnit->baseCombatStr();
						}
					}
				}

				iPromotionValue += iNumCities * 50;
			}
		}
		
		iValue += (iPromotionValue * iGameSpeedPercent) / 100;
	}

	if (kEvent.getFreeUnitSupport() != 0)
	{
		iValue += (20 * kEvent.getFreeUnitSupport() * iGameSpeedPercent) / 100;
	}
	
	if (kEvent.getInflationModifier() != 0)
	{
		iValue -= (20 * kEvent.getInflationModifier() * calculatePreInflatedCosts() * iGameSpeedPercent) / 100;		
	}

	if (kEvent.getSpaceProductionModifier() != 0)
	{
		iValue += ((20 + iNumCities) * getSpaceProductionModifier() * iGameSpeedPercent) / 100;
	}

	int iOtherPlayerAttitudeWeight = 0;
	if (kTriggeredData.m_eOtherPlayer != NO_PLAYER)
	{
		iOtherPlayerAttitudeWeight = AI_getAttitudeWeight(kTriggeredData.m_eOtherPlayer);
		iOtherPlayerAttitudeWeight += 10 - GC.getGame().getSorenRandNum(20, "AI event value attitude");
	}

	//Religion
	if (kTriggeredData.m_eReligion != NO_RELIGION)
	{
		ReligionTypes eReligion = kTriggeredData.m_eReligion;
		
		int iReligionValue = 15;
		
		if (getStateReligion() == eReligion)
		{
			iReligionValue += 15;
		}
		if (hasHolyCity(eReligion))
		{
			iReligionValue += 15;
		}
	
		iValue += (kEvent.getConvertOwnCities() * iReligionValue * iGameSpeedPercent) / 100;

		if (kEvent.getConvertOtherCities() > 0)
		{
			//Don't like them much = fairly indifferent, hate them = negative.
			iValue += (kEvent.getConvertOtherCities() * (iOtherPlayerAttitudeWeight + 50) * iReligionValue * iGameSpeedPercent) / 15000;
		}
	}

	if (NO_PLAYER != kTriggeredData.m_eOtherPlayer)
	{
		CvPlayerAI& kOtherPlayer = GET_PLAYER(kTriggeredData.m_eOtherPlayer);

		int iDiploValue = 0;
		//if we like this player then value positive attitude, if however we really hate them then
		//actually value negative attitude.
		iDiploValue += ((iOtherPlayerAttitudeWeight + 50) * kEvent.getAttitudeModifier() * GET_PLAYER(kTriggeredData.m_eOtherPlayer).getPower()) / std::max(1, getPower());
		
		if (kEvent.getTheirEnemyAttitudeModifier() != 0)
		{
			//Oh wow this sure is mildly complicated.
			TeamTypes eWorstEnemy = GET_TEAM(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam()).AI_getWorstEnemy();
			
			if (NO_TEAM != eWorstEnemy && eWorstEnemy != getTeam())
			{
			int iThirdPartyAttitudeWeight = kTeam.AI_getAttitudeWeight(eWorstEnemy);
			
			//If we like both teams, we want them to get along.
			//If we like otherPlayer but not enemy (or vice-verca), we don't want them to get along.
			//If we don't like either, we don't want them to get along.
			//Also just value stirring up trouble in general.
			
			int iThirdPartyDiploValue = 50 * kEvent.getTheirEnemyAttitudeModifier();
			iThirdPartyDiploValue *= (iThirdPartyAttitudeWeight - 10);
			iThirdPartyDiploValue *= (iOtherPlayerAttitudeWeight - 10);
			iThirdPartyDiploValue /= 10000;
			
			if ((iOtherPlayerAttitudeWeight) < 0 && (iThirdPartyAttitudeWeight < 0))
			{
				iThirdPartyDiploValue *= -1;
			}
			
			iThirdPartyDiploValue /= 2;
			
			iDiploValue += iThirdPartyDiploValue;
		}
		}
		
		iDiploValue *= iGameSpeedPercent;
		iDiploValue /= 100;
		
		if (NO_BONUS != kEvent.getBonusGift())
		{
			/* original bts code
			int iBonusValue = -AI_bonusVal((BonusTypes)kEvent.getBonusGift(), -1);
			iBonusValue += (iOtherPlayerAttitudeWeight - 40) * kOtherPlayer.AI_bonusVal((BonusTypes)kEvent.getBonusGift(), +1);
			//Positive for friends, negative for enemies.
			iDiploValue += (iBonusValue * GC.getDefineINT("PEACE_TREATY_LENGTH")) / 60; */

			// K-Mod. The original code undervalued our loss of bonus by a factor of 100.
			iValue -= AI_bonusVal((BonusTypes)kEvent.getBonusGift(), -1) * GC.getDefineINT("PEACE_TREATY_LENGTH") / 4;
			int iGiftValue = kOtherPlayer.AI_bonusVal((BonusTypes)kEvent.getBonusGift(), +1) * (iOtherPlayerAttitudeWeight - 40) / 100;
			iDiploValue += iGiftValue * GC.getDefineINT("PEACE_TREATY_LENGTH") / 4;
			// K-Mod end
		}
		
		if (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI))
		{
			//What is this "relationships" thing?
			iDiploValue /= 2;
		}
		
		if (kEvent.isGoldToPlayer())
		{
			//If the gold goes to another player instead of the void, then this is a positive
			//thing if we like the player, otherwise it's a negative thing.
			int iGiftValue = (getEventCost(eEvent, kTriggeredData.m_eOtherPlayer, false) + getEventCost(eEvent, kTriggeredData.m_eOtherPlayer, true)) / 2;
			iGiftValue *= -iOtherPlayerAttitudeWeight;
			iGiftValue /= 110;
			
			iValue += iGiftValue;
		}

		if (kEvent.isDeclareWar())
		{
			// K-Mod. Veto declare war events which are against our character.
			// (This is moved from the start of this function, and rewritten.)
			TeamTypes eOtherTeam = GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam();
			if (kEvent.isDeclareWar() && kTriggeredData.m_eOtherPlayer != NO_PLAYER)
			{
				if (kTeam.AI_refuseWar(eOtherTeam))
					return INT_MIN/100; // Note: the divide by 100 is just to avoid an overflow later on.
			}
			// K-Mod end

			/* original bts code
			int iWarValue = GET_TEAM(getTeam()).getDefensivePower(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam())
				- GET_TEAM(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam()).getPower(true);// / std::max(1, GET_TEAM(getTeam()).getDefensivePower());
			iWarValue -= 30 * AI_getAttitudeVal(kTriggeredData.m_eOtherPlayer); */

			// K-Mod. Note: the original code doesn't touch iValue.
			// So whatever I do here is completely new.

			// Note: the event will make the other team declare war on us.
			// Also, I've decided nto to multiply this by number of turns or anything like that.
			// The war evaluation is rough, and optimistic... and besides, we can always declare war ourselves if we want to.
			int iWarValue = kTeam.AI_startWarVal(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam(), WARPLAN_ATTACKED);

			iValue += iWarValue;
			// K-Mod end
		}
			
		if (kEvent.getMaxPillage() > 0)
		{
			int iPillageValue = (40 * (kEvent.getMinPillage() + kEvent.getMaxPillage())) / 2;
			//If we hate them, this is good to do.
			iPillageValue *= 25 - iOtherPlayerAttitudeWeight;
			iPillageValue *= iGameSpeedPercent;
			iPillageValue /= 12500;

			iValue += iPillageValue; // K-Mod!
		}

		iValue += (iDiploValue * iGameSpeedPercent) / 100;
	}

	int iThisEventValue = iValue;
	//XXX THIS IS VULNERABLE TO NON-TRIVIAL RECURSIONS!
	//Event A effects Event B, Event B effects Event A
	for (int iEvent = 0; iEvent < GC.getNumEventInfos(); ++iEvent)
	{
		if (kEvent.getAdditionalEventChance(iEvent) > 0)
		{
			if (iEvent == eEvent)
			{
				//Infinite recursion is not our friend.
				//Fortunately we have the event value for this event - sans values of other events
				//disabled or cleared. Hopefully no events will be that complicated...
				//Double the value since it's recursive.
				iValue += (kEvent.getAdditionalEventChance(iEvent) * iThisEventValue) / 50;
			}
			else
			{
				iValue += (kEvent.getAdditionalEventChance(iEvent) * AI_eventValue((EventTypes)iEvent, kTriggeredData)) / 100;
			}
		}
	
		if (kEvent.getClearEventChance(iEvent) > 0)
		{
			if (iEvent == eEvent)
			{
				iValue -= (kEvent.getClearEventChance(iEvent) * iThisEventValue) / 50;
			}
			else
			{
				iValue -= (kEvent.getClearEventChance(iEvent) * AI_eventValue((EventTypes)iEvent, kTriggeredData)) / 100;
			}
		}
	}

	iValue *= 100 + GC.getGameINLINE().getSorenRandNum(20, "AI Event choice");
	iValue /= 100;

	return iValue;
}

EventTypes CvPlayerAI::AI_chooseEvent(int iTriggeredId) const
{
	EventTriggeredData* pTriggeredData = getEventTriggered(iTriggeredId);
	if (NULL == pTriggeredData)
	{
		return NO_EVENT;
	}

	CvEventTriggerInfo& kTrigger = GC.getEventTriggerInfo(pTriggeredData->m_eTrigger);

	//int iBestValue = -MAX_INT;
	int iBestValue = INT_MIN; // K-Mod
	EventTypes eBestEvent = NO_EVENT;

	for (int i = 0; i < kTrigger.getNumEvents(); i++)
	{
		//int iValue = -MAX_INT;
		int iValue = INT_MIN; // K-Mod
		if (kTrigger.getEvent(i) != NO_EVENT)
		{
			CvEventInfo& kEvent = GC.getEventInfo((EventTypes)kTrigger.getEvent(i));
			if (canDoEvent((EventTypes)kTrigger.getEvent(i), *pTriggeredData))
			{
				iValue = AI_eventValue((EventTypes)kTrigger.getEvent(i), *pTriggeredData);
			}
		}

		if (iValue > iBestValue)
		{
			iBestValue = iValue;
			eBestEvent = (EventTypes)kTrigger.getEvent(i);
		}
	}

	return eBestEvent;
}

void CvPlayerAI::AI_doSplit(bool force) // advc.104r: Param added
{
	PROFILE_FUNC();

	if (!canSplitEmpire())
	{
		return;
	}

	int iLoop;
	std::map<int, int> mapAreaValues;

	for (CvArea* pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		mapAreaValues[pLoopArea->getID()] = 0;
	}

	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		//mapAreaValues[pLoopCity->area()->getID()] += pLoopCity->AI_cityValue();
		// K-Mod
		// we don't consider splitting empire while there is a land war. (this check use to be in AI_cityValue)
		if (!AI_isLandWar(pLoopCity->area()))
		{
			mapAreaValues[pLoopCity->getArea()] += pLoopCity->AI_cityValue();
		}
		// K-Mod end
	}

	std::map<int, int>::iterator it;
	for (it = mapAreaValues.begin(); it != mapAreaValues.end(); ++it)
	{
		if (it->second < 0
			|| force) // advc.104r
		{
			int iAreaId = it->first;

			if (canSplitArea(iAreaId))
			{
				splitEmpire(iAreaId);

				for (CvUnit* pUnit = firstUnit(&iLoop); pUnit != NULL; pUnit = nextUnit(&iLoop))
				{
					if (pUnit->area()->getID() == iAreaId)
					{
						TeamTypes ePlotTeam = pUnit->plot()->getTeam();

						if (NO_TEAM != ePlotTeam)
						{
							CvTeam& kPlotTeam = GET_TEAM(ePlotTeam);
							if (kPlotTeam.isVassal(getTeam()) && GET_TEAM(getTeam()).isParent(ePlotTeam))
							{
								pUnit->gift();
							}
						}
					}
				}
				break;
			}
		}
	}
}

// <advc.099b>
double CvPlayerAI::exclusiveRadiusWeight(int dist) const {

	if(dist > 2)
		return 0;
	double mult = 1.5;
	if(dist == 0 || dist == 1)
		mult = 2;
	else if(dist == 2)
		mult = 1;
	/*  Between 0 and 1. Expresses our confidence about winning culturally
		contested tiles that are within the working radius of our cities
		exclusively. Since the decay of tile culture isn't based on game speed,
		that confidence is greater on the slower speed settings. */
	return 1 - std::pow(1 - (mult * GC.getEXCLUSIVE_RADIUS_DECAY() /
			1000.0), 25 * GC.getGameSpeedInfo(GC.getGameINLINE().
			getGameSpeedType()).getGoldenAgePercent() / 100.0);
} // </advc.099b>


void CvPlayerAI::AI_launch(VictoryTypes eVictory)
{
	if (GET_TEAM(getTeam()).isHuman())
	{
		return;
	}

	if (!GET_TEAM(getTeam()).canLaunch(eVictory))
	{
		return;
	}

	bool bLaunch = true;

	int iBestArrival = MAX_INT;
	TeamTypes eBestTeam = NO_TEAM;

	for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
	{
		if (iTeam != getTeam())
		{
			CvTeam& kTeam = GET_TEAM((TeamTypes)iTeam);
			if (kTeam.isAlive())
			{
				int iCountdown = kTeam.getVictoryCountdown(eVictory);
				if (iCountdown > 0)
				{
					if (iCountdown < iBestArrival)
					{
						iBestArrival = iCountdown;
						eBestTeam = (TeamTypes)iTeam;
					}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/07/09                       r_rolo1 & jdog5000     */
/*                                                                                              */
/*                                                                                              */
/************************************************************************************************/
/* original BTS code
					if (iCountdown < GET_TEAM(getTeam()).getVictoryDelay(eVictory) && kTeam.getLaunchSuccessRate(eVictory) == 100)
					{
						bLaunch = false;
						break;
					}
*/
					// Other civs capital could still be captured, might as well send spaceship
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				}
			}
		}
	}

	if (bLaunch)
	{
		if (NO_TEAM == eBestTeam || iBestArrival > GET_TEAM(getTeam()).getVictoryDelay(eVictory))
		{
			if (GET_TEAM(getTeam()).getLaunchSuccessRate(eVictory) < 100)
			{
				bLaunch = false;
			}
		}
	}

	if (bLaunch)
	{
		launch(eVictory);
	}
}

void CvPlayerAI::AI_doCheckFinancialTrouble()
{
	// if we just ran into financial trouble this turn
	bool bFinancialTrouble = AI_isFinancialTrouble();
	if (bFinancialTrouble != m_bWasFinancialTrouble)
	{
		if (bFinancialTrouble)
		{
			int iGameTurn = GC.getGameINLINE().getGameTurn();
			
			// only reset at most every 10 turns
			if (iGameTurn > m_iTurnLastProductionDirty + 10)
			{
				// redeterimine the best things to build in each city
				AI_makeProductionDirty();
			
				m_iTurnLastProductionDirty = iGameTurn;
			}
		}
		
		m_bWasFinancialTrouble = bFinancialTrouble;
	}
}

// bool CvPlayerAI::AI_disbandUnit(int iExpThreshold, bool bObsolete)
// K-Mod, deleted this function.


// heavily edited by K-Mod and bbai
int CvPlayerAI::AI_calculateCultureVictoryStage(
		int countdownThresh) const // advc.115
{
	PROFILE_FUNC();

	if( GC.getDefineINT("BBAI_VICTORY_STRATEGY_CULTURE") <= 0 )
	{
		return 0;
	}

    if (!GC.getGameINLINE().culturalVictoryValid())
    {
        return 0;
    }

	// Necessary as capital city pointer is used later
    if (getCapitalCity() == NULL)
    {
        return 0;
    }

	int iHighCultureCount = 0;
	int iCloseToLegendaryCount = 0;
	int iLegendaryCount = 0;
	// K-Mod
	int iEraThresholdPercent = 80 - (AI_getStrategyRand(1) % 2)*20; // some (arbitrary) fraction of the game, after which we get more serious. (cf. old code)
	int iLegendaryCulture = GC.getGame().getCultureThreshold((CultureLevelTypes)(GC.getNumCultureLevelInfos() - 1));
	int iVictoryCities = GC.getGameINLINE().culturalVictoryNumCultureCities();

	int iHighCultureMark = 300; // turns
	iHighCultureMark *= (3*GC.getNumEraInfos() - 2*getCurrentEra());
	iHighCultureMark /= std::max(1, 3*GC.getNumEraInfos());
	iHighCultureMark *= GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getVictoryDelayPercent();
	iHighCultureMark /= 100;
	
	int iGoldCommercePercent = AI_estimateBreakEvenGoldPercent();

	std::vector<int> countdownList;
	// K-Mod end

	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (pLoopCity->getCultureLevel() >= (GC.getGameINLINE().culturalVictoryCultureLevel() - 1))
		{
			// K-Mod. Try to tell us what the culture would be like if we were to turn up the slider. (cf. AI_updateCommerceWeights)
			// (current culture rate, minus what we're current getting from raw commerce, plus what we would be getting from raw commerce at maximum percentage.)
			int iEstimatedRate = pLoopCity->getCommerceRate(COMMERCE_CULTURE);
			iEstimatedRate += (100 - iGoldCommercePercent - getCommercePercent(COMMERCE_CULTURE)) * pLoopCity->getYieldRate(YIELD_COMMERCE) * pLoopCity->getTotalCommerceRateModifier(COMMERCE_CULTURE) / 10000;
			int iCountdown = (iLegendaryCulture - pLoopCity->getCulture(getID())) / std::max(1, iEstimatedRate);
			countdownList.push_back(iCountdown);
			if (iCountdown < iHighCultureMark)
			{
				iHighCultureCount++;
			}

			// <advc.115> Threshold was 50%, now 67%, 60% for human.
			double thresh = 0.67;
			if(isHuman())
				thresh = 0.6;
			if( pLoopCity->getCulture(getID()) > ::round(thresh * pLoopCity->getCultureThreshold(GC.getGameINLINE().culturalVictoryCultureLevel())) )
			{// </ advc.115>
				iCloseToLegendaryCount++;
			}

			// is already there?
			if( pLoopCity->getCulture(getID()) > pLoopCity->getCultureThreshold(GC.getGameINLINE().culturalVictoryCultureLevel()) )
			{
				iLegendaryCount++;
			}
		}
	}

	if( iLegendaryCount >= iVictoryCities )
	{
		// Already won, keep playing culture heavy but do some tech to keep pace if human wants to keep playing
		return 3;
	}

	// K-Mod. Originally, the BBAI condition for culture4 was just "iCloseToLegendaryCount >= iVictoryCities".
	// I think that is far too simplistic for the AI players; so I've removed that clause.
	if (isHuman())
			// advc.115d: Commented out
			//&& !GC.getGameINLINE().isDebugMode())
	{
		if (iCloseToLegendaryCount >= iVictoryCities)
		{
			return 4;
		}
		else if (getCommercePercent(COMMERCE_CULTURE) > 50)
		{
			return 3;
		}
		return 0;
	}
	// K-Mod end

	// K-Mod, disabling some stuff.
	// It is still possible to get a cultural victory in advanced start games.
	// and moving your capital city certainly does not indicate that you shouldn't go for a cultural victory!
	// ... and colonies don't get their capital on turn 1 anyway.
	/* original code
	if (GC.getGame().getStartEra() > 1)
    {
    	return 0;
    }

	if (getCapitalCity()->getGameTurnFounded() > (10 + GC.getGameINLINE().getStartTurn()))
    {
		if( iHighCultureCount < GC.getGameINLINE().culturalVictoryNumCultureCities() )
		{
			//the loss of the capital indicates it might be a good idea to abort any culture victory
			return 0;
		}
    } */

	// If we aren't already obviously close to a cultural victory, then we need to consider whether or not to aim for culture at all.
	if (iCloseToLegendaryCount < iVictoryCities)
	{
		int iValue = GC.getLeaderHeadInfo(getPersonalityType()).getCultureVictoryWeight();

		if (GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE))
		{
			iValue += 30;
		}

		iValue += (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI)
				? -10 : 0); // advc.019: was ?-20:0

		// K-Mod
		iValue += 30 * std::min(iCloseToLegendaryCount, iVictoryCities + 1);
		iValue += 10 * std::min(iHighCultureCount, iVictoryCities + 1);
		iValue += getCurrentEra() < (iEraThresholdPercent-20)*GC.getNumEraInfos()/100; // prep for culture in the early game, just in case.
		// K-Mod end
		if( iValue > 20 && getNumCities() >= iVictoryCities )
		{
			iValue += 5*countHolyCities(); // was 10*
			// K-Mod: be wary of going for a cultural victory if everyone hates us.
			int iScore = 0;
			int iTotalPop = 0;
			for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
			{
				const CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
				if (iI != getID() && kLoopPlayer.isAlive() && GET_TEAM(getTeam()).isHasMet(kLoopPlayer.getTeam())
					&& !kLoopPlayer.isMinorCiv() && !GET_TEAM(kLoopPlayer.getTeam()).isAVassal())
				{
					iTotalPop += kLoopPlayer.getTotalPopulation();
					if (AI_getAttitude((PlayerTypes)iI) <= ATTITUDE_ANNOYED)
						iScore -= 20 * kLoopPlayer.getTotalPopulation();
					else if (AI_getAttitude((PlayerTypes)iI) >= ATTITUDE_PLEASED)
						iScore += 10 * kLoopPlayer.getTotalPopulation();
				}
			}
			iValue += iScore / std::max(1, iTotalPop);
			iValue -= AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST1) ? 20 : 0;
			iValue -= AI_isDoVictoryStrategy(AI_VICTORY_SPACE2) ? 10 : 0;
			iValue -= AI_isDoVictoryStrategy(AI_VICTORY_SPACE3) ? 20 : 0;
			// K-Mod end
		}
		/*
		if ((GET_TEAM(getTeam()).isAVassal()) && (getNumCities() > 5))
		{
			int iReligionCount = countTotalHasReligion();
			if (((iReligionCount * 100) / getNumCities()) > 250)
			{
				iValue += 1;
				iValue += ((2 * iReligionCount) + 1) / getNumCities();
			}
		}
		*/
		// <advc.109>
		if(isLonely())
			iValue -= 30; // </advc.109>
		//int iNonsense = AI_getStrategyRand() + 10;
		iValue += (AI_getStrategyRand(0) % 70); // advc.115: was 100

		if (iValue < 100)
		{
			return 0;
		}
	}

	int iWinningCountdown = INT_MAX;
	if ((int)countdownList.size() >= iVictoryCities && iVictoryCities > 0)
	{
		std::partial_sort(countdownList.begin(), countdownList.begin() + iVictoryCities, countdownList.end());
		iWinningCountdown = countdownList[iVictoryCities-1];
	}
    //if (iCloseToLegendaryCount >= iVictoryCities || getCurrentEra() >= (GC.getNumEraInfos() - (2 + AI_getStrategyRand(1) % 2)))
	if (iCloseToLegendaryCount >= iVictoryCities || getCurrentEra() >= iEraThresholdPercent*GC.getNumEraInfos()/100) // K-Mod (note: this matches the line above)
    {
		bool bAt3 = false;
        
		// if we have enough high culture cities, go to stage 3
		if (iHighCultureCount >= iVictoryCities)
		{
			bAt3 = true;
		}

		// if we have a lot of religion, may be able to catch up quickly
		/* disabled by K-Mod
		if (countTotalHasReligion() >= getNumCities() * 3)
        {
			if( getNumCities() >= GC.getGameINLINE().culturalVictoryNumCultureCities() )
			{
				bAt3 = true;
			}
        } */

		if( bAt3 )
		{
			/* original code
			if (AI_cultureVictoryTechValue(getCurrentResearch()) < 100)
			{
				return 4;
			}*/
			// K-Mod. Do full culture if our winning countdown is below the countdown target.
			/*  <advc.115> Was 180. Now set to the countdownThresh param if
				it is provided, otherwise 100. */
			int iCountdownTarget = 100;
			if(countdownThresh >= 0)
				iCountdownTarget = countdownThresh; // </advc.115>
			{
				// The countdown target depends on what other victory strategies we have in mind. The target is 180 turns * 100 / iDenominator.
				// For example, if we're already at war, and going for conquest 4, the target countdown is then ~ 180 * 100 / 470 == 38 turns.
				// Note: originally culture 4 was blocked completely by stage 4 of any other victory strategy. But this is no longer the case.
				int iDemoninator = 100;
				iDemoninator += 20 * AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST1 | AI_VICTORY_DOMINATION1);
				iDemoninator += 50 * AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST2);
				iDemoninator += 50 * AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3);
				iDemoninator += 80 * AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION3);
				iDemoninator += 70 * AI_isDoVictoryStrategy(AI_VICTORY_SPACE2);
				iDemoninator += 80 * AI_isDoVictoryStrategy(AI_VICTORY_SPACE3);
				iDemoninator += 50 * AI_isDoVictoryStrategy(AI_VICTORY_SPACE4);
				iDemoninator += AI_cultureVictoryTechValue(getCurrentResearch()) > 100 ? 80 : 0;
				iDemoninator += AI_isDoStrategy(AI_STRATEGY_GET_BETTER_UNITS)? 80 : 0;
				if(isFocusWar()) // advc.105
				//if (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0)
				{
					iDemoninator += 50;
					iDemoninator += 200 * AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST4 | AI_VICTORY_DOMINATION4);
					int iWarSuccessRating = GET_TEAM(getTeam()).AI_getWarSuccessRating();
					iDemoninator += iWarSuccessRating < 0 ? 10 - 2*iWarSuccessRating : 0;
				}
				// and a little bit of personal variation.
				iDemoninator += 50 - GC.getLeaderHeadInfo(getPersonalityType()).getCultureVictoryWeight();

				iCountdownTarget *= GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getVictoryDelayPercent();
				iCountdownTarget /= std::max(1, iDemoninator);
			}
			if (iWinningCountdown < iCountdownTarget)
			{
				return 4;
			}
			// K-Mod end

			return 3;
		}
    }

	//if (getCurrentEra() >= ((GC.getNumEraInfos() / 3) + iNonsense % 2))
	if (getCurrentEra() >= ((GC.getNumEraInfos() / 3) + AI_getStrategyRand(1) % 2) || iHighCultureCount >= iVictoryCities-1)
	{
		if (iHighCultureCount < getCurrentEra() + iVictoryCities - GC.getNumEraInfos())
			return 1;
	    return 2;
	}
        
	return 1;
}

int CvPlayerAI::AI_calculateSpaceVictoryStage() const
{
    int iValue;

	if( GC.getDefineINT("BBAI_VICTORY_STRATEGY_SPACE") <= 0 )
	{
		return 0;
	}

	if (getCapitalCity() == NULL)
    {
        return 0;
    }
	
	// Better way to determine if spaceship victory is valid?
	VictoryTypes eSpace = NO_VICTORY;
	for (int iI = 0; iI < GC.getNumVictoryInfos(); iI++)
	{
		if (GC.getGameINLINE().isVictoryValid((VictoryTypes) iI))
		{
			CvVictoryInfo& kVictoryInfo = GC.getVictoryInfo((VictoryTypes) iI);
			if( kVictoryInfo.getVictoryDelayTurns() > 0 )
			{
				eSpace = (VictoryTypes)iI;
				break;
			}
		}
	}

	if( eSpace == NO_VICTORY )
	{
		return 0;
	}

	// If have built Apollo, then the race is on!
	bool bHasApollo = false;
	bool bNearAllTechs = true;
	for( int iI = 0; iI < GC.getNumProjectInfos(); iI++ )
	{
		if( GC.getProjectInfo((ProjectTypes)iI).getVictoryPrereq() == eSpace )
		{
			if( GET_TEAM(getTeam()).getProjectCount((ProjectTypes)iI) > 0 )
			{
				bHasApollo = true;
			}
			else
			{
				if( !GET_TEAM(getTeam()).isHasTech((TechTypes)GC.getProjectInfo((ProjectTypes)iI).getTechPrereq()) )
				{
					if( !isResearchingTech((TechTypes)GC.getProjectInfo((ProjectTypes)iI).getTechPrereq()) )
					{
						bNearAllTechs = false;
					}
				}
			}
		}
	}
	/*  <advc.115> All the parts together cost some 15000 to 20000 production on
		Normal speed. Can't be sure that production will increase much in the future;
		most tech that could boost production is already known by the time
		Apollo is built. If it'll take more than 100 turns, then it's hopeless
		(and not all production is going to go into spaceship parts either). */
	int totalProduction = calculateTotalYield(YIELD_PRODUCTION);
	bool enoughProduction = (totalProduction > (240 * GC.getGameSpeedInfo(
			GC.getGameINLINE().getGameSpeedType()).getCreatePercent()) / 100);
	// </advc.115>
	if( bHasApollo )
	{
		if( bNearAllTechs )
		{
			bool bOtherLaunched = false;
			// K-Mod. (just tidying up a bit.)
			int iOurCountdown = GET_TEAM(getTeam()).getVictoryCountdown(eSpace);
			for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
			{
				if (iTeam == getTeam())
					continue;

				if (GET_TEAM((TeamTypes)iTeam).getVictoryCountdown(eSpace) >= 0 &&
					(iOurCountdown < 0 || GET_TEAM((TeamTypes)iTeam).getVictoryCountdown(eSpace) <= iOurCountdown))
				{
					bOtherLaunched = true;
					break;
				}
			}
			// K-Mod end

			if( !bOtherLaunched )
			{
				return 4;
			}
			if(enoughProduction || iOurCountdown > 0) // advc.115
				return 3;
		}

		/* original bbai code
		if( GET_TEAM(getTeam()).getBestKnownTechScorePercent() > (m_iVictoryStrategyHash & AI_VICTORY_SPACE3 ? 80 : 85) )
		{
			return 3;
		} */
		// K-Mod. I don't think that's a good way to do it.
		// instead, compare our spaceship progress to that of our rivals.
		int iOurProgress = 0;
		for (int i = 0; i < GC.getNumProjectInfos(); i++)
		{
			if (GC.getProjectInfo((ProjectTypes)i).isSpaceship())
			{
				int iBuilt = GET_TEAM(getTeam()).getProjectCount((ProjectTypes)i);

				if (iBuilt > 0 || GET_TEAM(getTeam()).isHasTech((TechTypes)(GC.getProjectInfo((ProjectTypes)i).getTechPrereq())))
					iOurProgress += 2 + iBuilt;
			}
		}
		int iKnownTeams = 0;
		int iSpaceTeams = 0; // teams ahead of us in the space race
		for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
		{
			const CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
			if (iTeam != getTeam() && kLoopTeam.isAlive() && kLoopTeam.isHasMet(getTeam()))
			{
				int iProgress = 0;
				for (int i = 0; i < GC.getNumProjectInfos(); i++)
				{
					if (GC.getProjectInfo((ProjectTypes)i).isSpaceship())
					{
						int iBuilt = kLoopTeam.getProjectCount((ProjectTypes)i);

						if (iBuilt > 0 || kLoopTeam.isHasTech((TechTypes)(GC.getProjectInfo((ProjectTypes)i).getTechPrereq())))
							iProgress += 2 + iBuilt;
					}
				}
				iKnownTeams++;
				if (iProgress > iOurProgress)
					iSpaceTeams++;
			}
		}
		if (200 * iSpaceTeams / (1+iKnownTeams)
				<= GC.getLeaderHeadInfo(getPersonalityType()).getSpaceVictoryWeight() +
				AI_getStrategyRand(3) % 100 && // note, correlated with number used lower down.
				(enoughProduction || iOurProgress > 10)) // advc.115
			return 3;
		// K-Mod end
	}

	if( isHuman())
			// advc.115d: Commented out
			//&& !(GC.getGameINLINE().isDebugMode()) )
	{
		return 0;
	}
	/*  <advc.115>*/
	if(!enoughProduction)
		return 0; // </advc.115>
	// If can't build Apollo yet, then consider making player push for this victory type
	{
		iValue = GC.getLeaderHeadInfo(getPersonalityType()).getSpaceVictoryWeight();

		if (GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE))
		{
    		iValue += 30;
		}

		if(GET_TEAM(getTeam()).isAVassal())
		{
			iValue += 20;
		}

		iValue += (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI)
				? -10 : 0); // advc.019: was ?-20:0

		// K-Mod: take into account how we're doing in tech
		int iScore = 0;
		int iTotalPop = 0;
		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			const CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
			if (iI != getID() && kLoopPlayer.isAlive() && GET_TEAM(getTeam()).isHasMet(kLoopPlayer.getTeam()))
			{
				iTotalPop += kLoopPlayer.getTotalPopulation();
				iScore += (getTechScore() > kLoopPlayer.getTechScore() ? 20 : -20) * kLoopPlayer.getTotalPopulation();
			}
		}
		iValue += iScore / std::max(1, iTotalPop);
		// K-Mod end

		//int iNonsense = AI_getStrategyRand() + 50;
		iValue += (AI_getStrategyRand(3) % 100);

		if (iValue >= 100)
		{
			if( getCurrentEra() >= GC.getNumEraInfos() - 3 )
			{
				return 2;
			}

			return 1;
		}
	}

	return 0;
}


// This function has been completely rewritten for K-Mod
int CvPlayerAI::AI_calculateConquestVictoryStage() const
{
	PROFILE_FUNC();
	const CvTeamAI& kTeam = GET_TEAM(getTeam());

	// check for validity of conquest victory
	if (GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE) || kTeam.isAVassal() || GC.getDefineINT("BBAI_VICTORY_STRATEGY_CONQUEST") <= 0)
		return 0;

	{
		bool bValid = false;

		for (VictoryTypes i = (VictoryTypes)0; !bValid && i < GC.getNumVictoryInfos(); i=(VictoryTypes)(i+1))
		{
			if (GC.getGameINLINE().isVictoryValid(i) && GC.getVictoryInfo(i).isConquest())
				bValid = true;
		}
		if (!bValid)
			return 0;
	}

	/* Things to consider:
	 - personality (conquest victory weight)
	 - our relationship with other civs
	 - our current power and productivity
	 - our war record. (ideally, we'd look at wars won / lost, but that info is unavailable - so wars declared will have to do.)
	*/

	// first, gather some data.
	int iDoWs = 0, iKnownCivs = 0, iRivalPop = 0, iStartCivs = 0, iConqueredCivs = 0, iAttitudeWeight = 0;

	for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
	{
		const CvPlayerAI& kLoopPlayer = GET_PLAYER(i);
		const CvTeamAI& kLoopTeam = GET_TEAM(kLoopPlayer.getTeam());

		if (!kLoopPlayer.isEverAlive() || kLoopPlayer.getTeam() == getTeam() || kLoopTeam.isMinorCiv())
			continue;

		iDoWs += kLoopPlayer.AI_getMemoryCount(getID(), MEMORY_DECLARED_WAR);
		/*  advc.130j: DoW memory counted times 3, but there's also decay now
			and CvTeamAI::forgiveEnemies, so let's go with 2.5 */
		iDoWs = (int)ceil(iDoWs / 2.5);

		if (kLoopPlayer.getParent() != NO_PLAYER)
			continue;

		iStartCivs++;

		if (!kLoopPlayer.isAlive() || kLoopTeam.isVassal(getTeam()))
		{
			iKnownCivs++; // eliminated civs are announced.
			iConqueredCivs++;
			continue;
		}

		if (!kTeam.isHasMet(kLoopPlayer.getTeam()))
			continue;

		iKnownCivs++;
		iRivalPop += kLoopPlayer.getTotalPopulation();
		iAttitudeWeight += kLoopPlayer.getTotalPopulation() * AI_getAttitudeWeight(i);
	}
	iAttitudeWeight /= std::max(1, iRivalPop);

	// (please excuse the fact that the AI magically knows the power of all teams.)
	int iRivalTeams = 0; // number of other teams we know
	int iRivalPower = 0; // sum of their power
	int iStrongerTeams = 0; // number of teams known to have greater power than us.
	int iOurPower = kTeam.getPower(true);
	// <advc.104c>
	int nFriends = 0;
	int nRemaining = 0; // </advc.104c>
	int offshoreRivals = 0; // advc.115
	for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
	{
		const CvTeamAI& kLoopTeam = GET_TEAM(i);

		if (i == getTeam() || !kLoopTeam.isAlive() || kLoopTeam.isMinorCiv())
			continue;
		if(!kLoopTeam.isAVassal()) nRemaining++; // advc.104c
		if (kTeam.isHasMet(i) && !kLoopTeam.isAVassal())
		{
			iRivalTeams++;
			int p = kLoopTeam.getPower(true);
			iRivalPower += p;
			if (p >= iOurPower)
				iStrongerTeams++;
			// <advc.104c>
			if(kTeam.AI_getAttitude(i) >= ATTITUDE_FRIENDLY)
				nFriends++; // </advc.104c>
			if(!GET_TEAM(getTeam()).AI_isLandTarget(i))
				offshoreRivals++;
		}
	}
	int iAverageRivalPower = iRivalPower / std::max(1, iRivalTeams);
	bool manyOffshoreRivals = 100 * offshoreRivals / std::max(1, iKnownCivs) >= 40;
	// Ok. Now, decide whether we want to consider a conquest victory.
	// This decision is based on situation and on personality.


	// personality & randomness
	int iValue = GC.getLeaderHeadInfo(getPersonalityType()).getConquestVictoryWeight();
	//iValue += (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 20 : 0);
	iValue += (AI_getStrategyRand(4) % 100);

	// stats
	bool bVeryStrong = iRivalTeams > 0 && iOurPower > 2*iAverageRivalPower && iOurPower - iAverageRivalPower > 100;
	bool bTopRank = iStrongerTeams < (iRivalTeams + 4) / 5;
	bool bWarmonger = iDoWs > iStartCivs/2 || (iKnownCivs >= iStartCivs && iDoWs > iRivalTeams*3/4);
	bool bHateful = iAttitudeWeight < -25;

	iValue += iDoWs > 1 || AI_isDoStrategy(AI_STRATEGY_CRUSH) ? 10 : 0;
	iValue += bVeryStrong ? 30 : 0;
	iValue += bTopRank && bHateful ? 30 : 0;
	iValue += bWarmonger ? 30 : 0;
	iValue += iRivalTeams > 1 && (iStrongerTeams > iRivalTeams/2 || iOurPower < iAverageRivalPower * 3/4) ? -30 : 0;
	iValue += iOurPower < iAverageRivalPower * 4/3 && iAttitudeWeight >= 50 ? -30 : 0;

	if ((iKnownCivs <= iConqueredCivs || iValue < 110) &&
			// advc.115c:
			(GC.getGameINLINE().countCivTeamsAlive() > 2 || iValue < 50))
		return 0;

	// So, we have some interest in a conquest victory. Now we just have to decide how strong the interest is.

	/*  advc.115: The "level 1" comment below was apparently intended to mean
		that we're now already at level 1; the condition below leads to level 2.
		I'm changing the code so that the first condition is needed for level 1,
		the second for 2 etc. Could get the same effect by subtracting one level
		at the end, but I'm also changing some of the conditions. */
	// level 1
	if ((iDoWs > 0 || iRivalTeams == 1 || iConqueredCivs > iStartCivs/3) &&
			(bTopRank || bVeryStrong || bWarmonger ||
			(iConqueredCivs >= iKnownCivs/3 && bHateful)))
	{
		// level 2
		if ((bVeryStrong || bWarmonger || (bTopRank && bHateful)) && iKnownCivs >= iStartCivs*3/4 && iConqueredCivs >= iKnownCivs/3)
		{
			// level 3
			if (iKnownCivs >= iStartCivs && bTopRank &&
					!manyOffshoreRivals && // advc.115
					(bVeryStrong || (bWarmonger && bHateful && iConqueredCivs >= iKnownCivs/2))
					/*  advc.104c: The ==1 might be exploitable; by keeping some
						insignificant civ in the game. Probably no problem. */
					&& (nFriends == 0 || (nFriends == nRemaining && nFriends == 1)))
			{
				// finally, before confirming level 4, check that there is at least one team that we can declare war on.
				if(iConqueredCivs >= iKnownCivs/2) { // advc.115
					for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
					{
						const CvTeamAI& kLoopTeam = GET_TEAM(i);
						if (kLoopTeam.isAlive() && kTeam.isHasMet(i))
						{
							if (kTeam.AI_getWarPlan(i) != NO_WARPLAN ||
								(kTeam.canEventuallyDeclareWar(i) /*&& kTeam.AI_startWarVal(i, WARPLAN_TOTAL) > 0*/))
							{
								return 4;
							}
							// I'm still not sure about the WarVal thing, because that value actually depends on conquest victory flags.
						}
					}
				// <advc.115>
				}
				return 3; // </advc.115>
			}
			return 2; // advc.115: was 3
		}
		return 1; // advc.115: was 2
	}
	return 0; // advc.115: was 1
}

int CvPlayerAI::AI_calculateDominationVictoryStage() const
{
	int iValue = 0;

	if (GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE))
	{
    	return 0;
	}

	if(GET_TEAM(getTeam()).isAVassal())
	{
		return 0;
	}

	if( GC.getDefineINT("BBAI_VICTORY_STRATEGY_DOMINATION") <= 0 )
	{
		return 0;
	}

	VictoryTypes eDomination = NO_VICTORY;
	for (int iI = 0; iI < GC.getNumVictoryInfos(); iI++)
	{
		if (GC.getGameINLINE().isVictoryValid((VictoryTypes) iI))
		{
			CvVictoryInfo& kVictoryInfo = GC.getVictoryInfo((VictoryTypes) iI);
			if( kVictoryInfo.getLandPercent() > 0 && kVictoryInfo.getPopulationPercentLead() )
			{
				eDomination = (VictoryTypes)iI;
				break;
			}
		}
	}

	if( eDomination == NO_VICTORY )
	{
		return 0;
	}

	int iPercentOfDomination = 0;
	int iOurPopPercent = (100 * GET_TEAM(getTeam()).getTotalPopulation()) / std::max(1, GC.getGameINLINE().getTotalPopulation());
	int iOurLandPercent = (100 * GET_TEAM(getTeam()).getTotalLand()) / std::max(1, GC.getMapINLINE().getLandPlots());

	// <advc.104c>
	CvTeamAI& kTeam = GET_TEAM(getTeam());
	int popObjective = std::max(1, GC.getGameINLINE().getAdjustedPopulationPercent(
			eDomination));
	int landObjective =std::max(1, GC.getGameINLINE().getAdjustedLandPercent(
			eDomination));
	bool blockedByFriend = false;
	for(TeamTypes t = (TeamTypes)0; t < MAX_CIV_TEAMS; t=(TeamTypes)(t+1)) {
		CvTeamAI& kLoopTeam = GET_TEAM(t);
		if(t == getTeam() || !kLoopTeam.isAlive() || kLoopTeam.isMinorCiv()
				|| kLoopTeam.isBarbarian() || !kTeam.isHasMet(t)
				|| kLoopTeam.isAVassal())
			continue;
		int theirPopPercent = (100 * kLoopTeam.getTotalPopulation()) /
				std::max(1, GC.getGameINLINE().getTotalPopulation());
		int theirLandPercent = (100 * kLoopTeam.getTotalLand()) /
				std::max(1, GC.getMapINLINE().getLandPlots());
		if(kTeam.AI_getAttitude(t) >= ATTITUDE_FRIENDLY &&
				(theirPopPercent >= popObjective - iOurPopPercent ||
				theirLandPercent >= landObjective - iOurLandPercent))
			blockedByFriend = true;
	} // </avdri.104c>
	
	iPercentOfDomination = (100 * iOurPopPercent) / 
			popObjective; // advc.104c
	iPercentOfDomination = std::min( iPercentOfDomination, (100 * iOurLandPercent) /
			landObjective ); // advc.104c

	if(!blockedByFriend) { // advc.104c
		// <advc.115>
		int civs = GC.getGameINLINE().countCivPlayersEverAlive();
		if( iPercentOfDomination > 87 - civs ) // was simply 80
		{ // </advc.115>
			return 4;
		}

		if( iPercentOfDomination >
				// advc.115: was simply 50
				62 - civs )
		{
			return 3;
		}
	}

	if( isHuman())
			// advc.115d: Commented out
			//&& !(GC.getGameINLINE().isDebugMode()) )
	{
		return 0;
	}

	// Check for whether we are inclined to pursue a domination strategy
	{
		iValue = GC.getLeaderHeadInfo(getPersonalityType()).getDominationVictoryWeight();
		
		iValue += (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI)
				? 10 : 0); // advc.019: was ?20:0

		//int iNonsense = AI_getStrategyRand() + 70;
		iValue += (AI_getStrategyRand(5) % 100);

		if (iValue >= 100)
		{
			if( getNumCities() > 3 && (GC.getGameINLINE().getPlayerRank(getID()) < (GC.getGameINLINE().countCivPlayersAlive() + 1)/2) )
			{
				return 2;
			}

			return 1;
		}
	}

	return 0;
}

int CvPlayerAI::AI_calculateDiplomacyVictoryStage() const
{
	//int iValue = 0; // advc.115b

	if( GC.getDefineINT("BBAI_VICTORY_STRATEGY_DIPLOMACY") <= 0 )
	{
		return 0;
	}
	// <advc.115c>
	if(GC.getGameINLINE().countCivTeamsAlive() <= 2)
		return 0; // </advc.115c>
	std::vector<VictoryTypes> veDiplomacy;
	for (int iI = 0; iI < GC.getNumVictoryInfos(); iI++)
	{
		if (GC.getGameINLINE().isVictoryValid((VictoryTypes) iI))
		{
			CvVictoryInfo& kVictoryInfo = GC.getVictoryInfo((VictoryTypes) iI);
			if( kVictoryInfo.isDiploVote() )
			{
				veDiplomacy.push_back((VictoryTypes)iI);
			}
		}
	}

	if( veDiplomacy.size() == 0 )
	{
		return 0;
	}

	// Check for whether we are eligible for election
	bool bVoteEligible = false;
	for( int iVoteSource = 0; iVoteSource < GC.getNumVoteSourceInfos(); iVoteSource++ )
	{
		if( GC.getGameINLINE().isDiploVote((VoteSourceTypes)iVoteSource) )
		{
			if( GC.getGameINLINE().isTeamVoteEligible(getTeam(),(VoteSourceTypes)iVoteSource) )
			{
				bVoteEligible = true;
				break;
			}
		}
	}

	//bool bDiploInclined = false; // advc.115b: No longer used

	// Check for whether we are inclined to pursue a diplomacy strategy
	// <advc.115b>
	int iValue = 0;
	if(isHuman()) // Perhaps not inclined, but very capable.
		iValue += 100;
	else iValue += GC.getLeaderHeadInfo(getPersonalityType()).getDiplomacyVictoryWeight();
	iValue = ::round(iValue * 0.67); // Victory weight too high
	double voteTarget = -1;
	double votesToGo = GET_TEAM(getTeam()).warAndPeaceAI().
			computeVotesToGoForVictory(&voteTarget);
	FAssert(voteTarget > 0);
	double progressRatio = ::dRange((voteTarget - votesToGo) / voteTarget,
			0.0, 1.0);
	double membersProgress = 1;
	VoteSourceTypes voteSource = GET_TEAM(getTeam()).getLatestVictoryVoteSource();
	if(voteSource != NO_VOTESOURCE) {
		bool isAP = (GC.getVoteSourceInfo(voteSource).getVoteInterval() >= 7);
		// If AP religion hasn't spread far, our high voter portion doesn't help
		int nonMembers = GET_TEAM(getTeam()).warAndPeaceAI().
				countNonMembers(voteSource);
		int targetMembers = GC.getGameINLINE().countCivPlayersAlive();
		membersProgress = (targetMembers - nonMembers) / ((double)targetMembers);
		progressRatio = std::min(membersProgress, progressRatio);
		/*  WarAndPeaceAI uses the UN vote target if neither vote source is
			active yet. Also should look at UN votes in Industrial era if AP victory
			looks infeasible. */
		if(progressRatio < 0.66 && getCurrentEra() >= 4 && isAP) {
			votesToGo = GET_TEAM(getTeam()).warAndPeaceAI().
					computeVotesToGoForVictory(&voteTarget, true); // force UN
			progressRatio = ::dRange((voteTarget - votesToGo) / voteTarget, 0.0, 1.0);
			bVoteEligible = false;
			membersProgress = 1; // not a concern for UN
		}
	}
	/*  LeaderHead weight is between 0 and 70. We're adding a hash between
		0 and 50 below. These are for flavor and randomness. This is the
		intelligent part; put that between 0 and 180.
		Square because having e.g. 50% of the necessary votes isn't nearly
		enough; need to get close to the vote target. */
	iValue += ::round(200 * progressRatio * progressRatio * membersProgress * membersProgress);
	// </advc.115b>

	// <advc.115b> Commented out.
	/*if (GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE))
	{
    	iValue += 30;
	}
		
	iValue += (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? -20 : 0);
	*/ // </advc.115b>

	//int iNonsense = AI_getStrategyRand() + 90;
	iValue += (AI_getStrategyRand(6) % 30); // advc.115b: was %100 - way too random

	// BBAI TODO: Level 2?

	// <advc.115b> Commented out:
	/*if (iValue >= 100)
	{
		bDiploInclined = true;
	}*/

	if( bVoteEligible &&
		iValue > 150) // advc.115b: was bDiploInclined; isHuman clause removed
	{
		// BBAI TODO: Level 4 - close to enough to win a vote?
		// <advc.115b>
		if(iValue > 185 && membersProgress >= 1)
			return 4; // </advc.115b>

		return 3;
	}
	
	if( isHuman())
			// advc.115d: Commented out
			//&& !(GC.getGameINLINE().isDebugMode()) )
	{
		return 0;
	}
	// <advc.115b>
	if(iValue > 140)
		return 2; // </advc.115b>

	if(iValue > 110) // advc.115b: was bDiploInclined
	{
		return 1;
	}

	return 0;
}

/// \brief Returns whether player is pursuing a particular victory strategy.
///
/// Victory strategies are computed on demand once per turn and stored for the rest
/// of the turn.  Each victory strategy type has 4 levels, the first two are
/// determined largely from AI tendencies and random dice rolls.  The second
/// two are based on measurables and past actions, so the AI can use them to
/// determine what other players (including the human player) are doing.
bool CvPlayerAI::AI_isDoVictoryStrategy(int iVictoryStrategy) const
{
	if( isBarbarian() || isMinorCiv() || !isAlive() )
	{
		return false;
	}
	
    return (iVictoryStrategy & AI_getVictoryStrategyHash());
}

// K-Mod note. The bbai version of this function checked each victory type one at a time.
// I've changed it to test them all at once. This is possible since it's a bitfield.
bool CvPlayerAI::AI_isDoVictoryStrategyLevel4() const
{
	return AI_isDoVictoryStrategy(AI_VICTORY_SPACE4 | AI_VICTORY_CONQUEST4 | AI_VICTORY_CULTURE4 | AI_VICTORY_DOMINATION4 | AI_VICTORY_DIPLOMACY4);
}

// (same)
bool CvPlayerAI::AI_isDoVictoryStrategyLevel3() const
{	// advc.115b: Replaced DIPLO3 with DIPLO4 b/c Diplo3 often isn't close to victory
	return AI_isDoVictoryStrategy(AI_VICTORY_SPACE3 | AI_VICTORY_CONQUEST3 | AI_VICTORY_CULTURE3 | AI_VICTORY_DOMINATION3 | AI_VICTORY_DIPLOMACY4);
}

void CvPlayerAI::AI_updateVictoryStrategyHash()
{
	PROFILE_FUNC();

	if( isBarbarian() || isMinorCiv() || !isAlive() ||
			GET_TEAM(getTeam()).isCapitulated()) // advc.014
	{
		m_iVictoryStrategyHash = 0;
		return;
	}

	m_iVictoryStrategyHash = AI_DEFAULT_VICTORY_STRATEGY;
    //m_iVictoryStrategyHashCacheTurn = GC.getGameINLINE().getGameTurn();

	if (getCapitalCity() == NULL)
    {
        return;
    }

	bool bStartedOtherLevel3 = false;
	bool bStartedOtherLevel4 = false;

	// Space victory
	int iVictoryStage = AI_calculateSpaceVictoryStage();

	if( iVictoryStage >= 1 )
	{
		m_iVictoryStrategyHash |= AI_VICTORY_SPACE1;
	    if (iVictoryStage > 1)
	    {
	        m_iVictoryStrategyHash |= AI_VICTORY_SPACE2;
            if (iVictoryStage > 2)
            {
				bStartedOtherLevel3 = true;
                m_iVictoryStrategyHash |= AI_VICTORY_SPACE3;

				if (iVictoryStage > 3 && !bStartedOtherLevel4)
				{
					bStartedOtherLevel4 = true;
                	m_iVictoryStrategyHash |= AI_VICTORY_SPACE4;
                }
            }
	    }
	}

	// Conquest victory
	iVictoryStage = AI_calculateConquestVictoryStage();

	if( iVictoryStage >= 1 )
	{
		m_iVictoryStrategyHash |= AI_VICTORY_CONQUEST1;
	    if (iVictoryStage > 1)
	    {
	        m_iVictoryStrategyHash |= AI_VICTORY_CONQUEST2;
            if (iVictoryStage > 2)
            {
				bStartedOtherLevel3 = true;
                m_iVictoryStrategyHash |= AI_VICTORY_CONQUEST3;

				if (iVictoryStage > 3 && !bStartedOtherLevel4)
				{
					bStartedOtherLevel4 = true;
                	m_iVictoryStrategyHash |= AI_VICTORY_CONQUEST4;
                }
            }
	    }
	}

	// Domination victory
	iVictoryStage = AI_calculateDominationVictoryStage();

	if( iVictoryStage >= 1 )
	{
		m_iVictoryStrategyHash |= AI_VICTORY_DOMINATION1;
	    if (iVictoryStage > 1)
	    {
	        m_iVictoryStrategyHash |= AI_VICTORY_DOMINATION2;
            if (iVictoryStage > 2)
            {
				bStartedOtherLevel3 = true;
                m_iVictoryStrategyHash |= AI_VICTORY_DOMINATION3;

				if (iVictoryStage > 3 && !bStartedOtherLevel4)
				{
					bStartedOtherLevel4 = true;
                	m_iVictoryStrategyHash |= AI_VICTORY_DOMINATION4;
                }
            }
	    }
	}

	// Cultural victory
	// K-Mod Note: AI_calculateCultureVictoryStage now checks some of the other victory strategies,
	// so it is important that they are set first.
	iVictoryStage = AI_calculateCultureVictoryStage();

	if( iVictoryStage >= 1 )
	{
		m_iVictoryStrategyHash |= AI_VICTORY_CULTURE1;
	    if (iVictoryStage > 1)
	    {
	        m_iVictoryStrategyHash |= AI_VICTORY_CULTURE2;
            if (iVictoryStage > 2)
            {
				bStartedOtherLevel3 = true;
                m_iVictoryStrategyHash |= AI_VICTORY_CULTURE3;

				//if (iVictoryStage > 3 && !bStartedOtherLevel4)
				if (iVictoryStage > 3) // K-Mod. If we're close to a cultural victory - then allow Culture4 even with other stage4 strategies already running.
				{
					bStartedOtherLevel4 = true;
                	m_iVictoryStrategyHash |= AI_VICTORY_CULTURE4;
                }
            }
	    }
	}

	// Diplomacy victory
	iVictoryStage = AI_calculateDiplomacyVictoryStage();

	if( iVictoryStage >= 1 )
	{
		m_iVictoryStrategyHash |= AI_VICTORY_DIPLOMACY1;
	    if (iVictoryStage > 1)
	    {
	        m_iVictoryStrategyHash |= AI_VICTORY_DIPLOMACY2;
            if (iVictoryStage > 2 
				)/*&& !bStartedOtherLevel3) advc.115b: I really don't see
				 how pursuing diplo would get in the way of other victories,
				 and UN is often easier than a military victory. */
            {
				bStartedOtherLevel3 = true;
                m_iVictoryStrategyHash |= AI_VICTORY_DIPLOMACY3;

				if (iVictoryStage > 3
					)//&& !bStartedOtherLevel4) // advc.115b
				{
					bStartedOtherLevel4 = true;
                	m_iVictoryStrategyHash |= AI_VICTORY_DIPLOMACY4;
                }
            }
	    }
	}
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

// K-Mod, based on BBAI
void CvPlayerAI::AI_initStrategyRand()
{
	const unsigned iBits = 24;
	m_iStrategyRand = GC.getGameINLINE().getSorenRandNum((1<<(iBits+1))-1, "AI Strategy Rand");
}

int CvPlayerAI::AI_getStrategyRand(int iShift) const
{
	const unsigned iBits = 24; // cf bits in AI_initStrategyRand

	iShift += getCurrentEra();
	while (iShift < 0)
		iShift += iBits;
	iShift %= iBits;

	FAssert(m_iStrategyRand != UINT_MAX);
	unsigned x = 2654435761; // golden ratio of 2^32
	x *= (m_iStrategyRand << iShift) + (m_iStrategyRand >> (iBits - iShift));
	x >>= 1; // force positive;
	FAssert(x <= INT_MAX);
	return x;
}
// K-Mod end
// <advc.115b>
bool CvPlayerAI::isCloseToReligiousVictory() const {

	if(!AI_isDoVictoryStrategy(AI_VICTORY_DIPLOMACY3))
		return false;
	VoteSourceTypes vs = GET_TEAM(getTeam()).getLatestVictoryVoteSource();
	if(vs == NO_VOTESOURCE)
		return false;
	return GC.getGameINLINE().getVoteSourceReligion(vs) == getStateReligion();
}// </advc.115b>

bool CvPlayerAI::AI_isDoStrategy(int iStrategy) const
{
    if (isHuman())
    {
        return false;
    }

	if( isBarbarian() || isMinorCiv() || !isAlive() )
	{
		return false;
	}

    return (iStrategy & AI_getStrategyHash());
}

// K-mod. The body of this function use to be inside "AI_getStrategyHash"
void CvPlayerAI::AI_updateStrategyHash()
{

// K-Mod. Macros to help log changes in the AI strategy.
#define log_strat(s) \
	if (gPlayerLogLevel >= 2) \
	{ \
		if ((m_iStrategyHash & s) != (iLastStrategyHash & s)) \
		{ \
			logBBAI( "    Player %d (%S) %s strategy "#s" on turn %d", getID(), getCivilizationDescription(0), m_iStrategyHash & s ? "starts" : "stops", GC.getGameINLINE().getGameTurn()); \
		} \
	}
#define log_strat2(s, x) \
	if (gPlayerLogLevel >= 2) \
	{ \
		if ((m_iStrategyHash & s) != (iLastStrategyHash & s)) \
		{ \
			logBBAI( "    Player %d (%S) %s strategy "#s" on turn %d with "#x" %d", getID(), getCivilizationDescription(0), m_iStrategyHash & s ? "starts" : "stops", GC.getGameINLINE().getGameTurn(), x); \
		} \
	}
//

	const CvTeamAI& kTeam = GET_TEAM(getTeam()); // K-Mod. (and replaced all through this function)

    UnitTypes eLoopUnit;

	int iLastStrategyHash = m_iStrategyHash;
    
    m_iStrategyHash = AI_DEFAULT_STRATEGY;
    
	/* original bts code
	if (AI_getFlavorValue(FLAVOR_PRODUCTION) >= 2) // 0, 2, 5 or 10 in default xml [augustus 5, frederick 10, huayna 2, jc 2, chinese leader 2, qin 5, ramsess 2, roosevelt 5, stalin 2]
	{
		m_iStrategyHash |= AI_STRATEGY_PRODUCTION;
	} */ // K-Mod. This strategy is now set later on, with new conditions.
	
	if (getCapitalCity() == NULL)
    {
        return;
    }
    
    //int iNonsense = AI_getStrategyRand();
    
	int iMetCount = kTeam.getHasMetCivCount(true);
    
    //Unit Analysis
    int iBestSlowUnitCombat = -1;
    int iBestFastUnitCombat = -1;
    
    bool bHasMobileArtillery = false;
    bool bHasMobileAntiair = false;
    bool bHasBomber = false;
    
    int iNukeCount = 0;
    
    int iAttackUnitCount = 0;
	// K-Mod
	int iAverageEnemyUnit = 0;
	int iTypicalAttack = getTypicalUnitValue(UNITAI_ATTACK);
	int iTypicalDefence = getTypicalUnitValue(UNITAI_CITY_DEFENSE);
	int iWarSuccessRating = kTeam.AI_getWarSuccessRating();
	// K-Mod end

	for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));
		
		if (NO_UNIT != eLoopUnit)
		{
			if (getCapitalCity() != NULL)
			{
				if (getCapitalCity()->canTrain(eLoopUnit))
				{
					CvUnitInfo& kLoopUnit = GC.getUnitInfo(eLoopUnit);
					bool bIsUU = (GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex()) != (GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI));
					/* original bts code
					if (kLoopUnit.getUnitAIType(UNITAI_RESERVE) || kLoopUnit.getUnitAIType(UNITAI_ATTACK_CITY)
						|| kLoopUnit.getUnitAIType(UNITAI_COUNTER) || kLoopUnit.getUnitAIType(UNITAI_PILLAGE)) */
					// K-Mod. Original code was missing the obvious: UNITAI_ATTACK. Was this a bug? (I'm skipping "pillage".)
					if (kLoopUnit.getUnitAIType(UNITAI_ATTACK) || kLoopUnit.getUnitAIType(UNITAI_ATTACK_CITY)
						|| kLoopUnit.getUnitAIType(UNITAI_RESERVE) || kLoopUnit.getUnitAIType(UNITAI_COUNTER))
					// K-Mod end
					{
						iAttackUnitCount++;

						//UU love
						if (bIsUU)
						{
							if (kLoopUnit.getUnitAIType(UNITAI_ATTACK_CITY) || 
								(kLoopUnit.getUnitAIType(UNITAI_ATTACK)	&& !kLoopUnit.getUnitAIType(UNITAI_CITY_DEFENSE)))
							{
								iAttackUnitCount++;					
							}
						}
						int iCombat = kLoopUnit.getCombat();
						int iMoves = kLoopUnit.getMoves();
						if (iMoves == 1)
						{
							iBestSlowUnitCombat = std::max(iBestSlowUnitCombat, iCombat);
						}
						else if (iMoves > 1)
						{
							iBestFastUnitCombat = std::max(iBestFastUnitCombat, iCombat);
							if (bIsUU)
							{
								iBestFastUnitCombat += 1;
							}
						}
					}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       09/10/08                                jdog5000      */
/*                                                                                              */
/* Bugfix				                                                                         */
/************************************************************************************************/
/* original BTS code
					if (kLoopUnit.getMoves() > 1)
*/
					// Mobile anti-air and artillery flags only meant for land units
					if ( kLoopUnit.getDomainType() == DOMAIN_LAND && kLoopUnit.getMoves() > 1 )
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
					{
						if (kLoopUnit.getInterceptionProbability() > 25)
						{
							bHasMobileAntiair = true;
						}
						if (kLoopUnit.getBombardRate() > 10)
						{
							bHasMobileArtillery = true;
						}
					}

					if (kLoopUnit.getAirRange() > 1)
					{
						if (!kLoopUnit.isSuicide())
						{
							if ((kLoopUnit.getBombRate() > 10) && (kLoopUnit.getAirCombat() > 0))
							{
								bHasBomber = true;								
							}
						}
					}
					
					if (kLoopUnit.getNukeRange() > 0)
					{
						iNukeCount++;
					}
				}
			}
		}
	}

	// K-Mod
	{
		int iTotalPower = 0;
		int iTotalWeightedValue = 0;
		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			CvPlayer &kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
			if (kLoopPlayer.getTeam() != getTeam())
			{
				if (kLoopPlayer.isAlive() && kTeam.isHasMet(kLoopPlayer.getTeam()))
				{
					// Attack units are scaled down to roughly reflect their limitations.
					// (eg. Knights (10) vs Macemen (8). Cavalry (15) vs Rifles (14). Tank (28) vs Infantry (20) / Marine (24) )
					int iValue = std::max(100*kLoopPlayer.getTypicalUnitValue(UNITAI_ATTACK)/110, kLoopPlayer.getTypicalUnitValue(UNITAI_CITY_DEFENSE));

					iTotalWeightedValue += kLoopPlayer.getPower() * iValue;
					iTotalPower += kLoopPlayer.getPower();
				}
			}
		}
		if (iTotalPower == 0)
			iAverageEnemyUnit = 0;
		else
			iAverageEnemyUnit = iTotalWeightedValue / iTotalPower;
		// A bit of random variation...
		iAverageEnemyUnit *= (91+AI_getStrategyRand(1)%20);
		iAverageEnemyUnit /= 100;
	}

	//if (iAttackUnitCount <= 1)
	if ((iAttackUnitCount <= 1 && GC.getGame().getGameTurn() > GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getBarbPercent()/20)
		|| (100*iAverageEnemyUnit > 135*iTypicalAttack && 100*iAverageEnemyUnit > 135*iTypicalDefence))
	{
		m_iStrategyHash |= AI_STRATEGY_GET_BETTER_UNITS;
	}
	// K-Mod end
	if (iBestFastUnitCombat > iBestSlowUnitCombat)
	{
		m_iStrategyHash |= AI_STRATEGY_FASTMOVERS;
		if (bHasMobileArtillery && bHasMobileAntiair)
		{
			m_iStrategyHash |= AI_STRATEGY_LAND_BLITZ;
		}
	}
	if (iNukeCount > 0)
	{
		if ((GC.getLeaderHeadInfo(getPersonalityType()).getBuildUnitProb() + AI_getStrategyRand(7) % 15) >= (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 37 : 43))
		{
			m_iStrategyHash |= AI_STRATEGY_OWABWNW;
		}
	}
	if (bHasBomber)
	{
		if (!(m_iStrategyHash & AI_STRATEGY_LAND_BLITZ))
		{
			m_iStrategyHash |= AI_STRATEGY_AIR_BLITZ;
		}
		else
		{
			if ((AI_getStrategyRand(8) % 2) == 0)
			{
				m_iStrategyHash |= AI_STRATEGY_AIR_BLITZ;
				m_iStrategyHash &= ~AI_STRATEGY_LAND_BLITZ;				
			}
		}
	}

	log_strat(AI_STRATEGY_LAND_BLITZ)
	log_strat(AI_STRATEGY_AIR_BLITZ)
    
	//missionary
	{
	    if (getStateReligion() != NO_RELIGION)
	    {
            int iHolyCityCount = countHolyCities();
            if ((iHolyCityCount > 0) && hasHolyCity(getStateReligion()))
            {
                int iMissionary = 0;
                //Missionary
				/*  <advc.020> Replaced FLAVOR_GROWTH with FLAVOR_GOLD and set the
					multipliers to 3; was 2*Growth ("up to 10"), 4*Culture ("up to 40") */
                iMissionary += AI_getFlavorValue(FLAVOR_GOLD) * 3; // up to 15
                iMissionary += AI_getFlavorValue(FLAVOR_CULTURE) * 3; // up to 30
				// </advc.020>
                iMissionary += AI_getFlavorValue(FLAVOR_RELIGION) * 6; // up to 60
                
                CivicTypes eCivic = (CivicTypes)GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic();
                if ((eCivic != NO_CIVIC) && (GC.getCivicInfo(eCivic).isNoNonStateReligionSpread()))
                {
                	iMissionary += 20;
                }
                
                iMissionary += (iHolyCityCount - 1) * 5;
                
				iMissionary += std::min(iMetCount, 5) * 7;

                for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
                {
					if (iI != getID())
                    {
						if (GET_PLAYER((PlayerTypes)iI).isAlive() && kTeam.isHasMet(GET_PLAYER((PlayerTypes)iI).getTeam()))
						{
                            if (kTeam.isOpenBorders(GET_PLAYER((PlayerTypes)iI).getTeam()) ||
									// advc.001i
									kTeam.isFriendlyTerritory(GET_PLAYER((PlayerTypes)iI).getTeam()))
                            {
								if ((GET_PLAYER((PlayerTypes)iI).getStateReligion() == getStateReligion()))
								{
									iMissionary += 10;
								}
								else if( !GET_PLAYER((PlayerTypes)iI).isNoNonStateReligionSpread() )
								{
									iMissionary += (GET_PLAYER((PlayerTypes)iI).countHolyCities() == 0) ? 12 : 4;
								}
							}
                        }
                    }
                }
                
                iMissionary += (AI_getStrategyRand(9) % 7) * 3;
				// <advc.115b>
				if(iMissionary > 0 && isCloseToReligiousVictory())
					iMissionary = ::round(1.7 * iMissionary); // </advc.115b>
                if (iMissionary > 100)
                {
                    m_iStrategyHash |= AI_STRATEGY_MISSIONARY;
                }
            }
	    }
	}

	// Espionage
	// K-Mod
	// Apparently BBAI wanted to use "big espionage" to save points when our espionage is weak.
	// I've got other plans.
	if (!GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		// don't start espionage strategy if we have no spies
		if (iLastStrategyHash & AI_STRATEGY_BIG_ESPIONAGE || AI_getNumAIUnits(UNITAI_SPY) > 0)
		{
			int iTempValue = 0;
			iTempValue += AI_commerceWeight(COMMERCE_ESPIONAGE) / 8;
			// Note: although AI_commerceWeight is doubled for Big Espionage,
			// the value here is unaffected because the strategy hash has been cleared.
			iTempValue += kTeam.getBestKnownTechScorePercent() < 85 ? 3 : 0;
			iTempValue += kTeam.getAnyWarPlanCount(true) > kTeam.getAtWarCount(true) ? 2 : 0; // build up espionage before the start of a war
			if (iWarSuccessRating < 0)
				iTempValue += iWarSuccessRating/15 - 1;
			iTempValue += AI_getStrategyRand(10) % 8;

			if (iTempValue > 11)
			{
				// Don't allow big esp if we have no local esp targets
				for (int i = 0; i < MAX_CIV_TEAMS; i++)
				{
					if (i != getTeam() && kTeam.isHasMet((TeamTypes)i) && kTeam.AI_hasCitiesInPrimaryArea((TeamTypes)i))
					{
						m_iStrategyHash |= AI_STRATEGY_BIG_ESPIONAGE;
						break;
					}
				}
			}
		}

		// The espionage economy decision is actually somewhere else. This is just a marker.
		if (getCommercePercent(COMMERCE_ESPIONAGE) > 20)
			m_iStrategyHash |= AI_STRATEGY_ESPIONAGE_ECONOMY;
	}
	log_strat(AI_STRATEGY_ESPIONAGE_ECONOMY)
	// K-Mod end

	// Turtle strategy
	if( kTeam.getAtWarCount(true) > 0 && getNumCities() > 0 )
	{
		int iMaxWarCounter = 0;
		for( int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++ )
		{
			if( iTeam != getTeam() )
			{
				if( GET_TEAM((TeamTypes)iTeam).isAlive() && !GET_TEAM((TeamTypes)iTeam).isMinorCiv() )
				{
					iMaxWarCounter = std::max( iMaxWarCounter, kTeam.AI_getAtWarCounter((TeamTypes)iTeam) );
				}
			}
		}

		// Are we losing badly or recently attacked?
		if( iWarSuccessRating < -50 || iMaxWarCounter < 10 )
		{
			if( kTeam.AI_getEnemyPowerPercent(true) > std::max(150, GC.getDefineINT("BBAI_TURTLE_ENEMY_POWER_RATIO")) 
					// advc.107
					&& getNumMilitaryUnits() < (5 + getCurrentEra() * 1.5) * getNumCities())
			{
				m_iStrategyHash |= AI_STRATEGY_TURTLE;
			}
		}
	}
	log_strat(AI_STRATEGY_TURTLE)

	int iCurrentEra = getCurrentEra();
	int iParanoia = 0;
	int iCloseTargets = 0;
	int iOurDefensivePower = kTeam.getDefensivePower();

    for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
    {
		const CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		const CvTeamAI& kLoopTeam = GET_TEAM(kLoopPlayer.getTeam());
		// <advc.003>
		if (!kLoopPlayer.isAlive() || kLoopPlayer.isMinorCiv())
			continue;
		if (kLoopPlayer.getTeam() == getTeam() || !kTeam.isHasMet(kLoopPlayer.getTeam()))
			continue;
		if (kLoopTeam.isAVassal() || kTeam.isVassal(kLoopPlayer.getTeam()))
			continue; // </advc.003>
		// <advc.022> Don't fear (AI) civs that are busy
		if(!kLoopPlayer.isHuman() && kLoopPlayer.isFocusWar() &&
				kLoopTeam.getAtWarCount() > 0 &&
				kLoopTeam.AI_getWarSuccessRating() < 50)
			continue; // </advc.022>
		bool bCitiesInPrime = kTeam.AI_hasCitiesInPrimaryArea(kLoopPlayer.getTeam()); // K-Mod

		if (kTeam.AI_getWarPlan(kLoopPlayer.getTeam()) != NO_WARPLAN)
		{
			iCloseTargets++;
			continue; // advc.003
		}
		// Are they a threat?
		int iTempParanoia = 0;

		int iTheirPower = kLoopTeam.getPower(true);
		if (4*iTheirPower > 3*iOurDefensivePower)
		{
			if (//kLoopTeam.getAtWarCount(true) == 0
				!kLoopPlayer.isFocusWar() // advc.105: Replacing the above
				|| kLoopTeam.AI_getEnemyPowerPercent(false) < 140 )
			{
				// Memory of them declaring on us and our friends
				int iWarMemory = AI_getMemoryCount((PlayerTypes)iI, MEMORY_DECLARED_WAR);
				iWarMemory += (AI_getMemoryCount((PlayerTypes)iI, MEMORY_DECLARED_WAR_ON_FRIEND) + 1)/2;
				iWarMemory = (int)ceil(iWarMemory / 2.5); // advc.130j
				if (iWarMemory > 0)
				{
					//they are a snake
					iTempParanoia += 50 + 50 * iWarMemory;

					if( gPlayerLogLevel >= 2 )
					{
						logBBAI( "    Player %d (%S) wary of %S because of war memory %d", getID(), getCivilizationDescription(0), GET_PLAYER((PlayerTypes)iI).getCivilizationDescription(0), iWarMemory);
					}
				}
			}
		}

		// Do we think our relations are bad?
		int iCloseness = AI_playerCloseness((PlayerTypes)iI, DEFAULT_PLAYER_CLOSENESS);
		// <advc.022>
		if(!AI_hasSharedPrimaryArea(kLoopPlayer.getID())) {
			int noSharePenalty = 99;
			EraTypes loopEra = kLoopPlayer.getCurrentEra();
			if(loopEra >= 3)
				noSharePenalty -= 33;
			if(loopEra >= 4)
				noSharePenalty -= 33;
			if(loopEra >= 5)
				noSharePenalty -= 33;
			iCloseness = std::max(0, iCloseness - noSharePenalty);
		} // </advc.022>
		// if (iCloseness > 0)
		if (iCloseness > 0 || bCitiesInPrime) // K-Mod
		{
			// <advc.022>
			// Humans tend to reciprocate our feelings
			int humanWarProb = 70 - AI_getAttitude(kLoopPlayer.getID()) * 10;
			int iAttitudeWarProb = (kLoopPlayer.isHuman() ? humanWarProb :
					// Now based on kLoopPlayer's personality and attitude
					100 - GC.getLeaderHeadInfo(
					kLoopPlayer.getPersonalityType()).
					getNoWarAttitudeProb(kLoopPlayer.AI_getAttitude(getID())));
			// </advc.022>
			// (original BBAI code deleted) // advc.003
			// K-Mod. Paranoia gets scaled by relative power anyway...
			iTempParanoia += std::max(0, iAttitudeWarProb/2);
			/*  <advc.022> This is about us attacking someone (not relevant for
				Paranoia, but for Dagger, so our attitude should be used.
				K-Mod already did that, but iAttitudeWarProb now refers to
				kLoopPlayer. */
			//if (iAttitudeWarProb > 10 && iCloseness > 0)
			if(iCloseness > 0 && 100 - GC.getLeaderHeadInfo(getPersonalityType()).
					getNoWarAttitudeProb(AI_getAttitude(kLoopPlayer.getID())) > 10)
			{ // </advc.022>
				iCloseTargets++;
			}
			// K-Mod end
			/*  advc.022: Commented out. Defensive measures make most sense
				around 150% power ratio; 200% is probably a lost cause. */
			/*if( iTheirPower > 2*iOurDefensivePower )
			{
				//if( AI_getAttitude((PlayerTypes)iI) != ATTITUDE_FRIENDLY )
				// advc.022: Replacing the above
				if(iAttitudeWarProb > 0)
				{
					iTempParanoia += 25;
				}
			}*/
		}

		if( iTempParanoia > 0 )
		{
			iTempParanoia *= std::min(iTheirPower,
					// advc.022: At most double paranoia based on power ratio
					2 * iOurDefensivePower);
			iTempParanoia /= std::max(1, iOurDefensivePower);
			// K-Mod
			if (kLoopTeam.AI_getWorstEnemy() == getTeam())
			{
				//iTempParanoia *= 2;
				/*  advc.022: Don't give their attitude too much weight (replacing
					the above) */
				iTempParanoia = ::round(iTempParanoia * 1.5);
			}
			// K-Mod end
		}
		
		// Do they look like they're going for militaristic victory?
		// advc.022: New temp variable
		int victStratParanoia = 0;
		if( kLoopPlayer.AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST4) )
		{
			victStratParanoia += 200;
		}
		else if( kLoopPlayer.AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3) )
		{
			victStratParanoia += 100;
		}
		else if( kLoopPlayer.AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION3) )
		{
			victStratParanoia += 50;
		}
		/*  advc.022: Too high in K-Mod I think; who knows when they'll get around
			to attack us. (Could count the alternative targets I guess ...). */
		iTempParanoia += victStratParanoia / 2;
		if( iTempParanoia > 0 )
		{	// <advc.022> Replace this with something smoother
			/*if( iCloseness == 0 )
			{
				iTempParanoia /= 2;
			}*/
			double multiplier = 2;
			/*  I don't think closeness is intended to be a percentage, but based on
				some sample values (Ctrl key on the capital in debug mode; closeness
				is shown in square brackets), it tends to between 0 and 100. */
			multiplier *= ::dRange(iCloseness / 100.0, 0.0, 1.0) + 0.3;;
			// <advc.022> Reduced paranoia if resistance futile
			double powRatio = iTheirPower / (double)iOurDefensivePower;
			/*  No change if ratio is 165% or less; 215% -> 50% reduced paranoia;
				260% -> 0 paranoia */
			multiplier *= std::max(0.0, 1 - std::max(0.0, powRatio - 1.65));
			iTempParanoia = ::round(iTempParanoia * multiplier);
			// </advc.022>
			iParanoia += iTempParanoia;
		}
    }
	/*  advc.022: Commented out. BETTER_UNITS is supposed to make us train fewer
		units (b/c they're no good), but high paranoia will lead to more units. */
	/*if( m_iStrategyHash & AI_STRATEGY_GET_BETTER_UNITS )
	{
		iParanoia *= 3;
		iParanoia /= 2;
	}*/

	// Scale paranoia in later eras/larger games
	//iParanoia -= (100*(iCurrentEra + 1)) / std::max(1, GC.getNumEraInfos());

	// K-Mod. You call that scaling for "later eras/larger games"? It isn't scaling, and it doesn't use the map size.
	// Lets try something else. Rough and ad hoc, but hopefully a bit better.
	iParanoia *= (3*GC.getNumEraInfos() - 2*iCurrentEra);
	iParanoia /= 3*(std::max(1, GC.getNumEraInfos()));
	// That starts as a factor of 1, and drop to 1/3.  And now for game size...
	iParanoia *= 14;
	iParanoia /= (7+std::max(kTeam.getHasMetCivCount(true),
			GC.getGame().getRecommendedPlayers() // advc.137
			//GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getDefaultPlayers()
		));

	// Alert strategy
	if( iParanoia >= 200 )
	{
		m_iStrategyHash |= AI_STRATEGY_ALERT1;
		if( iParanoia >= 400
				&& !isFocusWar()) // advc.022: Need to focus on the war at hand
		{
			m_iStrategyHash |= AI_STRATEGY_ALERT2;
		}
	}
	log_strat2(AI_STRATEGY_ALERT1, iParanoia)
	log_strat2(AI_STRATEGY_ALERT2, iParanoia)

	// Economic focus (K-Mod) - This strategy is a gambit. The goal is to tech faster by neglecting military.
	if(!isFocusWar()) // advc.105
	//if (kTeam.getAnyWarPlanCount(true) == 0)
	{
		int iFocus = (100 - iParanoia) / 20;
		iFocus += std::max(iAverageEnemyUnit - std::max(iTypicalAttack, iTypicalDefence), std::min(iTypicalAttack, iTypicalDefence) - iAverageEnemyUnit) / 12;
		//Note: if we haven't met anyone then average enemy is zero. So this essentially assures economic strategy when in isolation.
		iFocus += (AI_getPeaceWeight() + AI_getStrategyRand(2)%10)/3; // note: peace weight will be between 0 and 12

		if (iFocus >= 12
				|| feelsSafe()) // advc.109
			m_iStrategyHash |= AI_STRATEGY_ECONOMY_FOCUS;
	}
	log_strat(AI_STRATEGY_ECONOMY_FOCUS)

	/*  <advc.104f> Don't want dagger even if UWAI only in the background
		b/c it gets in the way of testing. If UWAI fully disabled, allow dagger
		with additional restrictions. */
	bool noDagger = getWPAI.isEnabled() || getWPAI.isEnabled(true);
	for(int i = 0; i < MAX_CIV_TEAMS; i++) {
		TeamTypes tId = (TeamTypes)i;
		CvTeam const& t = GET_TEAM(tId);
		if(t.isAlive() && !t.isMinorCiv() && t.isAtWar(getTeam()) &&
				GET_TEAM(getTeam()).AI_isChosenWar(tId)) {
			noDagger = true;
			break;
		}
	} // </advc.104f>

	// BBAI TODO: Integrate Dagger with new conquest victory strategy, have Dagger focus on early rushes
    //dagger
	if( !(AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2)) 
	 && !(m_iStrategyHash & AI_STRATEGY_MISSIONARY)
     && (iCurrentEra <= (2+(AI_getStrategyRand(11)%2))) && (iCloseTargets > 0)
	 && !noDagger // advc.104f
	 )
    {	    
	    int iDagger = 0;
	    iDagger += 12000 / std::max(100, (50 + GC.getLeaderHeadInfo(getPersonalityType()).getMaxWarRand()));
	    iDagger *= (AI_getStrategyRand(12) % 11);
	    iDagger /= 10;
	    iDagger += 5 * std::min(8, AI_getFlavorValue(FLAVOR_MILITARY));
	    
        for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
        {
            eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

            if ((eLoopUnit != NO_UNIT) && (GC.getUnitInfo(eLoopUnit).getCombat() > 0))
            {
                if ((GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex()) != (GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)))
                {
                	bool bIsDefensive = (GC.getUnitInfo(eLoopUnit).getUnitAIType(UNITAI_CITY_DEFENSE) &&
                		!GC.getUnitInfo(eLoopUnit).getUnitAIType(UNITAI_RESERVE));
					
					iDagger += bIsDefensive ? -10 : 0;
                       
                    if (getCapitalCity()->canTrain(eLoopUnit))
                    {
                        iDagger += bIsDefensive ? 10 : 40;
                        
                        int iUUStr = GC.getUnitInfo(eLoopUnit).getCombat();
                        int iNormalStr = GC.getUnitInfo((UnitTypes)(GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex())).getCombat();
                        iDagger += 20 * range((iUUStr - iNormalStr), 0, 2);
						if (GC.getUnitInfo(eLoopUnit).getPrereqAndTech() == NO_TECH)
                        {
                            iDagger += 20;
                        }
                    }
                    else
                    {
                        if (GC.getUnitInfo(eLoopUnit).getPrereqAndTech() != NO_TECH)
                        {
                            if (GC.getTechInfo((TechTypes)(GC.getUnitInfo(eLoopUnit).getPrereqAndTech())).getEra() <= (iCurrentEra + 1))
                            {
                                if (kTeam.isHasTech((TechTypes)GC.getUnitInfo(eLoopUnit).getPrereqAndTech()))
                                {
                                	//we have the tech but can't train the unit, dejection.
                                    iDagger += 10;
                                }
                                else
                                {
                                	//we don't have the tech, it's understandable we can't train.
                                    iDagger += 30;
                                }
                            }
                        }
                                
                        bool bNeedsAndBonus = false;
                        int iOrBonusCount = 0;
                        int iOrBonusHave = 0;
                        
                        for (int iJ = 0; iJ < GC.getNumBonusInfos(); iJ++)
                        {
                            BonusTypes eBonus = (BonusTypes)iJ;
                            if (eBonus != NO_BONUS)
                            {
                                if (GC.getUnitInfo(eLoopUnit).getPrereqAndBonus() == eBonus)
                                {
                                    if (getNumTradeableBonuses(eBonus) == 0)
                                    {
                                        bNeedsAndBonus = true;
                                    }
                                }

                                for (int iK = 0; iK < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); iK++)
                                {
                                    if (GC.getUnitInfo(eLoopUnit).getPrereqOrBonuses(iK) == eBonus)
                                    {
                                        iOrBonusCount++;
                                        if (getNumTradeableBonuses(eBonus) > 0)
                                        {
                                            iOrBonusHave++;
                                        }
                                    }
                                }
                            }
                        }
                        
                        
                        iDagger += 20;
                        if (bNeedsAndBonus)
                        {
                            iDagger -= 20;
                        }
                        if ((iOrBonusCount > 0) && (iOrBonusHave == 0))
                        {
                            iDagger -= 20;
                        }
                    }
                }
            }
        }
        
        if (!GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI))
        {
			iDagger += range(100 - GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAITrainPercent(), 0, 15);
        }
        
        if ((getCapitalCity()->area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE) || (getCapitalCity()->area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE))
        {
            iDagger += (iAttackUnitCount > 0) ? 40 : 20;
        }
        
        if (iDagger >= AI_DAGGER_THRESHOLD)
        {
            m_iStrategyHash |= AI_STRATEGY_DAGGER;            
        }
		else
		{
			if( iLastStrategyHash &= AI_STRATEGY_DAGGER )
			{
				if (iDagger >= (9*AI_DAGGER_THRESHOLD)/10)
				{
					m_iStrategyHash |= AI_STRATEGY_DAGGER;            
				}
			}
		}

		log_strat2(AI_STRATEGY_DAGGER, iDagger)
	}
	
	if (!(m_iStrategyHash & AI_STRATEGY_ALERT2) && !(m_iStrategyHash & AI_STRATEGY_TURTLE))
	{//Crush
		int iWarCount = 0;
		int iCrushValue = 0;
		

	// K-Mod. (experimental)
		//iCrushValue += (iNonsense % 4);
		// A leader dependant value. (MaxWarRand is roughly between 50 and 200. Gandi is 400.)
		//iCrushValue += (iNonsense % 3000) / (400+GC.getLeaderHeadInfo(getPersonalityType()).getMaxWarRand());
	// On second thought, lets try this
		iCrushValue += AI_getStrategyRand(13) % (4 + AI_getFlavorValue(FLAVOR_MILITARY)/2 + (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 2 : 0));
		iCrushValue += std::min(0, kTeam.AI_getWarSuccessRating()/15);
		// note: flavor military is between 0 and 10
		// K-Mod end
		/*  advc.018: Commented out.
			Not clear that conq3 should make the AI more willing to neglect
			its defense. Also, once crush is adopted, the AI becomes more inclined
			towards conquest; should only work one way. */
		/*if (AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3))
		{
			iCrushValue += 1;
		}*/
		
		if (m_iStrategyHash & AI_STRATEGY_DAGGER)
		{
			iCrushValue += 3;			
		}

		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			if ((GET_TEAM((TeamTypes)iI).isAlive()) && (iI != getTeam()))
			{
				if (kTeam.AI_getWarPlan((TeamTypes)iI) != NO_WARPLAN)
				{
					if (!GET_TEAM((TeamTypes)iI).isAVassal())
					{
						if (kTeam.AI_teamCloseness((TeamTypes)iI) > 0)
						{
							iWarCount++;
						}

						// K-Mod. (if we attack with our defenders, would they beat their defenders?)
						if (100*iTypicalDefence >= 110 * GET_TEAM((TeamTypes)iI).getTypicalUnitValue(UNITAI_CITY_DEFENSE))
						{
							iCrushValue += 2;
						}
					}
					// advc.018: New temp variable
					int crushValFromWarplanType = 0;
					if (kTeam.AI_getWarPlan((TeamTypes)iI) == WARPLAN_PREPARING_TOTAL)
					{
						crushValFromWarplanType += 6;
					}
					else if ((kTeam.AI_getWarPlan((TeamTypes)iI) == WARPLAN_TOTAL) &&
							/*  advc.104q: Was AI_getWarPlanStateCounter. Probably war duration was
								intended. That said, I'm now also resetting WarPlanStateCounter upon
								a DoW (see advc.104q in CvTeam::declareWar), so that it shouldn't make
								a difference which function is used (in this context). */
							(kTeam.AI_getAtWarCounter((TeamTypes)iI) < 20))
					{
						crushValFromWarplanType += 6;
					}
					
					if ((kTeam.AI_getWarPlan((TeamTypes)iI) == WARPLAN_DOGPILE) &&
							// advc.104q:
							(kTeam.AI_getAtWarCounter((TeamTypes)iI) < 20))
					{
						for (int iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
						{
							if ((iJ != iI) && iJ != getTeam() && GET_TEAM((TeamTypes)iJ).isAlive())
							{
								if ((atWar((TeamTypes)iI, (TeamTypes)iJ)) && !GET_TEAM((TeamTypes)iI).isAVassal())
								{
									crushValFromWarplanType += 4;
								}
							}
						}
					}
					// advc.018: Halved the impact
					iCrushValue += crushValFromWarplanType / 2;
				}
			}
		}
		//if ((iWarCount <= 1) && (iCrushValue >= ((iLastStrategyHash & AI_STRATEGY_CRUSH) ? 9 :10)))
		if ((iWarCount == 1) && (iCrushValue >= ((iLastStrategyHash & AI_STRATEGY_CRUSH) ? 9 :10))) // K-Mod
		{
			m_iStrategyHash |= AI_STRATEGY_CRUSH;
		}

		log_strat2(AI_STRATEGY_CRUSH, iCrushValue)
	}

	// K-Mod
	{//production
		int iProductionValue = AI_getStrategyRand(2) % (5 + AI_getFlavorValue(FLAVOR_PRODUCTION)/2);
		iProductionValue += (iLastStrategyHash & AI_STRATEGY_PRODUCTION) ? 1 : 0;
		iProductionValue += AI_getFlavorValue(FLAVOR_PRODUCTION) > 0 ? 1 : 0;
		iProductionValue += (m_iStrategyHash & AI_STRATEGY_DAGGER) ? 1 : 0;
		// advc.018: Commented out
		//iProductionValue += (m_iStrategyHash & AI_STRATEGY_CRUSH) ? 1 : 0;
		iProductionValue += AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST4 | AI_VICTORY_SPACE4) ? 3 : 0;
		// warplans. (done manually rather than using getWarPlanCount, so that we only have to do the loop once.)
		bool bAnyWarPlans = false;
		bool bTotalWar = false;
		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			const CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
			if (kLoopTeam.isAlive() && !kLoopTeam.isMinorCiv())
			{
				switch (kTeam.AI_getWarPlan((TeamTypes)iI))
				{
				case NO_WARPLAN:
					break;
				case WARPLAN_PREPARING_TOTAL:
				case WARPLAN_TOTAL:
					bTotalWar = true;
				default:
					bAnyWarPlans = true;
					break;
				}
			}
		}
		iProductionValue += bAnyWarPlans ? 1 : 0;
		iProductionValue += bTotalWar ? 3 : 0;

		if (iProductionValue >= 10)
		{
			m_iStrategyHash |= AI_STRATEGY_PRODUCTION;
		}
		log_strat2(AI_STRATEGY_PRODUCTION, iProductionValue)
	}
	// K-Mod end

	{
		int iOurVictoryCountdown = kTeam.AI_getLowestVictoryCountdown();

		int iTheirVictoryCountdown = MAX_INT;
		
		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       02/14/10                             jdog5000         */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original bts code
			if ((GET_TEAM((TeamTypes)iI).isAlive()) && (iI != getID()))
*/
			if ((GET_TEAM((TeamTypes)iI).isAlive()) && (iI != getTeam()))
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
			{
				CvTeamAI& kOtherTeam = GET_TEAM((TeamTypes)iI);
				iTheirVictoryCountdown = std::min(iTheirVictoryCountdown, kOtherTeam.AI_getLowestVictoryCountdown());
			}
		}

		if (MAX_INT == iTheirVictoryCountdown)
		{
			iTheirVictoryCountdown = -1;
		}

		if ((iOurVictoryCountdown >= 0) && (iTheirVictoryCountdown < 0 || iOurVictoryCountdown <= iTheirVictoryCountdown))
		{
			m_iStrategyHash |= AI_STRATEGY_LAST_STAND;
		}
		else if ((iTheirVictoryCountdown >= 0))
		{
			if((iTheirVictoryCountdown < iOurVictoryCountdown))
			{
				m_iStrategyHash |= AI_STRATEGY_FINAL_WAR;
			}
			else if( GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) )
			{
				m_iStrategyHash |= AI_STRATEGY_FINAL_WAR;
			}
			else if( AI_isDoVictoryStrategyLevel4() || AI_isDoVictoryStrategy(AI_VICTORY_SPACE3) )
			{
				m_iStrategyHash |= AI_STRATEGY_FINAL_WAR;
			}
		}

		if (iOurVictoryCountdown < 0)
		{
			if (isCurrentResearchRepeat())
			{
				int iStronger = 0;
				int iAlive = 1;
				for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
				{
					if (iTeam != getTeam())
					{
						CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
						if (kLoopTeam.isAlive())
						{
							iAlive++;
							if (kTeam.getPower(true) < kLoopTeam.getPower(true))
							{
								iStronger++;
							}
						}
					}
				}
				
				if ((iStronger <= 1) || (iStronger <= iAlive / 4))
				{
					m_iStrategyHash |= AI_STRATEGY_FINAL_WAR;
				}
			}
		}
		
	}

	if (isCurrentResearchRepeat())
	{
		int iTotalVictories = 0;
		int iAchieveVictories = 0;
		int iWarVictories = 0;
		
		
		int iThreshold = std::max(1, (GC.getGame().countCivTeamsAlive() + 1) / 4);
		
		for (int iVictory = 0; iVictory < GC.getNumVictoryInfos(); iVictory++)
		{
			CvVictoryInfo& kVictory = GC.getVictoryInfo((VictoryTypes)iVictory);
			if (GC.getGame().isVictoryValid((VictoryTypes)iVictory))
			{
				iTotalVictories ++;
				if (kVictory.isDiploVote())
				{
					//
				}
				else if (kVictory.isEndScore())
				{
					int iHigherCount = 0;
					int IWeakerCount = 0;
					for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
					{
						if (iTeam != getTeam())
						{
							CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
							if (kLoopTeam.isAlive())
							{
								if (GC.getGame().getTeamScore(getTeam()) < ((GC.getGame().getTeamScore((TeamTypes)iTeam) * 90) / 100))
								{
									iHigherCount++;
									if (kTeam.getPower(true) > kLoopTeam.getPower(true))
									{
										IWeakerCount++;
									}
								}
							}
						}
					}

					if (iHigherCount > 0)
					{
						if (IWeakerCount == iHigherCount)
						{
							iWarVictories++;
						}
					}
				}
				else if (kVictory.getCityCulture() > 0)
				{
					if (m_iStrategyHash & AI_VICTORY_CULTURE1)
					{
						iAchieveVictories++;
					}
				}
				else if (kVictory.getMinLandPercent() > 0 || kVictory.getLandPercent() > 0)
				{
					int iLargerCount = 0;
					for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
					{
						if (iTeam != getTeam())
						{
							CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
							if (kLoopTeam.isAlive())
							{
								if (kTeam.getTotalLand(true) < kLoopTeam.getTotalLand(true))
								{
									iLargerCount++;
								}
							}
						}
					}
					if (iLargerCount <= iThreshold)
					{
						iWarVictories++;
					}
				}
				else if (kVictory.isConquest())
				{
					int iStrongerCount = 0;
					for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
					{
						if (iTeam != getTeam())
						{
							CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
							if (kLoopTeam.isAlive())
							{
								if (kTeam.getPower(true) < kLoopTeam.getPower(true))
								{
									iStrongerCount++;
								}
							}
						}
					}
					if (iStrongerCount <= iThreshold)
					{
						iWarVictories++;
					}
				}
				else
				{
					if (kTeam.getVictoryCountdown((VictoryTypes)iVictory) > 0)
					{
						iAchieveVictories++;
					}
				}
			}
		}

		if (iAchieveVictories == 0)
		{
			if (iWarVictories > 0)
			{
				m_iStrategyHash |= AI_STRATEGY_FINAL_WAR;
			}
		}
	}
	
	
	//Turn off inappropriate strategies.
	if (GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE))
	{
		m_iStrategyHash &= ~AI_STRATEGY_DAGGER;
		m_iStrategyHash &= ~AI_STRATEGY_CRUSH;
		m_iStrategyHash &= ~AI_STRATEGY_ALERT1;
		m_iStrategyHash &= ~AI_STRATEGY_ALERT2;
		m_iStrategyHash &= ~AI_STRATEGY_TURTLE;
		m_iStrategyHash &= ~AI_STRATEGY_FINAL_WAR;
		m_iStrategyHash &= ~AI_STRATEGY_LAST_STAND;

		m_iStrategyHash &= ~AI_STRATEGY_OWABWNW;
		m_iStrategyHash &= ~AI_STRATEGY_FASTMOVERS;
	}
#undef log_strat
#undef log_strat2
}

// K-Mod
void CvPlayerAI::AI_updateGreatPersonWeights()
{
	PROFILE_FUNC();
	int iLoop;

	m_GreatPersonWeights.clear();
	
	if (isHuman())
		return; // Humans can use their own reasoning for choosing specialists

	for (int i = 0; i < GC.getNumSpecialistInfos(); i++)
	{
		UnitClassTypes eGreatPersonClass = (UnitClassTypes)GC.getSpecialistInfo((SpecialistTypes)i).getGreatPeopleUnitClass();

		if (eGreatPersonClass == NO_UNITCLASS)
			continue;

		FAssert(GC.getSpecialistInfo((SpecialistTypes)i).getGreatPeopleRateChange() > 0);

		if (m_GreatPersonWeights.find(eGreatPersonClass) != m_GreatPersonWeights.end())
			continue; // already evaluated

		UnitTypes eGreatPerson = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(eGreatPersonClass);
		if (eGreatPerson == NO_UNIT)
			continue;

		/* I've disabled the validity check, because 'invalid' specialists still affect the value of the things that enable them.
		bool bValid = isSpecialistValid((SpecialistTypes)i) || i == GC.getDefineINT("DEFAULT_SPECIALIST");
		int iLoop; // moved.
		for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity && !bValid; pLoopCity = nextCity(&iLoop))
		{
			if (pLoopCity->getMaxSpecialistCount((SpecialistTypes)i) > 0)
			{
				bValid = true;
				break;
			}
		}

		if (!bValid)
			continue; */

		// We've estabilish that we can use this specialist, and that they provide great-person points for a unit which
		// we have not yet evaluated. So now we just have to evaluate the unit!
		const CvUnitInfo& kInfo = GC.getUnitInfo(eGreatPerson);

		if (kInfo.getUnitCombatType() != NO_UNITCOMBAT)
			continue; // don't try to evaluate combat units.

		int iValue = 0;
		// value of joining a city as a super-specialist
		for (int j = 0; j < GC.getNumSpecialistInfos(); j++)
		{
			if (kInfo.getGreatPeoples((SpecialistTypes)j))
			{
				for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
				{
					// Note, specialistValue is roughly 400x the commerce it provides. So /= 4 to make it 100x.
					int iTempValue = pLoopCity->AI_permanentSpecialistValue((SpecialistTypes)j)/4;
					iValue = std::max(iValue, iTempValue);
				}
			}
		}
		// value of building something.
		for (int j = 0; j < GC.getNumBuildingClassInfos(); j++)
		{
			BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(j);

			if (eBuilding != NO_BUILDING)
			{
				if (kInfo.getForceBuildings(eBuilding) ||
					(kInfo.getBuildings(eBuilding) && canConstruct(eBuilding, false, false, true)))
				{
					for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
					{
						// cf. conditions used in CvUnit::canConstruct
						if (pLoopCity->getNumRealBuilding(eBuilding) > 0 ||
							(!kInfo.getForceBuildings(eBuilding) && !pLoopCity->canConstruct(eBuilding, false, false, true)))
						{
							continue;
						}

						// Note, building value is roughly 4x the value of the commerce it provides.
						// so we * 25 to match the scale of specialist value.
						int iTempValue = pLoopCity->AI_buildingValue(eBuilding) * 25;

						// if the building is a world wonder, increase the value.
						if (isWorldWonderClass((BuildingClassTypes)j))
							iTempValue = 3*iTempValue/2;

						// reduce the value if we already have great people of this type
						iTempValue /= 1 + AI_totalUnitAIs((UnitAITypes)kInfo.getDefaultUnitAIType());

						iValue = std::max(iValue, iTempValue);
					}
				}
			}
		}
		// don't bother trying to evaluate bulbing etc. That kind of value is too volatile anyway.

		// store the value in the weights map - but remember, this isn't yet the actual weight.
		FAssert(iValue >= 0);
		m_GreatPersonWeights[eGreatPersonClass] = iValue;
	}

	// find the mean value.
	int iSum = 0;

	std::map<UnitClassTypes, int>::iterator it;
	for (it = m_GreatPersonWeights.begin(); it != m_GreatPersonWeights.end(); ++it)
	{
		iSum += it->second;
	}
	int iMean = iSum / std::max(1, (int)m_GreatPersonWeights.size());

	// scale the values so that they are between 50 and 500 (depending on flavour), with the mean value translating to 100.
	// (note: I don't expect it to get anywhere near the maximum. The maximum only occurs when the value is infinite!)
	const int iMin = AI_getFlavorValue(FLAVOR_SCIENCE) == 0 ? 50 : 35 - AI_getFlavorValue(FLAVOR_SCIENCE);
	FAssert(iMin > 10 && iMin < 95);
	// (smaller iMin means more focus on high-value specialists)
	for (it = m_GreatPersonWeights.begin(); it != m_GreatPersonWeights.end(); ++it)
	{
		int iValue = it->second;
		iValue = 100 * iValue / std::max(1, iMean);
		//iValue = (40000 + 500 * iValue) / (800 + iValue);
		iValue = (800*iMin + (9*100-8*iMin) * iValue) / (800 + iValue);
		it->second = iValue;
	}
}

int CvPlayerAI::AI_getGreatPersonWeight(UnitClassTypes eGreatPerson) const
{
	std::map<UnitClassTypes, int>::const_iterator it = m_GreatPersonWeights.find(eGreatPerson);
	if (it == m_GreatPersonWeights.end())
		return 100;
	else
		return it->second;
}
// K-Mod end

void CvPlayerAI::AI_nowHasTech(TechTypes eTech)
{
	// while its _possible_ to do checks, for financial trouble, and this tech adds financial buildings
	// if in war and this tech adds important war units
	// etc
	// it makes more sense to just redetermine what to produce
	// that is already done every time a civ meets a new civ, it makes sense to do it when a new tech is learned
	// if this is changed, then at a minimum, AI_isFinancialTrouble should be checked
	if (!isHuman())
	{
		int iGameTurn = GC.getGameINLINE().getGameTurn();
		
		// only reset at most every 10 turns
		if (iGameTurn > m_iTurnLastProductionDirty + 10)
		{
			// redeterimine the best things to build in each city
			AI_makeProductionDirty();
		
			m_iTurnLastProductionDirty = iGameTurn;
		}
	}

}


int CvPlayerAI::AI_countDeadlockedBonuses(CvPlot* pPlot) const
{
    CvPlot* pLoopPlot;
    CvPlot* pLoopPlot2;
    int iDX, iDY;
    int iI;
    
    int iMinRange = GC.getMIN_CITY_RANGE();
    int iRange = iMinRange * 2;
    int iCount = 0;

    for (iDX = -(iRange); iDX <= iRange; iDX++)
    {
        for (iDY = -(iRange); iDY <= iRange; iDY++)
        {
            if (plotDistance(iDX, iDY, 0, 0) > CITY_PLOTS_RADIUS)
            {
                pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

                if (pLoopPlot != NULL)
                {
                    if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
                    {
                        if (!pLoopPlot->isCityRadius() && ((pLoopPlot->area() == pPlot->area()) || pLoopPlot->isWater()))
                        {
                            bool bCanFound = false;
                            bool bNeverFound = true;
                            //potentially blockable resource
                            //look for a city site within a city radius
                            for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
                            {
                                pLoopPlot2 = plotCity(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), iI);
                                if (pLoopPlot2 != NULL)
                                {
                                    //canFound usually returns very quickly
                                    if (canFound(pLoopPlot2->getX_INLINE(), pLoopPlot2->getY_INLINE(), false))
                                    {
                                        bNeverFound = false;
                                        if (stepDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pLoopPlot2->getX_INLINE(), pLoopPlot2->getY_INLINE()) > iMinRange)
                                        {
                                            bCanFound = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (!bNeverFound && !bCanFound)
                            {
                                iCount++;
                            }
                        }
                    }
                }
            }
        }
    }
    
    return iCount;
}

// K-Mod. This function use to be the bulk of AI_goldToUpgradeAllUnits()
void CvPlayerAI::AI_updateGoldToUpgradeAllUnits()
{
	/*if (m_iUpgradeUnitsCacheTurn == GC.getGameINLINE().getGameTurn() && m_iUpgradeUnitsCachedExpThreshold == iExpThreshold)
	{
		return m_iUpgradeUnitsCachedGold;
	}*/

	int iTotalGold = 0;
	
	CvCivilizationInfo& kCivilizationInfo = GC.getCivilizationInfo(getCivilizationType());
	
	// cache the value for each unit type
	std::vector<int> aiUnitUpgradePrice(GC.getNumUnitInfos(), 0);	// initializes to zeros

	int iLoop;
	for (CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		/*
		// if experience is below threshold, skip this unit
		if (pLoopUnit->getExperience() < iExpThreshold)
		{
			continue;
		}*/
		
		UnitTypes eUnitType = pLoopUnit->getUnitType();

		// check cached value for this unit type
		int iCachedUnitGold = aiUnitUpgradePrice[eUnitType];
		if (iCachedUnitGold != 0)
		{
			// if positive, add it to the sum
			if (iCachedUnitGold > 0)
			{
				iTotalGold += iCachedUnitGold;
			}
			
			// either way, done with this unit
			continue;
		}
		
		int iUnitGold = 0;
		int iUnitUpgradePossibilities = 0;
		
		UnitAITypes eUnitAIType = pLoopUnit->AI_getUnitAIType();
		CvArea* pUnitArea = pLoopUnit->area();
		int iUnitValue = AI_unitValue(eUnitType, eUnitAIType, pUnitArea);

		for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
		{
			UnitClassTypes eUpgradeUnitClassType = (UnitClassTypes) iI;
			UnitTypes eUpgradeUnitType = (UnitTypes)(kCivilizationInfo.getCivilizationUnits(iI));

			if (NO_UNIT != eUpgradeUnitType)
			{
				// is it better?
				int iUpgradeValue = AI_unitValue(eUpgradeUnitType, eUnitAIType, pUnitArea);
				if (iUpgradeValue > iUnitValue)
				{
					// is this a valid upgrade?
					if (pLoopUnit->upgradeAvailable(eUnitType, eUpgradeUnitClassType))
					{
						// can we actually make this upgrade?
						bool bCanUpgrade = false;
						CvCity* pCapitalCity = getCapitalCity();
						if (pCapitalCity != NULL && pCapitalCity->canTrain(eUpgradeUnitType))
						{
							bCanUpgrade = true;
						}
						else
						{
							CvCity* pCloseCity = GC.getMapINLINE().findCity(pLoopUnit->getX_INLINE(), pLoopUnit->getY_INLINE(), getID(), NO_TEAM, true, (pLoopUnit->getDomainType() == DOMAIN_SEA));
							if (pCloseCity != NULL && pCloseCity->canTrain(eUpgradeUnitType))
							{
								bCanUpgrade = true;
							}
						}

						if (bCanUpgrade)
						{
							iUnitGold += pLoopUnit->upgradePrice(eUpgradeUnitType);
							iUnitUpgradePossibilities++; 
						}
					}
				}
			}
		}
		
		// if we found any, find average and add to total
		if (iUnitUpgradePossibilities > 0)
		{
			iUnitGold /= iUnitUpgradePossibilities;

			// add to cache
			aiUnitUpgradePrice[eUnitType] = iUnitGold;

			// add to sum
			iTotalGold += iUnitGold;
		}
		else
		{
			// add to cache, dont upgrade to this type
			aiUnitUpgradePrice[eUnitType] = -1;
		}
	}

	/*m_iUpgradeUnitsCacheTurn = GC.getGameINLINE().getGameTurn();
	m_iUpgradeUnitsCachedExpThreshold = iExpThreshold; */
	m_iUpgradeUnitsCachedGold = iTotalGold;

	//return iTotalGold;
}

// K-Mod. 'available income' is meant to roughly represent the amount of gold-per-turn the player would produce with 100% gold on the commerce slider.
// (without subtracting costs)
// In the original code, this value was essentially calculated by simply adding total gold to total science.
// This new version is better able to handle civs which are using culture or espionage on their commerce slider.
void CvPlayerAI::AI_updateAvailableIncome()
{
	int iTotalRaw = calculateTotalYield(YIELD_COMMERCE);

	m_iAvailableIncome = iTotalRaw * AI_averageCommerceMultiplier(COMMERCE_GOLD) / 100;
	m_iAvailableIncome += std::max(0, getGoldPerTurn());
	m_iAvailableIncome += getCommerceRate(COMMERCE_GOLD) - iTotalRaw * AI_averageCommerceMultiplier(COMMERCE_GOLD) * getCommercePercent(COMMERCE_GOLD) / 10000;
}

// Estimate what percentage of commerce is needed on gold to cover our expenses
int CvPlayerAI::AI_estimateBreakEvenGoldPercent() const
{
	PROFILE_FUNC();
	//int iGoldCommerceRate = kPlayer.getCommercePercent(COMMERCE_GOLD); // (rough approximation)

	// (detailed approximation)
	int iTotalRaw = calculateTotalYield(YIELD_COMMERCE);
	int iExpenses = calculateInflatedCosts() * 12 / 10; // (20% increase to approximate other costs)

	// estimate how much gold we get from specialists & buildings by taking our current gold rate and subtracting what it would be from raw commerce alone.
	iExpenses -= getCommerceRate(COMMERCE_GOLD) - iTotalRaw * AI_averageCommerceMultiplier(COMMERCE_GOLD) * getCommercePercent(COMMERCE_GOLD) / 10000;

	// divide what's left to determine what our gold slider would need to be to break even.
	int iGoldCommerceRate = 10000 * iExpenses / (iTotalRaw * AI_averageCommerceMultiplier(COMMERCE_GOLD)
			+ 1); // advc.001: Caused a division by 0 in the EarthAD1000 scenario

	iGoldCommerceRate = range(iGoldCommerceRate, 0, 100); // (perhaps it would be useful to just return the unbounded value?)

	return iGoldCommerceRate;
}
// K-Mod end

int CvPlayerAI::AI_goldTradeValuePercent() const
{
	int iValue = 2;
	// advc.036: Commented out
	/*if (AI_isFinancialTrouble())
	{
		iValue += 1;
	}*/
	return 100 * iValue;
	
}

int CvPlayerAI::AI_averageYieldMultiplier(YieldTypes eYield) const
{
	FAssert(eYield > -1);
	FAssert(eYield < NUM_YIELD_TYPES);
	
	/*if (m_iAveragesCacheTurn != GC.getGameINLINE().getGameTurn())
	{
		AI_calculateAverages();
	}*/
	
	FAssert(m_aiAverageYieldMultiplier[eYield] > 0);
	return m_aiAverageYieldMultiplier[eYield];
}

int CvPlayerAI::AI_averageCommerceMultiplier(CommerceTypes eCommerce) const
{
	FAssert(eCommerce > -1);
	FAssert(eCommerce < NUM_COMMERCE_TYPES);
	
	/*if (m_iAveragesCacheTurn != GC.getGameINLINE().getGameTurn())
	{
		AI_calculateAverages();
	}*/
	
	return m_aiAverageCommerceMultiplier[eCommerce];	
}

int CvPlayerAI::AI_averageGreatPeopleMultiplier() const
{
	/*if (m_iAveragesCacheTurn != GC.getGameINLINE().getGameTurn())
	{
		AI_calculateAverages();
	}*/
	return m_iAverageGreatPeopleMultiplier;	
}

// K-Mod
int CvPlayerAI::AI_averageCulturePressure() const
{
	/*if (m_iAveragesCacheTurn != GC.getGameINLINE().getGameTurn())
	{
		AI_calculateAverages();
	}*/
	return m_iAverageCulturePressure;	
}

//"100 eCommerce is worth (return) raw YIELD_COMMERCE
int CvPlayerAI::AI_averageCommerceExchange(CommerceTypes eCommerce) const
{
	FAssert(eCommerce > -1);
	FAssert(eCommerce < NUM_COMMERCE_TYPES);
	
	/*if (m_iAveragesCacheTurn != GC.getGameINLINE().getGameTurn())
	{
		AI_calculateAverages();
	}*/
	
	return m_aiAverageCommerceExchange[eCommerce];
}

void CvPlayerAI::AI_calculateAverages()
{
	CvCity* pLoopCity;
	int iLoop;

	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		m_aiAverageYieldMultiplier[iI] = 0;		
	}
	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		m_aiAverageCommerceMultiplier[iI] = 0;
	}
	m_iAverageGreatPeopleMultiplier = 0;

	int iTotalPopulation = 0;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		int iPopulation = std::max(pLoopCity->getPopulation(), NUM_CITY_PLOTS);
		iTotalPopulation += iPopulation;
			
		for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			m_aiAverageYieldMultiplier[iI] += iPopulation * pLoopCity->AI_yieldMultiplier((YieldTypes)iI);
		}
		for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
		{
			m_aiAverageCommerceMultiplier[iI] += iPopulation * pLoopCity->getTotalCommerceRateModifier((CommerceTypes)iI);
		}
		m_iAverageGreatPeopleMultiplier += iPopulation * pLoopCity->getTotalGreatPeopleRateModifier();
	}


	if (iTotalPopulation > 0)
	{
		for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			m_aiAverageYieldMultiplier[iI] /= iTotalPopulation;
			FAssert(m_aiAverageYieldMultiplier[iI] > 0);
		}
		for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
		{
			m_aiAverageCommerceMultiplier[iI] /= iTotalPopulation;
			FAssert(m_aiAverageCommerceMultiplier[iI] > 0);	
		}
		m_iAverageGreatPeopleMultiplier /= iTotalPopulation;
		FAssert(m_iAverageGreatPeopleMultiplier > 0);
	}
	else
	{
		for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			m_aiAverageYieldMultiplier[iI] = 100;
		}
		for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
		{
			m_aiAverageCommerceMultiplier[iI] = 100;
		}
		m_iAverageGreatPeopleMultiplier = 100;
	}


	//Calculate Exchange Rate

	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		m_aiAverageCommerceExchange[iI] = 0;		
	}

	int iCommerce = 0;
	int iTotalCommerce = 0;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iCommerce = pLoopCity->getYieldRate(YIELD_COMMERCE);
		iTotalCommerce += iCommerce;
		
		int iExtraCommerce = 0;
		for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
		{
			iExtraCommerce +=((pLoopCity->getSpecialistPopulation() + pLoopCity->getNumGreatPeople()) * getSpecialistExtraCommerce((CommerceTypes)iI));
			iExtraCommerce += (pLoopCity->getBuildingCommerce((CommerceTypes)iI) + pLoopCity->getSpecialistCommerce((CommerceTypes)iI) + pLoopCity->getReligionCommerce((CommerceTypes)iI) + getFreeCityCommerce((CommerceTypes)iI));
		}
		iTotalCommerce += iExtraCommerce;
		
		for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
		{
			m_aiAverageCommerceExchange[iI] += ((iCommerce + iExtraCommerce) * pLoopCity->getTotalCommerceRateModifier((CommerceTypes)iI)) / 100;		
		}
	}

	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		if (m_aiAverageCommerceExchange[iI] > 0)
		{
			m_aiAverageCommerceExchange[iI] = (100 * iTotalCommerce) / m_aiAverageCommerceExchange[iI];
		}
		else
		{
			m_aiAverageCommerceExchange[iI] = 100;
		}
	}

	// K-Mod Culture pressure

	int iTotal = 0;
	int iWeightedTotal = 0;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		int iCultureRate = pLoopCity->getCommerceRateTimes100(COMMERCE_CULTURE);
		iTotal += iCultureRate;
		iWeightedTotal += iCultureRate * pLoopCity->AI_culturePressureFactor();
	}
	if (iTotal == 0)
		m_iAverageCulturePressure = 100;
	else
		m_iAverageCulturePressure = iWeightedTotal / iTotal;

	//m_iAveragesCacheTurn = GC.getGameINLINE().getGameTurn();
}

// K-Mod edition
void CvPlayerAI::AI_convertUnitAITypesForCrush()
{
	int iLoop;

	std::map<int, int> spare_units;
	std::vector<std::pair<int, int> > unit_list; // { score, unitID }.
	// note unitID is used rather than CvUnit* to ensure that the list gives the same order for players on different computers.

	for (CvArea *pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		// Keep 1/2 of recommended floating defenders.
		if (!pLoopArea || pLoopArea->getAreaAIType(getTeam()) == AREAAI_ASSAULT
			|| pLoopArea->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE)
		{
			spare_units[pLoopArea->getID()] = 0;
		}
		else
		{
			spare_units[pLoopArea->getID()] = (2 * AI_getTotalFloatingDefenders(pLoopArea) - AI_getTotalFloatingDefendersNeeded(pLoopArea))/2;
		}
	}
	
	for (CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		bool bValid = false;
		const CvPlot* pPlot = pLoopUnit->plot();

		/* if (pLoopUnit->AI_getUnitAIType() == UNITAI_RESERVE
			|| pLoopUnit->AI_isCityAIType() && (pLoopUnit->getExtraCityDefensePercent() <= 0)) */
		// K-Mod, protective leaders might still want to use their gunpowder units...
		if (pLoopUnit->AI_getUnitAIType() == UNITAI_RESERVE || pLoopUnit->AI_getUnitAIType() == UNITAI_COLLATERAL
			|| (pLoopUnit->AI_isCityAIType() && pLoopUnit->getExtraCityDefensePercent()
					// cdtw:
					+ pLoopUnit->getUnitInfo().getCityDefenseModifier() / 2
					<= 30))
		{
			bValid = true;
		}

		/*if ((pLoopUnit->area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT)
			|| (pLoopUnit->area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE))
		{
			bValid = false;
		}*/
		
		if (!pLoopUnit->canAttack() || (pLoopUnit->AI_getUnitAIType() == UNITAI_CITY_SPECIAL))
		{
			bValid = false;
		}

		if (spare_units[pLoopUnit->area()->getID()] <= 0)
			bValid = false;
	
		if (bValid)
		{
			if (pPlot->isCity())
			{
				if (pPlot->getPlotCity()->getOwner() == getID())
				{
					if (pPlot->getBestDefender(getID()) == pLoopUnit)
					{
						bValid = false;
					}
				}
				/*if (pLoopUnit->AI_getUnitAIType() == UNITAI_CITY_DEFENSE && GET_TEAM(getTeam()).getAtWarCount(true) > 0)
				{
					int iCityDefenders = pLoopUnit->plot()->plotCount(PUF_isUnitAIType, UNITAI_CITY_DEFENSE, -1, getID());

					if (iCityDefenders <= pLoopUnit->plot()->getPlotCity()->AI_minDefenders())
						bValid = false;
				}*/
				if (pLoopUnit->getGroup()->AI_getMissionAIType() == MISSIONAI_GUARD_CITY)
				{
					int iDefendersHere = pPlot->plotCount(PUF_isMissionAIType, MISSIONAI_GUARD_CITY, -1, getID());
					int iDefendersWant = pLoopUnit->area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE
						? pPlot->getPlotCity()->AI_neededDefenders()
						: pPlot->getPlotCity()->AI_minDefenders();

					if (iDefendersHere <= iDefendersWant)
						bValid = false;
				}
			}
		}
		
		if (bValid)
		{
			int iValue = AI_unitValue(pLoopUnit->getUnitType(), UNITAI_ATTACK_CITY, pLoopUnit->area());
			unit_list.push_back(std::make_pair(iValue, pLoopUnit->getID()));
		}
	}

	// convert the highest scoring units first.
	std::sort(unit_list.begin(), unit_list.end(), std::greater<std::pair<int, int> >());
	std::vector<std::pair<int, int> >::iterator it;
	for (it = unit_list.begin(); it != unit_list.end(); ++it)
	{
		if (it->first > 0 && spare_units[getUnit(it->second)->area()->getID()] > 0)
		{
			if (gPlayerLogLevel >= 2)
			{
				CvWString sOldType;
				getUnitAIString(sOldType, getUnit(it->second)->AI_getUnitAIType());
				logBBAI("    %S converts %S from %S to attack city for crush. (%d)", getName(), getUnit(it->second)->getName().GetCString(), sOldType.GetCString(), getUnit(it->second)->getID());
			}

			getUnit(it->second)->AI_setUnitAIType(UNITAI_ATTACK_CITY);
			// only convert half of our spare units, so that we can reevaluate which units we need before converting more.
			spare_units[getUnit(it->second)->area()->getID()]-=2;
		}
	}
}

int CvPlayerAI::AI_playerCloseness(PlayerTypes eIndex, int iMaxDistance) const
{
	PROFILE_FUNC();	
	FAssert(GET_PLAYER(eIndex).isAlive());
	FAssert(eIndex != getID());
	
	int iValue = 0;
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iValue += pLoopCity->AI_playerCloseness(eIndex, iMaxDistance);
	}
	
	return iValue;
}


int CvPlayerAI::AI_getTotalAreaCityThreat(CvArea* pArea) const
{
	PROFILE_FUNC();
	CvCity* pLoopCity;
	int iLoop;
	int iValue;
	
	iValue = 0;
	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (pLoopCity->getArea() == pArea->getID())
		{
			iValue += pLoopCity->AI_cityThreat();
		}
	}
	return iValue;
}

int CvPlayerAI::AI_countNumAreaHostileUnits(CvArea* pArea, bool bPlayer, bool bTeam, bool bNeutral, bool bHostile) const
{
	PROFILE_FUNC();
	CvPlot* pLoopPlot;
	int iCount;
	int iI;

	iCount = 0;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
		if ((pLoopPlot->area() == pArea) && pLoopPlot->isVisible(getTeam(), false) && 
			((bPlayer && pLoopPlot->getOwnerINLINE() == getID()) || (bTeam && pLoopPlot->getTeam() == getTeam()) 
				|| (bNeutral && !pLoopPlot->isOwned()) || (bHostile && pLoopPlot->isOwned() && GET_TEAM(getTeam()).isAtWar(pLoopPlot->getTeam()))))
			{
			iCount += pLoopPlot->plotCount(PUF_isEnemy, getID(), false, NO_PLAYER, NO_TEAM, PUF_isVisible, getID());
		}
	}
	return iCount;
}

//this doesn't include the minimal one or two garrison units in each city.
int CvPlayerAI::AI_getTotalFloatingDefendersNeeded(CvArea* pArea) const
{
	PROFILE_FUNC();
	int iDefenders;
	int iCurrentEra = getCurrentEra();
	int iAreaCities = pArea->getCitiesPerPlayer(getID());
	
	iCurrentEra = std::max(0, iCurrentEra - GC.getGame().getStartEra() / 2);
	
	/* original bts code
	iDefenders = 1 + ((iCurrentEra + ((GC.getGameINLINE().getMaxCityElimination() > 0) ? 3 : 2)) * iAreaCities);
	iDefenders /= 3;
	iDefenders += pArea->getPopulationPerPlayer(getID()) / 7; */
	// K-Mod
	iDefenders = 1 + iAreaCities + AI_totalAreaUnitAIs(pArea, UNITAI_SETTLE);
	iDefenders += pArea->getPopulationPerPlayer(getID()) / 7;
	if (AI_isLandWar(pArea))
		iDefenders += 1 + (2+GET_TEAM(getTeam()).countEnemyCitiesByArea(pArea))/3;
	iDefenders *= iCurrentEra + (GC.getGameINLINE().getMaxCityElimination() > 0 ? 3 : 2);
	iDefenders /= 3;
	// K-Mod end

	if (pArea->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE)
	{
		// <advc.107>
		//iDefenders *= 2;
		iDefenders *= 3;
		iDefenders /= 2;
		// </advc.107>
	}
	else 
	{
		if( AI_isDoStrategy(AI_STRATEGY_ALERT2) )
		{
			iDefenders *= 2;
		}
		else if( AI_isDoStrategy(AI_STRATEGY_ALERT1) )
		{
			iDefenders *= 3;
			iDefenders /= 2;
		}
		// advc.022: "else" removed; allow AreaAI to cancel out alertness
		//else
		if (pArea->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE)
		{
			iDefenders *= 2;
			iDefenders /= 3;
		}
		else if (pArea->getAreaAIType(getTeam()) == AREAAI_MASSING)
		{
			if( GET_TEAM(getTeam()).AI_getEnemyPowerPercent(true) < (10 + GC.getLeaderHeadInfo(getPersonalityType()).getMaxWarNearbyPowerRatio()) ) // bbai
			{
				iDefenders *= 2;
				iDefenders /= 3;
			}
		}
	}
	
	if (AI_getTotalAreaCityThreat(pArea) == 0)
	{
		iDefenders /= 2;
	}
	// advc.107: Don't want even more rounding artifacts
	double mod = 1;
	if (!GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI))
	{
		mod *= 0.67; // advc.107: Replacing the two lines below
		/*iDefenders *= 2;
		iDefenders /= 3;*/
	} /* <advc.107> Fewer defenders on low difficulty, more on high difficulty.
		 Times 0.9 b/c I don't actually want more defenders on moderately high
		 difficulty. */
	mod /= ::dRange(trainingModifierFromHandicap() * 0.9, 0.7, 1.5);
	iDefenders = ::round(mod * iDefenders); // </advc.107>
	// BBAI: Removed AI_STRATEGY_GET_BETTER_UNITS reduction, it was reducing defenses twice
	
	if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3))
	{
		iDefenders += 2 * iAreaCities;
		//if (pArea->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE)
		if (pArea->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE && AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4)) // K-Mod
		{
			iDefenders *= 2; //go crazy
		}
	}
	
	iDefenders *= 60;
	iDefenders /= std::max(30, (GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAITrainPercent() - 20));
	
    // <advc.107> Replacing code below (extra defenses vs. Raging Barbarians)
    if(GC.getGame().isOption(GAMEOPTION_RAGING_BARBARIANS)
            && !GC.getEraInfo(getCurrentEra()).isNoBarbUnits()) {
        int startEra = GC.getGameINLINE().getStartEra();
        if(iCurrentEra <= 1 + startEra)
           iDefenders += 1 + startEra;
    }
    /*if ((iCurrentEra < 3) && (GC.getGameINLINE().isOption(GAMEOPTION_RAGING_BARBARIANS)))
    {
        iDefenders += 2;
    }*/ // </advc.107>

	if (getCapitalCity() != NULL)
	{
		if (getCapitalCity()->area() != pArea)
		{

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       01/23/09                                jdog5000      */
/*                                                                                              */
/* Bugfix, War tactics AI                                                                       */
/************************************************************************************************/
/* original BTS code
			//Defend offshore islands only lightly.
			iDefenders = std::min(iDefenders, iAreaCities * iAreaCities - 1);
*/
			// Lessen defensive requirements only if not being attacked locally
			if( pArea->getAreaAIType(getTeam()) != AREAAI_DEFENSIVE )
			{
				// This may be our first city captured on a large enemy continent, need defenses to scale up based
				// on total number of area cities not just ours
				iDefenders = std::min(iDefenders, iAreaCities * iAreaCities + pArea->getNumCities() - iAreaCities - 1);
			}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
		}
	}
	
	return iDefenders;
}

int CvPlayerAI::AI_getTotalFloatingDefenders(CvArea* pArea) const
{
	PROFILE_FUNC();
	int iCount = 0;

	iCount += AI_totalAreaUnitAIs(pArea, UNITAI_COLLATERAL);
	iCount += AI_totalAreaUnitAIs(pArea, UNITAI_RESERVE);
	iCount += std::max(0, (AI_totalAreaUnitAIs(pArea, UNITAI_CITY_DEFENSE) - (pArea->getCitiesPerPlayer(getID()) * 2)));
	iCount += AI_totalAreaUnitAIs(pArea, UNITAI_CITY_COUNTER);
	iCount += AI_totalAreaUnitAIs(pArea, UNITAI_CITY_SPECIAL);
	return iCount;
}

// K-Mod. (very basic just as a starting point. I'll refine this later.)
int CvPlayerAI::AI_getTotalAirDefendersNeeded() const
{
	int iNeeded = getNumCities() + 1;

	//iNeeded = iNeeded + iNeeded*(getCurrentEra()+1) / std::max(1, GC.getNumEraInfos()*2);

	// Todo. Adjust based on what other civs are doing.

	if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4 | AI_VICTORY_SPACE4))
		iNeeded = iNeeded*3/2;
	if (AI_isDoStrategy(AI_STRATEGY_ALERT2))
		iNeeded = iNeeded*3/2;

	return iNeeded;
}
// K-Mod end

RouteTypes CvPlayerAI::AI_bestAdvancedStartRoute(CvPlot* pPlot, int* piYieldValue) const
{
	RouteTypes eBestRoute = NO_ROUTE;
	int iBestValue = -1;
    for (int iI = 0; iI < GC.getNumRouteInfos(); iI++)
    {
        RouteTypes eRoute = (RouteTypes)iI;

		int iValue = 0;
		int iCost = getAdvancedStartRouteCost(eRoute, true, pPlot);
		
		if (iCost >= 0)
		{
			iValue += GC.getRouteInfo(eRoute).getValue();
									
			if (iValue > 0)
			{
				int iYieldValue = 0;
				if (pPlot->getImprovementType() != NO_IMPROVEMENT)
				{
					iYieldValue += ((GC.getImprovementInfo(pPlot->getImprovementType()).getRouteYieldChanges(eRoute, YIELD_FOOD)) * 100);
					iYieldValue += ((GC.getImprovementInfo(pPlot->getImprovementType()).getRouteYieldChanges(eRoute, YIELD_PRODUCTION)) * 60);
					iYieldValue += ((GC.getImprovementInfo(pPlot->getImprovementType()).getRouteYieldChanges(eRoute, YIELD_COMMERCE)) * 40);
				}
				iValue *= 1000;
				iValue /= (1 + iCost);

				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					eBestRoute = eRoute;
					if (piYieldValue != NULL)
					{
						*piYieldValue = iYieldValue;
					}
				}
			}
		}
	}
	return eBestRoute;
}

UnitTypes CvPlayerAI::AI_bestAdvancedStartUnitAI(CvPlot* pPlot, UnitAITypes eUnitAI) const
{
	UnitTypes eLoopUnit;
	UnitTypes eBestUnit;
	int iValue;
	int iBestValue;
	int iI, iJ, iK;
	
	FAssertMsg(eUnitAI != NO_UNITAI, "UnitAI is not assigned a valid value");

	iBestValue = 0;
	eBestUnit = NO_UNIT;

	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

		if (eLoopUnit != NO_UNIT)
		{
			//if (!isHuman() || (GC.getUnitInfo(eLoopUnit).getDefaultUnitAIType() == eUnitAI))
			{
				int iUnitCost = getAdvancedStartUnitCost(eLoopUnit, true, pPlot);
				if (iUnitCost >= 0)
				{
					iValue = AI_unitValue(eLoopUnit, eUnitAI, pPlot->area());

					if (iValue > 0)
					{
						//free promotions. slow?
						//only 1 promotion per source is counted (ie protective isn't counted twice)
						int iPromotionValue = 0;

						//special to the unit
						for (iJ = 0; iJ < GC.getNumPromotionInfos(); iJ++)
						{
							if (GC.getUnitInfo(eLoopUnit).getFreePromotions(iJ))
							{
								iPromotionValue += 15;
								break;
							}
						}

						for (iK = 0; iK < GC.getNumPromotionInfos(); iK++)
						{
							if (isFreePromotion((UnitCombatTypes)GC.getUnitInfo(eLoopUnit).getUnitCombatType(), (PromotionTypes)iK))
							{
								iPromotionValue += 15;
								break;
							}

							if (isFreePromotion((UnitClassTypes)GC.getUnitInfo(eLoopUnit).getUnitClassType(), (PromotionTypes)iK))
							{
								iPromotionValue += 15;
								break;
							}
						}

						//traits
						for (iJ = 0; iJ < GC.getNumTraitInfos(); iJ++)
						{
							if (hasTrait((TraitTypes)iJ))
							{
								for (iK = 0; iK < GC.getNumPromotionInfos(); iK++)
								{
									if (GC.getTraitInfo((TraitTypes) iJ).isFreePromotion(iK))
									{
										if ((GC.getUnitInfo(eLoopUnit).getUnitCombatType() != NO_UNITCOMBAT) && GC.getTraitInfo((TraitTypes) iJ).isFreePromotionUnitCombat(GC.getUnitInfo(eLoopUnit).getUnitCombatType()))
										{
											iPromotionValue += 15;
											break;
										}
									}
								}
							}
						}

						iValue *= (iPromotionValue + 100);
						iValue /= 100;

						iValue *= (GC.getGameINLINE().getSorenRandNum(40, "AI Best Advanced Start Unit") + 100);
						iValue /= 100;

						iValue *= (getNumCities() + 2);
						iValue /= (getUnitClassCountPlusMaking((UnitClassTypes)iI) + getNumCities() + 2);

						FAssert((MAX_INT / 1000) > iValue);
						iValue *= 1000;
						
						iValue /= 1 + iUnitCost;

						iValue = std::max(1, iValue);

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							eBestUnit = eLoopUnit;
						}
					}
				}
			}
		}
	}

	return eBestUnit;
}

CvPlot* CvPlayerAI::AI_advancedStartFindCapitalPlot()
{
	CvPlot* pBestPlot = NULL;
	int iBestValue = -1;
	
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kPlayer.isAlive())
		{
			if (kPlayer.getTeam() == getTeam())
			{
				CvPlot* pLoopPlot = kPlayer.getStartingPlot();
				if (pLoopPlot != NULL)
				{
					if (getAdvancedStartCityCost(true, pLoopPlot) > 0)
					{
					int iX = pLoopPlot->getX_INLINE();
					int iY = pLoopPlot->getY_INLINE();
						
						int iValue = 1000;
						if (iPlayer == getID())
						{
							iValue += 1000;
						}
						else
						{
							iValue += GC.getGame().getSorenRandNum(100, "AI Advanced Start Choose Team Start");
						}
						CvCity * pNearestCity = GC.getMapINLINE().findCity(iX, iY, NO_PLAYER, getTeam());
						if (NULL != pNearestCity)
						{
							FAssert(pNearestCity->getTeam() == getTeam());
							int iDistance = stepDistance(iX, iY, pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE());
							if (iDistance < 10)
							{
								iValue /= (10 - iDistance);
							}
						}
						
						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopPlot;							
						}
					}
				}
				else
				{
					FAssertMsg(false, "StartingPlot for a live player is NULL!");
				}
			}
		}
	}
	
	if (pBestPlot != NULL)
	{
		return pBestPlot;
	}
	
	FAssertMsg(false, "AS: Failed to find a starting plot for a player");
	
	//Execution should almost never reach here.
	
	//Update found values just in case - particulary important for simultaneous turns.
	AI_updateFoundValues();
	
	pBestPlot = NULL;
	iBestValue = -1;
	
	if (NULL != getStartingPlot())
	{
		for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
		{
			CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
			if (pLoopPlot->getArea() == getStartingPlot()->getArea())
			{
				int iValue = pLoopPlot->getFoundValue(getID());
				if (iValue > 0)
				{
					if (getAdvancedStartCityCost(true, pLoopPlot) > 0)
					{
						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopPlot;
						}
					}
				}
			}
		}
	}
	
	if (pBestPlot != NULL)
	{
		return pBestPlot;
	}
	
	//Commence panic.
	FAssertMsg(false, "Failed to find an advanced start starting plot");
	return NULL;
}


bool CvPlayerAI::AI_advancedStartPlaceExploreUnits(bool bLand)
{
	CvPlot* pBestExplorePlot = NULL;
	int iBestExploreValue = 0;
	UnitTypes eBestUnitType = NO_UNIT;
	
	UnitAITypes eUnitAI = NO_UNITAI;
	if (bLand)
	{
		eUnitAI = UNITAI_EXPLORE;
	}
	else
	{
		eUnitAI = UNITAI_EXPLORE_SEA;
	}
	
	int iLoop;
	CvCity* pLoopCity;
	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		CvPlot* pLoopPlot = pLoopCity->plot();
		CvArea* pLoopArea = bLand ? pLoopCity->area() : pLoopPlot->waterArea();
		
		if (pLoopArea != NULL)
			{
			int iValue = std::max(0, pLoopArea->getNumUnrevealedTiles(getTeam()) - 10) * 10;
			iValue += std::max(0, pLoopArea->getNumTiles() - 50);
				
				if (iValue > 0)
				{
					int iOtherPlotCount = 0;
					int iGoodyCount = 0;
					int iExplorerCount = 0;
				int iAreaId = pLoopArea->getID();
					
				int iRange = 4;
					for (int iX = -iRange; iX <= iRange; iX++)
					{
						for (int iY = -iRange; iY <= iRange; iY++)
						{
							CvPlot* pLoopPlot2 = plotXY(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), iX, iY);
						if (NULL != pLoopPlot2) 
							{
								iExplorerCount += pLoopPlot2->plotCount(PUF_isUnitAIType, eUnitAI, -1, NO_PLAYER, getTeam());
							if (pLoopPlot2->getArea() == iAreaId)
							{
								if (pLoopPlot2->isGoody())
								{
									iGoodyCount++;
								}
								if (pLoopPlot2->getTeam() != getTeam())
								{
									iOtherPlotCount++;
								}
							}
						}
					}
				}
					
					iValue -= 300 * iExplorerCount;
					iValue += 200 * iGoodyCount;
					iValue += 10 * iOtherPlotCount;
					if (iValue > iBestExploreValue)
					{
						UnitTypes eUnit = AI_bestAdvancedStartUnitAI(pLoopPlot, eUnitAI);
						if (eUnit != NO_UNIT)
						{
							eBestUnitType = eUnit;
							iBestExploreValue = iValue;
							pBestExplorePlot = pLoopPlot;
						}
					}
				}
			}
		}
	
	if (pBestExplorePlot != NULL)
	{
		doAdvancedStartAction(ADVANCEDSTARTACTION_UNIT, pBestExplorePlot->getX_INLINE(), pBestExplorePlot->getY_INLINE(), eBestUnitType, true);
		return true;
	}
	return false;	
}

void CvPlayerAI::AI_advancedStartRevealRadius(CvPlot* pPlot, int iRadius)
{
	for (int iRange = 1; iRange <=iRadius; iRange++)
	{
		for (int iX = -iRange; iX <= iRange; iX++)
		{
			for (int iY = -iRange; iY <= iRange; iY++)
			{
				if (plotDistance(0, 0, iX, iY) <= iRadius)
				{
					CvPlot* pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iX, iY);

					if (NULL != pLoopPlot)
					{
						if (getAdvancedStartVisibilityCost(true, pLoopPlot) > 0)
						{
							doAdvancedStartAction(ADVANCEDSTARTACTION_VISIBILITY, pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), -1, true);
						}
					}
				}
			}
		}
	}
}

bool CvPlayerAI::AI_advancedStartPlaceCity(CvPlot* pPlot)
{
	//If there is already a city, then improve it.
	CvCity* pCity = pPlot->getPlotCity();
	if (pCity == NULL)
	{
		doAdvancedStartAction(ADVANCEDSTARTACTION_CITY, pPlot->getX(), pPlot->getY(), -1, true);

		pCity = pPlot->getPlotCity();
		if ((pCity == NULL) || (pCity->getOwnerINLINE() != getID()))
		{
			//this should never happen since the cost for a city should be 0 if
			//the city can't be placed.
			//(It can happen if another player has placed a city in the fog)
			FAssertMsg(false, "ADVANCEDSTARTACTION_CITY failed in unexpected way");
			return false;
		}
	}
	
	if (pCity->getCultureLevel() <= 1)
	{
		doAdvancedStartAction(ADVANCEDSTARTACTION_CULTURE, pPlot->getX(), pPlot->getY(), -1, true);
	}
	
	//to account for culture expansion.
	pCity->AI_updateBestBuild();
	
	int iPlotsImproved = 0;
	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		if (iI != CITY_HOME_PLOT)
		{
			CvPlot* pLoopPlot = plotCity(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iI);
			if ((pLoopPlot != NULL) && (pLoopPlot->getWorkingCity() == pCity))
			{
				if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT)
				{
					iPlotsImproved++;
				}
			}
		}
	}

	int iTargetPopulation = pCity->happyLevel() + (getCurrentEra() / 2);
	
	while (iPlotsImproved < iTargetPopulation)
	{
		CvPlot* pBestPlot;
		ImprovementTypes eBestImprovement = NO_IMPROVEMENT;
		int iBestValue = 0;
		for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			int iValue = pCity->AI_getBestBuildValue(iI);
			if (iValue > iBestValue)
			{
				BuildTypes eBuild = pCity->AI_getBestBuild(iI);
				if (eBuild != NO_BUILD)
				{
					ImprovementTypes eImprovement = (ImprovementTypes)GC.getBuildInfo(eBuild).getImprovement();
					if (eImprovement != NO_IMPROVEMENT)
					{
						CvPlot* pLoopPlot = plotCity(pCity->getX_INLINE(), pCity->getY_INLINE(), iI);
						if ((pLoopPlot != NULL) && (pLoopPlot->getImprovementType() != eImprovement))
						{
							eBestImprovement = eImprovement;
							pBestPlot = pLoopPlot;
							iBestValue = iValue;
						}
					}
				}
			}
		}
		
		if (iBestValue > 0)
		{
			
			FAssert(pBestPlot != NULL);
			doAdvancedStartAction(ADVANCEDSTARTACTION_IMPROVEMENT, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), eBestImprovement, true);
			iPlotsImproved++;
			if (pCity->getPopulation() < iPlotsImproved)
			{
				doAdvancedStartAction(ADVANCEDSTARTACTION_POP, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), -1, true);
			}
		}
		else
		{
			break;
		}
	}
		

	while (iPlotsImproved > pCity->getPopulation())
	{
		int iPopCost = getAdvancedStartPopCost(true, pCity);
		if (iPopCost <= 0 || iPopCost > getAdvancedStartPoints())
		{
			break;
		}
		if (pCity->healthRate() < 0)
		{
			break;
		}
		doAdvancedStartAction(ADVANCEDSTARTACTION_POP, pPlot->getX_INLINE(), pPlot->getY_INLINE(), -1, true);
	}
	
	pCity->AI_updateAssignWork();

	return true;
}




//Returns false if we have no more points.
bool CvPlayerAI::AI_advancedStartDoRoute(CvPlot* pFromPlot, CvPlot* pToPlot)
{
	FAssert(pFromPlot != NULL);
	FAssert(pToPlot != NULL);
	
	FAStarNode* pNode;
	gDLL->getFAStarIFace()->ForceReset(&GC.getStepFinder());
	if (gDLL->getFAStarIFace()->GeneratePath(&GC.getStepFinder(), pFromPlot->getX_INLINE(), pFromPlot->getY_INLINE(), pToPlot->getX_INLINE(), pToPlot->getY_INLINE(), false, 0, true))
	{
		pNode = gDLL->getFAStarIFace()->GetLastNode(&GC.getStepFinder());
		if (pNode != NULL)
		{
			if (pNode->m_iData1 > (1 + stepDistance(pFromPlot->getX(), pFromPlot->getY(), pToPlot->getX(), pToPlot->getY())))
			{
				//Don't build convulted paths.
				return true;
			}
		}

		while (pNode != NULL)
		{
			CvPlot* pPlot = GC.getMapINLINE().plotSorenINLINE(pNode->m_iX, pNode->m_iY);
			RouteTypes eRoute = AI_bestAdvancedStartRoute(pPlot);
			if (eRoute != NO_ROUTE)
			{
				if (getAdvancedStartRouteCost(eRoute, true, pPlot) > getAdvancedStartPoints())
				{
					return false;
				}
				doAdvancedStartAction(ADVANCEDSTARTACTION_ROUTE, pNode->m_iX, pNode->m_iY, eRoute, true);
			}
			pNode = pNode->m_pParent;			
		}
	}
	return true;
}
void CvPlayerAI::AI_advancedStartRouteTerritory()
{
//	//This uses a heuristic to create a road network
//	//which is at least effecient if not all inclusive
//	//Basically a human will place roads where they connect
//	//the maximum number of trade groups and this
//	//mimics that.
//	
//	
//	CvPlot* pLoopPlot;
//	CvPlot* pLoopPlot2;
//	int iI, iJ;
//	int iPass;
//	
//
//	std::vector<int> aiPlotGroups;
//	for (iPass = 4; iPass > 1; --iPass)
//	{
//		for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
//		{
//			pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
//			if ((pLoopPlot != NULL) && (pLoopPlot->getOwner() == getID()) && (pLoopPlot->getRouteType() == NO_ROUTE))
//			{
//				aiPlotGroups.clear();
//				if (pLoopPlot->getPlotGroup(getID()) != NULL)
//				{
//					aiPlotGroups.push_back(pLoopPlot->getPlotGroup(getID())->getID());
//				}
//				for (iJ = 0; iJ < NUM_DIRECTION_TYPES; iJ++)
//				{
//					pLoopPlot2 = plotDirection(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), (DirectionTypes)iJ);
//					if ((pLoopPlot2 != NULL) && (pLoopPlot2->getRouteType() != NO_ROUTE))
//					{
//						CvPlotGroup* pPlotGroup = pLoopPlot2->getPlotGroup(getID());
//						if (pPlotGroup != NULL)
//						{
//							if (std::find(aiPlotGroups.begin(),aiPlotGroups.end(), pPlotGroup->getID()) == aiPlotGroups.end())
//							{
//								aiPlotGroups.push_back(pPlotGroup->getID());
//							}
//						}
//					}
//				}
//				if ((int)aiPlotGroups.size() >= iPass)
//				{
//					RouteTypes eBestRoute = AI_bestAdvancedStartRoute(pLoopPlot);
//					if (eBestRoute != NO_ROUTE)
//					{
//						doAdvancedStartAction(ADVANCEDSTARTACTION_ROUTE, pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), eBestRoute, true);
//					}
//				}
//			}
//		}
//	}
//	
//	//Maybe try to build road network for mobility but bearing in mind
//	//that routes can't be built outside culture atm. I think workers
//	//can do that just fine.

	CvPlot* pLoopPlot;
	int iI;
	
	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
		if ((pLoopPlot != NULL) && (pLoopPlot->getOwner() == getID()) && (pLoopPlot->getRouteType() == NO_ROUTE))
		{
			if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT)
			{
				BonusTypes eBonus = pLoopPlot->getBonusType(getTeam());
				if (eBonus != NO_BONUS)
				{
					//if (GC.getImprovementInfo(pLoopPlot->getImprovementType()).isImprovementBonusTrade(eBonus))
					if (doesImprovementConnectBonus(pLoopPlot->getImprovementType(), eBonus))
					{
						int iBonusValue = AI_bonusVal(eBonus, 1, true);
						if (iBonusValue > 9)
						{
							int iBestValue = 0;
							CvPlot* pBestPlot = NULL;
							int iRange = 2;
							for (int iX = -iRange; iX <= iRange; iX++)
							{
								for (int iY = -iRange; iY <= iRange; iY++)
								{
									CvPlot* pLoopPlot2 = plotXY(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), iX, iY);
									if (pLoopPlot2 != NULL)
									{
										if (pLoopPlot2->getOwner() == getID())
										{
											if ((pLoopPlot2->isConnectedToCapital()) || pLoopPlot2->isCity())
											{
												int iValue = 1000;
												if (pLoopPlot2->isCity())
												{
													iValue += 100;
													if (pLoopPlot2->getPlotCity()->isCapital())
													{
														iValue += 100;
													}
												}
												if (pLoopPlot2->isRoute())
												{
													iValue += 100;
												}
												int iDistance = GC.getMapINLINE().calculatePathDistance(pLoopPlot, pLoopPlot2);
												if (iDistance > 0)
												{
													iValue /= (1 + iDistance);
													
													if (iValue > iBestValue)
													{
														iBestValue = iValue;
														pBestPlot = pLoopPlot2;
													}
												}
											}
										}
									}
								}
							}
							if (pBestPlot != NULL)
							{
								if (!AI_advancedStartDoRoute(pLoopPlot, pBestPlot))
								{
									return;
								}
							}
						}
					}
				}
				if (pLoopPlot->getRouteType() == NO_ROUTE)
				{
					int iRouteYieldValue = 0;
					RouteTypes eRoute = (AI_bestAdvancedStartRoute(pLoopPlot, &iRouteYieldValue));
					if (eRoute != NO_ROUTE && iRouteYieldValue > 0)
					{
						doAdvancedStartAction(ADVANCEDSTARTACTION_ROUTE, pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), eRoute, true);
					}
				}
			}
		}
	}
	
	//Connect Cities
	int iLoop;
	CvCity* pLoopCity;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (!pLoopCity->isCapital() && !pLoopCity->isConnectedToCapital())
		{
			int iBestValue = 0;
			CvPlot* pBestPlot = NULL;
			int iRange = 5;
			for (int iX = -iRange; iX <= iRange; iX++)
			{
				for (int iY = -iRange; iY <= iRange; iY++)
				{
					CvPlot* pLoopPlot = plotXY(pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), iX, iY);
					if ((pLoopPlot != NULL) && (pLoopPlot->getOwner() == getID()))
					{
						if ((pLoopPlot->isConnectedToCapital()) || pLoopPlot->isCity())
						{
							int iValue = 1000;
							if (pLoopPlot->isCity())
							{
								iValue += 500;
								if (pLoopPlot->getPlotCity()->isCapital())
								{
									iValue += 500;
								}
							}
							if (pLoopPlot->isRoute())
							{
								iValue += 100;
							}
							int iDistance = GC.getMapINLINE().calculatePathDistance(pLoopCity->plot(), pLoopPlot);
							if (iDistance > 0)
							{
								iValue /= (1 + iDistance);
								
								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = pLoopPlot;
								}
							}
						}
					}
				}
			}
			if (NULL != pBestPlot)
			{
				if (!AI_advancedStartDoRoute(pBestPlot, pLoopCity->plot()))
				{
					return;
				}
			}
		}
	}
}


void CvPlayerAI::AI_doAdvancedStart(bool bNoExit)
{
	FAssertMsg(!isBarbarian(), "Should not be called for barbarians!");

	if (NULL == getStartingPlot())
	{
		FAssert(false);
		return;
	}

	int iLoop;
	CvCity* pLoopCity;
	
	int iStartingPoints = getAdvancedStartPoints();
	int iRevealPoints = (iStartingPoints * 10) / 100;
	int iMilitaryPoints = (iStartingPoints * (isHuman() ? 17 : (10 + (GC.getLeaderHeadInfo(getPersonalityType()).getBuildUnitProb() / 3)))) / 100;
	int iCityPoints = iStartingPoints - (iMilitaryPoints + iRevealPoints);

	if (getCapitalCity() != NULL)
	{
		AI_advancedStartPlaceCity(getCapitalCity()->plot());
	}
	else
	{
		for (int iPass = 0; iPass < 2 && NULL == getCapitalCity(); ++iPass)
		{
			CvPlot* pBestCapitalPlot = AI_advancedStartFindCapitalPlot();

			if (pBestCapitalPlot != NULL)
			{
				if (!AI_advancedStartPlaceCity(pBestCapitalPlot))
				{
					FAssertMsg(false, "AS AI: Unexpected failure placing capital");
				}
				break;
			}
			else
			{
				//If this point is reached, the advanced start system is broken.
				//Find a new starting plot for this player
				setStartingPlot(findStartingPlot(false), true);
				//Redo Starting visibility
				CvPlot* pStartingPlot = getStartingPlot();
				if (NULL != pStartingPlot)
				{
					for (int iPlotLoop = 0; iPlotLoop < GC.getMapINLINE().numPlots(); ++iPlotLoop)
					{
						CvPlot* pPlot = GC.getMapINLINE().plotByIndex(iPlotLoop);

						if (plotDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE()) <= GC.getDefineINT("ADVANCED_START_SIGHT_RANGE"))
						{
							pPlot->setRevealed(getTeam(), true, false, NO_TEAM, false);
						}
					}
				}
			}
		}

		if (getCapitalCity() == NULL)
		{
			if (!bNoExit)
			{
				doAdvancedStartAction(ADVANCEDSTARTACTION_EXIT, -1, -1, -1, true);
			}
			return;
		}
	}
	
	iCityPoints -= (iStartingPoints - getAdvancedStartPoints());
	
	int iLastPointsTotal = getAdvancedStartPoints();
	
	for (int iPass = 0; iPass < 6; iPass++)
	{
		for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
		{
			CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
			if (pLoopPlot->isRevealed(getTeam(), false))
			{
				if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
				{
					AI_advancedStartRevealRadius(pLoopPlot, CITY_PLOTS_RADIUS);					
				}
				else
				{
					for (int iJ = 0; iJ < NUM_CARDINALDIRECTION_TYPES; iJ++)
					{
						CvPlot* pLoopPlot2 = plotCardinalDirection(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), (CardinalDirectionTypes)iJ);
						if ((pLoopPlot2 != NULL) && (getAdvancedStartVisibilityCost(true, pLoopPlot2) > 0))
						{
							//Mildly maphackery but any smart human can see the terrain type of a tile.
							pLoopPlot2->getTerrainType();
							int iFoodYield = GC.getTerrainInfo(pLoopPlot2->getTerrainType()).getYield(YIELD_FOOD);
							if (pLoopPlot2->getFeatureType() != NO_FEATURE)
							{
								iFoodYield += GC.getFeatureInfo(pLoopPlot2->getFeatureType()).getYieldChange(YIELD_FOOD);
							}
							if (((iFoodYield >= 2) && !pLoopPlot2->isFreshWater()) || pLoopPlot2->isHills() || pLoopPlot2->isRiver())
							{
								doAdvancedStartAction(ADVANCEDSTARTACTION_VISIBILITY, pLoopPlot2->getX_INLINE(), pLoopPlot2->getY_INLINE(), -1, true);						
							}
						}
					}
				}
			}
			if ((iLastPointsTotal - getAdvancedStartPoints()) > iRevealPoints)
			{
				break;
			}
		}
	}
	
	iLastPointsTotal = getAdvancedStartPoints();
	iCityPoints = std::min(iCityPoints, iLastPointsTotal);
	int iArea = -1; //getStartingPlot()->getArea();
	
	//Spend econ points on a tech?
	int iTechRand = 90 + GC.getGame().getSorenRandNum(20, "AI AS Buy Tech 1");
	int iTotalTechSpending = 0;
	
	if (getCurrentEra() == 0)
	{
		TechTypes eTech = AI_bestTech(1);
		if ((eTech != NO_TECH) && !GC.getTechInfo(eTech).isRepeat())
		{
			int iTechCost = getAdvancedStartTechCost(eTech, true);
			if (iTechCost > 0)
			{
				doAdvancedStartAction(ADVANCEDSTARTACTION_TECH, -1, -1, eTech, true);
				iTechRand -= 50;
				iTotalTechSpending += iTechCost;
			}
		}
	}
	
	bool bDonePlacingCities = false;	
	for (int iPass = 0; iPass < 100; ++iPass)
	{
		int iRand = iTechRand + 10 * getNumCities();
		if ((iRand > 0) && (GC.getGame().getSorenRandNum(100, "AI AS Buy Tech 2") < iRand))
		{
			TechTypes eTech = AI_bestTech(1);
			if ((eTech != NO_TECH) && !GC.getTechInfo(eTech).isRepeat())
			{
				int iTechCost = getAdvancedStartTechCost(eTech, true);
				if ((iTechCost > 0) && ((iTechCost + iTotalTechSpending) < (iCityPoints / 4)))
				{
					doAdvancedStartAction(ADVANCEDSTARTACTION_TECH, -1, -1, eTech, true);
					iTechRand -= 50;
					iTotalTechSpending += iTechCost;
					
					for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
					{	
						AI_advancedStartPlaceCity(pLoopCity->plot());
					}
				}
			}
		}
		int iBestFoundValue = 0;
		CvPlot* pBestFoundPlot = NULL;
		AI_updateFoundValues(false);
		for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
		{
			CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
			//if (pLoopPlot->area() == getStartingPlot()->area())
			{
				if (plotDistance(getStartingPlot()->getX_INLINE(), getStartingPlot()->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()) < 9)
				{
					if (pLoopPlot->getFoundValue(getID()) > iBestFoundValue)
					{
						if (getAdvancedStartCityCost(true, pLoopPlot) > 0)
						{
							pBestFoundPlot = pLoopPlot;
							iBestFoundValue = pLoopPlot->getFoundValue(getID());
						}
					}
				}
			}
		}
		
		if (iBestFoundValue < ((getNumCities() == 0) ? 1 : (500 + 250 * getNumCities())))
		{
			bDonePlacingCities = true;			
		}
		if (!bDonePlacingCities)
		{
			int iCost = getAdvancedStartCityCost(true, pBestFoundPlot);
			if (iCost > getAdvancedStartPoints())
			{
				bDonePlacingCities = true;
			}// at 500pts, we have 200, we spend 100. 
			else if (((iLastPointsTotal - getAdvancedStartPoints()) + iCost) > iCityPoints)
			{
				bDonePlacingCities = true;
			}
		}
		
		if (!bDonePlacingCities)
		{
			if (!AI_advancedStartPlaceCity(pBestFoundPlot))
			{
				FAssertMsg(false, "AS AI: Failed to place city (non-capital)");
				bDonePlacingCities = true;
			}
		}

		if (bDonePlacingCities)
		{
			break;
		}
	}


	bool bDoneWithTechs = false;
	while (!bDoneWithTechs)
	{
		bDoneWithTechs = true;
		TechTypes eTech = AI_bestTech(1);
		if (eTech != NO_TECH && !GC.getTechInfo(eTech).isRepeat())
		{
			int iTechCost = getAdvancedStartTechCost(eTech, true);
			if ((iTechCost > 0) && ((iTechCost + iLastPointsTotal - getAdvancedStartPoints()) <= iCityPoints))
			{
				doAdvancedStartAction(ADVANCEDSTARTACTION_TECH, -1, -1, eTech, true);
				bDoneWithTechs = false;
			}
		}
	}
	
	{
		//Land
		AI_advancedStartPlaceExploreUnits(true);
		if (getCurrentEra() > 2)
		{
			//Sea
			AI_advancedStartPlaceExploreUnits(false);
			if (GC.getGameINLINE().circumnavigationAvailable())
			{
				if (GC.getGameINLINE().getSorenRandNum(GC.getGameINLINE().countCivPlayersAlive(), "AI AS buy 2nd sea explorer") < 2)
				{
					AI_advancedStartPlaceExploreUnits(false);
				}
			}
		}
	}
	
	AI_advancedStartRouteTerritory();
		
	bool bDoneBuildings = (iLastPointsTotal - getAdvancedStartPoints()) > iCityPoints;
	for (int iPass = 0; iPass < 10 && !bDoneBuildings; ++iPass)
	{
		for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			BuildingTypes eBuilding = pLoopCity->AI_bestAdvancedStartBuilding(iPass);
			if (eBuilding != NO_BUILDING)
			{
				bDoneBuildings = (iLastPointsTotal - (getAdvancedStartPoints() - getAdvancedStartBuildingCost(eBuilding, true, pLoopCity))) > iCityPoints;
				if (!bDoneBuildings)
				{
					doAdvancedStartAction(ADVANCEDSTARTACTION_BUILDING, pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), eBuilding, true);
				}
				else
				{
					//continue there might be cheaper buildings in other cities we can afford
				}
			}
		}
	}
	
	//Units
	std::vector<UnitAITypes> aeUnitAITypes;
	aeUnitAITypes.push_back(UNITAI_CITY_DEFENSE);
	aeUnitAITypes.push_back(UNITAI_WORKER);
	aeUnitAITypes.push_back(UNITAI_RESERVE);
	aeUnitAITypes.push_back(UNITAI_COUNTER);
	
	
	bool bDone = false;
	for (int iPass = 0; iPass < 10; ++iPass)
	{
		for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			if ((iPass == 0) || (pLoopCity->getArea() == getStartingPlot()->getArea()))
			{
				CvPlot* pUnitPlot = pLoopCity->plot();
				//Token defender
				UnitTypes eBestUnit = AI_bestAdvancedStartUnitAI(pUnitPlot, aeUnitAITypes[iPass % aeUnitAITypes.size()]);
				if (eBestUnit != NO_UNIT)
				{
					if (getAdvancedStartUnitCost(eBestUnit, true, pUnitPlot) > getAdvancedStartPoints())
					{
						bDone = true;
						break;
					}
					doAdvancedStartAction(ADVANCEDSTARTACTION_UNIT, pUnitPlot->getX(), pUnitPlot->getY(), eBestUnit, true);
				}
			}
		}
	}
	
	if (isHuman())
	{
		// remove unhappy population
		for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			while (pLoopCity->angryPopulation() > 0 && getAdvancedStartPopCost(false, pLoopCity) > 0)
			{
				doAdvancedStartAction(ADVANCEDSTARTACTION_POP, pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), -1, false);
			}
		}
	}

	if (!bNoExit)
	{
		doAdvancedStartAction(ADVANCEDSTARTACTION_EXIT, -1, -1, -1, true);
	}

}


void CvPlayerAI::AI_recalculateFoundValues(int iX, int iY, int iInnerRadius, int iOuterRadius) const
{
	CvPlot* pLoopPlot;
	int iLoopX, iLoopY;
	//int iValue;
	
	for (iLoopX = -iOuterRadius; iLoopX <= iOuterRadius; iLoopX++)
	{
		for (iLoopY = -iOuterRadius; iLoopY <= iOuterRadius; iLoopY++)
		{
			pLoopPlot = plotXY(iX, iY, iLoopX, iLoopY);
			if ((NULL != pLoopPlot) && !AI_isPlotCitySite(pLoopPlot))
			{
				if (stepDistance(0, 0, iLoopX, iLoopY) <= iInnerRadius)
				{
					if (!((iLoopX == 0) && (iLoopY == 0)))
					{
						pLoopPlot->setFoundValue(getID(), 0);
					}
				}
				else
				{
					if ((pLoopPlot != NULL) && (pLoopPlot->isRevealed(getTeam(), false)))
					{
						long lResult=-1;
						if(GC.getUSE_GET_CITY_FOUND_VALUE_CALLBACK())
						{
							CyArgsList argsList;
							argsList.add((int)getID());
							argsList.add(pLoopPlot->getX());
							argsList.add(pLoopPlot->getY());
							gDLL->getPythonIFace()->callFunction(PYGameModule, "getCityFoundValue", argsList.makeFunctionArgs(), &lResult);
						}

						short iValue; // K-Mod
						if (lResult == -1)
						{
							iValue = AI_foundValue(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
						}
						else
						{
							//iValue = lResult;
							iValue = (short)std::min((long)MAX_SHORT, lResult); // K-Mod
						}

						pLoopPlot->setFoundValue(getID(), iValue);

						if (iValue > pLoopPlot->area()->getBestFoundValue(getID()))
						{
							pLoopPlot->area()->setBestFoundValue(getID(), iValue);
						}
					}
				}
			}
		}
	}
}

	
int CvPlayerAI::AI_getMinFoundValue() const
{
	PROFILE_FUNC();
	//int iValue = 600;
	int iValue = GC.getDefineINT("BBAI_MINIMUM_FOUND_VALUE"); // K-Mod
	//int iNetCommerce = 1 + getCommerceRate(COMMERCE_GOLD) + getCommerceRate(COMMERCE_RESEARCH) + std::max(0, getGoldPerTurn());
	int iNetCommerce = AI_getAvailableIncome(); // K-Mod
	int iNetExpenses = calculateInflatedCosts() + std::max(0, -getGoldPerTurn());
	/*  <advc.031> Prevent the AI from stopping to expand once corporation
		maintenance becomes a major factor */
	iNetExpenses = std::max(0, iNetExpenses -
			countHeadquarters() * getNumCities() * 4); // </advc.031>
	iValue *= iNetCommerce;
	iValue /= std::max(std::max(1, iNetCommerce / 4), iNetCommerce - iNetExpenses);

	// advc.105: Don't do this at all
	/*if (GET_TEAM(getTeam()).getAnyWarPlanCount(1) > 0)
	{
		iValue *= 2;
	}*/

	// K-Mod. # of cities maintenance cost increase...
	int iNumCitiesPercent = 100;
	//iNumCitiesPercent *= (getAveragePopulation() + 17);
	//iNumCitiesPercent /= 18;

	iNumCitiesPercent *= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getNumCitiesMaintenancePercent();
	iNumCitiesPercent /= 100;

	iNumCitiesPercent *= GC.getHandicapInfo(getHandicapType()).getNumCitiesMaintenancePercent();
	iNumCitiesPercent /= 100;

	//iNumCitiesPercent *= std::max(0, getNumCitiesMaintenanceModifier() + 100);
	//iNumCitiesPercent /= 100;

	// The marginal cost increase is roughly equal to double the cost of a current city...
	// But we're really going to have to fudge it anyway, because the city value is in arbitrary units
	// lets just say each gold per turn is worth roughly 60 'value points'.
	// In the future, this could be AI flavour based.
	iValue += (iNumCitiesPercent * getNumCities() * //60) / 100;
			/*  advc.130v: We may not be paying much for cities, but our master
				will have to pay as well. */
			// advc.031: Also raise the MinFoundValue a bit overall (60->67)
			(67 + (GET_TEAM(getTeam()).isCapitulated() ? 33 : 0))) / 100;
	// K-Mod end
	// advc.031:
	iValue = ::round(iValue / std::min(1.0, 1.65 * amortizationMultiplier(15)));
	return iValue;
}
	
void CvPlayerAI::AI_updateCitySites(int iMinFoundValueThreshold, int iMaxSites)
{
	// kmodx: redundant code removed
	int iValue;
	int iI;

	// K-Mod. Always recommend the starting location on the first turn.
	// (because we don't have enough information to overrule what the game has cooked for us.)
	if (
		isHuman() &&  /* advc.108: OK for recommendations, but allow AI to evaluate
					     plots and settle elsewhere - in principle;
						 CvUnitAI::AI_settleMove still always settles in the
						 plot. updateCitySites isn't the place to enforce this
						 behavior. */
		getNumCities() == 0 && iMaxSites > 0 && GC.getGameINLINE().getElapsedGameTurns() == 0 &&
		m_iStartingX != INVALID_PLOT_COORD && m_iStartingY != INVALID_PLOT_COORD)
	{
		m_aiAICitySites.push_back(GC.getMapINLINE().plotNum(m_iStartingX, m_iStartingY));
		//AI_recalculateFoundValues(m_iStartingX, m_iStartingY, CITY_PLOTS_RADIUS, 2 * CITY_PLOTS_RADIUS);
		return; // don't bother trying to pick a secondary spot
	}
	// K-Mod end

	int iPass = 0;
	while (iPass < iMaxSites)
	{
		//Add a city to the list.
		int iBestFoundValue = 0;
		CvPlot* pBestFoundPlot = NULL;

		for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
		{
			CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
			if (pLoopPlot->isRevealed(getTeam(), false))
			{
				iValue = pLoopPlot->getFoundValue(getID());
				if (iValue > iMinFoundValueThreshold)
				{
					if (!AI_isPlotCitySite(pLoopPlot))
					{
						iValue *= std::min(NUM_CITY_PLOTS * 2, pLoopPlot->area()->getNumUnownedTiles());					

						if (iValue > iBestFoundValue)
						{
							iBestFoundValue = iValue;
							pBestFoundPlot = pLoopPlot;
						}
					}
				}
			}
		}
		if (pBestFoundPlot != NULL)
		{
			m_aiAICitySites.push_back(GC.getMapINLINE().plotNum(pBestFoundPlot->getX_INLINE(), pBestFoundPlot->getY_INLINE()));
			AI_recalculateFoundValues(pBestFoundPlot->getX_INLINE(), pBestFoundPlot->getY_INLINE(), CITY_PLOTS_RADIUS, 2 * CITY_PLOTS_RADIUS);
		}
		else
		{
			break;
		}
		iPass++;
	}
}

void CvPlayerAI::AI_invalidateCitySites(int iMinFoundValueThreshold)
{
	/* original bts code
	m_aiAICitySites.clear(); */
	// K-Mod. note: this clear-by-value stuff isn't actually used yet... but at least it works now.
	std::vector<int> keptSites;
	if (iMinFoundValueThreshold > 0) // less than zero means clear all.
	{
		for (size_t iI = 0; iI < m_aiAICitySites.size(); iI++)
		{
			if (GC.getMapINLINE().plotByIndexINLINE(m_aiAICitySites[iI])->getFoundValue(getID()) >= iMinFoundValueThreshold)
				keptSites.push_back(m_aiAICitySites[iI]);
		}
	}
	m_aiAICitySites.swap(keptSites);
	// K-Mod end
}

int CvPlayerAI::AI_getNumCitySites() const
{
	return m_aiAICitySites.size();
}

bool CvPlayerAI::AI_isPlotCitySite(CvPlot* pPlot) const
{
	std::vector<int>::const_iterator it;
	int iPlotIndex = GC.getMapINLINE().plotNumINLINE(pPlot->getX_INLINE(), pPlot->getY_INLINE());
	
	for (it = m_aiAICitySites.begin(); it != m_aiAICitySites.end(); it++)
	{
		if ((*it) == iPlotIndex)
		{
			return true;
		}
	}
	return false;
}

int CvPlayerAI::AI_getNumAreaCitySites(int iAreaID, int& iBestValue) const
{
	std::vector<int>::const_iterator it;
	int iCount = 0;
	iBestValue = 0;
	
	for (it = m_aiAICitySites.begin(); it != m_aiAICitySites.end(); it++)
	{
		CvPlot* pCitySitePlot = GC.getMapINLINE().plotByIndex((*it));
		if (pCitySitePlot->getArea() == iAreaID)
		{
			iCount++;
			iBestValue = std::max(iBestValue, pCitySitePlot->getFoundValue(getID()));
		}
	}
	return iCount;
}

int CvPlayerAI::AI_getNumAdjacentAreaCitySites(int iWaterAreaID, int iExcludeArea, int& iBestValue) const
{
	std::vector<int>::const_iterator it;
	int iCount = 0;
	iBestValue = 0;

	for (it = m_aiAICitySites.begin(); it != m_aiAICitySites.end(); it++)
	{
		CvPlot* pCitySitePlot = GC.getMapINLINE().plotByIndex((*it));
		if (pCitySitePlot->getArea() != iExcludeArea)
		{
			if (pCitySitePlot->isAdjacentToArea(iWaterAreaID))
			{
				iCount++;
				iBestValue = std::max(iBestValue, pCitySitePlot->getFoundValue(getID()));
			}
		}
	}
	return iCount;
}

// K-Mod. Return the number of city sites that are in a primary area, with a site value above the minimum
int CvPlayerAI::AI_getNumPrimaryAreaCitySites(int iMinimumValue) const
{
	int iCount = 0;

	std::vector<int>::const_iterator it;
	for (it = m_aiAICitySites.begin(); it != m_aiAICitySites.end(); it++)
	{
		CvPlot* pCitySitePlot = GC.getMapINLINE().plotByIndex((*it));
		if (AI_isPrimaryArea(pCitySitePlot->area()) && pCitySitePlot->getFoundValue(getID()) >= iMinimumValue)
		{
			iCount++;
		}
	}
	return iCount;
}
// K-Mod end

CvPlot* CvPlayerAI::AI_getCitySite(int iIndex) const
{
	FAssert(iIndex < (int)m_aiAICitySites.size());
	return GC.getMapINLINE().plotByIndex(m_aiAICitySites[iIndex]);
}

// K-Mod
bool CvPlayerAI::AI_deduceCitySite(const CvCity* pCity) const
{
	return GET_TEAM(getTeam()).AI_deduceCitySite(pCity);
}

// K-Mod. This function is essentially a merged version of two original bts functions: CvPlayer::countPotentialForeignTradeCities and CvPlayer::countPotentialForeignTradeCitiesConnected.
// I've rewritten it to be a single function, and changed it so that when checking that he cities are connected, it does not count cities if the foreign civ has "no foreign trade".
// (it ignores that effect for our civ, but checks it for the foreign civ - the reasoning is basically that the other player's civics are out of our control.)
int CvPlayerAI::AI_countPotentialForeignTradeCities(bool bCheckConnected, bool bCheckForeignTradePolicy, CvArea* pIgnoreArea) const
{
	CvCity* pCapitalCity = getCapitalCity();

	if (pCapitalCity == 0)
		return 0;

	const CvTeam& kTeam = GET_TEAM(getTeam());

	int iCount = 0;

	for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i = (PlayerTypes)(i+1))
	{
		const CvPlayer& kLoopPlayer = GET_PLAYER(i);

		if (!kLoopPlayer.isAlive() || kLoopPlayer.getTeam() == getTeam() || !kTeam.isFreeTrade(kLoopPlayer.getTeam()))
			continue;

		if (bCheckForeignTradePolicy && kLoopPlayer.isNoForeignTrade() && !kTeam.isVassal(kLoopPlayer.getTeam()) && !GET_TEAM(kLoopPlayer.getTeam()).isVassal(getTeam()))
			continue;

		// this is a legitimate foreign trade partner. Count the number of (connected) cities.
		if (bCheckConnected)
		{
			int iLoop;
			for (CvCity* pLoopCity = kLoopPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kLoopPlayer.nextCity(&iLoop))
			{
				if ((!pIgnoreArea || pLoopCity->area() != pIgnoreArea) && pLoopCity->plotGroup(getID()) == pCapitalCity->plotGroup(getID()))
				{
					iCount++;
				}
			}
		}
		else
		{
			iCount += kLoopPlayer.getNumCities();

			if (pIgnoreArea)
			{
				iCount -= pIgnoreArea->getCitiesPerPlayer(i);
			}
		}
	}

	FAssert(iCount >= 0);

	return iCount;
}
// K-Mod end

int CvPlayerAI::AI_bestAreaUnitAIValue(UnitAITypes eUnitAI, CvArea* pArea, UnitTypes* peBestUnitType) const
{
	
	CvCity* pCity = NULL;
	
	if (pArea != NULL)
	{
		if (getCapitalCity() != NULL)
		{
			if (pArea->isWater())
			{
				if (getCapitalCity()->plot()->isAdjacentToArea(pArea))
				{
					pCity = getCapitalCity();
				}
			}
			else
			{
				if (getCapitalCity()->getArea() == pArea->getID())
				{
					pCity = getCapitalCity();
				}
			}
		}
	
		if (NULL == pCity)
		{
			CvCity* pLoopCity;
			int iLoop;
			for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
			{
				if (pArea->isWater())
				{
					if (pLoopCity->plot()->isAdjacentToArea(pArea))
					{
						pCity = pLoopCity;
						break;
					}
				}
				else
				{
					if (pLoopCity->getArea() == pArea->getID())
					{
						pCity = pLoopCity;
						break;
					}
				}
			}
		}
	}

	return AI_bestCityUnitAIValue(eUnitAI, pCity, peBestUnitType);
}

int CvPlayerAI::AI_bestCityUnitAIValue(UnitAITypes eUnitAI, CvCity* pCity, UnitTypes* peBestUnitType) const
{
	UnitTypes eLoopUnit;
	int iValue;
	int iBestValue;
	int iI;
	
	FAssertMsg(eUnitAI != NO_UNITAI, "UnitAI is not assigned a valid value");
	
	iBestValue = 0;
	
	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

		if (eLoopUnit != NO_UNIT)
		{
			//if (!isHuman() || (GC.getUnitInfo(eLoopUnit).getDefaultUnitAIType() == eUnitAI)) // disabled by K-Mod
			{
				if (NULL == pCity ? (canTrain(eLoopUnit)
						&& AI_haveResourcesToTrain(eLoopUnit)) : // k146
						pCity->canTrain(eLoopUnit))
				{
					iValue = AI_unitValue(eLoopUnit, eUnitAI, (pCity == NULL) ? NULL : pCity->area());
					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						if (peBestUnitType != NULL)
						{
							*peBestUnitType = eLoopUnit;
						}
					}
				}
			}
		}
	}

	return iBestValue;
}

int CvPlayerAI::AI_calculateTotalBombard(DomainTypes eDomain) const
{
	int iI;
	int iTotalBombard = 0;
	
	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		UnitTypes eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));
		if (eLoopUnit != NO_UNIT)
		{
			if (GC.getUnitInfo(eLoopUnit).getDomainType() == eDomain)
			{
				int iBombardRate = GC.getUnitInfo(eLoopUnit).getBombardRate();
				
				if (iBombardRate > 0)
				{
					if( GC.getUnitInfo(eLoopUnit).isIgnoreBuildingDefense() )
					{
						iBombardRate *= 3;
						iBombardRate /= 2;
					}

					iTotalBombard += iBombardRate * getUnitClassCount((UnitClassTypes)iI);
				}
				
				int iBombRate = GC.getUnitInfo(eLoopUnit).getBombRate();
				if (iBombRate > 0)
				{
					iTotalBombard += iBombRate * getUnitClassCount((UnitClassTypes)iI);
				}
			}
		}
	}
	
	return iTotalBombard;
}

void CvPlayerAI::AI_updateBonusValue(BonusTypes eBonus)
{
	FAssert(m_aiBonusValue != NULL);

	//reset
    m_aiBonusValue[eBonus] = -1;
	m_aiBonusValueTrade[eBonus] = -1; // advc.036
}

void CvPlayerAI::AI_updateBonusValue()
{
	PROFILE_FUNC();

	FAssert(m_aiBonusValue != NULL);

	for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
	{
		AI_updateBonusValue((BonusTypes)iI);		
	}
}

int CvPlayerAI::AI_getUnitClassWeight(UnitClassTypes eUnitClass) const
{
	return m_aiUnitClassWeights[eUnitClass] / 100;
}

int CvPlayerAI::AI_getUnitCombatWeight(UnitCombatTypes eUnitCombat) const
{
	return m_aiUnitCombatWeights[eUnitCombat] / 100;
}

void CvPlayerAI::AI_doEnemyUnitData()
{
	std::vector<int> aiUnitCounts(GC.getNumUnitInfos(), 0);
	
	std::vector<int> aiDomainSums(NUM_DOMAIN_TYPES, 0);
	
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	int iI;
	
	int iOldTotal = 0;
	int iNewTotal = 0;
	
	// Count enemy land and sea units visible to us
	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
		int iAdjacentAttackers = -1;
		if (pLoopPlot->isVisible(getTeam(), false))
		{
			pUnitNode = pLoopPlot->headUnitNode();

			while (pUnitNode != NULL)
			{
				pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);
				
				if (pLoopUnit->canFight())
				{
					int iUnitValue = 1;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/18/10                                jdog5000      */
/*                                                                                              */
/* War Strategy AI                                                                              */
/************************************************************************************************/
					//if (atWar(getTeam(), pLoopUnit->getTeam()))
					if( GET_TEAM(getTeam()).AI_getWarPlan(pLoopUnit->getTeam()) != NO_WARPLAN )
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
					{
						iUnitValue += 10;
					
						if ((pLoopPlot->getOwnerINLINE() == getID()))
						{
							iUnitValue += 15;
							// <cdtw> try a little harder to counter city attack units
							if(pLoopUnit->combatLimit() == GC.getMAX_HIT_POINTS())
								iUnitValue += pLoopUnit->cityAttackModifier() / 10;
							// </cdtw>
						}
						else if (atWar(getTeam(), pLoopPlot->getTeam()))
						{
							if (iAdjacentAttackers == -1)
							{
								iAdjacentAttackers = GET_PLAYER(pLoopPlot->getOwnerINLINE()).AI_adjacentPotentialAttackers(pLoopPlot);
							}
							if (iAdjacentAttackers > 0)
							{
								iUnitValue += 15;
							}
						}
					}
					else if (pLoopUnit->getOwnerINLINE() != getID())
					{
						iUnitValue += pLoopUnit->canAttack() ? 4 : 1;
						if (pLoopPlot->getCulture(getID()) > 0)
						{
							iUnitValue += pLoopUnit->canAttack() ? 4 : 1;
						}
					}
					
					// If we hadn't seen any of this class before
					if (m_aiUnitClassWeights[pLoopUnit->getUnitClassType()] == 0)
					{
						iUnitValue *= 4;
					}
					
					iUnitValue *= pLoopUnit->baseCombatStr();
					aiUnitCounts[pLoopUnit->getUnitType()] += iUnitValue;
					aiDomainSums[pLoopUnit->getDomainType()] += iUnitValue;
					iNewTotal += iUnitValue;
				}
			}
		}
	}

	if (iNewTotal == 0)
	{
		//This should rarely happen.
		return;
	}

	//Decay
	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		m_aiUnitClassWeights[iI] -= 100;
		m_aiUnitClassWeights[iI] *= 3;
		m_aiUnitClassWeights[iI] /= 4;
		m_aiUnitClassWeights[iI] = std::max(0, m_aiUnitClassWeights[iI]);
	}
	
	for (iI = 0; iI < GC.getNumUnitInfos(); iI++)
	{
		if (aiUnitCounts[iI] > 0)
		{
			UnitTypes eLoopUnit = (UnitTypes)iI;

			TechTypes eTech = (TechTypes)GC.getUnitInfo((UnitTypes)iI).getPrereqAndTech();
			
			int iEraDiff = (eTech == NO_TECH) ? 4 : std::min(4, getCurrentEra() - GC.getTechInfo(eTech).getEra());

			if (iEraDiff > 1)
			{
				iEraDiff -= 1;
				aiUnitCounts[iI] *= 3 - iEraDiff;
				aiUnitCounts[iI] /= 3;
			}
			FAssert(aiDomainSums[GC.getUnitInfo(eLoopUnit).getDomainType()] > 0);
			m_aiUnitClassWeights[GC.getUnitInfo(eLoopUnit).getUnitClassType()] += (5000 * aiUnitCounts[iI]) / std::max(1, aiDomainSums[GC.getUnitInfo(eLoopUnit).getDomainType()]);
		}
	}
	
	for (iI = 0; iI < GC.getNumUnitCombatInfos(); ++iI)
	{
		m_aiUnitCombatWeights[iI] = 0;
	}
	
	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		if (m_aiUnitClassWeights[iI] > 0)
		{
			UnitTypes eUnit = (UnitTypes)GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex();
			m_aiUnitCombatWeights[GC.getUnitInfo(eUnit).getUnitCombatType()] += m_aiUnitClassWeights[iI];

		}
	}
	
	for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
	{
		if (m_aiUnitCombatWeights[iI] > 25)
		{
			m_aiUnitCombatWeights[iI] += 2500;
		}
		else if (m_aiUnitCombatWeights[iI] > 0)
		{
			m_aiUnitCombatWeights[iI] += 1000;
		}
	}
}

int CvPlayerAI::AI_calculateUnitAIViability(UnitAITypes eUnitAI, DomainTypes eDomain) const
{
	int iBestUnitAIStrength = 0;
	int iBestOtherStrength = 0;
	
	for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		UnitTypes eLoopUnit = (UnitTypes)GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex();
		const CvUnitInfo& kUnitInfo = GC.getUnitInfo(eLoopUnit);
		if (kUnitInfo.getDomainType() == eDomain)
		{

			TechTypes eTech = (TechTypes)kUnitInfo.getPrereqAndTech();
			if ((m_aiUnitClassWeights[iI] > 0) || GET_TEAM(getTeam()).isHasTech((eTech)))
			{
				if (kUnitInfo.getUnitAIType(eUnitAI))
				{
					iBestUnitAIStrength = std::max(iBestUnitAIStrength, kUnitInfo.getCombat());
				}

				iBestOtherStrength = std::max(iBestOtherStrength, kUnitInfo.getCombat());
			}
		}
	}
	
	return (100 * iBestUnitAIStrength) / std::max(1, iBestOtherStrength);
}

// K-Mod. Units with the lowest "disband value" will be disbanded first.
// (this code is based on the old code from CvPlayerAI::AI_disbandUnit)
int CvPlayerAI::AI_disbandValue(const CvUnit* pUnit, bool bMilitaryOnly) const
{
	if (pUnit->hasCargo() || pUnit->isGoldenAge() || pUnit->getUnitInfo().getProductionCost() < 0 || (bMilitaryOnly && !pUnit->canFight()))
		return MAX_INT;

	if (pUnit->isMilitaryHappiness() && pUnit->plot()->isCity() && pUnit->plot()->plotCount(PUF_isMissionAIType, MISSIONAI_GUARD_CITY, -1, getID()) < 2)
		return MAX_INT;

	int iValue = 1000 + GC.getGameINLINE().getSorenRandNum(200, "Disband Value");

	iValue *= 100 + (pUnit->getUnitInfo().getProductionCost() * 3);
	iValue /= 100;

	iValue *= 100 + std::max(pUnit->getExperience(), pUnit->getLevel() * pUnit->getLevel() * 2 / 3) * 15;
	iValue /= 100;

	/*if (plot()->getTeam() == getTeam())
	{
		iValue *= 2;

		if (canDefend() && plot()->isCity())
		{
			iValue *= 2;
		}
	}*/

	switch (pUnit->getGroup()->AI_getMissionAIType())
	{
	case MISSIONAI_GUARD_CITY:
	case MISSIONAI_PICKUP:
	case MISSIONAI_FOUND:
	case MISSIONAI_SPREAD:
		iValue *= 3; // high value
		break;

	case MISSIONAI_STRANDED:
		iValue /= 2;
	case MISSIONAI_RETREAT:
	case MISSIONAI_PATROL:
	case NO_MISSIONAI:
		break; // low value

	default:
		iValue *= 2; // medium
		break;
	}

	// Multiplying by higher number means unit has higher priority, less likely to be disbanded
	switch (pUnit->AI_getUnitAIType())
	{
	case UNITAI_UNKNOWN:
	case UNITAI_ANIMAL:
		break;

	case UNITAI_SETTLE:
		iValue *= 16;
		break;

	case UNITAI_WORKER:
		if (GC.getGame().getGameTurn() - pUnit->getGameTurnCreated() <= 10 ||
			!pUnit->plot()->isCity() ||
			pUnit->plot()->getPlotCity()->AI_getWorkersNeeded() > 0)
		{
			iValue *= 10;
		}
		break;

	case UNITAI_ATTACK:
	case UNITAI_COUNTER:
		iValue *= 4;
		break;

	case UNITAI_ATTACK_CITY:
	case UNITAI_COLLATERAL:
	case UNITAI_PILLAGE:
	case UNITAI_RESERVE:
		iValue *= 3;
		break;

	case UNITAI_CITY_DEFENSE:
	case UNITAI_CITY_COUNTER:
	case UNITAI_CITY_SPECIAL:
	case UNITAI_PARADROP:
		iValue *= 6;
		break;

	case UNITAI_EXPLORE:
		if (GC.getGame().getGameTurn() - pUnit->getGameTurnCreated() < 20)
			iValue *= 2;
		if (pUnit->plot()->getTeam() != getTeam())
			iValue *= 2;
		break;

	case UNITAI_MISSIONARY:
		if (GC.getGame().getGameTurn() - pUnit->getGameTurnCreated() < 10
			|| pUnit->plot()->getTeam() != getTeam())
		{
			iValue *= 8;
		}
		break;

	case UNITAI_PROPHET:
	case UNITAI_ARTIST:
	case UNITAI_SCIENTIST:
	case UNITAI_GENERAL:
	case UNITAI_MERCHANT:
	case UNITAI_ENGINEER:
	case UNITAI_GREAT_SPY:
		iValue *= 20;
		break;

	case UNITAI_SPY:
		iValue *= 10;
		break;

	case UNITAI_ICBM:
		iValue *= 5;
		break;

	case UNITAI_WORKER_SEA:
		iValue *= 15;
		break;

	case UNITAI_ATTACK_SEA:
	case UNITAI_RESERVE_SEA:
	case UNITAI_ESCORT_SEA:
		iValue *= 2;
		break;

	case UNITAI_EXPLORE_SEA:
		if (GC.getGame().getGameTurn() - pUnit->getGameTurnCreated() < 20)
			iValue *= 3;
		if (pUnit->plot()->getTeam() != getTeam())
			iValue *= 3;
		break;

	case UNITAI_SETTLER_SEA:
		iValue *= 6;
		break;

	case UNITAI_MISSIONARY_SEA:
	case UNITAI_SPY_SEA:
		iValue *= 4;
		break;

	case UNITAI_ASSAULT_SEA:
	case UNITAI_CARRIER_SEA:
	case UNITAI_MISSILE_CARRIER_SEA:
		if (isFocusWar()) // advc.105
			//GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0)
		{
			iValue *= 5;
		}
		else
		{
			iValue *= 2;
		}
		break;

	case UNITAI_PIRATE_SEA:
	case UNITAI_ATTACK_AIR:
		break;

	case UNITAI_DEFENSE_AIR:
	case UNITAI_CARRIER_AIR:
	case UNITAI_MISSILE_AIR:
		if (isFocusWar()) // advc.105
				//GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0)
		{
			iValue *= 5;
		}
		else
		{
			iValue *= 3;
		}
		break;

	default:
		FAssert(false);
		break;
	}

	if (pUnit->getUnitInfo().getExtraCost() > 0)
	{
		iValue /= pUnit->getUnitInfo().getExtraCost() + 1;
	}

	return iValue;
}
// K-Mod end

ReligionTypes CvPlayerAI::AI_chooseReligion()
{
	ReligionTypes eFavorite = (ReligionTypes)GC.getLeaderHeadInfo(getLeaderType()).getFavoriteReligion();
	if (NO_RELIGION != eFavorite && !GC.getGameINLINE().isReligionFounded(eFavorite))
	{
		return eFavorite;
	}

	std::vector<ReligionTypes> aeReligions;
	for (int iReligion = 0; iReligion < GC.getNumReligionInfos(); ++iReligion)
	{
		if (!GC.getGameINLINE().isReligionFounded((ReligionTypes)iReligion))
		{
			aeReligions.push_back((ReligionTypes)iReligion);
		}
	}

	if (aeReligions.size() > 0)
	{
		return aeReligions[GC.getGameINLINE().getSorenRandNum(aeReligions.size(), "AI pick religion")];
	}

	return NO_RELIGION;
}

int CvPlayerAI::AI_getAttitudeWeight(PlayerTypes ePlayer) const
{
	int iAttitudeWeight = 0;
	switch (AI_getAttitude(ePlayer))
	{
	case ATTITUDE_FURIOUS:
		iAttitudeWeight = -100;
		break;
	case ATTITUDE_ANNOYED:
		iAttitudeWeight = -50;
		break;
	case ATTITUDE_CAUTIOUS:
		iAttitudeWeight = 0;
		break;
	case ATTITUDE_PLEASED:
		iAttitudeWeight = 50;
		break;
	case ATTITUDE_FRIENDLY:
		iAttitudeWeight = 100;			
		break;
	}
	
	return iAttitudeWeight;
}

int CvPlayerAI::AI_getPlotAirbaseValue(CvPlot* pPlot) const
{
	PROFILE_FUNC();
	
	FAssert(pPlot != NULL);
	
	if (pPlot->getTeam() != getTeam())
	{
		return 0;
	}
	
	if (pPlot->isCityRadius())
	{
		CvCity* pWorkingCity = pPlot->getWorkingCity();
		if (pWorkingCity != NULL)
		{
			/* original bts code
			if (pWorkingCity->AI_getBestBuild(pWorkingCity->getCityPlotIndex(pPlot)) != NO_BUILD)
			{
				return 0;
			}
			if (pPlot->getImprovementType() != NO_IMPROVEMENT)
			{
				CvImprovementInfo &kImprovementInfo = GC.getImprovementInfo(pPlot->getImprovementType());
				if (!kImprovementInfo.isActsAsCity())
				{
					return 0;
				}
			} */
			// K-Mod
			ImprovementTypes eBestImprovement = pPlot->getImprovementType();
			BuildTypes eBestBuild = pWorkingCity->AI_getBestBuild(pWorkingCity->getCityPlotIndex(pPlot));
			if (eBestBuild != NO_BUILD)
			{
				if (GC.getBuildInfo(eBestBuild).getImprovement() != NO_IMPROVEMENT)
					eBestImprovement = (ImprovementTypes)GC.getBuildInfo(eBestBuild).getImprovement();
			}
			if (eBestImprovement != NO_IMPROVEMENT)
			{
				CvImprovementInfo &kImprovementInfo = GC.getImprovementInfo(eBestImprovement);
				if (!kImprovementInfo.isActsAsCity())
				{
					return 0;
				}
			}
			// K-Mod end
		}
	}
	
	int iMinOtherCityDistance = MAX_INT;
	CvPlot* iMinOtherCityPlot = NULL;
	
	int iMinFriendlyCityDistance = MAX_INT;
	CvPlot* iMinFriendlyCityPlot = NULL;
	
	int iOtherCityCount = 0;
	
	int iRange = 4;
	for (int iX = -iRange; iX <= iRange; iX++)
	{
		for (int iY = -iRange; iY <= iRange; iY++)
		{
			CvPlot* pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iX, iY);
			if ((pLoopPlot != NULL) && (pPlot != pLoopPlot))
			{
				int iDistance = plotDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
				
				if (pLoopPlot->getTeam() == getTeam())
				{
					if (pLoopPlot->isCity(true))
					{
						if (1 == iDistance)
						{
							return 0;
						}
						if (iDistance < iMinFriendlyCityDistance)
						{
							iMinFriendlyCityDistance = iDistance;
							iMinFriendlyCityPlot = pLoopPlot;
						}
					}
				}
				else
				{
					if (pLoopPlot->isOwned())
					{
						if (pLoopPlot->isCity(false))
						{
							if (iDistance < iMinOtherCityDistance)
							{
								iMinOtherCityDistance = iDistance;
								iMinOtherCityPlot = pLoopPlot;
								iOtherCityCount++;
							}
						}
					}
				}
			}
		}
	}
	
	if (0 == iOtherCityCount)
	{
		return 0;
	}
	
//	if (iMinFriendlyCityPlot != NULL)
//	{
//		FAssert(iMinOtherCityPlot != NULL);
//		if (plotDistance(iMinFriendlyCityPlot->getX_INLINE(), iMinFriendlyCityPlot->getY_INLINE(), iMinOtherCityPlot->getX_INLINE(), iMinOtherCityPlot->getY_INLINE()) < (iMinOtherCityDistance - 1))
//		{
//			return 0;
//		}
//	}

//	if (iMinOtherCityPlot != NULL)
//	{
//		CvCity* pNearestCity = GC.getMapINLINE().findCity(iMinOtherCityPlot->getX_INLINE(), iMinOtherCityPlot->getY_INLINE(), NO_PLAYER, getTeam(), false);
//		if (NULL == pNearestCity)
//		{
//			return 0;
//		}
//		if (plotDistance(pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE(), iMinOtherCityPlot->getX_INLINE(), iMinOtherCityPlot->getY_INLINE()) < iRange)
//		{
//			return 0;
//		}
//	}
		
	
	int iDefenseModifier = pPlot->defenseModifier(getTeam(), false);
//	if (iDefenseModifier <= 0)
//	{
//		return 0;
//	}
	
	int iValue = iOtherCityCount * 50;
	iValue *= 100 + (2 * (iDefenseModifier + (pPlot->isHills() ? 25 : 0)));
	iValue /= 100;
	
	return iValue;
}

int CvPlayerAI::AI_getPlotCanalValue(CvPlot* pPlot) const
{
	PROFILE_FUNC();
	
	FAssert(pPlot != NULL);
	
	if (pPlot->isOwned())
	{
		if (pPlot->getTeam() != getTeam())
		{
			return 0;
		}
		if (pPlot->isCityRadius())
		{
			CvCity* pWorkingCity = pPlot->getWorkingCity();
			if (pWorkingCity != NULL)
			{
				/* original bts code
				if (pWorkingCity->AI_getBestBuild(pWorkingCity->getCityPlotIndex(pPlot)) != NO_BUILD)
				{
					return 0;
				}
				if (pPlot->getImprovementType() != NO_IMPROVEMENT)
				{
					CvImprovementInfo &kImprovementInfo = GC.getImprovementInfo(pPlot->getImprovementType());
					if (!kImprovementInfo.isActsAsCity())
					{
						return 0;
					}
				} */
				// K-Mod
				ImprovementTypes eBestImprovement = pPlot->getImprovementType();
				BuildTypes eBestBuild = pWorkingCity->AI_getBestBuild(pWorkingCity->getCityPlotIndex(pPlot));
				if (eBestBuild != NO_BUILD)
				{
					if (GC.getBuildInfo(eBestBuild).getImprovement() != NO_IMPROVEMENT)
						eBestImprovement = (ImprovementTypes)GC.getBuildInfo(eBestBuild).getImprovement();
				}
				if (eBestImprovement != NO_IMPROVEMENT)
				{
					CvImprovementInfo &kImprovementInfo = GC.getImprovementInfo(eBestImprovement);
					if (!kImprovementInfo.isActsAsCity())
					{
						return 0;
					}
				}
				// K-Mod end
			}
		}
	}
	
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		CvPlot* pLoopPlot = plotDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), (DirectionTypes)iI);
		if (pLoopPlot != NULL)
		{
			if (pLoopPlot->isCity(true))
			{
				return 0;
			}
		}
	}
	
	CvArea* pSecondWaterArea = pPlot->secondWaterArea();
	if (pSecondWaterArea == NULL)
	{
		return 0;
	}
	
	//return 10 * std::min(0, pSecondWaterArea->getNumTiles() - 2);
	return 10 * std::max(0, pSecondWaterArea->getNumTiles() - 2);
}

//This returns approximately to the sum
//of the percentage values of each unit (there is no need to scale the output by iHappy)
//100 * iHappy means a high value.
int CvPlayerAI::AI_getHappinessWeight(int iHappy, int iExtraPop, bool bPercent) const
{
	int iWorstHappy = 0;
	int iBestHappy = 0;
	int iTotalUnhappy = 0;
	int iTotalHappy = 0;
	int iLoop;
	CvCity* pLoopCity;
	/*  advc.036: This was counting all our cities except Globe Theater (GT).
		I think they should all count; happiness in the GT city just won't
		provide any value. */
	int iCount = getNumCities();

	if (0 == iHappy)
	{
		iHappy = 1;
	}
	int iValue = 0;
	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		/* original bts code
		int iCityHappy = pLoopCity->happyLevel() - pLoopCity->unhappyLevel(iExtraPop);
		
		iCityHappy -= std::max(0, pLoopCity->getCommerceHappiness());
		int iHappyNow = iCityHappy;
		int iHappyThen = iCityHappy +
			(bPercent ? ROUND_DIVIDE(pLoopCity->getPopulation()*iHappy, 100) : iHappy);
		
		//Integration
		int iTempValue = (((100 * iHappyThen - 10 * iHappyThen * iHappyThen)) - (100 * iHappyNow - 10 * iHappyNow * iHappyNow));
		if (iHappy > 0)
		{
			iValue += std::max(0, iTempValue);
		}
		else
		{
			iValue += std::min(0, iTempValue);
		} */

		// K-Mod. I have no idea what they were trying to 'integrate', and it was giving some strange answers.
		// So lets try it my way.
		if (pLoopCity->isNoUnhappiness())
			continue;
		/*  <advc.036> By the time iExtraPop matters, the espionage effect will
			be gone. */
		if(iExtraPop > 0) {
			iExtraPop = std::max(0, iExtraPop -
					pLoopCity->getEspionageHappinessCounter() / 2);
		} // </advc.036>
		int iCurrentHappy = 100*(pLoopCity->happyLevel() - pLoopCity->unhappyLevel(iExtraPop)
				/*  advc.036: Subtract half the anger from espionage b/c it is
					fleeting */
				+ pLoopCity->getEspionageHappinessCounter() / 2);
		// I'm only going to subtract half of the commerce happiness, because we might not be using that commerce for only happiness.
		iCurrentHappy -= 50*std::max(0, pLoopCity->getCommerceHappiness());
		int iTestHappy = iCurrentHappy +
			(bPercent ? ((pLoopCity->getPopulation()+iExtraPop)*iHappy) : 100 * iHappy);
		iValue += std::max(0, -iCurrentHappy) - std::max(0, -iTestHappy); // change in the number of angry citizens
		// a small bonus for happiness beyond what we need
		iValue += 100*(std::max(0, iTestHappy) - std::max(0, iCurrentHappy))/(400 + std::max(0, iTestHappy) + std::max(0, iCurrentHappy));
		// K-Mod end
		/*  <advc.036> A little extra when unhappiness is very high because that
			may lead to good tiles not getting worked. */
		// Not sure how bPercent works; better exclude that.
		if(iCurrentHappy < 0 && !bPercent)
			iValue += ::round(std::pow(-iCurrentHappy/100 - 1.0 - iExtraPop, 1.5));
		// </advc.036>
		/* original bts code - (huh?)
		if (iCount > 6)
		{
			break;
		}*/
	}
	
	return (0 == iCount) ? 50 * iHappy : iValue / iCount;
}

int CvPlayerAI::AI_getHealthWeight(int iHealth, int iExtraPop, bool bPercent) const
{
	int iWorstHealth = 0;
	int iBestHealth = 0;
	int iTotalUnhappy = 0;
	int iTotalHealth = 0;
	int iLoop;
	CvCity* pLoopCity;
	int iCount = getNumCities(); // advc.036: See AI_getHappinessWeight
	if (0 == iHealth)
	{
		iHealth = 1;
	}
	int iValue = 0;
	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		// original bts code: [...] deleted (advc)
		// <advc.036> See HappinessWeight
		if(iExtraPop > 0) {
			iExtraPop = std::max(0, iExtraPop -
					pLoopCity->getEspionageHealthCounter() / 2);
		} // </advc.036>
		// K-Mod. Changed to match the happiness function.
		int iCurrentHealth = 100*(pLoopCity->goodHealth() - pLoopCity->badHealth(false, iExtraPop)
				/*  advc.036: Subtract half the bad health from espionage b/c
					it is fleeting */
				+ pLoopCity->getEspionageHealthCounter() / 2);
		int iTestHealth = iCurrentHealth +
			(bPercent ? ((pLoopCity->getPopulation()+iExtraPop)*iHealth) : 100 * iHealth);
		iValue += std::max(0, -iCurrentHealth) - std::max(0, -iTestHealth); // change in the number of unhealthy citizens
		// a small bonus for health beyond what we need
		iValue += 100*(std::max(0, iTestHealth) - std::max(0, iCurrentHealth))/(400 + std::max(0, iTestHealth) + std::max(0, iCurrentHealth));
		// K-Mod end
		/*  <advc.036> A little extra when health is very poor because that may
			well lead to population loss and good tiles not getting worked. */
		// Not sure how bPercent works; better exclude that.
		if(iCurrentHealth < 0 && !bPercent)
			iValue += ::round(std::pow(-iCurrentHealth/100 - 1.0 - iExtraPop, 1.5));
		// </advc.036>
		/* original bts code
		if (iCount > 6)
		{
			break;
		} */
	}
	
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       10/21/09                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* orginal bts code
	return (0 == iCount) ? 50 : iValue / iCount;
*/
	// Mirror happiness valuation code
	return (0 == iCount) ? 50*iHealth : iValue / iCount;
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
}
	
/* void CvPlayerAI::AI_invalidateCloseBordersAttitudeCache()
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		m_aiCloseBordersAttitudeCache[i] = MAX_INT;
	}
} */ // disabled by K-Mod

bool CvPlayerAI::AI_isPlotThreatened(CvPlot* pPlot, int iRange, bool bTestMoves) const
{
	PROFILE_FUNC();

	CvArea *pPlotArea = pPlot->area();

	if (iRange == -1)
	{
		iRange = DANGER_RANGE;
	}

	for (int iDX = -iRange; iDX <= iRange; iDX++)
	{
		for (int iDY = -iRange; iDY <= iRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{	// <advc.030>
				CvArea const& fromArea = *pLoopPlot->area();
				if(pPlotArea->canBeEntered(fromArea)) // Replacing:
				//if (pLoopPlot->area() == pPlotArea) // </advc.030>
				{
					for (CLLNode<IDInfo>* pUnitNode = pLoopPlot->headUnitNode(); pUnitNode != NULL; pUnitNode = pLoopPlot->nextUnitNode(pUnitNode))
					{
						CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
						if (pLoopUnit->isEnemy(getTeam()) && pLoopUnit->canAttack() && !pLoopUnit->isInvisible(getTeam(), false))
						{ /* advc.030, advc.001k: Replacing the line below
							 (which calls isMadeAttack) */
							if(pPlotArea->canBeEntered(fromArea, pLoopUnit))
							//if (pLoopUnit->canMoveOrAttackInto(pPlot))
							{
								int iPathTurns = 0;
								if (bTestMoves)
								{
									/* original bts code
									if (!pLoopUnit->getGroup()->generatePath(pLoopPlot, pPlot, MOVE_MAX_MOVES | MOVE_IGNORE_DANGER, false, &iPathTurns))
									{
										iPathTurns = MAX_INT;
									}*/

									// K-Mod. Use a temp pathfinder, so as not to interfere with the normal one.
									KmodPathFinder temp_finder;
									temp_finder.SetSettings(pLoopUnit->getGroup(), MOVE_MAX_MOVES | MOVE_IGNORE_DANGER, 1, GC.getMOVE_DENOMINATOR());
									if (temp_finder.GeneratePath(pPlot))
										iPathTurns = 1;
									// K-Mod end
								}

								if (iPathTurns <= 1)
								{
									return true;
								}
							}
						}
					}
				}
			}
		}
	}

	return false;
}

bool CvPlayerAI::AI_isFirstTech(TechTypes eTech) const
{
	if (eTech == NO_TECH)
		return false; // K-Mod

	for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		if (GC.getReligionInfo((ReligionTypes)iI).getTechPrereq() == eTech)
		{
			if (!(GC.getGameINLINE().isReligionSlotTaken((ReligionTypes)iI)))
			{
				return true;
			}
		}
	}

	if (GC.getGameINLINE().countKnownTechNumTeams(eTech) == 0)
	{
		if ((getTechFreeUnit(eTech) != NO_UNIT) ||
			(GC.getTechInfo(eTech).getFirstFreeTechs() > 0))
		{
			return true;
		}
	}

	return false;
}

// K-Mod
void CvPlayerAI::AI_ClearConstructionValueCache()
{
	CvCity *pLoopCity;
	int iLoop;
	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		// Have I meantioned that the way the "AI" classes are used in this code is an abomination and an insult to OO programming?
		//static_cast<CvCityAI*>(pLoopCity)->AI_ClearConstructionValueCache();
		/*  advc.003: Declared it as a CvCity member. (Lots of these - most likely
			harmless - casts in CvGameTextMgr; I'm leaving those alone.) */
		pLoopCity->AI_ClearConstructionValueCache();
	}
}
// K-Mod end
/*  <k146> Check that we have the required bonuses to train the given unit.
	This isn't for any particular city. It's just a rough guide for whether or
	not we could build the unit. */
bool CvPlayerAI::AI_haveResourcesToTrain(UnitTypes eUnit) const
{
	FASSERT_BOUNDS(0, GC.getNumUnitInfos(), eUnit, "CvPlayer::haveResourcesToTrain");

	const CvUnitInfo& kUnit = GC.getUnitInfo(eUnit);
	//const CvTeam& kTeam = GET_TEAM(getTeam());

	// "and" bonus
	BonusTypes ePrereqBonus = (BonusTypes)kUnit.getPrereqAndBonus();
	if (ePrereqBonus != NO_BONUS)
	{
		if (!hasBonus(ePrereqBonus) && countOwnedBonuses(ePrereqBonus) == 0)
		{
			return false;
		}
	}

	// "or" bonuses
	bool bMissingBonus = false;
	for (int i = 0; i < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); ++i)
	{
		BonusTypes ePrereqBonus = (BonusTypes)kUnit.getPrereqOrBonuses(i);

		if (ePrereqBonus == NO_BONUS)
			continue;

		if (hasBonus(ePrereqBonus) || countOwnedBonuses(ePrereqBonus) > 0)
		{
			bMissingBonus = false;
			break;
		}
		bMissingBonus = true;
	}

	return !bMissingBonus;
}

// </k146>

// <advc.130r><advc.130h>
bool CvPlayerAI::atWarWithPartner(TeamTypes theyId, bool checkPartnerAttacked) const {

	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayer const& partner = GET_PLAYER((PlayerTypes)i);
		if(partner.isAlive() && partner.getTeam() != getTeam() &&
				!partner.isMinorCiv() && GET_TEAM(theyId).isAtWar(
				partner.getTeam()) && AI_getAttitude(partner.getID()) >=
				ATTITUDE_PLEASED) {
			if(!checkPartnerAttacked)
				return true;
			WarPlanTypes wp = GET_TEAM(partner.getTeam()).AI_getWarPlan(theyId);
			if(wp == WARPLAN_ATTACKED || wp == WARPLAN_ATTACKED_RECENT)
				return true;
		}
	}
	return false;
} // </advc.130h></advc.130r>

// <advc.651>
void CvPlayerAI::checkDangerFromSubmarines() {

	dangerFromSubs = false;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayerAI const& warEnemy = GET_PLAYER((PlayerTypes)i);
		if(!warEnemy.isAlive() || warEnemy.isMinorCiv() ||
				!GET_TEAM(warEnemy.getTeam()).isAtWar(getTeam()))
			continue;
		for(int j = 0; j < GC.getNumUnitClassInfos(); j++) {
			UnitTypes ut = (UnitTypes)(GC.getCivilizationInfo(
					getCivilizationType()).getCivilizationUnits(j));
			if(ut == NO_UNIT)
				continue;
			CvUnitInfo const& u = GC.getUnitInfo(ut);
			if(u.getDomainType() == DOMAIN_SEA &&
					u.getInvisibleType() != NO_INVISIBLE &&
					warEnemy.canBeExpectedToTrain(ut)) {
				dangerFromSubs = true;
				return;
			}
		}
	}
}

bool CvPlayerAI::isDangerFromSubmarines() const {

	return dangerFromSubs;
} // </advc.651>

// <advc.104><advc.651>
bool CvPlayerAI::canBeExpectedToTrain(UnitTypes ut) const {

	if(ut == NO_UNIT) {
		FAssert(false);
		return false;
	}
	CvUnitInfo& u = GC.getUnitInfo(ut);
	bool isSeaUnit = (u.getDomainType() == DOMAIN_SEA);
	int minAreaSz = std::max(0, u.getMinAreaSize());
	/* Should be able to build at least two units in ten turns (i.e. one in five);
	   otherwise, the unit probably won't be trained, or just 1 or 2. */
	int targetProduction = ::round(getProductionNeeded(ut) / 5.0);
	int partialSum = 0;
	CvCity* c; int i;
	for(c = firstCity(&i); c != NULL; c = nextCity(&i)) {
		if((isSeaUnit && !c->isCoastal(minAreaSz)) || !c->canTrain(ut))
			continue;
		partialSum += c->getProduction();
		if(partialSum >= targetProduction)
			return true;
	}
	return false;
} // </advc.104></advc.651>

// <advc.300>
bool CvPlayerAI::isDefenseFocusOnBarbarians(int areaId) const {

	CvArea* ap = GC.getMapINLINE().getArea(areaId);
	if(ap == NULL) return false; CvArea const& a = *ap;
	CvGame& g = GC.getGameINLINE();
	return a.getAreaAIType(getTeam()) != AREAAI_DEFENSIVE &&
			getNumCities() > 1 &&
			!AI_isDoStrategy(AI_STRATEGY_ALERT1) && !isFocusWar() &&
			!GC.getGame().isOption(GAMEOPTION_NO_BARBARIANS) &&
			g.getGameTurn() >= g.getBarbarianStartTurn() &&
			g.getCurrentEra() < g.getStartEra() + 2;
} // </advc.300>

/*  <advc.132> This function is going to overestimate the cost of anarchy
	when multiple TRADE_CIVIC and TRADE_RELIGION items are on the table.
	Would have to know the full list of trade items for an accurate cost. */
int CvPlayerAI::anarchyTradeVal(CivicTypes eCivic) const {

	// This omits food surplus and GPP, but also expenses.
	double r = estimateYieldRate(YIELD_COMMERCE) +
			2 * estimateYieldRate(YIELD_PRODUCTION);
	int anarchyLength = -1;
	int switchBackAnarchyLength = -1;
	if(eCivic != NO_CIVIC) {
		CivicTypes* civics = new CivicTypes[GC.getNumCivicOptionInfos()];
		for(int i = 0; i < GC.getNumCivicOptionInfos(); i++)
			civics[i] = getCivics((CivicOptionTypes)i);
		CivicOptionTypes eOption = (CivicOptionTypes)GC.getCivicInfo(eCivic).getCivicOptionType();
		if(eOption != NO_CIVICOPTION)
			civics[eOption] = eCivic;
		else FAssert(eOption != NO_CIVICOPTION);
		anarchyLength = getCivicAnarchyLength(civics);
		switchBackAnarchyLength = getCivicAnarchyLengthBulk(civics, true);
		SAFE_DELETE_ARRAY(civics);
	}
	else {
		anarchyLength = getReligionAnarchyLength();
		switchBackAnarchyLength = getReligionAnarchyLengthBulk(true);
	}
	/*  Uncertain whether we'll want to switch back; fair chance that it can
		be piggybacked on another revolution. */
	r *= (anarchyLength + 0.36 * switchBackAnarchyLength);
	/*  Going to get only half the trade val as gpt. But we're also going to
		benefit from denying gold from a competitor. So far, only humans can
		trade for religion and civics changes, and they're strong competitors. */
	r *= 1.73;
	return ::round(r);
} // </advc.132>

// <advc.104>
WarAndPeaceAI::Civ& CvPlayerAI::warAndPeaceAI() {

	return wpai;
}
WarAndPeaceAI::Civ const& CvPlayerAI::warAndPeaceAI() const {

	return wpai;
}
// </advc.104>
