// team.cpp

#include "CvGameCoreDLL.h"
#include "CvTeam.h"
#include "CvAgents.h" // advc.agent
#include "CoreAI.h"
#include "UWAIAgent.h" // advc.104t
#include "CvCity.h"
#include "CvUnit.h"
#include "CvSelectionGroup.h"
#include "CvDeal.h"
#include "CvMap.h"
#include "CvArea.h"
#include "CvInfo_City.h"
#include "CvInfo_Terrain.h"
#include "CvInfo_GameOption.h"
#include "CvInfo_Civics.h"
#include "CvDiploParameters.h"
#include "CvPopupInfo.h"
#include "BBAILog.h" // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000
#include "CvBugOptions.h" // advc.071

// advc.003u: Statics moved from CvTeamAI
CvTeamAI** CvTeam::m_aTeams = NULL;

void CvTeam::initStatics()
{
	m_aTeams = new CvTeamAI*[MAX_TEAMS];
	for (int i = 0; i < MAX_PLAYERS; i++)
		m_aTeams[i] = new CvTeamAI((TeamTypes)i);
}

void CvTeam::freeStatics()
{
	for (int i = 0; i < MAX_TEAMS; i++)
		SAFE_DELETE(m_aTeams[i]);
	SAFE_DELETE_ARRAY(m_aTeams);
}

// <kekm.26>
std::queue<TeamTypes> CvTeam::attacking_queue;
std::queue<TeamTypes> CvTeam::defending_queue;
std::queue<bool> CvTeam::newdiplo_queue;
std::queue<WarPlanTypes> CvTeam::warplan_queue;
std::queue<bool> CvTeam::primarydow_queue;
bool CvTeam::bTriggeringWars = false;
// </kekm.26>

CvTeam::CvTeam(/* advc.003u: */ TeamTypes eID)
{
	m_pavProjectArtTypes = NULL;
	reset(eID, true);
}


CvTeam::~CvTeam()
{
	uninit();
}


void CvTeam::init(TeamTypes eID)
{
	reset(eID); // Reset serialized data

	AI().AI_init();

	// BETTER_BTS_AI_MOD 12/30/08 jdog5000
	if(GC.getGame().isFinalInitialized())
	{
		// advc (note): This is for teams spawned through liberation
		for(int i = 0; i < MAX_TEAMS; i++)
		{
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
	SAFE_DELETE_ARRAY(m_pavProjectArtTypes);
}

// Initializes data members that are serialized.
void CvTeam::reset(TeamTypes eID, bool bConstructorCall)
{
	uninit();

	m_eID = eID;

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
	// <advc.134a>
	m_eOfferingPeace = NO_TEAM;
	m_iPeaceOfferStage = 0;
	// </advc.134a>
	// <advc.opt>
	m_bAnyVictoryCountdown = false;
	m_eMaster = NO_TEAM;
	m_eLeader = NO_PLAYER;
	// </advc.opt>
	m_iTechCount = 0; // advc.101

	m_aiStolenVisibilityTimer.reset();
	m_aiWarWeariness.reset();
	m_aiTechShareCount.reset();
	m_aiEspionagePointsAgainstTeam.reset();
	m_aiCounterespionageTurnsLeftAgainstTeam.reset();
	m_aiCounterespionageModAgainstTeam.reset();
	m_aiCommerceFlexibleCount.reset();
	m_aiExtraMoves.reset();
	m_aiForceTeamVoteEligibilityCount.reset();
	m_aiRouteChange.reset();
	m_aiProjectCount.reset();
	m_aiProjectMaking.reset();
	m_aiProjectDefaultArtTypes.reset();
	m_aiUnitClassCount.reset();
	m_aiBuildingClassCount.reset();
	m_aiObsoleteBuildingCount.reset();
	m_aiResearchProgress.reset();
	m_aiTechCount.reset();
	m_aiTerrainTradeCount.reset();
	m_aiVictoryCountdown.reset();
	m_aiHasMetTurn.reset(); // advc.091
	m_aaiImprovementYieldChange.reset();
	m_abAtWar.reset();
	m_abJustDeclaredWar.reset(); // advc.162
	m_abHasSeen.reset(); // K-Mod
	m_abPermanentWarPeace.reset();
	m_abOpenBorders.reset();
	m_abDisengage.reset(); // advc.034
	m_abDefensivePact.reset();
	m_abForcePeace.reset();
	m_abCanLaunch.reset();
	m_abHasTech.reset();
	m_abNoTradeTech.reset();
	if (!bConstructorCall && getID() != NO_TEAM)
	{
		for (int i = 0; i < MAX_TEAMS; i++)
		{
			CvTeam& kLoopTeam = GET_TEAM((TeamTypes)i);
			kLoopTeam.m_aiStolenVisibilityTimer.reset(getID());
			kLoopTeam.m_aiWarWeariness.reset(getID());
			kLoopTeam.m_aiTechShareCount.reset(getID());
			kLoopTeam.m_aiEspionagePointsAgainstTeam.reset(getID());
			kLoopTeam.m_aiCounterespionageTurnsLeftAgainstTeam.reset(getID());
			kLoopTeam.m_aiCounterespionageModAgainstTeam.reset(getID());
			kLoopTeam.m_aiHasMetTurn.reset(getID()); // advc.091
			kLoopTeam.m_abHasSeen.reset(getID()); // K-Mod
			kLoopTeam.m_abAtWar.reset(getID());
			kLoopTeam.m_abJustDeclaredWar.reset(getID()); // advc.162
			kLoopTeam.m_abPermanentWarPeace.reset(getID());
			kLoopTeam.m_abOpenBorders.reset(getID());
			kLoopTeam.m_abDisengage.reset(getID()); // advc.034
			kLoopTeam.m_abDefensivePact.reset(getID());
			kLoopTeam.m_abForcePeace.reset(getID());
			// <advc.opt>
			if (kLoopTeam.m_eMaster == getID())
				kLoopTeam.m_eMaster = NO_TEAM; // </advc.opt>
		}
	}

	if (!bConstructorCall)
	{
		FAssertMsg(m_pavProjectArtTypes == NULL, "memory leak?");
		m_pavProjectArtTypes = new std::vector<int>[GC.getNumProjectInfos()];

		m_aeRevealedBonuses.clear();

		AI().AI_reset(false);
	}
}


/*  BETTER_BTS_AI_MOD, 12/30/08, jdog5000:
	(for clearing data stored in plots and cities for this team) */
void CvTeam::resetPlotAndCityData()
{
	CvMap const& kMap = GC.getMap();
	for (int i = 0; i < kMap.numPlots(); i++)
	{
		CvPlot& kPlot = kMap.getPlotByIndex(i);
		kPlot.setRevealedOwner(getID(), NO_PLAYER);
		kPlot.setRevealedImprovementType(getID(), NO_IMPROVEMENT);
		kPlot.setRevealedRouteType(getID(), NO_ROUTE);
		kPlot.setRevealed(getID(), false, false, getID(), true);

		CvCity* pCity = kPlot.getPlotCity();
		if (pCity != NULL)
		{
			pCity->setRevealed(getID(), false);
			pCity->setEspionageVisibility(getID(), false, true);
		}
	}
}


void CvTeam::addTeam(TeamTypes eTeam)
{
	FAssert(eTeam != NO_TEAM);
	FAssert(eTeam != getID());

	int const iOriginalTeamSize = getNumMembers(); // K-Mod

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayer const& kObs = GET_PLAYER((PlayerTypes)i);
		if (!kObs.isAlive())
			continue;
		if (kObs.getTeam() != getID() && kObs.getTeam() != eTeam)
		{
			if ((isHasMet(kObs.getTeam()) && GET_TEAM(eTeam).isHasMet(kObs.getTeam())) ||
				kObs.isSpectator()) // advc.127
			{
				CvWString szBuffer(gDLL->getText("TXT_KEY_MISC_PLAYER_PERMANENT_ALLIANCE",
						getName().GetCString(), GET_TEAM(eTeam).getName().GetCString()));
				gDLL->UI().addMessage(kObs.getID(), false, -1, szBuffer, "AS2D_THEIRALLIANCE",
						MESSAGE_TYPE_MAJOR_EVENT, // advc.106b
						NULL, GC.getColorType("HIGHLIGHT_TEXT"),
						// advc.127b:
						getCapitalX(kObs.getTeam(), true), getCapitalY(kObs.getTeam(), true));
			}
		}
	}

	CvWString szBuffer(gDLL->getText("TXT_KEY_MISC_PLAYER_PERMANENT_ALLIANCE",
			getReplayName().GetCString(), GET_TEAM(eTeam).getReplayName().GetCString()));
	CvGame& kGame = GC.getGame();
	kGame.addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(), szBuffer, -1, -1,
			GC.getColorType("HIGHLIGHT_TEXT"));

	// K-Mod note: the cancel deals code use to be here. I've moved it lower down.

	shareItems(eTeam);
	// kekm.26: "This team is not going to be used anymore and doing this might break some things."
	//GET_TEAM(eTeam).shareItems(getID());
	// <advc>
	std::vector<CvTeam*> apOther;
	for (int i = 0; i < MAX_TEAMS; i++)
	{
		CvTeam& kOther = GET_TEAM((TeamTypes)i);
		if (kOther.getID() != getID() && kOther.getID() != eTeam && kOther.isAlive())
			apOther.push_back(&kOther);
	} // </advc>

	for (size_t i = 0; i < apOther.size(); i++)
	{
		TeamTypes const eOther = apOther[i]->getID();
		if (GET_TEAM(eTeam).isHasMet(eOther))
			meet(eOther, false);
		else if (isHasMet(eOther))
			GET_TEAM(eTeam).meet(eOther, false);
	}

	for (size_t i = 0; i < apOther.size(); i++)
	{
		TeamTypes const eOther = apOther[i]->getID();
		if (GET_TEAM(eTeam).isAtWar(eOther))
		{
			//declareWar(...);
			queueWar( // kekm.26
					getID(), eOther, false, GET_TEAM(eTeam).AI_getWarPlan(eOther));
		}
		else if (isAtWar(eOther))
		{
			//GET_TEAM(eTeam).declareWar(...);
			queueWar( // kekm.26
					eTeam, eOther, false, AI().AI_getWarPlan(eOther));
		}
	}
	// <kekm.26>
	triggerWars();
	for (size_t i = 0; i < apOther.size(); i++)
	{
		TeamTypes const eOther = apOther[i]->getID();
		if (GET_TEAM(eTeam).isAtWar(eOther))
			queueWar(eOther, getID(), false, WARPLAN_DOGPILE, false);
		else if (isAtWar(eOther))
			queueWar(eOther, eTeam, false, WARPLAN_DOGPILE, false);
	}
	triggerWars();
	// </kekm.26>
	for (size_t i = 0; i < apOther.size(); i++)
	{
		TeamTypes const eOther = apOther[i]->getID();
		if (GET_TEAM(eTeam).isPermanentWarPeace(eOther))
				setPermanentWarPeace(eOther, true);
		else if (isPermanentWarPeace(eOther))
			GET_TEAM(eTeam).setPermanentWarPeace(eOther, true);
	}

	for (size_t i = 0; i < apOther.size(); i++)
	{
		TeamTypes const eOther = apOther[i]->getID();
		if (GET_TEAM(eTeam).isOpenBorders(eOther))
		{
			setOpenBorders(eOther, true);
			apOther[i]->setOpenBorders(getID(), true);
		}
		else if (isOpenBorders(eOther))
		{
			GET_TEAM(eTeam).setOpenBorders(eOther, true);
			apOther[i]->setOpenBorders(eTeam, true);
		}
	}

	for (size_t i = 0; i < apOther.size(); i++)
	{
		TeamTypes const eOther = apOther[i]->getID();
		if (GET_TEAM(eTeam).isDefensivePact(eOther))
		{
			setDefensivePact(eOther, true);
			apOther[i]->setDefensivePact(getID(), true);
		}
		else if (isDefensivePact(eOther))
		{
			GET_TEAM(eTeam).setDefensivePact(eOther, true);
			apOther[i]->setDefensivePact(eTeam, true);
		}
	}

	for (size_t i = 0; i < apOther.size(); i++)
	{
		TeamTypes const eOther = apOther[i]->getID();
		if (GET_TEAM(eTeam).isForcePeace(eOther))
		{
			setForcePeace(eOther, true);
			apOther[i]->setForcePeace(getID(), true);
		}
		else if (isForcePeace(eOther))
		{
			GET_TEAM(eTeam).setForcePeace(eOther, true);
			apOther[i]->setForcePeace(eTeam, true);
		}
	}
	/*  <advc.opt> Some changes are necessary here b/c vassals now store only
		the id of a single master. */
	if (GET_TEAM(eTeam).isAVassal())
	{
		bool bCapitulated = //isCapitulated()
				GET_TEAM(eTeam).isCapitulated(); // K-Mod
		setVassal(getMasterTeam(), false, bCapitulated);
		setVassal(GET_TEAM(eTeam).getMasterTeam(), true, bCapitulated);
	}
	// Don't turn eTeam into a vassal; it'll die anyway.
	/*else if (isAVassal()) {
		bool bCapitulated = isCapitulated();
		GET_TEAM(eTeam).setVassal(GET_TEAM(eTeam).getMasterTeam(), false, bCapitulated);
		GET_TEAM(eTeam).setVassal(getMasterTeam(), true, bCapitulated);
	}*/
	for (size_t i = 0; i < apOther.size(); i++)
	{
		bool bCapitulated = apOther[i]->isCapitulated();
		if (apOther[i]->isVassal(eTeam))
		{
			apOther[i]->setVassal(eTeam, false, bCapitulated);
			apOther[i]->setVassal(getID(), true, bCapitulated);
		}
		/*else if (apOther[i]->isVassal(getID())) {
			apOther[i]->setVassal(getID(), false, bCapitulated);
			apOther[i]->setVassal(eTeam, true, bCapitulated);
		}*/
	}
	// </advc.opt>
	shareCounters(eTeam);
	//GET_TEAM(eTeam).shareCounters(getID());
	/*  K-Mod note: eTeam is not going to be used after we've finished this merge,
		so the sharing does not need to be two-way. */

	/*  <kekm.26> "Fix for permanent alliance bug that caused permanent espionage visibility
		for cities that only the players with the higher team number sees at the time of the merge. */
	CvMap const& kMap = GC.getMap();
	for(int i = 0; i < kMap.numPlots(); i++)
	{
		CvPlot const& kPlot = kMap.getPlotByIndex(i);
		if (kPlot.isCity())
			kPlot.getPlotCity()->setEspionageVisibility(eTeam, false, true);
	} // </kekm.26>

	/*  advc.104t: Leader id needed later for merging data; unavailable after the
		loop below. */
	PlayerTypes eTeamLeader = GET_TEAM(eTeam).getLeaderID();
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (GET_PLAYER((PlayerTypes)i).getTeam() == eTeam)
			GET_PLAYER((PlayerTypes)i).setTeam(getID());
	}
	updateLeaderID(); // advc.opt
	// <kekm.13>
	// "AP resident and UN secretary general teams need to be updated if that team will not be used anymore."
	FOR_EACH_ENUM(VoteSource)
	{
		if (kGame.canHaveSecretaryGeneral(eLoopVoteSource) &&
			kGame.getSecretaryGeneral(eLoopVoteSource) == eTeam)
		{
			FOR_EACH_ENUM(Vote)
			{
				if (GC.getInfo(eLoopVote).isVoteSourceType(eLoopVoteSource) &&
					GC.getInfo(eLoopVote).isSecretaryGeneral())
				{
					VoteTriggeredData kData;
					kData.iId = FFreeList::INVALID_INDEX;
					kData.eVoteSource = eLoopVoteSource;
					kData.kVoteOption.eVote = eLoopVote;
					kData.kVoteOption.iCityId = -1;
					kData.kVoteOption.szText.clear();
					kData.kVoteOption.ePlayer = NO_PLAYER;
					kGame.setVoteOutcome(kData, (PlayerVoteTypes)getID());
				}
			}
		}
	} // </kekm.13>
	/*	K-Mod. Adjust the progress of unfinished research
		so that it is proportionally the same as it was before the merge. */
	{
		// cf. CvTeam::getResearchCost
		int iCostMultiplier = 100;
		iCostMultiplier *= 100 +
				GC.getDefineINT(CvGlobals::TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER) *
				(getNumMembers() - 1); // new
		iCostMultiplier /= 100 +
				GC.getDefineINT(CvGlobals::TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER) *
				(iOriginalTeamSize - 1); // old

		FAssert(iCostMultiplier >= 100);

		FOR_EACH_ENUM(Tech)
		{
			if (!isHasTech(eLoopTech) && getResearchProgress(eLoopTech) > 0)
			{
				setResearchProgress(eLoopTech,
						getResearchProgress(eLoopTech) * iCostMultiplier / 100,
						getLeaderID());
			}
		}
	} // K-Mod end

	/*	K-Mod: The following cancel deals code has been moved from higher up.
		I've done this so that when open-borders is canceled,
		it doesn't bump our new allies out of our borders. */
	FOR_EACH_DEAL_VAR(pLoopDeal)
	{
		/*if ((TEAMID(pLoopDeal->getFirstPlayer()) == getID() && TEAMID(pLoopDeal->getSecondPlayer()) == eTeam) ||
			  (TEAMID(pLoopDeal->getFirstPlayer()) == eTeam && TEAMID(pLoopDeal->getSecondPlayer()) == getID()))*/ // BtS
		/*	K-Mod: The player's teams have already been reassigned -
			so we don't check for eTeam anymore. */
		if (!pLoopDeal->isBetween(getID(), getID())) // advc: Replacing the K-Mod replacement
			continue;
		FOR_EACH_TRADE_ITEM(pLoopDeal->getFirstList())
		{
			TradeableItems eType = pItem->m_eItemType;
			if (eType == TRADE_OPEN_BORDERS || eType == TRADE_DEFENSIVE_PACT ||
				eType == TRADE_PEACE_TREATY || eType == TRADE_VASSAL ||
				eType == TRADE_SURRENDER ||
				// advc.034: Simplest to just cancel it
				eType == TRADE_DISENGAGE)
			{
				pLoopDeal->kill();
				break;
			}
		}
	}
	// <kekm.1>
	FOR_EACH_ENUM(Domain)
	{
		changeExtraMoves(eLoopDomain, std::max(0,
				GET_TEAM(eTeam).getExtraMoves(eLoopDomain) -
				getExtraMoves(eLoopDomain)));
	} // </kekm.1>
	for (int i = 0; i < kMap.numPlots(); i++)
	{
		CvPlot& kPlot = kMap.getPlotByIndex(i); // advc

		/*  kekm.26: This part is moved above to the part where members of the
			other team still had their old team number. */
		// <kekm.2>
		/*if(kPlot.isCity()) {
			CvCity& kCity = *kPlot.getPlotCity(); // advc
			kCity.setEspionageVisibility(eTeam, false, false);
		}*/ // </kekm.2>

		kPlot.changeVisibilityCount(getID(),
				kPlot.getVisibilityCount(eTeam), NO_INVISIBLE, false);

		FOR_EACH_ENUM2(Invisible, eInv)
		{
			kPlot.changeInvisibleVisibilityCount(getID(),
					eInv, kPlot.getInvisibleVisibilityCount(eTeam, eInv));
		}

		if (kPlot.isRevealed(eTeam))
			kPlot.setRevealed(getID(), true, false, eTeam, false);
		kPlot.updateTeam(); // advc.opt: Need to update cached team
	}

	kGame.updatePlotGroups();
	int const iSecondTeamSize = getNumMembers() - iOriginalTeamSize; // kekm.26
	for (int i = 0; i < MAX_TEAMS; i++)
	{
		if (i == getID() || i == eTeam)
			continue;

		CvTeamAI& kOther = GET_TEAM((TeamTypes)i); // K-Mod
		/*kOther.setWarWeariness(getID(), ((kOther.getWarWeariness(getID()) + kOther.getWarWeariness(eTeam)) / 2));
		kOther.AI_setAtWarCounter(getID(), ((kOther.AI_getAtWarCounter(getID()) + kOther.AI_getAtWarCounter(eTeam)) / 2));
		// ... (BtS code deleted)
		kOther.setEspionagePointsAgainstTeam(getID(), std::max(kOther.getEspionagePointsAgainstTeam(getID()), kOther.getEspionagePointsAgainstTeam(eTeam))); // unofficial patch*/
		/*  <kekm.26> "These counters now scale properly with number of players in teams.
			Also, espionage is now sum instead of max. */
		kOther.AI_setAtWarCounter(getID(), (iOriginalTeamSize *
				kOther.AI_getAtWarCounter(getID()) + iSecondTeamSize *
				kOther.AI_getAtWarCounter(eTeam)) / getNumMembers());
		// advc: The at-war counters should be consistent
		AI().AI_setAtWarCounter(kOther.getID(), kOther.AI_getAtWarCounter(getID()));
		kOther.setStolenVisibilityTimer(getID(), (iOriginalTeamSize *
				kOther.getStolenVisibilityTimer(getID()) + iSecondTeamSize *
				kOther.getStolenVisibilityTimer(eTeam)) / getNumMembers());
		kOther.AI_setAtPeaceCounter(getID(), (iOriginalTeamSize *
				kOther.AI_getAtPeaceCounter(getID()) + iSecondTeamSize *
				kOther.AI_getAtPeaceCounter(eTeam)) / getNumMembers());
		kOther.AI_setHasMetCounter(getID(), (iOriginalTeamSize *
				kOther.AI_getHasMetCounter(getID()) + iSecondTeamSize *
				kOther.AI_getHasMetCounter(eTeam)) / getNumMembers());
		// <advc.003n>
		if (kOther.isBarbarian())
			continue; // </advc.003n>
		kOther.setWarWeariness(getID(), (iOriginalTeamSize *
				kOther.getWarWeariness(getID()) + iSecondTeamSize *
				kOther.getWarWeariness(eTeam)) / getNumMembers());
		kOther.AI_setDefensivePactCounter(getID(), (iOriginalTeamSize *
				kOther.AI_getDefensivePactCounter(getID()) + iSecondTeamSize *
				kOther.AI_getDefensivePactCounter(eTeam)) / getNumMembers());
		kOther.AI_setShareWarCounter(getID(), (iOriginalTeamSize *
				kOther.AI_getShareWarCounter(getID()) + iSecondTeamSize *
				kOther.AI_getShareWarCounter(eTeam)) / getNumMembers());
		kOther.AI_setWarSuccess(getID(), (iOriginalTeamSize *
				kOther.AI_getWarSuccess(getID()) + iSecondTeamSize *
				kOther.AI_getWarSuccess(eTeam)) / getNumMembers());
		// <advc.130m>
		kOther.AI_setSharedWarSuccess(getID(), (iOriginalTeamSize *
				kOther.AI_getSharedWarSuccess(getID()) + iSecondTeamSize *
				kOther.AI_getSharedWarSuccess(eTeam)) / getNumMembers());
		// </advc.130m>
		kOther.setEspionagePointsAgainstTeam(getID(),
				kOther.getEspionagePointsAgainstTeam(getID()) +
				kOther.getEspionagePointsAgainstTeam(eTeam));
		// <advc.003n>
		if (kOther.isMinorCiv())
			continue; // </advc.003n>
		kOther.AI_setEnemyPeacetimeTradeValue(getID(), (iOriginalTeamSize *
				kOther.AI_getEnemyPeacetimeTradeValue(getID()) + iSecondTeamSize *
				kOther.AI_getEnemyPeacetimeTradeValue(eTeam)) / getNumMembers());
		kOther.AI_setEnemyPeacetimeGrantValue(getID(), (iOriginalTeamSize *
				kOther.AI_getEnemyPeacetimeGrantValue(getID()) + iSecondTeamSize *
				kOther.AI_getEnemyPeacetimeGrantValue(eTeam)) / getNumMembers());
		// </kekm.26>

		if (kOther.isAlive())
		{
			kOther.AI_setWarPlan(getID(), NO_WARPLAN, false);
			kOther.AI_setWarPlan(eTeam, NO_WARPLAN, false);
			// advc.001: Cancel our war plans too (eTeam's is taken care of when it dies)
			AI().AI_setWarPlan(kOther.getID(), NO_WARPLAN, false);
		}
	}

	AI().AI_updateWorstEnemy();
	// <advc.104t>
	if(getUWAI.isEnabled())
		AI().uwai().addTeam(eTeamLeader);
	// </advc.104t>
	AI().AI_updateAreaStrategies();

	kGame.updateScore(true);
}


void CvTeam::shareItems(TeamTypes eTeam)
{
	FAssert(eTeam != getID());

	FOR_EACH_ENUM(Tech)
	{
		if (GET_TEAM(eTeam).isHasTech(eLoopTech))
		{	// <kekm.26> "Preserve no tech brokering status."
			setNoTradeTech(eLoopTech, (!isHasTech(eLoopTech) || isNoTradeTech(eLoopTech)) &&
					GET_TEAM(eTeam).isNoTradeTech(eLoopTech)); // </kekm.26>
			setHasTech(eLoopTech, true, NO_PLAYER, true, false);
		}
	}
	/*  <kekm.26> "Other direction also done here as other direction of shareItems
		is not used anymore." */
	FOR_EACH_ENUM(Tech)
	{
		if (isHasTech(eLoopTech))
		{
			GET_TEAM(eTeam).setNoTradeTech(eLoopTech,
					(!GET_TEAM(eTeam).isHasTech(eLoopTech) ||
					GET_TEAM(eTeam).isNoTradeTech(eLoopTech)) &&
					isNoTradeTech(eLoopTech));
			GET_TEAM(eTeam).setHasTech(eLoopTech, true, NO_PLAYER, true, false);
		}
	} // </kekm.26>

	FOR_EACH_ENUM(Bonus)
	{
		if (GET_TEAM(eTeam).isForceRevealedBonus(eLoopBonus))
			setForceRevealedBonus(eLoopBonus, true);
	}

	/*  <kekm.26> "Other direction also done here as other direction of shareItems
		is not used anymore." */
	FOR_EACH_ENUM(Bonus)
	{
		if (isForceRevealedBonus(eLoopBonus))
			GET_TEAM(eTeam).setForceRevealedBonus(eLoopBonus, true);
	} // </kekm.26>

	for (int i = 0; i < MAX_TEAMS; i++)
	{
		TeamTypes eLoopTeam = (TeamTypes)i;
		//setEspionagePointsAgainstTeam(eLoopTeam, std::max(GET_TEAM(eTeam).getEspionagePointsAgainstTeam(eLoopTeam), getEspionagePointsAgainstTeam(eLoopteam)));
		// <kekm.26> "Espionage is now sum instead of max."
		setEspionagePointsAgainstTeam(eLoopTeam,
				GET_TEAM(eTeam).getEspionagePointsAgainstTeam(eLoopTeam) +
				getEspionagePointsAgainstTeam(eLoopTeam)); // </kekm.26>
	}
	//setEspionagePointsEver(std::max(GET_TEAM(eTeam).getEspionagePointsEver(), getEspionagePointsEver())); // K-Mod
	// kekm.26: Replacing the above
	setEspionagePointsEver(GET_TEAM(eTeam).getEspionagePointsEver() + getEspionagePointsEver());

	for(int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayer const& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
		if(!kLoopPlayer.isAlive() || kLoopPlayer.getTeam() != eTeam)
			continue;

		CvCivilization const& kLoopCiv = kLoopPlayer.getCivilization();
		FOR_EACH_CITY(pLoopCity, kLoopPlayer)
		{
			for (int j = 0; j < kLoopCiv.getNumBuildings(); j++)
			{
				BuildingTypes eBuilding = kLoopCiv.buildingAt(j);
				int iCityBuildings = pLoopCity->getNumBuilding(eBuilding);
				if(iCityBuildings <= 0 || isObsoleteBuilding(eBuilding))
					continue;
				if(GC.getInfo(eBuilding).isTeamShare())
				{
					for(int k = 0; k < MAX_PLAYERS; k++)
					{
						if(GET_PLAYER((PlayerTypes)k).getTeam() == getID())
						{
							GET_PLAYER((PlayerTypes)k).processBuilding(eBuilding,
									iCityBuildings, pLoopCity->getArea());
						}
					}
				}
				processBuilding(eBuilding, iCityBuildings);
			}
		}
	}
	/*  <kekm.26> "Other direction also done here as other direction of shareItems
		is not used anymore." */
	for(int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayer const& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
		if(!kLoopPlayer.isAlive() || kLoopPlayer.getTeam() != getID())
			continue;

		CvCivilization const& kLoopCiv = kLoopPlayer.getCivilization();
		FOR_EACH_CITY(pLoopCity, kLoopPlayer)
		{
			for (int j = 0; j < kLoopCiv.getNumBuildings(); j++)
			{
				BuildingTypes eBuilding = kLoopCiv.buildingAt(j);
				int iCityBuildings = pLoopCity->getNumBuilding(eBuilding);
				if(iCityBuildings <= 0 || isObsoleteBuilding(eBuilding))
					continue;
				if(GC.getInfo(eBuilding).isTeamShare())
				{
					for(int k = 0; k < MAX_PLAYERS; k++)
					{
						if(GET_PLAYER((PlayerTypes)k).getTeam() == eTeam)
						{
							GET_PLAYER((PlayerTypes)k).processBuilding(eBuilding,
									iCityBuildings, pLoopCity->getArea());
						}
					}
				}
				GET_TEAM(eTeam).processBuilding(eBuilding, iCityBuildings);
			}
		}
	} // </kekm.26>

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
		if (!kLoopPlayer.isAlive())
			continue;
		if (kLoopPlayer.getTeam() == eTeam ||
			/*  kekm.26: "Other direction also done here as other direction
				of shareItems is not used anymore." */
			kLoopPlayer.getTeam() == getID())
		{
			kLoopPlayer.AI_updateBonusValue();
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

			if (kShareTeam.AI_getAtWarCounter(eLoopTeam) > AI().AI_getAtWarCounter(eLoopTeam))
				AI().AI_setAtWarCounter(eLoopTeam, kShareTeam.AI_getAtWarCounter(eLoopTeam));
			//else kShareTeam.AI_setAtWarCounter(eLoopTeam, AI_getAtWarCounter(eLoopTeam));

			if (kShareTeam.AI_getAtPeaceCounter(eLoopTeam) > AI().AI_getAtPeaceCounter(eLoopTeam))
				AI().AI_setAtPeaceCounter(eLoopTeam, kShareTeam.AI_getAtPeaceCounter(eLoopTeam));
			//else kShareTeam.AI_setAtPeaceCounter(eLoopTeam, AI_getAtPeaceCounter(eLoopTeam));

			if (kShareTeam.AI_getHasMetCounter(eLoopTeam) > AI().AI_getHasMetCounter(eLoopTeam))
				AI().AI_setHasMetCounter(eLoopTeam, kShareTeam.AI_getHasMetCounter(eLoopTeam));
			//else kShareTeam.AI_setHasMetCounter(eLoopTeam, AI_getHasMetCounter(eLoopTeam));

			if (kShareTeam.AI_getOpenBordersCounter(eLoopTeam) > AI().AI_getOpenBordersCounter(eLoopTeam))
				AI().AI_setOpenBordersCounter(eLoopTeam, kShareTeam.AI_getOpenBordersCounter(eLoopTeam));
			//else kShareTeam.AI_setOpenBordersCounter(eLoopTeam, AI_getOpenBordersCounter(eLoopTeam));

			if (kShareTeam.AI_getDefensivePactCounter(eLoopTeam) > AI().AI_getDefensivePactCounter(eLoopTeam))
				AI().AI_setDefensivePactCounter(eLoopTeam, kShareTeam.AI_getDefensivePactCounter(eLoopTeam));
			//else kShareTeam.AI_setDefensivePactCounter(eLoopTeam, AI_getDefensivePactCounter(eLoopTeam));

			if (kShareTeam.AI_getShareWarCounter(eLoopTeam) > AI().AI_getShareWarCounter(eLoopTeam))
				AI().AI_setShareWarCounter(eLoopTeam, kShareTeam.AI_getShareWarCounter(eLoopTeam));
			//else kShareTeam.AI_setShareWarCounter(eLoopTeam, AI_getShareWarCounter(eLoopTeam));

			if (kShareTeam.AI_getWarSuccess(eLoopTeam) > AI().AI_getWarSuccess(eLoopTeam))
				AI().AI_setWarSuccess(eLoopTeam, kShareTeam.AI_getWarSuccess(eLoopTeam));
			//else kShareTeam.AI_setWarSuccess(eLoopTeam, AI_getWarSuccess(eLoopTeam));

			if (kShareTeam.AI_getEnemyPeacetimeTradeValue(eLoopTeam) > AI().AI_getEnemyPeacetimeTradeValue(eLoopTeam))
				AI().AI_setEnemyPeacetimeTradeValue(eLoopTeam, kShareTeam.AI_getEnemyPeacetimeTradeValue(eLoopTeam));
			//else kShareTeam.AI_setEnemyPeacetimeTradeValue(eLoopTeam, AI_getEnemyPeacetimeTradeValue(eLoopTeam));

			if (kShareTeam.AI_getEnemyPeacetimeGrantValue(eLoopTeam) > AI().AI_getEnemyPeacetimeGrantValue(eLoopTeam))
				AI().AI_setEnemyPeacetimeGrantValue(eLoopTeam, kShareTeam.AI_getEnemyPeacetimeGrantValue(eLoopTeam));
			//else kShareTeam.AI_setEnemyPeacetimeGrantValue(eLoopTeam, AI_getEnemyPeacetimeGrantValue(eLoopTeam));

			kShareTeam.AI_setWarPlan(eLoopTeam, NO_WARPLAN, false);
			// K-Mod note. presumably, the warplan is cleared under the assumption that kShareTeam is going to be removed.
		}
	}

	FOR_EACH_ENUM2(Project, eProject)
	{
		/* int iExtraProjects = kShareTeam.getProjectCount(eProject) - getProjectCount(eProject);
		if (iExtraProjects > 0) {
			changeProjectCount(eProject, iExtraProjects);
			kGame.incrementProjectCreatedCount(eProject, -iExtraProjects);
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

	FOR_EACH_ENUM(UnitClass)
	{
		changeUnitClassCount(eLoopUnitClass, kShareTeam.getUnitClassCount(eLoopUnitClass));
		//kShareTeam.changeUnitClassCount(eLoopUnitClass, getUnitClassCount(eLoopUnitClass) - kShareTeam.getUnitClassCount(eLoopUnitClass));
	}

	FOR_EACH_ENUM(BuildingClass)
	{
		changeBuildingClassCount(eLoopBuildingClass,
				kShareTeam.getBuildingClassCount(eLoopBuildingClass));
		//kShareTeam.changeBuildingClassCount(eLoopBuildingClass, getBuildingClassCount(eLoopBuildingClass) - kShareTeam.getBuildingClassCount(eLoopBuildingClass));
	}

	FOR_EACH_ENUM2(Tech, eTech)
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
			if (kShareTeam.getResearchProgress(eTech) * getResearchCost(eTech) >
				getResearchProgress(eTech) * kShareTeam.getResearchCost(eTech))
			{
				setResearchProgress(eTech,
						kShareTeam.getResearchProgress(eTech) * getResearchCost(eTech) /
						std::max(1, kShareTeam.getResearchCost(eTech)), getLeaderID());
			}
			//else kShareTeam.setResearchProgress(eTech, getResearchProgress(eTech) * kShareTeam.getResearchCost(eTech) / std::max(1, getResearchCost(eTech)), kShareTeam.getLeaderID());
		}

		/*if (isHasTech(eTech) && !isNoTradeTech(eTech)))
			kShareTeam.setNoTradeTech((eTech), false);*/
		// unofficial patch
		/*if (kShareTeam.isHasTech(eTech) && !(kShareTeam.isNoTradeTech(eTech)))
			setNoTradeTech((eTech), false);*/
		/*  kekm.26 (commented out): "What was this even supposed to be doing?
			No tech brokering is now applied if necessary when tech is shared
			in CvTeam::shareItems. */
	}

	// K-Mod. Share extra moves.
	// Note: there is no reliable way to do this. We can't tell if the bonus is from something unique- such as circumnavigation,
	// or from something that is already taken into account - such as refrigeration.
	FOR_EACH_ENUM(Domain)
	{
		if (kShareTeam.getExtraMoves(eLoopDomain) > getExtraMoves(eLoopDomain))
		{
			changeExtraMoves(eLoopDomain,
					kShareTeam.getExtraMoves(eLoopDomain) - getExtraMoves(eLoopDomain));
		}
	}
	// K-Mod end
}


void CvTeam::processBuilding(BuildingTypes eBuilding, int iChange)
{
	FOR_EACH_ENUM(VoteSource)
	{
		if (GC.getInfo(eBuilding).getVoteSourceType() == eLoopVoteSource)
		{
			changeForceTeamVoteEligibilityCount(eLoopVoteSource,
					GC.getInfo(eBuilding).isForceTeamVoteEligible() ? iChange : 0);
		}
	}

	if (GC.getInfo(eBuilding).isMapCentering())
	{
		if (iChange > 0)
			setMapCentering(true);
	}

	changeEnemyWarWearinessModifier(GC.getInfo(eBuilding).getEnemyWarWearinessModifier() * iChange);
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
	AI().AI_doTurnPre();
	m_abJustDeclaredWar.reset(); // advc.162
	if (isBarbarian())
		doBarbarianResearch(); // advc: Moved into subroutine

	for (TeamIter<CIV_ALIVE,NOT_SAME_TEAM_AS> it(getID()); it.hasNext(); ++it)
	{
		TeamTypes eOther = it->getID();
		if (getStolenVisibilityTimer(eOther) > 0)
			changeStolenVisibilityTimer(eOther, -1);
		if (getCounterespionageTurnsLeftAgainstTeam(eOther) > 0)
			changeCounterespionageTurnsLeftAgainstTeam(eOther, -1);
		if (getCounterespionageTurnsLeftAgainstTeam(eOther) == 0)
			setCounterespionageModAgainstTeam(eOther, 0);
	}

	if (!GC.getGame().isOption(GAMEOPTION_NO_TECH_BROKERING))
	{
		FOR_EACH_ENUM(Tech)
			setNoTradeTech(eLoopTech, false);
	}

	doWarWeariness();

	// advc.136a: Moved to CvPlayer::doTurn
	//testCircumnavigated(); // K-Mod note: is it a bit unfair to test circumnavigation in this function?

	AI().AI_doTurnPost();
}


void CvTeam::updateYield()
{
	for (MemberIter it(getID()); it.hasNext(); ++it)
		it->updateYield();
}


void CvTeam::updatePowerHealth()
{
	for (MemberIter it(getID()); it.hasNext(); ++it)
		it->updatePowerHealth();
}


void CvTeam::updateCommerce()
{
	for (MemberIter it(getID()); it.hasNext(); ++it)
		it->updateCommerce();
}


bool CvTeam::canChangeWarPeace(TeamTypes eTeam, bool bAllowVassal) const
{
	if (GC.getGame().isOption(GAMEOPTION_NO_CHANGING_WAR_PEACE))
		return false;

	if (eTeam == getID())
		return false;

	if (isPermanentWarPeace(eTeam) || GET_TEAM(eTeam).isPermanentWarPeace(getID()))
		return false;

	if (isAVassal())
		return false;
	if (bAllowVassal)
	{
		if (GET_TEAM(eTeam).isVassal(getID()))
			return false;
	}
	else
	{
		if (GET_TEAM(eTeam).isAVassal())
			return false;
	}
	// advc.opt: moved down
	for (TeamIter<CIV_ALIVE,VASSAL_OF> it(getID()); it.hasNext(); ++it)
	{
		if (it->isPermanentWarPeace(eTeam))
			return false;
	}
	for (TeamIter<CIV_ALIVE,VASSAL_OF> it(eTeam); it.hasNext(); ++it)
	{
		if (it->isPermanentWarPeace(getID()))
			return false;
	}
	/*  <advc.001> Had a civ make peace with a minor civ in one game. Not sure how
		that happened; probably through a random event. */
	if(isMinorCiv() || GET_TEAM(eTeam).isMinorCiv())
		return false; // </advc.001>
	// <advc.104> Don't want to have to check this separately in the UWAI code
	if(GC.getGame().isOption(GAMEOPTION_ALWAYS_WAR) && isAtWar(eTeam) &&
		(isHuman() || GET_TEAM(eTeam).isHuman()))
	{
		return false;
	} // </advc.104>

	return true;
}

// K-Mod, I've removed the bulk of this function and replaced it with just a call to "canEventuallyDeclareWar",
// which contains all of the original checks. I've done this to reduce code dupliation.
bool CvTeam::canDeclareWar(TeamTypes eTeam) const
{
	if (!canEventuallyDeclareWar(eTeam))
		return false;

	// advc: Simplified this check
	if (isForcePeace(GET_TEAM(eTeam).getMasterTeam()) || isForcePeace(eTeam))
		return false;

	return true;
}

// BETTER_BTS_AI_MOD, 01/16/10, jdog5000, War Strategy AI:
bool CvTeam::canEventuallyDeclareWar(TeamTypes eTeam) const
{
	return (eTeam != getID() && isAlive() && GET_TEAM(eTeam).isAlive() &&
			!isAtWar(eTeam) && isHasMet(eTeam) && canChangeWarPeace(eTeam, true) &&
			!GC.getGame().isOption(GAMEOPTION_ALWAYS_PEACE) &&
			GC.getPythonCaller()->canDeclareWar(getID(), eTeam));
}

// K-Mod note: I've shuffled things around a bit in this function.  // advc: refactored
void CvTeam::declareWar(TeamTypes eTarget, bool bNewDiplo, WarPlanTypes eWarPlan,
	bool bPrimaryDoW, // K-Mod
	PlayerTypes eSponsor, // advc.100
	bool bRandomEvent) // advc.106g
{
	PROFILE_FUNC();
	FAssert(eTarget != NO_TEAM);
	FAssert(eTarget != getID());
	// <advc.100>
	FAssert(eSponsor == NO_PLAYER || (TEAMID(eSponsor) != getID() &&
			TEAMID(eSponsor) != eTarget)); // </advc.100>
	if (isAtWar(eTarget))
		return;
	if (gTeamLogLevel >= 1) logBBAI("  Team %d (%S) declares war on team %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eTarget); // BETTER_BTS_AI_MOD (10/02/09, jdog5000): AI logging
	CvTeam& kTarget = GET_TEAM(eTarget);
	std::vector<CvPlayer*> kMembers; // advc: of either team
	for (MemberIter it(getID()); it.hasNext(); ++it)
		kMembers.push_back(&*it);
	for (MemberIter it(eTarget); it.hasNext(); ++it)
		kMembers.push_back(&*it);
	{
		bool bFirst = true; // advc.002l
		FOR_EACH_DEAL_VAR(pLoopDeal)
		{
			if (pLoopDeal->isBetween(getID(), eTarget))
			{
				pLoopDeal->kill(true, NO_PLAYER, /* <advc.002l> */ !bFirst);
				bFirst = false; // </advc.002l>
			}
		}
	}
	for (size_t i = 0; i < kMembers.size(); i++)
		kMembers[i]->updatePlunder(-1, false);

	// advc: AI code moved into new function
	AI().AI_preDeclareWar(eTarget, eWarPlan, bPrimaryDoW, eSponsor);

	setAtWar(eTarget, true);
	kTarget.setAtWar(getID(), true);
	// <advc.162>
	if(GC.getDefineBOOL(CvGlobals::ENABLE_162))
		m_abJustDeclaredWar.set(eTarget, true); // </advc.162>
	// BETTER_BTS_AI_MOD (08/21/09, jdog5000, Efficiency): START
	GC.getMap().invalidateBorderDangerCache(eTarget);
	GC.getMap().invalidateBorderDangerCache(getID());
	// BETTER_BTS_AI_MOD: END
	for (size_t i = 0; i < kMembers.size(); i++)
		kMembers[i]->updatePlunder(1, false);

	meet(eTarget, false);

	// advc: AI code moved into new function
	AI().AI_postDeclareWar(eTarget, eWarPlan);

	GC.getMap().verifyUnitValidPlot();

	for (MemberIter it(getID()); it.hasNext(); ++it)
		it->verifyUnitStacksValid();

	GC.getGame().AI_makeAssignWorkDirty();

	if (getID() == GC.getGame().getActiveTeam() || eTarget == GC.getGame().getActiveTeam())
	{
		gDLL->UI().setDirty(Score_DIRTY_BIT, true);
		gDLL->UI().setDirty(CityInfo_DIRTY_BIT, true);
		// advc.001w: Can now enter each other's territory
		gDLL->UI().setDirty(Waypoints_DIRTY_BIT, true);
		// advc.162: DoW increases certain path costs
		CvSelectionGroup::resetPath();
	}
	// advc.003j: Obsolete
	/*for (iI = 0; iI < MAX_PLAYERS; iI++) {
		if (GET_PLAYER((PlayerTypes)iI).isAlive()) {
			for (int iJ = 0; iJ < MAX_PLAYERS; iJ++) {
				if (GET_PLAYER((PlayerTypes)iJ).isAlive()) {
					if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID() && GET_PLAYER((PlayerTypes)iJ).getTeam() == eTarget) {
						GET_PLAYER((PlayerTypes)iI).AI_setFirstContact(((PlayerTypes)iJ), true);
						GET_PLAYER((PlayerTypes)iJ).AI_setFirstContact(((PlayerTypes)iI), true);
	} } } } }*/
	for (size_t i = 0; i < kMembers.size(); i++)
		kMembers[i]->updateWarWearinessPercentAnger();
	for (size_t i = 0; i < kMembers.size(); i++)
		kMembers[i]->updatePlotGroups();
	for (size_t i = 0; i < kMembers.size(); i++)
		kMembers[i]->updateTradeRoutes();

	if (GC.getGame().isFinalInitialized() && !gDLL->GetWorldBuilderMode() &&
		!isBarbarian() && !kTarget.isBarbarian() && !isMinorCiv() && !kTarget.isMinorCiv()) // advc: Moved these checks up
	{
		if (bNewDiplo && !isHuman())
		{
			for (PlayerIter<HUMAN,MEMBER_OF> it(eTarget); it.hasNext(); ++it)
			{
				CvPlayer const& kTargetMember = *it;
				if (GET_PLAYER(getLeaderID()).canContact(kTargetMember.getID()))
				{
					CvDiploParameters* pDiplo = new CvDiploParameters(getLeaderID());
					pDiplo->setDiploComment(GC.getAIDiploCommentType("DECLARE_WAR"));
					pDiplo->setAIContact(true);
					gDLL->beginDiplomacy(pDiplo, kTargetMember.getID());
				}
			}
		}
		CvWString szBuffer;
		// <advc.100>
		CvWString szSponsorName;
		wchar const* cpSponsorName = L"";
		if (eSponsor != NO_PLAYER)
		{
			// Need to make a copy b/c getName returns a pointer into a local string object
			szSponsorName = GET_PLAYER(eSponsor).getName();
			cpSponsorName = szSponsorName.GetCString();
		} // </advc.100>
		for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
		{
			CvPlayer const& kObs = *it;
			// <advc.002l>
			LPCTSTR szSoundYou = (bPrimaryDoW ? "AS2D_DECLAREWAR" : NULL);
			LPCTSTR szSoundThey = (bPrimaryDoW ? "AS2D_THEIRDECLAREWAR" : NULL);
			// </advc.002l>
			if (kObs.getTeam() == getID())
			{
				szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_DECLARED_WAR_ON",
						kTarget.getName().GetCString());
				gDLL->UI().addMessage(kObs.getID(), true, -1, szBuffer,
						szSoundYou, // advc.002l
						MESSAGE_TYPE_MAJOR_EVENT, NULL, GC.getColorType("WARNING_TEXT"),
						// advc.127b:
						kTarget.getCapitalX(kObs.getTeam()), kTarget.getCapitalY(kObs.getTeam()));
			}
			else if(kObs.getTeam() == eTarget)
			{	// <advc.100> Inform the target of the DoW about the sponsor
				if(eSponsor != NO_PLAYER)
					szBuffer = gDLL->getText("TXT_KEY_MISC_HIRED_WAR_ON_YOU",
							getName().GetCString(), cpSponsorName);
				else // </advc.100>
					szBuffer = gDLL->getText("TXT_KEY_MISC_DECLARED_WAR_ON_YOU", getName().GetCString());
				gDLL->UI().addMessage(kObs.getID(), true, -1, szBuffer,
						szSoundYou, // advc.002l
						MESSAGE_TYPE_MAJOR_EVENT, NULL, GC.getColorType("WARNING_TEXT"),
						// advc.127b:
						getCapitalX(kObs.getTeam()), getCapitalY(kObs.getTeam()));
			}
			else if((isHasMet(kObs.getTeam()) && kTarget.isHasMet(kObs.getTeam())) ||
				kObs.isSpectator()) // advc.127
			{	// <advc.100> Inform third parties about sponsor
				if(eSponsor != NO_PLAYER && eSponsor != kObs.getID() &&
					(GET_TEAM(eSponsor).isHasMet(kObs.getTeam()) ||
					kObs.isSpectator())) // advc.127
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_HIRED_WAR",
							getName().GetCString(), kTarget.getName().GetCString(), cpSponsorName);
				}
				else // </advc.100>
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_DECLARED_WAR",
							getName().GetCString(), kTarget.getName().GetCString());
				}
				gDLL->UI().addMessage(kObs.getID(), false, -1, szBuffer,
						szSoundThey, // advc.002l
						// <advc.106b>
						(isAVassal() || kTarget.isAVassal() ?
						MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY : // </advc.106b>
						MESSAGE_TYPE_MAJOR_EVENT), NULL, GC.getColorType("WARNING_TEXT"),
						// advc.127b:
						getCapitalX(kObs.getTeam(), true), getCapitalY(kObs.getTeam(), true));
			}
		}
		// <advc.100> Put info about hired wars in the replay log
		if (eSponsor != NO_PLAYER)
		{
			szSponsorName = GET_PLAYER(eSponsor).getReplayName();
			cpSponsorName = szSponsorName.GetCString();
			szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_HIRED_WAR",
					getReplayName().GetCString(),
					kTarget.getReplayName().GetCString(), cpSponsorName);
		}
		else // </advc.100>
		{	// <advc.106g>
			if (bRandomEvent)
			{
				szBuffer = gDLL->getText("TXT_KEY_MISC_WAR_VIA_EVENT",
						getReplayName().GetCString(), kTarget.getReplayName().GetCString());
			}
			else // </advc.106g>
			{
				szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_DECLARES_WAR",
						getReplayName().GetCString(), kTarget.getReplayName().GetCString());
			}
		}
		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(), szBuffer,
				-1, -1, GC.getColorType("WARNING_TEXT"));
	}
	// kekm.26 (advc): EventReporter call moved down
	triggerDefensivePacts(eTarget, bNewDiplo, bPrimaryDoW); // advc: Moved into subroutine
	for (TeamIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		CvTeam& kThirdTeam = *it;
		if (kThirdTeam.getID() == getID() || kThirdTeam.getID() == eTarget)
			continue;
		if (kThirdTeam.isVassal(eTarget) || kTarget.isVassal(kThirdTeam.getID()))
		{
			//declareWar(kThirdTeam.getID(), bNewDiplo, AI_getWarPlan(eTeam), false);
			// kekm.26:
			queueWar(getID(), kThirdTeam.getID(), bNewDiplo, AI().AI_getWarPlan(eTarget), false);
		}
		else if (kThirdTeam.isVassal(getID()) || isVassal(kThirdTeam.getID()))
		{
			//kThirdTeam.declareWar(eTeam, bNewDiplo, WARPLAN_DOGPILE, false);
			// kekm.26:
			queueWar(kThirdTeam.getID(), eTarget, bNewDiplo, WARPLAN_DOGPILE, false);
		}
	}
	/*if (bPrimaryDoW) { // K-Mod. update attitude
		for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
			GET_PLAYER(i).AI_updateAttitude();
	}*/
	// <kekm.26> The above is "updated when the war queue is emptied."
	/*  advc (bugfix): But not unless this function communicates to triggerWars that
		a (primary) DoW has already occurred. */
	triggerWars(true);
	// advc: Moved down so that war status has already changed when event is reported
	CvEventReporter::getInstance().changeWar(true, getID(), eTarget); // </kekm.26>
}

// advc: Cut from declareWar
void CvTeam::triggerDefensivePacts(TeamTypes eTarget, bool bNewDiplo, bool bPrimary)
{
	/*	advc (kekm3): New vassal joining its master's wars shouldn't trigger DP.
		Master should already be at war with all DP allies of its enemies, but,
		at least in multiplayer, this isn't guaranteed (I think). */
	if (isAVassal())
		return;
	/*  K-Mod / BBAI. Customization options from BBAI have been modified, so that
		they use "bPrimary". The original BtS code has been deleted. */

	/* kekm.3: 'BBAI option 1 didn't work because if clauses for canceling pacts
	   were wrong. BBAI otpion 2 needs further fixing. When all players have
	   defensive pacts with all other players and someone declares war the
	   correct behaviour would be to have all attack the inital attacker,
	   but additional wars are declared due to recursive calls of declareWar
	   in the loop below.' */
	int const iDPBehavior = GC.getDefineINT(CvGlobals::BBAI_DEFENSIVE_PACT_BEHAVIOR);
	if(iDPBehavior == 0 || (iDPBehavior == 1 && bPrimary))
		cancelDefensivePacts();
	CvTeamAI& kTarget = GET_TEAM(eTarget);
	bool bDefPactTriggered = false; // advc.104i
	for (TeamIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		CvTeam& kThirdTeam = *it;
		if (kThirdTeam.getID() == getID() || kThirdTeam.getID() == eTarget)
			continue;
		// <kekm.3> Ally can already be at war with the aggressor
		if(kThirdTeam.isAtWar(eTarget))
			continue; // </kekm.3>
		if (kThirdTeam.isDefensivePact(eTarget))
		{
			FAssert(!kTarget.isAVassal() && !kThirdTeam.isAVassal());
			//kThirdTeam.declareWar(getID(), bNewDiplo, WARPLAN_DOGPILE, false);
			// kekm.26:
			queueWar(kThirdTeam.getID(), getID(), bNewDiplo, WARPLAN_DOGPILE, false);
			// <advc.104i>
			bDefPactTriggered = true;
			if(!isAVassal())
			{ /* Team iI declares war on us, and this makes our team
				 unwilling to talk to both iI and its ally eTeam. */
				AI().AI_makeUnwillingToTalk(eTarget);
				AI().AI_makeUnwillingToTalk(kThirdTeam.getID());
			} // </advc.104i>
		}
		else if (iDPBehavior > 1 && kThirdTeam.isDefensivePact(getID()))
		{	// For alliance option.  This teams pacts are canceled above if not using alliance option.
			//kThirdTeam.declareWar(eTeam, bNewDiplo, WARPLAN_DOGPILE, false);
			// kekm.26:
			queueWar(kThirdTeam.getID(), eTarget, bNewDiplo, WARPLAN_DOGPILE, false);
		}
	}
	if (iDPBehavior == 0)// kekm.3: || (iDPBehavior == 1 && bPrimaryDoW))
		kTarget.cancelDefensivePacts();
	// K-Mod / BBAI end.
	kTarget.allowDefensivePactsToBeCanceled(); // kekm.3
	/*  <advc.104i> When other teams come to the help of eTeam through a
		defensive pact, then eTeam becomes unwilling to talk with us. */
	if(bDefPactTriggered)
		kTarget.AI_makeUnwillingToTalk(getID()); // </advc.104i>
}


void CvTeam::makePeace(TeamTypes eTarget, bool bBumpUnits,  // advc: refactored
	TeamTypes eBroker, // advc.100b
	bool bCapitulate, // advc.034
	CLinkList<TradeData> const* pReparations, // advc.039
	bool bRandomEvent) // advc.106g
{
	FAssert(eTarget != NO_TEAM);
	FAssert(eTarget != getID());
	if (!isAtWar(eTarget))
		return;
	CvTeam& kTarget = GET_TEAM(eTarget);
	std::vector<CvPlayer*> kMembers; // advc: of either team
	for (MemberIter it(getID()); it.hasNext(); ++it)
		kMembers.push_back(&*it);
	for (MemberIter it(eTarget); it.hasNext(); ++it)
		kMembers.push_back(&*it);

	AI().AI_preMakePeace(eTarget, pReparations); // advc: AI code moved into new function

	if (gTeamLogLevel >= 1) logBBAI("    Team %d (%S) and team %d (%S) make peace", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eTarget, GET_PLAYER(kTarget.getLeaderID()).getCivilizationDescription(0)); // BETTER_BTS_AI_MOD, AI logging, 05/21/10, jdog5000

	for (size_t i = 0; i < kMembers.size(); i++)
		kMembers[i]->updatePlunder(-1, false);

	setAtWar(eTarget, false);
	kTarget.setAtWar(getID(), false);

	for (size_t i = 0; i < kMembers.size(); i++)
		kMembers[i]->updatePlunder(1, false);

	// BETTER_BTS_AI_MOD, Efficiency: plot danger cache, 08/21/09, jdog5000: START
	GC.getMap().invalidateBorderDangerCache(eTarget);
	GC.getMap().invalidateBorderDangerCache(getID());
	// BETTER_BTS_AI_MOD: END
	// <advc.034>
	if(!bCapitulate && GC.getDefineINT(CvGlobals::DISENGAGE_LENGTH) > 0)
		signDisengage(eTarget); // </advc.034>

	if (bBumpUnits)
		GC.getMap().verifyUnitValidPlot();

	GC.getGame().AI_makeAssignWorkDirty();

	if (getID() == GC.getGame().getActiveTeam() || eTarget == GC.getGame().getActiveTeam())
	{
		gDLL->UI().setDirty(Score_DIRTY_BIT, true);
		gDLL->UI().setDirty(CityInfo_DIRTY_BIT, true);
	}
	for (size_t i = 0; i < kMembers.size(); i++)
		kMembers[i]->updateWarWearinessPercentAnger();
	for (size_t i = 0; i < kMembers.size(); i++)
		kMembers[i]->updatePlotGroups();
	for (size_t i = 0; i < kMembers.size(); i++)
		kMembers[i]->updateTradeRoutes();
	// advc: AI code moved down a bit and then into a new function
	AI().AI_postMakePeace(eTarget);

	for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
	{
		CvPlayer const& kObs = *it;
		// <advc.002l>
		LPCTSTR szSoundYou = "AS2D_MAKEPEACE";
		LPCTSTR szSoundThey = "AS2D_THEIRMAKEPEACE";
		if (isAVassal() || kTarget.isAVassal())
			szSoundYou = szSoundThey = NULL; // </advc.002l>
		bool bWarTeam = false; // advc.039
		if (kObs.getTeam() == getID())
		{
			CvWString szBuffer(gDLL->getText("TXT_KEY_MISC_YOU_MADE_PEACE_WITH",
					kTarget.getName().GetCString()));
			gDLL->UI().addMessage(kObs.getID(), true, -1, szBuffer,
					szSoundYou, // advc.002l
					MESSAGE_TYPE_MAJOR_EVENT, NULL, GC.getColorType("HIGHLIGHT_TEXT"),
					// advc.127b:
					kTarget.getCapitalX(kObs.getTeam(), true), kTarget.getCapitalY(kObs.getTeam(), true));
			bWarTeam = true; // advc.039
		}
		else if (kObs.getTeam() == eTarget)
		{
			CvWString szBuffer(gDLL->getText("TXT_KEY_MISC_YOU_MADE_PEACE_WITH",
					getName().GetCString()));
			gDLL->UI().addMessage(kObs.getID(), true, -1, szBuffer,
					szSoundYou, // advc.002l
					MESSAGE_TYPE_MAJOR_EVENT, NULL, GC.getColorType("HIGHLIGHT_TEXT"),
					// advc.127b:
					getCapitalX(kObs.getTeam()), getCapitalY(kObs.getTeam()));
			/*  <advc.039> Show message about reparations also to non-leading
				members of a (human) team (in addition to YOU_MADE_PEACE) */
			bWarTeam = true;
		}
		//else
		if ((!bWarTeam || (pReparations != NULL && kObs.getID() != getLeaderID() &&
			kObs.getID() != kTarget.getLeaderID())) &&  // </advc.039>
			((isHasMet(kObs.getTeam()) && kTarget.isHasMet(kObs.getTeam())) ||
			kObs.isSpectator())) // advc.127
		{
			// <advc.039>
			CvWString szBuffer;
			bool bAnyReparations = false;
			if (pReparations != NULL)
			{
				szBuffer = gDLL->getText("TXT_KEY_MISC_PEACE_IN_EXCHANGE",
						getName().GetCString(), kTarget.getName().GetCString()) + L" ";
				std::vector<CvWString> aszTradeItems;
				FOR_EACH_TRADE_ITEM(*pReparations)
				{
					CvWString const szItem(tradeItemString(
							pItem->m_eItemType, pItem->m_iData, eTarget));
					if (szItem.length() > 0)
						aszTradeItems.push_back(szItem);
				}
				// The loop above has only filtered out items w/o string representation
				for (size_t i = 0; i < aszTradeItems.size(); i++)
				{
					bAnyReparations = true;
					szBuffer += aszTradeItems[i];
					int iRemaining = ((int)aszTradeItems.size()) - i - 1;
					if (iRemaining > 0)
					{
						if(iRemaining == 1)
							szBuffer += L" " + gDLL->getText("TXT_KEY_AND") + L" ";
						else szBuffer += L", ";
					}
					else szBuffer += L".";
				} // Can handle it, but I don't think it should happen:
				FAssert(bAnyReparations);
			}
			if(!bAnyReparations) // </advc.039>
			{
				szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_MADE_PEACE",
						getName().GetCString(), kTarget.getName().GetCString());
			}
			gDLL->UI().addMessage(kObs.getID(), false, -1, szBuffer,
					szSoundThey, // advc.002l
					// <advc.106b>
					(isAVassal() || kTarget.isAVassal() ?
					MESSAGE_TYPE_MAJOR_EVENT_LOG_ONLY : // </advc.106b>
					MESSAGE_TYPE_MAJOR_EVENT), NULL,
					(bAnyReparations ? NO_COLOR : // advc.039
					GC.getColorType("HIGHLIGHT_TEXT")),
					// advc.127b:
					getCapitalX(kObs.getTeam(), true), getCapitalY(kObs.getTeam(), true));
		}
	}
	CvWString szBuffer;
	// <advc.100b>
	if (eBroker != NO_TEAM && eBroker != eTarget && eBroker != getID())
	{
		szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_BROKERED_PEACE",
				getReplayName().GetCString(),
				kTarget.getReplayName().GetCString(),
				GET_TEAM(eBroker).getReplayName().GetCString());
	}
	else // </advc.100b>
	{
		szBuffer = gDLL->getText(/* advc.106g: */ bRandomEvent ? "TXT_KEY_MISC_PEACE_VIA_EVENT" :
			"TXT_KEY_MISC_SOMEONE_MADE_PEACE", getReplayName().GetCString(),
			kTarget.getReplayName().GetCString());
	}
	GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(), szBuffer,
			-1, -1, GC.getColorType("HIGHLIGHT_TEXT"));

	CvEventReporter::getInstance().changeWar(false, getID(), eTarget);

	for (TeamIter<MAJOR_CIV> it; it.hasNext(); ++it)
	{
		CvTeam& kVassal = *it;
		if (kVassal.isVassal(eTarget))
			kVassal.makePeace(getID(), bBumpUnits);
		else if (kVassal.isVassal(getID()))
			kVassal.makePeace(eTarget, bBumpUnits);
	}
}

// K-Mod. I've added bCheckWillingness.  // advc: refactored
bool CvTeam::canContact(TeamTypes eTeam, bool bCheckWillingness) const
{
	for (MemberIter itOurMember(getID()); itOurMember.hasNext(); ++itOurMember)
	{
		for (MemberIter itTheirMember(eTeam); itTheirMember.hasNext(); ++itTheirMember)
		{
			if (itOurMember->canContact(itTheirMember->getID(), bCheckWillingness))
				return true;
		}
	}
	return false;
}


void CvTeam::meet(TeamTypes eTeam, bool bNewDiplo,
	FirstContactData* pData) // advc.071: Just passing this along
{
	if (isHasMet(eTeam))
		return;
	CvTeam& kTeam = GET_TEAM(eTeam);
	makeHasMet(eTeam, bNewDiplo, pData);
	kTeam.makeHasMet(getID(), bNewDiplo, pData);

	if (gTeamLogLevel >= 2 && GC.getGame().isFinalInitialized() && eTeam != getID() && isAlive() && GET_TEAM(eTeam).isAlive()) logBBAI("    Team %d (%S) meets team %d (%S)", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eTeam, GET_PLAYER(GET_TEAM(eTeam).getLeaderID()).getCivilizationDescription(0)); // BETTER_BTS_AI_MOD, AI logging, 02/20/10, jdog5000
	// <advc.001> Moved from makeHasMet in order to get the attitude update right
	for (TeamIter<ALIVE,VASSAL_OF> it(getID()); it.hasNext(); ++it)
		it->meet(eTeam, bNewDiplo, /* advc.071: */ pData);
	for (TeamIter<ALIVE,VASSAL_OF> it(eTeam); it.hasNext(); ++it)
		it->meet(getID(), bNewDiplo, /* advc.071: */ pData);
	if (isAVassal())
		GET_TEAM(getMasterTeam()).meet(eTeam, bNewDiplo, /* advc.071: */ pData);
	if (kTeam.isAVassal())
		GET_TEAM(kTeam.getMasterTeam()).meet(getID(), bNewDiplo, /* advc.071: */ pData);
	AI().AI_updateAttitude(eTeam);
	kTeam.AI().AI_updateAttitude(getID()); // </advc.001>
}

// advc.034: Sign a disengagement agreement (based on signOpenBorders)
void CvTeam::signDisengage(TeamTypes otherId)
{
	CvTeam& other = GET_TEAM(otherId);
	TradeData item(TRADE_DISENGAGE);
	if (!GET_PLAYER(getLeaderID()).canTradeItem(other.getLeaderID(), item) ||
		!GET_PLAYER(other.getLeaderID()).canTradeItem(getLeaderID(), item))
	{
		return;
	}
	CLinkList<TradeData> ourList;
	CLinkList<TradeData> theirList;
	ourList.insertAtEnd(item);
	theirList.insertAtEnd(item);
	GC.getGame().implementDeal(getLeaderID(), other.getLeaderID(), ourList, theirList);
}


// K-Mod:
void CvTeam::signPeaceTreaty(TeamTypes eTeam, /* advc: */ bool bForce)
{
	TradeData item(TRADE_PEACE_TREATY);
	if (/* advc: */ bForce ||
		(GET_PLAYER(getLeaderID()).canTradeItem(GET_TEAM(eTeam).getLeaderID(), item) &&
		GET_PLAYER(GET_TEAM(eTeam).getLeaderID()).canTradeItem(getLeaderID(), item)))
	{
		CLinkList<TradeData> ourList;
		CLinkList<TradeData> theirList;
		ourList.insertAtEnd(item);
		theirList.insertAtEnd(item);
		GC.getGame().implementDeal(getLeaderID(), GET_TEAM(eTeam).getLeaderID(), ourList, theirList);
	}
}


void CvTeam::signOpenBorders(TeamTypes eTeam, /* advc.032: */ bool bProlong)
{
	FAssert(eTeam != NO_TEAM);
	FAssert(eTeam != getID());

	if (!isAtWar(eTeam) && (getID() != eTeam))
	{
		TradeData item(TRADE_OPEN_BORDERS);
		if ((bProlong && isOpenBorders(eTeam)) || // advc.032
			(GET_PLAYER(getLeaderID()).canTradeItem(GET_TEAM(eTeam).getLeaderID(), item) &&
			GET_PLAYER(GET_TEAM(eTeam).getLeaderID()).canTradeItem(getLeaderID(), item)))
		{
			CLinkList<TradeData> ourList;
			CLinkList<TradeData> theirList;
			ourList.insertAtEnd(item);
			theirList.insertAtEnd(item);
			GC.getGame().implementDeal(getLeaderID(), GET_TEAM(eTeam).getLeaderID(),
					ourList, theirList, /* advc.132: */ bProlong);
		}
	}
}


void CvTeam::signDefensivePact(TeamTypes eTeam, /* advc.032: */ bool bProlong)
{
	FAssert(eTeam != getID());
	if (isAtWar(eTeam))
		return;

	TradeData item(TRADE_DEFENSIVE_PACT);
	if ((bProlong && isDefensivePact(eTeam)) || // advc.032
		(GET_PLAYER(getLeaderID()).canTradeItem(GET_TEAM(eTeam).getLeaderID(), item) &&
		GET_PLAYER(GET_TEAM(eTeam).getLeaderID()).canTradeItem(getLeaderID(), item)))
	{
		CLinkList<TradeData> ourList;
		CLinkList<TradeData> theirList;
		ourList.insertAtEnd(item);
		theirList.insertAtEnd(item);

		GC.getGame().implementDeal(getLeaderID(), GET_TEAM(eTeam).getLeaderID(),
				ourList, theirList, /* advc.132: */ bProlong);
	}
}


bool CvTeam::canSignDefensivePact(TeamTypes eTeam) /* advc: */ const
{
	for (TeamIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		CvTeam& kThirdTeam = *it;
		if (kThirdTeam.getID() == getID() || kThirdTeam.getID() == eTeam)
			continue;

		if (kThirdTeam.isPermanentWarPeace(eTeam) != kThirdTeam.isPermanentWarPeace(getID()))
			return false;
		if (isPermanentWarPeace(kThirdTeam.getID()) != GET_TEAM(eTeam).isPermanentWarPeace(kThirdTeam.getID()))
			return false;
	}
	return true;
}


int CvTeam::getAssets() const
{
	int iCount = 0;
	for (MemberIter it(getID()); it.hasNext(); ++it)
		iCount += it->getAssets();
	return iCount;
}


int CvTeam::getPower(bool bIncludeVassals) const
{
	int r = 0;

	for (MemberIter it(getID()); it.hasNext(); ++it)
		r += it->getPower();

	if (bIncludeVassals)
	{
		for (PlayerIter<ALIVE,VASSAL_OF> it(getID()); it.hasNext(); ++it)
			r += it->getPower();
	}
	return r;
}


int CvTeam::getDefensivePower(TeamTypes eExcludeTeam) const  // advc: refactored
{
	FAssert(eExcludeTeam != getID());
	FAssert(!isBarbarian()); // advc

	int r = 0;
	// K-Mod. only our master will have defensive pacts.
	CvTeam const& kOurMaster = GET_TEAM(getMasterTeam());
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		CvPlayer const& kPlayer = *it;
		if (kPlayer.getTeam() == eExcludeTeam)
			continue;
		TeamTypes eMaster = kPlayer.getMasterTeam();
		if (eMaster == kOurMaster.getID() || kOurMaster.isDefensivePact(eMaster))
			r += kPlayer.getPower();
	}
	return r;
}

// advc.003j (comment): Unused. Added by the BtS expansion; looks like it was never used.
int CvTeam::getEnemyPower() const
{
	int r = 0;
	for (TeamIter<CIV_ALIVE,ENEMY_OF> it(getID()); it.hasNext(); ++it)
		r += it->getPower(false);
	return r;
}


int CvTeam::getNumNukeUnits() const
{
	int iCount = 0;
	for (MemberIter it(getID()); it.hasNext(); ++it)
		iCount += it->getNumNukeUnits();
	for (PlayerIter<ALIVE,VASSAL_OF> it(getID()); it.hasNext(); ++it)
		iCount += it->getNumNukeUnits();
	return iCount;
}


int CvTeam::getVotes(VoteTypes eVote, VoteSourceTypes eVoteSource) const
{
	int iCount = 0;
	for (MemberIter it(getID()); it.hasNext(); ++it)
		iCount += it->getVotes(eVote, eVoteSource);
	return iCount;
}


bool CvTeam::isVotingMember(VoteSourceTypes eVoteSource) const
{
	return (getVotes(NO_VOTE, eVoteSource) > 0);
}


bool CvTeam::isFullMember(VoteSourceTypes eVoteSource) const
{
	if (isForceTeamVoteEligible(eVoteSource))
		return true;
	if (!isAlive())
		return false;
	for (MemberIter it(getID()); it.hasNext(); ++it)
	{
		if (!it->isFullMember(eVoteSource))
			return false;
	}
	return true;
}

// BETTER_BTS_AI_MOD, General AI, 07/20/09, jdog5000: START
int CvTeam::getNumWars(bool bIgnoreMinors, bool bIgnoreVassals) const
{	// <advc.003m> Cached
	int r = m_iMajorWarEnemies;
	if(!bIgnoreMinors)
		r += m_iMinorWarEnemies;
	if(bIgnoreVassals)
		r -= m_iVassalWarEnemies;
	FAssert(r >= 0 || !GC.getGame().isAllGameDataRead());
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
	for (TeamIter<CIV_ALIVE,ENEMY_OF> it(getID()); it.hasNext(); ++it)
	{
		CvTeam const& kEnemy = *it;
		if (bIgnoreMinors && kEnemy.isMinorCiv())
			continue;
		if (bIgnoreVassals && kEnemy.isAVassal())
			continue;
		iCount++;
		// advc.006: Disabled; see K-Mod comment.
		//FAssert(!(AI_isSneakAttackPreparing((TeamTypes)iI)) // K-Mod note. This assert can fail when in the process of declaring war
	}
	return iCount;
}
// BETTER_BTS_AI_MOD: END

// kekm.3: (actually an advc change)
bool CvTeam::allWarsShared(TeamTypes eOther, /* advc.130f: */ bool bCheckBothWays) const
{
	for (TeamIter<MAJOR_CIV> it; it.hasNext(); ++it)
	{
		CvTeam const& kLoopTeam = *it;
		if(bCheckBothWays && // advc.130f
				kLoopTeam.isAtWar(getID()) != kLoopTeam.isAtWar(eOther))
			return false;
		// <advc.130f>
		if(!kLoopTeam.isAtWar(eOther) && kLoopTeam.isAtWar(getID()))
			return false; // </advc.130f>
	}
	return true;
}


int CvTeam::getHasMetCivCount(bool bIgnoreMinors) const
{
	PROFILE_FUNC(); // advc.agent: Would be easy enough to cache this
	int r = (bIgnoreMinors ?
			TeamIter<MAJOR_CIV,OTHER_KNOWN_TO>::count(getID()) :
			TeamIter<CIV_ALIVE,OTHER_KNOWN_TO>::count(getID()));
	return r;
}


bool CvTeam::hasMetHuman() const
{
	for (TeamIter<HUMAN,NOT_SAME_TEAM_AS> it(getID()); it.hasNext(); ++it)
	{
		if (isHasMet(it->getID()))
			return true;
	}
	return false;
}


int CvTeam::getDefensivePactCount(TeamTypes eObs) const
{
	int iCount = 0;
	if (eObs == NO_TEAM)
	{
		for (TeamIter<MAJOR_CIV> it; it.hasNext(); ++it)
		{
			if (isDefensivePact(it->getID()))
				iCount++;
		}
	}
	else
	{
		for (TeamIter<MAJOR_CIV,OTHER_KNOWN_TO> it(eObs); it.hasNext(); ++it)
		{
			if (isDefensivePact(it->getID()))
				iCount++;
		}
	}
	return iCount;
}

int CvTeam::getVassalCount(TeamTypes eObs) const
{
	if (eObs == NO_TEAM)
		return TeamIter<ALIVE,VASSAL_OF>::count(getID());
	int iCount = 0;
	for (TeamIter<ALIVE,VASSAL_OF> it(getID()); it.hasNext(); ++it)
	{
		if (GET_TEAM(eObs).isHasMet(it->getID()))
			iCount++;
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

	int const iFREE_VASSAL_LAND_PERCENT = GC.getDefineINT(CvGlobals::FREE_VASSAL_LAND_PERCENT); // advc
	if (iFREE_VASSAL_LAND_PERCENT < 0 ||
		100 * std::max(10, getTotalLand(false)) < // advc.112: Lower bound added
		kMaster.getTotalLand(false) * iFREE_VASSAL_LAND_PERCENT)
	{
		return false;
	}
	int const iFREE_VASSAL_POPULATION_PERCENT = GC.getDefineINT(CvGlobals::FREE_VASSAL_POPULATION_PERCENT); // advc
	if (iFREE_VASSAL_POPULATION_PERCENT < 0 || 100 * getTotalPopulation(false) <
		kMaster.getTotalPopulation(false) * iFREE_VASSAL_POPULATION_PERCENT)
	{
		return false;
	}

	return true;
}

// advc.112: Cut from canVassalRevolt
bool CvTeam::isLossesAllowRevolt(TeamTypes eMaster) const
{
	CvTeam& kMaster = GET_TEAM(eMaster);
	if (isVassal(eMaster))
	{	// advc.112: Lower bound 10 added
		if (100 * std::max(10, getTotalLand(false)) < getVassalPower() *
			GC.getDefineINT("VASSAL_REVOLT_OWN_LOSSES_FACTOR"))
		{
			return true;
		}
		if (100 * kMaster.getTotalLand() < getMasterPower() *
			GC.getDefineINT("VASSAL_REVOLT_MASTER_LOSSES_FACTOR")) // advc (note): 0 in XML
		{
			return true;
		}
	}
	return false;
}


int CvTeam::getUnitClassMaking(UnitClassTypes eUnitClass) const
{
	int iCount = 0;
	for (MemberIter it(getID()); it.hasNext(); ++it)
		iCount += it->getUnitClassMaking(eUnitClass);
	return iCount;
}


int CvTeam::getUnitClassCountPlusMaking(UnitClassTypes eIndex) const
{
	return getUnitClassCount(eIndex) + getUnitClassMaking(eIndex);
}


int CvTeam::getBuildingClassMaking(BuildingClassTypes eBuildingClass) const
{
	int iCount = 0;
	for (MemberIter it(getID()); it.hasNext(); ++it)
		iCount += it->getBuildingClassMaking(eBuildingClass);
	return iCount;
}


int CvTeam::getBuildingClassCountPlusMaking(BuildingClassTypes eIndex) const
{
	return getBuildingClassCount(eIndex) + getBuildingClassMaking(eIndex);
}


int CvTeam::getHasReligionCount(ReligionTypes eReligion) const
{
	int iCount = 0;
	for (MemberIter it(getID()); it.hasNext(); ++it)
		iCount += it->getHasReligionCount(eReligion);
	return iCount;
}


int CvTeam::getHasCorporationCount(CorporationTypes eCorporation) const
{
	int iCount = 0;
	for (MemberIter it(getID()); it.hasNext(); ++it)
		iCount += it->getHasCorporationCount(eCorporation);
	return iCount;
}


int CvTeam::countTotalCulture() const
{
	int iCount = 0;
	for (MemberIter it(getID()); it.hasNext(); ++it)
		iCount += it->countTotalCulture();
	return iCount;
}

// advc.302:
bool CvTeam::isInContactWithBarbarians() const
{
	CvGame const& kGame = GC.getGame();
	if (kGame.isOption(GAMEOPTION_NO_BARBARIANS))
		return true; // Needed for advc.314 (free unit from goody hut)
	bool bCheckCity = kGame.getElapsedGameTurns() >=
			GC.getInfo(kGame.getGameSpeedType()).getBarbPercent();
	// (Perhaps just iUnitThresh=1 would have the same effect)
	int iUnitThresh = kGame.getCurrentEra();
	CvTeam const& kBarbarianTeam = GET_TEAM(BARBARIAN_TEAM);
	FOR_EACH_AREA(pArea)
	{
		if(bCheckCity && countNumCitiesByArea(*pArea) == 0)
			continue;
		if(!bCheckCity && countNumUnitsByArea(*pArea) < iUnitThresh)
			continue;
		if(bCheckCity)
		{
			int iBarbarianCities = kBarbarianTeam.countNumCitiesByArea(*pArea);
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
		int iBarbarianUnits = kBarbarianTeam.countNumUnitsByArea(*pArea);
		if(iBarbarianUnits > iUnitThresh) // Preliminary check to save time
			return true;
		std::vector<Shelf*> apShelves;
		GC.getMap().getShelves(*pArea, apShelves);
		for(size_t i = 0; i < apShelves.size(); i++)
			iBarbarianUnits += apShelves[i]->countBarbarians();
		if(iBarbarianUnits > iUnitThresh) // Actual check incl. ships
			return true;
	}
	return false;
}


int CvTeam::countNumUnitsByArea(CvArea const& kArea) const
{
	PROFILE_FUNC();

	int iCount = 0;
	for (MemberIter it(getID()); it.hasNext(); ++it)
		iCount += kArea.getUnitsPerPlayer(it->getID());
	return iCount;
}


int CvTeam::countNumCitiesByArea(CvArea const& kArea) const
{
	PROFILE_FUNC();

	int iCount = 0;
	for (MemberIter it(getID()); it.hasNext(); ++it)
		iCount += kArea.getCitiesPerPlayer(it->getID());
	return iCount;
}


int CvTeam::countTotalPopulationByArea(CvArea const& kArea) const
{
	int iCount = 0;
	for (MemberIter it(getID()); it.hasNext(); ++it)
		iCount += kArea.getPopulationPerPlayer(it->getID());
	return iCount;
}


int CvTeam::countPowerByArea(CvArea const& kArea) const
{
	int iCount = 0;
	for (MemberIter it(getID()); it.hasNext(); ++it)
		iCount += kArea.getPower(it->getID());
	return iCount;
}


int CvTeam::countNumAIUnitsByArea(CvArea const& kArea, UnitAITypes eUnitAI) const
{
	PROFILE_FUNC();

	int iCount = 0;
	for (MemberIter it(getID()); it.hasNext(); ++it)
		iCount += kArea.getNumAIUnits(it->getID(), eUnitAI);
	return iCount;
}

// BETTER_BTS_AI_MOD, War strategy AI, 05/19/10, jdog5000:
int CvTeam::countEnemyDangerByArea(CvArea const& kArea, TeamTypes eEnemyTeam) const
{
	PROFILE_FUNC();

	int iCount = 0;
	for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
	{
		CvPlot const& kPlot = GC.getMap().getPlotByIndex(iI);
		if (kPlot.isArea(kArea))
		{
			if (kPlot.getTeam() == getID())
			{
				iCount += kPlot.plotCount(PUF_canDefendEnemy, getLeaderID(),
						false, NO_PLAYER, eEnemyTeam, PUF_isVisible, getLeaderID());
			}
		}
	}
	return iCount;
}

/*	advc.112b: (Akin to CvGame::getCurrentEra;
	in part duplicated in CvTeamAI::AI_getCurrEraFactor) */
EraTypes CvTeam::getCurrentEra() const
{
	scaled rSum;
	MemberIter it(getID());
	for (; it.hasNext(); ++it)
		rSum += it->getCurrentEra();
	int const iDiv = it.nextIndex();
	if (iDiv <= 0)
	{
		FAssertMsg(iDiv > 0, "Shouldn't calculate era of dead team");
		return NO_ERA;
	}
	return (EraTypes)(rSum / iDiv).round();
}

// K-Mod:
int CvTeam::getTypicalUnitValue(UnitAITypes eUnitAI, DomainTypes eDomain) const
{
	int iMax = 0;
	for (MemberIter it(getID()); it.hasNext(); ++it)
		iMax = std::max(iMax, it->getTypicalUnitValue(eUnitAI, eDomain));
	return iMax;
}


int CvTeam::getResearchCost(TechTypes eTech,
	bool bGlobalModifiers, bool bTeamSizeModifiers) const // K-Mod: params added
{
	CvGame const& kGame = GC.getGame();
	CvTechInfo const& kTech = GC.getInfo(eTech);

	// advc.251: To reduce rounding errors (as there are quite a few modifiers to apply)
	scaled rCost = kTech.getResearchCost();
	rCost *= per100(GC.getInfo(getHandicapType()).getResearchPercent());
	// <advc.251>
	if (!isHuman() && !isBarbarian())
	{
		// Important to use game handicap here (not team handicap)
		rCost *= per100(GC.getInfo(kGame.getHandicapType()).
				getAIResearchPercent() + kGame.AIHandicapAdjustment());
	}
	// <advc.910> Moved from CvPlayer::calculateResearchModifier
	EraTypes const eTechEra = kTech.getEra();
	scaled rModifier = 1 + per100(GC.getInfo(eTechEra).getTechCostModifier());
	/*  This is a BBAI tech diffusion thing, but, since it applies always, I think
		it's better to let it reduce the tech cost than to modify research rate. */
	static int const iTECH_COST_MODIFIER = GC.getDefineINT("TECH_COST_MODIFIER");
	rModifier += per100(iTECH_COST_MODIFIER);
	// </advc.910>
	rModifier += GC.getGame().groundbreakingNormalizationModifier(eTech); // advc.groundbr
	if (bGlobalModifiers) // K-Mod
	{
		CvWorldInfo const& kWorld = GC.getInfo(GC.getMap().getWorldSize());
		if (eTechEra > 0) // advc.910
		{
			rCost *= per100(kWorld.getResearchPercent());
			// <advc.910>
			rCost *= per100(GC.getInfo(GC.getMap().getSeaLevel()).getResearchPercent());
			if (kGame.isOption(GAMEOPTION_ALWAYS_PEACE) &&
				!kGame.isOption(GAMEOPTION_ALWAYS_WAR))
			{
				rCost *= per100(105);
			}
		}
		else if (kGame.isOption(GAMEOPTION_NO_GOODY_HUTS))
			rCost /= per100(105); // </advc.910>
		rCost *= per100(GC.getInfo(kGame.getGameSpeedType()).getResearchPercent());
		rCost *= per100(GC.getInfo(kGame.getStartEra()).getResearchPercent());
		// <advc.308>
		if(kGame.isOption(GAMEOPTION_RAGING_BARBARIANS) && kGame.getStartEra() == 0)
		{
			if (eTechEra == 1)
				rModifier -= per100(14);
			else if (eTechEra == 2)
				rModifier -= per100(7);
		}
		if(kGame.getStartEra() == 0 && kGame.isOption(GAMEOPTION_NO_BARBARIANS) &&
			eTechEra <= 1)
		{
			rModifier += per100(3);
		} // </advc.308>
		// <advc.550d>
		if (kGame.isOption(GAMEOPTION_NO_TECH_TRADING) &&
			eTechEra > 0 && !kTech.isRepeat())
		{
			static scaled const rTECH_COST_NOTRADE_MODIFIER = per100(
					GC.getDefineINT("TECH_COST_NOTRADE_MODIFIER"));
			scaled rNoTradeAdjustment =
					(rTECH_COST_NOTRADE_MODIFIER + per100(5) *
					(eTechEra - fixp(2.5)).abs().pow(fixp(1.5))) *
					scaled::clamp(scaled(kWorld.getDefaultPlayers() - 2,
					11 - eTechEra), 0, fixp(8/3.));
			rNoTradeAdjustment.decreaseTo(0); // No Tech Trading can only lower tech costs
			rModifier += rNoTradeAdjustment;
		} // </advc.550d>
	}

	if (bTeamSizeModifiers) // K-Mod
	{
		rCost *= scaled::max(per100(100 +
				GC.getDefineINT(CvGlobals::TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER) *
				(getNumMembers() - 1)), 0);
	}
	// <advc.251>
	rCost *= scaled::max(rModifier, 1);
	int iCost = rCost.roundToMultiple(isHuman() ? 5 : 1);
	// </advc.251>
	return std::max(1, iCost);
}


int CvTeam::getResearchLeft(TechTypes eTech) const
{
	// <advc> Safer, cleaner this way. (NB: NO_TECH isn't allowed here.)
	if (isHasTech(eTech) && !GC.getInfo(eTech).isRepeat())
		return 0; // </advc>
	return std::max(0, getResearchCost(eTech) - getResearchProgress(eTech));
}


bool CvTeam::hasHolyCity(ReligionTypes eReligion) const
{
	CvCity* pHolyCity = GC.getGame().getHolyCity(eReligion);
	if (pHolyCity != NULL)
		return (pHolyCity->getTeam() == getID());

	return false;
}


bool CvTeam::hasHeadquarters(CorporationTypes eCorporation) const
{
	CvCity* pHeadquarters = GC.getGame().getHeadquarters(eCorporation);
	if (pHeadquarters != NULL)
		return (pHeadquarters->getTeam() == getID());

	return false;
}

bool CvTeam::hasBonus(BonusTypes eBonus) const
{
	for (MemberIter it(getID()); it.hasNext(); ++it)
	{
		if (it->hasBonus(eBonus))
			return true;
	}
	return false;
}

bool CvTeam::isBonusObsolete(BonusTypes eBonus) const
{
	TechTypes eObsoleteTech = GC.getInfo(eBonus).getTechObsolete();
	return (eObsoleteTech != NO_TECH && isHasTech(eObsoleteTech));
}


bool CvTeam::isHuman() const
{
	// advc.opt: Now that updateLeaderID prefers human leaders, we can save some time.
	return (getLeaderID() != NO_PLAYER && GET_PLAYER(getLeaderID()).isHuman());
	/*for (MemberIter it(getID()); it.hasNext(); ++it) {
		if (it->isHuman())
			return true;
	}
	return false;*/
}


bool CvTeam::checkMinorCiv() const // advc.003m: Renamed from isMinorCiv
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
				bValid = true;
			else return false;
		}
	}
	return bValid;
}

// advc.opt:
void CvTeam::updateLeaderID()
{
	PlayerTypes eFormerLeader = getLeaderID();
	int iBestValue = 0;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayer const& kPlayer = GET_PLAYER((PlayerTypes)i);
		if (kPlayer.getTeam() != getID())
			continue;
		int iValue = 1;
		if (kPlayer.isAlive())
			iValue += 10;
		// Init core knows humans even while loading a save and during Auto Play
		if (GC.getInitCore().getHuman(kPlayer.getID()))
			iValue += 1; // advc: Prefer human leader
		if (iValue > iBestValue)
		{
			iBestValue = iValue;
			m_eLeader = kPlayer.getID();
		}
	}
	// <advc.104t>
	if (m_eLeader != eFormerLeader && getUWAI.isEnabled())
		GET_PLAYER(m_eLeader).uwai().getCache().onTeamLeaderChanged(eFormerLeader);
	// </advc.104t>
	if (m_eLeader == NO_PLAYER)
	{
		FAssert(m_eLeader != NO_PLAYER);
		m_eLeader = eFormerLeader; // Better than nothing (maybe)
	}
}


PlayerTypes CvTeam::getSecretaryID() const
{
	// advc.opt: No longer needed
	/*for (PlayerIter<HUMAN,MEMBER_OF> it(getID()); it.hasNext(); ++it)
		return it->getID();*/
	return getLeaderID();
}


HandicapTypes CvTeam::getHandicapType() const
{
	int iGameHandicap = 0;
	MemberIter it(getID());
	for (; it.hasNext(); ++it)
	{
		iGameHandicap += GC.getInfo(it->getHandicapType()).
				getDifficulty(); // advc.250a
	}
	int const iDiv = it.nextIndex();
	if (iDiv <= 0)
		return (HandicapTypes)GC.getDefineINT("STANDARD_HANDICAP");
	//FAssert((iGameHandicap / iDiv) >= 0);
	// advc.250a: (also disabled the assertion above)
	return (HandicapTypes)std::min(GC.getNumHandicapInfos() - 1,
			(iGameHandicap / (10 * iDiv)));

}


CvWString CvTeam::getName() const
{
	CvWString szBuffer;
	bool bFirst = true;
	for (MemberIter it (getID()); it.hasNext(); ++it)
	{
		setListHelp(szBuffer, L"", it->getName(), L"/", bFirst);
	}
	return szBuffer;
}

// K-Mod: Name to be used in replay
CvWString CvTeam::getReplayName() const
{
	CvWString szBuffer;
	bool bFirst = true;
	for (MemberIter it (getID()); it.hasNext(); ++it)
	{
		setListHelp(szBuffer, L"", it->getReplayName(), L"/", bFirst);
	}
	return szBuffer;
} 


void CvTeam::changeNumMembers(int iChange)
{
	m_iNumMembers += iChange;
	FAssert(getNumMembers() >= 0);
}


void CvTeam::changeAliveCount(int iChange)
{
	bool const bEverAlive = isEverAlive();
	m_iAliveCount += iChange;
	FAssert(getAliveCount() >= 0);

	if (m_iAliveCount == 0)
	{
		for (int iTeam = 0; iTeam < MAX_TEAMS; iTeam++)
		{
			if (iTeam == getID())
				continue;

			CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
			// free vassals
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
			// </advc.003m>  <advc.opt> Also keep WarPlanCounts updated
			kLoopTeam.AI_setWarPlanNoUpdate(getID(), NO_WARPLAN);
			AI().AI_setWarPlanNoUpdate(kLoopTeam.getID(), NO_WARPLAN);
		}
	}
	if (!isBarbarian() && m_iAliveCount - iChange <= 0 && m_iAliveCount > 0 && !bEverAlive)
		GC.getGame().changeCivTeamsEverAlive(1); // </advc.opt>
	// <advc.104> Can't do this in AI_init because alive status isn't yet set at that point
	if (m_iAliveCount == 1 && m_iAliveCount - iChange <= 0 && isMajorCiv() &&
		(getUWAI.isEnabled() || getUWAI.isEnabled(true)))
	{
		AI().uwai().init(getID());
	} // </advc.104>
}


void CvTeam::changeEverAliveCount(int iChange)
{
	m_iEverAliveCount += iChange;
	FAssert(getEverAliveCount() >= 0);
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
			return m_iTotalPopulation / 2;

		for (TeamIter<ALIVE,VASSAL_OF> it(getID()); it.hasNext(); ++it)
			iVassalPop += it->getTotalPopulation(false) / 2;
	}

	return m_iTotalPopulation + iVassalPop;
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
			return m_iTotalLand / 2;

		for (TeamIter<ALIVE,VASSAL_OF> it(getID()); it.hasNext(); ++it)
			iVassalLand += it->getTotalLand(false) / 2;
	}

	return m_iTotalLand + iVassalLand;
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
	return m_aiForceTeamVoteEligibilityCount.get(eVoteSource);
}


bool CvTeam::isForceTeamVoteEligible(VoteSourceTypes eVoteSource) const
{
	return (getForceTeamVoteEligibilityCount(eVoteSource) > 0 &&
			!isMinorCiv() /* advc.104: */ && !isCapitulated());
}


void CvTeam::changeForceTeamVoteEligibilityCount(VoteSourceTypes eVoteSource, int iChange)
{
	m_aiForceTeamVoteEligibilityCount.add(eVoteSource, iChange);
	FAssert(getForceTeamVoteEligibilityCount(eVoteSource) >= 0);
}


void CvTeam::changeExtraWaterSeeFromCount(int iChange)
{
	if (iChange == 0)
		return;

	GC.getMap().updateSight(false);

	m_iExtraWaterSeeFromCount = (m_iExtraWaterSeeFromCount + iChange);
	FAssert(getExtraWaterSeeFromCount() >= 0);

	GC.getMap().updateSight(true);
}


void CvTeam::changeMapTradingCount(int iChange)
{
	m_iMapTradingCount = (m_iMapTradingCount + iChange);
	FAssert(getMapTradingCount() >= 0);
}


void CvTeam::changeTechTradingCount(int iChange)
{
	m_iTechTradingCount = (m_iTechTradingCount + iChange);
	FAssert(getTechTradingCount() >= 0);
}


void CvTeam::changeGoldTradingCount(int iChange)
{
	m_iGoldTradingCount = (m_iGoldTradingCount + iChange);
	FAssert(getGoldTradingCount() >= 0);
}


void CvTeam::changeOpenBordersTradingCount(int iChange)
{
	m_iOpenBordersTradingCount = (m_iOpenBordersTradingCount + iChange);
	FAssert(getOpenBordersTradingCount() >= 0);
}


void CvTeam::changeDefensivePactTradingCount(int iChange)
{
	m_iDefensivePactTradingCount = (m_iDefensivePactTradingCount + iChange);
	FAssert(getDefensivePactTradingCount() >= 0);
}


bool CvTeam::isPermanentAllianceTrading() const
{
	return (GC.getGame().isOption(GAMEOPTION_PERMANENT_ALLIANCES) &&
		getPermanentAllianceTradingCount() > 0);
}


void CvTeam::changePermanentAllianceTradingCount(int iChange)
{
	m_iPermanentAllianceTradingCount = (m_iPermanentAllianceTradingCount + iChange);
	FAssert(getPermanentAllianceTradingCount() >= 0);
}


bool CvTeam::isVassalStateTrading() const
{
	return (!GC.getGame().isOption(GAMEOPTION_NO_VASSAL_STATES) &&
			getVassalTradingCount() > 0);
}


void CvTeam::changeVassalTradingCount(int iChange)
{
	m_iVassalTradingCount += iChange;
	FAssert(getVassalTradingCount() >= 0);
}


void CvTeam::changeBridgeBuildingCount(int iChange)
{
	if (iChange == 0)
		return;

	m_iBridgeBuildingCount = (m_iBridgeBuildingCount + iChange);
	FAssert(getBridgeBuildingCount() >= 0);

	if (GC.IsGraphicsInitialized())
		gDLL->getEngineIFace()->MarkBridgesDirty();
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


void CvTeam::changeIgnoreIrrigationCount(int iChange)
{
	m_iIgnoreIrrigationCount = (m_iIgnoreIrrigationCount + iChange);
	FAssert(getIgnoreIrrigationCount() >= 0);
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


void CvTeam::changeEnemyWarWearinessModifier(int iChange)
{
	m_iEnemyWarWearinessModifier += iChange;
}


void CvTeam::setMapCentering(bool bNewValue)
{
	if (isMapCentering() != bNewValue)
	{
		m_bMapCentering = bNewValue;
		if (getID() == GC.getGame().getActiveTeam())
			gDLL->UI().setDirty(MinimapSection_DIRTY_BIT, true);
	}
}


void CvTeam::setStolenVisibilityTimer(TeamTypes eIndex, int iNewValue)
{
	if(getStolenVisibilityTimer(eIndex) == iNewValue)
		return;

	bool bOldStolenVisibility = isStolenVisibility(eIndex);

	m_aiStolenVisibilityTimer.set(eIndex, iNewValue);
	FAssert(getStolenVisibilityTimer(eIndex) >= 0);

	if (bOldStolenVisibility != isStolenVisibility(eIndex))
	{
		CvMap const& kMap = GC.getMap();
		for (int iI = 0; iI < kMap.numPlots(); iI++)
		{
			CvPlot& kPlot = kMap.getPlotByIndex(iI);
			if (kPlot.isVisible(eIndex))
			{
				kPlot.changeStolenVisibilityCount(
						getID(), isStolenVisibility(eIndex) ? 1 : -1);
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
	return  /* <K-Mod> */ (bUseEnemyModifer ? m_aiWarWeariness.get(eIndex) *
			std::max(0, 100 + GET_TEAM(eIndex).getEnemyWarWearinessModifier()) / 100 :
			// </K-Mod>
			m_aiWarWeariness.get(eIndex));
}


void CvTeam::setWarWeariness(TeamTypes eIndex, int iNewValue)
{
	m_aiWarWeariness.set(eIndex, std::max(0, iNewValue));
}


void CvTeam::changeWarWeariness(TeamTypes eIndex, int iChange)
{
	setWarWeariness(eIndex, getWarWeariness(eIndex) + iChange);
}

void CvTeam::changeWarWeariness(TeamTypes eOtherTeam, const CvPlot& kPlot, int iFactor)
{
	int iOurCulture = kPlot.countFriendlyCulture(getID());
	int iTheirCulture = kPlot.countFriendlyCulture(eOtherTeam);

	int iRatio = 100;
	if (iOurCulture + iTheirCulture != 0)
		iRatio = (100 * iTheirCulture) / (iOurCulture + iTheirCulture);

	changeWarWeariness(eOtherTeam, iRatio * iFactor);
}


int CvTeam::getTechShareCount(TeamTypes eIndex) const
{
	return m_aiTechShareCount.get(eIndex);
}


bool CvTeam::isTechShare(TeamTypes eIndex) const
{
	return (getTechShareCount(eIndex) > 0);
}


void CvTeam::changeTechShareCount(TeamTypes eIndex, int iChange)
{
	if (iChange == 0)
		return;

	m_aiTechShareCount.add(eIndex, iChange);
	FAssert(getTechShareCount(eIndex) >= 0);
	if (isTechShare(eIndex))
		updateTechShare();
}


int CvTeam::getCommerceFlexibleCount(CommerceTypes eIndex) const
{
	return m_aiCommerceFlexibleCount.get(eIndex);
}


bool CvTeam::isCommerceFlexible(CommerceTypes eIndex) const
{
	return (getCommerceFlexibleCount(eIndex) > 0);
}


void CvTeam::changeCommerceFlexibleCount(CommerceTypes eIndex, int iChange)
{
	if (iChange == 0)
		return;

	m_aiCommerceFlexibleCount.add(eIndex, iChange);
	FAssert(getCommerceFlexibleCount(eIndex) >= 0);

	if (getID() == GC.getGame().getActiveTeam())
	{
		gDLL->UI().setDirty(PercentButtons_DIRTY_BIT, true);
		gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
	}
}


int CvTeam::getExtraMoves(DomainTypes eIndex) const
{
	return m_aiExtraMoves.get(eIndex);
}


void CvTeam::changeExtraMoves(DomainTypes eIndex, int iChange)
{
	m_aiExtraMoves.add(eIndex, iChange);
	FAssert(getExtraMoves(eIndex) >= 0);
}


void CvTeam::makeHasMet(TeamTypes eOther, bool bNewDiplo,
	FirstContactData* pData) // advc.071
{
	if(isHasMet(eOther))
		return;

	makeHasSeen(eOther); // K-Mod
	//m_abHasMet.set(eOther, true);
	// <advc.091>
	{
		int iGameTurn = GC.getGame().getGameTurn();
		FAssert(iGameTurn >= 0);
		m_aiHasMetTurn.set(eOther, toShort(iGameTurn));
	} // </advc.091>
	updateTechShare();

	/*if(GET_TEAM(eOther).isHuman()) {
		for (iI = 0; iI < MAX_PLAYERS; iI++) {
			if (GET_PLAYER((PlayerTypes)iI).isAlive()) {
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID()) {
					if (!(GET_PLAYER((PlayerTypes)iI).isHuman())) {
						GET_PLAYER((PlayerTypes)iI).clearResearchQueue();
						GET_PLAYER((PlayerTypes)iI).AI_makeProductionDirty();
					}}}}}*/ // BtS
	// disabled by K-Mod (wtf did they hope to achieve with this?)

	//if (isAVassal()) ...  // advc.001: Vassal/ master meetings moved to CvTeam::meet

	// report event to Python, along with some other key state
	CvEventReporter::getInstance().firstContact(getID(), eOther);
	// <advc.071> ^Moved EventReporter call up // advc.001n:
	if(eOther == getID() || isBarbarian() || GET_TEAM(eOther).isBarbarian())
		return; // </advc.071>

	// K-Mod: Initialize attitude cache for players on our team towards player's on their team.
	// advc.001: Too early for that. Moved to caller (CvTeam::meet).

	if (getID() == GC.getGame().getActiveTeam() || eOther == GC.getGame().getActiveTeam())
		gDLL->UI().setDirty(Score_DIRTY_BIT, true);
	// <advc.071>
	bool bShowMessage = (isHuman() && pData != NULL);
	if(bShowMessage || bNewDiplo)
	{
		int iOnFirstContact = 1;
		// If met during the placement of free starting units, show only a diplo popup.
		if(GC.IsGraphicsInitialized())
			iOnFirstContact = BUGOption::getValue("Civ4lerts__OnFirstContact", 2);
		if(bNewDiplo && iOnFirstContact == 0)
			bNewDiplo = false;
		if(bShowMessage && iOnFirstContact == 1)
			bShowMessage = false;
	} // </advc.071>
	if (GC.getGame().isOption(GAMEOPTION_ALWAYS_WAR))
	{
		if (isHuman() && getID() != eOther)
			declareWar(eOther, false, NO_WARPLAN);
	}  // advc: reduce indentation in else branch
	else if (GC.getGame().isFinalInitialized() && !gDLL->GetWorldBuilderMode() &&
		bNewDiplo && !isHuman() && !isAtWar(eOther))
	{
		for (PlayerIter<HUMAN,MEMBER_OF> it(eOther); it.hasNext(); ++it)
		{
			CvPlayer const& kMember = *it;
			if (GET_PLAYER(getLeaderID()).canContact(kMember.getID()))
			{
				CvDiploParameters* pDiplo = new CvDiploParameters(getLeaderID());
				pDiplo->setDiploComment(GC.getAIDiploCommentType("FIRST_CONTACT"));
				pDiplo->setAIContact(true);
				gDLL->beginDiplomacy(pDiplo, kMember.getID());
			}
		}
	}
	// <advc.071>
	if(bShowMessage)
	{
		FirstContactData fcData = *pData;
		CvMap const& m = GC.getMap();
		CvPlot const* pAt1 = m.plot(fcData.x1, fcData.y1);
		CvPlot const* pAt2 = m.plot(fcData.x2, fcData.y2);
		CvUnit const* pUnit1 = ::getUnit(fcData.u1);
		CvUnit const* pUnit2 = ::getUnit(fcData.u2);
		CvUnit const* pUnitMet = NULL;
		CvPlot const* pAt = NULL;
		PlayerTypes ePlayerMet = NO_PLAYER;
		if (pUnit1 != NULL && pUnit1->getTeam() == eOther)
		{
			ePlayerMet = pUnit1->getOwner();
			if (pUnit1->getPlot().isVisible(getID()))
				pUnitMet = pUnit1;
		}
		if (pUnit2 != NULL && pUnit2->getTeam() == eOther)
		{
			if (ePlayerMet == NO_PLAYER)
				ePlayerMet = pUnit2->getOwner();
			if (pUnit2->getPlot().isVisible(getID()))
				pUnitMet = pUnit2;
		}
		if (pAt1 != NULL && pAt1->isOwned() && pAt1->getTeam() == eOther)
		{
			if (pAt1->isVisible(getID()))
				pAt = pAt1;
			if (ePlayerMet == NO_PLAYER)
				ePlayerMet = pAt1->getOwner();
		}
		if (pAt2 != NULL && pAt2->isOwned() && pAt2->getTeam() == eOther)
		{
			if (pAt2->isVisible(getID()))
				pAt = pAt2;
			if (ePlayerMet == NO_PLAYER)
				ePlayerMet = pAt2->getOwner();
		}
		if (ePlayerMet == NO_PLAYER)
			ePlayerMet = GET_TEAM(eOther).getLeaderID();
		if (pUnitMet != NULL && pUnitMet->getPlot().isVisible(getID()))
			pAt = pUnitMet->plot();
		if (pAt == NULL) // We can't see any of their tiles or units, but they see ours.
		{
			if (pAt1 != NULL && pAt1->isOwned() && pAt1->getTeam() == getID())
				pAt = pAt1;
			else if (pAt2 != NULL && pAt2->isOwned() && pAt2->getTeam() == getID())
				pAt = pAt2;
			else if (pUnit1 != NULL && pUnit1->getTeam() == getID())
			{
				//pUnitMet = pUnit1; // Better not to show our own unit's icon
				pAt = pUnit1->plot();
			}
			else if (pUnit2 != NULL && pUnit2->getTeam() == getID())
			{
				//pUnitMet = pUnit2;
				pAt = pUnit2->plot();
			}
		}
		CvWString szMsg = gDLL->getText("TXT_KEY_MISC_TEAM_MET",
				GET_PLAYER(ePlayerMet).getCivilizationAdjectiveKey());
		ColorTypes ePlayerColor = GET_PLAYER(ePlayerMet).getPlayerTextColor();
		LPCSTR icon = (pUnitMet == NULL ? GC.getInfo(GET_PLAYER(ePlayerMet).
				getLeaderType()).getButton() : pUnitMet->getButton());
		for (PlayerIter<HUMAN,MEMBER_OF> it(getID()); it.hasNext(); ++it)
		{
			gDLL->UI().addMessage(it->getID(), false, -1, szMsg, NULL,
					MESSAGE_TYPE_MINOR_EVENT, icon, ePlayerColor,
					pAt == NULL ? -1 : pAt->getX(), pAt == NULL ? -1 : pAt->getY(),
					pAt != NULL, pAt != NULL);
		}
	} // </advc.071>
}

// <advc.134a>
bool CvTeam::isAtWarExternal(TeamTypes eIndex) const
{
	/*  Feign peace if we know that the EXE is about to check a peace offer
		(b/c being at war shouldn't prevent AI-to-human peace offers). */
	if (m_iPeaceOfferStage == 2 && m_eOfferingPeace == eIndex)
	{
		m_iPeaceOfferStage = 0;
		m_eOfferingPeace = NO_TEAM;
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


bool CvTeam::isPeaceOfferStage(int iStage, TeamTypes eOffering) const
{
	return (eOffering == m_eOfferingPeace && iStage == m_iPeaceOfferStage);
} // </advc.134a>


void CvTeam::setAtWar(TeamTypes eIndex, bool bNewValue)
{
	// <advc.035>
	if(m_abAtWar.get(eIndex) == bNewValue)
		return; // </advc.035>
	m_abAtWar.set(eIndex, bNewValue);
	// <advc.003m>
	if (eIndex != BARBARIAN_TEAM)
	{
		changeAtWarCount(bNewValue ? 1 : -1, GET_TEAM(eIndex).isMinorCiv(),
				GET_TEAM(eIndex).isAVassal());
	} // </advc.003m>
	// <advc.035>
	if (eIndex < getID() ||
		!isAlive() || !GET_TEAM(eIndex).isAlive()) // advc.003m
	{
		return; // setAtWar gets called on both sides; do this only once.
	}
	std::vector<CvPlot*> flipPlots;
	::contestedPlots(flipPlots, getID(), eIndex);
	for (size_t i = 0; i < flipPlots.size(); i++)
	{
		CvPlot& p = *flipPlots[i];
		PlayerTypes secondOwner = p.getSecondOwner();
		p.setSecondOwner(p.getOwner());
		/*  I guess it's a bit cleaner (faster?) not to bump units and update
			plot groups until all tiles are flipped */
		p.setOwner(secondOwner, false, false);
	}
	for (size_t i = 0; i < flipPlots.size(); i++)
	{
		CvPlot& p = *flipPlots[i];
		p.updatePlotGroup();
		p.verifyUnitValidPlot();
	}
	for (MemberIter itOurMember(getID()); itOurMember.hasNext(); ++itOurMember)
	{
		CvPlayerAI& ourMember = *itOurMember;
		for (MemberIter itTheirMember(eIndex); itTheirMember.hasNext(); ++itTheirMember)
		{
			CvPlayerAI& theirMember = *itTheirMember;
			ourMember.AI_updateCloseBorderAttitude(theirMember.getID());
			theirMember.AI_updateCloseBorderAttitude(ourMember.getID());
		}
	} // (Attitude cache is updated by caller)
	// </advc.035>
}

/*  advc.162: "Just" meaning on the current turn. Don't want to rely on
	AI code (AI_getWarPlanStateCounter) for this. */
bool CvTeam::hasJustDeclaredWar(TeamTypes eIndex) const
{
	return m_abJustDeclaredWar.get(eIndex);
}


void CvTeam::setPermanentWarPeace(TeamTypes eIndex, bool bNewValue)
{
	m_abPermanentWarPeace.set(eIndex, bNewValue);
}

// advc: Body cut from CvPlayer::canTradeWith
bool CvTeam::canTradeWith(TeamTypes eWhoTo) const
{
	CvTeam const& kToTeam = GET_TEAM(eWhoTo);
	return (isAtWar(eWhoTo) ||
			isTechTrading() || kToTeam.isTechTrading() ||
			isGoldTrading() || kToTeam.isGoldTrading() ||
			isMapTrading() || kToTeam.isMapTrading() ||
			isOpenBordersTrading() || kToTeam.isOpenBordersTrading() ||
			isDefensivePactTrading() || kToTeam.isDefensivePactTrading() ||
			isPermanentAllianceTrading() || kToTeam.isPermanentAllianceTrading() ||
			isVassalStateTrading() || kToTeam.isVassalStateTrading());
}


bool CvTeam::isFreeTrade(TeamTypes eIndex) const
{
	if (isAtWar(eIndex))
		return false;

	if (!isHasMet(eIndex))
		return false;

	return (isOpenBorders(eIndex) || GC.getGame().isFreeTrade());
}


void CvTeam::setOpenBorders(TeamTypes eIndex, bool bNewValue)
{
	if(isOpenBorders(eIndex) == bNewValue)
		return; // advc
	bool bOldFreeTrade = isFreeTrade(eIndex);
	m_abOpenBorders.set(eIndex, bNewValue);
	// <advc.130p> OB affect diplo from rival trade
	for (PlayerIter<MAJOR_CIV,NOT_SAME_TEAM_AS> itOther(getID()); itOther.hasNext(); ++itOther)
	{
		for (MemberIter itMember(getID()); itMember.hasNext(); ++itMember)
			itOther->AI_updateAttitude(itMember->getID());
	} // </advc.130p>
	AI().AI_setOpenBordersCounter(eIndex, 0);

	GC.getMap().verifyUnitValidPlot();

	if (getID() == GC.getGame().getActiveTeam() || eIndex == GC.getGame().getActiveTeam())
		gDLL->UI().setDirty(Score_DIRTY_BIT, true);

	if (bOldFreeTrade != isFreeTrade(eIndex))
	{
		for (MemberIter it(getID()); it.hasNext(); ++it)
			it->updateTradeRoutes();
	} // <advc.034>
	if(bNewValue)
		AI().cancelDisengage(eIndex); // </advc.034>
}

// <advc.034>
void CvTeam::setDisengage(TeamTypes eIndex, bool bNewValue)
{
	m_abDisengage.set(eIndex, bNewValue);
	if (!bNewValue && !isFriendlyTerritory(eIndex) && !isAtWar(eIndex))
		GC.getMap().verifyUnitValidPlot(); // Bump units
}

void CvTeam::cancelDisengage(TeamTypes otherId)
{
	FOR_EACH_DEAL_VAR(d)
	{
		if(d->isDisengage() && d->isBetween(getID(), otherId))
		{
			d->kill(false);
			break;
		}
	}
} // </advc.034>


void CvTeam::setDefensivePact(TeamTypes eIndex, bool bNewValue)
{
	FAssert(getID() != eIndex); // advc
	if (isDefensivePact(eIndex) == bNewValue)
		return; // advc
	m_abDefensivePact.set(eIndex, bNewValue);
	if(getID() == GC.getGame().getActiveTeam() || eIndex == GC.getGame().getActiveTeam())
		gDLL->UI().setDirty(Score_DIRTY_BIT, true);
	CvTeam const& kOther = GET_TEAM(eIndex); // advc
	if (bNewValue && !kOther.isDefensivePact(getID()))
	{
		CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_PLAYERS_SIGN_DEFENSIVE_PACT", getReplayName().GetCString(), kOther.getReplayName().GetCString());
		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(), szBuffer,
				-1, -1, GC.getColorType("HIGHLIGHT_TEXT"));
		for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
		{
			CvPlayer& kObs = *it;
			if ((isHasMet(kObs.getTeam()) && kOther.isHasMet(kObs.getTeam())) ||
				kObs.isSpectator()) // advc.127
			{
				gDLL->UI().addMessage(kObs.getID(), false, -1, szBuffer,
						"AS2D_THEIRALLIANCE", MESSAGE_TYPE_MAJOR_EVENT, NULL,
						GC.getColorType("HIGHLIGHT_TEXT"),
						// advc.127b:
						getCapitalX(kObs.getTeam(), true), getCapitalY(kObs.getTeam(), true));
			}
		}
	}
	// <advc.106f> Based on the previous block
	else if (!bNewValue && kOther.isDefensivePact(getID()))
	{
		CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_PLAYERS_CANCEL_DEFENSIVE_PACT",
				getReplayName().GetCString(), kOther.getReplayName().GetCString());
		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(), szBuffer,
				-1, -1, GC.getColorType("HIGHLIGHT_TEXT"));
		for (PlayerIter<MAJOR_CIV,NOT_SAME_TEAM_AS> it(getID()); it.hasNext(); ++it)
		{
			CvPlayer& kObs = *it;
			if ((isHasMet(kObs.getTeam()) && kOther.isHasMet(kObs.getTeam())) ||
				kObs.isSpectator()) // advc.127
			{
				gDLL->UI().addMessage(kObs.getID(), false, -1, szBuffer,
						NULL,
						//"AS2D_DEAL_CANCELLED" // Rather use no sound
						MESSAGE_TYPE_MAJOR_EVENT, NULL, NO_COLOR,
						// advc.127b:
						getCapitalX(kObs.getTeam(), true), getCapitalY(kObs.getTeam(), true));
			}
		}
	} // </advc.106f>
	// K-Mod. update attitude
	if (GC.getGame().isFinalInitialized())
	{
		for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
			it->AI_updateAttitude();
	} // K-Mod end
}


void CvTeam::setForcePeace(TeamTypes eIndex, bool bNewValue)
{
	m_abForcePeace.set(eIndex, bNewValue);
	if (isForcePeace(eIndex))
	{
		// advc.001: Don't see why sneak-attack-ready shouldn't be canceled as well
		//if (AI().AI_isSneakAttackPreparing(eIndex))
		AI().AI_setWarPlan(eIndex, NO_WARPLAN);
		for (TeamIter<ALIVE,VASSAL_OF> it(eIndex); it.hasNext(); ++it)
		{
			//if (AI().AI_isSneakAttackPreparing(it->getID())) // advc.001
			AI().AI_setWarPlan(it->getID(), NO_WARPLAN);
		}
	}
}

// advc.104:
int CvTeam::turnsOfForcedPeaceRemaining(TeamTypes eOther) const
{
	if(!canEventuallyDeclareWar(eOther))
		return MAX_INT;
	if(!isForcePeace(eOther))
		return 0;
	TeamTypes eOurMaster = getMasterTeam();
	TeamTypes eTheirMaster = GET_TEAM(eOther).getMasterTeam();
	int r = 0;
	FOR_EACH_DEAL(d)
	{
		TeamTypes eFirstMaster = GET_PLAYER(d->getFirstPlayer()).getMasterTeam();
		TeamTypes eSecondMaster = GET_PLAYER(d->getSecondPlayer()).getMasterTeam();
		if (((eFirstMaster == eOurMaster && eSecondMaster == eTheirMaster) ||
			(eFirstMaster == eTheirMaster && eSecondMaster == eOurMaster)) &&
			d->getLengthFirst() > 0 &&
			d->getFirstList().head()->m_data.m_eItemType == TRADE_PEACE_TREATY)
		{
			r = std::max(r, d->turnsToCancel());
		}
	}
	return r;
}

// advc: First param was called eIndex
void CvTeam::setVassal(TeamTypes eMaster, bool bNewValue, bool bCapitulated)
{
	FAssertMsg(!bNewValue || !GET_TEAM(eMaster).isAVassal(), "can't become a vassal of a vassal");
	bool const bWasCapitulated = isCapitulated(); // advc.130v
	/*  <advc> If this function is used for turning capitulated into
		voluntary vassals at some point, then the code for processing this change
		will have to be placed before this clause, or will also have to check if
		isCapitulated()==bCapitulated here. */
	if (isVassal(eMaster) == bNewValue)
		return; // <advc>
	for (MemberIter it(getID()); it.hasNext(); ++it)
		it->updateCitySight(false, false);

	for (MemberIter it(eMaster); it.hasNext(); ++it)
	{
		CvPlayer& kLoopPlayer = *it;
		FOR_EACH_UNIT(pLoopUnit, kLoopPlayer)
		{
			CvPlot const& kPlot = pLoopUnit->getPlot();
			if (pLoopUnit->getTeam() != kPlot.getTeam() &&
				(kPlot.getTeam() == NO_TEAM ||
				!GET_TEAM(kPlot.getTeam()).isVassal(pLoopUnit->getTeam())))
			{
				kLoopPlayer.changeNumOutsideUnits(-1);
			}
		}
	}  // advc: Update war and war plan counters
	for (TeamIter<ALIVE> it; it.hasNext(); ++it)
	{
		CvTeamAI& t = *it;
		// <advc.003m>
		if (t.isAtWar(getID()))
		{
			t.changeAtWarCount(1, false, bNewValue);
			t.changeAtWarCount(-1, false, !bNewValue);
		} // </advc.003m>
		// <advc.opt> War plans against vassals don't count
		if (bNewValue)
			t.AI_updateWarPlanCounts(getID(), t.AI_getWarPlan(getID()), NO_WARPLAN);
		else t.AI_updateWarPlanCounts(getID(), NO_WARPLAN, t.AI_getWarPlan(getID()));
		// </advc.opt>
	}
	// <advc.opt>
	if (bNewValue)
	{
		FAssert(m_eMaster == NO_TEAM);
		m_eMaster = eMaster;
	}
	else
	{
		FAssert(m_eMaster == eMaster);
		m_eMaster = NO_TEAM;
	} // </advc.opt>
	// <advc.agent>
	if (bNewValue)
	{
		if (bCapitulated)
			GC.getAgents().teamCapitulated(getID(), eMaster);
		else GC.getAgents().voluntaryVassalAgreementSigned(getID(), eMaster);
	}
	else GC.getAgents().vassalFreed(getID(), eMaster);
	// </advc.agent>
	for (MemberIter it(eMaster); it.hasNext(); ++it)
	{
		CvPlayer& kLoopPlayer = *it;
		FOR_EACH_UNIT(pLoopUnit, kLoopPlayer)
		{
			CvPlot const& kPlot = pLoopUnit->getPlot();
			if (pLoopUnit->getTeam() != kPlot.getTeam() &&
				(kPlot.getTeam() == NO_TEAM ||
				!GET_TEAM(kPlot.getTeam()).isVassal(pLoopUnit->getTeam())))
			{
				kLoopPlayer.changeNumOutsideUnits(1);
			}
		}
	}

	for (MemberIter it(getID()); it.hasNext(); ++it)
		it->updateCitySight(true, false);

	CvMap const& kMap = GC.getMap();
	for (int i = 0; i < kMap.numPlots(); ++i)
	{
		CvPlot& kLoopPlot = kMap.getPlotByIndex(i);
		if (kLoopPlot.getTeam() == getID() || kLoopPlot.getTeam() == eMaster)
			kLoopPlot.updateCulture(true, false);
	}

	/*	advc.064d (note): m_bCapitulated hasn't been set yet. If the
		plot group update causes an AI city to choose new production
		immediately, there could be trouble. I don't think that can happen though. */
	GC.getGame().updatePlotGroups();

	if (isVassal(eMaster))
	{
		m_bCapitulated = bCapitulated;
		FOR_EACH_DEAL_VAR(pLoopDeal)
		{	// <advc.034>
			if (pLoopDeal->isDisengage() && pLoopDeal->isBetween(eMaster, getID()))
				pLoopDeal->kill();
			// </advc.034>
			if (!pLoopDeal->involves(getID()))
				continue;
			FOR_EACH_TRADE_ITEM(pLoopDeal->getFirstList())
			{
				if (pItem->m_eItemType == TRADE_DEFENSIVE_PACT ||
					pItem->m_eItemType == TRADE_PEACE_TREATY)
				{
					pLoopDeal->kill();
					break;
				}
			}
		}
		setForcePeace(eMaster, false);
		GET_TEAM(eMaster).setForcePeace(getID(), false);
		// <advc.130o> Forget tribute demands
		for (MemberIter itMember(getID()); itMember.hasNext(); ++itMember)
		{
			for (PlayerIter<MAJOR_CIV> itOther; itOther.hasNext(); ++itOther)
				itMember->AI_setMemoryCount(itOther->getID(), MEMORY_MADE_DEMAND, 0);
		} // </advc.130o>
		// advc.130y: Used to be done after declaring wars
		for (TeamIter<ALIVE,VASSAL_OF> it(getID()); it.hasNext(); ++it)
		{
			CvTeam& kVassalOfVassal = *it;
			freeVassal(kVassalOfVassal.getID());
			// <advc.130y>
			if(kVassalOfVassal.isAtWar(eMaster))
				kVassalOfVassal.makePeace(eMaster);
			// </advc.130y>
		}
		// Align war/peace and hasMet
		for (TeamIter<MAJOR_CIV> it; it.hasNext(); ++it)
		{
			TeamTypes eThirdParty = it->getID();
			if (eThirdParty == getID() || eThirdParty == eMaster)
				continue;
			if (GET_TEAM(eMaster).isHasMet(eThirdParty))
				meet(eThirdParty, true);

			if (isHasMet(eThirdParty))
				GET_TEAM(eMaster).meet(eThirdParty, true);

			if (GET_TEAM(eMaster).isAtWar(eThirdParty))
			{
				//declareWar((TeamTypes)iI, false, WARPLAN_DOGPILE);
				// kekm.26: "These wars declared by capitulated vassal don't trigger defensive pacts."
				queueWar(getID(), eThirdParty, false, WARPLAN_DOGPILE, !bCapitulated);
			}
			else if (isAtWar(eThirdParty))
			{
				if (bCapitulated)
					makePeace(eThirdParty);
				else
				{
					//GET_TEAM(eMaster).declareWar((TeamTypes)iI, false, WARPLAN_DOGPILE);
					// kekm.26:
					queueWar(eMaster, eThirdParty, false, WARPLAN_DOGPILE);
				}
			}
		}
		triggerWars(); // kekm.26
		for (TeamIter<MAJOR_CIV> it; it.hasNext(); ++it)
		{
			CvTeamAI& kRival = *it;
			if (!kRival.isAtWar(getID()))
			{
				kRival.AI_setWarPlan(getID(), NO_WARPLAN);
				AI().AI_setWarPlan(kRival.getID(), NO_WARPLAN);
			}
			if (!kRival.isAtWar(eMaster))
			{
				/*  advc.104j: Third parties shouldn't discard their plans
					against the master */
				if (!getUWAI.isEnabled())
					kRival.AI_setWarPlan(eMaster, NO_WARPLAN);
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
			GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(), szReplayMessage,
					-1, -1, GC.getColorType("HIGHLIGHT_TEXT"));

			for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
			{
				CvPlayer& kObs = *it;
				if ((isHasMet(kObs.getTeam()) &&
					GET_TEAM(eMaster).isHasMet(kObs.getTeam())) ||
					kObs.isSpectator()) // advc.127
				{
					gDLL->UI().addMessage(kObs.getID(), false, -1, szReplayMessage,
							"AS2D_WELOVEKING", MESSAGE_TYPE_MAJOR_EVENT, NULL, GC.getColorType("HIGHLIGHT_TEXT"),
							// advc.127b:
							getCapitalX(kObs.getTeam(), true), getCapitalY(kObs.getTeam(), true));
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

			GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(), szReplayMessage,
					-1, -1, GC.getColorType("HIGHLIGHT_TEXT"));

			for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
			{
				CvPlayer& kObs = *it;
				CvWString szBuffer;
				if (getID() == kObs.getTeam() || eMaster == kObs.getTeam() ||
					(isHasMet(kObs.getTeam()) &&
					GET_TEAM(eMaster).isHasMet(kObs.getTeam())) ||
					kObs.isSpectator()) // advc.127
				{
					szBuffer = szReplayMessage;
				}
				if (!szBuffer.empty())
				{
					gDLL->UI().addMessage(kObs.getID(), false, -1, szBuffer,
							"AS2D_REVOLTSTART", MESSAGE_TYPE_MAJOR_EVENT, NULL,
							GC.getColorType("HIGHLIGHT_TEXT"),
							// advc.127b:
							getCapitalX(kObs.getTeam(), true), getCapitalY(kObs.getTeam(), true));
				}
			}
		}
		// <advc.130y>
		// Don't forgive if it's apparent that we haven't fought wars as a vassal
		if(m_bCapitulated && AI().AI_getSharedWarSuccess(eMaster) +
			GET_TEAM(eMaster).AI_getSharedWarSuccess(getID()) > 0)
		{
			for (TeamIter<MAJOR_CIV> it; it.hasNext(); ++it)
			{
				CvTeamAI& kRival = *it;
				if (kRival.getID() != getID() && kRival.getID() != eMaster)
				{
					AI().AI_forgiveEnemy(kRival.getID(), true, true);
					kRival.AI_forgiveEnemy(getID(), true, true);
				}
			}
		} // </advc.130y>
		m_bCapitulated = false;
		// <advc.133>
		FOR_EACH_DEAL_VAR(d)
		{
			if (d->involves(getID()))
			{
				// Treat deal as very old so that turnsToCancel returns 0
				d->setInitialGameTurn(-100);
			}
		} // </advc.133>
		// <advc.104j> Stop any war plans that eMaster may have forced on us
		for (TeamIter<MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> it(eMaster); it.hasNext(); ++it)
		{
			CvTeamAI const& kRival = *it;
			if(!kRival.isAtWar(getID()))
				AI().AI_setWarPlan(kRival.getID(), NO_WARPLAN);
		} // </advc.104j>
	}

	for (MemberIter it(eMaster); it.hasNext(); ++it)
		it->updateMaintenance();

	// <advc.130v> Border conflicts of capitulated vassal are held against the master
	if (isCapitulated())
	{
		for (PlayerIter<MAJOR_CIV> itOther; itOther.hasNext(); ++itOther)
		{
			for (MemberIter itMember(eMaster); itMember.hasNext(); ++itMember)
				itOther->AI_updateCloseBorderAttitude(itMember->getID());
		}
	} // </advc.130v>

	if (GC.getGame().isFinalInitialized() && !gDLL->GetWorldBuilderMode())
		CvEventReporter::getInstance().vassalState(eMaster, getID(), bNewValue);
	/*  <advc.001> Replacing K-Mod code that had only updated attitude between
		members of the vassal and master team. Attitude of third parties may change too. */
	for (TeamTypes eTeam = getID(); eTeam != NO_TEAM; eTeam =
		(eTeam == getID() ? eMaster : NO_TEAM))
	{
		for (MemberIter itMember(eTeam); itMember.hasNext(); ++itMember)
		{
			for (PlayerIter<MAJOR_CIV,NOT_SAME_TEAM_AS> itOther(eTeam);
				itOther.hasNext(); ++itOther)
			{
				itMember->AI_updateAttitude(itOther->getID());
				itOther->AI_updateAttitude(itMember->getID());
			}
		}
	} // </advc.001>
	// <advc.014> Early re-election if vote source owner capitulates
	if(isCapitulated())
		GC.getGame().updateSecretaryGeneral(); // </advc.014>
	// <advc.143b>
	if (isCapitulated())
	{
		for (MemberIter it(getID()); it.hasNext(); ++it)
		{
			FOR_EACH_UNIT_VAR(u, *it)
			{
				if(u->getUnitInfo().getNukeRange() >= 0) // non-nukes have -1
					u->scrap();
			}
		}
	} // </advc.143b>
	// <advc.130v>
	if(bNewValue && bCapitulated)
	{
		for (PlayerIter<HUMAN,MEMBER_OF> it(eMaster); it.hasNext(); ++it)
			it->setEspionageSpendingWeightAgainstTeam(getID(), 0);
	} // </advc.130v>
	// <advc.130f> Delete stopped-trading memory
	if(bWasCapitulated != bCapitulated)
	{
		for (MemberIter itMember(getID()); itMember.hasNext(); ++itMember)
		{
			CvPlayerAI& kOurMember = *itMember;
			for (PlayerIter<MAJOR_CIV,NOT_SAME_TEAM_AS> itOther(getID());
				itOther.hasNext(); ++itOther)
			{
				CvPlayerAI& kOther = *itOther;
				kOurMember.AI_setMemoryCount(kOther.getID(), MEMORY_STOPPED_TRADING, 0);
				kOurMember.AI_setMemoryCount(kOther.getID(), MEMORY_STOPPED_TRADING_RECENT, 0);
				kOurMember.AI_setMemoryCount(kOther.getID(), MEMORY_DECLARED_WAR_RECENT, 0);
				kOther.AI_setMemoryCount(kOurMember.getID(), MEMORY_STOPPED_TRADING, 0);
				kOther.AI_setMemoryCount(kOurMember.getID(), MEMORY_STOPPED_TRADING_RECENT, 0);
				kOther.AI_setMemoryCount(kOurMember.getID(), MEMORY_DECLARED_WAR_RECENT, 0);
			}
		}
	} // </advc.130f
}


void CvTeam::assignVassal(TeamTypes eVassal, bool bSurrender) const
{
	GET_TEAM(eVassal).setVassal(getID(), true, bSurrender);
	CLinkList<TradeData> ourList;
	CLinkList<TradeData> vassalList;
	vassalList.insertAtEnd(TradeData(bSurrender ? TRADE_SURRENDER : TRADE_VASSAL, 1));
	for (MemberIter itVassal(eVassal); itVassal.hasNext(); ++itVassal)
	{
		for (MemberIter itMember(getID()); itMember.hasNext(); ++itMember)
		{
			GC.getGame().implementDeal(itMember->getID(), itVassal->getID(),
					ourList, vassalList, true);
		}
	}
}


void CvTeam::freeVassal(TeamTypes eVassal) const
{
	FOR_EACH_DEAL_VAR(pLoopDeal)
	{
		if (!pLoopDeal->isBetween(getID(), eVassal))
			continue;
		FOR_EACH_TRADE_ITEM(pLoopDeal->getGivesList(eVassal))
		{
			if (pItem->m_eItemType == TRADE_VASSAL ||
				pItem->m_eItemType == TRADE_SURRENDER)
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
	for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
	{
		CvPlayerAI const& kOther = *it;
		if(kOther.getTeam() == getMasterTeam() || kOther.getTeam() == eVassal)
			continue;
		if(kVassalLeader.AI_getMemoryCount(kOther.getID(),
			MEMORY_CANCELLED_VASSAL_AGREEMENT) < 2)
		{
			/*  2 memory, that's 10 turns on average, and guarantees that it can't
				go to 0 in just one turn. */
			kVassalLeader.AI_changeMemoryCount(kOther.getID(),
					MEMORY_CANCELLED_VASSAL_AGREEMENT, 2);
		}
	} // </advc.130y>
}

/*  <kekm.26> "Changed how multiple war declarations work. declareWar used to
	nest war declarations, now they are queued to trigger defensive pacts and
	everything else in the correct order." */
void CvTeam::queueWar(TeamTypes eAttackingTeam, TeamTypes eDefendingTeam,
		bool bNewDiplo, WarPlanTypes eWarPlan, bool bPrimaryDOW)
{
	attacking_queue.push(eAttackingTeam);
	defending_queue.push(eDefendingTeam);
	newdiplo_queue.push(bNewDiplo);
	warplan_queue.push(eWarPlan);
	primarydow_queue.push(bPrimaryDOW);
}

void CvTeam::triggerWars(bool bForceUpdateAttitude)
{
	bool bWarsDeclared = false;
	if(bTriggeringWars)
		return;
	else bTriggeringWars = true;
	while(!attacking_queue.empty())
	{
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
	if(bWarsDeclared /* advc: */ || bForceUpdateAttitude)
	{
		// from declareWar (K-Mod code)
		for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
			it->AI_updateAttitude();
	}
	bTriggeringWars = false;
} // </kekm.26>


void CvTeam::changeRouteChange(RouteTypes eIndex, int iChange)
{
	m_aiRouteChange.add(eIndex, iChange);
}


int CvTeam::getProjectDefaultArtType(ProjectTypes eIndex) const
{
	return m_aiProjectDefaultArtTypes.get(eIndex);
}


void CvTeam::setProjectDefaultArtType(ProjectTypes eIndex, int iValue)
{
	m_aiProjectDefaultArtTypes.set(eIndex, iValue);
}

int CvTeam::getProjectArtType(ProjectTypes eIndex, int number) const
{
	FAssertEnumBounds(eIndex);
	FAssertBounds(0, getProjectCount(eIndex), number);
	return m_pavProjectArtTypes[eIndex][number];
}


void CvTeam::setProjectArtType(ProjectTypes eIndex, int number, int value)
{
	FAssertEnumBounds(eIndex);
	FAssertBounds(0, getProjectCount(eIndex), number);
	m_pavProjectArtTypes[eIndex][number] = value;
}


bool CvTeam::isProjectMaxedOut(ProjectTypes eIndex, int iExtra) const
{
	if (!GC.getInfo(eIndex).isTeamProject())
		return false;

	return (getProjectCount(eIndex) + iExtra >= GC.getInfo(eIndex).getMaxTeamInstances());
}


bool CvTeam::isProjectAndArtMaxedOut(ProjectTypes eIndex) const
{
	if(getProjectCount(eIndex) < GC.getInfo(eIndex).getMaxTeamInstances())
		return false;
	int iCount = getProjectCount(eIndex);
	for (int i = 0; i < iCount; i++)
	{
		if(getProjectArtType(eIndex, i) == -1) //undefined
			return false;
	}
	return true;
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
	if(iChange == 0)
		return;

	GC.getGame().incrementProjectCreatedCount(eIndex, iChange);

	int iOldProjectCount = getProjectCount(eIndex);

	m_aiProjectCount.add(eIndex, iChange);
	FAssert(getProjectCount(eIndex) >= 0);
	// advc: Moved from isProjectMaxedOut
	FAssert(!GC.getInfo(eIndex).isTeamProject() || getProjectCount(eIndex) <= GC.getInfo(eIndex).getMaxTeamInstances());

	//adjust default art types
	if (iChange >= 0)
	{
		int iDefaultType = -1;
		for(int i = 0; i < iChange; i++)
			m_pavProjectArtTypes[eIndex].push_back(iDefaultType);
	}
	else
	{
		for(int i = 0; i < -iChange; i++)
			m_pavProjectArtTypes[eIndex].pop_back();
	}
	FAssertMsg(getProjectCount(eIndex) == m_pavProjectArtTypes[eIndex].size(), "[Jason] Unbalanced project art types.");

	CvProjectInfo& kProject = GC.getInfo(eIndex);

	changeNukeInterception(kProject.getNukeInterception() * iChange);

	if (kProject.getTechShare() > 0 && kProject.getTechShare() <= MAX_TEAMS)
		changeTechShareCount((TeamTypes)(kProject.getTechShare() - 1), iChange);

	FOR_EACH_ENUM(Victory)
	{
		if (kProject.getVictoryThreshold(eLoopVictory) > 0)
		{
			setCanLaunch(eLoopVictory, GC.getGame().testVictory(eLoopVictory, getID()));
		}
	}

	if (iChange <= 0)
		return;

	if (kProject.getEveryoneSpecialUnit() != NO_SPECIALUNIT)
		GC.getGame().makeSpecialUnitValid((SpecialUnitTypes)kProject.getEveryoneSpecialUnit());

	if (kProject.getEveryoneSpecialBuilding() != NO_SPECIALBUILDING)
		GC.getGame().makeSpecialBuildingValid((SpecialBuildingTypes)kProject.getEveryoneSpecialBuilding());

	if (kProject.isAllowsNukes())
		GC.getGame().makeNukesValid(true);

	for (MemberIter it(getID()); it.hasNext(); ++it)
	{
		CvPlayerAI& kAIMember = *it;
		if (kAIMember.isHuman())
			continue;

		bool bChangeProduction = false;
		FOR_EACH_ENUM(Project)
		{
			if (getProjectCount(eIndex) >= GC.getInfo(eLoopProject).getProjectsNeeded(eIndex) &&
				iOldProjectCount < GC.getInfo(eLoopProject).getProjectsNeeded(eIndex))
			{
				bChangeProduction = true;
				break;
			}
		}
		if (bChangeProduction)
			kAIMember.AI_makeProductionDirty();
	}

	if (GC.getGame().isFinalInitialized() && !gDLL->GetWorldBuilderMode())
	{
		CvWString szBuffer = gDLL->getText( // <advc.008e>
				GC.getInfo(eIndex).nameNeedsArticle() ?
				"TXT_KEY_MISC_COMPLETES_PROJECT_THE" :
				"TXT_KEY_MISC_COMPLETES_PROJECT", // </advc.008e>
				getReplayName().GetCString(), kProject.getTextKeyWide());
		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(), szBuffer,
				-1, -1, GC.getColorType("HIGHLIGHT_TEXT"));

		for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
		{
			CvPlayer const& kObs = *it;
			szBuffer = gDLL->getText( // <advc.008e>
					GC.getInfo(eIndex).nameNeedsArticle() ?
					"TXT_KEY_MISC_SOMEONE_HAS_COMPLETED_THE" :
					"TXT_KEY_MISC_SOMEONE_HAS_COMPLETED", // </advc.008e>
					getName().GetCString(), kProject.getTextKeyWide());
			gDLL->UI().addMessage(kObs.getID(), false, -1, szBuffer,
					"AS2D_PROJECT_COMPLETED", MESSAGE_TYPE_MAJOR_EVENT, NULL,
					GC.getColorType("HIGHLIGHT_TEXT"),
					// advc.127b:
					getCapitalX(kObs.getTeam(), true), getCapitalY(kObs.getTeam(), true));
		}
	}
}


void CvTeam::changeProjectMaking(ProjectTypes eIndex, int iChange)
{
	m_aiProjectMaking.add(eIndex, iChange);
	FAssert(getProjectMaking(eIndex) >= 0);
}


bool CvTeam::isUnitClassMaxedOut(UnitClassTypes eIndex, int iExtra) const
{
	if (!GC.getInfo(eIndex).isTeamUnit())
		return false;
	return (getUnitClassCount(eIndex) + iExtra >= GC.getInfo(eIndex).getMaxTeamInstances());
}


void CvTeam::changeUnitClassCount(UnitClassTypes eIndex, int iChange)
{
	m_aiUnitClassCount.add(eIndex, iChange);
	FAssert(getUnitClassCount(eIndex) >= 0);
	// advc: Moved from isUnitClassMaxedOut
	FAssert(!GC.getInfo(eIndex).isTeamUnit() || getUnitClassCount(eIndex) <= GC.getInfo(eIndex).getMaxTeamInstances());
}


bool CvTeam::isBuildingClassMaxedOut(BuildingClassTypes eIndex, int iExtra) const
{
	if (!GC.getInfo(eIndex).isTeamWonder())
		return false;
	return (getBuildingClassCount(eIndex) + iExtra >= GC.getInfo(eIndex).getMaxTeamInstances());
}


void CvTeam::changeBuildingClassCount(BuildingClassTypes eIndex, int iChange)
{
	m_aiBuildingClassCount.add(eIndex, iChange);
	FAssert(getBuildingClassCount(eIndex) >= 0);
	// advc: Moved from isBuildingClassMaxedOut
	FAssert(!GC.getInfo(eIndex).isTeamWonder() || getBuildingClassCount(eIndex) <= GC.getInfo(eIndex).getMaxTeamInstances());
}


void CvTeam::changeObsoleteBuildingCount(BuildingTypes eIndex, int iChange)
{
	if(iChange == 0)
		return;

	bool bOldObsoleteBuilding = isObsoleteBuilding(eIndex);

	m_aiObsoleteBuildingCount.add(eIndex, iChange);
	FAssert(getObsoleteBuildingCount(eIndex) >= 0);

	if (bOldObsoleteBuilding == isObsoleteBuilding(eIndex))
		return;

	for (MemberIter it(getID()); it.hasNext(); ++it)
	{
		FOR_EACH_CITY_VAR(pLoopCity, *it)
		{
			if (pLoopCity->getNumBuilding(eIndex) > 0)
			{
				pLoopCity->processBuilding(eIndex, isObsoleteBuilding(eIndex) ?
						-pLoopCity->getNumBuilding(eIndex) :
						pLoopCity->getNumBuilding(eIndex), true);
			}
		}
	}
}

/*	advc (note): If the tech is already known, then the research progress
	is equal to the number of beakers that this team has put into it;
	i.e. it's not necessarily greater than the tech cost, can even be 0. */
int CvTeam::getResearchProgress(TechTypes eIndex) const
{
	if (eIndex != NO_TECH)
		return m_aiResearchProgress.get(eIndex);
	return 0;
}


void CvTeam::setResearchProgress(TechTypes eIndex, int iNewValue, PlayerTypes ePlayer)
{
	if(getResearchProgress(eIndex) == iNewValue)
		return;

	m_aiResearchProgress.set(eIndex, iNewValue);
	FAssert(getResearchProgress(eIndex) >= 0);

	if (getID() == GC.getGame().getActiveTeam())
	{
		gDLL->UI().setDirty(GameData_DIRTY_BIT, true);
		gDLL->UI().setDirty(Score_DIRTY_BIT, true);
		/*  <advc.004x> Update research-turns shown in popup (tbd.: perhaps setting
			Popup_DIRTY_BIT would suffice here?) */
		CvPlayer& kActivePlayer = GET_PLAYER(GC.getGame().getActivePlayer());
		if(kActivePlayer.getCurrentResearch() == NO_TECH &&
			kActivePlayer.isFoundedFirstCity() &&
			kActivePlayer.isHuman()) // i.e. not during Auto Play
		{
			kActivePlayer.killAll(BUTTONPOPUP_CHOOSETECH);
			kActivePlayer.chooseTech();
		} // </advc.004x>
	}

	if (getResearchProgress(eIndex) >= getResearchCost(eIndex))
	{
		int iOverflow = (100 * (getResearchProgress(eIndex) - getResearchCost(eIndex))) /
				std::max(1, GET_PLAYER(ePlayer).calculateResearchModifier(eIndex));
		GET_PLAYER(ePlayer).changeOverflowResearch(iOverflow);
		// <advc> Cleaner to subtract the overflow. Cf. comment in getResearchProgress.
		m_aiResearchProgress.add(eIndex,
				getResearchProgress(eIndex) - getResearchCost(eIndex)); // </advc>
		setHasTech(eIndex, true, ePlayer, true, true, /* advc.121: */ true);
		/*if (!GC.getGame().isMPOption(MPOPTION_SIMULTANEOUS_TURNS) && !GC.getGame().isOption(GAMEOPTION_NO_TECH_BROKERING))
			setNoTradeTech(eIndex, true);*/ // BtS
		// disabled by K-Mod. I don't know why this was here, and it conflicts with my changes to the order of the doTurn functions.
	}
}


void CvTeam::changeResearchProgress(TechTypes eIndex, int iChange, PlayerTypes ePlayer)
{
	setResearchProgress(eIndex, getResearchProgress(eIndex) + iChange, ePlayer);
}

int CvTeam::changeResearchProgressPercent(TechTypes eIndex, int iPercent, PlayerTypes ePlayer)
{
	int iBeakers = 0;

	if (iPercent != 0 && !isHasTech(eIndex))
	{
		int const iResearchCostPersent = (getResearchCost(eIndex) * iPercent) / 100; // advc
		if (iPercent > 0)
			iBeakers = std::min(getResearchLeft(eIndex), iResearchCostPersent);
		else
		{
			iBeakers = std::max(getResearchLeft(eIndex) - getResearchCost(eIndex),
					iResearchCostPersent);
		}

		changeResearchProgress(eIndex, iBeakers, ePlayer);
	}

	return iBeakers;
}


int CvTeam::getTechCount(TechTypes eIndex)		 const
{
	return m_aiTechCount.get(eIndex);
}

// BETTER_BTS_AI_MOD, General AI, 07/27/09, jdog5000:
int CvTeam::getBestKnownTechScorePercent() const
{
	int iOurTechScore = 0;
	for (MemberIter it(getID()); it.hasNext(); ++it)
		iOurTechScore = std::max(iOurTechScore, it->getTechScore());

	int iBestKnownTechScore = 0;
	for (PlayerIter<CIV_ALIVE,KNOWN_TO> it(getID()); it.hasNext(); ++it)
		iBestKnownTechScore = std::max(iBestKnownTechScore, it->getTechScore());

	FAssert(iBestKnownTechScore >= iOurTechScore /* advc: */ || isBarbarian());
	return (100 * iOurTechScore) / std::max(iBestKnownTechScore, 1);
}


void CvTeam::changeTerrainTradeCount(TerrainTypes eIndex, int iChange)
{
	if (iChange == 0)
		return;

	m_aiTerrainTradeCount.add(eIndex, iChange);
	FAssert(getTerrainTradeCount(eIndex) >= 0);

	for (MemberIter it(getID()); it.hasNext(); ++it)
		it->updatePlotGroups();
}


int CvTeam::getRiverTradeCount() const
{
	return m_iRiverTradeCount;
}


void CvTeam::changeRiverTradeCount(int iChange)
{
	if (iChange == 0)
		return;

	m_iRiverTradeCount += iChange;
	FAssert(getRiverTradeCount() >= 0);

	for (MemberIter it(getID()); it.hasNext(); ++it)
		it->updatePlotGroups();
}


int CvTeam::getVictoryCountdown(VictoryTypes eIndex) const
{
	return m_aiVictoryCountdown.get(eIndex);
}

void CvTeam::setVictoryCountdown(VictoryTypes eIndex, int iTurnsLeft)
{
	m_aiVictoryCountdown.set(eIndex, iTurnsLeft);
	// <advc.opt>
	if (iTurnsLeft >= 0)
		m_bAnyVictoryCountdown = true;
	else
	{
		m_bAnyVictoryCountdown = false;
		FOR_EACH_ENUM(Victory)
		{
			if (eLoopVictory != eIndex && getVictoryCountdown(eLoopVictory) >= 0)
				m_bAnyVictoryCountdown = true;
		}
	} // </advc.opt>
}


void CvTeam::changeVictoryCountdown(VictoryTypes eIndex, int iChange)
{
	setVictoryCountdown(eIndex, getVictoryCountdown(eIndex) + iChange); // advc: instead of m_aiVictoryCountdown.add(eIndex, iChange)
}

// <advc.opt>
bool CvTeam::isAnyVictoryCountdown() const
{
	return m_bAnyVictoryCountdown;
} // </advc.opt>


int CvTeam::getVictoryDelay(VictoryTypes eVictory) const
{
	int iExtraDelayPercent = 0;
	FOR_EACH_ENUM(Project)
	{
		CvProjectInfo& kProject = GC.getInfo(eLoopProject);
		int iCount = getProjectCount(eLoopProject);

		if (iCount < kProject.getVictoryMinThreshold(eVictory))
		{
			FAssert(false);
			return -1;
		}

		if (iCount < kProject.getVictoryThreshold(eVictory))
		{
			iExtraDelayPercent += ((kProject.getVictoryThreshold(eVictory) -
					iCount) * kProject.getVictoryDelayPercent()) /
					kProject.getVictoryThreshold(eVictory);
		}
	}

	return (GC.getGame().victoryDelay(eVictory)  * (100 + iExtraDelayPercent)) / 100;
}

void CvTeam::setCanLaunch(VictoryTypes eVictory, bool bCan)
{
	m_abCanLaunch.set(eVictory, bCan);
}

bool CvTeam::canLaunch(VictoryTypes eVictory) const
{
	return m_abCanLaunch.get(eVictory);
}

int CvTeam::getLaunchSuccessRate(VictoryTypes eVictory) const
{
	int iSuccessRate = 100;
	FOR_EACH_ENUM(Project)
	{
		CvProjectInfo& kProject = GC.getInfo(eLoopProject);
		int iCount = getProjectCount(eLoopProject);
		if (iCount < kProject.getVictoryMinThreshold(eVictory))
			return 0;

		if (iCount < kProject.getVictoryThreshold(eVictory))
		{
			if (kProject.getSuccessRate() > 0)
			{
				iSuccessRate -= (kProject.getSuccessRate() *
						(kProject.getVictoryThreshold(eVictory) - iCount));
			}
		}
	}

	return iSuccessRate;
}

void CvTeam::resetVictoryProgress()
{	// <advc.opt>
	if (!isAnyVictoryCountdown() || GC.getGame().getGameState() != GAMESTATE_ON)
		return; // </advc.opt>
	FOR_EACH_ENUM(Victory)
	{
		if (getVictoryCountdown(eLoopVictory) < 0)
			continue;

		setVictoryCountdown(eLoopVictory, -1);
		FOR_EACH_ENUM(Project)
		{
			if (GC.getInfo(eLoopProject).getVictoryMinThreshold(eLoopVictory) > 0)
				changeProjectCount(eLoopProject, -getProjectCount(eLoopProject));
		}

		CvWString szBuffer = gDLL->getText("TXT_KEY_VICTORY_RESET",
				getReplayName().GetCString(), GC.getInfo(eLoopVictory).getTextKeyWide());

		for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
		{
			CvPlayer& kObs = *it;
			gDLL->UI().addMessage(kObs.getID(), false, -1, szBuffer,
					"AS2D_MELTDOWN", MESSAGE_TYPE_MAJOR_EVENT,
					// <advc.127b>
					NULL, NO_COLOR, getCapitalX(kObs.getTeam(), true),
					getCapitalY(kObs.getTeam(), true)); // </advc.127b>
			if (kObs.getTeam() == getID())
			{
				CvPopupInfo* pInfo = new CvPopupInfo();
				pInfo->setText(szBuffer);
				gDLL->UI().addPopup(pInfo, kObs.getID());
			}
		}
		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(), szBuffer,
				-1, -1, GC.getColorType("HIGHLIGHT_TEXT"));
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
			return true;
	}

	return false;
} // K-Mod end


bool CvTeam::isParent(TeamTypes eChildTeam) const
{
	if (!GET_TEAM(eChildTeam).isVassal(getID()))
		return false;

	for (MemberIter it(eChildTeam); it.hasNext(); ++it)
	{
		PlayerTypes eLoopParent = it->getParent();
		if (eLoopParent != NO_PLAYER && TEAMID(eLoopParent) == getID())
			return true;
	}

	return false;
}

bool CvTeam::isHasTech(TechTypes eIndex) const
{
	if (eIndex == NO_TECH)
		return true;

	return m_abHasTech.get(eIndex);
}

// <advc.039>
CvWString const CvTeam::tradeItemString(TradeableItems eItem, int iData, TeamTypes eFrom) const
{
	CvTeam const& kFrom = GET_TEAM(eFrom);
	switch(eItem)
	{
	case TRADE_CITIES:
	{
		CvCity* c = GET_PLAYER(kFrom.getLeaderID()).getCity(iData);
		if (c == NULL)
			return L"";
		return c->getName();
	}
	case TRADE_CIVIC:
	{
		CivicTypes eCivic = (CivicTypes)iData;
		if (eCivic == NO_CIVIC)
			return L"";
		return gDLL->getText("TXT_KEY_MISC_ADOPTING_CIVIC", GC.getInfo(eCivic).getTextKeyWide());
	}
	case TRADE_RELIGION:
	{
		ReligionTypes eReligion = (ReligionTypes)iData;
		if (eReligion == NO_RELIGION)
			return L"";
		return gDLL->getText("TXT_KEY_MISC_CONVERTING_RELIGION", GC.getInfo(eReligion).getTextKeyWide());
	}
	case TRADE_EMBARGO:
	{
		TeamTypes eTarget = (TeamTypes)iData;
		if (eTarget == NO_TEAM)
			return L"";
		return gDLL->getText("TXT_KEY_MISC_EMBARGO_AGAINST", GET_TEAM(eTarget).getName().GetCString());
	}
	case TRADE_GOLD:
		if (iData <= 0)
			return L"";
		return gDLL->getText("TXT_KEY_MISC_CASH", iData);
	case TRADE_GOLD_PER_TURN:
		if (iData <= 0)
			return L"";
		return gDLL->getText("TXT_KEY_MISC_GPT", iData);
	case TRADE_MAPS:
		return gDLL->getText("TXT_KEY_MISC_MAPS_CIV", kFrom.getName().GetCString());
	case TRADE_TECHNOLOGIES:
	{
		TechTypes eTech = (TechTypes)iData;
		if (eTech == NO_TECH)
			return L"";
		return CvWString::format(SETCOLR L"%s" ENDCOLR,
				TEXT_COLOR("COLOR_HIGHLIGHT_TEXT"),
				GC.getInfo(eTech).getDescription());
	}
	case TRADE_PEACE:
	{
		TeamTypes ePeaceTeam = (TeamTypes)iData;
		if (ePeaceTeam == NO_TEAM)
			return L"";
		return gDLL->getText("TXT_KEY_TRADE_PEACE_WITH") +
				GET_TEAM(ePeaceTeam).getName();
	}
	}
	return L"";
} // </advc.039>

void CvTeam::announceTechToPlayers(TechTypes eIndex, /* advc.156: */ PlayerTypes eDiscoverPlayer,
	bool bPartial)
{
	CvGame const& kGame = GC.getGame();
	bool bSound = ((kGame.isNetworkMultiPlayer() ||
			/*  advc.156: I think HotSeat doesn't play sounds along with messages,
				but let's try. */
			kGame.isHotSeat() ||
			gDLL->UI().noTechSplash()) && !bPartial);
	for (MemberIter it(getID()); it.hasNext(); ++it)
	{
		CvPlayer const& kPlayer = *it;
		// <advc.156>
		TCHAR const* szSound = NULL;
		if(bSound)
		{
			if(kPlayer.getID() == eDiscoverPlayer)
				szSound = GC.getInfo(eIndex).getSound();
			else szSound = GC.getInfo(eIndex).getSoundMP();
		} // </advc.156>
		CvWString szBuffer = gDLL->getText((bPartial ?
				"TXT_KEY_MISC_PROGRESS_TOWARDS_TECH" :
				"TXT_KEY_MISC_YOU_DISCOVERED_TECH"),
				GC.getInfo(eIndex).getTextKeyWide());
		gDLL->UI().addMessage(kPlayer.getID(), false, -1, szBuffer,
				szSound, // advc.156
				MESSAGE_TYPE_MINOR_EVENT, // advc.106b
				NULL, GC.getColorType("TECH_TEXT"));
				// K-Mod. Play the quote sound always, the "MP" sound is boring.
				//(bSound ? GC.getInfo(eIndex).getSound() : NULL)
	}
}

void CvTeam::setHasTech(TechTypes eTech, bool bNewValue, PlayerTypes ePlayer,
	bool bFirst, bool bAnnounce, /* advc.121: */ bool bEndOfTurn)
{
	PROFILE_FUNC();

	if (eTech == NO_TECH)
		return;

	if (isHasTech(eTech) == bNewValue)
		return;

	if (ePlayer == NO_PLAYER)
		ePlayer = getLeaderID();

	CvGame& kGame = GC.getGame();
	CvTechInfo const& kTech = GC.getInfo(eTech);

	if (kTech.isRepeat())
	{
		m_aiTechCount.add(eTech, 1);
		setResearchProgress(eTech, 0, ePlayer);
		CvEventReporter::getInstance().techAcquired(eTech, getID(), ePlayer,
				bAnnounce && 1 == m_aiTechCount.get(eTech));

		if (m_aiTechCount.get(eTech) == 1 && bAnnounce &&
			kGame.isFinalInitialized() && !gDLL->GetWorldBuilderMode())
		{
			announceTechToPlayers(eTech, /* advc.156: */ ePlayer);
		}
	}
	else
	{
		updatePlotGroupBonus(eTech, false); // advc: Code moved into auxiliary function
		m_abHasTech.set(eTech, bNewValue);
		m_iTechCount++; // advc.101
		updatePlotGroupBonus(eTech, true);
	}

	processTech(eTech, bNewValue ? 1 : -1, /* advc.121: */ bEndOfTurn);

	if (isHasTech(eTech))
	{
		if (gTeamLogLevel >= 2) logBBAI("    Team %d (%S) acquires tech %S", getID(), getName().GetCString(), kTech.getDescription()); // BETTER_BTS_AI_MOD, AI logging, 10/02/09, jdog5000

		for (MemberIter it(getID()); it.hasNext(); ++it)
		{
			CvPlayer& kMember = *it;
			if (kMember.getCurrentEra() < kTech.getEra())
				kMember.setCurrentEra((EraTypes)kTech.getEra());
		}

		if (kTech.isMapVisible())
			GC.getMap().setRevealedPlots(getID(), true, true);

		FOR_EACH_ENUM(SpecialBuilding)
		{
			if (eTech == GC.getInfo(eLoopSpecialBuilding).getTechPrereqAnyone())
				kGame.makeSpecialBuildingValid(eLoopSpecialBuilding, bAnnounce);
		}

		// report event to Python, along with some other key state
		CvEventReporter::getInstance().techAcquired(eTech, getID(), ePlayer, bAnnounce);

		bool bReligionFounded = false;
		bool bFirstPerk = false; // advc: Reneamed from bFirstBonus
		bool const bFirstToDiscover = (kGame.countKnownTechNumTeams(eTech) == 1); // advc.106
		if (bFirst && bFirstToDiscover &&
			!GC.getPythonCaller()->doOrganizationTech(getID(), ePlayer, eTech))
		{
			FOR_EACH_ENUM(Religion)
			{
				if (GC.getInfo(eLoopReligion).getTechPrereq() != eTech)
					continue;

				int iBestValue = MAX_INT;
				PlayerTypes eBestPlayer = NO_PLAYER;
				for (MemberIter it(getID()); it.hasNext(); ++it)
				{
					CvPlayer const& kMember = *it;

					int iValue = 10;
					iValue += kGame.getSorenRandNum(10, "Found Religion (Player)");
					for (int iK = 0; iK < GC.getNumReligionInfos(); iK++)
						iValue += kMember.getHasReligionCount((ReligionTypes)iK) * 10;

					if (kMember.getCurrentResearch() != eTech)
						iValue *= 10;

					if (iValue < iBestValue)
					{
						iBestValue = iValue;
						eBestPlayer = kMember.getID();
					}
				}
				if (eBestPlayer == NO_PLAYER)
					continue;

				kGame.setReligionSlotTaken(eLoopReligion, true);
				if (kGame.isOption(GAMEOPTION_PICK_RELIGION))
				{
					if (GET_PLAYER(eBestPlayer).isHuman())
					{
						CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_FOUND_RELIGION, eLoopReligion);
						if (pInfo != NULL)
							gDLL->UI().addPopup(pInfo, eBestPlayer);
					}
					else
					{
						ReligionTypes eFoundReligion = GET_PLAYER(eBestPlayer).AI_chooseReligion();
						if (eFoundReligion != NO_RELIGION)
							GET_PLAYER(eBestPlayer).foundReligion(eFoundReligion, eLoopReligion, true);
					}
				}
				else GET_PLAYER(eBestPlayer).foundReligion(eLoopReligion, eLoopReligion, true);
				bReligionFounded = true;
				bFirstPerk = true;
			}
			FOR_EACH_ENUM(Corporation)
			{
				CorporationTypes const eCorp = eLoopCorporation;
				if (GC.getInfo(eCorp).getTechPrereq() != eTech ||
					kGame.isCorporationFounded(eCorp))
				{
					continue;
				}
				/*  advc (comment): From here on unused and thus not properly tested;
					see comment in CvGame::doHeadquarters. */
				int iBestValue = MAX_INT;
				PlayerTypes eBestPlayer = NO_PLAYER;
				for (MemberIter it(getID()); it.hasNext(); ++it)
				{
					CvPlayer const& kMember = *it;
					int iValue = 10;
					iValue += kGame.getSorenRandNum(10, "Found Corporation (Player)");
					if (kMember.getCurrentResearch() != eTech)
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
				bFirstPerk = true;
			}
		}

		for (MemberIter it(getID()); it.hasNext(); ++it)
		{
			CvPlayerAI& kMember = *it;
			if (kMember.isResearchingTech(eTech))
				kMember.popResearch(eTech);
			// notify the player they now have the tech, if they want to make immediate changes
			kMember.AI_nowHasTech(eTech);
			kMember.invalidateYieldRankCache();
		}

		if (bFirst && bFirstToDiscover)
		{
			bool bAnnounceFirst = false; // advc.004
			CvWString szBuffer;
			UnitTypes eFreeUnit = GET_PLAYER(ePlayer).getTechFreeUnit(eTech);
			if (eFreeUnit != NO_UNIT)
			{
				bFirstPerk = true;
				bAnnounceFirst = true; // advc.004
				CvCity* pCapital = GET_PLAYER(ePlayer).getCapital();
				if (pCapital != NULL)
					pCapital->createGreatPeople(eFreeUnit, false, false);
			}

			if (kTech.getFirstFreeTechs() > 0)
			{
				bFirstPerk = true;
				bAnnounceFirst = true; // advc.004

				if (!isHuman())
				{
					for (int iI = 0; iI < kTech.getFirstFreeTechs(); iI++)
					{
						GET_PLAYER(ePlayer).AI_chooseFreeTech();
					}
				}
				else
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_FIRST_TECH_CHOOSE_FREE", kTech.getTextKeyWide());
					GET_PLAYER(ePlayer).chooseTech(kTech.getFirstFreeTechs(), szBuffer.GetCString());
				}
				// advc.004: Announcement code moved into next block
				// advc.106: Do it at the end instead
				if (GC.getDefineINT("SHOW_FIRST_TO_DISCOVER_IN_REPLAY") <= 0)
				{
					szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_FIRST_TO_TECH",
							GET_PLAYER(ePlayer).getReplayName(), kTech.getTextKeyWide());
					kGame.addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, ePlayer, szBuffer,
							-1, -1, GC.getColorType("HIGHLIGHT_TEXT"));
				} // advc.106
			} // <advc.004>
			if (bAnnounceFirst) // Cut, pasted, refactored from above
			{
				// Free GP only minor event
				bool bMajor = (kTech.getFirstFreeTechs() > 0);
				for (PlayerIter<MAJOR_CIV,NOT_SAME_TEAM_AS> it(TEAMID(ePlayer)); it.hasNext(); ++it)
				{
					CvPlayer const& kObs = *it;
					if (isHasMet(kObs.getTeam()) || /* advc.127: */ kObs.isSpectator())
					{
						szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_FIRST_TO_TECH",
								GET_PLAYER(ePlayer).getNameKey(), kTech.getTextKeyWide());
					}
					else szBuffer = gDLL->getText("TXT_KEY_MISC_UNKNOWN_FIRST_TO_TECH",
							kTech.getTextKeyWide());
					gDLL->UI().addMessage(kObs.getID(), false, -1, szBuffer,
							(bMajor ? "AS2D_FIRSTTOTECH" : NULL),
							(bMajor ? MESSAGE_TYPE_MAJOR_EVENT :
							MESSAGE_TYPE_MINOR_EVENT), NULL, GC.getColorType("HIGHLIGHT_TEXT"),
							// advc.127b:
							getCapitalX(kObs.getTeam(), true), getCapitalY(kObs.getTeam(), true));
				}
			} // </advc.004>
			if (bFirstPerk)
			{
				for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
				{
					CvPlayerAI& kOther = *it;
					if (!kOther.isHuman() && kOther.isResearchingTech(eTech))
					{
						/*	K-Mod note: we just want to flag it for re-evaluation.
							Clearing the queue is currently the only way to do that. */
						kOther.clearResearchQueue();
					}
				}
			}
		}


		if (bAnnounce && kGame.isFinalInitialized() &&
			!gDLL->GetWorldBuilderMode()) // advc
		{
			announceTechToPlayers(eTech, /* advc.156: */ ePlayer);
			bool bMessageSent = false; // advc.004r
			bool bAnyDiscovered = false; // advc.004r
			CvMap const& kMap = GC.getMap();
			for (int i = 0; i < kMap.numPlots(); i++)
			{
				CvPlot const& kLoopPlot = kMap.getPlotByIndex(i);
				if (!kLoopPlot.isRevealed(getID()))
					continue;
				BonusTypes eBonus = kLoopPlot.getBonusType();
				if (eBonus == NO_BONUS)
					continue;
				if (GC.getInfo(eBonus).getTechReveal() != eTech ||
					isForceRevealedBonus(eBonus))
				{
					continue;
				}
				// <advc.004r>
				bAnyDiscovered = true;
				TeamTypes eRevealedTeam = kLoopPlot.getRevealedTeam(getID(), false);
				if (eRevealedTeam != getID() && eRevealedTeam != NO_TEAM &&
					eRevealedTeam != BARBARIAN_TEAM &&
					!GET_TEAM(eRevealedTeam).isVassal(getID())) // </advc.004r>
				{
					continue;
				}
				CvCity const* pCity = kMap.findCity(kLoopPlot.getX(), kLoopPlot.getY(), NO_PLAYER,
						// advc.004r: Pass ID as eObserver (last param) instead of city owner
						NO_TEAM, false, false, NO_TEAM, NO_DIRECTION, NULL, getID());
				if (pCity == NULL)
					continue;
				CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_DISCOVERED_BONUS",
						GC.getInfo(eBonus).getTextKeyWide(), pCity->getNameKey());
				/*  <advc.004r> Announce to all team members (instead of
					plot owner, which may not even be alive) */
				for (MemberIter it(getID()); it.hasNext(); ++it)
				{
					bMessageSent = true;
					gDLL->UI().addMessage(it->getID(), // </advc.004r>
							false, -1, szBuffer, kLoopPlot, "AS2D_DISCOVERBONUS",
							MESSAGE_TYPE_INFO, GC.getInfo(eBonus).getButton());
				}
			}
			// <advc.004r> Report no sources
			if (!bMessageSent && !isBarbarian() && !isMinorCiv())
			{
				BonusTypes eBonus = NO_BONUS;
				FOR_EACH_ENUM(Bonus)
				{
					if(GC.getInfo(eLoopBonus).getTechReveal() == eTech)
					{
						eBonus = eLoopBonus;
						break;
					}
				}
				if (eBonus != NO_BONUS)
				{
					CvWString szOwnershipStatus;
					if (bAnyDiscovered)
						szOwnershipStatus = gDLL->getText("TXT_KEY_MISC_DISCOVERED_UNCLAIMED");
					CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_DISCOVERED_NO_BONUS",
							szOwnershipStatus.GetCString(), GC.getInfo(eBonus).getTextKeyWide());
					for (MemberIter it(getID()); it.hasNext(); ++it)
					{
						gDLL->UI().addMessage(it->getID(), false, -1, szBuffer
								/*,"AS2D_DISCOVERBONUS"*/); // Don't play the sound
					}
				}
			} // </advc.004r>
		}
		/*  advc.004x: Don't check bAnnounce for civics popup. FinalInitialized:
			Let CvPlayer::doChangeCivicsPopup handle that. */
		if (!gDLL->GetWorldBuilderMode() && getID() == kGame.getActiveTeam())
		{
			for (PlayerIter<HUMAN,MEMBER_OF> it(getID()); it.hasNext(); ++it)
			{	// advc: Un-nested the conditions
				CvPlayer& kMember = *it;
				if (kMember.getID() != ePlayer || !bReligionFounded ||
					kMember.getLastStateReligion() != NO_RELIGION /*&&
					kMember.canRevolution(NULL)*/) // advc.004x
				{
					CivicTypes eCivic = NO_CIVIC;
					FOR_EACH_ENUM(CivicOption)
					{
						if (kMember.isHasCivicOption(eLoopCivicOption))
							continue;
						FOR_EACH_ENUM(Civic)
						{
							if (GC.getInfo(eLoopCivic).getCivicOptionType() != eLoopCivicOption)
								continue;
							if (GC.getInfo(eLoopCivic).getTechPrereq() == eTech)
								eCivic = eLoopCivic;
						}
					} // <advc.004x>
					if (eCivic != NO_CIVIC && kMember.canDoCivics(eCivic))
					{
						// BtS code moved into subroutine
						kMember.doChangeCivicsPopup(eCivic);
					} // </advc.004x>
				}
			}
		}
		for (TeamIter<ALIVE,NOT_SAME_TEAM_AS> it(getID()); it.hasNext(); ++it)
			it->updateTechShare(eTech); // Share through "Internet" project
		// <advc.106>
		if (bFirst && bFirstToDiscover && // (Note: CvGame::initFreeState uses bFirst=false)
			GC.getDefineINT("SHOW_FIRST_TO_DISCOVER_IN_REPLAY") > 0)
		{
			CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_FIRST_TO_TECH",
					GET_PLAYER(ePlayer).getReplayName(), kTech.getTextKeyWide());
			kGame.addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, ePlayer, szBuffer,
					-1, -1, GC.getColorType("ALT_HIGHLIGHT_TEXT"));
		} // </advc.106>
	}

	if (bNewValue && bAnnounce && kGame.isFinalInitialized() &&
		!gDLL->GetWorldBuilderMode())
	{
		FAssert(ePlayer != NO_PLAYER);
		if (GET_PLAYER(ePlayer).isResearch() &&
			GET_PLAYER(ePlayer).getCurrentResearch() == NO_TECH &&
			GET_PLAYER(ePlayer).isHuman()) // K-Mod
		{
			CvWString szBuffer = gDLL->getText("TXT_KEY_MISC_WHAT_TO_RESEARCH_NEXT");
			GET_PLAYER(ePlayer).chooseTech(0, szBuffer);
		}
	}

	if (getID() == kGame.getActiveTeam())
	{
		gDLL->UI().setDirty(MiscButtons_DIRTY_BIT, true);
		gDLL->UI().setDirty(SelectionButtons_DIRTY_BIT, true);
		gDLL->UI().setDirty(ResearchButtons_DIRTY_BIT, true);
		gDLL->UI().setDirty(GlobeLayer_DIRTY_BIT, true);
	}
}


bool CvTeam::isNoTradeTech(TechTypes eIndex) const
{
	return m_abNoTradeTech.get(eIndex);
}


void CvTeam::setNoTradeTech(TechTypes eIndex, bool bNewValue)
{
	m_abNoTradeTech.set(eIndex, bNewValue);
}


void CvTeam::changeImprovementYieldChange(ImprovementTypes eIndex1, YieldTypes eIndex2, int iChange)
{
	if (iChange != 0)
	{
		m_aaiImprovementYieldChange.add(eIndex1, eIndex2, iChange);
		FAssert(getImprovementYieldChange(eIndex1, eIndex2) >= 0);
		updateYield();
	}
}

/*	K-Mod: In the original code, there seems to be a lot of confusion
	about what the exact conditions are for a bonus being connected.
	There were heaps of bugs where CvImprovementInfo::isImprovementBonusTrade
	was mistakenly used as the sole condition for a bonus being connected or not.
	I created this function to make the situation a bit more clear... */
bool CvTeam::doesImprovementConnectBonus(ImprovementTypes eImprovement, BonusTypes eBonus) const
{
	if (eImprovement == NO_IMPROVEMENT || eBonus == NO_BONUS)
		return false;

	CvBonusInfo const& kBonus = GC.getInfo(eBonus);
	if (!isHasTech(kBonus.getTechCityTrade()) ||
		(kBonus.getTechObsolete() != NO_TECH &&
		isHasTech(kBonus.getTechObsolete())))
	{
		return false;
	}
	CvImprovementInfo const& kImprovement = GC.getInfo(eImprovement);
	return (kImprovement.isImprovementBonusTrade(eBonus) ||
			kImprovement.isActsAsCity());
}

// advc (note): Some overlap with canPeacefullyEnter
bool CvTeam::isFriendlyTerritory(TeamTypes eTerritoryOwner) const
{
	if (eTerritoryOwner == NO_TEAM)
		return false;

	if (eTerritoryOwner == getID())
		return true;

	if (GET_TEAM(eTerritoryOwner).isVassal(getID()))
		return true;

	if (isVassal(eTerritoryOwner) && isOpenBorders(eTerritoryOwner))
		return true;

	return false;
}

/*	advc: Says whether kPlot is a land plot that sea units of this team can enter
	-- or could enter if kPlot were adjacent to water (not checked).
	Was previously handled by CvPlot::isCity(bool,TeamTypes). */
bool CvTeam::isBase(CvPlot const& kPlot) const
{
	//if (!isFriendlyTerritory(kPlot.getTeam())
	// advc.183: Lower the requirements for using foreign ports
	if (!kPlot.isOwned() || !canPeacefullyEnter(kPlot.getTeam()))
		return false;
	ImprovementTypes eImprov = kPlot.getImprovementType();
	if (eImprov != NO_IMPROVEMENT)
		return GC.getInfo(eImprov).isActsAsCity();
	return kPlot.isCity();
}

/*	advc: Like isBase, but checks the map knowledge of this team.
	advc.pf (note): TeamPathFinder doesn't call this function (too slow);
	may have to be adjusted if isRevealedBase is modified further. */
bool CvTeam::isRevealedBase(CvPlot const& kPlot) const
{
	TeamTypes const eRevealedTeam = kPlot.getRevealedTeam(getID(), false);
	if (eRevealedTeam == NO_TEAM || !canPeacefullyEnter(eRevealedTeam))
		return false;
	ImprovementTypes eRevealedImprov = kPlot.getRevealedImprovementType(getID());
	if (eRevealedImprov != NO_IMPROVEMENT)
		return GC.getInfo(eRevealedImprov).isActsAsCity();
	return (kPlot.isCity() && kPlot.getPlotCity()->isRevealed(getID()));
}

/*	advc.183: Says whether kPlot is a city friendly to this team or providing
	city-like defensive advantages to this team. No fog-of-war checks.
	Was previously handled by CvPlot::isCity(bool,TeamTypes).
	Needs to be consistent with CvPlot::defenseModifier. */
bool CvTeam::isCityDefense(CvPlot const& kPlot) const
{
	/*	I'm assuming that calls happen in the context of imminent combat, and
		then the units wouldn't be visible if the plot weren't visible as well.
		This assertion fails, however, when center units are updated while
		updating sight (in CvPlot). */
	//FAssert(kPlot.isVisible(getID()));
	if (kPlot.isOwned())
	{
		if (!isFriendlyTerritory(kPlot.getTeam()))
			return false;
		if (kPlot.isCity())
			return true;
	}
	ImprovementTypes eImprov = kPlot.getImprovementType();
	return (eImprov != NO_IMPROVEMENT && GC.getInfo(eImprov).isActsAsCity() &&
			GC.getInfo(eImprov).getDefenseModifier() > 0); // idea by Erik
}

// advc.901:
bool CvTeam::canAccessHappyHealth(CvPlot const& kPlot, int iHealthOrHappy) const
{
	return (iHealthOrHappy <= 0 || !kPlot.isOwned() || canPeacefullyEnter(kPlot.getTeam()));
}

// advc: Moved from CvGameCoreUtils
int CvTeam::getEspionageModifier(TeamTypes eTarget) const
{
	FAssert(getID() != eTarget);
	FAssert(!isBarbarian());
	// K-Mod: This is possible for legitimate reasons (although the result is never important...)
	//FAssert(eTarget != BARBARIAN_TEAM);

	CvTeam const& kTarget = GET_TEAM(eTarget);
	/*int iTargetPoints = kTarget.getEspionagePointsEver();
	int iOurPoints = getEspionagePointsEver();
	int iModifier = GC.getDefineINT("ESPIONAGE_SPENDING_MULTIPLIER") * (2 * iTargetPoints + iOurPoints);
	iModifier /= std::max(1, iTargetPoints + 2 * iOurPoints);
	return iModifier;*/ // BtS
	/*	K-Mod. Scale the points modifier based on the teams' population.
		(Note ESPIONAGE_SPENDING_MULTIPLIER is 100 in the default xml.) */
	int iPopScale = 5 * GC.getInfo(GC.getMap().getWorldSize()).getTargetNumCities();
	int iTargetPoints = 10 * kTarget.getEspionagePointsEver() /
			std::max(1, iPopScale + kTarget.getTotalPopulation(false));
	int iOurPoints = 10 * getEspionagePointsEver() /
			std::max(1, iPopScale + getTotalPopulation(false));
	static int const iESPIONAGE_SPENDING_MULTIPLIER = GC.getDefineINT("ESPIONAGE_SPENDING_MULTIPLIER"); // advc.opt
	return iESPIONAGE_SPENDING_MULTIPLIER *
			std::max(1, 2 * iTargetPoints + iOurPoints) /
			std::max(1, iTargetPoints + 2 * iOurPoints);
	// K-Mod end
}

int CvTeam::getEspionagePointsAgainstTeam(TeamTypes eIndex) const
{
	return m_aiEspionagePointsAgainstTeam.get(eIndex);
}

void CvTeam::setEspionagePointsAgainstTeam(TeamTypes eIndex, int iValue)
{
	PROFILE_FUNC(); // advc: So far not called frequently enough to be of any concern
	if (iValue != getEspionagePointsAgainstTeam(eIndex))
	{
		m_aiEspionagePointsAgainstTeam.set(eIndex, iValue);
		verifySpyUnitsValidPlot();
		GET_TEAM(eIndex).verifySpyUnitsValidPlot();
		// <advc.091>
		for (MemberIter itOurMember(getID()); itOurMember.hasNext(); ++itOurMember)
			itOurMember->updateEverSeenDemographics(eIndex); // </advc.091>
	}
}

void CvTeam::changeEspionagePointsAgainstTeam(TeamTypes eIndex, int iChange)
{
	setEspionagePointsAgainstTeam(eIndex, getEspionagePointsAgainstTeam(eIndex) + iChange);
}

// advc.120d:
bool CvTeam::canSeeTech(TeamTypes eOther) const
{
	/*	In part based on drawTechDeals in ExoticForeignAdvisor.py.
		Exposed to Python via CvPlayer. Therefore has to pretty foolproof. */
	CvTeam const& kOther = GET_TEAM(eOther);
	// Can see vassal tech through "we'd like you to research ..."
	if (getID() == kOther.getID() || kOther.isVassal(getID()))
		return true;
	// advc.553: Make tech visible despite No Tech Trading (as in BtS)
	/*if(GC.getGame().isOption(GAMEOPTION_NO_TECH_TRADING) && !canSeeResearch(eOther))
		return false;*/
	if (!isAlive() || !kOther.isAlive() ||
		!isMajorCiv() || !kOther.isMajorCiv() ||
		isMinorCiv() || kOther.isMinorCiv())
	{
		return false;
	}
	return (isHasMet(kOther.getID()) && (isTechTrading() || kOther.isTechTrading()));
}

// K-Mod:
int CvTeam::getTotalUnspentEspionage() const
{
	int iTotal = 0;
	for (TeamIter<CIV_ALIVE> it; it.hasNext(); ++it)
		iTotal += getEspionagePointsAgainstTeam(it->getID());
	return iTotal;
}

int CvTeam::getEspionagePointsEver() const
{
	return m_iEspionagePointsEver;
}

void CvTeam::setEspionagePointsEver(int iValue)
{
	if (iValue != getEspionagePointsEver())
		m_iEspionagePointsEver = iValue;
}

void CvTeam::changeEspionagePointsEver(int iChange)
{
	setEspionagePointsEver(getEspionagePointsEver() + iChange);
}

int CvTeam::getCounterespionageTurnsLeftAgainstTeam(TeamTypes eIndex) const
{
	return m_aiCounterespionageTurnsLeftAgainstTeam.get(eIndex);
}

void CvTeam::setCounterespionageTurnsLeftAgainstTeam(TeamTypes eIndex, int iValue)
{
	if (iValue != getCounterespionageTurnsLeftAgainstTeam(eIndex))
	{
		m_aiCounterespionageTurnsLeftAgainstTeam.set(eIndex, iValue);
		gDLL->UI().setDirty(Espionage_Advisor_DIRTY_BIT, true);
	}
}

void CvTeam::changeCounterespionageTurnsLeftAgainstTeam(TeamTypes eIndex, int iChange)
{
	setCounterespionageTurnsLeftAgainstTeam(eIndex, getCounterespionageTurnsLeftAgainstTeam(eIndex) + iChange);
}

int CvTeam::getCounterespionageModAgainstTeam(TeamTypes eIndex) const
{
	return m_aiCounterespionageModAgainstTeam.get(eIndex);
}

void CvTeam::setCounterespionageModAgainstTeam(TeamTypes eIndex, int iValue)
{
	if (iValue != getCounterespionageModAgainstTeam(eIndex))
	{
		m_aiCounterespionageModAgainstTeam.set(eIndex, iValue);
		gDLL->UI().setDirty(Espionage_Advisor_DIRTY_BIT, true);
	}
}

void CvTeam::changeCounterespionageModAgainstTeam(TeamTypes eIndex, int iChange)
{
	setCounterespionageModAgainstTeam(eIndex, getCounterespionageModAgainstTeam(eIndex) + iChange);
}

void CvTeam::verifySpyUnitsValidPlot()
{
	std::vector<CvUnit*> aUnits;
	for (MemberIter it(getID()); it.hasNext(); ++it)
	{
		CvPlayer const& kMember = *it;
		FOR_EACH_UNIT_VAR(pUnit, kMember)
		{
			PlayerTypes eOwner = pUnit->getPlot().getOwner();
			if (eOwner != NO_PLAYER)
			{
				if (pUnit->isSpy() && !kMember.canSpiesEnterBorders(eOwner))
					aUnits.push_back(pUnit);
			}
		}
	}
	for (uint i = 0; i < aUnits.size(); ++i)
		aUnits[i]->jumpToNearestValidPlot();
}

void CvTeam::setForceRevealedBonus(BonusTypes eBonus, bool bRevealed)
{
	if (isForceRevealedBonus(eBonus) == bRevealed)
		return;
	CvMap const& kMap = GC.getMap();
	for (int iI = 0; iI < kMap.numPlots(); ++iI)
	{
		CvPlot& kPlot = kMap.getPlotByIndex(iI);
		if (kPlot.getBonusType() == eBonus)
		{
			if (kPlot.getTeam() == getID())
				kPlot.updatePlotGroupBonus(false, /* advc.064d: */ false);
		}
	}
	if (bRevealed)
		m_aeRevealedBonuses.push_back(eBonus);
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

	for (int iI = 0; iI < kMap.numPlots(); ++iI)
	{
		CvPlot& kPlot = kMap.getPlotByIndex(iI);
		if (kPlot.getBonusType() == eBonus)
		{
			if (kPlot.getTeam() == getID())
				kPlot.updatePlotGroupBonus(true);
		}
	}

	for (int iI = 0; iI < kMap.numPlots(); ++iI)
	{
		CvPlot& kPlot = kMap.getPlotByIndex(iI);
		if (kPlot.getBonusType() == eBonus)
		{
			kPlot.updateYield();
			kPlot.setLayoutDirty(true);
		}
	}
}

bool CvTeam::isForceRevealedBonus(BonusTypes eBonus) const
{
	// <advc> Replacing equivalent BtS code
	return (!m_aeRevealedBonuses.empty() && // To make sure we're not wasting any more time than necessary with this
			std::find(m_aeRevealedBonuses.begin(), m_aeRevealedBonuses.end(), eBonus) != m_aeRevealedBonuses.end());
	// </advc>
}

bool CvTeam::isBonusRevealed(BonusTypes eBonus) const // K-Mod
{
	return (isHasTech(GC.getInfo(eBonus).getTechReveal()) || isForceRevealedBonus(eBonus));
}

// <advc.108> Based on CvPlayer::initFreeUnits
void CvTeam::revealSurroundingPlots(CvPlot const& kCenter, int iRange) const
{
	CvMap const& kMap = GC.getMap();
	for(int i = 0; i < kMap.numPlots(); i++)
	{
		CvPlot& kPlot = kMap.getPlotByIndex(i);
		if(plotDistance(&kPlot, &kCenter) <= iRange)
			kPlot.setRevealed(getID(), true, false, NO_TEAM, false);
	}
} // </advc.108>

int CvTeam::countNumHumanGameTurnActive() const
{
	int iCount = 0;
	for (PlayerIter<HUMAN,MEMBER_OF> it(getID()); it.hasNext(); ++it)
	{
		if (it->isTurnActive())
			iCount++;
	}
	return iCount;
}

void CvTeam::setTurnActive(bool bNewValue, bool bDoTurn)
{
	FAssert(GC.getGame().isSimultaneousTeamTurns());
	for (MemberIter it(getID()); it.hasNext(); ++it)
		it->setTurnActive(bNewValue, bDoTurn);
}

bool CvTeam::isTurnActive() const
{
	FAssert(GC.getGame().isSimultaneousTeamTurns());
	for (MemberIter it(getID()); it.hasNext(); ++it)
	{
		if (it->isTurnActive())
			return true;
	}
	return false;
}


void CvTeam::doWarWeariness()
{
	static int const iWW_DECAY_RATE = GC.getDefineINT("WW_DECAY_RATE"); // advc.opt
	static int const iWW_DECAY_PEACE_PERCENT = GC.getDefineINT("WW_DECAY_PEACE_PERCENT"); // advc.opt
	CvGame const& g = GC.getGame();
	for (TeamIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		TeamTypes eLoopTeam = it->getID();
		if (getWarWeariness(eLoopTeam) > 0)
		{
			changeWarWeariness(eLoopTeam, 100 * iWW_DECAY_RATE);
			if (!GET_TEAM(eLoopTeam).isAlive() || !isAtWar(eLoopTeam) ||
				g.isOption(GAMEOPTION_ALWAYS_WAR) || g.isOption(GAMEOPTION_NO_CHANGING_WAR_PEACE))
			{
				setWarWeariness(eLoopTeam, (getWarWeariness(eLoopTeam) *
						iWW_DECAY_PEACE_PERCENT) / 100);
			}
		}
	}
}

// advc: Body cut from doTurn
void CvTeam::doBarbarianResearch()
{
	FAssert(isBarbarian());
	CvGame const& kGame = GC.getGame();
	int iElapsed = kGame.getElapsedGameTurns();
	/*  <kekm.28> "Give some starting research to barbarians in advanced start
		depending on other players' tech status after advanced start. */
	if (iElapsed == 1 && // This function isn't called on turn 0
		kGame.isOption(GAMEOPTION_ADVANCED_START))
	{
		FOR_EACH_ENUM(Tech)
		{
			if (isHasTech(eLoopTech))
				continue;
			int iCount = 0;
			TeamIter<CIV_ALIVE> it;
			for (; it.hasNext(); ++it)
			{
				if (it->isHasTech(eLoopTech))
					iCount++;
			}
			if (iCount > 0)
			{
				int const iPossibleCount = it.nextIndex();
				setResearchProgress(eLoopTech, (getResearchCost(eLoopTech) * iCount) /
						iPossibleCount, getLeaderID());
			}
		}
	} // </kekm.28>
	// <advc.307>
	bool const bNoBarbCities = GC.getInfo(kGame.getCurrentEra()).isNoBarbCities();
	bool const bIgnorePrereqs = bNoBarbCities;
			/*  Barbs get all tech from earlier eras for free. Don't need
				to catch up. */ //|| g.getStartEra() > 0;
	// </advc.307>
	/*  K-Mod. Delay the start of the barbarian research. (This is an
		experimental change. It is currently compensated by an increase in
		the barbarian tech rate.) */
	if (iElapsed < GC.getInfo(kGame.getHandicapType()).
		getBarbarianCreationTurnsElapsed() *
		GC.getInfo(kGame.getGameSpeedType()).getBarbPercent() / 200)
	{
		return;
	}

	CvPlayerAI const& kBarbPlayer = GET_PLAYER(BARBARIAN_PLAYER);
	FOR_EACH_ENUM(Tech)
	{
		if (!isHasTech(eLoopTech) && /* advc.307: */ (bIgnorePrereqs ||
			// K-Mod. Make no progress on techs until prereqs are researched.
			kBarbPlayer.canResearch(eLoopTech, false, true)))
		{
			int iCount = 0;
			int iHasTech = 0; // advc.307
			TeamIter<CIV_ALIVE> it;
			for (; it.hasNext(); ++it)
			{
				CvTeam const& kLoopTeam = *it;
				if (kLoopTeam.isHasTech(eLoopTech)
						&& kLoopTeam.isInContactWithBarbarians()) // advc.302
					iCount++;
				// <advc.307>
				if(kLoopTeam.isHasTech(eLoopTech))
					iHasTech++; // </advc.307>
			} /* advc.302: Don't stop barb research entirely even when there is
				 no contact with any civs */
			iCount = std::max(iCount, iHasTech / 3);
			if (iCount > 0)
			{
				int const iPossible = it.nextIndex();
				/*  advc.307: In the late game, count all civs as having contact with
					barbs if at least one of them has contact. Otherwise, New World barbs
					catch up too slowly when colonized only by one or two civs. */
				if(bNoBarbCities)
					iCount = std::max(iCount, (2 * iHasTech) / 3);
				static int const iBARBARIAN_FREE_TECH_PERCENT = GC.getDefineINT("BARBARIAN_FREE_TECH_PERCENT"); // advc.opt
				//changeResearchProgress(eLoopTech, (getResearchCost(eLoopTech) * ((iBARBARIAN_FREE_TECH_PERCENT * iCount) / iPossible)) / 100, getLeaderID());
				// K-Mod. Adjust research rate for game-speed & start-era - but _not_ world-size. And fix the rounding error.
				int iBaseCost = getResearchCost(eLoopTech, false) *
						GC.getInfo(GC.getMap().getWorldSize()).getResearchPercent() / 100;
				changeResearchProgress(eLoopTech, std::max(1, iBaseCost *
						iBARBARIAN_FREE_TECH_PERCENT * iCount /
						(100 * iPossible)), kBarbPlayer.getID());
				// K-Mod end
			}
		}
	}
}

void CvTeam::updateTechShare(TechTypes eTech)
{
	if (isHasTech(eTech) /* advc.opt: */ || !isAnyTechShare())
		return;

	/*  advc (comment): isTechShare(iI) means that the team gets to
		learn all techs known by at least iI+1 other teams.
		So iI doesn't refer to a particular team here, but to a team count. */
	int iBestShare = MAX_INT;
	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (isTechShare((TeamTypes)iI))
		{
			iBestShare = std::min(iBestShare, iI + 1);
			break;
		}
	}
	FAssert(iBestShare < MAX_INT); // advc

	int iCount = 0;
	for (TeamIter<CIV_ALIVE,OTHER_KNOWN_TO> it(getID()); it.hasNext(); ++it)
	{
		if (it->isHasTech(eTech))
			iCount++;
		if (iCount >= iBestShare) // advc.opt: Moved into the loop
		{
			setHasTech(eTech, true, NO_PLAYER, true, true);
			if(GET_PLAYER(getLeaderID()).isSignificantDiscovery(eTech)) // advc.550e
				setNoTradeTech(eTech, true); // kekm.31
			return;
		}
	}
}


void CvTeam::updateTechShare()
{
	// <advc.opt>
	if (!isAnyTechShare())
		return; // </advc.opt>
	FOR_EACH_ENUM(Tech)
		updateTechShare(eLoopTech);
}

// advc: Duplicate code cut from setHasTech
void CvTeam::updatePlotGroupBonus(TechTypes eTech, bool bAdd)
{
	CvMap const& kMap = GC.getMap();
	for (int i = 0; i < kMap.numPlots(); i++)
	{
		CvPlot& kPlot = kMap.getPlotByIndex(i);
		if (kPlot.getTeam() != getID())
			continue;
		BonusTypes eBonus = kPlot.getBonusType();
		if (eBonus == NO_BONUS)
			continue;
		CvBonusInfo const& kBonus = GC.getInfo(eBonus);
		if (kBonus.getTechReveal() == eTech || kBonus.getTechCityTrade() == eTech ||
			kBonus.getTechObsolete() == eTech)
		{
			kPlot.updatePlotGroupBonus(bAdd);
		}
	}
}


void CvTeam::testCircumnavigated()
{
	if (isBarbarian())
		return;

	if (!GC.getGame().circumnavigationAvailable())
		return;

	CvMap const& kMap = GC.getMap(); // advc
	if (kMap.isWrapX())
	{
		for (int iX = 0; iX < kMap.getGridWidth(); iX++)
		{
			bool bFoundVisible = false;
			for (int iY = 0; iY < kMap.getGridHeight(); iY++)
			{
				if (kMap.getPlot(iX, iY).isRevealed(getID()))
				{
					bFoundVisible = true;
					break;
				}
			}
			if (!bFoundVisible)
				return;
		}
	}

	if (kMap.isWrapY())
	{
		for (int iY = 0; iY < kMap.getGridHeight(); iY++)
		{
			bool bFoundVisible = false;
			for (int iX = 0; iX < kMap.getGridWidth(); iX++)
			{
				if (kMap.getPlot(iX, iY).isRevealed(getID()))
				{
					bFoundVisible = true;
					break;
				}
			}
			if (!bFoundVisible)
				return;
		}
	}

	GC.getGame().makeCircumnavigated();

	//if (GC.getGame().getElapsedGameTurns() > 0)
	if (GC.getGame().getElapsedGameTurns() > 1 && // K-Mod (due to changes in when CvTeam::doTurn is called)
		GC.getDefineINT("CIRCUMNAVIGATE_FREE_MOVES") != 0)
	{
		changeExtraMoves(DOMAIN_SEA, GC.getDefineINT("CIRCUMNAVIGATE_FREE_MOVES"));
		for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
		{
			CvPlayer const& kObs = *it;
			CvWString szBuffer;
			if (getID() == kObs.getTeam())
			{
				szBuffer = gDLL->getText("TXT_KEY_MISC_YOU_CIRC_GLOBE",
						GC.getDefineINT("CIRCUMNAVIGATE_FREE_MOVES"));
			}
			else if (isHasMet(kObs.getTeam()) /* advc.127: */ || kObs.isSpectator())
			{
				szBuffer = gDLL->getText("TXT_KEY_MISC_SOMEONE_CIRC_GLOBE",
						getName().GetCString());
			}
			else szBuffer = gDLL->getText("TXT_KEY_MISC_UNKNOWN_CIRC_GLOBE");
			gDLL->UI().addMessage(kObs.getID(), false, -1, szBuffer,
					"AS2D_GLOBECIRCUMNAVIGATED", MESSAGE_TYPE_MAJOR_EVENT,
					NULL, GC.getColorType("HIGHLIGHT_TEXT"),
					// advc.127b:
					getCapitalX(kObs.getTeam(), true), getCapitalY(kObs.getTeam(), true));
		}
		CvWString szBuffer(gDLL->getText("TXT_KEY_MISC_SOMEONE_CIRC_GLOBE",
				getReplayName().GetCString()));
		GC.getGame().addReplayMessage(REPLAY_MESSAGE_MAJOR_EVENT, getLeaderID(), szBuffer,
				-1, -1, GC.getColorType("HIGHLIGHT_TEXT"));
	}
}

// <advc.127b>
CvCity* CvTeam::getLeaderCapital(TeamTypes eObserver, bool bDebug) const
{
	CvCity* r = GET_PLAYER(getLeaderID()).getCapital();
	if (r != NULL && eObserver != NO_TEAM && !r->isRevealed(eObserver, bDebug))
		r = NULL;
	if (r != NULL)
		return r;
	int iMinRank = MAX_INT;
	for (MemberIter it(getID()); it.hasNext(); ++it)
	{
		CvPlayer const& kMember = *it;
		CvCity* pCapital = kMember.getCapital();
		if (pCapital == NULL ||
			(eObserver != NO_TEAM && !pCapital->isRevealed(eObserver, bDebug)))
		{
			continue;
		}
		int iRank = GC.getGame().getPlayerRank(kMember.getID());
		if(iRank < iMinRank)
		{
			r = pCapital;
			iMinRank = iRank;
		}
	}
	return r;
}

int CvTeam::getCapitalX(TeamTypes eObserver, bool bDebug) const
{
	CvCity* pCapital = getLeaderCapital(eObserver, bDebug);
	if(pCapital == NULL)
		return -1;
	return pCapital->getX();
}

int CvTeam::getCapitalY(TeamTypes eObserver, bool bDebug) const
{
	CvCity* pCapital = getLeaderCapital(eObserver, bDebug);
	if(pCapital == NULL)
		return -1;
	return pCapital->getY();
} // </advc.127b>

void CvTeam::processTech(TechTypes eTech, int iChange,
	bool bEndOfTurn) // advc.121
{
	PROFILE_FUNC();

	CvTechInfo const& kTech = GC.getInfo(eTech);

	if (kTech.isExtraWaterSeeFrom())
		changeExtraWaterSeeFromCount(iChange);

	if (kTech.isMapCentering())
	{
		if (iChange > 0)
			setMapCentering(true);
	}

	if (kTech.isMapTrading())
		changeMapTradingCount(iChange);

	if (kTech.isTechTrading())
		changeTechTradingCount(iChange);

	if (kTech.isGoldTrading())
		changeGoldTradingCount(iChange);

	if (kTech.isOpenBordersTrading())
		changeOpenBordersTradingCount(iChange);

	if (kTech.isDefensivePactTrading())
		changeDefensivePactTradingCount(iChange);

	if (kTech.isPermanentAllianceTrading())
		changePermanentAllianceTradingCount(iChange);

	if (kTech.isVassalStateTrading())
		changeVassalTradingCount(iChange);

	if (kTech.isBridgeBuilding())
		changeBridgeBuildingCount(iChange);

	if (kTech.isIrrigation())
		changeIrrigationCount(iChange);

	if (kTech.isIgnoreIrrigation())
		changeIgnoreIrrigationCount(iChange);

	if (kTech.isWaterWork())
		changeWaterWorkCount(iChange);

	FOR_EACH_ENUM(Route)
	{
		changeRouteChange(eLoopRoute, GC.getInfo(eLoopRoute).getTechMovementChange(eTech) * iChange);
	}
	FOR_EACH_ENUM(Domain)
	{
		changeExtraMoves(eLoopDomain, kTech.getDomainExtraMoves(eLoopDomain) * iChange);
	}
	FOR_EACH_ENUM(Commerce)
	{
		if (kTech.isCommerceFlexible(eLoopCommerce))
			changeCommerceFlexibleCount(eLoopCommerce, iChange);
	}
	if (kTech.isAnyTerrainTrade()) // advc.003t
	{
		FOR_EACH_ENUM(Terrain)
		{
			if (kTech.isTerrainTrade(eLoopTerrain))
				changeTerrainTradeCount(eLoopTerrain, iChange);
		}
	}
	if (kTech.isRiverTrade())
	{
		changeRiverTradeCount(iChange);
	}
	FOR_EACH_ENUM(Building)
	{
		if (GC.getInfo(eLoopBuilding).getObsoleteTech() == eTech)
			changeObsoleteBuildingCount(eLoopBuilding, iChange);

		if (GC.getInfo(eLoopBuilding).getSpecialBuildingType() != NO_SPECIALBUILDING)
		{
			if (GC.getInfo(GC.getInfo(eLoopBuilding).getSpecialBuildingType()).
				getObsoleteTech() == eTech)
			{
				changeObsoleteBuildingCount(eLoopBuilding, iChange);
			}
		}
	}
	FOR_EACH_ENUM(Improvement)
	{
		FOR_EACH_ENUM(Yield)
		{
			changeImprovementYieldChange(eLoopImprovement, eLoopYield, GC.getInfo(eLoopImprovement).
					getTechYieldChanges(eTech, eLoopYield) * iChange);
		}
	}

	for (MemberIter it(getID()); it.hasNext(); ++it)
	{
		CvPlayer& kMember = *it;
		kMember.changeFeatureProductionModifier(kTech.getFeatureProductionModifier() * iChange);
		kMember.changeWorkerSpeedModifier(kTech.getWorkerSpeedModifier() * iChange);
		kMember.changeTradeRoutes(kTech.getTradeRoutes() * iChange);
		kMember.changeExtraHealth(kTech.getHealth() * iChange);
		kMember.changeExtraHappiness(kTech.getHappiness() * iChange);
		kMember.changeAssets(((kTech.getAssetValue()
				* 6) / 8) // advc.131: Makes it 6 per era instead of 8
				* iChange);
		kMember.changePower(kTech.getPowerValue() * iChange);
		kMember.changeTechScore(GC.getGame().getTechScore(eTech) * iChange);
		// K-Mod. Extra commerce for specialist (new xml field)
		FOR_EACH_ENUM(Commerce)
		{
			kMember.changeSpecialistExtraCommerce(eLoopCommerce,
					kTech.getSpecialistExtraCommerce(eLoopCommerce) * iChange);
		} // K-Mod end
	}
	CvMap const& kMap = GC.getMap();
	for (int i = 0; i < kMap.numPlots(); i++)
	{
		CvPlot& kLoopPlot = kMap.getPlotByIndex(i);
		BonusTypes eBonus = kLoopPlot.getBonusType();
		if (eBonus != NO_BONUS)
		{
			if (GC.getInfo(eBonus).getTechReveal() == eTech)
			{
				kLoopPlot.updateYield();
				kLoopPlot.setLayoutDirty(true);
			}
		}
	}
	FOR_EACH_ENUM(Build)
	{
		if (GC.getInfo(eLoopBuild).getTechPrereq() != eTech)
			continue;
		if (GC.getInfo(eLoopBuild).getRoute() != NO_ROUTE)
		{
			for (int i = 0; i < GC.getMap().numPlots(); i++)
			{
				CvPlot& kLoopPlot = GC.getMap().getPlotByIndex(i);
				CvCity const* pCity = kLoopPlot.getPlotCity();
				if (pCity != NULL)
				{
					if (pCity->getTeam() == getID())
						kLoopPlot.updateCityRoute(true);
				}
			}
		}  // <advc.121>
		if (!bEndOfTurn) // Otherwise, CvCity::doTurn is about to be called anyway.
		{
			for (MemberIter it(getID()); it.hasNext(); ++it)
			{
				if (!it->isHuman())
					it->AI_processNewBuild(eLoopBuild);
			}
		} // </advc.121>
	}
	for (MemberIter it(getID()); it.hasNext(); ++it)
		it->updateCorporation();
}


void CvTeam::cancelDefensivePacts()
{
	FOR_EACH_DEAL_VAR(pLoopDeal)
	{
		if (!pLoopDeal->involves(getID()))
			continue;
		FOR_EACH_TRADE_ITEM(pLoopDeal->getFirstList())
		{
			if (pItem->m_eItemType == TRADE_DEFENSIVE_PACT)
			{
				pLoopDeal->kill();
				break;
			}
		}
	}
}

// kekm.3: (actually an advc change)
void CvTeam::allowDefensivePactsToBeCanceled()
{
	FOR_EACH_DEAL_VAR(d)
	{
		if (!d->involves(getID()) || d->getLengthFirst() <= 0)
			continue;
		if(d->getFirstList().head()->m_data.m_eItemType == TRADE_DEFENSIVE_PACT)
			d->setInitialGameTurn(-100);
	}
}


void CvTeam::read(FDataStreamBase* pStream)
{
	reset(); // Init data before load

	uint uiFlag=0;
	pStream->Read(&uiFlag);

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
	if(uiFlag >= 5)
	{
		pStream->Read(&m_iMajorWarEnemies);
		pStream->Read(&m_iMinorWarEnemies);
		pStream->Read(&m_iVassalWarEnemies);
		pStream->Read(&m_bMinorTeam);
	}
	else
	{
		updateMinorCiv(); // Need to do this before CvTeamAI::read
		/*	All teams need to be loaded before war enemies can be counted.
			Set negative counts in order to signal to CvGame::onAllGameDataRead
			that finalizeInit needs to be called. */
		m_iMajorWarEnemies = m_iMinorWarEnemies = m_iVassalWarEnemies = -1;
	} // </advc.003m>

	pStream->Read(&m_bMapCentering);
	pStream->Read(&m_bCapitulated);
	// <advc.opt>
	if (uiFlag >= 7)
		pStream->Read(&m_bAnyVictoryCountdown); // </advc.opt>

	pStream->Read((int*)&m_eID);

	m_aiStolenVisibilityTimer.Read(pStream);
	m_aiWarWeariness.Read(pStream);
	m_aiTechShareCount.Read(pStream);
	m_aiEspionagePointsAgainstTeam.Read(pStream);
	m_aiCounterespionageTurnsLeftAgainstTeam.Read(pStream);
	m_aiCounterespionageModAgainstTeam.Read(pStream);
	m_aiCommerceFlexibleCount.Read(pStream);
	// <advc.120g> Prior to uiFlag=6, espionage was flexible from the beginning.
	if(uiFlag < 6)
		m_aiCommerceFlexibleCount.set(COMMERCE_ESPIONAGE, 1); // </advc.120g>
	m_aiExtraMoves.Read(pStream);
	m_aiForceTeamVoteEligibilityCount.Read(pStream);
	// <advc.091>
	if (uiFlag < 12)
	{
		EnumMap<TeamTypes,bool> abHasMet;
		abHasMet.Read(pStream);
		CvGame const& kGame = GC.getGame();
		int iGameTurn = kGame.getGameTurn();
		int iStartTurn = kGame.getStartTurn();
		FAssert(iGameTurn >= 0 && iStartTurn >= 0);
		for (TeamIter<> itTeam; itTeam.hasNext(); ++itTeam)
		{
			TeamTypes eTeam = itTeam->getID();
			if (abHasMet.get(eTeam))
			{
				m_aiHasMetTurn.set(eTeam, toShort(
						eTeam == getID() ? iStartTurn : iGameTurn));
			}
		}
	}
	else m_aiHasMetTurn.Read(pStream);
	// </advc.091>
	/*if (uiFlag >= 1)
		pStream->Read(MAX_TEAMS, m_abHasSeen);
	else memcpy(m_abHasSeen, m_abHasMet, sizeof(*m_abHasSeen)*MAX_TEAMS);*/ // K-Mod
	// <advc.enum>
	FAssert(uiFlag >= 1);
	m_abHasSeen.Read(pStream); // </advc.enum>
	m_abAtWar.Read(pStream);
	// <advc.162>
	if(uiFlag >= 4)
		m_abJustDeclaredWar.Read(pStream); // </advc.162>
	m_abPermanentWarPeace.Read(pStream);
	m_abOpenBorders.Read(pStream);
	// <advc.034>
	if(uiFlag >= 3)
	{
		if (uiFlag >= 9)
			m_abDisengage.Read(pStream);
		else // I had previously written only MAX_CIV_TEAMS values
		{
			bool abTmp[MAX_CIV_TEAMS];
			pStream->Read(MAX_CIV_TEAMS, abTmp);
			for (int i = 0; i < MAX_CIV_TEAMS; i++)
				m_abDisengage.set((TeamTypes)i, abTmp[i]);
		}
	} // </advc.034>
	m_abDefensivePact.Read(pStream);
	m_abForcePeace.Read(pStream);
	// <advc.opt>
	if (uiFlag < 9)
	{
		EnumMap<TeamTypes,bool> dummy; // m_abVassal
		dummy.Read(pStream);
	}
	pStream->Read((int*)&m_eMaster);
	if(uiFlag >= 2)
		pStream->Read((int*)&m_eLeader);
	if (uiFlag < 8)
		updateLeaderID();
	// </advc.opt>
	m_abCanLaunch.Read(pStream);

	m_aiRouteChange.Read(pStream);
	m_aiProjectCount.Read(pStream);
	m_aiProjectDefaultArtTypes.Read(pStream);

	//project art types
	for(int i = 0; i < GC.getNumProjectInfos(); i++)
	{
		int iTmp;
		for(int j = 0; j < m_aiProjectCount.get((ProjectTypes)i); j++)
		{
			pStream->Read(&iTmp);
			m_pavProjectArtTypes[i].push_back(iTmp);
		}
	}

	m_aiProjectMaking.Read(pStream);
	m_aiUnitClassCount.Read(pStream);
	m_aiBuildingClassCount.Read(pStream);
	m_aiObsoleteBuildingCount.Read(pStream);
	m_aiResearchProgress.Read(pStream);
	m_aiTechCount.Read(pStream);
	m_aiTerrainTradeCount.Read(pStream);
	m_aiVictoryCountdown.Read(pStream);
	// <advc.opt>
	if (uiFlag == 10) // Fix a bug in AdvCiv 0.97
	{
		FOR_EACH_ENUM(Victory)
		{	/*	Is 0 a legit value? Well, let's hope that it represents
				"no countdown started" in this savegame. */
			if (getVictoryCountdown(eLoopVictory) == 0)
				m_aiVictoryCountdown.set(eLoopVictory, -1);
		}
	}
	if (uiFlag < 7 || uiFlag == 10)
	{
		FOR_EACH_ENUM(Victory)
		{
			if (getVictoryCountdown(eLoopVictory) >= 0)
				m_bAnyVictoryCountdown = true;
		}
	} // </advc.opt>

	m_abHasTech.Read(pStream);
	// <advc.101>
	if (uiFlag >= 10)
		pStream->Read(&m_iTechCount);
	else
	{
		FOR_EACH_ENUM(Tech)
		{
			if (m_abHasTech.get(eLoopTech))
				m_iTechCount++;
		}
	} // </advc.101>
	m_abNoTradeTech.Read(pStream);
	m_aaiImprovementYieldChange.Read(pStream);

	int iSize;
	m_aeRevealedBonuses.clear();
	pStream->Read(&iSize);
	for (int i = 0; i < iSize; ++i)
	{
		BonusTypes eBonus;
		pStream->Read((int*)&eBonus);
		m_aeRevealedBonuses.push_back(eBonus);
	}
	// <advc.183> Reveal any destroyed forts (so that aircraft can't rebase to them)
	if (uiFlag < 13 && isAlive())
	{
		FOR_EACH_ENUM(PlotNum)
		{
			CvPlot& kPlot = GC.getMap().getPlotByIndex(eLoopPlotNum);
			if (kPlot.isWater()) // to save time
				continue;
			ImprovementTypes eRevealedImprov = kPlot.getRevealedImprovementType(getID());
			if (eRevealedImprov != NO_IMPROVEMENT &&
				kPlot.getImprovementType() != eRevealedImprov &&
				GC.getInfo(eRevealedImprov).isActsAsCity())
			{
				kPlot.setRevealedImprovementType(getID(), NO_IMPROVEMENT);
			}
		}
	} // </advc.183>
}

// <advc.003m>  (for legacy savegames)
void CvTeam::finalizeInit()
{
	m_iMajorWarEnemies = countWarEnemies();
	m_iMinorWarEnemies = countWarEnemies(false, false) - m_iMajorWarEnemies;
	FAssert(m_iMinorWarEnemies >= 0);
	m_iVassalWarEnemies = m_iMajorWarEnemies - countWarEnemies(true, true);
	FAssert(m_iVassalWarEnemies >= 0);
} // </advc.003m>


void CvTeam::write(FDataStreamBase* pStream)
{
	PROFILE_FUNC(); // advc
	REPRO_TEST_BEGIN_WRITE(CvString::format("Team(%d)", getID()));
	uint uiFlag;
	//uiFlag = 1; // K-Mod
	//uiFlag = 2; // advc.opt: m_eLeader added
	//uiFlag = 3; // advc.034
	//uiFlag = 4; // advc.162
	//uiFlag = 5; // advc.003m
	//uiFlag = 6; // advc.120g
	//uiFlag = 7; // advc.opt: m_bAnyVictoryCountdown
	//uiFlag = 8; // advc.opt: change in updateLeaderID
	//uiFlag = 9; // advc.opt: remove m_abVassal; advc.enum/ advc.034: write m_abDisengage[BARBARIAN_TEAM]
	//uiFlag = 10; // advc.101: m_iTechCount
	//uiFlag = 11; // advc.opt: fix m_aiVictoryCountdown bug
	//uiFlag = 12; // advc.091
	uiFlag = 13; // advc.183
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

	m_aiStolenVisibilityTimer.Write(pStream);
	m_aiWarWeariness.Write(pStream);
	m_aiTechShareCount.Write(pStream);
	m_aiEspionagePointsAgainstTeam.Write(pStream);
	m_aiCounterespionageTurnsLeftAgainstTeam.Write(pStream);
	m_aiCounterespionageModAgainstTeam.Write(pStream);
	m_aiCommerceFlexibleCount.Write(pStream);
	m_aiExtraMoves.Write(pStream);
	m_aiForceTeamVoteEligibilityCount.Write(pStream);

	m_aiHasMetTurn.Write(pStream); // advc.091
	m_abHasSeen.Write(pStream); // K-Mod. uiFlag >= 1
	m_abAtWar.Write(pStream);
	m_abJustDeclaredWar.Write(pStream); // advc.162
	m_abPermanentWarPeace.Write(pStream);
	m_abOpenBorders.Write(pStream);
	m_abDisengage.Write(pStream); // advc.034
	m_abDefensivePact.Write(pStream);
	m_abForcePeace.Write(pStream);
	// <advc.opt>
	//m_abVassal.Write(pStream);
	pStream->Write(m_eMaster);
	pStream->Write(m_eLeader);
	// </advc.opt>
	m_abCanLaunch.Write(pStream);
	m_aiRouteChange.Write(pStream);
	m_aiProjectCount.Write(pStream);
	m_aiProjectDefaultArtTypes.Write(pStream);

	//project art types
	for(int i = 0; i < GC.getNumProjectInfos(); i++)
	{
		for(int j = 0; j < m_aiProjectCount.get((ProjectTypes)i); j++)
			pStream->Write(m_pavProjectArtTypes[i][j]);
	}

	m_aiProjectMaking.Write(pStream);
	m_aiUnitClassCount.Write(pStream);
	m_aiBuildingClassCount.Write(pStream);
	m_aiObsoleteBuildingCount.Write(pStream);
	m_aiResearchProgress.Write(pStream);
	m_aiTechCount.Write(pStream);
	m_aiTerrainTradeCount.Write(pStream);
	m_aiVictoryCountdown.Write(pStream);
	m_abHasTech.Write(pStream);
	pStream->Write(m_iTechCount); // advc.101
	m_abNoTradeTech.Write(pStream);
	m_aaiImprovementYieldChange.Write(pStream);

	pStream->Write(m_aeRevealedBonuses.size());
	for (std::vector<BonusTypes>::iterator it = m_aeRevealedBonuses.begin(); it != m_aeRevealedBonuses.end(); ++it)
	{
		pStream->Write(*it);
	}
	REPRO_TEST_END_WRITE();
}


bool CvTeam::hasShrine(ReligionTypes eReligion) const
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
	FOR_EACH_ENUM(Project)
	{
		if (GC.getInfo(eLoopProject).isSpaceship())
			mapProjects[eLoopProject] = getProjectCount(eLoopProject);
	}
}

int CvTeam::getProjectPartNumber(ProjectTypes eProject, bool bAssert) const
{
	int iNumBuilt = getProjectCount(eProject);
	for (int i = 0; i < iNumBuilt; i++)
	{
		if (getProjectArtType(eProject, i) < 0)
			return i;
	}
	FAssertMsg(!bAssert, "Didn't find empty part number");
	//return the last one
	return std::min(iNumBuilt, GC.getInfo(eProject).getMaxTeamInstances() - 1);
}

bool CvTeam::hasLaunched() const
{	// <advc.opt>
	if (!isAnyVictoryCountdown())
		return false; // </advc.opt>
	VictoryTypes spaceVictory = GC.getGame().getSpaceVictory();
	if (spaceVictory != NO_VICTORY)
		return (getVictoryCountdown(spaceVictory) >= 0);
	return false;
}

// <advc>
bool CvTeam::hasTechToClear(FeatureTypes eFeature, TechTypes eCurrentResearch) const
{
	FOR_EACH_ENUM(Build)
	{
		CvBuildInfo const& kBuild = GC.getInfo(eLoopBuild);
		if(kBuild.getFeatureTime(eFeature) <= 0)
			continue;
		TechTypes aeReqs[2] = { kBuild.getTechPrereq(),
								kBuild.getFeatureTech(eFeature) };
		bool bValid = true;
		for(int j = 0; j < 2; j++)
		{
			if(aeReqs[j] != NO_TECH && eCurrentResearch != aeReqs[j] && !isHasTech(aeReqs[j]))
			{
				bValid = false;
				break;
			}
		}
		if(bValid)
			return true;
	}
	return false;
} // </advc>
