// teamAI.cpp

#include "CvGameCoreDLL.h"
#include "CvTeamAI.h"
#include "CvAI.h"
#include "CvMap.h"
#include "CvAreaList.h" // advc.003s
#include "CvInfo_City.h"
#include "CvInfo_Terrain.h"
#include "CvInfo_GameOption.h"
#include "BBAILog.h"
#include "WarAndPeaceAgent.h" // advc.104
#include <numeric> // K-Mod. used in AI_warSpoilsValue

// statics: (advc.003u: Mostly moved to CvTeam)
DllExport CvTeamAI& CvTeamAI::getTeamNonInl(TeamTypes eTeam)
{
	return AI_getTeam(eTeam);
}


CvTeamAI::CvTeamAI(/* advc.003u: */ TeamTypes eID) : CvTeam(eID)
{
	m_aiWarPlanStateCounter = new int[MAX_TEAMS];
	m_aiAtWarCounter = new int[MAX_TEAMS];
	m_aiAtPeaceCounter = new int[MAX_TEAMS];
	m_aiHasMetCounter = new int[MAX_TEAMS];
	m_aiOpenBordersCounter = new int[MAX_TEAMS];
	m_aiDefensivePactCounter = new int[MAX_TEAMS];
	m_aiShareWarCounter = new int[MAX_TEAMS];
	m_aiWarSuccess = new int[MAX_TEAMS];
	m_aiSharedWarSuccess = new int[MAX_TEAMS]; // advc.130m
	m_aiEnemyPeacetimeTradeValue = new int[MAX_TEAMS];
	m_aiEnemyPeacetimeGrantValue = new int[MAX_TEAMS];
	m_aiWarPlanCounts = new int[NUM_WARPLAN_TYPES]; // advc.opt
	m_aeWarPlan = new WarPlanTypes[MAX_TEAMS];
	m_pWpai = new WarAndPeaceAI::Team(); // advc.104

	AI_reset(true);
}


CvTeamAI::~CvTeamAI()
{
	AI_uninit();

	SAFE_DELETE_ARRAY(m_aiWarPlanStateCounter);
	SAFE_DELETE_ARRAY(m_aiAtWarCounter);
	SAFE_DELETE_ARRAY(m_aiAtPeaceCounter);
	SAFE_DELETE_ARRAY(m_aiHasMetCounter);
	SAFE_DELETE_ARRAY(m_aiOpenBordersCounter);
	SAFE_DELETE_ARRAY(m_aiDefensivePactCounter);
	SAFE_DELETE_ARRAY(m_aiShareWarCounter);
	SAFE_DELETE_ARRAY(m_aiWarSuccess);
	SAFE_DELETE_ARRAY(m_aiSharedWarSuccess); // advc.130m
	SAFE_DELETE_ARRAY(m_aiEnemyPeacetimeTradeValue);
	SAFE_DELETE_ARRAY(m_aiEnemyPeacetimeGrantValue);
	SAFE_DELETE_ARRAY(m_aiWarPlanCounts); // advc.opt
	SAFE_DELETE_ARRAY(m_aeWarPlan);
	SAFE_DELETE(m_pWpai); // advc.104
}


void CvTeamAI::AI_init()
{
	AI_reset(false);
}

// K-Mod
void CvTeamAI::AI_initMemory()
{
	// <advc.104>
	if(isEverAlive() && !isBarbarian() && !isMinorCiv())
		m_pWpai->init(getID()); // </advc.104>
	// Note. this needs to be done after the map is set. unfortunately, AI_init is called before that happens.
	FAssert(GC.getMap().numPlots() > 0);
	m_aiStrengthMemory.clear();
	m_aiStrengthMemory.resize(GC.getMap().numPlots(), 0);
}
// K-Mod end


void CvTeamAI::AI_uninit()
{
	//m_aiStrengthMemory.clear(); // K-Mod. Clearing the memory will cause problems if the game is still in progress.
}


void CvTeamAI::AI_reset(bool bConstructor)
{
	AI_uninit();

	m_eWorstEnemy = NO_TEAM;

	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		m_aiWarPlanStateCounter[iI] = 0;
		m_aiAtWarCounter[iI] = 0;
		m_aiAtPeaceCounter[iI] = 0;
		m_aiHasMetCounter[iI] = 0;
		m_aiOpenBordersCounter[iI] = 0;
		m_aiDefensivePactCounter[iI] = 0;
		m_aiShareWarCounter[iI] = 0;
		m_aiWarSuccess[iI] = 0;
		m_aiSharedWarSuccess[iI] = 0; // advc.130m
		m_aiEnemyPeacetimeTradeValue[iI] = 0;
		m_aiEnemyPeacetimeGrantValue[iI] = 0;
		m_aeWarPlan[iI] = NO_WARPLAN;

		if (!bConstructor && getID() != NO_TEAM)
		{
			TeamTypes eLoopTeam = (TeamTypes) iI;
			CvTeamAI& kLoopTeam = GET_TEAM(eLoopTeam);
			kLoopTeam.m_aiWarPlanStateCounter[getID()] = 0;
			kLoopTeam.m_aiAtWarCounter[getID()] = 0;
			kLoopTeam.m_aiAtPeaceCounter[getID()] = 0;
			kLoopTeam.m_aiHasMetCounter[getID()] = 0;
			kLoopTeam.m_aiOpenBordersCounter[getID()] = 0;
			kLoopTeam.m_aiDefensivePactCounter[getID()] = 0;
			kLoopTeam.m_aiShareWarCounter[getID()] = 0;
			kLoopTeam.m_aiWarSuccess[getID()] = 0;
			kLoopTeam.m_aiSharedWarSuccess[getID()] = 0; // advc.130m
			kLoopTeam.m_aiEnemyPeacetimeTradeValue[getID()] = 0;
			kLoopTeam.m_aiEnemyPeacetimeGrantValue[getID()] = 0;
			kLoopTeam.m_aeWarPlan[getID()] = NO_WARPLAN;
		}
	} // <advc.opt>
	for (int i = 0; i < NUM_WARPLAN_TYPES; i++)
		m_aiWarPlanCounts[i] = 0; // </advc.opt>
	m_bAnyWarPlan = false; // advc.opt
	m_bLonely = false; // advc.109
	m_religionKnownSince.clear(); // advc.130n
}


void CvTeamAI::AI_doTurnPre()
{
	AI_doCounter();

	/*if(isHuman()) // advc: Caller handles these
		return;
	if(isBarbarian())
		return;
	if(isMinorCiv())
		return;*/
	// <advc.104>
	if((getWPAI.isEnabled() || getWPAI.isEnabled(true)) && !isBarbarian() &&
			!isMinorCiv() && isAlive())
	{
		getWPAI.update();
		/*  Calls turnPre on the team members, i.e. WarAndPeaceAI::Civ::turnPre
			happens before CvPlayerAI::AI_turnPre. Needs to be this way b/c
			WarAndPeaceAI::Team::doWar requires the members to be up-to-date. */
		m_pWpai->turnPre();
	} // </advc.104>
	// <advc.130n> Game turn increment can affect attitudes now
	if(isHuman())
	{
		for(int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)i);
			if(kPlayer.isAlive())
				kPlayer.AI_updateAttitudeCache();
		}
	} // </advc.130n>
}


void CvTeamAI::AI_doTurnPost()
{
	// K-Mod
	AI_updateStrengthMemory();

	// update the attitude cache for all team members.
	// (Note: attitude use to be updated near the start of CvGame::doTurn. I've moved it here for various reasons.)
	if(!isBarbarian()) // advc.003n
	{
		for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
		{
			CvPlayerAI& kLoopPlayer = GET_PLAYER(i);
			if (kLoopPlayer.getTeam() == getID() && kLoopPlayer.isAlive())
			{
				kLoopPlayer.AI_updateCloseBorderAttitudeCache();
				kLoopPlayer.AI_updateAttitudeCache();
			}
		} // K-Mod end
	}
	// <advc.109>
	if(!isBarbarian() && !isMinorCiv() && getCurrentEra() > GC.getGame().getStartEra())
	{
		// Civs who haven't met half their competitors (rounded down) are lonely
		int iHasMet = getHasMetCivCount(false);
		int iYetToMeet = GC.getGame().countCivTeamsAlive() - iHasMet;
		m_bLonely = (iYetToMeet > iHasMet + 1 && iHasMet <= 2);
	} // </advc.109>

	//AI_updateWorstEnemy(); // advc.130e: Covered by AI_updateAttitudeCache now

	AI_updateAreaStrategies(false);

	/* if (isHuman())
		return;
	if (isBarbarian())
		return;
	if (isMinorCiv())
		return;*/
	// disabled by K-Mod. There are some basic things inside AI_doWar which are important for all players.

	AI_doWar();
}


void CvTeamAI::AI_makeAssignWorkDirty()
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kMember = GET_PLAYER((PlayerTypes)iI);
		if (!kMember.isAlive() || kMember.getTeam() != getID())
			continue;
		kMember.AI_makeAssignWorkDirty();
	}
}

void CvTeamAI::AI_updateAreaStrategies(bool bTargets)
{
	PROFILE_FUNC();

	if (!GC.getGame().isFinalInitialized())
		return;

	FOR_EACH_AREA_VAR(pLoopArea)
		pLoopArea->setAreaAIType(getID(), AI_calculateAreaAIType(pLoopArea));

	if (bTargets)
		AI_updateAreaTargets();
}


void CvTeamAI::AI_updateAreaTargets()
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayerAI& kMember = GET_PLAYER((PlayerTypes)iI);
		if (!kMember.isAlive() || kMember.getTeam() != getID())
			continue;
		if (kMember.AI_getCityTargetTimer() == 0) // K-Mod added timer check.
			kMember.AI_updateAreaTargets();
	}
}


int CvTeamAI::AI_countFinancialTrouble() const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayerAI const& kMember = GET_PLAYER((PlayerTypes)iI);
		if (!kMember.isAlive() || kMember.getTeam() != getID())
			continue;
		if (kMember.AI_isFinancialTrouble())
			iCount++;
	}
	return iCount;
}


int CvTeamAI::AI_countMilitaryWeight(CvArea* pArea) const
{
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayerAI const& kMember = GET_PLAYER((PlayerTypes)iI);
		if (!kMember.isAlive() || kMember.getTeam() != getID())
			continue;
		iCount += kMember.AI_militaryWeight(pArea);
	}
	return iCount;
}

// K-Mod. return the total yield of the team, estimated by averaging over the last few turns of the yield's history graph.
// (advc.104 replaces this with CvPlayer::estimateYieldRate)
int CvTeamAI::AI_estimateTotalYieldRate(YieldTypes eYield) const
{
	PROFILE_FUNC();
	const int iSampleSize = 5;
	// number of turns to use in weighted average.
	// Ignore turns with 0 production, because they are probably a revolt. Bias towards most recent turns.
	const int iTurn = GC.getGame().getGameTurn();

	int iTotal = 0;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		const CvPlayerAI& kMember = GET_PLAYER((PlayerTypes)i);
		if (!kMember.isAlive() || kMember.getTeam() != getID())
			continue; // advc

		int iSubTotal = 0;
		int iBase = 0;
		for (int j = 0; j < iSampleSize; j++)
		{
			int p = 0;
			switch (eYield)
			{
			case YIELD_PRODUCTION:
				p = kMember.getIndustryHistory(iTurn - (j+1));
				break;
			case YIELD_COMMERCE:
				p = kMember.getEconomyHistory(iTurn - (j+1));
				break;
			case YIELD_FOOD:
				p = kMember.getAgricultureHistory(iTurn - (j+1));
				break;
			default:
				FAssertMsg(false, "invalid yield type in AI_estimateTotalYieldRate");
				break;
			}
			if (p > 0)
			{
				iSubTotal += (iSampleSize - j) * p;
				iBase += iSampleSize - j;
			}
		}
		iTotal += iSubTotal / std::max(1, iBase);
	}
	return iTotal;
} // K-Mod end

// K-Mod. return true if is fair enough for the AI to know there is a city here
bool CvTeamAI::AI_deduceCitySite(const CvCity* pCity) const
{
	PROFILE_FUNC();

	if (pCity->isRevealed(getID(), false))
		return true;

	// The rule is this:
	// if we can see more than n plots of the nth culture ring, we can deduce where the city is.

	int iPoints = 0;
	int iLevel = pCity->getCultureLevel();

	for (int iDX = -iLevel; iDX <= iLevel; iDX++)
	{
		for (int iDY = -iLevel; iDY <= iLevel; iDY++)
		{
			int iDist = pCity->cultureDistance(iDX, iDY);
			if (iDist > iLevel)
				continue;

			CvPlot* pLoopPlot = plotXY(pCity->getX(), pCity->getY(), iDX, iDY);
			if (pLoopPlot && pLoopPlot->getRevealedOwner(getID()) == pCity->getOwner())
			{
				// if multiple cities have their plot in their range, then that will make it harder to deduce the precise city location.
				iPoints += 1 + std::max(0, iLevel - iDist - pLoopPlot->getNumCultureRangeCities(pCity->getOwner())+1);
				if (iPoints > iLevel)
					return true;
			}
		}
	}
	return false;
}
// K-Mod end

bool CvTeamAI::AI_isAnyCapitalAreaAlone() const
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayerAI const& kMember = GET_PLAYER((PlayerTypes)iI);
		if (!kMember.isAlive() || kMember.getTeam() != getID())
			continue;
		if (kMember.AI_isCapitalAreaAlone())
			return true;
	}
	return false;
}


bool CvTeamAI::AI_isPrimaryArea(CvArea* pArea) const
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayerAI const& kMember = GET_PLAYER((PlayerTypes)iI);
		if (!kMember.isAlive() || kMember.getTeam() != getID())
			continue;
		if (kMember.AI_isPrimaryArea(pArea))
			return true;
	}
	return false;
}


bool CvTeamAI::AI_hasCitiesInPrimaryArea(TeamTypes eTeam) const
{
	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");
	FOR_EACH_AREA_VAR(pLoopArea)
	{
		if (AI_isPrimaryArea(pLoopArea))
		{
			if (GET_TEAM(eTeam).countNumCitiesByArea(pLoopArea))
				return true;
		}
	}
	return false;
}

// K-Mod. Return true if this team and eTeam have at least one primary area in common.
bool CvTeamAI::AI_hasSharedPrimaryArea(TeamTypes eTeam) const
{
	FAssert(eTeam != getID());

	CvTeamAI const& kTeam = GET_TEAM(eTeam);
	FOR_EACH_AREA_VAR(pLoopArea)
	{
		if (AI_isPrimaryArea(pLoopArea) && kTeam.AI_isPrimaryArea(pLoopArea))
			return true;
	}
	return false;
} // K-Mod end

/*  advc.104s (note): If UWAI is enabled, AI_doWar may adjust (i.e. overwrite) the
	result of this calculation through WarAndPeaceAgent::Team::alignAreaAI. */
AreaAITypes CvTeamAI::AI_calculateAreaAIType(CvArea* pArea, bool bPreparingTotal) const  // advc: style changes
{
	PROFILE_FUNC();

	// K-Mod. This function originally had "!isWater()" wrapping all of the code.
	// I've changed it to be more readable.
	if (pArea->isWater())
		return AREAAI_NEUTRAL;

	if (isBarbarian())
	{
		if (pArea->getNumCities() - pArea->getCitiesPerPlayer(BARBARIAN_PLAYER) == 0
				// advc.300: Make (New World) Barbarians relatively peaceable unless outnumbered
				|| pArea->countCivCities() < pArea->getCitiesPerPlayer(BARBARIAN_PLAYER))
			return AREAAI_ASSAULT;

		if (countNumAIUnitsByArea(pArea, UNITAI_ATTACK) +
			countNumAIUnitsByArea(pArea, UNITAI_ATTACK_CITY) +
			countNumAIUnitsByArea(pArea, UNITAI_PILLAGE) +
			countNumAIUnitsByArea(pArea, UNITAI_ATTACK_AIR) >
			1 + (AI_countMilitaryWeight(pArea) * 20) / 100)
		{
			return AREAAI_OFFENSIVE; // XXX does this ever happen?
			/*  advc (comment): Does this ever NOT happen? Only once a continent
				is almost entirely owned by civs. */
		}
		return AREAAI_MASSING;
	}

	bool bRecentAttack = false;
	bool bTargets = false;
	bool bChosenTargets = false;
	bool bDeclaredTargets = false;

	bool bAssault = false;
	bool bPreparingAssault = false;

	// int iOffensiveThreshold = (bPreparingTotal ? 25 : 20); // K-Mod, I don't use this.
	int iAreaCities = countNumCitiesByArea(pArea);
	int iWarSuccessRating = AI_getWarSuccessRating(); // K-Mod

	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		TeamTypes const eTarget = (TeamTypes)iI;
		CvTeam const& kTarget = GET_TEAM(eTarget);
		if (!kTarget.isAlive() || AI_getWarPlan(eTarget) == NO_WARPLAN)
			continue;
		FAssert(eTarget != getID());
		FAssert(isHasMet(eTarget) || GC.getGame().isOption(GAMEOPTION_ALWAYS_WAR));

		if (AI_getWarPlan(eTarget) == WARPLAN_ATTACKED_RECENT)
		{
			FAssert(isAtWar(eTarget));
			bRecentAttack = true;
		}

		if (kTarget.countNumCitiesByArea(pArea) > 0
				//|| GET_TEAM((TeamTypes)iI).countNumUnitsByArea(pArea) > 4)
			/*  advc.104s: Replacing the above. Setting AreaAI to ASSAULT won't stop
				the AI from fighting any landed units. Need to focus on cities.
				isLandTarget makes sure that there are reachable cities. Still check
				city count for efficiency (there can be a lot of land areas to
				calculate AI types for). */
			&& AI_isLandTarget(eTarget))
		{
			bTargets = true;
			if (AI_isChosenWar(eTarget))
			{
				bChosenTargets = true;

				if (isAtWar(eTarget) ?
						(AI_getAtWarCounter(eTarget) < 10) :
						AI_isSneakAttackReady(eTarget))
					bDeclaredTargets = true;
			}
		}
		else
		{
			bAssault = true;
			if (AI_isSneakAttackPreparing(eTarget))
				bPreparingAssault = true;
		}
	}

	// K-Mod - based on idea from BBAI
	if (bTargets && iAreaCities > 0 && getNumWars() > 0)
	{
		int iPower = countPowerByArea(pArea);
		int iEnemyPower = countEnemyPowerByArea(pArea);
		iPower *=
				AI_limitedWarPowerRatio() // advc.107: was 100 flat (see Karadoc's comment below on personality)
				+ iWarSuccessRating + ((bChosenTargets
				|| !bRecentAttack) // advc.107
				? 100 : 70); // advc.107: Was 100 : 50
		iEnemyPower *= 100;
		/*  it would be nice to put some personality modifiers into this.
			But this is a Team function. :( */
		if (iPower < iEnemyPower)
			return AREAAI_DEFENSIVE;
	} // K-Mod end

	if (bDeclaredTargets)
		return AREAAI_OFFENSIVE;

	if (bTargets)
	{
		/* BBAI code. -- This code has two major problems.
		* Firstly, it makes offense more likely when we are in more wars.
		* Secondly, it chooses offense based on how many offense units we have --
		* but offense units are built for offense areas!
		*
		// AI_countMilitaryWeight is based on this team's pop and cities ...
		// if this team is the biggest, it will over estimate needed units
		int iMilitaryWeight = AI_countMilitaryWeight(pArea);
		int iCount = 1;
		for (int iJ = 0; iJ < MAX_CIV_TEAMS; iJ++) {
			if (iJ != getID() && GET_TEAM((TeamTypes)iJ).isAlive()) {
				if (!(GET_TEAM((TeamTypes)iJ).isBarbarian() ||
						GET_TEAM((TeamTypes)iJ).isMinorCiv())) {
					if (AI_getWarPlan((TeamTypes)iJ) != NO_WARPLAN) {
						iMilitaryWeight += GET_TEAM((TeamTypes)iJ).
								AI_countMilitaryWeight(pArea);
						iCount++;
						if (GET_TEAM((TeamTypes)iJ).isAVassal()) {
							for (int iK = 0; iK < MAX_CIV_TEAMS; iK++) {
								if (iK != getID() && GET_TEAM((TeamTypes)iK).isAlive()) {
									if (GET_TEAM((TeamTypes)iJ).isVassal((TeamTypes)iK)) {
										iMilitaryWeight += GET_TEAM((TeamTypes)iK).
												AI_countMilitaryWeight(pArea);
		} } } } } } } }
		iMilitaryWeight /= iCount;
		if (countNumAIUnitsByArea(pArea, UNITAI_ATTACK) +
				countNumAIUnitsByArea(pArea, UNITAI_ATTACK_CITY) +
				countNumAIUnitsByArea(pArea, UNITAI_PILLAGE) +
				countNumAIUnitsByArea(pArea, UNITAI_ATTACK_AIR) >
				(iMilitaryWeight * iOffensiveThreshold) / 100 + 1)
			return AREAAI_OFFENSIVE;*/
		/*  K-Mod. I'm not sure how best to do this yet. Let me just try a rough
			idea for now. I'm using AI_countMilitaryWeight; but what I really
			want is "border territory which needs defending" */
		int iOurRelativeStrength = 100 * countPowerByArea(pArea) /
				(AI_countMilitaryWeight(pArea) + 20);
		iOurRelativeStrength *= 100 + (bDeclaredTargets ? 30 : 0) +
				(bPreparingTotal ? -20 : 0) + iWarSuccessRating/2;
		iOurRelativeStrength /= 100;
		int iEnemyRelativeStrength = 0;
		bool bEnemyCities = false;

		for (int iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
		{
			const CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iJ);
			if (iJ != getID() && kLoopTeam.isAlive() && AI_getWarPlan((TeamTypes)iJ) != NO_WARPLAN)
			{
				int iPower = 100 * kLoopTeam.countPowerByArea(pArea);
				int iCommitment = (bPreparingTotal ? 30 : 20) +
						kLoopTeam.AI_countMilitaryWeight(pArea) *
						((isAtWar((TeamTypes)iJ) ? 1 : 2) +
						kLoopTeam.getNumWars(true, true)) / 2;
				iPower /= iCommitment;
				iEnemyRelativeStrength += iPower;
				if (kLoopTeam.countNumCitiesByArea(pArea) > 0)
					bEnemyCities = true;
			}
		}
		if (bEnemyCities && iOurRelativeStrength > iEnemyRelativeStrength)
			return AREAAI_OFFENSIVE;
		// K-Mod end
	}

	if (bTargets)
	{
		for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
		{
			CvPlayerAI const& kMember = GET_PLAYER((PlayerTypes)iPlayer);
			if (!kMember.isAlive() || kMember.getTeam() != getID())
				continue;

			if (kMember.AI_isDoStrategy(AI_STRATEGY_DAGGER) ||
					kMember.AI_isDoStrategy(AI_STRATEGY_FINAL_WAR))
			{
				if (pArea->getCitiesPerPlayer(kMember.getID()) > 0)
					return AREAAI_MASSING;
			}
		}
		if (bRecentAttack)
		{
			int iPower = countPowerByArea(pArea);
			int iEnemyPower = countEnemyPowerByArea(pArea);
			if (iPower > iEnemyPower)
				return AREAAI_MASSING;
			return AREAAI_DEFENSIVE;
		}
	}

	if (iAreaCities > 0 && countEnemyDangerByArea(pArea) > iAreaCities)
		return AREAAI_DEFENSIVE;

	if (bChosenTargets)
		return AREAAI_MASSING;

	if (bTargets)
	{
		if (iAreaCities > (getNumMembers() * 3))
		{
			if (GC.getGame().isOption(GAMEOPTION_AGGRESSIVE_AI) ||
					GC.getGame().isOption(GAMEOPTION_ALWAYS_WAR) ||
					(countPowerByArea(pArea) >
					((countEnemyPowerByArea(pArea) * 3) / 2)))
				return AREAAI_MASSING;
		}
		return AREAAI_DEFENSIVE;
	}
	else if (bAssault)
	{
		if (AI_isPrimaryArea(pArea))
		{
			if (bPreparingAssault)
				return AREAAI_ASSAULT_MASSING;
		}
		else if (countNumCitiesByArea(pArea) > 0)
			return AREAAI_ASSAULT_ASSIST;
		return AREAAI_ASSAULT;
	}
	return AREAAI_NEUTRAL;
}


int CvTeamAI::AI_calculateAdjacentLandPlots(TeamTypes eTeam) const
{
	PROFILE_FUNC();

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	int iCount = 0;
	for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
	{
		CvPlot const& kPlot = GC.getMap().getPlotByIndex(iI);
		if (!kPlot.isWater())
		{
			if (kPlot.getTeam() == eTeam && kPlot.isAdjacentTeam(getID(), true))
				iCount++;
		}
	}
	return iCount;
}

/*  advc.003j: These three functions (2x BtS, 1x BBAI) were obsoleted by K-Mod
	(used to be called from CvTeamAI::AI_startWarVal) */
#if 0
int CvTeamAI::AI_calculatePlotWarValue(TeamTypes eTeam) const
{
	FAssert(eTeam != getID());

	int iValue = 0;

	for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(iI);

		if (pLoopPlot->getTeam() == eTeam)
		{
			if (!pLoopPlot->isWater() && pLoopPlot->isAdjacentTeam(getID(), true))
			{
				iValue += 4;
			}
			// BETTER_BTS_AI_MOD, Notes, 06/17/08, jdog5000:
			/*  This section of code does nothing without XML modding as AIObjective is 0 for all bonuses
				Left alone for mods to use
				Resource driven war in BBAI is done with CvTeamAI::AI_calculateBonusWarValue
				without using the XML variable AIObjective */
			BonusTypes eBonus = pLoopPlot->getBonusType(getID());
			if (NO_BONUS != eBonus)
			{
				iValue += 40 * GC.getInfo(eBonus).getAIObjective();
			}
		}
	}

	return iValue;
}

// BETTER_BTS_AI_MOD, War Strategy AI, 07/21/08, jdog5000: START
int CvTeamAI::AI_calculateBonusWarValue(TeamTypes eTeam) const
{
	FAssert(eTeam != getID());

	int iValue = 0;

	for (int iI = 0; iI < GC.getMap().numPlots(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(iI);

		if (pLoopPlot->getTeam() == eTeam)
		{
			BonusTypes eNonObsoleteBonus = pLoopPlot->getNonObsoleteBonusType(getID());
			if (NO_BONUS != eNonObsoleteBonus)
			{
				int iThisValue = 0;
				for ( int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
				{
					if (getID() == TEAMID((PlayerTypes)iJ) && GET_PLAYER((PlayerTypes)iJ).isAlive())
					{
						// 10 seems like a typical value for a health/happiness resource the AI doesn't have
						// Values for strategic resources can be 60 or higher
						iThisValue += GET_PLAYER((PlayerTypes)iJ).AI_bonusVal(eNonObsoleteBonus, 1, true);
					}
				}
				iThisValue /= getAliveCount();

				if (!pLoopPlot->isWater())
				{
					if (pLoopPlot->isAdjacentTeam(getID(), true))
					{
						iThisValue *= 3;
					}
					else
					{
						CvCity* pWorkingCity = pLoopPlot->getWorkingCity();
						if (pWorkingCity != NULL)
						{
							for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
							{
								if (getID() == TEAMID((PlayerTypes)iJ) && GET_PLAYER((PlayerTypes)iJ).isAlive())
								{
									if (pWorkingCity->AI_playerCloseness((PlayerTypes)iJ) > 0)
									{
										iThisValue *= 2;
										break;
									}
								}
							}
						}
					}
				}

				iThisValue = std::max(0, iThisValue - 4);
				iThisValue /= 5;

				iValue += iThisValue;
			}
		}
	}

	return iValue;
} // BETTER_BTS_AI_MOD: END


int CvTeamAI::AI_calculateCapitalProximity(TeamTypes eTeam) const
{
	CvCity* pOurCapitalCity;
	CvCity* pTheirCapitalCity;
	int iTotalDistance;
	int iCount;
	int iI, iJ;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	iTotalDistance = 0;
	iCount = 0;

	int iMinDistance = MAX_INT;
	int iMaxDistance = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				pOurCapitalCity = GET_PLAYER((PlayerTypes)iI).getCapitalCity();

				if (pOurCapitalCity != NULL)
				{
					for (iJ = 0; iJ < MAX_PLAYERS; iJ++)
					{
						if (GET_PLAYER((PlayerTypes)iJ).isAlive())
						{
							if (GET_PLAYER((PlayerTypes)iJ).getTeam() != getID())
							{
								pTheirCapitalCity = GET_PLAYER((PlayerTypes)iJ).getCapitalCity();

								if (pTheirCapitalCity != NULL)
								{
									int iDistance = (plotDistance(pOurCapitalCity->getX(), pOurCapitalCity->getY(), pTheirCapitalCity->getX(), pTheirCapitalCity->getY()) * (pOurCapitalCity->area() != pTheirCapitalCity->area() ? 3 : 2));
									if (GET_PLAYER((PlayerTypes)iJ).getTeam() == eTeam)
									{
										iTotalDistance += iDistance;
										iCount++;
									}
									iMinDistance = std::min(iDistance, iMinDistance);
									iMaxDistance = std::max(iDistance, iMaxDistance);
								}
							}
						}
					}
				}
			}
		}
	}

	if (iCount > 0)
	{
		FAssert(iMaxDistance > 0);
		return ((GC.getMap().maxPlotDistance() * (iMaxDistance - ((iTotalDistance / iCount) - iMinDistance))) / iMaxDistance);
	}

	return 0;
}
#endif // advc.003j

// K-Mod. Return true if we can deduce the location of at least 'iMiniumum' cities belonging to eTeam.
bool CvTeamAI::AI_haveSeenCities(TeamTypes eTeam, bool bPrimaryAreaOnly, int iMinimum) const
{
	int iCount = 0;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayer const& kMember = GET_PLAYER((PlayerTypes)i);
		if (kMember.getTeam() != eTeam)
			continue; // advc

		FOR_EACH_CITY(pLoopCity, kMember)
		{
			if (AI_deduceCitySite(pLoopCity))
			{
				if (!bPrimaryAreaOnly || AI_isPrimaryArea(pLoopCity->area()))
				{
					if (++iCount >= iMinimum)
						return true;
				}
			}
		}
	}
	return false;
}
// K-Mod end

bool CvTeamAI::AI_isWarPossible() const
{
	if (getNumWars(false) > 0)
		return true;
	/*  advc (comment): The option applies only to humans but still implies that
		all the non-human civs will have a (human) war enemy. */
	if (GC.getGame().isOption(GAMEOPTION_ALWAYS_WAR))
		return true;
	return (!GC.getGame().isOption(GAMEOPTION_ALWAYS_PEACE) &&
			!GC.getGame().isOption(GAMEOPTION_NO_CHANGING_WAR_PEACE));
}

// This function has been completely rewritten for K-Mod. The original BtS code, and the BBAI code have been deleted.
bool CvTeamAI::AI_isLandTarget(TeamTypes eTeam) const
{
	PROFILE_FUNC();

	// <advc.104s>
	if(getWPAI.isEnabled())
		return warAndPeaceAI().isLandTarget(eTeam);
	// </advc.104s>

	const CvTeamAI& kOtherTeam = GET_TEAM(eTeam);
	FOR_EACH_AREA_VAR(pLoopArea)
	{
		if (AI_isPrimaryArea(pLoopArea) && kOtherTeam.AI_isPrimaryArea(pLoopArea))
			return true;
	}

	for (PlayerTypes i = (PlayerTypes)0; i < MAX_PLAYERS; i=(PlayerTypes)(i+1))
	{
		const CvPlayerAI& kOurPlayer = GET_PLAYER(i);
		if (kOurPlayer.getTeam() != getID() || !kOurPlayer.isAlive())
			continue;

		FOR_EACH_CITY(pOurCity, kOurPlayer)
		{
			if (!kOurPlayer.AI_isPrimaryArea(pOurCity->area()))
				continue;
			// city in a primary area.
			for (PlayerTypes j = (PlayerTypes)0; j < MAX_PLAYERS; j=(PlayerTypes)(j+1))
			{
				const CvPlayerAI& kTheirPlayer = GET_PLAYER(j);
				if (kTheirPlayer.getTeam() != eTeam || !kTheirPlayer.isAlive() || !kTheirPlayer.AI_isPrimaryArea(pOurCity->area()))
					continue;

				std::vector<TeamTypes> teamVec;
				teamVec.push_back(getID());
				teamVec.push_back(eTeam);
				FAStar* pTeamStepFinder = gDLL->getFAStarIFace()->create();
				gDLL->getFAStarIFace()->Initialize(pTeamStepFinder, GC.getMap().getGridWidth(), GC.getMap().getGridHeight(), GC.getMap().isWrapX(), GC.getMap().isWrapY(), stepDestValid, stepHeuristic, stepCost, teamStepValid, stepAdd, NULL, NULL);
				gDLL->getFAStarIFace()->SetData(pTeamStepFinder, &teamVec);

				FOR_EACH_CITY(pTheirCity, kTheirPlayer)
				{
					if (pTheirCity->area() != pOurCity->area())
						continue;


					if (gDLL->getFAStarIFace()->GeneratePath(pTeamStepFinder, pOurCity->getX(), pOurCity->getY(), pTheirCity->getX(), pTheirCity->getY(), false, 0, true))
					{
						// good.
						gDLL->getFAStarIFace()->destroy(pTeamStepFinder);
						return true;
					}
				}
				gDLL->getFAStarIFace()->destroy(pTeamStepFinder);
			}
		}
	}

	return false;
}

// this determines if eTeam or any of its allies are land targets of us
bool CvTeamAI::AI_isAllyLandTarget(TeamTypes eTeam) const
{
	for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
	{
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
		if (iTeam != getID())
		{
			if (iTeam == eTeam || kLoopTeam.isVassal(eTeam) || GET_TEAM(eTeam).isVassal((TeamTypes)iTeam) || kLoopTeam.isDefensivePact(eTeam))
			{
				if (AI_isLandTarget((TeamTypes)iTeam))
					return true;
			}
		}
	}

	return false;
}


bool CvTeamAI::AI_shareWar(TeamTypes eTeam) const
{
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive() && !GET_TEAM((TeamTypes)iI).isMinorCiv())
		{
			if (iI != getID() && iI != eTeam)
			{
				if (isAtWar((TeamTypes)iI) && GET_TEAM(eTeam).isAtWar((TeamTypes)iI))
					return true;
			}
		}
	}
	return false;
}

// <advc> This error-prone double loop is needed in several places
void CvTeamAI::AI_updateAttitudeCache(TeamTypes eTeam, /* advc.130e: */ bool bUpdateWorstEnemy)
{
	/*  The game has every team meet itself during initialization, but a call on
		self also happens when a colony is created. */
	if(!GC.getGame().isFinalInitialized() || eTeam == getID())
		return;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayerAI& ourMember = GET_PLAYER((PlayerTypes)i);
		if(ourMember.getTeam() != getID() || !ourMember.isAlive() ||
				ourMember.isMinorCiv())
			continue;
		for(int j = 0; j < MAX_CIV_PLAYERS; j++)
		{
			CvPlayerAI& theirMember = GET_PLAYER((PlayerTypes)j);
			if(theirMember.getTeam() != eTeam || !theirMember.isAlive() ||
					theirMember.isMinorCiv())
				continue;
			ourMember.AI_updateAttitudeCache(theirMember.getID(),
					bUpdateWorstEnemy); // advc.130e
		}
	}
} // </advc>

AttitudeTypes CvTeamAI::AI_getAttitude(TeamTypes eTeam, bool bForced) const
{
	PROFILE_FUNC(); // advc: Not _entirely_ negligible currently, but pretty much.
	//FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");
	// K-Mod
	if (eTeam == getID())
		return ATTITUDE_FRIENDLY;
	// K-Mod end

	int iAttitude = 0;
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isAlive() && iI != iJ)
					{
						TeamTypes eTeamLoop = GET_PLAYER((PlayerTypes)iJ).getTeam();
						//if (eTeamLoop == eTeam || GET_TEAM(eTeamLoop).isVassal(eTeam) || GET_TEAM(eTeam).isVassal(eTeamLoop))
						if (eTeamLoop == eTeam) // K-Mod. Removed attitude averaging between vassals and masters
						{
							//iAttitude += GET_PLAYER((PlayerTypes)iI).AI_getAttitude((PlayerTypes)iJ, bForced);
							// bbai. Average values rather than attitudes directly.
							iAttitude += GET_PLAYER((PlayerTypes)iI).AI_getAttitudeVal((PlayerTypes)iJ, bForced);
							iCount++;
						}
					}
				}
			}
		}
	}
	if (iCount > 0)
	{
		// return ((AttitudeTypes)(iAttitude / iCount));
		return CvPlayerAI::AI_getAttitudeFromValue(iAttitude/iCount); // bbai / K-Mod
	}

	return ATTITUDE_CAUTIOUS;
}


int CvTeamAI::AI_getAttitudeVal(TeamTypes eTeam, bool bForced) const  // advc: style changes
{
	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	int iAttitudeVal = 0;
	int iCount = 0;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayerAI const& kOurMember = GET_PLAYER((PlayerTypes)i);
		if (!kOurMember.isAlive() || kOurMember.getTeam() != getID())
			continue;
		for (int j = 0; j < MAX_PLAYERS; j++)
		{
			CvPlayer const& kTheirMember = GET_PLAYER((PlayerTypes)j);
			if (!kTheirMember.isAlive() || kTheirMember.getTeam() != eTeam)
				continue;
			iAttitudeVal += kOurMember.AI_getAttitudeVal(kTheirMember.getID(), bForced);
			iCount++;
		}
	}
	if (iCount > 0)
		return (iAttitudeVal / iCount);
	return 0;
}


int CvTeamAI::AI_getMemoryCount(TeamTypes eTeam, MemoryTypes eMemory) const
{
	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	int iMemoryCount = 0;
	int iCount = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isAlive())
					{
						if (GET_PLAYER((PlayerTypes)iJ).getTeam() == eTeam)
						{
							iMemoryCount += GET_PLAYER((PlayerTypes)iI).AI_getMemoryCount(((PlayerTypes)iJ), eMemory);
							iCount++;
						}
					}
				}
			}
		}
	}
	if (iCount > 0)
		return iMemoryCount / iCount;
	return 0;
}


int CvTeamAI::AI_chooseElection(const VoteSelectionData& kVoteSelectionData) const
{
	VoteSourceTypes eVoteSource = kVoteSelectionData.eVoteSource;

	FAssert(!isHuman());
	FAssert(GC.getGame().getSecretaryGeneral(eVoteSource) == getID());

	int iBestVote = -1;
	int iBestValue = 0;

	for (int iI = 0; iI < (int)kVoteSelectionData.aVoteOptions.size(); ++iI)
	{
		VoteTypes eVote = kVoteSelectionData.aVoteOptions[iI].eVote;
		CvVoteInfo& kVoteInfo = GC.getInfo(eVote);

		FAssert(kVoteInfo.isVoteSourceType(eVoteSource));

		FAssert(GC.getGame().isChooseElection(eVote));
		bool bValid = true;
		bool bCanWinDiplo = false; // advc.115b
		if (!GC.getGame().isTeamVote(eVote))
		{
			for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
			{
				if (GET_PLAYER((PlayerTypes)iJ).isAlive())
				{
					if (GET_PLAYER((PlayerTypes)iJ).getTeam() == getID())
					{
						PlayerVoteTypes //eVote =
								ePlayerVote = // dlph.25: 'Same variable name was confusing'
								GET_PLAYER((PlayerTypes)iJ).AI_diploVote(kVoteSelectionData.aVoteOptions[iI], eVoteSource, true);
						//if (eVote != PLAYER_VOTE_YES || eVote == GC.getGame().getVoteOutcome((VoteTypes)iI))
						/*  <dlph.25> Replacing the above.
							'AI can choose to repeal an already passed resolution
							if all team members agree' */
						bool bVoteYes = (ePlayerVote == PLAYER_VOTE_YES);
						bool bAlreadyPassed = (GC.getGame().getVoteOutcome(
								eVote) == PLAYER_VOTE_YES);
						  if((bVoteYes && bAlreadyPassed) || (!bVoteYes && !bAlreadyPassed))
						// </dlph.25>
						{
							bValid = false;
							break;
						}
					}
				}
			}
		}
		// <advc.115b>
		else if(!isAVassal())
			bCanWinDiplo = AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_DIPLOMACY4);
		// </advc.115b>
		if (bValid)
		{
			int iValue = (1 + GC.getGame().getSorenRandNum(10000, "AI Choose Vote"));
			/*  <advc.115b> Always pick victory. Probabilistically instead? 8000
				would be an 80% chance if there's one other proposal. */
			if(bCanWinDiplo)
				iValue += 20000; // </advc.115b>
			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				iBestVote = iI;
			}
		}
	}

	return iBestVote;
}

// <advc> Code cut from CvTeam::declareWar in order to reduce entanglement of AI and game rule code
void CvTeamAI::AI_preDeclareWar(TeamTypes eTarget, WarPlanTypes eWarPlan, bool bPrimaryDoW,
	PlayerTypes eSponsor) // advc.100
{
	CvTeamAI& kTarget = GET_TEAM(eTarget);
	/*  <advc.104q> At this point, the state counter says how long war has been
		imminent - that's no longer important once war is declared. Should instead
		count the war duration. (Note that AI_getAtWarCounter isn't an accurate counter.) */
	AI_setWarPlanStateCounter(eTarget, 0);
	kTarget.AI_setWarPlanStateCounter(getID(), 0);
	// </advc.104q>
	/*  advc.130h: Moved this loop into AI_preDeclareWar b/c I'm adding code that
		depends on the atWarWithPartner status before the DoW */
	// K-Mod. Same functionality as in BtS, but a bit cleaner and a bit faster.
	for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i = (PlayerTypes)(i+1))  // advc.003n: was MAX_PLAYERS
	{
		CvPlayerAI& kPlayer_i = GET_PLAYER(i);
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
				kTarget.AI_getMemoryCount(getID(), MEMORY_MADE_DEMAND) > 0 &&
				GET_TEAM(j).getMasterTeam() != getMasterTeam() &&
				kTarget.isHasMet(kPlayer_j.getTeam()))
			{
				// Raise it to 8 (or what XML says)
				int iMemory = kPlayer_j.AI_getMemoryCount(i, MEMORY_MADE_DEMAND_RECENT);
				int iDelta = GC.getDefineINT("WAR_DESPITE_TRIBUTE_MEMORY");
				iDelta = std::max(iMemory, iDelta) - iMemory;
				kPlayer_j.AI_changeMemoryCount(i, MEMORY_MADE_DEMAND_RECENT, iDelta);
			}
			if(bPrimaryDoW && i != j)
				kPlayer_i.AI_setMemoryCount(j, MEMORY_MADE_DEMAND, 0);
			// </advc.130o>
			if (kPlayer_j.getTeam() == eTarget)
			{
				if(bPrimaryDoW) // advc.130y
					kPlayer_j.AI_rememberEvent(i, MEMORY_DECLARED_WAR); // advc.130j
				// advc.130y:
				else kPlayer_j.AI_changeMemoryCount(i, MEMORY_DECLARED_WAR, 2);
			} // advc.130h:
			if(kPlayer_j.AI_disapprovesOfDoW(getID(), eTarget)) // advc.130j:
				kPlayer_j.AI_rememberEvent(i, MEMORY_DECLARED_WAR_ON_FRIEND);
		}
	} // K-Mod end.
	// <advc.104i>
	if(eSponsor != NO_PLAYER)
	{
		AI_makeUnwillingToTalk(eTarget);
		if(GET_TEAM(eSponsor).isAtWar(eTarget))
			GET_TEAM(eSponsor).AI_makeUnwillingToTalk(eTarget);
	} // </advc.104i>
}

void CvTeamAI::AI_postDeclareWar(TeamTypes eTarget, WarPlanTypes eWarPlan)
{
	CvTeamAI& kTarget = GET_TEAM(eTarget);

	AI_setAtPeaceCounter(eTarget, 0);
	kTarget.AI_setAtPeaceCounter(getID(), 0);
	AI_setShareWarCounter(eTarget, 0);
	kTarget.AI_setShareWarCounter(getID(), 0);
	kTarget.AI_setWarPlan(getID(), (isBarbarian() || isMinorCiv()) ?
			WARPLAN_ATTACKED : WARPLAN_ATTACKED_RECENT);
	if (!getWPAI.isEnabled()) // advc.104
	{
		for (int i = 0; i < MAX_TEAMS; i++)
		{
			CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)i);
			if (!kLoopTeam.isAlive())
				continue;

			if (!kLoopTeam.isAtWar(eTarget) && kTarget.AI_isChosenWar(kLoopTeam.getID()))
				kTarget.AI_setWarPlan(kLoopTeam.getID(), NO_WARPLAN);
		}
	}
	if (eWarPlan != NO_WARPLAN)
		AI_setWarPlan(eTarget, eWarPlan);

	FAssert(!AI_isSneakAttackPreparing(eTarget)
		/*  advc.104o: Can happen when hired to declare war while preparing.
			BtS/K-Mod doesn't allow hired war while preparing, but UWAI does.
			The K-Mod code below already handles WARPLAN_PREPARING_..., so, no problem. */
			|| getWPAI.isEnabled());
	if (AI_getWarPlan(eTarget) == NO_WARPLAN || AI_isSneakAttackPreparing(eTarget))
	{
		if (isHuman()
				// K-Mod. (for vassals that have been told to prepare for war)
				|| AI_getWarPlan(eTarget) == WARPLAN_PREPARING_TOTAL)
			AI_setWarPlan(eTarget, WARPLAN_TOTAL);
		else if (isMinorCiv() || isBarbarian() || kTarget.getNumWars() == 1)
			AI_setWarPlan(eTarget, WARPLAN_LIMITED);
		else AI_setWarPlan(eTarget, WARPLAN_DOGPILE);
	}
}


void CvTeamAI::AI_preMakePeace(TeamTypes eTarget, CLinkList<TradeData>* pReparations)
{
	CvTeamAI& kTarget = GET_TEAM(eTarget);
	// <advc.104> Report who won the war before war success is reset
	warAndPeaceAI().reportWarEnding(eTarget, pReparations, NULL);
	kTarget.warAndPeaceAI().reportWarEnding(getID(), NULL, pReparations);
	// </advc.104>
	/*  <advc.130y> Don't know if they started the war, but, if we did and they had
		started a war against us some time earlier, we may as well forgive them for
		that. (If there's no declared-war-on-us memory, then this call has no effect.) */
	AI_forgiveEnemy(eTarget, isCapitulated(), false);
	kTarget.AI_forgiveEnemy(getID(), kTarget.isCapitulated(), false);
	// </advc.130y>  <advc.130i>
	if (getWPAI.isEnabled() && !isAVassal() && !kTarget.isAVassal())
	{
		for (int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			CvPlayerAI& kDPPlayer = GET_PLAYER((PlayerTypes)i);
			if (!kDPPlayer.isAlive() || kDPPlayer.getTeam() == getID() ||
					kDPPlayer.getTeam() == eTarget)
				continue;
			CvTeam const& kDPTeam = GET_TEAM(kDPPlayer.getTeam());
			if (!kDPTeam.isDefensivePact(eTarget) && !kDPTeam.isDefensivePact(getID()))
				continue;
			FAssert(!kDPTeam.isAVassal());
			for (int j = 0; j < MAX_CIV_PLAYERS; j++)
			{
				CvPlayerAI& kMember = GET_PLAYER((PlayerTypes)j);
				if(!kMember.isAlive() || (kMember.getTeam() != eTarget &&
						kMember.getTeam() != getID()))
					continue;
				kDPPlayer.AI_setMemoryCount(kMember.getID(), MEMORY_DECLARED_WAR_RECENT, 0);
				kMember.AI_setMemoryCount(kDPPlayer.getID(), MEMORY_DECLARED_WAR_RECENT, 0);
			}
		}
	} // </advc.130i>	
}


void CvTeamAI::AI_postMakePeace(TeamTypes eTarget)
{
	CvTeamAI& kTarget = GET_TEAM(eTarget);
	AI_setAtWarCounter(eTarget, 0);
	kTarget.AI_setAtWarCounter(getID(), 0);

	AI_setWarSuccess(eTarget, 0);
	kTarget.AI_setWarSuccess(getID(), 0);

	AI_setWarPlan(eTarget, NO_WARPLAN);
	kTarget.AI_setWarPlan(getID(), NO_WARPLAN);

	// K-Mod. update attitude
	if (!GC.getGame().isFinalInitialized())
		return;

	for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
	{
		const CvTeam& kTeam_i = GET_TEAM(GET_PLAYER(i).getTeam());
		if (kTeam_i.getID() == eTarget || kTeam_i.getID() == getID() ||
			kTeam_i.isAtWar(eTarget) || kTeam_i.isAtWar(getID()))
		{
			for (PlayerTypes j = (PlayerTypes)0; j < MAX_CIV_PLAYERS; j=(PlayerTypes)(j+1))
			{	// <advc.001>
				if(i == j)
					continue; // </advc.001>
				TeamTypes eTeam_j = GET_PLAYER(j).getTeam();
				if (eTeam_j == eTarget || eTeam_j == getID())
					GET_PLAYER(i).AI_updateAttitudeCache(j);
			}
		}
	} // K-Mod end
} // </advc>

// K-Mod. New war evaluation functions. WIP
// Very rough estimate of what would be gained by conquering the target - in units of Gold/turn (kind of).
int CvTeamAI::AI_warSpoilsValue(TeamTypes eTarget, WarPlanTypes eWarPlan,
	bool bConstCache) const // advc.001n
{
	PROFILE_FUNC();

	FAssert(eTarget != getID());
	const CvTeamAI& kTargetTeam = GET_TEAM(eTarget);
	bool bAggresive = GC.getGame().isOption(GAMEOPTION_AGGRESSIVE_AI);

	// Deny factor: the percieved value of denying the enemy team of its resources (generally)
	int iDenyFactor = bAggresive
		? 40 - AI_getAttitudeWeight(eTarget)/3
		: 20 - AI_getAttitudeWeight(eTarget)/2;

	iDenyFactor += AI_getWorstEnemy() == eTarget ? 20 : 0; // (in addition to attitude pentalities)

	if (kTargetTeam.AI_isAnyMemberDoVictoryStrategyLevel3())
	{
		if (kTargetTeam.AI_isAnyMemberDoVictoryStrategyLevel4())
		{
			iDenyFactor += AI_isAnyMemberDoVictoryStrategyLevel4() ? 50 : 30;
		}

		if (bAggresive || AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST3))
		{
			iDenyFactor += 20;
		}

		if (GC.getGame().getTeamRank(eTarget) < GC.getGame().getTeamRank(getID()))
		{
			iDenyFactor += 10;
		}
	}
	if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST4 | AI_VICTORY_DOMINATION4))
	{
		iDenyFactor += 20;
	}

	int iRankDelta = GC.getGame().getTeamRank(getID()) - GC.getGame().getTeamRank(eTarget);
	if (iRankDelta > 0)
	{
		int iRankHate = 0;
		for (int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			const CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
			if (kLoopPlayer.getTeam() == getID() && kLoopPlayer.isAlive())
				iRankHate += GC.getInfo(kLoopPlayer.getPersonalityType()).getWorseRankDifferenceAttitudeChange(); // generally around 0-3
		}

		if (iRankHate > 0)
		{
			int iTotalTeams = GC.getGame().getCivTeamsEverAlive();
			iDenyFactor += (100 - AI_getAttitudeWeight(eTarget)) *
					(iRankHate * iRankDelta + (iTotalTeams+1)/2) /
					std::max(1, 8*(iTotalTeams + 1)*getAliveCount());
			// that's a max of around 200 * 3 / 8. ~ 75
		}
	}

	bool bImminentVictory = kTargetTeam.AI_getLowestVictoryCountdown() >= 0;

	bool bTotalWar = eWarPlan == WARPLAN_TOTAL || eWarPlan == WARPLAN_PREPARING_TOTAL;
	bool bOverseasWar = !AI_hasSharedPrimaryArea(eTarget);

	int iGainedValue = 0;
	int iDeniedValue = 0;

	// Cities & Land
	int iPopCap = 2 + getTotalPopulation(false) / std::max(1, getNumCities()); // max number of plots to consider the value of.
	int iYieldMultiplier = 0; // multiplier for the value of plot yields.
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayerAI const& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
		if (kLoopPlayer.getTeam() != getID() || !kLoopPlayer.isAlive())
			continue;
		int iFoodMulti = kLoopPlayer.AI_averageYieldMultiplier(YIELD_FOOD);
		iYieldMultiplier += kLoopPlayer.AI_averageYieldMultiplier(YIELD_PRODUCTION) * iFoodMulti / 100;
		iYieldMultiplier += kLoopPlayer.AI_averageYieldMultiplier(YIELD_COMMERCE) * iFoodMulti / 100;
	}
	iYieldMultiplier /= std::max(1, 2 * getAliveCount());
	// now.. here's a bit of ad-hoccery.
	// the actual yield multiplayer is not the only thing that goes up as the game progresses.
	// the raw produce of land also tends to increase, as improvements become more powerful. Therefore...:
	iYieldMultiplier = iYieldMultiplier * (1 + GET_PLAYER(getLeaderID()).getCurrentEra() + GC.getNumEraInfos()) / std::max(1, GC.getNumEraInfos());
	//

	std::set<int> close_areas; // set of area IDs for which the enemy has cities close to ours.
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		const CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
		if (kLoopPlayer.getTeam() != eTarget || !kLoopPlayer.isAlive())
			continue;

		FOR_EACH_CITYAI(pLoopCity, kLoopPlayer)
		{
			if (!AI_deduceCitySite(pLoopCity))
				continue;

			int iCityValue = pLoopCity->getPopulation();

			bool bCoastal = pLoopCity->isCoastal();
			iCityValue += bCoastal ? 2 : 0;

			// plots
			std::vector<int> plot_values;
			for (int j = 1; j < NUM_CITY_PLOTS; j++) // don't count city plot
			{
				CvPlot* pLoopPlot = pLoopCity->getCityIndexPlot(j);
				if (!pLoopPlot || !pLoopPlot->isRevealed(getID()) ||
						pLoopPlot->getWorkingCity() != pLoopCity)
					continue;

				if (pLoopPlot->isWater() && !(bCoastal && pLoopPlot->calculateNatureYield(YIELD_FOOD, getID()) >= GC.getFOOD_CONSUMPTION_PER_POPULATION()))
					continue;

				// This is a very rough estimate of the value of the plot. It's a bit ad-hoc. I'm sorry about that, but I want it to be fast.
				//BonusTypes eBonus = pLoopPlot->getBonusType(getID());
				int iPlotValue = 0;
				iPlotValue += 3 * pLoopPlot->calculateBestNatureYield(YIELD_FOOD, getID()); // don't ignore floodplains
				iPlotValue += 2 * pLoopPlot->calculateNatureYield(YIELD_PRODUCTION, getID(), true); // ignore forest
				iPlotValue += GC.getInfo(pLoopPlot->getTerrainType()).getYield(YIELD_FOOD) >= GC.getFOOD_CONSUMPTION_PER_POPULATION() ? 1 : 0; // bonus for grassland
				iPlotValue += pLoopPlot->isRiver() ? 1 : 0;
				if (pLoopPlot->getBonusType(getID()) != NO_BONUS)
					iPlotValue = iPlotValue * 3/2;
				iPlotValue += pLoopPlot->getYield(YIELD_COMMERCE) / 2; // include some value for existing towns.

				plot_values.push_back(iPlotValue);
			}
			std::partial_sort(plot_values.begin(), plot_values.begin() + std::min(iPopCap, (int)plot_values.size()), plot_values.end(), std::greater<int>());
			iCityValue = std::accumulate(plot_values.begin(), plot_values.begin() + std::min(iPopCap, (int)plot_values.size()), iCityValue);
			iCityValue = iCityValue * iYieldMultiplier / 100;

			// holy city value
			for (ReligionTypes i = (ReligionTypes)0; i < GC.getNumReligionInfos(); i=(ReligionTypes)(i+1))
			{
				if (pLoopCity->isHolyCity(i))
					iCityValue += std::max(0, GC.getGame().countReligionLevels(i) / (pLoopCity->hasShrine(i) ? 1 : 2) - 4);
				// note: the -4 at the end is mostly there to offset the 'wonder' value that will be added later.
				// I don't want to double-count the value of the shrine, and the religion without the shrine isn't worth much anyway.
			}

			// corp HQ value
			for (CorporationTypes i = (CorporationTypes)0; i < GC.getNumCorporationInfos(); i=(CorporationTypes)(i+1))
			{
				if (pLoopCity->isHeadquarters(i))
					iCityValue += std::max(0, 2 * GC.getGame().countCorporationLevels(i) - 4);
			}

			// wonders
			iCityValue += 4 * pLoopCity->getNumActiveWorldWonders();

			// denied
			iDeniedValue += iCityValue * iDenyFactor / 100;
			if (2*pLoopCity->getCulture(kLoopPlayer.getID()) > pLoopCity->getCultureThreshold(GC.getGame().culturalVictoryCultureLevel()))
			{
				iDeniedValue += (kLoopPlayer.AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4) ? 100 : 30) * iDenyFactor / 100;
			}
			if (bImminentVictory && pLoopCity->isCapital())
			{
				iDeniedValue += 200 * iDenyFactor / 100;
			}

			// gained
			int iGainFactor = 0;
			if (bTotalWar)
			{
				if (AI_isPrimaryArea(pLoopCity->area()))
					iGainFactor = 70;
				else
				{
					if (bOverseasWar && GET_PLAYER(pLoopCity->getOwner()).AI_isPrimaryArea(pLoopCity->area()))
						iGainFactor = 45;
					else
						iGainFactor = 30;

					iGainFactor += AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST3 | AI_VICTORY_DOMINATION2) ? 10 : 0;
				}
			}
			else
			{
				if (AI_isPrimaryArea(pLoopCity->area()))
					iGainFactor = 40;
				else
					iGainFactor = 25;
			}
			if (pLoopCity->AI_highestTeamCloseness(getID(), bConstCache) > 0)
			{
				iGainFactor += 30;
				close_areas.insert(pLoopCity->getArea());
			}

			iGainedValue += iCityValue * iGainFactor / 100;
		}
	}

	// Resources
	std::vector<int> bonuses(GC.getNumBonusInfos(), 0); // percentage points
	for (int i = 0; i < GC.getMap().numPlots(); i++)
	{
		CvPlot* pLoopPlot = GC.getMap().plotByIndex(i);
		if (pLoopPlot->getTeam() == eTarget)
		{
			// note: There are ways of knowning that the team has resources even if the plot cannot be seen; so my handling of isRevealed is a bit ad-hoc.
			BonusTypes eBonus = pLoopPlot->getNonObsoleteBonusType(getID());
			if (eBonus != NO_BONUS)
			{
				if (pLoopPlot->isRevealed(getID()) && AI_isPrimaryArea(pLoopPlot->area()))
					bonuses[eBonus] += bTotalWar ? 60 : 20;
				else bonuses[eBonus] += bTotalWar ? 20 : 0;
			}
		}
	}
	for (BonusTypes i = (BonusTypes)0; i < GC.getNumBonusInfos(); i=(BonusTypes)(i+1))
	{
		if (bonuses[i] > 0)
		{
			int iBonusValue = 0;
			int iMissing = 0;
			for (int j = 0; j < MAX_PLAYERS; j++)
			{
				const CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)j);
				if (kLoopPlayer.getTeam() == getID() && kLoopPlayer.isAlive())
				{
					iBonusValue += kLoopPlayer.AI_bonusVal(i, 0, true);
					if (kLoopPlayer.getNumAvailableBonuses(i) == 0)
						iMissing++;
				}
			}
			iBonusValue += GC.getInfo(i).getAIObjective(); // (support for mods.)
			iBonusValue = iBonusValue * getNumCities() *
					(std::min(100*iMissing, bonuses[i]) +
					std::max(0, bonuses[i] - 100*iMissing)/8) /
					std::max(1, 400 * getAliveCount());
			//
			iGainedValue += iBonusValue;
			// ignore denied value.
		}
	}

	// magnify the gained value based on how many of the target's cities are in close areas
	int iCloseCities = 0;
	for (std::set<int>::iterator it = close_areas.begin(); it != close_areas.end(); ++it)
	{
		CvArea* pLoopArea = GC.getMap().getArea(*it);
		if (AI_isPrimaryArea(pLoopArea))
		{
			for (int i = 0; i < MAX_PLAYERS; i++)
			{
				const CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
				if (kLoopPlayer.getTeam() == eTarget)
					iCloseCities += pLoopArea->getCitiesPerPlayer(kLoopPlayer.getID());
			}
		}
	}
	iGainedValue *= 75 + 50 * iCloseCities / std::max(1, kTargetTeam.getNumCities());
	iGainedValue /= 100;

	// amplify the gained value if we are aiming for a conquest or domination victory
	if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST2 | AI_VICTORY_DOMINATION2))
		iGainedValue = iGainedValue * 4/3;

	// reduce the gained value based on how many other teams are at war with the target
	// similar to the way the target's strength estimate is reduced.
	iGainedValue *= 2;
	iGainedValue /= (isAtWar(eTarget) ? 1 : 2) + kTargetTeam.getNumWars(true, true);

	return iGainedValue + iDeniedValue;
}

int CvTeamAI::AI_warCommitmentCost(TeamTypes eTarget, WarPlanTypes eWarPlan,
	bool bConstCache) const // advc.001n
{
	PROFILE_FUNC();
	// Things to consider:
	//
	// risk of losing cities
	// relative unit strength
	// relative current power
	// productivity
	// war weariness
	// diplomacy
	// opportunity cost (need for expansion, research, etc.)

	// For most parts of the calculation, it is actually more important to consider the master of the target rather than the target itself.
	// (this is important for defensive pacts; and it also affects relative power, relative productivity, and so on.)
	TeamTypes eTargetMaster = GET_TEAM(eTarget).getMasterTeam();
	FAssert(eTargetMaster == eTarget || GET_TEAM(eTarget).isAVassal());

	const CvTeamAI& kTargetMasterTeam = GET_TEAM(eTargetMaster);

	bool bTotalWar = eWarPlan == WARPLAN_TOTAL || eWarPlan == WARPLAN_PREPARING_TOTAL;
	bool bAttackWar = bTotalWar || (eWarPlan == WARPLAN_DOGPILE && kTargetMasterTeam.getNumWars() + (isAtWar(eTarget) ? 0 : 1) > 1);
	bool bPendingDoW = !isAtWar(eTarget) && eWarPlan != WARPLAN_ATTACKED && eWarPlan != WARPLAN_ATTACKED_RECENT;

	int iTotalCost = 0;

	// Estimate of military production costs
	{
		// Base commitment for a war of this type.
		int iCommitmentPerMil = bTotalWar ? 540 : 250;
		const int iBaseline = -25; // this value will be added to iCommitmentPerMil at the end of the calculation.

		// scale based on our current strength relative to our enemies.
		// cf. with code in AI_calculateAreaAIType
		{
			int iWarSuccessRating = isAtWar(eTarget) ? AI_getWarSuccessRating() : 0;
			int iOurRelativeStrength = 100 * getPower(true) / (AI_countMilitaryWeight(0) + 20); // whether to include vassals is a tricky issue...
			// Sum the relative strength for all enemies, including existing wars and wars with civs attached to the target team.
			int iEnemyRelativeStrength = 0;
			int iFreePowerBonus = GC.getInfo(GC.getGame().getBestLandUnit()).getPowerValue() * 2;
			for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
			{
				const CvTeamAI& kLoopTeam = GET_TEAM(i);
				if (!kLoopTeam.isAlive() || i == getID() || kLoopTeam.isVassal(getID()))
					continue;

				// note: this still doesn't count vassal siblings. (ie. if the target is a vassal, this will not count the master's other vassals.)
				// also, the vassals of defensive pact civs are currently not counted either.
				if (isAtWar(i) || i == eTargetMaster || kLoopTeam.isVassal(eTargetMaster) || (bPendingDoW && kLoopTeam.isDefensivePact(eTargetMaster)))
				{
					// the + power is meant to account for the fact that the target may get stronger while we are planning for war - especially early in the game.
					// use a slightly reduced value if we're actually not intending to attack this target. (full weight to all enemies in defensive wars)
					int iWeight = !bAttackWar || isAtWar(i) || i == eTarget ? 100 : 80;
					iEnemyRelativeStrength += iWeight * ((isAtWar(i) ? 0 : iFreePowerBonus) + kLoopTeam.getPower(false)) / (((isAtWar(i) ? 1 : 2) + kLoopTeam.getNumWars(true, true))*kLoopTeam.AI_countMilitaryWeight(0)/2 + 20);
				}
			}
			//

			//iCommitmentPerMil = iCommitmentPerMil * (100 * iEnemyRelativeStrength) / std::max(1, iOurRelativeStrength * (100+iWarSuccessRating/2));
			iCommitmentPerMil = iCommitmentPerMil * iEnemyRelativeStrength / std::max(1, iOurRelativeStrength);
		}

		// scale based on the relative size of our civilizations.
		int iOurProduction = AI_estimateTotalYieldRate(YIELD_PRODUCTION); // (note: this is separate from our total production, because I use it again a bit later.)
		{
			int iOurTotalProduction = iOurProduction * 100;
			int iEnemyTotalProduction = 0;
			const int iVassalFactor = 60; // only count some reduced percentage of vassal production.
			// Note: I've chosen not to count the production of the target's other enemies.

			for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
			{
				const CvTeamAI& kLoopTeam = GET_TEAM(i);
				if (!kLoopTeam.isAlive() || i == getID()) // our team is already counted.
					continue;

				if (kLoopTeam.isVassal(getID()))
					iOurTotalProduction += kLoopTeam.AI_estimateTotalYieldRate(YIELD_PRODUCTION) * iVassalFactor;
				else if (i == eTargetMaster)
					iEnemyTotalProduction += kLoopTeam.AI_estimateTotalYieldRate(YIELD_PRODUCTION) * 100;
				else if (kLoopTeam.isVassal(eTargetMaster))
					iEnemyTotalProduction += kLoopTeam.AI_estimateTotalYieldRate(YIELD_PRODUCTION) * iVassalFactor;
				else if (isAtWar(i) || (bPendingDoW && kLoopTeam.isDefensivePact(eTargetMaster)))
					iEnemyTotalProduction += kLoopTeam.AI_estimateTotalYieldRate(YIELD_PRODUCTION) * 100;
			}

			iCommitmentPerMil *= 6 * iEnemyTotalProduction + iOurTotalProduction;
			iCommitmentPerMil /= std::max(1, iEnemyTotalProduction + 6 * iOurTotalProduction);
		}

		// scale based on the relative strengths of our units
		{
			int iEnemyUnit = std::max(30, kTargetMasterTeam.getTypicalUnitValue(NO_UNITAI, DOMAIN_LAND));
			int iOurAttackUnit = std::max(30, getTypicalUnitValue(UNITAI_ATTACK, DOMAIN_LAND));
			int iOurDefenceUnit = std::max(30, getTypicalUnitValue(UNITAI_CITY_DEFENSE, DOMAIN_LAND));
			int iHighScale = 30 + 70 * std::max(iOurAttackUnit, iOurDefenceUnit) / iEnemyUnit;
			int iLowScale = 10 + 90 * std::min(iOurAttackUnit, iOurDefenceUnit) / iEnemyUnit;

			iCommitmentPerMil = std::min(iCommitmentPerMil, 300) * 100 / iHighScale + std::max(0, iCommitmentPerMil - 300) * 100 / iLowScale;

			// Adjust for overseas wars
			if (!AI_hasSharedPrimaryArea(eTarget))
			{
				int iOurNavy = getTypicalUnitValue(NO_UNITAI, DOMAIN_SEA);
				int iEnemyNavy = std::max(1, kTargetMasterTeam.getTypicalUnitValue(NO_UNITAI, DOMAIN_SEA)); // (using max here to avoid div-by-zero later on)

				// rescale (unused)
				/* {
					int x = std::max(2, iOurNavy + iEnemyNavy) / 2;
					iOurNavy = iOurNavy * 100 / x;
					iEnemyNavy = iEnemyNavy * 100 / x;
				} */

				// Note: Commitment cost is currently meant to take into account risk as well as resource requirements.
				// But with overseas wars, the relative strength of navy units effects these things differently.
				// If our navy is much stronger than theirs, then our risk is low but we still need to commit a
				// just as much resources to win the land-war for an invasion.
				// If their navy is stronger than ours, our risk is high and our resources will be higher too.
				//
				// The current calculations are too simplistic to explicitly specify all that stuff.
				if (bTotalWar)
				{
					//iCommitmentPerMil = iCommitmentPerMil * (4*iOurNavy + 5*iEnemyNavy) / (8*iOurNavy + 1*iEnemyNavy);
					//iCommitmentPerMil = iCommitmentPerMil * 200 / std::min(200, iLowScale + iHighScale);
					//
					iCommitmentPerMil = iCommitmentPerMil * 200 / std::min(240, (iLowScale + iHighScale) * (9*iOurNavy + 1*iEnemyNavy) / (6*iOurNavy + 4*iEnemyNavy));
				}
				else
					iCommitmentPerMil = iCommitmentPerMil * (1*iOurNavy + 4*iEnemyNavy) / (4*iOurNavy + 1*iEnemyNavy);
			}
		}

		// increase the cost for distant targets...
		if (0 == AI_teamCloseness(eTarget,
				-1, false, bConstCache)) // advc.001n
		{
			// ... in the early game.
			if (getNumCities() < GC.getInfo(GC.getMap().getWorldSize()).getTargetNumCities() * getAliveCount())
				iCommitmentPerMil = iCommitmentPerMil * 3/2;
			/* else
				iCommitmentPerMil = iCommitmentPerMil * 5/4; */

			// ... and for total war
			if (bTotalWar)
				iCommitmentPerMil = iCommitmentPerMil * 5/4;
		}

		iCommitmentPerMil += iBaseline; // The baseline should be a negative value which represents some amount of "free" commitment.

		if (iCommitmentPerMil > 0)
		{
			// iCommitmentPerMil will be multiplied by a rough estimate of the total resources this team could devote to war.
			int iCommitmentPool = iOurProduction * 3 + AI_estimateTotalYieldRate(YIELD_COMMERCE); // cf. AI_yieldWeight
			// Note: it would probably be good to take into account the expected increase in unit spending - but that's a bit tricky.

			// sometimes are resources are more in demand than other times...
			int iPoolMultiplier = 0;
			for (int i = 0; i < MAX_PLAYERS; i++)
			{
				const CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
				if (kLoopPlayer.getTeam() == getID() && kLoopPlayer.isAlive())
				{
					iPoolMultiplier += 100;
					// increase value if we are still trying to expand peacefully. Now, the minimum site value is pretty arbitrary...
					int iSites = kLoopPlayer.AI_getNumPrimaryAreaCitySites(kLoopPlayer.AI_getMinFoundValue()*2); // note, there's a small cap on the number of sites, around 3.
					if (iSites > 0)
					{
						iPoolMultiplier += (50 + 50 * range(GC.getInfo(GC.getMap().getWorldSize()).getTargetNumCities() - kLoopPlayer.getNumCities(), 0, iSites))/(bTotalWar ? 2 : 1);
					}
				}
			}
			iPoolMultiplier /= std::max(1, getAliveCount());
			iCommitmentPool = iCommitmentPool * iPoolMultiplier / 100;

			// Don't pick a fight if we're expecting to beat them to a peaceful victory.
			if (!AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_DOMINATION4 | AI_VICTORY_CONQUEST4))
			{
				if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE4) ||
					(AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_SPACE4) && !GET_TEAM(eTarget).AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE4 | AI_VICTORY_SPACE4)) ||
					(AI_getLowestVictoryCountdown() > 0 && (GET_TEAM(eTarget).AI_getLowestVictoryCountdown() < 0 || AI_getLowestVictoryCountdown() < GET_TEAM(eTarget).AI_getLowestVictoryCountdown())))
				{
					iCommitmentPool *= 2;
				}
			}

			iTotalCost += iCommitmentPerMil * iCommitmentPool / 1000;
		}
	}

	// war weariness
	int iTotalWw = 0;
	for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
	{
		const CvTeamAI& kLoopTeam = GET_TEAM(i);
		if (kLoopTeam.isAlive() && (i == eTargetMaster || kLoopTeam.isVassal(eTargetMaster)  || (bPendingDoW && kLoopTeam.isDefensivePact(eTargetMaster))))
			iTotalWw += getWarWeariness(i, true)/100;
	}
	// note: getWarWeariness has units of anger per 100,000 population, and it is customary to divide it by 100 immediately
	if (iTotalWw > 50)
	{
		int iS = isAtWar(eTarget) ? -1 : 1;
		int iWwCost = 0;
		for (int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			const CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
			if (kLoopPlayer.getTeam() == getID() && kLoopPlayer.isAlive())
			{
				int iEstimatedPercentAnger = kLoopPlayer.getModifiedWarWearinessPercentAnger(iTotalWw) / 10; // (ugly, I know. But that's just how it's done.)
				// note. Unfortunately, we haven't taken the effect of jails into account.
				iWwCost += iS * kLoopPlayer.getNumCities() * kLoopPlayer.AI_getHappinessWeight(iS * iEstimatedPercentAnger * (100 + kLoopPlayer.getWarWearinessModifier())/100, 0, true) / 20;
			}
		}
		iTotalCost += iWwCost;
	}

	// Note: diplomacy cost is handled elsewhere

	return iTotalCost;
}

// diplomatic costs for declaring war (somewhat arbitrary - to encourage the AI to attack its enemies, and the enemies of its friends.)
int CvTeamAI::AI_warDiplomacyCost(TeamTypes eTarget) const
{
	if (isAtWar(eTarget))
	{
		//FAssertMsg(false, "AI_warDiplomacyCost called when already at war."); // sometimes we call this function for debug purposes.
		return 0;
	}

	if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST4))
		return 0;

	const CvTeamAI& kTargetTeam = GET_TEAM(eTarget);

	// first, the cost of upsetting the team we are declaring war on.
	int iDiploPopulation = kTargetTeam.getTotalPopulation(false);
	int iDiploCost = 3 * iDiploPopulation * (100 + AI_getAttitudeWeight(eTarget)) / 200;

	// cost of upsetting their friends
	for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
	{
		const CvTeamAI& kLoopTeam = GET_TEAM(i);

		if (!kLoopTeam.isAlive() || i == getID() || i == eTarget)
			continue;

		if (isHasMet(i) && kTargetTeam.isHasMet(i) && !kLoopTeam.isCapitulated())
		{
			int iPop = kLoopTeam.getTotalPopulation(false);
			//iDiploPopulation += iPop; // advc: commented out; value never read
			if (kLoopTeam.AI_getAttitude(eTarget) >= ATTITUDE_PLEASED && AI_getAttitude(i) >= ATTITUDE_PLEASED)
			{
				iDiploCost += iPop * (100 + AI_getAttitudeWeight(i)) / 200;
			}
			else if (kLoopTeam.isAtWar(eTarget))
			{
				iDiploCost -= iPop * (100 + AI_getAttitudeWeight(i)) / 400;
			}
		}
	}

	// scale the diplo cost based the personality of the team.
	{
		int iPeaceWeight = 0;
		for (int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			const CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
			if (kLoopPlayer.getTeam() == getID() && kLoopPlayer.isAlive())
			{
				iPeaceWeight += kLoopPlayer.AI_getPeaceWeight(); // usually between 0-10.
			}
		}

		int iDiploWeight = 40;
		iDiploWeight += 10 * iPeaceWeight / getAliveCount();
		// This puts iDiploWeight somewhere around 50 - 250.
		if (GC.getGame().isOption(GAMEOPTION_AGGRESSIVE_AI))
			iDiploWeight /= 2;
		if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_DIPLOMACY3))
			iDiploWeight += 50;
		if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_DIPLOMACY4))
			iDiploWeight += 50;
		if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST3)) // note: conquest4 ignores diplo completely.
			iDiploWeight /= 2;

		iDiploCost *= iDiploWeight;
		iDiploCost /= 100;
	}

	// Finally, reduce the value for large maps;
	// so that this diplomacy stuff doesn't become huge relative to other parts of the war evaluation.
	iDiploCost *= 3;
	iDiploCost /= std::max(5,
			GC.getGame().getRecommendedPlayers() // advc.137
			//GC.getInfo((WorldSizeTypes)GC.getMap().getWorldSize()).getDefaultPlayers()
		);

	return iDiploCost;
}
// K-Mod end.

/*  Relative value of starting a war against eTeam.
	This function computes the value of starting a war against eTeam.
	The returned value should be compared against other possible targets
	to pick the best target. */ // advc.104: UWAI bypasses this function
// K-Mod. Complete remake of the function.
int CvTeamAI::AI_startWarVal(TeamTypes eTarget, WarPlanTypes eWarPlan,
	bool bConstCache) const // advc.001n
{
	PROFILE_FUNC(); // advc.opt
	TeamTypes eTargetMaster = GET_TEAM(eTarget).getMasterTeam(); // we need this for checking defensive pacts.
	bool bPendingDoW = !isAtWar(eTarget) && eWarPlan != WARPLAN_ATTACKED && eWarPlan != WARPLAN_ATTACKED_RECENT;
	/*  advc.001n: These subroutines call the (cached) AI closeness funtions.
		Pass along bConstCache. */
	int iTotalValue = AI_warSpoilsValue(eTarget, eWarPlan, bConstCache) -
			AI_warCommitmentCost(eTarget, eWarPlan, bConstCache) -
			(bPendingDoW ? AI_warDiplomacyCost(eTarget) : 0);

	// Call AI_warSpoilsValue for each additional enemy team involved in this war.
	// NOTE: a single call to AI_warCommitmentCost should include the cost of fighting all of these enemies.
	for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
	{
		if (i == getID() || i == eTarget)
			continue;

		const CvTeam& kLoopTeam = GET_TEAM(i);

		if (!kLoopTeam.isAlive() || kLoopTeam.isVassal(getID()) || isAtWar(i))
			continue;

		if (kLoopTeam.isVassal(eTarget) || GET_TEAM(eTarget).isVassal(i))
		{
			iTotalValue += AI_warSpoilsValue(i, WARPLAN_DOGPILE,
					bConstCache) // advc.001n
					- (bPendingDoW ? AI_warDiplomacyCost(i) : 0);
		}
		else if (bPendingDoW && kLoopTeam.isDefensivePact(eTargetMaster))
		{
			FAssert(!isAtWar(eTarget));
			iTotalValue += AI_warSpoilsValue(i, WARPLAN_ATTACKED, // note: no diplo cost for this b/c it isn't us declaring war.
					bConstCache); // advc.001n
		}
	}
	return iTotalValue;
}
// K-Mod end

// XXX this should consider area power...
int CvTeamAI::AI_endWarVal(TeamTypes eTeam) const
{
	PROFILE_FUNC(); // advc.opt
	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(isAtWar(eTeam), "Current AI Team instance is expected to be at war with eTeam");

	const CvTeamAI& kWarTeam = GET_TEAM(eTeam); // K-Mod

	int iValue = 100;

	iValue += (getNumCities() * 3);
	iValue += (kWarTeam.getNumCities() * 3);

	iValue += getTotalPopulation();
	iValue += kWarTeam.getTotalPopulation();

	iValue += (kWarTeam.AI_getWarSuccess(getID()) * 20);

	int iOurPower = std::max(1, getPower(true));
	int iTheirPower = std::max(1, kWarTeam.getDefensivePower(getID()));

	iValue *= iTheirPower + 10;
	iValue /= std::max(1, iOurPower + iTheirPower + 10);

	WarPlanTypes eWarPlan = AI_getWarPlan(eTeam);

	// if we are not human, do we want to continue war for strategic reasons?
	// only check if our power is at least 120% of theirs
	if (!isHuman() && iOurPower > 120 * iTheirPower / 100)
	{
		bool bDagger = false;

		bool bAnyFinancialTrouble = false;
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			const CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)iI); // K-Mod
			if (kLoopPlayer.isAlive() && kLoopPlayer.getTeam() == getID())
			{
				if (kLoopPlayer.AI_isDoStrategy(AI_STRATEGY_DAGGER))
				{
					bDagger = true;
				}

				if (kLoopPlayer.AI_isFinancialTrouble())
				{
					bAnyFinancialTrouble = true;
				}
			}
		}

		// if dagger, value peace at 90% * power ratio
		if (bDagger)
		{
			iValue *= 9 * iTheirPower;
			iValue /= 10 * iOurPower;
		}

		// for now, we will always do the land mass check for domination
		// if we have more than half the land, then value peace at 90% * land ratio
		int iLandRatio = getTotalLand(true) * 100 / std::max(1, kWarTeam.getTotalLand(true));
		if (iLandRatio > 120)
		{
			iValue *= 9 * 100;
			iValue /= 10 * iLandRatio;
		}

		// if in financial trouble, warmongers will continue the fight to make more money
		if (bAnyFinancialTrouble)
		{
			switch (eWarPlan)
			{
				case WARPLAN_TOTAL:
					// if we total warmonger, value peace at 70% * power ratio factor
					if (bDagger || AI_maxWarRand() < 100)
					{
						iValue *= 7 * (5 * iTheirPower);
						iValue /= 10 * (iOurPower + (4 * iTheirPower));
					}
					break;

				case WARPLAN_LIMITED:
					// if we limited warmonger, value peace at 70% * power ratio factor
					if (AI_limitedWarRand() < 100)
					{
						iValue *= 7 * (5 * iTheirPower);
						iValue /= 10 * (iOurPower + (4 * iTheirPower));
					}
					break;

				case WARPLAN_DOGPILE:
					// if we dogpile warmonger, value peace at 70% * power ratio factor
					if (AI_dogpileWarRand() < 100)
					{
						iValue *= 7 * (5 * iTheirPower);
						iValue /= 10 * (iOurPower + (4 * iTheirPower));
					}
					break;

			}
		}
	}

	// BETTER_BTS_AI_MOD, War strategy AI, Victory Strategy AI, 05/19/10, jdog5000: START
	/* BBAI code
	if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE4))
		iValue *= 4;
	else if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE3) || AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_SPACE4))
		iValue *= 2;*/
	// K-Mod
	if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE4))
		iValue *= 3;
	else if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_SPACE4))
		iValue *= 2;
	else if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE3 | AI_VICTORY_SPACE3))
	{
		iValue *= 4;
		iValue /= 3;
	} // K-Mod end

	if ((!isHuman() && eWarPlan == WARPLAN_TOTAL) ||
		(!kWarTeam.isHuman() && kWarTeam.AI_getWarPlan(getID()) == WARPLAN_TOTAL))
	{
		iValue *= 2;
	}
	else if ((!isHuman() && eWarPlan == WARPLAN_DOGPILE && kWarTeam.getNumWars() > 1) ||
			(!kWarTeam.isHuman() && kWarTeam.AI_getWarPlan(getID()) == WARPLAN_DOGPILE && getNumWars() > 1))
	{
		iValue *= 3;
		iValue /= 2;
	}

	// Do we have a big stack en route?
	int iOurAttackers = 0;
	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
	{
		if (GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID())
		{
			iOurAttackers += GET_PLAYER((PlayerTypes)iPlayer).AI_enemyTargetMissions(eTeam);
		}
	}
	int iTheirAttackers = 0;
	CvArea* pLoopArea = NULL;
	FOR_EACH_AREA_VAR(pLoopArea)
		iTheirAttackers += countEnemyDangerByArea(pLoopArea, eTeam);

	int iAttackerRatio = (100 * iOurAttackers) / std::max(1 + GC.getGame().getCurrentEra(), iTheirAttackers);

	if (GC.getGame().isOption(GAMEOPTION_AGGRESSIVE_AI))
	{
		iValue *= 150;
		iValue /= range(iAttackerRatio, 150, 900);
	}
	else
	{
		iValue *= 200;
		iValue /= range(iAttackerRatio, 200, 600);
	}
	// BETTER_BTS_AI_MOD: END
	return AI_roundTradeVal(iValue); // advc.104k
}

// K-Mod. This is the tech value modifier for devaluing techs that are known by other civs
// It's based on the original bts code from AI_techTradVal
int CvTeamAI::AI_knownTechValModifier(TechTypes eTech) const
{
	int iTechCivs = 0;
	int iCivsMet = 0;

	for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
	{
		const CvTeam& kLoopTeam = GET_TEAM(i);
		if (i != getID() && kLoopTeam.isAlive() && isHasMet(i)
				&& !kLoopTeam.isCapitulated()) // advc.551
		{
			if (kLoopTeam.isHasTech(eTech))
				iTechCivs++;

			iCivsMet++;
		}
	}
	//return 50 * (iCivsMet - iTechCivs) / std::max(1, iCivsMet);
	// <advc.551> Replacing the above
	double const maxModifierPercent = 34;
	return::round((maxModifierPercent * (iCivsMet - iTechCivs)) /
			std::max(1, iCivsMet) - maxModifierPercent / 2); // </advc.551>
} // K-Mod end

// advc (comment): How much this CvTeam is willing to pay to eFromTeam for eTech
int CvTeamAI::AI_techTradeVal(TechTypes eTech, TeamTypes eFromTeam,
	bool bIgnoreDiscount, // advc.550a
	bool bPeaceDeal) const // advc.140h
{
	FAssert(eFromTeam != getID());
	// advc: Original BtS code deleted; K-Mod replaced it with AI_knownTechValModifier.
	CvTechInfo const& kTech = GC.getInfo(eTech);
	int iValue = (125 // advc.551: was 150
			// K-Mod. Standardized the modifier for # of teams with the tech; and removed the effect of team size.
			+ AI_knownTechValModifier(eTech)) *
			std::max(0, (getResearchCost(eTech, true, false) -
			getResearchProgress(eTech))) / 100;
			// K-Mod end
	/*  <advc.104h> Peace for tech isn't that attractive for the receiving side
		b/c they could continue the war and still get the tech when making peace
		later on. Doesn't work the same way with gold b/c the losing side may well
		spend the gold if the war continues */
	if(bPeaceDeal)
		iValue = ::round(0.7 * iValue); // </advc.104h>
	iValue *= std::max(0, (kTech.getAITradeModifier() + 100));
	iValue /= 100;
	// <advc.550a>
	// No discounts for vassals
	if(!bIgnoreDiscount && !isVassal(eFromTeam) && !GET_TEAM(eFromTeam).isVassal(getID()))
	{
		/*  If they're more advanced/powerful, they shouldn't mind giving us
			a tech, so we lower our tech value, assuming/hoping that we'll be
			able to get it cheap.
			Handle this here instead of CvPlayer::AI_tradeAcceptabilityThreshold
			b/c tech is shared rather than given away. Giving away e.g. gold is bad
			even if we don't think the other side is a threat. */
		float powerRatio = getPower(true) /
				((float)GET_TEAM(eFromTeam).getPower(true) + 1);
		/*  As far as I can tell, techs (and tech scores) are the same for all
			team members (strange that getTechScore is a CvPlayer member).
			Even if there is some exception, it's still better than CvTeam::
			getBestKnownTechScorePercent which is based on the teams that the
			callee has met. This team shouldn't be able to know the techs of teams
			that eTeam has met (unless this team has also met them). */
		float techRatio = GET_PLAYER(getLeaderID()).getTechScore() /
				((float)GET_PLAYER(GET_TEAM(eFromTeam).getLeaderID()).getTechScore() + 1);
		CvGame const& g = GC.getGame();
		float gameProgressFactor = (g.getGameTurn() - g.getStartTurn()) /
				((float)g.getEstimateEndTurn() - g.getStartTurn());
		gameProgressFactor = ::range(gameProgressFactor, 0.0f, 0.5f);
		powerRatio = ::range(powerRatio, 1 - gameProgressFactor, 1 + gameProgressFactor);
		techRatio = ::range(techRatio, 1 - gameProgressFactor, 1 + gameProgressFactor);
		// Powerful civs shouldn't grant large discounts to advanced civs
		if(techRatio < 1 && powerRatio > 1)
			powerRatio = std::max(1.f, powerRatio * techRatio * techRatio);
		else if(techRatio > 1 && powerRatio < 1)
			powerRatio = std::min(1.f, powerRatio * techRatio * techRatio);
		float modifier = (powerRatio + 2 * techRatio) / 3.f;
		/*  Not sure about this guard: Should weak civs charge more for their techs?
			In tech-vs-tech trades, the modifier would then take effect twice,
			resulting in one side demanding up to twice as much as the other;
			too much I think. */
		if(modifier < 1)
			iValue = ::round(iValue * modifier);
	} // </advc.550a>
	return AI_roundTradeVal(iValue); // advc.104k
}


DenialTypes CvTeamAI::AI_techTrade(TechTypes eTech, TeamTypes eToTeam) const  // advc: style changes
{
	PROFILE_FUNC();

	FAssertMsg(eToTeam != getID(), "shouldn't call this function on ourselves");
	// advc.550e: No longer needed
	//if (GC.getGame().isOption(GAMEOPTION_NO_TECH_BROKERING)) { ... }

	if(isHuman() || isVassal(eToTeam) || isAtWar(eToTeam))
		return NO_DENIAL; // advc

	if (AI_getWorstEnemy() == eToTeam)
		return DENIAL_WORST_ENEMY;

	AttitudeTypes eAttitude = AI_getAttitude(eToTeam);

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayerAI const& kOurMember = GET_PLAYER((PlayerTypes)i);
		if (!kOurMember.isAlive() || kOurMember.getTeam() != getID())
			continue;
		if (eAttitude <= GC.getInfo(kOurMember.getPersonalityType()).getTechRefuseAttitudeThreshold())
			return DENIAL_ATTITUDE;
	}

	if (eAttitude < ATTITUDE_FRIENDLY)
	{
		if ((GC.getGame().getTeamRank(getID()) < (GC.getGame().getCivTeamsEverAlive() / 2)) ||
			(GC.getGame().getTeamRank(eToTeam) < (GC.getGame().getCivTeamsEverAlive() / 2)))
		{
			int iNoTechTradeThreshold = AI_noTechTradeThreshold();

			iNoTechTradeThreshold *= std::max(0, (GC.getInfo(GET_TEAM(eToTeam).getHandicapType()).getNoTechTradeModifier() + 100));
			iNoTechTradeThreshold /= 100;

			if (AI_getMemoryCount(eToTeam, MEMORY_RECEIVED_TECH_FROM_ANY) > iNoTechTradeThreshold)
				return DENIAL_TECH_WHORE;
		}
	// K-Mod. Generic tech trade test
	}
	if (eTech == NO_TECH)
		return NO_DENIAL;

	if (eAttitude < ATTITUDE_FRIENDLY)
	{
	// K-Mod end
		int iKnownCount = 0;
		int iPossibleKnownCount = 0;
		int iNotMet = 0; // advc.550c

		for (int i = 0; i < MAX_CIV_TEAMS; i++)
		{
			CvTeam const& kOther = GET_TEAM((TeamTypes)i);
			if (!kOther.isAlive() || kOther.getID() == getID() || kOther.getID() == eToTeam)
				continue;
			if (isHasMet(kOther.getID()))
			{
				if (kOther.isHasTech(eTech))
					iKnownCount++;
				iPossibleKnownCount++;
			}
			else iNotMet++; // advc.550c
		}

		int iTechTradeKnownPercent = AI_techTradeKnownPercent();
		/*  <advc.550c>
			Don't want the threshold to change everytime this function is called.
			Therefore, can't use a random number. Just hashing eTech would lead
			to the same per-tech adjustment each game. Hash in the position of our
			leader's capital, which is different every game, but doesn't change
			from turn to turn. */
		// Between -15 and +15; put it in a variable only for debugging.
		int iRandomAdjustment = ::round(::hash(eTech, getLeaderID()) * 30 - 15);
		iTechTradeKnownPercent += iRandomAdjustment;
		// </advc.550c>

		iTechTradeKnownPercent *= std::max(0, (GC.getInfo(GET_TEAM(eToTeam).getHandicapType()).getTechTradeKnownModifier() + 100));
		iTechTradeKnownPercent /= 100;

		iTechTradeKnownPercent *= AI_getTechMonopolyValue(eTech, eToTeam);
		iTechTradeKnownPercent /= 100;

		// <advc.550c>
		int iKnownPercent = (iPossibleKnownCount == 0 ? 0 :
				(iKnownCount * 100) / iPossibleKnownCount);
		// No functional change so far
		/*  Make AI more willing to trade if it hasn't met most teams yet,
			especially if it has met just one or two. */
		if(iNotMet > iPossibleKnownCount)
		{
			iKnownPercent = std::max(iKnownPercent,
					/*  If just one team to trade with, that's 40%;
						two teams (neither knowing eTech): 28.6%. */
					::round(100 / (iPossibleKnownCount + 1.5)));
		}
		if(iKnownPercent < iTechTradeKnownPercent) // No functional change
		// </advc.550c>
		{
			return DENIAL_TECH_MONOPOLY;
		}
	}
	FOR_EACH_ENUM(Unit)
	{
		if (!isTechRequiredForUnit(eTech, eLoopUnit))
			continue;
		UnitClassTypes eLoopUnitClass = (UnitClassTypes)GC.getInfo(eLoopUnit).getUnitClassType();
		if (isWorldUnitClass(eLoopUnitClass) && getUnitClassMaking(eLoopUnitClass) > 0)
			return DENIAL_MYSTERY;
	}
	FOR_EACH_ENUM(Building)
	{
		if (!isTechRequiredForBuilding(eTech, eLoopBuilding))
			continue;
		BuildingClassTypes eLoopBuildingClass = (BuildingClassTypes)GC.getInfo(eLoopBuilding).getBuildingClassType();
		if (isWorldWonderClass(eLoopBuildingClass) && getBuildingClassMaking(eLoopBuildingClass) > 0)
			return DENIAL_MYSTERY;
	}
	FOR_EACH_ENUM(Project)
	{
		if (GC.getInfo(eLoopProject).getTechPrereq() != eTech)
			continue;
		if (isWorldProject(eLoopProject) && getProjectMaking(eLoopProject) > 0)
			return DENIAL_MYSTERY;
		FOR_EACH_ENUM(Victory)
		{
			if (GC.getGame().isVictoryValid(eLoopVictory) &&
					GC.getInfo(eLoopProject).getVictoryThreshold(eLoopVictory))
				return DENIAL_VICTORY;
		}
	}
	return NO_DENIAL;
}

// advc (comment): How much this CvTeam is willing to pay to eFromTeam for its map
int CvTeamAI::AI_mapTradeVal(TeamTypes eFromTeam) const
{
	FAssertMsg(eFromTeam != getID(), "shouldn't call this function on ourselves");

	int iValue = 0;
	CvMap const& kMap = GC.getMap();
	for (int iI = 0; iI < kMap.numPlots(); iI++)
	{
		CvPlot const& kPlot = kMap.getPlotByIndex(iI);
		if (!kPlot.isRevealed(getID(), false) && kPlot.isRevealed(eFromTeam))
		{
			if (kPlot.isWater())
				iValue++;
			else iValue += 5;
		}
	}

	iValue /= 10;
	// <advc.136a>
	if(AI_isPursuingCircumnavigation())
		iValue *= 2; // </advc.136a>

	if (GET_TEAM(eFromTeam).isVassal(getID()))
		iValue /= 2;

	return AI_roundTradeVal(iValue); // advc.104k
}


DenialTypes CvTeamAI::AI_mapTrade(TeamTypes eToTeam) const  // advc: style changes
{
	//PROFILE_FUNC(); // advc.003o
	FAssertMsg(eToTeam != getID(), "shouldn't call this function on ourselves");

	if (isHuman() || isVassal(eToTeam) || isAtWar(eToTeam))
		return NO_DENIAL;

	if (AI_getWorstEnemy() == eToTeam)
		return DENIAL_WORST_ENEMY;

	AttitudeTypes eOurAttitude = AI_getAttitude(eToTeam);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayerAI const& kOurMember = GET_PLAYER((PlayerTypes)i);
		if (!kOurMember.isAlive() || kOurMember.getTeam() != getID())
			continue;
		if (eOurAttitude <= GC.getInfo(kOurMember.getPersonalityType()).getMapRefuseAttitudeThreshold())
			return DENIAL_ATTITUDE;
	}
	// <advc.136a>
	if(AI_isPursuingCircumnavigation())
		return DENIAL_MYSTERY; // </advc.136a>

	return NO_DENIAL;
}


int CvTeamAI::AI_vassalTradeVal(TeamTypes eTeam) const
{
	return AI_surrenderTradeVal(eTeam);
}


DenialTypes CvTeamAI::AI_vassalTrade(TeamTypes eMasterTeam) const  // advc: some style changes
{
	FAssertMsg(eMasterTeam != getID(), "shouldn't call this function on ourselves");

	CvTeamAI& kMasterTeam = GET_TEAM(eMasterTeam);
	for (int i = 0; i < MAX_CIV_TEAMS; i++) // advc.003n: was MAX_TEAMS
	{
		TeamTypes eLoopTeam = (TeamTypes)i;
		CvTeam& kLoopTeam = GET_TEAM(eLoopTeam);
		if (kLoopTeam.isAlive() && eLoopTeam != getID() && eLoopTeam != eMasterTeam)
		{
			if (!kLoopTeam.isAtWar(eMasterTeam) && kLoopTeam.isAtWar(getID()))
			{
				if (kMasterTeam.isForcePeace(eLoopTeam) ||
					!kMasterTeam.canChangeWarPeace(eLoopTeam)
					// advc.112: Don't ask master who hasn't met vassal's war enemies
					|| !kMasterTeam.isHasMet(eLoopTeam))
				{
					if (!kLoopTeam.isAVassal())
					{
						return DENIAL_WAR_NOT_POSSIBLE_YOU;
					}
				}

				if (!kMasterTeam.isHuman() &&
					!getWPAI.isEnabled()) // advc.104o: Handled in the next loop
				{
					DenialTypes eWarDenial = kMasterTeam.AI_declareWarTrade(eLoopTeam, getID(), true);
					if (NO_DENIAL != eWarDenial)
					{
						return DENIAL_WAR_NOT_POSSIBLE_YOU;
					}
				}
			}
			else if (kLoopTeam.isAtWar(eMasterTeam) && !kLoopTeam.isAtWar(getID()))
			{
				if (!kMasterTeam.canChangeWarPeace(eLoopTeam))
				{
					if (!kLoopTeam.isAVassal())
					{
						return DENIAL_PEACE_NOT_POSSIBLE_YOU;
					}
				}

				if (!kMasterTeam.isHuman())
				{
					DenialTypes ePeaceDenial = kMasterTeam.AI_makePeaceTrade(eLoopTeam, getID());
					if (NO_DENIAL != ePeaceDenial)
					{
						return DENIAL_PEACE_NOT_POSSIBLE_YOU;
					}
				}
			}
		}
	}
	bool bMasterNeedsToDeclareWar = false; // advc.104o
	// K-Mod. some conditions moved from AI_surrenderTrade. (see the comments there)
	for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
	{
		const CvTeam& kLoopTeam = GET_TEAM(i);
		if (!kLoopTeam.isAlive() || (i == getID() && i == eMasterTeam)) // kmodx: Added parentheses
			continue;

		if (kMasterTeam.isAtWar(i) && !isAtWar(i))
		{
			if (isForcePeace(i) || !canChangeWarPeace(i))
			{
				if (!kLoopTeam.isAVassal())
				{
					return DENIAL_WAR_NOT_POSSIBLE_US;
				}
			}
		}
		else if (!kMasterTeam.isAtWar(i) && isAtWar(i))
		{
			// K-Mod. (peace-vassal deals cause the new master to declare war)
			if (!kLoopTeam.isAVassal())
			{	/*  advc (comment): This condition appears to be covered already
					by the previous loop (?) */
				if (kMasterTeam.isForcePeace(i) || !kMasterTeam.canChangeWarPeace(i))
				{
					return DENIAL_WAR_NOT_POSSIBLE_YOU;
				}
				// K-Mod. New rule: AI civs won't accept a vassal if it would mean joining a war they would never otherwise join.
				// Note: the following denials actually come from kMasterTeam, not this team. This is just the only way to do it.
				if (kMasterTeam.AI_refuseWar(i))
					return DENIAL_ATTITUDE_THEM;
				bMasterNeedsToDeclareWar = true; // advc.104o
				// (if we're concerned about AI_startWarVal, then that should be checked in the trade value part; not the trade denial part.)
			}
			//
		}
	} // K-Mod end
	// <advc.143>
	if(!isAtWar(eMasterTeam) &&
			(AI_getMemoryCount(eMasterTeam, MEMORY_CANCELLED_VASSAL_AGREEMENT) > 0 ||
			kMasterTeam.AI_getMemoryCount(getID(),
			MEMORY_CANCELLED_VASSAL_AGREEMENT) > 0))
		return DENIAL_RECENT_CANCEL;
	// </advc.143>
	// <advc.112> Master refuses if vassal too insignificant
	if(!isAtWar(eMasterTeam) && !isVassal(eMasterTeam) && !kMasterTeam.isHuman() &&
		!kMasterTeam.AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST2) &&
		kMasterTeam.getTotalPopulation() > 6 * getTotalPopulation())
	{
		int iAttitudeThresh = ATTITUDE_FRIENDLY;
		/*  Those that don't start wars when Pleased are "nice" civs that will
			protect a small vassal at Pleased */
		if(GC.getInfo(GET_PLAYER(kMasterTeam.getLeaderID()).
				getPersonalityType()).getNoWarAttitudeProb(ATTITUDE_PLEASED) >= 100)
			iAttitudeThresh = ATTITUDE_PLEASED;
		if(kMasterTeam.AI_getAttitude(getID()) < iAttitudeThresh)
			return DENIAL_POWER_YOU;
	} // </advc.112>
	// return AI_surrenderTrade(eTeam); // (BtS code)
	/*  <advc.104o> Do the UWAI stuff after AI_surrenderTrade in order to avoid
		uninteresting log output. */
	DenialTypes eDenial = AI_surrenderTrade(eMasterTeam);
	if(eDenial != NO_DENIAL)
		return eDenial;
	/*  Regarding the K-Mod comment above ("if we're concerned ..."):
		I'm indeed checking if the triggered wars are worth it. Could do this
		in AI_surrenderTradeVal I guess. I know it'll work if I do it here ... */
	if(getWPAI.isEnabled() && bMasterNeedsToDeclareWar && !kMasterTeam.isHuman())
		return kMasterTeam.warAndPeaceAI().acceptVassal(getID());
	return NO_DENIAL; // </advc.104o>
}


int CvTeamAI::AI_surrenderTradeVal(TeamTypes eTeam) const
{
	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	return 0;
}


DenialTypes CvTeamAI::AI_surrenderTrade(TeamTypes eMasterTeam, int iPowerMultiplier,
	bool bCheckAccept) const // advc.104o
{
	PROFILE_FUNC();

	FAssertMsg(eMasterTeam != getID(), "shouldn't call this function on ourselves");

	CvTeamAI& kMasterTeam = GET_TEAM(eMasterTeam);
	bool const bWar = isAtWar(eMasterTeam);
	/*  advc.112: Colonies are more loyal than other peace vassals, but not more
		likely to come back once broken away (isVassal is checked). */
	bool bColony = isVassal(eMasterTeam) && !isCapitulated() && kMasterTeam.isParent(getID());

	// K-Mod. I've disabled the denial for "war not possible"
	// In K-Mod, surrendering overrules existing peace treaties - just like defensive pacts overrule peace treaties.
	// However, for voluntary vassals, the treaties still apply. So I've moved the code to there.
	/* original bts code
	for (int iLoopTeam = 0; iLoopTeam < MAX_TEAMS; iLoopTeam++)
	{	...			{
						return DENIAL_WAR_NOT_POSSIBLE_US;
					}
					...
					{
						return DENIAL_PEACE_NOT_POSSIBLE_US;
					} ...} */
	// K-Mod. However, "peace not possible" should still be checked here --  but only if this is a not a peace-vassal deal!
	if (bWar)
	{
		for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
		{
			const CvTeam& kLoopTeam = GET_TEAM(i);
			if (!kLoopTeam.isAlive() || /* kmodx: */ (i == getID() && i == eMasterTeam))
				continue;

			if (isAtWar(i) && !kMasterTeam.isAtWar(i))
			{
				if (!canChangeWarPeace(i))
				{
					if (!kLoopTeam.isAVassal())
						return DENIAL_PEACE_NOT_POSSIBLE_US;
				}
			}
		}
	} // K-Mod end

	if (isHuman() && kMasterTeam.isHuman())
		return NO_DENIAL;
	// <advc.112>
	if(AI_isAnyMemberDoVictoryStrategyLevel3())
		return DENIAL_VICTORY;
	for(int i = 0; i < GC.getNumVoteSourceInfos(); i++)
	{
		VoteSourceTypes vs = (VoteSourceTypes)i;
		// Doesn't imply stage 3 of diplo victory
		if(GC.getGame().getSecretaryGeneral(vs) == getID())
			return DENIAL_VICTORY;
	} // <advc>
	CvGame const& g = GC.getGame();
	CvMap& m = GC.getMap();
	AttitudeTypes eTowardMaster = AI_getAttitude(eMasterTeam, false); // </advc>
	if(isVassal(eMasterTeam) && eTowardMaster >= ATTITUDE_PLEASED)
	{
		// Moved up // </advc.112>
		for(int i = 0; i < GC.getNumVictoryInfos(); i++) // advc: Refactored
		{
			bool bPopulationThreat = true;
			VictoryTypes eVict = (VictoryTypes)i;
			if(g.getAdjustedPopulationPercent(eVict) > 0)
			{
				bPopulationThreat = false;
				int iThreshold = g.getTotalPopulation() *
						g.getAdjustedPopulationPercent(eVict);
				// advc.112: The DENIAL_VICTORY check above should cover this
				/*if(400 * getTotalPopulation(!isAVassal()) > 3 * iThreshold)
					return DENIAL_VICTORY;*/
				if(400 * (getTotalPopulation() +kMasterTeam.getTotalPopulation()) > 3 * iThreshold)
					bPopulationThreat = true;
			}
			bool bLandThreat = true;
			if(g.getAdjustedLandPercent(eVict) > 0)
			{
				bLandThreat = false;
				int iThreshold = m.getLandPlots() * g.getAdjustedLandPercent(eVict);
				/*if(400 * getTotalLand(!isAVassal()) > 3 * iThreshold)
					return DENIAL_VICTORY;*/ // advc.112
				if(400 * (getTotalLand() + kMasterTeam.getTotalLand()) > 3 * iThreshold)
					bLandThreat = true;
			}
			if(bLandThreat && bPopulationThreat &&
					(g.getAdjustedPopulationPercent(eVict) > 0 ||
					g.getAdjustedLandPercent(eVict) > 0))
			{
				//return DENIAL_POWER_YOU;
				/*  advc.112: On the contrary, when the master is close (75%) to
					a military victory, we're not breaking away. */
				return NO_DENIAL;
			}
		}
	}

	bool bFaraway = false;
	/*  Moved this block up b/c these checks are cheap, and the primary-area
		condition is something the player can't easily change, so the AI should
		mention it right away instead of suggesting that a change in power ratios
		or diplo could lead to a vassal agreement. */
	if (!bWar)
	{	// advc.112: Removed !isParent check
		if(AI_getWorstEnemy() == eMasterTeam)
			return DENIAL_WORST_ENEMY;
		// <advc.112> This used to be checked later
		if(eTowardMaster <= ATTITUDE_FURIOUS)
			return DENIAL_ATTITUDE; // </advc.112>
		if (!bColony && // advc.112
			!AI_hasCitiesInPrimaryArea(eMasterTeam) &&
			AI_calculateAdjacentLandPlots(eMasterTeam) == 0)
		{
			// <advc.112>
			bFaraway = true;
			if(kMasterTeam.getCurrentEra() < 4) // </advc.112>
				return DENIAL_TOO_FAR;
		}
	}
	// <advc.112>
	/*  NB: When this team is human (i.e. BBAI_HUMAN_AS_VASSAL_OPTION enabled
		-- which is not currently supported by AdvCiv/K-Mod), then
		DENIAL_POWER_US and DENIAL_POWER_YOU seem to become swapped outside the SDK. */
	// Don't vassal while we still have plans to expand
	if(!isAVassal() && AI_isSneakAttackPreparing())
		return DENIAL_POWER_US;
	//int iAttitudeModifier = 0; // Computation rewritten further down this function
	// !isParent check removed
	// Computation of iPersonalityModifier moved down
	// Commented-out BBAI code (06/03/09, jdog) deleted
	if(getNumWars() <= 0 && getDefensivePactCount() > 0 && !isAVassal())
	{
		// "We" (= this team and its partners) "are doing fine on our own"
		return DENIAL_POWER_US;
	} // </advc.112>  <advc.143b>
	if(!bWar && getNumNukeUnits() > 0 &&
			getNukeInterception() >= kMasterTeam.getNukeInterception())
		return DENIAL_POWER_US;
	// </advc.143b>
	int iTotalPower = 0;
	int iNumNonVassals = 0;
	std::vector<double> powerValues; // advc.112
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		CvTeam& kTeam = GET_TEAM((TeamTypes) iI);
		if (kTeam.isAlive() && !(kTeam.isMinorCiv()))
		{
			// advc.112: Put this in a variable
			int pow = kTeam.getPower(false);
			if (kTeam.isCapitulated())
			{
				// Count capitulated vassals as a fractional add to their master's power
				iTotalPower += (2*pow)/5;
			}
			else
			{
				// <advc.112> For median computation
				powerValues.push_back(pow);
				iTotalPower += pow;
				// </advc.112>
				iNumNonVassals++;
			}
		}
	}
	// advc.112: Probably not much of an improvement over using the mean
	double medianPow = ::dMedian(powerValues);
	int iAveragePower = iTotalPower / std::max(1, iNumNonVassals);
	int iMasterPower = kMasterTeam.getPower(false);
	// <advc.112>
	if(bFaraway || (getNumWars() <= 0 && AI_teamCloseness(eMasterTeam, -1, false, true) <= 0))
		iMasterPower = ::round(iMasterPower  * 0.7); // </advc.112>
	int iOurPower = getPower(true); // K-Mod (this value is used a bunch of times separately)
	/*  <advc.143> Reluctant to sign voluntary vassal agreement if we recently
		canceled one */
	if(!bWar && !isVassal(eMasterTeam))
	{
		CvPlayerAI const& kOurLeader = GET_PLAYER(getLeaderID());
		int iCancelMem = 0;
		for(int j = 0; j < MAX_CIV_PLAYERS; j++)
		{
			CvPlayerAI const& kFormerMaster = GET_PLAYER((PlayerTypes)j);
			if(kFormerMaster.isAlive() && kFormerMaster.getTeam() != eMasterTeam &&
				/*  If we have memory, then we canceled due to lack of protection;
					doesn't count here. */
				kOurLeader.AI_getMemoryCount(kFormerMaster.getID(),
				MEMORY_CANCELLED_VASSAL_AGREEMENT) <= 0)
			{
				iCancelMem = std::max(iCancelMem, kFormerMaster.AI_getMemoryCount(
						kOurLeader.getID(), MEMORY_CANCELLED_VASSAL_AGREEMENT));
			}
		}
		/*  iCancelMem is normally no more than 4, which results in the same
			iPowerMultiplier as for capitulation. */
		iPowerMultiplier += 10 * iCancelMem;
	} // </advc.143>
	/*  <advc.112> Once signed, show some constancy (previously handled by
		CvPlayerAI::AI_considerOffer) */
	if(isAVassal() && !isCapitulated())
	{
		// Obscured; don't want player to aim for a very specific power ratio
		std::vector<long> hashInput;
		hashInput.push_back(g.getTeamRank(getID()));
		hashInput.push_back(g.getTeamRank(eMasterTeam));
		hashInput.push_back(getNumWars());
		double h = ::hash(hashInput, getLeaderID());
		iPowerMultiplier -= ::round(10 + h * 30);
	}
	// iPersonalityModifier: moved here from farther up
	// </advc.112>
	int iPersonalityModifier = 0;
	int iMembers = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iPersonalityModifier += GC.getInfo(GET_PLAYER((PlayerTypes)iI).
						getPersonalityType()).getVassalPowerModifier();
				++iMembers;
			}
		}
	}
	int iVassalPower = (iOurPower * (iPowerMultiplier + iPersonalityModifier /
			std::max(1, iMembers))) / 100;

	if (bWar)
	{
		int iTheirSuccess = std::max(10, kMasterTeam.AI_getWarSuccess(getID()));
		int iOurSuccess = std::max(10, AI_getWarSuccess(eMasterTeam));
		int iOthersBestSuccess = 0;
		for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
		{
			if (iTeam != eMasterTeam && iTeam != getID())
			{
				CvTeamAI const& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
				/*  <advc.112> War success of a vassal shouldn't spoil its
					master's chances of winning another vassal. */
				if(kLoopTeam.isVassal(eMasterTeam))
					continue; // </advc.112>

				if (kLoopTeam.isAlive() && kLoopTeam.isAtWar(getID()))
				{
					int iSuccess = kLoopTeam.AI_getWarSuccess(getID());
					if (iSuccess > iOthersBestSuccess)
						iOthersBestSuccess = iSuccess;
				}
			}
		}

		// Discourage capitulation to a team that has not done the most damage
		if (iTheirSuccess < iOthersBestSuccess)
			iOurSuccess += iOthersBestSuccess - iTheirSuccess;
		// <advc.112> Instead multiply VassalPower by the inverse factor
		//iMasterPower = (2 * iMasterPower * iTheirSuccess) / (iTheirSuccess + iOurSuccess);
		iVassalPower = ::round(iVassalPower * (iTheirSuccess + iOurSuccess) /
				(1.8 * iTheirSuccess)); // Slightly reduce the coefficient
		// FURIOUS clause added; WorstEnemy doesn't say much when at war.
		if (AI_getWorstEnemy() == eMasterTeam && eTowardMaster <= ATTITUDE_FURIOUS)
		{	// was 75%, now 90%.
			iMasterPower *= 9;
			iMasterPower /= 10; // </advc.112>
		}
	}
	else if (!kMasterTeam.AI_isLandTarget(getID()))
		iMasterPower /= 2;

	// K-Mod. (condition moved here from lower down; for efficiency.)
	// <advc.112> Special treatment of vassal-master power ratio if colony
	if((!bColony && 3 * iVassalPower > 2 * iMasterPower) ||
			(bColony && 5 * getPower(true) > 4 * iMasterPower)) // </advc.112>
		return DENIAL_POWER_US;
	// K-Mod end
	// <advc.112b> Don't surrender if there isn't an acute threat
	if(bWar)
	{
		int iNukes = 0;
		int iSafePopulation = 0;
		for(int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			CvPlayerAI const& kMember = GET_PLAYER((PlayerTypes)i);
			if(!kMember.isEverAlive() || kMember.getTeam() != getID())
				continue;
			for(int j = 0; j < MAX_CIV_PLAYERS; j++)
			{
				CvPlayerAI const& kEnemy = GET_PLAYER((PlayerTypes)j);
				if(!kEnemy.isAlive() || kEnemy.getTeam() != eMasterTeam)
					continue;
				iNukes += kMember.AI_getMemoryCount(kEnemy.getID(), MEMORY_NUKED_US);
			}
		}
		if(iNukes == 0)
		{
			// Based on code in AI_endWarVal:
			int iTheirAttackers = 0;
			FOR_EACH_AREA_VAR(pLoopArea)
			{
				int iAreaCities = countNumCitiesByArea(pLoopArea);
				if(iAreaCities <= 0)
					continue;
				int iAreaDanger = countEnemyDangerByArea(pLoopArea, eMasterTeam);
				int iAreaPop = countTotalPopulationByArea(pLoopArea);
				if(iAreaDanger < iAreaPop / 3)
					iSafePopulation += iAreaPop;
				if(iAreaCities > getNumCities() / 3)
					iTheirAttackers += iAreaDanger;
			}
			/*  Randomly between these two bounds; randomness from a hash of the
				turn number */
			double bound1 = (0.5 * getCurrentEra() + 1.5) * 0.75 * getNumCities();
			double bound2 = (0.5 * getCurrentEra() + 1.5) * getNumCities();
			if(iTheirAttackers < bound1 + ::hash(g.getGameTurn()) * (bound2 - bound1))
				return DENIAL_NEVER;
			if(iSafePopulation / (getTotalPopulation() + 0.1) > 0.3)
				return DENIAL_NEVER;
		}
	} // </advc.112b>
	for (int iLoopTeam = 0; iLoopTeam < MAX_CIV_TEAMS; iLoopTeam++)
	{
		CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iLoopTeam);
		if (!kLoopTeam.isAlive() || iLoopTeam == getID())
			continue; // advc
		if (kLoopTeam.AI_isLandTarget(getID()))
		{
			if (iLoopTeam == eMasterTeam)
				continue;
			int iLoopPower = kLoopTeam.getPower(true); // K-Mod
			if (iLoopPower > iOurPower)
			{
				if (kLoopTeam.isAtWar(eMasterTeam) && !kLoopTeam.isAtWar(getID())
						&& (!bWar || iMasterPower < 2 * iLoopPower)) // K-Mod
					return DENIAL_POWER_YOUR_ENEMIES;
				iAveragePower = (2 * iAveragePower * iLoopPower) / std::max(1, iLoopPower + iOurPower);
				// advc.112: The same threat adjustment for the median
				medianPow = (2 * medianPow * iLoopPower) / std::max(1, iLoopPower + iOurPower);

				//iAttitudeModifier += (3 * kLoopTeam.getPower(true)) / std::max(1, getPower(true)) - 2;
				/*  advc.112: Commented out K-Mod's replacement as well.
					Attitude is handled later. */
				//iAttitudeModifier += (6 * iLoopPower / std::max(1, iOurPower) - 5)/2; // K-Mod. (effectively -2.5 instead of 2)
			}
			if (!kLoopTeam.isAtWar(eMasterTeam) && kLoopTeam.isAtWar(getID()))
			{
				iAveragePower = iAveragePower *
						(iOurPower + iMasterPower) / std::max(1, iOurPower + std::max(iOurPower, iLoopPower)); // K-Mod
						// advc.112:
					medianPow = medianPow * (iOurPower + iMasterPower) / std::max(1, iOurPower + std::max(iOurPower, iLoopPower));
			}
		}
		if (!bWar)
		{
			if (kLoopTeam.isAtWar(eMasterTeam) && !kLoopTeam.isAtWar(getID()))
			{
				DenialTypes eDenial = AI_declareWarTrade((TeamTypes)iLoopTeam, eMasterTeam, false);
				if (eDenial != NO_DENIAL)
					return eDenial;
			}
		}
	}

	if (!isVassal(eMasterTeam) && canVassalRevolt(eMasterTeam))
		return DENIAL_POWER_US;

	if(!bColony) // advc.112
	{
		// if (iVassalPower > iAveragePower || 3 * iVassalPower > 2 * iMasterPower)
		// advc.112: Changed coefficients from 5/4 to 1/0.76
		if (iVassalPower > 0.76*iAveragePower // K-Mod. (second condition already checked)
			// <advc.112> Median condition; randomization when breaking free
			|| (!bWar && iVassalPower > 0.76 * medianPow))
		{
			if(!isAVassal() || ::hash(g.getGameTurn(),
					getLeaderID()) < 0.1) // </advc.112>
				return DENIAL_POWER_US;
		}
	}

	if (!bWar)
	{	// <advc.112> (code block about DENIAL_TOO_FAR moved up)
		/*  Calculation rewritten with a different goal in mind:
			Prospective vassal evaluates prospective master based on ow threatened
			the vassal feels. (Originally more based on what chances the vassal
			still has to win the game). */
		int iAttitudeModifier = -2;
		int losingWars = 0;
		for(int i = 0; i < MAX_CIV_TEAMS; i++)
		{
			CvTeamAI const& kThreat = GET_TEAM((TeamTypes)i);
			if(kThreat.getID() == getID() || !kThreat.isAlive() || kThreat.isAVassal() ||
				kThreat.isMinorCiv() || !isHasMet(kThreat.getID()) ||
				!kThreat.AI_isLandTarget(getID()) ||
				6 * getPower(true) > (isAVassal() ? 5 : 4) * kThreat.getPower(true))
			{
				continue;
			}
			// Immediate threat from ongoing wars
			WarPlanTypes eWarPlan = AI_getWarPlan(kThreat.getID());
			if(kThreat.getID() != eMasterTeam && (eWarPlan == WARPLAN_ATTACKED_RECENT ||
					eWarPlan == WARPLAN_ATTACKED || AI_getWarSuccessRating() <= -30))
			{
				losingWars++;
				continue;
			}
			// Threat from future wars. kMasterTeam can contribute to this.
			AttitudeTypes eTowardUs = NO_ATTITUDE;
			if(kThreat.isHuman()) // Assume that human likes or hates us back
			{
				eTowardUs = AI_getAttitude(kThreat.getID(), false);
				iAttitudeModifier++; // They're always a threat though
			}
			else
			{
				eTowardUs = kThreat.AI_getAttitude(getID(), false);
				/*  Potentially dangerous AI team pleased w/ us or currently busy
					harassing someone else => probably not an immediate threat */
				if(eTowardUs >= ATTITUDE_PLEASED || kThreat.getNumWars() > 0)
					continue;
			}
			iAttitudeModifier += std::max(0, (int)(ATTITUDE_PLEASED - eTowardUs));
		}
		/*  In large games, there tend to be alternative targets for dangerous civs
			to attack. */
		iAttitudeModifier = (7 * iAttitudeModifier) / g.countCivPlayersAlive();
		if(losingWars > 0)
			iAttitudeModifier += 4;
		// No matter how much we like kMasterTeam, when we can safely go it alone, we do.
		// Master might not like it (and need sth. to prevent oscillation)
		if(isAVassal()) // (should perhaps check master's NoWarProb at Pleased)
			iAttitudeModifier += 3;
		if(iAttitudeModifier <= 2 &&
			4 * iVassalPower > 3 * iAveragePower &&
			4 * iVassalPower > 3 * medianPow &&
			// Exception: stick to our colonial master
			(!isAVassal() || !kMasterTeam.isParent(getID())))
		{
			return DENIAL_POWER_US;
		}
		// Moved this line down:
		//AttitudeTypes eModifiedAttitude = CvPlayerAI::AI_getAttitudeFromValue(AI_getAttitudeVal(eTeam, false) + iAttitudeModifier);
		// </advc.112>

		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			CvPlayerAI const& kOurMember = GET_PLAYER((PlayerTypes)iI);
			if(!kOurMember.isAlive() || kOurMember.getTeam() != getID())
				continue; // advc
			// <advc.112> Handled higher up
			int iAttitudeThresh = GC.getInfo(kOurMember.getPersonalityType()).
					getVassalRefuseAttitudeThreshold();
			/*  Don't use Annoyed thresh from XML, only increase the
				relations modifier by 1. */
			if(iAttitudeThresh < ATTITUDE_CAUTIOUS)
			{
				iAttitudeModifier += (ATTITUDE_CAUTIOUS - iAttitudeThresh);
				iAttitudeThresh = ATTITUDE_CAUTIOUS;
			}
			AttitudeTypes eModifiedAttitude = CvPlayerAI::AI_getAttitudeFromValue(
					AI_getAttitudeVal(eMasterTeam, false) + iAttitudeModifier);
			if(eModifiedAttitude <= iAttitudeThresh) // </advc.112>
				return DENIAL_ATTITUDE;
		}
	}
	else
	{
		/* original BTS code
		if (AI_getWarSuccess(eTeam) + 4 * GC.getDefineINT("WAR_SUCCESS_CITY_CAPTURING") > GET_TEAM(eTeam).AI_getWarSuccess(getID()))
			return DENIAL_JOKING;*/
		/*  BETTER_BTS_AI_MOD, Diplomacy AI, 12/07/09, jdog5000: START
			Scale better for small empires, particularly necessary if WAR_SUCCESS_CITY_CAPTURING > 10 */
		// <advc> For debugger:
		int iVassalSuccess = AI_getWarSuccess(eMasterTeam);
		int iMasterSuccess = kMasterTeam.AI_getWarSuccess(getID());
		int iTargetDelta = std::min(getNumCities(), 4) *
				GC.getWAR_SUCCESS_CITY_CAPTURING(); // </advc>
		// <advc.104o> Factor in past wars
		if(getWPAI.isEnabled())
		{
			int iPastWarScore = GET_PLAYER(getLeaderID()).warAndPeaceAI().getCache().
					pastWarScore(eMasterTeam);
			if(iPastWarScore < 0)
				iTargetDelta = ::round(iTargetDelta * 2 / 3.0);
		} // </advc.104o>
		if (iVassalSuccess + iTargetDelta > iMasterSuccess)
			return DENIAL_POWER_US;
		if(!kMasterTeam.isHuman())
		{
			if(!bCheckAccept || // advc.104o
					!kMasterTeam.AI_acceptSurrender(getID()))
				return DENIAL_JOKING;
		}
		// BETTER_BTS_AI_MOD: END
		/*  <advc.112> Based on code in endWarVal. Don't capitulate during
			counteroffensive. AI_enemyTargetMissions is kind of costly, therefore
			check this last of all.*/
		int iOurMissions = 0;
		for(int i = 0; i < MAX_CIV_PLAYERS; i++)
		{
			CvPlayerAI const& kOurMember = GET_PLAYER((PlayerTypes)i);
			if(kOurMember.isAlive() && kOurMember.getTeam() == getID())
				iOurMissions += kOurMember.AI_enemyTargetMissions(eMasterTeam);
		}
		if(iOurMissions > kMasterTeam.getNumCities())
			return DENIAL_JOKING; // </advc.112>
		/*  <advc.104o> Make sure that WarAndPeaceAI::Team::considerPeace has been
			called before surrendering. */
		if(getWPAI.isEnabled() && kMasterTeam.isHuman() &&
				!warAndPeaceAI().leaderWpai().getCache().isReadyToCapitulate(eMasterTeam))
			return DENIAL_RECENT_CANCEL; // </advc.104o>
	}
	// <advc.143>
	if(!isVassal(eMasterTeam) || isCapitulated())
		return NO_DENIAL;
	/*  When there is already a voluntary vassal agreement, then we're deciding
		whether to cancel the agreement. (VVA is normally handled by AI_vassalTrade,
		which then calls AI_surrenderTrade. However, when it comes to canceling trades,
		AI_surrenderTrade is called directly, so cancellation has to be handled here.)
		Cancel if we have lost much territory. BtS has similar code for
		capitulated vassals in CvTeam::canVassalRevolt. Tiles lost b/c of culture
		can also trigger cancellation; I guess that's OK (and can't be helped). */
	// VassalPower is the land at the time of signing the vassal agreement
	// <advc.112> Lower bound: 10
	double landRatio = std::max(10, getTotalLand(false)) / (double)
			std::max(10, getVassalPower()); // </advc.112>
	static double const lossesThresh = GC.getDefineINT("VASSAL_DENY_OWN_LOSSES_FACTOR") / 100.0;
	if(landRatio < 0.85 * lossesThresh || (landRatio < lossesThresh &&
			::hash(g.getGameTurn(), getLeaderID()) < 0.15))
		return DENIAL_POWER_YOUR_ENEMIES; // Denial type doesn't matter
	// </advc.143> <advc.143b>
	double nuked = 0;
	for(int i = 0; i < MAX_CIV_TEAMS; i++)
	{
		CvTeam const& kEnemy = GET_TEAM((TeamTypes)i);
		if(!kEnemy.isAlive() || !isAtWar(kEnemy.getID()) ||
				kEnemy.getCurrentEra() < 5) // for performance
			continue;
		// advc.130q: The average nuke adds 2 to memory
		nuked += 0.5 * AI_getMemoryCount(kEnemy.getID(), MEMORY_NUKED_US);
	}
	int iCities = getNumCities();
	double nukeThresh = std::max(2.0,
			(0.4 + 0.6 * ::hash(eMasterTeam, getLeaderID())) * iCities);
	if(nuked > nukeThresh && kMasterTeam.getNukeInterception() <= getNukeInterception())
		return DENIAL_POWER_YOUR_ENEMIES;
	// </advc.143b>
	return NO_DENIAL;
}

// K-Mod
int CvTeamAI::AI_countMembersWithStrategy(int iStrategy) const
{
	int iCount = 0;
	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
	{
		if (GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID())
		{
			if (GET_PLAYER((PlayerTypes)iPlayer).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iPlayer).AI_isDoStrategy(iStrategy))
					iCount++;
			}
		}
	}
	return iCount;
} // K-Mod end

// BETTER_BTS_AI_MOD, Victory Strategy AI, 03/20/10, jdog5000: START
bool CvTeamAI::AI_isAnyMemberDoVictoryStrategy(int iVictoryStrategy) const
{
	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
	{
		if (GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID())
		{
			if (GET_PLAYER((PlayerTypes)iPlayer).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iPlayer).AI_isDoVictoryStrategy(iVictoryStrategy))
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool CvTeamAI::AI_isAnyMemberDoVictoryStrategyLevel4() const
{
	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
	{
		if (GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID())
		{
			if (GET_PLAYER((PlayerTypes)iPlayer).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iPlayer).AI_isDoVictoryStrategyLevel4())
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool CvTeamAI::AI_isAnyMemberDoVictoryStrategyLevel3() const
{
	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
	{
		if (GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID())
		{
			if (GET_PLAYER((PlayerTypes)iPlayer).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iPlayer).AI_isDoVictoryStrategyLevel3())
				{
					return true;
				}
			}
		}
	}

	return false;
}
// BETTER_BTS_AI_MOD: END

// K-Mod. return a rating of our war success between -99 and 99.
// -99 means we losing and have very little hope of surviving. 99 means we are soundly defeating our enemies. Zero is neutral (eg. no wars being fought).
// (Based on K-Mod code for Force Peace diplomacy voting.)
int CvTeamAI::AI_getWarSuccessRating() const
{
	PROFILE_FUNC();

	int iMilitaryUnits = 0;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		const CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.getTeam() == getID())
		{
			iMilitaryUnits += kLoopPlayer.getNumMilitaryUnits();
		}
	}
	int iSuccessScale = iMilitaryUnits * GC.getDefineINT(CvGlobals::WAR_SUCCESS_ATTACKING) / 5;

	int iThisTeamPower = getPower(true);
	int iScore = 0;

	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		CvTeamAI const& kLoopTeam = GET_TEAM((TeamTypes)iI);
		if (iI != getID() && isAtWar((TeamTypes)iI) && kLoopTeam.isAlive() && !kLoopTeam.isAVassal())
		{
			int iThisTeamSuccess = AI_getWarSuccess((TeamTypes)iI);
			int iOtherTeamSuccess = kLoopTeam.AI_getWarSuccess(getID());

			int iOtherTeamPower = kLoopTeam.getPower(true);

			iScore += (iThisTeamSuccess+iSuccessScale) * iThisTeamPower;
			iScore -= (iOtherTeamSuccess+iSuccessScale) * iOtherTeamPower;
		}
	}
	iScore = range((100*iScore)/std::max(1, iThisTeamPower*iSuccessScale*5), -99, 99);
	return iScore;
}
// K-Mod end

/*  BETTER_BTS_AI_MOD, War Strategy AI, 03/20/10, jdog5000: START
	Compute power of enemies as percentage of our power */
int CvTeamAI::AI_getEnemyPowerPercent(bool bConsiderOthers) const  // advc: style changes
{
	int iEnemyPower = 0;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if(iI == getID())
			continue;
		CvTeamAI const& kEnemy = GET_TEAM((TeamTypes)iI);
		if(!kEnemy.isAlive() || !kEnemy.isHasMet(getID()))
			continue;
		if(isAtWar(kEnemy.getID()))
		{
			int iTempPower = 220 * kEnemy.getPower(false);
			iTempPower /= (AI_hasCitiesInPrimaryArea(kEnemy.getID()) ? 2 : 3);
			iTempPower /= (kEnemy.isMinorCiv() ? 3 : 1);
			iTempPower /= std::max(1, (bConsiderOthers ?
					kEnemy.getNumWars(true, true) : 1));
			iEnemyPower += iTempPower;
		}
		else if(AI_isChosenWar(kEnemy.getID()) && // Haven't declared war yet
			/*  advc.104j: getDefensivePower counts vassals already.
				If planning war against multiple civs, DP allies could also be
				double counted (fixme). Could collect the war enemies in a std::set
				in a first pass; though it sucks to implement the vassal/DP logic
				multiple times (already in getDefensivePower and MilitaryAnalyst).
				Also, the computation for bConsiderOthers above can be way off. */
			!kEnemy.isAVassal())
		{
			int iTempPower = 240 * kEnemy.getDefensivePower(getID());
			iTempPower /= (AI_hasCitiesInPrimaryArea(kEnemy.getID()) ? 2 : 3);
			iTempPower /= 1 + (bConsiderOthers ? kEnemy.getNumWars(true, true) : 0);
			iEnemyPower += iTempPower;
		}
	}
	//return (iEnemyPower/std::max(1, (isAVassal() ? getCurrentMasterPower(true) : getPower(true))));
	// K-Mod - Lets not rely too much on our vassals...
	int iOurPower = getPower(false);
	const CvTeam& kMasterTeam = GET_TEAM(getMasterTeam());
	iOurPower += kMasterTeam.getPower(true);
	iOurPower /= 2;
	return iEnemyPower/std::max(1, iOurPower);
	// K-Mod end
}

// K-Mod
int CvTeamAI::AI_getAirPower() const
{
	int iTotalPower = 0;

	for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		// Since units of each class are counted per team rather than units of each type, just assume the default unit type.
		UnitTypes eLoopUnit = (UnitTypes)GC.getInfo((UnitClassTypes)iI).getDefaultUnitIndex();
		if (eLoopUnit != NO_UNIT)
		{
			const CvUnitInfo& kUnitInfo = GC.getInfo(eLoopUnit);
			if (kUnitInfo.getDomainType() == DOMAIN_AIR && kUnitInfo.getAirCombat() > 0)
			{
				iTotalPower += getUnitClassCount((UnitClassTypes)iI) * kUnitInfo.getPowerValue();
			}
		}
	}

	return iTotalPower;
}

// Sum up air power of enemies plus average of other civs we've met
// K-Mod: I've rewritten this BBAI function to loop over unit classes rather than unit types.
// This is because a loop over unit types will double-count if there are two units in the same class.
int CvTeamAI::AI_getRivalAirPower() const
{
	// Count enemy air units, not just those visible to us
	int iRivalAirPower = 0;
	int iEnemyAirPower = 0;
	int iTeamCount = 0;

	for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		// Since units of each class are counted per team rather than units of each type, just assume the default unit type.
		UnitTypes eLoopUnit = (UnitTypes)GC.getInfo((UnitClassTypes)iI).getDefaultUnitIndex();
		if (eLoopUnit != NO_UNIT)
		{
			const CvUnitInfo& kUnitInfo = GC.getInfo(eLoopUnit);

			if (kUnitInfo.getDomainType() == DOMAIN_AIR && kUnitInfo.getAirCombat() > 0)
			{
				for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
				{
					if (iTeam != getID() && GET_TEAM((TeamTypes)iTeam).isAlive()
						&& isHasMet((TeamTypes)iTeam) && !GET_TEAM((TeamTypes)iTeam).isMinorCiv())
					{
						iTeamCount++;

						int iUnitPower = kUnitInfo.getPowerValue() * GET_TEAM((TeamTypes)iTeam).getUnitClassCount((UnitClassTypes)iI);

						iRivalAirPower += iUnitPower;
						if (AI_getWarPlan((TeamTypes)iTeam) != NO_WARPLAN)
							iEnemyAirPower += iUnitPower;
					}
				}
			}
		}
	}
	return iEnemyAirPower + iRivalAirPower / std::max(1, iTeamCount);
}

// K-Mod
bool CvTeamAI::AI_refusePeace(TeamTypes ePeaceTeam) const
{
	// Refuse peace if we need the war for our conquest / domination victory.
	return (!isHuman() && AI_isAnyMemberDoVictoryStrategy(
			AI_VICTORY_CONQUEST4 | AI_VICTORY_DOMINATION4) &&
			((AI_isChosenWar(ePeaceTeam)
			// advc.115:
			&& GET_TEAM(ePeaceTeam).AI_getWarPlan(getID()) == WARPLAN_ATTACKED_RECENT)
			|| getNumWars(true, true) == 1) && AI_getWarSuccessRating() > 0);
}

bool CvTeamAI::AI_refuseWar(TeamTypes eWarTeam) const
{
	if (isHuman())
		return false;
	// <advc.104y>
	if (AI_isAvoidWar(eWarTeam))
	{
		AttitudeTypes eAttitude = AI_getAttitude(eWarTeam); // </advc.104y>
		// ok, so we wouldn't independently choose this war, but could we be bought into it?
		// If any of our team would refuse, then the team refuses. (cf. AI_declareWarTrade)
		for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
		{
			const CvPlayerAI& kLoopPlayer = GET_PLAYER(i);
			if (kLoopPlayer.isAlive() && kLoopPlayer.getTeam() == getID())
			{
				if (eAttitude > GC.getInfo(kLoopPlayer.getPersonalityType()).getDeclareWarThemRefuseAttitudeThreshold())
				{
					return true;
				}
			}
		}
	}

	// otherwise, war is acceptable
	return false;
}
// K-Mod end

// the following is a bbai function which has been edited for K-Mod (most of the K-Mod changes are unmarked)
bool CvTeamAI::AI_acceptSurrender(TeamTypes eSurrenderTeam) const  // advc: style changes
{
	//PROFILE_FUNC(); // advc.003o
	const CvTeamAI& kSurrenderTeam = GET_TEAM(eSurrenderTeam);

	if (isHuman())
		return true;

	if (!isAtWar(eSurrenderTeam))
		return true;

	// advc.112: Now handled by the vassal
	/*if (kSurrenderTeam.AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_SPACE3 | AI_VICTORY_CULTURE3)) {
		// Capturing capital or Apollo city will stop space
		// Capturing top culture cities will stop culture
		return false;
	}*/

	// Check for whether another team has won enough to cause capitulation
	bool bMightCapToOther = false; // K-Mod
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (iI == getID())
			continue;
		CvTeamAI const& kOther = GET_TEAM((TeamTypes)iI);
		// advc.112: was isVassal(getID()) and I've added the isMinorCiv check
		if (!kOther.isAlive() || kOther.isAVassal() || kOther.isMinorCiv() ||
			!kSurrenderTeam.isAtWar(kOther.getID()) ||
			kSurrenderTeam.AI_getAtWarCounter(kOther.getID()) <
			12 - kOther.getCurrentEra()) // advc.112: was 10 flat
		{
			continue;
		}
		if ((kSurrenderTeam.AI_getWarSuccess(kOther.getID()) +
			std::min(kSurrenderTeam.getNumCities(), 4) *
			GC.getWAR_SUCCESS_CITY_CAPTURING()) <
			kOther.AI_getWarSuccess(eSurrenderTeam))
		{	//return true;
			// K-Mod: that's not the only capitulation condition. I might revise it later, but in the mean time I'll just relax the effect.
			bMightCapToOther = true;
			break;
		}
	}

	int iValuableCities = 0;
	int iCitiesThreatenedByUs = 0;
	int iValuableCitiesThreatenedByUs = 0;
	int iCitiesThreatenedByOthers = 0;
	int iValuablesWithOurCulture = 0; // advc.099c
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayerAI const& kSurrenderCiv = GET_PLAYER((PlayerTypes)iI);
		if (!kSurrenderCiv.isAlive() || kSurrenderCiv.getTeam() != eSurrenderTeam)
			continue;

		FOR_EACH_CITYAI(pLoopCity, kSurrenderCiv)
		{
			CvCityAI const& kCity = *pLoopCity;
			bool bValuable = (kCity.isHolyCity() || kCity.isHeadquarters() ||
					kCity.hasActiveWorldWonder() ||
					(AI_isPrimaryArea(kCity.area()) &&
					kSurrenderTeam.countNumCitiesByArea(kCity.area()) < 3) ||
					(kCity.isCapital() &&
					(kSurrenderTeam.getNumCities() > kSurrenderTeam.getNumMembers() ||
					countNumCitiesByArea(kCity.area()) > 0)));
			if(!bValuable)
			{	// Valuable terrain bonuses
				for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
				{
					CvPlot* pLoopPlot = ::plotCity(kCity.getX(), kCity.getY(), iJ);
					if (pLoopPlot == NULL)
						continue;
					BonusTypes eBonus = pLoopPlot->getNonObsoleteBonusType(getID());
					if (eBonus == NO_BONUS)
						continue;
					if(GET_PLAYER(getLeaderID()).AI_bonusVal(eBonus, 1) > 15)
					{
						bValuable = true;
						break;
					}
				}
			}
			// <advc.099c>
			if(bValuable && pLoopCity->plot()->calculateTeamCulturePercent(getID()) > 25)
				iValuablesWithOurCulture++; // </advc.099c>
			/*int iOwnerPower = kSurrenderCiv.AI_getOurPlotStrength(kCity.plot(), 2, true, false);
			int iOurPower = AI_getOurPlotStrength(kCity.plot(), 2, false, false, true);
			int iOtherPower = kSurrenderCiv.AI_getEnemyPlotStrength(pkCity.plot(), 2, false, false) - iOurPower; */
			// K-Mod. Note. my new functions are not quite the same as the old.
			// a) this will not count vassals in "our power". b) it will only count forces that can been seen by the player calling the function.
			int iOwnerPower = kSurrenderCiv.AI_localDefenceStrength(
					kCity.plot(), kSurrenderCiv.getTeam(), DOMAIN_LAND, 2, true, false, true);
			int iOurPower = GET_PLAYER(getLeaderID()).AI_localAttackStrength(
					kCity.plot(), getID());
			int iOtherPower = kSurrenderCiv.AI_localAttackStrength(
					kCity.plot(), NO_TEAM) - iOurPower;
			// K-Mod end
			if (iOtherPower > iOwnerPower)
				iCitiesThreatenedByOthers++;

			/*  advc.130v: Added the coefficients. Now that capitulated vassals
				have fewer downsides, the AI should be quicker to accept them,
				at least if they put up a fight. */
			if (2 * iOurPower > 3 * iOwnerPower)
			{
				iCitiesThreatenedByUs++;
				if (bValuable)
				{
					iValuableCities++;
					iValuableCitiesThreatenedByUs++;
					continue;
				}
			}

			if (bValuable && kCity.getHighestPopulation() < 5)
				bValuable = false;

			if (!bValuable)
				continue;

			if (AI_isPrimaryArea(kCity.area()))
			{
				iValuableCities++;
				continue;
			}

			for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
			{
				CvPlayerAI const& kOurMember = GET_PLAYER((PlayerTypes)iJ);
				if (!kOurMember.isAlive() || kOurMember.getTeam() != getID())
					continue;
				if (kCity.AI_playerCloseness(kOurMember.getID(),
					/* <advc.001n> */ DEFAULT_PLAYER_CLOSENESS, true /* </advc.001n> */) > 5)
				{
					iValuableCities++;
					break;
				}
			}
		}
	}

	int iOurWarSuccessRating = AI_getWarSuccessRating();
	if (iOurWarSuccessRating < -30)
	{	// We're doing badly overall, need to be done with this war and gain an ally
		return true;
	}
	/*  advc.099c: Was > 0. Should be less keen on conquering large cities b/c
		the foreign culture will cause problems. maxWarRand is between 50 and 400 (Gandhi). */
	if(iValuableCitiesThreatenedByUs > AI_maxWarRand() / 100)
	{	// Press for capture of valuable city
		return false;
	}
	// K-Mod.
	if (iCitiesThreatenedByOthers > (1 + iCitiesThreatenedByUs/2) &&
			(bMightCapToOther || iCitiesThreatenedByOthers >= iValuableCities)) //
	{	// Keep others from capturing spoils, but let it go if surrender civ is too small to care about
		/* original BBAI code
		if (6 * (iValuableCities + kSurrenderTeam.getNumCities()) > getNumCities())
			return true;*/
		// K-Mod
		if (5*iValuableCities + 3*(kSurrenderTeam.getNumCities()-iCitiesThreatenedByUs) > getNumCities())
			return true;
		// K-Mod end
	}

	// If we're low on the totem pole, accept so enemies don't drag anyone else into war with us
	// Top rank is 0, second is 1, etc.
	if ((bMightCapToOther || iOurWarSuccessRating < 60) &&
			GC.getGame().getTeamRank(getID()) >
			1 + GC.getGame().countCivTeamsAlive()/3)
		return true;

	if (iOurWarSuccessRating < 50)
	{	// Accept if we have other wars to fight
		for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
		{
			CvTeamAI const& kLoopTeam = GET_TEAM(i);
			if (isAtWar(i) && kLoopTeam.isAlive() && !kLoopTeam.isMinorCiv() &&
					i != eSurrenderTeam && !kLoopTeam.isVassal(eSurrenderTeam))
			{
				if (kLoopTeam.AI_getWarSuccess(getID()) > 5*GC.getDefineINT(CvGlobals::WAR_SUCCESS_ATTACKING))
				{
					return true;
				}
			}
		}
	}

	// War weariness
	int iWearinessThreshold = (GC.getGame().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 300 : 240);
	if (!bMightCapToOther)
	{
		iWearinessThreshold += 20*iValuableCities + 30*iCitiesThreatenedByUs;
		iWearinessThreshold += 10*std::max(0, GC.getInfo(GC.getMap().getWorldSize()).getTargetNumCities() - kSurrenderTeam.getNumCities()); // (to help finish off small civs)
	}

	for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
	{
		const CvPlayer& kLoopPlayer = GET_PLAYER(i);
		if (kLoopPlayer.isAlive() && kLoopPlayer.getTeam() == getID())
		{
			if (kLoopPlayer.getWarWearinessPercentAnger() > iWearinessThreshold) // K-Mod note: it isn't really "percent". The API lies.
			{
				int iWwPerMil = kLoopPlayer.getModifiedWarWearinessPercentAnger(getWarWeariness(eSurrenderTeam, true) / 100);
				if (iWwPerMil > iWearinessThreshold/2)
				{
					return true;
				}
			}
		}
	}
	/*  advc.099c: Was iCitiesThreatenedByUs+iValuableCities.
		Note that most cities are considered valuable. Added the >0 clause. */
	if(iCitiesThreatenedByUs > 0 && iCitiesThreatenedByUs >= AI_maxWarRand() / 100)
	{	// Continue conquest
		return false;
	}
	/*  <advc.099c> Instead keep fighting if they have cities with our culture, or
		if we still have relatively few cities. */
	if(iValuablesWithOurCulture > 0 || getNumCities() <
			(GC.getInfo(GC.getMap().getWorldSize()).getTargetNumCities() *
			7 * getNumMembers()) / 5)
		return false;
	// </advc.099c>
	if (kSurrenderTeam.getNumCities() < (getNumCities()/4 - (AI_maxWarRand()/100)))
	{	// Too small to bother leaving alive
		return false;
	}

	return true;
}

// BETTER_BTS_AI_MOD, War Strategy AI, 03/20/10, jdog5000: START
void CvTeamAI::AI_getWarRands(int &iMaxWarRand, int &iLimitedWarRand, int &iDogpileWarRand) const
{
	iMaxWarRand = AI_maxWarRand();
	iLimitedWarRand = AI_limitedWarRand();
	iDogpileWarRand = AI_dogpileWarRand();

	bool bCult4 = false;
	bool bSpace4 = false;
	bool bCult3 = false;
	bool bFinalWar = false;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).AI_isDoStrategy(AI_STRATEGY_FINAL_WAR))
				{
					bFinalWar = true;
				}

				if (GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4))
				{
					bCult4 = true;
				}
				if (GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3))
				{
					bCult3 = true;
				}
				if (GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_SPACE4))
				{
					bSpace4 = true;
				}
			}
		}
	}

	if (bCult4)
	{
		iMaxWarRand *= 4;
		iLimitedWarRand *= 3;
		iDogpileWarRand *= 2;
	}
	else if (bSpace4)
	{
		iMaxWarRand *= 3;

		iLimitedWarRand *= 2;

		iDogpileWarRand *= 3;
		iDogpileWarRand /= 2;
	}
	else if (bCult3)
	{
		iMaxWarRand *= 2;

		iLimitedWarRand *= 3;
		iLimitedWarRand /= 2;

		iDogpileWarRand *= 3;
		iDogpileWarRand /= 2;
	}

	int iNumMembers = getNumMembers();
	int iNumVassals = getVassalCount();

	iMaxWarRand *= (2 + iNumMembers);
	iMaxWarRand /= (2 + iNumMembers + iNumVassals);

	if (bFinalWar)
	{
		iMaxWarRand /= 4;
	}

	iLimitedWarRand *= (2 + iNumMembers);
	iLimitedWarRand /= (2 + iNumMembers + iNumVassals);

	iDogpileWarRand *= (2 + iNumMembers);
	iDogpileWarRand /= (2 + iNumMembers + iNumVassals);
}


void CvTeamAI::AI_getWarThresholds(int &iTotalWarThreshold, int &iLimitedWarThreshold, int &iDogpileWarThreshold) const
{
	iTotalWarThreshold = 0;
	iLimitedWarThreshold = 0;
	iDogpileWarThreshold = 0;

	//int iHighUnitSpendingPercent = 0;
	int iHighUnitSpending = 0; // K-Mod
	bool bConq2 = false;
	bool bDom3 = false;
	bool bAggressive = GC.getGame().isOption(GAMEOPTION_AGGRESSIVE_AI);
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)  // advc: refactored the body
	{
		CvPlayerAI const& kMember = GET_PLAYER((PlayerTypes)iI);
		if (kMember.getTeam() != getID() || !kMember.isAlive())
			continue;

		/* int iUnitSpendingPercent = (kMember.calculateUnitCost() * 100) / std::max(1, kMember.calculatePreInflatedCosts());
		iHighUnitSpendingPercent += (std::max(0, iUnitSpendingPercent - 7) / 2); */
		int iUnitSpendingPerMil = kMember.AI_unitCostPerMil(); // K-Mod
		iHighUnitSpending += (std::max(0, iUnitSpendingPerMil - 16) / 6); // K-Mod

		if (kMember.AI_isDoStrategy(AI_STRATEGY_DAGGER) ||
				kMember.AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST4) ||
				kMember.AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION4))
			bAggressive = true;
		if (kMember.AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST2))
			bConq2 = true;
		if (kMember.AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION3))
			bDom3 = true;
	}

	iHighUnitSpending /= std::max(1, getNumMembers());
	iTotalWarThreshold = iHighUnitSpending *
			//(bAggressive ? 3 : 2);
			2; // advc.019: The  +=bAggressive?1:0  below should be enough aggro
	if (bDom3)
	{
		iTotalWarThreshold *= 3;

		iDogpileWarThreshold += 5;
	}
	else if (bConq2)
	{
		iTotalWarThreshold *= 2;

		iDogpileWarThreshold += 2;
	}
	iTotalWarThreshold /= 3;
	iTotalWarThreshold += bAggressive ? 1 : 0;

	if (bAggressive && GET_PLAYER(getLeaderID()).getCurrentEra() < 3)
	{
		iLimitedWarThreshold += 2;
	}
}

// Returns odds of player declaring total war times 100
int CvTeamAI::AI_getTotalWarOddsTimes100() const
{
	int iTotalWarRand;
	int iLimitedWarRand;
	int iDogpileWarRand;
	AI_getWarRands(iTotalWarRand, iLimitedWarRand, iDogpileWarRand);

	/*  <advc.104> With UWAI, this function is only called for AI tech and civic
		decisions. Don't want unit spending (see AI_getWarThresholds) to matter
		for those decisions. */
	if(getWPAI.isEnabled())
	{
		// I don't see a fundamental difference between Domination and Conquest here
		int iMilitaryVictoryFactor = 0;
		if(AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST2 | AI_VICTORY_DOMINATION2))
			iMilitaryVictoryFactor = 3; // Don't care about 2 vs. 3 vs. 4 here
		else if(AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST1 | AI_VICTORY_DOMINATION1))
			iMilitaryVictoryFactor = 2;
		return 100 * iMilitaryVictoryFactor + 20000 / std::max(iTotalWarRand, 1);
	} // </advc.104>

	int iTotalWarThreshold;
	int iLimitedWarThreshold;
	int iDogpileWarThreshold;
	AI_getWarThresholds(iTotalWarThreshold, iLimitedWarThreshold, iDogpileWarThreshold);

	return ((100 * 100 * iTotalWarThreshold) / std::max(1, iTotalWarRand));
}
// BETTER_BTS_AI_MOD: END

int CvTeamAI::AI_makePeaceTradeVal(TeamTypes ePeaceTeam, TeamTypes eTeam) const
{
	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(ePeaceTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(GET_TEAM(ePeaceTeam).isAlive(), "GET_TEAM(ePeaceTeam).isAlive is expected to be true");
	FAssertMsg(atWar(ePeaceTeam, eTeam), "eTeam should be at war with ePeaceTeam");
	// <advc.104>
	if(getWPAI.isEnabled())
		return GET_TEAM(eTeam).warAndPeaceAI().makePeaceTradeVal(ePeaceTeam, getID());
	// </advc.104>
	int iValue = (50 + GC.getGame().getGameTurn());
	iValue += ((GET_TEAM(eTeam).getNumCities() + GET_TEAM(ePeaceTeam).getNumCities()) * 8);

	int iModifier = 0;

	switch ((GET_TEAM(eTeam).AI_getAttitude(ePeaceTeam) + GET_TEAM(ePeaceTeam).AI_getAttitude(eTeam)) / 2)
	{
	case ATTITUDE_FURIOUS:
		iModifier += 400;
		break;

	case ATTITUDE_ANNOYED:
		iModifier += 200;
		break;

	case ATTITUDE_CAUTIOUS:
		iModifier += 100;
		break;

	case ATTITUDE_PLEASED:
		iModifier += 50;
		break;

	case ATTITUDE_FRIENDLY:
		break;

	default:
		FAssert(false);
		break;
	}

	iValue *= std::max(0, (iModifier + 100));
	iValue /= 100;

	iValue *= 40;
	iValue /= (GET_TEAM(eTeam).AI_getAtWarCounter(ePeaceTeam) + 10);
	return AI_roundTradeVal(iValue); // advc.104k
}

// <advc.104k> Same procedure as in BtS mostly
int CvTeamAI::AI_roundTradeVal(int iVal) const
{
	int rem = GC.getDefineINT(CvGlobals::DIPLOMACY_VALUE_REMAINDER);
	iVal -= iVal % rem;
	/*  Not sure if this lower bound is really needed. The BtS code
		(see CvPlayerAI::AI_roundTradeVal) doesn't have it. */
	if (isHuman()) 
		return std::max(iVal, rem);
	return iVal;
} // </advc.104k>

// <advc.104i>
void CvTeamAI::AI_makeUnwillingToTalk(TeamTypes eOther)
{
	if (!getWPAI.isEnabled())
		return;
	// No need to make vassals unwilling to talk; can't negotiate war/ peace anyway.
	if (eOther == NO_TEAM || isAVassal() || GET_TEAM(eOther).isAVassal())
		return;
	/*  Make each leading member i of our team unwilling to talk to every
		leading member j of the other team. "Leading": team leader or human;
		only these can make peace. */
	for (int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayerAI& kOurMember = GET_PLAYER((PlayerTypes)i);
		if(!kOurMember.isAlive() || kOurMember.getTeam() != getID() ||
				(!kOurMember.isHuman() && kOurMember.getID() != getLeaderID()))
			continue;
		for (int j = 0; j < MAX_CIV_PLAYERS; j++)
		{
			CvPlayerAI& kTheirMember = GET_PLAYER((PlayerTypes)j);
			if(!kTheirMember.isAlive() || kTheirMember.getTeam() != eOther ||
					(!kTheirMember.isHuman() && kTheirMember.getID() !=
					GET_TEAM(eOther).getLeaderID()))
				continue;
			if (!kOurMember.isHuman() &&
				kOurMember.AI_getMemoryCount(kTheirMember.getID(), MEMORY_DECLARED_WAR_RECENT) < 2)
			{
				kOurMember.AI_rememberEvent(kTheirMember.getID(),
						MEMORY_DECLARED_WAR_RECENT);
			}
			/*  Memory has no effect on humans. Make the other side unwilling then.
				Could simply always make both sides unwilling, but then, the
				expected RTT duration would become longer. */
			else if(kOurMember.isHuman() && kTheirMember.AI_getMemoryCount(
				kOurMember.getID(), MEMORY_DECLARED_WAR_RECENT) < 2)
			{
				kTheirMember.AI_rememberEvent(kOurMember.getID(),
						MEMORY_DECLARED_WAR_RECENT);
			}
		}
	}
} // </advc.104i>


DenialTypes CvTeamAI::AI_makePeaceTrade(TeamTypes ePeaceTeam, TeamTypes eBroker) const  // advc: eTeam renamed to eBroker
{
	FAssertMsg(eBroker != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(ePeaceTeam != getID(), "shouldn't call this function on ourselves");
	FAssert(GET_TEAM(ePeaceTeam).isAlive());
	FAssert(isAtWar(ePeaceTeam));

	if (GET_TEAM(ePeaceTeam).isHuman())
	{	//return DENIAL_CONTACT_THEM;
		// advc.004d: Reserve "contact them" for when we'd like to end the war
		return DENIAL_PEACE_NOT_POSSIBLE_US;
	}

	if (GET_TEAM(ePeaceTeam).isAVassal())
		return DENIAL_VASSAL;

	if (isHuman())
		return NO_DENIAL;

	if (!canChangeWarPeace(ePeaceTeam))
		return DENIAL_VASSAL;
	// <advc.104>
	if(getWPAI.isEnabled())
		return warAndPeaceAI().makePeaceTrade(ePeaceTeam, eBroker);
	// </advc.104>
	if (AI_endWarVal(ePeaceTeam) > (GET_TEAM(ePeaceTeam).AI_endWarVal(getID()) * 2))
		return DENIAL_CONTACT_THEM;

	/* original bts code
	int iLandRatio = ((getTotalLand(true) * 100) / std::max(20, GET_TEAM(eTeam).getTotalLand(true)));
	if (iLandRatio > 250)
		return DENIAL_VICTORY;*/
	// K-Mod
	if (AI_refusePeace(ePeaceTeam))
		return DENIAL_VICTORY;
	// <advc.004d>
	if(isAtWar(eBroker) && !GET_TEAM(ePeaceTeam).AI_refusePeace(getID()) &&
			GET_PLAYER(GET_TEAM(ePeaceTeam).getLeaderID()).AI_isWillingToTalk(getLeaderID()))
		return NO_DENIAL; // </advc.004d>
	if (!GET_PLAYER(getLeaderID()).canContactAndTalk(GET_TEAM(ePeaceTeam).getLeaderID()) ||
			GET_TEAM(ePeaceTeam).AI_refusePeace(getID()))
		//return DENIAL_CONTACT_THEM;
		return DENIAL_RECENT_CANCEL; // advc.004d: Contacting them is not helpful
	// K-Mod end
	return NO_DENIAL;
}

// advc.104o: Moved the K-Mod/BtS code from AI_declareWarTradeVal here
int CvTeamAI::AI_declareWarTradeValLegacy(TeamTypes eWarTeam, TeamTypes eTeam) const
{
	PROFILE_FUNC();

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(eWarTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(GET_TEAM(eWarTeam).isAlive(), "GET_TEAM(eWarTeam).isAlive is expected to be true");
	FAssertMsg(!atWar(eWarTeam, eTeam), "eTeam should be at peace with eWarTeam");

	int iValue = 0;
	iValue += (GET_TEAM(eWarTeam).getNumCities() * 10);
	iValue += (GET_TEAM(eWarTeam).getTotalPopulation(true) * 2);

	int iModifier = 0;

	switch (GET_TEAM(eTeam).AI_getAttitude(eWarTeam))
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
		iModifier += 150;
		break;

	case ATTITUDE_FRIENDLY:
		iModifier += 400;
		break;

	default:
		FAssert(false);
		break;
	}

	iValue *= std::max(0, (iModifier + 100));
	iValue /= 100;

	int iTheirPower = GET_TEAM(eTeam).getPower(true);
	int iWarTeamPower = GET_TEAM(eWarTeam).getPower(true);

	static int const iWAR_TRADEVAL_POWER_WEIGHT = GC.getDefineINT("WAR_TRADEVAL_POWER_WEIGHT"); // advc.100
	iValue *= 50 + ((/* advc.100: */ iWAR_TRADEVAL_POWER_WEIGHT
			* iWarTeamPower) / (iTheirPower + iWarTeamPower + 1));
	iValue /= 100;

	if (!(GET_TEAM(eTeam).AI_isAllyLandTarget(eWarTeam)))
	{
		iValue *= 2;
	}

	if (!isAtWar(eWarTeam))
	{
		iValue *= 3;
	}
	else
	{
		iValue *= 150;
		iValue /= 100 + ((50 * std::min(100, (100 * AI_getWarSuccess(eWarTeam)) / (8 + getTotalPopulation(false)))) / 100);
	}

	iValue += (GET_TEAM(eTeam).getNumCities() * 20);
	iValue += (GET_TEAM(eTeam).getTotalPopulation(true) * 15);

	if (isAtWar(eWarTeam))
	{
		switch (GET_TEAM(eTeam).AI_getAttitude(getID()))
		{
		case ATTITUDE_FURIOUS:
		case ATTITUDE_ANNOYED:
		case ATTITUDE_CAUTIOUS:
			iValue *= 100;
			break;

		case ATTITUDE_PLEASED:
			iValue *= std::max(75, 100 - getNumWars() * 10);
			break;

		case ATTITUDE_FRIENDLY:
			iValue *= std::max(50, 100 - getNumWars() * 20);
			break;

		default:
			FAssert(false);
			break;
		}
		iValue /= 100;
	}

	iValue += GET_TEAM(eWarTeam).getNumNukeUnits() * 250;//Don't want to get nuked
	iValue += GET_TEAM(eTeam).getNumNukeUnits() * 150;//Don't want to use nukes on another's behalf

	if (GET_TEAM(eWarTeam).getNumWars(false) <= 0)
	{
		iValue *= 2;

		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (iI != getID() && iI != eWarTeam && iI != eTeam)
				{
					if (GET_TEAM(eWarTeam).isDefensivePact((TeamTypes)iI))
					{
						iValue += (GET_TEAM((TeamTypes)iI).getNumCities() * 30);
						iValue += (GET_TEAM((TeamTypes)iI).getTotalPopulation(true) * 20);
					}
				}
			}
		}
	}

	iValue *= 60 + (140 * GC.getGame().getGameTurn()) / std::max(1, GC.getGame().getEstimateEndTurn());
	iValue /= 100;
	return iValue;
}


int CvTeamAI::AI_declareWarTradeVal(TeamTypes eTarget, TeamTypes eSponsor) const // advc: params renamed
{
	// <advc.104o>
	int r = -1;
	if(getWPAI.isEnabled())
		r = GET_TEAM(eSponsor).warAndPeaceAI().declareWarTradeVal(
				// eWarTeam can be a (voluntary) vassal
				GET_TEAM(eTarget).getMasterTeam(), getID());
	else r = AI_declareWarTradeValLegacy(eTarget, eSponsor);
	// Don't charge much less than for an embargo
	CvPlayerAI const& kAllyLeader = GET_PLAYER(GET_TEAM(eSponsor).getLeaderID());
	if(kAllyLeader.canStopTradingWithTeam(eTarget)) {
		r = std::max(r, ::round(0.83 * GET_PLAYER(getLeaderID()).AI_stopTradingTradeVal(
				eTarget, kAllyLeader.getID(), true)));
	} // </advc.104o>
	return AI_roundTradeVal(r); // advc.104k
}


DenialTypes CvTeamAI::AI_declareWarTrade(TeamTypes eTarget, TeamTypes eSponsor, bool bConsiderPower) const  // advc: params renamed; some other style changes
{
	PROFILE_FUNC();

	FAssertMsg(eSponsor != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(eTarget != getID(), "shouldn't call this function on ourselves");
	FAssert(GET_TEAM(eTarget).isAlive());
	FAssert(!isAtWar(eTarget));

	if (GET_TEAM(eTarget).isVassal(eSponsor) || GET_TEAM(eTarget).isDefensivePact(eSponsor))
		return DENIAL_JOKING;

	if (isHuman())
		return NO_DENIAL;

	if (!canDeclareWar(eTarget))
		return DENIAL_VASSAL;
	// <advc.104o> Provide no further info to an enemy (applies even when UWAI disabled)
	if(AI_getWorstEnemy() == eSponsor)
		return DENIAL_WORST_ENEMY;
	if(!getWPAI.isEnabled())
	{	// UWAI handles these DenialTypes later // </advc.104o>
		// BETTER_BTS_AI_MOD, Diplomacy, 12/06/09, jdog5000
		/* original BTS code
		if (getAnyWarPlanCount(true) > 0)
			return DENIAL_TOO_MANY_WARS;*/
		// Hide WHEOOHRN revealing war plans
		if (getNumWars() > 0)
			return DENIAL_TOO_MANY_WARS;
		// BETTER_BTS_AI_MOD: END
		if (bConsiderPower)
		{
			bool bLandTarget = AI_isAllyLandTarget(eTarget);
			int iDefPower = GET_TEAM(eTarget).getDefensivePower(getID());
			int iPow = getPower(true);
			int iAggPower = iPow + ((::atWar(eTarget, eSponsor)) ? GET_TEAM(eSponsor).getPower(true) : 0);
			/*  <advc.100> Introduced variables iPow, iAggPower, iDefPower.
				Made the comparison stricter (more reluctant to declare war).
				Added a second clause: don't gang up on enemies with far greater power. */
			if (iDefPower / (bLandTarget ? 1.5 : 1.0) > iAggPower || // was 2 : 1
					(iDefPower > iAggPower && iPow * 2 < iDefPower)) // </advc.100>
			{
				if (bLandTarget)
					return DENIAL_POWER_THEM;
				else return DENIAL_NO_GAIN;
			}
		}
	}

	AttitudeTypes eTowardSponsor = AI_getAttitude(eSponsor);
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayerAI const& kOurMember = GET_PLAYER((PlayerTypes)iI);
		if (!kOurMember.isAlive() || kOurMember.getTeam() != getID())
			continue;
		if (eTowardSponsor <= GC.getInfo(kOurMember.getPersonalityType()).
				getDeclareWarRefuseAttitudeThreshold())
			return DENIAL_ATTITUDE;
	}

	AttitudeTypes eAttitudeThem = AI_getAttitude(eTarget);
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayerAI const& kOurMember = GET_PLAYER((PlayerTypes)iI);
		if (!kOurMember.isAlive() || kOurMember.getTeam() != getID())
			continue;
		if (eAttitudeThem > GC.getInfo(kOurMember.getPersonalityType()).
				getDeclareWarThemRefuseAttitudeThreshold())
			return DENIAL_ATTITUDE_THEM;
	}

	if (!atWar(eTarget, eSponsor))
	{
		if (GET_TEAM(eTarget).getNumNukeUnits() > 0)
		{	//return DENIAL_JOKING;
			return DENIAL_POWER_THEM; // advc.004g: Easier to understand
		}
	} // <advc.104o>
	if(getWPAI.isEnabled()) // (ignore bConsiderPower)
	{/* Refuse to start wars that we'll probably not engage in b/c we're busy
		fighting a closer enemy. Unless eTeam is already at war with eWarTeam.
		The main goal is to reduce the amount of messages generated by the
		war trade alert (advc.210a). */
		TeamTypes eClosestWarEnemy = NO_TEAM;
		int iHighestCloseness = MIN_INT;
		if(!GET_TEAM(eTarget).isAtWar(eSponsor))
		{
			for(int i = 0; i < MAX_CIV_TEAMS; i++)
			{
				CvTeamAI const& kEnemy = GET_TEAM((TeamTypes)i);
				if(!kEnemy.isAlive() || kEnemy.isMinorCiv() || !kEnemy.isAtWar(getID()))
					continue;
				int iCloseness = AI_teamCloseness(kEnemy.getID(), DEFAULT_PLAYER_CLOSENESS,
						true, true); // bConstCache!
				if(iCloseness > iHighestCloseness) 
				{
					iHighestCloseness = iCloseness;
					eClosestWarEnemy = kEnemy.getID();
				}
			}
		}
		if(eClosestWarEnemy != NO_TEAM)
		{
			/*  If any fighting occurred or recently declared, don't treat closeness
				as 0. (And don't be willing to attack teams with 0 closeness.) */
			if(iHighestCloseness <= 0 && (GET_TEAM(eClosestWarEnemy).
					AI_getWarPlan(getID()) == WARPLAN_ATTACKED_RECENT ||
					AI_getWarPlan(eClosestWarEnemy) == WARPLAN_ATTACKED_RECENT ||
					AI_getWarSuccess(eClosestWarEnemy) +
					GET_TEAM(eClosestWarEnemy).AI_getWarSuccess(getID()) > 0))
				iHighestCloseness = 1;
			int iCloseness = AI_teamCloseness(eTarget, DEFAULT_PLAYER_CLOSENESS,
					true, true);
			if(iCloseness < iHighestCloseness)
				return DENIAL_TOO_MANY_WARS;
		}
		return warAndPeaceAI().declareWarTrade(
				GET_TEAM(eTarget).getMasterTeam(), eSponsor); // eTarget can be a (voluntary) vassal
	} // </advc.104o>
	// BETTER_BTS_AI_MOD, Diplomacy, 12/06/09, jdog5000: START
	if (AI_isAnyWarPlan())
		return DENIAL_TOO_MANY_WARS;
	// BETTER_BTS_AI_MOD: END
	return NO_DENIAL;
}


int CvTeamAI::AI_openBordersTradeVal(TeamTypes eTeam) const
{
	return (getNumCities() + GET_TEAM(eTeam).getNumCities());
}


DenialTypes CvTeamAI::AI_openBordersTrade(TeamTypes eWithTeam) const  // advc: some style changes
{
	//PROFILE_FUNC(); // advc,003o
	FAssertMsg(eWithTeam != getID(), "shouldn't call this function on ourselves");

	if (isHuman() || isVassal(eWithTeam))
		return NO_DENIAL;

	// advc.124: Handled later now.
	/*if (AI_shareWar(eTeam))
		return NO_DENIAL;*/

	if (AI_getMemoryCount(eWithTeam, MEMORY_CANCELLED_OPEN_BORDERS) > 0
			&& !AI_shareWar(eWithTeam)) // advc.124
		return DENIAL_RECENT_CANCEL;

	if (AI_getWorstEnemy() == eWithTeam)
		return DENIAL_WORST_ENEMY;

	int iOurAttitude = AI_getAttitude(eWithTeam);
	// <advc.124>
	bool bTheirLandRevealed = false;
	for(int i = 0; i < GC.getMap().numPlots(); i++)
	{
		CvPlot const& kPlot = GC.getMap().getPlotByIndex(i);
		if(kPlot.getTeam() == eWithTeam && kPlot.isRevealed(getID()) && !kPlot.isWater())
		{
			bTheirLandRevealed = true;
			break;
		}
	}
	if(bTheirLandRevealed && AI_shareWar(eWithTeam))
		iOurAttitude = std::min(NUM_ATTITUDE_TYPES - 1, iOurAttitude + 1);
	// </advc.124>

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer const& kOurMember = GET_PLAYER((PlayerTypes)iI);
		if (!kOurMember.isAlive() || kOurMember.getTeam() != getID())
			continue;
		// <advc.124>
		int iAttitudeThresh = GC.getInfo(GET_PLAYER((PlayerTypes)iI).
				getPersonalityType()).getOpenBordersRefuseAttitudeThreshold();
		if(iOurAttitude <= iAttitudeThresh)
			return DENIAL_ATTITUDE;
		else if(iOurAttitude == iAttitudeThresh + 1 && !bTheirLandRevealed)
			return DENIAL_NO_GAIN;
		// </advc.124>
	}

	return NO_DENIAL;
}


int CvTeamAI::AI_defensivePactTradeVal(TeamTypes eTeam) const
{
	return (getNumCities() + GET_TEAM(eTeam).getNumCities()) * 3;
}


DenialTypes CvTeamAI::AI_defensivePactTrade(TeamTypes eWithTeam) const
{
	//PROFILE_FUNC(); // advc.003o
	FAssertMsg(eWithTeam != getID(), "shouldn't call this function on ourselves");

	if (isHuman())
		return NO_DENIAL;

	// <dlph.3> (actually an advc change): Refuses/ cancels DP when ally makes peace
	if(!allWarsShared(eWithTeam))
		return DENIAL_JOKING; // </dlph.3>
	// <advc.130p>
	if(AI_getMemoryCount(eWithTeam, MEMORY_CANCELLED_DEFENSIVE_PACT) > 0)
		return DENIAL_RECENT_CANCEL;
	// </advc.130p>
	if (GC.getGame().countCivTeamsAlive() == 2)
		return DENIAL_NO_GAIN;
	// <advc.130t>
	if(!isOpenBorders(eWithTeam))
		return DENIAL_JOKING; // </advc.130t>
	if (AI_getWorstEnemy() == eWithTeam)
		return DENIAL_WORST_ENEMY;

	int iAttitude = AI_getAttitude(eWithTeam);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayerAI const& kOurMember = GET_PLAYER((PlayerTypes)i);
		if (!kOurMember.isAlive() || kOurMember.getTeam() != getID())
			continue;
		if (iAttitude <= GC.getInfo(kOurMember.getPersonalityType()).getDefensivePactRefuseAttitudeThreshold())
			return DENIAL_ATTITUDE;
	}
	return NO_DENIAL;
}


DenialTypes CvTeamAI::AI_permanentAllianceTrade(TeamTypes eWithTeam) const  // advc: style changes
{
	PROFILE_FUNC();

	FAssertMsg(eWithTeam != getID(), "shouldn't call this function on ourselves");

	if (isHuman())
		return NO_DENIAL;

	if (AI_getWorstEnemy() == eWithTeam)
		return DENIAL_WORST_ENEMY;

	if (getPower(true) + GET_TEAM(eWithTeam).getPower(true) > GC.getGame().countTotalCivPower() / 2)
	{
		if (getPower(true) > GET_TEAM(eWithTeam).getPower(true))
			return DENIAL_POWER_US;
		return DENIAL_POWER_YOU;
	}

	if (AI_getDefensivePactCounter(eWithTeam) + AI_getShareWarCounter(eWithTeam) < 40)
		return DENIAL_NOT_ALLIED;

	int iAttitude = AI_getAttitude(eWithTeam);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		CvPlayerAI const& kOurMember = GET_PLAYER((PlayerTypes)i);
		if (!kOurMember.isAlive() || kOurMember.getTeam() != getID())
			continue;
		if (iAttitude <= GC.getInfo(kOurMember.getPersonalityType()).getPermanentAllianceRefuseAttitudeThreshold())
			return DENIAL_ATTITUDE;
	}

	return NO_DENIAL;
}


TeamTypes CvTeamAI::AI_getWorstEnemy() const
{
	return m_eWorstEnemy;
}


void CvTeamAI::AI_updateWorstEnemy(/* advc.130p: */ bool bUpdateRivalTrade)
{
	PROFILE_FUNC();

	TeamTypes eBestTeam = NO_TEAM;

	int iBestValue = AI_enmityValue(m_eWorstEnemy);
	if(iBestValue > 0) {
		eBestTeam = m_eWorstEnemy;
		/*  advc.130p: Inertia; to reduce oscillation. New worst enemy has to be
			strictly worse than current minus 1.
			Oscillation is already a problem in BtS, but changes in CvPlayerAI::
			AI_getRivalTradeAttitude (penalty for OB) make it worse.
			Inertia could lead to a situation where we're only Annoyed against
			the worst enemy, but Furious towards another civ, but flip-flopping
			between the two would be even worse. */
		iBestValue++;
	}
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		TeamTypes eLoopTeam = (TeamTypes)iI;
		// <advc.130p>
		if(eLoopTeam == m_eWorstEnemy) // No need to evaluate this one twice
			continue;
		// Moved into new function
		int iValue = AI_enmityValue(eLoopTeam);
		if(iValue > iBestValue) // Now computes a maximum // </advc.130p>
		{
			iBestValue = iValue;
			eBestTeam = eLoopTeam;
		}
	}
	// <advc.130p>
	if(eBestTeam == m_eWorstEnemy)
		return;
	if(bUpdateRivalTrade && m_eWorstEnemy != NO_TEAM && m_eWorstEnemy != eBestTeam) {
		for(int i = 0; i < MAX_CIV_TEAMS; i++)
		{
			TeamTypes eOther = (TeamTypes)i;
			// The old enemy can't have traded with itself
			if(!GET_TEAM(eOther).isAlive() || eOther == m_eWorstEnemy)
				continue;
			int iOldGrantVal = AI_getEnemyPeacetimeGrantValue(eOther);
			int iOldTradeVal = AI_getEnemyPeacetimeTradeValue(eOther);
			AI_setEnemyPeacetimeGrantValue(eOther, (2 * iOldGrantVal) / 3);
			AI_setEnemyPeacetimeTradeValue(eOther, (2 * iOldTradeVal) / 3);
		}
		// The above loop may have improved relations with eBestTeam
		AI_updateWorstEnemy(false);
		return;
	}
	m_eWorstEnemy = eBestTeam;
	/*  Changing EnemyPeacetime values updates the attitude cache, but that's
		still based on the old worst enemy. Need another update vs. everyone
		(anyone could have OB with the new enemy). */
	for(int i = 0; i < MAX_CIV_TEAMS; i++)
	{
		CvTeam const& t = GET_TEAM((TeamTypes)i);
		if(t.isAlive() && t.getID() != getID() && !t.isMinorCiv())
		{
			AI_updateAttitudeCache(t.getID(),
					false); // Don't update worst enemy again after that
		}
	} // </advc.130p>
}

// <advc.130p>
int CvTeamAI::AI_enmityValue(TeamTypes eEnemy) const
{
	if(eEnemy == NO_TEAM)
		return 0;
	CvTeam const& kEnemy = GET_TEAM(eEnemy);
	if(eEnemy == getID() || !kEnemy.isAlive() || kEnemy.isCapitulated() ||
			isVassal(eEnemy) || // advc.130d
			kEnemy.isMinorCiv() || // wasn't excluded in BtS
			!isHasMet(eEnemy) ||
			(AI_getAttitude(eEnemy) >= ATTITUDE_CAUTIOUS && !isAtWar(eEnemy)))
		return 0;
	int r = 100 - AI_getAttitudeVal(eEnemy);
	if(isAtWar(eEnemy) && AI_getWarPlan(eEnemy) != WARPLAN_DOGPILE)
		r += 100;
	return r;
} // </advc.130p>

int CvTeamAI::AI_getWarPlanStateCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiWarPlanStateCounter[eIndex];
}


void CvTeamAI::AI_setWarPlanStateCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiWarPlanStateCounter[eIndex] = iNewValue;
	FAssert(AI_getWarPlanStateCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeWarPlanStateCounter(TeamTypes eIndex, int iChange)
{
	AI_setWarPlanStateCounter(eIndex, (AI_getWarPlanStateCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getAtWarCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiAtWarCounter[eIndex];
}


void CvTeamAI::AI_setAtWarCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiAtWarCounter[eIndex] = iNewValue;
	FAssert(AI_getAtWarCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeAtWarCounter(TeamTypes eIndex, int iChange)
{
	AI_setAtWarCounter(eIndex, (AI_getAtWarCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getAtPeaceCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiAtPeaceCounter[eIndex];
}


void CvTeamAI::AI_setAtPeaceCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiAtPeaceCounter[eIndex] = iNewValue;
	FAssert(AI_getAtPeaceCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeAtPeaceCounter(TeamTypes eIndex, int iChange)
{
	AI_setAtPeaceCounter(eIndex, (AI_getAtPeaceCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getHasMetCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiHasMetCounter[eIndex];
}


void CvTeamAI::AI_setHasMetCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiHasMetCounter[eIndex] = iNewValue;
	FAssert(AI_getHasMetCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeHasMetCounter(TeamTypes eIndex, int iChange)
{
	AI_setHasMetCounter(eIndex, (AI_getHasMetCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getOpenBordersCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiOpenBordersCounter[eIndex];
}


void CvTeamAI::AI_setOpenBordersCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiOpenBordersCounter[eIndex] = iNewValue;
	FAssert(AI_getOpenBordersCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeOpenBordersCounter(TeamTypes eIndex, int iChange)
{
	AI_setOpenBordersCounter(eIndex, (AI_getOpenBordersCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getDefensivePactCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiDefensivePactCounter[eIndex];
}


void CvTeamAI::AI_setDefensivePactCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiDefensivePactCounter[eIndex] = iNewValue;
	FAssert(AI_getDefensivePactCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeDefensivePactCounter(TeamTypes eIndex, int iChange)
{
	AI_setDefensivePactCounter(eIndex, (AI_getDefensivePactCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getShareWarCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiShareWarCounter[eIndex];
}


void CvTeamAI::AI_setShareWarCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiShareWarCounter[eIndex] = iNewValue;
	FAssert(AI_getShareWarCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeShareWarCounter(TeamTypes eIndex, int iChange)
{
	AI_setShareWarCounter(eIndex, (AI_getShareWarCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getWarSuccess(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiWarSuccess[eIndex];
}


void CvTeamAI::AI_setWarSuccess(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiWarSuccess[eIndex] = iNewValue;
	FAssert(AI_getWarSuccess(eIndex) >= 0);
}


void CvTeamAI::AI_changeWarSuccess(TeamTypes eIndex, int iChange)
{
	AI_setWarSuccess(eIndex, (AI_getWarSuccess(eIndex) + iChange));
	// <advc.130m>
	if(iChange <= 0 || eIndex == BARBARIAN_TEAM)
		return;
	for(int i = 0; i < MAX_CIV_TEAMS; i++)
	{
		CvTeamAI& kWarAlly = GET_TEAM((TeamTypes)i);
		if(!kWarAlly.isAlive() || eIndex == kWarAlly.getID() || kWarAlly.getID() == getID())
			continue;
		// Let our allies know that we've had a war success
		if(kWarAlly.isAtWar(eIndex) && !kWarAlly.isAtWar(getID()))
			kWarAlly.AI_reportSharedWarSuccess(iChange, getID(), eIndex);
		/*  Let the allies of our enemy know that their ally has suffered a loss
			from us, their shared enemy */
		if(!kWarAlly.isAtWar(eIndex) && kWarAlly.isAtWar(getID()))
			kWarAlly.AI_reportSharedWarSuccess(iChange, eIndex, getID());
	}
}

/*  eEnemy is a war enemy that this team and eWarAlly have in common. Either eWarAlly
	has inflicted a war success on eEnemy or vice versa.
	This team is being informed about the war success, and iIntensity says how
	significant the war success was. */
void CvTeamAI::AI_reportSharedWarSuccess(int iIntensity, TeamTypes eWarAlly,
	TeamTypes eEnemy, // (doesn't currently matter)
	// True means: don't check if this team needs the assistance
	bool bIgnoreDistress)
{
	/*  War success against us as a measure of how distressed we are, i.e. how
		much we need the assistance from agentId. Counts all our enemies,
		not just enemyId. */
	double distress = 0;
	double const maxDistress = GC.getWAR_SUCCESS_CITY_CAPTURING() / 5.0;
	if(getNumCities() > 0)
	{
		if(bIgnoreDistress) // Not "ignored", I guess, but computed differently
			// The divisor is half the number of cities, rounded up
			distress = maxDistress / ((int)(getNumCities() / 2.0 + 0.5));
		else
		{
			// advc.test: To assert that eEnemy is among our enemies
			//std::set<TeamTypes> ourEnemies;
			for(int i = 0; i < MAX_CIV_TEAMS; i++)
			{
				CvTeamAI const& kLoopEnemy = GET_TEAM((TeamTypes)i);
				if(kLoopEnemy.isAlive() && !kLoopEnemy.isMinorCiv() &&
					isAtWar(kLoopEnemy.getID()))
				{
					//ourEnemies.insert(kLoopEnemy.getID()); // advc.test
					distress += kLoopEnemy.AI_getWarSuccess(getID());
				}
			}
			//FAssert(ourEnemies.count(eEnemy) > 0 || eEnemy == BARBARIAN_TEAM || GET_TEAM(eEnemy).isMinorCiv()); // advc.test
			distress /= getNumCities();
		}
	}
	/*  Don't give distress too much weight. It's mostly there to discount
		unwelcome assistance when allies just snatch away our loot.
		Assuming that killing the defenders in a city results in about as much
		war success as taking the city itself, the highest possible distress is
		2 * WAR_SUCCESS_CITY_CAPTURING * NumCities. If our distress is just 10%
		of that, the distress multiplier already takes its maximal value. */
	distress = std::min(distress, maxDistress);
	int iOldValue = AI_getSharedWarSuccess(eWarAlly);
	// Asymptote at 5000
	double brakeFactor = std::max(0.0, 1 - iOldValue / 5000.0);
	int iNewValue = ::round(iOldValue + brakeFactor *
			100.0 * distress * iIntensity / // Times 100 for accuracy
			/*  Use number of cities as an indicator of how capable the war ally is
				militarily - how difficult was this war success to accomplish, or
				how big a sacrifice was the loss. */
			std::max(1, GET_TEAM(eWarAlly).getNumCities()));
	FAssert(iNewValue >= iOldValue);
	AI_setSharedWarSuccess(eWarAlly, iNewValue);
}

/*  The war success of our war ally against a shared enemy, plus the war success
	of shared enemies against our war ally. This is quite different from AI_getWarSuccess,
	which counts our success against team eIndex. Also on a different scale. */
int CvTeamAI::AI_getSharedWarSuccess(TeamTypes eWarAlly) const
{
	return m_aiSharedWarSuccess[eWarAlly];
}


void CvTeamAI::AI_setSharedWarSuccess(TeamTypes eWarAlly, int iWS)
{
	m_aiSharedWarSuccess[eWarAlly] = iWS;
} // </advc.130m>

/*  <advc.130n> Game turn on which eReligion was first encountered by this team;
	-1 if never. */
int CvTeamAI::AI_getReligionKnownSince(ReligionTypes eReligion) const
{
	std::map<ReligionTypes,int>::const_iterator pos = m_religionKnownSince.find(eReligion);
	if(pos == m_religionKnownSince.end())
		return -1;
	return pos->second;
}

// Report encounter with a religion; callee will check if it's the first encounter.
void CvTeamAI::AI_reportNewReligion(ReligionTypes eReligion)
{
	std::map<ReligionTypes,int>::const_iterator pos = m_religionKnownSince.find(eReligion);
	if(pos != m_religionKnownSince.end())
		return;
	m_religionKnownSince.insert(std::make_pair(eReligion, GC.getGame().getGameTurn()));
}// </advc.130n>

int CvTeamAI::AI_getEnemyPeacetimeTradeValue(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiEnemyPeacetimeTradeValue[eIndex];
}


void CvTeamAI::AI_setEnemyPeacetimeTradeValue(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiEnemyPeacetimeTradeValue[eIndex] = iNewValue;
	FAssert(AI_getEnemyPeacetimeTradeValue(eIndex) >= 0);
	// K-Mod. update attitude
	for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
	{
		CvPlayerAI& kPlayer_i = GET_PLAYER(i);
		if (kPlayer_i.getTeam() == getID())
		{
			for (PlayerTypes j = (PlayerTypes)0; j < MAX_CIV_PLAYERS; j=(PlayerTypes)(j+1))
			{
				if (GET_PLAYER(j).getTeam() == eIndex)
				{
					kPlayer_i.AI_updateAttitudeCache(j/* advc.130e: */, false);
				}
			}
		}
	}
	// K-Mod end
}


void CvTeamAI::AI_changeEnemyPeacetimeTradeValue(TeamTypes eIndex, int iChange)
{
	AI_setEnemyPeacetimeTradeValue(eIndex, (AI_getEnemyPeacetimeTradeValue(eIndex) + iChange));
}

// <advc.130p><advc.130m> To keep the rate consistent between TeamAI and PlayerAI
double CvTeamAI::AI_getDiploDecay() const
{
	/*  On Normal speed, this decay rate halves a value in about 50 turns:
		0.9865^50 = 0.507 */
	return 1.45 / GC.getInfo(GC.getGame().getGameSpeedType()).
			getGoldenAgePercent();
} // </advc.130m>

// Needed for both RivalTrade and "fair trade"
double CvTeamAI::AI_recentlyMetMultiplier(TeamTypes eOther) const
{
	double recency = std::min(1.0, AI_getHasMetCounter(eOther) /
			((double)GC.getInfo(GC.getGame().getGameSpeedType()).
			getResearchPercent()));
	// +50% if just met, declining linearly to +0% if met 100 turns ago (Normal speed)
	return 1 + 0.5 * (1 - recency);
} // </advc.130p>


int CvTeamAI::AI_getEnemyPeacetimeGrantValue(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiEnemyPeacetimeGrantValue[eIndex];
}


void CvTeamAI::AI_setEnemyPeacetimeGrantValue(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiEnemyPeacetimeGrantValue[eIndex] = iNewValue;
	FAssert(AI_getEnemyPeacetimeGrantValue(eIndex) >= 0);
	// K-Mod. update attitude
	for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
	{
		CvPlayerAI& kPlayer_i = GET_PLAYER(i);
		if (kPlayer_i.getTeam() == getID())
		{
			for (PlayerTypes j = (PlayerTypes)0; j < MAX_CIV_PLAYERS; j=(PlayerTypes)(j+1))
			{
				if (GET_PLAYER(j).getTeam() == eIndex)
				{
					kPlayer_i.AI_updateAttitudeCache(j,
							false); // advc.130e: Else infinite recursion possible
				}
			}
		}
	}
	// K-Mod end
}


void CvTeamAI::AI_changeEnemyPeacetimeGrantValue(TeamTypes eIndex, int iChange)
{
	AI_setEnemyPeacetimeGrantValue(eIndex, (AI_getEnemyPeacetimeGrantValue(eIndex) + iChange));
}

// advc: Body cut from the deleted CvTeam::getChosenWarCount. Never called, not from Python either.
int CvTeamAI::AI_countChosenWars(bool bIgnoreMinors) const
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


bool CvTeamAI::AI_isChosenWar(TeamTypes eIndex) const
{
	return (AI_isChosenWarPlan( // advc.105: Code moved into AI_isChosenWarPlan
			AI_getWarPlan(/* advc.104j: */ GET_TEAM(eIndex).getMasterTeam())));
}

// advc.105: Body cut from AI_isChosenWar
bool CvTeamAI::AI_isChosenWarPlan(WarPlanTypes eWarPlanType)
{
	switch(eWarPlanType)
	{
	case WARPLAN_ATTACKED_RECENT:
	case WARPLAN_ATTACKED:
		return false;
	case WARPLAN_PREPARING_LIMITED:
	case WARPLAN_PREPARING_TOTAL:
	case WARPLAN_LIMITED:
	case WARPLAN_TOTAL:
	case WARPLAN_DOGPILE:
		return true;
	default: return false; // NO_WARPLAN
	}
}

// <advc.105>
bool CvTeamAI::AI_isAnyChosenWar() const
{
	for (int i = 0; i < NUM_WARPLAN_TYPES; i++)
	{
		WarPlanTypes eWarPlanType = (WarPlanTypes)i;
		if (AI_isChosenWarPlan(eWarPlanType) && AI_getNumWarPlans(eWarPlanType) > 0)
			return true;
	}
	return false;
} // </advc.105>

/*  advc: Body based on the deleted CvTeam::getWarPlanCount. (War plans are an AI thing.)
	NUM_WARPLAN_TYPES causes all war plans (except NO_WARPLAN) to be counted.
	Barbarians and vassals are never counted. (BtS had counted vassals.) */
int CvTeamAI::AI_countWarPlans(WarPlanTypes eWarPlanType, bool bIgnoreMinors, unsigned int iMaxCount) const
{
	FAssertMsg(!bIgnoreMinors || eWarPlanType == NO_WARPLAN || iMaxCount != MAX_PLAYERS || (eWarPlanType == NUM_WARPLAN_TYPES && iMaxCount > 1),
			"Should use getNumWarPlans or isAnyWarPlan instead (faster)");
	int r = 0;
	for (int i = 0; i < MAX_CIV_TEAMS; i++)
	{
		CvTeam const& kTarget = GET_TEAM((TeamTypes)i);
		if (!kTarget.isAlive() || kTarget.isAVassal())
			continue;
		if (!bIgnoreMinors || !kTarget.isMinorCiv())
		{
			WarPlanTypes eLoopWarPlanType = AI_getWarPlan(kTarget.getID());
			if (eWarPlanType == NUM_WARPLAN_TYPES ?
					(eLoopWarPlanType != NO_WARPLAN) :
					(eLoopWarPlanType == eWarPlanType))
				r++;
			if (r >= (int)iMaxCount)
				return r;
		}
	}
	FAssert(eWarPlanType != NUM_WARPLAN_TYPES || r >= getNumWars(bIgnoreMinors, true));
	return r;
}


bool CvTeamAI::AI_isSneakAttackReady(TeamTypes eIndex) const
{
	//return (AI_isChosenWar(eIndex) && !(AI_isSneakAttackPreparing(eIndex))); // BtS
	// K-Mod  (advc: K-Mod code from an overloaded function)
	if(eIndex != NO_TEAM)
		return !isAtWar(eIndex) && AI_isChosenWar(eIndex) && !AI_isSneakAttackPreparing(eIndex); // K-Mod
	for (int i = 0; i < MAX_CIV_TEAMS; i++) {
		if (AI_isSneakAttackReady((TeamTypes)i))
			return true;
	}
	return false;
	// K-Mod end
}


bool CvTeamAI::AI_isSneakAttackPreparing(TeamTypes eIndex) const
{
	if(eIndex != NO_TEAM) // advc
	{
		WarPlanTypes eWarPlan = AI_getWarPlan(GET_TEAM(eIndex).getMasterTeam()); // advc.104j
		return (eWarPlan == WARPLAN_PREPARING_LIMITED || eWarPlan == WARPLAN_PREPARING_TOTAL);
	} // <advc>
	for (int i = 0; i < MAX_CIV_TEAMS; i++)
	{
		if (AI_isSneakAttackPreparing((TeamTypes)i))
			return true;
	}
	return false; // </advc>
}


void CvTeamAI::AI_setWarPlan(TeamTypes eIndex, WarPlanTypes eNewValue, bool bWar)  // advc: style changes
{
	FASSERT_BOUNDS(0, MAX_TEAMS, eIndex, "CvTeamAI::AI_setWarPlan");
	FAssert(eIndex != getID() || eNewValue == NO_WARPLAN); // advc: Moved from AI_getWarPlan
	if(AI_getWarPlan(eIndex) == eNewValue || (!bWar && isAtWar(eIndex)))
		return;
	AI_updateWarPlanCounts(eIndex, m_aeWarPlan[eIndex], eNewValue); // advc.opt
	m_aeWarPlan[eIndex] = eNewValue;
	AI_setWarPlanStateCounter(eIndex, 0);
	AI_updateAreaStrategies();
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayerAI& kOurAIMember = GET_PLAYER((PlayerTypes)i);
		if(kOurAIMember.isAlive() && kOurAIMember.getTeam() == getID() &&
				!kOurAIMember.isHuman())
			kOurAIMember.AI_makeProductionDirty();
	}
	// <advc.104j>
	if(isHuman()) // Human has to instruct vassals manually
		return;
	WarPlanTypes eVassalWP = NO_WARPLAN;
	if(eNewValue == NO_WARPLAN)
		eVassalWP = NO_WARPLAN;
	else if(eNewValue == WARPLAN_PREPARING_LIMITED || eNewValue == WARPLAN_PREPARING_TOTAL)
		eVassalWP = WARPLAN_PREPARING_LIMITED;
	else return;
	for(int i = 0; i < MAX_CIV_TEAMS; i++)
	{
		CvTeamAI& kOurAIVassal = GET_TEAM((TeamTypes)i);
		if(kOurAIVassal.isAlive() && kOurAIVassal.isVassal(getID()) &&
				!kOurAIVassal.isHuman() &&
				// Don't set NO_WARPLAN before the vassal has been set to !isAtWar
				(eVassalWP != NO_WARPLAN || !kOurAIVassal.isAtWar(eIndex)))
			kOurAIVassal.AI_setWarPlan(eIndex, eVassalWP);
	} // </advc.104j>
}

// BETTER_BTS_AI_MOD, General AI, 07/20/09, jdog5000: START
/*  advc: Moved these two functions from CvTeam and refactored them
	(they were using a loop instead of getMasterTeam) */
bool CvTeamAI::AI_isMasterPlanningLandWar(CvArea* pArea) const
{
	if (!isAVassal())
		return false;
	AreaAITypes eAreaAI = pArea->getAreaAIType(getID());
	if (eAreaAI == AREAAI_OFFENSIVE || eAreaAI == AREAAI_DEFENSIVE || eAreaAI == AREAAI_MASSING)
		return true;

	CvTeamAI const& kMaster = GET_TEAM(getMasterTeam());
	if (kMaster.AI_isAnyWarPlan())
	{
		AreaAITypes eMasterAreaAI = pArea->getAreaAIType(kMaster.getID());
		if (eMasterAreaAI == AREAAI_OFFENSIVE || eMasterAreaAI == AREAAI_DEFENSIVE ||
				eMasterAreaAI == AREAAI_MASSING)
			return true;
		else if (eMasterAreaAI == AREAAI_NEUTRAL)
		{
			// Master has no presence here
			if (pArea->getNumCities() - countNumCitiesByArea(pArea) > 2)
				return (GC.getGame().getSorenRandNum(isCapitulated() ? 6 : 4, "Vassal land war") == 0);
		}
	}
	else if (kMaster.isHuman())
	{
		static bool const bBBAI_HUMAN_VASSAL_WAR_BUILD = GC.getDefineBOOL("BBAI_HUMAN_VASSAL_WAR_BUILD");
		if (bBBAI_HUMAN_VASSAL_WAR_BUILD &&
				pArea->getNumCities() - countNumCitiesByArea(pArea) -
				kMaster.countNumCitiesByArea(pArea) > 2)
			return (GC.getGame().getSorenRandNum(4, "Vassal land war") == 0);
	}
	return false;
}


bool CvTeamAI::AI_isMasterPlanningSeaWar(CvArea* pArea) const
{
	if (!isAVassal())
		return false;
	AreaAITypes eAreaAI = pArea->getAreaAIType(getID());
	if (eAreaAI == AREAAI_ASSAULT || eAreaAI == AREAAI_ASSAULT_ASSIST || eAreaAI == AREAAI_ASSAULT_MASSING)
		return true;

	CvTeamAI const& kMaster = GET_TEAM(getMasterTeam());
	if (kMaster.AI_isAnyWarPlan())
	{
		AreaAITypes eMasterAreaAI = pArea->getAreaAIType(kMaster.getID());
		if (eMasterAreaAI == AREAAI_ASSAULT || eMasterAreaAI == AREAAI_ASSAULT_ASSIST ||
				eMasterAreaAI == AREAAI_ASSAULT_MASSING)
			return (GC.getGame().getSorenRandNum(isCapitulated() ? 3 : 2, "Vassal sea war") == 0);
	}
	return false;
} // BETTER_BTS_AI_MOD: END

// <advc.104>
void CvTeamAI::AI_setWarPlanNoUpdate(TeamTypes eIndex, WarPlanTypes eNewValue)
{
	FASSERT_BOUNDS(0, MAX_TEAMS, eIndex, "CvTeamAI::AI_setWarPlanNoUpdate");
	AI_updateWarPlanCounts(eIndex, m_aeWarPlan[eIndex], eNewValue); // advc.opt
	m_aeWarPlan[eIndex] = eNewValue;
} // </advc.104>

// <advc.opt>
void CvTeamAI::AI_updateWarPlanCounts(TeamTypes eTarget, WarPlanTypes eOldPlan, WarPlanTypes eNewPlan)
{
	if (eTarget == BARBARIAN_TEAM || eOldPlan == eNewPlan)
		return;
	CvTeam const& kTarget = GET_TEAM(eTarget);
	// (Don't exclude dead teams - CvTeam::changeAliveCount will set NO_WARPLAN for those.)
	if (kTarget.isMinorCiv() || kTarget.isAVassal())
		return;
	if (eOldPlan != NO_WARPLAN)
		m_aiWarPlanCounts[eOldPlan]--;
	if (eNewPlan != NO_WARPLAN)
	{
		m_aiWarPlanCounts[eNewPlan]++;
		m_bAnyWarPlan = true;
		return;
	}
	for (int i = 0; i < NUM_WARPLAN_TYPES; i++)
	{
		if (m_aiWarPlanCounts[i] > 0)
		{
			m_bAnyWarPlan = true;
			return;
		}
	}
	m_bAnyWarPlan = false;
} // </advc.opt>

//if this number is over 0 the teams are "close"
//this may be expensive to run, kinda O(N^2)...
int CvTeamAI::AI_teamCloseness(TeamTypes eIndex, int iMaxDistance,
		bool bConsiderLandTarget, // advc.104o
		bool bConstCache) const // advc.001n
{
	//PROFILE_FUNC(); // advc.003o (the cache seems to be very effective)

	if (iMaxDistance == -1)
		iMaxDistance = DEFAULT_PLAYER_CLOSENESS;

	FAssert(eIndex != getID());
	int iValue = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++) // advc: style changes
	{
		CvPlayerAI const& kMember = GET_PLAYER((PlayerTypes)iI);
		if (!kMember.isAlive() || kMember.getTeam() != getID())
			continue;

		for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
		{
			CvPlayer const& kOtherMember = GET_PLAYER((PlayerTypes)iJ);
			if (!kOtherMember.isAlive() || kOtherMember.getTeam() != eIndex)
				continue;

			iValue += kMember.AI_playerCloseness(kOtherMember.getID(), iMaxDistance,
					bConstCache); // advc.001n
		}
	} /* <advc.104o> (Change disabled for now b/c advc.107 now factors
		land connection into AI_playerCloseness. Could increase iValue here
		in order to further increase the impact.) */
	/*if(bConsiderLandTarget && AI_isLandTarget(eIndex))
		iValue += 50; */ // </advc.104o>
	return iValue;
}


void CvTeamAI::read(FDataStreamBase* pStream)
{
	CvTeam::read(pStream);

	uint uiFlag=0;
	pStream->Read(&uiFlag);

	pStream->Read(MAX_TEAMS, m_aiWarPlanStateCounter);
	pStream->Read(MAX_TEAMS, m_aiAtWarCounter);
	pStream->Read(MAX_TEAMS, m_aiAtPeaceCounter);
	pStream->Read(MAX_TEAMS, m_aiHasMetCounter);
	pStream->Read(MAX_TEAMS, m_aiOpenBordersCounter);
	pStream->Read(MAX_TEAMS, m_aiDefensivePactCounter);
	pStream->Read(MAX_TEAMS, m_aiShareWarCounter);
	pStream->Read(MAX_TEAMS, m_aiWarSuccess);
	pStream->Read(MAX_TEAMS, m_aiSharedWarSuccess); // advc.130m
	// <advc.130n>
	int iReligions;
	pStream->Read(&iReligions);
	for(int i = 0; i < iReligions; i++)
	{
		int first; int second;
		pStream->Read(&first);
		pStream->Read(&second);
		m_religionKnownSince.insert(std::make_pair((ReligionTypes)first, second));
	} // </advc.130n>
	pStream->Read(MAX_TEAMS, m_aiEnemyPeacetimeTradeValue);
	pStream->Read(MAX_TEAMS, m_aiEnemyPeacetimeGrantValue);
	// <advc.opt>
	if (uiFlag >= 3)
	{
		pStream->Read(NUM_WARPLAN_TYPES, (int*)m_aiWarPlanCounts);
		pStream->Read(&m_bAnyWarPlan);
	} // </advc.opt>
	pStream->Read(MAX_TEAMS, (int*)m_aeWarPlan);
	pStream->Read((int*)&m_eWorstEnemy);
	// <advc.109>
	if(uiFlag >= 2)
		pStream->Read(&m_bLonely); // </advc.109>
	// K-Mod
	m_aiStrengthMemory.resize(GC.getMap().numPlots(), 0);
	FAssert(m_aiStrengthMemory.size() > 0);
	if (uiFlag >= 1)
		pStream->Read(m_aiStrengthMemory.size(), &m_aiStrengthMemory[0]);
	// K-Mod end
	// <advc.104>
	if(isEverAlive() && !isBarbarian() && !isMinorCiv())
		m_pWpai->read(pStream); // </advc.104>
	// <advc.opt> Once alive status has been set for all teams ...
	if(uiFlag < 3 && getID() == MAX_TEAMS - 1)
	{
		for(int i = 0; i < MAX_TEAMS; i++)
		{
			CvTeamAI& kTeam = GET_TEAM((TeamTypes)i);
			for (int j = 0; j < NUM_WARPLAN_TYPES; j++)
			{
				kTeam.m_aiWarPlanCounts[j] = kTeam.AI_countWarPlans((WarPlanTypes)j,
						true, MAX_CIV_PLAYERS); // Last argument just to avoid a failed assertion
				if (kTeam.m_aiWarPlanCounts[j] > 0)
					kTeam.m_bAnyWarPlan = true;
			}
		}
	} // </advc.opt>
}


void CvTeamAI::write(FDataStreamBase* pStream)
{
	PROFILE_FUNC(); // advc
	CvTeam::write(pStream);

	uint uiFlag=1;
	uiFlag = 2; // advc.109
	uiFlag = 3; // advc.opt: m_aiWarPlanCounts
	pStream->Write(uiFlag);

	pStream->Write(MAX_TEAMS, m_aiWarPlanStateCounter);
	pStream->Write(MAX_TEAMS, m_aiAtWarCounter);
	pStream->Write(MAX_TEAMS, m_aiAtPeaceCounter);
	pStream->Write(MAX_TEAMS, m_aiHasMetCounter);
	pStream->Write(MAX_TEAMS, m_aiOpenBordersCounter);
	pStream->Write(MAX_TEAMS, m_aiDefensivePactCounter);
	pStream->Write(MAX_TEAMS, m_aiShareWarCounter);
	pStream->Write(MAX_TEAMS, m_aiWarSuccess);
	pStream->Write(MAX_TEAMS, m_aiSharedWarSuccess); // advc.130m
	// <advc.130n>
	pStream->Write((int)m_religionKnownSince.size());
	for(std::map<ReligionTypes,int>::const_iterator it = m_religionKnownSince.begin();
		it != m_religionKnownSince.end(); it++)
	{
		pStream->Write(it->first);
		pStream->Write(it->second);
	} // </advc.130n>
	pStream->Write(MAX_TEAMS, m_aiEnemyPeacetimeTradeValue);
	pStream->Write(MAX_TEAMS, m_aiEnemyPeacetimeGrantValue);

	pStream->Write(NUM_WARPLAN_TYPES, (int*)m_aiWarPlanCounts); // advc.opt
	pStream->Write(m_bAnyWarPlan); // advc.opt
	pStream->Write(MAX_TEAMS, (int*)m_aeWarPlan);
	pStream->Write(m_eWorstEnemy);
	pStream->Write(m_bLonely); // advc.109

	// K-Mod.
	FAssert(m_aiStrengthMemory.size() == GC.getMap().numPlots());
	m_aiStrengthMemory.resize(GC.getMap().numPlots()); // the consequences of the assert failing are really bad.
	FAssert(m_aiStrengthMemory.size() > 0);
	pStream->Write(m_aiStrengthMemory.size(), &m_aiStrengthMemory[0]); // uiFlag >= 1
	// K-Mod end
	// <advc.104>
	if(isEverAlive() && !isBarbarian() && !isMinorCiv())
		m_pWpai->write(pStream); // </advc.104>
}

// <advc.012>
int CvTeamAI::AI_plotDefense(CvPlot const& kPlot, bool bIgnoreBuilding,
	bool bGarrisonStrength) const // advc.500b
{
	TeamTypes eAttacker = NO_TEAM;
	/*  We could also be attacked in p by a second war enemy that doesn't own the
		plot; impossible to predict. An attack by the plot owner is far more likely
		though. */
	if(kPlot.getOwner() != NO_PLAYER && GET_TEAM(getID()).isAtWar(kPlot.getTeam()))
		eAttacker = kPlot.getTeam();
	return kPlot.defenseModifier(getID(), bIgnoreBuilding, eAttacker,
			bGarrisonStrength); // advc.500b
} // </advc.012>

// <advc.130y> ('bFreed' is unused; not needed after all, I guess.)
void CvTeamAI::AI_forgiveEnemy(TeamTypes eEnemyTeam, bool bCapitulated, bool bFreed)
{
	/*  'capitulated' refers to us, the callee. This function is called when
		making peace but also when breaking free. Can therefore not rely on
		this->isCapitulated (but GET_TEAM(eEnemyTeam).isCapitulated() is fine).
		If we make peace after having broken free, it's called twice for each
		former enemy in total. */
	int iDelta = 0;
	bCapitulated = (bCapitulated || GET_TEAM(eEnemyTeam).isCapitulated());
	if(bCapitulated)
		iDelta--;
	int ws = GET_TEAM(eEnemyTeam).AI_getWarSuccess(getID());
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayerAI& kMember = GET_PLAYER((PlayerTypes)i);
		if(!kMember.isAlive() || kMember.getTeam() != getID())
			continue;
		// No vassal-related forgiveness when war success high
		iDelta = std::min(0, iDelta + ws / kMember.warSuccessAttitudeDivisor());
		for(int j = 0; j < MAX_CIV_PLAYERS; j++)
		{
			PlayerTypes eOtherCiv = (PlayerTypes)j;
			if(!GET_PLAYER(eOtherCiv).isAlive())
				continue;
			// <advc.104i> Be willing to talk to everyone, not just 'enemyId'.
			kMember.AI_setMemoryCount(eOtherCiv, MEMORY_DECLARED_WAR_RECENT, 0);
			// </advc.104i>
			// <advc.130f> Cap at 1
			if(kMember.AI_getMemoryCount(eOtherCiv, MEMORY_STOPPED_TRADING_RECENT) > 1)
				kMember.AI_setMemoryCount(eOtherCiv, MEMORY_STOPPED_TRADING_RECENT, 1);
			// </advc.130f>
			CvPlayer const& kEnemyMember = GET_PLAYER(eOtherCiv);
			if(kEnemyMember.getTeam() != eEnemyTeam)
				continue;
			int iLimit = -kMember.AI_getMemoryCount(kEnemyMember.getID(), MEMORY_DECLARED_WAR);
			int iDeltaLoop = iDelta;
			/*  Forgiveness if war success small, but only if memory high and
				no other forgiveness condition applies, and not (times 0) in the
				Ancient era (attacks on Workers and Settlers). */
			if(iLimit <= -3 && iDelta >= 0 && ws < 0.3 * getCurrentEra() *
					GC.getWAR_SUCCESS_CITY_CAPTURING())
				iDeltaLoop--;
			// No complete forgiveness unless capitulated
			if(!bCapitulated && iLimit < 0)
				iLimit++;
			int iChange = std::min(0, std::max(iDeltaLoop, iLimit));
			if(iChange != 0)
				kMember.AI_changeMemoryCount(kEnemyMember.getID(),
						MEMORY_DECLARED_WAR, iChange);
			if(bCapitulated) // Directly willing to sign OB
			{
				kMember.AI_setMemoryCount(kEnemyMember.getID(),
						MEMORY_CANCELLED_OPEN_BORDERS, 0);
			} // <advc.134a>
			int iContactPeace = kMember.AI_getContactTimer(kEnemyMember.getID(), CONTACT_PEACE_TREATY);
			if(iContactPeace != 0) // for debugger stop
				kMember.AI_changeContactTimer(kEnemyMember.getID(), CONTACT_PEACE_TREATY, -iContactPeace);
			// </advc.134a>
		}
	}
}

void CvTeamAI::AI_thankLiberator(TeamTypes eLiberator)
{
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayerAI& kMember = GET_PLAYER((PlayerTypes)i);
		if(!kMember.isAlive() || kMember.getTeam() != getID())
			continue;
		int iWSDivisor = kMember.warSuccessAttitudeDivisor();
		for(int j = 0; j < MAX_CIV_PLAYERS; j++)
		{
			CvPlayerAI& kLiberatorMember = GET_PLAYER((PlayerTypes)j);
			if(!kLiberatorMember.isAlive() || kLiberatorMember.getTeam() != eLiberator)
				continue;
			int iMemory = std::max(0, 2 -
					GET_TEAM(eLiberator).AI_getWarSuccess(getID()) / iWSDivisor);
			kMember.AI_changeMemoryCount(kLiberatorMember.getID(), MEMORY_INDEPENDENCE,
					2 * iMemory); // advc.130j: doubled
		}
	}
} // </advc.130y>

// <advc.115b> <advc.104>
VoteSourceTypes CvTeamAI::AI_getLatestVictoryVoteSource() const
{
	VoteSourceTypes r = NO_VOTESOURCE;
	CvGame& g = GC.getGame();
	for(int i = 0; i < GC.getNumVoteSourceInfos(); i++)
	{
		VoteSourceTypes eVS = (VoteSourceTypes)i;
		if(g.isDiploVote(eVS))
		{
			r = eVS;
			if(g.isTeamVoteEligible(getID(), eVS) &&
					GC.getInfo(eVS).getVoteInterval() < 7)
				break;
		}
	}
	return r;
} // </advc.104>

bool CvTeamAI::AI_isAnyCloseToReligiousVictory() const
{
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayerAI const& kMember = GET_PLAYER((PlayerTypes)i);
		if(kMember.isAlive() && kMember.getTeam() == getID() &&
				kMember.isCloseToReligiousVictory())
			return true;
	}
	return false;
} //</advc.115b>

// K-Mod - AI tactical memory.
// The AI uses this to remember how strong the enemy defence is at particular plots.
// NOTE: AI_setStrengthMemory should not be used by human players - because it may cause OOS errors.
int CvTeamAI::AI_getStrengthMemory(int x, int y) const
{
	FAssert(m_aiStrengthMemory.size() == GC.getMap().numPlots());
	return m_aiStrengthMemory[GC.getMap().plotNum(x, y)];
}

void CvTeamAI::AI_setStrengthMemory(int x, int y, int value)
{
	FAssert(m_aiStrengthMemory.size() == GC.getMap().numPlots());
	m_aiStrengthMemory[GC.getMap().plotNum(x, y)] = value;
}
// <advc.make> Was inlined in CvTeamAI.h
int CvTeamAI::AI_getStrengthMemory(const CvPlot* pPlot)
{
	//return AI_getStrengthMemory(pPlot->getX(), pPlot->getY());
	// To make sure that it won't be slower than before
	return m_aiStrengthMemory[GC.getMap().plotNum(pPlot->getX(), pPlot->getY())];
}

void CvTeamAI::AI_setStrengthMemory(const CvPlot* pPlot, int value)
{
	//AI_setStrengthMemory(pPlot->getX(), pPlot->getY(), value);
	m_aiStrengthMemory[GC.getMap().plotNum(pPlot->getX(), pPlot->getY())] = value;
} // </advc.make>

void CvTeamAI::AI_updateStrengthMemory()
{
	PROFILE_FUNC();

	if (!isAlive() || isHuman() || isMinorCiv() || isBarbarian())
		return;

	FAssert(m_aiStrengthMemory.size() == GC.getMap().numPlots());
	for (int i = 0; i < GC.getMap().numPlots(); i++)
	{
		if (m_aiStrengthMemory[i] == 0)
			continue;

		CvPlot const& kPlot = GC.getMap().getPlotByIndex(i);
		if (kPlot.isVisible(getID(), false) && !kPlot.isVisibleEnemyUnit(getLeaderID()))
			m_aiStrengthMemory[i] = 0;
		else
			m_aiStrengthMemory[i] = 96 * m_aiStrengthMemory[i] / 100; // reduce by 4%, rounding down. (arbitrary number)
	}
}
// K-Mod end

/*  advc: Renamed variable iRand to iValue and this function and the next few
	(probably copy-paste errors) */
int CvTeamAI::AI_noTechTradeThreshold() const
{
	int iValue = 0;
	int iCount = 0;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iValue += GC.getInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getNoTechTradeThreshold();
				iCount++;
			}
		}
	}

	if (iCount > 1)
		iValue /= iCount;

	return iValue;
}


int CvTeamAI::AI_techTradeKnownPercent() const
{
	int iValue = 0; // advc: was "iRand"
	int iCount = 0;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iValue += GC.getInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getTechTradeKnownPercent();
				iCount++;
			}
		}
	}

	if (iCount > 1)
		iValue /= iCount;

	return iValue;
}


int CvTeamAI::AI_maxWarRand() const
{
	int iRand = 0;
	int iCount = 0;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getMaxWarRand();
				iCount++;
			}
		}
	}

	if (iCount > 1)
		iRand /= iCount;

	return iRand;
}


int CvTeamAI::AI_maxWarNearbyPowerRatio() const
{
	int iValue = 0; // advc: was "iRand"
	int iCount = 0;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iValue += GC.getInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getMaxWarNearbyPowerRatio();
				iCount++;
			}
		}
	}

	if (iCount > 1)
		iValue /= iCount;

	return iValue;
}


int CvTeamAI::AI_maxWarDistantPowerRatio() const
{
	int iValue = 0;
	int iCount = 0;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iValue += GC.getInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getMaxWarDistantPowerRatio();
				iCount++;
			}
		}
	}

	if (iCount > 1)
		iValue /= iCount;

	return iValue;
}


int CvTeamAI::AI_maxWarMinAdjacentLandPercent() const
{
	int iValue = 0;
	int iCount = 0;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iValue += GC.getInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getMaxWarMinAdjacentLandPercent();
				iCount++;
			}
		}
	}

	if (iCount > 1)
		iValue /= iCount;

	return iValue;
}


int CvTeamAI::AI_limitedWarRand() const
{
	int iRand = 0;
	int iCount = 0;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getLimitedWarRand();
				iCount++;
			}
		}
	}

	if (iCount > 1)
		iRand /= iCount;

	return iRand;
}


int CvTeamAI::AI_limitedWarPowerRatio() const
{
	int iValue = 0;
	int iCount = 0;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iValue += GC.getInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getLimitedWarPowerRatio();
				iCount++;
			}
		}
	}

	if (iCount > 1)
		iValue /= iCount;

	return iValue;
}


int CvTeamAI::AI_dogpileWarRand() const
{
	int iRand = 0;
	int iCount = 0;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getDogpileWarRand();
				iCount++;
			}
		}
	}

	if (iCount > 1)
		iRand /= iCount;

	return iRand;
}


int CvTeamAI::AI_makePeaceRand() const
{
	int iRand = 0;
	int iCount = 0;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getMakePeaceRand();
				iCount++;
			}
		}
	}

	if (iCount > 1)
		iRand /= iCount;

	return iRand;
}


int CvTeamAI::AI_noWarAttitudeProb(AttitudeTypes eAttitude) const
{
	int iProb = 0;
	int iCount = 0;
	// BETTER_BTS_AI_MOD, War Strategy AI, 03/20/10, jdog5000: START
	int iVictoryStrategyAdjust = 0;
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iProb += GC.getInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getNoWarAttitudeProb(eAttitude);
				iCount++;

				// In final stages of miltaristic victory, AI may turn on its friends!
				if (GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST4))
				{
					iVictoryStrategyAdjust += 30;
				}
				else if (GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION4))
				{
					iVictoryStrategyAdjust += 20;
				}
			}
		}
	}
	if (iCount > 1)
	{
		iProb /= iCount;
		iVictoryStrategyAdjust /= iCount;
	}

	iProb = std::max(0, iProb - iVictoryStrategyAdjust);
	// BETTER_BTS_AI_MOD: END
	return iProb;
}

// <advc.104y>
int CvTeamAI::AI_noWarProbAdjusted(TeamTypes eOther) const
{
	AttitudeTypes eTowardThem = AI_getAttitude(eOther, true);
	int r = AI_noWarAttitudeProb(eTowardThem);
	if(r < 100 || isOpenBorders(eOther) || eTowardThem == ATTITUDE_FURIOUS)
		return r;
	return AI_noWarAttitudeProb((AttitudeTypes)(eTowardThem - 1));
} // </advc.104y>

bool CvTeamAI::AI_isAvoidWar(TeamTypes eOther) const
{
	return (!isHuman() && // advc.130u
			AI_noWarProbAdjusted(eOther) >= 100);
} // </advc.104y>

// <advc.130i>
int CvTeamAI::AI_getOpenBordersAttitudeDivisor() const
{
	int r = 0;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayer const& kMember = GET_PLAYER((PlayerTypes)i);
		if(kMember.isAlive() && kMember.getTeam() == getID())
		{
			r = std::max(r, GC.getInfo(kMember.getPersonalityType()).
					getOpenBordersAttitudeDivisor());
		}
	}
	return r;
}

double CvTeamAI::AI_OpenBordersCounterIncrement(TeamTypes eOther) const
{
	if(eOther == getID() || eOther == NO_TEAM)
	{
		FAssert(false);
		return 0;
	}
	int iTotalForeignTrade = 0;
	int iTradeFromThem = 0;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		CvPlayerAI const& kOurMember = GET_PLAYER((PlayerTypes)i);
		if(!kOurMember.isAlive() || kOurMember.getTeam() != getID())
			continue;
		// Based on calculateTradeRoutes in BUG's TradeUtil.py
		FOR_EACH_CITY(c, kOurMember)
		{
			for(int j = 0; j < c->getTradeRoutes(); j++)
			{
				CvCity* pPartnerCity = c->getTradeCity(j);
				if(pPartnerCity == NULL)
					continue;
				TeamTypes ePartnerTeam = pPartnerCity->getTeam();
				if(ePartnerTeam == NO_TEAM || ePartnerTeam == getID())
					continue;
				int iTradeCommerce = c->calculateTradeYield(YIELD_COMMERCE,
						c->calculateTradeProfit(pPartnerCity));
				iTotalForeignTrade += iTradeCommerce;
				if(ePartnerTeam == eOther)
					iTradeFromThem += iTradeCommerce;
			}
		}
	}
	double fromTrade = 0;
	if(iTotalForeignTrade > 0 && iTradeFromThem > 0)
		fromTrade = std::sqrt(iTradeFromThem / (double)iTotalForeignTrade);
	double fromCloseness = 0;
	int iOurCities = getNumCities();
	int iTheirCities = GET_TEAM(eOther).getNumCities();
	if(iOurCities > 0 && iTheirCities > 0)
		fromCloseness = AI_teamCloseness(eOther, DEFAULT_PLAYER_CLOSENESS) /
				(std::sqrt(iOurCities + (double)iTheirCities) * 20);
	/*  Would be nice to add another, say, 0.25 if any of our units w/o
		isRivalTerritory is currently inside the borders of an eOther team member,
		but that's too costly to check here and too complicated to keep track of. */
	return ::dRange(fromTrade + fromCloseness, 1/6.0, 8/6.0);
} // </advc.130i>

/*  <advc.130k> Random number to add or subtract from state counters
	(instead of just incrementing or decrementing). Binomial distribution
	with 2 trials and a probability of pr.
	Non-negative result, caller will have to multiply by -1 to decrease a counter.
	Result is capped at iUpperCap; -1: none. */
int CvTeamAI::AI_randomCounterChange(int iUpperCap, double pr) const
{
	CvGameSpeedInfo const& kSpeed = GC.getInfo(GC.getGame().getGameSpeedType());
	int iSpeedPercent = kSpeed.getGoldenAgePercent();
	int iOurEra = getCurrentEra();
	if(iOurEra <= 0)
		iSpeedPercent = kSpeed.getGrowthPercent();
	else if(iOurEra == 1)
		iSpeedPercent = (kSpeed.getGrowthPercent() + kSpeed.getGoldenAgePercent()) / 2;
	pr = 100 * pr / std::max(50, iSpeedPercent);
	int r = 0;
	if(::bernoulliSuccess(pr, "advc.130k"))
		r++;
	if(::bernoulliSuccess(pr, "advc.130k"))
		r++;
	if(iUpperCap < 0)
		return r;
	return std::min(r, iUpperCap);
} // </advc.130k>


void CvTeamAI::AI_doCounter()
{
	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		// <advc.130k>
		TeamTypes eOther = (TeamTypes)iI;
		if(!GET_TEAM(eOther).isAlive() || eOther == getID())
			continue;
		/*  advc.001: Guard added. NO_WARPLAN should imply that the state counter
			is at 0, rather than some arbitrary value. advc.104 relies on this. */
		if(AI_getWarPlan(eOther) != NO_WARPLAN)
			AI_changeWarPlanStateCounter(eOther, 1);
		/*  No randomization for atWar and hasMet. These are used by the AI in
			several places that more or less assume an exact count. */
		if(isAtWar(eOther))
			AI_changeAtWarCounter(eOther, 1);
		else // Better count to 1 deterministically
		{
			AI_changeAtPeaceCounter(eOther, AI_getAtPeaceCounter(eOther) == 0 ?
					1 : AI_randomCounterChange());
		}
		if(!isHasMet(eOther) || GET_TEAM(eOther).isBarbarian())
			continue;
		AI_changeHasMetCounter(eOther, 1);
		double decay = AI_getDiploDecay(); // advc.130k
		// <advc.130i>
		if(isOpenBorders(eOther))
		{
			double pr = AI_OpenBordersCounterIncrement(eOther) / 2; // advc.130i
			int iMax = 2 * AI_getOpenBordersAttitudeDivisor() + 10;
			AI_changeOpenBordersCounter(eOther, AI_randomCounterChange(iMax, pr));
		} // <advc.130k>
		else AI_setOpenBordersCounter(eOther, (int)(
				(1 - decay) * AI_getOpenBordersCounter(eOther))); // </advc.130k>
		// </advc.130i>
		if(isDefensivePact(eOther))
			AI_changeDefensivePactCounter(eOther, AI_randomCounterChange());
		// <advc.130k>
		else AI_setDefensivePactCounter(eOther, (int)(
				(1 - decay) * AI_getDefensivePactCounter(eOther))); // </advc.130k>
		if(AI_shareWar(eOther))
			AI_changeShareWarCounter(eOther, AI_randomCounterChange()); // </advc.130k>
		// <advc.130m> Decay by 1 with 10% probability
		else if(AI_getShareWarCounter(eOther) > 0 &&
				::bernoulliSuccess(0.1, "advc.130m"))
			AI_changeShareWarCounter(eOther, -1);
		AI_setSharedWarSuccess(eOther, (int)
				((1 - decay) * AI_getSharedWarSuccess(eOther))); // </advc.130m>
		// <advc.130p>
		AI_changeEnemyPeacetimeGrantValue(eOther, -(int)std::ceil(
				decay * AI_getEnemyPeacetimeGrantValue(eOther)));
		AI_changeEnemyPeacetimeTradeValue(eOther, -(int)std::ceil(
				decay * AI_getEnemyPeacetimeTradeValue(eOther)));
		// </advc.130p>
		// <advc.130r> Double decay rate for war success
		int iWSOld = AI_getWarSuccess(eOther);
		int iWSNew = std::max(0, iWSOld - (int)std::ceil(2 * decay * iWSOld));
		AI_setWarSuccess(eOther, iWSNew); // </advc.130r>
	}
}

// BETTER_BTS_AI_MOD, War Strategy AI, 03/26/10, jdog5000: START
// Block AI from declaring war on a distant vassal if it shares an area with the master
/*  advc.104j (comment): Since a war plan against a master implies a war plan
	against its vassal, I don't think this function is relevant anymore. */
bool CvTeamAI::AI_isOkayVassalTarget(TeamTypes eTeam) const
{
	// <advc.130v>
	if(GET_TEAM(eTeam).isCapitulated())
		return false; // </advc.130v>
	/*if (GET_TEAM(eTeam).isAVassal()) {
		if (!AI_hasCitiesInPrimaryArea(eTeam) || AI_calculateAdjacentLandPlots(eTeam) == 0) {
			for (int iI = 0; iI < MAX_CIV_TEAMS; iI++) {
				if (GET_TEAM(eTeam).isVassal((TeamTypes)iI)) {
					if (AI_hasCitiesInPrimaryArea((TeamTypes)iI) && AI_calculateAdjacentLandPlots((TeamTypes)iI) > 0)
						return false;
	} } } }
	return true;*/
	// K-Mod version. Same functionality (but without support for multiple masters)
	TeamTypes eMasterTeam = GET_TEAM(eTeam).getMasterTeam();
	if (eMasterTeam == eTeam)
		return true; // not a vassal.

	if ((!AI_hasCitiesInPrimaryArea(eTeam) || AI_calculateAdjacentLandPlots(eTeam) == 0) &&
		(AI_hasCitiesInPrimaryArea(eMasterTeam) && AI_calculateAdjacentLandPlots(eMasterTeam) > 0))
		return false;

	return true;
	// K-Mod end
} // BETTER_BTS_AI_MOD: END

// advc: New function, old content (cut from doWar); only relevant if UWAI disabled.
void CvTeamAI::AI_abandonWarPlanIfTimedOut(int iAbandonTimeModifier,
	TeamTypes eTarget, bool bLimited, int iEnemyPowerPercent)
{
	FAssert(canEventuallyDeclareWar(eTarget));
	bool bActive = false;
	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
	{
		if (GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID())
		{
			if (GET_PLAYER((PlayerTypes)iPlayer).AI_enemyTargetMissions(eTarget) > 0)
			{
				bActive = true;
				break;
			}
		}
	}
	if (!bActive)
	{
		if (AI_getWarPlanStateCounter(eTarget) > ((15 * iAbandonTimeModifier) / (100)))
		{
			if (gTeamLogLevel >= 1)
			{
				logBBAI("    Team %d (%S) abandoning WARPLAN_LIMITED or WARPLAN_DOGPILE against team %d (%S) after %d turns with enemy power percent %d",
						getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0),
						eTarget, GET_PLAYER(GET_TEAM(eTarget).getLeaderID()).getCivilizationDescription(0),
						AI_getWarPlanStateCounter(eTarget), iEnemyPowerPercent);
			}
			AI_setWarPlan(eTarget, NO_WARPLAN);
		}
	}
	if(!bLimited)
		return;
	if (AI_getWarPlan(eTarget) == WARPLAN_DOGPILE)
	{
		if (GET_TEAM(eTarget).getNumWars() <= 0)
		{
			if (gTeamLogLevel >= 1)
			{
				logBBAI("    Team %d (%S) abandoning WARPLAN_DOGPILE against team %d (%S) after %d turns because enemy has no war",
						getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0),
						eTarget, GET_PLAYER(GET_TEAM(eTarget).getLeaderID()).getCivilizationDescription(0),
						AI_getWarPlanStateCounter(eTarget));
			}
			AI_setWarPlan(eTarget, NO_WARPLAN);
		}
	}
}

// <advc.104>
WarAndPeaceAI::Team& CvTeamAI::warAndPeaceAI()
{
	return *m_pWpai;
}


WarAndPeaceAI::Team const& CvTeamAI::warAndPeaceAI() const
{
	return *m_pWpai;
} // </advc.104>

// <advc.136a>
bool CvTeamAI::AI_isPursuingCircumnavigation() const
{
	//PROFILE_FUNC(); // No problem at all it seems
	if(!GC.getGame().circumnavigationAvailable())
		return false;
	for(int i = 0; i < GC.getNumUnitInfos(); i++)
	{
		UnitTypes eUnit = (UnitTypes)i;
		if(GC.getInfo(eUnit).getDomainType() != DOMAIN_SEA)
			continue;
		for(int j = 0; j < MAX_CIV_PLAYERS; j++)
		{
			CvPlayerAI const& kMember = GET_PLAYER((PlayerTypes)j);
			if(kMember.isAlive() && kMember.getTeam() == getID() &&
					kMember.canTrain(eUnit) && kMember.AI_unitImpassableCount(eUnit) <= 0)
				return true;
		}
	}
	return false;
} // </advc.136a>

// Make war decisions, mainly for starting or switching war plans.
//
// This function has been tweaked throughout by BBAI and K-Mod, some changes marked others not.
// (K-Mod has made several structural changes.)
void CvTeamAI::AI_doWar()
{
	PROFILE_FUNC();

	CvGame& kGame = GC.getGame(); // K-Mod
	/* FAssert(!isHuman());
	FAssert(!isBarbarian());
	FAssert(!isMinorCiv());
	if (isAVassal())
		return;*/
	// disabled by K-Mod. All civs still need to do some basic updates.
	if (GC.getPythonCaller()->AI_doWar(getID()))
		return;
	// <advc.104>
	if(getWPAI.isEnabled() || getWPAI.isEnabled(true))
	{
		// Let the K-Mod code handle Barbarians and minors
		if(!isBarbarian() && !isMinorCiv() && getNumCities() > 0)
		{
			m_pWpai->doWar();
			if(getWPAI.isEnabled())
				return;
		}
	} // </advc.104>

	int iEnemyPowerPercent = AI_getEnemyPowerPercent();

	// K-Mod note: This first section also used for vassals, and for human players.
	for (int i = 0; i < MAX_CIV_TEAMS; i++)
	{
		TeamTypes eLoopTeam = (TeamTypes)i; // advc
		if (!GET_TEAM(eLoopTeam).isAlive())
			continue;
		if (!isHasMet(eLoopTeam))
			continue;
		if (AI_getWarPlan(eLoopTeam) == NO_WARPLAN)
			continue;

		int iTimeModifier = 100; // preperation time modifier
		int iAbandonTimeModifier = 100; // deadline for attack modifier
		iAbandonTimeModifier *= 50 + GC.getInfo(kGame.getGameSpeedType()).getTrainPercent();
		iAbandonTimeModifier /= 150;
		// (more adjustments to the time modifiers will come later)

		if (AI_getWarPlan(eLoopTeam) == WARPLAN_ATTACKED_RECENT)
		{
			FAssert(isAtWar(eLoopTeam));

			if (AI_getAtWarCounter(eLoopTeam) > ((GET_TEAM(eLoopTeam).AI_isLandTarget(getID())) ? 9 : 3))
			{
				if (gTeamLogLevel >= 1)
				{
					logBBAI("    Team %d (%S) switching WARPLANS against team %d (%S) from ATTACKED_RECENT to ATTACKED with enemy power percent %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eLoopTeam, GET_PLAYER(GET_TEAM(eLoopTeam).getLeaderID()).getCivilizationDescription(0), iEnemyPowerPercent);
				}
				AI_setWarPlan(eLoopTeam, WARPLAN_ATTACKED);
			}
		}

		// K-Mod
		if (isHuman() || isAVassal())
		{
			CvTeamAI& kOurMaster = GET_TEAM(getMasterTeam());
			if (!isAtWar(eLoopTeam))
			{
				/* advc.006: The second clause was ==WARPLAN_LIMITED.
				   Breached in one of my (non-AIAutoPlay) games, and I don't think
				   it made sense either.
				   Actually, disabling it entirely. Not always true when returning
				   from AI Auto Play, and I don't think this is a problem. */
				//FAssert(AI_getWarPlan(eLoopTeam) == WARPLAN_PREPARING_TOTAL || AI_getWarPlan(eLoopTeam) == WARPLAN_PREPARING_LIMITED);
				if (isHuman() || kOurMaster.isHuman())
				{
					if (AI_getWarPlanStateCounter(eLoopTeam) > 20 * iAbandonTimeModifier / 100)
					{
						if (gTeamLogLevel >= 1)
						{
							logBBAI("    Team %d (%S) abandoning WARPLANS against team %d (%S) due to human / vassal timeout", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eLoopTeam, GET_PLAYER(GET_TEAM(eLoopTeam).getLeaderID()).getCivilizationDescription(0));
						}
						AI_setWarPlan(eLoopTeam, NO_WARPLAN);
					}
				}
				else
				{
					if (kOurMaster.AI_getWarPlan(eLoopTeam) == NO_WARPLAN)
					{
						if (gTeamLogLevel >= 1)
						{
							logBBAI("    Team %d (%S) abandoning WARPLANS against team %d (%S) due to AI master's warplan cancelation", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eLoopTeam, GET_PLAYER(GET_TEAM(eLoopTeam).getLeaderID()).getCivilizationDescription(0));
						}
						AI_setWarPlan(eLoopTeam, NO_WARPLAN);
					}
				}
			}

			continue; // Nothing else required for vassals and human players
		}
		// K-Mod

		if (!isAtWar(eLoopTeam)) // K-Mod. time / abandon modifiers are only relevant for war preparations. We don't need them if we are already at war.
		{
			int iThreshold = (80*AI_maxWarNearbyPowerRatio())/100;

			if (iEnemyPowerPercent < iThreshold)
			{
				iTimeModifier *= iEnemyPowerPercent;
				iTimeModifier /= iThreshold;
			}
			// K-Mod
			// intercontinental wars need more prep time
			if (!AI_hasCitiesInPrimaryArea(eLoopTeam))
			{
				iTimeModifier *= 5;
				iTimeModifier /= 4;
				iAbandonTimeModifier *= 5;
				iAbandonTimeModifier /= 4;
				// maybe in the future I'll count the number of local cities and the number of overseas cities
				// and use it to make a more appropriate modifier... but not now.
			}
			else
			{
				//with crush strategy, use just 2/3 of the prep time.
				int iCrushMembers = AI_countMembersWithStrategy(AI_STRATEGY_CRUSH);
				iTimeModifier *= 3 * (getNumMembers()-iCrushMembers) + 2 * iCrushMembers;
				iTimeModifier /= 3;
			}
			// K-Mod end

			iTimeModifier *= 50 + GC.getInfo(kGame.getGameSpeedType()).getTrainPercent();
			iTimeModifier /= 150;

			FAssert(iTimeModifier >= 0);
		}

		bool bEnemyVictoryLevel4 = GET_TEAM(eLoopTeam).AI_isAnyMemberDoVictoryStrategyLevel4();

		if (AI_getWarPlan(eLoopTeam) == WARPLAN_PREPARING_LIMITED)
		{
			FAssert(canEventuallyDeclareWar(eLoopTeam));

			if (AI_getWarPlanStateCounter(eLoopTeam) > ((5 * iTimeModifier) / (bEnemyVictoryLevel4 ? 400 : 100)))
			{
				if (AI_startWarVal(eLoopTeam, WARPLAN_LIMITED) > 0) // K-Mod. Last chance to change our mind if circumstances have changed
				{
					AI_setWarPlan(eLoopTeam, WARPLAN_LIMITED);
					if (gTeamLogLevel >= 1) logBBAI("    Team %d (%S) switching WARPLANS against team %d (%S) from PREPARING_LIMITED to LIMITED after %d turns with enemy power percent %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eLoopTeam, GET_PLAYER(GET_TEAM(eLoopTeam).getLeaderID()).getCivilizationDescription(0), AI_getWarPlanStateCounter(eLoopTeam), iEnemyPowerPercent);
				}
				else
				{	// advc.001: Actually abandon the war plan
					AI_setWarPlan(eLoopTeam, NO_WARPLAN);
					if (gTeamLogLevel >= 1) logBBAI("    Team %d (%S) abandoning WARPLAN_LIMITED against team %d (%S) after %d turns with enemy power percent %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eLoopTeam, GET_PLAYER(GET_TEAM(eLoopTeam).getLeaderID()).getCivilizationDescription(0), AI_getWarPlanStateCounter(eLoopTeam), iEnemyPowerPercent);
				}
			}
		}
		else if (AI_getWarPlan(eLoopTeam) == WARPLAN_LIMITED || AI_getWarPlan(eLoopTeam) == WARPLAN_DOGPILE)
		{
			if (!isAtWar(eLoopTeam))
			{	// advc: Moved into a subroutine
				AI_abandonWarPlanIfTimedOut(iAbandonTimeModifier, eLoopTeam, true, iEnemyPowerPercent);
			}
		}
		else if (AI_getWarPlan(eLoopTeam) == WARPLAN_PREPARING_TOTAL)
		{
			FAssert(canEventuallyDeclareWar(eLoopTeam));
			if (AI_getWarPlanStateCounter(eLoopTeam) > ((10 * iTimeModifier) / (bEnemyVictoryLevel4 ? 400 : 100)))
			{
				bool bAreaValid = false;
				bool bShareValid = false;

				FOR_EACH_AREA_VAR(pLoopArea)
				{
					if (AI_isPrimaryArea(pLoopArea))
					{
						if (GET_TEAM(eLoopTeam).countNumCitiesByArea(pLoopArea) > 0)
						{
							bShareValid = true;

							AreaAITypes eAreaAI = AI_calculateAreaAIType(pLoopArea, true);

							/* BBAI code
							if (eAreaAI == AREAAI_DEFENSIVE)
								bAreaValid = false;
							else if (eAreaAI == AREAAI_OFFENSIVE)
								bAreaValid = true;*/
							// K-Mod. Doing it that way means the order the areas are checked is somehow important...
							if (eAreaAI == AREAAI_OFFENSIVE)
							{
								bAreaValid = true; // require at least one offense area
							}
							else if (eAreaAI == AREAAI_DEFENSIVE)
							{
								bAreaValid = false;
								break; // false if there are _any_ defence areas
							}
							// K-Mod end
						}
					}
				}

				if (((bAreaValid && iEnemyPowerPercent < 140 ||
					!bShareValid && iEnemyPowerPercent < 110 ||
					GET_TEAM(eLoopTeam).AI_getLowestVictoryCountdown() >= 0) &&
					AI_startWarVal(eLoopTeam, WARPLAN_TOTAL) > 0)) // K-Mod. Last chance to change our mind if circumstances have changed
				{
					AI_setWarPlan(eLoopTeam, WARPLAN_TOTAL);
					if (gTeamLogLevel >= 1) logBBAI("    Team %d (%S) switching WARPLANS against team %d (%S) from PREPARING_TOTAL to TOTAL after %d turns with enemy power percent %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eLoopTeam, GET_PLAYER(GET_TEAM(eLoopTeam).getLeaderID()).getCivilizationDescription(0), AI_getWarPlanStateCounter(eLoopTeam), iEnemyPowerPercent);
				}
				else if (AI_getWarPlanStateCounter(eLoopTeam) > ((20 * iAbandonTimeModifier) / 100))
				{
					AI_setWarPlan(eLoopTeam, NO_WARPLAN);
					if (gTeamLogLevel >= 1) logBBAI("    Team %d (%S) abandoning WARPLAN_TOTAL_PREPARING against team %d (%S) after %d turns with enemy power percent %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eLoopTeam, GET_PLAYER(GET_TEAM(eLoopTeam).getLeaderID()).getCivilizationDescription(0), AI_getWarPlanStateCounter(eLoopTeam), iEnemyPowerPercent);
				}
			}
		}
		else if (AI_getWarPlan(eLoopTeam) == WARPLAN_TOTAL && !isAtWar(eLoopTeam))
		{	// advc: Moved into a subroutine
			AI_abandonWarPlanIfTimedOut(iAbandonTimeModifier, eLoopTeam, false, iEnemyPowerPercent);
		}
	}

	// K-Mod. This is the end of the basics updates.
	// The rest of the stuff is related to making peace deals, and planning future wars.
	if (isHuman() || isBarbarian() || isMinorCiv() || isAVassal())
		return;
	// K-Mod end

	for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
	{
		CvPlayerAI& kLoopPlayer = GET_PLAYER(i);
		if (kLoopPlayer.isAlive())
		{
			if (kLoopPlayer.getTeam() == getID())
			{
				kLoopPlayer.AI_doPeace();
			}
		}
	}

	int iNumMembers = getNumMembers();
	/* original bts code
	int iHighUnitSpendingPercent = 0;
	int iLowUnitSpendingPercent = 0;
	for (iI = 0; iI < MAX_PLAYERS; iI++) {
		if (GET_PLAYER((PlayerTypes)iI).isAlive()) {
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID()) {
				int iUnitSpendingPercent = (GET_PLAYER((PlayerTypes)iI).calculateUnitCost() * 100) / std::max(1, GET_PLAYER((PlayerTypes)iI).calculatePreInflatedCosts());
				iHighUnitSpendingPercent += (std::max(0, iUnitSpendingPercent - 7) / 2);
				iLowUnitSpendingPercent += iUnitSpendingPercent;
			}
		}
	}
	iHighUnitSpendingPercent /= iNumMembers;
	iLowUnitSpendingPercent /= iNumMembers; */
	// K-Mod, this simply wasn't being used anywhere.

	// K-Mod. Gather some data...
	bool bAtWar = false;
	bool bTotalWarPlan = false;
	bool bAnyWarPlan = false;
	bool bLocalWarPlan = false;
	for (int i = 0; i < MAX_CIV_TEAMS; i++)
	{
		TeamTypes eLoopTeam = (TeamTypes)i; // advc
		if (GET_TEAM(eLoopTeam).isAlive() && !GET_TEAM(eLoopTeam).isMinorCiv())
		{
			bAtWar = bAtWar || isAtWar(eLoopTeam);

			switch (AI_getWarPlan(eLoopTeam))
			{
			case NO_WARPLAN:
				break;
			case WARPLAN_PREPARING_TOTAL:
			case WARPLAN_TOTAL:
				bTotalWarPlan = true;
			default: // all other warplans
				bLocalWarPlan = bLocalWarPlan || AI_isLandTarget(eLoopTeam);
				bAnyWarPlan = true;
				break;
			}
		}
	}
	// K-Mod end

	// if at war, check for making peace
	// Note: this section relates to automatic peace deals for inactive wars.
	if (bAtWar && kGame.getSorenRandNum(AI_makePeaceRand(), "AI Make Peace") == 0)
	{
		for (int i = 0; i < MAX_CIV_TEAMS; i++)
		{
			TeamTypes eLoopTeam = (TeamTypes)i; // advc
			if (!GET_TEAM(eLoopTeam).isAlive())
				continue;
			if (eLoopTeam == getID())
				continue;
			if (GET_TEAM(eLoopTeam).isHuman())
				continue;

			if (!isAtWar(eLoopTeam))
				continue;
			if (!AI_isChosenWar(eLoopTeam))
				continue;
			if (!canContact(eLoopTeam, true))
				continue;

			FAssert(!(GET_TEAM(eLoopTeam).isMinorCiv()));

			if (AI_getAtWarCounter(eLoopTeam) > std::max(10, (14 * GC.getInfo(kGame.getGameSpeedType()).getVictoryDelayPercent()) / 100))
			{
				// If nothing is happening in war
				if (AI_getWarSuccess(eLoopTeam) + GET_TEAM(eLoopTeam).AI_getWarSuccess(getID()) < 2 * GC.getDefineINT(CvGlobals::WAR_SUCCESS_ATTACKING))
				{
					if (kGame.getSorenRandNum(8, "AI Make Peace 1") == 0)
					{
						bool bValid = true;

						for (PlayerTypes j = (PlayerTypes)0; j < MAX_CIV_PLAYERS; j=(PlayerTypes)(j+1))
						{
							CvPlayerAI& kLoopPlayer = GET_PLAYER(j);
							if (kLoopPlayer.getTeam() == getID())
							{
								if (kLoopPlayer.AI_enemyTargetMissions(eLoopTeam) > 0)
								{
									bValid = false;
									break;
								}
							}

							if (kLoopPlayer.getTeam() == eLoopTeam)
							{
								//MissionAITypes eMissionAI = MISSIONAI_ASSAULT;
								if (kLoopPlayer.AI_enemyTargetMissions(getID()) > 0)
								{
									bValid = false;
									break;
								}
							}
						}

						if (bValid)
						{
							makePeace(eLoopTeam);

							if (gTeamLogLevel >= 1)
							{
								logBBAI("  Team %d (%S) making peace due to time and no fighting", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0));
							}

							continue;
						}
					}
				}

				// Fought to a long draw
				if (AI_getAtWarCounter(eLoopTeam) > ((AI_getWarPlan(eLoopTeam) == WARPLAN_TOTAL ? 40 : 30) *
					GC.getInfo(kGame.getGameSpeedType()).getVictoryDelayPercent()) / 100)
				{
					int iOurValue = AI_endWarVal(eLoopTeam);
					int iTheirValue = GET_TEAM(eLoopTeam).AI_endWarVal(getID());
					if ((iOurValue > (iTheirValue / 2)) && (iTheirValue > (iOurValue / 2)))
					{
						if (gTeamLogLevel >= 1)
						{
							logBBAI("  Team %d (%S) making peace due to time and endWarVal %d vs their %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0) , iOurValue, iTheirValue);
						}
						makePeace(eLoopTeam);
						continue;
					}
				}

				// All alone in what we thought was a dogpile
				if (AI_getWarPlan(eLoopTeam) == WARPLAN_DOGPILE)
				{
					if (GET_TEAM(eLoopTeam).getNumWars(true) == 1)
					{
						int iOurValue = AI_endWarVal(eLoopTeam);
						int iTheirValue = GET_TEAM(eLoopTeam).AI_endWarVal(getID());
						if ((iTheirValue > (iOurValue / 2)))
						{
							if (gTeamLogLevel >= 1)
							{
								logBBAI("  Team %d (%S) making peace due to being only dog-piler left", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0));
							}
							makePeace(eLoopTeam);
							continue;
						}
					}
				}
			}
		}
	}

	// if no war plans, consider starting one!

	//if (getAnyWarPlanCount(true) == 0 || iEnemyPowerPercent < 45)
	// K-Mod. Some more nuance to the conditions for considering war
	// First condition: only consider a new war if there are no current wars that need more attention. (local total war, or a war we aren't winning)
	bool bConsiderWar = !bAnyWarPlan || (iEnemyPowerPercent < 45 && !(bLocalWarPlan && bTotalWarPlan) && AI_getWarSuccessRating() > (bTotalWarPlan ? 40 : 15));
	// Second condition: don't consider war very early in the game. It would be unfair on human players to rush them with our extra starting units and techs!
	bConsiderWar = bConsiderWar &&
		(kGame.isOption(GAMEOPTION_AGGRESSIVE_AI) ||
		 kGame.getElapsedGameTurns() >= GC.getInfo(kGame.getGameSpeedType()).getBarbPercent() * 30 / 100 ||
		 kGame.getNumCivCities() > GC.getInfo(GC.getMap().getWorldSize()).getTargetNumCities() * kGame.countCivPlayersAlive()/2);
	// (Perhaps the no-war turn threshold should depend on the game difficulty level; but I don't think it would make much difference.)

	if (bConsiderWar)
	// K-mod end
	{
		bool bAggressive = kGame.isOption(GAMEOPTION_AGGRESSIVE_AI);

		int iFinancialTroubleCount = 0;
		int iDaggerCount = 0;
		int iGetBetterUnitsCount = 0;
		for (PlayerTypes i = (PlayerTypes)0; i < MAX_PLAYERS; i=(PlayerTypes)(i+1))
		{
			CvPlayerAI& kLoopPlayer = GET_PLAYER(i);

			if (kLoopPlayer.isAlive())
			{
				if (kLoopPlayer.getTeam() == getID())
				{
					if (kLoopPlayer.AI_isDoStrategy(AI_STRATEGY_DAGGER)
						|| kLoopPlayer.AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3)
						|| kLoopPlayer.AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION4))
					{
						iDaggerCount++;
						bAggressive = true;
					}

					if (kLoopPlayer.AI_isDoStrategy(AI_STRATEGY_GET_BETTER_UNITS))
					{
						iGetBetterUnitsCount++;
					}

					if (kLoopPlayer.AI_isFinancialTrouble())
					{
						iFinancialTroubleCount++;
					}
				}
			}
		}

		// if random in this range is 0, we go to war of this type (so lower numbers are higher probablity)
		// average of everyone on our team
		int iTotalWarRand;
		int iLimitedWarRand;
		int iDogpileWarRand;
		AI_getWarRands(iTotalWarRand, iLimitedWarRand, iDogpileWarRand);

		int iTotalWarThreshold;
		int iLimitedWarThreshold;
		int iDogpileWarThreshold;
		AI_getWarThresholds(iTotalWarThreshold, iLimitedWarThreshold, iDogpileWarThreshold);

		// we oppose war if half the non-dagger teammates in financial trouble
		bool bFinancesOpposeWar = false;
		if (iFinancialTroubleCount - iDaggerCount >= std::max(1, getNumMembers() / 2))
		{
			// this can be overridden by by the pro-war booleans
			bFinancesOpposeWar = true;
		}

		// if agressive, we may start a war to get money
		bool bFinancesProTotalWar = false;
		bool bFinancesProLimitedWar = false;
		bool bFinancesProDogpileWar = false;
		if (iFinancialTroubleCount > 0)
		{
			// do we like all out wars?
			if (iDaggerCount > 0 || iTotalWarRand < 100)
			{
				bFinancesProTotalWar = true;
			}

			// do we like limited wars?
			if (iLimitedWarRand < 100)
			{
				bFinancesProLimitedWar = true;
			}

			// do we like dogpile wars?
			if (iDogpileWarRand < 100)
			{
				bFinancesProDogpileWar = true;
			}
		}
		bool bFinancialProWar = (bFinancesProTotalWar || bFinancesProLimitedWar || bFinancesProDogpileWar);

		// overall war check (quite frequently true)
		bool bMakeWarChecks = false;
		if ((iGetBetterUnitsCount - iDaggerCount) * 3 < iNumMembers * 2)
		{
			if (bFinancialProWar || !bFinancesOpposeWar)
			{
				// random overall war chance (at noble+ difficulties this is 100%)
				if (kGame.getSorenRandNum(100, "AI Declare War 1") < GC.getInfo(kGame.getHandicapType()).getAIDeclareWarProb())
				{
					bMakeWarChecks = true;
				}
			}
		}

		if (bMakeWarChecks)
		{
			int iOurPower = getPower(true);

			if (bAggressive && !AI_isAnyWarPlan())
			{
				iOurPower *= 4;
				iOurPower /= 3;
			}

			iOurPower *= (100 - iEnemyPowerPercent);
			iOurPower /= 100;

			if ((bFinancesProTotalWar || !bFinancesOpposeWar) &&
				(kGame.getSorenRandNum(iTotalWarRand, "AI Maximum War") <= iTotalWarThreshold))
			{
				int iNoWarRoll = kGame.getSorenRandNum(100, "AI No War");
				iNoWarRoll = range(iNoWarRoll + (bAggressive ? 10 : 0) + (bFinancesProTotalWar ? 10 : 0) - (20*iGetBetterUnitsCount)/iNumMembers, 0, 99);

				int iBestValue = 10; // K-Mod. I've set the starting value above zero just as a buffer against close-calls which end up being negative value in the near future.
				TeamTypes eBestTeam = NO_TEAM;

				for (int iPass = 0; iPass < 3; iPass++)
				{
					for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
					{
						if (canEventuallyDeclareWar((TeamTypes)iI) && AI_haveSeenCities((TeamTypes)iI))
						{
							TeamTypes eLoopMasterTeam = GET_TEAM((TeamTypes)iI).getMasterTeam(); // K-Mod (plus all changes which refer to this variable).
							bool bVassal = eLoopMasterTeam != iI;
							if (bVassal && !AI_isOkayVassalTarget((TeamTypes)iI))
								continue;

							if (iNoWarRoll >= AI_noWarAttitudeProb(AI_getAttitude((TeamTypes)iI)) && (!bVassal || iNoWarRoll >= AI_noWarAttitudeProb(AI_getAttitude(eLoopMasterTeam))))
							{
								int iDefensivePower = (GET_TEAM((TeamTypes)iI).getDefensivePower(getID()) * 2) / 3;

								if (iDefensivePower < ((iOurPower * ((iPass > 1) ? AI_maxWarDistantPowerRatio() : AI_maxWarNearbyPowerRatio())) / 100))
								{
									// XXX make sure they share an area....

									FAssertMsg(!(GET_TEAM((TeamTypes)iI).isBarbarian()), "Expected to not be declaring war on the barb civ");
									FAssertMsg(iI != getID(), "Expected not to be declaring war on self (DOH!)");

									if ((iPass > 1 && !bLocalWarPlan) || AI_isLandTarget((TeamTypes)iI) || AI_isAnyCapitalAreaAlone() || GET_TEAM((TeamTypes)iI).AI_isAnyMemberDoVictoryStrategyLevel4())
									{
										if ((iPass > 0) || (AI_calculateAdjacentLandPlots((TeamTypes)iI) >= ((getTotalLand() * AI_maxWarMinAdjacentLandPercent()) / 100)) || GET_TEAM((TeamTypes)iI).AI_isAnyMemberDoVictoryStrategyLevel4())
										{
											int iValue = AI_startWarVal((TeamTypes)iI, WARPLAN_TOTAL);

											if (iValue > 0 && gTeamLogLevel >= 2)
											{
												logBBAI("    Team %d (%S) considering starting TOTAL warplan with team %d with value %d on pass %d with %d adjacent plots", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), iI, iValue, iPass, AI_calculateAdjacentLandPlots((TeamTypes)iI));
											}

											if (iValue > iBestValue)
											{
												iBestValue = iValue;
												eBestTeam = ((TeamTypes)iI);
											}
										}
									}
								}
							}
						}
					}

					if (eBestTeam != NO_TEAM)
					{
						if (gTeamLogLevel >= 1)
						{
							logBBAI("    Team %d (%S) starting TOTAL warplan preparations against team %d on pass %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eBestTeam, iPass);
						}

						AI_setWarPlan(eBestTeam, (iDaggerCount > 0) ? WARPLAN_TOTAL : WARPLAN_PREPARING_TOTAL);
						break;
					}
				}
			}
			else if ((bFinancesProLimitedWar || !bFinancesOpposeWar) &&
					kGame.getSorenRandNum(iLimitedWarRand, "AI Limited War") <=
					// UNOFFICIAL_PATCH, Bugfix, 01/02/09, jdog5000: (was 0)
					iLimitedWarThreshold)
			{
				int iNoWarRoll = kGame.getSorenRandNum(100, "AI No War") - 10;
				iNoWarRoll = range(iNoWarRoll + (bAggressive ? 10 : 0) + (bFinancesProLimitedWar ? 10 : 0), 0, 99);

				int iBestValue = 0;
				TeamTypes eBestTeam = NO_TEAM;

				for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
				{
					if (canEventuallyDeclareWar((TeamTypes)iI) && AI_haveSeenCities((TeamTypes)iI))
					{
						TeamTypes eLoopMasterTeam = GET_TEAM((TeamTypes)iI).getMasterTeam(); // K-Mod (plus all changes which refer to this variable).
						bool bVassal = eLoopMasterTeam != iI;

						if (bVassal && !AI_isOkayVassalTarget((TeamTypes)iI))
							continue;

						if (iNoWarRoll >= AI_noWarAttitudeProb(AI_getAttitude((TeamTypes)iI)) && (!bVassal || iNoWarRoll >= AI_noWarAttitudeProb(AI_getAttitude(eLoopMasterTeam))))
						{
							if (AI_isLandTarget((TeamTypes)iI) || (AI_isAnyCapitalAreaAlone() && GET_TEAM((TeamTypes)iI).AI_isAnyCapitalAreaAlone()))
							{
								if (GET_TEAM((TeamTypes)iI).getDefensivePower(getID()) < ((iOurPower * AI_limitedWarPowerRatio()) / 100))
								{
									int iValue = AI_startWarVal((TeamTypes)iI, WARPLAN_LIMITED);

									if (iValue > 0 && gTeamLogLevel >= 2)
									{
										logBBAI("    Team %d (%S) considering starting LIMITED warplan with team %d with value %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), iI, iValue);
									}

									if (iValue > iBestValue)
									{
										//FAssert(!AI_shareWar((TeamTypes)iI)); // disabled by K-Mod. (It isn't always true.)
										iBestValue = iValue;
										eBestTeam = ((TeamTypes)iI);
									}
								}
							}
						}
					}
				}

				if (eBestTeam != NO_TEAM)
				{
					if (gTeamLogLevel >= 1)
					{
						logBBAI("    Team %d (%S) starting LIMITED warplan preparations against team %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eBestTeam);
					}

					AI_setWarPlan(eBestTeam, (iDaggerCount > 0) ? WARPLAN_LIMITED : WARPLAN_PREPARING_LIMITED);
				}
			}
			else if ((bFinancesProDogpileWar || !bFinancesOpposeWar) &&
				(kGame.getSorenRandNum(iDogpileWarRand, "AI Dogpile War") <= iDogpileWarThreshold))
			{
				int iNoWarRoll = kGame.getSorenRandNum(100, "AI No War") - 20;
				iNoWarRoll = range(iNoWarRoll + (bAggressive ? 10 : 0) + (bFinancesProDogpileWar ? 10 : 0), 0, 99);

				int iBestValue = 0;
				TeamTypes eBestTeam = NO_TEAM;

				for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
				{
					if (canDeclareWar((TeamTypes)iI) && AI_haveSeenCities((TeamTypes)iI))
					{
						TeamTypes eLoopMasterTeam = GET_TEAM((TeamTypes)iI).getMasterTeam(); // K-Mod (plus all changes which refer to this variable).
						bool bVassal = eLoopMasterTeam != iI;

						if (bVassal && !AI_isOkayVassalTarget((TeamTypes)iI))
							continue;

						if (iNoWarRoll >= AI_noWarAttitudeProb(AI_getAttitude((TeamTypes)iI)) && (!bVassal || iNoWarRoll >= AI_noWarAttitudeProb(AI_getAttitude(eLoopMasterTeam))))
						{
							if (GET_TEAM((TeamTypes)iI).getNumWars() > 0)
							{
								if (AI_isLandTarget((TeamTypes)iI) || GET_TEAM((TeamTypes)iI).AI_isAnyMemberDoVictoryStrategyLevel4())
								{
									int iDogpilePower = iOurPower;

									for (int iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
									{
										if (GET_TEAM((TeamTypes)iJ).isAlive())
										{
											if (iJ != iI)
											{
												if (atWar(((TeamTypes)iJ), ((TeamTypes)iI)))
												{
													iDogpilePower += GET_TEAM((TeamTypes)iJ).getPower(false);
												}
											}
										}
									}

									FAssert(GET_TEAM((TeamTypes)iI).getPower(true) == GET_TEAM((TeamTypes)iI).getDefensivePower(getID()) || GET_TEAM((TeamTypes)iI).isAVassal());

									if (((GET_TEAM((TeamTypes)iI).getDefensivePower(getID()) * 3) / 2) < iDogpilePower)
									{
										int iValue = AI_startWarVal((TeamTypes)iI, WARPLAN_DOGPILE);

										if (iValue > 0 && gTeamLogLevel >= 2)
										{
											logBBAI("    Team %d (%S) considering starting DOGPILE warplan with team %d with value %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), iI, iValue);
										}

										if (iValue > iBestValue)
										{
											//FAssert(!AI_shareWar((TeamTypes)iI)); // disabled by K-Mod. (why is this even here?)
											iBestValue = iValue;
											eBestTeam = ((TeamTypes)iI);
										}
									}
								}
							}
						}
					}
				}

				if (eBestTeam != NO_TEAM)
				{
					if (gTeamLogLevel >= 1)
					{
						logBBAI("  Team %d (%S) starting DOGPILE warplan preparations with team %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eBestTeam);
					}
					AI_setWarPlan(eBestTeam, WARPLAN_DOGPILE);
				}
			}
		}
	}
}

//returns true if war is veto'd by rolls.
bool CvTeamAI::AI_performNoWarRolls(TeamTypes eTeam)
{
	if (GC.getGame().getSorenRandNum(100, "AI Declare War 1") >
			GC.getInfo(GC.getGame().getHandicapType()).getAIDeclareWarProb())
		return true;
	if (GC.getGame().getSorenRandNum(100, "AI No War") <=
			AI_noWarAttitudeProb(AI_getAttitude(eTeam)))
		return true;
	return false;
}


int CvTeamAI::AI_getAttitudeWeight(TeamTypes eTeam) const
{
	int iAttitudeWeight = 0;
	switch (AI_getAttitude(eTeam))
	{
	case ATTITUDE_FURIOUS:
		iAttitudeWeight = -100;
		break;
	case ATTITUDE_ANNOYED:
		iAttitudeWeight = -40;
		break;
	case ATTITUDE_CAUTIOUS:
		iAttitudeWeight = -5;
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

int CvTeamAI::AI_getLowestVictoryCountdown() const
{
	int iBestVictoryCountdown = MAX_INT;
	for (int iVictory = 0; iVictory < GC.getNumVictoryInfos(); iVictory++)
	{
		 int iCountdown = getVictoryCountdown((VictoryTypes)iVictory);
		 if (iCountdown > 0)
		 {
			iBestVictoryCountdown = std::min(iBestVictoryCountdown, iCountdown);
		 }
	}
	if (MAX_INT == iBestVictoryCountdown)
	{
		iBestVictoryCountdown = -1;
	}
	return iBestVictoryCountdown;
}


int CvTeamAI::AI_getTechMonopolyValue(TechTypes eTech, TeamTypes eTeam) const  // advc: style changes
{
	PROFILE_FUNC(); // advc: called not that infrequently
	//bool bWarPlan = (getAnyWarPlanCount(eTeam) > 0);
	bool bWarPlan = (AI_getWarPlan(eTeam) != NO_WARPLAN); // advc.001
	int iValue = 0;
	for (int i = 0; i < GC.getNumUnitClassInfos(); i++)
	{
		UnitClassTypes eUnitClass = (UnitClassTypes)i;
		UnitTypes eUnit = (UnitTypes)GC.getInfo(eUnitClass).getDefaultUnitIndex();
		if (eUnit == NO_UNIT || !isTechRequiredForUnit(eTech, eUnit))
			continue;

		if (isWorldUnitClass(eUnitClass))
			iValue += 50;

		CvUnitInfo const& kUnit = GC.getInfo(eUnit);
		/*  advc (comment): Relying only on the AndTech seems questionable. Anything
			that the canTrain functions check could be relevant here. */
		if (kUnit.getPrereqAndTech() != eTech)
			continue;

		int iNavalValue = 0;
		int iCombatRatio = (kUnit.getCombat() * 100) / std::max(1, GC.getGame().getBestLandUnitCombat());
		if (iCombatRatio > 50)
			iValue += ((bWarPlan ? 100 : 50) * (iCombatRatio - 40)) / 50;

		switch (kUnit.getDefaultUnitAIType())
		{
		case UNITAI_UNKNOWN:
		case UNITAI_ANIMAL:
		case UNITAI_SETTLE:
		case UNITAI_WORKER:
			break;
		case UNITAI_ATTACK:
		case UNITAI_ATTACK_CITY:
		case UNITAI_COLLATERAL:
			iValue += bWarPlan ? 50 : 20;
			break;
		case UNITAI_PILLAGE:
		case UNITAI_RESERVE:
		case UNITAI_COUNTER:
		case UNITAI_PARADROP:
		case UNITAI_CITY_DEFENSE:
		case UNITAI_CITY_COUNTER:
		case UNITAI_CITY_SPECIAL:
			iValue += bWarPlan ? 40 : 15;
			break;
		case UNITAI_EXPLORE:
		case UNITAI_MISSIONARY:
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
			break;
		case UNITAI_ICBM:
			iValue += bWarPlan ? 80 : 40;
			break;
		case UNITAI_WORKER_SEA:
			break;
		case UNITAI_ATTACK_SEA:
			iNavalValue += 50;
			break;
		case UNITAI_RESERVE_SEA:
		case UNITAI_ESCORT_SEA:
			iNavalValue += 30;
			break;
		case UNITAI_EXPLORE_SEA:
			iValue += GC.getGame().circumnavigationAvailable() ? 100 : 0;
			break;
		case UNITAI_ASSAULT_SEA:
			iNavalValue += 60;
			break;
		case UNITAI_SETTLER_SEA:
		case UNITAI_MISSIONARY_SEA:
		case UNITAI_SPY_SEA:
			break;
		case UNITAI_CARRIER_SEA:
		case UNITAI_MISSILE_CARRIER_SEA:
			iNavalValue += 40;
			break;
		case UNITAI_PIRATE_SEA:
			iNavalValue += 20;
			break;
		case UNITAI_ATTACK_AIR:
		case UNITAI_DEFENSE_AIR:
			iValue += bWarPlan ? 60 : 30;
			break;
		case UNITAI_CARRIER_AIR:
			iNavalValue += 40;
			break;
		case UNITAI_MISSILE_AIR:
			iValue += bWarPlan ? 40 : 20;
			break;
		default:
			FAssert(false);
			break;
		}
		if (iNavalValue > 0)
		{
			if (AI_isAnyCapitalAreaAlone())
				iValue += iNavalValue / 2;
			if (bWarPlan && !AI_isLandTarget(eTeam))
				iValue += iNavalValue / 2;
		}
	}
	for (int i = 0; i < GC.getNumBuildingClassInfos(); i++) // advc.opt: was getNumBuildingInfos
	{
		BuildingClassTypes eBuildingClass = (BuildingClassTypes)i;
		BuildingTypes eBuilding = (BuildingTypes)GC.getInfo(eBuildingClass).getDefaultBuildingIndex();
		if (eBuilding == NO_BUILDING || !isTechRequiredForBuilding(eTech, eBuilding))
			continue;

		CvBuildingInfo const& kBuilding = GC.getInfo(eBuilding);
		if (kBuilding.getReligionType() == NO_RELIGION)
			iValue += 20; // advc.550c: was 30
		if (isWorldWonderClass(eBuildingClass) && !GC.getGame().isBuildingClassMaxedOut(eBuildingClass))
			iValue += 40; // advc.550c: was 50
	}
	for (int i = 0; i < GC.getNumProjectInfos(); i++)
	{
		ProjectTypes eProject = (ProjectTypes)i;
		if (GC.getInfo(eProject).getTechPrereq() != eTech)
			continue;
		if (isWorldProject(eProject) && !GC.getGame().isProjectMaxedOut(eProject))
			iValue += 80; // advc.550c: was 100
		else iValue += 40; // advc.550c: was 50
	}
	return iValue;
}


bool CvTeamAI::AI_isWaterAreaRelevant(CvArea* pArea) const
{
	int iTeamCities = 0;
	int iOtherTeamCities = 0;
	/*  BETTER_BTS_AI_MOD, City AI, 01/15/09, jdog5000: START
		(minor changes to BtS code) */
	CvArea* pBiggestArea = GC.getMap().findBiggestArea(true);
	if (pBiggestArea == pArea)
		return true;

	// An area is deemed relevant if it has at least 2 cities of our and different teams.
	// Also count lakes which are connected to ocean by a bridge city
	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
	{
		CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if ((iTeamCities < 2 && kPlayer.getTeam() == getID()) || (iOtherTeamCities < 2 && kPlayer.getTeam() != getID()))
		{
			FOR_EACH_CITY(pLoopCity, kPlayer)
			{
				if (pLoopCity->plot()->isAdjacentToArea(pArea->getID()))
				{
					if (kPlayer.getTeam() == getID())
					{
						iTeamCities++;
						if (pLoopCity->waterArea() == pBiggestArea)
						{
							return true;
						}
					}
					else iOtherTeamCities++;
				}
			}
		}
		if (iTeamCities >= 2 && iOtherTeamCities >= 2)
			return true;
	}
	// BETTER_BTS_AI_MOD: END
	return false;
}
