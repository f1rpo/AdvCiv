#include "CvGameCoreDLL.h"
#include "MilitaryAnalyst.h"
#include "WarEvalParameters.h"
#include "UWAILogMuteState.h" // <!-- custom: Concrete nested-mute state is needed because MilitaryAnalyst queries it while the evaluation tree shares the same mute depth. (ChatGPT-5.6-Sol) -->
#include "UWAIAgent.h"
#include "InvasionGraph.h"
#include "CoreAI.h"
#include "CvCity.h"
#include "CvPlot.h"
#include "CvInfo_GameOption.h"
#include "BBAILog.h" // <!-- custom: Cached SAS_BBAI UWAI subsystem levels pre-gate inherited diagnostic work before argument/setup cost. See KI#505.3. (ChatGPT-5.6-Sol) -->



// empty sets (static)
CitySet MilitaryAnalyst::m_emptyCitySet;
PlyrSet MilitaryAnalyst::m_emptyPlayerSet;

namespace
{
	// <!-- custom: Interception belongs to the defending team, while the assumed Tactical-Nuke share belongs to the firing team. See KI#966. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	scaled nukeChanceToHit(TeamTypes eAttacker, TeamTypes eDefender)
	{
		scaled const rInterceptionProb = per100(std::max(
				GET_TEAM(eDefender).getNukeInterception(),
				// advc.143b:
				GET_TEAM(GET_TEAM(eDefender).getMasterTeam()).getNukeInterception()));
		scaled const rEvasionProb = fixp(0.5); // Fixme: Shouldn't hardcode this
		// Percentage of Tactical Nukes
		scaled rTactRatio = (GET_TEAM(eAttacker).isHuman() ? fixp(0.5) : fixp(1/3.));
		scaled rHitProb = 1 - (rInterceptionProb *
				((1 - rTactRatio) + rTactRatio * (1 - rEvasionProb)));
		rHitProb.clamp(0, 1);
		return rHitProb;
	}

	// Known to be at war with anyone. If no observer, all wars are checked.
	bool isKnownToBeAtWar(TeamTypes eTeam, TeamTypes eObserver)
	{
		for (TeamIter<MAJOR_CIV,ENEMY_OF> it(eTeam); it.hasNext(); ++it)
		{
			if (eObserver == NO_TEAM || GET_TEAM(eObserver).isHasMet(it->getID()))
				return true;
		}
		return false;
	}
}


MilitaryAnalyst::MilitaryAnalyst(PlayerTypes eAgentPlayer, WarEvalParameters& kWarEvalParams, bool bPeaceScenario)
// <!-- custom: Share one nested mute state across MilitaryAnalyst and its InvasionGraph/forecast descendants; output itself is ordinary BBAI. (ChatGPT-5.6-Sol) -->
:	m_kWarEvalParams(kWarEvalParams), m_kLogMuteState(kWarEvalParams.getLogMuteState()),
	m_eWe(eAgentPlayer), m_eTarget(kWarEvalParams.getTarget()),
	m_bPeaceScenario(bPeaceScenario), m_iTurnsSimulated(0)
{
	PROFILE_FUNC();
	// <!-- custom: Mute depth is stable across this constructor; nested descendants may push/pop it only in balanced scopes.
	// Reuse one descriptive level-2 gate for the constructor's scenario-context rows. (ChatGPT-5.6-Sol) -->
	bool const bLogMilitaryAnalystContext = (gUWAIMilitaryAnalystLogLevel >= 2 && !m_kLogMuteState.isMuted());
	m_playerResults.resize(MAX_CIV_PLAYERS, NULL);
	m_warTable.resize(MAX_CIV_PLAYERS, std::vector<bool>(MAX_CIV_PLAYERS, false));
	m_nukedCities.resize(MAX_CIV_PLAYERS, std::vector<scaled>(MAX_CIV_PLAYERS, 0));
	m_capitulationsAcceptedPerTeam.resize(MAX_CIV_TEAMS);
	if (gUWAIMilitaryAnalystLogLevel >= 1 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_MILITARY_ANALYSIS_BEGIN turn=%d agentPlayer=%d agentTeam=%d targetTeam=%d scenario=%s",
			GC.getGame().getGameTurn(), m_eWe, TEAMID(m_eWe), m_eTarget, m_bPeaceScenario ? "PEACE" : "WAR");
	CvTeamAI const& kAgent = GET_TEAM(m_eWe);
	PlyrSet currentlyAtWar; // ("atWar" is already a name of a global function)
	PlyrSet ourFutureOpponents;
	PlyrSet ourSide;
	PlyrSet theirSide;
	PlyrSet weAndOurVassals;
	PlyrSet ourAllies;
	PlyrSet theyAndTheirVassals;
	// Exclude unknown civs from analysis
	for (PlayerIter<MAJOR_CIV,KNOWN_TO> it(kAgent.getID()); it.hasNext(); ++it)
	{
		PlayerTypes const ePlayer = it->getID();
		/*	Civs already at war (not necessarily with us).
			If we are at war, our vassals are covered here as well. */
		if (isKnownToBeAtWar(TEAMID(ePlayer), kAgent.getID()))
			currentlyAtWar.insert(ePlayer);
		TeamTypes eMaster = GET_TEAM(ePlayer).getMasterTeam();
		if (eMaster == kAgent.getMasterTeam())
			weAndOurVassals.insert(ePlayer);
		if (eMaster == GET_TEAM(m_eTarget).getMasterTeam())
			theyAndTheirVassals.insert(ePlayer);
		if (m_kWarEvalParams.isWarAlly(GET_TEAM(ePlayer).getMasterTeam()))
			ourAllies.insert(ePlayer);
		if (m_kWarEvalParams.isExtraTarget(GET_TEAM(ePlayer).getMasterTeam()))
			theirSide.insert(ePlayer);
		// Civs soon to be at war with us or an ally that we bring in
		if (doWePlanToDeclWar(ePlayer) || (m_kWarEvalParams.isAnyWarAlly() &&
			kAgent.isAtWar(TEAMID(ePlayer))))
		{
			// doWePlanToDeclWar checks if ePlayer is part of the enemy team
			ourFutureOpponents.insert(ePlayer);
			theirSide.insert(ePlayer);
			// Defensive pacts that our war plans will trigger
			for(PlayerIter<MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itAlly(kAgent.getID());
				itAlly.hasNext(); ++itAlly)
			{
				// Can happen b/c of the kekm.3 change
				if (kAgent.isAtWar(itAlly->getTeam()))
					continue;
				if (isDefactoDefensivePact(TEAMID(ePlayer), itAlly->getTeam()))
					ourFutureOpponents.insert(itAlly->getID());
			}
		}
	}
	ourSide.insert(weAndOurVassals.begin(), weAndOurVassals.end());
	ourSide.insert(ourAllies.begin(), ourAllies.end());
	theirSide.insert(theyAndTheirVassals.begin(), theyAndTheirVassals.end());
	bool const bNoWarVsExtra = (m_bPeaceScenario && m_kWarEvalParams.isNoWarVsExtra());
	PlyrSet& declaringWar = ((kAgent.isAtWar(m_eTarget) &&
			!m_kWarEvalParams.isConsideringPeace()) ? ourAllies : ourSide);
	if ((!m_bPeaceScenario && !m_kWarEvalParams.isConsideringPeace()) || bNoWarVsExtra)
	{
		// Store war-scenario DoW for getWarsDeclaredOn/By
		for (PlyrSetIter it1 = declaringWar.begin();
			it1 != declaringWar.end(); ++it1)
		{
			for (PlyrSetIter it2 = theirSide.begin();
				it2 != theirSide.end(); ++it2)
			{
				if (!GET_TEAM(*it1).isAtWar(TEAMID(*it2)))
				{
					playerResult(*it1).setDoWBy(*it2);
					playerResult(*it2).setDoWOn(*it1);
				}
			}
		}
		PlyrSet theirDPAllies;
		for(PlayerIter<MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> it(kAgent.getID());
			it.hasNext(); ++it)
		{
			PlayerTypes const ePlayer = it->getID();
			if (kAgent.isAtWar(TEAMID(ePlayer)))
				continue;
			if (isOnTheirSide(TEAMID(ePlayer), true) &&
				theirSide.count(ePlayer) <= 0)
			{
				theirDPAllies.insert(ePlayer);
			}
		}
		#ifdef _DEBUG
		{
			PlyrSet isect;
			set_intersection(theirSide.begin(), theirSide.end(),
					theirDPAllies.begin(), theirDPAllies.end(), std::inserter(
					 isect, isect.begin()));
			FAssert(isect.empty());
		}
		#endif
		for (PlyrSetIter it = theirDPAllies.begin(); it != theirDPAllies.end(); ++it)
			playerResult(*it).setDoWBy(declaringWar.begin(), declaringWar.end());
		for (PlyrSetIter it = declaringWar.begin(); it != declaringWar.end(); ++it)
			playerResult(*it).setDoWOn(theirDPAllies.begin(), theirDPAllies.end());
	}
	// Wars continued
	for (PlayerIter<MAJOR_CIV> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		PlayerTypes const ePlayer = itPlayer->getID();
		for(PlayerIter<MAJOR_CIV,ENEMY_OF> itEnemy(TEAMID(ePlayer));
			itEnemy.hasNext(); ++itEnemy)
		{
			PlayerTypes const eEnemy = itEnemy->getID();
			if ((declaringWar.count(ePlayer) <= 0 || !m_bPeaceScenario ||
				theirSide.count(eEnemy) <= 0) &&
				(declaringWar.count(eEnemy) <= 0 || !m_bPeaceScenario ||
				theirSide.count(ePlayer) <= 0))
			{
				playerResult(ePlayer).setWarContinued(eEnemy);
				FAssert(getWarsDeclaredBy(ePlayer).count(eEnemy) <= 0);
				FAssert(getWarsDeclaredBy(eEnemy).count(ePlayer) <= 0);
				FAssert(getWarsDeclaredOn(ePlayer).count(eEnemy) <= 0);
				FAssert(getWarsDeclaredOn(eEnemy).count(ePlayer) <= 0);
			}
		}
	}
	// Need ArmamentForecasts for initially uninvolved parties (if any)
	PlyrSet uninvolved;
	uninvolved.insert(declaringWar.begin(), declaringWar.end());
	uninvolved.insert(ourFutureOpponents.begin(), ourFutureOpponents.end());
	m_partOfAnalysis.insert(currentlyAtWar.begin(), currentlyAtWar.end());
	m_partOfAnalysis.insert(uninvolved.begin(), uninvolved.end());
	m_pInvGraph = new InvasionGraph(*this, currentlyAtWar, m_bPeaceScenario);
	m_pInvGraph->addUninvolvedParties(uninvolved);
	/*	Simulation in two phases: The first phase lasts until we have finished
		our war preparations (NB: we may already be in a war while preparing
		for another); the second phase lasts for a set number of turns. */
	int iPrepTime = m_kWarEvalParams.getPreparationTime();
	/*	Above, uninvolved parties are added and preparation time is set
		regardless of peace scenario; otherwise, the results for the
		war and peace scenario wouldn't be fully comparable. */
	if (m_bPeaceScenario)
	{
		// Master isn't included in the removed war opponents (theirSide) then
		FAssert(!kAgent.isAtWar(m_eTarget) || !GET_TEAM(m_eTarget).isAVassal());
		if (bNoWarVsExtra)
		{
			PlyrSet diff; // diff = theirSide minus theyAndTheirVassals
			std::set_difference(theirSide.begin(), theirSide.end(),
					theyAndTheirVassals.begin(), theyAndTheirVassals.end(),
					std::inserter(diff, diff.begin()));
			m_pInvGraph->removeWar(declaringWar, diff);
		}
		/*	Capitulation: Not fully implemented; misses the wars that we need
			to declare on the enemies of theyId. */
		else if (m_kWarEvalParams.getCapitulationTeam() == m_eTarget)
			m_pInvGraph->removeWar(declaringWar, currentlyAtWar);
		else m_pInvGraph->removeWar(declaringWar, theirSide);
	}
	int iTimeHorizon = 25;
	// Look a bit farther into the future when in a total war
	if (iPrepTime <= 0 && m_kWarEvalParams.isTotal())
		iTimeHorizon += 5;
	// Look a bit farther on Marathon speed
	if(GC.getInfo(GC.getGame().getGameSpeedType()).getGoldenAgePercent() >= 150)
		iTimeHorizon += 5;
	/*  Skip phase 1 if it would be short (InvasionGraph::Node::isSneakAttack
		will still read the actual prep time from the WarEvalParameters) */
	if (iPrepTime < 4)
	{
		if (bLogMilitaryAnalystContext && iPrepTime > 0) logBBAI("UWAI_MILITARY_ANALYSIS_PREP_SKIPPED turn=%d agentPlayer=%d targetTeam=%d scenario=%s prepTurns=%d",
				GC.getGame().getGameTurn(), m_eWe, m_eTarget, m_bPeaceScenario ? "PEACE" : "WAR", iPrepTime);
		iTimeHorizon += iPrepTime; // Prolong 2nd phase instead
		iPrepTime = 0;
	}
	else
	{
		// <!-- custom: Keep %S player-name arguments out of narrow-string ternaries; CvPlayer::getName(0) is wchar const* on the Civ4 toolchain. (ChatGPT-5.6-Sol) -->
		if (bLogMilitaryAnalystContext && m_bPeaceScenario) logBBAI("UWAI_MILITARY_ANALYSIS_PHASE1_PEACE turn=%d agentPlayer=%d targetTeam=%d turns=%d",
			GC.getGame().getGameTurn(), m_eWe, m_eTarget, iPrepTime);
		else if (bLogMilitaryAnalystContext) logBBAI("UWAI_MILITARY_ANALYSIS_PHASE1_WAR_PREPARATION turn=%d agentPlayer=%d targetTeam=%d turns=%d",
			GC.getGame().getGameTurn(), m_eWe, m_eTarget, iPrepTime);
		m_pInvGraph->simulate(iPrepTime);
		m_iTurnsSimulated += iPrepTime;
	}
	if (!m_bPeaceScenario || bNoWarVsExtra)
		m_pInvGraph->addFutureWarParties(declaringWar, ourFutureOpponents);
	/*  Force update of targets (needed even if no parties were added b/c of
		defeats in phase I. */
	if (m_bPeaceScenario)
		m_pInvGraph->updateTargets();
	if (bLogMilitaryAnalystContext && !m_bPeaceScenario && !kAgent.isAtWar(m_eTarget)) logBBAI("UWAI_MILITARY_ANALYSIS_PHASE2_ASSUMED_DOW turn=%d agentPlayer=%d targetTeam=%d turns=%d",
		GC.getGame().getGameTurn(), m_eWe, m_eTarget, iTimeHorizon);
	else if (bLogMilitaryAnalystContext) logBBAI("UWAI_MILITARY_ANALYSIS_PHASE2 turn=%d agentPlayer=%d targetTeam=%d scenario=%s turns=%d",
		GC.getGame().getGameTurn(), m_eWe, m_eTarget, m_bPeaceScenario ? "PEACE" : "WAR", iTimeHorizon);
	m_pInvGraph->simulate(iTimeHorizon);
	m_iTurnsSimulated += iTimeHorizon;
	prepareResults(); // ... of conventional war
	simulateNuclearWar();
}


MilitaryAnalyst::~MilitaryAnalyst()
{
	delete m_pInvGraph;
	for (size_t i = 0; i < m_playerResults.size(); i++)
		SAFE_DELETE(m_playerResults[i]);
}


MilitaryAnalyst::PlayerResult& MilitaryAnalyst::playerResult(PlayerTypes ePlayer)
{
	/*	Lazy creation of playerResults means that NULL needs to be checked when accessing
		a PlayerResult and that memory needs to be allocated dynamically. Based on a test,
		this is still faster than creating a PlayerResult for every player upfront and
		accessing the empty sets of the dummy entries. */
	FAssertBounds(0, MAX_CIV_PLAYERS, ePlayer);
	if (m_playerResults[ePlayer] == NULL)
		m_playerResults[ePlayer] = new PlayerResult();
	return *m_playerResults[ePlayer];
}


bool MilitaryAnalyst::isOnOurSide(TeamTypes eTeam) const
{
	return (GET_TEAM(eTeam).getMasterTeam() == GET_TEAM(m_eWe).getMasterTeam() ||
			m_kWarEvalParams.isWarAlly(eTeam));
}


bool MilitaryAnalyst::isOnTheirSide(TeamTypes eTeam, bool bDefensivePacts) const
{
	bool bR = (GET_TEAM(eTeam).getMasterTeam() ==
			GET_TEAM(m_kWarEvalParams.getTarget()).getMasterTeam() ||
			m_kWarEvalParams.isExtraTarget(eTeam));
	if (!bDefensivePacts || bR)
		return bR;
	for (TeamIter<MAJOR_CIV> it; it.hasNext(); ++it)
	{
		if (GET_TEAM(eTeam).isDefensivePact(it->getID()) &&
			isOnTheirSide(it->getID()))
		{
			return true;
		}
	}
	return false;
}


bool MilitaryAnalyst::doWePlanToDeclWar(PlayerTypes ePlayer) const
{
	CvTeamAI const& kAgent = GET_TEAM(m_eWe);
	TeamTypes const eTeam = TEAMID(ePlayer);
	/*	Ongoing war preparations will be replaced by the war plan under
		consideration if adopted; disregard those. */
	return (//kAgent.AI_getWarPlan(eTeam) != NO_WARPLAN ||
			eTeam == m_kWarEvalParams.getTarget()) &&
			!kAgent.isAtWar(eTeam);
}


void MilitaryAnalyst::simulateNuclearWar()
{
	// Only simulates nukes fired by or on us
	PlayerTypes const eWe = m_eWe; // abbreviate
	CvPlayerAI const& kWe = GET_PLAYER(eWe);
	// <!-- custom: Conventional simulation runs first; use its remaining-city state throughout the later nuclear exchange so eliminated players cannot fire, receive or dilute nukes. See KI#967. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (simulatedRemainingCities(eWe) <= 0)
		return; // Who cares then
	CvTeamAI const& kAgent = GET_TEAM(eWe);
	UWAICache const& kOurCache = kWe.uwai().getCache();
	/*	Counts the number of nukes. Assume no further build-up of nukes throughout
		the military analysis. (They take long to build.) */
	int const iOurNukes = kOurCache.getPowerValues()[NUCLEAR]->numUnits();
	scaled rOurTargets;
	for (PlayerIter<MAJOR_CIV> itEnemy; itEnemy.hasNext(); ++itEnemy)
	{
		if (isWar(eWe, itEnemy->getID()) && simulatedRemainingCities(itEnemy->getID()) > 0)
			rOurTargets += (itEnemy->isAVassal() ? fixp(0.5) : 1);
	}
	for (PlayerAIIter<MAJOR_CIV> itEnemy; itEnemy.hasNext(); ++itEnemy)
	{
		CvPlayerAI const& kEnemy = *itEnemy;
		if (!isWar(eWe, kEnemy.getID()) || simulatedRemainingCities(kEnemy.getID()) <= 0)
			continue;
		int iEnemyNukes = kEnemy.uwai().getCache().
				getPowerValues()[NUCLEAR]->numUnits();
		if (iOurNukes + iEnemyNukes <= 0)
			continue;
		// The AI is (much) more willing to fire nukes in total war than in limited war
		WarPlanTypes const eOurWP = kAgent.AI_getWarPlan(kEnemy.getTeam());
		WarPlanTypes const eEnemyWP = GET_TEAM(kEnemy.getTeam()).AI_getWarPlan(
				kAgent.getID());
		// If one side goes total, the other will follow suit.
		bool bTotal = false;
		if (eOurWP == WARPLAN_PREPARING_TOTAL || eOurWP == WARPLAN_TOTAL ||
			eEnemyWP == WARPLAN_PREPARING_TOTAL || eEnemyWP == WARPLAN_TOTAL)
		{
			bTotal = true;
		}
		// (Human: let's trust the war plan type of the human proxy AI)
		if (m_kWarEvalParams.getTarget() == kEnemy.getTeam())
			bTotal = m_kWarEvalParams.isTotal();
		scaled rPortionFired = (bTotal ? fixp(0.75) : fixp(0.25));
		// Assume that vassals are hit by fewer nukes, and fire fewer nukes.
		scaled rOurVassalMult = 1;
		if (kAgent.isAVassal())
			rOurVassalMult /= 2;
		scaled rEnemyVassalMult = 1;
		if (kEnemy.isAVassal())
			rEnemyVassalMult /= 2;
		scaled rEnemyTargets;
		for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
		{
			CvPlayer const& kEnemyOfEnemy = *it;
			if (isWar(kEnemyOfEnemy.getID(), kEnemy.getID()) &&
				simulatedRemainingCities(kEnemyOfEnemy.getID()) > 0)
			{
				rEnemyTargets += (kEnemyOfEnemy.isAVassal() ? fixp(0.5) : 1);
			}
		}
		scaled rFiredOnUs = rPortionFired * rOurVassalMult * rEnemyVassalMult *
				/*  sqrt for pessimism - we may get hit worse than the
					average target, and that worries us. */
				(iEnemyNukes / rEnemyTargets.sqrt());
		rFiredOnUs.decreaseTo(fixp(1.5) * simulatedRemainingCities(eWe));
		scaled rFiredByUs;
		if (rOurTargets > 0)
		{
			rFiredByUs = rPortionFired * rOurVassalMult * rEnemyVassalMult * iOurNukes *
					(kWe.AI_isDoStrategy(AI_STRATEGY_OWABWNW) ? fixp(1.1) : fixp(0.9)) /
					rOurTargets;
		}
		rFiredByUs.decreaseTo(fixp(1.5) * simulatedRemainingCities(kEnemy.getID()));
		scaled rHittingUs = rFiredOnUs * nukeChanceToHit(kEnemy.getTeam(), kAgent.getID());
		scaled rHittingEnemy = rFiredByUs * nukeChanceToHit(kAgent.getID(), kEnemy.getTeam());
		// Make sure not to allocate PlayerResult unnecessarily
		if (rHittingUs > 0)
		{
			playerResult(eWe).addNukesSuffered(rHittingUs);
			m_nukedCities[kEnemy.getID()][eWe] += rHittingUs;
		}
		if (rHittingEnemy > 0)
		{
			playerResult(kEnemy.getID()).addNukesSuffered(rHittingEnemy);
			m_nukedCities[eWe][kEnemy.getID()] += rHittingEnemy;
		}
		if (rFiredByUs > 0)
			playerResult(eWe).addNukesFired(rFiredByUs);
		if (rFiredOnUs > 0)
			playerResult(kEnemy.getID()).addNukesFired(rFiredOnUs);
	}
}


// <!-- custom: Nuclear exchange follows conventional simulation, so real city counts include cities already predicted lost.
// Keep this analysis local and use the graph result instead. See KI#967. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
int MilitaryAnalyst::simulatedRemainingCities(PlayerTypes ePlayer) const
{
	return std::max(0, GET_PLAYER(ePlayer).getNumCities() - (int)lostCities(ePlayer).size());
}


bool MilitaryAnalyst::isDefactoDefensivePact(TeamTypes eFirst, TeamTypes eSecond) const
{
	CvTeam const& kFirst = GET_TEAM(eFirst);
	return (kFirst.isDefensivePact(eSecond) ||
			kFirst.isVassal(eSecond) || GET_TEAM(eSecond).isVassal(eFirst) ||
			(kFirst.getMasterTeam() == GET_TEAM(eSecond).getMasterTeam() &&
			eFirst != eSecond));
}


void MilitaryAnalyst::prepareResults()
{
	PROFILE_FUNC();
	for (PlayerIter<MAJOR_CIV> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		PlayerTypes const ePlayer = itPlayer->getID();
		InvasionGraph::Node const* pNode = m_pInvGraph->getNode(ePlayer);
		if (pNode == NULL)
			continue;
		// Allocate PlayerResult only if necessary
		if (pNode->anyConquests())
			pNode->getConquests(playerResult(ePlayer).getConqueredCities());
		if (pNode->anyCityLosses())
			pNode->getCityLosses(playerResult(ePlayer).getLostCities());
		// <!-- custom: This raw vector is indexed by TeamTypes; unlike overloaded GET_TEAM(PlayerTypes), operator[] cannot convert a player ID.
		// Union every member node's team-level result into its actual team slot. See KI#442. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		pNode->getCapitulationsAccepted(m_capitulationsAcceptedPerTeam[TEAMID(ePlayer)]);
	}
	// Predict scores as current game score modified based on gained/ lost population
	CvGame const& kGame = GC.getGame();
	UWAICache const& kOurCache = GET_PLAYER(m_eWe).uwai().getCache();
	for (PlayerIter<MAJOR_CIV> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		PlayerTypes const ePlayer = itPlayer->getID();
		int const iCurrTotalPop = GET_PLAYER(ePlayer).getTotalPopulation();
		if (iCurrTotalPop <= 0)
			continue;
		scaled rPopIncrease;
		CitySet const& kConq = conqueredCities(ePlayer);
		for (CitySetIter it = kConq.begin(); it != kConq.end(); ++it)
		{
			CvCity const& kCity = kOurCache.lookupCity(*it)->city();
			rPopIncrease += kCity.getPopulation() *
					// Likely to lose population in the short and medium term
					(2 + per100(kCity.getPlot().calculateCulturePercent(ePlayer))) / 3;
		}
		CitySet const& kLost = lostCities(ePlayer);
		for (CitySetIter it = kLost.begin(); it != kLost.end(); ++it)
		{
			rPopIncrease -= kOurCache.lookupCity(*it)->city().getPopulation();
		}
		rPopIncrease /= std::max(1, iCurrTotalPop);
		// This is just a (poor) starting point for predictedMemberGameScore
		playerResult(ePlayer).setGameScore(kGame.getPlayerScore(ePlayer) *
				scaled::max(0, 1 + rPopIncrease));
	}
	// Precompute a table for isWar b/c it's frequently used
	for (PlayerIter<MAJOR_CIV> it1; it1.hasNext(); ++it1)
	{
		PlayerTypes const eFirst = it1->getID();
		for (PlayerIter<MAJOR_CIV> it2; it2.hasNext(); ++it2)
		{
			PlayerTypes const eSecond = it2->getID();
			m_warTable[eFirst][eSecond] = (
					getWarsContinued(eFirst).count(eSecond) > 0 ||
					getWarsDeclaredBy(eFirst).count(eSecond) > 0 ||
					getWarsDeclaredOn(eFirst).count(eSecond) > 0);
		}
	}
}


// <!-- custom: Return only this member's predicted score; Kingmaking adds any team-shared accepted-vassal score once after aggregating all members. See KI#433. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
scaled MilitaryAnalyst::predictedMemberGameScore(PlayerTypes ePlayer) const
{
	FAssertBounds(0, MAX_CIV_PLAYERS, ePlayer);
	int const iCurrScore = GC.getGame().getPlayerScore(ePlayer);
	scaled r = iCurrScore *
			(1 + GET_PLAYER(m_eWe).uwai().estimateDemographicGrowthRate(
			ePlayer, PLAYER_HISTORY_SCORE, turnsSimulated()));
	PlayerResult* pR = m_playerResults[ePlayer];
	if (pR == NULL || pR->getGameScore() < 0)
		return r;
	if (isEliminated(ePlayer))
		return 0;
	scaled rFromSim = pR->getGameScore();
	if (rFromSim < iCurrScore)
		r = (r + rFromSim) / 2;
	else r.increaseTo(rFromSim);
	if (hasCapitulated(TEAMID(ePlayer)))
		r /= 2;
	return r;
}


bool MilitaryAnalyst::isWar(TeamTypes eFirst, TeamTypes eSecond) const
{
	return isWar(GET_TEAM(eFirst).getLeaderID(), GET_TEAM(eSecond).getLeaderID());
}


bool MilitaryAnalyst::hasCapitulated(TeamTypes eTeam) const
{
	for (MemberIter it(eTeam); it.hasNext(); ++it)
	{
		PlayerTypes const ePlayer = it->getID();
		InvasionGraph::Node const* pNode = m_pInvGraph->getNode(ePlayer);
		if (pNode != NULL && pNode->hasCapitulated())
		{
			FAssert(!GET_TEAM(eTeam).isAVassal());
			return true;
		}
	}
	return false;
}


bool MilitaryAnalyst::isEliminated(PlayerTypes ePlayer) const
{
	InvasionGraph::Node const* pNode = m_pInvGraph->getNode(ePlayer);
	if (pNode == NULL)
		return false;
	CitySet lost;
	pNode->getCityLosses(lost);
	return (((int)lost.size()) == GET_PLAYER(ePlayer).getNumCities());
}


scaled MilitaryAnalyst::lostPower(PlayerTypes ePlayer, MilitaryBranchTypes eBranch) const
{
	InvasionGraph::Node const* pNode = m_pInvGraph->getNode(ePlayer);
	if (pNode == NULL)
		return 0;
	return pNode->getLostPower(eBranch);
}


scaled MilitaryAnalyst::gainedPower(PlayerTypes ePlayer, MilitaryBranchTypes eBranch) const
{
	InvasionGraph::Node const* pNode = m_pInvGraph->getNode(ePlayer);
	if (pNode == NULL)
		return 0;
	return pNode->getGainedPower(eBranch);
}


scaled MilitaryAnalyst::militaryProduction(PlayerTypes ePlayer) const
{
	InvasionGraph::Node const* pNode = m_pInvGraph->getNode(ePlayer);
	if (pNode == NULL)
		return 0;
	return pNode->getProductionInvested();
}


void MilitaryAnalyst::logResults(PlayerTypes ePlayer)
{
	FAssert(gUWAIMilitaryAnalystLogLevel >= 1 && !m_kLogMuteState.isMuted());
	// Not the best way to identify civs that weren't part of the simulation ...
	if (militaryProduction(ePlayer).uround() == 0)
		return;
	if (gUWAIMilitaryAnalystLogLevel >= 2)
	{
		logBBAI("UWAI_MILITARY_ANALYSIS_PLAYER_RESULT turn=%d agentPlayer=%d targetTeam=%d scenario=%s player=%d investedProduction=%d",
				GC.getGame().getGameTurn(), m_eWe, m_eTarget, m_bPeaceScenario ? "PEACE" : "WAR", ePlayer, militaryProduction(ePlayer).uround());
		logCities(ePlayer, true);
		logCities(ePlayer, false);
		logPower(ePlayer, false);
		logPower(ePlayer, true);
	}
	logCapitulations(ePlayer);
	logDoW(ePlayer);
}


void MilitaryAnalyst::logCities(PlayerTypes ePlayer, bool bConquests)
{
	FAssert(gUWAIMilitaryAnalystLogLevel >= 2 && !m_kLogMuteState.isMuted());
	PlayerResult const* pResult = m_playerResults[ePlayer];
	if (pResult == NULL)
		return;
	CitySet const& kCities = (bConquests ? pResult->getConqueredCities() :
			pResult->getLostCities());
	if (kCities.empty())
		return;
	for (CitySetIter it = kCities.begin(); it != kCities.end(); ++it)
		logBBAI("UWAI_MILITARY_ANALYSIS_CITY turn=%d agentPlayer=%d targetTeam=%d scenario=%s player=%d result=%s city=%S",
				GC.getGame().getGameTurn(), m_eWe, m_eTarget, m_bPeaceScenario ? "PEACE" : "WAR", ePlayer,
				bConquests ? "CONQUERED" : "LOST", (UWAICache::cvCityById(*it)).getName().GetCString());
}


void MilitaryAnalyst::logCapitulations(PlayerTypes ePlayer)
{
	FAssert(gUWAIMilitaryAnalystLogLevel >= 1 && !m_kLogMuteState.isMuted());
	if (isEliminated(ePlayer))
	{
		logBBAI("UWAI_MILITARY_ANALYSIS_PLAYER_ELIMINATED turn=%d agentPlayer=%d targetTeam=%d scenario=%s player=%d",
			GC.getGame().getGameTurn(), m_eWe, m_eTarget, m_bPeaceScenario ? "PEACE" : "WAR", ePlayer);
		return;
	}
	TeamTypes const eTeam = TEAMID(ePlayer);
	if (hasCapitulated(eTeam))
	{
		logBBAI("UWAI_MILITARY_ANALYSIS_TEAM_CAPITULATED turn=%d agentPlayer=%d targetTeam=%d scenario=%s player=%d team=%d",
			GC.getGame().getGameTurn(), m_eWe, m_eTarget, m_bPeaceScenario ? "PEACE" : "WAR", ePlayer, eTeam);
		return;
	}
	TeamSet const& kCaps = m_capitulationsAcceptedPerTeam[eTeam];
	if (kCaps.empty())
		return;
	for (TeamSetIter it = kCaps.begin(); it != kCaps.end(); ++it)
	{
		logBBAI("UWAI_MILITARY_ANALYSIS_CAPITULATION_ACCEPTED turn=%d agentPlayer=%d targetTeam=%d scenario=%s accepterTeam=%d surrenderedTeam=%d",
				GC.getGame().getGameTurn(), m_eWe, m_eTarget, m_bPeaceScenario ? "PEACE" : "WAR", eTeam, *it);
		// The team name (e.g. Team1) would not be helpful
		// <!-- custom: Structured team IDs identify the capitulation; level-2 player rows retain the member names previously printed by the report. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
		if (gUWAIMilitaryAnalystLogLevel >= 2)
		{
			for (MemberIter itMember(*it); itMember.hasNext(); ++itMember)
				logBBAI("UWAI_MILITARY_ANALYSIS_CAPITULATION_MEMBER turn=%d surrenderedTeam=%d player=%d name=%S",
					GC.getGame().getGameTurn(), *it, itMember->getID(), GET_PLAYER(itMember->getID()).getName(0));
		}
	}
}


void MilitaryAnalyst::logDoW(PlayerTypes ePlayer)
{
	FAssert(gUWAIMilitaryAnalystLogLevel >= 1 && !m_kLogMuteState.isMuted());
	PlayerResult const* pResult = m_playerResults[ePlayer];
	if (pResult == NULL)
		return;
	PlyrSet const& DoWBy = pResult->getDoWBy();
	if (!DoWBy.empty())
	{
		logBBAI("UWAI_MILITARY_ANALYSIS_DOW_BY turn=%d agentPlayer=%d targetTeam=%d scenario=%s player=%d count=%d",
			GC.getGame().getGameTurn(), m_eWe, m_eTarget, m_bPeaceScenario ? "PEACE" : "WAR", ePlayer, (int)DoWBy.size());
		if (gUWAIMilitaryAnalystLogLevel >= 2)
		{
			for (PlyrSetIter it = DoWBy.begin(); it != DoWBy.end(); ++it)
				logBBAI("UWAI_MILITARY_ANALYSIS_DOW_BY_TARGET turn=%d player=%d otherPlayer=%d otherName=%S", GC.getGame().getGameTurn(), ePlayer, *it, GET_PLAYER(*it).getName(0));
		}
	}
	PlyrSet const& DoWOn = pResult->getDoWOn();
	if (!DoWOn.empty())
	{
		logBBAI("UWAI_MILITARY_ANALYSIS_DOW_ON turn=%d agentPlayer=%d targetTeam=%d scenario=%s player=%d count=%d",
			GC.getGame().getGameTurn(), m_eWe, m_eTarget, m_bPeaceScenario ? "PEACE" : "WAR", ePlayer, (int)DoWOn.size());
		if (gUWAIMilitaryAnalystLogLevel >= 2)
		{
			for (PlyrSetIter it = DoWOn.begin(); it != DoWOn.end(); ++it)
				logBBAI("UWAI_MILITARY_ANALYSIS_DOW_ON_SOURCE turn=%d player=%d otherPlayer=%d otherName=%S", GC.getGame().getGameTurn(), ePlayer, *it, GET_PLAYER(*it).getName(0));
		}
	}
	PlyrSet const& kWarsCont = pResult->getWarsContinued();
	if (!kWarsCont.empty())
	{
		logBBAI("UWAI_MILITARY_ANALYSIS_WAR_CONTINUED turn=%d agentPlayer=%d targetTeam=%d scenario=%s player=%d count=%d",
			GC.getGame().getGameTurn(), m_eWe, m_eTarget, m_bPeaceScenario ? "PEACE" : "WAR", ePlayer, (int)kWarsCont.size());
		if (gUWAIMilitaryAnalystLogLevel >= 2)
		{
			for (PlyrSetIter it = kWarsCont.begin(); it != kWarsCont.end(); ++it)
				logBBAI("UWAI_MILITARY_ANALYSIS_WAR_CONTINUED_OPPONENT turn=%d player=%d otherPlayer=%d otherName=%S",
					GC.getGame().getGameTurn(), ePlayer, *it, GET_PLAYER(*it).getName(0));
		}
	}
}


void MilitaryAnalyst::logPower(PlayerTypes ePlayer, bool bGained)
{
	FAssert(gUWAIMilitaryAnalystLogLevel >= 2 && !m_kLogMuteState.isMuted());
	// Some overlap with InvasionGraph::Node::logPower
	// <!-- custom: Preserve the inherited distinction: the first measure is net build-up minus losses, while the second is power lost to casualties.
	// A generic GAIN label obscured that the net value can be negative. (GPT-6.1-Sol) -->
	char const* const szChange = (bGained ? "NET_BUILDUP_MINUS_LOSSES" : "CASUALTY_LOSS");
	int iLogged = 0;
	for (int i = 0; i < NUM_BRANCHES; i++)
	{
		MilitaryBranchTypes eBranch = (MilitaryBranchTypes)i;
		int iPowChange = (bGained ?
			gainedPower(ePlayer, eBranch) - lostPower(ePlayer, eBranch) :
			lostPower(ePlayer, eBranch)).round();
		if (iPowChange == 0)
			continue;
		logBBAI("UWAI_MILITARY_ANALYSIS_POWER_CHANGE turn=%d agentPlayer=%d targetTeam=%d scenario=%s player=%d change=%s branch=%s power=%d",
				GC.getGame().getGameTurn(), m_eWe, m_eTarget, m_bPeaceScenario ? "PEACE" : "WAR", ePlayer, szChange, MilitaryBranch::str(eBranch), iPowChange);
		iLogged++;
	}
	if (iLogged == 0) logBBAI("UWAI_MILITARY_ANALYSIS_POWER_CHANGE_NONE turn=%d agentPlayer=%d targetTeam=%d scenario=%s player=%d change=%s",
			GC.getGame().getGameTurn(), m_eWe, m_eTarget, m_bPeaceScenario ? "PEACE" : "WAR", ePlayer, szChange);
}
