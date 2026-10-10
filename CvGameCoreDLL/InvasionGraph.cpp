#include "CvGameCoreDLL.h"
#include "InvasionGraph.h"
#include "UWAIAgent.h"
#include "ArmamentForecast.h"
#include "MilitaryAnalyst.h"
#include "WarEvalParameters.h"
#include "UWAILogMuteState.h" // <!-- custom: Concrete nested-mute state is needed because the graph queries and temporarily changes the shared mute depth; it owns no logger. See KI#505.3. (ChatGPT-5.6-Sol) -->
#include "BBAILog.h" // <!-- custom: Level-3 WAR diagnostics trace why apparently feasible overseas invasions fail inside InvasionGraph. See KI#53.6. (ChatGPT-5.6-Sol) -->
#include "CoreAI.h"
#include "CvCity.h"
#include "CvPlot.h"
#include "CvArea.h"
#include "CvInfo_GameOption.h"

using std::vector;


namespace
{
	__inline scaled powerCorrect(scaled rMultiplier)
	{
		static scaled const rPOWER_ORRECTION = per100(GC.getDefineINT(CvGlobals::POWER_CORRECTION));
		return rMultiplier.pow(rPOWER_ORRECTION);
	}
}


InvasionGraph::InvasionGraph(MilitaryAnalyst& kMilitaryAnalyst, PlyrSet const& kWarParties, bool bPeaceScenario)
// <!-- custom: Share the MilitaryAnalyst evaluation's nested mute state; the graph itself owns no logger, formatter or file state. (ChatGPT-5.6-Sol) -->
:	m_kMA(kMilitaryAnalyst), m_kWarParties(kWarParties),
	m_kLogMuteState(m_kMA.evaluationParams().getLogMuteState()),
	m_bPeaceScenario(bPeaceScenario),
	m_bLossesDone(false), m_bAllWarPartiesKnown(false), m_bFirstSimulateCall(true),
	m_iTimeLimit(-1), m_eAgent(m_kMA.getAgentPlayer())
{
	if (gUWAIInvasionGraphLogLevel >= 2 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_INVASION_GRAPH_BEGIN turn=%d agentPlayer=%d scenario=%s warPartyCount=%d",
			GC.getGame().getGameTurn(), m_eAgent, m_bPeaceScenario ? "PEACE" : "WAR", (int)kWarParties.size());
	// (Barbarians and dead players will remain NULL)
	m_nodeMap.resize(MAX_PLAYERS, NULL);
	for (PlyrSetIter it = kWarParties.begin(); it != kWarParties.end(); ++it)
		m_nodeMap[*it] = new Node(*it, *this);
	for (PlyrSetIter it = kWarParties.begin(); it != kWarParties.end(); ++it)
		m_nodeMap[*it]->findAndLinkTarget();
	if (gUWAIInvasionGraphLogLevel >= 3 && !m_kLogMuteState.isMuted() && kWarParties.empty()) logBBAI("UWAI_INVASION_GRAPH_NO_WAR_PARTIES turn=%d agentPlayer=%d scenario=%s",
			GC.getGame().getGameTurn(), m_eAgent, m_bPeaceScenario ? "PEACE" : "WAR");
}


InvasionGraph::~InvasionGraph()
{
	for(size_t i = 0; i < m_nodeMap.size(); i++)
		SAFE_DELETE(m_nodeMap[i]);
}


void InvasionGraph::addFutureWarParties(PlyrSet const& kOurSide, PlyrSet const& kOurFutureOpponents)
{
	m_bAllWarPartiesKnown = true;
	if (kOurSide.empty())
		return; // Don't update targets unnecessarily
	for (PlyrSetIter it = kOurSide.begin(); it != kOurSide.end(); ++it)
	{
		FAssertBounds(0, MAX_CIV_PLAYERS, *it);
		if (m_nodeMap[*it] == NULL)
			m_nodeMap[*it] = new Node(*it, *this);
		m_nodeMap[*it]->addWarOpponents(kOurFutureOpponents);
	}
	for (PlyrSetIter it = kOurFutureOpponents.begin();
		it != kOurFutureOpponents.end(); ++it)
	{
		FAssertBounds(0, MAX_CIV_PLAYERS, *it);
		if (m_nodeMap[*it] == NULL)
			m_nodeMap[*it] = new Node(*it, *this);
		m_nodeMap[*it]->addWarOpponents(kOurSide);
	}
	/*	Finding targets for the new nodes doesn't necessarily suffice b/c
		phase 1 may have erased edges which may become valid again after
		the armament forecast. */
	if (gUWAIInvasionGraphLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_INVASION_GRAPH_FUTURE_WAR_PARTIES_ADDED turn=%d agentPlayer=%d ourSideCount=%d futureOpponentCount=%d",
			GC.getGame().getGameTurn(), m_eAgent, (int)kOurSide.size(), (int)kOurFutureOpponents.size());
	updateTargets();
}


void InvasionGraph::removeWar(PlyrSet const& kOurSide, PlyrSet const& kTheirSide)
{
	m_bAllWarPartiesKnown = true;
	if (kOurSide.empty())
		return;
	for (PlyrSetIter it = kOurSide.begin(); it != kOurSide.end(); ++it)
	{
		FAssertBounds(0, MAX_CIV_PLAYERS, *it);
		if (m_nodeMap[*it] != NULL)
			m_nodeMap[*it]->removeWarOpponents(kTheirSide);
	}
	for (PlyrSetIter it = kTheirSide.begin(); it != kTheirSide.end(); ++it)
	{
		FAssertBounds(0, MAX_CIV_PLAYERS, *it);
		if (m_nodeMap[*it] != NULL)
			m_nodeMap[*it]->removeWarOpponents(kOurSide);
	}
	if (gUWAIInvasionGraphLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_INVASION_GRAPH_WAR_REMOVED turn=%d agentPlayer=%d ourSideCount=%d theirSideCount=%d",
			GC.getGame().getGameTurn(), m_eAgent, (int)kOurSide.size(), (int)kTheirSide.size());
	updateTargets();
	/*	Don't delete any nodes; even if no longer a war party, may want to
		know their (peacetime) armament forecast. */
}


void InvasionGraph::updateTargets()
{
	if (gUWAIInvasionGraphLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_INVASION_GRAPH_TARGET_REASSIGN_BEGIN turn=%d agentPlayer=%d",
			GC.getGame().getGameTurn(), m_eAgent);
	for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
	{
		if (m_nodeMap[it->getID()] != NULL)
			m_nodeMap[it->getID()]->findAndLinkTarget();
	}
}


void InvasionGraph::addUninvolvedParties(PlyrSet const& kParties)
{
	for (PlyrSetIter it = kParties.begin(); it != kParties.end(); ++it)
	{
		FAssertBounds(0, MAX_CIV_PLAYERS, *it);
		if (m_nodeMap[*it] == NULL)
			m_nodeMap[*it] = new Node(*it, *this);
	}
}


InvasionGraph::Node::Node(PlayerTypes ePlayer, InvasionGraph const& kOuter)
// <!-- custom: Nodes share the outer graph's nested mute state; this reference carries no formatting or file state. (ChatGPT-5.6-Sol) -->
:	m_kOuter(kOuter), m_kLogMuteState(kOuter.m_kLogMuteState),
	m_eAgent(kOuter.m_eAgent), m_ePlayer(ePlayer),
	// I.e. the agent is going to cheat by using info from other players' caches
	m_kCache(GET_PLAYER(m_ePlayer).uwai().getCache()),
	/*	Needed frequently to sanitize cities from the (possibly outdated) caches
		of other players */
	m_kAgentCache(GET_PLAYER(m_eAgent).uwai().getCache()),
	m_bEliminated(false), m_bCapitulated(false), m_bHasClashed(false),
	m_iWarTurnsSimulated(0), m_pPrimaryTarget(NULL)
{
	// Node ids are used for vector.operator[]() accesses
	FAssertBounds(0, MAX_CIV_PLAYERS, ePlayer);

	// Properly initialized in prepareForSimulation
	m_iCacheIndex = -1;
	m_bComponentDone = false;

	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	CvTeamAI const& kTeam = GET_TEAM(kPlayer.getTeam());
	// Collect war opponents
	m_abWarOpponent.resize(MAX_CIV_PLAYERS, false);
	for (PlayerIter<MAJOR_CIV,ENEMY_OF> it(kTeam.getID()); it.hasNext(); ++it)
	{
		PlayerTypes const eEnemy = it->getID();
		// Exclude enemies of kTeam that the agent hasn't met
		if (kOuter.m_kWarParties.count(eEnemy) > 0)
		{
			m_warOpponents.insert(eEnemy);
			m_abWarOpponent[eEnemy] = true;
		}
	}
	for (int i = 0; i < NUM_BRANCHES; i++)
		m_arLostPower[i] = m_arShiftedPower[i] = 0;
	initMilitary();
	if (gUWAIInvasionGraphLogLevel >= 3 && !m_kLogMuteState.isMuted()) logPower("CURRENT");
}


void InvasionGraph::Node::initMilitary()
{
	//PROFILE_FUNC(); // Dynamic memory allocation doesn't seem to hurt performance
	// Copy present military from cache and split HOME_GUARD off from ARMY
	vector<MilitaryBranch*> const& kCacheMilitary = m_kCache.getPowerValues();
	MilitaryBranch::HomeGuard* pHomeGuard = new MilitaryBranch::HomeGuard(
			*kCacheMilitary[HOME_GUARD]);
	// <!-- custom: Use only conventional units for the same Home Guard ratio that partitions nuclear-free Army power.
	// Apply it only to Army's conventional headcount, then retain every separately counted nuclear unit. See KI#561. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	scaled const rGuardRatio = pHomeGuard->initTotals(m_kCache.numNonNavalUnits(),
			kCacheMilitary[ARMY]->power() - kCacheMilitary[NUCLEAR]->power());
	m_military.push_back(pHomeGuard);
	MilitaryBranch::Army* pArmy = new MilitaryBranch::Army(*kCacheMilitary[ARMY]);
	int const iNuclearUnits = kCacheMilitary[NUCLEAR]->numUnits();
	int const iConventionalArmyUnits = kCacheMilitary[ARMY]->numUnits() - iNuclearUnits;
	FAssert(iConventionalArmyUnits >= 0);
	pArmy->setTotals(
			iNuclearUnits + (std::max(0, iConventionalArmyUnits) * (1 - rGuardRatio)).round(),
			(1 - rGuardRatio) *
			(kCacheMilitary[ARMY]->power() - kCacheMilitary[NUCLEAR]->power()) +
			std::min(kCacheMilitary[NUCLEAR]->power(),
			/*	Limit contribution of nukes to invasions. (Someone needs to actually
				conquer and occupy the enemy cities.) */
			fixp(0.35) *
			(kCacheMilitary[ARMY]->power() - kCacheMilitary[NUCLEAR]->power())));
	m_military.push_back(pArmy);
	m_military.push_back(new MilitaryBranch::Fleet(*kCacheMilitary[FLEET]));
	m_military.push_back(new MilitaryBranch::Logistics(*kCacheMilitary[LOGISTICS]));
	m_military.push_back(new MilitaryBranch::Cavalry(*kCacheMilitary[CAVALRY]));
	m_military.push_back(new MilitaryBranch::NuclearArsenal(*kCacheMilitary[NUCLEAR]));
	/*	Remember the current values (including home guard, which isn't in cache)
		for computing GainedPower later */
	for (size_t i = 0; i < m_military.size(); i++)
		m_arCurrentPow.push_back(m_military[i]->power());
}


InvasionGraph::Node::~Node()
{
	for (size_t i = 0; i < m_military.size(); i++)
		delete m_military[i];
}


void InvasionGraph::Node::addWarOpponents(PlyrSet const& kWarOpponents)
{
	for (PlyrSetIter it = kWarOpponents.begin(); it != kWarOpponents.end(); ++it)
	{
		if (!m_abWarOpponent[*it] &&
			/*	Should arguably be guaranteed somewhere higher up; not currently
				guaranteed when there is a holy war vote. */
			GET_TEAM(m_ePlayer).getMasterTeam() != GET_TEAM(*it).getMasterTeam())
		{
			m_abWarOpponent[*it] = true;
			m_warOpponents.insert(*it);
		}
	}
	FAssertMsg(!m_warOpponents.empty(), "Node w/o war opponents");
}


void InvasionGraph::Node::removeWarOpponents(PlyrSet const& kWarOpponents)
{
	for (PlyrSetIter it = kWarOpponents.begin(); it != kWarOpponents.end(); ++it)
	{
		if (m_abWarOpponent[*it])
		{
			m_abWarOpponent[*it] = false;
			m_warOpponents.erase(*it);
		}
	}
}


// <!-- custom: szStage is emitted as the structured stage= field so callers pass stable tokens rather than prose prefixes. (ChatGPT-5.6-Sol) -->
void InvasionGraph::Node::logPower(char const* szStage) const
{
	FAssert(gUWAIInvasionGraphLogLevel >= 3 && !m_kLogMuteState.isMuted());
	int iLogged = 0;
	for (size_t i = 0; i < m_military.size(); i++)
	{
		int iPow = (m_military[i]->power() - m_arLostPower[i]).round();
		if (iPow <= 0)
		{
			// -5 as threshold b/c minor errors in the simulation aren't worth fixing
			FAssertMsg(iPow > -5, "More lost power than there was to begin with");
			continue;
		}
		logBBAI("UWAI_INVASION_GRAPH_POWER turn=%d agentPlayer=%d player=%d stage=%s branch=%s power=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, szStage, m_military[i]->str(), iPow);
		iLogged++;
	}
	if (iLogged == 0) logBBAI("UWAI_INVASION_GRAPH_POWER_NONE turn=%d agentPlayer=%d player=%d stage=%s",
			GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, szStage);
}


void InvasionGraph::Node::findAndLinkTarget()
{
	PROFILE_FUNC();
	PlayerTypes eTarget = NO_PLAYER;
	bool bCanHaveTarget = true;
	bool const bLogInvasionDetail = (gUWAIInvasionGraphLogLevel >= 3 && !m_kLogMuteState.isMuted());
	if (isEliminated() || hasCapitulated())
	{
		bCanHaveTarget = false;
		if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_TARGET_UNAVAILABLE turn=%d agentPlayer=%d player=%d reason=DEFEATED_OR_CAPITULATED",
			GC.getGame().getGameTurn(), m_eAgent, m_ePlayer);
	}
	if (bCanHaveTarget)
	{
		eTarget = findTarget();
		if (eTarget == NO_PLAYER)
		{
			if (bLogInvasionDetail && m_warOpponents.empty()) logBBAI("UWAI_INVASION_GRAPH_TARGET_NONE_NO_WAR_OPPONENTS turn=%d agentPlayer=%d player=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer);
			else if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_TARGET_NONE_UNREACHABLE turn=%d agentPlayer=%d player=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer);
		}
	}
	if (eTarget == NO_PLAYER)
	{
		if (m_pPrimaryTarget != NULL)
		{
			m_pPrimaryTarget->m_targetedBy.erase(m_ePlayer);
			if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_TARGET_CLEARED turn=%d agentPlayer=%d player=%d oldTarget=%d",
					GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, m_pPrimaryTarget->m_ePlayer);
			m_pPrimaryTarget = NULL;
		}
		return;
	}
	if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_TARGET_ASSUMED turn=%d agentPlayer=%d player=%d target=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, eTarget);
	if (m_pPrimaryTarget != NULL && m_pPrimaryTarget->m_ePlayer != eTarget)
	{
		m_pPrimaryTarget->m_targetedBy.erase(m_ePlayer);
		if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_TARGET_SWITCH turn=%d agentPlayer=%d player=%d oldTarget=%d newTarget=%d",
			GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, m_pPrimaryTarget->m_ePlayer, eTarget);
	}
	m_pPrimaryTarget = m_kOuter.m_nodeMap[eTarget];
	if (m_pPrimaryTarget == NULL)
	{
		FAssert(!GET_TEAM(m_eAgent).isHasMet(TEAMID(eTarget)));
		if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_TARGET_IGNORED_UNMET turn=%d agentPlayer=%d player=%d target=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, eTarget);
	}
	else m_pPrimaryTarget->m_targetedBy.insert(m_ePlayer);
}


PlayerTypes InvasionGraph::Node::findTarget(TeamTypes eExtra) const
{
	PlayerTypes eBestTarget = NO_PLAYER;
	/*	W/e the current target is according to unit missions, assume it
		will change once war is declared. */
	bool bSearchedBest = false;
	if (m_kOuter.m_bAllWarPartiesKnown || eExtra != NO_TEAM)
	{
		bSearchedBest = true;
		eBestTarget = findBestTarget(eExtra);
		if (eBestTarget == NO_PLAYER)
			return NO_PLAYER;
		if (!GET_TEAM(m_ePlayer).isAtWar(TEAMID(eBestTarget)))
			return eBestTarget;
		// Otherwise, check unit missions, but may still fall back on eBestTarget.
	}
	/*	If unit missions can matter, then don't search for the best target yet -
		b/c that's expensive. */
	PlayerTypes eMostMissions = NO_PLAYER; // Against whom this Node has the most missions
	// Tbd., possibly: Use a fraction of m_kCache.numNonNavalUnits() as the threshold
	int iMaxMissions = 3;
	for (PlyrSetIter it = m_warOpponents.begin(); it != m_warOpponents.end(); ++it)
	{
		PlayerTypes eOpponent = *it;
		// Don't cheat too much with visibility
		if (m_ePlayer != m_eAgent && eOpponent != m_eAgent)
			continue;
		int iMissions = m_kCache.targetMissionCount(eOpponent);
		if (iMissions <= iMaxMissions)
			continue;
		// Same conditions as in isValidTarget
		if (m_kOuter.m_nodeMap[eOpponent] == NULL ||
			m_kOuter.m_nodeMap[eOpponent]->isEliminated() ||
			m_kOuter.m_nodeMap[eOpponent]->hasCapitulated())
		{
			continue;
		}
		// Still need to ensure that oppId has a valid city
		bool bValidFound = false;
		/*	In large games, looking up eOpponent's cities is faster than a pass
			through the whole m_kCache. */
		CvPlayer const& kOpponent = GET_PLAYER(eOpponent);
		FOR_EACH_CITY(pCity, kOpponent)
		{
			UWAICache::City* pCacheCity = m_kCache.lookupCity(pCity->plotNum());
			if (pCacheCity != NULL && pCacheCity->canReach() &&
				!m_kOuter.m_nodeMap[eOpponent]->hasLost(pCacheCity->getID()) &&
				m_kAgentCache.lookupCity(pCacheCity->getID()) != NULL)
			{
				bValidFound = true;
				break;
			}
		}
		if (!bValidFound)
			continue;
		iMaxMissions = iMissions;
		eMostMissions = eOpponent;
	}
	if (eMostMissions != NO_PLAYER)
	{
		if (gUWAIInvasionGraphLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_INVASION_GRAPH_TARGET_FROM_UNIT_MISSIONS turn=%d agentPlayer=%d player=%d target=%d missionCount=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, eMostMissions, iMaxMissions);
		return eMostMissions;
	}
	// Fall back on bestTarget.
	if (eBestTarget != NO_PLAYER)
		return eBestTarget;
	if (bSearchedBest)
		return NO_PLAYER;
	return findBestTarget(eExtra);
}


PlayerTypes InvasionGraph::Node::findBestTarget(TeamTypes eExtra) const
{
	/*	Find the city with the highest targetValue and check which potential target
		has reachable cities left. (City::canReach says which ones are reachable in
		the actual game state; doesn't take into account Node::hasLost.) */
	/*	Bias the search toward the agent. But only if already at war -
		don't want the AI to start a war based on an unrealistic assumption
		that the war will distract the enemy from a different target. */
	bool const bPessimistic = (GET_TEAM(m_eAgent).isAtWar(TEAMID(m_ePlayer)) &&
			isValidTarget(m_eAgent, eExtra));
	int iSkipped = 0;
	/*	For every 2 cities of the invader, pessimistic search may ignore up to one city
		that is a better target than any city owned by the agent. */
	int iMaxSkip = GET_PLAYER(m_ePlayer).getNumCities() / 2;
	PlayerTypes eBestTarget = NO_PLAYER;
	for (int i = 0; i < m_kCache.numCities(); i++)
	{
		UWAICache::City& kCacheCity = m_kCache.cityAt(i);
		// Because of ordering, first hit is best target (if any target is valid).
		if (!kCacheCity.canReach() || kCacheCity.isOwnTeamCity())
			break;
		if (isValidTarget(kCacheCity, eExtra))
		{
			eBestTarget = kCacheCity.city().getOwner();
			if (eBestTarget == m_eAgent || !bPessimistic || iSkipped >= iMaxSkip)
				break;
			iSkipped++;
		}
	}
	if (iSkipped > 0)
	{
		if (gUWAIInvasionGraphLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_INVASION_GRAPH_TARGET_BIAS_SKIP turn=%d agentPlayer=%d player=%d skippedCities=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, iSkipped);
	}
	return eBestTarget;
}


bool InvasionGraph::Node::isValidTarget(UWAICache::City const& kCacheCity, TeamTypes eExtra) const
{
	PlayerTypes const eOwner = kCacheCity.city().getOwner();
	return (isValidTarget(eOwner, eExtra) &&
			!m_kOuter.m_nodeMap[eOwner]->hasLost(kCacheCity.getID()) &&
			m_kAgentCache.lookupCity(kCacheCity.getID()) != NULL);
}


bool InvasionGraph::Node::isValidTarget(PlayerTypes eTarget, TeamTypes eExtra) const
{
	return !(eTarget == NO_PLAYER || (!m_abWarOpponent[eTarget] &&
			(eExtra == NO_TEAM ||
			GET_PLAYER(eTarget).getMasterTeam() != GET_TEAM(eExtra).getMasterTeam())) ||
			m_kOuter.m_nodeMap[eTarget] == NULL ||
			// Important for phase II:
			m_kOuter.m_nodeMap[eTarget]->isEliminated() ||
			m_kOuter.m_nodeMap[eTarget]->hasCapitulated());
}


size_t InvasionGraph::Node::findCycle(vector<Node*>& kPath)
{
	// Check path to detect cycle
	for (size_t i = 0; i < kPath.size(); i++)
	{
		if (kPath[i]->m_ePlayer == m_ePlayer)
			return i;
	}
	kPath.push_back(this);
	if (m_pPrimaryTarget == NULL)
		return kPath.size();
	return m_pPrimaryTarget->findCycle(kPath);
}


void InvasionGraph::Node::resolveLossesRec()
{
	m_bComponentDone = true;
	if (m_targetedBy.empty())
		return;
	// Copy targetedBy b/c the loop may remove elements
	PlyrSet targetedByOld;
	targetedByOld.insert(m_targetedBy.begin(), m_targetedBy.end());
	for (PlyrSetIter it = targetedByOld.begin(); it != targetedByOld.end(); ++it)
	{
		if (m_targetedBy.count(*it) == 0) // Check if current element has been removed
			continue;
		m_kOuter.m_nodeMap[*it]->resolveLossesRec();
	}
	resolveLosses();
}


void InvasionGraph::Node::addConquest(UWAICache::City const& kConqCity)
{
	if (gUWAIInvasionGraphLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_INVASION_GRAPH_CITY_CONQUERED turn=%d agentPlayer=%d attacker=%d defender=%d city=%S cityX=%d cityY=%d",
			GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kConqCity.city().getOwner(), kConqCity.city().getName().GetCString(),
			kConqCity.city().getX(), kConqCity.city().getY());
	m_conquests.push_back(&kConqCity);
	// Advance cache index past the city just conquered
	while (m_iCacheIndex < m_kCache.numCities())
	{
		 UWAICache::City& kLoopCity = m_kCache.cityAt(m_iCacheIndex);
		 if (kLoopCity.getID() == kConqCity.getID())
			break;
		 m_iCacheIndex++;
	}
	m_iCacheIndex++;
}


void InvasionGraph::Node::prepareForSimulation()
{
	PROFILE_FUNC();
	m_iCacheIndex = 0;
	//m_kCache.sortCitiesByAttackPriority(); // Done in UWAICache now
	m_bComponentDone = false;
	m_rEmergencyDefPow = 0;
	m_rDistractionByConquest = 0;
	m_rDistractionByDefense = 0;
	m_rTempArmyLosses = 0;
}


void InvasionGraph::Node::logTypicalUnits()
{
	// Should no longer be needed; now done in initMilitary (where it belongs)
	/*if(m_military[HOME_GUARD] != NULL &&
		m_military[HOME_GUARD]->getTypicalUnit() == NO_UNIT)
		m_military[HOME_GUARD]->updateTypicalUnit();*/
	FAssert(gUWAIInvasionGraphLogLevel >= 3 && !m_kLogMuteState.isMuted());
	// Log typical units only once per evaluation (they don't change)
	if (m_kOuter.m_bPeaceScenario || (m_kOuter.m_bAllWarPartiesKnown &&
		m_kOuter.m_kMA.evaluationParams().getPreparationTime() > 0))
	{
		return;
	}
	for (size_t i = 0; i < m_military.size(); i++)
	{
		MilitaryBranch const& kBranch = *m_military[i];
		UnitTypes const eUnit = kBranch.getTypicalUnit();
		if (eUnit == NO_UNIT)
			continue;
		CvUnitInfo const& kUnit = GC.getInfo(eUnit);
		int const iActualCost = kUnit.getProductionCost();
		int const iActualPow = kBranch.getTypicalPower().round();
		int const iAgentCost = kBranch.getTypicalCost(TEAMID(m_eAgent)).round();
		int const iAgentPow = kBranch.getTypicalPower(TEAMID(m_eAgent)).round();
		/*	(iAgentCost and iActualCost often won't match b/c iActualCost here
			ignores handicap) */
		// <!-- custom: The branch's actual typical unit and the agent-view estimate describe one model input; keep both on one row instead of emitting a second row only when power differs. (ChatGPT-5.6-Sol) -->
		logBBAI("UWAI_INVASION_GRAPH_TYPICAL_UNIT turn=%d agentPlayer=%d player=%d branch=%s unit=%S actualCost=%d actualPower=%d agentEstimatedCost=%d agentEstimatedPower=%d estimatePowerDiffers=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kBranch.str(), GC.getInfo(eUnit).getDescription(),
				iActualCost, iActualPow, iAgentCost, iAgentPow, iAgentPow != iActualPow);
	}
}


void InvasionGraph::Node::predictArmament(int iTurns, bool bNoUpgrading)
{
	bool const bLogInvasionDetail = (gUWAIInvasionGraphLogLevel >= 3 && !m_kLogMuteState.isMuted());
	if (isEliminated())
	{
		if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_ARMAMENT_SKIPPED turn=%d agentPlayer=%d player=%d reason=ELIMINATED",
			GC.getGame().getGameTurn(), m_eAgent, m_ePlayer);
		return;
	}
	// Target city assumed for the forecast (to decide on naval build-up)
	UWAICache::City const* pTargetCity = targetCity();
	WarEvalParameters const& kParams = m_kOuter.m_kMA.evaluationParams();
	TeamTypes const eMasterTeam = GET_TEAM(m_ePlayer).getMasterTeam();
	/*	Simulated DoW is taken into account by targetCity, but a
		(simulated) warplan isn't. */
	if (eMasterTeam == GET_TEAM(m_eAgent).getMasterTeam() ||
		kParams.isWarAlly(eMasterTeam))
	{
		TeamTypes eTarget = kParams.getTarget();
		if (!m_kOuter.m_bPeaceScenario && !m_kOuter.m_bAllWarPartiesKnown &&
			!GET_TEAM(m_eAgent).isAtWar(eTarget))
		{
			m_kLogMuteState.pushMute();
			PlayerTypes eActualTarget = findTarget(eTarget);
			pTargetCity = targetCity(eActualTarget);
			m_kLogMuteState.popMute();
		}
	}
	ArmamentForecast forec(m_ePlayer, m_kOuter.m_kMA, m_military, iTurns,
			m_kOuter.m_bPeaceScenario, bNoUpgrading,
			!m_kOuter.m_bLossesDone, m_kOuter.m_bAllWarPartiesKnown,
			pTargetCity, productionPortion());
	m_rProductionInvested += forec.getProductionInvested();
	if (bLogInvasionDetail) logPower("PREDICTED");
}


scaled InvasionGraph::Node::productionPortion() const
{
	int iLostPop = 0;
	for (CitySetIter it = m_cityLosses.begin(); it != m_cityLosses.end(); ++it)
	{
		CvCity& kLostCity = UWAICache::cvCityById(*it);
		iLostPop += kLostCity.getPopulation();
	}
	int iOriginalPop = GET_PLAYER(m_ePlayer).getTotalPopulation();
	if (iOriginalPop <= 0)
		return 0;
	FAssert(iLostPop <= iOriginalPop);
	return scaled(iOriginalPop - iLostPop, iOriginalPop);
}


// <!-- custom: Subtract graph-local city losses from real-map area counts so later simulated battles cannot draw mobile Army or rallied garrisons from already-conquered cities. See KI#592. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
int InvasionGraph::Node::remainingCitiesInArea(CvArea const& kArea) const
{
	int iRemaining = kArea.getCitiesPerPlayer(m_ePlayer);
	for (CitySetIter it = m_cityLosses.begin(); it != m_cityLosses.end(); ++it)
	{
		if (UWAICache::cvCityById(*it).isArea(kArea))
			iRemaining--;
	}
	FAssert(iRemaining >= 0);
	return std::max(0, iRemaining);
}


// For calculating threat values when breaking cycles. Attacks against lightly defended cities shouldn't result in larger threat values.
// <!-- custom: hoisted from multiline signature between `bClashOnly` and `bUniformGarrisons` by collapse_cpp_signatures.py. (GPT-5.5 (reviewed script output)) -->
SimulationStep* InvasionGraph::Node::step(scaled rArmyPortionDefender, scaled rArmyPortionAttacker, bool bClashOnly, bool bUniformGarrisons) const
{
	PROFILE_FUNC();
	UWAICache::City const* const pCacheCity = (bClashOnly ? NULL : targetCity());
	if (pCacheCity == NULL && !bClashOnly)
	{
		// <!-- custom: A missing target city ends the simulated invasion before Fleet, Logistics or city-combat strength can matter.
		// Log this rare terminal state for the evaluated agent/target pair so KI#53.6 can distinguish reachability/target-selection failures from pessimistic combat simulation. (ChatGPT-5.6-Sol) -->
		WarEvalParameters const& kEvalParams = m_kOuter.m_kMA.evaluationParams();
		CvTeamAI const& kAgentTeam = GET_TEAM(TEAMID(m_eAgent));
		CvTeamAI const& kTargetTeam = GET_TEAM(kEvalParams.getTarget());
		if (gWarLogLevel >= 3 && !m_kOuter.m_bPeaceScenario && m_ePlayer == m_eAgent && m_pPrimaryTarget != NULL &&
			TEAMID(m_pPrimaryTarget->m_ePlayer) == kEvalParams.getTarget() && !kAgentTeam.isAtWar(kTargetTeam.getID()) &&
			kTargetTeam.getDefensivePower(kAgentTeam.getID()) <= kAgentTeam.getPower(true) && m_military[LOGISTICS]->power() > 0)
		{
			logBBAI("WAR_NAVAL_INVASION_STEP turn=%d agentTeam=%d targetTeam=%d total=%d prepTurns=%d stage=NO_TARGET_CITY",
				GC.getGame().getGameTurn(), TEAMID(m_eAgent), kEvalParams.getTarget(), kEvalParams.isTotal(), kEvalParams.getPreparationTime());
		}
		return NULL;
	}
	CvCity const* const pCity = (pCacheCity == NULL ? NULL : &pCacheCity->city());
	Node& kDefender = *m_pPrimaryTarget;
	int const iDefCities = GET_PLAYER(kDefender.m_ePlayer).getNumCities();
	int const iAttCities = GET_PLAYER(m_ePlayer).getNumCities();
	scaled rConfAlliesAtt = 1, rConfAlliesDef = 1;
	// If the portion is 0, then the army should be ignored by all leaders.
	if (m_ePlayer == m_eAgent && rArmyPortionDefender > 0)
		rConfAlliesAtt = GET_PLAYER(m_ePlayer).uwai().warConfidenceAllies();
	if (kDefender.m_ePlayer == m_eAgent && rArmyPortionAttacker > 0)
		rConfAlliesDef = GET_PLAYER(kDefender.m_ePlayer).uwai().warConfidenceAllies();
	/*	NB: There's also InvasionGraph::willingness, which is applied before
		computing the army portions. */
	/*	Adjust the portion of the army that is assumed to be absent,
		i.e. 1 minus portion */
	// <!-- custom: AdvCiv practical 2923 accidentally changed these assignments to multiplication, squaring every partial portion at neutral ally confidence. Restore the original absent-portion adjustment. See KI#579. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	rArmyPortionDefender = scaled::max(0, 1 - (1 - rArmyPortionDefender) *
			rConfAlliesAtt);
	rArmyPortionAttacker = scaled::max(0, 1 - (1 - rArmyPortionAttacker) *
			rConfAlliesDef);
	// No clash w/o mutual reachability
	FAssert(!bClashOnly || (targetCity() != NULL && kDefender.targetCity() != NULL));
	// <!-- custom: The UWAI gates are not needed by the missing-target terminal path above; cache them after that early return, immediately before the first combat diagnostic block rather than before its gameplay setup. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	bool const bLogInvasionSummary = (gUWAIInvasionGraphLogLevel >= 1 && !m_kLogMuteState.isMuted());
	bool const bLogInvasionDetail = (gUWAIInvasionGraphLogLevel >= 3 && !m_kLogMuteState.isMuted());
	if (bClashOnly)
	{
		if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_COMBAT_BEGIN turn=%d agentPlayer=%d attacker=%d defender=%d mode=CLASH",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer);
	}
	else
	{
		if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_COMBAT_BEGIN_CITY turn=%d agentPlayer=%d attacker=%d defender=%d mode=CITY_ATTACK cityX=%d cityY=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer,
				pCity->getX(), pCity->getY());
	}
	if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_ARMY_PORTIONS turn=%d agentPlayer=%d attacker=%d defender=%d attackerPercent=%d defenderPercent=%d",
			GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer,
			rArmyPortionAttacker.getPercent(), rArmyPortionDefender.getPercent());
	// Only log if portions are non-trivial
	if (rArmyPortionDefender != 0 && rArmyPortionDefender != 1 && rConfAlliesAtt != 1)
	{
		if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_ALLY_CONFIDENCE_ATTACKER turn=%d agentPlayer=%d attacker=%d defender=%d percent=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer,
				rConfAlliesAtt.getPercent());
	}
	if (rArmyPortionAttacker != 0 && rArmyPortionAttacker != 1 && rConfAlliesDef != 1)
	{
		if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_ALLY_CONFIDENCE_DEFENDER turn=%d agentPlayer=%d attacker=%d defender=%d percent=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer,
				rConfAlliesDef.getPercent());
	}
	SimulationStep& kStep = *new SimulationStep(m_ePlayer, pCacheCity);
	scaled rArmyPowRaw = m_military[ARMY]->power() - m_arLostPower[ARMY];
	scaled rCavPowRaw = m_military[CAVALRY]->power() - m_arLostPower[CAVALRY];
	scaled rCavRatioRaw;
	if (rArmyPowRaw > 0)
	{
		rCavRatioRaw = rCavPowRaw / (rArmyPowRaw +
				m_military[HOME_GUARD]->power() - m_arLostPower[HOME_GUARD]);
		rCavRatioRaw.clamp(0, 1);
	}
	rArmyPowRaw -= m_rTempArmyLosses;
	rCavPowRaw -= m_rTempArmyLosses * rCavRatioRaw;
	scaled rArmyPow = rArmyPowRaw * rArmyPortionAttacker;
	rArmyPow.increaseTo(0);
	scaled rCavPow = rCavPowRaw * rArmyPortionAttacker;
	/*	Cavalry being used defensively (counter) is not uncommon, and then
		rCavPow can be greater than rArmyPow. (Still assume that cavalry is used
		aggressively most of the time.) */
	rCavPow.decreaseTo(rArmyPow);
	// (kDefender.m_rTempArmyLosses assumed to be available for defense)
	scaled rDefArmyPow = (kDefender.m_military[ARMY]->power()
			- kDefender.m_arLostPower[ARMY]) * rArmyPortionDefender;
	scaled rDefCavPow = (kDefender.m_military[CAVALRY]->power()
			- kDefender.m_arLostPower[CAVALRY]) * rArmyPortionDefender;
	rDefArmyPow.increaseTo(0);
	rDefCavPow.decreaseTo(rDefArmyPow);
	/*	Notes regarding bNaval, upon re-reading the code some months after writing it:
	bClashOnly: bNaval if neither side can reach its target city by land; then
		the clash is between fleets only. If !bNaval, then the two armies are
		assumed to meet each other halfway in the largest shared area and fleets
		don't play any role.
	!bClashOnly: bNaval if the attacker (*this) can't reach its target city by land.
		Then there is a naval battle (same as fleet clash above) and the
		surviving cargo of the attacker proceeds as described under !bNaval:
		If !bNaval, then some naval fighting can happen along the coast. A clash may
		occur near the target city, but it's skipped if the defending army is too weak.
		If the attacker wins the clash, a siege of the target city is simulated. */
	bool bNaval = false;
	if (bClashOnly)
	{
		UWAICache::City const* pDefTargetCity = kDefender.targetCity();
		UWAICache::City const* pAttTargetCity = targetCity();
		if (pDefTargetCity != NULL && pAttTargetCity != NULL)
		{
			/*	Not clear whether reachability from a primary area
				should suffice for !bNaval. As a compromise, check reachability
				from capital when clashing but not when conquering cities. */
			bNaval = (!canReachByLand(pAttTargetCity->getID(), true) &&
					!kDefender.canReachByLand(pDefTargetCity->getID(), true));
		}
		else FErrorMsg("Shouldn't clash when not mutually reachable");
	}
	else bNaval = !canReachByLand(pCacheCity->getID(), false);
	WarEvalParameters const& kEvalParams = m_kOuter.m_kMA.evaluationParams();
	// <!-- custom: Keep the step-level naval diagnostic to a plausible weaker overseas target that the agent is not already fighting and for which Logistics exists.
	// The actual cached deployment distance is logged rather than used as another filter, so a bad distance estimate cannot hide the failure we're trying to diagnose.
	// Only the real defended city-attack step is logged below; zero-defense priority probes are intentionally skipped. See KI#53.6. (ChatGPT-5.6-Sol) -->
	CvTeamAI const& kSASAgentTeam = GET_TEAM(TEAMID(m_eAgent));
	CvTeamAI const& kSASTargetTeam = GET_TEAM(kEvalParams.getTarget());
	bool const bSASLogNavalInvasionStep = (gWarLogLevel >= 3 && !m_kOuter.m_bPeaceScenario && m_ePlayer == m_eAgent && !bClashOnly && bNaval &&
		TEAMID(kDefender.m_ePlayer) == kEvalParams.getTarget() && !kSASAgentTeam.isAtWar(kSASTargetTeam.getID()) &&
		kSASTargetTeam.getDefensivePower(kSASAgentTeam.getID()) <= kSASAgentTeam.getPower(true) && m_military[LOGISTICS]->power() > 0);
	scaled rSASFleetPow = 0, rSASDefFleetPow = 0, rSASCargoCap = 0, rSASSurvivingCargo = 0, rSASArmySize = 0;
	scaled rNavalLandingRatio = 1;
	scaled rSASNavalAreaBonus = 0;
	bool bSASFleetWin = true;
	bool bCanBombard = false;
	bool bCanBombardFromSea = false;
	bool bCanSoften = false;
	for (int i = 0; i < NUM_BRANCHES; i++)
	{
		MilitaryBranchTypes const eBranch = (MilitaryBranchTypes)i;
		if (m_military[eBranch]->canSoftenCityDefenders())
			bCanSoften = true;
		if (m_military[eBranch]->canBombard())
		{
			if (eBranch != FLEET)
				bCanBombard = true;
			else if (!bClashOnly && pCity->isCoastal())
				bCanBombardFromSea = true;
		}
	}
	// Optimism/ pessimism, learning from past success/ failure
	scaled rConfAttPers = 1, rConfDefPers = 1;
	if (m_ePlayer == m_eAgent || kDefender.m_ePlayer == m_eAgent)
	{
		bool const bTotal = m_kOuter.m_kMA.evaluationParams().isTotal();
		if (m_ePlayer == m_eAgent)
		{
			rConfAttPers = GET_PLAYER(m_eAgent).uwai().warConfidencePersonal(
					bNaval, bTotal, kDefender.m_ePlayer);
		}
		else if (kDefender.m_ePlayer == m_eAgent)
		{
			rConfDefPers = GET_PLAYER(m_eAgent).uwai().warConfidencePersonal(
					// Defense doesn't hinge on navies and fighting in faraway lands
					false, bTotal, m_ePlayer);
		}
	}
	if (GET_PLAYER(m_ePlayer).isHuman())
		rConfDefPers *= GET_PLAYER(kDefender.m_ePlayer).uwai().confidenceAgainstHuman();
	if (GET_PLAYER(kDefender.m_ePlayer).isHuman())
		rConfAttPers *= GET_PLAYER(m_ePlayer).uwai().confidenceAgainstHuman();
	if (rConfAttPers != 1 || rConfDefPers != 1)
	{
		if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_PERSONAL_CONFIDENCE turn=%d agentPlayer=%d attacker=%d defender=%d attackerPercent=%d defenderPercent=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer,
				rConfAttPers.getPercent(), rConfDefPers.getPercent());
	}
	scaled rConfAtt = rConfAttPers;
	scaled rConfDef = rConfDefPers;
	// Assume that we don't know war successes of third parties
	if (m_ePlayer == m_eAgent || kDefender.m_ePlayer == m_eAgent)
	{
		scaled rConfAttLearned, rConfDefLearned = 0;
		if (m_ePlayer == m_eAgent)
		{
			rConfAttLearned = (GET_PLAYER(m_ePlayer).uwai().
					warConfidenceLearned(kDefender.m_ePlayer, true) - 1);
		}
		if (kDefender.m_ePlayer == m_eAgent)
		{
			rConfDefLearned = (GET_PLAYER(kDefender.m_ePlayer).uwai().
					warConfidenceLearned(m_ePlayer, false) - 1);
		}
		if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_LEARNED_CONFIDENCE turn=%d agentPlayer=%d attacker=%d defender=%d attackerPercent=%d defenderPercent=%d",
					GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer,
					rConfAttLearned.getPercent() + 100, rConfDefLearned.getPercent() + 100);
		/*	To avoid extreme bias, don't multiply with rConf...Pers unless
			learned and personal confidence contradict each other. */
		if ((rConfAttLearned < 0) != (rConfAtt < 1) &&
			rConfAtt != 1 && rConfAttLearned != 0)
		{
			rConfAttLearned *= 1 + (1 - rConfAtt).abs();
		}
		if ((rConfDefLearned < 0) != (rConfDef < 1) &&
			rConfDef != 1 && rConfDefLearned != 0)
		{
			rConfDefLearned *= 1 + (1 - rConfDef).abs();
		}
		rConfAtt += rConfAttLearned;
		rConfDef += rConfDefLearned;
	}
	rConfAtt.clamp(fixp(0.4), fixp(1.8));
	rConfDef.clamp(fixp(0.4), fixp(1.8));
	// Coastal battle (much overlap with clash code)
	if (!bClashOnly && bCanBombardFromSea && !bNaval)
	{
		/*	Only defender is assumed to bring cargo units into battle.
			Fixme: [LOGISTICS]->power is the cargo capacity. The military power
			is greater, but currently not tracked. */
		scaled rFleetPow =
				(m_military[FLEET]->power() - m_military[LOGISTICS]->power()
				- m_arLostPower[FLEET] + m_arLostPower[LOGISTICS]) *
				// Use attacker/ defender portion also for fleet
				rConfAtt * rArmyPortionAttacker;
		rFleetPow.increaseTo(0);
		scaled rDefFleetPow =
				(kDefender.m_military[FLEET]->power() - kDefender.m_arLostPower[FLEET]) *
				rConfDef * rArmyPortionDefender;
		rDefFleetPow.increaseTo(0);
		// <!-- custom: The coastal defender brings combat-capable transports into this Fleet battle. Preserve their overlapping cargo-capacity loss for later naval invasions instead of making destroyed transports available again.
		// This retains UWAI's existing cargo/Fleet dimensional approximation. See KI#587. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		scaled rDefLogisticsPortion = 0;
		if (rDefFleetPow > 1)
		{
			rDefLogisticsPortion = (kDefender.m_military[LOGISTICS]->power()
					- kDefender.m_arLostPower[LOGISTICS]) * rConfDef / rDefFleetPow;
		}
		/*	Don't factor in distance; naval units tend to be fast;
			would have to use a special metric b/c City::getDistance is
			for land units only. */
		bool const bAttWin = (rFleetPow > rDefFleetPow);
		// bAtt=false: Defender has no mobility advantage at sea
		std::pair<scaled,scaled> rrLossesWL = clashLossesWinnerLoser(rFleetPow,
				rDefFleetPow, false, true);
		scaled rLossesAtt, rLossesDef;
		if (bAttWin)
		{
			rLossesAtt = rrLossesWL.first / rConfAtt;
			rLossesDef = rrLossesWL.second / rConfDef;
		}
		else
		{
			rLossesAtt = rrLossesWL.second / rConfAtt;
			rLossesDef = rrLossesWL.first / rConfDef;
			if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_SEA_BOMBARDMENT_AVERTED turn=%d agentPlayer=%d attacker=%d defender=%d",
					GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer);
			bCanBombardFromSea = false;
		}
		kStep.reducePower(m_ePlayer, FLEET, rLossesAtt);
		kStep.reducePower(kDefender.m_ePlayer, FLEET, rLossesDef);
		scaled rDefLogisticsLosses = rLossesDef * rDefLogisticsPortion;
		rDefLogisticsLosses.decreaseTo(kDefender.m_military[LOGISTICS]->power()
				- kDefender.m_arLostPower[LOGISTICS]);
		kStep.reducePower(kDefender.m_ePlayer, LOGISTICS, rDefLogisticsLosses);
		if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_SEA_BATTLE_LOSSES_FLEET turn=%d agentPlayer=%d attacker=%d defender=%d attackerLoss=%d defenderLoss=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer,
				rLossesAtt.round(), rLossesDef.round());
	}
	if (bCanBombardFromSea)
		bCanBombard = true;
	bool bCavalryAttack = (!bClashOnly && !bCanBombard && !bNaval &&
			!bCanSoften && !bUniformGarrisons &&
		/*	Minor inconsistency: A cavalry attack might be assumed when assessing
			priority b/c then rDefArmyPow is 0, but for the actual simulation,
			no cavalry attack would be assumed. */
			rCavPow > fixp(2/3.) * rArmyPow && rCavPow > rDefArmyPow);
	if (!bClashOnly)
	{
		if (bCavalryAttack)
			rArmyPow = rCavPow;
		if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_FIELD_ARMY_POWER turn=%d agentPlayer=%d attacker=%d defender=%d attackerPower=%d defenderPower=%d cavalryAttack=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer,
				rArmyPow.round(), rDefArmyPow.round(), bCavalryAttack);
	}
	bool const bSneakAttack = isSneakAttack(kDefender, bClashOnly);
	bool const bAttackerUnprepared = kDefender.isSneakAttack(*this, false);
	scaled rDeploymentDistDefender;
	scaled rDeploymentDistAttacker;
	scaled rMaxWarMinAdjLandFactor = 1;
	if (m_ePlayer == m_eAgent)
	{
		// in the interval [0,4]
		int iMaxWarMinAdjLand = GC.getInfo(GET_PLAYER(m_ePlayer).
				getPersonalityType()).getMaxWarMinAdjacentLandPercent();
		rMaxWarMinAdjLandFactor = scaled(
				std::max(1, iMaxWarMinAdjLand + 5 - (iMaxWarMinAdjLand == 0 ? 1 : 0)), 6);
		// NB: MaxWarMinAdjLand also factors into defensibility (WarUtilityAspect::Greed)
	}
	int iHealTurns = 0;
	/*	The city-based distances are deliberately not updated upon the simulated
		conquest of a city -- a conquered city won't immediately start producing
		troops and won't immediately provide full mobility through tile ownership. */
	if (!bClashOnly)
	{
		/*	If distance is so far that no city can be expected to be conquered
			within our maximal time horizon, we'd have to conclude that no conquest
			will ever happen, which is probably incorrect. Better to assume a
			shorter distance in this case. */
		// <!-- custom: Once the graph selects a land invasion, use its cached land duration rather than a faster sea route that would require Fleet and Logistics. See KI#591. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		rDeploymentDistAttacker = std::min(getUWAI().maxSeaDist(),
				bNaval ? pCacheCity->getDistance() : pCacheCity->getDistanceByLand());
		/*	If this makes leaders with a high MaxWarMinAdjLand value assume
			that they can't reach a very distant target -- OK. */
		rDeploymentDistAttacker *= rMaxWarMinAdjLandFactor;
		/*	Often, the attacker can move from one conquered city to the next.
			Too complicated to do the distance measurements. The code above
			assumes that the attacking units always start at a city owned in the
			actual game. Reduce distance for subsequent attacks a bit in order to
			match an average case. */
		if (!m_conquests.empty())
		{
			CvCity const& kLatestConq = m_conquests[m_conquests.size() - 1]->city();
			if (kLatestConq.getOwner() == kDefender.m_ePlayer &&
				pCity->sameArea(kLatestConq))
			{
				rDeploymentDistAttacker *= fixp(0.6);
				// Will have to wait for some units to heal then though
				iHealTurns += 2;
				if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_DEPLOYMENT_DISTANCE_REDUCED turn=%d agentPlayer=%d attacker=%d defender=%d reason=PRIOR_CONQUEST",
						GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer);
			}
		}
	}
	else
	{
		// <!-- custom: Pass the selected mode so overland clashes use land distance while fleet-only clashes retain the mixed route. See KI#591. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		scaled const rClashDist = clashDistance(kDefender, bNaval);
		// UWAICache::City::updateDistance should guarantee this for rival cities
		FAssert(rClashDist.isPositive());
		rDeploymentDistAttacker = rClashDist;
		rDeploymentDistDefender = rClashDist;
		// Can't rule out that both armies are eliminated at this point
		if (rArmyPow + rDefArmyPow > 1)
		{
			rDeploymentDistAttacker *= rArmyPow / (rArmyPow + rDefArmyPow);
			// <!-- custom: Practical 1341 applied the attacker's personality factor before deriving the defender's remaining physical distance, which could push the meeting point beyond the defender and make its distance negative.
			// Split the bounded geometry first, then retain the personality effect only as attacker deployment effort. See KI#582. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			rDeploymentDistDefender -= rDeploymentDistAttacker;
			rDeploymentDistAttacker *= rMaxWarMinAdjLandFactor;
		}
	}
	int iDeployTurns = rDeploymentDistAttacker.round();
	if (bAttackerUnprepared) // Time for gathering attackers
		iDeployTurns += (5 - GET_PLAYER(m_ePlayer).AI_getCurrEraFactor() / 2).floor();
	scaled rReinforcementDist = rDeploymentDistAttacker;
	/*	The reduction in attacking army power based on distance mainly accounts for
		two effects:
		a)	Produced units become available with a delay.
		b)	The tactical AI tends to form smaller stacks when distances are long;
			they arrive in a trickle.
		When sneak attacking, b) is less of a concern, but don't want to encourage
		early long-distance wars; reduce rDeploymentDist only slightly. */
	if (bSneakAttack)
	{
		if (bClashOnly && kDefender.m_ePlayer == m_eAgent)
		{
			rDeploymentDistDefender *= fixp(0.8);
			rDeploymentDistAttacker = 0;
		}
		else if (!bClashOnly)
		{
			rDeploymentDistAttacker *= fixp(0.8);
			rDeploymentDistDefender = 0;
		}
		/*	This often allows sneak attackers to attack a city before civs that
			have been at war for some time. These civs already get a chance to
			conquer cities during the simulation prolog. That said, no build-up
			is assumed during the prolog, so the sneak-attacking AI may over-
			estimate its chances when it comes to grabbing cities. The proper
			solution would be to interleave the simulation and build-up steps. */
		iDeployTurns = 3;
	}
	else if (isContinuedWar(kDefender))
	{
		/*	War that is already being fought in the actual game state. Assume that
			units are already being deployed then. */
		iDeployTurns /= 2;
	}
	if (bCavalryAttack)
	{
		iDeployTurns = (fixp(0.6) * iDeployTurns).uround();
		rDeploymentDistAttacker *= fixp(0.75);
	}
	if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_DEPLOYMENT_DISTANCE turn=%d agentPlayer=%d attacker=%d defender=%d attackerDistance=%d defenderDistance=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer,
				rDeploymentDistAttacker.uround(), rDeploymentDistDefender.uround());
	if (!bClashOnly) // Duration has no bearing on clashes
	{
		if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_DEPLOYMENT_DURATION turn=%d agentPlayer=%d attacker=%d defender=%d turns=%d sneakAttack=%d attackerUnprepared=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer,
				iDeployTurns, bSneakAttack, bAttackerUnprepared);
	}
	// Sea battle about naval landing
	if (bNaval)
	{
		scaled rFleetPow =
				(m_military[FLEET]->power()
				- m_arLostPower[FLEET]) *
				rConfAtt * rArmyPortionAttacker;
		scaled rDefFleetPow =
				(kDefender.m_military[FLEET]->power()
				- kDefender.m_arLostPower[FLEET]) *
				rConfDef * rArmyPortionDefender;
		rDefFleetPow.increaseTo(0);
		// Reduced b/c not all transports are available for military purposes
		scaled rCargoCap = fixp(0.73) *
				(m_military[LOGISTICS]->power() - m_arLostPower[LOGISTICS]);
		rSASFleetPow = rFleetPow;
		rSASDefFleetPow = rDefFleetPow;
		rSASCargoCap = rCargoCap;
		scaled rLogisticsPortion = 0;
		/*	Fixme: This logistics portion is way too small; rCargoCap counts
			cargo space, whereas rFleetPow is a power rating (strength^1.7).
			Means that Logistics losses aren't really counted. Will need to
			track power and cargo separately for the Logistics branch to fix this. */
		if (rFleetPow > 1)
		{
			// <!-- custom: Fleet battle losses are confidence-scaled, but Logistics records physical cargo capacity. Match the defender conversion by cancelling attacker confidence in this ratio. See KI#584. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			rLogisticsPortion = rCargoCap * rConfAtt / rFleetPow;
		}
		// <!-- custom: Repeat trips happen after the naval battle, so only surviving transports can make them. Retain the factor until Logistics losses have been removed. See KI#585. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		scaled const rRepeatTripFactor = 1 + 1 / scaled::max(2, rReinforcementDist);
		scaled rLogisticsPortionDef;
		if (rDefFleetPow > 1)
		{
			rLogisticsPortionDef =
					(kDefender.m_military[LOGISTICS]->power()
					- kDefender.m_arLostPower[LOGISTICS]) *
					rConfDef / rDefFleetPow;
		}
		// +30% for attacker b/c only a clear victory can prevent a naval landing
		scaled rFleetPowMult = fixp(1.3);
		if (bClashOnly)
		{
			rFleetPowMult = 1;
			/*	In a clash, let every civ overestimate its own chances; this
				should make it harder to deter the AI through naval build-up. */
			if (m_ePlayer == m_eAgent)
				rFleetPowMult *= fixp(1.25);
			else if (kDefender.m_ePlayer == m_eAgent)
				rFleetPowMult /= fixp(1.25);
		}
		bool const bAttWin = (rFleetPow > 1 && rFleetPowMult * rFleetPow > rDefFleetPow);
		bSASFleetWin = bAttWin;
		std::pair<scaled,scaled> rrLossesWL = clashLossesWinnerLoser(
				rFleetPow, rDefFleetPow, false, true);
		scaled rLossesAtt, rLossesDef;
		scaled rTypicalArmyUnitPow = m_military[ARMY]->getTypicalPower(
				TEAMID(m_eAgent));
		if (m_military[ARMY]->getTypicalUnit() == NO_UNIT)
		{
			FAssertMsg(iAttCities <= 0, "No typical army unit found");
			rTypicalArmyUnitPow = fixp(3.25); // That's a Warrior
		}
		/*	Tend to underestimate the head count b/c of outdated
			units with power lower than typical. Hence the extra 20%. */
		scaled rArmySize = fixp(1.2) * rArmyPow / rTypicalArmyUnitPow;
		rSASArmySize = rArmySize;
		if (bAttWin)
		{
			rLossesAtt = rrLossesWL.first / rConfAtt;
			kStep.reducePower(m_ePlayer, FLEET, rLossesAtt);
			// Don't try to predict lost army in cargo
			scaled rLogisticsLosses = rrLossesWL.first *
					rLogisticsPortion / rConfAtt;
			kStep.reducePower(m_ePlayer, LOGISTICS, rLogisticsLosses);
			rLossesDef = rrLossesWL.second / rConfDef;
			kStep.reducePower(kDefender.m_ePlayer, FLEET, rLossesDef);
			kStep.reducePower(kDefender.m_ePlayer, LOGISTICS,
					rrLossesWL.second * rLogisticsPortionDef / rConfDef);
			if (bClashOnly)
			{
				// Only clash fleets in a naval war
				kStep.setSuccess(true);
				return &kStep;
			}
			else
			{
				scaled rCargoSize = rCargoCap - rLogisticsLosses;
				rCargoSize.increaseTo(0);
				rCargoSize *= rRepeatTripFactor;
				rSASSurvivingCargo = rCargoSize;
				if (bLogInvasionSummary) logBBAI("UWAI_INVASION_GRAPH_NAVAL_LANDING_SUCCESS turn=%d agentPlayer=%d attacker=%d defender=%d survivingCargo=%d",
						GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer,
						rCargoSize.round());
				if (rArmySize > 0)
				{
					scaled rLandingRatio = rCargoSize / rArmySize;
					rLandingRatio.decreaseTo(1);
					rNavalLandingRatio = rLandingRatio;
					rArmyPow *= rLandingRatio;
					rCavPow *= rLandingRatio;
					if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_NAVAL_LANDING_POWER turn=%d agentPlayer=%d attacker=%d defender=%d power=%d",
							GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer, rArmyPow.uround());
				}
			}
		}
		else
		{
			rLossesAtt = rrLossesWL.second / rConfAtt;
			kStep.reducePower(m_ePlayer, FLEET, rLossesAtt);
			scaled rLogisticsLosses = rLossesAtt * rLogisticsPortion;
			kStep.reducePower(m_ePlayer, LOGISTICS, rLogisticsLosses);
			rLossesDef = rrLossesWL.first / rConfDef;
			kStep.reducePower(kDefender.m_ePlayer, FLEET, rLossesDef);
			kStep.reducePower(kDefender.m_ePlayer, LOGISTICS,
					rLossesDef * rLogisticsPortionDef);
			if (!bClashOnly)
			{
				scaled rDrowned;
				if (rArmySize > 0)
					rDrowned = scaled::min(1, rLogisticsLosses / rArmySize) * rArmyPow;
				kStep.reducePower(m_ePlayer, ARMY, rDrowned);
				if (rArmyPow > 0)
					kStep.reducePower(m_ePlayer, CAVALRY, rDrowned * rCavPow / rArmyPow);
				if (bLogInvasionSummary) logBBAI("UWAI_INVASION_GRAPH_NAVAL_LANDING_REPELLED turn=%d agentPlayer=%d attacker=%d defender=%d",
						GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer);
			}
			kStep.setSuccess(false);
			kStep.setDuration(iDeployTurns + (bSneakAttack ? 0 : 2));
			if (bSASLogNavalInvasionStep && rArmyPortionDefender > 0) logBBAI("WAR_NAVAL_INVASION_STEP turn=%d agentTeam=%d targetTeam=%d total=%d prepTurns=%d stage=LANDING_REPELLED targetCityX=%d targetCityY=%d cacheDistance=%d targetValue=%d armyPortionAttackerPercent=%d armyPortionDefenderPercent=%d attackerArmyPower=%d defenderArmyPower=%d attackerFleetPower=%d defenderFleetPower=%d cargoCapacity=%d armySize=%d duration=%d",
				GC.getGame().getGameTurn(), TEAMID(m_eAgent), kEvalParams.getTarget(), kEvalParams.isTotal(), kEvalParams.getPreparationTime(),
				pCity->getX(), pCity->getY(), pCacheCity->getDistance(), pCacheCity->getTargetValue(), rArmyPortionAttacker.getPercent(), rArmyPortionDefender.getPercent(),
				rArmyPowRaw.uround(), rDefArmyPow.uround(), rSASFleetPow.uround(), rSASDefFleetPow.uround(), rSASCargoCap.uround(), rSASArmySize.uround(), kStep.getDuration());
			return &kStep;
		}
		if (rLossesAtt > 0 || rLossesDef > 0)
		{
			if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_SEA_BATTLE_LOSSES_LANDING turn=%d agentPlayer=%d attacker=%d defender=%d attackerLoss=%d defenderLoss=%d",
					GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer,
					rLossesAtt.uround(), rLossesDef.uround());
		}
	}
	// Subtract 2% per distance unit, but at most 50%
	scaled rDefDeploymentMod = scaled::max(
			100 - 2 * rDeploymentDistDefender, 50) / 100;
	rDefArmyPow *= rDefDeploymentMod;
	// <!-- custom: Cavalry is an overlapping Army subset. Apply the attacker's deployment-availability factor to both branches so distance cannot leave more available Cavalry than total Army. See KI#588. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	scaled const rAttDeploymentMod = scaled::max(100 - fixp(1.55) * rDeploymentDistAttacker.pow(fixp(1.15)), 50) / 100;
	rArmyPow *= rAttDeploymentMod;
	rCavPow *= rAttDeploymentMod;
	// Units available in battle area
	CvArea const* pBattleArea = NULL;
	if (pCity != NULL)
		pBattleArea = pCity->area();
	else if (!bNaval)
	{
		pBattleArea = clashArea(kDefender.m_ePlayer);
		/*	This can happen when a civ has just lost its foothold in an area
			and reachability hasn't been updated yet. */
		//FAssertMsg(pBattleArea != NULL, "No shared area should imply bNaval");
	} // (Else only naval battle; assume that fleets are fully deployed.)
	int const iRemainingCitiesAtt = iAttCities - m_cityLosses.size();
	int const iRemainingCitiesDef = iDefCities - kDefender.m_cityLosses.size();
	scaled rAreaWeightAtt = 1;
	scaled rAreaWeightDef = 1;
	/*	Don't base the weight on pBattleArea->getPower(m_ePlayer); too fleeting
		(and hidden knowledge unless m_ePlayer==m_eAgent). */
	if (pBattleArea != NULL)
	{
		if (iRemainingCitiesAtt > 0)
		{
			CvCity const* pCapital = GET_PLAYER(m_ePlayer).getCapital();
			if (!bNaval || bClashOnly)
			{
				// <!-- custom: Use the hypothetical remaining-city distribution rather than stale real-map CvArea ownership after simulated conquests. See KI#592. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				rAreaWeightAtt *= scaled(remainingCitiesInArea(*pBattleArea),
						iRemainingCitiesAtt);
				if (pCapital != NULL && pCapital->isArea(*pBattleArea))
				{
					rAreaWeightAtt *= (GET_PLAYER(m_ePlayer).isHuman() ?
							fixp(1.5) : fixp(4/3.));
				}
				rAreaWeightAtt.decreaseTo(1);
			}
			else
			{
				if (pCapital != NULL)
				{
					rAreaWeightAtt *= scaled(
							pCapital->getArea().getCitiesPerPlayer(m_ePlayer),
							GET_PLAYER(m_ePlayer).getNumCities());
				}
				/*	A human naval attack focuses much of the army (if supported by
					LOGISTICS), while the AI struggles with deploying units from
					different areas. */
				if (GET_PLAYER(m_ePlayer).isHuman())
					rAreaWeightAtt += fixp(0.25);
				// <!-- custom: KI#53.6 Equal Islands diagnostics found that, after a successful naval landing with enough surviving cargo for essentially the full modeled army, this inherited multi-area proxy was still cutting the AI attacker's available Army to typically about 75% (and sometimes near 55%) before deployment-distance penalties.
				// In 2,056 failed scheduled city attacks from otherwise favorable naval evaluations, the median landing ratio was 100% while the median area weight was 75%; a rounded-log sensitivity check found that the inherited human-style +25 percentage-point allowance, scaled by actual surviving lift, would reverse about 44% of those simulated city defeats.
				// Apply only to the evaluating team and scale the allowance by the simulated landing ratio, so weak lift still preserves most of the inherited caution. (ChatGPT-5.6-Sol) -->
				else if (TEAMID(m_ePlayer) == TEAMID(m_eAgent))
				{
					static const bool bSASNavalLogisticsDeploymentOptimize = GC.getDefineBOOL("SAS_UWAI_INVASION_GRAPH_NAVAL_LOGISTICS_DEPLOYMENT_OPTIMIZE");
					if (bSASNavalLogisticsDeploymentOptimize)
					{
						scaled const rOldAreaWeight = rAreaWeightAtt;
						rAreaWeightAtt += fixp(0.25) * rNavalLandingRatio;
						rAreaWeightAtt.decreaseTo(1);
						rSASNavalAreaBonus = rAreaWeightAtt - rOldAreaWeight;
					}
				}
				rAreaWeightAtt.clamp(fixp(0.5), 1);
			}
			if (rAreaWeightAtt < 1)
			{
				if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_AREA_WEIGHT_ATTACKER turn=%d agentPlayer=%d attacker=%d defender=%d percent=%d",
						GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer, rAreaWeightAtt.getPercent());
				// <!-- custom: Cavalry is an overlapping Army sub-branch. AdvCiv weighted only the attacker's Army by battle area, breaking Cavalry <= Army even though its defender-side sibling was fixed. Scale both together. See KI#583. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				rArmyPow *= rAreaWeightAtt;
				rCavPow *= rAreaWeightAtt;
			}
		}
		if (iRemainingCitiesDef > 0)
		{
			rAreaWeightDef *= scaled(kDefender.remainingCitiesInArea(*pBattleArea),
					iRemainingCitiesDef);
			CvCity const* pCapital = GET_PLAYER(kDefender.m_ePlayer).getCapital();
			if (pCapital != NULL && pCapital->isArea(*pBattleArea))
			{
				rAreaWeightDef *= (GET_PLAYER(kDefender.m_ePlayer).isHuman() ?
						fixp(1.5) : fixp(4/3.));
			}
			/*	Even if the local army is too small to prevent the (temporary)
				capture of a city, reinforcements will arrive before long. */
			rAreaWeightDef.clamp(fixp(1/3.), 1);
			if (rAreaWeightDef != 1)
			{
				if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_AREA_WEIGHT_DEFENDER turn=%d agentPlayer=%d attacker=%d defender=%d percent=%d",
						GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer,
						rAreaWeightDef.getPercent());
				rDefArmyPow *= rAreaWeightDef;
				rDefCavPow *= rAreaWeightDef;
			}
		}
	}
	// Combat bonuses
	/*	Aggressive trait; should probably exclude cavalry in any case or check the
		combat type of the typical army unit; tedious to implement. */
	scaled rArmyModAtt, rArmyModDef;
	if (GET_PLAYER(m_ePlayer).uwai().getCache().hasOffensiveTrait() &&
		!bCavalryAttack)
	{
		rArmyModAtt += fixp(0.1);
	}
	if (GET_PLAYER(kDefender.m_ePlayer).uwai().getCache().hasOffensiveTrait())
		rArmyModDef += fixp(0.1);
	scaled rArmyModAttCorr = powerCorrect(1 + rArmyModAtt);
	scaled rArmyModDefCorr = powerCorrect(1 + rArmyModDef);
	// These bonuses are removed again (through division) when applying losses
	scaled rArmyPowModified = rArmyPow * rArmyModAttCorr * rConfAtt;
	scaled rDefArmyPowModified = rDefArmyPow * rArmyModDefCorr * rConfDef;
	// Two turns for the actual attack
	kStep.setDuration(iHealTurns + iDeployTurns + (bSneakAttack ? 0 : 2));
	//bool bDefenderOutnumbered = (fixp(1.5) * rDefArmyPowModified < rArmyPowModified);
	/*	Don't do a second clash. Can reward the attacker for having fewer units.
		Not unrealistic (a larger army can cause the defending army to dig in),
		but can lead to erratic AI decisions since I'm only considering a
		single simulation trajectory. */
	bool const bDefenderOutnumbered = true;
	if (!bDefenderOutnumbered || bClashOnly)
	{
		bool const bAttWin = (rArmyPowModified > rDefArmyPowModified);
		if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_CLASH_POWER turn=%d agentPlayer=%d attacker=%d defender=%d attackerPower=%d defenderPower=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer,
				rArmyPowModified.uround(), rDefArmyPowModified.uround());
		std::pair<scaled,scaled> rrLossesWL = clashLossesWinnerLoser(
				rArmyPowModified, rDefArmyPowModified, !bClashOnly, false);
		scaled rCavRatio, rDefCavRatio;
		if (rArmyPow > 0)
		{
			rCavRatio = rCavPow /
					(rArmyPow +
					m_military[HOME_GUARD]->power()
					- m_arLostPower[HOME_GUARD]);
			rCavRatio.clamp(0, 1);
		}
		if (rDefArmyPow > 0)
		{
			rDefCavRatio = rDefCavPow /
					(rDefArmyPow +
					kDefender.m_military[HOME_GUARD]->power()
					- kDefender.m_arLostPower[HOME_GUARD]);
			rDefCavRatio.clamp(0, 1);
		}
		scaled rLossesWinner = rrLossesWL.first;
		scaled rLossesLoser = rrLossesWL.second;
		scaled rTempLosses = clashLossesTemporary(
				rArmyPowModified, rDefArmyPowModified);
		if (bAttWin)
		{
			// <!-- custom: Clash results are measured from confidence- and combat-modified power. Convert all persistent and temporary casualties back to physical branch power before applying them. See KI#580. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			rLossesWinner /= rArmyModAttCorr * rConfAtt;
			rLossesLoser /= rArmyModDefCorr * rConfDef;
			rTempLosses /= rArmyModAttCorr * rConfAtt;
			// Have tempLosses take effect immediately (not necessary if bClashOnly)
			scaled rUnavail = rLossesWinner + rTempLosses;
			rArmyPow -= rUnavail;
			rCavPow -= rCavRatio * rUnavail;
			// Remember the tempLosses
			kStep.setTempLosses(rTempLosses);
			kStep.reducePower(m_ePlayer, ARMY, rLossesWinner);
			kStep.reducePower(kDefender.m_ePlayer, ARMY, rLossesLoser);
			kStep.reducePower(m_ePlayer, CAVALRY, rLossesWinner * rCavRatio);
			kStep.reducePower(kDefender.m_ePlayer, CAVALRY, rLossesLoser * rDefCavRatio);
			if (!bClashOnly)
			{
				if (bLogInvasionSummary) logBBAI("UWAI_INVASION_GRAPH_FIELD_BATTLE_DEFENDER_DEFEATED turn=%d agentPlayer=%d attacker=%d defender=%d attackerLoss=%d defenderLoss=%d",
						GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer,
						rLossesWinner.round(), rLossesLoser.round());
			}
			else
			{
				kStep.setSuccess(true);
				return &kStep;
			}
		}
		else
		{
			FAssert(rArmyModAttCorr > 0 && rArmyModDefCorr > 0);
			// <!-- custom: As in the attacker-win branch, remove both confidence and combat modifiers before storing physical Army, Cavalry and temporary losses. See KI#580. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			rLossesLoser /= rArmyModAttCorr * rConfAtt;
			rLossesWinner /= rArmyModDefCorr * rConfDef;
			if (bClashOnly) // No rTempLosses for repelled city attack
				kStep.setTempLosses(rTempLosses / (rArmyModDefCorr * rConfDef));
			kStep.reducePower(m_ePlayer, ARMY, rLossesLoser);
			kStep.reducePower(kDefender.m_ePlayer, ARMY, rLossesWinner);
			kStep .reducePower(m_ePlayer, CAVALRY, rLossesLoser * rCavRatio);
			kStep.reducePower(kDefender.m_ePlayer, CAVALRY, rLossesWinner * rDefCavRatio);
			kStep.setSuccess(false);
			if (!bClashOnly)
			{
				if (bLogInvasionSummary) logBBAI("UWAI_INVASION_GRAPH_FIELD_BATTLE_ATTACK_REPELLED turn=%d agentPlayer=%d attacker=%d defender=%d attackerLoss=%d defenderLoss=%d",
						GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer,
						rLossesLoser.uround(), rLossesWinner.uround());
			}
			return &kStep;
		}
	}
	else rDefArmyPow /= rDefDeploymentMod; // Retreated army has depl. dist. 0
	/*	Attackers normally assumed to have city raider promotions;
		cavalry doesn't, but might ignore first strikes. */
	if (bCavalryAttack)
	{
		rArmyModAtt -= fixp(0.12);
		rArmyModAttCorr = powerCorrect(1 + rArmyModAtt);
	}
	/*	Needs to be updated in any case in order to take into account potential
		losses from clash. */
	rArmyPowModified = rArmyPow * rArmyModAttCorr * rConfAtt;
	FAssert(iRemainingCitiesDef > 0);
	/*	Assume that the defenders stationed in a city are 50% static city defenders
		and 50% floating defenders that can move to reinforce a nearby city that is
		threatened. To this end, assume that each city has two garrisons;
		a garrison may be just one unit or many. One garrison can move,
		the other can't. */
	scaled rPowerPerGarrison = fixp(0.5) *
			(kDefender.m_military[HOME_GUARD]->power() + kDefender.m_rEmergencyDefPow
			- kDefender.m_arLostPower[HOME_GUARD]) /
			iRemainingCitiesDef;
	int iLocalGarrisons = 2; // Could also make this fractional
	/*	Extra garrisons assumed to be stationed in important cities
		(similar to code in CvCityAI::AI_neededDefenders) */
	bool const bCityImportant = (pCity->isCapital() || pCity->isHolyCity() ||
			pCity->hasActiveWorldWonder());
	if (bCityImportant && !bUniformGarrisons)
		iLocalGarrisons += 2;
	/*	If a civ has only 3 cities, then the code above assigns 4 (of the 6)
		garrisons to an important city (e.g. the capital). That adds up
		(1 garrisons remains for each of the other cities), but CvCityAI doesn't
		distribute defenders quite as unevenly. */
	iLocalGarrisons = std::min(iLocalGarrisons, 2 + iRemainingCitiesDef / 2);
	scaled rTypicalGarrisonPow = kDefender.m_military[HOME_GUARD]->
			getTypicalPower(TEAMID(m_eAgent));
	if (kDefender.m_military[HOME_GUARD]->getTypicalUnit() == NO_UNIT)
	{
		FAssertMsg(iDefCities <= 0, "No typical garrison unit found");
		rTypicalGarrisonPow = fixp(3.25); // That's a Warrior
	}
	// Fewer rallies if all spread thin
	int iRallied = ((rPowerPerGarrison *
			// Humans are better at rallying
			(GET_PLAYER(kDefender.m_ePlayer).isHuman() ? fixp(1.3) : 1)) /
			rTypicalGarrisonPow).uround();
	// Upper bound for rallies based on importance of city
	int iRallyBound = 0;
	if (bUniformGarrisons)
		iRallyBound = 1;
	else
	{
		// Population above 75% of the average
		if (pCity->getPopulation() >
			(fixp(0.75) * GET_PLAYER(kDefender.m_ePlayer).getTotalPopulation()) /
			std::max(1, iDefCities))
		{
			iRallyBound = 1;
		}
		if (bCityImportant)
			iRallyBound = 2;
		if (pBattleArea != NULL)
		{
			// <!-- custom: The attacked city's garrison is already local; cap additional rallies by other simulated remaining cities in this area, excluding cities already conquered in the graph. See KI#592. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			iRallyBound = std::min(kDefender.remainingCitiesInArea(*pBattleArea) - 1, iRallyBound);
			iRallyBound = std::min(iRemainingCitiesDef - 1, iRallyBound);
			iRallyBound = std::max(0, iRallyBound);
		}
		else FAssert(false);
	}
	iRallied = std::min(iRallyBound, iRallied);
	if (bCavalryAttack) // Swift attack
		iRallied = 0;
	int iGarrisons = iLocalGarrisons + iRallied;
	/*	Example: Just 2 cities left, i.e. 4 garrisons. 1 has to stay
		in the other city, therefore only 3 in the attacked city. */
	iGarrisons = std::min(iRemainingCitiesDef + 1, iGarrisons);
	// Recently conquered city likely to lack a garrison
	if (!bUniformGarrisons && pCity->isEverOwned(m_ePlayer) &&
		pCity->isOccupation() &&
		GET_TEAM(m_ePlayer).isAtWar(TEAMID(kDefender.m_ePlayer)))
	{
		iGarrisons = 1;
	}
	iLocalGarrisons = std::min(iGarrisons, iLocalGarrisons);
	scaled rGuardPowUnmodified = iGarrisons * rPowerPerGarrison;
	// Only for local garrisons
	scaled rFortificationMod = fixp(0.25);
	MilitaryBranch const& kDefHomeGuard = *kDefender.m_military[HOME_GUARD];
	bool bNoGuardUnit = (kDefHomeGuard.getTypicalUnit() == NO_UNIT);
	FAssert(!bNoGuardUnit || iDefCities <= 0);
	// For all garrisons
	scaled rCityDefenderMod = (bNoGuardUnit ? 0 : per100(GC.getInfo(kDefHomeGuard.getTypicalUnit()).getCityDefenseModifier()));
	// <!-- custom: Every graph node shares m_eAgent, so the inherited check gave all city defenders the evaluating player's trait. Query the actual defender owner. See KI#581. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (GET_PLAYER(kDefender.m_ePlayer).uwai().getCache().hasDefensiveTrait())
		rCityDefenderMod += fixp(0.3);
	bool const bIgnoreCityDef = (m_military[ARMY]->getTypicalUnit() != NO_UNIT &&
			GC.getInfo(m_military[ARMY]->getTypicalUnit()).isIgnoreBuildingDefense());
	// Normal defensive promotions are assumed to be countered by city raider
	// For all non-mounted defenders
	scaled rPlotDef = (m_military[ARMY]->getTypicalUnit() == NO_UNIT ? 0 :
			pCity->getDefenseModifier(bIgnoreCityDef));
	/*	AI tends to run low on siege after a while. era+1 based on assumption that
		AI tends to bring enough siege to bomb. twice per turn on average
		(on the final turn, some siege units will also attack, i.e. can't bomb).
		Too little? 10 * instead of 8? */
	int const iEraFactor = std::max(1, GET_PLAYER(m_ePlayer).getCurrentEra() +
			5 - CvEraInfo::AI_getAgeOfGuns());
	scaled rBombPerTurn(8 * iEraFactor * (6 - (int)m_conquests.size()), 6);
	/*	Don't assume that AI will endlessly bombard.
		For a Medieval Castle backed by Chichen Itza (i.e. 125% def),
		tileBonus / bombPerTurn is about 5.2. W/o bombardment, an attack is pretty
		hopeless. Don't want such a city to completely discourage war, therefore
		set the threshold slightly higher than 5.2; at first, I had set it to 5. */
	if (rBombPerTurn < 8 || rPlotDef / rBombPerTurn > fixp(5.25))
		bCanBombard = false;
	scaled rBombDmg;
	if (bCanBombard)
	{
		rBombDmg = fixp(0.8) * rPlotDef;
		// Assume that city defense still helps a little
		rPlotDef -= rBombDmg;
	}
	// Don't log bCanSoften - it's usually implied
	else if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_BOMBARDMENT_UNAVAILABLE turn=%d agentPlayer=%d attacker=%d defender=%d",
			GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer);
	// Assume 20 damage per turn
	scaled rBombTurns = (bCanBombard ? rBombDmg / rBombPerTurn : 0);
	/*	Walls and Castle slow down bombardment.
		min(75...: A mod could grant 100% bombard defense; treat >75% as 75% */
	int iBombDefPercent = std::min(75, pCity->getBuildingBombardDefense());
	if (bIgnoreCityDef)
		iBombDefPercent = 0;
	FAssert(iBombDefPercent >= 0);
	if (iBombDefPercent >= 85) // No building does that
	{
		rBombTurns = 0;
		bCanBombard = false;
	}
	else rBombTurns *= scaled(100, 100 - iBombDefPercent);
	// Just the hill defense (help=true skips city defense)
	rPlotDef += pCity->getPlot().defenseModifier(NO_TEAM, true, NO_TEAM, true);
	rPlotDef /= 100;
	/*	Would be nice to check also if the city is enclosed by rivers, e.g. in a
		river delta, but I think this can't really happen on randomly generated maps,
		and the test could be a bit slow. */
	scaled rLocalGarrisonPow = (iLocalGarrisons * rPowerPerGarrison
			* powerCorrect(
			1 + rFortificationMod + rCityDefenderMod + rPlotDef)) * rConfDef;
	scaled rRalliedGarrisonPow = ((iGarrisons - iLocalGarrisons)
			* rPowerPerGarrison
			* powerCorrect(1 + rCityDefenderMod + rPlotDef)) * rConfDef;
	scaled rDefendingArmyPow;
	scaled rDefArmyPortion = scaled::min(1, (bSneakAttack ? fixp(0.375) : fixp(0.5)) /
			// More units rallied at the first few cities that are attacked
			scaled(kDefender.m_cityLosses.size() + 1).sqrt() + iRallied * fixp(0.25));
	if (iRemainingCitiesDef == 1)
		rDefArmyPortion = 1;
	else if (!kDefender.m_bHasClashed)
	{
		/*	If their army hasn't been engaged yet, expect the bulk of it to be
			rallied to any attacked city */
		rDefArmyPortion = std::max(rDefArmyPortion, fixp(0.75));
		if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_DEFENDER_PORTION_RAISED turn=%d agentPlayer=%d attacker=%d defender=%d reason=NO_PRIOR_CLASH minimumPercent=75",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer);
	}
	if (bDefenderOutnumbered)
	{
		rDefendingArmyPow = (rDefCavPow + (rDefArmyPow - rDefCavPow) *
				powerCorrect(1 + rPlotDef + rArmyModDef)) * rConfDef *
				rDefArmyPortion;
	}
	scaled rGarrisonPow = rLocalGarrisonPow + rRalliedGarrisonPow;
	/*	The way that attacking and defending units get paired by the combat system
		generally gives the defender an advantage. This adjustment rather underestimates
		that factor I think. It wasn't in place at all until AdvCiv 1.0; don't want to
		go overboard with it now. advc.159 adds similar code to
		CvPlayerAI::AI_localDefenceStrength.*/
	scaled rPowFromDefAdvantage = rGarrisonPow / 8;
	// A stack of just garrisons is going to lack in (rock-paper-scissors) diversity
	rPowFromDefAdvantage.decreaseTo(rDefendingArmyPow / 3);
	if (bCanSoften)
		rPowFromDefAdvantage /= fixp(1.55);
	rGarrisonPow += rPowFromDefAdvantage;
	scaled rDefenderPow = rGarrisonPow + rDefendingArmyPow;
	scaled rPowRatio = rArmyPowModified / scaled::max(1, rDefenderPow);
	scaled rThreat = rPowRatio;
	// <!-- custom: Defender composition and attacker/defender ratio are one city-combat power state; emit one row rather than repeating the same participants and defender total. (ChatGPT-5.6-Sol) -->
	if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_CITY_COMBAT_POWER turn=%d agentPlayer=%d attacker=%d defender=%d besiegerPower=%d defenderPower=%d localGarrisonPower=%d ralliedGarrisonPower=%d mobileArmyPower=%d mobileArmyPercent=%d defenderAdvantagePower=%d ratioPercent=%d",
			GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer,
			rArmyPowModified.uround(), rDefenderPow.uround(), rLocalGarrisonPow.uround(),
			rRalliedGarrisonPow.uround(), rDefendingArmyPow.uround(), rDefArmyPortion.getPercent(),
			rPowFromDefAdvantage.uround(), rPowRatio.getPercent());
	/*	Attacks on important cities may result in greater distraction for
		the defender -- or perhaps not; attacks on remote cities could
		be equally distracting ... */
	//if (bCityImportant) rThreat *= fixp(1.5);
	kStep.setThreat(rThreat);
	/*	Add extra turns for coordinating city raiders and siege. Even with ships
		or aircraft, it takes (the AI) some effort, and in the lategame
		iDeployTurns should be small; hopefully, no special treatment needed. */
	if (bCanBombard || bCanSoften)
		rBombTurns += iDeployTurns * fixp(0.2);
	int const iBombTurns = (rBombTurns / std::max(fixp(0.75), rPowRatio)).round();
	if (bLogInvasionDetail && iBombTurns > 0) logBBAI("UWAI_INVASION_GRAPH_BOMBARDMENT_DURATION turn=%d agentPlayer=%d attacker=%d defender=%d turns=%d",
			GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kDefender.m_ePlayer, iBombTurns);
	// Faster conquest when defenders outnumbered
	kStep.setDuration(iBombTurns + kStep.getDuration());
	FAssert(rArmyModAttCorr > 0 && rConfAtt > 0);
	if (rArmyPowModified > rDefenderPow)
	{
		/*	Full losses for defender would be realistic, but doesn't capture the
			uncertainty of success if threat is near 100%. Therefore: */
		scaled const rDefLossRatio = scaled::min(1, fixp(0.65) * rThreat.pow(fixp(1.5)));
		kStep.reducePower(kDefender.m_ePlayer, ARMY, rDefLossRatio * rDefArmyPow * rDefArmyPortion);
		kStep.reducePower(kDefender.m_ePlayer, CAVALRY, rDefLossRatio * rDefCavPow * rDefArmyPortion);
		kStep.reducePower(kDefender.m_ePlayer, HOME_GUARD,
				rDefLossRatio * rGuardPowUnmodified);
		/*	These losses are a bit exaggerated I think, at least when threat is
			near 1. Deliberate, in order to account for uncertainty. */
		scaled const rAttLossRatio = fixp(0.55) / rThreat.pow(fixp(0.75));
		scaled rLossesAttArmy = (rAttLossRatio * rArmyPowModified) /
				(rArmyModAttCorr * rConfAtt);
		/*	Assume less impact of softening when already some cities conquered
			b/c AI tends to run out of/ low on siege units after a while.
			Reduce losses to 12/18 (66%) for the first city, then 13/18 etc. */
		if (bCanSoften)
			rLossesAttArmy *= scaled::min(1, scaled(12 + (int)m_conquests.size(), 18));
		kStep.reducePower(m_ePlayer, ARMY, rLossesAttArmy);
		if (rArmyPow > 0)
			kStep.reducePower(m_ePlayer, CAVALRY, rLossesAttArmy * rCavPow / rArmyPow);
		kStep.setSuccess(true);
	}
	else
	{
		/*	Few losses for city defenders (can heal and reinforce quickly).
			A multiplier of 0.4 might be realistic (0.3 for army); use higher values
			to account (at least a bit) for the possibility of losing the city. */
		kStep.reducePower(kDefender.m_ePlayer, HOME_GUARD, rGuardPowUnmodified *
				fixp(0.75) * rThreat);
		// Few losses for defending army
		if (bDefenderOutnumbered && rArmyPowModified > rGarrisonPow)
		{
			scaled rLossesDefArmy = rDefArmyPow * rDefArmyPortion * fixp(0.5) * rThreat;
			kStep.reducePower(kDefender.m_ePlayer, ARMY, rLossesDefArmy);
			if (rDefArmyPow > 0)
			{
				kStep.reducePower(kDefender.m_ePlayer, CAVALRY,
						rLossesDefArmy * rDefCavPow / rDefArmyPow);
			}
		}
		/*	Attacker may lose nothing b/c city is so strongly defended that no
			attack is attempted; could also have heavy losses. I don't think this
			can be foreseen; go with an optimistic outcome (expected could be
			perhaps 35% losses). Need to avoid cases where a greater army brings
			the attacker greater losses (by winning the initial clash, but not
			taking a city) as this can lead to counterintuitive AI decisions.
			Also, whether an attack happens in the simulation depends on the
			time horizon; timing quirks mustn't have a major impact on the outcome. */
		scaled rLossesAttArmy = (fixp(0.2) * std::min(rArmyPowModified, rDefenderPow)) /
				(rArmyModAttCorr * rConfAtt);
		kStep.reducePower(m_ePlayer, ARMY, rLossesAttArmy);
		if (rArmyPow > 0)
			kStep.reducePower(m_ePlayer, CAVALRY, rLossesAttArmy * rCavPow / rArmyPow);
		kStep.setSuccess(false);
	}
	if (bSASLogNavalInvasionStep && rArmyPortionDefender > 0) logBBAI("WAR_NAVAL_INVASION_STEP turn=%d agentTeam=%d targetTeam=%d total=%d prepTurns=%d stage=CITY_ATTACK targetCityX=%d targetCityY=%d cacheDistance=%d targetValue=%d fleetWin=%d armyPortionAttackerPercent=%d armyPortionDefenderPercent=%d attackerArmyRawPower=%d defenderArmyPower=%d attackerFleetPower=%d defenderFleetPower=%d cargoCapacity=%d survivingCargo=%d armySize=%d landingRatioPercent=%d navalAreaBonusPercent=%d deploymentDistance=%d attackerDeploymentPercent=%d attackerAreaWeightPercent=%d defenderAreaWeightPercent=%d localGarrisonPower=%d ralliedGarrisonPower=%d mobileDefenderPower=%d mobileDefenderPortionPercent=%d canBombard=%d canSoften=%d bombardTurns=%d besiegerPower=%d cityDefenderPower=%d powerRatioPercent=%d duration=%d success=%d",
		GC.getGame().getGameTurn(), TEAMID(m_eAgent), kEvalParams.getTarget(), kEvalParams.isTotal(), kEvalParams.getPreparationTime(),
		pCity->getX(), pCity->getY(), pCacheCity->getDistance(), pCacheCity->getTargetValue(), bSASFleetWin,
		rArmyPortionAttacker.getPercent(), rArmyPortionDefender.getPercent(), rArmyPowRaw.uround(), rDefArmyPow.uround(),
		rSASFleetPow.uround(), rSASDefFleetPow.uround(), rSASCargoCap.uround(), rSASSurvivingCargo.uround(), rSASArmySize.uround(),
		rNavalLandingRatio.getPercent(), rSASNavalAreaBonus.getPercent(), rDeploymentDistAttacker.uround(),
		rAttDeploymentMod.getPercent(), rAreaWeightAtt.getPercent(), rAreaWeightDef.getPercent(), rLocalGarrisonPow.uround(),
		rRalliedGarrisonPow.uround(), rDefendingArmyPow.uround(), rDefArmyPortion.getPercent(), bCanBombard, bCanSoften, iBombTurns,
		rArmyPowModified.uround(), rDefenderPow.uround(), rPowRatio.getPercent(), kStep.getDuration(), kStep.isAttackerSuccessful());
	return &kStep;
}


bool InvasionGraph::Node::canReachByLand(PlotNumTypes eCityPlot, bool bFromCapital) const
{
	// Allow m_eAgent to magically know whether other civs can reach a city by land
	UWAICache::City const* pCacheCity = m_kCache.lookupCity(eCityPlot);
	if (pCacheCity == NULL) // Then even they don't know where it is
		return false;
	// <!-- custom: Match the land-reachability boolean and max-land-distance gate to the land-only duration, not the fastest mixed route. See KI#591. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	return (bFromCapital ?
			pCacheCity->canReachByLandFromCapital() : pCacheCity->canReachByLand()) &&
			(pCacheCity->getDistanceByLand() <= getUWAI().maxLandDist() ||
			!m_kCache.canTrainDeepSeaCargo());
}


CvArea const* InvasionGraph::Node::clashArea(PlayerTypes eEnemy) const
{
	CvPlayer const& kPlayer = GET_PLAYER(m_ePlayer);
	CvPlayer const& kEnemy = GET_PLAYER(eEnemy);
	/*	For better performance, treat the very common case of a common
		capital area upfront. */
	if (kPlayer.hasCapital() && kEnemy.hasCapital())
	{
		CvArea const& kCapitalArea = kPlayer.getCapital()->getArea();
		if (kEnemy.getCapital()->isArea(kCapitalArea))
			return &kCapitalArea;
	}
	CvArea const* pClashArea = NULL;
	int iClashAreaCities = 0;
	// Going through cities should be faster than going through all areas
	CvPlayer const& kFewerCitiesPlayer = (kPlayer.getNumCities() < kEnemy.getNumCities() ?
			kPlayer : kEnemy);
	FOR_EACH_CITY(pCity, kFewerCitiesPlayer)
	{
		CvArea const& kArea = pCity->getArea();
		int iMinAreaCities = std::min(
				kArea.getCitiesPerPlayer(kPlayer.getID()),
				kArea.getCitiesPerPlayer(kEnemy.getID()));
		if (iMinAreaCities <= 0)
			continue;
		if (pCity->isCapital()) // Count it double
		{
			if (pCity->getOwner() == kPlayer.getID() ||
				pCity->getOwner() == kEnemy.getID())
			{
				iMinAreaCities++;
			}
		}
		if (iMinAreaCities > iClashAreaCities)
		{
			iClashAreaCities = iMinAreaCities;
			pClashArea = &kArea;
		}
	}
	return pClashArea;
}

// <!-- custom: City combat includes simulated emergency defenders beyond the original Home Guard branch. Apply their casualties to availability without misreporting them as losses of the original military. See KI#589. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void InvasionGraph::Node::applyPowerLoss(MilitaryBranchTypes eBranch, scaled rLostPower)
{
	if (eBranch != HOME_GUARD)
	{
		m_arLostPower[eBranch] += rLostPower;
		m_arLostPower[eBranch].decreaseTo(m_military[eBranch]->power());
		return;
	}
	scaled const rRemainingHomeGuard = scaled::max(0,
			m_military[HOME_GUARD]->power() - m_arLostPower[HOME_GUARD]);
	scaled const rHomeGuardLoss = scaled::min(rLostPower, rRemainingHomeGuard);
	m_arLostPower[HOME_GUARD] += rHomeGuardLoss;
	scaled rEmergencyLoss = rLostPower - rHomeGuardLoss;
	rEmergencyLoss.decreaseTo(m_rEmergencyDefPow);
	m_rEmergencyDefPow -= rEmergencyLoss;
}


// <!-- custom: Removed the inherited local stage-1-first helpers: victory-stage bits are cumulative, so every active higher stage returned 1. The capitulation gate below now uses the shared highest-stage helpers. See KI#573. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void InvasionGraph::Node::applyStep(SimulationStep const& kStep)
{
	Node& kAttacker = *m_kOuter.m_nodeMap[kStep.getAttacker()];
	bool bReportedLosses = false;
	bool const bLogInvasionSummary = (gUWAIInvasionGraphLogLevel >= 1 && !m_kLogMuteState.isMuted());
	bool const bLogInvasionDetail = (gUWAIInvasionGraphLogLevel >= 3 && !m_kLogMuteState.isMuted());
	for (int i = 0; i < NUM_BRANCHES; i++)
	{
		MilitaryBranchTypes const eBranch = (MilitaryBranchTypes)i;
		scaled rLostPowerAtt = kStep.getLostPower(kAttacker.m_ePlayer, eBranch);
		scaled rLostPowerDef = kStep.getLostPower(m_ePlayer, eBranch);
		if (rLostPowerAtt >= fixp(0.5) || rLostPowerDef >= fixp(0.5))
		{
			if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_APPLY_BRANCH_LOSS turn=%d agentPlayer=%d attacker=%d defender=%d branch=%s attackerLoss=%d defenderLoss=%d",
					GC.getGame().getGameTurn(), m_eAgent, kAttacker.m_ePlayer, m_ePlayer,
					m_military[eBranch]->str(), rLostPowerAtt.uround(), rLostPowerDef.uround());
			bReportedLosses = true;
		}
		applyPowerLoss(eBranch, rLostPowerDef);
		kAttacker.applyPowerLoss(eBranch, rLostPowerAtt);
	}
	// Remembered until end of phase
	scaled rTempLosses = kStep.getTempLosses();
	if (kStep.isAttackerSuccessful())
		kAttacker.m_rTempArmyLosses += rTempLosses;
	else if (kStep.isClashOnly())
		m_rTempArmyLosses += rTempLosses;
	if (rTempLosses >= fixp(0.5))
	{
		if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_APPLY_TEMP_LOSS turn=%d agentPlayer=%d attacker=%d defender=%d power=%d",
				GC.getGame().getGameTurn(), m_eAgent, kAttacker.m_ePlayer, m_ePlayer, rTempLosses.uround());
		bReportedLosses = true;
	}
	if (bLogInvasionDetail && !bReportedLosses) logBBAI("UWAI_INVASION_GRAPH_APPLY_NO_POWER_LOSS turn=%d agentPlayer=%d attacker=%d defender=%d",
			GC.getGame().getGameTurn(), m_eAgent, kAttacker.m_ePlayer, m_ePlayer);
	if (kStep.isAttackerSuccessful())
	{
		if (kStep.isClashOnly())
			changePrimaryTarget(NULL);
		else
		{
			UWAICache::City const& kStepCity = *kStep.getCity();
			addCityLoss(kStepCity);
			// <!-- custom: Node::step is also used for discarded priority, cycle and time-horizon probes, so halving emergency defense there mutated persistent state without an applied attack.
			// Apply it only after an executed successful city conquest. See KI#576. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			m_rEmergencyDefPow /= 2;
			int const iCurrentCities = GET_PLAYER(m_ePlayer).getNumCities();
			if (m_cityLosses.size() == iCurrentCities)
				setEliminated(true);
			/*	Assume that own offensive stops upon losing a city to a third civ,
				and whole army becomes available for defense.
				m_rDistractionByDefense stays in effect for the whole
				simulation stage. The distraction staying constant
				already implies that losses on the offensive front are recuperated
				with units from the defensive front over time. */
			m_rDistractionByConquest = 0;
			// <!-- custom: The inherited comment says a third-party city loss stops this defender's own offensive, but only its defensive distraction was cleared.
			// Remove the still-live outgoing edge unless it targets the attacker that took the city. See KI#590. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			if (m_pPrimaryTarget != NULL && m_pPrimaryTarget != &kAttacker)
				changePrimaryTarget(NULL);
			/*	This value is currently not read. See comment in resolveLosses.
				The code here only counts time spent on successful city attacks;
				might also want to count failed attacks or clashes. */
			kAttacker.m_iWarTurnsSimulated += kStep.getDuration();
			kAttacker.addConquest(kStepCity);
			// <!-- custom: The conqueror leaves its own units behind, but inherited UWAI converted their count using the victim's era and representative Army power.
			// Use the attacker for both inputs and for the shifted state. See KI#575. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			scaled rUnitsLeftBehind = 2 + GET_PLAYER(kAttacker.m_ePlayer).AI_getCurrEraFactor();
			if (GET_PLAYER(kAttacker.m_ePlayer).isHuman())
				rUnitsLeftBehind = (2 * rUnitsLeftBehind) / 3;
			scaled rPowLeftBehind = rUnitsLeftBehind * kAttacker.m_military[ARMY]->getTypicalPower(TEAMID(m_eAgent));
			rPowLeftBehind.decreaseTo(kAttacker.m_military[ARMY]->power()
					- kAttacker.m_arLostPower[ARMY]);
			if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_GARRISON_LEFT_BEHIND turn=%d agentPlayer=%d attacker=%d defender=%d power=%d",
					GC.getGame().getGameTurn(), m_eAgent, kAttacker.m_ePlayer, m_ePlayer, rPowLeftBehind.uround());
			kAttacker.m_arLostPower[ARMY] += rPowLeftBehind;
			/*	Don't want to add the power to guard b/c the newly conquered city
				can't be attacked by third parties (nor reconquered) and doesn't
				count when computing garrison strength. However, mustn't treat
				units left behind as losses; record them in shiftedPower. */
			kAttacker.m_arShiftedPower[ARMY] += rPowLeftBehind;
			CvTeamAI const& kAttTeam = GET_TEAM(kAttacker.m_ePlayer);
			CvTeamAI const& kDefTeam = GET_TEAM(m_ePlayer);
			AIVictoryStage const eAttackerVictoryStages = GET_PLAYER(kAttacker.m_ePlayer).AI_getVictoryStageHash();
			if (!isEliminated() &&
				GET_PLAYER(m_ePlayer).canPossiblyTradeItem(
				kAttacker.m_ePlayer, TRADE_SURRENDER) &&
				// Don't expect node to capitulate so long as it has vassals
				kDefTeam.getVassalCount(TEAMID(m_eAgent)) <= 0 &&
				getSASConquestVictoryStageLevel(eAttackerVictoryStages) <=
				getSASDominationVictoryStageLevel(eAttackerVictoryStages))
			{
				int iConqueredByAtt = 0;
				for (size_t i = 0; i < kAttacker.m_conquests.size(); i++)
				{
					if (m_cityLosses.count(kAttacker.m_conquests[i]->getID()) > 0)
						iConqueredByAtt++;
				}
				int iConqueredByOther = (int)m_cityLosses.size() - iConqueredByAtt;
				FAssert(iConqueredByOther >= 0);
				/*	If at war, attacker may already have war successes outside
					the simulation that could scare the defender into capitulation. */
				bool const bWar = kAttTeam.isAtWar(kDefTeam.getID());
				if (!kDefTeam.isHuman() &&
					// Last team member standing
					kDefTeam.getAliveCount() <= 1 &&
					GET_PLAYER(m_ePlayer).getNumNukeUnits() <= 0 && // advc.143b
					(m_ePlayer != m_eAgent ||
					/*	When deciding whether to capitulate to a given team, we
						want to know what will happen w/o a capitulation. */
					kAttTeam.getID() !=
					m_kOuter.m_kMA.evaluationParams().getCapitulationTeam()) &&
					iConqueredByAtt >= std::max(iConqueredByOther + 1, bWar ? 1 : 2))
				{
					scaled rPowModPercent = GC.getInfo(GET_PLAYER(m_ePlayer).
							getPersonalityType()).getVassalPowerModifier();
					{
						int const iPowModSign = (rPowModPercent.isNegative() ? -1 : 1);
						rPowModPercent = iPowModSign *
								(iPowModSign * rPowModPercent).pow(fixp(0.75));
					}
					/*	Need to conquer about half of the cities; some fewer
						if there are a lot of them. */
					scaled rCapitulationThresh = scaled(
							iCurrentCities - iConqueredByOther).pow(fixp(0.95)) *
							scaled::max(36,
							55 + (bWar ? -10 : 0) + rPowModPercent) / 100;
					if (iConqueredByAtt >= rCapitulationThresh.uround())
					{
						setCapitulated(TEAMID(kAttacker.m_ePlayer));
						if (bLogInvasionSummary) logBBAI("UWAI_INVASION_GRAPH_CAPITULATION turn=%d agentPlayer=%d vassalPlayer=%d masterPlayer=%d",
								GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kAttacker.m_ePlayer);
					}
				}
			}
			/*	Assume that some defenders are trained upon the loss of a city.
				Defenders trained while the war is conducted are generally
				covered by ArmamentForecast, including defenders hurried just prior
				to an attack, but not emergency defenders trained while the
				invading army heals and approaches its next target. */
			if (!isEliminated())
			{
				m_rEmergencyDefPow +=
						m_military[HOME_GUARD]->getTypicalPower(TEAMID(m_eAgent)) *
						scaled(kStep.getDuration(),
						GET_PLAYER(m_ePlayer).getMaxConscript() > 0 ? 2 : 3);
				if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_EMERGENCY_DEFENDER_POWER turn=%d agentPlayer=%d player=%d power=%d",
						GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, m_rEmergencyDefPow.round());
			}
		}
	}
	// Remove link from graph
	else kAttacker.changePrimaryTarget(NULL);
	// Remove link if army eliminated
	if (m_military[ARMY]->power() <= m_arLostPower[ARMY] && m_pPrimaryTarget != NULL)
	{
		if (bLogInvasionSummary) logBBAI("UWAI_INVASION_GRAPH_ARMY_ELIMINATED_DEFENDER turn=%d agentPlayer=%d player=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer);
		changePrimaryTarget(NULL);
	}
	if (kAttacker.m_military[ARMY]->power() <= kAttacker.m_arLostPower[ARMY] &&
		kAttacker.m_pPrimaryTarget != NULL)
	{
		if (bLogInvasionSummary) logBBAI("UWAI_INVASION_GRAPH_ARMY_ELIMINATED_ATTACKER turn=%d agentPlayer=%d player=%d",
				GC.getGame().getGameTurn(), m_eAgent, kAttacker.m_ePlayer);
		kAttacker.changePrimaryTarget(NULL);
	}
	kAttacker.m_bHasClashed = true;
	if (kStep.isClashOnly() || kStep.isAttackerSuccessful())
		m_bHasClashed = true;
	/*	Don't set hasClashed if the attack fails b/c then the attack may have been
		very minor; could overstate the distractive effect of some small war party. */
}

// <!-- custom: City elimination or capitulation previously left this node in its target's reverse-adjacency set, allowing the defeated player to attack later in the same recursive simulation phase. Enforce the terminal-node edge invariant at the state transition. See KI#574. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void InvasionGraph::Node::setEliminated(bool b)
{
	m_bEliminated = b;
	if (b && m_pPrimaryTarget != NULL)
		changePrimaryTarget(NULL);
}

void InvasionGraph::Node::setCapitulated(TeamTypes eMaster)
{
	m_bCapitulated = true;
	/*	Treat capitulated as eliminated, i.e. assume no further losses.
		According to the actual rules, the vassal immediately joins the
		wars of its master, but that gets too complicated. */
	setEliminated(true);
	/*	Set capitulation-accepted for master team members,
		set other members of m_ePlayer as capitulated (recursive call). */
	for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
	{
		PlayerTypes const eLoopPlayer = it->getID();
		if (m_kOuter.m_nodeMap[eLoopPlayer] == NULL || eLoopPlayer == m_ePlayer ||
			m_kOuter.m_nodeMap[eLoopPlayer]->isEliminated())
		{
			continue;
		}
		TeamTypes const eLoopTeam = TEAMID(eLoopPlayer);
		if (eLoopTeam == eMaster)
		{
			m_kOuter.m_nodeMap[eLoopPlayer]->m_capitulationsAccepted.insert(
					TEAMID(m_ePlayer));
		}
		else if (eLoopTeam == TEAMID(m_ePlayer))
		{
			bool const bHasCapitulated = m_kOuter.m_nodeMap[eLoopPlayer]->hasCapitulated();
			if (!bHasCapitulated)
			{	// Make sure not to produce a stack overflow
				FAssert(!bHasCapitulated);
				continue;
			}
			m_kOuter.m_nodeMap[eLoopPlayer]->setCapitulated(eMaster);
		}
	}
}


UWAICache::City const* InvasionGraph::Node::targetCity(PlayerTypes eTargetOwner) const
{
	if (eTargetOwner == NO_PLAYER)
	{
		if (m_pPrimaryTarget == NULL)
			return NULL;
		eTargetOwner = m_pPrimaryTarget->m_ePlayer;
	}
	for (int i = m_iCacheIndex; i < m_kCache.numCities(); i++)
	{
		UWAICache::City& kCacheCity = m_kCache.cityAt(i);
		if (!kCacheCity.canReach() || kCacheCity.isOwnTeamCity())
			break; // Because of sorting, the rest is also going to be invalid.
		PlayerTypes const eLoopOwner = kCacheCity.city().getOwner();
		if (eLoopOwner == eTargetOwner)
		{
			Node const* pOwner = m_kOuter.m_nodeMap[eTargetOwner];
			/*	(Target may also have conquered the city; however,
				cities being won and lost within one military analysis
				gets too complicated.) */
			if ((pOwner == NULL || !pOwner->hasLost(kCacheCity.getID())) &&
				m_kAgentCache.lookupCity(kCacheCity.getID()) != NULL)
			{
				return &kCacheCity;
			}
		}
	}
	return NULL;
}


void InvasionGraph::Node::resolveLosses()
{
	bool const bLogInvasionDetail = (gUWAIInvasionGraphLogLevel >= 3 && !m_kLogMuteState.isMuted());
	if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_RESOLVE_LOSSES_BEGIN turn=%d agentPlayer=%d player=%d targetedByCount=%d",
			GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, (int)m_targetedBy.size());
	if (m_targetedBy.empty())
	{
		if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_RESOLVE_LOSSES_NONE turn=%d agentPlayer=%d player=%d reason=NO_ATTACKERS",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer);
		return;
	}
	vector<SimulationStep*> apSteps(MAX_PLAYERS, NULL);
	vector<int> aiTurnsSimulated(MAX_PLAYERS, 0);
	vector<bool> abTargetingThis(MAX_PLAYERS, false);
	// <!-- custom: An actual step outside this defender's horizon must not be selected repeatedly or abort unrelated concurrent invaders that can still act in time. See KI#577. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	vector<bool> abHorizonExcluded(MAX_PLAYERS, false);
	/*	Reduces m_rDistractionByDefense to 70%. Otherwise it would be implied that
		losses incurred from an attack by a third power lead to potential attackers
		closing the gap at that front.
		While the UnitAI might do that, accounting for this on the defensive front
		gets too complicated. Counting the distraction only partially seems
		like a reasonable approximation. */
	scaled const rInvaderDistractionMult = fixp(0.7);
	int iTimeLimit = m_kOuter.m_iTimeLimit;
	// Threat that carries over from one iteration to the next
	scaled rPastThreat;
	do {
		for (PlyrSetIter it = m_targetedBy.begin(); it != m_targetedBy.end(); ++it)
		{
			if (abHorizonExcluded[*it])
				continue;
			InvasionGraph::Node& kInvader = *m_kOuter.m_nodeMap[*it];
			if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_PRIORITY_ASSESS turn=%d agentPlayer=%d defender=%d attacker=%d",
					GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, kInvader.m_ePlayer);
			m_kLogMuteState.pushMute();
			apSteps[*it] = m_kOuter.m_nodeMap[*it]->step(0,
					1 - (rInvaderDistractionMult * kInvader.m_rDistractionByDefense));
			m_kLogMuteState.popMute();
			if (bLogInvasionDetail && apSteps[*it] != NULL) logBBAI("UWAI_INVASION_GRAPH_PRIORITY_REACHABLE turn=%d agentPlayer=%d defender=%d attacker=%d duration=%d",
					GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, *it, apSteps[*it]->getDuration());
			else if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_PRIORITY_UNREACHABLE turn=%d agentPlayer=%d defender=%d attacker=%d",
					GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, *it);
			abTargetingThis[*it] = true;
		}
		int iShortestDuration = MAX_INT;
		PlayerTypes eNextInvader = NO_PLAYER;
		scaled rNextInvaderThreat = -1;
		scaled rSumOfThreats;
		for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
		{
			PlayerTypes const eLoopPlayer = it->getID();
			if (abHorizonExcluded[eLoopPlayer])
				continue;
			if (apSteps[eLoopPlayer] == NULL)
			{
				/*	Could remove the NULL entries during the iteration above
					(more efficient), but modifying a container during
					iteration is a can of worms.
					NULL means the invader can't pick a target city (none left
					or none revealed on the map). NB: applyStep may also erase
					Nodes from targetedBy. */
				if (abTargetingThis[eLoopPlayer])
				{
					m_kOuter.m_nodeMap[eLoopPlayer]->changePrimaryTarget(NULL);
					abTargetingThis[eLoopPlayer] = false;
				}
				continue;
			}
			else
			{
				rSumOfThreats += apSteps[eLoopPlayer]->getThreat();
				int iDuration = apSteps[eLoopPlayer]->getDuration() +
						aiTurnsSimulated[eLoopPlayer];
				if (iDuration < iShortestDuration)
				{
					iShortestDuration = iDuration;
					eNextInvader = eLoopPlayer;
					rNextInvaderThreat = apSteps[eLoopPlayer]->getThreat();
				}
			}
		}
		/*	Steps so far are preliminary, ignoring the defending army.
			Need those steps only to determine how the defenders split up
			against several invaders, and which invader's step is
			resolved next. */
		for (PlayerIter<> it; it.hasNext(); ++it)
			SAFE_DELETE(apSteps[it->getID()]);
		// <!-- custom: Zero-defense probes determine priority and the defender split only.
		// A naval probe can conquer and continue into a siege while the actual defended landing is repelled sooner, so its duration is not a valid lower bound for horizon pruning. See KI#577. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if (eNextInvader == NO_PLAYER || rSumOfThreats <= 0)
			break;
		InvasionGraph::Node& kInvader = *m_kOuter.m_nodeMap[eNextInvader];
		SimulationStep& kNextStep = *kInvader.step(rNextInvaderThreat /
				(rSumOfThreats + rPastThreat +
				// Multiplicative distractionByConquest instead of additive?
				m_rDistractionByConquest),
				1 - (rInvaderDistractionMult * kInvader.m_rDistractionByDefense));
		int iActualDuration = kNextStep.getDuration();
		bool const bStepFitsHorizon = (iTimeLimit < 0 || aiTurnsSimulated[eNextInvader] + iActualDuration <= iTimeLimit);
		// <!-- custom: A plausible naval city attack can be predicted successfully yet still contribute no conquest if its duration falls outside MilitaryAnalyst's horizon.
		// Record the scheduler decision separately from WAR_NAVAL_INVASION_STEP so KI#53.6 can distinguish combat pessimism from timing exclusion. (ChatGPT-5.6-Sol) -->
		UWAICache::City const* const pSASStepCity = kNextStep.getCity();
		WarEvalParameters const& kSASEvalParams = m_kOuter.m_kMA.evaluationParams();
		CvTeamAI const& kSASAgentTeam = GET_TEAM(TEAMID(m_eAgent));
		CvTeamAI const& kSASTargetTeam = GET_TEAM(kSASEvalParams.getTarget());
		if (gWarLogLevel >= 3 && !m_kOuter.m_bPeaceScenario && eNextInvader == m_eAgent && pSASStepCity != NULL &&
			TEAMID(m_ePlayer) == kSASEvalParams.getTarget() && !kSASAgentTeam.isAtWar(kSASTargetTeam.getID()) &&
			kSASTargetTeam.getDefensivePower(kSASAgentTeam.getID()) <= kSASAgentTeam.getPower(true) &&
			!kInvader.canReachByLand(pSASStepCity->getID(), false) && kInvader.m_military[LOGISTICS]->power() > 0)
		{
			logBBAI("WAR_NAVAL_INVASION_SCHEDULE turn=%d agentTeam=%d targetTeam=%d total=%d prepTurns=%d targetCityX=%d targetCityY=%d cacheDistance=%d duration=%d turnsAlready=%d timeLimit=%d fitsHorizon=%d success=%d threatPercent=%d",
				GC.getGame().getGameTurn(), TEAMID(m_eAgent), kSASEvalParams.getTarget(), kSASEvalParams.isTotal(), kSASEvalParams.getPreparationTime(),
				pSASStepCity->city().getX(), pSASStepCity->city().getY(), pSASStepCity->getDistance(), iActualDuration, aiTurnsSimulated[eNextInvader], iTimeLimit,
				bStepFitsHorizon, kNextStep.isAttackerSuccessful(), kNextStep.getThreat().getPercent());
		}
		// Don't apply step if it would exceed the time limit
		if (bStepFitsHorizon)
		{
			applyStep(kNextStep);
			/*	If attack fails, the attacker doesn't get another step. However,
				the defending army portion assigned to that attacker mustn't
				be assumed to instantaneously redeploy to another front. */
			if (!kNextStep.isAttackerSuccessful())
				rPastThreat = rNextInvaderThreat;
			else rPastThreat = 0;
			aiTurnsSimulated[eNextInvader] += iActualDuration;
			if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_STEP_APPLIED turn=%d agentPlayer=%d defender=%d attacker=%d simulatedTurns=%d",
					GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, eNextInvader, aiTurnsSimulated[eNextInvader]);
			delete &kNextStep;
		}
		else
		{
			delete &kNextStep;
			// <!-- custom: Exclude only this over-horizon step for the current defender; continue scheduling other invaders whose actual steps may still fit. See KI#577. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			abHorizonExcluded[eNextInvader] = true;
			if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_STEP_EXCLUDED turn=%d agentPlayer=%d defender=%d attacker=%d reason=TIME_LIMIT timeLimit=%d",
					GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, eNextInvader, m_kOuter.m_iTimeLimit);
		}
	} while (!isEliminated());
}


void InvasionGraph::Node::changePrimaryTarget(Node* pNewTarget)
{
	Node* const pOldTarget = m_pPrimaryTarget;
	m_pPrimaryTarget = pNewTarget;
	bool const bLogInvasionTargetChanges = (gUWAIInvasionGraphLogLevel >= 2 && !m_kLogMuteState.isMuted());
	if (pNewTarget != NULL)
	{
		pNewTarget->m_targetedBy.insert(m_ePlayer);
		if (bLogInvasionTargetChanges) logBBAI("UWAI_INVASION_GRAPH_TARGET_SET turn=%d agentPlayer=%d player=%d target=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, pNewTarget->m_ePlayer);
	}
	if (pOldTarget != NULL)
	{
		pOldTarget->m_targetedBy.erase(m_ePlayer);
		if (bLogInvasionTargetChanges) logBBAI("UWAI_INVASION_GRAPH_TARGET_UNSET turn=%d agentPlayer=%d player=%d target=%d",
				GC.getGame().getGameTurn(), m_eAgent, m_ePlayer, pOldTarget->m_ePlayer);
	}
}


InvasionGraph::Node& InvasionGraph::Node::findSink()
{
	if (!hasTarget())
		return *this;
	return m_pPrimaryTarget->findSink();
}


void InvasionGraph::Node::clash(scaled rArmyPortion, scaled rTargetArmyPortion)
{
	Node& kTarget = *m_pPrimaryTarget;
	/*	Handling the clash in the step function is a bit of a hack. It turned out
		that a separate clash function would have a lot of overlap with step.
		A clash should be symmetrical, but, the way it's now implemented,
		step has to be called the on target and applyStep on the targeting node if
		it's a clash step (whereas, for city attack steps, it's the other way around).
		I've considered resolving clashes only as part of city attacks,
		but I want to resolve the clash first in order to give third parties
		a chance to conquer cities (pre-empting the winner of the clash). */
	SimulationStep& kClashStep = *kTarget.step(rArmyPortion, rTargetArmyPortion, true);
	applyStep(kClashStep);
	// Army surviving clash and going after cities won't be available for defense
	// <!-- custom: Inherited UWAI assigned each winner the opponent's independently computed Army portion, transferring asymmetric outside pressure to the wrong civilization.
	// Preserve each node's portion for both conquest and defense distraction. See KI#578. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (kClashStep.isAttackerSuccessful())
	{
		kTarget.m_rDistractionByConquest = rTargetArmyPortion;
		m_rDistractionByDefense = rArmyPortion;
	}
	else
	{
		m_rDistractionByConquest = rArmyPortion;
		kTarget.m_rDistractionByDefense = rTargetArmyPortion;
	}
	delete &kClashStep;
}


// <!-- custom: Accept the selected invasion mode so overland clashes use each side's available land-only deployment distance, while fleet-only clashes retain the mixed metric.
// A non-fleet clash requires only one side to have a qualifying land route, so fall back per side to its valid mixed distance rather than averaging an unavailable -1 land sentinel. See KI#591 and KI#1044. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
scaled InvasionGraph::Node::clashDistance(InvasionGraph::Node const& kOther,
	bool bNaval) const
{
	UWAICache::City const* pTargetCity = targetCity();
	UWAICache::City const* pOtherTargetCity = kOther.targetCity();
	// Clash half-way in the middle
	if (pTargetCity != NULL && pOtherTargetCity != NULL)
	{
		int const iDistance = (!bNaval && pTargetCity->getDistanceByLand() >= 0 ?
				pTargetCity->getDistanceByLand() : pTargetCity->getDistance());
		int const iOtherDistance = (!bNaval && pOtherTargetCity->getDistanceByLand() >= 0 ?
				pOtherTargetCity->getDistanceByLand() : pOtherTargetCity->getDistance());
		return scaled(iDistance + iOtherDistance, 2);
	}
	FErrorMsg("Shouldn't clash if not mutually reachable");
	return -1;
}


bool InvasionGraph::Node::isSneakAttack(InvasionGraph::Node const& kOther, bool bClash) const
{
	if ((m_ePlayer != m_eAgent && (!bClash || kOther.m_ePlayer != m_eAgent)) ||
		GET_TEAM(m_ePlayer).isAtWar(TEAMID(kOther.m_ePlayer)))
	{
		return false;
	}
	if (!bClash)
	{
		for (size_t i = 0; i < m_conquests.size(); i++)
		{
			if (m_conquests[i]->city().getOwner() == kOther.m_ePlayer)
				return false;
		}
	}
	// Agent can't ready its units then
	return !m_kOuter.m_kMA.evaluationParams().isImmediateDoW();
}


bool InvasionGraph::Node::isContinuedWar(Node const& kOther) const
{
	return (GET_TEAM(m_ePlayer).isAtWar(TEAMID(kOther.m_ePlayer)) &&
			m_conquests.empty());
}


void InvasionGraph::Node::getConquests(CitySet& kResult) const
{
	for (size_t i = 0; i < m_conquests.size(); i++)
		 kResult.insert(m_conquests[i]->getID());
}


void InvasionGraph::Node::getCityLosses(CitySet& kResult) const
{
	for (CitySetIter it = m_cityLosses.begin(); it != m_cityLosses.end(); ++it)
		kResult.insert(*it);
}


void InvasionGraph::Node::getCapitulationsAccepted(TeamSet& kResult) const
{
	for (TeamSetIter it = m_capitulationsAccepted.begin();
		it != m_capitulationsAccepted.end(); ++it)
	{
		kResult.insert(*it);
	}
}


void InvasionGraph::simulate(int iTurns)
{
	PROFILE_FUNC();
	/*	Build-up assumed to take place until 50% into the war. Use the result for
		the simulation of losses. Maybe should simulate the timing of reinforcements
		being deployed in more detail. As it is, the initial assault is overestimated
		and subsequent steps underestimated. */
	int const iInitialBuildUpTurns = intdiv::uround(iTurns, 2);
	int const iConcurrentBuildUpTurns = iTurns - iInitialBuildUpTurns;
	bool const bLogInvasionDetail = (gUWAIInvasionGraphLogLevel >= 3 && !m_kLogMuteState.isMuted());
	for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
	{
		PlayerTypes const ePlayer = it->getID();
		if (m_nodeMap[ePlayer] != NULL)
		{
			if (bLogInvasionDetail) m_nodeMap[ePlayer]->logTypicalUnits();
			m_nodeMap[ePlayer]->prepareForSimulation();
		}
	}
	if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_BUILDUP_INITIAL turn=%d agentPlayer=%d turns=%d",
			GC.getGame().getGameTurn(), m_eAgent, iInitialBuildUpTurns);
	// Assume that upgrades are done in the prolog, if any, and only in phase I.
	simulateArmament(iInitialBuildUpTurns, !m_bFirstSimulateCall);
	m_bFirstSimulateCall = false;
	// Simulation of losses over the whole duration
	m_iTimeLimit = iTurns;
	simulateLosses();
	if (m_bAllWarPartiesKnown)
		m_bLossesDone = true;
	/*	Needed for estimating the war effort. Assumes reduced production output if
		cities have been lost. */
	if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_BUILDUP_CONCURRENT turn=%d agentPlayer=%d turns=%d",
			GC.getGame().getGameTurn(), m_eAgent, iConcurrentBuildUpTurns);
	m_kLogMuteState.pushMute(); // Some more build-up isn't very interesting
	simulateArmament(iConcurrentBuildUpTurns, true);
	m_kLogMuteState.popMute();
}


void InvasionGraph::simulateArmament(int iTurns, bool bNoUpgrading)
{
	PROFILE_FUNC();
	for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
	{
		PlayerTypes const ePlayer = it->getID();
		if (m_nodeMap[ePlayer] != NULL)
			m_nodeMap[ePlayer]->predictArmament(iTurns, bNoUpgrading);
	}
}


void InvasionGraph::simulateLosses()
{
	PROFILE_FUNC();
	/*	Start with the agent's component, but simulate the other ones as well.
		While they can't affect the outcome of the agent's wars, anticipating
		conquests of other civs is going to be helpful for war evaluation. */
	if (m_nodeMap[m_eAgent] != NULL)
		simulateComponent(*m_nodeMap[m_eAgent]);
	for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
	{
		PlayerTypes const ePlayer = it->getID();
		if (m_nodeMap[ePlayer] != NULL && !m_nodeMap[ePlayer]->isComponentDone())
			simulateComponent(*m_nodeMap[ePlayer]);
	}
}


void InvasionGraph::simulateComponent(Node& kStart)
{
	/*	Key observation about invasion graphs: Each component contains
		at most one cycle. The cycle often consists of just two civs targeting
		each other, but it could be a triangle as well, or a more complex
		polygon. The cycle property is a consequence of each node having an
		out-degree of at most 1. Around the central cycle, there can be trees,
		each rooted at a node within the cycle. (The edges point towards
		the root.) If an edge is removed from the cycle, the remaining dag
		has a single sink (out-degree 0). */
	bool const bLogInvasionDetail = (gUWAIInvasionGraphLogLevel >= 3 && !m_kLogMuteState.isMuted());
	// <!-- custom: Isolation is an attribute of this component start, not a separate event; keep it on the begin row to avoid duplicate context. (ChatGPT-5.6-Sol) -->
	if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_COMPONENT_BEGIN turn=%d agentPlayer=%d startPlayer=%d isolated=%d",
			GC.getGame().getGameTurn(), m_eAgent, kStart.getPlayer(), kStart.isIsolated());
	vector<Node*> apForwardPath;
	size_t uiStartOfCycle = kStart.findCycle(apForwardPath);
	vector<Node*> aCycle;
	aCycle.insert(aCycle.begin(), apForwardPath.begin() + uiStartOfCycle,
			apForwardPath.end());
	if (aCycle.size() == 1)
	{
		FAssert(aCycle.size() != 1);
		return;
	}
	if(!aCycle.empty())
	{
		if (bLogInvasionDetail)
		{
			for (size_t i = 0; i < aCycle.size(); i++)
			{
				PlayerTypes const eFrom = aCycle[i]->getPlayer();
				PlayerTypes const eTo = aCycle[(i + 1) % aCycle.size()]->getPlayer();
				logBBAI("UWAI_INVASION_GRAPH_CYCLE_EDGE turn=%d agentPlayer=%d player=%d target=%d",
						GC.getGame().getGameTurn(), m_eAgent, eFrom, eTo);
			}
		}
		breakCycle(aCycle);
	}
	kStart.findSink().resolveLossesRec();
}


void InvasionGraph::breakCycle(vector<Node*> const& kCycle)
{
	bool const bLogInvasionDetail = (gUWAIInvasionGraphLogLevel >= 3 && !m_kLogMuteState.isMuted());
	if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_CYCLE_BREAK turn=%d agentPlayer=%d length=%d",
			GC.getGame().getGameTurn(), m_eAgent, (int)kCycle.size());
	/*	Treat the simplest case upfront: two nodes targeting each other,
		and no further nodes involved. */
	if (kCycle.size() == 2 && kCycle[0]->getTargetedBy().size() == 1 &&
		kCycle[1]->getTargetedBy().size() == 1)
	{
		kCycle[0]->clash(1, 1);
		return;
		/*	The code below could handle this case as well, but leads to
			unnecessary computations and logging. */
	}
	// Only for cycles of length > 2
	int iToughestEnemiesCycleIndex = -1;
	scaled rHighestEnemyThreat = -1;
	// Only for length 2
	scaled rFirstArmyPortion, rSecondArmyPortion;
	for (size_t i = 0; i < kCycle.size(); i++)
	{
		Node& kNode = *kCycle[i];
		scaled rThreatOfAttackFromCycle = -1;
		scaled rSumOfEnemyThreat;
		/*	The targeting Nodes may themselves be busy fending off attackers.
			This is taken into account when resolving losses, but not when
			breaking cycles. */
		PlyrSet const& kTargetedBy = kNode.getTargetedBy();
		for (PlyrSetIter it = kTargetedBy.begin(); it != kTargetedBy.end(); ++it)
		{
			if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_CYCLE_THREAT_ASSESS turn=%d agentPlayer=%d attacker=%d defender=%d ignoreDefenderArmy=1",
					GC.getGame().getGameTurn(), m_eAgent, m_nodeMap[*it]->getPlayer(), kNode.getPlayer());
			m_kLogMuteState.pushMute();
			SimulationStep* pStep = m_nodeMap[*it]->step(0, 1, false, true);
			m_kLogMuteState.popMute();
			if (pStep == NULL)
			{
				/*	Might be fine in some circumstances, but suggests inconsistent
					results of the targetCity and findTarget function. The former
					relies on the order of the m_kCache. */
				FAssert(pStep != NULL);
				#ifdef _DEBUG
					// Call step function again for investigation in the debugger
					FAssert(pStep != NULL || m_nodeMap[*it]->step() != NULL);
				#endif
				continue;
				/*	Not sure how to handle it gracefully when there's no threat from
					_any_ node targeting n, i.e. when step returns NULL for all *it. */
			}
			scaled const rBaseThreat = pStep->getThreat();
			scaled const rThreat = rBaseThreat *
					willingness(m_nodeMap[*it]->getPlayer(), kNode.getPlayer());
			if (!rThreat.approxEquals(rBaseThreat, fixp(0.005)))
			{
				if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_CYCLE_THREAT_ADJUSTED turn=%d agentPlayer=%d attacker=%d defender=%d basePercent=%d adjustedPercent=%d",
						GC.getGame().getGameTurn(), m_eAgent, m_nodeMap[*it]->getPlayer(), kNode.getPlayer(), rBaseThreat.getPercent(), rThreat.getPercent());
			}
			else if (bLogInvasionDetail) logBBAI("UWAI_INVASION_GRAPH_CYCLE_THREAT_UNCHANGED turn=%d agentPlayer=%d attacker=%d defender=%d basePercent=%d adjustedPercent=%d",
					GC.getGame().getGameTurn(), m_eAgent, m_nodeMap[*it]->getPlayer(), kNode.getPlayer(), rBaseThreat.getPercent(), rThreat.getPercent());
			rSumOfEnemyThreat += rThreat;
			// Only relevant for kCycle.size() == 2
			if (pStep->getAttacker() == kNode.getPrimaryTarget()->getPlayer())
				rThreatOfAttackFromCycle = rThreat;
			delete pStep;
		}
		if (rSumOfEnemyThreat > rHighestEnemyThreat)
		{
			rHighestEnemyThreat = rSumOfEnemyThreat;
			iToughestEnemiesCycleIndex = i;
		}
		/*	Only relevant for kCycle.size() == 2. Two civs target each other,
			and others may intefere from the outside. Army portions available
			for clash depend on those outside threats. */
		FAssert(rSumOfEnemyThreat >= rThreatOfAttackFromCycle);
		// Coefficient: prioritize primary target
		scaled rArmyPortion = fixp(1.45) * rThreatOfAttackFromCycle /
				(rSumOfEnemyThreat + scaled::epsilon());
		rArmyPortion.decreaseTo(1);
		if (i == 0)
			rFirstArmyPortion = rArmyPortion;
		if (i == 1)
			rSecondArmyPortion = rArmyPortion;
	}
	if (kCycle.size() > 2)
	{
		Node& kToughestEnemiesNode = *kCycle[iToughestEnemiesCycleIndex];
		/*	Break the cycle by removing the target of the most besieged node.
			Rationale: This node is likely to need all its units for defending
			itself, or at least likelier than any other node in the cycle. */
		kToughestEnemiesNode.changePrimaryTarget(NULL);
	}
	else kCycle[0]->clash(rFirstArmyPortion, rSecondArmyPortion);
}

/*	UWAI::Player::warConfidenceAllies is about our general confidence in
	war allies, regardless of circumstances. This function is about gauging
	rationally whether a war party (eAggressor) is willing to commit resources
	against another (eTarget).
	This is to make sure that the AI can't be goaded into a joint war by a human,
	and that joint wars are generally expensive enough. */
scaled InvasionGraph::willingness(PlayerTypes eAggressor, PlayerTypes eTarget) const
{
	if (m_kMA.evaluationParams().getSponsor() != eAggressor)
		return 1;
	scaled r =
			(GET_TEAM(eAggressor).AI_getWarSuccess(TEAMID(eTarget)) +
			GET_TEAM(eTarget).AI_getWarSuccess(TEAMID(eAggressor))) /
			std::max(GET_TEAM(eAggressor).getNumCities() * 4, 1);
	r.clamp(fixp(0.5), 1);
	return r;
}


SimulationStep::SimulationStep(PlayerTypes eAttacker, UWAICache::City const* pContestedCity)
:	m_eAttacker(eAttacker), m_pContestedCity(pContestedCity),
	m_iDuration(0), m_bSuccess(false)
{
	for(int i = 0; i < NUM_BRANCHES; i++)
		m_arLostPowerAttacker[i] = m_arLostPowerDefender[i] = 0;
}


void SimulationStep::reducePower(PlayerTypes ePlayer, MilitaryBranchTypes eBranch, scaled rSubtrahend)
{
	FAssert(rSubtrahend >= 0);
	rSubtrahend.increaseTo(0);
	(ePlayer == m_eAttacker ?
			m_arLostPowerAttacker[eBranch] :
			m_arLostPowerDefender[eBranch]) += rSubtrahend;
}


scaled SimulationStep::getLostPower(PlayerTypes ePlayer, MilitaryBranchTypes eBranch) const
{
	if (ePlayer == m_eAttacker)
		return m_arLostPowerAttacker[eBranch];
	return m_arLostPowerDefender[eBranch];
}

/*	Underlying model: Only a portion of the invaders clashes - clashPortion -,
	the rest arrive in time for siege, but not for a clash.
	Moreover, the less powerful side will avoid a clash with probability
	equal to the power ratio; assume average-size forces by multiplying by the
	power ratio.
	Assume that the outcome of the actual clash depends only on the power values,
	not on which side has more advanced units. Can then assume w.l.o.g. that
	all units are equal and that each unit of power corresponds to one combat unit.
	Assume that each unit of the losing side fights once, leading to equal losses
	of 0.5 * lesserPow on both sides. The additional units of the winning side then
	manage to attack the damaged surviving units a few times more, leading
	to additional losses only for the losing side. Assume that half of the additional
	units manage to make such an additional attack.
	After some derivation, this result in the following formulas:
	lossesWinner = 0.5 * clashPortion * lesserPow^2 / greaterPow
	lossesLoser = 0.5 * clashPortion * lesserPow.
	bNearCity=true means that the clash happens as part of a city attack, i.e. near
	a city of the defender. In this case, if the attacker loses, higher losses are
	assumed b/c it's difficult to withdraw from hostile territory. Assume that a clash
	is 1.6 times as likely (i.e. more units assumed to clash) as in the
	bNearCity=false case - the attacker can't easily back up. This assumption leads
	to higher losses for both sides. Additionally, 2/3 of the damaged units of the
	(unsuccessful) attacker are assumed to be killed. To simplify the math a little,
	I'm assuming that 4 in 5 attackers are killed, i.e. not based on by how much the
	attacker is outnumbered. Formulas:
	stake = clashPortion * min{1, 1.6 * powerRatio}
	lossesAtt = stake * 4/5 * powAtt
	lossesDef = stake * 1/2 * powAtt
	clashLossesTemporary (inlined in header) accounts for units of the winner
	that are significantly damaged and thus can't continue the invasion;
	they're assumed to be sidelined until the end of the simulation phase.
	2/3 of the successful attacks are assumed to lead to significant damage
	on the survivor.
	Damaged survivors of the loser are not accounted for b/c most units of the losing
	side participating in the clash are killed according to the above assumptions,
	and the rest may well be able to heal until the city they retreat to is attacked.
	The resulting formula is
	tempLosses = 1/3 * clashPortion * lesserPow
	(in addition to lossesWinner). */
scaled const InvasionGraph::Node::m_rClashPortion = fixp(0.65);

std::pair<scaled,scaled> InvasionGraph::Node::clashLossesWinnerLoser(scaled rPowAtt, scaled rPowDef, bool bNearCity, bool bNaval)
{
	scaled rLesserPow = std::min(rPowAtt, rPowDef);
	scaled rGreaterPow = std::max(rPowAtt, rPowDef);
	if (rGreaterPow < 1)
		return std::make_pair(0, 0);
	scaled rClashPortionWinner = m_rClashPortion;
	scaled rClashPortionLoser = m_rClashPortion;
	/*	Since I've gotten rid of the bNearCity=true case for non-naval
		attacks, the initial clash needs to produce higher losses;
		hence the increased clash portions.
		Need higher losses for the loser in order to ensure that it's better to win
		a clash and fail to conquer a city than to lose the clash. */
	if (!bNaval && !bNearCity)
	{
		rClashPortionWinner += fixp(0.15);
		rClashPortionLoser += fixp(0.35);
	}
	if (!bNearCity || rPowAtt > rPowDef)
	{
		return std::make_pair(
				fixp(0.5) * rClashPortionWinner * SQR(rLesserPow) / rGreaterPow,
				fixp(0.5) * rClashPortionLoser * rLesserPow);
	}
	return std::make_pair(
			fixp(0.5) * stake(rPowAtt, rPowDef) * rPowAtt,
			fixp(0.8) * stake(rPowAtt, rPowDef) * rPowAtt);
}
