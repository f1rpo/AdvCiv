// player.cpp

#include "CvGameCoreDLL.h"
#include "CvPlayer.h"
#include "CvGamePlay.h"
#include "WarAndPeaceAgent.h" // advc.104
#include "CvMap.h"
#include "CvArtFileMgr.h"
#include "CvDiploParameters.h"
#include "CvInitCore.h"
#include "CyArgsList.h"
#include "CvInfos.h"
#include "CvPopupInfo.h"
#include "CvDiploParameters.h"
#include "CvGameTextMgr.h"
#include "RiseFall.h" // advc.700
#include "AdvCiv4lerts.h" // advc.210
#include "CyCity.h"
#include "CyPlot.h"
#include "CyUnit.h"
#include "CvEventReporter.h"
#include "CvBugOptions.h" // advc.106b
#include "CvDLLInterfaceIFaceBase.h"
#include "CvDLLEntityIFaceBase.h"
#include "CvDLLEngineIFaceBase.h"
#include "CvDLLFAStarIFaceBase.h"
#include "CvDLLPythonIFaceBase.h"
//bbai
#include "CvDLLFlagEntityIFaceBase.h"
#include "BBAILog.h"
//bbai end


// Public Functions...

CvPlayer::CvPlayer()
{
	m_aiSeaPlotYield = new int[NUM_YIELD_TYPES];
	m_aiYieldRateModifier = new int[NUM_YIELD_TYPES];
	m_aiCapitalYieldRateModifier = new int[NUM_YIELD_TYPES];
	m_aiExtraYieldThreshold = new int[NUM_YIELD_TYPES];
	m_aiTradeYieldModifier = new int[NUM_YIELD_TYPES];
	m_aiFreeCityCommerce = new int[NUM_COMMERCE_TYPES];
	m_aiCommercePercent = new int[NUM_COMMERCE_TYPES];
	m_aiCommerceRate = new int[NUM_COMMERCE_TYPES];
	m_aiCommerceRateModifier = new int[NUM_COMMERCE_TYPES];
	m_aiCapitalCommerceRateModifier = new int[NUM_COMMERCE_TYPES];
	m_aiStateReligionBuildingCommerce = new int[NUM_COMMERCE_TYPES];
	m_aiSpecialistExtraCommerce = new int[NUM_COMMERCE_TYPES];
	m_aiCommerceFlexibleCount = new int[NUM_COMMERCE_TYPES];
	m_aiGoldPerTurnByPlayer = new int[MAX_PLAYERS];
	m_aiEspionageSpendingWeightAgainstTeam = new int[MAX_TEAMS];

	m_abFeatAccomplished = new bool[NUM_FEAT_TYPES];
	m_abOptions = new bool[NUM_PLAYEROPTION_TYPES];

	m_paiBonusExport = NULL;
	m_paiBonusImport = NULL;
	m_paiImprovementCount = NULL;
	m_paiFreeBuildingCount = NULL;
	m_paiExtraBuildingHappiness = NULL;
	m_paiExtraBuildingHealth = NULL;
	m_paiFeatureHappiness = NULL;
	m_paiUnitClassCount = NULL;
	m_paiUnitClassMaking = NULL;
	m_paiBuildingClassCount = NULL;
	m_paiBuildingClassMaking = NULL;
	m_paiHurryCount = NULL;
	m_paiSpecialBuildingNotRequiredCount = NULL;
	m_paiHasCivicOptionCount = NULL;
	m_paiNoCivicUpkeepCount = NULL;
	m_paiHasReligionCount = NULL;
	m_paiHasCorporationCount = NULL;
	m_paiUpkeepCount = NULL;
	m_paiSpecialistValidCount = NULL;

	m_pabResearchingTech = NULL;
	m_pabLoyalMember = NULL;

	m_paeCivics = NULL;

	m_ppaaiSpecialistExtraYield = NULL;
	m_ppaaiImprovementYieldChange = NULL;

	m_aszBonusHelp = NULL; // advc.003p
	/*  advc.210: The order of this array needs to correspond to the ids returned
		by the AdvCiv4lers.getID functions in Civ4lerts.py */
	m_paAlerts.push_back(new WarTradeAlert()); // advc.210a
	m_paAlerts.push_back(new RevoltAlert()); // advc.210b
	m_paAlerts.push_back(new BonusThirdPartiesAlert()); // advc.210d

	m_bDisableHuman = false; // bbai
	m_iChoosingFreeTechCount = 0; // K-Mod

	reset(NO_PLAYER, true);
}


CvPlayer::~CvPlayer()
{
	uninit();

	SAFE_DELETE_ARRAY(m_aiSeaPlotYield);
	SAFE_DELETE_ARRAY(m_aiYieldRateModifier);
	SAFE_DELETE_ARRAY(m_aiCapitalYieldRateModifier);
	SAFE_DELETE_ARRAY(m_aiExtraYieldThreshold);
	SAFE_DELETE_ARRAY(m_aiTradeYieldModifier);
	SAFE_DELETE_ARRAY(m_aiFreeCityCommerce);
	SAFE_DELETE_ARRAY(m_aiCommercePercent);
	SAFE_DELETE_ARRAY(m_aiCommerceRate);
	SAFE_DELETE_ARRAY(m_aiCommerceRateModifier);
	SAFE_DELETE_ARRAY(m_aiCapitalCommerceRateModifier);
	SAFE_DELETE_ARRAY(m_aiStateReligionBuildingCommerce);
	SAFE_DELETE_ARRAY(m_aiSpecialistExtraCommerce);
	SAFE_DELETE_ARRAY(m_aiCommerceFlexibleCount);
	SAFE_DELETE_ARRAY(m_aiGoldPerTurnByPlayer);
	SAFE_DELETE_ARRAY(m_aiEspionageSpendingWeightAgainstTeam);
	SAFE_DELETE_ARRAY(m_abFeatAccomplished);
	SAFE_DELETE_ARRAY(m_abOptions);
	// <advc.210>
	for(size_t i = 0; i < m_paAlerts.size(); i++)
		SAFE_DELETE(m_paAlerts[i]); // </advc.210>
}


void CvPlayer::init(PlayerTypes eID)
{
	//--------------------------------
	// Init saved data
	reset(eID);

	//--------------------------------
	// Init containers
	initContainers(); // advc.003q: Moved into a subroutine for initInGame

	//--------------------------------
	// Init non-saved data
	setupGraphical();

	//--------------------------------
	// Init team data
	FAssert(getTeam() != NO_TEAM);
	GET_TEAM(getTeam()).changeNumMembers(1);
	GET_TEAM(getTeam()).updateMinorCiv(); // advc.003m

	//--------------------------------
	// Init other player data
	// <advc.003q> Moved into a subroutine for initInGame
	if(!initOtherData())
		return; // </advc.003q>

	for (int iI = 0; iI < GC.getNumEventInfos(); iI++)
		resetEventOccured((EventTypes)iI, false);

	for (int iI = 0; iI < GC.getNumEventTriggerInfos(); iI++)
		resetTriggerFired((EventTriggerTypes)iI);

	AI_init();
}

// advc.003q: Cut from CvPlayer::init
void CvPlayer::initContainers() {

	m_plotGroups.init();
	m_cities.init();
	m_units.init();
	m_selectionGroups.init();
	m_eventsTriggered.init();
}

/*  advc.003q: Cut from CvPlayer::init and refactored a bit. Returns false if
	data not initialized due to slot status. */
bool CvPlayer::initOtherData() {

	SlotStatus eStatus = GC.getInitCore().getSlotStatus(getID());
	if (eStatus != SS_TAKEN && eStatus != SS_COMPUTER)
		return false;

	setAlive(true);
	int iI, iJ;
	if (GC.getGame().isOption(GAMEOPTION_RANDOM_PERSONALITIES) &&
			!isBarbarian() && !isMinorCiv()) {
		int iBestValue = 0;
		LeaderHeadTypes eBestPersonality = NO_LEADER;
		for (iI = 0; iI < GC.getNumLeaderHeadInfos(); iI++) {
			if (iI == GC.getDefineINT("BARBARIAN_LEADER")) // XXX minor civ???
				continue;

			int iValue = (1 + GC.getGame().getSorenRandNum(10000, "Choosing Personality"));
			for (iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++) {
				if (GET_PLAYER((PlayerTypes)iJ).isAlive()) {
					if (GET_PLAYER((PlayerTypes)iJ).getPersonalityType() == iI)
						iValue /= 2;
				}
			}
			if (iValue > iBestValue) {
				iBestValue = iValue;
				eBestPersonality = ((LeaderHeadTypes)iI);
			}
		}
		if (eBestPersonality != NO_LEADER)
			setPersonalityType(eBestPersonality);
	}

	changeBaseFreeUnits(GC.getDefineINT("INITIAL_BASE_FREE_UNITS"));
	changeBaseFreeMilitaryUnits(GC.getDefineINT("INITIAL_BASE_FREE_MILITARY_UNITS"));
	changeFreeUnitsPopulationPercent(GC.getDefineINT("INITIAL_FREE_UNITS_POPULATION_PERCENT"));
	changeFreeMilitaryUnitsPopulationPercent(GC.getDefineINT("INITIAL_FREE_MILITARY_UNITS_POPULATION_PERCENT"));
	changeGoldPerUnit(GC.getDefineINT("INITIAL_GOLD_PER_UNIT"));
	changeTradeRoutes(GC.getDefineINT("INITIAL_TRADE_ROUTES"));
	changeStateReligionHappiness(GC.getDefineINT("INITIAL_STATE_RELIGION_HAPPINESS"));
	changeNonStateReligionHappiness(GC.getDefineINT("INITIAL_NON_STATE_RELIGION_HAPPINESS"));

	for (iI = 0; iI < NUM_YIELD_TYPES; iI++) {
		YieldTypes eYield = (YieldTypes)iI;
		changeTradeYieldModifier(eYield, GC.getYieldInfo(eYield).getTradeModifier());
	}

	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++) {
		CommerceTypes eCommerce = (CommerceTypes)iI;
		setCommercePercent(eCommerce, GC.getCommerceInfo(eCommerce).getInitialPercent(), true);
	}

	processTraits(1); // advc.003q: Moved into a (sub-)subroutine
	return true;
}


/*  BETTER_BTS_AI_MOD, 12/30/08, jdog5000: START
	(copy of CvPlayer::init but with modifications for use in the middle of a game) */
void CvPlayer::initInGame(PlayerTypes eID)
{
	int iI, iJ;
	reset(eID);
	initContainers(); // advc.003q: New subroutine to avoid code duplication
	setupGraphical();

	// BBAI: Some effects on team necessary if this is the only member of the team
	int iOtherTeamMembers = 0;
	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (iI != getID())
		{
			if (TEAMID((PlayerTypes)iI) == getTeam())
			{
				iOtherTeamMembers++;
			}
		}
	}
	bool bTeamInit = false;
	if (iOtherTeamMembers == 0 || GET_TEAM(getTeam()).getNumMembers() == 0)
	{
		bTeamInit = true;
		GET_TEAM(getTeam()).init(getTeam());
		GET_TEAM(getTeam()).resetPlotAndCityData();
	}
	if (bTeamInit || (GET_TEAM(getTeam()).getNumMembers() == iOtherTeamMembers))
	{
		GET_TEAM(getTeam()).changeNumMembers(1);
	}
	// <advc.003q>
	// End of BBAI team effects
	if(!initOtherData()) // New subroutine to avoid code duplication
		return;
	/*  I've kept the initialization of random event data out of initOtherData
		b/c the BBAI code handles that part differently: */ // </advc.003q>

	// Reset all triggers at first, set those whose events have fired in next block
	for (iI = 0; iI < GC.getNumEventTriggerInfos(); iI++)
		resetTriggerFired((EventTriggerTypes)iI);

	for (iI = 0; iI < GC.getNumEventInfos(); iI++) {
		/* original bts code
		resetEventOccured((EventTypes)iI, false);*/
		// Has global trigger fired already?
		const EventTriggeredData* pEvent = NULL;
		for (iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++) {
			if (iJ == getID())
				continue; // advc.003

			pEvent = GET_PLAYER((PlayerTypes)iJ).getEventOccured((EventTypes)iI);
			if (pEvent == NULL)
				continue; // advc.003

			CvEventTriggerInfo& kTrigger = GC.getEventTriggerInfo(pEvent->m_eTrigger);
			if (kTrigger.isGlobal()) {
				setTriggerFired(*pEvent, false, false);
				break;
			}
			else if (kTrigger.isTeam() && GET_PLAYER((PlayerTypes)iJ).getTeam() == getTeam()) {
				setTriggerFired(*pEvent, false, false);
				break;
			}
		}
		resetEventOccured((EventTypes)iI, false);
	}

	resetPlotAndCityData(); // BBAI
	AI_init();
}

// Reset all data for this player stored in plot and city objects
void CvPlayer::resetPlotAndCityData()
{
	for (int iPlot = 0; iPlot < GC.getMap().numPlots(); ++iPlot)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(iPlot);

		pLoopPlot->setCulture(getID(), 0, false, false);
		pLoopPlot->setFoundValue(getID(), 0);

		CvCity* pLoopCity = pLoopPlot->getPlotCity();
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
	SAFE_DELETE_ARRAY(m_paiBonusExport);
	SAFE_DELETE_ARRAY(m_paiBonusImport);
	SAFE_DELETE_ARRAY(m_paiImprovementCount);
	SAFE_DELETE_ARRAY(m_paiFreeBuildingCount);
	SAFE_DELETE_ARRAY(m_paiExtraBuildingHappiness);
	SAFE_DELETE_ARRAY(m_paiExtraBuildingHealth);
	SAFE_DELETE_ARRAY(m_paiFeatureHappiness);
	SAFE_DELETE_ARRAY(m_paiUnitClassCount);
	SAFE_DELETE_ARRAY(m_paiUnitClassMaking);
	SAFE_DELETE_ARRAY(m_paiBuildingClassCount);
	SAFE_DELETE_ARRAY(m_paiBuildingClassMaking);
	SAFE_DELETE_ARRAY(m_paiHurryCount);
	SAFE_DELETE_ARRAY(m_paiSpecialBuildingNotRequiredCount);
	SAFE_DELETE_ARRAY(m_paiHasCivicOptionCount);
	SAFE_DELETE_ARRAY(m_paiNoCivicUpkeepCount);
	SAFE_DELETE_ARRAY(m_paiHasReligionCount);
	SAFE_DELETE_ARRAY(m_paiHasCorporationCount);
	SAFE_DELETE_ARRAY(m_paiUpkeepCount);
	SAFE_DELETE_ARRAY(m_paiSpecialistValidCount);

	SAFE_DELETE_ARRAY(m_pabResearchingTech);
	SAFE_DELETE_ARRAY(m_pabLoyalMember);

	SAFE_DELETE_ARRAY(m_paeCivics);

	m_triggersFired.clear();

	if (m_ppaaiSpecialistExtraYield != NULL)
	{
		for (int iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
		{
			SAFE_DELETE_ARRAY(m_ppaaiSpecialistExtraYield[iI]);
		}
		SAFE_DELETE_ARRAY(m_ppaaiSpecialistExtraYield);
	}

	if (m_ppaaiImprovementYieldChange != NULL)
	{
		for (int iI = 0; iI < GC.getNumImprovementInfos(); iI++)
		{
			SAFE_DELETE_ARRAY(m_ppaaiImprovementYieldChange[iI]);
		}
		SAFE_DELETE_ARRAY(m_ppaaiImprovementYieldChange);
	}
	// <advc.003p>
	if(m_aszBonusHelp != NULL) {
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
}

// Initialize data members that are serialized.
void CvPlayer::reset(PlayerTypes eID, bool bConstructorCall)
{
	int iI, iJ;

	//--------------------------------
	// Uninit class
	uninit();

	m_iStartingX = INVALID_PLOT_COORD;
	m_iStartingY = INVALID_PLOT_COORD;
	m_iTotalPopulation = 0;
	m_iTotalLand = 0;
	m_iTotalLandScored = 0;
	m_iGold = 0;
	m_iGoldPerTurn = 0;
	m_iAdvancedStartPoints = -1;
	m_iGoldenAgeTurns = 0;
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
	m_uiStartTime = 0;

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
	m_iChoosingFreeTechCount = 0; // K-Mod
	m_iNewMessages = 0; // advc.106b
	m_bAutoPlayJustEnded = false; // advc.127
	m_bSavingReplay = false; // advc.106i
	m_bScoreboardExpanded = false; // advc.085
	m_eReminderPending = NO_CIVIC; // advc.004x

	m_eID = eID;
	updateTeamType();
	updateHuman();
	if (m_eID != NO_PLAYER)
	{
		m_ePersonalityType = GC.getInitCore().getLeader(m_eID); //??? Is this repeated data???
	}
	else
	{
		m_ePersonalityType = NO_LEADER;
	}
	m_eCurrentEra = ((EraTypes)0);  //??? Is this repeated data???
	m_eLastStateReligion = NO_RELIGION;
	m_eParent = NO_PLAYER;

	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		m_aiSeaPlotYield[iI] = 0;
		m_aiYieldRateModifier[iI] = 0;
		m_aiCapitalYieldRateModifier[iI] = 0;
		m_aiExtraYieldThreshold[iI] = 0;
		m_aiTradeYieldModifier[iI] = 0;
	}

	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		m_aiFreeCityCommerce[iI] = 0;
		m_aiCommercePercent[iI] = 0;
		m_aiCommerceRate[iI] = 0;
		m_aiCommerceRateModifier[iI] = 0;
		m_aiCapitalCommerceRateModifier[iI] = 0;
		m_aiStateReligionBuildingCommerce[iI] = 0;
		m_aiSpecialistExtraCommerce[iI] = 0;
		m_aiCommerceFlexibleCount[iI] = 0;
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_aiGoldPerTurnByPlayer[iI] = 0;
		if (!bConstructorCall && getID() != NO_PLAYER)
		{
			GET_PLAYER((PlayerTypes) iI).m_aiGoldPerTurnByPlayer[getID()] = 0;
		}
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		m_aiEspionageSpendingWeightAgainstTeam[iI] = 0; // advc.120: Instead of 1

		if (!bConstructorCall && getTeam() != NO_TEAM)
		{
			for (iJ = 0; iJ < MAX_PLAYERS; iJ++)
			{
				if (GET_PLAYER((PlayerTypes) iJ).getTeam() == iI)
				{
					GET_PLAYER((PlayerTypes) iJ).setEspionageSpendingWeightAgainstTeam(getTeam(), 0); // advc.120
				}
			}
		}
	}

	for (iI = 0; iI < NUM_FEAT_TYPES; iI++)
	{
		m_abFeatAccomplished[iI] = false;
	}

	for (iI = 0; iI < NUM_PLAYEROPTION_TYPES; iI++)
	{
		m_abOptions[iI] = false;
	}

	m_szScriptData = "";

	if (!bConstructorCall)
	{
		FAssertMsg(0 < GC.getNumBonusInfos(), "GC.getNumBonusInfos() is not greater than zero but it is used to allocate memory in CvPlayer::reset");
		FAssertMsg(m_paiBonusExport==NULL, "about to leak memory, CvPlayer::m_paiBonusExport");
		m_paiBonusExport = new int [GC.getNumBonusInfos()];
		FAssertMsg(m_paiBonusImport==NULL, "about to leak memory, CvPlayer::m_paiBonusImport");
		m_paiBonusImport = new int [GC.getNumBonusInfos()];
		for (iI = 0; iI < GC.getNumBonusInfos(); iI++)
		{
			m_paiBonusExport[iI] = 0;
			m_paiBonusImport[iI] = 0;
		}

		FAssertMsg(0 < GC.getNumImprovementInfos(), "GC.getNumImprovementInfos() is not greater than zero but it is used to allocate memory in CvPlayer::reset");
		FAssertMsg(m_paiImprovementCount==NULL, "about to leak memory, CvPlayer::m_paiImprovementCount");
		m_paiImprovementCount = new int [GC.getNumImprovementInfos()];
		for (iI = 0; iI < GC.getNumImprovementInfos(); iI++)
		{
			m_paiImprovementCount[iI] = 0;
		}

		FAssertMsg(m_paiFreeBuildingCount==NULL, "about to leak memory, CvPlayer::m_paiFreeBuildingCount");
		m_paiFreeBuildingCount = new int [GC.getNumBuildingInfos()];
		FAssertMsg(m_paiExtraBuildingHappiness==NULL, "about to leak memory, CvPlayer::m_paiExtraBuildingHappiness");
		m_paiExtraBuildingHappiness = new int [GC.getNumBuildingInfos()];
		FAssertMsg(m_paiExtraBuildingHealth==NULL, "about to leak memory, CvPlayer::m_paiExtraBuildingHealth");
		m_paiExtraBuildingHealth = new int [GC.getNumBuildingInfos()];
		for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
		{
			m_paiFreeBuildingCount[iI] = 0;
			m_paiExtraBuildingHappiness[iI] = 0;
			m_paiExtraBuildingHealth[iI] = 0;
		}

		FAssertMsg(m_paiFeatureHappiness==NULL, "about to leak memory, CvPlayer::m_paiFeatureHappiness");
		m_paiFeatureHappiness = new int [GC.getNumFeatureInfos()];
		for (iI = 0; iI < GC.getNumFeatureInfos(); iI++)
		{
			m_paiFeatureHappiness[iI] = 0;
		}

		FAssertMsg(m_paiUnitClassCount==NULL, "about to leak memory, CvPlayer::m_paiUnitClassCount");
		m_paiUnitClassCount = new int [GC.getNumUnitClassInfos()];
		FAssertMsg(m_paiUnitClassMaking==NULL, "about to leak memory, CvPlayer::m_paiUnitClassMaking");
		m_paiUnitClassMaking = new int [GC.getNumUnitClassInfos()];
		for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
		{
			m_paiUnitClassCount[iI] = 0;
			m_paiUnitClassMaking[iI] = 0;
		}

		FAssertMsg(m_paiBuildingClassCount==NULL, "about to leak memory, CvPlayer::m_paiBuildingClassCount");
		m_paiBuildingClassCount = new int [GC.getNumBuildingClassInfos()];
		FAssertMsg(m_paiBuildingClassMaking==NULL, "about to leak memory, CvPlayer::m_paiBuildingClassMaking");
		m_paiBuildingClassMaking = new int [GC.getNumBuildingClassInfos()];
		for (iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
		{
			m_paiBuildingClassCount[iI] = 0;
			m_paiBuildingClassMaking[iI] = 0;
		}

		FAssertMsg(m_paiHurryCount==NULL, "about to leak memory, CvPlayer::m_paiHurryCount");
		m_paiHurryCount = new int [GC.getNumHurryInfos()];
		for (iI = 0; iI < GC.getNumHurryInfos(); iI++)
		{
			m_paiHurryCount[iI] = 0;
		}

		FAssertMsg(m_paiSpecialBuildingNotRequiredCount==NULL, "about to leak memory, CvPlayer::m_paiSpecialBuildingNotRequiredCount");
		m_paiSpecialBuildingNotRequiredCount = new int [GC.getNumSpecialBuildingInfos()];
		for (iI = 0; iI < GC.getNumSpecialBuildingInfos(); iI++)
		{
			m_paiSpecialBuildingNotRequiredCount[iI] = 0;
		}

		FAssertMsg(m_paiHasCivicOptionCount==NULL, "about to leak memory, CvPlayer::m_paiHasCivicOptionCount");
		m_paiHasCivicOptionCount = new int[GC.getNumCivicOptionInfos()];
		FAssertMsg(m_paiNoCivicUpkeepCount==NULL, "about to leak memory, CvPlayer::m_paiNoCivicUpkeepCount");
		m_paiNoCivicUpkeepCount = new int[GC.getNumCivicOptionInfos()];
		FAssertMsg(m_paeCivics==NULL, "about to leak memory, CvPlayer::m_paeCivics");
		m_paeCivics = new CivicTypes [GC.getNumCivicOptionInfos()];
		for (iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
		{
			m_paiHasCivicOptionCount[iI] = 0;
			m_paiNoCivicUpkeepCount[iI] = 0;
			m_paeCivics[iI] = NO_CIVIC;
		}

		FAssertMsg(m_paiHasReligionCount==NULL, "about to leak memory, CvPlayer::m_paiHasReligionCount");
		m_paiHasReligionCount = new int[GC.getNumReligionInfos()];
		for (iI = 0;iI < GC.getNumReligionInfos();iI++)
		{
			m_paiHasReligionCount[iI] = 0;
		}

		FAssertMsg(m_paiHasCorporationCount==NULL, "about to leak memory, CvPlayer::m_paiHasCorporationCount");
		m_paiHasCorporationCount = new int[GC.getNumCorporationInfos()];
		for (iI = 0;iI < GC.getNumCorporationInfos();iI++)
		{
			m_paiHasCorporationCount[iI] = 0;
		}

		FAssertMsg(m_pabResearchingTech==NULL, "about to leak memory, CvPlayer::m_pabResearchingTech");
		m_pabResearchingTech = new bool[GC.getNumTechInfos()];
		for (iI = 0; iI < GC.getNumTechInfos(); iI++)
		{
			m_pabResearchingTech[iI] = false;
		}

		FAssertMsg(m_pabLoyalMember==NULL, "about to leak memory, CvPlayer::m_pabLoyalMember");
		m_pabLoyalMember = new bool[GC.getNumVoteSourceInfos()];
		for (iI = 0; iI < GC.getNumVoteSourceInfos(); iI++)
		{
			m_pabLoyalMember[iI] = true;
		}

		FAssertMsg(0 < GC.getNumUpkeepInfos(), "GC.getNumUpkeepInfos() is not greater than zero but it is used to allocate memory in CvPlayer::reset");
		FAssertMsg(m_paiUpkeepCount==NULL, "about to leak memory, CvPlayer::m_paiUpkeepCount");
		m_paiUpkeepCount = new int[GC.getNumUpkeepInfos()];
		for (iI = 0; iI < GC.getNumUpkeepInfos(); iI++)
		{
			m_paiUpkeepCount[iI] = 0;
		}

		FAssertMsg(0 < GC.getNumSpecialistInfos(), "GC.getNumSpecialistInfos() is not greater than zero but it is used to allocate memory in CvPlayer::reset");
		FAssertMsg(m_paiSpecialistValidCount==NULL, "about to leak memory, CvPlayer::m_paiSpecialistValidCount");
		m_paiSpecialistValidCount = new int[GC.getNumSpecialistInfos()];
		for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
		{
			m_paiSpecialistValidCount[iI] = 0;
		}

		FAssertMsg(0 < GC.getNumSpecialistInfos(), "GC.getNumSpecialistInfos() is not greater than zero but it is used to allocate memory in CvPlayer::reset");
		FAssertMsg(m_ppaaiSpecialistExtraYield==NULL, "about to leak memory, CvPlayer::m_ppaaiSpecialistExtraYield");
		m_ppaaiSpecialistExtraYield = new int*[GC.getNumSpecialistInfos()];
		for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
		{
			m_ppaaiSpecialistExtraYield[iI] = new int[NUM_YIELD_TYPES];
			for (iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
			{
				m_ppaaiSpecialistExtraYield[iI][iJ] = 0;
			}
		}

		FAssertMsg(m_ppaaiImprovementYieldChange==NULL, "about to leak memory, CvPlayer::m_ppaaiImprovementYieldChange");
		m_ppaaiImprovementYieldChange = new int*[GC.getNumImprovementInfos()];
		for (iI = 0; iI < GC.getNumImprovementInfos(); iI++)
		{
			m_ppaaiImprovementYieldChange[iI] = new int[NUM_YIELD_TYPES];
			for (iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
			{
				m_ppaaiImprovementYieldChange[iI][iJ] = 0;
			}
		}
		// <advc.003p>
		FAssert(m_aszBonusHelp == NULL);
		m_aszBonusHelp = new CvWString*[GC.getNumBonusInfos()];
		for(int i = 0; i < GC.getNumBonusInfos(); i++)
			m_aszBonusHelp[i] = NULL;
		// </advc.003p>

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
	{
		AI_reset(false);
		// <advc.210>
		for(size_t i = 0; i < m_paAlerts.size(); i++)
			m_paAlerts[i]->init(getID()); // </advc.210>
	}
}


// CHANGE_PLAYER, 08/17/08, jdog5000: START
// for stripping obsolete trait bonuses
// for complete reset, use in conjunction with addTraitBonuses
//void CvPlayer::clearTraitBonuses() { ... }
//void CvPlayer::addTraitBonuses() { ... }
// advc.003q: Combined into a single function, which CvPlayer::init can call as well.
void CvPlayer::processTraits(int iChange)
{
	int iI, iJ;
	FAssert(GC.getNumTraitInfos() > 0);
	for (iI = 0; iI < GC.getNumTraitInfos(); iI++) {
		if (!hasTrait((TraitTypes)iI))
			continue;

		changeExtraHealth(iChange * GC.getTraitInfo((TraitTypes)iI).getHealth());
		changeExtraHappiness(iChange * GC.getTraitInfo((TraitTypes)iI).getHappiness());

		for (iJ = 0; iJ < GC.getNumBuildingInfos(); iJ++) {
			BuildingTypes eBuilding = (BuildingTypes)iJ;
			changeExtraBuildingHappiness(eBuilding, iChange * GC.getBuildingInfo(eBuilding).getHappinessTraits(iI));
		}

		CvTraitInfo const& kTrait = GC.getTraitInfo((TraitTypes)iI);
		changeUpkeepModifier(iChange * kTrait.getUpkeepModifier());
		changeLevelExperienceModifier(iChange * kTrait.getLevelExperienceModifier());
		changeGreatPeopleRateModifier(iChange * kTrait.getGreatPeopleRateModifier());
		changeGreatGeneralRateModifier(iChange * kTrait.getGreatGeneralRateModifier());
		changeDomesticGreatGeneralRateModifier(iChange * kTrait.getDomesticGreatGeneralRateModifier());

		changeMaxGlobalBuildingProductionModifier(iChange * kTrait.getMaxGlobalBuildingProductionModifier());
		changeMaxTeamBuildingProductionModifier(iChange * kTrait.getMaxTeamBuildingProductionModifier());
		changeMaxPlayerBuildingProductionModifier(iChange * kTrait.getMaxPlayerBuildingProductionModifier());

		for (iJ = 0; iJ < NUM_YIELD_TYPES; iJ++) {
			YieldTypes eYield = (YieldTypes)iJ;
			changeTradeYieldModifier(eYield, iChange * kTrait.getTradeYieldModifier(eYield));
		}

		for (iJ = 0; iJ < NUM_COMMERCE_TYPES; iJ++) {
			CommerceTypes eCommerce = (CommerceTypes)iJ;
			changeFreeCityCommerce(eCommerce, iChange * kTrait.getCommerceChange(eCommerce));
			changeCommerceRateModifier(eCommerce, iChange * kTrait.getCommerceModifier(eCommerce));
		}

		for (iJ = 0; iJ < GC.getNumCivicOptionInfos(); iJ++) {
			CivicOptionTypes eCivicOption = (CivicOptionTypes)iJ;
			if (GC.getCivicOptionInfo(eCivicOption).getTraitNoUpkeep(iI))
				changeNoCivicUpkeepCount(eCivicOption, iChange);
		}
	}

	/*  advc.003q: BBAI had only put the update calls into addTraitBonuses,
		but I reckon that they're also needed when clearing traits. And the
		setUnitExtraCost update wasn't in the BBAI code at all, but is probably
		needed for the Imperialistic trait. */

	updateMaxAnarchyTurns();

	CvCivilizationInfo const& kCivilization = GC.getCivilizationInfo(getCivilizationType());
	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
		updateExtraYieldThreshold((YieldTypes)iI);
	for (iI = 0; iI < GC.getNumCivicOptionInfos(); iI++) {
		setCivics((CivicOptionTypes)iI, (CivicTypes)kCivilization.
				getCivilizationInitialCivics(iI));
	}
	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++) {
		UnitTypes eUnit = (UnitTypes)(kCivilization.getCivilizationUnits(iI));
		if (eUnit != NO_UNIT && GC.getUnitInfo(eUnit).isFound())
			setUnitExtraCost((UnitClassTypes)iI, getNewCityProductionValue());
	}
}

// for changing the personality of the player
void CvPlayer::changePersonalityType()
{
	if(isBarbarian())
		return;

	if(!GC.getGame().isOption(GAMEOPTION_RANDOM_PERSONALITIES)) {
		setPersonalityType(getLeaderType());
		return;
	}

	int iBestValue = 0;
	LeaderHeadTypes eBestPersonality = NO_LEADER;

	for (int iI = 0; iI < GC.getNumLeaderHeadInfos(); iI++)
	{
		if (iI != GC.getDefineINT("BARBARIAN_LEADER")) // XXX minor civ???
		{
			int iValue = (1 + GC.getGame().getSorenRandNum(10000, "Choosing Personality"));

			for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
			{
				if (GET_PLAYER((PlayerTypes)iJ).isAlive())
				{
					if (GET_PLAYER((PlayerTypes)iJ).getPersonalityType() == ((LeaderHeadTypes)iI))
					{
						iValue /= 2;
					}
				}
			}

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				eBestPersonality = ((LeaderHeadTypes)iI);
			}
		}
	}

	if (eBestPersonality != NO_LEADER)
	{
		setPersonalityType(eBestPersonality);
	}
}

// reset state of event logic, unit prices
void CvPlayer::resetCivTypeEffects()
{
	int iI;

	if (!isAlive())
	{
		for (iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
		{
			setCivics(((CivicOptionTypes)iI), ((CivicTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationInitialCivics(iI))));
		}

		for (iI = 0; iI < GC.getNumEventInfos(); iI++)
		{
			resetEventOccured((EventTypes)iI, false);
		}

		for (iI = 0; iI < GC.getNumEventTriggerInfos(); iI++)
		{
			if(!GC.getEventTriggerInfo((EventTriggerTypes)iI).isGlobal() && (!GC.getEventTriggerInfo((EventTriggerTypes)iI).isTeam() || GET_TEAM(getTeam()).getNumMembers() == 1))
			{
				resetTriggerFired((EventTriggerTypes)iI);
			}
		}
	}

	for (iI = 0; iI < GC.getNumUnitClassInfos(); ++iI)
	{
		UnitTypes eUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

		if (NO_UNIT != eUnit)
		{
			if (GC.getUnitInfo(eUnit).isFound())
			{
				setUnitExtraCost((UnitClassTypes)iI, getNewCityProductionValue());
			}
		}
	}
}

// for switching the leaderhead of this player
void CvPlayer::changeLeader(LeaderHeadTypes eNewLeader)
{
	LeaderHeadTypes eOldLeader = getLeaderType();

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
		gDLL->getInterfaceIFace()->setDirty(HighlightPlot_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(UnitInfo_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(Flag_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(MinimapSection_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(Foreign_Screen_DIRTY_BIT, true);
	}
	/*  advc.104: Re-initializing the AI here was probably always a bad idea,
		but definitely mustn't re-initialize UWAI. */
	//AI_init();
	// <advc.tmp> Copy-pasted from AI_init. (AdvCiv 0.97 will clean this up.)
	AI().AI_setPeaceWeight(GC.getLeaderHeadInfo(getPersonalityType()).getBasePeaceWeight() + GC.getGame().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getPeaceWeightRand(), "AI Peace Weight"));
	AI().AI_setEspionageWeight(GC.getLeaderHeadInfo(getPersonalityType()).getEspionageWeight() * GC.getCommerceInfo(COMMERCE_ESPIONAGE).getAIWeightPercent() / 100);
	// </advc.tmp>
}

// (advc.003: Moved up so that all the CHANGE_PLAYER code dated 08/17/08 is in one place)
// for changing whether this player is human or not
void CvPlayer::setIsHuman(bool bNewValue)
{
	// <advc.706> Make sure that these are consistent
	if(!bNewValue)
		m_bDisableHuman = false; // </advc.706>
	if (bNewValue == isHuman())
		return;

	if (bNewValue)
		GC.getInitCore().setSlotStatus(getID(), SS_TAKEN);
	else
		GC.getInitCore().setSlotStatus(getID(), SS_COMPUTER); // or SS_OPEN for multiplayer?
}
// CHANGE_PLAYER: END
// CHANGE_PLAYER, 05/09/09, jdog5000: START
// for changing the civilization of this player
void CvPlayer::changeCiv(CivilizationTypes eNewCiv)
{
	CivilizationTypes eOldCiv = getCivilizationType();
	PlayerColorTypes eColor = (PlayerColorTypes)GC.getCivilizationInfo(eNewCiv).getDefaultPlayerColor();

	if (eOldCiv == eNewCiv)
		return;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (eColor == NO_PLAYERCOLOR || (GET_PLAYER((PlayerTypes)iI).getPlayerColor() == eColor && iI != getID()))
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

	GC.getInitCore().setCiv(getID(), eNewCiv);
	GC.getInitCore().setColor(getID(), eColor);

	resetCivTypeEffects();

	if (isAlive())
	{
		// if the player is alive and showing on scoreboard, etc
		// change colors, graphics, flags, units
		GC.getInitCore().setFlagDecal(getID(), (CvWString)GC.getCivilizationInfo(eNewCiv).getFlagTexture());
		GC.getInitCore().setArtStyle(getID(), (ArtStyleTypes)GC.getCivilizationInfo(eNewCiv).getArtStyleType());

		// Forces update of units flags
		EraTypes eEra = getCurrentEra();
		bool bAuto = m_bDisableHuman;
		m_bDisableHuman = true;
		//setCurrentEra((EraTypes)((eEra + 1)%GC.getNumEraInfos()));
		setCurrentEra((EraTypes)0);
		setCurrentEra((EraTypes)(GC.getNumEraInfos() - 1));

		setCurrentEra(eEra);
		m_bDisableHuman = bAuto;
		gDLL->getInterfaceIFace()->makeInterfaceDirty();

		int iLoop;
		CvCity* pLoopCity;
		// dirty all of this player's cities...
		for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			//if (pLoopCity->getOwner() == getID())
			FAssert(pLoopCity->getOwner() == getID()); // K-Mod
			{
				pLoopCity->setLayoutDirty(true);
			}
		}

		//update unit eras
		CvUnit* pLoopUnit;
		CvPlot* pLoopPlot;
		for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
		{
			pLoopUnit->reloadEntity();
			pLoopPlot = pLoopUnit->plot();
			/*if (pLoopPlot != NULL) {
				CvFlagEntity* pFlag = pLoopPlot->getFlagSymbol();
				if (pFlag != NULL) {
					if (gDLL->getFlagEntityIFace()->getPlayer(pFlag) == getID()) {
						gDLL->getFlagEntityIFace()->destroy(pFlag);
						CvFlagEntity* pNewFlag = gDLL->getFlagEntityIFace()->create(getID());
						if (pFlag != NULL)
							gDLL->getFlagEntityIFace()->setPlot(pNewFlag, pLoopPlot, false);
						gDLL->getFlagEntityIFace()->updateGraphicEra(pNewFlag);
					}
				}
				pLoopPlot->setFlagDirty(true);
				//pLoopPlot->updateGraphicEra();
			}*/
		}

		//update flag eras
		gDLL->getInterfaceIFace()->setDirty(Flag_DIRTY_BIT, true);
		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(Soundtrack_DIRTY_BIT, true);
		}
		gDLL->getInterfaceIFace()->makeInterfaceDirty();

		// Need to force redraw
		gDLL->getEngineIFace()->SetDirty(CultureBorders_DIRTY_BIT, true);
		gDLL->getEngineIFace()->SetDirty(MinimapTexture_DIRTY_BIT, true);
		gDLL->getEngineIFace()->SetDirty(GlobeTexture_DIRTY_BIT, true);
		gDLL->getEngineIFace()->SetDirty(GlobePartialTexture_DIRTY_BIT, true);

		gDLL->getInterfaceIFace()->setDirty(ColoredPlots_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(HighlightPlot_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(UnitInfo_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(GlobeLayer_DIRTY_BIT, true);
		// <advc.003p>
		if(getID() == GC.getGame().getActivePlayer())
			setBonusHelpDirty(); // </advc.003p>
		gDLL->getInterfaceIFace()->setDirty(MinimapSection_DIRTY_BIT, true);
		gDLL->getEngineIFace()->SetDirty(MinimapTexture_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(Foreign_Screen_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(SelectionSound_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(GlobeInfo_DIRTY_BIT, true);
	}
	else if (isEverAlive())
	{
		// Not currently alive, but may show on some people's scoreboard
		// or graphs
		// change colors
		gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
	}
	setupGraphical();
} // CHANGE_PLAYER: END

//////////////////////////////////////
// graphical only setup
//////////////////////////////////////
void CvPlayer::setupGraphical()
{
	if (!GC.IsGraphicsInitialized())
		return;

	// Setup m_cities
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		pLoopCity->setupGraphical();

	// Setup m_units
	for(CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
		pLoopUnit->setupGraphical();
}


void CvPlayer::initFreeState()
{
	setGold(0);

	if(!GC.getGame().isOption(GAMEOPTION_ADVANCED_START)) // advc.250c
		changeGold(GC.getHandicapInfo(getHandicapType()).getStartingGold());
	changeGold(GC.getEraInfo(GC.getGame().getStartEra()).getStartingGold());

	clearResearchQueue();
}


void CvPlayer::initFreeUnits()
{	// <dlph.28>
	if(isBarbarian())
		return; // </dlph.28>
	// <advc.003>
	CvGame& g = GC.getGame();
	int const iStartingUnitMultiplier = GC.getEraInfo(g.getStartEra()).
			getStartingUnitMultiplier();
	// </advc.003>
	if (g.isOption(GAMEOPTION_ADVANCED_START) &&
			(!isHuman() || !g.isOption(GAMEOPTION_SPAH))) // advc.250b
	{
		int iPoints = g.getNumAdvancedStartPoints();
		// advc.250b (comment): Disabled through Handicap XML
		iPoints *= GC.getHandicapInfo(getHandicapType()).getAdvancedStartPointsMod();
		iPoints /= 100;

		if (!isHuman() /* advc.250b: */ && !g.isOption(GAMEOPTION_SPAH))
		{	/*  advc.250b, advc.001: Was this->getHandicapType(), i.e. Noble, which
				means that this code block did nothing. */
			iPoints *= GC.getHandicapInfo(g.getHandicapType()).getAIAdvancedStartPercent();
			iPoints /= 100;
		}
		/*  <advc.250c> Civs in Advanced Start can place a city even if they don't
			have enough start points, but I prefer to enforce a minimum. */
		if(iPoints > 0) {
			int iMinPoints = GC.getInitCore().getAdvancedStartMinPoints();
			if(iPoints < iMinPoints)
				iPoints = iMinPoints;
		} // </advc.250c>
		setAdvancedStartPoints(iPoints);

		// Starting visibility
		CvPlot* pStartingPlot = getStartingPlot();
		if (NULL != pStartingPlot)
		{
			/*  advc.108: BtS code moved into a new function (b/c I need the same
				behavior elsewhere). */
			TEAMREF(getID()).revealSurroundingPlots(*pStartingPlot,
					GC.getDefineINT("ADVANCED_START_SIGHT_RANGE"));
		}
	}
	else
	{	// advc.003:
		CvCivilizationInfo const& kCivilization = GC.getCivilizationInfo(getCivilizationType());
		for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
		{
			UnitTypes eLoopUnit = (UnitTypes)kCivilization.getCivilizationUnits(iI);
			if (eLoopUnit != NO_UNIT)
			{
				int iFreeCount = kCivilization.getCivilizationFreeUnitsClass(iI);
				iFreeCount *= (iStartingUnitMultiplier + (!isHuman() ?
						GC.getHandicapInfo(g.getHandicapType()).
						getAIStartingUnitMultiplier() : 0));
				for (int iJ = 0; iJ < iFreeCount; iJ++)
					addFreeUnit(eLoopUnit);
			}
		}
		int iFreeCount = GC.getEraInfo(g.getStartEra()).getStartingDefenseUnits();
		iFreeCount += GC.getHandicapInfo(getHandicapType()).getStartingDefenseUnits();
		// <advc.126>
		CvHandicapInfo& h = GC.getHandicapInfo(g.getHandicapType());
		int iFreeAIDefenders = 0;
		// </advc.126>
		if (!isHuman())
		{   // <advc.126>
			iFreeAIDefenders = h.getAIStartingDefenseUnits();
			if(iFreeAIDefenders > 0)
				iFreeAIDefenders += g.getStartEra();
			iFreeCount += iFreeAIDefenders;
			// <advc.126>
		}

		if(iFreeCount > 0)
			addFreeUnitAI(UNITAI_CITY_DEFENSE, iFreeCount);

		iFreeCount = GC.getEraInfo(g.getStartEra()).getStartingWorkerUnits();
		iFreeCount += GC.getHandicapInfo(getHandicapType()).getStartingWorkerUnits();

		if (!isHuman())
		{   // <advc.126>
			int iFreeAIWorkers = h.getAIStartingWorkerUnits();
			if(iFreeAIWorkers > 0)
				iFreeAIWorkers += g.getStartEra() / 2;
			iFreeCount += iFreeAIWorkers;
			// <advc.126>
		}

		if (iFreeCount > 0)
		{
			addFreeUnitAI(UNITAI_WORKER, iFreeCount);
		}

		iFreeCount = GC.getEraInfo(g.getStartEra()).getStartingExploreUnits();
		iFreeCount += GC.getHandicapInfo(getHandicapType()).getStartingExploreUnits();

		if (!isHuman())
		{   // <advc.126>
			int iFreeAIExplorers = h.getAIStartingExploreUnits();
			/*  Need at least one addl. when starting in Classical era or later
				b/c the AI doesn't use its free defenders for exploration. */
			if(iFreeAIDefenders > 0)
				iFreeAIExplorers += (g.getStartEra() + 2)  / 3;
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
	for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		UnitTypes eLoopUnit = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI);

		if (eLoopUnit != NO_UNIT)
		{
			if (canTrain(eLoopUnit))
			{
				bool bValid = true;

				if (GC.getUnitInfo(eLoopUnit).getPrereqAndBonus() != NO_BONUS)
				{
					bValid = false;
				}

				for (int iJ = 0; iJ < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); iJ++)
				{
					if (GC.getUnitInfo(eLoopUnit).getPrereqOrBonuses(iJ) != NO_BONUS)
					{
						bValid = false;
					}
				}

				// <advc.307> Machine Gun not useful enough against Barbarians
				if(eUnitAI == UNITAI_CITY_DEFENSE &&
						GC.getUnitInfo(eLoopUnit).isOnlyDefensive())
					bValid = false; // </advc.307>

				if (bValid)
				{
					int iValue = AI_unitValue(eLoopUnit, eUnitAI, NULL);
					// <advc.250e> No Archer for exploration
					if(eUnitAI == UNITAI_EXPLORE)
						iValue -= AI_unitValue(eLoopUnit, UNITAI_CITY_DEFENSE, NULL);
					// </advc.250e>
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
		for (int iI = 0; iI < iCount; iI++)
			addFreeUnit(eBestUnit, eUnitAI);
	}
}


void CvPlayer::addFreeUnit(UnitTypes eUnit, UnitAITypes eUnitAI)
{
	// advc.108: Right-hand side cut from below
	bool const bFound = (eUnitAI == UNITAI_SETTLE) ||
			(GC.getUnitInfo(eUnit).getDefaultUnitAIType() == UNITAI_SETTLE);

	if (GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
	{
		if (bFound) // advc.108: No functional change
		{
			if (AI_getNumAIUnits(UNITAI_SETTLE) >= 1)
				return;
		}
	}

	CvPlot* pStartingPlot = getStartingPlot();
	if(pStartingPlot == NULL)
		return;

	CvPlot* pBestPlot = NULL;

	//if (isHuman())
	if(eUnitAI == UNITAI_EXPLORE) // advc.108
	{
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "startHumansOnSameTile", NULL, &lResult);
		if (lResult == 0)
		{
			if (!GC.getUnitInfo(eUnit).isFound())
			{
				int iRandOffset = GC.getGame().getSorenRandNum(NUM_CITY_PLOTS, "Place Units (Player)");

				for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
				{
					CvPlot* pLoopPlot = plotCity(pStartingPlot->getX(), pStartingPlot->getY(), ((iI + iRandOffset) % NUM_CITY_PLOTS));

					if (pLoopPlot != NULL)
					{
						if (pLoopPlot->getArea() == pStartingPlot->getArea())
						{
							if (!pLoopPlot->isImpassable())
							{
								if (!pLoopPlot->isUnit())
								{
									if (!pLoopPlot->isGoody())
									{
										pBestPlot = pLoopPlot;
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

	if (pBestPlot == NULL)
		pBestPlot = pStartingPlot;

	// <advc.108> Centered on the Settler, not on the starting plot.
	if(bFound)
	{
		TEAMREF(getID()).revealSurroundingPlots(*pBestPlot,
				GC.getDefineINT("START_SIGHT_RANGE"));
	} // </advc.108>
	initUnit(eUnit, pBestPlot->getX(), pBestPlot->getY(), eUnitAI);
}

int CvPlayer::startingPlotRange() const
{
	int iRange;

	iRange = (GC.getMap().maxStepDistance() + 10);

	iRange *= GC.getDefineINT("STARTING_DISTANCE_PERCENT");
	iRange /= 100;

	iRange *= (GC.getMap().getLandPlots() / (GC.getWorldInfo(GC.getMap().getWorldSize()).getTargetNumCities() * GC.getGame().countCivPlayersAlive()));
	iRange /= NUM_CITY_PLOTS;

	iRange += std::min(((GC.getMap().getNumAreas() + 1) / 2), GC.getGame().countCivPlayersAlive());

	long lResult=0;
	if (gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "minStartingDistanceModifier", NULL, &lResult))
	{
		iRange *= std::max<int>(0, (lResult + 100));
		iRange /= 100;
	}

	return std::max(iRange, GC.getDefineINT("MIN_CIV_STARTING_DISTANCE"));
}


bool CvPlayer::startingPlotWithinRange(CvPlot* pPlot, PlayerTypes ePlayer, int iRange, int iPass) const
{
	//PROFILE_FUNC();

	//XXX changes to AI_foundValue (which are far more flexible) make this function
	//    redundant but it is still called from Python.
	return false;
}

int CvPlayer::startingPlotDistanceFactor(CvPlot* pPlot, PlayerTypes ePlayer, int iRange) const
{
	PROFILE_FUNC();

	FAssert(ePlayer != getID());


	int iValue = 1000;
	CvPlot* pStartingPlot = getStartingPlot();
	if (pStartingPlot == NULL)
		return iValue; // advc.003

	if (GC.getGame().isTeamGame())
	{
		if (GET_PLAYER(ePlayer).getTeam() == getTeam())
		{
			iRange *= GC.getDefineINT("OWN_TEAM_STARTING_MODIFIER");
			iRange /= 100;
		}
		else
		{
			iRange *= GC.getDefineINT("RIVAL_TEAM_STARTING_MODIFIER");
			iRange /= 100;
		}
	}

	int iDistance = stepDistance(pPlot->getX(), pPlot->getY(), pStartingPlot->getX(), pStartingPlot->getY());
	if (pStartingPlot->getArea() != pPlot->getArea())
	{
		iDistance *= 4;
		iDistance /= 3;
	}

	iValue *= iDistance;
	iValue /= iRange ;

	return std::max(1, iValue);
}

// <advc.027>
int CvPlayer::coastRiverStartingAreaScore(CvArea const& a) const {

	int r = 0;
	// Loop based on CvArea::countCoastalLand
	for(int i = 0; i < GC.getMap().numPlots(); i++) {
		CvPlot* p = GC.getMap().plotByIndex(i);
		if(p->isPeak() || p->getArea() != a.getID())
			continue;
		int iTotalYield = p->calculateTotalBestNatureYield(getTeam());
		int iFoodYield = p->calculateBestNatureYield(YIELD_FOOD, getTeam());
		if((iFoodYield > 0 && iTotalYield >= 2) || iTotalYield >= 3) {
			if(p->isCoastalLand())
				r += 2;
			if(p->isRiver())
				r++;
		}
	}
	return r;
} // </advc.027>

// <dlph.35>
class VectorPairSecondGreaterComparator {
public:
	bool operator() (const std::pair<int,int> &a, const std::pair<int,int> &b) {
		return (a.second >= b.second);
	}
}; // </dlph.35>
// Returns the id of the best area, or -1 if it doesn't matter:
//int CvPlayer::findStartingArea() const
// dlph.35: "Returns a vector of all starting areas sorted by their value (instead of one best starting area)."
std::vector<std::pair<int,int> > CvPlayer::findStartingAreas() const  // advc.003: style changes
{
	PROFILE_FUNC();

	std::vector<std::pair<int,int> > areas_by_value; // dlph.35
	CvMap const& m = GC.getMap();

	long result = -1;
	CyArgsList argsList;
	argsList.add(getID());		// pass in this players ID
	if (gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->getMapScriptModule(), "findStartingArea", argsList.makeFunctionArgs(), &result)) {
		if (!gDLL->getPythonIFace()->pythonUsingDefaultImpl()) {
			if (result == -1 || m.getArea(result) != NULL) {
				//return result; // <dlph.35>
				areas_by_value.push_back(std::make_pair(result, 1));
				return areas_by_value; // </dlph.35>
			}
			else FAssertMsg(false, "python findStartingArea() must return -1 or the ID of a valid area");
		}
	}

	// find best land area
	//int iBestValue = 0; int iBestArea = -1; // dlph.35
	int iLoop = 0;
	for(CvArea* pLoopArea = m.firstArea(&iLoop); pLoopArea != NULL; pLoopArea = m.nextArea(&iLoop))
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
				::round(pLoopArea->getNumTotalBonuses() * 1.5) +
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
		}*/ // dlph.35:
		areas_by_value.push_back(std::make_pair(pLoopArea->getID(), iValue));
	}
	//return iBestArea; // <dlph.35>
	VectorPairSecondGreaterComparator kComparator;
	std::sort(areas_by_value.begin(), areas_by_value.end(), kComparator);
	areas_by_value.resize(8); // advc: No need to pass around every little island
	return areas_by_value; // </dlph.35>
}


CvPlot* CvPlayer::findStartingPlot(bool bRandomize)
{
	PROFILE_FUNC();
	CvMap const& m = GC.getMap();

	long result = -1;
	CyArgsList argsList;
	argsList.add(getID());		// pass in this players ID
	if (gDLL->getPythonIFace()->callFunction(gDLL->getPythonIFace()->
			getMapScriptModule(), "findStartingPlot", argsList.makeFunctionArgs(), &result)) {
		if (!gDLL->getPythonIFace()->pythonUsingDefaultImpl()) { // Python override
			CvPlot *pPlot = m.plotByIndex(result);
			if (pPlot != NULL)
				return pPlot;
			else {
				FAssertMsg(//false
					/*  advc.021a: Tectonics apparently assigns all starting plots
						at once on its own and then returns "None". Or something;
						doesn't seem to be a problem anyway. Now I'm having it
						return -10 to indicate that it's a known (or non-) issue. */
					result == -10,
					"python findStartingPlot() returned an invalid plot index!");
			}
		}
	}

	//int iBestArea = -1;
	// dlph.35: "This function is adjusted to work with a list of possible starting areas instead of a single one."
	std::vector<std::pair<int,int> > areas_by_value;
	bool bNew = false;
	if (getStartingPlot() != NULL)
	{
		//iBestArea = getStartingPlot()->getArea(); // dlph.35:
		areas_by_value.push_back(std::make_pair(getStartingPlot()->getArea(), 1));
		setStartingPlot(NULL, true);
		bNew = true;
	}

	AI_updateFoundValues(true);//this sets all plots found values to -1

	if (!bNew)
	{
		//iBestArea = findStartingArea();
		areas_by_value = findStartingAreas(); // dlph.35
	}

	int iRange = startingPlotRange();
	/*  <advc.140> Cut and pasted from CvMap::maxPlotDistance. I've changed that
		function, but I think the original formula might be needed here.
		I'm not sure I understand the purpose of this outer loop. */
	// ^dlph.35 replaces the outer loop
	/*int iMaxPlotDist = std::max(1, ::plotDistance(0, 0, ((m.isWrapX()) ?
			(m.getGridWidth() / 2) : (m.getGridWidth() - 1)),
			((m.isWrapY()) ? (m.getGridHeight() / 2) :
			(m.getGridHeight() - 1))));
	for(int iPass = 0; iPass < iMaxPlotDist; iPass++)*/ // </advc.140>
	/*  <dlph.35> "First pass avoids starting locations that have very little food
		(before normalization) to avoid starting on the edge of very bad terrain." */
	int iStartingRange = GC.getDefineINT("ADVANCED_START_SIGHT_RANGE");
	for(int iPass = 0; iPass < 2; iPass++)
	{
		for(size_t iJ = 0; iJ < areas_by_value.size(); iJ++)
		{ // </dlph.35>
			CvPlot *pBestPlot = NULL;
			int iBestValue = 0;
			for (int iI = 0; iI < m.numPlots(); iI++)
			{
				CvPlot* pLoopPlot = m.plotByIndex(iI);
				//if ((iBestArea == -1) || (pLoopPlot->getArea() == iBestArea))
				// <dlph.35>
				if (pLoopPlot->getArea() != areas_by_value[iJ].first)
					continue;
				if (iPass == 0) // "Avoid very bad terrain in the first pass."
				{
					int iTotalFood = 0;
					int iLandPlots = 0;
					for (int iX = -iStartingRange; iX <= iStartingRange; iX++)
					{
						for (int iY = -iStartingRange; iY <= iStartingRange; iY++)
						{
							CvPlot* pCheckPlot = plotXY(pLoopPlot->getX(), pLoopPlot->getY(), iX, iY);
							if (pCheckPlot != NULL && !pCheckPlot->isWater() &&
									(::plotDistance(pLoopPlot, pCheckPlot) <= iStartingRange))
							{
								iLandPlots++;
								iTotalFood += pCheckPlot->calculateBestNatureYield(YIELD_FOOD, NO_TEAM);
							}
						}
					}
					if (iTotalFood < std::max(1, iLandPlots) / 2)
						continue;
				} // </dlph.35>
				//the distance factor is now done inside foundValue
				int iValue = pLoopPlot->getFoundValue(getID());
				if (bRandomize && iValue > 0)
				{	/*  advc.003 (comment): That's a high random portion (found values tend
						to range between 0 and 9999 too), but I'm not sure which map scripts
						(if any) use bRandomize=true, so I'm not changing this. */
					iValue += GC.getGame().getSorenRandNum(10000,
							"Randomize Starting Location");
				}

				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					pBestPlot = pLoopPlot;
				}
			}

			if (pBestPlot != NULL)
				return pBestPlot;
		} // dlph.35: end of areas_by_value loop
		FAssertMsg(iPass != 0, "CvPlayer::findStartingPlot - could not find starting plot in first pass.");
	}

	FAssertMsg(false, "Could not find starting plot.");
	return NULL;
}


CvPlotGroup* CvPlayer::initPlotGroup(CvPlot* pPlot)
{
	CvPlotGroup* pPlotGroup = addPlotGroup();

	FAssertMsg(pPlotGroup != NULL, "PlotGroup is not assigned a valid value");

	pPlotGroup->init(pPlotGroup->getID(), getID(), pPlot);

	return pPlotGroup;
}


CvCity* CvPlayer::initCity(int iX, int iY, bool bBumpUnits, bool bUpdatePlotGroups,
		int iOccupationTimer) // advc.122
{
	PROFILE_FUNC();

	CvCity* pCity = addCity();

	FAssertMsg(pCity != NULL, "City is not assigned a valid value");
	FAssertMsg(!(GC.getMap().plot(iX, iY)->isCity()), "No city is expected at this plot when initializing new city");

	pCity->init(pCity->getID(), getID(), iX, iY, bBumpUnits, bUpdatePlotGroups,
			iOccupationTimer); // advc.122
	/*  advc.104: Moved out of CvCity::init so that the new city is
		already fully initialized */
	setFoundedFirstCity(true);
	return pCity;
}


void CvPlayer::acquireCity(CvCity* pOldCity, bool bConquest, bool bTrade, bool bUpdatePlotGroups)
{
	CvWString szBuffer;
	CvWString szName;
	bool bRaze = false;
	bool bGift = false;
	int iI=-1;

	CvPlot* pCityPlot = pOldCity->plot();

	CLLNode<IDInfo>* pUnitNode = pCityPlot->headUnitNode();
	CLinkList<IDInfo> oldUnits;
	while (pUnitNode != NULL)
	{
		oldUnits.insertAtEnd(pUnitNode->m_data);
		pUnitNode = pCityPlot->nextUnitNode(pUnitNode);
	}

	pUnitNode = oldUnits.head();

	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = oldUnits.next(pUnitNode);

		if (pLoopUnit && pLoopUnit->getTeam() != getTeam())
		{
			if (pLoopUnit->getDomainType() == DOMAIN_IMMOBILE)
			{
				pLoopUnit->kill(false, getID());
			}
		}
	}

	if (bConquest)
	{
		int iRange = pOldCity->getCultureLevel();
		for (int iDX = -iRange; iDX <= iRange; iDX++)  // advc.003: Some changes to reduce indentation
		{
			for (int iDY = -iRange; iDY <= iRange; iDY++)
			{
				if (pOldCity->cultureDistance(iDX, iDY) > iRange)
					continue;

				CvPlot* pLoopPlot = plotXY(pOldCity->getX(), pOldCity->getY(), iDX, iDY);
				if (pLoopPlot == NULL)
					continue;

				if (pLoopPlot->getOwner() != pOldCity->getOwner() ||
						pLoopPlot->getNumCultureRangeCities(pOldCity->getOwner()) != 1)
					continue;

				bool bForceUnowned = false;
				for (iI = 0; iI < MAX_PLAYERS; iI++)
				{
					CvPlayer const& kThirdParty = GET_PLAYER((PlayerTypes)iI);
					if (!kThirdParty.isAlive() || kThirdParty.getTeam() == getTeam() ||
							kThirdParty.getTeam() == pOldCity->getTeam())
						continue;

					if (pLoopPlot->getNumCultureRangeCities(kThirdParty.getID()) > 0)
					{
						bForceUnowned = true;
						break;
					}
				}
				if (bForceUnowned)
					pLoopPlot->setForceUnownedTimer(GC.getDefineINT("FORCE_UNOWNED_CITY_TIMER"));
			}
		}
	}

	if (pOldCity->getOriginalOwner() == pOldCity->getOwner())
		GET_PLAYER(pOldCity->getOriginalOwner()).changeCitiesLost(1);
	else if (pOldCity->getOriginalOwner() == getID())
		GET_PLAYER(pOldCity->getOriginalOwner()).changeCitiesLost(-1);

	if (bConquest)
	{
		szBuffer = gDLL->getText("TXT_KEY_MISC_CAPTURED_CITY", pOldCity->getNameKey()).GetCString();
		gDLL->getInterfaceIFace()->addHumanMessage(getID(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_CITYCAPTURE", MESSAGE_TYPE_MAJOR_EVENT, ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_CITY_EDIT")->getPath(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pOldCity->getX(), pOldCity->getY(), true, true);

		szName.Format(L"%s (%s)", pOldCity->getName().GetCString(), GET_PLAYER(pOldCity->getOwner()).getReplayName());

		for (iI = 0; iI < MAX_PLAYERS; iI++)
		{
			CvPlayer const& kObs = GET_PLAYER((PlayerTypes)iI); // advc.003
			if (!kObs.isAlive() || iI == getID())
				continue; // advc.003
			if (pOldCity->isRevealed(kObs.getTeam(), false)
					|| kObs.isSpectator()) // advc.127
			{
				szBuffer = gDLL->getText("TXT_KEY_MISC_CITY_CAPTURED_BY", szName.GetCString(), getCivilizationDescriptionKey());
				gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_CITYCAPTURED",
						MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY, // advc.106b
						ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_CITY_EDIT")->getPath(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pOldCity->getX(), pOldCity->getY(), true, true);
			}
		}

		szBuffer = gDLL->getText("TXT_KEY_MISC_CITY_WAS_CAPTURED_BY", szName.GetCString(), getCivilizationDescriptionKey());
		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szBuffer, pOldCity->getX(), pOldCity->getY(), (ColorTypes)GC.getInfoTypeForString("COLOR_WARNING_TEXT"));
	}

	int iCaptureGold = 0;

	if (bConquest)
	{	// Use python to determine city capture gold amounts...
		long lCaptureGold;
		lCaptureGold = 0;
		CyCity* pyOldCity = new CyCity(pOldCity);
		CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyOldCity));
		gDLL->getPythonIFace()->callFunction(PYGameModule, "doCityCaptureGold", argsList.makeFunctionArgs(),&lCaptureGold);
		delete pyOldCity;
		iCaptureGold = (int)lCaptureGold;
	}

	changeGold(iCaptureGold);

	bool* pabHasReligion = new bool[GC.getNumReligionInfos()];
	bool* pabHolyCity = new bool[GC.getNumReligionInfos()];
	bool* pabHasCorporation = new bool[GC.getNumCorporationInfos()];
	bool* pabHeadquarters = new bool[GC.getNumCorporationInfos()];
	int* paiNumRealBuilding = new int[GC.getNumBuildingInfos()];
	int* paiBuildingOriginalOwner = new int[GC.getNumBuildingInfos()];
	int* paiBuildingOriginalTime = new int[GC.getNumBuildingInfos()];

	for (iI = 0; iI < GC.getNumVoteSourceInfos(); ++iI)
	{
		pOldCity->processVoteSourceBonus((VoteSourceTypes)iI, false);
	}

	PlayerTypes eOldOwner = pOldCity->getOwner();
	PlayerTypes eOriginalOwner = pOldCity->getOriginalOwner();
	PlayerTypes eHighestCulturePlayer = pOldCity->findHighestCulture();
	int iGameTurnFounded = pOldCity->getGameTurnFounded();
	int iPopulation = pOldCity->getPopulation();
	int iHighestPopulation = pOldCity->getHighestPopulation();
	int iHurryAngerTimer = pOldCity->getHurryAngerTimer();
	int iConscriptAngerTimer = pOldCity->getConscriptAngerTimer();
	int iDefyResolutionAngerTimer = pOldCity->getDefyResolutionAngerTimer();
	int iOccupationTimer = pOldCity->getOccupationTimer();
	szName = pOldCity->getNameKey();
	int iDamage = pOldCity->getDefenseDamage();
	bool bBombarded = pOldCity->isBombarded(); // advc.004c
	int iOldCityId = pOldCity->getID();
	std::vector<int> aeFreeSpecialists;
	for (iI = 0; iI < GC.getNumSpecialistInfos(); ++iI)
	{
		aeFreeSpecialists.push_back(pOldCity->getAddedFreeSpecialistCount((SpecialistTypes)iI));
	}
	bool abEverOwned[MAX_PLAYERS];
	int aiCulture[MAX_PLAYERS];
	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		abEverOwned[iI] = pOldCity->isEverOwned((PlayerTypes)iI);
		aiCulture[iI] = pOldCity->getCultureTimes100((PlayerTypes)iI);
	}

	abEverOwned[getID()] = true;

	for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		pabHasReligion[iI] = pOldCity->isHasReligion((ReligionTypes)iI);
		pabHolyCity[iI] = pOldCity->isHolyCity((ReligionTypes)iI);
	}

	for (iI = 0; iI < GC.getNumCorporationInfos(); iI++)
	{
		pabHasCorporation[iI] = pOldCity->isHasCorporation((CorporationTypes)iI);
		pabHeadquarters[iI] = pOldCity->isHeadquarters((CorporationTypes)iI);
	}

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		paiNumRealBuilding[iI] = pOldCity->getNumRealBuilding((BuildingTypes)iI);
		paiBuildingOriginalOwner[iI] = pOldCity->getBuildingOriginalOwner((BuildingTypes)iI);
		paiBuildingOriginalTime[iI] = pOldCity->getBuildingOriginalTime((BuildingTypes)iI);
	}
	// <advc.001f>
	bool m_abRevealed[MAX_TEAMS];
	for(int i = 0; i < MAX_TEAMS; i++)
		m_abRevealed[i] = pOldCity->isRevealed((TeamTypes)i, false);
	// </advc.001f>
	std::vector<BuildingYieldChange> aBuildingYieldChange;
	std::vector<BuildingCommerceChange> aBuildingCommerceChange;
	BuildingChangeArray aBuildingHappyChange;
	BuildingChangeArray aBuildingHealthChange;
	for (iI = 0; iI < GC.getNumBuildingClassInfos(); ++iI)
	{
		for (int iYield = 0; iYield < NUM_YIELD_TYPES; ++iYield)
		{
			BuildingYieldChange kChange;
			kChange.eBuildingClass = (BuildingClassTypes)iI;
			kChange.eYield = (YieldTypes)iYield;
			kChange.iChange = pOldCity->getBuildingYieldChange((BuildingClassTypes)iI, (YieldTypes)iYield);
			if (kChange.iChange != 0)
			{
				aBuildingYieldChange.push_back(kChange);
			}
		}

		for (int iCommerce = 0; iCommerce < NUM_COMMERCE_TYPES; ++iCommerce)
		{
			BuildingCommerceChange kChange;
			kChange.eBuildingClass = (BuildingClassTypes)iI;
			kChange.eCommerce = (CommerceTypes)iCommerce;
			kChange.iChange = pOldCity->getBuildingCommerceChange((BuildingClassTypes)iI, (CommerceTypes)iCommerce);
			if (kChange.iChange != 0)
			{
				aBuildingCommerceChange.push_back(kChange);
			}
		}

		int iChange = pOldCity->getBuildingHappyChange((BuildingClassTypes)iI);
		if (iChange != 0)
		{
			aBuildingHappyChange.push_back(std::make_pair((BuildingClassTypes)iI, iChange));
		}

		iChange = pOldCity->getBuildingHealthChange((BuildingClassTypes)iI);
		if (iChange != 0)
		{
			aBuildingHealthChange.push_back(std::make_pair((BuildingClassTypes)iI, iChange));
		}
	}

	bool bRecapture = ((eHighestCulturePlayer != NO_PLAYER) ? (GET_PLAYER(eHighestCulturePlayer).getTeam() == getTeam()) : false);

	pOldCity->kill(false);

	if (bTrade)
	{
		// <advc.122>
		for(int i = 0; i < NUM_CITY_PLOTS; i++) {
			CvPlot* pp = plotCity(
					pCityPlot->getX(), pCityPlot->getY(), i);
			if(pp == NULL) continue; CvPlot& p = *pp;
			int dist = ::plotDistance(pp, pCityPlot);
			CvCity* wc = p.getWorkingCity();
			/*  Always reduce eOldOwner culture in the inner circle. Outer circle:
				Only if tiles not worked by other cities of eOldOwner. */
			if(dist > 1 && wc != NULL && wc->plot() != pCityPlot &&
					wc->getOwner() != eOldOwner)
				continue;
			int convertedCulture = std::min(p.getCulture(eOldOwner) / 2,
					2 * p.getCulture(getID()));
			p.changeCulture(eOldOwner, -convertedCulture, false);
			p.changeCulture(getID(), convertedCulture, false);
		}
		// BtS code replaced by the loop above // </advc.122>
		/*for (int iDX = -1; iDX <= 1; iDX++) {
			for (int iDY = -1; iDY <= 1; iDY++) {
				CvPlot* pLoopPlot = plotXY(pCityPlot->getX(), pCityPlot->getY(), iDX, iDY);
				if (pLoopPlot != NULL)
					pLoopPlot->setCulture(eOldOwner, 0, false, false);
			}
		}*/
		// <advc.123d>
		for(int i = 0; i < MAX_CIV_TEAMS; i++) {
			CvTeam const& t = GET_TEAM((TeamTypes)i);
			if(!t.isAlive() || t.isMinorCiv() || t.getID() == TEAMID(eOldOwner) ||
					t.getID() == getTeam() || !t.isAtWar(TEAMID(eOldOwner)))
				continue;
			bool bEverOwned = false;
			for(int j = 0; j < MAX_CIV_PLAYERS; j++) {
				CvPlayer const& kMember = GET_PLAYER((PlayerTypes)j);
				if(kMember.isAlive() && kMember.getTeam() == t.getID() &&
						abEverOwned[kMember.getID()]) {
					bEverOwned = true;
					break;
				}
			}
			if(bEverOwned) {
				TEAMREF(eOldOwner).AI_changeWarSuccess(t.getID(),
						-std::min(GC.getWAR_SUCCESS_CITY_CAPTURING(),
						TEAMREF(eOldOwner).AI_getWarSuccess(t.getID())));
			}
		} // </advc.123d>
	}

	CvCity* pNewCity = initCity(pCityPlot->getX(), pCityPlot->getY(), !bConquest, false,
			// advc.122: Moved (way) up
			(bTrade && !bRecapture) ? iOccupationTimer : 0);

	FAssertMsg(pNewCity != NULL, "NewCity is not assigned a valid value");

	pNewCity->setPreviousOwner(eOldOwner);
	pNewCity->setOriginalOwner(eOriginalOwner);
	pNewCity->setGameTurnFounded(iGameTurnFounded);
	pNewCity->setPopulation((bConquest && !bRecapture) ? std::max(1, (iPopulation - 1)) : iPopulation);
	pNewCity->setHighestPopulation(iHighestPopulation);
	pNewCity->setName(szName, /* advc.106k: */ false, true);
	pNewCity->setNeverLost(false);
	pNewCity->changeDefenseDamage(iDamage);
	/*  advc.004c: The above will set the city as bombarded if iDamage>0.
		But iDamage>0 doesn't imply that the city was recently bombarded. */
	pNewCity->setBombarded(bBombarded);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		pNewCity->setEverOwned(((PlayerTypes)iI), abEverOwned[iI]);
		pNewCity->setCultureTimes100(((PlayerTypes)iI), aiCulture[iI], false, false);
	} // <dlph.23>
	if(bTrade) {
		int oldOwnerCulture = pNewCity->getCultureTimes100(eOldOwner);
		int newOwnerCulture = pNewCity->getCultureTimes100(getID());
		// Round down to a multiple of 100
		int convertedCulture = (oldOwnerCulture / 300) * 100;
		pNewCity->setCultureTimes100(getID(),
				newOwnerCulture + convertedCulture, true, false);
		pNewCity->setCultureTimes100(eOldOwner,
				oldOwnerCulture - convertedCulture, true, false);
	} // </dlph.23>

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)  // advc.003: body refactored (incl. added comments)
	{
		if (paiNumRealBuilding[iI] <= 0)
			continue;

		BuildingTypes eBuilding = (BuildingTypes)iI;
		BuildingClassTypes eBuildingClass = (BuildingClassTypes)GC.getBuildingInfo(
				eBuilding).getBuildingClassType();
		// Can't acquire another civ's unique building
		if (!::isWorldWonderClass(eBuildingClass)) // So that Barbarians can capture wonders
		{
			eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).
					getCivilizationBuildings(eBuildingClass);
		}
		if (eBuilding == NO_BUILDING)
			continue;

		CvBuildingInfo const& kBuilding = GC.getBuildingInfo(eBuilding);
		// Can acquire never-capture buildings only through city trade
		if (!bTrade && kBuilding.isNeverCapture())
			continue;

		if (isProductionMaxedBuildingClass(eBuildingClass, true) ||
				!pNewCity->isValidBuildingLocation(eBuilding))
			continue;

		// Capture roll unless recapture
		int iOdds = kBuilding.getConquestProbability();
		if (!bConquest || bRecapture || (iOdds > 0 &&
				iOdds > GC.getGame().getSorenRandNum(100, "Capture Probability")))
		{
			pNewCity->setNumRealBuildingTimed(eBuilding, std::min(GC.getCITY_MAX_NUM_BUILDINGS(),
					pNewCity->getNumRealBuilding(eBuilding) + paiNumRealBuilding[iI]),
					false, (PlayerTypes)paiBuildingOriginalOwner[iI], paiBuildingOriginalTime[iI]);
		}

	}

	for (std::vector<BuildingYieldChange>::iterator it = aBuildingYieldChange.begin(); it != aBuildingYieldChange.end(); ++it)
	{
		pNewCity->setBuildingYieldChange((*it).eBuildingClass, (*it).eYield, (*it).iChange);
	}

	for (std::vector<BuildingCommerceChange>::iterator it = aBuildingCommerceChange.begin(); it != aBuildingCommerceChange.end(); ++it)
	{
		pNewCity->setBuildingCommerceChange((*it).eBuildingClass, (*it).eCommerce, (*it).iChange);
	}

	for (BuildingChangeArray::iterator it = aBuildingHappyChange.begin(); it != aBuildingHappyChange.end(); ++it)
	{
		pNewCity->setBuildingHappyChange((*it).first, (*it).second);
	}

	for (BuildingChangeArray::iterator it = aBuildingHealthChange.begin(); it != aBuildingHealthChange.end(); ++it)
	{
		pNewCity->setBuildingHealthChange((*it).first, (*it).second);
	}

	for (iI = 0; iI < GC.getNumSpecialistInfos(); ++iI)
	{
		pNewCity->changeFreeSpecialistCount((SpecialistTypes)iI, aeFreeSpecialists[iI]);
	}

	for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		if (pabHasReligion[iI])
		{
			pNewCity->setHasReligion(((ReligionTypes)iI), true, false, true);
		}

		if (pabHolyCity[iI])
		{
			GC.getGame().setHolyCity(((ReligionTypes)iI), pNewCity, false);
		}
	}

	for (iI = 0; iI < GC.getNumCorporationInfos(); iI++)
	{
		if (pabHasCorporation[iI])
		{
			pNewCity->setHasCorporation(((CorporationTypes)iI), true, false);
		}

		if (pabHeadquarters[iI])
		{
			GC.getGame().setHeadquarters(((CorporationTypes)iI), pNewCity, false);
		}
	}

	if (bTrade)
	{
		if (isHuman() || (getTeam() == GET_PLAYER(eOldOwner).getTeam()))
		{
			pNewCity->changeHurryAngerTimer(iHurryAngerTimer);
			pNewCity->changeConscriptAngerTimer(iConscriptAngerTimer);
			pNewCity->changeDefyResolutionAngerTimer(iDefyResolutionAngerTimer);
		}
		/*if (!bRecapture) // advc.122: Moved up
			pNewCity->changeOccupationTimer(iOccupationTimer);*/
	}

	if (bConquest)
	{
		int iTeamCulturePercent = pNewCity->calculateTeamCulturePercent(getTeam());

		if (iTeamCulturePercent < GC.getDefineINT("OCCUPATION_CULTURE_PERCENT_THRESHOLD"))
		{
			int iPopPercent = GC.getDefineINT("OCCUPATION_TURNS_POPULATION_PERCENT");
			pNewCity->changeOccupationTimer(
				/*  advc.023: Population size as upper bound, and iPopPercent set to
					0 through XML. (Im multiplying by 1+100*iPopPercent so that the
					upper bound has no effect if iPopPercent is set back to 50 in XML.)
					NB: iTeamCulturePercent is city culture, not tile culture;
					only relevant when a city is reconquered. */
				std::min(pNewCity->getPopulation() * (1 + iPopPercent * 100),
				((GC.getDefineINT("BASE_OCCUPATION_TURNS") +
				((pNewCity->getPopulation() * iPopPercent) / 100)) *
				(100 - iTeamCulturePercent)) / 100));
		}

		GC.getMap().verifyUnitValidPlot();
	}
	//pCityPlot->setRevealed(GET_PLAYER(eOldOwner).getTeam(), true, false, NO_TEAM, false);
	/*  <advc.001f> Need to reveal the new city so that the raze message is
		properly delivered. Reveal the tile once the raze decision is through
		(too early here). */
	for(int i = 0; i < MAX_TEAMS; i++)
	{
		if(m_abRevealed[i])
			pNewCity->setRevealed((TeamTypes)i, true);
	} // </advc.001f>
	pNewCity->updateEspionageVisibility(false);

	if (bUpdatePlotGroups)
	{
		GC.getGame().updatePlotGroups();
	} // <advc.104>
	if(getWPAI.isEnabled() || getWPAI.isEnabled(true)) {
		for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
			CvPlayerAI& civ = GET_PLAYER((PlayerTypes)i);
			if(civ.isAlive() && !civ.isMinorCiv()) {
				civ.warAndPeaceAI().getCache().reportCityOwnerChanged(
						pNewCity, eOldOwner);
			}
		}
	} // </advc.104>
	CvEventReporter::getInstance().cityAcquired(eOldOwner, getID(), pNewCity, bConquest, bTrade);
	if (gPlayerLogLevel >= 1) // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
		logBBAI("  Player %d (%S) acquires city %S bConq %d bTrade %d", getID(), getCivilizationDescription(0), pNewCity->getName(0).GetCString(), bConquest, bTrade);

	SAFE_DELETE_ARRAY(pabHasReligion);
	SAFE_DELETE_ARRAY(pabHolyCity);
	SAFE_DELETE_ARRAY(pabHasCorporation);
	SAFE_DELETE_ARRAY(pabHeadquarters);
	SAFE_DELETE_ARRAY(paiNumRealBuilding);
	SAFE_DELETE_ARRAY(paiBuildingOriginalOwner);
	SAFE_DELETE_ARRAY(paiBuildingOriginalTime);

	if (bConquest)
	{
		CyCity* pyCity = new CyCity(pNewCity);
		CyArgsList argsList;
		argsList.add(getID());
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyCity));
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "canRazeCity", argsList.makeFunctionArgs(), &lResult);
		delete pyCity;
		if (lResult == 1)
		{	//auto raze based on game rules
			if (pNewCity->isAutoRaze())
			{
				if (iCaptureGold > 0)
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_PILLAGED_CITY", iCaptureGold, pNewCity->getNameKey());
					gDLL->getInterfaceIFace()->addHumanMessage(getID(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_CITYRAZE",
							MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY, // advc.106b
							ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_CITY_EDIT")->getPath(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pNewCity->getX(), pNewCity->getY(), true, true);
				}

				pNewCity->doTask(TASK_RAZE);
			}
			else if (!isHuman())
				AI_conquerCity(pNewCity); // could delete the pointer...
			else
			{	//popup raze option
				eHighestCulturePlayer = pNewCity->getLiberationPlayer(true);
				bRaze = canRaze(pNewCity);
				bGift = (eHighestCulturePlayer != NO_PLAYER &&
						eHighestCulturePlayer != getID() &&
						(getTeam() == GET_PLAYER(eHighestCulturePlayer).getTeam() ||
						GET_TEAM(getTeam()).isOpenBorders(TEAMID(eHighestCulturePlayer)) ||
						GET_TEAM(GET_PLAYER(eHighestCulturePlayer).getTeam()).isVassal(getTeam())));

				if (bRaze || bGift)
				{
					CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_RAZECITY);
					pInfo->setData1(pNewCity->getID());
					pInfo->setData2(eHighestCulturePlayer);
					pInfo->setData3(iCaptureGold);
					gDLL->getInterfaceIFace()->addPopup(pInfo, getID());
				}
				else
				{/* original bts code
					pNewCity->chooseProduction();
					CvEventReporter::getInstance().cityAcquiredAndKept(getID(), pNewCity); */
					lResult = 0; // K-Mod. (signal that we cannot raze the city.)
				}
			}
		}
		// K-Mod. properly handle the case where python says we can't raze the city
		// (note. this isn't in an 'else' because I overwrite lResult in the case above.)
		if (lResult == 0)
		{
			CvEventReporter::getInstance().cityAcquiredAndKept(getID(), pNewCity);
			if (isHuman())
				pNewCity->chooseProduction();
		} // K-Mod end
	}
	else if (!bTrade)
	{
		if (isHuman())
		{
			CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_DISBANDCITY);
			pInfo->setData1(pNewCity->getID());
			gDLL->getInterfaceIFace()->addPopup(pInfo, getID());
		}
		else
		{
			CvEventReporter::getInstance().cityAcquiredAndKept(getID(), pNewCity);
		}
	} // <advc.004x>
	if(bTrade && isHuman())
		pNewCity->chooseProduction(); // </advc.004x>
	// Forcing events that deal with the old city not to expire just because we conquered that city
	for (CvEventMap::iterator it = m_mapEventsOccured.begin(); it != m_mapEventsOccured.end(); ++it)
	{
		EventTriggeredData &triggerData = it->second;
		if((triggerData.m_eOtherPlayer == eOldOwner) && (triggerData.m_iOtherPlayerCityId == iOldCityId))
		{
			triggerData.m_iOtherPlayerCityId = -1;
		}
	} // <advc.001f>
	for(int i = 0; i < MAX_TEAMS; i++)
		if(m_abRevealed[i])
			pCityPlot->setRevealed((TeamTypes)i, true, false, NO_TEAM, false);
	// </advc.001f>
	/* <advc.001> Elimination otherwise happens only in CvGame::update, which
	   appears to be called only during the human player's turn. That means a dead
	   AI player can get one more turn before being eliminated. Not normally a problem
	   because there isn't much a civ without assets can do, but I've had a case where
	   a dead player arranged a vassal agreement. */
	if(eOldOwner != NO_PLAYER)
		GET_PLAYER(eOldOwner).verifyAlive(); // </advc.001>
	AI().AI_updateCityAttitude(*pCityPlot); // advc.130w
}


void CvPlayer::killCities()
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		pLoopCity->kill(false);
	GC.getGame().updatePlotGroups();
}


CvWString CvPlayer::getNewCityName() const
{
	CLLNode<CvWString>* pNode;
	CvWString szName;
	int iI;

	/* original bts code
	for (pNode = headCityNameNode(); (pNode != NULL); pNode = nextCityNameNode(pNode)) {
		szName = gDLL->getText(pNode->m_data);
		if (isCityNameValid(szName, true)) {
			szName = pNode->m_data;
			break;
		}
	} */
	// K-Mod
	for (pNode = headCityNameNode(); pNode && szName.empty(); pNode = nextCityNameNode(pNode))
	{
		szName = gDLL->getText(pNode->m_data); // (temp use of the buffer)
		if (isCityNameValid(szName, true))
			szName = pNode->m_data;
		else szName.clear(); // clear the buffer if the name is not valid!
	}
	// Note: unfortunately, the name-skipping system in getCivilizationCityName does not apply here.
	// K-Mod end

	if (szName.empty())
	{
		getCivilizationCityName(szName, getCivilizationType());
	}

	if (szName.empty())
	{
		// Pick a name from another random civ
		int iRandOffset = GC.getGame().getSorenRandNum(GC.getNumCivilizationInfos(), "Place Units (Player)");
		for (iI = 0; iI < GC.getNumCivilizationInfos(); iI++)
		{
			int iLoopName = ((iI + iRandOffset) % GC.getNumCivilizationInfos());

			getCivilizationCityName(szName, ((CivilizationTypes)iLoopName));

			if (!szName.empty())
			{
				break;
			}
		}
	}

	if (szName.empty())
	{
		szName = "TXT_KEY_CITY";
	}

	return szName;
}


void CvPlayer::getCivilizationCityName(CvWString& szBuffer, CivilizationTypes eCivilization) const
{
	int iRandOffset;
	int iLoopName;
	int iI;

	/* original bts code
	if (isBarbarian() || isMinorCiv())
		iRandOffset = GC.getGame().getSorenRandNum(GC.getCivilizationInfo(eCivilization).getNumCityNames(), "Place Units (Player)");
	else iRandOffset = 0;*/
	// K-Mod
	if (eCivilization != getCivilizationType() || isBarbarian() || isMinorCiv())
		iRandOffset = GC.getGame().getSorenRandNum(GC.getCivilizationInfo(eCivilization).getNumCityNames(), "City name offset");
	else
		iRandOffset = std::max(0, getPlayerRecord()->getNumCitiesBuilt() - getNumCityNames()); // note: the explicit city names list is checked before this function is called.
	// K-Mod end

	for (iI = 0; iI < GC.getCivilizationInfo(eCivilization).getNumCityNames(); iI++)
	{
		iLoopName = ((iI + iRandOffset) % GC.getCivilizationInfo(eCivilization).getNumCityNames());

		CvWString szName = gDLL->getText(GC.getCivilizationInfo(eCivilization).getCityNames(iLoopName));

		if (isCityNameValid(szName, true))
		{
			szBuffer = GC.getCivilizationInfo(eCivilization).getCityNames(iLoopName);
			break;
		}
	}
}


bool CvPlayer::isCityNameValid(CvWString& szName, bool bTestDestroyed) const
{
	CvCity* pLoopCity;
	int iLoop;

	if (bTestDestroyed)
	{
		if (GC.getGame().isDestroyedCityName(szName))
		{
			return false;
		}

		for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
		{
			CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
			for (pLoopCity = kLoopPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kLoopPlayer.nextCity(&iLoop))
			{
				if (pLoopCity->getName() == szName)
				{
					return false;
				}
			}
		}
	}
	else
	{
		for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			if (pLoopCity->getName() == szName)
			{
				return false;
			}
		}
	}

	return true;
}


CvUnit* CvPlayer::initUnit(UnitTypes eUnit, int iX, int iY, UnitAITypes eUnitAI, DirectionTypes eFacingDirection)
{
	PROFILE_FUNC();

	FAssertMsg(eUnit != NO_UNIT, "Unit is not assigned a valid value");

	CvUnit* pUnit = addUnit();
	FAssertMsg(pUnit != NULL, "Unit is not assigned a valid value");
	if (NULL != pUnit)
	{
		pUnit->init(pUnit->getID(), eUnit, ((eUnitAI == NO_UNITAI) ? ((UnitAITypes)(GC.getUnitInfo(eUnit).getDefaultUnitAIType())) : eUnitAI), getID(), iX, iY, eFacingDirection);
	}

	return pUnit;
}


void CvPlayer::disbandUnit(bool bAnnounce)
{
	int iBestValue = MAX_INT;
	CvUnit* pBestUnit = NULL;
	int iLoop;
	for(CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		if (!pLoopUnit->hasCargo())
		{
			if (!pLoopUnit->isGoldenAge())
			{
				if (pLoopUnit->getUnitInfo().getProductionCost() > 0)
				{
					if (!pLoopUnit->isMilitaryHappiness() || !pLoopUnit->plot()->isCity() || pLoopUnit->plot()->plotCount(PUF_isMilitaryHappiness, -1, -1, getID()) > 1)
					{
						int iValue = (10000 + GC.getGame().getSorenRandNum(1000, "Disband Unit"));
						iValue += (pLoopUnit->getUnitInfo().getProductionCost() * 5);
						iValue += (pLoopUnit->getExperience() * 20);
						iValue += (pLoopUnit->getLevel() * 100);

						if (pLoopUnit->canDefend() && pLoopUnit->plot()->isCity()
								/*  advc.001s: I suppose this clause is intended for
									potential city defenders */
								&& pLoopUnit->getDomainType() == DOMAIN_LAND)
							iValue *= 2;

						if (pLoopUnit->plot()->getTeam() == pLoopUnit->getTeam())
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
							break;
						}

						if (pLoopUnit->getUnitInfo().getExtraCost() > 0)
						{
							iValue /= (pLoopUnit->getUnitInfo().getExtraCost() + 1);
						}

						if (iValue < iBestValue)
						{
							iBestValue = iValue;
							pBestUnit = pLoopUnit;
						}
					}
				}
			}
		}
	}

	if (pBestUnit != NULL)
	{
		wchar szBuffer[1024];
		swprintf(szBuffer, gDLL->getText("TXT_KEY_MISC_UNIT_DISBANDED_NO_MONEY", pBestUnit->getNameKey()).GetCString());
		gDLL->getInterfaceIFace()->addHumanMessage(getID(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_UNITDISBANDED", MESSAGE_TYPE_MINOR_EVENT, pBestUnit->getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pBestUnit->getX(), pBestUnit->getY(), true, true);

		FAssert(!(pBestUnit->isGoldenAge()));

		pBestUnit->kill(false);
	}
}


void CvPlayer::killUnits()
{
	int iLoop;
	for (CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
		pLoopUnit->kill(false);
}


// XXX should pUnit be a CvSelectionGroup???
// Returns the next unit in the cycle...
CvSelectionGroup* CvPlayer::cycleSelectionGroups(CvUnit* pUnit, bool bForward,
		bool bWorkers, bool* pbWrap)
{
	FAssert(GC.getGame().getActivePlayer() == getID() && isHuman());
	// <advc.004h>
	if(pUnit->canFound())
		pUnit->updateFoundingBorder(true); // </advc.004h>
	// K-Mod
	bool bDummy;
	// this means we can just use bWrap directly and it will update *pbWrap if need be.
	bool& bWrap = pbWrap ? *pbWrap : bDummy;
	std::set<int>& cycled_groups = GC.getGame().getActivePlayerCycledGroups();
	// K-Mod end

	/* if (pbWrap != NULL)
		*pbWrap = false;*/
	bWrap = false;

	CLLNode<int>* pSelectionGroupNode = headGroupCycleNode();

	if (pUnit != NULL)
	{
		while (pSelectionGroupNode != NULL)
		{
			if (getSelectionGroup(pSelectionGroupNode->m_data) == pUnit->getGroup())
			{
				// K-Mod
				if (isTurnActive())
					cycled_groups.insert(pSelectionGroupNode->m_data);
				//
				if (bForward)
				{
					pSelectionGroupNode = nextGroupCycleNode(pSelectionGroupNode);
				}
				else
				{
					pSelectionGroupNode = previousGroupCycleNode(pSelectionGroupNode);
				}
				break;
			}

			pSelectionGroupNode = nextGroupCycleNode(pSelectionGroupNode);
		}
	}

	if (pSelectionGroupNode == NULL)
	{
		if (bForward)
		{
			pSelectionGroupNode = headGroupCycleNode();
		}
		else
		{
			pSelectionGroupNode = tailGroupCycleNode();
		}

		/* if (pbWrap != NULL)
			*pbWrap = true;*/ // disabled by K-Mod
	}

	if(pSelectionGroupNode == NULL)
		return NULL;

	CLLNode<int>* pFirstSelectionGroupNode = pSelectionGroupNode;
	while (true)
	{
		CvSelectionGroup* pLoopSelectionGroup = getSelectionGroup(pSelectionGroupNode->m_data);
		FAssertMsg(pLoopSelectionGroup != NULL, "LoopSelectionGroup is not assigned a valid value");

		if (pLoopSelectionGroup->readyToSelect()
				&& cycled_groups.count(pSelectionGroupNode->m_data) == 0) // K-Mod
		{
			if (!bWorkers || pLoopSelectionGroup->hasWorker())
			{
				/*if (pUnit && pLoopSelectionGroup == pUnit->getGroup()) {
					if (pbWrap != NULL)
						*pbWrap = true;
				}*/
				return pLoopSelectionGroup;
			}
		}

		if (bForward)
		{
			pSelectionGroupNode = nextGroupCycleNode(pSelectionGroupNode);

			if (pSelectionGroupNode == NULL)
			{
				pSelectionGroupNode = headGroupCycleNode();
				/* if (pbWrap != NULL)
					*pbWrap = true;*/
			}
		}
		else
		{
			pSelectionGroupNode = previousGroupCycleNode(pSelectionGroupNode);

			if (pSelectionGroupNode == NULL)
			{
				pSelectionGroupNode = tailGroupCycleNode();
				/* if (pbWrap != NULL)
					*pbWrap = true;*/
			}
		}

		if (pSelectionGroupNode == pFirstSelectionGroupNode)
		{
			// break;
			// K-Mod
			if (bWrap)
				break;
			else
			{
				cycled_groups.clear();
				bWrap = true;
			}
			// K-Mod end
		}
	} //

	return NULL;
}


bool CvPlayer::hasTrait(TraitTypes eTrait) const
{
	FAssertMsg((getLeaderType() >= 0), "getLeaderType() is less than zero");
	FAssertMsg((eTrait >= 0), "eTrait is less than zero");
	return GC.getLeaderHeadInfo(getLeaderType()).hasTrait(eTrait);
}

// AI_AUTO_PLAY_MOD, 07/09/08, jdog5000: START
void CvPlayer::setHumanDisabled(bool bNewVal)
{
	// <advc.127>
	m_bAutoPlayJustEnded = true;
	CvGame& g = GC.getGame();
	// Not sure if this is needed:
	bool const bActive = (g.getActivePlayer() == getID());
	CvWString szReplayText;
	if(bNewVal && !m_bDisableHuman) {
		AI().AI_setHumanDisabled(true);
		if(bActive) {
			// advc.004h:
			gDLL->getEngineIFace()->clearAreaBorderPlots(AREA_BORDER_LAYER_FOUNDING_BORDER);
			gDLL->getInterfaceIFace()->clearQueuedPopups();
			szReplayText = gDLL->getText("TXT_KEY_AUTO_PLAY_STARTED");
		}
	}
	else if(!bNewVal && m_bDisableHuman) {
		AI().AI_setHumanDisabled(false);
		m_iNewMessages = 0; // Don't open Event Log when coming out of Auto Play
		if(bActive)
			szReplayText = gDLL->getText("TXT_KEY_AUTO_PLAY_ENDED");
	}
	if(!szReplayText.empty()) {
		g.addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szReplayText, -1, -1,
				(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
	} // </advc.127>
	m_bDisableHuman = bNewVal;
	updateHuman();
}


bool CvPlayer::isHumanDisabled() /* advc.127: */ const
{
	return m_bDisableHuman;
}

// <advc.127>
bool CvPlayer::isSpectator() const {

	return isHumanDisabled() && GC.getGame().isDebugMode();
}


bool CvPlayer::isAutoPlayJustEnded() const {

	return m_bAutoPlayJustEnded;
} // </advc.127>
// AI_AUTO_PLAY_MOD: END

void CvPlayer::updateHuman()
{
	if (getID() == NO_PLAYER)
	{
		m_bHuman = false;
	}
	else
	{
		// AI_AUTO_PLAY_MOD, 09/01/07, MRGENIE:
		if (m_bDisableHuman)
			m_bHuman = false;
		else // AI_AUTO_PLAY_MOD: END
			m_bHuman = GC.getInitCore().getHuman(getID());
	}
}

// K-Mod
static bool concealUnknownCivs() {

	CvGame const& g = GC.getGame();
	return g.getActiveTeam() != NO_TEAM &&
			//gDLL->getChtLvl() == 0
			// advc.135c: Replacing the above (which doesn't work in multiplayer)
			!g.isDebugMode() &&
			!gDLL->GetWorldBuilderMode();
}

// <advc.106i>
void CvPlayer::setSavingReplay(bool b) {

	m_bSavingReplay = b;
} // </advc.106i>


const wchar* CvPlayer::getName(uint uiForm) const
{
	if (GC.getInitCore().getLeaderName(getID(), uiForm).empty() ||
			(GC.getGame().isMPOption(MPOPTION_ANONYMOUS) && isAlive() &&
			GC.getGame().getGameState() == GAMESTATE_ON))
	{
		//return GC.getLeaderHeadInfo(getLeaderType()).getDescription(uiForm);
		// K-Mod. Conceal the leader name of unmet players.
		if (!concealUnknownCivs() || GET_TEAM(GC.getGame().getActiveTeam()).isHasSeen(getTeam()))
			return GC.getLeaderHeadInfo(getLeaderType()).getDescription(uiForm);
		else
		{
			static CvWString string = gDLL->getText("TXT_KEY_UNKNOWN"); // hack to stop the string from going out of scope.
			return string;
		}
		// K-Mod end
	}
	else
	{	// <advc.106i>
		if(m_bSavingReplay) {
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
}

// K-Mod. Player name to be used in replay
const wchar* CvPlayer::getReplayName(uint uiForm) const
{
	if (GC.getInitCore().getLeaderName(getID(), uiForm).empty() || (GC.getGame().isMPOption(MPOPTION_ANONYMOUS) && isAlive() && GC.getGame().getGameState() == GAMESTATE_ON))
	{
		return GC.getLeaderHeadInfo(getLeaderType()).getDescription(uiForm);
	}
	else
	{
		return GC.getInitCore().getLeaderName(getID(), uiForm);
	}
}
// K-Mod end

const wchar* CvPlayer::getNameKey() const
{
	if (GC.getInitCore().getLeaderNameKey(getID()).empty() || (GC.getGame().isMPOption(MPOPTION_ANONYMOUS) && isAlive()))
	{
		return GC.getLeaderHeadInfo(getLeaderType()).getTextKeyWide();
	}
	else
	{
		return GC.getInitCore().getLeaderNameKey(getID());
	}
}


const wchar* CvPlayer::getCivilizationDescription(uint uiForm) const
{
	// K-Mod. Conceal the civilization of unmet players.
	if (concealUnknownCivs() && !GET_TEAM(GC.getGame().getActiveTeam()).isHasSeen(getTeam()))
	{
		static CvWString string = gDLL->getText("TXT_KEY_TOPCIVS_UNKNOWN"); // hack to stop the string from going out of scope.
		return string;
	}
	// K-Mod end

	if (GC.getInitCore().getCivDescription(getID(), uiForm).empty())
	{
		return GC.getCivilizationInfo(getCivilizationType()).getDescription(uiForm);
	}
	else
	{
		return GC.getInitCore().getCivDescription(getID(), uiForm);
	}
}


const wchar* CvPlayer::getCivilizationDescriptionKey() const
{
	if (GC.getInitCore().getCivDescriptionKey(getID()).empty())
	{
		return GC.getCivilizationInfo(getCivilizationType()).getTextKeyWide();
	}
	else
	{
		return GC.getInitCore().getCivDescriptionKey(getID());
	}
}


const wchar* CvPlayer::getCivilizationShortDescription(uint uiForm) const
{
	// K-Mod. Conceal the civilization of unmet players.
	if (concealUnknownCivs() && !GET_TEAM(GC.getGame().getActiveTeam()).isHasSeen(getTeam()))
	{
		static CvWString string = gDLL->getText("TXT_KEY_UNKNOWN"); // hack to stop the string from going out of scope.
		return string;
	}
	// K-Mod end

	if (GC.getInitCore().getCivShortDesc(getID(), uiForm).empty())
	{
		return GC.getCivilizationInfo(getCivilizationType()).getShortDescription(uiForm);
	}
	else
	{
		return GC.getInitCore().getCivShortDesc(getID(), uiForm);
	}
}


const wchar* CvPlayer::getCivilizationShortDescriptionKey() const
{
	if (GC.getInitCore().getCivShortDescKey(getID()).empty())
	{
		return GC.getCivilizationInfo(getCivilizationType()).getShortDescriptionKey();
	}
	else
	{
		return GC.getInitCore().getCivShortDescKey(getID());
	}
}


const wchar* CvPlayer::getCivilizationAdjective(uint uiForm) const
{
	if (GC.getInitCore().getCivAdjective(getID(), uiForm).empty())
	{
		return GC.getCivilizationInfo(getCivilizationType()).getAdjective(uiForm);
	}
	else
	{
		return GC.getInitCore().getCivAdjective(getID(), uiForm);
	}
}

const wchar* CvPlayer::getCivilizationAdjectiveKey() const
{
	if (GC.getInitCore().getCivAdjectiveKey(getID()).empty())
	{
		return GC.getCivilizationInfo(getCivilizationType()).getAdjectiveKey();
	}
	else
	{
		return GC.getInitCore().getCivAdjectiveKey(getID());
	}
}


CvWString CvPlayer::getFlagDecal() const
{
	if (GC.getInitCore().getFlagDecal(getID()).empty())
	{
		return GC.getCivilizationInfo(getCivilizationType()).getFlagTexture();
	}
	else
	{
		return GC.getInitCore().getFlagDecal(getID());
	}
}

bool CvPlayer::isWhiteFlag() const
{
	if (GC.getInitCore().getFlagDecal(getID()).empty())
	{
		return GC.getCivilizationInfo(getCivilizationType()).getArtInfo()->isWhiteFlag();
	}
	else
	{
		return GC.getInitCore().getWhiteFlag(getID());
	}
}


const wchar* CvPlayer::getStateReligionName(uint uiForm) const
{
	return GC.getReligionInfo(getStateReligion()).getDescription(uiForm);
}

const wchar* CvPlayer::getStateReligionKey() const
{
	if (getStateReligion() != NO_RELIGION)
	{
		return GC.getReligionInfo(getStateReligion()).getTextKeyWide();
	}

	return L"TXT_KEY_MISC_NO_STATE_RELIGION";
}


const CvWString CvPlayer::getBestAttackUnitName(uint uiForm) const
{
	return gDLL->getObjectText((CvString)getBestAttackUnitKey(), uiForm, true);
}


const CvWString CvPlayer::getWorstEnemyName() const
{
	TeamTypes eWorstEnemy;

	eWorstEnemy = GET_TEAM(getTeam()).AI_getWorstEnemy();

	if (eWorstEnemy != NO_TEAM)
	{
		return GET_TEAM(eWorstEnemy).getName();
	}

	return "";
}

const wchar* CvPlayer::getBestAttackUnitKey() const
{	// advc.079: Code moved into subroutine
	UnitTypes eBestUnit = AI().AI_getBestAttackUnit();

	if (eBestUnit != NO_UNIT)
	{
		return GC.getUnitInfo(eBestUnit).getTextKeyWide();
	}

	return L"TXT_KEY_MISC_NO_UNIT";
}

ArtStyleTypes CvPlayer::getArtStyleType() const
{
	if (GC.getInitCore().getArtStyle(getID()) == NO_ARTSTYLE)
	{
		return ((ArtStyleTypes)(GC.getCivilizationInfo(getCivilizationType()).getArtStyleType()));
	}
	else
	{
		return GC.getInitCore().getArtStyle(getID());
	}
}

const TCHAR* CvPlayer::getUnitButton(UnitTypes eUnit) const
{
	return GC.getUnitInfo(eUnit).getArtInfo(0, getCurrentEra(), (UnitArtStyleTypes) GC.getCivilizationInfo(getCivilizationType()).getUnitArtStyleType())->getButton();
}

/*  advc (comment): Contains the entire sequence of an AI turn, and is called
	when a human player ends his/her turn. */
void CvPlayer::doTurn()  // advc.003: style changes
{
	PROFILE_FUNC();

	FAssertMsg(isAlive(), "isAlive is expected to be true");
	FAssertMsg(!hasBusyUnit() || GC.getGame().isMPOption(MPOPTION_SIMULTANEOUS_TURNS)  || GC.getGame().isSimultaneousTeamTurns(), "End of turn with busy units in a sequential-turn game");
	CvGame& g = GC.getGame();
	/* <advc.106b> Can't figure out from within CvGame whether it's an AI turn,
	   need assistance from CvPlayer. */
	if (!g.isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
		g.setInBetweenTurns(true);
	if(isHuman() && //getStartOfTurnMessageLimit() >= 0 && // The message should be helpful even if the log doesn't auto-open
			g.getElapsedGameTurns() > 0 && !m_listGameMessages.empty()) {
		gDLL->getInterfaceIFace()->addHumanMessage(getID(), false, 0,
				gDLL->getText("TXT_KEY_END_TURN_MSG"), 0, MESSAGE_TYPE_EOT, 0,
				(ColorTypes)GC.getInfoTypeForString("COLOR_LIGHT_GREY"));
	}
	if(isHuman())
		m_iNewMessages = 0;
	/*  This way, NewMessages is never reset for non-humans. It is reset in
		setHumanDisabled though, i.e. when coming out of AI Auto Play. */
	if(isHuman())
		gDLL->getInterfaceIFace()->clearEventMessages();
	// </advc.106b>

	CvEventReporter::getInstance().beginPlayerTurn(g.getGameTurn(), getID());
	/*  advc.127: Only needs to be true when Civ4lerts are checked, which is done
		in response to beginPlayerTurn. */
	m_bAutoPlayJustEnded = false;

	doUpdateCacheOnTurn();
	g.verifyDeals();
	AI_doTurnPre();

	if (getRevolutionTimer() > 0)
		changeRevolutionTimer(-1);
	if (getConversionTimer() > 0)
		changeConversionTimer(-1);
	setConscriptCount(0);

	AI_assignWorkingPlots();

	//if (0 == GET_TEAM(getTeam()).getHasMetCivCount(true) || g.isOption(GAMEOPTION_NO_ESPIONAGE))
	// K-Mod.
	if (isCommerceFlexible(COMMERCE_ESPIONAGE) &&
			(GET_TEAM(getTeam()).getHasMetCivCount(true) == 0 ||
			g.isOption(GAMEOPTION_NO_ESPIONAGE))) //
		setCommercePercent(COMMERCE_ESPIONAGE, 0); // (note: not forced)

	verifyGoldCommercePercent();
	doGold();
	doResearch();
	doEspionagePoints();

	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		pLoopCity->doTurn();

	if (getGoldenAgeTurns() > 0)
		changeGoldenAgeTurns(-1);
	if (getAnarchyTurns() > 0)
		changeAnarchyTurns(-1);
	verifyCivics();
	doChangeCivicsPopup(NO_CIVIC); // advc.004x
	//verifyStateReligion(); // dlph.10: disabled for now

	updateTradeRoutes();
	updateWarWearinessPercentAnger();
	// <advc.011>
	if(GC.getDELAY_UNTIL_BUILD_DECAY() > 0)
		decayBuildProgress();
	// </advc.011>
	doEvents();
	/* advc.136a: Moved here from CvTeam::doTurn. (CvPlayer::doTurn happens at the
	   end of a human turn, CvTeam::doTurn at the beginning.) Doing it in
	   CvPlot::setRevealed would be nice, but possibly too slow - game already tends
	   to hang for a moment after map trades. */
	GET_TEAM(getTeam()).testCircumnavigated();
	// <advc.029>
	int foo=-1;
	for(CvUnit* u = firstUnit(&foo); u != NULL; u = nextUnit(&foo))
		u->doTurnPost();
	// </advc.029>
	// <advc.004l>
	for(CvSelectionGroup* gr = firstSelectionGroup(&foo); gr != NULL; gr = nextSelectionGroup(&foo))
		gr->doTurnPost();
	// </advc.004l>
	/*  <advc.034> Cancel disengagement agreements at the end of a round, i.e.
		at the end of the Barbarian turn. */
	int iDisengageLength = GC.getDefineINT("DISENGAGE_LENGTH");
	if(isBarbarian() && iDisengageLength > 0) {
		for(CvDeal* d = g.firstDeal(&foo); d != NULL; d = g.nextDeal(&foo)) {
			if(d->isDisengage() && d->turnsToCancel() <= 1) {
				/*  Still one turn to cancel, but that turn is practically over.
					Set to 0 turns to cancel so that the deal-canceled message
					says "0 turns". */
				d->setInitialGameTurn(d->getInitialGameTurn() - 1);
				d->kill();
			}
		}
	}
	else if(iDisengageLength < 0) { // See GlobalDefines_advc.xml about this
		CvTeam& kOurTeam = GET_TEAM(getTeam());
		for(int i = 0; i < MAX_CIV_TEAMS; i++) {
			TeamTypes tId = (TeamTypes)i;
			if(kOurTeam.isDisengage(tId))
				kOurTeam.cancelDisengage(tId);
		}
	} // </advc.034>
	/*  advc.074: Make sure (if only for performance) that we don't keep expecting
		a cancel-trade popup that never comes (the importer may have died) */
	m_cancelingExport.clear();
	int iGameTurn = g.getGameTurn();
	updateEconomyHistory(iGameTurn, calculateTotalCommerce());
	updateIndustryHistory(iGameTurn, calculateTotalYield(YIELD_PRODUCTION));
	updateAgricultureHistory(iGameTurn, calculateTotalYield(YIELD_FOOD));
	updatePowerHistory(iGameTurn, getPower());
	updateCultureHistory(iGameTurn, countTotalCulture());
	updateEspionageHistory(iGameTurn, GET_TEAM(getTeam()).getEspionagePointsEver());
	expireMessages();  // turn log

	showForeignPromoGlow(false); // advc.002e: To match call in doWarnings
	gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);

	AI_doTurnPost();
	// <advc.700>
	if(g.isOption(GAMEOPTION_RISE_FALL))
		g.getRiseFall().atTurnEnd(getID()); // </advc.700>

	if (g.isDebugMode()) // BETTER_BTS_AI_MOD, Debug, 07/08/09, jdog5000
		g.updateColoredPlots();

	CvEventReporter::getInstance().endPlayerTurn(iGameTurn, getID());
}


void CvPlayer::doTurnUnits()
{
	PROFILE_FUNC();

	int iLoop;

	AI_doTurnUnitsPre();

	for(CvSelectionGroup* pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup != NULL; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		pLoopSelectionGroup->doDelayedDeath();
	}

	for (int iPass = 0; iPass < 4; iPass++)
	{
		for (CvSelectionGroup* pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup != NULL; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
		{
			switch (pLoopSelectionGroup->getDomainType())
			{
			case DOMAIN_AIR:
				if (iPass == 1)
				{
					pLoopSelectionGroup->doTurn();
				}
				break;
			case DOMAIN_SEA:
				if (iPass == 2)
				{
					pLoopSelectionGroup->doTurn();
				}
				break;
			case DOMAIN_LAND:
				if (iPass == 3)
				{
					pLoopSelectionGroup->doTurn();
				}
				break;
			case DOMAIN_IMMOBILE:
				if (iPass == 0)
				{
					pLoopSelectionGroup->doTurn();
				}
				break;
			case NO_DOMAIN:
				FAssertMsg(NULL == pLoopSelectionGroup->getHeadUnit(), "Unit with no Domain");
			default:
				if (iPass == 3)
				{
					pLoopSelectionGroup->doTurn();
				}
				break;
			}
		}
	}

	// K-Mod. (currently unused)
	/*for (CvSelectionGroup* pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup != NULL; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
		updateGroupCycle(pLoopSelectionGroup);*/
	// K-Mod end

	if (getID() == GC.getGame().getActivePlayer())
	{
		gDLL->getFAStarIFace()->ForceReset(&GC.getInterfacePathFinder());

		gDLL->getInterfaceIFace()->setDirty(Waypoints_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
	}

	gDLL->getInterfaceIFace()->setDirty(UnitInfo_DIRTY_BIT, true);

	AI_doTurnUnitsPost();
}


void CvPlayer::verifyCivics()
{
	int iI, iJ;

	if (!isAnarchy())
	{
		for (iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
		{
			if (!canDoCivics(getCivics((CivicOptionTypes)iI)))
			{
				for (iJ = 0; iJ < GC.getNumCivicInfos(); iJ++)
				{
					if (GC.getCivicInfo((CivicTypes)iJ).getCivicOptionType() == iI)
					{
						if (canDoCivics((CivicTypes)iJ))
						{
							setCivics(((CivicOptionTypes)iI), ((CivicTypes)iJ));
							break;
						}
					}
				}
			}
		}
	}
}

// <dlph.10>
void CvPlayer::verifyStateReligion() {

	if(!isAnarchy() && !canDoReligion(getStateReligion()))
		setLastStateReligion(NO_RELIGION);
} // </dlph.10>

// <advc.064d>
void CvPlayer::verifyCityProduction() {

	int foo;
	for(CvCity* c = firstCity(&foo); c != NULL; c = nextCity(&foo))
		c->verifyProduction();
} // </advc.064d>


void CvPlayer::updatePlotGroups()
{
	PROFILE_FUNC();

	if(!GC.getGame().isFinalInitialized())
		return;

	int iLoop;
	for(CvPlotGroup* pLoopPlotGroup = pLoopPlotGroup = firstPlotGroup(&iLoop);
			pLoopPlotGroup != NULL; pLoopPlotGroup = nextPlotGroup(&iLoop))
		pLoopPlotGroup->recalculatePlots();

	for(int iI = 0; iI < GC.getMap().numPlots(); iI++)
		GC.getMap().plotByIndex(iI)->updatePlotGroup(getID(), false);

	updateTradeRoutes();
}


void CvPlayer::updateYield()
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->updateYield();
	}
}


void CvPlayer::updateMaintenance()
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->updateMaintenance();
	}
}


void CvPlayer::updatePowerHealth()
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->updatePowerHealth();
	}
}


void CvPlayer::updateExtraBuildingHappiness()
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->updateExtraBuildingHappiness();
	}
}


void CvPlayer::updateExtraBuildingHealth()
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->updateExtraBuildingHealth();
	}
}


void CvPlayer::updateFeatureHappiness()
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->updateFeatureHappiness();
	}
}


void CvPlayer::updateReligionHappiness()
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->updateReligionHappiness();
	}
}


void CvPlayer::updateExtraSpecialistYield()
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->updateExtraSpecialistYield();
	}
}


void CvPlayer::updateCommerce(CommerceTypes eCommerce)
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->updateCommerce(eCommerce);
	}
}


void CvPlayer::updateCommerce()
{
	PROFILE_FUNC(); // K-Mod
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->updateCommerce();
	}
}


void CvPlayer::updateBuildingCommerce()
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->updateBuildingCommerce();
	}
}


void CvPlayer::updateReligionCommerce()
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->updateReligionCommerce();
	}
}


void CvPlayer::updateCorporation()
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->updateCorporation();
	}
}

// advc.003j: Vanilla Civ 4 function that was, apparently, never used.
/*void CvPlayer::updateCityPlotYield()
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->plot()->updateYield();
	}
}*/


void CvPlayer::updateCitySight(bool bIncrement, bool bUpdatePlotGroups)
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->plot()->updateSight(bIncrement, bUpdatePlotGroups);
	}
}


void CvPlayer::updateTradeRoutes()
{
	PROFILE_FUNC();

	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->clearTradeRoutes();
	}

	CLinkList<int> cityList;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
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
			else
			{
				pCityNode = cityList.next(pCityNode);
			}
		}

		if (pCityNode == NULL)
		{
			cityList.insertAtEnd(pLoopCity->getID());
		}
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
	int iLoop;
	for (CvUnit* pLoopUnit = firstUnit(&iLoop); NULL != pLoopUnit; pLoopUnit = nextUnit(&iLoop))
	{
		if (pLoopUnit->isBlockading())
		{
			pLoopUnit->updatePlunder(iChange, bUpdatePlotGroups);
		}
	}
}

void CvPlayer::updateTimers()
{
	CvSelectionGroup* pLoopSelectionGroup;
	int iLoop;

	for(pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		pLoopSelectionGroup->updateTimers(); // could destroy the selection group...
	}

	// if a unit was busy, perhaps it was not quite deleted yet, give it one more try
	if (getNumSelectionGroups() > getNumUnits())
	{
		for(pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
		{
			pLoopSelectionGroup->doDelayedDeath(); // could destroy the selection group...
		}
	}

	FAssertMsg(getNumSelectionGroups() <= getNumUnits(), "The number of Units is expected not to exceed the number of Selection Groups");
}


bool CvPlayer::hasReadyUnit(bool bAny) const
{
	PROFILE_FUNC();

	int iLoop;
	for(CvSelectionGroup* pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup->readyToMove(bAny) &&
				!pLoopSelectionGroup->isAutomated()) // K-Mod
			return true;
	}
	return false;
}


bool CvPlayer::hasAutoUnit() const
{
	PROFILE_FUNC();

	int iLoop;
	for(CvSelectionGroup* pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup->readyToAuto())
			return true;
	}
	return false;
}


bool CvPlayer::hasBusyUnit() const
{
	//PROFILE_FUNC();
	int iLoop;
	for(CvSelectionGroup* pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup->isBusy())
		{
			/* original bts code
			if (pLoopSelectionGroup->getNumUnits() == 0) {
				pLoopSelectionGroup->kill();
				return false;
			} */ // disabled by K-Mod. isBusy returns false if there are no units in the group.
			return true;
		}
	}
	return false;
}

void CvPlayer::chooseTech(int iDiscover, CvWString szText, bool bFront)
{
	// K-mod
	FAssert(isHuman());
	if (iDiscover > 0)
		changeChoosingFreeTechCount(1); // note: if iDiscover is > 1, this function will be called again with iDiscover-=1
	// K-Mod end

	CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_CHOOSETECH);
	if (NULL != pInfo)
	{
		pInfo->setData1(iDiscover);
		pInfo->setText(szText);
		gDLL->getInterfaceIFace()->addPopup(pInfo, getID(), false, bFront);
	}
}


int CvPlayer::calculateScore(bool bFinal, bool bVictory) const
{
	PROFILE_FUNC();

	if (!isAlive())
	{
		return 0;
	}

	if (GET_TEAM(getTeam()).getNumMembers() == 0)
	{
		return 0;
	}

	long lScore = 0;

	CyArgsList argsList;
	argsList.add((int) getID());
	argsList.add(bFinal);
	argsList.add(bVictory);
	gDLL->getPythonIFace()->callFunction(PYGameModule, "calculateScore", argsList.makeFunctionArgs(), &lScore);
	// <advc.707>
	if(GC.getGame().isOption(GAMEOPTION_RISE_FALL) && bFinal)
		return GC.getGame().getRiseFall().getNormalizedFinalScore();
	// </advc.707>
	return ((int)lScore);
}


int CvPlayer::findBestFoundValue() const
{
	int iBestValue = 0;
	int iLoop=-1;
	for(CvArea* pLoopArea = GC.getMap().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMap().nextArea(&iLoop))
	{
		int iValue = pLoopArea->getBestFoundValue(getID());

		if (iValue > iBestValue)
		{
			iBestValue = iValue;
		}
	}

	return iBestValue;
}


int CvPlayer::upgradeAllPrice(UnitTypes eUpgradeUnit, UnitTypes eFromUnit) const
{
	int iPrice = 0;
	int iLoop;
	for (CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		if (pLoopUnit->getUnitType() == eFromUnit)
		{
			if (pLoopUnit->canUpgrade(eUpgradeUnit, true))
			{
				iPrice += pLoopUnit->upgradePrice(eUpgradeUnit);
			}
		}
	}

	return iPrice;
}

// <advc.080> Based on upgradeAllPrice
int CvPlayer::upgradeAllXPChange(UnitTypes eUpgradeUnit, UnitTypes eFromUnit) const {

	int r = 0; int foo=-1;
	for(CvUnit* u = firstUnit(&foo); u != NULL; u = nextUnit(&foo)) {
		if(u->getUnitType() == eFromUnit && u->canUpgrade(eUpgradeUnit, true))
			r += u->upgradeXPChange(eUpgradeUnit);
	}
	return r;
} // </advc.080>

int CvPlayer::countReligionSpreadUnits(CvArea* pArea, ReligionTypes eReligion, bool bIncludeTraining) const
{
	PROFILE_FUNC();

	int iCount = 0;
	int iLoop;
	for (CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		if (pLoopUnit->getArea() == pArea->getID())
		{
			if (pLoopUnit->getUnitInfo().getReligionSpreads(eReligion) > 0)
			{
				iCount++;
			}
		}
	}

	// bbai
	if (bIncludeTraining)
	{
		for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			UnitTypes eUnit = pLoopCity->getProductionUnit();

			if (eUnit != NO_UNIT)
			{
				if (GC.getUnitInfo(eUnit).getReligionSpreads(eReligion) > 0)
				{
					iCount++;
				}
			}
		}
	}
	// bbai end

	return iCount;
}

int CvPlayer::countCorporationSpreadUnits(CvArea* pArea, CorporationTypes eCorporation, bool bIncludeTraining) const
{
	PROFILE_FUNC();

	int iCount = 0;
	int iLoop;
	for (CvUnit* pLoopUnit = firstUnit(&iLoop); NULL != pLoopUnit; pLoopUnit = nextUnit(&iLoop))
	{
		//if (pLoopUnit->area() == pArea)
		if (pArea == NULL || pLoopUnit->area() == pArea) // K-Mod
		{
			if (pLoopUnit->getUnitInfo().getCorporationSpreads(eCorporation) > 0)
			{
				++iCount;
			}
		}
	}

	// bbai
	if (bIncludeTraining)
	{
		for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			if (pArea == NULL || pLoopCity->area() == pArea) // K-Mod
			{
				UnitTypes eUnit = pLoopCity->getProductionUnit();

				if (eUnit != NO_UNIT)
				{
					if(GC.getUnitInfo(eUnit).getCorporationSpreads(eCorporation) > 0)
					{
						iCount++;
					}
				}
			}
		}
	}
	// bbai end

	return iCount;
}

int CvPlayer::countNumCoastalCities() const
{
	int iCount = 0;
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (pLoopCity->isCoastal())
		{
			iCount++;
		}
	}

	return iCount;
}


int CvPlayer::countNumCoastalCitiesByArea(CvArea* pArea) const
{
	int iCount = 0;
	int iAreaID = pArea->getID();
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (pLoopCity->isCoastal())
		{
			if ((pLoopCity->getArea() == iAreaID) || pLoopCity->plot()->isAdjacentToArea(iAreaID))
			{
				iCount++;
			}
		}
	}

	return iCount;
}


int CvPlayer::countTotalCulture() const
{
	int iCount = 0;
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iCount += pLoopCity->getCultureTimes100(getID());
	}

	return iCount/100;
}


int CvPlayer::countOwnedBonuses(BonusTypes eBonus) const
{
	PROFILE("CvPlayer::countOwnedBonuses");

	// K-Mod. Shortcut.
	if (!GET_TEAM(getTeam()).isBonusRevealed(eBonus))
		return 0;
	// K-Mod end

	bool bAdvancedStart = (getAdvancedStartPoints() >= 0) && (getCurrentEra() < 3);

	int iCount = 0;

	//count bonuses outside city radius
	for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(iI);

		if ((pLoopPlot->getOwner() == getID()) && !pLoopPlot->isCityRadius())
		{
			if (pLoopPlot->getBonusType(getTeam()) == eBonus)
			{
				iCount++;
			}
		}
		else if (bAdvancedStart && pLoopPlot->isRevealed(getTeam(), false))
		{
			if (pLoopPlot->getBonusType(getTeam()) == eBonus)
			{
				iCount++;
			}
		}
	}



	//count bonuses inside city radius or easily claimed
	int iLoop=-1;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iCount += pLoopCity->AI_countNumBonuses(eBonus, true, pLoopCity->getCommerceRate(COMMERCE_CULTURE) > 0, -1);
	}

	return iCount;
}


int CvPlayer::countCityFeatures(FeatureTypes eFeature) const
{
	PROFILE_FUNC();

	int iCount = 0;
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			CvPlot* pLoopPlot = plotCity(pLoopCity->getX(), pLoopCity->getY(), iI);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->getFeatureType() == eFeature)
				{
					iCount++;
				}
			}
		}
	}

	return iCount;
}


int CvPlayer::countNumBuildings(BuildingTypes eBuilding) const
{
	PROFILE_FUNC();

	int iCount = 0;
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (pLoopCity->getNumBuilding(eBuilding) > 0)
		{
			iCount += pLoopCity->getNumBuilding(eBuilding);
		}
	}

	return iCount;
}


int CvPlayer::countNumCitiesConnectedToCapital() const
{
	int iCount = 0;
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (pLoopCity->isConnectedToCapital())
		{
			iCount++;
		}
	}

	return iCount;
}

// K-Mod
bool CvPlayer::doesImprovementConnectBonus(ImprovementTypes eImprovement, BonusTypes eBonus) const
{
	return GET_TEAM(getTeam()).doesImprovementConnectBonus(eImprovement, eBonus);
}
// K-Mod end


bool CvPlayer::canContact(PlayerTypes ePlayer) const
{
	if (ePlayer == getID())
	{
		return false;
	}

	if (!isAlive() || !(GET_PLAYER(ePlayer).isAlive()))
	{
		return false;
	}

	if (isBarbarian() || GET_PLAYER(ePlayer).isBarbarian())
	{
		return false;
	}

	if (isMinorCiv() || GET_PLAYER(ePlayer).isMinorCiv())
	{
		return false;
	}

	if (getTeam() != TEAMID(ePlayer))
	{
		if (!GET_TEAM(getTeam()).isHasMet(TEAMID(ePlayer)))
		{
			return false;
		}

		if (::atWar(getTeam(), TEAMID(ePlayer)))
		{
			if (!GET_TEAM(getTeam()).canChangeWarPeace(TEAMID(ePlayer)))
			{
				return false;
			}
		}

		if (isHuman() || GET_PLAYER(ePlayer).isHuman())
		{
			if (GC.getGame().isOption(GAMEOPTION_ALWAYS_WAR))
			{
				return false;
			}
		}
	}

	return true;
}

// K-Mod
bool CvPlayer::canContactAndTalk(PlayerTypes ePlayer) const
{
	return canContact(ePlayer) && AI_isWillingToTalk(ePlayer) && GET_PLAYER(ePlayer).AI_isWillingToTalk(getID());
}
// K-Mod end

void CvPlayer::contact(PlayerTypes ePlayer)
{
	if (!canContact(ePlayer) || isTurnDone())
	{
		return;
	}

	if (GET_PLAYER(ePlayer).isHuman())
	{
		if (GC.getGame().isPbem() || GC.getGame().isHotSeat() || (GC.getGame().isPitboss() && !gDLL->isConnected(GET_PLAYER(ePlayer).getNetID())))
		{
			if (gDLL->isMPDiplomacy())
			{
				gDLL->beginMPDiplomacy(ePlayer, false, false);
			}
		}
		else
		{
			if (gDLL->getInterfaceIFace()->isFlashing(ePlayer))
			{
				if (!gDLL->getInterfaceIFace()->isDiplomacyLocked())
				{
					gDLL->getInterfaceIFace()->setDiplomacyLocked(true);
					gDLL->sendContactCiv(NETCONTACT_RESPONSE, ePlayer);
				}
			}
			else
			{
				gDLL->sendContactCiv(NETCONTACT_INITIAL, ePlayer);
			}
		}
	}
	else
	{
		CvDiploParameters* pDiplo = new CvDiploParameters(ePlayer);
		FAssert(pDiplo != NULL);
		if (GC.ctrlKey())
		{
			pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_TRADING"));
		}
		gDLL->getInterfaceIFace()->setDiploQueue(pDiplo, GC.getGame().getActivePlayer());
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
		forcePeace(ePlayer);
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
		/*  advc.003 (comment): This event (and its counterpart REJECTED_DEMAND)
			is only triggered when a human accepts an AI demand. When a human
			demands sth., DIPLOEVENT_MADE_DEMAND triggers (and that one doesn't
			trigger when the AI demands sth.). */
		forcePeace(ePlayer);
		/*  advc.104m (comment): Sadly, we can't reliably identify the CvDeal object
			here that corresponds to the tribute demand. Otherwise, that deal could
			be marked as a tribute deal and be canceled automatically after 10 turns.
			Getting the necessary data from CvPlayerAI::AI_demandTribute to here is
			awkward. Tbd.: Could CvGame::handleDiplomacySetAIComment help with this? */
		break;

	case DIPLOEVENT_REJECTED_DEMAND:
		FAssertMsg(GET_PLAYER(ePlayer).getTeam() != getTeam(), "shouldn't call this function on our own team");
		// advc.130j:
		AI().AI_rememberEvent(ePlayer, MEMORY_REJECTED_DEMAND);
		if (AI_demandRebukedSneak(ePlayer))
		{
			GET_TEAM(getTeam()).AI_setWarPlan(GET_PLAYER(ePlayer).getTeam(), WARPLAN_PREPARING_LIMITED);
		}
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

	case DIPLOEVENT_REVOLUTION: {
		AI_changeMemoryCount(ePlayer, MEMORY_ACCEPTED_CIVIC, 1);

		CivicTypes* paeNewCivics = new CivicTypes[GC.getNumCivicOptionInfos()];
		for (int iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
		{
			paeNewCivics[iI] = GET_PLAYER(ePlayer).getCivics((CivicOptionTypes)iI);
		}

		FAssertMsg(GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic() != NO_CIVIC, "getFavoriteCivic() must be valid");

		paeNewCivics[GC.getCivicInfo((CivicTypes)(GC.getLeaderHeadInfo(getPersonalityType())).getFavoriteCivic()).getCivicOptionType()] = ((CivicTypes)(GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic()));

		GET_PLAYER(ePlayer).revolution(paeNewCivics, true);

		SAFE_DELETE_ARRAY(paeNewCivics);
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
		TEAMREF(ePlayer).declareWar((TeamTypes)iData1, false, WARPLAN_DOGPILE,
				true, getID()); // advc.100
		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			CvPlayerAI& kAttacked = GET_PLAYER((PlayerTypes)iI);
			if(!kAttacked.isAlive() || kAttacked.getTeam() != (TeamTypes)iData1)
				continue; // advc.003
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
		if(!TEAMREF(ePlayer).isCapitulated() || !TEAMREF(ePlayer).isVassal(getTeam()))
			stopTradingWithTeam((TeamTypes)iData1, false); // </advc.130f>
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == ((TeamTypes)iData1))
				{   // advc.130j:
					GET_PLAYER((PlayerTypes)iI).AI_rememberEvent(getID(), MEMORY_HIRED_TRADE_EMBARGO);
				}
			}
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
		if(getTeam() != TEAMID(ePlayer)) { // advc.155: Handled in AI_considerOffer
			// <advc.130j>
			int iDemandRecentMem = AI_getMemoryCount(ePlayer, MEMORY_MADE_DEMAND_RECENT);
			if(iDemandRecentMem <= 0)
				AI().AI_rememberEvent(ePlayer, MEMORY_MADE_DEMAND_RECENT);
			// Only remember it half if already remembered as recent, and cap at 2.
			else if(iDemandRecentMem < 2)
				AI_changeMemoryCount(ePlayer, MEMORY_MADE_DEMAND_RECENT, 1);
		} // </advc.130o> </advc.130j>
		// <advc.144>
		if(iData1 > 0) { // Let proxy AI remember when a human request is granted
			GET_PLAYER(ePlayer).AI_rememberEvent(getID(), // advc.130j
					eDiploEvent == DIPLOEVENT_ASK_HELP ?
					MEMORY_GIVE_HELP : MEMORY_ACCEPT_DEMAND);
		} // </advc.144>
		break;

	case DIPLOEVENT_MADE_DEMAND_VASSAL:
		break;

	case DIPLOEVENT_RESEARCH_TECH:
		pushResearch(((TechTypes)iData1), true);
		break;

	case DIPLOEVENT_TARGET_CITY: {
		CvCity* pCity = GET_PLAYER((PlayerTypes)iData1).getCity(iData2);
		if (pCity != NULL)
		{
			pCity->area()->setTargetCity(getID(), pCity);
			AI().AI_setCityTargetTimer(GC.getPEACE_TREATY_LENGTH()); // K-Mod
		}
		break;
	} // K-Mod
	case DIPLOEVENT_SET_WARPLAN:
	{
		CvTeamAI& kOurTeam = GET_TEAM(getTeam());
		FAssert(kOurTeam.getAtWarCount(true) == 0);
		if (iData1 == NO_TEAM)
		{
			FAssert(iData2 == NO_WARPLAN);
			for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
			{
				if (!kOurTeam.isAtWar(i))
					kOurTeam.AI_setWarPlan(i, NO_WARPLAN, false);
			}
		}
		else
			kOurTeam.AI_setWarPlan((TeamTypes)iData1, (WarPlanTypes)iData2, false);
		break;
	} // K-Mod end

	default:
		FAssert(false);
		break;
	}
}

// <advc.122> Body of the original canTradeWith moved into canTradeWithBulk.
bool CvPlayer::canTradeWith(PlayerTypes eWhoTo) const {

	if(!canTradeWith_bulk(eWhoTo))
		return false;
	CLinkList<TradeData> items;
	// Speed is not an issue; canTradeWith is only called during human-AI diplo.
	buildTradeTable(eWhoTo, items);
	if(items.getLength() > 0)
		return true;
	items.clear();
	GET_PLAYER(eWhoTo).buildTradeTable(getID(), items);
	return (items.getLength() > 0);
}

bool CvPlayer::canTradeWith_bulk(PlayerTypes eWhoTo) const // </advc.122>
{
	if (atWar(getTeam(), GET_PLAYER(eWhoTo).getTeam()))
	{
		return true;
	}

	if (GET_TEAM(getTeam()).isTechTrading() || GET_TEAM(GET_PLAYER(eWhoTo).getTeam()).isTechTrading())
	{
		return true;
	}

	if (GET_TEAM(getTeam()).isGoldTrading() || GET_TEAM(GET_PLAYER(eWhoTo).getTeam()).isGoldTrading())
	{
		return true;
	}

	if (GET_TEAM(getTeam()).isMapTrading() || GET_TEAM(GET_PLAYER(eWhoTo).getTeam()).isMapTrading())
	{
		return true;
	}

	if (canTradeNetworkWith(eWhoTo))
	{
		return true;
	}

	if (GET_TEAM(getTeam()).isOpenBordersTrading() || GET_TEAM(GET_PLAYER(eWhoTo).getTeam()).isOpenBordersTrading())
	{
		return true;
	}

	if (GET_TEAM(getTeam()).isDefensivePactTrading() || GET_TEAM(GET_PLAYER(eWhoTo).getTeam()).isDefensivePactTrading())
	{
		return true;
	}

	if (GET_TEAM(getTeam()).isPermanentAllianceTrading() || GET_TEAM(GET_PLAYER(eWhoTo).getTeam()).isPermanentAllianceTrading())
	{
		return true;
	}

	if (GET_TEAM(getTeam()).isVassalStateTrading() || GET_TEAM(GET_PLAYER(eWhoTo).getTeam()).isVassalStateTrading())
	{
		return true;
	}

	return false;
}

bool CvPlayer::canReceiveTradeCity() const
{
	if (GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
	{
		return false;
	}

	return true;
}

bool CvPlayer::canTradeItem(PlayerTypes eWhoTo, TradeData item, bool bTestDenial) const
{	// <advc.003>
	if (bTestDenial && getTradeDenial(eWhoTo, item) != NO_DENIAL)
		return false;
	CvTeam const& ourTeam = GET_TEAM(getTeam());
	CvTeam const& theirTeam = TEAMREF(eWhoTo); // </advc.003>
	switch (item.m_eItemType)
	{
	case TRADE_TECHNOLOGIES:
		if (!GC.getGame().isOption(GAMEOPTION_NO_TECH_TRADING))
		{
			if (GC.getTechInfo((TechTypes)item.m_iData).isTrade())
			{
				if (ourTeam.isHasTech((TechTypes)item.m_iData) && !ourTeam.isNoTradeTech((TechTypes)(item.m_iData)))
				{
					if (!theirTeam.isHasTech((TechTypes)item.m_iData))
					{
						//if (GET_PLAYER(eWhoTo).isHuman() || (GET_PLAYER(eWhoTo).getCurrentResearch() != item.m_iData))
						{
							if (ourTeam.isTechTrading() || theirTeam.isTechTrading())
							{
								FAssertMsg(item.m_iData >= 0, "item.m_iData is expected to be non-negative (invalid Index)");

								if (GET_PLAYER(eWhoTo).canResearch(((TechTypes)item.m_iData), true))
								{
									return true;
								}
							}
						}
					}
				}
			}
		}
		break;

	case TRADE_RESOURCES:
		FAssertMsg(item.m_iData > -1, "iData is expected to be non-negative");

		if (canTradeNetworkWith(eWhoTo))
		{
			if (!theirTeam.isBonusObsolete((BonusTypes) item.m_iData) && !ourTeam.isBonusObsolete((BonusTypes) item.m_iData))
			{
				bool const bCanTradeAll = true; // advc.036
						//(isHuman() || getTeam() == GET_PLAYER(eWhoTo).getTeam() || ourTeam.isVassal(theirTeam.getID()));
				if (getNumTradeableBonuses((BonusTypes) item.m_iData) > (bCanTradeAll ? 0 : 1))
				{
					// if (GET_PLAYER(eWhoTo).getNumAvailableBonuses(eBonus) == 0)
					{
						return true;
					}
				}
			}
		}
		break;

	case TRADE_CITIES:
		{
			CvCity* pCityTraded = getCity(item.m_iData);

			if (pCityTraded != NULL && pCityTraded->getLiberationPlayer(
					false, /* advc.122: */ getTeam())
					== eWhoTo)
				return true;
			// <advc.003>
			if(!GET_PLAYER(eWhoTo).canReceiveTradeCity() ||
					GC.getGame().getMaxCityElimination() > 0)
				break; // </advc.003>
			// <advc.122>
			if(pCityTraded == NULL || pCityTraded->isCapital() ||
					!pCityTraded->isRevealed(theirTeam.getID(), false))
				break;
			// Can't trade so long as the previous owner hasn't accepted the loss
			int const iNumPrevOwners = 1;
			bool bValid = true;
			PlayerTypes aePrevOwners[iNumPrevOwners] = {
					pCityTraded->getPreviousOwner(),
					// Let's ignore the original owner
					//pCityTraded->getOriginalOwner(),
			};
			for(int i = 0; i < iNumPrevOwners; i++) {
				if(aePrevOwners[i] != NO_PLAYER && TEAMID(aePrevOwners[i]) != TEAMID(eWhoTo) &&
						// advc.134a: Avoid calling CvTeam::isAtWar in canTradeItem
						::atWar(TEAMID(aePrevOwners[i]), getTeam()) &&
						!::atWar(TEAMID(aePrevOwners[i]),TEAMID(eWhoTo))) {
					bValid = false;
					break;
				}
			}
			if(!bValid)
				break;
			CvPlot const& kCityPlot = *pCityTraded->plot();
			// Can't trade an endangered city to a third party
			if(!::atWar(TEAMID(eWhoTo), getTeam()) && // advc.134a: Not: CvTeam::isAtWar
					kCityPlot.isVisibleEnemyCityAttacker(getID()))
				break;
			if(kCityPlot.calculateCulturePercent(eWhoTo) < GC.getCITY_TRADE_CULTURE_THRESH())
				break;
			// The BtS condition:
			if ((!ourTeam.isAVassal() && !theirTeam.isVassal(getTeam())) ||
					// Alternative condition:
					(theirTeam.isVassal(getTeam()) &&
					// Tile culture, not city culture.
					kCityPlot.getCulture(eWhoTo) > kCityPlot.getCulture(getID())))
				return true;
			// </advc.122>
		}
		break;

	case TRADE_GOLD:
		if (ourTeam.isGoldTrading() || theirTeam.isGoldTrading())
		{
			if (getGold() >= item.m_iData)
			{
				return true;
			}
		}
		break;

	case TRADE_GOLD_PER_TURN:
		if (ourTeam.isGoldTrading() || theirTeam.isGoldTrading())
		{
			return true;
		}
		break;

	case TRADE_MAPS:
		if (getTeam() != theirTeam.getID())
		{
			if (ourTeam.isMapTrading() || theirTeam.isMapTrading())
			{
				return true;
			}
		}
		break;

	case TRADE_VASSAL:
	case TRADE_SURRENDER:
		if (!isHuman() || GET_PLAYER(eWhoTo).isHuman() ||
				GC.getBBAI_HUMAN_AS_VASSAL_OPTION()) // BETTER_BTS_AI_MOD, Customization, 12/06/09, jdog5000
		{
			CvTeam& kVassalTeam = GET_TEAM(getTeam());
			CvTeam& kMasterTeam = GET_TEAM(GET_PLAYER(eWhoTo).getTeam());
			if (kMasterTeam.isVassalStateTrading()) // the master must possess the tech
			{
				if (!kVassalTeam.isAVassal() && !kMasterTeam.isAVassal() && getTeam() != GET_PLAYER(eWhoTo).getTeam())
				{	// advc.134a: Avoid calling CvTeam::isAtWar in canTradeItem
					bool bWar = ::atWar(kMasterTeam.getID(), getTeam());
					if ((bWar || item.m_iData == 1) && item.m_eItemType == TRADE_SURRENDER)
					{
						return true;
					}
					if (!bWar && item.m_eItemType == TRADE_VASSAL)
					{
						return true;
					}
				}
			}
		}
		break;

	case TRADE_PEACE:
		if (!ourTeam.isHuman())
		{
			if (!ourTeam.isAVassal())
			{
				if (ourTeam.isHasMet((TeamTypes)item.m_iData) &&
						theirTeam.isHasMet((TeamTypes)item.m_iData))
				{
					if (::atWar(getTeam(), (TeamTypes)item.m_iData))
					{
						return true;
					}
				}
			}
		}
		break;

	case TRADE_WAR:
		if (!ourTeam.isHuman())
		{
			if (!ourTeam.isAVassal())
			{
				if (!GET_TEAM((TeamTypes)item.m_iData).isAVassal())
				{
					if (ourTeam.isHasMet((TeamTypes)item.m_iData) &&
							theirTeam.isHasMet((TeamTypes)item.m_iData))
					{
						if (ourTeam.canDeclareWar((TeamTypes)item.m_iData))
						{
							//return true;
							// advc.100: Replacing the above (advc.134a: Don't call CvTeam::isAtWar)
							return !::atWar(ourTeam.getID(), theirTeam.getID());
						}
					}
				}
			}
		}
		break;

	case TRADE_EMBARGO: {
		if(ourTeam.isHuman()) // advc.003
			return false;
		TeamTypes eEmbargoTarget = (TeamTypes)(item.m_iData);
		if (ourTeam.isHasMet(eEmbargoTarget) && theirTeam.isHasMet(eEmbargoTarget))
		{
			if (canStopTradingWithTeam(eEmbargoTarget)
					// <advc.130f>
					&& (!GET_PLAYER(eWhoTo).isTradingWithTeam(eEmbargoTarget, true) ||
					(ourTeam.isCapitulated() && ourTeam.isVassal(theirTeam.getID()))))
					// </advc.130f>
				return true;
		}
		break;
	}
	case TRADE_CIVIC:
		//if (!ourTeam.isHuman()
				// UNOFFICIAL_PATCH, Diplomacy, 10/22/09, denev & jdog5000
				//|| getTeam() == theirTeam.getID())
		// <advc.155> Replacing the patched code above
		if((getTeam() == theirTeam.getID()) ?
				(!isHuman() || GET_PLAYER(eWhoTo).isHuman()) :
				/*  Don't allow humans from one team to manipulate the civics of
					AI civs on another human team. */
				!ourTeam.isHuman()) // </advc.155>
		{
			if (GET_PLAYER(eWhoTo).isCivic((CivicTypes)item.m_iData)
				/*  <advc.132> canForceCivic double-checks everything checked here
					already, plus some clauses that I've added there. */
					|| ((TEAMREF(getID()).isVassal(theirTeam.getID()) ||
					::atWar(getTeam(), theirTeam.getID())) &&
					GET_PLAYER(eWhoTo).canForceCivics(getID(),
					(CivicTypes)item.m_iData))) // </advc.132>
			{
				if (canDoCivics((CivicTypes)(item.m_iData)) && !isCivic((CivicTypes)(item.m_iData)))
				{
					if (canRevolution(NULL))
						return true;
				}
			}
		}
		break;

	case TRADE_RELIGION:
		//if (!ourTeam.isHuman()
				// UNOFFICIAL_PATCH, Diplomacy, 10/22/09, denev & jdog5000
				//|| getTeam() == theirTeam.getID())
		// <advc.155> Replacing the patched code above
		if((getTeam() == theirTeam.getID()) ?
				(!isHuman() || GET_PLAYER(eWhoTo).isHuman()) :
				!ourTeam.isHuman()) // </advc.155>
		{
			if (GET_PLAYER(eWhoTo).getStateReligion() == ((ReligionTypes)item.m_iData)
					// <advc.132> Same thing as for civics above
					|| ((TEAMREF(getID()).isVassal(theirTeam.getID())
					|| ::atWar(getTeam(), theirTeam.getID()))
					&& GET_PLAYER(eWhoTo).canForceReligion(getID(),
					(ReligionTypes)item.m_iData))) // </advc.132>
			{
				if (canConvert((ReligionTypes)(item.m_iData)))
					return true;
			}
		}
		break;

	case TRADE_OPEN_BORDERS:
		if (getTeam() != theirTeam.getID())
		{
			if (!::atWar(getTeam(), theirTeam.getID()))
			{
				if (!ourTeam.isOpenBorders(theirTeam.getID()))
				{
					if (ourTeam.isOpenBordersTrading() || theirTeam.isOpenBordersTrading())
					{
						return true;
					}
				}
			}
		}
		break;

	case TRADE_DEFENSIVE_PACT:
		if (!ourTeam.isAVassal() && !theirTeam.isAVassal())
		{
			if (getTeam() != theirTeam.getID() && !theirTeam.isVassal(getTeam()))
			{
				if (!::atWar(getTeam(), theirTeam.getID()))
				{
					if (!ourTeam.isDefensivePact(theirTeam.getID()))
					{
						if (ourTeam.isDefensivePactTrading() || theirTeam.isDefensivePactTrading())
						{
							/*  <dlph.3> 'Added possibility of signing defensive pact while
								in war if BBAI defensive pact option is >= 1' */
							if (((ourTeam.getAtWarCount(true) == 0) &&
									(theirTeam.getAtWarCount(true) == 0)) ||
									(GC.getBBAI_DEFENSIVE_PACT_BEHAVIOR() >= 1
									/*  advc: Prohibit DP when not all wars shared?
										Enough to have the AI refuse such pacts I think
										(in CvTeamAI::AI_defensivePactTrade). */
									//&& TEAMREF(getID()).allWarsShared(theirTeam.getID())
									)) // </dlph.3>
							{
								if (ourTeam.canSignDefensivePact(theirTeam.getID()))
									return true;
							}
						}
					}
				}
			}
		}
		break;

	case TRADE_PERMANENT_ALLIANCE:
		if (!ourTeam.isAVassal() && !theirTeam.isAVassal())
		{
			if (getTeam() != theirTeam.getID() && !theirTeam.isVassal(getTeam()))
			{
				if (!::atWar(getTeam(), theirTeam.getID()))
				{
					if (ourTeam.isPermanentAllianceTrading() || theirTeam.isPermanentAllianceTrading())
					{
						if (ourTeam.getNumMembers() == 1 && theirTeam.getNumMembers() == 1)
						{
							return true;
						}
					}
				}
			}
		}
		break;

	case TRADE_PEACE_TREATY:
		return true;
		break;
	// <advc.034>
	case TRADE_DISENGAGE:
		return  !theirTeam.isDisengage(getTeam()) &&
				!theirTeam.isAtWar(getTeam()) &&
				!theirTeam.isOpenBorders(getTeam()) &&
				!theirTeam.isVassal(getTeam()) &&
				!ourTeam.isVassal(theirTeam.getID()) &&
				theirTeam.isHasMet(getTeam());
		break; // </advc.034>
	}

	return false;
}


DenialTypes CvPlayer::getTradeDenial(PlayerTypes eWhoTo, TradeData item) const
{
	CvCity* pCity;
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
		return kOurTeam.AI_techTrade(((TechTypes)(item.m_iData)), GET_PLAYER(eWhoTo).getTeam());
		break;

	case TRADE_RESOURCES:
		return AI_bonusTrade(((BonusTypes)(item.m_iData)), eWhoTo,
				1); // advc.036
		break;

	case TRADE_CITIES:
		pCity = getCity(item.m_iData);
		if (pCity != NULL)
		{
			return AI_cityTrade(pCity, eWhoTo);
		}
		break;

	case TRADE_GOLD:
	case TRADE_GOLD_PER_TURN:
		break;

	case TRADE_MAPS:
		return kOurTeam.AI_mapTrade(GET_PLAYER(eWhoTo).getTeam());
		break;

	case TRADE_SURRENDER:
		return kOurTeam.AI_surrenderTrade(GET_PLAYER(eWhoTo).getTeam(),
				// advc.104o: No functional change
				CvTeamAI::VASSAL_POWER_MOD_SURRENDER);
		break;

	case TRADE_VASSAL:
		// K-Mod
		if (!isHuman() && kOurTeam.isHuman())
			return DENIAL_MYSTERY;
		// K-Mod end
		return kOurTeam.AI_vassalTrade(GET_PLAYER(eWhoTo).getTeam());
		break;

	case TRADE_PEACE:
		return kOurTeam.AI_makePeaceTrade(((TeamTypes)(item.m_iData)), GET_PLAYER(eWhoTo).getTeam());
		break;

	case TRADE_WAR:
		return kOurTeam.AI_declareWarTrade(((TeamTypes)(item.m_iData)), GET_PLAYER(eWhoTo).getTeam());
		break;

	case TRADE_EMBARGO:
		return AI_stopTradingTrade(((TeamTypes)(item.m_iData)), eWhoTo);
		break;

	case TRADE_CIVIC:
		return AI_civicTrade(((CivicTypes)(item.m_iData)), eWhoTo);
		break;

	case TRADE_RELIGION:
		return AI_religionTrade(((ReligionTypes)(item.m_iData)), eWhoTo);
		break;

	case TRADE_OPEN_BORDERS:
		return kOurTeam.AI_openBordersTrade(GET_PLAYER(eWhoTo).getTeam());
		break;

	case TRADE_DEFENSIVE_PACT:
		// K-Mod
		if (!isHuman() && kOurTeam.isHuman())
			return DENIAL_MYSTERY;
		// K-Mod end
		return kOurTeam.AI_defensivePactTrade(GET_PLAYER(eWhoTo).getTeam());
		break;

	case TRADE_PERMANENT_ALLIANCE:
		// K-Mod
		if (!isHuman() && kOurTeam.isHuman())
			return DENIAL_MYSTERY;
		// K-Mod end
		return kOurTeam.AI_permanentAllianceTrade(GET_PLAYER(eWhoTo).getTeam());
		break;

	case TRADE_PEACE_TREATY:
		// K-Mod
		if (kOurTeam.AI_refusePeace(GET_PLAYER(eWhoTo).getTeam()))
			return DENIAL_VICTORY;
		// K-Mod end
		break;
	/*  <advc.034> Can't be traded voluntarily anyway, but NO_DENIAL suppresses
		explanation text in CvDLLWidgetData::parseTradeItem */
	case TRADE_DISENGAGE:
		return NO_DENIAL;
		break; // <advc.034>
	}

	return NO_DENIAL;
}


bool CvPlayer::canTradeNetworkWith(PlayerTypes ePlayer) const
{
	// <advc.124>
	PROFILE_FUNC();
	CvCity* pOurCap = getCapitalCity();
	CvPlayer const& kThey = GET_PLAYER(ePlayer);
	CvCity* pTheirCap = kThey.getCapitalCity();
	if(pOurCap != NULL && pTheirCap != NULL) { int foo;
		for(CvCity* c = kThey.firstCity(&foo); c != NULL; c = kThey.nextCity(&foo))
			if(pOurCap->isConnectedTo(c) && c->isConnectedToCapital(ePlayer))
				return true;
		for(CvCity* c = firstCity(&foo); c != NULL; c = nextCity(&foo))
			if(pTheirCap->isConnectedTo(c) && c->isConnectedToCapital(getID()))
				return true;
		// Replaced BtS code:
		/*if (pOurCapitalCity->isConnectedToCapital(ePlayer))
			return true;*/
	} // </advc.124>

	return false;
}


int CvPlayer::getNumAvailableBonuses(BonusTypes eBonus) const
{
	CvPlotGroup* pPlotGroup = ((getCapitalCity() != NULL) ? getCapitalCity()->plot()->getOwnerPlotGroup() : NULL);
	if (pPlotGroup != NULL)
	{
		return pPlotGroup->getNumBonuses(eBonus);
	}

	return 0;
}


int CvPlayer::getNumTradeableBonuses(BonusTypes eBonus) const
{
	return (getNumAvailableBonuses(eBonus) - getBonusImport(eBonus));
}

bool CvPlayer::hasBonus(BonusTypes eBonus) const
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); NULL != pLoopCity; pLoopCity = nextCity(&iLoop))
	{
		if (pLoopCity->hasBonus(eBonus))
		{
			return true;
		}
	}

	return false;
}

int CvPlayer::getNumTradeBonusImports(PlayerTypes ePlayer) const
{
	FAssert(ePlayer != getID());
	CLLNode<TradeData>* pNode;
	int iCount = 0;
	int iLoop;
	for(CvDeal* pLoopDeal = GC.getGame().firstDeal(&iLoop); pLoopDeal != NULL; pLoopDeal = GC.getGame().nextDeal(&iLoop))
	{
		if ((pLoopDeal->getFirstPlayer() == getID()) && (pLoopDeal->getSecondPlayer() == ePlayer))
		{
			for (pNode = pLoopDeal->headSecondTradesNode(); (pNode != NULL); pNode = pLoopDeal->nextSecondTradesNode(pNode))
			{
				if (pNode->m_data.m_eItemType == TRADE_RESOURCES)
				{
					iCount++;
				}
			}
		}

		if ((pLoopDeal->getFirstPlayer() == ePlayer) && (pLoopDeal->getSecondPlayer() == getID()))
		{
			for (pNode = pLoopDeal->headFirstTradesNode(); (pNode != NULL); pNode = pLoopDeal->nextFirstTradesNode(pNode))
			{
				if (pNode->m_data.m_eItemType == TRADE_RESOURCES)
				{
					iCount++;
				}
			}
		}
	}

	return iCount;
}

// advc.003: Renamed parameter
bool CvPlayer::isTradingWithTeam(TeamTypes eTeam, bool bIncludeUncancelable) const
{
	if (eTeam == getTeam())
		return false;

	int iLoop;
	for (CvDeal* pLoopDeal = GC.getGame().firstDeal(&iLoop); pLoopDeal != NULL; pLoopDeal = GC.getGame().nextDeal(&iLoop))
	{
		// advc.003: Un(!)cancelable
		if (bIncludeUncancelable || pLoopDeal->isCancelable(getID()))
		{
			if (!pLoopDeal->isPeaceDeal()
					&& !pLoopDeal->isDisengage()) // advc.034
			{
				if ((pLoopDeal->getFirstPlayer() == getID()) && (GET_PLAYER(pLoopDeal->getSecondPlayer()).getTeam() == eTeam))
				{
					if (pLoopDeal->getLengthFirstTrades() > 0)
					{
						return true;
					}
				}

				if ((pLoopDeal->getSecondPlayer() == getID()) && (GET_PLAYER(pLoopDeal->getFirstPlayer()).getTeam() == eTeam))
				{
					if (pLoopDeal->getLengthSecondTrades() > 0)
					{
						return true;
					}
				}
			}
		}
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
	if(bContinueNotTrading) {
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
	int iLoop;
	for(CvDeal* pLoopDeal = GC.getGame().firstDeal(&iLoop); pLoopDeal != NULL; pLoopDeal = GC.getGame().nextDeal(&iLoop))
	{
		if (pLoopDeal->isEverCancelable(getID()) // advc.130f
				&& !pLoopDeal->isPeaceDeal()
				&& !pLoopDeal->isDisengage()) // advc.034
		{
			if ((pLoopDeal->getFirstPlayer() == getID() &&
					TEAMID(pLoopDeal->getSecondPlayer()) == eTeam) ||
					(pLoopDeal->getSecondPlayer() == getID() &&
					TEAMID(pLoopDeal->getFirstPlayer()) == eTeam))
			{
				pLoopDeal->kill();
				bDealCanceled = true; // advc.130f
			}
		}
	} // <advc.130f>
	if(!bDealCanceled && bDiploPenalty)
		return; // </advc.130f>
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++) // advc.003n: was MAX_PLAYERS
	{   CvPlayerAI& kTargetMember = GET_PLAYER((PlayerTypes)iI); // advc.003
		if (!kTargetMember.isAlive() || kTargetMember.getTeam() != eTeam)
			continue;
		// <advc.130j>
		if(bDiploPenalty) // advc.130f: RECENT causes only refusal to talk
			kTargetMember.AI_rememberEvent(getID(), MEMORY_STOPPED_TRADING);
		// Don't rememberEvent - not supposed to be based on attitude
		kTargetMember.AI_setMemoryCount(getID(), MEMORY_STOPPED_TRADING_RECENT, 2);
	} // </advc.130j>
}

// <advc.130f>
bool CvPlayer::isAnyDealTooRecentToCancel(TeamTypes eTeam) const {

	CvGame& g = GC.getGame(); int foo=-1;
	for(CvDeal* d = g.firstDeal(&foo); d != NULL; d = g.nextDeal(&foo)) {
		if(d->isBetween(getTeam(), eTeam) && !d->isCancelable(getID()) &&
				!d->isPeaceDeal() && !d->isDisengage()) // advc.034
			return true;
	}
	return false;
} // </advc.130f>

void CvPlayer::killAllDeals()
{
	int iLoop;
	for(CvDeal* pLoopDeal = GC.getGame().firstDeal(&iLoop); pLoopDeal != NULL; pLoopDeal = GC.getGame().nextDeal(&iLoop))
	{
		if (pLoopDeal->getFirstPlayer() == getID() ||
				pLoopDeal->getSecondPlayer() == getID())
			pLoopDeal->kill();
	}
}


void CvPlayer::findNewCapital()  // advc.003 style changes
{
	BuildingTypes eCapitalBuilding = (BuildingTypes)
			GC.getCivilizationInfo(getCivilizationType()).
			getCivilizationBuildings(GC.getDefineINT("CAPITAL_BUILDINGCLASS"));
	if (eCapitalBuilding == NO_BUILDING)
		return;

	CvCity* pOldCapital = getCapitalCity();

	int iBestValue = 0;
	CvCity* pBestCity = NULL; int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (pLoopCity == pOldCapital ||
				pLoopCity->getNumRealBuilding(eCapitalBuilding) != 0)
			continue;

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
		if (pOldCapital != NULL)
			pOldCapital->setNumRealBuilding(eCapitalBuilding, 0);
		FAssertMsg(!(pBestCity->getNumRealBuilding(eCapitalBuilding)), "(pBestCity->getNumRealBuilding(eCapitalBuilding)) did not return false as expected");
		pBestCity->setNumRealBuilding(eCapitalBuilding, 1);
	}
}


int CvPlayer::getNumGovernmentCenters() const
{
	int iCount = 0; int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (pLoopCity->isGovernmentCenter())
			iCount++;
	}
	return iCount;
}


bool CvPlayer::canRaze(CvCity* pCity) const
{
	if (!pCity->isAutoRaze())
	{
		if (GC.getGame().isOption(GAMEOPTION_NO_CITY_RAZING))
			return false;

		if (pCity->getOwner() != getID())
			return false;

		if (pCity->calculateTeamCulturePercent(getTeam()) >= GC.getDefineINT("RAZING_CULTURAL_PERCENT_THRESHOLD"))
			return false;
	}

	CyCity* pyCity = new CyCity(pCity);
	CyArgsList argsList; argsList.add(getID());
	argsList.add(gDLL->getPythonIFace()->makePythonObject(pyCity));
	long lResult=0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "canRazeCity", argsList.makeFunctionArgs(), &lResult);
	delete pyCity;
	if (lResult == 0)
		return (false);

	return true;
}


void CvPlayer::raze(CvCity* pCity)
{
	if (!canRaze(pCity))
		return;

	FAssert(pCity->getOwner() == getID());

	AI().AI_processRazeMemory(*pCity); // advc.003n: Moved into subroutine

	wchar szBuffer[1024];
	swprintf(szBuffer, gDLL->getText("TXT_KEY_MISC_DESTROYED_CITY", pCity->getNameKey()).GetCString());
	gDLL->getInterfaceIFace()->addHumanMessage(getID(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_CITYRAZE", MESSAGE_TYPE_MAJOR_EVENT, ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_CITY_EDIT")->getPath(), (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), pCity->getX(), pCity->getY(), true, true);

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer const& kObs = GET_PLAYER((PlayerTypes)iI); // advc.003
		if (!kObs.isAlive() || iI == getID())
			continue;
		if (pCity->isRevealed(kObs.getTeam(), false)
				|| kObs.isSpectator()) // advc.127
		{
			swprintf(szBuffer, gDLL->getText("TXT_KEY_MISC_CITY_HAS_BEEN_RAZED_BY", pCity->getNameKey(), getCivilizationDescriptionKey()).GetCString());
			gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_CITYRAZED",
					MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY, // advc.106b
					ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_CITY_EDIT")->getPath(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pCity->getX(), pCity->getY(), true, true);
		}
	}

	swprintf(szBuffer, gDLL->getText("TXT_KEY_MISC_CITY_RAZED_BY", pCity->getNameKey(), getCivilizationDescriptionKey()).GetCString());
	GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szBuffer, pCity->getX(), pCity->getY(), (ColorTypes)GC.getInfoTypeForString("COLOR_WARNING_TEXT"));

	// Report this event
	CvEventReporter::getInstance().cityRazed(pCity, getID());
	CvPlot const& kCityPlot = *pCity->plot(); // advc.130w
	disband(pCity);
	AI().AI_updateCityAttitude(kCityPlot); // advc.130w
}


void CvPlayer::disband(CvCity* pCity)
{
	if (getNumCities() == 1)
	{
		setFoundedFirstCity(false);
	}

	GC.getGame().addDestroyedCityName(pCity->getName());

	pCity->kill(true);
}


bool CvPlayer::canReceiveGoody(CvPlot* pPlot, GoodyTypes eGoody, CvUnit* pUnit) const
{
	CvGoodyInfo const& goody = GC.getGoodyInfo(eGoody); // advc.003
	// <advc.314>
	CvGame const& g = GC.getGame();
	int const iTrainHalved = (GC.getGameSpeedInfo(g.getGameSpeedType()).
			getTrainPercent() + 100) / 2;
	bool bVeryEarlyGame = (100 * g.getGameTurn() < 20 * iTrainHalved);
	bool bVeryVeryEarlyGame = (100 * g.getGameTurn() < 10 * iTrainHalved);
	if (goody.getExperience() > 0)
	{
		if (pUnit == NULL || !pUnit->canAcquirePromotionAny() ||
				//(GC.getGame().getElapsedGameTurns() < 10)
				bVeryVeryEarlyGame) // advc.314
			return false;
	}

	if (goody.getDamagePrereq() > 0)
	{
		if (pUnit == NULL || pUnit->getDamage() < (pUnit->maxHitPoints() *
				goody.getDamagePrereq()) / 100)
			return false;
	} // advc.314:
	bool bAnyGold = (goody.getGold() + goody.getGoldRand1() + goody.getGoldRand2() > 0);
	if (goody.isTech())
	{
		bool bTechFound = false;
		for (int iI = 0; iI < GC.getNumTechInfos(); iI++)
		{	// advc.114: Replacing the conditions below
			if(isGoodyTech((TechTypes)iI, bAnyGold))
			{
				/*if (GC.getTechInfo((TechTypes) iI).isGoodyTech()) {
					//if (canResearch((TechTypes)iI), false, true)*/ // K-Mod
				if (canResearch((TechTypes)iI, false, true)) // advc.003
				{
					bTechFound = true;
					break;
				}
			}
		}

		if (!bTechFound)
			return false;
	}

	if (goody.isBad())
	{
		if (pUnit == NULL || pUnit->isNoBadGoodies())
			return false;
	}
	// <advc.314>
	if(bVeryEarlyGame && goody.getMinBarbarians() > 1)
		return false;
	/*  Moved up and added the era clause; a single free unit in the Medieval
		era isn't going to be a problem. */
	bool bEarlyMP = (GC.getGame().isGameMultiPlayer() &&
			GC.getGame().getCurrentEra() < 2);
	// No free unit from Spy when hut guarded
	if(!goody.isBad() && (goody.getUnitClassType() != NO_UNITCLASS ||
		goody.getMinBarbarians() > 0) && pPlot->isVisibleEnemyUnit(getID()) &&
			pUnit != NULL && pUnit->isSpy())
		return false;
	if (goody.getUnitClassType() != NO_UNITCLASS)
	{
		UnitTypes eUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(goody.getUnitClassType())));
		if (eUnit == NO_UNIT)
			return false;
		CvUnitInfo const& u = GC.getUnitInfo(eUnit); // advc.003
		if (u.getCombat() > 0 && /* advc.315: */ !u.isMostlyDefensive())
		{
			if (bEarlyMP || // advc.314
					//GC.getGame().getElapsedGameTurns() < 20
					// advc.314: Replacing the above
					bVeryEarlyGame)
				return false;
		} // <advc.314> I guess a Worker with a slow WorkRate would be OK
		if(bVeryEarlyGame && u.getWorkRate() > 30)
			return false; // </advc.314>
		if (GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
		{
			if (u.isFound())
				return false;
		}
	} // <advc.314> Free unit and no UnitClassType given
	else if(!goody.isBad() && goody.getMinBarbarians() > 0 &&
			(bEarlyMP || bVeryEarlyGame))
		return false; // </advc.314>
	if ((goody.getBarbarianUnitClass() != NO_UNITCLASS
			// <advc.314> Hostile unit w/o any UnitClassType given
			|| (goody.getMinBarbarians() > 0 && goody.getBarbarianUnitProb() > 0))
			// BarbarianUnitClass has a different use now when !isBad
			&& goody.isBad()) // </advc.314>
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
	if(pUnit != NULL && pUnit->isNoBadGoodies() && AI_getNumAIUnits(UNITAI_EXPLORE) >= 2) {
		UnitClassTypes eUnitClass = (UnitClassTypes)goody.getUnitClassType();
		if(eUnitClass != NO_UNITCLASS) {
			UnitTypes eUnit = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).
					getCivilizationUnits(eUnitClass);
			if(eUnit != NO_UNIT && GC.getUnitInfo(eUnit).isNoBadGoodies())
				return false;
		}
	} // </advc.315d>
	// <advc.315e> No map reveal at the edges of a non-wrapping map
	if(goody.getMapProb() > 0) {
		int const iRange = 3;
		for(int dx = -iRange; dx <= iRange; dx++) {
			for(int dy = -iRange; dy <= iRange; dy++) {
				CvPlot* pLoopPlot = ::plotXY(pPlot->getX(), pPlot->getY(), dx, dy);
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

	FAssertMsg(canReceiveGoody(pPlot, eGoody, pUnit), "Instance is expected to be able to receive goody");
	// <advc.003>
	CvGoodyInfo const& goody = GC.getGoodyInfo(eGoody);
	CvGame& g = GC.getGame();
	// </advc.003>
	szBuffer = goody.getDescription();

	// <advc.314>
	double prUpgrade = (goody.isBad() ? 0 : goody.getBarbarianUnitProb()  / 100.0);
	/*  Coefficient chosen such that it's 100% on turn 150. Would be better to
		program that computation and set the 100%-turn in XML ... */
	prUpgrade *= (0.225 * (g.goodyHutEffectFactor(false) - 1));
	/*  Meaning that an upgraded version of 'goody' should be used, or, if there
		is none, that an additional outcome should be rolled. */
	bool bUpgrade = ::bernoulliSuccess(prUpgrade, "advc.314");
	// </advc.314>
	int iGold = goody.getGold() + g.getSorenRandNum(goody.getGoldRand1(), "Goody Gold 1") + g.getSorenRandNum(goody.getGoldRand2(), "Goody Gold 2");
	//iGold  = (iGold * GC.getGameSpeedInfo(g.getGameSpeedType()).getGrowthPercent()) / 100;
	// advc.314: Replacing the above
	iGold = (int)(iGold * g.goodyHutEffectFactor());
	if (iGold != 0
			// advc.314: isTech means that iGold is the research progress
			&& !goody.isTech())
	{
		changeGold(iGold);
		szBuffer += gDLL->getText("TXT_KEY_MISC_RECEIVED_GOLD", iGold);
	}
	/*  advc.134: (Moved the addHumanMessage down b/c some of the handlers
		have custom messages) */

	int iRange = goody.getMapRange();
	if (iRange > 0)
	{
		int iOffset = goody.getMapOffset();
		CvPlot* pBestPlot = NULL;
		if (iOffset > 0)
		{
			int iBestValue = 0;
			for (int iDX = -(iOffset); iDX <= iOffset; iDX++)
			{
				for (int iDY = -(iOffset); iDY <= iOffset; iDY++)
				{
					CvPlot* pLoopPlot = plotXY(pPlot->getX(), pPlot->getY(), iDX, iDY);
					if(pLoopPlot == NULL || pLoopPlot->isRevealed(getTeam(), false))
						continue; // advc.003
					int iValue = (1 + g.getSorenRandNum(10000, "Goody Map"));
					iValue *= plotDistance(pPlot->getX(), pPlot->getY(), pLoopPlot->getX(), pLoopPlot->getY());
					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestPlot = pLoopPlot;
					}
				}
			}
		}

		if (pBestPlot == NULL)
			pBestPlot = pPlot;

		for (int iDX = -(iRange); iDX <= iRange; iDX++)
		{
			for (int iDY = -(iRange); iDY <= iRange; iDY++)
			{
				CvPlot* pLoopPlot = plotXY(pBestPlot->getX(), pBestPlot->getY(), iDX, iDY);

				if (pLoopPlot != NULL)
				{
					if (plotDistance(pBestPlot->getX(), pBestPlot->getY(), pLoopPlot->getX(), pLoopPlot->getY()) <= iRange)
					{
						if (g.getSorenRandNum(100, "Goody Map") < goody.getMapProb())
						{
							pLoopPlot->setRevealed(getTeam(), true, false, NO_TEAM, true);
						}
					}
				}
			}
		}
	}

	if (pUnit != NULL)
	{
		pUnit->changeExperience(goody.getExperience());
	}

	if (pUnit != NULL)
	{
		pUnit->changeDamage(-(goody.getHealing()));
	}

	if (goody.isTech())
	{
		int iBestValue = 0;
		TechTypes eBestTech = NO_TECH;

		for (int iI = 0; iI < GC.getNumTechInfos(); iI++)
		{	// advc.314: Replacing the conditions below
			if(isGoodyTech((TechTypes)iI, iGold > 0))
			/*if (GC.getTechInfo((TechTypes) iI).isGoodyTech()) {
				//if (canResearch((TechTypes)iI), false, true) { // K-Mod */
			{
				int iValue = (1 + g.getSorenRandNum(10000, "Goody Tech"));
				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					eBestTech = ((TechTypes)iI);
				}
			}
		}
		FAssertMsg(eBestTech != NO_TECH, "BestTech is not assigned a valid value");
		// <advc.314> Most of the code from here on is modified
		CvTeam& kOurTeam = GET_TEAM(getTeam());
		if(iGold <= 0 || 0.8 * kOurTeam.getResearchLeft(eBestTech) <= iGold) {
			kOurTeam.setHasTech(eBestTech, true, getID(), true, true);
			if(isSignificantDiscovery(eBestTech))
				kOurTeam.setNoTradeTech(eBestTech, true);
		}
		else {
			kOurTeam.changeResearchProgress(eBestTech, iGold, getID());
			szBuffer = gDLL->getText("TXT_KEY_MISC_PROGRESS_TOWARDS_TECH", iGold,
					GC.getTechInfo(eBestTech).getDescription());
		}
	}
	std::vector<UnitTypes> aeBestUnits;
	/*  When units need to be placed, but no unit class is given.
		Times 3 b/c MinBarbarians is only a lower bound. */
	for(int i = 0; i < 3 * goody.getMinBarbarians(); i++) {
		UnitTypes eBestUnit = NO_UNIT;
		if(goody.getUnitClassType() == NO_UNITCLASS &&
			goody.getBarbarianUnitClass() == NO_UNITCLASS) {
			int iBestValue = 0;
			for(int j = 0; j < GC.getNumUnitClassInfos(); j++) {
				UnitTypes eUnit = (UnitTypes)GC.getCivilizationInfo(
						GET_PLAYER(BARBARIAN_PLAYER).getCivilizationType()).
						getCivilizationUnits(j);
				if(eUnit == NO_UNIT)
					continue;
				CvUnitInfo const& u = GC.getUnitInfo(eUnit);
				if(u.getDomainType() != DOMAIN_LAND)
					continue;
				if(u.getPrereqOrBonuses(0) == NO_BONUS &&
						u.getPrereqAndBonus() == NO_BONUS &&
						u.getCombat() > 0 &&
						(u.getPrereqAndTech() == NO_TECH || // pre-Industrial:
						GC.getTechInfo((TechTypes)u.getPrereqAndTech()).getEra() <= 3) &&
						GET_PLAYER(BARBARIAN_PLAYER).canTrain(eUnit, false, true)) {
					int iValue = u.getCombat() + (goody.isBad() ?
							// Randomize hostile units a bit
							g.getSorenRandNum(10, "advc.314") : 0);
					if(iValue > iBestValue) {
						iBestValue = iValue;
						eBestUnit = eUnit;
					}
				}
			}
			FAssert(eBestUnit != NO_UNIT);
			aeBestUnits.push_back(eBestUnit);
		}
	}
	if (goody.getUnitClassType() != NO_UNITCLASS
			// Pick a unit based on barb techs then
			|| (!goody.isBad() && goody.getMinBarbarians() > 0)) {
		UnitTypes eUnit = NO_UNIT; // Declaration moved down
		UnitClassTypes eUnitClass = (UnitClassTypes)goody.getUnitClassType();
		if(eUnitClass != NO_UNITCLASS) {
			// Interpret BarbarianUnitClass as an upgrade to replace UnitClassType
			if(bUpgrade && goody.getBarbarianUnitClass() != NO_UNITCLASS) {
				eUnitClass = (UnitClassTypes)goody.getBarbarianUnitClass();
				// Upgrade applied, don't roll an additional outcome.
				bNoRecursion = true;
			}
			eUnit = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).
					getCivilizationUnits(eUnitClass);
		}
		/*  Let MinBarbarians > 1 generate more than 1 unit
			(though I'm not using this so far; not tested either) */
		for(int i = 0; i < std::max(1, goody.getMinBarbarians()); i++) {
			UnitTypes eLoopUnit = eUnit;
			if(eLoopUnit == NO_UNIT && !aeBestUnits.empty())
				eLoopUnit = aeBestUnits[std::min((int)aeBestUnits.size() - 1, i)];
			FAssert(eLoopUnit != NO_UNIT);
			if(eLoopUnit != NO_UNIT) {
				CvUnit* pNewUnit = initUnit(eLoopUnit, pPlot->getX(), pPlot->getY());
				if(pNewUnit == NULL)
					continue;
				else FAssert(pNewUnit != NULL);
				szBuffer = gDLL->getText("TXT_KEY_GOODY_FREE_UNIT",
						GC.getUnitInfo(eLoopUnit).getDescription());
				addGoodyMsg(szBuffer, *pPlot, goody.getSound());
				szBuffer.clear();
				if(pNewUnit->canAcquirePromotionAny())
					promoteFreeUnit(*pNewUnit, prUpgrade);
			}
		}
	}
	if(!szBuffer.empty()) // Moved from higher up
		addGoodyMsg(szBuffer, *pPlot, goody.getSound());
	/*  If not isBad, then BarbarianUnitClass has a different meaning, which is
		handled above. */
	if(goody.isBad() && (goody.getBarbarianUnitClass() != NO_UNITCLASS
			// Will use eBestUnit in this case
			|| (goody.getMinBarbarians() > 0 && goody.getBarbarianUnitProb() > 0))) {
		UnitTypes eUnit = NO_UNIT;
		if(goody.getBarbarianUnitClass() != NO_UNITCLASS) {
			eUnit = (UnitTypes)GC.getCivilizationInfo(GET_PLAYER(BARBARIAN_PLAYER).getCivilizationType()).
					getCivilizationUnits(goody.getBarbarianUnitClass());
		}
		int iMinBarbs = std::max((int)(std::sqrt(goody.getMinBarbarians() *
				g.goodyHutEffectFactor(false))), goody.getMinBarbarians());
		// Increase the probability proportional to the change to MinBarbs
		int iProb = (goody.getBarbarianUnitProb() * iMinBarbs) / goody.getMinBarbarians();
		int iMaxBarbs = iMinBarbs + 2; // upper bound for iPass=0
		int iBarbCount = 0;
		for(int iPass = 0; iPass < 2; iPass++) {
			if(iBarbCount >= iMinBarbs)
				continue;
			for(int iI = 0; iI < NUM_DIRECTION_TYPES; iI++) {
				CvPlot* pLoopPlot = plotDirection(pPlot->getX(), pPlot->getY(),
						(DirectionTypes)iI);
				if(pLoopPlot == NULL || pLoopPlot->getArea() != pPlot->getArea() ||
						pLoopPlot->isImpassable() || pLoopPlot->getNumUnits() > 0)
					continue;
				if(iPass > 0 || (g.getSorenRandNum(100, "Goody Barbs") < iProb)) {
					UnitTypes eLoopUnit = eUnit;
					if(eLoopUnit == NO_UNIT && !aeBestUnits.empty()) {
						eLoopUnit = aeBestUnits[
								std::min((int)aeBestUnits.size() - 1, iBarbCount)];
					}
					FAssert(eLoopUnit != NO_UNIT);
					if(eLoopUnit != NO_UNIT) {
						GET_PLAYER(BARBARIAN_PLAYER).initUnit(eLoopUnit,
								pLoopPlot->getX(), pLoopPlot->getY(),
								((pLoopPlot->isWater()) ? UNITAI_ATTACK_SEA :
								UNITAI_ATTACK));
						iBarbCount++;
					}
					if ((iPass > 0 && iBarbCount >= iMinBarbs) ||
							iBarbCount >= iMaxBarbs)
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
	CyPlot kGoodyPlot(pPlot);
	CyUnit kGoodyUnit(pUnit);
	CyArgsList argsList;
	argsList.add(getID());
	argsList.add(gDLL->getPythonIFace()->makePythonObject(&kGoodyPlot));
	argsList.add(gDLL->getPythonIFace()->makePythonObject(&kGoodyUnit));
	long result=0;
	bool ok = gDLL->getPythonIFace()->callFunction(PYGameModule, "doGoody", argsList.makeFunctionArgs(), &result);
	if (ok && result)
		return;

	FAssert(pPlot->isGoody() /* advc.314: */ || eTaboo != NO_GOODY);
	pPlot->removeGoody();
	// <advc.003>
	if(isBarbarian()) {
		FAssertMsg(pPlot->isOwned(), "Barbarians should remove hut only when receiving a city");
		return;
	} // </advc.003>
	for (int iI = 0; iI < GC.getDefineINT("NUM_DO_GOODY_ATTEMPTS"); iI++)
	{
		if (GC.getHandicapInfo(getHandicapType()).getNumGoodies() <= 0)
			continue; // advc.003
		GoodyTypes eGoody = (GoodyTypes)GC.getHandicapInfo(getHandicapType()).
				getGoodies(GC.getGame().getSorenRandNum(
				GC.getHandicapInfo(getHandicapType()).getNumGoodies(), "Goodies"));
		FAssert(eGoody >= 0 && eGoody < GC.getNumGoodyInfos());
		// <advc.314>
		if(eGoody == eTaboo || (eTaboo != NO_GOODY && GC.getGoodyInfo(eGoody).isBad() !=
				GC.getGoodyInfo(eTaboo).isBad())) // Don't pair a good with a bad outcome
			continue; // </advc.314>

		if (canReceiveGoody(pPlot, eGoody, pUnit))
		{
			receiveGoody(pPlot, eGoody, pUnit, /* advc.314: */ eTaboo != NO_GOODY);
			// Python Event
			CvEventReporter::getInstance().goodyReceived(getID(), pPlot, pUnit, eGoody);
			break;
		}
	}
}


bool CvPlayer::canFound(int iX, int iY, bool bTestVisible) const  // advc.003: some style changes
{
	long lResult=0;
	if(GC.getUSE_CANNOT_FOUND_CITY_CALLBACK()) {
		CyArgsList argsList;
		argsList.add((int)getID());
		argsList.add(iX);
		argsList.add(iY);
		gDLL->getPythonIFace()->callFunction(PYGameModule, "cannotFoundCity", argsList.makeFunctionArgs(), &lResult);
		if (lResult == 1)
			return false;
	}

	if (GC.getGame().isFinalInitialized() && getNumCities() > 0 &&
			GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
		return false;

	CvPlot* pPlot = GC.getMap().plot(iX, iY);
	if (pPlot->isImpassable())
		return false;

	if (pPlot->getFeatureType() != NO_FEATURE && GC.getFeatureInfo(pPlot->getFeatureType()).isNoCity())
		return false;

	if (pPlot->isOwned() && pPlot->getOwner() != getID())
		return false;

	bool bValid = false;
	//if (!bValid) // advc.003: redundant
	{
		if (GC.getTerrainInfo(pPlot->getTerrainType()).isFound())
			bValid = true;
	}

	if (!bValid)
	{
		if (GC.getTerrainInfo(pPlot->getTerrainType()).isFoundCoast() && pPlot->isCoastalLand())
			bValid = true;
	}

	if (!bValid)
	{
		if (GC.getTerrainInfo(pPlot->getTerrainType()).isFoundFreshWater() &&
				pPlot->isFreshWater())
			bValid = true;
	}
	/*  UNOFFICIAL_PATCH, Bugfix, 02/16/10, EmperorFool & jdog5000
		(canFoundCitiesOnWater callback handling was incorrect and ignored isWater() if it returned true) */
	if (pPlot->isWater() && GC.getUSE_CAN_FOUND_CITIES_ON_WATER_CALLBACK()) {
			bValid = false;
			CyArgsList argsList2;
			argsList2.add(iX);
			argsList2.add(iY);
			lResult=0;
			gDLL->getPythonIFace()->callFunction(PYGameModule, "canFoundCitiesOnWater", argsList2.makeFunctionArgs(), &lResult);
			if (lResult == 1)
				bValid = true;
	} // UNOFFICIAL_PATCH: END

	if (!bValid)
		return false;

	if (bTestVisible)
		return true;

	int iRange = GC.getMIN_CITY_RANGE();
	for (int iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (int iDY = -(iRange); iDY <= iRange; iDY++)
		{
			CvPlot* pLoopPlot	= plotXY(pPlot->getX(), pPlot->getY(), iDX, iDY);
			if (pLoopPlot == NULL)
				continue;

			if (pLoopPlot->isCity() && pLoopPlot->area() == pPlot->area())
				return false;
		}
	}
	return true;
}


void CvPlayer::found(int iX, int iY)  // advc.003: some style changes
{
	if (!canFound(iX, iY))
		return;

	CvCity* pCity = initCity(iX, iY, true, true);
	FAssertMsg(pCity != NULL, "City is not assigned a valid value");
	CvGame const& g = GC.getGame();
	int iI;
	if (isBarbarian())
	{
		UnitTypes eDefenderUnit = pCity->AI_bestUnitAI(UNITAI_CITY_DEFENSE);

		if (eDefenderUnit == NO_UNIT)
		{
			eDefenderUnit = pCity->AI_bestUnitAI(UNITAI_ATTACK);
		}

		if (eDefenderUnit != NO_UNIT)
		{
			for (iI = 0; iI < GC.getHandicapInfo(g.getHandicapType()).getBarbarianInitialDefenders(); iI++)
			{
				initUnit(eDefenderUnit, iX, iY, UNITAI_CITY_DEFENSE);
			}
		}
	}

	for (iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingTypes eLoopBuilding = ((BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI)));
		if (eLoopBuilding != NO_BUILDING)
		{
			if(g.isFreeStartEraBuilding(eLoopBuilding))
			{
				if (pCity->canConstruct(eLoopBuilding))
				{
					pCity->setNumRealBuilding(eLoopBuilding, 1);
				}
			}
		}
	}

	if (getAdvancedStartPoints() >= 0)
	{	// Free border expansion for Creative
		bool bCreative = false;
		for (iI = 0; iI < GC.getNumTraitInfos(); ++iI)
		{
			if (hasTrait((TraitTypes)iI))
			{
				if (GC.getTraitInfo((TraitTypes)iI).getCommerceChange(COMMERCE_CULTURE) > 0)
				{
					bCreative = true;
					break;
				}

			}
		}

		if (bCreative)
		{
			for (iI = 0; iI < GC.getNumCultureLevelInfos(); ++iI)
			{
				int iCulture = g.getCultureThreshold((CultureLevelTypes)iI);
				if (iCulture > 0)
				{
					pCity->setCulture(getID(), iCulture, true, true);
					break;
				}
			}
		}
	}

	if (isHuman() && getAdvancedStartPoints() < 0)
	{
		pCity->chooseProduction(
				/*  advc.124g: Somehow, the choose-production popup comes up twice
					(despite being pushed only once) in multiplayer if the
					choose-tech popup is handled before choose-production. */
				NO_UNIT, NO_BUILDING, NO_PROJECT, false, g.isNetworkMultiPlayer());
	}
	else pCity->doFoundMessage();
	// <advc.210c>
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayer const& kObs = GET_PLAYER((PlayerTypes)i);
		if(kObs.isAlive() && pCity->getOwner() != kObs.getID() &&
				pCity->isRevealed(kObs.getTeam(), false)) {
			gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(), false,
					GC.getEVENT_MESSAGE_TIME(),
					gDLL->getText("TXT_KEY_MORECIV4LERTS_CITY_FOUNDED",
					getNameKey(), pCity->getNameKey()), 0, MESSAGE_TYPE_INFO,
					ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_CITY_EDIT")->
					getPath(), (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"),
					pCity->getX(), pCity->getY(), true, true);

		}
	} // </advc.210c>
	if(!CvPlot::isAllFog()) // advc.706: Suppress name-city popup
		CvEventReporter::getInstance().cityBuilt(pCity);
	if (gPlayerLogLevel >= 1) // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000:
		logBBAI("  Player %d (%S) founds new city %S at %d, %d", getID(), getCivilizationDescription(0), pCity->getName(0).GetCString(), iX, iY);
}


bool CvPlayer::canTrain(UnitTypes eUnit, bool bContinue, bool bTestVisible, bool bIgnoreCost) const
{
	//PROFILE_FUNC(); // advc.003o
	UnitClassTypes eUnitClass = (UnitClassTypes)(GC.getUnitInfo(eUnit).getUnitClassType());

	// K-Mod note. This assert can fail if team games when checking whether this city can upgrade a unit to one of our team member's UUs.
	//FAssert(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(eUnitClass) == eUnit);
	if (GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(eUnitClass) != eUnit)
	{
		return false;
	}

	if (!bIgnoreCost)
	{
		if (GC.getUnitInfo(eUnit).getProductionCost() == -1)
		{
			return false;
		}
	}

	if (GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
	{
		if (GC.getUnitInfo(eUnit).isFound())
		{
			return false;
		}
	}

	if (GC.getGame().isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		if (GC.getUnitInfo(eUnit).isSpy() || GC.getUnitInfo(eUnit).getEspionagePoints() > 0)
		{
			return false;
		}
	}

	if (!(GET_TEAM(getTeam()).isHasTech((TechTypes)(GC.getUnitInfo(eUnit).getPrereqAndTech()))))
	{
		return false;
	}

	for (int iI = 0; iI < GC.getNUM_UNIT_AND_TECH_PREREQS(); iI++)
	{
		if (GC.getUnitInfo(eUnit).getPrereqAndTechs(iI) != NO_TECH)
		{
			if (!(GET_TEAM(getTeam()).isHasTech((TechTypes)(GC.getUnitInfo(eUnit).getPrereqAndTechs(iI)))))
			{
				return false;
			}
		}
	}

	if (GC.getUnitInfo(eUnit).getStateReligion() != NO_RELIGION)
	{
		if (getStateReligion() != GC.getUnitInfo(eUnit).getStateReligion())
		{
			return false;
		}
	}

	if (GC.getGame().isUnitClassMaxedOut(eUnitClass))
	{
		return false;
	}

	/* original bts code
	if (GET_TEAM(getTeam()).isUnitClassMaxedOut(eUnitClass))
		return false;
	if (isUnitClassMaxedOut(eUnitClass))
		return false;*/  // disabled by K-Mod.
	// Note that unlike the global limit, these two limits apply to the number of units currently alive rather than the total ever trained.
	// Therefore these limits should be ignored for the visibility test.

	if (!bTestVisible)
	{
		if (GC.getGame().isUnitClassMaxedOut(eUnitClass, (GET_TEAM(getTeam()).getUnitClassMaking(eUnitClass) + ((bContinue) ? -1 : 0))))
		{
			return false;
		}

		if (GET_TEAM(getTeam()).isUnitClassMaxedOut(eUnitClass, (GET_TEAM(getTeam()).getUnitClassMaking(eUnitClass) + ((bContinue) ? -1 : 0))))
		{
			return false;
		}

		if (isUnitClassMaxedOut(eUnitClass, (getUnitClassMaking(eUnitClass) + ((bContinue) ? -1 : 0))))
		{
			return false;
		}

		if (GC.getGame().isNoNukes() || !GC.getGame().isNukesValid())
		{
			if (GC.getUnitInfo(eUnit).getNukeRange() != -1)
			{
				return false;
			}
		}

		if (GC.getUnitInfo(eUnit).getSpecialUnitType() != NO_SPECIALUNIT)
		{
			if (!(GC.getGame().isSpecialUnitValid((SpecialUnitTypes)(GC.getUnitInfo(eUnit).getSpecialUnitType()))))
			{
				return false;
			}
		}
	}

	return true;
}


bool CvPlayer::canConstruct(BuildingTypes eBuilding, bool bContinue, bool bTestVisible, bool bIgnoreCost, bool bIgnoreTech) const
{
	/*  <advc.003> Global checks moved into new function. These are all fast
		(now that CivTeamsEverAlive is cached). */
	if(!GC.getGame().canConstruct(eBuilding, bIgnoreCost, bTestVisible))
		return false; // </advc.003>

	int iI;
	CvTeamAI& kOurTeam = GET_TEAM(getTeam());
	BuildingClassTypes eBuildingClass = (BuildingClassTypes)(GC.getBuildingInfo(eBuilding).getBuildingClassType());

	if (GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(eBuildingClass) != eBuilding)
	{
		return false;
	}

	if (!bIgnoreTech) // K-Mod
	{
		if (!kOurTeam.isHasTech((TechTypes) GC.getBuildingInfo(eBuilding).getPrereqAndTech()))
		{
			return false;
		}

		for (iI = 0; iI < GC.getNUM_BUILDING_AND_TECH_PREREQS(); iI++)
		{
			if (GC.getBuildingInfo(eBuilding).getPrereqAndTechs(iI) != NO_TECH)
			{
				if (!kOurTeam.isHasTech((TechTypes)GC.getBuildingInfo(eBuilding).getPrereqAndTechs(iI)))
				{
					return false;
				}
			}
		}
	}

	if (kOurTeam.isObsoleteBuilding(eBuilding))
	{
		return false;
	}

	SpecialBuildingTypes eSpecial = (SpecialBuildingTypes)GC.getBuildingInfo(eBuilding).getSpecialBuildingType(); // advc.003
	if (eSpecial != NO_SPECIALBUILDING)
	{
		if (!kOurTeam.isHasTech((TechTypes)GC.getSpecialBuildingInfo(eSpecial).getTechPrereq()))
		{
			return false;
		}
	}

	if (GC.getBuildingInfo(eBuilding).getStateReligion() != NO_RELIGION)
	{
		if (getStateReligion() != GC.getBuildingInfo(eBuilding).getStateReligion())
		{
			return false;
		}
	}

	if (GC.getBuildingInfo(eBuilding).getVictoryPrereq() != NO_VICTORY)
	{
		if (isMinorCiv())
		{
			return false;
		}

		if (kOurTeam.getVictoryCountdown((VictoryTypes)GC.getBuildingInfo(eBuilding).getVictoryPrereq()) >= 0)
		{
			return false;
		}
	}
	// <dlph.19>
	if(eBuilding == (BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).
			getCivilizationBuildings(GC.getDefineINT("CAPITAL_BUILDINGCLASS")))) {
		for(int i = 0; i < GC.getNumVictoryInfos(); i++) {
			if(kOurTeam.getVictoryCountdown((VictoryTypes)i) >= 0 &&
					GC.getGame().getGameState() == GAMESTATE_ON)
				return false;
		}
	} // </dlph.19>

	if (kOurTeam.isBuildingClassMaxedOut(eBuildingClass))
	{
		return false;
	}

	if (isBuildingClassMaxedOut(eBuildingClass))
	{
		return false;
	}

	CvCivilizationInfo &civilizationInfo = GC.getCivilizationInfo(getCivilizationType());
	int numBuildingClassInfos = GC.getNumBuildingClassInfos();
	for (iI = 0; iI < numBuildingClassInfos; iI++)
	{
		BuildingTypes ePrereqBuilding = (BuildingTypes)civilizationInfo.getCivilizationBuildings(iI);
		if (NO_BUILDING != ePrereqBuilding && kOurTeam.isObsoleteBuilding(ePrereqBuilding))
		{
			if (getBuildingClassCount((BuildingClassTypes)iI) < getBuildingClassPrereqBuilding(eBuilding, (BuildingClassTypes)iI, 0))
			{
				return false;
			}
		}
	}

	if(bTestVisible)
		return true; // advc.003

	if (GC.getGame().isBuildingClassMaxedOut(eBuildingClass,
			(kOurTeam.getBuildingClassMaking(eBuildingClass) +
			((bContinue) ? -1 : 0))))
	{
		return false;
	}

	if (kOurTeam.isBuildingClassMaxedOut(eBuildingClass,
			(kOurTeam.getBuildingClassMaking(eBuildingClass) +
			((bContinue) ? -1 : 0))))
	{
		return false;
	}

	if (isBuildingClassMaxedOut(eBuildingClass,
			(getBuildingClassMaking(eBuildingClass) + ((bContinue) ? -1 : 0))))
	{
		return false;
	}

	if (getHighestUnitLevel() < GC.getBuildingInfo(eBuilding).getUnitLevelPrereq())
	{
		return false;
	}

	for (iI = 0; iI < numBuildingClassInfos; iI++)
	{
		if (getBuildingClassCount((BuildingClassTypes)iI) < getBuildingClassPrereqBuilding(eBuilding, ((BuildingClassTypes)iI), ((bContinue) ? 0 : getBuildingClassMaking(eBuildingClass))))
		{
			return false;
		}
	}

	return true;
}


bool CvPlayer::canCreate(ProjectTypes eProject, bool bContinue, bool bTestVisible) const
{
	int iI;

	if (isBarbarian())
	{
		return false;
	}

	if (GC.getProjectInfo(eProject).getProductionCost() == -1)
	{
		return false;
	}

	if (!(GET_TEAM(getTeam()).isHasTech((TechTypes)(GC.getProjectInfo(eProject).getTechPrereq()))))
	{
		return false;
	}

	if (GC.getProjectInfo(eProject).getVictoryPrereq() != NO_VICTORY)
	{
		if (!(GC.getGame().isVictoryValid((VictoryTypes)(GC.getProjectInfo(eProject).getVictoryPrereq()))))
		{
			return false;
		}

		if (isMinorCiv())
		{
			return false;
		}

		if (GET_TEAM(getTeam()).getVictoryCountdown((VictoryTypes)GC.getProjectInfo(eProject).getVictoryPrereq()) >= 0)
		{
			return false;
		}
	}

	if (GC.getGame().isProjectMaxedOut(eProject))
	{
		return false;
	}

	if (GET_TEAM(getTeam()).isProjectMaxedOut(eProject))
	{
		return false;
	}

	if (!bTestVisible)
	{
		if (GC.getGame().isProjectMaxedOut(eProject, (GET_TEAM(getTeam()).getProjectMaking(eProject) + ((bContinue) ? -1 : 0))))
		{
			return false;
		}

		if (GET_TEAM(getTeam()).isProjectMaxedOut(eProject, (GET_TEAM(getTeam()).getProjectMaking(eProject) + ((bContinue) ? -1 : 0))))
		{
			return false;
		}

		if (GC.getGame().isNoNukes())
		{
			if (GC.getProjectInfo(eProject).isAllowsNukes())
			{
				return false; // advc.003b
				/*for (iI = 0; iI < GC.getNumUnitInfos(); iI++) {
					if (GC.getUnitInfo((UnitTypes)iI).getNukeRange() != -1)
						return false;
				}*/
			}
		}

		if (GC.getProjectInfo(eProject).getAnyoneProjectPrereq() != NO_PROJECT)
		{
			if (GC.getGame().getProjectCreatedCount((ProjectTypes)(GC.getProjectInfo(eProject).getAnyoneProjectPrereq())) == 0)
			{
				return false;
			}
		}

		for (iI = 0; iI < GC.getNumProjectInfos(); iI++)
		{
			if (GET_TEAM(getTeam()).getProjectCount((ProjectTypes)iI) < GC.getProjectInfo(eProject).getProjectsNeeded(iI))
			{
				return false;
			}
		}
	}

	return true;
}


bool CvPlayer::canMaintain(ProcessTypes eProcess, bool bContinue) const
{
	if (!GET_TEAM(getTeam()).isHasTech((TechTypes)GC.getProcessInfo(eProcess).getTechPrereq()))
	{
		return false;
	}

	return true;
}


bool CvPlayer::isProductionMaxedUnitClass(UnitClassTypes eUnitClass) const
{
	if (eUnitClass == NO_UNITCLASS)
	{
		return false;
	}

	if (GC.getGame().isUnitClassMaxedOut(eUnitClass))
	{
		return true;
	}

	if (GET_TEAM(getTeam()).isUnitClassMaxedOut(eUnitClass))
	{
		return true;
	}

	if (isUnitClassMaxedOut(eUnitClass))
	{
		return true;
	}

	return false;
}


bool CvPlayer::isProductionMaxedBuildingClass(BuildingClassTypes eBuildingClass, bool bAcquireCity) const
{
	if (eBuildingClass == NO_BUILDINGCLASS)
	{
		return false;
	}

	if (!bAcquireCity)
	{
		if (GC.getGame().isBuildingClassMaxedOut(eBuildingClass))
		{
			return true;
		}
	}

	if (GET_TEAM(getTeam()).isBuildingClassMaxedOut(eBuildingClass))
	{
		return true;
	}

	if (isBuildingClassMaxedOut(eBuildingClass, ((bAcquireCity) ? GC.getBuildingClassInfo(eBuildingClass).getExtraPlayerInstances() : 0)))
	{
		return true;
	}

	return false;
}


bool CvPlayer::isProductionMaxedProject(ProjectTypes eProject) const
{
	if (eProject == NO_PROJECT)
	{
		return false;
	}

	if (GC.getGame().isProjectMaxedOut(eProject))
	{
		return true;
	}

	if (GET_TEAM(getTeam()).isProjectMaxedOut(eProject))
	{
		return true;
	}

	return false;
}


int CvPlayer::getProductionNeeded(UnitTypes eUnit) const
{
	UnitClassTypes eUnitClass = (UnitClassTypes)GC.getUnitInfo(eUnit).getUnitClassType();
	FAssert(NO_UNITCLASS != eUnitClass);

	int iProductionNeeded = GC.getUnitInfo(eUnit).getProductionCost();

	iProductionNeeded *= 100 + getUnitClassCount(eUnitClass) * GC.getUnitClassInfo(eUnitClass).getInstanceCostModifier();
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getDefineINT("UNIT_PRODUCTION_PERCENT");
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getTrainPercent();
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getEraInfo(GC.getGame().getStartEra()).getTrainPercent();
	iProductionNeeded /= 100;
	/*  <advc.107> Code moved into auxiliary function b/c I need this modifier
		in AI_getTotalFloatingDefendersNeeded. */
	iProductionNeeded = ::roundToMultiple(iProductionNeeded *
			trainingModifierFromHandicap(isWorldUnitClass(eUnitClass)),
			isHuman() ? 5 : 1); // advc.251
	// </advc.107>
	// advc.251 (comment): See getNewCityProductionValue
	iProductionNeeded += getUnitExtraCost(eUnitClass);
	// Python cost modifier
	if(GC.getUSE_GET_UNIT_COST_MOD_CALLBACK()) {
		CyArgsList argsList;
		argsList.add(getID());
		argsList.add((int)eUnit);
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "getUnitCostMod", argsList.makeFunctionArgs(), &lResult);
		if (lResult > 1) {
			iProductionNeeded *= lResult;
			iProductionNeeded /= 100;
		}
	}

	return std::max(1, iProductionNeeded);
}


int CvPlayer::getProductionNeeded(BuildingTypes eBuilding) const
{
	CvGame const& g = GC.getGame(); // advc.003
	int iProductionNeeded = GC.getBuildingInfo(eBuilding).getProductionCost();

	iProductionNeeded *= GC.getDefineINT("BUILDING_PRODUCTION_PERCENT");
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getGameSpeedInfo(g.getGameSpeedType()).getConstructPercent();
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getEraInfo(g.getStartEra()).getConstructPercent();
	iProductionNeeded /= 100;
	// <advc.251>
	iProductionNeeded = ::roundToMultiple(0.01 * iProductionNeeded *
			GC.getHandicapInfo(getHandicapType()).getConstructPercent(),
			isHuman() ? 5 : 1);
	if(!isHuman()) { // Barbarians too
		CvHandicapInfo const& h = GC.getHandicapInfo(g.getHandicapType());
		int iAIModifier = //h.getAIPerEraModifier() * getCurrentEra()
				g.AIHandicapAdjustment();
		if(isWorldWonderClass((BuildingClassTypes)(GC.getBuildingInfo(eBuilding).getBuildingClassType())))
			iAIModifier += h.getAIWorldConstructPercent();
		else iAIModifier += h.getAIConstructPercent();
		iProductionNeeded *= iAIModifier;
		iProductionNeeded /= 100;
	} // </advc.251>

	return std::max(1, iProductionNeeded);
}


int CvPlayer::getProductionNeeded(ProjectTypes eProject) const
{
	CvGame const& g = GC.getGame(); // advc.003
	// <advc.251>
	int const iBaseCost = GC.getProjectInfo(eProject).getProductionCost();
	int iProductionNeeded = iBaseCost; // </advc.251>

	iProductionNeeded *= GC.getDefineINT("PROJECT_PRODUCTION_PERCENT");
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getGameSpeedInfo(g.getGameSpeedType()).getCreatePercent();
	iProductionNeeded /= 100;

	iProductionNeeded *= GC.getEraInfo(g.getStartEra()).getCreatePercent();
	iProductionNeeded /= 100;
	// <advc.251>
	iProductionNeeded = ::roundToMultiple(0.01 * iProductionNeeded *
			GC.getHandicapInfo(getHandicapType()).getCreatePercent(),
			isHuman() ? (iBaseCost > 500 ? 50 : 5) : 1);
	if(!isHuman() && !isBarbarian()) {
		CvHandicapInfo const& h = GC.getHandicapInfo(g.getHandicapType());
		int iAIModifier = //h.getAIPerEraModifier() * getCurrentEra()
				g.AIHandicapAdjustment();
		if(isWorldProject(eProject))
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

	if (GC.getUnitInfo(eUnit).isMilitaryProduction())
	{
		iMultiplier += getMilitaryProductionModifier();
	}

	for (int iI = 0; iI < GC.getNumTraitInfos(); iI++)
	{
		if (hasTrait((TraitTypes)iI))
		{
			iMultiplier += GC.getUnitInfo(eUnit).getProductionTraits(iI);

			if (GC.getUnitInfo(eUnit).getSpecialUnitType() != NO_SPECIALUNIT)
			{
				iMultiplier += GC.getSpecialUnitInfo((SpecialUnitTypes) GC.getUnitInfo(eUnit).getSpecialUnitType()).getProductionTraits(iI);
			}
		}
	}

	return iMultiplier;
}

int CvPlayer::getProductionModifier(BuildingTypes eBuilding) const
{
	int iMultiplier = 0;
	for (int iI = 0; iI < GC.getNumTraitInfos(); iI++)
	{
		if (hasTrait((TraitTypes)iI))
		{
			iMultiplier += GC.getBuildingInfo(eBuilding).getProductionTraits(iI);

			if (GC.getBuildingInfo(eBuilding).getSpecialBuildingType() != NO_SPECIALBUILDING)
			{
				iMultiplier += GC.getSpecialBuildingInfo((SpecialBuildingTypes) GC.getBuildingInfo(eBuilding).getSpecialBuildingType()).getProductionTraits(iI);
			}
		}
	}

	if (::isWorldWonderClass((BuildingClassTypes)(GC.getBuildingInfo(eBuilding).getBuildingClassType())))
	{
		iMultiplier += getMaxGlobalBuildingProductionModifier();
	}

	if (::isTeamWonderClass((BuildingClassTypes)(GC.getBuildingInfo(eBuilding).getBuildingClassType())))
	{
		iMultiplier += getMaxTeamBuildingProductionModifier();
	}

	if (::isNationalWonderClass((BuildingClassTypes)(GC.getBuildingInfo(eBuilding).getBuildingClassType())))
	{
		iMultiplier += getMaxPlayerBuildingProductionModifier();
	}

	return iMultiplier;
}

int CvPlayer::getProductionModifier(ProjectTypes eProject) const
{
	int iMultiplier = 0;

	if (GC.getProjectInfo(eProject).isSpaceship())
	{
		iMultiplier += getSpaceProductionModifier();
	}

	return iMultiplier;
}

// advc.107: Cut from getProductionNeeded; refactored.
double CvPlayer::trainingModifierFromHandicap(bool bWorldClass) const {

	// <advc.251>
	int iPercent = GC.getHandicapInfo(getHandicapType()).getTrainPercent();
	if(!isHuman()) { // Apply this also to Barbarians // </advc.251>
		CvHandicapInfo& h = GC.getHandicapInfo(GC.getGame().getHandicapType());
		int iAIPercent = //h.getAIPerEraModifier() * getCurrentEra()
				GC.getGame().AIHandicapAdjustment();
		if(bWorldClass)
			iAIPercent += h.getAIWorldTrainPercent();
		else iAIPercent += h.getAITrainPercent();
		iPercent *= iAIPercent;
		iPercent /= 100;
	}
	return 0.01 * std::max(iPercent, 1);
}

int CvPlayer::getBuildingClassPrereqBuilding(BuildingTypes eBuilding, BuildingClassTypes ePrereqBuildingClass, int iExtra) const
{
	CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);

	int iPrereqs = kBuilding.getPrereqNumOfBuildingClass(ePrereqBuildingClass);

	// dont bother with the rest of the calcs if we have no prereqs
	if (iPrereqs < 1)
	{
		return 0;
	}

	BuildingClassTypes eBuildingClass = (BuildingClassTypes)kBuilding.getBuildingClassType();

	iPrereqs *= std::max(0, (GC.getWorldInfo(GC.getMap().getWorldSize()).getBuildingClassPrereqModifier() + 100));
	//iPrereqs /= 100;
	iPrereqs = (int)std::ceil(iPrereqs / 100.0); // advc.140: Round up

	if (!isLimitedWonderClass(eBuildingClass))
	{
		iPrereqs *= (getBuildingClassCount((BuildingClassTypes)(GC.getBuildingInfo(eBuilding).getBuildingClassType())) + iExtra + 1);
	}

	if (GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
	{
		iPrereqs = std::min(1, iPrereqs);
	}

	return iPrereqs;
}


void CvPlayer::removeBuildingClass(BuildingClassTypes eBuildingClass)
{
	BuildingTypes eBuilding = ((BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(eBuildingClass)));
	int iLoop;
	if (eBuilding != NO_BUILDING)
	{
		for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
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
}

// courtesy of the Gourd Bros...
void CvPlayer::processBuilding(BuildingTypes eBuilding, int iChange, CvArea* pArea)
{
	int iI, iJ;

	if (GC.getBuildingInfo(eBuilding).getFreeBuildingClass() != NO_BUILDINGCLASS)
	{
		BuildingTypes eFreeBuilding = (BuildingTypes)GC.getCivilizationInfo(
				getCivilizationType()).getCivilizationBuildings(
				GC.getBuildingInfo(eBuilding).getFreeBuildingClass());
		// advc.303: Barbarians can't receive the Monuments from Stonehenge
		if(eFreeBuilding != NO_BUILDING)
			changeFreeBuildingCount(eFreeBuilding, iChange);
	}

	if (GC.getBuildingInfo(eBuilding).getCivicOption() != NO_CIVICOPTION)
	{	// <advc.004x>
		CivicOptionTypes eCivicOption = (CivicOptionTypes)GC.getBuildingInfo(eBuilding).
				getCivicOption();
		CivicTypes eNewCivic = NO_CIVIC;
		if(iChange > 0 && isHuman() && !gDLL->GetWorldBuilderMode()) {
			for(int i = 0; i < GC.getNumCivicInfos(); i++) {
				CivicTypes eCivic = (CivicTypes)i;
				if(GC.getCivicInfo(eCivic).getCivicOptionType() == eCivicOption) {
					if(!canDoCivics(eCivic)) {
						eNewCivic = eCivic;
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

	changeGreatPeopleRateModifier(GC.getBuildingInfo(eBuilding).getGlobalGreatPeopleRateModifier() * iChange);
	changeGreatGeneralRateModifier(GC.getBuildingInfo(eBuilding).getGreatGeneralRateModifier() * iChange);
	changeDomesticGreatGeneralRateModifier(GC.getBuildingInfo(eBuilding).getDomesticGreatGeneralRateModifier() * iChange);
	changeAnarchyModifier(GC.getBuildingInfo(eBuilding).getAnarchyModifier() * iChange);
	changeGoldenAgeModifier(GC.getBuildingInfo(eBuilding).getGoldenAgeModifier() * iChange);
	changeHurryModifier(GC.getBuildingInfo(eBuilding).getGlobalHurryModifier() * iChange);
	changeFreeExperience(GC.getBuildingInfo(eBuilding).getGlobalFreeExperience() * iChange);
	changeWarWearinessModifier(GC.getBuildingInfo(eBuilding).getGlobalWarWearinessModifier() * iChange);
	pArea->changeFreeSpecialist(getID(), (GC.getBuildingInfo(eBuilding).getAreaFreeSpecialist() * iChange));
	changeFreeSpecialist(GC.getBuildingInfo(eBuilding).getGlobalFreeSpecialist() * iChange);
	changeCoastalTradeRoutes(GC.getBuildingInfo(eBuilding).getCoastalTradeRoutes() * iChange);
	// <advc.310> Now per area
	//changeTradeRoutes(GC.getBuildingInfo(eBuilding).getGlobalTradeRoutes() * iChange);
	pArea->changeTradeRoutes(getID(),
			GC.getBuildingInfo(eBuilding).getAreaTradeRoutes() * iChange);
	// </advc.310>
	if (GC.getBuildingInfo(eBuilding).getAreaHealth() > 0)
	{
		pArea->changeBuildingGoodHealth(getID(), (GC.getBuildingInfo(eBuilding).getAreaHealth() * iChange));
	}
	else
	{
		pArea->changeBuildingBadHealth(getID(), (GC.getBuildingInfo(eBuilding).getAreaHealth() * iChange));
	}
	if (GC.getBuildingInfo(eBuilding).getGlobalHealth() > 0)
	{
		changeBuildingGoodHealth(GC.getBuildingInfo(eBuilding).getGlobalHealth() * iChange);
	}
	else
	{
		changeBuildingBadHealth(GC.getBuildingInfo(eBuilding).getGlobalHealth() * iChange);
	}
	pArea->changeBuildingHappiness(getID(), (GC.getBuildingInfo(eBuilding).getAreaHappiness() * iChange));
	changeBuildingHappiness(GC.getBuildingInfo(eBuilding).getGlobalHappiness() * iChange);
	changeWorkerSpeedModifier(GC.getBuildingInfo(eBuilding).getWorkerSpeedModifier() * iChange);
	changeSpaceProductionModifier(GC.getBuildingInfo(eBuilding).getGlobalSpaceProductionModifier() * iChange);
	changeCityDefenseModifier(GC.getBuildingInfo(eBuilding).getAllCityDefenseModifier() * iChange);
	pArea->changeCleanPowerCount(getTeam(), ((GC.getBuildingInfo(eBuilding).isAreaCleanPower()) ? iChange : 0));
	pArea->changeBorderObstacleCount(getTeam(), ((GC.getBuildingInfo(eBuilding).isAreaBorderObstacle()) ? iChange : 0));

	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		changeSeaPlotYield(((YieldTypes)iI), (GC.getBuildingInfo(eBuilding).getGlobalSeaPlotYieldChange(iI) * iChange));
		pArea->changeYieldRateModifier(getID(), ((YieldTypes)iI), (GC.getBuildingInfo(eBuilding).getAreaYieldModifier(iI) * iChange));
		changeYieldRateModifier(((YieldTypes)iI), (GC.getBuildingInfo(eBuilding).getGlobalYieldModifier(iI) * iChange));
	}

	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		changeCommerceRateModifier(((CommerceTypes)iI), (GC.getBuildingInfo(eBuilding).getGlobalCommerceModifier(iI) * iChange));
		changeSpecialistExtraCommerce(((CommerceTypes)iI), (GC.getBuildingInfo(eBuilding).getSpecialistExtraCommerce(iI) * iChange));
		changeStateReligionBuildingCommerce(((CommerceTypes)iI), (GC.getBuildingInfo(eBuilding).getStateReligionCommerce(iI) * iChange));
		changeCommerceFlexibleCount(((CommerceTypes)iI), (GC.getBuildingInfo(eBuilding).isCommerceFlexible(iI)) ? iChange : 0);
	}

	for (iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingTypes eOurBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI);
		if (NO_BUILDING != eOurBuilding)
		{
			changeExtraBuildingHappiness(eOurBuilding, (GC.getBuildingInfo(eBuilding).getBuildingHappinessChanges(iI) * iChange));
		}
	}

	for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
	{
		for (iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
		{
			changeSpecialistExtraYield(((SpecialistTypes)iI), ((YieldTypes)iJ), (GC.getBuildingInfo(eBuilding).getSpecialistYieldChange(iI, iJ) * iChange));
		}
	}
}


bool CvPlayer::canBuild(const CvPlot* pPlot, BuildTypes eBuild, bool bTestEra, bool bTestVisible) const
{
	//PROFILE_FUNC(); // advc.003o
	if (!pPlot->canBuild(eBuild, getID(), bTestVisible))
		return false;

	if (GC.getBuildInfo(eBuild).getTechPrereq() != NO_TECH)
	{	// advc.003:
		TechTypes ePrereqTech = (TechTypes)GC.getBuildInfo(eBuild).getTechPrereq();
		if (!GET_TEAM(getTeam()).isHasTech(ePrereqTech))
		{
			if ((!bTestEra && !bTestVisible) || getCurrentEra() + 1 <
					GC.getTechInfo(ePrereqTech).getEra())
				return false;
		}
	}

	if (!bTestVisible)
	{
		if (pPlot->getFeatureType() != NO_FEATURE)
		{
			if (!GET_TEAM(getTeam()).isHasTech((TechTypes)GC.getBuildInfo(eBuild).
					getFeatureTech(pPlot->getFeatureType())))
				return false;
		}

		if (std::max(0, getGold()) < getBuildCost(pPlot, eBuild))
			return false;
	}

	return true;
}

// Returns the cost
int CvPlayer::getBuildCost(const CvPlot* pPlot, BuildTypes eBuild) const
{
	FAssert(eBuild >= 0 && eBuild < GC.getNumBuildInfos());

	if (pPlot->getBuildProgress(eBuild) > 0)
	{
		return 0;
	}

	return std::max(0, GC.getBuildInfo(eBuild).getCost() * (100 + calculateInflationRate())) / 100;
}




RouteTypes CvPlayer::getBestRoute(const CvPlot* pPlot) const
{
	PROFILE_FUNC();

	int iBestValue = 0;
	RouteTypes eBestRoute = NO_ROUTE;

	// BBAI TODO: Efficiency: Could cache this, decent savings on large maps
	// Perhaps save best route type per player each turn, then just check that
	// one first and only check others if can't do best.

	// K-Mod: I've reversed the order of iteration because the best builds are usually at the end.
	for (int iI = GC.getNumBuildInfos()-1; iI >= 0 ; iI--)
	{
		RouteTypes eRoute = ((RouteTypes)(GC.getBuildInfo((BuildTypes)iI).getRoute()));

		if (eRoute != NO_ROUTE)
		{
			// K-Mod: I've swapped the order of the if statments, because the value check is much faster. (faster trumps convention)
			int iValue = GC.getRouteInfo(eRoute).getValue();
			if (iValue > iBestValue)
			{
				if (pPlot != NULL ? (pPlot->getRouteType() == eRoute || canBuild(pPlot, (BuildTypes)iI)) : GET_TEAM(getTeam()).isHasTech((TechTypes)(GC.getBuildInfo((BuildTypes)iI).getTechPrereq())))
				{
					iBestValue = iValue;
					eBestRoute = eRoute;
				}
			}
		}
	}

	return eBestRoute;
}


int CvPlayer::getImprovementUpgradeRate() const
{
	int iRate = 1; // XXX

	iRate *= std::max(0, (getImprovementUpgradeRateModifier() + 100));
	iRate /= 100;

	return iRate;
}


int CvPlayer::calculateTotalYield(YieldTypes eYield) const
{
	PROFILE_FUNC();

	int iTotalCommerce = 0;
	int iLoop = 0;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iTotalCommerce += pLoopCity->getYieldRate(eYield);
	}
	return iTotalCommerce;
}


int CvPlayer::calculateTotalCityHappiness() const
{
	int iTotalHappiness = 0;
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iTotalHappiness += pLoopCity->happyLevel();
	}
	return iTotalHappiness;
}


int CvPlayer::calculateTotalExports(YieldTypes eYield) const
{
	int iTotalExports = 0;
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		for (int iTradeLoop = 0; iTradeLoop < pLoopCity->getTradeRoutes(); iTradeLoop++)
		{
			CvCity* pTradeCity = pLoopCity->getTradeCity(iTradeLoop);
			if (pTradeCity != NULL)
			{
				if (pTradeCity->getOwner() != getID())
				{
					iTotalExports += pLoopCity->calculateTradeYield(eYield, pLoopCity->calculateTradeProfit(pTradeCity));
				}
			}
		}
	}
	return iTotalExports;
}


int CvPlayer::calculateTotalImports(YieldTypes eYield) const
{
	int iTotalImports = 0;
	for (int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
	{
		if (iPlayerLoop == getID())
			continue;
		int iLoop;
		for (CvCity* pLoopCity = GET_PLAYER((PlayerTypes) iPlayerLoop).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes) iPlayerLoop).nextCity(&iLoop))
		{
			for (int iTradeLoop = 0; iTradeLoop < pLoopCity->getTradeRoutes(); iTradeLoop++)
			{
				CvCity* pTradeCity = pLoopCity->getTradeCity(iTradeLoop);
				if (pTradeCity != NULL)
				{
					if (pTradeCity->getOwner() == getID())
					{
						iTotalImports += pLoopCity->calculateTradeYield(eYield, pLoopCity->calculateTradeProfit(pTradeCity));
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
	int iLoop;
	for (CvCity*  pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iTotalUnhappiness += pLoopCity->unhappyLevel();
	}
	return iTotalUnhappiness;
}


int CvPlayer::calculateTotalCityHealthiness() const
{
	int iTotalHealthiness = 0;
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iTotalHealthiness += pLoopCity->goodHealth();
	}
	return iTotalHealthiness;
}

int CvPlayer::calculateTotalCityUnhealthiness() const
{
	int iTotalUnhealthiness = 0;
	int iLoop;
	for (CvCity*  pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iTotalUnhealthiness += pLoopCity->badHealth();
	}
	return iTotalUnhealthiness;
}

/*
** K-Mod
** calculate the pollution output of a civ.
** iTypes is a bit-field whos members are POLLUTION_POPULTION, _BUILDINGS, _BONUS, _POWER. (and _ALL)
*/
int CvPlayer::calculatePollution(int iTypes) const
{
	int iTotal = 0;

	int iBuildingWeight = ((iTypes & POLLUTION_BUILDINGS) == 0)?0 :GC.getDefineINT("GLOBAL_WARMING_BUILDING_WEIGHT");
	int iBonusWeight = ((iTypes & POLLUTION_BONUSES) == 0)?0 :GC.getDefineINT("GLOBAL_WARMING_BONUS_WEIGHT");
	int iPowerWeight = ((iTypes & POLLUTION_POWER) == 0)?0 :GC.getDefineINT("GLOBAL_WARMING_POWER_WEIGHT");
	int iPopWeight = ((iTypes & POLLUTION_POPULATION) == 0)?0 :GC.getDefineINT("GLOBAL_WARMING_POPULATION_WEIGHT");

	int iLoop;
	for (CvCity* pCity = firstCity(&iLoop); pCity != NULL; pCity = nextCity(&iLoop))
	{
		// note: "bad health" values are negative, except for population! (crazy, but true. Who writes this junk?)
		iTotal -=
			(pCity->totalBadBuildingHealth() * iBuildingWeight)
			+ (pCity->getBonusBadHealth() * iBonusWeight)
			+ (pCity->getPowerBadHealth() * iPowerWeight)
			- (pCity->unhealthyPopulation() * iPopWeight);
	}

	return iTotal;
}

int CvPlayer::getGwPercentAnger() const
{
	return m_iGwPercentAnger;
}

void CvPlayer::setGwPercentAnger(int iNewValue)
{
	if (iNewValue != m_iGwPercentAnger)
	{
		m_iGwPercentAnger = iNewValue;
		AI_makeAssignWorkDirty();
	}
}

/*
** K-Mod end
*/

// K-Mod
int CvPlayer::getUnitCostMultiplier() const
{
	int iMultiplier = 100;
	iMultiplier *= GC.getHandicapInfo(getHandicapType()).getUnitCostPercent();
	iMultiplier /= 100;

	if (!isHuman() && !isBarbarian())
	{
		iMultiplier *= GC.getHandicapInfo(GC.getGame().getHandicapType()).getAIUnitCostPercent();
		iMultiplier /= 100;
		// advc.251: Gold costs are no longer adjusted to handicap
		/*iMultiplier *= std::max(0, ((GC.getHandicapInfo(GC.getGame().getHandicapType()).getAIPerEraModifier() * getCurrentEra()) + 100));
		iMultiplier /= 100;*/
	}

	return iMultiplier;
}
// K-Mod end

int CvPlayer::calculateUnitCost(int& iFreeUnits, int& iFreeMilitaryUnits, int& iPaidUnits,
		int& iPaidMilitaryUnits, int& iUnitCost, int& iMilitaryCost, int& iExtraCost,
		int iExtraPop, int iExtraUnits) const // advc.004b
{
	iFreeUnits = GC.getHandicapInfo(getHandicapType()).getFreeUnits();

	iFreeUnits += getBaseFreeUnits();
	iFreeUnits += (((getTotalPopulation()
		+ iExtraPop) // advc.004b
		* getFreeUnitsPopulationPercent()) / 100);

	iFreeMilitaryUnits = getBaseFreeMilitaryUnits();
	iFreeMilitaryUnits += (((getTotalPopulation()
		+ iExtraPop) // advc.004b
		* getFreeMilitaryUnitsPopulationPercent()) / 100);

	/* original BTS code
	if (!isHuman()) {
		if (GET_TEAM(getTeam()).hasMetHuman())
		{
			iFreeUnits += getNumCities(); // XXX
			iFreeMilitaryUnits += getNumCities(); // XXX
		}
	} */ // Hidden AI bonus removed by BBAI.

	iPaidUnits = std::max(0, getNumUnits() - iFreeUnits
			+ iExtraUnits); // advc.004b
	iPaidMilitaryUnits = std::max(0, getNumMilitaryUnits()
			+ iExtraUnits // advc.004b
			- iFreeMilitaryUnits);
	//iSupport = 0;
	/* original bts code
	iBaseUnitCost = iPaidUnits * getGoldPerUnit();
	iMilitaryCost = iPaidMilitaryUnits * getGoldPerMilitaryUnit();
	iExtraCost = getExtraUnitCost();
	iSupport = iMilitaryCost + iBaseUnitCost + iExtraCost;
	iSupport *= GC.getHandicapInfo(getHandicapType()).getUnitCostPercent();
	iSupport /= 100;
	if (!isHuman() && !isBarbarian()) {
		iSupport *= GC.getHandicapInfo(GC.getGame().getHandicapType()).getAIUnitCostPercent();
		iSupport /= 100;
		iSupport *= std::max(0, ((GC.getHandicapInfo(GC.getGame().getHandicapType()).getAIPerEraModifier() * getCurrentEra()) + 100));
		iSupport /= 100;
	}*/
	// K-Mod. GoldPerUnit, etc, are now done as percentages.
	// Also, "UnitCostPercent" handicap modifiers now apply directly to unit cost only, not military or extra cost.
	// (iBaseUnitCost is no longer fed back to the caller. Only the modified cost is.)
	iUnitCost = iPaidUnits * getGoldPerUnit() * getUnitCostMultiplier() / 10000;
	iMilitaryCost = iPaidMilitaryUnits * getGoldPerMilitaryUnit() / 100;
	// <advc.912b>
	if(!isHuman() && !isBarbarian()) {
		iMilitaryCost = ::round(iMilitaryCost * 0.01 * GC.getHandicapInfo(
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

	/*  advc.003 (note): Several distinct variables need to be passed b/c
		calculateUnitCost uses them for intermediate results */
	int iFreeUnits, iFreeMilitaryUnits, iPaidUnits, iPaidMilitaryUnits,
			iMilitaryCost, iBaseUnitCost, iExtraCost;
	return calculateUnitCost(iFreeUnits, iFreeMilitaryUnits, iPaidUnits, iPaidMilitaryUnits,
			iBaseUnitCost, iMilitaryCost, iExtraCost,
			iExtraPop, iExtraUnits); // advc.004b
}

int CvPlayer::calculateUnitSupply(
		int iExtraOutsideUnits) const // advc.004b
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
	iPaidUnits = std::max(0, getNumOutsideUnits()
		+ iExtraOutsideUnits // advc.004b
		- GC.getDefineINT("INITIAL_FREE_OUTSIDE_UNITS"));

	iBaseSupplyCost = iPaidUnits * GC.getDefineINT("INITIAL_OUTSIDE_UNIT_GOLD_PERCENT");
	iBaseSupplyCost /= 100;
	int iSupply = iBaseSupplyCost;
	if (!isHuman() && !isBarbarian())
	{
		iSupply *= GC.getHandicapInfo(GC.getGame().getHandicapType()).getAIUnitSupplyPercent();
		iSupply /= 100;
		// advc.250d: Commented out
		/*iSupply *= std::max(0, ((GC.getHandicapInfo(GC.getGame().getHandicapType()).getAIPerEraModifier() * getCurrentEra()) + 100));
		iSupply /= 100;*/
	}
	FAssert(iSupply >= 0);
	return iSupply;
}


int CvPlayer::calculatePreInflatedCosts() const
{
	CyArgsList argsList;
	argsList.add(getID());
	long lResult;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "getExtraCost", argsList.makeFunctionArgs(), &lResult);

	return (calculateUnitCost() + calculateUnitSupply() + getTotalMaintenance() + getCivicUpkeep() + (int)lResult);
}


int CvPlayer::calculateInflationRate() const
{
	int iTurns = ((GC.getGame().getGameTurn() + GC.getGame().getElapsedGameTurns()) / 2);

	if (GC.getGame().getMaxTurns() > 0)
	{
		iTurns = std::min(GC.getGame().getMaxTurns(), iTurns);
	}

	iTurns += GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getInflationOffset();

	if (iTurns <= 0)
	{
		return 0;
	}

	int iInflationPerTurnTimes10000 = GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getInflationPercent();
	iInflationPerTurnTimes10000 *= GC.getHandicapInfo(getHandicapType()).getInflationPercent();
	iInflationPerTurnTimes10000 /= 100;

	int iModifier = m_iInflationModifier;
	if (!isHuman() && !isBarbarian())
	{
		int iAIModifier = GC.getHandicapInfo(GC.getGame().getHandicapType()).getAIInflationPercent();
		// advc.251: Gold costs are no longer adjusted to handicap
		/*iAIModifier *= std::max(0, ((GC.getHandicapInfo(GC.getGame().getHandicapType()).getAIPerEraModifier() * getCurrentEra()) + 100));
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

	int iCosts;

	iCosts = calculatePreInflatedCosts();

	iCosts *= std::max(0, (calculateInflationRate() + 100));
	iCosts /= 100;

	return iCosts;
}


/* int CvPlayer::calculateBaseNetGold() const {  // (see calculateGoldRate)

	int iNetGold = (getCommerceRate(COMMERCE_GOLD) + getGoldPerTurn());
	iNetGold -= calculateInflatedCosts();
	return iNetGold;
} */

int CvPlayer::calculateResearchModifier(TechTypes eTech,
		// <advc.910>
		int* piFromOtherKnown, int* piFromPaths, int* piFromTeam) const {
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
	if (GC.getTECH_DIFFUSION_ENABLE())
	{
		double knownExp = 0.0;
		// Tech flows better through open borders
		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (GET_TEAM((TeamTypes)iI).isHasTech(eTech))
				{
					if (GET_TEAM(getTeam()).isHasMet((TeamTypes)iI))
					{
						knownExp += 0.5;

						if (GET_TEAM(getTeam()).isOpenBorders((TeamTypes)iI) || GET_TEAM((TeamTypes)iI).isVassal(getTeam()))
						{
							knownExp += 1.5;
						}
						else if (GET_TEAM(getTeam()).isAtWar((TeamTypes)iI) || GET_TEAM(getTeam()).isVassal((TeamTypes)iI))
						{
							knownExp += 0.5;
						}
					}
				}
			}
		}
		int techDiffMod = GC.getTECH_DIFFUSION_KNOWN_TEAM_MODIFIER();
		if (knownExp > 0.0)
		{
			*piFromOtherKnown += // advc.910
					techDiffMod - (int)(techDiffMod * pow(0.85, knownExp) + 0.5);
		}
		// Tech flows downhill to those who are far behind
		int iTechScorePercent = GET_TEAM(getTeam()).getBestKnownTechScorePercent();
		int iWelfareThreshold = GC.getTECH_DIFFUSION_WELFARE_THRESHOLD();
		if (iTechScorePercent < iWelfareThreshold)
		{
			if (knownExp > 0.0)
			{
				*piFromOtherKnown += // advc.910
						(GC.getTECH_DIFFUSION_WELFARE_MODIFIER() *
						GC.getGame().getCurrentEra() *
						(iWelfareThreshold - iTechScorePercent))/200;
			}
		}
	}
	else
	{
		// Default BTS code
		int iKnownCount = 0;
		int iPossibleKnownCount = 0;
		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (GET_TEAM(getTeam()).isHasMet((TeamTypes)iI))
				{
					if (GET_TEAM((TeamTypes)iI).isHasTech(eTech))
					{
						iKnownCount++;
					}
				}

				iPossibleKnownCount++;
			}
		}

		if (iPossibleKnownCount > 0)
		{
			*piFromOtherKnown += // advc.910
				(GC.getTECH_COST_TOTAL_KNOWN_TEAM_MODIFIER() * iKnownCount) / iPossibleKnownCount;
		}
	}
	iModifier += *piFromOtherKnown; // advc.910

	int iPossiblePaths = 0;
	int iUnknownPaths = 0;
	for (int iI = 0; iI < GC.getNUM_OR_TECH_PREREQS(); iI++)
	{	// advc.003:
		TechTypes eOrTech = (TechTypes)GC.getTechInfo(eTech).getPrereqOrTechs(iI);
		if (eOrTech != NO_TECH)
		{
			if (!GET_TEAM(getTeam()).isHasTech(eOrTech))
			{
				iUnknownPaths++;
			}

			iPossiblePaths++;
		}
	}

	FAssertMsg(iPossiblePaths >= iUnknownPaths, "The number of possible paths is expected to match or exceed the number of unknown ones");

	if(iPossiblePaths > iUnknownPaths)
	{
		*piFromPaths += // advc.910
				GC.getTECH_COST_FIRST_KNOWN_PREREQ_MODIFIER();
		iPossiblePaths--;
		*piFromPaths +=
			(iPossiblePaths - iUnknownPaths) * GC.getTECH_COST_KNOWN_PREREQ_MODIFIER();
	}
	// BETTER_BTS_AI_MOD: END
	iModifier += *piFromPaths;
	// <advc.156>
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		if(i == getID())
			continue;
		CvPlayer const& kMember = GET_PLAYER((PlayerTypes)i);
		if(kMember.isAlive() && kMember.getTeam() == getTeam() &&
				kMember.getCurrentResearch() == eTech) {
			*piFromTeam = // advc.910
					GC.getRESEARCH_MODIFIER_EXTRA_TEAM_MEMBER();
			break; // Or should the penalty stack?
		}
	}
	iModifier += *piFromTeam; // advc.910
	// </advc.156>
	return iModifier;
}

/* int CvPlayer::calculateBaseNetResearch(TechTypes eTech) const {  // (see calculateResearchRate)
	TechTypes eResearchTech;
	if(eTech != NO_TECH)
		eResearchTech = eTech;
	else eResearchTech = getCurrentResearch();
	return (((GC.getDefineINT("BASE_RESEARCH_RATE") + getCommerceRate(COMMERCE_RESEARCH)) * calculateResearchModifier(eResearchTech)) / 100);
} */


int CvPlayer::calculateGoldRate() const
{	/* original bts code
	int iRate = 0;
	if(isCommerceFlexible(COMMERCE_RESEARCH))
		iRate = calculateBaseNetGold();
	else iRate = std::min(0, (calculateBaseNetResearch() + calculateBaseNetGold()));
	return iRate; */
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
	/* original bts code
	int iRate = 0;
	if(isCommerceFlexible(COMMERCE_RESEARCH))
		iRate = calculateBaseNetResearch(eTech);
	else iRate = std::max(1, (calculateBaseNetResearch(eTech) + calculateBaseNetGold()));
	return iRate; */
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
	if(GC.getUSE_IS_PLAYER_RESEARCH_CALLBACK()) {
		CyArgsList argsList;
		argsList.add(getID());
		long lResult = 1;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "isPlayerResearch", argsList.makeFunctionArgs(), &lResult);
		if(lResult == 0)
			return false;
	}
	return (isFoundedFirstCity() /* advc.004x: */ && !isAnarchy());
}


bool CvPlayer::canEverResearch(TechTypes eTech) const
{
	if (GC.getTechInfo(eTech).isDisable())
	{
		return false;
	}

	if (GC.getCivilizationInfo(getCivilizationType()).isCivilizationDisableTechs(eTech))
	{
		return false;
	}

	if(GC.getUSE_CANNOT_RESEARCH_CALLBACK())
	{
		CyArgsList argsList;
		argsList.add(getID());
		argsList.add(eTech);
		argsList.add(false);
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "cannotResearch", argsList.makeFunctionArgs(), &lResult);
		if (lResult == 1)
			return false;
	}

	return true;
}


bool CvPlayer::canResearch(TechTypes eTech, bool bTrade, bool bFree,
		bool bCouldResearchAgain) const // advc.126
{
	if(GC.getUSE_CAN_RESEARCH_CALLBACK()) {
		CyArgsList argsList;
		argsList.add(getID());
		argsList.add(eTech);
		argsList.add(bTrade);
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "canResearch", argsList.makeFunctionArgs(), &lResult);
		if(lResult == 1)
			return true;
	}

	/*  advc.004x: Commented out - shouldn't matter here.
		(And wouldn't prevent players from queuing up research during anarchy or
		before founding a city) */
	/*if(!isResearch() && !bFree && getAdvancedStartPoints() < 0)
		return false;*/

	if (GET_TEAM(getTeam()).isHasTech(eTech)
			&& !bCouldResearchAgain) // advc.126
		return false;

	bool bFoundPossible = false;
	bool bFoundValid = false;
	int iI;
	for (iI = 0; iI < GC.getNUM_OR_TECH_PREREQS(); iI++)
	{
		TechTypes ePrereq = (TechTypes)GC.getTechInfo(eTech).getPrereqOrTechs(iI);
		// <advc.126> Cycle detection
		if(ePrereq == eTech) {
			FAssert(false);
			continue;
		} // </advc.126>
		if (ePrereq != NO_TECH)
		{
			bFoundPossible = true;

			if (GET_TEAM(getTeam()).isHasTech(ePrereq)
					// advc.126: Don't check recursively (for execution speed concerns)
					&& (bCouldResearchAgain || canResearch(ePrereq, false, true, true)))
			{
				if (!bTrade || GC.getGame().isOption(GAMEOPTION_NO_TECH_BROKERING) || !GET_TEAM(getTeam()).isNoTradeTech(ePrereq))
				{
					bFoundValid = true;
					break;
				}
			}
		}
	}

	if (bFoundPossible && !bFoundValid)
	{
		return false;
	}

	for (iI = 0; iI < GC.getNUM_AND_TECH_PREREQS(); iI++)
	{
		TechTypes ePrereq = (TechTypes)GC.getTechInfo(eTech).getPrereqAndTechs(iI);
		if (ePrereq != NO_TECH)
		{
			if (!GET_TEAM(getTeam()).isHasTech(ePrereq)
					// advc.126:
					|| (bCouldResearchAgain && !canResearch(ePrereq, false, true, true)))
				return false;

			if (bTrade && !GC.getGame().isOption(GAMEOPTION_NO_TECH_BROKERING) && GET_TEAM(getTeam()).isNoTradeTech(ePrereq))
				return false;
		}
	}

	if (!canEverResearch(eTech))
	{
		return false;
	}

	return true;
}


TechTypes CvPlayer::getCurrentResearch() const
{
	CLLNode<TechTypes>* pResearchNode;

	pResearchNode = headResearchQueueNode();

	if (pResearchNode != NULL)
	{
		return pResearchNode->m_data;
	}
	else
	{
		return NO_TECH;
	}
}


bool CvPlayer::isCurrentResearchRepeat() const
{
	TechTypes eCurrentResearch;

	eCurrentResearch = getCurrentResearch();

	if (eCurrentResearch == NO_TECH)
	{
		return false;
	}

	return GC.getTechInfo(eCurrentResearch).isRepeat();
}


bool CvPlayer::isNoResearchAvailable() const
{
	int iI;

	if (getCurrentResearch() != NO_TECH)
	{
		return false;
	}

	for (iI = 0; iI < GC.getNumTechInfos(); iI++)
	{
		if (canResearch((TechTypes)iI))
		{
			return false;
		}
	}

	return true;
}


int CvPlayer::getResearchTurnsLeft(TechTypes eTech, bool bOverflow) const
{
	int iTurnsLeft = getResearchTurnsLeftTimes100(eTech, bOverflow);
	if(iTurnsLeft >= 0) // advc.004x
		iTurnsLeft = (iTurnsLeft + 99) / 100; // round up
	//return std::max(1, iTurnsLeft);
	return iTurnsLeft; // advc.004x: -1 now means infinitely many turns
}

int CvPlayer::getResearchTurnsLeftTimes100(TechTypes eTech, bool bOverflow) const  // advc.003: style changes
{
	int iResearchRate = 0;
	int iOverflow = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer const& kMember = GET_PLAYER((PlayerTypes)iI);
		if(!kMember.isAlive() || kMember.getTeam() != getTeam()
				|| !kMember.isResearch()) // advc.004x
			continue;
		if(iI == getID() || kMember.getCurrentResearch() == eTech)
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
	iResearchLeft *= 100;
	if(iResearchRate <= 0) {
		//return iResearchLeft;
		return -1; // advc.004x
	} // BETTER_BTS_AI_MOD: END

	int iTurnsLeft = iResearchLeft / iResearchRate;
	if(iTurnsLeft * iResearchRate < iResearchLeft)
		iTurnsLeft++;
	//return std::max(1, iTurnsLeft);
	return iTurnsLeft; // advc.004x
}

// K-Mod. Return true if this player can see what ePlayer is researching
bool CvPlayer::canSeeResearch(PlayerTypes ePlayer,
		bool bCheckPoints) const // advc.085
{
	FASSERT_BOUNDS(0, MAX_PLAYERS, ePlayer, "CvPlayer::canSeeResearch");
	const CvPlayer& kOther = GET_PLAYER(ePlayer);

	if (kOther.getTeam() == getTeam() || GET_TEAM(kOther.getTeam()).isVassal(getTeam()))
		return true;

	if (!GET_TEAM(getTeam()).isHasMet(kOther.getTeam()))
		return false;

	if (!GC.getGame().isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		for (EspionageMissionTypes i = (EspionageMissionTypes)0; i < GC.getNumEspionageMissionInfos(); i = (EspionageMissionTypes)(i+1))
		{
			CvEspionageMissionInfo& kMissionInfo = GC.getEspionageMissionInfo(i);

			if (kMissionInfo.isSeeResearch() && kMissionInfo.isPassive() &&
					canDoEspionageMission(i, ePlayer, NULL, 0, NULL,
					bCheckPoints)) // advc.085
				return true;
		}
	}
	return false;
}

// return true if this player can see ePlayer's demographics (power graph, culture graph, etc.)
bool CvPlayer::canSeeDemographics(PlayerTypes ePlayer,
		bool bCheckPoints) const // advc.085
{
	FASSERT_BOUNDS(0, MAX_PLAYERS, ePlayer, "CvPlayer::canSeeDemographics");
	const CvPlayer& kOther = GET_PLAYER(ePlayer);

	if (kOther.getTeam() == getTeam() || GET_TEAM(kOther.getTeam()).isVassal(getTeam()))
		return true;

	if (!GET_TEAM(getTeam()).isHasMet(kOther.getTeam()))
		return false;

	if (GC.getGame().isOption(GAMEOPTION_NO_ESPIONAGE))
		return true;

	for (EspionageMissionTypes i = (EspionageMissionTypes)0; i < GC.getNumEspionageMissionInfos(); i = (EspionageMissionTypes)(i+1))
	{
		CvEspionageMissionInfo& kMissionInfo = GC.getEspionageMissionInfo(i);

		if (kMissionInfo.isSeeDemographics() && kMissionInfo.isPassive() &&
				canDoEspionageMission(i, ePlayer, NULL, 0, NULL,
				bCheckPoints)) // advc.085
			return true;
	}
	return false;
} // K-Mod end

/*  <advc.085> !bDemographics means: return espionage needed to see research.
	Mix of code from canSeeDemographics, canSeeResearch and canDoEspionageMission. */
int CvPlayer::espionageNeededToSee(PlayerTypes ePlayer, bool bDemographics) const {

	int r = MAX_INT;
	if(!(bDemographics ?
			canSeeDemographics(ePlayer, false) : canSeeResearch(ePlayer, false)))
		return r;
	int iEspionagePoints = GET_TEAM(getTeam()).getEspionagePointsAgainstTeam(TEAMID(ePlayer));
	for(int i = 0; i < GC.getNumEspionageMissionInfos(); i++) {
		EspionageMissionTypes eLoopMission = (EspionageMissionTypes)i;
		CvEspionageMissionInfo& kMission = GC.getEspionageMissionInfo(eLoopMission);
		if(!kMission.isPassive())
			continue;
		if(bDemographics ? kMission.isSeeDemographics() : kMission.isSeeResearch()) {
			r = std::min(r, getEspionageMissionCost(
					eLoopMission, ePlayer, NULL, 0, NULL) - iEspionagePoints);
		}
	}
	return r;
} // </advc.085>
// <advc.550e>
bool CvPlayer::isSignificantDiscovery(TechTypes eTech) const {

	// (K-Mod comment moved here from CvDeal::startTrade)
	// Only adjust tech_from_any memory if this is a tech from a recent era
	// and the team receiving the tech isn't already more than 2/3 of the way through.
	// (This is to prevent the AI from being crippled by human players selling them lots of tech scraps.)
	// Note: the current game era is the average of all the player eras, rounded down. (It no longer includes barbs.)
	// advc: I'm going to use the recipient's era instead, the rest is as in K-Mod.
	return GC.getTechInfo(eTech).getEra() >= getCurrentEra() - 1 &&
			GET_TEAM(getTeam()).getResearchLeft(eTech) >
			GET_TEAM(getTeam()).getResearchCost(eTech) / 3;
} // <advc.550e>

bool CvPlayer::isCivic(CivicTypes eCivic) const
{
	for (int iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
	{
		if (getCivics((CivicOptionTypes)iI) == eCivic)
		{
			return true;
		}
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

	if (GC.getGame().isForceCivicOption((CivicOptionTypes)(GC.getCivicInfo(eCivic).getCivicOptionType())))
	{
		return GC.getGame().isForceCivic(eCivic);
	}

	if(GC.getUSE_CAN_DO_CIVIC_CALLBACK())
	{
		CyArgsList argsList;
		argsList.add(getID());
		argsList.add(eCivic);
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "canDoCivic", argsList.makeFunctionArgs(), &lResult);
		if (lResult == 1)
		{
			return true;
		}
	}

	if (!isHasCivicOption((CivicOptionTypes)(GC.getCivicInfo(eCivic).getCivicOptionType())) && !(GET_TEAM(getTeam()).isHasTech((TechTypes)(GC.getCivicInfo(eCivic).getTechPrereq()))))
	{
		return false;
	}

	if(GC.getUSE_CANNOT_DO_CIVIC_CALLBACK())
	{
		CyArgsList argsList2; // XXX
		argsList2.add(getID());
		argsList2.add(eCivic);
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "cannotDoCivic", argsList2.makeFunctionArgs(), &lResult);
		if (lResult == 1)
			return false;
	}
	// <advc.912d>
	if(GC.getGame().isOption(GAMEOPTION_NO_SLAVERY) && isHuman()) {
		for(int i = 0; i < GC.getNumHurryInfos(); i++) {
			if(GC.getCivicInfo(eCivic).isHurry(i) &&
					GC.getHurryInfo((HurryTypes)i).getProductionPerPopulation() > 0)
				return false;
		}
	} // </advc.912d>
	return true;
}


bool CvPlayer::canRevolution(CivicTypes* paeNewCivics) const
{
	if (isAnarchy())
	{
		return false;
	}

	if (getRevolutionTimer() > 0)
	{
		return false;
	}

	if (paeNewCivics == NULL)
	{	// XXX is this necessary?
		// ^advc.003: Only for the call in CvPlayer::doAdvancedStartAction I think
		for (int iI = 0; iI < GC.getNumCivicInfos(); iI++)
		{
			if (canDoCivics((CivicTypes)iI))
			{
				if (getCivics((CivicOptionTypes)GC.getCivicInfo((CivicTypes) iI).getCivicOptionType()) != iI)
				{
					return true;
				}
			}
		}
	}
	else
	{
		for (int iI = 0; iI < GC.getNumCivicOptionInfos(); ++iI)
		{
			if (GC.getGame().isForceCivicOption((CivicOptionTypes)iI))
			{
				if (!GC.getGame().isForceCivic(paeNewCivics[iI]))
				{
					return false;
				}
			}

			if (getCivics((CivicOptionTypes)iI) != paeNewCivics[iI])
			{
				return true;
			}
		}
	}

	return false;
}


void CvPlayer::revolution(CivicTypes* paeNewCivics, bool bForce)
{

	if (!bForce && !canRevolution(paeNewCivics))
		return;

	int iI;

	int iAnarchyLength = getCivicAnarchyLength(paeNewCivics);
	if (iAnarchyLength > 0)
	{
		changeAnarchyTurns(iAnarchyLength);

		for (iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
		{
			setCivics(((CivicOptionTypes)iI), paeNewCivics[iI]);
		}
	}
	else
	{
		for (iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
		{
			setCivics(((CivicOptionTypes)iI), paeNewCivics[iI]);
		}
	}

	setRevolutionTimer(std::max(1, ((100 + getAnarchyModifier()) * GC.getDefineINT("MIN_REVOLUTION_TURNS")) / 100) + iAnarchyLength);

	if (getID() == GC.getGame().getActivePlayer())
	{
		gDLL->getInterfaceIFace()->setDirty(Popup_DIRTY_BIT, true); // to force an update of the civic chooser popup
		// <advc.004x>
		killAll(BUTTONPOPUP_CHANGECIVIC);
		if(iAnarchyLength > 0) {
			killAll(BUTTONPOPUP_CHOOSEPRODUCTION);
			killAll(BUTTONPOPUP_CHOOSETECH);
		}
	} // </advc.004x>
}


int CvPlayer::getCivicPercentAnger(CivicTypes eCivic, bool bIgnore) const
{
	if (GC.getCivicInfo(eCivic).getCivicPercentAnger() == 0)
	{
		return 0;
	}

	if (!bIgnore && (getCivics((CivicOptionTypes)(GC.getCivicInfo(eCivic).getCivicOptionType())) == eCivic))
	{
		return 0;
	}

	int iCount = 0;
	int iPossibleCount = 0;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam())
			{
				if (GET_PLAYER((PlayerTypes)iI).getCivics((CivicOptionTypes)(GC.getCivicInfo(eCivic).getCivicOptionType())) == eCivic)
				{
					iCount += GET_PLAYER((PlayerTypes)iI).getNumCities();
				}

				iPossibleCount += GET_PLAYER((PlayerTypes)iI).getNumCities();
			}
		}
	}

	if (iPossibleCount == 0)
	{
		return 0;
	}

	return ((GC.getCivicInfo(eCivic).getCivicPercentAnger() * iCount) / iPossibleCount);
}


bool CvPlayer::canDoReligion(ReligionTypes eReligion) const
{
	/*if (GET_TEAM(getTeam()).getHasReligionCount(eReligion) == 0)
		return false;
	return true;*/
	// advc.132c: Replacing the above
	return (getHasReligionCount(eReligion) != 0);
}


bool CvPlayer::canChangeReligion() const
{
	for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		if (canConvert((ReligionTypes)iI))
		{
			return true;
		}
	}

	return false;
}


bool CvPlayer::canConvert(ReligionTypes eReligion) const
{
	if (isBarbarian())
	{
		return false;
	}

	if (isAnarchy())
	{
		return false;
	}

	if (getConversionTimer() > 0)
	{
		return false;
	}

	if (!isStateReligion())
	{
		return false;
	}

	if (getLastStateReligion() == eReligion)
	{
		return false;
	}

	if (eReligion != NO_RELIGION)
	{
		if (!canDoReligion(eReligion))
		{
			return false;
		}
	}

	return true;
}


void CvPlayer::convert(ReligionTypes eReligion, // <advc.001v>
		bool bForce)
{
	if(!bForce && /* </advc.001v> */ !canConvert(eReligion))
		return;

	int iAnarchyLength = getReligionAnarchyLength();

	changeAnarchyTurns(iAnarchyLength);

	setLastStateReligion(eReligion);

	setConversionTimer(std::max(1, ((100 + getAnarchyModifier()) * GC.getDefineINT("MIN_CONVERSION_TURNS")) / 100) + iAnarchyLength);
	// <advc.004x>
	if(getID() == GC.getGame().getActivePlayer()) {
		killAll(BUTTONPOPUP_CHANGERELIGION);
		if(iAnarchyLength > 0) {
			killAll(BUTTONPOPUP_CHOOSEPRODUCTION);
			killAll(BUTTONPOPUP_CHOOSETECH);
		}
	} // </advc.004x>
}


bool CvPlayer::hasHolyCity(ReligionTypes eReligion) const
{
	FAssertMsg(eReligion != NO_RELIGION, "Religion is not assigned a valid value");

	CvCity* pHolyCity = GC.getGame().getHolyCity(eReligion);
	if (pHolyCity != NULL)
	{
		return (pHolyCity->getOwner() == getID());
	}
	return false;
}


int CvPlayer::countHolyCities() const
{
	int iCount = 0;
	for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		if (hasHolyCity((ReligionTypes)iI))
		{
			iCount++;
		}
	}

	return iCount;
}


void CvPlayer::foundReligion(ReligionTypes eReligion, ReligionTypes eSlotReligion, bool bAward)  // advc.003: some style changes
{
	if (eReligion == NO_RELIGION)
		return;

	CvReligionInfo const& kSlotReligion = GC.getReligionInfo(eSlotReligion);
	CvGame& g = GC.getGame();
	if (g.isReligionFounded(eReligion))
	{
		if (isHuman())
		{
			CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_FOUND_RELIGION, eSlotReligion);
			if (pInfo != NULL)
				gDLL->getInterfaceIFace()->addPopup(pInfo, getID());
		}
		else foundReligion(AI_chooseReligion(), eSlotReligion, bAward);
		return;
	}
	g.setReligionSlotTaken(eSlotReligion, true);

	bool bStarting = (kSlotReligion.getTechPrereq() == NO_TECH ||
			GC.getTechInfo((TechTypes)kSlotReligion.getTechPrereq()).getEra() < g.getStartEra());

	int iBestValue = 0;
	CvCity* pBestCity = NULL;
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (bStarting && pLoopCity->isHolyCity())
			continue;

		int iValue = 10;
		iValue += pLoopCity->getPopulation();
		iValue += g.getSorenRandNum(GC.getDefineINT("FOUND_RELIGION_CITY_RAND"), "Found Religion");
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

	g.setHolyCity(eReligion, pBestCity, true);
	if (bAward && kSlotReligion.getNumFreeUnits() > 0)
	{
		UnitTypes eFreeUnit = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).
				getCivilizationUnits(GC.getReligionInfo(eReligion).getFreeUnitClass());
		if (eFreeUnit != NO_UNIT)
		{
			for (int i = 0; i < kSlotReligion.getNumFreeUnits(); i++)
				initUnit(eFreeUnit, pBestCity->getX(), pBestCity->getY());
		}
	}
}


bool CvPlayer::hasHeadquarters(CorporationTypes eCorporation) const
{
	CvCity* pHeadquarters = GC.getGame().getHeadquarters(eCorporation);

	FAssert(eCorporation != NO_CORPORATION);

	if (pHeadquarters != NULL)
	{
		return (pHeadquarters->getOwner() == getID());
	}

	return false;
}


int CvPlayer::countHeadquarters() const
{
	int iCount = 0;

	for (int iI = 0; iI < GC.getNumCorporationInfos(); iI++)
	{
		if (hasHeadquarters((CorporationTypes)iI))
		{
			iCount++;
		}
	}

	return iCount;
}


//int CvPlayer::countCorporations(CorporationTypes eCorporation) const
int CvPlayer::countCorporations(CorporationTypes eCorporation, CvArea* pArea) const // K-Mod
{
	int iCount = 0;

	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (!pArea || pLoopCity->area() == pArea) // K-Mod
		{
			if (pLoopCity->isHasCorporation(eCorporation))
			{
				++iCount;
			}
		}
	}

	return iCount;
}


void CvPlayer::foundCorporation(CorporationTypes eCorporation)
{
	if (GC.getGame().isCorporationFounded(eCorporation))
	{
		return;
	}

	bool bStarting = ((GC.getCorporationInfo(eCorporation).getTechPrereq() == NO_TECH) || (GC.getTechInfo((TechTypes) GC.getCorporationInfo(eCorporation).getTechPrereq()).getEra() < GC.getGame().getStartEra()));

	int iBestValue = 0;
	CvCity* pBestCity = NULL;
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (!bStarting || !(pLoopCity->isHeadquarters()))
		{
			int iValue = 10;
			iValue += pLoopCity->getPopulation();

			for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
			{
				if (NO_BONUS != GC.getCorporationInfo(eCorporation).getPrereqBonus(i))
				{
					iValue += 10 * pLoopCity->getNumBonuses((BonusTypes)GC.getCorporationInfo(eCorporation).getPrereqBonus(i));
				}
			}

			iValue += GC.getGame().getSorenRandNum(GC.getDefineINT("FOUND_CORPORATION_CITY_RAND"), "Found Corporation");

			iValue /= (pLoopCity->getCorporationCount() + 1);

			iValue = std::max(1, iValue);

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				pBestCity = pLoopCity;
			}
		}
	}

	if (pBestCity != NULL)
	{
		pBestCity->setHeadquarters(eCorporation);
	}
}


int CvPlayer::getCivicAnarchyLength(CivicTypes* paeNewCivics,
		bool bIgnoreGoldenAge) const { // advc.132

	if(getMaxAnarchyTurns() == 0)
		return 0;

	if(/* <advc.132> */ !bIgnoreGoldenAge && /* </advc.132> */ isGoldenAge())
		return 0;

	int iAnarchyLength = 0;

	bool bChange = false;

	for (int iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
	{
		if (paeNewCivics[iI] != getCivics((CivicOptionTypes)iI))
		{
			iAnarchyLength += GC.getCivicInfo(paeNewCivics[iI]).getAnarchyLength();

			bChange = true;
		}
	}

	if (bChange)
	{
		iAnarchyLength += GC.getDefineINT("BASE_CIVIC_ANARCHY_LENGTH");

		iAnarchyLength += ((getNumCities() * GC.getWorldInfo(GC.getMap().
				getWorldSize()).getNumCitiesAnarchyPercent()) / 100);
	}

	iAnarchyLength = ((iAnarchyLength * std::max(0, getAnarchyModifier() + 100)) / 100);

	if(iAnarchyLength == 0)
		return 0;

	iAnarchyLength *= GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).
			getAnarchyPercent();
	iAnarchyLength /= 100;

	iAnarchyLength *= GC.getEraInfo(GC.getGame().getStartEra()).
			getAnarchyPercent();
	iAnarchyLength /= 100;

	return range(iAnarchyLength, 1, getMaxAnarchyTurns());
}


int CvPlayer::getReligionAnarchyLength(
		bool ignoreGoldenAge) const { // advc.132

	if(getMaxAnarchyTurns() == 0)
		return 0;

	if(/* <advc.132> */ !ignoreGoldenAge && /* </advc.132> */ isGoldenAge())
		return 0;

	int iAnarchyLength = GC.getDefineINT("BASE_RELIGION_ANARCHY_LENGTH");

	iAnarchyLength += ((getNumCities() * GC.getWorldInfo(GC.getMap().
			getWorldSize()).getNumCitiesAnarchyPercent()) / 100);

	iAnarchyLength = ((iAnarchyLength * std::max(0, getAnarchyModifier() + 100)) / 100);

	if(iAnarchyLength == 0)
		return 0;

	iAnarchyLength *= GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).
			getAnarchyPercent();
	iAnarchyLength /= 100;

	iAnarchyLength *= GC.getEraInfo(GC.getGame().getStartEra()).
			getAnarchyPercent();
	iAnarchyLength /= 100;

	return range(iAnarchyLength, 1, getMaxAnarchyTurns());
}



int CvPlayer::unitsRequiredForGoldenAge() const
{
	return (GC.getDefineINT("BASE_GOLDEN_AGE_UNITS") + (getNumUnitGoldenAges() * GC.getDefineINT("GOLDEN_AGE_UNITS_MULTIPLIER")));
}


int CvPlayer::unitsGoldenAgeCapable() const
{
	int iCount = 0;
	int iLoop;
	for(CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		if (pLoopUnit->isGoldenAge())
		{
			iCount++;
		}
	}

	return iCount;
}

// Rewritten for K-Mod. (The only functionality difference is that unit class is now used rather unit type. But this version is far more efficient.)
int CvPlayer::unitsGoldenAgeReady() const
{
	PROFILE_FUNC();

	std::set<UnitClassTypes> golden_age_units;

	int iLoop;
	for (CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		if (pLoopUnit->isGoldenAge() && golden_age_units.count(pLoopUnit->getUnitClassType()) == 0)
		{
			golden_age_units.insert(pLoopUnit->getUnitClassType());
		}
	}

	return golden_age_units.size();
}


void CvPlayer::killGoldenAgeUnits(CvUnit* pUnitAlive)
{
	bool* pabUnitUsed = new bool[GC.getNumUnitInfos()];
	for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
	{
		pabUnitUsed[iI] = false;
	}

	int iUnitsRequired = unitsRequiredForGoldenAge();

	if (pUnitAlive != NULL)
	{
		pabUnitUsed[pUnitAlive->getUnitType()] = true;
		iUnitsRequired--;
	}

	for (int iI = 0; iI < iUnitsRequired; iI++)
	{
		int iBestValue = 0;
		CvUnit* pBestUnit = NULL;
		int iLoop=-1;
		for(CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
		{
			if (pLoopUnit->isGoldenAge())
			{
				if (!(pabUnitUsed[pLoopUnit->getUnitType()]))
				{
					int iValue = 10000;

					iValue /= (plotDistance(pLoopUnit->getX(), pLoopUnit->getY(), pUnitAlive->getX(), pUnitAlive->getY()) + 1);

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						pBestUnit = pLoopUnit;
					}
				}
			}
		}

		FAssert(pBestUnit != NULL);
		if (pBestUnit != NULL)
		{
			pabUnitUsed[pBestUnit->getUnitType()] = true;

			pBestUnit->kill(true);

			//play animations
			if (pBestUnit->plot()->isActiveVisible(false))
			{
				//kill removes bestUnit from any groups
				pBestUnit->getGroup()->pushMission(MISSION_GOLDEN_AGE, 0);
			}
		}
	}

	SAFE_DELETE_ARRAY(pabUnitUsed);
}


int CvPlayer::greatPeopleThreshold(bool bMilitary) const
{
	int iThreshold;

	if (bMilitary)
	{
		iThreshold = ((GC.getDefineINT("GREAT_GENERALS_THRESHOLD") *
				std::max(0, (getGreatGeneralsThresholdModifier() + 100))) / 100);
	}
	else
	{
		iThreshold = ((GC.getDefineINT("GREAT_PEOPLE_THRESHOLD") *
				std::max(0, (getGreatPeopleThresholdModifier() + 100))) / 100);
	}
	CvGame const& g = GC.getGame(); // advc.003
	iThreshold *= GC.getGameSpeedInfo(g.getGameSpeedType()).getGreatPeoplePercent();
	if (bMilitary)
	{
		iThreshold /= std::max(1, GC.getGameSpeedInfo(g.getGameSpeedType()).getTrainPercent());
	}
	else
	{
		iThreshold /= 100;
	}

	iThreshold *= GC.getEraInfo(g.getStartEra()).getGreatPeoplePercent();
	iThreshold /= 100;
	// <advc.251>
	iThreshold = ::roundToMultiple(0.01 * iThreshold * GC.getHandicapInfo(
			getHandicapType()).getGPThresholdPercent(),
			isHuman() ? 5 : 1);
	if(!isHuman() && !isBarbarian()) {
		iThreshold = ::round(0.01 * iThreshold * GC.getHandicapInfo(
				g.getHandicapType()).getAIGPThresholdPercent());
	} // </advc.251>

	return std::max(1, iThreshold);
}


int CvPlayer::specialistYield(SpecialistTypes eSpecialist, YieldTypes eYield) const
{
	return (GC.getSpecialistInfo(eSpecialist).getYieldChange(eYield) + getSpecialistExtraYield(eSpecialist, eYield));
}


int CvPlayer::specialistCommerce(SpecialistTypes eSpecialist, CommerceTypes eCommerce) const
{
	return (GC.getSpecialistInfo(eSpecialist).getCommerceChange(eCommerce) + getSpecialistExtraCommerce(eCommerce));
}


CvPlot* CvPlayer::getStartingPlot() const
{
	return GC.getMap().plotSoren(m_iStartingX, m_iStartingY);
}


void CvPlayer::setStartingPlot(CvPlot* pNewValue, bool bUpdateStartDist)
{
	CvPlot* pOldStartingPlot = getStartingPlot();
	if(pOldStartingPlot == pNewValue)
		return; // advc.003

	if (pOldStartingPlot != NULL)
	{
		pOldStartingPlot->area()->changeNumStartingPlots(-1);

		if (bUpdateStartDist)
		{
			GC.getMap().updateMinOriginalStartDist(pOldStartingPlot->area());
		}
	}

	if (pNewValue == NULL)
	{
		m_iStartingX = INVALID_PLOT_COORD;
		m_iStartingY = INVALID_PLOT_COORD;
	}
	else
	{
		m_iStartingX = pNewValue->getX();
		m_iStartingY = pNewValue->getY();

		getStartingPlot()->area()->changeNumStartingPlots(1);

		if (bUpdateStartDist)
		{
			GC.getMap().updateMinOriginalStartDist(getStartingPlot()->area());
		}
	}
	FAssert(pNewValue == NULL || !pNewValue->isWater()); // advc.021b
}


int CvPlayer::getTotalPopulation() const
{
	return m_iTotalPopulation;
}


int CvPlayer::getAveragePopulation() const
{
	if (getNumCities() == 0)
	{
		return 0;
	}

	//return ((getTotalPopulation() / getNumCities()) + 1);
	// advc.131: The above is 100% off at the start of a game
	return ::round(getTotalPopulation() / getNumCities());
}


void CvPlayer::changeTotalPopulation(int iChange)
{
	changeAssets(-(getPopulationAsset(getTotalPopulation())));
	changePower(-(getPopulationPower(getTotalPopulation())));
	changePopScore(-(getPopulationScore(getTotalPopulation())));

	m_iTotalPopulation = (m_iTotalPopulation + iChange);
	FAssert(getTotalPopulation() >= 0);

	changeAssets(getPopulationAsset(getTotalPopulation()));
	changePower(getPopulationPower(getTotalPopulation()));
	changePopScore(getPopulationScore(getTotalPopulation()));
}


long CvPlayer::getRealPopulation() const
{
	CvCity* pLoopCity;
	__int64 iTotalPopulation = 0;
	int iLoop = 0;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iTotalPopulation += pLoopCity->getRealPopulation();
	}

	if (iTotalPopulation > MAX_INT)
	{
		iTotalPopulation = MAX_INT;
	}

	return ((long)(iTotalPopulation));
}


int CvPlayer::getTotalLand() const
{
	return m_iTotalLand;
}


void CvPlayer::changeTotalLand(int iChange)
{
	m_iTotalLand = (m_iTotalLand + iChange);
	FAssert(getTotalLand() >= 0);
}


int CvPlayer::getTotalLandScored() const
{
	return m_iTotalLandScored;
}


void CvPlayer::changeTotalLandScored(int iChange)
{
	if (iChange != 0)
	{
		changeAssets(-(getLandPlotsAsset(getTotalLandScored())));
		changeLandScore(-(getLandPlotsScore(getTotalLandScored())));

		m_iTotalLandScored = (m_iTotalLandScored + iChange);
		FAssert(getTotalLandScored() >= 0);

		changeAssets(getLandPlotsAsset(getTotalLandScored()));
		changeLandScore(getLandPlotsScore(getTotalLandScored()));
	}
}


int CvPlayer::getGold() const
{
	return m_iGold;
}


void CvPlayer::setGold(int iNewValue)
{
	if (getGold() != iNewValue)
	{
		m_iGold = iNewValue;

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(MiscButtons_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}

void CvPlayer::changeGold(int iChange)
{
	setGold(getGold() + iChange);
}

int CvPlayer::getGoldPerTurn() const
{
	return m_iGoldPerTurn;
}

int CvPlayer::getAdvancedStartPoints() const
{
	return m_iAdvancedStartPoints;
}


void CvPlayer::setAdvancedStartPoints(int iNewValue)
{
	if (getAdvancedStartPoints() != iNewValue)
	{
		m_iAdvancedStartPoints = iNewValue;

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(MiscButtons_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}

void CvPlayer::changeAdvancedStartPoints(int iChange)
{
	setAdvancedStartPoints(getAdvancedStartPoints() + iChange);
}


int CvPlayer::getGoldenAgeTurns() const
{
	return m_iGoldenAgeTurns;
}


bool CvPlayer::isGoldenAge() const
{
	return (getGoldenAgeTurns() > 0);
}


void CvPlayer::changeGoldenAgeTurns(int iChange)
{
	if(iChange == 0)
		return;

	CvWString szBuffer;

	bool bOldGoldenAge = isGoldenAge();

	m_iGoldenAgeTurns = (m_iGoldenAgeTurns + iChange);
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
			GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szBuffer, -1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));

			CvEventReporter::getInstance().goldenAge(getID());
		}
		else
		{
			CvEventReporter::getInstance().endGoldenAge(getID());
		}

		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			CvPlayer const& kObs = GET_PLAYER((PlayerTypes)iI); // advc.003
			if (!kObs.isAlive())
				continue; // advc.003

			if (GET_TEAM(getTeam()).isHasMet(kObs.getTeam())
					|| kObs.isSpectator()) // advc.127
			{
				if (isGoldenAge())
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_PLAYER_GOLDEN_AGE_HAS_BEGUN", getNameKey());
					gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(),
							kObs.getID() == getID(), GC.getEVENT_MESSAGE_TIME(),
							szBuffer, "AS2D_GOLDAGESTART",
							MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY, // advc.106b
							NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
							// <advc.127b>
							getCapitalX(kObs.getTeam(), true),
							getCapitalY(kObs.getTeam(), true)); // </advc.127b>
				}
				else
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_PLAYER_GOLDEN_AGE_ENDED", getNameKey());
					gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(), false,
							GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_GOLDAGEEND",
							MESSAGE_TYPE_MINOR_EVENT, NULL, (ColorTypes)
							GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
							// <advc.127b>
							getCapitalX(kObs.getTeam(), true),
							getCapitalY(kObs.getTeam(), true)); // </advc.127b>
				}
			}
		}
	}

	if (getID() == GC.getGame().getActivePlayer())
	{
		gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
	}
}

int CvPlayer::getGoldenAgeLength() const
{
	return (GC.getGame().goldenAgeLength() * std::max(0, 100 + getGoldenAgeModifier())) / 100;
}

int CvPlayer::getNumUnitGoldenAges() const
{
	return m_iNumUnitGoldenAges;
}


void CvPlayer::changeNumUnitGoldenAges(int iChange)
{
	m_iNumUnitGoldenAges = (m_iNumUnitGoldenAges + iChange);
	FAssert(getNumUnitGoldenAges() >= 0);
}


int CvPlayer::getStrikeTurns() const
{
	return m_iStrikeTurns;
}


void CvPlayer::changeStrikeTurns(int iChange)
{
	m_iStrikeTurns = (m_iStrikeTurns + iChange);
	FAssert(getStrikeTurns() >= 0);
}


int CvPlayer::getAnarchyTurns() const
{
	return m_iAnarchyTurns;
}


bool CvPlayer::isAnarchy() const
{
	return (getAnarchyTurns() > 0);
}


void CvPlayer::changeAnarchyTurns(int iChange) // advc.003: Refactored
{
	if(iChange == 0)
		return;

	CvGame const& g = GC.getGame();
	if(getID() == g.getActivePlayer())
		gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);

	bool bOldAnarchy = isAnarchy();
	m_iAnarchyTurns += iChange;
	FAssert(getAnarchyTurns() >= 0);
	if(bOldAnarchy == isAnarchy())
		return;

	if(getID() == g.getActivePlayer()) {
		gDLL->getInterfaceIFace()->setDirty(MiscButtons_DIRTY_BIT, true);
		// advc.004x:
		gDLL->getInterfaceIFace()->setDirty(ResearchButtons_DIRTY_BIT, true);
	}
	if(getTeam() == g.getActiveTeam())
		gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);

	updateCommerce();
	updateMaintenance();
	updateTradeRoutes();
	updateCorporation();
	GC.getGame().updateTradeRoutes(); // advc.124
	AI_makeAssignWorkDirty();

	if (isAnarchy())
	{
		gDLL->getInterfaceIFace()->addHumanMessage(getID(), true, GC.getEVENT_MESSAGE_TIME(),
				gDLL->getText("TXT_KEY_MISC_REVOLUTION_HAS_BEGUN").GetCString(), "AS2D_REVOLTSTART",
				MESSAGE_TYPE_MINOR_EVENT, // advc.106b: was MAJOR
				NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_WARNING_TEXT"),
				getCapitalX(getTeam()), getCapitalY(getTeam())); // advc.127b
	}
	else
	{
		gDLL->getInterfaceIFace()->addHumanMessage(getID(), false, GC.getEVENT_MESSAGE_TIME(),
				gDLL->getText("TXT_KEY_MISC_REVOLUTION_OVER").GetCString(), "AS2D_REVOLTEND",
				MESSAGE_TYPE_MINOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString(
				"COLOR_WHITE"), // advc.004g: Was COLOR_WARNING_TEXT
				getCapitalX(getTeam()), getCapitalY(getTeam())); // advc.127b
		// K-Mod. trigger production/research popups that have been suppressed.
		if(isHuman())
		{
			if(isResearch() && getCurrentResearch() == NO_TECH)
				chooseTech();
			int iLoop=-1;
			for(CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
			{
				if (pLoopCity->AI_isChooseProductionDirty() &&
						!pLoopCity->isProduction() && !pLoopCity->isDisorder() &&
						!pLoopCity->isProductionAutomated())
					pLoopCity->chooseProduction();
			}
		} // K-Mod end
	}
}


int CvPlayer::getMaxAnarchyTurns() const
{
	return m_iMaxAnarchyTurns;
}


void CvPlayer::updateMaxAnarchyTurns()
{
	int iBestValue = GC.getDefineINT("MAX_ANARCHY_TURNS");

	FAssertMsg((GC.getNumTraitInfos() > 0), "GC.getNumTraitInfos() is less than or equal to zero but is expected to be larger than zero in CvPlayer::updateMaxAnarchyTurns");
	for (int iI = 0; iI < GC.getNumTraitInfos(); iI++)
	{
		if (hasTrait((TraitTypes)iI))
		{
			if (GC.getTraitInfo((TraitTypes)iI).getMaxAnarchy() >= 0)
			{
				if (GC.getTraitInfo((TraitTypes)iI).getMaxAnarchy() < iBestValue)
				{
					iBestValue = GC.getTraitInfo((TraitTypes)iI).getMaxAnarchy();
				}
			}
		}
	}

	m_iMaxAnarchyTurns = iBestValue;
	FAssert(getMaxAnarchyTurns() >= 0);
}


int CvPlayer::getAnarchyModifier() const
{
	return m_iAnarchyModifier;
}


void CvPlayer::changeAnarchyModifier(int iChange)
{
	if (iChange == 0)
		return;

	/* original bts code
	setRevolutionTimer(std::max(0, ((100 + iChange) * getRevolutionTimer()) / 100));
	setConversionTimer(std::max(0, ((100 + iChange) * getConversionTimer()) / 100)); */
	// K-Mod. The original code is wrong, and it is missing the anarchy length change.
	changeRevolutionTimer(getRevolutionTimer() * iChange / std::max(1, 100+getAnarchyModifier()));
	changeConversionTimer(getConversionTimer() * iChange / std::max(1, 100+getAnarchyModifier()));
	changeAnarchyTurns(getAnarchyTurns() * iChange / std::max(1, 100+getAnarchyModifier()));
	// K-Mod end

	m_iAnarchyModifier += iChange;
}


int CvPlayer::getGoldenAgeModifier() const
{
	return m_iGoldenAgeModifier;
}


void CvPlayer::changeGoldenAgeModifier(int iChange)
{
	// K-Mod. If we are currently in a golden age, adjust its duration!
	changeGoldenAgeTurns(getGoldenAgeTurns() * iChange / std::max(1, 100 + getGoldenAgeModifier()));

	m_iGoldenAgeModifier += iChange;
}


int CvPlayer::getHurryModifier() const
{
	return m_iGlobalHurryModifier;
}


void CvPlayer::changeHurryModifier(int iChange)
{
	m_iGlobalHurryModifier = (m_iGlobalHurryModifier + iChange);
}


int CvPlayer::getGreatPeopleCreated() const
{
	return m_iGreatPeopleCreated;
}


void CvPlayer::incrementGreatPeopleCreated()
{
	m_iGreatPeopleCreated++;
}

int CvPlayer::getGreatGeneralsCreated() const
{
	return m_iGreatGeneralsCreated;
}

void CvPlayer::incrementGreatGeneralsCreated()
{
	m_iGreatGeneralsCreated++;
}

int CvPlayer::getGreatPeopleThresholdModifier() const
{
	return m_iGreatPeopleThresholdModifier;
}


void CvPlayer::changeGreatPeopleThresholdModifier(int iChange)
{
	m_iGreatPeopleThresholdModifier = (m_iGreatPeopleThresholdModifier + iChange);
}


int CvPlayer::getGreatGeneralsThresholdModifier() const
{
	return m_iGreatGeneralsThresholdModifier;
}


void CvPlayer::changeGreatGeneralsThresholdModifier(int iChange)
{
	m_iGreatGeneralsThresholdModifier += iChange;
}


int CvPlayer::getGreatPeopleRateModifier() const
{
	return m_iGreatPeopleRateModifier;
}


void CvPlayer::changeGreatPeopleRateModifier(int iChange)
{
	m_iGreatPeopleRateModifier = (m_iGreatPeopleRateModifier + iChange);
}


int CvPlayer::getGreatGeneralRateModifier() const
{
	return m_iGreatGeneralRateModifier;
}


void CvPlayer::changeGreatGeneralRateModifier(int iChange)
{
	m_iGreatGeneralRateModifier += iChange;
}


int CvPlayer::getDomesticGreatGeneralRateModifier() const
{
	return (GC.getDefineINT("COMBAT_EXPERIENCE_IN_BORDERS_PERCENT") + m_iDomesticGreatGeneralRateModifier);
}


void CvPlayer::changeDomesticGreatGeneralRateModifier(int iChange)
{
	m_iDomesticGreatGeneralRateModifier += iChange;
}


int CvPlayer::getStateReligionGreatPeopleRateModifier() const
{
	return m_iStateReligionGreatPeopleRateModifier;
}


void CvPlayer::changeStateReligionGreatPeopleRateModifier(int iChange)
{
	m_iStateReligionGreatPeopleRateModifier = (m_iStateReligionGreatPeopleRateModifier + iChange);
}


int CvPlayer::getMaxGlobalBuildingProductionModifier() const
{
	return m_iMaxGlobalBuildingProductionModifier;
}


void CvPlayer::changeMaxGlobalBuildingProductionModifier(int iChange)
{
	m_iMaxGlobalBuildingProductionModifier = (m_iMaxGlobalBuildingProductionModifier + iChange);
}


int CvPlayer::getMaxTeamBuildingProductionModifier() const
{
	return m_iMaxTeamBuildingProductionModifier;
}


void CvPlayer::changeMaxTeamBuildingProductionModifier(int iChange)
{
	m_iMaxTeamBuildingProductionModifier = (m_iMaxTeamBuildingProductionModifier + iChange);
}


int CvPlayer::getMaxPlayerBuildingProductionModifier() const
{
	return m_iMaxPlayerBuildingProductionModifier;
}


void CvPlayer::changeMaxPlayerBuildingProductionModifier(int iChange)
{
	m_iMaxPlayerBuildingProductionModifier = (m_iMaxPlayerBuildingProductionModifier + iChange);
}


int CvPlayer::getFreeExperience() const
{
	return m_iFreeExperience;
}


void CvPlayer::changeFreeExperience(int iChange)
{
	m_iFreeExperience = (m_iFreeExperience + iChange);
}


int CvPlayer::getFeatureProductionModifier() const
{
	return m_iFeatureProductionModifier;
}


void CvPlayer::changeFeatureProductionModifier(int iChange)
{
	m_iFeatureProductionModifier = (m_iFeatureProductionModifier + iChange);
}


int CvPlayer::getWorkerSpeedModifier() const
{
	return m_iWorkerSpeedModifier;
}


void CvPlayer::changeWorkerSpeedModifier(int iChange)
{
	m_iWorkerSpeedModifier = (m_iWorkerSpeedModifier + iChange);
}

// <advc.011c>
int CvPlayer::getWorkRate(BuildTypes eBuild) const {

	int iRate = 0;
	CvCivilizationInfo& kCiv = GC.getCivilizationInfo(getCivilizationType());
	for(int i = 0; i < GC.getNumUnitClassInfos(); i++) {
		CvUnitInfo& kUnit = GC.getUnitInfo((UnitTypes)kCiv.getCivilizationUnits(i));
		if (kUnit.getBuilds(eBuild)) {
			iRate = kUnit.getWorkRate();
			break;
		}
	}
	iRate *= std::max(0, 100 + getWorkerSpeedModifier());
	iRate /= 100;
	if(!isHuman() && !isBarbarian()) {
		iRate *= std::max(0, 100 + GC.getHandicapInfo(GC.getGame().getHandicapType()).
				getAIWorkRateModifier() + 100);
		iRate /= 100;
	}
	return iRate;
} // </advc.011c>


int CvPlayer::getImprovementUpgradeRateModifier() const
{
	return m_iImprovementUpgradeRateModifier;
}


void CvPlayer::changeImprovementUpgradeRateModifier(int iChange)
{
	m_iImprovementUpgradeRateModifier = (m_iImprovementUpgradeRateModifier + iChange);
}


int CvPlayer::getMilitaryProductionModifier() const
{
	return m_iMilitaryProductionModifier;
}


void CvPlayer::changeMilitaryProductionModifier(int iChange)
{
	m_iMilitaryProductionModifier = (m_iMilitaryProductionModifier + iChange);
}


int CvPlayer::getSpaceProductionModifier() const
{
	return m_iSpaceProductionModifier;
}


void CvPlayer::changeSpaceProductionModifier(int iChange)
{
	m_iSpaceProductionModifier = (m_iSpaceProductionModifier + iChange);
}


int CvPlayer::getCityDefenseModifier() const
{
	return m_iCityDefenseModifier;
}


void CvPlayer::changeCityDefenseModifier(int iChange)
{
	m_iCityDefenseModifier = (m_iCityDefenseModifier + iChange);
}


int CvPlayer::getNumNukeUnits() const
{
	return m_iNumNukeUnits;
}


void CvPlayer::changeNumNukeUnits(int iChange)
{
	m_iNumNukeUnits = (m_iNumNukeUnits + iChange);
	FAssert(getNumNukeUnits() >= 0);
}


int CvPlayer::getNumOutsideUnits() const
{
	return m_iNumOutsideUnits;
}


void CvPlayer::changeNumOutsideUnits(int iChange)
{
	if (iChange != 0)
	{
		m_iNumOutsideUnits += iChange;
		FAssert(getNumOutsideUnits() >= 0);

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}


int CvPlayer::getBaseFreeUnits() const
{
	return m_iBaseFreeUnits;
}


void CvPlayer::changeBaseFreeUnits(int iChange)
{
	if (iChange != 0)
	{
		m_iBaseFreeUnits = (m_iBaseFreeUnits + iChange);

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}


int CvPlayer::getBaseFreeMilitaryUnits() const
{
	return m_iBaseFreeMilitaryUnits;
}


void CvPlayer::changeBaseFreeMilitaryUnits(int iChange)
{
	if (iChange != 0)
	{
		m_iBaseFreeMilitaryUnits = (m_iBaseFreeMilitaryUnits + iChange);

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}


int CvPlayer::getFreeUnitsPopulationPercent() const
{
	return m_iFreeUnitsPopulationPercent;
}


void CvPlayer::changeFreeUnitsPopulationPercent(int iChange)
{
	if (iChange != 0)
	{
		m_iFreeUnitsPopulationPercent = (m_iFreeUnitsPopulationPercent + iChange);

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}


int CvPlayer::getFreeMilitaryUnitsPopulationPercent() const
{
	return m_iFreeMilitaryUnitsPopulationPercent;
}


void CvPlayer::changeFreeMilitaryUnitsPopulationPercent(int iChange)
{
	if (iChange != 0)
	{
		m_iFreeMilitaryUnitsPopulationPercent = (m_iFreeMilitaryUnitsPopulationPercent + iChange);

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}

// K-Mod
int CvPlayer::getTypicalUnitValue(UnitAITypes eUnitAI, DomainTypes eDomain) const
{
	//UnitTypes eBestUnit = NO_UNIT;
	int iHighestValue = 0;

	for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		UnitTypes eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

		if (eLoopUnit != NO_UNIT &&
			(eUnitAI == NO_UNITAI || GC.getUnitInfo(eLoopUnit).getUnitAIType(eUnitAI)) &&
			(eDomain == NO_DOMAIN || GC.getUnitInfo(eLoopUnit).getDomainType() == eDomain) &&
			canTrain(eLoopUnit))
		{
			// Note: currently the above checks do not consider any resource prerequites.
			int iValue = GC.getGame().AI_combatValue(eLoopUnit);
			if (iValue > iHighestValue)
			{
				iHighestValue = iValue;
				//eBestUnit = eLoopUnit;
			}
		}
	}

	return iHighestValue;
}

int CvPlayer::getGoldPerUnit() const
{	// <dlph.14>
	if(isBarbarian())
		return 0; // </dlph.14>
	return m_iGoldPerUnit;
}


void CvPlayer::changeGoldPerUnit(int iChange)
{
	if (iChange != 0)
	{
		m_iGoldPerUnit = (m_iGoldPerUnit + iChange);

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}


int CvPlayer::getGoldPerMilitaryUnit() const
{	// <dlph.14>
	if(isBarbarian())
		return 0; // </dlph.14>
	return m_iGoldPerMilitaryUnit;
}


void CvPlayer::changeGoldPerMilitaryUnit(int iChange)
{
	if (iChange != 0)
	{
		m_iGoldPerMilitaryUnit = (m_iGoldPerMilitaryUnit + iChange);

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}


int CvPlayer::getExtraUnitCost() const
{
	return m_iExtraUnitCost;
}


void CvPlayer::changeExtraUnitCost(int iChange)
{
	if (iChange != 0)
	{
		m_iExtraUnitCost = (m_iExtraUnitCost + iChange);

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}


int CvPlayer::getNumMilitaryUnits() const
{
	return m_iNumMilitaryUnits;
}


void CvPlayer::changeNumMilitaryUnits(int iChange)
{
	if (iChange != 0)
	{
		m_iNumMilitaryUnits = (m_iNumMilitaryUnits + iChange);
		FAssert(getNumMilitaryUnits() >= 0);

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}


int CvPlayer::getHappyPerMilitaryUnit() const
{
	return m_iHappyPerMilitaryUnit;
}


void CvPlayer::changeHappyPerMilitaryUnit(int iChange)
{
	if (iChange != 0)
	{
		m_iHappyPerMilitaryUnit = (m_iHappyPerMilitaryUnit + iChange);

		AI_makeAssignWorkDirty();
	}
}

// <advc.912c>
int CvPlayer::getLuxuryModifier() const	{ return m_iLuxuryModifier; }

void CvPlayer::changeLuxuryModifier(int iChange) {

	if(iChange == 0)
		return;
	m_iLuxuryModifier += iChange;
	m_iLuxuryModifier = ::range(m_iLuxuryModifier, -100, 1000);
	AI_makeAssignWorkDirty();
} // </advc.912c>


int CvPlayer::getMilitaryFoodProductionCount() const
{
	return m_iMilitaryFoodProductionCount;
}


bool CvPlayer::isMilitaryFoodProduction() const
{
	return (getMilitaryFoodProductionCount() > 0);
}


void CvPlayer::changeMilitaryFoodProductionCount(int iChange)
{
	if (iChange != 0)
	{
		m_iMilitaryFoodProductionCount = (m_iMilitaryFoodProductionCount + iChange);
		FAssert(getMilitaryFoodProductionCount() >= 0);

		if (getTeam() == GC.getGame().getActiveTeam())
		{
			gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
		}
	}
}


int CvPlayer::getHighestUnitLevel()	const
{
	return m_iHighestUnitLevel;
}


void CvPlayer::setHighestUnitLevel(int iNewValue)
{
	m_iHighestUnitLevel = iNewValue;
	FAssert(getHighestUnitLevel() >= 0);
}


int CvPlayer::getMaxConscript() const
{
	return m_iMaxConscript;
}


void CvPlayer::changeMaxConscript(int iChange)
{
	m_iMaxConscript = (m_iMaxConscript + iChange);
	FAssert(getMaxConscript() >= 0);
}


int CvPlayer::getConscriptCount() const
{
	return m_iConscriptCount;
}


void CvPlayer::setConscriptCount(int iNewValue)
{
	m_iConscriptCount = iNewValue;
	FAssert(getConscriptCount() >= 0);
}


void CvPlayer::changeConscriptCount(int iChange)
{
	setConscriptCount(getConscriptCount() + iChange);
}


int CvPlayer::getOverflowResearch() const
{
	return m_iOverflowResearch;
}


void CvPlayer::setOverflowResearch(int iNewValue)
{
	m_iOverflowResearch = iNewValue;
	FAssert(getOverflowResearch() >= 0);
}


void CvPlayer::changeOverflowResearch(int iChange)
{
	setOverflowResearch(getOverflowResearch() + iChange);
}


/* original bts code
int CvPlayer::getNoUnhealthyPopulationCount() const {
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
}*/
/*  K-Mod, 27/dec/10, karadoc
	replace NoUnhealthyPopulation with UnhealthyPopulationModifier */
int CvPlayer::getUnhealthyPopulationModifier() const
{
	return m_iUnhealthyPopulationModifier;
}


void CvPlayer::changeUnhealthyPopulationModifier(int iChange)
{
	m_iUnhealthyPopulationModifier += iChange;
}
// K-Mod end

int CvPlayer::getExpInBorderModifier() const
{
	return m_iExpInBorderModifier;
}


void CvPlayer::changeExpInBorderModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iExpInBorderModifier += iChange;
		FAssert(getExpInBorderModifier() >= 0);
	}
}


int CvPlayer::getBuildingOnlyHealthyCount() const
{
	return m_iBuildingOnlyHealthyCount;
}


bool CvPlayer::isBuildingOnlyHealthy() const
{
	return (getBuildingOnlyHealthyCount() > 0);
}


void CvPlayer::changeBuildingOnlyHealthyCount(int iChange)
{
	if (iChange != 0)
	{
		m_iBuildingOnlyHealthyCount = (m_iBuildingOnlyHealthyCount + iChange);
		FAssert(getBuildingOnlyHealthyCount() >= 0);

		AI_makeAssignWorkDirty();
	}
}


int CvPlayer::getDistanceMaintenanceModifier() const
{
	return m_iDistanceMaintenanceModifier;
}


void CvPlayer::changeDistanceMaintenanceModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iDistanceMaintenanceModifier += iChange;

		updateMaintenance();
	}
}


int CvPlayer::getNumCitiesMaintenanceModifier() const
{
	return m_iNumCitiesMaintenanceModifier;
}


void CvPlayer::changeNumCitiesMaintenanceModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iNumCitiesMaintenanceModifier += iChange;

		updateMaintenance();
	}
}


int CvPlayer::getCorporationMaintenanceModifier() const
{
	return m_iCorporationMaintenanceModifier;
}


void CvPlayer::changeCorporationMaintenanceModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iCorporationMaintenanceModifier += iChange;

		updateMaintenance();
	}
}


int CvPlayer::getTotalMaintenance() const
{
	return m_iTotalMaintenance / 100;
}

// <advc.004b>
int CvPlayer::getTotalMaintenanceTimes100() const
{
	return m_iTotalMaintenance;
} // </advc.004b>

void CvPlayer::changeTotalMaintenance(int iChange)
{
	m_iTotalMaintenance += iChange;
	FAssert(m_iTotalMaintenance >= 0);
}


int CvPlayer::getUpkeepModifier() const
{
	return m_iUpkeepModifier;
}


void CvPlayer::changeUpkeepModifier(int iChange)
{
	m_iUpkeepModifier = (m_iUpkeepModifier + iChange);
}


int CvPlayer::getLevelExperienceModifier() const
{
	return m_iLevelExperienceModifier;
}

void CvPlayer::changeLevelExperienceModifier(int iChange)
{
	m_iLevelExperienceModifier += iChange;
}



int CvPlayer::getExtraHealth() const
{
	return m_iExtraHealth;
}


void CvPlayer::changeExtraHealth(int iChange)
{
	if (iChange != 0)
	{
		m_iExtraHealth = (m_iExtraHealth + iChange);

		AI_makeAssignWorkDirty();
	}
}


int CvPlayer::getBuildingGoodHealth() const
{
	return m_iBuildingGoodHealth;
}


void CvPlayer::changeBuildingGoodHealth(int iChange)
{
	if (iChange != 0)
	{
		m_iBuildingGoodHealth = (m_iBuildingGoodHealth + iChange);
		FAssert(getBuildingGoodHealth() >= 0);

		AI_makeAssignWorkDirty();
	}
}


int CvPlayer::getBuildingBadHealth() const
{
	return m_iBuildingBadHealth;
}


void CvPlayer::changeBuildingBadHealth(int iChange)
{
	if (iChange != 0)
	{
		m_iBuildingBadHealth = (m_iBuildingBadHealth + iChange);
		FAssert(getBuildingBadHealth() <= 0);

		AI_makeAssignWorkDirty();
	}
}


int CvPlayer::getExtraHappiness() const
{
	return m_iExtraHappiness;
}


void CvPlayer::changeExtraHappiness(int iChange)
{
	if (iChange != 0)
	{
		m_iExtraHappiness = (m_iExtraHappiness + iChange);

		AI_makeAssignWorkDirty();
	}
}


int CvPlayer::getBuildingHappiness() const
{
	return m_iBuildingHappiness;
}


void CvPlayer::changeBuildingHappiness(int iChange)
{
	if (iChange != 0)
	{
		m_iBuildingHappiness = (m_iBuildingHappiness + iChange);

		AI_makeAssignWorkDirty();
	}
}


int CvPlayer::getLargestCityHappiness() const
{
	return m_iLargestCityHappiness;
}


void CvPlayer::changeLargestCityHappiness(int iChange)
{
	if (iChange != 0)
	{
		m_iLargestCityHappiness = (m_iLargestCityHappiness + iChange);

		AI_makeAssignWorkDirty();
	}
}


int CvPlayer::getWarWearinessPercentAnger() const
{
	return m_iWarWearinessPercentAnger;
}


void CvPlayer::updateWarWearinessPercentAnger()
{
	int iNewWarWearinessPercentAnger = 0;

	if (!isBarbarian() && !isMinorCiv())
	{
		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			CvTeam& kTeam = GET_TEAM((TeamTypes)iI);
			if (kTeam.isAlive() && !kTeam.isMinorCiv())
			{
				if (kTeam.isAtWar(getTeam()))
				{
					//iNewWarWearinessPercentAnger += (GET_TEAM(getTeam()).getWarWeariness((TeamTypes)iI) * std::max(0, 100 + kTeam.getEnemyWarWearinessModifier())) / 10000;
					iNewWarWearinessPercentAnger += GET_TEAM(getTeam()).getWarWeariness((TeamTypes)iI, true) / 100; // K-Mod
				}
			}
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
	iWarWearinessPercentAnger *= GC.getDefineINT("BASE_WAR_WEARINESS_MULTIPLIER");

	if (GC.getGame().isOption(GAMEOPTION_ALWAYS_WAR) || GC.getGame().isOption(GAMEOPTION_NO_CHANGING_WAR_PEACE))
	{
		iWarWearinessPercentAnger *= std::max(0, (GC.getDefineINT("FORCED_WAR_WAR_WEARINESS_MODIFIER") + 100));
		iWarWearinessPercentAnger /= 100;
	}

	if (GC.getGame().isGameMultiPlayer())
	{
		iWarWearinessPercentAnger *= std::max(0, (GC.getDefineINT("MULTIPLAYER_WAR_WEARINESS_MODIFIER") + 100));
		iWarWearinessPercentAnger /= 100;
	}

	iWarWearinessPercentAnger *= std::max(0, (GC.getWorldInfo(GC.getMap().getWorldSize()).getWarWearinessModifier() + 100));
	iWarWearinessPercentAnger /= 100;

	if (!isHuman() && !isBarbarian() && !isMinorCiv())
	{
		iWarWearinessPercentAnger *= GC.getHandicapInfo(GC.getGame().getHandicapType()).getAIWarWearinessPercent();
		iWarWearinessPercentAnger /= 100;
		// advc.251: No longer adjusted to handicap
		/*iWarWearinessPercentAnger *= std::max(0, ((GC.getHandicapInfo(GC.getGame().getHandicapType()).getAIPerEraModifier() * getCurrentEra()) + 100));
		iWarWearinessPercentAnger /= 100;*/
	}

	return iWarWearinessPercentAnger;
}


int CvPlayer::getWarWearinessModifier() const
{
	return m_iWarWearinessModifier;
}


void CvPlayer::changeWarWearinessModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iWarWearinessModifier = (m_iWarWearinessModifier + iChange);

		AI_makeAssignWorkDirty();
	}
}


int CvPlayer::getFreeSpecialist() const
{
	return m_iFreeSpecialist;
}


void CvPlayer::changeFreeSpecialist(int iChange)
{
	if (iChange != 0)
	{
		m_iFreeSpecialist = (m_iFreeSpecialist + iChange);
		FAssert(getFreeSpecialist() >= 0);

		AI_makeAssignWorkDirty();
	}
}


int CvPlayer::getNoForeignTradeCount() const
{
	return m_iNoForeignTradeCount;
}


bool CvPlayer::isNoForeignTrade() const
{
	return (getNoForeignTradeCount() > 0);
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


int CvPlayer::getNoCorporationsCount() const
{
	return m_iNoCorporationsCount;
}


bool CvPlayer::isNoCorporations() const
{
	return (getNoCorporationsCount() > 0);
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


int CvPlayer::getNoForeignCorporationsCount() const
{
	return m_iNoForeignCorporationsCount;
}


bool CvPlayer::isNoForeignCorporations() const
{
	return (getNoForeignCorporationsCount() > 0);
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


int CvPlayer::getCoastalTradeRoutes() const
{
	return m_iCoastalTradeRoutes;
}


void CvPlayer::changeCoastalTradeRoutes(int iChange)
{
	if (iChange != 0)
	{
		m_iCoastalTradeRoutes = (m_iCoastalTradeRoutes + iChange);
		FAssert(getCoastalTradeRoutes() >= 0);

		updateTradeRoutes();
	}
}


int CvPlayer::getTradeRoutes() const
{
	return m_iTradeRoutes;
}


void CvPlayer::changeTradeRoutes(int iChange)
{
	if (iChange != 0)
	{
		m_iTradeRoutes = (m_iTradeRoutes + iChange);
		FAssert(getTradeRoutes() >= 0);

		updateTradeRoutes();
	}
}


int CvPlayer::getRevolutionTimer() const
{
	return m_iRevolutionTimer;
}


void CvPlayer::setRevolutionTimer(int iNewValue)
{
	if (getRevolutionTimer() != iNewValue)
	{
		m_iRevolutionTimer = iNewValue;
		FAssert(getRevolutionTimer() >= 0);

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(MiscButtons_DIRTY_BIT, true);
		}
	}
}


void CvPlayer::changeRevolutionTimer(int iChange)
{
	setRevolutionTimer(getRevolutionTimer() + iChange);
}


int CvPlayer::getConversionTimer() const
{
	return m_iConversionTimer;
}


void CvPlayer::setConversionTimer(int iNewValue)
{
	if (getConversionTimer() != iNewValue)
	{
		m_iConversionTimer = iNewValue;
		FAssert(getConversionTimer() >= 0);

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(MiscButtons_DIRTY_BIT, true);
		}
	}
}


void CvPlayer::changeConversionTimer(int iChange)
{
	setConversionTimer(getConversionTimer() + iChange);
}


int CvPlayer::getStateReligionCount() const
{
	return m_iStateReligionCount;
}


bool CvPlayer::isStateReligion() const
{
	return (getStateReligionCount() > 0);
}


void CvPlayer::changeStateReligionCount(int iChange)
{
	if (iChange != 0)
	{
		// religion visibility now part of espionage
		//GC.getGame().updateCitySight(false, true);

		m_iStateReligionCount = (m_iStateReligionCount + iChange);
		FAssert(getStateReligionCount() >= 0);

		// religion visibility now part of espionage
		//GC.getGame().updateCitySight(true, true);

		updateMaintenance();
		updateReligionHappiness();
		updateReligionCommerce();

		GC.getGame().AI_makeAssignWorkDirty();

		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
	}
}


int CvPlayer::getNoNonStateReligionSpreadCount() const
{
	return m_iNoNonStateReligionSpreadCount;
}


bool CvPlayer::isNoNonStateReligionSpread() const
{
	return (getNoNonStateReligionSpreadCount() > 0);
}


void CvPlayer::changeNoNonStateReligionSpreadCount(int iChange)
{
	m_iNoNonStateReligionSpreadCount = (m_iNoNonStateReligionSpreadCount + iChange);
	FAssert(getNoNonStateReligionSpreadCount() >= 0);
}


int CvPlayer::getStateReligionHappiness() const
{
	return m_iStateReligionHappiness;
}


void CvPlayer::changeStateReligionHappiness(int iChange)
{
	if (iChange != 0)
	{
		m_iStateReligionHappiness = (m_iStateReligionHappiness + iChange);

		updateReligionHappiness();
	}
}


int CvPlayer::getNonStateReligionHappiness() const
{
	return m_iNonStateReligionHappiness;
}


void CvPlayer::changeNonStateReligionHappiness(int iChange)
{
	if (iChange != 0)
	{
		m_iNonStateReligionHappiness = (m_iNonStateReligionHappiness + iChange);

		updateReligionHappiness();
	}
}


int CvPlayer::getStateReligionUnitProductionModifier() const
{
	return m_iStateReligionUnitProductionModifier;
}


void CvPlayer::changeStateReligionUnitProductionModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iStateReligionUnitProductionModifier = (m_iStateReligionUnitProductionModifier + iChange);

		if (getTeam() == GC.getGame().getActiveTeam())
		{
			gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
		}
	}
}


int CvPlayer::getStateReligionBuildingProductionModifier() const
{
	return m_iStateReligionBuildingProductionModifier;
}


void CvPlayer::changeStateReligionBuildingProductionModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iStateReligionBuildingProductionModifier = (m_iStateReligionBuildingProductionModifier + iChange);

		if (getTeam() == GC.getGame().getActiveTeam())
		{
			gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
		}
	}
}


int CvPlayer::getStateReligionFreeExperience() const
{
	return m_iStateReligionFreeExperience;
}


void CvPlayer::changeStateReligionFreeExperience(int iChange)
{
	m_iStateReligionFreeExperience = (m_iStateReligionFreeExperience + iChange);
}


CvCity* CvPlayer::getCapitalCity() const
{
	return getCity(m_iCapitalCityID);
}


void CvPlayer::setCapitalCity(CvCity* pNewCapitalCity)
{
	CvCity* pOldCapitalCity = getCapitalCity();
	if(pOldCapitalCity == pNewCapitalCity)
		return;

	bool bUpdatePlotGroups = ((pOldCapitalCity == NULL) || (pNewCapitalCity == NULL) ||
			(pOldCapitalCity->plot()->getOwnerPlotGroup() !=
			pNewCapitalCity->plot()->getOwnerPlotGroup()));
	if (bUpdatePlotGroups)
	{
		if (pOldCapitalCity != NULL)
		{
			pOldCapitalCity->plot()->updatePlotGroupBonus(false);
		}
		if (pNewCapitalCity != NULL)
		{
			pNewCapitalCity->plot()->updatePlotGroupBonus(false);
		}
	}

	if (pNewCapitalCity != NULL)
	{
		m_iCapitalCityID = pNewCapitalCity->getID();
	}
	else
	{
		m_iCapitalCityID = FFreeList::INVALID_INDEX;
	}

	if (bUpdatePlotGroups)
	{
		if (pOldCapitalCity != NULL)
		{
			pOldCapitalCity->plot()->updatePlotGroupBonus(true);
		}
		if (pNewCapitalCity != NULL)
		{
			pNewCapitalCity->plot()->updatePlotGroupBonus(true);
		}
	}

	updateMaintenance();
	updateTradeRoutes();

	if (pOldCapitalCity != NULL)
	{
		pOldCapitalCity->updateCommerce();

		pOldCapitalCity->setInfoDirty(true);
	}
	if (pNewCapitalCity != NULL)
	{
		pNewCapitalCity->updateCommerce();

		pNewCapitalCity->setInfoDirty(true);
	}
}

// <advc.127b>
int CvPlayer::getCapitalX(TeamTypes eObserver, bool bDebug) const {

	CvCity* cap = getCapitalCity();
	if(cap == NULL || (eObserver != NO_TEAM && !cap->isRevealed(eObserver, bDebug)))
		return -1;
	return cap->getX();
}

int CvPlayer::getCapitalY(TeamTypes eObserver, bool bDebug) const {

	CvCity* cap = getCapitalCity();
	if(cap == NULL || (eObserver != NO_TEAM && !cap->isRevealed(eObserver, bDebug)))
		return -1;
	return cap->getY();
}

int CvPlayer::getCapitalX(PlayerTypes eObserver, bool bDebug) const {
	return getCapitalX(eObserver == NO_PLAYER ? NO_TEAM : TEAMID(eObserver), bDebug);
}
int CvPlayer::getCapitalY(PlayerTypes eObserver, bool bDebug) const {
	return getCapitalY(eObserver == NO_PLAYER ? NO_TEAM : TEAMID(eObserver), bDebug);
} // </advc.127b>


int CvPlayer::getCitiesLost() const
{
	return m_iCitiesLost;
}


void CvPlayer::changeCitiesLost(int iChange)
{
	m_iCitiesLost = (m_iCitiesLost + iChange);
}


int CvPlayer::getWinsVsBarbs() const
{
	return m_iWinsVsBarbs;
}


void CvPlayer::changeWinsVsBarbs(int iChange)
{
	m_iWinsVsBarbs = (m_iWinsVsBarbs + iChange);
	FAssert(getWinsVsBarbs() >= 0);
}


int CvPlayer::getAssets() const
{
	return m_iAssets;
}


void CvPlayer::changeAssets(int iChange)
{
	m_iAssets = (m_iAssets + iChange);
	FAssert(getAssets() >= 0);
}


int CvPlayer::getPower() const
{
	return m_iPower;
}


void CvPlayer::changePower(int iChange)
{
	m_iPower = (m_iPower + iChange);
	FAssert(getPower() >= 0);
}


int CvPlayer::getPopScore(bool bCheckVassal) const
{
	if (bCheckVassal && isAVassal())
	{
		return m_iPopulationScore / 2;
	}

	int iVassalScore = 0;

	if (bCheckVassal)
	{
		for (int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			if (i != getID())
			{
				CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
				if (kLoopPlayer.isAlive() && GET_TEAM(kLoopPlayer.getTeam()).isVassal(getTeam()))
				{
					iVassalScore += kLoopPlayer.getPopScore(false) / 2;
				}
			}
		}
	}

	return (m_iPopulationScore + iVassalScore / std::max(1, GET_TEAM(getTeam()).getNumMembers()));
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


int CvPlayer::getLandScore(bool bCheckVassal) const
{
	if (bCheckVassal && isAVassal())
	{
		return m_iLandScore / 2;
	}

	int iVassalScore = 0;

	if (bCheckVassal)
	{
		for (int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			if (i != getID())
			{
				CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
				if (kLoopPlayer.isAlive() && GET_TEAM(kLoopPlayer.getTeam()).isVassal(getTeam()))
				{
					iVassalScore += kLoopPlayer.getLandScore(false) / 2;
				}
			}
		}
	}

	return (m_iLandScore + iVassalScore  / std::max(1, GET_TEAM(getTeam()).getNumMembers()));
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


int CvPlayer::getWondersScore() const
{
	return m_iWondersScore;
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


int CvPlayer::getTechScore() const
{
	return m_iTechScore;
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

int CvPlayer::getCombatExperience() const
{
	return m_iCombatExperience;
}

void CvPlayer::setCombatExperience(int iExperience)
{
	FAssert(iExperience >= 0);
	// <advc.003>
	if(iExperience == getCombatExperience())
		return;
	m_iCombatExperience = iExperience;
	if(isBarbarian())
		return;
	CvGame& g = GC.getGame();
	// </advc.003>

	int iExperienceThreshold = greatPeopleThreshold(true);
	if (m_iCombatExperience >= iExperienceThreshold && iExperienceThreshold > 0)
	{	// create great person
		CvCity* pBestCity = NULL;
		int iBestValue = MAX_INT;
		int iLoop;
		for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			int iValue = 4 * g.getSorenRandNum(getNumCities(), "Warlord City Selection");

			for (int i = 0; i < NUM_YIELD_TYPES; i++)
			{
				iValue += pLoopCity->findYieldRateRank((YieldTypes)i);
			}
			iValue += pLoopCity->findPopulationRank();

			if (iValue < iBestValue)
			{
				pBestCity = pLoopCity;
				iBestValue = iValue;
			}
		}

		if (pBestCity)
		{
			int iRandOffset = g.getSorenRandNum(GC.getNumUnitInfos(), "Warlord Unit Generation");
			for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
			{
				UnitTypes eLoopUnit = (UnitTypes)((iI + iRandOffset) % GC.getNumUnitInfos());
				if (GC.getUnitInfo(eLoopUnit).getLeaderExperience() > 0 || GC.getUnitInfo(eLoopUnit).getLeaderPromotion() != NO_PROMOTION)
				{
					pBestCity->createGreatPeople(eLoopUnit, false, true);
					setCombatExperience(getCombatExperience() - iExperienceThreshold);
					break;
				}
			}
		}
	} // <advc.078>
	if(getID() == g.getActivePlayer() && getBugOptionBOOL("MainInterface__Combat_Counter", false))
		gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
	// </advc.078>
}

void CvPlayer::changeCombatExperience(int iChange)
{
	setCombatExperience(getCombatExperience() + iChange);
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

void CvPlayer::sendReminder()
{
	CvWString szYearStr;

	// Only perform this step if we have a valid email address on record,
	// and we have provided information about how to send emails
	if (!getPbemEmailAddress().empty() &&
		!gDLL->GetPitbossSmtpHost().empty())
	{
		GAMETEXT.setTimeStr(szYearStr, GC.getGame().getGameTurn(), true);

		// Generate our arguments
		CyArgsList argsList;
		argsList.add(getPbemEmailAddress());
		argsList.add(gDLL->GetPitbossSmtpHost());
		argsList.add(gDLL->GetPitbossSmtpLogin());
		argsList.add(gDLL->GetPitbossSmtpPassword());
		argsList.add(GC.getGame().getName());
		argsList.add(GC.getGame().isMPOption(MPOPTION_TURN_TIMER));
		argsList.add(GC.getGame().getPitbossTurnTime());
		argsList.add(gDLL->GetPitbossEmail());
		argsList.add(szYearStr);

		// Now send our email via Python
		long iResult;
		bool bOK = gDLL->getPythonIFace()->callFunction(PYPitBossModule, "sendEmail", argsList.makeFunctionArgs(), &iResult);

		FAssertMsg(bOK, "Pitboss Python call to onSendEmail failed!");
		FAssertMsg(iResult == 0, "Pitboss Python fn onSendEmail encountered an error");
	}
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
	return ((timeGetTime() - m_uiStartTime)/1000);
}


bool CvPlayer::isMinorCiv() const
{
	return GC.getInitCore().getMinorNationCiv(m_eID);
}


void CvPlayer::setAlive(bool bNewValue)  // advc.003: some style changes
{
	if(isAlive() == bNewValue)
		return;

	m_bAlive = bNewValue;

	CvGame& g = GC.getGame();
	// <advc.003b>
	if(isAlive() && getParent() == NO_PLAYER && !isBarbarian())
		g.changeCivPlayersEverAlive(1);
	GET_TEAM(getTeam()).updateLeaderID(); // </advc.003b>
	GET_TEAM(getTeam()).changeAliveCount(isAlive() ? 1 : -1);

	// Report event to Python
	CvEventReporter::getInstance().setPlayerAlive(getID(), bNewValue);

	if (isAlive())
	{
		if (!isEverAlive())
		{
			m_bEverAlive = true;

			GET_TEAM(getTeam()).changeEverAliveCount(1);
		}

		if (getNumCities() == 0)
		{
			setFoundedFirstCity(false);
		}

		updatePlotGroups();

		if (g.isMPOption(MPOPTION_SIMULTANEOUS_TURNS) ||
				g.getNumGameTurnActive() == 0 ||
				(g.isSimultaneousTeamTurns() && GET_TEAM(getTeam()).isTurnActive()))
			setTurnActive(true);

		gDLL->openSlot(getID());

		// K-Mod. Attitude cache
		if(!isBarbarian()) { // advc.003n
			for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
			{
				GET_PLAYER(i).AI_updateAttitudeCache(getID());
				AI().AI_updateAttitudeCache(i);
			}
		// K-Mod end
		}
	}
	else
	{
		// <advc.001> CvTeam::makePeace does this, but here they missed it
		for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
			CvPlayer& kWarEnemy = GET_PLAYER((PlayerTypes)i);
			if(kWarEnemy.isAlive() && kWarEnemy.getID() != getID() &&
					!kWarEnemy.isMinorCiv() && GET_TEAM(kWarEnemy.getTeam()).isAtWar(getTeam()))
				kWarEnemy.updateWarWearinessPercentAnger();
			/*  The two will, by the way, remain at war for the rest of the game
				(which doesn't matter b/c dead civs get excluded everywhere). */
		} // </advc.001>
		clearResearchQueue();
		killUnits();
		killCities();
		killAllDeals();

		setTurnActive(false);

		gDLL->endMPDiplomacy();
		gDLL->endDiplomacy();

		if (!isHuman())
		{
			gDLL->closeSlot(getID());
		}

		if (g.getElapsedGameTurns() > 0)
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
						gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_CIVDESTROYED", MESSAGE_TYPE_MAJOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_WARNING_TEXT"));
					}
				}

				g.addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szBuffer, -1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_WARNING_TEXT"));
			}
		}
	}

	g.setScoreDirty(true);
	// <advc.700>
	if(g.isOption(GAMEOPTION_RISE_FALL))
		g.getRiseFall().reportElimination(getID()); // </advc.700>
}


void CvPlayer::verifyAlive()
{
	if (isAlive())
	{
		bool bKill = false; // advc.003: Removed superfluous code
		if (!isBarbarian())
		{
			if (getNumCities() == 0 && getAdvancedStartPoints() < 0)
			{
				//if ((getNumUnits() == 0) || (!(GC.getGame().isOption(GAMEOPTION_COMPLETE_KILLS)) && isFoundedFirstCity()))
				// advc.701: COMPLETE_KILLS option removed (replacing the line above)
				if (getNumUnits() <= 0 || isFoundedFirstCity())
				{
					bKill = true;
				}
			}
		}

		if (!bKill)
		{
			if (!isBarbarian())
			{
				if (GC.getGame().getMaxCityElimination() > 0)
				{
					if (getCitiesLost() >= GC.getGame().getMaxCityElimination())
					{
						bKill = true;
					}
				}
			}
		}

		if (bKill)
		{
			setAlive(false);
		}
	}
	else
	{
		if ((getNumCities() > 0) || (getNumUnits() > 0))
		{
			setAlive(true);
		}
	}
}


bool CvPlayer::isTurnActive() const
{
	return m_bTurnActive;
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
	CvGame& g = GC.getGame();
	if (isTurnActive())
	{
		// K-Mod
		AI().updateCacheData();
		onTurnLogging(); // bbai logging
		// K-Mod end

		if (GC.getLogging())
		{
			if (gDLL->getChtLvl() > 0)
			{
				TCHAR szOut[1024];
				sprintf(szOut, "Player %d Turn ON\n", getID());
				gDLL->messageControlLog(szOut);
			}
		}

		FAssertMsg(isAlive(), "isAlive is expected to be true");

		setEndTurn(false);
		g.resetTurnTimer();

		// If we are the Pitboss, send this player an email
		if (gDLL->IsPitbossHost())
		{
			// If this guy is not currently connected, try sending him an email
			if (isHuman() && !isConnected())
			{
				sendReminder();
			}
		}

		if ((g.isHotSeat() || g.isPbem()) && isHuman() && bDoTurn)
		{
			gDLL->getInterfaceIFace()->clearEventMessages();
			// advc.135a: Commented out
			//gDLL->getEngineIFace()->setResourceLayer(false);

			g.setActivePlayer(getID());
		}

		g.changeNumGameTurnActive(1);

		if (bDoTurn)
		{
			if (isAlive() && !isHuman() && !isBarbarian() && (getAdvancedStartPoints() >= 0))
			{
				AI_doAdvancedStart();
			}

			if (g.getElapsedGameTurns() > 0)
			{
				if (isAlive())
				{
					if (g.isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
					{
						doTurn();
					}
					// K-Mod. Call CvTeam::doTurn at the start of this team's turn. ie. when the leader's turn is activated.
					// Note: in simultaneous turns mode this is called by CvGame::doTurn,
					// because from here we can't tell which player in each team will be activated first.
					else
					{
						if (GET_TEAM(getTeam()).getLeaderID() == getID())
							GET_TEAM(getTeam()).doTurn();
					}
					// K-Mod end

					doTurnUnits();
				}
			}

			if (getID() == g.getActivePlayer() && g.getElapsedGameTurns() > 0)
			{
				if (g.isNetworkMultiPlayer())
				{
					gDLL->getInterfaceIFace()->addHumanMessage(getID(), true, GC.getEVENT_MESSAGE_TIME(), gDLL->getText("TXT_KEY_MISC_TURN_BEGINS").GetCString(), "AS2D_NEWTURN", MESSAGE_TYPE_DISPLAY_ONLY);
				}
				else
				{
					gDLL->getInterfaceIFace()->playGeneralSound("AS2D_NEWTURN");
				}
			}
			// <advc.706> Skip warnings and messages if only pausing for civ selection
			if(!g.isOption(GAMEOPTION_RISE_FALL) ||
					!g.getRiseFall().isSelectingCiv()) { // </advc.706>
				doWarnings();
				// <advc.106b>
				if(isHuman()) {
					validateDiplomacy(); // advc.001e
					if(g.getActivePlayer() == getID()) {
						/*  Make sure that Python events like Civ4lerts are
							triggered before processing messages */
						CyArgsList pyArgs;
						pyArgs.add(g.getTurnSlice());
						CvEventReporter::getInstance().genericEvent("gameUpdate", pyArgs.makeFunctionArgs());
						postProcessMessages();
					}
				}
			} // advc.706
			// <advc.700>
			if(isHuman() || isHumanDisabled()) {
				if(g.isOption(GAMEOPTION_RISE_FALL))
					g.getRiseFall().atActiveTurnStart();
			} // </advc.700>
			// Clear messages in any case (in particular during AIAutoPlay)
			for(size_t i = 0; i < m_aMajorMsgs.size(); i++)
				SAFE_DELETE(m_aMajorMsgs[i]);
			m_aMajorMsgs.clear();
			// </106b>
		}

		if (getID() == g.getActivePlayer())
		{
			if (gDLL->getInterfaceIFace()->getLengthSelectionList() == 0)
			{
				g.cycleSelectionGroups_delayed(1, false);
			}

			gDLL->getInterfaceIFace()->setDirty(SelectionCamera_DIRTY_BIT, true);
		}
	}
	else
	{
		if (GC.getLogging())
		{
			if (gDLL->getChtLvl() > 0)
			{
				TCHAR szOut[1024];
				sprintf(szOut, "Player %d Turn OFF\n", getID());
				gDLL->messageControlLog(szOut);
			}
		}

		if (getID() == g.getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setForcePopup(false);
			gDLL->getInterfaceIFace()->clearQueuedPopups();
			gDLL->getInterfaceIFace()->flushTalkingHeadMessages();
		}

		// start profiling DLL if desired
		if (getID() == g.getActivePlayer())
		{
			startProfilingDLL(true);
		}

		g.changeNumGameTurnActive(-1);

		if (bDoTurn)
		{
			if (!g.isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
			{
				if (isAlive())
				{
					doTurn();
				}

				if ((g.isPbem() || g.isHotSeat()) && isHuman() &&
						g.countHumanPlayersAlive() > 1)
				{
					g.setHotPbemBetweenTurns(true);
				}

				if (g.isSimultaneousTeamTurns())
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
					for (int iI = (getID() + 1); iI < MAX_PLAYERS; iI++)
					{
						if (GET_PLAYER((PlayerTypes)iI).isAlive())
						{
							if (g.isPbem() && GET_PLAYER((PlayerTypes)iI).isHuman())
							{
								if (!g.getPbemTurnSent())
								{
									gDLL->sendPbemTurn((PlayerTypes)iI);
								}
							}
							else
							{
								GET_PLAYER((PlayerTypes)iI).setTurnActive(true);
							}
							break;
						}
					}
				}
			}
		}
	}

	gDLL->getInterfaceIFace()->updateCursorType();

	gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);

	GC.getMap().invalidateActivePlayerSafeRangeCache();
}

// K-Mod. The body of this function use to be part of setTurnActive.
// I've moved it here just to improve the readability of that function.
// (This logging is from bbai.)
void CvPlayer::onTurnLogging() const
{
	if (gPlayerLogLevel > 0)
	{
		logBBAI("Player %d (%S) setTurnActive for turn %d (%d %s)", getID(), getCivilizationDescription(0), GC.getGame().getGameTurn(), std::abs(GC.getGame().getGameTurnYear()), GC.getGame().getGameTurnYear()>0 ? "AD" : "BC");

		if (GC.getGame().getGameTurn() > 0 && (GC.getGame().getGameTurn() % 25) == 0 && !isBarbarian())
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
					iEconomy += getEconomyHistory(iGameTurn - iI);
					iProduction += getIndustryHistory(iGameTurn - iI);
					iAgri += getAgricultureHistory(iGameTurn - iI);
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
				{
					szBuffer.append(CvWString::format(L"%d,", iI));
				}
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
					{
						szBuffer.append(CvWString::format(L"%d,", iI));
					}
				}
			}
		}

		if (GET_TEAM(getTeam()).getAtWarCount(false) > 0)
		{
			szBuffer.append(CvWString::format(L";  at war with: "));

			for(int iI = 0; iI < MAX_CIV_TEAMS; iI++)
			{
				if(iI != getTeam() && GET_TEAM(getTeam()).isAtWar((TeamTypes)iI))
				{
					if (GET_TEAM((TeamTypes)iI).isAlive())
					{
						szBuffer.append(CvWString::format(L"%d,", iI));
					}
				}
			}
		}

		if (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0)
		{
			szBuffer.append(CvWString::format(L";  planning war with: "));

			for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
			{
				if (iI != getTeam() && !GET_TEAM(getTeam()).isAtWar((TeamTypes)iI) && GET_TEAM(getTeam()).AI_getWarPlan((TeamTypes)iI) != NO_WARPLAN)
				{
					if (GET_TEAM((TeamTypes)iI).isAlive())
					{
						szBuffer.append(CvWString::format(L"%d,", iI));
					}
				}
			}
		}

		logBBAI("%S", szBuffer.getCString());

		szBuffer.clear();

		if (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0) logBBAI("    Enemy power perc: %d (%d with others reduction)", GET_TEAM(getTeam()).AI_getEnemyPowerPercent(), GET_TEAM(getTeam()).AI_getEnemyPowerPercent(true));
	}
}
// K-Mod end

bool CvPlayer::isAutoMoves() const
{
	return m_bAutoMoves;
}


void CvPlayer::setAutoMoves(bool bNewValue)
{
	if (isAutoMoves() == bNewValue)
		return; // advc.003

	m_bAutoMoves = bNewValue;

	if (!isAutoMoves())
	{
		if (isEndTurn() || !isHuman())
		{
			setTurnActive(false);
		}
		else
		{
			if (getID() == GC.getGame().getActivePlayer())
			{
				gDLL->getInterfaceIFace()->setCycleSelectionCounter(1);
				//GC.getGame().cycleSelectionGroups_delayed(1, false, true); // this is a subtle case. I think it's best to just use the normal delay
			}
		}
	}
}


bool CvPlayer::isEndTurn() const
{
	return m_bEndTurn;
}


void CvPlayer::setEndTurn(bool bNewValue)
{
	if (isEndTurn() != bNewValue)
	{
		FAssertMsg(isTurnActive(), "isTurnActive is expected to be true");

		m_bEndTurn = bNewValue;

		if (isEndTurn())
		{
			setAutoMoves(true);
		}
	}
}

bool CvPlayer::isTurnDone() const
{
	// if this returns true, popups and diplomacy will wait to appear until next turn
	if (!GC.getGame().isPbem() && !GC.getGame().isHotSeat())
	{
		return false;
	}
	if (!isHuman())
	{
		return true;
	}
	if (!isEndTurn())
	{
		return false;
	}
	return (!isAutoMoves());
}

bool CvPlayer::isExtendedGame() const
{
	return m_bExtendedGame;
}


void CvPlayer::makeExtendedGame()
{
	m_bExtendedGame = true;
}


bool CvPlayer::isFoundedFirstCity() const
{
	return m_bFoundedFirstCity;
}


void CvPlayer::setFoundedFirstCity(bool bNewValue)
{
	if (isFoundedFirstCity() == bNewValue)
		return;

	m_bFoundedFirstCity = bNewValue;
	if (getID() == GC.getGame().getActivePlayer())
	{
		gDLL->getInterfaceIFace()->setDirty(PercentButtons_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(ResearchButtons_DIRTY_BIT, true);
	}
	/*  <advc.104> So that rivals (with higher ids) can immediately evaluate
		war plans against this player  */
	if(!isBarbarian() && getParent() == NO_PLAYER && getWPAI.isEnabled())
		AI().warAndPeaceAI().getCache().update(); // </advc.104>
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


bool CvPlayer::isStrike() const
{
	return m_bStrike;
}


void CvPlayer::setStrike(bool bNewValue)
{
	if (isStrike() != bNewValue)
	{
		m_bStrike = bNewValue;

		if (isStrike())
		{
			if (getID() == GC.getGame().getActivePlayer())
			{
				gDLL->getInterfaceIFace()->addHumanMessage(getID(), false, GC.getEVENT_MESSAGE_TIME(), gDLL->getText("TXT_KEY_MISC_UNITS_ON_STRIKE").GetCString(), "AS2D_STRIKE", MESSAGE_TYPE_MINOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_WARNING_TEXT"));

				gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
			}
		}
	}
}


HandicapTypes CvPlayer::getHandicapType() const
{
	// <advc.127>
	if(isHumanDisabled() && // <advc.706>
			// With R&F, Ctrl+Shift+X still leads to Auto Play with AI handicap.
			(!GC.getGame().isOption(GAMEOPTION_RISE_FALL) ||
			!GC.getGame().getRiseFall().hasRetired()))
			// </advc.706>
		return GC.getGame().getAIHandicap();
	// </advc.127>
	return GC.getInitCore().getHandicap(getID());
}


CivilizationTypes CvPlayer::getCivilizationType() const
{
	return GC.getInitCore().getCiv(getID());
}


LeaderHeadTypes CvPlayer::getLeaderType() const
{
	return GC.getInitCore().getLeader(getID());
}


LeaderHeadTypes CvPlayer::getPersonalityType() const
{
	return m_ePersonalityType;
}


void CvPlayer::setPersonalityType(LeaderHeadTypes eNewValue)
{
	m_ePersonalityType = eNewValue;
}


void CvPlayer::setCurrentEra(EraTypes eNewValue)
{
	if (getCurrentEra() == eNewValue)
		return; // advc.003

	EraTypes eOldEra = m_eCurrentEra;
	m_eCurrentEra = eNewValue;

	if (GC.getGame().getActiveTeam() != NO_TEAM)
	{
		for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
		{
			CvPlot* pLoopPlot = GC.getMap().plotByIndex(iI);
			pLoopPlot->updateGraphicEra();

			if (pLoopPlot->getRevealedImprovementType(GC.getGame().getActiveTeam(), true) != NO_IMPROVEMENT)
			{
				if ((pLoopPlot->getOwner() == getID()) || (!(pLoopPlot->isOwned()) && (getID() == GC.getGame().getActivePlayer())))
				{
					pLoopPlot->setLayoutDirty(true);
				}
			}
		}
	}

	// dirty all of this player's cities...
	int iLoop=-1;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		//if (pLoopCity->getOwner() == getID())
		FAssert(pLoopCity->getOwner() == getID()); // K-Mod
		{
			pLoopCity->setLayoutDirty(true);
		}
	}

	//update unit eras
	for(CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		gDLL->getEntityIFace()->updateGraphicEra(pLoopUnit->getUnitEntity(), eOldEra);
	}

	//update flag eras
	gDLL->getInterfaceIFace()->setDirty(Flag_DIRTY_BIT, true);

	if (getID() == GC.getGame().getActivePlayer())
	{
		gDLL->getInterfaceIFace()->setDirty(Soundtrack_DIRTY_BIT, true);
	}

	if (isHuman() && (getCurrentEra() != GC.getGame().getStartEra()) && !GC.getGame().isNetworkMultiPlayer())
	{
		if (GC.getGame().isFinalInitialized() && !(gDLL->GetWorldBuilderMode()))
		{
			CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN);
			if (NULL != pInfo)
			{
				pInfo->setData1(eNewValue);
				pInfo->setText(L"showEraMovie");
				addPopup(pInfo);
			}
		}
	} // <advc.106>
	if(GC.getDefineINT("SHOW_ENTERED_ERA_IN_REPLAY") > 0) {
		CvWString szBuffer = gDLL->getText("TXT_KEY_SOMEONE_ENTERED_ERA",
				getNameKey(), GC.getEraInfo(eNewValue).getTextKeyWide());
		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT,
				getID(), szBuffer, -1, -1, (ColorTypes)
				GC.getInfoTypeForString("COLOR_ALT_HIGHLIGHT_TEXT"));
	} // </advc.106>
}


ReligionTypes CvPlayer::getLastStateReligion() const
{
	return m_eLastStateReligion;
}


ReligionTypes CvPlayer::getStateReligion() const
{
	return ((isStateReligion()) ? getLastStateReligion() : NO_RELIGION);
}


void CvPlayer::setLastStateReligion(ReligionTypes eNewValue)
{
	if(getLastStateReligion() == eNewValue)
		return;
	FAssert(!isBarbarian()); // advc.003n
	// religion visibility now part of espionage
	//GC.getGame().updateCitySight(false, true);

	ReligionTypes eOldReligion = getLastStateReligion();
	m_eLastStateReligion = eNewValue;

	// religion visibility now part of espionage
	//GC.getGame().updateCitySight(true, true);

	updateMaintenance();
	updateReligionHappiness();
	updateReligionCommerce();

	GC.getGame().updateSecretaryGeneral();

	GC.getGame().AI_makeAssignWorkDirty();

	gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);

	if (!GC.getGame().isFinalInitialized())
		return; // advc.003
	if (gDLL->isDiplomacy() && (gDLL->getDiplomacyPlayer() == getID()))
	{
		gDLL->updateDiplomacyAttitude(true);
	}

	if (!isBarbarian()
			/*  advc.150a: Message superfluous when already reported switch to
				civic that prohibits state religion. */
			&& isStateReligion())
	{
		CvWString szBuffer;
		// <advc.150a> Sufficient to set szBuffer once (always the same message)
		if(eNewValue == NO_RELIGION)
			szBuffer = gDLL->getText("TXT_KEY_MISC_PLAYER_RENOUNCE_RELIGION",
					getNameKey(), GC.getReligionInfo(eOldReligion).getTextKeyWide());
		else {
			szBuffer = gDLL->getText("TXT_KEY_MISC_PLAYER_CONVERT_RELIGION",
					getNameKey(), GC.getReligionInfo(eNewValue).getTextKeyWide());
			// <advc.151>
			if(eOldReligion != NO_RELIGION) {
				szBuffer += L" " + gDLL->getText("TXT_KEY_MISC_AND_RENOUNCE_RELIGION",
						GC.getReligionInfo(eOldReligion).getTextKeyWide());
			} // </advc.151>
		} // </advc.150a>
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			CvPlayer const& kObs = GET_PLAYER((PlayerTypes)iI);
			if (!kObs.isAlive())
				continue; // advc.003
			if (GET_TEAM(getTeam()).isHasMet(kObs.getTeam())
					|| kObs.isSpectator()) // advc.127
			{
				gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_RELIGION_CONVERT", MESSAGE_TYPE_MAJOR_EVENT,
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
	// K-Mod. Attitude cache
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++) // advc.003n: was MAX_PLAYERS
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive() && GET_PLAYER((PlayerTypes)iI).getStateReligion() != NO_RELIGION)
		{
			AI().AI_updateAttitudeCache((PlayerTypes)iI);
			GET_PLAYER((PlayerTypes)iI).AI_updateAttitudeCache(getID());
		}
	} // K-Mod end
}

PlayerTypes CvPlayer::getParent() const
{
	return m_eParent;
}

void CvPlayer::setParent(PlayerTypes eParent)
{
	if(m_eParent == eParent)
		return; // advc.003b
	m_eParent = eParent;
	// <advc.003b>
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
	// </advc.003b>
}

// <advc.003> More convenient access to CvTeam::getMasterTeam
TeamTypes CvPlayer::getMasterTeam() const {

	return GET_TEAM(getTeam()).getMasterTeam();
}
bool CvPlayer::isAVassal() const {

	return GET_TEAM(getTeam()).isAVassal();
}
// </advc.003>

void CvPlayer::updateTeamType()
{
	if(getID() == NO_PLAYER)
	{
		m_eTeamType = NO_TEAM;
	}
	else
	{
		m_eTeamType = GC.getInitCore().getTeam(getID());
	}
}

void CvPlayer::setTeam(TeamTypes eTeam)
{
	FAssert(eTeam != NO_TEAM);
	FAssert(getTeam() != NO_TEAM);
	TeamTypes oldTeam = getTeam(); // advc.003b
	GET_TEAM(getTeam()).changeNumMembers(-1);
	if (isAlive())
	{
		GET_TEAM(getTeam()).changeAliveCount(-1);
	}
	if (isEverAlive())
	{
		GET_TEAM(getTeam()).changeEverAliveCount(-1);
	}
	GET_TEAM(getTeam()).changeNumCities(-(getNumCities()));
	GET_TEAM(getTeam()).changeTotalPopulation(-(getTotalPopulation()));
	GET_TEAM(getTeam()).changeTotalLand(-(getTotalLand()));

	GC.getInitCore().setTeam(getID(), eTeam);

	GET_TEAM(getTeam()).changeNumMembers(1);
	if (isAlive())
	{
		GET_TEAM(getTeam()).changeAliveCount(1);
	}
	if (isEverAlive())
	{
		GET_TEAM(getTeam()).changeEverAliveCount(1);
	} // <advc.003b>
	if(getTeam() != oldTeam) {
		GET_TEAM(getTeam()).updateLeaderID();
		GET_TEAM(oldTeam).updateLeaderID();
	} // </advc.003b>
	GET_TEAM(getTeam()).changeNumCities(getNumCities());
	GET_TEAM(getTeam()).changeTotalPopulation(getTotalPopulation());
	GET_TEAM(getTeam()).changeTotalLand(getTotalLand());
	// <advc.104t>
	if(getWPAI.isEnabled())
		getWPAI.update(); // </advc.104t>
	// K-Mod Attitude cache
	if (GC.getGame().isFinalInitialized()
			&& !isBarbarian()) { // advc.003n
		for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
		{
			AI().AI_updateAttitudeCache(i);
			GET_PLAYER(i).AI_updateAttitudeCache(getID());
		}
	} // K-Mod end
}


PlayerColorTypes CvPlayer::getPlayerColor() const
{
	//return GC.getInitCore().getColor(getID());
	// K-Mod. Conceal the player colour of unmet players.
	if (!concealUnknownCivs() || GET_TEAM(GC.getGame().getActiveTeam()).isHasSeen(getTeam()))
		return GC.getInitCore().getColor(getID());
	else
		return GC.getInitCore().getColor(BARBARIAN_PLAYER);
	// K-Mod end
}


int CvPlayer::getPlayerTextColorR() const
{
	FAssertMsg(getPlayerColor() != NO_PLAYERCOLOR, "getPlayerColor() is not expected to be equal with NO_PLAYERCOLOR");
	return ((int)(GC.getColorInfo((ColorTypes) GC.getPlayerColorInfo(getPlayerColor()).getTextColorType()).getColor().r * 255));
}


int CvPlayer::getPlayerTextColorG() const
{
	FAssertMsg(getPlayerColor() != NO_PLAYERCOLOR, "getPlayerColor() is not expected to be equal with NO_PLAYERCOLOR");
	return ((int)(GC.getColorInfo((ColorTypes) GC.getPlayerColorInfo(getPlayerColor()).getTextColorType()).getColor().g * 255));
}


int CvPlayer::getPlayerTextColorB() const
{
	FAssertMsg(getPlayerColor() != NO_PLAYERCOLOR, "getPlayerColor() is not expected to be equal with NO_PLAYERCOLOR");
	return ((int)(GC.getColorInfo((ColorTypes) GC.getPlayerColorInfo(getPlayerColor()).getTextColorType()).getColor().b * 255));
}


int CvPlayer::getPlayerTextColorA() const
{
	FAssertMsg(getPlayerColor() != NO_PLAYERCOLOR, "getPlayerColor() is not expected to be equal with NO_PLAYERCOLOR");
	return ((int)(GC.getColorInfo((ColorTypes) GC.getPlayerColorInfo(getPlayerColor()).getTextColorType()).getColor().a * 255));
}

// <advc.106>
ColorTypes CvPlayer::getPlayerTextColor() const {

	// Don't call getPlayerColor -- that function conceals the colors of unmet civs.
	return (ColorTypes)GC.getPlayerColorInfo(GC.getInitCore().getColor(getID())).
			getTextColorType();
} // </advc.106>


int CvPlayer::getSeaPlotYield(YieldTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiSeaPlotYield[eIndex];
}


void CvPlayer::changeSeaPlotYield(YieldTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_aiSeaPlotYield[eIndex] = (m_aiSeaPlotYield[eIndex] + iChange);

		updateYield();
	}
}


int CvPlayer::getYieldRateModifier(YieldTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiYieldRateModifier[eIndex];
}


void CvPlayer::changeYieldRateModifier(YieldTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_aiYieldRateModifier[eIndex] = (m_aiYieldRateModifier[eIndex] + iChange);

		invalidateYieldRankCache(eIndex);

		if (eIndex == YIELD_COMMERCE)
		{
			updateCommerce();
		}

		AI_makeAssignWorkDirty();

		if (getTeam() == GC.getGame().getActiveTeam())
		{
			gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
		}
	}
}


int CvPlayer::getCapitalYieldRateModifier(YieldTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiCapitalYieldRateModifier[eIndex];
}


void CvPlayer::changeCapitalYieldRateModifier(YieldTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange == 0)
		return; // advc.003

	m_aiCapitalYieldRateModifier[eIndex] += iChange;
	invalidateYieldRankCache(eIndex);

	CvCity* pCapitalCity = getCapitalCity();
	if (pCapitalCity == NULL)
		return;

	if (eIndex == YIELD_COMMERCE)
		pCapitalCity->updateCommerce();
	pCapitalCity->AI_setAssignWorkDirty(true);
	if (pCapitalCity->getTeam() == GC.getGame().getActiveTeam())
		pCapitalCity->setInfoDirty(true);
}


int CvPlayer::getExtraYieldThreshold(YieldTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiExtraYieldThreshold[eIndex];
}


void CvPlayer::updateExtraYieldThreshold(YieldTypes eIndex)
{
	int iBestValue;
	int iI;

	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	iBestValue = 0;

	FAssertMsg((GC.getNumTraitInfos() > 0), "GC.getNumTraitInfos() is less than or equal to zero but is expected to be larger than zero in CvPlayer::updateExtraYieldThreshold");
	for (iI = 0; iI < GC.getNumTraitInfos(); iI++)
	{
		if (hasTrait((TraitTypes)iI))
		{
			if (GC.getTraitInfo((TraitTypes) iI).getExtraYieldThreshold(eIndex) > 0)
			{
				if ((iBestValue == 0) || (GC.getTraitInfo((TraitTypes) iI).getExtraYieldThreshold(eIndex) < iBestValue))
				{
					iBestValue = GC.getTraitInfo((TraitTypes) iI).getExtraYieldThreshold(eIndex);
				}
			}
		}
	}

	if (getExtraYieldThreshold(eIndex) != iBestValue)
	{
		m_aiExtraYieldThreshold[eIndex] = iBestValue;
		FAssert(getExtraYieldThreshold(eIndex) >= 0);

		updateYield();
	}
}


int CvPlayer::getTradeYieldModifier(YieldTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiTradeYieldModifier[eIndex];
}


void CvPlayer::changeTradeYieldModifier(YieldTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_aiTradeYieldModifier[eIndex] = (m_aiTradeYieldModifier[eIndex] + iChange);

		updateTradeRoutes();
	}
}


int CvPlayer::getFreeCityCommerce(CommerceTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiFreeCityCommerce[eIndex];
}


void CvPlayer::changeFreeCityCommerce(CommerceTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_aiFreeCityCommerce[eIndex] = (m_aiFreeCityCommerce[eIndex] + iChange);
		FAssert(getFreeCityCommerce(eIndex) >= 0);

		updateCommerce(eIndex);
	}
}


int CvPlayer::getCommercePercent(CommerceTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiCommercePercent[eIndex];
}

// K-Mod. This function has been rewritten to enforce the rules of flexible / inflexible commerce types.
// (not all changes marked)
bool CvPlayer::setCommercePercent(CommerceTypes eIndex, int iNewValue, bool bForce)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (!bForce && !isCommerceFlexible(eIndex))
	{
		FAssertMsg(false, "setCommercePercent called without permission.");
		return false; // can't change percent
	}

	int iOldValue = getCommercePercent(eIndex);
	iNewValue = range(iNewValue, 0, 100);

	if (iOldValue == iNewValue)
		return false;

	m_aiCommercePercent[eIndex] = iNewValue;

	int iTotalCommercePercent = 0;

	for (CommerceTypes i = (CommerceTypes)0; i < NUM_COMMERCE_TYPES; i = (CommerceTypes)(i+1))
	{
		iTotalCommercePercent += getCommercePercent(i);
	}

	for (CommerceTypes i = (CommerceTypes)0; iTotalCommercePercent != 100 && i < NUM_COMMERCE_TYPES; i = (CommerceTypes)(i+1))
	{
		if (i != eIndex && isCommerceFlexible(i))
		{
			FAssert(bForce || isCommerceFlexible(i));
			int iAdjustment = std::min(m_aiCommercePercent[i], iTotalCommercePercent - 100);
			m_aiCommercePercent[i] -= iAdjustment;
			iTotalCommercePercent -= iAdjustment;
		}
	}
	// if we couldn't balance the books, we need to do a second pass, with fewer restrictions
	if (iTotalCommercePercent != 100)
	{
		for (CommerceTypes i = (CommerceTypes)0; iTotalCommercePercent != 100 && i < NUM_COMMERCE_TYPES; i = (CommerceTypes)(i+1))
		{
			if (bForce ? i != eIndex : isCommerceFlexible(i))
			{
				FAssert(bForce || isCommerceFlexible(i));
				int iAdjustment = std::min(m_aiCommercePercent[i], iTotalCommercePercent - 100);
				m_aiCommercePercent[i] -= iAdjustment;
				iTotalCommercePercent -= iAdjustment;
			}
		}
	}
	FAssert(100 == iTotalCommercePercent);

	if (iOldValue == getCommercePercent(eIndex))
		return false;

	updateCommerce();
	// K-Mod. For human players, update commerce weight immediately so that they can see effects on working plots, etc.
	if (isHuman() && isTurnActive())
		AI().AI_updateCommerceWeights();
	// K-Mod end
	AI_makeAssignWorkDirty();

	/* original bts code
	if (getTeam() == GC.getGame().getActiveTeam())
	{
		gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(CityScreen_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(Financial_Screen_DIRTY_BIT, true);
	} */
	// K-Mod
	if (getTeam() == GC.getGame().getActiveTeam())
		gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true); // research turns left?

	if (getID() == GC.getGame().getActivePlayer())
	{
		gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(Financial_Screen_DIRTY_BIT, true);
		// <advc.120c>
		// For slider on Espionage screen
		gDLL->getInterfaceIFace()->setDirty(Espionage_Advisor_DIRTY_BIT, true);
		// Redraw +/- buttons if espionage set to 0
		gDLL->getInterfaceIFace()->setDirty(PercentButtons_DIRTY_BIT, true);
		/*  There seems to be some sort of race condition here. Occasionally
			(rarely?), the espionage screen isn't correctly updated when the
			slider goes from 0 to 10. Also, the order of these two setDirty calls
			seems to matter. Dirtying the Espionage_Advisor first works better. */
		// </advc.120c>
	}
	// K-Mod end

	return true;
}


bool CvPlayer::changeCommercePercent(CommerceTypes eIndex, int iChange)
{
	return setCommercePercent(eIndex, (getCommercePercent(eIndex) + iChange));
}


int CvPlayer::getCommerceRate(CommerceTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	int iRate = m_aiCommerceRate[eIndex];
	if (GC.getGame().isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		if (eIndex == COMMERCE_CULTURE)
		{
			iRate += m_aiCommerceRate[COMMERCE_ESPIONAGE];
		}
		else if (eIndex == COMMERCE_ESPIONAGE)
		{
			iRate = 0;
		}
	} // <advc.004x>
	iRate /= 100;
	if(eIndex == COMMERCE_RESEARCH)
		iRate += GC.getBASE_RESEARCH_RATE();
	return iRate; // </advc.004x>
}


void CvPlayer::changeCommerceRate(CommerceTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_aiCommerceRate[eIndex] += iChange;
		FAssert(getCommerceRate(eIndex) >= 0);

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}

int CvPlayer::getCommerceRateModifier(CommerceTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiCommerceRateModifier[eIndex];
}


void CvPlayer::changeCommerceRateModifier(CommerceTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_aiCommerceRateModifier[eIndex] = (m_aiCommerceRateModifier[eIndex] + iChange);

		updateCommerce(eIndex);

		AI_makeAssignWorkDirty();
	}
}


int CvPlayer::getCapitalCommerceRateModifier(CommerceTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiCapitalCommerceRateModifier[eIndex];
}


void CvPlayer::changeCapitalCommerceRateModifier(CommerceTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange == 0)
		return;

	m_aiCapitalCommerceRateModifier[eIndex] = (m_aiCapitalCommerceRateModifier[eIndex] + iChange);

	CvCity* pCapitalCity = getCapitalCity();

	if (pCapitalCity != NULL)
	{
		pCapitalCity->updateCommerce();

		pCapitalCity->AI_setAssignWorkDirty(true);
	}
}


int CvPlayer::getStateReligionBuildingCommerce(CommerceTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiStateReligionBuildingCommerce[eIndex];
}


void CvPlayer::changeStateReligionBuildingCommerce(CommerceTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_aiStateReligionBuildingCommerce[eIndex] = (m_aiStateReligionBuildingCommerce[eIndex] + iChange);
		FAssert(getStateReligionBuildingCommerce(eIndex) >= 0);

		updateCommerce(eIndex);
	}
}


int CvPlayer::getSpecialistExtraCommerce(CommerceTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiSpecialistExtraCommerce[eIndex];
}


void CvPlayer::changeSpecialistExtraCommerce(CommerceTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_aiSpecialistExtraCommerce[eIndex] = (m_aiSpecialistExtraCommerce[eIndex] + iChange);
		FAssert(getSpecialistExtraCommerce(eIndex) >= 0);

		updateCommerce(eIndex);

		AI_makeAssignWorkDirty();
	}
}


int CvPlayer::getCommerceFlexibleCount(CommerceTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiCommerceFlexibleCount[eIndex];
}


bool CvPlayer::isCommerceFlexible(CommerceTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	/* if (!isFoundedFirstCity())
		return false;
	if (eIndex == COMMERCE_ESPIONAGE) {
		if (0 == GET_TEAM(getTeam()).getHasMetCivCount(true) || GC.getGame().isOption(GAMEOPTION_NO_ESPIONAGE))
			return false;
	} */ // Disabled by K-Mod. (I don't want to enforce these conditions in such a fundamental way.)

	return (GC.getCommerceInfo(eIndex).isFlexiblePercent() || (getCommerceFlexibleCount(eIndex) > 0) || GET_TEAM(getTeam()).isCommerceFlexible(eIndex));
}


void CvPlayer::changeCommerceFlexibleCount(CommerceTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_aiCommerceFlexibleCount[eIndex] = (m_aiCommerceFlexibleCount[eIndex] + iChange);
		FAssert(getCommerceFlexibleCount(eIndex) >= 0);

		if (!isCommerceFlexible(eIndex))
		{
			//setCommercePercent(eIndex, 0);
			setCommercePercent(eIndex, GC.getCommerceInfo(eIndex).getInitialPercent(), true); // K-Mod
		}

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(PercentButtons_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}


int CvPlayer::getGoldPerTurnByPlayer(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiGoldPerTurnByPlayer[eIndex];
}


void CvPlayer::changeGoldPerTurnByPlayer(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_iGoldPerTurn = (m_iGoldPerTurn + iChange);
		m_aiGoldPerTurnByPlayer[eIndex] = (m_aiGoldPerTurnByPlayer[eIndex] + iChange);

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		}

		if (!isHuman())
		{
			AI_doCommerce();
		}
	}
}


bool CvPlayer::isFeatAccomplished(FeatTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_FEAT_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_abFeatAccomplished[eIndex];
}


void CvPlayer::setFeatAccomplished(FeatTypes eIndex, bool bNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_FEAT_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_abFeatAccomplished[eIndex] = bNewValue;
}


bool CvPlayer::isOption(PlayerOptionTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_PLAYEROPTION_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	/*  <advc.127> AI Auto-Play should probably disable player options temporarily,
		but I don't think it does and this looks like a quick way to fix the problem.
		AI players can't have options. Tagging advc.001. */
	if(!isHuman())
		return false; // </advc.127>
	return m_abOptions[eIndex];
}


void CvPlayer::setOption(PlayerOptionTypes eIndex, bool bNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_PLAYEROPTION_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_abOptions[eIndex] = bNewValue;
}

bool CvPlayer::isPlayable() const
{
	return GC.getInitCore().getPlayableCiv(getID());
}

void CvPlayer::setPlayable(bool bNewValue)
{
	GC.getInitCore().setPlayableCiv(getID(), bNewValue);
}


int CvPlayer::getBonusExport(BonusTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiBonusExport[eIndex];
}


void CvPlayer::changeBonusExport(BonusTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	if(iChange == 0)
		return; // advc.003
	CvCity* pCapitalCity = getCapitalCity();
	if (pCapitalCity != NULL)
	{
		pCapitalCity->plot()->updatePlotGroupBonus(false);
	}
	m_paiBonusExport[eIndex] = (m_paiBonusExport[eIndex] + iChange);
	FAssert(getBonusExport(eIndex) >= 0);
	if (pCapitalCity != NULL)
	{
		pCapitalCity->plot()->updatePlotGroupBonus(true);
	}
	AI_updateBonusValue(); // advc.036
}


int CvPlayer::getBonusImport(BonusTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiBonusImport[eIndex];
}


void CvPlayer::changeBonusImport(BonusTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange == 0)
		return; // advc.003
	CvCity* pCapitalCity = getCapitalCity();
	if (pCapitalCity != NULL)
	{
		pCapitalCity->plot()->updatePlotGroupBonus(false);
	}
	m_paiBonusImport[eIndex] = (m_paiBonusImport[eIndex] + iChange);
	FAssert(getBonusImport(eIndex) >= 0);
	if (pCapitalCity != NULL)
	{
		pCapitalCity->plot()->updatePlotGroupBonus(true);
	}
	AI_updateBonusValue(); // advc.036
}


int CvPlayer::getImprovementCount(ImprovementTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumImprovementInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiImprovementCount[eIndex];
}


void CvPlayer::changeImprovementCount(ImprovementTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumImprovementInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiImprovementCount[eIndex] = (m_paiImprovementCount[eIndex] + iChange);
	FAssert(getImprovementCount(eIndex) >= 0);
}


int CvPlayer::getFreeBuildingCount(BuildingTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiFreeBuildingCount[eIndex];
}


bool CvPlayer::isBuildingFree(BuildingTypes eIndex)	const
{
	return (getFreeBuildingCount(eIndex) > 0);
}


void CvPlayer::changeFreeBuildingCount(BuildingTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange == 0)
		return;

	int iOldFreeBuildingCount = getFreeBuildingCount(eIndex);

	m_paiFreeBuildingCount[eIndex] = (m_paiFreeBuildingCount[eIndex] + iChange);
	FAssert(getFreeBuildingCount(eIndex) >= 0);

	CvCity* pLoopCity;
	int iLoop;
	if (iOldFreeBuildingCount == 0)
	{
		FAssertMsg(getFreeBuildingCount(eIndex) > 0, "getFreeBuildingCount(eIndex) is expected to be greater than 0");

		for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			pLoopCity->setNumFreeBuilding(eIndex, 1);
		}
	}
	else if (getFreeBuildingCount(eIndex) == 0)
	{
		FAssertMsg(iOldFreeBuildingCount > 0, "iOldFreeBuildingCount is expected to be greater than 0");

		for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			pLoopCity->setNumFreeBuilding(eIndex, 0);
		}
	}

}


int CvPlayer::getExtraBuildingHappiness(BuildingTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiExtraBuildingHappiness[eIndex];
}


void CvPlayer::changeExtraBuildingHappiness(BuildingTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_paiExtraBuildingHappiness[eIndex] += iChange;

		updateExtraBuildingHappiness();
	}
}

int CvPlayer::getExtraBuildingHealth(BuildingTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiExtraBuildingHealth[eIndex];
}


void CvPlayer::changeExtraBuildingHealth(BuildingTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_paiExtraBuildingHealth[eIndex] += iChange;

		updateExtraBuildingHealth();
	}
}


int CvPlayer::getFeatureHappiness(FeatureTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumFeatureInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiFeatureHappiness[eIndex];
}


void CvPlayer::changeFeatureHappiness(FeatureTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumFeatureInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_paiFeatureHappiness[eIndex] = (m_paiFeatureHappiness[eIndex] + iChange);

		updateFeatureHappiness();
	}
}


int CvPlayer::getUnitClassCount(UnitClassTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUnitClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiUnitClassCount[eIndex];
}


bool CvPlayer::isUnitClassMaxedOut(UnitClassTypes eIndex, int iExtra) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUnitClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (!isNationalUnitClass(eIndex))
	{
		return false;
	}

	FAssertMsg(getUnitClassCount(eIndex) <= GC.getUnitClassInfo(eIndex).getMaxPlayerInstances(), "getUnitClassCount is expected to be less than maximum bound of MaxPlayerInstances (invalid index)");

	return ((getUnitClassCount(eIndex) + iExtra) >= GC.getUnitClassInfo(eIndex).getMaxPlayerInstances());
}


void CvPlayer::changeUnitClassCount(UnitClassTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUnitClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiUnitClassCount[eIndex] = (m_paiUnitClassCount[eIndex] + iChange);
	FAssert(getUnitClassCount(eIndex) >= 0);
}


int CvPlayer::getUnitClassMaking(UnitClassTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUnitClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiUnitClassMaking[eIndex];
}


void CvPlayer::changeUnitClassMaking(UnitClassTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUnitClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_paiUnitClassMaking[eIndex] = (m_paiUnitClassMaking[eIndex] + iChange);
		FAssert(getUnitClassMaking(eIndex) >= 0);

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(Help_DIRTY_BIT, true);
		}
	}
}


int CvPlayer::getUnitClassCountPlusMaking(UnitClassTypes eIndex) const
{
	return (getUnitClassCount(eIndex) + getUnitClassMaking(eIndex));
}


int CvPlayer::getBuildingClassCount(BuildingClassTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiBuildingClassCount[eIndex];
}


bool CvPlayer::isBuildingClassMaxedOut(BuildingClassTypes eIndex, int iExtra) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (!isNationalWonderClass(eIndex))
	{
		return false;
	}

	FAssertMsg(getBuildingClassCount(eIndex) <= (GC.getBuildingClassInfo(eIndex).getMaxPlayerInstances() + GC.getBuildingClassInfo(eIndex).getExtraPlayerInstances()), "BuildingClassCount is expected to be less than or match the number of max player instances plus extra player instances");

	return ((getBuildingClassCount(eIndex) + iExtra) >= (GC.getBuildingClassInfo(eIndex).getMaxPlayerInstances() + GC.getBuildingClassInfo(eIndex).getExtraPlayerInstances()));
}


void CvPlayer::changeBuildingClassCount(BuildingClassTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiBuildingClassCount[eIndex] = (m_paiBuildingClassCount[eIndex] + iChange);
	FAssert(getBuildingClassCount(eIndex) >= 0);
}


int CvPlayer::getBuildingClassMaking(BuildingClassTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiBuildingClassMaking[eIndex];
}


void CvPlayer::changeBuildingClassMaking(BuildingClassTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_paiBuildingClassMaking[eIndex] = (m_paiBuildingClassMaking[eIndex] + iChange);
		FAssert(getBuildingClassMaking(eIndex) >= 0);

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(Help_DIRTY_BIT, true);
		}
	}
}


int CvPlayer::getBuildingClassCountPlusMaking(BuildingClassTypes eIndex) const
{
	return (getBuildingClassCount(eIndex) + getBuildingClassMaking(eIndex));
}


int CvPlayer::getHurryCount(HurryTypes eIndex) const
{
	FAssert(eIndex >= 0);
	FAssert(eIndex < GC.getNumHurryInfos());
	return m_paiHurryCount[eIndex];
}


bool CvPlayer::canHurry(HurryTypes eIndex) const
{
	return (getHurryCount(eIndex) > 0);
}


bool CvPlayer::canPopRush() const
{
	return (m_iPopRushHurryCount > 0);
}

// <advc.064b>
bool CvPlayer::canGoldRush() const {

	return (m_iGoldRushHurryCount > 0);
} // </advc.064b>


void CvPlayer::changeHurryCount(HurryTypes eIndex, int iChange)
{
	FAssert(eIndex >= 0);
	FAssert(eIndex < GC.getNumHurryInfos());

	int oldHurryCount = m_paiHurryCount[eIndex];
	m_paiHurryCount[eIndex] = (m_paiHurryCount[eIndex] + iChange);
	FAssert(getHurryCount(eIndex) >= 0);

	// if we just went from 0 to 1 (or the reverse)
	if ((oldHurryCount > 0) != (m_paiHurryCount[eIndex] > 0))
	{
		// does this hurry reduce population?
		if (GC.getHurryInfo(eIndex).getProductionPerPopulation() > 0)
		{
			m_iPopRushHurryCount += iChange;
			FAssert(m_iPopRushHurryCount >= 0);
		} // <advc.064b>
		if(GC.getHurryInfo(eIndex).getGoldPerProduction() > 0) {
			m_iGoldRushHurryCount += iChange;
			FAssert(m_iGoldRushHurryCount >= 0);
		} // </advc.064b>
	}
}

int CvPlayer::getSpecialBuildingNotRequiredCount(SpecialBuildingTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumSpecialBuildingInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiSpecialBuildingNotRequiredCount[eIndex];
}


bool CvPlayer::isSpecialBuildingNotRequired(SpecialBuildingTypes eIndex) const
{
	return (getSpecialBuildingNotRequiredCount(eIndex) > 0);
}


void CvPlayer::changeSpecialBuildingNotRequiredCount(SpecialBuildingTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumSpecialBuildingInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiSpecialBuildingNotRequiredCount[eIndex] = (m_paiSpecialBuildingNotRequiredCount[eIndex] + iChange);
	FAssert(getSpecialBuildingNotRequiredCount(eIndex) >= 0);
}


int CvPlayer::getHasCivicOptionCount(CivicOptionTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCivicOptionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiHasCivicOptionCount[eIndex];
}


bool CvPlayer::isHasCivicOption(CivicOptionTypes eIndex) const
{
	return (getHasCivicOptionCount(eIndex) > 0);
}


void CvPlayer::changeHasCivicOptionCount(CivicOptionTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCivicOptionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiHasCivicOptionCount[eIndex] = (m_paiHasCivicOptionCount[eIndex] + iChange);
	FAssert(getHasCivicOptionCount(eIndex) >= 0);
}


int CvPlayer::getNoCivicUpkeepCount(CivicOptionTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCivicOptionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiNoCivicUpkeepCount[eIndex];
}


bool CvPlayer::isNoCivicUpkeep(CivicOptionTypes eIndex) const
{
	return (getNoCivicUpkeepCount(eIndex) > 0);
}


void CvPlayer::changeNoCivicUpkeepCount(CivicOptionTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCivicOptionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_paiNoCivicUpkeepCount[eIndex] = (m_paiNoCivicUpkeepCount[eIndex] + iChange);
		FAssert(getNoCivicUpkeepCount(eIndex) >= 0);

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}


int CvPlayer::getHasReligionCount(ReligionTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumReligionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiHasReligionCount[eIndex];
}


int CvPlayer::countTotalHasReligion() const
{
	int iCount;
	int iI;

	iCount = 0;

	for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		iCount += getHasReligionCount((ReligionTypes)iI);
	}

	return iCount;
}

int CvPlayer::getHasCorporationCount(CorporationTypes eIndex) const
{
	if (!isActiveCorporation(eIndex))
	{
		return 0;
	}

	return m_paiHasCorporationCount[eIndex];
}


int CvPlayer::countTotalHasCorporation() const
{
	int iCount = 0;

	for (int iI = 0; iI < GC.getNumCorporationInfos(); iI++)
	{
		iCount += getHasCorporationCount((CorporationTypes)iI);
	}

	return iCount;
}

bool CvPlayer::isActiveCorporation(CorporationTypes eIndex) const
{
	if (isNoCorporations())
	{
		return false;
	}

	if (isNoForeignCorporations() && !hasHeadquarters(eIndex))
	{
		return false;
	}

	return true;
}


int CvPlayer::findHighestHasReligionCount() const
{
	int iBestValue = 0;

	for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		int iValue = getHasReligionCount((ReligionTypes)iI);

		if (iValue > iBestValue)
		{
			iBestValue = iValue;
		}
	}

	return iBestValue;
}


void CvPlayer::changeHasReligionCount(ReligionTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumReligionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_paiHasReligionCount[eIndex] = (m_paiHasReligionCount[eIndex] + iChange);
		FAssert(getHasReligionCount(eIndex) >= 0);

		GC.getGame().updateBuildingCommerce();

		GC.getGame().AI_makeAssignWorkDirty();
	}
}


// <advc.132> Body basically from CvPlayerAI::AI_religionTrade (thresholds tweaked)
bool CvPlayer::isMajorReligion(ReligionTypes eReligion) const {

	int iReligionCities = getHasReligionCount(eReligion);
	if(getStateReligion() == NO_RELIGION)
		return iReligionCities > getNumCities() / 3;
	int iOldReligionCities = getHasReligionCount(getStateReligion());
	/*  Not necessarily equal to the number of cities with neither religion,
		but close enough. */
	int iOtherCities = getNumCities() - iReligionCities - iOldReligionCities;
	return (iReligionCities > iOtherCities / 3 +
			std::max(0, iOldReligionCities - 1) / 2);
} // </advc.132>


void CvPlayer::changeHasCorporationCount(CorporationTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCorporationInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_paiHasCorporationCount[eIndex] += iChange;
		FAssert(getHasCorporationCount(eIndex) >= 0);

		GC.getGame().updateBuildingCommerce();

		GC.getGame().AI_makeAssignWorkDirty();
	}
}


int CvPlayer::getUpkeepCount(UpkeepTypes eIndex) const
{
	FAssertMsg(false, "m_paiUpkeepCount is unused"); // advc.003j
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUpkeepInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(m_paiUpkeepCount != NULL, "m_paiUpkeepCount is not expected to be equal with NULL");
	return m_paiUpkeepCount[eIndex];
}


void CvPlayer::changeUpkeepCount(UpkeepTypes eIndex, int iChange)
{
	FAssertMsg(false, "m_paiUpkeepCount is unused"); // advc.003j
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUpkeepInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		FAssertMsg(m_paiUpkeepCount != NULL, "m_paiUpkeepCount is not expected to be equal with NULL");
		m_paiUpkeepCount[eIndex] = (m_paiUpkeepCount[eIndex] + iChange);
		FAssertMsg(getUpkeepCount(eIndex) >= 0, "getUpkeepCount(eIndex) is expected to be non-negative (invalid Index)");

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}


int CvPlayer::getSpecialistValidCount(SpecialistTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumSpecialistInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(m_paiSpecialistValidCount != NULL, "m_paiSpecialistValidCount is not expected to be equal with NULL");
	return m_paiSpecialistValidCount[eIndex];
}


bool CvPlayer::isSpecialistValid(SpecialistTypes eIndex) const
{
	return (getSpecialistValidCount(eIndex) > 0);
}


void CvPlayer::changeSpecialistValidCount(SpecialistTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumSpecialistInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		FAssertMsg(m_paiSpecialistValidCount != NULL, "m_paiSpecialistValidCount is not expected to be equal with NULL");
		m_paiSpecialistValidCount[eIndex] = (m_paiSpecialistValidCount[eIndex] + iChange);
		FAssertMsg(getSpecialistValidCount(eIndex) >= 0, "getSpecialistValidCount(eIndex) is expected to be non-negative (invalid Index)");

		AI_makeAssignWorkDirty();
	}
}


bool CvPlayer::isResearchingTech(TechTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumTechInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_pabResearchingTech[eIndex];
}


void CvPlayer::setResearchingTech(TechTypes eIndex, bool bNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumTechInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (isResearchingTech(eIndex) != bNewValue)
	{
		m_pabResearchingTech[eIndex] = bNewValue;

		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setDirty(Popup_DIRTY_BIT, true); // to check whether we still need the tech chooser popup
		}
	}
}

bool CvPlayer::isLoyalMember(VoteSourceTypes eVoteSource) const
{
	FAssertMsg(eVoteSource >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eVoteSource < GC.getNumVoteSourceInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_pabLoyalMember[eVoteSource];
}

void CvPlayer::setLoyalMember(VoteSourceTypes eVoteSource, bool bNewValue)
{
	FAssertMsg(eVoteSource >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eVoteSource < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (isLoyalMember(eVoteSource) != bNewValue)
	{
		processVoteSourceBonus(eVoteSource, false);
		m_pabLoyalMember[eVoteSource] = bNewValue;
		processVoteSourceBonus(eVoteSource, true);

		GC.getGame().updateSecretaryGeneral();
	}
}

CivicTypes CvPlayer::getCivics(CivicOptionTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCivicOptionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paeCivics[eIndex];
}


int CvPlayer::getSingleCivicUpkeep(CivicTypes eCivic, bool bIgnoreAnarchy,
		int iExtraCities) const // advc.004b
{
	if (eCivic == NO_CIVIC)
	{
		return 0;
	}

	if (isNoCivicUpkeep((CivicOptionTypes)(GC.getCivicInfo(eCivic).getCivicOptionType())))
	{
		return 0;
	}

	if (GC.getCivicInfo(eCivic).getUpkeep() == NO_UPKEEP)
	{
		return 0;
	}

	if (!bIgnoreAnarchy)
	{
		if (isAnarchy())
		{
			return 0;
		}
	}

	int iUpkeep = 0;

	iUpkeep += ((std::max(0, (getTotalPopulation() +
			(iExtraCities * CvCity::initialPopulation()) + // advc.004b
			GC.getDefineINT("UPKEEP_POPULATION_OFFSET") - GC.getCivicInfo(eCivic).getCivicOptionType())) * GC.getUpkeepInfo((UpkeepTypes)(GC.getCivicInfo(eCivic).getUpkeep())).getPopulationPercent()) / 100);
	iUpkeep += ((std::max(0, (getNumCities() +
			iExtraCities + // advc.004b
			GC.getDefineINT("UPKEEP_CITY_OFFSET") + GC.getCivicInfo(eCivic).getCivicOptionType() - (GC.getNumCivicOptionInfos() / 2))) * GC.getUpkeepInfo((UpkeepTypes)(GC.getCivicInfo(eCivic).getUpkeep())).getCityPercent()) / 100);

	iUpkeep *= std::max(0, (getUpkeepModifier() + 100));
	iUpkeep /= 100;

	iUpkeep *= GC.getHandicapInfo(getHandicapType()).getCivicUpkeepPercent();
	iUpkeep /= 100;

	if (!isHuman() && !isBarbarian())
	{
		iUpkeep *= GC.getHandicapInfo(GC.getGame().getHandicapType()).getAICivicUpkeepPercent();
		iUpkeep /= 100;
		// advc.251: Gold costs are no longer adjusted to handicap
		/*iUpkeep *= std::max(0, ((GC.getHandicapInfo(GC.getGame().getHandicapType()).getAIPerEraModifier() * getCurrentEra()) + 100));
		iUpkeep /= 100;*/
	}

	return std::max(0, iUpkeep);
}


int CvPlayer::getCivicUpkeep(CivicTypes* paeCivics, bool bIgnoreAnarchy,
		int iExtraCities) const // advc.004b
{
	int iTotalUpkeep;
	int iI;

	if (paeCivics == NULL)
	{
		paeCivics = m_paeCivics;
	}

	iTotalUpkeep = 0;

	for (iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
	{
		iTotalUpkeep += getSingleCivicUpkeep(paeCivics[iI], bIgnoreAnarchy,
				iExtraCities); // advc.400b
	}

	return iTotalUpkeep;
}


void CvPlayer::setCivics(CivicOptionTypes eIndex, CivicTypes eNewValue)
{
	CvWString szBuffer;

	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumCivicOptionInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(eNewValue >= 0, "eNewValue is expected to be non-negative (invalid Index)");
	FAssertMsg(eNewValue < GC.getNumCivicInfos(), "eNewValue is expected to be within maximum bounds (invalid Index)");

	CivicTypes eOldCivic = getCivics(eIndex);

	if(eOldCivic == eNewValue)
		return;

	bool bWasStateReligion = isStateReligion(); // advc.106

	m_paeCivics[eIndex] = eNewValue;
	if (eOldCivic != NO_CIVIC)
		processCivics(eOldCivic, -1);
	if (getCivics(eIndex) != NO_CIVIC)
		processCivics(getCivics(eIndex), 1);

	CvGame& g = GC.getGame();
	g.updateSecretaryGeneral();
	g.AI_makeAssignWorkDirty();

	if(!g.isFinalInitialized() || /* advc.003n: */ isBarbarian())
		return;

	if (gDLL->isDiplomacy() && (gDLL->getDiplomacyPlayer() == getID()))
		gDLL->updateDiplomacyAttitude(true);

	if (getCivics(eIndex) != NO_CIVIC)
	{	/* original code (which erroneously blocked the message for certain civic switches)
		if (getCivics(eIndex) != GC.getCivilizationInfo(getCivilizationType()).getCivilizationInitialCivics(eIndex))*/
		if (eOldCivic != NO_CIVIC) // K-Mod
		{	// <advc.151> Moved out of the loop
			szBuffer = gDLL->getText("TXT_KEY_MISC_PLAYER_ADOPTED_CIVIC", getNameKey(),
					GC.getCivicInfo(getCivics(eIndex)).getTextKeyWide());
			// <advc.106>
			bool bRenounce = (!GC.getCivicInfo(getCivics(eIndex)).isStateReligion() &&
					GC.getCivicInfo(eOldCivic).isStateReligion() && bWasStateReligion &&
					getLastStateReligion() != NO_RELIGION);
			if(bRenounce) {
				szBuffer += L" " +  gDLL->getText("TXT_KEY_MISC_AND_RENOUNCE_RELIGION",
						GC.getReligionInfo(getLastStateReligion()).getTextKeyWide());
			}
			else // </advc.106>
			if(eOldCivic != GC.getCivilizationInfo(getCivilizationType()).
					getCivilizationInitialCivics(eIndex)) {
				szBuffer += L" " + gDLL->getText("TXT_KEY_MISC_AND_ABOLISH_CIVIC",
						GC.getCivicInfo(eOldCivic).getTextKeyWide());
			} // </advc.151>
			for (int iI = 0; iI < MAX_PLAYERS; iI++)
			{
				CvPlayer const& kObs = GET_PLAYER((PlayerTypes)iI);
				if (!kObs.isAlive() || !GET_TEAM(getTeam()).isHasMet(kObs.getTeam()))
					continue; // advc.003
				gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(), false,
						GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_CIVIC_ADOPT",
						bRenounce ? MESSAGE_TYPE_MAJOR_EVENT : // advc.106
						MESSAGE_TYPE_MINOR_EVENT, // advc.106b
						// advc.127b:
						NULL, NO_COLOR, getCapitalX(kObs.getTeam()), getCapitalY(kObs.getTeam()));
			}
			if(bRenounce) { // advc.106
				szBuffer = gDLL->getText("TXT_KEY_MISC_PLAYER_ADOPTED_CIVIC", getNameKey(),
						GC.getCivicInfo(getCivics(eIndex)).getTextKeyWide());
				// <advc.106>
				szBuffer += L" " + gDLL->getText("TXT_KEY_MISC_AND_RENOUNCE_RELIGION",
						GC.getReligionInfo(getLastStateReligion()).getTextKeyWide());
				// </advc.106>
				g.addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szBuffer);
			}
		}
	} // K-Mod. (environmentalism can change this. It's nice to see the effects immediately.)
	GC.getGame().updateGwPercentAnger();

	// K-Mod. Attitude cache.
	for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
	{
		AI().AI_updateAttitudeCache(i);
		GET_PLAYER(i).AI_updateAttitudeCache(getID());
	} // K-Mod end
}


int CvPlayer::getSpecialistExtraYield(SpecialistTypes eIndex1, YieldTypes eIndex2) const
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 expected to be >= 0");
	FAssertMsg(eIndex1 < GC.getNumSpecialistInfos(), "eIndex1 expected to be < GC.getNumSpecialistInfos()");
	FAssertMsg(eIndex2 >= 0, "eIndex2 expected to be >= 0");
	FAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 expected to be < NUM_YIELD_TYPES");
	return m_ppaaiSpecialistExtraYield[eIndex1][eIndex2];
}


void CvPlayer::changeSpecialistExtraYield(SpecialistTypes eIndex1, YieldTypes eIndex2, int iChange)
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 expected to be >= 0");
	FAssertMsg(eIndex1 < GC.getNumSpecialistInfos(), "eIndex1 expected to be < GC.getNumSpecialistInfos()");
	FAssertMsg(eIndex2 >= 0, "eIndex2 expected to be >= 0");
	FAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_ppaaiSpecialistExtraYield[eIndex1][eIndex2] = (m_ppaaiSpecialistExtraYield[eIndex1][eIndex2] + iChange);
		FAssert(getSpecialistExtraYield(eIndex1, eIndex2) >= 0);

		updateExtraSpecialistYield();

		AI_makeAssignWorkDirty();
	}
}


int CvPlayer::getImprovementYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2) const
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex1 < GC.getNumImprovementInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");
	return m_ppaaiImprovementYieldChange[eIndex1][eIndex2];
}


void CvPlayer::changeImprovementYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2, int iChange)
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex1 < GC.getNumImprovementInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_ppaaiImprovementYieldChange[eIndex1][eIndex2] = (m_ppaaiImprovementYieldChange[eIndex1][eIndex2] + iChange);
		// FAssert(getImprovementYieldChange(eIndex1, eIndex2) >= 0); // Towns in K-Mod get -1 commerce with serfdom.

		updateYield();
	}
}


// K-Mod. I've changed this function from using pUnit to using pGroup.
// I've also rewritten most of the code, to give more natural ordering, and to be more robust and readable code.
void CvPlayer::updateGroupCycle(CvSelectionGroup* pGroup)
{
	PROFILE_FUNC();
	FAssert(pGroup);

	CvPlot* pPlot = pGroup->plot();

	if (!pPlot || !pGroup->isCycleGroup())
		return;

	CvUnit* pUnit = pGroup->getHeadUnit();
	FAssert(pUnit);

	//removeGroupCycle(pGroup->getID()); // will be removed while we reposition it

	CLLNode<int>* pBestSelectionGroupNode = NULL;
	int iBestCost = MAX_INT;

	CvSelectionGroup* pPreviousGroup = NULL;

	CLLNode<int>* pSelectionGroupNode = headGroupCycleNode();

	while (pSelectionGroupNode)
	{
		CvSelectionGroup* pNextGroup = getSelectionGroup(pSelectionGroupNode->m_data);
		FAssert(pNextGroup);

		// if we find our group in the list, remove it.
		if (pNextGroup == pGroup)
		{
			pSelectionGroupNode = deleteGroupCycleNode(pSelectionGroupNode);
		}
		else if (pNextGroup->isCycleGroup() && pNextGroup->canAllMove())
		{
			//int iCost = groupCycleDistance(pPreviousGroup, pGroup) + groupCycleDistance(pGroup, pNextGroup) - groupCycleDistance(pPreviousGroup, pNextGroup);
			int iCost = groupCycleDistance(pGroup, pNextGroup) + (pPreviousGroup ? groupCycleDistance(pPreviousGroup, pGroup) - groupCycleDistance(pPreviousGroup, pNextGroup) : 3);
			if (iCost < iBestCost)
			{
				iBestCost = iCost;
				pBestSelectionGroupNode = pSelectionGroupNode;
			}
			pPreviousGroup = pNextGroup;
			pSelectionGroupNode = nextGroupCycleNode(pSelectionGroupNode);
		}
		else
		{
			pSelectionGroupNode = nextGroupCycleNode(pSelectionGroupNode);
		}
	}
	if (pPreviousGroup)
	{
		FAssert(pPreviousGroup->isCycleGroup() && pPreviousGroup->canAllMove());
		int iCost = groupCycleDistance(pPreviousGroup, pGroup) + 3; // cost for being at the end of the list.
		if (iCost < iBestCost)
		{
			pBestSelectionGroupNode = 0;
		}
	}

	if (pBestSelectionGroupNode)
	{
		m_groupCycle.insertBefore(pUnit->getGroupID(), pBestSelectionGroupNode);
	}
	else
	{
		m_groupCycle.insertAtEnd(pUnit->getGroupID());
	}
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
		else
		{
			pSelectionGroupNode = nextGroupCycleNode(pSelectionGroupNode);
		}
	}
}

// K-Mod
void CvPlayer::refreshGroupCycleList()
{
	std::vector<CvSelectionGroup*> update_list;

	CLLNode<int>* pNode = headGroupCycleNode();
	while (pNode)
	{
		CvSelectionGroup* pLoopGroup = getSelectionGroup(pNode->m_data);
		CvUnit* pLoopHead = pLoopGroup->getHeadUnit();
		if (pLoopHead && pLoopGroup->isCycleGroup() && pLoopGroup->canAllMove() && (pLoopHead->hasMoved() || (pLoopHead->isCargo() && pLoopHead->getTransportUnit()->hasMoved())))
		{
			update_list.push_back(pLoopGroup);
			pNode = deleteGroupCycleNode(pNode);
		}
		else
			pNode = nextGroupCycleNode(pNode);
	}

	for (size_t i = 0; i < update_list.size(); i++)
	{
		updateGroupCycle(update_list[i]);
	}
}
// K-Mod end

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
	for (int i = 0; i < GC.getNUM_AND_TECH_PREREQS(); i++)
	{
		TechTypes ePreReq = (TechTypes)GC.getTechInfo(eTech).getPrereqAndTechs(i);

		if (ePreReq != NO_TECH)
		{
			iPathLength += findPathLength(ePreReq, bCost);
		}
	}

	TechTypes eShortestOr = NO_TECH;
	iShortestPath = MAX_INT;
	//	Find the shortest OR tech
	for (int i = 0; i < GC.getNUM_OR_TECH_PREREQS(); i++)
	{
		//	Grab the tech
		TechTypes ePreReq = (TechTypes)GC.getTechInfo(eTech).getPrereqOrTechs(i);

		//	If this is a valid tech
		if (ePreReq != NO_TECH)
		{
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
	}

	//	If the shortest OR is a valid tech, add the steps to it...
	if (eShortestOr != NO_TECH)
	{
		iPathLength += iShortestPath;
	}

	return (iPathLength + ((bCost) ? GET_TEAM(getTeam()).getResearchCost(eTech) : 1));
}


//	Function specifically for python/tech chooser screen
int CvPlayer::getQueuePosition(TechTypes eTech) const
{
	int i = 1;
	CLLNode<TechTypes>* pResearchNode;

	for (pResearchNode = headResearchQueueNode(); pResearchNode; pResearchNode = nextResearchQueueNode(pResearchNode))
	{
		if (pResearchNode->m_data == eTech)
		{
			return i;
		}
		i++;
	}

	return -1;
}


void CvPlayer::clearResearchQueue()
{
	m_researchQueue.clear();

	for (int iI = 0; iI < GC.getNumTechInfos(); iI++)
	{
		setResearchingTech((TechTypes)iI, false);
	}

	if (getTeam() == GC.getGame().getActiveTeam())
	{
		gDLL->getInterfaceIFace()->setDirty(ResearchButtons_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
	}
}


//	Pushes research onto the queue.  If it is an append if will put it
//	and its pre-reqs into the queue.  If it is not an append it will change
//	research immediately and should be used with clear.  Clear will clear the entire queue.
bool CvPlayer::pushResearch(TechTypes eTech, bool bClear,
		bool bKillPopup) // advc.004x
{
	FAssertMsg(eTech != NO_TECH, "Tech is not assigned a valid value");

	if (GET_TEAM(getTeam()).isHasTech(eTech) || isResearchingTech(eTech))
	{
		//	We have this tech, no reason to add this to the pre-reqs
		return true;
	}

	if (!canEverResearch(eTech))
	{
		return false;
	}
	bool bWasEmpty = (m_researchQueue.getLength() == 0); // advc.004x
	//	Pop the entire queue...
	if (bClear)
	{
		clearResearchQueue();
	}

	//	Add in all the pre-reqs for the and techs...
	for (int i = 0; i < GC.getNUM_AND_TECH_PREREQS(); i++)
	{
		TechTypes ePreReq = (TechTypes)GC.getTechInfo(eTech).getPrereqAndTechs(i);

		if (ePreReq != NO_TECH)
		{
			if (!pushResearch(ePreReq))
			{
				return false;
			}
		}
	}

	// Will return the shortest path of all the or techs.  Tie breaker goes to the first one...
	TechTypes eShortestOr = NO_TECH;
	int iShortestPath = MAX_INT;
	bool bOrPrereqFound = false;
	//	Cycle through all the OR techs
	for (int i = 0; i < GC.getNUM_OR_TECH_PREREQS(); i++)
	{
		TechTypes ePreReq = (TechTypes)GC.getTechInfo(eTech).getPrereqOrTechs(i);

		if (ePreReq == NO_TECH)
			continue; // advc.003

		bOrPrereqFound = true;

		//	If the pre-req exists, and we have it, it is the shortest path, get out, we're done
		if (GET_TEAM(getTeam()).isHasTech(ePreReq))
		{
			eShortestOr = ePreReq;
			break;
		}

		if (canEverResearch(ePreReq))
		{
			//	Find the length of the path to this pre-req
			int iNumSteps = findPathLength(ePreReq);

			//	If this pre-req is a valid tech, and its the shortest current path, set it as such
			if (iNumSteps < iShortestPath)
			{
				eShortestOr = ePreReq;
				iShortestPath = iNumSteps;
			}
		}
	}

	//	If the shortest path tech is valid, push it (and its children) on to the research queue recursively
	if (eShortestOr != NO_TECH)
	{
		if (!pushResearch(eShortestOr))
		{
			return false;
		}
	}
	else if (bOrPrereqFound)
	{
		return false;
	}

	//	Insert this tech at the end of the queue
	m_researchQueue.insertAtEnd(eTech);

	setResearchingTech(eTech, true);

	//	Set the dirty bits
	if (getTeam() == GC.getGame().getActiveTeam())
	{
		gDLL->getInterfaceIFace()->setDirty(ResearchButtons_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
		// <advc.004x>
		if(bKillPopup && bWasEmpty && getID() == GC.getGame().getActivePlayer())
			killAll(BUTTONPOPUP_CHOOSETECH, 0); // </advc.004x>
	}
	// ONEVENT - Tech selected (any)
	CvEventReporter::getInstance().techSelected(eTech, getID());
	return true;
}


//	If bHead is true we delete the entire queue...
void CvPlayer::popResearch(TechTypes eTech)
{
	CLLNode<TechTypes>* pResearchNode;

	for (pResearchNode = headResearchQueueNode(); pResearchNode; pResearchNode = nextResearchQueueNode(pResearchNode))
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
		gDLL->getInterfaceIFace()->setDirty(ResearchButtons_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
	}
}


int CvPlayer::getLengthResearchQueue() const
{
	return m_researchQueue.getLength();
}


CLLNode<TechTypes>* CvPlayer::nextResearchQueueNode(CLLNode<TechTypes>* pNode) const
{
	return m_researchQueue.next(pNode);
}


CLLNode<TechTypes>* CvPlayer::headResearchQueueNode() const
{
	return m_researchQueue.head();
}


CLLNode<TechTypes>* CvPlayer::tailResearchQueueNode() const
{
	return m_researchQueue.tail();
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
	CLLNode<CvWString>* pCityNameNode;

	pCityNameNode = m_cityNames.nodeNum(iIndex);

	if (pCityNameNode != NULL)
	{
		return pCityNameNode->m_data;
	}
	else
	{
		return L"";
	}
}


CLLNode<CvWString>* CvPlayer::nextCityNameNode(CLLNode<CvWString>* pNode) const
{
	return m_cityNames.next(pNode);
}


CLLNode<CvWString>* CvPlayer::headCityNameNode() const
{
	return m_cityNames.head();
}


CvPlotGroup* CvPlayer::firstPlotGroup(int *pIterIdx, bool bRev) const
{
	return !bRev ? m_plotGroups.beginIter(pIterIdx) : m_plotGroups.endIter(pIterIdx);
}


CvPlotGroup* CvPlayer::nextPlotGroup(int *pIterIdx, bool bRev) const
{
	return !bRev ? m_plotGroups.nextIter(pIterIdx) : m_plotGroups.prevIter(pIterIdx);
}


int CvPlayer::getNumPlotGroups() const
{
	return m_plotGroups.getCount();
}


CvPlotGroup* CvPlayer::getPlotGroup(int iID) const
{
	return((CvPlotGroup *)(m_plotGroups.getAt(iID)));
}


CvPlotGroup* CvPlayer::addPlotGroup()
{
	return((CvPlotGroup *)(m_plotGroups.add()));
}


void CvPlayer::deletePlotGroup(int iID)
{
	m_plotGroups.removeAt(iID);
}


CvCity* CvPlayer::firstCity(int *pIterIdx, bool bRev) const
{
	return !bRev ? m_cities.beginIter(pIterIdx) : m_cities.endIter(pIterIdx);
}


CvCity* CvPlayer::nextCity(int *pIterIdx, bool bRev) const
{
	return !bRev ? m_cities.nextIter(pIterIdx) : m_cities.prevIter(pIterIdx);
}


int CvPlayer::getNumCities() const
{
	return m_cities.getCount();
}


CvCity* CvPlayer::getCity(int iID) const
{
	return(m_cities.getAt(iID));
}


CvCity* CvPlayer::addCity()
{
	return(m_cities.add());
}


void CvPlayer::deleteCity(int iID)
{
	m_cities.removeAt(iID);
}


CvUnit* CvPlayer::firstUnit(int *pIterIdx, bool bRev) const
{
	return !bRev ? m_units.beginIter(pIterIdx) : m_units.endIter(pIterIdx);
}


CvUnit* CvPlayer::nextUnit(int *pIterIdx, bool bRev) const
{
	return !bRev ? m_units.nextIter(pIterIdx) : m_units.prevIter(pIterIdx);
}


int CvPlayer::getNumUnits() const
{
	return m_units.getCount();
}


CvUnit* CvPlayer::getUnit(int iID) const
{
	return (m_units.getAt(iID));
}


CvUnit* CvPlayer::addUnit()
{
	return (m_units.add());
}


void CvPlayer::deleteUnit(int iID)
{
	m_units.removeAt(iID);
}


CvSelectionGroup* CvPlayer::firstSelectionGroup(int *pIterIdx, bool bRev) const
{
	return !bRev ? m_selectionGroups.beginIter(pIterIdx) : m_selectionGroups.endIter(pIterIdx);
}


CvSelectionGroup* CvPlayer::nextSelectionGroup(int *pIterIdx, bool bRev) const
{
	return !bRev ? m_selectionGroups.nextIter(pIterIdx) : m_selectionGroups.prevIter(pIterIdx);
}


int CvPlayer::getNumSelectionGroups() const
{
	return m_selectionGroups.getCount();
}


CvSelectionGroup* CvPlayer::getSelectionGroup(int iID) const
{
	return ((CvSelectionGroup *)(m_selectionGroups.getAt(iID)));
}


CvSelectionGroup* CvPlayer::addSelectionGroup()
{
	//return ((CvSelectionGroup *)(m_selectionGroups.add()));
	// K-Mod. Make sure that every time we add a group, it also gets added to the group cycle list.
	// (we can update the specific position in the cycle list later; but it's important it gets into the list.)
	CvSelectionGroup* pGroup = m_selectionGroups.add();
	if (pGroup)
		m_groupCycle.insertAtEnd(pGroup->getID());
	return pGroup;
	// K-Mod end
}


void CvPlayer::deleteSelectionGroup(int iID)
{
	bool bRemoved = m_selectionGroups.removeAt(iID);

	FAssertMsg(bRemoved, "could not find group, delete failed");
}

EventTriggeredData* CvPlayer::firstEventTriggered(int *pIterIdx, bool bRev) const
{
	return !bRev ? m_eventsTriggered.beginIter(pIterIdx) : m_eventsTriggered.endIter(pIterIdx);
}

EventTriggeredData* CvPlayer::nextEventTriggered(int *pIterIdx, bool bRev) const
{
	return !bRev ? m_eventsTriggered.nextIter(pIterIdx) : m_eventsTriggered.prevIter(pIterIdx);
}

int CvPlayer::getNumEventsTriggered() const
{
	return m_eventsTriggered.getCount();
}

EventTriggeredData* CvPlayer::getEventTriggered(int iID) const
{
	return ((EventTriggeredData*)(m_eventsTriggered.getAt(iID)));
}

EventTriggeredData* CvPlayer::addEventTriggered()
{
	return ((EventTriggeredData*)(m_eventsTriggered.add()));
}

void CvPlayer::deleteEventTriggered(int iID)
{
	m_eventsTriggered.removeAt(iID);
}


void CvPlayer::addMessage(const CvTalkingHeadMessage& message)
{
	// <advc.706> Remove messages arriving during interlude from display immediately
	CvGame const& g = GC.getGame();
	if(g.isOption(GAMEOPTION_RISE_FALL) && g.getActivePlayer() == getID() &&
			g.getRiseFall().getInterludeCountdown() >= 0)
		gDLL->getInterfaceIFace()->clearEventMessages();
	// </advc.706>
	m_listGameMessages.push_back(message);
	// <advc.106b>
	// Special treatment only for events in other civs' turns.
	if(!g.isInBetweenTurns() && g.getActivePlayer() == getID())
		return;
	/* DISPLAY_ONLY, COMBAT, CHAT, QUEST don't show up on the Event tab
	   of the Turn Log, and therefore shouldn't count.
	   (That is assuming that quests also send INFO messages, which I haven't
	   verified - tbd.) */
	InterfaceMessageTypes eMessage = message.getMessageType();
	if(eMessage == MESSAGE_TYPE_INFO || eMessage == MESSAGE_TYPE_MINOR_EVENT ||
			eMessage == MESSAGE_TYPE_MAJOR_EVENT || eMessage == MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY)
		m_iNewMessages++; // See comment in postProcessBeginTurnEvents
	if(eMessage == MESSAGE_TYPE_MAJOR_EVENT) {
		/*  Need to make a copy b/c, apparently, the EXE deletes the original
			before postProcessBeginTurnEvents gets called. */
		CvTalkingHeadMessage* pCopy = new CvTalkingHeadMessage(message.getTurn(),
				message.getLength(), message.getDescription(),
				// Don't play it twice
				message.getSoundPlayed() ? NULL : message.getSound(),
				MESSAGE_TYPE_MAJOR_EVENT, message.getIcon(), message.getFlashColor(),
				message.getX(), message.getY(), message.getOffScreenArrows(),
				message.getOnScreenArrows());
		m_aMajorMsgs.push_back(pCopy);
	} // </advc.106b>
}

// <advc.106b>
void CvPlayer::postProcessMessages() {

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
	TCHAR const* relevantNonOffers[] = { "AI_DIPLOCOMMENT_CANCEL_DEAL",
			"AI_DIPLOCOMMENT_RELIGION_PRESSURE", "AI_DIPLOCOMMENT_CIVIC_PRESSURE",
			"AI_DIPLOCOMMENT_JOIN_WAR", "AI_DIPLOCOMMENT_STOP_TRADING",
	};
	if(!m_listDiplomacy.empty() && m_iNewMessages > 0) {
		for(CvDiploQueue::const_iterator it = m_listDiplomacy.begin(); it !=
				m_listDiplomacy.end(); it++) {
			CvDiploParameters* dp = *it;
			if(dp == NULL) {
				FAssert(dp != NULL);
				continue;
			}
			if(dp->getHumanDiplo() || dp->getOurOfferList().getLength() > 0 ||
					dp->getTheirOfferList().getLength() > 0) {
				bRelevantDiplo = true;
				break;
			}
			for(int i = 0; i < sizeof(relevantNonOffers) / sizeof(TCHAR*); i++) {
				if(dp->getDiploComment() == (DiploCommentTypes)GC.getInfoTypeForString(relevantNonOffers[i])) {
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
			GC.getGame().isHotSeat())))) {
		gDLL->getInterfaceIFace()->clearEventMessages();
		if(!GC.getGame().isHotSeat()) {
			/*  Show major events even if the Turn Log gets opened. As with
				NewMessages, CvPlayer needs to keep track of the recent messages;
				use MajorMsgs for that. */
			for(size_t i = 0; i < m_aMajorMsgs.size(); i++)
				gDLL->getInterfaceIFace()->showMessage(*m_aMajorMsgs[i]);
		}
		gDLL->getInterfaceIFace()->showTurnLog();
	}
	// Clear messages in any case
	for(size_t i = 0; i < m_aMajorMsgs.size(); i++)
		SAFE_DELETE(m_aMajorMsgs[i]);
	m_aMajorMsgs.clear();
	GC.getGame().setInBetweenTurns(false);
}

int CvPlayer::getStartOfTurnMessageLimit() const {

	if(!getBugOptionBOOL("MainInterface__AutoOpenEventLog", true))
		return -1;
	int r = getBugOptionINT("MainInterface__MessageLimit", 3);
	if(!isOption(PLAYEROPTION_MINIMIZE_POP_UPS) &&
			GC.getDefineINT("MESSAGE_LIMIT_WITHOUT_MPU") == 0)
		return -1;
	return r;
} // </advc.106b>


void CvPlayer::clearMessages()
{
	m_listGameMessages.clear();
}


const CvMessageQueue& CvPlayer::getGameMessages() const
{
	return (m_listGameMessages);
}


void CvPlayer::expireMessages()
{
	CvMessageQueue::iterator it = m_listGameMessages.begin();
	bool bFoundExpired = false;
	while(it != m_listGameMessages.end())
	{
		CvTalkingHeadMessage& message = *it;
		if (GC.getGame().getGameTurn() >= message.getExpireTurn(
				isHuman() || isHumanDisabled())) // advc.700
		{
			it = m_listGameMessages.erase(it);
			bFoundExpired = true;
		}
		else
		{
			++it;
		}
	}
	if (bFoundExpired)
	{
		gDLL->getInterfaceIFace()->dirtyTurnLog(getID());
	}
}


void CvPlayer::addPopup(CvPopupInfo* pInfo, bool bFront)
{
	if (!isHuman()) {
		SAFE_DELETE(pInfo);
		return;
	} // <advc.004x>
	ButtonPopupTypes eType = pInfo->getButtonPopupType();
	if(eType == BUTTONPOPUP_CHANGERELIGION)
		killAll(BUTTONPOPUP_CHANGERELIGION);
	else if(eType == BUTTONPOPUP_CHANGECIVIC)
		killAll(BUTTONPOPUP_CHANGECIVIC);
	else if(eType == BUTTONPOPUP_CHOOSETECH)
		killAll(BUTTONPOPUP_CHOOSETECH, 0);
	else if(eType == BUTTONPOPUP_PYTHON_SCREEN) {
		CvGame& g = GC.getGame();
		if(g.getElapsedGameTurns() <= 0 && g.getActivePlayer() != NO_PLAYER) {
			// Must be DawnOfMan then
			g.setDawnOfManShown(true);
			bFront = true;
			GET_PLAYER(g.getActivePlayer()).doChangeCivicsPopup(NO_CIVIC);
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
		CvPopupInfo* pInfo = *it;
		if (NULL != pInfo)
		{
			delete pInfo;
		}
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
	return (m_listPopups);
}


void CvPlayer::addDiplomacy(CvDiploParameters* pDiplo)
{
	if (NULL != pDiplo)
	{
		m_listDiplomacy.push_back(pDiplo);
	}
}


void CvPlayer::clearDiplomacy()
{
	CvDiploQueue::iterator it;
	for (it = m_listDiplomacy.begin(); it != m_listDiplomacy.end(); ++it)
	{
		CvDiploParameters* pDiplo = *it;
		if (NULL != pDiplo)
		{
			delete pDiplo;
		}
	}
	m_listDiplomacy.clear();
}


const CvDiploQueue& CvPlayer::getDiplomacy() const
{
	return (m_listDiplomacy);
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
void CvPlayer::validateDiplomacy() {

	bool bDone = false;
	while(!bDone && !m_listDiplomacy.empty()) {
		for(CvDiploQueue::iterator it = m_listDiplomacy.begin(); it != m_listDiplomacy.end(); ++it) {
			CvDiploParameters* pDiplo = *it;
			CLLNode<TradeData>* pNode = pDiplo->getOurOfferList().head(); // Can be NULL!
			// Worst enemy may have changed
			if(pDiplo->getDiploComment() == GC.getInfoTypeForString("AI_DIPLOCOMMENT_STOP_TRADING") &&
					pDiplo->getData() != TEAMREF(pDiplo->getWhoTalkingTo()).AI_getWorstEnemy()) {
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
			else if(pNode != NULL && pNode->m_data.m_eItemType == TRADE_DEFENSIVE_PACT &&
					!GET_TEAM(getTeam()).allWarsShared(TEAMID(pDiplo->getWhoTalkingTo()))) {
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
	for(CvDiploQueue::iterator it = m_listDiplomacy.begin(); it != m_listDiplomacy.end(); ++it) {
		CvDiploParameters* pDiplo = *it;
		if(pDiplo->getDiploComment() != GC.getInfoTypeForString(
				"AI_DIPLOCOMMENT_OFFER_PEACE"))
			continue;
		/*  Make sure that this isn't a peace offer that the EXE will discard for
			valid reasons. I.e. check everything that the EXE checks.
			(Based on tracing DLL calls in the debugger.) */
		PlayerTypes eWho = pDiplo->getWhoTalkingTo();
		CvPlayerAI const& who = GET_PLAYER(eWho);
		// This is what the EXE seems to get wrong
		if(!::atWar(getTeam(), who.getTeam())) {
			apInvalid.push_back(pDiplo);
			continue;
		}
		if(!canContact(eWho) || !who.canContact(getID())) {
			apInvalid.push_back(pDiplo);
			continue;
		}
		CLLNode<TradeData>* pNode = NULL;
		bool bCapitulate = false;
		bool bValid = true;
		for(pNode = pDiplo->getOurOfferList().head(); pNode != NULL;
				pNode = pDiplo->getOurOfferList().next(pNode)) {
			/*  Also test trade denial (although the EXE doesn't do that);
				important for capitulation. */
			if(!canTradeItem(eWho, pNode->m_data, true)) {
				bValid = false;
				break;
			}
			if(pNode->m_data.m_eItemType == TRADE_SURRENDER)
				bCapitulate = true;
		}
		if(!bValid) {
			apInvalid.push_back(pDiplo);
			continue;
		}
		for(pNode = pDiplo->getTheirOfferList().head(); pNode != NULL;
				pNode = pDiplo->getTheirOfferList().next(pNode)) {
			if(!who.canTradeItem(getID(), pNode->m_data, true)) {
				bValid = false;
				break;
			}
			if(pNode->m_data.m_eItemType == TRADE_SURRENDER)
				bCapitulate = true;
		}
		if(!bValid) {
			apInvalid.push_back(pDiplo);
			continue;
		}
		/*  Finally, check if the offer still makes sense for eWho.
			(Sth. the EXE doesn't check.) */
		if(!bCapitulate && !who.AI_upholdPeaceOffer(getID(), *pDiplo)) {
			apInvalid.push_back(pDiplo);
			continue;
		}
		if(iValidPeaceOffers == 0) {
			/*  advancePeaceOfferStage supports only one peace offer per turn
				(this could be amended) */
			GET_TEAM(getTeam()).advancePeaceOfferStage(TEAMID(eWho));
			iValidPeaceOffers++;
		}
		else apInvalid.push_back(pDiplo);
	}
	for(size_t i = 0; i < apInvalid.size(); i++) {
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
		if (NULL != pInfo)
		{
			if(pInfo->getText().compare(L"showSpaceShip") == 0)
			{
				it = m_listPopups.erase(it);
				SAFE_DELETE(pInfo);
			}
			else
			{
				it++;
			}
		}
		else
		{
			it++;
		}
	}
}

// <advc.004x> Partly cut and pasted from CvTeam::setHasTech
void CvPlayer::doChangeCivicsPopup(CivicTypes eCivic) {

	if(!isHuman()) { // Forget reminder during Auto Play
		m_eReminderPending = NO_CIVIC;
		return;
	}
	if(!canRevolution(NULL) || GC.getGame().isAboutToShowDawnOfMan()) {
		m_eReminderPending = (CivicTypes)std::max(m_eReminderPending, eCivic);
		return;
	}
	if(eCivic == NO_CIVIC) { // Then we're only supposed to check for a pending reminder
		if(m_eReminderPending != NO_CIVIC)
			doChangeCivicsPopup(m_eReminderPending);
		return;
	}
	m_eReminderPending = NO_CIVIC;
	CivicOptionTypes eCivicOption = (CivicOptionTypes)GC.getCivicInfo(eCivic).
			getCivicOptionType();
	CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_CHANGECIVIC);
	if(pInfo != NULL) {
		pInfo->setData1(eCivicOption);
		pInfo->setData2(eCivic);
		gDLL->getInterfaceIFace()->addPopup(pInfo, getID());
	}
} // </advc.004x>


int CvPlayer::getScoreHistory(int iTurn) const
{
	CvTurnScoreMap::const_iterator it = m_mapScoreHistory.find(iTurn);
	if (it != m_mapScoreHistory.end())
	{
		return it->second;
	}
	return 0;
}

void CvPlayer::updateScoreHistory(int iTurn, int iBestScore)
{
	m_mapScoreHistory[iTurn] = iBestScore;
}

int CvPlayer::getEconomyHistory(int iTurn) const
{
	CvTurnScoreMap::const_iterator it = m_mapEconomyHistory.find(iTurn);
	if (it != m_mapEconomyHistory.end())
	{
		return it->second;
	}
	return 0;
}

// <advc.004s>
void CvPlayer::updateHistoryMovingAvg(CvTurnScoreMap& history, int iGameTurn, int iNewSample) {

	int iOldSamples = std::min(2, iGameTurn - 1); // Not sure if i=0 would be a valid sample
	int iSamples = iOldSamples;
	int iSum = 0;
	// Discard NewSample if in anarchy
	if(!isAnarchy() || iGameTurn == 0) {
		iSum += iNewSample;
		iSamples++;
	}
	for(int i = iGameTurn - 1; i >= iGameTurn - iOldSamples; i--)
		iSum += history[i];
	history[iGameTurn] = ::round(iSum / (double)std::max(1, iSamples));
}
// </advc.004s>

void CvPlayer::updateEconomyHistory(int iTurn, int iBestEconomy)
{
	//m_mapEconomyHistory[iTurn] = iBestEconomy;
	// advc.004s: Replacing the above
	updateHistoryMovingAvg(m_mapEconomyHistory, iTurn, iBestEconomy);
}

int CvPlayer::getIndustryHistory(int iTurn) const
{
	CvTurnScoreMap::const_iterator it = m_mapIndustryHistory.find(iTurn);
	if (it != m_mapIndustryHistory.end())
	{
		return it->second;
	}
	return 0;
}

void CvPlayer::updateIndustryHistory(int iTurn, int iBestIndustry)
{
	//m_mapIndustryHistory[iTurn] = iBestIndustry;
	// advc.004s: Replacing the above
	updateHistoryMovingAvg(m_mapIndustryHistory, iTurn, iBestIndustry);
}

int CvPlayer::getAgricultureHistory(int iTurn) const
{
	CvTurnScoreMap::const_iterator it = m_mapAgricultureHistory.find(iTurn);
	if (it != m_mapAgricultureHistory.end())
	{
		return it->second;
	}
	return 0;
}

void CvPlayer::updateAgricultureHistory(int iTurn, int iBestAgriculture)
{
	//m_mapAgricultureHistory[iTurn] = iBestAgriculture;
	// advc.004s: Replacing the above
	updateHistoryMovingAvg(m_mapAgricultureHistory, iTurn, iBestAgriculture);
}

int CvPlayer::getPowerHistory(int iTurn) const
{
	CvTurnScoreMap::const_iterator it = m_mapPowerHistory.find(iTurn);
	if (it != m_mapPowerHistory.end())
	{
		return it->second;
	}
	return 0;
}

void CvPlayer::updatePowerHistory(int iTurn, int iBestPower)
{
	m_mapPowerHistory[iTurn] = iBestPower;
}

int CvPlayer::getCultureHistory(int iTurn) const
{
	CvTurnScoreMap::const_iterator it = m_mapCultureHistory.find(iTurn);
	if (it != m_mapCultureHistory.end())
	{
		return it->second;
	}
	return 0;
}

void CvPlayer::updateCultureHistory(int iTurn, int iBestCulture)
{
	//m_mapCultureHistory[iTurn] = iBestCulture;
	// advc.004s: Replacing the above
	updateHistoryMovingAvg(m_mapCultureHistory, iTurn, iBestCulture);
}

int CvPlayer::getEspionageHistory(int iTurn) const
{
	CvTurnScoreMap::const_iterator it = m_mapEspionageHistory.find(iTurn);
	if (it != m_mapEspionageHistory.end())
	{
		return it->second;
	}
	return 0;
}

void CvPlayer::updateEspionageHistory(int iTurn, int iBestEspionage)
{
	//m_mapEspionageHistory[iTurn] = iBestEspionage;
	// advc.004s: Replacing the above
	updateHistoryMovingAvg(m_mapEspionageHistory, iTurn, iBestEspionage);
}

// K-Mod. Note, this function is a friend of CvEventReporter, so that it can access the data we need.
// (This saves us from having to use the built-in CyStatistics class)
const CvPlayerRecord* CvPlayer::getPlayerRecord() const
{
	return CvEventReporter::getInstance().m_kStatistics.getPlayerRecord(getID());
}
// K-Mod end

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
	CyArgsList argsList;
	argsList.add(getID());
	long lResult=0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "doGold", argsList.makeFunctionArgs(), &lResult);
	if(lResult == 1)
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
		{
			bStrike = true;
		}
	}

	if (bStrike)
	{
		setStrike(true);
		changeStrikeTurns(1);

		if (getStrikeTurns() > 1)
		{
			int iDisbandUnit = (getStrikeTurns() / 2); // XXX mod?
			// advc.131: Can happen, but should be quite rare.
			FAssert(isHuman());
			for (int iI = 0; iI < iDisbandUnit; iI++)
			{
				disbandUnit(true);

				if (calculateGoldRate() >= 0)
				{
					break;
				}
			}
		}
	}
	else
	{
		setStrike(false);
	}
}


void CvPlayer::doResearch()
{
	CyArgsList argsList;
	argsList.add(getID());
	long lResult=0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "doResearch", argsList.makeFunctionArgs(), &lResult);
	if(lResult == 1)
		return;

	if (isResearch() /* K-Mod: */ && !isAnarchy())
	{
		bool bForceResearchChoice = false;

		if (getCurrentResearch() == NO_TECH /* K-Mod: */ && isHuman())
		{
			if (getID() == GC.getGame().getActivePlayer())
			{
				chooseTech();
			}
			//if (GC.getGame().getElapsedGameTurns() > 4) { // advc.124g (commented out)
			AI_chooseResearch();
			bForceResearchChoice = true;
		}

		TechTypes eCurrentTech = getCurrentResearch();
		if (eCurrentTech == NO_TECH)
		{
			int iOverflow = (100 * calculateResearchRate()) / std::max(1, calculateResearchModifier(eCurrentTech));
			changeOverflowResearch(iOverflow);
		}
		else
		{
			int iOverflowResearch = (getOverflowResearch() * calculateResearchModifier(eCurrentTech)) / 100;
			setOverflowResearch(0);
			GET_TEAM(getTeam()).changeResearchProgress(eCurrentTech,
					// K-Mod (replacing the minimum which used to be in calculateResearchRate)
					std::max(1, calculateResearchRate()) +
					iOverflowResearch, getID());
		}

		if (bForceResearchChoice)
		{
			clearResearchQueue();
		}
	}
}

void CvPlayer::doEspionagePoints()  // advc.003: some style changes
{
	if (getCommerceRate(COMMERCE_ESPIONAGE) <= 0)
		return;

	GET_TEAM(getTeam()).changeEspionagePointsEver(getCommerceRate(COMMERCE_ESPIONAGE));

	int iSpending = 0;
	// Divide up Espionage between Teams
	for (int iLoop = 0; iLoop < MAX_CIV_TEAMS; iLoop++)
	{
		if (getTeam() == iLoop || !GET_TEAM((TeamTypes)iLoop).isAlive())
			continue;

		if (GET_TEAM(getTeam()).isHasMet((TeamTypes)iLoop))
		{
			iSpending = getEspionageSpending((TeamTypes)iLoop);
			if (iSpending > 0)
			{
				GET_TEAM(getTeam()).changeEspionagePointsAgainstTeam((TeamTypes)iLoop, iSpending);
			}
		}
	}
}

int CvPlayer::getEspionageSpending(TeamTypes eAgainstTeam) const
{
	int iSpendingValue = 0;

	int iTotalPoints = getCommerceRate(COMMERCE_ESPIONAGE);
	int iAvailablePoints = iTotalPoints;

	int iTotalWeight = 0;
	int iBestWeight = 0;

	bool bFoundTeam = false;

	int iLoop;

	// Get sum of all weights to be used later on
	for (iLoop = 0; iLoop < MAX_CIV_TEAMS; iLoop++)
	{
		if (getTeam() != iLoop)
		{
			if (GET_TEAM((TeamTypes)iLoop).isAlive())
			{
				if (GET_TEAM(getTeam()).isHasMet((TeamTypes)iLoop))
				{
					if (iLoop == int(eAgainstTeam))
					{
						bFoundTeam = true;
					}

					int iWeight = getEspionageSpendingWeightAgainstTeam((TeamTypes)iLoop);

					if (iWeight > iBestWeight)
					{
						iBestWeight = iWeight;
					}

					iTotalWeight += iWeight;
				}
			}
		}
	}

	// The player requested is not valid
	if (!bFoundTeam)
	{
		return -1;
	}

	// Split up Espionage Point budget based on weights (if any weights have been assigned)
	if (iTotalWeight > 0)
	{
		for (iLoop = 0; iLoop < MAX_CIV_TEAMS; iLoop++)
		{
			if (getTeam() != iLoop)
			{
				if (GET_TEAM((TeamTypes)iLoop).isAlive())
				{
					if (GET_TEAM(getTeam()).isHasMet((TeamTypes)iLoop))
					{
						int iChange = (iTotalPoints * getEspionageSpendingWeightAgainstTeam((TeamTypes)iLoop) / iTotalWeight);
						iAvailablePoints -= iChange;

						if (iLoop == int(eAgainstTeam))
						{
							iSpendingValue += iChange;
						}
					}
				}
			}
		}
	}

	// Divide remainder evenly among top Teams
	while (iAvailablePoints > 0)
	{
		for (iLoop = 0; iLoop < MAX_CIV_TEAMS; iLoop++)
		{
			if (getTeam() != iLoop)
			{
				if (GET_TEAM((TeamTypes)iLoop).isAlive())
				{
					if (GET_TEAM(getTeam()).isHasMet((TeamTypes)iLoop))
					{
						if (getEspionageSpendingWeightAgainstTeam((TeamTypes)iLoop) == iBestWeight)
						{
							if (iLoop == int(eAgainstTeam))
							{
								++iSpendingValue;
							}
							--iAvailablePoints;

							if (iAvailablePoints <= 0)
							{
								break;
							}
						}
					}
				}
			}
		}
	}

	return iSpendingValue;
}

bool CvPlayer::canDoEspionageMission(EspionageMissionTypes eMission, PlayerTypes eTargetPlayer, const CvPlot* pPlot, int iExtraData, const CvUnit* pUnit,
		bool bCheckPoints) const // advc.085
{
	if (getID() == eTargetPlayer || NO_PLAYER == eTargetPlayer)
		return false;

	if (!GET_PLAYER(eTargetPlayer).isAlive() || !GET_TEAM(getTeam()).isHasMet(TEAMID(eTargetPlayer)))
		return false;

	// K-Mod. Bugfix
	if (pUnit && pPlot && !pUnit->canEspionage(pPlot, false))
		return false;
	// K-Mod end

	CvEspionageMissionInfo& kMission = GC.getEspionageMissionInfo(eMission);
	// Need Tech Prereq, if applicable
	if (kMission.getTechPrereq() != NO_TECH)
	{
		if (!GET_TEAM(getTeam()).isHasTech((TechTypes)kMission.getTechPrereq()))
			return false;
	}

	int iCost = getEspionageMissionCost(eMission, eTargetPlayer, pPlot, iExtraData, pUnit);
	if (iCost < 0)
		return false;

	int iEspionagePoints = GET_TEAM(getTeam()).getEspionagePointsAgainstTeam(TEAMID(eTargetPlayer));
	// <advc.085>
	if (!bCheckPoints)
		return true;
	// </advc.085>
	if (iEspionagePoints < iCost)
		return false;
	if (iEspionagePoints <= 0)
		return false;
	return true;
}

// <advc.120d> Mostly cut-and-paste from getEspionageMissionBaseCost
TechTypes CvPlayer::getStealCostTech(PlayerTypes eTargetPlayer) const {

	TechTypes r = NO_TECH;
	if(eTargetPlayer == NO_PLAYER)
		return r;
	int iProdCost = MAX_INT;
	for(int iTech = 0; iTech < GC.getNumTechInfos(); iTech++) {
		TechTypes eTech = (TechTypes)iTech;
		if(canStealTech(eTargetPlayer, eTech)) {
			int iCost = GET_TEAM(getTeam()).getResearchCost(eTech);
			if(iCost < iProdCost) {
				iProdCost = iCost;
				r = eTech;
			}
		}
	}
	return r;
}

// <advc.104>  (also used for advc.079)
bool CvPlayer::canSeeTech(PlayerTypes eOther) const {

	// Partly based on drawTechDeals in ExoticForeignAdvisor.py
	if(eOther == NO_PLAYER)
		return false;
	CvTeam const& ourTeam = GET_TEAM(getTeam());
	CvTeam const& otherTeam = TEAMREF(eOther);
	if(ourTeam.getID() == otherTeam.getID())
		return true;
	if(otherTeam.isVassal(ourTeam.getID()))
		return true; // Can see tech through "we'd like you to research ..."
	// advc.553: Make tech visible despite No Tech Trading (as in BtS)
	/*if(GC.getGame().isOption(GAMEOPTION_NO_TECH_TRADING))
		return false;*/
	if(!ourTeam.isAlive() || !otherTeam.isAlive() ||
			ourTeam.isBarbarian() || otherTeam.isBarbarian() ||
			ourTeam.isMinorCiv() || otherTeam.isMinorCiv())
		return false;
	return ourTeam.isHasMet(otherTeam.getID()) &&
			(ourTeam.isTechTrading() || otherTeam.isTechTrading());
} // </advc.104>

bool CvPlayer::canSpy() const {

	for(int i = 0; i < GC.getNumUnitInfos(); i++) {
		UnitTypes eUnit = (UnitTypes)i;
		CvUnitInfo const& u = GC.getUnitInfo(eUnit);
		if(u.isSpy() && canTrain(eUnit))
			return true;
	} int foo=-1;
	for(CvUnit* u = firstUnit(&foo); u != NULL; u = nextUnit(&foo)) {
		if(u->isSpy())
			return true;
	}
	return false;
} // </advc.120d>

int CvPlayer::getEspionageMissionCost(EspionageMissionTypes eMission, PlayerTypes eTargetPlayer,
		const CvPlot* pPlot, int iExtraData, const CvUnit* pSpyUnit) const
{
	int iMissionCost = getEspionageMissionBaseCost(eMission, eTargetPlayer, pPlot, iExtraData, pSpyUnit);
	if (iMissionCost < 0)
		return -1;

	// Multiply cost of mission * number of team members
	//iMissionCost *= GET_TEAM(getTeam()).getNumMembers(); // K-Mod
	// dlph.33/advc
	iMissionCost = adjustMissionCostToTeamSize(iMissionCost, eTargetPlayer);

	iMissionCost *= getEspionageMissionCostModifier(eMission, eTargetPlayer, pPlot, iExtraData, pSpyUnit);
	iMissionCost /= 100;

	return std::max(0, iMissionCost);
}

// advc: Auxiliary function for dlph.33
int CvPlayer::adjustMissionCostToTeamSize(int iBaseCost, PlayerTypes eTargetPlayer) const {

	// Don't compute anything when the teams have equal size
	int iOurTeamSize = GET_TEAM(getTeam()).getNumMembers();
	int iTheirTeamSize = TEAMREF(eTargetPlayer).getNumMembers();
	if(iOurTeamSize == iTheirTeamSize)
		return iBaseCost;
	// Tie it to the tech cost modifier
	double extraTeamMemberModifier = GC.getDefineINT("TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER") / 100.f;
	/* <dlph.33> "New formula for espionage costs in team. Essentially, I want costs
		to scale with 1+0.5(number of members - 1), but since there are two teams
		(and two directions) involved, it will scale with the square root of the
		ratio of those values. Idea for formula by Fran." */
	return (int)(iBaseCost * std::sqrt(
			(1 + extraTeamMemberModifier * (iOurTeamSize - 1)) /
			(1 + extraTeamMemberModifier * (iTheirTeamSize - 1)))); // </dlph.33>
}


int CvPlayer::getEspionageMissionBaseCost(EspionageMissionTypes eMission, PlayerTypes eTargetPlayer, const CvPlot* pPlot, int iExtraData, const CvUnit* pSpyUnit) const
{
	if (eMission == NO_ESPIONAGEMISSION) // K-Mod
		return -1;

	CvEspionageMissionInfo& kMission = GC.getEspionageMissionInfo(eMission);
	int iBaseMissionCost = kMission.getCost();

	// -1 means this mission is disabled
	if (iBaseMissionCost == -1)
	{
		return -1;
	}

	CvCity* pCity = NULL;
	if (NULL != pPlot)
	{
		pCity = pPlot->getPlotCity();
	}

	if (kMission.isSelectPlot())
	{
		if (NULL == pPlot)
		{
			return -1;
		}

		if (!pPlot->isRevealed(getTeam(), false))
		{
			return -1;
		}
	}

	if (NULL == pCity && kMission.isTargetsCity())
	{
		return -1;
	}

	int iMissionCost = -1;

	if (kMission.getStealTreasuryTypes() > 0)
	{
		// Steal Treasury
		//int iNumTotalGold = (GET_PLAYER(eTargetPlayer).getGold() * kMission.getStealTreasuryTypes()) / 100;
		int iNumTotalGold
			= 0; // kmodx: Missing initialization

		if (NULL != pCity)
		{
			/* iNumTotalGold *= pCity->getPopulation();
			iNumTotalGold /= std::max(1, GET_PLAYER(eTargetPlayer).getTotalPopulation()); */
			// K-Mod
			iNumTotalGold = getEspionageGoldQuantity(eMission, eTargetPlayer, pCity);
		}

		if (iNumTotalGold > 0)
		{
			iMissionCost = (iBaseMissionCost * iNumTotalGold) / 100;
		}
	}
	else if (kMission.getBuyTechCostFactor() > 0)
	{
		// Buy (Steal) Tech
		TechTypes eTech = (TechTypes)iExtraData;
		// <advc.120d> Original code moved into auxiliary function
		iMissionCost = -1;
		if(eTech == NO_TECH)
			eTech = getStealCostTech(eTargetPlayer);
		int iProdCost = (eTech == NO_TECH ? -1 :
				GET_TEAM(getTeam()).getResearchCost(eTech));
		// </advc.120d>
		if (NO_TECH != eTech)
		{
			if (canStealTech(eTargetPlayer, eTech))
			{
				iMissionCost = iBaseMissionCost + ((100 + kMission.getBuyTechCostFactor()) * iProdCost) / 100;
			}
		}
	}
	else if (kMission.getSwitchCivicCostFactor() > 0)
	{
		// Switch Civics
		CivicTypes eCivic = (CivicTypes)iExtraData;

		/*if (NO_CIVIC == eCivic) {
			for (int iCivic = 0; iCivic < GC.getNumCivicInfos(); ++iCivic) {
				if (canForceCivics(eTargetPlayer, (CivicTypes)iCivic)) {
					eCivic = (CivicTypes)iCivic;
					break;
				}
			}
		}
		if (NO_CIVIC != eCivic)*/
		/*  <advc.132> Replacing the above: Need to compute a cost for every civic
			(like for the DestroyBuilding mission) */
		int iMinCost = MAX_INT;
		for(int i = 0; i < GC.getNumCivicInfos(); i++) {
			CivicTypes eLoopCivic = (CivicTypes)i;
			if(eCivic != NO_CIVIC && eLoopCivic != eCivic)
				continue; // </advc.132>
			if (canForceCivics(eTargetPlayer, eLoopCivic))
			{
				//iMissionCost = iBaseMissionCost + (kMission.getSwitchCivicCostFactor() * GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getAnarchyPercent()) / 10000;
				// K-Mod
				int iLoopCost = iBaseMissionCost + (kMission.getSwitchCivicCostFactor() * GET_PLAYER(eTargetPlayer).getTotalPopulation() * (GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getAnarchyPercent()
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
		for(int i = 0; i < GC.getNumReligionInfos(); i++) {
			ReligionTypes eLoopReligion = (ReligionTypes)i;
			if(eReligion != NO_RELIGION && eLoopReligion != eReligion)
				continue;
			if (canForceReligion(eTargetPlayer, eLoopReligion)
					&& (pCity == NULL || pCity->isHasReligion(eLoopReligion)))
			{// </advc.132>
				//iMissionCost = iBaseMissionCost + (kMission.getSwitchReligionCostFactor() * GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getAnarchyPercent()) / 10000;
				// K-Mod
				int iLoopCost = iBaseMissionCost + (kMission.getSwitchReligionCostFactor() * GET_PLAYER(eTargetPlayer).getTotalPopulation() * (GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getAnarchyPercent()
						// advc.132: As above for civics
						+ (getStateReligion() == eLoopReligion ? 0 : 100))
					) / 10000;
				ReligionTypes eCurrentReligion = GET_PLAYER(eTargetPlayer).getStateReligion();
				// amplify the mission cost if we are trying to switch to a minority religion.
				if (eCurrentReligion != NO_RELIGION)
				{
					// maybe getReligionPopulation would be slightly better, but it's a bit slower.
					int iCurrent = GET_PLAYER(eTargetPlayer).getHasReligionCount(eCurrentReligion);
					int iNew = GET_PLAYER(eTargetPlayer).getHasReligionCount(eLoopReligion);
					int iCitiesTarget = GC.getWorldInfo(GC.getMap().getWorldSize()).getTargetNumCities();
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
		CvUnit* pUnit = GET_PLAYER(eTargetPlayer).getUnit(iExtraData);
		int iCost = MAX_INT;

		if (NULL == pUnit)
		{
			if (NULL != pPlot)
			{
				CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();

				while (pUnitNode != NULL)
				{
					CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
					pUnitNode = pPlot->nextUnitNode(pUnitNode);

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
		}
		else
		{
			iCost = getProductionNeeded(pUnit->getUnitType());
		}

		if (NULL != pUnit)
		{
			if (canSpyDestroyUnit(eTargetPlayer, *pUnit))
			{
				iMissionCost = iBaseMissionCost + ((100 + kMission.getDestroyUnitCostFactor()) * iCost) / 100;
			}
		}
	}
	else if (kMission.getDestroyProjectCostFactor() > 0)
	{
		ProjectTypes eProject = (ProjectTypes) iExtraData;
		int iCost = MAX_INT;

		if (NO_PROJECT == eProject)
		{
			for (int iProject = 0; iProject < GC.getNumProjectInfos(); ++iProject)
			{
				if (canSpyDestroyProject(eTargetPlayer, (ProjectTypes)iProject))
				{
					int iValue = getProductionNeeded((ProjectTypes)iProject);

					if (iValue < iCost)
					{
						iCost = iValue;
						eProject = (ProjectTypes)iProject;
					}
				}
			}
		}
		else
		{
			iCost = getProductionNeeded(eProject);
		}

		if (NO_PROJECT != eProject)
		{
			if (canSpyDestroyProject(eTargetPlayer, eProject))
			{
				iMissionCost = iBaseMissionCost + ((100 + kMission.getDestroyProjectCostFactor()) * iCost) / 100;
			}
		}
	}
	else if (kMission.getDestroyProductionCostFactor() > 0)
	{
		FAssert(NULL != pCity);
		if (NULL != pCity)
		{
			iMissionCost = iBaseMissionCost + ((100 + kMission.getDestroyProductionCostFactor()) * pCity->getProduction()) / 100;
		}
	}
	else if (kMission.getBuyUnitCostFactor() > 0)
	{
		// Buy Unit
		CvUnit* pUnit = GET_PLAYER(eTargetPlayer).getUnit(iExtraData);
		int iCost = MAX_INT;

		if (NULL == pUnit)
		{
			if (NULL != pPlot)
			{
				CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();

				while (pUnitNode != NULL)
				{
					CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
					pUnitNode = pPlot->nextUnitNode(pUnitNode);

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
		}
		else
		{
			iCost = getProductionNeeded(pUnit->getUnitType());
		}


		if (NULL != pUnit)
		{
			if (canSpyBribeUnit(eTargetPlayer, *pUnit))
			{
				iMissionCost = iBaseMissionCost + ((100 + kMission.getBuyUnitCostFactor()) * iCost) / 100;
			}
		}
	}
	else if (kMission.getDestroyBuildingCostFactor() > 0)
	{
		BuildingTypes eBuilding = (BuildingTypes) iExtraData;
		int iCost = MAX_INT;

		if (NO_BUILDING == eBuilding)
		{
			for (int iBuilding = 0; iBuilding < GC.getNumBuildingInfos(); ++iBuilding)
			{
				if (NULL != pCity && pCity->getNumRealBuilding((BuildingTypes)iBuilding) > 0)
				{
					if (canSpyDestroyBuilding(eTargetPlayer, (BuildingTypes)iBuilding))
					{
						int iValue = getProductionNeeded((BuildingTypes)iBuilding);

						if (iValue < iCost)
						{
							iCost = iValue;
							eBuilding = (BuildingTypes)iBuilding;
						}
					}
				}
			}
		}
		else
		{
			iCost = getProductionNeeded(eBuilding);
		}

		if (NO_BUILDING != eBuilding)
		{
			if (NULL != pCity && pCity->getNumRealBuilding(eBuilding) > 0)
			{
				if (canSpyDestroyBuilding(eTargetPlayer, eBuilding))
				{
					iMissionCost = iBaseMissionCost + ((100 + kMission.getDestroyBuildingCostFactor()) * iCost) / 100;
				}
			}
		}
	}
	else if (kMission.getBuyCityCostFactor() > 0)
	{
		// Buy City
		if (NULL != pCity)
		{
			iMissionCost = iBaseMissionCost + (kMission.getBuyCityCostFactor() * GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getGrowthPercent()) / 10000;
		}
	}
	else if (kMission.getCityInsertCultureCostFactor() > 0)
	{
		// Insert Culture into City
		if (NULL != pPlot && pPlot->getCulture(getID()) > 0)
		{
			int iCultureAmount = kMission.getCityInsertCultureAmountFactor() *  pCity->countTotalCultureTimes100();
			iCultureAmount /= 10000;
			iCultureAmount = std::max(1, iCultureAmount);
			iMissionCost = iBaseMissionCost + (kMission.getCityInsertCultureCostFactor() * iCultureAmount) / 100;
		}
	}
	else if (kMission.isDestroyImprovement())
	{
		if (NULL != pPlot && !pPlot->isCity())
		{
			if (pPlot->getImprovementType() != NO_IMPROVEMENT || pPlot->getRouteType() != NO_ROUTE)
			{
				iMissionCost = (iBaseMissionCost * GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getBuildPercent()) / 100;
			}
		}
	}
	else if (kMission.getCityPoisonWaterCounter() > 0)
	{
		FAssert(NULL != pCity);
		// Cannot poison a city's water supply if it's already poisoned (value is negative when active)
		if (NULL != pCity && pCity->getEspionageHealthCounter() <= 0)
		{
			iMissionCost = iBaseMissionCost;
		}
	}

	// Make city unhappy
	else if (kMission.getCityUnhappinessCounter() > 0)
	{
		FAssert(NULL != pCity);
		// Cannot make a city unhappy if you've already done it (value is negative when active)
		if (NULL != pCity && pCity->getEspionageHappinessCounter() <= 0)
		{
			iMissionCost = iBaseMissionCost;
		}
	}

	// Make city Revolt
	else if (kMission.getCityRevoltCounter() > 0)
	{
		FAssert(NULL != pCity);
		// Cannot make a city revolt if it's already revolting
		if (NULL != pCity && pCity->getOccupationTimer() == 0)
		{
			iMissionCost = iBaseMissionCost;
		}
	}
	else if (kMission.getCounterespionageMod() > 0)
	{
		if (GET_TEAM(getTeam()).getCounterespionageTurnsLeftAgainstTeam(GET_PLAYER(eTargetPlayer).getTeam()) <= 0)
		{
			iMissionCost = (iBaseMissionCost * GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getResearchPercent()) / 100;
		}
	}
	else if (kMission.getPlayerAnarchyCounter() > 0)
	{
		// Player anarchy timer: can't add more turns of anarchy to player already in the midst of it
		if (!GET_PLAYER(eTargetPlayer).isAnarchy())
		{
			iMissionCost = (iBaseMissionCost * GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getAnarchyPercent()) / 100;
		}
	}
	else if (kMission.isPassive())
	{
		iMissionCost = (iBaseMissionCost * (100 + GET_TEAM(GET_PLAYER(eTargetPlayer).getTeam()).getEspionagePointsAgainstTeam(getTeam()))) / 100;
	}
	else
	{
		iMissionCost = (iBaseMissionCost * GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getResearchPercent()) / 100;
	}

	if (iMissionCost < 0)
	{
		return -1;
	}

	return iMissionCost;
}


int CvPlayer::getEspionageMissionCostModifier(EspionageMissionTypes eMission, PlayerTypes eTargetPlayer, const CvPlot* pPlot, int iExtraData, const CvUnit* pSpyUnit) const
{
	// K-Mod. I've altered this function to give a generic answer when NO_ESPIONAGEMISSION is passed.

	//CvEspionageMissionInfo& kMission = GC.getEspionageMissionInfo(eMission);
	int iModifier = 100;

	CvCity* pCity = NULL;
	if (NULL != pPlot)
	{
		pCity = pPlot->getPlotCity();
	}

	if (NO_PLAYER == eTargetPlayer)
	{
		eTargetPlayer = getID();
	}

	const CvTeam& kTargetTeam = GET_TEAM(GET_PLAYER(eTargetPlayer).getTeam()); // (moved from the bottom of the function)

	//if (pCity != NULL && kMission.isTargetsCity())
	if (pCity != NULL && (eMission == NO_ESPIONAGEMISSION || GC.getEspionageMissionInfo(eMission).isTargetsCity()))
	{
		// City Population
		iModifier *= 100 + (GC.getDefineINT("ESPIONAGE_CITY_POP_EACH_MOD") * (pCity->getPopulation() - 1));
		iModifier /= 100;

		// Trade Route
		if (pCity->isTradeRoute(getID()))
		{
			iModifier *= 100 + GC.getDefineINT("ESPIONAGE_CITY_TRADE_ROUTE_MOD");
			iModifier /= 100;
		}

		ReligionTypes eReligion = getStateReligion();
		if (NO_RELIGION != eReligion)
		{
			int iReligionModifier = 0;

			// City has Your State Religion
			if (pCity->isHasReligion(eReligion))
			{
				if (GET_PLAYER(eTargetPlayer).getStateReligion() != eReligion)
				{
					iReligionModifier += GC.getDefineINT("ESPIONAGE_CITY_RELIGION_STATE_MOD");
				}

				if (hasHolyCity(eReligion))
				{
					iReligionModifier += GC.getDefineINT("ESPIONAGE_CITY_HOLY_CITY_MOD");
				}
			}

			iModifier *= 100 + iReligionModifier;
			iModifier /= 100;

		}

		// City's culture affects cost
		/* original bts code
		iModifier *= 100 - (pCity->getCultureTimes100(getID()) * GC.getDefineINT("ESPIONAGE_CULTURE_MULTIPLIER_MOD")) / std::max(1, pCity->getCultureTimes100(eTargetPlayer) + pCity->getCultureTimes100(getID()));
		iModifier /= 100; */

		iModifier *= 100 + pCity->getEspionageDefenseModifier();
		iModifier /= 100;
	}

	if (pPlot != NULL)
	{
		// K-Mod. Culture Mod. (Based on plot culture rather than city culture.)
		if (eMission == NO_ESPIONAGEMISSION || GC.getEspionageMissionInfo(eMission).isSelectPlot() || GC.getEspionageMissionInfo(eMission).isTargetsCity())
		{
			iModifier *= 100 - (pPlot->getCulture(getID()) * GC.getDefineINT("ESPIONAGE_CULTURE_MULTIPLIER_MOD")) / std::max(1, pPlot->getCulture(eTargetPlayer) + pPlot->getCulture(getID()));
			iModifier /= 100;
		}
		// K-Mod end

		// Distance mod
		int iDistance = GC.getMap().maxPlotDistance();

		CvCity* pOurCapital = getCapitalCity();
		if (NULL != pOurCapital)
		{
			//if (kMission.isSelectPlot() || kMission.isTargetsCity())
			if (eMission == NO_ESPIONAGEMISSION || GC.getEspionageMissionInfo(eMission).isSelectPlot() || GC.getEspionageMissionInfo(eMission).isTargetsCity())
			{
				iDistance = plotDistance(pOurCapital->getX(), pOurCapital->getY(), pPlot->getX(), pPlot->getY());
			}
			else
			{
				CvCity* pTheirCapital = GET_PLAYER(eTargetPlayer).getCapitalCity();
				if (NULL != pTheirCapital)
				{
					iDistance = plotDistance(pOurCapital->getX(), pOurCapital->getY(), pTheirCapital->getX(), pTheirCapital->getY());
				}
			}
		}

		iModifier *= (iDistance + GC.getMap().maxPlotDistance()) * GC.getDefineINT("ESPIONAGE_DISTANCE_MULTIPLIER_MOD") / GC.getMap().maxPlotDistance();
		iModifier /= 100;
	}

	// Spy presence mission cost alteration
	if (NULL != pSpyUnit)
	{
		iModifier *= 100 - (pSpyUnit->getFortifyTurns() * GC.getDefineINT("ESPIONAGE_EACH_TURN_UNIT_COST_DECREASE"));
		iModifier /= 100;
	}

	// My points VS. Your points to mod cost
	/* original bts code
	int iTargetPoints = kTargetTeam.getEspionagePointsEver();
	int iOurPoints = GET_TEAM(getTeam()).getEspionagePointsEver();
		iModifier *= (GC.getDefineINT("ESPIONAGE_SPENDING_MULTIPLIER") * (2 * iTargetPoints + iOurPoints)) / std::max(1, iTargetPoints + 2 * iOurPoints);
		iModifier /= 100;
	} */
	// K-Mod. use the dedicated function that exists for this modifier, for consistency.
	iModifier *= ::getEspionageModifier(getTeam(), kTargetTeam.getID());
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


bool CvPlayer::doEspionageMission(EspionageMissionTypes eMission, PlayerTypes eTargetPlayer, CvPlot* pPlot, int iExtraData, CvUnit* pSpyUnit)
{
	if (!canDoEspionageMission(eMission, eTargetPlayer, pPlot, iExtraData, pSpyUnit))
	{
		return false;
	}

	TeamTypes eTargetTeam = NO_TEAM;
	if (NO_PLAYER != eTargetPlayer)
	{
		eTargetTeam = GET_PLAYER(eTargetPlayer).getTeam();
	}

	CvEspionageMissionInfo& kMission = GC.getEspionageMissionInfo(eMission);

	bool bSomethingHappened = false;
	bool bAggressiveMission = true; // advc.120
	bool bShowExplosion = false;
	CvWString szBuffer;

	int iMissionCost = getEspionageMissionCost(eMission, eTargetPlayer, pPlot, iExtraData, pSpyUnit);


	//////////////////////////////
	// Destroy Improvement

	if (kMission.isDestroyImprovement())
	{
		if (NULL != pPlot)
		{
			// Blow it up
			if (pPlot->getImprovementType() != NO_IMPROVEMENT)
			{
				szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_SOMETHING_DESTROYED", GC.getImprovementInfo(pPlot->getImprovementType()).getDescription()).GetCString();
				pPlot->setImprovementType((ImprovementTypes)(GC.getImprovementInfo(pPlot->getImprovementType()).getImprovementPillage()));
				bSomethingHappened = true;
			}
			else if (pPlot->getRouteType() != NO_ROUTE)
			{
				szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_SOMETHING_DESTROYED", GC.getRouteInfo(pPlot->getRouteType()).getDescription()).GetCString();
				pPlot->setRouteType(NO_ROUTE, true);
				bSomethingHappened = true;
			}

			if (bSomethingHappened)
			{
				bShowExplosion = true;
			}
		}
	}

	//////////////////////////////
	// Destroy Building

	if (kMission.getDestroyBuildingCostFactor() > 0)
	{
		BuildingTypes eTargetBuilding = (BuildingTypes)iExtraData;

		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_SOMETHING_DESTROYED_IN", GC.getBuildingInfo(eTargetBuilding).getDescription(), pCity->getNameKey()).GetCString();
				pCity->setNumRealBuilding(eTargetBuilding, pCity->getNumRealBuilding(eTargetBuilding) - 1);

				bSomethingHappened = true;
				bShowExplosion = true;
				// K-Mod
				if (!isHuman() || pCity->isProductionAutomated())
					pCity->AI_setChooseProductionDirty(true);
				// K-Mod end
			}
		}
	}

	//////////////////////////////
	// Destroy Project

	if (kMission.getDestroyProjectCostFactor() > 0)
	{
		ProjectTypes eTargetProject = (ProjectTypes)iExtraData;

		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_SOMETHING_DESTROYED_IN", GC.getProjectInfo(eTargetProject).getDescription(), pCity->getNameKey()).GetCString();
				GET_TEAM(eTargetTeam).changeProjectCount(eTargetProject, -1);

				bSomethingHappened = true;
				bShowExplosion = true;
			}
		}
	}

	//////////////////////////////
	// Destroy Production

	if (kMission.getDestroyProductionCostFactor() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_PRODUCTION_DESTROYED_IN", pCity->getProductionName(), pCity->getNameKey());
				pCity->setProduction(0);

				bSomethingHappened = true;
				bShowExplosion = true;
				// K-Mod
				if (!isHuman()) // not for automated cities
					pCity->AI_setChooseProductionDirty(true);
				// K-Mod end
			}
		}
	}

	//////////////////////////////
	// Destroy Unit

	if (kMission.getDestroyUnitCostFactor() > 0)
	{
		if (NO_PLAYER != eTargetPlayer)
		{
			int iTargetUnitID = iExtraData;

			CvUnit* pUnit = GET_PLAYER(eTargetPlayer).getUnit(iTargetUnitID);

			if (NULL != pUnit)
			{
				FAssert(pUnit->plot() == pPlot);
				szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_SOMETHING_DESTROYED", pUnit->getNameKey()).GetCString();
				pUnit->kill(false, getID());

				bSomethingHappened = true;
				bShowExplosion = true;
			}
		}
	}

	//////////////////////////////
	// Buy Unit

	if (kMission.getBuyUnitCostFactor() > 0)
	{
		if (NO_PLAYER != eTargetPlayer)
		{
			int iTargetUnitID = iExtraData;

			CvUnit* pUnit = GET_PLAYER(eTargetPlayer).getUnit(iTargetUnitID);

			if (NULL != pUnit)
			{
				FAssert(pUnit->plot() == pPlot);

				szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_UNIT_BOUGHT", pUnit->getNameKey()).GetCString();

				UnitTypes eUnitType = pUnit->getUnitType();
				int iX = pUnit->getX();
				int iY = pUnit->getY();
				pUnit->kill(false, getID());
				initUnit(eUnitType, iX, iY, NO_UNITAI);

				bSomethingHappened = true;
			}
		}
	}

	//////////////////////////////
	// Buy City

	if (kMission.getBuyCityCostFactor() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_CITY_BOUGHT", pCity->getNameKey()).GetCString();
				acquireCity(pCity, false, true, true);

				bSomethingHappened = true;
			}
		}
	}

	//////////////////////////////
	// Insert Culture into City

	if (kMission.getCityInsertCultureCostFactor() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_CITY_CULTURE_INSERTED", pCity->getNameKey()).GetCString();

				/* original bts code
				int iCultureAmount = kMission.getCityInsertCultureAmountFactor() * pCity->countTotalCultureTimes100();
				iCultureAmount /= 10000;
				iCultureAmount = std::max(1, iCultureAmount);
				int iNumTurnsApplied = (GC.getDefineINT("GREAT_WORKS_CULTURE_TURNS") * GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getUnitGreatWorkPercent()) / 100;
				for (int i = 0; i < iNumTurnsApplied; ++i)
					pCity->changeCulture(getID(), iCultureAmount / iNumTurnsApplied, true, true);
				if (iNumTurnsApplied > 0)
					pCity->changeCulture(getID(), iCultureAmount % iNumTurnsApplied, false, true);
				} */
				// K-Mod. apply culture in one hit. We don't need fake 'free city culture' anymore.
				int iCultureTimes100 = std::max(1, kMission.getCityInsertCultureAmountFactor() * pCity->countTotalCultureTimes100() / 100);

				//pCity->changeCultureTimes100(getID(), iCultureTimes100, true, true);
				pCity->doPlotCultureTimes100(true, getID(), iCultureTimes100, false); // plot culture only.
				// K-Mod end

				bSomethingHappened = true;
			}

		}
	}

	//////////////////////////////
	// Poison City's Water Supply

	if (kMission.getCityPoisonWaterCounter() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_CITY_POISONED", pCity->getNameKey()).GetCString();
				pCity->changeEspionageHealthCounter(kMission.getCityPoisonWaterCounter());

				bShowExplosion = true;
				bSomethingHappened = true;
			}
		}
	}

	//////////////////////////////
	// Make city Unhappy

	if (kMission.getCityUnhappinessCounter() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_CITY_UNHAPPY", pCity->getNameKey()).GetCString();
				pCity->changeEspionageHappinessCounter(kMission.getCityUnhappinessCounter());

				bShowExplosion = true;
				bSomethingHappened = true;
			}
		}
	}

	//////////////////////////////
	// Make city Revolt

	if (kMission.getCityRevoltCounter() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_CITY_REVOLT", pCity->getNameKey()).GetCString();
				pCity->changeCultureUpdateTimer(kMission.getCityRevoltCounter());
				pCity->changeOccupationTimer(kMission.getCityRevoltCounter());

				bSomethingHappened = true;
				bShowExplosion = true;

				if (gUnitLogLevel >= 2 && !isHuman())
				{
					logBBAI("      Spy for player %d (%S) causes revolt in %S, owned by %S (%d)", getID(), getCivilizationDescription(0), pCity->getName().GetCString(), GET_PLAYER(pCity->getOwner()).getCivilizationDescription(0), pCity->getOwner());
				}
			}
		}
	}

	//////////////////////////////
	// Steal Treasury

	if (kMission.getStealTreasuryTypes() > 0)
	{
		if (NO_PLAYER != eTargetPlayer)
		{
			//int iNumTotalGold = (GET_PLAYER(eTargetPlayer).getGold() * kMission.getStealTreasuryTypes()) / 100;
			int iNumTotalGold
					= 0; // kmodx

			if (NULL != pPlot)
			{
				CvCity* pCity = pPlot->getPlotCity();

				if (NULL != pCity)
				{
					/* iNumTotalGold *= pCity->getPopulation();
					iNumTotalGold /= std::max(1, GET_PLAYER(eTargetPlayer).getTotalPopulation()); */
					// K-Mod. Make stealing gold still worthwhile against large civs.
					iNumTotalGold = getEspionageGoldQuantity(eMission, eTargetPlayer, pCity);
				}
			}

			szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_STEAL_TREASURY",
					/* advc.004i: */ iNumTotalGold).GetCString();
			changeGold(iNumTotalGold);
			if (NO_PLAYER != eTargetPlayer)
			{
				GET_PLAYER(eTargetPlayer).changeGold(-iNumTotalGold);
			}

			bSomethingHappened = true;
		}
	}

	//////////////////////////////
	// Buy (Steal) Tech

	if (kMission.getBuyTechCostFactor() > 0)
	{
		TechTypes eTech = (TechTypes)iExtraData;
		szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_TECH_BOUGHT",
				GC.getTechInfo(eTech).getDescription()).GetCString();
		GET_TEAM(getTeam()).setHasTech(eTech, true, getID(), false, true);
		if(isSignificantDiscovery(eTech)) // advc.550e
			GET_TEAM(getTeam()).setNoTradeTech(eTech, true);
		bSomethingHappened = true;
	}

	//////////////////////////////
	// Switch Civic

	if (kMission.getSwitchCivicCostFactor() > 0)
	{
		if (NO_PLAYER != eTargetPlayer)
		{
			announceEspionageToThirdParties(eMission, eTargetPlayer); // advc.120f
			int iCivic = iExtraData;
			szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_SWITCH_CIVIC", GC.getCivicInfo((CivicTypes) iCivic).getDescription()).GetCString();
			GET_PLAYER(eTargetPlayer).setCivics((CivicOptionTypes) GC.getCivicInfo((CivicTypes) iCivic).getCivicOptionType(), (CivicTypes) iCivic);
			GET_PLAYER(eTargetPlayer).setRevolutionTimer(std::max(1, ((100 + GET_PLAYER(eTargetPlayer).getAnarchyModifier()) * GC.getDefineINT("MIN_REVOLUTION_TURNS")) / 100));
			bSomethingHappened = true;
		}
	}

	//////////////////////////////
	// Switch Religion

	if (kMission.getSwitchReligionCostFactor() > 0)
	{
		if (NO_PLAYER != eTargetPlayer)
		{
			announceEspionageToThirdParties(eMission, eTargetPlayer); // advc.120f
			int iReligion = iExtraData;
			szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_SWITCH_RELIGION", GC.getReligionInfo((ReligionTypes) iReligion).getDescription()).GetCString();
			GET_PLAYER(eTargetPlayer).setLastStateReligion((ReligionTypes) iReligion);
			GET_PLAYER(eTargetPlayer).setConversionTimer(std::max(1, ((100 + GET_PLAYER(eTargetPlayer).getAnarchyModifier()) * GC.getDefineINT("MIN_CONVERSION_TURNS")) / 100));
			bSomethingHappened = true;
		}
	}

	//////////////////////////////
	// Player Anarchy

	if (kMission.getPlayerAnarchyCounter() > 0)
	{
		if (NO_PLAYER != eTargetPlayer)
		{
			int iTurns = (kMission.getPlayerAnarchyCounter() * GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getAnarchyPercent()) / 100;
			szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_PLAYER_ANARCHY").GetCString();
			GET_PLAYER(eTargetPlayer).changeAnarchyTurns(iTurns);

			bSomethingHappened = true;
		}
	}

	//////////////////////////////
	// Counterespionage

	if (kMission.getCounterespionageNumTurns() > 0 && kMission.getCounterespionageMod() > 0)
	{
		if (NO_TEAM != eTargetTeam)
		{
			szBuffer = gDLL->getText("TXT_KEY_ESPIONAGE_TARGET_COUNTERESPIONAGE").GetCString();
			int iTurns = (kMission.getCounterespionageNumTurns() * GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getResearchPercent()) / 100;
			GET_TEAM(getTeam()).changeCounterespionageTurnsLeftAgainstTeam(eTargetTeam, iTurns);
			GET_TEAM(getTeam()).changeCounterespionageModAgainstTeam(eTargetTeam, kMission.getCounterespionageMod());
			// <advc.120>
			if(!bSomethingHappened)
				bAggressiveMission = false; // </advc.120>
			bSomethingHappened = true;

		}
	}

	// <advc.103>
	if(kMission.isInvestigateCity() && pPlot != NULL) {
		CvCity* pCity = pPlot->getPlotCity();
		if(pCity != NULL) {
			pCity->setInvestigate(true);
			gDLL->getInterfaceIFace()->selectCity(pCity);
			bSomethingHappened = true;
		}
	} // </advc.103>

	int iHave = 0;
	if (NO_TEAM != eTargetTeam)
	{
		iHave = GET_TEAM(getTeam()).getEspionagePointsAgainstTeam(eTargetTeam);

		if (bSomethingHappened)
		{
			GET_TEAM(getTeam()).changeEspionagePointsAgainstTeam(eTargetTeam, -iMissionCost);
		}
	}

	if (bShowExplosion)
	{
		if (pPlot)
		{
			if (pPlot->isVisible(GC.getGame().getActiveTeam(), false)
					&& GC.getGame().getActiveTeam() == getTeam()) // advc.120i
			{
				EffectTypes eEffect = GC.getEntityEventInfo(GC.getMissionInfo(MISSION_BOMBARD).getEntityEvent()).getEffectType();
				gDLL->getEngineIFace()->TriggerEffect(eEffect, pPlot->getPoint(), (float)(GC.getASyncRand().get(360)));
				gDLL->getInterfaceIFace()->playGeneralSound("AS3D_UN_CITY_EXPLOSION", pPlot->getPoint());
			}
		}
	}

	if (bSomethingHappened)
	{
		int iX = -1;
		int iY = -1;
		if (NULL != pPlot)
		{
			iX = pPlot->getX();
			iY = pPlot->getY();
		}
		// advc.103: The city screen having opened is confirmation enough
		if(!kMission.isInvestigateCity()) {
			gDLL->getInterfaceIFace()->addHumanMessage(getID(), true,
					GC.getEVENT_MESSAGE_TIME(),
					gDLL->getText("TXT_KEY_ESPIONAGE_MISSION_PERFORMED"),
					"AS2D_POSITIVE_DINK", MESSAGE_TYPE_INFO,
					ARTFILEMGR.getInterfaceArtInfo("ESPIONAGE_BUTTON")->getPath(),
					(ColorTypes)GC.getInfoTypeForString("COLOR_GREEN"), iX, iY,
					true, true);
		}
	}
	else if (getID() == GC.getGame().getActivePlayer())
	{
		CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_TEXT);
		if (iHave < iMissionCost)
		{
			pInfo->setText(gDLL->getText("TXT_KEY_ESPIONAGE_TOO_EXPENSIVE", iMissionCost, iHave));
		}
		else
		{
			pInfo->setText(gDLL->getText("TXT_KEY_ESPIONAGE_CANNOT_DO_MISSION"));
		}

		addPopup(pInfo);
	}

	if (bSomethingHappened && !szBuffer.empty())
	{
		int iX = -1;
		int iY = -1;
		if (NULL != pPlot)
		{
			iX = pPlot->getX();
			iY = pPlot->getY();
		}

		if (NO_PLAYER != eTargetPlayer)
		{	// <advc.120>
			ColorTypes eColor = NO_COLOR;
			if(bAggressiveMission)
				eColor = (ColorTypes)GC.getInfoTypeForString("COLOR_RED");
			// </advc.120>
			gDLL->getInterfaceIFace()->addHumanMessage(eTargetPlayer,
					/*true*/ false, // advc.120i
					GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_DEAL_CANCELLED",
					MESSAGE_TYPE_INFO, ARTFILEMGR.getInterfaceArtInfo(
					"ESPIONAGE_BUTTON")->getPath(), eColor, iX, iY, true, true);
		}
	}

	return bSomethingHappened;
}

int CvPlayer::getEspionageSpendingWeightAgainstTeam(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiEspionageSpendingWeightAgainstTeam[eIndex];
}

void CvPlayer::setEspionageSpendingWeightAgainstTeam(TeamTypes eIndex, int iValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	//FAssert(iValue >= 0);

	iValue = std::min(std::max(0, iValue), 99);

	if (iValue != getEspionageSpendingWeightAgainstTeam(eIndex))
	{
		m_aiEspionageSpendingWeightAgainstTeam[eIndex] = iValue;

		gDLL->getInterfaceIFace()->setDirty(Espionage_Advisor_DIRTY_BIT, true);
	}
}

void CvPlayer::changeEspionageSpendingWeightAgainstTeam(TeamTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");

	setEspionageSpendingWeightAgainstTeam(eIndex, getEspionageSpendingWeightAgainstTeam(eIndex) + iChange);
}

void CvPlayer::doAdvancedStartAction(AdvancedStartActionTypes eAction, int iX, int iY, int iData, bool bAdd,
		int iData2) // advc.250c
{
	if (getAdvancedStartPoints() < 0)
	{
		return;
	}

	CvPlot* pPlot = GC.getMap().plot(iX, iY);

	if (getNumCities() == 0)
	{
		switch (eAction)
		{
		case ADVANCEDSTARTACTION_EXIT:
			//Try to build this player's empire
			if (getID() == GC.getGame().getActivePlayer())
			{
				gDLL->getInterfaceIFace()->setBusy(true);
			}
			AI_doAdvancedStart(true);
			if (getID() == GC.getGame().getActivePlayer())
			{
				gDLL->getInterfaceIFace()->setBusy(false);
			}
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

	switch (eAction)
	{
	case ADVANCEDSTARTACTION_EXIT:
		changeGold(getAdvancedStartPoints());
		setAdvancedStartPoints(-1);
		if (GC.getGame().getActivePlayer() == getID())
		{
			gDLL->getInterfaceIFace()->setInAdvancedStart(false);
		}

		if (isHuman())
		{
			int iLoop;
			for (CvCity* pCity = firstCity(&iLoop); NULL != pCity; pCity = nextCity(&iLoop))
			{
				pCity->chooseProduction();
			}

			chooseTech();

			if (canRevolution(NULL))
			{
				CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_CHANGECIVIC);
				if (NULL != pInfo)
				{
					gDLL->getInterfaceIFace()->addPopup(pInfo, getID());
				}
			}
		}
		break;
	case ADVANCEDSTARTACTION_AUTOMATE:
		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setBusy(true);
		}
		AI_doAdvancedStart(true);
		if (getID() == GC.getGame().getActivePlayer())
		{
			gDLL->getInterfaceIFace()->setBusy(false);
		}
		break;
	case ADVANCEDSTARTACTION_UNIT:
		{
			if(pPlot == NULL)
				return;

			UnitTypes eUnit = (UnitTypes) iData;
			int iCost = getAdvancedStartUnitCost(eUnit, bAdd, pPlot);

			if (bAdd && iCost < 0)
			{
				return;
			}

			// Add unit to the map
			if (bAdd)
			{
				if (getAdvancedStartPoints() >= iCost)
				{
					CvUnit* pUnit = initUnit(eUnit, iX, iY,
							(UnitAITypes)iData2); // advc.250c
					if (NULL != pUnit)
					{
						pUnit->finishMoves();
						changeAdvancedStartPoints(-iCost);
					}
				}
			}

			// Remove unit from the map
			else
			{
				// If cost is -1 we already know this unit isn't present
				if (iCost != -1)
				{
					CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();
					while (pUnitNode != NULL)
					{
						CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = pPlot->nextUnitNode(pUnitNode);

						if (pLoopUnit->getUnitType() == eUnit)
						{
							pLoopUnit->kill(false);
							changeAdvancedStartPoints(iCost);
							return;
						}
					}
				}

				// Proper unit not found above, delete first found
				CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();
				if (pUnitNode != NULL)
				{
					CvUnit* pUnit = ::getUnit(pUnitNode->m_data);

					iCost = getAdvancedStartUnitCost(pUnit->getUnitType(), false,
							pPlot); // dlph.11
					FAssertMsg(iCost != -1, "If this is -1 then that means it's going to try to delete a unit which shouldn't exist");
					pUnit->kill(false);
					changeAdvancedStartPoints(iCost);
				}
			}

			if (getID() == GC.getGame().getActivePlayer())
			{
				gDLL->getInterfaceIFace()->setDirty(Advanced_Start_DIRTY_BIT, true);
			}
		}
		break;
	case ADVANCEDSTARTACTION_CITY:
		{
			if(pPlot == NULL)
				return;

			int iCost = getAdvancedStartCityCost(bAdd, pPlot);

			if (iCost < 0)
			{
				return;
			}

			// Add City to the map
			if (bAdd)
			{
				if (getNumCities() == 0)
				{
					PlayerTypes eClosestPlayer = NO_PLAYER;
					int iMinDistance = MAX_INT;
					for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
					{
						CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
						if (kPlayer.isAlive())
						{
							if (kPlayer.getTeam() == getTeam())
							{
								if (kPlayer.getNumCities() == 0)
								{
									FAssert(kPlayer.getStartingPlot() != NULL);
									int iDistance = plotDistance(iX, iY, kPlayer.getStartingPlot()->getX(), kPlayer.getStartingPlot()->getY());
									if (iDistance < iMinDistance)
									{
										eClosestPlayer = kPlayer.getID();
										iMinDistance = iDistance;
									}
								}
							}
						}
					}
					FAssertMsg(eClosestPlayer != NO_PLAYER, "Self at a minimum should always be valid");
					if (eClosestPlayer != getID())
					{
						CvPlot* pTempPlot = GET_PLAYER(eClosestPlayer).getStartingPlot();
						GET_PLAYER(eClosestPlayer).setStartingPlot(getStartingPlot(), false);
						setStartingPlot(pTempPlot, false);
					}
				}
				if (getAdvancedStartPoints() >= iCost || getNumCities() == 0)
				{
					found(iX, iY);
					changeAdvancedStartPoints(-std::min(iCost, getAdvancedStartPoints()));
					GC.getGame().updateColoredPlots();
					// advc.250c: Commented out
					/*CvCity* pCity = pPlot->getPlotCity();
					if (pCity != NULL)
					{
						if (pCity->getPopulation() > 1)
						{
							pCity->setFood(pCity->growthThreshold() / 2);
						}
					}*/
				}
			}

			// Remove City from the map
			else
			{
				pPlot->setRouteType(NO_ROUTE, true);
				pPlot->getPlotCity()->kill(true);
				pPlot->setImprovementType(NO_IMPROVEMENT);
				changeAdvancedStartPoints(iCost);
			}

			if (getID() == GC.getGame().getActivePlayer())
			{
				gDLL->getInterfaceIFace()->setDirty(Advanced_Start_DIRTY_BIT, true);
			}
		}
		break;
	case ADVANCEDSTARTACTION_POP:
		{
			if(pPlot == NULL)
				return;

			CvCity* pCity = pPlot->getPlotCity();

			if (pCity != NULL)
			{
				int iCost = getAdvancedStartPopCost(bAdd, pCity);

				if (iCost < 0)
				{
					return;
				}

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
					// advc.250c, dlph: Commented out
					/*if (pCity->getPopulation() == 1) {
						pCity->setFood(0);
						pCity->setFoodKept(0);
					}
					else if (pCity->getPopulation() > 1) {
						pCity->setFood(pCity->growthThreshold() / 2);
						pCity->setFoodKept((pCity->getFood() * pCity->getMaxFoodKeptPercent()) / 100);
					}*/
				}
			}
		}
		break;
	case ADVANCEDSTARTACTION_CULTURE:
		{
			if(pPlot == NULL)
				return;

			CvCity* pCity = pPlot->getPlotCity();

			if (pCity != NULL)
			{
				int iCost = getAdvancedStartCultureCost(bAdd, pCity);

				if (iCost < 0)
				{
					return;
				}

				// Add Culture to the City
				if (bAdd)
				{
					if (getAdvancedStartPoints() >= iCost)
					{
						pCity->setCulture(getID(), pCity->getCultureThreshold(), true, true);
						changeAdvancedStartPoints(-iCost);
					}
				}

				// Remove Culture from the city
				else
				{
					CultureLevelTypes eLevel = (CultureLevelTypes)std::max(0, pCity->getCultureLevel() - 1);
					pCity->setCulture(getID(), CvCity::getCultureThreshold(eLevel), true, true);
					changeAdvancedStartPoints(iCost);
				}
			}
		}
		break;
	case ADVANCEDSTARTACTION_BUILDING:
		{
			if(pPlot == NULL)
				return;

			CvCity* pCity = pPlot->getPlotCity();

			if (pCity != NULL)
			{
				BuildingTypes eBuilding = (BuildingTypes) iData;
				int iCost = getAdvancedStartBuildingCost(eBuilding, bAdd, pCity);

				if (iCost < 0)
				{
					return;
				}

				// Add Building to the City
				if (bAdd)
				{
					if (getAdvancedStartPoints() >= iCost)
					{
						pCity->setNumRealBuilding(eBuilding, pCity->getNumRealBuilding(eBuilding)+1);
						changeAdvancedStartPoints(-iCost);
						if (GC.getBuildingInfo(eBuilding).getFoodKept() != 0)
						{
							pCity->setFoodKept((pCity->getFood() * pCity->getMaxFoodKeptPercent()) / 100);
						}
					}
				}

				// Remove Building from the map
				else
				{
					pCity->setNumRealBuilding(eBuilding, pCity->getNumRealBuilding(eBuilding)-1);
					changeAdvancedStartPoints(iCost);
					if (GC.getBuildingInfo(eBuilding).getFoodKept() != 0)
					{
						pCity->setFoodKept((pCity->getFood() * pCity->getMaxFoodKeptPercent()) / 100);
					}
				}
			}

			if (getID() == GC.getGame().getActivePlayer())
			{
				gDLL->getInterfaceIFace()->setDirty(Advanced_Start_DIRTY_BIT, true);
			}
		}
		break;
	case ADVANCEDSTARTACTION_ROUTE:
		{
			if(pPlot == NULL)
				return;

			RouteTypes eRoute = (RouteTypes) iData;
			int iCost = getAdvancedStartRouteCost(eRoute, bAdd, pPlot);

			if (bAdd && iCost < 0)
			{
				return;
			}

			// Add Route to the plot
			if (bAdd)
			{
				if (getAdvancedStartPoints() >= iCost)
				{
					pPlot->setRouteType(eRoute, true);
					changeAdvancedStartPoints(-iCost);
				}
			}

			// Remove Route from the Plot
			else
			{
				if (pPlot->getRouteType() != eRoute)
				{
					eRoute = pPlot->getRouteType();
					iCost = getAdvancedStartRouteCost(eRoute, bAdd);
				}

				if (iCost < 0)
				{
					return;
				}

				pPlot->setRouteType(NO_ROUTE, true);
				changeAdvancedStartPoints(iCost);
			}

			if (getID() == GC.getGame().getActivePlayer())
			{
				gDLL->getInterfaceIFace()->setDirty(Advanced_Start_DIRTY_BIT, true);
			}
		}
		break;
	case ADVANCEDSTARTACTION_IMPROVEMENT:
		{
			if(pPlot == NULL)
				return;

			ImprovementTypes eImprovement = (ImprovementTypes) iData;
			int iCost = getAdvancedStartImprovementCost(eImprovement, bAdd, pPlot);

			if (bAdd && iCost < 0)
			{
				return;
			}

			// Add Improvement to the plot
			if (bAdd)
			{
				if (getAdvancedStartPoints() >= iCost)
				{
					if (pPlot->getFeatureType() != NO_FEATURE)
					{
						for (int iI = 0; iI < GC.getNumBuildInfos(); ++iI)
						{
							ImprovementTypes eLoopImprovement = ((ImprovementTypes)(GC.getBuildInfo((BuildTypes)iI).getImprovement()));

							if (eImprovement == eLoopImprovement)
							{
								if (GC.getBuildInfo((BuildTypes)iI).isFeatureRemove(pPlot->getFeatureType()) && canBuild(pPlot, (BuildTypes)iI))
								{
									pPlot->setFeatureType(NO_FEATURE);
									break;
								}
							}
						}
					}

					pPlot->setImprovementType(eImprovement);

					changeAdvancedStartPoints(-iCost);
				}
			}

			// Remove Improvement from the Plot
			else
			{
				if (pPlot->getImprovementType() != eImprovement)
				{
					eImprovement = pPlot->getImprovementType();
					iCost = getAdvancedStartImprovementCost(eImprovement, bAdd, pPlot);
				}

				if (iCost < 0)
				{
					return;
				}

				pPlot->setImprovementType(NO_IMPROVEMENT);
				changeAdvancedStartPoints(iCost);
			}

			if (getID() == GC.getGame().getActivePlayer())
			{
				gDLL->getInterfaceIFace()->setDirty(Advanced_Start_DIRTY_BIT, true);
			}
		}
		break;
	case ADVANCEDSTARTACTION_TECH:
		{
			TechTypes eTech = (TechTypes) iData;
			int iCost = getAdvancedStartTechCost(eTech, bAdd);

			if (iCost < 0)
			{
				return;
			}

			// Add Tech to team
			if (bAdd)
			{
				if (getAdvancedStartPoints() >= iCost)
				{
					GET_TEAM(getTeam()).setHasTech(eTech, true, getID(), false, false);
					changeAdvancedStartPoints(-iCost);
				}
			}

			// Remove Tech from the Team
			else
			{
				GET_TEAM(getTeam()).setHasTech(eTech, false, getID(), false, false);
				changeAdvancedStartPoints(iCost);
			}

			if (getID() == GC.getGame().getActivePlayer())
			{
				gDLL->getInterfaceIFace()->setDirty(Advanced_Start_DIRTY_BIT, true);
			}
		}
		break;
	case ADVANCEDSTARTACTION_VISIBILITY:
		{
			if(pPlot == NULL)
				return;

			int iCost = getAdvancedStartVisibilityCost(bAdd, pPlot);

			if (iCost < 0)
			{
				return;
			}

			// Add Visibility to the plot
			if (bAdd)
			{
				if (getAdvancedStartPoints() >= iCost)
				{
					pPlot->setRevealed(getTeam(), true, true, NO_TEAM, true);
					changeAdvancedStartPoints(-iCost);
				}
			}

			// Remove Visibility from the Plot
			else
			{
				pPlot->setRevealed(getTeam(), false, true, NO_TEAM, true);
				changeAdvancedStartPoints(iCost);
			}
		}
		break;
	default:
		FAssert(false);
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adding or removing a unit
/////////////////////////////////////////////////////////////////////////////////////////////
int CvPlayer::getAdvancedStartUnitCost(UnitTypes eUnit, bool bAdd, CvPlot* pPlot) const
{
	int iLoop;
	int iNumUnitType = 0;

	if (getNumCities() <= 0)
		return -1;

	int iCost = (getProductionNeeded(eUnit) *
			//GC.getUnitInfo(eUnit).getAdvancedStartCost() / 100;
			// advc.250c:
			3 * GC.getUnitInfo(eUnit).getAdvancedStartCost()) / 200;

	if (iCost < 0)
		return -1;

	if (NULL == pPlot)
	{
		if (bAdd)
		{
			bool bValid = false;
			for (CvCity* pLoopCity = firstCity(&iLoop); NULL != pLoopCity; pLoopCity = nextCity(&iLoop))
			{
				if (pLoopCity->canTrain(eUnit))
				{
					bValid = true;
					break;
				}
			}

			if (!bValid)
			{
				return -1;
			}
		}
	}
	else
	{
		CvCity* pCity = NULL;

		if (GC.getDefineINT("ADVANCED_START_ALLOW_UNITS_OUTSIDE_CITIES") == 0)
		{
			pCity = pPlot->getPlotCity();

			if (NULL == pCity || pCity->getOwner() != getID())
			{
				return -1;
			}

			iCost *= 100;
			iCost /= std::max(1, 100 + pCity->getProductionModifier(eUnit));
		}
		else
		{
			if (pPlot->getOwner() != getID())
			{
				return -1;
			}

			iCost *= 100;
			iCost /= std::max(1, 100 + getProductionModifier(eUnit));
		}


		if (bAdd)
		{
			int iMaxUnitsPerCity = GC.getDefineINT("ADVANCED_START_MAX_UNITS_PER_CITY");
			if (iMaxUnitsPerCity >= 0)
			{
				if (GC.getUnitInfo(eUnit).isMilitarySupport() && getNumMilitaryUnits() >= iMaxUnitsPerCity * getNumCities())
				{
					return -1;
				}
			}

			if (NULL != pCity)
			{
				if (!pCity->canTrain(eUnit))
				{
					return -1;
				}
			}
			else
			{
				if (!pPlot->canTrain(eUnit, false, false))
				{
					return -1;
				}

				if (pPlot->isImpassable() && !GC.getUnitInfo(eUnit).isCanMoveImpassable())
				{
					return -1;
				}

				if (pPlot->getFeatureType() != NO_FEATURE)
				{
					if (GC.getUnitInfo(eUnit).getFeatureImpassable(pPlot->getFeatureType()))
					{
						TechTypes eTech = (TechTypes)GC.getUnitInfo(eUnit).getFeaturePassableTech(pPlot->getFeatureType());
						if (NO_TECH == eTech || !GET_TEAM(getTeam()).isHasTech(eTech))
						{
							return -1;
						}
					}
				}
				else
				{
					if (GC.getUnitInfo(eUnit).getTerrainImpassable(pPlot->getTerrainType()))
					{
						TechTypes eTech = (TechTypes)GC.getUnitInfo(eUnit).getTerrainPassableTech(pPlot->getTerrainType());
						if (NO_TECH == eTech || !GET_TEAM(getTeam()).isHasTech(eTech))
						{
							return -1;
						}
					}
				}
			}
		}
		// Must be this unit at plot in order to remove
		else
		{
			bool bUnitFound = false;

			CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();
			while (pUnitNode != NULL)
			{
				CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = pPlot->nextUnitNode(pUnitNode);

				if (pLoopUnit->getUnitType() == eUnit)
				{
					bUnitFound = true;
				}
			}

			if (!bUnitFound)
			{
				return -1;
			}
		}
	}

	// Increase cost if the XML defines that additional units will cost more
	if (GC.getUnitInfo(eUnit).getAdvancedStartCostIncrease() != 0)
	{
		for (CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
		{
			if (pLoopUnit->getUnitType() == eUnit)
			{
				++iNumUnitType;
			}
		}

		if (!bAdd)
		{
			--iNumUnitType;
		}

		if (iNumUnitType > 0)
		{
			iCost *= 100 + GC.getUnitInfo(eUnit).getAdvancedStartCostIncrease() * iNumUnitType;
			iCost /= 100;
		}
	}

	return adjustAdvStartPtsToSpeed(iCost); // advc.250c
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adding or removing a City
/////////////////////////////////////////////////////////////////////////////////////////////
int CvPlayer::getAdvancedStartCityCost(bool bAdd, CvPlot* pPlot) const
{
	int iNumCities = getNumCities();

	int iCost = getNewCityProductionValue();

	iCost = ::round(1.5 * iCost); // advc.250c

	if (iCost < 0)
	{
		return -1;
	}

	// Valid plot?
	if (pPlot != NULL)
	{
		// Need valid plot to found on if adding
		if (bAdd)
		{
			if (!canFound(pPlot->getX(), pPlot->getY(), false))
			{
				return -1;
			}
		}
		// Need your own city present to remove
		else
		{
			if (pPlot->isCity())
			{
				if (pPlot->getPlotCity()->getOwner() != getID())
				{
					return -1;
				}
			}
			else
			{
				return -1;
			}
		}

		// Is there a distance limit on how far a city can be placed from a player's start/another city?
		if (GC.getDefineINT("ADVANCED_START_CITY_PLACEMENT_MAX_RANGE") > 0)
		{
			PlayerTypes eClosestPlayer = NO_PLAYER;
			int iClosestDistance = MAX_INT;

			for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
			{
				CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);

				if (kPlayer.isAlive())
				{
					CvPlot* pStartingPlot = kPlayer.getStartingPlot();

					if (NULL != pStartingPlot)
					{
						int iDistance = ::plotDistance(pPlot->getX(), pPlot->getY(), pStartingPlot->getX(), pStartingPlot->getY());
						if (iDistance <= GC.getDefineINT("ADVANCED_START_CITY_PLACEMENT_MAX_RANGE"))
						{
							if (iDistance < iClosestDistance || (iDistance == iClosestDistance && getTeam() != kPlayer.getTeam()))
							{
								iClosestDistance = iDistance;
								eClosestPlayer = kPlayer.getID();
							}
						}
					}
				}
			}

			if (NO_PLAYER == eClosestPlayer || GET_PLAYER(eClosestPlayer).getTeam() != getTeam())
			{
				return -1;
			}
			//Only allow founding a city at someone else's start point if
			//We have no cities and they have no cities.
			if (getID() != eClosestPlayer && (getNumCities() > 0 || GET_PLAYER(eClosestPlayer).getNumCities() > 0))
			{
				return -1;
			}
		}
	}

	// Increase cost if the XML defines that additional units will cost more
	if (GC.getDefineINT("ADVANCED_START_CITY_COST_INCREASE") != 0)
	{
		if (!bAdd)
		{
			--iNumCities;
		}

		if (iNumCities > 0)
		{
			iCost *= 100 + GC.getDefineINT("ADVANCED_START_CITY_COST_INCREASE") * iNumCities;
			iCost /= 100;
		}
	} // <advc.250c>
	if(getNumCities() > 0) { // <dlph>
		for(int i = 0; i < GC.getNumUnitInfos(); i++) {
			UnitTypes eUnit = (UnitTypes)i;
			if(GC.getUnitInfo(eUnit).isFound()) {
				iCost *= 100;
				iCost /= std::max(1, 100 + getProductionModifier(eUnit));
				break;
			}
		} // </dlph>
	}
	return adjustAdvStartPtsToSpeed(iCost); // </advc.250c>
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adding or removing Population
/////////////////////////////////////////////////////////////////////////////////////////////
int CvPlayer::getAdvancedStartPopCost(bool bAdd, CvCity* pCity) const
{
	if (getNumCities() == 0)
	{
		return -1;
	}

	int iCost = (getGrowthThreshold(1) * GC.getDefineINT("ADVANCED_START_POPULATION_COST")) / 100;

	if (NULL != pCity)
	{
		if (pCity->getOwner() != getID())
		{
			return -1;
		}

		int iPopulation = pCity->getPopulation();

		// Need to have Population to remove it
		if (!bAdd)
		{
			--iPopulation;

			if (iPopulation < GC.getDefineINT("INITIAL_CITY_POPULATION") + GC.getEraInfo(GC.getGame().getStartEra()).getFreePopulation())
			{
				return -1;
			}
		}

		iCost = (getGrowthThreshold(iPopulation) * GC.getDefineINT("ADVANCED_START_POPULATION_COST")) / 100;

		// Increase cost if the XML defines that additional Pop will cost more
		if (GC.getDefineINT("ADVANCED_START_POPULATION_COST_INCREASE") != 0)
		{
			--iPopulation;

			if (iPopulation > 0)
			{
				iCost *= 100 + GC.getDefineINT("ADVANCED_START_POPULATION_COST_INCREASE") * iPopulation;
				iCost /= 100;
			}
		}
	}

	return adjustAdvStartPtsToSpeed(iCost); // advc.250c
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adding or removing Culture
/////////////////////////////////////////////////////////////////////////////////////////////
int CvPlayer::getAdvancedStartCultureCost(bool bAdd, CvCity* pCity) const
{	// <advc.003>
	if(getNumCities() == 0)
		return -1;
	int iCost = GC.getDefineINT("ADVANCED_START_CULTURE_COST");
	if(iCost < 0)
		return -1;
	if(pCity == NULL)
		return adjustAdvStartPtsToSpeed(iCost); // advc.250c
	if(pCity->getOwner() != getID())
		return -1; // </advc.003>

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
	iCost /= std::max(1, GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).
			getHurryPercent());
	return adjustAdvStartPtsToSpeed(iCost); // advc.250c
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adding or removing a Building from a city
/////////////////////////////////////////////////////////////////////////////////////////////
int CvPlayer::getAdvancedStartBuildingCost(BuildingTypes eBuilding, bool bAdd, CvCity* pCity) const
{
	if (getNumCities() <= 0)
		return -1;

	int iNumBuildingType = 0;

	int iCost = (getProductionNeeded(eBuilding) * GC.getBuildingInfo(eBuilding).getAdvancedStartCost()) / 100;
	iCost = ::round(iCost * 1.5); // advc.250c
	if (iCost < 0)
		return -1;

	if (GC.getGame().isFreeStartEraBuilding(eBuilding)) // advc.003
	{
		// you get this building for free
		return -1;
	}

	if (NULL == pCity)
	{
		if (bAdd)
		{
			bool bValid = false;
			int iLoop;
			for (CvCity* pLoopCity = firstCity(&iLoop); NULL != pLoopCity; pLoopCity = nextCity(&iLoop))
			{
				if (pLoopCity->canConstruct(eBuilding))
				{
					bValid = true;
					break;
				}
			}

			if (!bValid)
			{
				return -1;
			}
		}
	}
	if (NULL != pCity)
	{
		if (pCity->getOwner() != getID())
		{
			return -1;
		}

		iCost *= 100;
		iCost /= std::max(1, 100 + pCity->getProductionModifier(eBuilding));

		if (bAdd)
		{
			if (!pCity->canConstruct(eBuilding, true, false, false))
			{
				return -1;
			}
		}
		else
		{
			if (pCity->getNumRealBuilding(eBuilding) <= 0)
			{
				return -1;
			}

			// Check other buildings in this city and make sure none of them require this one

			// Loop through Buildings to see which are present
			for (int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
			{
				BuildingTypes eBuildingLoop = (BuildingTypes) iBuildingLoop;

				if (pCity->getNumBuilding(eBuildingLoop) > 0)
				{
					// Loop through present Building's requirements
					for (int iBuildingClassPrereqLoop = 0; iBuildingClassPrereqLoop < GC.getNumBuildingClassInfos(); iBuildingClassPrereqLoop++)
					{
						if (GC.getBuildingInfo(eBuildingLoop).isBuildingClassNeededInCity(iBuildingClassPrereqLoop))
						{
							if ((BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iBuildingClassPrereqLoop)) == eBuilding)
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
	if (GC.getBuildingInfo(eBuilding).getAdvancedStartCostIncrease() != 0)
	{
		iNumBuildingType = countNumBuildings(eBuilding);

		if (!bAdd)
		{
			--iNumBuildingType;
		}

		if (iNumBuildingType > 0)
		{
			iCost *= 100 + GC.getBuildingInfo(eBuilding).getAdvancedStartCostIncrease() * std::max(0, iNumBuildingType - getNumCities());
			iCost /= 100;
		}
	}

	return adjustAdvStartPtsToSpeed(iCost); // advc.250c
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adding or removing Route
/////////////////////////////////////////////////////////////////////////////////////////////
int CvPlayer::getAdvancedStartRouteCost(RouteTypes eRoute, bool bAdd, CvPlot* pPlot) const
{
	if (getNumCities() <= 0 || eRoute == NO_ROUTE)
		return -1;

	int iNumRoutes = 0;

	int iCost = GC.getRouteInfo(eRoute).getAdvancedStartCost();

	// <advc.250c>
	iCost *= GC.getDefineINT("ADVANCED_START_WORKER_BUILD_MODIFIER");
	iCost /= 100; // </advc.250c>

	// This denotes cities may not be purchased through Advanced Start
	if (iCost < 0)
	{
		return -1;
	}

	iCost *= GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getBuildPercent();
	iCost /= 100;

	// No invalid plots!
	if (pPlot != NULL)
	{
		if (pPlot->isCity())
		{
			return -1;
		}

		if (bAdd)
		{
			if (pPlot->isImpassable() || pPlot->isWater())
			{
				return -1;
			}
			// Can't place twice
			if (pPlot->getRouteType() == eRoute)
			{
				return -1;
			}
		}
		else
		{
			// Need Route to remove it
			if (pPlot->getRouteType() != eRoute)
			{
				return -1;
			}
		}

		// Must be owned by me
		if (pPlot->getOwner() != getID())
		{
			return -1;
		}
	}

	// Tech requirement
	for (int iBuildLoop = 0; iBuildLoop < GC.getNumBuildInfos(); iBuildLoop++)
	{
		if (GC.getBuildInfo((BuildTypes) iBuildLoop).getRoute() == eRoute)
		{
			if (!(GET_TEAM(getTeam()).isHasTech((TechTypes)GC.getBuildInfo((BuildTypes) iBuildLoop).getTechPrereq())))
			{
				return -1;
			}
		}
	}

	// Increase cost if the XML defines that additional units will cost more
	if (GC.getRouteInfo(eRoute).getAdvancedStartCostIncrease() != 0)
	{
		int iPlotLoop = 0;
		CvPlot* pPlot;

		for (iPlotLoop = 0; iPlotLoop < GC.getMap().numPlots(); iPlotLoop++)
		{
			pPlot = GC.getMap().plotByIndex(iPlotLoop);

			if (pPlot->getRouteType() == eRoute)
			{
				++iNumRoutes;
			}
		}

		if (!bAdd)
		{
			--iNumRoutes;
		}

		if (iNumRoutes > 0)
		{
			iCost *= 100 + GC.getRouteInfo(eRoute).getAdvancedStartCostIncrease() * iNumRoutes;
			iCost /= 100;
		}
	}

	return adjustAdvStartPtsToSpeed(iCost); // advc.250c
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adding or removing Improvement
/////////////////////////////////////////////////////////////////////////////////////////////
int CvPlayer::getAdvancedStartImprovementCost(ImprovementTypes eImprovement, bool bAdd, CvPlot* pPlot) const
{
	if (eImprovement == NO_IMPROVEMENT || getNumCities() <= 0)
		return -1;

	int iNumImprovements = 0;
	int iCost = GC.getImprovementInfo(eImprovement).getAdvancedStartCost();

	// <advc.250c>
	iCost *= GC.getDefineINT("ADVANCED_START_WORKER_BUILD_MODIFIER");
	iCost /= 100; // </advc.250c>
	// This denotes cities may not be purchased through Advanced Start
	if (iCost < 0)
		return -1;

	iCost *= GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getBuildPercent();
	iCost /= 100;

	// Can this Improvement be on our plot?
	if (pPlot != NULL)
	{
		if (bAdd)
		{
			// Valid Plot
			if (!pPlot->canHaveImprovement(eImprovement, getTeam(), false))
			{
				return -1;
			}

			bool bValid = false;

			for (int iI = 0; iI < GC.getNumBuildInfos(); ++iI)
			{
				CvBuildInfo& kBuild = GC.getBuildInfo((BuildTypes)iI);
				ImprovementTypes eLoopImprovement = ((ImprovementTypes)(kBuild.getImprovement()));

				if (eImprovement == eLoopImprovement && canBuild(pPlot, (BuildTypes)iI))
				{
					bValid = true;

					FeatureTypes eFeature = pPlot->getFeatureType();
					if (NO_FEATURE != eFeature && kBuild.isFeatureRemove(eFeature))
					{
						iCost += GC.getFeatureInfo(eFeature).getAdvancedStartRemoveCost();
					}

					break;
				}
			}

			if (!bValid)
			{
				return -1;
			}

			// Can't place twice
			if (pPlot->getImprovementType() == eImprovement)
			{
				return -1;
			}
		}
		else
		{
			// Need this improvement in order to remove it
			if (pPlot->getImprovementType() != eImprovement)
			{
				return -1;
			}
		}

		// Must be owned by me
		if (pPlot->getOwner() != getID())
		{
			return -1;
		}
	}

	// Tech requirement
	for (int iBuildLoop = 0; iBuildLoop < GC.getNumBuildInfos(); iBuildLoop++)
	{
		if (GC.getBuildInfo((BuildTypes) iBuildLoop).getImprovement() == eImprovement)
		{
			if (!(GET_TEAM(getTeam()).isHasTech((TechTypes)GC.getBuildInfo((BuildTypes) iBuildLoop).getTechPrereq())))
			{
				return -1;
			}
		}
	}

	// Increase cost if the XML defines that additional units will cost more
	if (GC.getImprovementInfo(eImprovement).getAdvancedStartCostIncrease() != 0)
	{
		int iPlotLoop = 0;
		CvPlot* pPlot;

		for (iPlotLoop = 0; iPlotLoop < GC.getMap().numPlots(); iPlotLoop++)
		{
			pPlot = GC.getMap().plotByIndex(iPlotLoop);
			if (pPlot->getImprovementType() == eImprovement)
			{
				++iNumImprovements;
			}
		}

		if (!bAdd)
		{
			--iNumImprovements;
		}

		if (iNumImprovements > 0)
		{
			iCost *= 100 + GC.getImprovementInfo(eImprovement).getAdvancedStartCostIncrease() * iNumImprovements;
			iCost /= 100;
		}
	}

	return adjustAdvStartPtsToSpeed(iCost); // advc.250c
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adding or removing Tech
/////////////////////////////////////////////////////////////////////////////////////////////
int CvPlayer::getAdvancedStartTechCost(TechTypes eTech, bool bAdd) const
{
	if (eTech == NO_TECH || getNumCities() <= 0)
		return -1;

	int iNumTechs = 0;

	int iCost = (GET_TEAM(getTeam()).getResearchCost(eTech) * GC.getTechInfo(eTech).getAdvancedStartCost()) / 100;
	if (iCost < 0)
		return -1;

	if (bAdd)
	{
		if (!canResearch(eTech, false))
		{
			return -1;
		}
	}
	else if (!bAdd)
	{
		if (!GET_TEAM(getTeam()).isHasTech(eTech))
		{
			return -1;
		}

		// Search through all techs to see if any of the currently owned ones requires this tech
		for (int iTechLoop = 0; iTechLoop < GC.getNumTechInfos(); iTechLoop++)
		{
			TechTypes eTechLoop = (TechTypes) iTechLoop;

			if (GET_TEAM(getTeam()).isHasTech(eTechLoop))
			{
				int iPrereqLoop;

				// Or Prereqs
				for (iPrereqLoop = 0; iPrereqLoop < GC.getNUM_OR_TECH_PREREQS(); iPrereqLoop++)
				{
					if (GC.getTechInfo(eTechLoop).getPrereqOrTechs(iPrereqLoop) == eTech)
					{
						return -1;
					}
				}

				// And Prereqs
				for (iPrereqLoop = 0; iPrereqLoop < GC.getNUM_AND_TECH_PREREQS(); iPrereqLoop++)
				{
					if (GC.getTechInfo(eTechLoop).getPrereqAndTechs(iPrereqLoop) == eTech)
					{
						return -1;
					}
				}
			}
		}

		// If player has placed anything on the map which uses this tech then you cannot remove it
		int iLoop;

		// Units
		CvUnit* pLoopUnit;
		for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
		{
			if (pLoopUnit->getUnitInfo().getPrereqAndTech() == eTech)
			{
				return -1;
			}

			for (int iI = 0; iI < GC.getNUM_UNIT_AND_TECH_PREREQS(); iI++)
			{
				if (pLoopUnit->getUnitInfo().getPrereqAndTechs(iI) == eTech)
				{
					return -1;
				}
			}
		}

		// Cities
		CvCity* pLoopCity;
		for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			// All Buildings
			for (int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
			{
				BuildingTypes eBuilding = (BuildingTypes) iBuildingLoop;

				if (pLoopCity->getNumRealBuilding(eBuilding) > 0)
				{
					if (GC.getBuildingInfo(eBuilding).getPrereqAndTech() == eTech)
					{
						return -1;
					}

					for (int iI = 0; iI < GC.getNUM_BUILDING_AND_TECH_PREREQS(); iI++)
					{
						if (GC.getBuildingInfo(eBuilding).getPrereqAndTechs(iI) == eTech)
						{
							return -1;
						}
					}
				}
			}
		}

	}

	// Increase cost if the XML defines that additional units will cost more
	if (GC.getTechInfo(eTech).getAdvancedStartCostIncrease() != 0)
	{
		for (int iTechLoop = 0; iTechLoop < GC.getNumTechInfos(); iTechLoop++)
		{
			if (GET_TEAM(getTeam()).isHasTech((TechTypes) iTechLoop))
			{
				++iNumTechs;
			}
		}

		if (!bAdd)
		{
			--iNumTechs;
		}

		if (iNumTechs > 0)
		{
			iCost *= 100 + GC.getTechInfo(eTech).getAdvancedStartCostIncrease() * iNumTechs;
			iCost /= 100;
		}
	}

	return adjustAdvStartPtsToSpeed(iCost); // advc.250c
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adding or removing Visibility
/////////////////////////////////////////////////////////////////////////////////////////////
int CvPlayer::getAdvancedStartVisibilityCost(bool bAdd, CvPlot* pPlot) const
{
	if (getNumCities() <= 0)
		return -1;

	int iNumVisiblePlots = 0;
	int iCost = GC.getDefineINT("ADVANCED_START_VISIBILITY_COST");
	// This denotes Visibility may not be purchased through Advanced Start
	if (iCost == -1)
		return -1;

	// Valid Plot?
	if(pPlot != NULL) {
		if(bAdd) {
			if(pPlot->isRevealed(getTeam(), false))
				return -1;
			if(!pPlot->isAdjacentRevealed(getTeam(),
					GC.getGame().getStartEra() < 4)) // advc.250c
				return -1;
		}
		else if (!pPlot->isRevealed(getTeam(), false))
			return -1;
	}

	// Increase cost if the XML defines that additional units will cost more
	if (GC.getDefineINT("ADVANCED_START_VISIBILITY_COST_INCREASE") != 0)
	{
		int iPlotLoop = 0;
		for (iPlotLoop = 0; iPlotLoop < GC.getMap().numPlots(); iPlotLoop++)
		{
			CvPlot* pPlot = GC.getMap().plotByIndex(iPlotLoop);

			if (pPlot->isRevealed(getTeam(), false))
			{
				++iNumVisiblePlots;
			}
		}

		if (!bAdd)
		{
			--iNumVisiblePlots;
		}
		iNumVisiblePlots -= NUM_CITY_PLOTS; // advc.210c
		if (iNumVisiblePlots > 0)
		{
			iCost *= 100 + GC.getDefineINT("ADVANCED_START_VISIBILITY_COST_INCREASE") * iNumVisiblePlots;
			iCost /= 100;
		}
	}

	return adjustAdvStartPtsToSpeed(iCost); // advc.250c
}

// <advc.250c> Should arguably be at CvGame, but more convenient to use at CvPlayer.
int CvPlayer::adjustAdvStartPtsToSpeed(int iPoints) {

	iPoints *= 100;
	return std::max(0, iPoints / GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).
			getGrowthPercent());
} // </advc.250c>


void CvPlayer::doWarnings()
{
	//update enemy unit in your territory glow
	int iLoop;
	for(CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		//update glow
		gDLL->getEntityIFace()->updateEnemyGlow(pLoopUnit->getUnitEntity());
	}

	//update enemy units close to your territory
	int iMaxCount = range(((getNumCities() + 4) / 7), 2, 5);
	for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
	{
		if (iMaxCount == 0)
		{
			break;
		}

		CvPlot* pLoopPlot = GC.getMap().plotByIndex(iI);
		if (pLoopPlot->isAdjacentPlayer(getID()))
		{
			if (!(pLoopPlot->isCity()))
			{
				if (pLoopPlot->isVisible(getTeam(), false))
				{
					CvUnit *pUnit = pLoopPlot->getVisibleEnemyDefender(getID());
					if (pUnit != NULL)
					{
						if (!pUnit->isAnimal())
						{
							CvCity* pNearestCity = GC.getMap().findCity(pLoopPlot->getX(), pLoopPlot->getY(), getID(), NO_TEAM, !(pLoopPlot->isWater()));
							if (pNearestCity != NULL)
							{
								wchar szBuffer[1024];
								swprintf(szBuffer, gDLL->getText("TXT_KEY_MISC_ENEMY_TROOPS_SPOTTED", pNearestCity->getNameKey()).GetCString());
								gDLL->getInterfaceIFace()->addHumanMessage(getID(), true, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_ENEMY_TROOPS", MESSAGE_TYPE_INFO, pUnit->getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pLoopPlot->getX(), pLoopPlot->getY(), true, true);

								iMaxCount--;
							}
						}
					}
				}
			}
		}
	} // <advc.002e>
	if(GC.IsGraphicsInitialized())
		showForeignPromoGlow(true); // </advc.002e>
}


void CvPlayer::verifyGoldCommercePercent()
{
	/* original bts code
	while ((getGold() + calculateGoldRate()) < 0) {
		changeCommercePercent(COMMERCE_GOLD, GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS"));
		if (getCommercePercent(COMMERCE_GOLD) == 100)
			break;
	} */
	// K-Mod
	bool bValid = isCommerceFlexible(COMMERCE_GOLD);
	while (bValid && getCommercePercent(COMMERCE_GOLD) < 100 && getGold() + calculateGoldRate() < 0)
	{
		bValid = changeCommercePercent(COMMERCE_GOLD, GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS"));
	} // K-Mod end
}


void CvPlayer::processCivics(CivicTypes eCivic, int iChange)
{
	int iI, iJ;

	changeGreatPeopleRateModifier(GC.getCivicInfo(eCivic).getGreatPeopleRateModifier() * iChange);
	changeGreatGeneralRateModifier(GC.getCivicInfo(eCivic).getGreatGeneralRateModifier() * iChange);
	changeDomesticGreatGeneralRateModifier(GC.getCivicInfo(eCivic).getDomesticGreatGeneralRateModifier() * iChange);
	changeStateReligionGreatPeopleRateModifier(GC.getCivicInfo(eCivic).getStateReligionGreatPeopleRateModifier() * iChange);
	changeDistanceMaintenanceModifier(GC.getCivicInfo(eCivic).getDistanceMaintenanceModifier() * iChange);
	changeNumCitiesMaintenanceModifier(GC.getCivicInfo(eCivic).getNumCitiesMaintenanceModifier() * iChange);
	changeCorporationMaintenanceModifier(GC.getCivicInfo(eCivic).getCorporationMaintenanceModifier() * iChange);
	changeExtraHealth(GC.getCivicInfo(eCivic).getExtraHealth() * iChange);
	changeExtraHappiness(GC.getCivicInfo(eCivic).getExtraHappiness() * iChange); // K-Mod
	changeFreeExperience(GC.getCivicInfo(eCivic).getFreeExperience() * iChange);
	changeWorkerSpeedModifier(GC.getCivicInfo(eCivic).getWorkerSpeedModifier() * iChange);
	changeImprovementUpgradeRateModifier(GC.getCivicInfo(eCivic).getImprovementUpgradeRateModifier() * iChange);
	changeMilitaryProductionModifier(GC.getCivicInfo(eCivic).getMilitaryProductionModifier() * iChange);
	changeBaseFreeUnits(GC.getCivicInfo(eCivic).getBaseFreeUnits() * iChange);
	changeBaseFreeMilitaryUnits(GC.getCivicInfo(eCivic).getBaseFreeMilitaryUnits() * iChange);
	changeFreeUnitsPopulationPercent(GC.getCivicInfo(eCivic).getFreeUnitsPopulationPercent() * iChange);
	changeFreeMilitaryUnitsPopulationPercent(GC.getCivicInfo(eCivic).getFreeMilitaryUnitsPopulationPercent() * iChange);
	changeGoldPerUnit(GC.getCivicInfo(eCivic).getGoldPerUnit() * iChange);
	changeGoldPerMilitaryUnit(GC.getCivicInfo(eCivic).getGoldPerMilitaryUnit() * iChange);
	changeHappyPerMilitaryUnit(GC.getCivicInfo(eCivic).getHappyPerMilitaryUnit() * iChange);
	// <advc.912c>
	changeLuxuryModifier(GC.getCivicInfo(eCivic).getLuxuryModifier() * iChange);
	if(GC.getCivicInfo(eCivic).getLuxuryModifier() != 0)
		AI().AI_updateBonusValue(); // </advc.912c>
	changeMilitaryFoodProductionCount((GC.getCivicInfo(eCivic).isMilitaryFoodProduction()) ? iChange : 0);
	changeMaxConscript(getWorldSizeMaxConscript(eCivic) * iChange);
	//changeNoUnhealthyPopulationCount((GC.getCivicInfo(eCivic).isNoUnhealthyPopulation()) ? iChange : 0);
	changeUnhealthyPopulationModifier(GC.getCivicInfo(eCivic).getUnhealthyPopulationModifier() * iChange); // K-Mod
	changeBuildingOnlyHealthyCount((GC.getCivicInfo(eCivic).isBuildingOnlyHealthy()) ? iChange : 0);
	changeLargestCityHappiness(GC.getCivicInfo(eCivic).getLargestCityHappiness() * iChange);
	changeWarWearinessModifier(GC.getCivicInfo(eCivic).getWarWearinessModifier() * iChange);
	changeFreeSpecialist(GC.getCivicInfo(eCivic).getFreeSpecialist() * iChange);
	changeTradeRoutes(GC.getCivicInfo(eCivic).getTradeRoutes() * iChange);
	changeNoForeignTradeCount(GC.getCivicInfo(eCivic).isNoForeignTrade() * iChange);
	changeNoCorporationsCount(GC.getCivicInfo(eCivic).isNoCorporations() * iChange);
	changeNoForeignCorporationsCount(GC.getCivicInfo(eCivic).isNoForeignCorporations() * iChange);
	changeStateReligionCount((GC.getCivicInfo(eCivic).isStateReligion()) ? iChange : 0);
	changeNoNonStateReligionSpreadCount((GC.getCivicInfo(eCivic).isNoNonStateReligionSpread()) ? iChange : 0);
	changeStateReligionHappiness(GC.getCivicInfo(eCivic).getStateReligionHappiness() * iChange);
	changeNonStateReligionHappiness(GC.getCivicInfo(eCivic).getNonStateReligionHappiness() * iChange);
	changeStateReligionUnitProductionModifier(GC.getCivicInfo(eCivic).getStateReligionUnitProductionModifier() * iChange);
	changeStateReligionBuildingProductionModifier(GC.getCivicInfo(eCivic).getStateReligionBuildingProductionModifier() * iChange);
	changeStateReligionFreeExperience(GC.getCivicInfo(eCivic).getStateReligionFreeExperience() * iChange);
	changeExpInBorderModifier(GC.getCivicInfo(eCivic).getExpInBorderModifier() * iChange);

	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		changeYieldRateModifier(((YieldTypes)iI), (GC.getCivicInfo(eCivic).getYieldModifier(iI) * iChange));
		changeCapitalYieldRateModifier(((YieldTypes)iI), (GC.getCivicInfo(eCivic).getCapitalYieldModifier(iI) * iChange));
		changeTradeYieldModifier(((YieldTypes)iI), (GC.getCivicInfo(eCivic).getTradeYieldModifier(iI) * iChange));
	}

	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		changeCommerceRateModifier(((CommerceTypes)iI), (GC.getCivicInfo(eCivic).getCommerceModifier(iI) * iChange));
		changeCapitalCommerceRateModifier(((CommerceTypes)iI), (GC.getCivicInfo(eCivic).getCapitalCommerceModifier(iI) * iChange));
		changeSpecialistExtraCommerce(((CommerceTypes)iI), (GC.getCivicInfo(eCivic).getSpecialistExtraCommerce(iI) * iChange));
	}

	for (iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingTypes eOurBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI);
		if (NO_BUILDING != eOurBuilding)
		{
			changeExtraBuildingHappiness(eOurBuilding, (GC.getCivicInfo(eCivic).getBuildingHappinessChanges(iI) * iChange));
			changeExtraBuildingHealth(eOurBuilding, (GC.getCivicInfo(eCivic).getBuildingHealthChanges(iI) * iChange));
		}
	}

	for (iI = 0; iI < GC.getNumFeatureInfos(); iI++)
	{
		changeFeatureHappiness(((FeatureTypes)iI), (GC.getCivicInfo(eCivic).getFeatureHappinessChanges(iI) * iChange));
	}

	for (iI = 0; iI < GC.getNumHurryInfos(); iI++)
	{
		changeHurryCount(((HurryTypes)iI), ((GC.getCivicInfo(eCivic).isHurry(iI)) ? iChange : 0));
	}

	for (iI = 0; iI < GC.getNumSpecialBuildingInfos(); iI++)
	{
		changeSpecialBuildingNotRequiredCount(((SpecialBuildingTypes)iI), ((GC.getCivicInfo(eCivic).isSpecialBuildingNotRequired(iI)) ? iChange : 0));
	}

	for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
	{
		changeSpecialistValidCount(((SpecialistTypes)iI), ((GC.getCivicInfo(eCivic).isSpecialistValid(iI)) ? iChange : 0));
	}

	for (iI = 0; iI < GC.getNumImprovementInfos(); iI++)
	{
		for (iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
		{
			changeImprovementYieldChange(((ImprovementTypes)iI), ((YieldTypes)iJ), (GC.getCivicInfo(eCivic).getImprovementYieldChanges(iI, iJ) * iChange));
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
			gDLL->getInterfaceIFace()->showMessage(msg);
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


// read object from a stream
// used during load
void CvPlayer::read(FDataStreamBase* pStream)
{
	int iI=0;
	// Init data before load
	reset();

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iStartingX);
	pStream->Read(&m_iStartingY);
	pStream->Read(&m_iTotalPopulation);
	pStream->Read(&m_iTotalLand);
	pStream->Read(&m_iTotalLandScored);
	pStream->Read(&m_iGold);
	pStream->Read(&m_iGoldPerTurn);
	pStream->Read(&m_iAdvancedStartPoints);
	pStream->Read(&m_iGoldenAgeTurns);
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
	if(uiFlag >= 8) {
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

	pStream->Read(NUM_YIELD_TYPES, m_aiSeaPlotYield);
	pStream->Read(NUM_YIELD_TYPES, m_aiYieldRateModifier);
	pStream->Read(NUM_YIELD_TYPES, m_aiCapitalYieldRateModifier);
	pStream->Read(NUM_YIELD_TYPES, m_aiExtraYieldThreshold);
	pStream->Read(NUM_YIELD_TYPES, m_aiTradeYieldModifier);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiFreeCityCommerce);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiCommercePercent);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiCommerceRate);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiCommerceRateModifier);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiCapitalCommerceRateModifier);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiStateReligionBuildingCommerce);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiSpecialistExtraCommerce);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiCommerceFlexibleCount);
	pStream->Read(MAX_PLAYERS, m_aiGoldPerTurnByPlayer);
	pStream->Read(MAX_TEAMS, m_aiEspionageSpendingWeightAgainstTeam);

	pStream->Read(NUM_FEAT_TYPES, m_abFeatAccomplished);
	pStream->Read(NUM_PLAYEROPTION_TYPES, m_abOptions);

	pStream->ReadString(m_szScriptData);

	FAssertMsg((0 < GC.getNumBonusInfos()), "GC.getNumBonusInfos() is not greater than zero but it is expected to be in CvPlayer::read");
	pStream->Read(GC.getNumBonusInfos(), m_paiBonusExport);
	pStream->Read(GC.getNumBonusInfos(), m_paiBonusImport);
	pStream->Read(GC.getNumImprovementInfos(), m_paiImprovementCount);
	pStream->Read(GC.getNumBuildingInfos(), m_paiFreeBuildingCount);
	pStream->Read(GC.getNumBuildingInfos(), m_paiExtraBuildingHappiness);
	pStream->Read(GC.getNumBuildingInfos(), m_paiExtraBuildingHealth);
	pStream->Read(GC.getNumFeatureInfos(), m_paiFeatureHappiness);
	pStream->Read(GC.getNumUnitClassInfos(), m_paiUnitClassCount);
	pStream->Read(GC.getNumUnitClassInfos(), m_paiUnitClassMaking);
	pStream->Read(GC.getNumBuildingClassInfos(), m_paiBuildingClassCount);
	pStream->Read(GC.getNumBuildingClassInfos(), m_paiBuildingClassMaking);
	pStream->Read(GC.getNumHurryInfos(), m_paiHurryCount);
	pStream->Read(GC.getNumSpecialBuildingInfos(), m_paiSpecialBuildingNotRequiredCount);
	pStream->Read(GC.getNumCivicOptionInfos(), m_paiHasCivicOptionCount);
	pStream->Read(GC.getNumCivicOptionInfos(), m_paiNoCivicUpkeepCount);
	pStream->Read(GC.getNumReligionInfos(), m_paiHasReligionCount);
	pStream->Read(GC.getNumCorporationInfos(), m_paiHasCorporationCount);
	pStream->Read(GC.getNumUpkeepInfos(), m_paiUpkeepCount);
	pStream->Read(GC.getNumSpecialistInfos(), m_paiSpecialistValidCount);

	FAssertMsg((0 < GC.getNumTechInfos()), "GC.getNumTechInfos() is not greater than zero but it is expected to be in CvPlayer::read");
	pStream->Read(GC.getNumTechInfos(), m_pabResearchingTech);

	pStream->Read(GC.getNumVoteSourceInfos(), m_pabLoyalMember);

	for (iI=0;iI<GC.getNumCivicOptionInfos();iI++)
	{
		pStream->Read((int*)&m_paeCivics[iI]);
	}

	for (iI=0;iI<GC.getNumSpecialistInfos();iI++)
	{
		pStream->Read(NUM_YIELD_TYPES, m_ppaaiSpecialistExtraYield[iI]);
	}

	for (iI=0;iI<GC.getNumImprovementInfos();iI++)
	{
		pStream->Read(NUM_YIELD_TYPES, m_ppaaiImprovementYieldChange[iI]);
	}

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
			if (NULL != pDiplo)
			{
				pDiplo->read(*pStream);
				m_listDiplomacy.push_back(pDiplo);
			}
		}
	}

	{
		uint iSize;
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			int iTurn;
			int iScore;
			pStream->Read(&iTurn);
			pStream->Read(&iScore);
			m_mapScoreHistory[iTurn] = iScore;
		}
	}

	{
		m_mapEconomyHistory.clear();
		uint iSize;
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			int iTurn;
			int iScore;
			pStream->Read(&iTurn);
			pStream->Read(&iScore);
			m_mapEconomyHistory[iTurn] = iScore;
		}
	}

	{
		m_mapIndustryHistory.clear();
		uint iSize;
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			int iTurn;
			int iScore;
			pStream->Read(&iTurn);
			pStream->Read(&iScore);
			m_mapIndustryHistory[iTurn] = iScore;
		}
	}

	{
		m_mapAgricultureHistory.clear();
		uint iSize;
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			int iTurn;
			int iScore;
			pStream->Read(&iTurn);
			pStream->Read(&iScore);
			m_mapAgricultureHistory[iTurn] = iScore;
		}
	}

	{
		m_mapPowerHistory.clear();
		uint iSize;
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			int iTurn;
			int iScore;
			pStream->Read(&iTurn);
			pStream->Read(&iScore);
			m_mapPowerHistory[iTurn] = iScore;
		}
	}

	{
		m_mapCultureHistory.clear();
		uint iSize;
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			int iTurn;
			int iScore;
			pStream->Read(&iTurn);
			pStream->Read(&iScore);
			m_mapCultureHistory[iTurn] = iScore;
		}
	}

	{
		m_mapEspionageHistory.clear();
		uint iSize;
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			int iTurn;
			int iScore;
			pStream->Read(&iTurn);
			pStream->Read(&iScore);
			m_mapEspionageHistory[iTurn] = iScore;
		}
	}

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
		int iNumEventTriggers = std::min(176, GC.getNumEventTriggerInfos()); // yuck, hardcoded number of eventTriggers in the epic game in initial release
		for (iI=0; iI < iNumEventTriggers; iI++)
		{
			bool bTriggered;
			pStream->Read(&bTriggered);
			if (bTriggered)
			{
				m_triggersFired.push_back((EventTriggerTypes)iI);
			}
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
	else {
		for(int i = 0; i < GC.getNumHurryInfos(); i++) {
			if(GC.getHurryInfo((HurryTypes)i).getGoldPerProduction() > 0) {
				m_iGoldRushHurryCount = m_paiHurryCount[i];
				break;
			}
		}
	} // </advc.064b>
	pStream->Read(&m_iInflationModifier);

	if(!isAlive())
		return; // advc.003
	// <advc.210>
	for(size_t i = 0; i < m_paAlerts.size(); i++)
		m_paAlerts[i]->init(getID()); // </advc.210>
	/*  <advc.706> Loading into retirement. Can't do this in RiseFall::read b/c
		CvPlayer::reset has to be through first. */
	CvGame& g = GC.getGame();
	if(g.isOption(GAMEOPTION_RISE_FALL) && g.getRiseFall().hasRetired())
		g.getRiseFall().retire(); // </advc.706>
	// <advc.908a>
	if(uiFlag < 5 && !isBarbarian()) {
		for(int i = 0; i < NUM_YIELD_TYPES; i++)
			updateExtraYieldThreshold((YieldTypes)i);
	} // </advc.908a>
	// <advc.912c>
	if(uiFlag <= 6) {
		for(int i = 0; i < GC.getNumCivicInfos(); i++) {
			CivicTypes cvId = (CivicTypes)i;
			if(!isCivic(cvId))
				continue;
			CvCivicInfo& cv = GC.getCivicInfo(cvId);
			if(cv.getLuxuryModifier() <= 0)
				continue;
			int previousHappyPer = (uiFlag < 6 ? 1 : 0);
			int previousLux =  (uiFlag < 6 ? 0 : 50);
			changeHappyPerMilitaryUnit(cv.getHappyPerMilitaryUnit() -
					previousHappyPer);
			changeLuxuryModifier(cv.getLuxuryModifier() - previousLux);
		}
	} // </advc.912c>
}

//
// save object to a stream
// used during save
//
void CvPlayer::write(FDataStreamBase* pStream)
{
	int iI;

	uint uiFlag = 4;
	uiFlag = 5; // advc.908a
	uiFlag = 7; // advc.912c (6 used up for a test version)
	uiFlag = 8; // advc.004x
	uiFlag = 9; // advc.078
	uiFlag = 10; // advc.064b
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iStartingX);
	pStream->Write(m_iStartingY);
	pStream->Write(m_iTotalPopulation);
	pStream->Write(m_iTotalLand);
	pStream->Write(m_iTotalLandScored);
	pStream->Write(m_iGold);
	pStream->Write(m_iGoldPerTurn);
	pStream->Write(m_iAdvancedStartPoints);
	pStream->Write(m_iGoldenAgeTurns);
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

	pStream->Write(NUM_YIELD_TYPES, m_aiSeaPlotYield);
	pStream->Write(NUM_YIELD_TYPES, m_aiYieldRateModifier);
	pStream->Write(NUM_YIELD_TYPES, m_aiCapitalYieldRateModifier);
	pStream->Write(NUM_YIELD_TYPES, m_aiExtraYieldThreshold);
	pStream->Write(NUM_YIELD_TYPES, m_aiTradeYieldModifier);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiFreeCityCommerce);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiCommercePercent);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiCommerceRate);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiCommerceRateModifier);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiCapitalCommerceRateModifier);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiStateReligionBuildingCommerce);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiSpecialistExtraCommerce);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiCommerceFlexibleCount);
	pStream->Write(MAX_PLAYERS, m_aiGoldPerTurnByPlayer);
	pStream->Write(MAX_TEAMS, m_aiEspionageSpendingWeightAgainstTeam);

	pStream->Write(NUM_FEAT_TYPES, m_abFeatAccomplished);
	pStream->Write(NUM_PLAYEROPTION_TYPES, m_abOptions);

	pStream->WriteString(m_szScriptData);

	FAssertMsg((0 < GC.getNumBonusInfos()), "GC.getNumBonusInfos() is not greater than zero but an array is being allocated in CvPlayer::write");
	pStream->Write(GC.getNumBonusInfos(), m_paiBonusExport);
	pStream->Write(GC.getNumBonusInfos(), m_paiBonusImport);
	pStream->Write(GC.getNumImprovementInfos(), m_paiImprovementCount);
	pStream->Write(GC.getNumBuildingInfos(), m_paiFreeBuildingCount);
	pStream->Write(GC.getNumBuildingInfos(), m_paiExtraBuildingHappiness);
	pStream->Write(GC.getNumBuildingInfos(), m_paiExtraBuildingHealth);
	pStream->Write(GC.getNumFeatureInfos(), m_paiFeatureHappiness);
	pStream->Write(GC.getNumUnitClassInfos(), m_paiUnitClassCount);
	pStream->Write(GC.getNumUnitClassInfos(), m_paiUnitClassMaking);
	pStream->Write(GC.getNumBuildingClassInfos(), m_paiBuildingClassCount);
	pStream->Write(GC.getNumBuildingClassInfos(), m_paiBuildingClassMaking);
	pStream->Write(GC.getNumHurryInfos(), m_paiHurryCount);
	pStream->Write(GC.getNumSpecialBuildingInfos(), m_paiSpecialBuildingNotRequiredCount);
	pStream->Write(GC.getNumCivicOptionInfos(), m_paiHasCivicOptionCount);
	pStream->Write(GC.getNumCivicOptionInfos(), m_paiNoCivicUpkeepCount);
	pStream->Write(GC.getNumReligionInfos(), m_paiHasReligionCount);
	pStream->Write(GC.getNumCorporationInfos(), m_paiHasCorporationCount);
	pStream->Write(GC.getNumUpkeepInfos(), m_paiUpkeepCount);
	pStream->Write(GC.getNumSpecialistInfos(), m_paiSpecialistValidCount);

	FAssertMsg((0 < GC.getNumTechInfos()), "GC.getNumTechInfos() is not greater than zero but it is expected to be in CvPlayer::write");
	pStream->Write(GC.getNumTechInfos(), m_pabResearchingTech);

	pStream->Write(GC.getNumVoteSourceInfos(), m_pabLoyalMember);

	for (iI=0;iI<GC.getNumCivicOptionInfos();iI++)
	{
		pStream->Write(m_paeCivics[iI]);
	}

	for (iI=0;iI<GC.getNumSpecialistInfos();iI++)
	{
		pStream->Write(NUM_YIELD_TYPES, m_ppaaiSpecialistExtraYield[iI]);
	}

	for (iI=0;iI<GC.getNumImprovementInfos();iI++)
	{
		pStream->Write(NUM_YIELD_TYPES, m_ppaaiImprovementYieldChange[iI]);
	}

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

	WriteStreamableFFreeListTrashArray(m_plotGroups, pStream);
	WriteStreamableFFreeListTrashArray(m_cities, pStream);
	WriteStreamableFFreeListTrashArray(m_units, pStream);
	WriteStreamableFFreeListTrashArray(m_selectionGroups, pStream);
	WriteStreamableFFreeListTrashArray(m_eventsTriggered, pStream);

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
		if (GC.getGame().isNetworkMultiPlayer())
		{
			// don't save open popups in MP to avoid having different state on different machines
			currentPopups.clear();
		}
		else
		{
			gDLL->getInterfaceIFace()->getDisplayedButtonPopups(currentPopups);
		}
		CvPopupQueue::_Alloc::size_type iSize = m_listPopups.size() + currentPopups.size();
		pStream->Write(iSize);
		CvPopupQueue::iterator it;
		for (it = currentPopups.begin(); it != currentPopups.end(); ++it)
		{
			CvPopupInfo* pInfo = *it;
			if (NULL != pInfo)
			{
				pInfo->write(*pStream);
			}
		}
		for (it = m_listPopups.begin(); it != m_listPopups.end(); ++it)
		{
			CvPopupInfo* pInfo = *it;
			if (NULL != pInfo)
			{
				pInfo->write(*pStream);
			}
		}
	}

	{
		CvDiploQueue::_Alloc::size_type iSize = m_listDiplomacy.size();
		pStream->Write(iSize);
		CvDiploQueue::iterator it;
		for (it = m_listDiplomacy.begin(); it != m_listDiplomacy.end(); ++it)
		{
			CvDiploParameters* pDiplo = *it;
			if (NULL != pDiplo)
			{
				pDiplo->write(*pStream);
			}
		}
	}

	{
		uint iSize = m_mapScoreHistory.size();
		pStream->Write(iSize);
		CvTurnScoreMap::iterator it;
		for (it = m_mapScoreHistory.begin(); it != m_mapScoreHistory.end(); ++it)
		{
			pStream->Write(it->first);
			pStream->Write(it->second);
		}
	}

	{
		uint iSize = m_mapEconomyHistory.size();
		pStream->Write(iSize);
		CvTurnScoreMap::iterator it;
		for (it = m_mapEconomyHistory.begin(); it != m_mapEconomyHistory.end(); ++it)
		{
			pStream->Write(it->first);
			pStream->Write(it->second);
		}
	}

	{
		uint iSize = m_mapIndustryHistory.size();
		pStream->Write(iSize);
		CvTurnScoreMap::iterator it;
		for (it = m_mapIndustryHistory.begin(); it != m_mapIndustryHistory.end(); ++it)
		{
			pStream->Write(it->first);
			pStream->Write(it->second);
		}
	}

	{
		uint iSize = m_mapAgricultureHistory.size();
		pStream->Write(iSize);
		CvTurnScoreMap::iterator it;
		for (it = m_mapAgricultureHistory.begin(); it != m_mapAgricultureHistory.end(); ++it)
		{
			pStream->Write(it->first);
			pStream->Write(it->second);
		}
	}

	{
		uint iSize = m_mapPowerHistory.size();
		pStream->Write(iSize);
		CvTurnScoreMap::iterator it;
		for (it = m_mapPowerHistory.begin(); it != m_mapPowerHistory.end(); ++it)
		{
			pStream->Write(it->first);
			pStream->Write(it->second);
		}
	}

	{
		uint iSize = m_mapCultureHistory.size();
		pStream->Write(iSize);
		CvTurnScoreMap::iterator it;
		for (it = m_mapCultureHistory.begin(); it != m_mapCultureHistory.end(); ++it)
		{
			pStream->Write(it->first);
			pStream->Write(it->second);
		}
	}

	{
		uint iSize = m_mapEspionageHistory.size();
		pStream->Write(iSize);
		CvTurnScoreMap::iterator it;
		for (it = m_mapEspionageHistory.begin(); it != m_mapEspionageHistory.end(); ++it)
		{
			pStream->Write(it->first);
			pStream->Write(it->second);
		}
	}

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
			pStream->Write((*it).first);
			pStream->Write((*it).second);
		}
	}

	{
		uint iSize = m_aFreeUnitClassPromotions.size();
		pStream->Write(iSize);
		UnitClassPromotionArray::iterator it;
		for (it = m_aFreeUnitClassPromotions.begin(); it != m_aFreeUnitClassPromotions.end(); ++it)
		{
			pStream->Write((*it).first);
			pStream->Write((*it).second);
		}
	}

	{
		uint iSize = m_aVote.size();
		pStream->Write(iSize);
		std::vector< std::pair<int, PlayerVoteTypes> >::iterator it;
		for (it = m_aVote.begin(); it != m_aVote.end(); ++it)
		{
			pStream->Write((*it).first);
			pStream->Write((*it).second);
		}
	}

	{
		uint iSize = m_aUnitExtraCosts.size();
		pStream->Write(iSize);
		std::vector< std::pair<UnitClassTypes, int> >::iterator it;
		for (it = m_aUnitExtraCosts.begin(); it != m_aUnitExtraCosts.end(); ++it)
		{
			pStream->Write((*it).first);
			pStream->Write((*it).second);
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
}

void CvPlayer::createGreatPeople(UnitTypes eGreatPersonUnit,
		bool bIncrementThreshold, bool bIncrementExperience, int iX, int iY)
{
	CvPlot* pPlot = GC.getMap().plot(iX, iY);
	if (pPlot == NULL)
	{
		FAssertMsg(false, "Invalid plot in createGreatPeople()");
		return;
	}
	CvUnit* pGreatPeopleUnit = initUnit(eGreatPersonUnit, iX, iY);
	if (NULL == pGreatPeopleUnit)
	{
		FAssert(false);
		return;
	}
	CvCity* pCity = pPlot->getPlotCity();

	if (bIncrementThreshold)
	{
		incrementGreatPeopleCreated();

		changeGreatPeopleThresholdModifier(
				GC.getDefineINT("GREAT_PEOPLE_THRESHOLD_INCREASE") *
				((getGreatPeopleCreated() / 10) + 1));

		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (TEAMID((PlayerTypes)iI) == getTeam())
			{
				GET_PLAYER((PlayerTypes)iI).changeGreatPeopleThresholdModifier(
						GC.getDefineINT("GREAT_PEOPLE_THRESHOLD_INCREASE_TEAM") *
						((getGreatPeopleCreated() / 10) + 1));
			}
		}
	}

	if (bIncrementExperience)
	{
		incrementGreatGeneralsCreated();

		changeGreatGeneralsThresholdModifier(
				GC.getDefineINT("GREAT_GENERALS_THRESHOLD_INCREASE") *
				((getGreatGeneralsCreated() / 10) + 1));

		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (TEAMID((PlayerTypes)iI) == getTeam())
			{
				GET_PLAYER((PlayerTypes)iI).changeGreatGeneralsThresholdModifier(
						GC.getDefineINT("GREAT_GENERALS_THRESHOLD_INCREASE_TEAM") *
						((getGreatGeneralsCreated() / 10) + 1));
			}
		}
	}
	// <advc.106>
	CvPlayer const& kGPOwner = GET_PLAYER(pPlot->getOwner());
	// Use shorter message for replays
	CvWString szReplayMessage = gDLL->getText("TXT_KEY_MISC_GP_BORN_REPLAY",
			pGreatPeopleUnit->getReplayName().GetCString(),
			kGPOwner.getCivilizationDescriptionKey()); // </advc.106>
	GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(),
			szReplayMessage, iX, iY, (ColorTypes)GC.getInfoTypeForString(
			"COLOR_UNIT_TEXT"));
	// advc.106: Non-replay message
	CvWString szMessage;
	if (pCity)
	{
		CvWString szCity;
		szCity.Format(L"%s (%s)", pCity->getName().GetCString(), GET_PLAYER(pCity->getOwner()).getReplayName());
		szMessage = gDLL->getText("TXT_KEY_MISC_GP_BORN", pGreatPeopleUnit->getName().GetCString(), szCity.GetCString());
	}
	else
	{
		szMessage = gDLL->getText("TXT_KEY_MISC_GP_BORN_FIELD", pGreatPeopleUnit->getName().GetCString());
	} // <advc.106>
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayer& kObs = GET_PLAYER((PlayerTypes)i);
		if(!kObs.isAlive())
			continue;
		bool bRev = pPlot->isRevealed(kObs.getTeam(), true);
		if(!GET_TEAM(kObs.getTeam()).isHasMet(kGPOwner.getTeam()))
			continue;
		if(!bRev) {
			szMessage = gDLL->getText("TXT_KEY_MISC_GP_BORN_CIV",
					pGreatPeopleUnit->getName().GetCString(),
					kGPOwner.getCivilizationDescriptionKey());
		} // <advc.106b>
		InterfaceMessageTypes eMsgType = MESSAGE_TYPE_MINOR_EVENT;
		// Only birth of own GP is major
		// ^On second thought, make all GP births minor.
		/*if(kObs.getID() == kGPOwner.getID())
			eMsgType = MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY;*/ // </advc.106b>
		gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)i), false,
				GC.getEVENT_MESSAGE_TIME(), szMessage, "AS2D_UNIT_GREATPEOPLE",
				eMsgType, pGreatPeopleUnit->getButton(),
				NO_COLOR, // advc.106: Colored through XML now
				//(ColorTypes)GC.getInfoTypeForString("COLOR_UNIT_TEXT"),
				// Indicate location only if revealed.
				bRev ? iX : -1, bRev ? iY : -1, bRev, bRev);
	} // </advc.106>

	// Python Event
	if (pCity)
	{
		CvEventReporter::getInstance().greatPersonBorn(pGreatPeopleUnit, getID(), pCity);
	}
}


const EventTriggeredData* CvPlayer::getEventOccured(EventTypes eEvent) const
{
	FAssert(eEvent >= 0 && eEvent < GC.getNumEventInfos());

	CvEventMap::const_iterator it = m_mapEventsOccured.find(eEvent);

	if (it == m_mapEventsOccured.end())
	{
		return NULL;
	}

	return &(it->second);
}

bool CvPlayer::isTriggerFired(EventTriggerTypes eEventTrigger) const
{
	return (std::find(m_triggersFired.begin(), m_triggersFired.end(), eEventTrigger) != m_triggersFired.end());
}

void CvPlayer::resetEventOccured(EventTypes eEvent, bool bAnnounce)
{
	FAssert(eEvent >= 0 && eEvent < GC.getNumEventInfos());

	CvEventMap::iterator it = m_mapEventsOccured.find(eEvent);

	if (it != m_mapEventsOccured.end())
	{
		expireEvent(it->first, it->second, bAnnounce);
		m_mapEventsOccured.erase(it);
	}
}

void CvPlayer::setEventOccured(EventTypes eEvent, const EventTriggeredData& kEventTriggered, bool bOthers)
{
	FAssert(eEvent >= 0 && eEvent < GC.getNumEventInfos());

	m_mapEventsOccured[eEvent] = kEventTriggered;

	if (GC.getEventInfo(eEvent).isQuest())
	{
		CvWStringBuffer szMessageBuffer;
		szMessageBuffer.append(GC.getEventInfo(eEvent).getDescription());
		GAMETEXT.setEventHelp(szMessageBuffer, eEvent, kEventTriggered.getID(), getID());
		gDLL->getInterfaceIFace()->addQuestMessage(getID(), szMessageBuffer.getCString(), kEventTriggered.getID());
	}

	if (bOthers)
	{
		if (GC.getEventInfo(eEvent).isGlobal())
		{
			for (int i = 0; i < MAX_CIV_PLAYERS; i++)
			{
				if (i != getID())
				{
					GET_PLAYER((PlayerTypes)i).setEventOccured(eEvent, kEventTriggered, false);
				}
			}
		}
		else if (GC.getEventInfo(eEvent).isTeam())
		{
			for (int i = 0; i < MAX_CIV_PLAYERS; i++)
			{
				if (i != getID() && getTeam() == GET_PLAYER((PlayerTypes)i).getTeam())
				{
					GET_PLAYER((PlayerTypes)i).setEventOccured(eEvent, kEventTriggered, false);
				}
			}
		}
	}
}


const EventTriggeredData* CvPlayer::getEventCountdown(EventTypes eEvent) const
{
	FAssert(eEvent >= 0 && eEvent < GC.getNumEventInfos());

	CvEventMap::const_iterator it = m_mapEventCountdown.find(eEvent);

	if (it == m_mapEventCountdown.end())
	{
		return NULL;
	}

	return &(it->second);
}

void CvPlayer::setEventCountdown(EventTypes eEvent, const EventTriggeredData& kEventTriggered)
{
	FAssert(eEvent >= 0 && eEvent < GC.getNumEventInfos());

	m_mapEventCountdown[eEvent] = kEventTriggered;
}

void CvPlayer::resetEventCountdown(EventTypes eEvent)
{
	FAssert(eEvent >= 0 && eEvent < GC.getNumEventInfos());

	CvEventMap::iterator it = m_mapEventCountdown.find(eEvent);

	if (it != m_mapEventCountdown.end())
	{
		m_mapEventCountdown.erase(it);
	}
}


void CvPlayer::resetTriggerFired(EventTriggerTypes eTrigger)
{
	std::vector<EventTriggerTypes>::iterator it = std::find(m_triggersFired.begin(), m_triggersFired.end(), eTrigger);

	if (it != m_triggersFired.end())
	{
		m_triggersFired.erase(it);
	}
}

void CvPlayer::setTriggerFired(const EventTriggeredData& kTriggeredData, bool bOthers, bool bAnnounce)
{
	FAssert(kTriggeredData.m_eTrigger >= 0 && kTriggeredData.m_eTrigger < GC.getNumEventTriggerInfos());

	CvEventTriggerInfo& kTrigger = GC.getEventTriggerInfo(kTriggeredData.m_eTrigger);

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
					{
						GET_PLAYER((PlayerTypes)i).setTriggerFired(kTriggeredData, false, false);
					}
				}
			}
			else if (kTrigger.isTeam())
			{
				for (int i = 0; i < MAX_CIV_PLAYERS; i++)
				{
					if (i != getID() && getTeam() == GET_PLAYER((PlayerTypes)i).getTeam())
					{
						GET_PLAYER((PlayerTypes)i).setTriggerFired(kTriggeredData, false, false);
					}
				}
			}
		}
	}

	if (!CvString(kTrigger.getPythonCallback()).empty())
	{
		long lResult;

		CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(&kTriggeredData));

		gDLL->getPythonIFace()->callFunction(PYRandomEventModule, kTrigger.getPythonCallback(), argsList.makeFunctionArgs(), &lResult);
	}

	if (bAnnounce)
	{
		CvPlot* pPlot = GC.getMap().plot(kTriggeredData.m_iPlotX, kTriggeredData.m_iPlotY);

		if (!kTriggeredData.m_szGlobalText.empty())
		{
			for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
			{
				CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);

				if (kLoopPlayer.isAlive())
				{
					if (GET_TEAM(kLoopPlayer.getTeam()).isHasMet(getTeam()) && (NO_PLAYER == kTriggeredData.m_eOtherPlayer || GET_TEAM(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam()).isHasMet(getTeam())))
					{
						bool bShowPlot = kTrigger.isShowPlot();

						if (bShowPlot)
						{
							if (kLoopPlayer.getTeam() != getTeam())
							{
								if (NULL == pPlot || !pPlot->isRevealed(kLoopPlayer.getTeam(), false))
								{
									bShowPlot = false;
								}
							}
						}

						if (bShowPlot)
						{
							gDLL->getInterfaceIFace()->addHumanMessage((PlayerTypes)iPlayer, false, GC.getEVENT_MESSAGE_TIME(), kTriggeredData.m_szGlobalText, "AS2D_CIVIC_ADOPT", MESSAGE_TYPE_MINOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"), kTriggeredData.m_iPlotX, kTriggeredData.m_iPlotY, true, true);
						}
						else
						{
							gDLL->getInterfaceIFace()->addHumanMessage((PlayerTypes)iPlayer, false, GC.getEVENT_MESSAGE_TIME(), kTriggeredData.m_szGlobalText, "AS2D_CIVIC_ADOPT", MESSAGE_TYPE_MINOR_EVENT);
						}
					}
				}
			}
			// advc.106g: Don't show (most) random events in replays
			//GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), kTriggeredData.m_szGlobalText, kTriggeredData.m_iPlotX, kTriggeredData.m_iPlotY, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
		}
		else if (!kTriggeredData.m_szText.empty())
		{
			if (kTrigger.isShowPlot() && NULL != pPlot && pPlot->isRevealed(getTeam(), false))
			{
				gDLL->getInterfaceIFace()->addHumanMessage(getID(), false, GC.getEVENT_MESSAGE_TIME(), kTriggeredData.m_szText, "AS2D_CIVIC_ADOPT", MESSAGE_TYPE_MINOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"), kTriggeredData.m_iPlotX, kTriggeredData.m_iPlotY, true, true);
			}
			else
			{
				gDLL->getInterfaceIFace()->addHumanMessage(getID(), false, GC.getEVENT_MESSAGE_TIME(), kTriggeredData.m_szText, "AS2D_CIVIC_ADOPT", MESSAGE_TYPE_MINOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"));
			}
		}
	}
}

EventTriggeredData* CvPlayer::initTriggeredData(EventTriggerTypes eEventTrigger, bool bFire, int iCityId, int iPlotX, int iPlotY, PlayerTypes eOtherPlayer, int iOtherPlayerCityId, ReligionTypes eReligion, CorporationTypes eCorporation, int iUnitId, BuildingTypes eBuilding)
{

	CvEventTriggerInfo& kTrigger = GC.getEventTriggerInfo(eEventTrigger);

	CvCity* pCity = getCity(iCityId);
	CvCity* pOtherPlayerCity = NULL;
	if (NO_PLAYER != eOtherPlayer)
	{
		pOtherPlayerCity = GET_PLAYER(eOtherPlayer).getCity(iOtherPlayerCityId);
	}
	CvPlot* pPlot = GC.getMap().plot(iPlotX, iPlotY);
	CvUnit* pUnit = getUnit(iUnitId);

	std::vector<CvPlot*> apPlots;
	bool bPickPlot = ::isPlotEventTrigger(eEventTrigger);
	if (kTrigger.isPickCity())
	{
		if (NULL == pCity)
		{
			pCity = pickTriggerCity(eEventTrigger);
		}

		if (NULL != pCity)
		{
			if (bPickPlot)
			{
				for (int iPlot = 0; iPlot < NUM_CITY_PLOTS; ++iPlot)
				{
					if (CITY_HOME_PLOT != iPlot)
					{
						CvPlot* pLoopPlot = pCity->getCityIndexPlot(iPlot);

						if (NULL != pLoopPlot)
						{
							if (pLoopPlot->canTrigger(eEventTrigger, getID()))
							{
								apPlots.push_back(pLoopPlot);
							}
						}
					}
				}
			}
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		if (kTrigger.getNumBuildings() > 0 && kTrigger.getNumBuildingsRequired() > 0)
		{
			int iFoundValid = 0;

			for (int i = 0; i < kTrigger.getNumBuildingsRequired(); ++i)
			{
				if (kTrigger.getBuildingRequired(i) != NO_BUILDINGCLASS)
				{
					iFoundValid += getBuildingClassCount((BuildingClassTypes)kTrigger.getBuildingRequired(i));
				}
			}

			if (iFoundValid < kTrigger.getNumBuildings())
			{
				return NULL;
			}
		}

		if (kTrigger.getNumReligions() > 0)
		{
			int iFoundValid = 0;

			if (kTrigger.getNumReligionsRequired() > 0)
			{
				for (int i = 0; i < kTrigger.getNumReligionsRequired(); ++i)
				{
					if (kTrigger.getReligionRequired(i) != NO_RELIGION)
					{
						if (getHasReligionCount((ReligionTypes)kTrigger.getReligionRequired(i)) > 0)
						{
							++iFoundValid;
						}
					}
				}
			}
			else
			{
				for (int i = 0; i < GC.getNumReligionInfos(); ++i)
				{
					if (getHasReligionCount((ReligionTypes)i) > 0)
					{
						++iFoundValid;
					}
				}
			}

			if (iFoundValid < kTrigger.getNumReligions())
			{
				return NULL;
			}
		}

		if (kTrigger.getNumCorporations() > 0)
		{
			int iFoundValid = 0;

			if (kTrigger.getNumCorporationsRequired() > 0)
			{
				for (int i = 0; i < kTrigger.getNumCorporationsRequired(); ++i)
				{
					if (kTrigger.getCorporationRequired(i) != NO_CORPORATION)
					{
						if (getHasCorporationCount((CorporationTypes)kTrigger.getCorporationRequired(i)) > 0)
						{
							++iFoundValid;
						}
					}
				}
			}
			else
			{
				for (int i = 0; i < GC.getNumCorporationInfos(); ++i)
				{
					if (getHasCorporationCount((CorporationTypes)i) > 0)
					{
						++iFoundValid;
					}
				}
			}

			if (iFoundValid < kTrigger.getNumCorporations())
			{
				return NULL;
			}
		}

		if (kTrigger.getMinPopulation() > 0)
		{
			if (getTotalPopulation() < kTrigger.getMinPopulation())
			{
				return NULL;
			}
		}

		if (kTrigger.getMaxPopulation() > 0)
		{
			if (getTotalPopulation() > kTrigger.getMaxPopulation())
			{
				return NULL;
			}
		}

		if (bPickPlot)
		{
			for (int iPlot = 0; iPlot < GC.getMap().numPlots(); ++iPlot)
			{
				CvPlot* pLoopPlot = GC.getMap().plotByIndex(iPlot);

				if (pLoopPlot->canTrigger(eEventTrigger, getID()))
				{
					apPlots.push_back(pLoopPlot);
				}
			}
		}
	}

	if (kTrigger.isPickReligion())
	{
		if (NO_RELIGION == eReligion)
		{
			if (kTrigger.isStateReligion())
			{
				ReligionTypes eStateReligion = getStateReligion();
				if (NO_RELIGION != eStateReligion && isValidTriggerReligion(kTrigger, pCity, eStateReligion))
				{
					eReligion = getStateReligion();
				}
			}
			else
			{
				int iOffset = GC.getGame().getSorenRandNum(GC.getNumReligionInfos(), "Event pick religion");

				for (int i = 0; i < GC.getNumReligionInfos(); ++i)
				{
					int iReligion = (i + iOffset) % GC.getNumReligionInfos();

					if (isValidTriggerReligion(kTrigger, pCity, (ReligionTypes)iReligion))
					{
						eReligion = (ReligionTypes)iReligion;
						break;
					}
				}
			}
		}

		if (NO_RELIGION == eReligion)
		{
			return NULL;
		}
	}

	if (kTrigger.isPickCorporation())
	{
		if (NO_CORPORATION == eCorporation)
		{
			int iOffset = GC.getGame().getSorenRandNum(GC.getNumCorporationInfos(), "Event pick corporation");

			for (int i = 0; i < GC.getNumCorporationInfos(); ++i)
			{
				int iCorporation = (i + iOffset) % GC.getNumCorporationInfos();

				if (isValidTriggerCorporation(kTrigger, pCity, (CorporationTypes)iCorporation))
				{
					eCorporation = (CorporationTypes)iCorporation;
					break;
				}
			}
		}

		if (NO_CORPORATION == eCorporation)
		{
			return NULL;
		}
	}

	if (NULL == pPlot)
	{
		if (apPlots.size() > 0)
		{
			int iChosen = GC.getGame().getSorenRandNum(apPlots.size(), "Event pick plot");
			pPlot = apPlots[iChosen];

			if (NULL == pCity)
			{
				pCity = GC.getMap().findCity(pPlot->getX(), pPlot->getY(), getID(), NO_TEAM, false);
			}
		}
		else
		{
			if (bPickPlot)
			{
				return NULL;
			}

			if (NULL != pCity)
			{
				pPlot = pCity->plot();
			}
		}
	}

	if (kTrigger.getNumBuildings() > 0)
	{
		if (NULL != pCity && NO_BUILDING == eBuilding)
		{
			std::vector<BuildingTypes> aeBuildings;
			for (int i = 0; i < kTrigger.getNumBuildingsRequired(); ++i)
			{
				if (kTrigger.getBuildingRequired(i) != NO_BUILDINGCLASS)
				{
					BuildingTypes eTestBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(kTrigger.getBuildingRequired(i));
					if (NO_BUILDING != eTestBuilding && pCity->getNumRealBuilding(eTestBuilding) > 0)
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
			else
			{
				return NULL;
			}
		}
	}

	if (NULL == pUnit)
	{
		pUnit = pickTriggerUnit(eEventTrigger, pPlot, bPickPlot);
	}

	if (NULL == pUnit && kTrigger.getNumUnits() > 0)
	{
		return NULL;
	}

	if (NULL == pPlot && NULL != pUnit)
	{
		pPlot = pUnit->plot();
	}

	if (NULL == pPlot && bPickPlot)
	{
		return NULL;
	}

	if (kTrigger.getNumUnitsGlobal() > 0)
	{
		int iNumUnits = 0;
		for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
		{
			CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);

			if (kLoopPlayer.isAlive())
			{
				int iLoop;
				for (CvUnit* pLoopUnit = kLoopPlayer.firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = kLoopPlayer.nextUnit(&iLoop))
				{
					if (MIN_INT != pLoopUnit->getTriggerValue(eEventTrigger, pPlot, true))
					{
						++iNumUnits;
					}
				}
			}
		}

		if (iNumUnits < kTrigger.getNumUnitsGlobal())
		{
			return NULL;
		}
	}

	if (kTrigger.getNumBuildingsGlobal() > 0)
	{
		int iNumBuildings = 0;
		for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
		{
			CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);

			if (kLoopPlayer.isAlive())
			{
				for (int i = 0; i < kTrigger.getNumBuildingsRequired(); ++i)
				{
					if (kTrigger.getBuildingRequired(i) != NO_BUILDINGCLASS)
					{
						iNumBuildings += getBuildingClassCount((BuildingClassTypes)kTrigger.getBuildingRequired(i));
					}
				}
			}
		}

		if (iNumBuildings < kTrigger.getNumBuildingsGlobal())
		{
			return NULL;
		}
	}

	if (kTrigger.isPickPlayer())
	{
		std::vector<PlayerTypes> aePlayers;
		std::vector<CvCity*> apCities;

		if (NO_PLAYER == eOtherPlayer)
		{
			for (int i = 0; i < MAX_CIV_PLAYERS; i++)
			{	// advc.001: (from Leoreth's RFC:DoC mod)
				if (!GET_PLAYER((PlayerTypes)i).isMinorCiv() &&
						GET_PLAYER((PlayerTypes)i).canTrigger(eEventTrigger, getID(), eReligion))
				{
					if (kTrigger.isPickOtherPlayerCity())
					{
						CvCity* pBestCity = NULL;

						if (NULL != pCity)
						{
							pBestCity = GC.getMap().findCity(pCity->getX(), pCity->getY(), (PlayerTypes)i);
						}
						else
						{
							pBestCity = GET_PLAYER((PlayerTypes)i).pickTriggerCity(eEventTrigger);
						}

						if (NULL != pBestCity)
						{
							apCities.push_back(pBestCity);
							aePlayers.push_back((PlayerTypes)i);
						}
					}
					else
					{
						apCities.push_back(NULL);
						aePlayers.push_back((PlayerTypes)i);
					}
				}
			}

			if (aePlayers.size() > 0)
			{
				int iChosen = GC.getGame().getSorenRandNum(aePlayers.size(), "Event pick player");
				eOtherPlayer = aePlayers[iChosen];
				pOtherPlayerCity = apCities[iChosen];
			}
			else
			{
				return NULL;
			}
		}
	}

	EventTriggeredData* pTriggerData = addEventTriggered();

	if (NULL != pTriggerData)
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
	else
	{
		return NULL;
	}

	if (!CvString(kTrigger.getPythonCanDo()).empty())
	{
		long lResult;

		CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pTriggerData));

		gDLL->getPythonIFace()->callFunction(PYRandomEventModule, kTrigger.getPythonCanDo(), argsList.makeFunctionArgs(), &lResult);

		if (lResult == 0)
		{
			deleteEventTriggered(pTriggerData->getID());
			return NULL;
		}

		// python may change pTriggerData
		pCity = getCity(pTriggerData->m_iCityId);
		pPlot = GC.getMap().plot(pTriggerData->m_iPlotX, pTriggerData->m_iPlotY);
		pUnit = getUnit(pTriggerData->m_iUnitId);
		eOtherPlayer = pTriggerData->m_eOtherPlayer;
		if (NO_PLAYER != eOtherPlayer)
		{
			pOtherPlayerCity = GET_PLAYER(eOtherPlayer).getCity(pTriggerData->m_iOtherPlayerCityId);
		}
		eReligion = pTriggerData->m_eReligion;
		eCorporation = pTriggerData->m_eCorporation;
		eBuilding = pTriggerData->m_eBuilding;
	}

	std::vector<CvWString> aszTexts;
	for (int i = 0; i < kTrigger.getNumTexts(); ++i)
	{
		if (NO_ERA == kTrigger.getTextEra(i) || kTrigger.getTextEra(i) == getCurrentEra())
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
			NO_RELIGION != eReligion ? GC.getReligionInfo(eReligion).getAdjectiveKey() : L"",
			NO_BUILDING != eBuilding ? GC.getBuildingInfo(eBuilding).getTextKeyWide() : L"",
			NULL != pOtherPlayerCity ? pOtherPlayerCity->getNameKey() : L"",
			NULL != pPlot && NO_TERRAIN != pPlot->getTerrainType() ? GC.getTerrainInfo(pPlot->getTerrainType()).getTextKeyWide() : L"",
			NULL != pPlot && NO_IMPROVEMENT != pPlot->getImprovementType() ? GC.getImprovementInfo(pPlot->getImprovementType()).getTextKeyWide() : L"",
			NULL != pPlot && NO_BONUS != pPlot->getBonusType() ? GC.getBonusInfo(pPlot->getBonusType()).getTextKeyWide() : L"",
			NULL != pPlot && NO_ROUTE != pPlot->getRouteType() ? GC.getRouteInfo(pPlot->getRouteType()).getTextKeyWide() : L"",
			NO_CORPORATION != eCorporation ? GC.getCorporationInfo(eCorporation).getTextKeyWide() : L""
			);

	}
	else
	{
		pTriggerData->m_szText = L"";
	}

	if (kTrigger.getNumWorldNews() > 0)
	{
		int iText = GC.getGame().getSorenRandNum(kTrigger.getNumWorldNews(), "Trigger World News choice");

		pTriggerData->m_szGlobalText = gDLL->getText(kTrigger.getWorldNews(iText).GetCString(),
			getCivilizationAdjectiveKey(),
			NULL != pCity ? pCity->getNameKey() : L"",
			pTriggerData->m_eReligion != NO_RELIGION ? GC.getReligionInfo(pTriggerData->m_eReligion).getAdjectiveKey() : L"",
			eOtherPlayer != NO_PLAYER ? GET_PLAYER(eOtherPlayer).getCivilizationAdjectiveKey() : L"",
			NULL != pOtherPlayerCity ? pOtherPlayerCity->getNameKey() : L"",
			pTriggerData->m_eCorporation != NO_CORPORATION ? GC.getCorporationInfo(pTriggerData->m_eCorporation).getTextKeyWide() : L""
			);
	}
	else
	{
		pTriggerData->m_szGlobalText.clear();
	}

	if (bFire)
	{
		trigger(*pTriggerData);
	}

	return pTriggerData;
}


bool CvPlayer::canDoEvent(EventTypes eEvent, const EventTriggeredData& kTriggeredData) const
{
	if (eEvent == NO_EVENT)
	{
		FAssert(false);
		return false;
	}

	CvEventInfo& kEvent = GC.getEventInfo(eEvent);

	int iGold = std::min(getEventCost(eEvent, kTriggeredData.m_eOtherPlayer, false), getEventCost(eEvent, kTriggeredData.m_eOtherPlayer, true));

	if (iGold != 0)
	{
		if (iGold > 0 && NO_PLAYER != kTriggeredData.m_eOtherPlayer && kEvent.isGoldToPlayer())
		{
			if (GET_PLAYER(kTriggeredData.m_eOtherPlayer).getGold() < iGold)
			{
				return false;
			}
		}
		else if (iGold < 0)
		{
			if (getGold() < -iGold)
			{
				return false;
			}
		}
	}

	if (kEvent.getSpaceProductionModifier() != 0)
	{
		bool bValid = false;
		for (int iProject = 0; iProject < GC.getNumProjectInfos(); ++iProject)
		{
			CvProjectInfo& kProject = GC.getProjectInfo((ProjectTypes)iProject);
			if (kProject.isSpaceship())
			{
				if (kProject.getVictoryPrereq() != NO_VICTORY)
				{
					if (GC.getGame().isVictoryValid((VictoryTypes)(kProject.getVictoryPrereq())))
					{
						bValid = true;
						break;
					}
				}
			}
		}

		if (!bValid)
		{
			return false;
		}
	}

	if (kEvent.getEspionagePoints() > 0 && GC.getGame().isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		return false;
	}

	if (NO_PLAYER != kTriggeredData.m_eOtherPlayer)
	{
		if (kEvent.getEspionagePoints() + GET_TEAM(getTeam()).getEspionagePointsAgainstTeam(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam()) < 0)
		{
			return false;
		}
	}

	if (kEvent.getTechPercent() != 0 || kEvent.getTechCostPercent() != 0)
	{
		if (NO_TECH == getBestEventTech(eEvent, kTriggeredData.m_eOtherPlayer))
		{
			return false;
		}
	}

	if (NO_TECH != kEvent.getPrereqTech())
	{
		if (!GET_TEAM(getTeam()).isHasTech((TechTypes)kEvent.getPrereqTech()))
		{
			return false;
		}
	}

	if (NO_BONUS != kEvent.getBonusGift())
	{
		BonusTypes eBonus = (BonusTypes)kEvent.getBonusGift();
		if (NO_PLAYER == kTriggeredData.m_eOtherPlayer)
		{
			return false;
		}

		if (!canTradeNetworkWith(kTriggeredData.m_eOtherPlayer))
		{
			return false;
		}

		if (GET_PLAYER(kTriggeredData.m_eOtherPlayer).getNumAvailableBonuses(eBonus) > 0)
		{
			return false;
		}

		if (getNumTradeableBonuses(eBonus) <= 1)
		{
			return false;
		}
	}

	if (kEvent.getUnitClass() != NO_UNITCLASS)
	{
		UnitTypes eUnit = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(kEvent.getUnitClass());
		if (eUnit == NO_UNIT)
		{
			return false;
		}
	}

	if (kEvent.isCityEffect())
	{
		CvCity* pCity =	getCity(kTriggeredData.m_iCityId);
		if (NULL == pCity || !pCity->canApplyEvent(eEvent, kTriggeredData))
		{
			return false;
		}
	}
	else if (kEvent.isOtherPlayerCityEffect())
	{
		if (NO_PLAYER == kTriggeredData.m_eOtherPlayer)
		{
			return false;
		}

		CvCity* pCity = GET_PLAYER(kTriggeredData.m_eOtherPlayer).getCity(kTriggeredData.m_iOtherPlayerCityId);
		if (NULL == pCity || !pCity->canApplyEvent(eEvent, kTriggeredData))
		{
			return false;
		}
	}

	if (::isPlotEventTrigger(kTriggeredData.m_eTrigger))
	{
		CvPlot* pPlot = GC.getMap().plot(kTriggeredData.m_iPlotX, kTriggeredData.m_iPlotY);
		if (NULL != pPlot)
		{
			if (!pPlot->canApplyEvent(eEvent))
			{
				return false;
			}
		}
	}

	CvUnit* pUnit = getUnit(kTriggeredData.m_iUnitId);
	if (NULL != pUnit)
	{
		if (!pUnit->canApplyEvent(eEvent))
		{
			return false;
		}
	}

	if (NO_BONUS != kEvent.getBonusRevealed())
	{
		if (GET_TEAM(getTeam()).isHasTech((TechTypes)GC.getBonusInfo((BonusTypes)kEvent.getBonusRevealed()).getTechReveal()))
		{
			return false;
		}

		if (GET_TEAM(getTeam()).isForceRevealedBonus((BonusTypes)kEvent.getBonusRevealed()))
		{
			return false;
		}
	}

	if (kEvent.getConvertOwnCities() > 0)
	{
		bool bFoundValid = false;

		if (NO_RELIGION != kTriggeredData.m_eReligion)
		{
			int iLoop;
			for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
			{
				if (!pLoopCity->isHasReligion(kTriggeredData.m_eReligion))
				{
					if (-1 == kEvent.getMaxNumReligions() || pLoopCity->getReligionCount() <= kEvent.getMaxNumReligions())
					{
						bFoundValid = true;
						break;
					}
				}
			}
		}

		if (!bFoundValid)
		{
			return false;
		}
	}

	if (kEvent.getConvertOtherCities() > 0)
	{
		bool bFoundValid = false;

		if (NO_RELIGION != kTriggeredData.m_eReligion)
		{
			if (NO_PLAYER != kTriggeredData.m_eOtherPlayer)
			{
				int iLoop;
				for (CvCity* pLoopCity = GET_PLAYER(kTriggeredData.m_eOtherPlayer).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(kTriggeredData.m_eOtherPlayer).nextCity(&iLoop))
				{
					if (!pLoopCity->isHasReligion(kTriggeredData.m_eReligion))
					{
						if (-1 == kEvent.getMaxNumReligions() || pLoopCity->getReligionCount() <= kEvent.getMaxNumReligions())
						{
							bFoundValid = true;
							break;
						}
					}
				}
			}
		}

		if (!bFoundValid)
		{
			return false;
		}
	}

	if (kEvent.getAttitudeModifier() != 0)
	{
		if (NO_PLAYER == kTriggeredData.m_eOtherPlayer)
		{
			return false;
		}

		if (GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam() == getTeam())
		{
			return false;
		}

		if (GET_PLAYER(kTriggeredData.m_eOtherPlayer).isHuman())
		{
			if (kEvent.getOurAttitudeModifier() == 0)
			{
				return false;
			}
		}
	}

	if (kEvent.getTheirEnemyAttitudeModifier() != 0)
	{
		if (NO_PLAYER == kTriggeredData.m_eOtherPlayer)
		{
			return false;
		}

		TeamTypes eWorstEnemy = GET_TEAM(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam()).AI_getWorstEnemy();
		if (NO_TEAM == eWorstEnemy || eWorstEnemy == getTeam())
		{
			return false;
		}

		if (!GET_TEAM(eWorstEnemy).isAlive())
		{
			return false;
		}

		if (eWorstEnemy == getTeam())
		{
			return false;
		}
	}

	if (kEvent.isDeclareWar())
	{
		if (NO_PLAYER == kTriggeredData.m_eOtherPlayer)
		{
			return false;
		}

		if (!GET_TEAM(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam()).canDeclareWar(getTeam()) || !GET_TEAM(getTeam()).canDeclareWar(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam()))
		{
			return false;
		}
	}

	if (kEvent.isQuest())
	{
		for (int iTrigger = 0; iTrigger < GC.getNumEventTriggerInfos(); ++iTrigger)
		{
			CvEventTriggerInfo& kTrigger = GC.getEventTriggerInfo((EventTriggerTypes)iTrigger);
			if (!kTrigger.isRecurring())
			{
				for (int i = 0; i < kTrigger.getNumPrereqEvents(); ++i)
				{
					if (kTrigger.getPrereqEvent(i) == eEvent)
					{
						if (isTriggerFired((EventTriggerTypes)iTrigger))
						{
							return false;
						}
					}
				}
			}
		}
	}

	if (!CvString(kEvent.getPythonCanDo()).empty())
	{
		long lResult; CyArgsList argsList;
		argsList.add(eEvent);
		argsList.add(gDLL->getPythonIFace()->makePythonObject(&kTriggeredData));
		gDLL->getPythonIFace()->callFunction(PYRandomEventModule, kEvent.getPythonCanDo(), argsList.makeFunctionArgs(), &lResult);
		if (lResult == 0)
			return false;
	}

	return true;
}


void CvPlayer::applyEvent(EventTypes eEvent, int iEventTriggeredId, bool bUpdateTrigger)
{
	FAssert(eEvent != NO_EVENT);

	EventTriggeredData* pTriggeredData = getEventTriggered(iEventTriggeredId);

	if (NULL == pTriggeredData)
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
		{
			deleteEventTriggered(iEventTriggeredId);
		}
		return;
	}

	setEventOccured(eEvent, *pTriggeredData);

	CvEventInfo& kEvent = GC.getEventInfo(eEvent);
	CvCity* pCity =	getCity(pTriggeredData->m_iCityId);
	CvCity* pOtherPlayerCity = NULL;

	if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
	{
		pOtherPlayerCity = GET_PLAYER(pTriggeredData->m_eOtherPlayer).getCity(pTriggeredData->m_iOtherPlayerCityId);
	}

	int iGold = getEventCost(eEvent, pTriggeredData->m_eOtherPlayer, false);
	int iRandomGold = getEventCost(eEvent, pTriggeredData->m_eOtherPlayer, true);

	iGold += GC.getGame().getSorenRandNum(iRandomGold - iGold + 1, "Event random gold");

	if (iGold != 0)
	{
		changeGold(iGold);

		if (NO_PLAYER != pTriggeredData->m_eOtherPlayer && kEvent.isGoldToPlayer())
		{
			GET_PLAYER(pTriggeredData->m_eOtherPlayer).changeGold(-iGold);
		}
	}

	if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
	{
		if (kEvent.getEspionagePoints() != 0)
		{
			GET_TEAM(getTeam()).changeEspionagePointsAgainstTeam(GET_PLAYER(pTriggeredData->m_eOtherPlayer).getTeam(), kEvent.getEspionagePoints());
		}
	}

	if (kEvent.getTechPercent() != 0)
	{
		TechTypes eBestTech = getBestEventTech(eEvent, pTriggeredData->m_eOtherPlayer);

		if (eBestTech != NO_TECH)
		{
			int iBeakers  = GET_TEAM(getTeam()).changeResearchProgressPercent(eBestTech, kEvent.getTechPercent(), getID());

			if (iBeakers > 0)
			{
				for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
				{
					if (GET_PLAYER((PlayerTypes)iI).isAlive())
					{
						if (GET_PLAYER((PlayerTypes)iI).getTeam()
								== getTeam()) // kmodx: was getID()
						{
							CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_PROGRESS_TOWARDS_TECH", iBeakers, GC.getTechInfo(eBestTech).getTextKeyWide());

							gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, NULL, MESSAGE_TYPE_MINOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_TECH_TEXT"));
						}
					}
				}
			}
		}
	}

	if (kEvent.isGoldenAge())
	{
		changeGoldenAgeTurns(getGoldenAgeLength());
	}

	if (kEvent.getInflationModifier() != 0)
	{
		m_iInflationModifier += kEvent.getInflationModifier();
	}

	if (kEvent.getSpaceProductionModifier() != 0)
	{
		changeSpaceProductionModifier(kEvent.getSpaceProductionModifier());
	}

	if (kEvent.getFreeUnitSupport() != 0)
	{
		changeBaseFreeUnits(kEvent.getFreeUnitSupport());
	}

	if (kEvent.isDeclareWar())
	{
		if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
		{
			if (gTeamLogLevel >= 2) // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
				logBBAI("    Team %d (%S) declares war on team %d due to event", GET_PLAYER(pTriggeredData->m_eOtherPlayer).getTeam(), GET_PLAYER(pTriggeredData->m_eOtherPlayer).getCivilizationDescription(0), getTeam());
			GET_TEAM(GET_PLAYER(pTriggeredData->m_eOtherPlayer).getTeam()).declareWar(getTeam(), false, WARPLAN_LIMITED,
					true, NO_PLAYER, true); // advc.106g
		}
	}

	if (NO_BONUS != kEvent.getBonusGift())
	{
		if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
		{
			CLinkList<TradeData> ourList;
			CLinkList<TradeData> theirList;
			TradeData kTradeData;
			setTradeItem(&kTradeData, TRADE_RESOURCES, kEvent.getBonusGift());
			ourList.insertAtEnd(kTradeData);
			GC.getGame().implementDeal(getID(), pTriggeredData->m_eOtherPlayer, &ourList, &theirList);
		}
	}

	bool bClear = false;
	for (int iEvent = 0; iEvent < GC.getNumEventInfos(); ++iEvent)
	{
		if (kEvent.getClearEventChance(iEvent) > 0)
		{
			bClear = GC.getGame().getSorenRandNum(100, "Event Clear") < kEvent.getClearEventChance(iEvent);
			if (bClear)
			{
				if (kEvent.isGlobal())
				{
					for (int j = 0; j < MAX_CIV_PLAYERS; j++)
					{
						GET_PLAYER((PlayerTypes)j).resetEventOccured((EventTypes)iEvent, j != getID());
					}
				}
				else if (kEvent.isTeam())
				{
					for (int j = 0; j < MAX_CIV_PLAYERS; j++)
					{
						if (getTeam() == GET_PLAYER((PlayerTypes)j).getTeam())
						{
							GET_PLAYER((PlayerTypes)j).resetEventOccured((EventTypes)iEvent, j != getID());
						}
					}
				}
				else
				{
					resetEventOccured((EventTypes)iEvent, false);
				}
			}
		}
	}

	if (NULL != pCity && kEvent.isCityEffect())
	{
		pCity->applyEvent(eEvent, *pTriggeredData, bClear);
	}
	else if (NULL != pOtherPlayerCity && kEvent.isOtherPlayerCityEffect())
	{
		pOtherPlayerCity->applyEvent(eEvent, *pTriggeredData, bClear);
	}

	if (!kEvent.isCityEffect() && !kEvent.isOtherPlayerCityEffect())
	{
		if (kEvent.getHappy() != 0)
		{
			changeExtraHappiness(kEvent.getHappy());
		}

		if (kEvent.getHealth() != 0)
		{
			changeExtraHealth(kEvent.getHealth());
		}

		if (kEvent.getNumBuildingYieldChanges() > 0)
		{
			int iLoop;
			for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
			{
				for (int iYield = 0; iYield < NUM_YIELD_TYPES; ++iYield)
				{
					for (CvCity* pLoopCity = firstCity(&iLoop); NULL != pLoopCity; pLoopCity = nextCity(&iLoop))
					{
						pLoopCity->changeBuildingYieldChange((BuildingClassTypes)iBuildingClass, (YieldTypes)iYield, kEvent.getBuildingYieldChange(iBuildingClass, iYield));
					}
				}
			}
		}

		if (kEvent.getNumBuildingCommerceChanges() > 0)
		{
			int iLoop;
			for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
			{
				for (int iCommerce = 0; iCommerce < NUM_COMMERCE_TYPES; ++iCommerce)
				{
					for (CvCity* pLoopCity = firstCity(&iLoop); NULL != pLoopCity; pLoopCity = nextCity(&iLoop))
					{
						pLoopCity->changeBuildingCommerceChange((BuildingClassTypes)iBuildingClass, (CommerceTypes)iCommerce, kEvent.getBuildingCommerceChange(iBuildingClass, iCommerce));
					}
				}
			}
		}

		if (kEvent.getNumBuildingHappyChanges() > 0)
		{
			for (int i = 0; i < GC.getNumBuildingClassInfos(); ++i)
			{
				if (kEvent.getBuildingHappyChange(i) != 0)
				{
					BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(i);
					if (NO_BUILDING != eBuilding)
					{
						changeExtraBuildingHappiness(eBuilding, kEvent.getBuildingHappyChange(i));
					}
				}
			}
		}

		if (kEvent.getNumBuildingHealthChanges() > 0)
		{
			for (int i = 0; i < GC.getNumBuildingClassInfos(); ++i)
			{
				if (kEvent.getBuildingHealthChange(i) != 0)
				{
					BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(i);
					if (NO_BUILDING != eBuilding)
					{
						changeExtraBuildingHealth(eBuilding, kEvent.getBuildingHealthChange(i));
					}
				}
			}
		}

		if (kEvent.getHurryAnger() != 0)
		{
			int iLoop;

			for (CvCity* pLoopCity = firstCity(&iLoop); NULL != pLoopCity; pLoopCity = nextCity(&iLoop))
			{
				pLoopCity->changeHurryAngerTimer(kEvent.getHurryAnger() * pLoopCity->flatHurryAngerLength());
			}
		}

		if (kEvent.getHappyTurns() > 0)
		{
			int iLoop;

			for (CvCity* pLoopCity = firstCity(&iLoop); NULL != pLoopCity; pLoopCity = nextCity(&iLoop))
			{
				pLoopCity->changeHappinessTimer(kEvent.getHappyTurns());
			}
		}

		if (kEvent.getMaxPillage() > 0)
		{
			FAssert(kEvent.getMaxPillage() >= kEvent.getMinPillage());
			int iNumPillage = kEvent.getMinPillage() + GC.getGame().getSorenRandNum(kEvent.getMaxPillage() - kEvent.getMinPillage(), "Pick number of event pillaged plots");

			int iNumPillaged = 0;
			for (int i = 0; i < iNumPillage; ++i)
			{
				int iRandOffset = GC.getGame().getSorenRandNum(GC.getMap().numPlots(), "Pick event pillage plot (any city)");
				for (int j = 0; j < GC.getMap().numPlots(); ++j)
				{
					int iPlot = (j + iRandOffset) % GC.getMap().numPlots();
					CvPlot* pPlot = GC.getMap().plotByIndex(iPlot);
					if (NULL != pPlot && pPlot->getOwner() == getID() && pPlot->isCity())
					{
						if (NO_IMPROVEMENT != pPlot->getImprovementType() && !GC.getImprovementInfo(pPlot->getImprovementType()).isPermanent())
						{
							CvWString szBuffer = gDLL->getText("TXT_KEY_EVENT_CITY_IMPROVEMENT_DESTROYED", GC.getImprovementInfo(pPlot->getImprovementType()).getTextKeyWide());
							gDLL->getInterfaceIFace()->addHumanMessage(getID(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_PILLAGED", MESSAGE_TYPE_INFO, GC.getImprovementInfo(pPlot->getImprovementType()).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX(), pPlot->getY(), true, true);
							pPlot->setImprovementType(NO_IMPROVEMENT);
							++iNumPillaged;
							break;
						}
					}
				}
			}

			if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
			{
				CvWString szBuffer = gDLL->getText("TXT_KEY_EVENT_NUM_CITY_IMPROVEMENTS_DESTROYED", iNumPillaged, getCivilizationAdjectiveKey());
				gDLL->getInterfaceIFace()->addHumanMessage(pTriggeredData->m_eOtherPlayer, false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_PILLAGED", MESSAGE_TYPE_INFO);
			}
		}

		if (kEvent.getFood() != 0)
		{
			int iLoop;

			for (CvCity* pLoopCity = firstCity(&iLoop); NULL != pLoopCity; pLoopCity = nextCity(&iLoop))
			{
				pLoopCity->changeFood(kEvent.getFood());
			}
		}

		if (kEvent.getFoodPercent() != 0)
		{
			int iLoop;

			for (CvCity* pLoopCity = firstCity(&iLoop); NULL != pLoopCity; pLoopCity = nextCity(&iLoop))
			{
				pLoopCity->changeFood((pLoopCity->getFood() * kEvent.getFoodPercent()) / 100);
			}
		}

		if (kEvent.getPopulationChange() != 0)
		{
			int iLoop;

			for (CvCity* pLoopCity = firstCity(&iLoop); NULL != pLoopCity; pLoopCity = nextCity(&iLoop))
			{
				if (pLoopCity->getPopulation() + kEvent.getPopulationChange() > 0)
				{
					pLoopCity->changePopulation(kEvent.getPopulationChange());
				}
			}
		}

		if (kEvent.getCulture() != 0)
		{
			int iLoop;

			for (CvCity* pLoopCity = firstCity(&iLoop); NULL != pLoopCity; pLoopCity = nextCity(&iLoop))
			{
				if (pLoopCity->getCultureTimes100(pLoopCity->getOwner()) + 100 * kEvent.getCulture() > 0)
				{
					pLoopCity->changeCulture(pLoopCity->getOwner(), kEvent.getCulture(), true, true);
				}
			}
		}

		if (kEvent.getUnitClass() != NO_UNITCLASS)
		{
			UnitTypes eUnit = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(kEvent.getUnitClass());
			if (eUnit != NO_UNIT)
			{
				CvCity* pUnitCity = pCity;

				if (NULL == pUnitCity)
				{
					pUnitCity = getCapitalCity();
				}

				if (NULL != pUnitCity)
				{
					for (int i = 0; i < kEvent.getNumUnits(); ++i)
					{
						initUnit(eUnit, pUnitCity->getX(), pUnitCity->getY());
					}
				}
			}
		}
	}

	CvPlot* pPlot = GC.getMap().plot(pTriggeredData->m_iPlotX, pTriggeredData->m_iPlotY);
	if (NULL != pPlot)
	{
		if (::isPlotEventTrigger(pTriggeredData->m_eTrigger))
		{
			FAssert(pPlot->canApplyEvent(eEvent));
			pPlot->applyEvent(eEvent);
		}
	}

	CvUnit* pUnit = getUnit(pTriggeredData->m_iUnitId);
	if (NULL != pUnit)
	{
		FAssert(pUnit->canApplyEvent(eEvent));
		pUnit->applyEvent(eEvent);   // might kill the unit
	}

	for (int i = 0; i < GC.getNumUnitCombatInfos(); ++i)
	{
		if (NO_PROMOTION != kEvent.getUnitCombatPromotion(i))
		{
			int iLoop;
			for (CvUnit* pLoopUnit = firstUnit(&iLoop); NULL != pLoopUnit; pLoopUnit = nextUnit(&iLoop))
			{
				if (pLoopUnit->getUnitCombatType() == i)
				{
					pLoopUnit->setHasPromotion((PromotionTypes)kEvent.getUnitCombatPromotion(i), true);
				}
			}

			setFreePromotion((UnitCombatTypes)i, (PromotionTypes)kEvent.getUnitCombatPromotion(i), true);
		}
	}

	for (int i = 0; i < GC.getNumUnitClassInfos(); ++i)
	{
		if (NO_PROMOTION != kEvent.getUnitClassPromotion(i))
		{
			int iLoop;
			for (CvUnit* pLoopUnit = firstUnit(&iLoop); NULL != pLoopUnit; pLoopUnit = nextUnit(&iLoop))
			{
				if (pLoopUnit->getUnitClassType() == i)
				{
					pLoopUnit->setHasPromotion((PromotionTypes)kEvent.getUnitClassPromotion(i), true);
				}
			}

			setFreePromotion((UnitClassTypes)i, (PromotionTypes)kEvent.getUnitClassPromotion(i), true);
		}
	}

	if (NO_BONUS != kEvent.getBonusRevealed())
	{
		GET_TEAM(getTeam()).setForceRevealedBonus((BonusTypes)kEvent.getBonusRevealed(), true);
	}

	std::vector<CvCity*> apSpreadReligionCities;

	if (kEvent.getConvertOwnCities() > 0)
	{
		if (NO_RELIGION != pTriggeredData->m_eReligion)
		{
			int iLoop;
			for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
			{
				if (!pLoopCity->isHasReligion(pTriggeredData->m_eReligion))
				{
					if (-1 == kEvent.getMaxNumReligions() || pLoopCity->getReligionCount() <= kEvent.getMaxNumReligions())
					{
						apSpreadReligionCities.push_back(pLoopCity);
					}
				}
			}
		}
	}

	while ((int)apSpreadReligionCities.size() > kEvent.getConvertOwnCities())
	{
		int iChosen = GC.getGame().getSorenRandNum(apSpreadReligionCities.size(), "Even Spread Religion (own)");

		int i = 0;
		for (std::vector<CvCity*>::iterator it = apSpreadReligionCities.begin(); it != apSpreadReligionCities.end(); ++it)
		{
			if (i == iChosen)
			{
				apSpreadReligionCities.erase(it);
				break;
			}
			++i;
		}
	}

	for (std::vector<CvCity*>::iterator it = apSpreadReligionCities.begin(); it != apSpreadReligionCities.end(); ++it)
	{
		(*it)->setHasReligion(pTriggeredData->m_eReligion, true, true, false);
	}

	apSpreadReligionCities.clear();

	if (kEvent.getConvertOtherCities() > 0)
	{
		if (NO_RELIGION != pTriggeredData->m_eReligion)
		{
			if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
			{
				// modx: removed redundant code
				int iLoop;
				for (CvCity* pLoopCity = GET_PLAYER(pTriggeredData->m_eOtherPlayer).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(pTriggeredData->m_eOtherPlayer).nextCity(&iLoop))
				{
					if (!pLoopCity->isHasReligion(pTriggeredData->m_eReligion))
					{
						if (-1 == kEvent.getMaxNumReligions() || pLoopCity->getReligionCount() <= kEvent.getMaxNumReligions())
						{
							apSpreadReligionCities.push_back(pLoopCity);
						}
					}
				}
			}
		}
	}

	while ((int)apSpreadReligionCities.size() > kEvent.getConvertOtherCities())
	{
		int iChosen = GC.getGame().getSorenRandNum(apSpreadReligionCities.size(), "Even Spread Religion (other)");

		int i = 0;
		for (std::vector<CvCity*>::iterator it = apSpreadReligionCities.begin(); it != apSpreadReligionCities.end(); ++it)
		{
			if (i == iChosen)
			{
				apSpreadReligionCities.erase(it);
				break;
			}
			++i;
		}
	}

	for (std::vector<CvCity*>::iterator it = apSpreadReligionCities.begin(); it != apSpreadReligionCities.end(); ++it)
	{
		(*it)->setHasReligion(pTriggeredData->m_eReligion, true, true, false);
	}

	if (kEvent.getOurAttitudeModifier() != 0)
	{
		if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
		{
			if (kEvent.getOurAttitudeModifier() > 0)
			{
				AI_changeMemoryCount(pTriggeredData->m_eOtherPlayer, MEMORY_EVENT_GOOD_TO_US, kEvent.getOurAttitudeModifier());
			}
			else
			{
				AI_changeMemoryCount(pTriggeredData->m_eOtherPlayer, MEMORY_EVENT_BAD_TO_US, -kEvent.getOurAttitudeModifier());
			}
		}
	}

	if (kEvent.getAttitudeModifier() != 0)
	{
		if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
		{	// <advc.130j> Replace AI_changeMemoryCount with multiple AI_rememberEvent calls */
			CvPlayerAI& kOther = GET_PLAYER(pTriggeredData->m_eOtherPlayer);
			if(kEvent.getAttitudeModifier() > 0) {
				for(int i = 0; i < kEvent.getAttitudeModifier(); i++)
					kOther.AI_rememberEvent(getID(), MEMORY_EVENT_GOOD_TO_US);
			}
			else {
				for(int i = 0; i < -kEvent.getAttitudeModifier(); i++)
					kOther.AI_rememberEvent(getID(), MEMORY_EVENT_BAD_TO_US);
			} // </advc.130j>
		}
	}

	if (kEvent.getTheirEnemyAttitudeModifier() != 0)
	{
		if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
		{
			TeamTypes eWorstEnemy = GET_TEAM(GET_PLAYER(pTriggeredData->m_eOtherPlayer).getTeam()).AI_getWorstEnemy();
			if (NO_TEAM != eWorstEnemy)
			{
				for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
				{
					CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
					if (kLoopPlayer.isAlive() && kLoopPlayer.getTeam() == eWorstEnemy)
					{
						if (kEvent.getTheirEnemyAttitudeModifier() > 0)
						{
							kLoopPlayer.AI_changeMemoryCount(getID(), MEMORY_EVENT_GOOD_TO_US, kEvent.getTheirEnemyAttitudeModifier());
							AI_changeMemoryCount((PlayerTypes)iPlayer, MEMORY_EVENT_GOOD_TO_US, kEvent.getTheirEnemyAttitudeModifier());
						}
						else
						{
							kLoopPlayer.AI_changeMemoryCount(getID(), MEMORY_EVENT_BAD_TO_US, -kEvent.getTheirEnemyAttitudeModifier());
							AI_changeMemoryCount((PlayerTypes)iPlayer, MEMORY_EVENT_BAD_TO_US, -kEvent.getTheirEnemyAttitudeModifier());
						}
					}
				}
			}
		}
	}

	if (!CvString(kEvent.getPythonCallback()).empty())
	{
		long lResult;

		CyArgsList argsList;
		argsList.add(eEvent);
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pTriggeredData));

		gDLL->getPythonIFace()->callFunction(PYRandomEventModule, kEvent.getPythonCallback(), argsList.makeFunctionArgs(), &lResult);
	}

	if (kEvent.getNumWorldNews() > 0)
	{
		int iText = GC.getGame().getSorenRandNum(kEvent.getNumWorldNews(), "Event World News choice");

		CvWString szGlobalText;

		TeamTypes eTheirWorstEnemy = NO_TEAM;
		if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
		{
			eTheirWorstEnemy = GET_TEAM(GET_PLAYER(pTriggeredData->m_eOtherPlayer).getTeam()).AI_getWorstEnemy();
		}

		szGlobalText = gDLL->getText(kEvent.getWorldNews(iText).GetCString(),
			getCivilizationAdjectiveKey(),
			NULL != pCity ? pCity->getNameKey() : L"",
			pTriggeredData->m_eOtherPlayer != NO_PLAYER ? GET_PLAYER(pTriggeredData->m_eOtherPlayer).getCivilizationAdjectiveKey() : L"",
			NULL != pOtherPlayerCity ? pOtherPlayerCity->getNameKey() : L"",
			NO_RELIGION != pTriggeredData->m_eReligion ? GC.getReligionInfo(pTriggeredData->m_eReligion).getAdjectiveKey() : L"",
			NO_TEAM != eTheirWorstEnemy ? GET_TEAM(eTheirWorstEnemy).getName().GetCString() : L"",
			NO_CORPORATION != pTriggeredData->m_eCorporation ? GC.getCorporationInfo(pTriggeredData->m_eCorporation).getTextKeyWide() : L""
			);

		for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
		{
			CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);

			if (kLoopPlayer.isAlive())
			{
				if (GET_TEAM(kLoopPlayer.getTeam()).isHasMet(getTeam()) && (NO_PLAYER == pTriggeredData->m_eOtherPlayer || GET_TEAM(GET_PLAYER(pTriggeredData->m_eOtherPlayer).getTeam()).isHasMet(getTeam())))
				{
					bool bShowPlot = GC.getEventTriggerInfo(pTriggeredData->m_eTrigger).isShowPlot();

					if (bShowPlot)
					{
						if (kLoopPlayer.getTeam() != getTeam())
						{
							if (NULL == pPlot || !pPlot->isRevealed(kLoopPlayer.getTeam(), false))
							{
								bShowPlot = false;
							}
						}
					}

					if (bShowPlot)
					{
						gDLL->getInterfaceIFace()->addHumanMessage((PlayerTypes)iPlayer, false, GC.getEVENT_MESSAGE_TIME(), szGlobalText, "AS2D_CIVIC_ADOPT", MESSAGE_TYPE_MINOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"), pTriggeredData->m_iPlotX, pTriggeredData->m_iPlotY, true, true);
					}
					else
					{
						gDLL->getInterfaceIFace()->addHumanMessage((PlayerTypes)iPlayer, false, GC.getEVENT_MESSAGE_TIME(), szGlobalText, "AS2D_CIVIC_ADOPT", MESSAGE_TYPE_MINOR_EVENT);
					}
				}
			}
		}
		// advc.106g: Don't show (most) random events in replays
		//GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szGlobalText, pTriggeredData->m_iPlotX, pTriggeredData->m_iPlotY, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
	}

	if (!CvWString(kEvent.getLocalInfoTextKey()).empty())
	{
		CvWString szLocalText;

		TeamTypes eTheirWorstEnemy = NO_TEAM;
		if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
		{
			eTheirWorstEnemy = GET_TEAM(GET_PLAYER(pTriggeredData->m_eOtherPlayer).getTeam()).AI_getWorstEnemy();
		}

		szLocalText = gDLL->getText(kEvent.getLocalInfoTextKey(),
			getCivilizationAdjectiveKey(),
			NULL != pCity ? pCity->getNameKey() : L"",
			pTriggeredData->m_eOtherPlayer != NO_PLAYER ? GET_PLAYER(pTriggeredData->m_eOtherPlayer).getCivilizationAdjectiveKey() : L"",
			NULL != pOtherPlayerCity ? pOtherPlayerCity->getNameKey() : L"",
			NO_RELIGION != pTriggeredData->m_eReligion ? GC.getReligionInfo(pTriggeredData->m_eReligion).getAdjectiveKey() : L"",
			NO_TEAM != eTheirWorstEnemy ? GET_TEAM(eTheirWorstEnemy).getName().GetCString() : L"",
			NO_CORPORATION != pTriggeredData->m_eCorporation ? GC.getCorporationInfo(pTriggeredData->m_eCorporation).getTextKeyWide() : L""
			);

			if (GC.getEventTriggerInfo(pTriggeredData->m_eTrigger).isShowPlot())
			{
				gDLL->getInterfaceIFace()->addHumanMessage(getID(), false, GC.getEVENT_MESSAGE_TIME(), szLocalText, "AS2D_CIVIC_ADOPT", MESSAGE_TYPE_MINOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"), pTriggeredData->m_iPlotX, pTriggeredData->m_iPlotY, true, true);
			}
			else
			{
				gDLL->getInterfaceIFace()->addHumanMessage(getID(), false, GC.getEVENT_MESSAGE_TIME(), szLocalText, "AS2D_CIVIC_ADOPT", MESSAGE_TYPE_MINOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"));
			}

	}

	if (!CvWString(kEvent.getOtherPlayerPopup()).empty())
	{
		if (NO_PLAYER != pTriggeredData->m_eOtherPlayer)
		{
			CvWString szText = gDLL->getText(kEvent.getOtherPlayerPopup(),
				getCivilizationAdjectiveKey(),
				NULL != pCity ? pCity->getNameKey() : L"",
				pTriggeredData->m_eOtherPlayer != NO_PLAYER ? GET_PLAYER(pTriggeredData->m_eOtherPlayer).getCivilizationAdjectiveKey() : L"",
				NULL != pOtherPlayerCity ? pOtherPlayerCity->getNameKey() : L"",
				NO_RELIGION != pTriggeredData->m_eReligion ? GC.getReligionInfo(pTriggeredData->m_eReligion).getAdjectiveKey() : L"",
				NO_CORPORATION != pTriggeredData->m_eCorporation ? GC.getCorporationInfo(pTriggeredData->m_eCorporation).getTextKeyWide() : L""
				);

			CvPopupInfo* pInfo = new CvPopupInfo();

			if (NULL != pInfo)
			{
				pInfo->setText(szText);

				GET_PLAYER(pTriggeredData->m_eOtherPlayer).addPopup(pInfo);
			}
		}
	}

	bool bDeleteTrigger = bUpdateTrigger;

	for (int iEvent = 0; iEvent < GC.getNumEventInfos(); ++iEvent)
	{
		if (kEvent.getAdditionalEventTime(iEvent) == 0)
		{
			if (kEvent.getAdditionalEventChance(iEvent) > 0)
			{
				if (canDoEvent((EventTypes)iEvent, *pTriggeredData))
				{
					if (GC.getGame().getSorenRandNum(100, "Additional Event") < kEvent.getAdditionalEventChance(iEvent))
					{
						applyEvent((EventTypes)iEvent, iEventTriggeredId, false);
					}
				}
			}
		}
		else
		{
			bool bSetTimer = true;
			if (kEvent.getAdditionalEventChance(iEvent) > 0)
			{
				if (GC.getGame().getSorenRandNum(100, "Additional Event 2") >= kEvent.getAdditionalEventChance(iEvent))
				{
					bSetTimer = false;
				}
			}

			if (bSetTimer)
			{
				EventTriggeredData kTriggered = *pTriggeredData;
				kTriggered.m_iTurn = (GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getGrowthPercent() * kEvent.getAdditionalEventTime((EventTypes)iEvent)) / 100 + GC.getGame().getGameTurn();

				const EventTriggeredData* pExistingTriggered = getEventCountdown((EventTypes)iEvent);

				if (NULL != pExistingTriggered)
				{
					kTriggered.m_iTurn = std::min(kTriggered.m_iTurn, pExistingTriggered->m_iTurn);
				}

				setEventCountdown((EventTypes)iEvent, kTriggered);
				bDeleteTrigger = false;
			}
		}
	}

	if (bDeleteTrigger)
	{
		deleteEventTriggered(iEventTriggeredId);
	}
}

bool CvPlayer::isValidEventTech(TechTypes eTech, EventTypes eEvent, PlayerTypes eOtherPlayer) const
{
	CvEventInfo& kEvent = GC.getEventInfo(eEvent);

	if (kEvent.getTechPercent() == 0 && kEvent.getTechCostPercent() == 0)
	{
		return false;
	}

	if (kEvent.getTechPercent() < 0 && GET_TEAM(getTeam()).getResearchProgress(eTech) <= 0)
	{
		return false;
	}

	if (!canResearch(eTech))
	{
		return false;
	}
	/*  advc.004x (comment): No change here; OK with me to make all tech invalid
		during anarchy (ResearchTurnsLeft=-1). */
	if (getResearchTurnsLeft(eTech, true) < kEvent.getTechMinTurnsLeft())
	{
		return false;
	}

	if (NO_PLAYER != eOtherPlayer && !GET_TEAM(GET_PLAYER(eOtherPlayer).getTeam()).isHasTech(eTech))
	{
		return false;
	}

	return true;
}


TechTypes CvPlayer::getBestEventTech(EventTypes eEvent, PlayerTypes eOtherPlayer) const
{
	TechTypes eBestTech = NO_TECH;
	CvEventInfo& kEvent = GC.getEventInfo(eEvent);

	if (kEvent.getTechPercent() == 0 && kEvent.getTechCostPercent() == 0)
	{
		return NO_TECH;
	}

	if (NO_TECH != kEvent.getTech())
	{
		eBestTech = (TechTypes)kEvent.getTech();
	}
	else
	{
		bool bFoundFlavor = false;
		for (int i = 0; i < GC.getNumFlavorTypes(); ++i)
		{
			if (kEvent.getTechFlavorValue(i) != 0)
			{
				bFoundFlavor = true;
				break;
			}
		}

		if (!bFoundFlavor)
		{
			eBestTech = getCurrentResearch();
		}
	}

	if (NO_TECH != eBestTech)
	{
		if (!isValidEventTech(eBestTech, eEvent, eOtherPlayer))
		{
			eBestTech = NO_TECH;
		}
	}
	else
	{
		int iBestValue = 0;
		for (int iTech = 0; iTech < GC.getNumTechInfos(); ++iTech)
		{
			if (isValidEventTech((TechTypes)iTech, eEvent, eOtherPlayer))
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

	return eBestTech;
}

int CvPlayer::getEventCost(EventTypes eEvent, PlayerTypes eOtherPlayer, bool bRandom) const
{
	CvEventInfo& kEvent = GC.getEventInfo(eEvent);

	int iGold = kEvent.getGold();
	if (bRandom)
	{
		iGold += kEvent.getRandomGold();
	}

	iGold *= std::max(0, calculateInflationRate() + 100);
	iGold /= 100;

	TechTypes eBestTech = getBestEventTech(eEvent, eOtherPlayer);

	if (NO_TECH != eBestTech)
	{
		iGold -= (kEvent.getTechCostPercent() * GET_TEAM(getTeam()).getResearchCost(eBestTech)) / 100;
	}

	return iGold;
}


void CvPlayer::doEvents()
{
	if (GC.getGame().isOption(GAMEOPTION_NO_EVENTS))
	{
		return;
	}

	if (isBarbarian() || isMinorCiv())
	{
		return;
	}
	{ // advc.003: scope for iterator
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
	{
		bNewEventEligible = false;
	}

	if (bNewEventEligible)
	{
		if (GC.getGame().getSorenRandNum(GC.getDefineINT("EVENT_PROBABILITY_ROLL_SIDES"), "Global event check") >= GC.getEraInfo(getCurrentEra()).getEventChancePerTurn())
		{
			bNewEventEligible = false;
		}
	}

	std::vector< std::pair<EventTriggeredData*, int> > aePossibleEventTriggerWeights;
	int iTotalWeight = 0;
	for (int i = 0; i < GC.getNumEventTriggerInfos(); ++i)
	{
		int iWeight = getEventTriggerWeight((EventTriggerTypes)i);
		if (iWeight == -1)
		{
			trigger((EventTriggerTypes)i);
		}
		else if (iWeight > 0 && bNewEventEligible)
		{
			EventTriggeredData* pTriggerData = initTriggeredData((EventTriggerTypes)i);
			if (NULL != pTriggerData)
			{
				iTotalWeight += iWeight;
				aePossibleEventTriggerWeights.push_back(std::make_pair(pTriggerData, iTotalWeight));
			}
		}
	}

	if (iTotalWeight > 0)
	{
		bool bFired = false;
		int iValue = GC.getGame().getSorenRandNum(iTotalWeight, "Event trigger");
		for (std::vector< std::pair<EventTriggeredData*, int> >::iterator it = aePossibleEventTriggerWeights.begin(); it != aePossibleEventTriggerWeights.end(); ++it)
		{
			EventTriggeredData* pTriggerData = (*it).first;
			if (NULL != pTriggerData)
			{
				if (iValue < (*it).second && !bFired)
				{
					trigger(*pTriggerData);
					bFired = true;
				}
				else
				{
					deleteEventTriggered(pTriggerData->getID());
				}
			}
		}
	}

	std::vector<int> aCleanup;
	for (int i = 0; i < GC.getNumEventInfos(); ++i)
	{
		const EventTriggeredData* pTriggeredData = getEventCountdown((EventTypes)i);
		if (NULL != pTriggeredData)
		{
			if (GC.getGame().getGameTurn() >= pTriggeredData->m_iTurn)
			{
				applyEvent((EventTypes)i, pTriggeredData->m_iId);
				resetEventCountdown((EventTypes)i);
				aCleanup.push_back(pTriggeredData->m_iId);
			}
		}
	}

	for (std::vector<int>::iterator it = aCleanup.begin(); it != aCleanup.end(); ++it)
	{
		bool bDelete = true;

		for (int i = 0; i < GC.getNumEventInfos(); ++i)
		{
			const EventTriggeredData* pTriggeredData = getEventCountdown((EventTypes)i);
			if (NULL != pTriggeredData)
			{
				if (pTriggeredData->m_iId == *it)
				{
					bDelete = false;
					break;
				}
			}
		}

		if (bDelete)
		{
			deleteEventTriggered(*it);
		}
	}
}

// <advc.011>
void CvPlayer::decayBuildProgress() {

	CvMap& m = GC.getMap();
	for(int i = 0; i < m.numPlots(); i++) {
		CvPlot* pPlot = m.plotByIndex(i);
		if(pPlot == NULL) continue; CvPlot& p = *pPlot;
		if(!p.isWater() && p.getOwner() == getID())
			p.decayBuildProgress();
	}
} // </advc.011>

// <advc.002e>
void CvPlayer::showForeignPromoGlow(bool b) {

	if(getBugOptionBOOL("PLE__ShowPromotionGlow", false))
		return;
	for(int i = 0; i < MAX_PLAYERS; i++) {
		CvPlayer const& kPlayer = GET_PLAYER((PlayerTypes)i);
		if(!kPlayer.isAlive() || kPlayer.getID() == getID())
			continue;
		int foo=-1;
		for(CvUnit* u = kPlayer.firstUnit(&foo); u != NULL; u = kPlayer.nextUnit(&foo))
			gDLL->getEntityIFace()->showPromotionGlow(u->getUnitEntity(),
					u->isReadyForPromotion() && b);
	}
}// </advc.002e>

void CvPlayer::expireEvent(EventTypes eEvent, const EventTriggeredData& kTriggeredData, bool bFail)
{
	FAssert(getEventOccured(eEvent) == &kTriggeredData);
	FAssert(GC.getEventInfo(eEvent).isQuest() || GC.getGame().getGameTurn() - kTriggeredData.m_iTurn <= 4);

	if (GC.getEventInfo(eEvent).isQuest())
	{
		CvMessageQueue::iterator it;
		for (it = m_listGameMessages.begin(); it != m_listGameMessages.end(); ++it)
		{
			CvTalkingHeadMessage& message = *it;

			// the trigger ID is stored in the otherwise unused length field
			if (message.getLength() == kTriggeredData.getID())
			{
				m_listGameMessages.erase(it);
				gDLL->getInterfaceIFace()->dirtyTurnLog(getID());
				break;
			}
		}

		if (bFail)
		{
			gDLL->getInterfaceIFace()->addHumanMessage(getID(), false, GC.getEVENT_MESSAGE_TIME(), gDLL->getText(GC.getEventInfo(eEvent).getQuestFailTextKey()), "AS2D_CIVIC_ADOPT", MESSAGE_TYPE_MINOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_RED"));
		}
	}
}

bool CvPlayer::checkExpireEvent(EventTypes eEvent, const EventTriggeredData& kTriggeredData) const
{
	CvEventInfo& kEvent = GC.getEventInfo(eEvent);

	if (!CvString(kEvent.getPythonExpireCheck()).empty())
	{
		long lResult;
		CyArgsList argsList;
		argsList.add(eEvent);
		argsList.add(gDLL->getPythonIFace()->makePythonObject(&kTriggeredData));
		gDLL->getPythonIFace()->callFunction(PYRandomEventModule, kEvent.getPythonExpireCheck(), argsList.makeFunctionArgs(), &lResult);
		if (lResult != 0)
			return true;
	}

	if (!kEvent.isQuest())
	{
		if (GC.getGame().getGameTurn() - kTriggeredData.m_iTurn > 2)
		{
			return true;
		}

		return false;
	}

	CvEventTriggerInfo& kTrigger = GC.getEventTriggerInfo(kTriggeredData.m_eTrigger);

	FAssert(kTriggeredData.m_ePlayer != NO_PLAYER);

	CvPlayer& kPlayer = GET_PLAYER(kTriggeredData.m_ePlayer);

	if (kTrigger.isStateReligion() & kTrigger.isPickReligion())
	{
		if (kPlayer.getStateReligion() != kTriggeredData.m_eReligion)
		{
			return true;
		}
	}

	if (NO_CIVIC != kTrigger.getCivic())
	{
		if (!kPlayer.isCivic((CivicTypes)kTrigger.getCivic()))
		{
			return true;
		}
	}

	if (kTriggeredData.m_iCityId != -1)
	{
		if (NULL == kPlayer.getCity(kTriggeredData.m_iCityId))
		{
			return true;
		}
	}

	if (kTriggeredData.m_iUnitId != -1)
	{
		if (NULL == kPlayer.getUnit(kTriggeredData.m_iUnitId))
		{
			return true;
		}
	}

	if (NO_PLAYER != kTriggeredData.m_eOtherPlayer)
	{
		if (!GET_PLAYER(kTriggeredData.m_eOtherPlayer).isAlive())
		{
			return true;
		}

		if (kTriggeredData.m_iOtherPlayerCityId != -1)
		{
			if (NULL == GET_PLAYER(kTriggeredData.m_eOtherPlayer).getCity(kTriggeredData.m_iOtherPlayerCityId))
			{
				return true;
			}
		}
	}

	if (kTrigger.getNumObsoleteTechs() > 0)
	{
		for (int iI = 0; iI < kTrigger.getNumObsoleteTechs(); iI++)
		{
			if (GET_TEAM(getTeam()).isHasTech((TechTypes)(kTrigger.getObsoleteTech(iI))))
			{
				return true;
			}
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
		EventTypes eEvent = AI_chooseEvent(kData.getID());
		if (NO_EVENT != eEvent)
		{
			applyEvent(eEvent, kData.getID());
		}
	}
}

bool CvPlayer::canTrigger(EventTriggerTypes eTrigger, PlayerTypes ePlayer, ReligionTypes eReligion) const
{
	if (!isAlive())
	{
		return false;
	}

	if (getID() == ePlayer)
	{
		return false;
	}

	CvPlayer& kPlayer = GET_PLAYER(ePlayer);
	CvEventTriggerInfo& kTrigger = GC.getEventTriggerInfo(eTrigger);

	if (getTeam() == kPlayer.getTeam())
	{
		return false;
	}

	if (!kTrigger.isPickPlayer())
	{
		return false;
	}

	if (!GET_TEAM(getTeam()).isHasMet(kPlayer.getTeam()))
	{
		return false;
	}

	if (isHuman() && kTrigger.isOtherPlayerAI())
	{
		return false;
	}

	if (GET_TEAM(getTeam()).isAtWar(kPlayer.getTeam()) != kTrigger.isOtherPlayerWar())
	{
		return false;
	}

	if (NO_TECH != kTrigger.getOtherPlayerHasTech())
	{
		if (!GET_TEAM(getTeam()).isHasTech((TechTypes)kTrigger.getOtherPlayerHasTech()))
		{
			return false;
		}
	}

	if (kTrigger.getOtherPlayerShareBorders() > 0)
	{
		int iCount = 0;

		for (int iI = 0; iI < GC.getMap().numPlots(); ++iI)
		{
			CvPlot* pLoopPlot = GC.getMap().plotByIndex(iI);

			if (!pLoopPlot->isWater())
			{
				if ((pLoopPlot->getOwner() == getID()) && pLoopPlot->isAdjacentPlayer(ePlayer, true))
				{
					++iCount;
				}
			}
		}

		if (iCount < kTrigger.getOtherPlayerShareBorders())
		{
			return false;
		}
	}

	if (NO_RELIGION != eReligion)
	{
		bool bHasReligion = kTrigger.isStateReligion() ? (getStateReligion() == eReligion) : (getHasReligionCount(eReligion) > 0);

		if (kTrigger.isOtherPlayerHasReligion())
		{
			if (!bHasReligion)
			{
				return false;
			}
		}

		if (kTrigger.isOtherPlayerHasOtherReligion())
		{
			if (bHasReligion)
			{
				return false;
			}

			if (kTrigger.isStateReligion() && getStateReligion() == NO_RELIGION)
			{
				return false;
			}

		}
	}

	return true;
}

CvCity* CvPlayer::pickTriggerCity(EventTriggerTypes eTrigger) const
{
	CvCity* pCity = NULL;
	std::vector<CvCity*> apCities;
	int iLoop;
	int iBestValue = MIN_INT;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
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
	int iLoop;
	int iBestValue = MIN_INT;
	for (CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
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
	CvEventTriggerInfo& kTrigger = GC.getEventTriggerInfo(eTrigger);

	if (NO_HANDICAP != kTrigger.getMinDifficulty())
	{
		if (GC.getHandicapInfo(GC.getGame().getHandicapType()).
				getDifficulty() < 10 * // advc.250a
				kTrigger.getMinDifficulty())
		{
			return 0;
		}
	}

	if (kTrigger.isSinglePlayer() && GC.getGame().isGameMultiPlayer())
	{
		return 0;
	}

	if (!GC.getGame().isEventActive(eTrigger))
	{
		return 0;
	}

	if (kTrigger.getNumObsoleteTechs() > 0)
	{
		for (int iI = 0; iI < kTrigger.getNumObsoleteTechs(); iI++)
		{
			if (GET_TEAM(getTeam()).isHasTech((TechTypes)(kTrigger.getObsoleteTech(iI))))
			{
				return 0;
			}
		}
	}

	if (!kTrigger.isRecurring())
	{
		if (isTriggerFired(eTrigger))
		{
			return 0;
		}
	}

	if (kTrigger.getNumPrereqOrTechs() > 0)
	{
		bool bFoundValid = false;

		for (int iI = 0; iI < kTrigger.getNumPrereqOrTechs(); iI++)
		{
			if (GET_TEAM(getTeam()).isHasTech((TechTypes)(kTrigger.getPrereqOrTechs(iI))))
			{
				bFoundValid = true;
				break;
			}
		}

		if (!bFoundValid)
		{
			return 0;
		}
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
		{
			return 0;
		}
	}

	if (kTrigger.getNumPrereqEvents() > 0)
	{
		bool bFoundValid = true;
		for (int iI = 0; iI < kTrigger.getNumPrereqEvents(); iI++)
		{
			if (NULL == getEventOccured((EventTypes)kTrigger.getPrereqEvent(iI)))
			{
				bFoundValid = false;
				break;
			}
		}

		if (!bFoundValid)
		{
			return 0;
		}
	}

	if (NO_CIVIC != kTrigger.getCivic())
	{
		bool bFoundValid = false;

		for (int iI = 0; iI < GC.getNumCivicOptionInfos(); ++iI)
		{
			if (getCivics((CivicOptionTypes)iI) == kTrigger.getCivic())
			{
				bFoundValid = true;
				break;
			}
		}

		if (!bFoundValid)
		{
			return 0;
		}
	}

	if (kTrigger.getMinTreasury() > 0)
	{
		if (getGold() < kTrigger.getMinTreasury())
		{
			return 0;
		}
	}

	if (GC.getMap().getNumLandAreas() < kTrigger.getMinMapLandmass())
	{
		return 0;
	}

	if (kTrigger.getMinOurLandmass() > 0 || kTrigger.getMaxOurLandmass() != -1)
	{
		int iNumLandmass = 0;

		int iLoop;
		for (CvArea* pArea = GC.getMap().firstArea(&iLoop); NULL != pArea; pArea = GC.getMap().nextArea(&iLoop))
		{
			if (!pArea->isWater())
			{
				if (pArea->getCitiesPerPlayer(getID()) > 0)
				{
					++iNumLandmass;
				}
			}
		}

		if (iNumLandmass < kTrigger.getMinOurLandmass())
		{
			return 0;
		}

		if (kTrigger.getMaxOurLandmass() != -1 && iNumLandmass > kTrigger.getMaxOurLandmass())
		{
			return 0;
		}
	}

	if (kTrigger.getProbability() < 0)
	{
		return kTrigger.getProbability();
	}

	int iProbability = kTrigger.getProbability();

	if (kTrigger.isProbabilityUnitMultiply() && kTrigger.getNumUnits() > 0)
	{
		int iNumUnits = 0;
		int iLoop;
		for (CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
		{
			if (MIN_INT != pLoopUnit->getTriggerValue(eTrigger, NULL, true))
			{
				++iNumUnits;
			}
		}

		iProbability *= iNumUnits;
	}

	if (kTrigger.isProbabilityBuildingMultiply() && kTrigger.getNumBuildings() > 0)
	{
		int iNumBuildings = 0;
		for (int i = 0; i < kTrigger.getNumBuildingsRequired(); ++i)
		{
			if (kTrigger.getBuildingRequired(i) != NO_BUILDINGCLASS)
			{
				iNumBuildings += getBuildingClassCount((BuildingClassTypes)kTrigger.getBuildingRequired(i));
			}
		}

		iProbability *= iNumBuildings;
	}

	return iProbability;
}


PlayerTypes CvPlayer::getSplitEmpirePlayer(int iAreaId) const
{
	// can't create different derivative civs on the same continent
	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kLoopPlayer.isAlive() && kLoopPlayer.getParent() == getID())
		{
			CvCity* pLoopCapital = kLoopPlayer.getCapitalCity();
			if (NULL != pLoopCapital)
			{
				if (pLoopCapital->area()->getID() == iAreaId)
				{
					return NO_PLAYER;
				}
			}
		}
	}

	PlayerTypes eNewPlayer = NO_PLAYER;

	// Try to find a player who's never been in the game before
	for (int i = 0; i < MAX_CIV_PLAYERS; ++i)
	{
		if (!GET_PLAYER((PlayerTypes)i).isEverAlive())
		{
			eNewPlayer = (PlayerTypes)i;
			break;
		}
	}

	if (eNewPlayer == NO_PLAYER)
	{
		// Try to recycle a dead player
		for (int i = 0; i < MAX_CIV_PLAYERS; ++i)
		{
			if (!GET_PLAYER((PlayerTypes)i).isAlive())
			{	// advc.test (see under dlph.24 in manual)
				FAssertMsg(false, "About to reuse a defeated player -- this may or may not work correctly");
				eNewPlayer = (PlayerTypes)i;
				break;
			}
		}
	}

	return eNewPlayer;
}

bool CvPlayer::canSplitEmpire() const
{
	int iLoopArea;

	if (GC.getGame().isOption(GAMEOPTION_NO_VASSAL_STATES))
	{
		return false;
	}

	if (isAVassal())
	{
		return false;
	}

	CivLeaderArray aLeaders;
	if (!getSplitEmpireLeaders(aLeaders))
	{
		return false;
	}

	bool bFoundArea = false;

	for (CvArea* pLoopArea = GC.getMap().firstArea(&iLoopArea); pLoopArea != NULL; pLoopArea = GC.getMap().nextArea(&iLoopArea))
	{
		if (canSplitArea(pLoopArea->getID()))
		{
			bFoundArea = true;
			break;
		}
	}

	if (!bFoundArea)
	{
		return false;
	}

	return true;
}

bool CvPlayer::canSplitArea(int iAreaId) const
{
	CvArea* pArea = GC.getMap().getArea(iAreaId);
	CvCity* pCapital = getCapitalCity();

	if (NULL == pCapital)
	{
		return false;
	}

	if (NULL == pArea || pArea == pCapital->area())
	{
		return false;
	}

	if (pArea->getCitiesPerPlayer(getID()) == 0)
	{
		return false;
	}

	PlayerTypes ePlayer = getSplitEmpirePlayer(pArea->getID());
	if (NO_PLAYER == ePlayer)
	{
		return false;
	}

	if (!GET_PLAYER(ePlayer).isAlive())
	{
		if (pArea->getCitiesPerPlayer(getID()) <= 1)
		{
			return false;
		}
	}

	return true;
}

bool CvPlayer::getSplitEmpireLeaders(CivLeaderArray& aLeaders) const
{
	aLeaders.clear();

	for (int i = 0; i < GC.getNumCivilizationInfos(); ++i)
	{
		bool bValid = true;

		if (getCivilizationType() == i)
		{
			bValid = false;
		}

		if (bValid)
		{
			if (!GC.getCivilizationInfo((CivilizationTypes)i).isPlayable() || !GC.getCivilizationInfo((CivilizationTypes)i).isAIPlayable())
			{
				bValid = false;
			}
		}

		if (bValid)
		{
			for (int j = 0; j < MAX_CIV_PLAYERS; ++j)
			{
				if (getID() != j && GET_PLAYER((PlayerTypes)j).isEverAlive() && GET_PLAYER((PlayerTypes)j).getCivilizationType() == i)
				{
					bValid = false;
					break;
				}
			}
		}

		if (bValid)
		{
			for (int j = 0; j < GC.getNumLeaderHeadInfos(); ++j)
			{
				bool bLeaderValid = true;
				if (!GC.getCivilizationInfo((CivilizationTypes)i).isLeaders(j) && !GC.getGame().isOption(GAMEOPTION_LEAD_ANY_CIV))
				{
					bLeaderValid = false;
				}

				if (bLeaderValid)
				{
					for (int k = 0; k < MAX_CIV_PLAYERS; ++k)
					{
						if (GET_PLAYER((PlayerTypes)k).isEverAlive() && GET_PLAYER((PlayerTypes)k).getPersonalityType() == j)
						{
							bLeaderValid = false;
						}
					}
				}

				if (bLeaderValid)
				{
					aLeaders.push_back(std::make_pair((CivilizationTypes)i, (LeaderHeadTypes)j));
				}
			}
		}
	}

	return (aLeaders.size() > 0);
}

bool CvPlayer::splitEmpire(int iAreaId)
{
	PROFILE_FUNC();

	if (!canSplitEmpire() || !canSplitArea(iAreaId))
		return false;

	CvArea* pArea = GC.getMap().getArea(iAreaId);
	if (pArea == NULL)
		return false;

	PlayerTypes eNewPlayer = getSplitEmpirePlayer(iAreaId);
	if (eNewPlayer == NO_PLAYER)
		return false;

	CvGame& g = GC.getGame();
	bool bPlayerExists = TEAMREF(eNewPlayer).isAlive();
	FAssert(!bPlayerExists);
	CvWString szMessage; // advc.127b
	if (!bPlayerExists)
	{
		int iBestValue = -1;
		LeaderHeadTypes eBestLeader = NO_LEADER;
		CivilizationTypes eBestCiv = NO_CIVILIZATION;

		CivLeaderArray aLeaders;
		if (getSplitEmpireLeaders(aLeaders))
		{
			CivLeaderArray::iterator it;
			for (it = aLeaders.begin(); it != aLeaders.end(); ++it)
			{
				int iValue = (1 + g.getSorenRandNum(100, "Choosing Split Personality"));
				if (GC.getCivilizationInfo(getCivilizationType()).getDerivativeCiv() == it->first)
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

		szMessage = gDLL->getText("TXT_KEY_MISC_EMPIRE_SPLIT", getNameKey(), GC.getCivilizationInfo(eBestCiv).getShortDescriptionKey(), GC.getLeaderHeadInfo(eBestLeader).getTextKeyWide());
		// advc.127b: Announcement loop moved down

		g.addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getID(), szMessage, -1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));

		// remove leftover culture from old recycled player
		/*  BETTER_BTS_AI_MOD, Bugfix, 12/30/08, jdog5000: commented out
			(Clearing plot culture along with many other bits of data now handled by CvGame::addPlayer) */
		/* for (int iPlot = 0; iPlot < GC.getMap().numPlots(); ++iPlot) {
			CvPlot* pLoopPlot = GC.getMap().plotByIndex(iPlot);
			pLoopPlot->setCulture(eNewPlayer, 0, false, false);
		}*/

		g.addPlayer(eNewPlayer, eBestLeader, eBestCiv);
		GET_PLAYER(eNewPlayer).setParent(getID());
		GC.getInitCore().setLeaderName(eNewPlayer, GC.getLeaderHeadInfo(eBestLeader).getTextKeyWide());

		CvTeam& kNewTeam = GET_TEAM(GET_PLAYER(eNewPlayer).getTeam());
		for (int i = 0; i < GC.getNumTechInfos(); ++i)
		{
			TechTypes eLoopTech = (TechTypes)i;
			if (GET_TEAM(getTeam()).isHasTech(eLoopTech))
			{
				kNewTeam.setHasTech(eLoopTech, true, eNewPlayer, false, false);
				if (GET_TEAM(getTeam()).isNoTradeTech(eLoopTech) ||
						(g.isOption(GAMEOPTION_NO_TECH_BROKERING)
						&& isSignificantDiscovery(eLoopTech))) // advc.550e
					kNewTeam.setNoTradeTech(eLoopTech, true);
			}
		}
		// dlph.24: Commented out
		/*for (int iTeam = 0; iTeam < MAX_TEAMS; ++iTeam) {
			CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
			if (kLoopTeam.isAlive()) {
				kNewTeam.setEspionagePointsAgainstTeam((TeamTypes)iTeam, GET_TEAM(getTeam()).getEspionagePointsAgainstTeam((TeamTypes)iTeam));
				kLoopTeam.setEspionagePointsAgainstTeam(GET_PLAYER(eNewPlayer).getTeam(), kLoopTeam.getEspionagePointsAgainstTeam(getTeam()));
			}
		}*/
		kNewTeam.setEspionagePointsEver(GET_TEAM(getTeam()).getEspionagePointsEver());

		GET_TEAM(getTeam()).assignVassal(GET_PLAYER(eNewPlayer).getTeam(), false);

		AI_updateBonusValue();
	}

	std::vector< std::pair<int, int> > aCultures;
	for (int iPlot = 0; iPlot < GC.getMap().numPlots(); ++iPlot)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(iPlot);

		bool bTranferPlot = false;

		if (pLoopPlot->area() == pArea)
		{
			bTranferPlot = true;
		}

		if (!bTranferPlot)
		{
			CvCity* pWorkingCity = pLoopPlot->getWorkingCity();
			if (NULL != pWorkingCity && pWorkingCity->getOwner() == getID() && pWorkingCity->area() == pArea)
			{
				bTranferPlot = true;
			}
		}

		if (!bTranferPlot && pLoopPlot->isWater() && pLoopPlot->isAdjacentToArea(pArea))
		{
			bTranferPlot = true;
		}

		if (bTranferPlot)
		{
			int iCulture = pLoopPlot->getCulture(getID());

			if (bPlayerExists)
			{
				iCulture = std::max(iCulture, pLoopPlot->getCulture(eNewPlayer));
			}

			aCultures.push_back(std::make_pair(iPlot, iCulture));
		}

		if (pLoopPlot->isRevealed(getTeam(), false))
		{
			pLoopPlot->setRevealed(GET_PLAYER(eNewPlayer).getTeam(), true, false, getTeam(), false);
		}
	}
	std::vector<CvCity*> apAcquiredCities; // advc.104r
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (pLoopCity->area() == pArea)
		{
			int iCulture = pLoopCity->getCultureTimes100(getID());
			CvPlot* pPlot = pLoopCity->plot();

			GET_PLAYER(eNewPlayer).acquireCity(pLoopCity, false, true, false);

			if (NULL != pPlot)
			{
				CvCity* pCity = pPlot->getPlotCity();
				if (NULL != pCity)
				{
					pCity->setCultureTimes100(eNewPlayer, iCulture, false, false);
					/*  <advc.104r> Want to initialize UWAI data after assigning
						cities but before creating units. Therefore move
						unit placement into a separate loop. */
					apAcquiredCities.push_back(pCity);
				}
				/*for (int i = 0; i < GC.getDefineINT("COLONY_NUM_FREE_DEFENDERS"); ++i)
					pCity->initConscriptedUnit();*/ // </advc.104r>
			}
		}
	}

	for (uint i = 0; i < aCultures.size(); ++i)
	{
		CvPlot* pPlot = GC.getMap().plotByIndex(aCultures[i].first);
		pPlot->setCulture(eNewPlayer, aCultures[i].second, true, false);
		pPlot->setCulture(getID(), 0, true, false);

		for (int iTeam = 0; iTeam < MAX_TEAMS; ++iTeam)
		{
			if (pPlot->getRevealedOwner((TeamTypes)iTeam, false) == getID())
			{
				pPlot->setRevealedOwner((TeamTypes)iTeam, eNewPlayer);
			}
		}
	}
	// advc.130r:
	GET_PLAYER(eNewPlayer).AI_changeMemoryCount(getID(), MEMORY_INDEPENDENCE,
			// advc.130j: Twice remembered
			2 * GC.getLeaderHeadInfo(getPersonalityType()).getFreedomAppreciation());

	g.updatePlotGroups();
	// K-Mod
	GET_PLAYER(eNewPlayer).AI_updateAttitudeCache(getID());
	AI().AI_updateAttitudeCache(eNewPlayer);
	// K-Mod end
	// <advc.104r>
	if(getWPAI.isEnabled())
		getWPAI.processNewCivInGame(eNewPlayer);
	for(size_t i = 0; i < apAcquiredCities.size(); i++)
		for(int j = 0; j < GC.getDefineINT("COLONY_NUM_FREE_DEFENDERS"); j++)
			apAcquiredCities[i]->initConscriptedUnit();
	// </advc.104r>
	/*  <advc.127b> Cut and pasted here b/c I want the announcement to point
		to the new capital */
	if(!bPlayerExists && eNewPlayer != NO_PLAYER) {
		FAssert(!szMessage.empty());
		CvCity* pNewCapital = GET_PLAYER(eNewPlayer).getCapitalCity();
		for (int i = 0; i < MAX_CIV_PLAYERS; ++i) {
			CvPlayer const& kObs = GET_PLAYER((PlayerTypes)i);
			if(!kObs.isAlive())
				continue;
			if (i == getID() || i == eNewPlayer || GET_TEAM(getTeam()).isHasMet(kObs.getTeam()) ||
					kObs.isSpectator()) // advc.127
			{
				bool bRev = (pNewCapital != NULL && pNewCapital->isRevealed(
						kObs.getTeam(), true));
				LPCSTR szButton = (bRev ? ARTFILEMGR.getInterfaceArtInfo(
						"INTERFACE_CITY_BAR_CAPITAL_TEXTURE")->getPath() : NULL);
				gDLL->getInterfaceIFace()->addHumanMessage((PlayerTypes)i, false,
						GC.getEVENT_MESSAGE_TIME(), szMessage, "AS2D_REVOLTEND",
						MESSAGE_TYPE_MAJOR_EVENT,
						szButton, NO_COLOR, bRev ? pNewCapital->getX() : -1,
						bRev ? pNewCapital->getY() : -1);
			}
		}
	} // </advc.127b>
	return true;
}

bool CvPlayer::isValidTriggerReligion(const CvEventTriggerInfo& kTrigger, CvCity* pCity, ReligionTypes eReligion) const
{
	if (kTrigger.getNumReligionsRequired() > 0)
	{
		bool bFound = false;

		for (int i = 0; i < kTrigger.getNumReligionsRequired(); ++i)
		{
			if (eReligion == kTrigger.getReligionRequired(i))
			{
				bFound = true;
				break;
			}
		}

		if (!bFound)
		{
			return false;
		}
	}

	if (NULL != pCity)
	{
		if (!pCity->isHasReligion(eReligion))
		{
			return false;
		}

		if (kTrigger.isHolyCity())
		{
			if (!pCity->isHolyCity(eReligion))
			{
				return false;
			}
		}
	}
	else
	{
		if (getHasReligionCount(eReligion) == 0)
		{
			return false;
		}

		if (kTrigger.isHolyCity())
		{
			CvCity* pHolyCity = GC.getGame().getHolyCity(eReligion);
			if (NULL == pHolyCity || pHolyCity->getOwner() != getID())
			{
				return false;
			}
		}
	}

	return true;
}

bool CvPlayer::isValidTriggerCorporation(const CvEventTriggerInfo& kTrigger, CvCity* pCity, CorporationTypes eCorporation) const
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
		{
			return false;
		}
	}

	if (NULL != pCity)
	{
		if (!pCity->isHasCorporation(eCorporation))
		{
			return false;
		}

		if (kTrigger.isHeadquarters())
		{
			if (!pCity->isHeadquarters(eCorporation))
			{
				return false;
			}
		}
	}
	else
	{
		/* if (getHasCorporationCount(eCorporation) > 0)
			return true;*/
		// K-Mod. (bugfix)
		if (getHasCorporationCount(eCorporation) == 0)
			return false;
		// K-Mod end

		if (kTrigger.isHeadquarters())
		{
			CvCity* pHeadquarters = GC.getGame().getHeadquarters(eCorporation);
			if (NULL == pHeadquarters || pHeadquarters->getOwner() != getID())
			{
				return false;
			}
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

	CvCity* pCapital = getCapitalCity();
	// <advc.106> Cut from the loop below. Use this for the replay message as well.
	CvWString szMsg(gDLL->getText("TXT_KEY_VICTORY_TEAM_HAS_LAUNCHED",
			kTeam.getName().GetCString()));
	int iPlotX = -1;
	int iPlotY = -1; // </advc.106>
	for(int i = 0; i < MAX_CIV_PLAYERS; ++i)
	{
		CvPlayer const& kObs = GET_PLAYER((PlayerTypes)i);
		if (!kObs.isAlive())
			continue;

		if(pCapital != NULL && pCapital->isRevealed(kObs.getTeam(), false))
		{
			iPlotX = pCapital->getX();
			iPlotY = pCapital->getY();
		}
		CvWString szBuffer(szMsg); // advc.106
		if (kObs.getTeam() == getTeam())
			szBuffer = gDLL->getText("TXT_KEY_VICTORY_YOU_HAVE_LAUNCHED");

		gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(), true,
				GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_CULTURELEVEL",
				MESSAGE_TYPE_MAJOR_EVENT, ARTFILEMGR.getMiscArtInfo("SPACE_SHIP_BUTTON")->getPath(),
				(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
				iPlotX, iPlotY, true, true);
	}
	// <advc.106>
	GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT,
			getID(), szMsg, iPlotX, iPlotY,
			(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
	// </advc.106>
}

bool CvPlayer::isFreePromotion(UnitCombatTypes eUnitCombat, PromotionTypes ePromotion) const
{
	for (UnitCombatPromotionArray::const_iterator it = m_aFreeUnitCombatPromotions.begin(); it != m_aFreeUnitCombatPromotions.end(); ++it)
	{
		if ((*it).first == eUnitCombat && (*it).second == ePromotion)
		{
			return true;
		}
	}

	return false;
}

void CvPlayer::setFreePromotion(UnitCombatTypes eUnitCombat, PromotionTypes ePromotion, bool bFree)
{
	for (UnitCombatPromotionArray::iterator it = m_aFreeUnitCombatPromotions.begin(); it != m_aFreeUnitCombatPromotions.end(); ++it)
	{
		if ((*it).first == eUnitCombat && (*it).second == ePromotion)
		{
			if (!bFree)
			{
				m_aFreeUnitCombatPromotions.erase(it);
			}
			return;
		}
	}

	if (bFree)
	{
		m_aFreeUnitCombatPromotions.push_back(std::make_pair(eUnitCombat, ePromotion));
	}
}

bool CvPlayer::isFreePromotion(UnitClassTypes eUnitClass, PromotionTypes ePromotion) const
{
	for (UnitClassPromotionArray::const_iterator it = m_aFreeUnitClassPromotions.begin(); it != m_aFreeUnitClassPromotions.end(); ++it)
	{
		if ((*it).first == eUnitClass && (*it).second == ePromotion)
		{
			return true;
		}
	}

	return false;
}

void CvPlayer::setFreePromotion(UnitClassTypes eUnitClass, PromotionTypes ePromotion, bool bFree)
{
	for (UnitClassPromotionArray::iterator it = m_aFreeUnitClassPromotions.begin(); it != m_aFreeUnitClassPromotions.end(); ++it)
	{
		if ((*it).first == eUnitClass && (*it).second == ePromotion)
		{
			if (!bFree)
			{
				m_aFreeUnitClassPromotions.erase(it);
			}
			return;
		}
	}

	if (bFree)
	{
		m_aFreeUnitClassPromotions.push_back(std::make_pair(eUnitClass, ePromotion));
	}
}

PlayerVoteTypes CvPlayer::getVote(int iId) const
{
	for (std::vector< std::pair<int, PlayerVoteTypes> >::const_iterator it = m_aVote.begin(); it != m_aVote.end(); ++it)
	{
		if ((*it).first == iId)
		{
			return ((*it).second);
		}
	}

	return NO_PLAYER_VOTE;
}

void CvPlayer::setVote(int iId, PlayerVoteTypes ePlayerVote)
{
	for (std::vector< std::pair<int, PlayerVoteTypes> >::iterator it = m_aVote.begin(); it != m_aVote.end(); ++it)
	{
		if ((*it).first == iId)
		{
			if (ePlayerVote == NO_PLAYER_VOTE)
			{
				m_aVote.erase(it);
			}
			else
			{
				(*it).second = ePlayerVote;
			}
			return;
		}
	}

	if (ePlayerVote != NO_PLAYER_VOTE)
	{
		m_aVote.push_back(std::make_pair(iId, ePlayerVote));
	}
}

int CvPlayer::getUnitExtraCost(UnitClassTypes eUnitClass) const
{
	for (std::vector< std::pair<UnitClassTypes, int> >::const_iterator it = m_aUnitExtraCosts.begin(); it != m_aUnitExtraCosts.end(); ++it)
	{
		if ((*it).first == eUnitClass)
		{
			return ((*it).second);
		}
	}

	return 0;
}

void CvPlayer::setUnitExtraCost(UnitClassTypes eUnitClass, int iCost)
{
	for (std::vector< std::pair<UnitClassTypes, int> >::iterator it = m_aUnitExtraCosts.begin(); it != m_aUnitExtraCosts.end(); ++it)
	{
		if ((*it).first == eUnitClass)
		{
			if (iCost == 0)
			{
				m_aUnitExtraCosts.erase(it);
			}
			else
			{
				(*it).second = iCost;
			}
			return;
		}
	}

	if (iCost != 0)
	{
		m_aUnitExtraCosts.push_back(std::make_pair(eUnitClass, iCost));
	}
}

// CACHE: cache frequently used values
///////////////////////////////////////
bool CvPlayer::hasShrine(ReligionTypes eReligion)
{
	bool bHasShrine = false;

	if (eReligion != NO_RELIGION)
	{
		CvCity* pHolyCity = GC.getGame().getHolyCity(eReligion);

		// if the holy city exists, and we own it
		if (pHolyCity != NULL && pHolyCity->getOwner() == getID())
			bHasShrine = pHolyCity->hasShrine(eReligion);
	}
	return bHasShrine;
}

void CvPlayer::invalidatePopulationRankCache()
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->invalidatePopulationRankCache();
	}
}

void CvPlayer::invalidateYieldRankCache(YieldTypes eYield)
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->invalidateYieldRankCache();
	}
}

void CvPlayer::invalidateCommerceRankCache(CommerceTypes eCommerce)
{
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->invalidateCommerceRankCache();
	}
}


void CvPlayer::doUpdateCacheOnTurn()
{
	// add this back, after testing without it
	// invalidateYieldRankCache();
}

void CvPlayer::processVoteSourceBonus(VoteSourceTypes eVoteSource, bool bActive)
{
	int iLoop;
	for (CvCity* pCity = firstCity(&iLoop); NULL != pCity; pCity = nextCity(&iLoop))
	{
		pCity->processVoteSourceBonus(eVoteSource, bActive);
	}
}

int CvPlayer::getVotes(VoteTypes eVote, VoteSourceTypes eVoteSource) const
{
	int iVotes = 0;

	ReligionTypes eReligion = GC.getGame().getVoteSourceReligion(eVoteSource);

	if (NO_VOTE == eVote)
	{
		if (NO_RELIGION != eReligion)
		{
			iVotes = getReligionPopulation(eReligion);
		}
		else
		{
			iVotes = getTotalPopulation();
		}
	}
	else
	{
		if (!GC.getVoteInfo(eVote).isVoteSourceType(eVoteSource))
		{
			return 0;
		}

		if (GC.getVoteInfo(eVote).isCivVoting())
		{
			if (NO_RELIGION == eReligion || getHasReligionCount(eReligion) > 0)
			{
				iVotes = 1;
			}
		}
		else if (GC.getVoteInfo(eVote).isCityVoting())
		{
			if (NO_RELIGION != eReligion)
			{
				iVotes = getHasReligionCount(eReligion);
			}
			else
			{
				iVotes = getNumCities();
			}
		}
		else
		{
			if (NO_RELIGION == eReligion)
			{
				iVotes = getTotalPopulation();
			}
			else
			{
				iVotes = getReligionPopulation(eReligion);
			}
		}

		if (NO_RELIGION != eReligion && getStateReligion() == eReligion)
		{
			iVotes *= (100 + GC.getVoteInfo(eVote).getStateReligionVotePercent());
			iVotes /= 100;
		}
	}

	return iVotes;
}

bool CvPlayer::canDoResolution(VoteSourceTypes eVoteSource, const VoteSelectionSubData& kData) const
{
	CvTeam& kOurTeam = GET_TEAM(getTeam());
	CvGame const& g = GC.getGame(); // advc.003
	if (NO_PLAYER != kData.ePlayer)
	{
		if (!kOurTeam.isHasMet(GET_PLAYER(kData.ePlayer).getTeam()))
		{
			return false;
		}
	}
	CvVoteInfo const& kVote = GC.getVoteInfo(kData.eVote); // advc.003
	if (kVote.isOpenBorders())
	{
		for (int iTeam2 = 0; iTeam2 < MAX_CIV_TEAMS; ++iTeam2)
		{
			if (GET_TEAM((TeamTypes)iTeam2).isVotingMember(eVoteSource))
			{
				if (!kOurTeam.isOpenBordersTrading() && !GET_TEAM((TeamTypes)iTeam2).isOpenBordersTrading())
				{
					return false;
				}

				if (kOurTeam.isAtWar((TeamTypes)iTeam2))
				{
					return false;
				}
			}
		}
	}
	else if (kVote.isDefensivePact())
	{
		for (int iTeam2 = 0; iTeam2 < MAX_CIV_TEAMS; ++iTeam2)
		{
			if (GET_TEAM((TeamTypes)iTeam2).isVotingMember(eVoteSource))
			{
				if (!kOurTeam.isDefensivePactTrading() && !GET_TEAM((TeamTypes)iTeam2).isDefensivePactTrading())
				{
					return false;
				}

				if ((kOurTeam.getAtWarCount(true) > 0 || GET_TEAM((TeamTypes)iTeam2).getAtWarCount(true) > 0)
						// dlph.25: 'Sometimes defensive pact can be signed while at war'
						&& GC.getBBAI_DEFENSIVE_PACT_BEHAVIOR() == 0)
				{
					return false;
				} // <dlph.25>
				if(kOurTeam.isAtWar((TeamTypes)iTeam2)
						/*  advc: Same additional restriction as for DP between
							AI teams (dlph.3) */
						|| !kOurTeam.allWarsShared((TeamTypes)iTeam2))
					return false; // </dlph.25>
				if (!kOurTeam.canSignDefensivePact((TeamTypes)iTeam2))
				{
					return false;
				}
			}
		}
	}
	else if (kVote.isForcePeace())
	{
		FAssert(NO_PLAYER != kData.ePlayer);
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);

		if (kPlayer.getTeam() != getTeam())
		{
			if (kOurTeam.isAtWar(kPlayer.getTeam()))
			{
				TeamTypes eMaster = getTeam();
				for (int iMaster = 0; iMaster < MAX_CIV_TEAMS; ++iMaster)
				{
					if (iMaster != getID() && kOurTeam.isVassal((TeamTypes)iMaster))
					{
						if (GET_TEAM((TeamTypes)iMaster).isVotingMember(eVoteSource))
						{
							eMaster = (TeamTypes)iMaster;
							break;
						}
					}
				}

				if (!GET_TEAM(eMaster).canContact(kPlayer.getTeam()))
				{
					return false;
				}
			}
		}
	}
	else if (kVote.isForceWar())
	{
		FAssert(NO_PLAYER != kData.ePlayer);
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);

		if (!kOurTeam.isAtWar(kPlayer.getTeam())
				&& kOurTeam.isFullMember(eVoteSource)) // dlph.25/advc
		{
			TeamTypes eMaster = getTeam();
			for (int iMaster = 0; iMaster < MAX_CIV_TEAMS; ++iMaster)
			{
				if (iMaster != getID() && kOurTeam.isVassal((TeamTypes)iMaster))
				{	// dlph.25/advc: was isVotingMember
					if (GET_TEAM((TeamTypes)iMaster).isFullMember(eVoteSource))
					{
						eMaster = (TeamTypes)iMaster;
						break;
					}
				}
			}
			if ((!GET_TEAM(eMaster).canDeclareWar(kPlayer.getTeam()) &&
					// <dlph.25/advc>
					g.getSecretaryGeneral(eVoteSource) == eMaster) ||
					!GET_TEAM(eMaster).canEventuallyDeclareWar(kPlayer.getTeam()))
					// </advc.25/advc>
				return false;
		}
	}
	else if (kVote.isForceNoTrade())
	{
		FAssert(NO_PLAYER != kData.ePlayer);
		CvPlayer& kPlayer = GET_PLAYER(kData.ePlayer);
		/*  <advc.130f> Don't allow players to propose resolutions that would cancel
			deals with turnsToCancel > 0 */
		if(g.getSecretaryGeneral(eVoteSource) == getID() &&
				isAnyDealTooRecentToCancel(kPlayer.getTeam()))
			return false; // </advc.130f>
		if (!canStopTradingWithTeam(kPlayer.getTeam(), true))
		{
			return false;
		}

	}
	else if (kVote.isAssignCity())
	{
		if (GET_TEAM(GET_PLAYER(kData.eOtherPlayer).getTeam()).isVassal(GET_PLAYER(kData.ePlayer).getTeam()))
		{
			return false;
		}
	} // <advc.178>
	else if(kVote.isVictory() && !GC.getGame().isDiploVictoryValid())
		return false; // </advc.178>
	return true;
}

bool CvPlayer::canDefyResolution(VoteSourceTypes eVoteSource, const VoteSelectionSubData& kData) const
{
	if (GC.getGame().getSecretaryGeneral(eVoteSource) == getTeam())
	{
		return false;
	}
	CvVoteInfo const& kVote = GC.getVoteInfo(kData.eVote); // advc.003
	// <dlph.25/advc> Kek-Mod just checks isAVassal
	if(GET_TEAM(getTeam()).isCapitulated()
			|| (isAVassal() && (kVote.isForceWar() || kVote.isForcePeace())))
		return false;
	// </dlph.25/advc>
	if (kVote.isOpenBorders())
	{
		for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
		{
			CvTeam& kTeam = GET_TEAM((TeamTypes)iTeam);
			if ((TeamTypes)iTeam != getTeam()) // kmodx: was (PlayerTypes)iTeam
			{
				if (kTeam.isVotingMember(eVoteSource))
				{
					if (!kTeam.isOpenBorders(getTeam()))
					{
						return true;
					}
				}
			}
		}
	}
	else if (kVote.isDefensivePact())
	{
		for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
		{
			CvTeam& kTeam = GET_TEAM((TeamTypes)iTeam);
			if ((TeamTypes)iTeam != getTeam()) // kmodx
			{
				if (kTeam.isVotingMember(eVoteSource))
				{
					if (!kTeam.isDefensivePact(getTeam()))
					{
						return true;
					}
				}
			}
		}
	}
	else if (kVote.isForceNoTrade())
	{
		return true;
	}
	else if (kVote.isForceWar())
	{
		if (!::atWar(getTeam(), TEAMID(kData.ePlayer))
				// dlph.25: 'Cannot defy war declaration against itself'
				&& TEAMREF(kData.ePlayer).getMasterTeam() != getMasterTeam()
				&& isFullMember(eVoteSource)) // advc
		{
			// BETTER_BTS_AI_MOD, 12/31/08, jdog5000: Vassals can't defy declarations of war
			//if (!GET_TEAM(getTeam()).isAVassal()) // dlph.25: Vassals already handled
			return true;
		}
	}
	else if (kVote.isForcePeace())
	{
		if (GET_PLAYER(kData.ePlayer).getTeam() == getTeam())
		{
			return true;
		}

		if (::atWar(getTeam(), GET_PLAYER(kData.ePlayer).getTeam()))
		{
			return true;
		}
	}
	else if (kVote.isAssignCity())
	{
		if (kData.ePlayer == getID()
				// dlph.25: 'You can defy resolution giving you a city'
				|| kData.eOtherPlayer == getID())
		{
			return true;
		}
	}
	else if (!GC.getGame().isTeamVote(kData.eVote))
	{
		return true;
	}

	return false;
}


void CvPlayer::setDefiedResolution(VoteSourceTypes eVoteSource, const VoteSelectionSubData& kData)
{
	FAssertMsg(canDefyResolution(eVoteSource, kData),
			"OK to fail when a team member defies a resolution"); // dlph.25

	// cities get unhappiness
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); NULL != pLoopCity; pLoopCity = nextCity(&iLoop))
	{
		ReligionTypes eReligion = GC.getGame().getVoteSourceReligion(eVoteSource);

		if (NO_RELIGION == eReligion || pLoopCity->isHasReligion(eReligion))
		{
			int iAngerLength = pLoopCity->flatDefyResolutionAngerLength();
			if (NO_RELIGION != eReligion && pLoopCity->isHasReligion(eReligion))
			{
				iAngerLength /= std::max(1, pLoopCity->getReligionCount());
			}

			pLoopCity->changeDefyResolutionAngerTimer(iAngerLength);
		}
	}

	setLoyalMember(eVoteSource, false);
}


void CvPlayer::setEndorsedResolution(VoteSourceTypes eVoteSource, const VoteSelectionSubData& kData)
{
	setLoyalMember(eVoteSource, true);
}

bool CvPlayer::isFullMember(VoteSourceTypes eVoteSource) const
{
	if (NO_RELIGION != GC.getGame().getVoteSourceReligion(eVoteSource))
	{
		if (getStateReligion() != GC.getGame().getVoteSourceReligion(eVoteSource))
		{
			return false;
		}
	}

	if (NO_CIVIC != GC.getVoteSourceInfo(eVoteSource).getCivic())
	{
		if (!isCivic((CivicTypes)GC.getVoteSourceInfo(eVoteSource).getCivic()))
		{
			return false;
		}
	}

	if (!isLoyalMember(eVoteSource))
	{
		return false;
	}

	return isVotingMember(eVoteSource);
}

bool CvPlayer::isVotingMember(VoteSourceTypes eVoteSource) const
{
	return (getVotes(NO_VOTE, eVoteSource) > 0);
}

PlayerTypes CvPlayer::pickConqueredCityOwner(const CvCity& kCity) const
{
	PlayerTypes eBestPlayer = kCity.getLiberationPlayer(true);

	if (NO_PLAYER != eBestPlayer)
	{
		if (GET_TEAM(getTeam()).isVassal(GET_PLAYER(eBestPlayer).getTeam()))
		{
			return eBestPlayer;
		}
	}

	return getID();
}

bool CvPlayer::canHaveTradeRoutesWith(PlayerTypes ePlayer) const
{
	CvPlayer& kOtherPlayer = GET_PLAYER(ePlayer);

	if (!kOtherPlayer.isAlive())
	{
		return false;
	}

	if (getTeam() == kOtherPlayer.getTeam())
	{
		return true;
	}
	// <advc.124>
	if(kOtherPlayer.isAnarchy())
		return false; // </advc.124>

	if (GET_TEAM(getTeam()).isFreeTrade(kOtherPlayer.getTeam()))
	{
		if (GET_TEAM(getTeam()).isVassal(kOtherPlayer.getTeam()))
		{
			return true;
		}

		if (GET_TEAM(kOtherPlayer.getTeam()).isVassal(getTeam()))
		{
			return true;
		}

		if (!isNoForeignTrade() && !kOtherPlayer.isNoForeignTrade())
		{
			return true;
		}
	}

	return false;
}

bool CvPlayer::canStealTech(PlayerTypes eTarget, TechTypes eTech) const
{
	if (TEAMREF(eTarget).isHasTech(eTech))
	{
		if (canResearch(eTech, /* K-Mod: */ false, true))
		{
			return true;
		}
	}

	return false;
}

bool CvPlayer::canForceCivics(PlayerTypes eTarget, CivicTypes eCivic) const
{	// return (GET_PLAYER(eTarget).canDoCivics(eCivic) && !GET_PLAYER(eTarget).isCivic(eCivic) && isCivic(eCivic));
	// <advc.132>
	if(!GET_PLAYER(eTarget).canDoCivics(eCivic) ||
			GET_PLAYER(eTarget).isCivic(eCivic) || !isCivic(eCivic))
		return false;
	/* Identify economy civics as those in the same column as a civic
	   granting extra trade routes (Free Market).
	   Religion civics are those that allow or disallow a state religion.
	   Marking "flexible" civics in XML (schema change) would be cleaner. */
	int iEconomyOption = -1;
	for(int i = 0; i < GC.getNumCivicInfos(); i++) {
		CvCivicInfo const& kLoopCivic = GC.getCivicInfo((CivicTypes)i);
		if(kLoopCivic.getTradeRoutes() != 0) {
			iEconomyOption = kLoopCivic.getCivicOptionType();
			break;
		}
	}
	CvCivicInfo const& kCivic = GC.getCivicInfo(eCivic);
	return (GC.getCivilizationInfo(getCivilizationType()). // Not an initial civic
			getCivilizationInitialCivics(kCivic.getCivicOptionType()) != eCivic &&
			(kCivic.isStateReligion() || kCivic.getNonStateReligionHappiness() > 0 ||
			kCivic.getCivicOptionType() == iEconomyOption));
}

bool CvPlayer::canForceReligion(PlayerTypes eTarget, ReligionTypes eReligion) const
{
	//return (GET_PLAYER(eTarget).canDoReligion(eReligion) && GET_PLAYER(eTarget).getStateReligion() != eReligion && getStateReligion() == eReligion);
	// K-Mod - You shouldn't be able to force a religion on an irreligious civ.
	//return (GET_PLAYER(eTarget).isStateReligion() && GET_PLAYER(eTarget).canDoReligion(eReligion) && GET_PLAYER(eTarget).getStateReligion() != eReligion && getStateReligion() == eReligion);
	// <advc.132> Rewritten based on the K-Mod condition (commented out above)
	CvPlayer const& kTarget = GET_PLAYER(eTarget);
	/*  Just the conditions from above. (Better use canConvert, which checks recent
		religion change? Would also have to use canRevolution then in canForceCivics.) */
	if(!kTarget.isStateReligion() || !kTarget.canDoReligion(eReligion) ||
			kTarget.getStateReligion() == eReligion)
		return false;
	if(getStateReligion() == eReligion)
		return true;
	// New: Accept any major religion
	return kTarget.isMajorReligion(eReligion); // </advc.132>
}

bool CvPlayer::canSpyDestroyUnit(PlayerTypes eTarget, CvUnit& kUnit) const
{
	if (kUnit.getTeam() == getTeam())
	{
		return false;
	}

	if (kUnit.getUnitInfo().getProductionCost() <= 0)
	{
		return false;
	}

	if (!kUnit.plot()->isVisible(getTeam(), false))
	{
		return false;
	}

	return true;
}

bool CvPlayer::canSpyBribeUnit(PlayerTypes eTarget, CvUnit& kUnit) const
{
	if (!canSpyDestroyUnit(eTarget, kUnit))
	{
		return false;
	}

	// Can't buy units when at war
	if (kUnit.isEnemy(getTeam()))
	{
		return false;
	}

	// Can't buy units if they are not in a legal plot
	if (!GET_TEAM(getTeam()).isFriendlyTerritory(GET_PLAYER(eTarget).getTeam()) && !GET_TEAM(getTeam()).isOpenBorders(GET_PLAYER(eTarget).getTeam()))
	{
		return false;
	}

	CLLNode<IDInfo>* pUnitNode = kUnit.plot()->headUnitNode();

	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = kUnit.plot()->nextUnitNode(pUnitNode);

		if (NULL != pLoopUnit && pLoopUnit != &kUnit)
		{
			if (pLoopUnit->isEnemy(getTeam()))
			{
				// If we buy the unit, we will be on the same plot as an enemy unit! Not good.
				return false;
			}
		}
	}

	return true;
}

bool CvPlayer::canSpyDestroyBuilding(PlayerTypes eTarget, BuildingTypes eBuilding) const
{
	CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	if (kBuilding.getProductionCost() <= 0)
	{
		return false;
	}

	if (::isLimitedWonderClass((BuildingClassTypes)kBuilding.getBuildingClassType()))
	{
		return false;
	}

	return true;
}

bool CvPlayer::canSpyDestroyProject(PlayerTypes eTarget, ProjectTypes eProject) const
{
	CvProjectInfo& kProject = GC.getProjectInfo(eProject);
	if (kProject.getProductionCost() <= 0)
	{
		return false;
	}

	if (GET_TEAM(GET_PLAYER(eTarget).getTeam()).getProjectCount(eProject) <= 0)
	{
		return false;
	}

	if (::isWorldProject(eProject))
	{
		return false;
	}

	if (!kProject.isSpaceship())
	{
		return false;
	}
	else
	{
		VictoryTypes eVictory = (VictoryTypes)kProject.getVictoryPrereq();
		if (NO_VICTORY != eVictory)
		{
			// Can't destroy spaceship components if we have already launched
			if (GET_TEAM(GET_PLAYER(eTarget).getTeam()).getVictoryCountdown(eVictory) >= 0)
			{
				return false;
			}
		}
	}

	return true;
}

// K-Mod
int CvPlayer::getEspionageGoldQuantity(EspionageMissionTypes eMission, PlayerTypes eTargetPlayer, const CvCity* pCity) const
{
	if (!pCity || pCity->getOwner() != eTargetPlayer)
		return 0;

	CvEspionageMissionInfo& kMission = GC.getEspionageMissionInfo(eMission);
	int iGoldStolen = (GET_PLAYER(eTargetPlayer).getGold() * kMission.getStealTreasuryTypes()) / 100;

	iGoldStolen *= 3*pCity->getPopulation();
	iGoldStolen /= std::max(1, pCity->getPopulation() + 2*GET_PLAYER(eTargetPlayer).getAveragePopulation());

	return std::min(iGoldStolen, GET_PLAYER(eTargetPlayer).getGold());
}

void CvPlayer::forcePeace(PlayerTypes ePlayer)
{
	/* original bts code
	if (!GET_TEAM(getTeam()).isAVassal()) {
		FAssert(GET_TEAM(getTeam()).canChangeWarPeace(GET_PLAYER(ePlayer).getTeam())); */

	// K-Mod. "canChangeWarPeace" can return false here if the peace team vassalates after the vote is cast.
	if (GET_TEAM(getTeam()).canChangeWarPeace(GET_PLAYER(ePlayer).getTeam()))
	{
	// K-Mod end
		CLinkList<TradeData> playerList;
		CLinkList<TradeData> loopPlayerList;
		TradeData kTradeData;
		setTradeItem(&kTradeData, TRADE_PEACE_TREATY);
		playerList.insertAtEnd(kTradeData);
		loopPlayerList.insertAtEnd(kTradeData);

		GC.getGame().implementDeal(getID(), ePlayer, &playerList, &loopPlayerList);
	}
}

// <advc.032>
bool CvPlayer::resetPeaceTreaty(PlayerTypes ePlayer) {

	CvGame const& g = GC.getGame(); int foo=-1;
	for(CvDeal* d = g.firstDeal(&foo); d != NULL; d = g.nextDeal(&foo)) {
		if(d->isBetween(getID(), ePlayer) && d->getFirstTrades() != NULL) {
			for(CLLNode<TradeData>* pNode = d->getFirstTrades()->head();
					pNode != NULL; pNode = d->getFirstTrades()->next(pNode)) {
				if(pNode->m_data.m_eItemType == TRADE_PEACE_TREATY) {
					d->setInitialGameTurn(g.getGameTurn());
					return true; // Assume that there is at most 1 peace treaty
				}
			}
		}
	}
	return false;
} // </advc.032>

bool CvPlayer::canSpiesEnterBorders(PlayerTypes ePlayer) const
{
	for (int iMission = 0; iMission < GC.getNumEspionageMissionInfos(); ++iMission)
	{
		if (GC.getEspionageMissionInfo((EspionageMissionTypes)iMission).isNoActiveMissions() && GC.getEspionageMissionInfo((EspionageMissionTypes)iMission).isPassive())
		{
			if (GET_PLAYER(ePlayer).canDoEspionageMission((EspionageMissionTypes)iMission, getID(), NULL, -1, NULL))
			{
				return false;
			}
		}
	}

	return true;
}

int CvPlayer::getReligionPopulation(ReligionTypes eReligion) const
{
	int iPopulation = 0;
	int iLoop;
	for (CvCity* pCity = firstCity(&iLoop); NULL != pCity; pCity = nextCity(&iLoop))
	{
		if (pCity->isHasReligion(eReligion))
		{
			iPopulation += pCity->getPopulation();
		}
	}

	return iPopulation;
}

int CvPlayer::getNewCityProductionValue() const
{
	int iValue = 0;
	for (int iJ = 0; iJ < GC.getNumBuildingClassInfos(); iJ++)
	{
		BuildingTypes eBuilding = ((BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iJ)));

		if (NO_BUILDING != eBuilding)
		{
			if(GC.getGame().isFreeStartEraBuilding(eBuilding)) // advc.003
			{
				iValue += (100 * getProductionNeeded(eBuilding)) /
						std::max(1, 100 + getProductionModifier(eBuilding));
			}
		}
	}

	iValue *= 100 + GC.getDefineINT("NEW_CITY_BUILDING_VALUE_MODIFIER");
	iValue /= 100;

	iValue += (GC.getDefineINT("ADVANCED_START_CITY_COST") * GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getGrowthPercent()) / 100;

	int iPopulation = GC.getDefineINT("INITIAL_CITY_POPULATION") + GC.getEraInfo(GC.getGame().getStartEra()).getFreePopulation();
	for (int i = 1; i <= iPopulation; ++i)
	{
		iValue += (getGrowthThreshold(i) * GC.getDefineINT("ADVANCED_START_POPULATION_COST")) / 100;
	}
	// <advc.251>
	iValue = ::roundToMultiple(iValue *
			// Apply production modifier half (b/c getGrowthThreshold is also dependent on handicap)
			(1 + 0.5 * (trainingModifierFromHandicap(false) - 1)), isHuman() ? 5 : 1);
	// </advc.251>
	return iValue;
}

int CvPlayer::getGrowthThreshold(int iPopulation) const
{
	CvGame const& g = GC.getGame(); // advc.003
	// <advc.251>
	int iBaseThreshold = GC.getDefineINT("BASE_CITY_GROWTH_THRESHOLD");
	iBaseThreshold = ::round(0.01 * iBaseThreshold * GC.getHandicapInfo(
			getHandicapType()).getBaseGrowthThresholdPercent());
	int iThreshold = iBaseThreshold + // </advc.251>
			(iPopulation * GC.getDefineINT("CITY_GROWTH_MULTIPLIER"));
	// <advc.251>
	int iAIModifier = 100;
	if(!isHuman()) { // Also apply it to Barbarians
		CvHandicapInfo const& h = GC.getHandicapInfo(g.getHandicapType());
		iAIModifier = h.getAIGrowthPercent() +
				//h.getAIPerEraModifier() * getCurrentEra()
				g.AIHandicapAdjustment();
	} // Reduce rounding error:
	iThreshold = ::round(iThreshold * 0.01 * iAIModifier *
			0.01 * GC.getGameSpeedInfo(g.getGameSpeedType()).getGrowthPercent() *
			0.01 * GC.getEraInfo(g.getStartEra()).getGrowthPercent());
	// </advc.251>
	return std::max(1, iThreshold);
}

void CvPlayer::verifyUnitStacksValid()
{
	int iLoop;
	for(CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		pLoopUnit->verifyStackValid();
	}
}

UnitTypes CvPlayer::getTechFreeUnit(TechTypes eTech) const
{
	UnitClassTypes eUnitClass = (UnitClassTypes) GC.getTechInfo(eTech).getFirstFreeUnitClass();
	if (eUnitClass == NO_UNITCLASS)
	{
		return NO_UNIT;
	}

	UnitTypes eUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(eUnitClass)));
	if (eUnit == NO_UNIT)
	{
		return NO_UNIT;
	}

	if (GC.getUnitInfo(eUnit).getEspionagePoints() > 0 && GC.getGame().isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		return NO_UNIT;
	}

	return eUnit;
}

// BULL - Trade Hover - start  // advc: style changes, _MOD_FRACTRADE removed
/*  Adds the yield and count for each trade route with eWithPlayer to the
	int references (out parameters). */
void CvPlayer::calculateTradeTotals(YieldTypes eIndex,
		int& iDomesticYield, int& iDomesticRoutes, int& iForeignYield, int& iForeignRoutes,
		PlayerTypes eWithPlayer) const {

	int foo;
	for(CvCity* pCity = firstCity(&foo); NULL != pCity; pCity = nextCity(&foo)) {
		pCity->calculateTradeTotals(eIndex, iDomesticYield, iDomesticRoutes,
				iForeignYield, iForeignRoutes, eWithPlayer);
	}
} // BULL - Trade Hover - end

// <advc.085>
void CvPlayer::setScoreboardExpanded(bool b) {

	m_bScoreboardExpanded = b;
	if(b) {
		FAssert(getBugOptionBOOL("Scores__AlignIcons", true, false));
		if(getBugOptionBOOL("Scores__ExpandOnHover", false, false))
			gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
		else m_bScoreboardExpanded = false;
	}
	/*  If !b, then it doesn't help to dirty it b/c the update is still in progress
		(when called from Python). */
	else {
		/*  0 would make the scoreboard collapse almost instantly when the mouse is
			moved away. Can't click buttons that way. A higher delay than 1
			(1/4 second) shouldn't be necessary. */
		int const iDelay = 1;
		GC.getGame().setUpdateTimer(CvGame::UPDATE_SCORE_BOARD_DIRTY, iDelay);
	}
}


bool CvPlayer::isScoreboardExpanded() const {

	return m_bScoreboardExpanded;
} // </advc.085>


void CvPlayer::buildTradeTable(PlayerTypes eOtherPlayer, CLinkList<TradeData>& ourList) const
{
	PROFILE_FUNC(); // advc.003b
	TradeData item;
	int iLoop;
	bool const bOtherHuman = GET_PLAYER(eOtherPlayer).isHuman(); // advc.003b
	//	Put the gold and maps into the table
	setTradeItem(&item, TRADE_GOLD);
	if (canTradeItem(eOtherPlayer, item))
	{
		ourList.insertAtEnd(item);
	}

	//	Gold per turn
	setTradeItem(&item, TRADE_GOLD_PER_TURN);
	if (canTradeItem(eOtherPlayer, item))
	{
		ourList.insertAtEnd(item);
	}

	//	Maps
	setTradeItem(&item, TRADE_MAPS, 0);
	if (canTradeItem(eOtherPlayer, item))
	{
		ourList.insertAtEnd(item);
	}

	//	Vassal
	setTradeItem(&item, TRADE_VASSAL, 0);
	if (canTradeItem(eOtherPlayer, item))
	{
		ourList.insertAtEnd(item);
	}

	//	Open Borders
	setTradeItem(&item, TRADE_OPEN_BORDERS);
	if (canTradeItem(eOtherPlayer, item))
	{
		ourList.insertAtEnd(item);
	}

	//	Defensive Pact
	setTradeItem(&item, TRADE_DEFENSIVE_PACT);
	if (canTradeItem(eOtherPlayer, item))
	{
		ourList.insertAtEnd(item);
	}

	//	Permanent Alliance
	setTradeItem(&item, TRADE_PERMANENT_ALLIANCE);
	if (canTradeItem(eOtherPlayer, item))
	{
		ourList.insertAtEnd(item);
	}

	if (::atWar(getTeam(), GET_PLAYER(eOtherPlayer).getTeam()))
	{
		//	We are at war, allow a peace treaty option
		setTradeItem(&item, TRADE_PEACE_TREATY);
		ourList.insertAtEnd(item);

		//	Capitulation
		setTradeItem(&item, TRADE_SURRENDER, 0);
		if (canTradeItem(eOtherPlayer, item))
		{
			ourList.insertAtEnd(item);
		}
	}

	//	Initial build of the inventory lists and buttons.
	//	Go through all the possible headings
	for (int i = NUM_BASIC_ITEMS; i < NUM_TRADEABLE_HEADINGS; i++)
	{
		bool bFoundItemUs = false;

		//	Build what we need to build for this item
		switch (i)
		{
		case TRADE_TECHNOLOGIES:
			for (int j = 0; j < GC.getNumTechInfos(); j++)
			{
				setTradeItem(&item, TRADE_TECHNOLOGIES, j);
				if (canTradeItem(eOtherPlayer, item))
				{
					bFoundItemUs = true;
					ourList.insertAtEnd(item);
				}
			}
			break;

		case TRADE_RESOURCES:
			for (int j = 0; j < GC.getNumBonusInfos(); j++)
			{
				setTradeItem(&item, TRADE_RESOURCES, j);
				if (!canTradeItem(eOtherPlayer, item))
					continue;
				// <advc.074>
				bool bHuman = (bOtherHuman || isHuman());
				bool bValid = (!bHuman || getTradeDenial(eOtherPlayer, item) != DENIAL_JOKING);
				/*  Hack: Check if we're expecting a renegotiate-popup from the EXE.
					Don't want any resources in the canceled deal to be excluded
					from the trade table. */
				if(!bValid && !GC.getGame().isInBetweenTurns() &&
					// Probably too complicated to get this right with simultaneous turns
					!GC.getGame().isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
				{
					for(CLLNode<std::pair<PlayerTypes,BonusTypes> >* pNode =
							m_cancelingExport.head(); pNode != NULL; pNode =
							m_cancelingExport.next(pNode)) {
						PlayerTypes eCancelPlayer = pNode->m_data.first;
						BonusTypes eCancelBonus = pNode->m_data.second;
						FAssert(eCancelPlayer != getID());
						if(eCancelPlayer == eOtherPlayer && eCancelBonus == j) {
							bValid = true;
							const_cast<CvPlayer*>(this)->m_cancelingExport.deleteNode(pNode);
							break;
						}
					}
				}
				if(bValid) { // </advc.074>
					bFoundItemUs = true;
					ourList.insertAtEnd(item);
				}
			}
			break;

		case TRADE_CITIES:
			for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
			{
				//if (AI_cityTrade(pLoopCity, eOtherPlayer) != DENIAL_NEVER) // K-Mod
				/*  advc.003b: Replace the above b/c TradeDenial tests are slower
					than canTradeItem (though in this case the difference should be negligible). */
				setTradeItem(&item, TRADE_CITIES, pLoopCity->getID());
				if (canTradeItem(eOtherPlayer, item))
				{	// <advc.003b>
					if(!bOtherHuman || getTradeDenial(eOtherPlayer, item)
							!= DENIAL_NEVER) { // </advc.003b>
						bFoundItemUs = true;
						ourList.insertAtEnd(item);
					}
				}
			}
			break;

		case TRADE_PEACE:
			if (!isHuman())
			{
				for (int j = 0; j < MAX_CIV_TEAMS; j++)
				{
					if (GET_TEAM((TeamTypes)j).isAlive())
					{
						if (j != getTeam() && j != GET_PLAYER(eOtherPlayer).getTeam())
						{
							setTradeItem(&item, TRADE_PEACE, j);
							if (canTradeItem(eOtherPlayer, item))
							{
								ourList.insertAtEnd(item);
								bFoundItemUs = true;
							}
						}
					}
				}
			}
			break;

		case TRADE_WAR:
			if (!isHuman())
			{
				for (int j = 0; j < MAX_CIV_TEAMS; j++)
				{
					if (GET_TEAM((TeamTypes)j).isAlive())
					{
						if (j != getTeam() && j != GET_PLAYER(eOtherPlayer).getTeam())
						{
							setTradeItem(&item, TRADE_WAR, j);
							if (canTradeItem(eOtherPlayer, item))
							{
								ourList.insertAtEnd(item);
								bFoundItemUs = true;
							}
						}
					}
				}
			}
			break;

		case TRADE_EMBARGO:
			if (!isHuman())
			{
				for (int j = 0; j < MAX_CIV_TEAMS; j++)
				{
					if (GET_TEAM((TeamTypes)j).isAlive())
					{
						if (j != getTeam() && j != GET_PLAYER(eOtherPlayer).getTeam())
						{
							setTradeItem(&item, TRADE_EMBARGO, j);
							if (canTradeItem(eOtherPlayer, item))
							{
								ourList.insertAtEnd(item);
								bFoundItemUs = true;
							}
						}
					}
				}
			}
			break;

		case TRADE_CIVIC:
			for (int j = 0; j < GC.getNumCivicInfos(); j++)
			{
				setTradeItem(&item, TRADE_CIVIC, j);
				if (canTradeItem(eOtherPlayer, item))
				{	// <advc.074>
					if((!bOtherHuman && !isHuman()) ||
						getTradeDenial(eOtherPlayer, item) != DENIAL_JOKING) {
					// </advc.074>
						bFoundItemUs = true;
						ourList.insertAtEnd(item);
					}
				}
			}
			break;

		case TRADE_RELIGION:
			for (int j = 0; j < GC.getNumReligionInfos(); j++)
			{
				setTradeItem(&item, TRADE_RELIGION, j);
				if (canTradeItem(eOtherPlayer, item))
				{
					bFoundItemUs = true;
					ourList.insertAtEnd(item);
				}
			}
			break;
		}
	}
}

bool CvPlayer::getHeadingTradeString(PlayerTypes eOtherPlayer, TradeableItems eItem, CvWString& szString, CvString& szIcon) const
{
	szIcon.clear();

	switch (eItem)
	{
	case TRADE_TECHNOLOGIES:
		szString = gDLL->getText("TXT_KEY_CONCEPT_TECHNOLOGY");
		break;

	case TRADE_RESOURCES:
		szString = gDLL->getText("TXT_KEY_TRADE_RESOURCES");
		break;

	case TRADE_CITIES:
		szString = gDLL->getText("TXT_KEY_TRADE_CITIES");
		break;

	case TRADE_PEACE:
		szString = gDLL->getText("TXT_KEY_TRADE_MAKE_PEACE_WITH");
		break;

	case TRADE_WAR:
		szString = gDLL->getText("TXT_KEY_TRADE_DECLARE_WAR_ON");
		break;

	case TRADE_EMBARGO:
		szString = gDLL->getText("TXT_KEY_TRADE_STOP_TRADING_WITH");
		break;

	case TRADE_CIVIC:
		szString = gDLL->getText("TXT_KEY_TRADE_ADOPT");
		break;

	case TRADE_RELIGION:
		szString = gDLL->getText("TXT_KEY_TRADE_CONVERT");
		break;
	default:
		szString.clear();
		return false;
		break;
	}

	return true;
}


bool CvPlayer::getItemTradeString(PlayerTypes eOtherPlayer, bool bOffer,
		bool bShowingCurrent, const TradeData& zTradeData,
		CvWString& szString, CvString& szIcon) const
{
	szIcon.clear();
	// <advc.072>
	CvDeal* pDeal = NULL;
	if(bShowingCurrent) {
		int iTurnsLeftMode = getBugOptionINT("Advisors__DealTurnsLeft", 3);
		if(iTurnsLeftMode == 2 || iTurnsLeftMode == 1) {
			TradeableItems eItemType = zTradeData.m_eItemType;
			if(CvDeal::isAnnual(eItemType) || eItemType == TRADE_PEACE_TREATY) {
				pDeal = GC.getGame().nextCurrentDeal(eOtherPlayer, getID(),
						eItemType, zTradeData.m_iData);
				/*  Call nextCurrentDeal for all annual deals plus peace treaties
					in order to stay in-sync with the iteration done by the EXE,
					but we're only interested in a subset here: */
				if(!CvDeal::isDual(eItemType) && eItemType != TRADE_RESOURCES &&
						eItemType != TRADE_GOLD_PER_TURN)
					pDeal = NULL;
			}
		}
	} // </advc.072>
	switch (zTradeData.m_eItemType)
	{
	case TRADE_GOLD:
		if (bOffer)
		{
			szString = gDLL->getText("TXT_KEY_TRADE_GOLD_NUM", zTradeData.m_iData);
		}
		else
		{
			szString = gDLL->getText("TXT_KEY_TRADE_GOLD_NUM", AI_maxGoldTrade(eOtherPlayer));
		}
		break;
	case TRADE_GOLD_PER_TURN:
		if (bOffer)
		{
			szString = gDLL->getText("TXT_KEY_TRADE_GOLD_PER_TURN_NUM", zTradeData.m_iData);
		}
		else
		{
			szString = gDLL->getText("TXT_KEY_TRADE_GOLD_PER_TURN_NUM", AI_maxGoldPerTurnTrade(eOtherPlayer));
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
		if(TEAMREF(eOtherPlayer).isAtWar(getTeam())) { // advc.072
			szString = gDLL->getText("TXT_KEY_TRADE_PEACE_TREATY_STRING",
					GC.getPEACE_TREATY_LENGTH());
		}
		else szString = gDLL->getText("TXT_KEY_TRADE_PEACE_TREATY_STR"); // advc.072
		break;
	case TRADE_TECHNOLOGIES:
		szString = GC.getTechInfo((TechTypes)zTradeData.m_iData).getDescription();
		szIcon = GC.getTechInfo((TechTypes)zTradeData.m_iData).getButton();
		break;
	case TRADE_RESOURCES:
		if (bOffer)
		{
			int iNumResources = GET_PLAYER(eOtherPlayer).getNumTradeableBonuses((BonusTypes)zTradeData.m_iData);
			if (bShowingCurrent)
			{
				++iNumResources;
			}
			szString = gDLL->getText("TXT_KEY_TRADE_RESOURCE", GC.getBonusInfo((BonusTypes)zTradeData.m_iData).getDescription(), iNumResources);

		}
		else
		{
			szString.Format( L"%s (%d)", GC.getBonusInfo((BonusTypes)zTradeData.m_iData).getDescription(), getNumTradeableBonuses((BonusTypes)zTradeData.m_iData));
		}
		szIcon = GC.getBonusInfo((BonusTypes)zTradeData.m_iData).getButton();
		break;
	case TRADE_CITIES:
		{
			CvCity* pCity = NULL;
			if (bOffer)
			{
				pCity = GET_PLAYER(eOtherPlayer).getCity(zTradeData.m_iData);
			}
			else
			{
				pCity = getCity(zTradeData.m_iData);
			}
			if (NULL != pCity)
			{
				if (pCity->getLiberationPlayer(false) == eOtherPlayer)
				{
					szString.Format(L"%s (%s)", pCity->getName().GetCString(), gDLL->getText("TXT_KEY_LIBERATE_CITY").GetCString());
				}
				else
				{
					szString = gDLL->getText("TXT_KEY_CITY_OF", pCity->getNameKey());
				}
			}
		}
		break;
	case TRADE_PEACE:
		if (bOffer)
		{
			szString = gDLL->getText("TXT_KEY_TRADE_PEACE_WITH");
			szString += GET_TEAM((TeamTypes)zTradeData.m_iData).getName();
		}
		else
		{
			szString = GET_TEAM((TeamTypes)zTradeData.m_iData).getName();
		}
		break;
	case TRADE_WAR:
		if (bOffer)
		{
			szString = gDLL->getText("TXT_KEY_TRADE_WAR_WITH");
			szString += GET_TEAM((TeamTypes)zTradeData.m_iData).getName();
		}
		else
		{
			szString = GET_TEAM((TeamTypes)zTradeData.m_iData).getName();
		}
		break;
	case TRADE_EMBARGO:
		if (bOffer)
		{
			szString = gDLL->getText("TXT_KEY_TRADE_STOP_TRADING_WITH");
			szString += L" " + GET_TEAM((TeamTypes)zTradeData.m_iData).getName();
		}
		else
		{
			szString = GET_TEAM((TeamTypes)zTradeData.m_iData).getName();
		}
		break;
	case TRADE_CIVIC:
		if (bOffer)
		{
			szString = gDLL->getText("TXT_KEY_TRADE_ADOPT");
			szString += GC.getCivicInfo((CivicTypes)zTradeData.m_iData).getDescription();
		}
		else
		{
			szString = GC.getCivicInfo((CivicTypes)zTradeData.m_iData).getDescription();
		}
		szIcon = GC.getCivicInfo((CivicTypes)zTradeData.m_iData).getButton();
		break;
	case TRADE_RELIGION:
		if (bOffer)
		{
			szString = gDLL->getText("TXT_KEY_TRADE_CONVERT");
			szString += GC.getReligionInfo((ReligionTypes)zTradeData.m_iData).getDescription();
		}
		else
		{
			szString = GC.getReligionInfo((ReligionTypes)zTradeData.m_iData).getDescription();
		}
		szIcon = GC.getReligionInfo((ReligionTypes)zTradeData.m_iData).getButton();
		break; // <advc.034>
	case TRADE_DISENGAGE:
		szString.clear();
		GAMETEXT.buildDisengageString(szString, getID(), eOtherPlayer);
		break; // </advc.034>
	default:
		szString.clear();
		return false;
	}
	// <advc.072>
	if(pDeal != NULL && pDeal->turnsToCancel() > 0) {
		szString.append(L" ");
		szString.append(gDLL->getText("INTERFACE_CITY_TURNS",
				pDeal->turnsToCancel()).GetCString());
	} // </advc.072>
	return true;
}

void CvPlayer::updateTradeList(PlayerTypes eOtherPlayer, CLinkList<TradeData>& ourInventory, const CLinkList<TradeData>& ourOffer, const CLinkList<TradeData>& theirOffer) const
{
	for (CLLNode<TradeData>* pNode = ourInventory.head(); pNode != NULL; pNode = ourInventory.next(pNode))
	{
		pNode->m_data.m_bHidden = false;

		// Don't show peace treaties when not at war
		if (!::atWar(getTeam(), GET_PLAYER(eOtherPlayer).getTeam()))
		{
			if (pNode->m_data.m_eItemType == TRADE_PEACE_TREATY || pNode->m_data.m_eItemType == TRADE_SURRENDER)
			{
				pNode->m_data.m_bHidden = true;
			}
		}

		// Don't show technologies with no tech trading game option
		if (GC.getGame().isOption(GAMEOPTION_NO_TECH_TRADING) && pNode->m_data.m_eItemType == TRADE_TECHNOLOGIES)
		{
			pNode->m_data.m_bHidden = true;
		}
	}

	for (CLLNode<TradeData>* pNode = ourInventory.head(); pNode != NULL; pNode = ourInventory.next(pNode))
	{
		switch (pNode->m_data.m_eItemType)
		{
		case TRADE_PEACE_TREATY:
			for (CLLNode<TradeData>* pOfferNode = ourOffer.head(); pOfferNode != NULL; pOfferNode = ourOffer.next(pOfferNode))
			{
				// Don't show vassal deals if peace treaty is already on the table
				if (CvDeal::isVassal(pOfferNode->m_data.m_eItemType))
				{
					pNode->m_data.m_bHidden = true;
					break;
				}
			}
			break;
		case TRADE_VASSAL:
		case TRADE_SURRENDER:
			for (CLLNode<TradeData>* pOfferNode = theirOffer.head(); pOfferNode != NULL; pOfferNode = theirOffer.next(pOfferNode))
			{
				// Don't show vassal deals if another type of vassal deal is on the table
				if (CvDeal::isVassal(pOfferNode->m_data.m_eItemType))
				{
					pNode->m_data.m_bHidden = true;
					break;
				}
			}

			if (!pNode->m_data.m_bHidden)
			{
				for (CLLNode<TradeData>* pOfferNode = ourOffer.head(); pOfferNode != NULL; pOfferNode = ourOffer.next(pOfferNode))
				{
					// Don't show peace deals if the other player is offering to be a vassal
					if (CvDeal::isEndWar(pOfferNode->m_data.m_eItemType))
					{
						pNode->m_data.m_bHidden = true;
						break;
					}
				}
			}
			break;
		default:
			break;
		}
	}

	if (!isHuman() || !GET_PLAYER(eOtherPlayer).isHuman())  // everything allowed in human-human trades
	{
		CLLNode<TradeData>* pFirstOffer = ourOffer.head();
		if (pFirstOffer == NULL)
		{
			pFirstOffer = theirOffer.head();
		}

		if (pFirstOffer != NULL)
		{
			//if (!CvDeal::isEndWar(pFirstOffer->m_data.m_eItemType) || !::atWar(getTeam(), GET_PLAYER(eOtherPlayer).getTeam()))
			if (!::atWar(getTeam(), GET_PLAYER(eOtherPlayer).getTeam())) // K-Mod
			{
				for (CLLNode<TradeData>* pNode = ourInventory.head(); pNode != NULL; pNode = ourInventory.next(pNode))
				{	// advc.122: Allow cities to be bought
					/*if (pFirstOffer->m_data.m_eItemType == TRADE_CITIES || pNode->m_data.m_eItemType == TRADE_CITIES)
						pNode->m_data.m_bHidden = true;
					else*/
// advc.tmp:
if(pFirstOffer->m_data.m_eItemType == TRADE_CITIES || pNode->m_data.m_eItemType == TRADE_CITIES) pNode->m_data.m_bHidden = true; else
					if (CvDeal::isAnnual(pFirstOffer->m_data.m_eItemType) != CvDeal::isAnnual(pNode->m_data.m_eItemType))
					{
						pNode->m_data.m_bHidden = true;
					}
				}
			}
		}
	}
}

// K-Mod. Find each item from the offer list in the inventory list - mark it as m_bOffering == true.
// (This is usually done somewhere in the game engine, or when the offer list is being generated or something.)
void CvPlayer::markTradeOffers(CLinkList<TradeData>& ourInventory, const CLinkList<TradeData>& ourOffer) const
{
	for (CLLNode<TradeData>* pOfferNode = ourOffer.head(); pOfferNode != NULL; pOfferNode = ourOffer.next(pOfferNode))
	{
		CLLNode<TradeData>* pInvNode; // (defined here just for the assertion at the end)
		for (pInvNode = ourInventory.head(); pInvNode != NULL; pInvNode = ourInventory.next(pInvNode))
		{
			if (pInvNode->m_data.m_eItemType == pOfferNode->m_data.m_eItemType &&
				pInvNode->m_data.m_iData == pOfferNode->m_data.m_iData)
			{
				pInvNode->m_data.m_bOffering = pOfferNode->m_data.m_bOffering = true;
				break;
			}
		}
		FAssertMsg(pInvNode, "failed to find offered item in inventory");
	}
}
// K-Mod end

int CvPlayer::getIntroMusicScriptId(PlayerTypes eForPlayer) const
{
	CvPlayer& kForPlayer = GET_PLAYER(eForPlayer);
	EraTypes eEra = kForPlayer.getCurrentEra();
	CvLeaderHeadInfo& kLeader = GC.getLeaderHeadInfo(getPersonalityType());
	if (GET_TEAM(kForPlayer.getTeam()).isAtWar(getTeam()))
	{
		return kLeader.getDiploWarIntroMusicScriptIds(eEra);
	}
	else
	{
		return kLeader.getDiploPeaceIntroMusicScriptIds(eEra);
	}
}

int CvPlayer::getMusicScriptId(PlayerTypes eForPlayer) const
{
	CvPlayer& kForPlayer = GET_PLAYER(eForPlayer);
	EraTypes eEra = kForPlayer.getCurrentEra();
	CvLeaderHeadInfo& kLeader = GC.getLeaderHeadInfo(getLeaderType());
	if (GET_TEAM(kForPlayer.getTeam()).isAtWar(getTeam()))
	{
		return kLeader.getDiploWarMusicScriptIds(eEra);
	}
	else
	{
		return kLeader.getDiploPeaceMusicScriptIds(eEra);
	}
}


void CvPlayer::getGlobeLayerColors(GlobeLayerTypes eGlobeLayerType, int iOption, std::vector<NiColorA>& aColors, std::vector<CvPlotIndicatorData>& aIndicators) const
{
	PROFILE_FUNC(); // advc.003b
	CvGame const& g = GC.getGame();
	/*  <advc.003> These get cleared by some of the subroutines, but should be
		empty to begin with. If not, there could be a memory leak. */
	FAssert(aColors.empty() && aIndicators.empty());
	FAssert(getID() == g.getActivePlayer()); // </advc.003>
	// <advc.706>
	if(g.isOption(GAMEOPTION_RISE_FALL) && CvPlot::isAllFog())
		return; // </advc.706>
	switch (eGlobeLayerType)
	{
	case GLOBE_LAYER_TRADE:
		getTradeLayerColors(aColors, aIndicators);
		break;
	case GLOBE_LAYER_UNIT:
		getUnitLayerColors((GlobeLayerUnitOptionTypes) iOption, aColors, aIndicators);
		break;
	case GLOBE_LAYER_RESOURCE:
		getResourceLayerColors((GlobeLayerResourceOptionTypes) iOption, aColors, aIndicators);
		break;
	case GLOBE_LAYER_RELIGION:
		getReligionLayerColors((ReligionTypes) iOption, aColors, aIndicators);
		break;
	case GLOBE_LAYER_CULTURE:
		getCultureLayerColors(aColors, aIndicators);
		break;
	default:
		FAssertMsg(false, "Unknown globe layer type");
		break;
	}
}

void CvPlayer::getTradeLayerColors(std::vector<NiColorA>& aColors, std::vector<CvPlotIndicatorData>& aIndicators) const
{
	CvMap const& m = GC.getMap();
	aColors.resize(m.numPlots(), NiColorA(0, 0, 0, 0));
	aIndicators.clear();
	// <advc.004z>
	bool bShowForeign = (GC.getDefineINT("FOREIGN_GROUPS_ON_TRADE_LAYER") > 0);
	bool bShowCapitalConn = (GC.getDefineINT("CONNECTION_TO_CAPITAL_ON_TRADE_LAYER") > 0);
	// </advc.004z>
	typedef std::map< int, std::vector<int> > PlotGroupMap;
	PlotGroupMap mapPlotGroups;
	std::vector<int> aiNotConn; // advc.004z
	for(int iI = 0; iI < m.numPlots(); iI++) {
		CvPlot* pLoopPlot = m.plotByIndex(iI);
		PlayerTypes ownerId = pLoopPlot->getOwner(); // advc.004z
		CvPlotGroup* pPlotGroup = pLoopPlot->getPlotGroup(getID());
		if (pPlotGroup != NULL && pLoopPlot->isRevealed(getTeam(), true) &&
				(pLoopPlot->getTeam() == getTeam()
				// <advc.004z>
				|| (bShowForeign && !pLoopPlot->isImpassable() &&
				ownerId != BARBARIAN_PLAYER &&
				// Get rid of insignificant groups
				(pPlotGroup->getLengthPlots() >= 5 ||
				!pLoopPlot->isWater() || ownerId != NO_PLAYER)))) {
			if(bShowCapitalConn && pLoopPlot->isVisible(getTeam(), false) &&
					pLoopPlot->isCity() && !pLoopPlot->getPlotCity()->
					isConnectedToCapital())
				aiNotConn.push_back(iI);
			else // </advc.004z>
				mapPlotGroups[pPlotGroup->getID()].push_back(iI);
		}
	} // <advc.004z> Use the player color for the plot group of the capital
	CvPlotGroup* pCapitalGroup = (getCapitalCity() == NULL ? NULL :
			getCapitalCity()->plot()->getPlotGroup(getID()));
	if(pCapitalGroup != NULL) {
		FAssert(pCapitalGroup->getLengthPlots() > 0);
		int iCount = mapPlotGroups.count(pCapitalGroup->getID());
		if(iCount > 0) {
			NiColorA kColor(getPlayerTextColorR() / 255.f,
					getPlayerTextColorG() / 255.f,
					getPlayerTextColorB() / 255.f,
					getPlayerTextColorA() / 255.f * (bShowForeign ? 0.5f : 0.8f));
			std::vector<int>& aPlots = mapPlotGroups[pCapitalGroup->getID()];
			for(size_t i = 0; i < aPlots.size(); i++)
				aColors[aPlots[i]] = kColor;
		}
		else FAssert(iCount > 0);
	} // </advc.004z>
	CvRandom kRandom;
	kRandom.init(42);
	for (PlotGroupMap::iterator it = mapPlotGroups.begin(); it != mapPlotGroups.end(); ++it)
	{	// <advc.004z> Already handled above
		if(pCapitalGroup != NULL && it->first == pCapitalGroup->getID())
			continue; // </advc.004z>
		NiColorA kColor(kRandom.getFloat(), kRandom.getFloat(), kRandom.getFloat(),
				// advc.004z: Can't tell apart land and water at 0.8
				bShowForeign ? 0.5f :
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
	for(size_t i = 0; i < aiNotConn.size(); i++)
		aColors[aiNotConn[i]] = kColor; // </advc.004z>
}

void CvPlayer::getUnitLayerColors(GlobeLayerUnitOptionTypes eOption, std::vector<NiColorA>& aColors, std::vector<CvPlotIndicatorData>& aIndicators) const
{
	// <advc.004z>
	if(eOption == 2) // Somehow, eOption==GLOBE_LAYER_UNIT_DUMMY doesn't work.
		eOption = SHOW_ALL_MILITARY;
	// </advc.004z>
	CvMap const& m = GC.getMap();
	aColors.resize(m.numPlots(), NiColorA(0, 0, 0, 0));
	aIndicators.clear();

	std::vector< std::vector<float> > aafPlayerPlotStrength(MAX_PLAYERS);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (GET_PLAYER((PlayerTypes)i).isAlive())
		{
			aafPlayerPlotStrength[i].resize(m.numPlots());
		}
	}

	float fMaxPlotStrength = 0.0f;

	// create unit plot indicators...
	// build the trade group texture
	typedef std::map<int, NiColor> GroupMap;
	GroupMap mapColors;

	// Loop through all the players  (advc.003: reduced indentation in the loops)
	CvWStringBuffer szBuffer;
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
	{
		if (!GET_PLAYER((PlayerTypes)iPlayer).isAlive())
			continue;
		for (int iI = 0; iI < m.numPlots(); ++iI)
		{
			CvPlot* pLoopPlot = m.plotByIndex(iI);
			int iNumUnits = pLoopPlot->getNumUnits();
			if (iNumUnits <= 0 || !pLoopPlot->isVisible(getTeam(), true))
				continue;

			float fPlotStrength = 0.0f;
			bool bShowIndicator = false;

			CLLNode<IDInfo>* pUnitNode = pLoopPlot->headUnitNode();
			while (pUnitNode != NULL)
			{
				CvUnit* pUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);
				if (pUnit->getVisualOwner() != iPlayer ||
						pUnit->isInvisible(getTeam(), true))
					continue;

				// now, is this unit of interest?
				bool bMilitary = pUnit->baseCombatStr() > 0;
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
						fPlotStrength += ((float) pUnit->currHitPoints() / (float) pUnit->maxHitPoints() * (float) pUnit->baseCombatStr());
					}
					break;
				}
				case SHOW_TEAM_MILITARY:
				{
					bOfInterest = bMilitary && bOnOurTeam;
					if (bOfInterest)
						fPlotStrength += ((float) pUnit->currHitPoints() / (float) pUnit->maxHitPoints() * (float) pUnit->baseCombatStr());
					break;
				}
				case SHOW_ENEMIES:
				{
					bOfInterest = bMilitary && bEnemy;
					if (bOfInterest)
						fPlotStrength += ((float) pUnit->currHitPoints() / (float) pUnit->maxHitPoints() * (float) pUnit->baseCombatStr());
					break;
				}
				case SHOW_ENEMIES_IN_TERRITORY:
				{
					bOfInterest = bMilitary;
					break;
				}
				case SHOW_PLAYER_DOMESTICS:
				{
					bOfInterest = !bMilitary;// && (pUnit->getVisualOwner() == eCurPlayer);
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
				aafPlayerPlotStrength[iPlayer][iI] = fPlotStrength;
			}

			if (bShowIndicator)
			{	// <advc.004z> Don't show a defender when we need !bMilitary
				CvUnit* pUnit = NULL;
				if(eOption == SHOW_PLAYER_DOMESTICS) {
					UnitAITypes priorityList[] = {
						UNITAI_GENERAL, UNITAI_ARTIST, UNITAI_ENGINEER,
						UNITAI_MERCHANT, UNITAI_GREAT_SPY, UNITAI_PROPHET,
						UNITAI_SCIENTIST, UNITAI_SETTLE, UNITAI_SPY,
						UNITAI_MISSIONARY, UNITAI_WORKER, UNITAI_WORKER_SEA
					};
					PlayerTypes eActivePlayer = GC.getGame().getActivePlayer();
					TeamTypes eActiveTeam = GC.getGame().getActiveTeam();
					for(int j = 0; j < sizeof(priorityList) / sizeof(UnitAITypes); j++) {
						// Use unit owner as tiebreaker
						for(int k = 0; k < 3; k++) {
							pUnit = pLoopPlot->plotCheck(PUF_isUnitAIType,
									priorityList[j], -1,
									(k == 0 ? eActivePlayer : NO_PLAYER),
									(k == 1 ? eActiveTeam : NO_TEAM),
									PUF_isVisibleDebug, eActivePlayer, -1);
							if(pUnit != NULL)
								break;
						}
						if(pUnit != NULL)
							break;
					}
					if(pUnit == NULL) {
						pUnit = pLoopPlot->plotCheck(PUF_cannotDefend,
								-1, -1, NO_PLAYER, NO_TEAM,
								PUF_isVisibleDebug, eActivePlayer, -1);
					}
				}
				else // </advc.004z>
					pUnit = pLoopPlot->getBestDefender(NO_PLAYER);
				if (pUnit != NULL)
				{
					PlayerColorTypes eUnitColor = GET_PLAYER(pUnit->getVisualOwner()).getPlayerColor();
					const NiColorA& kColor = GC.getColorInfo((ColorTypes) GC.getPlayerColorInfo(eUnitColor).getColorTypePrimary()).getColor();

					szBuffer.clear();
					GAMETEXT.setPlotListHelp(szBuffer, pLoopPlot, true, true,
							true); // advc.061, advc.007

					CvPlotIndicatorData kIndicator;
					kIndicator.m_pUnit = pUnit;
					kIndicator.m_strLabel = "UNITS";
					kIndicator.m_strIcon = pUnit->getButton();

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
						break;
					}
					aIndicators.push_back(kIndicator);
				}
			}
		}
	}

	if (fMaxPlotStrength > 0)
	{
		for (int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
		{
			if (GET_PLAYER((PlayerTypes)iPlayer).isAlive())
			{
				PlayerColorTypes eCurPlayerColor = GET_PLAYER((PlayerTypes) iPlayer).getPlayerColor();
				const NiColorA& kColor = GC.getColorInfo((ColorTypes) GC.getPlayerColorInfo(eCurPlayerColor).getColorTypePrimary()).getColor();

				for (int iI = 0; iI < m.numPlots(); iI++)
				{
					CvPlot* pLoopPlot = m.plotByIndex(iI);
					if (pLoopPlot->isVisible(getTeam(), true))
					{
						float fPlotStrength = aafPlayerPlotStrength[iPlayer][iI];
						if (fPlotStrength > 0)
						{
							float fAlpha = (fPlotStrength / fMaxPlotStrength * 0.75f + 0.25f) * 0.8f;
							if (fAlpha > aColors[iI].a)
							{
								aColors[iI] = kColor;
								aColors[iI].a = fAlpha;
							}
						}
					}
				}
			}
		}
	}
}

void CvPlayer::getResourceLayerColors(GlobeLayerResourceOptionTypes eOption, std::vector<NiColorA>& aColors, std::vector<CvPlotIndicatorData>& aIndicators) const
{
	aColors.clear();
	aIndicators.clear();

	PlayerColorTypes ePlayerColor = getPlayerColor();
	CvWStringBuffer szBuffer;
	CvMap& m = GC.getMap();
	for (int iI = 0; iI < m.numPlots(); iI++)
	{
		CvPlot* pLoopPlot = m.plotByIndex(iI);
		if(!pLoopPlot->isRevealed(getTeam(), true))
			continue;
		BonusTypes eLoopBonus = pLoopPlot->getBonusType(
				(GC.getGame().isDebugMode()) ? NO_TEAM : getTeam());
		bool bOfInterest = false; // advc.004z
		if (eLoopBonus != NO_BONUS)
		{
			CvBonusInfo& kBonusInfo = GC.getBonusInfo(eLoopBonus);
			switch (eOption)
			{
			case SHOW_ALL_RESOURCES:
				bOfInterest = true;
				break;
			case SHOW_STRATEGIC_RESOURCES:
				bOfInterest = (kBonusInfo.getHappiness() == 0) && (kBonusInfo.getHealth() == 0);
				break;
			case SHOW_HAPPY_RESOURCES:
				bOfInterest = (kBonusInfo.getHappiness() != 0) && (kBonusInfo.getHealth() == 0);
				break;
			case SHOW_HEALTH_RESOURCES:
				bOfInterest = (kBonusInfo.getHappiness() == 0) && (kBonusInfo.getHealth() != 0);
				break;
			}
		} // <advc.004z>
		ImprovementTypes eImpr = NO_IMPROVEMENT;
		if(!bOfInterest && eOption == SHOW_ALL_RESOURCES &&
				isOption(PLAYEROPTION_NO_UNIT_RECOMMENDATIONS) &&
				getBugOptionBOOL("MainInterface__TribalVillageIcons", true)) {
			eImpr = pLoopPlot->getRevealedImprovementType(getTeam(), false);
			bOfInterest = (eImpr != NO_IMPROVEMENT && GC.getImprovementInfo(eImpr).
					isGoody());
		} // </advc.004z>
		if (bOfInterest)
		{
			CvPlotIndicatorData kData;
			kData.m_strLabel = "RESOURCES";
			kData.m_eVisibility = PLOT_INDICATOR_VISIBLE_ONSCREEN_ONLY;
			kData.m_strIcon = // <advc.004z>
					(eLoopBonus == NO_BONUS ? GC.getImprovementInfo(eImpr).
					getButton() // </advc.004z>
					: GC.getBonusInfo(eLoopBonus).getButton());

			int x = pLoopPlot->getX();
			int y = pLoopPlot->getY();
			kData.m_Target = NiPoint2(m.plotXToPointX(x), m.plotYToPointY(y));
			PlayerTypes eOwner = pLoopPlot->getRevealedOwner(getTeam(), true);
			if (eOwner == NO_PLAYER)
			{
				kData.m_kColor.r = 0.8f;
				kData.m_kColor.g = 0.8f;
				kData.m_kColor.b = 0.8f;
			}
			else
			{
				PlayerColorTypes eCurPlayerColor = GET_PLAYER(eOwner).getPlayerColor();
				const NiColorA& kColor = GC.getColorInfo((ColorTypes) GC.getPlayerColorInfo(eCurPlayerColor).getColorTypePrimary()).getColor();
				kData.m_kColor.r = kColor.r;
				kData.m_kColor.g = kColor.g;
				kData.m_kColor.b = kColor.b;
			}

			szBuffer.clear();
			// <advc.004z>
			if(eLoopBonus == NO_BONUS)
				GAMETEXT.setImprovementHelp(szBuffer, pLoopPlot->getImprovementType());
			else { // </advc.004z>
				//GAMETEXT.setBonusHelp(szBuffer, eCurType, false);
				// <advc.003p> Replacing the above
				if(m_aszBonusHelp[eLoopBonus] != NULL)
					szBuffer.append(*m_aszBonusHelp[eLoopBonus]);
				else {
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

void CvPlayer::getReligionLayerColors(ReligionTypes eSelectedReligion, std::vector<NiColorA>& aColors, std::vector<CvPlotIndicatorData>& aIndicators) const
{
	aColors.resize(GC.getMap().numPlots(), NiColorA(0, 0, 0, 0));
	aIndicators.clear();

	CvRandom kRandom;
	kRandom.init(42 * eSelectedReligion);
	const NiColorA kBaseColor(kRandom.getFloat(), kRandom.getFloat(), kRandom.getFloat(), 1.0f);

	for (int iI = 0; iI  < MAX_PLAYERS; iI ++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			int iLoop;
			for (CvCity* pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
			{
				if (pLoopCity->isRevealed(getTeam(), true))
				{
					if (pLoopCity->isHasReligion(eSelectedReligion))
					{
						float fAlpha = 0.8f;
						if (!pLoopCity->isHolyCity(eSelectedReligion))
						{
							fAlpha *= 0.5f;
						}

						// loop through the city's plots
						for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
						{
							CvPlot* pLoopPlot = plotCity(pLoopCity->getX(), pLoopCity->getY(), iJ);
							if (pLoopPlot != NULL)
							{
								// visibility query
								if (pLoopPlot->isRevealed(getTeam(), true))
								{
									int iIndex = GC.getMap().plotNum(pLoopPlot->getX(), pLoopPlot->getY());
									if (fAlpha > aColors[iIndex].a)
									{
										aColors[iIndex] = kBaseColor;
										aColors[iIndex].a = fAlpha;
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

void CvPlayer::getCultureLayerColors(std::vector<NiColorA>& aColors, std::vector<CvPlotIndicatorData>& aIndicators) const
{	/*  advc.004z (comment): Can increase this for more precise color proportions,
		but the color pattern used by the EXE makes higher values (e.g. 10) look strange. */
	const int iColorsPerPlot = 4;
	CvMap const& m = GC.getMap();
	aColors.resize(m.numPlots() * iColorsPerPlot, NiColorA(0, 0, 0, 0));
	aIndicators.clear();

	// find maximum total culture
	int iMaxTotalCulture = MIN_INT;
	int iMinTotalCulture = MAX_INT;
	for (int iI = 0; iI < m.numPlots(); iI++)
	{
		CvPlot const& kLoopPlot = *m.plotByIndex(iI);
		// <advc.004z>
		if(!kLoopPlot.isVisible(getTeam(), true))
			continue; // </advc.004z>
		int iTotalCulture = kLoopPlot.getTotalCulture(); // advc.003b: was countTotalCulture
		if (iTotalCulture > iMaxTotalCulture)
		{
			iMaxTotalCulture = iTotalCulture;
		}
		if (iTotalCulture < iMinTotalCulture && iTotalCulture > 0)
		{
			iMinTotalCulture = iTotalCulture;
		}
	}
	iMinTotalCulture = 0;

	// find culture percentages
	for (int iI = 0; iI < m.numPlots(); iI++)
	{
		CvPlot const& kLoopPlot = *m.plotByIndex(iI);
		PlayerTypes eOwner = kLoopPlot.getRevealedOwner(getTeam(), true);
		if(eOwner == NO_PLAYER)
			continue; // advc.003: Moved up
		// how many people own this plot?
		std::vector < std::pair<int,int> > plot_owners;
		//int iNumNonzeroOwners = 0;
		// K-Mod
		int iTotalCulture = kLoopPlot.getTotalCulture(); // advc.003b: was countTotalCulture
		if (iTotalCulture <= 0)
			continue;
		// K-Mod end
		// <advc.004z>
		plot_owners.push_back(std::make_pair(kLoopPlot.getCulture(eOwner), eOwner));
		bool bVisible = kLoopPlot.isVisible(getTeam(), true);
		if(bVisible)
		{ // </advc.004z>
			for (int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++) // dlph.21: was MAX_CIV_PLAYERS
			{	// <advc.004z> Owner handled above
				if(iPlayer == eOwner)
					continue; // </advc.004z>
				if(!GET_PLAYER((PlayerTypes)iPlayer).isEverAlive()) // advc.099: was isAlive
					continue;
				int iCurCultureAmount = kLoopPlot.getCulture((PlayerTypes)iPlayer);
				//if (iCurCultureAmount != 0)
				// K-Mod (to reduce visual spam from small amounts of culture)
				if (iCurCultureAmount * 100 / iTotalCulture >= 20)
				{	//iNumNonzeroOwners ++;
					plot_owners.push_back(std::pair<int,int>(iCurCultureAmount, iPlayer));
				}
			}
		}
		//if (!plot_owners.empty())
		/*  <advc.004z> Give players with a high percentage a bigger share of
			the colored area. */
		bool baDone[MAX_PLAYERS] = {false};
		for(int iPass = 0; iPass < 2 &&
				// Try to fill plot_owners up
				plot_owners.size() < iColorsPerPlot; iPass++) {
			// To avoid adding to plot_owners while looping through it
			std::vector <std::pair<int,int> > repeated_owners;
			for(size_t i = 0; i < plot_owners.size(); i++) {
				if(baDone[plot_owners[i].second])
					continue; // Skip iPass==1 if iExtra added in iPass==0
				int iExtra = -1; // Already once in plot_owners
				if(iPass == 0) // Round down
					iExtra += (plot_owners[i].first * iColorsPerPlot) / iTotalCulture;
				else if(iPass == 1) { // Round to nearest
					iExtra += ::round((plot_owners[i].first * iColorsPerPlot) /
							(double)iTotalCulture);
				} // Respect size limit
				iExtra = std::min(iExtra, iColorsPerPlot - (int)
						(plot_owners.size() + repeated_owners.size()));
				for(int j = 0; j < iExtra; j++) {
					repeated_owners.push_back(plot_owners[i]);
					baDone[plot_owners[i].second] = true;
				}
			}
			/*  The more often a civ appears in plot_owners, the more pixels
				will be set to the civ's color. */
			for(size_t i = 0; i < repeated_owners.size(); i++)
				plot_owners.push_back(repeated_owners[i]);
		} // Ideally ==, but my algorithm above can't guarantee that.
		FAssert(plot_owners.size() <= iColorsPerPlot);
		// </advc.004z>
		for (int i = 0; i < iColorsPerPlot; ++i)
		{
			int iCurOwnerIdx = i % plot_owners.size();
			PlayerTypes eCurOwnerID = (PlayerTypes) plot_owners[iCurOwnerIdx].second;
			int iCurCulture = plot_owners[iCurOwnerIdx].first;
			const NiColorA& kCurColor = GC.getColorInfo((ColorTypes)
					GC.getPlayerColorInfo(GET_PLAYER(eCurOwnerID).getPlayerColor()).
					getColorTypePrimary()).getColor();
			// damp the color by the value...
			aColors[iI * iColorsPerPlot + i] = kCurColor;
			/*  <advc.004z> Don't give away info about fogged tiles.
				Use a low factor b/c fogged tiles already look darker than
				visible tiles. */
			float blend_factor = 0.1f;
			if(bVisible) // </advc.004z>
				blend_factor = (iCurCulture - iMinTotalCulture) / (float)iMaxTotalCulture;
			blend_factor = 0.5f * std::min(1.0f, std::max(//0.0f,
					0.1f, // advc.004z
					blend_factor));
			// advc.004z: Coefficient before blend_factor was 0.8
			aColors[iI * iColorsPerPlot + i].a = std::min(0.75f * blend_factor + 0.5f, 1.0f);
		}
	}
}

// <advc.003p>
void CvPlayer::setBonusHelpDirty() {

	if(m_aszBonusHelp == NULL) {
		FAssert(m_aszBonusHelp != NULL);
		return;
	}
	for(int i = 0; i < GC.getNumBonusInfos(); i++)
		SAFE_DELETE(m_aszBonusHelp[i])
} // </advc.003p>


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
	{
		eCivilization = (CivilizationTypes) GC.getDefineINT("BARBARIAN_CIVILIZATION");
	}
	// <advc.001> Redirect the call to the city owner
	if(gDLL->getInterfaceIFace()->isCityScreenUp())
	{
		PlayerTypes eCityOwner = gDLL->getInterfaceIFace()->getHeadSelectedCity()->getOwner();
		if(eCityOwner != getID())
			return GET_PLAYER(eCityOwner).getUnitArtInfo(eUnit, iMeshGroup);
	} // </advc.001>
	UnitArtStyleTypes eStyle = (UnitArtStyleTypes) GC.getCivilizationInfo(eCivilization).getUnitArtStyleType();
	EraTypes eEra = getCurrentEra();
	if (eEra == NO_ERA)
	{
		eEra = (EraTypes) 0;
	}
	return GC.getUnitInfo(eUnit).getArtInfo(iMeshGroup, eEra, eStyle);
}

// K-Mod. I've moved the original code to a new function: CvTeam::hasSpaceshipArrived
bool CvPlayer::hasSpaceshipArrived() const
{
	return GET_TEAM(getTeam()).hasSpaceshipArrived();
}

// <advc.210>
void CvPlayer::checkAlert(int iAlertID, bool bSilent)  {

	if(iAlertID < 0 || iAlertID > (int)m_paAlerts.size()) {
		FAssertMsg(false, "Invalid alert");
		return;
	}
	m_paAlerts[iAlertID]->check(bSilent);
} // </advc.210>

// <advc.104> Inspired by CvTeamAI::AI_estimateTotalYieldRate
// (Tbd.: Move to CvPlayerAI)
double CvPlayer::estimateYieldRate(YieldTypes eYield, int iSamples) const {

	//PROFILE_FUNC(); // Called very frequently; about 1.5% of the turn times (July 2019).
	CvGame const& g = GC.getGame();
	int iGameTurn = g.getGameTurn();
	int iTurnsPlayed = iGameTurn - g.getStartTurn();
	iSamples = std::min(iSamples, iTurnsPlayed - 1);
	std::vector<double> samples; // double for ::dMedian
	/* When anarchy lasts several turns, the sample may not contain a single
	   non-revolution turn. In this case, increase the sample size gradually. */
	while(samples.empty() && iSamples < iTurnsPlayed) {
		for(int i = 1; i <= iSamples; i++) {
			int iSampleIndex = iGameTurn - i;
			int h = 0;
			switch(eYield) {
			case YIELD_COMMERCE:
				h = getEconomyHistory(iSampleIndex);
				break;
			case YIELD_PRODUCTION:
				h = getIndustryHistory(iSampleIndex);
				break;
			case YIELD_FOOD:
				h = getAgricultureHistory(iSampleIndex);
				break;
			default: FAssert(false);
			}
			if(h > 0) // Omit revolution turns
				samples.push_back(h);
		}
		iSamples++;
	}
	if(samples.empty())
		return 0;
	return ::dMedian(samples);
} // </advc.104>

// <advc.004x>
void CvPlayer::killAll(ButtonPopupTypes ePopupType, int iData1)
{
	CvGame const& g = GC.getGame();
	if(getID() != g.getActivePlayer() || !isHuman() ||
			 /* (If outdated non-minimized popups are also a problem, then this
				check could just be removed; should work alright.) */
			!isOption(PLAYEROPTION_MINIMIZE_POP_UPS) ||
			/*  I can't get this to work in network games. The delays introduced by
				net messages cause popups to appear several times. */
			g.isNetworkMultiPlayer())
		return;
	// Preserve the popups we don't want killed in newQueue
	std::list<CvPopupInfo*> newQueue;
	for(int pass = 0; pass < 2; pass++) {
		if(pass == 1) {
			// Recall popups already launched
			gDLL->getInterfaceIFace()->getDisplayedButtonPopups(m_listPopups);
		}
		for(std::list<CvPopupInfo*>::iterator it = m_listPopups.begin();
				it != m_listPopups.end(); it++) {
			CvPopupInfo* pPopup = *it;
			if((pPopup->getButtonPopupType() != ePopupType &&
					/*  Don't relaunch a found-religion popup in response to a
						change-religion popup. The player will already have chosen
						and founded a religion, i.e. the found-religion popup is
						essentially already done. */
					(ePopupType != BUTTONPOPUP_CHANGERELIGION || pass < 1 ||
					pPopup->getButtonPopupType() != BUTTONPOPUP_FOUND_RELIGION)) ||
					(iData1 >= 0 && pPopup->getData1() != iData1))
				newQueue.push_back(pPopup);
			else {
				if(pass <= 0)
					SAFE_DELETE(pPopup);
				// else it's still in the list of popups on display
			}
		}
		m_listPopups.clear();
	}
	// The EXE will relaunch these from m_listPopups
	gDLL->getInterfaceIFace()->clearQueuedPopups();
	for(std::list<CvPopupInfo*>::iterator it = newQueue.begin();
			it != newQueue.end(); it++)
		m_listPopups.push_back(*it);
} // </advc.004x>

// <advc.314>
bool CvPlayer::isGoodyTech(TechTypes techId, bool bProgress) const {

	CvTechInfo& t = GC.getTechInfo(techId);
	if(!bProgress && !t.isGoodyTech())
		return false;
	if(bProgress && t.getEra() >= 4)
		return false;
	return canResearch(techId, false, true);
}

void CvPlayer::addGoodyMsg(CvWString s, CvPlot const& p, TCHAR const* sound) {

	gDLL->getInterfaceIFace()->addHumanMessage(getID(), true,
			GC.getEVENT_MESSAGE_TIME(), s, sound, MESSAGE_TYPE_MINOR_EVENT,
			ARTFILEMGR.getImprovementArtInfo("ART_DEF_IMPROVEMENT_GOODY_HUT")->
			getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"),
			p.getX(), p.getY());
}

void CvPlayer::promoteFreeUnit(CvUnit& u, double pr) {

	FeatureTypes eDefFeature = NO_FEATURE;
	for(int i = 0; i < GC.getNumFeatureInfos(); i++) {
		if(GC.getFeatureInfo((FeatureTypes)i).getDefenseModifier() +
		   GC.getFeatureInfo((FeatureTypes)i).getRivalDefenseModifier() > 0) {
			eDefFeature = (FeatureTypes)i;
			break;
		}
	}
	for(int i = 0; i < 2; i++) {
		if(!::bernoulliSuccess(pr, "advc.314"))
			break;
		int iBestValue = -1;
		PromotionTypes eBestPromo = NO_PROMOTION;
		PromotionTypes ePrevPromo = NO_PROMOTION;
		for(int j = 0; j < GC.getNumPromotionInfos(); j++) {
			PromotionTypes ePromo = (PromotionTypes)j;
			CvPromotionInfo& kPromo = GC.getPromotionInfo(ePromo);
			int iUnitCombat = 0;
			for(int k = 0; k < GC.getNumUnitCombatInfos(); k++)
				if(kPromo.getUnitCombat(k))
					iUnitCombat++;
			if((kPromo.getCombatPercent() <= 0 || /* No Combat2 */ j > 0) &&
					(eDefFeature == NO_FEATURE ||
					kPromo.getFeatureDefensePercent(eDefFeature) <= 0) &&
					kPromo.getHillsDefensePercent() <= 0 &&
					/*  This is the Cover promotion. Only that one, Woodsman,
						Guerilla and City Raider have exactly 3 eligible unit
						combat classes. */
					(iUnitCombat != 3 || kPromo.getCityAttackPercent() > 0))
				continue;
			// Second promo needs to build on the first
			/*if(ePrevPromo != NO_PROMOTION &&
					promo.getPrereqPromotion() != ePrevPromo &&
					promo.getPrereqOrPromotion1() != ePrevPromo &&
					promo.getPrereqOrPromotion2() != ePrevPromo &&
					promo.getPrereqOrPromotion2() != ePrevPromo)
				continue;*/
			if(!u.canAcquirePromotion(ePromo))
				continue;
			int iValue = GC.getGame().getSorenRandNum(100, "advc.314");
			std::vector<CvPlot const*> apSurroundings;
			apSurroundings.push_back(u.plot());
			for(int k = 0; k < NUM_DIRECTION_TYPES; k++) {
				CvPlot const* p = plotDirection(u.getX(), u.getY(), (DirectionTypes)k);
				if(p != NULL && !p->isWater())
					apSurroundings.push_back(p);
			}
			for(size_t k = 0; k < apSurroundings.size(); k++) {
				int iTmpVal = 0;
				if(eDefFeature != NO_FEATURE && apSurroundings[k]->getFeatureType() ==
						eDefFeature)
					iTmpVal += kPromo.getFeatureDefensePercent(eDefFeature);
				// Encourage non-Woodsman then
				else iTmpVal -= kPromo.getFeatureDefensePercent(eDefFeature) / 3;
				if(apSurroundings[k]->isHills())
					iTmpVal += kPromo.getHillsDefensePercent();
				else iTmpVal -= kPromo.getHillsDefensePercent() / 3;
				if(k == 0) // Higher weight for pPlot
					iTmpVal *= 3;
				iValue = std::max(0, iValue + iTmpVal);
			}
			if(iValue > iBestValue) {
				iBestValue = iValue;
				eBestPromo = ePromo;
			}
		}
		if(eBestPromo == NO_PROMOTION)
			break;
		u.setHasPromotion(eBestPromo, true);
		ePrevPromo = eBestPromo;
	}
} // </advc.314>

// <advc.120f>
void CvPlayer::announceEspionageToThirdParties(EspionageMissionTypes eMission, PlayerTypes eTarget) {

	bool bReligion = (GC.getEspionageMissionInfo(eMission).
			getSwitchReligionCostFactor() > 0);
	CvWString szBuffer = gDLL->getText((bReligion ?
			"TXT_KEY_ESPIONAGE_3RD_PARTY_SWITCH_RELIGION" :
			"TXT_KEY_ESPIONAGE_3RD_PARTY_SWITCH_CIVIC"),
			GET_PLAYER(eTarget).getCivilizationAdjectiveKey()).GetCString();
	int x = -1, y = -1;
	CvCity* pCapital = GET_PLAYER(eTarget).getCapitalCity();
	if(pCapital != NULL) {
		x = pCapital->getX();
		y = pCapital->getY();
	}
	if(GC.getDefineINT("ANNOUNCE_ESPIONAGE_REVOLUTION") > 0) {
		for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
			CvPlayer const& kObs = GET_PLAYER((PlayerTypes)i);
			if(kObs.isAlive() && kObs.getID() != getID() && kObs.getID() != eTarget &&
					TEAMREF(eTarget).isHasMet(kObs.getTeam())) {
				gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(),
						false, GC.getEVENT_MESSAGE_TIME(), szBuffer,
						NULL, MESSAGE_TYPE_INFO, NULL, NO_COLOR, x, y);
			}
		}
	}
	if(bReligion) {
		CvWString szTmp = gDLL->getText("TXT_KEY_ESPIONAGE_REVEAL_OWNER",
				getCivilizationAdjectiveKey()).GetCString();
		szTmp += L" " + szBuffer;
		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT,
				eTarget, szTmp);
	}
} // </advc.120f>
