// team.cpp

#include "CvGameCoreDLL.h"
#include "CvTeam.h"
#include "CvGamePlay.h"
#include "CvDealList.h" // advc.003s
#include "WarAndPeaceAgent.h" // advc.104t
#include "CvMap.h"
#include "CvAreaList.h" // advc.003s
#include "CvInfo_City.h"
#include "CvInfo_Terrain.h"
#include "CvInfo_GameOption.h"
#include "CvInfo_Civics.h"
#include "CvDiploParameters.h"
#include "CvPopupInfo.h"
#include "BBAILog.h" // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
#include "BBAI_Defines.h"
#include "CvBugOptions.h" // advc.071

// <dlph.26> "Initializations of static variables."
std::queue<TeamTypes> CvTeam::attacking_queue;
std::queue<TeamTypes> CvTeam::defending_queue;
std::queue<bool> CvTeam::newdiplo_queue;
std::queue<WarPlanTypes> CvTeam::warplan_queue;
std::queue<bool> CvTeam::primarydow_queue;
bool CvTeam::bTriggeringWars = false;
// </dlph.26>

CvTeam::CvTeam()
{
	m_aiStolenVisibilityTimer = new int[MAX_TEAMS];
	m_aiWarWeariness = new int[MAX_TEAMS];
	m_aiTechShareCount = new int[MAX_TEAMS];
	m_aiCommerceFlexibleCount = new int[NUM_COMMERCE_TYPES];
	m_aiExtraMoves = new int[NUM_DOMAIN_TYPES];

	m_aiEspionagePointsAgainstTeam = new int[MAX_TEAMS];
	m_aiCounterespionageTurnsLeftAgainstTeam = new int[MAX_TEAMS];
	m_aiCounterespionageModAgainstTeam = new int[MAX_TEAMS];

	m_abAtWar = new bool[MAX_TEAMS];
	m_abHasMet = new bool[MAX_TEAMS];
	m_abHasSeen = new bool[MAX_TEAMS]; // K-Mod
	m_abPermanentWarPeace = new bool[MAX_TEAMS];
	m_abOpenBorders = new bool[MAX_TEAMS];
	m_abDefensivePact = new bool[MAX_TEAMS];
	m_abForcePeace = new bool[MAX_TEAMS];
	m_abVassal = new bool[MAX_TEAMS];
	// <advc.opt>
	m_eMaster = NO_TEAM;
	m_eLeader = NO_PLAYER;
	// </advc.opt>
	m_abCanLaunch = NULL;

	m_paiRouteChange = NULL;
	m_paiProjectCount = NULL;
	m_paiProjectDefaultArtTypes = NULL;
	m_pavProjectArtTypes = NULL;
	m_paiProjectMaking = NULL;
	m_paiUnitClassCount = NULL;
	m_paiBuildingClassCount = NULL;
	m_paiObsoleteBuildingCount = NULL;
	m_paiResearchProgress = NULL;
	m_paiTechCount = NULL;
	m_paiTerrainTradeCount = NULL;
	m_aiVictoryCountdown = NULL;
	m_aiForceTeamVoteEligibilityCount = NULL;

	m_pabHasTech = NULL;
	m_pabNoTradeTech = NULL;

	m_ppaaiImprovementYieldChange = NULL;

	reset((TeamTypes)0, true);
}


CvTeam::~CvTeam()
{
	uninit();

	SAFE_DELETE_ARRAY(m_aiStolenVisibilityTimer);
	SAFE_DELETE_ARRAY(m_aiWarWeariness);
	SAFE_DELETE_ARRAY(m_aiTechShareCount);
	SAFE_DELETE_ARRAY(m_aiCommerceFlexibleCount);
	SAFE_DELETE_ARRAY(m_aiExtraMoves);
	SAFE_DELETE_ARRAY(m_aiEspionagePointsAgainstTeam);
	SAFE_DELETE_ARRAY(m_aiCounterespionageTurnsLeftAgainstTeam);
	SAFE_DELETE_ARRAY(m_aiCounterespionageModAgainstTeam);
	SAFE_DELETE_ARRAY(m_abAtWar);
	SAFE_DELETE_ARRAY(m_abHasMet);
	SAFE_DELETE_ARRAY(m_abHasSeen); // K-Mod
	SAFE_DELETE_ARRAY(m_abPermanentWarPeace);
	SAFE_DELETE_ARRAY(m_abOpenBorders);
	SAFE_DELETE_ARRAY(m_abDefensivePact);
	SAFE_DELETE_ARRAY(m_abForcePeace);
	SAFE_DELETE_ARRAY(m_abVassal);
}


void CvTeam::init(TeamTypes eID)
{
	reset(eID); // Reset serialized data

	AI_init();

	// BETTER_BTS_AI_MOD 12/30/08 jdog5000
	if(GC.getGame().isFinalInitialized()) {
		// advc (note): This is for teams spawned through liberation
		for(int i = 0; i < MAX_TEAMS; i++) {
			CvTeam& kTarget = GET_TEAM((TeamTypes)i);
			if(i == getID() || !kTarget.isAlive()) // advc.003m: Alive check added
				continue;
			if(kTarget.isMinorCiv() || i == BARBARIAN_TEAM)
				kTarget.declareWar(getID(), false, WARPLAN_LIMITED);
		}
	} // BETTER_BTS_AI_MOD END
}


void CvTeam::uninit()
{
	SAFE_DELETE_ARRAY(m_abCanLaunch);

	SAFE_DELETE_ARRAY(m_paiRouteChange);
	SAFE_DELETE_ARRAY(m_paiProjectCount);
	SAFE_DELETE_ARRAY(m_paiProjectDefaultArtTypes);
	SAFE_DELETE_ARRAY(m_pavProjectArtTypes);
	SAFE_DELETE_ARRAY(m_paiProjectMaking);
	SAFE_DELETE_ARRAY(m_paiUnitClassCount);
	SAFE_DELETE_ARRAY(m_paiBuildingClassCount);
	SAFE_DELETE_ARRAY(m_paiObsoleteBuildingCount);
	SAFE_DELETE_ARRAY(m_paiResearchProgress);
	SAFE_DELETE_ARRAY(m_paiTechCount);
	SAFE_DELETE_ARRAY(m_paiTerrainTradeCount);
	SAFE_DELETE_ARRAY(m_aiVictoryCountdown);
	SAFE_DELETE_ARRAY(m_aiForceTeamVoteEligibilityCount);

	SAFE_DELETE_ARRAY(m_pabHasTech);
	SAFE_DELETE_ARRAY(m_pabNoTradeTech);

	if (m_ppaaiImprovementYieldChange != NULL)
	{
		for (int iI = 0; iI < GC.getNumImprovementInfos(); iI++)
		{
			SAFE_DELETE_ARRAY(m_ppaaiImprovementYieldChange[iI]);
		}
		SAFE_DELETE_ARRAY(m_ppaaiImprovementYieldChange);
	}
}

// Initializes data members that are serialized.
void CvTeam::reset(TeamTypes eID, bool bConstructorCall)
{
	int iI, iJ;

	uninit();

	m_iNumMembers = 0;
	m_iAliveCount = 0;
	m_iEverAliveCount = 0;
	m_iNumCities = 0;
	m_iTotalPopulation = 0;
	m_iTotalLand = 0;
	m_iNukeInterception = 0;
	m_iExtraWaterSeeFromCount = 0;
	m_iMapTradingCount = 0;
	m_iTechTradingCount = 0;
	m_iGoldTradingCount = 0;
	m_iOpenBordersTradingCount = 0;
	m_iDefensivePactTradingCount = 0;
	m_iPermanentAllianceTradingCount = 0;
	m_iVassalTradingCount = 0;
	m_iBridgeBuildingCount = 0;
	m_iIrrigationCount = 0;
	m_iIgnoreIrrigationCount = 0;
	m_iWaterWorkCount = 0;
	m_iVassalPower = 0;
	m_iMasterPower = 0;
	m_iEnemyWarWearinessModifier = 0;
	m_iRiverTradeCount = 0;
	m_iEspionagePointsEver = 0;
	// <advc.003m>
	m_iMajorWarEnemies = m_iMinorWarEnemies = m_iVassalWarEnemies = 0;
	m_bMinorTeam = false; // </advc.003m>
	m_bMapCentering = false;
	m_bCapitulated = false;
	m_bAnyVictoryCountdown = false; // advc.opt

	m_eID = eID;
	// <advc.134a>
	m_eOfferingPeace = NO_TEAM;
	m_iPeaceOfferStage = 0;
	// </advc.134a>
	// <advc.opt>
	m_eMaster = NO_TEAM;
	m_eLeader = NO_PLAYER;
	// </advc.opt>
	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		m_aiStolenVisibilityTimer[iI] = 0;
		m_aiWarWeariness[iI] = 0;
		m_aiTechShareCount[iI] = 0;
		m_aiEspionagePointsAgainstTeam[iI] = 0;
		m_aiCounterespionageTurnsLeftAgainstTeam[iI] = 0;
		m_aiCounterespionageModAgainstTeam[iI] = 0;
		m_abHasMet[iI] = false;
		m_abHasSeen[iI] = false; // K-Mod
		m_abAtWar[iI] = false;
		m_abJustDeclaredWar[iI] = false; // advc.162
		m_abPermanentWarPeace[iI] = false;
		m_abOpenBorders[iI] = false;
		m_abDisengage[iI] = false; // advc.034
		m_abDefensivePact[iI] = false;
		m_abForcePeace[iI] = false;
		m_abVassal[iI] = false;

		if (!bConstructorCall && getID() != NO_TEAM)
		{
			CvTeam& kLoopTeam = GET_TEAM((TeamTypes) iI);
			kLoopTeam.m_aiStolenVisibilityTimer[getID()] = 0;
			kLoopTeam.m_aiWarWeariness[getID()] = 0;
			kLoopTeam.m_aiTechShareCount[getID()] = 0;
			kLoopTeam.m_aiEspionagePointsAgainstTeam[getID()] = 0;
			kLoopTeam.m_aiCounterespionageTurnsLeftAgainstTeam[getID()] = 0;
			kLoopTeam.m_aiCounterespionageModAgainstTeam[getID()] = 0;
			kLoopTeam.m_abHasMet[getID()] = false;
			kLoopTeam.m_abHasSeen[getID()] = false; // K-Mod
			kLoopTeam.m_abAtWar[getID()] = false;
			kLoopTeam.m_abJustDeclaredWar[getID()] = false; // advc.162
			kLoopTeam.m_abPermanentWarPeace[getID()] = false;
			kLoopTeam.m_abOpenBorders[getID()] = false;
			kLoopTeam.m_abDisengage[getID()] = false; // advc.034
			kLoopTeam.m_abDefensivePact[getID()] = false;
			kLoopTeam.m_abForcePeace[getID()] = false;
			kLoopTeam.m_abVassal[getID()] = false;
		}
	}

	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		m_aiCommerceFlexibleCount[iI] = 0;
	}

	for (iI = 0; iI < NUM_DOMAIN_TYPES; iI++)
	{
		m_aiExtraMoves[iI] = 0;
	}

	if (!bConstructorCall)
	{
		FAssertMsg(m_abCanLaunch==NULL, "about to leak memory, CvTeam::m_abCanLaunch");
		m_abCanLaunch = new bool[GC.getNumVictoryInfos()];
		for (iI = 0; iI < GC.getNumVictoryInfos(); iI++)
		{
			m_abCanLaunch[iI] = false;
		}

		FAssertMsg(m_paiRouteChange==NULL, "about to leak memory, CvTeam::m_paiRouteChange");
		m_paiRouteChange = new int[GC.getNumRouteInfos()];
		for (iI = 0; iI < GC.getNumRouteInfos(); iI++)
		{
			m_paiRouteChange[iI] = 0;
		}

		FAssertMsg(m_paiProjectCount==NULL, "about to leak memory, CvPlayer::m_paiProjectCount");
		m_paiProjectCount = new int [GC.getNumProjectInfos()];
		FAssertMsg(m_paiProjectDefaultArtTypes==NULL, "about to leak memory, CvPlayer::m_paiProjectDefaultArtTypes");
		m_paiProjectDefaultArtTypes = new int [GC.getNumProjectInfos()];
		FAssertMsg(m_pavProjectArtTypes==NULL, "about to leak memory, CvPlayer::m_pavProjectArtTypes");
		m_pavProjectArtTypes = new std::vector<int> [GC.getNumProjectInfos()];
		FAssertMsg(m_paiProjectMaking==NULL, "about to leak memory, CvPlayer::m_paiProjectMaking");
		m_paiProjectMaking = new int [GC.getNumProjectInfos()];
		for (iI = 0; iI < GC.getNumProjectInfos(); iI++)
		{
			m_paiProjectCount[iI] = 0;
			m_paiProjectDefaultArtTypes[iI] = 0;
			m_paiProjectMaking[iI] = 0;
		}

		FAssertMsg(m_paiUnitClassCount==NULL, "about to leak memory, CvTeam::m_paiUnitClassCount");
		m_paiUnitClassCount = new int [GC.getNumUnitClassInfos()];
		for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
		{
			m_paiUnitClassCount[iI] = 0;
		}

		FAssertMsg(m_paiBuildingClassCount==NULL, "about to leak memory, CvTeam::m_paiBuildingClassCount");
		m_paiBuildingClassCount = new int [GC.getNumBuildingClassInfos()];
		for (iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
		{
			m_paiBuildingClassCount[iI] = 0;
		}

		FAssertMsg(m_paiObsoleteBuildingCount==NULL, "about to leak memory, CvTeam::m_paiObsoleteBuildingCount");
		m_paiObsoleteBuildingCount = new int[GC.getNumBuildingInfos()];
		for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
		{
			m_paiObsoleteBuildingCount[iI] = 0;
		}

		FAssertMsg(m_paiResearchProgress==NULL, "about to leak memory, CvPlayer::m_paiResearchProgress");
		m_paiResearchProgress = new int [GC.getNumTechInfos()];
		FAssertMsg(m_paiTechCount==NULL, "about to leak memory, CvPlayer::m_paiTechCount");
		m_paiTechCount = new int [GC.getNumTechInfos()];
		for (iI = 0; iI < GC.getNumTechInfos(); iI++)
		{
			m_paiResearchProgress[iI] = 0;
			m_paiTechCount[iI] = 0;
		}

		FAssertMsg(m_paiTerrainTradeCount==NULL, "about to leak memory, CvTeam::m_paiTerrainTradeCount");
		m_paiTerrainTradeCount = new int[GC.getNumTerrainInfos()];
		for (iI = 0; iI < GC.getNumTerrainInfos(); iI++)
		{
			m_paiTerrainTradeCount[iI] = 0;
		}

		FAssertMsg(m_aiVictoryCountdown==NULL, "about to leak memory, CvTeam::m_aiVictoryCountdown");
		m_aiVictoryCountdown = new int[GC.getNumVictoryInfos()];
		for (iI = 0; iI < GC.getNumVictoryInfos(); iI++)
		{
			m_aiVictoryCountdown[iI] = -1;
		}

		FAssertMsg(m_pabHasTech==NULL, "about to leak memory, CvTeam::m_pabHasTech");
		m_pabHasTech = new bool[GC.getNumTechInfos()];
		FAssertMsg(m_pabNoTradeTech==NULL, "about to leak memory, CvTeam::m_pabNoTradeTech");
		m_pabNoTradeTech = new bool[GC.getNumTechInfos()];
		for (iI = 0; iI < GC.getNumTechInfos(); iI++)
		{
			m_pabHasTech[iI] = false;
			m_pabNoTradeTech[iI] = false;
		}

		FAssertMsg(m_ppaaiImprovementYieldChange==NULL, "about to leak memory, CvTeam::m_ppaaiImprovementYieldChange");
		m_ppaaiImprovementYieldChange = new int*[GC.getNumImprovementInfos()];
		for (iI = 0; iI < GC.getNumImprovementInfos(); iI++)
		{
			m_ppaaiImprovementYieldChange[iI] = new int[NUM_YIELD_TYPES];
			for (iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
			{
				m_ppaaiImprovementYieldChange[iI][iJ] = 0;
			}
		}

		FAssertMsg(m_aiForceTeamVoteEligibilityCount==NULL, "about to leak memory, CvTeam::m_aiForceTeamVoteEligibilityCount");
		m_aiForceTeamVoteEligibilityCount = new int[GC.getNumVoteSourceInfos()];
		for (iI = 0; iI < GC.getNumVoteSourceInfos(); iI++)
		{
			m_aiForceTeamVoteEligibilityCount[iI] = 0;
		}

		m_aeRevealedBonuses.clear();

		AI_reset(false);
	}
}


/*  BETTER_BTS_AI_MOD, 12/30/08, jdog5000: START
	for clearing data stored in plots and cities for this team */
void CvTeam::resetPlotAndCityData()
{
	for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(iI);
		pLoopPlot->setRevealedOwner(getID(), NO_PLAYER);
		pLoopPlot->setRevealedImprovementType(getID(), NO_IMPROVEMENT);
		pLoopPlot->setRevealedRouteType(getID(), NO_ROUTE);
		pLoopPlot->setRevealed(getID(), false, false, getID(), true);

		CvCity* pLoopCity = pLoopPlot->getPlotCity();
		if (pLoopCity != NULL)
		{
			pLoopCity->setRevealed(getID(), false);
			pLoopCity->setEspionageVisibility(getID(), false, true);
		}
	}
} // BETTER_BTS_AI_MOD: END


void CvTeam::addTeam(TeamTypes eTeam)
{
	int iI, iJ;
	int iOriginalTeamSize = getNumMembers();// K-Mod

	FAssert(eTeam != NO_TEAM);
	FAssert(eTeam != getID());

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer const& kObs = GET_PLAYER((PlayerTypes)iI);
		if (!kObs.isAlive())
			continue; // advc
		if (kObs.getTeam() != getID() && kObs.getTeam() != eTeam)
		{
			if ((isHasMet(kObs.getTeam()) && GET_TEAM(eTeam).isHasMet(kObs.getTeam()))
					|| kObs.isSpectator()) // advc.127
			{
				CvWString szBuffer(gDLL->getText("TXT_KEY_MISC_PLAYER_PERMANENT_ALLIANCE",
						getName().GetCString(), GET_TEAM(eTeam).getName().GetCString()));
				gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(), false,
						GC.getEVENT_MESSAGE_TIME(), szBuffer, "AS2D_THEIRALLIANCE",
						MESSAGE_TYPE_MAJOR_EVENT, // advc.106b
						NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
						// <advc.127b>
						getCapitalX(kObs.getTeam(), true),
						getCapitalY(kObs.getTeam(), true)); // </advc.127b>
			}
		}
	}

	CvWString szBuffer(gDLL->getText("TXT_KEY_MISC_PLAYER_PERMANENT_ALLIANCE",
			getReplayName().GetCString(), GET_TEAM(eTeam).getReplayName().GetCString()));
	CvGame& g = GC.getGame();
	g.addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(), szBuffer, -1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));

	// K-Mod note: the cancel deals code use to be here. I've moved it lower down.

	shareItems(eTeam);
	// dlph.26: "This team is not going to be used anymore and doing this might break some things."
	//GET_TEAM(eTeam).shareItems(getID());

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (iI != getID() && iI != eTeam)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (GET_TEAM(eTeam).isHasMet((TeamTypes)iI))
				{
					meet((TeamTypes)iI, false);
				}
				else if (isHasMet((TeamTypes)iI))
				{
					GET_TEAM(eTeam).meet((TeamTypes)iI, false);
				}
			}
		}
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (iI != getID() && iI != eTeam)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (GET_TEAM(eTeam).isAtWar((TeamTypes)iI))
				{
					//declareWar((TeamTypes)iI, false, GET_TEAM(eTeam).AI_getWarPlan((TeamTypes)iI));
					// <dlph.26>
					queueWar(getID(), (TeamTypes)iI, false,
							GET_TEAM(eTeam).AI_getWarPlan((TeamTypes)iI)); // </dlph.26>
				}
				else if (isAtWar((TeamTypes)iI))
				{
					//GET_TEAM(eTeam).declareWar(((TeamTypes)iI), false, AI_getWarPlan((TeamTypes)iI));
					// <dlph.26>
					queueWar(eTeam, (TeamTypes)iI, false,
							AI_getWarPlan((TeamTypes)iI)); // </dlph.26>
				}
			}
		}
	}
	// <dlph.26>
	triggerWars();
	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (iI != getID() && iI != eTeam)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (GET_TEAM(eTeam).isAtWar((TeamTypes)iI))
				{
					queueWar((TeamTypes)iI, getID(), false, WARPLAN_DOGPILE, false);
				}
				else if (isAtWar((TeamTypes)iI))
				{
					GET_TEAM(eTeam).declareWar((TeamTypes)iI, false, AI_getWarPlan((TeamTypes)iI));
					queueWar((TeamTypes)iI, eTeam, false, WARPLAN_DOGPILE, false);
				}
			}
		}
	}
	triggerWars();
	// </dlph.26>
	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (iI != getID() && iI != eTeam)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (GET_TEAM(eTeam).isPermanentWarPeace((TeamTypes)iI))
				{
					setPermanentWarPeace(((TeamTypes)iI), true);
				}
				else if (isPermanentWarPeace((TeamTypes)iI))
				{
					GET_TEAM(eTeam).setPermanentWarPeace(((TeamTypes)iI), true);
				}
			}
		}
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if(iI == getID() || iI == eTeam)
			continue;
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
		if(!kLoopTeam.isAlive())
			continue; // advc
		if (GET_TEAM(eTeam).isOpenBorders(kLoopTeam.getID()))
		{
			setOpenBorders(kLoopTeam.getID(), true);
			kLoopTeam.setOpenBorders(getID(), true);
		}
		else if (isOpenBorders(kLoopTeam.getID()))
		{
			GET_TEAM(eTeam).setOpenBorders(kLoopTeam.getID(), true);
			kLoopTeam.setOpenBorders(eTeam, true);
		}
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (iI != getID() && iI != eTeam)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (GET_TEAM(eTeam).isDefensivePact((TeamTypes)iI))
				{
					setDefensivePact(((TeamTypes)iI), true);
					GET_TEAM((TeamTypes)iI).setDefensivePact(getID(), true);
				}
				else if (isDefensivePact((TeamTypes)iI))
				{
					GET_TEAM(eTeam).setDefensivePact(((TeamTypes)iI), true);
					GET_TEAM((TeamTypes)iI).setDefensivePact(eTeam, true);
				}
			}
		}
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (iI != getID() && iI != eTeam)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (GET_TEAM(eTeam).isForcePeace((TeamTypes)iI))
				{
					setForcePeace(((TeamTypes)iI), true);
					GET_TEAM((TeamTypes)iI).setForcePeace(getID(), true);
				}
				else if (isForcePeace((TeamTypes)iI))
				{
					GET_TEAM(eTeam).setForcePeace(((TeamTypes)iI), true);
					GET_TEAM((TeamTypes)iI).setForcePeace(eTeam, true);
				}
			}
		}
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (iI != getID() && iI != eTeam)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (GET_TEAM(eTeam).isVassal((TeamTypes)iI))
				{
					//setVassal(((TeamTypes)iI), true, isCapitulated());
					setVassal(((TeamTypes)iI), true, GET_TEAM(eTeam).isCapitulated()); // K-Mod
				}
				else if (isVassal((TeamTypes)iI))
				{
					GET_TEAM(eTeam).setVassal(((TeamTypes)iI), true, isCapitulated());
				}
			}
		}
	}

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (iI != getID() && iI != eTeam)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (GET_TEAM((TeamTypes)iI).isVassal(eTeam))
				{
					GET_TEAM((TeamTypes)iI).setVassal(getID(), true, GET_TEAM((TeamTypes)iI).isCapitulated());
				}
				else if (GET_TEAM((TeamTypes)iI).isVassal(getID()))
				{
					GET_TEAM((TeamTypes)iI).setVassal(eTeam, true, GET_TEAM((TeamTypes)iI).isCapitulated());
				}
			}
		}
	}

	shareCounters(eTeam);
	//GET_TEAM(eTeam).shareCounters(getID());
	/*  K-Mod note: eTeam is not going to be used after we've finished this merge,
		so the sharing does not need to be two-way. */

	/*  <dlph.26> "Fix for permanent alliance bug that caused permanent espionage visibility
		for cities that only the players with the higher team number sees at the time of the merge. */
	for(iI = 0; iI < GC.getMap().numPlots(); iI++) {
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(iI);
		if (pLoopPlot->isCity())
			pLoopPlot->getPlotCity()->setEspionageVisibility(eTeam, false, true);
	} // </dlph.26>

	/*  advc.104t: Leader id needed later for merging data; unavailable after the
		loop below. */
	PlayerTypes eTeamLeader = GET_TEAM(eTeam).getLeaderID();
	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).getTeam() == eTeam)
		{
			GET_PLAYER((PlayerTypes)iI).setTeam(getID());
		}
	}
	updateLeaderID(); // advc.opt
	// <dlph.13>
	// "AP resident and UN secretary general teams need to be updated if that team will not be used anymore."
	for (iI = 0; iI < GC.getNumVoteSourceInfos(); iI++)
	{
		if (g.canHaveSecretaryGeneral((VoteSourceTypes)iI) && g.getSecretaryGeneral((VoteSourceTypes)iI) == eTeam)
		{
			for (iJ = 0; iJ < GC.getNumVoteInfos(); iJ++)
			{
				if (GC.getVoteInfo((VoteTypes)iJ).isVoteSourceType((VoteSourceTypes)iI) && GC.getVoteInfo((VoteTypes)iJ).isSecretaryGeneral())
				{
					VoteTriggeredData kData;
					kData.iId = FFreeList::INVALID_INDEX;
					kData.eVoteSource = (VoteSourceTypes)iI;
					kData.kVoteOption.eVote = (VoteTypes)iJ;
					kData.kVoteOption.iCityId = -1;
					kData.kVoteOption.szText.clear();
					kData.kVoteOption.ePlayer = NO_PLAYER;
					g.setVoteOutcome(kData, (PlayerVoteTypes)getID());
				}
			}
		}
	} // </dlph.13>
	// K-Mod. Adjust the progress of unfinished research so that it is proportionally the same as it was before the merge.
	{
		// cf. CvTeam::getResearchCost
		int iCostMultiplier = 100;
		iCostMultiplier *= 100 + GC.getDefineINT(CvGlobals::TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER) * (getNumMembers() - 1); // new
		iCostMultiplier /= 100 + GC.getDefineINT(CvGlobals::TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER) * (iOriginalTeamSize - 1); // old

		FAssert(iCostMultiplier >= 100);

		for (TechTypes i = (TechTypes)0; i < GC.getNumTechInfos(); i=(TechTypes)(i+1))
		{
			if (!isHasTech(i) && getResearchProgress(i) > 0)
				setResearchProgress(i, getResearchProgress(i)*iCostMultiplier/100, getLeaderID());
		}
	}
	// K-Mod end

	// K-Mod: The following cancel deals code has been moved from higher up.
	// I've done this so that when open-borders is canceled, it doesn't bump our new allies out of our borders.
	FOR_EACH_DEAL_VAR(pLoopDeal)
	{
		/* original bts code
		if (((GET_PLAYER(pLoopDeal->getFirstPlayer()).getTeam() == getID()) && (GET_PLAYER(pLoopDeal->getSecondPlayer()).getTeam() == eTeam)) ||
			  ((GET_PLAYER(pLoopDeal->getFirstPlayer()).getTeam() == eTeam) && (GET_PLAYER(pLoopDeal->getSecondPlayer()).getTeam() == getID()))) */
		// K-Mod: The player's teams have already been reassigned - so we don't check for eTeam anymore.
		if (!pLoopDeal->involves(getID())) // advc: Replacing the K-Mod replacement
			continue;

		for (CLLNode<TradeData>* pNode = pLoopDeal->headTradesNode(); pNode != NULL;
			pNode = pLoopDeal->nextTradesNode(pNode))
		{
			if ((pNode->m_data.m_eItemType == TRADE_OPEN_BORDERS) ||
				(pNode->m_data.m_eItemType == TRADE_DEFENSIVE_PACT) ||
				(pNode->m_data.m_eItemType == TRADE_PEACE_TREATY) ||
				(pNode->m_data.m_eItemType == TRADE_VASSAL) ||
				(pNode->m_data.m_eItemType == TRADE_SURRENDER)
				// advc.034: Simplest to just cancel it
				|| pNode->m_data.m_eItemType == TRADE_DISENGAGE)
			{
				pLoopDeal->kill();
				break;
			}
		}
	}
	// <dlph.1>
	for(iI = 0; iI < NUM_DOMAIN_TYPES; iI++) {
		DomainTypes eDomain = (DomainTypes)iI; // advc
		changeExtraMoves(eDomain, std::max(0, GET_TEAM(eTeam).getExtraMoves(eDomain) -
				getExtraMoves(eDomain)));
	} // </dlph.1>
	CvMap const& m = GC.getMap();
	for (iI = 0; iI < m.numPlots(); iI++)
	{
		CvPlot& p = *m.plotByIndex(iI); // advc

		/*  dlph.26: This part is moved above to the part where members of the
			other team still had their old team number. */
		// <dlph.2>
		/*if(p.isCity()) {
			CvCity& c = *p.getPlotCity(); // advc
			c.setEspionageVisibility(eTeam, false, false);
		}*/ // </dlph.2>

		p.changeVisibilityCount(getID(), p.getVisibilityCount(eTeam), NO_INVISIBLE, false);

		for (iJ = 0; iJ < GC.getNumInvisibleInfos(); iJ++)
		{
			InvisibleTypes eInv = (InvisibleTypes)iJ;
			p.changeInvisibleVisibilityCount(getID(), eInv, p.getInvisibleVisibilityCount(eTeam, eInv));
		}

		if (p.isRevealed(eTeam, false))
			p.setRevealed(getID(), true, false, eTeam, false);
	}

	g.updatePlotGroups();
	int iOtherTeamSize = getNumMembers() - iOriginalTeamSize; // dlph.26
	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (iI != getID() && iI != eTeam)
		{
			CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iI); // K-Mod
			/*kLoopTeam.setWarWeariness(getID(), ((kLoopTeam.getWarWeariness(getID()) + kLoopTeam.getWarWeariness(eTeam)) / 2));
			kLoopTeam.setStolenVisibilityTimer(getID(), ((kLoopTeam.getStolenVisibilityTimer(getID()) + kLoopTeam.getStolenVisibilityTimer(eTeam)) / 2));
			kLoopTeam.AI_setAtWarCounter(getID(), ((kLoopTeam.AI_getAtWarCounter(getID()) + kLoopTeam.AI_getAtWarCounter(eTeam)) / 2));
			kLoopTeam.AI_setAtPeaceCounter(getID(), ((kLoopTeam.AI_getAtPeaceCounter(getID()) + kLoopTeam.AI_getAtPeaceCounter(eTeam)) / 2));
			kLoopTeam.AI_setHasMetCounter(getID(), ((kLoopTeam.AI_getHasMetCounter(getID()) + kLoopTeam.AI_getHasMetCounter(eTeam)) / 2));
			kLoopTeam.AI_setDefensivePactCounter(getID(), ((kLoopTeam.AI_getDefensivePactCounter(getID()) + kLoopTeam.AI_getDefensivePactCounter(eTeam)) / 2));
			kLoopTeam.AI_setShareWarCounter(getID(), ((kLoopTeam.AI_getShareWarCounter(getID()) + kLoopTeam.AI_getShareWarCounter(eTeam)) / 2));
			kLoopTeam.AI_setWarSuccess(getID(), ((kLoopTeam.AI_getWarSuccess(getID()) + kLoopTeam.AI_getWarSuccess(eTeam)) / 2));
			kLoopTeam.AI_setEnemyPeacetimeTradeValue(getID(), ((kLoopTeam.AI_getEnemyPeacetimeTradeValue(getID()) + kLoopTeam.AI_getEnemyPeacetimeTradeValue(eTeam)) / 2));
			kLoopTeam.AI_setEnemyPeacetimeGrantValue(getID(), ((kLoopTeam.AI_getEnemyPeacetimeGrantValue(getID()) + kLoopTeam.AI_getEnemyPeacetimeGrantValue(eTeam)) / 2));
			kLoopTeam.setEspionagePointsAgainstTeam(getID(), std::max(kLoopTeam.getEspionagePointsAgainstTeam(getID()), kLoopTeam.getEspionagePointsAgainstTeam(eTeam))); // unofficial patch*/
			/*  <dlph.26> "These counters now scale properly with number of players in teams.
				Also, espionage is now sum instead of max. */
			kLoopTeam.setWarWeariness(getID(), (iOriginalTeamSize *
					kLoopTeam.getWarWeariness(getID()) + iOtherTeamSize *
					kLoopTeam.getWarWeariness(eTeam)) / getNumMembers());
			kLoopTeam.setStolenVisibilityTimer(getID(), (iOriginalTeamSize *
					kLoopTeam.getStolenVisibilityTimer(getID()) + iOtherTeamSize *
					kLoopTeam.getStolenVisibilityTimer(eTeam)) / getNumMembers());
			kLoopTeam.AI_setAtWarCounter(getID(), (iOriginalTeamSize *
					kLoopTeam.AI_getAtWarCounter(getID()) + iOtherTeamSize *
					kLoopTeam.AI_getAtWarCounter(eTeam)) / getNumMembers());
			kLoopTeam.AI_setAtPeaceCounter(getID(), (iOriginalTeamSize *
					kLoopTeam.AI_getAtPeaceCounter(getID()) + iOtherTeamSize *
					kLoopTeam.AI_getAtPeaceCounter(eTeam)) / getNumMembers());
			kLoopTeam.AI_setHasMetCounter(getID(), (iOriginalTeamSize *
					kLoopTeam.AI_getHasMetCounter(getID()) + iOtherTeamSize *
					kLoopTeam.AI_getHasMetCounter(eTeam)) / getNumMembers());
			kLoopTeam.AI_setDefensivePactCounter(getID(), (iOriginalTeamSize *
					kLoopTeam.AI_getDefensivePactCounter(getID()) + iOtherTeamSize *
					kLoopTeam.AI_getDefensivePactCounter(eTeam)) / getNumMembers());
			kLoopTeam.AI_setShareWarCounter(getID(), (iOriginalTeamSize *
					kLoopTeam.AI_getShareWarCounter(getID()) + iOtherTeamSize *
					kLoopTeam.AI_getShareWarCounter(eTeam)) / getNumMembers());
			kLoopTeam.AI_setWarSuccess(getID(), (iOriginalTeamSize *
					kLoopTeam.AI_getWarSuccess(getID()) + iOtherTeamSize *
					kLoopTeam.AI_getWarSuccess(eTeam)) / getNumMembers());
			// <advc.130m>
			kLoopTeam.AI_setSharedWarSuccess(getID(), (iOriginalTeamSize *
					kLoopTeam.AI_getSharedWarSuccess(getID()) + iOtherTeamSize *
					kLoopTeam.AI_getSharedWarSuccess(eTeam)) / getNumMembers());
			// </advc.130m>
			kLoopTeam.AI_setEnemyPeacetimeTradeValue(getID(), (iOriginalTeamSize *
					kLoopTeam.AI_getEnemyPeacetimeTradeValue(getID()) + iOtherTeamSize *
					kLoopTeam.AI_getEnemyPeacetimeTradeValue(eTeam)) / getNumMembers());
			kLoopTeam.AI_setEnemyPeacetimeGrantValue(getID(), (iOriginalTeamSize *
					kLoopTeam.AI_getEnemyPeacetimeGrantValue(getID()) + iOtherTeamSize *
					kLoopTeam.AI_getEnemyPeacetimeGrantValue(eTeam)) / getNumMembers());
			kLoopTeam.setEspionagePointsAgainstTeam(getID(),
					kLoopTeam.getEspionagePointsAgainstTeam(getID()) +
					kLoopTeam.getEspionagePointsAgainstTeam(eTeam));
			// </dlph.26>

			if (kLoopTeam.isAlive())
			{
				kLoopTeam.AI_setWarPlan(getID(), NO_WARPLAN, false);
				kLoopTeam.AI_setWarPlan(eTeam, NO_WARPLAN, false);
				// <advc.001> Cancel our war plans too
				if(isHuman() && !isAtWar(kLoopTeam.getID()))
					AI_setWarPlan(kLoopTeam.getID(), NO_WARPLAN); // </advc.001>
			}
		}
	}

	AI_updateWorstEnemy();
	// <advc.104t>
	if(getWPAI.isEnabled()) {
		AI().warAndPeaceAI().addTeam(eTeamLeader);
		getWPAI.update();
	} // </advc.104t>
	AI_updateAreaStrategies();

	g.updateScore(true);
}


void CvTeam::shareItems(TeamTypes eTeam)
{
	int iI, iJ, iK;

	FAssert(eTeam != NO_TEAM);
	FAssert(eTeam != getID());

	for (iI = 0; iI < GC.getNumTechInfos(); iI++)
	{
		TechTypes eTech = (TechTypes)iI;
		if (GET_TEAM(eTeam).isHasTech(eTech))
		{	// <dlph.26> "Preserve no tech brokering status."
			setNoTradeTech(eTech, (!isHasTech(eTech) || isNoTradeTech(eTech)) &&
					GET_TEAM(eTeam).isNoTradeTech(eTech)); // </dlph.26>
			setHasTech(eTech, true, NO_PLAYER, true, false);
		}
	}
	/*  <dlph.26> "Other direction also done here as other direction of shareItems
		is not used anymore." */
	for (iI = 0; iI < GC.getNumTechInfos(); iI++)
	{
		TechTypes eTech = (TechTypes)iI;
		if (isHasTech(eTech))
		{
			GET_TEAM(eTeam).setNoTradeTech(eTech,
					(!GET_TEAM(eTeam).isHasTech(eTech) ||
					GET_TEAM(eTeam).isNoTradeTech(eTech)) &&
					isNoTradeTech(eTech));
			GET_TEAM(eTeam).setHasTech(eTech, true, NO_PLAYER, true, false);
		}
	} // </dlph.26>

	for (iI = 0; iI < GC.getNumBonusInfos(); ++iI)
	{
		if (GET_TEAM(eTeam).isForceRevealedBonus((BonusTypes)iI))
		{
			setForceRevealedBonus((BonusTypes)iI, true);
		}
	}

	/*  <dlph.26> "Other direction also done here as other direction of shareItems
		is not used anymore." */
	for (iI = 0; iI < GC.getNumBonusInfos(); ++iI)
	{
		if (isForceRevealedBonus((BonusTypes)iI))
		{
			GET_TEAM(eTeam).setForceRevealedBonus((BonusTypes)iI, true);
		}
	} // </dlph.26>

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		TeamTypes eLoopTeam = (TeamTypes)iI;
		//setEspionagePointsAgainstTeam((TeamTypes)iTeam, std::max(GET_TEAM(eTeam).getEspionagePointsAgainstTeam((TeamTypes)iTeam), getEspionagePointsAgainstTeam((TeamTypes)iTeam)));
		// <dlph.26> "Espionage is now sum instead of max."
		setEspionagePointsAgainstTeam(eLoopTeam,
				GET_TEAM(eTeam).getEspionagePointsAgainstTeam(eLoopTeam) +
				getEspionagePointsAgainstTeam(eLoopTeam)); // </dlph.26>
	}
	//setEspionagePointsEver(std::max(GET_TEAM(eTeam).getEspionagePointsEver(), getEspionagePointsEver())); // K-Mod
	// dlph.26: Replacing the above
	setEspionagePointsEver(GET_TEAM(eTeam).getEspionagePointsEver() + getEspionagePointsEver());

	for(iI = 0; iI < MAX_PLAYERS; iI++) {  // advc: style changes
		CvPlayer const& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if(!kLoopPlayer.isAlive() || kLoopPlayer.getTeam() != eTeam)
			continue;

		FOR_EACH_CITY(pLoopCity, kLoopPlayer) {
			for(iJ = 0; iJ < GC.getNumBuildingInfos(); iJ++) {
				BuildingTypes eBuilding = (BuildingTypes)iJ;
				int iCityBuildings = pLoopCity->getNumBuilding(eBuilding);
				if(iCityBuildings <= 0 ||
						isObsoleteBuilding(eBuilding))
					continue;
				if(GC.getBuildingInfo(eBuilding).isTeamShare()) {
					for(iK = 0; iK < MAX_PLAYERS; iK++) {
						if(GET_PLAYER((PlayerTypes)iK).getTeam() == getID()) {
							GET_PLAYER((PlayerTypes)iK).processBuilding(eBuilding,
									iCityBuildings, pLoopCity->area());
						}
					}
				}
				processBuilding(eBuilding, iCityBuildings);
			}
		}
	}
	/*  <dlph.26> "Other direction also done here as other direction of shareItems
		is not used anymore." */
	for(iI = 0; iI < MAX_PLAYERS; iI++) {
		CvPlayer const& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if(!kLoopPlayer.isAlive() || kLoopPlayer.getTeam() != getID())
			continue;

		FOR_EACH_CITY(pLoopCity, kLoopPlayer) {
			for(iJ = 0; iJ < GC.getNumBuildingInfos(); iJ++) {
				BuildingTypes eBuilding = (BuildingTypes)iJ;
				int iCityBuildings = pLoopCity->getNumBuilding(eBuilding);
				if(iCityBuildings <= 0 ||
						isObsoleteBuilding(eBuilding))
					continue;
				if(GC.getBuildingInfo(eBuilding).isTeamShare()) {
					for(iK = 0; iK < MAX_PLAYERS; iK++) {
						if(GET_PLAYER((PlayerTypes)iK).getTeam() == eTeam) {
							GET_PLAYER((PlayerTypes)iK).processBuilding(eBuilding,
									iCityBuildings, pLoopCity->area());
						}
					}
				}
				GET_TEAM(eTeam).processBuilding(eBuilding, iCityBuildings);
			}
		}
	} // </dlph.26>

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == eTeam
				/*  dlph.26: "Other direction also done here as other direction
					of shareItems is not used anymore." */
				|| GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				GET_PLAYER((PlayerTypes)iI).AI_updateBonusValue();
			}
		}
	}
}

// K-Mod. I've edited this function quite a lot. (for reasons that have been lost in the sands of time)
void CvTeam::shareCounters(TeamTypes eTeam)
{
	CvTeamAI& kShareTeam = GET_TEAM(eTeam); // K-Mod
	for (int i = 0; i < MAX_TEAMS; i++)
	{
		TeamTypes eLoopTeam = (TeamTypes)i; // advc
		if (eLoopTeam != getID() && eLoopTeam != eTeam)
		{
			if (kShareTeam.getWarWeariness(eLoopTeam) > getWarWeariness(eLoopTeam))
				setWarWeariness(eLoopTeam, kShareTeam.getWarWeariness(eLoopTeam));
			//else kShareTeam.setWarWeariness(eLoopTeam, getWarWeariness(eLoopTeam));

			if (kShareTeam.getStolenVisibilityTimer(eLoopTeam) > getStolenVisibilityTimer(eLoopTeam))
				setStolenVisibilityTimer(eLoopTeam, kShareTeam.getStolenVisibilityTimer(eLoopTeam));
			//else kShareTeam.setStolenVisibilityTimer(eLoopTeam, getStolenVisibilityTimer(eLoopTeam));

			if (kShareTeam.AI_getAtWarCounter(eLoopTeam) > AI_getAtWarCounter(eLoopTeam))
				AI_setAtWarCounter(eLoopTeam, kShareTeam.AI_getAtWarCounter(eLoopTeam));
			//else kShareTeam.AI_setAtWarCounter(eLoopTeam, AI_getAtWarCounter(eLoopTeam));

			if (kShareTeam.AI_getAtPeaceCounter(eLoopTeam) > AI_getAtPeaceCounter(eLoopTeam))
				AI_setAtPeaceCounter(eLoopTeam, kShareTeam.AI_getAtPeaceCounter(eLoopTeam));
			//else kShareTeam.AI_setAtPeaceCounter(eLoopTeam, AI_getAtPeaceCounter(eLoopTeam));

			if (kShareTeam.AI_getHasMetCounter(eLoopTeam) > AI_getHasMetCounter(eLoopTeam))
				AI_setHasMetCounter(eLoopTeam, kShareTeam.AI_getHasMetCounter(eLoopTeam));
			//else kShareTeam.AI_setHasMetCounter(eLoopTeam, AI_getHasMetCounter(eLoopTeam));

			if (kShareTeam.AI_getOpenBordersCounter(eLoopTeam) > AI_getOpenBordersCounter(eLoopTeam))
				AI_setOpenBordersCounter(eLoopTeam, kShareTeam.AI_getOpenBordersCounter(eLoopTeam));
			//else kShareTeam.AI_setOpenBordersCounter(eLoopTeam, AI_getOpenBordersCounter(eLoopTeam));

			if (kShareTeam.AI_getDefensivePactCounter(eLoopTeam) > AI_getDefensivePactCounter(eLoopTeam))
				AI_setDefensivePactCounter(eLoopTeam, kShareTeam.AI_getDefensivePactCounter(eLoopTeam));
			//else kShareTeam.AI_setDefensivePactCounter(eLoopTeam, AI_getDefensivePactCounter(eLoopTeam));

			if (kShareTeam.AI_getShareWarCounter(eLoopTeam) > AI_getShareWarCounter(eLoopTeam))
				AI_setShareWarCounter(eLoopTeam, kShareTeam.AI_getShareWarCounter(eLoopTeam));
			//else kShareTeam.AI_setShareWarCounter(eLoopTeam, AI_getShareWarCounter(eLoopTeam));

			if (kShareTeam.AI_getWarSuccess(eLoopTeam) > AI_getWarSuccess(eLoopTeam))
				AI_setWarSuccess(eLoopTeam, kShareTeam.AI_getWarSuccess(eLoopTeam));
			//else kShareTeam.AI_setWarSuccess(eLoopTeam, AI_getWarSuccess(eLoopTeam));

			if (kShareTeam.AI_getEnemyPeacetimeTradeValue(eLoopTeam) > AI_getEnemyPeacetimeTradeValue(eLoopTeam))
				AI_setEnemyPeacetimeTradeValue(eLoopTeam, kShareTeam.AI_getEnemyPeacetimeTradeValue(eLoopTeam));
			//else kShareTeam.AI_setEnemyPeacetimeTradeValue(eLoopTeam, AI_getEnemyPeacetimeTradeValue(eLoopTeam));

			if (kShareTeam.AI_getEnemyPeacetimeGrantValue(eLoopTeam) > AI_getEnemyPeacetimeGrantValue(eLoopTeam))
				AI_setEnemyPeacetimeGrantValue(eLoopTeam, kShareTeam.AI_getEnemyPeacetimeGrantValue(eLoopTeam));
			//else kShareTeam.AI_setEnemyPeacetimeGrantValue(eLoopTeam, AI_getEnemyPeacetimeGrantValue(eLoopTeam));

			kShareTeam.AI_setWarPlan(eLoopTeam, NO_WARPLAN, false);
			// K-Mod note. presumably, the warplan is cleared under the assumption that kShareTeam is going to be removed.
		}
	}

	for (ProjectTypes eProject = (ProjectTypes)0; eProject < GC.getNumProjectInfos(); eProject=(ProjectTypes)(eProject+1))
	{
		/* int iExtraProjects = kShareTeam.getProjectCount(eProject) - getProjectCount(eProject);
		if (iExtraProjects > 0) {
			changeProjectCount(eProject, iExtraProjects);
			GC.getGame().incrementProjectCreatedCount(eProject, -iExtraProjects);
		}
		changeProjectMaking(eProject, kShareTeam.getProjectMaking(eProject));*/

		// set project counts to the max of the two teams.
		int iDelta = kShareTeam.getProjectCount(eProject) - getProjectCount(eProject);
		if (iDelta > 0)
		{
			changeProjectCount(eProject, iDelta);
			// don't count the additional projects that have been added in this way
			GC.getGame().incrementProjectCreatedCount(eProject, -iDelta);
		}
		/*else {
			kShareTeam.changeProjectCount(eProject, -iDelta);
			GC.getGame().incrementProjectCreatedCount(eProject, iDelta);
		}*/

		// projects still under construction should be counted for both teams
		changeProjectMaking(eProject, kShareTeam.getProjectMaking(eProject));
		//kShareTeam.changeProjectMaking(eProject, getProjectMaking(eProject) - kShareTeam.setProjectMaking(eProject));
	}

	for (UnitClassTypes eUnitClass = (UnitClassTypes)0; eUnitClass < GC.getNumUnitClassInfos(); eUnitClass=(UnitClassTypes)(eUnitClass+1))
	{
		changeUnitClassCount(eUnitClass, kShareTeam.getUnitClassCount(eUnitClass));
		//kShareTeam.changeUnitClassCount(eUnitClass, getUnitClassCount(eUnitClass) - kShareTeam.getUnitClassCount(eUnitClass));
	}

	for (BuildingClassTypes eBuildingClass = (BuildingClassTypes)0; eBuildingClass < GC.getNumBuildingClassInfos(); eBuildingClass=(BuildingClassTypes)(eBuildingClass+1))
	{
		changeBuildingClassCount(eBuildingClass, kShareTeam.getBuildingClassCount(eBuildingClass));
		//kShareTeam.changeBuildingClassCount(eBuildingClass, getBuildingClassCount(eBuildingClass) - kShareTeam.getBuildingClassCount(eBuildingClass));
	}

	for (TechTypes eTech = (TechTypes)0; eTech < GC.getNumTechInfos(); eTech=(TechTypes)(eTech+1))
	{
		//if (!isHasTech(eTech))
		if (!isHasTech(eTech) && !kShareTeam.isHasTech(eTech))
		{
			// K-Mod note: it's difficult to do any combined proportionality adjustments here, because if we set
			// the progress higher than the current cost then we'll get the tech right now before the cost is increased.
			// We can however adjust for uneven tech costs before the teams are merged.
			// (eg. suppose techs are more expensive for team 2; if team 2 almost has a tech - and if progress is
			//  transfered without adjustment, team 1 will immediately get the tech even though team 2 didn't finish it.)

			//if (kShareTeam.getResearchProgress(eTech) > getResearchProgress(eTech))
			if (kShareTeam.getResearchProgress(eTech) * getResearchCost(eTech) > getResearchProgress(eTech) * kShareTeam.getResearchCost(eTech))
			{
				setResearchProgress(eTech, kShareTeam.getResearchProgress(eTech) * getResearchCost(eTech) / std::max(1, kShareTeam.getResearchCost(eTech)), getLeaderID());
			}
			//else kShareTeam.setResearchProgress(eTech, getResearchProgress(eTech) * kShareTeam.getResearchCost(eTech) / std::max(1, getResearchCost(eTech)), kShareTeam.getLeaderID());
		}

		/*if (isHasTech(eTech) && !isNoTradeTech(eTech)))
			kShareTeam.setNoTradeTech((eTech), false);*/
		// unofficial patch
		/*if (kShareTeam.isHasTech(eTech) && !(kShareTeam.isNoTradeTech(eTech)))
			setNoTradeTech((eTech), false);*/
		/*  dlph.26 (commented out): "What was this even supposed to be doing?
			No tech brokering is now applied if necessary when tech is shared
			in CvTeam::shareItems. */
	}

	// K-Mod. Share extra moves.
	// Note: there is no reliable way to do this. We can't tell if the bonus is from something unique- such as circumnavigation,
	// or from something that is already taken into account - such as refrigeration.
	for (DomainTypes t = (DomainTypes)0; t < NUM_DOMAIN_TYPES; t=(DomainTypes)(t+1))
	{
		if (kShareTeam.getExtraMoves(t) > getExtraMoves(t))
			changeExtraMoves(t, kShareTeam.getExtraMoves(t)-getExtraMoves(t));
	}
	// K-Mod end
}


void CvTeam::processBuilding(BuildingTypes eBuilding, int iChange)
{
	for (int i = 0; i < GC.getNumVoteSourceInfos(); ++i)
	{
		if (GC.getBuildingInfo(eBuilding).getVoteSourceType() == i)
		{
			changeForceTeamVoteEligibilityCount((VoteSourceTypes)i, (GC.getBuildingInfo(eBuilding).isForceTeamVoteEligible()) ? iChange : 0);
		}
	}

	if (GC.getBuildingInfo(eBuilding).isMapCentering())
	{
		if (iChange > 0)
		{
			setMapCentering(true);
		}
	}

	changeEnemyWarWearinessModifier(GC.getBuildingInfo(eBuilding).getEnemyWarWearinessModifier() * iChange);
}


void CvTeam::doTurn()
{
	PROFILE("CvTeam::doTurn()");

	FAssert(isAlive());
	FAssert(countWarEnemies() == m_iMajorWarEnemies); // advc.003m
	// <advc.134a>
	FAssert(m_iPeaceOfferStage == 0 && m_eOfferingPeace == NO_TEAM);
	// Known issue in networked multiplayer
	m_iPeaceOfferStage = 0; m_eOfferingPeace = NO_TEAM;
	// </advc.134a>
	AI_doTurnPre();
	// <advc.162>
	for(int i = 0; i < MAX_TEAMS; i++)
		m_abJustDeclaredWar[i] = false;
	// </advc.162>
	if (isBarbarian())
		doBarbarianResearch(); // advc: Moved into subroutine

	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (getStolenVisibilityTimer((TeamTypes)iI) > 0)
			{
				changeStolenVisibilityTimer(((TeamTypes)iI), -1);
			}

			if (getCounterespionageTurnsLeftAgainstTeam((TeamTypes) iI) > 0)
			{
				changeCounterespionageTurnsLeftAgainstTeam((TeamTypes) iI, -1);
			}

			if (getCounterespionageTurnsLeftAgainstTeam((TeamTypes) iI) == 0)
			{
				setCounterespionageModAgainstTeam((TeamTypes) iI, 0);
			}
		}
	}

	if (!GC.getGame().isOption(GAMEOPTION_NO_TECH_BROKERING))
	{
		for (int iI = 0; iI < GC.getNumTechInfos(); iI++)
		{
			setNoTradeTech(((TechTypes)iI), false);
		}

	}

	doWarWeariness();

	// advc.136a: Moved to CvPlayer::doTurn
	//testCircumnavigated(); // K-Mod note: is it a bit unfair to test circumnavigation in this function?

	AI_doTurnPost();
}


void CvTeam::updateYield()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				GET_PLAYER((PlayerTypes)iI).updateYield();
			}
		}
	}
}


void CvTeam::updatePowerHealth()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				GET_PLAYER((PlayerTypes)iI).updatePowerHealth();
			}
		}
	}
}


void CvTeam::updateCommerce()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				GET_PLAYER((PlayerTypes)iI).updateCommerce();
			}
		}
	}
}


bool CvTeam::canChangeWarPeace(TeamTypes eTeam, bool bAllowVassal) const
{
	if (GC.getGame().isOption(GAMEOPTION_NO_CHANGING_WAR_PEACE))
	{
		return false;
	}

	if (eTeam == getID())
	{
		return false;
	}

	if (isPermanentWarPeace(eTeam) || GET_TEAM(eTeam).isPermanentWarPeace(getID()))
	{
		return false;
	}

	for (int iLoopTeam = 0; iLoopTeam < MAX_CIV_TEAMS; ++iLoopTeam)
	{
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iLoopTeam);
		if (kLoopTeam.isAlive())
		{
			if (kLoopTeam.isVassal(getID()) && kLoopTeam.isPermanentWarPeace(eTeam))
			{
				return false;
			}

			if (kLoopTeam.isVassal(eTeam) && kLoopTeam.isPermanentWarPeace(getID()))
			{
				return false;
			}
		}
	}

	if (isAVassal())
	{
		return false;
	}
	/*  <advc.001> Had a civ make peace with a minor civ in one game. Not sure how
		that happened; probably through a random event. */
	if(isMinorCiv() || GET_TEAM(eTeam).isMinorCiv())
		return false; // </advc.001>
	if (bAllowVassal)
	{
		if (GET_TEAM(eTeam).isVassal(getID()))
		{
			return false;
		}
	}
	else
	{
		if (GET_TEAM(eTeam).isAVassal())
		{
			return false;
		}
	}
	// <advc.104> Don't want to have to check this separately in the UWAI code
	if(GC.getGame().isOption(GAMEOPTION_ALWAYS_WAR) && isAtWar(eTeam) &&
			(isHuman() || GET_TEAM(eTeam).isHuman()))
		return false; // </advc.104>
	return true;
}

// K-Mod, I've removed the bulk of this function and replaced it with just a call to "canEventuallyDeclareWar",
// which contains all of the original checks. I've done this to reduce code dupliation.
bool CvTeam::canDeclareWar(TeamTypes eTeam) const
{
	if (!canEventuallyDeclareWar(eTeam))
		return false;

	if (isForcePeace(eTeam))
		return false;

	for (int i = 0; i < MAX_CIV_TEAMS; i++) // advc.003n: was MAX_TEAM
	{
		if (i != eTeam && i != getID() && GET_TEAM(eTeam).isVassal((TeamTypes)i))
		{
			if (isForcePeace((TeamTypes)i))
			{
				return false;
			}
		}
	}

	return true;
}

// bbai (advc: refactored)
bool CvTeam::canEventuallyDeclareWar(TeamTypes eTeam) const
{
	return (eTeam != getID() && isAlive() && GET_TEAM(eTeam).isAlive() &&
			!isAtWar(eTeam) && isHasMet(eTeam) && canChangeWarPeace(eTeam, true) &&
			!GC.getGame().isOption(GAMEOPTION_ALWAYS_PEACE) &&
			GC.getPythonCaller()->canDeclareWar(getID(), eTeam));
} // bbai end

// K-Mod note: I've shuffled things around a bit in this function.
void CvTeam::declareWar(TeamTypes eTeam, bool bNewDiplo, WarPlanTypes eWarPlan, bool bPrimaryDoW,
		PlayerTypes eSponsor, // advc.100
		bool bRandomEvent) // advc.106g
{
	PROFILE_FUNC();

	int iI;

	FAssertMsg(eTeam != NO_TEAM, "eTeam is not assigned a valid value");
	FAssertMsg(eTeam != getID(), "eTeam is not expected to be equal with getID()");
	// <advc.100>
	FAssert(eSponsor == NO_PLAYER || (TEAMID(eSponsor) != getID() &&
			TEAMID(eSponsor) != eTeam)); // </advc.100>
	if (isAtWar(eTeam))
		return;

	if (gTeamLogLevel >= 1) logBBAI("  Team %d (%S) declares war on team %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eTeam);

	FOR_EACH_DEAL_VAR(pLoopDeal)
	{
		if (pLoopDeal->isBetween(getID(), eTeam))
			pLoopDeal->kill();
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kEitherMember = GET_PLAYER((PlayerTypes)iI);
		if (kEitherMember.getTeam() == getID() || kEitherMember.getTeam() == eTeam)
			kEitherMember.updatePlunder(-1, false);
	}

	FAssertMsg(eTeam != getID(), "eTeam is not expected to be equal with getID()");
	/*  <advc.104q> War plan becomes Limited or Total already when war is imminent.
		In that case, this function doesn't reset the StateCounter. Can still
		(and should) use AI_getAtWarCounter to determine war duration, but I don't
		think that the war duration plus the time war was imminent is worth tracking. */
	if(!isAtWar(eTeam)) {
		AI().AI_setWarPlanStateCounter(eTeam, 0);
		GET_TEAM(eTeam).AI_setWarPlanStateCounter(getID(), 0);
	} // </advc.104q>

	/*  advc.130h: Moved this loop up b/c I'm adding code that depends on the
		atWarWithPartner status before the DoW */
	// advc: Deleted the BtS code that karadoc refers to
	// K-Mod. Same functionality, but a bit cleaner and a bit faster.
	for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i = (PlayerTypes)(i+1))  // advc.003n: was MAX_PLAYERS
	{
		CvPlayerAI& kPlayer_i = GET_PLAYER(i); // advc.130o: was const

		if (!kPlayer_i.isAlive() || kPlayer_i.getTeam() != getID())
			continue;
		// player i is a member of this team.

		for (PlayerTypes j = (PlayerTypes)0; j < MAX_CIV_PLAYERS; j = (PlayerTypes)(j+1)) // advc.003n: was MAX_PLAYERS
		{
			CvPlayerAI& kPlayer_j = GET_PLAYER(j);

			if (!kPlayer_j.isAlive() /* advc.opt: */ || kPlayer_j.isMinorCiv())
				continue;

			// <advc.130o>
			if(bPrimaryDoW && kPlayer_i.isHuman() && !kPlayer_j.isHuman() &&
					GET_TEAM(eTeam).AI_getMemoryCount(getID(), MEMORY_MADE_DEMAND) > 0 &&
					TEAMREF(j).getMasterTeam() != getMasterTeam() &&
					GET_TEAM(eTeam).isHasMet(kPlayer_j.getTeam())) {
				// Raise it to 8 (or what XML says)
				int iMemory = kPlayer_j.AI_getMemoryCount(i, MEMORY_MADE_DEMAND_RECENT);
				int iDelta = GC.getDefineINT("WAR_DESPITE_TRIBUTE_MEMORY");
				iDelta = std::max(iMemory, iDelta) - iMemory;
				kPlayer_j.AI_changeMemoryCount(i, MEMORY_MADE_DEMAND_RECENT, iDelta);
			}
			if(bPrimaryDoW && i != j)
				kPlayer_i.AI_setMemoryCount(j, MEMORY_MADE_DEMAND, 0);
			// </advc.130o>
			if (kPlayer_j.getTeam() == eTeam)
			{
				if(bPrimaryDoW) // advc.130y
					kPlayer_j.AI_rememberEvent(i, MEMORY_DECLARED_WAR); // advc.130j
				// advc.130y:
				else kPlayer_j.AI_changeMemoryCount(i, MEMORY_DECLARED_WAR, 2);
			} // advc.130h:
			if(kPlayer_j.AI_disapprovesOfDoW(getID(), eTeam)) // advc.130j:
				kPlayer_j.AI_rememberEvent(i, MEMORY_DECLARED_WAR_ON_FRIEND);
		}
	}
	// K-Mod end.
	// <advc.104i>
	if(eSponsor != NO_PLAYER) {
		makeUnwillingToTalk(eTeam);
		if(TEAMREF(eSponsor).isAtWar(eTeam))
			TEAMREF(eSponsor).makeUnwillingToTalk(eTeam);
	} // </advc.104i>

	setAtWar(eTeam, true);
	GET_TEAM(eTeam).setAtWar(getID(), true);
	// <advc.162>
	if(GC.getDefineINT("ENABLE_162") > 0)
		m_abJustDeclaredWar[eTeam] = true; // </advc.162>

	// Plot danger cache (bbai)
	GC.getMap().invalidateBorderDangerCache(eTeam);
	GC.getMap().invalidateBorderDangerCache(getID());
	//

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if ((GET_PLAYER((PlayerTypes)iI).getTeam() == getID()) || (GET_PLAYER((PlayerTypes)iI).getTeam() == eTeam))
		{
			GET_PLAYER((PlayerTypes)iI).updatePlunder(1, false);
		}
	}

	meet(eTeam, false);

	AI_setAtPeaceCounter(eTeam, 0);
	GET_TEAM(eTeam).AI_setAtPeaceCounter(getID(), 0);

	AI_setShareWarCounter(eTeam, 0);
	GET_TEAM(eTeam).AI_setShareWarCounter(getID(), 0);

	GET_TEAM(eTeam).AI_setWarPlan(getID(), ((isBarbarian() || isMinorCiv()) ? WARPLAN_ATTACKED : WARPLAN_ATTACKED_RECENT));

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (!GET_TEAM(eTeam).isAtWar((TeamTypes)iI) && GET_TEAM(eTeam).AI_isChosenWar((TeamTypes)iI))
			{
				GET_TEAM(eTeam).AI_setWarPlan(((TeamTypes)iI), NO_WARPLAN);
			}
		}
	}

	if (NO_WARPLAN != eWarPlan)
	{
		AI_setWarPlan(eTeam, eWarPlan);
	}

	FAssert(!AI_isSneakAttackPreparing(eTeam)
		/*  advc.104o: Can happen when hired to declare war while preparing.
			BtS/K-Mod doesn't allow hired war while preparing, but UWAI does.
			The K-Mod code below already handles WARPLAN_PREPARING_..., so, no problem. */
			|| getWPAI.isEnabled());
	if ((AI_getWarPlan(eTeam) == NO_WARPLAN) || AI_isSneakAttackPreparing(eTeam))
	{
		if (isHuman()
				// K-Mod. (for vassals that have been told to prepare for war)
				|| AI_getWarPlan(eTeam) == WARPLAN_PREPARING_TOTAL)
			AI_setWarPlan(eTeam, WARPLAN_TOTAL);
		else if (isMinorCiv() || isBarbarian() || GET_TEAM(eTeam).getAtWarCount(true) == 1)
			AI_setWarPlan(eTeam, WARPLAN_LIMITED);
		else AI_setWarPlan(eTeam, WARPLAN_DOGPILE);
	}

	GC.getMap().verifyUnitValidPlot();

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
		{
			GET_PLAYER((PlayerTypes)iI).verifyUnitStacksValid();
		}
	}

	GC.getGame().AI_makeAssignWorkDirty();

	if (getID() == GC.getGame().getActiveTeam() || eTeam == GC.getGame().getActiveTeam())
	{
		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
		// advc.001w: Can now enter each other's territory
		gDLL->getInterfaceIFace()->setDirty(Waypoints_DIRTY_BIT, true);
		// advc.162: DoW increases certain path costs
		CvSelectionGroup::path_finder.Reset();
	}
	// advc.003j: Obsolete
	/*for (iI = 0; iI < MAX_PLAYERS; iI++) {
		if (GET_PLAYER((PlayerTypes)iI).isAlive()) {
			for (int iJ = 0; iJ < MAX_PLAYERS; iJ++) {
				if (GET_PLAYER((PlayerTypes)iJ).isAlive()) {
					if ((GET_PLAYER((PlayerTypes)iI).getTeam() == getID()) && (GET_PLAYER((PlayerTypes)iJ).getTeam() == eTeam)) {
						GET_PLAYER((PlayerTypes)iI).AI_setFirstContact(((PlayerTypes)iJ), true);
						GET_PLAYER((PlayerTypes)iJ).AI_setFirstContact(((PlayerTypes)iI), true);
	} } } } }*/

	// (advc.130h: Code block dealing with diplo repercussions moved up)

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if ((GET_PLAYER((PlayerTypes)iI).getTeam() == getID()) || (GET_PLAYER((PlayerTypes)iI).getTeam() == eTeam))
			{
				GET_PLAYER((PlayerTypes)iI).updateWarWearinessPercentAnger();
			}
		}
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if ((GET_PLAYER((PlayerTypes)iI).getTeam() == getID()) || (GET_PLAYER((PlayerTypes)iI).getTeam() == eTeam))
			{
				GET_PLAYER((PlayerTypes)iI).updatePlotGroups();
			}
		}
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if ((GET_PLAYER((PlayerTypes)iI).getTeam() == getID()) || (GET_PLAYER((PlayerTypes)iI).getTeam() == eTeam))
			{
				GET_PLAYER((PlayerTypes)iI).updateTradeRoutes();
			}
		}
	}

	if (GC.getGame().isFinalInitialized() && !(gDLL->GetWorldBuilderMode()))
	{
		if (bNewDiplo)
		{
			if (!isHuman())
			{
				for (iI = 0; iI < MAX_PLAYERS; iI++)
				{
					if (GET_PLAYER((PlayerTypes)iI).isAlive())
					{
						if (GET_PLAYER((PlayerTypes)iI).getTeam() == eTeam)
						{
							if (GET_PLAYER(getLeaderID()).canContact((PlayerTypes)iI))
							{
								if (GET_PLAYER((PlayerTypes)iI).isHuman())
								{
									CvDiploParameters* pDiplo = new CvDiploParameters(getLeaderID());
									FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
									pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_DECLARE_WAR"));
									pDiplo->setAIContact(true);
									gDLL->beginDiplomacy(pDiplo, ((PlayerTypes)iI));
								}
							}
						}
					}
				}
			}
		}

		if (!isBarbarian() && !(GET_TEAM(eTeam).isBarbarian()) &&
			  !isMinorCiv() && !(GET_TEAM(eTeam).isMinorCiv()))
		{
			CvWString szBuffer;
			// <advc.100>
			CvWString szSponsorName;
			wchar const* cpSponsorName = L"";
			if(eSponsor != NO_PLAYER) {
				/*  Need to make a local copy b/c the thing returned by getName
					gets somehow overwritten with an empty string before the
					message is sent. */
				szSponsorName = GET_PLAYER(eSponsor).getName();
				cpSponsorName = szSponsorName.GetCString();
			} // </advc.100>
			for (iI = 0; iI < MAX_PLAYERS; iI++)
			{
				CvPlayer const& kObs = GET_PLAYER((PlayerTypes)iI);
				if(!kObs.isAlive())
					continue; // advc
				// <advc.106b>
				LPCTSTR sndYou = "AS2D_DECLAREWAR";
				LPCTSTR sndThey = "AS2D_THEIRDECLAREWAR";
				if((isAVassal() && !isHuman()) || (GET_TEAM(eTeam).isAVassal() &&
						!GET_TEAM(eTeam).isHuman()))
					sndYou = sndThey = NULL; // </advc.106b>
				if (kObs.getTeam() == getID())
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_DECLARED_WAR_ON",
							GET_TEAM(eTeam).getName().GetCString());
					gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(), true,
							GC.getEVENT_MESSAGE_TIME(), szBuffer,
							sndYou, // advc.106b
							MESSAGE_TYPE_MAJOR_EVENT, NULL, (ColorTypes)
							GC.getInfoTypeForString("COLOR_WARNING_TEXT"),
							// <advc.127b>
							GET_TEAM(eTeam).getCapitalX(kObs.getTeam()),
							GET_TEAM(eTeam).getCapitalY(kObs.getTeam()));
							// </advc.127b>
				}
				else if(kObs.getTeam() == eTeam)
				{	// <advc.100> Inform the target of the DoW about the sponsor
					if(eSponsor != NO_PLAYER)
						szBuffer = gDLL->getText("TXT_KEY_MISC_HIRED_WAR_ON_YOU",
								getName().GetCString(), cpSponsorName);
					else // </advc.100>
						szBuffer = gDLL->getText("TXT_KEY_MISC_DECLARED_WAR_ON_YOU", getName().GetCString());
						gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(),
								true, GC.getEVENT_MESSAGE_TIME(), szBuffer,
								sndYou, // advc.106b
								MESSAGE_TYPE_MAJOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_WARNING_TEXT"),
								// <advc.127b>
								getCapitalX(kObs.getTeam()),
								getCapitalY(kObs.getTeam()));
								// </advc.127b>
				}
				else if((isHasMet(kObs.getTeam()) && GET_TEAM(eTeam).isHasMet(kObs.getTeam()))
						|| kObs.isSpectator()) // advc.127
				{	// <advc.100> Inform third parties about sponsor
					if(eSponsor != NO_PLAYER && eSponsor != kObs.getID() &&
							(TEAMREF(eSponsor).isHasMet(kObs.getTeam()) ||
							kObs.isSpectator())) // advc.127
						szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_HIRED_WAR",
								getName().GetCString(),
								GET_TEAM(eTeam).getName().GetCString(), cpSponsorName);
					else // </advc.100>
						szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_DECLARED_WAR", getName().GetCString(), GET_TEAM(eTeam).getName().GetCString());
					gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(),
							false, GC.getEVENT_MESSAGE_TIME(), szBuffer,
							// <advc.106b>
							sndThey, (isAVassal() || GET_TEAM(eTeam).isAVassal() ?
							MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY : // </advc.106b>
							MESSAGE_TYPE_MAJOR_EVENT), NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_WARNING_TEXT"),
							// <advc.127b>
							getCapitalX(kObs.getTeam(), true),
							getCapitalY(kObs.getTeam(), true)); // </advc.127b>
				}
			}

			// <advc.100> Put info about hired wars in the replay log
			if(eSponsor != NO_PLAYER) {
				szSponsorName = GET_PLAYER(eSponsor).getReplayName();
				cpSponsorName = szSponsorName.GetCString();
				szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_HIRED_WAR",
						getReplayName().GetCString(),
						GET_TEAM(eTeam).getReplayName().GetCString(), cpSponsorName);
			}
			else { // </advc.100>
				// <advc.106g>
				if(bRandomEvent) {
					szBuffer = gDLL->getText("TXT_KEY_MISC_WAR_VIA_EVENT",
							getReplayName().GetCString(), GET_TEAM(eTeam).
							getReplayName().GetCString());
				}
				else { // </advc.106g>
					szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_DECLARES_WAR",
							getReplayName().GetCString(), GET_TEAM(eTeam).
							getReplayName().GetCString());
				}
			}
			GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(), szBuffer, -1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_WARNING_TEXT"));
		}
	}
	// dlph.26 (advc): EventReporter call moved down

	// K-Mod / BBAI.
	// This section includes some customization options from BBAI.
	// The code has been modified for K-Mod, so that it uses "bPrimaryDoW" rather than the BBAI parameter.
	// The original BtS code has been deleted.
	/* dlph.3: 'BBAI option 1 didn't work because if clauses for canceling pacts
	   were wrong. BBAI otpion 2 needs further fixing. When all players have
	   defensive pacts with all other players and someone declares war the
	   correct behaviour would be to have all attack the inital attacker,
	   but additional wars are declared due to recursive calls of declareWar
	   in the loop below.' */
	int const iDPBehavior = GC.getDefineINT(CvGlobals::BBAI_DEFENSIVE_PACT_BEHAVIOR);
	if(iDPBehavior == 0 || (iDPBehavior == 1 && bPrimaryDoW))
		cancelDefensivePacts();
	bool bDefPactTriggered = false; // advc.104i
	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (iI != getID() && iI != eTeam && GET_TEAM((TeamTypes)iI).isAlive())
		{	// <dlph.3> Ally can already be at war with the aggressor
			if(GET_TEAM((TeamTypes)iI).isAtWar(eTeam))
				continue; // </dlph.3>
			if (GET_TEAM((TeamTypes)iI).isDefensivePact(eTeam)) {
				FAssert(!GET_TEAM(eTeam).isAVassal() &&
						!GET_TEAM((TeamTypes)iI).isAVassal());
				//GET_TEAM((TeamTypes)iI).declareWar(getID(), bNewDiplo, WARPLAN_DOGPILE, false);
				// dlph.26:
				queueWar((TeamTypes)iI, getID(), bNewDiplo, WARPLAN_DOGPILE, false);
				// <advc.104i>
				bDefPactTriggered = true;
				if(!isAVassal()) {
				/*  Team iI declares war on us, and this makes our team
					unwilling to talk to both iI and its ally eTeam. */
					makeUnwillingToTalk(eTeam);
					makeUnwillingToTalk((TeamTypes)iI);
				} // </advc.104i>
			}
			else if (iDPBehavior > 1 && GET_TEAM((TeamTypes)iI).isDefensivePact(getID()))
			{
				// For alliance option.  This teams pacts are canceled above if not using alliance option.
				//GET_TEAM((TeamTypes)iI).declareWar(eTeam, bNewDiplo, WARPLAN_DOGPILE, false);
				// dlph.26:
				queueWar((TeamTypes)iI, eTeam, bNewDiplo, WARPLAN_DOGPILE, false);
			}
		}
	}

	if (iDPBehavior == 0)// dlph.3: || (iDPBehavior == 1 && bPrimaryDoW))
	{
		GET_TEAM(eTeam).cancelDefensivePacts();
	}
	// K-Mod / BBAI end.
	GET_TEAM(eTeam).allowDefensivePactsToBeCanceled(); // dlph.3
	/*  <advc.104i> When other teams come to the help of eTeam through a
		defensive pact, then eTeam becomes unwilling to talk with us. */
	if(bDefPactTriggered)
		GET_TEAM(eTeam).makeUnwillingToTalk(getID()); // </advc.104i>
	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (iI != getID() && iI != eTeam)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (GET_TEAM((TeamTypes)iI).isVassal(eTeam) || GET_TEAM(eTeam).isVassal((TeamTypes)iI))
				{
					//declareWar((TeamTypes)iI, bNewDiplo, AI_getWarPlan(eTeam), false);
					// dlph.26:
					queueWar(getID(), (TeamTypes)iI, bNewDiplo, AI_getWarPlan(eTeam), false);
				}
				else if (GET_TEAM((TeamTypes)iI).isVassal(getID()) || isVassal((TeamTypes)iI))
				{
					//GET_TEAM((TeamTypes)iI).declareWar(eTeam, bNewDiplo, WARPLAN_DOGPILE, false);
					// dlph.26:
					queueWar((TeamTypes)iI, eTeam, bNewDiplo, WARPLAN_DOGPILE, false);
				}
			}
		}
	}
	// K-Mod. update attitude
	/*if (bPrimaryDoW) {
		for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
			GET_PLAYER(i).AI_updateAttitudeCache();
	}*/ // K-Mod end
	// <dlph.26> The above is "updated when the war queue is emptied."
	/*  advc (bugfix): But not unless this function communicates to triggerWars that
		a (primary) DoW has already occurred. */
	triggerWars(true);
	// advc: Moved down so that war status has already changed when event reported
	CvEventReporter::getInstance().changeWar(true, getID(), eTeam); // </dlph.26>
}


void CvTeam::makePeace(TeamTypes eTeam, bool bBumpUnits,
		TeamTypes eBroker, // advc.100b
		bool bCapitulate, // advc.034
		CLinkList<TradeData>* pReparations, // advc.039
		bool bRandomEvent) // advc.106g
{
	CvWString szBuffer;
	int iI=-1;

	FAssertMsg(eTeam != NO_TEAM, "eTeam is not assigned a valid value");
	FAssertMsg(eTeam != getID(), "eTeam is not expected to be equal with getID()");

	if(!isAtWar(eTeam))
		return; // advc
	// <advc.104> To record who won the war, before war success is reset.
	AI().warAndPeaceAI().reportWarEnding(eTeam, pReparations, NULL);
	GET_TEAM(eTeam).warAndPeaceAI().reportWarEnding(getID(), NULL, pReparations);
	// </advc.104>
	/*  <advc.130y> Don't know if they started the war, but, if we did and they had
		started a war against us some time earlier, we may as well forgive them for
		that. (If there's no declared-war-on-us memory, then this call has no effect.)
		advc.104i also does sth. in forgiveEnemy. */
	AI().AI_forgiveEnemy(eTeam, isCapitulated(), false);
	GET_TEAM(eTeam).AI_forgiveEnemy(getID(), GET_TEAM(eTeam).isCapitulated(), false);
	// </advc.130y>
	// <advc.130i>
	if(getWPAI.isEnabled() && !isAVassal() && !GET_TEAM(eTeam).isAVassal()) {
		for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
			CvPlayerAI& kDPPlayer = GET_PLAYER((PlayerTypes)i);
			if(!kDPPlayer.isAlive() || kDPPlayer.getTeam() == getID() ||
					kDPPlayer.getTeam() == eTeam)
				continue;
			CvTeam const& kDPTeam = GET_TEAM(kDPPlayer.getTeam());
			if(!kDPTeam.isDefensivePact(eTeam) && !kDPTeam.isDefensivePact(getID()))
				continue;
			FAssert(!kDPTeam.isAVassal());
			for(int j = 0; j < MAX_CIV_PLAYERS; j++) {
				CvPlayerAI& kMember = GET_PLAYER((PlayerTypes)j);
				if(!kMember.isAlive() || (kMember.getTeam() != eTeam &&
						kMember.getTeam() != getID()))
					continue;
				kDPPlayer.AI_setMemoryCount(kMember.getID(), MEMORY_DECLARED_WAR_RECENT, 0);
				kMember.AI_setMemoryCount(kDPPlayer.getID(), MEMORY_DECLARED_WAR_RECENT, 0);
			}
		}
	} // </advc.130i>

	if (gTeamLogLevel >= 1) // BETTER_BTS_AI_MOD, AI logging, 05/21/10, jdog5000
		logBBAI("    Team %d (%S) and team %d (%S) make peace", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eTeam, GET_PLAYER(GET_TEAM(eTeam).getLeaderID()).getCivilizationDescription(0));

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kMember = GET_PLAYER((PlayerTypes)iI);
		if(!kMember.isAlive())
			continue; // advc.opt
		if (kMember.getTeam() == getID() || kMember.getTeam() == eTeam)
			kMember.updatePlunder(-1, false);
	}
	setAtWar(eTeam, false);
	GET_TEAM(eTeam).setAtWar(getID(), false);
	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kMember = GET_PLAYER((PlayerTypes)iI);
		if(!kMember.isAlive())
			continue; // advc.opt
		if (kMember.getTeam() == getID() || kMember.getTeam() == eTeam)
			kMember.updatePlunder(1, false);
	}

	// BETTER_BTS_AI_MOD, Efficiency: plot danger cache, 08/21/09, jdog5000: START
	GC.getMap().invalidateBorderDangerCache(eTeam);
	GC.getMap().invalidateBorderDangerCache(getID());
	// BETTER_BTS_AI_MOD: END

	AI_setAtWarCounter(eTeam, 0);
	GET_TEAM(eTeam).AI_setAtWarCounter(getID(), 0);

	AI_setWarSuccess(eTeam, 0);
	GET_TEAM(eTeam).AI_setWarSuccess(getID(), 0);

	AI_setWarPlan(eTeam, NO_WARPLAN);
	GET_TEAM(eTeam).AI_setWarPlan(getID(), NO_WARPLAN);
	// <advc.034>
	if(!bCapitulate && GC.getDefineINT(CvGlobals::DISENGAGE_LENGTH) > 0)
		signDisengage(eTeam); // </advc.034>

	if (bBumpUnits)
		GC.getMap().verifyUnitValidPlot();

	GC.getGame().AI_makeAssignWorkDirty();

	if (getID() == GC.getGame().getActiveTeam() ||
			eTeam == GC.getGame().getActiveTeam())
	{
		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(CityInfo_DIRTY_BIT, true);
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kMember = GET_PLAYER((PlayerTypes)iI); // advc
		if (!kMember.isAlive())
			continue;
		if (kMember.getTeam() == getID() || kMember.getTeam() == eTeam)
			kMember.updateWarWearinessPercentAnger();
			kMember.updatePlotGroups(); // advc: was in a separate loop
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kMember = GET_PLAYER((PlayerTypes)iI); // advc
		if (!kMember.isAlive())
			continue;
		if (kMember.getTeam() == getID() || kMember.getTeam() == eTeam)
			kMember.updateTradeRoutes();
	}

	// K-Mod. update attitude
	if (GC.getGame().isFinalInitialized())
	{
		for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
		{
			const CvTeam& kTeam_i = GET_TEAM(GET_PLAYER(i).getTeam());
			if (kTeam_i.getID() == eTeam || kTeam_i.getID() == getID() || kTeam_i.isAtWar(eTeam) || kTeam_i.isAtWar(getID()))
			{
				for (PlayerTypes j = (PlayerTypes)0; j < MAX_CIV_PLAYERS; j=(PlayerTypes)(j+1))
				{	// <advc.001>
					if(i == j)
						continue; // </advc.001>
					TeamTypes eTeam_j = GET_PLAYER(j).getTeam();
					if (eTeam_j == eTeam || eTeam_j == getID())
						GET_PLAYER(i).AI_updateAttitudeCache(j);
				}
			}
		}
	} // K-Mod end

	for(iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer const& kObs = GET_PLAYER((PlayerTypes)iI); // advc
		if(!kObs.isAlive())
			continue;
		// <advc.106b>
		LPCTSTR szSoundYou = "AS2D_MAKEPEACE";
		LPCTSTR szSoundThey = "AS2D_THEIRMAKEPEACE";
		if(isAVassal() && !isHuman() || (GET_TEAM(eTeam).isAVassal() &&
				!GET_TEAM(eTeam).isHuman()))
			szSoundYou = szSoundThey = NULL; // </advc.106b>
		bool bWarTeam = false; // advc.039
		if(kObs.getTeam() == getID()) {
			szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_MADE_PEACE_WITH",
					GET_TEAM(eTeam).getName().GetCString());
			gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(),
					true, GC.getEVENT_MESSAGE_TIME(), szBuffer,
					szSoundYou, // advc.106b
					MESSAGE_TYPE_MAJOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
					// <advc.127b>
					GET_TEAM(eTeam).getCapitalX(kObs.getTeam(), true),
					GET_TEAM(eTeam).getCapitalY(kObs.getTeam(), true));
					// </advc.127b>
			bWarTeam = true; // advc.039
		}
		else if(kObs.getTeam() == eTeam) {
			szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_MADE_PEACE_WITH",
					getName().GetCString());
			gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(),
					true, GC.getEVENT_MESSAGE_TIME(), szBuffer,
					szSoundYou, // advc.106b
					MESSAGE_TYPE_MAJOR_EVENT, NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
					// <advc.127b>
					getCapitalX(kObs.getTeam()), getCapitalY(kObs.getTeam()));
					// </advc.127b>
			/*  <advc.039> Show message about reparations also to non-leading
				members of a (human) team (in addition to YOU_MADE_PEACE) */
			bWarTeam = true;
		}
		//else
		if((!bWarTeam || (pReparations != NULL && kObs.getID() != getLeaderID() &&
				kObs.getID() != GET_TEAM(eTeam).getLeaderID())) &&
				// </advc.039>
				((isHasMet(kObs.getTeam()) && GET_TEAM(eTeam).isHasMet(kObs.getTeam()))
				|| kObs.isSpectator())) { // advc.127
			// <advc.039>
			bool bReparations = false;
			if(pReparations != NULL) {
				szBuffer = gDLL->getText("TXT_KEY_MISC_PEACE_IN_EXCHANGE",
						getName().GetCString(), GET_TEAM(eTeam).getName().GetCString()) + L" ";
				for(CLLNode<TradeData>* pNode = pReparations->head(); pNode != NULL;
						pNode = pReparations->next(pNode)) {
					CvWString const szItem = tradeItemString(
							pNode->m_data.m_eItemType, pNode->m_data.m_iData, eTeam);
					if(szItem.length() <= 0)
						continue;
					bReparations = true;
					szBuffer += szItem;
					if(pReparations->next(pNode) != NULL) {
						if(pReparations->next(pReparations->next(pNode)) == NULL)
							szBuffer += L" " + gDLL->getText("TXT_KEY_AND") + L" ";
						else szBuffer += L", ";
					}
					else szBuffer += L".";
				} // Can handle it, but I don't think it should happen:
				FAssert(bReparations);
			}
			if(!bReparations) { // </advc.039>
				szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_MADE_PEACE",
						getName().GetCString(), GET_TEAM(eTeam).getName().GetCString());
			}
			gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(),
					false, GC.getEVENT_MESSAGE_TIME(), szBuffer,
					// <advc.106b>
					szSoundThey, (isAVassal() || GET_TEAM(eTeam).isAVassal() ?
					MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY : // </advc.106b>
					MESSAGE_TYPE_MAJOR_EVENT), NULL,
					(bReparations ? NO_COLOR : // advc.039
					(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT")),
					// <advc.127b>
					getCapitalX(kObs.getTeam(), true),
					getCapitalY(kObs.getTeam(), true)); // </advc.127b>
		}
	}
	// <advc.106g>
	if(bRandomEvent) {
		szBuffer = gDLL->getText("TXT_KEY_MISC_PEACE_VIA_EVENT", getReplayName().
				GetCString(), GET_TEAM(eTeam).getReplayName().GetCString());
	}
	else { // </advc.106g>
		szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_MADE_PEACE", getReplayName().
				GetCString(), GET_TEAM(eTeam).getReplayName().GetCString());
	} // <advc.100b>
	if(eBroker != NO_TEAM && eBroker != eTeam && eBroker != getID())
		szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_BROKERED_PEACE",
				getReplayName().GetCString(),
				GET_TEAM(eTeam).getReplayName().GetCString(),
				GET_TEAM(eBroker).getReplayName().GetCString()); // </advc.100b>
	GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(),
			szBuffer, -1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));

	CvEventReporter::getInstance().changeWar(false, getID(), eTeam);

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (iI != getID() && iI != eTeam)
		{
			CvTeam& kVassal = GET_TEAM((TeamTypes)iI); // advc
			if (!kVassal.isAlive())
				continue;
			if (kVassal.isVassal(eTeam))
				kVassal.makePeace(getID(), bBumpUnits);
			else if (kVassal.isVassal(getID()))
				kVassal.makePeace(eTeam, bBumpUnits);
		}
	}
}

// K-Mod. I've added bCheckWillingness.
// note. I would have done this the same way in CvPlayer::canContact
// but unfortunately, changing the signature of that function causes the game to crash - because it's a dll export.
bool CvTeam::canContact(TeamTypes eTeam, bool bCheckWillingness) const
{
	int iI, iJ;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				for (iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isAlive())
					{
						if (GET_PLAYER((PlayerTypes)iJ).getTeam() == eTeam)
						{
							//if (GET_PLAYER((PlayerTypes)iI).canContact((PlayerTypes)iJ))
							if (bCheckWillingness
								? GET_PLAYER((PlayerTypes)iI).canContactAndTalk((PlayerTypes)iJ)
								: GET_PLAYER((PlayerTypes)iI).canContact((PlayerTypes)iJ))
							{
								return true;
							}
						}
					}
				}
			}
		}
	}

	return false;
}


void CvTeam::meet(TeamTypes eTeam, bool bNewDiplo,
		FirstContactData* pData) { // advc.071: Just passing this along

	if(isHasMet(eTeam))
		return; // advc

	makeHasMet(eTeam, bNewDiplo, pData);
	GET_TEAM(eTeam).makeHasMet(getID(), bNewDiplo, pData);
	// BETTER_BTS_AI_MOD, AI logging, 02/20/10, jdog5000: START
	if(gTeamLogLevel >= 2 && GC.getGame().isFinalInitialized()) {
		if(eTeam != getID() && isAlive() && GET_TEAM(eTeam).isAlive())
			logBBAI("    Team %d (%S) meets team %d (%S)", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eTeam, GET_PLAYER(GET_TEAM(eTeam).getLeaderID()).getCivilizationDescription(0));
	} // BETTER_BTS_AI_MOD: END
}

// K-Mod
void CvTeam::signPeaceTreaty(TeamTypes eTeam)
{
	TradeData item;
	setTradeItem(&item, TRADE_PEACE_TREATY);

	if (GET_PLAYER(getLeaderID()).canTradeItem(GET_TEAM(eTeam).getLeaderID(), item) && GET_PLAYER(GET_TEAM(eTeam).getLeaderID()).canTradeItem(getLeaderID(), item))
	{
		CLinkList<TradeData> ourList;
		CLinkList<TradeData> theirList;

		ourList.insertAtEnd(item);
		theirList.insertAtEnd(item);

		GC.getGame().implementDeal(getLeaderID(), (GET_TEAM(eTeam).getLeaderID()), &ourList, &theirList);
	}
}
// K-Mod end

void CvTeam::signOpenBorders(TeamTypes eTeam)
{
	CLinkList<TradeData> ourList;
	CLinkList<TradeData> theirList;
	TradeData item;

	FAssert(eTeam != NO_TEAM);
	FAssert(eTeam != getID());

	if (!isAtWar(eTeam) && (getID() != eTeam))
	{
		setTradeItem(&item, TRADE_OPEN_BORDERS);

		if (GET_PLAYER(getLeaderID()).canTradeItem(GET_TEAM(eTeam).getLeaderID(), item) && GET_PLAYER(GET_TEAM(eTeam).getLeaderID()).canTradeItem(getLeaderID(), item))
		{
			ourList.clear();
			theirList.clear();

			ourList.insertAtEnd(item);
			theirList.insertAtEnd(item);

			GC.getGame().implementDeal(getLeaderID(), (GET_TEAM(eTeam).getLeaderID()), &ourList, &theirList);
		}
	}
}

// <advc.034> Sign a disengagement agreement (based on signOpenBorders)
void CvTeam::signDisengage(TeamTypes otherId) {

	CvTeam& other = GET_TEAM(otherId);
	TradeData item;
	setTradeItem(&item, TRADE_DISENGAGE);
	if(!GET_PLAYER(getLeaderID()).canTradeItem(other.getLeaderID(), item) ||
			!GET_PLAYER(other.getLeaderID()).canTradeItem(getLeaderID(), item))
		return;
	CLinkList<TradeData> ourList;
	CLinkList<TradeData> theirList;
	ourList.insertAtEnd(item);
	theirList.insertAtEnd(item);
	GC.getGame().implementDeal(getLeaderID(), other.getLeaderID(),
			&ourList, &theirList);
} // </advc.034>


void CvTeam::signDefensivePact(TeamTypes eTeam)
{
	CLinkList<TradeData> ourList;
	CLinkList<TradeData> theirList;
	TradeData item;

	FAssert(eTeam != NO_TEAM);
	FAssert(eTeam != getID());

	if (!isAtWar(eTeam) && (getID() != eTeam))
	{
		setTradeItem(&item, TRADE_DEFENSIVE_PACT);

		if (GET_PLAYER(getLeaderID()).canTradeItem(GET_TEAM(eTeam).getLeaderID(), item) && GET_PLAYER(GET_TEAM(eTeam).getLeaderID()).canTradeItem(getLeaderID(), item))
		{
			ourList.clear();
			theirList.clear();

			ourList.insertAtEnd(item);
			theirList.insertAtEnd(item);

			GC.getGame().implementDeal(getLeaderID(), (GET_TEAM(eTeam).getLeaderID()), &ourList, &theirList);
		}
	}
}

bool CvTeam::canSignDefensivePact(TeamTypes eTeam) /* advc: */ const
{
	for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
	{
		if (iTeam != getID() && iTeam != eTeam)
		{
			CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
			if (kLoopTeam.isPermanentWarPeace(eTeam) != kLoopTeam.isPermanentWarPeace(getID()))
			{
				return false;
			}

			if (isPermanentWarPeace((TeamTypes)iTeam) != GET_TEAM(eTeam).isPermanentWarPeace((TeamTypes)iTeam))
			{
				return false;
			}
		}
	}

	return true;
}


int CvTeam::getAssets() const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iCount += GET_PLAYER((PlayerTypes)iI).getAssets();
			}
		}
	}
	return iCount;
}


int CvTeam::getPower(bool bIncludeVassals) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.isAlive())
		{
			if (kLoopPlayer.getTeam() == getID() || (bIncludeVassals && GET_TEAM(kLoopPlayer.getTeam()).isVassal(getID())))
			{
				iCount += kLoopPlayer.getPower();
			}
		}
	}
	return iCount;
}


int CvTeam::getDefensivePower(TeamTypes eExcludeTeam) const
{
	int iCount = 0;

	FAssert(eExcludeTeam != getID());
	// K-Mod. only our master will have defensive pacts.
	const CvTeam& kMasterTeam = GET_TEAM(getMasterTeam());

	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		const CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
		/*  K-Mod. (vassal check unnecessary, b/c vassals can't be the master team,
			and they can't have a pact.) */
		//if (kLoopTeam.isAlive() && !kLoopTeam.isAVassal())
		if(!kLoopTeam.isAlive() || iI == eExcludeTeam)
			continue;
		//if (getID() == iI || isVassal((TeamTypes)iI) || isDefensivePact((TeamTypes)iI))
		if (kMasterTeam.getID() == iI ||
				kMasterTeam.isDefensivePact(kLoopTeam.getID())) // K-Mod
			iCount += kLoopTeam.getPower(true);
	}
	return iCount;
}

// advc.003j (comment): Unused. Added by the BtS expansion; looks like it was never used.
int CvTeam::getEnemyPower() const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
		if (kLoopTeam.isAlive())
		{
			if (getID() != iI && isAtWar((TeamTypes)iI))
			{
				iCount += kLoopTeam.getPower(false);
			}
		}
	}
	return iCount;
}


int CvTeam::getNumNukeUnits() const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.isAlive())
		{
			if (kLoopPlayer.getTeam() == getID() || GET_TEAM(kLoopPlayer.getTeam()).isVassal(getID()))
			{
				iCount += kLoopPlayer.getNumNukeUnits();
			}
		}
	}
	return iCount;
}


int CvTeam::getVotes(VoteTypes eVote, VoteSourceTypes eVoteSource) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.getTeam() == getID())
		{
			iCount += kLoopPlayer.getVotes(eVote, eVoteSource);
		}
	}
	return iCount;
}


bool CvTeam::isVotingMember(VoteSourceTypes eVoteSource) const
{
	return (getVotes(NO_VOTE, eVoteSource) > 0);
}


bool CvTeam::isFullMember(VoteSourceTypes eVoteSource) const
{
	if (isForceTeamVoteEligible(eVoteSource))
	{
		return true;
	}

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.getTeam() == getID())
		{
			if (!kLoopPlayer.isFullMember(eVoteSource))
			{
				return false;
			}
		}
	}

	return true;
}

// BETTER_BTS_AI_MOD, General AI, 07/20/09, jdog5000: START
int CvTeam::getAtWarCount(bool bIgnoreMinors, bool bIgnoreVassals) const {
// <advc.003m> Cached
	int r = m_iMajorWarEnemies;
	if(!bIgnoreMinors)
		r += m_iMinorWarEnemies;
	if(bIgnoreVassals)
		r -= m_iVassalWarEnemies;
	FAssert(r >= 0);
	return r;
}

void CvTeam::changeAtWarCount(int iChange, bool bMinorTeam, bool bVassal) {

	if(bMinorTeam)
		m_iMinorWarEnemies += iChange;
	else m_iMajorWarEnemies += iChange;
	if(bVassal)
		m_iVassalWarEnemies += iChange;
	FAssert(!bMinorTeam || !bVassal);
}

int CvTeam::countWarEnemies(bool bIgnoreMinors, bool bIgnoreVassals) const
	// </advc.003m>
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (!bIgnoreMinors || !GET_TEAM((TeamTypes)iI).isMinorCiv())
			{
				if (!bIgnoreVassals || !GET_TEAM((TeamTypes)iI).isAVassal())
				{
					if (isAtWar((TeamTypes)iI))
					{
						FAssert(iI != getID());
						// advc.006: Disabled; see K-Mod comment.
						//FAssert(!(AI_isSneakAttackPreparing((TeamTypes)iI)) // K-Mod note. This assert can fail when in the process of declaring war
						iCount++;
					}
				}
			}
		}
	}
	return iCount;
}
// BETTER_BTS_AI_MOD: END
// <dlph.3> (actually an advc change)
bool CvTeam::allWarsShared(TeamTypes eOther,
		bool bCheckBothWays) const { // advc.130f

	for(int i = 0; i < MAX_CIV_TEAMS; i++) {
		CvTeam const& kLoopTeam = GET_TEAM((TeamTypes)i);
		if(!kLoopTeam.isAlive())
			continue;
		if(bCheckBothWays && // advc.130f
				kLoopTeam.isAtWar(getID()) != kLoopTeam.isAtWar(eOther))
			return false;
		// <advc.130f>
		if(!kLoopTeam.isAtWar(eOther) && kLoopTeam.isAtWar(getID()))
			return false; // </advc.130f>
	}
	return true;
} // </dlph.3>

// <advc.130s>
bool CvTeam::anyWarShared(TeamTypes eOther) const {

	for(int i = 0; i < MAX_CIV_TEAMS; i++) {
		CvTeam const& kLoopTeam = GET_TEAM((TeamTypes)i);
		if(kLoopTeam.isAlive() && kLoopTeam.isAtWar(getID()) == kLoopTeam.isAtWar(eOther))
			return true;
	}
	return false;
} // </dlph.3>


int CvTeam::getWarPlanCount(WarPlanTypes eWarPlan, bool bIgnoreMinors) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (!bIgnoreMinors || !(GET_TEAM((TeamTypes)iI).isMinorCiv()))
			{
				if (AI_getWarPlan((TeamTypes)iI) == eWarPlan)
				{
					FAssert(iI != getID());
					iCount++;
				}
			}
		}
	}
	return iCount;
}


int CvTeam::getAnyWarPlanCount(bool bIgnoreMinors) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (!bIgnoreMinors || !(GET_TEAM((TeamTypes)iI).isMinorCiv()))
			{
				if (AI_getWarPlan((TeamTypes)iI) != NO_WARPLAN)
				{
					FAssert(iI != getID());
					iCount++;
				}
			}
		}
	}
	FAssert(iCount >= getAtWarCount(bIgnoreMinors));
	return iCount;
}

// advc: This is never called, not from Python either.
int CvTeam::getChosenWarCount(bool bIgnoreMinors) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (!bIgnoreMinors || !(GET_TEAM((TeamTypes)iI).isMinorCiv()))
			{
				if (AI_isChosenWar((TeamTypes)iI))
				{
					FAssert(iI != getID());
					iCount++;
				}
			}
		}
	}
	return iCount;
}


int CvTeam::getHasMetCivCount(bool bIgnoreMinors) const
{
	PROFILE_FUNC(); // advc.opt: Would be easy enough to cache this
	int iCount = 0;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (iI != getID())
			{
				if (!bIgnoreMinors || !(GET_TEAM((TeamTypes)iI).isMinorCiv()))
				{
					if (isHasMet((TeamTypes)iI))
					{
						iCount++;
					}
				}
			}
		}
	}
	return iCount;
}


bool CvTeam::hasMetHuman() const
{
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (iI != getID())
			{
				if (GET_TEAM((TeamTypes)iI).isHuman())
				{
					if (isHasMet((TeamTypes)iI))
					{
						FAssert(iI != getID());
						return true;
					}
				}
			}
		}
	}

	return false;
}


int CvTeam::getDefensivePactCount(TeamTypes eTeam) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (iI != getID())
			{
				if (isDefensivePact((TeamTypes)iI))
				{
					if (NO_TEAM == eTeam || GET_TEAM(eTeam).isHasMet((TeamTypes)iI))
					{
						iCount++;
					}
				}
			}
		}
	}
	return iCount;
}

int CvTeam::getVassalCount(TeamTypes eTeam) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
		if (kLoopTeam.isAlive())
		{
			if (iI != getID())
			{
				if (kLoopTeam.isVassal(getID()))
				{
					if (NO_TEAM == eTeam || GET_TEAM(eTeam).isHasMet((TeamTypes)iI))
					{
						iCount++;
					}
				}
			}
		}
	}
	return iCount;
}


bool CvTeam::canVassalRevolt(TeamTypes eMaster) const
{
	FAssert(NO_TEAM != eMaster);

	CvTeam& kMaster = GET_TEAM(eMaster);

	// <advc.112>
	if(isLossesAllowRevolt(eMaster))
		return true; // </advc.112>

	if (GC.getDefineINT("FREE_VASSAL_LAND_PERCENT") < 0 ||
			100 * std::max(10, getTotalLand(false)) < // advc.112: Lower bound added
			kMaster.getTotalLand(false) * GC.getDefineINT("FREE_VASSAL_LAND_PERCENT"))
	{
		return false;
	}

	if (GC.getDefineINT("FREE_VASSAL_POPULATION_PERCENT") < 0 ||
		100 * getTotalPopulation(false) < kMaster.getTotalPopulation(false) * GC.getDefineINT("FREE_VASSAL_POPULATION_PERCENT"))
	{
		return false;
	}

	return true;
}

// <advc.112> Cut from canVassalRevolt
bool CvTeam::isLossesAllowRevolt(TeamTypes eMaster) const {

	CvTeam& kMaster = GET_TEAM(eMaster);
	if (isVassal(eMaster))
	{	// advc.112: Lower bound 10 added
		if (100 * std::max(10, getTotalLand(false)) < GC.getDefineINT("VASSAL_REVOLT_OWN_LOSSES_FACTOR") * getVassalPower())
		{
			return true;
		}
		// advc (note): this INT is 0
		if (100 * kMaster.getTotalLand() < GC.getDefineINT("VASSAL_REVOLT_MASTER_LOSSES_FACTOR") * getMasterPower())
		{
			return true;
		}
	}
	return false;
} // </advc.112>


int CvTeam::getUnitClassMaking(UnitClassTypes eUnitClass) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iCount += GET_PLAYER((PlayerTypes)iI).getUnitClassMaking(eUnitClass);
			}
		}
	}
	return iCount;
}


int CvTeam::getUnitClassCountPlusMaking(UnitClassTypes eIndex) const
{
	return (getUnitClassCount(eIndex) + getUnitClassMaking(eIndex));
}


int CvTeam::getBuildingClassMaking(BuildingClassTypes eBuildingClass) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iCount += GET_PLAYER((PlayerTypes)iI).getBuildingClassMaking(eBuildingClass);
			}
		}
	}
	return iCount;
}


int CvTeam::getBuildingClassCountPlusMaking(BuildingClassTypes eIndex) const
{
	return (getBuildingClassCount(eIndex) + getBuildingClassMaking(eIndex));
}


int CvTeam::getHasReligionCount(ReligionTypes eReligion) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iCount += GET_PLAYER((PlayerTypes)iI).getHasReligionCount(eReligion);
			}
		}
	}
	return iCount;
}


int CvTeam::getHasCorporationCount(CorporationTypes eCorporation) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iCount += GET_PLAYER((PlayerTypes)iI).getHasCorporationCount(eCorporation);
			}
		}
	}
	return iCount;
}


int CvTeam::countTotalCulture() const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iCount += GET_PLAYER((PlayerTypes)iI).countTotalCulture();
			}
		}
	}
	return iCount;
}


// <advc.302>
bool CvTeam::isInContactWithBarbarians() const {

	CvGame const& g = GC.getGame();
	if(g.isOption(GAMEOPTION_NO_BARBARIANS))
		return true; // Needed for advc.314 (free unit from goody hut)
	bool bCheckCity = g.getElapsedGameTurns() >=
			GC.getGameSpeedInfo(g.getGameSpeedType()).getBarbPercent();
	// (Perhaps just iUnitThresh=1 would have the same effect)
	int iUnitThresh = g.getCurrentEra();
	CvTeam const& kBarbarianTeam = GET_TEAM(BARBARIAN_TEAM);
	CvMap const& m = GC.getMap();
	FOR_EACH_AREA_VAR(pArea) {
		if(bCheckCity && countNumCitiesByArea(pArea) == 0)
			continue;
		if(!bCheckCity && countNumUnitsByArea(pArea) < iUnitThresh)
			continue;
		if(bCheckCity) {
			int iBarbarianCities = kBarbarianTeam.countNumCitiesByArea(pArea);
			//  Always allow barbs to progress in their main area (if any).
			if(2 * iBarbarianCities > kBarbarianTeam.getNumCities())
				return true;
			/*  Only allow barb research to progress if they're at least half as
				important as an average civ. */
			int iCityThresh = (pArea->getNumCities() - iBarbarianCities) /
					(2 * GC.getGame().countCivPlayersAlive());
			if(iBarbarianCities > iCityThresh)
				return true;
			else continue;
		}
		int iBarbarianUnits = kBarbarianTeam.countNumUnitsByArea(pArea);
		if(iBarbarianUnits > iUnitThresh) // Preliminary check to save time
			return true;
		std::vector<Shelf*> sh;
		m.getShelves(pArea->getID(), sh);
		for(size_t i = 0; i < sh.size(); i++)
			iBarbarianUnits += sh[i]->countBarbarians();
		if(iBarbarianUnits > iUnitThresh) // Actual check incl. ships
			return true;
	}
	return false;
} // </advc.302>


int CvTeam::countNumUnitsByArea(CvArea* pArea) const
{
	PROFILE_FUNC();

	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
				iCount += pArea->getUnitsPerPlayer((PlayerTypes)iI);
		}
	}
	return iCount;
}


int CvTeam::countNumCitiesByArea(CvArea* pArea) const
{
	PROFILE_FUNC();

	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
				iCount += pArea->getCitiesPerPlayer((PlayerTypes)iI);
		}
	}
	return iCount;
}


int CvTeam::countTotalPopulationByArea(CvArea* pArea) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
				iCount += pArea->getPopulationPerPlayer((PlayerTypes)iI);
		}
	}
	return iCount;
}


int CvTeam::countPowerByArea(CvArea* pArea) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
				iCount += pArea->getPower((PlayerTypes)iI);
		}
	}
	return iCount;
}


int CvTeam::countEnemyPowerByArea(CvArea* pArea) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() != getID())
			{
				//if (isAtWar(GET_PLAYER((PlayerTypes)iI).getTeam()))
				// BETTER_BTS_AI_MOD, General AI, 01/11/09, jdog5000: Count planned wars as well
				if (AI_getWarPlan(GET_PLAYER((PlayerTypes)iI).getTeam()) != NO_WARPLAN)
					iCount += pArea->getPower((PlayerTypes)iI);
			}
		}
	}
	return iCount;
}

// K-Mod
// Note: this includes barbarian cities.
int CvTeam::countEnemyCitiesByArea(CvArea* pArea) const
{
	int iCount = 0;
	for (PlayerTypes i = (PlayerTypes)0; i < MAX_PLAYERS; i=(PlayerTypes)(i+1))
	{
		const CvPlayer& kLoopPlayer = GET_PLAYER(i);
		if (kLoopPlayer.isAlive() && AI_getWarPlan(kLoopPlayer.getTeam()) != NO_WARPLAN)
			iCount += pArea->getCitiesPerPlayer(i);
	}
	return 0;
}
// K-Mod end

// BETTER_BTS_AI_MOD, War strategy AI, 04/01/10, jdog5000: START
// advc.003j (comment): unused
int CvTeam::countEnemyPopulationByArea(CvArea* pArea) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() != getID())
			{
				if (AI_getWarPlan(GET_PLAYER((PlayerTypes)iI).getTeam()) != NO_WARPLAN)
					iCount += pArea->getPopulationPerPlayer((PlayerTypes)iI);
			}
		}
	}
	return iCount;
} // BETTER_BTS_AI_MOD: END


int CvTeam::countNumAIUnitsByArea(CvArea* pArea, UnitAITypes eUnitAI) const
{
	PROFILE_FUNC();

	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
				iCount += pArea->getNumAIUnits(((PlayerTypes)iI), eUnitAI);
		}
	}
	return iCount;
}

// BETTER_BTS_AI_MOD, War strategy AI, 05/19/10, jdog5000
int CvTeam::countEnemyDangerByArea(CvArea* pArea, TeamTypes eEnemyTeam) const
{
	PROFILE_FUNC();

	int iCount = 0;
	for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(iI);
		if (pLoopPlot != NULL)
		{
			if (pLoopPlot->area() == pArea)
			{
				if (pLoopPlot->getTeam() == getID())
				{
					iCount += pLoopPlot->plotCount(PUF_canDefendEnemy, getLeaderID(),
							false, NO_PLAYER, eEnemyTeam, PUF_isVisible, getLeaderID());
				}
			}
		}
	}
	return iCount;
} // BETTER_BTS_AI_MOD: END

// <advc.112b>
EraTypes CvTeam::getCurrentEra() const {

	double sum = 0;
	int iDiv = 0;
	for(int i = 0; i < MAX_PLAYERS; i++) {
		CvPlayer const& kMember = GET_PLAYER((PlayerTypes)i);
		if(kMember.isAlive() && kMember.getTeam() == getID()) {
			iDiv++;
			sum += kMember.getCurrentEra();
		}
	}
	if(iDiv == 0) {
		FAssertMsg(false, "No team members alive");
		return (EraTypes)0;
	}
	return (EraTypes)::round(sum / iDiv);
} // </advc.112b>

// K-Mod
int CvTeam::getTypicalUnitValue(UnitAITypes eUnitAI, DomainTypes eDomain) const
{
	int iMax = 0;
	for (int iI = 0; iI < MAX_PLAYERS; ++iI)
	{
		if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
		{
			iMax = std::max(iMax, GET_PLAYER((PlayerTypes)iI).getTypicalUnitValue(eUnitAI, eDomain));
		}
	}
	return iMax;
}

int CvTeam::getResearchCost(TechTypes eTech, bool bGlobalModifiers, bool bTeamSizeModifiers) const // K-Mod added bGlobalModifiers & bTeamSizeModifiers
{
	FAssertMsg(eTech != NO_TECH, "Tech is not assigned a valid value");
	CvGame const& g = GC.getGame();

	// advc.251: To reduce rounding errors (as there are quite a few modifiers to apply)
	double cost = GC.getTechInfo(eTech).getResearchCost();
	cost *= 0.01 * GC.getHandicapInfo(getHandicapType()).getResearchPercent();
	// <advc.251>
	if(!isHuman() && !isBarbarian()) {
		// Important to use game handicap here (not team handicap)
		cost *= 0.01 * (GC.getHandicapInfo(g.getHandicapType()).
				getAIResearchPercent() + g.AIHandicapAdjustment());
	}
	// <advc.910> Moved from CvPlayer::calculateResearchModifier
	EraTypes eTechEra = (EraTypes)GC.getTechInfo(eTech).getEra();
	int iModifier = 100 + GC.getEraInfo(eTechEra).getTechCostModifier();
	/*  This is a BBAI tech diffusion thing, but, since it applies always, I think
		it's better to let it reduce the tech cost than to modify research rate. */
	static int const iTECH_COST_MODIFIER = GC.getDefineINT("TECH_COST_MODIFIER");
	iModifier += iTECH_COST_MODIFIER;
	// </advc.910>
	if (bGlobalModifiers) // K-Mod
	{	// advc:
		CvWorldInfo const& kWorld = GC.getWorldInfo(GC.getMap().getWorldSize());
		if(eTechEra > 0) { // advc.910
			cost *= 0.01 * kWorld.getResearchPercent();
			// advc.910:
			cost *= 0.01 * GC.getSeaLevelInfo(GC.getMap().getSeaLevel()).getResearchPercent();
		}
		cost *= 0.01 * GC.getGameSpeedInfo(g.getGameSpeedType()).getResearchPercent();
		cost *= 0.01 * GC.getEraInfo(g.getStartEra()).getResearchPercent();
		// <advc.308>
		if(g.isOption(GAMEOPTION_RAGING_BARBARIANS) && g.getStartEra() == 0) {
			switch(eTechEra) {
			case 1: iModifier -= 14; break;
			case 2: iModifier -= 7; break;
			}
		} // </advc.308>
		// <advc.550d>
		if(g.isOption(GAMEOPTION_NO_TECH_TRADING) && eTechEra > 0 && eTechEra < 6) {
			static int const iTECH_COST_NOTRADE_MODIFIER = GC.getDefineINT("TECH_COST_NOTRADE_MODIFIER");
			iModifier += std::max(0, ::round((iTECH_COST_NOTRADE_MODIFIER + 5 *
					std::pow(std::abs(eTechEra - 2.5), 1.5)) *
					::dRange((kWorld.getDefaultPlayers() - 2) / 6.0, 0, 2)));
		} // </advc.550d>
	}

	if (bTeamSizeModifiers) // K-Mod
	{
		cost *= 0.01 * std::max(0, 100 +
				GC.getDefineINT(CvGlobals::TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER) *
				(getNumMembers() - 1));
	}
	// <advc.251>
	cost *= 0.01 * std::max(1, iModifier);
	int iCost = ::roundToMultiple(cost, isHuman() ? 5 : 1);
	// </advc.251>
	return std::max(1, iCost);
}


int CvTeam::getResearchLeft(TechTypes eTech) const
{
	return std::max(0, (getResearchCost(eTech) - getResearchProgress(eTech)));
}


bool CvTeam::hasHolyCity(ReligionTypes eReligion) const
{
	FAssertMsg(eReligion != NO_RELIGION, "Religion is not assigned a valid value");

	CvCity* pHolyCity = GC.getGame().getHolyCity(eReligion);

	if (pHolyCity != NULL)
	{
		return (pHolyCity->getTeam() == getID());
	}

	return false;
}


bool CvTeam::hasHeadquarters(CorporationTypes eCorporation) const
{
	FAssertMsg(eCorporation != NO_CORPORATION, "Corporation is not assigned a valid value");

	CvCity* pHeadquarters = GC.getGame().getHeadquarters(eCorporation);

	if (pHeadquarters != NULL)
	{
		return (pHeadquarters->getTeam() == getID());
	}

	return false;
}

bool CvTeam::hasBonus(BonusTypes eBonus) const
{
	for (int iI = 0; iI < MAX_PLAYERS; ++iI)
	{
		if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
		{
			if (GET_PLAYER((PlayerTypes)iI).hasBonus(eBonus))
			{
				return true;
			}
		}
	}

	return false;
}

bool CvTeam::isBonusObsolete(BonusTypes eBonus) const
{
	TechTypes eObsoleteTech = (TechTypes) GC.getBonusInfo(eBonus).getTechObsolete();
	if (eObsoleteTech != NO_TECH && isHasTech(eObsoleteTech))
	{
		return true;
	}
	return false;
}

/*  <advc.301>: Only used for Barbarians so far. Should arguably also check if the
	resource can be used, however, the only case in which this matters, is Fusion,
	which isn't relevant for Barbarians. */
bool CvTeam::canSeeReqBonuses(UnitTypes eUnit) {

	CvUnitInfo& kUnit = GC.getUnitInfo(eUnit);
	BonusTypes eAndBonus = (BonusTypes)kUnit.getPrereqAndBonus();
	if(eAndBonus != NO_BONUS && !isBonusRevealed(eAndBonus))
		return false;
	bool bAllBlank = true; // Handle dummy NONE XML elements
	for(int i = 0; i < GC.getNUM_UNIT_PREREQ_OR_BONUSES(eUnit); i++) {
		BonusTypes eOrBonus = (BonusTypes)kUnit.getPrereqOrBonuses(i);
		if(eOrBonus != NO_BONUS) {
			bAllBlank = false;
			if(isBonusRevealed(eOrBonus))
				return true;
		}
	}
	return bAllBlank;
} // </advc.301>


bool CvTeam::isHuman() const
{
	PROFILE_FUNC();

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
		{
			if (GET_PLAYER((PlayerTypes)iI).isHuman())
			{
				return true;
			}
		}
	}

	return false;
}


bool CvTeam::checkMinorCiv() const // advc.003m: Renamed
{
	bool bValid = false;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
		{
			//if (GET_PLAYER((PlayerTypes)iI).isMinorCiv())
			/*  advc.003m: Bypass CvPlayer::isMinorCiv b/c that funtion won't work
				while loading a savegame */
			if(GC.getInitCore().getMinorNationCiv((PlayerTypes)iI))
			{
				bValid = true;
			}
			else
			{
				return false;
			}
		}
	}

	return bValid;
}

// <advc.opt> This gets called a lot; now precomputed.
PlayerTypes CvTeam::getLeaderID() const {

	return m_eLeader;
}


void CvTeam::updateLeaderID() {

	PlayerTypes eFormerLeader = getLeaderID();
	bool bDone = false;
	for (int iI = 0; iI < MAX_PLAYERS; iI++) {
		if (GET_PLAYER((PlayerTypes)iI).isAlive()) {
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID()) {
				m_eLeader = (PlayerTypes)iI;
				bDone = true;
				break;
			}
		}
	}
	if(!bDone) {
		for (int iI = 0; iI < MAX_PLAYERS; iI++) {
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID()) {
				m_eLeader = (PlayerTypes)iI;
				bDone = true;
				break;
			}
		}
	}
	if(bDone) {
		// <advc.104t>
		 if(getWPAI.isEnabled() && eFormerLeader != m_eLeader)
			GET_PLAYER(m_eLeader).warAndPeaceAI().getCache().onTeamLeaderChanged(eFormerLeader);
		 // </advc.104t>
	}
	else m_eLeader = NO_PLAYER;
} // </advc.opt>


PlayerTypes CvTeam::getSecretaryID() const
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).isHuman())
			{
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
				{
					return ((PlayerTypes)iI);
				}
			}
		}
	}

	return getLeaderID();
}


HandicapTypes CvTeam::getHandicapType() const
{
	int iGameHandicap = 0;
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iGameHandicap += GC.getHandicapInfo(GET_PLAYER((PlayerTypes)iI).
						getHandicapType()).getDifficulty(); // advc.250a
				iCount++;
			}
		}
	}

	if (iCount > 0)
	{
		//FAssertMsg((iGameHandicap / iCount) >= 0, "(iGameHandicap / iCount) is expected to be non-negative (invalid Index)");
		// advc.250a: (also disabled the assertion above)
		return (HandicapTypes)std::min(GC.getNumHandicapInfos() - 1,
				(iGameHandicap / (10 * iCount)));
	}
	else
	{
		return ((HandicapTypes)(GC.getDefineINT("STANDARD_HANDICAP")));
	}
}


CvWString CvTeam::getName() const
{
	CvWString szBuffer;
	bool bFirst = true;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive() && GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
		{
			setListHelp(szBuffer, L"", GET_PLAYER((PlayerTypes)iI).getName(), L"/", bFirst);
			bFirst = false;
		}
	}
	return szBuffer;
}

// K-Mod. Name to be used in replay
CvWString CvTeam::getReplayName() const
{
	CvWString szBuffer;
	bool bFirst = true;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive() && GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
		{
			setListHelp(szBuffer, L"", GET_PLAYER((PlayerTypes)iI).getReplayName(), L"/", bFirst);
			bFirst = false;
		}
	}
	return szBuffer;
}
// K-Mod end

int CvTeam::getNumMembers() const
{
	return m_iNumMembers;
}


void CvTeam::changeNumMembers(int iChange)
{
	m_iNumMembers += iChange;
	FAssert(getNumMembers() >= 0);
}


int CvTeam::getAliveCount() const
{
	return m_iAliveCount;
}


void CvTeam::changeAliveCount(int iChange)
{
	m_iAliveCount += iChange;
	FAssert(getAliveCount() >= 0);

	// free vassals
	if (m_iAliveCount == 0)
	{
		for (int iTeam = 0; iTeam < MAX_TEAMS; iTeam++)
		{
			if (iTeam != getID())
			{
				CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
				if (kLoopTeam.isAlive() && !kLoopTeam.isBarbarian() && !isBarbarian())
				{
					if (kLoopTeam.isVassal(getID()))
						kLoopTeam.setVassal(getID(), false, false);
					/*  advc.004v (inspired by a Kek-Mod change):
						So that the identity of dead teams isn't concealed */
					kLoopTeam.makeHasSeen(getID());
				}
				/*  <advc.003m> So that AtWarCounts are updated. Also seems prudent
					in general not to keep dead teams at war. */
				kLoopTeam.setAtWar(getID(), false);
				setAtWar(kLoopTeam.getID(), false);
				// </advc.003m>
			}
		}
	} // <advc.opt>
	if(!isBarbarian() && m_iAliveCount - iChange <= 0 && m_iAliveCount > 0)
		GC.getGame().changeCivTeamsEverAlive(1); // </advc.opt>
}


int CvTeam::getEverAliveCount() const
{
	return m_iEverAliveCount;
}


int CvTeam::isEverAlive() const
{
	return (getEverAliveCount() > 0);
}


void CvTeam::changeEverAliveCount(int iChange)
{
	m_iEverAliveCount += iChange;
	FAssert(getEverAliveCount() >= 0);
}


int CvTeam::getNumCities() const
{
	return m_iNumCities;
}


void CvTeam::changeNumCities(int iChange)
{
	m_iNumCities += iChange;
	FAssert(getNumCities() >= 0);
}


int CvTeam::getTotalPopulation(bool bCheckVassals) const
{
	int iVassalPop = 0;

	if (bCheckVassals)
	{
		if (isAVassal())
		{
			return m_iTotalPopulation / 2;
		}

		for (int iI = 0; iI < MAX_TEAMS; iI++)
		{
			if ((TeamTypes)iI != getID())
			{
				CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
				if (kLoopTeam.isAlive() && kLoopTeam.isVassal(getID()))
				{
					iVassalPop += kLoopTeam.getTotalPopulation(false) / 2;
				}
			}
		}
	}

	return (m_iTotalPopulation + iVassalPop);
}


void CvTeam::changeTotalPopulation(int iChange)
{
	m_iTotalPopulation += iChange;
	FAssert(getTotalPopulation() >= 0);
}


int CvTeam::getTotalLand(bool bCheckVassals) const
{
	int iVassalLand = 0;

	if (bCheckVassals)
	{
		if (isAVassal())
		{
			return m_iTotalLand / 2;
		}

		for (int iI = 0; iI < MAX_TEAMS; iI++)
		{
			if ((TeamTypes)iI != getID())
			{
				CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
				if (kLoopTeam.isAlive() && kLoopTeam.isVassal(getID()))
				{
					iVassalLand += kLoopTeam.getTotalLand(false) / 2;
				}
			}
		}
	}

	return (m_iTotalLand + iVassalLand);
}


void CvTeam::changeTotalLand(int iChange)
{
	m_iTotalLand += iChange;
	FAssert(getTotalLand() >= 0);
}


int CvTeam::getNukeInterception() const
{
	return m_iNukeInterception;
}


void CvTeam::changeNukeInterception(int iChange)
{
	m_iNukeInterception += iChange;
	FAssert(getNukeInterception() >= 0);
}


int CvTeam::getForceTeamVoteEligibilityCount(VoteSourceTypes eVoteSource) const
{
	return m_aiForceTeamVoteEligibilityCount[eVoteSource];
}


bool CvTeam::isForceTeamVoteEligible(VoteSourceTypes eVoteSource) const
{
	return ((getForceTeamVoteEligibilityCount(eVoteSource) > 0) && !isMinorCiv()
			&& !isCapitulated()); // advc.014
}


void CvTeam::changeForceTeamVoteEligibilityCount(VoteSourceTypes eVoteSource, int iChange)
{
	m_aiForceTeamVoteEligibilityCount[eVoteSource] += iChange;
	FAssert(getForceTeamVoteEligibilityCount(eVoteSource) >= 0);
}


int CvTeam::getExtraWaterSeeFromCount() const
{
	return m_iExtraWaterSeeFromCount;
}


bool CvTeam::isExtraWaterSeeFrom() const
{
	return (getExtraWaterSeeFromCount() > 0);
}


void CvTeam::changeExtraWaterSeeFromCount(int iChange)
{
	if (iChange != 0)
	{
		GC.getMap().updateSight(false);

		m_iExtraWaterSeeFromCount = (m_iExtraWaterSeeFromCount + iChange);
		FAssert(getExtraWaterSeeFromCount() >= 0);

		GC.getMap().updateSight(true);
	}
}


int CvTeam::getMapTradingCount() const
{
	return m_iMapTradingCount;
}


bool CvTeam::isMapTrading()	const
{
	return (getMapTradingCount() > 0);
}


void CvTeam::changeMapTradingCount(int iChange)
{
	m_iMapTradingCount = (m_iMapTradingCount + iChange);
	FAssert(getMapTradingCount() >= 0);
}


int CvTeam::getTechTradingCount() const
{
	return m_iTechTradingCount;
}


bool CvTeam::isTechTrading() const
{
	return (getTechTradingCount() > 0);
}


void CvTeam::changeTechTradingCount(int iChange)
{
	m_iTechTradingCount = (m_iTechTradingCount + iChange);
	FAssert(getTechTradingCount() >= 0);
}


int CvTeam::getGoldTradingCount() const
{
	return m_iGoldTradingCount;
}


bool CvTeam::isGoldTrading() const
{
	return (getGoldTradingCount() > 0);
}


void CvTeam::changeGoldTradingCount(int iChange)
{
	m_iGoldTradingCount = (m_iGoldTradingCount + iChange);
	FAssert(getGoldTradingCount() >= 0);
}


int CvTeam::getOpenBordersTradingCount() const
{
	return m_iOpenBordersTradingCount;
}


bool CvTeam::isOpenBordersTrading() const
{
	return (getOpenBordersTradingCount() > 0);
}


void CvTeam::changeOpenBordersTradingCount(int iChange)
{
	m_iOpenBordersTradingCount = (m_iOpenBordersTradingCount + iChange);
	FAssert(getOpenBordersTradingCount() >= 0);
}


int CvTeam::getDefensivePactTradingCount() const
{
	return m_iDefensivePactTradingCount;
}


bool CvTeam::isDefensivePactTrading() const
{
	return (getDefensivePactTradingCount() > 0);
}


void CvTeam::changeDefensivePactTradingCount(int iChange)
{
	m_iDefensivePactTradingCount = (m_iDefensivePactTradingCount + iChange);
	FAssert(getDefensivePactTradingCount() >= 0);
}


int CvTeam::getPermanentAllianceTradingCount() const
{
	return m_iPermanentAllianceTradingCount;
}


bool CvTeam::isPermanentAllianceTrading() const
{
	if (!GC.getGame().isOption(GAMEOPTION_PERMANENT_ALLIANCES))
	{
		return false;
	}

	return (getPermanentAllianceTradingCount() > 0);
}


void CvTeam::changePermanentAllianceTradingCount(int iChange)
{
	m_iPermanentAllianceTradingCount = (m_iPermanentAllianceTradingCount + iChange);
	FAssert(getPermanentAllianceTradingCount() >= 0);
}


int CvTeam::getVassalTradingCount() const
{
	return m_iVassalTradingCount;
}


bool CvTeam::isVassalStateTrading() const
{
	if (GC.getGame().isOption(GAMEOPTION_NO_VASSAL_STATES))
	{
		return false;
	}

	return (getVassalTradingCount() > 0);
}


void CvTeam::changeVassalTradingCount(int iChange)
{
	m_iVassalTradingCount += iChange;
	FAssert(getVassalTradingCount() >= 0);
}


int CvTeam::getBridgeBuildingCount() const
{
	return m_iBridgeBuildingCount;
}


bool CvTeam::isBridgeBuilding()	const
{
	return (getBridgeBuildingCount() > 0);
}


void CvTeam::changeBridgeBuildingCount(int iChange)
{
	if (iChange != 0)
	{
		m_iBridgeBuildingCount = (m_iBridgeBuildingCount + iChange);
		FAssert(getBridgeBuildingCount() >= 0);

		if (GC.IsGraphicsInitialized())
		{
			gDLL->getEngineIFace()->MarkBridgesDirty();
		}
	}
}


int CvTeam::getIrrigationCount() const
{
	return m_iIrrigationCount;
}


bool CvTeam::isIrrigation() const
{
	return (getIrrigationCount() > 0);
}


void CvTeam::changeIrrigationCount(int iChange)
{
	if (iChange != 0)
	{
		m_iIrrigationCount = (m_iIrrigationCount + iChange);
		FAssert(getIrrigationCount() >= 0);

		GC.getMap().updateIrrigated();
	}
}


int CvTeam::getIgnoreIrrigationCount() const
{
	return m_iIgnoreIrrigationCount;
}


bool CvTeam::isIgnoreIrrigation() const
{
	return (getIgnoreIrrigationCount() > 0);
}


void CvTeam::changeIgnoreIrrigationCount(int iChange)
{
	m_iIgnoreIrrigationCount = (m_iIgnoreIrrigationCount + iChange);
	FAssert(getIgnoreIrrigationCount() >= 0);
}


int CvTeam::getWaterWorkCount() const
{
	return m_iWaterWorkCount;
}


bool CvTeam::isWaterWork() const
{
	return (getWaterWorkCount() > 0);
}


void CvTeam::changeWaterWorkCount(int iChange)
{
	if (iChange != 0)
	{
		m_iWaterWorkCount = (m_iWaterWorkCount + iChange);
		FAssert(getWaterWorkCount() >= 0);

		AI_makeAssignWorkDirty();
	}
}

int CvTeam::getVassalPower() const
{
	return m_iVassalPower;
}

void CvTeam::setVassalPower(int iPower)
{
	m_iVassalPower = iPower;
}

int CvTeam::getMasterPower() const
{
	return m_iMasterPower;
}

void CvTeam::setMasterPower(int iPower)
{
	m_iMasterPower = iPower;
}

int CvTeam::getEnemyWarWearinessModifier() const
{
	return m_iEnemyWarWearinessModifier;
}

void CvTeam::changeEnemyWarWearinessModifier(int iChange)
{
	m_iEnemyWarWearinessModifier += iChange;
}

bool CvTeam::isMapCentering() const
{
	return m_bMapCentering;
}


void CvTeam::setMapCentering(bool bNewValue)
{
	if (isMapCentering() != bNewValue)
	{
		m_bMapCentering = bNewValue;

		if (getID() == GC.getGame().getActiveTeam())
		{
			gDLL->getInterfaceIFace()->setDirty(MinimapSection_DIRTY_BIT, true);
		}
	}
}


int CvTeam::getStolenVisibilityTimer(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "iIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "iIndex is expected to be within maximum bounds (invalid Index)");

	return m_aiStolenVisibilityTimer[eIndex];
}


bool CvTeam::isStolenVisibility(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "iIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "iIndex is expected to be within maximum bounds (invalid Index)");

	return (getStolenVisibilityTimer(eIndex) > 0);
}


void CvTeam::setStolenVisibilityTimer(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");

	if(getStolenVisibilityTimer(eIndex) == iNewValue)
		return;

	bool bOldStolenVisibility = isStolenVisibility(eIndex);

	m_aiStolenVisibilityTimer[eIndex] = iNewValue;
	FAssert(getStolenVisibilityTimer(eIndex) >= 0);

	if (bOldStolenVisibility != isStolenVisibility(eIndex))
	{
		for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
		{
			CvPlot* pLoopPlot = GC.getMap().plotByIndex(iI);

			if (pLoopPlot->isVisible(eIndex, false))
			{
				pLoopPlot->changeStolenVisibilityCount(getID(), ((isStolenVisibility(eIndex)) ? 1 : -1));
			}
		}
	}
}


void CvTeam::changeStolenVisibilityTimer(TeamTypes eIndex, int iChange)
{
	setStolenVisibilityTimer(eIndex, (getStolenVisibilityTimer(eIndex) + iChange));
}


// (K-Mod note: units are unhappiness per 100,000 population. ie. 1000 * percent unhappiness.)
int CvTeam::getWarWeariness(TeamTypes eIndex, bool bUseEnemyModifer) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	//return m_aiWarWeariness[eIndex];
	return bUseEnemyModifer ? m_aiWarWeariness[eIndex] * std::max(0, 100 + GET_TEAM(eIndex).getEnemyWarWearinessModifier())/100 : m_aiWarWeariness[eIndex]; // K-Mod
}


void CvTeam::setWarWeariness(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiWarWeariness[eIndex] = std::max(0, iNewValue);
}


void CvTeam::changeWarWeariness(TeamTypes eIndex, int iChange)
{
	FAssert(eIndex >= 0 && eIndex < MAX_TEAMS);
	setWarWeariness(eIndex, getWarWeariness(eIndex) + iChange);
}

void CvTeam::changeWarWeariness(TeamTypes eOtherTeam, const CvPlot& kPlot, int iFactor)
{
	int iOurCulture = kPlot.countFriendlyCulture(getID());
	int iTheirCulture = kPlot.countFriendlyCulture(eOtherTeam);

	int iRatio = 100;
	if (iOurCulture + iTheirCulture != 0)
	{
		iRatio = (100 * iTheirCulture) / (iOurCulture + iTheirCulture);
	}

	changeWarWeariness(eOtherTeam, iRatio * iFactor);
}


int CvTeam::getTechShareCount(int iIndex) const
{
	FAssertMsg(iIndex >= 0, "iIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(iIndex < MAX_TEAMS, "iIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiTechShareCount[iIndex];
}


bool CvTeam::isTechShare(int iIndex) const
{
	return (getTechShareCount(iIndex) > 0);
}


void CvTeam::changeTechShareCount(int iIndex, int iChange)
{
	FAssertMsg(iIndex >= 0, "iIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(iIndex < MAX_TEAMS, "iIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_aiTechShareCount[iIndex] = (m_aiTechShareCount[iIndex] + iChange);
		FAssert(getTechShareCount(iIndex) >= 0);

		if (isTechShare(iIndex))
		{
			updateTechShare();
		}
	}
}


int CvTeam::getCommerceFlexibleCount(CommerceTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiCommerceFlexibleCount[eIndex];
}


bool CvTeam::isCommerceFlexible(CommerceTypes eIndex) const
{
	return (getCommerceFlexibleCount(eIndex) > 0);
}


void CvTeam::changeCommerceFlexibleCount(CommerceTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_COMMERCE_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_aiCommerceFlexibleCount[eIndex] = (m_aiCommerceFlexibleCount[eIndex] + iChange);
		FAssert(getCommerceFlexibleCount(eIndex) >= 0);

		if (getID() == GC.getGame().getActiveTeam())
		{
			gDLL->getInterfaceIFace()->setDirty(PercentButtons_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		}
	}
}


int CvTeam::getExtraMoves(DomainTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiExtraMoves[eIndex];
}


void CvTeam::changeExtraMoves(DomainTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_DOMAIN_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiExtraMoves[eIndex] = (m_aiExtraMoves[eIndex] + iChange);
	FAssert(getExtraMoves(eIndex) >= 0);
}


bool CvTeam::isHasMet(TeamTypes eIndex)	const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	//FAssert((eIndex != getID()) || m_abHasMet[eIndex]);
	return m_abHasMet[eIndex];
}

void CvTeam::makeHasMet(TeamTypes eIndex, bool bNewDiplo,
		FirstContactData* pData) // advc.071
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");

	if(isHasMet(eIndex))
		return;

	makeHasSeen(eIndex); // K-mod
	m_abHasMet[eIndex] = true;

	updateTechShare();

	/* original bts code
	if(GET_TEAM(eIndex).isHuman()) {
		for (iI = 0; iI < MAX_PLAYERS; iI++) {
			if (GET_PLAYER((PlayerTypes)iI).isAlive()) {
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID()) {
					if (!(GET_PLAYER((PlayerTypes)iI).isHuman())) {
						GET_PLAYER((PlayerTypes)iI).clearResearchQueue();
						GET_PLAYER((PlayerTypes)iI).AI_makeProductionDirty();
					}}}}}*/
	// disabled by K-Mod (wtf did they hope to achieve with this?)

	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (GET_TEAM((TeamTypes)iI).isVassal(getID()) || isVassal((TeamTypes)iI))
			{
				GET_TEAM((TeamTypes)iI).meet(eIndex, bNewDiplo,
						pData); // advc.071
			}
		}
	}
	// report event to Python, along with some other key state
	CvEventReporter::getInstance().firstContact(getID(), eIndex);
	// <advc.071> ^Moved EventReporter call up // advc.001n:
	if(eIndex == getID() || isBarbarian() || GET_TEAM(eIndex).isBarbarian())
		return; // </advc.071>
	// K-Mod: Initialize attitude cache for players on our team towards player's on their team.
	AI().AI_updateAttitudeCache(eIndex); // advc: Loop replaced

	if (getID() == GC.getGame().getActiveTeam() || eIndex == GC.getGame().getActiveTeam())
		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
	// <advc.071>
	bool bShowMessage = isHuman() && pData != NULL;
	if(bShowMessage || bNewDiplo) {
		int iOnFirstContact = 1;
		// If met during the placement of free starting units, show only a diplo popup.
		if(GC.IsGraphicsInitialized())
			iOnFirstContact = getBugOptionINT("Civ4lerts__OnFirstContact", 2);
		if(bNewDiplo && iOnFirstContact == 0)
			bNewDiplo = false;
		if(bShowMessage && iOnFirstContact == 1)
			bShowMessage = false;
	} // </advc.071>
	if (GC.getGame().isOption(GAMEOPTION_ALWAYS_WAR))
	{
		if (isHuman() && getID() != eIndex)
		{
			declareWar(eIndex, false, NO_WARPLAN);
		}
	}  // advc: Style changes in the else branch to reduce indentation
	else if (GC.getGame().isFinalInitialized() && !gDLL->GetWorldBuilderMode() &&
			bNewDiplo && !isHuman() && !isAtWar(eIndex))
	{
		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			CvPlayer const& kMember = GET_PLAYER((PlayerTypes)iI);
			if (!kMember.isAlive() || kMember.getTeam() != eIndex)
				continue;
			if (kMember.isHuman() && GET_PLAYER(getLeaderID()).canContact(kMember.getID()))
			{
				CvDiploParameters* pDiplo = new CvDiploParameters(getLeaderID());
				FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
				pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_FIRST_CONTACT"));
				pDiplo->setAIContact(true);
				gDLL->beginDiplomacy(pDiplo, kMember.getID());
			}
		}
	}
	// <advc.071>
	if(bShowMessage) {
		FirstContactData fcData = *pData;
		CvMap const& m = GC.getMap();
		CvPlot const* pAt1 = m.plot(fcData.x1, fcData.y1);
		CvPlot const* pAt2 = m.plot(fcData.x2, fcData.y2);
		CvUnit const* pUnit1 = ::getUnit(fcData.u1);
		CvUnit const* pUnit2 = ::getUnit(fcData.u2);
		CvUnit const* pUnitMet = NULL;
		CvPlot const* pAt = NULL;
		PlayerTypes ePlayerMet = NO_PLAYER;
		if(pUnit1 != NULL && pUnit1->getTeam() == eIndex) {
			ePlayerMet = pUnit1->getOwner();
			if(pUnit1->plot()->isVisible(getID(), false))
				pUnitMet = pUnit1;
		}
		if(pUnit2 != NULL && pUnit2->getTeam() == eIndex) {
			if(ePlayerMet == NO_PLAYER)
				ePlayerMet = pUnit2->getOwner();
			if(pUnit2->plot()->isVisible(getID(), false))
				pUnitMet = pUnit2;
		}
		if(pAt1 != NULL && pAt1->isOwned() && pAt1->getTeam() == eIndex) {
			if(pAt1->isVisible(getID(), false))
				pAt = pAt1;
			if(ePlayerMet == NO_PLAYER)
				ePlayerMet = pAt1->getOwner();
		}
		if(pAt2 != NULL && pAt2->isOwned() && pAt2->getTeam() == eIndex) {
			if(pAt2->isVisible(getID(), false))
				pAt = pAt2;
			if(ePlayerMet == NO_PLAYER)
				ePlayerMet = pAt2->getOwner();
		}
		if(ePlayerMet == NO_PLAYER)
			ePlayerMet = GET_TEAM(eIndex).getLeaderID();
		if(pUnitMet != NULL && pUnitMet->plot()->isVisible(getID(), false))
			pAt = pUnitMet->plot();
		if(pAt == NULL) { // We can't see any of their tiles or units, but they see ours.
			if(pAt1 != NULL && pAt1->isOwned() && pAt1->getTeam() == getID())
				pAt = pAt1;
			else if(pAt2 != NULL && pAt2->isOwned() && pAt2->getTeam() == getID())
				pAt = pAt2;
			else if(pUnit1 != NULL && pUnit1->getTeam() == getID()) {
				//pUnitMet = pUnit1; // Better not to show our own unit's icon
				pAt = pUnit1->plot();
			}
			else if(pUnit2 != NULL && pUnit2->getTeam() == getID()) {
				//pUnitMet = pUnit2;
				pAt = pUnit2->plot();
			}
		}
		CvWString szMsg = gDLL->getText("TXT_KEY_MISC_TEAM_MET",
				GET_PLAYER(ePlayerMet).getCivilizationAdjectiveKey());
		ColorTypes ePlayerColor = GET_PLAYER(ePlayerMet).getPlayerTextColor();
		LPCSTR icon = (pUnitMet == NULL ? GC.getLeaderHeadInfo(GET_PLAYER(ePlayerMet).
				getLeaderType()).getButton() : pUnitMet->getButton());
		for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
			CvPlayer& kMember = GET_PLAYER((PlayerTypes)i);
			if(kMember.isAlive() && kMember.isHuman() && kMember.getTeam() == getID()) {
				gDLL->getInterfaceIFace()->addHumanMessage(kMember.getID(),
						false, GC.getEVENT_MESSAGE_TIME(), szMsg, NULL,
						MESSAGE_TYPE_MINOR_EVENT, icon, (ColorTypes)ePlayerColor,
						pAt == NULL ? -1 : pAt->getX(), pAt == NULL ? -1 : pAt->getY(),
						pAt != NULL, pAt != NULL);
			}
		}
	} // </advc.071>
}

// <advc.134a>
bool CvTeam::isAtWarExternal(TeamTypes eIndex) const
{
	/*  Feign peace if we know that the EXE is about to check a peace offer
		(b/c being at war shouldn't prevent AI-to-human peace offers). */
	if(m_iPeaceOfferStage == 2 && m_eOfferingPeace == eIndex)
	{
		const_cast<CvTeam*>(this)->m_iPeaceOfferStage = 0;
		const_cast<CvTeam*>(this)->m_eOfferingPeace = NO_TEAM;
		return false;
	} 
	return isAtWar(eIndex);
} 


void CvTeam::advancePeaceOfferStage(TeamTypes eAITeam)
{
	if(eAITeam != NO_TEAM)
		m_eOfferingPeace = eAITeam;
	m_iPeaceOfferStage++;
}


bool CvTeam::isPeaceOfferStage(int iStage, TeamTypes eOffering) const {

	return (eOffering == m_eOfferingPeace && iStage == m_iPeaceOfferStage);
} // </advc.134a>


void CvTeam::setAtWar(TeamTypes eIndex, bool bNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	// <advc.035>
	if(m_abAtWar[eIndex] == bNewValue)
		return; // </advc.035>
	m_abAtWar[eIndex] = bNewValue;
	// <advc.003m>
	if(eIndex != BARBARIAN_TEAM)
	{
		changeAtWarCount(bNewValue ? 1 : -1, GET_TEAM(eIndex).isMinorCiv(),
				GET_TEAM(eIndex).isAVassal());
	} // </advc.003m>
	// <advc.035>
	if(eIndex < getID()
			|| !isAlive() || !GET_TEAM(eIndex).isAlive()) // advc.003m
		return; // setAtWar gets called on both sides; do this only once.
	std::vector<CvPlot*> flipPlots;
	::contestedPlots(flipPlots, getID(), eIndex);
	for(size_t i = 0; i < flipPlots.size(); i++)
	{
		CvPlot& p = *flipPlots[i];
		PlayerTypes secondOwner = p.getSecondOwner();
		p.setSecondOwner(p.getOwner());
		/*  I guess it's a bit cleaner (faster?) not to bump units and update
			plot groups until all tiles are flipped */
		p.setOwner(secondOwner, false, false);
	}
	for(size_t i = 0; i < flipPlots.size(); i++)
	{
		CvPlot& p = *flipPlots[i];
		p.updatePlotGroup();
		p.verifyUnitValidPlot();
	}
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayerAI& ourMember = GET_PLAYER((PlayerTypes)i);
		if(!ourMember.isAlive() || ourMember.getTeam() != getID())
			continue;
		for(int j = 0; j < MAX_CIV_PLAYERS; j++)
		{
			CvPlayerAI& theirMember = GET_PLAYER((PlayerTypes)j);
			if(!theirMember.isAlive() || theirMember.getTeam() != eIndex)
				continue;
			ourMember.AI_updateCloseBorderAttitudeCache(theirMember.getID());
			theirMember.AI_updateCloseBorderAttitudeCache(ourMember.getID());
		}
	} // (AttitudeCache is updated by caller)
	// </advc.035>
}

/*  <advc.162> "Just" meaning on the current turn. Don't want to rely on
	CvTeamAI::AI_atWarCounter for this b/c that's an AI function. */
bool CvTeam::hasJustDeclaredWar(TeamTypes eIndex) const
{
	return m_abJustDeclaredWar[eIndex];
} // </advc.162>


bool CvTeam::isPermanentWarPeace(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_abPermanentWarPeace[eIndex];
}


void CvTeam::setPermanentWarPeace(TeamTypes eIndex, bool bNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_abPermanentWarPeace[eIndex] = bNewValue;
}


bool CvTeam::isFreeTrade(TeamTypes eIndex) const
{
	if (isAtWar(eIndex))
	{
		return false;
	}

	if (!isHasMet(eIndex))
	{
		return false;
	}

	return (isOpenBorders(eIndex) || GC.getGame().isFreeTrade());
}


bool CvTeam::isOpenBorders(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_abOpenBorders[eIndex];
}


void CvTeam::setOpenBorders(TeamTypes eIndex, bool bNewValue)
{
	int iI;
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");

	if(isOpenBorders(eIndex) == bNewValue)
		return; // advc
	bool bOldFreeTrade = isFreeTrade(eIndex);
	m_abOpenBorders[eIndex] = bNewValue;
	// <advc.130p> OB affect diplo from rival trade
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayerAI& kOther = GET_PLAYER((PlayerTypes)i);
		if(kOther.getTeam() == getID())
			continue;
		for(int j = 0; j < MAX_CIV_PLAYERS; j++) {
			CvPlayerAI& kMember = GET_PLAYER((PlayerTypes)j);
			if(kMember.getTeam() == getID())
				kOther.AI_updateAttitudeCache(kMember.getID());
		}
	} // </advc.130p>
	AI_setOpenBordersCounter(eIndex, 0);

	GC.getMap().verifyUnitValidPlot();

	if (getID() == GC.getGame().getActiveTeam() || eIndex == GC.getGame().getActiveTeam())
	{
		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
	}

	if (bOldFreeTrade != isFreeTrade(eIndex))
	{
		for (iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
				{
					GET_PLAYER((PlayerTypes)iI).updateTradeRoutes();
				}
			}
		}
	} // <advc.034>
	if(bNewValue)
		AI().cancelDisengage(eIndex); // </advc.034>
}

// <advc.034>
bool CvTeam::isDisengage(TeamTypes eIndex) const {

	if(eIndex >= MAX_CIV_TEAMS || eIndex < 0)
		return false;
	return m_abDisengage[eIndex];
}

void CvTeam::setDisengage(TeamTypes eIndex, bool bNewValue) {

	if(eIndex >= MAX_CIV_TEAMS || eIndex < 0)
		return;
	m_abDisengage[eIndex] = bNewValue;
	if(!bNewValue && !isFriendlyTerritory(eIndex) && !isAtWar(eIndex))
		GC.getMap().verifyUnitValidPlot(); // Bump units
}

void CvTeam::cancelDisengage(TeamTypes otherId) {

	FOR_EACH_DEAL_VAR(d) {
		if(d->isDisengage() && d->isBetween(getID(), otherId)) {
			d->kill(false);
			break;
		}
	}
} // </advc.034>


bool CvTeam::isDefensivePact(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_abDefensivePact[eIndex];
}


void CvTeam::setDefensivePact(TeamTypes eIndex, bool bNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (isDefensivePact(eIndex) == bNewValue)
		return; // advc
	m_abDefensivePact[eIndex] = bNewValue;
	if(getID() == GC.getGame().getActiveTeam() ||
			eIndex == GC.getGame().getActiveTeam())
		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
	CvTeam const& kOther = GET_TEAM(eIndex); // advc
	if (bNewValue && !kOther.isDefensivePact(getID()))
	{
		CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_PLAYERS_SIGN_DEFENSIVE_PACT", getReplayName().GetCString(), kOther.getReplayName().GetCString());

		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(), szBuffer, -1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));


		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			CvPlayer& kObs = GET_PLAYER((PlayerTypes)iI);
			if (kObs.isAlive())
			{
				if ((isHasMet(kObs.getTeam()) && kOther.isHasMet(kObs.getTeam()))
						|| kObs.isSpectator()) // advc.127
				{
					gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(),
						false, GC.getEVENT_MESSAGE_TIME(), szBuffer,
						"AS2D_THEIRALLIANCE", MESSAGE_TYPE_MAJOR_EVENT, NULL,
						(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
						// <advc.127b>
						getCapitalX(kObs.getTeam(), true),
						getCapitalY(kObs.getTeam(), true)); // </advc.127b>
				}
			}
		}
	}
	// <advc.106f> Based on the previous block
	else if(!bNewValue && kOther.isDefensivePact(getID())) {
		CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_PLAYERS_CANCEL_DEFENSIVE_PACT",
				getReplayName().GetCString(), kOther.getReplayName().GetCString());
		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT,
				getLeaderID(), szBuffer, -1, -1,
				(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
		for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
			CvPlayer& kObs = GET_PLAYER((PlayerTypes)i);
			if(!kObs.isAlive() || kObs.getTeam() == getID())
				continue;
			if((isHasMet(kObs.getTeam()) && kOther.isHasMet(kObs.getTeam()))
					|| kObs.isSpectator()) { // advc.127
				gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(), false,
						GC.getEVENT_MESSAGE_TIME(), szBuffer, NULL,
						//"AS2D_DEAL_CANCELLED" // Rather use no sound
						MESSAGE_TYPE_MAJOR_EVENT, NULL, NO_COLOR,
						// <advc.127b>
						getCapitalX(kObs.getTeam(), true),
						getCapitalY(kObs.getTeam(), true)); // </advc.127b>
			}
		}
	} // </advc.106f>
	// K-Mod. update attitude
	if (GC.getGame().isFinalInitialized())
	{
		for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
		{
			GET_PLAYER(i).AI_updateAttitudeCache();
		}
	}
	// K-Mod end
}


bool CvTeam::isForcePeace(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_abForcePeace[eIndex];
}


void CvTeam::setForcePeace(TeamTypes eIndex, bool bNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_abForcePeace[eIndex] = bNewValue;

	if (isForcePeace(eIndex))
	{
		if (AI_isSneakAttackPreparing(eIndex))
		{
			AI_setWarPlan(eIndex, NO_WARPLAN);
		}

		for (int iTeam = 0; iTeam < MAX_TEAMS; ++iTeam)
		{
			if (GET_TEAM((TeamTypes)iTeam).isVassal(eIndex))
			{
				if (AI_isSneakAttackPreparing((TeamTypes)iTeam))
				{
					AI_setWarPlan((TeamTypes)iTeam, NO_WARPLAN);
				}
			}
		}
	}
}

// <advc.104>
int CvTeam::turnsOfForcedPeaceRemaining(TeamTypes eOther) const {

	if(!canEventuallyDeclareWar(eOther))
		return MAX_INT;
	if(!isForcePeace(eOther))
		return 0;
	TeamTypes eOurMaster = getMasterTeam();
	TeamTypes eTheirMaster = GET_TEAM(eOther).getMasterTeam();
	int r = 0;
	FOR_EACH_DEAL(d) {
		TeamTypes eFirstMaster = GET_PLAYER(d->getFirstPlayer()).getMasterTeam();
		TeamTypes eSecondMaster = GET_PLAYER(d->getSecondPlayer()).getMasterTeam();
		if(((eFirstMaster == eOurMaster && eSecondMaster == eTheirMaster) ||
				(eFirstMaster == eTheirMaster && eSecondMaster == eOurMaster)) &&
				d->headFirstTradesNode() != NULL &&
				d->headFirstTradesNode()->m_data.m_eItemType == TRADE_PEACE_TREATY)
			r = std::max(r, d->turnsToCancel());
	}
	return r;
} // </advc.104>


bool CvTeam::isVassal(TeamTypes eMaster) const  // advc: param was called eIndex
{
	FASSERT_BOUNDS(0, MAX_TEAMS, eMaster, "CvTeam::isVassal");
	return m_abVassal[eMaster];
}

// advc: First param was called eIndex
void CvTeam::setVassal(TeamTypes eMaster, bool bNewValue, bool bCapitulated)
{
	FASSERT_BOUNDS(0, MAX_CIV_TEAMS, eMaster, "CvTeam::setVassal");
	FAssertMsg(!bNewValue || !GET_TEAM(eMaster).isAVassal(), "can't become a vassal of a vassal");
	bool bWasCapitulated = isCapitulated(); // advc.130v
	/*  <advc> If this function is used for turning capitulated into
		voluntary vassals at some point, then the code for processing this change
		will have to be placed before this clause, or will also have to check if
		isCapitulated()==bCapitulated here. */
	if(isVassal(eMaster) == bNewValue)
		return; // <advc>
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (GET_PLAYER((PlayerTypes)i).getTeam() == getID())
			GET_PLAYER((PlayerTypes)i).updateCitySight(false, false);
	}

	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kLoopPlayer.isAlive() && kLoopPlayer.getTeam() == eMaster)
		{
			FOR_EACH_UNIT(pLoopUnit, kLoopPlayer)
			{
				CvPlot* pPlot = pLoopUnit->plot();
				if (pLoopUnit->getTeam() != pPlot->getTeam() &&
						(pPlot->getTeam() == NO_TEAM ||
						!GET_TEAM(pPlot->getTeam()).isVassal(pLoopUnit->getTeam())))
					kLoopPlayer.changeNumOutsideUnits(-1);
			}
		}
	}

	m_abVassal[eMaster] = bNewValue;
	m_eMaster = (bNewValue ? eMaster : NO_TEAM); // advc.opt
	// <advc.003m>
	for(int i = 0; i < MAX_TEAMS; i++) {
		CvTeam& t = GET_TEAM((TeamTypes)i);
		if(t.isAlive() && isAtWar(t.getID())) {
			t.changeAtWarCount(1, false, bNewValue);
			t.changeAtWarCount(-1, false, !bNewValue);
		}
	} // </advc.003m>
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kLoopPlayer.isAlive() && kLoopPlayer.getTeam() == eMaster)
		{
			FOR_EACH_UNIT(pLoopUnit, kLoopPlayer)
			{
				CvPlot* pPlot = pLoopUnit->plot();
				if (pLoopUnit->getTeam() != pPlot->getTeam() &&
						(pPlot->getTeam() == NO_TEAM ||
						!GET_TEAM(pPlot->getTeam()).isVassal(pLoopUnit->getTeam())))
					kLoopPlayer.changeNumOutsideUnits(1);
			}
		}
	}

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (GET_PLAYER((PlayerTypes)i).getTeam() == getID())
			GET_PLAYER((PlayerTypes)i).updateCitySight(true, false);
	}

	for (int i = 0; i < GC.getMap().numPlots(); ++i)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(i);
		if (pLoopPlot && (pLoopPlot->getTeam() == getID() || pLoopPlot->getTeam() == eMaster))
			pLoopPlot->updateCulture(true, false);
	}

	GC.getGame().updatePlotGroups();

	if (isVassal(eMaster))
	{
		m_bCapitulated = bCapitulated;
		FOR_EACH_DEAL_VAR(pLoopDeal) // advc: some style changes in this loop
		{	// <advc.034>
			if (pLoopDeal->isDisengage() && pLoopDeal->isBetween(eMaster, getID()))
				pLoopDeal->kill();
			// </advc.034>
			if (!pLoopDeal->involves(getID()))
				continue;

			for (CLLNode<TradeData>* pNode = pLoopDeal->headTradesNode(); pNode != NULL;
				pNode = pLoopDeal->nextTradesNode(pNode))
			{
				if (pNode->m_data.m_eItemType == TRADE_DEFENSIVE_PACT ||
					pNode->m_data.m_eItemType == TRADE_PEACE_TREATY)
				{
					pLoopDeal->kill();
					break;
				}
			}
		}
		setForcePeace(eMaster, false);
		GET_TEAM(eMaster).setForcePeace(getID(), false);
		// <advc.130o> Forget tribute demands
		for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
			CvPlayerAI& kOurMember = GET_PLAYER((PlayerTypes)i);
			if(!kOurMember.isAlive() || kOurMember.getTeam() != getID())
				continue;
			for(int j = 0; j < MAX_CIV_PLAYERS; j++) {
				CvPlayer& kOther = GET_PLAYER((PlayerTypes)j);
				if(!kOther.isAlive() || kOther.isMinorCiv())
					continue;
				kOurMember.AI_setMemoryCount(kOther.getID(), MEMORY_MADE_DEMAND, 0);
			}
		} // </advc.130o>
		// advc.130y: Used to be done after declaring wars
		for(int i = 0; i < MAX_CIV_TEAMS; i++) {
			CvTeam& t = GET_TEAM((TeamTypes)i);
			if(t.getID() == getID() || t.getID() == eMaster || !t.isAlive() ||
					!t.isVassal(getID()))
				continue;
			freeVassal(t.getID());
			// <advc.130y>
			if(t.isAtWar(eMaster))
				t.makePeace(eMaster);
			// </advc.130y>
		}

		// Declare war on teams you should be at war with
		for (int iI = 0; iI < MAX_TEAMS; iI++)
		{
			if (iI == getID() || iI == eMaster)
				continue;
			TeamTypes eThirdParty = (TeamTypes)iI; // advc
			if (!GET_TEAM(eThirdParty).isAlive())
				continue;

			if (GET_TEAM(eMaster).isHasMet(eThirdParty))
				meet(eThirdParty, true);

			if (isHasMet(eThirdParty))
				GET_TEAM(eMaster).meet(eThirdParty, true);

			if (GET_TEAM(eMaster).isAtWar(eThirdParty))
			{
				//declareWar((TeamTypes)iI, false, WARPLAN_DOGPILE);
				// dlph.26: "These wars declared by capitulated vassal don't trigger defensive pacts."
				queueWar(getID(), eThirdParty, false, WARPLAN_DOGPILE, !bCapitulated);
			}
			else if (isAtWar(eThirdParty))
			{
				if (bCapitulated)
					makePeace((TeamTypes)iI);
				else
				{
					//GET_TEAM(eMaster).declareWar((TeamTypes)iI, false, WARPLAN_DOGPILE);
					// dlph.26:
					queueWar(eMaster, eThirdParty, false, WARPLAN_DOGPILE);
				}
			}
		}
		triggerWars(); // dlph.26
		for (int iI = 0; iI < MAX_TEAMS; iI++)
		{
			CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
			if (!kLoopTeam.isAlive())
				continue; // advc

			if (!kLoopTeam.isAtWar(getID()))
			{
				kLoopTeam.AI_setWarPlan(getID(), NO_WARPLAN);
				AI_setWarPlan((TeamTypes)iI, NO_WARPLAN);
			}

			if (!kLoopTeam.isAtWar(eMaster))
			{
				/*  advc.104j: Third parties shouldn't discard their plans
					against the master */
				if(!getWPAI.isEnabled())
					kLoopTeam.AI_setWarPlan(eMaster, NO_WARPLAN);
			}
		}

		// All our vassals now become free
		// advc.130y: (now done earlier)

		setMasterPower(GET_TEAM(eMaster).getTotalLand());
		// advc.112: Lower bound added
		setVassalPower(std::max(10, getTotalLand(false)));

		if (GC.getGame().isFinalInitialized() && !gDLL->GetWorldBuilderMode())
		{
			CvWString szReplayMessage;

			if (bCapitulated)
			{
				szReplayMessage = gDLL->getText("TXT_KEY_MISC_CAPITULATE_AGREEMENT",
						getReplayName().GetCString(), GET_TEAM(eMaster).getReplayName().GetCString());
			}
			else
			{
				szReplayMessage = gDLL->getText("TXT_KEY_MISC_VASSAL_AGREEMENT",
						getReplayName().GetCString(), GET_TEAM(eMaster).getReplayName().GetCString());
			}
			GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT,
					getLeaderID(), szReplayMessage, -1, -1, (ColorTypes)
					GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));

			for (int iI = 0; iI < MAX_PLAYERS; iI++)
			{
				CvPlayer& kObs = GET_PLAYER((PlayerTypes)iI);
				if (!kObs.isAlive()) // advc
					continue;

				if ((isHasMet(kObs.getTeam()) && GET_TEAM(eMaster).isHasMet(kObs.getTeam()))
						|| kObs.isSpectator()) // advc.127
				{
					gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(),
							false, GC.getEVENT_MESSAGE_TIME(), szReplayMessage,
							"AS2D_WELOVEKING", MESSAGE_TYPE_MAJOR_EVENT, NULL,
							(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
							// <advc.127b>
							getCapitalX(kObs.getTeam(), true),
							getCapitalY(kObs.getTeam(), true)); // </advc.127b>
				}
			}
		}
	}
	else
	{
		setMasterPower(0);
		setVassalPower(0);

		if (GC.getGame().isFinalInitialized() && !gDLL->GetWorldBuilderMode() &&
				isAlive() && GET_TEAM(eMaster).isAlive())
		{
			CvWString szReplayMessage;

			if (m_bCapitulated)
			{
				szReplayMessage = gDLL->getText("TXT_KEY_MISC_SURRENDER_REVOLT",
						getReplayName().GetCString(), GET_TEAM(eMaster).getReplayName().GetCString());
			}
			else
			{
				szReplayMessage = gDLL->getText("TXT_KEY_MISC_VASSAL_REVOLT",
						getReplayName().GetCString(), GET_TEAM(eMaster).getReplayName().GetCString());
			}

			GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(), szReplayMessage, -1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));

			for (int iI = 0; iI < MAX_PLAYERS; iI++)
			{
				CvPlayer& kObs = GET_PLAYER((PlayerTypes)iI); // advc
				if (!kObs.isAlive())
					continue;

				CvWString szBuffer;
				if (getID() == kObs.getTeam() || eMaster == kObs.getTeam() ||
						(isHasMet(kObs.getTeam()) &&
						GET_TEAM(eMaster).isHasMet(kObs.getTeam()))
						|| kObs.isSpectator()) // advc.127
					szBuffer = szReplayMessage;

				if (!szBuffer.empty())
				{
					gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(),
							false, GC.getEVENT_MESSAGE_TIME(), szBuffer,
							"AS2D_REVOLTSTART", MESSAGE_TYPE_MAJOR_EVENT, NULL,
							(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
							// <advc.127b>
							getCapitalX(kObs.getTeam(), true),
							getCapitalY(kObs.getTeam(), true)); // </advc.127b>
				}
			}
		}
		// <advc.130y>
		// Don't forgive if it's apparent that we haven't fought wars as a vassal
		if(m_bCapitulated && AI().AI_getSharedWarSuccess(eMaster) +
				GET_TEAM(eMaster).AI_getSharedWarSuccess(getID()) > 0) {
			for(int i = 0; i < MAX_CIV_TEAMS; i++) {
				CvTeamAI& t = GET_TEAM((TeamTypes)i);
				if(t.isAlive() && t.getID() != getID() && t.getID() != eMaster) {
					AI().AI_forgiveEnemy(t.getID(), true, true);
					t.AI_forgiveEnemy(getID(), true, true);
				}
			}
		} // </advc.130y>
		m_bCapitulated = false;
		// <advc.133>
		FOR_EACH_DEAL_VAR(d) {
			if (d->involves(getID()))
			{
				// Treat deal as very old so that turnsToCancel returns 0
				d->setInitialGameTurn(-100);
			}
		} // </advc.133>
		// <advc.104j> Stop any war plans that eMaster may have forced on us
		for(int i = 0; i < MAX_CIV_TEAMS; i++) {
			CvTeamAI const& t = GET_TEAM((TeamTypes)i);
			if(t.isAlive() && t.getID() != getID() && t.getID() != eMaster &&
					!t.isMinorCiv() && !t.isAtWar(getID()))
				AI().AI_setWarPlan(t.getID(), NO_WARPLAN);
		} // </advc.104j>
	}

	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
	{
		CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kLoopPlayer.getTeam() == eMaster)
		{
			kLoopPlayer.updateMaintenance();
			// <advc.130v> Border conflicts of the vassal held against the master
			if(isCapitulated())
				kLoopPlayer.AI_updateCloseBorderAttitudeCache(); // </advc.130v>
		}
	}

	if (GC.getGame().isFinalInitialized() && !gDLL->GetWorldBuilderMode())
		CvEventReporter::getInstance().vassalState(eMaster, getID(), bNewValue);

	// K-Mod. update attitude
	for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
	{
		if (GET_PLAYER(i).getTeam() == getID() || GET_PLAYER(i).getTeam() == eMaster)
		{
			for (PlayerTypes j = (PlayerTypes)0; j < MAX_CIV_PLAYERS; j=(PlayerTypes)(j+1))
			{
				if (GET_PLAYER(j).getTeam() != GET_PLAYER(i).getTeam())
				{
					if (GET_PLAYER(j).getTeam() == getID() || GET_PLAYER(j).getTeam() == eMaster)
					{
						GET_PLAYER(i).AI_updateAttitudeCache(j);
						GET_PLAYER(j).AI_updateAttitudeCache(i);
					}
				}
			}
		}
	} // K-Mod end
	// <advc.014> Early re-election if vote source owner capitulates
	if(isCapitulated())
		GC.getGame().updateSecretaryGeneral(); // </advc.014>
	// <advc.143b>
	if(isCapitulated()) {
		for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
			CvPlayer& kVassalMember = GET_PLAYER((PlayerTypes)i);
			if(!kVassalMember.isAlive() || kVassalMember.getTeam() != getID())
				continue;
			FOR_EACH_UNIT_VAR(u, kVassalMember) {
				if(u->getUnitInfo().getNukeRange() >= 0) // non-nukes have -1
					u->scrap();
			}
		}
	} // </advc.143b>
	// <advc.130v>
	if(bNewValue && bCapitulated && GET_TEAM(eMaster).isHuman()) {
		for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
			CvPlayer& kMasterMember = GET_PLAYER((PlayerTypes)i);
			if(kMasterMember.isAlive() && kMasterMember.getTeam() == eMaster &&
					kMasterMember.isHuman())
				kMasterMember.setEspionageSpendingWeightAgainstTeam(getID(), 0);
		}
	} // </advc.130v>
	// <advc.130f> Delete stopped-trading memory
	if(bWasCapitulated != bCapitulated) {
		for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
			CvPlayerAI& kOurMember = GET_PLAYER((PlayerTypes)i);
			if(!kOurMember.isAlive() || kOurMember.getTeam() != getID())
				continue;
			for(int j = 0; j < MAX_CIV_PLAYERS; j++) {
				CvPlayerAI& kTheirMember = GET_PLAYER((PlayerTypes)j);
				if(!kTheirMember.isAlive() || kTheirMember.getTeam() == getID())
					continue;
				kOurMember.AI_setMemoryCount(kTheirMember.getID(), MEMORY_STOPPED_TRADING, 0);
				kOurMember.AI_setMemoryCount(kTheirMember.getID(), MEMORY_STOPPED_TRADING_RECENT, 0);
				kOurMember.AI_setMemoryCount(kTheirMember.getID(), MEMORY_DECLARED_WAR_RECENT, 0);
				kTheirMember.AI_setMemoryCount(kOurMember.getID(), MEMORY_STOPPED_TRADING, 0);
				kTheirMember.AI_setMemoryCount(kOurMember.getID(), MEMORY_STOPPED_TRADING_RECENT, 0);
				kTheirMember.AI_setMemoryCount(kOurMember.getID(), MEMORY_DECLARED_WAR_RECENT, 0);
			}
		}
	} // </advc.130f
}

// K-Mod. Return the team which is the master of this team. (if this team is free, return getID())
TeamTypes CvTeam::getMasterTeam() const
{
	/*  advc.opt: Since I use this function a lot, I've serialized the master team.
		Also speeds up isAVassal. */
	return (m_eMaster == NO_TEAM ? getID() : m_eMaster);
	/*for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1)) {
		if (isVassal(i) && GET_TEAM(i).isAlive())
			return i;
	}
	return getID();*/
}
// K-Mod end

void CvTeam::assignVassal(TeamTypes eVassal, bool bSurrender) const
{
	CLinkList<TradeData> ourList;
	CLinkList<TradeData> theirList;
	TradeData item;

	GET_TEAM(eVassal).setVassal(getID(), true, bSurrender);

	setTradeItem(&item, bSurrender ? TRADE_SURRENDER : TRADE_VASSAL);
	item.m_iData = 1;

	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kLoopPlayer.getTeam() == eVassal && kLoopPlayer.isAlive())
		{
			ourList.clear();
			theirList.clear();

			theirList.insertAtEnd(item);

			for (int jPlayer = 0; jPlayer < MAX_PLAYERS; jPlayer++)
			{
				if (GET_PLAYER((PlayerTypes)jPlayer).getTeam() == getID())
				{
					GC.getGame().implementDeal(((PlayerTypes)jPlayer), ((PlayerTypes)iPlayer), &ourList, &theirList, true);
				}
			}
		}
	}
}


void CvTeam::freeVassal(TeamTypes eVassal) const
{
	FOR_EACH_DEAL_VAR(pLoopDeal)
	{
		if (!pLoopDeal->isBetween(getID(), eVassal))
			continue;

		for (CLLNode<TradeData>* pNode = pLoopDeal->headGivesNode(eVassal); pNode != NULL;
			pNode = pLoopDeal->nextGivesNode(pNode, eVassal))
		{
			if (pNode->m_data.m_eItemType == TRADE_VASSAL ||
				pNode->m_data.m_eItemType == TRADE_SURRENDER)
			{
				pLoopDeal->kill();
				break;
			}
		}
	}
	// <advc.130y>
	if(isCapitulated() && GET_PLAYER(GET_TEAM(eVassal).getLeaderID()).
			// Not thankful if still thankful to old master
			AI_getMemoryAttitude(getLeaderID(), MEMORY_INDEPENDENCE) <= 0)
		GET_TEAM(eVassal).AI_thankLiberator(getMasterTeam());
	/*  Prevent freed vassal from immediately becoming someone else's vassal.
		Want the civ that made the former master capitulate (i.e. getMasterTeam)
		to have a right of first refusal. */
	CvPlayerAI& kVassalLeader = GET_PLAYER(GET_TEAM(eVassal).getLeaderID());
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayerAI const& kOther = GET_PLAYER((PlayerTypes)i);
		if(kOther.getTeam() == getMasterTeam() || kOther.getTeam() == eVassal || !kOther.isAlive())
			continue;
		if(kVassalLeader.AI_getMemoryCount(kOther.getID(),
				MEMORY_CANCELLED_VASSAL_AGREEMENT) < 2) {
			/*  2 memory, that's 10 turns on average, and guarantees that it can't
				go to 0 in just one turn. */
			kVassalLeader.AI_changeMemoryCount(kOther.getID(),
					MEMORY_CANCELLED_VASSAL_AGREEMENT, 2);
		}
	} // </advc.130y>
}

bool CvTeam::isCapitulated() const
{
	//FAssert(isAVassal());
	FAssert(!m_bCapitulated || isAVassal()); // K-Mod

	return m_bCapitulated;
}

/*  <dlph.26> "Changed how multiple war declarations work. declareWar used to
	nest war declarations, now they are queued to trigger defensive pacts and
	everything else in the correct order." */
void CvTeam::queueWar(TeamTypes eAttackingTeam, TeamTypes eDefendingTeam,
		bool bNewDiplo, WarPlanTypes eWarPlan, bool bPrimaryDOW) {

	attacking_queue.push(eAttackingTeam);
	defending_queue.push(eDefendingTeam);
	newdiplo_queue.push(bNewDiplo);
	warplan_queue.push(eWarPlan);
	primarydow_queue.push(bPrimaryDOW);
}

void CvTeam::triggerWars(bool bForceUpdateAttitude) {

	bool bWarsDeclared = false;
	if(bTriggeringWars)
		return;
	else bTriggeringWars = true;
	while(!attacking_queue.empty()) {
		GET_TEAM(attacking_queue.front()).declareWar(
				defending_queue.front(), newdiplo_queue.front(),
				warplan_queue.front(), primarydow_queue.front());
		attacking_queue.pop();
		defending_queue.pop();
		newdiplo_queue.pop();
		warplan_queue.pop();
		primarydow_queue.pop();
		bWarsDeclared = true;
	}
	if(bWarsDeclared
			|| bForceUpdateAttitude) { // advc
		// Cut and pasted from declareWar (K-Mod code)
		for(PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
			GET_PLAYER(i).AI_updateAttitudeCache();
	}
	bTriggeringWars = false;
} // </dlph.26>


int CvTeam::getRouteChange(RouteTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumRouteInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiRouteChange[eIndex];
}


void CvTeam::changeRouteChange(RouteTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumRouteInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiRouteChange[eIndex] = (m_paiRouteChange[eIndex] + iChange);
}


int CvTeam::getProjectCount(ProjectTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiProjectCount[eIndex];
}

int CvTeam::getProjectDefaultArtType(ProjectTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiProjectDefaultArtTypes[eIndex];
}

void CvTeam::setProjectDefaultArtType(ProjectTypes eIndex, int value)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiProjectDefaultArtTypes[eIndex] = value;
}

int CvTeam::getProjectArtType(ProjectTypes eIndex, int number) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(number >= 0, "number is expected to be non-negative (invalid Index)");
	FAssertMsg(number < getProjectCount(eIndex), "number is expected to be within maximum bounds (invalid Index)");
	return m_pavProjectArtTypes[eIndex][number];
}

void CvTeam::setProjectArtType(ProjectTypes eIndex, int number, int value)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(number >= 0, "number is expected to be non-negative (invalid Index)");
	FAssertMsg(number < getProjectCount(eIndex), "number is expected to be within maximum bounds (invalid Index)");
	m_pavProjectArtTypes[eIndex][number] = value;
}

bool CvTeam::isProjectMaxedOut(ProjectTypes eIndex, int iExtra) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (!isTeamProject(eIndex))
	{
		return false;
	}

	FAssertMsg(getProjectCount(eIndex) <= GC.getProjectInfo(eIndex).getMaxTeamInstances(), "Current Project count is expected to not exceed the maximum number of instances for this project");

	return ((getProjectCount(eIndex) + iExtra) >= GC.getProjectInfo(eIndex).getMaxTeamInstances());
}

bool CvTeam::isProjectAndArtMaxedOut(ProjectTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(getProjectCount(eIndex) >= GC.getProjectInfo(eIndex).getMaxTeamInstances())
	{
		int count = getProjectCount(eIndex);
		for(int i=0;i<count;i++)
		{
			if(getProjectArtType(eIndex, i) == -1) //undefined
				return false;
		}

		return true;
	}
	else
	{
		return false;
	}
}

void CvTeam::finalizeProjectArtTypes()
{
	//loop through each project and fill in default art values
	for(int i=0;i<GC.getNumProjectInfos();i++)
	{
		ProjectTypes eProject = (ProjectTypes) i;
		int projectCount = getProjectCount(eProject);
		for(int j=0;j<projectCount;j++)
		{
			int projectArtType = getProjectArtType(eProject, j);
			if(projectArtType == -1) //undefined
			{
				int defaultArtType = getProjectDefaultArtType(eProject);
				setProjectArtType(eProject, j, defaultArtType);
			}
		}
	}
}


void CvTeam::changeProjectCount(ProjectTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange == 0)
		return;

	GC.getGame().incrementProjectCreatedCount(eIndex, iChange);

	int iOldProjectCount = getProjectCount(eIndex);

	m_paiProjectCount[eIndex] = (m_paiProjectCount[eIndex] + iChange);
	FAssert(getProjectCount(eIndex) >= 0);

	//adjust default art types
	if(iChange >= 0)
	{
		int defaultType = -1;
		for(int i=0;i<iChange;i++)
			m_pavProjectArtTypes[eIndex].push_back(defaultType);
	}
	else
	{
		for(int i=0;i<-iChange;i++)
			m_pavProjectArtTypes[eIndex].pop_back();
	}
	FAssertMsg(getProjectCount(eIndex) == (int)m_pavProjectArtTypes[eIndex].size(), "[Jason] Unbalanced project art types.");

	CvProjectInfo& kProject = GC.getProjectInfo(eIndex);

	changeNukeInterception(kProject.getNukeInterception() * iChange);

	if ((kProject.getTechShare() > 0) && (kProject.getTechShare() <= MAX_TEAMS))
	{
		changeTechShareCount((kProject.getTechShare() - 1), iChange);
	}

	for (int iVictory = 0; iVictory < GC.getNumVictoryInfos(); ++iVictory)
	{
		if (kProject.getVictoryThreshold(iVictory) > 0)
		{
			setCanLaunch((VictoryTypes)iVictory, GC.getGame().testVictory((VictoryTypes)iVictory, getID()));
		}
	}

	if (iChange > 0)
	{
		if (kProject.getEveryoneSpecialUnit() != NO_SPECIALUNIT)
		{
			GC.getGame().makeSpecialUnitValid((SpecialUnitTypes)(kProject.getEveryoneSpecialUnit()));
		}

		if (kProject.getEveryoneSpecialBuilding() != NO_SPECIALBUILDING)
		{
			GC.getGame().makeSpecialBuildingValid((SpecialBuildingTypes)(kProject.getEveryoneSpecialBuilding()));
		}

		if (kProject.isAllowsNukes())
		{
			GC.getGame().makeNukesValid(true);
		}

		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
				{
					if (!(GET_PLAYER((PlayerTypes)iI).isHuman()))
					{
						bool bChangeProduction = false;

						for (int iJ = 0; iJ < GC.getNumProjectInfos(); iJ++)
						{
							if ((getProjectCount(eIndex) >= GC.getProjectInfo((ProjectTypes)iJ).getProjectsNeeded(eIndex)) &&
								(iOldProjectCount < GC.getProjectInfo((ProjectTypes)iJ).getProjectsNeeded(eIndex)))
							{
								bChangeProduction = true;
								break;
							}
						}

						if (bChangeProduction)
						{
							GET_PLAYER((PlayerTypes)iI).AI_makeProductionDirty();
						}
					}
				}
			}
		}

		if (GC.getGame().isFinalInitialized() && !(gDLL->GetWorldBuilderMode()))
		{
			CvWString szBuffer = gDLL->getText( // <advc.008e>
					::needsArticle(eIndex) ?
					"TXT_KEY_MISC_COMPLETES_PROJECT_THE" :
					"TXT_KEY_MISC_COMPLETES_PROJECT", // </advc.008e>
				getReplayName().GetCString(), kProject.getTextKeyWide());
			GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(), szBuffer, -1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));

			for (int iI = 0; iI < MAX_PLAYERS; iI++)
			{
				CvPlayer const& kObs = GET_PLAYER((PlayerTypes)iI);
				if(!kObs.isAlive())
					continue; // advc
				szBuffer = gDLL->getText( // <advc.008e>
						::needsArticle(eIndex) ?
						"TXT_KEY_MISC_SOMEONE_HAS_COMPLETED_THE" :
						"TXT_KEY_MISC_SOMEONE_HAS_COMPLETED", // </advc.008e>
						getName().GetCString(), kProject.getTextKeyWide());
				gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(),
						false, GC.getEVENT_MESSAGE_TIME(), szBuffer,
						"AS2D_PROJECT_COMPLETED", MESSAGE_TYPE_MAJOR_EVENT, NULL,
						(ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
						// <advc.127b>
						getCapitalX(kObs.getTeam(), true),
						getCapitalY(kObs.getTeam(), true)); // <advc.127b>
			}
		}
	}
}


int CvTeam::getProjectMaking(ProjectTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiProjectMaking[eIndex];
}


void CvTeam::changeProjectMaking(ProjectTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumProjectInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiProjectMaking[eIndex] = (m_paiProjectMaking[eIndex] + iChange);
	FAssert(getProjectMaking(eIndex) >= 0);
}


int CvTeam::getUnitClassCount(UnitClassTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUnitClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiUnitClassCount[eIndex];
}


bool CvTeam::isUnitClassMaxedOut(UnitClassTypes eIndex, int iExtra) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUnitClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (!isTeamUnitClass(eIndex))
	{
		return false;
	}

	FAssertMsg(getUnitClassCount(eIndex) <= GC.getUnitClassInfo(eIndex).getMaxTeamInstances(), "The current unit class count is expected not to exceed the maximum number of instances allowed for this team");

	return ((getUnitClassCount(eIndex) + iExtra) >= GC.getUnitClassInfo(eIndex).getMaxTeamInstances());
}


void CvTeam::changeUnitClassCount(UnitClassTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumUnitClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiUnitClassCount[eIndex] = (m_paiUnitClassCount[eIndex] + iChange);
	FAssert(getUnitClassCount(eIndex) >= 0);
}


int CvTeam::getBuildingClassCount(BuildingClassTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiBuildingClassCount[eIndex];
}


bool CvTeam::isBuildingClassMaxedOut(BuildingClassTypes eIndex, int iExtra) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (!isTeamWonderClass(eIndex))
	{
		return false;
	}

	FAssertMsg(getBuildingClassCount(eIndex) <= GC.getBuildingClassInfo(eIndex).getMaxTeamInstances(), "The current building class count is expected not to exceed the maximum number of instances allowed for this team");

	return ((getBuildingClassCount(eIndex) + iExtra) >= GC.getBuildingClassInfo(eIndex).getMaxTeamInstances());
}


void CvTeam::changeBuildingClassCount(BuildingClassTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingClassInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_paiBuildingClassCount[eIndex] = (m_paiBuildingClassCount[eIndex] + iChange);
	FAssert(getBuildingClassCount(eIndex) >= 0);
}


int CvTeam::getObsoleteBuildingCount(BuildingTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiObsoleteBuildingCount[eIndex];
}


bool CvTeam::isObsoleteBuilding(BuildingTypes eIndex) const
{
	return (getObsoleteBuildingCount(eIndex) > 0);
}


void CvTeam::changeObsoleteBuildingCount(BuildingTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumBuildingInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if(iChange == 0)
		return;

	bool bOldObsoleteBuilding = isObsoleteBuilding(eIndex);

	m_paiObsoleteBuildingCount[eIndex] = (m_paiObsoleteBuildingCount[eIndex] + iChange);
	FAssert(getObsoleteBuildingCount(eIndex) >= 0);

	if (bOldObsoleteBuilding != isObsoleteBuilding(eIndex))
	{
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			CvPlayer const& kMember = GET_PLAYER((PlayerTypes)iI);
			if (!kMember.isAlive() || kMember.getTeam() != getID())
				continue;
			FOR_EACH_CITY_VAR(pLoopCity, kMember)
			{
				if (pLoopCity->getNumBuilding(eIndex) > 0)
				{
					pLoopCity->processBuilding(eIndex, isObsoleteBuilding(eIndex) ?
						-pLoopCity->getNumBuilding(eIndex) : pLoopCity->getNumBuilding(eIndex), true);
				}
			}
		}
	}
}


int CvTeam::getResearchProgress(TechTypes eIndex) const
{
	if (eIndex != NO_TECH)
	{
		return m_paiResearchProgress[eIndex];
	}
	else
	{
		return 0;
	}
}


void CvTeam::setResearchProgress(TechTypes eIndex, int iNewValue, PlayerTypes ePlayer)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumTechInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(ePlayer >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(ePlayer < MAX_PLAYERS, "ePlayer is expected to be within maximum bounds (invalid Index)");

	if(getResearchProgress(eIndex) == iNewValue)
		return;

	m_paiResearchProgress[eIndex] = iNewValue;
	FAssert(getResearchProgress(eIndex) >= 0);

	if (getID() == GC.getGame().getActiveTeam())
	{
		gDLL->getInterfaceIFace()->setDirty(GameData_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(Score_DIRTY_BIT, true);
		/*  <advc.004x> Update research-turns shown in popup (tbd.: perhaps setting
			Popup_DIRTY_BIT would suffice here?) */
		CvPlayer& kActivePlayer = GET_PLAYER(GC.getGame().getActivePlayer());
		if(kActivePlayer.getCurrentResearch() == NO_TECH &&
				kActivePlayer.isFoundedFirstCity() &&
				kActivePlayer.isHuman()) { // i.e. not during Auto Play
			kActivePlayer.killAll(BUTTONPOPUP_CHOOSETECH);
			kActivePlayer.chooseTech();
		} // </advc.004x>
	}

	if (getResearchProgress(eIndex) >= getResearchCost(eIndex))
	{
		int iOverflow = (100 * (getResearchProgress(eIndex) - getResearchCost(eIndex))) / std::max(1, GET_PLAYER(ePlayer).calculateResearchModifier(eIndex));
		GET_PLAYER(ePlayer).changeOverflowResearch(iOverflow);
		setHasTech(eIndex, true, ePlayer, true, true);
		/* original bts code
		if (!GC.getGame().isMPOption(MPOPTION_SIMULTANEOUS_TURNS) && !GC.getGame().isOption(GAMEOPTION_NO_TECH_BROKERING))
			setNoTradeTech(eIndex, true);*/
		// disabled by K-Mod. I don't know why this was here, and it conflicts with my changes to the order of the doTurn functions.
	}
}


void CvTeam::changeResearchProgress(TechTypes eIndex, int iChange, PlayerTypes ePlayer)
{
	setResearchProgress(eIndex, (getResearchProgress(eIndex) + iChange), ePlayer);
}

int CvTeam::changeResearchProgressPercent(TechTypes eIndex, int iPercent, PlayerTypes ePlayer)
{
	int iBeakers = 0;

	if (iPercent != 0 && !isHasTech(eIndex))
	{
		if (iPercent > 0)
		{
			iBeakers = std::min(getResearchLeft(eIndex), (getResearchCost(eIndex) * iPercent) / 100);
		}
		else
		{
			iBeakers = std::max(getResearchLeft(eIndex) - getResearchCost(eIndex), (getResearchCost(eIndex) * iPercent) / 100);
		}

		changeResearchProgress(eIndex, iBeakers, ePlayer);
	}

	return iBeakers;
}


int CvTeam::getTechCount(TechTypes eIndex)		 const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumTechInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiTechCount[eIndex];
}

// BETTER_BTS_AI_MOD, General AI, 07/27/09, jdog5000: START
int CvTeam::getBestKnownTechScorePercent() const
{
	int iOurTechScore = 0;
	int iBestKnownTechScore = 0;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iOurTechScore = std::max(iOurTechScore, GET_PLAYER((PlayerTypes)iI).getTechScore());
			}
			else if (isHasMet(GET_PLAYER((PlayerTypes)iI).getTeam()))
			{
				iBestKnownTechScore = std::max(iBestKnownTechScore, GET_PLAYER((PlayerTypes)iI).getTechScore());
			}
		}
	}

	iBestKnownTechScore = std::max(iBestKnownTechScore, iOurTechScore);

	return ((100*iOurTechScore)/std::max(iBestKnownTechScore, 1));
} // BETTER_BTS_AI_MOD: END


int CvTeam::getTerrainTradeCount(TerrainTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumTerrainInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_paiTerrainTradeCount[eIndex];
}


bool CvTeam::isTerrainTrade(TerrainTypes eIndex) const
{
	return (getTerrainTradeCount(eIndex) > 0);
}


void CvTeam::changeTerrainTradeCount(TerrainTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumTerrainInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_paiTerrainTradeCount[eIndex] = (m_paiTerrainTradeCount[eIndex] + iChange);
		FAssert(getTerrainTradeCount(eIndex) >= 0);

		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
				{
					GET_PLAYER((PlayerTypes)iI).updatePlotGroups();
				}
			}
		}
	}
}


int CvTeam::getRiverTradeCount() const
{
	return m_iRiverTradeCount;
}


bool CvTeam::isRiverTrade() const
{
	//return (getRiverTradeCount() > 0);
	return true; // advc.124
}


void CvTeam::changeRiverTradeCount(int iChange)
{
	if (iChange != 0)
	{
		m_iRiverTradeCount += iChange;
		FAssert(getRiverTradeCount() >= 0);

		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
				{
					GET_PLAYER((PlayerTypes)iI).updatePlotGroups();
				}
			}
		}
	}
}


int CvTeam::getVictoryCountdown(VictoryTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumVictoryInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiVictoryCountdown[eIndex];
}

void CvTeam::setVictoryCountdown(VictoryTypes eIndex, int iTurnsLeft)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumVictoryInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiVictoryCountdown[eIndex] = iTurnsLeft;
	// <advc.opt>
	if (iTurnsLeft >= 0)
		m_bAnyVictoryCountdown = true;
	else
	{
		m_bAnyVictoryCountdown = false;
		for (int i = 0; i < GC.getNumVictoryInfos(); i++)
		{
			if (i != eIndex && getVictoryCountdown((VictoryTypes)i) >= 0)
				m_bAnyVictoryCountdown = true;
		}
	} // </advc.opt>
}


void CvTeam::changeVictoryCountdown(VictoryTypes eIndex, int iChange)
{
	setVictoryCountdown(eIndex, getVictoryCountdown(eIndex) + iChange); // advc: was m_aiVictoryCountdown[eIndex] += iChange
}

// <advc.opt>
bool CvTeam::isAnyVictoryCountdown() const
{
	return m_bAnyVictoryCountdown;
} // </advc.opt>


int CvTeam::getVictoryDelay(VictoryTypes eVictory) const
{
	int iExtraDelayPercent = 0;
	for (int iProject = 0; iProject < GC.getNumProjectInfos(); ++iProject)
	{
		CvProjectInfo& kProject = GC.getProjectInfo((ProjectTypes)iProject);
		int iCount = getProjectCount((ProjectTypes)iProject);

		if (iCount < kProject.getVictoryMinThreshold(eVictory))
		{
			FAssert(false);
			return -1;
		}

		if (iCount < kProject.getVictoryThreshold(eVictory))
		{
			iExtraDelayPercent += ((kProject.getVictoryThreshold(eVictory)  - iCount) * kProject.getVictoryDelayPercent()) / kProject.getVictoryThreshold(eVictory);
		}
	}

	return (GC.getGame().victoryDelay(eVictory)  * (100 + iExtraDelayPercent)) / 100;
}

void CvTeam::setCanLaunch(VictoryTypes eVictory, bool bCan)
{
	m_abCanLaunch[eVictory] = bCan;
}

bool CvTeam::canLaunch(VictoryTypes eVictory) const
{
	return m_abCanLaunch[eVictory];
}

int CvTeam::getLaunchSuccessRate(VictoryTypes eVictory) const
{
	int iSuccessRate = 100;
	for (int iProject = 0; iProject < GC.getNumProjectInfos(); ++iProject)
	{
		CvProjectInfo& kProject = GC.getProjectInfo((ProjectTypes)iProject);
		int iCount = getProjectCount((ProjectTypes)iProject);

		if (iCount < kProject.getVictoryMinThreshold(eVictory))
		{
			return 0;
		}

		if (iCount < kProject.getVictoryThreshold(eVictory))
		{
			if (kProject.getSuccessRate() > 0)
			{
				iSuccessRate -= (kProject.getSuccessRate() * (kProject.getVictoryThreshold(eVictory) - iCount));
			}
		}
	}

	return iSuccessRate;
}

void CvTeam::resetVictoryProgress()
{	// <advc.opt>
	if (!isAnyVictoryCountdown() || GC.getGame().getGameState() != GAMESTATE_ON)
		return; // </advc.opt>
	for (int iI = 0; iI < GC.getNumVictoryInfos(); ++iI)
	{
		if (getVictoryCountdown((VictoryTypes)iI) < 0)
			continue;

		setVictoryCountdown((VictoryTypes)iI, -1);

		for (int iK = 0; iK < GC.getNumProjectInfos(); iK++)
		{
			if (GC.getProjectInfo((ProjectTypes)iK).getVictoryMinThreshold((VictoryTypes)iI) > 0)
			{
				changeProjectCount((ProjectTypes)iK, -getProjectCount((ProjectTypes)iK));
			}
		}

		CvWString szBuffer = gDLL->getText("TXT_KEY_VICTORY_RESET", getReplayName().GetCString(), GC.getVictoryInfo((VictoryTypes)iI).getTextKeyWide());

		for (int iJ = 0; iJ < MAX_PLAYERS; ++iJ)
		{
			CvPlayer& kObs = GET_PLAYER((PlayerTypes)iJ);
			if(!kObs.isAlive())
				continue; // advc
			gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(),
					false, GC.getEVENT_MESSAGE_TIME(), szBuffer,
					"AS2D_MELTDOWN", MESSAGE_TYPE_MAJOR_EVENT,
					// <advc.127b>
					NULL, NO_COLOR, getCapitalX(kObs.getTeam(), true),
					getCapitalY(kObs.getTeam(), true)); // </advc.127b>
			if(kObs.getTeam() == getID())
			{
				CvPopupInfo* pInfo = new CvPopupInfo();
				pInfo->setText(szBuffer);
				gDLL->getInterfaceIFace()->addPopup(pInfo, (PlayerTypes) iJ);
			}
		}

		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(), szBuffer, -1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
	}
}

// K-Mod, code moved from CvPlayer::hasSpaceshipArrived. (it makes more sense to be here)
bool CvTeam::hasSpaceshipArrived() const
{	// <advc.opt>
	if (!isAnyVictoryCountdown())
		return false; // </advc.opt>
	VictoryTypes eSpaceVictory = GC.getGame().getSpaceVictory();
	if (eSpaceVictory != NO_VICTORY)
	{
		int iVictoryCountdown = getVictoryCountdown(eSpaceVictory);
		if (iVictoryCountdown == 0 || (iVictoryCountdown > 0 && GC.getGame().getGameState() == GAMESTATE_EXTENDED))
		{
			return true;
		}
	}

	return false;
}
// K-Mod end

bool CvTeam::isParent(TeamTypes eTeam) const
{
	FAssert(eTeam != NO_TEAM);

	if (GET_TEAM(eTeam).isVassal(getID()))
	{
		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
			if (kLoopPlayer.isAlive() && kLoopPlayer.getTeam() == eTeam)
			{
				if (kLoopPlayer.getParent() != NO_PLAYER)
				{
					if (GET_PLAYER(kLoopPlayer.getParent()).getTeam() == getID())
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool CvTeam::isHasTech(TechTypes eIndex) const
{
	if (eIndex == NO_TECH)
	{
		return true;
	}

	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumTechInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(m_pabHasTech != NULL, "m_pabHasTech is not expected to be equal with NULL");
	return m_pabHasTech[eIndex];
}

// <advc.039>
CvWString const CvTeam::tradeItemString(TradeableItems eItem, int iData,
			TeamTypes eFrom) const {

	CvTeam const& kFrom = GET_TEAM(eFrom);
	switch(eItem) {
	case TRADE_CITIES: {
		CvCity* c = GET_PLAYER(kFrom.getLeaderID()).getCity(iData);
		if(c == NULL)
			return L"";
		return c->getName();
	}
	case TRADE_CIVIC: {
		CivicTypes eCivic = (CivicTypes)iData;
		if(eCivic == NO_CIVIC)
			return L"";
		return gDLL->getText("TXT_KEY_MISC_ADOPTING_CIVIC",
				GC.getCivicInfo(eCivic).getTextKeyWide());
	}
	case TRADE_RELIGION: {
		ReligionTypes eReligion = (ReligionTypes)iData;
		if(eReligion == NO_RELIGION)
			return L"";
		return gDLL->getText("TXT_KEY_MISC_CONVERTING_RELIGION",
				GC.getReligionInfo(eReligion).getTextKeyWide());
	}
	case TRADE_EMBARGO: {
		TeamTypes eTarget = (TeamTypes)iData;
		if(eTarget == NO_TEAM)
			return L"";
		return gDLL->getText("TXT_KEY_MISC_EMBARGO_AGAINST",
				GET_TEAM(eTarget).getName().GetCString());
	}
	case TRADE_GOLD: {
		if(iData <= 0)
			return L"";
		return gDLL->getText("TXT_KEY_MISC_CASH", iData);
	}
	case TRADE_GOLD_PER_TURN: {
		if(iData <= 0)
			return L"";
		return gDLL->getText("TXT_KEY_MISC_GPT", iData);
	}
	case TRADE_MAPS: {
		return gDLL->getText("TXT_KEY_MISC_MAPS_CIV", kFrom.getName().GetCString());
	}
	case TRADE_TECHNOLOGIES: {
		TechTypes eTech = (TechTypes)iData;
		if(eTech == NO_TECH)
			return L"";
		return CvWString::format(SETCOLR L"%s" ENDCOLR,
				TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"),
				GC.getTechInfo(eTech).getDescription());
	}
	case TRADE_PEACE: {
		TeamTypes ePeaceTeam = (TeamTypes)iData;
		if(ePeaceTeam == NO_TEAM)
			return L"";
		return gDLL->getText("TXT_KEY_TRADE_PEACE_WITH") +
				GET_TEAM(ePeaceTeam).getName();
	}
	}
	return L"";
} // </advc.039>

void CvTeam::announceTechToPlayers(TechTypes eIndex,
		PlayerTypes eDiscoverPlayer, // advc.156
		bool bPartial)
{
	CvGame const& g = GC.getGame();
	bool bSound = ((g.isNetworkMultiPlayer() ||
			/*  advc.156: I think Hot Seat doesn't play sounds along with messages,
				but let's try. */
			g.isHotSeat() ||
			gDLL->getInterfaceIFace()->noTechSplash()) && !bPartial);
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer const& kPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kPlayer.isAlive() && kPlayer.getTeam() == getID()) // advc
		{	// <advc.156>
			TCHAR const* szSound = NULL;
			if(bSound) {
				if(kPlayer.getID() == eDiscoverPlayer)
					szSound = GC.getTechInfo(eIndex).getSound();
				else szSound = GC.getTechInfo(eIndex).getSoundMP();
			} // </advc.156>
			CvWString szBuffer = gDLL->getText((bPartial ?
					"TXT_KEY_MISC_PROGRESS_TOWARDS_TECH" :
					"TXT_KEY_MISC_YOU_DISCOVERED_TECH"),
					GC.getTechInfo(eIndex).getTextKeyWide());
			gDLL->getInterfaceIFace()->addHumanMessage((PlayerTypes)iI, false,
					bSound ? GC.getEVENT_MESSAGE_TIME() : -1, szBuffer,
					szSound, // advc.156
					MESSAGE_TYPE_MINOR_EVENT, // advc.106b
					NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_TECH_TEXT"));
			// K-Mod. Play the quote sound always, the "MP" sound is boring.
					//(bSound ? GC.getTechInfo(eIndex).getSound() : NULL)
		}
	}
}

void CvTeam::setHasTech(TechTypes eIndex, bool bNewValue, PlayerTypes ePlayer, bool bFirst, bool bAnnounce)
{
	PROFILE_FUNC();

	if (eIndex == NO_TECH)
		return;

	if (isHasTech(eIndex) == bNewValue)
		return;

	if (ePlayer == NO_PLAYER)
		ePlayer = getLeaderID();

	FAssert(eIndex >= 0);
	FAssert(eIndex < GC.getNumTechInfos());
	FAssert(ePlayer >= 0);
	FAssert(ePlayer < MAX_PLAYERS);

	CvGame& g = GC.getGame();

	if (GC.getTechInfo(eIndex).isRepeat())
	{
		m_paiTechCount[eIndex]++;
		setResearchProgress(eIndex, 0, ePlayer);
		CvEventReporter::getInstance().techAcquired(eIndex, getID(), ePlayer, bAnnounce && 1 == m_paiTechCount[eIndex]);

		if (m_paiTechCount[eIndex] == 1 && bAnnounce &&
				g.isFinalInitialized() && !gDLL->GetWorldBuilderMode())
			announceTechToPlayers(eIndex, /* advc.156: */ ePlayer);
	}
	else
	{
		updatePlotGroupBonus(eIndex, false); // advc: Code moved into auxiliary function
		m_pabHasTech[eIndex] = bNewValue;
		updatePlotGroupBonus(eIndex, true);
	}

	processTech(eIndex, ((bNewValue) ? 1 : -1));

	if (isHasTech(eIndex))
	{
		if (gTeamLogLevel >= 2) // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000: START
			logBBAI("    Team %d (%S) acquires tech %S", getID(), getName().GetCString(), GC.getTechInfo(eIndex).getDescription());

		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (GET_PLAYER((PlayerTypes)iI).getCurrentEra() < GC.getTechInfo(eIndex).getEra())
				{
					GET_PLAYER((PlayerTypes)iI).setCurrentEra((EraTypes)(GC.getTechInfo(eIndex).getEra()));
				}
			}
		}

		if (GC.getTechInfo(eIndex).isMapVisible())
		{
			GC.getMap().setRevealedPlots(getID(), true, true);
		}

		for (int iI = 0; iI < GC.getNumSpecialBuildingInfos(); ++iI)
		{
			if (eIndex == GC.getSpecialBuildingInfo((SpecialBuildingTypes)iI).getTechPrereqAnyone())
			{
				g.makeSpecialBuildingValid((SpecialBuildingTypes)iI, bAnnounce);
			}
		}

		// report event to Python, along with some other key state
		CvEventReporter::getInstance().techAcquired(eIndex, getID(), ePlayer, bAnnounce);

		bool bReligionFounded = false;
		bool bFirstBonus = false;
		bool firstToDiscover = (g.countKnownTechNumTeams(eIndex) == 1); // advc.106
		if (bFirst && firstToDiscover &&
			!GC.getPythonCaller()->doOrganizationTech(getID(), ePlayer, eIndex))
		{	// advc: style changes in this block
			for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
			{
				ReligionTypes eReligion = (ReligionTypes)iI;
				if (GC.getReligionInfo(eReligion).getTechPrereq() != eIndex)
					continue;

				int iBestValue = MAX_INT;
				PlayerTypes eBestPlayer = NO_PLAYER;
				for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					CvPlayer const& kMember = GET_PLAYER((PlayerTypes)iJ);
					if (!kMember.isAlive() || kMember.getTeam() != getID())
						continue;

					int iValue = 10;
					iValue += g.getSorenRandNum(10, "Found Religion (Player)");
					for (int iK = 0; iK < GC.getNumReligionInfos(); iK++)
						iValue += kMember.getHasReligionCount((ReligionTypes)iK) * 10;

					if (kMember.getCurrentResearch() != eIndex)
						iValue *= 10;

					if (iValue < iBestValue)
					{
						iBestValue = iValue;
						eBestPlayer = kMember.getID();
					}
				}
				if (eBestPlayer == NO_PLAYER)
					continue;

				g.setReligionSlotTaken(eReligion, true);
				if (g.isOption(GAMEOPTION_PICK_RELIGION))
				{
					if (GET_PLAYER(eBestPlayer).isHuman())
					{
						CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_FOUND_RELIGION, eReligion);
						if (pInfo != NULL)
							gDLL->getInterfaceIFace()->addPopup(pInfo, eBestPlayer);
					}
					else
					{
						ReligionTypes eFoundReligion = GET_PLAYER(eBestPlayer).AI_chooseReligion();
						if (eFoundReligion != NO_RELIGION)
							GET_PLAYER(eBestPlayer).foundReligion(eFoundReligion, eReligion, true);
					}
				}
				else GET_PLAYER(eBestPlayer).foundReligion(eReligion, eReligion, true);
				bReligionFounded = true;
				bFirstBonus = true;
			}
			for (int iI = 0; iI < GC.getNumCorporationInfos(); iI++)
			{
				CorporationTypes eCorp = (CorporationTypes)iI;
				if (GC.getCorporationInfo(eCorp).getTechPrereq() != eIndex ||
						g.isCorporationFounded(eCorp))
					continue;
				/*  advc (comment): From here on unused and thus not properly tested;
					see comment in CvGame::doHeadquarters. */
				int iBestValue = MAX_INT;
				PlayerTypes eBestPlayer = NO_PLAYER;
				for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					CvPlayer const& kMember = GET_PLAYER((PlayerTypes)iJ);
					if (!kMember.isAlive() || kMember.getTeam() != getID())
						continue;

					int iValue = 10;
					iValue += g.getSorenRandNum(10, "Found Corporation (Player)");
					if (kMember.getCurrentResearch() != eIndex)
						iValue *= 10;
					if (iValue < iBestValue)
					{
						iBestValue = iValue;
						eBestPlayer = kMember.getID();
					}
				}
				if (eBestPlayer == NO_PLAYER)
					continue;

				GET_PLAYER(eBestPlayer).foundCorporation(eCorp);
				bFirstBonus = true;
			}
		}

		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (GET_PLAYER((PlayerTypes)iI).isResearchingTech(eIndex))
				{
					GET_PLAYER((PlayerTypes)iI).popResearch(eIndex);
				}

				// notify the player they now have the tech, if they want to make immediate changes
				GET_PLAYER((PlayerTypes)iI).AI_nowHasTech(eIndex);

				GET_PLAYER((PlayerTypes)iI).invalidateYieldRankCache();
			}
		}

		if (bFirst && firstToDiscover)
		{
			bool bAnnounceFirst = false; // advc.004
			CvWString szBuffer;
			UnitTypes eFreeUnit = GET_PLAYER(ePlayer).getTechFreeUnit(eIndex);
			if (eFreeUnit != NO_UNIT)
			{
				bFirstBonus = true;
				bAnnounceFirst = true; // advc.004
				CvCity* pCapitalCity = GET_PLAYER(ePlayer).getCapitalCity();

				if (pCapitalCity != NULL)
				{
					pCapitalCity->createGreatPeople(eFreeUnit, false, false);
				}
			}

			if (GC.getTechInfo(eIndex).getFirstFreeTechs() > 0)
			{
				bFirstBonus = true;
				bAnnounceFirst = true; // advc.004

				if (!isHuman())
				{
					for (int iI = 0; iI < GC.getTechInfo(eIndex).getFirstFreeTechs(); iI++)
					{
						GET_PLAYER(ePlayer).AI_chooseFreeTech();
					}
				}
				else
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_FIRST_TECH_CHOOSE_FREE", GC.getTechInfo(eIndex).getTextKeyWide());
					GET_PLAYER(ePlayer).chooseTech(GC.getTechInfo(eIndex).getFirstFreeTechs(), szBuffer.GetCString());
				}
				// advc.004: Announcement code moved into next block
				// advc.106: Do it at the end instead
				if(GC.getDefineINT("SHOW_FIRST_TO_DISCOVER_IN_REPLAY") <= 0) {
					szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_FIRST_TO_TECH", GET_PLAYER(ePlayer).getReplayName(), GC.getTechInfo(eIndex).getTextKeyWide());
					g.addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, ePlayer, szBuffer, -1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
				} // advc.106
			} // <advc.004>
			if(bAnnounceFirst) { // Cut, pasted, refactored from above
				// Free GP only minor event
				bool bMajor = (GC.getTechInfo(eIndex).getFirstFreeTechs() > 0);
				for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
					CvPlayer const& kObs = GET_PLAYER((PlayerTypes)i);
					if(!kObs.isAlive() || kObs.getTeam() == TEAMID(ePlayer))
						continue;
					if(isHasMet(kObs.getTeam()) ||
							kObs.isSpectator()) { // advc.127
						szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_FIRST_TO_TECH",
							GET_PLAYER(ePlayer).getNameKey(),
							GC.getTechInfo(eIndex).getTextKeyWide());
					}
					else szBuffer = gDLL->getText("TXT_KEY_MISC_UNKNOWN_FIRST_TO_TECH",
							GC.getTechInfo(eIndex).getTextKeyWide());
					gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(),
							false, GC.getEVENT_MESSAGE_TIME(), szBuffer,
							(bMajor ? "AS2D_FIRSTTOTECH" : 0),
							(bMajor ? MESSAGE_TYPE_MAJOR_EVENT :
							MESSAGE_TYPE_MINOR_EVENT), NULL, (ColorTypes)
							GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
							// <advc.127b>
							getCapitalX(kObs.getTeam(), true),
							getCapitalY(kObs.getTeam(), true)); // </advc.127b>
				}
			} // </advc.004>
			if (bFirstBonus)
			{
				for (int iI = 0; iI < MAX_PLAYERS; iI++)
				{
					if (GET_PLAYER((PlayerTypes)iI).isAlive())
					{
						if (!(GET_PLAYER((PlayerTypes)iI).isHuman()))
						{
							if (GET_PLAYER((PlayerTypes)iI).isResearchingTech(eIndex))
							{
								GET_PLAYER((PlayerTypes)iI).clearResearchQueue(); // K-Mod note: we just want to flag it for re-evaluation. Clearing the queue is currently the only way to do that.
							}
						}
					}
				}
			}
		}


		if (bAnnounce && g.isFinalInitialized() &&
				!gDLL->GetWorldBuilderMode()) // advc
		{
			announceTechToPlayers(eIndex, /* advc.156: */ ePlayer);
			bool bMessageSent = false; // advc.004r
			for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
			{
				CvPlot const& kLoopPlot = *GC.getMap().plotByIndex(iI);
				// <advc.004r>
				TeamTypes eRevealedTeam = kLoopPlot.getRevealedTeam(getID(), false);
				if((eRevealedTeam != getID() && eRevealedTeam != NO_TEAM &&
						eRevealedTeam != BARBARIAN_TEAM &&
						!GET_TEAM(eRevealedTeam).isVassal(getID())) ||
						!kLoopPlot.isRevealed(getID(), false)) // </advc.004r>
					continue; // advc
				BonusTypes eBonus = kLoopPlot.getBonusType();
				if (eBonus == NO_BONUS)
					continue;
				if (GC.getBonusInfo(eBonus).getTechReveal() != eIndex ||
						isForceRevealedBonus(eBonus))
					continue;
				CvCity* pCity = GC.getMap().findCity(kLoopPlot.getX(), kLoopPlot.getY(), NO_PLAYER,
						// advc.004r: Pass ID as 'observer' (last param) instead of city owner
						NO_TEAM, false, false, NO_TEAM, NO_DIRECTION, NULL, getID());
				if (pCity == NULL)
					continue;
				CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_DISCOVERED_BONUS", GC.getBonusInfo(eBonus).getTextKeyWide(), pCity->getNameKey());
				/*  <advc.004r> Announce to all team members (instead of
					plot owner, which may not even be alive) */
				for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
					CvPlayer& kMember = GET_PLAYER((PlayerTypes)i);
					if(!kMember.isAlive() || kMember.getTeam() != getID())
						continue;
					bMessageSent = true;
					gDLL->getInterfaceIFace()->addHumanMessage(kMember.getID(),
					// </advc.004r>
							false, GC.getEVENT_MESSAGE_TIME(), szBuffer,
							"AS2D_DISCOVERBONUS", MESSAGE_TYPE_INFO,
							GC.getBonusInfo(eBonus).getButton(), (ColorTypes)
							GC.getInfoTypeForString("COLOR_WHITE"),
							kLoopPlot.getX(), kLoopPlot.getY(), true, true);
				}
			}
			// <advc.004r> Report no sources
			if(!bMessageSent && !isBarbarian() && !isMinorCiv()) {
				BonusTypes eBonus = NO_BONUS;
				for(int i = 0; i < GC.getNumBonusInfos() && eBonus == NO_BONUS; i++) {
					BonusTypes b = (BonusTypes)i;
					if(GC.getBonusInfo(b).getTechReveal() == eIndex)
						eBonus = b;
				}
				if(eBonus != NO_BONUS) {
					for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
						CvWString szBuffer = gDLL->getText(
								"TXT_KEY_MISC_DISCOVERED_NO_BONUS",
								GC.getBonusInfo(eBonus).getTextKeyWide());
						CvPlayer& kMember = GET_PLAYER((PlayerTypes)i);
						if(!kMember.isAlive() || kMember.getTeam() != getID())
							continue;
						gDLL->getInterfaceIFace()->addHumanMessage(kMember.getID(),
								false, GC.getEVENT_MESSAGE_TIME(), szBuffer
								// Don't play the sound
								/*,"AS2D_DISCOVERBONUS"*/);
					}
				}
			} // </advc.004r>
		}
		/*  advc.004x: Don't check bAnnounce for civics popup. FinalInitialized:
			Let CvPlayer::doChangeCivicsPopup handle that. */
		if(!gDLL->GetWorldBuilderMode() && getID() == g.getActiveTeam())
		{
			for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
			{	// advc: Un-nested the conditions
				CvPlayer& kMember = GET_PLAYER((PlayerTypes)iI);
				if (kMember.isAlive() && kMember.getTeam() == getID() && kMember.isHuman() &&
						(kMember.getID() != ePlayer || !bReligionFounded ||
						kMember.getLastStateReligion() != NO_RELIGION) /*&&
						kMember.canRevolution(NULL)*/) { // advc.004x
					CivicTypes eCivic = NO_CIVIC;
					for (int iJ = 0; iJ < GC.getNumCivicOptionInfos(); iJ++)
					{
						if (kMember.isHasCivicOption((CivicOptionTypes)iJ))
							continue;
						for (int iK = 0; iK < GC.getNumCivicInfos(); iK++)
						{
							CivicTypes eLoopCivic = (CivicTypes)iK;
							if (GC.getCivicInfo(eLoopCivic).getCivicOptionType() != iJ)
								continue;
							if (GC.getCivicInfo(eLoopCivic).getTechPrereq() == eIndex)
								eCivic = eLoopCivic;
						}
					} // <advc.004x>
					if(eCivic != NO_CIVIC && kMember.canDoCivics(eCivic)) {
						// BtS code moved into subroutine
						kMember.doChangeCivicsPopup(eCivic);
					} // </advc.004x>
				}
			}
		}
		for (int iI = 0; iI < MAX_TEAMS; iI++)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (iI != getID())
				{
					GET_TEAM((TeamTypes)iI).updateTechShare(eIndex);
				}
			}
		}
		// <advc.106>
		if(bFirst && firstToDiscover && g.getElapsedGameTurns() > 0 &&
				GC.getDefineINT("SHOW_FIRST_TO_DISCOVER_IN_REPLAY") > 0) {
			CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_FIRST_TO_TECH",
					GET_PLAYER(ePlayer).getReplayName(),
					GC.getTechInfo(eIndex).getTextKeyWide());
			g.addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, ePlayer, szBuffer,
					-1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_ALT_HIGHLIGHT_TEXT"));
		} // </advc.106>
	}

	if (bNewValue)
	{
		if (bAnnounce)
		{
			if (g.isFinalInitialized() && !(gDLL->GetWorldBuilderMode()))
			{
				FAssert(ePlayer != NO_PLAYER);
				//if (GET_PLAYER(ePlayer).isResearch() && (GET_PLAYER(ePlayer).getCurrentResearch() == NO_TECH))
				if (GET_PLAYER(ePlayer).isResearch() && GET_PLAYER(ePlayer).getCurrentResearch() == NO_TECH && GET_PLAYER(ePlayer).isHuman()) // K-Mod
				{
					CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_WHAT_TO_RESEARCH_NEXT");
					GET_PLAYER(ePlayer).chooseTech(0, szBuffer);
				}
			}
		}
	}

	if (getID() == g.getActiveTeam())
	{
		gDLL->getInterfaceIFace()->setDirty(MiscButtons_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(ResearchButtons_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(GlobeLayer_DIRTY_BIT, true);
	}
}


bool CvTeam::isNoTradeTech(TechTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumTechInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_pabNoTradeTech[eIndex];
}


void CvTeam::setNoTradeTech(TechTypes eIndex, bool bNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumTechInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	m_pabNoTradeTech[eIndex] = bNewValue;
}


int CvTeam::getImprovementYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2) const
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex1 < GC.getNumImprovementInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");
	return m_ppaaiImprovementYieldChange[eIndex1][eIndex2];
}


void CvTeam::changeImprovementYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2, int iChange)
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex1 < GC.getNumImprovementInfos(), "eIndex1 is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex2 < NUM_YIELD_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_ppaaiImprovementYieldChange[eIndex1][eIndex2] = (m_ppaaiImprovementYieldChange[eIndex1][eIndex2] + iChange);
		FAssert(getImprovementYieldChange(eIndex1, eIndex2) >= 0);

		updateYield();
	}
}

// K-Mod. In the original code, there seems to be a lot of confusion about what the exact conditions are for a bonus being connected.
// There were heaps of bugs where CvImprovementInfo::isImprovementBonusTrade was mistakenly used as the sole condition for a bonus being connected or not.
// I created this function to make the situation a bit more clear...
bool CvTeam::doesImprovementConnectBonus(ImprovementTypes eImprovement, BonusTypes eBonus) const
{
	FAssert(eImprovement < GC.getNumImprovementInfos());
	FAssert(eBonus < GC.getNumBonusInfos());

	if (eImprovement == NO_IMPROVEMENT || eBonus == NO_BONUS)
		return false;

	const CvImprovementInfo& kImprovementInfo = GC.getImprovementInfo(eImprovement);
	const CvBonusInfo& kBonusInfo = GC.getBonusInfo(eBonus);

	if (!isHasTech((TechTypes)kBonusInfo.getTechCityTrade()) || (kBonusInfo.getTechObsolete() != NO_TECH && isHasTech((TechTypes)kBonusInfo.getTechObsolete())))
		return false;

	return kImprovementInfo.isImprovementBonusTrade(eBonus) || kImprovementInfo.isActsAsCity();
}
// K-Mod end

bool CvTeam::isFriendlyTerritory(TeamTypes eTeam) const
{
	if (eTeam == NO_TEAM)
	{
		return false;
	}

	if (eTeam == getID())
	{
		return true;
	}

	if (GET_TEAM(eTeam).isVassal(getID()))
	{
		return true;
	}

	if (isVassal(eTeam) && isOpenBorders(eTeam))
	{
		return true;
	}

	return false;
}

int CvTeam::getEspionagePointsAgainstTeam(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiEspionagePointsAgainstTeam[eIndex];
}

void CvTeam::setEspionagePointsAgainstTeam(TeamTypes eIndex, int iValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iValue != getEspionagePointsAgainstTeam(eIndex))
	{
		m_aiEspionagePointsAgainstTeam[eIndex] = iValue;

		verifySpyUnitsValidPlot();
		GET_TEAM(eIndex).verifySpyUnitsValidPlot();
	}
}

void CvTeam::changeEspionagePointsAgainstTeam(TeamTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");

	setEspionagePointsAgainstTeam(eIndex, getEspionagePointsAgainstTeam(eIndex) + iChange);
}

// K-Mod
int CvTeam::getTotalUnspentEspionage() const
{
	int iTotal = 0;
	for (int i = 0; i < MAX_CIV_TEAMS; i++)
	{
		iTotal += getEspionagePointsAgainstTeam((TeamTypes)i);
	}
	return iTotal;
}
// K-Mod end

int CvTeam::getEspionagePointsEver() const
{
	return m_iEspionagePointsEver;
}

void CvTeam::setEspionagePointsEver(int iValue)
{
	if (iValue != getEspionagePointsEver())
	{
		m_iEspionagePointsEver = iValue;
	}
}

void CvTeam::changeEspionagePointsEver(int iChange)
{
	setEspionagePointsEver(getEspionagePointsEver() + iChange);
}

int CvTeam::getCounterespionageTurnsLeftAgainstTeam(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiCounterespionageTurnsLeftAgainstTeam[eIndex];
}

void CvTeam::setCounterespionageTurnsLeftAgainstTeam(TeamTypes eIndex, int iValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iValue != getCounterespionageTurnsLeftAgainstTeam(eIndex))
	{
		m_aiCounterespionageTurnsLeftAgainstTeam[eIndex] = iValue;

		gDLL->getInterfaceIFace()->setDirty(Espionage_Advisor_DIRTY_BIT, true);
	}
}

void CvTeam::changeCounterespionageTurnsLeftAgainstTeam(TeamTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");

	setCounterespionageTurnsLeftAgainstTeam(eIndex, getCounterespionageTurnsLeftAgainstTeam(eIndex) + iChange);
}

int CvTeam::getCounterespionageModAgainstTeam(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiCounterespionageModAgainstTeam[eIndex];
}

void CvTeam::setCounterespionageModAgainstTeam(TeamTypes eIndex, int iValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iValue != getCounterespionageModAgainstTeam(eIndex))
	{
		m_aiCounterespionageModAgainstTeam[eIndex] = iValue;

		gDLL->getInterfaceIFace()->setDirty(Espionage_Advisor_DIRTY_BIT, true);
	}
}

void CvTeam::changeCounterespionageModAgainstTeam(TeamTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");

	setCounterespionageModAgainstTeam(eIndex, getCounterespionageModAgainstTeam(eIndex) + iChange);
}

void CvTeam::verifySpyUnitsValidPlot()
{
	std::vector<CvUnit*> aUnits;
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);

		if (kPlayer.isAlive() && kPlayer.getTeam() == getID())
		{
			FOR_EACH_UNIT_VAR(pUnit, kPlayer)
			{
				PlayerTypes eOwner = pUnit->plot()->getOwner();
				if (NO_PLAYER != eOwner)
				{
					if (pUnit->isSpy())
					{
						if (!kPlayer.canSpiesEnterBorders(eOwner))
							aUnits.push_back(pUnit);
					}
				}
			}
		}
	}

	for (uint i = 0; i < aUnits.size(); ++i)
	{
		aUnits[i]->jumpToNearestValidPlot();
	}
}

void CvTeam::setForceRevealedBonus(BonusTypes eBonus, bool bRevealed)
{
	if (isForceRevealedBonus(eBonus) == bRevealed)
	{
		return;
	}

	for (int iI = 0; iI < GC.getMap().numPlots(); ++iI)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(iI);

		if (pLoopPlot->getBonusType() == eBonus)
		{
			if (pLoopPlot->getTeam() == getID())
			{
				pLoopPlot->updatePlotGroupBonus(false);
			}
		}
	}

	if (bRevealed)
	{
		m_aeRevealedBonuses.push_back(eBonus);
	}
	else
	{
		std::vector<BonusTypes>::iterator it;

		for (it = m_aeRevealedBonuses.begin(); it != m_aeRevealedBonuses.end(); ++it)
		{
			if (*it == eBonus)
			{
				m_aeRevealedBonuses.erase(it);
				break;
			}
		}
	}

	for (int iI = 0; iI < GC.getMap().numPlots(); ++iI)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(iI);

		if (pLoopPlot->getBonusType() == eBonus)
		{
			if (pLoopPlot->getTeam() == getID())
			{
				pLoopPlot->updatePlotGroupBonus(true);
			}
		}
	}

	for (int iI = 0; iI < GC.getMap().numPlots(); ++iI)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(iI);

		if (pLoopPlot->getBonusType() == eBonus)
		{
			pLoopPlot->updateYield();
			pLoopPlot->setLayoutDirty(true);
		}
	}
}

bool CvTeam::isForceRevealedBonus(BonusTypes eBonus) const
{
	std::vector<BonusTypes>::const_iterator it;

	for (it = m_aeRevealedBonuses.begin(); it != m_aeRevealedBonuses.end(); ++it)
	{
		if (*it == eBonus)
		{
			return true;
		}
	}

	return false;
}

// K-Mod
bool CvTeam::isBonusRevealed(BonusTypes eBonus) const
{
	FAssert(eBonus >= 0 && eBonus < GC.getNumBonusInfos());
	return isHasTech((TechTypes)GC.getBonusInfo(eBonus).getTechReveal()) || isForceRevealedBonus(eBonus);
}
// K-Mod end

// <advc.108> Based on CvPlayer::initFreeUnits
void CvTeam::revealSurroundingPlots(CvPlot const& kCenter, int iRange) const {

	for(int i = 0; i < GC.getMap().numPlots(); i++) {
		CvPlot& kPlot = *GC.getMap().plotByIndex(i);
		if(plotDistance(&kPlot, &kCenter) <= iRange)
			kPlot.setRevealed(getID(), true, false, NO_TEAM, false);
	}
} // </advc.108>

int CvTeam::countNumHumanGameTurnActive() const
{
	int iCount = 0;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);

		if (kLoopPlayer.isHuman() && kLoopPlayer.getTeam() == getID())
		{
			if (kLoopPlayer.isTurnActive())
			{
				++iCount;
			}
		}
	}

	return iCount;
}

void CvTeam::setTurnActive(bool bNewValue, bool bDoTurn)
{
	FAssert(GC.getGame().isSimultaneousTeamTurns());

	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kPlayer.isAlive() && kPlayer.getTeam() == getID())
		{
			kPlayer.setTurnActive(bNewValue, bDoTurn);
		}
	}
}

bool CvTeam::isTurnActive() const
{
	FAssert(GC.getGame().isSimultaneousTeamTurns());

	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kPlayer.getTeam() == getID())
		{
			if (kPlayer.isTurnActive())
			{
				return true;
			}
		}
	}

	return false;
}


void CvTeam::doWarWeariness()
{
	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (getWarWeariness((TeamTypes)iI) > 0)
		{
			changeWarWeariness(((TeamTypes)iI), 100 * GC.getDefineINT("WW_DECAY_RATE"));

			if (!(GET_TEAM((TeamTypes)iI).isAlive()) || !isAtWar((TeamTypes)iI) || GC.getGame().isOption(GAMEOPTION_ALWAYS_WAR) || GC.getGame().isOption(GAMEOPTION_NO_CHANGING_WAR_PEACE))
			{
				setWarWeariness(((TeamTypes)iI), ((getWarWeariness((TeamTypes)iI) * GC.getDefineINT("WW_DECAY_PEACE_PERCENT")) / 100));
			}
		}
	}
}

// advc: Body cut from doTurn
void CvTeam::doBarbarianResearch() {

	FAssert(isBarbarian());
	CvGame& g = GC.getGame();
	int iElapsed = g.getElapsedGameTurns();
	/*  <dlph.28> "Give some starting research to barbarians in advanced start
		depending on other players' tech status after advanced start. */
	if(iElapsed == 1 && // This function isn't called on turn 0
			g.isOption(GAMEOPTION_ADVANCED_START)) {
		for(TechTypes i = (TechTypes)0; i < GC.getNumTechInfos(); i = (TechTypes)(i+1)) {
			if(isHasTech((TechTypes)i))
				continue;
			int iCount = 0;
			int iPossibleCount = 0;
			for(int iJ = 0; iJ < MAX_CIV_TEAMS; iJ++) {
				if(GET_TEAM((TeamTypes)iJ).isAlive()) {
					if (GET_TEAM((TeamTypes)iJ).isHasTech(i))
						iCount++;
					iPossibleCount++;
				}
			}
			if(iCount > 0)
				setResearchProgress(i, (getResearchCost(i) * iCount) / iPossibleCount, getLeaderID());
		}
	} // </dlph.28>
	// <advc.307>
	bool bNoBarbCities = GC.getEraInfo(g.getCurrentEra()).isNoBarbCities();
	bool bIgnorePrereqs = bNoBarbCities;
			/*  Barbs get all tech from earlier eras for free. Don't need
				to catch up. */ //|| g.getStartEra() > 0;
	// </advc.307>
	/*  K-Mod. Delay the start of the barbarian research. (This is an
		experimental change. It is currently compensated by an increase in
		the barbarian tech rate.) */
	if (iElapsed < GC.getHandicapInfo(g.getHandicapType()).
			getBarbarianCreationTurnsElapsed() *
			GC.getGameSpeedInfo(g.getGameSpeedType()).getBarbPercent() / 200)
		return;

	CvPlayerAI const& kBarbPlayer = GET_PLAYER(BARBARIAN_PLAYER);
	for (TechTypes i = (TechTypes)0; i < GC.getNumTechInfos(); i = (TechTypes)(i+1))
	{
		//if (!isHasTech((TechTypes)iI))
		if (!isHasTech(i) && /* advc.307: */ (bIgnorePrereqs ||
				// K-Mod. Make no progress on techs until prereqs are researched.
				kBarbPlayer.canResearch(i, false, true)))
		{
			int iCount = 0;
			int iPossible = 0;
			int iHasTech = 0; // advc.307
			for (int iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
			{
				CvTeam const& kLoopTeam = GET_TEAM((TeamTypes)iJ);
				if (!kLoopTeam.isAlive())
					continue; // advc
				if (kLoopTeam.isHasTech(i)
						&& kLoopTeam.isInContactWithBarbarians()) // advc.302
					iCount++;
				iPossible++;
				// <advc.307>
				if(kLoopTeam.isHasTech(i))
					iHasTech++; // </advc.307>
			} /* advc.302: Don't stop barb research entirely even when there is
				 no contact with any civs */
			iCount = std::max(iCount, iHasTech / 3);
			if (iCount > 0)
			{
				FAssert(iPossible > 0);
				/*  advc.307: In the late game, count all civs as having contact with
					barbs if at least one of them has contact. Otherwise, New World barbs
					catch up too slowly when colonized only by one or two civs. */
				if(bNoBarbCities)
					iCount = std::max(iCount, (2 * iHasTech) / 3);
				//changeResearchProgress(((TechTypes)iI), ((getResearchCost((TechTypes)iI) * ((GC.getDefineINT("BARBARIAN_FREE_TECH_PERCENT") * iCount) / iPossibleCount)) / 100), getLeaderID());
				// K-Mod. Adjust research rate for game-speed & start-era - but _not_ world-size. And fix the rounding error.
				int iBaseCost = getResearchCost(i, false) * GC.getWorldInfo(GC.getMap().getWorldSize()).getResearchPercent() / 100;
				changeResearchProgress(i, std::max(1, iBaseCost *
						GC.getDefineINT("BARBARIAN_FREE_TECH_PERCENT") * iCount /
						(100 * iPossible)), kBarbPlayer.getID());
				// K-Mod end
			}
		}
	}
}

void CvTeam::updateTechShare(TechTypes eTech)
{
	if (isHasTech(eTech))
	{
		return;
	}

	int iBestShare = MAX_INT;

	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (isTechShare(iI))
		{
			iBestShare = std::min(iBestShare, (iI + 1));
		}
	}

	if (iBestShare != MAX_INT)
	{
		int iCount = 0;

		for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (GET_TEAM((TeamTypes)iI).isHasTech(eTech))
				{
					if (isHasMet((TeamTypes)iI))
					{
						FAssertMsg(iI != getID(), "iI is not expected to be equal with getID()");
						iCount++;
					}
				}
			}
		}

		if (iCount >= iBestShare)
		{
			setHasTech(eTech, true, NO_PLAYER, true, true);
			if(GET_PLAYER(getLeaderID()).isSignificantDiscovery(eTech)) // advc.550e
				setNoTradeTech(eTech, true); // dlph.31
		}
	}
}


void CvTeam::updateTechShare()
{
	for (int iI = 0; iI < GC.getNumTechInfos(); iI++)
	{
		updateTechShare((TechTypes)iI);
	}
}

// advc: Duplicate code cut from setHasTech
void CvTeam::updatePlotGroupBonus(TechTypes eTech, bool bAdd)
{
	for (int i = 0; i < GC.getMap().numPlots(); i++)
	{
		CvPlot& kPlot = *GC.getMap().plotByIndex(i);
		if (kPlot.getTeam() != getID())
			continue;
		BonusTypes eBonus = kPlot.getBonusType();
		if (eBonus == NO_BONUS)
			continue;
		CvBonusInfo const& kBonus = GC.getBonusInfo(eBonus);
		if (kBonus.getTechReveal() == eTech || kBonus.getTechCityTrade() == eTech ||
				kBonus.getTechObsolete() == eTech)
			kPlot.updatePlotGroupBonus(bAdd);
	}
}


void CvTeam::testCircumnavigated()
{
	CvWString szBuffer;
	int iX, iY;

	if (isBarbarian())
	{
		return;
	}

	if (!GC.getGame().circumnavigationAvailable())
	{
		return;
	}
	CvMap const& kMap = GC.getMap(); // advc
	if (kMap.isWrapX())
	{
		for (iX = 0; iX < kMap.getGridWidth(); iX++)
		{
			bool bFoundVisible = false;

			for (iY = 0; iY < kMap.getGridHeight(); iY++)
			{
				if (kMap.getPlot(iX, iY).isRevealed(getID(), false))
				{
					bFoundVisible = true;
					break;
				}
			}

			if (!bFoundVisible)
			{
				return;
			}
		}
	}

	if (kMap.isWrapY())
	{
		for (iY = 0; iY < kMap.getGridHeight(); iY++)
		{
			bool bFoundVisible = false;

			for (iX = 0; iX < kMap.getGridWidth(); iX++)
			{
				if (kMap.getPlot(iX, iY).isRevealed(getID(), false))
				{
					bFoundVisible = true;
					break;
				}
			}

			if (!bFoundVisible)
			{
				return;
			}
		}
	}

	GC.getGame().makeCircumnavigated();

	//if (GC.getGame().getElapsedGameTurns() > 0)
	if (GC.getGame().getElapsedGameTurns() > 1) // K-Mod (due to changes in when CvTeam::doTurn is called)
	{
		if (GC.getDefineINT("CIRCUMNAVIGATE_FREE_MOVES") != 0)
		{
			changeExtraMoves(DOMAIN_SEA, GC.getDefineINT("CIRCUMNAVIGATE_FREE_MOVES"));

			for (int iI = 0; iI < MAX_PLAYERS; iI++)
			{
				CvPlayer const& kObs = GET_PLAYER((PlayerTypes)iI);
				if(!kObs.isAlive())
					continue;
				if(getID() == kObs.getTeam())
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_CIRC_GLOBE",
							GC.getDefineINT("CIRCUMNAVIGATE_FREE_MOVES"));
				}
				else if(isHasMet(kObs.getTeam())
						|| kObs.isSpectator()) // advc.127
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_CIRC_GLOBE",
							getName().GetCString());
				}
				else szBuffer = gDLL->getText("TXT_KEY_MISC_UNKNOWN_CIRC_GLOBE");
					gDLL->getInterfaceIFace()->addHumanMessage(kObs.getID(),
							false, GC.getEVENT_MESSAGE_TIME(), szBuffer,
							"AS2D_GLOBECIRCUMNAVIGATED", MESSAGE_TYPE_MAJOR_EVENT,
							NULL, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"),
							// <advc.127b>
							getCapitalX(kObs.getTeam(), true),
							getCapitalY(kObs.getTeam(), true)); // </advc.127b>
			}

			szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_CIRC_GLOBE", getReplayName().GetCString());
			GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(), szBuffer, -1, -1, (ColorTypes)GC.getInfoTypeForString("COLOR_HIGHLIGHT_TEXT"));
		}
	}
}

// <advc.127b>
CvCity* CvTeam::getLeaderCapital(TeamTypes eObserver, bool bDebug) const {

	CvCity* r = GET_PLAYER(getLeaderID()).getCapitalCity();
	if(r != NULL && eObserver != NO_TEAM && !r->isRevealed(eObserver, bDebug))
		r = NULL;
	if(r != NULL)
		return r;
	int iMinRank = MAX_INT;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayer const& kMember = GET_PLAYER((PlayerTypes)i);
		if(kMember.isAlive() && kMember.getTeam() == getID()) {
			CvCity* pCapital = kMember.getCapitalCity();
			if(pCapital == NULL || (eObserver != NO_TEAM && !pCapital->isRevealed(eObserver, bDebug)))
				continue;
			int iRank = GC.getGame().getPlayerRank(kMember.getID());
			if(iRank < iMinRank) {
				r = pCapital;
				iMinRank = iRank;
			}
		}
	}
	return r;
}

int CvTeam::getCapitalX(TeamTypes eObserver, bool bDebug) const {

	CvCity* pCapital = getLeaderCapital(eObserver, bDebug);
	if(pCapital == NULL)
		return -1;
	return pCapital->getX();
}

int CvTeam::getCapitalY(TeamTypes eObserver, bool bDebug) const {

	CvCity* pCapital = getLeaderCapital(eObserver, bDebug);
	if(pCapital == NULL)
		return -1;
	return pCapital->getY();
} // </advc.127b>

void CvTeam::processTech(TechTypes eTech, int iChange)
{
	PROFILE_FUNC();

	int iI, iJ;

	if (GC.getTechInfo(eTech).isExtraWaterSeeFrom())
	{
		changeExtraWaterSeeFromCount(iChange);
	}

	if (GC.getTechInfo(eTech).isMapCentering())
	{
		if (iChange > 0)
		{
			setMapCentering(true);
		}
	}

	if (GC.getTechInfo(eTech).isMapTrading())
	{
		changeMapTradingCount(iChange);
	}

	if (GC.getTechInfo(eTech).isTechTrading())
	{
		changeTechTradingCount(iChange);
	}

	if (GC.getTechInfo(eTech).isGoldTrading())
	{
		changeGoldTradingCount(iChange);
	}

	if (GC.getTechInfo(eTech).isOpenBordersTrading())
	{
		changeOpenBordersTradingCount(iChange);
	}

	if (GC.getTechInfo(eTech).isDefensivePactTrading())
	{
		changeDefensivePactTradingCount(iChange);
	}

	if (GC.getTechInfo(eTech).isPermanentAllianceTrading())
	{
		changePermanentAllianceTradingCount(iChange);
	}

	if (GC.getTechInfo(eTech).isVassalStateTrading())
	{
		changeVassalTradingCount(iChange);
	}

	if (GC.getTechInfo(eTech).isBridgeBuilding())
	{
		changeBridgeBuildingCount(iChange);
	}

	if (GC.getTechInfo(eTech).isIrrigation())
	{
		changeIrrigationCount(iChange);
	}

	if (GC.getTechInfo(eTech).isIgnoreIrrigation())
	{
		changeIgnoreIrrigationCount(iChange);
	}

	if (GC.getTechInfo(eTech).isWaterWork())
	{
		changeWaterWorkCount(iChange);
	}

	for (iI = 0; iI < GC.getNumRouteInfos(); iI++)
	{
		changeRouteChange(((RouteTypes)iI), (GC.getRouteInfo((RouteTypes) iI).getTechMovementChange(eTech) * iChange));
	}

	for (iI = 0; iI < NUM_DOMAIN_TYPES; iI++)
	{
		changeExtraMoves(((DomainTypes)iI), (GC.getTechInfo(eTech).getDomainExtraMoves(iI) * iChange));
	}

	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		if (GC.getTechInfo(eTech).isCommerceFlexible(iI))
		{
			changeCommerceFlexibleCount(((CommerceTypes)iI), iChange);
		}
	}
	if (GC.getTechInfo(eTech).isAnyTerrainTrade()) // advc.003t
	{
		for (iI = 0; iI < GC.getNumTerrainInfos(); iI++)
		{
			if (GC.getTechInfo(eTech).isTerrainTrade(iI))
			{
				changeTerrainTradeCount(((TerrainTypes)iI), iChange);
			}
		}
	}
	if (GC.getTechInfo(eTech).isRiverTrade())
	{
		changeRiverTradeCount(iChange);
	}

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		if (GC.getBuildingInfo((BuildingTypes) iI).getObsoleteTech() == eTech)
		{
			changeObsoleteBuildingCount(((BuildingTypes)iI), iChange);
		}

		if (GC.getBuildingInfo((BuildingTypes) iI).getSpecialBuildingType() != NO_SPECIALBUILDING)
		{
			if (GC.getSpecialBuildingInfo((SpecialBuildingTypes) GC.getBuildingInfo((BuildingTypes) iI).getSpecialBuildingType()).getObsoleteTech() == eTech)
			{
				changeObsoleteBuildingCount(((BuildingTypes)iI), iChange);
			}
		}
	}

	for (iI = 0; iI < GC.getNumImprovementInfos(); iI++)
	{
		for (iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
		{
			changeImprovementYieldChange(((ImprovementTypes)iI), ((YieldTypes)iJ), (GC.getImprovementInfo((ImprovementTypes)iI).getTechYieldChanges(eTech, iJ) * iChange));
		}
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.getTeam() != getID())
			continue; // advc

		kLoopPlayer.changeFeatureProductionModifier(GC.getTechInfo(eTech).getFeatureProductionModifier() * iChange);
		kLoopPlayer.changeWorkerSpeedModifier(GC.getTechInfo(eTech).getWorkerSpeedModifier() * iChange);
		kLoopPlayer.changeTradeRoutes(GC.getTechInfo(eTech).getTradeRoutes() * iChange);
		kLoopPlayer.changeExtraHealth(GC.getTechInfo(eTech).getHealth() * iChange);
		kLoopPlayer.changeExtraHappiness(GC.getTechInfo(eTech).getHappiness() * iChange);

		kLoopPlayer.changeAssets(((GC.getTechInfo(eTech).getAssetValue()
				* 6) / 8) // advc.131: Makes it 6 per era instead of 8
				* iChange);
		kLoopPlayer.changePower(GC.getTechInfo(eTech).getPowerValue() * iChange);
		kLoopPlayer.changeTechScore(getTechScore(eTech) * iChange);
		// K-Mod. Extra commerce for specialist (new xml field)
		for (CommerceTypes eCommerce = (CommerceTypes)0; eCommerce < NUM_COMMERCE_TYPES; eCommerce=(CommerceTypes)(eCommerce+1))
		{
			kLoopPlayer.changeSpecialistExtraCommerce(eCommerce, GC.getTechInfo(eTech).getSpecialistExtraCommerce(eCommerce) * iChange);
		}
		// K-Mod end
	}

	for (iI = 0; iI < GC.getMap().numPlots(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(iI);
		BonusTypes eBonus = pLoopPlot->getBonusType();
		if (eBonus != NO_BONUS)
		{
			if (GC.getBonusInfo(eBonus).getTechReveal() == eTech)
			{
				pLoopPlot->updateYield();
				pLoopPlot->setLayoutDirty(true);
			}
		}
	}

	for (iI = 0; iI < GC.getNumBuildInfos(); iI++)
	{
		if (GC.getBuildInfo((BuildTypes) iI).getTechPrereq() == eTech)
		{
			if (GC.getBuildInfo((BuildTypes) iI).getRoute() != NO_ROUTE)
			{	// <kmodx> was iI=0 etc.
				for (iJ = 0; iJ < GC.getMap().numPlots(); iJ++)
				{
					CvPlot* pLoopPlot = GC.getMap().plotByIndex(iJ);
					// </kmodx>
					CvCity* pCity = pLoopPlot->getPlotCity();
					if (pCity != NULL)
					{
						if (pCity->getTeam() == getID())
						{
							pLoopPlot->updateCityRoute(true);
						}
					}
				}
			}
		}
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
		{
			GET_PLAYER((PlayerTypes)iI).updateCorporation();
		}
	}
}


void CvTeam::cancelDefensivePacts()
{
	FOR_EACH_DEAL_VAR(pLoopDeal)
	{
		if (!pLoopDeal->involves(getID()))
			continue;

		for (CLLNode<TradeData>* pNode = pLoopDeal->headTradesNode(); pNode != NULL;
			pNode = pLoopDeal->nextTradesNode(pNode))
		{
			if (pNode->m_data.m_eItemType == TRADE_DEFENSIVE_PACT)
			{
				pLoopDeal->kill();
				break;
			}
		}
	}
}

// <dlph.3> (actually an advc change)
void CvTeam::allowDefensivePactsToBeCanceled() {

	FOR_EACH_DEAL_VAR(d)
	{
		if (!d->involves(getID()) || d->getFirstTrades()->getLength() <= 0)
			continue;
		if(d->headFirstTradesNode()->m_data.m_eItemType == TRADE_DEFENSIVE_PACT)
			d->setInitialGameTurn(-100);
	}
} // </dlph.3>

// <advc.104i>
void CvTeam::makeUnwillingToTalk(TeamTypes eOther) {

	if(!getWPAI.isEnabled())
		return;
	// No need to make vassals unwilling to talk; can't negotiate war/ peace anyway.
	if(eOther == NO_TEAM || isAVassal() || GET_TEAM(eOther).isAVassal())
		return;
	/*  Make each leading member i of our team unwilling to talk to every
		leading member j of the other team. "Leading": team leader or human;
		only these can make peace. */
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		CvPlayerAI& kOurMember = GET_PLAYER((PlayerTypes)i);
		if(!kOurMember.isAlive() || kOurMember.getTeam() != getID() ||
				(!kOurMember.isHuman() && kOurMember.getID() != getLeaderID()))
			continue;
		for(int j = 0; j < MAX_CIV_PLAYERS; j++) {
			CvPlayerAI& kTheirMember = GET_PLAYER((PlayerTypes)j);
			if(!kTheirMember.isAlive() || kTheirMember.getTeam() != eOther ||
					(!kTheirMember.isHuman() && kTheirMember.getID() !=
					GET_TEAM(eOther).getLeaderID()))
				continue;
			if(!kOurMember.isHuman() &&
					kOurMember.AI_getMemoryCount(kTheirMember.getID(),
					MEMORY_DECLARED_WAR_RECENT) < 2) {
				kOurMember.AI_rememberEvent(kTheirMember.getID(),
						MEMORY_DECLARED_WAR_RECENT);
			}
			/*  Memory has no effect on humans. Make the other side unwilling then.
				Could simply always make both sides unwilling, but then, the
				expected RTT duration would become longer. */
			else if(kOurMember.isHuman() && kTheirMember.AI_getMemoryCount(
					kOurMember.getID(), MEMORY_DECLARED_WAR_RECENT) < 2) {
				kTheirMember.AI_rememberEvent(kOurMember.getID(),
						MEMORY_DECLARED_WAR_RECENT);
			}
		}
	}
} // </advc.104i>


void CvTeam::read(FDataStreamBase* pStream)
{
	// Init data before load
	reset();

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iNumMembers);
	pStream->Read(&m_iAliveCount);
	pStream->Read(&m_iEverAliveCount);
	pStream->Read(&m_iNumCities);
	pStream->Read(&m_iTotalPopulation);
	pStream->Read(&m_iTotalLand);
	pStream->Read(&m_iNukeInterception);
	pStream->Read(&m_iExtraWaterSeeFromCount);
	pStream->Read(&m_iMapTradingCount);
	pStream->Read(&m_iTechTradingCount);
	pStream->Read(&m_iGoldTradingCount);
	pStream->Read(&m_iOpenBordersTradingCount);
	pStream->Read(&m_iDefensivePactTradingCount);
	pStream->Read(&m_iPermanentAllianceTradingCount);
	pStream->Read(&m_iVassalTradingCount);
	pStream->Read(&m_iBridgeBuildingCount);
	pStream->Read(&m_iIrrigationCount);
	pStream->Read(&m_iIgnoreIrrigationCount);
	pStream->Read(&m_iWaterWorkCount);
	pStream->Read(&m_iVassalPower);
	pStream->Read(&m_iMasterPower);
	pStream->Read(&m_iEnemyWarWearinessModifier);
	pStream->Read(&m_iRiverTradeCount);
	pStream->Read(&m_iEspionagePointsEver);
	// <advc.003m>
	if(uiFlag >= 5) {
		pStream->Read(&m_iMajorWarEnemies);
		pStream->Read(&m_iMinorWarEnemies);
		pStream->Read(&m_iVassalWarEnemies);
		pStream->Read(&m_bMinorTeam);
	} // </advc.003m>

	pStream->Read(&m_bMapCentering);
	pStream->Read(&m_bCapitulated);
	// <advc.opt>
	if (uiFlag >= 7)
		pStream->Read(&m_bAnyVictoryCountdown); // </advc.opt>

	pStream->Read((int*)&m_eID);

	pStream->Read(MAX_TEAMS, m_aiStolenVisibilityTimer);
	pStream->Read(MAX_TEAMS, m_aiWarWeariness);
	pStream->Read(MAX_TEAMS, m_aiTechShareCount);
	pStream->Read(MAX_TEAMS, m_aiEspionagePointsAgainstTeam);
	pStream->Read(MAX_TEAMS, m_aiCounterespionageTurnsLeftAgainstTeam);
	pStream->Read(MAX_TEAMS, m_aiCounterespionageModAgainstTeam);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiCommerceFlexibleCount);
	// <advc.120g> Prior to uiFlag=6, espionage was flexible from the beginning.
	if(uiFlag < 6)
		m_aiCommerceFlexibleCount[COMMERCE_ESPIONAGE] = 1; // </advc.120g>
	pStream->Read(NUM_DOMAIN_TYPES, m_aiExtraMoves);
	pStream->Read(GC.getNumVoteSourceInfos(), m_aiForceTeamVoteEligibilityCount);

	pStream->Read(MAX_TEAMS, m_abHasMet);
	// K-mod
	if (uiFlag >= 1)
		pStream->Read(MAX_TEAMS, m_abHasSeen);
	else
		memcpy(m_abHasSeen, m_abHasMet, sizeof(*m_abHasSeen)*MAX_TEAMS);
	// K-Mod end
	pStream->Read(MAX_TEAMS, m_abAtWar);
	// <advc.162>
	if(uiFlag >= 4)
		pStream->Read(MAX_TEAMS, m_abJustDeclaredWar); // </advc.162>
	pStream->Read(MAX_TEAMS, m_abPermanentWarPeace);
	pStream->Read(MAX_TEAMS, m_abOpenBorders);
	// <advc.034>
	if(uiFlag >= 3)
		pStream->Read(MAX_CIV_TEAMS, m_abDisengage); // </advc.034>
	pStream->Read(MAX_TEAMS, m_abDefensivePact);
	pStream->Read(MAX_TEAMS, m_abForcePeace);
	pStream->Read(MAX_TEAMS, m_abVassal);
	// <advc.opt>
	pStream->Read((int*)&m_eMaster);
	if(uiFlag >= 2)
		pStream->Read((int*)&m_eLeader);
	else updateLeaderID();
	// </advc.opt>
	pStream->Read(GC.getNumVictoryInfos(), m_abCanLaunch);

	pStream->Read(GC.getNumRouteInfos(), m_paiRouteChange);
	pStream->Read(GC.getNumProjectInfos(), m_paiProjectCount);
	pStream->Read(GC.getNumProjectInfos(), m_paiProjectDefaultArtTypes);

	//project art types
	for(int i=0;i<GC.getNumProjectInfos();i++)
	{
		int temp;
		for(int j=0;j<m_paiProjectCount[i];j++)
		{
			pStream->Read(&temp);
			m_pavProjectArtTypes[i].push_back(temp);
		}
	}

	pStream->Read(GC.getNumProjectInfos(), m_paiProjectMaking);
	pStream->Read(GC.getNumUnitClassInfos(), m_paiUnitClassCount);
	pStream->Read(GC.getNumBuildingClassInfos(), m_paiBuildingClassCount);
	pStream->Read(GC.getNumBuildingInfos(), m_paiObsoleteBuildingCount);
	pStream->Read(GC.getNumTechInfos(), m_paiResearchProgress);
	pStream->Read(GC.getNumTechInfos(), m_paiTechCount);
	pStream->Read(GC.getNumTerrainInfos(), m_paiTerrainTradeCount);
	pStream->Read(GC.getNumVictoryInfos(), m_aiVictoryCountdown);
	// <advc.opt>
	if (uiFlag < 7)
	{
		for (int i = 0; i < GC.getNumVictoryInfos(); i++)
			if (getVictoryCountdown((VictoryTypes)i) >= 0)
				m_bAnyVictoryCountdown = true;
	} // </advc.opt>

	pStream->Read(GC.getNumTechInfos(), m_pabHasTech);
	pStream->Read(GC.getNumTechInfos(), m_pabNoTradeTech);

	for (int i = 0; i < GC.getNumImprovementInfos(); ++i)
	{
		pStream->Read(NUM_YIELD_TYPES, m_ppaaiImprovementYieldChange[i]);
	}

	int iSize;
	m_aeRevealedBonuses.clear();
	pStream->Read(&iSize);
	for (int i = 0; i < iSize; ++i)
	{
		BonusTypes eBonus;
		pStream->Read((int*)&eBonus);
		m_aeRevealedBonuses.push_back(eBonus);
	}
	// <advc.003m>
	if(uiFlag < 5) {
		updateMinorCiv(); // Need to do this before CvTeamAI::read
		if(getID() == MAX_PLAYERS - 1) {
			// All teams need to be loaded before war enemies can be counted
			for(int i = 0; i < MAX_TEAMS; i++) {
				CvTeam& t = GET_TEAM((TeamTypes)i);
				t.m_iMajorWarEnemies = t.countWarEnemies();
				t.m_iMinorWarEnemies = t.countWarEnemies(false, false) -
						t.m_iMajorWarEnemies;
				FAssert(t.m_iMinorWarEnemies >= 0);
				t.m_iVassalWarEnemies = t.m_iMajorWarEnemies -
						t.countWarEnemies(true, true);
				FAssert(t.m_iVassalWarEnemies >= 0);
			}
		}
	} // </advc.003m>
}


void CvTeam::write(FDataStreamBase* pStream)
{
	int iI;

	uint uiFlag = 1;
	uiFlag = 2; // advc.opt: m_eLeader added
	uiFlag = 3; // advc.034
	uiFlag = 4; // advc.162
	uiFlag = 5; // advc.003m
	uiFlag = 6; // advc.120g
	uiFlag = 7; // advc.opt: m_bAnyVictoryCountdown
	pStream->Write(uiFlag);

	pStream->Write(m_iNumMembers);
	pStream->Write(m_iAliveCount);
	pStream->Write(m_iEverAliveCount);
	pStream->Write(m_iNumCities);
	pStream->Write(m_iTotalPopulation);
	pStream->Write(m_iTotalLand);
	pStream->Write(m_iNukeInterception);
	pStream->Write(m_iExtraWaterSeeFromCount);
	pStream->Write(m_iMapTradingCount);
	pStream->Write(m_iTechTradingCount);
	pStream->Write(m_iGoldTradingCount);
	pStream->Write(m_iOpenBordersTradingCount);
	pStream->Write(m_iDefensivePactTradingCount);
	pStream->Write(m_iPermanentAllianceTradingCount);
	pStream->Write(m_iVassalTradingCount);
	pStream->Write(m_iBridgeBuildingCount);
	pStream->Write(m_iIrrigationCount);
	pStream->Write(m_iIgnoreIrrigationCount);
	pStream->Write(m_iWaterWorkCount);
	pStream->Write(m_iVassalPower);
	pStream->Write(m_iMasterPower);
	pStream->Write(m_iEnemyWarWearinessModifier);
	pStream->Write(m_iRiverTradeCount);
	pStream->Write(m_iEspionagePointsEver);
	// <advc.003m>
	pStream->Write(m_iMajorWarEnemies);
	pStream->Write(m_iMinorWarEnemies);
	pStream->Write(m_iVassalWarEnemies);
	pStream->Write(m_bMinorTeam);
	// </advc.003m>
	pStream->Write(m_bMapCentering);
	pStream->Write(m_bCapitulated);
	pStream->Write(m_bAnyVictoryCountdown); // advc.opt

	pStream->Write(m_eID);

	pStream->Write(MAX_TEAMS, m_aiStolenVisibilityTimer);
	pStream->Write(MAX_TEAMS, m_aiWarWeariness);
	pStream->Write(MAX_TEAMS, m_aiTechShareCount);
	pStream->Write(MAX_TEAMS, m_aiEspionagePointsAgainstTeam);
	pStream->Write(MAX_TEAMS, m_aiCounterespionageTurnsLeftAgainstTeam);
	pStream->Write(MAX_TEAMS, m_aiCounterespionageModAgainstTeam);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiCommerceFlexibleCount);
	pStream->Write(NUM_DOMAIN_TYPES, m_aiExtraMoves);
	pStream->Write(GC.getNumVoteSourceInfos(), m_aiForceTeamVoteEligibilityCount);

	pStream->Write(MAX_TEAMS, m_abHasMet);
	pStream->Write(MAX_TEAMS, m_abHasSeen); // K-Mod. uiFlag >= 1
	pStream->Write(MAX_TEAMS, m_abAtWar);
	pStream->Write(MAX_TEAMS, m_abJustDeclaredWar); // advc.162
	pStream->Write(MAX_TEAMS, m_abPermanentWarPeace);
	pStream->Write(MAX_TEAMS, m_abOpenBorders);
	pStream->Write(MAX_CIV_TEAMS, m_abDisengage); // advc.034
	pStream->Write(MAX_TEAMS, m_abDefensivePact);
	pStream->Write(MAX_TEAMS, m_abForcePeace);
	pStream->Write(MAX_TEAMS, m_abVassal);
	// <advc.opt>
	pStream->Write(m_eMaster);
	pStream->Write(m_eLeader);
	// </advc.opt>
	pStream->Write(GC.getNumVictoryInfos(), m_abCanLaunch);

	pStream->Write(GC.getNumRouteInfos(), m_paiRouteChange);
	pStream->Write(GC.getNumProjectInfos(), m_paiProjectCount);
	pStream->Write(GC.getNumProjectInfos(), m_paiProjectDefaultArtTypes);

	//project art types
	for(int i=0;i<GC.getNumProjectInfos();i++)
	{
		for(int j=0;j<m_paiProjectCount[i];j++)
			pStream->Write(m_pavProjectArtTypes[i][j]);
	}

	pStream->Write(GC.getNumProjectInfos(), m_paiProjectMaking);
	pStream->Write(GC.getNumUnitClassInfos(), m_paiUnitClassCount);
	pStream->Write(GC.getNumBuildingClassInfos(), m_paiBuildingClassCount);
	pStream->Write(GC.getNumBuildingInfos(), m_paiObsoleteBuildingCount);
	pStream->Write(GC.getNumTechInfos(), m_paiResearchProgress);
	pStream->Write(GC.getNumTechInfos(), m_paiTechCount);
	pStream->Write(GC.getNumTerrainInfos(), m_paiTerrainTradeCount);
	pStream->Write(GC.getNumVictoryInfos(), m_aiVictoryCountdown);

	pStream->Write(GC.getNumTechInfos(), m_pabHasTech);
	pStream->Write(GC.getNumTechInfos(), m_pabNoTradeTech);

	for (iI=0;iI<GC.getNumImprovementInfos();iI++)
	{
		pStream->Write(NUM_YIELD_TYPES, m_ppaaiImprovementYieldChange[iI]);
	}

	pStream->Write(m_aeRevealedBonuses.size());
	for (std::vector<BonusTypes>::iterator it = m_aeRevealedBonuses.begin(); it != m_aeRevealedBonuses.end(); ++it)
	{
		pStream->Write(*it);
	}
}


bool CvTeam::hasShrine(ReligionTypes eReligion)
{
	bool bHasShrine = false;

	if (eReligion != NO_RELIGION)
	{
		CvCity* pHolyCity = GC.getGame().getHolyCity(eReligion);

		// if the holy city exists, and we own it
		if (pHolyCity != NULL && GET_PLAYER(pHolyCity->getOwner()).getTeam() == getID())
			bHasShrine = pHolyCity->hasShrine(eReligion);
	}

	return bHasShrine;
}

void CvTeam::getCompletedSpaceshipProjects(std::map<ProjectTypes, int>& mapProjects) const
{
	for (int i = 0; i < GC.getNumProjectInfos(); i++)
	{
		ProjectTypes eProject = (ProjectTypes) i;
		if (GC.getProjectInfo(eProject).isSpaceship())
		{
			mapProjects[eProject] = getProjectCount(eProject);
		}
	}
}

int CvTeam::getProjectPartNumber(ProjectTypes eProject, bool bAssert) const
{
	int iNumBuilt = getProjectCount(eProject);
	for (int i = 0; i < iNumBuilt; i++)
	{
		int artType = getProjectArtType(eProject, i);
		if (artType < 0)
		{
			return i;
		}
	}

	//didn't find empty part number
	if (bAssert)
	{
		FAssertMsg(false, "Unknown part number.");
	}

	//return the last one
	return std::min(iNumBuilt, GC.getProjectInfo(eProject).getMaxTeamInstances() - 1);
}

bool CvTeam::hasLaunched() const
{	// <advc.opt>
	if (!isAnyVictoryCountdown())
		return false; // </advc.opt>
	VictoryTypes spaceVictory = GC.getGame().getSpaceVictory();
	if (spaceVictory != NO_VICTORY)
	{
		return (getVictoryCountdown(spaceVictory) >= 0);
	}
	return false;
}

// <advc>
bool CvTeam::hasTechToClear(FeatureTypes eFeature, TechTypes eCurrentResearch) const {

	for(int i = 0; i < GC.getNumBuildInfos(); i++) {
		CvBuildInfo const& kBuild = GC.getBuildInfo((BuildTypes)i);
		if(kBuild.getFeatureTime(eFeature) <= 0)
			continue;
		TechTypes aeReqs[2] = { (TechTypes)kBuild.getTechPrereq(),
								(TechTypes)kBuild.getFeatureTech(eFeature) };
		bool bValid = true;
		for(int j = 0; j < 2; j++) {
			if(aeReqs[j] != NO_TECH && eCurrentResearch != aeReqs[j] &&
					!isHasTech(aeReqs[j])) {
				bValid = false;
				break;
			}
		}
		if(bValid)
			return true;
	}
	return false;
} // </advc>
