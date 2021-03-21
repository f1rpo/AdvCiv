#include "CvGameCoreDLL.h"
#include "CvPlayer.h"
#include "CvAgents.h"
#include "CoreAI.h"
#include "CvCityAI.h"
#include "CvUnitAI.h"
#include "CvSelectionGroupAI.h"
#include "CvPlotGroup.h"
#include "CvDeal.h"
#include "CvTalkingHeadMessage.h"
#include "UWAIAgent.h"
#include "PlotRange.h"
#include "CvArea.h"
#include "CvInfo_All.h"
#include "CvDiploParameters.h"
#include "CvPopupInfo.h"
#include "CvGameTextMgr.h"
#include "RiseFall.h"
#include "AdvCiv4lerts.h"
#include "CvBugOptions.h"
#include "CvDLLFlagEntityIFaceBase.h" // BBAI
#include "BBAILog.h"

// advc.003u: Statics moved from CvPlayerAI
CvPlayerAI** CvPlayer::m_aPlayers = NULL;

void CvPlayer::initStatics()
{
	/*  advc.003u (comment): If further concrete classes derived from CvPlayer are
		added, then this function will have to decide which constructor to call. */
	m_aPlayers = new CvPlayerAI*[MAX_PLAYERS];
	for (int i = 0; i < MAX_PLAYERS; i++)
		m_aPlayers[i] = new CvPlayerAI((PlayerTypes)i);
}

void CvPlayer::freeStatics()
{
	for (int i = 0; i < MAX_PLAYERS; i++)
		SAFE_DELETE(m_aPlayers[i]);
	SAFE_DELETE_ARRAY(m_aPlayers);
}


CvPlayer::CvPlayer(/* advc.003u: */ PlayerTypes eID) :
	m_pCivilization(NULL), // advc.003w
	m_aszBonusHelp(NULL) // advc.003p
{
	// advc: redundant
	/*m_bDisableHuman = false; // bbai
	m_iChoosingFreeTechCount = 0;*/ // K-Mod
	reset(eID, true);
}


CvPlayer::~CvPlayer()
{
	uninit();
	SAFE_DELETE(m_pCivilization); // advc.003w
}


void CvPlayer::init(PlayerTypes eID)
{
	reset(eID); // Reset serialized data

	initContainers(); // advc.003q: Moved into a subroutine for initInGame

	setupGraphical();

	// Init team data ...
	FAssert(getTeam() != NO_TEAM);
	GET_TEAM(getTeam()).changeNumMembers(1);
	GET_TEAM(getTeam()).updateMinorCiv(); // advc.003m

	// Init other player data ...
	// <advc.003q> Moved into a subroutine for initInGame
	if(!initOtherData())
		return;
	resetCivTypeEffects(true); // There was already a subroutine for this
	// </advc.003q>

	AI().AI_init();
}

// advc.003q: Cut from CvPlayer::init
void CvPlayer::initContainers()
{
	m_plotGroups.init();
	m_cities.init();
	m_units.init();
	m_selectionGroups.init();
	m_eventsTriggered.init();
	// <advc.004s> For civs that don't start on turn 0
	FOR_EACH_ENUM(PlayerHistory)
	{
		m_playerHistory[eLoopPlayerHistory].grow(GC.getGame().getGameTurn());
	} // </advc.004s>
}

/*  advc.003q: Cut from CvPlayer::init.
	Returns false if data not initialized due to slot status. */
bool CvPlayer::initOtherData()
{
	SlotStatus eStatus = GC.getInitCore().getSlotStatus(getID());
	if (eStatus != SS_TAKEN && eStatus != SS_COMPUTER)
		return false;
	initAlerts(); // advc.210
	setAlive(true);
	changePersonalityType(); // advc.003q: Use BBAI subroutine

	changeBaseFreeUnits(GC.getDefineINT("INITIAL_BASE_FREE_UNITS"));
	changeBaseFreeMilitaryUnits(GC.getDefineINT("INITIAL_BASE_FREE_MILITARY_UNITS"));
	changeFreeUnitsPopulationPercent(GC.getDefineINT("INITIAL_FREE_UNITS_POPULATION_PERCENT"));
	changeFreeMilitaryUnitsPopulationPercent(GC.getDefineINT("INITIAL_FREE_MILITARY_UNITS_POPULATION_PERCENT"));
	changeGoldPerUnit(
			/*	kekm.14: (advc: had been implemented in getGoldPerUnit,
				getGoldPerMilitaryUnit; the latter seems entirely unnecessary.) */
			isBarbarian() ? 0 :
			GC.getDefineINT("INITIAL_GOLD_PER_UNIT"));
	changeTradeRoutes(GC.getDefineINT("INITIAL_TRADE_ROUTES"));
	changeStateReligionHappiness(GC.getDefineINT("INITIAL_STATE_RELIGION_HAPPINESS"));
	changeNonStateReligionHappiness(GC.getDefineINT("INITIAL_NON_STATE_RELIGION_HAPPINESS"));

	FOR_EACH_ENUM2(Yield, eYield)
		changeTradeYieldModifier(eYield, GC.getInfo(eYield).getTradeModifier());

	FOR_EACH_ENUM2(Commerce, eCommerce)
		setCommercePercent(eCommerce, GC.getInfo(eCommerce).getInitialPercent(), true);

	/*  advc.003q: Moved into a (sub-)subroutine - except for the setCivics code;
		that's handled (only) by resetCivTypeEffects. */
	processTraits(1);
	return true;
}


/*  BETTER_BTS_AI_MOD, 12/30/08, jdog5000: START
	(copy of CvPlayer::init but with modifications for use in the middle of a game) */
void CvPlayer::initInGame(PlayerTypes eID)
{
	reset(eID);
	initContainers(); // advc.003q: New subroutine to avoid code duplication
	setupGraphical();

	// BBAI: Some effects on team necessary if this is the only member of the team
	int iOtherTeamMembers = 0;
	for (int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		if (i != getID() && TEAMID((PlayerTypes)i) == getTeam())
			iOtherTeamMembers++;
	}
	bool bTeamInit = false;
	if (iOtherTeamMembers == 0 || GET_TEAM(getTeam()).getNumMembers() == 0)
	{
		bTeamInit = true;
		GET_TEAM(getTeam()).init(getTeam());
		GET_TEAM(getTeam()).resetPlotAndCityData();
		// advc.158:
		GET_TEAM(getTeam()).AI_strengthMemory().init(GC.getMap().numPlots(), getTeam());
	}
	if (bTeamInit || (GET_TEAM(getTeam()).getNumMembers() == iOtherTeamMembers))
	{
		GET_TEAM(getTeam()).changeNumMembers(1);
	}
	// <advc.003q>
	// End of BBAI team effects
	if(!initOtherData()) // New subroutine to avoid code duplication
		return;

	GC.getAgents().colonyCreated(getID()); // advc.agent
	// <advc.104r>
	if(getUWAI.isEnabled())
		getUWAI.initNewCivInGame(getID());
	// </advc.104r>
	/*  I've kept the initialization of random event data out of initOtherData
		b/c the BBAI code handles that part differently (cf. resetCivTypeEffects). */
	// </advc.003q>
	// Reset all triggers at first, set those whose events have fired in next block
	FOR_EACH_ENUM(EventTrigger)
		resetTriggerFired(eLoopEventTrigger);

	FOR_EACH_ENUM(Event)
	{
		//resetEventOccured((EventTypes)iI, false); // BtS
		EventTriggeredData const* pEvent = NULL; // Has global trigger fired already?
		for (int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			if (i == getID())
				continue;

			pEvent = GET_PLAYER((PlayerTypes)i).getEventOccured(eLoopEvent);
			if (pEvent == NULL)
				continue;

			CvEventTriggerInfo& kTrigger = GC.getInfo(pEvent->m_eTrigger);
			if (kTrigger.isGlobal())
			{
				setTriggerFired(*pEvent, false, false);
				break;
			}
			else if (kTrigger.isTeam() && TEAMID((PlayerTypes)i) == getTeam())
			{
				setTriggerFired(*pEvent, false, false);
				break;
			}
		}
		resetEventOccured(eLoopEvent, false);
	}

	FOR_EACH_ENUM2(CivicOption, eCivicOption)
		setCivics(eCivicOption, getCivilization().getInitialCivic(eCivicOption));

	resetPlotAndCityData(); // BBAI
	AI().AI_init();
}

// <advc.210>
void CvPlayer::initAlerts(bool bSilentCheck)
{
	if (!isHuman())
		return;
	if (!m_paAlerts.empty())
	{
		FAssertMsg(!isAlive() && isHumanDisabled(), "initAlerts called redundantly "
				"(OK upon defeat of active player during Auto Play)");
		uninitAlerts();
	}
	/*  The order of this array needs to correspond to the ids returned
		by the AdvCiv4lers.getID functions in Civ4lerts.py */
	m_paAlerts.push_back(new WarTradeAlert(getID())); // advc.210a
	m_paAlerts.push_back(new RevoltAlert(getID())); // advc.210b
	m_paAlerts.push_back(new BonusThirdPartiesAlert(getID())); // advc.210d
	m_paAlerts.push_back(new CityTradeAlert(getID())); // advc.ctr
	if (bSilentCheck)
	{
		for (size_t i = 0; i < m_paAlerts.size(); i++)
			checkAlert(i, true);
	}
}


void CvPlayer::uninitAlerts()
{
	for(size_t i = 0; i < m_paAlerts.size(); i++)
		SAFE_DELETE(m_paAlerts[i]);
	m_paAlerts.clear();
} // </advc.210>

// Reset all data for this player stored in plot and city objects
void CvPlayer::resetPlotAndCityData()
{
	CvMap const& kMap = GC.getMap();
	for (int iPlot = 0; iPlot < kMap.numPlots(); ++iPlot)
	{
		CvPlot& kPlot = kMap.getPlotByIndex(iPlot);

		kPlot.setCulture(getID(), 0, false, false);
		kPlot.setFoundValue(getID(), 0);

		CvCity* pLoopCity = kPlot.getPlotCity();
		if (pLoopCity != NULL)
		{
			pLoopCity->setCulture(getID(), 0, false, false);
			pLoopCity->changeNumRevolts(getID(), -pLoopCity->getNumRevolts(getID()));
			pLoopCity->setEverOwned(getID(), false);
			pLoopCity->setTradeRoute(getID(), false);
		}
	}
}
// BETTER_BTS_AI_MOD: END

void CvPlayer::uninit()
{
	m_triggersFired.clear();

	// <advc.003p>
	if(m_aszBonusHelp != NULL)
	{
		for(int i = 0; i < GC.getNumBonusInfos(); i++)
			SAFE_DELETE(m_aszBonusHelp[i]);
		SAFE_DELETE_ARRAY(m_aszBonusHelp);
	} // </advc.003p>

	m_groupCycle.clear();
	m_researchQueue.clear();
	m_cityNames.clear();

	m_plotGroups.uninit();
	m_cities.uninit();
	m_units.uninit();
	m_selectionGroups.uninit();
	m_eventsTriggered.uninit();

	clearMessages();
	clearPopups();
	clearDiplomacy();
	uninitAlerts(); // advc.210
}

// Initialize data members that are serialized.
void CvPlayer::reset(PlayerTypes eID, bool bConstructorCall)
{
	uninit();

	m_iTotalPopulation = 0;
	m_iTotalLand = 0;
	m_iTotalLandScored = 0;
	m_iGold = 0;
	m_iGoldPerTurn = 0;
	m_iAdvancedStartPoints = -1;
	m_iGoldenAgeTurns = 0;
	m_iScheduledGoldenAges = 0; // advc.001x
	m_iNumUnitGoldenAges = 0;
	m_iStrikeTurns = 0;
	m_iAnarchyTurns = 0;
	m_iMaxAnarchyTurns = 0;
	m_iAnarchyModifier = 0;
	m_iGoldenAgeModifier = 0;
	m_iGlobalHurryModifier = 0;
	m_iGreatPeopleCreated = 0;
	m_iGreatGeneralsCreated = 0;
	m_iGreatPeopleThresholdModifier = 0;
	m_iGreatGeneralsThresholdModifier = 0;
	m_iGreatPeopleRateModifier = 0;
	m_iGreatGeneralRateModifier = 0;
	m_iDomesticGreatGeneralRateModifier = 0;
	m_iStateReligionGreatPeopleRateModifier = 0;
	m_iMaxGlobalBuildingProductionModifier = 0;
	m_iMaxTeamBuildingProductionModifier = 0;
	m_iMaxPlayerBuildingProductionModifier = 0;
	m_iFreeExperience = 0;
	m_iFeatureProductionModifier = 0;
	m_iWorkerSpeedModifier = 0;
	m_iImprovementUpgradeRateModifier = 0;
	m_iMilitaryProductionModifier = 0;
	m_iSpaceProductionModifier = 0;
	m_iCityDefenseModifier = 0;
	m_iNumNukeUnits = 0;
	m_iNumOutsideUnits = 0;
	m_iBaseFreeUnits = 0;
	m_iBaseFreeMilitaryUnits = 0;
	m_iFreeUnitsPopulationPercent = 0;
	m_iFreeMilitaryUnitsPopulationPercent = 0;
	m_iGoldPerUnit = 0;
	m_iGoldPerMilitaryUnit = 0;
	m_iExtraUnitCost = 0;
	m_iNumMilitaryUnits = 0;
	m_iHappyPerMilitaryUnit = 0;
	m_iLuxuryModifier = 0; // advc.912c
	m_iMilitaryFoodProductionCount = 0;
	m_iConscriptCount = 0;
	m_iMaxConscript = 0;
	m_iHighestUnitLevel = 1;
	m_iOverflowResearch = 0;
	//m_iNoUnhealthyPopulationCount = 0;
	m_iUnhealthyPopulationModifier = 0; // K-Mod
	m_iExpInBorderModifier = 0;
	m_iBuildingOnlyHealthyCount = 0;
	m_iDistanceMaintenanceModifier = 0;
	m_iNumCitiesMaintenanceModifier = 0;
	m_iCorporationMaintenanceModifier = 0;
	m_iTotalMaintenance = 0;
	m_iUpkeepModifier = 0;
	m_iLevelExperienceModifier = 0;
	m_iExtraHealth = 0;
	m_iBuildingGoodHealth = 0;
	m_iBuildingBadHealth = 0;
	m_iExtraHappiness = 0;
	m_iBuildingHappiness = 0;
	m_iLargestCityHappiness = 0;
	m_iWarWearinessPercentAnger = 0;
	m_iWarWearinessModifier = 0;
	m_iGwPercentAnger = 0; // K-Mod
	m_iFreeSpecialist = 0;
	m_iNoForeignTradeCount = 0;
	m_iNoCorporationsCount = 0;
	m_iNoForeignCorporationsCount = 0;
	m_iCoastalTradeRoutes = 0;
	m_iTradeRoutes = 0;
	m_iRevolutionTimer = 0;
	m_iConversionTimer = 0;
	m_iStateReligionCount = 0;
	m_iNoNonStateReligionSpreadCount = 0;
	m_iStateReligionHappiness = 0;
	m_iNonStateReligionHappiness = 0;
	m_iStateReligionUnitProductionModifier = 0;
	m_iStateReligionBuildingProductionModifier = 0;
	m_iStateReligionFreeExperience = 0;
	m_iCapitalCityID = FFreeList::INVALID_INDEX;
	m_iCitiesLost = 0;
	m_iWinsVsBarbs = 0;
	m_iAssets = 0;
	m_iPower = 0;
	m_iPopulationScore = 0;
	m_iLandScore = 0;
	m_iTechScore = 0;
	m_iWondersScore = 0;
	m_iCombatExperience = 0;
	m_iPopRushHurryCount = 0;
	m_iGoldRushHurryCount = 0; // advc.064b
	m_iInflationModifier = 0;
	m_iChoosingFreeTechCount = 0; // K-Mod
	m_iNewMessages = 0; // advc.106b
	m_iButtonPopupsRelaunching = 0; // advc.004x
	m_uiStartTime = 0;
	m_eReminderPending = NO_CIVIC; // advc.004x

	m_bAlive = false;
	m_bEverAlive = false;
	m_bTurnActive = false;
	m_bAutoMoves = false;
	m_bEndTurn = false;
	m_bPbemNewTurn = false;
	m_bExtendedGame = false;
	m_bFoundedFirstCity = false;
	m_bAnyGPPEver = false; // advc.078
	m_bStrike = false;
	m_bDisableHuman = false; // bbai
	m_bAutoPlayJustEnded = false; // advc.127
	m_bSavingReplay = false; // advc.106i
	m_bScoreboardExpanded = false; // advc.085
	m_bRandomWBStart = false; // advc.027

	m_eID = eID;
	updateTeamType();
	updateHuman();
	if (m_eID != NO_PLAYER)
		m_ePersonalityType = GC.getInitCore().getLeader(m_eID); //??? Is this repeated data???
	else m_ePersonalityType = NO_LEADER;
	m_eCurrentEra = (EraTypes)0;  //??? Is this repeated data???
	m_eLastStateReligion = NO_RELIGION;
	m_eParent = NO_PLAYER;
	m_pStartingPlot = NULL; // advc.027

	m_szScriptData = "";

	m_aiSeaPlotYield.reset();
	m_aiYieldRateModifier.reset();
	m_aiCapitalYieldRateModifier.reset();
	m_aiExtraYieldThreshold.reset();
	m_aiTradeYieldModifier.reset();
	m_aiFreeCityCommerce.reset();
	m_aiCommercePercent.reset();
	m_aiCommerceRate.reset();
	m_aiCommerceRateModifier.reset();
	m_aiCapitalCommerceRateModifier.reset();
	m_aiStateReligionBuildingCommerce.reset();
	m_aiSpecialistExtraCommerce.reset();
	m_aiCommerceFlexibleCount.reset();
	m_aiGoldPerTurnByPlayer.reset();
	if (!bConstructorCall && getID() != NO_PLAYER)
	{
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			GET_PLAYER((PlayerTypes)i).m_aiGoldPerTurnByPlayer.reset(getID());
			GET_PLAYER((PlayerTypes)i).m_abEverSeenDemographics.reset(getID()); // advc.091
		}
	}
	m_aiEspionageSpendingWeightAgainstTeam.reset();
	if (!bConstructorCall && getTeam() != NO_TEAM)
	{
		for (int i = 0; i < MAX_PLAYERS; i++)
		{	/*	advc.120: Was setEspionageSpendingWeightAgainstTeam. Don't want to repeat
				the default value here. */
			GET_PLAYER((PlayerTypes)i).m_aiEspionageSpendingWeightAgainstTeam.
					reset(getTeam());
		}
	}
	m_aiBonusExport.reset();
	m_aiBonusImport.reset();
	m_aiImprovementCount.reset();
	m_aiFreeBuildingCount.reset();
	m_aiExtraBuildingHappiness.reset();
	m_aiExtraBuildingHealth.reset();
	m_aiFeatureHappiness.reset();
	m_aiUnitClassCount.reset();
	m_aiUnitClassMaking.reset();
	m_aiBuildingClassCount.reset();
	m_aiBuildingClassMaking.reset();
	m_aiHurryCount.reset();
	m_aiSpecialBuildingNotRequiredCount.reset();
	m_aiHasCivicOptionCount.reset();
	m_aiNoCivicUpkeepCount.reset();
	m_aiHasReligionCount.reset();
	m_aiHasCorporationCount.reset();
	m_aiUpkeepCount.reset();
	m_aiSpecialistValidCount.reset();
	m_aeCivics.reset();
	m_abFeatAccomplished.reset();
	m_abOptions.reset();
	m_abResearchingTech.reset();
	m_abEverSeenDemographics.reset(); // advc.091
	m_abLoyalMember.reset();
	m_aaeSpecialistExtraYield.reset();
	m_aaeImprovementYieldChange.reset();

	if (!bConstructorCall)
	{
		// <advc.003p>
		FAssert(m_aszBonusHelp == NULL);
		m_aszBonusHelp = new CvWString*[GC.getNumBonusInfos()];
		for(int i = 0; i < GC.getNumBonusInfos(); i++)
			m_aszBonusHelp[i] = NULL;
		// </advc.003p>  <advc.004s>
		FOR_EACH_ENUM(PlayerHistory)
		{
			m_playerHistory[eLoopPlayerHistory].reset(getID(),
					eLoopPlayerHistory == PLAYER_HISTORY_SCORE ? 0 : 3);
		} // </advc.004s>
		m_mapEventsOccured.clear();
		m_mapEventCountdown.clear();
		m_aFreeUnitCombatPromotions.clear();
		m_aFreeUnitClassPromotions.clear();
		m_aVote.clear();
		m_aUnitExtraCosts.clear();
		m_triggersFired.clear();
		// <advc.106b>
		for(size_t i = 0; i < m_aMajorMsgs.size(); i++)
			SAFE_DELETE(m_aMajorMsgs[i]);
		m_aMajorMsgs.clear(); // </advc.106b>
	}

	m_plotGroups.removeAll();
	m_cities.removeAll();
	m_units.removeAll();
	m_selectionGroups.removeAll();
	m_eventsTriggered.removeAll();

	if (!bConstructorCall)
		AI().AI_reset(false);
}


// CHANGE_PLAYER, 08/17/08, jdog5000: START
/*	For stripping obsolete trait bonuses;
	use in conjunction with addTraitBonuses for a complete reset. */
//void CvPlayer::clearTraitBonuses() { ... }
//void CvPlayer::addTraitBonuses() { ... }
// advc.003q: Combined into a single function, which CvPlayer::init can call as well.
void CvPlayer::processTraits(int iChange)
{
	FAssert(GC.getNumTraitInfos() > 0);
	FOR_EACH_ENUM2(Trait, eTrait)
	{
		if (!hasTrait(eTrait))
			continue;
		CvTraitInfo const& kTrait = GC.getInfo(eTrait);

		changeExtraHealth(iChange * kTrait.getHealth());
		changeExtraHappiness(iChange * kTrait.getHappiness());

		FOR_EACH_ENUM(Building)
		{
			changeExtraBuildingHappiness(eLoopBuilding,
					iChange * GC.getInfo(eLoopBuilding).getHappinessTraits(eTrait));
		}

		changeUpkeepModifier(iChange * kTrait.getUpkeepModifier());
		changeLevelExperienceModifier(iChange * kTrait.getLevelExperienceModifier());
		changeGreatPeopleRateModifier(iChange * kTrait.getGreatPeopleRateModifier());
		changeGreatGeneralRateModifier(iChange * kTrait.getGreatGeneralRateModifier());
		changeDomesticGreatGeneralRateModifier(iChange * kTrait.getDomesticGreatGeneralRateModifier());

		changeMaxGlobalBuildingProductionModifier(iChange * kTrait.getMaxGlobalBuildingProductionModifier());
		changeMaxTeamBuildingProductionModifier(iChange * kTrait.getMaxTeamBuildingProductionModifier());
		changeMaxPlayerBuildingProductionModifier(iChange * kTrait.getMaxPlayerBuildingProductionModifier());

		FOR_EACH_ENUM(Yield)
		{
			changeTradeYieldModifier(eLoopYield,
					iChange * kTrait.getTradeYieldModifier(eLoopYield));
		}
		FOR_EACH_ENUM(Commerce)
		{
			changeFreeCityCommerce(eLoopCommerce,
					iChange * kTrait.getCommerceChange(eLoopCommerce));
			changeCommerceRateModifier(eLoopCommerce,
					iChange * kTrait.getCommerceModifier(eLoopCommerce));
		}
		FOR_EACH_ENUM(CivicOption)
		{
			if (GC.getInfo(eLoopCivicOption).getTraitNoUpkeep(eTrait))
				changeNoCivicUpkeepCount(eLoopCivicOption, iChange);
		}
	}

	/*  advc.003q: BBAI had only put the update calls into addTraitBonuses,
		but I reckon that they're also needed when clearing traits. And the
		setUnitExtraCost update was only in resetCivTypeEffects, but is probably
		needed for the Imperialistic trait. */

	updateMaxAnarchyTurns();

	FOR_EACH_ENUM(Yield)
		updateExtraYieldThreshold(eLoopYield);

	// advc.003q: setCivics code removed; a change in traits should not reset civics.

	CvCivilization const& kCiv = getCivilization(); // advc.003w
	for (int i = 0; i < kCiv.getNumUnits(); i++)
	{
		UnitTypes eUnit = kCiv.unitAt(i);
		if (GC.getInfo(eUnit).isFound())
			setUnitExtraCost(kCiv.unitClass(eUnit), getNewCityProductionValue());
	}
}

/*	advc (note): The Change Player mod has copied this from CvPlayer::init.
	I've deleted the code there, so now it has effectively been moved from init.*/
void CvPlayer::changePersonalityType()
{
	if (isBarbarian())
		return;

	if (!GC.getGame().isOption(GAMEOPTION_RANDOM_PERSONALITIES))
	{
		setPersonalityType(getLeaderType());
		return;
	}

	int iBestValue = 0;
	LeaderHeadTypes eBestPersonality = NO_LEADER;
	int const iBARBARIAN_LEADER = GC.getDefineINT("BARBARIAN_LEADER"); // advc.opt
	FOR_EACH_ENUM2(LeaderHead, eLoopPersonality)
	{
		if (eLoopPersonality == iBARBARIAN_LEADER) // XXX minor civ???
			continue;

		int iValue = (1 + GC.getGame().getSorenRandNum(10000, "Choosing Personality"));
		for (int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			if (GET_PLAYER((PlayerTypes)i).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)i).getPersonalityType() == eLoopPersonality)
					iValue /= 2;
			}
		}
		if (iValue > iBestValue)
		{
			iBestValue = iValue;
			eBestPersonality = eLoopPersonality;
		}
	}
	if (eBestPersonality != NO_LEADER)
		setPersonalityType(eBestPersonality);
}

// Reset state of event logic, unit prices.
void CvPlayer::resetCivTypeEffects(/* advc.003q: */ bool bInit)
{
	CvCivilization const& kCiv = getCivilization(); // advc.003w
	if (/* <advc.003q> */ bInit /* </advc.003q> */ || !isAlive())
	{
		FOR_EACH_ENUM(CivicOption)
			setCivics(eLoopCivicOption, kCiv.getInitialCivic(eLoopCivicOption));

		FOR_EACH_ENUM(Event)
			resetEventOccured(eLoopEvent, false);

		FOR_EACH_ENUM2(EventTrigger, eEventTrigger)
		{
			if(bInit || // advc.003q
				(!GC.getInfo(eEventTrigger).isGlobal() &&
				(!GC.getInfo(eEventTrigger).isTeam() ||
				GET_TEAM(getTeam()).getNumMembers() == 1)))
			{
				resetTriggerFired(eEventTrigger);
			}
		}
	}
	for (int i = 0; i < kCiv.getNumUnits(); i++)
	{
		UnitTypes eUnit = kCiv.unitAt(i);
		if (GC.getInfo(eUnit).isFound())
			setUnitExtraCost(kCiv.unitClass(eUnit), getNewCityProductionValue());
	}
}

// for switching the leaderhead of this player
void CvPlayer::changeLeader(LeaderHeadTypes eNewLeader)
{
	LeaderHeadTypes const eOldLeader = getLeaderType();

	if (eOldLeader == eNewLeader)
		return;

	//clearTraitBonuses(); // Clear old traits
	processTraits(-1); // advc.003q
	GC.getInitCore().setLeader(getID(), eNewLeader);
	// addTraitBonuses(); // Add new traits
	processTraits(1); // advc.003q

	// Set new personality
	changePersonalityType();

	if (isAlive() || isEverAlive())
	{
		gDLL->UI().setDirty(HighlightPlot_DIRTY_BIT, true);
		gDLL->UI().setDirty(CityInfo_DIRTY_BIT, true);
		gDLL->UI().setDirty(UnitInfo_DIRTY_BIT, true);
		gDLL->UI().setDirty(InfoPane_DIRTY_BIT, true);
		gDLL->UI().setDirty(Flag_DIRTY_BIT, true);
		gDLL->UI().setDirty(MinimapSection_DIRTY_BIT, true);
		gDLL->UI().setDirty(Score_DIRTY_BIT, true);
		gDLL->UI().setDirty(Foreign_Screen_DIRTY_BIT, true);
	}
	/*  advc.104: Re-initializing the AI here was probably always a bad idea,
		but definitely mustn't re-initialize UWAI. Instead call AI_updatePersonality
		from setPersonalityType. */
	//AI().AI_init();
}

// (advc: Moved up so that all the CHANGE_PLAYER code dated 08/17/08 is in one place)
// for changing whether this player is human or not
void CvPlayer::setIsHuman(bool bNewValue, /* advc.127c: */ bool bUpdateAI)
{
	// <advc.706> Make sure that these are consistent
	if(!bNewValue)
		m_bDisableHuman = false; // </advc.706>
	if (bNewValue == isHuman())
		return;

	GC.getInitCore().setSlotStatus(getID(), bNewValue ? SS_TAKEN :
			SS_COMPUTER); // or SS_OPEN for multiplayer?
	// advc.opt (needs to be done before checking the alerts)
	GET_TEAM(getTeam()).updateLeaderID();
	// <advc.210> Only human players need alerts
	if (bNewValue)
	{
		/*  Tbd. (advc.106c): The Python Civ4lerts should also do a silent check
			here; to avoid alerts right after assuming control of the player. */
		initAlerts(true);
	}
	else uninitAlerts(); // </advc.210>
	if (bUpdateAI)
		AI().AI_setHuman(bNewValue);
}
// CHANGE_PLAYER: END
// CHANGE_PLAYER, 05/09/09, jdog5000: START
// for changing the civilization of this player
void CvPlayer::changeCiv(CivilizationTypes eNewCiv)
{
	CivilizationTypes eOldCiv = getCivilizationType();
	if (eOldCiv == eNewCiv)
		return;

	PlayerColorTypes eColor = (PlayerColorTypes)GC.getInfo(eNewCiv).getDefaultPlayerColor();
	PlayerColorTypes const eBarbarianColor = (PlayerColorTypes)GC.getInfo((CivilizationTypes)
			GC.getDefineINT("BARBARIAN_CIVILIZATION")).getDefaultPlayerColor();
	for (int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayer const& kPlayer = GET_PLAYER((PlayerTypes)i);
		if (eColor == NO_PLAYERCOLOR || (kPlayer.getPlayerColor() == eColor &&
			kPlayer.getID() != getID()))
		{
			FOR_EACH_ENUM(PlayerColor)
			{
				if (eLoopPlayerColor == eBarbarianColor)
					continue;

				bool bValid = true;
				for (int j = 0; j < MAX_CIV_PLAYERS; j++)
				{
					if (GET_PLAYER((PlayerTypes)j).getPlayerColor() == eLoopPlayerColor)
					{
						bValid = false;
						break;
					}
				}
				if (bValid)
				{
					eColor = eLoopPlayerColor;
					i = MAX_CIV_PLAYERS;
					break;
				}
			}
		}
	}

	CvInitCore& kInitCore = GC.getInitCore();
	kInitCore.setCiv(getID(), eNewCiv);
	kInitCore.setColor(getID(), eColor);
	setCivilization(eNewCiv); // advc.003u
	resetCivTypeEffects(/* advc.003q: */ false);
	CvDLLInterfaceIFaceBase& kUI = *gDLL->getInterfaceIFace();
	if (isAlive()) // if the player is alive and showing on scoreboard, etc
	{
		// change colors, graphics, flags, units
		kInitCore.setArtStyle(getID(), (ArtStyleTypes)GC.getInfo(eNewCiv).getArtStyleType());
		// advc.127c: The new setFlagDecal function can handle the flag update
		setFlagDecal(GC.getInfo(eNewCiv).getFlagTexture(), true);
		/*kInitCore.setFlagDecal(getID(), GC.getInfo(eNewCiv).getFlagTexture());
		// Force update of units flags
		bool bAuto = m_bDisableHuman; m_bDisableHuman = true;
		EraTypes eEra = getCurrentEra();
		setCurrentEra((EraTypes)(eEra + (eEra == 0 ? 1 : -1))); setCurrentEra(eEra);
		m_bDisableHuman = bAuto;
		kUI.makeInterfaceDirty(); kUI.setDirty(Flag_DIRTY_BIT, true);*/

		// dirty all of this player's cities...
		FOR_EACH_CITY_VAR(pLoopCity, *this)
			pLoopCity->setLayoutDirty(true);

		//update unit eras
		FOR_EACH_UNIT_VAR(pLoopUnit, *this)
		{
			pLoopUnit->reloadEntity();
			// (advc: Deleted flag update code that wasn't working and had already been commented out)
		}

		if (getID() == GC.getGame().getActivePlayer())
			kUI.setDirty(Soundtrack_DIRTY_BIT, true);
		kUI.makeInterfaceDirty();

		// Need to force redraw
		CvDLLEngineIFaceBase& kEngine = *gDLL->getEngineIFace();
		kEngine.SetDirty(CultureBorders_DIRTY_BIT, true);
		kEngine.SetDirty(MinimapTexture_DIRTY_BIT, true);
		kEngine.SetDirty(GlobeTexture_DIRTY_BIT, true);
		kEngine.SetDirty(GlobePartialTexture_DIRTY_BIT, true);

		kUI.setDirty(ColoredPlots_DIRTY_BIT, true);
		kUI.setDirty(HighlightPlot_DIRTY_BIT, true);
		kUI.setDirty(CityInfo_DIRTY_BIT, true);
		kUI.setDirty(UnitInfo_DIRTY_BIT, true);
		kUI.setDirty(InfoPane_DIRTY_BIT, true);
		kUI.setDirty(GlobeLayer_DIRTY_BIT, true);
		// <advc.003p>
		if(getID() == GC.getGame().getActivePlayer())
			setBonusHelpDirty(); // </advc.003p>
		kUI.setDirty(MinimapSection_DIRTY_BIT, true);
		kEngine.SetDirty(MinimapTexture_DIRTY_BIT, true);
		kUI.setDirty(Score_DIRTY_BIT, true);
		kUI.setDirty(Foreign_Screen_DIRTY_BIT, true);
		kUI.setDirty(SelectionSound_DIRTY_BIT, true);
		kUI.setDirty(GlobeInfo_DIRTY_BIT, true);
	}
	else if (isEverAlive())
	{
		// Not currently alive, but may show on some people's scoreboard or graphs
		// change colors
		kUI.setDirty(InfoPane_DIRTY_BIT, true);
		kUI.setDirty(Score_DIRTY_BIT, true);
	}
	setupGraphical();
} // CHANGE_PLAYER: END


void CvPlayer::setupGraphical() // graphical only setup
{
	if (!GC.IsGraphicsInitialized())
		return;

	// Set up m_cities
	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->setupGraphical();

	// Set up m_units
	FOR_EACH_UNIT_VAR(pLoopUnit, *this)
		pLoopUnit->setupGraphical();
}


void CvPlayer::initFreeState()
{
	setGold(0);

	if(!GC.getGame().isOption(GAMEOPTION_ADVANCED_START)) // advc.250c
		changeGold(GC.getInfo(getHandicapType()).getStartingGold());
	changeGold(GC.getInfo(GC.getGame().getStartEra()).getStartingGold());

	clearResearchQueue();
}


void CvPlayer::initFreeUnits()
{
	// <advc>
	CvGame const& kGame = GC.getGame();
	int const iStartingUnitMultiplier = GC.getInfo(kGame.getStartEra()).
			getStartingUnitMultiplier(); // </advc>
	// <advc.027> Goody huts no longer block starting sites
	CvPlot* pStartingPlot = getStartingPlot();
	if (pStartingPlot != NULL && !kGame.isScenario())
	{
		if (pStartingPlot->isGoody())
			pStartingPlot->setImprovementType(NO_IMPROVEMENT);
	}
	else FAssertMsg(pStartingPlot != NULL, "Player in scenario has no starting plot");
	// </advc.027>
	if (kGame.isOption(GAMEOPTION_ADVANCED_START) &&
		(!isHuman() || !kGame.isOption(GAMEOPTION_SPAH))) // advc.250b
	{
		int iPoints = kGame.getNumAdvancedStartPoints();
		// advc.250b (comment): Disabled through Handicap XML
		iPoints *= GC.getInfo(getHandicapType()).getAdvancedStartPointsMod();
		iPoints /= 100;

		if (!isHuman() /* advc.250b: */ && !kGame.isOption(GAMEOPTION_SPAH))
		{	/*  advc.250b, advc.001: Was this->getHandicapType(), i.e. Noble, which
				means that this code block did nothing. */
			iPoints *= GC.getInfo(kGame.getHandicapType()).getAIAdvancedStartPercent();
			iPoints /= 100;
		}
		/*  <advc.250c> Civs in Advanced Start can place a city even if they don't
			have enough start points, but I prefer to enforce a minimum. */
		if (iPoints > 0)
		{
			int iMinPoints = GC.getInitCore().getAdvancedStartMinPoints();
			if(iPoints < iMinPoints)
				iPoints = iMinPoints;
		} // </advc.250c>
		setAdvancedStartPoints(iPoints);

		// Starting visibility
		if (pStartingPlot != NULL)
		{
			/*  advc.108: BtS code moved into a new function (b/c I need the same
				behavior elsewhere). */
			GET_TEAM(getID()).revealSurroundingPlots(*pStartingPlot,
					GC.getDefineINT("ADVANCED_START_SIGHT_RANGE"));
		}
	}
	else
	{
		CvCivilization const& kCiv = getCivilization(); // advc.003w
		for (int i = 0; i < kCiv.getNumUnits(); i++)
		{
			UnitTypes eLoopUnit = kCiv.unitAt(i);
			int iFreeCount = kCiv.getNumFreeUnits(eLoopUnit);
			iFreeCount *= (iStartingUnitMultiplier + (!isHuman() ?
					GC.getInfo(kGame.getHandicapType()).
					getAIStartingUnitMultiplier() : 0));
			for (int iJ = 0; iJ < iFreeCount; iJ++)
				addFreeUnit(eLoopUnit);
		}
		int iFreeCount = GC.getInfo(kGame.getStartEra()).getStartingDefenseUnits();
		iFreeCount += GC.getInfo(getHandicapType()).getStartingDefenseUnits();
		// <advc.126>
		CvHandicapInfo const& kGameHandicap = GC.getInfo(kGame.getHandicapType());
		int iFreeAIDefenders = 0;
		// </advc.126>
		if (!isHuman())
		{   // <advc.126>
			iFreeAIDefenders = kGameHandicap.getAIStartingDefenseUnits();
			if(iFreeAIDefenders > 0)
				iFreeAIDefenders += kGame.getStartEra();
			iFreeCount += iFreeAIDefenders;
			// <advc.126>
		}

		if(iFreeCount > 0)
			addFreeUnitAI(UNITAI_CITY_DEFENSE, iFreeCount);

		iFreeCount = GC.getInfo(kGame.getStartEra()).getStartingWorkerUnits();
		iFreeCount += GC.getInfo(getHandicapType()).getStartingWorkerUnits();

		if (!isHuman())
		{   // <advc.126>
			int iFreeAIWorkers = kGameHandicap.getAIStartingWorkerUnits();
			if(iFreeAIWorkers > 0)
				iFreeAIWorkers += kGame.getStartEra() / 2;
			iFreeCount += iFreeAIWorkers;
			// <advc.126>
		}

		if (iFreeCount > 0)
		{
			addFreeUnitAI(UNITAI_WORKER, iFreeCount);
		}

		iFreeCount = GC.getInfo(kGame.getStartEra()).getStartingExploreUnits();
		iFreeCount += GC.getInfo(getHandicapType()).getStartingExploreUnits();

		if (!isHuman())
		{   // <advc.126>
			int iFreeAIExplorers = kGameHandicap.getAIStartingExploreUnits();
			/*  Need at least one addl. when starting in Classical era or later
				b/c the AI doesn't use its free defenders for exploration. */
			if(iFreeAIDefenders > 0)
				iFreeAIExplorers += (kGame.getStartEra() + 2)  / 3;
			iFreeCount += iFreeAIExplorers;
			// <advc.126>
		}

		if (iFreeCount > 0)
		{
			addFreeUnitAI(UNITAI_EXPLORE, iFreeCount);
		}
	}
}


void CvPlayer::addFreeUnitAI(UnitAITypes eUnitAI, int iCount)
{
	UnitTypes eBestUnit = NO_UNIT;
	int iBestValue = 0;
	CvCivilization const& kCiv = getCivilization(); // advc.003w
	for (int i = 0; i < kCiv.getNumUnits(); i++)
	{
		UnitTypes eLoopUnit = kCiv.unitAt(i);
		if (GC.getInfo(eLoopUnit).getPrereqAndBonus() == NO_BONUS &&
			// (advc: Replacing a loop)
			GC.getInfo(eLoopUnit).getNumPrereqOrBonuses() <= 0 &&
			canTrain(eLoopUnit) && // advc.opt: Moved down
			// advc.307: Machine Gun not useful enough against Barbarians
			(eUnitAI != UNITAI_CITY_DEFENSE || !GC.getInfo(eLoopUnit).isOnlyDefensive()))
		{
			int iValue = AI().AI_unitValue(eLoopUnit, eUnitAI, NULL);
			// <advc.250e> No Archer for exploration
			if (eUnitAI == UNITAI_EXPLORE)
				iValue -= AI().AI_unitValue(eLoopUnit, UNITAI_CITY_DEFENSE, NULL);
			// </advc.250e>
			if (iValue > iBestValue)
			{
				eBestUnit = eLoopUnit;
				iBestValue = iValue;
			}
		}
	}
	if (eBestUnit != NO_UNIT)
	{
		for (int i = 0; i < iCount; i++)
			addFreeUnit(eBestUnit, eUnitAI);
	}
}


void CvPlayer::addFreeUnit(UnitTypes eUnit, UnitAITypes eUnitAI)
{
	// advc.108: Right-hand side cut from below
	bool const bFound = (eUnitAI == UNITAI_SETTLE) ||
			(GC.getInfo(eUnit).getDefaultUnitAIType() == UNITAI_SETTLE);

	if (GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
	{
		if (bFound) // advc.108: No functional change
		{
			if (AI().AI_getNumAIUnits(UNITAI_SETTLE) >= 1)
				return;
		}
	}

	CvPlot* pStartingPlot = getStartingPlot();
	if(pStartingPlot == NULL)
		return;

	CvPlot* pBestPlot = NULL;
	//if (isHuman())
	if(eUnitAI == UNITAI_EXPLORE && // advc.108
		!GC.getInfo(eUnit).isFound() &&
		(GC.getPythonCaller()->isHumanExplorerPlacementRandomized() ||
		!isHuman())) // advc.108
	{
		//int iRandOffset = GC.getGame().getSorenRandNum(NUM_CITY_PLOTS, "Place Units (Player)");
		for (CityPlotRandIter it(*pStartingPlot, GC.getGame().getSorenRand(), true);
			it.hasNext(); ++it)
		{
			CvPlot& kLoopPlot = *it;
			if (!kLoopPlot.isGoody() &&
				!kLoopPlot.isImpassable() && !kLoopPlot.isUnit() &&
				kLoopPlot.sameArea(*pStartingPlot) &&
				// advc.108: Don't place the unit across a large bay
				GC.getMap().calculatePathDistance(&kLoopPlot, pStartingPlot) <= 3)
			{
				pBestPlot = &kLoopPlot;
				break;
			}
		}
	}

	if (pBestPlot == NULL)
		pBestPlot = pStartingPlot;

	// <advc.108> Centered on the Settler, not on the starting plot.
	if(bFound)
	{
		GET_TEAM(getID()).revealSurroundingPlots(*pBestPlot,
				GC.getDefineINT("START_SIGHT_RANGE"));
	} // </advc.108>
	initUnit(eUnit, pBestPlot->getX(), pBestPlot->getY(), eUnitAI);
}

// advc.opt: Now only a wrapper. I'm keeping it around for Python exporting.
int CvPlayer::startingPlotRange() const
{
	return GC.getGame().getStartingPlotRange();
}


bool CvPlayer::startingPlotWithinRange(CvPlot const& kPlot, PlayerTypes ePlayer, int iRange, int iPass) const
{
	/*	XXX changes to AI_foundValue (which are far more flexible) make this function
		redundant but it is still called from Python. */
	return false;
}


int CvPlayer::startingPlotDistanceFactor(CvPlot const& kPlot, PlayerTypes ePlayer, int iRange) const
{
	PROFILE_FUNC();

	FAssert(ePlayer != getID());
	FAssert(iRange > 0);

	int iValue = 1000;
	CvPlot const* pStartingPlot = getStartingPlot();
	if (pStartingPlot == NULL)
		return iValue;

	if (GC.getGame().isTeamGame())
	{
		if (GET_PLAYER(ePlayer).getTeam() == getTeam())
		{
			static int const iOWN_TEAM_STARTING_MODIFIER = GC.getDefineINT("OWN_TEAM_STARTING_MODIFIER"); // advc.opt
			iRange *= iOWN_TEAM_STARTING_MODIFIER;
			iRange /= 100;
		}
		else
		{
			static int const iRIVAL_TEAM_STARTING_MODIFIER = GC.getDefineINT("RIVAL_TEAM_STARTING_MODIFIER"); // advc.opt
			iRange *= iRIVAL_TEAM_STARTING_MODIFIER;
			iRange /= 100;
		}
	}

	int iDistance = ::stepDistance(&kPlot, pStartingPlot);
	if (!pStartingPlot->sameArea(kPlot))
	{
		iDistance *= 4;
		iDistance /= 3;
	}

	iValue *= iDistance;
	iValue /= iRange ;

	return std::max(1, iValue);
}

// advc.027:
int CvPlayer::coastRiverStartingAreaScore(CvArea const& a) const
{
	int r = 0;
	// Loop based on CvArea::countCoastalLand
	CvMap const& kMap = GC.getMap();
	for(int i = 0; i < kMap.numPlots(); i++)
	{
		CvPlot const& p = kMap.getPlotByIndex(i);
		if(p.isPeak() || !p.isArea(a))
			continue;
		int iTotalYield = p.calculateTotalBestNatureYield(getTeam());
		int iFoodYield = p.calculateBestNatureYield(YIELD_FOOD, getTeam());
		if((iFoodYield > 0 && iTotalYield >= 2) || iTotalYield >= 3)
		{
			if(p.isCoastalLand())
				r += 2;
			if(p.isRiver())
				r++;
		}
	}
	return r;
}

// kekm.35:
class VectorPairSecondGreaterComparator
{
public:
	bool operator() (const std::pair<int,int> &a, const std::pair<int,int> &b)
	{
		return (a.second >= b.second);
	}
};
// Returns the id of the best area, or -1 if it doesn't matter:
//int CvPlayer::findStartingArea() const
// kekm.35: "Returns a vector of all starting areas sorted by their value (instead of one best starting area)."
std::vector<std::pair<int,int> > CvPlayer::findStartingAreas(
	bool* pbFoundByMapScript) const // advc.027
{
	PROFILE_FUNC();
	// <advc.027>
	if (pbFoundByMapScript != NULL)
		*pbFoundByMapScript = false; // </advc.027>
	std::vector<std::pair<int,int> > areas_by_value; // kekm.35
	{
		CvArea* pyArea = GC.getPythonCaller()->findStartingArea(getID());
		if (pyArea != NULL)
		{
			areas_by_value.push_back(std::make_pair(pyArea->getID(), 1)); // kekm.35
			// <advc.027>
			if (pbFoundByMapScript != NULL)
				*pbFoundByMapScript = true; // </advc.027>
			return areas_by_value; // kekm.35
		}
	}
	// find best land area
	//int iBestValue = 0; int iBestArea = -1; // kekm.35
	FOR_EACH_AREA(pLoopArea)
	{
		if (pLoopArea->isWater())
			continue;

		// iNumPlayersOnArea is the number of players starting on the area, plus this player
		int iNumPlayersOnArea = (pLoopArea->getNumStartingPlots() + 1);
		// <advc.027>
		int iTileValue = 1 + pLoopArea->calculateTotalBestNatureYield() +
				/*2 * pLoopArea->countCoastalLand() +
				pLoopArea->getNumRiverEdges() +*/
				coastRiverStartingAreaScore(*pLoopArea) + // Replacing the above
				// New: factor in bonus resources
				intdiv::uround(pLoopArea->getNumTotalBonuses() * 3, 2) +
				pLoopArea->getNumTiles() / 2; // Halved
		// </advc.027>
		int iValue = iTileValue / iNumPlayersOnArea;
		iValue *= std::min(NUM_CITY_PLOTS + 1, pLoopArea->getNumTiles() + 1);
		iValue /= (NUM_CITY_PLOTS + 1);

		if (iNumPlayersOnArea <= 2)
		{
			iValue *= 4;
			iValue /= 3;
		}
		/*if (iValue > iBestValue) {
			iBestValue = iValue;
			iBestArea = pLoopArea->getID();
		}*/ // kekm.35:
		areas_by_value.push_back(std::make_pair(pLoopArea->getID(), iValue));
	}
	//return iBestArea; // <kekm.35>
	VectorPairSecondGreaterComparator kComparator;
	std::sort(areas_by_value.begin(), areas_by_value.end(), kComparator);
	// advc: No need to consider every little island
	areas_by_value.resize(std::min<int>(areas_by_value.size(),
			PlayerIter<CIV_ALIVE>::count()));
	return areas_by_value; // </kekm.35>
}


CvPlot* CvPlayer::findStartingPlot(
	// advc.027: (bRandomize param replaced with m_bRandomWBStart)
	bool* pbPlotFoundByMapScript, bool* pbAreaFoundByMapScript)
{
	PROFILE_FUNC();
	// <advc.027>
	if (pbPlotFoundByMapScript != NULL)
		*pbPlotFoundByMapScript = false;
	if (pbAreaFoundByMapScript != NULL)
		*pbAreaFoundByMapScript = false;
	// </advc.027>
	{
		CvPlot* r = GC.getPythonCaller()->findStartingPlot(getID());
		if (r != NULL)
		{	// <advc.027>
			if (pbPlotFoundByMapScript != NULL)
				*pbPlotFoundByMapScript = true; // </advc.027>
			return r;
		}
	}
	//int iBestArea = -1;
	// kekm.35: "This function is adjusted to work with a list of possible starting areas instead of a single one."
	std::vector<std::pair<int,int> > areasByValue;
	bool bNew = false;
	if (getStartingPlot() != NULL)
	{
		//iBestArea = getStartingPlot()->getArea().getID(); // kekm.35:
		areasByValue.push_back(std::make_pair(getStartingPlot()->getArea().getID(), 1));
		setStartingPlot(NULL/*, true*/); // advc.opt
		bNew = true;
	}

	AI().AI_updateFoundValues(true);

	if (!bNew)
	{
		//iBestArea = findStartingArea();
		areasByValue = findStartingAreas( // kekm.35
				pbAreaFoundByMapScript); // advc.027
	}
	/*  <advc.140> Cut and pasted from CvMap::maxPlotDistance. I've changed that
		function, but I think the original formula might be needed here.
		I'm not sure I understand the purpose of this outer loop. */
	// ^kekm.35 replaces the outer loop
	/*int iMaxPlotDist = std::max(1, ::plotDistance(0, 0, ((m.isWrapX()) ?
			(m.getGridWidth() / 2) : (m.getGridWidth() - 1)),
			((m.isWrapY()) ? (m.getGridHeight() / 2) :
			(m.getGridHeight() - 1))));
	for(int iPass = 0; iPass < iMaxPlotDist; iPass++)*/ // </advc.140>
	/*  <kekm.35> "First pass avoids starting locations that have very little food
		(before normalization) to avoid starting on the edge of very bad terrain." */
	int const iStartingRange = GC.getDefineINT("ADVANCED_START_SIGHT_RANGE");
	CvMap const& kMap = GC.getMap();
	int const iMaxPass = 1;
	for (int iPass = 0; iPass <= iMaxPass; iPass++)
	{
		for (size_t j = 0; j < areasByValue.size(); j++)
		{ // </kekm.35>
			CvPlot *pBestPlot = NULL;
			int iBestValue = iMaxPass - iPass; // advc: was 0 flat
			for (int i = 0; i < kMap.numPlots(); i++)
			{
				CvPlot* pLoopPlot = kMap.plotByIndex(i);
				//if (iBestArea == -1 || pLoopPlot->getArea() == iBestArea)
				// <kekm.35>
				if (pLoopPlot->getArea().getID() != areasByValue[j].first)
					continue;
				if (iPass == 0) // "Avoid very bad terrain in the first pass."
				{
					int iTotalFood = 0;
					int iLandPlots = 0;
					for (PlotCircleIter it(*pLoopPlot, iStartingRange); it.hasNext(); ++it)
					{
						CvPlot const& kCheckPlot = *it;
						if (!kCheckPlot.isWater())
						{
							iLandPlots++;
							iTotalFood += kCheckPlot.calculateBestNatureYield(
									YIELD_FOOD, NO_TEAM);
						}
					}
					if (iTotalFood < std::max(1, iLandPlots) / 2)
						continue;
				} // </kekm.35>
				//the distance factor is now done inside foundValue
				int iValue = pLoopPlot->getFoundValue(getID(),
						/*	advc.027: Replacing the randomization below, which is crude
							and too much. Also, findStartingPlot no longer has a
							bRandomize param. It had only been used by the WB scenario
							parser for the RandomStartLocation flag. The parser now uses
							CyPlayer::forceRandomWBStart. Note: Among the BtS scenarios,
							only "Europe" uses RandomStartLocation=true. */
						m_bRandomWBStart);
				/*if (bRandomize && iValue > 0)
					iValue += GC.getGame().getSorenRandNum(10000, "Randomize Starting Location");*/
				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					pBestPlot = pLoopPlot;
				}
			}
			if (pBestPlot != NULL)
				return pBestPlot;
		} // kekm.35: end of areas_by_value loop
		FAssertMsg(iPass != 0, "CvPlayer::findStartingPlot - could not find starting plot in first pass.");
	}

	FErrorMsg("Could not find starting plot.");
	return NULL;
}


CvPlotGroup* CvPlayer::initPlotGroup(CvPlot* pPlot)
{
	CvPlotGroup* pPlotGroup = addPlotGroup();
	FAssert(pPlotGroup != NULL);
	pPlotGroup->init(pPlotGroup->getID(), getID(), pPlot);
	return pPlotGroup;
}


CvCity* CvPlayer::initCity(int iX, int iY, bool bBumpUnits, bool bUpdatePlotGroups,
	int iOccupationTimer) // advc.ctr
{
	//PROFILE_FUNC(); // advc.003o
	CvCityAI* pCity = m_cities.AI_add(); // advc.003u: was = addCity()
	if (pCity == NULL)
	{
		FAssertMsg(pCity != NULL, "FLTA failed to allocate storage");
		return NULL;
	}
	FAssertMsg(!GC.getMap().plot(iX, iY)->isCity(), "No city is expected at this plot when initializing new city");
	pCity->init(pCity->getID(), getID(), iX, iY, bBumpUnits, bUpdatePlotGroups,
			iOccupationTimer); // advc.ctr
	/*  advc.104: Moved out of CvCity::init so that the new city is
		already fully initialized */
	setFoundedFirstCity(true);
	return pCity;
}

// (advc: Some comments added)
void CvPlayer::acquireCity(CvCity* pOldCity, bool bConquest, bool bTrade, bool bUpdatePlotGroups,
	bool bPeaceDeal) // advc.ctr
{
	CvPlot& kCityPlot = *pOldCity->plot();
	// Kill ICBMs
	//CLinkList<IDInfo> oldUnits; ... // advc: Deleted; unnecessary.
	FOR_EACH_UNIT_VAR_IN(pUnit, kCityPlot)
	{
		if (pUnit->getDomainType() == DOMAIN_IMMOBILE &&
			pUnit->getTeam() != getTeam())
		{
			pUnit->kill(false, getID());
		}
	}
	if (bConquest) // Force unowned after conquest
	{
		int const iRange = pOldCity->getCultureLevel();
		for (int iDX = -iRange; iDX <= iRange; iDX++)
		{
			for (int iDY = -iRange; iDY <= iRange; iDY++)
			{
				if (CvCity::cultureDistance(iDX, iDY) > iRange)
					continue;

				CvPlot* pLoopPlot = ::plotXY(kCityPlot.getX(), kCityPlot.getY(), iDX, iDY);
				if (pLoopPlot == NULL)
					continue;

				if (pLoopPlot->getOwner() != pOldCity->getOwner() ||
					pLoopPlot->getNumCultureRangeCities(pOldCity->getOwner()) != 1)
				{
					continue;
				}
				bool bForceUnowned = false;
				for (PlayerIter<ALIVE,NOT_SAME_TEAM_AS> itThirdPlayer(getTeam());
					itThirdPlayer.hasNext(); ++itThirdPlayer)
				{
					if (itThirdPlayer->getTeam() == pOldCity->getTeam())
						continue;
					if (pLoopPlot->getNumCultureRangeCities(itThirdPlayer->getID()) > 0)
					{
						bForceUnowned = true;
						break;
					}
				}
				if (bForceUnowned)
				{
					static int const iFORCE_UNOWNED_CITY_TIMER = GC.getDefineINT("FORCE_UNOWNED_CITY_TIMER"); // advc.opt
					pLoopPlot->setForceUnownedTimer(iFORCE_UNOWNED_CITY_TIMER);
				}
			}
		}
	}

	// Update city counts
	if (pOldCity->getOriginalOwner() == pOldCity->getOwner())
		GET_PLAYER(pOldCity->getOriginalOwner()).changeCitiesLost(1);
	else if (pOldCity->getOriginalOwner() == getID())
		GET_PLAYER(pOldCity->getOriginalOwner()).changeCitiesLost(-1);

	if (bConquest) // City-captured announcements, replay msg
	{
		CvWString szBuffer(gDLL->getText("TXT_KEY_MISC_CAPTURED_CITY", pOldCity->getNameKey()));
		gDLL->UI().addMessage(getID(), true, -1, szBuffer, pOldCity->getPlot(),
				"AS2D_CITYCAPTURE", MESSAGE_TYPE_MAJOR_EVENT, ARTFILEMGR.getInterfaceArtPath(
				"WORLDBUILDER_CITY_EDIT"), GC.getColorType("GREEN"));
		CvWString szName;
		szName.Format(L"%s (%s)", pOldCity->getName().GetCString(), GET_PLAYER(pOldCity->getOwner()).getReplayName());
		CvWString szCapturedBy(gDLL->getText("TXT_KEY_MISC_CITY_CAPTURED_BY",
				szName.GetCString(), getCivilizationDescriptionKey()));
		for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
		{
			CvPlayer const& kObs = *it;
			if (kObs.getID() == getID())
				continue;
			if (pOldCity->isRevealed(kObs.getTeam()) ||
				kObs.isSpectator()) // advc.127
			{
				gDLL->UI().addMessage(kObs.getID(), false, -1, szCapturedBy, pOldCity->getPlot(),
						"AS2D_CITYCAPTURED", /* advc.106b: */ MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY,
						ARTFILEMGR.getInterfaceArtPath("WORLDBUILDER_CITY_EDIT"),
						GC.getColorType("RED"));
			}
		}
		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szCapturedBy,
				pOldCity->getX(), pOldCity->getY(), GC.getColorType("WARNING_TEXT"));
	} // <advc.ctr> City-ceded announcement, replay msg
	else if (bTrade &&  // CvCity::liberate handles liberation announcement and replay msg.
		pOldCity->getLiberationPlayer() != getID())
	{
		CvWString szHasCeded;
		{	// Need to cache these locally
			CvWString szOldOwnerReplayName = GET_PLAYER(pOldCity->getOwner()).getReplayName();
			CvWString szNewOwnerReplayName = getReplayName();
			szHasCeded = gDLL->getText("TXT_KEY_MISC_CITY_CEDED_TO",
					/*	Don't obscure any names; isRevealed in the loop implies
						isHasSeen (though not isHasMet). */
					szOldOwnerReplayName.GetCString(), pOldCity->getNameKey(),
					szNewOwnerReplayName.GetCString());
		}
		// Don't announce if there's a reparations announcement
		if (!bPeaceDeal || !GC.getDefineBOOL(CvGlobals::ANNOUNCE_REPARATIONS))
		{
			for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
			{
				CvPlayer const& kObs = *it;
				if (kObs.getID() == getID() || kObs.getID() == pOldCity->getOwner())
					continue;
				if (!pOldCity->isRevealed(kObs.getTeam()) &&
					!kObs.isSpectator()) // advc.127
				{
					continue;
				}
				/*	advc.071: Meet before the announcement (with indicator at
					city coordinates). Callee will perform the relevant checks. */
				pOldCity->meetNewOwner(kObs.getTeam(), getTeam());
				gDLL->UI().addMessage(kObs.getID(), false, -1, szHasCeded, kCityPlot,
						NULL, /* advc.106b: */ MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY,
						ARTFILEMGR.getInterfaceArtPath("WORLDBUILDER_CITY_EDIT"),
						GC.getColorType("HIGHLIGHT_TEXT"));
			}
		}
		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szHasCeded,
				kCityPlot.getX(), kCityPlot.getY(), GC.getColorType("HIGHLIGHT_TEXT"));
	} // </advc.ctr>

	// Capture gold
	int iCaptureGold = 0;
	if (bConquest)
		iCaptureGold = doCaptureGold(*pOldCity); // advc.003y: Moved into subroutine

	// Deactivate vote sources before killing old city
	FOR_EACH_ENUM(VoteSource)
		pOldCity->processVoteSource(eLoopVoteSource, false);

	/*  Preserve city data in temporary variables (but not any data handled by the
		CvCity::process... functions) */ // advc.enum: use EnumMaps
	EnumMap<ReligionTypes,bool> abHasReligion;
	EnumMap<ReligionTypes,bool> abHolyCity;
	EnumMap<CorporationTypes,bool> abHasCorporation;
	EnumMap<CorporationTypes,bool> abHeadquarters;
	EnumMap<BuildingTypes,int> aiNumRealBuilding;
	EnumMap<BuildingTypes,PlayerTypes> aeBuildingOriginalOwner;
	EnumMapDefault<BuildingTypes,int,MIN_INT> aiBuildingOriginalTime;

	PlayerTypes const eOldOwner = pOldCity->getOwner();
	PlayerTypes const eOriginalOwner = pOldCity->getOriginalOwner();
	PlayerTypes const eHighestCulturePlayer = pOldCity->findHighestCulture();
	bool const bRecapture = (eHighestCulturePlayer != NO_PLAYER ?
			(GET_PLAYER(eHighestCulturePlayer).getTeam() == getTeam()) : false);
	int const iGameTurnFounded = pOldCity->getGameTurnFounded();
	int const iPopulation = pOldCity->getPopulation();
	int const iHighestPopulation = pOldCity->getHighestPopulation();
	int const iHurryAngerTimer = pOldCity->getHurryAngerTimer();
	int const iConscriptAngerTimer = pOldCity->getConscriptAngerTimer();
	int const iDefyResolutionAngerTimer = pOldCity->getDefyResolutionAngerTimer();
	int const iOccupationTimer = pOldCity->getOccupationTimer();
	CvWString szName(pOldCity->getNameKey());
	int const iDamage = pOldCity->getDefenseDamage();
	bool const bBombarded = pOldCity->isBombarded(); // advc.004c
	int const iOldCityId = pOldCity->getID();

	std::vector<int> aeFreeSpecialists;
	FOR_EACH_ENUM(Specialist)
		aeFreeSpecialists.push_back(pOldCity->getAddedFreeSpecialistCount(eLoopSpecialist));

	EnumMap<PlayerTypes,bool> abEverOwned;
	EnumMap<PlayerTypes,int> aiCulture;
	for (PlayerIter<> it; it.hasNext(); ++it)
	{
		abEverOwned.set(it->getID(), pOldCity->isEverOwned(it->getID()));
		aiCulture.set(it->getID(), pOldCity->getCultureTimes100(it->getID()));
	}
	// advc.ctr: This gets set automatically when kNewCity is initialized
	//abEverOwned.set(getID(), true);

	FOR_EACH_ENUM(Religion)
	{
		abHasReligion.set(eLoopReligion, pOldCity->isHasReligion(eLoopReligion));
		abHolyCity.set(eLoopReligion, pOldCity->isHolyCity(eLoopReligion));
	}
	FOR_EACH_ENUM2(Corporation, eCorp)
	{
		abHasCorporation.set(eCorp, pOldCity->isHasCorporation(eCorp));
		abHeadquarters.set(eCorp, pOldCity->isHeadquarters(eCorp));
	}
	FOR_EACH_ENUM2(Building, eBuilding)
	{
		aiNumRealBuilding.set(eBuilding, pOldCity->getNumRealBuilding(eBuilding));
		aeBuildingOriginalOwner.set(eBuilding, pOldCity->getBuildingOriginalOwner(eBuilding));
		aiBuildingOriginalTime.set(eBuilding, pOldCity->getBuildingOriginalTime(eBuilding));
	}
	// <advc.001f>
	EnumMap<TeamTypes,bool> abRevealed;
	for (TeamIter<> it; it.hasNext(); ++it)
		abRevealed.set(it->getID(), pOldCity->isRevealed(it->getID()));
	// </advc.001f>
	std::vector<BuildingYieldChange> aBuildingYieldChange;
	std::vector<BuildingCommerceChange> aBuildingCommerceChange;
	BuildingChangeArray aBuildingHappyChange;
	BuildingChangeArray aBuildingHealthChange;
	FOR_EACH_ENUM2(BuildingClass, eBuildingClass)
	{
		FOR_EACH_ENUM(Yield)
		{
			BuildingYieldChange kChange;
			kChange.eBuildingClass = eBuildingClass;
			kChange.eYield = eLoopYield;
			kChange.iChange = pOldCity->getBuildingYieldChange(
					eBuildingClass, eLoopYield);
			if (kChange.iChange != 0)
				aBuildingYieldChange.push_back(kChange);
		}
		FOR_EACH_ENUM(Commerce)
		{
			BuildingCommerceChange kChange;
			kChange.eBuildingClass = eBuildingClass;
			kChange.eCommerce = eLoopCommerce;
			kChange.iChange = pOldCity->getBuildingCommerceChange(
					eBuildingClass, eLoopCommerce);
			if (kChange.iChange != 0)
				aBuildingCommerceChange.push_back(kChange);
		}
		int iChange = pOldCity->getBuildingHappyChange(eBuildingClass);
		if (iChange != 0)
			aBuildingHappyChange.push_back(std::make_pair(eBuildingClass, iChange));
		iChange = pOldCity->getBuildingHealthChange(eBuildingClass);
		if (iChange != 0)
			aBuildingHealthChange.push_back(std::make_pair(eBuildingClass, iChange));
	}

	pOldCity->kill(false, /* advc.001: */ false); // Don't bump units yet
	pOldCity = NULL; // advc: Mustn't access that past this point

	if (bTrade) // Repercussions of cession: tile culture, war success (city culture: further down)
	{
		// <advc.ctr>
		for (CityPlotIter it(kCityPlot); it.hasNext(); ++it)
		{
			int iConvertedCulture = cultureConvertedUponCityTrade(
					kCityPlot, *it, eOldOwner, getID());
			it->changeCulture(eOldOwner, -iConvertedCulture, false);
			it->changeCulture(getID(), iConvertedCulture, false);
		}
		// BtS code replaced by the loop above // </advc.ctr>
		/*for (int iDX = -1; iDX <= 1; iDX++) {
			for (int iDY = -1; iDY <= 1; iDY++) {
				CvPlot* pLoopPlot = ::plotXY(kCityPlot.getX(), kCityPlot.getY(), iDX, iDY);
				if (pLoopPlot != NULL)
					pLoopPlot->setCulture(eOldOwner, 0, false, false);
			}
		}*/
		// <advc.123d>
		for (TeamIter<MAJOR_CIV,ENEMY_OF> itOtherEnemy(TEAMID(eOldOwner));
			itOtherEnemy.hasNext(); ++itOtherEnemy)
		{
			if (itOtherEnemy->getID() == getTeam())
				continue;
			bool bEverOwned = false;
			for (MemberIter itMember(itOtherEnemy->getID());
				itMember.hasNext(); ++itMember)
			{
				if (abEverOwned.get(itMember->getID()))
				{
					bEverOwned = true;
					break;
				}
			}
			if (bEverOwned)
			{
				GET_TEAM(eOldOwner).AI_changeWarSuccess(itOtherEnemy->getID(),
						-std::min(GC.getWAR_SUCCESS_CITY_CAPTURING(),
						GET_TEAM(eOldOwner).AI_getWarSuccess(itOtherEnemy->getID())));
			}
		} // </advc.123d>
	}

	// Create new city and assign data from temporary variables
	CvCity* pNewCity = initCity(kCityPlot.getX(), kCityPlot.getY(), !bConquest, false,
			// advc.ctr: Moved (way) up
			(bTrade && !bRecapture) ? iOccupationTimer : 0);
	CvCityAI& kNewCity = pNewCity->AI(); // advc.003u

	kNewCity.setPreviousOwner(eOldOwner);
	kNewCity.setOriginalOwner(eOriginalOwner);
	kNewCity.setGameTurnFounded(iGameTurnFounded);
	kNewCity.setPopulation((bConquest && !bRecapture) ?
			std::max(1, iPopulation - 1) : iPopulation);
	kNewCity.setHighestPopulation(iHighestPopulation);
	kNewCity.setName(szName, /* advc.106k: */ false, true);
	kNewCity.setNeverLost(false);
	kNewCity.changeDefenseDamage(iDamage);
	/*  advc.004c: The above will set the city as bombarded if iDamage>0.
		But iDamage>0 doesn't imply that the city was recently bombarded. */
	kNewCity.setBombarded(bBombarded);

	for (PlayerIter<> it; it.hasNext(); ++it)
	{
		// advc.ctr: Don't overwrite this player's ownership status
		if (!kNewCity.isEverOwned(it->getID()) && abEverOwned.get(it->getID()))
			kNewCity.setEverOwned(it->getID(), true);
		kNewCity.setCultureTimes100(it->getID(),
				aiCulture.get(it->getID()), false, false);
	} // <kekm.23>
	if(bTrade) // Further repercussions of cession: city culture
	{
		int iOldOwnerCulture = kNewCity.getCultureTimes100(eOldOwner);
		int iNewOwnerCulture = kNewCity.getCultureTimes100(getID());
		// Round down to a multiple of 100
		int iConvertedCulture = (iOldOwnerCulture / 300) * 100;
		kNewCity.setCultureTimes100(getID(),
				iNewOwnerCulture + iConvertedCulture, true, false);
		kNewCity.setCultureTimes100(eOldOwner,
				iOldOwnerCulture - iConvertedCulture, true, false);
	} // </kekm.23>

	// Destruction of buildings
	FOR_EACH_ENUM(Building)
	{
		if (aiNumRealBuilding.get(eLoopBuilding) <= 0)
			continue;
		BuildingClassTypes eBuildingClass = GC.getInfo(eLoopBuilding).
				getBuildingClassType();
		/*	Can't acquire another civ's unique building.
			Wonders exception allows Barbarians to capture wonders. */
		BuildingTypes const eBuilding = (GC.getInfo(eLoopBuilding).isWorldWonder() ?
				eLoopBuilding : getCivilization().getBuilding(eBuildingClass));
		if (eBuilding == NO_BUILDING)
			continue;
		CvBuildingInfo const& kBuilding = GC.getInfo(eBuilding);
		// Can acquire never-capture buildings only through city trade
		if (!bTrade && kBuilding.isNeverCapture())
			continue;

		if (isProductionMaxedBuildingClass(eBuildingClass, true) ||
			!kCityPlot.canConstruct(eBuilding))
		{
			continue;
		}
		// Capture roll unless recapture
		int iOdds = kBuilding.getConquestProbability();
		if (!bConquest || bRecapture || (iOdds > 0 &&
			iOdds > GC.getGame().getSorenRandNum(100, "Capture Probability")))
		{
			kNewCity.setNumRealBuildingTimed(eBuilding,
					std::min(GC.getDefineINT(CvGlobals::CITY_MAX_NUM_BUILDINGS),
					kNewCity.getNumRealBuilding(eBuilding) +
					aiNumRealBuilding.get(eBuilding)),
					false,
					aeBuildingOriginalOwner.get(eBuilding),
					aiBuildingOriginalTime.get(eBuilding));
		}
	}

	for (std::vector<BuildingYieldChange>::const_iterator it = aBuildingYieldChange.begin();
		it != aBuildingYieldChange.end(); ++it)
	{
		kNewCity.setBuildingYieldChange(it->eBuildingClass, it->eYield, it->iChange);
	}
	for (std::vector<BuildingCommerceChange>::const_iterator it = aBuildingCommerceChange.begin();
		it != aBuildingCommerceChange.end(); ++it)
	{
		kNewCity.setBuildingCommerceChange(it->eBuildingClass, it->eCommerce, it->iChange);
	}
	for (BuildingChangeArray::const_iterator it = aBuildingHappyChange.begin();
		it != aBuildingHappyChange.end(); ++it)
	{
		kNewCity.setBuildingHappyChange(it->first, it->second);
	}
	for (BuildingChangeArray::const_iterator it = aBuildingHealthChange.begin();
		it != aBuildingHealthChange.end(); ++it)
	{
		kNewCity.setBuildingHealthChange(it->first, it->second);
	}
	FOR_EACH_ENUM2(Specialist, eSpecialist)
		kNewCity.changeFreeSpecialistCount(eSpecialist, aeFreeSpecialists[eSpecialist]);
	FOR_EACH_ENUM2(Religion, eReligion)
	{
		if (abHasReligion.get(eReligion))
			kNewCity.setHasReligion(eReligion, true, false, true);
		if (abHolyCity.get(eReligion))
			GC.getGame().setHolyCity(eReligion, &kNewCity, false);
	}
	FOR_EACH_ENUM2(Corporation, eCorp)
	{
		if (abHasCorporation.get(eCorp))
			kNewCity.setHasCorporation(eCorp, true, false);
		if (abHeadquarters.get(eCorp))
			GC.getGame().setHeadquarters(eCorp, &kNewCity, false);
	}
	if (bTrade)
	{
		//if (isHuman() || getTeam() == TEAMID(eOldOwner))
		// advc.ctr: Only copy the anger timers if a reset to 0 could be exploited
		if (isHuman() && (GET_TEAM(eOldOwner).isHuman() || TEAMID(eOldOwner) == getTeam()))
		{
			kNewCity.changeHurryAngerTimer(iHurryAngerTimer);
			kNewCity.changeConscriptAngerTimer(iConscriptAngerTimer);
			kNewCity.changeDefyResolutionAngerTimer(iDefyResolutionAngerTimer);
		}
		/*if (!bRecapture) // advc.ctr: Moved up
			kNewCity.changeOccupationTimer(iOccupationTimer);*/
	}

	if (bConquest) // Set occupation timer, bump units
	{
		int iTeamCulturePercent = kNewCity.calculateTeamCulturePercent(getTeam());
		if (iTeamCulturePercent < GC.getDefineINT("OCCUPATION_CULTURE_PERCENT_THRESHOLD"))
		{
			int iPopPercent = GC.getDefineINT("OCCUPATION_TURNS_POPULATION_PERCENT");
			kNewCity.changeOccupationTimer(
					/*  advc.023: Population size as upper bound, and iPopPercent set to
						0 through XML. (Im multiplying by 1+100*iPopPercent so that the
						upper bound has no effect if iPopPercent is set back to 50 in XML.)
						NB: iTeamCulturePercent is city culture, not tile culture;
						only relevant when a city is reconquered. */
					std::min(kNewCity.getPopulation() * (1 + iPopPercent * 100),
					((GC.getDefineINT("BASE_OCCUPATION_TURNS") +
					((kNewCity.getPopulation() * iPopPercent) / 100)) *
					(100 - iTeamCulturePercent)) / 100));
		}
		GC.getMap().verifyUnitValidPlot();
	}
	// Update visibility, plot groups
	//pCityPlot->setRevealed(GET_PLAYER(eOldOwner).getTeam(), true, false, NO_TEAM, false);
	/*  <advc.001f> Need to reveal the new city so that the raze message is
		properly delivered. Reveal the tile once the raze decision is through
		(too early here). */
	for (TeamIter<> it; it.hasNext(); ++it)
	{
		if(abRevealed.get(it->getID()))
			kNewCity.setRevealed(it->getID(), true);
	} // </advc.001f>
	kNewCity.updateEspionageVisibility(false);
	if (bUpdatePlotGroups)
		GC.getGame().updatePlotGroups();

	// Notify observers ...
	// <advc.104>
	for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
		it->AI_cityCreated(kNewCity); // </advc.104>
	CvEventReporter::getInstance().cityAcquired(
			eOldOwner, getID(), &kNewCity, bConquest, bTrade);
	if (gPlayerLogLevel >= 1) logBBAI("  Player %d (%S) acquires city %S bConq %d bTrade %d", getID(), getCivilizationDescription(0), kNewCity.getName(0).GetCString(), bConquest, bTrade); // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000

	// Allow razing, disbanding
	if (bConquest)
	{
		bool bRazeImpossible = false; // advc.003y
		if (GC.getPythonCaller()->canRaze(kNewCity, getID()))
		{	// auto raze based on game rules
			if (kNewCity.isAutoRaze())
			{
				if (iCaptureGold > 0)
				{
					CvWString szBuffer(gDLL->getText("TXT_KEY_MISC_PILLAGED_CITY",
							iCaptureGold, kNewCity.getNameKey()));
					gDLL->UI().addMessage(getID(), true, -1, szBuffer,
							kNewCity.getPlot(), "AS2D_CITYRAZE",
							MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY, // advc.106b
							ARTFILEMGR.getInterfaceArtPath("WORLDBUILDER_CITY_EDIT"),
							GC.getColorType("GREEN"));
				}
				kNewCity.doTask(TASK_RAZE);
			}
			else if (!isHuman())
			{	// May delete kNewCity!
				AI().AI_conquerCity(kNewCity, abEverOwned.get(getID()));
			}
			else
			{	// popup raze option
				bool bRaze = canRaze(kNewCity);
				// <advc> Simplified
				PlayerTypes eLiberationPlayer = kNewCity.getLiberationPlayer(true);
				bool bGift = (eLiberationPlayer != NO_PLAYER &&
						eLiberationPlayer != getID() &&
						GET_TEAM(getTeam()).canPeacefullyEnter(TEAMID(eLiberationPlayer)));
				// </advc>  <advc.ctr> Make sure that the ownership change is legal
				if (bGift)
				{	// Don't check denial though; recipient can't refuse.
					bGift = canTradeCityTo(eLiberationPlayer, kNewCity, true);
				} // </advc.ctr>
				if (bRaze || bGift)
				{
					CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_RAZECITY);
					pInfo->setData1(kNewCity.getID());
					//pInfo->setData2(eLiberationPlayer);
					// advc: To communicate bGift to CvDLLButtonPopup::launchRazeCityPopup
					pInfo->setData2(bGift ? eLiberationPlayer : NO_PLAYER);
					pInfo->setData3(iCaptureGold);
					gDLL->UI().addPopup(pInfo, getID());
				} // <advc.003y> (based on K-Mod code)
				else bRazeImpossible = true;
			}
		}
		else bRazeImpossible = true;
		if (bRazeImpossible) // </advc.003y>
		{
			// K-Mod. properly handle the case where python says we can't raze the city
			CvEventReporter::getInstance().cityAcquiredAndKept(getID(), &kNewCity);
			if (isHuman())
				kNewCity.chooseProduction();
			// K-Mod end
		}
	}
	else if (!bTrade)
	{
		if (isHuman())
		{
			CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_DISBANDCITY);
			pInfo->setData1(kNewCity.getID());
			gDLL->UI().addPopup(pInfo, getID());
		}
		else CvEventReporter::getInstance().cityAcquiredAndKept(getID(), &kNewCity);
	} // <advc.004x>
	if(bTrade && isHuman())
		kNewCity.chooseProduction(); // </advc.004x>

	// Forcing events that deal with the old city not to expire just because we conquered that city [BtS comment]
	for (CvEventMap::iterator it = m_mapEventsOccured.begin();
		it != m_mapEventsOccured.end(); ++it)
	{
		EventTriggeredData &triggerData = it->second;
		if(triggerData.m_eOtherPlayer == eOldOwner && triggerData.m_iOtherPlayerCityId == iOldCityId)
			triggerData.m_iOtherPlayerCityId = -1;
	}
	// <advc.001f>
	for (TeamIter<> it; it.hasNext(); ++it)
	{
		if (abRevealed.get(it->getID()))
			kCityPlot.setRevealed(it->getID(), true, false, NO_TEAM, false);
	} // </advc.001f>
	/* <advc.001> Elimination otherwise happens only in CvGame::update, which
	   appears to be called only during the human player's turn. That means a dead
	   AI player can get one more turn before being eliminated. Not normally a problem
	   because there isn't much that a civ without cities can do, but I've had a
	   case where a dead player arranged a vassal agreement. */
	if(eOldOwner != NO_PLAYER)
		GET_PLAYER(eOldOwner).verifyAlive(); // </advc.001>
	// <advc.130w>
	AI().AI_updateCityAttitude(kCityPlot);
	GET_PLAYER(eOldOwner).AI_updateCityAttitude(kCityPlot); // </advc.130w>
}

// advc.ctr:
int CvPlayer::cultureConvertedUponCityTrade(CvPlot const& kCityPlot, CvPlot const& kPlot,
	PlayerTypes eOldOwner, PlayerTypes eNewOwner, bool bIgnorePriority) const
{
	bool bConvert = false;
	if (bIgnorePriority)
		bConvert = (::plotDistance(&kCityPlot, &kPlot) <= 2);
	else
	{
		// Always convert culture in the inner radius
		bConvert = ::adjacentOrSame(kPlot, kCityPlot);
		if (!bConvert)
		{
			// Outer circle: Based on plot priority when contested
			CvCity const* pDefaultWorkingCity = kPlot.defaultWorkingCity();
			bConvert = (pDefaultWorkingCity != NULL &&
					/*	Plot has to be assigned to the traded city or to
						a city of the new owner */
					(pDefaultWorkingCity->at(kCityPlot) ||
					pDefaultWorkingCity->getOwner() == eNewOwner));
		}
	}
	if (!bConvert)
		return 0;
	return std::min(kPlot.getCulture(eOldOwner) / 2,
			2 * kPlot.getCulture(getID()));
}


void CvPlayer::killCities()
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->kill(false);
	GC.getGame().updatePlotGroups();
}


CvWString CvPlayer::getNewCityName() const
{
	CvWString szName;
	/*for (pNode = headCityNameNode(); (pNode != NULL); pNode = nextCityNameNode(pNode)) {
		szName = gDLL->getText(pNode->m_data);
		if (isCityNameValid(szName, true)) {
			szName = pNode->m_data;
			break;
		}
	}*/ // BtS
	// K-Mod
	for (CLLNode<CvWString>* pNode = headCityNameNode();
		pNode != NULL && szName.empty(); pNode = nextCityNameNode(pNode))
	{
		szName = gDLL->getText(pNode->m_data); // (temp use of the buffer)
		if (isCityNameValid(szName, true))
			szName = pNode->m_data;
		else szName.clear(); // clear the buffer if the name is not valid!
	}
	/*	Note: unfortunately, the name-skipping system in getCivilizationCityName
		does not apply here. */
	// K-Mod end

	if (szName.empty())
		getCivilizationCityName(szName, getCivilizationType());

	if (szName.empty())
	{
		// Pick a name from another random civ
		int iRandOffset = GC.getGame().getSorenRandNum(GC.getNumCivilizationInfos(),
				"Place Units (Player)");
		FOR_EACH_ENUM(Civilization)
		{
			CivilizationTypes eRandCiv = (CivilizationTypes)
					((eLoopCivilization + iRandOffset) % GC.getNumCivilizationInfos());
			getCivilizationCityName(szName, eRandCiv);
			if (!szName.empty())
				break;
		}
	}
	if (szName.empty())
		szName = "TXT_KEY_CITY";

	return szName;
}


void CvPlayer::getCivilizationCityName(CvWString& szBuffer, CivilizationTypes eCivilization) const
{
	int iRandOffset=-1;
	/*if (isBarbarian() || isMinorCiv())
		iRandOffset = GC.getGame().getSorenRandNum(GC.getInfo(eCivilization).getNumCityNames(), "Place Units (Player)");
	else iRandOffset = 0;*/ // BtS
	// K-Mod
	if (eCivilization != getCivilizationType() || isBarbarian() || isMinorCiv())
	{
		iRandOffset = GC.getGame().getSorenRandNum(GC.getInfo(eCivilization).getNumCityNames(),
				"City name offset");
	}
	else
	{	// note: the explicit city names list is checked before this function is called.
		iRandOffset = std::max(0, getPlayerRecord()->getNumCitiesBuilt() - getNumCityNames());
	} // K-Mod end

	for (int i = 0; i < GC.getInfo(eCivilization).getNumCityNames(); i++)
	{
		int iLoopName = (i + iRandOffset) % GC.getInfo(eCivilization).getNumCityNames();
		CvWString szName = gDLL->getText(GC.getInfo(eCivilization).getCityNames(iLoopName));
		if (isCityNameValid(szName, true))
		{
			szBuffer = GC.getInfo(eCivilization).getCityNames(iLoopName);
			break;
		}
	}
}


bool CvPlayer::isCityNameValid(CvWString& szName, bool bTestDestroyed) const
{
	if (bTestDestroyed)
	{
		if (GC.getGame().isDestroyedCityName(szName))
			return false;

		for (PlayerIter<EVER_ALIVE> it; it.hasNext(); ++it)
		{
			FOR_EACH_CITY(pLoopCity, *it)
			{
				if (pLoopCity->getName() == szName)
					return false;
			}
		}
		return true;
	}

	FOR_EACH_CITY(pLoopCity, *this)
	{
		if (pLoopCity->getName() == szName)
			return false;
	}
	return true;
}


CvUnit* CvPlayer::initUnit(UnitTypes eUnit, int iX, int iY, UnitAITypes eUnitAI,
	DirectionTypes eFacingDirection)
{
	//PROFILE_FUNC(); // advc.003o

	CvUnitAI* pUnit = m_units.AI_add(); // advc.003u: was = addUnit()
	if (pUnit == NULL)
	{
		FAssertMsg(pUnit != NULL, "FLTA failed to allocate storage");
		return NULL;
	}
	FAssert(eUnit != NO_UNIT);
	pUnit->init(pUnit->getID(), eUnit, (UnitAITypes)
			(eUnitAI == NO_UNITAI ? GC.getInfo(eUnit).getDefaultUnitAIType() : eUnitAI),
			getID(), iX, iY, eFacingDirection);
	return pUnit;
}


void CvPlayer::disbandUnit(bool bAnnounce)
{
	int iBestValue = MAX_INT;
	CvUnit* pBestUnit = NULL;
	FOR_EACH_UNIT_VAR(pLoopUnit, *this)
	{
		if (pLoopUnit->hasCargo() || pLoopUnit->isGoldenAge() ||
			pLoopUnit->getUnitInfo().getProductionCost() <= 0)
		{
			continue;
		}
		if (pLoopUnit->isMilitaryHappiness() &&
			pLoopUnit->getPlot().plotCount(PUF_isMilitaryHappiness, -1, -1, getID()) <= 1)
		{
			continue;
		}
		int iValue = (10000 + GC.getGame().getSorenRandNum(1000, "Disband Unit"));
		iValue += (pLoopUnit->getUnitInfo().getProductionCost() * 5);
		iValue += (pLoopUnit->getExperience() * 20);
		iValue += (pLoopUnit->getLevel() * 100);

		if (pLoopUnit->canDefend() && pLoopUnit->getPlot().isCity() &&
			/*  advc.001s: I suppose this clause is intended for
				potential city defenders */
			pLoopUnit->getDomainType() == DOMAIN_LAND)
		{
			iValue *= 2;
		}
		if (pLoopUnit->getPlot().getTeam() == pLoopUnit->getTeam())
			iValue *= 3;

		switch (pLoopUnit->AI_getUnitAIType())
		{
		case UNITAI_UNKNOWN:
		case UNITAI_ANIMAL:
			break;

		case UNITAI_SETTLE:
			iValue *= 20;
			break;

		case UNITAI_WORKER:
			iValue *= 10;
			break;

		case UNITAI_ATTACK:
		case UNITAI_ATTACK_CITY:
		case UNITAI_COLLATERAL:
		case UNITAI_PILLAGE:
		case UNITAI_RESERVE:
		case UNITAI_COUNTER:
			iValue *= 2;
			break;

		case UNITAI_CITY_DEFENSE:
		case UNITAI_CITY_COUNTER:
		case UNITAI_CITY_SPECIAL:
		case UNITAI_PARADROP:
			iValue *= 6;
			break;

		case UNITAI_EXPLORE:
			iValue *= 15;
			break;

		case UNITAI_MISSIONARY:
			iValue *= 8;
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
			iValue *= 12;
			break;

		case UNITAI_ICBM:
			iValue *= 4;
			break;

		case UNITAI_WORKER_SEA:
			iValue *= 18;
			break;

		case UNITAI_ATTACK_SEA:
		case UNITAI_RESERVE_SEA:
		case UNITAI_ESCORT_SEA:
			break;

		case UNITAI_EXPLORE_SEA:
			iValue *= 25;
			break;

		case UNITAI_ASSAULT_SEA:
		case UNITAI_SETTLER_SEA:
		case UNITAI_MISSIONARY_SEA:
		case UNITAI_SPY_SEA:
		case UNITAI_CARRIER_SEA:
		case UNITAI_MISSILE_CARRIER_SEA:
			iValue *= 5;
			break;

		case UNITAI_PIRATE_SEA:
		case UNITAI_ATTACK_AIR:
			break;

		case UNITAI_DEFENSE_AIR:
		case UNITAI_CARRIER_AIR:
		case UNITAI_MISSILE_AIR:
			iValue *= 3;
			break;

		default:
			FAssert(false);
		}

		if (pLoopUnit->getUnitInfo().getExtraCost() > 0)
			iValue /= (pLoopUnit->getUnitInfo().getExtraCost() + 1);

		if (iValue < iBestValue)
		{
			iBestValue = iValue;
			pBestUnit = pLoopUnit;
		}
	}

	if (pBestUnit == NULL)
		return;

	FAssert(!pBestUnit->isGoldenAge());
	if (bAnnounce) // advc.001: Param was unused (since Vanilla Civ 4)
	{
		wchar szBuffer[1024];
		swprintf(szBuffer, gDLL->getText("TXT_KEY_MISC_UNIT_DISBANDED_NO_MONEY",
				pBestUnit->getNameKey()).GetCString());
		gDLL->UI().addMessage(getID(), false, -1, szBuffer, pBestUnit->getPlot(),
				"AS2D_UNITDISBANDED", MESSAGE_TYPE_MINOR_EVENT, pBestUnit->getButton(),
				GC.getColorType("RED"));
	}
	pBestUnit->kill(false);

}


void CvPlayer::killUnits()
{
	FOR_EACH_UNIT_VAR(pLoopUnit, *this)
		pLoopUnit->kill(false);
}


// advc.154: Cut from cycleSelectionGroups except for the non-const parts
CvSelectionGroup* CvPlayer::getNextGroupInCycle(CvUnit* pUnit, bool bForward,
	bool bWorkers, bool* pbWrap) const
{
	FAssert(GC.getGame().getActivePlayer() == getID() && isHuman());
	/* if (pbWrap != NULL)
		*pbWrap = false;*/
	// <K-Mod>
	bool bDummy = false;
	// this means we can just use bWrap directly and it will update *pbWrap if need be.
	bool& bWrap = (pbWrap != NULL ? *pbWrap : bDummy); // <K-Mod>
	bWrap = false;
	// advc.154: Copy the set
	std::set<int> kCycledGroups = GC.getGame().getActivePlayerCycledGroups(); // K-Mod
	CLLNode<int>* pSelectionGroupNode = headGroupCycleNode();
	if (pUnit != NULL)
	{
		while (pSelectionGroupNode != NULL)
		{
			if (getSelectionGroup(pSelectionGroupNode->m_data) == pUnit->getGroup())
			{	// <K-Mod>
				if (isTurnActive())
					kCycledGroups.insert(pSelectionGroupNode->m_data); // </K-Mod>
				if (bForward)
					pSelectionGroupNode = nextGroupCycleNode(pSelectionGroupNode);
				else pSelectionGroupNode = previousGroupCycleNode(pSelectionGroupNode);
				break;
			}
			pSelectionGroupNode = nextGroupCycleNode(pSelectionGroupNode);
		}
	}
	if (pSelectionGroupNode == NULL)
	{
		if (bForward)
			pSelectionGroupNode = headGroupCycleNode();
		else pSelectionGroupNode = tailGroupCycleNode();
		// if (pbWrap != NULL) *pbWrap = true; // disabled by K-Mod
	}

	if(pSelectionGroupNode == NULL)
		return NULL;

	CLLNode<int>* pFirstSelectionGroupNode = pSelectionGroupNode;
	while (true)
	{
		CvSelectionGroup* pLoopSelectionGroup = getSelectionGroup(
				pSelectionGroupNode->m_data);
		if (pLoopSelectionGroup->readyToSelect(/* advc.153: */ true) &&
			kCycledGroups.count(pSelectionGroupNode->m_data) <= 0) // K-Mod
		{
			if (!bWorkers ||
				// advc.153: with moves
				pLoopSelectionGroup->hasWorkerWithMoves())
			{
				/*if (pUnit && pLoopSelectionGroup == pUnit->getGroup())
					if (pbWrap != NULL) *pbWrap = true;*/
				return pLoopSelectionGroup;
			}
		}
		if (bForward)
		{
			pSelectionGroupNode = nextGroupCycleNode(pSelectionGroupNode);
			if (pSelectionGroupNode == NULL)
			{
				pSelectionGroupNode = headGroupCycleNode();
				// if (pbWrap != NULL) *pbWrap = true;
			}
		}
		else
		{
			pSelectionGroupNode = previousGroupCycleNode(pSelectionGroupNode);
			if (pSelectionGroupNode == NULL)
			{
				pSelectionGroupNode = tailGroupCycleNode();
				// if (pbWrap != NULL) *pbWrap = true;
			}
		}
		if (pSelectionGroupNode == pFirstSelectionGroupNode)
		{
			// break;
			// K-Mod
			if (bWrap)
				break;
			bWrap = true;
			// K-Mod end
		}
	}
	return NULL;
}

// XXX should pUnit be a CvSelectionGroup???
// Returns the next unit in the cycle...
CvSelectionGroup* CvPlayer::cycleSelectionGroups(CvUnit* pUnit, bool bForward,
	bool bWorkers, bool* pbWrap)
{
	FAssert(GC.getGame().getActivePlayer() == getID() && isHuman());
	if (pbWrap != NULL)
		*pbWrap = false;
	// <advc.004h>
	if(pUnit != NULL && pUnit->canFound())
		pUnit->updateFoundingBorder(true); // </advc.004h>
	// <K-Mod>
	std::set<int>& kCycledGroups = GC.getGame().getActivePlayerCycledGroups();
	if (pUnit != NULL && isTurnActive())
	{
		CLLNode<int>* pSelectionGroupNode = headGroupCycleNode();
		while (pSelectionGroupNode != NULL)
		{
			if (getSelectionGroup(pSelectionGroupNode->m_data) == pUnit->getGroup())
			{
				kCycledGroups.insert(pSelectionGroupNode->m_data);
				if (bForward)
					pSelectionGroupNode = nextGroupCycleNode(pSelectionGroupNode);
				else pSelectionGroupNode = previousGroupCycleNode(pSelectionGroupNode);
				break;
			}
			pSelectionGroupNode = nextGroupCycleNode(pSelectionGroupNode);
		}
	}
	// advc.154: Moved into new const function
	CvSelectionGroup* pGroup = getNextGroupInCycle(pUnit, bForward, bWorkers, pbWrap);
	if (pbWrap != NULL && pbWrap)
		kCycledGroups.clear(); // </K-Mod>
	return pGroup;
}

// AI_AUTO_PLAY_MOD, 07/09/08, jdog5000:
void CvPlayer::setHumanDisabled(bool bNewVal)
{
	// <advc.127>
	m_bAutoPlayJustEnded = true;
	CvGame& kGame = GC.getGame();
	// Not sure if this is needed:
	bool const bActive = (kGame.getActivePlayer() == getID());
	CvWString szReplayText;
	if (bNewVal && !m_bDisableHuman)
	{
		AI().AI_setHuman(false);
		if(bActive)
		{	// advc.004h:
			gDLL->getEngineIFace()->clearAreaBorderPlots(AREA_BORDER_LAYER_FOUNDING_BORDER);
			gDLL->UI().clearQueuedPopups();
			szReplayText = gDLL->getText("TXT_KEY_AUTO_PLAY_STARTED");
		}
	}
	else if (!bNewVal && m_bDisableHuman)
	{
		AI().AI_setHuman(true);
		m_iNewMessages = 0; // Don't open Event Log when coming out of Auto Play
		if(bActive)
			szReplayText = gDLL->getText("TXT_KEY_AUTO_PLAY_ENDED");
	}
	if(!szReplayText.empty())
	{
		kGame.addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szReplayText,
				-1, -1, GC.getColorType("HIGHLIGHT_TEXT"));
	} // </advc.127>
	m_bDisableHuman = bNewVal;
	updateHuman();
}

// advc.127:
bool CvPlayer::isSpectator() const
{
	return isHumanDisabled() && GC.getGame().isDebugMode();
}


void CvPlayer::updateHuman()
{
	if (getID() == NO_PLAYER)
		m_bHuman = false;
	else
	{
		// AI_AUTO_PLAY_MOD, 09/01/07, MRGENIE:
		if (m_bDisableHuman)
			m_bHuman = false;
		else // AI_AUTO_PLAY_MOD: END
			m_bHuman = GC.getInitCore().getHuman(getID());
	}
}

// K-Mod:
static bool concealUnknownCivs()
{
	return GC.getGame().getActiveTeam() != NO_TEAM &&
			//gDLL->getChtLvl() == 0
			// advc.135c: Replacing the above (which doesn't work in multiplayer)
			!GC.getGame().isDebugMode() &&
			!gDLL->GetWorldBuilderMode();
}

// advc.106i:
void CvPlayer::setSavingReplay(bool b)
{
	m_bSavingReplay = b;
}


const wchar* CvPlayer::getName(uint uiForm) const
{
	if (GC.getInitCore().getLeaderName(getID(), uiForm).empty() ||
		(GC.getGame().isMPOption(MPOPTION_ANONYMOUS) && isAlive() &&
		GC.getGame().getGameState() == GAMESTATE_ON))
	{
		return GC.getInfo(getLeaderType()).getDescription(uiForm);
	}
	// <advc.106i>
	if(m_bSavingReplay)
	{
		CvWString const szKey = "TXT_KEY_REPLAY_PREFIX";
		CvWString szPrefix = gDLL->getText(szKey);
		// No prefix if the key isn't present
		if(szKey.compare(szPrefix) == 0)
			szPrefix = L"";
		/*  Same hack as above. Don't call gDll->getModName b/c then I'd have
			to deal with narrow/wide string conversion. */
		static CvWString r; // Important to put the assignment on a separate line
		r = szPrefix + GC.getInitCore().getLeaderName(getID(), uiForm);
		return r;
	} // </advc.106i>
	return GC.getInitCore().getLeaderName(getID(), uiForm);
}

// advc.058: New function; unused, see getKnownCivDescription.
const wchar* CvPlayer::getKnownName(TeamTypes eObserver) const
{
	// advc.058: Moved from getName
	// K-Mod. Conceal the leader name of unmet players.
	if (concealUnknownCivs() &&
		!GET_TEAM(eObserver == NO_TEAM ? GC.getGame().getActiveTeam() : eObserver).
		isHasSeen(getTeam()))
	{	// hack to stop the string from going out of scope.
		static CvWString szUnknown = gDLL->getText("TXT_KEY_UNKNOWN");
		return szUnknown;
	} // K-Mod end
	return getName();
}

// K-Mod: Player name to be used in replay
const wchar* CvPlayer::getReplayName(uint uiForm) const
{
	if (GC.getInitCore().getLeaderName(getID(), uiForm).empty() ||
		(GC.getGame().isMPOption(MPOPTION_ANONYMOUS) && isAlive() &&
		GC.getGame().getGameState() == GAMESTATE_ON))
	{
		return GC.getInfo(getLeaderType()).getDescription(uiForm);
	}
	return GC.getInitCore().getLeaderName(getID(), uiForm)./*advc:*/GetCString();
}


const wchar* CvPlayer::getNameKey() const
{
	if ((GC.getInitCore().getLeaderNameKey(getID()).empty() ||
		GC.getGame().isMPOption(MPOPTION_ANONYMOUS) && isAlive()) &&
		/*  advc.001p: Had a crash here while loading a save from within a game with
			a higher player count than in the save. CvInitCore had already been reset.
			Can perhaps only occur with a debugger attached that slows the DLL down. */
		getLeaderType() != NO_LEADER)
	{
		return GC.getInfo(getLeaderType()).getTextKeyWide();
	}
	return GC.getInitCore().getLeaderNameKey(getID());
}


const wchar* CvPlayer::getCivilizationDescription(uint uiForm) const
{
	if (GC.getInitCore().getCivDescription(getID(), uiForm).empty())
		return GC.getInfo(getCivilizationType()).getDescription(uiForm);
	return GC.getInitCore().getCivDescription(getID(), uiForm);
}

/*	advc.058: New function. Currently unused because civ descriptions
	don't currently need to be concealed anywhere. (The Python screens
	take care of that themselves.) */
wchar const* CvPlayer::getKnownCivDescription(TeamTypes eObserver) const
{
	/*	advc.058: Moved from getCivilizationDescription, active team replaced
		with eObserver. */
	// K-Mod. Conceal the civilization of unmet players.
	if (concealUnknownCivs() &&
		!GET_TEAM(eObserver == NO_TEAM ? GC.getGame().getActiveTeam() : eObserver).
		isHasSeen(getTeam()))
	{	// hack to stop the string from going out of scope.
		static CvWString string = gDLL->getText("TXT_KEY_TOPCIVS_UNKNOWN");
		return string;
	} // K-Mod end
	return getCivilizationDescription();
}


const wchar* CvPlayer::getCivilizationDescriptionKey() const
{
	if (GC.getInitCore().getCivDescriptionKey(getID()).empty())
		return GC.getInfo(getCivilizationType()).getTextKeyWide();
	return GC.getInitCore().getCivDescriptionKey(getID());
}


const wchar* CvPlayer::getCivilizationShortDescription(uint uiForm) const
{
	if (GC.getInitCore().getCivShortDesc(getID(), uiForm).empty())
		return GC.getInfo(getCivilizationType()).getShortDescription(uiForm);
	return GC.getInitCore().getCivShortDesc(getID(), uiForm);
}

// advc.058: New function; unused, see getKnownCivDescription.
const wchar* CvPlayer::getKnownCivShortDescription(TeamTypes eObserver) const
{
	// advc.058: Moved from getCivilizationShortDescription
	// K-Mod. Conceal the civilization of unmet players.
	if (concealUnknownCivs() &&
		!GET_TEAM(eObserver == NO_TEAM ? GC.getGame().getActiveTeam() : eObserver).
		isHasSeen(getTeam()))
	{
		static CvWString szUnknown = gDLL->getText("TXT_KEY_UNKNOWN");
		return szUnknown;
	} // K-Mod end
	return getCivilizationShortDescription();
}


const wchar* CvPlayer::getCivilizationShortDescriptionKey() const
{
	if (GC.getInitCore().getCivShortDescKey(getID()).empty())
		return GC.getInfo(getCivilizationType()).getShortDescriptionKey();
	return GC.getInitCore().getCivShortDescKey(getID());
}


const wchar* CvPlayer::getCivilizationAdjective(uint uiForm) const
{
	if (GC.getInitCore().getCivAdjective(getID(), uiForm).empty())
		return GC.getInfo(getCivilizationType()).getAdjective(uiForm);
	return GC.getInitCore().getCivAdjective(getID(), uiForm);
}

const wchar* CvPlayer::getCivilizationAdjectiveKey() const
{
	if (GC.getInitCore().getCivAdjectiveKey(getID()).empty())
		return GC.getInfo(getCivilizationType()).getAdjectiveKey();
	return GC.getInitCore().getCivAdjectiveKey(getID());
}


CvWString CvPlayer::getFlagDecal() const
{
	if (GC.getInitCore().getFlagDecal(getID()).empty())
		return GC.getInfo(getCivilizationType()).getFlagTexture();
	return GC.getInitCore().getFlagDecal(getID());
}

bool CvPlayer::isWhiteFlag() const
{
	if (GC.getInitCore().getFlagDecal(getID()).empty())
		return GC.getInfo(getCivilizationType()).getArtInfo()->isWhiteFlag();
	return GC.getInitCore().getWhiteFlag(getID());
}

/*	advc.127c: Wrapper for CvInitCore::setFlagDecal that can (bUpdate=true)
	attempt to clear all flag symbols on the map in order to update
	the flag graphics. */
void CvPlayer::setFlagDecal(CvWString const& szFlagDecal, bool bUpdate)
{
	if (&szFlagDecal == &GC.getInitCore().getFlagDecal(getID()))
		return;
	GC.getInitCore().setFlagDecal(getID(), szFlagDecal);
	if (!bUpdate)
		return;
	gDLL->UI().setDirty(Flag_DIRTY_BIT, true);
	if (isBarbarian())
	{
		FErrorMsg("CvPlot::clearFlagSymbol might not work correctly when"
				" updating flag symbols after changing the Barbarian flag decal.");
		return;
	}
	for (int i = 0; i < GC.getMap().numPlots(); i++)
	{
		GC.getMap().plotByIndex(i)->clearFlagSymbol();
	}
}


wchar const* CvPlayer::getStateReligionName(uint uiForm) const
{
	return GC.getInfo(getStateReligion()).getDescription(uiForm);
}

wchar const* CvPlayer::getStateReligionKey() const
{
	if (getStateReligion() != NO_RELIGION)
		return GC.getInfo(getStateReligion()).getTextKeyWide();
	return L"TXT_KEY_MISC_NO_STATE_RELIGION";
}


CvWString CvPlayer::getBestAttackUnitName(uint uiForm) const
{
	return gDLL->getObjectText((CvString)getBestAttackUnitKey(), uiForm, true);
}


CvWString CvPlayer::getWorstEnemyName() const
{
	TeamTypes eWorstEnemy = GET_TEAM(getTeam()).AI_getWorstEnemy();
	if (eWorstEnemy != NO_TEAM)
		return GET_TEAM(eWorstEnemy).getName();
	return "";
}

wchar const* CvPlayer::getBestAttackUnitKey() const
{	// advc.079: Code moved into subroutine
	UnitTypes eBestUnit = AI().AI_getBestAttackUnit();
	if (eBestUnit != NO_UNIT)
		return GC.getInfo(eBestUnit).getTextKeyWide();
	return L"TXT_KEY_MISC_NO_UNIT";
}

ArtStyleTypes CvPlayer::getArtStyleType() const
{
	if (GC.getInitCore().getArtStyle(getID()) == NO_ARTSTYLE)
		return ((ArtStyleTypes)GC.getInfo(getCivilizationType()).getArtStyleType());
	return GC.getInitCore().getArtStyle(getID());
}

TCHAR const* CvPlayer::getUnitButton(UnitTypes eUnit) const
{
	return GC.getInfo(eUnit).getArtInfo(0, getCurrentEra(), (UnitArtStyleTypes)
			GC.getInfo(getCivilizationType()).getUnitArtStyleType())->getButton();
}

/*  advc (comment): Contains the entire sequence of an AI turn, and is called
	when a human player ends his/her turn. */
void CvPlayer::doTurn()  // advc: style changes
{
	PROFILE_FUNC();

	FAssert(isAlive());
	FAssertMsg(!hasBusyUnit() || GC.getGame().isMPOption(MPOPTION_SIMULTANEOUS_TURNS) ||
			GC.getGame().isSimultaneousTeamTurns(), "End of turn with busy units in a sequential-turn game");
	CvGame& kGame = GC.getGame();
	// <advc.106b>
	if (!kGame.isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
		kGame.setInBetweenTurns(true);
	if (isHuman() && //getStartOfTurnMessageLimit() >= 0 && // The message should be helpful even if the log doesn't auto-open
		kGame.getElapsedGameTurns() > 0 && !m_listGameMessages.empty())
	{
		gDLL->UI().addMessage(getID(), false, 0, gDLL->getText("TXT_KEY_END_TURN_MSG"), 0,
				MESSAGE_TYPE_EOT, 0, GC.getColorType("LIGHT_GREY"));
	}
	if (isHuman())
		m_iNewMessages = 0;
	/*  This way, NewMessages is never reset for non-humans. It is reset in
		setHumanDisabled though, i.e. when coming out of AI Auto Play. */
	if (isHuman())
		gDLL->UI().clearEventMessages();
	// </advc.106b>

	CvEventReporter::getInstance().beginPlayerTurn(kGame.getGameTurn(), getID());
	/*  advc.127: Only needs to be true when Civ4lerts are checked, which is done
		in response to beginPlayerTurn. */
	m_bAutoPlayJustEnded = false;

	//doUpdateCacheOnTurn(); // advc: removed
	kGame.verifyDeals();
	AI().AI_doTurnPre();

	if (getRevolutionTimer() > 0)
		changeRevolutionTimer(-1);
	if (getConversionTimer() > 0)
		changeConversionTimer(-1);
	setConscriptCount(0);

	AI().AI_assignWorkingPlots();

	//if (0 == GET_TEAM(getTeam()).getHasMetCivCount(true) || kGame.isOption(GAMEOPTION_NO_ESPIONAGE))
	// K-Mod.
	if (isCommerceFlexible(COMMERCE_ESPIONAGE) &&
		(GET_TEAM(getTeam()).getHasMetCivCount(true) == 0 ||
		kGame.isOption(GAMEOPTION_NO_ESPIONAGE))) //
	{
		setCommercePercent(COMMERCE_ESPIONAGE, 0); // (note: not forced)
	}
	verifyGoldCommercePercent();
	doGold();
	doResearch();
	doEspionagePoints();

	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->doTurn();

	if (getGoldenAgeTurns() > 0)
		changeGoldenAgeTurns(-1);
	if (getAnarchyTurns() > 0)
		changeAnarchyTurns(-1);
	verifyCivics();
	doChangeCivicsPopup(NO_CIVIC); // advc.004x
	//verifyStateReligion(); // kekm.10: disabled for now

	updateTradeRoutes();
	updateWarWearinessPercentAnger();
	// <advc.011>
	if(GC.getDefineINT(CvGlobals::DELAY_UNTIL_BUILD_DECAY) > 0)
		decayBuildProgress();
	// </advc.011>
	doEvents();
	/* advc.136a: Moved here from CvTeam::doTurn. (CvPlayer::doTurn happens at the
	   end of a human turn, CvTeam::doTurn at the beginning.) Doing it in
	   CvPlot::setRevealed would be nice, but possibly too slow - game already tends
	   to hang for a moment after map trades. */
	GET_TEAM(getTeam()).testCircumnavigated();
	// <advc.029>
	FOR_EACH_UNIT_VAR(u, *this)
		u->doTurnPost(); // </advc.029>
	// <advc.004l>
	FOR_EACH_GROUP_VAR(gr, *this)
		gr->doTurnPost(); // </advc.004l>
	/*  <advc.034> Cancel disengagement agreements at the end of a round, i.e.
		at the end of the Barbarian turn. */
	int iDisengageLength = GC.getDefineINT(CvGlobals::DISENGAGE_LENGTH);
	if(isBarbarian() && iDisengageLength > 0)
	{
		FOR_EACH_DEAL_VAR(d)
		{
			if(d->isDisengage() && d->turnsToCancel() <= 1)
			{
				/*  Still one turn to cancel, but that turn is practically over.
					Set to 0 turns to cancel so that the deal-canceled message
					says "0 turns". */
				d->setInitialGameTurn(d->getInitialGameTurn() - 1);
				d->kill();
			}
		}
	}
	else if(iDisengageLength < 0) // See GlobalDefines_advc.xml about this
	{
		CvTeam& kOurTeam = GET_TEAM(getTeam());
		for(int i = 0; i < MAX_CIV_TEAMS; i++)
		{
			TeamTypes tId = (TeamTypes)i;
			if(kOurTeam.isDisengage(tId))
				kOurTeam.cancelDisengage(tId);
		}
	} // </advc.034>
	/*  advc.074: Make sure (if only for performance) that we don't keep expecting
		a cancel-trade popup that never comes (the importer may have died) */
	m_cancelingExport.clear();
	int const iGameTurn = kGame.getGameTurn();
	// <advc.004s> BtS code moved into updateHistory
	FOR_EACH_ENUM(PlayerHistory)
	{
		if (eLoopPlayerHistory == PLAYER_HISTORY_SCORE)
			continue; // Handled by CvGame
		updateHistory(eLoopPlayerHistory, iGameTurn);
	} // </advc.004s>
	expireMessages(); // turn log

	showForeignPromoGlow(false); // advc.002e: To match call in doWarnings
	gDLL->UI().setDirty(CityInfo_DIRTY_BIT, true);

	AI().AI_doTurnPost();
	// <advc.700>
	if(kGame.isOption(GAMEOPTION_RISE_FALL))
		kGame.getRiseFall().atTurnEnd(getID()); // </advc.700>

	if (kGame.isDebugMode()) // BETTER_BTS_AI_MOD, Debug, 07/08/09, jdog5000
		kGame.updateColoredPlots();

	CvEventReporter::getInstance().endPlayerTurn(iGameTurn, getID());
}


void CvPlayer::doTurnUnits()
{
	PROFILE_FUNC();

	AI().AI_doTurnUnitsPre();
	FOR_EACH_GROUP_VAR(pLoopSelectionGroup, *this)
		pLoopSelectionGroup->doDelayedDeath();

	for (int iPass = 0; iPass < 4; iPass++)
	{
		FOR_EACH_GROUP_VAR(pLoopSelectionGroup, *this)
		{
			switch (pLoopSelectionGroup->getDomainType())
			{
			case DOMAIN_AIR:
				if (iPass == 1)
					pLoopSelectionGroup->doTurn();
				break;
			case DOMAIN_SEA:
				if (iPass == 2)
					pLoopSelectionGroup->doTurn();
				break;
			case DOMAIN_LAND:
				if (iPass == 3)
					pLoopSelectionGroup->doTurn();
				break;
			case DOMAIN_IMMOBILE:
				if (iPass == 0)
					pLoopSelectionGroup->doTurn();
				break;
			case NO_DOMAIN:
				FAssertMsg(pLoopSelectionGroup->getHeadUnit() == NULL, "Unit with no Domain");
			default:
				if (iPass == 3)
					pLoopSelectionGroup->doTurn();
			}
		}
	}

	// K-Mod. (currently unused)
	/*FOR_EACH_GROUP_VAR(pLoopSelectionGroup, *this)
		updateGroupCycle(pLoopSelectionGroup);*/
	// K-Mod end

	if (getID() == GC.getGame().getActivePlayer())
	{
		gDLL->getFAStarIFace()->ForceReset(&GC.getInterfacePathFinder());
		gDLL->UI().setDirty(Waypoints_DIRTY_BIT, true);
		gDLL->UI().setDirty(SelectionButtons_DIRTY_BIT, true);
	}

	gDLL->UI().setDirty(UnitInfo_DIRTY_BIT, true);

	AI().AI_doTurnUnitsPost();
}


void CvPlayer::verifyCivics()  // advc: refactored
{
	if (isAnarchy())
		return;

	FOR_EACH_ENUM(CivicOption)
	{
		if (canDoCivics(getCivics(eLoopCivicOption)))
			continue; // verified

		FOR_EACH_ENUM(Civic)
		{
			if (GC.getInfo(eLoopCivic).getCivicOptionType() == eLoopCivicOption &&
				canDoCivics(eLoopCivic))
			{
					setCivics(eLoopCivicOption, eLoopCivic);
					break;
			}
		}
	}
}

// kekm.10:
void CvPlayer::verifyStateReligion()
{
	if(!isAnarchy() && !canDoReligion(getStateReligion()))
		setLastStateReligion(NO_RELIGION);
}

// advc.064d:
void CvPlayer::verifyCityProduction()
{
	FOR_EACH_CITY_VAR(c, *this)
		c->verifyProduction();
}


void CvPlayer::updatePlotGroups()
{
	PROFILE_FUNC();

	if(!GC.getGame().isFinalInitialized())
		return;

	int iLoop;
	for(CvPlotGroup* pLoopPlotGroup = firstPlotGroup(&iLoop);
		pLoopPlotGroup != NULL; pLoopPlotGroup = nextPlotGroup(&iLoop))
	{
		pLoopPlotGroup->recalculatePlots();
	}
	CvMap const& kMap = GC.getMap();
	for(int i = 0; i < kMap.numPlots(); i++)
		kMap.getPlotByIndex(i).updatePlotGroup(getID(), false, /* advc.064d: */ false);

	verifyCityProduction(); // advc.064d
	updateTradeRoutes();
}


void CvPlayer::updateYield()
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->updateYield();
}


void CvPlayer::updateMaintenance()
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->updateMaintenance();
}


void CvPlayer::updatePowerHealth()
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->updatePowerHealth();
}


void CvPlayer::updateExtraBuildingHappiness()
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->updateExtraBuildingHappiness();
}


void CvPlayer::updateExtraBuildingHealth()
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->updateExtraBuildingHealth();
}


void CvPlayer::updateFeatureHappiness()
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->updateSurroundingHealthHappiness();
}


void CvPlayer::updateReligionHappiness()
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->updateReligionHappiness();
}


void CvPlayer::updateExtraSpecialistYield()
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->updateExtraSpecialistYield();
}


void CvPlayer::updateCommerce(CommerceTypes eCommerce)
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->updateCommerce(eCommerce);
}


void CvPlayer::updateCommerce()
{
	PROFILE_FUNC(); // K-Mod

	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->updateCommerce();
}


void CvPlayer::updateBuildingCommerce()
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->updateBuildingCommerce();
}


void CvPlayer::updateReligionCommerce()
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->updateReligionCommerce();
}


void CvPlayer::updateCorporation()
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->updateCorporation();
}

// advc.003j: Vanilla Civ 4 function that was, apparently, never used.
/*void CvPlayer::updateCityPlotYield()
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->getPlot().updateYield();
}*/


void CvPlayer::updateCitySight(bool bIncrement, bool bUpdatePlotGroups)
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->getPlot().updateSight(bIncrement, bUpdatePlotGroups);
}


void CvPlayer::updateTradeRoutes()
{
	PROFILE_FUNC();

	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->clearTradeRoutes();

	CLinkList<int> cityList;
	FOR_EACH_CITY_VAR(pLoopCity, *this)
	{
		int iTotalTradeModifier = pLoopCity->totalTradeModifier();

		CLLNode<int>* pCityNode = cityList.head();
		while (pCityNode != NULL)
		{
			CvCity* pListCity = getCity(pCityNode->m_data);
			if (iTotalTradeModifier > pListCity->totalTradeModifier())
			{
				cityList.insertBefore(pLoopCity->getID(), pCityNode);
				break;
			}
			else pCityNode = cityList.next(pCityNode);
		}

		if (pCityNode == NULL)
			cityList.insertAtEnd(pLoopCity->getID());
	}

	CLLNode<int>* pCityNode = cityList.head();
	while (pCityNode != NULL)
	{
		getCity(pCityNode->m_data)->updateTradeRoutes();
		pCityNode = cityList.next(pCityNode);
	}
}

void CvPlayer::updatePlunder(int iChange, bool bUpdatePlotGroups)
{
	FOR_EACH_UNIT_VAR(pLoopUnit, *this)
	{
		if (pLoopUnit->isBlockading())
			pLoopUnit->updatePlunder(iChange, bUpdatePlotGroups);
	}
}

void CvPlayer::updateTimers()
{
	FOR_EACH_GROUP_VAR(pLoopSelectionGroup, *this)
		pLoopSelectionGroup->updateTimers(); // could destroy the selection group...

	// if a unit was busy, perhaps it was not quite deleted yet, give it one more try
	if (getNumSelectionGroups() > getNumUnits())
	{
		FOR_EACH_GROUP_VAR(pLoopSelectionGroup, *this)
			pLoopSelectionGroup->doDelayedDeath(); // could destroy the selection group...
	}

	FAssert(getNumSelectionGroups() <= getNumUnits());
}


bool CvPlayer::hasReadyUnit(bool bAny) const
{
	PROFILE_FUNC();

	FOR_EACH_GROUP(pLoopSelectionGroup, *this)
	{
		if (pLoopSelectionGroup->readyToMove(bAny) &&
			!pLoopSelectionGroup->isAutomated()) // K-Mod
		{
			return true;
		}
	}
	return false;
}


bool CvPlayer::hasAutoUnit() const
{
	PROFILE_FUNC();

	FOR_EACH_GROUP(pLoopSelectionGroup, *this)
	{
		if (pLoopSelectionGroup->readyToAuto())
			return true;
	}
	return false;
}


bool CvPlayer::hasBusyUnit() const
{
	//PROFILE_FUNC();
	FOR_EACH_GROUP(pLoopSelectionGroup, *this)
	{
		if (pLoopSelectionGroup->isBusy())
		{
			/*if (pLoopSelectionGroup->getNumUnits() == 0) {
				pLoopSelectionGroup->kill();
				return false;
			}*/ // BtS - disabled by K-Mod. isBusy returns false if there are no units in the group.
			return true;
		}
	}
	return false;
}

void CvPlayer::chooseTech(int iDiscover, CvWString szText, bool bFront)
{
	// K-Mod
	FAssert(isHuman());
	if (iDiscover > 0)
	{	// note: if iDiscover is > 1, this function will be called again with iDiscover-=1
		changeChoosingFreeTechCount(1);
	} // K-Mod end

	CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_CHOOSETECH);
	if (pInfo != NULL)
	{
		pInfo->setData1(iDiscover);
		pInfo->setText(szText);
		gDLL->UI().addPopup(pInfo, getID(), false, bFront);
	}
}

// advc (caveat): Needs to be consistent with CvGameTextMgr::setScoreHelp
int CvPlayer::calculateScore(bool bFinal, bool bVictory) const
{
	PROFILE_FUNC();

	if (!isAlive() || GET_TEAM(getTeam()).getNumMembers() <= 0)
		return 0;
	// <advc.707>
	if(GC.getGame().isOption(GAMEOPTION_RISE_FALL) && bFinal)
		return GC.getGame().getRiseFall().getNormalizedFinalScore();
	// </advc.707>
	/*  <advc.003y> Ported from CvGameUtils.py; callback disabled by default.
		Apart from CvGame::getScoreComponent, the auxiliary functions were already
		in the DLL. */
	static int const iSCORE_POPULATION_FACTOR = GC.getDefineINT("SCORE_POPULATION_FACTOR");
	static int const iSCORE_LAND_FACTOR = GC.getDefineINT("SCORE_LAND_FACTOR");
	static int const iSCORE_TECH_FACTOR = GC.getDefineINT("SCORE_TECH_FACTOR");
	static int const iSCORE_WONDER_FACTOR = GC.getDefineINT("SCORE_WONDER_FACTOR");
	CvGame const& kGame = GC.getGame();
	int iPopulationScore = kGame.getScoreComponent(getPopScore(), kGame.getInitPopulation(),
			kGame.getMaxPopulation(), iSCORE_POPULATION_FACTOR, true, bFinal, bVictory);
	int iLandScore = kGame.getScoreComponent(getLandScore(), kGame.getInitLand(),
			kGame.getMaxLand(), iSCORE_LAND_FACTOR, true, bFinal, bVictory);
	int iTechScore = kGame.getScoreComponent(getTechScore(), kGame.getInitTech(),
			kGame.getMaxTech(), iSCORE_TECH_FACTOR, true, bFinal, bVictory);
	int iWondersScore = kGame.getScoreComponent(getWondersScore(), kGame.getInitWonders(),
			kGame.getMaxWonders(), iSCORE_WONDER_FACTOR, false, bFinal, bVictory);
	int iTotal = iPopulationScore + iLandScore + iWondersScore + iTechScore;

	GC.getPythonCaller()->doPlayerScore(getID(), bFinal, bVictory, iTotal); // </advc.003y>

	return iTotal;
}


int CvPlayer::findBestFoundValue() const
{
	int iBestValue = 0;
	FOR_EACH_AREA(pLoopArea)
	{
		int iValue = pLoopArea->getBestFoundValue(getID());
		if (iValue > iBestValue)
			iBestValue = iValue;
	}
	return iBestValue;
}


int CvPlayer::upgradeAllPrice(UnitTypes eUpgradeUnit, UnitTypes eFromUnit) const
{
	int iPrice = 0;
	FOR_EACH_UNIT(pLoopUnit, *this)
	{
		if (pLoopUnit->getUnitType() == eFromUnit)
		{
			if (pLoopUnit->canUpgrade(eUpgradeUnit, true))
				iPrice += pLoopUnit->upgradePrice(eUpgradeUnit);
		}
	}
	return iPrice;
}

// <advc.080> Based on upgradeAllPrice
int CvPlayer::upgradeAllXPChange(UnitTypes eUpgradeUnit, UnitTypes eFromUnit) const
{
	int r = 0;
	FOR_EACH_UNIT(u, *this)
	{
		if(u->getUnitType() == eFromUnit && u->canUpgrade(eUpgradeUnit, true))
			r += u->upgradeXPChange(eUpgradeUnit);
	}
	return r;
} // </advc.080>

int CvPlayer::countReligionSpreadUnits(CvArea const* pArea,
	ReligionTypes eReligion, /* BBAI: */ bool bIncludeTraining) const
{
	PROFILE_FUNC();

	int iCount = 0;
	FOR_EACH_UNIT(pLoopUnit, *this)
	{
		if (pLoopUnit->isArea(*pArea))
		{
			if (pLoopUnit->getUnitInfo().getReligionSpreads(eReligion) > 0)
				iCount++;
		}
	}
	// BETTER_BTS_AI_MOD (11/14/09, jdog5000): START
	if (bIncludeTraining)
	{
		FOR_EACH_CITY(pLoopCity, *this)
		{
			UnitTypes eUnit = pLoopCity->getProductionUnit();
			if (eUnit != NO_UNIT)
			{
				if (GC.getInfo(eUnit).getReligionSpreads(eReligion) > 0)
					iCount++;
			}
		}
	} // BETTER_BTS_AI_MOD: END

	return iCount;
}

int CvPlayer::countCorporationSpreadUnits(CvArea const* pArea,
	CorporationTypes eCorporation, /* BBAI: */ bool bIncludeTraining) const
{
	PROFILE_FUNC();

	int iCount = 0;
	FOR_EACH_UNIT(pLoopUnit, *this)
	{
		//if (pLoopUnit->area() == pArea)
		if (pArea == NULL || pLoopUnit->isArea(*pArea)) // K-Mod
		{
			if (pLoopUnit->getUnitInfo().getCorporationSpreads(eCorporation) > 0)
				iCount++;
		}
	}
	// BETTER_BTS_AI_MOD (11/14/09, jdog5000): START
	if (bIncludeTraining)
	{
		FOR_EACH_CITY(pLoopCity, *this)
		{
			if (pArea == NULL || pLoopCity->isArea(*pArea)) // K-Mod
			{
				UnitTypes eUnit = pLoopCity->getProductionUnit();
				if (eUnit != NO_UNIT)
				{
					if(GC.getInfo(eUnit).getCorporationSpreads(eCorporation) > 0)
						iCount++;
				}
			}
		}
	} // BETTER_BTS_AI_MOD: END

	return iCount;
}

int CvPlayer::countNumCoastalCities() const
{
	int iCount = 0;
	FOR_EACH_CITY(pLoopCity, *this)
	{
		if (pLoopCity->isCoastal())
			iCount++;
	}
	return iCount;
}


int CvPlayer::countNumCoastalCitiesByArea(CvArea const& kArea) const
{
	int iCount = 0;
	FOR_EACH_CITY(pLoopCity, *this)
	{
		if (pLoopCity->isCoastal())
		{
			if (pLoopCity->isArea(kArea) || pLoopCity->getPlot().isAdjacentToArea(kArea))
				iCount++;
		}
	}
	return iCount;
}


int CvPlayer::countTotalCulture() const
{
	int iCount = 0;
	FOR_EACH_CITY(pLoopCity, *this)
		iCount += pLoopCity->getCultureTimes100(getID());
	return iCount/100;
}


int CvPlayer::countCityFeatures(FeatureTypes eFeature) const
{
	PROFILE_FUNC();

	int iCount = 0;
	FOR_EACH_CITY(pLoopCity, *this)
	{
		for (CityPlotIter it(*pLoopCity); it.hasNext(); ++it)
		{
			if (it->getFeatureType() == eFeature)
				iCount++;
		}
	}
	return iCount;
}


int CvPlayer::countNumBuildings(BuildingTypes eBuilding) const
{
	PROFILE_FUNC();

	int iCount = 0;
	FOR_EACH_CITY(pLoopCity, *this)
	{
		if (pLoopCity->getNumBuilding(eBuilding) > 0)
			iCount += pLoopCity->getNumBuilding(eBuilding);
	}
	return iCount;
}


int CvPlayer::countNumCitiesConnectedToCapital() const
{
	int iCount = 0;
	FOR_EACH_CITY(pLoopCity, *this)
	{
		if (pLoopCity->isConnectedToCapital())
			iCount++;
	}
	return iCount;
}

// K-Mod:
bool CvPlayer::doesImprovementConnectBonus(ImprovementTypes eImprovement, BonusTypes eBonus) const
{
	return GET_TEAM(getTeam()).doesImprovementConnectBonus(eImprovement, eBonus);
}

// advc.905b:
bool CvPlayer::isSpeedBonusAvailable(BonusTypes eBonus, CvPlot const& kAt) const
{
	CvPlotGroup const* pPlotGroup = kAt.getPlotGroup(getID());
	if (pPlotGroup != NULL && pPlotGroup->getNumBonuses(eBonus) > 0)
		return true;
	return (hasCapital() && getCapital()->hasBonus(eBonus));
}

bool CvPlayer::canContact(PlayerTypes ePlayer, /* K-Mod: */ bool bCheckWillingness) const
{
	if (ePlayer == getID())
		return false;

	if (!isAlive() || !(GET_PLAYER(ePlayer).isAlive()))
		return false;

	if (isBarbarian() || GET_PLAYER(ePlayer).isBarbarian())
		return false;

	if (isMinorCiv() || GET_PLAYER(ePlayer).isMinorCiv())
		return false;

	if (getTeam() != TEAMID(ePlayer))
	{
		if (!GET_TEAM(getTeam()).isHasMet(TEAMID(ePlayer)))
			return false;

		if (::atWar(getTeam(), TEAMID(ePlayer)))
		{
			if (!GET_TEAM(getTeam()).canChangeWarPeace(TEAMID(ePlayer)))
				return false;
		}

		if (isHuman() || GET_PLAYER(ePlayer).isHuman())
		{
			if (GC.getGame().isOption(GAMEOPTION_ALWAYS_WAR))
				return false;
		}
	}

	// <K-Mod> (moved here by advc)
	if (bCheckWillingness)
		return (AI().AI_isWillingToTalk(ePlayer) && GET_PLAYER(ePlayer).AI_isWillingToTalk(getID()));
	// </K-Mod>

	return true;
}


void CvPlayer::contact(PlayerTypes ePlayer)
{
	if (!canContact(ePlayer) || isTurnDone())
		return;

	if (GET_PLAYER(ePlayer).isHuman())
	{
		if (GC.getGame().isPbem() || GC.getGame().isHotSeat() ||
			(GC.getGame().isPitboss() && !gDLL->isConnected(GET_PLAYER(ePlayer).getNetID())))
		{
			if (gDLL->isMPDiplomacy())
				gDLL->beginMPDiplomacy(ePlayer, false, false);
		}
		else
		{
			if (gDLL->UI().isFlashing(ePlayer))
			{
				if (!gDLL->UI().isDiplomacyLocked())
				{
					gDLL->UI().setDiplomacyLocked(true);
					gDLL->sendContactCiv(NETCONTACT_RESPONSE, ePlayer);
				}
			}
			else gDLL->sendContactCiv(NETCONTACT_INITIAL, ePlayer);
		}
	}
	else
	{
		CvDiploParameters* pDiplo = new CvDiploParameters(ePlayer);
		FAssert(pDiplo != NULL);
		if (GC.ctrlKey())
			pDiplo->setDiploComment(GC.getAIDiploCommentType("TRADING"));
		gDLL->UI().setDiploQueue(pDiplo, GC.getGame().getActivePlayer());
	}
}


void CvPlayer::handleDiploEvent(DiploEventTypes eDiploEvent, PlayerTypes ePlayer, int iData1, int iData2)
{
	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");

	switch (eDiploEvent)
	{
	case DIPLOEVENT_CONTACT:
		// advc.003j: Obsolete
		/*AI_setFirstContact(ePlayer, true);
		GET_PLAYER(ePlayer).AI_setFirstContact(getID(), true);*/
		break;

	case DIPLOEVENT_AI_CONTACT:
		break;

	case DIPLOEVENT_FAILED_CONTACT:
		// advc.003j: Obsolete
		/*AI_setFirstContact(ePlayer, true);
		GET_PLAYER(ePlayer).AI_setFirstContact(getID(), true);*/
		break;

	case DIPLOEVENT_GIVE_HELP:
		// advc.130j:
		AI().AI_rememberEvent(ePlayer, MEMORY_GIVE_HELP);
		/*	advc.104m: Was forcePeace(ePlayer) originally. The peace treaty is
			now part of the help deal. (Signing another one wouldn't hurt
			because of change advc.032; it's just unnecessary.) */
		//GET_TEAM(getTeam()).signPeaceTreaty(TEAMID(ePlayer));
		break;

	case DIPLOEVENT_REFUSED_HELP:
		// advc.130j:
		AI().AI_rememberEvent(ePlayer, MEMORY_REFUSED_HELP);
		break;

	case DIPLOEVENT_ACCEPT_DEMAND:
		// advc.130j:
		AI().AI_rememberEvent(ePlayer, MEMORY_ACCEPT_DEMAND);
		/*  advc.130o, advc.104: So that the AI can tell if a demand was
			_recently_ accepted. Add only 1 memory though. */
		GET_PLAYER(ePlayer).AI_changeMemoryCount(getID(), MEMORY_MADE_DEMAND_RECENT, 1);
		/*  advc (comment): This event (and its counterpart REJECTED_DEMAND)
			is only triggered when a human accepts an AI demand. When a human
			demands something from the AI, DIPLOEVENT_MADE_DEMAND triggers
			(and that one doesn't trigger when the AI makes a demand). */
		// advc.104m: (see note under DIPLOEVENT_GIVE_HELP above)
		//GET_TEAM(getTeam()).signPeaceTreaty(TEAMID(ePlayer));
		break;

	case DIPLOEVENT_REJECTED_DEMAND:
		FAssertMsg(GET_PLAYER(ePlayer).getTeam() != getTeam(), "shouldn't call this function on our own team");
		// advc.130j:
		AI().AI_rememberEvent(ePlayer, MEMORY_REJECTED_DEMAND);
		if (AI().AI_demandRebukedSneak(ePlayer))
			GET_TEAM(getTeam()).AI_setWarPlan(GET_PLAYER(ePlayer).getTeam(), WARPLAN_PREPARING_LIMITED);
		break;

	case DIPLOEVENT_DEMAND_WAR:
		FAssertMsg(GET_PLAYER(ePlayer).getTeam() != getTeam(), "shouldn't call this function on our own team");
		if (gTeamLogLevel >= 2) // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
			logBBAI("    Team %d (%S) declares war on team %d due to DIPLOEVENT", getTeam(), getCivilizationDescription(0), ePlayer);
		GET_TEAM(getTeam()).declareWar(GET_PLAYER(ePlayer).getTeam(), false, WARPLAN_LIMITED);
		break;

	case DIPLOEVENT_CONVERT:
		// advc.130j:
		AI().AI_rememberEvent(ePlayer, MEMORY_ACCEPTED_RELIGION);
		GET_PLAYER(ePlayer).convert(getStateReligion());
		break;

	case DIPLOEVENT_NO_CONVERT:
		// advc.130j:
		AI().AI_rememberEvent(ePlayer, MEMORY_DENIED_RELIGION);
		break;

	case DIPLOEVENT_REVOLUTION:
	{
		AI().AI_changeMemoryCount(ePlayer, MEMORY_ACCEPTED_CIVIC, 1);
		CivicMap aeNewCivics;
		GET_PLAYER(ePlayer).getCivics(aeNewCivics);
		CivicTypes eFavCivic = GC.getInfo(getPersonalityType()).getFavoriteCivic();
		aeNewCivics.set(GC.getInfo(eFavCivic).getCivicOptionType(), eFavCivic);
		GET_PLAYER(ePlayer).revolution(aeNewCivics, true);
		break;
	}
	case DIPLOEVENT_NO_REVOLUTION:
		// advc.130j:
		AI().AI_rememberEvent(ePlayer, MEMORY_DENIED_CIVIC);
		break;

	case DIPLOEVENT_JOIN_WAR:
		// advc.130j:
		AI().AI_rememberEvent(ePlayer, MEMORY_ACCEPTED_JOIN_WAR);
		// advc.146:
		GET_TEAM(getTeam()).signPeaceTreaty(TEAMID(ePlayer));
		if (gTeamLogLevel >= 2) // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
			logBBAI("    Team %d (%S) declares war on team %d due to DIPLOEVENT", getTeam(), getCivilizationDescription(0), ePlayer);
		GET_TEAM(ePlayer).declareWar((TeamTypes)iData1, false, WARPLAN_DOGPILE,
				true, getID()); // advc.100
		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			CvPlayerAI& kAttacked = GET_PLAYER((PlayerTypes)iI);
			if(!kAttacked.isAlive() || kAttacked.getTeam() != (TeamTypes)iData1)
				continue;
			// advc.130j:
			kAttacked.AI_rememberEvent(getID(), MEMORY_HIRED_WAR_ALLY);
		}
		break;

	case DIPLOEVENT_NO_JOIN_WAR:
		AI().AI_rememberEvent(ePlayer, MEMORY_DENIED_JOIN_WAR); // advc.130j
		break;

	case DIPLOEVENT_STOP_TRADING:
		AI().AI_rememberEvent(ePlayer, MEMORY_ACCEPTED_STOP_TRADING); // advc.130j
		GET_PLAYER(ePlayer).stopTradingWithTeam((TeamTypes)iData1);
		// <advc.130f> We also stop trading (unless ePlayer is our capitulated vassal)
		if(!GET_TEAM(ePlayer).isCapitulated() || !GET_TEAM(ePlayer).isVassal(getTeam()))
			stopTradingWithTeam((TeamTypes)iData1, false); // </advc.130f>
		for (MemberIter itMember((TeamTypes)iData1); itMember.hasNext(); ++itMember)
		{
			// advc (tbd.?): Should perhaps remember this about each member of this->getTeam()
			itMember->AI_rememberEvent(getID(), MEMORY_HIRED_TRADE_EMBARGO); // advc.130j
		}
		break;

	case DIPLOEVENT_NO_STOP_TRADING:
		AI().AI_rememberEvent(ePlayer, MEMORY_DENIED_STOP_TRADING); // advc.130j
		break;

	case DIPLOEVENT_ASK_HELP:
		// advc.130o: Now the same as DIPLOEVENT_MADE_DEMAND
		//break; // Don't break
	case DIPLOEVENT_MADE_DEMAND:
		/*  <advc.130o> Moved the handling of MEMORY_MADE_DEMAND (non-recent) to
			CvPlayerAI::AI_considerOffer */
		if (getTeam() != TEAMID(ePlayer)) // advc.155: Handled in AI_considerOffer
		{
			// <advc.130j>
			int iDemandRecentMem = AI().AI_getMemoryCount(ePlayer, MEMORY_MADE_DEMAND_RECENT);
			if (iDemandRecentMem <= 0)
				AI().AI_rememberEvent(ePlayer, MEMORY_MADE_DEMAND_RECENT);
			// Only remember it half if already remembered as recent, and cap at 2.
			else if (iDemandRecentMem < 2)
				AI().AI_changeMemoryCount(ePlayer, MEMORY_MADE_DEMAND_RECENT, 1);
		} // </advc.130o> </advc.130j>
		// <advc.144>
		if (iData1 > 0) // Let proxy AI remember when a human request is granted
		{
			GET_PLAYER(ePlayer).AI_rememberEvent(getID(), // advc.130j
					eDiploEvent == DIPLOEVENT_ASK_HELP ?
					MEMORY_GIVE_HELP : MEMORY_ACCEPT_DEMAND);
		} // </advc.144>
		break;

	case DIPLOEVENT_MADE_DEMAND_VASSAL:
		break;

	case DIPLOEVENT_RESEARCH_TECH:
		pushResearch((TechTypes)iData1, true);
		break;

	case DIPLOEVENT_TARGET_CITY:
	{
		CvCityAI* pCity = GET_PLAYER((PlayerTypes)iData1).AI_getCity(iData2);
		if (pCity != NULL)
		{
			pCity->getArea().AI_setTargetCity(getID(),
					// advc.001: Set no target if the requested target isn't revealed
					pCity->isRevealed(getTeam()) ? pCity : NULL);
			AI().AI_setCityTargetTimer(GC.getDefineINT(CvGlobals::PEACE_TREATY_LENGTH)); // K-Mod
		}
		break;
	} // K-Mod
	case DIPLOEVENT_SET_WARPLAN:
	{
		CvTeamAI& kOurTeam = GET_TEAM(getTeam());
		FAssert(kOurTeam.getNumWars() <= 0);
		if (iData1 == NO_TEAM)
		{
			FAssert(iData2 == NO_WARPLAN);
			for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
			{
				//if (!kOurTeam.isAtWar(i)) // advc: Redundant b/c of bWar=false param
				kOurTeam.AI_setWarPlan(i, NO_WARPLAN, false);
			}
		}
		else kOurTeam.AI_setWarPlan((TeamTypes)iData1, (WarPlanTypes)iData2, false);
		break;
	} // K-Mod end

	default:
		FAssert(false);
		break;
	}
}


bool CvPlayer::canTradeWith(PlayerTypes eWhoTo) const
{
	// advc: Team-level checks moved to new CvTeam function
	if(!GET_TEAM(getTeam()).canTradeWith(TEAMID(eWhoTo)) && !canTradeNetworkWith(eWhoTo))
		return false;
	// <advc.ctr>
	CLinkList<TradeData> items;
	// Speed is not an issue; canTradeWith is only called during human-AI diplo.
	buildTradeTable(eWhoTo, items);
	if(items.getLength() > 0)
		return true;
	items.clear();
	GET_PLAYER(eWhoTo).buildTradeTable(getID(), items);
	return (items.getLength() > 0); // </advc.ctr>
}


bool CvPlayer::canReceiveTradeCity() const
{
	return (!GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) || !isHuman());
}


bool CvPlayer::canTradeItem(PlayerTypes eWhoTo, TradeData item, bool bTestDenial) const  // advc: assertions added.
{
	PROFILE_FUNC(); // advc: To keep an eye on items that aren't profiled separately

	/*  <advc.opt> Moved the clauses that don't depend on item.m_iData into a
		subroutine so that client code can check them upfront before e.g.
		calling canTradeItem for every technology. */
	if (!canPossiblyTradeItem(eWhoTo, item.m_eItemType))
		return false; // </advc.opt>

	bool bValid = false; // advc.opt: TradeDenial check moved down
	CvTeam const& kOurTeam = GET_TEAM(getTeam());
	CvTeam const& kToTeam = GET_TEAM(eWhoTo);
	switch (item.m_eItemType)
	{
	case TRADE_TECHNOLOGIES:
	{
		PROFILE("CvPlayer::canTradeItem.TECH");
		TechTypes eTech = (TechTypes)item.m_iData;
		FAssertEnumBounds(eTech);
		if (GC.getInfo(eTech).isTrade() && kOurTeam.isHasTech(eTech) &&
			!kOurTeam.isNoTradeTech(eTech) && !kToTeam.isHasTech(eTech) &&
			GET_PLAYER(eWhoTo).canResearch(eTech, true))
		{
			bValid = true;
		}
		break;
	}
	case TRADE_RESOURCES:
	{
		//PROFILE("CvPlayer::canTradeItem.RESOURCES");
		BonusTypes eBonus = (BonusTypes)item.m_iData;
		FAssertEnumBounds(eBonus);
		if (!kToTeam.isBonusObsolete(eBonus) &&
			!kOurTeam.isBonusObsolete(eBonus))
		{
			//bCanTradeAll=(isHuman() || getTeam() == TEAMID(eWhoTo) || kOurTeam.isVassal(kToTeam.getID()));
			bool const bCanTradeAll = true; // advc.036
			if (getNumTradeableBonuses(eBonus) > (bCanTradeAll ? 0 : 1))
			{	// if (GET_PLAYER(eWhoTo).getNumAvailableBonuses(eBonus) == 0)
				bValid = true;
			}
		}
		break;
	}
	case TRADE_CITIES:
	{
		CvCity const* pCity = getCity(item.m_iData);
		if (pCity == NULL)
		{
			FAssert(pCity != NULL);
			break;
		}
		bValid = canTradeCityTo(eWhoTo, *pCity); // advc.ctr;
		break;
	}
	case TRADE_GOLD:
		FAssert(item.m_iData >= 0); // (advc: 0 is OK as an unspecified amount)
		if (getGold() >= item.m_iData)
			bValid = true;
		break;
	case TRADE_GOLD_PER_TURN:
		FAssert(item.m_iData >= 0);
		bValid = true;
		break;
	case TRADE_MAPS:
		bValid = true;
		break;
	case TRADE_VASSAL:
		// advc.112: Make sure that only capitulation is possible between war enemies
		if (!kToTeam.isAtWar(getTeam()))
			bValid = true;
	case TRADE_SURRENDER:
	{
		bool bForce = (item.m_iData == 1); // Used by CvDeal::startTeamTrade
		if ((kToTeam.isAtWar(getTeam()) || bForce) && item.m_eItemType == TRADE_SURRENDER)
			bValid = true;
		break;
	}
	case TRADE_PEACE:
	{
		FAssertBounds(0, MAX_CIV_TEAMS, item.m_iData);
		TeamTypes eTargetTeam = (TeamTypes)item.m_iData;
		if (kToTeam.isHasMet(eTargetTeam) && //kOurTeam.isHasMet(eTargetTeam) && // advc: redundant
			kOurTeam.isAtWar(eTargetTeam))
		{
			bValid = true;
		}
		break;
	}
	case TRADE_WAR:
	{
		FAssertBounds(0, MAX_CIV_TEAMS, item.m_iData);
		TeamTypes eTargetTeam = (TeamTypes)item.m_iData;
		if (!GET_TEAM(eTargetTeam).isAVassal() &&
			kOurTeam.isHasMet(eTargetTeam) && kToTeam.isHasMet(eTargetTeam) &&
			kOurTeam.canDeclareWar(eTargetTeam))
		{
			bValid = true;
		}
		break;
	}
	case TRADE_EMBARGO:
	{
		//PROFILE("CvPlayer::canTradeItem.EMBARGO");
		FAssertBounds(0, MAX_CIV_TEAMS, item.m_iData);
		TeamTypes eTargetTeam = (TeamTypes)item.m_iData;
		if (!kOurTeam.isHuman() &&
			kOurTeam.isHasMet(eTargetTeam) && kToTeam.isHasMet(eTargetTeam) &&
			canStopTradingWithTeam(eTargetTeam) &&
			// <advc.130f>
			(!GET_PLAYER(eWhoTo).isTradingWithTeam(eTargetTeam, true) ||
			(kOurTeam.isCapitulated() && kOurTeam.isVassal(kToTeam.getID()))))
			// </advc.130f>
		{
			bValid = true;
		}
		break;
	}
	case TRADE_CIVIC:
	{
		//PROFILE("CvPlayer::canTradeItem.CIVIC");
		CivicTypes eCivic = (CivicTypes)item.m_iData;
		FAssertEnumBounds(eCivic);
		if (GET_PLAYER(eWhoTo).isCivic(eCivic) /* <advc.132> */ ||
			/*  canForceCivic double-checks everything checked here already,
				plus some clauses that I've added there. */
			((kOurTeam.isVassal(kToTeam.getID()) ||
			kOurTeam.isAtWar(kToTeam.getID())) &&
			GET_PLAYER(eWhoTo).canForceCivics(getID(), eCivic))) // </advc.132>
		{
			if (canDoCivics(eCivic) && !isCivic(eCivic) && canDoAnyRevolution())
				bValid = true;
		}
		break;
	}
	case TRADE_RELIGION:
	{
		//PROFILE("CvPlayer::canTradeItem.RELIGION");
		ReligionTypes eReligion = (ReligionTypes)item.m_iData;
		FAssertEnumBounds(eReligion);
		if (GET_PLAYER(eWhoTo).getStateReligion() == eReligion /* <advc.132> */ ||
			// Same thing as for civics above
			((kOurTeam.isVassal(kToTeam.getID()) ||
			kOurTeam.isAtWar(kToTeam.getID())) &&
			GET_PLAYER(eWhoTo).canForceReligion(getID(), eReligion))) // </advc.132>
		{
			if (canConvert(eReligion))
				bValid = true;
		}
		break;
	}
	// advc.opt: The rest is handled by canPossiblyTradeItem
	case TRADE_OPEN_BORDERS:
	case TRADE_DEFENSIVE_PACT:
	case TRADE_PERMANENT_ALLIANCE:
	case TRADE_PEACE_TREATY:
	// <advc.034>
	case TRADE_DISENGAGE:
		bValid = true; // </advc.034>
	}
	// advc.opt: (denial check moved down)
	return (bValid && (!bTestDenial || getTradeDenial(eWhoTo, item) == NO_DENIAL));
}

/*  advc.opt: Cut from canTradeItem.
	'False' guarantees that no trade is possible, but 'true' only means that some
	trade might be possible. */
bool CvPlayer::canPossiblyTradeItem(PlayerTypes eWhoTo, TradeableItems eItemType) const // advc.opt
{
	PROFILE_FUNC();
	CvTeam const& kOurTeam = GET_TEAM(getTeam());
	CvTeam const& kToTeam = GET_TEAM(eWhoTo);
	switch (eItemType)
	{
	case TRADE_TECHNOLOGIES:
		return (!GC.getGame().isOption(GAMEOPTION_NO_TECH_TRADING) &&
				(kOurTeam.isTechTrading() || kToTeam.isTechTrading()));
	case TRADE_RESOURCES: return canTradeNetworkWith(eWhoTo);
	case TRADE_CITIES:
		return (GET_PLAYER(eWhoTo).canReceiveTradeCity() &&
				GC.getGame().getMaxCityElimination() <= 0);
	case TRADE_GOLD:
	case TRADE_GOLD_PER_TURN:
		return (kOurTeam.isGoldTrading() || kToTeam.isGoldTrading());
	case TRADE_MAPS:
		return ((getTeam() != kToTeam.getID()) &&
				(kOurTeam.isMapTrading() || kToTeam.isMapTrading()));
	case TRADE_VASSAL:
	case TRADE_SURRENDER:
	{
		static bool const bBBAI_HUMAN_AS_VASSAL_OPTION = GC.getDefineBOOL("BBAI_HUMAN_AS_VASSAL_OPTION"); // advc.opt
		if (isHuman() && !GET_PLAYER(eWhoTo).isHuman() &&
			!bBBAI_HUMAN_AS_VASSAL_OPTION) // BETTER_BTS_AI_MOD, Customization, 12/06/09, jdog5000
		{
			return false;
		}
		if (!kToTeam.isVassalStateTrading()) // the master must possess the tech
			return false;
		if (kOurTeam.isAVassal() || kToTeam.isAVassal() || getTeam() == kToTeam.getID())
			return false;
		if (!kToTeam.isAtWar(getTeam()) && eItemType == TRADE_VASSAL)
			return true;
		// TRADE_SURRENDER has an override for peacetime; can't check that here.
		return true;
	}
	case TRADE_WAR:
		if (kOurTeam.isAtWar(kToTeam.getID())) // advc.100
			return false;
		// fall through
	case TRADE_PEACE:
		return (!kOurTeam.isHuman() && !kOurTeam.isAVassal() &&
				!kToTeam.isCapitulated()); // advc.130v: Make sure that capitulated vassals can't broker war/ peace
	case TRADE_EMBARGO:
		return !kToTeam.isCapitulated(); // advc.130v: Make sure that capitulated vassals can't negotiate embargoes
	case TRADE_RELIGION:
	case TRADE_CIVIC:
		//if (!ourTeam.isHuman()
				// UNOFFICIAL_PATCH, Diplomacy, 10/22/09, denev & jdog5000
				//|| getTeam() == theirTeam.getID())
		// <advc.155> Replacing the patched code above
		return ((getTeam() == kToTeam.getID()) ?
			(!isHuman() || GET_PLAYER(eWhoTo).isHuman()) :
			/*  Don't allow humans from one team to manipulate the civics/ religion
				of AI civs on another human team. */
			!kOurTeam.isHuman()); // </advc.155>
	case TRADE_OPEN_BORDERS:
		return (getTeam() != kToTeam.getID() && !kOurTeam.isAtWar(kToTeam.getID()) &&
				!kOurTeam.isOpenBorders(kToTeam.getID()) &&
				(kOurTeam.isOpenBordersTrading() || kToTeam.isOpenBordersTrading()));
	case TRADE_DEFENSIVE_PACT:
		if (!kOurTeam.isAVassal() && !kToTeam.isAVassal() &&
			getTeam() != kToTeam.getID() && //!kToTeam.isVassal(getTeam()) // advc: redundant
			!kOurTeam.isAtWar(kToTeam.getID()) &&
			!kOurTeam.isDefensivePact(kToTeam.getID()) &&
			(kOurTeam.isDefensivePactTrading() || kToTeam.isDefensivePactTrading()))
		{
			/*  <kekm.3> 'Added possibility of signing defensive pact while in war
				if BBAI defensive pact option is >= 1' */
			if ((kOurTeam.getNumWars() <= 0 && kToTeam.getNumWars() <= 0) ||
				(GC.getDefineINT(CvGlobals::BBAI_DEFENSIVE_PACT_BEHAVIOR) >= 1
				/*  advc: Prohibit DP when not all wars shared?
					Enough to have the AI refuse such pacts I think
					(in CvTeamAI::AI_defensivePactTrade). */
					//&& GET_TEAM(getID()).allWarsShared(theirTeam.getID())
				)) // </kekm.3>
			{
				if (kOurTeam.canSignDefensivePact(kToTeam.getID()))
					return true;
			}
		}
		return false;
	case TRADE_PERMANENT_ALLIANCE:
		return (!kOurTeam.isAVassal() && !kToTeam.isAVassal() &&
				getTeam() != kToTeam.getID() && //!kToTeam.isVassal(getTeam()) // advc: redundant
				!kOurTeam.isAtWar(kToTeam.getID()) &&
				#ifndef TEST_PERMANENT_ALLIANCES // advc.test
				(kOurTeam.isPermanentAllianceTrading() || kToTeam.isPermanentAllianceTrading()) &&
				#endif
				kOurTeam.getNumMembers() == 1 && kToTeam.getNumMembers() == 1);
	case TRADE_PEACE_TREATY:
		return kOurTeam.canChangeWarPeace(kToTeam.getID()); // advc.130v
	// <advc.034>
	case TRADE_DISENGAGE:
		return (!kToTeam.isDisengage(getTeam()) &&
				!kToTeam.isAtWar(getTeam()) &&
				!kToTeam.isOpenBorders(getTeam()) &&
				!kToTeam.isVassal(getTeam()) &&
				!kOurTeam.isVassal(kToTeam.getID()) &&
				kToTeam.isHasMet(getTeam()));
	// </advc.034>
	default:
		FErrorMsg("Unknown trade item type");
		return false;
	}
}

// advc.ctr:
bool CvPlayer::canTradeCityTo(PlayerTypes eRecipient, CvCity const& kCity, bool bConquest) const
{
	PROFILE_FUNC(); // advc: Fine so far; not frequently called.
	CvPlayer const& kRecipient = GET_PLAYER(eRecipient);
	/*	canTradeItem already checks this through canPossiblyTradeItem, so this
		is redundant. However, I also need to check the city trade conditions
		from CvPlayer::acquireCity, so this can't be helped unless I add the
		bConquest param to canTradeItem, which I find inelegant. */
	if (!kRecipient.canReceiveTradeCity())
		return false;

	if (kCity.isCapital() || !kCity.isRevealed(kRecipient.getTeam()))
		return false;
	if (kCity.getLiberationPlayer(bConquest) == eRecipient) // as in BtS
		return true;
	/*	Can't trade so long as the previous owner hasn't accepted the loss
		(let's ignore kCity.getOriginalOwner()) */
	PlayerTypes ePreviousOwner = kCity.getPreviousOwner();
	if (ePreviousOwner != NO_PLAYER)
	{
		TeamTypes ePreviousTeam = TEAMID(ePreviousOwner);
		if (ePreviousTeam != kRecipient.getTeam() &&
			GET_TEAM(getTeam()).isAtWar(ePreviousTeam) &&
			!GET_TEAM(eRecipient).isAtWar(ePreviousTeam))
		{
			return false;
		}
	}
	CvPlot const& kCityPlot = *kCity.plot();
	if (kCityPlot.calculateCulturePercent(eRecipient) <
		GC.getDefineINT(CvGlobals::CITY_TRADE_CULTURE_THRESH))
	{
		return false;
	}
	if (!GET_TEAM(eRecipient).isAtWar(getTeam()) &&
		// Prevent back-and-forth trades between humans
		(kCity.isEverOwned(eRecipient) && isHuman() &&
		kRecipient.isHuman() ? 1 : 2) * kCityPlot.getCulture(eRecipient) <=
		kCityPlot.getCulture(getID()))
	{
		return false;
	}
	if (GET_TEAM(eRecipient).isVassal(getTeam()) &&
		kCityPlot.getCulture(eRecipient) <= kCityPlot.getCulture(getID()))
	{
		return false;
	}
	return true;
}


DenialTypes CvPlayer::getTradeDenial(PlayerTypes eWhoTo, TradeData item) const
{
	PROFILE_FUNC(); // (advc: BONUS_TRADE is the only -minor- concern currently)
	const CvTeamAI& kOurTeam = GET_TEAM(getTeam()); // K-Mod

	// K-Mod note: I've changed it so that AI players on human teams can be contacted when not at war.
	// So.. as a follow up on that change, I'm making the AI deny trades which affect the team, not just the player.
	switch (item.m_eItemType)
	{
	case TRADE_TECHNOLOGIES:
		// K-Mod
		if (!isHuman() && kOurTeam.isHuman())
			return DENIAL_MYSTERY;
		// K-Mod end
		return kOurTeam.AI_techTrade((TechTypes)item.m_iData, TEAMID(eWhoTo));

	case TRADE_RESOURCES:
		return AI().AI_bonusTrade((BonusTypes)item.m_iData, eWhoTo,
				1); // advc.036

	case TRADE_CITIES:
	{
		CvCityAI* pCity = AI().AI_getCity(item.m_iData);
		if (pCity != NULL)
			return AI().AI_cityTrade(*pCity, eWhoTo);
		break;
	}
	case TRADE_GOLD:
	case TRADE_GOLD_PER_TURN:
		break;

	case TRADE_MAPS:
		return kOurTeam.AI_mapTrade(TEAMID(eWhoTo));

	case TRADE_SURRENDER:
		return kOurTeam.AI_surrenderTrade(TEAMID(eWhoTo),
				// advc.104o: No functional change
				CvTeamAI::VASSAL_POWER_MOD_SURRENDER);

	case TRADE_VASSAL:
		// K-Mod
		if (!isHuman() && kOurTeam.isHuman())
			return DENIAL_MYSTERY; // K-Mod end
		return kOurTeam.AI_vassalTrade(TEAMID(eWhoTo));

	case TRADE_PEACE:
		return kOurTeam.AI_makePeaceTrade((TeamTypes)item.m_iData, TEAMID(eWhoTo));

	case TRADE_WAR:
		return kOurTeam.AI_declareWarTrade((TeamTypes)item.m_iData, TEAMID(eWhoTo));

	case TRADE_EMBARGO:
		return AI().AI_stopTradingTrade((TeamTypes)item.m_iData, eWhoTo);

	case TRADE_CIVIC:
		return AI().AI_civicTrade((CivicTypes)item.m_iData, eWhoTo);

	case TRADE_RELIGION:
		return AI().AI_religionTrade((ReligionTypes)item.m_iData, eWhoTo);

	case TRADE_OPEN_BORDERS:
		return kOurTeam.AI_openBordersTrade(TEAMID(eWhoTo));

	case TRADE_DEFENSIVE_PACT:
		// K-Mod
		if (!isHuman() && kOurTeam.isHuman())
			return DENIAL_MYSTERY;
		// K-Mod end
		return kOurTeam.AI_defensivePactTrade(TEAMID(eWhoTo));

	case TRADE_PERMANENT_ALLIANCE:
		// K-Mod
		if (!isHuman() && kOurTeam.isHuman())
			return DENIAL_MYSTERY;
		// K-Mod end
		return kOurTeam.AI_permanentAllianceTrade(TEAMID(eWhoTo));
	/*	advc.ctr: Disabled. I don't think this case can occur in K-Mod
		(future-proofing?), but it can get in the way of implied peace treaties,
		which the AI evaluates more properly under the trade item that mandates
		the peace treaty. */
	/*case TRADE_PEACE_TREATY:
		// K-Mod
		if (kOurTeam.AI_refusePeace(TEAMID(eWhoTo)))
			return DENIAL_VICTORY; // K-Mod end
		break;*/
	}

	return NO_DENIAL;
}


bool CvPlayer::canTradeNetworkWith(PlayerTypes ePlayer) const
{
	// <advc.124>
	PROFILE_FUNC();
	CvCity const* pOurCapital = getCapital();
	CvPlayer const& kThey = GET_PLAYER(ePlayer);
	CvCity const* pTheirCapital = kThey.getCapital();
	if(pOurCapital != NULL && pTheirCapital != NULL)
	{
		FOR_EACH_CITY(c, kThey)
		{
			if(pOurCapital->isConnectedTo(c) && c->isConnectedToCapital(ePlayer))
				return true;
		}
		FOR_EACH_CITY(c, *this)
		{
			if(pTheirCapital->isConnectedTo(c) && c->isConnectedToCapital(getID()))
				return true;
		}
		// Replaced BtS code:
		/*if (pOurCapital->isConnectedToCapital(ePlayer))
			return true;*/
	} // </advc.124>

	return false;
}


int CvPlayer::getNumAvailableBonuses(BonusTypes eBonus) const
{
	CvPlotGroup* pPlotGroup = (hasCapital() ?
			getCapital()->getPlot().getOwnerPlotGroup() : NULL);
	if (pPlotGroup != NULL)
		return pPlotGroup->getNumBonuses(eBonus);

	return 0;
}


int CvPlayer::getNumTradeableBonuses(BonusTypes eBonus) const
{
	return (getNumAvailableBonuses(eBonus) - getBonusImport(eBonus));
}

bool CvPlayer::hasBonus(BonusTypes eBonus) const
{
	FOR_EACH_CITY(pLoopCity, *this)
	{
		if (pLoopCity->hasBonus(eBonus))
			return true;
	}
	return false;
}

int CvPlayer::getNumTradeBonusImports(PlayerTypes eFromPlayer) const // advc: Simplified using the new CvDeal interface
{
	FAssert(eFromPlayer != getID());
	int iCount = 0;
	FOR_EACH_DEAL(pLoopDeal)
	{
		if (!pLoopDeal->isBetween(getID(), eFromPlayer))
			continue;
		FOR_EACH_TRADE_ITEM(pLoopDeal->getGivesList(eFromPlayer))
		{
			if (pItem->m_eItemType == TRADE_RESOURCES)
				iCount++;
		}
	}
	return iCount;
}

/*  advc: Renamed parameters. Note about the semantics (same as in BtS):
	- This functions checks only whether we're giving something to eToTeam, not vice versa.
	- It checks if this player is trading with any member of eToTeam. Likewise,
	  an embargo will affect only the enacting player - but the entire target team.
	Body simplified using the new CvDeal interface. */
bool CvPlayer::isTradingWithTeam(TeamTypes eToTeam, bool bIncludeUncancelable) const
{
	if (eToTeam == getTeam())
		return false;

	FOR_EACH_DEAL(pLoopDeal)
	{
		if (!pLoopDeal->isBetween(getID(), eToTeam))
			continue;
		// advc: Un(!)cancelable
		if (!bIncludeUncancelable && !pLoopDeal->isCancelable(getID()))
			continue;

		if (pLoopDeal->getGivesList(getID()).getLength() > 0 && !pLoopDeal->isPeaceDeal()
				&& !pLoopDeal->isDisengage()) // advc.034
			return true;
	}
	return false;
}


bool CvPlayer::canStopTradingWithTeam(TeamTypes eTeam, bool bContinueNotTrading) const
{
	// <advc.130d> Replacing the two conditions below.
	if(getMasterTeam() == GET_TEAM(eTeam).getMasterTeam())
		return false; // </advc.130d>
	/*if (eTeam == getTeam())
		return false;
	if (GET_TEAM(getTeam()).isVassal(eTeam))
		return false;*/
	if(bContinueNotTrading)
	{
		// <advc.130f>
		// Allow resolutions to overrule turns-to-cancel
		return true;
	}
	return isTradingWithTeam(eTeam, true);
	// BtS code: // </advc.130f>
	/*if (!isTradingWithTeam(eTeam, false)) {
		if (bContinueNotTrading && !isTradingWithTeam(eTeam, true))
			return true;
		return false;
	}
	return true;*/
}


void CvPlayer::stopTradingWithTeam(TeamTypes eTeam, /* advc.130f: */ bool bDiploPenalty)
{
	FAssert(eTeam != getTeam());
	bool bDealCanceled = false; // advc.130f
	FOR_EACH_DEAL_VAR(pLoopDeal)
	{
		if (!pLoopDeal->isBetween(getID(), eTeam))
			continue;

		if (pLoopDeal->isEverCancelable(getID()) // advc.130f
			&& !pLoopDeal->isPeaceDeal()
			&& !pLoopDeal->isDisengage()) // advc.034
		{
			pLoopDeal->kill();
			bDealCanceled = true; // advc.130f
		}
	}
	// <advc.130f>
	if(!bDealCanceled && bDiploPenalty)
		return;
	// </advc.130f>
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++) // advc.003n: was MAX_PLAYERS
	{   CvPlayerAI& kTargetMember = GET_PLAYER((PlayerTypes)iI); // advc
		if (!kTargetMember.isAlive() || kTargetMember.getTeam() != eTeam)
			continue;
		// <advc.130j>
		if(bDiploPenalty) // advc.130f: RECENT causes only refusal to talk
			kTargetMember.AI_rememberEvent(getID(), MEMORY_STOPPED_TRADING);
		// Don't rememberEvent - not supposed to be based on attitude
		kTargetMember.AI_setMemoryCount(getID(), MEMORY_STOPPED_TRADING_RECENT, 2);
	} // </advc.130j>
}

// advc.130f:
bool CvPlayer::isAnyDealTooRecentToCancel(TeamTypes eTeam) const
{
	FOR_EACH_DEAL(d)
	{
		if(d->isBetween(getID(), eTeam) && !d->isCancelable(getID()) &&
			!d->isPeaceDeal() && !d->isDisengage()) // advc.034
		{
			return true;
		}
	}
	return false;
}

void CvPlayer::killAllDeals()
{
	FOR_EACH_DEAL_VAR(pLoopDeal)
	{
		if (pLoopDeal->involves(getID()))
			pLoopDeal->kill();
	}
}


void CvPlayer::findNewCapital()
{
	BuildingTypes const eCapitalBuilding = getCivilization().getBuilding((BuildingClassTypes)
			GC.getDefineINT("CAPITAL_BUILDINGCLASS"));
	if (eCapitalBuilding == NO_BUILDING)
		return;

	CvCity* pOldCapital = getCapital();

	int iBestValue = 0;
	CvCity* pBestCity = NULL;
	FOR_EACH_CITY_VAR(pLoopCity, *this)
	{
		if (pLoopCity == pOldCapital ||
			pLoopCity->getNumRealBuilding(eCapitalBuilding) != 0)
		{
			continue;
		}
		int iValue = pLoopCity->getPopulation() * 4;
		iValue += pLoopCity->getYieldRate(YIELD_FOOD);
		iValue += (pLoopCity->getYieldRate(YIELD_PRODUCTION) * 3);
		iValue += (pLoopCity->getYieldRate(YIELD_COMMERCE) * 2);
		iValue += pLoopCity->getCultureLevel();
		iValue += pLoopCity->getReligionCount();
		iValue += pLoopCity->getCorporationCount();
		iValue += (pLoopCity->getNumGreatPeople() * 2);

		iValue *= (pLoopCity->calculateCulturePercent(getID()) + 100);
		iValue /= 100;

		if (iValue > iBestValue)
		{
			iBestValue = iValue;
			pBestCity = pLoopCity;
		}
	}

	if (pBestCity != NULL)
	{
		FAssert(pBestCity->getNumRealBuilding(eCapitalBuilding) == 0);
		pBestCity->setNumRealBuilding(eCapitalBuilding, 1);
		// advc.106: Moved down so that setCapital can announce the old capital
		if (pOldCapital != NULL)
			pOldCapital->setNumRealBuilding(eCapitalBuilding, 0);
	}
}


int CvPlayer::getNumGovernmentCenters() const
{
	int iCount = 0;
	FOR_EACH_CITY(pLoopCity, *this)
	{
		if (pLoopCity->isGovernmentCenter())
			iCount++;
	}
	return iCount;
}


bool CvPlayer::canRaze(CvCity const& kCity) const // advc: param was CvCity*
{
	if (!kCity.isAutoRaze())
	{
		if (GC.getGame().isOption(GAMEOPTION_NO_CITY_RAZING))
			return false;

		if (kCity.getOwner() != getID())
			return false;
		static int const iRAZING_CULTURAL_PERCENT_THRESHOLD = GC.getDefineINT("RAZING_CULTURAL_PERCENT_THRESHOLD"); // advc.opt
		if (kCity.calculateTeamCulturePercent(getTeam()) >= iRAZING_CULTURAL_PERCENT_THRESHOLD)
			return false;
	}
	/*  advc.003y (note): This Python function is also called from acquireCity.
		acquireCity should arguably call CvPlayer::canRaze instead. But that seems
		difficult to untangle. */
	if (!GC.getPythonCaller()->canRaze(kCity, getID()))
		return false;

	return true;
}


void CvPlayer::raze(CvCity& kCity) // advc: param was CvCity*
{
	if (!canRaze(kCity))
		return;

	FAssert(kCity.getOwner() == getID());

	AI().AI_processRazeMemory(kCity); // advc.003n: Moved into subroutine

	wchar szBuffer[1024];
	swprintf(szBuffer, gDLL->getText("TXT_KEY_MISC_DESTROYED_CITY",
			kCity.getNameKey()).GetCString());
	gDLL->UI().addMessage(getID(), true, -1, szBuffer, kCity.getPlot(),
			"AS2D_CITYRAZE", MESSAGE_TYPE_MAJOR_EVENT,
			ARTFILEMGR.getInterfaceArtPath("WORLDBUILDER_CITY_EDIT"),
			GC.getColorType("GREEN"));

	for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
	{
		CvPlayer const& kObs = *it;
		if (kObs.getID() == getID())
			continue;
		if (kCity.isRevealed(kObs.getTeam()) /* advc.127: */ || kObs.isSpectator())
		{
			swprintf(szBuffer, gDLL->getText("TXT_KEY_MISC_CITY_HAS_BEEN_RAZED_BY",
					kCity.getNameKey(), getCivilizationDescriptionKey()).GetCString());
			gDLL->UI().addMessage(kObs.getID(), false, -1, szBuffer, kCity.getPlot(),
					"AS2D_CITYRAZED",
					MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY, // advc.106b
					ARTFILEMGR.getInterfaceArtPath("WORLDBUILDER_CITY_EDIT"),
					GC.getColorType("RED"));
		}
	}

	swprintf(szBuffer, gDLL->getText("TXT_KEY_MISC_CITY_RAZED_BY",
			kCity.getNameKey(), getCivilizationDescriptionKey()).GetCString());
	GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szBuffer,
			kCity.getX(), kCity.getY(), GC.getColorType("WARNING_TEXT"));

	kCity.doPartisans(); // advc.003y
	CvEventReporter::getInstance().cityRazed(&kCity, getID());
	CvPlot const& kCityPlot = *kCity.plot(); // advc.130w
	disband(kCity);
	AI().AI_updateCityAttitude(kCityPlot); // advc.130w
}


void CvPlayer::disband(CvCity& kCity) // advc: param was CvCity*
{
	if (getNumCities() == 1)
		setFoundedFirstCity(false);
	GC.getGame().addDestroyedCityName(kCity.getName());
	kCity.kill(true);
}


bool CvPlayer::canReceiveGoody(CvPlot* pPlot, GoodyTypes eGoody, CvUnit* pUnit) const
{
	CvGoodyInfo const& kGoody = GC.getInfo(eGoody); // advc
	// <advc.314>
	CvGame const& kGame = GC.getGame();
	int const iTrainHalved = (GC.getInfo(kGame.getGameSpeedType()).
			getTrainPercent() + 100) / 2;
	bool bVeryEarlyGame = (100 * kGame.getGameTurn() < 20 * iTrainHalved);
	bool bVeryVeryEarlyGame = (100 * kGame.getGameTurn() < 10 * iTrainHalved);
	if (kGoody.getExperience() > 0)
	{
		if (pUnit == NULL || !pUnit->canAcquirePromotionAny() ||
			//(GC.getGame().getElapsedGameTurns() < 10)
			bVeryVeryEarlyGame) // advc.314
		{
			return false;
		}
	}

	if (kGoody.getDamagePrereq() > 0)
	{
		if (pUnit == NULL || pUnit->getDamage() < (pUnit->maxHitPoints() *
			kGoody.getDamagePrereq()) / 100)
		{
			return false;
		}
	}
	if (kGoody.isTech())
	{
		bool bTechFound = false;
		FOR_EACH_ENUM(Tech)
		{
			/*if (GC.getInfo((TechTypes) iI).isGoodyTech()) {
				if (canResearch((TechTypes)iI), false, true)*/ // K-Mod
			// <advc.314>
			if(isGoodyTech(eLoopTech,
				kGoody.getGold() + kGoody.getGoldRand1() + kGoody.getGoldRand2()))
			{	// </advc.314>
				if (canResearch(eLoopTech, false, true)) // advc
				{
					bTechFound = true;
					break;
				}
			}
		}
		if (!bTechFound)
			return false;
	}

	if (kGoody.isBad())
	{
		if (pUnit == NULL || pUnit->isNoBadGoodies())
			return false;
	}
	// <advc.314>
	if(bVeryEarlyGame && kGoody.getMinBarbarians() > 1)
		return false;
	/*  Moved up and added the era clause; a single free unit in the Medieval
		era isn't going to be a problem. */
	bool bEarlyMP = (GC.getGame().isGameMultiPlayer() &&
			GC.getGame().getCurrentEra() < 2);
	// No free unit from Spy when hut guarded
	if(!kGoody.isBad() && (kGoody.getUnitClassType() != NO_UNITCLASS ||
		kGoody.getMinBarbarians() > 0) && pPlot->isVisibleEnemyUnit(getID()) &&
		pUnit != NULL && pUnit->isSpy())
	{
		return false;
	}
	UnitClassTypes eGoodyUnitClass = (UnitClassTypes)kGoody.getUnitClassType();
	if (eGoodyUnitClass!= NO_UNITCLASS)
	{
		UnitTypes eUnit = getCivilization().getUnit(eGoodyUnitClass);
		if (eUnit == NO_UNIT)
			return false;
		CvUnitInfo const& kUnit = GC.getInfo(eUnit); // advc
		if (kUnit.getCombat() > 0 && /* advc.315: */ !kUnit.isMostlyDefensive())
		{
			if (//kGame.getElapsedGameTurns() < 20
				bEarlyMP || bVeryEarlyGame) // advc.314
			{
				return false;
			}
		} // <advc.314> I guess a Worker with a slow WorkRate would be OK
		if(bVeryEarlyGame && kUnit.getWorkRate() > 30)
			return false; // </advc.314>
		if (GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
		{
			if (kUnit.isFound())
				return false;
		}
	} // <advc.314> Free unit and no UnitClassType given
	else if(!kGoody.isBad() && kGoody.getMinBarbarians() > 0 &&
		(bEarlyMP || bVeryEarlyGame))
	{
		return false;
	} // </advc.314>
	if ((kGoody.getBarbarianUnitClass() != NO_UNITCLASS ||
		// <advc.314> Hostile unit w/o any UnitClassType given
		(kGoody.getMinBarbarians() > 0 && kGoody.getBarbarianUnitProb() > 0)) &&
		// BarbarianUnitClass has a different use now when !isBad
		kGoody.isBad()) // </advc.314>
	{
		if (GC.getGame().isOption(GAMEOPTION_NO_BARBARIANS))
			return false;

		if (getNumCities() == 0)
			return false;

		if (getNumCities() == 1)
		{
			CvCity* pCity = GC.getMap().findCity(pPlot->getX(), pPlot->getY(), NO_PLAYER, getTeam());
			if (pCity != NULL)
			{
				if (plotDistance(pPlot->getX(), pPlot->getY(), pCity->getX(), pCity->getY()) <=
						// advc.314 (comment):
						8 - getNumCities()) // = 7 b/c of the NumCities==1 check
					return false;
			}
		}
	} // <advc.315d> No Scout from a Scout if already 2 Scouts
	if(pUnit != NULL && pUnit->isNoBadGoodies() && AI().AI_getNumAIUnits(UNITAI_EXPLORE) >= 2)
	{
		UnitClassTypes eUnitClass = (UnitClassTypes)kGoody.getUnitClassType();
		if(eUnitClass != NO_UNITCLASS)
		{
			UnitTypes eUnit = getCivilization().getUnit(eUnitClass);
			if(eUnit != NO_UNIT && GC.getInfo(eUnit).isNoBadGoodies())
				return false;
		}
	} // </advc.315d>
	// <advc.315e> No map reveal at the edges of a non-wrapping map
	if(kGoody.getMapProb() > 0)
	{
		int const iRange = 3;
		for(int iDX = -iRange; iDX <= iRange; iDX++)
		{
			for(int iDY = -iRange; iDY <= iRange; iDY++)
			{
				CvPlot* pLoopPlot = ::plotXY(pPlot->getX(), pPlot->getY(), iDX, iDY);
				if(pLoopPlot == NULL)
					return false;
			}
		}
	} // </advc.315e>
	return true;
}


void CvPlayer::receiveGoody(CvPlot* pPlot, GoodyTypes eGoody, CvUnit* pUnit,
	bool bNoRecursion) // advc.314
{
	CvWString szBuffer;
	CvWString szTempBuffer;

	FAssert(canReceiveGoody(pPlot, eGoody, pUnit));
	// <advc>
	CvGoodyInfo const& kGoody = GC.getInfo(eGoody);
	CvGame& kGame = GC.getGame();
	// </advc>
	szBuffer = kGoody.getDescription();

	// <advc.314>
	scaled prUpgrade = (kGoody.isBad() ? 0 : per100(kGoody.getBarbarianUnitProb()));
	/*  Coefficient chosen such that it's 100% on turn 150. Would be better to
		program that computation and set the 100%-turn in XML ... */
	prUpgrade *= (fixp(0.225) * (kGame.goodyHutEffectFactor(false) - 1));
	/*  Meaning that an upgraded version of kGoody should be used, or, if there
		is none, that an additional outcome should be rolled. */
	bool bUpgrade = prUpgrade.bernoulliSuccess(kGame.getSRand(), "advc.314");
	// </advc.314>
	int iGold = kGoody.getGold() + kGame.getSorenRandNum(kGoody.getGoldRand1(), "Goody Gold 1") +
			kGame.getSorenRandNum(kGoody.getGoldRand2(), "Goody Gold 2");
	//iGold  = (iGold * GC.getInfo(kGame.getGameSpeedType()).getGrowthPercent()) / 100;
	// advc.314: Replacing the above
	iGold = (iGold * kGame.goodyHutEffectFactor()).round();
	if (iGold != 0 &&
		// advc.314: isTech means that iGold is the research progress
		!kGoody.isTech())
	{
		changeGold(iGold);
		szBuffer += gDLL->getText("TXT_KEY_MISC_RECEIVED_GOLD", iGold);
	}
	/*  advc.134: (Moved the addMessage down b/c some of the handlers
		have custom messages) */

	int const iMapRange = kGoody.getMapRange();
	if (iMapRange > 0)
	{
		int const iOffset = kGoody.getMapOffset();
		CvPlot* pBestPlot = NULL;
		if (iOffset > 0)
		{
			int iBestValue = 0;
			for (SquareIter it(*pPlot, iOffset, false); it.hasNext(); ++it)
			{
				CvPlot& kLoopPlot = *it;
				if (kLoopPlot.isRevealed(getTeam()))
					continue;
				int iValue = 1 + kGame.getSorenRandNum(10000, "Goody Map");
				iValue *= it.currPlotDist();
				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					pBestPlot = &kLoopPlot;
				}
			}
		}
		if (pBestPlot == NULL)
			pBestPlot = pPlot;

		for (PlotCircleIter it(*pBestPlot, iMapRange, false); it.hasNext(); ++it)
		{
			if (kGame.getSorenRandNum(100, "Goody Map") < kGoody.getMapProb())
				it->setRevealed(getTeam(), true, false, NO_TEAM, true);
		}
	}

	if (pUnit != NULL)
		pUnit->changeExperience(kGoody.getExperience());

	if (pUnit != NULL)
		pUnit->changeDamage(-(kGoody.getHealing()));

	if (kGoody.isTech())
	{
		TechTypes eBestTech = NO_TECH;
		int iBestValue = -1;
		FOR_EACH_ENUM(Tech)
		{
			/*if (GC.getInfo((TechTypes) iI).isGoodyTech()) {
				if (canResearch((TechTypes)iI), false, true) { // K-Mod */
			if (!isGoodyTech(eLoopTech, iGold)) // advc.314
				continue;
			int iValue = kGame.getSorenRandNum(10000, "Goody Tech");
			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				eBestTech = eLoopTech;
			}
		}
		FAssert(eBestTech != NO_TECH);
		// <advc.314> Most of the code from here on is modified
		CvTeam& kOurTeam = GET_TEAM(getTeam());
		if(iGold <= 0 || fixp(0.8) * kOurTeam.getResearchLeft(eBestTech) <= iGold)
		{
			kOurTeam.setHasTech(eBestTech, true, getID(), true, true);
			if(isSignificantDiscovery(eBestTech))
				kOurTeam.setNoTradeTech(eBestTech, true);
		}
		else
		{
			kOurTeam.changeResearchProgress(eBestTech, iGold, getID());
			szBuffer = gDLL->getText("TXT_KEY_MISC_PROGRESS_TOWARDS_TECH", iGold,
					GC.getInfo(eBestTech).getDescription());
		}
	}
	std::vector<UnitTypes> aeBestUnits;
	CvCivilization const& kCiv = GET_PLAYER(BARBARIAN_PLAYER).getCivilization();
	/*  When units need to be placed, but no unit class is given.
		Times 3 b/c MinBarbarians is only a lower bound. */
	for(int i = 0; i < 3 * kGoody.getMinBarbarians(); i++)
	{
		UnitTypes eBestUnit = NO_UNIT;
		if(kGoody.getUnitClassType() == NO_UNITCLASS &&
			kGoody.getBarbarianUnitClass() == NO_UNITCLASS)
		{
			int iBestValue = 0;
			for (int j = 0; j < kCiv.getNumUnits(); j++)
			{
				UnitTypes eUnit = kCiv.unitAt(j);
				CvUnitInfo const& kUnit = GC.getInfo(eUnit);
				if(kUnit.getDomainType() != DOMAIN_LAND)
					continue;
				if(kUnit.getNumPrereqOrBonuses() <= 0 &&
					kUnit.getPrereqAndBonus() == NO_BONUS &&
					kUnit.getCombat() > 0 &&
					(kUnit.getPrereqAndTech() == NO_TECH ||
					GC.getInfo(kUnit.getPrereqAndTech()).getEra() <=
					CvEraInfo::AI_getAgeOfExploration()) &&
					GET_PLAYER(BARBARIAN_PLAYER).canTrain(eUnit, false, true))
				{
					int iValue = kUnit.getCombat() + (kGoody.isBad() ?
							// Randomize hostile units a bit
							kGame.getSorenRandNum(10, "advc.314") : 0);
					if(iValue > iBestValue)
					{
						iBestValue = iValue;
						eBestUnit = eUnit;
					}
				}
			}
			FAssert(eBestUnit != NO_UNIT);
			aeBestUnits.push_back(eBestUnit);
		}
	}
	if (kGoody.getUnitClassType() != NO_UNITCLASS ||
		// Pick a unit based on Barbarian tech then
		(!kGoody.isBad() && kGoody.getMinBarbarians() > 0))
	{
		UnitTypes eUnit = NO_UNIT; // Declaration moved down
		UnitClassTypes eUnitClass = (UnitClassTypes)kGoody.getUnitClassType();
		if(eUnitClass != NO_UNITCLASS)
		{
			// Interpret BarbarianUnitClass as an upgrade to replace UnitClassType
			if(bUpgrade && kGoody.getBarbarianUnitClass() != NO_UNITCLASS)
			{
				eUnitClass = (UnitClassTypes)kGoody.getBarbarianUnitClass();
				// Upgrade applied, don't roll an additional outcome.
				bNoRecursion = true;
			}
			eUnit = getCivilization().getUnit(eUnitClass);
		}
		/*  Let MinBarbarians > 1 generate more than 1 unit
			(though I'm not using this so far; not tested either) */
		for(int i = 0; i < std::max(1, kGoody.getMinBarbarians()); i++)
		{
			UnitTypes eLoopUnit = eUnit;
			if(eLoopUnit == NO_UNIT && !aeBestUnits.empty())
				eLoopUnit = aeBestUnits[std::min((int)aeBestUnits.size() - 1, i)];
			FAssert(eLoopUnit != NO_UNIT);
			if(eLoopUnit != NO_UNIT)
			{
				CvUnit* pNewUnit = initUnit(eLoopUnit, pPlot->getX(), pPlot->getY());
				if(pNewUnit == NULL)
					continue;
				else FAssert(pNewUnit != NULL);
				szBuffer = gDLL->getText("TXT_KEY_GOODY_FREE_UNIT",
						GC.getInfo(eLoopUnit).getDescription());
				addGoodyMsg(szBuffer, *pPlot, kGoody.getSound());
				szBuffer.clear();
				if(pNewUnit->canAcquirePromotionAny())
					promoteFreeUnit(*pNewUnit, prUpgrade);
			}
		}
	}
	if(!szBuffer.empty()) // Moved from higher up
		addGoodyMsg(szBuffer, *pPlot, kGoody.getSound());
	/*  If not isBad, then BarbarianUnitClass has a different meaning, which is
		handled above. */
	if(kGoody.isBad() && (kGoody.getBarbarianUnitClass() != NO_UNITCLASS ||
		// Will use eBestUnit in this case
		(kGoody.getMinBarbarians() > 0 && kGoody.getBarbarianUnitProb() > 0)))
	{
		UnitTypes eUnit = NO_UNIT;
		UnitClassTypes eUnitClass = (UnitClassTypes)kGoody.getBarbarianUnitClass();
		if(eUnitClass != NO_UNITCLASS)
			eUnit = GET_PLAYER(BARBARIAN_PLAYER).getCivilization().getUnit(eUnitClass);
		int iMinBarbs = std::max(kGoody.getMinBarbarians(),
				(kGoody.getMinBarbarians() * kGame.goodyHutEffectFactor(false)).sqrt().
				round());
		// Increase the probability proportional to the change to MinBarbs
		int iProb = (kGoody.getBarbarianUnitProb() * iMinBarbs) / kGoody.getMinBarbarians();
		int iMaxBarbs = iMinBarbs + 2; // upper bound for iPass=0
		int iBarbCount = 0;
		for(int iPass = 0; iPass < 2; iPass++)
		{
			if(iBarbCount >= iMinBarbs)
				continue;
			FOR_EACH_ADJ_PLOT(*pPlot)
			{
				if(!pAdj->sameArea(*pPlot) || pAdj->isImpassable() ||
					pAdj->getNumUnits() > 0)
				{
					continue;
				}
				if(iPass > 0 || (kGame.getSorenRandNum(100, "Goody Barbs") < iProb))
				{
					UnitTypes eLoopUnit = eUnit;
					if(eLoopUnit == NO_UNIT && !aeBestUnits.empty())
					{
						eLoopUnit = aeBestUnits[
								std::min((int)aeBestUnits.size() - 1, iBarbCount)];
					}
					FAssert(eLoopUnit != NO_UNIT);
					if(eLoopUnit != NO_UNIT)
					{
						GET_PLAYER(BARBARIAN_PLAYER).initUnit(eLoopUnit,
								pAdj->getX(), pAdj->getY(),
								(pAdj->isWater() ? UNITAI_ATTACK_SEA : UNITAI_ATTACK));
						iBarbCount++;
					}
					if ((iPass > 0 && iBarbCount >= iMinBarbs) || iBarbCount >= iMaxBarbs)
						break;
				}
			}
		}
	}
	if(bUpgrade && !bNoRecursion)
		doGoody(pPlot, pUnit, eGoody); // </advc.314>
}


void CvPlayer::doGoody(CvPlot* pPlot, CvUnit* pUnit, /* advc.314: */ GoodyTypes eTaboo)
{
	if (GC.getPythonCaller()->doGoody(*pPlot, pUnit, getID()))
		return;

	FAssert(pPlot->isGoody() /* advc.314: */ || eTaboo != NO_GOODY);
	pPlot->removeGoody();
	// <advc>
	if(isBarbarian())
	{
		FAssertMsg(pPlot->isOwned(), "Barbarians should remove hut only when receiving a city");
		return;
	} // </advc>
	int const iAttempts = GC.getDefineINT("NUM_DO_GOODY_ATTEMPTS"); // advc.opt
	for (int i = 0; i < iAttempts; i++)
	{
		if (GC.getInfo(getHandicapType()).getNumGoodies() <= 0)
			continue;
		GoodyTypes eGoody = (GoodyTypes)GC.getInfo(getHandicapType()).
				getGoodies(GC.getGame().getSorenRandNum(
				GC.getInfo(getHandicapType()).getNumGoodies(), "Goodies"));
		FAssert(eGoody >= 0 && eGoody < GC.getNumGoodyInfos());
		// <advc.314>
		if(eGoody == eTaboo || (eTaboo != NO_GOODY && GC.getInfo(eGoody).isBad() !=
			GC.getInfo(eTaboo).isBad())) // Don't pair a good with a bad outcome
		{
			continue;
		} // </advc.314>
		if (canReceiveGoody(pPlot, eGoody, pUnit))
		{
			receiveGoody(pPlot, eGoody, pUnit, /* advc.314: */ eTaboo != NO_GOODY);
			// advc (note): pUnit can be NULL here, but a CyUnit for Python can still be created.
			CvEventReporter::getInstance().goodyReceived(getID(), pPlot, pUnit, eGoody);
			break;
		}
	}
}


bool CvPlayer::canFound(int iX, int iY, bool bTestVisible) const
{
	CvPlot const& kPlot = *GC.getMap().plot(iX, iY);
	if (kPlot.isOwned() && kPlot.getOwner() != getID())
		return false;
	// advc: Checks that don't depend on player moved into new function at CvPlot
	if (!kPlot.canFound(bTestVisible))
		return false;
	if (GC.getGame().isFinalInitialized() && getNumCities() > 0 &&
		GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
	{
		return false; // (advc.opt: Moved down)
	}
	// advc.003y: Moved down
	return !GC.getPythonCaller()->cannotFoundCityOverride(kPlot, getID());
}


void CvPlayer::found(int iX, int iY)
{
	if (!canFound(iX, iY))
		return;
	// <advc.031c>
	if (gFoundLogLevel > 0 && !isHuman() &&
		// (advc.108 forces founding in place in scenarios)
		(getNumCities() > 0 || !GC.getGame().isScenario()))
	{
		AI().logFoundValue(GC.getMap().getPlot(iX, iY));
	} // </advc.031c>
	CvCity* pCity = initCity(iX, iY, true, true);
	FAssertMsg(pCity != NULL, "City is not assigned a valid value");
	CvGame const& kGame = GC.getGame();

	if (isBarbarian())
	{
		UnitTypes eDefenderUnit = pCity->AI().AI_bestUnitAI(UNITAI_CITY_DEFENSE);
		if (eDefenderUnit == NO_UNIT)
			eDefenderUnit = pCity->AI().AI_bestUnitAI(UNITAI_ATTACK);

		if (eDefenderUnit != NO_UNIT)
		{
			for (int i = 0; i < GC.getInfo(kGame.getHandicapType()).getBarbarianInitialDefenders(); i++)
			{
				initUnit(eDefenderUnit, iX, iY, UNITAI_CITY_DEFENSE);
			}
		}
	}

	CvCivilization const& kCiv = getCivilization(); // advc.003w
	for (int i = 0; i < kCiv.getNumBuildings(); i++)
	{
		BuildingTypes eLoopBuilding = kCiv.buildingAt(i);
		if(kGame.isFreeStartEraBuilding(eLoopBuilding))
		{
			if (pCity->canConstruct(eLoopBuilding))
				pCity->setNumRealBuilding(eLoopBuilding, 1);
		}
	}

	if (getAdvancedStartPoints() >= 0)
	{	// Free border expansion for Creative
		bool bCreative = false;
		FOR_EACH_ENUM(Trait)
		{
			if (hasTrait(eLoopTrait))
			{
				if (GC.getInfo(eLoopTrait).getCommerceChange(COMMERCE_CULTURE) > 0)
				{
					bCreative = true;
					break;
				}
			}
		}

		if (bCreative)
		{
			FOR_EACH_ENUM(CultureLevel)
			{
				int iCulture = kGame.getCultureThreshold(eLoopCultureLevel);
				if (iCulture > 0)
				{
					pCity->setCulture(getID(), iCulture, true, true);
					break;
				}
			}
		}
	}
	// <advc.104>
	else
	{
		for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
			it->AI_cityCreated(*pCity);
	} // </advc.104>

	if (isHuman())
	{	// advc.001: Else branch shouldn't depend on advanced start (minor BtS bug)
		if (getAdvancedStartPoints() < 0)
		{
			pCity->chooseProduction(NO_UNIT, NO_BUILDING, NO_PROJECT,
					/*  advc.124g: Somehow, the choose-production popup comes up twice
						(despite being pushed only once) in multiplayer if the
						choose-tech popup is handled before choose-production. */
					false, kGame.isNetworkMultiPlayer());
		}
	}
	else pCity->doFoundMessage();
	// <advc.210c>
	for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
	{
		CvPlayer const& kObs = *it;
		if (pCity->getOwner() != kObs.getID() && pCity->isRevealed(kObs.getTeam()))
		{
			gDLL->UI().addMessage(kObs.getID(), false, -1,
					gDLL->getText("TXT_KEY_MORECIV4LERTS_CITY_FOUNDED",
					getNameKey(), pCity->getNameKey()),
					pCity->getPlot(), 0, MESSAGE_TYPE_INFO,
					ARTFILEMGR.getInterfaceArtPath("WORLDBUILDER_CITY_EDIT"));

		}
	} // </advc.210c>
	if (!CvPlot::isAllFog()) // advc.706: Suppress name-city popup
		CvEventReporter::getInstance().cityBuilt(pCity);
	if (gPlayerLogLevel >= 1 || /* advc.031c: */ gFoundLogLevel >= 1) logBBAI("  Player %d (%S) founds new city %S at %d, %d", getID(), getCivilizationDescription(0), pCity->getName(0).GetCString(), iX, iY); // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
}


bool CvPlayer::canTrain(UnitTypes eUnit, bool bContinue, bool bTestVisible, bool bIgnoreCost) const
{
	//PROFILE_FUNC(); // advc.003o
	UnitClassTypes eUnitClass = GC.getInfo(eUnit).getUnitClassType();

	/*	K-Mod note. This assert can fail if team games when checking whether this city can
		upgrade a unit to one of our team member's UUs. */
	//FAssert(GC.getInfo(getCivilizationType()).getCivilizationUnits(eUnitClass) == eUnit);
	if (getCivilization().getUnit(eUnitClass) != eUnit)
		return false;

	if (GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
	{
		if (GC.getInfo(eUnit).isFound())
			return false;
	}

	if (!GET_TEAM(getTeam()).isHasTech((TechTypes)GC.getInfo(eUnit).getPrereqAndTech()))
		return false;

	for (int i = 0; i < GC.getInfo(eUnit).getNumPrereqAndTechs(); i++)
	{
		if (!GET_TEAM(getTeam()).isHasTech(GC.getInfo(eUnit).getPrereqAndTechs(i)))
			return false;
	}

	if (GC.getInfo(eUnit).getStateReligion() != NO_RELIGION)
	{
		if (getStateReligion() != GC.getInfo(eUnit).getStateReligion())
			return false;
	}
	// <advc> Some checks moved to CvGame
	if (!GC.getGame().canTrain(eUnit, bIgnoreCost, bTestVisible))
		return false; // </advc>

	/*if (GET_TEAM(getTeam()).isUnitClassMaxedOut(eUnitClass))
		return false;
	if (isUnitClassMaxedOut(eUnitClass))
		return false;*/ // BtS - disabled by K-Mod.
	/*	Note that unlike the global limit, these two limits apply to the
		number of units currently alive rather than the total ever trained.
		Therefore these limits should be ignored for the visibility test. */

	if (!bTestVisible)
	{
		if (GC.getGame().isUnitClassMaxedOut(eUnitClass,
			GET_TEAM(getTeam()).getUnitClassMaking(eUnitClass) + (bContinue ? -1 : 0)))
		{
			return false;
		}
		if (GET_TEAM(getTeam()).isUnitClassMaxedOut(eUnitClass,
			GET_TEAM(getTeam()).getUnitClassMaking(eUnitClass) + (bContinue ? -1 : 0)))
		{
			return false;
		}
		if (isUnitClassMaxedOut(eUnitClass,
			getUnitClassMaking(eUnitClass) + (bContinue ? -1 : 0)))
		{
			return false;
		}
	}

	return true;
}


bool CvPlayer::canConstruct(BuildingTypes eBuilding, bool bContinue, bool bTestVisible, bool bIgnoreCost, bool bIgnoreTech) const
{
	/*  advc: Global checks moved into new function. These are all fast (now that
		CivTeamsEverAlive is cached). */
	if(!GC.getGame().canConstruct(eBuilding, bIgnoreCost, bTestVisible))
		return false;

	CvBuildingInfo const& kBuilding = GC.getInfo(eBuilding);
	BuildingClassTypes const eBuildingClass = kBuilding.getBuildingClassType();
	// (advc.003w: Usually, the caller checks this now, but can't rely on that.)
	if (getCivilization().getBuilding(eBuildingClass) != eBuilding)
		return false;

	CvTeamAI& kOurTeam = GET_TEAM(getTeam());
	if (!bIgnoreTech) // K-Mod
	{
		if (!kOurTeam.isHasTech(kBuilding.getPrereqAndTech()))
			return false;

		for (int i = 0; i < kBuilding.getNumPrereqAndTechs(); i++)
		{
			if (!kOurTeam.isHasTech(kBuilding.getPrereqAndTechs(i)))
				return false;
		}
	}

	if (kOurTeam.isObsoleteBuilding(eBuilding))
		return false;
	{
		SpecialBuildingTypes eSpecial = kBuilding.getSpecialBuildingType();
		if (eSpecial != NO_SPECIALBUILDING &&
			!kOurTeam.isHasTech(GC.getInfo(eSpecial).getTechPrereq()))
		{
			return false;
		}
	}
	{
		ReligionTypes ePrereqStateReligion = GC.getInfo(eBuilding).getStateReligion();
		if (ePrereqStateReligion != NO_RELIGION && ePrereqStateReligion != getStateReligion())
			return false;
	}
	{
		VictoryTypes ePrereqVictory = kBuilding.getVictoryPrereq();
		if (ePrereqVictory != NO_VICTORY)
		{
			if (isMinorCiv() || isBarbarian())
				return false;
			if (kOurTeam.getVictoryCountdown(ePrereqVictory) >= 0)
				return false;
		}
	}
	{
		CorporationTypes eFoundCorp = kBuilding.getFoundsCorporation();
		if (eFoundCorp != NO_CORPORATION && isNoCorporations())
			return false;
	}
	// <kekm.19> (advc: simplified)
	if (kBuilding.isCapital() && GC.getGame().getGameState() == GAMESTATE_ON &&
		GET_TEAM(getTeam()).isAnyVictoryCountdown()) // advc.opt
	{
		return false;
	} // </kekm.19>
	if (kOurTeam.isBuildingClassMaxedOut(eBuildingClass))
		return false;

	if (isBuildingClassMaxedOut(eBuildingClass))
		return false;

	CvCivilization const& kCiv = getCivilization(); // advc.003w
	for (int i = 0; i < kCiv.getNumBuildings(); i++)
	{
		BuildingTypes ePrereqBuilding = kCiv.buildingAt(i);
		BuildingClassTypes ePrereqClass = kCiv.buildingClassAt(i);
		if (ePrereqBuilding != NO_BUILDING && kOurTeam.isObsoleteBuilding(ePrereqBuilding) &&
			getBuildingClassCount(ePrereqClass) <
			getBuildingClassPrereqBuilding(eBuilding, ePrereqClass, 0))
		{
			return false;
		}
	}

	if(bTestVisible)
		return true;

	if (getHighestUnitLevel() < kBuilding.getUnitLevelPrereq())
		return false;
	{
		int iTeamMaking = kOurTeam.getBuildingClassMaking(eBuildingClass);
		int iTeamMakingCont = iTeamMaking + (bContinue ? -1 : 0);
		if (GC.getGame().isBuildingClassMaxedOut(eBuildingClass, iTeamMakingCont))
			return false;
		if (kOurTeam.isBuildingClassMaxedOut(eBuildingClass, iTeamMakingCont))
			return false;
		int iMaking = getBuildingClassMaking(eBuildingClass);
		int iMakingCont = iMaking + (bContinue ? -1 : 0);
		if (isBuildingClassMaxedOut(eBuildingClass, iMakingCont))
			return false;
		FOR_EACH_ENUM2(BuildingClass, ePrereqClass)
		{
			if (getBuildingClassCount(ePrereqClass) < getBuildingClassPrereqBuilding(
				eBuilding, ePrereqClass, bContinue ? 0 : iMaking))
			{
				return false;
			}
		}
	}

	return true;
}


bool CvPlayer::canCreate(ProjectTypes eProject, bool bContinue, bool bTestVisible) const
{
	if (isBarbarian())
		return false;

	if (GC.getInfo(eProject).getProductionCost() == -1)
		return false;

	if (!GET_TEAM(getTeam()).isHasTech((TechTypes)
		GC.getInfo(eProject).getTechPrereq()))
	{
		return false;
	}
	if (GC.getInfo(eProject).getVictoryPrereq() != NO_VICTORY)
	{
		if (!GC.getGame().isVictoryValid((VictoryTypes)
			GC.getInfo(eProject).getVictoryPrereq()))
		{
			return false;
		}
		if (isMinorCiv())
			return false;

		if (GET_TEAM(getTeam()).getVictoryCountdown((VictoryTypes)
			GC.getInfo(eProject).getVictoryPrereq()) >= 0)
		{
			return false;
		}
	}

	if (GC.getGame().isProjectMaxedOut(eProject))
		return false;

	if (GET_TEAM(getTeam()).isProjectMaxedOut(eProject))
		return false;

	if (!bTestVisible)
	{
		if (GC.getGame().isProjectMaxedOut(eProject,
			GET_TEAM(getTeam()).getProjectMaking(eProject) + (bContinue ? -1 : 0)))
		{
			return false;
		}
		if (GET_TEAM(getTeam()).isProjectMaxedOut(eProject,
			GET_TEAM(getTeam()).getProjectMaking(eProject) + (bContinue ? -1 : 0)))
		{
			return false;
		}
		if (GC.getGame().isNoNukes() && GC.getInfo(eProject).isAllowsNukes())
		{
			return false; // advc.opt
			/*FOR_EACH_ENUM(Unit) {
				if (GC.getInfo(eLoopUnit).getNukeRange() != -1)
					return false;
			}*/
		}
		if (GC.getInfo(eProject).getAnyoneProjectPrereq() != NO_PROJECT &&
			GC.getGame().getProjectCreatedCount((ProjectTypes)
			GC.getInfo(eProject).getAnyoneProjectPrereq()) == 0)
		{
			return false;
		}
		FOR_EACH_ENUM(Project)
		{
			if (GET_TEAM(getTeam()).getProjectCount(eLoopProject) <
				GC.getInfo(eProject).getProjectsNeeded(eLoopProject))
			{
				return false;
			}
		}
	}

	return true;
}


bool CvPlayer::canMaintain(ProcessTypes eProcess, bool bContinue) const
{
	return GET_TEAM(getTeam()).isHasTech(GC.getInfo(eProcess).getTechPrereq());
}


bool CvPlayer::isProductionMaxedUnitClass(UnitClassTypes eUnitClass) const
{
	if (eUnitClass == NO_UNITCLASS)
		return false;
	if (GC.getGame().isUnitClassMaxedOut(eUnitClass))
		return true;
	if (GET_TEAM(getTeam()).isUnitClassMaxedOut(eUnitClass))
		return true;
	if (isUnitClassMaxedOut(eUnitClass))
		return true;
	return false;
}


bool CvPlayer::isProductionMaxedBuildingClass(BuildingClassTypes eBuildingClass, bool bAcquireCity) const
{
	if (eBuildingClass == NO_BUILDINGCLASS)
		return false;
	if (!bAcquireCity && GC.getGame().isBuildingClassMaxedOut(eBuildingClass))
		return true;
	if (GET_TEAM(getTeam()).isBuildingClassMaxedOut(eBuildingClass))
		return true;
	if (isBuildingClassMaxedOut(eBuildingClass,
		bAcquireCity ? GC.getInfo(eBuildingClass).getExtraPlayerInstances() : 0))
	{
		return true;
	}
	return false;
}


bool CvPlayer::isProductionMaxedProject(ProjectTypes eProject) const
{
	if (eProject == NO_PROJECT)
		return false;
	if (GC.getGame().isProjectMaxedOut(eProject))
		return true;
	if (GET_TEAM(getTeam()).isProjectMaxedOut(eProject))
		return true;
	return false;
}


int CvPlayer::getProductionNeeded(UnitTypes eUnit,
	int iExtraInstances) const // advc.104
{
	UnitClassTypes eUnitClass = GC.getInfo(eUnit).getUnitClassType();
	FAssert(NO_UNITCLASS != eUnitClass);

	int iProductionNeeded = GC.getInfo(eUnit).getProductionCost();

	iProductionNeeded *= 100 + (GC.getInfo(eUnitClass).getInstanceCostModifier() *
			(getUnitClassCount(eUnitClass) + /* advc.104: */ iExtraInstances));
	iProductionNeeded /= 100;

	static int const iUNIT_PRODUCTION_PERCENT = GC.getDefineINT("UNIT_PRODUCTION_PERCENT"); // advc.opt
	iProductionNeeded *= iUNIT_PRODUCTION_PERCENT;
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getInfo(GC.getGame().getGameSpeedType()).getTrainPercent();
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getInfo(GC.getGame().getStartEra()).getTrainPercent();
	iProductionNeeded /= 100;
	/*  <advc.107> Code moved into auxiliary function b/c I need this modifier
		in AI_getTotalFloatingDefendersNeeded. */
	iProductionNeeded = (iProductionNeeded *
			trainingModifierFromHandicap(GC.getInfo(eUnitClass).isWorldUnit())).
			roundToMultiple(/* advc.251: */ isHuman() ? 5 : 1);
	// </advc.107>
	// advc.251 (comment): See getNewCityProductionValue
	iProductionNeeded += getUnitExtraCost(eUnitClass);

	int iPyMod = GC.getPythonCaller()->unitCostMod(getID(), eUnit);
	if (iPyMod > 0)
	{
		iProductionNeeded *= iPyMod;
		iProductionNeeded /= 100;
	}
	return std::max(1, iProductionNeeded);
}


int CvPlayer::getProductionNeeded(BuildingTypes eBuilding) const
{
	CvGame const& kGame = GC.getGame(); // advc
	int iProductionNeeded = GC.getInfo(eBuilding).getProductionCost();

	static int const iBUILDING_PRODUCTION_PERCENT = GC.getDefineINT("BUILDING_PRODUCTION_PERCENT"); // advc.opt
	iProductionNeeded *= iBUILDING_PRODUCTION_PERCENT;
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getInfo(kGame.getGameSpeedType()).getConstructPercent();
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getInfo(kGame.getStartEra()).getConstructPercent();
	iProductionNeeded /= 100;
	// <advc.251>
	iProductionNeeded = (iProductionNeeded * per100(
			GC.getInfo(getHandicapType()).getConstructPercent())).
			roundToMultiple(isHuman() ? 5 : 1);
	if (!isHuman()) // Barbarians too
	{
		CvHandicapInfo const& h = GC.getInfo(kGame.getHandicapType());
		int iAIModifier = //h.getAIPerEraModifier() * getCurrentEra()
				kGame.AIHandicapAdjustment();
		if(GC.getInfo(eBuilding).isWorldWonder())
			iAIModifier += h.getAIWorldConstructPercent();
		else iAIModifier += h.getAIConstructPercent();
		iProductionNeeded *= iAIModifier;
		iProductionNeeded /= 100;
	} // </advc.251>

	return std::max(1, iProductionNeeded);
}


int CvPlayer::getProductionNeeded(ProjectTypes eProject) const
{
	CvGame const& kGame = GC.getGame(); // advc
	// <advc.251>
	int const iBaseCost = GC.getInfo(eProject).getProductionCost();
	int iProductionNeeded = iBaseCost; // </advc.251>

	static int const iPROJECT_PRODUCTION_PERCENT = GC.getDefineINT("PROJECT_PRODUCTION_PERCENT"); // advc.opt
	iProductionNeeded *= iPROJECT_PRODUCTION_PERCENT;
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getInfo(kGame.getGameSpeedType()).getCreatePercent();
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getInfo(kGame.getStartEra()).getCreatePercent();
	iProductionNeeded /= 100;
	// <advc.251>
	iProductionNeeded = ::roundToMultiple(0.01 * iProductionNeeded *
			GC.getInfo(getHandicapType()).getCreatePercent(),
			isHuman() ? (iBaseCost > 500 ? 50 : 5) : 1);
	if (!isHuman() && !isBarbarian())
	{
		CvHandicapInfo const& h = GC.getInfo(kGame.getHandicapType());
		int iAIModifier = //h.getAIPerEraModifier() * getCurrentEra()
				kGame.AIHandicapAdjustment();
		if(GC.getInfo(eProject).isWorldProject())
			iAIModifier += h.getAIWorldCreatePercent();
		else iAIModifier += h.getAICreatePercent();
		iProductionNeeded *= iAIModifier;
		iProductionNeeded /= 100;
	} // </advc.251>

	return std::max(1, iProductionNeeded);
}

int CvPlayer::getProductionModifier(UnitTypes eUnit) const
{
	int iMultiplier = 0;
	if (GC.getInfo(eUnit).isMilitaryProduction())
		iMultiplier += getMilitaryProductionModifier();

	FOR_EACH_ENUM(Trait)
	{
		if (!hasTrait(eLoopTrait))
			continue;
		iMultiplier += GC.getInfo(eUnit).getProductionTraits(eLoopTrait);
		if (GC.getInfo(eUnit).getSpecialUnitType() != NO_SPECIALUNIT)
		{
			iMultiplier += GC.getInfo(GC.getInfo(eUnit).getSpecialUnitType()).
					getProductionTraits(eLoopTrait);
		}
	}

	return iMultiplier;
}

int CvPlayer::getProductionModifier(BuildingTypes eBuilding) const
{
	int iMultiplier = 0;
	CvBuildingInfo const& kBuilding = GC.getInfo(eBuilding);
	FOR_EACH_ENUM(Trait)
	{
		if (!hasTrait(eLoopTrait))
			continue;
		iMultiplier += kBuilding.getProductionTraits(eLoopTrait);
		if (GC.getInfo(eBuilding).getSpecialBuildingType() != NO_SPECIALBUILDING)
		{
			iMultiplier += GC.getInfo(GC.getInfo(eBuilding).getSpecialBuildingType()).
					getProductionTraits(eLoopTrait);
		}
	}
	if (kBuilding.isWorldWonder())
		iMultiplier += getMaxGlobalBuildingProductionModifier();
	if (kBuilding.isTeamWonder())
		iMultiplier += getMaxTeamBuildingProductionModifier();
	if (kBuilding.isNationalWonder())
		iMultiplier += getMaxPlayerBuildingProductionModifier();

	return iMultiplier;
}

int CvPlayer::getProductionModifier(ProjectTypes eProject) const
{
	int iMultiplier = 0;
	if (GC.getInfo(eProject).isSpaceship())
		iMultiplier += getSpaceProductionModifier();
	return iMultiplier;
}

// advc.107: Cut from getProductionNeeded; refactored.
scaled CvPlayer::trainingModifierFromHandicap(bool bWorldClass) const
{
	// <advc.251>
	scaled r = per100(GC.getInfo(getHandicapType()).getTrainPercent());
	if (!isHuman()) // Apply this also to Barbarians // </advc.251>
	{
		CvHandicapInfo& kGameHandicap = GC.getInfo(GC.getGame().getHandicapType());
		int iAIPercent = //h.getAIPerEraModifier() * getCurrentEra()
				GC.getGame().AIHandicapAdjustment();
		if(bWorldClass)
			iAIPercent += kGameHandicap.getAIWorldTrainPercent();
		else iAIPercent += kGameHandicap.getAITrainPercent();
		r *= per100(iAIPercent);
	}
	return std::max(r, per100(1));
}

int CvPlayer::getBuildingClassPrereqBuilding(BuildingTypes eBuilding, BuildingClassTypes ePrereqBuildingClass, int iExtra) const
{
	CvBuildingInfo const& kBuilding = GC.getInfo(eBuilding);

	int iPrereqs = kBuilding.getPrereqNumOfBuildingClass(ePrereqBuildingClass);
	if (iPrereqs < 1) // dont bother with the rest of the calcs if we have no prereqs
		return 0;

	iPrereqs *= std::max(0, GC.getInfo(GC.getMap().getWorldSize()).
			getBuildingClassPrereqModifier() + 100);
	//iPrereqs /= 100;
	iPrereqs = per100(iPrereqs).ceil(); // advc.140: Round up

	if (!kBuilding.isLimited())
	{
		iPrereqs *= getBuildingClassCount(GC.getInfo(eBuilding).getBuildingClassType()) +
				iExtra + 1;
	}

	if (GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
		iPrereqs = std::min(1, iPrereqs);

	return iPrereqs;
}


void CvPlayer::removeBuildingClass(BuildingClassTypes eBuildingClass)
{
	BuildingTypes eBuilding = getCivilization().getBuilding(eBuildingClass);
	if (eBuilding == NO_BUILDING)
		return;

	FOR_EACH_CITY_VAR(pLoopCity, *this)
	{
		if (pLoopCity->getNumRealBuilding(eBuilding) > 0)
		{
			pLoopCity->setNumRealBuilding(eBuilding,
					// UNOFFICIAL_PATCH, Bugfix, 02/16/10, EmperorFool
					pLoopCity->getNumRealBuilding(eBuilding) - 1);
			break;
		}
	}
}

// courtesy of the Gourd Bros...
void CvPlayer::processBuilding(BuildingTypes eBuilding, int iChange, CvArea& kArea)
{
	CvBuildingInfo const& kBuilding = GC.getInfo(eBuilding); // advc

	if (kBuilding.getFreeBuildingClass() != NO_BUILDINGCLASS)
	{
		BuildingTypes eFreeBuilding = getCivilization().getBuilding((BuildingClassTypes)
				kBuilding.getFreeBuildingClass());
		// advc.303: Barbarians can't receive the Monuments from Stonehenge
		if(eFreeBuilding != NO_BUILDING)
			changeFreeBuildingCount(eFreeBuilding, iChange);
	}

	if (kBuilding.getCivicOption() != NO_CIVICOPTION)
	{	// <advc.004x>
		CivicOptionTypes const eCivicOption = kBuilding.getCivicOption();
		CivicTypes eNewCivic = NO_CIVIC;
		if (iChange > 0 && isHuman() && !gDLL->GetWorldBuilderMode())
		{
			FOR_EACH_ENUM(Civic)
			{
				if (GC.getInfo(eLoopCivic).getCivicOptionType() == eCivicOption)
				{
					if (!canDoCivics(eLoopCivic))
					{
						eNewCivic = eLoopCivic;
						break;
					}
				}
			}
		} // </advc.004x>
		changeHasCivicOptionCount(eCivicOption, iChange);
		// <advc.004x>
		if(eNewCivic != NO_CIVIC)
			doChangeCivicsPopup(eNewCivic); // </advc.004x>
	}

	changeGreatPeopleRateModifier(kBuilding.getGlobalGreatPeopleRateModifier() * iChange);
	changeGreatGeneralRateModifier(kBuilding.getGreatGeneralRateModifier() * iChange);
	changeDomesticGreatGeneralRateModifier(kBuilding.getDomesticGreatGeneralRateModifier() * iChange);
	changeAnarchyModifier(kBuilding.getAnarchyModifier() * iChange);
	changeGoldenAgeModifier(kBuilding.getGoldenAgeModifier() * iChange);
	changeHurryModifier(kBuilding.getGlobalHurryModifier() * iChange);
	changeFreeExperience(kBuilding.getGlobalFreeExperience() * iChange);
	changeWarWearinessModifier(kBuilding.getGlobalWarWearinessModifier() * iChange);
	kArea.changeFreeSpecialist(getID(), (kBuilding.getAreaFreeSpecialist() * iChange));
	changeFreeSpecialist(kBuilding.getGlobalFreeSpecialist() * iChange);
	changeCoastalTradeRoutes(kBuilding.getCoastalTradeRoutes() * iChange);
	//changeTradeRoutes(kBuilding.getGlobalTradeRoutes() * iChange);
	// advc.310: Now per area
	kArea.changeTradeRoutes(getID(), kBuilding.getAreaTradeRoutes() * iChange);
	if (kBuilding.getAreaHealth() > 0)
		kArea.changeBuildingGoodHealth(getID(), (kBuilding.getAreaHealth() * iChange));
	else kArea.changeBuildingBadHealth(getID(), (kBuilding.getAreaHealth() * iChange));
	if (kBuilding.getGlobalHealth() > 0)
		changeBuildingGoodHealth(kBuilding.getGlobalHealth() * iChange);
	else changeBuildingBadHealth(kBuilding.getGlobalHealth() * iChange);
	kArea.changeBuildingHappiness(getID(), (kBuilding.getAreaHappiness() * iChange));
	changeBuildingHappiness(kBuilding.getGlobalHappiness() * iChange);
	changeWorkerSpeedModifier(kBuilding.getWorkerSpeedModifier() * iChange);
	changeSpaceProductionModifier(kBuilding.getGlobalSpaceProductionModifier() * iChange);
	changeCityDefenseModifier(kBuilding.getAllCityDefenseModifier() * iChange);
	kArea.changeCleanPowerCount(getTeam(), ((kBuilding.isAreaCleanPower()) ? iChange : 0));
	kArea.changeBorderObstacleCount(getTeam(), ((kBuilding.isAreaBorderObstacle()) ? iChange : 0));

	FOR_EACH_ENUM2(Yield, y)
	{
		changeSeaPlotYield(y, kBuilding.getGlobalSeaPlotYieldChange(y) * iChange);
		kArea.changeYieldRateModifier(getID(), y, kBuilding.getAreaYieldModifier(y) * iChange);
		changeYieldRateModifier(y, kBuilding.getGlobalYieldModifier(y) * iChange);
	}
	FOR_EACH_ENUM2(Commerce, c)
	{
		changeCommerceRateModifier(c, kBuilding.getGlobalCommerceModifier(c) * iChange);
		changeSpecialistExtraCommerce(c, kBuilding.getSpecialistExtraCommerce(c) * iChange);
		changeStateReligionBuildingCommerce(c, kBuilding.getStateReligionCommerce(c) * iChange);
		changeCommerceFlexibleCount(c, kBuilding.isCommerceFlexible(c) ? iChange : 0);
	}
	if (kBuilding.isAnyBuildingHappinessChanges()) // advc.003t
	{
		CvCivilization const& kCiv = getCivilization(); // advc.003w
		for (int i = 0; i < kCiv.getNumBuildings(); i++)
		{
			BuildingTypes eOldBuilding = kCiv.buildingAt(i);
			changeExtraBuildingHappiness(eOldBuilding,
					kBuilding.getBuildingHappinessChanges(eOldBuilding) * iChange);
		}
	}
	FOR_EACH_ENUM(Specialist)
	{
		FOR_EACH_ENUM(Yield)
		{
			changeSpecialistExtraYield(eLoopSpecialist, eLoopYield,
					GC.getInfo(eBuilding).getSpecialistYieldChange(
					eLoopSpecialist, eLoopYield) * iChange);
		}
	}
}


bool CvPlayer::canBuild(CvPlot const& kPlot, BuildTypes eBuild, bool bTestEra, bool bTestVisible) const
{
	//PROFILE_FUNC(); // advc.003o
	if (!kPlot.canBuild(eBuild, getID(), bTestVisible))
		return false;

	if (GC.getInfo(eBuild).getTechPrereq() != NO_TECH)
	{	// advc:
		TechTypes ePrereqTech = GC.getInfo(eBuild).getTechPrereq();
		if (!GET_TEAM(getTeam()).isHasTech(ePrereqTech))
		{
			if ((!bTestEra && !bTestVisible) ||
				getCurrentEra() + 1 < GC.getInfo(ePrereqTech).getEra())
			{
				return false;
			}
		}
	}

	if (!bTestVisible)
	{
		if (kPlot.isFeature())
		{
			if (!GET_TEAM(getTeam()).isHasTech(GC.getInfo(eBuild).
				getFeatureTech(kPlot.getFeatureType())))
			{
				return false;
			}
		}

		if (std::max(0, getGold()) < getBuildCost(kPlot, eBuild))
			return false;
	}

	return true;
}


int CvPlayer::getBuildCost(CvPlot const& kPlot, BuildTypes eBuild) const  // advc: 1st param was pointer
{
	if (kPlot.getBuildProgress(eBuild) > 0)
		return 0;

	return std::max(0, GC.getInfo(eBuild).getCost() *
			(100 + calculateInflationRate())) / 100;
}


RouteTypes CvPlayer::getBestRoute(CvPlot const* pPlot,
	BuildTypes* peBestBuild) const // advc.121
{
	PROFILE_FUNC();

	int iBestValue = 0;
	RouteTypes eBestRoute = NO_ROUTE;
	BuildTypes eBestBuild = NO_BUILD; // advc.121

	// BBAI TODO: Efficiency: Could cache this, decent savings on large maps
	// Perhaps save best route type per player each turn, then just check that
	// one first and only check others if can't do best.

	// K-Mod: I've reversed the order of iteration because the best builds are usually at the end.
	FOR_EACH_ENUM_REV(Build)
	{
		RouteTypes const eLoopRoute = GC.getInfo(eLoopBuild).getRoute();
		if (eLoopRoute == NO_ROUTE)
			continue;
		// K-Mod: I've swapped the order of the if statments, because the value check is much faster. (faster trumps convention)
		int iValue = GC.getInfo(eLoopRoute).getValue();
		if (iValue > iBestValue)
		{
			if (pPlot != NULL ?
				(pPlot->getRouteType() == eLoopRoute || canBuild(*pPlot, eLoopBuild)) :
				GET_TEAM(getTeam()).isHasTech(GC.getInfo(eLoopBuild).getTechPrereq()))
			{
				iBestValue = iValue;
				eBestRoute = eLoopRoute;
				eBestBuild = eLoopBuild; // advc.121
			}
		}
	}
	// <advc.121>
	if (peBestBuild != NULL)
		*peBestBuild = eBestBuild; // </advc.121>
	return eBestRoute;
}


int CvPlayer::getImprovementUpgradeRate() const
{
	//int iRate = 1; // XXX
	int iRate = 100; // advc.912f: Times-100 precision
	iRate *= std::max(0, getImprovementUpgradeRateModifier() + 100);
	iRate /= 100;
	return std::max(0, iRate); // advc.912f: Negative rate is still not supported
}


int CvPlayer::calculateTotalYield(YieldTypes eYield) const
{
	PROFILE_FUNC();

	int iTotalCommerce = 0;
	FOR_EACH_CITY(pLoopCity, *this)
		iTotalCommerce += pLoopCity->getYieldRate(eYield);
	return iTotalCommerce;
}


int CvPlayer::calculateTotalCityHappiness() const
{
	int iTotalHappiness = 0;
	FOR_EACH_CITY(pLoopCity, *this)
		iTotalHappiness += pLoopCity->happyLevel();
	return iTotalHappiness;
}


int CvPlayer::calculateTotalExports(YieldTypes eYield) const
{
	int iTotalExports = 0;
	FOR_EACH_CITY(pLoopCity, *this)
	{
		for (int iTradeLoop = 0; iTradeLoop < pLoopCity->getTradeRoutes(); iTradeLoop++)
		{
			CvCity* pTradeCity = pLoopCity->getTradeCity(iTradeLoop);
			if (pTradeCity != NULL)
			{
				if (pTradeCity->getOwner() != getID())
				{
					iTotalExports += pLoopCity->calculateTradeYield(eYield,
							pLoopCity->calculateTradeProfit(pTradeCity));
				}
			}
		}
	}
	return iTotalExports;
}


int CvPlayer::calculateTotalImports(YieldTypes eYield) const
{
	int iTotalImports = 0;
	for (PlayerIter<MAJOR_CIV> itOther; itOther.hasNext(); ++itOther)
	{
		if (itOther->getID() == getID())
			continue;
		FOR_EACH_CITY(pLoopCity, *itOther)
		{
			for (int iTradeLoop = 0; iTradeLoop < pLoopCity->getTradeRoutes(); iTradeLoop++)
			{
				CvCity* pTradeCity = pLoopCity->getTradeCity(iTradeLoop);
				if (pTradeCity != NULL)
				{
					if (pTradeCity->getOwner() == getID())
					{
						iTotalImports += pLoopCity->calculateTradeYield(eYield,
								pLoopCity->calculateTradeProfit(pTradeCity));
					}
				}
			}
		}
	}
	return iTotalImports;
}


int CvPlayer::calculateTotalCityUnhappiness() const
{
	int iTotalUnhappiness = 0;
	FOR_EACH_CITY(pLoopCity, *this)
		iTotalUnhappiness += pLoopCity->unhappyLevel();
	return iTotalUnhappiness;
}


int CvPlayer::calculateTotalCityHealthiness() const
{
	int iTotalHealthiness = 0;
	FOR_EACH_CITY(pLoopCity, *this)
		iTotalHealthiness += pLoopCity->goodHealth();
	return iTotalHealthiness;
}

int CvPlayer::calculateTotalCityUnhealthiness() const
{
	int iTotalUnhealthiness = 0;
	FOR_EACH_CITY(pLoopCity, *this)
		iTotalUnhealthiness += pLoopCity->badHealth();
	return iTotalUnhealthiness;
}

// K-Mod: calculate the pollution output of a civ
int CvPlayer::calculatePollution(PollutionFlags ePollution) const
{
	int iTotal = 0;
	int iBuildingWeight = 0, iBonusWeight = 0, iPowerWeight = 0, iPopWeight = 0;
	if (ePollution & POLLUTION_BUILDINGS)
		iBuildingWeight = GC.getDefineINT("GLOBAL_WARMING_BUILDING_WEIGHT");
	if (ePollution & POLLUTION_BONUSES)
		iBonusWeight = GC.getDefineINT("GLOBAL_WARMING_BONUS_WEIGHT");
	if (ePollution & POLLUTION_POWER)
		iPowerWeight = GC.getDefineINT("GLOBAL_WARMING_POWER_WEIGHT");
	if (ePollution & POLLUTION_POPULATION)
		iPopWeight = GC.getDefineINT("GLOBAL_WARMING_POPULATION_WEIGHT");

	FOR_EACH_CITY(pCity, *this)
	{
		// note: "bad health" values are negative, except for population!
		iTotal -=
				(pCity->totalBadBuildingHealth() * iBuildingWeight)
				+ (pCity->getBonusBadHealth() * iBonusWeight)
				+ (pCity->getPowerBadHealth() * iPowerWeight)
				- (pCity->unhealthyPopulation() * iPopWeight);
	}
	return iTotal;
}

// <K-Mod>
void CvPlayer::setGwPercentAnger(int iNewValue)
{
	if (iNewValue != m_iGwPercentAnger)
	{
		m_iGwPercentAnger = iNewValue;
		AI_makeAssignWorkDirty();
	}
} 


int CvPlayer::getUnitCostMultiplier() const
{
	int iMultiplier = 100;
	iMultiplier *= GC.getInfo(getHandicapType()).getUnitCostPercent();
	iMultiplier /= 100;

	if (!isHuman() && !isBarbarian())
	{
		iMultiplier *= GC.getInfo(GC.getGame().getHandicapType()).getAIUnitCostPercent();
		iMultiplier /= 100;
		// advc.251: Gold costs are no longer adjusted to handicap
		/*iMultiplier *= std::max(0, ((GC.getInfo(GC.getGame().getHandicapType()).getAIPerEraModifier() * getCurrentEra()) + 100));
		iMultiplier /= 100;*/
	}

	return iMultiplier;
} // </K-Mod>


int CvPlayer::calculateUnitCost(int& iFreeUnits, int& iFreeMilitaryUnits, int& iPaidUnits,
	int& iPaidMilitaryUnits, int& iUnitCost, int& iMilitaryCost, int& iExtraCost,
	int iExtraPop, int iExtraUnits) const // advc.004b
{
	iFreeUnits = GC.getInfo(getHandicapType()).getFreeUnits();

	iFreeUnits += getBaseFreeUnits();
	iFreeUnits += (((getTotalPopulation() /* advc.004b: */ + iExtraPop) *
			getFreeUnitsPopulationPercent()) / 100);

	iFreeMilitaryUnits = getBaseFreeMilitaryUnits();
	iFreeMilitaryUnits += (((getTotalPopulation() /* advc.004b: */ + iExtraPop) *
			getFreeMilitaryUnitsPopulationPercent()) / 100);

	/*if (!isHuman()) {
		if (GET_TEAM(getTeam()).hasMetHuman()) {
			iFreeUnits += getNumCities(); // XXX
			iFreeMilitaryUnits += getNumCities(); // XXX
		}
	}*/ // BtS - Hidden AI bonus removed by BBAI.

	iPaidUnits = std::max(0, getNumUnits() - iFreeUnits +
			iExtraUnits); // advc.004b
	iPaidMilitaryUnits = std::max(0, getNumMilitaryUnits() - iFreeMilitaryUnits +
			iExtraUnits); // advc.004b
	//iSupport = 0;
	/*iBaseUnitCost = iPaidUnits * getGoldPerUnit();
	iMilitaryCost = iPaidMilitaryUnits * getGoldPerMilitaryUnit();
	iExtraCost = getExtraUnitCost();
	iSupport = iMilitaryCost + iBaseUnitCost + iExtraCost;
	iSupport *= GC.getInfo(getHandicapType()).getUnitCostPercent();
	iSupport /= 100;
	if (!isHuman() && !isBarbarian()) {
		iSupport *= GC.getInfo(GC.getGame().getHandicapType()).getAIUnitCostPercent();
		iSupport /= 100;
		iSupport *= std::max(0, ((GC.getInfo(GC.getGame().getHandicapType()).getAIPerEraModifier() * getCurrentEra()) + 100));
		iSupport /= 100;
	}*/ // BtS
	// K-Mod. GoldPerUnit, etc, are now done as percentages.
	// Also, "UnitCostPercent" handicap modifiers now apply directly to unit cost only, not military or extra cost.
	// (iBaseUnitCost is no longer fed back to the caller. Only the modified cost is.)
	iUnitCost = iPaidUnits * getGoldPerUnit() * getUnitCostMultiplier() / 10000;
	iMilitaryCost = iPaidMilitaryUnits * getGoldPerMilitaryUnit() / 100;
	// <advc.912b>
	if (!isHuman() && !isBarbarian())
	{
		iMilitaryCost = ::round(iMilitaryCost * 0.01 * GC.getInfo(
				GC.getGame().getHandicapType()).getAIUnitSupplyPercent());
	} // </advc.912b>
	iExtraCost = getExtraUnitCost() / 100;
	int iSupport = iUnitCost + iMilitaryCost + iExtraCost;
	// K-Mod end

	FAssert(iSupport >= 0);

	return std::max(0, iSupport);
}


int CvPlayer::calculateUnitCost(
	int iExtraPop, int iExtraUnits) const // advc.004b
{
	if(isAnarchy())
		return 0;

	/*  advc (note): Several distinct variables need to be passed b/c
		calculateUnitCost uses them for intermediate results */
	int iFreeUnits, iFreeMilitaryUnits, iPaidUnits, iPaidMilitaryUnits,
			iMilitaryCost, iBaseUnitCost, iExtraCost;
	return calculateUnitCost(iFreeUnits, iFreeMilitaryUnits, iPaidUnits, iPaidMilitaryUnits,
			iBaseUnitCost, iMilitaryCost, iExtraCost,
			iExtraPop, iExtraUnits); // advc.004b
}

int CvPlayer::calculateUnitSupply(/* advc.004b: */ int iExtraOutsideUnits) const
{
	if(isAnarchy())
		return 0;
	int iPaidUnits, iBaseSupplyCost;
	return calculateUnitSupply(iPaidUnits, iBaseSupplyCost,
			iExtraOutsideUnits); // advc.004b
}

int CvPlayer::calculateUnitSupply(int& iPaidUnits, int& iBaseSupplyCost,
		int iExtraOutsideUnits) const // advc.004b
{
	static int iINITIAL_FREE_OUTSIDE_UNITS = GC.getDefineINT("INITIAL_FREE_OUTSIDE_UNITS"); // advc.opt
	static int iINITIAL_OUTSIDE_UNIT_GOLD_PERCENT = GC.getDefineINT("INITIAL_OUTSIDE_UNIT_GOLD_PERCENT"); // advc.opt

	iPaidUnits = std::max(0, getNumOutsideUnits()
		+ iExtraOutsideUnits // advc.004b
		- iINITIAL_FREE_OUTSIDE_UNITS);

	iBaseSupplyCost = iPaidUnits * iINITIAL_OUTSIDE_UNIT_GOLD_PERCENT;
	iBaseSupplyCost /= 100;
	int iSupply = iBaseSupplyCost;
	if (!isHuman() && !isBarbarian())
	{
		iSupply *= GC.getInfo(GC.getGame().getHandicapType()).getAIUnitSupplyPercent();
		iSupply /= 100;
		// advc.250d: Commented out
		/*iSupply *= std::max(0, ((GC.getInfo(GC.getGame().getHandicapType()).getAIPerEraModifier() * getCurrentEra()) + 100));
		iSupply /= 100;*/
	}
	FAssert(iSupply >= 0);
	return iSupply;
}


int CvPlayer::calculatePreInflatedCosts() const
{
	return calculateUnitCost() + calculateUnitSupply() + getTotalMaintenance() +
			getCivicUpkeep() + GC.getPythonCaller()->extraExpenses(getID());
}


int CvPlayer::calculateInflationRate() const
{
	CvGame const& kGame = GC.getGame();
	int iTurns = (kGame.getGameTurn() + kGame.getElapsedGameTurns()) / 2;
	if (kGame.getMaxTurns() > 0)
		iTurns = std::min(kGame.getMaxTurns(), iTurns);
	iTurns += GC.getInfo(kGame.getGameSpeedType()).getInflationOffset();
	if (iTurns <= 0)
		return 0;

	int iInflationPerTurnTimes10000 = GC.getInfo(kGame.getGameSpeedType()).
			getInflationPercent();
	iInflationPerTurnTimes10000 *= GC.getInfo(getHandicapType()).getInflationPercent();
	iInflationPerTurnTimes10000 /= 100;

	int iModifier = m_iInflationModifier;
	if (!isHuman() && !isBarbarian())
	{
		int iAIModifier = GC.getInfo(kGame.getHandicapType()).getAIInflationPercent();
		// advc.251: Gold costs are no longer adjusted to handicap
		/*iAIModifier *= std::max(0, ((GC.getInfo(GC.getGame().getHandicapType()).getAIPerEraModifier() * getCurrentEra()) + 100));
		iAIModifier /= 100;*/
		iModifier += iAIModifier - 100;
	}

	iInflationPerTurnTimes10000 *= std::max(0, 100 + iModifier);
	iInflationPerTurnTimes10000 /= 100;

	// Keep up to second order terms in binomial series
	int iRatePercent = (iTurns * iInflationPerTurnTimes10000) / 100;
	iRatePercent += (iTurns * (iTurns - 1) * iInflationPerTurnTimes10000 *
			iInflationPerTurnTimes10000) / 2000000;
	FAssert(iRatePercent >= 0);
	return iRatePercent;
}


int CvPlayer::calculateInflatedCosts() const
{
	PROFILE_FUNC();

	int iCosts = calculatePreInflatedCosts();
	iCosts *= std::max(0, calculateInflationRate() + 100);
	iCosts /= 100;
	return iCosts;
}


/*int CvPlayer::calculateBaseNetGold() const { // (see calculateGoldRate)
	int iNetGold = (getCommerceRate(COMMERCE_GOLD) + getGoldPerTurn());
	iNetGold -= calculateInflatedCosts();
	return iNetGold;
}*/

int CvPlayer::calculateResearchModifier(TechTypes eTech,  // <advc.910>
		int* piFromOtherKnown, int* piFromPaths, int* piFromTeam) const
{
	// So that the caller isn't required to provide the pointer params
	int iFromOtherKnown, iFromPaths, iFromTeam;
	if(piFromOtherKnown == NULL)
		piFromOtherKnown = &iFromOtherKnown;
	if(piFromPaths == NULL)
		piFromPaths = &iFromPaths;
	if(piFromTeam == NULL)
		piFromTeam = &iFromTeam;
	*piFromOtherKnown = *piFromPaths = *piFromTeam = 0;
	// </advc.910>
	int iModifier = 100;
	if(NO_TECH == eTech)
		return iModifier;

	// BETTER_BTS_AI_MOD, Tech Diffusion, 07/27/09, jdog5000: START
	static bool const bTECH_DIFFUSION_ENABLE = GC.getDefineBOOL("TECH_DIFFUSION_ENABLE");
	if (bTECH_DIFFUSION_ENABLE)
	{
		scaled rKnownExp; // advc: BBAI had used floating-point math
		// Tech flows better through open borders
		for (TeamIter<CIV_ALIVE,KNOWN_TO> it(getTeam()); it.hasNext(); ++it)
		{
			CvTeam const& kTechTeam = *it;
			if (!kTechTeam.isHasTech(eTech))
				continue;
			rKnownExp += fixp(0.5);
			if (GET_TEAM(getTeam()).canPeacefullyEnter(kTechTeam.getID()))
				rKnownExp += fixp(1.5);
			else if (kTechTeam.isAtWar(getTeam()) || GET_TEAM(getTeam()).isVassal(kTechTeam.getID()))
				rKnownExp += fixp(0.5);
		}
		static int const iTechDiffMod = GC.getDefineINT("TECH_DIFFUSION_KNOWN_TEAM_MODIFIER", 30);
		if (rKnownExp > 0)
		{
			*piFromOtherKnown += // advc.910
					iTechDiffMod - (iTechDiffMod * fixp(0.85).pow(rKnownExp)).round();
		}
		// Tech flows downhill to those who are far behind
		int iTechScorePercent = GET_TEAM(getTeam()).getBestKnownTechScorePercent();
		static int const iWelfareThreshold = GC.getDefineINT("TECH_DIFFUSION_WELFARE_THRESHOLD", 88);
		if (iTechScorePercent < iWelfareThreshold)
		{
			if (rKnownExp > 0)
			{
				static int const iWelfareModifier = GC.getDefineINT("TECH_DIFFUSION_WELFARE_MODIFIER", 30);
				*piFromOtherKnown += // advc.910
						(iWelfareModifier * GC.getGame().getCurrentEra() *
						(iWelfareThreshold - iTechScorePercent)) / 200;
			}
		}
	}
	else
	{	// BtS tech diffusion
		int iKnownCount = 0;
		for (TeamIter<CIV_ALIVE,KNOWN_TO> it(getTeam()); it.hasNext(); ++it)
		{
			if (it->isHasTech(eTech))
				iKnownCount++;
		}
		int const iPossibleKnownCount = TeamIter<CIV_ALIVE>::count();
		if (iPossibleKnownCount > 0)
		{
			static int iTECH_COST_TOTAL_KNOWN_TEAM_MODIFIER = GC.getDefineINT("TECH_COST_TOTAL_KNOWN_TEAM_MODIFIER"); // advc.opt
			*piFromOtherKnown += // advc.910
				(iTECH_COST_TOTAL_KNOWN_TEAM_MODIFIER * iKnownCount) / iPossibleKnownCount;
		}
	}
	iModifier += *piFromOtherKnown; // advc.910

	int iPossiblePaths = 0;
	int iUnknownPaths = 0;
	for (int i = 0; i < GC.getInfo(eTech).getNumOrTechPrereqs(); i++)
	{
		if (!GET_TEAM(getTeam()).isHasTech(GC.getInfo(eTech).getPrereqOrTechs(i)))
			iUnknownPaths++;
		iPossiblePaths++;
	}
	FAssert(iPossiblePaths >= iUnknownPaths);
	if(iPossiblePaths > iUnknownPaths)
	{
		*piFromPaths += // advc.910
				GC.getDefineINT(CvGlobals::TECH_COST_FIRST_KNOWN_PREREQ_MODIFIER);
		iPossiblePaths--;
		*piFromPaths += (iPossiblePaths - iUnknownPaths) *
				GC.getDefineINT(CvGlobals::TECH_COST_KNOWN_PREREQ_MODIFIER);
	}
	// BETTER_BTS_AI_MOD: END
	iModifier += *piFromPaths;
	// <advc.156>
	for (MemberIter it(getTeam()); it.hasNext(); ++it)
	{
		CvPlayer const& kMember = *it;
		if (kMember.getID() != getID() && kMember.getCurrentResearch() == eTech)
		{
			*piFromTeam = // advc.910
					GC.getDefineINT(CvGlobals::RESEARCH_MODIFIER_EXTRA_TEAM_MEMBER); // advc.210
			break; // Or should the penalty stack?
		}
	}
	iModifier += *piFromTeam; // advc.910
	// </advc.156>
	iModifier -= groundbreakingPenalty(eTech); // advc.groundbr
	return iModifier;
}

/* int CvPlayer::calculateBaseNetResearch(TechTypes eTech) const {  // (see calculateResearchRate)
	TechTypes eResearchTech;
	if(eTech != NO_TECH)
		eResearchTech = eTech;
	else eResearchTech = getCurrentResearch();
	return (((GC.getDefineINT("BASE_RESEARCH_RATE") + getCommerceRate(COMMERCE_RESEARCH)) * calculateResearchModifier(eResearchTech)) / 100);
} */

// advc.groundbr: Loosely based on CvTeam::getSpreadResearchModifier in DoC
int CvPlayer::groundbreakingPenalty(TechTypes eTech) const
{
	bool const bAIEnable = GC.getDefineBOOL(CvGlobals::AI_GROUNDBREAKING_PENALTY_ENABLE);
	bool const bHumanEnable = GC.getDefineBOOL(CvGlobals::HUMAN_GROUNDBREAKING_PENALTY_ENABLE);
	if (isHuman() ? !bHumanEnable : !bAIEnable)
		return 0;
	EraTypes const eCurrentEra = getCurrentEra();
	EraTypes const eStartEra = GC.getGame().getStartEra();
	if (eCurrentEra <= eStartEra)
		return 0;
	int const iMaxPenalty = GC.getInfo(eCurrentEra).get(isHuman() ?
			CvEraInfo::HumanMaxGroundbreakingPenalty :
			CvEraInfo::AIMaxGroundbreakingPenalty);
	if (iMaxPenalty == 0)
		return 0;
	// For comments see TechDiffusion_GlobalDefines.xml or the original code (DoC mod)
	int iTotal = 0;
	int iHasTech = 0;
	for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
	{
		CvTeam const& kLoopTeam = GET_TEAM(it->getTeam());
		if (kLoopTeam.isCapitulated())
			continue;
		iTotal++;
		if (kLoopTeam.isHasTech(eTech))
			iHasTech++;
	}
	scaled const rPercentile(iTotal, 4); // quartile
	if (iHasTech >= rPercentile)
		return 0;
	return (iMaxPenalty * (rPercentile - iHasTech) / rPercentile).round();
}


int CvPlayer::calculateGoldRate() const
{	/*int iRate = 0;
	if(isCommerceFlexible(COMMERCE_RESEARCH))
		iRate = calculateBaseNetGold();
	else iRate = std::min(0, (calculateBaseNetResearch() + calculateBaseNetGold()));
	return iRate;*/ // BtS
	// K-Mod. (Just moved from calculateBaseNetGold.)
	int iNetGold = getCommerceRate(COMMERCE_GOLD) + getGoldPerTurn();
	iNetGold -= calculateInflatedCosts();
	return iNetGold;
	// K-Mod end
}


int CvPlayer::calculateResearchRate(TechTypes eTech) const
{	// <advc.004x> No BASE_RESEARCH_RATE either
	if(!isResearch())
		return 0; // </advc.004x>
	/*int iRate = 0;
	if(isCommerceFlexible(COMMERCE_RESEARCH))
		iRate = calculateBaseNetResearch(eTech);
	else iRate = std::max(1, (calculateBaseNetResearch(eTech) + calculateBaseNetGold()));
	return iRate;*/ // BtS
	// K-Mod. (Just moved from calculateBaseNetResearch.)
	// Note: the original code had a floor of 1. This version does not.
	TechTypes eResearchTech;
	if(eTech != NO_TECH)
		eResearchTech = eTech;
	else eResearchTech = getCurrentResearch();
	// advc.004x: BASE_RESEARCH_RATE now handled by getCommerceRate
	return (getCommerceRate(COMMERCE_RESEARCH) *
			calculateResearchModifier(eResearchTech)) / 100;
	// K-Mod end
}

int CvPlayer::calculateTotalCommerce() const
{	//int iTotalCommerce = calculateBaseNetGold() + calculateBaseNetResearch();
	/*int iTotalCommerce = calculateGoldRate() + calculateResearchRate(); // K-Mod
	for (int i = 0; i < NUM_COMMERCE_TYPES; ++i) {
		if (COMMERCE_GOLD != i && COMMERCE_RESEARCH != i)
			iTotalCommerce += getCommerceRate((CommerceTypes)i);
	}
	return iTotalCommerce;*/
	/*  <advc.004s> Replacing the K-Mod code above.
		Of course this isn't really "total commerce" anymore. */
	int r = getCommerceRate(COMMERCE_GOLD) + getGoldPerTurn() +
			getCommerceRate(COMMERCE_RESEARCH);
	return r;
	// </advc.004s>
}

bool CvPlayer::isResearch() const
{
	if (!GC.getPythonCaller()->canDoResearch(getID()))
		return false;
	return (isFoundedFirstCity() /* advc.004x: */ && !isAnarchy());
}


bool CvPlayer::canEverResearch(TechTypes eTech) const
{
	if (GC.getInfo(eTech).isDisable())
		return false;
	if (GC.getInfo(getCivilizationType()).isCivilizationDisableTechs(eTech))
		return false;
	if (GC.getPythonCaller()->cannotResearchOverride(getID(), eTech, false))
		return false;
	return true;
}


TechTypes CvPlayer::getDiscoveryTech(UnitTypes eUnit) const
{
	TechTypes eBestTech = NO_TECH;
	int iBestValue = 0;
	FOR_EACH_ENUM(Tech)
	{
		if (canResearch(eLoopTech))
		{
			int iValue = 0;
			FOR_EACH_ENUM(Flavor)
			{
				iValue += (GC.getInfo(eLoopTech).getFlavorValue(eLoopFlavor) *
						GC.getInfo(eUnit).getFlavorValue(eLoopFlavor));
			}
			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				eBestTech = eLoopTech;
			}
		}
	}
	return eBestTech;
}


bool CvPlayer::canResearch(TechTypes eTech, bool bTrade,
	bool bFree, // K-Mod (advc.004x: disused)
	bool bCouldResearchAgain) const // advc.126
{
	if (GC.getPythonCaller()->canResearchOverride(getID(), eTech, bTrade))
		return true;

	/*  advc.004x: Commented out - shouldn't matter here.
		(And wouldn't prevent players from queuing up research during anarchy or
		before founding a city) */
	/*if(!isResearch() && getAdvancedStartPoints() < 0 &&
		!bFree) // K-Mod
	{
		return false;
	}*/

	if (GET_TEAM(getTeam()).isHasTech(eTech) /* advc.126: */ && !bCouldResearchAgain)
		return false;

	bool bFoundPossible = false;
	bool bFoundValid = false;
	for (int i = 0; i < GC.getInfo(eTech).getNumOrTechPrereqs(); i++)
	{
		TechTypes const ePrereq = GC.getInfo(eTech).getPrereqOrTechs(i);
		FAssert(ePrereq != eTech); // advc
		bFoundPossible = true;
		if (GET_TEAM(getTeam()).isHasTech(ePrereq) &&
			// advc.126: Don't check recursively (for execution speed concerns)
			(bCouldResearchAgain || canResearch(ePrereq, false, true, true)))
		{
			if (!bTrade || GC.getGame().isOption(GAMEOPTION_NO_TECH_BROKERING) ||
				!GET_TEAM(getTeam()).isNoTradeTech(ePrereq))
			{
				bFoundValid = true;
				break;
			}
		}
	}

	if (bFoundPossible && !bFoundValid)
		return false;

	for (int i = 0; i < GC.getInfo(eTech).getNumAndTechPrereqs(); i++)
	{
		TechTypes const ePrereq = GC.getInfo(eTech).getPrereqAndTechs(i);
		if (!GET_TEAM(getTeam()).isHasTech(ePrereq) ||
			// advc.126:
			(bCouldResearchAgain && !canResearch(ePrereq, false, true, true)))
		{
			return false;
		}
		if (bTrade && !GC.getGame().isOption(GAMEOPTION_NO_TECH_BROKERING) &&
			GET_TEAM(getTeam()).isNoTradeTech(ePrereq))
		{
			return false;
		}
	}

	if (!canEverResearch(eTech))
		return false;

	return true;
}


TechTypes CvPlayer::getCurrentResearch() const
{
	CLLNode<TechTypes>* pResearchNode = headResearchQueueNode();
	if (pResearchNode == NULL)
		return NO_TECH;
	return pResearchNode->m_data;
}


bool CvPlayer::isCurrentResearchRepeat() const
{
	TechTypes eCurrentResearch = getCurrentResearch();
	if (eCurrentResearch == NO_TECH)
		return false;
	return GC.getInfo(eCurrentResearch).isRepeat();
}


bool CvPlayer::isNoResearchAvailable() const
{
	if (getCurrentResearch() != NO_TECH)
		return false;

	FOR_EACH_ENUM(Tech)
	{
		if (canResearch(eLoopTech))
			return false;
	}
	return true;
}


int CvPlayer::getResearchTurnsLeft(TechTypes eTech, bool bOverflow) const
{
	int iTurnsLeft = getResearchTurnsLeftTimes100(eTech, bOverflow);
	if(iTurnsLeft >= 0) // advc.004x
		iTurnsLeft = intdiv::uceil(iTurnsLeft, 100);
	//return std::max(1, iTurnsLeft);
	return iTurnsLeft; // advc.004x: -1 now means infinitely many turns
}

int CvPlayer::getResearchTurnsLeftTimes100(TechTypes eTech, bool bOverflow) const
{
	// <advc>
	if (GET_TEAM(getTeam()).isHasTech(eTech))
		return 0; // </advc>
	int iResearchRate = 0;
	int iOverflow = 0;

	for (MemberIter it(getTeam()); it.hasNext(); ++it)
	{
		CvPlayer const& kMember = *it;
		if (!kMember.isResearch()) // advc.004x
			continue;
		if (kMember.getID() == getID() || kMember.getCurrentResearch() == eTech)
		{
			//iResearchRate += GET_PLAYER((PlayerTypes)iI).calculateResearchRate(eTech);
			// K-Mod (replacing the minimum which used to be in calculateResearchRate)
				// advc.004x: Ensure only a non-negative rate (not positive)
			iResearchRate += std::max(0, kMember.calculateResearchRate(eTech));
			iOverflow += (kMember.getOverflowResearch() *
					calculateResearchModifier(eTech)) / 100;
		}
	}
	// BETTER_BTS_AI_MOD, Tech AI, 03/18/10, jdog5000: START
	// Mainly just so debug display shows sensible value
	int iResearchLeft = GET_TEAM(getTeam()).getResearchLeft(eTech);
	if(bOverflow)
		iResearchLeft -= iOverflow;
	// <advc.004x>
	if (iResearchLeft <= 0)
		return 1; // 1/100. getResearchTurnsLeft will round that up.
	// </advc.004x>
	iResearchLeft *= 100;
	if(iResearchRate <= 0)
	{
		//return iResearchLeft;
		return -1; // advc.004x
	} // BETTER_BTS_AI_MOD: END

	int iTurnsLeft = iResearchLeft / iResearchRate;
	if(iTurnsLeft * iResearchRate < iResearchLeft)
		iTurnsLeft++;
	//return std::max(1, iTurnsLeft);
	return iTurnsLeft; // advc.004x
}

// advc.120d:
bool CvPlayer::canSeeTech(PlayerTypes eOther) const
{
	if (eOther == NO_PLAYER)
		return false;
	return GET_TEAM(getTeam()).canSeeTech(TEAMID(eOther));
}

/*	advc: To get rid of some duplicate K-Mod code in canSeeResearch, canSeeDemographics.
	bDemographics = true checks the latter, bDemographics=false the former. */
bool CvPlayer::canSeeIntel(PlayerTypes ePlayer, bool bDemographics,
	bool bCheckPoints) const // advc.085
{
	CvPlayer const& kOther = GET_PLAYER(ePlayer);

	if (kOther.getTeam() == getTeam() || GET_TEAM(kOther.getTeam()).isVassal(getTeam()))
		return true;

	if (!GET_TEAM(getTeam()).isHasMet(kOther.getTeam()))
		return false;

	if (!GC.getGame().isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		FOR_EACH_ENUM(EspionageMission)
		{
			CvEspionageMissionInfo const& kLoopMission = GC.getInfo(eLoopEspionageMission);
			if (kLoopMission.isPassive() &&
				((!bDemographics && kLoopMission.isSeeResearch()) ||
				(bDemographics && kLoopMission.isSeeDemographics())) &&
				canDoEspionageMission(eLoopEspionageMission, ePlayer, NULL, 0, NULL,
				bCheckPoints)) // advc.085
			{
				return true;
			}
		}
	}
	return false;
}

// K-Mod. Return true if this player can see what ePlayer is researching
bool CvPlayer::canSeeResearch(PlayerTypes ePlayer, /* advc.085: */ bool bCheckPoints) const
{
	return canSeeIntel(ePlayer, false, bCheckPoints); // advc: Moved into aux. function
}

// Return true if this player can see ePlayer's graphs
bool CvPlayer::canSeeDemographics(PlayerTypes ePlayer, /* advc.085: */ bool bCheckPoints) const 
{
	return canSeeIntel(ePlayer, true, bCheckPoints); // advc: Moved into aux. function
} // K-Mod end

/*  advc.085: !bDemographics means: return espionage needed to see research.
	Akin to code in canSeeIntel and canDoEspionageMission. */
int CvPlayer::espionageNeededToSee(PlayerTypes ePlayer, bool bDemographics) const
{
	int iR = MAX_INT;
	if (!canSeeIntel(ePlayer, bDemographics, false))
		return iR;
	int const iEspionagePoints = GET_TEAM(getTeam()).getEspionagePointsAgainstTeam(TEAMID(ePlayer));
	FOR_EACH_ENUM(EspionageMission)
	{
		CvEspionageMissionInfo& kLoopMission = GC.getInfo(eLoopEspionageMission);
		if (!kLoopMission.isPassive())
			continue;
		if (bDemographics ? kLoopMission.isSeeDemographics() : kLoopMission.isSeeResearch())
		{
			iR = std::min(iR, getEspionageMissionCost(
					eLoopEspionageMission, ePlayer, NULL, 0, NULL) - iEspionagePoints);
		}
	}
	return iR;
}

// advc.091:
void CvPlayer::updateEverSeenDemographics(TeamTypes eTargetTeam)
{
	for (MemberIter it(eTargetTeam); it.hasNext(); ++it)
	{
		if (!hasEverSeenDemographics(it->getID()) && canSeeDemographics(it->getID()))
			m_abEverSeenDemographics.set(it->getID(), true);
	}
}

// advc.550e:
bool CvPlayer::isSignificantDiscovery(TechTypes eTech) const
{
	/*	(K-Mod comment moved here from CvDeal::startTrade)
		Only adjust tech_from_any memory if this is a tech from a recent era
		and the team receiving the tech isn't already more than 2/3 of the way through.
		(This is to prevent the AI from being crippled by
		human players selling them lots of tech scraps.)
		Note: the current game era is the average of all the player eras, rounded down.
		(It no longer includes Barbarians.) */
	// advc: I'm going to use the recipient's era instead, the rest is as in K-Mod.
	return (GC.getInfo(eTech).getEra() >= getCurrentEra() - 1 &&
			GET_TEAM(getTeam()).getResearchLeft(eTech) >
			GET_TEAM(getTeam()).getResearchCost(eTech) / 3);
}

bool CvPlayer::isCivic(CivicTypes eCivic) const
{
	FOR_EACH_ENUM(CivicOption)
	{
		if (getCivics(eLoopCivicOption) == eCivic)
			return true;
	}
	return false;
}


bool CvPlayer::canDoCivics(CivicTypes eCivic) const
{
	PROFILE_FUNC();

	/*  UNOFFICIAL_PATCH, Tech AI (Bugfix), 02/16/10, jdog5000: START
		Circumvents second crash bug in simultaneous turns MP games */
	if (eCivic == NO_CIVIC)
		return true; // UNOFFICIAL_PATCH: END

	if (GC.getGame().isForceCivicOption(GC.getInfo(eCivic).getCivicOptionType()))
		return GC.getGame().isForceCivic(eCivic);

	if (GC.getPythonCaller()->canDoCivicOverride(getID(), eCivic))
		return true;

	if (!isHasCivicOption(GC.getInfo(eCivic).getCivicOptionType()) &&
		!GET_TEAM(getTeam()).isHasTech(GC.getInfo(eCivic).getTechPrereq()))
	{
		return false;
	}
	if (GC.getPythonCaller()->cannotDoCivicOverride(getID(), eCivic))
		return false;
	// <advc.912d>
	if(GC.getGame().isOption(GAMEOPTION_NO_SLAVERY) && isHuman())
	{
		FOR_EACH_ENUM(Hurry)
		{
			if(GC.getInfo(eCivic).isHurry(eLoopHurry) &&
				GC.getInfo(eLoopHurry).getProductionPerPopulation() > 0)
			{
				return false;
			}
		}
	} // </advc.912d>
	return true;
}

// advc.enum: Was previously handled by canRevolution
bool CvPlayer::canDoAnyRevolution() const
{
	if (isAnarchy() || getRevolutionTimer() > 0)
		return false;
	FOR_EACH_ENUM(Civic)
	{
		if (canDoCivics(eLoopCivic))
		{
			if (getCivics(GC.getInfo(eLoopCivic).getCivicOptionType()) != eLoopCivic)
				return true;
		}
	}
	return false;
}


bool CvPlayer::canRevolution(CivicMap const& kNewCivics) const 
{
	if (isAnarchy() || getRevolutionTimer() > 0)
		return false;
	//if (aeNewCivics == NULL) {...} // advc.enum: Moved into new function canDoAnyRevolution
	FOR_EACH_ENUM(CivicOption)
	{
		if (GC.getGame().isForceCivicOption(eLoopCivicOption))
		{
			if (!GC.getGame().isForceCivic(kNewCivics.get(eLoopCivicOption)))
				return false;
		}
		if (getCivics(eLoopCivicOption) != kNewCivics.get(eLoopCivicOption))
			return true;
	}
	return false;
}


void CvPlayer::revolution(CivicMap const& kNewCivics, bool bForce)
{
	if (!bForce && !canRevolution(kNewCivics))
		return;

	int const iAnarchyLength = getCivicAnarchyLength(kNewCivics);
	if (iAnarchyLength > 0)
	{
		changeAnarchyTurns(iAnarchyLength);
		FOR_EACH_ENUM(CivicOption)
			setCivics(eLoopCivicOption, kNewCivics.get(eLoopCivicOption));
	}
	else
	{
		FOR_EACH_ENUM(CivicOption)
			setCivics(eLoopCivicOption, kNewCivics.get(eLoopCivicOption));
	}
	// advc: Revolution turns calculation moved into auxiliary function
	setRevolutionTimer(iAnarchyLength + getMinTurnsBetweenRevolutions());

	if (getID() == GC.getGame().getActivePlayer())
	{
		// to force an update of the civic chooser popup
		gDLL->UI().setDirty(Popup_DIRTY_BIT, true);
		// <advc.004x>
		killAll(BUTTONPOPUP_CHANGECIVIC);
		if(iAnarchyLength > 0)
		{
			killAll(BUTTONPOPUP_CHOOSEPRODUCTION);
			killAll(BUTTONPOPUP_CHOOSETECH);
		} // </advc.004x>
	}
}


int CvPlayer::getCivicPercentAnger(CivicTypes eCivic, bool bIgnore) const
{
	if (GC.getInfo(eCivic).getCivicPercentAnger() == 0)
		return 0;

	CivicOptionTypes const eCivicOption = GC.getInfo(eCivic).getCivicOptionType();
	if (!bIgnore && getCivics(eCivicOption) == eCivic)
		return 0;

	int iCount = 0;
	int iPossibleCount = 0;
	// <advc.155> Don't skip our teammates (but do skip unmet civs)
	for (PlayerIter<CIV_ALIVE,KNOWN_TO> itOther(getTeam());
		itOther.hasNext(); ++itOther)
	{
		if (itOther->getID() == getID())
			continue; // </advc.155>
		if (itOther->getCivics(eCivicOption) == eCivic)
			iCount += itOther->getNumCities();
		iPossibleCount += itOther->getNumCities();
	}

	if (iPossibleCount <= 0)
		return 0;

	return (GC.getInfo(eCivic).getCivicPercentAnger() * iCount) / iPossibleCount;
}


bool CvPlayer::canChangeReligion() const
{
	FOR_EACH_ENUM(Religion)
	{
		if (canConvert(eLoopReligion))
			return true;
	}
	return false;
}


bool CvPlayer::canConvert(ReligionTypes eReligion) const
{
	if (isBarbarian())
		return false;
	if (isAnarchy())
		return false;
	if (getConversionTimer() > 0)
		return false;
	if (!isStateReligion())
		return false;
	if (getLastStateReligion() == eReligion)
		return false;
	if (eReligion != NO_RELIGION && !canDoReligion(eReligion))
		return false;
	return true;
}


void CvPlayer::convert(ReligionTypes eReligion, /* <advc.001v> */ bool bForce)
{
	if(!bForce && /* </advc.001v> */ !canConvert(eReligion))
		return;

	int const iAnarchyLength = getReligionAnarchyLength();
	changeAnarchyTurns(iAnarchyLength);
	setLastStateReligion(eReligion);
	setConversionTimer(std::max(1, ((100 + getAnarchyModifier()) *
			GC.getDefineINT("MIN_CONVERSION_TURNS")) / 100) + iAnarchyLength);
	// <advc.004x>
	if(getID() == GC.getGame().getActivePlayer())
	{
		killAll(BUTTONPOPUP_CHANGERELIGION);
		if(iAnarchyLength > 0) {
			killAll(BUTTONPOPUP_CHOOSEPRODUCTION);
			killAll(BUTTONPOPUP_CHOOSETECH);
		}
	} // </advc.004x>
}


bool CvPlayer::hasHolyCity(ReligionTypes eReligion) const
{
	CvCity* pHolyCity = GC.getGame().getHolyCity(eReligion);
	if (pHolyCity != NULL)
		return (pHolyCity->getOwner() == getID());
	return false;
}


int CvPlayer::countHolyCities() const
{
	int iCount = 0;
	FOR_EACH_ENUM(Religion)
	{
		if (hasHolyCity(eLoopReligion))
			iCount++;
	}
	return iCount;
}


void CvPlayer::foundReligion(ReligionTypes eReligion, ReligionTypes eSlotReligion, bool bAward)
{
	if (eReligion == NO_RELIGION)
		return;

	CvReligionInfo const& kSlotReligion = GC.getInfo(eSlotReligion);
	CvGame& kGame = GC.getGame();
	if (kGame.isReligionFounded(eReligion))
	{
		if (isHuman())
		{
			CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_FOUND_RELIGION, eSlotReligion);
			if (pInfo != NULL)
				gDLL->UI().addPopup(pInfo, getID());
		}
		else foundReligion(AI().AI_chooseReligion(), eSlotReligion, bAward);
		return;
	}
	kGame.setReligionSlotTaken(eSlotReligion, true);

	bool const bStarting = (kSlotReligion.getTechPrereq() == NO_TECH ||
			GC.getInfo((TechTypes)kSlotReligion.getTechPrereq()).getEra() < kGame.getStartEra());

	int iBestValue = 0;
	CvCity* pBestCity = NULL;
	FOR_EACH_CITY_VAR(pLoopCity, *this)
	{
		if (bStarting && pLoopCity->isHolyCity())
			continue;

		int iValue = 10;
		iValue += pLoopCity->getPopulation();
		iValue += kGame.getSorenRandNum(GC.getDefineINT("FOUND_RELIGION_CITY_RAND"), "Found Religion");
		iValue /= (pLoopCity->getReligionCount() + 1);
		if (pLoopCity->isCapital())
			iValue /= 8;
		iValue = std::max(1, iValue);

		if (iValue > iBestValue)
		{
			iBestValue = iValue;
			pBestCity = pLoopCity;
		}
	}

	if (pBestCity == NULL)
		return;

	kGame.setHolyCity(eReligion, pBestCity, true);
	if (bAward && kSlotReligion.getNumFreeUnits() > 0)
	{
		UnitTypes eFreeUnit = getCivilization().getUnit((UnitClassTypes)
				GC.getInfo(eReligion).getFreeUnitClass());
		if (eFreeUnit != NO_UNIT)
		{
			for (int i = 0; i < kSlotReligion.getNumFreeUnits(); i++)
				initUnit(eFreeUnit, pBestCity->getX(), pBestCity->getY());
		}
	}
}


bool CvPlayer::hasHeadquarters(CorporationTypes eCorporation) const
{
	CvCity const* pHeadquarters = GC.getGame().getHeadquarters(eCorporation);
	return (pHeadquarters != NULL && pHeadquarters->getOwner() == getID());
}


int CvPlayer::countHeadquarters() const
{
	int iCount = 0;
	FOR_EACH_ENUM(Corporation)
	{
		if (hasHeadquarters(eLoopCorporation))
			iCount++;
	}
	return iCount;
}


int CvPlayer::countCorporations(CorporationTypes eCorporation,
	CvArea const* pArea) const // K-Mod
{
	int iCount = 0;
	FOR_EACH_CITY(pLoopCity, *this)
	{
		if (pArea == NULL || pLoopCity->isArea(*pArea)) // K-Mod
		{
			if (pLoopCity->isHasCorporation(eCorporation))
				iCount++;
		}
	}
	return iCount;
}


void CvPlayer::foundCorporation(CorporationTypes eCorporation)
{
	if (GC.getGame().isCorporationFounded(eCorporation))
		return;

	CvCorporationInfo const& kCorp = GC.getInfo(eCorporation);
	bool const bStarting = (kCorp.getTechPrereq() == NO_TECH ||
			GC.getInfo((TechTypes)kCorp.getTechPrereq()).getEra() <
			GC.getGame().getStartEra());

	int iBestValue = 0;
	CvCity* pBestCity = NULL;
	FOR_EACH_CITY_VAR(pLoopCity, *this)
	{
		if (bStarting && pLoopCity->isHeadquarters())
			continue;

		int iValue = 10 + pLoopCity->getPopulation();
		for (int i = 0; i < GC.getInfo(eCorporation).getNumPrereqBonuses(); ++i)
		{
			iValue += 10 * pLoopCity->getNumBonuses(
					GC.getInfo(eCorporation).getPrereqBonus(i));
		}
		iValue += GC.getGame().getSorenRandNum(GC.getDefineINT("FOUND_CORPORATION_CITY_RAND"),
				"Found Corporation");
		iValue /= (pLoopCity->getCorporationCount() + 1);
		iValue = std::max(1, iValue);
		if (iValue > iBestValue)
		{
			iBestValue = iValue;
			pBestCity = pLoopCity;
		}
	}

	if (pBestCity != NULL)
		pBestCity->setHeadquarters(eCorporation);
}


int CvPlayer::getCivicAnarchyLength(CivicMap const& kNewCivics,
	bool bIgnoreGoldenAge) const // advc.132
{
	int const iMaxAnarchyTurns = getMaxAnarchyTurns(); // advc
	if (iMaxAnarchyTurns == 0)
		return 0;
	if(/* <advc.132> */ !bIgnoreGoldenAge && /* </advc.132> */ isGoldenAge())
		return 0;

	int iAnarchyLength = 0;
	bool bChange = false;
	FOR_EACH_ENUM(CivicOption)
	{
		CivicTypes eNewCivic = kNewCivics.get(eLoopCivicOption);
		if (eNewCivic != getCivics(eLoopCivicOption))
		{
			iAnarchyLength += GC.getInfo(eNewCivic).getAnarchyLength();
			bChange = true;
		}
	}
	if (bChange)
	{
		static int const iBASE_CIVIC_ANARCHY_LENGTH = GC.getDefineINT("BASE_CIVIC_ANARCHY_LENGTH"); // advc.opt
		iAnarchyLength += iBASE_CIVIC_ANARCHY_LENGTH;
		iAnarchyLength += ((getNumCities() * GC.getInfo(GC.getMap().
				getWorldSize()).getNumCitiesAnarchyPercent()) / 100);
	}

	iAnarchyLength = ((iAnarchyLength * std::max(0, getAnarchyModifier() + 100)) / 100);

	if(iAnarchyLength == 0)
		return 0;

	iAnarchyLength *= GC.getInfo(GC.getGame().getGameSpeedType()).getAnarchyPercent();
	iAnarchyLength /= 100;

	iAnarchyLength *= GC.getInfo(GC.getGame().getStartEra()).getAnarchyPercent();
	iAnarchyLength /= 100;

	return range(iAnarchyLength, 1, iMaxAnarchyTurns);
}


int CvPlayer::getReligionAnarchyLength(/* advc.132: */ bool ignoreGoldenAge) const
{
	if(getMaxAnarchyTurns() == 0)
		return 0;

	if(/* <advc.132> */ !ignoreGoldenAge && /* </advc.132> */ isGoldenAge())
		return 0;

	static int const iBASE_RELIGION_ANARCHY_LENGTH = GC.getDefineINT("BASE_RELIGION_ANARCHY_LENGTH"); // advc.opt
	int iAnarchyLength = iBASE_RELIGION_ANARCHY_LENGTH;
	iAnarchyLength += ((getNumCities() * GC.getInfo(GC.getMap().
			getWorldSize()).getNumCitiesAnarchyPercent()) / 100);

	iAnarchyLength = ((iAnarchyLength * std::max(0, getAnarchyModifier() + 100)) / 100);

	if(iAnarchyLength == 0)
		return 0;

	iAnarchyLength *= GC.getInfo(GC.getGame().getGameSpeedType()).getAnarchyPercent();
	iAnarchyLength /= 100;

	iAnarchyLength *= GC.getInfo(GC.getGame().getStartEra()).
			getAnarchyPercent();
	iAnarchyLength /= 100;

	return range(iAnarchyLength, 1, getMaxAnarchyTurns());
}



int CvPlayer::unitsRequiredForGoldenAge() const
{
	static int const iBASE_GOLDEN_AGE_UNITS = GC.getDefineINT("BASE_GOLDEN_AGE_UNITS"); // advc.opt
	static int const iGOLDEN_AGE_UNITS_MULTIPLIER = GC.getDefineINT("GOLDEN_AGE_UNITS_MULTIPLIER"); // advc.opt
	return (iBASE_GOLDEN_AGE_UNITS + getNumUnitGoldenAges() * iGOLDEN_AGE_UNITS_MULTIPLIER);
}


int CvPlayer::unitsGoldenAgeCapable() const
{
	int iCount = 0;
	FOR_EACH_UNIT(pLoopUnit, *this)
	{
		if (pLoopUnit->isGoldenAge())
			iCount++;
	}
	return iCount;
}

/*	Rewritten for K-Mod. (The only functionality difference is that
	unit class is now used rather unit type. But this version is far more efficient.) */
int CvPlayer::unitsGoldenAgeReady() const
{
	PROFILE_FUNC();

	std::set<UnitClassTypes> golden_age_units;
	FOR_EACH_UNIT(pLoopUnit, *this)
	{
		if (pLoopUnit->isGoldenAge() &&
			golden_age_units.count(pLoopUnit->getUnitClassType()) == 0)
		{
			golden_age_units.insert(pLoopUnit->getUnitClassType());
		}
	}
	return golden_age_units.size();
}


void CvPlayer::killGoldenAgeUnits(CvUnit* pUnitAlive)
{
	EnumMap<UnitTypes,bool> abUnitUsed; // advc.enum
	int iUnitsRequired = unitsRequiredForGoldenAge();
	if (pUnitAlive != NULL)
	{
		abUnitUsed.set(pUnitAlive->getUnitType(), true);
		iUnitsRequired--;
	}

	for (int i = 0; i < iUnitsRequired; i++)
	{
		int iBestValue = 0;
		CvUnit* pBestUnit = NULL;
		FOR_EACH_UNIT_VAR(pLoopUnit, *this)
		{
			if (!pLoopUnit->isGoldenAge())
				continue;
			if (!abUnitUsed.get(pLoopUnit->getUnitType()))
			{
				int iValue = 10000;
				iValue /= plotDistance(pLoopUnit->plot(), pUnitAlive->plot()) + 1;
				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					pBestUnit = pLoopUnit;
				}
			}
		}

		FAssert(pBestUnit != NULL);
		if (pBestUnit != NULL)
		{
			abUnitUsed.set(pBestUnit->getUnitType(), true);
			pBestUnit->kill(true);
			//play animations
			if (pBestUnit->getPlot().isActiveVisible(false))
			{
				//kill removes bestUnit from any groups
				pBestUnit->getGroup()->pushMission(MISSION_GOLDEN_AGE, 0);
			}
		}
	}
}


int CvPlayer::greatPeopleThreshold(bool bMilitary) const
{
	int iThreshold=-1;
	if (bMilitary)
	{
		static int const iGREAT_GENERALS_THRESHOLD = GC.getDefineINT("GREAT_GENERALS_THRESHOLD"); // advc.opt
		iThreshold = ((iGREAT_GENERALS_THRESHOLD *
				std::max(0, getGreatGeneralsThresholdModifier() + 100)) / 100);
	}
	else
	{
		static int const iGREAT_PEOPLE_THRESHOLD = GC.getDefineINT("GREAT_PEOPLE_THRESHOLD"); // advc.opt
		iThreshold = ((iGREAT_PEOPLE_THRESHOLD *
				std::max(0, getGreatPeopleThresholdModifier() + 100)) / 100);
	}
	CvGame const& kGame = GC.getGame(); // advc
	iThreshold *= GC.getInfo(kGame.getGameSpeedType()).getGreatPeoplePercent();
	if (bMilitary)
		iThreshold /= std::max(1, GC.getInfo(kGame.getGameSpeedType()).getTrainPercent());
	else iThreshold /= 100;

	iThreshold *= GC.getInfo(kGame.getStartEra()).getGreatPeoplePercent();
	iThreshold /= 100;
	// <advc.251>
	iThreshold = (per100(1) * iThreshold * GC.getInfo(
			getHandicapType()).getGPThresholdPercent()).roundToMultiple(
			isHuman() ? 5 : 1);
	if (!isHuman() && !isBarbarian())
	{
		iThreshold = intdiv::round(iThreshold * GC.getInfo(
				kGame.getHandicapType()).getAIGPThresholdPercent(), 100);
	} // </advc.251>

	return std::max(1, iThreshold);
}


int CvPlayer::specialistYield(SpecialistTypes eSpecialist, YieldTypes eYield) const
{
	return GC.getInfo(eSpecialist).getYieldChange(eYield) +
			getSpecialistExtraYield(eSpecialist, eYield);
}


int CvPlayer::specialistCommerce(SpecialistTypes eSpecialist, CommerceTypes eCommerce) const
{
	return GC.getInfo(eSpecialist).getCommerceChange(eCommerce) +
			getSpecialistExtraCommerce(eCommerce);
}


void CvPlayer::setStartingPlot(CvPlot* pNewValue, bool bUpdateStartDist)
{
	CvPlot* pOldStartingPlot = getStartingPlot();
	if (pOldStartingPlot == pNewValue)
		return;

	if (pOldStartingPlot != NULL)
	{
		pOldStartingPlot->getArea().changeNumStartingPlots(-1);
		if (bUpdateStartDist)
			GC.getMap().updateMinOriginalStartDist(pOldStartingPlot->getArea());
	}

	if (pNewValue == NULL)
		m_pStartingPlot = NULL;
	else
	{
		// <advc.031c>
		if (gFoundLogLevel > 0 && !GC.getInitCore().isScenario() &&
			m_pStartingPlot == NULL)
		{
			AI().logFoundValue(*pNewValue, true);
		} // </advc.031c>
		m_pStartingPlot = pNewValue;

		getStartingPlot()->getArea().changeNumStartingPlots(1);
		if (bUpdateStartDist)
			GC.getMap().updateMinOriginalStartDist(getStartingPlot()->getArea());
	}
	FAssert(pNewValue == NULL || !pNewValue->isWater()); // advc.021b
}


int CvPlayer::getAveragePopulation() const
{
	if (getNumCities() == 0)
		return 0;
	//return ((getTotalPopulation() / getNumCities()) + 1);
	// advc.131: The above is 100% off at the start of a game
	return scaled(getTotalPopulation(), getNumCities()).round();
}


void CvPlayer::changeTotalPopulation(int iChange)
{
	// <advc>
	if (iChange == 0)
		return; // <advc>
	CvGame const& kGame = GC.getGame();
	changeAssets(-kGame.getPopulationAsset(getTotalPopulation()));
	changePower(-kGame.getPopulationPower(getTotalPopulation()));
	changePopScore(-kGame.getPopulationScore(getTotalPopulation()));

	m_iTotalPopulation += iChange;
	FAssert(getTotalPopulation() >= 0);

	changeAssets(kGame.getPopulationAsset(getTotalPopulation()));
	changePower(kGame.getPopulationPower(getTotalPopulation()));
	changePopScore(kGame.getPopulationScore(getTotalPopulation()));
}

// advc: Return type was long. Not helpful since sizeof(int)==sizeof(long).
int CvPlayer::getRealPopulation() const
{
	long long iTotalPopulation = 0;
	FOR_EACH_CITY(pLoopCity, *this)
		iTotalPopulation += pLoopCity->getRealPopulation();
	return ::longLongToInt(iTotalPopulation);
}


void CvPlayer::changeTotalLand(int iChange)
{
	m_iTotalLand += iChange;
	FAssert(getTotalLand() >= 0);
}


void CvPlayer::changeTotalLandScored(int iChange)
{
	if (iChange == 0)
		return; // advc

	CvGame const& kGame = GC.getGame();
	changeAssets(-kGame.getLandPlotsAsset(getTotalLandScored()));
	changeLandScore(-kGame.getLandPlotsScore(getTotalLandScored()));

	m_iTotalLandScored += iChange;
	FAssert(getTotalLandScored() >= 0);

	changeAssets(kGame.getLandPlotsAsset(getTotalLandScored()));
	changeLandScore(kGame.getLandPlotsScore(getTotalLandScored()));
}


void CvPlayer::setGold(int iNewValue)
{
	if (getGold() != iNewValue)
	{
		m_iGold = iNewValue;
		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->UI().setDirty(MiscButtons_DIRTY_BIT, true);
			gDLL->UI().setDirty(SelectionButtons_DIRTY_BIT, true);
			gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
		}
	}
}


void CvPlayer::changeGold(int iChange)
{
	setGold(getGold() + iChange);
}


void CvPlayer::setAdvancedStartPoints(int iNewValue)
{
	if (getAdvancedStartPoints() != iNewValue)
	{
		m_iAdvancedStartPoints = iNewValue;
		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->UI().setDirty(MiscButtons_DIRTY_BIT, true);
			gDLL->UI().setDirty(SelectionButtons_DIRTY_BIT, true);
			gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
		}
	}
}

void CvPlayer::changeAdvancedStartPoints(int iChange)
{
	setAdvancedStartPoints(getAdvancedStartPoints() + iChange);
}


void CvPlayer::changeGoldenAgeTurns(int iChange)
{
	if(iChange == 0)
		return;

	CvWString szBuffer;

	bool const bOldGoldenAge = isGoldenAge();
	m_iGoldenAgeTurns += iChange;
	FAssert(getGoldenAgeTurns() >= 0);

	if (bOldGoldenAge != isGoldenAge())
	{
		if (isGoldenAge())
		{
			changeAnarchyTurns(-getAnarchyTurns());
			// K-Mod. Allow the AI to reconsider their civics. (a golden age is a good time for reform!)
			if (!isHuman() && getMaxAnarchyTurns() != 0 && getAnarchyModifier() + 100 > 0)
				AI().AI_setCivicTimer(0); // K-Mod end
		}

		updateYield();

		if (isGoldenAge())
		{
			szBuffer = gDLL->getText("TXT_KEY_MISC_PLAYER_GOLDEN_AGE_BEGINS", getNameKey());
			GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szBuffer,
					-1, -1, GC.getColorType("HIGHLIGHT_TEXT"));

			CvEventReporter::getInstance().goldenAge(getID());
		}
		else CvEventReporter::getInstance().endGoldenAge(getID());

		for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
		{
			CvPlayer const& kObs = *it;
			if (GET_TEAM(getTeam()).isHasMet(kObs.getTeam()) ||
				kObs.isSpectator()) // advc.127
			{
				if (isGoldenAge())
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_PLAYER_GOLDEN_AGE_HAS_BEGUN", getNameKey());
					gDLL->UI().addMessage(kObs.getID(), kObs.getID() == getID(), -1,
							szBuffer, "AS2D_GOLDAGESTART",
							MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY, // advc.106b
							NULL, GC.getColorType("HIGHLIGHT_TEXT"),
							// advc.127b:
							getCapitalX(kObs.getTeam(), true), getCapitalY(kObs.getTeam(), true));
				}
				else
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_PLAYER_GOLDEN_AGE_ENDED", getNameKey());
					gDLL->UI().addMessage(kObs.getID(), false, -1, szBuffer, "AS2D_GOLDAGEEND",
							MESSAGE_TYPE_MINOR_EVENT, NULL, GC.getColorType("HIGHLIGHT_TEXT"),
							// advc.127b:
							getCapitalX(kObs.getTeam(), true), getCapitalY(kObs.getTeam(), true));
				}
			}
		}
	}

	if (getID() == GC.getGame().getActivePlayer())
		gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
}
// advc.001x:
void CvPlayer::startGoldenAgeDelayed()
{
	m_iScheduledGoldenAges++;
}

int CvPlayer::getGoldenAgeLength() const
{
	return (GC.getGame().goldenAgeLength() *
			std::max(0, 100 + getGoldenAgeModifier())) / 100;
}


void CvPlayer::changeNumUnitGoldenAges(int iChange)
{
	m_iNumUnitGoldenAges += iChange;
	FAssert(getNumUnitGoldenAges() >= 0);
}


void CvPlayer::changeStrikeTurns(int iChange)
{
	m_iStrikeTurns += iChange;
	FAssert(getStrikeTurns() >= 0);
}


void CvPlayer::changeAnarchyTurns(int iChange) // advc: Refactored
{
	if(iChange == 0)
		return;

	CvGame const& kGame = GC.getGame();
	if(getID() == kGame.getActivePlayer())
		gDLL->UI().setDirty(GameData_DIRTY_BIT, true);

	bool bOldAnarchy = isAnarchy();
	m_iAnarchyTurns += iChange;
	FAssert(getAnarchyTurns() >= 0);
	if(bOldAnarchy == isAnarchy())
		return;

	if(getID() == kGame.getActivePlayer())
	{
		gDLL->UI().setDirty(MiscButtons_DIRTY_BIT, true);
		// advc.004x:
		gDLL->UI().setDirty(ResearchButtons_DIRTY_BIT, true);
	}
	if(getTeam() == kGame.getActiveTeam())
		gDLL->UI().setDirty(CityInfo_DIRTY_BIT, true);

	updateCommerce();
	updateMaintenance();
	updateTradeRoutes();
	updateCorporation();
	GC.getGame().updateTradeRoutes(); // advc.124
	AI_makeAssignWorkDirty();

	if (isAnarchy())
	{
		gDLL->UI().addMessage(getID(), true, -1,
				gDLL->getText("TXT_KEY_MISC_REVOLUTION_HAS_BEGUN").GetCString(), "AS2D_REVOLTSTART",
				MESSAGE_TYPE_MINOR_EVENT, // advc.106b: was MAJOR
				NULL, GC.getColorType("WARNING_TEXT"),
				getCapitalX(getTeam()), getCapitalY(getTeam())); // advc.127b
	}
	else
	{
		gDLL->UI().addMessage(getID(), false, -1,
				gDLL->getText("TXT_KEY_MISC_REVOLUTION_OVER").GetCString(), "AS2D_REVOLTEND",
				MESSAGE_TYPE_MINOR_EVENT, NULL, NO_COLOR, // advc.004g: Was COLOR_WARNING_TEXT
				getCapitalX(getTeam()), getCapitalY(getTeam())); // advc.127b
		// K-Mod. trigger production/research popups that have been suppressed.
		if (isHuman())
		{
			if (isResearch() && getCurrentResearch() == NO_TECH)
				chooseTech();
			FOR_EACH_CITYAI_VAR(pLoopCity, AI())
			{
				if (pLoopCity->isChooseProductionDirty() &&
					!pLoopCity->isProduction() && !pLoopCity->isDisorder() &&
					!pLoopCity->isProductionAutomated())
				{
					pLoopCity->chooseProduction();
				}
			}
		} // K-Mod end
	}
}


void CvPlayer::updateMaxAnarchyTurns()
{
	int iBestValue = GC.getDefineINT("MAX_ANARCHY_TURNS");
	FOR_EACH_ENUM2(Trait, eTrait)
	{
		if (hasTrait(eTrait) && GC.getInfo(eTrait).getMaxAnarchy() >= 0)
			iBestValue = std::min(iBestValue, GC.getInfo(eTrait).getMaxAnarchy());
	}
	m_iMaxAnarchyTurns = iBestValue;
	FAssert(getMaxAnarchyTurns() >= 0);
}


void CvPlayer::changeAnarchyModifier(int iChange)
{
	if (iChange == 0)
		return;

	/*setRevolutionTimer(std::max(0, ((100 + iChange) * getRevolutionTimer()) / 100));
	setConversionTimer(std::max(0, ((100 + iChange) * getConversionTimer()) / 100));*/ // BtS
	// K-Mod. The original code is wrong, and it is missing the anarchy length change.
	changeRevolutionTimer(getRevolutionTimer() * iChange /
			std::max(1, 100 + getAnarchyModifier()));
	changeConversionTimer(getConversionTimer() * iChange /
			std::max(1, 100 + getAnarchyModifier()));
	changeAnarchyTurns(getAnarchyTurns() * iChange /
			std::max(1, 100 + getAnarchyModifier()));
	// K-Mod end

	m_iAnarchyModifier += iChange;
}


void CvPlayer::changeGoldenAgeModifier(int iChange)
{	// <advc>
	if (iChange == 0)
		return; // </advc>
	// <K-Mod> If we are currently in a golden age, adjust its duration!
	changeGoldenAgeTurns(getGoldenAgeTurns() * iChange /
			std::max(1, 100 + getGoldenAgeModifier())); // </K-Mod>
	m_iGoldenAgeModifier += iChange;
}


void CvPlayer::changeHurryModifier(int iChange)
{
	m_iGlobalHurryModifier += iChange;
}


void CvPlayer::incrementGreatPeopleCreated()
{
	m_iGreatPeopleCreated++;
}


void CvPlayer::incrementGreatGeneralsCreated()
{
	m_iGreatGeneralsCreated++;
}


void CvPlayer::changeGreatPeopleThresholdModifier(int iChange)
{
	m_iGreatPeopleThresholdModifier += iChange;
}


void CvPlayer::changeGreatGeneralsThresholdModifier(int iChange)
{
	m_iGreatGeneralsThresholdModifier += iChange;
}


void CvPlayer::changeGreatPeopleRateModifier(int iChange)
{
	m_iGreatPeopleRateModifier += iChange;
}


void CvPlayer::changeGreatGeneralRateModifier(int iChange)
{
	m_iGreatGeneralRateModifier += iChange;
}


int CvPlayer::getDomesticGreatGeneralRateModifier() const
{
	static int const iCOMBAT_EXPERIENCE_IN_BORDERS_PERCENT = GC.getDefineINT("COMBAT_EXPERIENCE_IN_BORDERS_PERCENT"); // advc.opt
	return (iCOMBAT_EXPERIENCE_IN_BORDERS_PERCENT + m_iDomesticGreatGeneralRateModifier);
}


void CvPlayer::changeDomesticGreatGeneralRateModifier(int iChange)
{
	m_iDomesticGreatGeneralRateModifier += iChange;
}


void CvPlayer::changeStateReligionGreatPeopleRateModifier(int iChange)
{
	m_iStateReligionGreatPeopleRateModifier += iChange;
}


void CvPlayer::changeMaxGlobalBuildingProductionModifier(int iChange)
{
	m_iMaxGlobalBuildingProductionModifier += iChange;
}


void CvPlayer::changeMaxTeamBuildingProductionModifier(int iChange)
{
	m_iMaxTeamBuildingProductionModifier += iChange;
}


void CvPlayer::changeMaxPlayerBuildingProductionModifier(int iChange)
{
	m_iMaxPlayerBuildingProductionModifier += iChange;
}


void CvPlayer::changeFreeExperience(int iChange)
{
	m_iFreeExperience += iChange;
}


void CvPlayer::changeFeatureProductionModifier(int iChange)
{
	m_iFeatureProductionModifier += iChange;
}


void CvPlayer::changeWorkerSpeedModifier(int iChange)
{
	m_iWorkerSpeedModifier += iChange;
}

// advc.011c:
int CvPlayer::getWorkRate(BuildTypes eBuild) const
{
	int iRate = 0;
	CvCivilization const& kCiv = getCivilization(); // advc.003w
	for (int i = 0; i < kCiv.getNumUnits(); i++)
	{
		UnitTypes eUnit = kCiv.unitAt(i);
		CvUnitInfo& kUnit = GC.getInfo(eUnit);
		if (kUnit.getBuilds(eBuild))
		{
			iRate = kUnit.getWorkRate();
			break;
		}
	}
	iRate *= std::max(0, 100 + getWorkerSpeedModifier());
	iRate /= 100;
	if(!isHuman() && !isBarbarian())
	{
		iRate *= std::max(0, 100 + GC.getInfo(GC.getGame().getHandicapType()).
				getAIWorkRateModifier() + 100);
		iRate /= 100;
	}
	return iRate;
}


void CvPlayer::changeImprovementUpgradeRateModifier(int iChange)
{
	m_iImprovementUpgradeRateModifier += iChange;
}


void CvPlayer::changeMilitaryProductionModifier(int iChange)
{
	m_iMilitaryProductionModifier += iChange;
}


void CvPlayer::changeSpaceProductionModifier(int iChange)
{
	m_iSpaceProductionModifier += iChange;
}


void CvPlayer::changeCityDefenseModifier(int iChange)
{
	m_iCityDefenseModifier += iChange;
}


void CvPlayer::changeNumNukeUnits(int iChange)
{
	m_iNumNukeUnits += iChange;
	FAssert(getNumNukeUnits() >= 0);
}


void CvPlayer::changeNumOutsideUnits(int iChange)
{
	if (iChange != 0)
	{
		m_iNumOutsideUnits += iChange;
		FAssert(getNumOutsideUnits() >= 0);
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
	}
}


void CvPlayer::changeBaseFreeUnits(int iChange)
{
	if (iChange != 0)
	{
		m_iBaseFreeUnits += iChange;
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
	}
}


void CvPlayer::changeBaseFreeMilitaryUnits(int iChange)
{
	if (iChange != 0)
	{
		m_iBaseFreeMilitaryUnits += iChange;
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
	}
}


void CvPlayer::changeFreeUnitsPopulationPercent(int iChange)
{
	if (iChange != 0)
	{
		m_iFreeUnitsPopulationPercent += iChange;
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
	}
}


void CvPlayer::changeFreeMilitaryUnitsPopulationPercent(int iChange)
{
	if (iChange != 0)
	{
		m_iFreeMilitaryUnitsPopulationPercent += iChange;
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
	}
}

// K-Mod:
int CvPlayer::getTypicalUnitValue(UnitAITypes eUnitAI, DomainTypes eDomain) const
{
	//UnitTypes eBestUnit = NO_UNIT;
	int iHighestValue = 0;
	CvCivilization const& kCiv = getCivilization(); // advc.003w
	for (int i = 0; i < kCiv.getNumUnits(); i++)
	{
		UnitTypes eLoopUnit = kCiv.unitAt(i);
		if ((eUnitAI == NO_UNITAI || GC.getInfo(eLoopUnit).getUnitAIType(eUnitAI)) &&
			(eDomain == NO_DOMAIN || GC.getInfo(eLoopUnit).getDomainType() == eDomain) &&
			canTrain(eLoopUnit))
		{
			// Note: currently the above checks do not consider any resource prerequites.
			int iValue = GC.AI_getGame().AI_combatValue(eLoopUnit);
			if (iValue > iHighestValue)
			{
				iHighestValue = iValue;
				//eBestUnit = eLoopUnit;
			}
		}
	}

	return iHighestValue;
}


void CvPlayer::changeGoldPerUnit(int iChange)
{
	if (iChange != 0)
	{
		m_iGoldPerUnit += iChange;
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
	}
}


void CvPlayer::changeGoldPerMilitaryUnit(int iChange)
{
	if (iChange != 0)
	{
		m_iGoldPerMilitaryUnit += iChange;
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
	}
}


void CvPlayer::changeExtraUnitCost(int iChange)
{
	if (iChange != 0)
	{
		m_iExtraUnitCost += iChange;
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
	}
}


void CvPlayer::changeNumMilitaryUnits(int iChange)
{
	if (iChange != 0)
	{
		m_iNumMilitaryUnits += iChange;
		FAssert(getNumMilitaryUnits() >= 0);
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
	}
}


void CvPlayer::changeHappyPerMilitaryUnit(int iChange)
{
	if (iChange != 0)
	{
		m_iHappyPerMilitaryUnit += iChange;
		AI_makeAssignWorkDirty();
	}
}

// advc.912c:
void CvPlayer::changeLuxuryModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iLuxuryModifier += iChange;
		m_iLuxuryModifier = ::range(m_iLuxuryModifier, -100, 1000);
		AI_makeAssignWorkDirty();
	}
}


void CvPlayer::changeMilitaryFoodProductionCount(int iChange)
{
	if (iChange != 0)
	{
		m_iMilitaryFoodProductionCount += iChange;
		FAssert(getMilitaryFoodProductionCount() >= 0);
		if (getTeam() == GC.getGame().getActiveTeam())
			gDLL->UI().setDirty(CityInfo_DIRTY_BIT, true);
	}
}


void CvPlayer::setHighestUnitLevel(int iNewValue)
{
	m_iHighestUnitLevel = iNewValue;
	FAssert(getHighestUnitLevel() >= 0);
}


void CvPlayer::changeMaxConscript(int iChange)
{
	m_iMaxConscript += iChange;
	FAssert(getMaxConscript() >= 0);
}


void CvPlayer::setConscriptCount(int iNewValue)
{
	m_iConscriptCount = iNewValue;
	FAssert(getConscriptCount() >= 0);
}


void CvPlayer::setOverflowResearch(int iNewValue)
{
	m_iOverflowResearch = iNewValue;
	FAssert(getOverflowResearch() >= 0);
}


/*int CvPlayer::getNoUnhealthyPopulationCount() const {
	return m_iNoUnhealthyPopulationCount;
}
bool CvPlayer::isNoUnhealthyPopulation() const {
	return (getNoUnhealthyPopulationCount() > 0);
}
void CvPlayer::changeNoUnhealthyPopulationCount(int iChange) {
	if (iChange != 0) {
		m_iNoUnhealthyPopulationCount = (m_iNoUnhealthyPopulationCount + iChange);
		FAssert(getNoUnhealthyPopulationCount() >= 0);
		AI_makeAssignWorkDirty();
	}
}*/ // BtS
// K-Mod, 27/dec/10: replace with UnhealthyPopulationModifier
void CvPlayer::changeUnhealthyPopulationModifier(int iChange)
{
	m_iUnhealthyPopulationModifier += iChange;
}


void CvPlayer::changeExpInBorderModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iExpInBorderModifier += iChange;
		FAssert(getExpInBorderModifier() >= 0);
	}
}


void CvPlayer::changeBuildingOnlyHealthyCount(int iChange)
{
	if (iChange != 0)
	{
		m_iBuildingOnlyHealthyCount += iChange;
		FAssert(getBuildingOnlyHealthyCount() >= 0);
		AI_makeAssignWorkDirty();
	}
}


void CvPlayer::changeDistanceMaintenanceModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iDistanceMaintenanceModifier += iChange;
		updateMaintenance();
	}
}


void CvPlayer::changeNumCitiesMaintenanceModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iNumCitiesMaintenanceModifier += iChange;
		updateMaintenance();
	}
}


void CvPlayer::changeCorporationMaintenanceModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iCorporationMaintenanceModifier += iChange;
		updateMaintenance();
	}
}


void CvPlayer::changeTotalMaintenance(int iChange)
{
	m_iTotalMaintenance += iChange;
	FAssert(m_iTotalMaintenance >= 0);
}


void CvPlayer::changeUpkeepModifier(int iChange)
{
	m_iUpkeepModifier += iChange;
}


void CvPlayer::changeLevelExperienceModifier(int iChange)
{
	m_iLevelExperienceModifier += iChange;
}


void CvPlayer::changeExtraHealth(int iChange)
{
	if (iChange != 0)
	{
		m_iExtraHealth += iChange;
		AI_makeAssignWorkDirty();
	}
}


void CvPlayer::changeBuildingGoodHealth(int iChange)
{
	if (iChange != 0)
	{
		m_iBuildingGoodHealth += iChange;
		FAssert(getBuildingGoodHealth() >= 0);
		AI_makeAssignWorkDirty();
	}
}


void CvPlayer::changeBuildingBadHealth(int iChange)
{
	if (iChange != 0)
	{
		m_iBuildingBadHealth += iChange;
		FAssert(getBuildingBadHealth() <= 0);
		AI_makeAssignWorkDirty();
	}
}


void CvPlayer::changeExtraHappiness(int iChange)
{
	if (iChange != 0)
	{
		m_iExtraHappiness += iChange;
		AI_makeAssignWorkDirty();
	}
}


void CvPlayer::changeBuildingHappiness(int iChange)
{
	if (iChange != 0)
	{
		m_iBuildingHappiness += iChange;
		AI_makeAssignWorkDirty();
	}
}


void CvPlayer::changeLargestCityHappiness(int iChange)
{
	if (iChange != 0)
	{
		m_iLargestCityHappiness += iChange;
		AI_makeAssignWorkDirty();
	}
}


void CvPlayer::updateWarWearinessPercentAnger()
{
	int iNewWarWearinessPercentAnger = 0;
	if (!isBarbarian() && !isMinorCiv())
	{
		for (TeamIter<MAJOR_CIV,ENEMY_OF> itEnemy(getTeam());
			itEnemy.hasNext(); ++itEnemy)
		{
			iNewWarWearinessPercentAnger += GET_TEAM(getTeam()).getWarWeariness(itEnemy->getID(),
					//) * std::max(0, 100 + kTeam.getEnemyWarWearinessModifier())) / 10000;
					true) / 100; // K-Mod
		}
	}
	iNewWarWearinessPercentAnger = getModifiedWarWearinessPercentAnger(iNewWarWearinessPercentAnger);
	if (getWarWearinessPercentAnger() != iNewWarWearinessPercentAnger)
	{
		m_iWarWearinessPercentAnger = iNewWarWearinessPercentAnger;
		AI_makeAssignWorkDirty();
	}
}

int CvPlayer::getModifiedWarWearinessPercentAnger(int iWarWearinessPercentAnger) const
{
	static int const iBASE_WAR_WEARINESS_MULTIPLIER = GC.getDefineINT("BASE_WAR_WEARINESS_MULTIPLIER"); // advc.opt
	iWarWearinessPercentAnger *= iBASE_WAR_WEARINESS_MULTIPLIER;

	if (GC.getGame().isOption(GAMEOPTION_ALWAYS_WAR) ||
		GC.getGame().isOption(GAMEOPTION_NO_CHANGING_WAR_PEACE))
	{
		static int const iFORCED_WAR_WAR_WEARINESS_MODIFIER = GC.getDefineINT("FORCED_WAR_WAR_WEARINESS_MODIFIER"); // advc.opt
		iWarWearinessPercentAnger *= std::max(0, iFORCED_WAR_WAR_WEARINESS_MODIFIER + 100);
		iWarWearinessPercentAnger /= 100;
	}

	if (GC.getGame().isGameMultiPlayer())
	{
		static int const iMULTIPLAYER_WAR_WEARINESS_MODIFIER = GC.getDefineINT("MULTIPLAYER_WAR_WEARINESS_MODIFIER"); // advc.opt
		iWarWearinessPercentAnger *= std::max(0, iMULTIPLAYER_WAR_WEARINESS_MODIFIER + 100);
		iWarWearinessPercentAnger /= 100;
	}

	iWarWearinessPercentAnger *= std::max(0,
			GC.getInfo(GC.getMap().getWorldSize()).getWarWearinessModifier() + 100);
	iWarWearinessPercentAnger /= 100;

	if (!isHuman() && !isBarbarian() && !isMinorCiv())
	{
		iWarWearinessPercentAnger *= GC.getInfo(GC.getGame().getHandicapType()).
				getAIWarWearinessPercent();
		iWarWearinessPercentAnger /= 100;
		// advc.251: No longer adjusted to handicap
		/*iWarWearinessPercentAnger *= std::max(0, ((GC.getInfo(GC.getGame().getHandicapType()).getAIPerEraModifier() * getCurrentEra()) + 100));
		iWarWearinessPercentAnger /= 100;*/
	}

	return iWarWearinessPercentAnger;
}


void CvPlayer::changeWarWearinessModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iWarWearinessModifier += iChange;
		AI_makeAssignWorkDirty();
	}
}


void CvPlayer::changeFreeSpecialist(int iChange)
{
	if (iChange != 0)
	{
		m_iFreeSpecialist += iChange;
		FAssert(getFreeSpecialist() >= 0);
		AI_makeAssignWorkDirty();
	}
}


void CvPlayer::changeNoForeignTradeCount(int iChange)
{
	if (iChange == 0)
		return;
	m_iNoForeignTradeCount += iChange;
	FAssert(getNoForeignTradeCount() >= 0);
	if(!isAnarchy()) // advc.124: Update them when anarchy ends
		GC.getGame().updateTradeRoutes();
}


void CvPlayer::changeNoCorporationsCount(int iChange)
{
	if (iChange != 0)
	{
		m_iNoCorporationsCount += iChange;
		FAssert(getNoCorporationsCount() >= 0);
		updateCorporation();
	}
}


void CvPlayer::changeNoForeignCorporationsCount(int iChange)
{
	if (iChange != 0)
	{
		m_iNoForeignCorporationsCount += iChange;
		FAssert(getNoForeignCorporationsCount() >= 0);
		updateCorporation();
	}
}


void CvPlayer::changeCoastalTradeRoutes(int iChange)
{
	if (iChange != 0)
	{
		m_iCoastalTradeRoutes += iChange;
		FAssert(getCoastalTradeRoutes() >= 0);
		updateTradeRoutes();
	}
}


void CvPlayer::changeTradeRoutes(int iChange)
{
	if (iChange != 0)
	{
		m_iTradeRoutes += iChange;
		FAssert(getTradeRoutes() >= 0);
		updateTradeRoutes();
	}
}


void CvPlayer::setRevolutionTimer(int iNewValue)
{
	if (getRevolutionTimer() != iNewValue)
	{
		m_iRevolutionTimer = iNewValue;
		FAssert(getRevolutionTimer() >= 0);
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(MiscButtons_DIRTY_BIT, true);
	}
}


void CvPlayer::setConversionTimer(int iNewValue)
{
	if (getConversionTimer() != iNewValue)
	{
		m_iConversionTimer = iNewValue;
		FAssert(getConversionTimer() >= 0);
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(MiscButtons_DIRTY_BIT, true);
	}
}


void CvPlayer::changeStateReligionCount(int iChange)
{
	if (iChange == 0)
		return;
	// religion visibility now part of espionage
	//GC.getGame().updateCitySight(false, true);

	m_iStateReligionCount += iChange;
	FAssert(getStateReligionCount() >= 0);

	// religion visibility now part of espionage
	//GC.getGame().updateCitySight(true, true);

	updateMaintenance();
	updateReligionHappiness();
	updateReligionCommerce();

	GC.getGame().AI_makeAssignWorkDirty();
	gDLL->UI().setDirty(Score_DIRTY_BIT, true);
}


void CvPlayer::changeNoNonStateReligionSpreadCount(int iChange)
{
	m_iNoNonStateReligionSpreadCount += iChange;
	FAssert(getNoNonStateReligionSpreadCount() >= 0);
}


void CvPlayer::changeStateReligionHappiness(int iChange)
{
	if (iChange != 0)
	{
		m_iStateReligionHappiness += iChange;
		updateReligionHappiness();
	}
}


void CvPlayer::changeNonStateReligionHappiness(int iChange)
{
	if (iChange != 0)
	{
		m_iNonStateReligionHappiness += iChange;
		updateReligionHappiness();
	}
}


void CvPlayer::changeStateReligionUnitProductionModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iStateReligionUnitProductionModifier += iChange;
		if (getTeam() == GC.getGame().getActiveTeam())
			gDLL->UI().setDirty(CityInfo_DIRTY_BIT, true);
	}
}


void CvPlayer::changeStateReligionBuildingProductionModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iStateReligionBuildingProductionModifier += iChange;
		if (getTeam() == GC.getGame().getActiveTeam())
			gDLL->UI().setDirty(CityInfo_DIRTY_BIT, true);
	}
}


void CvPlayer::changeStateReligionFreeExperience(int iChange)
{
	m_iStateReligionFreeExperience += iChange;
}

// advc: Renamed from "setCapitalCity"
void CvPlayer::setCapital(CvCity* pNewCapital)
{
	CvCity* const pOldCapital = getCapital();
	if (pOldCapital == pNewCapital)
	{	// <advc> Make sure these are consistent
		if (pNewCapital == NULL)
			m_iCapitalCityID = FFreeList::INVALID_INDEX; // </advc>
		return;
	}
	bool bUpdatePlotGroups = (pOldCapital == NULL || pNewCapital == NULL ||
			pOldCapital->getPlot().getOwnerPlotGroup() !=
			pNewCapital->getPlot().getOwnerPlotGroup());
	if (bUpdatePlotGroups)
	{
		if (pOldCapital != NULL)
			pOldCapital->getPlot().updatePlotGroupBonus(false, /* advc.064d: */ false);
		if (pNewCapital != NULL)
			pNewCapital->getPlot().updatePlotGroupBonus(false);
	}

	if (pNewCapital != NULL)
		m_iCapitalCityID = pNewCapital->getID();
	else m_iCapitalCityID = FFreeList::INVALID_INDEX;

	if (bUpdatePlotGroups)
	{
		if (pOldCapital != NULL)
			pOldCapital->getPlot().updatePlotGroupBonus(true, /* advc.064d: */ false);
		if (pNewCapital != NULL)
			pNewCapital->getPlot().updatePlotGroupBonus(true);
	}

	updateMaintenance();
	updateTradeRoutes();

	if (pOldCapital != NULL)
	{
		pOldCapital->updateCommerce();
		pOldCapital->setInfoDirty(true);
	}
	if (pNewCapital != NULL)
	{
		pNewCapital->updateCommerce();
		pNewCapital->setInfoDirty(true);
	}
	// <advc.106> Announcement:
	if (pOldCapital != NULL && pNewCapital != NULL)
	{
		CvWString szFullInfo = gDLL->getText("TXT_KEY_MISC_CAPITAL_MOVED",
				pNewCapital->getNameKey(), getCivilizationShortDescriptionKey(),
				pOldCapital->getNameKey());
		CvWString szOnlyOldKnown = gDLL->getText("TXT_KEY_MISC_NO_LONGER_CAPITAL",
				getCivilizationShortDescriptionKey(), pOldCapital->getNameKey());
		CvWString szOnlyNewKnown = gDLL->getText("TXT_KEY_MISC_IS_NOW_CAPITAL",
				pNewCapital->getNameKey(), getCivilizationShortDescriptionKey());
		for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
		{
			CvPlayer const& kObs = *it;
			if (kObs.getID() == getID())
				continue;
			if (GET_TEAM(getTeam()).isHasMet(kObs.getTeam()) ||
				 kObs.isSpectator()) // advc.127
			{
				CvWString* pszMsg = NULL;
				bool bOldRevealed = pOldCapital->isRevealed(kObs.getTeam(),
						kObs.isSpectator()); // advc.127
				bool bNewRevealed = pNewCapital->isRevealed(kObs.getTeam(),
						kObs.isSpectator()); // advc.127
				if (bOldRevealed && bNewRevealed)
					pszMsg = &szFullInfo;
				else if (bOldRevealed)
					pszMsg = &szOnlyOldKnown;
				else if (bNewRevealed)
					pszMsg = &szOnlyNewKnown;
				if (pszMsg != NULL)
				{
					CvPlot const& kFlashPlot = (bNewRevealed ?
							pNewCapital->getPlot() : pOldCapital->getPlot());
					/*	<advc.127> Not really a major event, but minor events
						don't get shown in spectator mode. */
					InterfaceMessageTypes eMsgType = (kObs.isSpectator() ?
							MESSAGE_TYPE_MAJOR_EVENT : MESSAGE_TYPE_MINOR_EVENT);
					// </advc.127>
					gDLL->UI().addMessage(kObs.getID(), false, -1, *pszMsg, NULL,
							eMsgType, ARTFILEMGR.getInterfaceArtInfo(
							"INTERFACE_CITY_BAR_CAPITAL_TEXTURE")->getPath(),
							NO_COLOR, kFlashPlot.getX(), kFlashPlot.getY());
				}
			}
		}
	} // </advc.106>
}

// <advc.127b>
int CvPlayer::getCapitalX(TeamTypes eObserver, bool bDebug) const
{
	CvCity* pCapital = getCapital();
	if (pCapital == NULL ||
		(eObserver != NO_TEAM && !pCapital->isRevealed(eObserver, bDebug)))
	{
		return FFreeList::INVALID_INDEX;
	}
	return pCapital->getX();
}

int CvPlayer::getCapitalY(TeamTypes eObserver, bool bDebug) const
{
	CvCity* pCapital = getCapital();
	if (pCapital == NULL ||
		(eObserver != NO_TEAM && !pCapital->isRevealed(eObserver, bDebug)))
	{
		return FFreeList::INVALID_INDEX;
	}
	return pCapital->getY();
}

int CvPlayer::getCapitalX(PlayerTypes eObserver, bool bDebug) const
{
	return getCapitalX(eObserver == NO_PLAYER ? NO_TEAM : TEAMID(eObserver), bDebug);
}
int CvPlayer::getCapitalY(PlayerTypes eObserver, bool bDebug) const
{
	return getCapitalY(eObserver == NO_PLAYER ? NO_TEAM : TEAMID(eObserver), bDebug);
} // </advc.127b>


void CvPlayer::changeCitiesLost(int iChange)
{
	m_iCitiesLost = (m_iCitiesLost + iChange);
}

// advc:
int CvPlayer::getFreeWinsVsBarbs() const
{
	if (GC.getGame().isOption(GAMEOPTION_SPAH) || // advc.250b
		GC.getGame().isOption(GAMEOPTION_RISE_FALL)) // advc.700
	{
		return 0;
	}
	// Cut from CvUnit::getDefenderCombatValues
	return GC.getInfo(getHandicapType()).getFreeWinsVsBarbs();
}


int CvPlayer::getWinsVsBarbs() const
{
	return m_iWinsVsBarbs;
}


void CvPlayer::changeWinsVsBarbs(int iChange)
{
	m_iWinsVsBarbs += iChange;
	FAssert(getWinsVsBarbs() >= 0);
}


void CvPlayer::changeAssets(int iChange)
{
	m_iAssets += iChange;
	FAssert(getAssets() >= 0);
}


void CvPlayer::changePower(int iChange)
{
	m_iPower += iChange;
	FAssert(getPower() >= 0);
}


int CvPlayer::getPopScore(bool bIncludeVassals) const
{
	if (bIncludeVassals && isAVassal())
		return m_iPopulationScore / 2;

	int iVassalScore = 0;
	if (bIncludeVassals)
	{
		for (PlayerIter<ALIVE,VASSAL_OF> itVassal(getTeam());
			itVassal.hasNext(); ++itVassal)
		{
			iVassalScore += itVassal->getPopScore(false) / 2;
		}
	}
	return m_iPopulationScore +
			iVassalScore / std::max(1, GET_TEAM(getTeam()).getNumMembers());
}

void CvPlayer::changePopScore(int iChange)
{
	if (iChange != 0)
	{
		m_iPopulationScore += iChange;
		FAssert(getPopScore() >= 0);
		GC.getGame().setScoreDirty(true);
	}
}


int CvPlayer::getLandScore(bool bIncludeVassals) const
{
	if (bIncludeVassals && isAVassal())
		return m_iLandScore / 2;

	int iVassalScore = 0;
	if (bIncludeVassals)
	{
		for (PlayerIter<ALIVE,VASSAL_OF> itVassal(getTeam());
			itVassal.hasNext(); ++itVassal)
		{
			iVassalScore += itVassal->getLandScore(false) / 2;
		}
	}
	return m_iLandScore +
			iVassalScore / std::max(1, GET_TEAM(getTeam()).getNumMembers());
}


void CvPlayer::changeLandScore(int iChange)
{
	if (iChange != 0)
	{
		m_iLandScore += iChange;
		FAssert(getLandScore() >= 0);
		GC.getGame().setScoreDirty(true);
	}
}


void CvPlayer::changeTechScore(int iChange)
{
	if (iChange != 0)
	{
		m_iTechScore += iChange;
		FAssert(getTechScore() >= 0);
		GC.getGame().setScoreDirty(true);
	}
}


void CvPlayer::changeWondersScore(int iChange)
{
	if (iChange != 0)
	{
		m_iWondersScore += iChange;
		FAssert(getWondersScore() >= 0);
		GC.getGame().setScoreDirty(true);
	}
}


void CvPlayer::setCombatExperience(int iExperience)
{
	FAssert(iExperience >= 0);
	// <advc>
	if(iExperience == getCombatExperience())
		return;
	m_iCombatExperience = iExperience;
	if(isBarbarian())
		return;
	CvGame& kGame = GC.getGame();
	// </advc>

	int iExperienceThreshold = greatPeopleThreshold(true);
	if (m_iCombatExperience >= iExperienceThreshold && iExperienceThreshold > 0)
	{
		// create great person
		CvCity const* pBestCity = NULL;
		int iBestValue = MAX_INT;
		FOR_EACH_CITY(pLoopCity, *this)
		{
			int iValue = 4 * kGame.getSorenRandNum(getNumCities(),
					"Warlord City Selection");
			FOR_EACH_ENUM(Yield)
				iValue += pLoopCity->findYieldRateRank(eLoopYield);
			iValue += pLoopCity->findPopulationRank();
			if (iValue < iBestValue)
			{
				pBestCity = pLoopCity;
				iBestValue = iValue;
			}
		}

		if (pBestCity != NULL)
		{
			int iRandOffset = kGame.getSorenRandNum(GC.getNumUnitInfos(),
					"Warlord Unit Generation");
			FOR_EACH_ENUM(Unit)
			{
				UnitTypes eRandUnit = (UnitTypes)
						((eLoopUnit + iRandOffset) % GC.getNumUnitInfos());
				if (GC.getInfo(eRandUnit).getLeaderExperience() > 0 ||
					GC.getInfo(eRandUnit).getLeaderPromotion() != NO_PROMOTION)
				{
					pBestCity->createGreatPeople(eRandUnit, false, true);
					setCombatExperience(getCombatExperience() - iExperienceThreshold);
					break;
				}
			}
		}
	} // <advc.078>
	if (getID() == kGame.getActivePlayer() &&
		BUGOption::isEnabled("MainInterface__Combat_Counter", false))
	{
		gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
	} // </advc.078>
}


bool CvPlayer::isConnected() const
{
	return gDLL->isConnected(getNetID());
}


int CvPlayer::getNetID() const
{
	return GC.getInitCore().getNetID(getID());
}


void CvPlayer::setNetID(int iNetID)
{
	GC.getInitCore().setNetID(getID(), iNetID);
}


uint CvPlayer::getStartTime() const
{
	return m_uiStartTime;
}


void CvPlayer::setStartTime(uint uiStartTime)
{
	m_uiStartTime = uiStartTime;
}


uint CvPlayer::getTotalTimePlayed() const
{
	return (timeGetTime() - m_uiStartTime) / 1000;
}


void CvPlayer::setAlive(bool bNewValue)
{
	if(isAlive() == bNewValue)
		return;
	/*	<advc.003m> Moved up b/c, once the team's AliveCount is set to 0,
		at-war status is lost. Need that for lifting blockades.
		Not sure about killing cities (there should be none anyway).
		killAllDeals relies on bAlive=false.  */
	if (!bNewValue)
	{
		setCapital(NULL);
		killUnits(); // </advc.003m>
	}
	bool const bEverAlive = isEverAlive();
	m_bAlive = bNewValue;
	CvGame& kGame = GC.getGame();
	// <advc.opt>
	if (isAlive() && !bEverAlive && getParent() == NO_PLAYER && !isBarbarian())
		kGame.changeCivPlayersEverAlive(1);
	GET_TEAM(getTeam()).updateLeaderID(); // </advc.opt>
	GET_TEAM(getTeam()).changeAliveCount(isAlive() ? 1 : -1);
	// <advc.agent>
	if (!isAlive())
		GC.getAgents().playerDefeated(getID());
	else if (bEverAlive && getParent() == NO_PLAYER) // Colonies are handled by initInGame
	{
		GC.getAgents().playerRevived(getID());
		// <advc.104r> (UWAI data gets deleted upon death)
		if(getUWAI.isEnabled())
			getUWAI.processNewCivInGame(getID()); // </advc.104r>
	}
	// </advc.agent>
	// Report event to Python
	CvEventReporter::getInstance().setPlayerAlive(getID(), bNewValue);

	if (isAlive())
	{
		if (!isEverAlive())
		{
			m_bEverAlive = true;
			GET_TEAM(getTeam()).changeEverAliveCount(1);
		}
		if (getNumCities() <= 0)
			setFoundedFirstCity(false);
		updatePlotGroups();
		if (kGame.isMPOption(MPOPTION_SIMULTANEOUS_TURNS) ||
			kGame.getNumGameTurnActive() == 0 ||
			(kGame.isSimultaneousTeamTurns() && GET_TEAM(getTeam()).isTurnActive()))
		{
			setTurnActive(true);
		}
		gDLL->openSlot(getID());
		if(!isBarbarian()) // advc.003n
		{	// K-Mod. Attitude cache
			for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
			{
				/*GET_PLAYER(i).AI_updateAttitude(getID());
				AI().AI_updateAttitude(i);*/
				/*  advc.001: E.g. AI_getRankDifferenceAttitude can change between
					any two civs. */
				if (GET_PLAYER(i).isAlive() && /* advc.003n: */ !GET_PLAYER(i).isMinorCiv())
					GET_PLAYER(i).AI_updateAttitude();
			} // K-Mod end
		}
	}
	else
	{
		// <advc.001> CvTeam::makePeace does this, but here they missed it.
		for(int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			CvPlayer& kWarEnemy = GET_PLAYER((PlayerTypes)i);
			if(kWarEnemy.isAlive() && kWarEnemy.getID() != getID() &&
				!kWarEnemy.isMinorCiv() && GET_TEAM(kWarEnemy.getTeam()).isAtWar(getTeam()))
			{
				kWarEnemy.updateWarWearinessPercentAnger();
			}
		} // </advc.001>
		clearResearchQueue();
		clearPopups(); // advc
		//killUnits(); // advc.003m: Moved up
		killCities();
		killAllDeals();

		setTurnActive(false);

		gDLL->endMPDiplomacy();
		gDLL->endDiplomacy();

		if (!isHuman())
			gDLL->closeSlot(getID());

		if (kGame.getElapsedGameTurns() > 0)
		{
			if (!isBarbarian())
			{
				CvWString szBuffer
					//= gDLL->getText("TXT_KEY_MISC_CIV_DESTROYED", getCivilizationAdjectiveKey());
					// advc.099: Replacing the above
					= gDLL->getText("TXT_KEY_PLAYER_ELIMINATED", getNameKey());
				for (int iI = 0; iI < MAX_PLAYERS; iI++)
				{
					if (GET_PLAYER((PlayerTypes)iI).isAlive())
					{
						gDLL->UI().addMessage((PlayerTypes)iI, false, -1, szBuffer,
								"AS2D_CIVDESTROYED", MESSAGE_TYPE_MAJOR_EVENT, NULL,
								GC.getColorType("WARNING_TEXT"));
					}
				}
				kGame.addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szBuffer, -1, -1,
						GC.getColorType("WARNING_TEXT"));
				// <advc.104>
				if ((getUWAI.isEnabled() || getUWAI.isEnabled(true)) && !isMinorCiv())
					AI().uwai().uninit(); // </advc.104>
			}
		}
	}

	kGame.setScoreDirty(true);
	// <advc.700>
	if(kGame.isOption(GAMEOPTION_RISE_FALL))
		kGame.getRiseFall().reportElimination(getID()); // </advc.700>
}


void CvPlayer::verifyAlive()
{
	CvGame const& kGame = GC.getGame();
	if (!isAlive())
	{
		if (getNumCities() > 0 || getNumUnits() > 0)
			setAlive(true);
		return; // advc
	}
	bool bKill = false; // advc: Removed superfluous code
	if (!isBarbarian())
	{
		if (getNumCities() == 0 && getAdvancedStartPoints() < 0)
		{
			//if (getNumUnits() == 0 || (!kGame.isOption(GAMEOPTION_COMPLETE_KILLS) && isFoundedFirstCity()))
			// advc.701: COMPLETE_KILLS option removed (replacing the line above)
			if (getNumUnits() <= 0 || isFoundedFirstCity())
				bKill = true;
		}
	}
	if (!bKill && !isBarbarian() && kGame.getMaxCityElimination() > 0 &&
		getCitiesLost() >= kGame.getMaxCityElimination())
	{
		bKill = true;
	}
	if (bKill)
	{	// <advc.127> Defeat of active player during AI Auto Play
		if (isHumanDisabled() && kGame.getActivePlayer() == getID() &&
			!kGame.isOption(GAMEOPTION_RISE_FALL)) // advc.701
		{
			// Automatically change active player
			PlayerTypes eNextAlive = getID();
			for (int iPass = 0; iPass < 2; iPass++)
			{
				bool bAllowMinor = (iPass == 1);
				do // Akin to RiseFall::nextCivAlive
				{
					eNextAlive = (PlayerTypes)((eNextAlive + 1) % MAX_CIV_PLAYERS);
				} while(eNextAlive != getID() &&
					(!GET_PLAYER(eNextAlive).isAlive() || GET_PLAYER(eNextAlive).isHuman() ||
					GET_PLAYER(eNextAlive).isHumanDisabled() ||
					(!bAllowMinor && GET_PLAYER(eNextAlive).isMinorCiv())));
				if (eNextAlive != getID())
					break;
			}
			if (eNextAlive != getID())
			{
				setIsHuman(false);
				setAlive(false);
				GC.getGame().changeHumanPlayer(eNextAlive);
				GET_PLAYER(eNextAlive).setHumanDisabled(true);
				return;
			}
			else
			{
				FAssertMsg(kGame.isGameMultiPlayer(), "No other civ alive left?");
				// (Let the original AI Auto Play code handle it)
			}
		} // </advc.127>
		setAlive(false);
	}
}


void CvPlayer::setTurnActiveForPbem(bool bActive)
{
	FAssertMsg(GC.getGame().isPbem(), "You are using setTurnActiveForPbem. Are you sure you know what you're doing?");

	// does nothing more than to set the member variable before saving the game
	// the rest of the turn will be performed upon loading the game
	// This allows the player to browse the game in paused mode after he has generated the save
	if (isTurnActive() != bActive)
	{
		m_bTurnActive = bActive;
		GC.getGame().changeNumGameTurnActive(isTurnActive() ? 1 : -1);
		// BETTER_BTS_AI_MOD, Efficiency (plot danger cache), 08/21/09, jdog5000: START
		//if (GC.getGame().getNumGameTurnActive() != 1)  // advc (comment): This BBAI line had already been commented out in BBAI
		GC.getMap().invalidateActivePlayerSafeRangeCache();
		// BETTER_BTS_AI_MOD: END
	}
}

// This structure of this function has been changed for K-Mod.
void CvPlayer::setTurnActive(bool bNewValue, bool bDoTurn)
{
	if (isTurnActive() == bNewValue)
		return;

	m_bTurnActive = bNewValue;
	CvGame& kGame = GC.getGame();
	if (isTurnActive())
	{
		FAssert(isAlive());
		// <advc.001x> (or move this to the very end of doTurn?)
		changeGoldenAgeTurns(getGoldenAgeLength() * m_iScheduledGoldenAges);
		m_iScheduledGoldenAges = 0;
		// </advc.001x>
		// K-Mod
		AI().AI_updateCacheData();
		onTurnLogging(); // bbai logging
		// K-Mod end
		GC.getLogger().logTurnActive(getID()); // advc.003t

		setEndTurn(false);
		kGame.resetTurnTimer();

		if (gDLL->IsPitbossHost()) // If we are the Pitboss, send this player an email
		{	// If this guy is not currently connected, try sending him an email
			if (isHuman() && !isConnected())
				GC.getPythonCaller()->sendEmailReminder(getPbemEmailAddress());
		}

		if ((kGame.isHotSeat() || kGame.isPbem()) && isHuman() && bDoTurn)
		{
			gDLL->UI().clearEventMessages();
			// advc.135a: Commented out
			//gDLL->getEngineIFace()->setResourceLayer(false);
			kGame.setActivePlayer(getID());
		}

		kGame.changeNumGameTurnActive(1);

		if (bDoTurn)
		{
			if (isAlive() && !isHuman() && !isBarbarian() && getAdvancedStartPoints() >= 0)
				AI().AI_doAdvancedStart();

			if (kGame.getElapsedGameTurns() > 0 && isAlive())
			{
					if (kGame.isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
						doTurn();
				/*	K-Mod. Call CvTeam::doTurn at the start of this team's turn.
					ie. when the leader's turn is activated.
					Note: in simultaneous turns mode this is called by CvGame::doTurn,
					because from here we can't tell which player in each team
					will be activated first. */
				else
				{
					if (GET_TEAM(getTeam()).getLeaderID() == getID())
						GET_TEAM(getTeam()).doTurn();
				} // K-Mod end

				doTurnUnits();
			}

			if (getID() == kGame.getActivePlayer() && kGame.getElapsedGameTurns() > 0)
			{
				if (kGame.isNetworkMultiPlayer())
				{
					gDLL->UI().addMessage(getID(), true, -1,
							gDLL->getText("TXT_KEY_MISC_TURN_BEGINS").GetCString(),
							"AS2D_NEWTURN", MESSAGE_TYPE_DISPLAY_ONLY);
				}
				else gDLL->UI().playGeneralSound("AS2D_NEWTURN");
			}
			// <advc.706> Skip warnings and messages if only pausing for civ selection
			if (!kGame.isOption(GAMEOPTION_RISE_FALL) ||
				!kGame.getRiseFall().isSelectingCiv()) // </advc.706>
			{
				doWarnings();
				// <advc.106b>
				if (isHuman())
				{
					validateDiplomacy(); // advc.001e
					if(kGame.getActivePlayer() == getID())
					{
						/*  Make sure that Python events like Civ4lerts are
							triggered before processing messages */
						CyArgsList pyArgs;
						pyArgs.add(kGame.getTurnSlice());
						CvEventReporter::getInstance().genericEvent("gameUpdate", pyArgs.makeFunctionArgs());
						postProcessMessages();
					}
				} // </advc.106b>
			}
			// <advc.044>
			if ((isHuman() || isHumanDisabled()) &&
				!kGame.isMPOption(MPOPTION_SIMULTANEOUS_TURNS) && !kGame.isHotSeat())
			{
				// <advc.700>
				if(kGame.isOption(GAMEOPTION_RISE_FALL))
					kGame.getRiseFall().atActiveTurnStart();
				// </advc.700>
				else kGame.autoSave(); // advc.106l
			} // </advc.044>
			// <advc.106b> Clear messages in any case (in particular during AIAutoPlay)
			for (size_t i = 0; i < m_aMajorMsgs.size(); i++)
				SAFE_DELETE(m_aMajorMsgs[i]);
			m_aMajorMsgs.clear(); // </106b>
		}

		if (getID() == kGame.getActivePlayer())
		{
			if (gDLL->UI().getLengthSelectionList() == 0)
				kGame.cycleSelectionGroups_delayed(1, false);
			gDLL->UI().setDirty(SelectionCamera_DIRTY_BIT, true);
		}
	}
	else
	{
		GC.getLogger().logTurnActive(getID()); // advc.003t

		if (getID() == kGame.getActivePlayer())
		{
			gDLL->UI().setForcePopup(false);
			gDLL->UI().clearQueuedPopups();
			gDLL->UI().flushTalkingHeadMessages();
		}

		if (getID() == kGame.getActivePlayer())
			startProfilingDLL(true); // start profiling DLL if desired

		kGame.changeNumGameTurnActive(-1);

		if (bDoTurn)
		{
			if (!kGame.isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
			{
				if (isAlive())
					doTurn();

				if ((kGame.isPbem() || kGame.isHotSeat()) && isHuman() &&
					kGame.countHumanPlayersAlive() > 1)
				{
					kGame.setHotPbemBetweenTurns(true);
				}

				if (kGame.isSimultaneousTeamTurns())
				{
					if (!GET_TEAM(getTeam()).isTurnActive())
					{
						for (int iI = (getTeam() + 1); iI < MAX_TEAMS; iI++)
						{
							if (GET_TEAM((TeamTypes)iI).isAlive())
							{
								GET_TEAM((TeamTypes)iI).setTurnActive(true);
								break;
							}
						}
					}
				}
				else
				{
					for (int iI = getID() + 1; iI < MAX_PLAYERS; iI++)
					{
						if (GET_PLAYER((PlayerTypes)iI).isAlive())
						{
							if (kGame.isPbem() && GET_PLAYER((PlayerTypes)iI).isHuman())
							{
								if (!kGame.getPbemTurnSent())
									gDLL->sendPbemTurn((PlayerTypes)iI);
							}
							else GET_PLAYER((PlayerTypes)iI).setTurnActive(true);
							break;
						}
					}
				}
			}
		}
	}
	gDLL->UI().updateCursorType();
	gDLL->UI().setDirty(Score_DIRTY_BIT, true);
	GC.getMap().invalidateActivePlayerSafeRangeCache();
}

/*	K-Mod. The body of this function use to be part of setTurnActive.
	I've moved it here just to improve the readability of that function.
	(Based on BETTER_BTS_AI_MOD, 10/26/09, jdog5000 - AI logging.) */
void CvPlayer::onTurnLogging() const
{
	if (gPlayerLogLevel > 0)
	{
		logBBAI("Player %d (%S) setTurnActive for turn %d (%d %s)", getID(), getCivilizationDescription(0), GC.getGame().getGameTurn(), std::abs(GC.getGame().getGameTurnYear()), GC.getGame().getGameTurnYear()>0 ? "AD" : "BC");

		if (GC.getGame().getGameTurn() > 0 &&
			(GC.getGame().getGameTurn() % 25) == 0 && !isBarbarian())
		{
			CvWStringBuffer szBuffer;
			GAMETEXT.setScoreHelp(szBuffer, getID());
			logBBAI("%S", szBuffer);

			int iGameTurn = GC.getGame().getGameTurn();
			logBBAI("  Total Score: %d, Population Score: %d (%d total pop), Land Score: %d, Tech Score: %d, Wonder Score: %d", calculateScore(), getPopScore(false), getTotalPopulation(), getLandScore(false), getTechScore(), getWondersScore());

			int iEconomy = 0;
			int iProduction = 0;
			int iAgri = 0;
			int iCount = 0;
			for (int iI = 1; iI <= 5; iI++)
			{
				if (iGameTurn - iI >= 0)
				{
					iEconomy += getHistory(PLAYER_HISTORY_ECONOMY, iGameTurn - iI);
					iProduction += getHistory(PLAYER_HISTORY_INDUSTRY, iGameTurn - iI);
					iAgri += getHistory(PLAYER_HISTORY_AGRICULTURE, iGameTurn - iI);
					iCount++;
				}
			}
			iEconomy /= std::max(1, iCount);
			iProduction /= std::max(1, iCount);
			iAgri /= std::max(1, iCount);

			logBBAI("  Economy avg: %d,  Industry avg: %d,  Agriculture avg: %d", iEconomy, iProduction, iAgri);
		}
	}

	if (gPlayerLogLevel >= 2)
	{
		CvWStringBuffer szBuffer;

		logBBAI("    Player %d (%S) has %d cities, %d pop, %d power, %d tech percent", getID(), getCivilizationDescription(0), getNumCities(), getTotalPopulation(), getPower(), GET_TEAM(getTeam()).getBestKnownTechScorePercent());
		if (AI().AI_isFinancialTrouble())
			logBBAI("    Financial trouble!");

		szBuffer.append(CvWString::format(L"    Team %d has met: ", getTeam()));

		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			if (iI != getTeam() && GET_TEAM(getTeam()).isHasMet((TeamTypes)iI))
			{
				if (GET_TEAM((TeamTypes)iI).isAlive())
					szBuffer.append(CvWString::format(L"%d,", iI));
			}
		}

		if (GET_TEAM(getTeam()).getVassalCount() > 0)
		{
			szBuffer.append(CvWString::format(L";  vassals: "));

			for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
			{
				if (iI != getTeam() && GET_TEAM((TeamTypes)iI).isVassal(getTeam()))
				{
					if (GET_TEAM((TeamTypes)iI).isAlive())
						szBuffer.append(CvWString::format(L"%d,", iI));
				}
			}
		}

		if (GET_TEAM(getTeam()).getNumWars(false) > 0)
		{
			szBuffer.append(CvWString::format(L";  at war with: "));

			for(int iI = 0; iI < MAX_CIV_TEAMS; iI++)
			{
				if(iI != getTeam() && GET_TEAM(getTeam()).isAtWar((TeamTypes)iI))
				{
					if (GET_TEAM((TeamTypes)iI).isAlive())
						szBuffer.append(CvWString::format(L"%d,", iI));
				}
			}
		}

		if (GET_TEAM(getTeam()).AI_isAnyWarPlan())
		{
			szBuffer.append(CvWString::format(L";  planning war with: "));
			for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
			{
				if (iI != getTeam() && !GET_TEAM(getTeam()).isAtWar((TeamTypes)iI) && GET_TEAM(getTeam()).AI_getWarPlan((TeamTypes)iI) != NO_WARPLAN)
				{
					if (GET_TEAM((TeamTypes)iI).isAlive())
						szBuffer.append(CvWString::format(L"%d,", iI));
				}
			}
		}

		logBBAI("%S", szBuffer.getCString());
		szBuffer.clear();
		if (GET_TEAM(getTeam()).AI_isAnyWarPlan()) logBBAI("    Enemy power perc: %d (%d with others reduction)", GET_TEAM(getTeam()).AI_getEnemyPowerPercent(), GET_TEAM(getTeam()).AI_getEnemyPowerPercent(true));
	}
}


void CvPlayer::setAutoMoves(bool bNewValue)
{
	if (isAutoMoves() == bNewValue)
		return; // advc

	m_bAutoMoves = bNewValue;
	if (!isAutoMoves())
	{
		if (isEndTurn() || !isHuman())
			setTurnActive(false);
		else
		{
			if (getID() == GC.getGame().getActivePlayer())
			{
				gDLL->UI().setCycleSelectionCounter(1);
				// this is a subtle case. I think it's best to just use the normal delay
				//GC.getGame().cycleSelectionGroups_delayed(1, false, true);
			}
		}
	}
}


void CvPlayer::setEndTurn(bool bNewValue)
{
	if (isEndTurn() != bNewValue)
	{
		FAssertMsg(isTurnActive(), "isTurnActive is expected to be true");
		m_bEndTurn = bNewValue;
		if (isEndTurn())
			setAutoMoves(true);
	}
}

bool CvPlayer::isTurnDone() const
{
	// if this returns true, popups and diplomacy will wait to appear until next turn
	if (!GC.getGame().isPbem() && !GC.getGame().isHotSeat())
		return false;
	if (!isHuman())
		return true;
	if (!isEndTurn())
		return false;
	return !isAutoMoves();
}


void CvPlayer::makeExtendedGame()
{
	m_bExtendedGame = true;
}


void CvPlayer::setFoundedFirstCity(bool bNewValue)
{
	if (isFoundedFirstCity() == bNewValue)
		return;

	m_bFoundedFirstCity = bNewValue;
	if (getID() == GC.getGame().getActivePlayer())
	{
		gDLL->UI().setDirty(PercentButtons_DIRTY_BIT, true);
		gDLL->UI().setDirty(ResearchButtons_DIRTY_BIT, true);
	}
	/*  <advc.104> So that rivals (with higher ids) can immediately evaluate
		war plans against this player  */
	if(!isBarbarian() && getParent() == NO_PLAYER && getUWAI.isEnabled() &&
		GC.getGame().isFinalInitialized()) // No update while setting up a scenario
	{
		AI().uwai().getCache().update();
	} // </advc.104>
}

// <advc.078>
bool CvPlayer::isAnyGPPEver() const
{
	return m_bAnyGPPEver;
}

void CvPlayer::reportFirstGPP()
{
	m_bAnyGPPEver = true;
} // </advc.078>


void CvPlayer::setStrike(bool bNewValue)
{
	if (isStrike() != bNewValue)
	{
		m_bStrike = bNewValue;
		if (isStrike())
		{
			if (getID() == GC.getGame().getActivePlayer())
			{
				gDLL->UI().addMessage(getID(), false, -1,
						gDLL->getText("TXT_KEY_MISC_UNITS_ON_STRIKE").GetCString(),
						"AS2D_STRIKE", MESSAGE_TYPE_MINOR_EVENT, NULL,
						GC.getColorType("WARNING_TEXT"));
				gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
			}
		}
	}
}


HandicapTypes CvPlayer::getHandicapType() const
{
	// <advc.127>
	if (isHumanDisabled() && // <advc.706>
		// With R&F, Ctrl+Shift+X still leads to Auto Play with AI handicap.
		(!GC.getGame().isOption(GAMEOPTION_RISE_FALL) ||
		!GC.getGame().getRiseFall().hasRetired())) // </advc.706>
	{
		return GC.getGame().getAIHandicap();
	} // </advc.127>
	// advc (note, little known fact): The EXE sets the Barbarian handicap to Warchief
	return GC.getInitCore().getHandicap(getID());
}

// advc.003w:
void CvPlayer::setCivilization(CivilizationTypes eCivilization)
{
	SAFE_DELETE(m_pCivilization);
	if (eCivilization != NO_CIVILIZATION)
		m_pCivilization = new CvCivilization(GC.getInfo(eCivilization));
}


void CvPlayer::setPersonalityType(LeaderHeadTypes eNewValue)
{
	m_ePersonalityType = eNewValue;
	AI().AI_updatePersonality(); // advc.104
}


void CvPlayer::setCurrentEra(EraTypes eNewValue)
{
	if (getCurrentEra() == eNewValue)
		return; // advc

	EraTypes eOldEra = m_eCurrentEra;
	m_eCurrentEra = eNewValue;

	if (GC.getGame().getActiveTeam() != NO_TEAM)
	{
		CvMap const& kMap = GC.getMap();
		for (int i = 0; i < kMap.numPlots(); i++)
		{
			CvPlot& kPlot = kMap.getPlotByIndex(i);
			kPlot.updateGraphicEra();
			if (kPlot.getRevealedImprovementType(
				GC.getGame().getActiveTeam(), true) != NO_IMPROVEMENT)
			{
				if (kPlot.getOwner() == getID() || (!kPlot.isOwned() &&
					getID() == GC.getGame().getActivePlayer()))
				{
					kPlot.setLayoutDirty(true);
				}
			}
		}
	}

	// dirty all of this player's cities...
	FOR_EACH_CITY_VAR(pLoopCity, *this)
	{
		//if (pLoopCity->getOwner() == getID())
		FAssert(pLoopCity->getOwner() == getID()); // K-Mod
		pLoopCity->setLayoutDirty(true);
	}

	//update unit eras
	FOR_EACH_UNIT_VAR(pLoopUnit, *this)
		gDLL->getEntityIFace()->updateGraphicEra(pLoopUnit->getEntity(), eOldEra);

	//update main interface flag
	gDLL->UI().setDirty(Flag_DIRTY_BIT, true);

	if (getID() == GC.getGame().getActivePlayer())
	{
		gDLL->UI().setDirty(Soundtrack_DIRTY_BIT, true);
		// advc.004m: Default cam distance can depend on era
		GC.getPythonCaller()->callScreenFunction("updateCameraStartDistance");
	}

	if (isHuman() && getCurrentEra() != GC.getGame().getStartEra() &&
		!GC.getGame().isNetworkMultiPlayer())
	{
		if (GC.getGame().isFinalInitialized() && !gDLL->GetWorldBuilderMode())
		{
			CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN);
			if (pInfo != NULL)
			{
				pInfo->setData1(eNewValue);
				pInfo->setText(L"showEraMovie");
				addPopup(pInfo);
			}
		}
	} // <advc.106>
	if (GC.getDefineBOOL("SHOW_ENTERED_ERA_IN_REPLAY"))
	{
		CvWString szBuffer = gDLL->getText("TXT_KEY_SOMEONE_ENTERED_ERA",
				getNameKey(), GC.getInfo(eNewValue).getTextKeyWide());
		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szBuffer,
				-1, -1, GC.getColorType("ALT_HIGHLIGHT_TEXT"));
	} // </advc.106>
	// <advc.106n> Save pre-Industrial minimap terrain for replay
	if (GC.getGame().isFinalInitialized() &&
		getCurrentEra() >= GC.getDefineINT("REPLAY_TEXTURE_ERA"))
	{
		CvMap& kMap = GC.getMap();
		if (kMap.getReplayTexture() == NULL)
			kMap.updateReplayTexture();
	} // </advc.106n>
}


void CvPlayer::setLastStateReligion(ReligionTypes eNewValue)
{
	if(getLastStateReligion() == eNewValue)
		return;
	FAssert(!isBarbarian()); // advc.003n
	// religion visibility now part of espionage
	//GC.getGame().updateCitySight(false, true);

	ReligionTypes const eOldReligion = getLastStateReligion();
	m_eLastStateReligion = eNewValue;

	// religion visibility now part of espionage
	//GC.getGame().updateCitySight(true, true);

	updateMaintenance();
	updateReligionHappiness();
	updateReligionCommerce();

	GC.getGame().updateSecretaryGeneral();

	GC.getGame().AI_makeAssignWorkDirty();
	gDLL->UI().setDirty(Score_DIRTY_BIT, true);

	if (!GC.getGame().isFinalInitialized())
		return; // advc
	if (gDLL->isDiplomacy() && (gDLL->getDiplomacyPlayer() == getID()))
	{
		gDLL->updateDiplomacyAttitude(true);
	}

	if (!isBarbarian() &&
		/*  advc.150a: Message superfluous when already reported switch to
			civic that prohibits state religion. */
		isStateReligion())
	{
		CvWString szBuffer;
		// <advc.150a> Sufficient to set szBuffer once (always the same message)
		if(eNewValue == NO_RELIGION)
			szBuffer = gDLL->getText("TXT_KEY_MISC_PLAYER_RENOUNCE_RELIGION",
					getNameKey(), GC.getInfo(eOldReligion).getTextKeyWide());
		else
		{
			szBuffer = gDLL->getText("TXT_KEY_MISC_PLAYER_CONVERT_RELIGION",
					getNameKey(), GC.getInfo(eNewValue).getTextKeyWide());
			// <advc.151>
			if(eOldReligion != NO_RELIGION)
			{
				szBuffer += L" " + gDLL->getText("TXT_KEY_MISC_AND_RENOUNCE_RELIGION",
						GC.getInfo(eOldReligion).getTextKeyWide());
			} // </advc.151>
		} // </advc.150a>
		for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
		{
			CvPlayer const& kObs = *it;
			if (GET_TEAM(getTeam()).isHasMet(kObs.getTeam()) ||
				 kObs.isSpectator()) // advc.127
			{
				gDLL->UI().addMessage(kObs.getID(), false, -1, szBuffer,
						"AS2D_RELIGION_CONVERT", MESSAGE_TYPE_MAJOR_EVENT,
						// <advc.127b>
						NULL, NO_COLOR, getCapitalX(kObs.getTeam(), true),
						getCapitalY(kObs.getTeam(), true)); // </advc.127b>
			}
		}
		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szBuffer);
	}
	// advc.064d: Production of religious buildings and units may no longer be valid
	verifyCityProduction();

	// Python Event
	CvEventReporter::getInstance().playerChangeStateReligion(getID(), eNewValue, eOldReligion);
	if (isMajorCiv()) // advc.003n
	{	// <K-Mod>
		for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it) // advc.003n
		{
			CvPlayerAI& kLoopPlayer = *it;
			if (kLoopPlayer.getStateReligion() != NO_RELIGION)
			{
				AI().AI_updateAttitude(kLoopPlayer.getID());
				kLoopPlayer.AI_updateAttitude(getID());
			}
		} // </K-Mod end>
	}
}


void CvPlayer::setParent(PlayerTypes eParent)
{
	if(m_eParent == eParent)
		return; // advc.opt
	m_eParent = eParent;
	// <advc.opt>
	FAssert(isAlive());
	/*  Wouldn't be too difficult to support NO_PLAYER, but it's not currently used -
		once a parent, ever a parent. */
	FAssert(eParent != NO_PLAYER);
	if(!isAlive() || eParent == NO_PLAYER)
		return;
	GC.getGame().changeCivPlayersEverAlive(-1);
	/*  Might later break free and find a permanent ally, but, upon creation,
		the colonial vassal is going to be in a singleton team. */
	FAssert(GET_TEAM(getTeam()).getNumMembers() == 1);
	GC.getGame().changeCivTeamsEverAlive(-1);
	// </advc.opt>
}

// <advc> More convenient access to CvTeam::getMasterTeam
TeamTypes CvPlayer::getMasterTeam() const
{
	return GET_TEAM(getTeam()).getMasterTeam();
}


bool CvPlayer::isAVassal() const
{
	return GET_TEAM(getTeam()).isAVassal();
} // </advc>


void CvPlayer::updateTeamType()
{
	if(getID() == NO_PLAYER)
		m_eTeamType = NO_TEAM;
	else m_eTeamType = GC.getInitCore().getTeam(getID());
}


void CvPlayer::setTeam(TeamTypes eTeam)
{
	FAssert(eTeam != NO_TEAM);
	FAssert(getTeam() != NO_TEAM);
	TeamTypes const eOldTeam = getTeam(); // advc.opt
	GET_TEAM(getTeam()).changeNumMembers(-1);
	if (isAlive())
		GET_TEAM(getTeam()).changeAliveCount(-1);
	if (isEverAlive())
		GET_TEAM(getTeam()).changeEverAliveCount(-1);
	GET_TEAM(getTeam()).changeNumCities(-(getNumCities()));
	GET_TEAM(getTeam()).changeTotalPopulation(-(getTotalPopulation()));
	GET_TEAM(getTeam()).changeTotalLand(-(getTotalLand()));

	GC.getInitCore().setTeam(getID(), eTeam);

	GET_TEAM(getTeam()).changeNumMembers(1);
	if (isAlive())
		GET_TEAM(getTeam()).changeAliveCount(1);
	if (isEverAlive())
		GET_TEAM(getTeam()).changeEverAliveCount(1);
	// <advc.opt>
	if(getTeam() != eOldTeam)
	{
		GET_TEAM(getTeam()).updateLeaderID();
		GET_TEAM(eOldTeam).updateLeaderID();
	} // </advc.opt>
	GET_TEAM(getTeam()).changeNumCities(getNumCities());
	GET_TEAM(getTeam()).changeTotalPopulation(getTotalPopulation());
	GET_TEAM(getTeam()).changeTotalLand(getTotalLand());
	// K-Mod Attitude cache
	if (GC.getGame().isFinalInitialized() /* advc.003n: */ && !isBarbarian())
	{
		for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
		{
			AI().AI_updateAttitude(i);
			GET_PLAYER(i).AI_updateAttitude(getID());
		}
	} // K-Mod end
}

/*	advc.058: New function; see getKnownCivDescription. (But this one here
	is actually used.) */
PlayerColorTypes CvPlayer::getKnownPlayerColor(TeamTypes eObserver) const
{
	// advc.058: Moved from getPlayerColor
	// K-Mod. Conceal the player colour of unmet players.
	if (concealUnknownCivs() &&
		!GET_TEAM(eObserver == NO_TEAM ? GC.getGame().getActiveTeam() : eObserver).
		isHasSeen(getTeam()))
	{
		return GC.getInitCore().getColor(BARBARIAN_PLAYER);
	} // K-Mod end
	return getPlayerColor();
}

// advc.058: Better conceal colors when the EXE asks (as K-Mod did too)
PlayerColorTypes CvPlayer::getPlayerColorExternal() const
{
	return getKnownPlayerColor();
}


int CvPlayer::getPlayerTextColorR() const
{	// advc: Round to nearest; also in the other getPlayerTextColor functions.
	return ::round(GC.getInfo(GC.getInfo(getPlayerColor()).getTextColorType()).
			getColor().r * 255);
}


int CvPlayer::getPlayerTextColorG() const
{
	return ::round(GC.getInfo(GC.getInfo(getPlayerColor()).getTextColorType()).
			getColor().g * 255);
}


int CvPlayer::getPlayerTextColorB() const
{
	return ::round(GC.getInfo(GC.getInfo(getPlayerColor()).getTextColorType()).
			getColor().b * 255);
}


int CvPlayer::getPlayerTextColorA() const
{
	return ::round(GC.getInfo(GC.getInfo(getPlayerColor()).getTextColorType()).
			getColor().a * 255);
}

// advc.106:
ColorTypes CvPlayer::getPlayerTextColor() const
{
	return GC.getInfo(getPlayerColor()).getTextColorType();
} 


void CvPlayer::changeSeaPlotYield(YieldTypes eYield, int iChange)
{
	if (iChange != 0)
	{
		m_aiSeaPlotYield.add(eYield, iChange);
		updateYield();
	}
}


void CvPlayer::changeYieldRateModifier(YieldTypes eYield, int iChange)
{
	if (iChange == 0)
		return;

	m_aiYieldRateModifier.add(eYield, iChange);
	invalidateYieldRankCache(eYield);
	if (eYield == YIELD_COMMERCE)
		updateCommerce();

	AI_makeAssignWorkDirty();
	if (getTeam() == GC.getGame().getActiveTeam())
		gDLL->UI().setDirty(CityInfo_DIRTY_BIT, true);
}


void CvPlayer::changeCapitalYieldRateModifier(YieldTypes eYield, int iChange)
{
	if (iChange == 0)
		return;

	m_aiCapitalYieldRateModifier.add(eYield, iChange);
	invalidateYieldRankCache(eYield);

	CvCity* pCapital = getCapital();
	if (pCapital == NULL)
		return;

	if (eYield == YIELD_COMMERCE)
		pCapital->updateCommerce();
	pCapital->AI_setAssignWorkDirty(true);
	if (pCapital->getTeam() == GC.getGame().getActiveTeam())
		pCapital->setInfoDirty(true);
}


void CvPlayer::updateExtraYieldThreshold(YieldTypes eYield)
{
	int iBestValue = 0;
	FOR_EACH_ENUM2(Trait, eTrait)
	{
		CvTraitInfo const& kTrait = GC.getInfo(eTrait);
		if (hasTrait(eTrait) && kTrait.getExtraYieldThreshold(eYield) > 0)
		{
			if (iBestValue == 0 ||
				kTrait.getExtraYieldThreshold(eYield) < iBestValue)
			{
				iBestValue = kTrait.getExtraYieldThreshold(eYield);
			}
		}
	}
	if (getExtraYieldThreshold(eYield) != iBestValue)
	{
		m_aiExtraYieldThreshold.set(eYield, iBestValue);
		FAssert(getExtraYieldThreshold(eYield) >= 0);
		updateYield();
	}
}


void CvPlayer::changeTradeYieldModifier(YieldTypes eYield, int iChange)
{
	if (iChange != 0)
	{
		m_aiTradeYieldModifier.add(eYield, iChange);
		updateTradeRoutes();
	}
}


void CvPlayer::changeFreeCityCommerce(CommerceTypes eCommerce, int iChange)
{
	if (iChange != 0)
	{
		m_aiFreeCityCommerce.add(eCommerce, iChange);
		FAssert(getFreeCityCommerce(eCommerce) >= 0);
		updateCommerce(eCommerce);
	}
}


/*	K-Mod. This function has been rewritten to enforce
	the rules of flexible / inflexible commerce types. (not all changes marked) */
bool CvPlayer::setCommercePercent(CommerceTypes eCommerce, int iNewValue, bool bForce)
{
	if (!bForce && !isCommerceFlexible(eCommerce))
	{
		FErrorMsg("setCommercePercent called without permission.");
		return false; // can't change percent
	}

	int const iOldValue = getCommercePercent(eCommerce);
	iNewValue = range(iNewValue, 0, 100);
	if (iOldValue == iNewValue)
		return false;

	m_aiCommercePercent.set(eCommerce, iNewValue);

	int iTotalCommercePercent = 0;
	FOR_EACH_ENUM(Commerce)
		iTotalCommercePercent += getCommercePercent(eLoopCommerce);
	FOR_EACH_ENUM(Commerce)
	{
		if (iTotalCommercePercent == 100) // advc (moved out of the header)
			break;
		if (eLoopCommerce != eCommerce && isCommerceFlexible(eLoopCommerce))
		{
			FAssert(bForce || isCommerceFlexible(eLoopCommerce));
			int iAdjustment = std::min(
					m_aiCommercePercent.get(eLoopCommerce),
					iTotalCommercePercent - 100);
			m_aiCommercePercent.add(eLoopCommerce, -iAdjustment);
			iTotalCommercePercent -= iAdjustment;
		}
	}
	/*	if we couldn't balance the books,
		then we need to do a second pass, with fewer restrictions */
	FOR_EACH_ENUM(Commerce)
	{
		if (iTotalCommercePercent == 100) // advc (moved out of the header)
			break;
		if (bForce ? (eLoopCommerce != eCommerce) : isCommerceFlexible(eLoopCommerce))
		{
			FAssert(bForce || isCommerceFlexible(eLoopCommerce));
			int iAdjustment = std::min(
					m_aiCommercePercent.get(eLoopCommerce),
					iTotalCommercePercent - 100);
			m_aiCommercePercent.add(eLoopCommerce, -iAdjustment);
			iTotalCommercePercent -= iAdjustment;
		}
	}
	FAssert(iTotalCommercePercent == 100);

	if (iOldValue == getCommercePercent(eCommerce))
		return false;

	updateCommerce();
	/*	K-Mod. For human players, update commerce weight immediately
		so that they can see effects on working plots, etc. */
	if (isHuman() && isTurnActive() &&
		/*	advc.001: Don't do this before the game is fully initialized,
			in particular not before CvGame::initGameHandicap. */
		getNumCities() > 0)
	{
		AI().AI_updateCommerceWeights();
	} // K-Mod end
	AI_makeAssignWorkDirty();
	/*if (getTeam() == GC.getGame().getActiveTeam()) {
		gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
		gDLL->UI().setDirty(Score_DIRTY_BIT, true);
		gDLL->UI().setDirty(CityScreen_DIRTY_BIT, true);
		gDLL->UI().setDirty(Financial_Screen_DIRTY_BIT, true);
	}*/ // BtS
	// K-Mod
	if (getTeam() == GC.getGame().getActiveTeam())
		gDLL->UI().setDirty(GameData_DIRTY_BIT, true); // research turns left?
	if (getID() == GC.getGame().getActivePlayer())
	{
		gDLL->UI().setDirty(CityInfo_DIRTY_BIT, true);
		gDLL->UI().setDirty(Financial_Screen_DIRTY_BIT, true);
		// <advc.120c>
		// For slider on Espionage screen
		gDLL->UI().setDirty(Espionage_Advisor_DIRTY_BIT, true);
		// Redraw +/- buttons if espionage set to 0
		gDLL->UI().setDirty(PercentButtons_DIRTY_BIT, true);
		/*  There seems to be some sort of race condition here. Occasionally
			(rarely?), the espionage screen isn't correctly updated when the
			slider goes from 0 to 10. Also, the order of these two setDirty calls
			seems to matter. Dirtying the Espionage_Advisor first works better. */
		// </advc.120c>
	}
	// K-Mod end

	return true;
}


int CvPlayer::getCommerceRate(CommerceTypes eCommerce) const
{
	int iRate = m_aiCommerceRate.get(eCommerce);
	if (GC.getGame().isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		if (eCommerce == COMMERCE_CULTURE)
			iRate += m_aiCommerceRate.get(COMMERCE_ESPIONAGE);
		else if (eCommerce == COMMERCE_ESPIONAGE)
			iRate = 0;
	}  // <advc.004x>
	iRate /= 100;
	if (eCommerce == COMMERCE_RESEARCH)
	{	// advc.910:
		static int const iBASE_RESEARCH_RATE = GC.getDefineINT("BASE_RESEARCH_RATE");
		iRate += iBASE_RESEARCH_RATE;
	}
	return iRate; // </advc.004x>
}


void CvPlayer::changeCommerceRate(CommerceTypes eCommerce, int iChange)
{
	if (iChange != 0)
	{
		m_aiCommerceRate.add(eCommerce, iChange);
		FAssert(getCommerceRate(eCommerce) >= 0);
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
	}
}


void CvPlayer::changeCommerceRateModifier(CommerceTypes eCommerce, int iChange)
{
	if (iChange != 0)
	{
		m_aiCommerceRateModifier.add(eCommerce, iChange);
		updateCommerce(eCommerce);
		AI_makeAssignWorkDirty();
	}
}


void CvPlayer::changeCapitalCommerceRateModifier(CommerceTypes eCommerce, int iChange)
{
	if(iChange == 0)
		return;
	m_aiCapitalCommerceRateModifier.add(eCommerce, iChange);
	CvCity* pCapital = getCapital();
	if (pCapital != NULL)
	{
		pCapital->updateCommerce();
		pCapital->AI_setAssignWorkDirty(true);
	}
}


void CvPlayer::changeStateReligionBuildingCommerce(CommerceTypes eCommerce, int iChange)
{
	if (iChange != 0)
	{
		m_aiStateReligionBuildingCommerce.add(eCommerce, iChange);
		FAssert(getStateReligionBuildingCommerce(eCommerce) >= 0);
		updateCommerce(eCommerce);
	}
}


void CvPlayer::changeSpecialistExtraCommerce(CommerceTypes eCommerce, int iChange)
{
	if (iChange != 0)
	{
		m_aiSpecialistExtraCommerce.add(eCommerce, iChange);
		FAssert(getSpecialistExtraCommerce(eCommerce) >= 0);
		updateCommerce(eCommerce);
		AI_makeAssignWorkDirty();
	}
}


bool CvPlayer::isCommerceFlexible(CommerceTypes eCommerce) const
{
	/* if (!isFoundedFirstCity())
		return false;
	if (eCommerce == COMMERCE_ESPIONAGE) {
		if (0 == GET_TEAM(getTeam()).getHasMetCivCount(true) || GC.getGame().isOption(GAMEOPTION_NO_ESPIONAGE))
			return false;
	} */ // Disabled by K-Mod. (I don't want to enforce these conditions in such a fundamental way.)
	return (GC.getInfo(eCommerce).isFlexiblePercent() ||
			getCommerceFlexibleCount(eCommerce) > 0 ||
			GET_TEAM(getTeam()).isCommerceFlexible(eCommerce));
}


void CvPlayer::changeCommerceFlexibleCount(CommerceTypes eCommerce, int iChange)
{
	if (iChange == 0)
		return;

	m_aiCommerceFlexibleCount.add(eCommerce, iChange);
	FAssert(getCommerceFlexibleCount(eCommerce) >= 0);
	if (!isCommerceFlexible(eCommerce))
	{
		setCommercePercent(eCommerce, //0
				GC.getInfo(eCommerce).getInitialPercent(), true); // K-Mod
	}
	if (getID() == GC.getGame().getActivePlayer())
	{
		gDLL->UI().setDirty(PercentButtons_DIRTY_BIT, true);
		gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
	}
}


void CvPlayer::changeGoldPerTurnByPlayer(PlayerTypes ePlayer, int iChange)
{
	if (iChange != 0)
	{
		m_iGoldPerTurn += iChange;
		m_aiGoldPerTurnByPlayer.add(ePlayer, iChange);

		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(GameData_DIRTY_BIT, true);

		if (!isHuman() /* advc: */ && isAlive())
		{			// ^Can get called while canceling deals of a dying player
			AI().AI_doCommerce();
		}
	}
}


bool CvPlayer::isFeatAccomplished(FeatTypes eFeat) const
{
	return m_abFeatAccomplished.get(eFeat);
}


void CvPlayer::setFeatAccomplished(FeatTypes eFeat, bool bNewValue)
{
	m_abFeatAccomplished.set(eFeat, bNewValue);
}


bool CvPlayer::isOption(PlayerOptionTypes eOption) const
{
	/*  <advc.127> AI Auto-Play should probably disable player options temporarily,
		but I don't think it does and this looks like a quick way to fix the problem.
		AI players can't have options. Tagging advc.001. */
	if (!isHuman())
		return false; // </advc.127>
	return m_abOptions.get(eOption);
}


void CvPlayer::setOption(PlayerOptionTypes eOption, bool bNewValue)
{
	m_abOptions.set(eOption, bNewValue);
	/*	<advc.004z>, advc.001: At game start, colors and plot indicators get
		updated before player options are set. Need to do another update after
		the recommendations option has been set. And, actually, should always
		do an update when that option changes. */
	if (eOption == PLAYEROPTION_NO_UNIT_RECOMMENDATIONS)
	{
		gDLL->UI().setDirty(GlobeLayer_DIRTY_BIT, true);
		gDLL->UI().setDirty(ColoredPlots_DIRTY_BIT, true);
	} // </advc.004z>
}

/*	advc: This was being checked in multiple places.
	(I haven't tagged the call locations with comments.)
	advc.121 (note): Forts are allowed outside of city radii despite safe automation.
	But this only matters in one call location currently (CvSelectionGroup::groupBuild).
	Most call locations don't check a particular build, and I don't want to slow
	isAutomationSafe down with an optional parameter. */
bool CvPlayer::isAutomationSafe(CvPlot const& kPlot) const
{
	return (isOption(PLAYEROPTION_SAFE_AUTOMATION) && kPlot.isImproved() &&
			kPlot.getImprovementType() != GC.getRUINS_IMPROVEMENT());
}


bool CvPlayer::isPlayable() const
{
	return GC.getInitCore().getPlayableCiv(getID());
}


void CvPlayer::setPlayable(bool bNewValue)
{
	GC.getInitCore().setPlayableCiv(getID(), bNewValue);
}


void CvPlayer::changeBonusExport(BonusTypes eBonus, int iChange)
{
	if(iChange == 0)
		return; // advc
	CvCity* pCapital = getCapital();
	if (pCapital != NULL)
		pCapital->getPlot().updatePlotGroupBonus(false, /* advc.064d: */ false);

	m_aiBonusExport.add(eBonus, iChange);
	FAssert(getBonusExport(eBonus) >= 0);
	if (pCapital != NULL)
		pCapital->getPlot().updatePlotGroupBonus(true);
	// <advc.036>
	if (isAlive()) // Deals can get canceled while dying
		AI().AI_updateBonusValue(); // </advc.036>
}


void CvPlayer::changeBonusImport(BonusTypes eBonus, int iChange)
{
	if(iChange == 0)
		return; // advc
	CvCity* pCapital = getCapital();
	if (pCapital != NULL)
		pCapital->getPlot().updatePlotGroupBonus(false, /* advc.064d: */ false);

	m_aiBonusImport.add(eBonus, iChange);
	FAssert(getBonusImport(eBonus) >= 0);
	if (pCapital != NULL)
		pCapital->getPlot().updatePlotGroupBonus(true);
	// <advc.036>
	if (isAlive()) // Deals can get canceled while dying
		AI().AI_updateBonusValue(); // </advc.036>
}


void CvPlayer::changeImprovementCount(ImprovementTypes eImprov, int iChange)
{
	m_aiImprovementCount.add(eImprov, iChange);
	FAssert(getImprovementCount(eImprov) >= 0);
}


void CvPlayer::changeFreeBuildingCount(BuildingTypes eBuilding, int iChange)
{
	if(iChange == 0)
		return;

	int const iOldFreeBuildingCount = getFreeBuildingCount(eBuilding);

	m_aiFreeBuildingCount.add(eBuilding, iChange);
	FAssert(getFreeBuildingCount(eBuilding) >= 0);

	if (iOldFreeBuildingCount == 0)
	{
		FAssert(getFreeBuildingCount(eBuilding) > 0);
		FOR_EACH_CITY_VAR(pLoopCity, *this)
			pLoopCity->setNumFreeBuilding(eBuilding, 1);
	}
	else if (getFreeBuildingCount(eBuilding) == 0)
	{
		FAssert(iOldFreeBuildingCount > 0);
		FOR_EACH_CITY_VAR(pLoopCity, *this)
			pLoopCity->setNumFreeBuilding(eBuilding, 0);
	}

}


void CvPlayer::changeExtraBuildingHappiness(BuildingTypes eBuilding, int iChange)
{
	if (iChange != 0)
	{
		m_aiExtraBuildingHappiness.add(eBuilding, iChange);
		updateExtraBuildingHappiness();
	}
}


void CvPlayer::changeExtraBuildingHealth(BuildingTypes eBuilding, int iChange)
{
	if (iChange != 0)
	{
		m_aiExtraBuildingHealth.add(eBuilding, iChange);
		updateExtraBuildingHealth();
	}
}


void CvPlayer::changeFeatureHappiness(FeatureTypes eFeature, int iChange)
{
	if (iChange != 0)
	{
		m_aiFeatureHappiness.add(eFeature, iChange);
		updateFeatureHappiness();
	}
}


bool CvPlayer::isUnitClassMaxedOut(UnitClassTypes eUnitClass, int iExtra) const
{
	if (!GC.getInfo(eUnitClass).isNationalUnit())
		return false;
	FAssert(getUnitClassCount(eUnitClass) <= GC.getInfo(eUnitClass).getMaxPlayerInstances());
	return (getUnitClassCount(eUnitClass) + iExtra >= GC.getInfo(eUnitClass).getMaxPlayerInstances());
}


void CvPlayer::changeUnitClassCount(UnitClassTypes eUnitClass, int iChange)
{
	m_aiUnitClassCount.add(eUnitClass, iChange);
	FAssert(getUnitClassCount(eUnitClass) >= 0);
}


void CvPlayer::changeUnitClassMaking(UnitClassTypes eUnitClass, int iChange)
{
	if (iChange != 0)
	{
		m_aiUnitClassMaking.add(eUnitClass, iChange);
		FAssert(getUnitClassMaking(eUnitClass) >= 0);
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(Help_DIRTY_BIT, true);
	}
}


bool CvPlayer::isBuildingClassMaxedOut(BuildingClassTypes eBuildingClass, int iExtra) const
{
	if (!GC.getInfo(eBuildingClass).isNationalWonder())
		return false;
	FAssert(getBuildingClassCount(eBuildingClass) <=
			GC.getInfo(eBuildingClass).getMaxPlayerInstances() +
			GC.getInfo(eBuildingClass).getExtraPlayerInstances());
	return (getBuildingClassCount(eBuildingClass) + iExtra >=
			GC.getInfo(eBuildingClass).getMaxPlayerInstances() +
			GC.getInfo(eBuildingClass).getExtraPlayerInstances());
}


void CvPlayer::changeBuildingClassCount(BuildingClassTypes eBuildingClass, int iChange)
{
	m_aiBuildingClassCount.add(eBuildingClass, iChange);
	FAssert(getBuildingClassCount(eBuildingClass) >= 0);
}


void CvPlayer::changeBuildingClassMaking(BuildingClassTypes eBuildingClass, int iChange)
{
	if (iChange != 0)
	{
		m_aiBuildingClassMaking.add(eBuildingClass, iChange);
		FAssert(getBuildingClassMaking(eBuildingClass) >= 0);
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(Help_DIRTY_BIT, true);
	}
}


void CvPlayer::changeHurryCount(HurryTypes eHurry, int iChange)
{
	int const iOldHurryCount = m_aiHurryCount.get(eHurry);
	m_aiHurryCount.add(eHurry, iChange);
	FAssert(getHurryCount(eHurry) >= 0);

	// if we just went from 0 to 1 (or the reverse)
	if ((iOldHurryCount > 0) != (m_aiHurryCount.get(eHurry) > 0))
	{
		// does this hurry reduce population?
		if (GC.getInfo(eHurry).getProductionPerPopulation() > 0)
		{
			m_iPopRushHurryCount += iChange;
			FAssert(m_iPopRushHurryCount >= 0);
		} // <advc.064b>
		if (GC.getInfo(eHurry).getGoldPerProduction() > 0)
		{
			m_iGoldRushHurryCount += iChange;
			FAssert(m_iGoldRushHurryCount >= 0);
		} // </advc.064b>
	}
}

// advc.912d:
int CvPlayer::getFoodKept(BuildingTypes eBuilding) const
{
	CvBuildingInfo const& kBuilding = GC.getInfo(eBuilding);
	int iFoodKept = kBuilding.getFoodKept();
	if (GC.getGame().isOption(GAMEOPTION_NO_SLAVERY) &&
		/*	Toggling this on and off during AI Auto Play
			could lead to inconsistent data at CvCity */
		(isHuman() || isHumanDisabled()))
	{
		iFoodKept *= 5;
		iFoodKept /= 4;
	}
	return iFoodKept;
}


void CvPlayer::changeSpecialBuildingNotRequiredCount(SpecialBuildingTypes eSpecial, int iChange)
{
	m_aiSpecialBuildingNotRequiredCount.add(eSpecial, iChange);
	FAssert(getSpecialBuildingNotRequiredCount(eSpecial) >= 0);
}


void CvPlayer::changeHasCivicOptionCount(CivicOptionTypes eCivicOption, int iChange)
{
	m_aiHasCivicOptionCount.add(eCivicOption, iChange);
	FAssert(getHasCivicOptionCount(eCivicOption) >= 0);
}


void CvPlayer::changeNoCivicUpkeepCount(CivicOptionTypes eCivicOption, int iChange)
{
	if (iChange != 0)
	{
		m_aiNoCivicUpkeepCount.add(eCivicOption, iChange);
		FAssert(getNoCivicUpkeepCount(eCivicOption) >= 0);
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
	}
}


int CvPlayer::getHasCorporationCount(CorporationTypes eCorp) const
{
	if (!isActiveCorporation(eCorp))
		return 0;
	return m_aiHasCorporationCount.get(eCorp);
}


int CvPlayer::countTotalHasCorporation() const
{
	int iCount = 0;
	FOR_EACH_ENUM(Corporation)
		iCount += getHasCorporationCount(eLoopCorporation);
	return iCount;
}

bool CvPlayer::isActiveCorporation(CorporationTypes eCorp) const
{
	if (isNoCorporations())
		return false;
	if (isNoForeignCorporations() && !hasHeadquarters(eCorp))
		return false;
	return true;
}


int CvPlayer::findHighestHasReligionCount() const
{
	int iBestValue = 0;
	FOR_EACH_ENUM(Religion)
		iBestValue = std::max(iBestValue, getHasReligionCount(eLoopReligion));
	return iBestValue;
}


void CvPlayer::changeHasReligionCount(ReligionTypes eReligion, int iChange)
{
	if (iChange != 0)
	{
		m_aiHasReligionCount.add(eReligion, iChange);
		FAssert(getHasReligionCount(eReligion) >= 0);
		GC.getGame().updateBuildingCommerce();
		GC.getGame().AI_makeAssignWorkDirty();
	}
}


// advc.132: Body basically from CvPlayerAI::AI_religionTrade (thresholds tweaked)
bool CvPlayer::isMajorReligion(ReligionTypes eReligion) const
{
	int iReligionCities = getHasReligionCount(eReligion);
	if(getStateReligion() == NO_RELIGION)
		return iReligionCities > getNumCities() / 3;
	int iOldReligionCities = getHasReligionCount(getStateReligion());
	/*  Not necessarily equal to the number of cities with neither religion,
		but close enough. */
	int iOtherCities = getNumCities() - iReligionCities - iOldReligionCities;
	return (iReligionCities > iOtherCities / 3 +
			std::max(0, iOldReligionCities - 1) / 2);
}


void CvPlayer::changeHasCorporationCount(CorporationTypes eCorp, int iChange)
{
	if (iChange != 0)
	{
		m_aiHasCorporationCount.add(eCorp, iChange);
		FAssert(getHasCorporationCount(eCorp) >= 0);
		GC.getGame().updateBuildingCommerce();
		GC.getGame().AI_makeAssignWorkDirty();
	}
}


int CvPlayer::getUpkeepCount(UpkeepTypes eUpkeep) const
{
	FErrorMsg("m_aiUpkeepCount not unused anymore?"); // advc.003j
	return m_aiUpkeepCount.get(eUpkeep);
}


void CvPlayer::changeUpkeepCount(UpkeepTypes eUpkeep, int iChange)
{
	FErrorMsg("m_aiUpkeepCount not unused anymore?"); // advc.003j
	if (iChange != 0)
	{
		m_aiUpkeepCount.add(eUpkeep, iChange);
		FAssert(getUpkeepCount(eUpkeep) >= 0);
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
	}
}


void CvPlayer::changeSpecialistValidCount(SpecialistTypes eSpecialist, int iChange)
{
	if (iChange != 0)
	{
		m_aiSpecialistValidCount.add(eSpecialist, iChange);
		FAssert(getSpecialistValidCount(eSpecialist) >= 0);
		AI_makeAssignWorkDirty();
	}
}


void CvPlayer::setResearchingTech(TechTypes eTech, bool bNewValue)
{
	if (isResearchingTech(eTech) != bNewValue)
	{
		m_abResearchingTech.set(eTech, bNewValue);
		if (getID() == GC.getGame().getActivePlayer())
		{
			// to check whether we still need the tech chooser popup
			gDLL->UI().setDirty(Popup_DIRTY_BIT, true);
		}
	}
}


void CvPlayer::setLoyalMember(VoteSourceTypes eVoteSource, bool bNewValue)
{
	if (isLoyalMember(eVoteSource) != bNewValue)
	{
		processVoteSource(eVoteSource, false);
		m_abLoyalMember.set(eVoteSource, bNewValue);
		processVoteSource(eVoteSource, true);
		GC.getGame().updateSecretaryGeneral();
	}
}

// advc.enum: Needed in a few places
void CvPlayer::getCivics(CivicMap& kResult) const
{
	FOR_EACH_ENUM(CivicOption)
		kResult.set(eLoopCivicOption, getCivics(eLoopCivicOption));
}


int CvPlayer::getSingleCivicUpkeep(CivicTypes eCivic, bool bIgnoreAnarchy,
	int iExtraCities) const // advc.004b
{
	if (eCivic == NO_CIVIC)
		return 0;

	if (isNoCivicUpkeep(GC.getInfo(eCivic).getCivicOptionType()))
		return 0;

	if (GC.getInfo(eCivic).getUpkeep() == NO_UPKEEP)
		return 0;


	if (!bIgnoreAnarchy && isAnarchy())
		return 0;

	static int const iUPKEEP_POPULATION_OFFSET = GC.getDefineINT("UPKEEP_POPULATION_OFFSET"); // advc.opt
	static int const iUPKEEP_CITY_OFFSET = GC.getDefineINT("UPKEEP_CITY_OFFSET"); // advc.opt

	int iUpkeep = 0;
	iUpkeep += (std::max(0, getTotalPopulation() +
			(iExtraCities * CvCity::initialPopulation()) + // advc.004b
			iUPKEEP_POPULATION_OFFSET - GC.getInfo(eCivic).getCivicOptionType()) *
			GC.getInfo((UpkeepTypes)GC.getInfo(eCivic).getUpkeep()).
			getPopulationPercent()) / 100;
	iUpkeep += (std::max(0, getNumCities() + /* advc.004b: */ iExtraCities + 
			iUPKEEP_CITY_OFFSET + GC.getInfo(eCivic).getCivicOptionType() -
			GC.getNumCivicOptionInfos() / 2) * GC.getInfo((UpkeepTypes)
			GC.getInfo(eCivic).getUpkeep()).getCityPercent()) / 100;

	iUpkeep *= std::max(0, getUpkeepModifier() + 100);
	iUpkeep /= 100;

	iUpkeep *= GC.getInfo(getHandicapType()).getCivicUpkeepPercent();
	iUpkeep /= 100;

	if (!isHuman() && !isBarbarian())
	{
		iUpkeep *= GC.getInfo(GC.getGame().getHandicapType()).getAICivicUpkeepPercent();
		iUpkeep /= 100;
		// advc.251: Gold costs are no longer adjusted to handicap
		/*iUpkeep *= std::max(0, ((GC.getInfo(GC.getGame().getHandicapType()).getAIPerEraModifier() * getCurrentEra()) + 100));
		iUpkeep /= 100;*/
	}

	return std::max(0, iUpkeep);
}


int CvPlayer::getCivicUpkeep(CivicMap const* pCivics, bool bIgnoreAnarchy,
	int iExtraCities) const // advc.004b
{
	if (pCivics == NULL)
		pCivics = &m_aeCivics;

	int iTotalUpkeep = 0;
	FOR_EACH_ENUM(CivicOption)
	{
		iTotalUpkeep += getSingleCivicUpkeep(pCivics->get(eLoopCivicOption),
				bIgnoreAnarchy, /* advc.004b: */ iExtraCities);
	}

	return iTotalUpkeep;
}


void CvPlayer::setCivics(CivicOptionTypes eCivicOption, CivicTypes eNewValue)
{
	CivicTypes const eOldCivic = getCivics(eCivicOption);
	if(eOldCivic == eNewValue)
		return;

	bool bWasStateReligion = isStateReligion(); // advc.106

	m_aeCivics.set(eCivicOption, eNewValue);
	if (eOldCivic != NO_CIVIC)
		processCivics(eOldCivic, -1);
	if (getCivics(eCivicOption) != NO_CIVIC)
		processCivics(getCivics(eCivicOption), 1);

	CvGame& kGame = GC.getGame();
	kGame.updateSecretaryGeneral();
	kGame.AI_makeAssignWorkDirty();

	if(!kGame.isFinalInitialized() || /* advc.003n: */ isBarbarian())
		return;

	if (gDLL->isDiplomacy() && (gDLL->getDiplomacyPlayer() == getID()))
		gDLL->updateDiplomacyAttitude(true);

	if (getCivics(eCivicOption) != NO_CIVIC &&
		/* BtS code (which erroneously blocked the message for certain civic switches)
		if (getCivics(eCivicOption) != GC.getInfo(getCivilizationType()).getCivilizationInitialCivics(eCivicOption))*/
		eOldCivic != NO_CIVIC) // K-Mod)
	{
		// <advc.151> Moved out of the loop
		CvWString szBuffer(gDLL->getText("TXT_KEY_MISC_PLAYER_ADOPTED_CIVIC", getNameKey(),
				GC.getInfo(getCivics(eCivicOption)).getTextKeyWide()));
		// <advc.106>
		bool const bRenounce = (!GC.getInfo(getCivics(eCivicOption)).isStateReligion() &&
				GC.getInfo(eOldCivic).isStateReligion() && bWasStateReligion &&
				getLastStateReligion() != NO_RELIGION);
		if(bRenounce)
		{
			szBuffer += L" " +  gDLL->getText("TXT_KEY_MISC_AND_RENOUNCE_RELIGION",
					GC.getInfo(getLastStateReligion()).getTextKeyWide());
		}
		else // </advc.106>
		if(eOldCivic != getCivilization().getInitialCivic(eCivicOption))
		{
			szBuffer += L" " + gDLL->getText("TXT_KEY_MISC_AND_ABOLISH_CIVIC",
					GC.getInfo(eOldCivic).getTextKeyWide());
		} // </advc.151>
		for (PlayerIter<MAJOR_CIV,KNOWN_TO> it(getTeam()); it.hasNext(); ++it)
		{
			CvPlayer const& kObs = *it;
			gDLL->UI().addMessage(kObs.getID(), false, -1, szBuffer, "AS2D_CIVIC_ADOPT",
					bRenounce ? MESSAGE_TYPE_MAJOR_EVENT : // advc.106
					MESSAGE_TYPE_MINOR_EVENT, // advc.106b
					// advc.127b:
					NULL, NO_COLOR, getCapitalX(kObs.getTeam()), getCapitalY(kObs.getTeam()));
		}
		if (bRenounce) // advc.106
		{
			szBuffer = gDLL->getText("TXT_KEY_MISC_PLAYER_ADOPTED_CIVIC", getNameKey(),
					GC.getInfo(getCivics(eCivicOption)).getTextKeyWide());
			// <advc.106>
			szBuffer += L" " + gDLL->getText("TXT_KEY_MISC_AND_RENOUNCE_RELIGION",
					GC.getInfo(getLastStateReligion()).getTextKeyWide());
			// </advc.106>
			kGame.addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szBuffer);
		}
	} // K-Mod. (environmentalism can change this. It's nice to see the effects immediately.)
	GC.getGame().updateGwPercentAnger();
	if (isMajorCiv()) // advc.003n
	{	// <K-Mod>
		for (PlayerIter<MAJOR_CIV> it(getTeam()); it.hasNext(); ++it) // advc.003n
		{
			AI().AI_updateAttitude(it->getID());
			it->AI_updateAttitude(getID());
		} // </K-Mod>
	}
}


void CvPlayer::changeSpecialistExtraYield(SpecialistTypes eSpecialist, YieldTypes eYield, int iChange)
{
	if (iChange != 0)
	{
		m_aaeSpecialistExtraYield.add(eSpecialist, eYield, iChange);
		FAssert(getSpecialistExtraYield(eSpecialist, eYield) >= 0);
		updateExtraSpecialistYield();
		AI_makeAssignWorkDirty();
	}
}


void CvPlayer::changeImprovementYieldChange(ImprovementTypes eImprov, YieldTypes eYield, int iChange)
{
	if (iChange != 0)
	{
		m_aaeImprovementYieldChange.add(eImprov, eYield, iChange);
		// FAssert(getImprovementYieldChange(eImprov, eYield) >= 0); // Towns in K-Mod get -1 commerce with serfdom.
		updateYield();
	}
}


/*	K-Mod. I've changed this function from using pUnit to using pGroup.
	I've also rewritten most of the code, to give more natural ordering,
	and to be more robust and readable code. */
void CvPlayer::updateGroupCycle(CvSelectionGroup const& kGroup)  // advc: const reference param
{
	PROFILE_FUNC();

	CvPlot const* pPlot = kGroup.plot();
	if (pPlot == NULL || !kGroup.isCycleGroup())
		return;

	CvUnit const& kUnit = *kGroup.getHeadUnit();

	//removeGroupCycle(kGroup.getID()); // will be removed while we reposition it

	CLLNode<int>* pBestSelectionGroupNode = NULL;
	int iBestCost = MAX_INT;
	CvSelectionGroup const* pPreviousGroup = NULL;
	CLLNode<int>* pSelectionGroupNode = headGroupCycleNode();
	while (pSelectionGroupNode != NULL)
	{
		CvSelectionGroup const& kNextGroup = *getSelectionGroup(pSelectionGroupNode->m_data);

		// if we find our group in the list, remove it.
		if (&kNextGroup == &kGroup)
			pSelectionGroupNode = deleteGroupCycleNode(pSelectionGroupNode);
		else if (kNextGroup.isCycleGroup() && //kNextGroup.canAllMove()
			kNextGroup.canAnyMove()) // advc.153
		{
			//int iCost = pPreviousGroup->groupCycleDistance(pGroup) + pGroup->groupCycleDistance(pNextGroup) - pPreviousGroup->groupCycleDistance(pNextGroup);
			int iCost = kGroup.groupCycleDistance(kNextGroup) +
					(pPreviousGroup == NULL ? 3 :
					pPreviousGroup->groupCycleDistance(kGroup) -
					pPreviousGroup->groupCycleDistance(kNextGroup));
			if (iCost < iBestCost)
			{
				iBestCost = iCost;
				pBestSelectionGroupNode = pSelectionGroupNode;
			}
			pPreviousGroup = &kNextGroup;
			pSelectionGroupNode = nextGroupCycleNode(pSelectionGroupNode);
		}
		else pSelectionGroupNode = nextGroupCycleNode(pSelectionGroupNode);
	}
	if (pPreviousGroup != NULL)
	{
		FAssert(pPreviousGroup->isCycleGroup() && pPreviousGroup->/*canAllMove*/canAnyMove()); // advc.153
		int iCost = pPreviousGroup->groupCycleDistance(kGroup) + 3; // cost for being at the end of the list.
		if (iCost < iBestCost)
			pBestSelectionGroupNode = NULL;
	}

	if (pBestSelectionGroupNode != NULL)
		m_groupCycle.insertBefore(kUnit.getGroupID(), pBestSelectionGroupNode);
	else m_groupCycle.insertAtEnd(kUnit.getGroupID());
	// <advc.154>
	if (isHuman() && GC.getGame().getActivePlayer() == getID())
		gDLL->UI().setDirty(SelectionButtons_DIRTY_BIT, true); // </advc.154>
}


void CvPlayer::removeGroupCycle(int iID)
{
	CLLNode<int>* pSelectionGroupNode = headGroupCycleNode();
	while (pSelectionGroupNode != NULL)
	{
		if (pSelectionGroupNode->m_data == iID)
		{
			pSelectionGroupNode = deleteGroupCycleNode(pSelectionGroupNode);
			break;
		}
		else pSelectionGroupNode = nextGroupCycleNode(pSelectionGroupNode);
	}
}

// K-Mod:
void CvPlayer::refreshGroupCycleList()
{
	std::vector<CvSelectionGroup*> update_list;

	CLLNode<int>* pNode = headGroupCycleNode();
	while (pNode)
	{
		CvSelectionGroup* pLoopGroup = getSelectionGroup(pNode->m_data);
		CvUnit* pLoopHead = pLoopGroup->getHeadUnit();
		if (pLoopHead && pLoopGroup->isCycleGroup() && //pLoopGroup->canAllMove()
			pLoopGroup->canAnyMove() && // advc.153
			(pLoopHead->hasMoved() ||
			(pLoopHead->isCargo() && pLoopHead->getTransportUnit()->hasMoved())))
		{
			update_list.push_back(pLoopGroup);
			pNode = deleteGroupCycleNode(pNode);
		}
		else pNode = nextGroupCycleNode(pNode);
	}

	for (size_t i = 0; i < update_list.size(); i++)
	{
		updateGroupCycle(*update_list[i]);
	}
}


CLLNode<int>* CvPlayer::deleteGroupCycleNode(CLLNode<int>* pNode)
{
	return m_groupCycle.deleteNode(pNode);
}


CLLNode<int>* CvPlayer::nextGroupCycleNode(CLLNode<int>* pNode) const
{
	return m_groupCycle.next(pNode);
}


CLLNode<int>* CvPlayer::previousGroupCycleNode(CLLNode<int>* pNode) const
{
	return m_groupCycle.prev(pNode);
}


CLLNode<int>* CvPlayer::headGroupCycleNode() const
{
	return m_groupCycle.head();
}


CLLNode<int>* CvPlayer::tailGroupCycleNode() const
{
	return m_groupCycle.tail();
}

//	Finds the path length from this tech type to one you already know
int CvPlayer::findPathLength(TechTypes eTech, bool bCost) const
{
	int iNumSteps = 0;
	int iShortestPath = 0;
	int iPathLength = 0;
	if (GET_TEAM(getTeam()).isHasTech(eTech) || isResearchingTech(eTech))
	{
		//	We have this tech, no reason to add this to the pre-reqs
		//	Base case return 0, we know it...
		return 0;
	}

	//	Cycle through the and paths and add up their tech lengths
	for (int i = 0; i < GC.getInfo(eTech).getNumAndTechPrereqs(); i++)
	{
		iPathLength += findPathLength(
				GC.getInfo(eTech).getPrereqAndTechs(i), bCost);
	}

	TechTypes eShortestOr = NO_TECH;
	iShortestPath = MAX_INT;
	//	Find the shortest OR tech
	for (int i = 0; i < GC.getInfo(eTech).getNumOrTechPrereqs(); i++)
	{
		TechTypes const ePreReq = GC.getInfo(eTech).getPrereqOrTechs(i);
		//	Recursively find the path length (takes into account all ANDs)
		// k146 (note): This will double-count any shared AND-prepreqs.
		iNumSteps = findPathLength(ePreReq, bCost);
		//	If the prereq is a valid tech and its the current shortest, mark it as such
		if (iNumSteps < iShortestPath)
		{
			eShortestOr = ePreReq;
			iShortestPath = iNumSteps;
		}
	}

	//	If the shortest OR is a valid tech, add the steps to it...
	if (eShortestOr != NO_TECH)
		iPathLength += iShortestPath;

	return iPathLength + (bCost ? GET_TEAM(getTeam()).getResearchCost(eTech) : 1);
}


//	Function specifically for python/tech chooser screen
int CvPlayer::getQueuePosition(TechTypes eTech) const
{
	int i = 1;
	for (CLLNode<TechTypes>* pResearchNode = headResearchQueueNode();
		pResearchNode != NULL; pResearchNode = nextResearchQueueNode(pResearchNode))
	{
		if (pResearchNode->m_data == eTech)
			return i;
		i++;
	}
	return -1;
}


void CvPlayer::clearResearchQueue()
{
	m_researchQueue.clear();

	FOR_EACH_ENUM(Tech)
		setResearchingTech(eLoopTech, false);

	if (getTeam() == GC.getGame().getActiveTeam())
	{
		gDLL->UI().setDirty(ResearchButtons_DIRTY_BIT, true);
		gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
		gDLL->UI().setDirty(Score_DIRTY_BIT, true);
	}
}

/*	Pushes research onto the queue.
	If it is an append it will put it and its pre-reqs into the queue.
	If it is not an append it will change research immediately and
	should be used with bClear. bClear will clear the entire queue. */
bool CvPlayer::pushResearch(TechTypes eTech, bool bClear, /* advc.004x: */ bool bKillPopup)
{
	FAssert(eTech != NO_TECH);

	if (GET_TEAM(getTeam()).isHasTech(eTech) || isResearchingTech(eTech))
	{	// We have this tech, no reason to add this to the pre-reqs
		return true;
	}
	if (!canEverResearch(eTech))
		return false;

	bool const bWasEmpty = (m_researchQueue.getLength() == 0); // advc.004x
	// Pop the entire queue...
	if (bClear)
		clearResearchQueue();

	// Add in all the pre-reqs for the and techs...
	for (int i = 0; i < GC.getInfo(eTech).getNumAndTechPrereqs(); i++)
	{
		if (!pushResearch(GC.getInfo(eTech).getPrereqAndTechs(i)))
			return false;
	}

	// Will return the shortest path of all the or techs. Tie breaker goes to the first one...
	TechTypes eShortestOr = NO_TECH;
	int iShortestPath = MAX_INT;
	bool bOrPrereqFound = false;
	// Cycle through all the OR techs
	for (int i = 0; i < GC.getInfo(eTech).getNumOrTechPrereqs(); i++)
	{
		TechTypes const ePreReq = GC.getInfo(eTech).getPrereqOrTechs(i);
		bOrPrereqFound = true;
		/*	If the pre-req exists, and we have it, it is the shortest path,
			get out, we're done */
		if (GET_TEAM(getTeam()).isHasTech(ePreReq))
		{
			eShortestOr = ePreReq;
			break;
		}
		if (canEverResearch(ePreReq))
		{
			// Find the length of the path to this pre-req
			int iNumSteps = findPathLength(ePreReq);
			/*	If this pre-req is a valid tech, and it's the shortest current path,
				set it as such */
			if (iNumSteps < iShortestPath)
			{
				eShortestOr = ePreReq;
				iShortestPath = iNumSteps;
			}
		}
	}

	/*	If the shortest path tech is valid, push it (and its children)
		onto the research queue recursively */
	if (eShortestOr != NO_TECH)
	{
		if (!pushResearch(eShortestOr))
			return false;
	}
	else if (bOrPrereqFound)
		return false;

	// Insert this tech at the end of the queue
	m_researchQueue.insertAtEnd(eTech);

	setResearchingTech(eTech, true);

	// Set the dirty bits
	if (getTeam() == GC.getGame().getActiveTeam())
	{
		gDLL->UI().setDirty(ResearchButtons_DIRTY_BIT, true);
		gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
		gDLL->UI().setDirty(Score_DIRTY_BIT, true);
		// <advc.004x>
		if(bKillPopup && bWasEmpty && getID() == GC.getGame().getActivePlayer())
			killAll(BUTTONPOPUP_CHOOSETECH, 0); // </advc.004x>
	}
	// ONEVENT - Tech selected (any)
	CvEventReporter::getInstance().techSelected(eTech, getID());
	return true;
}


void CvPlayer::popResearch(TechTypes eTech)
{
	CLLNode<TechTypes>* pResearchNode;
	for (pResearchNode = headResearchQueueNode(); pResearchNode;
		pResearchNode = nextResearchQueueNode(pResearchNode))
	{
		if (pResearchNode->m_data == eTech)
		{
			m_researchQueue.deleteNode(pResearchNode);
			break;
		}
	}

	setResearchingTech(eTech, false);

	if (getTeam() == GC.getGame().getActiveTeam())
	{
		gDLL->UI().setDirty(ResearchButtons_DIRTY_BIT, true);
		gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
		gDLL->UI().setDirty(Score_DIRTY_BIT, true);
	}
}


void CvPlayer::addCityName(const CvWString& szName)
{
	m_cityNames.insertAtEnd(szName);
}


int CvPlayer::getNumCityNames() const
{
	return m_cityNames.getLength();
}


CvWString CvPlayer::getCityName(int iIndex) const
{
	CLLNode<CvWString>* pCityNameNode = m_cityNames.nodeNum(iIndex);
	if (pCityNameNode != NULL)
		return pCityNameNode->m_data;
	return L"";
}


CLLNode<CvWString>* CvPlayer::nextCityNameNode(CLLNode<CvWString>* pNode) const
{
	return m_cityNames.next(pNode);
}


CLLNode<CvWString>* CvPlayer::headCityNameNode() const
{
	return m_cityNames.head();
}


void CvPlayer::deleteCity(int iID)
{
	m_cities.removeAt(iID);
}


void CvPlayer::deleteUnit(int iID)
{
	m_units.removeAt(iID);
}


CvSelectionGroup* CvPlayer::addSelectionGroup()
{
	CvSelectionGroupAI* pGroup = m_selectionGroups.AI_add();
	/*	K-Mod. Make sure that group gets added to the group cycle list.
		(we can update the specific position in the cycle list later;
		but it's important to get it into the list.) */
	m_groupCycle.insertAtEnd(pGroup->getID());
	return pGroup;
}


void CvPlayer::deleteSelectionGroup(int iID)
{
	#ifdef FASSERT_ENABLE
	bool bRemoved =
	#endif
	m_selectionGroups.removeAt(iID);
	FAssertMsg(bRemoved, "could not find group, delete failed");
}

EventTriggeredData* CvPlayer::firstEventTriggered(int *pIterIdx, bool bRev) const
{
	return (!bRev ? m_eventsTriggered.beginIter(pIterIdx) : m_eventsTriggered.endIter(pIterIdx));
}

EventTriggeredData* CvPlayer::nextEventTriggered(int *pIterIdx, bool bRev) const
{
	return (!bRev ? m_eventsTriggered.nextIter(pIterIdx) : m_eventsTriggered.prevIter(pIterIdx));
}

int CvPlayer::getNumEventsTriggered() const
{
	return m_eventsTriggered.getCount();
}

EventTriggeredData* CvPlayer::getEventTriggered(int iID) const
{
	return m_eventsTriggered.getAt(iID);
}

EventTriggeredData* CvPlayer::addEventTriggered()
{
	return m_eventsTriggered.add();
}

void CvPlayer::deleteEventTriggered(int iID)
{
	m_eventsTriggered.removeAt(iID);
}


void CvPlayer::addMessage(CvTalkingHeadMessage const& kMessage)
{
	// <advc.706> Remove messages arriving during interlude from display immediately
	CvGame const& kGame = GC.getGame();
	if(kGame.isOption(GAMEOPTION_RISE_FALL) && kGame.getActivePlayer() == getID() &&
		kGame.getRiseFall().getInterludeCountdown() >= 0)
	{
		gDLL->UI().clearEventMessages();
	} // </advc.706>
	m_listGameMessages.push_back(kMessage);
	// <advc.106b>
	// Special treatment only for events in other civs' turns.
	if(!kGame.isInBetweenTurns() && kGame.getActivePlayer() == getID())
		return;
	/* DISPLAY_ONLY, COMBAT, CHAT, QUEST don't show up on the Event tab
	   of the Turn Log, and therefore shouldn't count.
	   (That is assuming that quests also send INFO messages, which I haven't
	   verified - tbd.) */
	InterfaceMessageTypes eMessage = kMessage.getMessageType();
	if(eMessage == MESSAGE_TYPE_INFO || eMessage == MESSAGE_TYPE_MINOR_EVENT ||
		eMessage == MESSAGE_TYPE_MAJOR_EVENT || eMessage == MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY)
	{
		m_iNewMessages++; // See comment in postProcessBeginTurnEvents
	}
	if(eMessage == MESSAGE_TYPE_MAJOR_EVENT)
	{
		/*  Need to make a copy b/c, apparently, the EXE deletes the original
			before postProcessBeginTurnEvents gets called. */
		CvTalkingHeadMessage* pCopy = new CvTalkingHeadMessage(kMessage.getTurn(),
				kMessage.getLength(), kMessage.getDescription(),
				// Don't play it twice
				kMessage.getSoundPlayed() ? NULL : kMessage.getSound(),
				MESSAGE_TYPE_MAJOR_EVENT, kMessage.getIcon(), kMessage.getFlashColor(),
				kMessage.getX(), kMessage.getY(), kMessage.getOffScreenArrows(),
				kMessage.getOnScreenArrows());
		m_aMajorMsgs.push_back(pCopy);
	} // </advc.106b>
}

// <advc.106b>
void CvPlayer::postProcessMessages()
{
	/* Determining how many messages are being displayed:
	   - showMissedMessages doesn't help, that's only for Hotseat games.
	   - m_listGameMessages: Those are all messages in the log, and there is
		 no way of telling which messages have already been displayed; setShown
		 is apparently called when a message is added to the log, which happens
		 right after triggering it.
	   => Need to track messages triggered during other civs' turns manually
		 (m_iNewMessages). */
	/*  Don't show the log at game start (also: the BUG setting for the limit
		isn't available at the start of the 0th turn) */
	int iLimit = (GC.getGame().getElapsedGameTurns() <= 0 ? MAX_INT :
			getStartOfTurnMessageLimit());
	/* Finishing a tech should generate a message, which is rather superfluous
	   b/c of the splash screen. Don't want to suppress it b/c it should go
	   into the log, but don't count it when deciding whether to open the log
	   b/c the tech finished message doesn't take up much attention. */
	if(getCurrentResearch() == NO_TECH)
		m_iNewMessages--;
	// Don't open the Turn Log when there's only first-contact diplo
	bool bRelevantDiplo = false;
	if(!m_listDiplomacy.empty() && m_iNewMessages > 0)
	{
		TCHAR const* aszRelevantNonOffers[] = { "CANCEL_DEAL", "RELIGION_PRESSURE",
			"CIVIC_PRESSURE", "JOIN_WAR", "STOP_TRADING",
		};
		for(CvDiploQueue::const_iterator it = m_listDiplomacy.begin(); it !=
			m_listDiplomacy.end(); ++it)
		{
			CvDiploParameters* dp = *it;
			if(dp == NULL)
			{
				FAssert(dp != NULL);
				continue;
			}
			if(dp->getHumanDiplo() || dp->getOurOfferList().getLength() > 0 ||
				dp->getTheirOfferList().getLength() > 0)
			{
				bRelevantDiplo = true;
				break;
			}
			for(int i = 0; i < ARRAY_LENGTH(aszRelevantNonOffers); i++)
			{
				if(dp->getDiploComment() == GC.getAIDiploCommentType(aszRelevantNonOffers[i]))
				{
					bRelevantDiplo = true;
					break;
				}
			}
		}
	}
	if(!GC.getGame().getAIAutoPlay() && iLimit >= 0 && (m_iNewMessages > iLimit ||
		(m_iNewMessages > 0 && (bRelevantDiplo ||
		/*  Hotseat seems to show messages only if there hasn't been another
			human turn since the message was triggered (can't check that here;
			have to show the Turn Log in all cases). */
		GC.getGame().isHotSeat()))))
	{
		gDLL->UI().clearEventMessages();
		if(!GC.getGame().isHotSeat())
		{
			/*  Show major events even if the Turn Log gets opened. As with
				NewMessages, CvPlayer needs to keep track of the recent messages;
				use aMajorMsgs for that. */
			for(size_t i = 0; i < m_aMajorMsgs.size(); i++)
				gDLL->UI().showMessage(*m_aMajorMsgs[i]);
		}
		gDLL->UI().showTurnLog();
	}
	// Clear messages in any case
	for(size_t i = 0; i < m_aMajorMsgs.size(); i++)
		SAFE_DELETE(m_aMajorMsgs[i]);
	m_aMajorMsgs.clear();
	GC.getGame().setInBetweenTurns(false);
}

int CvPlayer::getStartOfTurnMessageLimit() const
{
	if(!BUGOption::isEnabled("MainInterface__AutoOpenEventLog", true))
		return -1;
	int iR = BUGOption::getValue("MainInterface__MessageLimit", 3);
	if(!isOption(PLAYEROPTION_MINIMIZE_POP_UPS) &&
		GC.getDefineINT("MESSAGE_LIMIT_WITHOUT_MPU") == 0)
	{
		return -1;
	}
	return iR;
} // </advc.106b>


void CvPlayer::clearMessages()
{
	m_listGameMessages.clear();
}


const CvMessageQueue& CvPlayer::getGameMessages() const
{
	return m_listGameMessages;
}


void CvPlayer::expireMessages()
{
	CvMessageQueue::iterator it = m_listGameMessages.begin();
	bool bFoundExpired = false;
	while(it != m_listGameMessages.end())
	{
		CvTalkingHeadMessage& kMessage = *it;
		if (GC.getGame().getGameTurn() >= kMessage.getExpireTurn(
			isHuman() || isHumanDisabled())) // advc.700
		{
			it = m_listGameMessages.erase(it);
			bFoundExpired = true;
		}
		else ++it;
	}
	if (bFoundExpired)
		gDLL->UI().dirtyTurnLog(getID());
}


void CvPlayer::addPopup(CvPopupInfo* pInfo, bool bFront)
{
	if (!isHuman())
	{
		SAFE_DELETE(pInfo);
		return;
	}  // <advc.004x>
	ButtonPopupTypes eType = pInfo->getButtonPopupType();
	if (eType == BUTTONPOPUP_CHANGERELIGION)
		killAll(BUTTONPOPUP_CHANGERELIGION);
	else if (eType == BUTTONPOPUP_CHANGECIVIC)
		killAll(BUTTONPOPUP_CHANGECIVIC);
	else if (eType == BUTTONPOPUP_CHOOSETECH)
		killAll(BUTTONPOPUP_CHOOSETECH, 0);
	else if (eType == BUTTONPOPUP_PYTHON_SCREEN)
	{
		CvGame& kGame = GC.getGame();
		if (kGame.getElapsedGameTurns() <= 0 && kGame.getActivePlayer() != NO_PLAYER)
		{
			// Must be DawnOfMan then
			kGame.setDawnOfManShown(true);
			bFront = true;
			GET_PLAYER(kGame.getActivePlayer()).doChangeCivicsPopup(NO_CIVIC);
		}
	} // </advc.004x>
	if (bFront)
		m_listPopups.push_front(pInfo);
	else m_listPopups.push_back(pInfo);
}


void CvPlayer::clearPopups()
{
	CvPopupQueue::iterator it;
	for (it = m_listPopups.begin(); it != m_listPopups.end(); ++it)
	{
		SAFE_DELETE(*it);
	}
	m_listPopups.clear();
}


CvPopupInfo* CvPlayer::popFrontPopup()
{
	CvPopupInfo* pInfo = NULL;
	if (!m_listPopups.empty())
	{
		pInfo = m_listPopups.front();
		m_listPopups.pop_front();
	}
	return pInfo;
}


const CvPopupQueue& CvPlayer::getPopups() const
{
	// advc.test:
	FAssertMsg(GC.getGame().getActivePlayer() == getID(),
			"Just to see under which circumstances (if any) the EXE adds popups to AI players");
	return m_listPopups;
}


void CvPlayer::addDiplomacy(CvDiploParameters* pDiplo)
{
	FAssert(pDiplo != NULL); // advc
	m_listDiplomacy.push_back(pDiplo);
}


void CvPlayer::clearDiplomacy()
{
	CvDiploQueue::iterator it;
	for (it = m_listDiplomacy.begin(); it != m_listDiplomacy.end(); ++it)
	{
		SAFE_DELETE(*it);
	}
	m_listDiplomacy.clear();
}


const CvDiploQueue& CvPlayer::getDiplomacy() const
{
	return m_listDiplomacy;
}


CvDiploParameters* CvPlayer::popFrontDiplomacy()
{
	CvDiploParameters* pDiplo = NULL;
	if (!m_listDiplomacy.empty())
	{
		pDiplo = m_listDiplomacy.front();
		m_listDiplomacy.pop_front();
	}
	return pDiplo;
}

/*  advc.001e: The EXE is supposed to double-check all AI-to-human offers at the
	start of the human turn, i.e. just before displaying them. This function
	addresses errors and omissions in the EXE code. */
void CvPlayer::validateDiplomacy()
{
	bool bDone = false;
	while (!bDone && !m_listDiplomacy.empty())
	{
		for (CvDiploQueue::iterator it = m_listDiplomacy.begin(); it != m_listDiplomacy.end(); ++it)
		{
			CvDiploParameters* pDiplo = *it;
			CLLNode<TradeData> const* pNode = pDiplo->getOurOfferList().head(); // Can be NULL!
			// Worst enemy may have changed
			if (pDiplo->getDiploComment() == GC.getAIDiploCommentType("STOP_TRADING") &&
				pDiplo->getData() != GET_TEAM(pDiplo->getWhoTalkingTo()).AI_getWorstEnemy())
			{
				CvPlayerAI& who = GET_PLAYER(pDiplo->getWhoTalkingTo());
				/*  Recipient isn't getting contacted after all. Ideally,
					this should also be done for any contact attempts that the
					EXE cancels, but it's difficult to identify these here. */
				who.AI_changeContactTimer(getID(), CONTACT_STOP_TRADING,
						-who.AI_getContactTimer(getID(), CONTACT_STOP_TRADING));
				// Have to delete it here b/c the EXE will no longer have a pointer to it
				m_listDiplomacy.remove(pDiplo);
				delete pDiplo;
				bDone = false;
				break;
			}
			// AI may send offer for DP and then receive a DoW
			else if (pNode != NULL && pNode->m_data.m_eItemType == TRADE_DEFENSIVE_PACT &&
				!GET_TEAM(getTeam()).allWarsShared(TEAMID(pDiplo->getWhoTalkingTo())))
			{
				CvPlayerAI& who = GET_PLAYER(pDiplo->getWhoTalkingTo());
				who.AI_changeContactTimer(getID(), CONTACT_DEFENSIVE_PACT,
						-who.AI_getContactTimer(getID(), CONTACT_DEFENSIVE_PACT));
				m_listDiplomacy.remove(pDiplo);
				delete pDiplo;
				bDone = false;
				break;
			}
			bDone = true;
		}
	}
	/*  <advc.134a> The EXE discards all offers for peace and surrender. Since this
		is done right before displaying the diplo popup, it doesn't help to reinsert
		the offer into m_listDiplomacy -- need to prevent the removal by the EXE. */
	std::vector<CvDiploParameters*> apInvalid;
	int iValidPeaceOffers = 0;
	for (CvDiploQueue::iterator it = m_listDiplomacy.begin(); it != m_listDiplomacy.end(); ++it)
	{
		CvDiploParameters* pDiplo = *it;
		if (pDiplo->getDiploComment() != GC.getAIDiploCommentType("OFFER_PEACE"))
			continue;
		/*  Make sure that this isn't a peace offer that the EXE will discard for
			valid reasons. I.e. check everything that the EXE checks.
			(Based on tracing DLL calls in the debugger.) */
		PlayerTypes eWho = pDiplo->getWhoTalkingTo();
		CvPlayerAI const& who = GET_PLAYER(eWho);
		// This is what the EXE seems to get wrong
		if (!::atWar(getTeam(), who.getTeam()))
		{
			apInvalid.push_back(pDiplo);
			continue;
		}
		if (!canContact(eWho) || !who.canContact(getID()))
		{
			apInvalid.push_back(pDiplo);
			continue;
		}
		bool bCapitulate = false;
		bool bValid = true;
		FOR_EACH_TRADE_ITEM(pDiplo->getOurOfferList())
		{
			/*  Also test trade denial (although the EXE doesn't do that);
				important for capitulation. */
			if (!canTradeItem(eWho, *pItem, true))
			{
				bValid = false;
				break;
			}
			if (pItem->m_eItemType == TRADE_SURRENDER)
				bCapitulate = true;
		}
		if (!bValid)
		{
			apInvalid.push_back(pDiplo);
			continue;
		}
		FOR_EACH_TRADE_ITEM(pDiplo->getTheirOfferList())
		{
			if (!who.canTradeItem(getID(), *pItem, true))
			{
				bValid = false;
				break;
			}
			if (pItem->m_eItemType == TRADE_SURRENDER)
				bCapitulate = true;
		}
		if (!bValid)
		{
			apInvalid.push_back(pDiplo);
			continue;
		}
		/*  Finally, check if the offer still makes sense for eWho.
			(Sth. the EXE doesn't check.) */
		if (!bCapitulate && !who.AI_upholdPeaceOffer(getID(), *pDiplo))
		{
			apInvalid.push_back(pDiplo);
			continue;
		}
		if (iValidPeaceOffers == 0)
		{
			/*  advancePeaceOfferStage supports only one peace offer per turn
				(this could be amended) */
			GET_TEAM(getTeam()).advancePeaceOfferStage(TEAMID(eWho));
			iValidPeaceOffers++;
		}
		else apInvalid.push_back(pDiplo);
	}
	for (size_t i = 0; i < apInvalid.size(); i++)
	{
		CvPlayerAI& who = GET_PLAYER(apInvalid[i]->getWhoTalkingTo());
		who.AI_changeContactTimer(getID(), CONTACT_PEACE_TREATY,
				-who.AI_getContactTimer(getID(), CONTACT_PEACE_TREATY));
		m_listDiplomacy.remove(apInvalid[i]);
		delete apInvalid[i];
	}
	// </advc.134a>
}


void CvPlayer::showSpaceShip()
{
	CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN);
	pInfo->setData1(-1);
	pInfo->setText(L"showSpaceShip");
	addPopup(pInfo);
}


void CvPlayer::clearSpaceShipPopups()
{
	//clear all spaceship popups
	CvPopupQueue::iterator it;
	for (it = m_listPopups.begin(); it != m_listPopups.end(); )
	{
		CvPopupInfo* pInfo = *it;
		if (pInfo != NULL)
		{
			if(pInfo->getText().compare(L"showSpaceShip") == 0)
			{
				it = m_listPopups.erase(it);
				SAFE_DELETE(pInfo);
			}
			else ++it;
		}
		else ++it;
	}
}

// advc.004x: Partly cut from CvTeam::setHasTech
void CvPlayer::doChangeCivicsPopup(CivicTypes eCivic)
{
	if(!isHuman()) // Forget reminder during Auto Play
	{
		m_eReminderPending = NO_CIVIC;
		return;
	}
	if (!canDoAnyRevolution() || GC.getGame().isAboutToShowDawnOfMan())
	{
		m_eReminderPending = (CivicTypes)std::max(m_eReminderPending, eCivic);
		return;
	}
	if(eCivic == NO_CIVIC) // Then we're only supposed to check for a pending reminder
	{
		if(m_eReminderPending != NO_CIVIC)
			doChangeCivicsPopup(m_eReminderPending);
		return;
	}
	m_eReminderPending = NO_CIVIC;
	CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_CHANGECIVIC);
	if (pInfo != NULL)
	{
		pInfo->setData1(GC.getInfo(eCivic).getCivicOptionType());
		pInfo->setData2(eCivic);
		gDLL->UI().addPopup(pInfo, getID());
	}
}

// advc.004s: Rewritten, in part with code from doTurn and CvGame::updateScore.
void CvPlayer::updateHistory(PlayerHistoryTypes eHistory, int iTurn)
{
	FAssertEnumBounds(eHistory);
	int iNewValue = -1;
	switch(eHistory)
	{
	case PLAYER_HISTORY_SCORE:
		iNewValue = GC.getGame().getPlayerScore(getID());
		break;
	case PLAYER_HISTORY_ECONOMY:
		iNewValue = calculateTotalCommerce();
		break;
	case PLAYER_HISTORY_INDUSTRY:
		iNewValue = calculateTotalYield(YIELD_PRODUCTION);
		break;
	case PLAYER_HISTORY_AGRICULTURE:
		iNewValue = calculateTotalYield(YIELD_FOOD);
		break;
	case PLAYER_HISTORY_POWER:
		iNewValue = getPower();
		break;
	case PLAYER_HISTORY_CULTURE:
		iNewValue = countTotalCulture();
		break;
	case PLAYER_HISTORY_ESPIONAGE:
		iNewValue = GET_TEAM(getTeam()).getEspionagePointsEver();
		break;
	default: FErrorMsg("Unknown player history type");
	}
	m_playerHistory[eHistory].set(iTurn, iNewValue);
} // </advc.004s>


/*	K-Mod:  Note, this function is a friend of CvEventReporter, so that it can access the data we need.
	(This saves us from having to use the built-in CyStatistics class) */
CvPlayerRecord const* CvPlayer::getPlayerRecord() const
{
	return CvEventReporter::getInstance().
			/*	advc.make: CvEventReporter::getPlayerRecord added.
				We're no longer a friend of CvEventReporter. */
			/*m_kStatistics.*/getPlayerRecord(getID());
}


std::string CvPlayer::getScriptData() const
{
	return m_szScriptData;
}


void CvPlayer::setScriptData(std::string szNewValue)
{
	m_szScriptData = szNewValue;
}


const CvString CvPlayer::getPbemEmailAddress() const
{
	return GC.getInitCore().getEmail(getID());
}


void CvPlayer::setPbemEmailAddress(const char* szAddress)
{
	GC.getInitCore().setEmail(getID(), szAddress);
}


const CvString CvPlayer::getSmtpHost() const
{
	return GC.getInitCore().getSmtpHost(getID());
}


void CvPlayer::setSmtpHost(const char* szHost)
{
	GC.getInitCore().setSmtpHost(getID(), szHost);
}


void CvPlayer::doGold()
{
	if (GC.getPythonCaller()->doGold(getID()))
		return;

	// <advc.300> Let CvGame::killBarbarian handle overcrowding
	if(isBarbarian())
		return; // </advc.300>

	int iGoldChange = calculateGoldRate();

	//FAssert(isHuman() || isBarbarian() || ((getGold() + iGoldChange) >= 0) || isAnarchy());
	/*  advc.131: Disabled b/c all of these can be OK (except isBarbarian, which is
		now handled upfront) */

	changeGold(iGoldChange);
	bool bStrike = false;
	if (getGold() < 0)
	{
		setGold(0);
		if (!isBarbarian() && getNumCities() > 0)
			bStrike = true;
	}

	if (bStrike)
	{
		setStrike(true);
		changeStrikeTurns(1);

		if (getStrikeTurns() > 1)
		{
			int iDisbandUnit = (getStrikeTurns() / 2); // XXX mod?
			// advc.131: Can happen, but should be quite rare.
			FAssert(isHuman() || isAnarchy());
			for (int iI = 0; iI < iDisbandUnit; iI++)
			{
				disbandUnit(true);
				if (calculateGoldRate() >= 0)
					break;
			}
		}
	}
	else setStrike(false);
}


void CvPlayer::doResearch()
{
	if (GC.getPythonCaller()->doResearch(getID()))
		return;

	if (isResearch() /* K-Mod: */ && !isAnarchy())
	{
		bool bForceResearchChoice = false;
		if (getCurrentResearch() == NO_TECH /* K-Mod: */ && isHuman())
		{
			if (getID() == GC.getGame().getActivePlayer())
				chooseTech();
			//if (GC.getGame().getElapsedGameTurns() > 4) { // advc.124g (commented out)
			AI().AI_chooseResearch();
			bForceResearchChoice = true;
		}

		TechTypes eCurrentTech = getCurrentResearch();
		if (eCurrentTech == NO_TECH)
		{
			int iOverflow = (100 * calculateResearchRate()) /
					std::max(1, calculateResearchModifier(eCurrentTech));
			changeOverflowResearch(iOverflow);
		}
		else
		{
			int iOverflowResearch = (getOverflowResearch() *
					calculateResearchModifier(eCurrentTech)) / 100;
			setOverflowResearch(0);
			GET_TEAM(getTeam()).changeResearchProgress(eCurrentTech,
					// K-Mod (replacing the minimum which used to be in calculateResearchRate)
					std::max(1, calculateResearchRate()) +
					iOverflowResearch, getID());
		}

		if (bForceResearchChoice)
			clearResearchQueue();
	}
}

void CvPlayer::doEspionagePoints()
{
	if (getCommerceRate(COMMERCE_ESPIONAGE) <= 0)
		return;

	GET_TEAM(getTeam()).changeEspionagePointsEver(getCommerceRate(COMMERCE_ESPIONAGE));

	// Divide up Espionage between Teams
	for (TeamIter<CIV_ALIVE,OTHER_KNOWN_TO> itOther(getTeam());
		itOther.hasNext(); ++itOther)
	{
		int iSpending = getEspionageSpending(itOther->getID());
		if (iSpending > 0)
		{
			GET_TEAM(getTeam()).changeEspionagePointsAgainstTeam(
					itOther->getID(), iSpending);
		}
	}
}

int CvPlayer::getEspionageSpending(TeamTypes eAgainstTeam) const
{
	PROFILE_FUNC(); // advc (runtime seems totally negligible)
	int iTotalWeight = 0; // Get sum of all weights to be used later on
	int iBestWeight = 0;
	bool bFoundTeam = false;
	for (TeamIter<CIV_ALIVE,OTHER_KNOWN_TO> itOther(getTeam());
		itOther.hasNext(); ++itOther)
	{
		if (itOther->getID() == eAgainstTeam)
			bFoundTeam = true;

		int iWeight = getEspionageSpendingWeightAgainstTeam(itOther->getID());
		iBestWeight = std::max(iBestWeight, iWeight);
		iTotalWeight += iWeight;
	}

	if (!bFoundTeam) // The player requested is not valid
		return -1;

	int iSpendingValue = 0;
	int const iTotalPoints = getCommerceRate(COMMERCE_ESPIONAGE);
	int iAvailablePoints = iTotalPoints;
	// Split up Espionage Point budget based on weights (if any weights have been assigned)
	if (iTotalWeight > 0)
	{
		for (TeamIter<CIV_ALIVE,OTHER_KNOWN_TO> itOther(getTeam());
			itOther.hasNext(); ++itOther)
		{
			int iChange = iTotalPoints *
					getEspionageSpendingWeightAgainstTeam(itOther->getID()) / iTotalWeight;
			iAvailablePoints -= iChange;
			if (itOther->getID() == eAgainstTeam)
				iSpendingValue += iChange;
		}
	}

	// Divide remainder evenly among top Teams
	while (iAvailablePoints > 0) // advc (comment): bFoundTeam=true ensures termination
	{
		for (TeamIter<CIV_ALIVE,OTHER_KNOWN_TO> itOther(getTeam());
			itOther.hasNext(); ++itOther)
		{
			if (getEspionageSpendingWeightAgainstTeam(itOther->getID()) == iBestWeight)
			{
				if (itOther->getID() == eAgainstTeam)
					iSpendingValue++;
				iAvailablePoints--;
				if (iAvailablePoints <= 0)
					break;
			}
		}
	}
	return iSpendingValue;
}

bool CvPlayer::canDoEspionageMission(EspionageMissionTypes eMission, PlayerTypes eTargetPlayer,
	CvPlot const* pPlot, int iExtraData, const CvUnit* pUnit,
	bool bCheckPoints) const // advc.085
{
	if (getID() == eTargetPlayer || eTargetPlayer == NO_PLAYER ||
		!GET_PLAYER(eTargetPlayer).isAlive() ||
		!GET_TEAM(getTeam()).isHasMet(TEAMID(eTargetPlayer)))
	{
		return false;
	}
	// K-Mod. Bugfix
	if (pUnit != NULL && pPlot != NULL && !pUnit->canEspionage(pPlot, false))
		return false; // K-Mod end

	CvEspionageMissionInfo const& kMission = GC.getInfo(eMission);
	// Need Tech Prereq, if applicable
	if (kMission.getTechPrereq() != NO_TECH)
	{
		if (!GET_TEAM(getTeam()).isHasTech((TechTypes)kMission.getTechPrereq()))
			return false;
	}

	int iCost = getEspionageMissionCost(
			eMission, eTargetPlayer, pPlot, iExtraData, pUnit);
	if (iCost < 0)
		return false;

	int iEspionagePoints = GET_TEAM(getTeam()).
			getEspionagePointsAgainstTeam(TEAMID(eTargetPlayer));
	// <advc.085>
	if (!bCheckPoints)
		return true;
	// </advc.085>
	return (iEspionagePoints >= iCost && iEspionagePoints > 0);
}

// <advc.120d> Mostly cut-and-paste from getEspionageMissionBaseCost
TechTypes CvPlayer::getStealCostTech(PlayerTypes eTargetPlayer) const
{
	TechTypes eR = NO_TECH;
	if (eTargetPlayer == NO_PLAYER)
		return eR;
	int iProdCost = MAX_INT;
	FOR_EACH_ENUM(Tech)
	{
		if (canStealTech(eTargetPlayer, eLoopTech))
		{
			int iCost = GET_TEAM(getTeam()).getResearchCost(eLoopTech);
			if (iCost < iProdCost)
			{
				iProdCost = iCost;
				eR = eLoopTech;
			}
		}
	}
	return eR;
}


bool CvPlayer::canSpy() const
{
	FOR_EACH_ENUM(Unit)
	{
		if(GC.getInfo(eLoopUnit).isSpy() && canTrain(eLoopUnit))
			return true;
	}
	FOR_EACH_UNIT(u, *this)
	{
		if(u->isSpy())
			return true;
	}
	return false;
} // </advc.120d>


int CvPlayer::getEspionageMissionCost(EspionageMissionTypes eMission, PlayerTypes eTargetPlayer,
	CvPlot const* pPlot, int iExtraData, const CvUnit* pSpyUnit) const
{
	int iMissionCost = getEspionageMissionBaseCost(
			eMission, eTargetPlayer, pPlot, iExtraData, pSpyUnit);
	if (iMissionCost < 0)
		return -1;

	// Multiply cost of mission * number of team members
	//iMissionCost *= GET_TEAM(getTeam()).getNumMembers(); // K-Mod
	// kekm.33/advc
	iMissionCost = adjustMissionCostToTeamSize(iMissionCost, eTargetPlayer);

	iMissionCost *= getEspionageMissionCostModifier(
			eMission, eTargetPlayer, pPlot, iExtraData, pSpyUnit);
	iMissionCost /= 100;

	return std::max(0, iMissionCost);
}

// advc: Auxiliary function for kekm.33
int CvPlayer::adjustMissionCostToTeamSize(int iBaseCost, PlayerTypes eTargetPlayer) const
{
	// Don't compute anything when the teams have equal size
	int iOurTeamSize = GET_TEAM(getTeam()).getNumMembers();
	int iTheirTeamSize = GET_TEAM(eTargetPlayer).getNumMembers();
	if(iOurTeamSize == iTheirTeamSize)
		return iBaseCost;
	// Tie it to the tech cost modifier
	scaled rExtraTeamMemberModifier = per100(
			GC.getDefineINT(CvGlobals::TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER));
	/* <kekm.33> "New formula for espionage costs in team. Essentially, I want costs
		to scale with 1+0.5(number of members - 1), but since there are two teams
		(and two directions) involved, it will scale with the square root of the
		ratio of those values. Idea for formula by Fran." */
	scaled r = (1 + rExtraTeamMemberModifier * (iOurTeamSize - 1)) /
				(1 + rExtraTeamMemberModifier * (iTheirTeamSize - 1));
	r = r.sqrt();
	r *= iBaseCost;
	return r.floor(); // </kekm.33>
}


int CvPlayer::getEspionageMissionBaseCost(EspionageMissionTypes eMission, PlayerTypes eTargetPlayer,
	CvPlot const* pPlot, int iExtraData,
	const CvUnit* pSpyUnit) const // advc (note): The unit is not currently a factor
{
	if (eMission == NO_ESPIONAGEMISSION) // K-Mod
		return -1;

	CvEspionageMissionInfo const& kMission = GC.getInfo(eMission);
	int const iBaseMissionCost = kMission.getCost();
	if (iBaseMissionCost == -1) // -1 means this mission is disabled
		return -1;

	if (kMission.isSelectPlot())
	{
		if (pPlot == NULL || !pPlot->isRevealed(getTeam()))
			return -1;
	}
	CvCity const* pCity = (pPlot == NULL ? NULL : pPlot->getPlotCity());
	if (pCity == NULL && kMission.isTargetsCity())
		return -1;

	int iMissionCost = -1;
	CvGameSpeedInfo const& kSpeed = GC.getInfo(GC.getGame().getGameSpeedType()); //advc

	if (kMission.getStealTreasuryTypes() > 0)
	{
		// Steal Treasury
		//int iNumTotalGold = (GET_PLAYER(eTargetPlayer).getGold() * kMission.getStealTreasuryTypes()) / 100;
		int iNumTotalGold /* kmodx: */ = 0;
		if (pCity!= NULL)
		{
			/*iNumTotalGold *= pCity->getPopulation();
			iNumTotalGold /= std::max(1, GET_PLAYER(eTargetPlayer).getTotalPopulation());*/
			// K-Mod:
			iNumTotalGold = getEspionageGoldQuantity(eMission, eTargetPlayer, pCity);
		}
		if (iNumTotalGold > 0)
			iMissionCost = (iBaseMissionCost * iNumTotalGold) / 100;
	}
	else if (kMission.getBuyTechCostFactor() > 0)
	{
		// Buy (Steal) Tech
		TechTypes eTech = (TechTypes)iExtraData;
		// <advc.120d> Original code moved into auxiliary function
		iMissionCost = -1;
		if (eTech == NO_TECH)
			eTech = getStealCostTech(eTargetPlayer);
		int iProdCost = (eTech == NO_TECH ? -1 : GET_TEAM(getTeam()).//getResearchCost(eTech)
				getResearchLeft(eTech)); // advc.120i: Count own progress
		// </advc.120d>
		if (eTech != NO_TECH && canStealTech(eTargetPlayer, eTech))
		{
			iMissionCost = iBaseMissionCost +
					((100 + kMission.getBuyTechCostFactor()) * iProdCost) / 100;
		}
	}
	else if (kMission.getSwitchCivicCostFactor() > 0)
	{
		// Switch Civics
		CivicTypes const eCivic = (CivicTypes)iExtraData;

		/*if (eCivic == NO_CIVIC) {
			for (int iCivic = 0; iCivic < GC.getNumCivicInfos(); ++iCivic) {
				if (canForceCivics(eTargetPlayer, (CivicTypes)iCivic)) {
					eCivic = (CivicTypes)iCivic;
					break;
				}
			}
		}
		if (eCivic != NO_CIVIC)*/
		/*  <advc.132> Replacing the above: Need to compute a cost for every civic
			(like for the DestroyBuilding mission) */
		int iMinCost = MAX_INT;
		FOR_EACH_ENUM(Civic)
		{
			if(eCivic != NO_CIVIC && eLoopCivic != eCivic)
				continue; // </advc.132>
			if (canForceCivics(eTargetPlayer, eLoopCivic))
			{
				int iLoopCost = iBaseMissionCost + (kMission.getSwitchCivicCostFactor() *
						GET_PLAYER(eTargetPlayer).getTotalPopulation() * // K-Mod
						(kSpeed.getAnarchyPercent()
						/* advc.132: Add 100 to one of the modifiers
						   (doesn't matter which). */
						+ (isCivic(eLoopCivic) ? 0 : 100))
					) / 10000;
				iMinCost = std::min(iMinCost, iLoopCost); // advc.132
			}
		} // <advc.132>
		if(iMinCost < MAX_INT)
			iMissionCost = iMinCost; // </advc.132>
	}
	else if (kMission.getSwitchReligionCostFactor() > 0)
	{
		// Switch Religions
		ReligionTypes eReligion = (ReligionTypes)iExtraData;
		/*if (NO_RELIGION == eReligion) {
			for (int iReligion = 0; iReligion < GC.getNumReligionInfos(); ++iReligion) {
				if (canForceReligion(eTargetPlayer, (ReligionTypes)iReligion)) {
					eReligion = (ReligionTypes)iReligion;
					break;
				}
			}
		}
		if (NO_RELIGION != eReligion) {*/
		// <advc.132> Replacing the above: Need to compute a cost for every religion
		int iMinCost = MAX_INT;
		FOR_EACH_ENUM(Religion)
		{
			if(eReligion != NO_RELIGION && eLoopReligion != eReligion)
				continue;
			if (canForceReligion(eTargetPlayer, eLoopReligion) &&
				(pCity == NULL || pCity->isHasReligion(eLoopReligion)))
			{// </advc.132>
				int iLoopCost = iBaseMissionCost + (kMission.getSwitchReligionCostFactor() *
						GET_PLAYER(eTargetPlayer).getTotalPopulation() * // K-Mod
						(kSpeed.getAnarchyPercent()
						// advc.132: As above for civics
						+ (getStateReligion() == eLoopReligion ? 0 : 100))
					) / 10000;
				// K-Mod.
				ReligionTypes eCurrentReligion = GET_PLAYER(eTargetPlayer).getStateReligion();
				// amplify the mission cost if we are trying to switch to a minority religion.
				if (eCurrentReligion != NO_RELIGION)
				{
					// maybe getReligionPopulation would be slightly better, but it's a bit slower.
					int iCurrent = GET_PLAYER(eTargetPlayer).getHasReligionCount(eCurrentReligion);
					int iNew = GET_PLAYER(eTargetPlayer).getHasReligionCount(eLoopReligion);
					int iCitiesTarget = GC.getInfo(GC.getMap().getWorldSize()).getTargetNumCities();
					FAssert(//iCurrent > 0 && // advc.006: Possible to lose all
											  // cities of the current state religion.
						iNew > 0);
					/*iMissionCost *= std::max(iCurrent, iNew) + iCitiesTarget;
					iMissionCost /= iNew + iCitiesTarget;*/
					/*  <advc.132> Replacing the above (to increase the impact of
						the iCurrent/iNew ratio) */
					iLoopCost *= iCurrent + iCitiesTarget;
					iLoopCost /= std::max(1, iNew + iCitiesTarget / 2);
					// </advc.132>
				} // K-Mod end
				iMinCost = std::min(iMinCost, iLoopCost); // advc.132
			}
		} // <advc.132>
		if(iMinCost < MAX_INT)
			iMissionCost = iMinCost; // </advc.132>
	}
	else if (kMission.getDestroyUnitCostFactor() > 0)
	{
		// Destroys Unit
		CvUnit const* pUnit = GET_PLAYER(eTargetPlayer).getUnit(iExtraData);
		int iCost = MAX_INT;
		if (pUnit == NULL && pPlot != NULL)
		{
			FOR_EACH_UNIT_IN(pLoopUnit, *pPlot)
			{
				if (canSpyDestroyUnit(eTargetPlayer, *pLoopUnit))
				{
					int iValue = getProductionNeeded(pLoopUnit->getUnitType());
					if (iValue < iCost)
					{
						iCost = iValue;
						pUnit = pLoopUnit;
					}
				}
			}
		}
		else iCost = getProductionNeeded(pUnit->getUnitType());

		if (pUnit != NULL && canSpyDestroyUnit(eTargetPlayer, *pUnit))
		{
			iMissionCost = iBaseMissionCost +
					((100 + kMission.getDestroyUnitCostFactor()) * iCost) / 100;
		}
	}
	else if (kMission.getDestroyProjectCostFactor() > 0)
	{
		ProjectTypes eProject = (ProjectTypes)iExtraData;
		int iCost = MAX_INT;
		if (eProject == NO_PROJECT)
		{
			FOR_EACH_ENUM(Project)
			{
				if (canSpyDestroyProject(eTargetPlayer, eLoopProject))
				{
					int iValue = getProductionNeeded(eLoopProject);
					if (iValue < iCost)
					{
						iCost = iValue;
						eProject = eLoopProject;
					}
				}
			}
		}
		else iCost = getProductionNeeded(eProject);
		if (eProject != NO_PROJECT)
		{
			if (canSpyDestroyProject(eTargetPlayer, eProject))
			{
				iMissionCost = iBaseMissionCost +
						((100 + kMission.getDestroyProjectCostFactor()) * iCost) / 100;
			}
		}
	}
	else if (kMission.getDestroyProductionCostFactor() > 0)
	{
		FAssert(pCity != NULL);
		if (pCity != NULL)
		{
			iMissionCost = iBaseMissionCost +
					((100 + kMission.getDestroyProductionCostFactor()) *
					pCity->getProduction()) / 100;
		}
	}
	else if (kMission.getBuyUnitCostFactor() > 0)
	{
		// Buy Unit
		CvUnit const* pUnit = GET_PLAYER(eTargetPlayer).getUnit(iExtraData);
		int iCost = MAX_INT;
		if (pUnit == NULL && pPlot != NULL)
		{
			FOR_EACH_UNIT_IN(pLoopUnit, *pPlot)
			{
				if (canSpyBribeUnit(eTargetPlayer, *pLoopUnit))
				{
					int iValue = getProductionNeeded(pLoopUnit->getUnitType());
					if (iValue < iCost)
					{
						iCost = iValue;
						pUnit = pLoopUnit;
					}
				}
			}
		}
		else iCost = getProductionNeeded(pUnit->getUnitType());
		if (pUnit != NULL)
		{
			if (canSpyBribeUnit(eTargetPlayer, *pUnit))
			{
				iMissionCost = iBaseMissionCost +
						((100 + kMission.getBuyUnitCostFactor()) * iCost) / 100;
			}
		}
	}
	else if (kMission.getDestroyBuildingCostFactor() > 0)
	{
		BuildingTypes eBuilding = (BuildingTypes)iExtraData;
		int iCost = MAX_INT;
		if (eBuilding == NO_BUILDING)
		{
			FOR_EACH_ENUM(Building)
			{
				if (pCity != NULL && pCity->getNumRealBuilding(eLoopBuilding) > 0 &&
					canSpyDestroyBuilding(eTargetPlayer, eLoopBuilding))
				{
					int iValue = getProductionNeeded(eLoopBuilding);
					if (iValue < iCost)
					{
						iCost = iValue;
						eBuilding = eLoopBuilding;
					}
				}
			}
		}
		else iCost = getProductionNeeded(eBuilding);
		if (eBuilding != NO_BUILDING)
		{
			if (pCity != NULL && pCity->getNumRealBuilding(eBuilding) > 0 &&
				canSpyDestroyBuilding(eTargetPlayer, eBuilding))
			{
				iMissionCost = iBaseMissionCost +
						((100 + kMission.getDestroyBuildingCostFactor()) * iCost) / 100;
			}
		}
	}
	else if (kMission.getBuyCityCostFactor() > 0)
	{	// Buy City
		if (pCity != NULL)
		{
			iMissionCost = iBaseMissionCost +
					(kMission.getBuyCityCostFactor() *
					kSpeed.getGrowthPercent()) / 10000;
		}
	}
	else if (kMission.getCityInsertCultureCostFactor() > 0)
	{
		// Insert Culture into City
		if (pPlot != NULL && pPlot->getCulture(getID()) > 0)
		{
			int iCultureAmount = kMission.getCityInsertCultureAmountFactor() *
					pCity->countTotalCultureTimes100();
			iCultureAmount /= 10000;
			iCultureAmount = std::max(1, iCultureAmount);
			iMissionCost = iBaseMissionCost +
					(kMission.getCityInsertCultureCostFactor() * iCultureAmount) / 100;
		}
	}
	else if (kMission.isDestroyImprovement())
	{
		if (pPlot != NULL && !pPlot->isCity())
		{
			if (pPlot->isImproved() || pPlot->isRoute())
				iMissionCost = (iBaseMissionCost * kSpeed.getBuildPercent()) / 100;
		}
	}
	else if (kMission.getCityPoisonWaterCounter() > 0)
	{
		FAssert(pCity != NULL);
		// Cannot poison a city's water supply if it's already poisoned (value is negative when active)
		if (pCity != NULL && pCity->getEspionageHealthCounter() <= 0)
			iMissionCost = iBaseMissionCost;
	}

	// Make city unhappy
	else if (kMission.getCityUnhappinessCounter() > 0)
	{
		FAssert(pCity != NULL);
		// Cannot make a city unhappy if you've already done it (value is negative when active)
		if (pCity != NULL && pCity->getEspionageHappinessCounter() <= 0)
			iMissionCost = iBaseMissionCost;
	}

	// Make city Revolt
	else if (kMission.getCityRevoltCounter() > 0)
	{
		FAssert(pCity != NULL);
		// Cannot make a city revolt if it's already revolting
		if (pCity != NULL && pCity->getOccupationTimer() == 0)
			iMissionCost = iBaseMissionCost;
	}
	else if (kMission.getCounterespionageMod() > 0)
	{
		if (GET_TEAM(getTeam()).getCounterespionageTurnsLeftAgainstTeam(
			GET_PLAYER(eTargetPlayer).getTeam()) <= 0)
		{
			iMissionCost = (iBaseMissionCost * kSpeed.getResearchPercent()) / 100;
		}
	}
	else if (kMission.getPlayerAnarchyCounter() > 0)
	{
		/*	Player anarchy timer: can't add more turns of anarchy
			to player already in the midst of it */
		if (!GET_PLAYER(eTargetPlayer).isAnarchy())
			iMissionCost = (iBaseMissionCost * kSpeed.getAnarchyPercent()) / 100;
	}
	else if (kMission.isPassive())
	{
		iMissionCost = (iBaseMissionCost *
				(100 + GET_TEAM(GET_PLAYER(eTargetPlayer).getTeam()).
				getEspionagePointsAgainstTeam(getTeam()))) / 100;
	}
	else iMissionCost = (iBaseMissionCost * kSpeed.getResearchPercent()) / 100;

	if (iMissionCost < 0)
		return -1;

	return iMissionCost;
}

/*	K-Mod. I've altered this function to give a generic answer
	when NO_ESPIONAGEMISSION is passed. */
int CvPlayer::getEspionageMissionCostModifier(EspionageMissionTypes eMission,
	PlayerTypes eTargetPlayer, const CvPlot* pPlot, int iExtraData,
	const CvUnit* pSpyUnit) const
{
	// <advc.opt>
	static int const iESPIONAGE_CITY_POP_EACH_MOD = GC.getDefineINT("ESPIONAGE_CITY_POP_EACH_MOD");
	static int const iESPIONAGE_CITY_TRADE_ROUTE_MOD = GC.getDefineINT("ESPIONAGE_CITY_TRADE_ROUTE_MOD");
	static int const iESPIONAGE_CITY_RELIGION_STATE_MOD = GC.getDefineINT("ESPIONAGE_CITY_RELIGION_STATE_MOD");
	static int const iESPIONAGE_CITY_HOLY_CITY_MOD = GC.getDefineINT("ESPIONAGE_CITY_HOLY_CITY_MOD");
	static int const iESPIONAGE_CULTURE_MULTIPLIER_MOD = GC.getDefineINT("ESPIONAGE_CULTURE_MULTIPLIER_MOD");
	static int const iESPIONAGE_DISTANCE_MULTIPLIER_MOD = GC.getDefineINT("ESPIONAGE_DISTANCE_MULTIPLIER_MOD");
	static int const iESPIONAGE_EACH_TURN_UNIT_COST_DECREASE = GC.getDefineINT("ESPIONAGE_EACH_TURN_UNIT_COST_DECREASE");
	// </advc.opt>

	int iModifier = 100;

	CvCity const* pCity = (pPlot == NULL ? NULL : pPlot->getPlotCity());

	if (eTargetPlayer == NO_PLAYER)
		eTargetPlayer = getID();

	// (moved from the bottom of the function)
	CvTeam const& kTargetTeam = GET_TEAM(eTargetPlayer);

	//if (pCity != NULL && kMission.isTargetsCity())
	if (pCity != NULL &&
		(eMission == NO_ESPIONAGEMISSION || GC.getInfo(eMission).isTargetsCity()))
	{
		// City Population
		iModifier *= 100 + (iESPIONAGE_CITY_POP_EACH_MOD *
				(pCity->getPopulation() - 1));
		iModifier /= 100;

		// Trade Route
		if (pCity->isTradeRoute(getID()))
		{
			iModifier *= 100 + iESPIONAGE_CITY_TRADE_ROUTE_MOD;
			iModifier /= 100;
		}

		ReligionTypes eReligion = getStateReligion();
		if (eReligion != NO_RELIGION)
		{
			int iReligionModifier = 0;

			// City has Your State Religion
			if (pCity->isHasReligion(eReligion))
			{
				if (GET_PLAYER(eTargetPlayer).getStateReligion() != eReligion)
					iReligionModifier += iESPIONAGE_CITY_RELIGION_STATE_MOD;

				if (hasHolyCity(eReligion))
					iReligionModifier += iESPIONAGE_CITY_HOLY_CITY_MOD;
			}

			iModifier *= 100 + iReligionModifier;
			iModifier /= 100;

		}

		// City's culture affects cost
		/*iModifier *= 100 - (pCity->getCultureTimes100(getID()) * GC.getDefineINT("ESPIONAGE_CULTURE_MULTIPLIER_MOD")) / std::max(1, pCity->getCultureTimes100(eTargetPlayer) + pCity->getCultureTimes100(getID()));
		iModifier /= 100;*/ // BtS

		iModifier *= 100 + pCity->getEspionageDefenseModifier();
		iModifier /= 100;
	}

	if (pPlot != NULL)
	{
		// K-Mod. Culture Mod. (Based on plot culture rather than city culture.)
		if (eMission == NO_ESPIONAGEMISSION || GC.getInfo(eMission).isSelectPlot() ||
			GC.getInfo(eMission).isTargetsCity())
		{
			iModifier *= 100 - (pPlot->getCulture(getID()) *
					iESPIONAGE_CULTURE_MULTIPLIER_MOD) /
					std::max(1, pPlot->getCulture(eTargetPlayer) +
					pPlot->getCulture(getID()));
			iModifier /= 100;
		}
		// K-Mod end

		// Distance mod
		int const iMaxPlotDistance = GC.getMap().maxTypicalDistance(); // advc.140: was maxPlotDistance
		int iDistance = iMaxPlotDistance;

		CvCity const* pOurCapital = getCapitalCity();
		if (pOurCapital != NULL)
		{
			//if (kMission.isSelectPlot() || kMission.isTargetsCity())
			if (eMission == NO_ESPIONAGEMISSION ||
				GC.getInfo(eMission).isSelectPlot() ||
				GC.getInfo(eMission).isTargetsCity())
			{
				iDistance = ::plotDistance(pOurCapital->plot(), pPlot);
			}
			else
			{
				CvCity const* pTheirCapital = GET_PLAYER(eTargetPlayer).getCapitalCity();
				if (pTheirCapital != NULL)
				{
					iDistance = ::plotDistance(
							pOurCapital->plot(), pTheirCapital->plot());
				}
			}
		}

		iModifier *= (iDistance + iMaxPlotDistance) *
				iESPIONAGE_DISTANCE_MULTIPLIER_MOD / iMaxPlotDistance;
		iModifier /= 100;
	}

	// Spy presence mission cost alteration
	if (pSpyUnit != NULL)
	{
		iModifier *= 100 - pSpyUnit->getFortifyTurns() *
				iESPIONAGE_EACH_TURN_UNIT_COST_DECREASE;
		iModifier /= 100;
	}

	// My points VS. Your points to mod cost
	/*int iTargetPoints = kTargetTeam.getEspionagePointsEver();
	int iOurPoints = GET_TEAM(getTeam()).getEspionagePointsEver();
		iModifier *= (GC.getDefineINT("ESPIONAGE_SPENDING_MULTIPLIER") * (2 * iTargetPoints + iOurPoints)) / std::max(1, iTargetPoints + 2 * iOurPoints);
		iModifier /= 100;
	}*/ // BtS
	// K-Mod. use the dedicated function that exists for this modifier, for consistency.
	iModifier *= GET_TEAM(getTeam()).getEspionageModifier(kTargetTeam.getID());
	iModifier /= 100;
	// K-Mod end

	// Counterespionage Mission Mod
	/* if (kTargetTeam.getCounterespionageModAgainstTeam(getTeam()) > 0) {
		iModifier *= kTargetTeam.getCounterespionageModAgainstTeam(getTeam());
		iModifier /= 100;
	} */
	// K-Mod
	iModifier *= 100 + std::max(-100, kTargetTeam.getCounterespionageModAgainstTeam(getTeam()));
	iModifier /= 100;
	// K-Mod end

	return iModifier;
}


bool CvPlayer::doEspionageMission(EspionageMissionTypes eMission, PlayerTypes eTargetPlayer,
	CvPlot* pPlot, int iExtraData, CvUnit* pSpyUnit)
{
	if (!canDoEspionageMission(eMission, eTargetPlayer, pPlot, iExtraData, pSpyUnit))
		return false;

	bool bSomethingHappened = false;
	bool bAggressiveMission = true; // advc.120
	bool bShowExplosion = false;

	CvEspionageMissionInfo const& kMission = GC.getInfo(eMission);
	CvWString szBuffer;
	TeamTypes const eTargetTeam = (eTargetPlayer == NO_PLAYER ?
			NO_TEAM : TEAMID(eTargetPlayer));
	CvCity* pCity = (pPlot == NULL ? NULL : pPlot->getPlotCity()); // advc

	if (kMission.isDestroyImprovement() && pPlot != NULL)
	{
		if (pPlot->isImproved())
		{
			szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_SOMETHING_DESTROYED",
					GC.getInfo(pPlot->getImprovementType()).getDescription()).GetCString();
			pPlot->setImprovementType(GC.getInfo(pPlot->getImprovementType()).getImprovementPillage());
			bSomethingHappened = true;
		}
		else if (pPlot->isRoute())
		{
			szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_SOMETHING_DESTROYED",
					GC.getInfo(pPlot->getRouteType()).getDescription()).GetCString();
			pPlot->setRouteType(NO_ROUTE, true);
			bSomethingHappened = true;
		}
		if (bSomethingHappened)
			bShowExplosion = true;
	}

	if (kMission.getDestroyBuildingCostFactor() > 0 && pCity != NULL)
	{
		BuildingTypes const eTargetBuilding = (BuildingTypes)iExtraData;
		szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_SOMETHING_DESTROYED_IN",
				GC.getInfo(eTargetBuilding).getDescription(), pCity->getNameKey()).GetCString();
		pCity->setNumRealBuilding(eTargetBuilding,
				pCity->getNumRealBuilding(eTargetBuilding) - 1);
		bSomethingHappened = true;
		bShowExplosion = true;
		// K-Mod
		if (!isHuman() || pCity->isProductionAutomated())
			pCity->setChooseProductionDirty(true); // K-Mod end
	}

	if (kMission.getDestroyProjectCostFactor() > 0 && pCity != NULL)
	{
		ProjectTypes const eTargetProject = (ProjectTypes)iExtraData;
		szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_SOMETHING_DESTROYED_IN",
				GC.getInfo(eTargetProject).getDescription(), pCity->getNameKey()).GetCString();
		GET_TEAM(eTargetTeam).changeProjectCount(eTargetProject, -1);
		bSomethingHappened = true;
		bShowExplosion = true;
	}

	if (kMission.getDestroyProductionCostFactor() > 0 && pCity != NULL)
	{
		szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_PRODUCTION_DESTROYED_IN",
				pCity->getProductionName(), pCity->getNameKey());
		pCity->setProduction(0);
		bSomethingHappened = true;
		bShowExplosion = true;
		// K-Mod
		if (!isHuman()) // not for automated cities
			pCity->setChooseProductionDirty(true); // K-Mod end
	}

	if (kMission.getDestroyUnitCostFactor() > 0 && eTargetPlayer != NO_PLAYER)
	{
		int iTargetUnitID = iExtraData;
		CvUnit* pUnit = GET_PLAYER(eTargetPlayer).getUnit(iTargetUnitID);
		if (pUnit != NULL)
		{
			FAssert(pUnit->atPlot(pPlot));
			szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_SOMETHING_DESTROYED",
					pUnit->getNameKey()).GetCString();
			pUnit->kill(false, getID());
			bSomethingHappened = true;
			bShowExplosion = true;
		}
	}

	if (kMission.getBuyUnitCostFactor() > 0 && eTargetPlayer != NO_PLAYER)
	{
		int iTargetUnitID = iExtraData;
		CvUnit* pUnit = GET_PLAYER(eTargetPlayer).getUnit(iTargetUnitID);
		if (pUnit != NULL)
		{
			FAssert(pUnit->atPlot(pPlot));
			szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_UNIT_BOUGHT",
					pUnit->getNameKey()).GetCString();
			UnitTypes eUnitType = pUnit->getUnitType();
			int iX = pUnit->getX();
			int iY = pUnit->getY();
			pUnit->kill(false, getID());
			initUnit(eUnitType, iX, iY, NO_UNITAI);
			bSomethingHappened = true;
		}
	}

	if (kMission.getBuyCityCostFactor() > 0 && pCity != NULL)
	{
		szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_CITY_BOUGHT",
				pCity->getNameKey()).GetCString();
		acquireCity(pCity, false, true, true);
		bSomethingHappened = true;
	}


	if (kMission.getCityInsertCultureCostFactor() > 0 && pCity != NULL)
	{
		szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_CITY_CULTURE_INSERTED",
				pCity->getNameKey()).GetCString();
		/*int iCultureAmount = kMission.getCityInsertCultureAmountFactor() * pCity->countTotalCultureTimes100();
		iCultureAmount /= 10000;
		iCultureAmount = std::max(1, iCultureAmount);
		int iNumTurnsApplied = (GC.getDefineINT("GREAT_WORKS_CULTURE_TURNS") * GC.getInfo(GC.getGame().getGameSpeedType()).getUnitGreatWorkPercent()) / 100;
		for (int i = 0; i < iNumTurnsApplied; ++i)
			pCity->changeCulture(getID(), iCultureAmount / iNumTurnsApplied, true, true);
		if (iNumTurnsApplied > 0)
			pCity->changeCulture(getID(), iCultureAmount % iNumTurnsApplied, false, true);
		}*/ // BtS
		// K-Mod. apply culture in one hit. We don't need fake 'free city culture' anymore.
		int iCultureTimes100 = std::max(1, kMission.getCityInsertCultureAmountFactor() *
				pCity->countTotalCultureTimes100() / 100);
		//pCity->changeCultureTimes100(getID(), iCultureTimes100, true, true);
		// plot culture only
		pCity->doPlotCultureTimes100(true, getID(), iCultureTimes100, false);
		// K-Mod end
		bSomethingHappened = true;
	}

	if (kMission.getCityPoisonWaterCounter() > 0 && pCity != NULL)
	{
		szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_CITY_POISONED",
				pCity->getNameKey()).GetCString();
		pCity->changeEspionageHealthCounter(kMission.getCityPoisonWaterCounter());
		bShowExplosion = true;
		bSomethingHappened = true;
	}

	if (kMission.getCityUnhappinessCounter() > 0 && pCity != NULL)
	{
		szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_CITY_UNHAPPY",
				pCity->getNameKey()).GetCString();
		pCity->changeEspionageHappinessCounter(kMission.getCityUnhappinessCounter());
		bShowExplosion = true;
		bSomethingHappened = true;
	}

	if (kMission.getCityRevoltCounter() > 0 && pCity != NULL)
	{
		szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_CITY_REVOLT",
				pCity->getNameKey()).GetCString();
		pCity->changeCultureUpdateTimer(kMission.getCityRevoltCounter());
		pCity->changeOccupationTimer(kMission.getCityRevoltCounter());
		bSomethingHappened = true;
		bShowExplosion = true;
		if (gUnitLogLevel >= 2 && !isHuman()) logBBAI("      Spy for player %d (%S) causes revolt in %S, owned by %S (%d)", getID(), getCivilizationDescription(0), pCity->getName().GetCString(), GET_PLAYER(pCity->getOwner()).getCivilizationDescription(0), pCity->getOwner());
	}

	if (kMission.getStealTreasuryTypes() > 0 && pCity != NULL)
	{	/*	<advc> Part of this branch was executed for eTargetPlayer=NO_PLAYER.
			I don't think that can happen. */
		FAssert(eTargetPlayer == pCity->getOwner());
		if (eTargetPlayer == NO_PLAYER)
			eTargetPlayer = pCity->getOwner(); // </advc>
		//int iNumTotalGold = (GET_PLAYER(eTargetPlayer).getGold() * kMission.getStealTreasuryTypes()) / 100;
		int iNumTotalGold /* kmodx: */ = 0;
		/* iNumTotalGold *= pCity->getPopulation();
		iNumTotalGold /= std::max(1, GET_PLAYER(eTargetPlayer).getTotalPopulation()); */
		// K-Mod. Make stealing gold still worthwhile against large civs.
		iNumTotalGold = getEspionageGoldQuantity(eMission, eTargetPlayer, pCity);
		szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_STEAL_TREASURY",
				iNumTotalGold); // advc.004i
		changeGold(iNumTotalGold);
		GET_PLAYER(eTargetPlayer).changeGold(-iNumTotalGold);
		bSomethingHappened = true;
	}

	if (kMission.getBuyTechCostFactor() > 0) // "Steal" Tech
	{
		TechTypes eTech = (TechTypes)iExtraData;
		szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_TECH_BOUGHT",
				GC.getInfo(eTech).getDescription()).GetCString();
		GET_TEAM(getTeam()).setHasTech(eTech, true, getID(), false, true);
		if(isSignificantDiscovery(eTech)) // advc.550e
			GET_TEAM(getTeam()).setNoTradeTech(eTech, true);
		bSomethingHappened = true;
	}

	if (kMission.getSwitchCivicCostFactor() > 0 && eTargetPlayer != NO_PLAYER)
	{
		announceEspionageToThirdParties(eMission, eTargetPlayer); // advc.120f
		CivicTypes eCivic = (CivicTypes)iExtraData;
		szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_SWITCH_CIVIC",
				GC.getInfo(eCivic).getDescription()).GetCString();
		GET_PLAYER(eTargetPlayer).setCivics(
				GC.getInfo(eCivic).getCivicOptionType(), eCivic);
		// advc: Revolution turns calculation moved into auxiliary function
		GET_PLAYER(eTargetPlayer).setRevolutionTimer(getMinTurnsBetweenRevolutions());
		bSomethingHappened = true;
	}

	if (kMission.getSwitchReligionCostFactor() > 0 && eTargetPlayer != NO_PLAYER)
	{
		announceEspionageToThirdParties(eMission, eTargetPlayer); // advc.120f
		ReligionTypes eReligion = (ReligionTypes)iExtraData;
		szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_SWITCH_RELIGION",
				GC.getInfo(eReligion).getDescription()).GetCString();
		GET_PLAYER(eTargetPlayer).setLastStateReligion(eReligion);
		GET_PLAYER(eTargetPlayer).setConversionTimer(std::max(1,
				((100 + GET_PLAYER(eTargetPlayer).getAnarchyModifier()) *
				GC.getDefineINT("MIN_CONVERSION_TURNS")) / 100));
		bSomethingHappened = true;
	}

	if (kMission.getPlayerAnarchyCounter() > 0 && eTargetPlayer != NO_PLAYER)
	{
		int iTurns = (kMission.getPlayerAnarchyCounter() *
				GC.getInfo(GC.getGame().getGameSpeedType()).getAnarchyPercent()) / 100;
		szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_PLAYER_ANARCHY").GetCString();
		GET_PLAYER(eTargetPlayer).changeAnarchyTurns(iTurns);
		bSomethingHappened = true;
	}

	if (kMission.getCounterespionageNumTurns() > 0 && eTargetTeam != NO_TEAM &&
		kMission.getCounterespionageMod() > 0)
	{
		szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_COUNTERESPIONAGE").GetCString();
		int iTurns = (kMission.getCounterespionageNumTurns() *
				GC.getInfo(GC.getGame().getGameSpeedType()).getResearchPercent()) / 100;
		GET_TEAM(getTeam()).changeCounterespionageTurnsLeftAgainstTeam(eTargetTeam, iTurns);
		GET_TEAM(getTeam()).changeCounterespionageModAgainstTeam(
				eTargetTeam, kMission.getCounterespionageMod());
		// <advc.120>
		if(!bSomethingHappened)
			bAggressiveMission = false; // </advc.120>
		bSomethingHappened = true;
	}

	// <advc.103>
	if(kMission.isInvestigateCity() && pCity != NULL)
	{
		pCity->setInvestigate(true);
		gDLL->UI().selectCity(pCity);
		bSomethingHappened = true;
	} // </advc.103>

	int const iMissionCost = getEspionageMissionCost(eMission,
			eTargetPlayer, pPlot, iExtraData, pSpyUnit);

	int iHave = 0;
	if (eTargetTeam != NO_TEAM)
	{
		iHave = GET_TEAM(getTeam()).getEspionagePointsAgainstTeam(eTargetTeam);
		if (bSomethingHappened)
		{
			GET_TEAM(getTeam()).changeEspionagePointsAgainstTeam(
					eTargetTeam, -iMissionCost);
		}
	}

	if (bShowExplosion && pPlot != NULL)
	{
		if (pPlot->isVisible(GC.getGame().getActiveTeam()) &&
			GC.getGame().getActiveTeam() == getTeam()) // advc.120i
		{
			EffectTypes eEffect = GC.getInfo(GC.getInfo(MISSION_BOMBARD).
					getEntityEvent()).getEffectType();
			gDLL->getEngineIFace()->TriggerEffect(
					eEffect, pPlot->getPoint(), (float)(GC.getASyncRand().get(360)));
			gDLL->UI().playGeneralSound("AS3D_UN_CITY_EXPLOSION", pPlot->getPoint());
		}
	}

	if (bSomethingHappened)
	{
		int iX = -1;
		int iY = -1;
		if (pPlot != NULL)
		{
			iX = pPlot->getX();
			iY = pPlot->getY();
		}
		// advc.103: The city screen having opened is confirmation enough
		if (!kMission.isInvestigateCity())
		{
			gDLL->UI().addMessage(getID(), true, -1,
					gDLL->getText("TXT_KEY_ESPIONAGE_MISSION_PERFORMED"),
					"AS2D_POSITIVE_DINK", MESSAGE_TYPE_INFO,
					ARTFILEMGR.getInterfaceArtPath("ESPIONAGE_BUTTON"),
					GC.getColorType("GREEN"), iX, iY, true, true);
		}
	}
	else if (getID() == GC.getGame().getActivePlayer())
	{
		CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_TEXT);
		if (iHave < iMissionCost)
		{
			pInfo->setText(gDLL->getText("TXT_KEY_ESPIONAGE_TOO_EXPENSIVE",
					iMissionCost, iHave));
		}
		else pInfo->setText(gDLL->getText("TXT_KEY_ESPIONAGE_CANNOT_DO_MISSION"));
		addPopup(pInfo);
	}

	if (bSomethingHappened && !szBuffer.empty())
	{
		int iX = INVALID_PLOT_COORD;
		int iY = INVALID_PLOT_COORD;
		if (pPlot != NULL)
		{
			iX = pPlot->getX();
			iY = pPlot->getY();
		}
		if (eTargetPlayer != NO_PLAYER)
		{	// <advc.120>
			ColorTypes eColor = NO_COLOR;
			if(bAggressiveMission)
				eColor = GC.getColorType("RED");
			// </advc.120>
			gDLL->UI().addMessage(eTargetPlayer, /*true*/ false, // advc.120i
					-1, szBuffer, "AS2D_DEAL_CANCELLED", MESSAGE_TYPE_INFO,
					ARTFILEMGR.getInterfaceArtPath("ESPIONAGE_BUTTON"),
					eColor, iX, iY, true, true);
		}
	}

	return bSomethingHappened;
}


void CvPlayer::setEspionageSpendingWeightAgainstTeam(TeamTypes eTeam, int iValue)
{
	//FAssert(iValue >= 0);
	iValue = std::min(std::max(0, iValue), 99);
	if (iValue != getEspionageSpendingWeightAgainstTeam(eTeam))
	{
		m_aiEspionageSpendingWeightAgainstTeam.set(eTeam, iValue);
		gDLL->UI().setDirty(Espionage_Advisor_DIRTY_BIT, true);
	}
}


void CvPlayer::doAdvancedStartAction(AdvancedStartActionTypes eAction, int iX, int iY, int iData, bool bAdd,
	int iData2) // advc.250c
{
	if (getAdvancedStartPoints() < 0)
		return;

	CvPlot* pPlot = GC.getMap().plot(iX, iY);

	if (getNumCities() <= 0)
	{
		switch (eAction)
		{
		case ADVANCEDSTARTACTION_EXIT:
			//Try to build this player's empire
			if (getID() == GC.getGame().getActivePlayer())
				gDLL->UI().setBusy(true);
			AI().AI_doAdvancedStart(true);
			if (getID() == GC.getGame().getActivePlayer())
				gDLL->UI().setBusy(false);
			break;
		case ADVANCEDSTARTACTION_AUTOMATE:
		case ADVANCEDSTARTACTION_CITY:
			break;
		default:
			// The first action must be to place a city
			// so players can't lose by spending everything
			return;
		}
	}

	CvCity* pCity = (pPlot == NULL ? NULL : pPlot->getPlotCity()); // advc

	switch (eAction)
	{
	case ADVANCEDSTARTACTION_EXIT:
		changeGold(getAdvancedStartPoints());
		setAdvancedStartPoints(-1);
		if (GC.getGame().getActivePlayer() == getID())
			gDLL->UI().setInAdvancedStart(false);

		if (isHuman())
		{
			FOR_EACH_CITY_VAR(pLoopCity, *this)
				pLoopCity->chooseProduction();

			chooseTech();

			if (canDoAnyRevolution())
			{
				CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_CHANGECIVIC);
				if (pInfo != NULL)
					gDLL->UI().addPopup(pInfo, getID());
			}
		}
		break;
	case ADVANCEDSTARTACTION_AUTOMATE:
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setBusy(true);
		AI().AI_doAdvancedStart(true);
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setBusy(false);
		break;
	case ADVANCEDSTARTACTION_UNIT:
	{
		if(pPlot == NULL)
			return;
		UnitTypes const eUnit = (UnitTypes)iData;
		int iCost = getAdvancedStartUnitCost(eUnit, bAdd, pPlot);
		if (bAdd && iCost < 0)
			return;

		if (bAdd) // Add unit to the map
		{
			if (getAdvancedStartPoints() >= iCost)
			{
				CvUnit* pUnit = initUnit(eUnit, iX, iY,
						(UnitAITypes)iData2); // advc.250c
				if (pUnit != NULL)
				{
					pUnit->finishMoves();
					changeAdvancedStartPoints(-iCost);
				}
			}
		}
		else // Remove unit from the map
		{
			// If cost is -1 we already know this unit isn't present
			if (iCost != -1)
			{
				FOR_EACH_UNIT_VAR_IN(pUnit, *pPlot)
				{
					if (pUnit->getUnitType() == eUnit)
					{
						pUnit->kill(false);
						changeAdvancedStartPoints(iCost);
						return;
					}
				}
			}
			{
			// Proper unit not found above, delete first found.
				CLLNode<IDInfo>* pNode = pPlot->headUnitNode();
				if (pNode != NULL)
				{
					CvUnit* pUnit = ::getUnit(pNode->m_data);
					iCost = getAdvancedStartUnitCost(pUnit->getUnitType(), false,
							pPlot); // kekm.11
					FAssertMsg(iCost != -1, "Trying to delete a unit which shouldn't exist");
					pUnit->kill(false);
					changeAdvancedStartPoints(iCost);
				}
			}
		}
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(Advanced_Start_DIRTY_BIT, true);
		break;
	}
	case ADVANCEDSTARTACTION_CITY:
	{
		if (pPlot == NULL)
			return;
		int const iCost = getAdvancedStartCityCost(bAdd, pPlot);
		if (iCost < 0)
			return;

		if (bAdd) // Add City to the map
		{
			if (getNumCities() <= 0)
			{
				PlayerTypes eClosestPlayer = NO_PLAYER;
				int iMinDistance = MAX_INT;
				for (MemberIter itMember(getTeam()); itMember.hasNext(); ++itMember)
				{
					if (itMember->getNumCities() > 0)
						continue;
					int iDistance = plotDistance(iX, iY,
							itMember->getStartingPlot()->getX(),
							itMember->getStartingPlot()->getY());
					if (iDistance < iMinDistance)
					{
						eClosestPlayer = itMember->getID();
						iMinDistance = iDistance;
					}
				}
				FAssertMsg(eClosestPlayer != NO_PLAYER, "Self at a minimum should always be valid");
				if (eClosestPlayer != getID())
				{
					CvPlot* pTempPlot = GET_PLAYER(eClosestPlayer).getStartingPlot();
					GET_PLAYER(eClosestPlayer).setStartingPlot(getStartingPlot());
					setStartingPlot(pTempPlot);
				}
			}
			if (getAdvancedStartPoints() >= iCost || getNumCities() <= 0)
			{
				found(iX, iY);
				changeAdvancedStartPoints(-std::min(iCost, getAdvancedStartPoints()));
				GC.getGame().updateColoredPlots();
				// advc.250c: Commented out
				/*if (pCity != NULL) {
					if (pCity->getPopulation() > 1)
						pCity->setFood(pCity->growthThreshold() / 2);
				}*/
			}
		}
		else // Remove City from the map
		{
			pPlot->setRouteType(NO_ROUTE, true);
			pPlot->getPlotCity()->kill(true);
			pPlot->setImprovementType(NO_IMPROVEMENT);
			changeAdvancedStartPoints(iCost);
		}
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(Advanced_Start_DIRTY_BIT, true);
		break;
	}
	case ADVANCEDSTARTACTION_POP:
	{
		if (pCity == NULL)
			return;
		int const iCost = getAdvancedStartPopCost(bAdd, pCity);
		if (iCost < 0)
			return;

		bool bPopChanged = false;
		if (bAdd)
		{
			if (getAdvancedStartPoints() >= iCost)
			{
				pCity->changePopulation(1);
				changeAdvancedStartPoints(-iCost);
				bPopChanged = true;
			}
		}
		else
		{
			pCity->changePopulation(-1);
			changeAdvancedStartPoints(iCost);
			bPopChanged = true;
		}
		if (bPopChanged)
		{
			pCity->setHighestPopulation(pCity->getPopulation());
			// advc.250c, kekm: Commented out
			/*if (pCity->getPopulation() == 1) {
				pCity->setFood(0);
				pCity->setFoodKept(0);
			}
			else if (pCity->getPopulation() > 1) {
				pCity->setFood(pCity->growthThreshold() / 2);
				pCity->setFoodKept((pCity->getFood() * pCity->getMaxFoodKeptPercent()) / 100);
			}*/
		}
		break;
	}	
	case ADVANCEDSTARTACTION_CULTURE:
	{
		if (pCity == NULL)
			return;
		int const iCost = getAdvancedStartCultureCost(bAdd, pCity);
		if (iCost < 0)
			return;

		if (bAdd) // Add Culture to the City
		{
			if (getAdvancedStartPoints() >= iCost)
			{
				pCity->setCulture(getID(), pCity->getCultureThreshold(), true, true);
				changeAdvancedStartPoints(-iCost);
			}
		}	
		else // Remove Culture from the city
		{
			CultureLevelTypes eLevel = (CultureLevelTypes)std::max(0,
					pCity->getCultureLevel() - 1);
			pCity->setCulture(getID(), CvCity::getCultureThreshold(eLevel), true, true);
			changeAdvancedStartPoints(iCost);
		}
		break;
	}	
	case ADVANCEDSTARTACTION_BUILDING:
	{
		if (pCity == NULL)
			return;
		BuildingTypes const eBuilding = (BuildingTypes)iData;
		int const iCost = getAdvancedStartBuildingCost(eBuilding, bAdd, pCity);
		if (iCost < 0)
			return;

		if (bAdd) // Add Building to the City
		{
			if (getAdvancedStartPoints() >= iCost)
			{
				pCity->setNumRealBuilding(eBuilding, pCity->getNumRealBuilding(eBuilding)+1);
				changeAdvancedStartPoints(-iCost);
				//if (GC.getInfo(eBuilding).getFoodKept() != 0)
				if (getFoodKept(eBuilding) != 0) // advc.912d
				{
					pCity->setFoodKept((pCity->getFood() *
							pCity->getMaxFoodKeptPercent()) / 100);
				}
			}
		}
		else // Remove Building from the map
		{
			pCity->setNumRealBuilding(eBuilding, pCity->getNumRealBuilding(eBuilding) - 1);
			changeAdvancedStartPoints(iCost);
			//if (GC.getInfo(eBuilding).getFoodKept() != 0)
			if (getFoodKept(eBuilding) != 0) // advc.912d
			{
				pCity->setFoodKept((pCity->getFood() *
						pCity->getMaxFoodKeptPercent()) / 100);
			}
		}
		/*	advc (note): This used to be executed even for pCity==NULL,
			but I don't think that could've occurred. */
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(Advanced_Start_DIRTY_BIT, true);
		break;
	}
	case ADVANCEDSTARTACTION_ROUTE:
	{
		if(pPlot == NULL)
			return;
		RouteTypes eRoute = (RouteTypes)iData;
		int iCost = getAdvancedStartRouteCost(eRoute, bAdd, pPlot);
		if (bAdd && iCost < 0)
			return;

		if (bAdd) // Add Route to the plot
		{
			if (getAdvancedStartPoints() >= iCost)
			{
				pPlot->setRouteType(eRoute, true);
				changeAdvancedStartPoints(-iCost);
			}
		}
		else // Remove Route from the Plot
		{
			if (pPlot->getRouteType() != eRoute)
			{
				eRoute = pPlot->getRouteType();
				iCost = getAdvancedStartRouteCost(eRoute, bAdd);
			}
			if (iCost < 0)
				return;
			pPlot->setRouteType(NO_ROUTE, true);
			changeAdvancedStartPoints(iCost);
		}
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(Advanced_Start_DIRTY_BIT, true);
		break;
	}
	case ADVANCEDSTARTACTION_IMPROVEMENT:
	{
		if(pPlot == NULL)
			return;
		ImprovementTypes eImprovement = (ImprovementTypes) iData;
		int iCost = getAdvancedStartImprovementCost(eImprovement, bAdd, pPlot);
		if (bAdd && iCost < 0)
			return;

		if (bAdd) // Add Improvement to the plot
		{
			if (getAdvancedStartPoints() >= iCost && pPlot->isFeature())
			{
				FOR_EACH_ENUM(Build)
				{
					ImprovementTypes eLoopImprovement = GC.getInfo(eLoopBuild).
							getImprovement();
					if (eImprovement != eLoopImprovement)
						continue;
					if (GC.getInfo(eLoopBuild).isFeatureRemove(pPlot->getFeatureType()) &&
						canBuild(*pPlot, eLoopBuild))
					{
						pPlot->setFeatureType(NO_FEATURE);
						break;
					}
				}
				pPlot->setImprovementType(eImprovement);
				changeAdvancedStartPoints(-iCost);
			}
		}
		else // Remove Improvement from the Plot
		{
			if (pPlot->getImprovementType() != eImprovement)
			{
				eImprovement = pPlot->getImprovementType();
				iCost = getAdvancedStartImprovementCost(eImprovement, bAdd, pPlot);
			}
			if (iCost < 0)
				return;
			pPlot->setImprovementType(NO_IMPROVEMENT);
			changeAdvancedStartPoints(iCost);
		}
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(Advanced_Start_DIRTY_BIT, true);
		break;
	}
	case ADVANCEDSTARTACTION_TECH:
	{
		TechTypes const eTech = (TechTypes)iData;
		int const iCost = getAdvancedStartTechCost(eTech, bAdd);
		if (iCost < 0)
			return;

		if (bAdd) // Add Tech to team
		{
			if (getAdvancedStartPoints() >= iCost)
			{
				GET_TEAM(getTeam()).setHasTech(eTech, true, getID(), false, false);
				changeAdvancedStartPoints(-iCost);
			}
		}
		else // Remove Tech from the Team
		{
			GET_TEAM(getTeam()).setHasTech(eTech, false, getID(), false, false);
			changeAdvancedStartPoints(iCost);
		}
		if (getID() == GC.getGame().getActivePlayer())
			gDLL->UI().setDirty(Advanced_Start_DIRTY_BIT, true);
		break;
	}
	case ADVANCEDSTARTACTION_VISIBILITY:
	{
		if(pPlot == NULL)
			return;
		int const iCost = getAdvancedStartVisibilityCost(bAdd, pPlot);
		if (iCost < 0)
			return;

		if (bAdd) // Add Visibility to the plot
		{
			if (getAdvancedStartPoints() >= iCost)
			{
				pPlot->setRevealed(getTeam(), true, true, NO_TEAM, true);
				changeAdvancedStartPoints(-iCost);
			}
		}
		else // Remove Visibility from the Plot
		{
			pPlot->setRevealed(getTeam(), false, true, NO_TEAM, true);
			changeAdvancedStartPoints(iCost);
		}
		break;
	}
	default:
		FAssert(false);
	}
}

/*	advc.250c: (Should perhaps be moved to CvGame;
	though it's more convenient to use as it is) */
namespace
{
	int adjustAdvStartPtsToSpeed(int iPoints)
	{
		iPoints *= 100;
		return std::max(0, iPoints / GC.getInfo(GC.getGame().getGameSpeedType()).
				getGrowthPercent());
	}
}

// Adding or removing a unit
int CvPlayer::getAdvancedStartUnitCost(UnitTypes eUnit, bool bAdd, CvPlot const* pPlot) const
{
	if (getNumCities() <= 0)
		return -1;

	int iCost = (getProductionNeeded(eUnit) *
			//GC.getInfo(eUnit).getAdvancedStartCost() / 100;
			// advc.250c:
			3 * GC.getInfo(eUnit).getAdvancedStartCost()) / 200;

	if (iCost < 0)
		return -1;

	if (pPlot == NULL)
	{
		if (bAdd)
		{
			bool bValid = false;
			FOR_EACH_CITY(pLoopCity, *this)
			{
				if (pLoopCity->canTrain(eUnit))
				{
					bValid = true;
					break;
				}
			}
			if (!bValid)
				return -1;
		}
	}
	else
	{
		CvCity* pCity = NULL;
		if (GC.getDefineINT("ADVANCED_START_ALLOW_UNITS_OUTSIDE_CITIES") == 0)
		{
			pCity = pPlot->getPlotCity();
			if (pCity == NULL || pCity->getOwner() != getID())
				return -1;

			iCost *= 100;
			iCost /= std::max(1, 100 + pCity->getProductionModifier(eUnit));
		}
		else
		{
			if (pPlot->getOwner() != getID())
				return -1;

			iCost *= 100;
			iCost /= std::max(1, 100 + getProductionModifier(eUnit));
		}


		if (bAdd)
		{
			int iMaxUnitsPerCity = GC.getDefineINT("ADVANCED_START_MAX_UNITS_PER_CITY");
			if (iMaxUnitsPerCity >= 0)
			{
				if (GC.getInfo(eUnit).isMilitarySupport() &&
					getNumMilitaryUnits() >= iMaxUnitsPerCity * getNumCities())
				{
					return -1;
				}
			}

			if (pCity != NULL)
			{
				if (!pCity->canTrain(eUnit))
					return -1;
			}
			else
			{
				if (!pPlot->canTrain(eUnit, false, false))
					return -1;

				if (pPlot->isImpassable() && !GC.getInfo(eUnit).canMoveImpassable())
					return -1;

				if (pPlot->isFeature())
				{
					if (GC.getInfo(eUnit).getFeatureImpassable(pPlot->getFeatureType()))
					{
						TechTypes eTech = GC.getInfo(eUnit).getFeaturePassableTech(
								pPlot->getFeatureType());
						if (eTech == NO_TECH || !GET_TEAM(getTeam()).isHasTech(eTech))
							return -1;
					}
				}
				else
				{
					if (GC.getInfo(eUnit).getTerrainImpassable(pPlot->getTerrainType()))
					{
						TechTypes eTech = GC.getInfo(eUnit).getTerrainPassableTech(pPlot->getTerrainType());
						if (eTech == NO_TECH || !GET_TEAM(getTeam()).isHasTech(eTech))
							return -1;
					}
				}
			}
		}
		else // Must be this unit at plot in order to remove
		{
			bool bUnitFound = false;
			FOR_EACH_UNIT_IN(pUnit, *pPlot)
			{
				if (pUnit->getUnitType() == eUnit)
					bUnitFound = true;
			}
			if (!bUnitFound)
				return -1;
		}
	}

	// Increase cost if the XML defines that additional units will cost more
	if (GC.getInfo(eUnit).getAdvancedStartCostIncrease() != 0)
	{
		int iUnits = 0;
		FOR_EACH_UNIT(pLoopUnit, *this)
		{
			if (pLoopUnit->getUnitType() == eUnit)
				iUnits++;
		}

		if (!bAdd)
			iUnits--;

		if (iUnits > 0)
		{
			iCost *= 100 + GC.getInfo(eUnit).getAdvancedStartCostIncrease() * iUnits;
			iCost /= 100;
		}
	}

	return adjustAdvStartPtsToSpeed(iCost); // advc.250c
}

// Adding or removing a City
int CvPlayer::getAdvancedStartCityCost(bool bAdd, CvPlot const* pPlot) const
{
	int iCost = getNewCityProductionValue();
	iCost = (fixp(1.5) * iCost).floor(); // advc.250c
	if (iCost < 0)
		return -1;

	if (pPlot != NULL)
	{
		// Need valid plot to found on if adding
		if (bAdd)
		{
			if (!canFound(pPlot->getX(), pPlot->getY(), false))
				return -1;
		}
		// Need your own city present to remove
		else
		{
			if (pPlot->isCity())
			{
				if (pPlot->getPlotCity()->getOwner() != getID())
					return -1;
			}
			else return -1;
		}

		// Is there a distance limit on how far a city can be placed from a player's start/another city?
		if (GC.getDefineINT("ADVANCED_START_CITY_PLACEMENT_MAX_RANGE") > 0)
		{
			static int const iADVANCED_START_CITY_PLACEMENT_MAX_RANGE = GC.getDefineINT("ADVANCED_START_CITY_PLACEMENT_MAX_RANGE"); // advc.opt
			PlayerTypes eClosestPlayer = NO_PLAYER;
			int iClosestDistance = MAX_INT;
			for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
			{
				CvPlot const* pStartingPlot = itPlayer->getStartingPlot();
				if (pStartingPlot == NULL)
					continue;
				int const iDistance = plotDistance(pPlot, pStartingPlot);
				if (iDistance <= iADVANCED_START_CITY_PLACEMENT_MAX_RANGE &&
					(iDistance < iClosestDistance ||
					// advc (note): I.e. break ties in favor of rivals
					(iDistance == iClosestDistance && itPlayer->getTeam() != getTeam())))
				{
					iClosestDistance = iDistance;
					eClosestPlayer = itPlayer->getID();
				}
			}
			if (eClosestPlayer == NO_PLAYER || TEAMID(eClosestPlayer) != getTeam())
				return -1;
			if (getID() != eClosestPlayer &&
				/*	Only allow founding a city at someone else's start point if
					We have no cities and they have no cities. */
				!(getNumCities() <= 0 && GET_PLAYER(eClosestPlayer).getNumCities() <= 0))
			{
				return -1;
			}
		}
	}

	// Increase cost if the XML defines that additional cities will cost more
	if (GC.getDefineINT("ADVANCED_START_CITY_COST_INCREASE") != 0)
	{
		int iCities = getNumCities();
		if (!bAdd)
			iCities--;
		if (iCities > 0)
		{
			iCost *= 100 + GC.getDefineINT("ADVANCED_START_CITY_COST_INCREASE") * iCities;
			iCost /= 100;
		}
	} // <advc.250c> <kekm>
	if (getNumCities() > 0)
	{
		FOR_EACH_ENUM(Unit)
		{
			if(GC.getInfo(eLoopUnit).isFound())
			{
				iCost *= 100;
				iCost /= std::max(1, 100 + getProductionModifier(eLoopUnit));
				break;
			}
		} // </kekm>
	}
	return adjustAdvStartPtsToSpeed(iCost); // </advc.250c>
}

// Adding or removing Population
int CvPlayer::getAdvancedStartPopCost(bool bAdd, CvCity const* pCity) const
{
	if (getNumCities() <= 0)
		return -1;
	if (pCity == NULL)
	{
		return adjustAdvStartPtsToSpeed( // advc.250c
				(getGrowthThreshold(1) *
				GC.getDefineINT("ADVANCED_START_POPULATION_COST")) / 100);
	}
	if (pCity->getOwner() != getID())
		return -1;

	int iPopulation = pCity->getPopulation();
	if (!bAdd) // Need to have Population to remove it
	{
		iPopulation--;
		if (iPopulation < GC.getDefineINT("INITIAL_CITY_POPULATION") +
			GC.getInfo(GC.getGame().getStartEra()).getFreePopulation())
		{
			return -1;
		}
	}

	int iCost = (getGrowthThreshold(iPopulation) *
			GC.getDefineINT("ADVANCED_START_POPULATION_COST")) / 100;

	// Increase cost if the XML defines that additional Pop will cost more
	if (GC.getDefineINT("ADVANCED_START_POPULATION_COST_INCREASE") != 0)
	{
		iPopulation--;
		if (iPopulation > 0)
		{
			iCost *= 100 + iPopulation *
					GC.getDefineINT("ADVANCED_START_POPULATION_COST_INCREASE");
			iCost /= 100;
		}
	}

	return adjustAdvStartPtsToSpeed(iCost); // advc.250c
}

// Adding or removing Culture
int CvPlayer::getAdvancedStartCultureCost(bool bAdd, CvCity const* pCity) const
{
	if (getNumCities() <= 0)
		return -1;
	int iCost = GC.getDefineINT("ADVANCED_START_CULTURE_COST");
	if (iCost < 0)
		return -1;
	if (pCity == NULL)
		return adjustAdvStartPtsToSpeed(iCost); // advc.250c
	if (pCity->getOwner() != getID())
		return -1;

	// Need to have enough culture to remove it
	if(!bAdd && pCity->getCultureLevel() <= 0)
		return -1;

	int iCulture=-1;
	if (bAdd)
	{
		iCulture = CvCity::getCultureThreshold((CultureLevelTypes)
				(pCity->getCultureLevel() + 1)) - pCity->getCulture(getID());
	}
	else
	{
		iCulture = pCity->getCulture(getID()) - CvCity::getCultureThreshold((CultureLevelTypes)
				(pCity->getCultureLevel() - 1));
	}
	iCost *= iCulture;
	iCost /= std::max(1, GC.getInfo(GC.getGame().getGameSpeedType()).
			getHurryPercent());
	return adjustAdvStartPtsToSpeed(iCost); // advc.250c
}

// Adding or removing a Building from a city
int CvPlayer::getAdvancedStartBuildingCost(BuildingTypes eBuilding, bool bAdd,
	CvCity const* pCity) const
{
	if (getNumCities() <= 0)
		return -1;

	int iCost = (getProductionNeeded(eBuilding) *
			GC.getInfo(eBuilding).getAdvancedStartCost()) / 100;
	iCost = (iCost * fixp(1.5)).floor(); // advc.250c
	if (iCost < 0)
		return -1;

	if (GC.getGame().isFreeStartEraBuilding(eBuilding)) // advc
		return -1; // you get this building for free

	if (pCity == NULL)
	{
		if (bAdd)
		{
			bool bValid = false;
			FOR_EACH_CITY(pLoopCity, *this)
			{
				if (pLoopCity->canConstruct(eBuilding))
				{
					bValid = true;
					break;
				}
			}
			if (!bValid)
				return -1;
		}
	}
	else
	{
		if (pCity->getOwner() != getID())
			return -1;

		iCost *= 100;
		iCost /= std::max(1, 100 + pCity->getProductionModifier(eBuilding));

		if (bAdd)
		{
			if (!pCity->canConstruct(eBuilding, true, false, false))
				return -1;
		}
		else
		{
			if (pCity->getNumRealBuilding(eBuilding) <= 0)
				return -1;

			// Check other buildings in this city and make sure none of them require this one ...

			// Loop through Buildings to see which are present
			FOR_EACH_ENUM(Building)
			{
				if (pCity->getNumBuilding(eLoopBuilding) > 0)
				{
					// Loop through present Building's requirements
					for (int iBuildingClassPrereqLoop = 0; iBuildingClassPrereqLoop <
						GC.getNumBuildingClassInfos(); iBuildingClassPrereqLoop++)
					{
						if (GC.getInfo(eLoopBuilding).isBuildingClassNeededInCity(
							iBuildingClassPrereqLoop))
						{
							if ((getCivilization().getBuilding((BuildingClassTypes)
								iBuildingClassPrereqLoop)) == eBuilding)
							{
								return -1;
							}
						}
					}
				}
			}
		}
	}

	// Increase cost if the XML defines that additional Buildings will cost more
	if (GC.getInfo(eBuilding).getAdvancedStartCostIncrease() != 0)
	{
		int iBuildings = countNumBuildings(eBuilding);
		if (!bAdd)
			iBuildings--;

		if (iBuildings > 0)
		{
			iCost *= 100 + GC.getInfo(eBuilding).getAdvancedStartCostIncrease() *
					std::max(0, iBuildings - getNumCities());
			iCost /= 100;
		}
	}

	return adjustAdvStartPtsToSpeed(iCost); // advc.250c
}

// Adding or removing Route
int CvPlayer::getAdvancedStartRouteCost(RouteTypes eRoute, bool bAdd, CvPlot const* pPlot) const
{
	if (getNumCities() <= 0 || eRoute == NO_ROUTE)
		return -1;

	int iCost = GC.getInfo(eRoute).getAdvancedStartCost();
	if (iCost < 0)
		return -1; // cannot be purchased through Advanced Start

	// <advc.250c>
	iCost *= GC.getDefineINT("ADVANCED_START_WORKER_BUILD_MODIFIER");
	iCost /= 100; // </advc.250c>

	iCost *= GC.getInfo(GC.getGame().getGameSpeedType()).getBuildPercent();
	iCost /= 100;

	if (pPlot != NULL)
	{
		if (pPlot->isCity())
			return -1;

		if (bAdd)
		{
			if (pPlot->isImpassable() || pPlot->isWater())
				return -1;
			// Can't place twice
			if (pPlot->getRouteType() == eRoute)
				return -1;
		}
		else
		{
			// Need Route to remove it
			if (pPlot->getRouteType() != eRoute)
				return -1;
		}

		if (pPlot->getOwner() != getID())
			return -1;
	}

	// Tech requirement
	FOR_EACH_ENUM(Build)
	{
		if (GC.getInfo(eLoopBuild).getRoute() == eRoute)
		{
			if (!GET_TEAM(getTeam()).isHasTech(GC.getInfo(eLoopBuild).getTechPrereq()))
				return -1;
		}
	}

	// Increase cost if the XML defines that additional units will cost more
	if (GC.getInfo(eRoute).getAdvancedStartCostIncrease() != 0)
	{
		int iNumRoutes = 0;
		for (int i = 0; i < GC.getMap().numPlots(); i++)
		{
			CvPlot const& kLoopPlot = GC.getMap().getPlotByIndex(i);
			// <advc.001>
			if (kLoopPlot.getOwner() != getID())
				continue; // </advc.001>
			if (kLoopPlot.getRouteType() == eRoute)
				iNumRoutes++;
		}
		if (!bAdd)
			iNumRoutes--;

		if (iNumRoutes > 0)
		{
			iCost *= 100 + GC.getInfo(eRoute).getAdvancedStartCostIncrease() * iNumRoutes;
			iCost /= 100;
		}
	}

	return adjustAdvStartPtsToSpeed(iCost); // advc.250c
}

// Adding or removing Improvement
int CvPlayer::getAdvancedStartImprovementCost(ImprovementTypes eImprovement, bool bAdd, CvPlot const* pPlot) const
{
	if (eImprovement == NO_IMPROVEMENT || getNumCities() <= 0)
		return -1;

	int iNumImprovements = 0;
	int iCost = GC.getInfo(eImprovement).getAdvancedStartCost();
	if (iCost < 0)
		return -1; // Cannot be purchased through Advanced Start

	// <advc.250c>
	iCost *= GC.getDefineINT("ADVANCED_START_WORKER_BUILD_MODIFIER");
	iCost /= 100; // </advc.250c>

	iCost *= GC.getInfo(GC.getGame().getGameSpeedType()).getBuildPercent();
	iCost /= 100;

	// Can this Improvement be on our plot?
	if (pPlot != NULL)
	{
		if (bAdd)
		{
			if (!pPlot->canHaveImprovement(eImprovement, getTeam(), false))
				return -1;

			bool bValid = false;
			FOR_EACH_ENUM(Build)
			{
				CvBuildInfo const& kBuild = GC.getInfo(eLoopBuild);
				ImprovementTypes eLoopImprovement = (ImprovementTypes)kBuild.getImprovement();
				if (eImprovement == eLoopImprovement && canBuild(*pPlot, eLoopBuild))
				{
					bValid = true;
					FeatureTypes eFeature = pPlot->getFeatureType();
					if (eFeature != NO_FEATURE && kBuild.isFeatureRemove(eFeature))
						iCost += GC.getInfo(eFeature).getAdvancedStartRemoveCost();
					break;
				}
			}
			if (!bValid)
				return -1;

			// Can't place twice
			if (pPlot->getImprovementType() == eImprovement)
				return -1;
		}
		else
		{
			// Need this improvement in order to remove it
			if (pPlot->getImprovementType() != eImprovement)
				return -1;
		}

		// Must be owned by me
		if (pPlot->getOwner() != getID())
			return -1;
	}

	// Tech requirement
	FOR_EACH_ENUM(Build)
	{
		if (GC.getInfo(eLoopBuild).getImprovement() == eImprovement)
		{
			if (!GET_TEAM(getTeam()).isHasTech(GC.getInfo(eLoopBuild).getTechPrereq()))
				return -1;
		}
	}

	// Increase cost if the XML defines that additional units will cost more
	if (GC.getInfo(eImprovement).getAdvancedStartCostIncrease() != 0)
	{
		for (int i = 0; i < GC.getMap().numPlots(); i++)
		{
			CvPlot const& kLoopPlot = GC.getMap().getPlotByIndex(i);
			// <advc.001>
			if (kLoopPlot.getOwner() != getID())
				continue; // </advc.001>
			if (kLoopPlot.getImprovementType() == eImprovement)
				iNumImprovements++;
		}
		if (!bAdd)
			iNumImprovements--;

		if (iNumImprovements > 0)
		{
			iCost *= 100 + GC.getInfo(eImprovement).getAdvancedStartCostIncrease() * iNumImprovements;
			iCost /= 100;
		}
	}

	return adjustAdvStartPtsToSpeed(iCost); // advc.250c
}

// Adding or removing Tech
int CvPlayer::getAdvancedStartTechCost(TechTypes eTech, bool bAdd) const 
{
	if (eTech == NO_TECH || getNumCities() <= 0)
		return -1;

	int iCost = (GET_TEAM(getTeam()).getResearchCost(eTech) *
			GC.getInfo(eTech).getAdvancedStartCost()) / 100;
	if (iCost < 0)
		return -1;

	if (bAdd)
	{
		if (!canResearch(eTech, false))
			return -1;
	}
	else if (!bAdd)
	{
		if (!GET_TEAM(getTeam()).isHasTech(eTech))
			return -1;

		// Search through all techs to see if any of the currently owned ones requires this tech
		FOR_EACH_ENUM(Tech)
		{
			if (!GET_TEAM(getTeam()).isHasTech(eLoopTech))
				continue;
			for (int i = 0; i < GC.getInfo(eLoopTech).getNumOrTechPrereqs(); i++)
			{
				if (GC.getInfo(eLoopTech).getPrereqOrTechs(i) == eTech)
					return -1;
			}
			for (int i = 0; i < GC.getInfo(eLoopTech).getNumAndTechPrereqs(); i++)
			{
				if (GC.getInfo(eLoopTech).getPrereqAndTechs(i) == eTech)
					return -1;
			}
		}

		// If player has placed anything on the map which uses this tech then you cannot remove it

		FOR_EACH_UNIT(pLoopUnit, *this)
		{
			if (pLoopUnit->getUnitInfo().getPrereqAndTech() == eTech)
				return -1;
			for (int i = 0; i < pLoopUnit->getUnitInfo().getNumPrereqAndTechs(); i++)
			{
				if (pLoopUnit->getUnitInfo().getPrereqAndTechs(i) == eTech)
					return -1;
			}
		}

		FOR_EACH_CITY(pLoopCity, *this)
		{
			FOR_EACH_ENUM(Building)
			{
				if (pLoopCity->getNumRealBuilding(eLoopBuilding) > 0)
				{
					if (GC.getInfo(eLoopBuilding).getPrereqAndTech() == eTech)
						return -1;
					for (int i = 0; i < GC.getInfo(eLoopBuilding).
						getNumPrereqAndTechs(); i++)
					{
						if (GC.getInfo(eLoopBuilding).getPrereqAndTechs(i) == eTech)
							return -1;
					}
				}
			}
		}

	}

	// Increase cost if the XML defines that additional units will cost more
	if (GC.getInfo(eTech).getAdvancedStartCostIncrease() != 0)
	{
		int iTechs = 0;
		FOR_EACH_ENUM(Tech)
		{
			if (GET_TEAM(getTeam()).isHasTech(eLoopTech))
				iTechs++;
		}

		if (!bAdd)
			iTechs--;

		if (iTechs > 0)
		{
			iCost *= 100 + GC.getInfo(eTech).getAdvancedStartCostIncrease() * iTechs;
			iCost /= 100;
		}
	}

	return adjustAdvStartPtsToSpeed(iCost); // advc.250c
}

// Adding or removing Visibility
int CvPlayer::getAdvancedStartVisibilityCost(bool bAdd, CvPlot const* pPlot) const
{
	if (getNumCities() <= 0)
		return -1;

	int iCost = GC.getDefineINT("ADVANCED_START_VISIBILITY_COST");
	// This denotes Visibility may not be purchased through Advanced Start
	if (iCost == -1)
		return -1;

	// Valid Plot?
	if(pPlot != NULL)
	{
		if(bAdd)
		{
			if(pPlot->isRevealed(getTeam()))
				return -1;
			if (!pPlot->isAdjacentRevealed(getTeam(),
				// advc.250c:
				GC.getGame().getStartEra() <= CvEraInfo::AI_getAgeOfExploration()))
			{
				return -1;
			}
		}
		else if (!pPlot->isRevealed(getTeam()))
			return -1;
	}

	// Increase cost if the XML defines that additional units will cost more
	if (GC.getDefineINT("ADVANCED_START_VISIBILITY_COST_INCREASE") != 0)
	{
		int iVisible = -NUM_CITY_PLOTS; // advc.210c
		for (int i = 0; i < GC.getMap().numPlots(); i++)
		{
			CvPlot const& kLoopPlot = GC.getMap().getPlotByIndex(i);
			if (kLoopPlot.isRevealed(getTeam()))
				iVisible++;
		}

		if (!bAdd)
			iVisible--;

		if (iVisible > 0)
		{
			iCost *= 100 + GC.getDefineINT("ADVANCED_START_VISIBILITY_COST_INCREASE") * iVisible;
			iCost /= 100;
		}
	}

	return adjustAdvStartPtsToSpeed(iCost); // advc.250c
}


void CvPlayer::doWarnings()
{
	//update enemy unit in your territory glow
	FOR_EACH_UNIT_VAR(pLoopUnit, *this)
	{
		//update glow
		gDLL->getEntityIFace()->updateEnemyGlow(pLoopUnit->getEntity());
	}

	//update enemy units close to your territory
	int iMaxCount = range(((getNumCities() + 4) / 7), 2, 5);
	for (int i = 0; i < GC.getMap().numPlots(); i++)
	{
		if (iMaxCount == 0)
			break;

		CvPlot const& kPlot = GC.getMap().getPlotByIndex(i);
		if (kPlot.isAdjacentPlayer(getID()) && !kPlot.isCity() &&
			kPlot.isVisible(getTeam()))
		{
			CvUnit const* pUnit = kPlot.getVisibleEnemyDefender(getID());
			if (pUnit != NULL && !pUnit->isAnimal())
			{
				CvCity* pNearestCity = GC.getMap().findCity(kPlot.getX(), kPlot.getY(),
						getID(), NO_TEAM, !kPlot.isWater());
				if (pNearestCity != NULL)
				{
					wchar szBuffer[1024];
					swprintf(szBuffer, gDLL->getText("TXT_KEY_MISC_ENEMY_TROOPS_SPOTTED",
							pNearestCity->getNameKey()).GetCString());
					gDLL->UI().addMessage(getID(), true, -1, szBuffer, kPlot,
							"AS2D_ENEMY_TROOPS", MESSAGE_TYPE_INFO, pUnit->getButton(),
							GC.getColorType("RED"));
					iMaxCount--;
				}
			}
		}
	} // <advc.002e>
	if(GC.IsGraphicsInitialized())
		showForeignPromoGlow(true); // </advc.002e>
}


void CvPlayer::verifyGoldCommercePercent()
{
	/*while ((getGold() + calculateGoldRate()) < 0) {
		changeCommercePercent(COMMERCE_GOLD, GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS"));
		if (getCommercePercent(COMMERCE_GOLD) == 100)
			break;
	}*/ // BtS
	// K-Mod
	bool bValid = isCommerceFlexible(COMMERCE_GOLD);
	while (bValid && getCommercePercent(COMMERCE_GOLD) < 100 && getGold() + calculateGoldRate() < 0)
	{
		bValid = changeCommercePercent(COMMERCE_GOLD, GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS"));
	} // K-Mod end
}

/*  <advc.003y> Ported to C++ from CvGameUtils.py; callback disabled by default.
	On-screen message to be handled by the caller; hence the return value. */
int CvPlayer::doCaptureGold(CvCity const& kOldCity) // "old": city still fully intact
{
	int iCaptureGold = 0;
	if (!GC.getPythonCaller()->captureGold(kOldCity, iCaptureGold))
	{
		FAssert(iCaptureGold == 0);
		static int const iBASE_CAPTURE_GOLD = GC.getDefineINT("BASE_CAPTURE_GOLD");
		static int const iCAPTURE_GOLD_PER_POPULATION = GC.getDefineINT("CAPTURE_GOLD_PER_POPULATION");
		static int const iCAPTURE_GOLD_RAND1 = GC.getDefineINT("CAPTURE_GOLD_RAND1");
		static int const iCAPTURE_GOLD_RAND2 = GC.getDefineINT("CAPTURE_GOLD_RAND2");
		static int const iCAPTURE_GOLD_MAX_TURNS = GC.getDefineINT("CAPTURE_GOLD_MAX_TURNS");
		iCaptureGold += iBASE_CAPTURE_GOLD;
		iCaptureGold += kOldCity.getPopulation() * iCAPTURE_GOLD_PER_POPULATION;
		CvGame& kGame = GC.getGame();
		iCaptureGold += kGame.getSorenRandNum(iCAPTURE_GOLD_RAND1, "Capture Gold 1");
		iCaptureGold += kGame.getSorenRandNum(iCAPTURE_GOLD_RAND2, "Capture Gold 2");
		if (iCAPTURE_GOLD_MAX_TURNS > 0)
		{
			iCaptureGold *= ::range(kGame.getGameTurn() - kOldCity.getGameTurnAcquired(),
					0, iCAPTURE_GOLD_MAX_TURNS);
			iCaptureGold /= iCAPTURE_GOLD_MAX_TURNS;
		}
	}
	changeGold(iCaptureGold);
	return iCaptureGold;
} // </advc.003y>


void CvPlayer::processCivics(CivicTypes eCivic, int iChange)
{
	changeGreatPeopleRateModifier(GC.getInfo(eCivic).getGreatPeopleRateModifier() * iChange);
	changeGreatGeneralRateModifier(GC.getInfo(eCivic).getGreatGeneralRateModifier() * iChange);
	changeDomesticGreatGeneralRateModifier(GC.getInfo(eCivic).getDomesticGreatGeneralRateModifier() * iChange);
	changeStateReligionGreatPeopleRateModifier(GC.getInfo(eCivic).getStateReligionGreatPeopleRateModifier() * iChange);
	changeDistanceMaintenanceModifier(GC.getInfo(eCivic).getDistanceMaintenanceModifier() * iChange);
	changeNumCitiesMaintenanceModifier(GC.getInfo(eCivic).getNumCitiesMaintenanceModifier() * iChange);
	changeCorporationMaintenanceModifier(GC.getInfo(eCivic).getCorporationMaintenanceModifier() * iChange);
	changeExtraHealth(GC.getInfo(eCivic).getExtraHealth() * iChange);
	changeExtraHappiness(GC.getInfo(eCivic).getExtraHappiness() * iChange); // K-Mod
	changeFreeExperience(GC.getInfo(eCivic).getFreeExperience() * iChange);
	changeWorkerSpeedModifier(GC.getInfo(eCivic).getWorkerSpeedModifier() * iChange);
	changeImprovementUpgradeRateModifier(GC.getInfo(eCivic).getImprovementUpgradeRateModifier() * iChange);
	changeMilitaryProductionModifier(GC.getInfo(eCivic).getMilitaryProductionModifier() * iChange);
	changeBaseFreeUnits(GC.getInfo(eCivic).getBaseFreeUnits() * iChange);
	changeBaseFreeMilitaryUnits(GC.getInfo(eCivic).getBaseFreeMilitaryUnits() * iChange);
	changeFreeUnitsPopulationPercent(GC.getInfo(eCivic).getFreeUnitsPopulationPercent() * iChange);
	changeFreeMilitaryUnitsPopulationPercent(GC.getInfo(eCivic).getFreeMilitaryUnitsPopulationPercent() * iChange);
	changeGoldPerUnit(GC.getInfo(eCivic).getGoldPerUnit() * iChange);
	changeGoldPerMilitaryUnit(GC.getInfo(eCivic).getGoldPerMilitaryUnit() * iChange);
	changeHappyPerMilitaryUnit(GC.getInfo(eCivic).getHappyPerMilitaryUnit() * iChange);
	// <advc.912c>
	changeLuxuryModifier(GC.getInfo(eCivic).getLuxuryModifier() * iChange);
	if(GC.getInfo(eCivic).getLuxuryModifier() != 0)
		AI().AI_updateBonusValue(); // </advc.912c>
	changeMilitaryFoodProductionCount((GC.getInfo(eCivic).isMilitaryFoodProduction()) ? iChange : 0);
	changeMaxConscript(GC.getGame().getMaxConscript(eCivic) * iChange);
	//changeNoUnhealthyPopulationCount((GC.getInfo(eCivic).isNoUnhealthyPopulation()) ? iChange : 0);
	changeUnhealthyPopulationModifier(GC.getInfo(eCivic).getUnhealthyPopulationModifier() * iChange); // K-Mod
	changeBuildingOnlyHealthyCount((GC.getInfo(eCivic).isBuildingOnlyHealthy()) ? iChange : 0);
	changeLargestCityHappiness(GC.getInfo(eCivic).getLargestCityHappiness() * iChange);
	changeWarWearinessModifier(GC.getInfo(eCivic).getWarWearinessModifier() * iChange);
	changeFreeSpecialist(GC.getInfo(eCivic).getFreeSpecialist() * iChange);
	changeTradeRoutes(GC.getInfo(eCivic).getTradeRoutes() * iChange);
	changeNoForeignTradeCount(GC.getInfo(eCivic).isNoForeignTrade() * iChange);
	changeNoCorporationsCount(GC.getInfo(eCivic).isNoCorporations() * iChange);
	changeNoForeignCorporationsCount(GC.getInfo(eCivic).isNoForeignCorporations() * iChange);
	changeStateReligionCount((GC.getInfo(eCivic).isStateReligion()) ? iChange : 0);
	changeNoNonStateReligionSpreadCount((GC.getInfo(eCivic).isNoNonStateReligionSpread()) ? iChange : 0);
	changeStateReligionHappiness(GC.getInfo(eCivic).getStateReligionHappiness() * iChange);
	changeNonStateReligionHappiness(GC.getInfo(eCivic).getNonStateReligionHappiness() * iChange);
	changeStateReligionUnitProductionModifier(GC.getInfo(eCivic).getStateReligionUnitProductionModifier() * iChange);
	changeStateReligionBuildingProductionModifier(GC.getInfo(eCivic).getStateReligionBuildingProductionModifier() * iChange);
	changeStateReligionFreeExperience(GC.getInfo(eCivic).getStateReligionFreeExperience() * iChange);
	changeExpInBorderModifier(GC.getInfo(eCivic).getExpInBorderModifier() * iChange);

	FOR_EACH_ENUM2(Yield, y)
	{
		changeYieldRateModifier(y, GC.getInfo(eCivic).getYieldModifier(y) * iChange);
		changeCapitalYieldRateModifier(y, GC.getInfo(eCivic).getCapitalYieldModifier(y) * iChange);
		changeTradeYieldModifier(y, GC.getInfo(eCivic).getTradeYieldModifier(y) * iChange);
	}
	FOR_EACH_ENUM2(Commerce, c)
	{
		changeCommerceRateModifier(c, GC.getInfo(eCivic).getCommerceModifier(c) * iChange);
		changeCapitalCommerceRateModifier(c, GC.getInfo(eCivic).getCapitalCommerceModifier(c) * iChange);
		changeSpecialistExtraCommerce(c, GC.getInfo(eCivic).getSpecialistExtraCommerce(c) * iChange);
	}
	// <advc.003t>
	if (GC.getInfo(eCivic).isAnyBuildingHappinessChanges() ||
		GC.getInfo(eCivic).isAnyBuildingHealthChanges()) // </advc.003t>
	{
		CvCivilization const& kCiv = getCivilization(); // advc.003w
		for (int i = 0; i < kCiv.getNumBuildings(); i++)
		{
			BuildingTypes eOurBuilding = kCiv.buildingAt(i);
			BuildingClassTypes eLoopClass = kCiv.buildingClassAt(i);
			changeExtraBuildingHappiness(eOurBuilding, GC.getInfo(eCivic).
					getBuildingHappinessChanges(eLoopClass) * iChange);
			changeExtraBuildingHealth(eOurBuilding, GC.getInfo(eCivic).
					getBuildingHealthChanges(eLoopClass) * iChange);
		}
	}

	FOR_EACH_ENUM(Feature)
	{
		changeFeatureHappiness(eLoopFeature,
				GC.getInfo(eCivic).getFeatureHappinessChanges(eLoopFeature) * iChange);
	}
	FOR_EACH_ENUM(Hurry)
	{
		changeHurryCount(eLoopHurry,
				GC.getInfo(eCivic).isHurry(eLoopHurry) ?
				iChange : 0);
	}
	FOR_EACH_ENUM(SpecialBuilding)
	{
		changeSpecialBuildingNotRequiredCount(eLoopSpecialBuilding,
				GC.getInfo(eCivic).isSpecialBuildingNotRequired(eLoopSpecialBuilding) ?
				iChange : 0);
	}
	FOR_EACH_ENUM(Specialist)
	{
		changeSpecialistValidCount(eLoopSpecialist,
				GC.getInfo(eCivic).isSpecialistValid(eLoopSpecialist) ?
				iChange : 0);
	}
	FOR_EACH_ENUM(Improvement)
	{
		FOR_EACH_ENUM(Yield)
		{
			changeImprovementYieldChange(eLoopImprovement, eLoopYield,
					GC.getInfo(eCivic).getImprovementYieldChanges(
					eLoopImprovement, eLoopYield) * iChange);
		}
	}
}

void CvPlayer::showMissedMessages()
{
	CvMessageQueue::iterator it = m_listGameMessages.begin();
	while (it != m_listGameMessages.end())
	{
		CvTalkingHeadMessage& msg = *it;
		if (!msg.getShown())
		{
			msg.setShown(true);
			gDLL->UI().showMessage(msg);
		}
		++it;
	}
}

bool CvPlayer::isPbemNewTurn() const
{
	return m_bPbemNewTurn;
}

void CvPlayer::setPbemNewTurn(bool bNew)
{
	m_bPbemNewTurn = bNew;
}

// read object from a stream. used during load
void CvPlayer::read(FDataStreamBase* pStream)
{
	reset();

	uint uiFlag=0;
	pStream->Read(&uiFlag);
	// <advc.027>
	{	// (No longer stored as x,y)
		int iStartingX, iStartingY;
		pStream->Read(&iStartingX);
		pStream->Read(&iStartingY);
		m_pStartingPlot = GC.getMap().plot(iStartingX, iStartingY);
	}
	if (uiFlag >= 14)
		pStream->Read(&m_bRandomWBStart);
	// </advc.027>
	pStream->Read(&m_iTotalPopulation);
	pStream->Read(&m_iTotalLand);
	pStream->Read(&m_iTotalLandScored);
	pStream->Read(&m_iGold);
	pStream->Read(&m_iGoldPerTurn);
	pStream->Read(&m_iAdvancedStartPoints);
	pStream->Read(&m_iGoldenAgeTurns);
	// <advc.001x>
	if (uiFlag >= 11)
		pStream->Read(&m_iScheduledGoldenAges); // </advc.001x>
	pStream->Read(&m_iNumUnitGoldenAges);
	pStream->Read(&m_iStrikeTurns);
	pStream->Read(&m_iAnarchyTurns);
	pStream->Read(&m_iMaxAnarchyTurns);
	pStream->Read(&m_iAnarchyModifier);
	pStream->Read(&m_iGoldenAgeModifier);
	pStream->Read(&m_iGlobalHurryModifier);
	pStream->Read(&m_iGreatPeopleCreated);
	pStream->Read(&m_iGreatGeneralsCreated);
	pStream->Read(&m_iGreatPeopleThresholdModifier);
	pStream->Read(&m_iGreatGeneralsThresholdModifier);
	pStream->Read(&m_iGreatPeopleRateModifier);
	pStream->Read(&m_iGreatGeneralRateModifier);
	pStream->Read(&m_iDomesticGreatGeneralRateModifier);
	pStream->Read(&m_iStateReligionGreatPeopleRateModifier);
	pStream->Read(&m_iMaxGlobalBuildingProductionModifier);
	pStream->Read(&m_iMaxTeamBuildingProductionModifier);
	pStream->Read(&m_iMaxPlayerBuildingProductionModifier);
	pStream->Read(&m_iFreeExperience);
	pStream->Read(&m_iFeatureProductionModifier);
	pStream->Read(&m_iWorkerSpeedModifier);
	pStream->Read(&m_iImprovementUpgradeRateModifier);
	pStream->Read(&m_iMilitaryProductionModifier);
	pStream->Read(&m_iSpaceProductionModifier);
	pStream->Read(&m_iCityDefenseModifier);
	pStream->Read(&m_iNumNukeUnits);
	pStream->Read(&m_iNumOutsideUnits);
	pStream->Read(&m_iBaseFreeUnits);
	pStream->Read(&m_iBaseFreeMilitaryUnits);
	pStream->Read(&m_iFreeUnitsPopulationPercent);
	pStream->Read(&m_iFreeMilitaryUnitsPopulationPercent);
	pStream->Read(&m_iGoldPerUnit);
	pStream->Read(&m_iGoldPerMilitaryUnit);
	// K-Mod
	if (uiFlag < 3)
	{
		m_iGoldPerUnit *= 100;
		m_iGoldPerMilitaryUnit *= 100;
	}
	// K-Mod end
	pStream->Read(&m_iExtraUnitCost);
	pStream->Read(&m_iNumMilitaryUnits);
	pStream->Read(&m_iHappyPerMilitaryUnit);
	// <advc.912c>
	if(uiFlag >= 6)
		pStream->Read(&m_iLuxuryModifier); // </advc.912c>
	pStream->Read(&m_iMilitaryFoodProductionCount);
	pStream->Read(&m_iConscriptCount);
	pStream->Read(&m_iMaxConscript);
	pStream->Read(&m_iHighestUnitLevel);
	pStream->Read(&m_iOverflowResearch);
	//pStream->Read(&m_iNoUnhealthyPopulationCount);
	pStream->Read(&m_iUnhealthyPopulationModifier); // K-Mod
	pStream->Read(&m_iExpInBorderModifier);
	pStream->Read(&m_iBuildingOnlyHealthyCount);
	pStream->Read(&m_iDistanceMaintenanceModifier);
	pStream->Read(&m_iNumCitiesMaintenanceModifier);
	pStream->Read(&m_iCorporationMaintenanceModifier);
	pStream->Read(&m_iTotalMaintenance);
	pStream->Read(&m_iUpkeepModifier);
	pStream->Read(&m_iLevelExperienceModifier);
	pStream->Read(&m_iExtraHealth);
	pStream->Read(&m_iBuildingGoodHealth);
	pStream->Read(&m_iBuildingBadHealth);
	pStream->Read(&m_iExtraHappiness);
	pStream->Read(&m_iBuildingHappiness);
	pStream->Read(&m_iLargestCityHappiness);
	pStream->Read(&m_iWarWearinessPercentAnger);
	pStream->Read(&m_iWarWearinessModifier);
	pStream->Read(&m_iGwPercentAnger); // K-Mod
	pStream->Read(&m_iFreeSpecialist);
	pStream->Read(&m_iNoForeignTradeCount);
	pStream->Read(&m_iNoCorporationsCount);
	pStream->Read(&m_iNoForeignCorporationsCount);
	pStream->Read(&m_iCoastalTradeRoutes);
	pStream->Read(&m_iTradeRoutes);
	pStream->Read(&m_iRevolutionTimer);
	pStream->Read(&m_iConversionTimer);
	pStream->Read(&m_iStateReligionCount);
	pStream->Read(&m_iNoNonStateReligionSpreadCount);
	pStream->Read(&m_iStateReligionHappiness);
	pStream->Read(&m_iNonStateReligionHappiness);
	pStream->Read(&m_iStateReligionUnitProductionModifier);
	pStream->Read(&m_iStateReligionBuildingProductionModifier);
	pStream->Read(&m_iStateReligionFreeExperience);
	pStream->Read(&m_iCapitalCityID);
	pStream->Read(&m_iCitiesLost);
	pStream->Read(&m_iWinsVsBarbs);
	pStream->Read(&m_iAssets);
	pStream->Read(&m_iPower);
	pStream->Read(&m_iPopulationScore);
	pStream->Read(&m_iLandScore);
	pStream->Read(&m_iWondersScore);
	pStream->Read(&m_iTechScore);
	pStream->Read(&m_iCombatExperience);
	// <advc.004x>
	if(uiFlag >= 8)
	{
		int tmp=-1;
		pStream->Read(&tmp);
		m_eReminderPending = (CivicTypes)tmp;
	} // </advc.004x>
	pStream->Read(&m_bAlive);
	pStream->Read(&m_bEverAlive);
	pStream->Read(&m_bTurnActive);
	pStream->Read(&m_bAutoMoves);
	pStream->Read(&m_bEndTurn);
	pStream->Read(&m_bPbemNewTurn);
	pStream->Read(&m_bExtendedGame);
	pStream->Read(&m_bFoundedFirstCity);
	// <advc.078>
	if(uiFlag >= 9)
		pStream->Read(&m_bAnyGPPEver); // </advc.078>
	pStream->Read(&m_bStrike);
	// K-Mod
	if (uiFlag >= 4)
		pStream->Read(&m_iChoosingFreeTechCount);
	else if (uiFlag >= 2)
	{
		bool bFreeTech = false;
		pStream->Read(&bFreeTech);
		m_iChoosingFreeTechCount = bFreeTech ? 1 : 0;
	}
	// K-Mod end

	pStream->Read((int*)&m_eID);
	pStream->Read((int*)&m_ePersonalityType);
	pStream->Read((int*)&m_eCurrentEra);
	pStream->Read((int*)&m_eLastStateReligion);
	pStream->Read((int*)&m_eParent);
	updateTeamType(); //m_eTeamType not saved
	updateHuman();

	m_aiSeaPlotYield.Read(pStream);
	m_aiYieldRateModifier.Read(pStream);
	m_aiCapitalYieldRateModifier.Read(pStream);
	m_aiExtraYieldThreshold.Read(pStream);
	m_aiTradeYieldModifier.Read(pStream);
	m_aiFreeCityCommerce.Read(pStream);
	m_aiCommercePercent.Read(pStream);
	m_aiCommerceRate.Read(pStream);
	m_aiCommerceRateModifier.Read(pStream);
	m_aiCapitalCommerceRateModifier.Read(pStream);
	m_aiStateReligionBuildingCommerce.Read(pStream);
	m_aiSpecialistExtraCommerce.Read(pStream);
	m_aiCommerceFlexibleCount.Read(pStream);
	m_aiGoldPerTurnByPlayer.Read(pStream);
	m_aiEspionageSpendingWeightAgainstTeam.Read(pStream);
	m_abFeatAccomplished.Read(pStream);
	m_abOptions.Read(pStream);

	pStream->ReadString(m_szScriptData);

	m_aiBonusExport.Read(pStream);
	m_aiBonusImport.Read(pStream);
	m_aiImprovementCount.Read(pStream);
	m_aiFreeBuildingCount.Read(pStream);
	m_aiExtraBuildingHappiness.Read(pStream);
	m_aiExtraBuildingHealth.Read(pStream);
	m_aiFeatureHappiness.Read(pStream);
	m_aiUnitClassCount.Read(pStream);
	m_aiUnitClassMaking.Read(pStream);
	m_aiBuildingClassCount.Read(pStream);
	m_aiBuildingClassMaking.Read(pStream);
	m_aiHurryCount.Read(pStream);
	m_aiSpecialBuildingNotRequiredCount.Read(pStream);
	m_aiHasCivicOptionCount.Read(pStream);
	m_aiNoCivicUpkeepCount.Read(pStream);
	m_aiHasReligionCount.Read(pStream);
	m_aiHasCorporationCount.Read(pStream);
	m_aiUpkeepCount.Read(pStream);
	m_aiSpecialistValidCount.Read(pStream);
	m_abResearchingTech.Read(pStream);
	// <advc.091>
	if (uiFlag >= 12)
		m_abEverSeenDemographics.Read(pStream);
	else if(isBarbarian()) // Once all players have been loaded
	{
		for (PlayerIter<CIV_ALIVE> itSpyPlayer; itSpyPlayer.hasNext(); ++itSpyPlayer)
		{
			for (PlayerIter<CIV_ALIVE> itTargetPlayer; itTargetPlayer.hasNext();
				++itTargetPlayer)
			{
				itSpyPlayer->m_abEverSeenDemographics.set(itTargetPlayer->getID(),
						itSpyPlayer->canSeeDemographics(itTargetPlayer->getID()));
			}
		}
	} // </advc.091>
	m_abLoyalMember.Read(pStream);
	m_aeCivics.Read(pStream);
	m_aaeSpecialistExtraYield.Read(pStream);
	m_aaeImprovementYieldChange.Read(pStream);

	m_groupCycle.Read(pStream);
	m_researchQueue.Read(pStream);

	{
		m_cityNames.clear();
		CvWString szBuffer;
		uint iSize;
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			pStream->ReadString(szBuffer);
			m_cityNames.insertAtEnd(szBuffer);
		}
	}

	ReadStreamableFFreeListTrashArray(m_plotGroups, pStream);
	ReadStreamableFFreeListTrashArray(m_cities, pStream);
	ReadStreamableFFreeListTrashArray(m_units, pStream);
	ReadStreamableFFreeListTrashArray(m_selectionGroups, pStream);
	ReadStreamableFFreeListTrashArray(m_eventsTriggered, pStream);

	{
		CvMessageQueue::_Alloc::size_type iSize;
		pStream->Read(&iSize);
		for (CvMessageQueue::_Alloc::size_type i = 0; i < iSize; i++)
		{
			CvTalkingHeadMessage message;
			message.read(*pStream);
			m_listGameMessages.push_back(message);
		}
	}

	{
		clearPopups();
		CvPopupQueue::_Alloc::size_type iSize;
		pStream->Read(&iSize);
		for (CvPopupQueue::_Alloc::size_type i = 0; i < iSize; i++)
		{
			CvPopupInfo* pInfo = new CvPopupInfo();
			if (NULL != pInfo)
			{
				pInfo->read(*pStream);
				m_listPopups.push_back(pInfo);
			}
		}
	}

	{
		clearDiplomacy();
		CvDiploQueue::_Alloc::size_type iSize;
		pStream->Read(&iSize);
		for (CvDiploQueue::_Alloc::size_type i = 0; i < iSize; i++)
		{
			CvDiploParameters* pDiplo = new CvDiploParameters(NO_PLAYER);
			if (pDiplo != NULL)
			{
				pDiplo->read(*pStream);
				m_listDiplomacy.push_back(pDiplo);
				// <advc.074> (see comment in CvPlayerAI::AI_doDeals)
				if (pDiplo->getDiploComment() == GC.getAIDiploCommentType("CANCEL_DEAL"))
				{
					FOR_EACH_TRADE_ITEM(pDiplo->getOurOfferList())
					{
						if (pItem->m_eItemType == TRADE_RESOURCES)
						{
							m_cancelingExport.insertAtEnd(std::make_pair(
									pDiplo->getWhoTalkingTo(),
									(BonusTypes)pItem->m_iData));
						}
					}
				} // </advc.074>
			}
		}
	}
	// <advc.004s>
	FOR_EACH_ENUM(PlayerHistory)
	{
		PlayerHistory& kHist = m_playerHistory[eLoopPlayerHistory];
		kHist.read(pStream, getID(), uiFlag < 13);
	} // </advc.004s>

	{
		m_mapEventsOccured.clear();
		uint iSize;
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			EventTriggeredData kData;
			EventTypes eEvent;
			pStream->Read((int*)&eEvent);
			kData.read(pStream);
			m_mapEventsOccured[eEvent] = kData;
		}
	}

	{
		m_mapEventCountdown.clear();
		uint iSize;
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			EventTriggeredData kData;
			EventTypes eEvent;
			pStream->Read((int*)&eEvent);
			kData.read(pStream);
			m_mapEventCountdown[eEvent] = kData;
		}
	}

	{
		m_aFreeUnitCombatPromotions.clear();
		uint iSize;
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			int iUnitCombat;
			int iPromotion;
			pStream->Read(&iUnitCombat);
			pStream->Read(&iPromotion);
			m_aFreeUnitCombatPromotions.push_back(std::make_pair((UnitCombatTypes)iUnitCombat, (PromotionTypes)iPromotion));
		}
	}

	{
		m_aFreeUnitClassPromotions.clear();
		uint iSize;
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			int iUnitClass;
			int iPromotion;
			pStream->Read(&iUnitClass);
			pStream->Read(&iPromotion);
			m_aFreeUnitClassPromotions.push_back(std::make_pair((UnitClassTypes)iUnitClass, (PromotionTypes)iPromotion));
		}
	}

	{
		m_aVote.clear();
		uint iSize;
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			int iId;
			PlayerVoteTypes eVote;
			pStream->Read(&iId);
			pStream->Read((int*)&eVote);
			m_aVote.push_back(std::make_pair(iId, eVote));
		}
	}

	{
		m_aUnitExtraCosts.clear();
		uint iSize;
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			int iCost;
			UnitClassTypes eUnit;
			pStream->Read((int*)&eUnit);
			pStream->Read(&iCost);
			m_aUnitExtraCosts.push_back(std::make_pair(eUnit, iCost));
		}
	}

	if (uiFlag > 0)
	{
		m_triggersFired.clear();
		uint iSize;
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			int iTrigger;
			pStream->Read(&iTrigger);
			m_triggersFired.push_back((EventTriggerTypes)iTrigger);
		}
	}
	else
	{
		// yuck, hardcoded number of eventTriggers in the epic game in initial release
		int iEventTriggers = std::min(176, GC.getNumEventTriggerInfos());
		for (int i = 0; i < iEventTriggers; i++)
		{
			bool bTriggered;
			pStream->Read(&bTriggered);
			if (bTriggered)
				m_triggersFired.push_back((EventTriggerTypes)i);
		}
	}

	if (!isBarbarian())
	{
		// Get the NetID from the initialization structure
		setNetID(gDLL->getAssignedNetworkID(getID()));
	}

	pStream->Read(&m_iPopRushHurryCount);
	// <advc.064b>
	if(uiFlag >= 10)
		pStream->Read(&m_iGoldRushHurryCount);
	else
	{
		FOR_EACH_ENUM(Hurry)
		{
			if (GC.getInfo(eLoopHurry).getGoldPerProduction() > 0)
			{
				m_iGoldRushHurryCount = m_aiHurryCount.get(eLoopHurry);
				break;
			}
		}
	} // </advc.064b>
	pStream->Read(&m_iInflationModifier);

	if(!isAlive())
		return; // advc
	initAlerts(); // advc.210
	/*  <advc.706> Loading into retirement. Can't do this in RiseFall::read b/c
		CvPlayer::reset has to be through first. */
	CvGame& kGame = GC.getGame();
	if(kGame.isOption(GAMEOPTION_RISE_FALL) && kGame.getRiseFall().hasRetired())
		kGame.getRiseFall().retire(); // </advc.706>
	// <advc.908a>
	if(uiFlag < 5 && !isBarbarian())
	{
		FOR_EACH_ENUM(Yield)
			updateExtraYieldThreshold(eLoopYield);
	} // </advc.908a>
	// <advc.912c>
	if(uiFlag <= 6)
	{
		FOR_EACH_ENUM2(Civic, eCivic)
		{
			if (!isCivic(eCivic))
				continue;
			CvCivicInfo& kCivic = GC.getInfo(eCivic);
			if (kCivic.getLuxuryModifier() <= 0)
				continue;
			int iPreviousHappyPer = (uiFlag < 6 ? 1 : 0);
			int iPreviousLux =  (uiFlag < 6 ? 0 : 50);
			changeHappyPerMilitaryUnit(kCivic.getHappyPerMilitaryUnit() -
					iPreviousHappyPer);
			changeLuxuryModifier(kCivic.getLuxuryModifier() - iPreviousLux);
		}
	} // </advc.912c>
}

// save object to a stream
void CvPlayer::write(FDataStreamBase* pStream)
{
	PROFILE_FUNC(); // advc

	REPRO_TEST_BEGIN_WRITE(CvString::format("PlayerPt1(%d)", getID()));
	uint uiFlag;
	//uiFlag = 1; // BtS
	//uiFlag = 5; // K-Mod
	//uiFlag = 5; // advc.908a (I guess I broke compatibility here)
	//uiFlag = 7; // advc.912c (6 used up for a test version)
	//uiFlag = 8; // advc.004x
	//uiFlag = 9; // advc.078
	//uiFlag = 10; // advc.064b
	//uiFlag = 11; // advc.001x
	//uiFlag = 12; // advc.091
	//uiFlag = 13; // advc.004s
	uiFlag = 14; // advc.027 (m_bRandomWBStart)
	pStream->Write(uiFlag);

	// <advc.027>
	int iStartingX = INVALID_PLOT_COORD, iStartingY = INVALID_PLOT_COORD;
	if (m_pStartingPlot != NULL)
	{
		iStartingX = m_pStartingPlot->getX();
		iStartingY = m_pStartingPlot->getY();
	}
	pStream->Write(iStartingX);
	pStream->Write(iStartingY);
	/*	Should only be needed at game start, but let's save it anyway.
		Might actually be needed for map regeneration. */
	pStream->Write(&m_bRandomWBStart);
	// </advc.027>
	pStream->Write(m_iTotalPopulation);
	pStream->Write(m_iTotalLand);
	pStream->Write(m_iTotalLandScored);
	pStream->Write(m_iGold);
	pStream->Write(m_iGoldPerTurn);
	pStream->Write(m_iAdvancedStartPoints);
	pStream->Write(m_iGoldenAgeTurns);
	pStream->Write(m_iScheduledGoldenAges); // advc.001x
	pStream->Write(m_iNumUnitGoldenAges);
	pStream->Write(m_iStrikeTurns);
	pStream->Write(m_iAnarchyTurns);
	pStream->Write(m_iMaxAnarchyTurns);
	pStream->Write(m_iAnarchyModifier);
	pStream->Write(m_iGoldenAgeModifier);
	pStream->Write(m_iGlobalHurryModifier);
	pStream->Write(m_iGreatPeopleCreated);
	pStream->Write(m_iGreatGeneralsCreated);
	pStream->Write(m_iGreatPeopleThresholdModifier);
	pStream->Write(m_iGreatGeneralsThresholdModifier);
	pStream->Write(m_iGreatPeopleRateModifier);
	pStream->Write(m_iGreatGeneralRateModifier);
	pStream->Write(m_iDomesticGreatGeneralRateModifier);
	pStream->Write(m_iStateReligionGreatPeopleRateModifier);
	pStream->Write(m_iMaxGlobalBuildingProductionModifier);
	pStream->Write(m_iMaxTeamBuildingProductionModifier);
	pStream->Write(m_iMaxPlayerBuildingProductionModifier);
	pStream->Write(m_iFreeExperience);
	pStream->Write(m_iFeatureProductionModifier);
	pStream->Write(m_iWorkerSpeedModifier);
	pStream->Write(m_iImprovementUpgradeRateModifier);
	pStream->Write(m_iMilitaryProductionModifier);
	pStream->Write(m_iSpaceProductionModifier);
	pStream->Write(m_iCityDefenseModifier);
	pStream->Write(m_iNumNukeUnits);
	pStream->Write(m_iNumOutsideUnits);
	pStream->Write(m_iBaseFreeUnits);
	pStream->Write(m_iBaseFreeMilitaryUnits);
	pStream->Write(m_iFreeUnitsPopulationPercent);
	pStream->Write(m_iFreeMilitaryUnitsPopulationPercent);
	pStream->Write(m_iGoldPerUnit);
	pStream->Write(m_iGoldPerMilitaryUnit);
	pStream->Write(m_iExtraUnitCost);
	pStream->Write(m_iNumMilitaryUnits);
	pStream->Write(m_iHappyPerMilitaryUnit);
	pStream->Write(m_iLuxuryModifier); // advc.912c
	pStream->Write(m_iMilitaryFoodProductionCount);
	pStream->Write(m_iConscriptCount);
	pStream->Write(m_iMaxConscript);
	pStream->Write(m_iHighestUnitLevel);
	pStream->Write(m_iOverflowResearch);
	//pStream->Write(m_iNoUnhealthyPopulationCount);
	pStream->Write(m_iUnhealthyPopulationModifier); // K-Mod
	pStream->Write(m_iExpInBorderModifier);
	pStream->Write(m_iBuildingOnlyHealthyCount);
	pStream->Write(m_iDistanceMaintenanceModifier);
	pStream->Write(m_iNumCitiesMaintenanceModifier);
	pStream->Write(m_iCorporationMaintenanceModifier);
	pStream->Write(m_iTotalMaintenance);
	pStream->Write(m_iUpkeepModifier);
	pStream->Write(m_iLevelExperienceModifier);
	pStream->Write(m_iExtraHealth);
	pStream->Write(m_iBuildingGoodHealth);
	pStream->Write(m_iBuildingBadHealth);
	pStream->Write(m_iExtraHappiness);
	pStream->Write(m_iBuildingHappiness);
	pStream->Write(m_iLargestCityHappiness);
	pStream->Write(m_iWarWearinessPercentAnger);
	pStream->Write(m_iWarWearinessModifier);
	pStream->Write(m_iGwPercentAnger); // K-Mod
	pStream->Write(m_iFreeSpecialist);
	pStream->Write(m_iNoForeignTradeCount);
	pStream->Write(m_iNoCorporationsCount);
	pStream->Write(m_iNoForeignCorporationsCount);
	pStream->Write(m_iCoastalTradeRoutes);
	pStream->Write(m_iTradeRoutes);
	pStream->Write(m_iRevolutionTimer);
	pStream->Write(m_iConversionTimer);
	pStream->Write(m_iStateReligionCount);
	pStream->Write(m_iNoNonStateReligionSpreadCount);
	pStream->Write(m_iStateReligionHappiness);
	pStream->Write(m_iNonStateReligionHappiness);
	pStream->Write(m_iStateReligionUnitProductionModifier);
	pStream->Write(m_iStateReligionBuildingProductionModifier);
	pStream->Write(m_iStateReligionFreeExperience);
	pStream->Write(m_iCapitalCityID);
	pStream->Write(m_iCitiesLost);
	pStream->Write(m_iWinsVsBarbs);
	pStream->Write(m_iAssets);
	pStream->Write(m_iPower);
	pStream->Write(m_iPopulationScore);
	pStream->Write(m_iLandScore);
	pStream->Write(m_iWondersScore);
	pStream->Write(m_iTechScore);
	pStream->Write(m_iCombatExperience);
	pStream->Write(m_eReminderPending); // advc.004x

	pStream->Write(m_bAlive);
	pStream->Write(m_bEverAlive);
	pStream->Write(m_bTurnActive);
	pStream->Write(m_bAutoMoves);
	pStream->Write(m_bEndTurn);
	pStream->Write(m_bPbemNewTurn && GC.getGame().isPbem());
	pStream->Write(m_bExtendedGame);
	pStream->Write(m_bFoundedFirstCity);
	pStream->Write(m_bAnyGPPEver); // advc.078
	pStream->Write(m_bStrike);
	pStream->Write(m_iChoosingFreeTechCount); // K-Mod (bool for 2 <= uiFlag < 4. then int.)

	pStream->Write(m_eID);
	pStream->Write(m_ePersonalityType);
	pStream->Write(m_eCurrentEra);
	pStream->Write(m_eLastStateReligion);
	pStream->Write(m_eParent);
	//m_eTeamType not saved

	m_aiSeaPlotYield.Write(pStream);
	m_aiYieldRateModifier.Write(pStream);
	m_aiCapitalYieldRateModifier.Write(pStream);
	m_aiExtraYieldThreshold.Write(pStream);
	m_aiTradeYieldModifier.Write(pStream);
	m_aiFreeCityCommerce.Write(pStream);
	m_aiCommercePercent.Write(pStream);
	m_aiCommerceRate.Write(pStream);
	m_aiCommerceRateModifier.Write(pStream);
	m_aiCapitalCommerceRateModifier.Write(pStream);
	m_aiStateReligionBuildingCommerce.Write(pStream);
	m_aiSpecialistExtraCommerce.Write(pStream);
	m_aiCommerceFlexibleCount.Write(pStream);
	m_aiGoldPerTurnByPlayer.Write(pStream);
	m_aiEspionageSpendingWeightAgainstTeam.Write(pStream);
	m_abFeatAccomplished.Write(pStream);
	m_abOptions.Write(pStream);

	pStream->WriteString(m_szScriptData);

	m_aiBonusExport.Write(pStream);
	m_aiBonusImport.Write(pStream);
	m_aiImprovementCount.Write(pStream);
	m_aiFreeBuildingCount.Write(pStream);
	m_aiExtraBuildingHappiness.Write(pStream);
	m_aiExtraBuildingHealth.Write(pStream);
	m_aiFeatureHappiness.Write(pStream);
	m_aiUnitClassCount.Write(pStream);
	m_aiUnitClassMaking.Write(pStream);
	m_aiBuildingClassCount.Write(pStream);
	m_aiBuildingClassMaking.Write(pStream);
	m_aiHurryCount.Write(pStream);
	m_aiSpecialBuildingNotRequiredCount.Write(pStream);
	m_aiHasCivicOptionCount.Write(pStream);
	m_aiNoCivicUpkeepCount.Write(pStream);
	m_aiHasReligionCount.Write(pStream);
	m_aiHasCorporationCount.Write(pStream);
	m_aiUpkeepCount.Write(pStream);
	m_aiSpecialistValidCount.Write(pStream);
	m_abResearchingTech.Write(pStream);
	m_abEverSeenDemographics.Write(pStream); // advc.091
	m_abLoyalMember.Write(pStream);
	m_aeCivics.Write(pStream);
	m_aaeSpecialistExtraYield.Write(pStream);
	m_aaeImprovementYieldChange.Write(pStream);

	m_groupCycle.Write(pStream);
	m_researchQueue.Write(pStream);

	{
		CLLNode<CvWString>* pNode;
		uint iSize = m_cityNames.getLength();
		pStream->Write(iSize);
		pNode = m_cityNames.head();
		while (pNode != NULL)
		{
			pStream->WriteString(pNode->m_data);
			pNode = m_cityNames.next(pNode);
		}
	}
	{
		PROFILE("CvPlayer::Write.FLTA"); // advc
		WriteStreamableFFreeListTrashArray(m_plotGroups, pStream);
		REPRO_TEST_END_WRITE();
		WriteStreamableFFreeListTrashArray(m_cities, pStream);
		WriteStreamableFFreeListTrashArray(m_units, pStream);
		WriteStreamableFFreeListTrashArray(m_selectionGroups, pStream);
		WriteStreamableFFreeListTrashArray(m_eventsTriggered, pStream);
	}
	{
		CvMessageQueue::_Alloc::size_type iSize = m_listGameMessages.size();
		pStream->Write(iSize);
		CvMessageQueue::iterator it;
		for (it = m_listGameMessages.begin(); it != m_listGameMessages.end(); ++it)
		{
			CvTalkingHeadMessage& message = *it;
			message.write(*pStream);
		}
	}
	{
		CvPopupQueue currentPopups;
		// Don't save open popups in MP to avoid having different state on different machines
		if (GC.getGame().isNetworkMultiPlayer())
			currentPopups.clear();
		else gDLL->UI().getDisplayedButtonPopups(currentPopups);
		/*	<advc.001> Don't store popups for AI players. The EXE sometimes adds popups
			to AI players through CvPlayer::getPopups; not sure when and why. Those popups
			linger and appear when switching to an AI player through Alt+Z. */
		if (GC.getGame().getActivePlayer() != getID())
		{
			currentPopups.clear();
			clearPopups();
		} // </advc.001>
		CvPopupQueue::_Alloc::size_type iSize = m_listPopups.size() + currentPopups.size();
		pStream->Write(iSize);
		CvPopupQueue::iterator it;
		for (it = currentPopups.begin(); it != currentPopups.end(); ++it)
		{
			CvPopupInfo* pInfo = *it;
			if (pInfo != NULL)
				pInfo->write(*pStream);
		}
		for (it = m_listPopups.begin(); it != m_listPopups.end(); ++it)
		{
			CvPopupInfo* pInfo = *it;
			if (pInfo != NULL)
				pInfo->write(*pStream);
		}
	}
	REPRO_TEST_BEGIN_WRITE(CvString::format("PlayerPt2(%d)", getID())); // (skip popups, messages)
	{
		CvDiploQueue::_Alloc::size_type iSize = m_listDiplomacy.size();
		pStream->Write(iSize);
		CvDiploQueue::iterator it;
		for (it = m_listDiplomacy.begin(); it != m_listDiplomacy.end(); ++it)
		{
			CvDiploParameters* pDiplo = *it;
			if (pDiplo != NULL)
				pDiplo->write(*pStream);
		}
	}
	// <advc.004s>
	FOR_EACH_ENUM(PlayerHistory)
		m_playerHistory[eLoopPlayerHistory].write(pStream); // </advc.004s>

	{
		uint iSize = m_mapEventsOccured.size();
		pStream->Write(iSize);
		CvEventMap::iterator it;
		for (it = m_mapEventsOccured.begin(); it != m_mapEventsOccured.end(); ++it)
		{
			pStream->Write(it->first);
			it->second.write(pStream);
		}
	}

	{
		uint iSize = m_mapEventCountdown.size();
		pStream->Write(iSize);
		CvEventMap::iterator it;
		for (it = m_mapEventCountdown.begin(); it != m_mapEventCountdown.end(); ++it)
		{
			pStream->Write(it->first);
			it->second.write(pStream);
		}
	}

	{
		uint iSize = m_aFreeUnitCombatPromotions.size();
		pStream->Write(iSize);
		UnitCombatPromotionArray::iterator it;
		for (it = m_aFreeUnitCombatPromotions.begin(); it != m_aFreeUnitCombatPromotions.end(); ++it)
		{
			pStream->Write(it->first);
			pStream->Write(it->second);
		}
	}

	{
		uint iSize = m_aFreeUnitClassPromotions.size();
		pStream->Write(iSize);
		UnitClassPromotionArray::iterator it;
		for (it = m_aFreeUnitClassPromotions.begin(); it != m_aFreeUnitClassPromotions.end(); ++it)
		{
			pStream->Write(it->first);
			pStream->Write(it->second);
		}
	}

	{
		uint iSize = m_aVote.size();
		pStream->Write(iSize);
		std::vector< std::pair<int, PlayerVoteTypes> >::iterator it;
		for (it = m_aVote.begin(); it != m_aVote.end(); ++it)
		{
			pStream->Write(it->first);
			pStream->Write(it->second);
		}
	}

	{
		uint iSize = m_aUnitExtraCosts.size();
		pStream->Write(iSize);
		std::vector< std::pair<UnitClassTypes, int> >::iterator it;
		for (it = m_aUnitExtraCosts.begin(); it != m_aUnitExtraCosts.end(); ++it)
		{
			pStream->Write(it->first);
			pStream->Write(it->second);
		}
	}

	{
		uint iSize = m_triggersFired.size();
		pStream->Write(iSize);
		std::vector<EventTriggerTypes>::iterator it;
		for (it = m_triggersFired.begin(); it != m_triggersFired.end(); ++it)
		{
			pStream->Write((*it));
		}
	}

	pStream->Write(m_iPopRushHurryCount);
	pStream->Write(m_iGoldRushHurryCount); // advc.064b
	pStream->Write(m_iInflationModifier);
	REPRO_TEST_END_WRITE();
}

void CvPlayer::createGreatPeople(UnitTypes eGreatPersonUnit,
	bool bIncrementThreshold, bool bIncrementExperience,
	CvPlot& kAt) // advc: was iX,iY
{
	CvUnit* pGreatPeopleUnit = initUnit(eGreatPersonUnit, kAt.getX(), kAt.getY());
	if (pGreatPeopleUnit == NULL)
	{
		FAssert(pGreatPeopleUnit != NULL);
		return;
	}

	if (bIncrementThreshold)
	{
		incrementGreatPeopleCreated();
		changeGreatPeopleThresholdModifier(
				GC.getDefineINT("GREAT_PEOPLE_THRESHOLD_INCREASE") *
				((getGreatPeopleCreated() / 10) + 1));
		for (MemberIter itMember(getTeam()); itMember.hasNext(); ++itMember)
		{
			itMember->changeGreatPeopleThresholdModifier(
					GC.getDefineINT("GREAT_PEOPLE_THRESHOLD_INCREASE_TEAM") *
					((getGreatPeopleCreated() / 10) + 1));
		}
	}

	if (bIncrementExperience)
	{
		incrementGreatGeneralsCreated();
		changeGreatGeneralsThresholdModifier(
				GC.getDefineINT("GREAT_GENERALS_THRESHOLD_INCREASE") *
				((getGreatGeneralsCreated() / 10) + 1));
		for (MemberIter itMember(getTeam()); itMember.hasNext(); ++itMember)
		{
			itMember->changeGreatGeneralsThresholdModifier(
					GC.getDefineINT("GREAT_GENERALS_THRESHOLD_INCREASE_TEAM") *
					((getGreatGeneralsCreated() / 10) + 1));
		}
	}
	// <advc.106>
	CvPlayer const& kGPOwner = GET_PLAYER(kAt.getOwner());
	// Use shorter message for replays
	CvWString szReplayMessage = gDLL->getText("TXT_KEY_MISC_GP_BORN_REPLAY",
			pGreatPeopleUnit->getReplayName().GetCString(),
			kGPOwner.getCivilizationDescriptionKey()); // </advc.106>
	GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szReplayMessage,
			kAt.getX(), kAt.getY(), GC.getColorType("UNIT_TEXT"));
	// Non-replay message
	CvWString szMessage;
	CvCity* pCity = kAt.getPlotCity();
	if (pCity != NULL)
	{
		CvWString szCity;
		szCity.Format(L"%s (%s)", pCity->getName().GetCString(),
				GET_PLAYER(pCity->getOwner()).getReplayName());
		szMessage = gDLL->getText("TXT_KEY_MISC_GP_BORN",
				pGreatPeopleUnit->getName().GetCString(), szCity.GetCString());
	}
	else
	{
		szMessage = gDLL->getText("TXT_KEY_MISC_GP_BORN_FIELD",
				pGreatPeopleUnit->getName().GetCString());
	} // <advc.106>
	for (PlayerIter<MAJOR_CIV,KNOWN_TO> it(kGPOwner.getTeam()); it.hasNext(); ++it)
	{
		CvPlayer const& kObs = *it;
		bool const bRev = (kAt.isRevealed(kObs.getTeam(), true) &&
				(pCity == NULL || pCity->isRevealed(kObs.getTeam(), true)));
		if (!bRev)
		{
			szMessage = gDLL->getText("TXT_KEY_MISC_GP_BORN_CIV",
					pGreatPeopleUnit->getName().GetCString(),
					kGPOwner.getCivilizationDescriptionKey());
		} // <advc.106b>
		InterfaceMessageTypes eMsgType = MESSAGE_TYPE_MINOR_EVENT;
		// Only birth of own GP is major
		// ^On second thought, make all GP births minor.
		/*if(kObs.getID() == kGPOwner.getID())
			eMsgType = MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY;*/ // </advc.106b>
		gDLL->UI().addMessage(kObs.getID(), false, -1, szMessage, "AS2D_UNIT_GREATPEOPLE",
				eMsgType, pGreatPeopleUnit->getButton(),  // <advc.106>
				//(ColorTypes)GC.getInfoTypeForString("COLOR_UNIT_TEXT"),
				NO_COLOR, // Colored through XML now
				// Indicate location only if revealed.
				bRev ? kAt.getX() : -1, bRev ? kAt.getY() : -1, bRev, bRev);
	} // </advc.106>

	if (pCity != NULL) // Python Event
		CvEventReporter::getInstance().greatPersonBorn(pGreatPeopleUnit, getID(), pCity);
}


const EventTriggeredData* CvPlayer::getEventOccured(EventTypes eEvent) const
{
	FAssertEnumBounds(eEvent);
	CvEventMap::const_iterator it = m_mapEventsOccured.find(eEvent);
	if (it == m_mapEventsOccured.end())
		return NULL;
	return &it->second;
}

bool CvPlayer::isTriggerFired(EventTriggerTypes eEventTrigger) const
{
	return (std::find(m_triggersFired.begin(), m_triggersFired.end(), eEventTrigger) !=
			m_triggersFired.end());
}

void CvPlayer::resetEventOccured(EventTypes eEvent, bool bAnnounce)
{
	FAssertEnumBounds(eEvent);
	CvEventMap::iterator it = m_mapEventsOccured.find(eEvent);
	if (it != m_mapEventsOccured.end())
	{
		expireEvent(it->first, it->second, bAnnounce);
		m_mapEventsOccured.erase(it);
	}
}

void CvPlayer::setEventOccured(EventTypes eEvent, EventTriggeredData const& kEventTriggered,
	bool bOthers)
{
	FAssertEnumBounds(eEvent);
	m_mapEventsOccured[eEvent] = kEventTriggered;
	if (GC.getInfo(eEvent).isQuest())
	{
		CvWStringBuffer szMessageBuffer;
		szMessageBuffer.append(GC.getInfo(eEvent).getDescription());
		GAMETEXT.setEventHelp(szMessageBuffer, eEvent, kEventTriggered.getID(), getID());
		gDLL->UI().addQuestMessage(getID(), szMessageBuffer.getCString(), kEventTriggered.getID());
	}

	if (bOthers)
	{
		if (GC.getInfo(eEvent).isGlobal())
		{
			for (PlayerIter<> itOther; itOther.hasNext(); ++itOther)
			{
				if (itOther->getID() == getID() || itOther->isBarbarian())
					continue;
				itOther->setEventOccured(eEvent, kEventTriggered, false);
			}
		}
		else if (GC.getInfo(eEvent).isTeam())
		{
			for (MemberIter itOtherMember(getTeam());
				itOtherMember.hasNext(); ++itOtherMember)
			{
				if (itOtherMember->getID() != getID())
					continue;
				itOtherMember->setEventOccured(eEvent, kEventTriggered, false);
			}
		}
	}
}


const EventTriggeredData* CvPlayer::getEventCountdown(EventTypes eEvent) const
{
	FAssertEnumBounds(eEvent);
	CvEventMap::const_iterator it = m_mapEventCountdown.find(eEvent);
	if (it == m_mapEventCountdown.end())
		return NULL;
	return &it->second;
}

void CvPlayer::setEventCountdown(EventTypes eEvent, EventTriggeredData const& kEventTriggered)
{
	FAssertEnumBounds(eEvent);
	m_mapEventCountdown[eEvent] = kEventTriggered;
}

void CvPlayer::resetEventCountdown(EventTypes eEvent)
{
	FAssertEnumBounds(eEvent);
	CvEventMap::iterator it = m_mapEventCountdown.find(eEvent);
	if (it != m_mapEventCountdown.end())
		m_mapEventCountdown.erase(it);
}


void CvPlayer::resetTriggerFired(EventTriggerTypes eTrigger)
{
	std::vector<EventTriggerTypes>::iterator it = std::find(
			m_triggersFired.begin(), m_triggersFired.end(), eTrigger);
	if (it != m_triggersFired.end())
		m_triggersFired.erase(it);
}


void CvPlayer::setTriggerFired(EventTriggeredData const& kTriggeredData, bool bOthers, bool bAnnounce)
{
	FAssertEnumBounds(kTriggeredData.m_eTrigger);

	CvEventTriggerInfo& kTrigger = GC.getInfo(kTriggeredData.m_eTrigger);
	if (!isTriggerFired(kTriggeredData.m_eTrigger))
	{
		m_triggersFired.push_back(kTriggeredData.m_eTrigger);
		if (bOthers)
		{
			if (kTrigger.isGlobal())
			{
				for (int i = 0; i < MAX_CIV_PLAYERS; i++)
				{
					if (i != getID())
						GET_PLAYER((PlayerTypes)i).setTriggerFired(kTriggeredData, false, false);
				}
			}
			else if (kTrigger.isTeam())
			{
				for (int i = 0; i < MAX_CIV_PLAYERS; i++)
				{
					if (i != getID() && getTeam() == TEAMID((PlayerTypes)i))
						GET_PLAYER((PlayerTypes)i).setTriggerFired(kTriggeredData, false, false);
				}
			}
		}
	}

	GC.getPythonCaller()->afterEventTriggered(kTriggeredData);

	if (bAnnounce)
	{
		CvPlot* pPlot = GC.getMap().plot(kTriggeredData.m_iPlotX, kTriggeredData.m_iPlotY);
		if (!kTriggeredData.m_szGlobalText.empty())
		{
			// advc: Moved out of the loop
			bool bMetOtherPlayer = (kTriggeredData.m_eOtherPlayer == NO_PLAYER ||
					GET_TEAM(kTriggeredData.m_eOtherPlayer).isHasMet(getTeam()));
			for (int i = 0; i < MAX_CIV_PLAYERS; i++)
			{
				CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
				if (!kLoopPlayer.isAlive())
					continue;
				if (bMetOtherPlayer && GET_TEAM(kLoopPlayer.getTeam()).isHasMet(getTeam()))
				{
					bool bShowPlot = kTrigger.isShowPlot();
					if (bShowPlot && kLoopPlayer.getTeam() != getTeam())
					{
						if (pPlot == NULL || !pPlot->isRevealed(kLoopPlayer.getTeam(), /* advc.106: */ true))
							bShowPlot = false;
					}
					if (bShowPlot)
					{
						gDLL->UI().addMessage(kLoopPlayer.getID(), false, -1,
								kTriggeredData.m_szGlobalText, "AS2D_CIVIC_ADOPT",
								MESSAGE_TYPE_MINOR_EVENT, NULL, GC.getColorType("WHITE"),
								kTriggeredData.m_iPlotX, kTriggeredData.m_iPlotY, true, true);
					}
					else
					{
						gDLL->UI().addMessage(kLoopPlayer.getID(), false, -1,
								kTriggeredData.m_szGlobalText, "AS2D_CIVIC_ADOPT",
								MESSAGE_TYPE_MINOR_EVENT);
					}
				}
			}
			// advc.106g: Don't show (most) random events in replays
			//GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), kTriggeredData.m_szGlobalText, kTriggeredData.m_iPlotX, kTriggeredData.m_iPlotY, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
		}
		else if (!kTriggeredData.m_szText.empty())
		{
			if (kTrigger.isShowPlot() && NULL != pPlot && pPlot->isRevealed(getTeam(), /* advc.106: */ true))
			{
				gDLL->UI().addMessage(getID(), false, -1, kTriggeredData.m_szText,
						"AS2D_CIVIC_ADOPT", MESSAGE_TYPE_MINOR_EVENT, NULL, NO_COLOR,
						kTriggeredData.m_iPlotX, kTriggeredData.m_iPlotY, true, true);
			}
			else
			{
				gDLL->UI().addMessage(getID(), false, -1, kTriggeredData.m_szText,
						"AS2D_CIVIC_ADOPT", MESSAGE_TYPE_MINOR_EVENT, NULL);
			}
		}
	}
}


EventTriggeredData* CvPlayer::initTriggeredData(EventTriggerTypes eEventTrigger,
	bool bFire, int iCityId, int iPlotX, int iPlotY, PlayerTypes eOtherPlayer,
	int iOtherPlayerCityId, ReligionTypes eReligion, CorporationTypes eCorporation,
	int iUnitId, BuildingTypes eBuilding)
{
	CvEventTriggerInfo& kTrigger = GC.getInfo(eEventTrigger);

	CvCity* pCity = getCity(iCityId);
	CvCity* pOtherPlayerCity = NULL;
	if (eOtherPlayer != NO_PLAYER)
		pOtherPlayerCity = GET_PLAYER(eOtherPlayer).getCity(iOtherPlayerCityId);

	CvPlot* pPlot = GC.getMap().plot(iPlotX, iPlotY);
	CvUnit* pUnit = getUnit(iUnitId);

	std::vector<CvPlot*> apPlots;
	bool const bPickPlot = GC.getInfo(eEventTrigger).isPlotEventTrigger();
	if (kTrigger.isPickCity())
	{
		if (pCity == NULL)
			pCity = pickTriggerCity(eEventTrigger);
		if (pCity == NULL)
			return NULL;

		if (bPickPlot)
		{
			for (CityPlotIter it(*pCity, false); it.hasNext(); ++it)
			{
				if (it->canTrigger(eEventTrigger, getID()))
					apPlots.push_back(&(*it));
			}
		}
	}
	else
	{
		if (kTrigger.getNumBuildings() > 0 && kTrigger.getNumBuildingsRequired() > 0)
		{
			int iFoundValid = 0;
			for (int i = 0; i < kTrigger.getNumBuildingsRequired(); i++)
			{
				if (kTrigger.getBuildingRequired(i) != NO_BUILDINGCLASS)
				{
					iFoundValid += getBuildingClassCount((BuildingClassTypes)
							kTrigger.getBuildingRequired(i));
				}
			}
			if (iFoundValid < kTrigger.getNumBuildings())
				return NULL;
		}

		if (kTrigger.getNumReligions() > 0)
		{
			int iFoundValid = 0;
			if (kTrigger.getNumReligionsRequired() > 0)
			{
				for (int i = 0; i < kTrigger.getNumReligionsRequired(); i++)
				{
					if (kTrigger.getReligionRequired(i) != NO_RELIGION)
					{
						if (getHasReligionCount((ReligionTypes)
							kTrigger.getReligionRequired(i)) > 0)
						{
							iFoundValid++;
						}
					}
				}
			}
			else
			{
				FOR_EACH_ENUM(Religion)
				{
					if (getHasReligionCount(eLoopReligion) > 0)
						iFoundValid++;
				}
			}

			if (iFoundValid < kTrigger.getNumReligions())
				return NULL;
		}

		if (kTrigger.getNumCorporations() > 0)
		{
			int iFoundValid = 0;

			if (kTrigger.getNumCorporationsRequired() > 0)
			{
				for (int i = 0; i < kTrigger.getNumCorporationsRequired(); i++)
				{
					if (kTrigger.getCorporationRequired(i) != NO_CORPORATION)
					{
						if (getHasCorporationCount((CorporationTypes)
							kTrigger.getCorporationRequired(i)) > 0)
						{
							iFoundValid++;
						}
					}
				}
			}
			else
			{
				FOR_EACH_ENUM(Corporation)
				{
					if (getHasCorporationCount(eLoopCorporation) > 0)
						iFoundValid++;
				}
			}

			if (iFoundValid < kTrigger.getNumCorporations())
				return NULL;
		}

		if (kTrigger.getMinPopulation() > 0)
		{
			if (getTotalPopulation() < kTrigger.getMinPopulation())
				return NULL;
		}

		if (kTrigger.getMaxPopulation() > 0)
		{
			if (getTotalPopulation() > kTrigger.getMaxPopulation())
				return NULL;
		}

		if (bPickPlot)
		{
			CvMap const& kMap = GC.getMap();
			for (int iPlot = 0; iPlot < kMap.numPlots(); iPlot++)
			{
				CvPlot& kLoopPlot = kMap.getPlotByIndex(iPlot);
				if (kLoopPlot.canTrigger(eEventTrigger, getID()))
					apPlots.push_back(&kLoopPlot);
			}
		}
	}

	if (kTrigger.isPickReligion())
	{
		if (eReligion == NO_RELIGION)
		{
			if (kTrigger.isStateReligion())
			{
				ReligionTypes eStateReligion = getStateReligion();
				if (NO_RELIGION != eStateReligion &&
					isValidTriggerReligion(kTrigger, pCity, eStateReligion))
				{
					eReligion = getStateReligion();
				}
			}
			else
			{
				int iOffset = GC.getGame().getSorenRandNum(GC.getNumReligionInfos(),
						"Event pick religion");
				FOR_EACH_ENUM(Religion)
				{
					ReligionTypes eRandReligion = (ReligionTypes)
							((eLoopReligion + iOffset) % GC.getNumReligionInfos());
					if (isValidTriggerReligion(kTrigger, pCity, eRandReligion))
					{
						eReligion = eRandReligion;
						break;
					}
				}
			}
		}
		if (eReligion == NO_RELIGION)
			return NULL;
	}

	if (kTrigger.isPickCorporation())
	{
		if (NO_CORPORATION == eCorporation)
		{
			int iOffset = GC.getGame().getSorenRandNum(GC.getNumCorporationInfos(),
					"Event pick corporation");
			FOR_EACH_ENUM(Corporation)
			{
				CorporationTypes eRandCorp = (CorporationTypes)
						((eLoopCorporation + iOffset) % GC.getNumCorporationInfos());
				if (isValidTriggerCorporation(kTrigger, pCity, eRandCorp))
				{
					eCorporation = eRandCorp;
					break;
				}
			}
		}

		if (eCorporation == NO_CORPORATION)
			return NULL;
	}

	if (pPlot == NULL)
	{
		if (apPlots.size() > 0)
		{
			int iChosen = GC.getGame().getSorenRandNum(apPlots.size(), "Event pick plot");
			pPlot = apPlots[iChosen];
			if (pCity == NULL)
			{
				pCity = GC.getMap().findCity(pPlot->getX(), pPlot->getY(),
						getID(), NO_TEAM, false);
			}
		}
		else
		{
			if (bPickPlot)
				return NULL;
			if (pCity != NULL)
				pPlot = pCity->plot();
		}
	}

	if (kTrigger.getNumBuildings() > 0)
	{
		if (pCity != NULL && eBuilding == NO_BUILDING)
		{
			std::vector<BuildingTypes> aeBuildings;
			for (int i = 0; i < kTrigger.getNumBuildingsRequired(); i++)
			{
				if (kTrigger.getBuildingRequired(i) != NO_BUILDINGCLASS)
				{
					BuildingTypes eTestBuilding = getCivilization().getBuilding(
							(BuildingClassTypes)kTrigger.getBuildingRequired(i));
					if (eTestBuilding != NO_BUILDING && pCity->getNumRealBuilding(eTestBuilding) > 0)
					{
						aeBuildings.push_back(eTestBuilding);
					}
				}
			}
			if (aeBuildings.size() > 0)
			{
				int iChosen = GC.getGame().getSorenRandNum(aeBuildings.size(), "Event pick building");
				eBuilding = aeBuildings[iChosen];
			}
			else return NULL;
		}
	}

	if (pUnit == NULL)
		pUnit = pickTriggerUnit(eEventTrigger, pPlot, bPickPlot);

	if (pUnit == NULL && kTrigger.getNumUnits() > 0)
		return NULL;

	if (pPlot == NULL && pUnit != NULL)
		pPlot = pUnit->plot();

	if (pPlot == NULL && bPickPlot)
		return NULL;

	if (kTrigger.getNumUnitsGlobal() > 0)
	{
		int iNumUnits = 0;
		for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
		{
			FOR_EACH_UNIT(pLoopUnit, *itPlayer)
			{
				if (pLoopUnit->getTriggerValue(eEventTrigger, pPlot, true) != MIN_INT)
					iNumUnits++;
			}
		}
		if (iNumUnits < kTrigger.getNumUnitsGlobal())
			return NULL;
	}

	if (kTrigger.getNumBuildingsGlobal() > 0)
	{
		int iNumBuildings = 0;
		for (PlayerIter<CIV_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
		{
			for (int i = 0; i < kTrigger.getNumBuildingsRequired(); i++)
			{
				if (kTrigger.getBuildingRequired(i) != NO_BUILDINGCLASS)
				{
					iNumBuildings += /* advc.001: */ itPlayer->
							getBuildingClassCount((BuildingClassTypes)
							kTrigger.getBuildingRequired(i));
				}
			}
		}
		if (iNumBuildings < kTrigger.getNumBuildingsGlobal())
			return NULL;
	}

	if (kTrigger.isPickPlayer())
	{
		std::vector<PlayerTypes> aePlayers;
		std::vector<CvCity*> apCities;

		if (eOtherPlayer == NO_PLAYER)
		{	// advc.001: No minors (from Dawn of Civilization)
			for (PlayerIter<MAJOR_CIV> itPlayer; itPlayer.hasNext(); ++itPlayer)
			{	
				if (!itPlayer->canTrigger(eEventTrigger, getID(), eReligion))
					continue;
				if (kTrigger.isPickOtherPlayerCity())
				{
					CvCity* pBestCity = NULL;
					if (pCity != NULL)
					{
						pBestCity = GC.getMap().findCity(pCity->getX(), pCity->getY(),
								itPlayer->getID());
					}
					else pBestCity = itPlayer->pickTriggerCity(eEventTrigger);
					if (pBestCity != NULL)
					{
						apCities.push_back(pBestCity);
						aePlayers.push_back(itPlayer->getID());
					}
				}
				else
				{
					apCities.push_back(NULL);
					aePlayers.push_back(itPlayer->getID());
				}
			}
			if (aePlayers.size() > 0)
			{
				int iChosen = GC.getGame().getSorenRandNum(aePlayers.size(),
						"Event pick player");
				eOtherPlayer = aePlayers[iChosen];
				pOtherPlayerCity = apCities[iChosen];
			}
			else return NULL;
		}
	}

	EventTriggeredData* pTriggerData = addEventTriggered();

	if (pTriggerData != NULL)
	{
		pTriggerData->m_eTrigger = eEventTrigger;
		pTriggerData->m_ePlayer = getID();
		pTriggerData->m_iTurn = GC.getGame().getGameTurn();
		pTriggerData->m_iCityId = (NULL != pCity) ? pCity->getID() : -1;
		pTriggerData->m_iPlotX = (NULL != pPlot) ? pPlot->getX() : INVALID_PLOT_COORD;
		pTriggerData->m_iPlotY = (NULL != pPlot) ? pPlot->getY() : INVALID_PLOT_COORD;
		pTriggerData->m_eOtherPlayer = eOtherPlayer;
		pTriggerData->m_iOtherPlayerCityId = (NULL != pOtherPlayerCity) ? pOtherPlayerCity->getID() : -1;
		pTriggerData->m_eReligion = eReligion;
		pTriggerData->m_eCorporation = eCorporation;
		pTriggerData->m_iUnitId = (NULL != pUnit) ? pUnit->getID() : -1;
		pTriggerData->m_eBuilding = eBuilding;
	}
	else return NULL;

	if (!GC.getPythonCaller()->doEventTrigger(getID(), *pTriggerData,
		// The Python call may change these data
		pCity, pPlot, pUnit, eOtherPlayer, pOtherPlayerCity, eReligion, eCorporation, eBuilding))
	{
		deleteEventTriggered(pTriggerData->getID());
		return NULL;
	}

	std::vector<CvWString> aszTexts;
	for (int i = 0; i < kTrigger.getNumTexts(); i++)
	{
		if (kTrigger.getTextEra(i) == NO_ERA ||
			kTrigger.getTextEra(i) == getCurrentEra())
		{
			aszTexts.push_back(kTrigger.getText(i));
		}
	}

	if (aszTexts.size() > 0)
	{
		int iText = GC.getGame().getSorenRandNum(aszTexts.size(), "Event Text choice");
		pTriggerData->m_szText = gDLL->getText(aszTexts[iText].GetCString(),
			eOtherPlayer != NO_PLAYER ? GET_PLAYER(eOtherPlayer).getCivilizationAdjectiveKey() : L"",
			NULL != pCity ? pCity->getNameKey() : L"",
			NULL != pUnit ? pUnit->getNameKey() : L"",
			NO_RELIGION != eReligion ? GC.getInfo(eReligion).getAdjectiveKey() : L"",
			NO_BUILDING != eBuilding ? GC.getInfo(eBuilding).getTextKeyWide() : L"",
			NULL != pOtherPlayerCity ? pOtherPlayerCity->getNameKey() : L"",
			NULL != pPlot && NO_TERRAIN != pPlot->getTerrainType() ? GC.getInfo(pPlot->getTerrainType()).getTextKeyWide() : L"",
			NULL != pPlot && pPlot->isImproved() ? GC.getInfo(pPlot->getImprovementType()).getTextKeyWide() : L"",
			NULL != pPlot && NO_BONUS != pPlot->getBonusType() ? GC.getInfo(pPlot->getBonusType()).getTextKeyWide() : L"",
			NULL != pPlot && pPlot->isRoute() ? GC.getInfo(pPlot->getRouteType()).getTextKeyWide() : L"",
			NO_CORPORATION != eCorporation ? GC.getInfo(eCorporation).getTextKeyWide() : L""
			);

	}
	else pTriggerData->m_szText = L"";

	if (kTrigger.getNumWorldNews() > 0)
	{
		int iText = GC.getGame().getSorenRandNum(kTrigger.getNumWorldNews(), "Trigger World News choice");
		pTriggerData->m_szGlobalText = gDLL->getText(kTrigger.getWorldNews(iText).GetCString(),
			getCivilizationAdjectiveKey(),
			NULL != pCity ? pCity->getNameKey() : L"",
			pTriggerData->m_eReligion != NO_RELIGION ? GC.getInfo(pTriggerData->m_eReligion).getAdjectiveKey() : L"",
			eOtherPlayer != NO_PLAYER ? GET_PLAYER(eOtherPlayer).getCivilizationAdjectiveKey() : L"",
			NULL != pOtherPlayerCity ? pOtherPlayerCity->getNameKey() : L"",
			pTriggerData->m_eCorporation != NO_CORPORATION ? GC.getInfo(pTriggerData->m_eCorporation).getTextKeyWide() : L""
			);
	}
	else pTriggerData->m_szGlobalText.clear();

	if (bFire)
		trigger(*pTriggerData);

	return pTriggerData;
}


bool CvPlayer::canDoEvent(EventTypes eEvent, const EventTriggeredData& kTriggeredData) const
{
	if (eEvent == NO_EVENT)
	{
		FAssert(false);
		return false;
	}

	CvEventInfo& kEvent = GC.getInfo(eEvent);

	int const iGold = std::min(getEventCost(eEvent, kTriggeredData.m_eOtherPlayer, false),
			getEventCost(eEvent, kTriggeredData.m_eOtherPlayer, true));

	if (iGold != 0)
	{
		if (iGold > 0 && NO_PLAYER != kTriggeredData.m_eOtherPlayer && kEvent.isGoldToPlayer())
		{
			if (GET_PLAYER(kTriggeredData.m_eOtherPlayer).getGold() < iGold)
				return false;
		}
		else if (iGold < 0)
		{
			if (getGold() < -iGold)
				return false;
		}
	}

	if (kEvent.getSpaceProductionModifier() != 0)
	{
		bool bValid = false;
		FOR_EACH_ENUM2(Project, eProject)
		{
			CvProjectInfo& kProject = GC.getInfo(eProject);
			if (kProject.isSpaceship() && kProject.getVictoryPrereq() != NO_VICTORY)
			{
				if (GC.getGame().isVictoryValid((VictoryTypes)kProject.getVictoryPrereq()))
				{
					bValid = true;
					break;
				}
			}
		}
		if (!bValid)
			return false;
	}

	if (kEvent.getEspionagePoints() > 0 &&
		GC.getGame().isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		return false;
	}

	if (NO_PLAYER != kTriggeredData.m_eOtherPlayer)
	{
		if (kEvent.getEspionagePoints() +
			GET_TEAM(getTeam()).getEspionagePointsAgainstTeam(
			TEAMID(kTriggeredData.m_eOtherPlayer)) < 0)
		{
			return false;
		}
	}

	if (kEvent.getTechPercent() != 0 || kEvent.getTechCostPercent() != 0)
	{
		if (getBestEventTech(eEvent, kTriggeredData.m_eOtherPlayer) == NO_TECH)
			return false;
	}

	if (kEvent.getPrereqTech() != NO_TECH &&
		!GET_TEAM(getTeam()).isHasTech((TechTypes)kEvent.getPrereqTech()))
	{
		return false;
	}

	if (kEvent.getBonusGift() != NO_BONUS)
	{
		BonusTypes const eBonus = (BonusTypes)kEvent.getBonusGift();
		if (kTriggeredData.m_eOtherPlayer == NO_PLAYER ||
			!canTradeNetworkWith(kTriggeredData.m_eOtherPlayer) ||
			GET_PLAYER(kTriggeredData.m_eOtherPlayer).getNumAvailableBonuses(eBonus) > 0 ||
			getNumTradeableBonuses(eBonus) <= 1)
		{
			return false;
		}
	}
	{
		UnitClassTypes eUnitClass = (UnitClassTypes)kEvent.getUnitClass(); // advc
		if (eUnitClass != NO_UNITCLASS)
		{
			UnitTypes eUnit = getCivilization().getUnit(eUnitClass);
			if (eUnit == NO_UNIT)
				return false;
		}
	}
	if (kEvent.isCityEffect())
	{
		CvCity* pCity =	getCity(kTriggeredData.m_iCityId);
		if (pCity == NULL || !pCity->canApplyEvent(eEvent, kTriggeredData))
			return false;
	}
	else if (kEvent.isOtherPlayerCityEffect())
	{
		if (kTriggeredData.m_eOtherPlayer == NO_PLAYER)
			return false;

		CvCity* pCity = GET_PLAYER(kTriggeredData.m_eOtherPlayer).
				getCity(kTriggeredData.m_iOtherPlayerCityId);
		if (pCity == NULL || !pCity->canApplyEvent(eEvent, kTriggeredData))
			return false;
	}

	if (GC.getInfo(kTriggeredData.m_eTrigger).isPlotEventTrigger())
	{
		CvPlot* pPlot = GC.getMap().plot(kTriggeredData.m_iPlotX, kTriggeredData.m_iPlotY);
		if (pPlot != NULL && !pPlot->canApplyEvent(eEvent))
			return false;
	}

	CvUnit* pUnit = getUnit(kTriggeredData.m_iUnitId);
	if (pUnit != NULL && !pUnit->canApplyEvent(eEvent))
		return false;

	if (kEvent.getBonusRevealed() != NO_BONUS)
	{
		if (GET_TEAM(getTeam()).isHasTech(GC.getInfo((BonusTypes)
			kEvent.getBonusRevealed()).getTechReveal()))
		{
			return false;
		}
		if (GET_TEAM(getTeam()).isForceRevealedBonus((BonusTypes)
			kEvent.getBonusRevealed()))
		{
			return false;
		}
	}

	if (kEvent.getConvertOwnCities() > 0)
	{
		bool bFoundValid = false;
		if (kTriggeredData.m_eReligion != NO_RELIGION)
		{
			FOR_EACH_CITY(pLoopCity, *this)
			{
				if (!pLoopCity->isHasReligion(kTriggeredData.m_eReligion))
				{
					if (-1 == kEvent.getMaxNumReligions() ||
						pLoopCity->getReligionCount() <= kEvent.getMaxNumReligions())
					{
						bFoundValid = true;
						break;
					}
				}
			}
		}
		if (!bFoundValid)
			return false;
	}

	if (kEvent.getConvertOtherCities() > 0)
	{
		bool bFoundValid = false;
		if (kTriggeredData.m_eReligion != NO_RELIGION &&
			kTriggeredData.m_eOtherPlayer != NO_PLAYER)
		{
			FOR_EACH_CITY(pLoopCity, GET_PLAYER(kTriggeredData.m_eOtherPlayer))
			{
				if (!pLoopCity->isHasReligion(kTriggeredData.m_eReligion))
				{
					if (kEvent.getMaxNumReligions() == -1 ||
						pLoopCity->getReligionCount() <= kEvent.getMaxNumReligions())
					{
						bFoundValid = true;
						break;
					}
				}
			}
		}
		if (!bFoundValid)
			return false;
	}

	if (kEvent.getAttitudeModifier() != 0)
	{
		if (kTriggeredData.m_eOtherPlayer == NO_PLAYER ||
			TEAMID(kTriggeredData.m_eOtherPlayer) == getTeam())
		{
			return false;
		}
		if (GET_PLAYER(kTriggeredData.m_eOtherPlayer).isHuman() &&
			kEvent.getOurAttitudeModifier() == 0)
		{
			return false;
		}
	}

	if (kEvent.getTheirEnemyAttitudeModifier() != 0)
	{
		if (kTriggeredData.m_eOtherPlayer == NO_PLAYER)
			return false;

		TeamTypes const eWorstEnemy = GET_TEAM(kTriggeredData.m_eOtherPlayer).
				AI_getWorstEnemy();
		if (eWorstEnemy == NO_TEAM || eWorstEnemy == getTeam())
			return false;
		if (!GET_TEAM(eWorstEnemy).isAlive())
			return false;
		if (eWorstEnemy == getTeam())
			return false;
	}

	if (kEvent.isDeclareWar())
	{
		if (kTriggeredData.m_eOtherPlayer == NO_PLAYER)
			return false;
		if (!GET_TEAM(kTriggeredData.m_eOtherPlayer).canDeclareWar(getTeam()) ||
			!GET_TEAM(getTeam()).canDeclareWar(TEAMID(kTriggeredData.m_eOtherPlayer)))
		{
			return false;
		}
	}

	if (kEvent.isQuest())
	{
		FOR_EACH_ENUM(EventTrigger)
		{
			CvEventTriggerInfo const& kLoopTrigger = GC.getInfo(eLoopEventTrigger);
			if (!kLoopTrigger.isRecurring())
			{
				for (int i = 0; i < kLoopTrigger.getNumPrereqEvents(); i++)
				{
					if (kLoopTrigger.getPrereqEvent(i) == eEvent &&
						isTriggerFired(eLoopEventTrigger))
					{
						return false;
					}
				}
			}
		}
	}
	if (!GC.getPythonCaller()->canDoEvent(eEvent, kTriggeredData))
		return false;

	return true;
}


void CvPlayer::applyEvent(EventTypes eEvent, int iEventTriggeredId, bool bUpdateTrigger)
{
	FAssert(eEvent != NO_EVENT);

	EventTriggeredData* pTriggeredData = getEventTriggered(iEventTriggeredId);

	if (pTriggeredData == NULL)
	{
		deleteEventTriggered(iEventTriggeredId);
		return;
	}

	if (bUpdateTrigger)
	{
		setTriggerFired(*pTriggeredData, true);
	}

	if (!canDoEvent(eEvent, *pTriggeredData))
	{
		if (bUpdateTrigger)
			deleteEventTriggered(iEventTriggeredId);
		return;
	}

	setEventOccured(eEvent, *pTriggeredData);

	CvEventInfo& kEvent = GC.getInfo(eEvent);
	CvCity* pCity =	getCity(pTriggeredData->m_iCityId);
	CvCity* pOtherPlayerCity = NULL;
	// advc: Needed in multiple places
	TeamTypes const eTheirWorstEnemy = (pTriggeredData->m_eOtherPlayer == NO_PLAYER ?
			NO_TEAM : GET_TEAM(pTriggeredData->m_eOtherPlayer).AI_getWorstEnemy());

	if (pTriggeredData->m_eOtherPlayer != NO_PLAYER)
	{
		pOtherPlayerCity = GET_PLAYER(pTriggeredData->m_eOtherPlayer).
				getCity(pTriggeredData->m_iOtherPlayerCityId);
	}

	// advc (note): This computation of iGold seems overcomplicated - but correct.
	int const iRandomGold = getEventCost(eEvent, pTriggeredData->m_eOtherPlayer, true);
	int iGold = getEventCost(eEvent, pTriggeredData->m_eOtherPlayer, false);
	iGold += GC.getGame().getSorenRandNum(iRandomGold - iGold + 1, "Event random gold");

	if (iGold != 0)
	{
		changeGold(iGold);
		if (pTriggeredData->m_eOtherPlayer != NO_PLAYER && kEvent.isGoldToPlayer())
			GET_PLAYER(pTriggeredData->m_eOtherPlayer).changeGold(-iGold);
	}

	if (pTriggeredData->m_eOtherPlayer != NO_PLAYER &&
		kEvent.getEspionagePoints() != 0)
	{
			GET_TEAM(getTeam()).changeEspionagePointsAgainstTeam(
					TEAMID(pTriggeredData->m_eOtherPlayer), kEvent.getEspionagePoints());
	}

	if (kEvent.getTechPercent() != 0)
	{
		TechTypes const eBestTech = getBestEventTech(eEvent,
				pTriggeredData->m_eOtherPlayer);
		if (eBestTech != NO_TECH)
		{
			int const iBeakers = GET_TEAM(getTeam()).changeResearchProgressPercent(
					eBestTech, kEvent.getTechPercent(), getID());
			if (iBeakers > 0)
			{	// advc: Was effectively itMember(getID()); previously fixed by kmodx.
				for (MemberIter itMember(getTeam()); itMember.hasNext(); ++itMember)
				{
					CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_PROGRESS_TOWARDS_TECH",
							iBeakers, GC.getInfo(eBestTech).getTextKeyWide());
					gDLL->UI().addMessage(itMember->getID(), false, -1, szBuffer, NULL,
							MESSAGE_TYPE_MINOR_EVENT, NULL, GC.getColorType("TECH_TEXT"));
				}
			}
		}
	}

	if (kEvent.isGoldenAge())
		changeGoldenAgeTurns(getGoldenAgeLength());
	if (kEvent.getInflationModifier() != 0)
		m_iInflationModifier += kEvent.getInflationModifier();
	if (kEvent.getSpaceProductionModifier() != 0)
		changeSpaceProductionModifier(kEvent.getSpaceProductionModifier());
	if (kEvent.getFreeUnitSupport() != 0)
	changeBaseFreeUnits(kEvent.getFreeUnitSupport());

	if (kEvent.isDeclareWar())
	{
		if (pTriggeredData->m_eOtherPlayer != NO_PLAYER)
		{
			if (gTeamLogLevel >= 2) logBBAI("    Team %d (%S) declares war on team %d due to event", GET_PLAYER(pTriggeredData->m_eOtherPlayer).getTeam(), GET_PLAYER(pTriggeredData->m_eOtherPlayer).getCivilizationDescription(0), getTeam()); // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
			GET_TEAM(GET_PLAYER(pTriggeredData->m_eOtherPlayer).getTeam()).declareWar(getTeam(), false, WARPLAN_LIMITED,
					true, NO_PLAYER, true); // advc.106g
		}
	}

	if (kEvent.getBonusGift() != NO_BONUS)
	{
		if (pTriggeredData->m_eOtherPlayer != NO_PLAYER)
		{
			CLinkList<TradeData> ourList;
			CLinkList<TradeData> theirList;
			ourList.insertAtEnd(TradeData(TRADE_RESOURCES, kEvent.getBonusGift()));
			GC.getGame().implementDeal(getID(),
					pTriggeredData->m_eOtherPlayer, ourList, theirList);
		}
	}

	bool bClear = false;
	FOR_EACH_ENUM(Event)
	{
		if (kEvent.getClearEventChance(eLoopEvent) <= 0)
			continue;

		bClear = (GC.getGame().getSorenRandNum(100, "Event Clear") <
				kEvent.getClearEventChance(eLoopEvent));
		if (!bClear)
			continue;
		if (kEvent.isGlobal())
		{
			for (PlayerIter<> itPlayer; itPlayer.hasNext(); ++itPlayer)
			{
				if (!itPlayer->isBarbarian())
					itPlayer->resetEventOccured(eLoopEvent, itPlayer->getID() != getID());
			}
		}
		else if (kEvent.isTeam())
		{
			for (MemberIter itMember(getTeam()); itMember.hasNext(); ++itMember)
			{
				itMember->resetEventOccured(eLoopEvent, itMember->getID() != getID());
			}
		}
		else resetEventOccured(eLoopEvent, false);
	}

	if (pCity != NULL && kEvent.isCityEffect())
		pCity->applyEvent(eEvent, *pTriggeredData, bClear);
	else if (pOtherPlayerCity != NULL && kEvent.isOtherPlayerCityEffect())
		pOtherPlayerCity->applyEvent(eEvent, *pTriggeredData, bClear);

	if (!kEvent.isCityEffect() && !kEvent.isOtherPlayerCityEffect())
	{
		if (kEvent.getHappy() != 0)
			changeExtraHappiness(kEvent.getHappy());
		if (kEvent.getHealth() != 0)
			changeExtraHealth(kEvent.getHealth());
		if (kEvent.getNumBuildingYieldChanges() > 0)
		{
			FOR_EACH_ENUM(BuildingClass)
			{
				FOR_EACH_ENUM(Yield)
				{
					FOR_EACH_CITY_VAR(pLoopCity, *this)
					{
						pLoopCity->changeBuildingYieldChange(
								eLoopBuildingClass, eLoopYield,
								kEvent.getBuildingYieldChange(
								eLoopBuildingClass, eLoopYield));
					}
				}
			}
		}
		if (kEvent.getNumBuildingCommerceChanges() > 0)
		{
			FOR_EACH_ENUM(BuildingClass)
			{
				FOR_EACH_ENUM(Commerce)
				{
					FOR_EACH_CITY_VAR(pLoopCity, *this)
					{
						pLoopCity->changeBuildingCommerceChange(
								eLoopBuildingClass, eLoopCommerce,
								kEvent.getBuildingCommerceChange(
								eLoopBuildingClass, eLoopCommerce));
					}
				}
			}
		}
		if (kEvent.getNumBuildingHappyChanges() > 0)
		{
			CvCivilization const& kCiv = getCivilization(); // advc.003w
			for (int i = 0; i < kCiv.getNumBuildings(); i++)
			{
				BuildingTypes eBuilding = kCiv.buildingAt(i);
				BuildingClassTypes eBuildingClass = kCiv.buildingClassAt(i);
				if (kEvent.getBuildingHappyChange(eBuildingClass) != 0)
				{
					changeExtraBuildingHappiness(eBuilding,
							kEvent.getBuildingHappyChange(eBuildingClass));
				}
			}
		}
		if (kEvent.getNumBuildingHealthChanges() > 0)
		{
			CvCivilization const& kCiv = getCivilization(); // advc.003w
			for (int i = 0; i < kCiv.getNumBuildings(); i++)
			{
				BuildingTypes eBuilding = kCiv.buildingAt(i);
				BuildingClassTypes eBuildingClass = kCiv.buildingClassAt(i);
				if (kEvent.getBuildingHealthChange(eBuildingClass) != 0)
				{
					changeExtraBuildingHealth(eBuilding,
							kEvent.getBuildingHealthChange(eBuildingClass));
				}
			}
		}
		if (kEvent.getHurryAnger() != 0)
		{
			FOR_EACH_CITY_VAR(pLoopCity, *this)
			{
				pLoopCity->changeHurryAngerTimer(kEvent.getHurryAnger() *
						pLoopCity->flatHurryAngerLength());
			}
		}
		if (kEvent.getHappyTurns() > 0)
		{
			FOR_EACH_CITY_VAR(pLoopCity, *this)
				pLoopCity->changeHappinessTimer(kEvent.getHappyTurns());
		}
		if (kEvent.getMaxPillage() > 0)
		{
			FAssert(kEvent.getMaxPillage() >= kEvent.getMinPillage());
			int const iPillage = kEvent.getMinPillage() +
					GC.getGame().getSorenRandNum(kEvent.getMaxPillage() -
					kEvent.getMinPillage(), "Pick number of event pillaged plots");
			int iDone = 0;
			for (int i = 0; i < iPillage; i++)
			{
				int const iRandOffset = GC.getGame().getSorenRandNum(
						GC.getMap().numPlots(), "Pick event pillage plot (any city)");
				for (int iPlot = 0; iPlot < GC.getMap().numPlots(); iPlot++)
				{
					int iRandPlot = (iPlot + iRandOffset) % GC.getMap().numPlots();
					CvPlot* pPlot = GC.getMap().plotByIndex(iRandPlot);
					if (pPlot == NULL || pPlot->getOwner() != getID() || !pPlot->isCity())
						continue;
					if (pPlot->isImproved() &&
						!GC.getInfo(pPlot->getImprovementType()).isPermanent())
					{
						CvWString szBuffer = gDLL->getText("TXT_KEY_EVENT_CITY_IMPROVEMENT_DESTROYED",
								GC.getInfo(pPlot->getImprovementType()).getTextKeyWide());
						gDLL->UI().addMessage(getID(), false, -1, szBuffer, *pPlot, "AS2D_PILLAGED",
								MESSAGE_TYPE_INFO, GC.getInfo(pPlot->getImprovementType()).getButton(),
								GC.getColorType("RED"));
						pPlot->setImprovementType(NO_IMPROVEMENT);
						iDone++;
						break;
					}
				}
			}

			if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
			{
				CvWString szBuffer = gDLL->getText("TXT_KEY_EVENT_NUM_CITY_IMPROVEMENTS_DESTROYED",
						iDone, getCivilizationAdjectiveKey());
				gDLL->UI().addMessage(pTriggeredData->m_eOtherPlayer, false, -1, szBuffer,
						"AS2D_PILLAGED", MESSAGE_TYPE_INFO);
			}
		}
		if (kEvent.getFood() != 0)
		{
			FOR_EACH_CITY_VAR(pLoopCity, *this)
				pLoopCity->changeFood(kEvent.getFood());
		}
		if (kEvent.getFoodPercent() != 0)
		{
			FOR_EACH_CITY_VAR(pLoopCity, *this)
				pLoopCity->changeFood((pLoopCity->getFood() * kEvent.getFoodPercent()) / 100);
		}
		if (kEvent.getPopulationChange() != 0)
		{
			FOR_EACH_CITY_VAR(pLoopCity, *this)
			{
				if (pLoopCity->getPopulation() + kEvent.getPopulationChange() > 0)
					pLoopCity->changePopulation(kEvent.getPopulationChange());
			}
		}
		if (kEvent.getCulture() != 0)
		{
			FOR_EACH_CITY_VAR(pLoopCity, *this)
			{
				if (pLoopCity->getCultureTimes100(pLoopCity->getOwner()) +
					100 * kEvent.getCulture() > 0)
				{
					pLoopCity->changeCulture(pLoopCity->getOwner(),
							kEvent.getCulture(), true, true);
				}
			}
		}
		{
			UnitClassTypes eUnitClass = (UnitClassTypes)kEvent.getUnitClass();
			if (eUnitClass != NO_UNITCLASS)
			{
				UnitTypes eUnit = getCivilization().getUnit(eUnitClass);
				if (eUnit != NO_UNIT)
				{
					CvCity* pUnitCity = pCity;
					if (pUnitCity == NULL)
						pUnitCity = getCapital();
					if (pUnitCity != NULL)
					{
						for (int i = 0; i < kEvent.getNumUnits(); i++)
							initUnit(eUnit, pUnitCity->getX(), pUnitCity->getY());
					}
				}
			}
		}
	}

	CvPlot* pPlot = GC.getMap().plot(
			pTriggeredData->m_iPlotX, pTriggeredData->m_iPlotY);
	if (pPlot != NULL)
	{
		if (GC.getInfo(pTriggeredData->m_eTrigger).isPlotEventTrigger())
		{
			FAssert(pPlot->canApplyEvent(eEvent));
			pPlot->applyEvent(eEvent);
		}
	}

	{
		CvUnit* pUnit = getUnit(pTriggeredData->m_iUnitId);
		if (pUnit != NULL)
		{
			FAssert(pUnit->canApplyEvent(eEvent));
			pUnit->applyEvent(eEvent); // might kill the unit
		}
	}
	FOR_EACH_ENUM(UnitCombat)
	{
		PromotionTypes eEventPromo = (PromotionTypes)
				kEvent.getUnitCombatPromotion(eLoopUnitCombat);
		if (eEventPromo == NO_PROMOTION)
			continue;
		FOR_EACH_UNIT_VAR(pLoopUnit, *this)
		{
			if (pLoopUnit->getUnitCombatType() == eLoopUnitCombat)
				pLoopUnit->setHasPromotion(eEventPromo, true);
		}
		setFreePromotion(eLoopUnitCombat, eEventPromo, true);
	}
	FOR_EACH_ENUM(UnitClass)
	{
		PromotionTypes eEventPromo = (PromotionTypes)
				kEvent.getUnitClassPromotion(eLoopUnitClass);
		if (eEventPromo == NO_PROMOTION)
			continue;
		FOR_EACH_UNIT_VAR(pLoopUnit, *this)
		{
			if (pLoopUnit->getUnitClassType() == eLoopUnitClass)
				pLoopUnit->setHasPromotion(eEventPromo, true);
		}
		setFreePromotion(eLoopUnitClass, eEventPromo, true);
	}
	if (kEvent.getBonusRevealed() != NO_BONUS)
	{
		GET_TEAM(getTeam()).setForceRevealedBonus((BonusTypes)
				kEvent.getBonusRevealed(), true);
	}
	{
		std::vector<CvCity*> apSpreadReligionCities;
		if (kEvent.getConvertOwnCities() > 0 &&
			pTriggeredData->m_eReligion != NO_RELIGION)
		{
			FOR_EACH_CITY_VAR(pLoopCity, *this)
			{
				if (!pLoopCity->isHasReligion(pTriggeredData->m_eReligion))
				{
					if (kEvent.getMaxNumReligions() == -1 ||
						pLoopCity->getReligionCount() <= kEvent.getMaxNumReligions())
					{
						apSpreadReligionCities.push_back(pLoopCity);
					}
				}
			}
		}
		while (kEvent.getConvertOwnCities() < (int)apSpreadReligionCities.size())
		{
			int iChosen = GC.getGame().getSorenRandNum(apSpreadReligionCities.size(),
					"Even Spread Religion (own)");
			int i = 0;
			for (std::vector<CvCity*>::iterator it = apSpreadReligionCities.begin();
				it != apSpreadReligionCities.end(); ++it)
			{
				if (i == iChosen)
				{
					apSpreadReligionCities.erase(it);
					break;
				}
				i++;
			}
		}
		for (std::vector<CvCity*>::iterator it = apSpreadReligionCities.begin();
			it != apSpreadReligionCities.end(); ++it)
		{
			(*it)->setHasReligion(pTriggeredData->m_eReligion, true, true, false);
		}

		apSpreadReligionCities.clear();
		if (kEvent.getConvertOtherCities() > 0 &&
			pTriggeredData->m_eReligion != NO_RELIGION &&
			pTriggeredData->m_eOtherPlayer != NO_PLAYER)
		{
			// kmodx: removed redundant code
			FOR_EACH_CITY_VAR(pLoopCity, GET_PLAYER(pTriggeredData->m_eOtherPlayer))
			{
				if (!pLoopCity->isHasReligion(pTriggeredData->m_eReligion))
				{
					if (kEvent.getMaxNumReligions() == -1 ||
						pLoopCity->getReligionCount() <= kEvent.getMaxNumReligions())
					{
						apSpreadReligionCities.push_back(pLoopCity);
					}
				}
			}
		}
		while (kEvent.getConvertOtherCities() < (int)apSpreadReligionCities.size())
		{
			int iChosen = GC.getGame().getSorenRandNum(apSpreadReligionCities.size(),
					"Even Spread Religion (other)");
			int i = 0;
			for (std::vector<CvCity*>::iterator it = apSpreadReligionCities.begin();
				it != apSpreadReligionCities.end(); ++it)
			{
				if (i == iChosen)
				{
					apSpreadReligionCities.erase(it);
					break;
				}
				i++;
			}
		}
		for (std::vector<CvCity*>::iterator it = apSpreadReligionCities.begin();
			it != apSpreadReligionCities.end(); ++it)
		{
			(*it)->setHasReligion(pTriggeredData->m_eReligion, true, true, false);
		}
	}
	if (kEvent.getOurAttitudeModifier() != 0 &&
		pTriggeredData->m_eOtherPlayer != NO_PLAYER)
	{
		if (kEvent.getOurAttitudeModifier() > 0)
		{
			AI().AI_changeMemoryCount(pTriggeredData->m_eOtherPlayer,
					MEMORY_EVENT_GOOD_TO_US, kEvent.getOurAttitudeModifier());
		}
		else
		{
			AI().AI_changeMemoryCount(pTriggeredData->m_eOtherPlayer,
					MEMORY_EVENT_BAD_TO_US, -kEvent.getOurAttitudeModifier());
		}
	}
	if (kEvent.getAttitudeModifier() != 0)
	{
		if (pTriggeredData->m_eOtherPlayer != NO_PLAYER)
		{	// <advc.130j> Replace AI_changeMemoryCount with multiple AI_rememberEvent calls */
			CvPlayerAI& kOther = GET_PLAYER(pTriggeredData->m_eOtherPlayer);
			if (kEvent.getAttitudeModifier() > 0)
			{
				for(int i = 0; i < kEvent.getAttitudeModifier(); i++)
					kOther.AI_rememberEvent(getID(), MEMORY_EVENT_GOOD_TO_US);
			}
			else
			{
				for(int i = 0; i < -kEvent.getAttitudeModifier(); i++)
					kOther.AI_rememberEvent(getID(), MEMORY_EVENT_BAD_TO_US);
			} // </advc.130j>
		}
	}
	if (kEvent.getTheirEnemyAttitudeModifier() != 0 &&
		pTriggeredData->m_eOtherPlayer != NO_PLAYER &&
		eTheirWorstEnemy != NO_TEAM)
	{
		for (MemberIter itEnemyMember(eTheirWorstEnemy); itEnemyMember.hasNext();
			++itEnemyMember)
		{
			if (kEvent.getTheirEnemyAttitudeModifier() > 0)
			{
				itEnemyMember->AI_changeMemoryCount(getID(),
						MEMORY_EVENT_GOOD_TO_US, kEvent.getTheirEnemyAttitudeModifier());
				AI().AI_changeMemoryCount(itEnemyMember->getID(),
						MEMORY_EVENT_GOOD_TO_US, kEvent.getTheirEnemyAttitudeModifier());
			}
			else
			{
				itEnemyMember->AI_changeMemoryCount(getID(),
						MEMORY_EVENT_BAD_TO_US, -kEvent.getTheirEnemyAttitudeModifier());
				AI().AI_changeMemoryCount(itEnemyMember->getID(),
						MEMORY_EVENT_BAD_TO_US, -kEvent.getTheirEnemyAttitudeModifier());
			}
		}
	}

	GC.getPythonCaller()->applyEvent(eEvent, *pTriggeredData);

	if (kEvent.getNumWorldNews() > 0)
	{
		int iText = GC.getGame().getSorenRandNum(kEvent.getNumWorldNews(),
				"Event World News choice");
		CvWString szGlobalText = gDLL->getText(kEvent.getWorldNews(iText).GetCString(),
			getCivilizationAdjectiveKey(),
			NULL != pCity ? pCity->getNameKey() : L"",
			pTriggeredData->m_eOtherPlayer != NO_PLAYER ? GET_PLAYER(pTriggeredData->m_eOtherPlayer).getCivilizationAdjectiveKey() : L"",
			NULL != pOtherPlayerCity ? pOtherPlayerCity->getNameKey() : L"",
			NO_RELIGION != pTriggeredData->m_eReligion ? GC.getInfo(pTriggeredData->m_eReligion).getAdjectiveKey() : L"",
			NO_TEAM != eTheirWorstEnemy ? GET_TEAM(eTheirWorstEnemy).getName().GetCString() : L"",
			NO_CORPORATION != pTriggeredData->m_eCorporation ? GC.getInfo(pTriggeredData->m_eCorporation).getTextKeyWide() : L""
			);

		for (PlayerIter<MAJOR_CIV,KNOWN_TO> it(getTeam()); it.hasNext(); ++it)
		{
			CvPlayer& kObs = *it;
			if (pTriggeredData->m_eOtherPlayer != NO_PLAYER &&
				!GET_TEAM(pTriggeredData->m_eOtherPlayer).isHasMet(//getTeam()
				/*	advc.001: I think the above was already checked by the caller.
					Certainly doesn't belong in this loop. */
				kObs.getTeam()))
			{
				continue;
			}
			bool bShowPlot = GC.getInfo(pTriggeredData->m_eTrigger).isShowPlot();
			if (bShowPlot)
			{
				if (kObs.getTeam() != getTeam())
				{
					if (pPlot == NULL || !pPlot->isRevealed(kObs.getTeam()))
						bShowPlot = false;
				}
			}
			if (bShowPlot)
			{
				gDLL->UI().addMessage(kObs.getID(), false, -1, szGlobalText,
						"AS2D_CIVIC_ADOPT", MESSAGE_TYPE_MINOR_EVENT, NULL, NO_COLOR,
						pTriggeredData->m_iPlotX, pTriggeredData->m_iPlotY, true, true);
			}
			else
			{
				gDLL->UI().addMessage(kObs.getID(), false, -1, szGlobalText,
						"AS2D_CIVIC_ADOPT", MESSAGE_TYPE_MINOR_EVENT);
			}
		}
		// advc.106g: Don't show (most) random events in replays
		//GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szGlobalText, pTriggeredData->m_iPlotX, pTriggeredData->m_iPlotY, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
	}

	if (!CvWString(kEvent.getLocalInfoTextKey()).empty())
	{
		CvWString szLocalText = gDLL->getText(kEvent.getLocalInfoTextKey(),
			getCivilizationAdjectiveKey(),
			NULL != pCity ? pCity->getNameKey() : L"",
			pTriggeredData->m_eOtherPlayer != NO_PLAYER ? GET_PLAYER(pTriggeredData->m_eOtherPlayer).getCivilizationAdjectiveKey() : L"",
			NULL != pOtherPlayerCity ? pOtherPlayerCity->getNameKey() : L"",
			NO_RELIGION != pTriggeredData->m_eReligion ? GC.getInfo(pTriggeredData->m_eReligion).getAdjectiveKey() : L"",
			NO_TEAM != eTheirWorstEnemy ? GET_TEAM(eTheirWorstEnemy).getName().GetCString() : L"",
			NO_CORPORATION != pTriggeredData->m_eCorporation ? GC.getInfo(pTriggeredData->m_eCorporation).getTextKeyWide() : L""
			);

		if (GC.getInfo(pTriggeredData->m_eTrigger).isShowPlot())
		{
			gDLL->UI().addMessage(getID(), false, -1, szLocalText, "AS2D_CIVIC_ADOPT",
					MESSAGE_TYPE_MINOR_EVENT, NULL, NO_COLOR,
					pTriggeredData->m_iPlotX, pTriggeredData->m_iPlotY, true, true);
		}
		else
		{
			gDLL->UI().addMessage(getID(), false, -1, szLocalText, "AS2D_CIVIC_ADOPT",
					MESSAGE_TYPE_MINOR_EVENT, NULL, NO_COLOR);
		}
	}

	if (!CvWString(kEvent.getOtherPlayerPopup()).empty() &&
		pTriggeredData->m_eOtherPlayer != NO_PLAYER)
	{
		CvWString szText = gDLL->getText(kEvent.getOtherPlayerPopup(),
			getCivilizationAdjectiveKey(),
			NULL != pCity ? pCity->getNameKey() : L"",
			pTriggeredData->m_eOtherPlayer != NO_PLAYER ? GET_PLAYER(pTriggeredData->m_eOtherPlayer).getCivilizationAdjectiveKey() : L"",
			NULL != pOtherPlayerCity ? pOtherPlayerCity->getNameKey() : L"",
			NO_RELIGION != pTriggeredData->m_eReligion ? GC.getInfo(pTriggeredData->m_eReligion).getAdjectiveKey() : L"",
			NO_CORPORATION != pTriggeredData->m_eCorporation ? GC.getInfo(pTriggeredData->m_eCorporation).getTextKeyWide() : L""
			);
		CvPopupInfo* pInfo = new CvPopupInfo();
		if (pInfo != NULL)
		{
			pInfo->setText(szText);
			GET_PLAYER(pTriggeredData->m_eOtherPlayer).addPopup(pInfo);
		}
	}

	bool bDeleteTrigger = bUpdateTrigger;

	FOR_EACH_ENUM(Event)
	{
		if (kEvent.getAdditionalEventTime(eLoopEvent) == 0)
		{
			if (kEvent.getAdditionalEventChance(eLoopEvent) > 0 &&
				canDoEvent(eLoopEvent, *pTriggeredData) &&
				GC.getGame().getSorenRandNum(100, "Additional Event") <
				kEvent.getAdditionalEventChance(eLoopEvent))
			{
				applyEvent(eLoopEvent, iEventTriggeredId, false);
			}
		}
		else
		{
			bool bSetTimer = true;
			if (kEvent.getAdditionalEventChance(eLoopEvent) > 0 &&
				GC.getGame().getSorenRandNum(100, "Additional Event 2") >=
				kEvent.getAdditionalEventChance(eLoopEvent))
			{
				bSetTimer = false;
			}
			if (bSetTimer)
			{
				EventTriggeredData kTriggered = *pTriggeredData;
				kTriggered.m_iTurn = (GC.getInfo(GC.getGame().getGameSpeedType()).getGrowthPercent() *
						kEvent.getAdditionalEventTime(eLoopEvent)) / 100 + GC.getGame().getGameTurn();
				EventTriggeredData const* pExistingTriggered = getEventCountdown(eLoopEvent);
				if (pExistingTriggered != NULL)
				{
					kTriggered.m_iTurn = std::min(kTriggered.m_iTurn,
							pExistingTriggered->m_iTurn);
				}
				setEventCountdown(eLoopEvent, kTriggered);
				bDeleteTrigger = false;
			}
		}
	}

	if (bDeleteTrigger)
		deleteEventTriggered(iEventTriggeredId);
}

bool CvPlayer::isValidEventTech(TechTypes eTech, EventTypes eEvent, PlayerTypes eOtherPlayer) const
{
	CvEventInfo const& kEvent = GC.getInfo(eEvent);

	if (kEvent.getTechPercent() == 0 && kEvent.getTechCostPercent() == 0)
		return false;
	if (kEvent.getTechPercent() < 0 &&
		(GET_TEAM(getTeam()).isHasTech(eTech) || // advc: Don't rely on getResearchProgress
		GET_TEAM(getTeam()).getResearchProgress(eTech) <= 0))
	{
		return false;
	}
	if (!canResearch(eTech))
		return false;
	/*  advc.004x (comment): No change here; OK with me to make all tech invalid
		during anarchy (ResearchTurnsLeft=-1). */
	if (getResearchTurnsLeft(eTech, true) < kEvent.getTechMinTurnsLeft())
		return false;
	if (eOtherPlayer != NO_PLAYER && !GET_TEAM(eOtherPlayer).isHasTech(eTech))
		return false;

	return true;
}


TechTypes CvPlayer::getBestEventTech(EventTypes eEvent, PlayerTypes eOtherPlayer) const
{
	TechTypes eBestTech = NO_TECH;
	CvEventInfo const& kEvent = GC.getInfo(eEvent);

	if (kEvent.getTechPercent() == 0 && kEvent.getTechCostPercent() == 0)
		return NO_TECH;

	if (kEvent.getTech() != NO_TECH)
		eBestTech = (TechTypes)kEvent.getTech();
	else
	{
		bool bFoundFlavor = false;
		FOR_EACH_ENUM(Flavor)
		{
			if (kEvent.getTechFlavorValue(eLoopFlavor) != 0)
			{
				bFoundFlavor = true;
				break;
			}
		}
		if (!bFoundFlavor)
			eBestTech = getCurrentResearch();
	}

	if (eBestTech != NO_TECH)
	{
		if (!isValidEventTech(eBestTech, eEvent, eOtherPlayer))
			eBestTech = NO_TECH;
	}
	else
	{
		int iBestValue = 0;
		FOR_EACH_ENUM(Tech)
		{
			if (!isValidEventTech(eLoopTech, eEvent, eOtherPlayer))
				continue;
			int iValue = 0;
			FOR_EACH_ENUM(Flavor)
			{
				iValue += kEvent.getTechFlavorValue(eLoopFlavor) *
						GC.getInfo(eLoopTech).getFlavorValue(eLoopFlavor);
			}
			if (iValue > iBestValue)
			{
				eBestTech = eLoopTech;
				iBestValue = iValue;
			}
		}
	}

	return eBestTech;
}

int CvPlayer::getEventCost(EventTypes eEvent, PlayerTypes eOtherPlayer, bool bRandom) const
{
	CvEventInfo const& kEvent = GC.getInfo(eEvent);

	int iGold = kEvent.getGold();
	if (bRandom)
		iGold += kEvent.getRandomGold();
	iGold *= std::max(0, calculateInflationRate() + 100);
	iGold /= 100;

	TechTypes eBestTech = getBestEventTech(eEvent, eOtherPlayer);
	if (eBestTech != NO_TECH)
	{
		iGold -= (kEvent.getTechCostPercent() *
				GET_TEAM(getTeam()).getResearchCost(eBestTech)) / 100;
	}

	return iGold;
}


void CvPlayer::doEvents()
{
	if (GC.getGame().isOption(GAMEOPTION_NO_EVENTS))
		return;
	if (isBarbarian() || isMinorCiv())
		return;

	{ // advc: scope for iterator
		CvEventMap::iterator it = m_mapEventsOccured.begin();
		while (it != m_mapEventsOccured.end())
		{
			if (checkExpireEvent(it->first, it->second))
			{
				expireEvent(it->first, it->second, true);
				it = m_mapEventsOccured.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
	bool bNewEventEligible = true;
	if (GC.getGame().getElapsedGameTurns() < GC.getDefineINT("FIRST_EVENT_DELAY_TURNS"))
		bNewEventEligible = false;

	if (bNewEventEligible &&
		GC.getGame().getSorenRandNum(GC.getDefineINT("EVENT_PROBABILITY_ROLL_SIDES"),
		"Global event check") >= GC.getInfo(getCurrentEra()).getEventChancePerTurn())
	{
		bNewEventEligible = false;
	}

	std::vector<std::pair<EventTriggeredData*,int> > aePossibleEventTriggerWeights;
	int iTotalWeight = 0;
	FOR_EACH_ENUM(EventTrigger)
	{
		int iWeight = getEventTriggerWeight(eLoopEventTrigger);
		if (iWeight == -1)
			trigger(eLoopEventTrigger);
		else if (iWeight > 0 && bNewEventEligible)
		{
			EventTriggeredData* pTriggerData = initTriggeredData(eLoopEventTrigger);
			if (pTriggerData != NULL)
			{
				iTotalWeight += iWeight;
				aePossibleEventTriggerWeights.push_back(std::make_pair(
						pTriggerData, iTotalWeight));
			}
		}
	}

	if (iTotalWeight > 0)
	{
		bool bFired = false;
		int iValue = GC.getGame().getSorenRandNum(iTotalWeight, "Event trigger");
		for (std::vector<std::pair<EventTriggeredData*,int> >::iterator it =
			aePossibleEventTriggerWeights.begin();
			it != aePossibleEventTriggerWeights.end(); ++it)
		{
			EventTriggeredData* pTriggerData = it->first;
			if (pTriggerData != NULL)
			{
				if (iValue < it->second && !bFired)
				{
					trigger(*pTriggerData);
					bFired = true;
				}
				else deleteEventTriggered(pTriggerData->getID());
			}
		}
	}

	std::vector<int> aCleanup;
	FOR_EACH_ENUM(Event)
	{
		EventTriggeredData const* pTriggeredData = getEventCountdown(eLoopEvent);
		if (pTriggeredData != NULL)
		{
			if (GC.getGame().getGameTurn() >= pTriggeredData->m_iTurn)
			{
				applyEvent(eLoopEvent, pTriggeredData->m_iId);
				resetEventCountdown(eLoopEvent);
				aCleanup.push_back(pTriggeredData->m_iId);
			}
		}
	}

	for (std::vector<int>::iterator it = aCleanup.begin(); it != aCleanup.end(); ++it)
	{
		bool bDelete = true;
		FOR_EACH_ENUM(Event)
		{
			EventTriggeredData const* pTriggeredData = getEventCountdown(eLoopEvent);
			if (pTriggeredData != NULL)
			{
				if (pTriggeredData->m_iId == *it)
				{
					bDelete = false;
					break;
				}
			}
		}
		if (bDelete)
			deleteEventTriggered(*it);
	}
}

// advc.011:
void CvPlayer::decayBuildProgress()
{
	CvMap const& kMap = GC.getMap();
	for(int i = 0; i < kMap.numPlots(); i++)
	{
		CvPlot& p = kMap.getPlotByIndex(i);
		if(!p.isWater() && p.getOwner() == getID())
			p.decayBuildProgress();
	}
}

// advc.002e:
void CvPlayer::showForeignPromoGlow(bool b)
{
	if (BUGOption::isEnabled("PLE__ShowPromotionGlow", false))
		return;
	for (PlayerIter<ALIVE> itOther; itOther.hasNext(); ++itOther)
	{
		if (itOther->getID() == getID())
			continue;
		FOR_EACH_UNIT_VAR(u, *itOther)
		{
			gDLL->getEntityIFace()->showPromotionGlow(u->getEntity(),
					u->isPromotionReady() && b);
		}
	}
}

void CvPlayer::expireEvent(EventTypes eEvent,
	EventTriggeredData const& kTriggeredData, bool bFail)
{
	FAssert(getEventOccured(eEvent) == &kTriggeredData);
	FAssert(GC.getInfo(eEvent).isQuest() || GC.getGame().getGameTurn() - kTriggeredData.m_iTurn <= 4);

	if (GC.getInfo(eEvent).isQuest())
	{
		CvMessageQueue::iterator it;
		for (it = m_listGameMessages.begin(); it != m_listGameMessages.end(); ++it)
		{
			CvTalkingHeadMessage& message = *it;

			// the trigger ID is stored in the otherwise unused length field
			if (message.getLength() == kTriggeredData.getID())
			{
				m_listGameMessages.erase(it);
				gDLL->UI().dirtyTurnLog(getID());
				break;
			}
		}
		if (bFail)
		{
			gDLL->UI().addMessage(getID(), false, -1,
					gDLL->getText(GC.getInfo(eEvent).getQuestFailTextKey()),
					"AS2D_CIVIC_ADOPT", MESSAGE_TYPE_MINOR_EVENT, NULL, GC.getColorType("RED"));
		}
	}
}

bool CvPlayer::checkExpireEvent(EventTypes eEvent,
	EventTriggeredData const& kTriggeredData) const
{
	if (GC.getPythonCaller()->checkExpireEvent(eEvent, kTriggeredData))
		return true;

	CvEventInfo& kEvent = GC.getInfo(eEvent);

	if (!kEvent.isQuest())
		return (GC.getGame().getGameTurn() - kTriggeredData.m_iTurn > 2);

	CvEventTriggerInfo& kTrigger = GC.getInfo(kTriggeredData.m_eTrigger);
	FAssert(kTriggeredData.m_ePlayer != NO_PLAYER);
	CvPlayer& kPlayer = GET_PLAYER(kTriggeredData.m_ePlayer);

	if (kTrigger.isStateReligion() & kTrigger.isPickReligion() &&
		kPlayer.getStateReligion() != kTriggeredData.m_eReligion)
	{
		return true;
	}
	if (kTrigger.getCivic() != NO_CIVIC &&
		!kPlayer.isCivic((CivicTypes)kTrigger.getCivic()))
	{
		return true;
	}
	if (kTriggeredData.m_iCityId != -1 &&
			kPlayer.getCity(kTriggeredData.m_iCityId) == NULL)
		return true;

	if (kTriggeredData.m_iUnitId != -1 &&
		kPlayer.getUnit(kTriggeredData.m_iUnitId) == NULL)
	{
		return true;
	}
	if (kTriggeredData.m_eOtherPlayer != NO_PLAYER)
	{
		if (!GET_PLAYER(kTriggeredData.m_eOtherPlayer).isAlive())
			return true;

		if (kTriggeredData.m_iOtherPlayerCityId != -1 &&
			GET_PLAYER(kTriggeredData.m_eOtherPlayer).
			getCity(kTriggeredData.m_iOtherPlayerCityId) == NULL)
		{
			return true;
		}
	}

	if (kTrigger.getNumObsoleteTechs() > 0)
	{
		for (int i = 0; i < kTrigger.getNumObsoleteTechs(); i++)
		{
			if (GET_TEAM(getTeam()).isHasTech((TechTypes)kTrigger.getObsoleteTech(i)))
				return true;
		}
	}

	return false;
}


void CvPlayer::trigger(EventTriggerTypes eTrigger)
{
	initTriggeredData(eTrigger, true);
}

void CvPlayer::trigger(const EventTriggeredData& kData)
{
	if (isHuman())
	{
		CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_EVENT, kData.getID());
		addPopup(pInfo);
	}
	else
	{
		EventTypes eEvent = AI().AI_chooseEvent(kData.getID());
		if (eEvent != NO_EVENT)
			applyEvent(eEvent, kData.getID());
	}
}

bool CvPlayer::canTrigger(EventTriggerTypes eTrigger, PlayerTypes ePlayer, ReligionTypes eReligion) const
{
	if (!isAlive())
		return false;
	if (getID() == ePlayer)
		return false;

	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	CvEventTriggerInfo const& kTrigger = GC.getInfo(eTrigger);

	if (getTeam() == kPlayer.getTeam())
		return false;
	if (!kTrigger.isPickPlayer())
		return false;
	if (!GET_TEAM(getTeam()).isHasMet(kPlayer.getTeam()))
		return false;
	if (isHuman() && kTrigger.isOtherPlayerAI())
		return false;
	if (GET_TEAM(getTeam()).isAtWar(kPlayer.getTeam()) != kTrigger.isOtherPlayerWar())
		return false;
	if (kTrigger.getOtherPlayerHasTech() != NO_TECH &&
		!GET_TEAM(getTeam()).isHasTech((TechTypes)kTrigger.getOtherPlayerHasTech()))
	{
		return false;
	}
	if (kTrigger.getOtherPlayerShareBorders() > 0)
	{
		int iCount = 0;
		for (int i = 0; i < GC.getMap().numPlots(); i++)
		{
			CvPlot const& kLoopPlot = GC.getMap().getPlotByIndex(i);
			if (!kLoopPlot.isWater())
			{
				if (kLoopPlot.getOwner() == getID() && kLoopPlot.isAdjacentPlayer(ePlayer, true))
					iCount++;
			}
		}
		if (iCount < kTrigger.getOtherPlayerShareBorders())
			return false;
	}
	if (eReligion != NO_RELIGION)
	{
		bool const bHasReligion = (kTrigger.isStateReligion() ?
				(getStateReligion() == eReligion) : (getHasReligionCount(eReligion) > 0));
		if (kTrigger.isOtherPlayerHasReligion() && !bHasReligion)
			return false;
		if (kTrigger.isOtherPlayerHasOtherReligion())
		{
			if (bHasReligion)
				return false;
			if (kTrigger.isStateReligion() && getStateReligion() == NO_RELIGION)
				return false;
		}
	}

	return true;
}

CvCity* CvPlayer::pickTriggerCity(EventTriggerTypes eTrigger) const
{
	CvCity* pCity = NULL;
	std::vector<CvCity*> apCities;
	int iBestValue = MIN_INT;
	FOR_EACH_CITY_VAR(pLoopCity, *this)
	{
		int iValue = pLoopCity->getTriggerValue(eTrigger);
		if (iValue >= iBestValue && iValue != MIN_INT)
		{
			if (iValue > iBestValue)
			{
				apCities.clear();
				iBestValue = iValue;
			}
			apCities.push_back(pLoopCity);
		}
	}

	if (apCities.size() > 0)
	{
		int iChosen = GC.getGame().getSorenRandNum(apCities.size(), "Event pick city");
		pCity = apCities[iChosen];
	}

	return pCity;
}

CvUnit* CvPlayer::pickTriggerUnit(EventTriggerTypes eTrigger, CvPlot* pPlot, bool bPickPlot) const
{
	CvUnit* pUnit = NULL;
	std::vector<CvUnit*> apUnits;
	int iBestValue = MIN_INT;
	FOR_EACH_UNIT_VAR(pLoopUnit, *this)
	{
		int iValue = pLoopUnit->getTriggerValue(eTrigger, pPlot, bPickPlot);
		if (iValue >= iBestValue && iValue != MIN_INT)
		{
			if (iValue > iBestValue)
			{
				apUnits.clear();
				iBestValue = iValue;
			}
			apUnits.push_back(pLoopUnit);
		}
	}

	if (apUnits.size() > 0)
	{
		int iChosen = GC.getGame().getSorenRandNum(apUnits.size(), "Event pick unit");
		pUnit = apUnits[iChosen];
	}

	return pUnit;
}

int CvPlayer::getEventTriggerWeight(EventTriggerTypes eTrigger) const
{
	CvEventTriggerInfo const& kTrigger = GC.getInfo(eTrigger);

	if (kTrigger.getMinDifficulty() != NO_HANDICAP)
	{
		if (GC.getInfo(GC.getGame().getHandicapType()).
			getDifficulty() < 10 * // advc.250a
			kTrigger.getMinDifficulty())
		{
			return 0;
		}
	}

	if (kTrigger.isSinglePlayer() && GC.getGame().isGameMultiPlayer())
		return 0;

	if (!GC.getGame().isEventActive(eTrigger))
		return 0;

	if (kTrigger.getNumObsoleteTechs() > 0)
	{
		for (int i = 0; i < kTrigger.getNumObsoleteTechs(); i++)
		{
			if (GET_TEAM(getTeam()).isHasTech((TechTypes)(kTrigger.getObsoleteTech(i))))
				return 0;
		}
	}

	if (!kTrigger.isRecurring())
	{
		if (isTriggerFired(eTrigger))
			return 0;
	}

	if (kTrigger.getNumPrereqOrTechs() > 0)
	{
		bool bFoundValid = false;
		for (int i = 0; i < kTrigger.getNumPrereqOrTechs(); i++)
		{
			if (GET_TEAM(getTeam()).isHasTech((TechTypes)(kTrigger.getPrereqOrTechs(i))))
			{
				bFoundValid = true;
				break;
			}
		}
		if (!bFoundValid)
			return 0;
	}

	if (kTrigger.getNumPrereqAndTechs() > 0)
	{
		bool bFoundValid = true;
		for (int iI = 0; iI < kTrigger.getNumPrereqAndTechs(); iI++)
		{
			if (!GET_TEAM(getTeam()).isHasTech((TechTypes)(kTrigger.getPrereqAndTechs(iI))))
			{
				bFoundValid = false;
				break;
			}
		}

		if (!bFoundValid)
			return 0;
	}

	if (kTrigger.getNumPrereqEvents() > 0)
	{
		bool bFoundValid = true;
		for (int i = 0; i < kTrigger.getNumPrereqEvents(); i++)
		{
			if (getEventOccured((EventTypes)kTrigger.getPrereqEvent(i)) == NULL)
			{
				bFoundValid = false;
				break;
			}
		}
		if (!bFoundValid)
			return 0;
	}

	if (kTrigger.getCivic() != NO_CIVIC)
	{
		bool bFoundValid = false;
		FOR_EACH_ENUM(CivicOption)
		{
			if (getCivics(eLoopCivicOption) == kTrigger.getCivic())
			{
				bFoundValid = true;
				break;
			}
		}
		if (!bFoundValid)
			return 0;
	}

	if (kTrigger.getMinTreasury() > 0 && getGold() < kTrigger.getMinTreasury())
		return 0;

	if (GC.getMap().getNumLandAreas() < kTrigger.getMinMapLandmass())
		return 0;

	if (kTrigger.getMinOurLandmass() > 0 || kTrigger.getMaxOurLandmass() != -1)
	{
		int iNumLandmass = 0;
		FOR_EACH_AREA(pArea)
		{
			if (!pArea->isWater())
			{
				if (pArea->getCitiesPerPlayer(getID()) > 0)
					iNumLandmass++;
			}
		}

		if (iNumLandmass < kTrigger.getMinOurLandmass())
			return 0;

		if (kTrigger.getMaxOurLandmass() != -1 && iNumLandmass > kTrigger.getMaxOurLandmass())
			return 0;
	}

	if (kTrigger.getProbability() < 0)
		return kTrigger.getProbability();

	int iProbability = kTrigger.getProbability();

	if (kTrigger.isProbabilityUnitMultiply() && kTrigger.getNumUnits() > 0)
	{
		int iUnits = 0;
		FOR_EACH_UNIT(pLoopUnit, *this)
		{
			if (pLoopUnit->getTriggerValue(eTrigger, NULL, true) != MIN_INT)
				iUnits++;
		}
		iProbability *= iUnits;
	}

	if (kTrigger.isProbabilityBuildingMultiply() && kTrigger.getNumBuildings() > 0)
	{
		int iBuildings = 0;
		for (int i = 0; i < kTrigger.getNumBuildingsRequired(); ++i)
		{
			if (kTrigger.getBuildingRequired(i) != NO_BUILDINGCLASS)
			{
				iBuildings += getBuildingClassCount((BuildingClassTypes)
						kTrigger.getBuildingRequired(i));
			}
		}
		iProbability *= iBuildings;
	}

	return iProbability;
}


PlayerTypes CvPlayer::getSplitEmpirePlayer(CvArea const& kArea) const // advc: was iAreaId
{
	// can't create different derivative civs on the same continent
	for (PlayerIter<MAJOR_CIV> itOldDeriv; itOldDeriv.hasNext(); ++itOldDeriv)
	{
		if (itOldDeriv->getParent() != getID())
			continue;
		if (itOldDeriv->hasCapital() && itOldDeriv->getCapital()->isArea(kArea))
			return NO_PLAYER;
	}

	PlayerTypes eNewPlayer = NO_PLAYER;

	// Try to find a player who's never been in the game before
	for (PlayerIter<> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		if (!itPlayer->isEverAlive())
		{
			eNewPlayer = itPlayer->getID();
			break;
		}
	}

	if (eNewPlayer == NO_PLAYER)
	{
		/*	<kekm.24> Reusing a defeated player might not work correctly. advc:
			Allow human players to try it, but don't let the AI wreck the game somehow. */
		if (!isHuman())
			return NO_PLAYER; // </kekm.24>
		for (PlayerIter<EVER_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
		{
			if (!itPlayer->isAlive())
			{
				eNewPlayer = itPlayer->getID();
				break;
			}
		}
	}

	FAssert(eNewPlayer != getID()); // advc

	return eNewPlayer;
}


bool CvPlayer::canSplitEmpire() const
{
	if (GC.getGame().isOption(GAMEOPTION_NO_VASSAL_STATES))
		return false;

	if (isAVassal())
		return false;

	CivLeaderArray aLeaders;
	if (!getSplitEmpireLeaders(aLeaders))
		return false;

	FOR_EACH_AREA(pLoopArea)
	{
		if (canSplitArea(*pLoopArea))
			return true; // advc (simplified)
	}
	return false;
}


bool CvPlayer::canSplitArea(CvArea const& kArea) const // advc: was iAreaId
{
	PROFILE_FUNC(); // advc: Moved from CvPlayerAI::AI_doSplit

	if (!hasCapital())
		return false;

	if (getCapital()->isArea(kArea))
		return false;

	if (kArea.getCitiesPerPlayer(getID()) == 0)
		return false;

	PlayerTypes ePlayer = getSplitEmpirePlayer(kArea);
	if (ePlayer == NO_PLAYER)
		return false;

	if (!GET_PLAYER(ePlayer).isAlive())
	{
		if (kArea.getCitiesPerPlayer(getID()) <= 1)
			return false;
	}

	return true;
}


bool CvPlayer::getSplitEmpireLeaders(CivLeaderArray& kLeaders) const
{
	kLeaders.clear();

	FOR_EACH_ENUM(Civilization)
	{
		if (getCivilizationType() == eLoopCivilization)
			continue;
		if (!GC.getInfo(eLoopCivilization).isPlayable() ||
			!GC.getInfo(eLoopCivilization).isAIPlayable())
		{
			continue;
		}
		{
			bool bValid = true;
			for (PlayerIter<EVER_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
			{
				if (itPlayer->getCivilizationType() == eLoopCivilization)
				{
					bValid = false;
					break;
				}
			}
			if (!bValid)
				continue;
		}
		FOR_EACH_ENUM(LeaderHead)
		{
			if (!GC.getInfo(eLoopCivilization).isLeaders(eLoopLeaderHead) &&
				!GC.getGame().isOption(GAMEOPTION_LEAD_ANY_CIV))
			{
				continue;
			}
			bool bValid = true;
			for (PlayerIter<EVER_ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
			{
				//if (itPlayer->getPersonalityType() == eLoopLeaderHead)
				/*	advc.001: Relevant for RANDOM_PERSONALITIES option.
					Duplicate leader appearance seems much more problematic than
					duplicate (secret) personality. I'll handle personality in
					CvGame::addPlayer. */
				if (itPlayer->getLeaderType() == eLoopLeaderHead)
				{
					bValid = false;
					break;
				}
			}
			if (bValid)
				kLeaders.push_back(std::make_pair(eLoopCivilization, eLoopLeaderHead));
		}
	}

	return (kLeaders.size() > 0);
}

/*  advc: Don't want to use area ids in parameter lists in general.
	This wrapper is needed, however, for CvMessageData. */
bool CvPlayer::splitEmpire(int iArea)
{
	CvArea* pArea = GC.getMap().getArea(iArea);
	if (pArea == NULL)
	{
		FAssert(pArea != NULL);
		return false;
	}
	return splitEmpire(*pArea);
}


bool CvPlayer::splitEmpire(CvArea& kArea) // advc: was iAreaId
{
	if (!canSplitEmpire() || !canSplitArea(kArea))
		return false;

	PlayerTypes eNewPlayer = getSplitEmpirePlayer(kArea);
	if (eNewPlayer == NO_PLAYER)
		return false;

	bool bPlayerExists = GET_TEAM(eNewPlayer).isAlive();
	FAssert(!bPlayerExists);
	CvGame& kGame = GC.getGame();
	CvWString szMessage; // advc.127b
	if (!bPlayerExists)
	{
		LeaderHeadTypes eBestLeader = NO_LEADER;
		CivilizationTypes eBestCiv = NO_CIVILIZATION;

		CivLeaderArray aLeaders;
		if (getSplitEmpireLeaders(aLeaders))
		{
			int iBestValue = -1;
			CivLeaderArray::iterator it;
			for (it = aLeaders.begin(); it != aLeaders.end(); ++it)
			{
				int iValue = (1 + kGame.getSorenRandNum(100, "Choosing Split Personality"));
				if (GC.getInfo(getCivilizationType()).getDerivativeCiv() == it->first)
					iValue += 1000;

				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					eBestLeader = it->second;
					eBestCiv = it->first;
				}
			}
		}
		if (eBestLeader == NO_LEADER || eBestCiv == NO_CIVILIZATION)
			return false;

		szMessage = gDLL->getText("TXT_KEY_MISC_EMPIRE_SPLIT", getNameKey(),
				GC.getInfo(eBestCiv).getShortDescriptionKey(),
				GC.getInfo(eBestLeader).getTextKeyWide());
		// advc.127b: Announcement loop moved down

		kGame.addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szMessage,
				-1, -1, GC.getColorType("HIGHLIGHT_TEXT"));

		// remove leftover culture from old recycled player
		/*  BETTER_BTS_AI_MOD, Bugfix, 12/30/08, jdog5000: commented out
			(Clearing plot culture along with many other bits of data now handled by CvGame::addPlayer) */
		/* for (int iPlot = 0; iPlot < GC.getMap().numPlots(); ++iPlot) {
			CvPlot* pLoopPlot = GC.getMap().plotByIndex(iPlot);
			pLoopPlot->setCulture(eNewPlayer, 0, false, false);
		}*/

		kGame.addPlayer(eNewPlayer, eBestLeader, eBestCiv);
		GET_PLAYER(eNewPlayer).setParent(getID());
		GC.getInitCore().setLeaderName(eNewPlayer,
				GC.getInfo(eBestLeader).getTextKeyWide());

		CvTeam& kNewTeam = GET_TEAM(eNewPlayer);
		FOR_EACH_ENUM(Tech)
		{
			if (GET_TEAM(getTeam()).isHasTech(eLoopTech))
			{
				kNewTeam.setHasTech(eLoopTech, true, eNewPlayer, false, false);
				if (GET_TEAM(getTeam()).isNoTradeTech(eLoopTech) ||
					(kGame.isOption(GAMEOPTION_NO_TECH_BROKERING) &&
					isSignificantDiscovery(eLoopTech))) // advc.550e
				{
					kNewTeam.setNoTradeTech(eLoopTech, true);
				}
			}
		}
		// kekm.24: Don't inherit espionage points
		/*for (TeamIter<ALIVE> it; it.hasNext(); ++it) {
			kNewTeam.setEspionagePointsAgainstTeam(it->getID(), GET_TEAM(getTeam()).getEspionagePointsAgainstTeam(it->getID()));
			kLoopTeam.setEspionagePointsAgainstTeam(TEAMID(eNewPlayer), it->getEspionagePointsAgainstTeam(getTeam()));
		}*/
		kNewTeam.setEspionagePointsEver(GET_TEAM(getTeam()).getEspionagePointsEver());
		GET_TEAM(getTeam()).assignVassal(TEAMID(eNewPlayer), false);
		AI().AI_updateBonusValue();
	}

	std::vector<std::pair<int,int> > aCultures;
	CvMap const& kMap = GC.getMap();
	for (int iPlot = 0; iPlot < kMap.numPlots(); iPlot++)
	{
		CvPlot& kLoopPlot = kMap.getPlotByIndex(iPlot);
		bool bTranferPlot = false;
		if (kLoopPlot.isArea(kArea))
			bTranferPlot = true;

		if (!bTranferPlot)
		{
			CvCity* pWorkingCity = kLoopPlot.getWorkingCity();
			if (pWorkingCity != NULL && pWorkingCity->getOwner() == getID() &&
				pWorkingCity->isArea(kArea))
			{
				bTranferPlot = true;
			}
		}

		if (!bTranferPlot && kLoopPlot.isWater() && kLoopPlot.isAdjacentToArea(kArea))
			bTranferPlot = true;

		if (bTranferPlot)
		{
			int iCulture = kLoopPlot.getCulture(getID());
			if (bPlayerExists)
				iCulture = std::max(iCulture, kLoopPlot.getCulture(eNewPlayer));

			aCultures.push_back(std::make_pair(iPlot, iCulture));
		}

		if (kLoopPlot.isRevealed(getTeam()))
			kLoopPlot.setRevealed(TEAMID(eNewPlayer), true, false, getTeam(), false);
	}
	std::vector<CvCity*> apAcquiredCities; // advc.104r
	FOR_EACH_CITY_VAR(pOldCity, *this)
	{
		if (!pOldCity->isArea(kArea))
			continue;

		int iCulture = pOldCity->getCultureTimes100(getID());
		CvPlot* pPlot = pOldCity->plot();
		GET_PLAYER(eNewPlayer).acquireCity(pOldCity, false, true, false);
		/*	advc: acquireCity kills pOldCity. Note that
			it's OK to do this while traversing the city list (m_cities). */
		pOldCity = NULL;
		CvCity* pNewCity = pPlot->getPlotCity();
		pNewCity->setCultureTimes100(eNewPlayer, iCulture, false, false);
		/*  <advc.104r> Want to initialize UWAI data after assigning
			cities but before creating units. Therefore move
			unit placement into a separate loop. */
		apAcquiredCities.push_back(pNewCity);	
		/*for (int i = 0; i < GC.getDefineINT("COLONY_NUM_FREE_DEFENDERS"); ++i)
			pCity->initConscriptedUnit();*/ // </advc.104r>
	}
	for (uint i = 0; i < aCultures.size(); i++)
	{
		CvPlot* pPlot = GC.getMap().plotByIndex(aCultures[i].first);
		pPlot->setCulture(eNewPlayer, aCultures[i].second, true, false);
		pPlot->setCulture(getID(), 0, true, false);
		for (TeamIter<ALIVE> itTeam; itTeam.hasNext(); ++itTeam) // advc: ALIVE
		{
			if (pPlot->getRevealedOwner(itTeam->getID()) == getID())
				pPlot->setRevealedOwner(itTeam->getID(), eNewPlayer);
		}
	}
	// advc.130r:
	GET_PLAYER(eNewPlayer).AI_changeMemoryCount(getID(), MEMORY_INDEPENDENCE,
			// advc.130j: Twice remembered
			2 * GC.getInfo(getPersonalityType()).getFreedomAppreciation());

	kGame.updatePlotGroups();
	// K-Mod
	GET_PLAYER(eNewPlayer).AI_updateAttitude(getID());
	AI().AI_updateAttitude(eNewPlayer);
	// K-Mod end
	// <advc.104r>
	for(size_t i = 0; i < apAcquiredCities.size(); i++)
	{
		for(int j = 0; j < GC.getDefineINT("COLONY_NUM_FREE_DEFENDERS"); j++)
			apAcquiredCities[i]->initConscriptedUnit();
	}
	if (getUWAI.isEnabled())
		getUWAI.processNewCivInGame(eNewPlayer); // </advc.104r>
	/*  <advc.127b> Moved here from above b/c I want the announcement
		to point to the new capital */
	if (!bPlayerExists)
	{
		FAssert(!szMessage.empty());
		CvCity const* pNewCapital = GET_PLAYER(eNewPlayer).getCapital();
		for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
		{
			CvPlayer const& kObs = *it;
			if (kObs.getID() == getID() || kObs.getID() == eNewPlayer ||
				GET_TEAM(getTeam()).isHasMet(kObs.getTeam()) ||
				kObs.isSpectator()) // advc.127
			{
				bool bRev = (pNewCapital != NULL && pNewCapital->isRevealed(
						kObs.getTeam(), true));
				LPCSTR szButton = (bRev ? ARTFILEMGR.getInterfaceArtInfo(
						"INTERFACE_CITY_BAR_CAPITAL_TEXTURE")->getPath() : NULL);
				gDLL->UI().addMessage(kObs.getID(), false, -1, szMessage,
						"AS2D_REVOLTEND", MESSAGE_TYPE_MAJOR_EVENT,
						szButton, NO_COLOR, bRev ? pNewCapital->getX() : -1,
						bRev ? pNewCapital->getY() : -1);
			}
		}
	} // </advc.127b>
	return true;
}


bool CvPlayer::isValidTriggerReligion(const CvEventTriggerInfo& kTrigger,
	CvCity const* pCity, ReligionTypes eReligion) const
{
	if (kTrigger.getNumReligionsRequired() > 0)
	{
		bool bFound = false;
		for (int i = 0; i < kTrigger.getNumReligionsRequired(); i++)
		{
			if (eReligion == kTrigger.getReligionRequired(i))
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
			return false;
	}

	if (pCity != NULL)
	{
		if (!pCity->isHasReligion(eReligion))
			return false;

		if (kTrigger.isHolyCity() && !pCity->isHolyCity(eReligion))
			return false;
	}
	else
	{
		if (getHasReligionCount(eReligion) == 0)
			return false;

		if (kTrigger.isHolyCity())
		{
			CvCity* pHolyCity = GC.getGame().getHolyCity(eReligion);
			if (pHolyCity == NULL || pHolyCity->getOwner() != getID())
				return false;
		}
	}

	return true;
}


bool CvPlayer::isValidTriggerCorporation(CvEventTriggerInfo const& kTrigger,
	CvCity const* pCity, CorporationTypes eCorporation) const
{
	if (kTrigger.getNumCorporationsRequired() > 0)
	{
		bool bFound = false;
		for (int i = 0; i < kTrigger.getNumCorporationsRequired(); ++i)
		{
			if (eCorporation == kTrigger.getCorporationRequired(i))
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
			return false;
	}

	if (pCity != NULL)
	{
		if (!pCity->isHasCorporation(eCorporation))
			return false;

		if (kTrigger.isHeadquarters() && !pCity->isHeadquarters(eCorporation))
			return false;
	}
	else
	{
		/* if (getHasCorporationCount(eCorporation) > 0)
			return true;*/
		// K-Mod. (bugfix)
		if (getHasCorporationCount(eCorporation) <= 0)
			return false; // K-Mod end

		if (kTrigger.isHeadquarters())
		{
			CvCity* pHeadquarters = GC.getGame().getHeadquarters(eCorporation);
			if (pHeadquarters == NULL || pHeadquarters->getOwner() != getID())
				return false;
		}
	}

	//return false;
	return true; // K-Mod. (bugfix)
}


void CvPlayer::launch(VictoryTypes eVictory)
{
	CvTeam& kTeam = GET_TEAM(getTeam());
	if (!kTeam.canLaunch(eVictory))
		return;

	kTeam.finalizeProjectArtTypes();
	kTeam.setVictoryCountdown(eVictory, kTeam.getVictoryDelay(eVictory));

	//gDLL->getEngineIFace()->AddLaunch(getID());
	// K-Mod. The spaceship launch causes pitboss to crash
	if (GC.IsGraphicsInitialized())
		gDLL->getEngineIFace()->AddLaunch(getID());
	// K-Mod end.

	kTeam.setCanLaunch(eVictory, false);

	CvCity const* pCapital = getCapital();
	// <advc.106> Cut from the loop below. Use this for the replay message as well.
	CvWString szMsg(gDLL->getText("TXT_KEY_VICTORY_TEAM_HAS_LAUNCHED",
			kTeam.getName().GetCString()));
	int iPlotX = -1;
	int iPlotY = -1; // </advc.106>
	for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
	{
		CvPlayer const& kObs = *it;
		if (pCapital != NULL && pCapital->isRevealed(kObs.getTeam()))
		{
			iPlotX = pCapital->getX();
			iPlotY = pCapital->getY();
		}
		CvWString szBuffer(szMsg); // advc.106
		if (kObs.getTeam() == getTeam())
			szBuffer = gDLL->getText("TXT_KEY_VICTORY_YOU_HAVE_LAUNCHED");
		gDLL->UI().addMessage(kObs.getID(), true, -1, szBuffer, "AS2D_CULTURELEVEL",
				MESSAGE_TYPE_MAJOR_EVENT, ARTFILEMGR.getMiscArtPath("SPACE_SHIP_BUTTON"),
				GC.getColorType("HIGHLIGHT_TEXT"), iPlotX, iPlotY, true, true);
	}
	// <advc.106>
	GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT,
			getID(), szMsg, iPlotX, iPlotY, GC.getColorType("HIGHLIGHT_TEXT"));
	// </advc.106>
}


bool CvPlayer::isFreePromotion(UnitCombatTypes eUnitCombat, PromotionTypes ePromotion) const
{
	for (size_t i = 0; i < m_aFreeUnitCombatPromotions.size(); i++)
	{
		if (m_aFreeUnitCombatPromotions[i].first == eUnitCombat &&
			m_aFreeUnitCombatPromotions[i].second == ePromotion)
		{
			return true;
		}
	}
	return false;
}


void CvPlayer::setFreePromotion(UnitCombatTypes eUnitCombat, PromotionTypes ePromotion, bool bFree)
{
	for (UnitCombatPromotionArray::iterator it = m_aFreeUnitCombatPromotions.begin();
		it != m_aFreeUnitCombatPromotions.end(); ++it)
	{
		if (it->first == eUnitCombat && it->second == ePromotion)
		{
			if (!bFree)
				m_aFreeUnitCombatPromotions.erase(it);
			return;
		}
	}
	if (bFree)
		m_aFreeUnitCombatPromotions.push_back(std::make_pair(eUnitCombat, ePromotion));
}


bool CvPlayer::isFreePromotion(UnitClassTypes eUnitClass,
	PromotionTypes ePromotion) const
{
	for (size_t i = 0; i < m_aFreeUnitClassPromotions.size(); i++)
	{
		if (m_aFreeUnitClassPromotions[i].first == eUnitClass &&
			m_aFreeUnitClassPromotions[i].second == ePromotion)
		{
			return true;
		}
	}
	return false;
}


void CvPlayer::setFreePromotion(UnitClassTypes eUnitClass,
	PromotionTypes ePromotion, bool bFree)
{
	for (UnitClassPromotionArray::iterator it = m_aFreeUnitClassPromotions.begin();
		it != m_aFreeUnitClassPromotions.end(); ++it)
	{
		if (it->first == eUnitClass && it->second == ePromotion)
		{
			if (!bFree)
				m_aFreeUnitClassPromotions.erase(it);
			return;
		}
	}
	if (bFree)
		m_aFreeUnitClassPromotions.push_back(std::make_pair(eUnitClass, ePromotion));
}


PlayerVoteTypes CvPlayer::getVote(int iId) const
{
	for (size_t i = 0; i < m_aVote.size(); i++)
	{
		if (m_aVote[i].first == iId)
			return m_aVote[i].second;
	}
	return NO_PLAYER_VOTE;
}


void CvPlayer::setVote(int iId, PlayerVoteTypes ePlayerVote)
{
	for (std::vector<std::pair<int,PlayerVoteTypes> >::
		iterator it = m_aVote.begin(); it != m_aVote.end(); ++it)
	{
		if (it->first == iId)
		{
			if (ePlayerVote == NO_PLAYER_VOTE)
				m_aVote.erase(it);
			else it->second = ePlayerVote;
			return;
		}
	}
	if (ePlayerVote != NO_PLAYER_VOTE)
		m_aVote.push_back(std::make_pair(iId, ePlayerVote));
}


int CvPlayer::getUnitExtraCost(UnitClassTypes eUnitClass) const
{
	for (size_t i = 0; i < m_aUnitExtraCosts.size(); i++)
	{
		if (m_aUnitExtraCosts[i].first == eUnitClass)
			return m_aUnitExtraCosts[i].second;
	}
	return 0;
}


void CvPlayer::setUnitExtraCost(UnitClassTypes eUnitClass, int iCost)
{
	for (std::vector<std::pair<UnitClassTypes,int> >::
		iterator it = m_aUnitExtraCosts.begin(); it != m_aUnitExtraCosts.end(); ++it)
	{
		if (it->first == eUnitClass)
		{
			if (iCost == 0)
				m_aUnitExtraCosts.erase(it);
			else it->second = iCost;
			return;
		}
	}
	if (iCost != 0)
		m_aUnitExtraCosts.push_back(std::make_pair(eUnitClass, iCost));
}


bool CvPlayer::hasShrine(ReligionTypes eReligion)
{
	bool bHasShrine = false;
	if (eReligion != NO_RELIGION)
	{
		CvCity* pHolyCity = GC.getGame().getHolyCity(eReligion);
		if (pHolyCity != NULL && pHolyCity->getOwner() == getID())
			bHasShrine = pHolyCity->hasShrine(eReligion);
	}
	return bHasShrine;
}


void CvPlayer::invalidatePopulationRankCache()
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
		pLoopCity->invalidatePopulationRankCache();
}


void CvPlayer::invalidateYieldRankCache(YieldTypes eYield)
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
	{
		pLoopCity->invalidateYieldRankCache(
				eYield); // advc.001: was NO_YIELD
	}
}


void CvPlayer::invalidateCommerceRankCache(CommerceTypes eCommerce)
{
	FOR_EACH_CITY_VAR(pLoopCity, *this)
	{
		pLoopCity->invalidateCommerceRankCache(
				eCommerce); // advc.001: was NO_COMMERCE
	}
}


/*void CvPlayer::doUpdateCacheOnTurn() // advc: Remove the whole function
{	// (advc: This function and comment were added by the BtS expansion)
	// add this back, after testing without it
	// invalidateYieldRankCache();
}*/

// advc: Renamed from "processVoteSourceBonus"
void CvPlayer::processVoteSource(VoteSourceTypes eVoteSource, bool bActive)
{
	FOR_EACH_CITY_VAR(pCity, *this)
		pCity->processVoteSource(eVoteSource, bActive);
}


int CvPlayer::getVotes(VoteTypes eVote, VoteSourceTypes eVoteSource) const
{
	int iVotes = 0;

	ReligionTypes const eReligion = GC.getGame().getVoteSourceReligion(eVoteSource);
	if (eVote == NO_VOTE)
	{
		if (eReligion != NO_RELIGION)
			iVotes = getReligionPopulation(eReligion);
		else iVotes = getTotalPopulation();
	}
	else
	{
		if (!GC.getInfo(eVote).isVoteSourceType(eVoteSource))
			return 0;

		if (GC.getInfo(eVote).isCivVoting())
		{
			if (eReligion == NO_RELIGION || getHasReligionCount(eReligion) > 0)
				iVotes = 1;
		}
		else if (GC.getInfo(eVote).isCityVoting())
		{
			if (eReligion != NO_RELIGION)
				iVotes = getHasReligionCount(eReligion);
			else iVotes = getNumCities();
		}
		else
		{
			if (eReligion == NO_RELIGION)
				iVotes = getTotalPopulation();
			else iVotes = getReligionPopulation(eReligion);
		}

		if (eReligion != NO_RELIGION && getStateReligion() == eReligion)
		{
			iVotes *= (100 + GC.getInfo(eVote).getStateReligionVotePercent());
			iVotes /= 100;
		}
	}

	return iVotes;
}

bool CvPlayer::canDoResolution(VoteSourceTypes eVoteSource,
	VoteSelectionSubData const& kData) const
{
	CvTeam const& kOurTeam = GET_TEAM(getTeam());
	if (kData.ePlayer != NO_PLAYER && !kOurTeam.isHasMet(TEAMID(kData.ePlayer)))
		return false;

	TeamTypes const eSecretaryGeneral = GC.getGame().getSecretaryGeneral(eVoteSource);
	CvVoteInfo const& kVote = GC.getInfo(kData.eVote);
	if (kVote.isOpenBorders())
	{
		for (TeamIter<MAJOR_CIV> it; it.hasNext(); ++it)
		{
			CvTeam const& kVotingMember = *it;
			if (!kVotingMember.isVotingMember(eVoteSource))
				continue;
			if (!kOurTeam.isOpenBordersTrading() && !kVotingMember.isOpenBordersTrading())
				return false;
			if (kOurTeam.isAtWar(kVotingMember.getID()))
				return false;
		}
	}
	else if (kVote.isDefensivePact())
	{
		for (TeamIter<MAJOR_CIV> it; it.hasNext(); ++it)
		{
			CvTeam const& kVotingMember = *it;
			if (!kVotingMember.isVotingMember(eVoteSource))
				continue;
			if (!kOurTeam.isDefensivePactTrading() && !kVotingMember.isDefensivePactTrading())
				return false;
			if ((kOurTeam.getNumWars() > 0 || kVotingMember.getNumWars() > 0) &&
				// kekm.25: 'Sometimes defensive pact can be signed while at war'
				GC.getDefineINT(CvGlobals::BBAI_DEFENSIVE_PACT_BEHAVIOR) == 0)
			{
				return false;
			} // <kekm.25>
			if(kOurTeam.isAtWar(kVotingMember.getID()) ||
				// advc: Same additional restriction as for DP between AI teams (kekm.3)
				!kOurTeam.allWarsShared(kVotingMember.getID()))
			{
				return false;
			} // </kekm.25>
			if (!kOurTeam.canSignDefensivePact(kVotingMember.getID()))
				return false;
		}
	}
	else if (kVote.isForcePeace())
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);
		if (kPlayer.getTeam() != getTeam() &&
			kOurTeam.isAtWar(kPlayer.getTeam()))
		{
			CvTeam const& kOurMaster = GET_TEAM(getMasterTeam()); // advc.opt
			if (kOurMaster.isVotingMember(eVoteSource) &&
				!kOurMaster.canContact(kPlayer.getTeam()))
			{
				return false;
			}
		}
	}
	else if (kVote.isForceWar())
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);
		if (!kOurTeam.isAtWar(kPlayer.getTeam()) &&
			kOurTeam.isFullMember(eVoteSource)) // kekm.25/advc
		{
			CvTeam const& kOurMaster = GET_TEAM(getMasterTeam());
			if ((kOurMaster.isFullMember(eVoteSource) && // kekm.25/advc: was isVotingMember
				!kOurMaster.canDeclareWar(kPlayer.getTeam()) &&
				// <kekm.25/advc>
				eSecretaryGeneral == kOurMaster.getID()) ||
				!kOurMaster.canEventuallyDeclareWar(kPlayer.getTeam()))
				// </advc.25/advc>
			{
				return false;
			}
		}
	}
	else if (kVote.isForceNoTrade())
	{
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);
		/*  <advc.130f> Don't allow players to propose resolutions that would cancel
			deals with turnsToCancel > 0 */
		if(eSecretaryGeneral == getTeam() &&
			isAnyDealTooRecentToCancel(kPlayer.getTeam()))
		{
			return false;
		} // </advc.130f>
		if (!canStopTradingWithTeam(kPlayer.getTeam(), true))
			return false;
	}
	else if (kVote.isAssignCity())
	{
		if (GET_TEAM(kData.eOtherPlayer).isVassal(TEAMID(kData.ePlayer)))
			return false;
	} // <advc.178>
	else if(kVote.isVictory() && !GC.getGame().isDiploVictoryValid())
		return false; // </advc.178>

	return true;
}

bool CvPlayer::canDefyResolution(VoteSourceTypes eVoteSource,
	VoteSelectionSubData const& kData) const
{
	if (GC.getGame().getSecretaryGeneral(eVoteSource) == getTeam())
		return false;

	CvVoteInfo const& kVote = GC.getInfo(kData.eVote); // advc
	// <kekm.25/advc> Kek-Mod just checks isAVassal
	if(GET_TEAM(getTeam()).isCapitulated() ||
		(isAVassal() && (kVote.isForceWar() || kVote.isForcePeace())))
	{
		return false;
	} // </kekm.25/advc>
	if (kVote.isOpenBorders())
	{
		for (TeamIter<MAJOR_CIV,NOT_SAME_TEAM_AS> it(getTeam()); it.hasNext(); ++it)
		{
			CvTeam& kVotingMember = *it;
			if (!kVotingMember.isVotingMember(eVoteSource))
				continue;
			if (!kVotingMember.isOpenBorders(getTeam()))
				return true;
		}
	}
	else if (kVote.isDefensivePact())
	{
		for (TeamIter<MAJOR_CIV,NOT_SAME_TEAM_AS> it(getTeam()); it.hasNext(); ++it)
		{
			CvTeam& kVotingMember = *it;
			if (!kVotingMember.isVotingMember(eVoteSource))
				continue;
			if (!kVotingMember.isDefensivePact(getTeam()))
				return true;
		}
	}
	else if (kVote.isForceNoTrade())
	{
		return true;
	}
	else if (kVote.isForceWar())
	{
		if (!::atWar(getTeam(), TEAMID(kData.ePlayer)) &&
			// kekm.25: 'Cannot defy war declaration against itself'
			GET_TEAM(kData.ePlayer).getMasterTeam() != getMasterTeam() &&
			isFullMember(eVoteSource)) // advc
		{
			// BETTER_BTS_AI_MOD, 12/31/08, jdog5000: Vassals can't defy declarations of war
			//if (!GET_TEAM(getTeam()).isAVassal()) // kekm.25: Vassals already handled
			return true;
		}
	}
	else if (kVote.isForcePeace())
	{
		if (TEAMID(kData.ePlayer) == getTeam())
			return true;

		if (::atWar(getTeam(), TEAMID(kData.ePlayer)))
			return true;
	}
	else if (kVote.isAssignCity())
	{
		if (kData.ePlayer == getID() ||
			// kekm.25: 'You can defy resolution giving you a city'
			kData.eOtherPlayer == getID())
		{
			return true;
		}
	}
	else if (!GC.getGame().isTeamVote(kData.eVote))
		return true;

	return false;
}


void CvPlayer::setDefiedResolution(VoteSourceTypes eVoteSource,
	VoteSelectionSubData const& kData)
{
	FAssertMsg(canDefyResolution(eVoteSource, kData),
			"OK to fail when a team member defies a resolution"); // kekm.25
	// cities get unhappiness
	FOR_EACH_CITY_VAR(pLoopCity, *this)
	{
		ReligionTypes eReligion = GC.getGame().getVoteSourceReligion(eVoteSource);
		if (eReligion == NO_RELIGION || pLoopCity->isHasReligion(eReligion))
		{
			int iAngerLength = pLoopCity->flatDefyResolutionAngerLength();
			if (eReligion != NO_RELIGION && pLoopCity->isHasReligion(eReligion))
				iAngerLength /= std::max(1, pLoopCity->getReligionCount());
			pLoopCity->changeDefyResolutionAngerTimer(iAngerLength);
		}
	}
	setLoyalMember(eVoteSource, false);
}


void CvPlayer::setEndorsedResolution(VoteSourceTypes eVoteSource,
	VoteSelectionSubData const& kData)
{
	setLoyalMember(eVoteSource, true);
}


bool CvPlayer::isFullMember(VoteSourceTypes eVoteSource) const
{
	if (GC.getGame().getVoteSourceReligion(eVoteSource) != NO_RELIGION)
	{
		if (getStateReligion() != GC.getGame().getVoteSourceReligion(eVoteSource))
			return false;
	}

	if (GC.getInfo(eVoteSource).getCivic() != NO_CIVIC)
	{
		if (!isCivic((CivicTypes)GC.getInfo(eVoteSource).getCivic()))
			return false;
	}

	if (!isLoyalMember(eVoteSource))
		return false;

	return isVotingMember(eVoteSource);
}


bool CvPlayer::isVotingMember(VoteSourceTypes eVoteSource) const
{
	return (getVotes(NO_VOTE, eVoteSource) > 0);
}


PlayerTypes CvPlayer::pickConqueredCityOwner(const CvCity& kCity) const
{
	PlayerTypes eBestPlayer = kCity.getLiberationPlayer(true);
	if (eBestPlayer!= NO_PLAYER)
	{
		if (GET_TEAM(getTeam()).isVassal(TEAMID(eBestPlayer)))
			return eBestPlayer;
	}
	return getID();
}

// advc (note): In part duplicated in CvPlayerAI::AI_civicValue
bool CvPlayer::canHaveTradeRoutesWith(PlayerTypes ePlayer) const
{
	CvPlayer& kOtherPlayer = GET_PLAYER(ePlayer);
	if (!kOtherPlayer.isAlive())
		return false;
	// <advc.124>
	if(getID() != ePlayer && kOtherPlayer.isAnarchy())
		return false; // </advc.124>
	if (getTeam() == kOtherPlayer.getTeam())
		return true;

	if (!GET_TEAM(getTeam()).isFreeTrade(kOtherPlayer.getTeam()))
		return false;

	if (GET_TEAM(getTeam()).isVassal(kOtherPlayer.getTeam()))
		return true;

	if (GET_TEAM(kOtherPlayer.getTeam()).isVassal(getTeam()))
		return true;

	if (!isNoForeignTrade() && !kOtherPlayer.isNoForeignTrade())
		return true;

	return false;
}

bool CvPlayer::canStealTech(PlayerTypes eTarget, TechTypes eTech) const
{
	if (GET_TEAM(eTarget).isHasTech(eTech))
	{
		if (canResearch(eTech, /* K-Mod: */ false, true))
			return true;
	}
	return false;
}


bool CvPlayer::canForceCivics(PlayerTypes eTarget, CivicTypes eCivic) const
{
	// return (GET_PLAYER(eTarget).canDoCivics(eCivic) && !GET_PLAYER(eTarget).isCivic(eCivic) && isCivic(eCivic));
	// <advc.132>
	if (!GET_PLAYER(eTarget).canDoCivics(eCivic) || GET_PLAYER(eTarget).isCivic(eCivic))
		return false;
	/*	(AdvCiv code for identifying economy and religion civics deleted on 4 Auf 2020
		and replaced with new XML tag) */
	return (isCivic(eCivic) || GC.getInfo(eCivic).canAlwaysForce()); // </advc.132>
}


bool CvPlayer::canForceReligion(PlayerTypes eTarget, ReligionTypes eReligion) const
{
	//return (GET_PLAYER(eTarget).canDoReligion(eReligion) && GET_PLAYER(eTarget).getStateReligion() != eReligion && getStateReligion() == eReligion);
	// K-Mod: You shouldn't be able to force a religion on an irreligious civ.
	//return (GET_PLAYER(eTarget).isStateReligion() && GET_PLAYER(eTarget).canDoReligion(eReligion) && GET_PLAYER(eTarget).getStateReligion() != eReligion && getStateReligion() == eReligion);
	// <advc.132>
	CvPlayer const& kTarget = GET_PLAYER(eTarget);
	/*  Just the conditions from above. (Better use canConvert, which checks recent
		religion change? Would also have to use canRevolution then in canForceCivics.) */
	if (!kTarget.isStateReligion() || !kTarget.canDoReligion(eReligion) ||
		kTarget.getStateReligion() == eReligion)
	{
		return false;
	}
	// New: Accept any major religion
	return (getStateReligion() == eReligion || kTarget.isMajorReligion(eReligion));
	// </advc.132>
}

bool CvPlayer::canSpyDestroyUnit(PlayerTypes eTarget, CvUnit const& kUnit) const
{
	if (kUnit.getTeam() == getTeam())
		return false;

	if (kUnit.getUnitInfo().getProductionCost() <= 0)
		return false;

	if (!kUnit.getPlot().isVisible(getTeam()))
		return false;

	return true;
}


bool CvPlayer::canSpyBribeUnit(PlayerTypes eTarget, CvUnit const& kUnit) const
{
	if (!canSpyDestroyUnit(eTarget, kUnit))
		return false;

	// Can't buy units when at war
	if (kUnit.isEnemy(getTeam()))
		return false;

	// Can't buy units if they are not in a legal plot
	if (!GET_TEAM(getTeam()).canPeacefullyEnter(TEAMID(eTarget)))
		return false;

	FOR_EACH_UNIT_IN(pLoopUnit, kUnit.getPlot())
	{
		if (pLoopUnit == NULL)
			continue;
		if (pLoopUnit != &kUnit && pLoopUnit->isEnemy(getTeam()))
		{
			// If we buy the unit, we will be on the same plot as an enemy unit! Not good.
			return false;
		}
	}

	return true;
}


bool CvPlayer::canSpyDestroyBuilding(PlayerTypes eTarget, BuildingTypes eBuilding) const
{
	if (GC.getInfo(eBuilding).getProductionCost() <= 0)
		return false;

	if (GC.getInfo(eBuilding).isLimited())
		return false;

	return true;
}


bool CvPlayer::canSpyDestroyProject(PlayerTypes eTarget, ProjectTypes eProject) const
{
	CvProjectInfo& kProject = GC.getInfo(eProject);
	if (kProject.getProductionCost() <= 0)
		return false;

	if (GET_TEAM(eTarget).getProjectCount(eProject) <= 0)
		return false;

	if (kProject.isWorldProject())
		return false;

	if (!kProject.isSpaceship())
		return false;
	else
	{
		VictoryTypes eVictory = (VictoryTypes)kProject.getVictoryPrereq();
		if (eVictory != NO_VICTORY)
		{
			// Can't destroy spaceship components if we have already launched
			if (GET_TEAM(eTarget).getVictoryCountdown(eVictory) >= 0)
				return false;
		}
	}
	return true;
}

// K-Mod
int CvPlayer::getEspionageGoldQuantity(EspionageMissionTypes eMission,
	PlayerTypes eTargetPlayer, CvCity const* pCity) const
{
	if (pCity == NULL || pCity->getOwner() != eTargetPlayer)
		return 0;

	CvPlayer const& kTargetPlayer = GET_PLAYER(eTargetPlayer);
	CvEspionageMissionInfo const& kMission = GC.getInfo(eMission);
	int iGoldStolen = (kTargetPlayer.getGold() *
			kMission.getStealTreasuryTypes()) / 100;
	iGoldStolen *= 3 * pCity->getPopulation();
	iGoldStolen /= std::max(1, pCity->getPopulation() +
			2 * kTargetPlayer.getAveragePopulation());
	return std::min(iGoldStolen, kTargetPlayer.getGold());
}


void CvPlayer::forcePeace(PlayerTypes ePlayer)
{
	/*if (!GET_TEAM(getTeam()).isAVassal()) {
		FAssert(GET_TEAM(getTeam()).canChangeWarPeace(GET_PLAYER(ePlayer).getTeam()));*/ // BtS
	// K-Mod: "canChangeWarPeace" can return false here if the peace team vassalates after the vote is cast.
	//if (GET_TEAM(getTeam()).canChangeWarPeace(GET_PLAYER(ePlayer).getTeam()))
	// ...
	/*	advc: Redundant code deleted; CvTeam::signPeaceTreaty does the same thing
		including, due to advc.130v, the canChangeWarPeace check.
		To avoid making forcePeace obsolete, I'm changing its semantics so that
		canChangeWarPeace is NOT checked. */
	GET_TEAM(getTeam()).signPeaceTreaty(TEAMID(ePlayer), true);
}

// advc.032:
bool CvPlayer::resetDualDeal(PlayerTypes ePlayer, TradeableItems eDealType)
{
	int iGameTurn = GC.getGame().getGameTurn();
	FOR_EACH_DEAL_VAR(d)
	{
		if (d->isBetween(getID(), ePlayer))
		{
			FOR_EACH_TRADE_ITEM(d->getFirstList())
			{
				if(pItem->m_eItemType == eDealType)
				{
					d->setInitialGameTurn(iGameTurn);
					return true; // Assume that there can be at most one deal of eDealType
				}
			}
		}
	}
	return false;
}


bool CvPlayer::canSpiesEnterBorders(PlayerTypes ePlayer) const
{
	FOR_EACH_ENUM(EspionageMission)
	{
		if (GC.getInfo(eLoopEspionageMission).isNoActiveMissions() &&
			GC.getInfo(eLoopEspionageMission).isPassive())
		{
			if (GET_PLAYER(ePlayer).canDoEspionageMission(eLoopEspionageMission, getID()))
				return false;
		}
	}
	return true;
}


int CvPlayer::getReligionPopulation(ReligionTypes eReligion) const
{
	int iPopulation = 0;
	FOR_EACH_CITY(pCity, *this)
	{
		if (pCity->isHasReligion(eReligion))
			iPopulation += pCity->getPopulation();
	}
	return iPopulation;
}


int CvPlayer::getNewCityProductionValue() const
{
	int iValue = 0;
	CvCivilization const& kCiv = getCivilization(); // advc.003w
	for (int i = 0; i < kCiv.getNumBuildings(); i++)
	{
		BuildingTypes eBuilding = kCiv.buildingAt(i);
		if(GC.getGame().isFreeStartEraBuilding(eBuilding)) // advc
		{
			iValue += (100 * getProductionNeeded(eBuilding)) /
					std::max(1, 100 + getProductionModifier(eBuilding));
		}
	}

	iValue *= 100 + GC.getDefineINT("NEW_CITY_BUILDING_VALUE_MODIFIER");
	iValue /= 100;

	iValue += (GC.getDefineINT("ADVANCED_START_CITY_COST") *
			GC.getInfo(GC.getGame().getGameSpeedType()).getGrowthPercent()) / 100;

	int iPopulation = GC.getDefineINT("INITIAL_CITY_POPULATION") +
			GC.getInfo(GC.getGame().getStartEra()).getFreePopulation();
	for (int i = 1; i <= iPopulation; ++i)
	{
		iValue += (getGrowthThreshold(i) *
				GC.getDefineINT("ADVANCED_START_POPULATION_COST")) / 100;
	}
	// <advc.251>
	iValue = (iValue *
			// Apply production modifier half (b/c getGrowthThreshold is also dependent on handicap)
			(1 + (trainingModifierFromHandicap(false) - 1) / 2)).roundToMultiple(isHuman() ? 5 : 1);
	// </advc.251>
	return iValue;
}


int CvPlayer::getGrowthThreshold(int iPopulation) const
{
	CvGame const& kGame = GC.getGame(); // advc
	// <advc.251>
	static int const iBASE_CITY_GROWTH_THRESHOLD = GC.getDefineINT("BASE_CITY_GROWTH_THRESHOLD");
	static int const iCITY_GROWTH_MULTIPLIER = GC.getDefineINT("CITY_GROWTH_MULTIPLIER");
	int iBaseThreshold = ::round(0.01 * iBASE_CITY_GROWTH_THRESHOLD * GC.getInfo(
			getHandicapType()).getBaseGrowthThresholdPercent());
	int iThreshold = iBaseThreshold + // </advc.251>
			(iPopulation * iCITY_GROWTH_MULTIPLIER);
	// <advc.251>
	int iAIModifier = 100;
	if(!isHuman()) // Also apply it to Barbarians
	{
		CvHandicapInfo const& h = GC.getInfo(kGame.getHandicapType());
		iAIModifier = h.getAIGrowthPercent() +
				//h.getAIPerEraModifier() * getCurrentEra()
				kGame.AIHandicapAdjustment();
	} // Reduce rounding error:
	iThreshold = (iThreshold * per100(iAIModifier) *
			per100(GC.getInfo(kGame.getGameSpeedType()).getGrowthPercent()) *
			per100(GC.getInfo(kGame.getStartEra()).getGrowthPercent())).round();
	// </advc.251>
	return std::max(1, iThreshold);
}


void CvPlayer::verifyUnitStacksValid()
{
	FOR_EACH_UNIT_VAR(pLoopUnit, *this)
		pLoopUnit->verifyStackValid();
}


UnitTypes CvPlayer::getTechFreeUnit(TechTypes eTech) const
{
	UnitClassTypes eUnitClass = (UnitClassTypes)GC.getInfo(eTech).
			getFirstFreeUnitClass();
	if (eUnitClass == NO_UNITCLASS)
		return NO_UNIT;

	UnitTypes eUnit = getCivilization().getUnit(eUnitClass);
	if (eUnit == NO_UNIT)
		return NO_UNIT;

	if (GC.getInfo(eUnit).getEspionagePoints() > 0 &&
			GC.getGame().isOption(GAMEOPTION_NO_ESPIONAGE))
		return NO_UNIT;

	return eUnit;
}

// BULL - Trade Hover:  // advc: _MOD_FRACTRADE removed
/*  Adds the yield and count for each trade route with eWithPlayer to the
	int references (out parameters). */
void CvPlayer::calculateTradeTotals(YieldTypes eYield,
	int& iDomesticYield, int& iDomesticRoutes, int& iForeignYield, int& iForeignRoutes,
	PlayerTypes eWithPlayer) const
{
	FOR_EACH_CITY(pCity, *this)
	{
		pCity->calculateTradeTotals(eYield, iDomesticYield, iDomesticRoutes,
				iForeignYield, iForeignRoutes, eWithPlayer);
	}
}

// <advc.085>
void CvPlayer::setScoreboardExpanded(bool b)
{
	CvGame& kGame = GC.getGame();
	/*	When a popup (e.g. diplo) is open, my code for detecting whether
		the cursor has been moved away from the scoreboard causes the
		hover text box to flicker. Don't think I can fix that.
		Workaround: If the player expands the scoreboard during
		diplomacy, it remains expanded until diplomacy ends. */
	if (gDLL->UI().isFocused())
	{
		if (!BUGOption::isEnabled("Scores__ExpandOnHover", false, false))
			return;
		// Expand the scoreboard
		if (b && !m_bScoreboardExpanded)
		{
			gDLL->UI().setDirty(Score_DIRTY_BIT, true);
			m_bScoreboardExpanded = b;
		}
		/*	Needed in conjunction with an update timer set by
			CvDLLWidgetData::doContactCiv to prevent the scoreboard
			from already getting stuck at expanded when the player
			clicks on the scoreboard to initiate diplomacy. */
		if (gDLL->isDiplomacy())
			m_bScoreboardExpanded = b;

		// Schedule callback for collapse
		kGame.setUpdateTimer(CvGame::UPDATE_COLLAPSE_SCORE_BOARD, 1);
		// Ignore callback while diplomacy ongoing
		return;
	}
	if (b)
	{
		FAssert(BUGOption::isEnabled("Scores__AlignIcons", true, false));
		if (BUGOption::isEnabled("Scores__ExpandOnHover", false, false))
		{
			/*  A delay of 1 means that the scoreboard collapses after
				two game updates (250 ms) */
			int const iDelay = 1;
			/*  So long as the mouse hovers over a scoreboard widget,
				setScoreboardExpanded(true) keeps getting called and
				the collapse timer keeps getting reset. */
			kGame.setUpdateTimer(CvGame::UPDATE_COLLAPSE_SCORE_BOARD, iDelay);
			// Scoreboard needs to be redrawn when expanding
			if (!isScoreboardExpanded())
				gDLL->UI().setDirty(Score_DIRTY_BIT, true);
			/*  The EXE calls CvDLLWidgetData::parseHelp when the cursor is moved
				onto a widget - but not while the cursor rests there. Workaround:
				Redraw the scoreboard and its widgets. That also causes a parseHelp
				call (if the cursor is on a widget). Must still allow game updates
				in between though (otherwise, e.g. animations will start lagging).
				Therefore, don't set the dirty bit until the next game update. */
			else kGame.setUpdateTimer(CvGame::UPDATE_DIRTY_SCORE_BOARD, iDelay - 1);
			m_bScoreboardExpanded = true;
		}
		else m_bScoreboardExpanded = false;
		return;
	}
	// Collapse callback from CvGame
	if (isScoreboardExpanded())
	{
		m_bScoreboardExpanded = false;
		gDLL->UI().setDirty(Score_DIRTY_BIT, true);
		/*  For some strange reason, the HUD retains mouse focus after expanding
			the scoreboard, and this is the only remedy I was able to find
			(apart from CvInterface::makeInterfaceDirty, which is far costlier). */
		gDLL->UI().makeSelectionListDirty();
	}
}


bool CvPlayer::isScoreboardExpanded() const
{
	return m_bScoreboardExpanded;
} // </advc.085>


void CvPlayer::buildTradeTable(PlayerTypes eOtherPlayer, CLinkList<TradeData>& kOurInventory) const
{
	PROFILE_FUNC(); // advc.opt (not frequently called)
	TradeData item;
	bool const bOtherHuman = GET_PLAYER(eOtherPlayer).isHuman(); // advc.opt

	setTradeItem(&item, TRADE_GOLD);
	if (canTradeItem(eOtherPlayer, item))
		kOurInventory.insertAtEnd(item);

	setTradeItem(&item, TRADE_GOLD_PER_TURN);
	if (canTradeItem(eOtherPlayer, item))
		kOurInventory.insertAtEnd(item);

	setTradeItem(&item, TRADE_MAPS, 0);
	if (canTradeItem(eOtherPlayer, item))
		kOurInventory.insertAtEnd(item);

	setTradeItem(&item, TRADE_VASSAL, 0);
	if (canTradeItem(eOtherPlayer, item))
		kOurInventory.insertAtEnd(item);

	setTradeItem(&item, TRADE_OPEN_BORDERS);
	if (canTradeItem(eOtherPlayer, item))
		kOurInventory.insertAtEnd(item);

	setTradeItem(&item, TRADE_DEFENSIVE_PACT);
	if (canTradeItem(eOtherPlayer, item))
		kOurInventory.insertAtEnd(item);

	setTradeItem(&item, TRADE_PERMANENT_ALLIANCE);
	if (canTradeItem(eOtherPlayer, item))
		kOurInventory.insertAtEnd(item);

	if (GET_TEAM(getTeam()).isAtWar(TEAMID(eOtherPlayer)))
	{
		setTradeItem(&item, TRADE_PEACE_TREATY);
		kOurInventory.insertAtEnd(item);

		setTradeItem(&item, TRADE_SURRENDER, 0);
		if (canTradeItem(eOtherPlayer, item))
			kOurInventory.insertAtEnd(item);
	}
	/*  <advc.104m> Make peace treaties hidden items at peacetime
		so that the trade screen (EXE) doesn't discard them from
		tribute and help requests. */
	else
	{
		setTradeItem(&item, TRADE_PEACE_TREATY);
		item.m_bHidden = true;
		kOurInventory.insertAtEnd(item);
	} // </advc.104m>

	//	Initial build of the inventory lists and buttons.
	//	Go through all the possible headings
	for (int i = NUM_BASIC_ITEMS; i < NUM_TRADEABLE_HEADINGS; i++)
	{	// <advc.opt>
		TradeableItems eItemType = (TradeableItems)i;
		if (!canPossiblyTradeItem(eOtherPlayer, eItemType))
			continue; // </advc.opt>
		bool bFoundItemUs = false;
		//	Build what we need to build for this item
		switch (eItemType)
		{
		case TRADE_TECHNOLOGIES:
			FOR_EACH_ENUM(Tech)
			{
				setTradeItem(&item, TRADE_TECHNOLOGIES, eLoopTech);
				if (canTradeItem(eOtherPlayer, item))
				{
					bFoundItemUs = true;
					kOurInventory.insertAtEnd(item);
				}
			}
			break;

		case TRADE_RESOURCES:
			FOR_EACH_ENUM(Bonus)
			{
				setTradeItem(&item, TRADE_RESOURCES, eLoopBonus);
				if (!canTradeItem(eOtherPlayer, item))
					continue;
				// <advc.074>
				bool const bHuman = (bOtherHuman || isHuman());
				bool bValid = (!bHuman || getTradeDenial(eOtherPlayer, item) != DENIAL_JOKING);
				/*  Hack: Check if we're expecting a renegotiate-popup from the EXE.
					Don't want any resources in the canceled deal to be excluded
					from the trade table. */
				if (!bValid && !GC.getGame().isInBetweenTurns() &&
					// Probably too complicated to get this right with simultaneous turns
					!GC.getGame().isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
				{
					for (CLLNode<std::pair<PlayerTypes,BonusTypes> >* pNode =
						m_cancelingExport.head(); pNode != NULL; pNode =
						m_cancelingExport.next(pNode))
					{
						PlayerTypes eCancelPlayer = pNode->m_data.first;
						BonusTypes eCancelBonus = pNode->m_data.second;
						FAssert(eCancelPlayer != getID());
						if (eCancelPlayer == eOtherPlayer && eCancelBonus == eLoopBonus)
						{
							bValid = true;
							m_cancelingExport.deleteNode(pNode);
							break;
						}
					}
				}
				if(bValid) // </advc.074>
				{
					bFoundItemUs = true;
					kOurInventory.insertAtEnd(item);
				}
			}
			break;

		case TRADE_CITIES:
		{
			FOR_EACH_CITY(pLoopCity, *this)
			{
				//if (AI_cityTrade(pLoopCity, eOtherPlayer) != DENIAL_NEVER) // K-Mod
				setTradeItem(&item, TRADE_CITIES, pLoopCity->getID());
				if (canTradeItem(eOtherPlayer, item))
				{
					/*  advc.ctr: If "never" cities are excluded, then "too much"
						should be excluded as well and the check should be done
						after the faster canTradeItem check. But I don't want to
						exclude either anymore. */
					/*bool bValid = !bOtherHuman;
					if (!bValid) 
					{
						DenialTypes eDenial = getTradeDenial(eOtherPlayer, item);
						if (eDenial != DENIAL_NEVER && eDenial != DENIAL_TOO_MUCH)
							bValid = true;
					}
					if (bValid)*/
					{
						bFoundItemUs = true;
						kOurInventory.insertAtEnd(item);
					}
				}
			}
			break;
		}
		case TRADE_PEACE: // advc: Merged these three cases
		case TRADE_WAR:
		case TRADE_EMBARGO:
			if (isHuman())
				break;
			for (TeamIter<MAJOR_CIV,NOT_SAME_TEAM_AS> itThirdTeam(getTeam());
				itThirdTeam.hasNext(); ++itThirdTeam)
			{
				if (itThirdTeam->getID() == TEAMID(eOtherPlayer))
					continue;
				setTradeItem(&item, eItemType, itThirdTeam->getID());
				if (canTradeItem(eOtherPlayer, item))
				{
					kOurInventory.insertAtEnd(item);
					bFoundItemUs = true;
				}
			}
			break;

		case TRADE_CIVIC:
			FOR_EACH_ENUM(Civic)
			{
				setTradeItem(&item, TRADE_CIVIC, eLoopCivic);
				if (canTradeItem(eOtherPlayer, item))
				{	// <advc.074>
					if((!bOtherHuman && !isHuman()) ||
						getTradeDenial(eOtherPlayer, item) != DENIAL_JOKING) {
					// </advc.074>
						bFoundItemUs = true;
						kOurInventory.insertAtEnd(item);
					}
				}
			}
			break;

		case TRADE_RELIGION:
			FOR_EACH_ENUM(Religion)
			{
				setTradeItem(&item, TRADE_RELIGION, eLoopReligion);
				if (canTradeItem(eOtherPlayer, item))
				{
					bFoundItemUs = true;
					kOurInventory.insertAtEnd(item);
				}
			}
			break;
		}
	}
}

/*	advc (note): This function doesn't use any CvPlayer members, but I suppose
	that could change. Can't easily move it b/c of DllExport. */
bool CvPlayer::getHeadingTradeString(PlayerTypes eOtherPlayer, TradeableItems eItem,
	CvWString& szString, CvString& szIcon) const
{
	szIcon.clear();

	CvWString szTag;
	switch (eItem)
	{
	case TRADE_TECHNOLOGIES:
		szTag = L"TXT_KEY_CONCEPT_TECHNOLOGY";
		break;

	case TRADE_RESOURCES:
		szTag = L"TXT_KEY_TRADE_RESOURCES";
		break;

	case TRADE_CITIES:
		szTag = L"TXT_KEY_TRADE_CITIES";
		break;

	case TRADE_PEACE:
		szTag = L"TXT_KEY_TRADE_MAKE_PEACE_WITH";
		break;

	case TRADE_WAR:
		szTag = L"TXT_KEY_TRADE_DECLARE_WAR_ON";
		break;

	case TRADE_EMBARGO:
		szTag = L"TXT_KEY_TRADE_STOP_TRADING_WITH";
		break;

	case TRADE_CIVIC:
		szTag = L"TXT_KEY_TRADE_ADOPT";
		break;

	case TRADE_RELIGION:
		szTag = L"TXT_KEY_TRADE_CONVERT";
		break;
	default:
		szString.clear();
		return false;
	}
	szString = gDLL->getText(szTag); // advc: Moved out of the cases

	return true;
}


bool CvPlayer::getItemTradeString(PlayerTypes eRecipient, bool bOffer,
	bool bShowingCurrent, const TradeData& zTradeData,
	CvWString& szString, CvString& szIcon) const
{
	szIcon.clear();
	// <advc.072>
	CvDeal* pDeal = NULL;
	if (bShowingCurrent)
	{
		int iTurnsLeftMode = BUGOption::getValue("Advisors__DealTurnsLeft", 3);
		if (iTurnsLeftMode == 2 || iTurnsLeftMode == 1)
		{
			TradeableItems eItemType = zTradeData.m_eItemType;
			if (CvDeal::isAnnual(eItemType) || eItemType == TRADE_PEACE_TREATY)
			{
				pDeal = GC.getGame().nextCurrentDeal(eRecipient, getID(),
						eItemType, zTradeData.m_iData);
				/*  Call nextCurrentDeal for all annual deals plus peace treaties
					in order to stay in-sync with the iteration done by the EXE,
					but we're only interested in a subset here: */
				if (!CvDeal::isDual(eItemType) && eItemType != TRADE_RESOURCES &&
					eItemType != TRADE_GOLD_PER_TURN)
				{
					pDeal = NULL;
				}
			}
		}
	} // </advc.072>
	switch (zTradeData.m_eItemType)
	{
	case TRADE_GOLD:
		if (bOffer)
		{
			szString = gDLL->getText("TXT_KEY_TRADE_GOLD_NUM",
					zTradeData.m_iData);
		}
		else
		{
			szString = gDLL->getText("TXT_KEY_TRADE_GOLD_NUM",
					AI().AI_maxGoldTrade(eRecipient));
		}
		break;
	case TRADE_GOLD_PER_TURN:
		if (bOffer)
		{
			szString = gDLL->getText("TXT_KEY_TRADE_GOLD_PER_TURN_NUM",
					zTradeData.m_iData);
		}
		else
		{
			szString = gDLL->getText("TXT_KEY_TRADE_GOLD_PER_TURN_NUM",
					AI().AI_maxGoldPerTurnTrade(eRecipient));
		}
		break;
	case TRADE_MAPS:
		szString = gDLL->getText("TXT_KEY_TRADE_WORLD_MAP_STRING");
		break;
	case TRADE_VASSAL:
		szString = gDLL->getText("TXT_KEY_TRADE_VASSAL_STRING");
		break;
	case TRADE_SURRENDER:
		szString = gDLL->getText("TXT_KEY_TRADE_CAPITULATE_STRING");
		break;
	case TRADE_OPEN_BORDERS:
		szString = gDLL->getText("TXT_KEY_TRADE_OPEN_BORDERS_STRING");
		break;
	case TRADE_DEFENSIVE_PACT:
		szString = gDLL->getText("TXT_KEY_TRADE_DEFENSIVE_PACT_STRING");
		break;
	case TRADE_PERMANENT_ALLIANCE:
		szString = gDLL->getText("TXT_KEY_TRADE_PERMANENT_ALLIANCE_STRING");
		break;
	case TRADE_PEACE_TREATY:
		if(GET_TEAM(eRecipient).isAtWar(getTeam())) // advc.072
		{
			szString = gDLL->getText("TXT_KEY_TRADE_PEACE_TREATY_STRING",
					GC.getDefineINT(CvGlobals::PEACE_TREATY_LENGTH));
		}  // <advc.104m> AI offering a peace treaty at peacetime
		else if (bOffer)
		{
			szString = gDLL->getText("TXT_KEY_TRADE_SIGN_PEACE_TREATY",
					GC.getDefineINT(CvGlobals::PEACE_TREATY_LENGTH));
		} // </advc.104m>
		else szString = gDLL->getText("TXT_KEY_TRADE_PEACE_TREATY_STR"); // advc.072
		break;
	case TRADE_TECHNOLOGIES:
		szString = GC.getInfo((TechTypes)zTradeData.m_iData).getDescription();
		szIcon = GC.getInfo((TechTypes)zTradeData.m_iData).getButton();
		break;
	case TRADE_RESOURCES:
	{
		BonusTypes const eBonus = (BonusTypes)zTradeData.m_iData; // advc
		if (bOffer)
		{
			int iResources = GET_PLAYER(eRecipient).getNumTradeableBonuses(eBonus);
			if (bShowingCurrent)
				iResources++;
			szString = gDLL->getText("TXT_KEY_TRADE_RESOURCE",
					GC.getInfo(eBonus).getDescription(), iResources);

		}
		else
		{
			szString.Format( L"%s (%d)", GC.getInfo(eBonus).getDescription(),
					getNumTradeableBonuses(eBonus));
		}
		szIcon = GC.getInfo(eBonus).getButton();
		break;
	}
	case TRADE_CITIES:
	{
		CvCity const* pCity = NULL;
		if (bOffer)
			pCity = GET_PLAYER(eRecipient).getCity(zTradeData.m_iData);
		else pCity = getCity(zTradeData.m_iData);
		if (pCity != NULL)
		{
			if (pCity->getLiberationPlayer() == //eOtherPlayer)
				(bOffer ? getID() : eRecipient)) // advc.001 (bugfix?), advc.ctr
			{
				szString.Format(L"%s (%s)", pCity->getName().GetCString(),
						gDLL->getText("TXT_KEY_LIBERATE_CITY").GetCString());
			}
			else szString = gDLL->getText("TXT_KEY_CITY_OF", pCity->getNameKey());
		}
		break;
	}
	case TRADE_PEACE:
		if (bOffer)
		{
			szString = gDLL->getText("TXT_KEY_TRADE_PEACE_WITH");
			szString += GET_TEAM((TeamTypes)zTradeData.m_iData).getName();
		}
		else szString = GET_TEAM((TeamTypes)zTradeData.m_iData).getName();
		break;
	case TRADE_WAR:
		if (bOffer)
		{
			szString = gDLL->getText("TXT_KEY_TRADE_WAR_WITH");
			szString += GET_TEAM((TeamTypes)zTradeData.m_iData).getName();
		}
		else szString = GET_TEAM((TeamTypes)zTradeData.m_iData).getName();
		break;
	case TRADE_EMBARGO:
		if (bOffer)
		{
			szString = gDLL->getText("TXT_KEY_TRADE_STOP_TRADING_WITH");
			szString += L" " + GET_TEAM((TeamTypes)zTradeData.m_iData).getName();
		}
		else szString = GET_TEAM((TeamTypes)zTradeData.m_iData).getName();
		break;
	case TRADE_CIVIC:
		if (bOffer)
		{
			szString = gDLL->getText("TXT_KEY_TRADE_ADOPT");
			szString += GC.getInfo((CivicTypes)zTradeData.m_iData).getDescription();
		}
		else szString = GC.getInfo((CivicTypes)zTradeData.m_iData).getDescription();
		szIcon = GC.getInfo((CivicTypes)zTradeData.m_iData).getButton();
		break;
	case TRADE_RELIGION:
		if (bOffer)
		{
			szString = gDLL->getText("TXT_KEY_TRADE_CONVERT");
			szString += GC.getInfo((ReligionTypes)zTradeData.m_iData).getDescription();
		}
		else szString = GC.getInfo((ReligionTypes)zTradeData.m_iData).getDescription();
		szIcon = GC.getInfo((ReligionTypes)zTradeData.m_iData).getButton();
		break; // <advc.034>
	case TRADE_DISENGAGE:
		szString.clear();
		GAMETEXT.buildDisengageString(szString, getID(), eRecipient);
		break; // </advc.034>
	default:
		szString.clear();
		return false;
	}
	// <advc.072>
	if(pDeal != NULL && pDeal->turnsToCancel() > 0)
	{
		szString.append(L" ");
		szString.append(gDLL->getText("INTERFACE_CITY_TURNS",
				pDeal->turnsToCancel()).GetCString());
	} // </advc.072>
	return true;
}


void CvPlayer::updateTradeList(PlayerTypes eOtherPlayer, CLinkList<TradeData>& kOurInventory,
	CLinkList<TradeData> const& kOurOffer, CLinkList<TradeData> const& kTheirOffer) const
{
	FOR_EACH_TRADE_ITEM_VAR(kOurInventory)
	{
		pItem->m_bHidden = false;

		// Don't show peace treaties when not at war
		if (!::atWar(getTeam(), GET_PLAYER(eOtherPlayer).getTeam()))
		{
			if (pItem->m_eItemType == TRADE_PEACE_TREATY ||
				pItem->m_eItemType == TRADE_SURRENDER)
			{
				pItem->m_bHidden = true;
			}
		}
		// Don't show technologies with no tech trading game option
		if (GC.getGame().isOption(GAMEOPTION_NO_TECH_TRADING) &&
			pItem->m_eItemType == TRADE_TECHNOLOGIES)
		{
			pItem->m_bHidden = true;
		}
	}

	FOR_EACH_TRADE_ITEM_VAR(kOurInventory)
	{
		switch (pItem->m_eItemType)
		{
		case TRADE_PEACE_TREATY:
		{
			FOR_EACH_TRADE_ITEM2(pOfferItem, kOurOffer)
			{
				// Don't show vassal deals if peace treaty is already on the table
				if (CvDeal::isVassal(pOfferItem->m_eItemType))
				{
					pItem->m_bHidden = true;
					break;
				}
			}
			break;
		}
		case TRADE_VASSAL:
		case TRADE_SURRENDER:
		{
			FOR_EACH_TRADE_ITEM2(pOfferItem, kTheirOffer)
			{
				// Don't show vassal deals if another type of vassal deal is on the table
				if (CvDeal::isVassal(pOfferItem->m_eItemType))
				{
					pItem->m_bHidden = true;
					break;
				}
			}
			if (pItem->m_bHidden)
				break;
			FOR_EACH_TRADE_ITEM2(pOfferItem, kOurOffer)
			{
				// Don't show peace deals if the other player is offering to be a vassal
				if (CvDeal::isEndWar(pOfferItem->m_eItemType))
				{
					pItem->m_bHidden = true;
					break;
				}
			}
			break;
		}
		/*// <advc.004> Only one side can pay gold  [better keep all gold on display I guess]
		case TRADE_GOLD:
		case TRADE_GOLD_PER_TURN:
			// Perhaps better to keep AI gold on display (human treasury is always visible anyway)
			if (isHuman()) {
				for (CLLNode<TradeData>* pOfferNode = theirOffer.head(); pOfferNode != NULL;
						pOfferNode = theirOffer.next(pOfferNode)) {
					TradeableItems eItemType = pOfferNode->m_data.m_eItemType;
					if (eItemType == TRADE_GOLD || eItemType == TRADE_GOLD_PER_TURN)
						pNode->m_data.m_bHidden = true;
				}
				break;
			} // </advc.004>*/
		}
	}

	if (!isHuman() || !GET_PLAYER(eOtherPlayer).isHuman()) // everything allowed in human-human trades
	{
		CLLNode<TradeData>* pFirstOffer = kOurOffer.head();
		if (pFirstOffer == NULL)
			pFirstOffer = kTheirOffer.head();
		if (pFirstOffer != NULL)
		{
			//if (!CvDeal::isEndWar(pFirstOffer->m_data.m_eItemType) || !::atWar(getTeam(), GET_PLAYER(eOtherPlayer).getTeam()))
			if (!::atWar(getTeam(), GET_PLAYER(eOtherPlayer).getTeam())) // K-Mod
			{
				FOR_EACH_TRADE_ITEM_VAR(kOurInventory)
				{	// advc.ctr: Allow cities to be bought
					/*if (pFirstOffer->m_data.m_eItemType == TRADE_CITIES || pNode->m_data.m_eItemType == TRADE_CITIES)
						pNode->m_data.m_bHidden = true;
					else*/
					if (CvDeal::isAnnual(pFirstOffer->m_data.m_eItemType) !=
						CvDeal::isAnnual(pItem->m_eItemType))
					{
						pItem->m_bHidden = true;
					}
				}
			}
		}
	}
	/*	<advc.ctr> Add PEACE_TREATY items to offers that imply a peace treaty.
		Note that this is only done to inform the human player. There is code
		elsewhere for signing the peace treaty upon implementation of the deal.
		For deals that involve a human, the game will end up attemping to
		implement the peace treaty twice. No harm in that.
		I don't know if updateTradeList gets called for all AI trades;
		don't want to rely on it. */
	if ((GC.getGame().getActivePlayer() != getID() &&
		GC.getGame().getActivePlayer() != eOtherPlayer) ||
		GET_TEAM(eOtherPlayer).isAtWar(getTeam()))
	{
		return;
	}
	TradeableItems eForcePeaceItemType = NO_TRADE_ITEM;
	TradeableItems aeForcePeace[] = { TRADE_CITIES, /* advc.146: */ TRADE_WAR };
	int const iForcePeaceSz = ARRAY_LENGTH(aeForcePeace);
	/*	When a player tries to deselect a peace treaty, the trade screen will
		remove it (temporarily) only from one player's side. Therefore need
		to check each player individually for an offered peace treaty. */
	bool abPeaceTreatyFound[] = { false, false };
	CLinkList<TradeData> const* apOffers[] = { &kOurOffer, &kTheirOffer };
	for (int i = 0; i < 2; i++)
	{
		FOR_EACH_TRADE_ITEM(*apOffers[i])
		{
			if (pItem->m_eItemType == TRADE_PEACE_TREATY)
			{
				FAssert(!abPeaceTreatyFound[i]);
				abPeaceTreatyFound[i] = true;
			}
			else FAssert(!CvDeal::isEndWar(pItem->m_eItemType));
			for (int j = 0; j < iForcePeaceSz; j++)
			{
				if (pItem->m_eItemType == aeForcePeace[j])
				{
					// No peace treaty through liberation
					if (pItem->m_eItemType == TRADE_CITIES)
					{
						CvCity const* pCity = GET_PLAYER(
								i == 0 ? getID() : eOtherPlayer).
								getCity(pItem->m_iData);
						if (pCity == NULL || pCity->getLiberationPlayer() == getID())
							continue;
					}
					eForcePeaceItemType = pItem->m_eItemType;
				}
			}
		}
	}
	bool bPeaceTreatyNeeded = (eForcePeaceItemType != NO_TRADE_ITEM);
	if (!abPeaceTreatyFound[0] && !abPeaceTreatyFound[1] && !bPeaceTreatyNeeded)
		return;
	/*	kOurOffer and kTheirOffer are most likely not really const objects.
		Perhaps call-by-const-reference could've gotten replaced with call-by-value
		when the EXE was compiled, but tests show that this hasn't happened. */
	CLinkList<TradeData>* apOffersVar[] =
	{
		const_cast<CLinkList<TradeData>*>(&kOurOffer),
		const_cast<CLinkList<TradeData>*>(&kTheirOffer)
	};
	/*	Always remove any peacetime peace treaties. Re-append if necessary.
		So that they're always at the bottom of the offer lists.
		That's important for CvDiplomacy.determineResponses in Python. */
	for (int i = 0; i < 2; i++)
	{
		if (!abPeaceTreatyFound[i])
			continue;
		for (CLLNode<TradeData>* pNode = apOffersVar[i]->head();
			pNode != NULL; pNode = apOffersVar[i]->next(pNode))
		{
			if (pNode->m_data.m_eItemType == TRADE_PEACE_TREATY)
			{
				apOffersVar[i]->deleteNode(pNode);
				break; // There should be at most one
			}
		}
	}
	if (bPeaceTreatyNeeded)
	{
		for (int i = 0; i < 2; i++)
		{
			TradeData item(TRADE_PEACE_TREATY);
			if (canTradeItem(eOtherPlayer, item) &&
				GET_PLAYER(eOtherPlayer).canTradeItem(getID(), item))
			{
				item.m_bOffering = true;
				/*	So that we can later (when processing trade values) tell
					whether the deal is the result of an AI request.
					(If it comes through here, then there's no AI request.) */
				item.m_iData = eForcePeaceItemType;
				apOffersVar[i]->insertAtEnd(item);
				if (i == 0)
				{
					/*	Update offering status in inventory
						(though probably unnecessary) */
					FOR_EACH_TRADE_ITEM_VAR(kOurInventory)
					{
						if (pItem->m_eItemType == TRADE_PEACE_TREATY)
							pItem->m_bOffering = true;
					}
				}
			}
		}
	} // </advc.ctr>
}

/*	K-Mod: Find each item from the offer list in the inventory list -
	mark it as m_bOffering == true.
	(This is usually done somewhere in the game engine,
	or when the offer list is being generated or something.) */
void CvPlayer::markTradeOffers(CLinkList<TradeData>& kOurInventory,
	// advc: Was const, but we do change the bOffering status.
	CLinkList<TradeData>& kOurOffer) const
{
	FOR_EACH_TRADE_ITEM_VAR2(pOfferItem, kOurOffer)
	{
		CLLNode<TradeData>* pInvNode; // (defined here just for the assertion at the end)
		for (pInvNode = kOurInventory.head(); pInvNode != NULL;
			pInvNode = kOurInventory.next(pInvNode))
		{
			if (pInvNode->m_data.m_eItemType == pOfferItem->m_eItemType &&
				pInvNode->m_data.m_iData == pOfferItem->m_iData)
			{
				pInvNode->m_data.m_bOffering = pOfferItem->m_bOffering = true;
				break;
			}
		}
		FAssertMsg(pInvNode != NULL ||
				// <advc.134a> I guess it's OK that these aren't part of the inventory
				pOfferItem->m_eItemType == TRADE_SURRENDER ||
				pOfferItem->m_eItemType == TRADE_PEACE_TREATY, // </advc.134a>
				"failed to find offered item in inventory");
	}
}


int CvPlayer::getIntroMusicScriptId(PlayerTypes eForPlayer) const
{
	EraTypes eEra = GET_PLAYER(eForPlayer).getCurrentEra();
	CvLeaderHeadInfo const& kLeader = GC.getInfo(getPersonalityType());
	if (GET_TEAM(eForPlayer).isAtWar(getTeam()))
		return kLeader.getDiploWarIntroMusicScriptIds(eEra);
	return kLeader.getDiploPeaceIntroMusicScriptIds(eEra);
}

int CvPlayer::getMusicScriptId(PlayerTypes eForPlayer) const
{
	EraTypes eEra = GET_PLAYER(eForPlayer).getCurrentEra();
	CvLeaderHeadInfo const& kLeader = GC.getInfo(getLeaderType());
	if (GET_TEAM(eForPlayer).isAtWar(getTeam()))
		return kLeader.getDiploWarMusicScriptIds(eEra);
	return kLeader.getDiploPeaceMusicScriptIds(eEra);
}


void CvPlayer::getGlobeLayerColors(GlobeLayerTypes eGlobeLayerType, int iOption,
	std::vector<NiColorA>& aColors, std::vector<CvPlotIndicatorData>& aIndicators) const
{
	PROFILE_FUNC(); // advc.opt
	CvGame const& kGame = GC.getGame();
	// <advc.004z> Too early; player options haven't been set yet.
	if (kGame.getTurnSlice() <= 0)
		return; // </advc.004z>
	/*  <advc> These get cleared by some of the subroutines, but should be
		empty to begin with. If not, there could be a memory leak. */
	FAssert(aColors.empty() && aIndicators.empty());
	FAssert(getID() == kGame.getActivePlayer()); // </advc>
	// <advc.706>
	if(kGame.isOption(GAMEOPTION_RISE_FALL) && CvPlot::isAllFog())
		return; // </advc.706>
	switch (eGlobeLayerType)
	{
	case GLOBE_LAYER_TRADE:
		getTradeLayerColors(aColors, aIndicators);
		break;
	case GLOBE_LAYER_UNIT:
		getUnitLayerColors((GlobeLayerUnitOptionTypes)iOption, aColors, aIndicators);
		break;
	case GLOBE_LAYER_RESOURCE:
		getResourceLayerColors((GlobeLayerResourceOptionTypes)iOption, aColors, aIndicators);
		break;
	case GLOBE_LAYER_RELIGION:
		getReligionLayerColors((ReligionTypes)iOption, aColors, aIndicators);
		break;
	case GLOBE_LAYER_CULTURE:
		getCultureLayerColors(aColors, aIndicators);
		break;
	default:
		FErrorMsg("Unknown globe layer type");
	}
}

// Used by Globeview trade layer
void CvPlayer::getTradeLayerColors(std::vector<NiColorA>& aColors,
	std::vector<CvPlotIndicatorData>& aIndicators) const
{
	CvMap const& kMap = GC.getMap();
	aColors.resize(kMap.numPlots(), NiColorA(0, 0, 0, 0));
	aIndicators.clear();
	// <advc.004z>
	static bool const bSHOW_FOREIGN = (GC.getDefineINT("FOREIGN_GROUPS_ON_TRADE_LAYER") > 0);
	static bool const bSHOW_CAPITAL_CONN = (GC.getDefineINT("CONNECTION_TO_CAPITAL_ON_TRADE_LAYER") > 0);
	// </advc.004z>
	typedef std::map< int, std::vector<int> > PlotGroupMap;
	PlotGroupMap mapPlotGroups;
	std::vector<int> aiNotConn; // advc.004z
	for (int iPlot = 0; iPlot < kMap.numPlots(); iPlot++)
	{
		CvPlot const& kPlot = kMap.getPlotByIndex(iPlot);
		PlayerTypes const eOwner = kPlot.getOwner(); // advc.004z
		CvPlotGroup const* pPlotGroup = kPlot.getPlotGroup(getID());
		if (pPlotGroup != NULL && kPlot.isRevealed(getTeam(), true) &&
			(kPlot.getTeam() == getTeam() ||
			// <advc.004z>
			(bSHOW_FOREIGN && !kPlot.isImpassable() &&
			eOwner != BARBARIAN_PLAYER &&
			// Get rid of insignificant groups
			(pPlotGroup->getLengthPlots() >= 5 ||
			!kPlot.isWater() || eOwner != NO_PLAYER))))
		{
			if(bSHOW_CAPITAL_CONN && kPlot.isVisible(getTeam()) &&
				kPlot.isCity() && !kPlot.getPlotCity()->isConnectedToCapital())
			{
				aiNotConn.push_back(iPlot);
			}
			else // </advc.004z>
				mapPlotGroups[pPlotGroup->getID()].push_back(iPlot);
		}
	} // <advc.004z> Use the player color for the plot group of the capital
	CvPlotGroup* pCapitalGroup = (!hasCapital() ? NULL :
			getCapital()->getPlot().getPlotGroup(getID()));
	if (pCapitalGroup != NULL)
	{
		FAssert(pCapitalGroup->getLengthPlots() > 0);
		int iCount = mapPlotGroups.count(pCapitalGroup->getID());
		if(iCount > 0)
		{
			NiColorA kColor(getPlayerTextColorR() / 255.f,
					getPlayerTextColorG() / 255.f,
					getPlayerTextColorB() / 255.f,
					getPlayerTextColorA() / 255.f * (bSHOW_FOREIGN ? 0.5f : 0.8f));
			std::vector<int>& aPlots = mapPlotGroups[pCapitalGroup->getID()];
			for(size_t i = 0; i < aPlots.size(); i++)
				aColors[aPlots[i]] = kColor;
		}
		else FAssert(iCount > 0);
	} // </advc.004z>
	CvRandom kRandom;
	kRandom.init(42);
	for (PlotGroupMap::iterator it = mapPlotGroups.begin();
		it != mapPlotGroups.end(); ++it)
	{	// <advc.004z> Already handled above
		if(pCapitalGroup != NULL && it->first == pCapitalGroup->getID())
			continue; // </advc.004z>
		NiColorA kColor(kRandom.getFloat(), kRandom.getFloat(), kRandom.getFloat(),
				// advc.004z: Can't tell apart land and water at 0.8
				bSHOW_FOREIGN ? 0.5f :
				0.8f);
		std::vector<int>& aPlots = it->second;
		for (size_t i = 0; i < aPlots.size(); ++i)
		{
			aColors[aPlots[i]] = kColor;
		}
		/*  <advc.004z> The first random color is good (white), but one of the
			next few is a light blue that is too similar to the white.
			Discard one float to avoid that color. */
		kRandom.getFloat();
	}
	NiColorA kColor(0, 0, 0, 0.75f);
	for (size_t i = 0; i < aiNotConn.size(); i++)
		aColors[aiNotConn[i]] = kColor; // </advc.004z>
}

// Used by Globeview unit layer
void CvPlayer::getUnitLayerColors(GlobeLayerUnitOptionTypes eOption,
	std::vector<NiColorA>& aColors, std::vector<CvPlotIndicatorData>& aIndicators) const
{
	// <advc.004z>
	if(eOption == GLOBE_LAYER_UNIT_DUMMY)
		eOption = SHOW_ALL_MILITARY;
	// </advc.004z>
	CvMap const& kMap = GC.getMap();
	aColors.resize(kMap.numPlots(), NiColorA(0, 0, 0, 0));
	aIndicators.clear();

	std::vector< std::vector<float> > aafPlayerPlotStrength(MAX_PLAYERS);
	for (PlayerIter<ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		aafPlayerPlotStrength[itPlayer->getID()].resize(kMap.numPlots());
	}

	float fMaxPlotStrength = 0.0f;

	// create unit plot indicators...
	// build the trade group texture
	typedef std::map<int, NiColor> GroupMap;
	GroupMap mapColors;

	// Loop through all the players  (advc: reduced indentation in the loops)
	CvWStringBuffer szBuffer;
	for (PlayerIter<ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		PlayerTypes const eLoopPlayer = itPlayer->getID();
		for (int iPlot = 0; iPlot < kMap.numPlots(); iPlot++)
		{
			CvPlot const& kPlot = kMap.getPlotByIndex(iPlot);
			int iNumUnits = kPlot.getNumUnits();
			if (iNumUnits <= 0 || !kPlot.isVisible(getTeam(), true))
				continue;

			float fPlotStrength = 0.0f;
			bool bShowIndicator = false;

			FOR_EACH_UNIT_IN(pUnit, kPlot)
			{
				if (pUnit->getVisualOwner() != eLoopPlayer ||
					pUnit->isInvisible(getTeam(), true))
				{
					continue;
				}
				// now, is this unit of interest?
				bool bMilitary = //pUnit->baseCombatStr() > 0;
						// advc.004z: The above doesn't work for air units
						pUnit->canCombat();
				bool bEnemy = pUnit->isEnemy(getTeam());
				bool bOnOurTeam = pUnit->getTeam() == getTeam();
				bool bOfInterest = false;

				switch (eOption)
				{
				case SHOW_ALL_MILITARY:
				{
					bOfInterest = bMilitary;
					if (bOfInterest)
					{
						fPlotStrength += pUnit->currHitPoints() /
								(float)(pUnit->maxHitPoints() *
								pUnit->baseCombatStr());
					}
					break;
				}
				case SHOW_TEAM_MILITARY:
				{
					bOfInterest = bMilitary && bOnOurTeam;
					if (bOfInterest)
					{
						fPlotStrength += pUnit->currHitPoints() /
								(float)(pUnit->maxHitPoints() *
								pUnit->baseCombatStr());
					}
					break;
				}
				case SHOW_ENEMIES:
				{
					bOfInterest = bMilitary && bEnemy;
					if (bOfInterest)
					{
						fPlotStrength += pUnit->currHitPoints() /
								(float)(pUnit->maxHitPoints() *
								pUnit->baseCombatStr());
					}
					break;
				}
				case SHOW_ENEMIES_IN_TERRITORY:
				{
					bOfInterest = bMilitary;
					break;
				}
				case SHOW_PLAYER_DOMESTICS:
				{
					bOfInterest = !bMilitary;// && pUnit->getVisualOwner() == eCurPlayer;
					break;
				}
				default:
					bOfInterest = false;
					break;
				}
				// create the indicator
				if (bOfInterest)
					bShowIndicator = true;

				fMaxPlotStrength = std::max(fPlotStrength, fMaxPlotStrength);
				aafPlayerPlotStrength[eLoopPlayer][iPlot] = fPlotStrength;
			}

			if (bShowIndicator)
			{	// <advc.004z> Don't show a defender when we need !bMilitary
				CvUnit* pUnit = NULL;
				if (eOption == SHOW_PLAYER_DOMESTICS)
				{
					UnitAITypes aePriorityList[] =
					{
						UNITAI_GENERAL, UNITAI_ARTIST, UNITAI_ENGINEER,
						UNITAI_MERCHANT, UNITAI_GREAT_SPY, UNITAI_PROPHET,
						UNITAI_SCIENTIST, UNITAI_SETTLE, UNITAI_SPY,
						UNITAI_MISSIONARY, UNITAI_WORKER, UNITAI_WORKER_SEA
					};
					PlayerTypes eActivePlayer = GC.getGame().getActivePlayer();
					TeamTypes eActiveTeam = GC.getGame().getActiveTeam();
					for (int j = 0; j < ARRAY_LENGTH(aePriorityList); j++)
					{
						// Use unit owner as tiebreaker
						for (int k = 0; k < 3; k++)
						{
							pUnit = kPlot.plotCheck(PUF_isUnitAIType,
									aePriorityList[j], -1,
									(k == 0 ? eActivePlayer : NO_PLAYER),
									(k == 1 ? eActiveTeam : NO_TEAM),
									PUF_isVisibleDebug, eActivePlayer, -1);
							if(pUnit != NULL)
								break;
						}
						if (pUnit != NULL)
							break;
					}
					if (pUnit == NULL)
					{
						pUnit = kPlot.plotCheck(PUF_cannotDefend,
								-1, -1, NO_PLAYER, NO_TEAM,
								PUF_isVisibleDebug, eActivePlayer, -1);
					}
				}
				else // </advc.004z>
					pUnit = kPlot.getBestDefender(NO_PLAYER);
				if (pUnit != NULL)
				{
					PlayerColorTypes eUnitColor = GET_PLAYER(pUnit->getVisualOwner()).
							getPlayerColor();
					const NiColorA& kColor = GC.getInfo(GC.getInfo(eUnitColor).
							getColorTypePrimary()).getColor();

					szBuffer.clear();
					GAMETEXT.setPlotListHelp(szBuffer, kPlot, true, true,
							true); // advc.061, advc.007

					CvPlotIndicatorData kIndicator;
					kIndicator.m_pUnit = pUnit;
					kIndicator.m_szLabel = "UNITS";
					kIndicator.m_szIcon = pUnit->getButton();

					if (eOption == SHOW_ENEMIES_IN_TERRITORY)
					{
						kIndicator.m_kColor.r = 1;
						/*kIndicator.m_kColor.r = 0;
						kIndicator.m_kColor.r = 0;*/
						// <advc.001> Replacing the above
						kIndicator.m_kColor.g = 0;
						kIndicator.m_kColor.b = 0; // </advc.001>
					}
					else
					{
						kIndicator.m_kColor.r = kColor.r;
						kIndicator.m_kColor.g = kColor.g;
						kIndicator.m_kColor.b = kColor.b;
					}
					kIndicator.m_strHelpText = szBuffer.getCString();

					//setup visibility
					switch (eOption)
					{
					case SHOW_ENEMIES_IN_TERRITORY:
						kIndicator.m_bTestEnemyVisibility = true;
						kIndicator.m_eVisibility = PLOT_INDICATOR_VISIBLE_ALWAYS;
						break;
					case SHOW_ENEMIES:
						kIndicator.m_eVisibility = PLOT_INDICATOR_VISIBLE_ALWAYS;
						break;
					default:
						kIndicator.m_eVisibility = PLOT_INDICATOR_VISIBLE_ONSCREEN_ONLY;
					}
					aIndicators.push_back(kIndicator);
				}
			}
		}
	}

	if (fMaxPlotStrength > 0)
	{
		for (PlayerIter<ALIVE> it; it.hasNext(); ++it)
		{
			CvPlayer const& kUnitOwner = *it;
			NiColorA const& kOwnerColor = GC.getInfo(GC.getInfo(
					kUnitOwner.getPlayerColor()).getColorTypePrimary()).getColor();
			for (int i = 0; i < kMap.numPlots(); i++)
			{
				CvPlot const& kPlot = kMap.getPlotByIndex(i);
				if (kPlot.isVisible(getTeam(), true))
				{
					float fPlotStrength = aafPlayerPlotStrength[kUnitOwner.getID()][i];
					if (fPlotStrength > 0)
					{
						float fAlpha = (fPlotStrength / fMaxPlotStrength * 0.75f + 0.25f) * 0.8f;
						if (fAlpha > aColors[i].a)
						{
							aColors[i] = kOwnerColor;
							aColors[i].a = fAlpha;
						}
					}
				}
			}
		}
	}
}

// advc.004z:
bool CvPlayer::showGoodyOnResourceLayer() const
{	/*	Let's let the player decide whether they want to show hut indicators
		alongside action recommendations */
	return (/*isOption(PLAYEROPTION_NO_UNIT_RECOMMENDATIONS) &&*/
			BUGOption::isEnabled("MainInterface__TribalVillageIcons", true));
}

// Used by Globeview resource layer
void CvPlayer::getResourceLayerColors(GlobeLayerResourceOptionTypes eOption,
	std::vector<NiColorA>& aColors, std::vector<CvPlotIndicatorData>& aIndicators) const
{
	aColors.clear();
	aIndicators.clear();

	CvWStringBuffer szBuffer;
	CvMap const& kMap = GC.getMap();
	for (int i = 0; i < kMap.numPlots(); i++)
	{
		CvPlot const& kPlot = kMap.getPlotByIndex(i);
		if(!kPlot.isRevealed(getTeam(), true))
			continue;
		BonusTypes eLoopBonus = kPlot.getBonusType(
				(GC.getGame().isDebugMode()) ? NO_TEAM : getTeam());
		bool bOfInterest = false; // advc.004z
		if (eLoopBonus != NO_BONUS)
		{
			CvBonusInfo& kBonusInfo = GC.getInfo(eLoopBonus);
			switch (eOption)
			{
			case SHOW_ALL_RESOURCES:
				bOfInterest = true;
				break;
			case SHOW_STRATEGIC_RESOURCES:
				bOfInterest = (kBonusInfo.getHappiness() == 0 && kBonusInfo.getHealth() == 0);
				break;
			case SHOW_HAPPY_RESOURCES:
				bOfInterest = (kBonusInfo.getHappiness() != 0 && kBonusInfo.getHealth() == 0);
				break;
			case SHOW_HEALTH_RESOURCES:
				bOfInterest = (kBonusInfo.getHappiness() == 0 && kBonusInfo.getHealth() != 0);
				break;
			}
		} // <advc.004z>
		ImprovementTypes eImpr = NO_IMPROVEMENT;
		if(!bOfInterest && eOption == SHOW_ALL_RESOURCES &&
			showGoodyOnResourceLayer())
		{
			eImpr = kPlot.getRevealedImprovementType(getTeam());
			bOfInterest = (eImpr != NO_IMPROVEMENT && GC.getInfo(eImpr).
					isGoody());
		} // </advc.004z>
		if (bOfInterest)
		{
			CvPlotIndicatorData kData;
			kData.m_szLabel = "RESOURCES";
			kData.m_eVisibility = PLOT_INDICATOR_VISIBLE_ONSCREEN_ONLY;
			kData.m_szIcon = // <advc.004z>
					(eLoopBonus == NO_BONUS ? GC.getInfo(eImpr).
					getButton() // </advc.004z>
					: GC.getInfo(eLoopBonus).getButton());

			int x = kPlot.getX();
			int y = kPlot.getY();
			kData.m_Target = NiPoint2(kMap.plotXToPointX(x), kMap.plotYToPointY(y));
			PlayerTypes eOwner = kPlot.getRevealedOwner(getTeam(), true);
			if (eOwner == NO_PLAYER)
			{
				kData.m_kColor.r = 0.8f;
				kData.m_kColor.g = 0.8f;
				kData.m_kColor.b = 0.8f;
			}
			else
			{
				PlayerColorTypes eCurPlayerColor = GET_PLAYER(eOwner).getKnownPlayerColor();
				const NiColorA& kColor = GC.getInfo(GC.getInfo(eCurPlayerColor).
						getColorTypePrimary()).getColor();
				kData.m_kColor.r = kColor.r;
				kData.m_kColor.g = kColor.g;
				kData.m_kColor.b = kColor.b;
			}

			szBuffer.clear();
			// <advc.004z>
			if(eLoopBonus == NO_BONUS)
				GAMETEXT.setImprovementHelp(szBuffer, eImpr);
			else // </advc.004z>
			{
				//GAMETEXT.setBonusHelp(szBuffer, eCurType, false);
				// <advc.003p> Replacing the above
				if(m_aszBonusHelp[eLoopBonus] != NULL)
					szBuffer.append(*m_aszBonusHelp[eLoopBonus]);
				else
				{
					CvWStringBuffer szTempBuffer;
					GAMETEXT.setBonusHelp(szTempBuffer, eLoopBonus, false);
					m_aszBonusHelp[eLoopBonus] = new CvWString(szTempBuffer.getCString());
					szBuffer.append(szTempBuffer);
				}
			} // </advc.003p>
			kData.m_strHelpText = szBuffer.getCString();

			aIndicators.push_back(kData);
		}
	}
}

// Used by Globeview religion layer
void CvPlayer::getReligionLayerColors(ReligionTypes eSelectedReligion,
	std::vector<NiColorA>& aColors, std::vector<CvPlotIndicatorData>& aIndicators) const
{
	aColors.resize(GC.getMap().numPlots(), NiColorA(0, 0, 0, 0));
	aIndicators.clear();

	CvRandom kRandom;
	kRandom.init(42 * eSelectedReligion);
	const NiColorA kBaseColor(kRandom.getFloat(), kRandom.getFloat(), kRandom.getFloat(), 1.0f);

	for (PlayerIter<ALIVE> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		FOR_EACH_CITY(pLoopCity, *itPlayer)
		{
			if (!pLoopCity->isRevealed(getTeam(), true) ||
				!pLoopCity->isHasReligion(eSelectedReligion))
			{
				continue;
			}
			float fAlpha = 0.8f;
			if (!pLoopCity->isHolyCity(eSelectedReligion))
				fAlpha *= 0.5f;

			for (CityPlotIter it(*pLoopCity); it.hasNext(); ++it)
			{
				if (!it->isRevealed(getTeam(), true))
					continue;
				int iIndex = GC.getMap().plotNum(*it);
				if (fAlpha > aColors[iIndex].a)
				{
					aColors[iIndex] = kBaseColor;
					aColors[iIndex].a = fAlpha;
				}
			}
		}
	}
}

// Used by Globeview culture layer
void CvPlayer::getCultureLayerColors(std::vector<NiColorA>& aColors,
	std::vector<CvPlotIndicatorData>& aIndicators) const
{	/*  advc.004z (comment): Can increase this for more precise color proportions,
		but the color pattern used by the EXE makes higher values (e.g. 10) look strange. */
	const int iColorsPerPlot = 4;
	CvMap const& kMap = GC.getMap();
	aColors.resize(kMap.numPlots() * iColorsPerPlot, NiColorA(0, 0, 0, 0));
	aIndicators.clear();

	// find maximum total culture
	int iMaxTotalCulture = MIN_INT;
	int iMinTotalCulture = MAX_INT;
	for (int i = 0; i < kMap.numPlots(); i++)
	{
		CvPlot const& kLoopPlot = kMap.getPlotByIndex(i);
		// <advc.004z>
		if(!kLoopPlot.isVisible(getTeam(), true))
			continue; // </advc.004z>
		int iTotalCulture = kLoopPlot.getTotalCulture(); // advc.opt: was countTotalCulture
		if (iTotalCulture > iMaxTotalCulture)
			iMaxTotalCulture = iTotalCulture;
		if (iTotalCulture < iMinTotalCulture && iTotalCulture > 0)
			iMinTotalCulture = iTotalCulture;
	}
	iMinTotalCulture = 0;

	// find culture percentages
	for (int iPlot = 0; iPlot < kMap.numPlots(); iPlot++)
	{
		CvPlot const& kLoopPlot = kMap.getPlotByIndex(iPlot);
		PlayerTypes eOwner = kLoopPlot.getRevealedOwner(getTeam(), true);
		if(eOwner == NO_PLAYER)
			continue; // advc: Moved up
		// how many people own this plot?
		std::vector<std::pair<int,PlayerTypes> > aieOwners;
		//int iNumNonzeroOwners = 0;
		// K-Mod
		int iTotalCulture = kLoopPlot.getTotalCulture(); // advc.opt: was countTotalCulture
		if (iTotalCulture <= 0)
			continue;
		// K-Mod end
		// <advc.004z>
		aieOwners.push_back(std::make_pair(kLoopPlot.getCulture(eOwner), eOwner));
		bool bVisible = kLoopPlot.isVisible(getTeam(), true);
		if (bVisible) // </advc.004z>
		{
			// kekm.21: include Barbarians; advc.099: include defeated.
			for (PlayerIter<EVER_ALIVE> it; it.hasNext(); ++it)
			{
				PlayerTypes ePlayer = it->getID();
				// <advc.004z> Owner handled above
				if(ePlayer == eOwner)
					continue; // </advc.004z>
				int iCurCultureAmount = kLoopPlot.getCulture(ePlayer);
				//if (iCurCultureAmount != 0)
				// K-Mod (to reduce visual spam from small amounts of culture)
				if (100 * iCurCultureAmount >= 20 * iTotalCulture)
				{	//iNumNonzeroOwners ++;
					aieOwners.push_back(std::make_pair(iCurCultureAmount, ePlayer));
				}
			}
		}
		//if (!plot_owners.empty())
		/*  <advc.004z> Give players with a high percentage a bigger share of
			the colored area. */
		EnumMap<PlayerTypes,bool> abDone;
		for (int iPass = 0; iPass < 2 &&
			// Try to fill plot_owners up
			aieOwners.size() < iColorsPerPlot; iPass++)
		{
			// To avoid adding to plot_owners while looping through it
			std::vector<std::pair<int,PlayerTypes> > aieRepeatedOwners;
			for(size_t i = 0; i < aieOwners.size(); i++)
			{
				if (abDone.get(aieOwners[i].second))
					continue; // Skip iPass==1 if iExtra added in iPass==0
				int iExtra = -1; // Already once in plot_owners
				if (iPass == 0) // Round down
					iExtra += (aieOwners[i].first * iColorsPerPlot) / iTotalCulture;
				else if (iPass == 1) // Round to nearest
				{
					iExtra += intdiv::round(aieOwners[i].first * iColorsPerPlot,
							iTotalCulture);
				} // Respect size limit
				iExtra = std::min(iExtra, iColorsPerPlot - (int)
						(aieOwners.size() + aieRepeatedOwners.size()));
				for (int j = 0; j < iExtra; j++)
				{
					aieRepeatedOwners.push_back(aieOwners[i]);
					abDone.set(aieOwners[i].second, true);
				}
			}
			/*  The more often a civ appears in plot_owners, the more pixels
				will be set to the civ's color. */  // advc: use insert
			aieOwners.insert(aieOwners.end(),
					aieRepeatedOwners.begin(), aieRepeatedOwners.end());
		}
		/*	Ideally ==iColorsPerPlot, but my algorithm above can't guarantee that.
			Can be 1 greater than iColorsPerPlot because the territorial owner is
			always included. */
		FAssert(aieOwners.size() <= iColorsPerPlot + 1);
		// </advc.004z>
		for (int i = 0;
			// adv.004z: max (see comment above)
			i < std::max(iColorsPerPlot, (int)aieOwners.size()); i++)
		{
			int const iCurOwnerIdx = i % aieOwners.size();
			PlayerTypes const eCurOwnerID = aieOwners[iCurOwnerIdx].second;
			int const iCurCulture = aieOwners[iCurOwnerIdx].first;
			NiColorA const& kCurColor = GC.getInfo(GC.getInfo(GET_PLAYER(eCurOwnerID).
					getKnownPlayerColor()).getColorTypePrimary()).getColor();
			// damp the color by the value...
			aColors[iPlot * iColorsPerPlot + i] = kCurColor;
			/*  <advc.004z> Don't give away info about fogged tiles.
				Use a low factor b/c fogged tiles already look darker than
				visible tiles. */
			float fBlendFactor = 0.1f;
			if (bVisible) // </advc.004z>
				fBlendFactor = (iCurCulture - iMinTotalCulture) / (float)iMaxTotalCulture;
			fBlendFactor = 0.5f * std::min(1.0f, std::max(//0.0f,
					0.1f, // advc.004z
					fBlendFactor));
			aColors[iPlot * iColorsPerPlot + i].a = std::min(
					// advc.004z: Coefficient before fBlendFactor was 0.8
					0.75f * fBlendFactor + 0.5f, 1.0f);
		}
	}
}

// advc.003p:
void CvPlayer::setBonusHelpDirty()
{
	if(m_aszBonusHelp == NULL)
	{
		FAssert(m_aszBonusHelp != NULL);
		return;
	}
	FOR_EACH_ENUM(Bonus)
		SAFE_DELETE(m_aszBonusHelp[eLoopBonus])
}


void CvPlayer::cheat(bool bCtrl, bool bAlt, bool bShift)
{
	//if (gDLL->getChtLvl() > 0)
	if(GC.getGame().isDebugMode()) // advc.007b
		GET_TEAM(getTeam()).setHasTech(getCurrentResearch(), true, getID(), true, false);
}


const CvArtInfoUnit* CvPlayer::getUnitArtInfo(UnitTypes eUnit, int iMeshGroup) const
{
	CivilizationTypes eCivilization = getCivilizationType();
	if (eCivilization == NO_CIVILIZATION)
		eCivilization = (CivilizationTypes)GC.getDefineINT("BARBARIAN_CIVILIZATION");
	// <advc.001> Redirect the call to the city owner
	if(gDLL->UI().isCityScreenUp())
	{
		PlayerTypes eCityOwner = gDLL->UI().getHeadSelectedCity()->getOwner();
		if(eCityOwner != getID())
			return GET_PLAYER(eCityOwner).getUnitArtInfo(eUnit, iMeshGroup);
	} // </advc.001>
	UnitArtStyleTypes eStyle = (UnitArtStyleTypes) GC.getInfo(eCivilization).getUnitArtStyleType();
	EraTypes eEra = getCurrentEra();
	if (eEra == NO_ERA)
		eEra = (EraTypes)0;
	return GC.getInfo(eUnit).getArtInfo(iMeshGroup, eEra, eStyle);
}

// K-Mod. I've moved the original code to a new function: CvTeam::hasSpaceshipArrived
bool CvPlayer::hasSpaceshipArrived() const
{
	return GET_TEAM(getTeam()).hasSpaceshipArrived();
}

// advc.135c: (see call location CvInitCore::setGameName)
void CvPlayer::announceGameNameChange(CvWString szOldName, CvWString szNewName)
{
	gDLL->UI().addMessage(getID(), true, -1,
			gDLL->getText("TXT_KEY_GAME_NAME_CHANGED",
			szOldName.GetCString(), szNewName.GetCString()),
			NULL, MESSAGE_TYPE_MINOR_EVENT);
	if (GC.getGame().isGameNameEnableDebugTools(szNewName))
	{
		gDLL->UI().addMessage(getID(), true, -1,
				gDLL->getText("TXT_KEY_DEBUG_TOOLS_ENABLED"),
				NULL, MESSAGE_TYPE_MAJOR_EVENT);
	}
}

// advc.210:
void CvPlayer::checkAlert(int iAlertID, bool bSilent)
{
	if (m_paAlerts.empty())
	{
		FErrorMsg("Alerts not initialized for this player");
		return;
	}
	if(iAlertID < 0 || iAlertID > (int)m_paAlerts.size())
	{
		FErrorMsg("Invalid alert");
		return;
	}
	m_paAlerts[iAlertID]->check(bSilent);
}

/*	advc.104: Inspired by CvTeamAI::AI_estimateTotalYieldRate
	(Tbd.: Move to CvPlayerAI) */
double CvPlayer::estimateYieldRate(YieldTypes eYield, int iSamples) const
{
	//PROFILE_FUNC(); // Called very frequently; about 1.5% of the turn times (July 2019).
	CvGame const& kGame = GC.getGame();
	int const iGameTurn = kGame.getGameTurn();
	int const iTurnsPlayed = iGameTurn - kGame.getStartTurn();
	iSamples = std::max(0, std::min(iSamples, iTurnsPlayed - 1));
	std::vector<double> adSamples; // double for ::dMedian
	adSamples.reserve(iSamples);
	/* When anarchy lasts several turns, the sample may not contain a single
	   non-revolution turn. In this case, increase the sample size gradually. */
	while (adSamples.empty() && iSamples < iTurnsPlayed)
	{
		for (int i = 1; i <= iSamples; i++)
		{
			int iSampleIndex = iGameTurn - i;
			int iHist = 0;
			switch (eYield)
			{
			case YIELD_COMMERCE:
				iHist = getHistory(PLAYER_HISTORY_ECONOMY, iSampleIndex);
				break;
			case YIELD_PRODUCTION:
				iHist = getHistory(PLAYER_HISTORY_INDUSTRY, iSampleIndex);
				break;
			case YIELD_FOOD:
				iHist = getHistory(PLAYER_HISTORY_AGRICULTURE, iSampleIndex);
				break;
			default: FAssert(false);
			}
			if (iHist > 0) // Omit revolution turns
				adSamples.push_back(iHist);
		}
		iSamples++;
	}
	if (adSamples.empty())
		return 0;
	return ::dMedian(adSamples);
}

// <advc.004x>
void CvPlayer::killAll(ButtonPopupTypes ePopupType, int iData1)
{
	CvGame const& kGame = GC.getGame();
	if (getID() != kGame.getActivePlayer() || !isHuman() ||
		/*	Non-minimized popups don't usually become outdated, but
			it can happen when multiple popups trigger at the same time and
			the handler for one popup affects game state relevant for another.
			E.g. when a tech is discovered that enables a civic (revolution via
			change-civic popup will invalidate the choose-next-tech popup). */
		//!isOption(PLAYEROPTION_MINIMIZE_POP_UPS) ||
		/*	I can't get this to work in network games. The delays introduced by
			net messages cause popups to appear several times. */
		kGame.isNetworkMultiPlayer())
	{
		return;
	}
	/*	Not sure about this. Perhaps it should essentially be 0, but can be 1
		due to the EXE/DLL/async call sequence, or something. */
	FAssert(m_iButtonPopupsRelaunching <= 1);
	CvPopupQueue relaunchDisplayed;
	CvPopupQueue relaunchQueued;
	int iOnDisplay = 0;
	for (int iPass = 0; iPass < 2; iPass++)
	{
		if (iPass == 1)
		{
			// Recall popups already launched
			gDLL->UI().getDisplayedButtonPopups(m_listPopups);
			iOnDisplay = m_listPopups.size();
		}
		for (std::list<CvPopupInfo*>::iterator it = m_listPopups.begin();
			it != m_listPopups.end(); ++it)
		{
			CvPopupInfo* pPopup = *it;
			if ((pPopup->getButtonPopupType() != ePopupType &&
				/*	Don't relaunch a found-religion popup in response to a
					change-religion popup. The player will already have chosen
					and founded a religion, i.e. the found-religion popup is
					essentially already done. */
				(ePopupType != BUTTONPOPUP_CHANGERELIGION || iPass < 1 ||
				pPopup->getButtonPopupType() != BUTTONPOPUP_FOUND_RELIGION) &&
				// Doesn't get minimized, never needs to be relaunched.
				(iPass < 1 || pPopup->getButtonPopupType() != BUTTONPOPUP_DECLAREWARMOVE))
				||
				(iData1 >= 0 && pPopup->getData1() != iData1))
			{
				(iPass == 0 ? relaunchQueued : relaunchDisplayed).push_back(pPopup);
			}
			else if (iPass <= 0)
			{
				SAFE_DELETE(pPopup);
			} /* Otherwise, the EXE owns it. The clearQueuedPopups call below will
				 delete it (I assume). */
		}
		m_listPopups.clear();
	}

	// Relaunch popups already on display only when necessary
	if (iOnDisplay != relaunchDisplayed.size())
	{
		gDLL->UI().clearQueuedPopups();
		// To suppress the sound
		m_iButtonPopupsRelaunching = relaunchDisplayed.size();
		m_listPopups.insert(m_listPopups.begin(),
				relaunchDisplayed.begin(), relaunchDisplayed.end());
	}
	m_listPopups.insert(m_listPopups.end(),
			relaunchQueued.begin(), relaunchQueued.end());
	/*	Note: The EXE will fetch the CvPopupInfo in m_listPopups,
		create CvPopup objects (which the DLL can't do) and
		launch them through CvDLLButtonPopup::launchButtonPopup. */
}

/*	Wrapper for CvDLLInterfaceIFaceBase::playGeneralSound that suppresses
	sounds upon relaunching (i.e. merely updating) a popup */
void CvPlayer::playButtonPopupSound(LPCTSTR pszSound) const
{
	if (m_iButtonPopupsRelaunching <= 0)
		gDLL->UI().playGeneralSound(pszSound);
}


void CvPlayer::reportButtonPopupLaunched()
{
	m_iButtonPopupsRelaunching = std::max(m_iButtonPopupsRelaunching - 1, 0);
} // </advc.004x>

// <advc.314>
// iProgress <= 0 means guaranteed discovery
bool CvPlayer::isGoodyTech(TechTypes eTech, int iProgress) const
{
	CvTechInfo const& kTech = GC.getInfo(eTech);
	if (!kTech.isGoodyTech())
	{
		if (iProgress <= 0)
			return false;
		if (kTech.getEra() == NO_ERA ||
			!GC.getInfo(kTech.getEra()).get(CvEraInfo::AllGoodyTechs))
		{
			return false;
		}
	}
	if (!isFoundedFirstCity() && // Can't receive a holy city (or corp. HQ) then
		// OK if the tech won't be discovered right away
		(iProgress <= 0 || iProgress > fixp(2/3.) *
		GET_TEAM(getTeam()).getResearchLeft(eTech)))
	{
		CvGame const& kGame = GC.getGame();
		FOR_EACH_ENUM(Religion)
		{
			if (!kGame.isReligionFounded(eLoopReligion) &&
				GC.getInfo(eLoopReligion).getTechPrereq() == eTech)
			{
				return false;
			}
		}
		FOR_EACH_ENUM(Corporation)
		{
			if (!kGame.isCorporationFounded(eLoopCorporation) &&
				GC.getInfo(eLoopCorporation).getTechPrereq() == eTech)
			{
				return false;
			}
		}
	}
	return canResearch(eTech, false, true);
}


void CvPlayer::addGoodyMsg(CvWString s, CvPlot const& p, TCHAR const* sound)
{
	gDLL->UI().addMessage(
			getID(), true, -1, s, sound, MESSAGE_TYPE_MINOR_EVENT,
			ARTFILEMGR.getImprovementArtInfo("ART_DEF_IMPROVEMENT_GOODY_HUT")->getButton(),
			NO_COLOR, p.getX(), p.getY());
}


void CvPlayer::promoteFreeUnit(CvUnit& u, scaled pr)
{
	FeatureTypes eDefFeature = NO_FEATURE;
	FOR_EACH_ENUM(Feature)
	{
		if (GC.getInfo(eLoopFeature).getDefenseModifier() +
		   GC.getInfo(eLoopFeature).getRivalDefenseModifier() > 0)
		{
			eDefFeature = eLoopFeature;
			break;
		}
	}
	for (int i = 0; i < 2; i++)
	{
		if (!pr.bernoulliSuccess(GC.getGame().getSRand(), "advc.314"))
			break;
		int iBestValue = -1;
		PromotionTypes eBestPromo = NO_PROMOTION;
		PromotionTypes ePrevPromo = NO_PROMOTION;
		FOR_EACH_ENUM2(Promotion, ePromo)
		{
			CvPromotionInfo& kPromo = GC.getInfo(ePromo);
			int iUnitCombat = 0;
			FOR_EACH_ENUM(UnitCombat)
			{
				if(kPromo.getUnitCombat(eLoopUnitCombat))
					iUnitCombat++;
			}
			if ((kPromo.getCombatPercent() <= 0 || /* No Combat2 */ ePromo > 0) &&
				(eDefFeature == NO_FEATURE ||
				kPromo.getFeatureDefensePercent(eDefFeature) <= 0) &&
				kPromo.getHillsDefensePercent() <= 0 &&
				/*  This is the Cover promotion. Only that one, Woodsman,
					Guerilla and City Raider have exactly 3 eligible unit
					combat classes. */
				(iUnitCombat != 3 || kPromo.getCityAttackPercent() > 0))
			{
				continue;
			}
			// Second promo needs to build on the first
			/*if(ePrevPromo != NO_PROMOTION &&
					kPromo.getPrereqPromotion() != ePrevPromo &&
					kPromo.getPrereqOrPromotion1() != ePrevPromo &&
					kPromo.getPrereqOrPromotion2() != ePrevPromo &&
					kPromo.getPrereqOrPromotion2() != ePrevPromo)
				continue;*/
			if (!u.canAcquirePromotion(ePromo))
				continue;
			int iValue = GC.getGame().getSorenRandNum(100, "advc.314");
			std::vector<CvPlot const*> apSurroundings;
			apSurroundings.push_back(u.plot());
			FOR_EACH_ADJ_PLOT(u.getPlot())
			{
				if (!pAdj->isWater())
					apSurroundings.push_back(pAdj);
			}
			for(size_t k = 0; k < apSurroundings.size(); k++)
			{
				int iTmpVal = 0;
				if (eDefFeature != NO_FEATURE &&
					apSurroundings[k]->getFeatureType() == eDefFeature)
				{
					iTmpVal += kPromo.getFeatureDefensePercent(eDefFeature);
				}
				// Encourage non-Woodsman then
				else iTmpVal -= kPromo.getFeatureDefensePercent(eDefFeature) / 3;
				if (apSurroundings[k]->isHills())
					iTmpVal += kPromo.getHillsDefensePercent();
				else iTmpVal -= kPromo.getHillsDefensePercent() / 3;
				if (k == 0) // Higher weight for pPlot
					iTmpVal *= 3;
				iValue = std::max(0, iValue + iTmpVal);
			}
			if(iValue > iBestValue)
			{
				iBestValue = iValue;
				eBestPromo = ePromo;
			}
		}
		if (eBestPromo == NO_PROMOTION)
			break;
		u.setHasPromotion(eBestPromo, true);
		ePrevPromo = eBestPromo;
	}
} // </advc.314>

// advc.120f:
void CvPlayer::announceEspionageToThirdParties(EspionageMissionTypes eMission,
	PlayerTypes eTarget)
{
	bool const bReligion = (GC.getInfo(eMission).
			getSwitchReligionCostFactor() > 0);
	CvWString szBuffer = gDLL->getText((bReligion ?
			"TXT_KEY_ESPIONAGE_3RD_PARTY_SWITCH_RELIGION" :
			"TXT_KEY_ESPIONAGE_3RD_PARTY_SWITCH_CIVIC"),
			GET_PLAYER(eTarget).getCivilizationAdjectiveKey()).GetCString();
	int iX = -1, iY = -1;
	{
		CvCity* pCapital = GET_PLAYER(eTarget).getCapital();
		if (pCapital != NULL)
		{
			iX = pCapital->getX();
			iY = pCapital->getY();
		}
	}
	if (GC.getDefineBOOL("ANNOUNCE_ESPIONAGE_REVOLUTION"))
	{
		for (PlayerIter<MAJOR_CIV,OTHER_KNOWN_TO> it(TEAMID(eTarget)); it.hasNext(); ++it)
		{
			CvPlayer const& kObs = *it;
			if (kObs.getID() != getID())
			{
				gDLL->UI().addMessage(kObs.getID(), false, -1, szBuffer,
						NULL, MESSAGE_TYPE_INFO, NULL, NO_COLOR, iX, iY);
			}
		}
	}
	if (bReligion)
	{
		CvWString szTmp = gDLL->getText("TXT_KEY_ESPIONAGE_REVEAL_OWNER",
				getCivilizationAdjectiveKey()).GetCString();
		szTmp += L" " + szBuffer;
		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, eTarget, szTmp);
	}
}

// <advc.opt> Global; see CvPlayer.h.
CvCity* getCityExternal(IDInfo city)
{
	if (city.eOwner == NO_PLAYER)
		return NULL;
	return getCity(city);
}

CvUnit* getUnitExternal(IDInfo unit)
{
	if (unit.eOwner == NO_PLAYER)
		return NULL;
	return getUnit(unit);
} // </advc.opt>
