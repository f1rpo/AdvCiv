// advc.104: New class; see MilitaryAnalyst.h for description.

#include "CvGameCoreDLL.h"
#include "MilitaryAnalyst.h"
#include "WarEvalParameters.h"
#include "UWAIAgent.h"
#include "InvasionGraph.h"
#include "CoreAI.h"
#include "CvCity.h"
#include "CvInfo_GameOption.h"

using std::ostringstream;


// empty sets (static)
CitySet MilitaryAnalyst::emptyCitySet;
PlyrSet MilitaryAnalyst::emptyPlayerSet;


MilitaryAnalyst::MilitaryAnalyst(PlayerTypes weId, WarEvalParameters& warEvalParams,
		bool peaceScenario)
	: weId(weId), warEvalParams(warEvalParams), theyId(warEvalParams.targetId()),
	  peaceScenario(peaceScenario), report(warEvalParams.getReport()), turnsSim(0) {

	PROFILE_FUNC();
	playerResults.resize(MAX_CIV_PLAYERS, NULL);
	warTable.resize(MAX_CIV_PLAYERS, std::vector<bool>(MAX_CIV_PLAYERS, false));
	nukedCities.resize(MAX_CIV_PLAYERS, std::vector<double>(MAX_CIV_PLAYERS, 0.0));
	capitulationsAcceptedPerTeam.resize(MAX_CIV_TEAMS);
	report.log("Military analysis from the pov of %s", report.leaderName(weId));
	CvTeamAI& agent = GET_TEAM(weId);
	PlyrSet currentlyAtWar; // 'atWar' is already a name of a global function
	PlyrSet ourFutureOpponents;
	PlyrSet ourSide;
	PlyrSet theirSide;
	PlyrSet weAndOurVassals;
	PlyrSet ourAllies;
	PlyrSet theyAndTheirVassals;
	// Exclude unknown civs from analysis
	for(PlayerIter<MAJOR_CIV,KNOWN_TO> it(agent.getID()); it.hasNext(); ++it) {
		PlayerTypes civId = it->getID();
		/*  Civs already at war (not necessarily with us).
			If we are at war, our vassals are covered here as well. */
		if(GET_TEAM(civId).uwai().isKnownToBeAtWar(agent.getID()))
			currentlyAtWar.insert(civId);
		TeamTypes masterTeamId = GET_PLAYER(civId).getMasterTeam();
		if(masterTeamId == GET_PLAYER(weId).getMasterTeam())
			weAndOurVassals.insert(civId);
		if(masterTeamId == GET_TEAM(theyId).getMasterTeam())
			theyAndTheirVassals.insert(civId);
		if(warEvalParams.isWarAlly(GET_TEAM(civId).getMasterTeam()))
			ourAllies.insert(civId);
		if(warEvalParams.isExtraTarget(GET_TEAM(civId).getMasterTeam()))
			theirSide.insert(civId);
		// Civs soon to be at war with us or an ally that we bring in
		if(doWePlanToDeclWar(civId) || (warEvalParams.isAnyWarAlly() &&
				agent.isAtWar(TEAMID(civId)))) {
			// doWePlanToDeclWar checks if civId is part of the enemy team
			ourFutureOpponents.insert(civId);
			theirSide.insert(civId);
			// Defensive pacts that our war plans will trigger
			for(PlayerIter<MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> allyIt(agent.getID());
					allyIt.hasNext(); ++allyIt) {
				CvPlayerAI& ally = *allyIt;
				// Can happen b/c of the kekm.3 change
				if(agent.isAtWar(ally.getTeam()))
					continue;
				if(GET_TEAM(civId).uwai().hasDefactoDefensivePact(ally.getTeam()))
					ourFutureOpponents.insert(ally.getID());
			}
		}
	}
	ourSide.insert(weAndOurVassals.begin(), weAndOurVassals.end());
	ourSide.insert(ourAllies.begin(), ourAllies.end());
	theirSide.insert(theyAndTheirVassals.begin(), theyAndTheirVassals.end());
	bool const noWarVsExtra = (peaceScenario && warEvalParams.isNoWarVsExtra());
	PlyrSet& declaringWar = ((agent.isAtWar(theyId) &&
			!warEvalParams.isConsideringPeace()) ? ourAllies : ourSide);
	if((!peaceScenario && !warEvalParams.isConsideringPeace()) || noWarVsExtra) {
		// Store war-scenario DoW for getWarsDeclaredOn/By
		for(PlyrSetIter it1 = declaringWar.begin();
				it1 != declaringWar.end(); ++it1) {
			for(PlyrSetIter it2 = theirSide.begin();
					it2 != theirSide.end(); ++it2) {
				if(!GET_TEAM(*it1).isAtWar(TEAMID(*it2))) {
					playerResult(*it1).setDoWBy(*it2);
					playerResult(*it2).setDoWOn(*it1);
				}
			}
		}
		PlyrSet theirDPAllies;
		for(PlayerIter<MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> it(agent.getID());
				it.hasNext(); ++it) {
			PlayerTypes civId = it->getID();
			if(agent.isAtWar(TEAMID(civId)))
				continue;
			if(isOnTheirSide(TEAMID(civId), true) && theirSide.count(civId) <= 0)
				theirDPAllies.insert(civId);
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
		for(PlyrSetIter it = theirDPAllies.begin(); it != theirDPAllies.end(); ++it)
			playerResult(*it).setDoWBy(declaringWar.begin(), declaringWar.end());
		for(PlyrSetIter it = declaringWar.begin(); it != declaringWar.end(); ++it)
			playerResult(*it).setDoWOn(theirDPAllies.begin(), theirDPAllies.end());
	}
	// Wars continued
	for(PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it) {
		PlayerTypes civId = it->getID();
		for(PlayerIter<MAJOR_CIV,ENEMY_OF> enemyIt(TEAMID(civId));
				enemyIt.hasNext(); ++enemyIt) {
			PlayerTypes enemyId = enemyIt->getID();
			if((declaringWar.count(civId) <= 0 || !peaceScenario ||
					theirSide.count(enemyId) <= 0) &&
					(declaringWar.count(enemyId) <= 0 || !peaceScenario ||
					theirSide.count(civId) <= 0)) {
				playerResult(civId).setWarContinued(enemyId);
				FAssert(getWarsDeclaredBy(civId).count(enemyId) <= 0);
				FAssert(getWarsDeclaredBy(enemyId).count(civId) <= 0);
				FAssert(getWarsDeclaredOn(civId).count(enemyId) <= 0);
				FAssert(getWarsDeclaredOn(enemyId).count(civId) <= 0);
			}
		}
	}
	// Need ArmamentForecasts for initially uninvolved parties (if any)
	PlyrSet uninvolved;
	uninvolved.insert(declaringWar.begin(), declaringWar.end());
	uninvolved.insert(ourFutureOpponents.begin(), ourFutureOpponents.end());
	partOfAnalysis.insert(currentlyAtWar.begin(), currentlyAtWar.end());
	partOfAnalysis.insert(uninvolved.begin(), uninvolved.end());
	ig = new InvasionGraph(*this, currentlyAtWar, peaceScenario);
	ig->addUninvolvedParties(uninvolved);
	/*  Simulation in two phases: The first phase lasts until we have finished
		our war preparations (NB: we may already be in a war while preparing
		for another); the second phase lasts for a set number of turns. */
	int prepTime = warEvalParams.getPreparationTime();
	/*  Above, uninvolved parties are added and preparation time is set
		regardless of peace scenario; otherwise, the results for war and peace
		scenario wouldn't be fully comparable. */
	if(peaceScenario) {
		FAssert(!agent.isAtWar(theyId) || !GET_TEAM(theyId).isAVassal()); /*
				Master isn't included in the removed war opponents
				(theirSide) then. */
		if(noWarVsExtra) { // diff = theirSide minus theyAndTheirVassals
			PlyrSet diff;
			std::set_difference(theirSide.begin(), theirSide.end(),
					theyAndTheirVassals.begin(), theyAndTheirVassals.end(),
					std::inserter(diff, diff.begin()));
			ig->removeWar(declaringWar, diff);
		}
		/*	Capitulation: Not fully implemented; misses the wars that we need
			to declare on the enemies of theyId. */
		else if(warEvalParams.getCapitulationTeam() == theyId)
			ig->removeWar(declaringWar, currentlyAtWar);
		else ig->removeWar(declaringWar, theirSide);
	}
	int timeHorizon = 25;
	// Look a bit farther into the future when in a total war
	if(prepTime <= 0 && warEvalParams.isTotal())
		timeHorizon += 5;
	// Look a bit farther on Marathon speed
	if(GC.getInfo(GC.getGame().getGameSpeedType()).
			getGoldenAgePercent() >= 150)
		timeHorizon += 5;
	/*  Skip phase 1 if it would be short (InvasionGraph::Node::isSneakAttack
		will still read the actual prep time from the WarEvalParameters) */
	if(prepTime < 4) {
		if(prepTime > 0)
			report.log("Skipping short prep. time (%d turns):", prepTime);
		timeHorizon += prepTime; // Prolong 2nd phase instead
		prepTime = 0;
	}
	else {
		report.log("Phase 1%s%s%s (%d turns)",
				peaceScenario ? "" : ": Prolog of simulation; ",
				peaceScenario ? "" : report.leaderName(weId),
				peaceScenario ? "": " is preparing war",
				prepTime);
		ig->simulate(prepTime);
		turnsSim += prepTime;
	}
	if(!peaceScenario || noWarVsExtra)
		ig->addFutureWarParties(declaringWar, ourFutureOpponents);
	/*  Force update of targets (needed even if no parties were added b/c of
		defeats in phase I. */
	if(peaceScenario)
		ig->updateTargets();
	report.log("Phase 2%s%s (%d turns)",
			(!peaceScenario && !agent.isAtWar(theyId) ?
			": Simulation assuming DoW by " : ""),
			(!peaceScenario && !agent.isAtWar(theyId) ?
			report.leaderName(weId) : ""), timeHorizon);
	ig->simulate(timeHorizon);
	turnsSim += timeHorizon;
	prepareResults(); // ... of conventional war
	simulateNuclearWar();
}

MilitaryAnalyst::~MilitaryAnalyst() {

	delete ig;
	for(size_t i = 0; i < playerResults.size(); i++)
		SAFE_DELETE(playerResults[i]);
}

MilitaryAnalyst::PlayerResult& MilitaryAnalyst::playerResult(PlayerTypes civId) {

	/*  Lazy creation of playerResults means that NULL needs to be checked when accessing
		a PlayerResult and that memory needs to be allocated dynamically. Based on a test,
		this is still faster than creating a PlayerResult for every player upfront and
		accessing the empty sets of the dummy entries. */
	FAssertBounds(0, MAX_CIV_PLAYERS, civId);
	if(playerResults[civId] == NULL)
		playerResults[civId] = new PlayerResult();
	return *playerResults[civId];
}

bool MilitaryAnalyst::isOnOurSide(TeamTypes tId) const {

	return GET_TEAM(tId).getMasterTeam() == GET_TEAM(weId).getMasterTeam() ||
			warEvalParams.isWarAlly(tId);
}

bool MilitaryAnalyst::isOnTheirSide(TeamTypes tId, bool defensivePacts) const {

	bool r = GET_TEAM(tId).getMasterTeam() == GET_TEAM(warEvalParams.targetId()).
			getMasterTeam() || warEvalParams.isExtraTarget(tId);
	if(!defensivePacts || r)
		return r;
	for(TeamIter<MAJOR_CIV> it; it.hasNext(); ++it) {
		if(GET_TEAM(tId).isDefensivePact(it->getID()) && isOnTheirSide(it->getID()))
			return true;
	}
	return false;
}

bool MilitaryAnalyst::doWePlanToDeclWar(PlayerTypes civId) const {

	CvTeamAI& agent = GET_TEAM(weId);
	TeamTypes teamId = TEAMID(civId);
	/* Ongoing war preparations will be replaced by the war plan under
	   consideration if adopted; disregard those. */
	return (//agent.AI_getWarPlan(teamId) != NO_WARPLAN ||
			teamId == warEvalParams.targetId()) &&
			!agent.isAtWar(teamId);
}

void MilitaryAnalyst::simulateNuclearWar() {

	// Only simulates nukes fired by or on weId
	CvPlayerAI const& we = GET_PLAYER(weId);
	if(isEliminated(weId) || we.getNumCities() <= 0)
		return; // Who cares then
	CvTeamAI const& agent = GET_TEAM(weId);
	UWAICache const& ourCache = we.uwai().getCache();
	/*  Counts the number of nukes. Assume no further build-up of nukes throughout
		the military analysis. (They take long to build.) */
	double ourNukes = ourCache.getPowerValues()[NUCLEAR]->num();
	double ourTargets = 0;
	for(PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it) {
		if(isWar(weId, it->getID()))
			ourTargets += (it->isAVassal() ? 0.5 : 1);
	}
	for(PlayerIter<MAJOR_CIV> enemyIt; enemyIt.hasNext(); ++enemyIt) {
		CvPlayerAI const& enemy = *enemyIt;
		if(!isWar(weId, enemy.getID()))
			continue;
		double enemyNukes = enemy.uwai().getCache().
				getPowerValues()[NUCLEAR]->num();
		if(ourNukes + enemyNukes < 0.5)
			continue;
		// The AI is (much) more willing to fire nukes in total war than in limited war
		WarPlanTypes ourWarplan = agent.AI_getWarPlan(enemy.getTeam());
		WarPlanTypes civWarplan = GET_TEAM(enemy.getTeam()).AI_getWarPlan(agent.getID());
		// If one side goes total, the other will follow suit
		bool isTotal = false;
		if(ourWarplan == WARPLAN_PREPARING_TOTAL || ourWarplan == WARPLAN_TOTAL ||
				civWarplan == WARPLAN_PREPARING_TOTAL || civWarplan == WARPLAN_TOTAL)
			isTotal = true;
		// (Human: let's trust the war plan type of the human proxy AI)
		if(warEvalParams.targetId() == enemy.getTeam())
			isTotal = warEvalParams.isTotal();
		double portionFired = isTotal ? 0.75 : 0.25;
		// Assume that vassals are hit by fewer nukes, and fire fewer nukes.
		double vassalFactorWe = 1;
		if(agent.isAVassal())
			vassalFactorWe = 0.5;
		double vassalFactorCiv = 1;
		if(agent.isAVassal())
			vassalFactorCiv = 0.5;
		double enemyTargets = 0;
		for(PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it) {
			CvPlayer const& enemyOfEnemy = *it;
			if(isWar(enemyOfEnemy.getID(), enemy.getID()))
				enemyTargets += (enemyOfEnemy.isAVassal() ? 0.5 : 1);
		}
		double firedOnUs = portionFired * vassalFactorWe * vassalFactorCiv *
				/*  sqrt for pessimism - we may get hit worse than the
					average target, and that worries us. */
				enemyNukes / std::sqrt((double)enemyTargets);
		firedOnUs = std::min(firedOnUs, 1.5 * we.getNumCities());
		double firedByUs = 0;
		if(ourTargets > 0) {
			firedByUs = portionFired * vassalFactorWe * vassalFactorCiv * ourNukes *
					/*  Apparently short for "Our words are backed with(/by)
						nuclear weapons" */
					(we.AI_isDoStrategy(AI_STRATEGY_OWABWNW) ? 1.1 : 0.9) /
					ourTargets;
		}
		firedByUs = std::min(firedByUs, 1.5 * enemy.getNumCities());
		double hitUs = firedOnUs * interceptionMultiplier(agent.getID());
		double hitCiv = firedByUs * interceptionMultiplier(enemy.getTeam());
		// Make sure not to allocate PlayerResult unnecessarily
		if(hitUs > 0.01) {
			playerResult(weId).addNukesSuffered(hitUs);
			nukedCities[enemy.getID()][weId] += hitUs;
		}
		if(hitCiv > 0.01) {
			playerResult(enemy.getID()).addNukesSuffered(hitCiv);
			nukedCities[weId][enemy.getID()] += hitCiv;
		}
		if(firedByUs > 0.01)
			playerResult(weId).addNukesFired(firedByUs);
		if(firedOnUs > 0.01)
			playerResult(enemy.getID()).addNukesFired(firedOnUs);
	}
}

double MilitaryAnalyst::interceptionMultiplier(TeamTypes tId) {

	int nukeInterception = std::max(GET_TEAM(tId).getNukeInterception(),
			// advc.143b:
			GET_TEAM(GET_TEAM(tId).getMasterTeam()).getNukeInterception());
	double const evasionPr = 0.5; // Fixme: Shouldn't hardcode this
	// Percentage of Tactical Nukes
	double tactRatio = (GET_TEAM(tId).isHuman() ? 0.5 : 0.33);
	return ::dRange(100 -
			(nukeInterception * ((1 - tactRatio) + evasionPr * tactRatio)),
			0.0, 100.0) / 100.0;
}

void MilitaryAnalyst::prepareResults() {

	PROFILE_FUNC();
	for(PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it) {
		PlayerTypes civId = it->getID();
		InvasionGraph::Node* node = ig->getNode(civId);
		if(node == NULL)
			continue;
		// Allocate PlayerResult only if necessary
		if(node->anyConquests())
			node->getConquests(playerResult(civId).getConqueredCities());
		if(node->anyLosses())
			node->getLosses(playerResult(civId).getLostCities());
		node->getCapitulationsAccepted(capitulationsAcceptedPerTeam[civId]);
	}
	// Predict scores as current game score modified based on gained/ lost population
	CvGame& g = GC.getGame();
	UWAICache const& ourCache = GET_PLAYER(weId).uwai().getCache();
	for(PlayerIter<MAJOR_CIV> itCiv; itCiv.hasNext(); ++itCiv) {
		PlayerTypes const civId = itCiv->getID();
		double popIncr = 0;
		CitySet const& conq = conqueredCities(civId);
		for(CitySetIter it = conq.begin(); it != conq.end(); ++it) {
			UWAICache::City* c = ourCache.lookupCity(*it);
			if(c == NULL) continue;
			popIncr += c->city().getPopulation();
		}
		CitySet const& lost = lostCities(civId);
		for(CitySetIter it = lost.begin(); it != lost.end(); ++it) {
			UWAICache::City* c = ourCache.lookupCity(*it);
			if(c == NULL) continue;
			popIncr -= c->city().getPopulation();
		}
		popIncr /= std::max(1, GET_PLAYER(civId).getTotalPopulation());
		if(popIncr > 0.01) {
			/*  Only apply half of the relative increase b/c game score is only partially
				determined by population */
			playerResult(civId).setGameScore(g.getPlayerScore(civId) * (1 + popIncr / 2));
		}
	}
	// Precompute a table for isWar b/c it's frequently used
	for(PlayerIter<MAJOR_CIV> it1; it1.hasNext(); ++it1) {
		PlayerTypes p1 = it1->getID();
		for(PlayerIter<MAJOR_CIV> it2; it2.hasNext(); ++it2) {
			PlayerTypes p2 = it2->getID();
			warTable[p1][p2] = (getWarsContinued(p1).count(p2) > 0 ||
					getWarsDeclaredBy(p1).count(p2) > 0 ||
					getWarsDeclaredOn(p1).count(p2) > 0);
		}
	}
}

double MilitaryAnalyst::predictedGameScore(PlayerTypes civId) const {

	FAssertBounds(0, MAX_CIV_PLAYERS, civId);
	PlayerResult* r = playerResults[civId];
	return (r == NULL || r->getGameScore() < 0 ?
			GC.getGame().getPlayerScore(civId) : r->getGameScore());
}

bool MilitaryAnalyst::isWar(TeamTypes t1, TeamTypes t2) const {

	return isWar(GET_TEAM(t1).getLeaderID(), GET_TEAM(t2).getLeaderID());
}

bool MilitaryAnalyst::hasCapitulated(TeamTypes teamId) const {

	FAssert(teamId != BARBARIAN_TEAM);
	for(MemberIter it(teamId); it.hasNext(); ++it) {
		PlayerTypes civId = it->getID();
		InvasionGraph::Node* node = ig->getNode(civId);
		if(node != NULL && node->hasCapitulated()) {
			FAssert(!GET_TEAM(teamId).isAVassal());
			return true;
		}
	}
	return false;
}

bool MilitaryAnalyst::isEliminated(PlayerTypes civId) const {

	InvasionGraph::Node* node = ig->getNode(civId);
	if(node == NULL)
		return false;
	CitySet r;
	node->getLosses(r);
	return (((int)r.size()) == GET_PLAYER(civId).getNumCities());
}

double MilitaryAnalyst::lostPower(PlayerTypes civId, MilitaryBranchTypes mb) const {

	InvasionGraph::Node* node = ig->getNode(civId);
	if(node == NULL)
		return 0;
	return node->getLostPower(mb);
}

double MilitaryAnalyst::gainedPower(PlayerTypes civId,
		MilitaryBranchTypes mb) const {

	InvasionGraph::Node* node = ig->getNode(civId);
	if(node == NULL)
		return 0;
	return node->getGainedPower(mb);
}

double MilitaryAnalyst::militaryProduction(PlayerTypes civId) const {

	InvasionGraph::Node* node = ig->getNode(civId);
	if(node == NULL)
		return 0;
	return node->getProductionInvested();
}

void MilitaryAnalyst::logResults(PlayerTypes civId) {

	if(report.isMute())
		return;
	// Not the best way to identify civs that weren't part of the simulation ...
	if(::round(militaryProduction(civId)) == 0)
		return;
	if(civId < 0 || civId > MAX_CIV_PLAYERS) {
		FErrorMsg("civId out of bounds");
		return;
	}
	report.log("Results about %s", report.leaderName(civId));
	report.log("\nbq.");
	logCapitulations(civId);
	logCities(civId, true);
	logCities(civId, false);
	logPower(civId, false);
	logPower(civId, true);
	report.log("Invested production: %d", ::round(militaryProduction(civId)));
	logDoW(civId);
	report.log("");
}

void MilitaryAnalyst::logCities(PlayerTypes civId, bool conquests) {

	PlayerResult* result = playerResults[civId];
	if(result == NULL)
		return;
	CitySet const& cities = (conquests ? result->getConqueredCities() : result->getLostCities());
	if(cities.empty())
		return;
	report.log("Cities %s:", conquests ? "conquered" : "lost");
	for(CitySetIter it = cities.begin(); it != cities.end(); ++it) {
		CvCity& c = UWAICache::cvCityById(*it);
		report.log("%s", report.cityName(c));
	}
}

void MilitaryAnalyst::logCapitulations(PlayerTypes civId) {

	if(isEliminated(civId)) {
		report.log("Eliminated");
		return;
	}
	TeamTypes teamId = TEAMID(civId);
	if(hasCapitulated(teamId)) {
		report.log("Team has capitulated");
		return;
	}
	TeamSet const& caps = capitulationsAcceptedPerTeam[teamId];
	if(caps.empty())
		return;
	report.log("Capitulation accepted from:");
	for(TeamSetIter it = caps.begin(); it != caps.end(); ++it) {
		// The team name (e.g. Team1) would not be helpful
		for(MemberIter memberIt(*it); memberIt.hasNext(); ++memberIt)
			report.log("%s", report.leaderName(memberIt->getID()));
	}
}

void MilitaryAnalyst::logDoW(PlayerTypes civId) {

	PlayerResult* result = playerResults[civId];
	if(result == NULL)
		return;
	PlyrSet const& DoWBy = result->getDoWBy();
	if(!DoWBy.empty()) {
		report.log("Wars declared by %s:",
				report.leaderName(civId));
		for(PlyrSetIter it = DoWBy.begin(); it != DoWBy.end(); ++it)
			report.log("%s", report.leaderName(*it));
	}
	PlyrSet const& DoWOn = result->getDoWOn();
	if(!DoWOn.empty()) {
		report.log("Wars declared on %s:",
				report.leaderName(civId));
		for(PlyrSetIter it = DoWOn.begin(); it != DoWOn.end(); ++it)
			report.log("%s", report.leaderName(*it));
	}
	PlyrSet const& warsCont = result->getWarsContinued();
	if(!warsCont.empty()) {
		report.log("Wars continued:");
		for(PlyrSetIter it = warsCont.begin(); it != warsCont.end(); ++it)
			report.log("%s", report.leaderName(*it));
	}
}

void MilitaryAnalyst::logPower(PlayerTypes civId, bool gained) {

	// Some overlap with InvasionGraph::Node::logPower
	ostringstream out;
	if(gained)
		out << "Net power gain (build-up minus losses): ";
	else out << "Lost power from casualties: ";
	int nLogged = 0;
	for(int i = 0; i < NUM_BRANCHES; i++) {
		MilitaryBranchTypes mb = (MilitaryBranchTypes)i;
		double pow = gained ?
				gainedPower(civId, mb) - lostPower(civId, mb) :
				lostPower(civId, mb);
		int powRounded = ::round(pow);
		if(powRounded == 0)
			continue;
		if(nLogged > 0)
			out << ", ";
		out << MilitaryBranch::str(mb) << " " << powRounded;
		nLogged++;
	}
	if(nLogged == 0)
		out << "none";
	report.log("%s", out.str().c_str());
}
