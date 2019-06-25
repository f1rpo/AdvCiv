// city.cpp

#include "CvGameCoreDLL.h"
#include "CvCity.h"
#include "CvGamePlay.h"
#include "CvMap.h"
#include "CvInfos.h"
#include "CvArtFileMgr.h"
#include "CvPopupInfo.h"
#include "CyCity.h"
#include "CyArgsList.h"
#include "CvGameTextMgr.h"
#include "CvEventReporter.h"
#include "CvBugOptions.h" // advc.060
#include "CvInitCore.h" // advc.001: Needed for bugfix in getCityBillboardSizeIconColors
#include "BetterBTSAI.h" // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
#include "CvDLLEngineIFaceBase.h"
#include "CvDLLPythonIFaceBase.h"
#include "CvDLLEntityIFaceBase.h"
#include "CvDLLInterfaceIFaceBase.h"


CvCity::CvCity()
{
	m_aiSeaPlotYield = new int[NUM_YIELD_TYPES];
	m_aiRiverPlotYield = new int[NUM_YIELD_TYPES];
	m_aiBaseYieldRate = new int[NUM_YIELD_TYPES];
	m_aiYieldRateModifier = new int[NUM_YIELD_TYPES];
	m_aiPowerYieldRateModifier = new int[NUM_YIELD_TYPES];
	m_aiBonusYieldRateModifier = new int[NUM_YIELD_TYPES];
	m_aiTradeYield = new int[NUM_YIELD_TYPES];
	m_aiCorporationYield = new int[NUM_YIELD_TYPES];
	m_aiExtraSpecialistYield = new int[NUM_YIELD_TYPES];
	m_aiCommerceRate = new int[NUM_COMMERCE_TYPES];
	m_aiProductionToCommerceModifier = new int[NUM_COMMERCE_TYPES];
	m_aiBuildingCommerce = new int[NUM_COMMERCE_TYPES];
	m_aiSpecialistCommerce = new int[NUM_COMMERCE_TYPES];
	m_aiReligionCommerce = new int[NUM_COMMERCE_TYPES];
	m_aiCorporationCommerce = new int[NUM_COMMERCE_TYPES];
	m_aiCommerceRateModifier = new int[NUM_COMMERCE_TYPES];
	m_aiCommerceHappinessPer = new int[NUM_COMMERCE_TYPES];
	m_aiDomainFreeExperience = new int[NUM_DOMAIN_TYPES];
	m_aiDomainProductionModifier = new int[NUM_DOMAIN_TYPES];

	m_aiCulture = new int[MAX_PLAYERS];
	m_aiNumRevolts = new int[MAX_PLAYERS];
	m_abEverOwned = new bool[MAX_PLAYERS];
	m_abTradeRoute = new bool[MAX_PLAYERS];
	m_abRevealed = new bool[MAX_TEAMS];
	m_abEspionageVisibility = new bool[MAX_TEAMS];

	m_paiNoBonus = NULL;
	m_paiFreeBonus = NULL;
	m_paiNumBonuses = NULL;
	m_paiNumCorpProducedBonuses = NULL;
	m_paiProjectProduction = NULL;
	m_paiBuildingProduction = NULL;
	m_paiBuildingProductionTime = NULL;
	m_paiBuildingOriginalOwner = NULL;
	m_paiBuildingOriginalTime = NULL;
	m_paiUnitProduction = NULL;
	m_paiUnitProductionTime = NULL;
	m_paiGreatPeopleUnitRate = NULL;
	m_paiGreatPeopleUnitProgress = NULL;
	m_paiSpecialistCount = NULL;
	m_paiMaxSpecialistCount = NULL;
	m_paiForceSpecialistCount = NULL;
	m_paiFreeSpecialistCount = NULL;
	m_paiImprovementFreeSpecialists = NULL;
	m_paiReligionInfluence = NULL;
	m_paiStateReligionHappiness = NULL;
	m_paiUnitCombatFreeExperience = NULL;
	m_paiFreePromotionCount = NULL;
	m_paiNumRealBuilding = NULL;
	m_paiNumFreeBuilding = NULL;

	m_pabWorkingPlot = NULL;
	m_pabHasReligion = NULL;
	m_pabHasCorporation = NULL;

	m_paTradeCities = NULL;

	CvDLLEntity::createCityEntity(this);		// create and attach entity to city

	m_aiBaseYieldRank = new int[NUM_YIELD_TYPES];
	m_abBaseYieldRankValid = new bool[NUM_YIELD_TYPES];
	m_aiYieldRank = new int[NUM_YIELD_TYPES];
	m_abYieldRankValid = new bool[NUM_YIELD_TYPES];
	m_aiCommerceRank = new int[NUM_COMMERCE_TYPES];
	m_abCommerceRankValid = new bool[NUM_COMMERCE_TYPES];

	reset(0, NO_PLAYER, 0, 0, true);
}

CvCity::~CvCity()
{
	CvDLLEntity::removeEntity();			// remove entity from engine
	CvDLLEntity::destroyEntity();			// delete CvCityEntity and detach from us

	uninit();

	SAFE_DELETE_ARRAY(m_aiBaseYieldRank);
	SAFE_DELETE_ARRAY(m_abBaseYieldRankValid);
	SAFE_DELETE_ARRAY(m_aiYieldRank);
	SAFE_DELETE_ARRAY(m_abYieldRankValid);
	SAFE_DELETE_ARRAY(m_aiCommerceRank);
	SAFE_DELETE_ARRAY(m_abCommerceRankValid);

	SAFE_DELETE_ARRAY(m_aiSeaPlotYield);
	SAFE_DELETE_ARRAY(m_aiRiverPlotYield);
	SAFE_DELETE_ARRAY(m_aiBaseYieldRate);
	SAFE_DELETE_ARRAY(m_aiYieldRateModifier);
	SAFE_DELETE_ARRAY(m_aiPowerYieldRateModifier);
	SAFE_DELETE_ARRAY(m_aiBonusYieldRateModifier);
	SAFE_DELETE_ARRAY(m_aiTradeYield);
	SAFE_DELETE_ARRAY(m_aiCorporationYield);
	SAFE_DELETE_ARRAY(m_aiExtraSpecialistYield);
	SAFE_DELETE_ARRAY(m_aiCommerceRate);
	SAFE_DELETE_ARRAY(m_aiProductionToCommerceModifier);
	SAFE_DELETE_ARRAY(m_aiBuildingCommerce);
	SAFE_DELETE_ARRAY(m_aiSpecialistCommerce);
	SAFE_DELETE_ARRAY(m_aiReligionCommerce);
	SAFE_DELETE_ARRAY(m_aiCorporationCommerce);
	SAFE_DELETE_ARRAY(m_aiCommerceRateModifier);
	SAFE_DELETE_ARRAY(m_aiCommerceHappinessPer);
	SAFE_DELETE_ARRAY(m_aiDomainFreeExperience);
	SAFE_DELETE_ARRAY(m_aiDomainProductionModifier);
	SAFE_DELETE_ARRAY(m_aiCulture);
	SAFE_DELETE_ARRAY(m_aiNumRevolts);
	SAFE_DELETE_ARRAY(m_abEverOwned);
	SAFE_DELETE_ARRAY(m_abTradeRoute);
	SAFE_DELETE_ARRAY(m_abRevealed);
	SAFE_DELETE_ARRAY(m_abEspionageVisibility);
}


void CvCity::init(int iID, PlayerTypes eOwner, int iX, int iY, bool bBumpUnits, bool bUpdatePlotGroups,
		int iOccupationTimer) // advc.122
{
	int iI=-1;
	CvGame& g = GC.getGameINLINE();
	CvPlot* pPlot = GC.getMapINLINE().plotINLINE(iX, iY);

	//--------------------------------
	// Log this event
	if (GC.getLogging())
	{
		if (gDLL->getChtLvl() > 0)
		{
			TCHAR szOut[1024];
			sprintf(szOut, "Player %d City %d built at %d:%d\n", eOwner, iID, iX, iY);
			gDLL->messageControlLog(szOut);
		}
	}

	//--------------------------------
	// Init saved data
	reset(iID, eOwner, pPlot->getX_INLINE(), pPlot->getY_INLINE());

	//--------------------------------
	// Init non-saved data
	setupGraphical();

	//--------------------------------
	// Init other game data
	CvPlayer& kOwner = GET_PLAYER(getOwnerINLINE());
	setName(kOwner.getNewCityName(),
			false, true); // advc.106k

	setEverOwned(getOwnerINLINE(), true);
	/*  advc.122: To prevent updateCultureLevel from bumping units in
		surrounding tiles after trading a city under occupation. Don't call
		setOccupationTimer though -- don't need all those updates. */
	m_iOccupationTimer = iOccupationTimer;
	updateCultureLevel(false);

	if (pPlot->getCulture(getOwnerINLINE()) < GC.getDefineINT("FREE_CITY_CULTURE"))
	{
		pPlot->setCulture(getOwnerINLINE(), GC.getDefineINT("FREE_CITY_CULTURE"), bBumpUnits, false);
	}
	pPlot->setOwner(getOwnerINLINE(), bBumpUnits, false);
	pPlot->setPlotCity(this);

	for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX_INLINE(), getY_INLINE(), ((DirectionTypes)iI));

		if (pAdjacentPlot != NULL)
		{
			if (pAdjacentPlot->getCulture(getOwnerINLINE()) < GC.getDefineINT("FREE_CITY_ADJACENT_CULTURE"))
			{
				pAdjacentPlot->setCulture(getOwnerINLINE(), GC.getDefineINT("FREE_CITY_ADJACENT_CULTURE"),
						// advc.003b: Updated in the next line in any case
						false/*was bBumpUnits*/, false);
			}
			pAdjacentPlot->updateCulture(bBumpUnits, false);
		}
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (GET_TEAM(getTeam()).isVassal((TeamTypes)iI))
		{
			pPlot->changeAdjacentSight((TeamTypes)iI, GC.getDefineINT("PLOT_VISIBILITY_RANGE"), true, NULL, false);
		}
	}

	CyArgsList argsList;
	argsList.add(iX);
	argsList.add(iY);
	long lResult=0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "citiesDestroyFeatures", argsList.makeFunctionArgs(), &lResult);

	if (lResult == 1)
	{
		if (pPlot->getFeatureType() != NO_FEATURE)
		{
			pPlot->setFeatureType(NO_FEATURE);
		}
	}

	pPlot->setImprovementType(NO_IMPROVEMENT);
	pPlot->updateCityRoute(false);

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (pPlot->isVisible(((TeamTypes)iI), false))
			{
				setRevealed(((TeamTypes)iI), true);
			}
		}
	}

	changeMilitaryHappinessUnits(pPlot->plotCount(PUF_isMilitaryHappiness));

	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		changeCommerceHappinessPer(((CommerceTypes)iI), GC.getCommerceInfo((CommerceTypes)iI).getInitialHappiness());
	}

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		if (kOwner.isBuildingFree((BuildingTypes)iI))
		{
			setNumFreeBuilding(((BuildingTypes)iI), 1);
		}
	}

	area()->changeCitiesPerPlayer(getOwnerINLINE(), 1);
	// <advc.030b>
	CvArea* wa = waterArea(true);
	if(wa != NULL)
		wa->changeCitiesPerPlayer(getOwnerINLINE(), 1); // </advc.030b>
	GET_TEAM(getTeam()).changeNumCities(1);
	g.changeNumCities(1);

	setGameTurnFounded(g.getGameTurn());
	setGameTurnAcquired(g.getGameTurn());

	changePopulation(
		// advc.004b: No functional change, just needed the same thing elsewhere
		initialPopulation());

	changeAirUnitCapacity(GC.getDefineINT("CITY_AIR_UNIT_CAPACITY"));

	updateFreshWaterHealth();
	updateFeatureHealth();
	updateFeatureHappiness();
	updatePowerHealth();

	kOwner.updateMaintenance();

	GC.getMapINLINE().updateWorkingCity();

	g.AI_makeAssignWorkDirty();

	//kOwner.setFoundedFirstCity(true); // advc.104: Moved to CvPlayer::initCity

	if (g.isFinalInitialized())
	{
		if (kOwner.getNumCities() == 1)
		{
			for (iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
			{
				if (GC.getCivilizationInfo(getCivilizationType()).isCivilizationFreeBuildingClass(iI))
				{
					BuildingTypes eLoopBuilding = ((BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI)));

					if (eLoopBuilding != NO_BUILDING)
					{
						setNumRealBuilding(eLoopBuilding, true);
					}
				}
			}

			if (!isHuman() /* advc.250b: */ && !g.isOption(GAMEOPTION_SPAH))
			{
				changeOverflowProduction(GC.getDefineINT("INITIAL_AI_CITY_PRODUCTION"), 0);
			} // <advc.124g>
			if(isHuman() && eOwner == g.getActivePlayer() &&
					kOwner.getCurrentResearch() == NO_TECH)
				kOwner.chooseTech();
			// </advc.124g>
		}
	}

	updateEspionageVisibility(false);

	if (bUpdatePlotGroups)
	{
		GC.getGameINLINE().updatePlotGroups();
	}

	AI_init();
}


void CvCity::uninit()
{
	SAFE_DELETE_ARRAY(m_paiNoBonus);
	SAFE_DELETE_ARRAY(m_paiFreeBonus);
	SAFE_DELETE_ARRAY(m_paiNumBonuses);
	SAFE_DELETE_ARRAY(m_paiNumCorpProducedBonuses);
	SAFE_DELETE_ARRAY(m_paiProjectProduction);
	SAFE_DELETE_ARRAY(m_paiBuildingProduction);
	SAFE_DELETE_ARRAY(m_paiBuildingProductionTime);
	SAFE_DELETE_ARRAY(m_paiBuildingOriginalOwner);
	SAFE_DELETE_ARRAY(m_paiBuildingOriginalTime);
	SAFE_DELETE_ARRAY(m_paiUnitProduction);
	SAFE_DELETE_ARRAY(m_paiUnitProductionTime);
	SAFE_DELETE_ARRAY(m_paiGreatPeopleUnitRate);
	SAFE_DELETE_ARRAY(m_paiGreatPeopleUnitProgress);
	SAFE_DELETE_ARRAY(m_paiSpecialistCount);
	SAFE_DELETE_ARRAY(m_paiMaxSpecialistCount);
	SAFE_DELETE_ARRAY(m_paiForceSpecialistCount);
	SAFE_DELETE_ARRAY(m_paiFreeSpecialistCount);
	SAFE_DELETE_ARRAY(m_paiImprovementFreeSpecialists);
	SAFE_DELETE_ARRAY(m_paiReligionInfluence);
	SAFE_DELETE_ARRAY(m_paiStateReligionHappiness);
	SAFE_DELETE_ARRAY(m_paiUnitCombatFreeExperience);
	SAFE_DELETE_ARRAY(m_paiFreePromotionCount);
	SAFE_DELETE_ARRAY(m_paiNumRealBuilding);
	SAFE_DELETE_ARRAY(m_paiNumFreeBuilding);

	SAFE_DELETE_ARRAY(m_pabWorkingPlot);
	SAFE_DELETE_ARRAY(m_pabHasReligion);
	SAFE_DELETE_ARRAY(m_pabHasCorporation);

	SAFE_DELETE_ARRAY(m_paTradeCities);

	m_orderQueue.clear();
}

// Initializes data members that are serialized.
void CvCity::reset(int iID, PlayerTypes eOwner, int iX, int iY, bool bConstructorCall)
{
	int iI;

	//--------------------------------
	// Uninit class
	uninit();

	m_iID = iID;
	m_iX = iX;
	m_iY = iY;
	m_iRallyX = INVALID_PLOT_COORD;
	m_iRallyY = INVALID_PLOT_COORD;
	m_iGameTurnFounded = 0;
	m_iGameTurnAcquired = 0;
	m_iPopulation = 0;
	m_iHighestPopulation = 0;
	m_iWorkingPopulation = 0;
	m_iSpecialistPopulation = 0;
	m_iNumGreatPeople = 0;
	m_iBaseGreatPeopleRate = 0;
	m_iGreatPeopleRateModifier = 0;
	m_iGreatPeopleProgress = 0;
	m_iNumWorldWonders = 0;
	m_iNumTeamWonders = 0;
	m_iNumNationalWonders = 0;
	m_iNumBuildings = 0;
	m_iGovernmentCenterCount = 0;
	m_iMaintenance = 0;
	m_iMaintenanceModifier = 0;
	m_iWarWearinessModifier = 0;
	m_iHurryAngerModifier = 0;
	m_iHealRate = 0;
	m_iEspionageHealthCounter = 0;
	m_iEspionageHappinessCounter = 0;
	m_iFreshWaterGoodHealth = 0;
	m_iFreshWaterBadHealth = 0;
	m_iFeatureGoodHealth = 0;
	m_iFeatureBadHealth = 0;
	m_iBuildingGoodHealth = 0;
	m_iBuildingBadHealth = 0;
	m_iPowerGoodHealth = 0;
	m_iPowerBadHealth = 0;
	m_iBonusGoodHealth = 0;
	m_iBonusBadHealth = 0;
	m_iHurryAngerTimer = 0;
	m_iConscriptAngerTimer = 0;
	m_iDefyResolutionAngerTimer = 0;
	m_iHappinessTimer = 0;
	m_iMilitaryHappinessUnits = 0;
	m_iBuildingGoodHappiness = 0;
	m_iBuildingBadHappiness = 0;
	m_iExtraBuildingGoodHappiness = 0;
	m_iExtraBuildingBadHappiness = 0;
	m_iExtraBuildingGoodHealth = 0;
	m_iExtraBuildingBadHealth = 0;
	m_iFeatureGoodHappiness = 0;
	m_iFeatureBadHappiness = 0;
	m_iBonusGoodHappiness = 0;
	m_iBonusBadHappiness = 0;
	m_iReligionGoodHappiness = 0;
	m_iReligionBadHappiness = 0;
	m_iExtraHappiness = 0;
	m_iExtraHealth = 0;
	m_iNoUnhappinessCount = 0;
	//m_iNoUnhealthyPopulationCount = 0;
	m_iUnhealthyPopulationModifier = 0; // K-Mod
	m_iBuildingOnlyHealthyCount = 0;
	m_iFood = 0;
	m_iFoodKept = 0;
	m_iMaxFoodKeptPercent = 0;
	m_iOverflowProduction = 0;
	m_iFeatureProduction = 0;
	m_iMilitaryProductionModifier = 0;
	m_iSpaceProductionModifier = 0;
	m_iExtraTradeRoutes = 0;
	m_iTradeRouteModifier = 0;
	m_iForeignTradeRouteModifier = 0;
	m_iBuildingDefense = 0;
	m_iBuildingBombardDefense = 0;
	m_iFreeExperience = 0;
	m_iCurrAirlift = 0;
	m_iMaxAirlift = 0;
	m_iAirModifier = 0;
	m_iAirUnitCapacity = 0;
	m_iNukeModifier = 0;
	m_iFreeSpecialist = 0;
	m_iPowerCount = 0;
	m_iDirtyPowerCount = 0;
	m_iDefenseDamage = 0;
	m_iLastDefenseDamage = 0;
	m_iOccupationTimer = 0;
	m_iCultureUpdateTimer = 0;
	m_iCitySizeBoost = 0;
	m_iSpecialistFreeExperience = 0;
	m_iEspionageDefenseModifier = 0;

	m_bNeverLost = true;
	m_bBombarded = false;
	m_bDrafted = false;
	m_bAirliftTargeted = false;
	m_bWeLoveTheKingDay = false;
	m_bCitizensAutomated = true;
	m_bProductionAutomated = false;
	m_bWallOverride = false;
	m_bInfoDirty = true;
	m_bLayoutDirty = false;
	m_bPlundered = false;
	m_bInvestigate = false; // advc.103

	m_eOwner = eOwner;
	m_ePreviousOwner = NO_PLAYER;
	m_eOriginalOwner = eOwner;
	m_eCultureLevel = NO_CULTURELEVEL;

	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		m_aiSeaPlotYield[iI] = 0;
		m_aiRiverPlotYield[iI] = 0;
		m_aiBaseYieldRate[iI] = 0;
		m_aiYieldRateModifier[iI] = 0;
		m_aiPowerYieldRateModifier[iI] = 0;
		m_aiBonusYieldRateModifier[iI] = 0;
		m_aiTradeYield[iI] = 0;
		m_aiCorporationYield[iI] = 0;
		m_aiExtraSpecialistYield[iI] = 0;
	}

	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		m_aiCommerceRate[iI] = 0;
		m_aiProductionToCommerceModifier[iI] = 0;
		m_aiBuildingCommerce[iI] = 0;
		m_aiSpecialistCommerce[iI] = 0;
		m_aiReligionCommerce[iI] = 0;
		m_aiCorporationCommerce[iI] = 0;
		m_aiCommerceRateModifier[iI] = 0;
		m_aiCommerceHappinessPer[iI] = 0;
	}

	for (iI = 0; iI < NUM_DOMAIN_TYPES; iI++)
	{
		m_aiDomainFreeExperience[iI] = 0;
		m_aiDomainProductionModifier[iI] = 0;
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_aiCulture[iI] = 0;
		m_aiNumRevolts[iI] = 0;
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_abEverOwned[iI] = false;
		m_abTradeRoute[iI] = false;
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		m_abRevealed[iI] = false;
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		m_abEspionageVisibility[iI] = false;
	}
	m_iPopRushHurryCount = 0; // advc.912d
	// <advc.004x>
	mrWasUnit = false;
	mrOrder = -1; // </advc.004x>
	m_szName.clear();
	m_szPreviousName.clear(); // advc.106k
	m_szScriptData = "";

	m_bPopulationRankValid = false;
	m_iPopulationRank = -1;

	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		m_abBaseYieldRankValid[iI] = false;
		m_abYieldRankValid[iI] = false;
		m_aiBaseYieldRank[iI] = -1;
		m_aiYieldRank[iI] = -1;
	}

	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		m_abCommerceRankValid[iI] = false;
		m_aiCommerceRank[iI] = -1;
	}

	if (!bConstructorCall)
	{
		FAssertMsg((0 < GC.getNumBonusInfos()),  "GC.getNumBonusInfos() is not greater than zero but an array is being allocated in CvCity::reset");
		m_paiNoBonus = new int[GC.getNumBonusInfos()];
		m_paiFreeBonus = new int[GC.getNumBonusInfos()];
		m_paiNumBonuses = new int[GC.getNumBonusInfos()];
		m_paiNumCorpProducedBonuses = new int[GC.getNumBonusInfos()];
		for (iI = 0; iI < GC.getNumBonusInfos(); iI++)
		{
			m_paiNoBonus[iI] = 0;
			m_paiFreeBonus[iI] = 0;
			m_paiNumBonuses[iI] = 0;
			m_paiNumCorpProducedBonuses[iI] = 0;
		}

		m_paiProjectProduction = new int[GC.getNumProjectInfos()];
		for (iI = 0; iI < GC.getNumProjectInfos(); iI++)
		{
			m_paiProjectProduction[iI] = 0;
		}

		FAssertMsg((0 < GC.getNumBuildingInfos()),  "GC.getNumBuildingInfos() is not greater than zero but an array is being allocated in CvCity::reset");
		//m_ppBuildings = new CvBuilding *[GC.getNumBuildingInfos()];
		m_paiBuildingProduction = new int[GC.getNumBuildingInfos()];
		m_paiBuildingProductionTime = new int[GC.getNumBuildingInfos()];
		m_paiBuildingOriginalOwner = new int[GC.getNumBuildingInfos()];
		m_paiBuildingOriginalTime = new int[GC.getNumBuildingInfos()];
		m_paiNumRealBuilding = new int[GC.getNumBuildingInfos()];
		m_paiNumFreeBuilding = new int[GC.getNumBuildingInfos()];
		for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
		{
			//m_ppBuildings[iI] = NULL;
			m_paiBuildingProduction[iI] = 0;
			m_paiBuildingProductionTime[iI] = 0;
			m_paiBuildingOriginalOwner[iI] = -1;
			m_paiBuildingOriginalTime[iI] = MIN_INT;
			m_paiNumRealBuilding[iI] = 0;
			m_paiNumFreeBuilding[iI] = 0;
		}

		FAssertMsg((0 < GC.getNumUnitInfos()),  "GC.getNumUnitInfos() is not greater than zero but an array is being allocated in CvCity::reset");
		m_paiUnitProduction = new int[GC.getNumUnitInfos()];
		m_paiUnitProductionTime = new int[GC.getNumUnitInfos()];
		m_paiGreatPeopleUnitRate = new int[GC.getNumUnitInfos()];
		m_paiGreatPeopleUnitProgress = new int[GC.getNumUnitInfos()];
		for (iI = 0;iI < GC.getNumUnitInfos();iI++)
		{
			m_paiUnitProduction[iI] = 0;
			m_paiUnitProductionTime[iI] = 0;
			m_paiGreatPeopleUnitRate[iI] = 0;
			m_paiGreatPeopleUnitProgress[iI] = 0;
		}

		FAssertMsg((0 < GC.getNumSpecialistInfos()),  "GC.getNumSpecialistInfos() is not greater than zero but an array is being allocated in CvCity::reset");
		m_paiSpecialistCount = new int[GC.getNumSpecialistInfos()];
		m_paiMaxSpecialistCount = new int[GC.getNumSpecialistInfos()];
		m_paiForceSpecialistCount = new int[GC.getNumSpecialistInfos()];
		m_paiFreeSpecialistCount = new int[GC.getNumSpecialistInfos()];
		for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
		{
			m_paiSpecialistCount[iI] = 0;
			m_paiMaxSpecialistCount[iI] = 0;
			m_paiForceSpecialistCount[iI] = 0;
			m_paiFreeSpecialistCount[iI] = 0;
		}

		FAssertMsg((0 < GC.getNumImprovementInfos()),  "GC.getNumImprovementInfos() is not greater than zero but an array is being allocated in CvCity::reset");
		m_paiImprovementFreeSpecialists = new int[GC.getNumImprovementInfos()];
		for (iI = 0; iI < GC.getNumImprovementInfos(); iI++)
		{
			m_paiImprovementFreeSpecialists[iI] = 0;
		}

		m_paiReligionInfluence = new int[GC.getNumReligionInfos()];
		m_paiStateReligionHappiness = new int[GC.getNumReligionInfos()];
		m_pabHasReligion = new bool[GC.getNumReligionInfos()];
		for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
		{
			m_paiReligionInfluence[iI] = 0;
			m_paiStateReligionHappiness[iI] = 0;
			m_pabHasReligion[iI] = false;
		}

		m_pabHasCorporation = new bool[GC.getNumCorporationInfos()];
		for (iI = 0; iI < GC.getNumCorporationInfos(); iI++)
		{
			m_pabHasCorporation[iI] = false;
		}

		FAssertMsg((0 < GC.getNumUnitCombatInfos()),  "GC.getNumUnitCombatInfos() is not greater than zero but an array is being allocated in CvCity::reset");
		m_paiUnitCombatFreeExperience = new int[GC.getNumUnitCombatInfos()];
		for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
		{
			m_paiUnitCombatFreeExperience[iI] = 0;
		}

		FAssertMsg((0 < GC.getNumPromotionInfos()),  "GC.getNumPromotionInfos() is not greater than zero but an array is being allocated in CvCity::reset");
		m_paiFreePromotionCount = new int[GC.getNumPromotionInfos()];
		for (iI = 0; iI < GC.getNumPromotionInfos(); iI++)
		{
			m_paiFreePromotionCount[iI] = 0;
		}

		FAssertMsg((0 < NUM_CITY_PLOTS),  "NUM_CITY_PLOTS is not greater than zero but an array is being allocated in CvCity::reset");
		m_pabWorkingPlot = new bool[NUM_CITY_PLOTS];
		for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			m_pabWorkingPlot[iI] = false;
		}

		FAssertMsg((0 < GC.getDefineINT("MAX_TRADE_ROUTES")),  "GC.getMAX_TRADE_ROUTES() is not greater than zero but an array is being allocated in CvCity::reset");
		m_paTradeCities = new IDInfo[GC.getDefineINT("MAX_TRADE_ROUTES")];
		for (iI = 0; iI < GC.getDefineINT("MAX_TRADE_ROUTES"); iI++)
		{
			m_paTradeCities[iI].reset();
		}

		m_aEventsOccured.clear();
		m_aBuildingYieldChange.clear();
		m_aBuildingCommerceChange.clear();
		m_aBuildingHappyChange.clear();
		m_aBuildingHealthChange.clear();
	}

	if (!bConstructorCall)
	{
		AI_reset();
	}
}

//////////////////////////////////////
// graphical only setup
//////////////////////////////////////
void CvCity::setupGraphical()
{
	if (!GC.IsGraphicsInitialized())
	{
		return;
	}

	CvDLLEntity::setup();

	setInfoDirty(true);
	setLayoutDirty(true);
}

void CvCity::kill(bool bUpdatePlotGroups)
{
	int iI=-1;

	if (isCitySelected())
	{
		gDLL->getInterfaceIFace()->clearSelectedCities();
	}

	CvPlot* pPlot = plot();

	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = getCityIndexPlot(iI);

		if (pLoopPlot != NULL)
		{
			if (pLoopPlot->getWorkingCityOverride() == this)
			{
				pLoopPlot->setWorkingCityOverride(NULL);
			}
		}
	}

	setCultureLevel(NO_CULTURELEVEL, false);

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		setNumRealBuilding(((BuildingTypes)iI), 0);
		setNumFreeBuilding(((BuildingTypes)iI), 0);
	}

	for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
	{
		setFreeSpecialistCount(((SpecialistTypes)iI), 0);
	}

	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		setTradeYield(((YieldTypes)iI), 0);
		setCorporationYield(((YieldTypes) iI), 0);
	}

	for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		setHasReligion(((ReligionTypes)iI), false, false, true);

		if (isHolyCity((ReligionTypes)iI))
		{
			GC.getGameINLINE().setHolyCity(((ReligionTypes)iI), NULL, false);
		}
	}

	for (iI = 0; iI < GC.getNumCorporationInfos(); iI++)
	{
		setHasCorporation(((CorporationTypes)iI), false, false);

		if (isHeadquarters((CorporationTypes)iI))
		{
			GC.getGameINLINE().setHeadquarters(((CorporationTypes)iI), NULL, false);
		}
	}

	setPopulation(0);

	AI_assignWorkingPlots();

	clearOrderQueue();

	// remember the visibility before we take away the city from the plot below
	std::vector<bool> abEspionageVisibility;
	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		abEspionageVisibility.push_back(getEspionageVisibility((TeamTypes)iI));
	}
	/*  UNOFFICIAL_PATCH, Bugfix, 08/04/09, jdog5000:
		Need to clear trade routes of dead city, else they'll be claimed for the owner forever. */
	clearTradeRoutes();

	pPlot->setPlotCity(NULL);
	pPlot->setRuinsName(getName()); // advc.005c

	// UNOFFICIAL_PATCH, replace floodplains after city is removed, 03/04/10, jdog5000: START
	if (pPlot->getBonusType() == NO_BONUS
			&& GC.getDefineINT("FLOODPLAIN_AFTER_RAZE") > 0) // advc.129b
	{
		for (int iJ = 0; iJ < GC.getNumFeatureInfos(); iJ++)
		{
			if (GC.getFeatureInfo((FeatureTypes)iJ).isRequiresRiver())
			{
				if (pPlot->canHaveFeature((FeatureTypes)iJ))
				{
					if (GC.getFeatureInfo((FeatureTypes)iJ).getAppearanceProbability() == 10000)
					{
						pPlot->setFeatureType((FeatureTypes)iJ);
						break;
					}
				}
			}
		}
	} // UNOFFICIAL_PATCH: END

	area()->changeCitiesPerPlayer(getOwnerINLINE(), -1);
	// <advc.030b>
	CvArea* wa = waterArea(true);
	/*  Can't really handle ice melted by global warming, but at least ensure
		that CitiesPerPlayer doesn't become negative. */
	if(wa != NULL && wa->getCitiesPerPlayer(getOwnerINLINE(), true) > 0)
		wa->changeCitiesPerPlayer(getOwnerINLINE(), -1); // </advc.030b>
	GET_TEAM(getTeam()).changeNumCities(-1);

	GC.getGameINLINE().changeNumCities(-1);

	FAssertMsg(getWorkingPopulation() == 0, "getWorkingPopulation is expected to be 0");
	FAssertMsg(!isWorkingPlot(CITY_HOME_PLOT), "isWorkingPlot(CITY_HOME_PLOT) is expected to be false");
	FAssertMsg(getSpecialistPopulation() == 0, "getSpecialistPopulation is expected to be 0");
	FAssertMsg(getNumGreatPeople() == 0, "getNumGreatPeople is expected to be 0");
	FAssertMsg(getBaseYieldRate(YIELD_FOOD) == 0, "getBaseYieldRate(YIELD_FOOD) is expected to be 0");
	FAssertMsg(getBaseYieldRate(YIELD_PRODUCTION) == 0, "getBaseYieldRate(YIELD_PRODUCTION) is expected to be 0");
	FAssertMsg(getBaseYieldRate(YIELD_COMMERCE) == 0, "getBaseYieldRate(YIELD_COMMERCE) is expected to be 0");
	FAssertMsg(!isProduction(), "isProduction is expected to be false");

	PlayerTypes eOwner = getOwnerINLINE();

	bool bCapital = isCapital();

	pPlot->setImprovementType(GC.getRUINS_IMPROVEMENT());

	CvEventReporter::getInstance().cityLost(this);

	GET_PLAYER(getOwnerINLINE()).deleteCity(getID());

	pPlot->updateCulture(true, false);

	for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		CvPlot* pAdjacentPlot = plotDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), ((DirectionTypes)iI));

		if (pAdjacentPlot != NULL)
		{
			pAdjacentPlot->updateCulture(true, false);
		}
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (GET_TEAM(GET_PLAYER(eOwner).getTeam()).isVassal((TeamTypes)iI))
		{
			pPlot->changeAdjacentSight((TeamTypes)iI, GC.getDefineINT("PLOT_VISIBILITY_RANGE"), false, NULL, false);
		}
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (abEspionageVisibility[iI])
		{
			pPlot->changeAdjacentSight((TeamTypes)iI, GC.getDefineINT("PLOT_VISIBILITY_RANGE"), false, NULL, false);
		}
	}

	GET_PLAYER(eOwner).updateMaintenance();

	GC.getMapINLINE().updateWorkingCity();

	GC.getGameINLINE().AI_makeAssignWorkDirty();

	if (bCapital)
	{
		GET_PLAYER(eOwner).findNewCapital();

		GET_TEAM(GET_PLAYER(eOwner).getTeam()).resetVictoryProgress();
	}

	if (bUpdatePlotGroups)
	{
		GC.getGameINLINE().updatePlotGroups();
	}

	if (eOwner == GC.getGameINLINE().getActivePlayer())
	{
		gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
	}
}


void CvCity::doTurn()  // advc.003: some style changes
{
	PROFILE("CvCity::doTurn()");

	int iI=-1;

	if (!isBombarded())
	{	// cdtw.2: Now cached
		changeDefenseDamage(-GC.getCITY_DEFENSE_DAMAGE_HEAL_RATE());
	}

	setLastDefenseDamage(getDefenseDamage());
	setBombarded(false);
	setPlundered(false);
	setDrafted(false);
	setAirliftTargeted(false);
	setCurrAirlift(0);
	m_bInvestigate = false; // advc.103

	AI_doTurn();
	// <advc.106k>
	if(!m_szPreviousName.empty() && m_szName.compare(m_szPreviousName) != 0) {
		FAssert(isHuman());
		GC.getGameINLINE().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getOwnerINLINE(),
				gDLL->getText("TXT_KEY_MISC_CITY_RENAMED",
				m_szPreviousName.GetCString(), m_szName.GetCString()), getX_INLINE(),
				getY_INLINE(), GET_PLAYER(getOwnerINLINE()).getPlayerTextColor());
		m_szPreviousName.clear();
	} // </advc.106k>
	bool bAllowNoProduction = !doCheckProduction();
	bAllowNoProduction = false; // advc.064d
	doGrowth();

	doCulture();

	//doPlotCulture(false, getOwnerINLINE(), getCommerceRate(COMMERCE_CULTURE));
	doPlotCultureTimes100(false, getOwnerINLINE(), getCommerceRateTimes100(COMMERCE_CULTURE), true); // K-Mod

	doProduction(bAllowNoProduction);

	doDecay();

	doReligion();

	doGreatPeople();

	doMeltdown();

	updateEspionageVisibility(true);

	if (!isDisorder())
	{
		for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			CvPlot* pLoopPlot = getCityIndexPlot(iI);
			if (pLoopPlot == NULL)
				continue;
			if (pLoopPlot->getWorkingCity() == this && pLoopPlot->isBeingWorked())
				pLoopPlot->doImprovement();

		}
	} // <advc.004x>
	else {
		if(isHuman() && !isProduction() && !isProductionAutomated() &&
				GET_PLAYER(getOwnerINLINE()).getAnarchyTurns() == 1 &&
				GC.getGameINLINE().getGameState() != GAMESTATE_EXTENDED) {
			UnitTypes mrUnit = NO_UNIT;
			BuildingTypes mrBuilding = NO_BUILDING;
			ProjectTypes mrProject = NO_PROJECT;
			if(mrOrder >= 0) {
				if(mrWasUnit)
					mrUnit = (UnitTypes)mrOrder;
				else if(mrOrder >= GC.getNumBuildingInfos())
					mrProject = (ProjectTypes)(mrOrder - GC.getNumBuildingInfos());
				else mrBuilding = (BuildingTypes)mrOrder;
			}
			chooseProduction(mrUnit, mrBuilding, mrProject, mrOrder >= 0);
		}
	} // </advc.004x>
	if (getCultureUpdateTimer() > 0)
		changeCultureUpdateTimer(-1);

	/*  advc.023: Now handled in doRevolt (which is called at the start of a round,
		not the end of a player turn) */
	/*if (getOccupationTimer() > 0)
		changeOccupationTimer(-1);*/

	if (getHurryAngerTimer() > 0)
		changeHurryAngerTimer(-1);

	if (getConscriptAngerTimer() > 0)
		changeConscriptAngerTimer(-1);

	if (getDefyResolutionAngerTimer() > 0)
		changeDefyResolutionAngerTimer(-1);

	if (getHappinessTimer() > 0)
		changeHappinessTimer(-1);

	if (getEspionageHealthCounter() > 0)
		changeEspionageHealthCounter(-1);

	if (getEspionageHappinessCounter() > 0)
		changeEspionageHappinessCounter(-1);

	if (isOccupation() || (angryPopulation() > 0) || (healthRate() < 0))
		setWeLoveTheKingDay(false);
	else if (getPopulation() >= GC.getDefineINT("WE_LOVE_THE_KING_POPULATION_MIN_POPULATION") &&
			GC.getGameINLINE().getSorenRandNum(GC.getDefineINT(
			"WE_LOVE_THE_KING_RAND"), "Do We Love The King?") < getPopulation())
		setWeLoveTheKingDay(true);
	else setWeLoveTheKingDay(false);

	// ONEVENT - Do turn
	CvEventReporter::getInstance().cityDoTurn(this, getOwnerINLINE());

	// XXX
#ifdef _DEBUG
	{
		for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{	// <advc.003> Want to see the value in the debugger
			YieldTypes y = (YieldTypes)iI;
			int byr = getBaseYieldRate(y);
			FAssert(byr >= 0);
			FAssert(getYieldRate(y) >= 0);
			// advc.104u: Code moved into auxiliary function
			int iCount = calculateBaseYieldRate(y);
			FAssert(iCount == byr); // advc.003
		}

		for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
		{
			FAssert(getBuildingCommerce((CommerceTypes)iI) >= 0);
			FAssert(getSpecialistCommerce((CommerceTypes)iI) >= 0);
			FAssert(getReligionCommerce((CommerceTypes)iI) >= 0);
			FAssert(getCorporationCommerce((CommerceTypes)iI) >= 0);
			FAssert(GET_PLAYER(getOwnerINLINE()).getFreeCityCommerce((CommerceTypes)iI) >= 0);
		}

		for (iI = 0; iI < GC.getNumBonusInfos(); iI++)
		{
			FAssert(isNoBonus((BonusTypes)iI) || getNumBonuses((BonusTypes)iI) >= ((isConnectedToCapital()) ? (GET_PLAYER(getOwnerINLINE()).getBonusImport((BonusTypes)iI) - GET_PLAYER(getOwnerINLINE()).getBonusExport((BonusTypes)iI)) : 0));
		}
	}
#endif
	// XXX
}

// <advc.104u> Cut, pasted, refactored from the end of CvCity::doTurn
int CvCity::calculateBaseYieldRate(YieldTypes y) {

	int r = 0;
	for(int i = 0; i < NUM_CITY_PLOTS; i++) {
		if(isWorkingPlot(i)) {
			CvPlot* pl = getCityIndexPlot(i);
			if(pl != NULL)
				r += pl->getYield(y);
		}
	}
	for(int i = 0; i < GC.getNumSpecialistInfos(); i++) {
		SpecialistTypes sp = (SpecialistTypes)i;
		r += GET_PLAYER(getOwnerINLINE()).specialistYield(sp, y) *
				(getSpecialistCount(sp) + getFreeSpecialistCount(sp));
	}
	for(int i = 0; i < GC.getNumBuildingInfos(); i++) {
		BuildingTypes bt = (BuildingTypes)i;
		CvBuildingInfo const& b = GC.getBuildingInfo(bt);
		r += getNumActiveBuilding(bt) * (b.getYieldChange(y) +
				getBuildingYieldChange((BuildingClassTypes)b.
				getBuildingClassType(), y));
	}
	r += getTradeYield(y);
	r += getCorporationYield(y);
	return r;
} // </advc.104u>

// <advc.003> Code cut and pasted from CvPlot::doCulture; also refactored.
void CvCity::doRevolt() {

	PROFILE("CvCity::doRevolts()");
	// <advc.023>
	double prDecr = probabilityOccupationDecrement();
	if(::bernoulliSuccess(prDecr, "advc.023")) {
		changeOccupationTimer(-1);
		return;
	} // </advc.023>
	PlayerTypes eCulturalOwner = calculateCulturalOwner();
	// <advc.099c>
	PlayerTypes eOwnerIgnRange = eCulturalOwner;
	if(GC.getDefineINT("REVOLTS_IGNORE_CULTURE_RANGE") > 0)
		eOwnerIgnRange = plot()->calculateCulturalOwner(true);
	// If not within culture range, can revolt but not flip 
	bool bCanFlip = (eOwnerIgnRange == eCulturalOwner);
	eCulturalOwner = eOwnerIgnRange;
	// </advc.099c>
	/*  <advc.101> To avoid duplicate code in CvDLLWidgetData::parseNationalityHelp,
		compute the revolt probability in a separate function. */
	double prRevolt = revoltProbability();
	if(!::bernoulliSuccess(prRevolt, "advc.101"))
		return; // </advc.101>
	damageGarrison(eCulturalOwner); // advc.003: Code moved into subroutine 
	if(bCanFlip && // advc.099
			canCultureFlip(eCulturalOwner)) {
		if(GC.getGameINLINE().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) &&
				GET_PLAYER(eCulturalOwner).isHuman())
			kill(true);
		else plot()->setOwner(eCulturalOwner, true, true); // will delete pCity
		/*  advc.003 (comment): setOwner doesn't actually delete this object; just
			calls CvCity::kill. Messages also handled by setOwner (through
			CvPlayer::acquireCity). */
		return;
	}
	/* K-Mod, 11/jan/11, karadoc
	** Changed number of revolt turns to not depend on iCityStrength,
	** because iCityStrength can be huge. */
	/* original bts code
	pCity->changeOccupationTimer(GC.getDefineINT("BASE_REVOLT_OCCUPATION_TURNS")
	+ ((iCityStrength * GC.getDefineINT("REVOLT_OCCUPATION_TURNS_PERCENT")) /
	100));*/
	// <advc.023>
	/*  Removed factor 2 from the second summand. K-Mod 1.45 changelog said:
		'Decreased the duration of revolts from
		 3+2*(previous revolts) to 2+(previous revolts). (...)
		 The net effect of these changes is that (...) the number of turns spent
		 in revolt will be similar to before (...).'
		This is just what I need now that the occupation timer decreases
		probabilistically, but it wasn't committed to the K-Mod repository;
		so I'm adding it here. */
	int iTurnsOccupation = GC.getDefineINT("BASE_REVOLT_OCCUPATION_TURNS")
			+ getNumRevolts(eCulturalOwner); // </advc.023>
	// K-Mod end
	changeNumRevolts(eCulturalOwner, 1);
	if(!isOccupation()) // advc.023: Don't prolong revolt
		changeOccupationTimer(iTurnsOccupation);
	CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_REVOLT_IN_CITY",
			GET_PLAYER(eCulturalOwner).getCivilizationAdjective(),
			getNameKey());
	// <advc.023>
	/*  Population loss if pCity should flip, but can't. We know at this point
		that the current revolt doesn't flip the city; but can it ever flip? */
	if((!bCanFlip || !canCultureFlip(eCulturalOwner, false)) &&
			getNumRevolts(eCulturalOwner) > GC.getNUM_WARNING_REVOLTS() &&
			getPopulation() > 1) {
		changePopulation(-1);
		// Says "Citizens have been killed"
		szBuffer.append(L" " + gDLL->getText("TXT_KEY_EVENT_HURRICANE_2"));
	} // </advc.023>
	/*  <advc.101> BtS comment: "XXX announce for all seen cities?"
		I guess that's better. Mustn't startle third parties with color
		and sound though. */
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayer const& kCiv = GET_PLAYER((PlayerTypes)i);
		if(!kCiv.isAlive() || kCiv.isMinorCiv())
			continue;
		bool bAffected = (kCiv.getID() == eCulturalOwner ||
				kCiv.getID() == getOwnerINLINE());
		if(!bAffected && !isRevealed(kCiv.getTeam(), false))
			continue;
		InterfaceMessageTypes eMsg = MESSAGE_TYPE_INFO;
		LPCTSTR szSound = NULL;
		// Color of both the text and the flashing icon
		ColorTypes eColor = (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE");
		if(bAffected) {
			eMsg = MESSAGE_TYPE_MINOR_EVENT;
			szSound = "AS2D_CITY_REVOLT";
			if(kCiv.getID() == getOwnerINLINE())
				eColor = (ColorTypes)GC.getInfoTypeForString("COLOR_RED");
			else eColor = (ColorTypes)GC.getInfoTypeForString("COLOR_GREEN");
		}
		gDLL->getInterfaceIFace()->addHumanMessage(kCiv.getID(), false,
				GC.getEVENT_MESSAGE_TIME(), szBuffer, szSound, eMsg,
				ARTFILEMGR.getInterfaceArtInfo("INTERFACE_RESISTANCE")->getPath(),
				eColor, getX_INLINE(), getY_INLINE(), true, true);
	} // </advc.101>
}

void CvCity::damageGarrison(PlayerTypes eRevoltSource) {

	CLinkList<IDInfo> oldUnits;
	CLLNode<IDInfo>* pUnitNode = plot()->headUnitNode();
	while(pUnitNode != NULL) {
		oldUnits.insertAtEnd(pUnitNode->m_data);
		pUnitNode = plot()->nextUnitNode(pUnitNode);
	}
	pUnitNode = oldUnits.head();
	while(pUnitNode != NULL) {
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = plot()->nextUnitNode(pUnitNode);
		if(pLoopUnit == NULL)
			continue;
		if(pLoopUnit->isBarbarian())
			pLoopUnit->kill(false, eRevoltSource);
		else if(pLoopUnit->canDefend())
			pLoopUnit->changeDamage((pLoopUnit->currHitPoints() / 2),
					eRevoltSource);
	}
} // </advc.003>

bool CvCity::isCitySelected()
{
	return gDLL->getInterfaceIFace()->isCitySelected(this);
}


bool CvCity::canBeSelected() const  // advc.003: refactored
{
	CvGame const& g = GC.getGameINLINE();
	TeamTypes eActiveTeam = g.getActiveTeam();
	if(m_bInvestigate || // advc.103
			getTeam() == eActiveTeam || g.isDebugMode())
		return true;

	if(eActiveTeam != NO_TEAM && plot()->isInvestigate(eActiveTeam))
		return true;

	for(int i = 0; i < GC.getNumEspionageMissionInfos(); i++) {
		EspionageMissionTypes eMission = (EspionageMissionTypes)i;
		CvEspionageMissionInfo const& kMission = GC.getEspionageMissionInfo(eMission);
		if(kMission.isPassive() && kMission.isInvestigateCity() &&
				GET_PLAYER(g.getActivePlayer()).canDoEspionageMission(
				eMission, getOwnerINLINE(), plot(), -1, NULL))
			return true;
	}
	return false;
}


void CvCity::updateSelectedCity(bool bTestProduction)
{
	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = getCityIndexPlot(iI);
		if (pLoopPlot != NULL)
		{
			pLoopPlot->updateShowCitySymbols();
		}
	}

	if (bTestProduction)
	{
		//if ((getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) && !isProduction())
		if ((getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) && !isProduction() && !isProductionAutomated()) // K-Mod
		{
			chooseProduction(NO_UNIT, NO_BUILDING, NO_PROJECT, false, true);
		}
	}
}

// <advc.103>
void CvCity::setInvestigate(bool b) {

	m_bInvestigate = b;
} // </advc.103>


void CvCity::updateYield()
{
	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = getCityIndexPlot(iI);

		if (pLoopPlot != NULL)
		{
			pLoopPlot->updateYield();
		}
	}
}


// XXX kill this?
void CvCity::updateVisibility()
{
	PROFILE_FUNC();

	if (!GC.IsGraphicsInitialized())
	{
		return;
	}

	FAssert(GC.getGameINLINE().getActiveTeam() != NO_TEAM);

	CvDLLEntity::setVisible(isRevealed(GC.getGameINLINE().getActiveTeam(), true));
}


void CvCity::createGreatPeople(UnitTypes eGreatPersonUnit, bool bIncrementThreshold, bool bIncrementExperience)
{
	GET_PLAYER(getOwnerINLINE()).createGreatPeople(eGreatPersonUnit, bIncrementThreshold, bIncrementExperience, getX_INLINE(), getY_INLINE());
}


void CvCity::doTask(TaskTypes eTask, int iData1, int iData2, bool bOption, bool bAlt, bool bShift, bool bCtrl)
{
	bool bCede = false; // advc.122
	switch (eTask)
	{
	case TASK_RAZE:
		GET_PLAYER(getOwnerINLINE()).raze(this);
		break;

	case TASK_DISBAND:
		GET_PLAYER(getOwnerINLINE()).disband(this);
		break;
	// <advc.122>
	case TASK_CEDE:
		bCede = true;
		// fall through // </advc.122>
	case TASK_GIFT:
		if (getLiberationPlayer(false) == iData1)
		{
			liberate(false, /* advc.122: */ bCede);
		}
		else
		{
			GET_PLAYER((PlayerTypes)iData1).acquireCity(this, false, true, true);
		}
		break;

	case TASK_LIBERATE:
		liberate(iData1 != 0);
		break;

	case TASK_SET_AUTOMATED_CITIZENS:
		setCitizensAutomated(bOption);
		break;

	case TASK_SET_AUTOMATED_PRODUCTION:
		setProductionAutomated(bOption, bAlt && bShift && bCtrl);
		break;

	case TASK_SET_EMPHASIZE:
		AI_setEmphasize(((EmphasizeTypes)iData1), bOption);
		break;

	case TASK_CHANGE_SPECIALIST:
		alterSpecialistCount(((SpecialistTypes)iData1), iData2);
		break;

	case TASK_CHANGE_WORKING_PLOT:
		alterWorkingPlot(iData1);
		break;

	case TASK_CLEAR_WORKING_OVERRIDE:
		clearWorkingOverride(iData1);
		break;

	case TASK_HURRY:
		hurry((HurryTypes)iData1);
		break;

	case TASK_CONSCRIPT:
		conscript();
		break;

	case TASK_CLEAR_ORDERS:
		clearOrderQueue();
		break;

	case TASK_RALLY_PLOT:
		setRallyPlot(GC.getMapINLINE().plotINLINE(iData1, iData2));
		break;

	case TASK_CLEAR_RALLY_PLOT:
		setRallyPlot(NULL);
		break;

	default:
		FAssertMsg(false, "eTask failed to match a valid option");
		break;
	}
}


void CvCity::chooseProduction(UnitTypes eTrainUnit, BuildingTypes eConstructBuilding, ProjectTypes eCreateProject, bool bFinish, bool bFront)
{
	// K-Mod. don't create the popup if the city is in disorder
	FAssert(isHuman() && !isProductionAutomated());
	if (isOccupation()) // advc.004x: was isDisorder
	{
		AI_setChooseProductionDirty(true);
		return;
	}
	AI_setChooseProductionDirty(false);
	// K-Mod end
	CvPopupInfo* pPopupInfo = new CvPopupInfo(BUTTONPOPUP_CHOOSEPRODUCTION);
	if (NULL == pPopupInfo)
	{
		return;
	}
	pPopupInfo->setData1(getID());
	pPopupInfo->setOption1(bFinish);

	if (eTrainUnit != NO_UNIT)
	{
		pPopupInfo->setData2(ORDER_TRAIN);
		pPopupInfo->setData3(eTrainUnit);
	}
	else if (eConstructBuilding != NO_BUILDING)
	{
		pPopupInfo->setData2(ORDER_CONSTRUCT);
		pPopupInfo->setData3(eConstructBuilding);
	}
	else if (eCreateProject != NO_PROJECT)
	{
		pPopupInfo->setData2(ORDER_CREATE);
		pPopupInfo->setData3(eCreateProject);
	}
	else
	{
		pPopupInfo->setData2(NO_ORDER);
		pPopupInfo->setData3(NO_UNIT);
	}

	gDLL->getInterfaceIFace()->addPopup(pPopupInfo, getOwnerINLINE(), false, bFront);
}


int CvCity::getCityPlotIndex(const CvPlot* pPlot) const
{
	return plotCityXY(this, pPlot);
}


CvPlot* CvCity::getCityIndexPlot(int iIndex) const
{
	return plotCity(getX_INLINE(), getY_INLINE(), iIndex);
}


bool CvCity::canWork(CvPlot* pPlot) const
{
	if (pPlot->getWorkingCity() != this)
	{
		return false;
	}

	FAssertMsg(getCityPlotIndex(pPlot) != -1, "getCityPlotIndex(pPlot) is expected to be assigned (not -1)");

	if (pPlot->plotCheck(PUF_canSiege, getOwnerINLINE()) != NULL)
	{
		return false;
	}

	if (pPlot->isWater())
	{
		if (!GET_TEAM(getTeam()).isWaterWork())
		{
			return false;
		}

		if (pPlot->getBlockadedCount(getTeam()) > 0)
		{   /*  <advc.124> Should work, but I don't want units to overwrite
				blockades after all. */
			/*bool bDefended = false;
			for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
				PlayerTypes eLoopPlayer = (PlayerTypes)i;
				if(GET_PLAYER(eLoopPlayer).isAlive() && GET_PLAYER(eLoopPlayer).getMasterTeam() ==
						GET_PLAYER(getOwnerINLINE()).getMasterTeam() &&
						pPlot->isVisibleOtherUnit(eLoopPlayer)) {
					isDefended = true;
					break;
				}
			}
			if(!isDefended) */// </advc.124>
				return false;
		}

		/* Replaced by blockade mission, above
		if (!(pPlot->plotCheck(PUF_canDefend, -1, -1, NO_PLAYER, getTeam()))) {
			for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++) {
				CvPlot* pLoopPlot = plotDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), ((DirectionTypes)iI));
				if (pLoopPlot != NULL) {
					if (pLoopPlot->isWater()) {
						if (pLoopPlot->plotCheck(PUF_canSiege, getOwnerINLINE()) != NULL)
							return false;
		} } } }*/
	}

	if (!pPlot->hasYield())
	{
		return false;
	}

	return true;
}


void CvCity::verifyWorkingPlot(int iIndex)
{
	FAssertMsg(iIndex >= 0, "iIndex expected to be >= 0");
	FAssertMsg(iIndex < NUM_CITY_PLOTS, "iIndex expected to be < NUM_CITY_PLOTS");

	if (isWorkingPlot(iIndex))
	{
		CvPlot* pPlot = getCityIndexPlot(iIndex);

		if (pPlot != NULL)
		{
			if (!canWork(pPlot))
			{
				setWorkingPlot(iIndex, false);

				AI_setAssignWorkDirty(true);
			}
		}
	}
}


void CvCity::verifyWorkingPlots()
{
	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		verifyWorkingPlot(iI);
	}
}


void CvCity::clearWorkingOverride(int iIndex)
{
	CvPlot* pPlot = getCityIndexPlot(iIndex);

	if (pPlot != NULL)
	{
		pPlot->setWorkingCityOverride(NULL);
	}
}


int CvCity::countNumImprovedPlots(ImprovementTypes eImprovement, bool bPotential) const
{
	int iCount = 0;

	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = getCityIndexPlot(iI);

		if (pLoopPlot != NULL)
		{
			if (pLoopPlot->getWorkingCity() == this)
			{
				if (eImprovement != NO_IMPROVEMENT)
				{
					if (pLoopPlot->getImprovementType() == eImprovement || 
						(bPotential && pLoopPlot->canHaveImprovement(eImprovement, getTeam())))
					{
						++iCount;
					}
				}
				else if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT)
				{
					iCount++;
				}
			}
		}
	}

	return iCount;
}


int CvCity::countNumWaterPlots() const
{
	int iCount = 0;

	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = getCityIndexPlot(iI);

		if (pLoopPlot != NULL)
		{
			if (pLoopPlot->getWorkingCity() == this)
			{
				if (pLoopPlot->isWater())
				{
					iCount++;
				}
			}
		}
	}

	return iCount;
}

int CvCity::countNumRiverPlots() const
{
	int iCount = 0;

	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = getCityIndexPlot(iI);

		if (pLoopPlot != NULL)
		{
			if (pLoopPlot->getWorkingCity() == this)
			{
				if (pLoopPlot->isRiver())
				{
					++iCount;
				}
			}
		}
	}

	return iCount;
}


int CvCity::findPopulationRank() const
{
	if (!m_bPopulationRankValid)
	{
		/* original bts code
		int iRank = 1; int iLoop;
		for (CvCity* pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop)) {
			if ((pLoopCity->getPopulation() > getPopulation()) ||
					((pLoopCity->getPopulation() == getPopulation()) && (pLoopCity->getID() < getID())))
				iRank++;
		}
		// shenanigans are to get around the const check
		m_bPopulationRankValid = true;
		m_iPopulationRank = iRank; */
		// K-Mod. Set all ranks at the same time.
		const CvPlayer& kPlayer = GET_PLAYER(getOwnerINLINE());

		std::vector<std::pair<int, int> > city_scores;
		int iLoop;
		for (CvCity* pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity; pLoopCity = kPlayer.nextCity(&iLoop))
		{
			city_scores.push_back(std::make_pair(-pLoopCity->getPopulation(), pLoopCity->getID()));
		}
		// note: we are sorting by minimum of _negative_ score, and then by min cityID.
		std::sort(city_scores.begin(), city_scores.end());
		FAssert(city_scores.size() == kPlayer.getNumCities());
		for (size_t i = 0; i < city_scores.size(); i++)
		{
			CvCity* pLoopCity = kPlayer.getCity(city_scores[i].second);
			pLoopCity->m_iPopulationRank = i+1;
			pLoopCity->m_bPopulationRankValid = true;
			// (It's strange that this is allowed. Aren't these values protected or something?)
		}
		FAssert(m_bPopulationRankValid);
		// K-Mod end
	}

	return m_iPopulationRank;
}


int CvCity::findBaseYieldRateRank(YieldTypes eYield) const
{
	if (!m_abBaseYieldRankValid[eYield])
	{
		/* original bts code
		int iRate = getBaseYieldRate(eYield);
		int iRank = 1; int iLoop;
		for (CvCity* pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop)) {
			if ((pLoopCity->getBaseYieldRate(eYield) > iRate) ||
				((pLoopCity->getBaseYieldRate(eYield) == iRate) && (pLoopCity->getID() < getID())))
				iRank++;
		}
		m_abBaseYieldRankValid[eYield] = true;
		m_aiBaseYieldRank[eYield] = iRank; */
		// K-Mod. Set all ranks at the same time.
		const CvPlayer& kPlayer = GET_PLAYER(getOwnerINLINE());

		std::vector<std::pair<int, int> > city_scores;
		int iLoop;
		for (CvCity* pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity; pLoopCity = kPlayer.nextCity(&iLoop))
		{
			city_scores.push_back(std::make_pair(-pLoopCity->getBaseYieldRate(eYield), pLoopCity->getID()));
		}
		// note: we are sorting by minimum of _negative_ score, and then by min cityID.
		std::sort(city_scores.begin(), city_scores.end());
		FAssert(city_scores.size() == kPlayer.getNumCities());
		for (size_t i = 0; i < city_scores.size(); i++)
		{
			CvCity* pLoopCity = kPlayer.getCity(city_scores[i].second);
			pLoopCity->m_aiBaseYieldRank[eYield] = i+1;
			pLoopCity->m_abBaseYieldRankValid[eYield] = true;
			// (It's strange that this is allowed. Aren't these values protected or something?)
		}
		FAssert(m_abBaseYieldRankValid[eYield]);
		// K-Mod end
	}

	return m_aiBaseYieldRank[eYield];
}


int CvCity::findYieldRateRank(YieldTypes eYield) const
{
	if (!m_abYieldRankValid[eYield])
	{
		/* original bts code
		int iRate = getYieldRate(eYield);
		int iRank = 1; int iLoop;
		for (CvCity* pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop)){
			if ((pLoopCity->getYieldRate(eYield) > iRate) ||
				((pLoopCity->getYieldRate(eYield) == iRate) && (pLoopCity->getID() < getID())))
				iRank++;
		}
		m_abYieldRankValid[eYield] = true;
		m_aiYieldRank[eYield] = iRank; */
		// K-Mod. Set all ranks at the same time.
		const CvPlayer& kPlayer = GET_PLAYER(getOwnerINLINE());

		std::vector<std::pair<int, int> > city_scores;
		int iLoop;
		for (CvCity* pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity; pLoopCity = kPlayer.nextCity(&iLoop))
		{
			city_scores.push_back(std::make_pair(-pLoopCity->getYieldRate(eYield), pLoopCity->getID()));
		}
		// note: we are sorting by minimum of _negative_ score, and then by min cityID.
		std::sort(city_scores.begin(), city_scores.end());
		FAssert(city_scores.size() == kPlayer.getNumCities());
		for (size_t i = 0; i < city_scores.size(); i++)
		{
			CvCity* pLoopCity = kPlayer.getCity(city_scores[i].second);
			pLoopCity->m_aiYieldRank[eYield] = i+1;
			pLoopCity->m_abYieldRankValid[eYield] = true;
			// (It's strange that this is allowed. Aren't these values protected or something?)
		}
		FAssert(m_abYieldRankValid[eYield]);
		// K-Mod end
	}

	return m_aiYieldRank[eYield];
}


int CvCity::findCommerceRateRank(CommerceTypes eCommerce) const
{
	if (!m_abCommerceRankValid[eCommerce])
	{
		/* original bts code
		int iRate = getCommerceRateTimes100(eCommerce);
		int iRank = 1; int iLoop;
		for (CvCity* pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop)) {
			if ((pLoopCity->getCommerceRateTimes100(eCommerce) > iRate) ||
					((pLoopCity->getCommerceRateTimes100(eCommerce) == iRate) && (pLoopCity->getID() < getID())))
				iRank++;
		}
		m_abCommerceRankValid[eCommerce] = true;
		m_aiCommerceRank[eCommerce] = iRank; */
		// K-Mod. Set all ranks at the same time.
		const CvPlayer& kPlayer = GET_PLAYER(getOwnerINLINE());

		std::vector<std::pair<int, int> > city_scores;
		int iLoop;
		for (CvCity* pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity; pLoopCity = kPlayer.nextCity(&iLoop))
		{
			city_scores.push_back(std::make_pair(-pLoopCity->getCommerceRateTimes100(eCommerce), pLoopCity->getID()));
		}
		// note: we are sorting by minimum of _negative_ score, and then by min cityID.
		std::sort(city_scores.begin(), city_scores.end());
		FAssert(city_scores.size() == kPlayer.getNumCities());
		for (size_t i = 0; i < city_scores.size(); i++)
		{
			CvCity* pLoopCity = kPlayer.getCity(city_scores[i].second);
			pLoopCity->m_aiCommerceRank[eCommerce] = i+1;
			pLoopCity->m_abCommerceRankValid[eCommerce] = true;
			// (It's strange that this is allowed. Aren't these values protected or something?)
		}
		FAssert(m_abCommerceRankValid[eCommerce]);
		// K-Mod end
	}

	return m_aiCommerceRank[eCommerce];
}


// Returns one of the upgrades...
UnitTypes CvCity::allUpgradesAvailable(UnitTypes eUnit, int iUpgradeCount,
		BonusTypes eAssumeAvailable) const // advc.001u
{
	PROFILE_FUNC(); // advc.003b

	FAssertMsg(eUnit != NO_UNIT, "eUnit is expected to be assigned (not NO_UNIT)");

	if (iUpgradeCount > GC.getNumUnitClassInfos())
	{
		return NO_UNIT;
	}

	UnitTypes eUpgradeUnit = NO_UNIT;
	bool bUpgradeFound = false;
	bool bUpgradeAvailable = false;
	bool bUpgradeUnavailable = false;
	for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		if (GC.getUnitInfo(eUnit).getUpgradeUnitClass(iI))
		{
			UnitTypes eLoopUnit = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI);
			if (eLoopUnit != NO_UNIT)
			{
				bUpgradeFound = true;

				UnitTypes eTempUnit = allUpgradesAvailable(eLoopUnit, iUpgradeCount + 1,
						eAssumeAvailable); // advc.001u
				if (eTempUnit != NO_UNIT)
				{
					eUpgradeUnit = eTempUnit;
					bUpgradeAvailable = true;
				}
				else
				{
					bUpgradeUnavailable = true;
				}
			}
		}
	}

	if (iUpgradeCount > 0)
	{
		if (bUpgradeFound && bUpgradeAvailable)
		{
			FAssertMsg(eUpgradeUnit != NO_UNIT, "eUpgradeUnit is expected to be assigned (not NO_UNIT)");
			return eUpgradeUnit;
		}
		if(canTrain(eUnit, false, false, false, true,
				false, // advc.001b
				eAssumeAvailable)) // advc.001u
			return eUnit;
	}
	else
	{
		if (bUpgradeFound && !bUpgradeUnavailable)
		{
			return eUpgradeUnit;
		}
	}

	return NO_UNIT;
}

// <advc.001b>
bool CvCity::canUpgradeTo(UnitTypes eUnit) const {

	return canTrain(eUnit, false, false, true, false, false);
}// </advc.001b>

bool CvCity::isWorldWondersMaxed() const
{
	if (GC.getGameINLINE().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
	{
		return false;
	}

	if (GC.getDefineINT("MAX_WORLD_WONDERS_PER_CITY") == -1)
	{
		return false;
	}

	if (getNumWorldWonders() >= GC.getDefineINT("MAX_WORLD_WONDERS_PER_CITY"))
	{
		return true;
	}

	return false;
}


bool CvCity::isTeamWondersMaxed() const
{
	if (GC.getGameINLINE().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
	{
		return false;
	}

	if (GC.getDefineINT("MAX_TEAM_WONDERS_PER_CITY") == -1)
	{
		return false;
	}

	if (getNumTeamWonders() >= GC.getDefineINT("MAX_TEAM_WONDERS_PER_CITY"))
	{
		return true;
	}

	return false;
}


bool CvCity::isNationalWondersMaxed() const
{	// <advc.004w>
	int iLeft = getNumNationalWondersLeft();
	if(iLeft < 0)
		return false;
	return (iLeft == 0);
	// Moved into getNumNationalWondersLeft
	/*int iMaxNumWonders = (GC.getGameINLINE().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman()) ? GC.getDefineINT("MAX_NATIONAL_WONDERS_PER_CITY_FOR_OCC") : GC.getDefineINT("MAX_NATIONAL_WONDERS_PER_CITY");
	if(iMaxNumWonders == -1) return false;
	if(getNumNationalWonders() >= iMaxNumWonders) return true;
	return false;*/
}

int CvCity::getNumNationalWondersLeft() const {

	int iMaxNumWonders = (GC.getGameINLINE().isOption(
			GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman()) ?
			GC.getDefineINT("MAX_NATIONAL_WONDERS_PER_CITY_FOR_OCC") :
			GC.getDefineINT("MAX_NATIONAL_WONDERS_PER_CITY");
	if(iMaxNumWonders < 0)
		return -1;
	return std::max(0, iMaxNumWonders - getNumNationalWonders());
} // </advc.004w>


bool CvCity::isBuildingsMaxed() const
{
	if (GC.getGameINLINE().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
	{
		return false;
	}

	if (GC.getDefineINT("MAX_BUILDINGS_PER_CITY") == -1)
	{
		return false;
	}

	if (getNumBuildings() >= GC.getDefineINT("MAX_BUILDINGS_PER_CITY"))
	{
		return true;
	}

	return false;
}

// <advc.064d>
void CvCity::verifyProduction() {

	if(isProduction()) // Only want to address invalid orders here; no production is OK.
		doCheckProduction();
} // </advc.064d>


bool CvCity::canTrain(UnitTypes eUnit, bool bContinue, bool bTestVisible, bool bIgnoreCost, bool bIgnoreUpgrades,
		bool bCheckAirUnitCap, // advc.001b
		BonusTypes eAssumeAvailable) const // advc.001u
{
	PROFILE_FUNC(); // advc.003b

	if(eUnit == NO_UNIT)
		return false;

	if(GC.getUSE_CAN_TRAIN_CALLBACK()) {
		CyCity* pyCity = new CyCity((CvCity*)this);
		CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyCity));
		argsList.add(eUnit);
		argsList.add(bContinue); argsList.add(bTestVisible);
		argsList.add(bIgnoreCost); argsList.add(bIgnoreUpgrades);
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "canTrain", argsList.makeFunctionArgs(), &lResult);
		delete pyCity;
		if (lResult == 1)
			return true;
	}
	/*  <advc.041> Don't allow any ships to be trained at lakes, except
		Work Boat if there are resources in the lake. */
	CvUnitInfo& u = GC.getUnitInfo(eUnit);
	if(u.getDomainType() == DOMAIN_SEA && !isCoastal() &&
			(!u.isPrereqBonuses() || !isPrereqBonusSea()))
		return false; // </advc.041>

	if(!GET_PLAYER(getOwnerINLINE()).canTrain(eUnit, bContinue, bTestVisible, bIgnoreCost))
		return false;

	if(!plot()->canTrain(eUnit, bContinue, bTestVisible,
			bCheckAirUnitCap, // advc.001b
			eAssumeAvailable)) // advc.001u
		return false;

	// advc.003b: Moved down. Seems a bit slower than CvPlot::canTrain.
	if (!bIgnoreUpgrades)
	{
		if (allUpgradesAvailable(eUnit) != NO_UNIT)
		{
			return false;
		}
	}

	if(GC.getUSE_CANNOT_TRAIN_CALLBACK())
	{
		CyCity* pyCity = new CyCity((CvCity*)this);
		CyArgsList argsList2; // XXX
		argsList2.add(gDLL->getPythonIFace()->makePythonObject(pyCity));
		argsList2.add(eUnit);
		argsList2.add(bContinue);
		argsList2.add(bTestVisible);
		argsList2.add(bIgnoreCost);
		argsList2.add(bIgnoreUpgrades);
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "cannotTrain", argsList2.makeFunctionArgs(), &lResult);
		delete pyCity;
		if (lResult == 1)
			return false;
	}

	return true;
}

bool CvCity::canTrain(UnitCombatTypes eUnitCombat) const
{
	for (int i = 0; i < GC.getNumUnitClassInfos(); i++)
	{
		UnitTypes eUnit = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(i);

		if (NO_UNIT != eUnit)
		{
			if (GC.getUnitInfo(eUnit).getUnitCombatType() == eUnitCombat)
			{
				if (canTrain(eUnit))
				{
					return true;
				}
			}
		}
	}

	return false;
}

// advc.003: Refactored
bool CvCity::canConstruct(BuildingTypes eBuilding, bool bContinue,
		bool bTestVisible, bool bIgnoreCost, bool bIgnoreTech) const
{
	if(eBuilding == NO_BUILDING)
		return false;

	if(GC.getUSE_CAN_CONSTRUCT_CALLBACK()) {
		CyCity* pyCity = new CyCity((CvCity*)this);
		CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyCity));
		argsList.add(eBuilding); argsList.add(bContinue);
		argsList.add(bTestVisible); argsList.add(bIgnoreCost);
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "canConstruct", argsList.makeFunctionArgs(), &lResult);
		delete pyCity;
		if(lResult == 1)
			return true;
	}

	if(!GET_PLAYER(getOwnerINLINE()).canConstruct(eBuilding, bContinue, bTestVisible, bIgnoreCost, bIgnoreTech))
		return false;

	if(getNumBuilding(eBuilding) >= GC.getCITY_MAX_NUM_BUILDINGS())
		return false;

	CvBuildingInfo const& bi = GC.getBuildingInfo(eBuilding);
	if( bi.isPrereqReligion())
	{
		//if (getReligionCount() > 0)
		if(getReligionCount() == 0) // K-Mod
			return false;
	}

	if (bi.isStateReligion())
	{
		ReligionTypes eStateReligion = GET_PLAYER(getOwnerINLINE()).getStateReligion();
		if(NO_RELIGION == eStateReligion || !isHasReligion(eStateReligion))
			return false;
	}

	if (bi.getPrereqReligion() != NO_RELIGION)
	{
		if(!isHasReligion((ReligionTypes)(bi.getPrereqReligion())))
			return false;
	}

	CorporationTypes ePrereqCorp = (CorporationTypes)bi.getPrereqCorporation();
	if (ePrereqCorp != NO_CORPORATION)
	{
		if(!isHasCorporation(ePrereqCorp))
			return false;
	}

	CorporationTypes eFoundCorp = (CorporationTypes)bi.getFoundsCorporation();
	if (eFoundCorp != NO_CORPORATION)
	{
		if(GC.getGameINLINE().isCorporationFounded(eFoundCorp))
			return false;

		for (int iCorporation = 0; iCorporation < GC.getNumCorporationInfos(); ++iCorporation)
		{
			CorporationTypes eLoopCorp = (CorporationTypes)iCorporation;
			if (isHeadquarters(eLoopCorp))
			{
				if(GC.getGameINLINE().isCompetingCorporation(eLoopCorp, eFoundCorp))
					return false;
			}
		}
	}

	if(!isValidBuildingLocation(eBuilding))
		return false;

	if (bi.isGovernmentCenter())
	{
		if(isGovernmentCenter())
			return false;
	}

	if (!bTestVisible)
	{
		if (!bContinue)
		{
			if(getFirstBuildingOrder(eBuilding) != -1)
				return false;
		}
		BuildingClassTypes bct = (BuildingClassTypes)(bi.getBuildingClassType());
		if (!(GC.getBuildingClassInfo(bct)).isNoLimit())
		{
			if (isWorldWonderClass(bct))
			{
				if(isWorldWondersMaxed())
					return false;
			}
			else if (isTeamWonderClass(bct))
			{
				if(isTeamWondersMaxed())
					return false;
			}
			else if (isNationalWonderClass(bct))
			{
				if(isNationalWondersMaxed())
					return false;
			}
			else if(isBuildingsMaxed())
				return false;
		}

		if (bi.getHolyCity() != NO_RELIGION)
		{
			if(!isHolyCity(((ReligionTypes)(bi.getHolyCity()))))
				return false;
		}

		if (bi.getPrereqAndBonus() != NO_BONUS)
		{
			if(!hasBonus((BonusTypes)bi.getPrereqAndBonus()))
				return false;
		}

		if (eFoundCorp != NO_CORPORATION)
		{
			if(GC.getGameINLINE().isCorporationFounded(eFoundCorp))
				return false;

			if(GET_PLAYER(getOwnerINLINE()).isNoCorporations())
				return false;

			bool bValid = false;
			for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
			{
				BonusTypes eBonus = (BonusTypes)GC.getCorporationInfo(eFoundCorp).getPrereqBonus(i);
				if (NO_BONUS != eBonus)
				{
					if (hasBonus(eBonus))
					{
						bValid = true;
						break;
					}
				}
			}

			if(!bValid)
				return false;
		}

		if(plot()->getLatitude() > bi.getMaxLatitude())
			return false;
		if(plot()->getLatitude() < bi.getMinLatitude())
			return false;

		bool bRequiresBonus = false;
		bool bNeedsBonus = true;

		for (int iI = 0; iI < GC.getNUM_BUILDING_PREREQ_OR_BONUSES(); iI++)
		{
			if (GC.getBuildingInfo(eBuilding).getPrereqOrBonuses(iI) != NO_BONUS)
			{
				bRequiresBonus = true;
				if(hasBonus((BonusTypes)GC.getBuildingInfo(eBuilding).getPrereqOrBonuses(iI)))
					bNeedsBonus = false;
			}
		}

		if(bRequiresBonus && bNeedsBonus)
			return false;

		for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
		{
			if (bi.isBuildingClassNeededInCity(iI))
			{
				BuildingTypes ePrereqBuilding = (BuildingTypes)
						(GC.getCivilizationInfo(getCivilizationType()).
						getCivilizationBuildings(iI));
				if (ePrereqBuilding != NO_BUILDING)
				{
					if(getNumBuilding(ePrereqBuilding) == 0
							/* && (bContinue || (getFirstBuildingOrder(ePrereqBuilding) == -1))*/)
						return false;
				}
			}
		}
	}

	if(GC.getUSE_CANNOT_CONSTRUCT_CALLBACK()) {
		CyCity* pyCity = new CyCity((CvCity*)this);
		CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyCity));
		argsList.add(eBuilding); argsList.add(bContinue);
		argsList.add(bTestVisible); argsList.add(bIgnoreCost);
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "cannotConstruct", argsList.makeFunctionArgs(), &lResult);
		delete pyCity;
		if(lResult == 1)
			return false;
	}

	return true;
}


bool CvCity::canCreate(ProjectTypes eProject, bool bContinue, bool bTestVisible) const
{
	CyCity* pyCity = new CyCity((CvCity*)this);
	CyArgsList argsList;
	argsList.add(gDLL->getPythonIFace()->makePythonObject(pyCity));
	argsList.add(eProject);
	argsList.add(bContinue);
	argsList.add(bTestVisible);
	long lResult=0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "canCreate", argsList.makeFunctionArgs(), &lResult);
	delete pyCity;
	if (lResult == 1)
		return true;

	if (!(GET_PLAYER(getOwnerINLINE()).canCreate(eProject, bContinue, bTestVisible)))
	{
		return false;
	}

	pyCity = new CyCity((CvCity*)this);
	CyArgsList argsList2; // XXX
	argsList2.add(gDLL->getPythonIFace()->makePythonObject(pyCity));
	argsList2.add(eProject);
	argsList2.add(bContinue);
	argsList2.add(bTestVisible);
	lResult=0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "cannotCreate", argsList2.makeFunctionArgs(), &lResult);
	delete pyCity;
	if (lResult == 1)
		return false;

	return true;
}


bool CvCity::canMaintain(ProcessTypes eProcess, bool bContinue) const
{
	CyCity* pyCity = new CyCity((CvCity*)this);
	CyArgsList argsList;
	argsList.add(gDLL->getPythonIFace()->makePythonObject(pyCity));
	argsList.add(eProcess);
	argsList.add(bContinue);
	long lResult=0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "canMaintain", argsList.makeFunctionArgs(), &lResult);
	delete pyCity;
	if (lResult == 1)
		return true;

	if (!(GET_PLAYER(getOwnerINLINE()).canMaintain(eProcess, bContinue)))
	{
		return false;
	}

	pyCity = new CyCity((CvCity*)this);
	CyArgsList argsList2; // XXX
	argsList2.add(gDLL->getPythonIFace()->makePythonObject(pyCity));
	argsList2.add(eProcess);
	argsList2.add(bContinue);
	lResult=0;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "cannotMaintain", argsList2.makeFunctionArgs(), &lResult);
	delete pyCity; 
	if (lResult == 1)
		return false;

	return true;
}


bool CvCity::canJoin() const
{
	return true;
}


int CvCity::getFoodTurnsLeft() const
{
	int iFoodLeft = (growthThreshold() - getFood());

	if (foodDifference() <= 0)
	{
		return iFoodLeft;
	}

	int iTurnsLeft = (iFoodLeft / foodDifference());

	if ((iTurnsLeft * foodDifference()) <  iFoodLeft)
	{
		iTurnsLeft++;
	}

	return std::max(1, iTurnsLeft);
}


bool CvCity::isProduction() const
{
	return (headOrderQueueNode() != NULL);
}


bool CvCity::isProductionLimited() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		switch (pOrderNode->m_data.eOrderType)
		{
		case ORDER_TRAIN:
			return isLimitedUnitClass((UnitClassTypes)(GC.getUnitInfo((UnitTypes)(pOrderNode->m_data.iData1)).getUnitClassType()));
			break;

		case ORDER_CONSTRUCT:
			return isLimitedWonderClass((BuildingClassTypes)(GC.getBuildingInfo((BuildingTypes)(pOrderNode->m_data.iData1)).getBuildingClassType()));
			break;

		case ORDER_CREATE:
			return isLimitedProject((ProjectTypes)(pOrderNode->m_data.iData1));
			break;

		case ORDER_MAINTAIN:
			break;

		default:
			FAssertMsg(false, "pOrderNode->m_data.eOrderType failed to match a valid option");
			break;
		}
	}

	return false;
}


bool CvCity::isProductionUnit() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		return (pOrderNode->m_data.eOrderType == ORDER_TRAIN);
	}

	return false;
}


bool CvCity::isProductionBuilding() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		return (pOrderNode->m_data.eOrderType == ORDER_CONSTRUCT);
	}

	return false;
}


bool CvCity::isProductionProject() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		return (pOrderNode->m_data.eOrderType == ORDER_CREATE);
	}

	return false;
}


bool CvCity::isProductionProcess() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		return (pOrderNode->m_data.eOrderType == ORDER_MAINTAIN);
	}

	return false;
}


bool CvCity::canContinueProduction(OrderData order)
{
	switch (order.eOrderType)
	{
	case ORDER_TRAIN:
		return canTrain((UnitTypes)(order.iData1), true);
		break;

	case ORDER_CONSTRUCT:
		return canConstruct((BuildingTypes)(order.iData1), true);
		break;

	case ORDER_CREATE:
		return canCreate((ProjectTypes)(order.iData1), true);
		break;

	case ORDER_MAINTAIN:
		return canMaintain((ProcessTypes)(order.iData1), true);
		break;

	default:
		FAssertMsg(false, "order.eOrderType failed to match a valid option");
		break;
	}

	return false;
}


int CvCity::getProductionExperience(UnitTypes eUnit) const
{
	int iExperience = getFreeExperience();
	iExperience += GET_PLAYER(getOwnerINLINE()).getFreeExperience();
	iExperience += getSpecialistFreeExperience(); // K-Mod (moved from below)

	if (eUnit != NO_UNIT)
	{
		if (GC.getUnitInfo(eUnit).getUnitCombatType() != NO_UNITCOMBAT)
		{
			iExperience += getUnitCombatFreeExperience((UnitCombatTypes)(GC.getUnitInfo(eUnit).getUnitCombatType()));
		}
		iExperience += getDomainFreeExperience((DomainTypes)(GC.getUnitInfo(eUnit).getDomainType()));

		//iExperience += getSpecialistFreeExperience();
	}

	if (GET_PLAYER(getOwnerINLINE()).getStateReligion() != NO_RELIGION)
	{
		if (isHasReligion(GET_PLAYER(getOwnerINLINE()).getStateReligion()))
		{
			iExperience += GET_PLAYER(getOwnerINLINE()).getStateReligionFreeExperience();
		}
	}

	return std::max(0, iExperience);
}


void CvCity::addProductionExperience(CvUnit* pUnit, bool bConscript)
{
	if (pUnit->canAcquirePromotionAny())
	{
		pUnit->changeExperience(getProductionExperience(pUnit->getUnitType()) / ((bConscript) ? 2 : 1));
	}

	for (int iI = 0; iI < GC.getNumPromotionInfos(); iI++)
	{
		if (isFreePromotion((PromotionTypes)iI))
		{
			if ((pUnit->getUnitCombatType() != NO_UNITCOMBAT) && GC.getPromotionInfo((PromotionTypes)iI).getUnitCombat(pUnit->getUnitCombatType()))
			{
				pUnit->setHasPromotion(((PromotionTypes)iI), true);
			}
		}
	}

	pUnit->testPromotionReady();
}


UnitTypes CvCity::getProductionUnit() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		switch (pOrderNode->m_data.eOrderType)
		{
		case ORDER_TRAIN:
			return ((UnitTypes)(pOrderNode->m_data.iData1));
			break;

		case ORDER_CONSTRUCT:
		case ORDER_CREATE:
		case ORDER_MAINTAIN:
			break;

		default:
			FAssertMsg(false, "pOrderNode->m_data.eOrderType failed to match a valid option");
			break;
		}
	}

	return NO_UNIT;
}


UnitAITypes CvCity::getProductionUnitAI() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		switch (pOrderNode->m_data.eOrderType)
		{
		case ORDER_TRAIN:
			return ((UnitAITypes)(pOrderNode->m_data.iData2));
			break;

		case ORDER_CONSTRUCT:
		case ORDER_CREATE:
		case ORDER_MAINTAIN:
			break;

		default:
			FAssertMsg(false, "pOrderNode->m_data.eOrderType failed to match a valid option");
			break;
		}
	}

	return NO_UNITAI;
}


BuildingTypes CvCity::getProductionBuilding() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		switch (pOrderNode->m_data.eOrderType)
		{
		case ORDER_TRAIN:
			break;

		case ORDER_CONSTRUCT:
			return ((BuildingTypes)(pOrderNode->m_data.iData1));
			break;

		case ORDER_CREATE:
		case ORDER_MAINTAIN:
			break;

		default:
			FAssertMsg(false, "pOrderNode->m_data.eOrderType failed to match a valid option");
			break;
		}
	}

	return NO_BUILDING;
}


ProjectTypes CvCity::getProductionProject() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		switch (pOrderNode->m_data.eOrderType)
		{
		case ORDER_TRAIN:
		case ORDER_CONSTRUCT:
			break;

		case ORDER_CREATE:
			return ((ProjectTypes)(pOrderNode->m_data.iData1));
			break;

		case ORDER_MAINTAIN:
			break;

		default:
			FAssertMsg(false, "pOrderNode->m_data.eOrderType failed to match a valid option");
			break;
		}
	}

	return NO_PROJECT;
}


ProcessTypes CvCity::getProductionProcess() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		switch (pOrderNode->m_data.eOrderType)
		{
		case ORDER_TRAIN:
		case ORDER_CONSTRUCT:
		case ORDER_CREATE:
			break;

		case ORDER_MAINTAIN:
			return ((ProcessTypes)(pOrderNode->m_data.iData1));
			break;

		default:
			FAssertMsg(false, "pOrderNode->m_data.eOrderType failed to match a valid option");
			break;
		}
	}

	return NO_PROCESS;
}


const wchar* CvCity::getProductionName() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		switch (pOrderNode->m_data.eOrderType)
		{
		case ORDER_TRAIN:
			return GC.getUnitInfo((UnitTypes) pOrderNode->m_data.iData1).getDescription();
			break;

		case ORDER_CONSTRUCT:
			return GC.getBuildingInfo((BuildingTypes) pOrderNode->m_data.iData1).getDescription();
			break;

		case ORDER_CREATE:
			return GC.getProjectInfo((ProjectTypes) pOrderNode->m_data.iData1).getDescription();
			break;

		case ORDER_MAINTAIN:
			return GC.getProcessInfo((ProcessTypes) pOrderNode->m_data.iData1).getDescription();
			break;

		default:
			FAssertMsg(false, "pOrderNode->m_data.eOrderType failed to match a valid option");
			break;
		}
	}

	return L"";
}


int CvCity::getGeneralProductionTurnsLeft() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		switch (pOrderNode->m_data.eOrderType)
		{
		case ORDER_TRAIN:
			return getProductionTurnsLeft((UnitTypes)pOrderNode->m_data.iData1, 0);
			break;

		case ORDER_CONSTRUCT:
			return getProductionTurnsLeft((BuildingTypes)pOrderNode->m_data.iData1, 0);
			break;

		case ORDER_CREATE:
			return getProductionTurnsLeft((ProjectTypes)pOrderNode->m_data.iData1, 0);
			break;

		case ORDER_MAINTAIN:
			return 0;
			break;

		default:
			FAssertMsg(false, "pOrderNode->m_data.eOrderType failed to match a valid option");
			break;
		}
	}

	return 0;
}


const wchar* CvCity::getProductionNameKey() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		switch (pOrderNode->m_data.eOrderType)
		{
		case ORDER_TRAIN:
			return GC.getUnitInfo((UnitTypes) pOrderNode->m_data.iData1).getTextKeyWide();
			break;

		case ORDER_CONSTRUCT:
			return GC.getBuildingInfo((BuildingTypes) pOrderNode->m_data.iData1).getTextKeyWide();
			break;

		case ORDER_CREATE:
			return GC.getProjectInfo((ProjectTypes) pOrderNode->m_data.iData1).getTextKeyWide();
			break;

		case ORDER_MAINTAIN:
			return GC.getProcessInfo((ProcessTypes) pOrderNode->m_data.iData1).getTextKeyWide();
			break;

		default:
			FAssertMsg(false, "pOrderNode->m_data.eOrderType failed to match a valid option");
			break;
		}
	}

	return L"";
}


bool CvCity::isFoodProduction() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		switch (pOrderNode->m_data.eOrderType)
		{
		case ORDER_TRAIN:
			return isFoodProduction((UnitTypes)(pOrderNode->m_data.iData1));
			break;

		case ORDER_CONSTRUCT:
		case ORDER_CREATE:
		case ORDER_MAINTAIN:
			break;

		default:
			FAssertMsg(false, "pOrderNode->m_data.eOrderType failed to match a valid option");
			break;
		}
	}

	return false;
}


bool CvCity::isFoodProduction(UnitTypes eUnit) const
{
	if (GC.getUnitInfo(eUnit).isFoodProduction())
	{
		return true;
	}

	if (GET_PLAYER(getOwnerINLINE()).isMilitaryFoodProduction())
	{
		if (GC.getUnitInfo(eUnit).isMilitaryProduction())
		{
			return true;
		}
	}

	return false;
}


int CvCity::getFirstUnitOrder(UnitTypes eUnit) const
{
	int iCount = 0;

	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	while (pOrderNode != NULL)
	{
		if (pOrderNode->m_data.eOrderType == ORDER_TRAIN)
		{
			if (pOrderNode->m_data.iData1 == eUnit)
			{
				return iCount;
			}
		}

		iCount++;

		pOrderNode = nextOrderQueueNode(pOrderNode);
	}

	return -1;
}


int CvCity::getFirstBuildingOrder(BuildingTypes eBuilding) const
{
	int iCount = 0;

	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	while (pOrderNode != NULL)
	{
		if (pOrderNode->m_data.eOrderType == ORDER_CONSTRUCT)
		{
			if (pOrderNode->m_data.iData1 == eBuilding)
			{
				return iCount;
			}
		}

		iCount++;

		pOrderNode = nextOrderQueueNode(pOrderNode);
	}

	return -1;
}


int CvCity::getFirstProjectOrder(ProjectTypes eProject) const
{
	int iCount = 0;

	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	while (pOrderNode != NULL)
	{
		if (pOrderNode->m_data.eOrderType == ORDER_CREATE)
		{
			if (pOrderNode->m_data.iData1 == eProject)
			{
				return iCount;
			}
		}

		iCount++;

		pOrderNode = nextOrderQueueNode(pOrderNode);
	}

	return -1;
}


int CvCity::getNumTrainUnitAI(UnitAITypes eUnitAI) const
{
	int iCount = 0;

	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	while (pOrderNode != NULL)
	{
		if (pOrderNode->m_data.eOrderType == ORDER_TRAIN)
		{
			if (pOrderNode->m_data.iData2 == eUnitAI)
			{
				iCount++;
			}
		}

		pOrderNode = nextOrderQueueNode(pOrderNode);
	}

	return iCount;
}


int CvCity::getProduction() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		switch (pOrderNode->m_data.eOrderType)
		{
		case ORDER_TRAIN:
			return getUnitProduction((UnitTypes)(pOrderNode->m_data.iData1));
			break;

		case ORDER_CONSTRUCT:
			return getBuildingProduction((BuildingTypes)(pOrderNode->m_data.iData1));
			break;

		case ORDER_CREATE:
			return getProjectProduction((ProjectTypes)(pOrderNode->m_data.iData1));
			break;

		case ORDER_MAINTAIN:
			break;

		default:
			FAssertMsg(false, "pOrderNode->m_data.eOrderType failed to match a valid option");
			break;
		}
	}

	return 0;
}


int CvCity::getProductionNeeded() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		switch (pOrderNode->m_data.eOrderType)
		{
		case ORDER_TRAIN:
			return getProductionNeeded((UnitTypes)(pOrderNode->m_data.iData1));
			break;

		case ORDER_CONSTRUCT:
			return getProductionNeeded((BuildingTypes)(pOrderNode->m_data.iData1));
			break;

		case ORDER_CREATE:
			return getProductionNeeded((ProjectTypes)(pOrderNode->m_data.iData1));
			break;

		case ORDER_MAINTAIN:
			break;

		default:
			FAssertMsg(false, "pOrderNode->m_data.eOrderType failed to match a valid option");
			break;
		}
	}

	return MAX_INT;
}

int CvCity::getProductionNeeded(UnitTypes eUnit) const
{
	return GET_PLAYER(getOwnerINLINE()).getProductionNeeded(eUnit);
}

int CvCity::getProductionNeeded(BuildingTypes eBuilding) const
{
	int iProductionNeeded = GET_PLAYER(getOwnerINLINE()).getProductionNeeded(eBuilding);

	// Python cost modifier
	if (GC.getUSE_GET_BUILDING_COST_MOD_CALLBACK())
	{
		CyArgsList argsList;
		argsList.add(getOwnerINLINE());	// Player ID
		argsList.add(getID());	// City ID
		argsList.add(eBuilding);	// Building ID
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "getBuildingCostMod", argsList.makeFunctionArgs(), &lResult);

		if (lResult > 1)
		{
			iProductionNeeded *= lResult;
			iProductionNeeded /= 100;
		}
	}

	return iProductionNeeded;
}

int CvCity::getProductionNeeded(ProjectTypes eProject) const
{
	return GET_PLAYER(getOwnerINLINE()).getProductionNeeded(eProject);
}

int CvCity::getProductionTurnsLeft() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		switch (pOrderNode->m_data.eOrderType)
		{
		case ORDER_TRAIN:
			return getProductionTurnsLeft(((UnitTypes)(pOrderNode->m_data.iData1)), 0);
			break;

		case ORDER_CONSTRUCT:
			return getProductionTurnsLeft(((BuildingTypes)(pOrderNode->m_data.iData1)), 0);
			break;

		case ORDER_CREATE:
			return getProductionTurnsLeft(((ProjectTypes)(pOrderNode->m_data.iData1)), 0);
			break;

		case ORDER_MAINTAIN:
			break;

		default:
			FAssertMsg(false, "pOrderNode->m_data.eOrderType failed to match a valid option");
			break;
		}
	}

	return MAX_INT;
}


int CvCity::getProductionTurnsLeft(UnitTypes eUnit, int iNum) const
{
	int iProduction = 0;

	int iFirstUnitOrder = getFirstUnitOrder(eUnit);

	if ((iFirstUnitOrder == -1) || (iFirstUnitOrder == iNum))
	{
		iProduction += getUnitProduction(eUnit);
	}

	int iProductionNeeded = getProductionNeeded(eUnit);
	int iProductionModifier = getProductionModifier(eUnit);
	// advc.064b: Moved into (yet another) auxiliary function
	return getProductionTurnsLeft(iProductionNeeded, iProduction,
			iProductionModifier, isFoodProduction(eUnit), iNum);
}


int CvCity::getProductionTurnsLeft(BuildingTypes eBuilding, int iNum) const
{
	int iProduction = 0;

	int iFirstBuildingOrder = getFirstBuildingOrder(eBuilding);

	if ((iFirstBuildingOrder == -1) || (iFirstBuildingOrder == iNum))
	{
		iProduction += getBuildingProduction(eBuilding);
	}

	int iProductionNeeded = getProductionNeeded(eBuilding);
	int iProductionModifier = getProductionModifier(eBuilding);
	// advc.064b:
	return getProductionTurnsLeft(iProductionNeeded, iProduction,
			iProductionModifier, false, iNum);
}


int CvCity::getProductionTurnsLeft(ProjectTypes eProject, int iNum) const
{
	int iProduction = 0;

	int iFirstProjectOrder = getFirstProjectOrder(eProject);

	if ((iFirstProjectOrder == -1) || (iFirstProjectOrder == iNum))
	{
		iProduction += getProjectProduction(eProject);
	}

	int iProductionNeeded = getProductionNeeded(eProject);
	int iProductionModifier = getProductionModifier(eProject);
	// advc.064b:
	return getProductionTurnsLeft(iProductionNeeded, iProduction,
			iProductionModifier, false, iNum);
}

/*  <advc.064b> New auxiliary function; some code cut from
	getProductionTurnsLeft(UnitTypes,int). Added a bit of code to predict
	overflow for queued orders (iNum>0); BtS never did that. */
int CvCity::getProductionTurnsLeft(int iProductionNeeded, int iProduction,
		int iProductionModifier, bool bFoodProduction, int iNum) const {

	int iFirstProductionDifference = 0;
	// Per-turn production assuming no overflow and feature production
	int iProductionDifference = getProductionDifference(iProductionNeeded,
			iProduction, iProductionModifier, bFoodProduction, false, true);
	if(iNum > 1) {
		// Not going to predict overflow here
		iFirstProductionDifference = iProductionDifference;
	}
	else if(iNum == 1 && getProductionTurnsLeft() <= 1) {
		// Current production about to finish; predict overflow.
		OrderTypes eCurrentOrder = getOrderData(0).eOrderType;
		if(eCurrentOrder == ORDER_TRAIN || eCurrentOrder == ORDER_CONSTRUCT ||
				eCurrentOrder == ORDER_CREATE) {
			int iFeatureProduction = 0;
			int iCurrentProductionDifference = getCurrentProductionDifference(false,
					true, false, false, false, &iFeatureProduction);
			int iRawOverflow = getProduction() + iCurrentProductionDifference - getProductionNeeded();
			int iOverflow = computeOverflow(iRawOverflow, getProductionModifier(),
					eCurrentOrder);
			/*  This ignores that some production modifiers are applied to overflow,
				but it's certainly better than ignoring overflow altogether. */
			iFirstProductionDifference = iProductionDifference + iOverflow +
					((getFeatureProduction() - iFeatureProduction) *
					(100 + iProductionModifier)) / 100;
		}
	}
	else if(iNum == 1)
		iFirstProductionDifference = iProductionDifference;
	if(iNum <= 0) {
		iFirstProductionDifference = getProductionDifference(iProductionNeeded,
				iProduction, iProductionModifier, bFoodProduction, true, false);
	}
	return getProductionTurnsLeft(iProductionNeeded, iProduction,
			iFirstProductionDifference, iProductionDifference);
} // </advc.064b>

// <advc.064d> Body cut from popOrder
void CvCity::doPopOrder(CLLNode<OrderData>* pOrder) {

	bool bStart = false;
	if(pOrder == headOrderQueueNode()) {
		bStart = true;
		stopHeadOrder();
	}
	m_orderQueue.deleteNode(pOrder);
	if(bStart)
		startHeadOrder();
} // </advc.064d>

int CvCity::getProductionTurnsLeft(int iProductionNeeded, int iProduction, int iFirstProductionDifference, int iProductionDifference) const
{
	int iProductionLeft = std::max(0,
			iProductionNeeded - iProduction - iFirstProductionDifference);

	if (iProductionDifference == 0) {
		//return iProductionLeft + 1;
		return MAX_INT; // advc.004x
	}
	int iTurnsLeft = (iProductionLeft / iProductionDifference);
	if (iTurnsLeft * iProductionDifference < iProductionLeft)
		iTurnsLeft++; // advc (comment): rounds up

	iTurnsLeft++;

	return std::max(1, iTurnsLeft);
}

// <advc.004x>
int CvCity::sanitizeProductionTurns(int iTurns, OrderTypes eOrder, int iData,
		bool bAssert) const {

	if(iTurns < MAX_INT)
		return iTurns;
	FAssert(!bAssert);
	int r = 1;
	switch(eOrder) {
	case ORDER_TRAIN: return r + getProductionNeeded((UnitTypes)iData);
	case ORDER_CONSTRUCT: return r + getProductionNeeded((BuildingTypes)iData);
	case ORDER_CREATE: return r + getProductionNeeded((ProjectTypes)iData);
	}
	return r + getProductionNeeded();
} // </advc.004x>


void CvCity::setProduction(int iNewValue)
{
	if (isProductionUnit())
	{
		setUnitProduction(getProductionUnit(), iNewValue);
	}
	else if (isProductionBuilding())
	{
		setBuildingProduction(getProductionBuilding(), iNewValue);
	}
	else if (isProductionProject())
	{
		setProjectProduction(getProductionProject(), iNewValue);
	}
	mrOrder = -1; mrWasUnit = false; // advc.004x
}


void CvCity::changeProduction(int iChange)
{
	if (isProductionUnit())
	{
		changeUnitProduction(getProductionUnit(), iChange);
	}
	else if (isProductionBuilding())
	{
		changeBuildingProduction(getProductionBuilding(), iChange);
	}
	else if (isProductionProject())
	{
		changeProjectProduction(getProductionProject(), iChange);
	}
}


int CvCity::getProductionModifier() const
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		switch (pOrderNode->m_data.eOrderType)
		{
		case ORDER_TRAIN:
			return getProductionModifier((UnitTypes)(pOrderNode->m_data.iData1));
			break;

		case ORDER_CONSTRUCT:
			return getProductionModifier((BuildingTypes)(pOrderNode->m_data.iData1));
			break;

		case ORDER_CREATE:
			return getProductionModifier((ProjectTypes)(pOrderNode->m_data.iData1));
			break;

		case ORDER_MAINTAIN:
			break;

		default:
			FAssertMsg(false, "pOrderNode->m_data.eOrderType failed to match a valid option");
			break;
		}
	}

	return 0;
}


int CvCity::getProductionModifier(UnitTypes eUnit) const
{
	int iMultiplier = GET_PLAYER(getOwnerINLINE()).getProductionModifier(eUnit);

	iMultiplier += getDomainProductionModifier((DomainTypes)(GC.getUnitInfo(eUnit).getDomainType()));

	if (GC.getUnitInfo(eUnit).isMilitaryProduction())
	{
		iMultiplier += getMilitaryProductionModifier();
	}

	for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
	{
		if (hasBonus((BonusTypes)iI))
		{
			iMultiplier += GC.getUnitInfo(eUnit).getBonusProductionModifier(iI);
		}
	}

	if (GET_PLAYER(getOwnerINLINE()).getStateReligion() != NO_RELIGION)
	{
		if (isHasReligion(GET_PLAYER(getOwnerINLINE()).getStateReligion()))
		{
			iMultiplier += GET_PLAYER(getOwnerINLINE()).getStateReligionUnitProductionModifier();
		}
	}
	//return std::max(0, iMultiplier);
	return std::max(-50, iMultiplier); // UNOFFICIAL_PATCH (for mods), 05/10/10, jdog5000
}


int CvCity::getProductionModifier(BuildingTypes eBuilding) const
{
	int iMultiplier = GET_PLAYER(getOwnerINLINE()).getProductionModifier(eBuilding);

	for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
	{
		if (hasBonus((BonusTypes)iI))
		{
			iMultiplier += GC.getBuildingInfo(eBuilding).getBonusProductionModifier(iI);
		}
	}

	if (GET_PLAYER(getOwnerINLINE()).getStateReligion() != NO_RELIGION)
	{
		if (isHasReligion(GET_PLAYER(getOwnerINLINE()).getStateReligion()))
		{
			iMultiplier += GET_PLAYER(getOwnerINLINE()).getStateReligionBuildingProductionModifier();
		}
	}
	// return std::max(0, iMultiplier);
	return std::max(-50, iMultiplier); // UNOFFICIAL_PATCH (for mods), 05/10/10, jdog5000
}


int CvCity::getProductionModifier(ProjectTypes eProject) const
{
	int iMultiplier = GET_PLAYER(getOwnerINLINE()).getProductionModifier(eProject);

	if (GC.getProjectInfo(eProject).isSpaceship())
	{
		iMultiplier += getSpaceProductionModifier();
	}

	for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
	{
		if (hasBonus((BonusTypes)iI))
		{
			iMultiplier += GC.getProjectInfo(eProject).getBonusProductionModifier(iI);
		}
	}
	// return std::max(0, iMultiplier);
	return std::max(-50, iMultiplier); // UNOFFICIAL_PATCH (for mods), 05/10/10, jdog5000
}


int CvCity::getProductionDifference(int iProductionNeeded, int iProduction,
		int iProductionModifier, bool bFoodProduction, bool bOverflow,
		// <advc.064bc>
		bool bIgnoreFeatureProd, bool bIgnoreYieldRate,
		bool bForceFeatureProd, int* piFeatureProd) const { // </advc.064bc>

	if (isDisorder())
		return 0;

	int iFoodProduction = (bFoodProduction ?
			std::max(0, getYieldRate(YIELD_FOOD) - foodConsumption()) : 0);

	int iModifier = getBaseYieldRateModifier(YIELD_PRODUCTION, iProductionModifier);
	int iRate = /* advc.064: */ (bIgnoreYieldRate ? 0 :
			getBaseYieldRate(YIELD_PRODUCTION));
	//int iOverflow = (bOverflow ? (getOverflowProduction() + getFeatureProduction()) : 0);
	// <advc.064b> Replacing the above
	int iOverflow = 0;
	if(bOverflow)
		iOverflow = getOverflowProduction();
	int iFeatureProduction = 0;
	FAssert(!bIgnoreFeatureProd || (!bForceFeatureProd && piFeatureProd == NULL));
	if(!bIgnoreFeatureProd) {
		if(bForceFeatureProd)
			iFeatureProduction = getFeatureProduction();
		else { /* Compute needed feature production (derived from the formula in the
				  return statement) */
			// Don't know what to call this; I need it in order to handle rounding.
			int iTmp = 100 * (iProductionNeeded - iProduction - iFoodProduction) -
					iOverflow * (100 + iProductionModifier);
			int iFeatureProdNeeded =  iTmp / iModifier - iRate;
			if(iTmp % iModifier != 0)
				iFeatureProdNeeded++;
			iFeatureProduction = ::range(iFeatureProdNeeded, 0, getFeatureProduction());
		}
	}
	if(piFeatureProd != NULL)
		*piFeatureProd = iFeatureProduction;
	/*  Replacing the BtS formula below. The BaseYieldRateModifier is now already
		applied when the overflow is generated; see comment in unmodifyOverflow. */
	return iFoodProduction + ((iRate + iFeatureProduction) * iModifier +
			iOverflow * (100 + iProductionModifier)) / 100;
	// </advc.064b>
	//return ((iRate + iOverflow) * iModifier) / 100 + iFoodProduction;
}


int CvCity::getCurrentProductionDifference(bool bIgnoreFood, bool bOverflow,
		// <advc.064bc>
		bool bIgnoreFeatureProd, bool bIgnoreYieldRate,
		bool bForceFeatureProd, int* piFeatureProd) const // </advc.064bc>
{
	return getProductionDifference(getProductionNeeded(), getProduction(),
			getProductionModifier(), !bIgnoreFood && isFoodProduction(), bOverflow,
			bIgnoreFeatureProd, bIgnoreYieldRate, bForceFeatureProd, piFeatureProd);
}


int CvCity::getExtraProductionDifference(int iExtra) const
{
	return getExtraProductionDifference(iExtra, getProductionModifier());
}

int CvCity::getExtraProductionDifference(int iExtra, UnitTypes eUnit) const
{
	return getExtraProductionDifference(iExtra, getProductionModifier(eUnit));
}

int CvCity::getExtraProductionDifference(int iExtra, BuildingTypes eBuilding) const
{
	return getExtraProductionDifference(iExtra, getProductionModifier(eBuilding));
}

int CvCity::getExtraProductionDifference(int iExtra, ProjectTypes eProject) const
{
	return getExtraProductionDifference(iExtra, getProductionModifier(eProject));
}

int CvCity::getExtraProductionDifference(int iExtra, int iModifier) const
{
	return ((iExtra * getBaseYieldRateModifier(YIELD_PRODUCTION, iModifier)) / 100);
}


bool CvCity::canHurry(HurryTypes eHurry, bool bTestVisible) const
{
	if(!GET_PLAYER(getOwnerINLINE()).canHurry(eHurry) &&
			// <advc.912d>
			(GC.getHurryInfo(eHurry).getProductionPerPopulation() <= 0 ||
			!canPopRush())) // </advc.912d>
		return false;
	if(isDisorder())
		return false;
	// advc.064b: Add overflow and features
	if(getCurrentProductionDifference(true, true, false, true, true)
			+ getProduction() >= getProductionNeeded())
		return false;
	// K-Mod. moved this check outside of !bTestVisible.
	if(!isProductionUnit() && !isProductionBuilding())
		return false;
	if(!bTestVisible) {
		// <advc.064b> Only the iHurryGold <= 0 check is new
		if(GC.getHurryInfo(eHurry).getGoldPerProduction() > 0) {
			int iHurryGold = hurryGold(eHurry);
			if(iHurryGold <= 0)
				return false;
			if(GET_PLAYER(getOwnerINLINE()).getGold() < iHurryGold)
				return false;
		} // </advc.064b>
		if(maxHurryPopulation() < hurryPopulation(eHurry))
			return false;
	}
	return true;
}

/*  advc.003j (comment): Not currently called b/c of a K-Mod change in
	CvCityAI::AI_bestUnitAI. Tbd.: I wonder if karadoc's reasoning ("inaccurate")
	also applies to calls to canHurryBuilding. */
bool CvCity::canHurryUnit(HurryTypes eHurry, UnitTypes eUnit, bool bIgnoreNew) const
{
	if (!GET_PLAYER(getOwnerINLINE()).canHurry(eHurry) &&
			// <advc.912d>
			(GC.getHurryInfo(eHurry).getProductionPerPopulation() <= 0 ||
			!canPopRush())) // </advc.912d>
	{
		return false;
	}

	if (isDisorder())
	{
		return false;
	}

	if (getUnitProduction(eUnit) >= getProductionNeeded(eUnit))
	{
		return false;
	}

	if (GET_PLAYER(getOwnerINLINE()).getGold() < getHurryGold(eHurry, getHurryCost(false, eUnit, bIgnoreNew)))
	{
		return false;
	}

	if (maxHurryPopulation() < getHurryPopulation(eHurry, getHurryCost(true, eUnit, bIgnoreNew)))
	{
		return false;
	}

	return true;
}

bool CvCity::canHurryBuilding(HurryTypes eHurry, BuildingTypes eBuilding, bool bIgnoreNew) const
{
	if (!GET_PLAYER(getOwnerINLINE()).canHurry(eHurry) &&
			// <advc.912d>
			(GC.getHurryInfo(eHurry).getProductionPerPopulation() <= 0 ||
			!canPopRush())) // </advc.912d>
	{
		return false;
	}

	if (isDisorder())
	{
		return false;
	}

	if (getBuildingProduction(eBuilding) >= getProductionNeeded(eBuilding))
	{
		return false;
	}

	if (GET_PLAYER(getOwnerINLINE()).getGold() < getHurryGold(eHurry, getHurryCost(false, eBuilding, bIgnoreNew)))
	{
		return false;
	}

	if (maxHurryPopulation() < getHurryPopulation(eHurry, getHurryCost(true, eBuilding, bIgnoreNew)))
	{
		return false;
	}

	return true;
}


void CvCity::hurry(HurryTypes eHurry)
{
	if (!canHurry(eHurry))
		return;

	int iHurryGold = hurryGold(eHurry);
	int iHurryPopulation = hurryPopulation(eHurry);
	int iHurryAngerLength = hurryAngerLength(eHurry);

	changeProduction(hurryProduction(eHurry));
	GET_PLAYER(getOwnerINLINE()).changeGold(-(iHurryGold));
	changePopulation(-(iHurryPopulation));

	changeHurryAngerTimer(iHurryAngerLength);

	if( gCityLogLevel >= 2 ) { // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
		CvWStringBuffer szBuffer; CvWString szString;
		if (isProductionUnit())
			szString = GC.getUnitInfo(getProductionUnit()).getDescription();
		else if (isProductionBuilding())
			szString = GC.getBuildingInfo(getProductionBuilding()).getDescription();
		else if (isProductionProject())
			szString = GC.getProjectInfo(getProductionProject()).getDescription();
		logBBAI("    City %S hurrying production of %S at cost of %d pop, %d gold, %d anger length", getName().GetCString(), szString.GetCString(), iHurryPopulation, iHurryGold, iHurryAngerLength );
	}

	if (getOwnerINLINE() == GC.getGameINLINE().getActivePlayer() && isCitySelected())
		gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);

	CvEventReporter::getInstance().cityHurry(this, eHurry);
}

// <advc.064b>
int CvCity::overflowCapacity(int iProductionModifier, int iPopulationChange) const {

	int iBound1 = getCurrentProductionDifference(false, false, true); // as in BtS/K-Mod
	// New: Take out build-specific modifiers
	iBound1 = unmodifyOverflow(iBound1, iProductionModifier);
	// Was the production cost of the completed order in BtS/K-Mod
	int iBound2 = growthThreshold(iPopulationChange);
	return std::max(iBound1, iBound2);
}

// Based on code (BtS/unoffical patch) cut from popOrder
int CvCity::computeOverflow(int iRawOverflow, int iProductionModifier,
		OrderTypes eOrderType, int* piProductionGold, int* piLostProduction,
		int iPopulationChange) const {

	if(piProductionGold != NULL)
		*piProductionGold = 0;
	if(piLostProduction != NULL)
		*piLostProduction = 0;
	/*  (BtS and the unofficial patch cap overflow before factoring out the modifiers.
		However, they also apply generic production modifiers to the overflow once
		work on the next order starts, which AdvCiv no longer does.) */
	int iOverflow = unmodifyOverflow(iRawOverflow, iProductionModifier);
	int iMaxOverflow = overflowCapacity(iProductionModifier, iPopulationChange);
	int iLostProduction = iOverflow - iMaxOverflow;
	iOverflow = std::min(iMaxOverflow, iOverflow);
	if(iLostProduction <= 0)
		return iOverflow;
	if(piLostProduction != NULL)
		*piLostProduction = iLostProduction;
	if(piProductionGold != NULL)
		*piProductionGold = (iLostProduction * failGoldPercent(eOrderType)) / 100;
	return iOverflow;
} // </advc.064b>

// BUG - Hurry Overflow - start (advc.064)
bool CvCity::hurryOverflow(HurryTypes eHurry, int* piProduction, int* piGold,
		bool bCountThisTurn) const {

	if(piProduction != NULL)
		*piProduction = 0;
	if(piGold != NULL)
		*piGold = 0;

	if(!canHurry(eHurry))
		return false;

	if(GC.getHurryInfo(eHurry).getProductionPerPopulation() == 0)
		return true;

	int iTotal, iCurrent, iModifier;
	OrderTypes eOrder = NO_ORDER; // advc.064b
	if(isProductionUnit()) {
		UnitTypes eUnit = getProductionUnit();
		iTotal = getProductionNeeded(eUnit);
		iCurrent = getUnitProduction(eUnit);
		iModifier = getProductionModifier(eUnit);
		eOrder = ORDER_TRAIN;
	}
	else if(isProductionBuilding()) {
		BuildingTypes eBuilding = getProductionBuilding();
		iTotal = getProductionNeeded(eBuilding);
		iCurrent = getBuildingProduction(eBuilding);
		iModifier = getProductionModifier(eBuilding);
		eOrder = ORDER_CONSTRUCT;
	}
	else if (isProductionProject()) {
		ProjectTypes eProject = getProductionProject();
		iTotal = getProductionNeeded(eProject);
		iCurrent = getProjectProduction(eProject);
		iModifier = getProductionModifier(eProject);
		eOrder = ORDER_CREATE;
	}
	else return false;
	int iHurry = hurryProduction(eHurry);
	// <advc.064b>
	int iRawOverflow = iCurrent + iHurry - iTotal +
			/*  BUG ignores chops and previous overflow if !bCountThisTurn, but these
				are entirely predictable, so they shouldn't depend on the "this turn"
				option. However: Feature production can no longer contribute to
				overflow, so ignore that in any case. */
			getCurrentProductionDifference(!bCountThisTurn, true, true, !bCountThisTurn);
	*piProduction = computeOverflow(iRawOverflow, iModifier, eOrder, piGold, NULL,
			-hurryPopulation(eHurry)); // </advc.064b>
	return true;
} // BUG - Hurry Overflow - end

// <advc.912d>
bool CvCity::canPopRush() const {

	return (m_iPopRushHurryCount > 0 || GET_PLAYER(getOwnerINLINE()).canPopRush());
}

void CvCity::changePopRushCount(int iChange) {

	m_iPopRushHurryCount += iChange;
} // </advc.912d>


UnitTypes CvCity::getConscriptUnit() const
{
	int iBestValue = 0;
	UnitTypes eBestUnit = NO_UNIT;

	for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		UnitTypes eLoopUnit = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI);
		if (eLoopUnit != NO_UNIT)
		{
			if (canTrain(eLoopUnit))
			{
				int iValue = GC.getUnitInfo(eLoopUnit).getConscriptionValue();
				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					eBestUnit = eLoopUnit;
				}
			}
		}
	}

	CyArgsList argsList;
	argsList.add(getOwnerINLINE());
	long lConscriptUnit = -1;
	gDLL->getPythonIFace()->callFunction(PYGameModule, "getConscriptUnitType", argsList.makeFunctionArgs(),&lConscriptUnit);
	if (lConscriptUnit != -1)
		eBestUnit = ((UnitTypes)lConscriptUnit);

	return eBestUnit;
}


int CvCity::getConscriptPopulation() const
{
	UnitTypes eConscriptUnit = getConscriptUnit();

	if (eConscriptUnit == NO_UNIT)
	{
		return 0;
	}

	if (GC.getDefineINT("CONSCRIPT_POPULATION_PER_COST") == 0)
	{
		return 0;
	}

	return std::max(1, ((GC.getUnitInfo(eConscriptUnit).getProductionCost()) / GC.getDefineINT("CONSCRIPT_POPULATION_PER_COST")));
}


int CvCity::conscriptMinCityPopulation() const
{
	int iPopulation;

	iPopulation = GC.getDefineINT("CONSCRIPT_MIN_CITY_POPULATION");

	iPopulation += getConscriptPopulation();

	return iPopulation;
}


int CvCity::flatConscriptAngerLength() const
{
	int iAnger = GC.getDefineINT("CONSCRIPT_ANGER_DIVISOR");

	iAnger *= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getHurryConscriptAngerPercent();
	iAnger /= 100;

	return std::max(1, iAnger);
}


bool CvCity::canConscript() const
{
	if (isDisorder())
	{
		return false;
	}

	if (isDrafted())
	{
		return false;
	}

	if (GET_PLAYER(getOwnerINLINE()).getConscriptCount() >= GET_PLAYER(getOwnerINLINE()).getMaxConscript())
	{
		return false;
	}

	if (getPopulation() <= getConscriptPopulation())
	{
		return false;
	}

	if (getPopulation() < conscriptMinCityPopulation())
	{
		return false;
	}

	if (plot()->calculateTeamCulturePercent(getTeam()) < GC.getDefineINT("CONSCRIPT_MIN_CULTURE_PERCENT"))
	{
		return false;
	}

	if (getConscriptUnit() == NO_UNIT)
	{
		return false;
	}

	return true;
}

CvUnit* CvCity::initConscriptedUnit()
{
	UnitAITypes eCityAI = NO_UNITAI;
	UnitTypes eConscriptUnit = getConscriptUnit();

	if (NO_UNIT == eConscriptUnit)
	{
		return NULL;
	}

	if (GET_PLAYER(getOwnerINLINE()).AI_unitValue(eConscriptUnit, UNITAI_ATTACK, area()) > 0) 
	{ 
		eCityAI = UNITAI_ATTACK; 
	} 
	else if (GET_PLAYER(getOwnerINLINE()).AI_unitValue(eConscriptUnit, UNITAI_CITY_DEFENSE, area()) > 0) 
	{ 
		eCityAI = UNITAI_CITY_DEFENSE; 
	} 
	else if (GET_PLAYER(getOwnerINLINE()).AI_unitValue(eConscriptUnit, UNITAI_CITY_COUNTER, area()) > 0)
	{
		eCityAI = UNITAI_CITY_COUNTER;
	}
	else if (GET_PLAYER(getOwnerINLINE()).AI_unitValue(eConscriptUnit, UNITAI_CITY_SPECIAL, area()) > 0)
	{
		eCityAI = UNITAI_CITY_SPECIAL;
	}
	else
	{
		eCityAI = NO_UNITAI;
	}

	CvUnit* pUnit = GET_PLAYER(getOwnerINLINE()).initUnit(eConscriptUnit, getX_INLINE(), getY_INLINE(), eCityAI);
	FAssertMsg(pUnit != NULL, "pUnit expected to be assigned (not NULL)");

	if (NULL != pUnit)
	{
		addProductionExperience(pUnit, true);

		pUnit->setMoves(0);
// K-Mod, karadoc, 26/Jun/2011: Conscription counts as building the unit
		CvEventReporter::getInstance().unitBuilt(this, pUnit);
// K-Mod end
	}

	return pUnit;
}


void CvCity::conscript()
{
	if (!canConscript())
		return;

	int iPopChange = -(getConscriptPopulation());
	int iAngerLength = flatConscriptAngerLength();
	changePopulation(iPopChange);
	changeConscriptAngerTimer(iAngerLength);

	setDrafted(true);

	GET_PLAYER(getOwnerINLINE()).changeConscriptCount(1);

	CvUnit* pUnit = initConscriptedUnit();
	FAssertMsg(pUnit != NULL, "pUnit expected to be assigned (not NULL)");

	if (NULL != pUnit)
	{
		if (GC.getGameINLINE().getActivePlayer() == getOwnerINLINE()
				&& !CvPlot::isAllFog()) // advc.706
		{
			gDLL->getInterfaceIFace()->lookAt(plot()->getPoint(), CAMERALOOKAT_NORMAL); // K-Mod
			gDLL->getInterfaceIFace()->selectUnit(pUnit, true, false, true);
		}
		if (gCityLogLevel >= 2 && !isHuman()) // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
			logBBAI("      City %S does conscript of a %S at cost of %d pop, %d anger", getName().GetCString(), pUnit->getName().GetCString(), iPopChange, iAngerLength );
	}
}


int CvCity::getBonusHealth(BonusTypes eBonus) const
{
	int iHealth = GC.getBonusInfo(eBonus).getHealth();

	for (int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		iHealth += getNumActiveBuilding((BuildingTypes)iI) * GC.getBuildingInfo((BuildingTypes) iI).getBonusHealthChanges(eBonus);
	}

	return iHealth;
}


int CvCity::getBonusHappiness(BonusTypes eBonus) const
{
	int iHappiness = GC.getBonusInfo(eBonus).getHappiness();

	for (int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		iHappiness += getNumActiveBuilding((BuildingTypes)iI) * GC.getBuildingInfo((BuildingTypes) iI).getBonusHappinessChanges(eBonus);
	}

	return iHappiness;
}


int CvCity::getBonusPower(BonusTypes eBonus, bool bDirty) const
{
	int iCount = 0;

	for (int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		if (getNumActiveBuilding((BuildingTypes)iI) > 0)
		{
			if (GC.getBuildingInfo((BuildingTypes) iI).getPowerBonus() == eBonus)
			{
				if (GC.getBuildingInfo((BuildingTypes) iI).isDirtyPower() == bDirty)
				{
					iCount += getNumActiveBuilding((BuildingTypes)iI);
				}
			}
		}
	}

	return iCount;
}


int CvCity::getBonusYieldRateModifier(YieldTypes eIndex, BonusTypes eBonus) const
{
	int iModifier;
	int iI;

	iModifier = 0;

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		iModifier += getNumActiveBuilding((BuildingTypes)iI) * GC.getBuildingInfo((BuildingTypes) iI).getBonusYieldModifier(eBonus, eIndex);
	}

	return iModifier;
}


void CvCity::processBonus(BonusTypes eBonus, int iChange)
{
	int iI;
	int iValue = GC.getBonusInfo(eBonus).getHealth();
	int iGoodValue = std::max(0, iValue);
	int iBadValue = std::min(0, iValue);

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		iValue = GC.getBuildingInfo((BuildingTypes) iI).getBonusHealthChanges(eBonus) * getNumActiveBuilding((BuildingTypes)iI);

		if (iValue >= 0)
		{
			iGoodValue += iValue;
		}
		else
		{
			iBadValue += iValue;
		}
	} // <advc.001w>
	CvGame const& g = GC.getGameINLINE();
	if((iGoodValue != 0 || iBadValue != 0) && getOwnerINLINE() == g.getActivePlayer()) {
		int iHealth = healthRate();
		if((iHealth < 0) != (iHealth + (iGoodValue - iBadValue) * iChange < 0))
			gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
	} // </advc.001w>
	changeBonusGoodHealth(iGoodValue * iChange);
	changeBonusBadHealth(iBadValue * iChange);


	iValue = GC.getBonusInfo(eBonus).getHappiness();
	iGoodValue = std::max(0, iValue);
	iBadValue = std::min(0, iValue);

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		iValue = getNumActiveBuilding((BuildingTypes)iI) * GC.getBuildingInfo((BuildingTypes) iI).getBonusHappinessChanges(eBonus);

		if (iValue >= 0)
		{
			iGoodValue += iValue;
		}
		else
		{
			iBadValue += iValue;
		}
	} // <advc.001w>
	if((iGoodValue != 0 || iBadValue != 0) && getOwnerINLINE() == g.getActivePlayer()) {
		int iHappy = happyLevel() - unhappyLevel();
		if((iHappy < 0) != (iHappy + (iGoodValue - iBadValue) * iChange < 0))
			gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
	} // </advc.001w>
	changeBonusGoodHappiness(iGoodValue * iChange);
	changeBonusBadHappiness(iBadValue * iChange);

	changePowerCount((getBonusPower(eBonus, true) * iChange), true);
	changePowerCount((getBonusPower(eBonus, false) * iChange), false);

	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		changeBonusYieldRateModifier(((YieldTypes)iI), (getBonusYieldRateModifier(((YieldTypes)iI), eBonus) * iChange));
	}
}


void CvCity::processBuilding(BuildingTypes eBuilding, int iChange, bool bObsolete)
{
	int iI, iJ;
	// <advc.003>
	CvBuildingInfo const& b = GC.getBuildingInfo(eBuilding);
	CvGame& g = GC.getGameINLINE(); // </advc.003>
	if (!GET_TEAM(getTeam()).isObsoleteBuilding(eBuilding) || bObsolete)
	{
		if (iChange > 0)
		{
			CorporationTypes eCorporation = (CorporationTypes)b.getFoundsCorporation();
			if (NO_CORPORATION != eCorporation && !g.isCorporationFounded(eCorporation))
			{
				setHeadquarters(eCorporation);
			}
		}

		if (b.getNoBonus() != NO_BONUS)
		{
			changeNoBonusCount(((BonusTypes)(b.getNoBonus())), iChange);
		}

		if (b.getFreeBonus() != NO_BONUS)
		{
			changeFreeBonus(((BonusTypes)(b.getFreeBonus())), (g.getNumFreeBonuses(eBuilding) * iChange));
		}

		if (b.getFreePromotion() != NO_PROMOTION)
		{
			changeFreePromotionCount(((PromotionTypes)(b.getFreePromotion())), iChange);
		}// <advc.912d>
		if(g.isOption(GAMEOPTION_NO_SLAVERY) && GET_PLAYER(getOwnerINLINE()).
				isHuman() && b.getHurryAngerModifier() < 0)
			changePopRushCount(iChange); // </advc.912d>
		changeEspionageDefenseModifier(b.getEspionageDefenseModifier() * iChange);
		changeGreatPeopleRateModifier(b.getGreatPeopleRateModifier() * iChange);
		changeFreeExperience(b.getFreeExperience() * iChange);
		changeMaxFoodKeptPercent(b.getFoodKept() * iChange);
		changeMaxAirlift(b.getAirlift() * iChange);
		changeAirModifier(b.getAirModifier() * iChange);
		changeAirUnitCapacity(b.getAirUnitCapacity() * iChange);
		changeNukeModifier(b.getNukeModifier() * iChange);
		changeFreeSpecialist(b.getFreeSpecialist() * iChange);
		changeMaintenanceModifier(b.getMaintenanceModifier() * iChange);
		changeWarWearinessModifier(b.getWarWearinessModifier() * iChange);
		changeHurryAngerModifier(b.getHurryAngerModifier() * iChange);
		changeHealRate(b.getHealRateChange() * iChange);
		if (b.getHealth() > 0)
		{
			changeBuildingGoodHealth(b.getHealth() * iChange);
		}
		else
		{
			changeBuildingBadHealth(b.getHealth() * iChange);
		}
		if (b.getHappiness() > 0)
		{
			changeBuildingGoodHappiness(b.getHappiness() * iChange);
		}
		else
		{
			changeBuildingBadHappiness(b.getHappiness() * iChange);
		}
		if (b.getReligionType() != NO_RELIGION)
		{
			changeStateReligionHappiness(((ReligionTypes)(b.getReligionType())), (b.getStateReligionHappiness() * iChange));
		}
		changeMilitaryProductionModifier(b.getMilitaryProductionModifier() * iChange);
		changeSpaceProductionModifier(b.getSpaceProductionModifier() * iChange);
		changeExtraTradeRoutes(b.getTradeRoutes() * iChange);
		changeTradeRouteModifier(b.getTradeRouteModifier() * iChange);
		changeForeignTradeRouteModifier(b.getForeignTradeRouteModifier() * iChange);
		changePowerCount(((b.isPower()) ? iChange : 0), b.isDirtyPower());
		changeGovernmentCenterCount((b.isGovernmentCenter()) ? iChange : 0);
		changeNoUnhappinessCount((b.isNoUnhappiness()) ? iChange : 0);
		//changeNoUnhealthyPopulationCount((b.isNoUnhealthyPopulation()) ? iChange : 0);
		changeUnhealthyPopulationModifier(b.getUnhealthyPopulationModifier() * iChange); // K-Mod
		changeBuildingOnlyHealthyCount((b.isBuildingOnlyHealthy()) ? iChange : 0);

		for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			changeSeaPlotYield(((YieldTypes)iI), (b.getSeaPlotYieldChange(iI) * iChange));
			changeRiverPlotYield(((YieldTypes)iI), (b.getRiverPlotYieldChange(iI) * iChange));
			changeBaseYieldRate(((YieldTypes)iI), ((b.getYieldChange(iI) + getBuildingYieldChange((BuildingClassTypes)b.getBuildingClassType(), (YieldTypes)iI))* iChange));
			changeYieldRateModifier(((YieldTypes)iI), (b.getYieldModifier(iI) * iChange));
			changePowerYieldRateModifier(((YieldTypes)iI), (b.getPowerYieldModifier(iI) * iChange));
		}

		for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
		{
			changeCommerceRateModifier(((CommerceTypes)iI), (b.getCommerceModifier(iI) * iChange));
			changeCommerceHappinessPer(((CommerceTypes)iI), (b.getCommerceHappiness(iI) * iChange));
		}

		for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
		{
			changeReligionInfluence(((ReligionTypes)iI), (b.getReligionChange(iI) * iChange));
		}

		for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
		{
			changeMaxSpecialistCount(((SpecialistTypes)iI), b.getSpecialistCount(iI) * iChange);
			changeFreeSpecialistCount(((SpecialistTypes)iI), b.getFreeSpecialistCount(iI) * iChange);
		}

		for (iI = 0; iI < GC.getNumImprovementInfos(); ++iI)
		{
			changeImprovementFreeSpecialists((ImprovementTypes)iI, b.getImprovementFreeSpecialist(iI) * iChange);
		}

		FAssertMsg(0 < GC.getNumBonusInfos(), "GC.getNumBonusInfos() is negative but an array is being allocated in CvPlotGroup::reset");
		for (iI = 0; iI < GC.getNumBonusInfos(); iI++)
		{
			if (hasBonus((BonusTypes)iI))
			{
				if (b.getBonusHealthChanges(iI) > 0)
				{
					changeBonusGoodHealth(b.getBonusHealthChanges(iI) * iChange);
				}
				else
				{
					changeBonusBadHealth(b.getBonusHealthChanges(iI) * iChange);
				}
				if (b.getBonusHappinessChanges(iI) > 0)
				{
					changeBonusGoodHappiness(b.getBonusHappinessChanges(iI) * iChange);
				}
				else
				{
					changeBonusBadHappiness(b.getBonusHappinessChanges(iI) * iChange);
				}

				if (b.getPowerBonus() == iI)
				{
					changePowerCount(iChange, b.isDirtyPower());
				}

				for (iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
				{
					changeBonusYieldRateModifier(((YieldTypes)iJ), (b.getBonusYieldModifier(iI, iJ) * iChange));
				}
			}
		}

		for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
		{
			changeUnitCombatFreeExperience(((UnitCombatTypes)iI), b.getUnitCombatFreeExperience(iI) * iChange);
		}

		for (iI = 0; iI < NUM_DOMAIN_TYPES; iI++)
		{
			changeDomainFreeExperience(((DomainTypes)iI), b.getDomainFreeExperience(iI) * iChange);
			changeDomainProductionModifier(((DomainTypes)iI), b.getDomainProductionModifier(iI) * iChange);
		}

		updateExtraBuildingHappiness();
		updateExtraBuildingHealth();

		GET_PLAYER(getOwnerINLINE()).changeAssets(b.getAssetValue() * iChange);

		area()->changePower(getOwnerINLINE(), (b.getPowerValue() * iChange));
		GET_PLAYER(getOwnerINLINE()).changePower(b.getPowerValue() * iChange);

		for (iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getTeam())
			{
				if (b.isTeamShare() || (iI == getOwnerINLINE()))
				{
					GET_PLAYER((PlayerTypes)iI).processBuilding(eBuilding, iChange, area());
				}
			}
		}

		GET_TEAM(getTeam()).processBuilding(eBuilding, iChange);

		g.processBuilding(eBuilding, iChange);
	}

	if (!bObsolete)
	{
		changeBuildingDefense(b.getDefenseModifier() * iChange);
		changeBuildingBombardDefense(b.getBombardDefenseModifier() * iChange);
		// advc.051: Moved
		//changeBaseGreatPeopleRate(b.getGreatPeopleRateChange() * iChange);
		if (b.getGreatPeopleUnitClass() != NO_UNITCLASS)
		{
			UnitTypes eGreatPeopleUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(b.getGreatPeopleUnitClass())));

			if (eGreatPeopleUnit != NO_UNIT)
			{
				changeGreatPeopleUnitRate(eGreatPeopleUnit, b.getGreatPeopleRateChange() * iChange);
				/*  advc.051: Moved from above. Barbarians can obtain wonders, but
					don't have GP units, and shouldn't have a positive base GP rate. */
				changeBaseGreatPeopleRate(b.getGreatPeopleRateChange() * iChange);
			}
		}
		BuildingClassTypes eBuildingClass = (BuildingClassTypes)b.getBuildingClassType();
		GET_TEAM(getTeam()).changeBuildingClassCount(eBuildingClass, iChange);
		GET_PLAYER(getOwnerINLINE()).changeBuildingClassCount(eBuildingClass, iChange);

		GET_PLAYER(getOwnerINLINE()).changeWondersScore(getWonderScore((BuildingClassTypes)(b.getBuildingClassType())) * iChange);
		// <advc.004w>
		if(GC.getGameINLINE().getCurrentLayer() == GLOBE_LAYER_RESOURCE) {
			// Update text of resource indicators (CvGameTextMgr::setBonusExtraHelp)
			PlayerTypes eDirtyPlayer = NO_PLAYER;
			if(::isNationalWonderClass(eBuildingClass) &&
					getOwnerINLINE() == GC.getGameINLINE().getActivePlayer())
				eDirtyPlayer = getOwnerINLINE();
			else if(::isWorldWonderClass(eBuildingClass))
				eDirtyPlayer = GC.getGameINLINE().getActivePlayer();
			if(eDirtyPlayer != NO_PLAYER) {
				gDLL->getInterfaceIFace()->setDirty(GlobeLayer_DIRTY_BIT, true);
				// advc.003p:
				GET_PLAYER(getOwnerINLINE()).setBonusHelpDirty();
			}
		} // </advc.004w>
	}

	updateBuildingCommerce();

	setLayoutDirty(true);
}


void CvCity::processProcess(ProcessTypes eProcess, int iChange)
{
	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		changeProductionToCommerceModifier(((CommerceTypes)iI), (GC.getProcessInfo(eProcess).getProductionToCommerceModifier(iI) * iChange));
	}
}


void CvCity::processSpecialist(SpecialistTypes eSpecialist, int iChange)
{
	UnitTypes eGreatPeopleUnit;
	int iI;

	if (GC.getSpecialistInfo(eSpecialist).getGreatPeopleUnitClass() != NO_UNITCLASS)
	{
		eGreatPeopleUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(GC.getSpecialistInfo(eSpecialist).getGreatPeopleUnitClass())));

		if (eGreatPeopleUnit != NO_UNIT)
		{
			changeGreatPeopleUnitRate(eGreatPeopleUnit, GC.getSpecialistInfo(eSpecialist).getGreatPeopleRateChange() * iChange);
			/*  advc.051: Moved from below. Barbarians don't have GP units; if they
				still end up with a specialist, the base GP rate shouldn't count it. */
			changeBaseGreatPeopleRate(GC.getSpecialistInfo(eSpecialist).getGreatPeopleRateChange() * iChange);
		}
	}
	// advc.051: Moved into the block above
	//changeBaseGreatPeopleRate(GC.getSpecialistInfo(eSpecialist).getGreatPeopleRateChange() * iChange);

	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		changeBaseYieldRate(((YieldTypes)iI), (GC.getSpecialistInfo(eSpecialist).getYieldChange(iI) * iChange));
	}

	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		changeSpecialistCommerce(((CommerceTypes)iI), (GC.getSpecialistInfo(eSpecialist).getCommerceChange(iI) * iChange));
	}

	updateExtraSpecialistYield();

	changeSpecialistFreeExperience(GC.getSpecialistInfo(eSpecialist).getExperience() * iChange);
}


HandicapTypes CvCity::getHandicapType() const
{
	return GET_PLAYER(getOwnerINLINE()).getHandicapType();
}


CivilizationTypes CvCity::getCivilizationType() const
{
	return GET_PLAYER(getOwnerINLINE()).getCivilizationType();
}


LeaderHeadTypes CvCity::getPersonalityType() const
{
	return GET_PLAYER(getOwnerINLINE()).getPersonalityType();
}


ArtStyleTypes CvCity::getArtStyleType() const
{
	//return GET_PLAYER(getOwnerINLINE()).getArtStyleType();
	// <advc.005f> Replacing the above
	PlayerTypes eArtPlayer = getOwnerINLINE();
	if(GC.getENABLE_005F() > 0) {
		PlayerTypes eCultOwner = calculateCulturalOwner();
		if(eCultOwner != NO_PLAYER)
			eArtPlayer = eCultOwner;
		else FAssert(eCultOwner != NO_PLAYER);
	}
	return GET_PLAYER(eArtPlayer).getArtStyleType(); // </advc.005f>
}


CitySizeTypes CvCity::getCitySizeType() const
{
	return ((CitySizeTypes)(range((getPopulation() / 7), 0, (NUM_CITYSIZE_TYPES - 1))));
}

const CvArtInfoBuilding* CvCity::getBuildingArtInfo(BuildingTypes eBuilding) const
{
	return GC.getBuildingInfo(eBuilding).getArtInfo();
}

float CvCity::getBuildingVisibilityPriority(BuildingTypes eBuilding) const
{
	return GC.getBuildingInfo(eBuilding).getVisibilityPriority();
}

bool CvCity::hasTrait(TraitTypes eTrait) const
{
	return GET_PLAYER(getOwnerINLINE()).hasTrait(eTrait);
}


bool CvCity::isBarbarian() const
{
	return GET_PLAYER(getOwnerINLINE()).isBarbarian();
}


bool CvCity::isHuman() const
{
	return GET_PLAYER(getOwnerINLINE()).isHuman();
}


bool CvCity::isVisible(TeamTypes eTeam, bool bDebug) const
{
	return plot()->isVisible(eTeam, bDebug);
}


bool CvCity::isCapital() const
{
	return (GET_PLAYER(getOwnerINLINE()).getCapitalCity() == this);
}

// <advc.041>
bool CvCity::isPrereqBonusSea() const {

	for(int i = 0; i < NUM_DIRECTION_TYPES; i++) {
		CvPlot* p = plotDirection(getX_INLINE(), getY_INLINE(), (DirectionTypes)i);
		if(p != NULL && p->isWater() && p->area()->getNumTotalBonuses() > 0)
			return true;
	}
	return false;
}// </advc.041>

bool CvCity::isCoastal(int iMinWaterSize) const
{	// <advc.003>
	if(iMinWaterSize < 0)
		iMinWaterSize = GC.getMIN_WATER_SIZE_FOR_OCEAN(); // </advc.003>
	return plot()->isCoastalLand(iMinWaterSize);
}


bool CvCity::isDisorder() const
{
	return (isOccupation() || GET_PLAYER(getOwnerINLINE()).isAnarchy());
}


bool CvCity::isHolyCity(ReligionTypes eIndex) const
{
	return (GC.getGameINLINE().getHolyCity(eIndex) == this);
}


bool CvCity::isHolyCity() const
{
	for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		if (isHolyCity((ReligionTypes)iI))
		{
			return true;
		}
	}

	return false;
}


bool CvCity::isHeadquarters(CorporationTypes eIndex) const
{
	return (GC.getGameINLINE().getHeadquarters(eIndex) == this);
}

void CvCity::setHeadquarters(CorporationTypes eIndex)
{
	GC.getGameINLINE().setHeadquarters(eIndex, this,
			false); // advc.106e

	if (GC.getCorporationInfo(eIndex).getFreeUnitClass() != NO_UNITCLASS)
	{
		UnitTypes eFreeUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(GC.getCorporationInfo(eIndex).getFreeUnitClass())));

		if (eFreeUnit != NO_UNIT)
		{
			GET_PLAYER(getOwnerINLINE()).initUnit(eFreeUnit, getX_INLINE(), getY_INLINE());
		}
	}
}

bool CvCity::isHeadquarters() const
{
	for (int iI = 0; iI < GC.getNumCorporationInfos(); iI++)
	{
		if (isHeadquarters((CorporationTypes)iI))
		{
			return true;
		}
	}

	return false;
}


int CvCity::getOvercrowdingPercentAnger(int iExtra) const
{
	int iAnger = 0;
	int iOvercrowding = (getPopulation() + iExtra);
	if (iOvercrowding > 0)
	{
		iAnger += (((iOvercrowding * GC.getPERCENT_ANGER_DIVISOR()) / std::max(1, (getPopulation() + iExtra))) + 1);
	}
	return iAnger;
}


int CvCity::getNoMilitaryPercentAnger() const
{
	if(GC.getDefineINT("DEMAND_BETTER_PROTECTION") <= 0) // advc.500b
	{
		int iAnger = 0;
		if (getMilitaryHappinessUnits() == 0)
			iAnger += GC.getDefineINT("NO_MILITARY_PERCENT_ANGER");
		return iAnger; // <advc.500b>
	}
	double actualGarrStr = garrisonStrength();
	double targetGarrStr = getPopulation() / 2.0;
	if(actualGarrStr >= targetGarrStr)
		return 0;
	/* Currently (as per vanilla) 334, meaning 33.4% of the population get angry.
	   The caller adds up all the anger percentages (actually permillages)
	   before rounding, so rounding shouldn't be a concern in this function. */
	int iMaxAnger = GC.getDefineINT("NO_MILITARY_PERCENT_ANGER");
	return iMaxAnger - (int)(iMaxAnger * actualGarrStr / targetGarrStr);
	// </advc.500b>
}


int CvCity::getCulturePercentAnger() const
{
	int iTotalCulture = plot()->getTotalCulture(); // advc.003b: was countTotalCulture
	if (iTotalCulture == 0)
		return 0;

	int iAngryCulture = 0;
	// <advc.099>
	int const iAngerModCB = GC.getDefineINT("CLOSED_BORDERS_CULTURE_ANGER_MODIFIER");
	int const iAngerModWar = GC.getDefineINT("AT_WAR_CULTURE_ANGER_MODIFIER");
	for(int iI = 0; iI < MAX_PLAYERS; iI++) {
		CvPlayer const& kRival = GET_PLAYER((PlayerTypes)iI);
		if(!kRival.isEverAlive() || kRival.getTeam() == getTeam())
			continue;
		int iCulture = plot()->getCulture(kRival.getID());
		if(iCulture <= 0)
			continue;
		int iRelationsModifier = 0;
		if(!kRival.isBarbarian()) {
			if(kRival.isAlive() && ::atWar(kRival.getTeam(), getTeam()))
				iRelationsModifier += iAngerModWar;
			else {
				bool const bCapitulatedVassal = (GET_TEAM(kRival.getTeam()).isCapitulated() &&
						GET_TEAM(kRival.getTeam()).isVassal(getTeam()));
				bool const bOB = GET_TEAM(getTeam()).isOpenBorders(kRival.getTeam());
				if((!bOB && !bCapitulatedVassal) || !kRival.isAlive())
					iRelationsModifier += iAngerModCB;
			}
		}
		iCulture *= std::max(0, iRelationsModifier + 100);
		iCulture /= 100;
		iAngryCulture += iCulture;
	} // </advc.099>
	return (GC.getDefineINT("CULTURE_PERCENT_ANGER") * iAngryCulture) / iTotalCulture;
}

// <advc.104>
int CvCity::getReligionPercentAnger() const {

	double r = 0;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayer const& civ = GET_PLAYER((PlayerTypes)i);
		if(!civ.isAlive() || !atWar(civ.getTeam(), getTeam()))
			continue;
		r += getReligionPercentAnger(civ.getID());
	}
	return ::round(r);
}


double CvCity::getReligionPercentAnger(PlayerTypes ePlayer) const {

	// Replacing BtS code; fewer rounding artifacts.
	CvGame const& g = GC.getGameINLINE();
	if(g.getNumCities() == 0 || getReligionCount() == 0)
		return 0;
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	ReligionTypes eReligion = kPlayer.getStateReligion();
	if(eReligion == NO_RELIGION || !isHasReligion(eReligion))
		return 0;
	double sameFaithCityRatio = kPlayer.getHasReligionCount(eReligion) / (double)g.getNumCities();
	// normally 800
	double angerFactor = GC.getDefineINT("RELIGION_PERCENT_ANGER") /
			(double)getReligionCount();
	return sameFaithCityRatio * angerFactor;
} // </advc.104>

int CvCity::getHurryPercentAnger(int iExtra) const
{
	if (getHurryAngerTimer() == 0)
		return 0;

	return ((((((getHurryAngerTimer() - 1) /
			flatHurryAngerLength()) + 1) *
			GC.getDefineINT("HURRY_POP_ANGER") *
			GC.getPERCENT_ANGER_DIVISOR()) /
			std::max(1, getPopulation() + iExtra)) + 1);
}


int CvCity::getConscriptPercentAnger(int iExtra) const
{
	if (getConscriptAngerTimer() == 0)
		return 0;

	return ((((((getConscriptAngerTimer() - 1) /
			flatConscriptAngerLength()) + 1) *
			GC.getDefineINT("CONSCRIPT_POP_ANGER") *
			GC.getPERCENT_ANGER_DIVISOR()) /
			std::max(1, getPopulation() + iExtra)) + 1);
}

int CvCity::getDefyResolutionPercentAnger(int iExtra) const
{
	if (getDefyResolutionAngerTimer() == 0)
		return 0;

	return ((((((getDefyResolutionAngerTimer() - 1) /
			flatDefyResolutionAngerLength()) + 1) *
			GC.getDefineINT("DEFY_RESOLUTION_POP_ANGER") *
			GC.getPERCENT_ANGER_DIVISOR()) /
			std::max(1, getPopulation() + iExtra)) + 1);
}


int CvCity::getWarWearinessPercentAnger() const
{
	int iAnger = GET_PLAYER(getOwnerINLINE()).getWarWearinessPercentAnger();

	iAnger *= std::max(0, (getWarWearinessModifier() + GET_PLAYER(getOwnerINLINE()).getWarWearinessModifier() + 100));
	iAnger /= 100;

	return iAnger;
}


int CvCity::getLargestCityHappiness() const
{
	if (findPopulationRank() <= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).
			getTargetNumCities())
		return GET_PLAYER(getOwnerINLINE()).getLargestCityHappiness();
	return 0;
}

int CvCity::getVassalHappiness() const
{	// <advc.003n>
	if(isBarbarian())
		return 0; // </advc.003n>
	int iHappy = 0;
	for (int i = 0; i < MAX_CIV_TEAMS; i++) // advc.003n: was MAX_PLAYERS
	{
		if (getTeam() == i)
			continue;

		if (GET_TEAM((TeamTypes)i).isVassal(getTeam())
				&& !GET_TEAM((TeamTypes)i).isCapitulated()) // advc.142
		{
			iHappy += GC.getDefineINT("VASSAL_HAPPINESS");
			break; // advc.142
		}
	}
	return iHappy;
}

int CvCity::getVassalUnhappiness() const
{	// <advc.003b> Replacing the BtS code below
	if(GET_TEAM(getTeam()).isAVassal())
		return GC.getDefineINT("VASSAL_HAPPINESS");
	return 0; // </advc.003b>
	/*int iUnhappy = 0;
	for (int i = 0; i < MAX_TEAMS; i++) {
		if (getTeam() != i) {
			if (GET_TEAM(getTeam()).isVassal((TeamTypes)i))
				iUnhappy += GC.getDefineINT("VASSAL_HAPPINESS");
		}
	}
	return iUnhappy;*/
}


int CvCity::unhappyLevel(int iExtra) const
{
	int iUnhappiness = 0;

	if (!isNoUnhappiness())
	{
		int iAngerPercent = 0;

		iAngerPercent += getOvercrowdingPercentAnger(iExtra);
		iAngerPercent += getNoMilitaryPercentAnger();
		iAngerPercent += getCulturePercentAnger();
		iAngerPercent += getReligionPercentAnger();
		iAngerPercent += getHurryPercentAnger(iExtra);
		iAngerPercent += getConscriptPercentAnger(iExtra);
		iAngerPercent += getDefyResolutionPercentAnger(iExtra);
		iAngerPercent += getWarWearinessPercentAnger();
		/*  K-Mod, 5/jan/11, karadoc
			global warming anger _percent_; as part per 100.
			Unfortunately, people who made the rest of the game used anger percent to mean part per 1000
			so I have to multiply my GwPercentAnger by 10 to make it fit in. */
		iAngerPercent += std::max(0, GET_PLAYER(getOwnerINLINE()).getGwPercentAnger()*10);

		for (int iI = 0; iI < GC.getNumCivicInfos(); iI++)
		{
			iAngerPercent += GET_PLAYER(getOwnerINLINE()).getCivicPercentAnger((CivicTypes)iI);
		}

		iUnhappiness = ((iAngerPercent * (getPopulation() + iExtra)) / GC.getPERCENT_ANGER_DIVISOR());

		iUnhappiness -= std::min(0, getLargestCityHappiness());
		iUnhappiness -= std::min(0, getMilitaryHappiness());
		iUnhappiness -= std::min(0, getCurrentStateReligionHappiness());
		iUnhappiness -= std::min(0, getBuildingBadHappiness());
		iUnhappiness -= std::min(0, getExtraBuildingBadHappiness());
		iUnhappiness -= std::min(0, getFeatureBadHappiness());
		iUnhappiness -= std::min(0, getBonusBadHappiness());
		iUnhappiness -= std::min(0, getReligionBadHappiness());
		iUnhappiness -= std::min(0, getCommerceHappiness());
		iUnhappiness -= std::min(0, area()->getBuildingHappiness(getOwnerINLINE()));
		iUnhappiness -= std::min(0, GET_PLAYER(getOwnerINLINE()).getBuildingHappiness());
		iUnhappiness -= std::min(0, (getExtraHappiness() + GET_PLAYER(getOwnerINLINE()).getExtraHappiness()));
		iUnhappiness -= std::min(0, GC.getHandicapInfo(getHandicapType()).getHappyBonus());
		iUnhappiness += std::max(0, getVassalUnhappiness());
		iUnhappiness += std::max(0, getEspionageHappinessCounter());
	}

	return std::max(0, iUnhappiness);
}


int CvCity::happyLevel() const
{
	int iHappiness = 0;
	iHappiness += std::max(0, getLargestCityHappiness());
	iHappiness += std::max(0, getMilitaryHappiness());
	iHappiness += std::max(0, getCurrentStateReligionHappiness());
	iHappiness += std::max(0, getBuildingGoodHappiness());
	iHappiness += std::max(0, getExtraBuildingGoodHappiness());
	iHappiness += std::max(0, getFeatureGoodHappiness());
	iHappiness += std::max(0, getBonusGoodHappiness());
	iHappiness += std::max(0, getReligionGoodHappiness());
	iHappiness += std::max(0, getCommerceHappiness());
	iHappiness += std::max(0, area()->getBuildingHappiness(getOwnerINLINE()));
	iHappiness += std::max(0, GET_PLAYER(getOwnerINLINE()).getBuildingHappiness());
	iHappiness += std::max(0, (getExtraHappiness() + GET_PLAYER(getOwnerINLINE()).getExtraHappiness()));
	iHappiness += std::max(0, GC.getHandicapInfo(getHandicapType()).getHappyBonus());
	iHappiness += std::max(0, getVassalHappiness());

	if (getHappinessTimer() > 0)
	{
		iHappiness += GC.getDefineINT("TEMP_HAPPY");
	}


	return std::max(0, iHappiness);
}


int CvCity::angryPopulation(int iExtra,
		bool bIgnoreCultureRate) const { // advc.104

	return range((unhappyLevel(iExtra) - happyLevel()
			+ (bIgnoreCultureRate ?
			// advc.104: Actually, ignore tales of "villainy" too.
			getCommerceHappiness() - getEspionageHappinessCounter() : 0)),
			0, (getPopulation() + iExtra));
}


int CvCity::visiblePopulation() const
{
	return (getPopulation() - angryPopulation() - getWorkingPopulation());
}


int CvCity::totalFreeSpecialists() const
{
	int iCount = 0;
	if (getPopulation() > 0)
	{
		iCount += getFreeSpecialist();
		iCount += area()->getFreeSpecialist(getOwnerINLINE());
		iCount += GET_PLAYER(getOwnerINLINE()).getFreeSpecialist();

		for (int iImprovement = 0; iImprovement < GC.getNumImprovementInfos(); ++iImprovement)
		{
			int iNumSpecialistsPerImprovement = getImprovementFreeSpecialists((ImprovementTypes)iImprovement);
			if (iNumSpecialistsPerImprovement > 0)
			{
				iCount += iNumSpecialistsPerImprovement * countNumImprovedPlots((ImprovementTypes)iImprovement);
			}
		}
	}

	return iCount;
}


int CvCity::extraPopulation() const
{
	return (visiblePopulation() + std::min(0, extraFreeSpecialists()));
}


int CvCity::extraSpecialists() const
{
	return (visiblePopulation() + extraFreeSpecialists());
}


int CvCity::extraFreeSpecialists() const
{
	return (totalFreeSpecialists() - getSpecialistPopulation());
}


int CvCity::unhealthyPopulation(bool bNoAngry, int iExtra) const
{
	/*  K-Mod, 27/dec/10, karadoc
		replaced NoUnhealthyPopulation with UnhealthyPopulationModifier */
	/* original bts code
	if (isNoUnhealthyPopulation())
		return 0;
	return std::max(0, ((getPopulation() + iExtra - ((bNoAngry) ? angryPopulation(iExtra) : 0))));*/
	int iUnhealth = getPopulation() + iExtra - ((bNoAngry)? angryPopulation(iExtra) : 0);
	iUnhealth *= std::max(0, 100+getUnhealthyPopulationModifier());
	iUnhealth = ROUND_DIVIDE(iUnhealth, 100);
	return std::max(0, iUnhealth);
	// K-Mod end
}


int CvCity::totalGoodBuildingHealth() const
{
	return (getBuildingGoodHealth() + area()->getBuildingGoodHealth(getOwnerINLINE()) + GET_PLAYER(getOwnerINLINE()).getBuildingGoodHealth() + getExtraBuildingGoodHealth());
}


int CvCity::totalBadBuildingHealth() const
{
	if (!isBuildingOnlyHealthy())
	{
		return (getBuildingBadHealth() + area()->getBuildingBadHealth(getOwnerINLINE()) + GET_PLAYER(getOwnerINLINE()).getBuildingBadHealth() + getExtraBuildingBadHealth());
	}

	return 0;
}


int CvCity::goodHealth() const
{
	int iTotalHealth;
	int iHealth;

	iTotalHealth = 0;

	iHealth = getFreshWaterGoodHealth();
	if (iHealth > 0)
	{
		iTotalHealth += iHealth;
	}

	iHealth = getFeatureGoodHealth();
	if (iHealth > 0)
	{
		iTotalHealth += iHealth;
	}

	iHealth = getPowerGoodHealth();
	if (iHealth > 0)
	{
		iTotalHealth += iHealth;
	}

	iHealth = getBonusGoodHealth();
	if (iHealth > 0)
	{
		iTotalHealth += iHealth;
	}

	iHealth = totalGoodBuildingHealth();
	if (iHealth > 0)
	{
		iTotalHealth += iHealth;
	}

	iHealth = GET_PLAYER(getOwnerINLINE()).getExtraHealth() + getExtraHealth();
	if (iHealth > 0)
	{
		iTotalHealth += iHealth;
	}

	iHealth = GC.getHandicapInfo(getHandicapType()).getHealthBonus();
	if (iHealth > 0)
	{
		iTotalHealth += iHealth;
	}

	return iTotalHealth;
}


int CvCity::badHealth(bool bNoAngry, int iExtra) const
{
	int iTotalHealth;
	int iHealth;

	iTotalHealth = 0;

	iHealth = getEspionageHealthCounter();
	if (iHealth > 0)
	{
		iTotalHealth -= iHealth;
	}

	iHealth = getFreshWaterBadHealth();
	if (iHealth < 0)
	{
		iTotalHealth += iHealth;
	}

	iHealth = getFeatureBadHealth();
	if (iHealth < 0)
	{
		iTotalHealth += iHealth;
	}

	iHealth = getPowerBadHealth();
	if (iHealth < 0)
	{
		iTotalHealth += iHealth;
	}

	iHealth = getBonusBadHealth();
	if (iHealth < 0)
	{
		iTotalHealth += iHealth;
	}

	iHealth = totalBadBuildingHealth();
	if (iHealth < 0)
	{
		iTotalHealth += iHealth;
	}

	iHealth = GET_PLAYER(getOwnerINLINE()).getExtraHealth() + getExtraHealth();
	if (iHealth < 0)
	{
		iTotalHealth += iHealth;
	}

	iHealth = GC.getHandicapInfo(getHandicapType()).getHealthBonus();
	if (iHealth < 0)
	{
		iTotalHealth += iHealth;
	}

	iHealth = getExtraBuildingBadHealth();
	if (iHealth < 0)
	{
		iTotalHealth += iHealth;
	}

	return (unhealthyPopulation(bNoAngry, iExtra) - iTotalHealth);
}


int CvCity::healthRate(bool bNoAngry, int iExtra) const
{
	return std::min(0, (goodHealth() - badHealth(bNoAngry, iExtra)));
}


int CvCity::foodConsumption(bool bNoAngry, int iExtra) const
{
	return ((((getPopulation() + iExtra) - ((bNoAngry) ? angryPopulation(iExtra) : 0)) * GC.getFOOD_CONSUMPTION_PER_POPULATION()) - healthRate(bNoAngry, iExtra));
}


int CvCity::foodDifference(bool bBottom, bool bIgnoreProduction) const
{
	int iDifference;

	if (isDisorder())
	{
		return 0;
	}

	//if (isFoodProduction())
	if (!bIgnoreProduction && isFoodProduction()) // K-Mod
	{
		iDifference = std::min(0, (getYieldRate(YIELD_FOOD) - foodConsumption()));
	}
	else
	{
		iDifference = (getYieldRate(YIELD_FOOD) - foodConsumption());
	}

	if (bBottom)
	{
		if ((getPopulation() == 1) && (getFood() == 0))
		{
			iDifference = std::max(0, iDifference);
		}
	}

	return iDifference;
}


int CvCity::growthThreshold(/* advc.064b: */ int iPopulationChange) const
{
	return (GET_PLAYER(getOwnerINLINE()).getGrowthThreshold(getPopulation()
			+ iPopulationChange)); // advc.064b
}


int CvCity::productionLeft() const
{
	return (getProductionNeeded() - getProduction());
}

int CvCity::getHurryCostModifier(bool bIgnoreNew) const
{
	int iModifier = 100;
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		switch (pOrderNode->m_data.eOrderType)
		{
		case ORDER_TRAIN:
			iModifier = getHurryCostModifier((UnitTypes) pOrderNode->m_data.iData1, bIgnoreNew);
			break;

		case ORDER_CONSTRUCT:
			iModifier = getHurryCostModifier((BuildingTypes) pOrderNode->m_data.iData1, bIgnoreNew);
			break;

		case ORDER_CREATE:
		case ORDER_MAINTAIN:
			break;

		default:
			FAssertMsg(false, "pOrderNode->m_data.eOrderType did not match a valid option");
			break;
		}
	}

	return iModifier;
}

int CvCity::getHurryCostModifier(UnitTypes eUnit, bool bIgnoreNew) const
{
	return getHurryCostModifier(GC.getUnitInfo(eUnit).getHurryCostModifier(), getUnitProduction(eUnit), bIgnoreNew);
}

int CvCity::getHurryCostModifier(BuildingTypes eBuilding, bool bIgnoreNew) const
{
	return getHurryCostModifier(GC.getBuildingInfo(eBuilding).getHurryCostModifier(), getBuildingProduction(eBuilding), bIgnoreNew);
}

int CvCity::getHurryCostModifier(int iBaseModifier, int iProduction, bool bIgnoreNew) const
{
	int iModifier = 100;
	iModifier *= std::max(0, iBaseModifier + 100);
	iModifier /= 100;

	if (iProduction == 0 && !bIgnoreNew)
	{	// advc.003b: NEW_HURRY_MODIFIER cached
		iModifier *= std::max(0, GC.getNEW_HURRY_MODIFIER() + 100);
		iModifier /= 100;
	}

	iModifier *= std::max(0, (GET_PLAYER(getOwnerINLINE()).getHurryModifier() + 100));
	iModifier /= 100;

	return iModifier;
}


int CvCity::hurryCost(bool bExtra) const
{
	return (getHurryCost(bExtra, productionLeft(), getHurryCostModifier(), getProductionModifier()));
}

int CvCity::getHurryCost(bool bExtra, UnitTypes eUnit, bool bIgnoreNew) const
{
	int iProductionLeft = getProductionNeeded(eUnit) - getUnitProduction(eUnit);

	return getHurryCost(bExtra, iProductionLeft, getHurryCostModifier(eUnit, bIgnoreNew), getProductionModifier(eUnit));
}

int CvCity::getHurryCost(bool bExtra, BuildingTypes eBuilding, bool bIgnoreNew) const
{
	int iProductionLeft = getProductionNeeded(eBuilding) - getBuildingProduction(eBuilding);

	return getHurryCost(bExtra, iProductionLeft, getHurryCostModifier(eBuilding, bIgnoreNew), getProductionModifier(eBuilding));
}

int CvCity::getHurryCost(bool bExtra, int iProductionLeft, int iHurryModifier, int iModifier) const
{	// <advc.064b>
	iProductionLeft -= getCurrentProductionDifference(bExtra, true, false, bExtra, true);
	if(bExtra) // City yield rate uncertain if pop is sacrificed (bExtra) ...
		iProductionLeft--; // ... but city production is going to be at least 1
	if(iProductionLeft <= 0)
		return 0; // </advc.064b>
	int iProduction = (iProductionLeft * iHurryModifier + 99) / 100; // round up

	if (bExtra)
	{
		int iExtraProduction = getExtraProductionDifference(iProduction,
			/*  advc.064c (comment): Passing 0 here instead of iModifier would
				only apply generic modifiers */
				iModifier);
		if (iExtraProduction > 0)
		{
			int iAdjustedProd = iProduction * iProduction;
			// round up
			iProduction = (iAdjustedProd + (iExtraProduction - 1)) / iExtraProduction;
		}
	}

	return std::max(0, iProduction);
}

int CvCity::hurryGold(HurryTypes eHurry) const
{
	return getHurryGold(eHurry, hurryCost(false));
}

int CvCity::getHurryGold(HurryTypes eHurry, int iHurryCost) const
{
	if (GC.getHurryInfo(eHurry).getGoldPerProduction() == 0)
	{
		return 0;
	}

	int iGold = (iHurryCost * GC.getHurryInfo(eHurry).getGoldPerProduction());

	return std::max(0, iGold); // advc.064b: lower bound was 1
}


int CvCity::hurryPopulation(HurryTypes eHurry) const
{
	return getHurryPopulation(eHurry, hurryCost(true));
}

int CvCity::getHurryPopulation(HurryTypes eHurry, int iHurryCost) const
{
	if (GC.getHurryInfo(eHurry).getProductionPerPopulation() == 0)
	{
		return 0;
	}

	int iPopulation = (iHurryCost - 1) / GC.getGameINLINE().getProductionPerPopulation(eHurry);

	return std::max(1, (iPopulation + 1));
}

int CvCity::hurryProduction(HurryTypes eHurry) const
{
	bool bPopRush = (GC.getHurryInfo(eHurry).getProductionPerPopulation() > 0);
	//int iProductionNeeded = productionLeft();
	// <advc.064b> ^Don't always need to generate that much production
	/*  If pop is to be removed, we can't rely on tile yields. (However, in that case,
		iProductionNeeded is only used for the assertion at the end. There's similar
		code for pop rushing in CvCity::getHurryCost.) */
	int iProductionDifference = getCurrentProductionDifference(bPopRush, true, false,
			bPopRush, true);
	if(bPopRush)
		iProductionDifference += GC.getYieldInfo(YIELD_PRODUCTION).getMinCity();
	int iProductionNeeded = std::max(0, getProductionNeeded() - getProduction() -
			iProductionDifference);
	// </advc.064b>
	if (!bPopRush)
		return iProductionNeeded;

	int iProduction = (100 * getExtraProductionDifference(hurryPopulation(eHurry) *
			GC.getGameINLINE().getProductionPerPopulation(eHurry)
			/*  advc.064c (comment): Passing 0 as a second arg would mean that only
				generic modifiers apply */
			)) / std::max(1, getHurryCostModifier());
	FAssert(iProduction >= iProductionNeeded);
	return iProduction;
}


int CvCity::flatHurryAngerLength() const
{
	int iAnger = GC.getDefineINT("HURRY_ANGER_DIVISOR");
	iAnger *= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getHurryConscriptAngerPercent();
	iAnger /= 100;
	iAnger *= std::max(0, 100 + getHurryAngerModifier());
	iAnger /= 100;

	return std::max(1, iAnger);
}


int CvCity::hurryAngerLength(HurryTypes eHurry) const
{
	if (GC.getHurryInfo(eHurry).isAnger())
		return flatHurryAngerLength();
	else return 0;
}


int CvCity::maxHurryPopulation() const
{
	return std::min(3, // advc.064c: Allow at most 3 pop to be sacrificed at once
			getPopulation() / 2);
}


int CvCity::cultureDistance(int iDX, int iDY) const
{
	return std::max(1, plotDistance(0, 0, iDX, iDY));
}


int CvCity::cultureStrength(PlayerTypes ePlayer) const
{
	//int iStrength = 1 + getHighestPopulation() * 2;
	// <advc.101> Replacing the above
	double pop = getPopulation();
	CvGame const& g = GC.getGameINLINE();
	/*  Would make more sense to use owner's era (if ePlayer is dead) b/c the
		insurgents would mostly use the owner's military tech. But don't want
		human owner to have to pay attention to his/her tech era. */
	int iEra = g.getCurrentEra();
	if(GET_PLAYER(ePlayer).isAlive())
		iEra = GET_PLAYER(ePlayer).getCurrentEra();
	double eraFactor = 1 + std::pow((double)iEra, 1.3);
	// To put a cap on the initial revolt chance in large cities:
	pop = std::min(pop, 1.5 * eraFactor);
	int iTimeOwned = g.getGameTurn() - getGameTurnAcquired();
	double div = 0.75 * GC.getGameSpeedInfo(g.getGameSpeedType()).
			getGoldenAgePercent();
	double timeRatio = std::min(1.0, iTimeOwned / div);
	// Gradually shift to highest pop
	pop += timeRatio * (getHighestPopulation() - pop);
	bool bCanFlip = canCultureFlip(ePlayer, false) &&
			ePlayer == plot()->calculateCulturalOwner();
	double strength = 1 + 2 * pop;
	double strengthFromInnerRadius = 0;
	CvPlayer const& kOwner = GET_PLAYER(getOwnerINLINE());
	// </advc.101>
	// <advc.099c>
	if(ePlayer == BARBARIAN_PLAYER)
		eraFactor /= 2; // </advc.099c>
	for(int i = 0; i < NUM_DIRECTION_TYPES; i++) {
		CvPlot* pLoopPlot = plotDirection(getX_INLINE(), getY_INLINE(),
				((DirectionTypes)i));
		if(pLoopPlot == NULL)
			continue;
		// <advc.035>
		PlayerTypes eLoopOwner = pLoopPlot->getOwnerINLINE();
		if(GC.getOWN_EXCLUSIVE_RADIUS() && eLoopOwner != NO_PLAYER &&
				!TEAMREF(eLoopOwner).isAtWar(kOwner.getTeam())) {
			PlayerTypes const eSecondOwner = pLoopPlot->getSecondOwner();
			if(eSecondOwner != eLoopOwner) // Checked only for easier debugging
				eLoopOwner = eSecondOwner;
		} // </advc.035>
		if(bCanFlip && // advc.101
				eLoopOwner == ePlayer) { // advc.035
			// advc.101:
			strengthFromInnerRadius += ::dRange(
					iTimeOwned / div - 2/3.0, 0.0, 3.5) * eraFactor;
		}
		// <advc.101>
		double cap = 0.25 + timeRatio * 0.75;
		strengthFromInnerRadius += eraFactor *
				::dRange((pLoopPlot->calculateCulturePercent(ePlayer) -
				pLoopPlot->calculateCulturePercent(getOwnerINLINE())) / 100.0,
				0.0, cap);
	}
	strength += std::min(strength, strengthFromInnerRadius);
	/*  HurryAnger also factors into grievanceModifier below, but for small cities
		(where Slavery is most effective), this constant bonus matters more. */
	if(getHurryAngerTimer() > 0)
		strength += 10; // </advc.101>
	/*  K-Mod, 7/jan/11, karadoc
		changed so that culture strength asymptotes as the attacking culture approaches 100% */
	//iStrength *= std::max(0, (GC.getDefineINT("REVOLT_TOTAL_CULTURE_MODIFIER") * (plot()->getCulture(ePlayer) - plot()->getCulture(getOwnerINLINE()))) / (plot()->getCulture(getOwnerINLINE()) + 1) + 100); //  K-Mod end
	// <advc.101> Restored BtS formula; now using floating point operations
	strength *= std::max(0.0, 1 +
			/*  Don't like the multiplicative interaction between this and the
				grievances; now added it to the grievances. */
			//(GC.getDefineINT("REVOLT_TOTAL_CULTURE_MODIFIER") *
			(plot()->getCulture(ePlayer) - plot()->getCulture(getOwnerINLINE())) /
			(double)plot()->getCulture(ePlayer));
	// New: Reduce strength if far less culture than some third party
	double thirdPartyModifier = (8.0 *
			plot()->calculateTeamCulturePercent(TEAMID(ePlayer))) /
			(5.0 * plot()->calculateTeamCulturePercent(
			plot()->findHighestCultureTeam()));
	if(thirdPartyModifier < 1.0)
		strength *= thirdPartyModifier;
	// Also/ further reduce strength if owner has almost as much culture as ePlayer
	double secondPartyModifier = ::dRange(plot()->getCulture(ePlayer) /
			(double)plot()->getCulture(getOwnerINLINE()) - 1, 0.0, 1.0);
	strength *= secondPartyModifier;
	// </advc.101>
	/*  <advc.099c> Religion offense might make sense even when a civ is dead,
		but can't expect the player to memorize the state religions of dead civs.
		Instead, count religion offense also when the owner's state religion
		is absent and at least one religion is in the city. This makes (some)
		sense whether the revolt civ is dead or not. */
	ReligionTypes eOwnerStateReligion = kOwner.getStateReligion();
	bool bReligionSuppressed = false;
	if(eOwnerStateReligion != NO_RELIGION && !isHasReligion(eOwnerStateReligion)) {
		for(int i = 0; i < GC.getNumReligionInfos(); i++) {
			ReligionTypes eLoopReligion = (ReligionTypes)i;
			if(isHasReligion(eLoopReligion) && eLoopReligion != eOwnerStateReligion) {
				bReligionSuppressed = true;
				break;
			}
		}
	}
	// advc.101: Apply them all at once
	double grievanceModifier = 0;
	/*  100 in BtS XML; I've increased it to 200 to keep pace with K-Mod's
		changes to culture spread */
	grievanceModifier += -1 + (GC.getDefineINT("REVOLT_TOTAL_CULTURE_MODIFIER") / 100.0);
	CvPlayer const& kRevoltPlayer = GET_PLAYER(ePlayer);
	if(bReligionSuppressed || (kRevoltPlayer.isAlive() &&
			(!GET_TEAM(kRevoltPlayer.getTeam()).isCapitulated() ||
			kRevoltPlayer.getMasterTeam() != getTeam()) &&
			kRevoltPlayer.getStateReligion() != NO_RELIGION && // No functional change
			isHasReligion(kRevoltPlayer.getStateReligion()))) {
		// </advc.099c>
		// advc.101: Replacing the code below
		grievanceModifier += GC.getDefineINT("REVOLT_OFFENSE_STATE_RELIGION_MODIFIER") / 100.0;
		//iStrength *= std::max(0, (GC.getDefineINT("REVOLT_OFFENSE_STATE_RELIGION_MODIFIER") + 100));
		//iStrength /= 100;
	}
	if(eOwnerStateReligion != NO_RELIGION && isHasReligion(eOwnerStateReligion)) {
		/*  <advc.099c> Replacing the code below. The OFFENSE modifier
			was originally set to 100 in XML and DEFENSE to -50, and they were
			cancelling out when both applied (multiplication by 100+100 and then
			by 100-50). I'm changing the values in XML so that they cancel out
			when added up. */
		grievanceModifier += GC.getDefineINT("REVOLT_DEFENSE_STATE_RELIGION_MODIFIER") / 100.0;
		//iStrength *= std::max(0, (GC.getDefineINT("REVOLT_DEFENSE_STATE_RELIGION_MODIFIER") + 100));
		//iStrength /= 100;
	} /* No state religion is still better than some oppressive state religion that
		 the city doesn't share. */
	if(eOwnerStateReligion == NO_RELIGION) {
		grievanceModifier += GC.getDefineINT("REVOLT_DEFENSE_STATE_RELIGION_MODIFIER")
				/ 100.0;
	} // <advc.099c>
	if(getHurryAngerTimer() > 0)
		grievanceModifier += 0.5;
	// <advc.099c>
	if(GET_TEAM(getTeam()).isCapitulated() && bCanFlip)
		grievanceModifier += 1; // </advc.099c>
	// The defense modifier should only halve culture strength, not reduce by 100%
	if(grievanceModifier < 0)
		grievanceModifier /= 2;
	strength *= (1 + std::max(-1.0, grievanceModifier));
	return ::round(strength);
	// </advc.101>
}


int CvCity::cultureGarrison(PlayerTypes ePlayer) const
{
	/*  <advc.101> Barbarian garrisons are not supposed to prevent revolts.
		BtS enforces this in CvPlot::doCulture (now renamed to CvPlot::doRevolts).
		Easier to do it here. */
	if(isBarbarian())
		return 0; // </advc.101>

	int iGarrison = 1;
	CLLNode<IDInfo>* pUnitNode = plot()->headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = plot()->nextUnitNode(pUnitNode);

		int iGarrisonLoop = pLoopUnit->getUnitInfo().getCultureGarrisonValue();
		/*  advc.101: Culture garrison values increase a bit too slowly over the
			course of the game. Easier to adjust that here than through XML.
			Note that this exponentiation is partly canceled out by exponentiation
			of the current game era in CvCity::cultureStrength. */
		iGarrisonLoop = ::round(std::pow(iGarrisonLoop * 2 / 3.0, 1.4));
		// <advc.023>
		iGarrisonLoop *= pLoopUnit->maxHitPoints() - pLoopUnit->getDamage();
		iGarrisonLoop /= 100;
		iGarrison += iGarrisonLoop;
	}
	// Commented out: </advc.023>
	/*if (atWar(GET_PLAYER(ePlayer).getTeam(), getTeam()))
		iGarrison *= 2; */
	// advc.101: Exponentiate again to give strength in numbers a superlinear effect
	iGarrison = ::round(std::pow((double)iGarrison, 1.2));
	return iGarrison;
}

// <advc.099c>
PlayerTypes CvCity::calculateCulturalOwner() const {

	return plot()->calculateCulturalOwner(GC.getREVOLTS_IGNORE_CULTURE_RANGE() > 0);
} // </advc.099c>


int CvCity::getNumBuilding(BuildingTypes eIndex) const
{
	FAssertMsg(eIndex != NO_BUILDING, "BuildingType eIndex is expected to not be NO_BUILDING");

	return std::min(GC.getCITY_MAX_NUM_BUILDINGS(), getNumRealBuilding(eIndex) + getNumFreeBuilding(eIndex));
}


int CvCity::getNumActiveBuilding(BuildingTypes eIndex) const
{
	FAssertMsg(eIndex != NO_BUILDING, "BuildingType eIndex is expected to not be NO_BUILDING");

	if (GET_TEAM(getTeam()).isObsoleteBuilding(eIndex))
	{
		return 0;
	}

	return (getNumBuilding(eIndex));
}


bool CvCity::hasActiveWorldWonder() const
{
	for (int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		if (isWorldWonderClass((BuildingClassTypes)(GC.getBuildingInfo((BuildingTypes)iI).getBuildingClassType())))
		{
			if (getNumRealBuilding((BuildingTypes)iI) > 0 && !(GET_TEAM(getTeam()).isObsoleteBuilding((BuildingTypes)iI)))
			{
				return true;
			}
		}
	}

	return false;
}

// UNOFFICIAL_PATCH, Bugfix from Mongoose SDK, 03/04/10, Mongoose & jdog5000: START
int CvCity::getNumActiveWorldWonders(/* advc.104d: */ PlayerTypes eOwner) const
{
	int iCount = 0;
	// advc.104d:
	TeamTypes eObsTeam = (eOwner == NO_PLAYER ? getTeam() : TEAMID(eOwner));

	for (int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		BuildingTypes eLoopBuilding = (BuildingTypes)iI;
		if (isWorldWonderClass((BuildingClassTypes)GC.getBuildingInfo(eLoopBuilding).
				getBuildingClassType()))
		{
			if (getNumRealBuilding(eLoopBuilding) > 0 &&
					!GET_TEAM(eObsTeam) // advc.104d
					.isObsoleteBuilding(eLoopBuilding))
				iCount++;
		}
	}
	return iCount;
} // UNOFFICIAL_PATCH: END


int CvCity::getReligionCount() const
{
	int iCount = 0;
	for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		if (isHasReligion((ReligionTypes)iI))
			iCount++;
	}
	return iCount;
}

int CvCity::getCorporationCount() const
{
	int iCount = 0;
	for (int iI = 0; iI < GC.getNumCorporationInfos(); iI++)
	{
		if (isHasCorporation((CorporationTypes)iI))
			iCount++;
	}
	return iCount;
}


int CvCity::getID() const
{
	return m_iID;
}


int CvCity::getIndex() const
{
	return (getID() & FLTA_INDEX_MASK);
}


IDInfo CvCity::getIDInfo() const
{
	IDInfo city(getOwnerINLINE(), getID());
	return city;
}


void CvCity::setID(int iID)
{
	m_iID = iID;
}

// <advc.104> getID is unique for a given player. plotNum is a globally unique id.
int CvCity::plotNum() const {

	return GC.getMapINLINE().plotNumINLINE(m_iX, m_iY);
} // </advc.104>

// <advc.003f>
int CvCity::getXExternal() const
{
	return getX_INLINE();
}


int CvCity::getYExternal() const
{
	return getY_INLINE();
} // </advc.003f>


bool CvCity::at(int iX,  int iY) const
{
	return ((getX_INLINE() == iX) && (getY_INLINE() == iY));
}


bool CvCity::at(CvPlot* pPlot) const
{
	return (plot() == pPlot);
}


CvPlot* CvCity::plot() const
{
	return GC.getMapINLINE().plotSorenINLINE(getX_INLINE(), getY_INLINE());
}


CvPlotGroup* CvCity::plotGroup(PlayerTypes ePlayer) const
{
	return plot()->getPlotGroup(ePlayer);
}


bool CvCity::isConnectedTo(CvCity* pCity) const
{
	return plot()->isConnectedTo(pCity);
}


bool CvCity::isConnectedToCapital(PlayerTypes ePlayer) const
{
	return plot()->isConnectedToCapital(ePlayer);
}


int CvCity::getArea() const
{
	return plot()->getArea();
}


CvArea* CvCity::area() const
{
	return plot()->area();
}

// BETTER_BTS_AI_MOD, General AI, 01/02/09, jdog5000: param added
CvArea* CvCity::waterArea(bool bNoImpassable) const
{
	return plot()->waterArea(bNoImpassable);
}

// BETTER_BTS_AI_MOD, General AI, 01/02/09, jdog5000: Expose plot function through city
CvArea* CvCity::secondWaterArea() const
{
	return plot()->secondWaterArea();
}

// advc.003j (comment): Unused; may or may not work correctly.
// BETTER_BTS_AI_MOD, General AI, 01/02/09, jdog5000: START
// Find the largest water area shared by this city and other city, if any
CvArea* CvCity::sharedWaterArea(CvCity* pOtherCity) const
{
	CvArea* pWaterArea = waterArea(true);
	if( pWaterArea != NULL )
	{
		CvArea* pOtherWaterArea = pOtherCity->waterArea(true);
		if( pOtherWaterArea != NULL )
		{
			if( pWaterArea == pOtherWaterArea )
			{
				return pWaterArea;
			}
			else
			{
				CvArea* pSecondWaterArea = secondWaterArea();
				CvArea* pOtherSecondWaterArea = pOtherCity->secondWaterArea();

				if( pSecondWaterArea != NULL && pSecondWaterArea == pOtherWaterArea )
				{
					return pSecondWaterArea;
				}
				else if( pOtherSecondWaterArea != NULL && pWaterArea == pOtherSecondWaterArea )
				{
					return pWaterArea;
				}
				else if( pSecondWaterArea != NULL && pOtherSecondWaterArea != NULL && pSecondWaterArea == pOtherSecondWaterArea )
				{
					return pSecondWaterArea;
				}
			}
		}
	}

	return NULL;
}


bool CvCity::isBlockaded() const
{
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		CvPlot* pAdjacentPlot = plotDirection(getX_INLINE(), getY_INLINE(), ((DirectionTypes)iI));

		if (pAdjacentPlot != NULL)
		{
			if( pAdjacentPlot->getBlockadedCount(getTeam()) > 0 )
			{
				return true;
			}
		}
	}

	return false;
} // BETTER_BTS_AI_MOD: END


CvPlot* CvCity::getRallyPlot() const
{
	return GC.getMapINLINE().plotSorenINLINE(m_iRallyX, m_iRallyY);
}


void CvCity::setRallyPlot(CvPlot* pPlot)
{
	if (getRallyPlot() != pPlot)
	{
		if (pPlot != NULL)
		{
			m_iRallyX = pPlot->getX_INLINE();
			m_iRallyY = pPlot->getY_INLINE();
		}
		else
		{
			m_iRallyX = INVALID_PLOT_COORD;
			m_iRallyY = INVALID_PLOT_COORD;
		}

		if (isCitySelected())
		{
			gDLL->getInterfaceIFace()->setDirty(ColoredPlots_DIRTY_BIT, true);
		}
	}
}


int CvCity::getGameTurnFounded() const
{
	return m_iGameTurnFounded;
}


void CvCity::setGameTurnFounded(int iNewValue)
{
	if (getGameTurnFounded() != iNewValue)
	{
		m_iGameTurnFounded = iNewValue;
		FAssert(getGameTurnFounded() >= 0);

		GC.getMapINLINE().updateWorkingCity();
	}
}


int CvCity::getGameTurnAcquired() const
{
	return m_iGameTurnAcquired;
}


void CvCity::setGameTurnAcquired(int iNewValue)
{
	m_iGameTurnAcquired = iNewValue;
	FAssert(getGameTurnAcquired() >= 0);
}


int CvCity::getPopulation() const
{
	return m_iPopulation;
}


void CvCity::setPopulation(int iNewValue)
{
	int iOldPopulation = getPopulation();

	if (iOldPopulation != iNewValue)
	{
		m_iPopulation = iNewValue;

		FAssert(getPopulation() >= 0);

		GET_PLAYER(getOwnerINLINE()).invalidatePopulationRankCache();

		if (getPopulation() > getHighestPopulation())
		{
			setHighestPopulation(getPopulation());
		}

		area()->changePopulationPerPlayer(getOwnerINLINE(), (getPopulation() - iOldPopulation));
		GET_PLAYER(getOwnerINLINE()).changeTotalPopulation(getPopulation() - iOldPopulation);
		GET_TEAM(getTeam()).changeTotalPopulation(getPopulation() - iOldPopulation);
		GC.getGameINLINE().changeTotalPopulation(getPopulation() - iOldPopulation);

		if (iOldPopulation > 0)
		{
			area()->changePower(getOwnerINLINE(), -(getPopulationPower(iOldPopulation)));
		}
		if (getPopulation() > 0)
		{
			area()->changePower(getOwnerINLINE(), getPopulationPower(getPopulation()));
		}

		plot()->updateYield();

		updateMaintenance();

		if (((iOldPopulation == 1) && (getPopulation() > 1)) ||
			  ((getPopulation() == 1) && (iOldPopulation > 1))
			  || ((getPopulation() > iOldPopulation) && (GET_PLAYER(getOwnerINLINE()).getNumCities() <= 2)))
		{
			if (!isHuman())
			{
				AI_setChooseProductionDirty(true);
			}
		}

		GET_PLAYER(getOwnerINLINE()).AI_makeAssignWorkDirty();

		setInfoDirty(true);
		setLayoutDirty(true);

		plot()->plotAction(PUF_makeInfoBarDirty);

		if ((getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) && isCitySelected())
		{
			gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(CityScreen_DIRTY_BIT, true);
		}

		//updateGenericBuildings();
	}
}


void CvCity::changePopulation(int iChange)
{
	setPopulation(getPopulation() + iChange);
}


long CvCity::getRealPopulation() const
{
	return (((long)(pow((float)getPopulation(), 2.8f))) * 1000);
}

int CvCity::getHighestPopulation() const
{
	return m_iHighestPopulation;
}


void CvCity::setHighestPopulation(int iNewValue)
{
 	m_iHighestPopulation = iNewValue;
	FAssert(getHighestPopulation() >= 0);
}


int CvCity::getWorkingPopulation() const
{
	return m_iWorkingPopulation;
}


void CvCity::changeWorkingPopulation(int iChange)														
{
	m_iWorkingPopulation = (m_iWorkingPopulation + iChange);
	FAssert(getWorkingPopulation() >= 0);
}


int CvCity::getSpecialistPopulation() const
{
	return m_iSpecialistPopulation;
}


void CvCity::changeSpecialistPopulation(int iChange)													
{
	if (iChange != 0)
	{
		m_iSpecialistPopulation = (m_iSpecialistPopulation + iChange);
		FAssert(getSpecialistPopulation() >= 0);

		GET_PLAYER(getOwnerINLINE()).invalidateYieldRankCache();

		updateCommerce();
	}
}


int CvCity::getNumGreatPeople() const
{
	return m_iNumGreatPeople;
}


void CvCity::changeNumGreatPeople(int iChange)															
{
	if (iChange != 0)
	{
		m_iNumGreatPeople = (m_iNumGreatPeople + iChange);
		FAssert(getNumGreatPeople() >= 0);

		updateCommerce();
	}
}


int CvCity::getBaseGreatPeopleRate() const
{
	return m_iBaseGreatPeopleRate;
}


int CvCity::getGreatPeopleRate() const
{
	if (isDisorder())
	{
		return 0;
	}

	return ((getBaseGreatPeopleRate() * getTotalGreatPeopleRateModifier()) / 100);
}


int CvCity::getTotalGreatPeopleRateModifier() const
{
	int iModifier = getGreatPeopleRateModifier();

	iModifier += GET_PLAYER(getOwnerINLINE()).getGreatPeopleRateModifier();

	if (GET_PLAYER(getOwnerINLINE()).getStateReligion() != NO_RELIGION)
	{
		if (isHasReligion(GET_PLAYER(getOwnerINLINE()).getStateReligion()))
		{
			iModifier += GET_PLAYER(getOwnerINLINE()).getStateReligionGreatPeopleRateModifier();
		}
	}

	if (GET_PLAYER(getOwnerINLINE()).isGoldenAge())
	{
		iModifier += GC.getDefineINT("GOLDEN_AGE_GREAT_PEOPLE_MODIFIER");
	}

	return std::max(0, (iModifier + 100));
}


void CvCity::changeBaseGreatPeopleRate(int iChange)
{
	m_iBaseGreatPeopleRate = (m_iBaseGreatPeopleRate + iChange);
	FAssert(getBaseGreatPeopleRate() >= 0);
}


int CvCity::getGreatPeopleRateModifier() const
{
	return m_iGreatPeopleRateModifier;
}


void CvCity::changeGreatPeopleRateModifier(int iChange)
{
	m_iGreatPeopleRateModifier = (m_iGreatPeopleRateModifier + iChange);
}

// BUG - Building Additional Great People - start
/*
 * Returns the total additional great people rate that adding one of the given buildings will provide.
 *
 * Doesn't check if the building can be constructed in this city.
 */
int CvCity::getAdditionalGreatPeopleRateByBuilding(BuildingTypes eBuilding) const
{
	FAssertMsg(eBuilding >= 0, "eBuilding expected to be >= 0");
	FAssertMsg(eBuilding < GC.getNumBuildingInfos(), "eBuilding expected to be < GC.getNumBuildingInfos()");

	int iRate = getBaseGreatPeopleRate();
	int iModifier = getTotalGreatPeopleRateModifier();
	int iExtra = ((iRate + getAdditionalBaseGreatPeopleRateByBuilding(eBuilding)) * (iModifier + getAdditionalGreatPeopleRateModifierByBuilding(eBuilding)) / 100) - (iRate * iModifier / 100);

	return iExtra;
}

/*
 * Returns the additional great people rate that adding one of the given buildings will provide.
 *
 * Doesn't check if the building can be constructed in this city.
 */
int CvCity::getAdditionalBaseGreatPeopleRateByBuilding(BuildingTypes eBuilding) const
{
	FAssertMsg(eBuilding >= 0, "eBuilding expected to be >= 0");
	FAssertMsg(eBuilding < GC.getNumBuildingInfos(), "eBuilding expected to be < GC.getNumBuildingInfos()");

	CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	bool bObsolete = GET_TEAM(getTeam()).isObsoleteBuilding(eBuilding);
	int iExtraRate = 0;

	iExtraRate += kBuilding.getGreatPeopleRateChange();

	// Specialists
	if (!bObsolete)
	{
		for (int iI = 0; iI < GC.getNumSpecialistInfos(); ++iI)
		{
			if (kBuilding.getFreeSpecialistCount((SpecialistTypes)iI) != 0)
			{
				iExtraRate += getAdditionalBaseGreatPeopleRateBySpecialist((SpecialistTypes)iI, kBuilding.getFreeSpecialistCount((SpecialistTypes)iI));
			}
		}
	}

	return iExtraRate;
}

/*
 * Returns the additional great people rate modifier that adding one of the given buildings will provide.
 *
 * Doesn't check if the building can be constructed in this city.
 */
int CvCity::getAdditionalGreatPeopleRateModifierByBuilding(BuildingTypes eBuilding) const
{
	FAssertMsg(eBuilding >= 0, "eBuilding expected to be >= 0");
	FAssertMsg(eBuilding < GC.getNumBuildingInfos(), "eBuilding expected to be < GC.getNumBuildingInfos()");

	CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	bool bObsolete = GET_TEAM(getTeam()).isObsoleteBuilding(eBuilding);
	int iExtraModifier = 0;

	if (!bObsolete)
	{
		iExtraModifier += kBuilding.getGreatPeopleRateModifier();
		iExtraModifier += kBuilding.getGlobalGreatPeopleRateModifier();
	}

	return iExtraModifier;
}
// BUG - Building Additional Great People - end


// BUG - Specialist Additional Great People - start
/*
 * Returns the total additional great people rate that changing the number of the given specialist will provide/remove.
 */
int CvCity::getAdditionalGreatPeopleRateBySpecialist(SpecialistTypes eSpecialist, int iChange) const
{
	int iRate = getBaseGreatPeopleRate();
	int iModifier = getTotalGreatPeopleRateModifier();
	int iExtraRate = getAdditionalBaseGreatPeopleRateBySpecialist(eSpecialist, iChange);

	int iExtra = ((iRate + iExtraRate) * iModifier / 100) - (iRate * iModifier / 100);

	return iExtra;
}

/*
 * Returns the additional great people rate that changing the number of the given specialist will provide/remove.
 */
int CvCity::getAdditionalBaseGreatPeopleRateBySpecialist(SpecialistTypes eSpecialist, int iChange) const
{
	FAssertMsg(eSpecialist >= 0, "eSpecialist expected to be >= 0");
	FAssertMsg(eSpecialist < GC.getNumSpecialistInfos(), "eSpecialist expected to be < GC.getNumSpecialistInfos()");

	return iChange * GC.getSpecialistInfo(eSpecialist).getGreatPeopleRateChange();
}
// BUG - Specialist Additional Great People - end


int CvCity::getGreatPeopleProgress() const
{
	return m_iGreatPeopleProgress;
}


void CvCity::changeGreatPeopleProgress(int iChange)
{
	// <advc.078>
	if(m_iGreatPeopleProgress <= 0 && iChange > 0)
		GET_PLAYER(getOwnerINLINE()).reportFirstGPP(); // </advc.078>
	m_iGreatPeopleProgress = (m_iGreatPeopleProgress + iChange);
	FAssert(getGreatPeopleProgress() >= 0);
}


int CvCity::getNumWorldWonders() const
{
	return m_iNumWorldWonders;
}


void CvCity::changeNumWorldWonders(int iChange)
{
	m_iNumWorldWonders = (m_iNumWorldWonders + iChange);
	FAssert(getNumWorldWonders() >= 0);
}


int CvCity::getNumTeamWonders() const
{
	return m_iNumTeamWonders;
}


void CvCity::changeNumTeamWonders(int iChange)
{
	m_iNumTeamWonders = (m_iNumTeamWonders + iChange);
	FAssert(getNumTeamWonders() >= 0);
}


int CvCity::getNumNationalWonders() const
{
	return m_iNumNationalWonders;
}


void CvCity::changeNumNationalWonders(int iChange)
{
	m_iNumNationalWonders = (m_iNumNationalWonders + iChange);
	FAssert(getNumNationalWonders() >= 0);
}


int CvCity::getNumBuildings() const
{
	return m_iNumBuildings;
}


void CvCity::changeNumBuildings(int iChange)
{
	m_iNumBuildings = (m_iNumBuildings + iChange);
	FAssert(getNumBuildings() >= 0);
}


int CvCity::getGovernmentCenterCount() const																
{
	return m_iGovernmentCenterCount;
}


bool CvCity::isGovernmentCenter() const
{
	return (getGovernmentCenterCount() > 0);
}


void CvCity::changeGovernmentCenterCount(int iChange)
{
	if (iChange != 0)
	{
		m_iGovernmentCenterCount = (m_iGovernmentCenterCount + iChange);
		FAssert(getGovernmentCenterCount() >= 0);

		GET_PLAYER(getOwnerINLINE()).updateMaintenance();
	}
}


// BUG - Building Saved Maintenance - start
/*
 * Returns the total additional gold from saved maintenance times 100 that adding one of the given buildings will provide.
 *
 * Doesn't check if the building can be constructed in this city.
 */
int CvCity::getSavedMaintenanceTimes100ByBuilding(BuildingTypes eBuilding) const
{
	FAssertMsg(eBuilding >= 0, "eBuilding expected to be >= 0");
	FAssertMsg(eBuilding < GC.getNumBuildingInfos(), "eBuilding expected to be < GC.getNumBuildingInfos()");

	CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	int iModifier = kBuilding.getMaintenanceModifier();
	if (iModifier != 0 && !isDisorder() && !isWeLoveTheKingDay() && (getPopulation() > 0))
	{
		int iNewMaintenance = calculateBaseMaintenanceTimes100() * std::max(0, getMaintenanceModifier() + iModifier + 100) / 100;
		//return getMaintenanceTimes100() - iNewMaintenance;
		return ROUND_DIVIDE((getMaintenanceTimes100() - iNewMaintenance)*(100+GET_PLAYER(getOwnerINLINE()).calculateInflationRate()), 100); // K-Mod
	}

	return 0;
}
// BUG - Building Saved Maintenance - end

int CvCity::getMaintenance() const
{
	return m_iMaintenance / 100;
}

int CvCity::getMaintenanceTimes100() const
{
	return m_iMaintenance;
}


void CvCity::updateMaintenance()
{
	PROFILE_FUNC();

	int iOldMaintenance;
	int iNewMaintenance;

	iOldMaintenance = getMaintenanceTimes100();

	iNewMaintenance = 0;

	if (!isDisorder() && !isWeLoveTheKingDay() && (getPopulation() > 0))
	{
		iNewMaintenance = (calculateBaseMaintenanceTimes100() * std::max(0, (getMaintenanceModifier() + 100))) / 100;
	}

	if (iOldMaintenance != iNewMaintenance)
	{
		FAssert(iOldMaintenance >= 0);
		FAssert(iNewMaintenance >= 0);

		m_iMaintenance = iNewMaintenance;
		FAssert(getMaintenance() >= 0);

		GET_PLAYER(getOwnerINLINE()).changeTotalMaintenance(getMaintenanceTimes100() - iOldMaintenance);
	}
}

int CvCity::calculateDistanceMaintenance() const
{
	return (calculateDistanceMaintenanceTimes100() / 100);
}

// advc.104: Added parameter
int CvCity::calculateDistanceMaintenanceTimes100(PlayerTypes eOwner) const
{
	// advc.004b: BtS code moved into new static function
	return CvCity::calculateDistanceMaintenanceTimes100(*plot(),
			eOwner == NO_PLAYER ? getOwnerINLINE() : eOwner, getPopulation());
}

int CvCity::calculateNumCitiesMaintenance() const
{
	return (calculateNumCitiesMaintenanceTimes100() / 100);
}

// advc.104: Added parameter
int CvCity::calculateNumCitiesMaintenanceTimes100(PlayerTypes eOwner) const
{
	// advc.004b: BtS code moved into new static function
	return calculateNumCitiesMaintenanceTimes100(*plot(),
			eOwner == NO_PLAYER ? getOwnerINLINE() : eOwner,
			getPopulation());
}

int CvCity::calculateColonyMaintenance() const
{
	return (calculateColonyMaintenanceTimes100() / 100);
}

// advc.104: Added parameter
int CvCity::calculateColonyMaintenanceTimes100(PlayerTypes eOwner) const
{
	// advc.004b: BtS code moved into new static function
	return calculateColonyMaintenanceTimes100(*plot(),
			eOwner == NO_PLAYER ? getOwnerINLINE() : eOwner,
			getPopulation());
}


int CvCity::calculateCorporationMaintenance() const
{
	return (calculateCorporationMaintenanceTimes100() / 100);
}

int CvCity::calculateCorporationMaintenanceTimes100() const
{
	int iMaintenance = 0;

	for (int iCorporation = 0; iCorporation < GC.getNumCorporationInfos(); ++iCorporation)
	{
		if (isActiveCorporation((CorporationTypes)iCorporation))
		{
			iMaintenance += calculateCorporationMaintenanceTimes100((CorporationTypes)iCorporation);
		}
	}

	FAssert(iMaintenance >= 0);

	return iMaintenance;
}

int CvCity::calculateCorporationMaintenanceTimes100(CorporationTypes eCorporation) const
{
	int iMaintenance = 0;

	for (int iCommerce = 0; iCommerce < NUM_COMMERCE_TYPES; ++iCommerce)
	{
		iMaintenance += 100 * GC.getCorporationInfo(eCorporation).getHeadquarterCommerce(iCommerce);
	}

	int iNumBonuses = 0;
	for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
	{
		BonusTypes eBonus = (BonusTypes)GC.getCorporationInfo(eCorporation).getPrereqBonus(i);
		if (NO_BONUS != eBonus)
		{
			iNumBonuses += getNumBonuses(eBonus);
		}
	}

	int iBonusMaintenance = GC.getCorporationInfo(eCorporation).getMaintenance() * iNumBonuses;
	iBonusMaintenance *= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getCorporationMaintenancePercent();
	iBonusMaintenance /= 100;
	iMaintenance += iBonusMaintenance;

	iMaintenance *= (getPopulation() + 17);
	iMaintenance /= 18;

	iMaintenance *= GC.getHandicapInfo(getHandicapType()).getCorporationMaintenancePercent();
	iMaintenance /= 100;

	iMaintenance *= std::max(0, (GET_PLAYER(getOwnerINLINE()).getCorporationMaintenanceModifier() + 100));
	iMaintenance /= 100;

	int iInflation = GET_PLAYER(getOwnerINLINE()).calculateInflationRate() + 100;
	if (iInflation > 0)
	{
		iMaintenance *= 100;
		iMaintenance /= iInflation;
	}

	FAssert(iMaintenance >= 0);
	// K-Mod note. This assert (and others like it) can fail sometimes during the process or updating plot groups; because the # of bonuses can be temporarily negative.

	return iMaintenance;
}


int CvCity::calculateBaseMaintenanceTimes100() const
{
	return (calculateDistanceMaintenanceTimes100() + calculateNumCitiesMaintenanceTimes100() + calculateColonyMaintenanceTimes100() + calculateCorporationMaintenanceTimes100());
}


int CvCity::getMaintenanceModifier() const
{
	return m_iMaintenanceModifier;
}


void CvCity::changeMaintenanceModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iMaintenanceModifier = (m_iMaintenanceModifier + iChange);

		updateMaintenance();
	}
}


int CvCity::getWarWearinessModifier() const
{
	return m_iWarWearinessModifier;
}


void CvCity::changeWarWearinessModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iWarWearinessModifier = (m_iWarWearinessModifier + iChange);

		AI_setAssignWorkDirty(true);
	}
}


int CvCity::getHurryAngerModifier() const
{
	return m_iHurryAngerModifier;
}


void CvCity::changeHurryAngerModifier(int iChange)
{
	if (0 != iChange)
	{
		int iRatio = 0;

		if (m_iHurryAngerTimer > 0)
		{
			iRatio = (100 * (m_iHurryAngerTimer - 1)) / std::max(1, 100 + getHurryAngerModifier());
		}

		m_iHurryAngerModifier += iChange;

		if (m_iHurryAngerTimer > 0)
		{
			m_iHurryAngerTimer = (iRatio * std::max(1, 100 + getHurryAngerModifier())) / 100 + 1;
		}
	}
}


int CvCity::getHealRate() const
{
	return m_iHealRate;
}


void CvCity::changeHealRate(int iChange)
{
	m_iHealRate = (m_iHealRate + iChange);
	FAssert(getHealRate() >= 0);
}

int CvCity::getEspionageHealthCounter() const
{
	return m_iEspionageHealthCounter;
}


void CvCity::changeEspionageHealthCounter(int iChange)
{
	if (iChange != 0)
	{
		m_iEspionageHealthCounter += iChange;
	}
}

int CvCity::getEspionageHappinessCounter() const
{
	return m_iEspionageHappinessCounter;
}


void CvCity::changeEspionageHappinessCounter(int iChange)
{
	if (iChange != 0)
	{
		m_iEspionageHappinessCounter += iChange;
	}
}


int CvCity::getFreshWaterGoodHealth() const
{
	return m_iFreshWaterGoodHealth;
}


int CvCity::getFreshWaterBadHealth() const
{
	return m_iFreshWaterBadHealth;
}


void CvCity::updateFreshWaterHealth()
{
	int iNewGoodHealth = 0;
	int iNewBadHealth = 0;

	if (plot()->isFreshWater())
	{
		if (GC.getDefineINT("FRESH_WATER_HEALTH_CHANGE") > 0)
		{
			iNewGoodHealth += GC.getDefineINT("FRESH_WATER_HEALTH_CHANGE");
		}
		else
		{
			iNewBadHealth += GC.getDefineINT("FRESH_WATER_HEALTH_CHANGE");
		}
	}

	if (getFreshWaterGoodHealth() != iNewGoodHealth || getFreshWaterBadHealth() != iNewBadHealth)
	{
		m_iFreshWaterGoodHealth = iNewGoodHealth;
		m_iFreshWaterBadHealth = iNewBadHealth;
		FAssert(getFreshWaterGoodHealth() >= 0);
		FAssert(getFreshWaterBadHealth() <= 0);

		AI_setAssignWorkDirty(true);

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			setInfoDirty(true);
		}
	}
}


int CvCity::getFeatureGoodHealth() const
{
	return m_iFeatureGoodHealth;
}


int CvCity::getFeatureBadHealth() const
{
	return m_iFeatureBadHealth;
}


void CvCity::updateFeatureHealth()
{
	int iNewGoodHealth = 0;
	int iNewBadHealth = 0;

	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = getCityIndexPlot(iI);
		if (pLoopPlot != NULL)
		{
			FeatureTypes eFeature = pLoopPlot->getFeatureType();
			if (eFeature != NO_FEATURE)
			{
				if (GC.getFeatureInfo(eFeature).getHealthPercent() > 0)
				{
					iNewGoodHealth += GC.getFeatureInfo(eFeature).getHealthPercent();
				}
				else
				{
					iNewBadHealth += GC.getFeatureInfo(eFeature).getHealthPercent();
				}
			}
		}
	}

	iNewGoodHealth /= 100;
	iNewBadHealth /= 100;

	if (getFeatureGoodHealth() != iNewGoodHealth || getFeatureBadHealth() != iNewBadHealth)
	{
		m_iFeatureGoodHealth = iNewGoodHealth;
		m_iFeatureBadHealth = iNewBadHealth;
		FAssert(getFeatureGoodHealth() >= 0);
		FAssert(getFeatureBadHealth() <= 0);

		AI_setAssignWorkDirty(true);

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			setInfoDirty(true);
		}
	}
}

// BUG - Actual Effects - start
/*
 * Returns the additional angry population caused by the given happiness changes.
 *
 * Positive values for iBad mean an increase in unhappiness.
 */
int CvCity::getAdditionalAngryPopuplation(int iGood, int iBad) const
{
	int iHappy = happyLevel();
	int iUnhappy = unhappyLevel();
	int iPop = getPopulation();

	return range((iUnhappy + iBad) - (iHappy + iGood), 0, iPop) - range(iUnhappy - iHappy, 0, iPop);
}

/*
 * Returns the additional spoiled food caused by the given health changes.
 *
 * Positive values for iBad mean an increase in unhealthiness.
 */
int CvCity::getAdditionalSpoiledFood(int iGood, int iBad) const
{
	int iHealthy = goodHealth();
	int iUnhealthy = badHealth();
	int iRate = iHealthy - iUnhealthy;

	return std::min(0, iRate) - std::min(0, iRate + iGood - iBad);
}

/*
 * Returns the additional starvation caused by the given spoiled food.
 */
int CvCity::getAdditionalStarvation(int iSpoiledFood) const
{
	int iFood = getYieldRate(YIELD_FOOD) - foodConsumption();

	if (iSpoiledFood > 0)
	{
		if (iFood <= 0)
		{
			return iSpoiledFood;
		}
		else if (iSpoiledFood > iFood)
		{
			return iSpoiledFood - iFood;
		}
	}
	else if (iSpoiledFood < 0)
	{
		if (iFood < 0)
		{
			return std::max(iFood, iSpoiledFood);
		}
	}

	return 0;
} // BUG - Actual Effects - end

/*  <advc.001c> These two function bodies are mostly cut and pasted from
	CvGameTextMgr::parseGreatPeopleHelp */
int CvCity::GPTurnsLeft() const {

	if(getGreatPeopleRate() <= 0)
		return -1;
	int iGPPLeft = GET_PLAYER(getOwnerINLINE()).greatPeopleThreshold(false) -
			getGreatPeopleProgress();
	if(iGPPLeft <= 0)
		return 0;
	int r = iGPPLeft / getGreatPeopleRate();
	if(r * getGreatPeopleRate() <  iGPPLeft)
		r++;
	return r;
}

void CvCity::GPProjection(std::vector<std::pair<UnitTypes,int> >& r) const {

	int iTurnsLeft = GPTurnsLeft();
	int iTotalTruncated = 0;
	/*  This should be kOwner.greatPeopleThreshold(false), but I don't want to
		predict GPP overflow here. */
	int iTarget = std::max(1, getGreatPeopleProgress() + iTurnsLeft *
			getGreatPeopleRate());
	for(int iI = 0; iI < GC.getNumUnitInfos(); iI++) {
		UnitTypes eGPType = (UnitTypes)iI;
		int iProgress = getGreatPeopleUnitProgress(eGPType) +
				(iTurnsLeft * getGreatPeopleUnitRate(eGPType) *
				getTotalGreatPeopleRateModifier()) / 100;
		iProgress *= 100;
		iProgress /= iTarget;
		// BtS code was:
		//int iProgress = ((city.getGreatPeopleUnitProgress((UnitTypes)iI) * 100) / iTotalGreatPeopleUnitProgress);
		if(iProgress > 0) {
			iTotalTruncated += iProgress;
			r.push_back(std::make_pair((UnitTypes)iI, iProgress));
		}
	}
	if(iTotalTruncated < 100 && !r.empty())
		r[0].second += 100 - iTotalTruncated;
} // </advc.001c>

int CvCity::getBuildingGoodHealth() const
{
	return m_iBuildingGoodHealth;
}


int CvCity::getBuildingBadHealth() const
{
	return m_iBuildingBadHealth;
}


int CvCity::getBuildingHealth(BuildingTypes eBuilding) const
{
	int iHealth = getBuildingGoodHealth(eBuilding);

	if (!isBuildingOnlyHealthy())
	{
		iHealth += getBuildingBadHealth(eBuilding);
	}

	return iHealth;
}

int CvCity::getBuildingGoodHealth(BuildingTypes eBuilding) const
{
	int iHealth = std::max(0, GC.getBuildingInfo(eBuilding).getHealth());
	iHealth += std::max(0, getBuildingHealthChange((BuildingClassTypes)GC.getBuildingInfo(eBuilding).getBuildingClassType()));
	iHealth += std::max(0, GET_PLAYER(getOwnerINLINE()).getExtraBuildingHealth(eBuilding));

	return iHealth;
}

int CvCity::getBuildingBadHealth(BuildingTypes eBuilding) const
{
	if (isBuildingOnlyHealthy())
	{
		return 0;
	}

	int iHealth = std::min(0, GC.getBuildingInfo(eBuilding).getHealth());
	iHealth += std::min(0, getBuildingHealthChange((BuildingClassTypes)GC.getBuildingInfo(eBuilding).getBuildingClassType()));
	iHealth += std::min(0, GET_PLAYER(getOwnerINLINE()).getExtraBuildingHealth(eBuilding));

	return iHealth;
}

void CvCity::changeBuildingGoodHealth(int iChange)
{
	if (iChange != 0)
	{
		m_iBuildingGoodHealth = (m_iBuildingGoodHealth + iChange);
		FAssert(getBuildingGoodHealth() >= 0);

		AI_setAssignWorkDirty(true);

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			setInfoDirty(true);
		}
	}
}


void CvCity::changeBuildingBadHealth(int iChange)
{
	if (iChange != 0)
	{
		m_iBuildingBadHealth += iChange;
		FAssert(getBuildingBadHealth() <= 0);

		AI_setAssignWorkDirty(true);

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			setInfoDirty(true);
		}
	}
}


int CvCity::getPowerGoodHealth() const
{
	return m_iPowerGoodHealth;
}


int CvCity::getPowerBadHealth() const
{
	return m_iPowerBadHealth;
}


void CvCity::updatePowerHealth()
{
	int iNewGoodHealth;
	int iNewBadHealth;

	iNewGoodHealth = 0;
	iNewBadHealth = 0;

	if (isPower())
	{
		int iPowerHealth = GC.getDefineINT("POWER_HEALTH_CHANGE");
		if (iPowerHealth > 0)
		{
			iNewGoodHealth += iPowerHealth;
		}
		else
		{
			iNewBadHealth += iPowerHealth;
		}
	}

	if (isDirtyPower())
	{
		int iDirtyPowerHealth = GC.getDefineINT("DIRTY_POWER_HEALTH_CHANGE");
		if (iDirtyPowerHealth > 0)
		{
			iNewGoodHealth += iDirtyPowerHealth;
		}
		else
		{
			iNewBadHealth += iDirtyPowerHealth;
		}
	}

	if ((getPowerGoodHealth() != iNewGoodHealth) || (getPowerBadHealth() != iNewBadHealth))
	{
		m_iPowerGoodHealth = iNewGoodHealth;
		m_iPowerBadHealth = iNewBadHealth;
		FAssert(getPowerGoodHealth() >= 0);
		FAssert(getPowerBadHealth() <= 0);

		AI_setAssignWorkDirty(true);

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			setInfoDirty(true);
		}
	}
}


int CvCity::getBonusGoodHealth() const
{
	return m_iBonusGoodHealth;
}


int CvCity::getBonusBadHealth() const
{
	return m_iBonusBadHealth;
}


void CvCity::changeBonusGoodHealth(int iChange)
{
	if (iChange != 0)
	{
		m_iBonusGoodHealth += iChange;
		FAssert(getBonusGoodHealth() >= 0);

		FAssertMsg(getBonusGoodHealth() >= 0, "getBonusGoodHealth is expected to be >= 0");

		AI_setAssignWorkDirty(true);

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			setInfoDirty(true);
		}
	}
}


void CvCity::changeBonusBadHealth(int iChange)
{
	if (iChange != 0)
	{
		m_iBonusBadHealth += iChange;
		FAssert(getBonusBadHealth() <= 0);

		FAssertMsg(getBonusBadHealth() <= 0, "getBonusBadHealth is expected to be <= 0");

		AI_setAssignWorkDirty(true);

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			setInfoDirty(true);
		}
	}
}


int CvCity::getMilitaryHappinessUnits() const
{
	return m_iMilitaryHappinessUnits;
}


int CvCity::getMilitaryHappiness() const
{
	return (getMilitaryHappinessUnits() * GET_PLAYER(getOwnerINLINE()).getHappyPerMilitaryUnit())
			/ 2; // advc.912c
}


void CvCity::changeMilitaryHappinessUnits(int iChange)
{
	if (iChange != 0)
	{
		m_iMilitaryHappinessUnits = (m_iMilitaryHappinessUnits + iChange);
		FAssert(getMilitaryHappinessUnits() >= 0);

		AI_setAssignWorkDirty(true);
	}
}


int CvCity::getBuildingGoodHappiness() const
{
	return m_iBuildingGoodHappiness;
}


int CvCity::getBuildingBadHappiness() const
{
	return m_iBuildingBadHappiness;
}


int CvCity::getBuildingHappiness(BuildingTypes eBuilding) const
{
	int iHappiness;
	int iI;

	iHappiness = GC.getBuildingInfo(eBuilding).getHappiness();

	if (GC.getBuildingInfo(eBuilding).getReligionType() != NO_RELIGION)
	{
		if (GC.getBuildingInfo(eBuilding).getReligionType() == GET_PLAYER(getOwnerINLINE()).getStateReligion())
		{
			iHappiness += GC.getBuildingInfo(eBuilding).getStateReligionHappiness();
		}
	}

	iHappiness += GET_PLAYER(getOwnerINLINE()).getExtraBuildingHappiness(eBuilding);

	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		iHappiness += ((GC.getBuildingInfo(eBuilding).getCommerceHappiness(iI) * GET_PLAYER(getOwnerINLINE()).getCommercePercent((CommerceTypes)iI)) / 100);
	}

	iHappiness += getBuildingHappyChange((BuildingClassTypes)GC.getBuildingInfo(eBuilding).getBuildingClassType());

	return iHappiness;
}


void CvCity::changeBuildingGoodHappiness(int iChange)
{
	if (iChange != 0)
	{
		m_iBuildingGoodHappiness = (m_iBuildingGoodHappiness + iChange);
		FAssert(getBuildingGoodHappiness() >= 0);

		AI_setAssignWorkDirty(true);
	}
}


void CvCity::changeBuildingBadHappiness(int iChange)
{
	if (iChange != 0)
	{
		m_iBuildingBadHappiness = (m_iBuildingBadHappiness + iChange);
		FAssert(getBuildingBadHappiness() <= 0);

		AI_setAssignWorkDirty(true);
	}
}


int CvCity::getExtraBuildingGoodHappiness() const
{
	return m_iExtraBuildingGoodHappiness;
}


int CvCity::getExtraBuildingBadHappiness() const
{
	return m_iExtraBuildingBadHappiness;
}


void CvCity::updateExtraBuildingHappiness()
{
	int iNewExtraBuildingGoodHappiness = 0;
	int iNewExtraBuildingBadHappiness = 0;

	for (int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		int iChange = getNumActiveBuilding((BuildingTypes)iI) * GET_PLAYER(getOwnerINLINE()).getExtraBuildingHappiness((BuildingTypes)iI);

		if (iChange > 0)
		{
			iNewExtraBuildingGoodHappiness += iChange;
		}
		else
		{
			iNewExtraBuildingBadHappiness += iChange;
		}
	}

	if (getExtraBuildingGoodHappiness() != iNewExtraBuildingGoodHappiness)
	{
		m_iExtraBuildingGoodHappiness = iNewExtraBuildingGoodHappiness;
		FAssert(getExtraBuildingGoodHappiness() >= 0);

		AI_setAssignWorkDirty(true);
	}

	if (getExtraBuildingBadHappiness() != iNewExtraBuildingBadHappiness)
	{
		m_iExtraBuildingBadHappiness = iNewExtraBuildingBadHappiness;
		FAssert(getExtraBuildingBadHappiness() <= 0);

		AI_setAssignWorkDirty(true);
	}
}

/*  BETTER_BTS_AI_MOD, City AI, 02/24/10, EmperorFool: START
	(BUG - Building Additional Happiness) */
/*
 * Returns the total additional happiness that adding one of the given buildings will provide
 * and sets the good and bad levels individually.
 *
 * Doesn't reset iGood or iBad to zero.
 * Doesn't check if the building can be constructed in this city.
 */
int CvCity::getAdditionalHappinessByBuilding(BuildingTypes eBuilding, int& iGood, int& iBad) const
{
	FAssertMsg(eBuilding >= 0, "eBuilding expected to be >= 0");
	FAssertMsg(eBuilding < GC.getNumBuildingInfos(), "eBuilding expected to be < GC.getNumBuildingInfos()");

	CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	int iI;
	int iStarting = iGood - iBad;
	int iStartingBad = iBad;

	// Basic
	addGoodOrBad(kBuilding.getHappiness(), iGood, iBad);

	// Building Class
	addGoodOrBad(getBuildingHappyChange((BuildingClassTypes)kBuilding.getBuildingClassType()), iGood, iBad);

	// Player Building
	addGoodOrBad(GET_PLAYER(getOwnerINLINE()).getExtraBuildingHappiness(eBuilding), iGood, iBad);

	// Area
	addGoodOrBad(kBuilding.getAreaHappiness(), iGood, iBad);

	// Religion
	if (kBuilding.getReligionType() != NO_RELIGION && kBuilding.getReligionType() == GET_PLAYER(getOwnerINLINE()).getStateReligion())
	{
		iGood += kBuilding.getStateReligionHappiness();
	}

	// Bonus
	for (iI = 0; iI < GC.getNumBonusInfos(); iI++)
	{
		if ((hasBonus((BonusTypes)iI) || kBuilding.getFreeBonus() == iI) && kBuilding.getNoBonus() != iI)
		{
			addGoodOrBad(kBuilding.getBonusHappinessChanges(iI), iGood, iBad);
		}
	}

	// Commerce
	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		addGoodOrBad(kBuilding.getCommerceHappiness(iI) * GET_PLAYER(getOwnerINLINE()).getCommercePercent((CommerceTypes)iI) / 100, iGood, iBad);
	}

	// War Weariness Modifier
	if (kBuilding.getWarWearinessModifier() != 0)
	{
		int iBaseAngerPercent = 0;

		iBaseAngerPercent += getOvercrowdingPercentAnger();
		iBaseAngerPercent += getNoMilitaryPercentAnger();
		iBaseAngerPercent += getCulturePercentAnger();
		iBaseAngerPercent += getReligionPercentAnger();
		iBaseAngerPercent += getHurryPercentAnger();
		iBaseAngerPercent += getConscriptPercentAnger();
		iBaseAngerPercent += getDefyResolutionPercentAnger();
		for (iI = 0; iI < GC.getNumCivicInfos(); iI++)
		{
			iBaseAngerPercent += GET_PLAYER(getOwnerINLINE()).getCivicPercentAnger((CivicTypes)iI);
		}

		int iCurrentAngerPercent = iBaseAngerPercent + getWarWearinessPercentAnger();
		int iCurrentUnhappiness = iCurrentAngerPercent * getPopulation() / GC.getPERCENT_ANGER_DIVISOR();

		int iNewWarAngerPercent = GET_PLAYER(getOwnerINLINE()).getWarWearinessPercentAnger();
		iNewWarAngerPercent *= std::max(0, (kBuilding.getWarWearinessModifier() + getWarWearinessModifier() + GET_PLAYER(getOwnerINLINE()).getWarWearinessModifier() + 100));
		iNewWarAngerPercent /= 100;
		int iNewAngerPercent = iBaseAngerPercent + iNewWarAngerPercent;
		int iNewUnhappiness = iNewAngerPercent * getPopulation() / GC.getPERCENT_ANGER_DIVISOR();

		iBad += iNewUnhappiness - iCurrentUnhappiness;
	}
	
	// K-Mod. If the city is immune to unhappiness, then clear the "bad" from this building.
	if (isNoUnhappiness())
	{
		iBad = iStartingBad;
	}
	// K-Mod end

	// No Unhappiness
	if (kBuilding.isNoUnhappiness())
	{
		// override extra unhappiness and completely negate all existing unhappiness
		iBad = iStartingBad - unhappyLevel();
	}

	return iGood - iBad - iStarting;
}

/*
 * Returns the total additional health that adding one of the given buildings will provide
 * and sets the good and bad levels individually.
 *
 * Doesn't reset iGood or iBad to zero.
 * Doesn't check if the building can be constructed in this city.
 */
int CvCity::getAdditionalHealthByBuilding(BuildingTypes eBuilding, int& iGood, int& iBad,
		bool bAssumeStrategicBonuses) const // advc.001h
{
	FAssertMsg(eBuilding >= 0, "eBuilding expected to be >= 0");
	FAssertMsg(eBuilding < GC.getNumBuildingInfos(), "eBuilding expected to be < GC.getNumBuildingInfos()");

	CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	int iI;
	int iStarting = iGood - iBad;
	int iStartingBad = iBad;

	// Basic
	addGoodOrBad(kBuilding.getHealth(), iGood, iBad);

	// Building Class
	addGoodOrBad(getBuildingHealthChange((BuildingClassTypes)kBuilding.getBuildingClassType()), iGood, iBad);

	// Player Building
	addGoodOrBad(GET_PLAYER(getOwnerINLINE()).getExtraBuildingHealth(eBuilding), iGood, iBad);

	// Area
	addGoodOrBad(kBuilding.getAreaHealth(), iGood, iBad);

	// No Unhealthiness from Buildings
	if (isBuildingOnlyHealthy())
	{
		// undo bad from this building
		iBad = iStartingBad;
	}
	if (kBuilding.isBuildingOnlyHealthy())
	{
		// undo bad from this and all existing buildings
		iBad = iStartingBad + totalBadBuildingHealth();
	}

	// Bonus
	for (iI = 0; iI < GC.getNumBonusInfos(); iI++)
	{
		if ((hasBonus((BonusTypes)iI) || kBuilding.getFreeBonus() == iI
				// <advc.001h>
				|| (bAssumeStrategicBonuses &&
				GC.getBonusInfo((BonusTypes)iI).getHappiness() == 0 &&
				GC.getBonusInfo((BonusTypes)iI).getHealth() == 0)) // </advc.001h>
				&& kBuilding.getNoBonus() != iI)
			addGoodOrBad(kBuilding.getBonusHealthChanges(iI), iGood, iBad);
	}
	// advc.001h: Need this several times
	BonusTypes ePowBonus = (BonusTypes)kBuilding.getPowerBonus();
	// Power
	if (kBuilding.isPower() || kBuilding.isAreaCleanPower() || 
			(ePowBonus != NO_BONUS && (hasBonus(ePowBonus)
			// <advc.001h>
			|| (bAssumeStrategicBonuses &&
			GC.getBonusInfo(ePowBonus).getHappiness() == 0 &&
			GC.getBonusInfo(ePowBonus).getHealth() == 0)))) // </advc.001h>
	{
		// adding power
		if (!isPower())
		{
			addGoodOrBad(GC.getDefineINT("POWER_HEALTH_CHANGE"), iGood, iBad);

			// adding dirty power
			if (kBuilding.isDirtyPower())
				addGoodOrBad(GC.getDefineINT("DIRTY_POWER_HEALTH_CHANGE"), iGood, iBad);
		} /* advc.001h: Count change from dirty to clean only if we already have
			 the resource (i.e. Uranium) */
		else if(ePowBonus == NO_BONUS || hasBonus(ePowBonus))
		{
			// replacing dirty power with clean power
			if (isDirtyPower() && !kBuilding.isDirtyPower())
			{
				subtractGoodOrBad(GC.getDefineINT("DIRTY_POWER_HEALTH_CHANGE"), iGood, iBad);
			}
		}
	}
	/* original bts code
	// No Unhealthiness from Population
	if (kBuilding.isNoUnhealthyPopulation())
		iBad -= getPopulation();*/
	/*  K-Mod, 27/dec/10, karadoc (start)
		replaced NoUnhealthyPopulation with UnhealthyPopulationModifier */
	// Modified unhealthiness from population
	int iEffectiveModifier = 0;
	if (kBuilding.getUnhealthyPopulationModifier()+getUnhealthyPopulationModifier() < -100)
	{
		iEffectiveModifier = std::min(0, -100 - getUnhealthyPopulationModifier());
	}
	else
	{
		iEffectiveModifier = std::max(-100, kBuilding.getUnhealthyPopulationModifier());
	}
	iBad += ROUND_DIVIDE(getPopulation() * iEffectiveModifier, 100);
	// K-Mod end
	return iGood - iBad - iStarting;
}

/*
 * Adds iValue to iGood if it is positive or its negative to iBad if it is negative.
 */
// advc.003: Turn these into CvCity members for now as they're only used by CvCity
void CvCity::addGoodOrBad(int iValue, int& iGood, int& iBad)
{
	if (iValue > 0)
	{
		iGood += iValue;
	}
	else if (iValue < 0)
	{
		iBad -= iValue;
	}
}

/*
 * Subtracts iValue from iGood if it is positive or its negative from iBad if it is negative.
 */
void CvCity::subtractGoodOrBad(int iValue, int& iGood, int& iBad)
{
	if (iValue > 0)
	{
		iGood -= iValue;
	}
	else if (iValue < 0)
	{
		iBad += iValue;
	}
}
// BETTER_BTS_AI_MOD: END

int CvCity::getExtraBuildingGoodHealth() const
{
	return m_iExtraBuildingGoodHealth;
}


int CvCity::getExtraBuildingBadHealth() const
{
	return m_iExtraBuildingBadHealth;
}


void CvCity::updateExtraBuildingHealth()
{
	int iNewExtraBuildingGoodHealth = 0;
	int iNewExtraBuildingBadHealth = 0;

	for (int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		int iChange = getNumActiveBuilding((BuildingTypes)iI) * GET_PLAYER(getOwnerINLINE()).getExtraBuildingHealth((BuildingTypes)iI);

		if (iChange > 0)
		{
			iNewExtraBuildingGoodHealth += iChange;
		}
		else
		{
			iNewExtraBuildingBadHealth += iChange;
		}
	}

	if (getExtraBuildingGoodHealth() != iNewExtraBuildingGoodHealth)
	{
		m_iExtraBuildingGoodHealth = iNewExtraBuildingGoodHealth;
		FAssert(getExtraBuildingGoodHealth() >= 0);

		AI_setAssignWorkDirty(true);
	}

	if (getExtraBuildingBadHealth() != iNewExtraBuildingBadHealth)
	{
		m_iExtraBuildingBadHealth = iNewExtraBuildingBadHealth;
		FAssert(getExtraBuildingBadHealth() <= 0);

		AI_setAssignWorkDirty(true);
	}
}


int CvCity::getFeatureGoodHappiness() const
{
	return m_iFeatureGoodHappiness;
}


int CvCity::getFeatureBadHappiness() const
{
	return m_iFeatureBadHappiness;
}


void CvCity::updateFeatureHappiness()
{
	int iNewFeatureGoodHappiness = 0;
	int iNewFeatureBadHappiness = 0;

	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = getCityIndexPlot(iI);

		if (pLoopPlot != NULL)
		{
			FeatureTypes eFeature = pLoopPlot->getFeatureType();

			if (eFeature != NO_FEATURE)
			{
				int iHappy = GET_PLAYER(getOwnerINLINE()).getFeatureHappiness(eFeature);
				if (iHappy > 0)
				{
					iNewFeatureGoodHappiness += iHappy;
				}
				else
				{
					iNewFeatureBadHappiness += iHappy;
				}
			}

			ImprovementTypes eImprovement = pLoopPlot->getImprovementType();

			if (NO_IMPROVEMENT != eImprovement)
			{
				int iHappy = GC.getImprovementInfo(eImprovement).getHappiness();
				if (iHappy > 0)
				{
					iNewFeatureGoodHappiness += iHappy;
				}
				else
				{
					iNewFeatureBadHappiness += iHappy;
				}
			}
		}
	}

	if (getFeatureGoodHappiness() != iNewFeatureGoodHappiness)
	{
		m_iFeatureGoodHappiness = iNewFeatureGoodHappiness;
		FAssert(getFeatureGoodHappiness() >= 0);

		AI_setAssignWorkDirty(true);
	}

	if (getFeatureBadHappiness() != iNewFeatureBadHappiness)
	{
		m_iFeatureBadHappiness = iNewFeatureBadHappiness;
		FAssert(getFeatureBadHappiness() <= 0);

		AI_setAssignWorkDirty(true);
	}
}


int CvCity::getBonusGoodHappiness(/* advc.912c: */ bool bIgnoreModifier) const 
{
	//return m_iBonusGoodHappiness;
	// <advc.912c> Replacing the above
	int r = m_iBonusGoodHappiness;
	if(!bIgnoreModifier)
		r = (r * (100 + GET_PLAYER(getOwnerINLINE()).getLuxuryModifier())) / 100;
	return r; // </advc.912c>
}


int CvCity::getBonusBadHappiness() const
{
	return m_iBonusBadHappiness;
}


void CvCity::changeBonusGoodHappiness(int iChange)
{
	if (iChange != 0)
	{
		m_iBonusGoodHappiness = (m_iBonusGoodHappiness + iChange);
		FAssert(getBonusGoodHappiness() >= 0);

		AI_setAssignWorkDirty(true);
	}
}


void CvCity::changeBonusBadHappiness(int iChange)
{
	if (iChange != 0)
	{
		m_iBonusBadHappiness = (m_iBonusBadHappiness + iChange);
		FAssert(getBonusBadHappiness() <= 0);

		AI_setAssignWorkDirty(true);
	}
}


int CvCity::getReligionGoodHappiness() const
{
	return m_iReligionGoodHappiness;
}


int CvCity::getReligionBadHappiness() const
{
	return m_iReligionBadHappiness;
}


int CvCity::getReligionHappiness(ReligionTypes eReligion) const
{
	int iHappiness = 0;

	if (isHasReligion(eReligion))
	{
		if (eReligion == GET_PLAYER(getOwnerINLINE()).getStateReligion())
		{
			iHappiness += GET_PLAYER(getOwnerINLINE()).getStateReligionHappiness();
		}
		else
		{
			iHappiness += GET_PLAYER(getOwnerINLINE()).getNonStateReligionHappiness();
		}
	}

	return iHappiness;
}


void CvCity::updateReligionHappiness()
{
	int iNewReligionGoodHappiness = 0;
	int iNewReligionBadHappiness = 0;

	for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		int iChange = getReligionHappiness((ReligionTypes)iI);

		if (iChange > 0)
		{
			iNewReligionGoodHappiness += iChange;
		}
		else
		{
			iNewReligionBadHappiness += iChange;
		}
	}

	if (getReligionGoodHappiness() != iNewReligionGoodHappiness)
	{
		m_iReligionGoodHappiness = iNewReligionGoodHappiness;
		FAssert(getReligionGoodHappiness() >= 0);

		AI_setAssignWorkDirty(true);
	}

	if (getReligionBadHappiness() != iNewReligionBadHappiness)
	{
		m_iReligionBadHappiness = iNewReligionBadHappiness;
		FAssert(getReligionBadHappiness() <= 0);

		AI_setAssignWorkDirty(true);
	}
}


int CvCity::getExtraHappiness() const
{
	return m_iExtraHappiness;
}


void CvCity::changeExtraHappiness(int iChange)
{
	if (iChange != 0)
	{
		m_iExtraHappiness += iChange;

		AI_setAssignWorkDirty(true);
	}
}

int CvCity::getExtraHealth() const
{
	return m_iExtraHealth;
}


void CvCity::changeExtraHealth(int iChange)
{
	if (iChange != 0)
	{
		m_iExtraHealth += iChange;

		AI_setAssignWorkDirty(true);
	}
}



int CvCity::getHurryAngerTimer() const
{
	return m_iHurryAngerTimer;
}


void CvCity::changeHurryAngerTimer(int iChange)
{
	if (iChange != 0)
	{
		m_iHurryAngerTimer = (m_iHurryAngerTimer + iChange);
		FAssert(getHurryAngerTimer() >= 0);

		AI_setAssignWorkDirty(true);
	}
}


int CvCity::getConscriptAngerTimer() const
{
	return m_iConscriptAngerTimer;
}


void CvCity::changeConscriptAngerTimer(int iChange)
{
	if (iChange != 0)
	{
		m_iConscriptAngerTimer = (m_iConscriptAngerTimer + iChange);
		FAssert(getConscriptAngerTimer() >= 0);

		AI_setAssignWorkDirty(true);
	}
}

int CvCity::getDefyResolutionAngerTimer() const
{
	return m_iDefyResolutionAngerTimer;
}


void CvCity::changeDefyResolutionAngerTimer(int iChange)
{
	if (iChange != 0)
	{
		m_iDefyResolutionAngerTimer += iChange;
		FAssert(getDefyResolutionAngerTimer() >= 0);

		AI_setAssignWorkDirty(true);
	}
}

int CvCity::flatDefyResolutionAngerLength() const
{
	int iAnger = GC.getDefineINT("DEFY_RESOLUTION_ANGER_DIVISOR");

	iAnger *= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getHurryConscriptAngerPercent();
	iAnger /= 100;

	return std::max(1, iAnger);
}


int CvCity::getHappinessTimer() const
{
	return m_iHappinessTimer;
}


void CvCity::changeHappinessTimer(int iChange)
{
	if (iChange != 0)
	{
		m_iHappinessTimer += iChange;
		FAssert(getHappinessTimer() >= 0);

		AI_setAssignWorkDirty(true);
	}
}


int CvCity::getNoUnhappinessCount() const
{
	return m_iNoUnhappinessCount;
}


bool CvCity::isNoUnhappiness() const
{
	return (getNoUnhappinessCount() > 0);
}


void CvCity::changeNoUnhappinessCount(int iChange)
{
	if (iChange != 0)
	{
		m_iNoUnhappinessCount = (m_iNoUnhappinessCount + iChange);
		FAssert(getNoUnhappinessCount() >= 0);

		AI_setAssignWorkDirty(true);
	}
}

/*  K-Mod, 27/dec/10, karadoc
	replaced NoUnhealthyPopulation with UnhealthyPopulationModifier */
/* original bts code
int CvCity::getNoUnhealthyPopulationCount()	const {
	return m_iNoUnhealthyPopulationCount;
}
bool CvCity::isNoUnhealthyPopulation() const {
	if (GET_PLAYER(getOwnerINLINE()).isNoUnhealthyPopulation())
		return true;
	return (getNoUnhealthyPopulationCount() > 0);
}
void CvCity::changeNoUnhealthyPopulationCount(int iChange) {
	if (iChange != 0) {
		m_iNoUnhealthyPopulationCount = (m_iNoUnhealthyPopulationCount + iChange);
		FAssert(getNoUnhealthyPopulationCount() >= 0);
		AI_setAssignWorkDirty(true);
	}
}*/

int CvCity::getUnhealthyPopulationModifier() const
{
	return m_iUnhealthyPopulationModifier + GET_PLAYER(getOwnerINLINE()).getUnhealthyPopulationModifier();
}


void CvCity::changeUnhealthyPopulationModifier(int iChange)
{
	m_iUnhealthyPopulationModifier += iChange;
}
// K-Mod end

int CvCity::getBuildingOnlyHealthyCount() const																	
{
	return m_iBuildingOnlyHealthyCount;
}


bool CvCity::isBuildingOnlyHealthy() const																		
 {
	if (GET_PLAYER(getOwnerINLINE()).isBuildingOnlyHealthy())
	{
		return true;
	}

	return (getBuildingOnlyHealthyCount() > 0);
}


void CvCity::changeBuildingOnlyHealthyCount(int iChange)
{
	if (iChange != 0)
	{
		m_iBuildingOnlyHealthyCount = (m_iBuildingOnlyHealthyCount + iChange);
		FAssert(getBuildingOnlyHealthyCount() >= 0);

		AI_setAssignWorkDirty(true);
	}
}


int CvCity::getFood() const
{
	return m_iFood;
}


void CvCity::setFood(int iNewValue)
{
	if (getFood() != iNewValue)
	{
		m_iFood = iNewValue;

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			setInfoDirty(true);
		}
	}
}


void CvCity::changeFood(int iChange)
{
	setFood(getFood() + iChange);
}


int CvCity::getFoodKept() const
{
	return m_iFoodKept;
}


void CvCity::setFoodKept(int iNewValue)
{
	m_iFoodKept = iNewValue;
}


void CvCity::changeFoodKept(int iChange)
{
	setFoodKept(getFoodKept() + iChange);
}


int CvCity::getMaxFoodKeptPercent() const
{
	return m_iMaxFoodKeptPercent;
}


void CvCity::changeMaxFoodKeptPercent(int iChange)
{
	m_iMaxFoodKeptPercent = (m_iMaxFoodKeptPercent + iChange);
	FAssert(getMaxFoodKeptPercent() >= 0);
}


int CvCity::getOverflowProduction() const
{
	return m_iOverflowProduction;
}


void CvCity::setOverflowProduction(int iNewValue)
{
	m_iOverflowProduction = iNewValue;
	FAssert(getOverflowProduction() >= 0);
}


void CvCity::changeOverflowProduction(int iChange, int iProductionModifier)
{
	setOverflowProduction(getOverflowProduction() +
			// advc.064b: Moved into new function
			unmodifyOverflow(iChange, iProductionModifier));
}

// <advc.064b> Cut from changeOverflowProduction
int CvCity::unmodifyOverflow(int iRawOverflow, int iProductionModifier) const {

	return (100 * iRawOverflow) / std::max(1,
			//getBaseYieldRateModifier(YIELD_PRODUCTION, iProductionModifier)
			/*  Keep the BaseYieldRateModifier; same treatment as Processes, and
				that's what we want for production gold too. However, I'm also
				changing the code that uses up overflow production (see getProductionDifference)
				so that the BaseYieldRateModifier isn't applied for a second time.
				On the bottom line, it's almost the same as what the Unofficial Patch,
				does, but results in more intuitive help text. */
			iProductionModifier + 100);
} // </advc.064b>


int CvCity::getFeatureProduction() const
{
	return m_iFeatureProduction;
}


void CvCity::setFeatureProduction(int iNewValue)														
{
	m_iFeatureProduction = iNewValue;
	FAssert(getFeatureProduction() >= 0);
}


void CvCity::changeFeatureProduction(int iChange)													
{
	setFeatureProduction(getFeatureProduction() + iChange);
}


int CvCity::getMilitaryProductionModifier()	const													
{
	return m_iMilitaryProductionModifier;
}


void CvCity::changeMilitaryProductionModifier(int iChange)												
{
	m_iMilitaryProductionModifier = (m_iMilitaryProductionModifier + iChange);
}


int CvCity::getSpaceProductionModifier() const															
{
	return m_iSpaceProductionModifier;
}


void CvCity::changeSpaceProductionModifier(int iChange)												
{
	m_iSpaceProductionModifier = (m_iSpaceProductionModifier + iChange);
}


int CvCity::getExtraTradeRoutes() const
{
	return m_iExtraTradeRoutes;
}


void CvCity::changeExtraTradeRoutes(int iChange)
{
	if (iChange != 0)
	{
		m_iExtraTradeRoutes = (m_iExtraTradeRoutes + iChange);
		FAssert(getExtraTradeRoutes() >= 0);

		updateTradeRoutes();
	}
}


int CvCity::getTradeRouteModifier() const
{
	return m_iTradeRouteModifier;
}

void CvCity::changeTradeRouteModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iTradeRouteModifier = (m_iTradeRouteModifier + iChange);

		updateTradeRoutes();
	}
}

int CvCity::getForeignTradeRouteModifier() const
{
	return m_iForeignTradeRouteModifier;
}

void CvCity::changeForeignTradeRouteModifier(int iChange)
{
	if (iChange != 0)
	{
		m_iForeignTradeRouteModifier = (m_iForeignTradeRouteModifier + iChange);

		updateTradeRoutes();
	}
}

/*  K-Mod, 26/sep/10, Karadoc
	Trade culture calculation */
int CvCity::getTradeCultureRateTimes100(int iLevel) const
{	// <k146>
	// Note: iLevel currently isn't used.
	//int iPercent = (int)getCultureLevel();
	// Note: GC.getNumCultureLevelInfos() is 7 with the standard xml, which means legendary culture is level 6.
	// So we have 3, 4, 4, 5, 5, 6, 6
	int iPercent = (GC.getNumCultureLevelInfos()+(int)getCultureLevel())/2;
	if (iPercent > 0)
	{
		// (originally this was 1% of culture rate for each culture level.)
		// </k146>
		return (m_aiCommerceRate[COMMERCE_CULTURE] * iPercent)/100;
	}
	return 0;
} // K-Mod end


int CvCity::getBuildingDefense() const
{
	return m_iBuildingDefense;
}


void CvCity::changeBuildingDefense(int iChange)
{
	if (iChange != 0)
	{
		m_iBuildingDefense = (m_iBuildingDefense + iChange);
		FAssert(getBuildingDefense() >= 0);

		setInfoDirty(true);

		plot()->plotAction(PUF_makeInfoBarDirty);
	}
}

// BUG - Building Additional Defense - start
int CvCity::getAdditionalDefenseByBuilding(BuildingTypes eBuilding) const
{
	FAssertMsg(eBuilding >= 0, "eBuilding expected to be >= 0");
	FAssertMsg(eBuilding < GC.getNumBuildingInfos(), "eBuilding expected to be < GC.getNumBuildingInfos()");

	CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	int iDefense = std::max(getBuildingDefense() + kBuilding.getDefenseModifier(), getNaturalDefense()) + GET_PLAYER(getOwnerINLINE()).getCityDefenseModifier() + kBuilding.getAllCityDefenseModifier();

	// doesn't take bombardment into account
	return iDefense - getTotalDefense(false);
}
// BUG - Building Additional Defense - end

int CvCity::getBuildingBombardDefense() const
{
	return m_iBuildingBombardDefense;
}


void CvCity::changeBuildingBombardDefense(int iChange)
{
	if (iChange != 0)
	{
		m_iBuildingBombardDefense += iChange;
		FAssert(getBuildingBombardDefense() >= 0);
	}
}


int CvCity::getFreeExperience() const
{
	return m_iFreeExperience;
}


void CvCity::changeFreeExperience(int iChange)
{
	m_iFreeExperience = (m_iFreeExperience + iChange);
	FAssert(getFreeExperience() >= 0);
}


int CvCity::getCurrAirlift() const
{
	return m_iCurrAirlift;
}


void CvCity::setCurrAirlift(int iNewValue)
{
	m_iCurrAirlift = iNewValue;
	FAssert(getCurrAirlift() >= 0);
}


void CvCity::changeCurrAirlift(int iChange)
{
	setCurrAirlift(getCurrAirlift() + iChange);
}


int CvCity::getMaxAirlift() const
{
	return m_iMaxAirlift;
}


void CvCity::changeMaxAirlift(int iChange)
{
	m_iMaxAirlift = (m_iMaxAirlift + iChange);
	FAssert(getMaxAirlift() >= 0);
}

int CvCity::getAirModifier() const
{
	return m_iAirModifier;
}

void CvCity::changeAirModifier(int iChange)
{
	m_iAirModifier += iChange;
}

int CvCity::getAirUnitCapacity(TeamTypes eTeam) const
{
	return (getTeam() == eTeam ? m_iAirUnitCapacity : GC.getDefineINT("CITY_AIR_UNIT_CAPACITY"));
}

void CvCity::changeAirUnitCapacity(int iChange)
{
	m_iAirUnitCapacity += iChange;
	FAssert(getAirUnitCapacity(getTeam()) >= 0);
}

int CvCity::getNukeModifier() const
{
	return m_iNukeModifier;
}


void CvCity::changeNukeModifier(int iChange)
{
	m_iNukeModifier = (m_iNukeModifier + iChange);
}


int CvCity::getFreeSpecialist() const
{
	return m_iFreeSpecialist;
}


void CvCity::changeFreeSpecialist(int iChange)
{
	if (iChange != 0)
	{
		m_iFreeSpecialist = (m_iFreeSpecialist + iChange);
		FAssert(getFreeSpecialist() >= 0);

		AI_setAssignWorkDirty(true);
	}
}


int CvCity::getPowerCount() const
{
	return m_iPowerCount;
}


bool CvCity::isPower() const
{
	return ((getPowerCount() > 0) || isAreaCleanPower());
}


bool CvCity::isAreaCleanPower() const
{
	if (area() == NULL)
	{
		return false;
	}

	return area()->isCleanPower(getTeam());
}


int CvCity::getDirtyPowerCount() const
{
	return m_iDirtyPowerCount;
}


bool CvCity::isDirtyPower() const
{
	return (isPower() && (getDirtyPowerCount() == getPowerCount()) && !isAreaCleanPower());
}


void CvCity::changePowerCount(int iChange, bool bDirty)
{
	if(iChange == 0)
		return;

	bool bOldPower = isPower();
	bool bOldDirtyPower = isDirtyPower();

	m_iPowerCount = (m_iPowerCount + iChange);
	FAssert(getPowerCount() >= 0);
	if (bDirty)
	{
		m_iDirtyPowerCount += iChange;
		FAssert(getDirtyPowerCount() >= 0);
	}

	if (bOldPower != isPower())
	{
		GET_PLAYER(getOwnerINLINE()).invalidateYieldRankCache();

		updateCommerce();

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			setInfoDirty(true);
		}
	}

	if (bOldDirtyPower != isDirtyPower() || bOldPower != isPower())
	{
		updatePowerHealth();
	}
}


int CvCity::getDefenseDamage() const																
{
	return m_iDefenseDamage;
}


void CvCity::changeDefenseDamage(int iChange)
{
	if (iChange == 0)
		return;
	m_iDefenseDamage = range((m_iDefenseDamage + iChange), 0, GC.getMAX_CITY_DEFENSE_DAMAGE());
	if (iChange > 0)
		setBombarded(true);
	setInfoDirty(true);
	plot()->plotAction(PUF_makeInfoBarDirty);
}

void CvCity::changeDefenseModifier(int iChange)
{
	if(iChange == 0) {
		setBombarded(true); // advc.004c: Set bombarded even if no change
		return;
	}
	int iTotalDefense = getTotalDefense(false);
	if(iTotalDefense <= 0)
		return;
	//changeDefenseDamage(-(GC.getMAX_CITY_DEFENSE_DAMAGE() * iChange) / iTotalDefense);
	// <advc.004c> Replacing the above
	int iDefenseDmg = getDefenseDamage();
	int iDefenseMod = getDefenseModifier(false);
	iChange = -iDefenseDmg + (GC.getMAX_CITY_DEFENSE_DAMAGE() * iTotalDefense -
			(iDefenseMod + iChange) * GC.getMAX_CITY_DEFENSE_DAMAGE()) / iTotalDefense;
	changeDefenseDamage(iChange);
	// </advc.004c>
}

/*  advc.003j (comment): Not currently used (b/c K-Mod has removed AI_finalOddsThreshold).
	Gets set in CvCity::doTurn. */
int CvCity::getLastDefenseDamage() const
{
	return m_iLastDefenseDamage;
}

void CvCity::setLastDefenseDamage(int iNewValue)
{
	m_iLastDefenseDamage = iNewValue;
}


bool CvCity::isBombardable(const CvUnit* pUnit) const
{
	if (NULL != pUnit && !pUnit->isEnemy(getTeam()))
		return false;

	return (getDefenseModifier(false) > 0)
		/*  advc.004c: Allow bombarding defenseless cities (to keep their def at 0),
			except cities that have nothing to recover.
			Don't allow the AI to do this b/c the AI can't tell if it matters.
			Much easier to prevent 0-bombardment by the AI here than in CvUnitAI. */
			|| (getTotalDefense(false) > 0 && pUnit->isHuman() && !isBombarded());
}


int CvCity::getNaturalDefense() const
{
	if (getCultureLevel() == NO_CULTURELEVEL)
	{
		return 0;
	}

	return GC.getCultureLevelInfo(getCultureLevel()).getCityDefenseModifier();
}


int CvCity::getTotalDefense(bool bIgnoreBuilding) const
{
	return (std::max(((bIgnoreBuilding) ? 0 : getBuildingDefense()), getNaturalDefense()) + GET_PLAYER(getOwnerINLINE()).getCityDefenseModifier());
}


int CvCity::getDefenseModifier(bool bIgnoreBuilding) const
{
	if (isOccupation())
	{
		return 0;
	}

	return ((getTotalDefense(bIgnoreBuilding) * (GC.getMAX_CITY_DEFENSE_DAMAGE() - getDefenseDamage())) / GC.getMAX_CITY_DEFENSE_DAMAGE());
}


int CvCity::getOccupationTimer() const
{
	return m_iOccupationTimer;
}


bool CvCity::isOccupation() const
{
	return (getOccupationTimer() > 0);
}


void CvCity::setOccupationTimer(int iNewValue)
{
	if(getOccupationTimer() == iNewValue)
		return;

	bool bOldOccupation = isOccupation();

	m_iOccupationTimer = iNewValue;
	FAssert(getOccupationTimer() >= 0);

	if (bOldOccupation != isOccupation())
	{
		updateCorporation();
		updateMaintenance();
		updateTradeRoutes();
		updateCommerce(); // K-Mod

		updateCultureLevel(true);

		AI_setAssignWorkDirty(true);
		// K-Mod
		if (isHuman() && !isDisorder() && AI_isChooseProductionDirty() && !isProduction() && !isProductionAutomated())
			chooseProduction();
		// K-Mod end
	}

	setInfoDirty(true);
}


void CvCity::changeOccupationTimer(int iChange)												
{
	setOccupationTimer(getOccupationTimer() + iChange);
}


int CvCity::getCultureUpdateTimer() const
{
	return m_iCultureUpdateTimer;
}


void CvCity::setCultureUpdateTimer(int iNewValue)
{
	m_iCultureUpdateTimer = iNewValue;
	FAssert(getOccupationTimer() >= 0);
}


void CvCity::changeCultureUpdateTimer(int iChange)												
{
	setCultureUpdateTimer(getCultureUpdateTimer() + iChange);
}


int CvCity::getCitySizeBoost() const
{
	return m_iCitySizeBoost;
}


void CvCity::setCitySizeBoost(int iBoost)
{
	if (getCitySizeBoost() != iBoost)
	{
		m_iCitySizeBoost = iBoost;

		setLayoutDirty(true);
	}
}


bool CvCity::isNeverLost() const
{
	return m_bNeverLost;
}


void CvCity::setNeverLost(bool bNewValue)
{
	m_bNeverLost = bNewValue;
}


bool CvCity::isBombarded() const
{
	return m_bBombarded;
}


void CvCity::setBombarded(bool bNewValue)
{
	m_bBombarded = bNewValue;
}


bool CvCity::isDrafted() const
{
	return m_bDrafted;
}


void CvCity::setDrafted(bool bNewValue)
{
	m_bDrafted = bNewValue;
}


bool CvCity::isAirliftTargeted() const
{
	return m_bAirliftTargeted;
}


void CvCity::setAirliftTargeted(bool bNewValue)
{
	m_bAirliftTargeted = bNewValue;
}


bool CvCity::isPlundered() const
{
	return m_bPlundered;
}


void CvCity::setPlundered(bool bNewValue)
{
	if (bNewValue != isPlundered())
	{
		m_bPlundered = bNewValue;

		updateTradeRoutes();
	}
}


bool CvCity::isWeLoveTheKingDay() const
{
	return m_bWeLoveTheKingDay;
}


void CvCity::setWeLoveTheKingDay(bool bNewValue)
{
	if(isWeLoveTheKingDay() == bNewValue)
		return;

	m_bWeLoveTheKingDay = bNewValue;

	updateMaintenance();

	CivicTypes eCivic = NO_CIVIC;

	for (int iI = 0; iI < GC.getNumCivicInfos(); iI++)
	{
		if (GET_PLAYER(getOwnerINLINE()).isCivic((CivicTypes)iI))
		{
			if (!CvWString(GC.getCivicInfo((CivicTypes)iI).getWeLoveTheKing()).empty())
			{
				eCivic = ((CivicTypes)iI);
				break;
			}
		}
	}

	if (eCivic != NO_CIVIC)
	{
		CvWString szBuffer = gDLL->getText("TXT_KEY_CITY_CELEBRATE", getNameKey(), GC.getCivicInfo(eCivic).getWeLoveTheKing());
		gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_WELOVEKING", MESSAGE_TYPE_MINOR_EVENT, ARTFILEMGR.getInterfaceArtInfo("INTERFACE_HAPPY_PERSON")->getPath(), (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"), getX_INLINE(), getY_INLINE(), true, true);
	}
}


bool CvCity::isCitizensAutomated() const																
{
	return m_bCitizensAutomated;
}


void CvCity::setCitizensAutomated(bool bNewValue)
{
	int iI;

	if (isCitizensAutomated() != bNewValue)
	{
		m_bCitizensAutomated = bNewValue;

		if (isCitizensAutomated())
		{
			AI_assignWorkingPlots();
		}
		else
		{
			for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
			{
				setForceSpecialistCount(((SpecialistTypes)iI), 0);
			}
		}

		if ((getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) && isCitySelected())
		{
			gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
		}
	}
}


bool CvCity::isProductionAutomated() const
{
	return m_bProductionAutomated;
}


void CvCity::setProductionAutomated(bool bNewValue, bool bClear)
{
	if (isProductionAutomated() != bNewValue)
	{
		m_bProductionAutomated = bNewValue;

		if ((getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) && isCitySelected())
		{
			gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);

			// if automated and not network game and all 3 modifiers down, clear the queue and choose again
			if (bNewValue && bClear)
			{
				clearOrderQueue();
			}
		}

		if (!isProduction())
		{
			AI_chooseProduction();
		}
	}
}


bool CvCity::isWallOverride() const
{
	return m_bWallOverride;
}


void CvCity::setWallOverride(bool bOverride)
{
	if (isWallOverride() != bOverride)
	{
		m_bWallOverride = bOverride;

		setLayoutDirty(true);
	}
}


bool CvCity::isInfoDirty() const
{
	return m_bInfoDirty;
}


void CvCity::setInfoDirty(bool bNewValue)
{
	m_bInfoDirty = bNewValue;
}


bool CvCity::isLayoutDirty() const
{
	return m_bLayoutDirty;
}


void CvCity::setLayoutDirty(bool bNewValue)
{
	m_bLayoutDirty = bNewValue;
}

// <advc.003f>
PlayerTypes CvCity::getOwnerExternal() const
{
	return getOwnerINLINE();
} // </advc.003f>


PlayerTypes CvCity::getPreviousOwner() const
{
	return m_ePreviousOwner;
}


void CvCity::setPreviousOwner(PlayerTypes eNewValue)
{
	m_ePreviousOwner = eNewValue;
}


PlayerTypes CvCity::getOriginalOwner() const
{
	return m_eOriginalOwner;
}


void CvCity::setOriginalOwner(PlayerTypes eNewValue)
{
	m_eOriginalOwner = eNewValue;
}


TeamTypes CvCity::getTeam() const
{
	return TEAMID(getOwnerINLINE());
}


CultureLevelTypes CvCity::getCultureLevel() const
{
	return m_eCultureLevel;
}


int CvCity::getCultureThreshold() const
{
	return getCultureThreshold(getCultureLevel());
}

int CvCity::getCultureThreshold(CultureLevelTypes eLevel)
{
	if (eLevel == NO_CULTURELEVEL)
	{
		return 1;
	}

	return std::max(1, GC.getGameINLINE().getCultureThreshold((CultureLevelTypes)(std::min((eLevel + 1), (GC.getNumCultureLevelInfos() - 1)))));
}


void CvCity::setCultureLevel(CultureLevelTypes eNewValue, bool bUpdatePlotGroups)
{
	CultureLevelTypes eOldValue = getCultureLevel();

	if (eOldValue == eNewValue)
		return;

	m_eCultureLevel = eNewValue;
	if (eOldValue != NO_CULTURELEVEL)
	{
		for (int iDX = -eOldValue; iDX <= eOldValue; iDX++)
		{
			for (int iDY = -eOldValue; iDY <= eOldValue; iDY++)
			{
				int iCultureRange = cultureDistance(iDX, iDY);
				if (iCultureRange > getCultureLevel())
				{
					if (iCultureRange <= eOldValue)
					{
						FAssert(iCultureRange <= GC.getNumCultureLevelInfos());

						CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);
						if (pLoopPlot != NULL)
						{
							pLoopPlot->changeCultureRangeCities(getOwnerINLINE(), iCultureRange, -1, bUpdatePlotGroups);
						}
					}
				}
			}
		}
	}

	if (getCultureLevel() != NO_CULTURELEVEL)
	{
		for (int iDX = -getCultureLevel(); iDX <= getCultureLevel(); iDX++)
		{
			for (int iDY = -getCultureLevel(); iDY <= getCultureLevel(); iDY++)
			{
				int iCultureRange = cultureDistance(iDX, iDY);
				if (iCultureRange > eOldValue)
				{
					if (iCultureRange <= getCultureLevel())
					{
						FAssert(iCultureRange <= GC.getNumCultureLevelInfos());

						CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);

						if (pLoopPlot != NULL)
						{
							pLoopPlot->changeCultureRangeCities(getOwnerINLINE(), iCultureRange, 1, bUpdatePlotGroups);
						}
					}
				}
			}
		}
	}
	if (GC.getGameINLINE().isFinalInitialized() && getCultureLevel() > eOldValue && getCultureLevel() > 1)
	{ // advc.003: Some style changes in this block
		CvWString szBuffer(gDLL->getText("TXT_KEY_MISC_BORDERS_EXPANDED", getNameKey()));
		gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), false,
				GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_CULTUREEXPANDS",
				MESSAGE_TYPE_MINOR_EVENT, GC.getCommerceInfo(COMMERCE_CULTURE).
				getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"),
				getX_INLINE(), getY_INLINE(), true, true);
		// <advc.106>
		// To replace hardcoded getCultureLevel()==GC.getNumCultureLevelInfos()-1
		int iVictoryCultureLevel = NO_CULTURELEVEL;
		for(int i = 0; i < GC.getNumVictoryInfos(); i++)
		{
			if(!GC.getGameINLINE().isVictoryValid((VictoryTypes)i))
				continue;
			int iLevel = GC.getVictoryInfo((VictoryTypes)i).getCityCulture();
			if(iLevel > 0 && (iVictoryCultureLevel <= 0 || iVictoryCultureLevel > iLevel))
				iVictoryCultureLevel = iLevel;
		}
		if (getCultureLevel() == iVictoryCultureLevel && iVictoryCultureLevel > 0)
		{	// Cut from below. Use this for the replay message as well.
			CvWString szMsg(gDLL->getText("TXT_KEY_MISC_CULTURE_LEVEL", getNameKey(),
					GC.getCultureLevelInfo(getCultureLevel()).getTextKeyWide()));
			// </advc.106>
			for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
			{
				CvPlayer const& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
				if (!kLoopPlayer.isAlive())
					continue;

				if (isRevealed(kLoopPlayer.getTeam(), false)
						|| kLoopPlayer.isSpectator()) // advc.127
				{
					gDLL->getInterfaceIFace()->addHumanMessage(
							kLoopPlayer.getID(), false, GC.getEVENT_MESSAGE_TIME(),
							szMsg, "AS2D_CULTURELEVEL", MESSAGE_TYPE_MAJOR_EVENT,
							GC.getCommerceInfo(COMMERCE_CULTURE).getButton(),
							(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
							getX_INLINE(), getY_INLINE(), true, true);
				}
				else
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_CULTURE_LEVEL_UNKNOWN",
							GC.getCultureLevelInfo(getCultureLevel()).getTextKeyWide());
					gDLL->getInterfaceIFace()->addHumanMessage(kLoopPlayer.getID(),
							false, GC.getEVENT_MESSAGE_TIME(), szBuffer,
							"AS2D_CULTURELEVEL", MESSAGE_TYPE_MAJOR_EVENT,
							GC.getCommerceInfo(COMMERCE_CULTURE).getButton(),
							(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
				}
			} // <advc.106>
			GC.getGameINLINE().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT,
					getOwnerINLINE(), szMsg, getX_INLINE(), getY_INLINE(),
					(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
			// </advc.106>
		}
		// ONEVENT - Culture growth
		CvEventReporter::getInstance().cultureExpansion(this, getOwnerINLINE());	
		//Stop Build Culture
		/* original BTS code
		if (isProductionProcess()) {
			if (GC.getProcessInfo(getProductionProcess()).getProductionToCommerceModifier(COMMERCE_CULTURE) > 0)
					popOrder(0, false, true);						
		} */ /* K-Mod does this in a different way, to avoid an overflow bug.
			(And a different way to the Unofficial Patch, to avoid OOS) */
	}
}


void CvCity::updateCultureLevel(bool bUpdatePlotGroups)
{
	if (getCultureUpdateTimer() > 0)
		return;

	CultureLevelTypes eCultureLevel = (CultureLevelTypes)0;

	if (!isOccupation()) {
		// advc.130f: Moved into subroutine
		eCultureLevel = calculateCultureLevel(getOwnerINLINE());
	}

	setCultureLevel(eCultureLevel, bUpdatePlotGroups);
}

// <advc.130f> Cut from updateCultureLevel
CultureLevelTypes CvCity::calculateCultureLevel(PlayerTypes ePlayer) const {

	CvGame const& g = GC.getGameINLINE();
	int const iCultureTimes100 = getCultureTimes100(ePlayer);
	for(int i = GC.getNumCultureLevelInfos() - 1; i > 0; i--) {
		CultureLevelTypes eLoopLevel = (CultureLevelTypes)i;
		if(iCultureTimes100 >= 100 * g.getCultureThreshold(eLoopLevel))
			return eLoopLevel;
	}
	return (CultureLevelTypes)0;
} // advc.130f>

// <advc.042> Mostly cut and pasted from CvDLLWidgetData::parseCultureHelp
int CvCity::getCultureTurnsLeft() const {

	int iCultureRateTimes100 = getCommerceRateTimes100(COMMERCE_CULTURE);
	if(iCultureRateTimes100 <= 0)
		return -1;
	int iCultureLeftTimes100 = 100 * getCultureThreshold() -
			getCultureTimes100(getOwnerINLINE());
	if(iCultureLeftTimes100 <= 0)
		return -1;
	int r = (iCultureLeftTimes100  + iCultureRateTimes100 - 1) / iCultureRateTimes100;
	FAssert(r != 0);
	return r;
} // </advc.042>


int CvCity::getSeaPlotYield(YieldTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiSeaPlotYield[eIndex];
}


void CvCity::changeSeaPlotYield(YieldTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiSeaPlotYield[eIndex] = (m_aiSeaPlotYield[eIndex] + iChange);
		FAssert(getSeaPlotYield(eIndex) >= 0);

		updateYield();
	}
}


int CvCity::getRiverPlotYield(YieldTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiRiverPlotYield[eIndex];
}


void CvCity::changeRiverPlotYield(YieldTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiRiverPlotYield[eIndex] += iChange;
		FAssert(getRiverPlotYield(eIndex) >= 0);

		updateYield();
	}
}

// BUG - Building Additional Yield - start
/*
 * Returns the total additional yield that adding one of the given buildings will provide.
 *
 * Doesn't check if the building can be constructed in this city.
 */
int CvCity::getAdditionalYieldByBuilding(YieldTypes eIndex, BuildingTypes eBuilding) const
{
	int iRate = getBaseYieldRate(eIndex);
	int iModifier = getBaseYieldRateModifier(eIndex);
	int iExtra = ((iRate + getAdditionalBaseYieldRateByBuilding(eIndex, eBuilding)) * (iModifier + getAdditionalYieldRateModifierByBuilding(eIndex, eBuilding)) / 100) - (iRate * iModifier / 100);

	return iExtra;
}

/*
 * Returns the additional yield rate that adding one of the given buildings will provide.
 *
 * Doesn't check if the building can be constructed in this city.
 */
int CvCity::getAdditionalBaseYieldRateByBuilding(YieldTypes eIndex, BuildingTypes eBuilding) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	FAssertMsg(eBuilding >= 0, "eBuilding expected to be >= 0");
	FAssertMsg(eBuilding < GC.getNumBuildingInfos(), "eBuilding expected to be < GC.getNumBuildingInfos()");

	CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	bool bObsolete = GET_TEAM(getTeam()).isObsoleteBuilding(eBuilding);
	if(bObsolete)
		return 0;
	int iExtraRate = 0;
	// <advc.179> Some overlap with code in CvGameTextMgr::buildBuildingReligionYieldString
	for(int i = 0; i < GC.getNumVoteSourceInfos(); i++) {
		VoteSourceTypes eVS = (VoteSourceTypes)i;
		if(eVS != kBuilding.getVoteSourceType())
			continue;
		CvVoteSourceInfo& kVS = GC.getVoteSourceInfo(eVS);
		if(kVS.getReligionYield(eIndex) != 0) {
			ReligionTypes eVSReligion =
					// This would be NO_RELIGION b/c the vote source doesn't exist yet:
					//GC.getGameINLINE().getVoteSourceReligion(eVS);
					GET_PLAYER(getOwnerINLINE()).getStateReligion();
			if(eVSReligion != NO_RELIGION) {
				// Based on processVoteSourceBonus
				for(int j = 0; j < GC.getNumBuildingInfos(); j++) {
					BuildingTypes eBuilding = (BuildingTypes)j;
					if(getNumBuilding(eBuilding) <= 0 || GET_TEAM(getTeam()).
							isObsoleteBuilding(eBuilding))
						continue;
					if(GC.getBuildingInfo(eBuilding).getReligionType() == eVSReligion)
						iExtraRate += kVS.getReligionYield(eIndex);
				}
			}
		}
	} // </advc.179>
	if (kBuilding.getSeaPlotYieldChange(eIndex) != 0)
	{
		int iChange = kBuilding.getSeaPlotYieldChange(eIndex);
		for (int iI = 0; iI < NUM_CITY_PLOTS; ++iI)
		{
			if (isWorkingPlot(iI) && getCityIndexPlot(iI)->isWater())
			{
				iExtraRate += iChange;
			}
		}
	}
	if (kBuilding.getRiverPlotYieldChange(eIndex) != 0)
	{
		int iChange = kBuilding.getRiverPlotYieldChange(eIndex);
		for (int iI = 0; iI < NUM_CITY_PLOTS; ++iI)
		{
			if (isWorkingPlot(iI) && getCityIndexPlot(iI)->isRiver())
			{
				iExtraRate += iChange;
			}
		}
	}
	iExtraRate += kBuilding.getYieldChange(eIndex);
	iExtraRate += getBuildingYieldChange((BuildingClassTypes)kBuilding.getBuildingClassType(), eIndex);

	// Trade
	int iPlayerTradeYieldModifier = GET_PLAYER(getOwnerINLINE()).getTradeYieldModifier(eIndex);
	if (iPlayerTradeYieldModifier > 0 && (kBuilding.getTradeRouteModifier() != 0 || kBuilding.getForeignTradeRouteModifier() != 0))
	{
		int iTotalTradeYield = 0;
		int iNewTotalTradeYield = 0;
		// BUG - Fractional Trade Routes - start
/*  advc (caveat): _MOD_FRACTRADE has never been tested in AdvCiv and the
	BULL - Trade Hover (CvCity::calculateTradeTotals) has been merged w/o support
	for _MOD_FRACTRADE. */
#ifdef _MOD_FRACTRADE
		int iTradeProfitDivisor = 100;
#else
		int iTradeProfitDivisor = 10000;
#endif
		// BUG - Fractional Trade Routes - end

		for (int iI = 0; iI < getTradeRoutes(); ++iI)
		{
			CvCity* pCity = getTradeCity(iI);
			if (pCity)
			{
				int iTradeProfit = getBaseTradeProfit(pCity);
				int iTradeModifier = totalTradeModifier(pCity);
				int iTradeYield = iTradeProfit * iTradeModifier / iTradeProfitDivisor * iPlayerTradeYieldModifier / 100;
				iTotalTradeYield += iTradeYield;

				iTradeModifier += kBuilding.getTradeRouteModifier();
				if (pCity->getOwnerINLINE() != getOwnerINLINE())
				{
					iTradeModifier += kBuilding.getForeignTradeRouteModifier();
				}
				int iNewTradeYield = iTradeProfit * iTradeModifier / iTradeProfitDivisor * iPlayerTradeYieldModifier / 100;
				iNewTotalTradeYield += iNewTradeYield;
			}
		}

		// BUG - Fractional Trade Routes - start
#ifdef _MOD_FRACTRADE
		iTotalTradeYield /= 100;
		iNewTotalTradeYield /= 100;
#endif
		// BUG - Fractional Trade Routes - end
		iExtraRate += iNewTotalTradeYield - iTotalTradeYield;
	}

	// Specialists
	for (int iI = 0; iI < GC.getNumSpecialistInfos(); ++iI)
	{
		if (kBuilding.getFreeSpecialistCount((SpecialistTypes)iI) != 0)
		{
			iExtraRate += getAdditionalBaseYieldRateBySpecialist(eIndex, (SpecialistTypes)iI, kBuilding.getFreeSpecialistCount((SpecialistTypes)iI));
		}
	}

	return iExtraRate;
}

/*
 * Returns the additional yield rate modifier that adding one of the given buildings will provide.
 *
 * Doesn't check if the building can be constructed in this city.
 */
int CvCity::getAdditionalYieldRateModifierByBuilding(YieldTypes eIndex, BuildingTypes eBuilding) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	FAssertMsg(eBuilding >= 0, "eBuilding expected to be >= 0");
	FAssertMsg(eBuilding < GC.getNumBuildingInfos(), "eBuilding expected to be < GC.getNumBuildingInfos()");

	CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	bool bObsolete = GET_TEAM(getTeam()).isObsoleteBuilding(eBuilding);
	int iExtraModifier = 0;

	if (!bObsolete)
	{
		iExtraModifier += kBuilding.getYieldModifier(eIndex);
		if (isPower())
		{
			iExtraModifier += kBuilding.getPowerYieldModifier(eIndex);
		}
		else
		{
			if (kBuilding.isPower() || kBuilding.isAreaCleanPower() || (kBuilding.getPowerBonus() != NO_BONUS && hasBonus((BonusTypes)kBuilding.getPowerBonus())))
			{
				for (int i = 0; i < GC.getNumBuildingInfos(); i++)
				{
					iExtraModifier += getNumActiveBuilding((BuildingTypes)i) * GC.getBuildingInfo((BuildingTypes)i).getPowerYieldModifier(eIndex);
				}
			}
		}
		if (eIndex == YIELD_PRODUCTION)
		{
			iExtraModifier += kBuilding.getMilitaryProductionModifier();
			iExtraModifier += kBuilding.getSpaceProductionModifier();
			iExtraModifier += kBuilding.getGlobalSpaceProductionModifier();

			int iMaxModifier = 0;
			for (int i = 0; i < NUM_DOMAIN_TYPES; i++)
			{
				iMaxModifier = std::max(iMaxModifier, kBuilding.getDomainProductionModifier((DomainTypes)i));
			}
			iExtraModifier += iMaxModifier;
		}
		for (int iI = 0; iI < GC.getNumBonusInfos(); ++iI)
		{
			if (hasBonus((BonusTypes)iI))
			{
				iExtraModifier += kBuilding.getBonusYieldModifier(iI, eIndex);
			}
		}
	}

	return iExtraModifier;
}
// BUG - Building Additional Yield - end

// BUG - Specialist Additional Yield - start
/*
 * Returns the total additional yield that changing the number of given specialists will provide/remove.
 */
int CvCity::getAdditionalYieldBySpecialist(YieldTypes eIndex, SpecialistTypes eSpecialist, int iChange) const
{
	int iRate = getBaseYieldRate(eIndex);
	int iModifier = getBaseYieldRateModifier(eIndex);
	int iExtra = ((iRate + getAdditionalBaseYieldRateBySpecialist(eIndex, eSpecialist, iChange)) * iModifier / 100) - (iRate * iModifier / 100);

	return iExtra;
}

/*
 * Returns the additional yield rate that changing the number of given specialists will provide/remove.
 */
int CvCity::getAdditionalBaseYieldRateBySpecialist(YieldTypes eIndex, SpecialistTypes eSpecialist, int iChange) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	FAssertMsg(eSpecialist >= 0, "eSpecialist expected to be >= 0");
	FAssertMsg(eSpecialist < GC.getNumSpecialistInfos(), "eSpecialist expected to be < GC.getNumSpecialistInfos()");
	
	CvSpecialistInfo& kSpecialist = GC.getSpecialistInfo(eSpecialist);
	return iChange * (kSpecialist.getYieldChange(eIndex) + GET_PLAYER(getOwnerINLINE()).getSpecialistExtraYield(eSpecialist, eIndex));
}
// BUG - Specialist Additional Yield - end

int CvCity::getBaseYieldRate(YieldTypes eIndex)	const													
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiBaseYieldRate[eIndex];
}


int CvCity::getBaseYieldRateModifier(YieldTypes eIndex, int iExtra) const
{
	int iModifier = getYieldRateModifier(eIndex);

	iModifier += getBonusYieldRateModifier(eIndex);

	if (isPower())
	{
		iModifier += getPowerYieldRateModifier(eIndex);
	}

	if (area() != NULL)
	{
		iModifier += area()->getYieldRateModifier(getOwnerINLINE(), eIndex);
	}

	iModifier += GET_PLAYER(getOwnerINLINE()).getYieldRateModifier(eIndex);

	if (isCapital())
	{
		iModifier += GET_PLAYER(getOwnerINLINE()).getCapitalYieldRateModifier(eIndex);
	}

	iModifier += iExtra;

	// note: player->invalidateYieldRankCache() must be called for anything that is checked here
	// so if any extra checked things are added here, the cache needs to be invalidated

	return std::max(0, (iModifier + 100));
}


int CvCity::getYieldRate(YieldTypes eIndex) const
{
	return ((getBaseYieldRate(eIndex) * getBaseYieldRateModifier(eIndex)) / 100);
}


void CvCity::setBaseYieldRate(YieldTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (getBaseYieldRate(eIndex) != iNewValue)
	{
		FAssertMsg(iNewValue >= 0, "iNewValue expected to be >= 0");
		FAssertMsg(((iNewValue * 100) / 100) >= 0, "((iNewValue * 100) / 100) expected to be >= 0");

		m_aiBaseYieldRate[eIndex] = iNewValue;
		FAssert(getYieldRate(eIndex) >= 0);

		updateCommerce();

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			setInfoDirty(true);

			if (isCitySelected())
			{
				gDLL->getInterfaceIFace()->setDirty(CityScreen_DIRTY_BIT, true);
				gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true );
			}
		}
	}
}


void CvCity::changeBaseYieldRate(YieldTypes eIndex, int iChange)										
{
	setBaseYieldRate(eIndex, (getBaseYieldRate(eIndex) + iChange));
}


int CvCity::getYieldRateModifier(YieldTypes eIndex)	const														
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiYieldRateModifier[eIndex];
}


void CvCity::changeYieldRateModifier(YieldTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiYieldRateModifier[eIndex] = (m_aiYieldRateModifier[eIndex] + iChange);
		FAssert(getYieldRate(eIndex) >= 0);

		GET_PLAYER(getOwnerINLINE()).invalidateYieldRankCache(eIndex);

		if (eIndex == YIELD_COMMERCE)
		{
			updateCommerce();
		}

		AI_setAssignWorkDirty(true);

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			setInfoDirty(true);
		}
	}
}


int CvCity::getPowerYieldRateModifier(YieldTypes eIndex) const												
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiPowerYieldRateModifier[eIndex];
}


void CvCity::changePowerYieldRateModifier(YieldTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiPowerYieldRateModifier[eIndex] = (m_aiPowerYieldRateModifier[eIndex] + iChange);
		FAssert(getYieldRate(eIndex) >= 0);

		GET_PLAYER(getOwnerINLINE()).invalidateYieldRankCache(eIndex);

		if (eIndex == YIELD_COMMERCE)
		{
			updateCommerce();
		}

		AI_setAssignWorkDirty(true);

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			setInfoDirty(true);
		}
	}
}


int CvCity::getBonusYieldRateModifier(YieldTypes eIndex) const												
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiBonusYieldRateModifier[eIndex];
}


void CvCity::changeBonusYieldRateModifier(YieldTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	if (iChange != 0)
	{
		m_aiBonusYieldRateModifier[eIndex] = (m_aiBonusYieldRateModifier[eIndex] + iChange);
		FAssert(getYieldRate(eIndex) >= 0);

		GET_PLAYER(getOwnerINLINE()).invalidateYieldRankCache(eIndex);

		if (eIndex == YIELD_COMMERCE)
		{
			updateCommerce();
		}

		AI_setAssignWorkDirty(true);

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			setInfoDirty(true);
		}
	}
}


int CvCity::getTradeYield(YieldTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiTradeYield[eIndex];
}


int CvCity::totalTradeModifier(CvCity* pOtherCity) const
{
	int iModifier = 100;

	iModifier += getTradeRouteModifier();

	iModifier += getPopulationTradeModifier();

	if (isConnectedToCapital())
	{
		iModifier += GC.getDefineINT("CAPITAL_TRADE_MODIFIER");
	}

	if (NULL != pOtherCity)
	{
		if (area() != pOtherCity->area())
		{
			iModifier += GC.getDefineINT("OVERSEAS_TRADE_MODIFIER");
		}

		if (getTeam() != pOtherCity->getTeam())
		{
			iModifier += getForeignTradeRouteModifier();

			iModifier += getPeaceTradeModifier(pOtherCity->getTeam());
		}
	}

	return iModifier;
}

int CvCity::getPopulationTradeModifier() const
{
	return std::max(0, (getPopulation() + GC.getDefineINT("OUR_POPULATION_TRADE_MODIFIER_OFFSET")) * GC.getDefineINT("OUR_POPULATION_TRADE_MODIFIER"));
}

int CvCity::getPeaceTradeModifier(TeamTypes eTeam) const
{
	FAssert(NO_TEAM != eTeam);
	FAssert(eTeam != getTeam());

	if (atWar(eTeam, getTeam()))
	{
		return 0;
	}

	int iPeaceTurns = std::min(GC.getDefineINT("FOREIGN_TRADE_FULL_CREDIT_PEACE_TURNS"), GET_TEAM(getTeam()).AI_getAtPeaceCounter(eTeam));

	if (GC.getGameINLINE().getElapsedGameTurns() <= iPeaceTurns)
	{
		return GC.getDefineINT("FOREIGN_TRADE_MODIFIER");
	}

	return ((GC.getDefineINT("FOREIGN_TRADE_MODIFIER") * iPeaceTurns) / std::max(1, GC.getDefineINT("FOREIGN_TRADE_FULL_CREDIT_PEACE_TURNS")));
}

int CvCity::getBaseTradeProfit(CvCity* pCity) const
{
	int iProfit = std::min(pCity->getPopulation() * GC.getDefineINT("THEIR_POPULATION_TRADE_PERCENT"), plotDistance(getX_INLINE(), getY_INLINE(), pCity->getX_INLINE(), pCity->getY_INLINE()) * GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTradeProfitPercent());

	iProfit *= GC.getDefineINT("TRADE_PROFIT_PERCENT");
	iProfit /= 100;

	iProfit = std::max(100, iProfit);

	return iProfit;
}

int CvCity::calculateTradeProfit(CvCity* pCity) const
{
	int iProfit = getBaseTradeProfit(pCity);

	iProfit *= totalTradeModifier(pCity);
	iProfit /= 10000;

	return iProfit;
}


int CvCity::calculateTradeYield(YieldTypes eIndex, int iTradeProfit) const
{
	if ((iTradeProfit > 0) && (GET_PLAYER(getOwnerINLINE()).getTradeYieldModifier(eIndex) > 0))
	{
		return ((iTradeProfit * GET_PLAYER(getOwnerINLINE()).getTradeYieldModifier(eIndex)) / 100);
	}
	else
	{
		return 0;
	}
}

// BULL - Trade Hover - start  (advc: simplified a bit, _MOD_FRACTRADE removed)
/*  Adds the yield and count for each trade route with eWithPlayer to the
	int references (out parameters). */
void CvCity::calculateTradeTotals(YieldTypes eIndex, int& iDomesticYield, int& iDomesticRoutes,
		int& iForeignYield, int& iForeignRoutes, PlayerTypes eWithPlayer) const {

	if(isDisorder())
		return;

	int iNumTradeRoutes = getTradeRoutes();
	for(int iI = 0; iI < iNumTradeRoutes; iI++) {
		CvCity* pTradeCity = getTradeCity(iI);
		if(pTradeCity != NULL && (eWithPlayer == NO_PLAYER ||
				pTradeCity->getOwnerINLINE() == eWithPlayer)) {
			int iTradeYield = getBaseTradeProfit(pTradeCity);
			iTradeYield = calculateTradeYield(YIELD_COMMERCE,
					calculateTradeProfit(pTradeCity));
			if(pTradeCity->getOwnerINLINE() == getOwnerINLINE()) {
				iDomesticYield += iTradeYield;
				iDomesticRoutes++;
			}
			else {
				iForeignYield += iTradeYield;
				iForeignRoutes++;
			}
		}
	}
} // BULL - Trade Hover - end

void CvCity::setTradeYield(YieldTypes eIndex, int iNewValue)
{
	int iOldValue;

	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	iOldValue = getTradeYield(eIndex);

	if (iOldValue != iNewValue)
	{
		m_aiTradeYield[eIndex] = iNewValue;
		FAssert(getTradeYield(eIndex) >= 0);

		changeBaseYieldRate(eIndex, (iNewValue - iOldValue));
	}
}


int CvCity::getExtraSpecialistYield(YieldTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiExtraSpecialistYield[eIndex];
}


int CvCity::getExtraSpecialistYield(YieldTypes eIndex, SpecialistTypes eSpecialist) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	FAssertMsg(eSpecialist >= 0, "eSpecialist expected to be >= 0");
	FAssertMsg(eSpecialist < GC.getNumSpecialistInfos(), "GC.getNumSpecialistInfos expected to be >= 0");
	return ((getSpecialistCount(eSpecialist) + getFreeSpecialistCount(eSpecialist)) * GET_PLAYER(getOwnerINLINE()).getSpecialistExtraYield(eSpecialist, eIndex));
}


void CvCity::updateExtraSpecialistYield(YieldTypes eYield)
{
	FAssertMsg(eYield >= 0, "eYield expected to be >= 0");
	FAssertMsg(eYield < NUM_YIELD_TYPES, "eYield expected to be < NUM_YIELD_TYPES");

	int iOldYield = getExtraSpecialistYield(eYield);

	int iNewYield = 0;
	for (int iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
		iNewYield += getExtraSpecialistYield(eYield, (SpecialistTypes)iI);

	if (iOldYield != iNewYield)
	{
		m_aiExtraSpecialistYield[eYield] = iNewYield;
		FAssert(getExtraSpecialistYield(eYield) >= 0);

		changeBaseYieldRate(eYield, iNewYield - iOldYield);
	}
}


void CvCity::updateExtraSpecialistYield()
{
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
		updateExtraSpecialistYield((YieldTypes)iI);
}


int CvCity::getCommerceRate(CommerceTypes eIndex) const
{
	return getCommerceRateTimes100(eIndex) / 100;
}

int CvCity::getCommerceRateTimes100(CommerceTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");

	int iRate = m_aiCommerceRate[eIndex];

	if (GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		if (eIndex == COMMERCE_CULTURE)
		{
			iRate += m_aiCommerceRate[COMMERCE_ESPIONAGE];
		}
		else if (eIndex == COMMERCE_ESPIONAGE)
		{
			iRate = 0;
		}
	}

	return iRate;
}


int CvCity::getCommerceFromPercent(CommerceTypes eIndex, int iYieldRate) const
{
	int iCommerce;

	iCommerce = ((iYieldRate * GET_PLAYER(getOwnerINLINE()).getCommercePercent(eIndex)) / 100);

	if (eIndex == COMMERCE_GOLD)
	{
		iCommerce += (iYieldRate - iCommerce - getCommerceFromPercent(COMMERCE_RESEARCH, iYieldRate) - getCommerceFromPercent(COMMERCE_CULTURE, iYieldRate) - getCommerceFromPercent(COMMERCE_ESPIONAGE, iYieldRate));
	}

	return iCommerce;
}


int CvCity::getBaseCommerceRate(CommerceTypes eIndex) const
{
	return (getBaseCommerceRateTimes100(eIndex) / 100);
}

int CvCity::getBaseCommerceRateTimes100(CommerceTypes eIndex) const
{
	int iBaseCommerceRate;

	iBaseCommerceRate = getCommerceFromPercent(eIndex, getYieldRate(YIELD_COMMERCE) * 100);

	iBaseCommerceRate += 100 * ((getSpecialistPopulation() + getNumGreatPeople()) *
			GET_PLAYER(getOwnerINLINE()).getSpecialistExtraCommerce(eIndex));
	iBaseCommerceRate += 100 * (getBuildingCommerce(eIndex) + getSpecialistCommerce(eIndex) +
			getReligionCommerce(eIndex) + getCorporationCommerce(eIndex) +
			GET_PLAYER(getOwnerINLINE()).getFreeCityCommerce(eIndex));
	return iBaseCommerceRate;
}


int CvCity::getTotalCommerceRateModifier(CommerceTypes eIndex) const
{
	CvPlayer const& kOwner = GET_PLAYER(getOwnerINLINE());
	return std::max(0, getCommerceRateModifier(eIndex) +
			kOwner.getCommerceRateModifier(eIndex) +
			(isCapital() ? kOwner.getCapitalCommerceRateModifier(eIndex) : 0) + 100);
}


void CvCity::updateCommerce(CommerceTypes eIndex)
{
	int iOldCommerce;
	int iNewCommerce;

	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");

	iOldCommerce = m_aiCommerceRate[eIndex];

	if (isDisorder())
	{
		iNewCommerce = 0;
	}
	else
	{
		iNewCommerce = (getBaseCommerceRateTimes100(eIndex) * getTotalCommerceRateModifier(eIndex)) / 100;
		iNewCommerce += getYieldRate(YIELD_PRODUCTION) * getProductionToCommerceModifier(eIndex);
	}

	if (iOldCommerce != iNewCommerce)
	{
		m_aiCommerceRate[eIndex] = iNewCommerce;
		FAssert(m_aiCommerceRate[eIndex] >= 0);

		GET_PLAYER(getOwnerINLINE()).invalidateCommerceRankCache(eIndex);

		GET_PLAYER(getOwnerINLINE()).changeCommerceRate(eIndex, (iNewCommerce - iOldCommerce));

		if (isCitySelected())
		{
			gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true );
			gDLL->getInterfaceIFace()->setDirty(CityScreen_DIRTY_BIT, true);
		}
	}
}


void CvCity::updateCommerce()
{
	GET_PLAYER(getOwnerINLINE()).invalidateYieldRankCache();

	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		updateCommerce((CommerceTypes)iI);
	}
}


int CvCity::getProductionToCommerceModifier(CommerceTypes eIndex) const										 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");
	return m_aiProductionToCommerceModifier[eIndex];
}


void CvCity::changeProductionToCommerceModifier(CommerceTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");

	if (iChange != 0)
	{
		m_aiProductionToCommerceModifier[eIndex] = (m_aiProductionToCommerceModifier[eIndex] + iChange);

		updateCommerce(eIndex);
	}
}


int CvCity::getBuildingCommerce(CommerceTypes eIndex) const												 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");
	return m_aiBuildingCommerce[eIndex];
}


int CvCity::getBuildingCommerceByBuilding(CommerceTypes eIndex, BuildingTypes eBuilding) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");
	FAssertMsg(eBuilding >= 0, "eBuilding expected to be >= 0");
	FAssertMsg(eBuilding < GC.getNumBuildingInfos(), "GC.getNumBuildingInfos expected to be >= 0");

	// K-Mod. I've rearranged some stuff so that bonus commerce does not get doubled at the end.
	// (eg. the bonus culture that the Sistine Chapel gives to religious buildings should not be doubled.)

	if (getNumBuilding(eBuilding) > 0)
	{
		const CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);

		int iTimeFactor =
			kBuilding.getCommerceChangeDoubleTime(eIndex) != 0 &&
			getBuildingOriginalTime(eBuilding) != MIN_INT &&
			GC.getGameINLINE().getGameTurnYear() - getBuildingOriginalTime(eBuilding) >= kBuilding.getCommerceChangeDoubleTime(eIndex)
			? 2 : 1;
		// there are just two components which get multiplied by the time factor: the standard commerce, and the "safe" commerce.
		// the rest of the components are bonuses which should not be doubled.

		if (!kBuilding.isCommerceChangeOriginalOwner(eIndex) || getBuildingOriginalOwner(eBuilding) == getOwnerINLINE())
		{
			int iCommerce = kBuilding.getObsoleteSafeCommerceChange(eIndex) * getNumBuilding(eBuilding) * iTimeFactor; // 1

			if (getNumActiveBuilding(eBuilding) > 0)
			{
				iCommerce += GC.getBuildingInfo(eBuilding).getCommerceChange(eIndex) * getNumActiveBuilding(eBuilding) * iTimeFactor; // 2
				iCommerce += getBuildingCommerceChange((BuildingClassTypes)GC.getBuildingInfo(eBuilding).getBuildingClassType(), eIndex) * getNumActiveBuilding(eBuilding);

				if (GC.getBuildingInfo(eBuilding).getReligionType() != NO_RELIGION)
				{
					if (GC.getBuildingInfo(eBuilding).getReligionType() == GET_PLAYER(getOwnerINLINE()).getStateReligion())
					{
						iCommerce += GET_PLAYER(getOwnerINLINE()).getStateReligionBuildingCommerce(eIndex) * getNumActiveBuilding(eBuilding);
					}
				}

				if (GC.getBuildingInfo(eBuilding).getGlobalReligionCommerce() != NO_RELIGION)
				{
					iCommerce += (GC.getReligionInfo((ReligionTypes)(GC.getBuildingInfo(eBuilding).getGlobalReligionCommerce())).getGlobalReligionCommerce(eIndex) * GC.getGameINLINE().countReligionLevels((ReligionTypes)(GC.getBuildingInfo(eBuilding).getGlobalReligionCommerce()))) * getNumActiveBuilding(eBuilding);
				}

				if (GC.getBuildingInfo(eBuilding).getGlobalCorporationCommerce() != NO_CORPORATION)
				{
					iCommerce += (GC.getCorporationInfo((CorporationTypes)(GC.getBuildingInfo(eBuilding).getGlobalCorporationCommerce())).getHeadquarterCommerce(eIndex) * GC.getGameINLINE().countCorporationLevels((CorporationTypes)(GC.getBuildingInfo(eBuilding).getGlobalCorporationCommerce()))) * getNumActiveBuilding(eBuilding);
				}
			}

			return iCommerce;
		}
	}

	return 0;
}


void CvCity::updateBuildingCommerce()
{
	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		int iNewBuildingCommerce = 0;

		for (int iJ = 0; iJ < GC.getNumBuildingInfos(); iJ++)
		{
			iNewBuildingCommerce += getBuildingCommerceByBuilding(((CommerceTypes)iI), ((BuildingTypes)iJ));
		}

		if (getBuildingCommerce((CommerceTypes)iI) != iNewBuildingCommerce)
		{
			m_aiBuildingCommerce[iI] = iNewBuildingCommerce;
			FAssert(getBuildingCommerce((CommerceTypes)iI) >= 0);

			updateCommerce((CommerceTypes)iI);
		}
	}
}

// BUG - Building Additional Commerce - start
/*
 * Returns the rounded total additional commerce that adding one of the given buildings will provide.
 *
 * Doesn't check if the building can be constructed in this city.
 * Takes the NO_ESPIONAGE game option into account for CULTURE and ESPIONAGE.
 */
int CvCity::getAdditionalCommerceByBuilding(CommerceTypes eIndex, BuildingTypes eBuilding) const
{
	return getAdditionalCommerceTimes100ByBuilding(eIndex, eBuilding) / 100;
}

/*
 * Returns the total additional commerce times 100 that adding one of the given buildings will provide.
 *
 * Doesn't check if the building can be constructed in this city.
 * Takes the NO_ESPIONAGE game option into account for CULTURE and ESPIONAGE.
 */
int CvCity::getAdditionalCommerceTimes100ByBuilding(CommerceTypes eIndex, BuildingTypes eBuilding) const
{
	int iExtraRate = getAdditionalBaseCommerceRateByBuilding(eIndex, eBuilding);
	int iExtraModifier = getAdditionalCommerceRateModifierByBuilding(eIndex, eBuilding);
	if (iExtraRate == 0 && iExtraModifier == 0)
	{
		return 0;
	}

	int iRateTimes100 = getBaseCommerceRateTimes100(eIndex);
	int iModifier = getTotalCommerceRateModifier(eIndex);
	int iExtraTimes100 = ((iModifier + iExtraModifier) * (100 * iExtraRate + iRateTimes100) / 100) - (iModifier * iRateTimes100 / 100);

	return iExtraTimes100;
}

/*
 * Returns the additional base commerce rate constructing the given building will provide.
 *
 * Doesn't check if the building can be constructed in this city.
 * Takes the NO_ESPIONAGE game option into account for CULTURE and ESPIONAGE.
 */
int CvCity::getAdditionalBaseCommerceRateByBuilding(CommerceTypes eIndex, BuildingTypes eBuilding) const
{
	bool bNoEspionage = GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE);
	if (bNoEspionage && eIndex == COMMERCE_ESPIONAGE)
	{
		return 0;
	}

	int iExtraRate = getAdditionalBaseCommerceRateByBuildingImpl(eIndex, eBuilding);
	if (bNoEspionage && eIndex == COMMERCE_CULTURE)
	{
		iExtraRate += getAdditionalBaseCommerceRateByBuildingImpl(COMMERCE_ESPIONAGE, eBuilding);
	}
	return iExtraRate;
}

/*
 * Returns the additional base commerce rate constructing the given building will provide.
 *
 * Doesn't check if the building can be constructed in this city.
 */
int CvCity::getAdditionalBaseCommerceRateByBuildingImpl(CommerceTypes eIndex, BuildingTypes eBuilding) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");
	FAssertMsg(eBuilding >= 0, "eBuilding expected to be >= 0");
	FAssertMsg(eBuilding < GC.getNumBuildingInfos(), "eBuilding expected to be < GC.getNumBuildingInfos()");

	CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	bool bObsolete = GET_TEAM(getTeam()).isObsoleteBuilding(eBuilding);
	int iExtraRate = 0;

	iExtraRate += kBuilding.getObsoleteSafeCommerceChange(eIndex);
	if (!bObsolete)
	{
		iExtraRate += kBuilding.getCommerceChange(eIndex);
		iExtraRate += getBuildingCommerceChange((BuildingClassTypes)kBuilding.getBuildingClassType(), eIndex);
		if (kBuilding.getReligionType() != NO_RELIGION)
		{
			if (kBuilding.getReligionType() == GET_PLAYER(getOwnerINLINE()).getStateReligion())
			{
				iExtraRate += GET_PLAYER(getOwnerINLINE()).getStateReligionBuildingCommerce(eIndex);
			}
		}
		if (kBuilding.getGlobalReligionCommerce() != NO_RELIGION)
		{
			iExtraRate += GC.getReligionInfo((ReligionTypes)(kBuilding.getGlobalReligionCommerce())).getGlobalReligionCommerce(eIndex) * GC.getGameINLINE().countReligionLevels((ReligionTypes)(kBuilding.getGlobalReligionCommerce()));
		}
		if (kBuilding.getGlobalCorporationCommerce() != NO_CORPORATION)
		{
			iExtraRate += GC.getCorporationInfo((CorporationTypes)(kBuilding.getGlobalCorporationCommerce())).getHeadquarterCommerce(eIndex) * GC.getGameINLINE().countCorporationLevels((CorporationTypes)(kBuilding.getGlobalCorporationCommerce()));
		}
		// ignore double-time check since this assumes you are building it this turn

		// Specialists
		for (int iI = 0; iI < GC.getNumSpecialistInfos(); ++iI)
		{
			if (kBuilding.getFreeSpecialistCount((SpecialistTypes)iI) != 0)
			{
				iExtraRate += getAdditionalBaseCommerceRateBySpecialistImpl(eIndex, (SpecialistTypes)iI, kBuilding.getFreeSpecialistCount((SpecialistTypes)iI));
			}
		}
	}
	
	return iExtraRate;
}

/*
 * Returns the additional commerce rate modifier constructing the given building will provide.
 *
 * Doesn't check if the building can be constructed in this city.
 * Takes the NO_ESPIONAGE game option into account for CULTURE and ESPIONAGE.
 */
int CvCity::getAdditionalCommerceRateModifierByBuilding(CommerceTypes eIndex, BuildingTypes eBuilding) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");
	FAssertMsg(eBuilding >= 0, "eBuilding expected to be >= 0");
	FAssertMsg(eBuilding < GC.getNumBuildingInfos(), "eBuilding expected to be < GC.getNumBuildingInfos()");

	bool bNoEspionage = GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE);
	if (bNoEspionage && eIndex == COMMERCE_ESPIONAGE)
	{
		return 0;
	}

	int iExtraModifier = getAdditionalCommerceRateModifierByBuildingImpl(eIndex, eBuilding);
	if (bNoEspionage && eIndex == COMMERCE_CULTURE)
	{
		iExtraModifier += getAdditionalCommerceRateModifierByBuildingImpl(COMMERCE_ESPIONAGE, eBuilding);
	}
	return iExtraModifier;
}

/*
 * Returns the additional commerce rate modifier constructing the given building will provide.
 *
 * Doesn't check if the building can be constructed in this city.
 */
int CvCity::getAdditionalCommerceRateModifierByBuildingImpl(CommerceTypes eIndex, BuildingTypes eBuilding) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");
	FAssertMsg(eBuilding >= 0, "eBuilding expected to be >= 0");
	FAssertMsg(eBuilding < GC.getNumBuildingInfos(), "eBuilding expected to be < GC.getNumBuildingInfos()");

	CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	bool bObsolete = GET_TEAM(getTeam()).isObsoleteBuilding(eBuilding);
	int iExtraModifier = 0;

	if (!bObsolete)
	{
		iExtraModifier += kBuilding.getCommerceModifier(eIndex);
		iExtraModifier += kBuilding.getGlobalCommerceModifier(eIndex);
	}
	
	return iExtraModifier;
}
// BUG - Building Additional Commerce - end

int CvCity::getSpecialistCommerce(CommerceTypes eIndex)	const												 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");
	return m_aiSpecialistCommerce[eIndex];
}


void CvCity::changeSpecialistCommerce(CommerceTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");

	if (iChange != 0)
	{
		m_aiSpecialistCommerce[eIndex] = (m_aiSpecialistCommerce[eIndex] + iChange);
		FAssert(getSpecialistCommerce(eIndex) >= 0);

		updateCommerce(eIndex);
	}
}

// BUG - Specialist Additional Commerce - start
/*
 * Returns the total additional commerce that changing the number of given specialists will provide/remove.
 *
 * Takes the NO_ESPIONAGE game option into account for CULTURE and ESPIONAGE.
 */
int CvCity::getAdditionalCommerceBySpecialist(CommerceTypes eIndex, SpecialistTypes eSpecialist, int iChange) const
{
	return getAdditionalCommerceTimes100BySpecialist(eIndex, eSpecialist, iChange) / 100;
}

/*
 * Returns the total additional commerce times 100 that changing the number of given specialists will provide/remove.
 *
 * Takes the NO_ESPIONAGE game option into account for CULTURE and ESPIONAGE.
 */
int CvCity::getAdditionalCommerceTimes100BySpecialist(CommerceTypes eIndex, SpecialistTypes eSpecialist, int iChange) const
{
	int iExtraRate = getAdditionalBaseCommerceRateBySpecialist(eIndex, eSpecialist, iChange);
	if (iExtraRate == 0)
	{
		return 0;
	}

	int iRateTimes100 = getBaseCommerceRateTimes100(eIndex);
	int iModifier = getTotalCommerceRateModifier(eIndex);
	int iExtraTimes100 = (iModifier * (100 * iExtraRate + iRateTimes100) / 100) - (iModifier * iRateTimes100 / 100);

	return iExtraTimes100;
}

/*
 * Returns the additional base commerce rate that changing the number of given specialists will provide/remove.
 *
 * Takes the NO_ESPIONAGE game option into account for CULTURE and ESPIONAGE.
 */
int CvCity::getAdditionalBaseCommerceRateBySpecialist(CommerceTypes eIndex, SpecialistTypes eSpecialist, int iChange) const
{
	bool bNoEspionage = GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE);
	if (bNoEspionage && eIndex == COMMERCE_ESPIONAGE)
	{
		return 0;
	}

	int iExtraRate = getAdditionalBaseCommerceRateBySpecialistImpl(eIndex, eSpecialist, iChange);
	if (bNoEspionage && eIndex == COMMERCE_CULTURE)
	{
		iExtraRate += getAdditionalBaseCommerceRateBySpecialistImpl(COMMERCE_ESPIONAGE, eSpecialist, iChange);
	}
	return iExtraRate;
}

/*
 * Returns the additional base commerce rate that changing the number of given specialists will provide/remove.
 */
int CvCity::getAdditionalBaseCommerceRateBySpecialistImpl(CommerceTypes eIndex, SpecialistTypes eSpecialist, int iChange) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");
	FAssertMsg(eSpecialist >= 0, "eSpecialist expected to be >= 0");
	FAssertMsg(eSpecialist < GC.getNumSpecialistInfos(), "eSpecialist expected to be < GC.getNumSpecialistInfos()");

	CvSpecialistInfo& kSpecialist = GC.getSpecialistInfo(eSpecialist);
	return iChange * (kSpecialist.getCommerceChange(eIndex) + GET_PLAYER(getOwnerINLINE()).getSpecialistExtraCommerce(eIndex));
}
// BUG - Specialist Additional Commerce - end

int CvCity::getReligionCommerce(CommerceTypes eIndex) const												 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");
	return m_aiReligionCommerce[eIndex];
}


int CvCity::getReligionCommerceByReligion(CommerceTypes eIndex, ReligionTypes eReligion) const
{
	int iCommerce;

	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");
	FAssertMsg(eReligion >= 0, "eReligion expected to be >= 0");
	FAssertMsg(eReligion < GC.getNumReligionInfos(), "GC.getNumReligionInfos expected to be >= 0");

	iCommerce = 0;

	if ((GET_PLAYER(getOwnerINLINE()).getStateReligion() == eReligion) || (GET_PLAYER(getOwnerINLINE()).getStateReligion() == NO_RELIGION))
	{
		if (isHasReligion(eReligion))
		{
			iCommerce += GC.getReligionInfo(eReligion).getStateReligionCommerce(eIndex);

			if (isHolyCity(eReligion))
			{
				iCommerce += GC.getReligionInfo(eReligion).getHolyCityCommerce(eIndex);
			}
		}
	}

	return iCommerce;
}


// XXX can this be simplified???
void CvCity::updateReligionCommerce(CommerceTypes eIndex)
{
	int iNewReligionCommerce;
	int iI;

	iNewReligionCommerce = 0;

	for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		iNewReligionCommerce += getReligionCommerceByReligion(eIndex, ((ReligionTypes)iI));
	}

	if (getReligionCommerce(eIndex) != iNewReligionCommerce)
	{
		m_aiReligionCommerce[eIndex] = iNewReligionCommerce;
		FAssert(getReligionCommerce(eIndex) >= 0);

		updateCommerce(eIndex);
	}
}


void CvCity::updateReligionCommerce()
{
	int iI;

	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		updateReligionCommerce((CommerceTypes)iI);
	}
}


int CvCity::getCorporationYield(YieldTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");
	return m_aiCorporationYield[eIndex];
}

void CvCity::setCorporationYield(YieldTypes eIndex, int iNewValue)
{
	int iOldValue;

	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_YIELD_TYPES");

	iOldValue = getCorporationYield(eIndex);

	if (iOldValue != iNewValue)
	{
		m_aiCorporationYield[eIndex] = iNewValue;
		FAssert(getCorporationYield(eIndex) >= 0);

		changeBaseYieldRate(eIndex, (iNewValue - iOldValue));
	}
}

int CvCity::getCorporationCommerce(CommerceTypes eIndex) const												 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");
	return m_aiCorporationCommerce[eIndex];
}


int CvCity::getCorporationYieldByCorporation(YieldTypes eIndex, CorporationTypes eCorporation) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");
	FAssertMsg(eCorporation >= 0, "eCorporation expected to be >= 0");
	FAssertMsg(eCorporation < GC.getNumCorporationInfos(), "GC.getNumCorporationInfos expected to be >= 0");

	int iYield = 0;

	if (isActiveCorporation(eCorporation) && !isDisorder())
	{
		for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
		{
			BonusTypes eBonus = (BonusTypes)GC.getCorporationInfo(eCorporation).getPrereqBonus(i);
			if (NO_BONUS != eBonus && getNumBonuses(eBonus) > 0)
			{
				iYield += (GC.getCorporationInfo(eCorporation).getYieldProduced(eIndex) * getNumBonuses(eBonus) * GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getCorporationMaintenancePercent()) / 100;
			}
		}
	}

	return (iYield + 99) / 100;
}

int CvCity::getCorporationCommerceByCorporation(CommerceTypes eIndex, CorporationTypes eCorporation) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");
	FAssertMsg(eCorporation >= 0, "eCorporation expected to be >= 0");
	FAssertMsg(eCorporation < GC.getNumCorporationInfos(), "GC.getNumCorporationInfos expected to be >= 0");

	int iCommerce = 0;

	if (isActiveCorporation(eCorporation) && !isDisorder())
	{
		for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
		{
			BonusTypes eBonus = (BonusTypes)GC.getCorporationInfo(eCorporation).getPrereqBonus(i);
			if (NO_BONUS != eBonus && getNumBonuses(eBonus) > 0)
			{
				iCommerce += (GC.getCorporationInfo(eCorporation).getCommerceProduced(eIndex) * getNumBonuses(eBonus) * GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getCorporationMaintenancePercent()) / 100;
			}
		}
	}

	return (iCommerce + 99) / 100;
}

void CvCity::updateCorporationCommerce(CommerceTypes eIndex)
{
	int iNewCommerce = 0;

	for (int iI = 0; iI < GC.getNumCorporationInfos(); iI++)
	{
		iNewCommerce += getCorporationCommerceByCorporation(eIndex, ((CorporationTypes)iI));
	}

	if (getCorporationCommerce(eIndex) != iNewCommerce)
	{
		m_aiCorporationCommerce[eIndex] = iNewCommerce;
		FAssert(getCorporationCommerce(eIndex) >= 0);

		updateCommerce(eIndex);
	}
}

void CvCity::updateCorporationYield(YieldTypes eIndex)
{
	int iOldYield = getCorporationYield(eIndex);
	int iNewYield = 0;

	for (int iI = 0; iI < GC.getNumCorporationInfos(); iI++)
	{
		iNewYield += getCorporationYieldByCorporation(eIndex, (CorporationTypes)iI);
	}

	if (iOldYield != iNewYield)
	{
		m_aiCorporationYield[eIndex] = iNewYield;
		FAssert(getCorporationYield(eIndex) >= 0);

		changeBaseYieldRate(eIndex, (iNewYield - iOldYield));
	}
}


void CvCity::updateCorporation()
{
	updateCorporationBonus();

	updateBuildingCommerce();

	for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
	{
		updateCorporationYield((YieldTypes)iI);
	}

	for (int iI = 0; iI < NUM_COMMERCE_TYPES; ++iI)
	{
		updateCorporationCommerce((CommerceTypes)iI);
	}

	updateMaintenance();
}


void CvCity::updateCorporationBonus()
{
	std::vector<int> aiExtraCorpProducedBonuses;
	std::vector<int> aiLastCorpProducedBonuses;
	std::vector<bool> abHadBonuses;

	for (int iI = 0; iI < GC.getNumBonusInfos(); ++iI)
	{
		abHadBonuses.push_back(hasBonus((BonusTypes)iI));
		m_paiNumCorpProducedBonuses[iI] = 0; 
		aiLastCorpProducedBonuses.push_back(getNumBonuses((BonusTypes)iI));
		aiExtraCorpProducedBonuses.push_back(0);
	}

	for (int iIter = 0; iIter < GC.getNumCorporationInfos(); ++iIter)
	{
		for (int iCorp = 0; iCorp < GC.getNumCorporationInfos(); ++iCorp)
		{
			int iBonusProduced = GC.getCorporationInfo((CorporationTypes)iCorp).getBonusProduced();

			if (NO_BONUS != iBonusProduced)
			{
				if (!GET_TEAM(getTeam()).isBonusObsolete((BonusTypes)iBonusProduced))
				{
					if (GET_TEAM(getTeam()).isHasTech((TechTypes)(GC.getBonusInfo((BonusTypes)iBonusProduced).getTechCityTrade())))
					{
						if (isHasCorporation((CorporationTypes)iCorp) && GET_PLAYER(getOwnerINLINE()).isActiveCorporation((CorporationTypes)iCorp))
						{
							for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
							{
								int iBonusConsumed = GC.getCorporationInfo((CorporationTypes)iCorp).getPrereqBonus(i);
								if (NO_BONUS != iBonusConsumed)
								{
									aiExtraCorpProducedBonuses[iBonusProduced] += aiLastCorpProducedBonuses[iBonusConsumed];
								}
							}
						}
					}
				}
			}
		}

		bool bChanged = false;

		for (int iI = 0; iI < GC.getNumBonusInfos(); ++iI)
		{
			if (aiExtraCorpProducedBonuses[iI] != 0)
			{
				m_paiNumCorpProducedBonuses[iI] += aiExtraCorpProducedBonuses[iI];

				bChanged = true;
			}

			aiLastCorpProducedBonuses[iI] = aiExtraCorpProducedBonuses[iI];
			aiExtraCorpProducedBonuses[iI] = 0;
		}

		if (!bChanged)
		{
			break;
		}

		FAssertMsg(iIter < GC.getNumCorporationInfos() - 1, "Corporation cyclical resource dependency");
	}

	for (int iI = 0; iI < GC.getNumBonusInfos(); ++iI)
	{
		if (abHadBonuses[iI] != hasBonus((BonusTypes)iI))
		{
			if (hasBonus((BonusTypes)iI))
			{
				processBonus((BonusTypes)iI, 1);
			}
			else
			{
				processBonus((BonusTypes)iI, -1);
				verifyProduction(); // advc.064d
			}
		}
	}
}


int CvCity::getCommerceRateModifier(CommerceTypes eIndex) const											 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");
	return m_aiCommerceRateModifier[eIndex];
}


void CvCity::changeCommerceRateModifier(CommerceTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");

	if (iChange != 0)
	{
		m_aiCommerceRateModifier[eIndex] = (m_aiCommerceRateModifier[eIndex] + iChange);

		updateCommerce(eIndex);

		AI_setAssignWorkDirty(true);
	}
}


int CvCity::getCommerceHappinessPer(CommerceTypes eIndex) const									 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");
	return m_aiCommerceHappinessPer[eIndex];
}


int CvCity::getCommerceHappinessByType(CommerceTypes eCommerce) const
{
	return ((getCommerceHappinessPer(eCommerce) * GET_PLAYER(getOwnerINLINE()).getCommercePercent(eCommerce)) / 100);
}


int CvCity::getCommerceHappiness() const
{
	int iHappiness;
	int iI;

	iHappiness = 0;

	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		iHappiness += getCommerceHappinessByType((CommerceTypes)iI);
	}

	return iHappiness;
}


void CvCity::changeCommerceHappinessPer(CommerceTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex expected to be < NUM_COMMERCE_TYPES");

	if (iChange != 0)
	{
		m_aiCommerceHappinessPer[eIndex] = (m_aiCommerceHappinessPer[eIndex] + iChange);
		FAssert(getCommerceHappinessPer(eIndex) >= 0);

		AI_setAssignWorkDirty(true);
	}
}


int CvCity::getDomainFreeExperience(DomainTypes eIndex) const													 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	return m_aiDomainFreeExperience[eIndex];
}


void CvCity::changeDomainFreeExperience(DomainTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	m_aiDomainFreeExperience[eIndex] = (m_aiDomainFreeExperience[eIndex] + iChange);
	FAssert(getDomainFreeExperience(eIndex) >= 0);
}


int CvCity::getDomainProductionModifier(DomainTypes eIndex) const										 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	return m_aiDomainProductionModifier[eIndex];
}


void CvCity::changeDomainProductionModifier(DomainTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex expected to be < NUM_DOMAIN_TYPES");
	m_aiDomainProductionModifier[eIndex] = (m_aiDomainProductionModifier[eIndex] + iChange);
}


int CvCity::getCulture(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex expected to be < MAX_PLAYERS");
	return m_aiCulture[eIndex] / 100;
}

int CvCity::getCultureTimes100(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex expected to be < MAX_PLAYERS");
	return m_aiCulture[eIndex];
}


int CvCity::countTotalCultureTimes100() const
{
	int iTotalCulture = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isEverAlive()) // advc.099: was isAlive
			iTotalCulture += getCultureTimes100((PlayerTypes)iI);
	}
	return iTotalCulture;
}


PlayerTypes CvCity::findHighestCulture() const
{

	int iBestValue = 0;
	PlayerTypes eBestPlayer = NO_PLAYER;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{	// advc.099: Replaced "Alive" with "EverAlive".
		/*  Note: This means that no civ gets angry about a razed city with a
			dead cultural owner. */ 
		if (GET_PLAYER((PlayerTypes)iI).isEverAlive())
		{
			int iValue = getCultureTimes100((PlayerTypes)iI);

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				eBestPlayer = ((PlayerTypes)iI);
			}
		}
	}

	return eBestPlayer;
}
// <advc.101>
/*  Doesn't check if city will flip. Doesn't take into account that the revolt test
	is skipped when decreasing the occupation timer.
	Difference from getRevoltTestProbability: that function only returns the
	probability of the first revolt test, not the second one based on
	culture garrison.
	If IgnoreWar is set, a probability is returned even if the city can't revolt
	(i.e. when it's already in occupation and at war with the cultural owner).
	If IgnoreGarrison is set, culture garrison strength is treated as 0.
	If IgnoreOccupation is set, the probability is computed assuming that the city
	isn't in occupation. */
double CvCity::revoltProbability(bool bIgnoreWar,
		bool bIgnoreGarrison, bool bIgnoreOccupation) const { // advc.023

	PlayerTypes eCulturalOwner = calculateCulturalOwner(); // advc.099c
	CvGame const& g = GC.getGameINLINE();
	if(eCulturalOwner == NO_PLAYER || TEAMID(eCulturalOwner) == getTeam()
			// <advc.099c> Barbarian revolts
			|| (eCulturalOwner == BARBARIAN_PLAYER &&
			GC.getDefineINT("BARBS_REVOLT") <= 0) ||
			(GET_PLAYER(getOwnerINLINE()).getCurrentEra() <= 0 &&
			g.getGameTurn() - getGameTurnFounded() <
			(10 * GC.getGameSpeedInfo(g.getGameSpeedType()).
			getConstructPercent()) / 100)) // </advc.099c>
		return 0;
	// <advc.023>
	double occupationFactor = 1;
	if(isOccupation() && !bIgnoreOccupation) {
		occupationFactor = 0.5;
		if(!bIgnoreWar && !isBarbarian() && !GET_TEAM(getTeam()).isMinorCiv() &&
				GET_PLAYER(eCulturalOwner).isAlive() &&
				GET_TEAM(getTeam()).isAtWar(TEAMID(eCulturalOwner)))
			return 0;
	} // </advc.023>
	int iCityStrength = cultureStrength(eCulturalOwner);
	int iGarrison =
		(bIgnoreGarrison ? 0 : // advc.023
		cultureGarrison(eCulturalOwner));
	if(iCityStrength <= iGarrison)
		return 0;
	/*  About the two revolt tests: I guess the first one checks if the city tries
		to revolt, and the second if the garrison can stop the revolt.
		Restored the BtS formula for the second test. */
	double r = std::max(0.0, (1.0 - (iGarrison / (double)iCityStrength))) *
			getRevoltTestProbability() * occupationFactor;
	// Don't use probabilities that are too small to be displayed
	if(r < 0.001)
		return 0;
	if(r > 0.999)
		return 1;
	return r;
} // </advc.101>

// <advc.023>
double CvCity::probabilityOccupationDecrement() const {

	if(!isOccupation())
		return 0;
	// While at war, require an occupying force.
	if(getMilitaryHappinessUnits() <= 0 && !isBarbarian() && !GET_TEAM(getTeam()).isMinorCiv()) {
		PlayerTypes eCulturalOwner = calculateCulturalOwner();
		if(eCulturalOwner != NO_PLAYER && TEAMREF(eCulturalOwner).isAtWar(getTeam()))
			return 0;
	}
	double r = std::pow(1 - revoltProbability(true, false, true),
			GC.getDefineINT("OCCUPATION_COUNTDOWN_EXPONENT"));
	// Don't use probabilities that are too small to be displayed
	if(r < 0.001)
		return 0;
	if(r > 0.999)
		return 1;
	return r;
} // </advc.023>

// K-Mod. The following function defines whether or not the city is allowed to flip to the given player
bool CvCity::canCultureFlip(PlayerTypes eToPlayer,
		bool bCheckPriorRevolts) const // advc.101
{
	/*if (isBarbarian()) // advc.101: Commented out
		return true;*/
	// <advc.099c> Actually still guaranteed by caller, but shouldn't rely on it
	if(eToPlayer == NO_PLAYER || !GET_PLAYER(eToPlayer).isAlive() ||
			eToPlayer == BARBARIAN_PLAYER || // Not guaranteed by caller
			TEAMREF(eToPlayer).isVassal(getTeam()))
		return false; // </advc.099c>
	return !GC.getGameINLINE().isOption(GAMEOPTION_NO_CITY_FLIPPING) &&
		// advc.101: City flipping option negated (now has inverse meaning)
		(!GC.getGameINLINE().isOption(GAMEOPTION_NO_FLIPPING_AFTER_CONQUEST) ||
		getPreviousOwner() == NO_PLAYER ||
		TEAMID(getPreviousOwner()) != TEAMID(eToPlayer)) && // advc.003
		(!bCheckPriorRevolts || // advc.101
		getNumRevolts(eToPlayer) >= GC.getNUM_WARNING_REVOLTS()
		- (isBarbarian() ? 1 : 0)); // advc.101
}
// K-Mod end

int CvCity::calculateCulturePercent(PlayerTypes eIndex) const
{
	int iTotalCulture;

	iTotalCulture = countTotalCultureTimes100();

	if (iTotalCulture > 0)
	{
		return ((getCultureTimes100(eIndex) * 100) / iTotalCulture);
	}

	return 0;
}


int CvCity::calculateTeamCulturePercent(TeamTypes eIndex) const
{
	int iTeamCulturePercent = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer const& kMember = GET_PLAYER((PlayerTypes)iI);
		if (kMember.isEverAlive() && // advc.099: was isAlive
				kMember.getTeam() == eIndex)
			iTeamCulturePercent += calculateCulturePercent(kMember.getID());
	}
	return iTeamCulturePercent;
}


void CvCity::setCulture(PlayerTypes eIndex, int iNewValue, bool bPlots, bool bUpdatePlotGroups)
{
	setCultureTimes100(eIndex, 100 * iNewValue, bPlots, bUpdatePlotGroups);
}

void CvCity::setCultureTimes100(PlayerTypes eIndex, int iNewValue, bool bPlots, bool bUpdatePlotGroups)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex expected to be < MAX_PLAYERS");

/*
** K-Mod, 26/sep/10, Karadoc
** fixed so that plots actually get the culture difference
*/
	/* original bts code
	if (getCultureTimes100(eIndex) != iNewValue)
	*/
	int iOldValue = getCultureTimes100(eIndex);

	if (iNewValue != iOldValue)
	{
		m_aiCulture[eIndex] = iNewValue;
		FAssert(getCultureTimes100(eIndex) >= 0);

		updateCultureLevel(bUpdatePlotGroups);

		if (bPlots)
		{
			/* original bts code
			doPlotCulture(true, eIndex, 0);
			*/
			//doPlotCulture(true, eIndex, (iNewValue-iOldValue)/100);
			doPlotCultureTimes100(true, eIndex, (iNewValue-iOldValue), false);
			// note: this function no longer applies free city culture.
			// also, note that if a city's culture is decreased to zero, there will probably still be some residual plot culture around the city
			// this is because the culture level on the way up will be higher than it is on the way down.
		}
	}
/*
** K-Mod end
*/
}


void CvCity::changeCulture(PlayerTypes eIndex, int iChange, bool bPlots, bool bUpdatePlotGroups)
{
	setCultureTimes100(eIndex, (getCultureTimes100(eIndex) + 100  * iChange), bPlots, bUpdatePlotGroups);
}

void CvCity::changeCultureTimes100(PlayerTypes eIndex, int iChange, bool bPlots, bool bUpdatePlotGroups)
{
	setCultureTimes100(eIndex, (getCultureTimes100(eIndex) + iChange), bPlots, bUpdatePlotGroups);
}


int CvCity::getNumRevolts(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex expected to be < MAX_PLAYERS");
	return m_aiNumRevolts[eIndex];
}
// <advc.099c>
int CvCity::getNumRevolts() const {

	PlayerTypes eCultOwner = calculateCulturalOwner();
	if(eCultOwner == NO_PLAYER)
		return 0;
	return getNumRevolts(eCultOwner);
} // </advc.099c>

void CvCity::changeNumRevolts(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex expected to be < MAX_PLAYERS");
	m_aiNumRevolts[eIndex] = (m_aiNumRevolts[eIndex] + iChange);
	FAssert(getNumRevolts(eIndex) >= 0);
}


double CvCity::getRevoltTestProbability() const // advc.101: Changed return type
{
	int iBestModifier = 0;

	CLLNode<IDInfo>* pUnitNode = plot()->headUnitNode();
	while (pUnitNode)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = plot()->nextUnitNode(pUnitNode);

		if (pLoopUnit->getRevoltProtection() > iBestModifier)
		{
			iBestModifier = pLoopUnit->getRevoltProtection();
		}
	}
	iBestModifier = range(iBestModifier, 0, 100);

	return std::min(1.0, // advc.101: Upper bound used to be handled by the caller
			((GC.getDefineINT("REVOLT_TEST_PROB") * (100 - iBestModifier)) / 100.0)
			// advc.101: Speed scaling as in K-Mod
			/ GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).
			getVictoryDelayPercent());
}

bool CvCity::isEverOwned(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex expected to be < MAX_PLAYERS");
	return m_abEverOwned[eIndex];
}


void CvCity::setEverOwned(PlayerTypes eIndex, bool bNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex expected to be < MAX_PLAYERS");
	m_abEverOwned[eIndex] = bNewValue;
}


bool CvCity::isTradeRoute(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex expected to be < MAX_PLAYERS");
	return m_abTradeRoute[eIndex];
}


void CvCity::setTradeRoute(PlayerTypes eIndex, bool bNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex expected to be < MAX_PLAYERS");
	if (m_abTradeRoute[eIndex] != bNewValue)
	{
		m_abTradeRoute[eIndex] = bNewValue;
	}
}


bool CvCity::isRevealed(TeamTypes eIndex, bool bDebug) const
{
	if (bDebug && GC.getGameINLINE().isDebugMode())
	{
		return true;
	}
	else
	{
		FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
		FAssertMsg(eIndex < MAX_TEAMS, "eIndex expected to be < MAX_TEAMS");

		return m_abRevealed[eIndex];
	}
}


void CvCity::setRevealed(TeamTypes eIndex, bool bNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex expected to be < MAX_TEAMS");

	if(isRevealed(eIndex, false) == bNewValue)
		return;

	m_abRevealed[eIndex] = bNewValue;

	// K-Mod
	if (bNewValue) {
		GET_TEAM(eIndex).makeHasSeen(getTeam());
	// K-Mod end
		// <advc.130n>
		for(int i = 0; i < GC.getNumReligionInfos(); i++) {
			ReligionTypes eReligion = (ReligionTypes)i;
			if(isHasReligion(eReligion))
				GET_TEAM(eIndex).AI_reportNewReligion(eReligion);
		} // </advc.130n>
	}

	updateVisibility();

	if (eIndex == GC.getGameINLINE().getActiveTeam())
	{
		for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			CvPlot* pLoopPlot = getCityIndexPlot(iI);

			if (pLoopPlot != NULL)
			{
				pLoopPlot->updateSymbols();
			}
		}
	}
}


bool CvCity::getEspionageVisibility(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex expected to be < MAX_TEAMS");

	return m_abEspionageVisibility[eIndex];
}


void CvCity::setEspionageVisibility(TeamTypes eIndex, bool bNewValue, bool bUpdatePlotGroups)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex expected to be < MAX_TEAMS");

	if (getEspionageVisibility(eIndex) != bNewValue)
	{
		plot()->updateSight(false, bUpdatePlotGroups);

		m_abEspionageVisibility[eIndex] = bNewValue;

		plot()->updateSight(true, bUpdatePlotGroups);
	}
}

void CvCity::updateEspionageVisibility(bool bUpdatePlotGroups)
{
	std::vector<EspionageMissionTypes> aMission;
	for (int iMission = 0; iMission < GC.getNumEspionageMissionInfos(); ++iMission)
	{
		if (GC.getEspionageMissionInfo((EspionageMissionTypes)iMission).isPassive() && GC.getEspionageMissionInfo((EspionageMissionTypes)iMission).getVisibilityLevel() > 0)
		{
			aMission.push_back((EspionageMissionTypes)iMission);
		}
	}

	for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
	{
		bool bVisibility = false;

		if (iTeam != getTeam())
		{
			if (isRevealed((TeamTypes)iTeam, false))
			{
				for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
				{
					CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
					if (kPlayer.isAlive() && kPlayer.getTeam() == iTeam)
					{
						for (std::vector<EspionageMissionTypes>::iterator it = aMission.begin(); it != aMission.end(); ++it)
						{
							if (kPlayer.canDoEspionageMission(*it, getOwnerINLINE(), plot(), -1, NULL))
							{
								bVisibility = true;
								break;
							}
						}

						if (bVisibility)
						{
							break;
						}
					}
				}
			}
		}

		setEspionageVisibility((TeamTypes)iTeam, bVisibility, bUpdatePlotGroups);
	}
}

const wchar* CvCity::getNameKey() const
{
	return m_szName;
}


const CvWString CvCity::getName(uint uiForm) const
{
	return gDLL->getObjectText(m_szName, uiForm, true);
}


void CvCity::setName(const wchar* szNewValue, bool bFound,
		bool bInitial) // advc.106k
{
	CvWString szName(szNewValue);
	gDLL->stripSpecialCharacters(szName);
	// K-Mod. stripSpecialCharacters apparently doesn't count '%' as a special character
	// however, strings with '%' in them will cause the game to crash. So I'm going to strip them out.
	for (CvWString::iterator it = szName.begin(); it != szName.end(); )
	{
		if (*it == '%')
			it = szName.erase(it);
		else
			++it;
	}
	// K-Mod end

	if (!szName.empty())
	{
		if (GET_PLAYER(getOwnerINLINE()).isCityNameValid(szName, false))
		{	// <advc.106k>
			if(bInitial)
				m_szPreviousName.clear();
			else if(m_szPreviousName.empty())
				m_szPreviousName = m_szName; // </advc.106k>
			m_szName = szName;

			setInfoDirty(true);

			if (isCitySelected())
			{
				gDLL->getInterfaceIFace()->setDirty(CityScreen_DIRTY_BIT, true);
			}
		}
		if (bFound)
		{
			doFoundMessage();
		}
	}
}


void CvCity::doFoundMessage()
{
	CvWString szBuffer;

	szBuffer = gDLL->getText("TXT_KEY_MISC_CITY_HAS_BEEN_FOUNDED", getNameKey());
	gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), false, -1, szBuffer, ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_CITY_EDIT")->getPath(),
			MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY, // advc.106b
			NULL, NO_COLOR, getX_INLINE(), getY_INLINE());

	szBuffer = gDLL->getText("TXT_KEY_MISC_CITY_IS_FOUNDED", getNameKey());
	GC.getGameINLINE().addReplayMessage(REPLAY_MESSAGE_CITY_FOUNDED, getOwnerINLINE(), szBuffer, getX_INLINE(), getY_INLINE(),
			//(ColorTypes)GC.getInfoTypeForString("COLOR_ALT_HIGHLIGHT_TEXT")
			// advc.106: Use ALT_HIGHLIGHT for research-related stuff now
			GET_PLAYER(getOwnerINLINE()).getPlayerTextColor());
}


std::string CvCity::getScriptData() const
{
	return m_szScriptData;
}


void CvCity::setScriptData(std::string szNewValue)
{
	m_szScriptData = szNewValue;
}


int CvCity::getNoBonusCount(BonusTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex expected to be < GC.getNumBonusInfos()");
	return m_paiNoBonus[eIndex];
}

bool CvCity::isNoBonus(BonusTypes eIndex) const
{
	return (getNoBonusCount(eIndex) > 0);
}

void CvCity::changeNoBonusCount(BonusTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex expected to be < GC.getNumBonusInfos()");

	if (iChange != 0)
	{
		if (getNumBonuses(eIndex) > 0)
		{
			processBonus(eIndex, -1);
			verifyProduction(); // advc.064d
		}

		m_paiNoBonus[eIndex] += iChange;
		FAssert(getNoBonusCount(eIndex) >= 0);

		if (getNumBonuses(eIndex) > 0)
		{
			processBonus(eIndex, 1);
		}

		updateCorporation();

		AI_setAssignWorkDirty(true);

		setInfoDirty(true);
	}
}


int CvCity::getFreeBonus(BonusTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex expected to be < GC.getNumBonusInfos()");
	return m_paiFreeBonus[eIndex];
}


void CvCity::changeFreeBonus(BonusTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex expected to be < GC.getNumBonusInfos()");

	if (iChange != 0)
	{
		plot()->updatePlotGroupBonus(false);
		m_paiFreeBonus[eIndex] += iChange;
		FAssert(getFreeBonus(eIndex) >= 0);
		plot()->updatePlotGroupBonus(true);
	}
}

int CvCity::getNumBonuses(BonusTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex expected to be < GC.getNumBonusInfos()");

	if (isNoBonus(eIndex))
	{
		return 0;
	}

	return m_paiNumBonuses[eIndex] + //m_paiNumCorpProducedBonuses[eIndex];
			// advc.003j: Shouldn't leave this function to be entirely unused
			getNumCorpProducedBonuses(eIndex);
}


bool CvCity::hasBonus(BonusTypes eIndex) const
{
	return (getNumBonuses(eIndex) > 0);
}


void CvCity::changeNumBonuses(BonusTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex expected to be < GC.getNumBonusInfos()");

	if (iChange == 0)
		return;

	bool bOldHasBonus = hasBonus(eIndex);

	m_paiNumBonuses[eIndex] += iChange;

	if (bOldHasBonus != hasBonus(eIndex))
	{
		if (hasBonus(eIndex))
		{
			processBonus(eIndex, 1);
		}
		else
		{
			processBonus(eIndex, -1);
		}
	}

	if (isCorporationBonus(eIndex))
	{
		updateCorporation();
	}
}

// <advc.149>
int CvCity::countUniqueBonuses() const {

	int r = 0;
	for(int i = 0; i < GC.getNumBonusInfos(); i++)
		if(hasBonus((BonusTypes)i))
			r++;
	return r;
}
// </advc.149>

int CvCity::getNumCorpProducedBonuses(BonusTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumBonusInfos(), "eIndex expected to be < GC.getNumBonusInfos()");
	return m_paiNumCorpProducedBonuses[eIndex];
}


bool CvCity::isCorporationBonus(BonusTypes eBonus) const
{
	FAssert(eBonus >= 0);
	FAssert(eBonus < GC.getNumBonusInfos());

	for (int iCorp = 0; iCorp < GC.getNumCorporationInfos(); ++iCorp)
	{
		if (GET_PLAYER(getOwnerINLINE()).isActiveCorporation((CorporationTypes)iCorp))
		{
			for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
			{
				if (NO_BONUS != GC.getCorporationInfo((CorporationTypes)iCorp).getPrereqBonus(i))
				{
					if (GC.getCorporationInfo((CorporationTypes)iCorp).getPrereqBonus(i) == eBonus && isHasCorporation((CorporationTypes)iCorp))
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool CvCity::isActiveCorporation(CorporationTypes eCorporation) const
{
	FAssert(eCorporation >= 0 && eCorporation < GC.getNumCorporationInfos());

	if (!isHasCorporation(eCorporation))
	{
		return false;
	}

	if (!GET_PLAYER(getOwnerINLINE()).isActiveCorporation(eCorporation))
	{
		return false;
	}

	for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
	{
		BonusTypes eBonus = (BonusTypes)GC.getCorporationInfo(eCorporation).getPrereqBonus(i);

		if (NO_BONUS != eBonus)
		{
			if (getNumBonuses(eBonus) > 0)
			{
				return true;
			}
		}
	}

	return false;
}

int CvCity::getBuildingProduction(BuildingTypes eIndex)	const															 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex expected to be < GC.getNumBuildingInfos()");
	return m_paiBuildingProduction[eIndex];
}


void CvCity::setBuildingProduction(BuildingTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex expected to be < GC.getNumBuildingInfos()");

	if (getBuildingProduction(eIndex) != iNewValue)
	{
		m_paiBuildingProduction[eIndex] = iNewValue;
		FAssert(getBuildingProduction(eIndex) >= 0);

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			setInfoDirty(true);
		}

		if ((getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) && isCitySelected())
		{
			gDLL->getInterfaceIFace()->setDirty(CityScreen_DIRTY_BIT, true);
		}
	}
}


void CvCity::changeBuildingProduction(BuildingTypes eIndex, int iChange)							 
{
	setBuildingProduction(eIndex, (getBuildingProduction(eIndex) + iChange));
}


int CvCity::getBuildingProductionTime(BuildingTypes eIndex)	const															 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex expected to be < GC.getNumBuildingInfos()");
	return m_paiBuildingProductionTime[eIndex];
}


void CvCity::setBuildingProductionTime(BuildingTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex expected to be < GC.getNumBuildingInfos()");
	m_paiBuildingProductionTime[eIndex] = iNewValue;
	FAssert(getBuildingProductionTime(eIndex) >= 0);
}


void CvCity::changeBuildingProductionTime(BuildingTypes eIndex, int iChange)							 
{
	setBuildingProductionTime(eIndex, (getBuildingProductionTime(eIndex) + iChange));
}


int CvCity::getProjectProduction(ProjectTypes eIndex) const																 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex expected to be < GC.getNumProjectInfos()");
	return m_paiProjectProduction[eIndex];
}


void CvCity::setProjectProduction(ProjectTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex expected to be < GC.getNumProjectInfos()");

	if (getProjectProduction(eIndex) != iNewValue)
	{
		m_paiProjectProduction[eIndex] = iNewValue;
		FAssert(getProjectProduction(eIndex) >= 0);

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			setInfoDirty(true);
		}

		if ((getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) && isCitySelected())
		{
			gDLL->getInterfaceIFace()->setDirty(CityScreen_DIRTY_BIT, true);
		}
	}
}


void CvCity::changeProjectProduction(ProjectTypes eIndex, int iChange)							 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex expected to be < GC.getNumProjectInfos()");
	setProjectProduction(eIndex, (getProjectProduction(eIndex) + iChange));
}


int CvCity::getBuildingOriginalOwner(BuildingTypes eIndex) const															 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex expected to be < GC.getNumBuildingInfos()");
	return m_paiBuildingOriginalOwner[eIndex];
}


int CvCity::getBuildingOriginalTime(BuildingTypes eIndex) const															  
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex expected to be < GC.getNumBuildingInfos()");
	return m_paiBuildingOriginalTime[eIndex];
}


int CvCity::getUnitProduction(UnitTypes eIndex)	const																			 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumUnitInfos(), "eIndex expected to be < GC.getNumUnitInfos()");
	return m_paiUnitProduction[eIndex];
}


void CvCity::setUnitProduction(UnitTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumUnitInfos(), "eIndex expected to be < GC.getNumUnitInfos()");

	if (getUnitProduction(eIndex) != iNewValue)
	{
		m_paiUnitProduction[eIndex] = iNewValue;
		FAssert(getUnitProduction(eIndex) >= 0);

		if (getTeam() == GC.getGameINLINE().getActiveTeam())
		{
			setInfoDirty(true);
		}

		if ((getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) && isCitySelected())
		{
			gDLL->getInterfaceIFace()->setDirty(CityScreen_DIRTY_BIT, true);
		}
	}
}


void CvCity::changeUnitProduction(UnitTypes eIndex, int iChange)											 
{
	setUnitProduction(eIndex, (getUnitProduction(eIndex) + iChange));
}


int CvCity::getUnitProductionTime(UnitTypes eIndex)	const																			 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumUnitInfos(), "eIndex expected to be < GC.getNumUnitInfos()");
	return m_paiUnitProductionTime[eIndex];
}


void CvCity::setUnitProductionTime(UnitTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumUnitInfos(), "eIndex expected to be < GC.getNumUnitInfos()");
	m_paiUnitProductionTime[eIndex] = iNewValue;
	FAssert(getUnitProductionTime(eIndex) >= 0);
}


void CvCity::changeUnitProductionTime(UnitTypes eIndex, int iChange)											 
{
	setUnitProductionTime(eIndex, (getUnitProductionTime(eIndex) + iChange));
}


int CvCity::getGreatPeopleUnitRate(UnitTypes eIndex) const																 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumUnitInfos(), "eIndex expected to be < GC.getNumUnitInfos()");
	return m_paiGreatPeopleUnitRate[eIndex];
}


void CvCity::setGreatPeopleUnitRate(UnitTypes eIndex, int iNewValue)										 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumUnitInfos(), "eIndex expected to be < GC.getNumUnitInfos()");
	if (GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE) && GC.getUnitInfo(eIndex).getEspionagePoints() > 0)
	{
		return;
	}

	m_paiGreatPeopleUnitRate[eIndex] = iNewValue;
	FAssert(getGreatPeopleUnitRate(eIndex) >= 0);
}


void CvCity::changeGreatPeopleUnitRate(UnitTypes eIndex, int iChange)									 
{
	setGreatPeopleUnitRate(eIndex, (getGreatPeopleUnitRate(eIndex) + iChange));
}


int CvCity::getGreatPeopleUnitProgress(UnitTypes eIndex) const														 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumUnitInfos(), "eIndex expected to be < GC.getNumUnitInfos()");
	return m_paiGreatPeopleUnitProgress[eIndex];
}


void CvCity::setGreatPeopleUnitProgress(UnitTypes eIndex, int iNewValue)							 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumUnitInfos(), "eIndex expected to be < GC.getNumUnitInfos()");
	m_paiGreatPeopleUnitProgress[eIndex] = iNewValue;
	FAssert(getGreatPeopleUnitProgress(eIndex) >= 0);
}


void CvCity::changeGreatPeopleUnitProgress(UnitTypes eIndex, int iChange)							 
{
	setGreatPeopleUnitProgress(eIndex, (getGreatPeopleUnitProgress(eIndex) + iChange));
}


int CvCity::getSpecialistCount(SpecialistTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumSpecialistInfos(), "eIndex expected to be < GC.getNumSpecialistInfos()");
	return m_paiSpecialistCount[eIndex];
}


void CvCity::setSpecialistCount(SpecialistTypes eIndex, int iNewValue)
{
	int iOldValue;

	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumSpecialistInfos(), "eIndex expected to be < GC.getNumSpecialistInfos()");

	iOldValue = getSpecialistCount(eIndex);

	if (iOldValue != iNewValue)
	{
		m_paiSpecialistCount[eIndex] = iNewValue;
		FAssert(getSpecialistCount(eIndex) >= 0);

		changeSpecialistPopulation(iNewValue - iOldValue);
		processSpecialist(eIndex, (iNewValue - iOldValue));

		if (isCitySelected())
		{
			gDLL->getInterfaceIFace()->setDirty(CitizenButtons_DIRTY_BIT, true);
		}
	}
}


void CvCity::changeSpecialistCount(SpecialistTypes eIndex, int iChange)
{
	setSpecialistCount(eIndex, (getSpecialistCount(eIndex) + iChange));
}


void CvCity::alterSpecialistCount(SpecialistTypes eIndex, int iChange)
{
	int iI;

	if(iChange == 0)
		return;

	if (isCitizensAutomated())
	{
		if ((getForceSpecialistCount(eIndex) + iChange) < 0)
		{
			setCitizensAutomated(false);
		}
	}

	if (isCitizensAutomated())
	{
		changeForceSpecialistCount(eIndex, iChange);
	}
	//else // (K-Mod. Without the following block, extra care is needed inside AI_assignWorkingPlots.)
	{
		if (iChange > 0)
		{
			for (iI = 0; iI < iChange; iI++)
			{
				if ((extraPopulation() > 0) || AI_removeWorstCitizen(eIndex))
				{
					if (isSpecialistValid(eIndex, 1))
					{
						changeSpecialistCount(eIndex, 1);
					}
				}
			}
		}
		else
		{
			for (iI = 0; iI < -(iChange); iI++)
			{
				if (getSpecialistCount(eIndex) > 0)
				{
					changeSpecialistCount(eIndex, -1);

					if ((eIndex != GC.getDefineINT("DEFAULT_SPECIALIST")) && (GC.getDefineINT("DEFAULT_SPECIALIST") != NO_SPECIALIST))
					{
						changeSpecialistCount(((SpecialistTypes)GC.getDefineINT("DEFAULT_SPECIALIST")), 1);
					}
					else if (extraFreeSpecialists() > 0)
					{
						AI_addBestCitizen(false, true);
					}
					else
					{
						int iNumCanWorkPlots = 0;
						for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
						{
							if (iJ != CITY_HOME_PLOT)
							{
								if (!isWorkingPlot(iJ))
								{
									CvPlot* pLoopPlot = getCityIndexPlot(iJ);

									if (pLoopPlot != NULL)
									{
										if (canWork(pLoopPlot))
										{
											++iNumCanWorkPlots;
										}
									}
								}
							}
						}

						if (iNumCanWorkPlots > 0)
						{
							AI_addBestCitizen(true, false);
						}
						else
						{
							AI_addBestCitizen(false, true);
						}
					}
				}
			}
		}
	}
}


int CvCity::getMaxSpecialistCount(SpecialistTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumSpecialistInfos(), "eIndex expected to be < GC.getNumSpecialistInfos()");
	return m_paiMaxSpecialistCount[eIndex];
}


bool CvCity::isSpecialistValid(SpecialistTypes eIndex, int iExtra) const
{
	return (((getSpecialistCount(eIndex) + iExtra) <= getMaxSpecialistCount(eIndex)) || GET_PLAYER(getOwnerINLINE()).isSpecialistValid(eIndex) || (eIndex == GC.getDefineINT("DEFAULT_SPECIALIST")));
}


void CvCity::changeMaxSpecialistCount(SpecialistTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumSpecialistInfos(), "eIndex expected to be < GC.getNumSpecialistInfos()");

	if (iChange != 0)
	{
		m_paiMaxSpecialistCount[eIndex] = std::max(0, (m_paiMaxSpecialistCount[eIndex] + iChange));

		AI_setAssignWorkDirty(true);
	}
}


int CvCity::getForceSpecialistCount(SpecialistTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumSpecialistInfos(), "eIndex expected to be < GC.getNumSpecialistInfos()");
	return m_paiForceSpecialistCount[eIndex];
}


bool CvCity::isSpecialistForced() const
{
	int iI;

	for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
	{
		if (getForceSpecialistCount((SpecialistTypes)iI) > 0)
		{
			return true;
		}
	}

	return false;
}


void CvCity::setForceSpecialistCount(SpecialistTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumSpecialistInfos(), "eIndex expected to be < GC.getNumSpecialistInfos()");

	if (getForceSpecialistCount(eIndex) != iNewValue)
	{
		m_paiForceSpecialistCount[eIndex] = std::max(0, iNewValue);

		if (isCitySelected())
		{
			gDLL->getInterfaceIFace()->setDirty(Help_DIRTY_BIT, true);
		}

		AI_setAssignWorkDirty(true);
	}
}


void CvCity::changeForceSpecialistCount(SpecialistTypes eIndex, int iChange)
{
	setForceSpecialistCount(eIndex, (getForceSpecialistCount(eIndex) + iChange));
}


int CvCity::getFreeSpecialistCount(SpecialistTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumSpecialistInfos(), "eIndex expected to be < GC.getNumSpecialistInfos()");
	return m_paiFreeSpecialistCount[eIndex];
}

int CvCity::getAddedFreeSpecialistCount(SpecialistTypes eIndex) const
{
	int iNumAddedSpecialists = getFreeSpecialistCount(eIndex);

	for (int iJ = 0; iJ < GC.getNumBuildingInfos(); ++iJ)
	{
		CvBuildingInfo& kBuilding = GC.getBuildingInfo((BuildingTypes)iJ);
		if (kBuilding.getFreeSpecialistCount(eIndex) > 0)
		{
			iNumAddedSpecialists -= getNumActiveBuilding((BuildingTypes)iJ) * kBuilding.getFreeSpecialistCount(eIndex);
		}
	}

	FAssert(iNumAddedSpecialists >= 0);
	return std::max(0, iNumAddedSpecialists);
}

void CvCity::setFreeSpecialistCount(SpecialistTypes eIndex, int iNewValue)
{
	int iOldValue;

	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumSpecialistInfos(), "eIndex expected to be < GC.getNumSpecialistInfos()");

	iOldValue = getFreeSpecialistCount(eIndex);

	if (iOldValue != iNewValue)
	{
		m_paiFreeSpecialistCount[eIndex] = iNewValue;
		FAssert(getFreeSpecialistCount(eIndex) >= 0);

		changeNumGreatPeople(iNewValue - iOldValue);
		processSpecialist(eIndex, (iNewValue - iOldValue));

		if (isCitySelected())
		{
			gDLL->getInterfaceIFace()->setDirty(CitizenButtons_DIRTY_BIT, true);
		}
	}
}

void CvCity::changeFreeSpecialistCount(SpecialistTypes eIndex, int iChange)
{
	setFreeSpecialistCount(eIndex, (getFreeSpecialistCount(eIndex) + iChange));
}

int CvCity::getImprovementFreeSpecialists(ImprovementTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumImprovementInfos(), "eIndex expected to be < GC.getNumImprovementInfos()");
	return m_paiImprovementFreeSpecialists[eIndex];
}

void CvCity::changeImprovementFreeSpecialists(ImprovementTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumImprovementInfos(), "eIndex expected to be < GC.getNumImprovementInfos()");

	if (iChange != 0)
	{
		m_paiImprovementFreeSpecialists[eIndex] = std::max(0, (m_paiImprovementFreeSpecialists[eIndex] + iChange));
	}
}

int CvCity::getReligionInfluence(ReligionTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumReligionInfos(), "eIndex expected to be < GC.getNumReligionInfos()");
	return m_paiReligionInfluence[eIndex];
}


void CvCity::changeReligionInfluence(ReligionTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumReligionInfos(), "eIndex expected to be < GC.getNumReligionInfos()");
	m_paiReligionInfluence[eIndex] = (m_paiReligionInfluence[eIndex] + iChange);
	FAssert(getReligionInfluence(eIndex) >= 0);
}


int CvCity::getCurrentStateReligionHappiness() const
{
	if (GET_PLAYER(getOwnerINLINE()).getStateReligion() != NO_RELIGION)
	{
		return getStateReligionHappiness(GET_PLAYER(getOwnerINLINE()).getStateReligion());
	}

	return 0;
}


int CvCity::getStateReligionHappiness(ReligionTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumReligionInfos(), "eIndex expected to be < GC.getNumReligionInfos()");
	return m_paiStateReligionHappiness[eIndex];
}


void CvCity::changeStateReligionHappiness(ReligionTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumReligionInfos(), "eIndex expected to be < GC.getNumReligionInfos()");

	if (iChange != 0)
	{
		m_paiStateReligionHappiness[eIndex] = (m_paiStateReligionHappiness[eIndex] + iChange);

		AI_setAssignWorkDirty(true);
	}
}


int CvCity::getUnitCombatFreeExperience(UnitCombatTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumUnitCombatInfos(), "eIndex expected to be < GC.getNumUnitCombatInfos()");
	return m_paiUnitCombatFreeExperience[eIndex];
}


void CvCity::changeUnitCombatFreeExperience(UnitCombatTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumUnitCombatInfos(), "eIndex expected to be < GC.getNumUnitCombatInfos()");
	m_paiUnitCombatFreeExperience[eIndex] = (m_paiUnitCombatFreeExperience[eIndex] + iChange);
	FAssert(getUnitCombatFreeExperience(eIndex) >= 0);
}


int CvCity::getFreePromotionCount(PromotionTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumPromotionInfos(), "eIndex expected to be < GC.getNumPromotionInfos()");
	return m_paiFreePromotionCount[eIndex];
}


bool CvCity::isFreePromotion(PromotionTypes eIndex) const
{
	return (getFreePromotionCount(eIndex) > 0);
}


void CvCity::changeFreePromotionCount(PromotionTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumPromotionInfos(), "eIndex expected to be < GC.getNumPromotionInfos()");
	m_paiFreePromotionCount[eIndex] = (m_paiFreePromotionCount[eIndex] + iChange);
	FAssert(getFreePromotionCount(eIndex) >= 0);
}


int CvCity::getSpecialistFreeExperience() const
{
	return m_iSpecialistFreeExperience;
}


void CvCity::changeSpecialistFreeExperience(int iChange)
{
	m_iSpecialistFreeExperience += iChange;
	FAssert(m_iSpecialistFreeExperience >= 0);
}


int CvCity::getEspionageDefenseModifier() const
{
	return m_iEspionageDefenseModifier;
}


void CvCity::changeEspionageDefenseModifier(int iChange)
{
	if (0 != iChange)
	{
		m_iEspionageDefenseModifier += iChange;
	}
}

bool CvCity::isWorkingPlot(int iIndex) const
{
	FAssertMsg(iIndex >= 0, "iIndex expected to be >= 0");
	FAssertMsg(iIndex < NUM_CITY_PLOTS, "iIndex expected to be < NUM_CITY_PLOTS");

	return m_pabWorkingPlot[iIndex];
}


bool CvCity::isWorkingPlot(const CvPlot* pPlot) const
{
	int iIndex;

	iIndex = getCityPlotIndex(pPlot);

	if (iIndex != -1)
	{
		return isWorkingPlot(iIndex);
	}

	return false;
}


void CvCity::setWorkingPlot(int iIndex, bool bNewValue)
{
	FAssertMsg(iIndex >= 0, "iIndex expected to be >= 0");
	FAssertMsg(iIndex < NUM_CITY_PLOTS, "iIndex expected to be < NUM_CITY_PLOTS");

	if (isWorkingPlot(iIndex) == bNewValue)
		return;
	// <advc.064b> To avoid unnecessary update of city screen
	bool bSelected = isCitySelected();
	int iOldTurns = -1;
	if(bSelected && GET_PLAYER(getOwnerINLINE()).canGoldRush())
		iOldTurns = getProductionTurnsLeft();
	// </advc.064b>
	m_pabWorkingPlot[iIndex] = bNewValue;

	CvPlot* pPlot = getCityIndexPlot(iIndex);

	if (pPlot != NULL)
	{
		FAssertMsg(pPlot->getWorkingCity() == this, "WorkingCity is expected to be this");

		if (isWorkingPlot(iIndex))
		{
			if (iIndex != CITY_HOME_PLOT)
			{
				changeWorkingPopulation(1);
			}

			for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
			{
				changeBaseYieldRate(((YieldTypes)iI), pPlot->getYield((YieldTypes)iI));
			}

			// update plot builder special case where a plot is being worked but is (a) unimproved  or (b) un-bonus'ed
			pPlot->updatePlotBuilder();
		}
		else
		{
			if (iIndex != CITY_HOME_PLOT)
			{
				changeWorkingPopulation(-1);
			}

			for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
			{
				changeBaseYieldRate(((YieldTypes)iI), -(pPlot->getYield((YieldTypes)iI)));
			}
		}

		if ((getTeam() == GC.getGameINLINE().getActiveTeam()) || GC.getGameINLINE().isDebugMode())
		{
			pPlot->updateSymbolDisplay();
		}
	}

	if (bSelected)
	{
		gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true );
		gDLL->getInterfaceIFace()->setDirty(CityScreen_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(ColoredPlots_DIRTY_BIT, true);
		// <advc.064b>
		if(iOldTurns >= 0 && getProductionTurnsLeft() != iOldTurns)
			gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
		// </advc.064b>
	}
}


void CvCity::setWorkingPlot(CvPlot* pPlot, bool bNewValue)
{
	setWorkingPlot(getCityPlotIndex(pPlot), bNewValue);
}


void CvCity::alterWorkingPlot(int iIndex)
{
	FAssertMsg(iIndex >= 0, "iIndex expected to be >= 0");
	FAssertMsg(iIndex < NUM_CITY_PLOTS, "iIndex expected to be < NUM_CITY_PLOTS");

	if (iIndex == CITY_HOME_PLOT)
	{
		setCitizensAutomated(true);
	}
	else
	{
		CvPlot* pPlot = getCityIndexPlot(iIndex);

		if (pPlot != NULL)
		{
			if (canWork(pPlot))
			{
				setCitizensAutomated(false);

				if (isWorkingPlot(iIndex))
				{
					setWorkingPlot(iIndex, false);

					if (GC.getDefineINT("DEFAULT_SPECIALIST") != NO_SPECIALIST)
					{
						changeSpecialistCount(((SpecialistTypes)GC.getDefineINT("DEFAULT_SPECIALIST")), 1);
					}
					else
					{
						AI_addBestCitizen(false, true);
					}
				}
				else
				{
					if ((extraPopulation() > 0) || AI_removeWorstCitizen())
					{
						setWorkingPlot(iIndex, true);
					}
				}
			}
			else if (pPlot->getOwnerINLINE() == getOwnerINLINE())
			{
				pPlot->setWorkingCityOverride(this);
			}
		}
	}
}


int CvCity::getNumRealBuilding(BuildingTypes eIndex) const									 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex expected to be < GC.getNumBuildingInfos()");
	return m_paiNumRealBuilding[eIndex];
}


void CvCity::setNumRealBuilding(BuildingTypes eIndex, int iNewValue)
{
	setNumRealBuildingTimed(eIndex, iNewValue, true, getOwnerINLINE(), GC.getGameINLINE().getGameTurnYear());
}


void CvCity::setNumRealBuildingTimed(BuildingTypes eIndex, int iNewValue, bool bFirst, PlayerTypes eOriginalOwner, int iOriginalTime)
{
	CvCity* pLoopCity;
	CvWString szBuffer;
	int iI;

	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex expected to be < GC.getNumBuildingInfos()");

	int iChangeNumRealBuilding = iNewValue - getNumRealBuilding(eIndex);

	if(iChangeNumRealBuilding == 0)
		return;

	int iOldNumBuilding = getNumBuilding(eIndex);

	m_paiNumRealBuilding[eIndex] = iNewValue;

	if (getNumRealBuilding(eIndex) > 0)
	{
		m_paiBuildingOriginalOwner[eIndex] = eOriginalOwner;
		m_paiBuildingOriginalTime[eIndex] = iOriginalTime;
	}
	else
	{
		m_paiBuildingOriginalOwner[eIndex] = NO_PLAYER;
		m_paiBuildingOriginalTime[eIndex] = MIN_INT;
	}

	if (iOldNumBuilding != getNumBuilding(eIndex))
	{
		if (getNumRealBuilding(eIndex) > 0)
		{
			if (GC.getBuildingInfo(eIndex).isStateReligion())
			{
				for (iI = 0; iI < GC.getNumVoteSourceInfos(); ++iI)
				{
					if (GC.getBuildingInfo(eIndex).getVoteSourceType() == (VoteSourceTypes)iI)
					{
						if (GC.getGameINLINE().getVoteSourceReligion((VoteSourceTypes)iI) == NO_RELIGION)
						{
							FAssert(GET_PLAYER(getOwnerINLINE()).getStateReligion() != NO_RELIGION);
							GC.getGameINLINE().setVoteSourceReligion((VoteSourceTypes)iI, GET_PLAYER(getOwnerINLINE()).getStateReligion(), true);
						}
					}
				}
			}
		}

		processBuilding(eIndex, getNumBuilding(eIndex) - iOldNumBuilding);
	}

	if (!GC.getBuildingClassInfo((BuildingClassTypes)(GC.getBuildingInfo(eIndex).getBuildingClassType())).isNoLimit())
	{
		if (isWorldWonderClass((BuildingClassTypes)(GC.getBuildingInfo(eIndex).getBuildingClassType())))
		{
			changeNumWorldWonders(iChangeNumRealBuilding);
		}
		else if (isTeamWonderClass((BuildingClassTypes)(GC.getBuildingInfo(eIndex).getBuildingClassType())))
		{
			changeNumTeamWonders(iChangeNumRealBuilding);
		}
		else if (isNationalWonderClass((BuildingClassTypes)(GC.getBuildingInfo(eIndex).getBuildingClassType())))
		{
			changeNumNationalWonders(iChangeNumRealBuilding);
		}
		else
		{
			changeNumBuildings(iChangeNumRealBuilding);
		}
	}

	if (iChangeNumRealBuilding > 0)
	{
		if (bFirst)
		{
			if (GC.getBuildingInfo(eIndex).isCapital())
			{
				GET_PLAYER(getOwnerINLINE()).setCapitalCity(this);
			}

			if (GC.getGameINLINE().isFinalInitialized() && !(gDLL->GetWorldBuilderMode()))
			{
				if (GC.getBuildingInfo(eIndex).isGoldenAge())
				{
					GET_PLAYER(getOwnerINLINE()).changeGoldenAgeTurns(iChangeNumRealBuilding * (GET_PLAYER(getOwnerINLINE()).getGoldenAgeLength() + 1));
				}

				if (GC.getBuildingInfo(eIndex).getGlobalPopulationChange() != 0)
				{
					for (iI = 0; iI < MAX_PLAYERS; iI++)
					{
						if (GET_PLAYER((PlayerTypes)iI).isAlive())
						{
							if (GET_PLAYER((PlayerTypes)iI).getTeam() == getTeam())
							{
								if (GC.getBuildingInfo(eIndex).isTeamShare() || (iI == getOwnerINLINE()))
								{	int iLoop=-1;
									for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
									{
										pLoopCity->setPopulation(std::max(1, (pLoopCity->getPopulation() + iChangeNumRealBuilding * GC.getBuildingInfo(eIndex).getGlobalPopulationChange())));
										// so subsequent cities don't starve with the extra citizen working nothing
										pLoopCity->AI_updateAssignWork();
									}
								}
							}
						}
					}
				}

				for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
				{
					if (GC.getBuildingInfo(eIndex).getReligionChange(iI) > 0)
					{
						setHasReligion(((ReligionTypes)iI), true, true, true);
					}
				}

				if (GC.getBuildingInfo(eIndex).getFreeTechs() > 0)
				{
					if (!isHuman())
					{
						for (iI = 0; iI < GC.getBuildingInfo(eIndex).getFreeTechs(); iI++)
						{
							for (int iLoop = 0; iLoop < iChangeNumRealBuilding; iLoop++)
							{
								GET_PLAYER(getOwnerINLINE()).AI_chooseFreeTech();
							}
						}
					}
					else
					{
						szBuffer = gDLL->getText( // <advc.008e>
								::needsArticle(eIndex) ?
								"TXT_KEY_MISC_COMPLETED_WONDER_CHOOSE_TECH_THE" :
								"TXT_KEY_MISC_COMPLETED_WONDER_CHOOSE_TECH",
								// </advc.008e>
								GC.getBuildingInfo(eIndex).getTextKeyWide());
						GET_PLAYER(getOwnerINLINE()).chooseTech(GC.getBuildingInfo(eIndex).getFreeTechs() * iChangeNumRealBuilding, szBuffer.GetCString());
					}
				}

				if (isWorldWonderClass((BuildingClassTypes)(GC.getBuildingInfo(eIndex).getBuildingClassType())))
				{
					szBuffer = gDLL->getText( // <advc.008e>
							::needsArticle(eIndex) ?
							"TXT_KEY_MISC_COMPLETES_WONDER_THE" :
							"TXT_KEY_MISC_COMPLETES_WONDER", // </advc.008e>
							GET_PLAYER(getOwnerINLINE()).getNameKey(), GC.getBuildingInfo(eIndex).getTextKeyWide());
					GC.getGameINLINE().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getOwnerINLINE(), szBuffer, getX_INLINE(), getY_INLINE(), (ColorTypes)GC.getInfoTypeForString("COLOR_BUILDING_TEXT"));

					// <advc.106>
					for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
						CvPlayer& kMsgTarget = GET_PLAYER((PlayerTypes)i);
						if(!kMsgTarget.isAlive())
							continue;
						bool bRevealed = isRevealed(kMsgTarget.getTeam(), true);
						bool bMet = TEAMREF(kMsgTarget.getID()).isHasMet(getTeam());
						if(bRevealed /* advc.127: */ || kMsgTarget.isSpectator())
							// <advc.008e>
							szBuffer = gDLL->getText(::needsArticle(eIndex) ?
									"TXT_KEY_MISC_WONDER_COMPLETED_CITY_THE" :
									"TXT_KEY_MISC_WONDER_COMPLETED_CITY", // </advc.008e>
									GET_PLAYER(getOwnerINLINE()).getNameKey(),
									GC.getBuildingInfo(eIndex).getTextKeyWide(),
									getName().GetCString());
						else if(bMet) {
							// <advc.008e>
							szBuffer = gDLL->getText(::needsArticle(eIndex) ?
									"TXT_KEY_MISC_WONDER_COMPLETED_THE" :
									"TXT_KEY_MISC_WONDER_COMPLETED", // </advc.008e>
									GET_PLAYER(getOwnerINLINE()).getNameKey(),
									GC.getBuildingInfo(eIndex).getTextKeyWide());
						}
						else { // <advc.008>
							szBuffer = gDLL->getText(::needsArticle(eIndex) ?
									"TXT_KEY_MISC_WONDER_COMPLETED_UNKNOWN_THE" :
									"TXT_KEY_MISC_WONDER_COMPLETED_UNKNOWN",
									GC.getBuildingInfo(eIndex).getTextKeyWide());
						}
						gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)i), false,
								GC.getEVENT_MESSAGE_TIME(), szBuffer,
								"AS2D_WONDER_BUILDING_BUILD",
								MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY, // advc.106b
								GC.getBuildingInfo(eIndex).getArtInfo()->getButton(),
								(ColorTypes)GC.getInfoTypeForString("COLOR_BUILDING_TEXT"),
								// Indicate location only if revealed.
								bRevealed ? getX_INLINE() : -1, bRevealed ? getY_INLINE() : -1,
								bRevealed, bRevealed);
					} // </advc.106>
				}
			}

			if (GC.getBuildingInfo(eIndex).isAllowsNukes())
			{
				GC.getGameINLINE().makeNukesValid(true);
			}

			GC.getGameINLINE().incrementBuildingClassCreatedCount((BuildingClassTypes)(GC.getBuildingInfo(eIndex).getBuildingClassType()));
		}
	}

	//great wall
	if (bFirst)
	{
		if (GC.getBuildingInfo(eIndex).isAreaBorderObstacle()
			/*  advc.310: Show GW as 3D model even when playing w/o Barbarians
				(BorderObstacle ability disabled through CvInfos then). */
				|| GC.getBuildingInfo(eIndex).getAreaTradeRoutes() > 0)
		{
			int iCountExisting = 0;
			for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
			{
				if (eIndex != iI && GC.getBuildingInfo((BuildingTypes)iI).isAreaBorderObstacle())
				{
					iCountExisting += getNumRealBuilding((BuildingTypes)iI);
				}
			}

			if (iCountExisting == 1 && iNewValue == 0)
			{
				gDLL->getEngineIFace()->RemoveGreatWall(this);
			}
			else if (iCountExisting == 0 && iNewValue > 0)
			{
				gDLL->getEngineIFace()->AddGreatWall(this);
			}
		}
	}
}


int CvCity::getNumFreeBuilding(BuildingTypes eIndex) const													 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex expected to be < GC.getNumBuildingInfos()");
	return m_paiNumFreeBuilding[eIndex];
}


void CvCity::setNumFreeBuilding(BuildingTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex expected to be < GC.getNumBuildingInfos()");

	if(getNumFreeBuilding(eIndex) == iNewValue)
		return;

	int iOldNumBuilding = getNumBuilding(eIndex);

	m_paiNumFreeBuilding[eIndex] = iNewValue;

	if (iOldNumBuilding != getNumBuilding(eIndex))
	{
		processBuilding(eIndex, iNewValue - iOldNumBuilding);
	}
}


bool CvCity::isHasReligion(ReligionTypes eIndex) const													 
{
	FASSERT_BOUNDS(0, GC.getNumReligionInfos(), eIndex, "CvCity::isHasReligion");
	return m_pabHasReligion[eIndex];
}


void CvCity::setHasReligion(ReligionTypes eIndex, bool bNewValue, bool bAnnounce, bool bArrows,
		PlayerTypes eSpreadPlayer) // advc.106e
{
	FASSERT_BOUNDS(0, GC.getNumReligionInfos(), eIndex, "CvCity::setHasReligion");

	if (isHasReligion(eIndex) == bNewValue)
		return; // advc.003

	for (int iVoteSource = 0; iVoteSource < GC.getNumVoteSourceInfos(); iVoteSource++)
		processVoteSourceBonus((VoteSourceTypes)iVoteSource, false);

	m_pabHasReligion[eIndex] = bNewValue;

	for (int iVoteSource = 0; iVoteSource < GC.getNumVoteSourceInfos(); iVoteSource++)
		processVoteSourceBonus((VoteSourceTypes)iVoteSource, true);

	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	kOwner.changeHasReligionCount(eIndex, isHasReligion(eIndex) ? 1 : -1);
	updateMaintenance();
	updateReligionHappiness();
	updateReligionCommerce();

	AI_setAssignWorkDirty(true);
	setInfoDirty(true);
	// <advc.106e>
	int iOwnerEra = kOwner.getCurrentEra();
	int iEraThresh = GC.getDefineINT("STOP_RELIGION_SPREAD_ANNOUNCE_ERA");
	bool bAnnounceStateReligionSpread = (GC.getDefineINT(
			"ANNOUNCE_STATE_RELIGION_SPREAD") > 0);
	// </advc.106e>
	if (isHasReligion(eIndex))
	{
		CvGame& g = GC.getGameINLINE();
		g.makeReligionFounded(eIndex, kOwner.getID());
		if (bAnnounce)
		{
			if (g.getHolyCity(eIndex) != this)
			{
				for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
				{
					CvPlayer const& kObs = GET_PLAYER((PlayerTypes)iI);
					if (!kObs.isAlive() || !isRevealed(kObs.getTeam(), false))
						continue; // advc.003
					// <advc.106e>
					if ((iOwnerEra < iEraThresh && (bAnnounceStateReligionSpread ||
							kOwner.getStateReligion() != eIndex)) ||
							eSpreadPlayer == kObs.getID() || // </advc.106e>
							kOwner.getID() == kObs.getID() ||
							// advc.106e: Disable this clause
							//(kObs.getStateReligion() == eIndex) ||
							kObs.hasHolyCity(eIndex))
					{
						CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_RELIGION_SPREAD",
								GC.getReligionInfo(eIndex).getTextKeyWide(), getNameKey());
						gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(), false,
								GC.getEVENT_MESSAGE_TIME(), // (K-Mod note: event time was originally "long".)
								szBuffer, GC.getReligionInfo(eIndex).getSound(),
								MESSAGE_TYPE_MINOR_EVENT, // advc.106b: was MAJOR
								GC.getReligionInfo(eIndex).getButton(),
								(ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"),
								getX_INLINE(), getY_INLINE(), bArrows, bArrows);
					}
				}
			}
			if (isHuman() && kOwner.getHasReligionCount(eIndex) == 1)
			{
				if (kOwner.canConvert(eIndex) && kOwner.getStateReligion() == NO_RELIGION)
				{
					CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_CHANGERELIGION);
					if (pInfo != NULL)
					{
						pInfo->setData1(eIndex);
						gDLL->getInterfaceIFace()->addPopup(pInfo, kOwner.getID());
					}
				} /* <advc.004w> Update text of resource indicators
					 (CvGameTextMgr::setBonusExtraHelp) */
				if(kOwner.getID() == g.getActivePlayer() && g.getCurrentLayer() == GLOBE_LAYER_RESOURCE)
				{
					gDLL->getInterfaceIFace()->setDirty(GlobeLayer_DIRTY_BIT, true);
					// advc.003p:
					kOwner.setBonusHelpDirty();
				} // </advc.004w>
			}
		}
	} // K-Mod start
	else // religion removed
	{
		if (bAnnounce)
		{	// <advc.106e> Announce removed religion to other civs as well
			for (int i = 0; i < MAX_CIV_PLAYERS; i++)
			{
				CvPlayer const& kObs = GET_PLAYER((PlayerTypes)i);
				if(!kObs.isAlive() || !isRevealed(kObs.getTeam(), false))
					continue;
				if(eSpreadPlayer != kObs.getID() && iOwnerEra >= iEraThresh &&
						kOwner.getID() != kObs.getID() && !kObs.hasHolyCity(eIndex))
					continue; // </advc.106e>
				CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_RELIGION_REMOVE", GC.getReligionInfo(eIndex).getTextKeyWide(), getNameKey());
				gDLL->getInterfaceIFace()->addHumanMessage(
						kObs.getID(), // advc.106e: was getOwner() in K-Mod
						false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_BLIGHT",
						MESSAGE_TYPE_MINOR_EVENT, // advc.106b: was MAJOR
						GC.getReligionInfo(eIndex).getButton(),
						(ColorTypes)GC.getInfoTypeForString("COLOR_RED"),
						getX_INLINE(), getY_INLINE(), bArrows, bArrows);
			}
		}
	} // K-Mod end
	if (bNewValue)
	{
		CvEventReporter::getInstance().religionSpread(eIndex, kOwner.getID(), this);
		// <advc.130n>
		for(int i = 0; i < MAX_CIV_TEAMS; i++) {
			CvTeamAI& t = GET_TEAM((TeamTypes)i);
			if(t.isAlive() && isRevealed(t.getID(), false))
				t.AI_reportNewReligion(eIndex);
		} // </advc.130n>
	}
	else CvEventReporter::getInstance().religionRemove(eIndex, kOwner.getID(), this);
}

// K-Mod. A rating for how strong a religion can take hold in this city
int CvCity::getReligionGrip(ReligionTypes eReligion) const
{
	PROFILE_FUNC();
	if (!GC.getGameINLINE().isReligionFounded(eReligion))
		return 0;

	int iScore = 0;

	if (isHasReligion(eReligion))
	{
		int iTempScore = GC.getDefineINT("RELIGION_INFLUENCE_POPULATION_WEIGHT") * getPopulation();
		// <advc.099c> Try to satisfy foreign pop
		if(revoltProbability(true, false, true) > 0) {
			PlayerTypes cultOwner = calculateCulturalOwner();
			if(GET_PLAYER(cultOwner).getStateReligion() == eReligion)
				iTempScore *= 2;
		}
		iScore += iTempScore; // </advc.099c>
		if (GET_PLAYER(getOwnerINLINE()).getStateReligion() == eReligion)
		{
			iScore += GC.getDefineINT("RELIGION_INFLUENCE_STATE_RELIGION_WEIGHT");
		}
	}

	for (int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		if (GC.getBuildingInfo((BuildingTypes)iI).getPrereqReligion() == eReligion)
		{
			iScore += GC.getDefineINT("RELIGION_INFLUENCE_BUILDING_WEIGHT") * getNumActiveBuilding((BuildingTypes)iI);
		}
	}

	CvCity* pHolyCity = GC.getGameINLINE().getHolyCity(eReligion);
	if (pHolyCity && isConnectedTo(pHolyCity))
	{
		if (pHolyCity->hasShrine(eReligion))
			iScore += GC.getDefineINT("RELIGION_INFLUENCE_SHRINE_WEIGHT");

		int iDistance = plotDistance(getX_INLINE(), getY_INLINE(), pHolyCity->getX_INLINE(), pHolyCity->getY_INLINE());
		iScore += GC.getDefineINT("RELIGION_INFLUENCE_DISTANCE_WEIGHT") * (GC.getMapINLINE().maxPlotDistance() - iDistance) / GC.getMapINLINE().maxPlotDistance();
	}

	int iCurrentTurn = GC.getGameINLINE().getGameTurn();
	int iTurnFounded = GC.getGameINLINE().getReligionGameTurnFounded(eReligion);
	int iTimeScale = GC.getDefineINT("RELIGION_INFLUENCE_TIME_SCALE")*GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getVictoryDelayPercent()/100;
	iScore += GC.getDefineINT("RELIGION_INFLUENCE_TIME_WEIGHT") * (iTurnFounded + iTimeScale) / (iCurrentTurn + iTimeScale);

	return iScore; // note. the random part is not included in this function.
}
// K-Mod end

void CvCity::processVoteSourceBonus(VoteSourceTypes eVoteSource, bool bActive)
{
	if (!GET_PLAYER(getOwnerINLINE()).isLoyalMember(eVoteSource))
	{
		return;
	}

	if (GC.getGameINLINE().isDiploVote(eVoteSource))
	{
		ReligionTypes eReligion = GC.getGameINLINE().getVoteSourceReligion(eVoteSource);

		SpecialistTypes eSpecialist = (SpecialistTypes)GC.getVoteSourceInfo(eVoteSource).getFreeSpecialist();
		if (NO_SPECIALIST != eSpecialist)
		{
			if (NO_RELIGION == eReligion || isHasReligion(eReligion))
			{
				changeFreeSpecialistCount(eSpecialist, bActive ? 1 : -1);
			}
		}

		if (NO_RELIGION != eReligion && isHasReligion(eReligion))
		{
			for (int iYield = 0; iYield < NUM_YIELD_TYPES; ++iYield)
			{
				int iChange = GC.getVoteSourceInfo(eVoteSource).getReligionYield(iYield);
				if (!bActive)
				{
					iChange = -iChange;
				}

				if (0 != iChange)
				{
					for (int iBuilding = 0; iBuilding < GC.getNumBuildingInfos(); ++iBuilding)
					{
						if (GC.getBuildingInfo((BuildingTypes)iBuilding).getReligionType() == eReligion)
						{
							changeBuildingYieldChange((BuildingClassTypes)GC.getBuildingInfo((BuildingTypes)iBuilding).getBuildingClassType(), (YieldTypes)iYield, iChange);
						}
					}
				}
			}

			for (int iCommerce = 0; iCommerce < NUM_COMMERCE_TYPES; ++iCommerce)
			{
				int iChange = GC.getVoteSourceInfo(eVoteSource).getReligionCommerce(iCommerce);
				if (!bActive)
				{
					iChange = -iChange;
				}

				if (0 != iChange)
				{
					for (int iBuilding = 0; iBuilding < GC.getNumBuildingInfos(); ++iBuilding)
					{
						if (GC.getBuildingInfo((BuildingTypes)iBuilding).getReligionType() == eReligion)
						{
							changeBuildingCommerceChange((BuildingClassTypes)GC.getBuildingInfo((BuildingTypes)iBuilding).getBuildingClassType(), (CommerceTypes)iCommerce, iChange);
						}
					}
				}
			}
		}
	}
}


bool CvCity::isHasCorporation(CorporationTypes eIndex) const													 
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumCorporationInfos(), "eIndex expected to be < GC.getNumCorporationInfos()");
	return m_pabHasCorporation[eIndex];
}


void CvCity::setHasCorporation(CorporationTypes eIndex, bool bNewValue, bool bAnnounce, bool bArrows)
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < GC.getNumCorporationInfos(), "eIndex expected to be < GC.getNumCorporationInfos()");

	if (isHasCorporation(eIndex) != bNewValue)
	{
		if (bNewValue)
		{
			bool bReplacedHeadquarters = false;
			for (int iCorp = 0; iCorp < GC.getNumCorporationInfos(); ++iCorp)
			{
				if (iCorp != eIndex && isHasCorporation((CorporationTypes)iCorp))
				{
					if (GC.getGameINLINE().isCompetingCorporation((CorporationTypes)iCorp, eIndex))
					{
						if (GC.getGameINLINE().getHeadquarters((CorporationTypes)iCorp) == this)
						{
							GC.getGameINLINE().replaceCorporation((CorporationTypes)iCorp, eIndex);
							bReplacedHeadquarters = true;
						}
						else
						{
							setHasCorporation((CorporationTypes)iCorp, false, false);
						}
					}
				}
			}

			if (bReplacedHeadquarters)
			{
				return; // already set the corporation in this city
			}
		}

		m_pabHasCorporation[eIndex] = bNewValue;

		GET_PLAYER(getOwnerINLINE()).changeHasCorporationCount(eIndex, ((isHasCorporation(eIndex)) ? 1 : -1));

		CvCity* pHeadquarters = GC.getGameINLINE().getHeadquarters(eIndex);

		if (NULL != pHeadquarters)
		{
			pHeadquarters->updateCorporation();
		}

		updateCorporation();

		AI_setAssignWorkDirty(true);

		setInfoDirty(true);

		if (isHasCorporation(eIndex))
		{
			GC.getGameINLINE().makeCorporationFounded(eIndex, getOwnerINLINE());
		}

		if (bAnnounce)
		{
			for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++) // advc.003: No msg to barbs
			{
				CvPlayer const& civ = GET_PLAYER((PlayerTypes)iI);
				if(!civ.isAlive())
					continue; // advc.003
				// <advc.106e>
				if(civ.hasHeadquarters(eIndex))
					plot()->setRevealed(civ.getTeam(), true, false, NO_TEAM, false);
				//if (getOwnerINLINE() == iI || GET_PLAYER((PlayerTypes)iI).hasHeadquarters(eIndex)) // BtS
				if(isRevealed(civ.getTeam(), false)) // </advc.106e>
				{
					if (getOwnerINLINE() == iI)
					{
						CvWStringBuffer szBonusString;
						GAMETEXT.setCorporationHelpCity(szBonusString, eIndex, this);

						CvWString szBonusList;
						bool bFirst = true;
						for (int iJ = 0; iJ < GC.getDefineINT("NUM_CORPORATION_PREREQ_BONUSES"); ++iJ)
						{
							int iBonus = GC.getCorporationInfo(eIndex).getPrereqBonus(iJ);
							if (iBonus != NO_BONUS)
							{
								CvWString szTemp;
								szTemp.Format(L"%s", GC.getBonusInfo((BonusTypes)iBonus).getDescription());
								setListHelp(szBonusList, L"", szTemp, L", ", bFirst);
								bFirst = false;
							}
						}

						CvWString szBuffer;
						szBuffer = gDLL->getText("TXT_KEY_MISC_CORPORATION_SPREAD_BONUS", GC.getCorporationInfo(eIndex).getTextKeyWide(), szBonusString.getCString(), getNameKey(), szBonusList.GetCString());
						gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, GC.getCorporationInfo(eIndex).getSound(), MESSAGE_TYPE_MINOR_EVENT, GC.getCorporationInfo(eIndex).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"), getX_INLINE(), getY_INLINE(), bArrows, bArrows);
					}
					// K-Mod. We don't need two announcements every time a corp spreads. So I've put the general announcement inside this 'else' block.
					else
					{
						CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_CORPORATION_SPREAD", GC.getCorporationInfo(eIndex).getTextKeyWide(), getNameKey());
						gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, GC.getCorporationInfo(eIndex).getSound(),
								MESSAGE_TYPE_MINOR_EVENT, // advc.106b: was MAJOR
								GC.getCorporationInfo(eIndex).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"), getX_INLINE(), getY_INLINE(), bArrows, bArrows);
					}
					// K-Mod end
				}
			}
		}

		if (bNewValue)
		{
			// Python Event
			CvEventReporter::getInstance().corporationSpread(eIndex, getOwnerINLINE(), this);
		}
		else
		{
			// Python Event
			CvEventReporter::getInstance().corporationRemove(eIndex, getOwnerINLINE(), this);
		}
	}
}


CvCity* CvCity::getTradeCity(int iIndex) const
{
	FAssert(iIndex >= 0);
	FAssert(iIndex < GC.getDefineINT("MAX_TRADE_ROUTES"));
	return getCity(m_paTradeCities[iIndex]);
}


int CvCity::getTradeRoutes() const
{
	/*  <advc.123e> 0 trade routes result in 0 profit from plundering; see
		CvUnit::collectBlockadeGold */
	if(getOwnerINLINE() == BARBARIAN_PLAYER)
		return 0; // </advc.123e>

	int iTradeRoutes = GC.getGameINLINE().getTradeRoutes();
	iTradeRoutes += GET_PLAYER(getOwnerINLINE()).getTradeRoutes();
	// advc.310: Continental TR no longer included in player TR
	iTradeRoutes += area()->getTradeRoutes(getOwnerINLINE());
	if (isCoastal())
		iTradeRoutes += GET_PLAYER(getOwnerINLINE()).getCoastalTradeRoutes();
	iTradeRoutes += getExtraTradeRoutes();

	return std::min(iTradeRoutes, GC.getDefineINT("MAX_TRADE_ROUTES"));
}


void CvCity::clearTradeRoutes()
{
	for (int iI = 0; iI < GC.getDefineINT("MAX_TRADE_ROUTES"); iI++)
	{
		CvCity* pLoopCity = getTradeCity(iI);

		if (pLoopCity != NULL)
		{
			pLoopCity->setTradeRoute(getOwnerINLINE(), false);
		}

		m_paTradeCities[iI].reset();
	}
}


// XXX eventually, this needs to be done when roads are built/destroyed...
void CvCity::updateTradeRoutes() // advc.003: refactored
{
	int const iMaxTradeRoutes = GC.getDefineINT("MAX_TRADE_ROUTES");
	CvPlayer const& kOwner = GET_PLAYER(getOwnerINLINE());

	clearTradeRoutes();

	if (!isDisorder() && !isPlundered())
	{
		int iTradeRoutes = getTradeRoutes();
		bool bIgnorePlotGroups = (GC.getDefineINT("IGNORE_PLOT_GROUP_FOR_TRADE_ROUTES") > 0);
		FAssert(iTradeRoutes <= iMaxTradeRoutes);
		int* paiBestValue = new int[iMaxTradeRoutes](); // value-initialize
		for(int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			CvPlayer const& kPartner = GET_PLAYER((PlayerTypes)iI);
			if(!kOwner.canHaveTradeRoutesWith(kPartner.getID()))
				continue;
			int iLoop;
			for(CvCity* pLoopCity = kPartner.firstCity(&iLoop);
					pLoopCity != NULL; pLoopCity = kPartner.nextCity(&iLoop))
			{
				if(pLoopCity == this)
					continue;
				/*  <advc.124> A connection along revealed tiles ensures that the
					city tile is revealed, but this doesn't imply that the city is
					also revealed: The tile could've been explored before the city
					existed. Need to check CvCity::isRevealed explicitly. */
				if(pLoopCity->isDisorder() || !pLoopCity->isRevealed(getTeam(), false))
					continue; // <advc.124>
				if(pLoopCity->isTradeRoute(kOwner.getID()) && getTeam() != kPartner.getTeam())
					continue;
				if(!bIgnorePlotGroups && pLoopCity->plotGroup(kOwner.getID()) !=
						plotGroup(kOwner.getID()))
					continue;
				int iValue = calculateTradeProfit(pLoopCity);
				for (int iJ = 0; iJ < iTradeRoutes; iJ++)
				{
					if(iValue <= paiBestValue[iJ])
						continue;
					for (int iK = iTradeRoutes - 1; iK > iJ; iK--)
					{
						paiBestValue[iK] = paiBestValue[(iK - 1)];
						m_paTradeCities[iK] = m_paTradeCities[(iK - 1)];
					}
					paiBestValue[iJ] = iValue;
					m_paTradeCities[iJ] = pLoopCity->getIDInfo();
					break;
				}
			}
		}
		SAFE_DELETE_ARRAY(paiBestValue);
	}

	int iTradeProfit = 0;
	for (int iI = 0; iI < iMaxTradeRoutes; iI++)
	{
		CvCity* pLoopCity = getTradeCity(iI);
		if (pLoopCity != NULL)
		{
			pLoopCity->setTradeRoute(kOwner.getID(), true);
			iTradeProfit += calculateTradeProfit(pLoopCity);
		}
	}

	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{	
		YieldTypes eYield = (YieldTypes)iI;
		// XXX could take this out if handled when CvPlotGroup changes...
		setTradeYield(eYield, calculateTradeYield(eYield, iTradeProfit));
	}
}


void CvCity::clearOrderQueue()
{
	while (headOrderQueueNode() != NULL)
	{
		popOrder(0);
	}

	if ((getTeam() == GC.getGameINLINE().getActiveTeam()) || GC.getGameINLINE().isDebugMode())
	{
		setInfoDirty(true);
	}
}


void CvCity::pushOrder(OrderTypes eOrder, int iData1, int iData2, bool bSave, bool bPop, int iPosition, bool bForce)
{
	OrderData order;
	bool bBuildingUnit = false;
	bool bBuildingBuilding = false;

	if (bPop)
	{
		popOrder(0);
	}

	bool bValid = false;

	switch (eOrder)
	{
	case ORDER_TRAIN:
		if (canTrain((UnitTypes)iData1) || bForce)
		{
			if (iData2 == -1)
			{
				iData2 = GC.getUnitInfo((UnitTypes)iData1).getDefaultUnitAIType();
			}

			GET_PLAYER(getOwnerINLINE()).changeUnitClassMaking(((UnitClassTypes)(GC.getUnitInfo((UnitTypes) iData1).getUnitClassType())), 1);

			area()->changeNumTrainAIUnits(getOwnerINLINE(), ((UnitAITypes)iData2), 1);
			GET_PLAYER(getOwnerINLINE()).AI_changeNumTrainAIUnits(((UnitAITypes)iData2), 1);

			bValid = true;
			bBuildingUnit = true;
			CvEventReporter::getInstance().cityBuildingUnit(this, (UnitTypes)iData1);
			if( gCityLogLevel >= 1 )
			{
				CvWString szString;
				getUnitAIString(szString, (UnitAITypes)iData2);
				logBBAI("    City %S pushes production of unit %S with UNITAI %S", getName().GetCString(), GC.getUnitInfo((UnitTypes) iData1).getDescription(), szString.GetCString() );
			}
		}
		break;

	case ORDER_CONSTRUCT:
		if (canConstruct((BuildingTypes)iData1) || bForce)
		{
			GET_PLAYER(getOwnerINLINE()).changeBuildingClassMaking(((BuildingClassTypes)(GC.getBuildingInfo((BuildingTypes) iData1).getBuildingClassType())), 1);

			bValid = true;
			bBuildingBuilding = true;
			CvEventReporter::getInstance().cityBuildingBuilding(this, (BuildingTypes)iData1);
			if( gCityLogLevel >= 1 )
				logBBAI("    City %S pushes production of building %S", getName().GetCString(), GC.getBuildingInfo((BuildingTypes)iData1).getDescription() );
		}
		break;

	case ORDER_CREATE:
		if (canCreate((ProjectTypes)iData1) || bForce)
		{
			GET_TEAM(getTeam()).changeProjectMaking(((ProjectTypes)iData1), 1);

			bValid = true;
			if( gCityLogLevel >= 1 )
				logBBAI("    City %S pushes production of project %S", getName().GetCString(), GC.getProjectInfo((ProjectTypes)iData1).getDescription() );
		}
		break;

	case ORDER_MAINTAIN:
		if (canMaintain((ProcessTypes)iData1) || bForce)
		{
			bValid = true;
			// K-Mod. For culture processes, use iData2 to flag the current culture level so that we know when to stop.
			// We could do a similar thing with research processes and tech... but lets not.
			if (isHuman() && GC.getProcessInfo((ProcessTypes)iData1).getProductionToCommerceModifier(COMMERCE_CULTURE) > 0)
			{
				FAssert(iData2 == -1);
				iData2 = getCultureLevel();
			}
			// K-Mod end
			if( gCityLogLevel >= 1 )
				logBBAI("    City %S pushes production of process %S", getName().GetCString(), GC.getProcessInfo((ProcessTypes)iData1).getDescription() );
		}
		break;

	default:
		FAssertMsg(false, "iOrder did not match a valid option");
		break;
	}

	if (!bValid)
	{
		return;
	}
	bool bWasEmpty = (m_orderQueue.getLength() == 0); // advc.004x
	order.eOrderType = eOrder;
	order.iData1 = iData1;
	order.iData2 = iData2;
	order.bSave = bSave;
	
	/* original bts code
	if (bAppend)
		m_orderQueue.insertAtEnd(order);
	else {
		stopHeadOrder();
		m_orderQueue.insertAtBeginning(order);
	}
	if (!bAppend || (getOrderQueueLength() == 1))
		startHeadOrder();*/
	// K-Mod
	if (iPosition == 0 || getOrderQueueLength() == 0)
	{
		stopHeadOrder();
		m_orderQueue.insertAtBeginning(order);
		startHeadOrder();
	}
	else if (iPosition < 0 || iPosition >= getOrderQueueLength())
		m_orderQueue.insertAtEnd(order);
	else
		m_orderQueue.insertBefore(order, m_orderQueue.nodeNum(iPosition));
	// K-Mod end

	// Why does this cause a crash???

/*	if (bBuildingUnit)
	{
		CvEventReporter::getInstance().cityBuildingUnit(this, (UnitTypes)iData1);
	}
	else if (bBuildingBuilding)
	{
		CvEventReporter::getInstance().cityBuildingBuilding(this, (BuildingTypes)iData1);
	}*/

	if ((getTeam() == GC.getGameINLINE().getActiveTeam()) || GC.getGameINLINE().isDebugMode())
	{
		setInfoDirty(true);

		if (isCitySelected())
		{
			gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true );
			gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(CityScreen_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(PlotListButtons_DIRTY_BIT, true);
		}
	} // <advc.004x>
	if(bWasEmpty && getOwnerINLINE() == GC.getGameINLINE().getActivePlayer())
		GET_PLAYER(getOwnerINLINE()).killAll(BUTTONPOPUP_CHOOSEPRODUCTION, getID());
	// </advc.004x>
}


void CvCity::popOrder(int iNum, bool bFinish, bool bChoose)
{
	wchar szBuffer[1024];
	wchar szTempBuffer[1024];
	TCHAR szSound[1024];
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE()); // advc.003
	bool bWasFoodProduction = isFoodProduction();

	if (iNum == -1)
		iNum = (getOrderQueueLength() - 1);

	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();
	{	// advc.003: scope for iCount
		int iCount = 0;
		while (pOrderNode != NULL)
		{
			if (iCount == iNum)
			{
				break;
			}

			iCount++;

			pOrderNode = nextOrderQueueNode(pOrderNode);
		}
	}
	if (pOrderNode == NULL)
	{
		return;
	}

	if (bFinish && pOrderNode->m_data.bSave)
	{
		//pushOrder(pOrderNode->m_data.eOrderType, pOrderNode->m_data.iData1, pOrderNode->m_data.iData2, true, false, true);
		pushOrder(pOrderNode->m_data.eOrderType, pOrderNode->m_data.iData1, pOrderNode->m_data.iData2, true, false, -1);
	}

	UnitTypes eTrainUnit = NO_UNIT;
	BuildingTypes eConstructBuilding = NO_BUILDING;
	ProjectTypes eCreateProject = NO_PROJECT;
	int maxedBuildingOrProject = NO_BUILDING; // advc.123f

	OrderTypes eOrderType = pOrderNode->m_data.eOrderType;
	switch(eOrderType) { // advc.003: Many style changes in this block
	case ORDER_TRAIN: {
		eTrainUnit = (UnitTypes)pOrderNode->m_data.iData1;
		UnitAITypes eTrainAIUnit = (UnitAITypes)pOrderNode->m_data.iData2;
		FAssertMsg(eTrainUnit != NO_UNIT, "eTrainUnit is expected to be assigned a valid unit type");
		FAssertMsg(eTrainAIUnit != NO_UNITAI, "eTrainAIUnit is expected to be assigned a valid unit AI type");
		kOwner.changeUnitClassMaking((UnitClassTypes)GC.getUnitInfo(eTrainUnit).getUnitClassType(), -1);
		area()->changeNumTrainAIUnits(getOwnerINLINE(), eTrainAIUnit, -1);
		kOwner.AI_changeNumTrainAIUnits(eTrainAIUnit, -1);
		doPopOrder(pOrderNode); // advc.064d (see case ORDER_CONSTRUCT)
		if(!bFinish)
			break;
		// <advc.064b> Moved into new function
		handleOverflow(getUnitProduction(eTrainUnit) - getProductionNeeded(eTrainUnit),
				getProductionModifier(eTrainUnit), eOrderType);
		// 14 March 2019: K-Mod multi-production code removed (including bugfix advc.001v)
		// Instead restored (two lines):
		setUnitProduction(eTrainUnit, 0);
		setUnitProductionTime(eTrainUnit, 0); // EmperorFool, Bugfix, 06/10/10
		// </advc.064b>
		CvUnit* pUnit = kOwner.initUnit(eTrainUnit, getX_INLINE(), getY_INLINE(), eTrainAIUnit);
		FAssertMsg(pUnit != NULL, "pUnit is expected to be assigned a valid unit object");
		pUnit->finishMoves();
		addProductionExperience(pUnit);
		CvPlot* pRallyPlot = getRallyPlot();
		if(pRallyPlot != NULL)
			pUnit->getGroup()->pushMission(MISSION_MOVE_TO, pRallyPlot->getX_INLINE(), pRallyPlot->getY_INLINE());
		if(isHuman()) {
			if(kOwner.isOption(PLAYEROPTION_START_AUTOMATED))
				pUnit->automate(AUTOMATE_BUILD);
			if(kOwner.isOption(PLAYEROPTION_MISSIONARIES_AUTOMATED))
				pUnit->automate(AUTOMATE_RELIGION);
		}
		CvEventReporter::getInstance().unitBuilt(this, pUnit);
		if(gCityLogLevel >= 1 ) { // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
			CvWString szString; getUnitAIString(szString, pUnit->AI_getUnitAIType());
			logBBAI("    City %S finishes production of unit %S with UNITAI %S", getName().GetCString(), pUnit->getName(0).GetCString(), szString.GetCString() );
		}
		CvUnitInfo const& kUnitInfo = GC.getUnitInfo(eTrainUnit);
		if(kUnitInfo.getDomainType() == DOMAIN_AIR) {
			if(plot()->countNumAirUnits(getTeam()) > getAirUnitCapacity(getTeam()))
				pUnit->jumpToNearestValidPlot();  // can destroy unit
		}
		break;
	}
	case ORDER_CONSTRUCT: {
		eConstructBuilding = (BuildingTypes)(pOrderNode->m_data.iData1);
		BuildingClassTypes bct = (BuildingClassTypes)GC.getBuildingInfo(
				eConstructBuilding).getBuildingClassType();
		kOwner.changeBuildingClassMaking(bct, -1);
		/*  advc.064d: processBuilding may now call verifyProduction. Important that
			the constructed building is no longer in the queue at that point. */
		doPopOrder(pOrderNode);
		if(!bFinish)
			break;
		if (kOwner.isBuildingClassMaxedOut(bct,
				// UNOFFICIAL_PATCH, Bugfix, 10/08/09, davidlallen & jdog5000: 
				GC.getBuildingClassInfo(bct).getExtraPlayerInstances()))
			kOwner.removeBuildingClass(bct);
		setNumRealBuilding(eConstructBuilding, getNumRealBuilding(eConstructBuilding) + 1);
		// <advc.064b> Moved into new function
		handleOverflow(getBuildingProduction(eConstructBuilding) - getProductionNeeded(eConstructBuilding),
				getProductionModifier(eConstructBuilding), eOrderType);
		// </advc.064b>
		setBuildingProduction(eConstructBuilding, 0);
		setBuildingProductionTime(eConstructBuilding, 0); // Bugfix, 06/10/10, EmperorFool
		// <advc.123f>
		if(::isWorldWonderClass(bct) &&
				GC.getGameINLINE().isBuildingClassMaxedOut(bct))
			maxedBuildingOrProject = eConstructBuilding; // </advc.123f>
		CvEventReporter::getInstance().buildingBuilt(this, eConstructBuilding);
		if( gCityLogLevel >= 1 ) // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
			logBBAI("    City %S finishes production of building %S", getName().GetCString(), GC.getBuildingInfo(eConstructBuilding).getDescription() );
		break;
	}
	case ORDER_CREATE: {
		eCreateProject = (ProjectTypes)pOrderNode->m_data.iData1;
		GET_TEAM(getTeam()).changeProjectMaking(eCreateProject, -1);
		doPopOrder(pOrderNode); // advc.064d
		if(!bFinish)
			break;
		// Event reported to Python before the project is built, so that we can show the movie before awarding free techs, for example
		CvEventReporter::getInstance().projectBuilt(this, eCreateProject);
		GET_TEAM(getTeam()).changeProjectCount(eCreateProject, 1);
		if (GC.getProjectInfo(eCreateProject).isSpaceship())
		{
			bool needsArtType = true;
			VictoryTypes eVictory = (VictoryTypes)GC.getProjectInfo(eCreateProject).getVictoryPrereq();
			if(NO_VICTORY != eVictory && GET_TEAM(getTeam()).canLaunch(eVictory)) {
				if(isHuman()) {
					CvPopupInfo* pInfo = NULL;
					if (GC.getGameINLINE().isNetworkMultiPlayer()) {
						pInfo = new CvPopupInfo(BUTTONPOPUP_LAUNCH,
								GC.getProjectInfo(eCreateProject).getVictoryPrereq());
					}
					else {
						pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN, eCreateProject);
						pInfo->setText(L"showSpaceShip");
						needsArtType = false;
					}
					gDLL->getInterfaceIFace()->addPopup(pInfo, getOwnerINLINE());
				}
				else kOwner.AI_launch(eVictory);
			}
			else { //show the spaceship progress
				if(isHuman() &&
						// advc.060:
						getBugOptionBOOL("TechWindow__ShowSSScreen", false)) {
					if(!GC.getGameINLINE().isNetworkMultiPlayer()) {
						CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_PYTHON_SCREEN, eCreateProject);
						pInfo->setText(L"showSpaceShip");
						gDLL->getInterfaceIFace()->addPopup(pInfo, getOwnerINLINE());
						needsArtType = false;
					}
				}
			}
			if(needsArtType) {
				int defaultArtType = GET_TEAM(getTeam()).getProjectDefaultArtType(eCreateProject);
				int projectCount = GET_TEAM(getTeam()).getProjectCount(eCreateProject);
				GET_TEAM(getTeam()).setProjectArtType(eCreateProject, projectCount - 1, defaultArtType);
			}
		}
		// <advc.064b> Moved into new function
		handleOverflow(getProjectProduction(eCreateProject) - getProductionNeeded(eCreateProject),
				getProductionModifier(eCreateProject), eOrderType);
		// </advc.064b>
		setProjectProduction(eCreateProject, 0);
		// <advc.123f>
		if(GC.getGameINLINE().isProjectMaxedOut(eCreateProject))
			maxedBuildingOrProject = eCreateProject; // </advc.123f>
		if( gCityLogLevel >= 1 ) // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
			logBBAI("    City %S finishes production of project %S", getName().GetCString(), GC.getProjectInfo(eCreateProject).getDescription() );
		break;
	}
	case ORDER_MAINTAIN:
		doPopOrder(pOrderNode); // advc.064d
		break;

	default:
		FAssert(false);
		doPopOrder(pOrderNode); // advc.064d
		break;
	}
	/*  advc.064d: (BtS code moved into auxiliary function doPopOrder; called
		from within the switch block)  */
	pOrderNode = NULL; // for safety
	// <advc.123f> Fail gold from great wonders and world projects
	if(maxedBuildingOrProject != NO_BUILDING) { int foo=-1;
		bool bProject = (eOrderType == ORDER_CREATE);
		ProjectTypes pt = (bProject ? (ProjectTypes)maxedBuildingOrProject :
				NO_PROJECT);
		BuildingTypes bt = (bProject ? NO_BUILDING :
				(BuildingTypes)maxedBuildingOrProject);
		BuildingClassTypes bct = (bProject ? NO_BUILDINGCLASS :
				(BuildingClassTypes)GC.getBuildingInfo(bt).getBuildingClassType());
		for(int i = 0; i < MAX_PLAYERS; i++) {
			if(i == getOwnerINLINE()) // No fail gold for the city owner
				continue;
			CvPlayer& p = GET_PLAYER((PlayerTypes)i);
			if(!p.isAlive())
				continue;
			// Just for efficiency
			if(!bProject && p.getBuildingClassMaking(bct) <= 0)
				continue;
			for(CvCity* c = p.firstCity(&foo); c != NULL; c = p.nextCity(&foo)) {
				// Fail gold only for queued orders
				/*  If a mod-mod allows e.g. a wonder with up to 2 instances
					and p is building 2 instances in parallel when this city
					finishes 1 instance, abort only 1 of p's instances:
					the one with less production invested. */
				CvCity* pMinProductionCity = NULL;
				int iMinProduction = 0;
				for(int j = 0; j < c->getOrderQueueLength(); j++) {
					OrderData* od = c->getOrderFromQueue(j);
					if(od == NULL || od->eOrderType != (bProject ? ORDER_CREATE :
							ORDER_CONSTRUCT) ||
							od->iData1 != maxedBuildingOrProject)
						continue;
					int productionInvested = (bProject ?
							c->getProjectProduction(pt) :
							c->getBuildingProduction(bt));
					if(productionInvested > iMinProduction) {
						iMinProduction = productionInvested;
						pMinProductionCity = c;
					}
				}
				if(pMinProductionCity != NULL) {
					// 0 fail gold for teammates
					if(p.getTeam() == getTeam())
						iMinProduction = 0;
					/*  No fail gold for overflow
						(Tbd.: Add the overflow to OverflowProduction?) */
					iMinProduction = std::min(iMinProduction,
							bProject ? getProductionNeeded(pt) :
							getProductionNeeded(bt));
					pMinProductionCity->failProduction(maxedBuildingOrProject,
							iMinProduction, bProject);
				}
			}
		}
	} // </advc.123f>

	if (getTeam() == GC.getGameINLINE().getActiveTeam() || GC.getGameINLINE().isDebugMode())
	{
		setInfoDirty(true);

		if (isCitySelected())
		{
			gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true );
			gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(CityScreen_DIRTY_BIT, true);
		}
	}

	bool bMessage = false;

	if (bChoose)
	{
		if (getOrderQueueLength() == 0)
		{
			if (!isHuman() || isProductionAutomated())
			{
				AI_chooseProduction();
			}
			else
			{
				if (bWasFoodProduction)
				{
					AI_assignWorkingPlots();
				}

				chooseProduction(eTrainUnit, eConstructBuilding, eCreateProject, bFinish);
				// <advc.004x> Remember the order in case the popup needs to be delayed
				mrWasUnit = (eTrainUnit != NO_UNIT);
				if(mrWasUnit)
					mrOrder = eTrainUnit;
				else if(eCreateProject != NO_PROJECT)
					mrOrder = GC.getNumBuildingInfos() + eCreateProject;
				else mrOrder = eConstructBuilding; // </advc.004x>
				bMessage = true;
			}
		}
	}

	LPCSTR szIcon = NULL;

	if (bFinish && !bMessage)
	{
		if (eTrainUnit != NO_UNIT)
		{
			swprintf(szBuffer, gDLL->getText(
					isLimitedUnitClass((UnitClassTypes)
					GC.getUnitInfo(eTrainUnit).getUnitClassType()) ?
					"TXT_KEY_MISC_TRAINED_UNIT_IN_LIMITED" :
					"TXT_KEY_MISC_TRAINED_UNIT_IN",
					GC.getUnitInfo(eTrainUnit).getTextKeyWide(),
					getNameKey()).GetCString());
			strcpy(szSound, GC.getUnitInfo(eTrainUnit).getArtInfo(
					0, kOwner.getCurrentEra(), NO_UNIT_ARTSTYLE)->getTrainSound());
			szIcon = kOwner.getUnitButton(eTrainUnit);
		}
		else if (eConstructBuilding != NO_BUILDING)
		{
			swprintf(szBuffer, gDLL->getText(((isLimitedWonderClass((BuildingClassTypes)(GC.getBuildingInfo(eConstructBuilding).getBuildingClassType()))) ? "TXT_KEY_MISC_CONSTRUCTED_BUILD_IN_LIMITED" : "TXT_KEY_MISC_CONSTRUCTED_BUILD_IN"), GC.getBuildingInfo(eConstructBuilding).getTextKeyWide(), getNameKey()).GetCString());
			strcpy(szSound, GC.getBuildingInfo(eConstructBuilding).getConstructSound());
			szIcon = GC.getBuildingInfo(eConstructBuilding).getButton();
		}
		else if (eCreateProject != NO_PROJECT)
		{
			swprintf(szBuffer, gDLL->getText(((isLimitedProject(eCreateProject)) ?
					// <advc.008e>
					(::needsArticle(eCreateProject) ?
					"TXT_KEY_MISC_CREATED_PROJECT_IN_LIMITED_THE" :
					"TXT_KEY_MISC_CREATED_PROJECT_IN_LIMITED") // </advc.008e>
					: "TXT_KEY_MISC_CREATED_PROJECT_IN"), GC.getProjectInfo(eCreateProject).getTextKeyWide(), getNameKey()).GetCString());
			strcpy(szSound, GC.getProjectInfo(eCreateProject).getCreateSound());
			szIcon = GC.getProjectInfo(eCreateProject).getButton();
		}
		if (isProduction())
		{
			swprintf(szTempBuffer, gDLL->getText((isProductionLimited() ? "TXT_KEY_MISC_WORK_HAS_BEGUN_LIMITED" : "TXT_KEY_MISC_WORK_HAS_BEGUN"), getProductionNameKey()).GetCString());
			wcscat(szBuffer, szTempBuffer);
		}
		gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, szSound,
				MESSAGE_TYPE_INFO, // advc.106b: was MINOR_EVENT
				szIcon, (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"), getX_INLINE(), getY_INLINE(), true, true);
	}

	if (getTeam() == GC.getGameINLINE().getActiveTeam() || GC.getGameINLINE().isDebugMode())
	{
		setInfoDirty(true);

		if (isCitySelected())
		{
			gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true );
			gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(CityScreen_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(PlotListButtons_DIRTY_BIT, true);
		}
	}
}


void CvCity::startHeadOrder()
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		if (pOrderNode->m_data.eOrderType == ORDER_MAINTAIN)
		{
			processProcess(((ProcessTypes)(pOrderNode->m_data.iData1)), 1);
		}

		AI_setAssignWorkDirty(true);
	}
}


void CvCity::stopHeadOrder()
{
	CLLNode<OrderData>* pOrderNode = headOrderQueueNode();

	if (pOrderNode != NULL)
	{
		if (pOrderNode->m_data.eOrderType == ORDER_MAINTAIN)
		{
			processProcess(((ProcessTypes)(pOrderNode->m_data.iData1)), -1);
		}
	}
}


int CvCity::getOrderQueueLength()
{
	return m_orderQueue.getLength();
}


OrderData* CvCity::getOrderFromQueue(int iIndex)
{
	CLLNode<OrderData>* pOrderNode;

	pOrderNode = m_orderQueue.nodeNum(iIndex);

	if (pOrderNode != NULL)
	{
		return &(pOrderNode->m_data);
	}
	else
	{
		return NULL;
	}
}


CLLNode<OrderData>* CvCity::nextOrderQueueNode(CLLNode<OrderData>* pNode) const
{
	return m_orderQueue.next(pNode);
}


CLLNode<OrderData>* CvCity::headOrderQueueNode() const
{
	return m_orderQueue.head();
}

int CvCity::getNumOrdersQueued() const
{
	return m_orderQueue.getLength();
}

OrderData CvCity::getOrderData(int iIndex) const
{
	int iCount = 0;
	CLLNode<OrderData>* pNode = headOrderQueueNode();
	while (pNode != NULL)
	{
		if (iIndex == iCount)
		{
			return pNode->m_data;
		}
		iCount++;
		pNode = nextOrderQueueNode(pNode);
	}
	OrderData kData;
	kData.eOrderType = NO_ORDER;
	kData.iData1 = -1;
	kData.iData2 = -1;
	kData.bSave = false;
	return kData;
}

void CvCity::setWallOverridePoints(const std::vector< std::pair<float, float> >& kPoints)
{
	m_kWallOverridePoints = kPoints;
	setLayoutDirty(true);
}


const std::vector< std::pair<float, float> >& CvCity::getWallOverridePoints() const
{
	return m_kWallOverridePoints;
}

// Protected Functions...

void CvCity::doGrowth()
{
	int iDiff;

	if (GC.getUSE_DO_GROWTH_CALLBACK()) { // K-Mod. block unused python callbacks
		CyCity* pyCity = new CyCity(this); CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyCity));	// pass in city class
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "doGrowth", argsList.makeFunctionArgs(), &lResult);
		delete pyCity;	// python fxn must not hold on to this pointer 
		if (lResult == 1)
			return;
	}

	iDiff = foodDifference();

	changeFood(iDiff);
	if(iDiff > 0) // advc.160: Don't empty the Granary when insufficient food
		changeFoodKept(iDiff);

	setFoodKept(range(getFoodKept(), 0, ((growthThreshold() * getMaxFoodKeptPercent()) / 100)));

	if (getFood() >= growthThreshold())
	{
		if (AI_isEmphasizeAvoidGrowth())
		{
			setFood(growthThreshold());
		}
		else
		{
			changeFood(-(std::max(0, (growthThreshold() - getFoodKept()))));
			changePopulation(1);

			// ONEVENT - City growth
			CvEventReporter::getInstance().cityGrowth(this, getOwnerINLINE());
		}
	}
	else if (getFood() < 0)
	{
		changeFood(-(getFood()));

		if (getPopulation() > 1)
		{
			changePopulation(-1);
			// <advc.160>
			changeFood(getFoodKept());
			setFoodKept(0); // </advc.160>
		}
	}
}


void CvCity::doCulture()
{
	if (GC.getUSE_DO_CULTURE_CALLBACK()) // K-Mod. block unused python callbacks
	{
		CyCity* pyCity = new CyCity(this);
		CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyCity));	// pass in city class
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "doCulture", argsList.makeFunctionArgs(), &lResult);
		delete pyCity;	// python fxn must not hold on to this pointer 
		if (lResult == 1)
		{
			return;
		}
	} // <advc.099b>
	if(isOccupation())
		return; // </advc.099b>
	/*  K-Mod, 26/sep/10, 31/oct/10, Karadoc
		Trade culture: START */
	int iI;
	int iLevel = getCultureLevel();
	if (iLevel > 0)
	{	// advc.125:
		int iUseKModTradeCulture = GC.getDefineINT("USE_KMOD_TRADE_CULTURE");
		// add up the culture contribution for each player before applying it
		// so that we avoid excessive calls to change culture and reduce rounding errors
		int iTradeCultureTimes100[MAX_PLAYERS] = {};

		for (iI = 0; iI < GC.getDefineINT("MAX_TRADE_ROUTES"); iI++)
		{
			CvCity* pLoopCity = getTradeCity(iI);
			if(pLoopCity != NULL)
			{	// foreign and domestic
				//if (pLoopCity->getOwnerINLINE() != getOwnerINLINE())
				{
					iTradeCultureTimes100[pLoopCity->getOwnerINLINE()]+= pLoopCity->getTradeCultureRateTimes100(iLevel);
				}
			}
		}
		for (iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (iTradeCultureTimes100[iI] > 0)
			{
				// <advc.125>
				if((iUseKModTradeCulture < 0 &&
						plot()->getCulture((PlayerTypes)iI) == 0) ||
						iUseKModTradeCulture == 0)
					iTradeCultureTimes100[iI] = 0; // </advc.125>
				// plot culture only.
				//changeCultureTimes100((PlayerTypes)iI, iTradeCultureTimes100[iI], false, false);
				doPlotCultureTimes100(false, (PlayerTypes)iI, iTradeCultureTimes100[iI], false);
			}
		}
	} // K-Mod END
	changeCultureTimes100(getOwnerINLINE(), getCommerceRateTimes100(COMMERCE_CULTURE), false, true);
}

// This function has essentially been rewritten for K-Mod. (and it used to not be 'times 100')
// A note about scale: the city plot itself gets roughly 10x culture. The outer edges of the cultural influence get 1x culture (ie. the influence that extends beyond the borders).
void CvCity::doPlotCultureTimes100(bool bUpdate, PlayerTypes ePlayer, int iCultureRateTimes100, bool bCityCulture)  // advc.003: some style changes
{
	CultureLevelTypes eCultureLevel = (CultureLevelTypes)0;

	if (GC.getUSE_DO_PLOT_CULTURE_CALLBACK()) // K-Mod. block unused python callbacks
	{
		CyCity* pyCity = new CyCity(this);
		CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyCity));	// pass in city class
		argsList.add(bUpdate);
		argsList.add(ePlayer);
		//argsList.add(iCultureRate);
		argsList.add(iCultureRateTimes100/100); // K-Mod
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "doPlotCulture", argsList.makeFunctionArgs(), &lResult);
		delete pyCity;	// python fxn must not hold on to this pointer 
		if (lResult == 1)
		{
			return;
		}
	}

	FAssert(NO_PLAYER != ePlayer);

	if (getOwnerINLINE() == ePlayer)
	{
		eCultureLevel = getCultureLevel();
	}
	else
	{
		for (int iI = (GC.getNumCultureLevelInfos() - 1); iI > 0; iI--)
		{
			if (getCultureTimes100(ePlayer) >= 100 * GC.getGameINLINE().getCultureThreshold((CultureLevelTypes)iI))
			{
				eCultureLevel = (CultureLevelTypes)iI;
				break;
			}
		}
	}

	/*  K-Mod, 30/oct/10, Karadoc
	increased culture range, added a percentage based distance bonus (decreasing the importance flat rate bonus). */
	// (original bts code deleted)

	// Experimental culture profile...
	// Ae^(-bx). A = 10 (no effect), b = log(full_range_ratio)/range
	// (iScale-1)(iDistance - iRange)^2/(iRange^2) + 1   // This approximates the exponential pretty well
	const int iScale = 10;
	const int iCultureRange = eCultureLevel + 3;

	//const int iOuterRatio = 10;
	//const double iB = log((double)iOuterRatio)/iCultureRange;

	// free culture bonus for cities
	iCultureRateTimes100+= bCityCulture ? 400 : 0;

	// note, original code had "if (getCultureTimes100(ePlayer) > 0)". I took that part out.
	if (eCultureLevel == NO_CULTURELEVEL || // <advc.003> Inverted some conditions
			(std::abs(iCultureRateTimes100*iScale) < 100 && !bCityCulture))
		return;
	// <advc.025>
	int iCultureToMaster = 100;
	if(GET_TEAM(getTeam()).isCapitulated())
		iCultureToMaster = GC.getDefineINT("CAPITULATED_TO_MASTER_CULTURE_PERCENT");
	// </advc.025>
	for (int iDX = -iCultureRange; iDX <= iCultureRange; iDX++)
	{
		for (int iDY = -iCultureRange; iDY <= iCultureRange; iDY++)
		{
			int iDistance = cultureDistance(iDX, iDY);
			if(iDistance > iCultureRange)
				continue; 
			CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iDX, iDY);
			if(pLoopPlot == NULL || !pLoopPlot->isPotentialCityWorkForArea(area()))
				continue;
			//int iCultureToAdd = iCultureRateTimes100*((iScale-1)*(iDistance-iCultureRange)*(iDistance-iCultureRange) + iCultureRange*iCultureRange)/(100*iCultureRange*iCultureRange);
			/*  <advc.001> Deleted some old K-Mod code that had been commented out.
				The line above was the most recent K-Mod code. Causes an integer
				overflow when a large amount of culture is added through the
				WorldBuilder (e.g. 50000). Corrected below. (Also fixed in K-Mod 1.46.) */
			double dCultureToAdd = iCultureRateTimes100 /
					(100.0*iCultureRange*iCultureRange);
			int iDelta = iDistance-iCultureRange;
			dCultureToAdd *= (iScale-1)*iDelta*iDelta + iCultureRange*iCultureRange;
			int iCultureToAdd = ::round(dCultureToAdd); // </advc.001>
			// <advc.025>
			if(iCultureToMaster != 100 && pLoopPlot->getTeam() != getTeam() &&
					pLoopPlot->getTeam() == GET_TEAM(getTeam()).getMasterTeam())
				iCultureToAdd = (iCultureToAdd * iCultureToMaster) / 100;
			// </advc.025>
			// <dlph.23> Loss of tile culture upon city trade
			if(iCultureToAdd < 0) {
				FAssert(iCultureRateTimes100 < 0);
				int iPlotCulture = pLoopPlot->getCulture(ePlayer);
				iCultureToAdd = -std::min(-iCultureToAdd, iPlotCulture);
			} // </dlph.23>
			pLoopPlot->changeCulture(ePlayer, iCultureToAdd, (bUpdate || !(pLoopPlot->isOwned())));
		}
	}
	// K-Mod end
}

bool CvCity::doCheckProduction()  // advc.003:some style changes
{
	CvWString szBuffer;
	int iI;
	bool bOK = true;
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	for (iI = 0; iI < GC.getNumUnitInfos(); iI++)
	{
		if (getUnitProduction((UnitTypes)iI) <= 0)
			continue;
		if (kOwner.isProductionMaxedUnitClass((UnitClassTypes)GC.getUnitInfo((UnitTypes)iI).getUnitClassType()))
		{	// advc.123f: Commented out (fail gold from national units)
			/*int iProductionGold = ((getUnitProduction((UnitTypes)iI) * GC.getDefineINT("MAXED_UNIT_GOLD_PERCENT")) / 100);
			if (iProductionGold > 0) {
				owner.changeGold(iProductionGold);
				szBuffer = gDLL->getText("TXT_KEY_MISC_LOST_WONDER_PROD_CONVERTED", getNameKey(), GC.getUnitInfo((UnitTypes)iI).getTextKeyWide(), iProductionGold);
				gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_WONDERGOLD", MESSAGE_TYPE_MINOR_EVENT, GC.getCommerceInfo(COMMERCE_GOLD).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), getX_INLINE(), getY_INLINE(), true, true);
			}*/
			setUnitProduction((UnitTypes)iI, 0);
		}
	}

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		if (getBuildingProduction((BuildingTypes)iI) <= 0)
			continue;
		if (kOwner.isProductionMaxedBuildingClass((BuildingClassTypes)(GC.getBuildingInfo((BuildingTypes)iI).getBuildingClassType())))
		{	// advc.123f: Commented out. Fail gold now handled in popOrder.
			/*iProductionGold = ((getBuildingProduction((BuildingTypes)iI) * GC.getDefineINT("MAXED_BUILDING_GOLD_PERCENT")) / 100);
			if(iProductionGold > 0) {
				owner.changeGold(iProductionGold);
				szBuffer = gDLL->getText("TXT_KEY_MISC_LOST_WONDER_PROD_CONVERTED", getNameKey(), GC.getBuildingInfo((BuildingTypes)iI).getTextKeyWide(), iProductionGold);
				gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_WONDERGOLD", MESSAGE_TYPE_MINOR_EVENT, GC.getCommerceInfo(COMMERCE_GOLD).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), getX_INLINE(), getY_INLINE(), true, true);
			}*/
			setBuildingProduction((BuildingTypes)iI, 0);
		}
	}

	for (iI = 0; iI < GC.getNumProjectInfos(); iI++)
	{
		if (getProjectProduction((ProjectTypes)iI) <= 0)
			continue;
		if (kOwner.isProductionMaxedProject((ProjectTypes)iI))
		{	// advc.123f: Commented out. Fail gold now handled in popOrder.
			/*iProductionGold = ((getProjectProduction((ProjectTypes)iI) * GC.getDefineINT("MAXED_BUILDING_GOLD_PERCENT")) / 100);
			if(iProductionGold > 0) {
				owner.changeGold(iProductionGold);
				szBuffer = gDLL->getText("TXT_KEY_MISC_LOST_WONDER_PROD_CONVERTED", getNameKey(), GC.getProjectInfo((ProjectTypes)iI).getTextKeyWide(), iProductionGold);
				gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_WONDERGOLD", MESSAGE_TYPE_MINOR_EVENT, GC.getCommerceInfo(COMMERCE_GOLD).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), getX_INLINE(), getY_INLINE(), true, true);
			}*/
			setProjectProduction((ProjectTypes)iI, 0);
		}
	}

	if (!isProduction() && !isDisorder() && isHuman() && !isProductionAutomated())
	{
		chooseProduction();
		return bOK;
	}

	for (iI = 0; iI < GC.getNumUnitInfos(); iI++)
	{
		if (getFirstUnitOrder((UnitTypes)iI) == -1)
			continue;

		UnitTypes eUpgradeUnit = allUpgradesAvailable((UnitTypes)iI);
		if (eUpgradeUnit == NO_UNIT)
			continue;
		FAssertMsg(eUpgradeUnit != iI, "eUpgradeUnit is expected to be different from iI");

		int iUpgradeProduction = getUnitProduction((UnitTypes)iI);
		setUnitProduction(((UnitTypes)iI), 0);
		setUnitProduction(eUpgradeUnit, iUpgradeProduction);

		CLLNode<OrderData>* pOrderNode = headOrderQueueNode();
		while (pOrderNode != NULL)
		{
			if (pOrderNode->m_data.eOrderType == ORDER_TRAIN)
			{
				if (pOrderNode->m_data.iData1 == iI)
				{
					kOwner.changeUnitClassMaking((UnitClassTypes)GC.getUnitInfo((UnitTypes)
							pOrderNode->m_data.iData1).getUnitClassType(), -1);
					pOrderNode->m_data.iData1 = eUpgradeUnit;
					if (kOwner.AI_unitValue(eUpgradeUnit, (UnitAITypes)pOrderNode->m_data.iData2, area()) == 0)
					{
						area()->changeNumTrainAIUnits(getOwnerINLINE(), (UnitAITypes)pOrderNode->m_data.iData2, -1);
						kOwner.AI_changeNumTrainAIUnits(((UnitAITypes)pOrderNode->m_data.iData2), -1);
						pOrderNode->m_data.iData2 = GC.getUnitInfo(eUpgradeUnit).getDefaultUnitAIType();
						area()->changeNumTrainAIUnits(getOwnerINLINE(), (UnitAITypes)pOrderNode->m_data.iData2, 1);
						kOwner.AI_changeNumTrainAIUnits((UnitAITypes)pOrderNode->m_data.iData2, 1);
					}
					// advc (note): pOrderNode may have changed
					kOwner.changeUnitClassMaking((UnitClassTypes)GC.getUnitInfo((UnitTypes)
							pOrderNode->m_data.iData1).getUnitClassType(), 1);
				}
			}
			pOrderNode = nextOrderQueueNode(pOrderNode);
		}
	}

	for (iI = (getOrderQueueLength() - 1); iI >= 0; iI--)
	{
		OrderData* pOrder = getOrderFromQueue(iI);
		if (pOrder != NULL)
		{
			if (!canContinueProduction(*pOrder))
			{
				popOrder(iI, false, true);
				bOK = false;
			}
		}
	}
	// <advc.064d>
	if (!isProduction() && !isDisorder())
	{
		if(isHuman() && !isProductionAutomated())
			chooseProduction();
		else AI_chooseProduction();
	} // </advc.064d>

	return bOK;
}


void CvCity::doProduction(bool bAllowNoProduction)
{
	if (GC.getUSE_DO_PRODUCTION_CALLBACK()) { // K-Mod. block unused python callbacks
		CyCity* pyCity = new CyCity(this); CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyCity));	// pass in city class
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "doProduction", argsList.makeFunctionArgs(), &lResult);
		delete pyCity;	// python fxn must not hold on to this pointer 
		if (lResult == 1)
			return;
	}

	if (!isHuman() || isProductionAutomated())
	{
		if (!isProduction() || isProductionProcess() || AI_isChooseProductionDirty())
		{
			AI_chooseProduction();
		}
	}

	if (!bAllowNoProduction && !isProduction())
	{
		return;
	}

	if (isProductionProcess())
	{
		// K-Mod. End the culture process if our borders have expanded.
		// (This function is called after "doResearch" etc.)
		const OrderData& order = headOrderQueueNode()->m_data;
		if (order.iData2 > 0 && GC.getProcessInfo((ProcessTypes)order.iData1).getProductionToCommerceModifier(COMMERCE_CULTURE) > 0 && getCultureLevel() > order.iData2)
		{
			popOrder(0, false, true);
		}
		// K-Mod end
		return;
	}

	if (isDisorder())
	{
		return;
	}

	if (isProduction())
	{
		int iFeatureProductionUsed=0; // advc.064b
		changeProduction(getCurrentProductionDifference(false, true, false,
				false, false, &iFeatureProductionUsed)); // advc.064b
		setOverflowProduction(0);
		//setFeatureProduction(0);
		changeFeatureProduction(-iFeatureProductionUsed); // advc.064b

		if (getProduction() >= getProductionNeeded())
			popOrder(0, true, true);
	}
	else
	{
		changeOverflowProduction(getCurrentProductionDifference(false, false,
				/* advc.064b: */ true), getProductionModifier());
	}
}


void CvCity::doDecay()
{
	int iI;

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		BuildingTypes eBuilding = (BuildingTypes) iI;
		if (getProductionBuilding() != eBuilding)
		{
			if (getBuildingProduction(eBuilding) > 0)
			{
				changeBuildingProductionTime(eBuilding, 1);

				if (isHuman())
				{
					int iGameSpeedPercent = GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getConstructPercent();
					if (100 * getBuildingProductionTime(eBuilding) > GC.getDefineINT("BUILDING_PRODUCTION_DECAY_TIME") * iGameSpeedPercent)
					{
						int iProduction = getBuildingProduction(eBuilding);
						setBuildingProduction(eBuilding, iProduction - (iProduction * (100 - GC.getDefineINT("BUILDING_PRODUCTION_DECAY_PERCENT")) + iGameSpeedPercent - 1) / iGameSpeedPercent);
					}
				}
			}
			else
			{
				setBuildingProductionTime(eBuilding, 0);
			}
		}
	}

	for (iI = 0; iI < GC.getNumUnitInfos(); iI++)
	{
		UnitTypes eUnit = (UnitTypes) iI;
		if (getProductionUnit() != eUnit)
		{
			if (getUnitProduction(eUnit) > 0)
			{
				changeUnitProductionTime(eUnit, 1);

				if (isHuman())
				{
					int iGameSpeedPercent = GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getTrainPercent();
					if (100 * getUnitProductionTime(eUnit) > GC.getDefineINT("UNIT_PRODUCTION_DECAY_TIME") * iGameSpeedPercent)
					{
						int iProduction = getUnitProduction(eUnit);
						setUnitProduction(eUnit, iProduction - (iProduction * (100 - GC.getDefineINT("UNIT_PRODUCTION_DECAY_PERCENT")) + iGameSpeedPercent - 1) / iGameSpeedPercent);
					}
				}
			}
			else
			{
				setUnitProductionTime(eUnit, 0);
			}
		}
	}
}


// K-Mod. I've completely rewritten this function, and deleted the old code.
void CvCity::doReligion()
{
	if (GC.getUSE_DO_RELIGION_CALLBACK()) // K-Mod. block unused python callbacks
	{
		CyCity* pyCity = new CyCity(this);
		CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyCity));	// pass in city class
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "doReligion", argsList.makeFunctionArgs(), &lResult);
		delete pyCity;	// python fxn must not hold on to this pointer 
		if (lResult == 1)
		{
			return;
		}
	}

	// gives some of the top religions a shot at spreading to the city.
	int iChances = 1 + (getCultureLevel() >= 4 ? 1 : 0) + (getPopulation() + 3) / 8 - getReligionCount();
	// (breakpoints at pop = 5, 13, 21, ...)

	if (iChances <= 0)
		return;

	std::vector<std::pair<int, ReligionTypes> > religion_grips;
	ReligionTypes eWeakestReligion = NO_RELIGION; // weakest religion already in the city
	int iWeakestGrip = MAX_INT;
	int iRandomWeight = GC.getDefineINT("RELIGION_INFLUENCE_RANDOM_WEIGHT");
	int iDivisorBase = GC.getDefineINT("RELIGION_SPREAD_DIVISOR_BASE");
	int iDistanceFactor = GC.getDefineINT("RELIGION_SPREAD_DISTANCE_FACTOR");

	for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		if (GC.getGameINLINE().isReligionFounded((ReligionTypes)iI))
		{
			if (isHasReligion((ReligionTypes)iI))
			{
				// if we have this religion; check to see if it weakest one we have...
				if (!isHolyCity((ReligionTypes)iI))
				{
					// ... only if it isn't the holy city.
					int iGrip = getReligionGrip((ReligionTypes)iI);
					if (iGrip < iWeakestGrip)
					{
						iWeakestGrip = iGrip;
						eWeakestReligion = (ReligionTypes)iI;
					}
				}
			}
			else if (!GET_PLAYER(getOwnerINLINE()).isNoNonStateReligionSpread() || GET_PLAYER(getOwnerINLINE()).getStateReligion() == iI)
			{
				// if we don't have the religion, and the religion is allowed to spread here, add it to the list.
				int iGrip = getReligionGrip((ReligionTypes)iI);
				iGrip += GC.getGameINLINE().getSorenRandNum(iRandomWeight/2, "Religion influence"); // only half the weight for self-spread

				religion_grips.push_back(std::make_pair(iGrip, (ReligionTypes)iI));
			}

		}
	}

	iChances = std::min(iChances, (int)religion_grips.size());
	std::partial_sort(religion_grips.begin(), religion_grips.begin()+iChances, religion_grips.end(), std::greater<std::pair<int, ReligionTypes> >());

	for (int i = 0; i < iChances; i++)
	{
		int iLoopGrip = religion_grips[i].first;
		ReligionTypes eLoopReligion = religion_grips[i].second;

		// give up if there is already a stronger religion in the city.
		if (eWeakestReligion != NO_RELIGION && iWeakestGrip >= iLoopGrip)
			break;

		FAssert(eLoopReligion != NO_RELIGION);
		FAssert(!isHasReligion(eLoopReligion));
		FAssert(!GET_PLAYER(getOwnerINLINE()).isNoNonStateReligionSpread() || GET_PLAYER(getOwnerINLINE()).getStateReligion() == eLoopReligion);

		int iRandThreshold = 0;

		for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
		{
			if (GET_PLAYER((PlayerTypes)iJ).isAlive())
			{
				int iLoop;
				for (CvCity* pLoopCity = GET_PLAYER((PlayerTypes)iJ).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iJ).nextCity(&iLoop))
				{
					if (pLoopCity->isConnectedTo(this))
					{
						int iSpread = pLoopCity->getReligionInfluence(eLoopReligion);

						iSpread *= GC.getReligionInfo(eLoopReligion).getSpreadFactor();

						if (iSpread > 0)
						{
							//iSpread /= std::max(1, (((GC.getDefineINT("RELIGION_SPREAD_DISTANCE_DIVISOR") * plotDistance(getX_INLINE(), getY_INLINE(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE())) / GC.getMapINLINE().maxPlotDistance()) - 5));

							// K-Mod. The original formula basically divided the spread by the percent of max distance. (RELIGION_SPREAD_DISTANCE_DIVISOR == 100)
							// In my view, this produced too much spread at short distance, and too little at long.
							int iDivisor = std::max(1, iDivisorBase);

							iDivisor *= 100 + 100 * iDistanceFactor * plotDistance(getX_INLINE(), getY_INLINE(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE()) / GC.getMapINLINE().maxPlotDistance();
							iDivisor /= 100;

							// now iDivisor is in the range [1, 1+iDistanceFactor] * iDivisorBase
							// this is approximately in the range [5, 50], depending on what the xml value are. (the value currently being tested and tuned.)
							iSpread /= iDivisor;
							// K-Mod end

							//iSpread /= (getReligionCount() + 1);

							iRandThreshold = std::max(iRandThreshold, iSpread);
						}
					}
				}
			}
		}

		// scale for game speed
		iRandThreshold *= 100;
		iRandThreshold /= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getVictoryDelayPercent();

		// K-Mod. Give a bonus for the first few cities.
		/* {
			int iReligionCities = GC.getGameINLINE().countReligionLevels(eLoopReligion);
			if (iReligionCities < 3)
			{
				iRandThreshold *= 2 + iReligionCities;
				iRandThreshold /= 1 + iReligionCities;
			}
		} */
		//

		if (GC.getGameINLINE().getSorenRandNum(GC.getDefineINT("RELIGION_SPREAD_RAND"), "Religion Spread") < iRandThreshold)
		{
			setHasReligion(eLoopReligion, true, true, true);
			if (iWeakestGrip < iLoopGrip)
			{
				FAssert(eWeakestReligion != NO_RELIGION);
				// If the existing religion is weak compared to the new religion, the existing religion can get removed.
				int iOdds = getReligionCount()*100*(iLoopGrip - iWeakestGrip) / std::max(1, iLoopGrip);
				if (GC.getGameINLINE().getSorenRandNum(100, "Religion departure") < iOdds)
				{
					setHasReligion(eWeakestReligion, false, true, true);
					break; // end the loop
				}
			}
			else
			{
				iWeakestGrip = iLoopGrip;
				eWeakestReligion = eLoopReligion;
			}
			iChances--;
		}
	}
}


void CvCity::doGreatPeople()
{
	if(GC.getUSE_DO_GREAT_PEOPLE_CALLBACK()) { // K-Mod. block unused python callbacks
		CyCity* pyCity = new CyCity(this);
		CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyCity));	// pass in city class
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "doGreatPeople", argsList.makeFunctionArgs(), &lResult);
		delete pyCity;	// python fxn must not hold on to this pointer 
		if(lResult == 1)
			return;
	}

	if (isDisorder())
	{
		return;
	}
	
	changeGreatPeopleProgress(getGreatPeopleRate());
	// advc.051: Verify that GreatPeopleRate is the sum of the GreatPeopleUnitRates
	int iTotalUnitRate = 0;
	for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
	{
		UnitTypes eUnit = (UnitTypes)iI;
		int iUnitRate = getGreatPeopleUnitRate(eUnit); // advc.051
		changeGreatPeopleUnitProgress(eUnit, iUnitRate);
		iTotalUnitRate += iUnitRate; // advc.051
	}
	FAssert(iTotalUnitRate == getBaseGreatPeopleRate()); // advc.051
	if (getGreatPeopleProgress() >= GET_PLAYER(getOwnerINLINE()).greatPeopleThreshold(false))
	{
		int iTotalGreatPeopleUnitProgress = 0;
		for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
		{
			iTotalGreatPeopleUnitProgress += getGreatPeopleUnitProgress((UnitTypes)iI);
		}

		int iGreatPeopleUnitRand = GC.getGameINLINE().getSorenRandNum(iTotalGreatPeopleUnitProgress, "Great Person");

		UnitTypes eGreatPeopleUnit = NO_UNIT;
		for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
		{
			if (iGreatPeopleUnitRand < getGreatPeopleUnitProgress((UnitTypes)iI))
			{
				eGreatPeopleUnit = ((UnitTypes)iI);
				break;
			}
			else
			{
				iGreatPeopleUnitRand -= getGreatPeopleUnitProgress((UnitTypes)iI);
			}
		}

		if (eGreatPeopleUnit != NO_UNIT)
		{
			changeGreatPeopleProgress(-(GET_PLAYER(getOwnerINLINE()).greatPeopleThreshold(false)));

			for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
			{
				setGreatPeopleUnitProgress(((UnitTypes)iI), 0);
			}

			createGreatPeople(eGreatPeopleUnit, true, false);
		}
	}
}


void CvCity::doMeltdown()
{
	CvWString szBuffer;
	int iI;

	if (GC.getUSE_DO_MELTDOWN_CALLBACK()) // K-Mod. block unused python callbacks
	{
		CyCity* pyCity = new CyCity(this);
		CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyCity));	// pass in city class
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "doMeltdown", argsList.makeFunctionArgs(), &lResult);
		delete pyCity;	// python fxn must not hold on to this pointer 
		if (lResult == 1)
		{
			return;
		}
	}

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		// <dlph.5>
		// <advc.003> Restructured DarkLunaPhantom's code the code a bit
		int oddsDivisor = GC.getBuildingInfo((BuildingTypes)iI).getNukeExplosionRand();
		if(oddsDivisor <= 0)
			continue; // Can save some time then
		if(getNumActiveBuilding((BuildingTypes)iI) == 0) // Was getNumBuilding
			continue;
		CvBuildingInfo& kBuilding = GC.getBuildingInfo((BuildingTypes)iI);
		if(kBuilding.getNukeExplosionRand() == 0 || isAreaCleanPower() ||
				(!kBuilding.isPower() && kBuilding.getPowerBonus() == NO_BONUS) ||
				(kBuilding.getPowerBonus() != NO_BONUS &&
				!hasBonus((BonusTypes)kBuilding.getPowerBonus())))
			continue;
		bool bUnused = false;
		// Check for hydroplant (or any modded plant with less severe drawbacks)
		for(int iJ = 0; iJ < GC.getNumBuildingInfos(); iJ++) {
			if(iI == iJ || getNumActiveBuilding((BuildingTypes)iJ) == 0)
				continue;
			CvBuildingInfo& kLoopBuilding = GC.getBuildingInfo((BuildingTypes)iJ);
			if(!kLoopBuilding.isPower() &&
					(kLoopBuilding.getPowerBonus() == NO_BONUS ||
					!hasBonus((BonusTypes)kLoopBuilding.getPowerBonus())))
				continue;
			if(kBuilding.isDirtyPower() && !kLoopBuilding.isDirtyPower()) {
				bUnused = true;
				break;
			}
			if(kBuilding.isDirtyPower() != kLoopBuilding.isDirtyPower())
				continue;
			if(kLoopBuilding.getNukeExplosionRand() == 0) {
				bUnused = true;
				break;
			}
			if(kBuilding.getNukeExplosionRand() >
					kLoopBuilding.getNukeExplosionRand())
				continue;
			if(kBuilding.getNukeExplosionRand() <
					kLoopBuilding.getNukeExplosionRand()) {
				bUnused = true;
				break;
			}
			if(iI < iJ) {
				bUnused = true;
				break;
			}
		}
		if(bUnused)
			continue;
		// Adjust odds to game speed:
		double pr = 1.0 / (oddsDivisor * GC.getGameINLINE().gameSpeedFactor());
		//if (GC.getGameINLINE().getSorenRandNum(GC.getBuildingInfo((BuildingTypes)iI).getNukeExplosionRand(), "Meltdown!!!") == 0)
		if(::bernoulliSuccess(pr, "dlph.5")) // </dlph.5> </advc.003>
		{
			if (getNumRealBuilding((BuildingTypes)iI) > 0)
			{
				setNumRealBuilding(((BuildingTypes)iI), 0);
			}
			//plot()->nukeExplosion(1);
			plot()->nukeExplosion(1, 0, false); // K-Mod

			szBuffer = gDLL->getText("TXT_KEY_MISC_MELTDOWN_CITY", getNameKey());
			gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_MELTDOWN", MESSAGE_TYPE_MINOR_EVENT, ARTFILEMGR.getInterfaceArtInfo("INTERFACE_UNHEALTHY_PERSON")->getPath(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), getX_INLINE(), getY_INLINE(), true, true);

			break;
		}
	}
}

// Private Functions...

void CvCity::read(FDataStreamBase* pStream)
{
	int iI;
	int iNumElts;

	// Init data before load
	reset();

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iID);
	pStream->Read(&m_iX);
	pStream->Read(&m_iY);
	pStream->Read(&m_iRallyX);
	pStream->Read(&m_iRallyY);
	pStream->Read(&m_iGameTurnFounded);
	pStream->Read(&m_iGameTurnAcquired);
	pStream->Read(&m_iPopulation);
	pStream->Read(&m_iHighestPopulation);
	pStream->Read(&m_iWorkingPopulation);
	pStream->Read(&m_iSpecialistPopulation);
	pStream->Read(&m_iNumGreatPeople);
	pStream->Read(&m_iBaseGreatPeopleRate);
	pStream->Read(&m_iGreatPeopleRateModifier);
	pStream->Read(&m_iGreatPeopleProgress);
	pStream->Read(&m_iNumWorldWonders);
	pStream->Read(&m_iNumTeamWonders);
	pStream->Read(&m_iNumNationalWonders);
	pStream->Read(&m_iNumBuildings);
	pStream->Read(&m_iGovernmentCenterCount);
	pStream->Read(&m_iMaintenance);
	pStream->Read(&m_iMaintenanceModifier);
	pStream->Read(&m_iWarWearinessModifier);
	pStream->Read(&m_iHurryAngerModifier);
	pStream->Read(&m_iHealRate);
	pStream->Read(&m_iEspionageHealthCounter);
	pStream->Read(&m_iEspionageHappinessCounter);
	pStream->Read(&m_iFreshWaterGoodHealth);
	pStream->Read(&m_iFreshWaterBadHealth);
	pStream->Read(&m_iFeatureGoodHealth);
	pStream->Read(&m_iFeatureBadHealth);
	pStream->Read(&m_iBuildingGoodHealth);
	pStream->Read(&m_iBuildingBadHealth);
	pStream->Read(&m_iPowerGoodHealth);
	pStream->Read(&m_iPowerBadHealth);
	pStream->Read(&m_iBonusGoodHealth);
	pStream->Read(&m_iBonusBadHealth);
	pStream->Read(&m_iHurryAngerTimer);
	pStream->Read(&m_iConscriptAngerTimer);
	pStream->Read(&m_iDefyResolutionAngerTimer);
	pStream->Read(&m_iHappinessTimer);
	pStream->Read(&m_iMilitaryHappinessUnits);
	pStream->Read(&m_iBuildingGoodHappiness);
	pStream->Read(&m_iBuildingBadHappiness);
	pStream->Read(&m_iExtraBuildingGoodHappiness);
	pStream->Read(&m_iExtraBuildingBadHappiness);
	pStream->Read(&m_iExtraBuildingGoodHealth);
	pStream->Read(&m_iExtraBuildingBadHealth);
	pStream->Read(&m_iFeatureGoodHappiness);
	pStream->Read(&m_iFeatureBadHappiness);
	pStream->Read(&m_iBonusGoodHappiness);
	pStream->Read(&m_iBonusBadHappiness);
	pStream->Read(&m_iReligionGoodHappiness);
	pStream->Read(&m_iReligionBadHappiness);
	pStream->Read(&m_iExtraHappiness);
	pStream->Read(&m_iExtraHealth);
	pStream->Read(&m_iNoUnhappinessCount);
	//pStream->Read(&m_iNoUnhealthyPopulationCount);
	pStream->Read(&m_iUnhealthyPopulationModifier); // K-Mod
	pStream->Read(&m_iBuildingOnlyHealthyCount);
	pStream->Read(&m_iFood);
	pStream->Read(&m_iFoodKept);
	pStream->Read(&m_iMaxFoodKeptPercent);
	pStream->Read(&m_iOverflowProduction);
	pStream->Read(&m_iFeatureProduction);
	pStream->Read(&m_iMilitaryProductionModifier);
	pStream->Read(&m_iSpaceProductionModifier);
	pStream->Read(&m_iExtraTradeRoutes);
	pStream->Read(&m_iTradeRouteModifier);
	pStream->Read(&m_iForeignTradeRouteModifier);
	pStream->Read(&m_iBuildingDefense);
	pStream->Read(&m_iBuildingBombardDefense);
	pStream->Read(&m_iFreeExperience);
	pStream->Read(&m_iCurrAirlift);
	pStream->Read(&m_iMaxAirlift);
	pStream->Read(&m_iAirModifier);
	pStream->Read(&m_iAirUnitCapacity);
	pStream->Read(&m_iNukeModifier);
	pStream->Read(&m_iFreeSpecialist);
	pStream->Read(&m_iPowerCount);
	pStream->Read(&m_iDirtyPowerCount);
	pStream->Read(&m_iDefenseDamage);
	pStream->Read(&m_iLastDefenseDamage);
	pStream->Read(&m_iOccupationTimer);
	pStream->Read(&m_iCultureUpdateTimer);
	pStream->Read(&m_iCitySizeBoost);
	pStream->Read(&m_iSpecialistFreeExperience);
	pStream->Read(&m_iEspionageDefenseModifier);

	pStream->Read(&m_bNeverLost);
	pStream->Read(&m_bBombarded);
	pStream->Read(&m_bDrafted);
	pStream->Read(&m_bAirliftTargeted);
	pStream->Read(&m_bWeLoveTheKingDay);
	pStream->Read(&m_bCitizensAutomated);
	pStream->Read(&m_bProductionAutomated);
	pStream->Read(&m_bWallOverride);
	// m_bInfoDirty not saved...
	// m_bLayoutDirty not saved...
	pStream->Read(&m_bPlundered);
	// <advc.103>
	if(uiFlag >= 6)
		pStream->Read(&m_bInvestigate);
	// </advc.103>
	pStream->Read((int*)&m_eOwner);
	pStream->Read((int*)&m_ePreviousOwner);
	pStream->Read((int*)&m_eOriginalOwner);
	pStream->Read((int*)&m_eCultureLevel);

	pStream->Read(NUM_YIELD_TYPES, m_aiSeaPlotYield);
	pStream->Read(NUM_YIELD_TYPES, m_aiRiverPlotYield);
	pStream->Read(NUM_YIELD_TYPES, m_aiBaseYieldRate);
	pStream->Read(NUM_YIELD_TYPES, m_aiYieldRateModifier);
	pStream->Read(NUM_YIELD_TYPES, m_aiPowerYieldRateModifier);
	pStream->Read(NUM_YIELD_TYPES, m_aiBonusYieldRateModifier);
	pStream->Read(NUM_YIELD_TYPES, m_aiTradeYield);
	pStream->Read(NUM_YIELD_TYPES, m_aiCorporationYield);
	pStream->Read(NUM_YIELD_TYPES, m_aiExtraSpecialistYield);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiCommerceRate);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiProductionToCommerceModifier);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiBuildingCommerce);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiSpecialistCommerce);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiReligionCommerce);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiCorporationCommerce);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiCommerceRateModifier);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiCommerceHappinessPer);
	pStream->Read(NUM_DOMAIN_TYPES, m_aiDomainFreeExperience);
	pStream->Read(NUM_DOMAIN_TYPES, m_aiDomainProductionModifier);
	pStream->Read(MAX_PLAYERS, m_aiCulture);
	pStream->Read(MAX_PLAYERS, m_aiNumRevolts);

	pStream->Read(MAX_PLAYERS, m_abEverOwned);
	pStream->Read(MAX_PLAYERS, m_abTradeRoute);
	pStream->Read(MAX_TEAMS, m_abRevealed);
	pStream->Read(MAX_TEAMS, m_abEspionageVisibility);

	pStream->ReadString(m_szName);
	// <advc.106k>
	if(uiFlag >= 5)
		pStream->ReadString(m_szPreviousName); // </advc.106k>
	pStream->ReadString(m_szScriptData);

	pStream->Read(GC.getNumBonusInfos(), m_paiNoBonus);
	pStream->Read(GC.getNumBonusInfos(), m_paiFreeBonus);
	pStream->Read(GC.getNumBonusInfos(), m_paiNumBonuses);
	pStream->Read(GC.getNumBonusInfos(), m_paiNumCorpProducedBonuses);
	pStream->Read(GC.getNumProjectInfos(), m_paiProjectProduction);
	pStream->Read(GC.getNumBuildingInfos(), m_paiBuildingProduction);
	pStream->Read(GC.getNumBuildingInfos(), m_paiBuildingProductionTime);
	pStream->Read(GC.getNumBuildingInfos(), m_paiBuildingOriginalOwner);
	pStream->Read(GC.getNumBuildingInfos(), m_paiBuildingOriginalTime);
	pStream->Read(GC.getNumUnitInfos(), m_paiUnitProduction);
	pStream->Read(GC.getNumUnitInfos(), m_paiUnitProductionTime);
	pStream->Read(GC.getNumUnitInfos(), m_paiGreatPeopleUnitRate);
	pStream->Read(GC.getNumUnitInfos(), m_paiGreatPeopleUnitProgress);
	pStream->Read(GC.getNumSpecialistInfos(), m_paiSpecialistCount);
	pStream->Read(GC.getNumSpecialistInfos(), m_paiMaxSpecialistCount);
	pStream->Read(GC.getNumSpecialistInfos(), m_paiForceSpecialistCount);
	pStream->Read(GC.getNumSpecialistInfos(), m_paiFreeSpecialistCount);
	pStream->Read(GC.getNumImprovementInfos(), m_paiImprovementFreeSpecialists);
	pStream->Read(GC.getNumReligionInfos(), m_paiReligionInfluence);
	pStream->Read(GC.getNumReligionInfos(), m_paiStateReligionHappiness);
	pStream->Read(GC.getNumUnitCombatInfos(), m_paiUnitCombatFreeExperience);
	pStream->Read(GC.getNumPromotionInfos(), m_paiFreePromotionCount);
	pStream->Read(GC.getNumBuildingInfos(), m_paiNumRealBuilding);
	pStream->Read(GC.getNumBuildingInfos(), m_paiNumFreeBuilding);

	pStream->Read(NUM_CITY_PLOTS, m_pabWorkingPlot);
	pStream->Read(GC.getNumReligionInfos(), m_pabHasReligion);
	pStream->Read(GC.getNumCorporationInfos(), m_pabHasCorporation);

	for (iI=0;iI<GC.getDefineINT("MAX_TRADE_ROUTES");iI++)
	{
		pStream->Read((int*)&m_paTradeCities[iI].eOwner);
		pStream->Read(&m_paTradeCities[iI].iID);
	}

	m_orderQueue.Read(pStream);

	pStream->Read(&m_iPopulationRank);
	pStream->Read(&m_bPopulationRankValid);
	pStream->Read(NUM_YIELD_TYPES, m_aiBaseYieldRank);
	pStream->Read(NUM_YIELD_TYPES, m_abBaseYieldRankValid);
	pStream->Read(NUM_YIELD_TYPES, m_aiYieldRank);
	pStream->Read(NUM_YIELD_TYPES, m_abYieldRankValid);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiCommerceRank);
	pStream->Read(NUM_COMMERCE_TYPES, m_abCommerceRankValid);

	pStream->Read(&iNumElts);
	m_aEventsOccured.clear();
	for (int i = 0; i < iNumElts; ++i)
	{
		EventTypes eEvent;
		pStream->Read((int*)&eEvent);
		m_aEventsOccured.push_back(eEvent);
	}

	pStream->Read(&iNumElts);
	m_aBuildingYieldChange.clear();
	for (int i = 0; i < iNumElts; ++i)
	{
		BuildingYieldChange kChange;
		kChange.read(pStream);
		m_aBuildingYieldChange.push_back(kChange);
	}

	pStream->Read(&iNumElts);
	m_aBuildingCommerceChange.clear();
	for (int i = 0; i < iNumElts; ++i)
	{
		BuildingCommerceChange kChange;
		kChange.read(pStream);
		m_aBuildingCommerceChange.push_back(kChange);
	}

	pStream->Read(&iNumElts);
	m_aBuildingHappyChange.clear();
	for (int i = 0; i < iNumElts; ++i)
	{
		int iBuildingClass;
		pStream->Read(&iBuildingClass);
		int iChange;
		pStream->Read(&iChange);
		m_aBuildingHappyChange.push_back(std::make_pair((BuildingClassTypes)iBuildingClass, iChange));
	}

	pStream->Read(&iNumElts);
	m_aBuildingHealthChange.clear();
	for (int i = 0; i < iNumElts; ++i)
	{
		int iBuildingClass;
		pStream->Read(&iBuildingClass);
		int iChange;
		pStream->Read(&iChange);
		m_aBuildingHealthChange.push_back(std::make_pair((BuildingClassTypes)iBuildingClass, iChange));
	} // <advc.912d>
	if(uiFlag >= 4)
		pStream->Read(&m_iPopRushHurryCount); // </advc.912d>
	// <advc.004x>
	if(uiFlag >= 2) {
		pStream->Read(&mrOrder);
		pStream->Read(&mrWasUnit);
	} // </advc.004x>
	// <advc.030b>
	if(uiFlag < 3) {
		CvArea* wa = waterArea(true);
		if(wa != NULL)
			wa->changeCitiesPerPlayer(getOwnerINLINE(), 1);
	} // </advc.030b>
}

void CvCity::write(FDataStreamBase* pStream)
{
	int iI;
	uint uiFlag = 1; // flag for expansion
	uiFlag = 2; // advc.004x
	uiFlag = 3; // advc.030b
	uiFlag = 4; // advc.912d
	uiFlag = 5; // advc.106k
	uiFlag = 6; // advc.103
	pStream->Write(uiFlag);

	pStream->Write(m_iID);
	pStream->Write(m_iX);
	pStream->Write(m_iY);
	pStream->Write(m_iRallyX);
	pStream->Write(m_iRallyY);
	pStream->Write(m_iGameTurnFounded);
	pStream->Write(m_iGameTurnAcquired);
	pStream->Write(m_iPopulation);
	pStream->Write(m_iHighestPopulation);
	pStream->Write(m_iWorkingPopulation);
	pStream->Write(m_iSpecialistPopulation);
	pStream->Write(m_iNumGreatPeople);
	pStream->Write(m_iBaseGreatPeopleRate);
	pStream->Write(m_iGreatPeopleRateModifier);
	pStream->Write(m_iGreatPeopleProgress);
	pStream->Write(m_iNumWorldWonders);
	pStream->Write(m_iNumTeamWonders);
	pStream->Write(m_iNumNationalWonders);
	pStream->Write(m_iNumBuildings);
	pStream->Write(m_iGovernmentCenterCount);
	pStream->Write(m_iMaintenance);
	pStream->Write(m_iMaintenanceModifier);
	pStream->Write(m_iWarWearinessModifier);
	pStream->Write(m_iHurryAngerModifier);
	pStream->Write(m_iHealRate);
	pStream->Write(m_iEspionageHealthCounter);
	pStream->Write(m_iEspionageHappinessCounter);
	pStream->Write(m_iFreshWaterGoodHealth);
	pStream->Write(m_iFreshWaterBadHealth);
	pStream->Write(m_iFeatureGoodHealth);
	pStream->Write(m_iFeatureBadHealth);
	pStream->Write(m_iBuildingGoodHealth);
	pStream->Write(m_iBuildingBadHealth);
	pStream->Write(m_iPowerGoodHealth);
	pStream->Write(m_iPowerBadHealth);
	pStream->Write(m_iBonusGoodHealth);
	pStream->Write(m_iBonusBadHealth);
	pStream->Write(m_iHurryAngerTimer);
	pStream->Write(m_iConscriptAngerTimer);
	pStream->Write(m_iDefyResolutionAngerTimer);
	pStream->Write(m_iHappinessTimer);
	pStream->Write(m_iMilitaryHappinessUnits);
	pStream->Write(m_iBuildingGoodHappiness);
	pStream->Write(m_iBuildingBadHappiness);
	pStream->Write(m_iExtraBuildingGoodHappiness);
	pStream->Write(m_iExtraBuildingBadHappiness);
	pStream->Write(m_iExtraBuildingGoodHealth);
	pStream->Write(m_iExtraBuildingBadHealth);
	pStream->Write(m_iFeatureGoodHappiness);
	pStream->Write(m_iFeatureBadHappiness);
	pStream->Write(m_iBonusGoodHappiness);
	pStream->Write(m_iBonusBadHappiness);
	pStream->Write(m_iReligionGoodHappiness);
	pStream->Write(m_iReligionBadHappiness);
	pStream->Write(m_iExtraHappiness);
	pStream->Write(m_iExtraHealth);
	pStream->Write(m_iNoUnhappinessCount);
	//pStream->Write(m_iNoUnhealthyPopulationCount);
	pStream->Write(m_iUnhealthyPopulationModifier); // K-Mod	
	pStream->Write(m_iBuildingOnlyHealthyCount);
	pStream->Write(m_iFood);
	pStream->Write(m_iFoodKept);
	pStream->Write(m_iMaxFoodKeptPercent);
	pStream->Write(m_iOverflowProduction);
	pStream->Write(m_iFeatureProduction);
	pStream->Write(m_iMilitaryProductionModifier);
	pStream->Write(m_iSpaceProductionModifier);
	pStream->Write(m_iExtraTradeRoutes);
	pStream->Write(m_iTradeRouteModifier);
	pStream->Write(m_iForeignTradeRouteModifier);
	pStream->Write(m_iBuildingDefense);
	pStream->Write(m_iBuildingBombardDefense);
	pStream->Write(m_iFreeExperience);
	pStream->Write(m_iCurrAirlift);
	pStream->Write(m_iMaxAirlift);
	pStream->Write(m_iAirModifier);
	pStream->Write(m_iAirUnitCapacity);
	pStream->Write(m_iNukeModifier);
	pStream->Write(m_iFreeSpecialist);
	pStream->Write(m_iPowerCount);
	pStream->Write(m_iDirtyPowerCount);
	pStream->Write(m_iDefenseDamage);
	pStream->Write(m_iLastDefenseDamage);
	pStream->Write(m_iOccupationTimer);
	pStream->Write(m_iCultureUpdateTimer);
	pStream->Write(m_iCitySizeBoost);
	pStream->Write(m_iSpecialistFreeExperience);
	pStream->Write(m_iEspionageDefenseModifier);

	pStream->Write(m_bNeverLost);
	pStream->Write(m_bBombarded);
	pStream->Write(m_bDrafted);
	pStream->Write(m_bAirliftTargeted);
	pStream->Write(m_bWeLoveTheKingDay);
	pStream->Write(m_bCitizensAutomated);
	pStream->Write(m_bProductionAutomated);
	pStream->Write(m_bWallOverride);
	// m_bInfoDirty not saved...
	// m_bLayoutDirty not saved...
	pStream->Write(m_bPlundered);
	pStream->Write(m_bInvestigate); // advc.103

	pStream->Write(m_eOwner);
	pStream->Write(m_ePreviousOwner);
	pStream->Write(m_eOriginalOwner);
	pStream->Write(m_eCultureLevel);

	pStream->Write(NUM_YIELD_TYPES, m_aiSeaPlotYield);
	pStream->Write(NUM_YIELD_TYPES, m_aiRiverPlotYield);
	pStream->Write(NUM_YIELD_TYPES, m_aiBaseYieldRate);
	pStream->Write(NUM_YIELD_TYPES, m_aiYieldRateModifier);
	pStream->Write(NUM_YIELD_TYPES, m_aiPowerYieldRateModifier);
	pStream->Write(NUM_YIELD_TYPES, m_aiBonusYieldRateModifier);
	pStream->Write(NUM_YIELD_TYPES, m_aiTradeYield);
	pStream->Write(NUM_YIELD_TYPES, m_aiCorporationYield);
	pStream->Write(NUM_YIELD_TYPES, m_aiExtraSpecialistYield);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiCommerceRate);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiProductionToCommerceModifier);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiBuildingCommerce);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiSpecialistCommerce);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiReligionCommerce);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiCorporationCommerce);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiCommerceRateModifier);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiCommerceHappinessPer);
	pStream->Write(NUM_DOMAIN_TYPES, m_aiDomainFreeExperience);
	pStream->Write(NUM_DOMAIN_TYPES, m_aiDomainProductionModifier);
	pStream->Write(MAX_PLAYERS, m_aiCulture);
	pStream->Write(MAX_PLAYERS, m_aiNumRevolts);

	pStream->Write(MAX_PLAYERS, m_abEverOwned);
	pStream->Write(MAX_PLAYERS, m_abTradeRoute);
	pStream->Write(MAX_TEAMS, m_abRevealed);
	pStream->Write(MAX_TEAMS, m_abEspionageVisibility);

	pStream->WriteString(m_szName);
	pStream->WriteString(m_szPreviousName); // advc.106k
	pStream->WriteString(m_szScriptData);

	pStream->Write(GC.getNumBonusInfos(), m_paiNoBonus);
	pStream->Write(GC.getNumBonusInfos(), m_paiFreeBonus);
	pStream->Write(GC.getNumBonusInfos(), m_paiNumBonuses);
	pStream->Write(GC.getNumBonusInfos(), m_paiNumCorpProducedBonuses);
	pStream->Write(GC.getNumProjectInfos(), m_paiProjectProduction);
	pStream->Write(GC.getNumBuildingInfos(), m_paiBuildingProduction);
	pStream->Write(GC.getNumBuildingInfos(), m_paiBuildingProductionTime);
	pStream->Write(GC.getNumBuildingInfos(), m_paiBuildingOriginalOwner);
	pStream->Write(GC.getNumBuildingInfos(), m_paiBuildingOriginalTime);
	pStream->Write(GC.getNumUnitInfos(), m_paiUnitProduction);
	pStream->Write(GC.getNumUnitInfos(), m_paiUnitProductionTime);
	pStream->Write(GC.getNumUnitInfos(), m_paiGreatPeopleUnitRate);
	pStream->Write(GC.getNumUnitInfos(), m_paiGreatPeopleUnitProgress);
	pStream->Write(GC.getNumSpecialistInfos(), m_paiSpecialistCount);
	pStream->Write(GC.getNumSpecialistInfos(), m_paiMaxSpecialistCount);
	pStream->Write(GC.getNumSpecialistInfos(), m_paiForceSpecialistCount);
	pStream->Write(GC.getNumSpecialistInfos(), m_paiFreeSpecialistCount);
	pStream->Write(GC.getNumImprovementInfos(), m_paiImprovementFreeSpecialists);
	pStream->Write(GC.getNumReligionInfos(), m_paiReligionInfluence);
	pStream->Write(GC.getNumReligionInfos(), m_paiStateReligionHappiness);
	pStream->Write(GC.getNumUnitCombatInfos(), m_paiUnitCombatFreeExperience);
	pStream->Write(GC.getNumPromotionInfos(), m_paiFreePromotionCount);
	pStream->Write(GC.getNumBuildingInfos(), m_paiNumRealBuilding);
	pStream->Write(GC.getNumBuildingInfos(), m_paiNumFreeBuilding);

	pStream->Write(NUM_CITY_PLOTS, m_pabWorkingPlot);
	pStream->Write(GC.getNumReligionInfos(), m_pabHasReligion);
	pStream->Write(GC.getNumCorporationInfos(), m_pabHasCorporation);

	for (iI=0;iI<GC.getDefineINT("MAX_TRADE_ROUTES");iI++)
	{
		pStream->Write(m_paTradeCities[iI].eOwner);
		pStream->Write(m_paTradeCities[iI].iID);
	}

	m_orderQueue.Write(pStream);

	pStream->Write(m_iPopulationRank);
	pStream->Write(m_bPopulationRankValid);
	pStream->Write(NUM_YIELD_TYPES, m_aiBaseYieldRank);
	pStream->Write(NUM_YIELD_TYPES, m_abBaseYieldRankValid);
	pStream->Write(NUM_YIELD_TYPES, m_aiYieldRank);
	pStream->Write(NUM_YIELD_TYPES, m_abYieldRankValid);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiCommerceRank);
	pStream->Write(NUM_COMMERCE_TYPES, m_abCommerceRankValid);

	pStream->Write(m_aEventsOccured.size());
	for (std::vector<EventTypes>::iterator it = m_aEventsOccured.begin(); it != m_aEventsOccured.end(); ++it)
	{
		pStream->Write(*it);
	}

	pStream->Write(m_aBuildingYieldChange.size());
	for (std::vector<BuildingYieldChange>::iterator it = m_aBuildingYieldChange.begin(); it != m_aBuildingYieldChange.end(); ++it)
	{
		(*it).write(pStream);
	}

	pStream->Write(m_aBuildingCommerceChange.size());
	for (std::vector<BuildingCommerceChange>::iterator it = m_aBuildingCommerceChange.begin(); it != m_aBuildingCommerceChange.end(); ++it)
	{
		(*it).write(pStream);
	}

	pStream->Write(m_aBuildingHappyChange.size());
	for (BuildingChangeArray::iterator it = m_aBuildingHappyChange.begin(); it != m_aBuildingHappyChange.end(); ++it)
	{
		pStream->Write((*it).first);
		pStream->Write((*it).second);
	}

	pStream->Write(m_aBuildingHealthChange.size());
	for (BuildingChangeArray::iterator it = m_aBuildingHealthChange.begin(); it != m_aBuildingHealthChange.end(); ++it)
	{
		pStream->Write((*it).first);
		pStream->Write((*it).second);
	}
	pStream->Write(m_iPopRushHurryCount); // advc.912d
	// <advc.004x>
	pStream->Write(mrOrder);
	pStream->Write(mrWasUnit); // </advc.004x>
}


//------------------------------------------------------------------------------------------------
class VisibleBuildingComparator
{
public:
	bool operator() (BuildingTypes e1, BuildingTypes e2)
	{
		if(GC.getBuildingInfo(e1).getVisibilityPriority() > GC.getBuildingInfo(e2).getVisibilityPriority())
			return true;
		else if(GC.getBuildingInfo(e1).getVisibilityPriority() == GC.getBuildingInfo(e2).getVisibilityPriority())
		{
			//break ties by building type higher building type
			if(e1 > e2)
				return true;
		}

		return false;
	}
};

void CvCity::getVisibleBuildings(std::list<BuildingTypes>& kChosenVisible, int& iChosenNumGenerics)
{
	int iNumBuildings;
	BuildingTypes eCurType;
	std::vector<BuildingTypes> kVisible;

	iNumBuildings = GC.getNumBuildingInfos();
	// <advc.045>
	PlayerTypes eActivePlayer = GC.getGameINLINE().getActivePlayer();
	bool allVisible = (eActivePlayer == NO_PLAYER ||
			plot()->isInvestigate(TEAMID(eActivePlayer)) ||
			plot()->plotCount(NULL, -1, -1, eActivePlayer) > 0 ||
			GC.getGameINLINE().isDebugMode()); // </advc.045>
	for(int i = 0; i < iNumBuildings; i++)
	{
		eCurType = (BuildingTypes) i;
		// <advc.045>
		if(getNumBuilding(eCurType) <= 0)
			continue;
		// Copied these two checks from Rise of Mankind
		CvBuildingInfo& kBuilding = GC.getBuildingInfo(eCurType);
		bool bWonder = isLimitedWonderClass((BuildingClassTypes)kBuilding.
				getBuildingClassType());
		bool bDefense = (kBuilding.getDefenseModifier() > 0);
		if(!allVisible && !bWonder && !bDefense) {
			bool visibleYieldChange = false;
			int* seaPlotYieldChanges = kBuilding.getSeaPlotYieldChangeArray();
			int* riverPlotYieldChanges = kBuilding.getRiverPlotYieldChangeArray();
			for(int j = 0; j < NUM_YIELD_TYPES; j++) {
				if(seaPlotYieldChanges[j] + riverPlotYieldChanges[j] != 0)
					visibleYieldChange = true;
			}
			if(!visibleYieldChange)
				continue;
		} // </advc.045>
		kVisible.push_back(eCurType);
	}

	// sort the visible ones by decreasing priority
	VisibleBuildingComparator kComp;
	std::sort(kVisible.begin(), kVisible.end(), kComp);

	// how big is this city, in terms of buildings?
	// general rule: no more than fPercentUnique percent of a city can be uniques
	int iTotalVisibleBuildings;
	if(stricmp(GC.getDefineSTRING("GAME_CITY_SIZE_METHOD"), "METHOD_EXPONENTIAL") == 0) 
	{
		int iCityScaleMod =  ((int)(pow((float)getPopulation(), GC.getDefineFLOAT("GAME_CITY_SIZE_EXP_MODIFIER")))) * 2;
		iTotalVisibleBuildings = (10 + iCityScaleMod);
	} 
	else 
	{
		float fLo = GC.getDefineFLOAT("GAME_CITY_SIZE_LINMAP_AT_0");
		float fHi = GC.getDefineFLOAT("GAME_CITY_SIZE_LINMAP_AT_50");
		float fCurSize = (float)getPopulation();
		iTotalVisibleBuildings = int(((fHi - fLo) / 50.0f) * fCurSize + fLo);
	}
	float fMaxUniquePercent = GC.getDefineFLOAT("GAME_CITY_SIZE_MAX_PERCENT_UNIQUE");
	int iMaxNumUniques = (int)(fMaxUniquePercent * iTotalVisibleBuildings);

	// compute how many buildings are generics vs. unique Civ buildings?
	int iNumGenerics;
	int iNumUniques;
	if((int)kVisible.size() > iMaxNumUniques)
	{
		iNumUniques = iMaxNumUniques;
	}
	else 
	{
		iNumUniques = kVisible.size();
	}
	iNumGenerics = iTotalVisibleBuildings - iNumUniques + getCitySizeBoost();
	
	// return
	iChosenNumGenerics = iNumGenerics;
	for(int i = 0; i < iNumUniques; i++)
	{
		kChosenVisible.push_back(kVisible[i]);
	}
}

static int natGetDeterministicRandom(int iMin, int iMax, int iSeedX, int iSeedY)
{
	srand(7297 * iSeedX + 2909  * iSeedY);
	return (rand() % (iMax - iMin)) + iMin;
}

void CvCity::getVisibleEffects(ZoomLevelTypes eCurZoom, std::vector<const TCHAR*>& kEffectNames) 
{
	if (isOccupation() && isVisible(getTeam(), false) == true)
	{
		if (eCurZoom  == ZOOM_DETAIL)
		{
			kEffectNames.push_back("EFFECT_CITY_BIG_BURNING_SMOKE");
			kEffectNames.push_back("EFFECT_CITY_FIRE");
		}
		else
		{
			kEffectNames.push_back("EFFECT_CITY_BIG_BURNING_SMOKE");
		}
		return;
	} 

	if ((getTeam() == GC.getGameINLINE().getActiveTeam()) || GC.getGameINLINE().isDebugMode()) 
	{
		
		if (angryPopulation() > 0)
		{
			kEffectNames.push_back("EFFECT_CITY_BURNING_SMOKE");
		}
		
		if (healthRate() < 0)
		{
			kEffectNames.push_back("EFFECT_CITY_DISEASED");
		}


		if (isWeLoveTheKingDay())
		{
			int iSeed = natGetDeterministicRandom(0, 32767, getX_INLINE(), getY_INLINE());
			CvRandom kRand;
			kRand.init(iSeed);

			// fireworks
			const TCHAR* szFireworkEffects[] =
			{"FIREWORKS_RED_LARGE_SLOW",
				"FIREWORKS_RED_SMALL_FAST",
				"FIREWORKS_GREEN_LARGE_SLOW",
				"FIREWORKS_GREEN_SMALL_FAST",
				"FIREWORKS_PURPLE_LARGE_SLOW",
				"FIREWORKS_PURPLE_SMALL_FAST",
				"FIREWORKS_YELLOW_LARGE_SLOW",
				"FIREWORKS_YELLOW_SMALL_FAST",
				"FIREWORKS_BLUE_LARGE_SLOW",
				"FIREWORKS_BLUE_SMALL_FAST"};

			int iNumEffects = sizeof(szFireworkEffects) / sizeof(TCHAR*);
			for(int i = 0; i < (iNumEffects < 3 ? iNumEffects : 3); i++)
			{
				kEffectNames.push_back(szFireworkEffects[kRand.get(iNumEffects)]);
			}
		}
	}
}

void CvCity::getCityBillboardSizeIconColors(NiColorA& kDotColor, NiColorA& kTextColor) const
{
	PlayerColorTypes ePlayerColor = //GET_PLAYER(getOwnerINLINE()).getPlayerColor())
		/*  advc.001: CvPlayer::getPlayerColor will return the Barbarian color
			if the city owner hasn't been met (city revealed through map trade) */
			GC.getInitCore().getColor(getOwnerINLINE());
	NiColorA kPlayerColor = GC.getColorInfo((ColorTypes)GC.getPlayerColorInfo(
			ePlayerColor).getColorTypePrimary()).getColor();
	NiColorA kGrowing;
	kGrowing = NiColorA(0.73f,1,0.73f,1);
	NiColorA kShrinking(1,0.73f,0.73f,1);
	NiColorA kStagnant(0.83f,0.83f,0.83f,1);
	NiColorA kUnknown(.5f,.5f,.5f,1);
	NiColorA kWhite(1,1,1,1);
	NiColorA kBlack(0,0,0,1);

	if (getTeam() == GC.getGameINLINE().getActiveTeam()
			&& isHuman()) // advc.127
	{
		if (foodDifference() < 0)
		{
			if (foodDifference() == -1 && getFood() >= (75 * growthThreshold()) / 100)
			{
				kDotColor = kStagnant;
				kTextColor = kBlack;	
			}
			else
			{
				kDotColor = kShrinking;
				kTextColor = kBlack;
			}
		}
		else if (foodDifference() > 0)
		{
			kDotColor = kGrowing;
			kTextColor = kBlack;
		}
		else if (foodDifference() == 0)
		{
			kDotColor = kStagnant;
			kTextColor = kBlack;
		}
	}
	else
	{
		kDotColor = kPlayerColor;
		NiColorA kPlayerSecondaryColor = GC.getColorInfo((ColorTypes)
				GC.getPlayerColorInfo(ePlayerColor). // advc.001
				getColorTypeSecondary()).getColor();
		kTextColor = kPlayerSecondaryColor;
	}
}

const TCHAR* CvCity::getCityBillboardProductionIcon() const
{
	if (canBeSelected() && isProduction())
	{
		CLLNode<OrderData>* pOrderNode;
		pOrderNode = headOrderQueueNode();
		FAssert(pOrderNode != NULL);

		const TCHAR* szIcon = NULL;
		switch(pOrderNode->m_data.eOrderType)
		{
		case ORDER_TRAIN:
			{
				UnitTypes eType = getProductionUnit();
				FAssert(eType != NO_UNIT);
				szIcon = GET_PLAYER(getOwnerINLINE()).getUnitButton(eType);
				break;
			}
		case ORDER_CONSTRUCT:
			{
				BuildingTypes eType = getProductionBuilding();
				FAssert(eType != NO_BUILDING);
				szIcon = GC.getBuildingInfo(eType).getButton();
				break;
			}
		case ORDER_CREATE:
			{
				ProjectTypes eType = getProductionProject();
				FAssert(eType != NO_PROJECT);
				szIcon = GC.getProjectInfo(eType).getButton();
				break;
			}
		case ORDER_MAINTAIN:
			{
				ProcessTypes eType = getProductionProcess();
				FAssert(eType != NO_PROCESS);
				szIcon = GC.getProcessInfo(eType).getButton();
				break;
			}
		default:
			{
				FAssert(false);
			}
		}
		return szIcon;
	}
	else
	{
		return ARTFILEMGR.getInterfaceArtInfo("INTERFACE_BUTTONS_NOPRODUCTION")->getPath();
	}
}

bool CvCity::getFoodBarPercentages(std::vector<float>& afPercentages) const
{
	if (!canBeSelected())
	{
		return false;
	}

	afPercentages.resize(NUM_INFOBAR_TYPES, 0.0f);
	if (foodDifference() < 0)
	{
		afPercentages[INFOBAR_STORED] = std::max(0, (getFood() + foodDifference())) / (float) growthThreshold();
		afPercentages[INFOBAR_RATE_EXTRA] = std::min(-foodDifference(), getFood()) / (float) growthThreshold();
	}
	else
	{
		afPercentages[INFOBAR_STORED] = getFood() / (float) growthThreshold();
		afPercentages[INFOBAR_RATE] = foodDifference() / (float) growthThreshold();
	}

	return true;
}

bool CvCity::getProductionBarPercentages(std::vector<float>& afPercentages) const
{
	if (!canBeSelected())
	{
		return false;
	}

	if (!isProductionProcess())
	{
		afPercentages.resize(NUM_INFOBAR_TYPES, 0.0f);
		int iProductionDiffNoFood = getCurrentProductionDifference(true, true);
		int iProductionDiffJustFood = getCurrentProductionDifference(false, true)
				- iProductionDiffNoFood;
		afPercentages[INFOBAR_STORED] = getProduction() / (float) getProductionNeeded();
		afPercentages[INFOBAR_RATE] = iProductionDiffNoFood / (float) getProductionNeeded();
		afPercentages[INFOBAR_RATE_EXTRA] = iProductionDiffJustFood / (float) getProductionNeeded();
	}

	return true;
}

NiColorA CvCity::getBarBackgroundColor() const
{
	if (atWar(getTeam(), GC.getGameINLINE().getActiveTeam()))
	{
		return NiColorA(0.5f, 0, 0, 0.5f); // red
	}
	return NiColorA(0, 0, 0, 0.5f);
}

bool CvCity::isStarCity() const
{
	return isCapital();
}

bool CvCity::isValidBuildingLocation(BuildingTypes eBuilding) const
{
	// if both the river and water flags are set, we require one of the two conditions, not both
	if (GC.getBuildingInfo(eBuilding).isWater())
	{
		if (!GC.getBuildingInfo(eBuilding).isRiver() || !plot()->isRiver())
		{
			if (!isCoastal(GC.getBuildingInfo(eBuilding).getMinAreaSize()))
			{
				return false;
			}
		}
	}
	else
	{
		if (area()->getNumTiles() < GC.getBuildingInfo(eBuilding).getMinAreaSize())
		{
			return false;
		}

		if (GC.getBuildingInfo(eBuilding).isRiver())
		{
			if (!(plot()->isRiver()))
			{
				return false;
			}
		}
	}

	return true;
}

int CvCity::getTriggerValue(EventTriggerTypes eTrigger) const
{
	FAssert(eTrigger >= 0);
	FAssert(eTrigger < GC.getNumEventTriggerInfos());

	CvEventTriggerInfo& kTrigger = GC.getEventTriggerInfo(eTrigger);


	if (!CvString(kTrigger.getPythonCanDoCity()).empty())
	{
		long lResult;

		CyArgsList argsList;
		argsList.add(eTrigger);
		argsList.add(getOwnerINLINE());
		argsList.add(getID());

		gDLL->getPythonIFace()->callFunction(PYRandomEventModule, kTrigger.getPythonCanDoCity(), argsList.makeFunctionArgs(), &lResult);

		if (0 == lResult)
		{
			return MIN_INT;
		}
	}

	if (kTrigger.getNumBuildings() > 0 && kTrigger.getNumBuildingsRequired() > 0)
	{
		bool bFoundValid = false;

		for (int i = 0; i < kTrigger.getNumBuildingsRequired(); ++i)
		{
			if (kTrigger.getBuildingRequired(i) != NO_BUILDINGCLASS)
			{
				BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(kTrigger.getBuildingRequired(i));
				if (NO_BUILDING != eBuilding)
				{
					if (getNumRealBuilding(eBuilding) > 0)
					{
						bFoundValid = true;
					}
				}
			}
		}

		if (!bFoundValid)
		{
			return MIN_INT;
		}
	}


	if (getReligionCount() < kTrigger.getNumReligions())
	{
		return MIN_INT;
	}

	if (kTrigger.getNumReligions() > 0 && kTrigger.getNumReligionsRequired() > 0)
	{
		bool bFoundValid = false;

		for (int i = 0; i < kTrigger.getNumReligionsRequired(); ++i)
		{
			if (!kTrigger.isStateReligion() || kTrigger.getReligionRequired(i) == GET_PLAYER(getOwnerINLINE()).getStateReligion())
			{
				if (isHasReligion((ReligionTypes)kTrigger.getReligionRequired(i)))
				{
					if (!kTrigger.isHolyCity() || isHolyCity((ReligionTypes)kTrigger.getReligionRequired(i)))
					{
						bFoundValid = true;
					}
				}
			}
		}

		if (!bFoundValid)
		{
			return MIN_INT;
		}
	}

	if (getCorporationCount() < kTrigger.getNumCorporations())
	{
		return MIN_INT;
	}

	if (kTrigger.getNumCorporations() > 0 && kTrigger.getNumCorporationsRequired() > 0)
	{
		bool bFoundValid = false;

		for (int i = 0; i < kTrigger.getNumCorporationsRequired(); ++i)
		{
			if (isHasCorporation((CorporationTypes)kTrigger.getCorporationRequired(i)))
			{
				if (!kTrigger.isHeadquarters() || isHeadquarters((CorporationTypes)kTrigger.getCorporationRequired(i)))
				{
					bFoundValid = true;
				}
			}
		}

		if (!bFoundValid)
		{
			return MIN_INT;
		}
	}

	if (kTrigger.getMinPopulation() > 0)
	{
		if (getPopulation() < kTrigger.getMinPopulation())
		{
			return MIN_INT;
		}
	}


	if (kTrigger.getMaxPopulation() > 0)
	{
		if (getPopulation() > kTrigger.getMaxPopulation())
		{
			return MIN_INT;
		}
	}

	if (kTrigger.getAngry() > 0)
	{
		if (unhappyLevel(0) - happyLevel() < kTrigger.getAngry())
		{
			return MIN_INT;
		}
	}
	else if (kTrigger.getAngry() < 0)
	{
		if (happyLevel() - unhappyLevel(0) < -kTrigger.getAngry())
		{
			return MIN_INT;
		}
	}

	if (kTrigger.getUnhealthy() > 0)
	{
		if (badHealth(false, 0) - goodHealth() < kTrigger.getUnhealthy())
		{
			return MIN_INT;
		}
	}
	else if (kTrigger.getUnhealthy() < 0)
	{
		if (goodHealth() - badHealth(false, 0) < -kTrigger.getUnhealthy())
		{
			return MIN_INT;
		}
	}

	if (kTrigger.isPrereqEventCity() && kTrigger.getNumPrereqEvents() > 0)
	{
		bool bFoundValid = true;

		for (int iI = 0; iI < kTrigger.getNumPrereqEvents(); ++iI)
		{
			if (!isEventOccured((EventTypes)kTrigger.getPrereqEvent(iI)))
			{
				bFoundValid = false;
				break;
			}
		}

		if (!bFoundValid)
		{
			return MIN_INT;
		}
	}


	int iValue = 0;

	if (0 == getFood() && kTrigger.getCityFoodWeight() > 0)
	{
		return MIN_INT;
	}

	iValue += getFood() * kTrigger.getCityFoodWeight();

	return iValue;
}

bool CvCity::canApplyEvent(EventTypes eEvent, const EventTriggeredData& kTriggeredData) const
{
	CvEventInfo& kEvent = GC.getEventInfo(eEvent);

	if (!kEvent.isCityEffect() && !kEvent.isOtherPlayerCityEffect())
	{
		return true;
	}

	if (-1 == kTriggeredData.m_iCityId && kEvent.isCityEffect())
	{
		return false;
	}

	if (-1 == kTriggeredData.m_iOtherPlayerCityId && kEvent.isOtherPlayerCityEffect())
	{
		return false;
	}

	if (kEvent.getFood() + ((100 + kEvent.getFoodPercent()) * getFood()) / 100 < 0)
	{
		return false;
	}

	if (kEvent.getPopulationChange() + getPopulation() <= 0)
	{
		return false;
	}

	if (100 * kEvent.getCulture() + getCultureTimes100(getOwnerINLINE()) < 0)
	{
		return false;
	}

	if (kEvent.getBuildingClass() != NO_BUILDINGCLASS)
	{
		BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(kEvent.getBuildingClass());
		if (eBuilding == NO_BUILDING)
		{
			return false;
		}

		if (kEvent.getBuildingChange() > 0)
		{
			if (getNumBuilding(eBuilding) >= GC.getCITY_MAX_NUM_BUILDINGS())
			{
				return false;
			}
		}
		else if (kEvent.getBuildingChange() < 0)
		{
			if (getNumRealBuilding(eBuilding) + kEvent.getBuildingChange() < 0)
			{
				return false;
			}
		}
	}

	if (-1 != kEvent.getMaxNumReligions() && getReligionCount() > kEvent.getMaxNumReligions())
	{
		return false;
	}

	if (kEvent.getMinPillage() > 0)
	{
		int iNumImprovements = 0;
		for (int i = 0; i < NUM_CITY_PLOTS; ++i)
		{
			if (CITY_HOME_PLOT != i)
			{
				CvPlot* pPlot = getCityIndexPlot(i);
				if (NULL != pPlot && pPlot->getOwnerINLINE() == getOwnerINLINE())
				{
					if (NO_IMPROVEMENT != pPlot->getImprovementType() && !GC.getImprovementInfo(pPlot->getImprovementType()).isPermanent())
					{
						++iNumImprovements;
					}
				}
			}
		}

		if (iNumImprovements < kEvent.getMinPillage())
		{
			return false;
		}
	}

	return true;
}

void CvCity::applyEvent(EventTypes eEvent, const EventTriggeredData& kTriggeredData, bool bClear)
{
	if (!canApplyEvent(eEvent, kTriggeredData))
	{
		return;
	}

	setEventOccured(eEvent, true);

	CvEventInfo& kEvent = GC.getEventInfo(eEvent);

	if (kEvent.isCityEffect() || kEvent.isOtherPlayerCityEffect())
	{
		if (kEvent.getHappy() != 0)
		{
			changeExtraHappiness(kEvent.getHappy());
		}

		if (kEvent.getHealth() != 0)
		{
			changeExtraHealth(kEvent.getHealth());
		}

		if (kEvent.getHurryAnger() != 0)
		{
			changeHurryAngerTimer(kEvent.getHurryAnger() * flatHurryAngerLength());
		}

		if (kEvent.getHappyTurns() != 0)
		{
			changeHappinessTimer(kEvent.getHappyTurns());
		}

		if (kEvent.getFood() != 0 || kEvent.getFoodPercent() != 0)
		{
			changeFood(kEvent.getFood() + (kEvent.getFoodPercent() * getFood()) / 100);
		}

		if (kEvent.getPopulationChange() != 0)
		{
			changePopulation(kEvent.getPopulationChange());
		}

		if (kEvent.getRevoltTurns() > 0)
		{
			changeCultureUpdateTimer(kEvent.getRevoltTurns());
			changeOccupationTimer(kEvent.getRevoltTurns());
		}

		if (0 != kEvent.getSpaceProductionModifier())
		{
			changeSpaceProductionModifier(kEvent.getSpaceProductionModifier());
		}

		if (kEvent.getMaxPillage() > 0)
		{
			FAssert(kEvent.getMaxPillage() >= kEvent.getMinPillage());
			int iNumPillage = kEvent.getMinPillage() + GC.getGameINLINE().getSorenRandNum(kEvent.getMaxPillage() - kEvent.getMinPillage(), "Pick number of event pillaged plots");

			int iNumPillaged = 0;
			for (int i = 0; i < iNumPillage; ++i)
			{
				int iRandOffset = GC.getGameINLINE().getSorenRandNum(NUM_CITY_PLOTS, "Pick event pillage plot");
				for (int j = 0; j < NUM_CITY_PLOTS; ++j)
				{
					int iPlot = (j + iRandOffset) % NUM_CITY_PLOTS;
					if (CITY_HOME_PLOT != iPlot)
					{
						CvPlot* pPlot = getCityIndexPlot(iPlot);
						if (NULL != pPlot && pPlot->getOwnerINLINE() == getOwnerINLINE())
						{
							if (NO_IMPROVEMENT != pPlot->getImprovementType() && !GC.getImprovementInfo(pPlot->getImprovementType()).isPermanent())
							{
								CvWString szBuffer = gDLL->getText("TXT_KEY_EVENT_CITY_IMPROVEMENT_DESTROYED", GC.getImprovementInfo(pPlot->getImprovementType()).getTextKeyWide());
								gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_PILLAGED", MESSAGE_TYPE_INFO, GC.getImprovementInfo(pPlot->getImprovementType()).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"), pPlot->getX_INLINE(), pPlot->getY_INLINE(), true, true);
								pPlot->setImprovementType(NO_IMPROVEMENT);
								++iNumPillaged;
								break;
							}
						}
					}
				}
			}

			PlayerTypes eOtherPlayer = kTriggeredData.m_eOtherPlayer;
			if (!kEvent.isCityEffect() && kEvent.isOtherPlayerCityEffect())
			{
				eOtherPlayer = kTriggeredData.m_ePlayer;
			}

			if (NO_PLAYER != eOtherPlayer)
			{
				CvWString szBuffer = gDLL->getText("TXT_KEY_EVENT_NUM_CITY_IMPROVEMENTS_DESTROYED", iNumPillaged, GET_PLAYER(getOwnerINLINE()).getCivilizationAdjectiveKey());
				gDLL->getInterfaceIFace()->addHumanMessage(eOtherPlayer, false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_PILLAGED", MESSAGE_TYPE_INFO);
			}
		}

		for (int i = 0; i < GC.getNumSpecialistInfos(); ++i)
		{
			if (kEvent.getFreeSpecialistCount(i) > 0)
			{
				changeFreeSpecialistCount((SpecialistTypes)i, kEvent.getFreeSpecialistCount(i));
			}
		}

		if (kEvent.getCulture() != 0)
		{
			changeCulture(getOwnerINLINE(), kEvent.getCulture(), true, true);
		}
	}


	if (kEvent.getUnitClass() != NO_UNITCLASS)
	{
		UnitTypes eUnit = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(kEvent.getUnitClass());
		if (eUnit != NO_UNIT)
		{
			for (int i = 0; i < kEvent.getNumUnits(); ++i)
			{
				GET_PLAYER(getOwnerINLINE()).initUnit(eUnit, getX_INLINE(), getY_INLINE());
			}
		}
	}

	if (kEvent.getBuildingClass() != NO_BUILDINGCLASS)
	{
		BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(kEvent.getBuildingClass());
		if (eBuilding != NO_BUILDING)
		{
			if (0 != kEvent.getBuildingChange())
			{
				setNumRealBuilding(eBuilding, getNumRealBuilding(eBuilding) + kEvent.getBuildingChange());
			}
		}
	}

	if (kEvent.getNumBuildingYieldChanges() > 0)
	{
		for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
		{
			for (int iYield = 0; iYield < NUM_YIELD_TYPES; ++iYield)
			{
				setBuildingYieldChange((BuildingClassTypes)iBuildingClass, (YieldTypes)iYield, getBuildingYieldChange((BuildingClassTypes)iBuildingClass, (YieldTypes)iYield) + kEvent.getBuildingYieldChange(iBuildingClass, iYield));
			}
		}
	}

	if (kEvent.getNumBuildingCommerceChanges() > 0)
	{
		for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
		{
			for (int iCommerce = 0; iCommerce < NUM_COMMERCE_TYPES; ++iCommerce)
			{
				setBuildingCommerceChange((BuildingClassTypes)iBuildingClass, (CommerceTypes)iCommerce, getBuildingCommerceChange((BuildingClassTypes)iBuildingClass, (CommerceTypes)iCommerce) + kEvent.getBuildingCommerceChange(iBuildingClass, iCommerce));
			}
		}
	}

	if (kEvent.getNumBuildingHappyChanges() > 0)
	{
		for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
		{
			setBuildingHappyChange((BuildingClassTypes)iBuildingClass, kEvent.getBuildingHappyChange(iBuildingClass));
		}
	}

	if (kEvent.getNumBuildingHealthChanges() > 0)
	{
		for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
		{
			setBuildingHealthChange((BuildingClassTypes)iBuildingClass, kEvent.getBuildingHealthChange(iBuildingClass));
		}
	}

	if (bClear)
	{
		for (int i = 0; i < GC.getNumEventInfos(); ++i)
		{
			setEventOccured((EventTypes)i, false);
		}
	}
}

bool CvCity::isEventOccured(EventTypes eEvent) const
{
	for (std::vector<EventTypes>::const_iterator it = m_aEventsOccured.begin(); it != m_aEventsOccured.end(); ++it)
	{
		if (*it == eEvent)
		{
			return true;
		}
	}

	return false;
}

void CvCity::setEventOccured(EventTypes eEvent, bool bOccured)
{
	for (std::vector<EventTypes>::iterator it = m_aEventsOccured.begin(); it != m_aEventsOccured.end(); ++it)
	{
		if (*it == eEvent)
		{
			if (!bOccured)
			{
				m_aEventsOccured.erase(it);
			}
			return;
		}
	}

	if (bOccured)
	{
		m_aEventsOccured.push_back(eEvent);
	}
}

bool CvCity::hasShrine(ReligionTypes eReligion) const
{
	bool bHasShrine = false;
	
	// note, for normal XML, this count will be one, there is only one shrine of each religion
	int	shrineBuildingCount = GC.getGameINLINE().getShrineBuildingCount(eReligion);
	for (int iI = 0; iI < shrineBuildingCount; iI++)
	{
		BuildingTypes eBuilding = GC.getGameINLINE().getShrineBuilding(iI, eReligion);

		if (getNumBuilding(eBuilding) > 0)
		{
			bHasShrine = true;
			break;
		}
	}

	return bHasShrine;
}

void CvCity::invalidatePopulationRankCache()
{
	m_bPopulationRankValid = false;
}

void CvCity::invalidateYieldRankCache(YieldTypes eYield)
{
	FAssertMsg(eYield >= NO_YIELD && eYield < NUM_YIELD_TYPES, "invalidateYieldRankCache passed bogus yield index");

	if (eYield == NO_YIELD)
	{
		for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			m_abBaseYieldRankValid[iI] = false;
			m_abYieldRankValid[iI] = false;
		}
	}
	else
	{
		m_abBaseYieldRankValid[eYield] = false;
		m_abYieldRankValid[eYield] = false;
	}
}

void CvCity::invalidateCommerceRankCache(CommerceTypes eCommerce)
{
	FAssertMsg(eCommerce >= NO_COMMERCE && eCommerce < NUM_COMMERCE_TYPES, "invalidateCommerceRankCache passed bogus commerce index"); // kmodx: was NO_YIELD and NUM_YIELD_TYPES

	if (eCommerce == NO_COMMERCE)
	{
		for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
		{
			m_abCommerceRankValid[iI] = false;
		}
	}
	else
	{
		m_abCommerceRankValid[eCommerce] = false;
	}
}

int CvCity::getBuildingYieldChange(BuildingClassTypes eBuildingClass, YieldTypes eYield) const
{
	for (std::vector<BuildingYieldChange>::const_iterator it = m_aBuildingYieldChange.begin(); it != m_aBuildingYieldChange.end(); ++it)
	{
		if ((*it).eBuildingClass == eBuildingClass && (*it).eYield == eYield)
		{
			return (*it).iChange;
		}
	}

	return 0;
}

void CvCity::setBuildingYieldChange(BuildingClassTypes eBuildingClass, YieldTypes eYield, int iChange)
{
	for (std::vector<BuildingYieldChange>::iterator it = m_aBuildingYieldChange.begin(); it != m_aBuildingYieldChange.end(); ++it)
	{
		if ((*it).eBuildingClass == eBuildingClass && (*it).eYield == eYield)
		{
			int iOldChange = (*it).iChange;
			if (iOldChange != iChange)
			{

				if (iChange == 0)
				{
					m_aBuildingYieldChange.erase(it);
				}
				else
				{
					(*it).iChange = iChange;
				}

				BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(eBuildingClass);
				if (NO_BUILDING != eBuilding)
				{
					if (getNumActiveBuilding(eBuilding) > 0)
					{
						changeBaseYieldRate(eYield, (iChange - iOldChange) * getNumActiveBuilding(eBuilding));
					}
				}
			}

			return;
		}
	}

	if (0 != iChange)
	{
		BuildingYieldChange kChange;
		kChange.eBuildingClass = eBuildingClass;
		kChange.eYield = eYield;
		kChange.iChange = iChange;
		m_aBuildingYieldChange.push_back(kChange);

		BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(eBuildingClass);
		if (NO_BUILDING != eBuilding)
		{
			if (getNumActiveBuilding(eBuilding) > 0)
			{
				changeBaseYieldRate(eYield, iChange * getNumActiveBuilding(eBuilding));
			}
		}
	}
}

void CvCity::changeBuildingYieldChange(BuildingClassTypes eBuildingClass, YieldTypes eYield, int iChange)
{
	setBuildingYieldChange(eBuildingClass, eYield, getBuildingYieldChange(eBuildingClass, eYield) + iChange);
}

int CvCity::getBuildingCommerceChange(BuildingClassTypes eBuildingClass, CommerceTypes eCommerce) const
{
	for (std::vector<BuildingCommerceChange>::const_iterator it = m_aBuildingCommerceChange.begin(); it != m_aBuildingCommerceChange.end(); ++it)
	{
		if ((*it).eBuildingClass == eBuildingClass && (*it).eCommerce == eCommerce)
		{
			return (*it).iChange;
		}
	}

	return 0;
}

void CvCity::setBuildingCommerceChange(BuildingClassTypes eBuildingClass, CommerceTypes eCommerce, int iChange)
{
	for (std::vector<BuildingCommerceChange>::iterator it = m_aBuildingCommerceChange.begin(); it != m_aBuildingCommerceChange.end(); ++it)
	{
		if ((*it).eBuildingClass == eBuildingClass && (*it).eCommerce == eCommerce)
		{
			if ((*it).iChange != iChange)
			{
				if (iChange == 0)
				{
					m_aBuildingCommerceChange.erase(it);
				}
				else
				{
					(*it).iChange = iChange;
				}

				updateBuildingCommerce();
			}

			return;
		}
	}

	if (0 != iChange)
	{
		BuildingCommerceChange kChange;
		kChange.eBuildingClass = eBuildingClass;
		kChange.eCommerce = eCommerce;
		kChange.iChange = iChange;
		m_aBuildingCommerceChange.push_back(kChange);

		updateBuildingCommerce();
	}
}

void CvCity::changeBuildingCommerceChange(BuildingClassTypes eBuildingClass, CommerceTypes eCommerce, int iChange)
{
	setBuildingCommerceChange(eBuildingClass, eCommerce, getBuildingCommerceChange(eBuildingClass, eCommerce) + iChange);
}


void CvCity::setBuildingHappyChange(BuildingClassTypes eBuildingClass, int iChange)
{
	for (BuildingChangeArray::iterator it = m_aBuildingHappyChange.begin(); it != m_aBuildingHappyChange.end(); ++it)
	{
		if ((*it).first == eBuildingClass)
		{
			if ((*it).second != iChange)
			{
				/*if ((*it).second > 0)
					changeBuildingGoodHappiness(-(*it).second);
				else if ((*it).second < 0)
					changeBuildingBadHappiness((*it).second);
				if (iChange == 0)
					m_aBuildingHappyChange.erase(it);
				else (*it).second = iChange;
				if (iChange > 0)
					changeBuildingGoodHappiness(iChange);
				else if (iChange < 0)
					changeBuildingGoodHappiness(-iChange);*/
				// UNOFFICIAL_PATCH (Bugfix), 10/22/09, jdog5000: START
				int iOldChange = (*it).second;

				m_aBuildingHappyChange.erase(it);

				BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(eBuildingClass);
				if (NO_BUILDING != eBuilding)
				{
					if (getNumActiveBuilding(eBuilding) > 0)
					{

						if (iOldChange > 0)
						{
							changeBuildingGoodHappiness(-iOldChange);
						}
						else if (iOldChange < 0)
						{
							changeBuildingBadHappiness(-iOldChange);
						}

						if( iChange != 0 )
						{
							m_aBuildingHappyChange.push_back(std::make_pair(eBuildingClass, iChange));
							if (iChange > 0)
							{
								changeBuildingGoodHappiness(iChange);
							}
							else if (iChange < 0)
							{
								changeBuildingBadHappiness(iChange);
							}
						}
					}
				}
				// UNOFFICIAL_PATCH: END
			}

			return;
		}
	}

	if (iChange != 0)
	{
		/*m_aBuildingHappyChange.push_back(std::make_pair(eBuildingClass, iChange));
		if (iChange > 0)
			changeBuildingGoodHappiness(iChange);
		else if (iChange < 0)
			changeBuildingGoodHappiness(-iChange);*/
		// UNOFFICIAL_PATCH (Bugfix), 10/22/09, jdog5000: START
		BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(eBuildingClass);
		if (NO_BUILDING != eBuilding)
		{
			if (getNumActiveBuilding(eBuilding) > 0)
			{
				m_aBuildingHappyChange.push_back(std::make_pair(eBuildingClass, iChange));

				if (iChange > 0)
				{
					changeBuildingGoodHappiness(iChange);
				}
				else if (iChange < 0)
				{
					changeBuildingBadHappiness(iChange);
				}
			}
		}
		// UNOFFICIAL_PATCH: END
	}
}


int CvCity::getBuildingHappyChange(BuildingClassTypes eBuildingClass) const
{
	for (BuildingChangeArray::const_iterator it = m_aBuildingHappyChange.begin(); it != m_aBuildingHappyChange.end(); ++it)
	{
		if ((*it).first == eBuildingClass)
		{
			return (*it).second;
		}
	}

	return 0;
}


void CvCity::setBuildingHealthChange(BuildingClassTypes eBuildingClass, int iChange)
{
	for (BuildingChangeArray::iterator it = m_aBuildingHealthChange.begin(); it != m_aBuildingHealthChange.end(); ++it)
	{
		if ((*it).first == eBuildingClass)
		{
			if ((*it).second != iChange)
			{
				/*if ((*it).second > 0)
					changeBuildingGoodHealth(-(*it).second);
				else if ((*it).second < 0)
					changeBuildingBadHealth((*it).second);
				if (iChange == 0)
					m_aBuildingHealthChange.erase(it);
				else (*it).second = iChange;
				if (iChange > 0)
					changeBuildingGoodHealth(iChange);
				else if (iChange < 0)
					changeBuildingBadHealth(-iChange);*/
				// UNOFFICIAL_PATCH, Bugfix, 10/22/09, jdog5000
				int iOldChange = (*it).second;

				m_aBuildingHealthChange.erase(it);

				BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(eBuildingClass);
				if (NO_BUILDING != eBuilding)
				{
					if (getNumActiveBuilding(eBuilding) > 0)
					{
						if (iOldChange > 0)
						{
							changeBuildingGoodHealth(-iOldChange);
						}
						else if (iOldChange < 0)
						{
							changeBuildingBadHealth(-iOldChange);
						}

						if( iChange != 0 )
						{
							m_aBuildingHealthChange.push_back(std::make_pair(eBuildingClass, iChange));
							if (iChange > 0)
							{
								changeBuildingGoodHealth(iChange);
							}
							else if (iChange < 0)
							{
								changeBuildingBadHealth(iChange);
							}
						}
					}
				}
				// UNOFFICIAL_PATCH: END
			}

			return;
		}
	}

	if (iChange != 0)
	{
		/*m_aBuildingHealthChange.push_back(std::make_pair(eBuildingClass, iChange));
		if (iChange > 0)
			changeBuildingGoodHappiness(iChange);
		else if (iChange < 0)
			changeBuildingGoodHappiness(-iChange);*/
		// UNOFFICIAL_PATCH, Bugfix, 10/22/09, jdog5000
		BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(eBuildingClass);
		if (NO_BUILDING != eBuilding)
		{
			if (getNumActiveBuilding(eBuilding) > 0)
			{	
				m_aBuildingHealthChange.push_back(std::make_pair(eBuildingClass, iChange));

				if (iChange > 0)
				{
					changeBuildingGoodHealth(iChange);
				}
				else if (iChange < 0)
				{
					changeBuildingBadHealth(iChange);
				}
			}
		}
		// UNOFFICIAL_PATCH: END
	}
}


int CvCity::getBuildingHealthChange(BuildingClassTypes eBuildingClass) const
{
	for (BuildingChangeArray::const_iterator it = m_aBuildingHealthChange.begin(); it != m_aBuildingHealthChange.end(); ++it)
	{
		if ((*it).first == eBuildingClass)
		{
			return (*it).second;
		}
	}

	return 0;
}

void CvCity::liberate(bool bConquest, /* advc.122: */ bool bCede)
{
	PlayerTypes ePlayer = getLiberationPlayer(bConquest);
	if(ePlayer == NO_PLAYER)
		return; // advc.003
	PlayerTypes eOwner = getOwnerINLINE();
	// dlph.23: No longer used
	/*CvPlot* pPlot = plot();
	int iOldOwnerCulture = getCultureTimes100(eOwner);
	bool bPreviouslyOwned = isEverOwned(ePlayer);*/ // K-Mod, for use below
	int iOldMasterLand = 0;
	int iOldVassalLand = 0;
	
	if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isVassal(GET_PLAYER(eOwner).getTeam()))
	{
		iOldMasterLand = GET_TEAM(GET_PLAYER(eOwner).getTeam()).getTotalLand();
		iOldVassalLand = std::max(10, // advc.112: Lower bound added
			GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getTotalLand(false));
	}

	CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_CITY_LIBERATED", getNameKey(), GET_PLAYER(eOwner).getNameKey(), GET_PLAYER(ePlayer).getCivilizationAdjectiveKey());
	for (int iI = 0; iI < MAX_PLAYERS; ++iI)
	{
		CvPlayer const& civ = GET_PLAYER((PlayerTypes)iI); // advc.003
		if (civ.isAlive())
		{
			if (isRevealed(civ.getTeam(), false)
					|| civ.isSpectator()) // advc.127
			{
				gDLL->getInterfaceIFace()->addHumanMessage(((PlayerTypes)iI), false, GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_REVOLTEND",
					MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY, // advc.106b
					ARTFILEMGR.getInterfaceArtInfo("WORLDBUILDER_CITY_EDIT")->getPath(), (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"), getX_INLINE(), getY_INLINE(), true, true);
			}
		}
	}
	GC.getGameINLINE().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, eOwner, szBuffer, getX_INLINE(), getY_INLINE(), (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));

	GET_PLAYER(ePlayer).acquireCity(this, false, true, true);
	if(!bCede) // advc.122
		GET_PLAYER(ePlayer).AI_rememberEvent(eOwner, MEMORY_LIBERATED_CITIES); // advc.130j
	// <advc.003>
	CvTeam& kTeam = TEAMREF(ePlayer);
	CvTeam const& kOwnerTeam = TEAMREF(eOwner); // </advc.003>
	if (kTeam.isVassal(kOwnerTeam.getID()))
	{
		int iNewMasterLand = kOwnerTeam.getTotalLand();
		int iNewVassalLand = std::max(10, // advc.112: Lower bound added
				kTeam.getTotalLand(false));

		kTeam.setMasterPower(kTeam.getMasterPower() + iNewMasterLand - iOldMasterLand);
		kTeam.setVassalPower(kTeam.getVassalPower() + iNewVassalLand - iOldVassalLand);
	}
	GET_PLAYER(ePlayer).AI_updateAttitudeCache(eOwner); // advc.122
	// dlph.23: Commented out. setCulture now done by advc.122 in acquireCity.
	/*if (NULL != pPlot) {
		CvCity* pCity = pPlot->getPlotCity();
		if (NULL != pCity) {
			// K-Mod, 7/jan/11, karadoc
			// This mechanic was exploitable. Players could increase their culture indefinitely in a single turn by gifting cities backwards and forwards.
			// I've attempted to close the exploit.
			if (!bPreviouslyOwned) // K-Mod
				pCity->setCultureTimes100(ePlayer, pCity->getCultureTimes100(ePlayer) + iOldOwnerCulture / 2, true, true);
			// K-Mod end
		}
		if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isAVassal()) {
			for (int i = 0; i < GC.getDefineINT("COLONY_NUM_FREE_DEFENDERS"); ++i)
				pCity->initConscriptedUnit();
		}
	}*/
}

PlayerTypes CvCity::getLiberationPlayer(bool bConquest,
		TeamTypes eWarTeam) const // advc.122
{
	if (isCapital())
	{
		return NO_PLAYER;
	}

	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kLoopPlayer.isAlive() && kLoopPlayer.getParent() == getOwnerINLINE())
		{
			CvCity* pLoopCapital = kLoopPlayer.getCapitalCity();
			if (NULL != pLoopCapital)
			{
				if (pLoopCapital->area() == area())
				{
					return (PlayerTypes)iPlayer;
				}
			}
		}
	}

	CvPlayer& kOwner = GET_PLAYER(getOwnerINLINE());
	if (kOwner.canSplitEmpire() && kOwner.canSplitArea(area()->getID()))
	{
		PlayerTypes ePlayer = GET_PLAYER(getOwnerINLINE()).getSplitEmpirePlayer(area()->getID());

		if (NO_PLAYER != ePlayer)
		{
			if (GET_PLAYER(ePlayer).isAlive())
			{
				return ePlayer;
			}
		}
	}

	PlayerTypes eBestPlayer = NO_PLAYER;
	int iBestValue = 0;

	int iTotalCultureTimes100 = countTotalCultureTimes100();
	// K-Mod - Base culture which is added to dilute the true culture values
	const int iBaseCulture = GC.getNumCultureLevelInfos() > 1
		? 50 * GC.getCultureLevelInfo((CultureLevelTypes)1).getSpeedThreshold(GC.getGameINLINE().getGameSpeedType())
		: 100;
	// K-Mod end

	// K-Mod. I've flattened the if blocks into if! continue conditions.
	// and I've changed the type of the iterator of the loop, from int to PlayerTypes

	//for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
	for (PlayerTypes ePlayer = (PlayerTypes)0; ePlayer < MAX_CIV_PLAYERS; ePlayer = (PlayerTypes)(ePlayer+1))
	{
		CvPlayerAI& kLoopPlayer = GET_PLAYER(ePlayer);

		if (!kLoopPlayer.isAlive())
			continue;

		if (!kLoopPlayer.canReceiveTradeCity())
			continue;

		CvCity* pCapital = kLoopPlayer.getCapitalCity();
		if (pCapital == NULL)
			continue;

		int iCapitalDistance = ::plotDistance(getX_INLINE(), getY_INLINE(),
				pCapital->getX_INLINE(), pCapital->getY_INLINE());
		if (area() != pCapital->area())
		{
			iCapitalDistance *= 2;
		}

		//int iCultureTimes100 = getCultureTimes100(ePlayer);
		int iCultureTimes100 = iBaseCulture + getCultureTimes100(ePlayer);// K-Mod

		if (bConquest)
		{
			if (ePlayer == getOriginalOwner())
			{
				iCultureTimes100 *= 3;
				iCultureTimes100 /= 2;
			}
		}

		if (kLoopPlayer.getTeam() == getTeam()
				|| GET_TEAM(kLoopPlayer.getTeam()).isVassal(getTeam())
				|| GET_TEAM(getTeam()).isVassal(kLoopPlayer.getTeam()))
		{
			// K-Mod: I don't see why the total culture should be used in this way. (I haven't changed anything)
			/*iCultureTimes100 *= 2;
			iCultureTimes100 = (iCultureTimes100 + iTotalCultureTimes100) / 2;*/
			// advc.003: Simplified (no functional change)
			iCultureTimes100 += iTotalCultureTimes100 / 2;
		}

		// K-Mod - adjust culture score based on plot ownership.
		iCultureTimes100 *= 100 + plot()->calculateTeamCulturePercent(kLoopPlayer.getTeam());
		iCultureTimes100 /= 100;
		// K-Mod end

		//int iValue = std::max(100, iCultureTimes100) / std::max(1, iCapitalDistance);
		int iValue = iCultureTimes100 / std::max(1, iCapitalDistance); // K-Mod (minimum culture moved higher up)

		if (iValue > iBestValue)
		{
			iBestValue = iValue;
			eBestPlayer = ePlayer;
		}
	}

	if (NO_PLAYER != eBestPlayer)
	{
		if (getOwnerINLINE() == eBestPlayer)
		{
			return NO_PLAYER;
		}

		for (int iPlot = 0; iPlot < NUM_CITY_PLOTS; ++iPlot)
		{
			CvPlot* pLoopPlot = ::plotCity(getX_INLINE(), getY_INLINE(), iPlot);

			if (NULL != pLoopPlot)
			{	// advc.122: was VisibleEnemyUnit; and eWarTeam added.
				if (pLoopPlot->isVisibleEnemyCityAttacker(eBestPlayer, eWarTeam))
				{
					return NO_PLAYER;
				}
			}
		}
	}

	return eBestPlayer;
}

// advc.003j (comment): Unused since karadoc rewrote CvCityAI::AI_yieldValue
int CvCity::getBestYieldAvailable(YieldTypes eYield) const
{
	int iBestYieldAvailable = 0;

	for (int iJ = 0; iJ < NUM_CITY_PLOTS; ++iJ)
	{
		if (iJ != CITY_HOME_PLOT)
		{
			if (!isWorkingPlot(iJ))
			{
				CvPlot* pPlot = getCityIndexPlot(iJ);

				if (NULL != pPlot && canWork(pPlot))
				{
					if (pPlot->getYield(eYield) > iBestYieldAvailable)
					{
						iBestYieldAvailable = pPlot->getYield(eYield);
					}
				}
			}
		}
	}

	for (int iJ = 0; iJ < GC.getNumSpecialistInfos(); ++iJ)
	{
		if (isSpecialistValid((SpecialistTypes)iJ, 1))
		{
			int iYield = GC.getSpecialistInfo((SpecialistTypes)iJ).getYieldChange(eYield);
			if (iYield > iBestYieldAvailable)
			{
				iBestYieldAvailable = iYield;
			}
		}
	}

	return iBestYieldAvailable;
}

bool CvCity::isAutoRaze() const
{
	if (!GC.getGameINLINE().isOption(GAMEOPTION_NO_CITY_RAZING))
	{
		if (getHighestPopulation() == 1)
		{
			return true;
		}

		if (GC.getGameINLINE().getMaxCityElimination() > 0)
		{
			return true;
		}
	}

	if (GC.getGameINLINE().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
	{
		return true;
	}

	return false;
}

int CvCity::getMusicScriptId() const
{	// <advc.001p>
	if(getOwnerINLINE() == NO_PLAYER)
		return 0; // </advc.001p>
	bool bHappy = true;
	if (GC.getGameINLINE().getActiveTeam() == getTeam())
	{
		if (angryPopulation() > 0)
			bHappy = false;
	}
	else
	{			
		if (GET_TEAM(GC.getGameINLINE().getActiveTeam()).isAtWar(getTeam()))
			bHappy = false;
	}
	CvLeaderHeadInfo& kLeaderInfo = GC.getLeaderHeadInfo(GET_PLAYER(getOwnerINLINE()).getLeaderType());
	EraTypes eCurEra = GET_PLAYER(getOwnerINLINE()).getCurrentEra();
	if (bHappy)
		return (kLeaderInfo.getDiploPeaceMusicScriptIds(eCurEra));
	else return (kLeaderInfo.getDiploWarMusicScriptIds(eCurEra));
}

int CvCity::getSoundscapeScriptId() const
{
	return GC.getEraInfo(GET_PLAYER(getOwnerINLINE()).getCurrentEra()).getCitySoundscapeSciptId(getCitySizeType());
}

void CvCity::cheat(bool bCtrl, bool bAlt, bool bShift)
{
	//if (gDLL->getChtLvl() > 0)
	if(GC.getGameINLINE().isDebugMode()) // advc.007b
	{
		if (bCtrl)
		{
			changeCulture(getOwnerINLINE(), 10, true, true);
		}
		else if (bShift)
		{
			changePopulation(1);
		}
		else
		{
			popOrder(0, true);
		}
	}
}

void CvCity::getBuildQueue(std::vector<std::string>& astrQueue) const
{
	CLLNode<OrderData>* pNode = headOrderQueueNode();
	while (pNode != NULL)
	{
		switch (pNode->m_data.eOrderType)
		{
		case ORDER_TRAIN:
			astrQueue.push_back(GC.getUnitInfo((UnitTypes)(pNode->m_data.iData1)).getType());
			break;

		case ORDER_CONSTRUCT:
			astrQueue.push_back(GC.getBuildingInfo((BuildingTypes)(pNode->m_data.iData1)).getType());
			break;

		case ORDER_CREATE:
			astrQueue.push_back(GC.getProjectInfo((ProjectTypes)(pNode->m_data.iData1)).getType());
			break;

		case ORDER_MAINTAIN:
			astrQueue.push_back(GC.getProcessInfo((ProcessTypes)(pNode->m_data.iData1)).getType());
			break;

		default:
			FAssert(false);
			break;
		}

		pNode = nextOrderQueueNode(pNode);
	}
}

// <advc.004b> See CvCity.h
int CvCity::initialPopulation() {

	return GC.getDefineINT("INITIAL_CITY_POPULATION") +
			GC.getEraInfo(GC.getGameINLINE().getStartEra()).getFreePopulation();
}

// advc.004b, advc.104: Parameters added
int CvCity::calculateDistanceMaintenanceTimes100(CvPlot const& kCityPlot,
		PlayerTypes eOwner, int iPopulation) {

	if(iPopulation < 0)
		iPopulation = initialPopulation();
	/* original bts code
	CvCity* pLoopCity;
	int iWorstCityMaintenance;
	int iBestCapitalMaintenance;
	int iTempMaintenance;
	int iLoop;
	iWorstCityMaintenance = 0;
	iBestCapitalMaintenance = MAX_INT;
	for (pLoopCity = GET_PLAYER(getOwnerINLINE()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(getOwnerINLINE()).nextCity(&iLoop))
		iTempMaintenance = 100 * (GC.getDefineINT("MAX_DISTANCE_CITY_MAINTENANCE") * plotDistance(getX_INLINE(), getY_INLINE(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE()));*/
	// K-Mod, 17/dec/10
	// Moved the search for maintenance distance to a separate function and improved the efficiency
	int iTempMaintenance = 100 * GC.getMAX_DISTANCE_CITY_MAINTENANCE() *
			calculateMaintenanceDistance(&kCityPlot, eOwner);
	// unaltered bts code
		iTempMaintenance *= (iPopulation + 7);
		iTempMaintenance /= 10;

		iTempMaintenance *= std::max(0, (GET_PLAYER(eOwner).getDistanceMaintenanceModifier() + 100));
		iTempMaintenance /= 100;

		iTempMaintenance *= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getDistanceMaintenancePercent();
		iTempMaintenance /= 100;

		iTempMaintenance *= GC.getHandicapInfo(GET_PLAYER(eOwner).getHandicapType()).getDistanceMaintenancePercent();
		iTempMaintenance /= 100;

		iTempMaintenance /= GC.getMapINLINE().maxPlotDistance();

	/* original bts code
		iWorstCityMaintenance = std::max(iWorstCityMaintenance, iTempMaintenance);
		if (pLoopCity->isGovernmentCenter())
			iBestCapitalMaintenance = std::min(iBestCapitalMaintenance, iTempMaintenance);
	}
	iTempMaintenance = std::min(iWorstCityMaintenance, iBestCapitalMaintenance);*/
	// K-Mod end
	FAssert(iTempMaintenance >= 0);
	return iTempMaintenance;
}

// K-Mod. new function to help with maintenance calculations
// advc.004b, advc.104: Parameters added
int CvCity::calculateMaintenanceDistance(CvPlot const* pCityPlot, PlayerTypes eOwner)
{
	int x = pCityPlot->getX_INLINE(), y = pCityPlot->getY_INLINE();

	int iLongest = 0;
	int iShortestGovernment = MAX_INT;
	int iLoop;
	for (CvCity* pLoopCity = GET_PLAYER(eOwner).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(eOwner).nextCity(&iLoop))
	{
		int d = plotDistance(x, y, pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE());
		iLongest = std::max(iLongest, d);
		if(pLoopCity->isGovernmentCenter())
			iShortestGovernment = std::min(iShortestGovernment, d);
	}
	// <advc.140> Upper bound added
	int r = std::min(iLongest, iShortestGovernment);
	int iCap = GC.getMapINLINE().maxMaintenanceDistance();
	return std::min(r, iCap);
	// </advc.140>
}
// K-mod end

// advc.004b, advc.104: Parameters added
int CvCity::calculateNumCitiesMaintenanceTimes100(CvPlot const& kCityPlot,
		PlayerTypes eOwner, int iPopulation, int iExtraCities) {
	
	if(iPopulation < 0)
		iPopulation = initialPopulation();

	int iNumCitiesPercent = 100;

	iNumCitiesPercent *= (iPopulation + 17);
	iNumCitiesPercent /= 18;

	iNumCitiesPercent *= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).
			getNumCitiesMaintenancePercent();
	iNumCitiesPercent /= 100;

	iNumCitiesPercent *= GC.getHandicapInfo(GET_PLAYER(eOwner).getHandicapType()).
			getNumCitiesMaintenancePercent();
	iNumCitiesPercent /= 100;

	int iNumVassalCities = 0;
	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kLoopPlayer.getTeam() != TEAMID(eOwner) &&
				GET_TEAM(kLoopPlayer.getTeam()).isVassal(TEAMID(eOwner)))
			iNumVassalCities += kLoopPlayer.getNumCities();
	}

	iNumVassalCities /= std::max(1, TEAMREF(eOwner).getNumMembers());
	/* original BTS code
	int iNumCitiesMaintenance = (GET_PLAYER(getOwnerINLINE()).getNumCities() + iNumVassalCities) * iNumCitiesPercent;
	iNumCitiesMaintenance = std::min(iNumCitiesMaintenance, GC.getHandicapInfo(getHandicapType()).getMaxNumCitiesMaintenance() * 100); */
	// K-Mod, 04/sep/10, karadoc
	// Reduced vassal maintenance and removed maintenance cap
	int iNumCitiesMaintenance = (GET_PLAYER(eOwner).getNumCities()
			+ iExtraCities + iNumVassalCities / 2) * iNumCitiesPercent;
	// K-Mod end

	iNumCitiesMaintenance *= std::max(0, (GET_PLAYER(eOwner).getNumCitiesMaintenanceModifier() + 100));
	iNumCitiesMaintenance /= 100;

	FAssert(iNumCitiesMaintenance >= 0);

	return iNumCitiesMaintenance;
}

// advc.004b, advc.104: Parameters added
int CvCity::calculateColonyMaintenanceTimes100(CvPlot const& kCityPlot,
		PlayerTypes eOwner, int iPopulation, int iExtraCities) {

	if(iPopulation < 0)
		iPopulation = initialPopulation();
	HandicapTypes eOwnerHandicap = GET_PLAYER(eOwner).getHandicapType();
	CvArea* pCityArea = kCityPlot.area();

	if (GC.getGameINLINE().isOption(GAMEOPTION_NO_VASSAL_STATES))
		return 0;

	CvCity* pCapital = GET_PLAYER(eOwner).getCapitalCity();
	if (pCapital && pCapital->area() == pCityArea)
		return 0;

	int iNumCitiesPercent = 100;

	iNumCitiesPercent *= (iPopulation + 17);
	iNumCitiesPercent /= 18;

	iNumCitiesPercent *= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getColonyMaintenancePercent();
	iNumCitiesPercent /= 100;

	iNumCitiesPercent *= GC.getHandicapInfo(eOwnerHandicap).getColonyMaintenancePercent();
	iNumCitiesPercent /= 100;

	int iNumCities = (pCityArea->getCitiesPerPlayer(eOwner) - 1 + iExtraCities) *
			iNumCitiesPercent;
	int iMaintenance = (iNumCities * iNumCities) / 100;
	/* original bts code
	iMaintenance = std::min(iMaintenance, (GC.getHandicapInfo(getHandicapType()).getMaxColonyMaintenance() * calculateDistanceMaintenanceTimes100()) / 100);*/
	/*  K-Mod, 17/dec/10, karadoc
		Changed colony maintenance cap to not include distance maintenance modifiers (such as state property) */
	int iMaintenanceCap = 100 * GC.getMAX_DISTANCE_CITY_MAINTENANCE() *
			calculateMaintenanceDistance(&kCityPlot, eOwner);

	iMaintenanceCap *= (iPopulation + 7);
	iMaintenanceCap /= 10;

	iMaintenanceCap *= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).
			getDistanceMaintenancePercent();
	iMaintenanceCap /= 100;

	iMaintenanceCap *= GC.getHandicapInfo(eOwnerHandicap).
			getDistanceMaintenancePercent();
	iMaintenanceCap /= 100;

	iMaintenanceCap /= GC.getMapINLINE().maxPlotDistance();

	iMaintenanceCap *= GC.getHandicapInfo(eOwnerHandicap).getMaxColonyMaintenance();
	iMaintenanceCap /= 100;

	iMaintenance = std::min(iMaintenance, iMaintenanceCap);
	// K-Mod end
	FAssert(iMaintenance >= 0);
	return iMaintenance;
}
// </advc.004b>

// <advc.500b>
double CvCity::garrisonStrength() const {

	double r = 0;
	for(int i = 0; i < plot()->getNumUnits(); i++) {
		CvUnit* pu = plot()->getUnitByIndex(i);
		if(pu == NULL)
			continue;
		CvUnitInfo const& u = pu->getUnitInfo();
		// Excludes Scout, Explorer, Gunship, naval units
		if(!u.isMilitaryHappiness())
			continue;
		double defStr = u.getCombat();
		CvPlayer const& kOwner = GET_PLAYER(getOwnerINLINE());
		int iModifier = getBuildingDefense(); // e.g. Walls
		iModifier += kOwner.getCityDefenseModifier(); // e.g. Chichen Itza
		// CvPlot::getDefense doesn't deliver exactly what's needed
		if(plot()->isHills())
			iModifier += GC.getHILLS_EXTRA_DEFENSE() + u.getHillsDefenseModifier();
		iModifier += u.getCityDefenseModifier();
		// Combat and Garrison promotions
		for(int j = 0; j < GC.getNumPromotionInfos(); j++) {
			PromotionTypes ePromotion = (PromotionTypes)j;
			if(!pu->isHasPromotion(ePromotion))
				continue;
			CvPromotionInfo const& kPromotion = GC.getPromotionInfo(ePromotion);
			iModifier += kPromotion.getCombatPercent();
			iModifier += kPromotion.getCityDefensePercent();
		}
		defStr *= 1 + iModifier / 100.0;
		// Outdated units count half
		if(allUpgradesAvailable(pu->getUnitType()) != NO_UNIT)
			defStr /= 2;
		r += defStr;
	}
	return r;
} // </advc.500b>

// <advc.123f>
void CvCity::failProduction(int iOrderData, int iInvestedProduction,
		bool bProject) {

	// Based on code in doCheckProduction
	if(bProject)
		setProjectProduction((ProjectTypes)iOrderData, 0);
	else setBuildingProduction((BuildingTypes)iOrderData, 0);
	OrderTypes eOrderType = (bProject ? ORDER_CREATE : ORDER_CONSTRUCT);
	for(int i = getOrderQueueLength() - 1; i >= 0; i--) {
		OrderData* od = getOrderFromQueue(i);
		if(od != NULL && od->eOrderType == eOrderType && od->iData1 == iOrderData) {
			FAssert(!canContinueProduction(*od));
			popOrder(i, false, true);
		}
	}
	int iGoldPercent = failGoldPercent(eOrderType);
	if(iGoldPercent <= 0)
		return;
	int iProductionGold = (iInvestedProduction * iGoldPercent) / 100;
	GET_PLAYER(getOwnerINLINE()).changeGold(iProductionGold);
	CvWString msg = gDLL->getText("TXT_KEY_MISC_LOST_WONDER_PROD_CONVERTED",
			getNameKey(), bProject ?
			GC.getProjectInfo((ProjectTypes)iOrderData).getTextKeyWide() :
			GC.getBuildingInfo((BuildingTypes)iOrderData).getTextKeyWide(),
			iProductionGold);
	gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), false,
			GC.getEVENT_MESSAGE_TIME(), msg, "AS2D_WONDERGOLD",
			MESSAGE_TYPE_MINOR_EVENT, GC.getCommerceInfo(COMMERCE_GOLD).
			getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_RED"),
			getX_INLINE(), getY_INLINE(), true, true);
} // </advc.123f>

// <advc.064b>
int CvCity::failGoldPercent(OrderTypes eOrder) const { // Fail and overflow gold

	/*  To make any future changes to the process conversion rates easier, tie
		fail gold to the Wealth process. */
	int r = GC.getProcessInfo((ProcessTypes)0).getProductionToCommerceModifier(COMMERCE_GOLD);
	switch(eOrder) {
	case ORDER_TRAIN: r *= GC.getDefineINT("MAXED_UNIT_GOLD_PERCENT"); break;
	case ORDER_CONSTRUCT: r *= GC.getDefineINT("MAXED_BUILDING_GOLD_PERCENT"); break;
	case ORDER_CREATE: r *= GC.getDefineINT("MAXED_PROJECT_GOLD_PERCENT"); break;
	default: r *= 100; FAssert(false);
	}
	return r / 100;
}


void CvCity::handleOverflow(int iRawOverflow, int iProductionModifier, OrderTypes eOrderType) {

	if(iRawOverflow < 0) // Can happen through the "+" cheat (Debug mode)
		return;
	FAssert(getOverflowProduction() == 0);
	int iProductionGold = 0;
	int iLostProduction = 0;
	int iOverflow = computeOverflow(iRawOverflow, iProductionModifier, eOrderType,
			&iProductionGold, &iLostProduction);
	if(iOverflow <= 0)
		return;
	setOverflowProduction(iOverflow);
	if(iProductionGold > 0 || iLostProduction > 0)
		payOverflowGold(iLostProduction, iProductionGold);
}


void CvCity::payOverflowGold(int iLostProduction, int iProductionGold) {

	CvPlayer& kOwner = GET_PLAYER(getOwnerINLINE());
	kOwner.changeGold(iProductionGold);
	CvWString szMsg(gDLL->getText("TXT_KEY_MISC_OVERFLOW_GOLD", iLostProduction,
			getNameKey(), iProductionGold));
	gDLL->getInterfaceIFace()->addHumanMessage(getOwnerINLINE(), false,
			GC.getEVENT_MESSAGE_TIME(), szMsg, "AS2D_WONDERGOLD",
			MESSAGE_TYPE_INFO, GC.getCommerceInfo(COMMERCE_GOLD).
			getButton(), NO_COLOR, getX_INLINE(), getY_INLINE(), false, false);
} // </advc.064b>
