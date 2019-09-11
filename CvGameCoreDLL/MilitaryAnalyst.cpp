// <advc.104> New class; see MilitaryAnalyst.h for description

#include "CvGameCoreDLL.h"
#include "MilitaryAnalyst.h"
#include "WarEvalParameters.h"
#include "WarAndPeaceAgent.h"
#include "InvasionGraph.h"
#include "CvGamePlay.h"
#include "CvGameAI.h"
#include "BBAI_Defines.h"
#include "CvInfo_GameOption.h"
#include <sstream>
#include <iterator>

using std::set;
using std::ostringstream;


MilitaryAnalyst::MilitaryAnalyst(PlayerTypes weId, WarEvalParameters& warEvalParams,
		bool peaceScenario)
	: weId(weId), warEvalParams(warEvalParams), theyId(warEvalParams.targetId()),
	  peaceScenario(peaceScenario), report(warEvalParams.getReport()), turnsSim(0) {

	PROFILE_FUNC();
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		gameScore[i] = -1;
		nukesFiredBy[i] = 0;
		nukesSufferedBy[i] = 0;
		lostCitiesPerCiv[i] = new set<int>();
		conqueredCitiesPerCiv[i] = new set<int>();
		DoWOn[i] = new set<PlayerTypes>();
		DoWBy[i] = new set<PlayerTypes>();
		warsCont[i] = new set<PlayerTypes>();
		for(int j = 0; j < MAX_CIV_PLAYERS; j++) {
			isWarTable[i][j] = false;
			nukedCities[i][j] = 0;
		}
	}
	for(int i = 0; i < MAX_CIV_TEAMS; i++)
		capitulationsAcceptedPerTeam[i] = new set<TeamTypes>();
	report.log("Military analysis from the pov of %s", report.leaderName(weId));
	CvTeamAI& agent = TEAMREF(weId);
	set<PlayerTypes> currentlyAtWar; // 'atWar' is already a name of a global-context function
	set<PlayerTypes> ourFutureOpponents;
	set<PlayerTypes> ourSide;
	set<PlayerTypes> theirSide;
	set<PlayerTypes> weAndOurVassals;
	set<PlayerTypes> ourAllies;
	set<PlayerTypes> theyAndTheirVassals;
	for(size_t i = 0; i < getWPAI.properCivs().size(); i++) {
		PlayerTypes civId = getWPAI.properCivs()[i];
		// Exclude unknown civs from analysis
		if(!agent.isHasMet(TEAMID(civId)))
			continue;
		/*  Civs already at war (not necessarily with us).
			If we are at war, our vassals are covered here as well. */
		if(TEAMREF(civId).warAndPeaceAI().isKnownToBeAtWar(agent.getID()))
			currentlyAtWar.insert(civId);
		TeamTypes masterTeamId = GET_PLAYER(civId).getMasterTeam();
		if(masterTeamId == GET_PLAYER(weId).getMasterTeam())
			weAndOurVassals.insert(civId);
		if(masterTeamId == GET_TEAM(theyId).getMasterTeam())
			theyAndTheirVassals.insert(civId);
		if(warEvalParams.isWarAlly(TEAMREF(civId).getMasterTeam()))
			ourAllies.insert(civId);
		if(warEvalParams.isExtraTarget(TEAMREF(civId).getMasterTeam()))
			theirSide.insert(civId);
		// Civs soon to be at war with us or an ally that we bring in
		if(doWePlanToDeclWar(civId) || (warEvalParams.isAnyWarAlly() &&
				agent.isAtWar(TEAMID(civId)))) {
			// doWePlanToDeclWar checks if civId is part of the enemy team
			ourFutureOpponents.insert(civId);
			theirSide.insert(civId);
			// Defensive pacts that our war plans will trigger.
			for(size_t j = 0; j < getWPAI.properCivs().size(); j++) {
				CvTeamAI& ally = TEAMREF(getWPAI.properCivs()[j]);
				/* If we attack our own ally, we still won't be at war with
				   ourselves. */
				if(ally.getMasterTeam() == TEAMREF(weId).getMasterTeam())
					continue;
				// Can happen b/c of the dlph.3 change
				if(ally.isAtWar(TEAMID(weId)))
					continue;
				if(TEAMREF(civId).warAndPeaceAI().hasDefactoDefensivePact(
						ally.getID()))
					ourFutureOpponents.insert(getWPAI.properCivs()[j]);
			}
		}
	}
	ourSide.insert(weAndOurVassals.begin(), weAndOurVassals.end());
	ourSide.insert(ourAllies.begin(), ourAllies.end());
	theirSide.insert(theyAndTheirVassals.begin(), theyAndTheirVassals.end());
	bool const noWarVsExtra = (peaceScenario && warEvalParams.isNoWarVsExtra());
	set<PlayerTypes>& declaringWar = ((agent.isAtWar(theyId) &&
			!warEvalParams.isConsideringPeace()) ? ourAllies : ourSide);
	if((!peaceScenario && !warEvalParams.isConsideringPeace()) || noWarVsExtra) {
		// Store war-scenario DoW for getWarsDeclaredOn/By
		for(set<PlayerTypes>::iterator it = declaringWar.begin();
				it != declaringWar.end(); it++) {
			for(set<PlayerTypes>::iterator it2 = theirSide.begin();
					it2 != theirSide.end(); it2++) {
				if(!TEAMREF(*it).isAtWar(TEAMID(*it2))) {
					DoWBy[*it]->insert(*it2);
					DoWOn[*it2]->insert(*it);
				}
			}
		}
		set<PlayerTypes> theirDPAllies;
		for(size_t i = 0; i < getWPAI.properCivs().size(); i++) {
			PlayerTypes civId = getWPAI.properCivs()[i];
			if(TEAMREF(civId).getMasterTeam() == TEAMREF(weId).getMasterTeam())
				continue;
			if(TEAMREF(civId).isAtWar(TEAMID(weId)))
				continue;
			if(isOnTheirSide(TEAMID(civId), true) && theirSide.count(civId) <= 0)
				theirDPAllies.insert(civId);
		}
		{ // Just debugging
			set<PlayerTypes> isect;
			set_intersection(theirSide.begin(), theirSide.end(),
					theirDPAllies.begin(), theirDPAllies.end(), std::inserter(
					 isect, isect.begin()));
			FAssert(isect.empty());
		}
		for(set<PlayerTypes>::iterator it = theirDPAllies.begin();
				it != theirDPAllies.end(); it++)
			DoWBy[*it]->insert(declaringWar.begin(), declaringWar.end());
		for(set<PlayerTypes>::iterator it = declaringWar.begin();
				it != declaringWar.end(); it++)
			DoWOn[*it]->insert(theirDPAllies.begin(), theirDPAllies.end());
	}
	// Wars continued
	for(size_t i = 0; i < getWPAI.properCivs().size(); i++) {
		PlayerTypes civId = getWPAI.properCivs()[i];
		for(size_t j = 0; j < getWPAI.properCivs().size(); j++) {
			PlayerTypes opponentId = getWPAI.properCivs()[j];
			if(TEAMREF(civId).isAtWar(TEAMID(opponentId)) &&
					(declaringWar.count(civId) <= 0 ||
					!peaceScenario || theirSide.count(opponentId) <= 0) &&
					(declaringWar.count(opponentId) <= 0 ||
					!peaceScenario || theirSide.count(civId) <= 0)) {
				warsCont[civId]->insert(opponentId);
				FAssert(DoWBy[civId]->count(opponentId) <= 0);
				FAssert(DoWBy[opponentId]->count(civId) <= 0);
				FAssert(DoWOn[civId]->count(opponentId) <= 0);
				FAssert(DoWOn[opponentId]->count(civId) <= 0);
			}
		}
	}
	// Need ArmamentForecasts for initially uninvolved parties (if any)
	set<PlayerTypes> uninvolved;
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
		if(noWarVsExtra) { // diff = theirSide - theyAndTheirVassals
			std::set<PlayerTypes> diff;
			std::set_difference(theirSide.begin(), theirSide.end(),
					theyAndTheirVassals.begin(), theyAndTheirVassals.end(),
					std::inserter(diff, diff.begin()));
			ig->removeWar(declaringWar, diff);
		}
		else ig->removeWar(declaringWar, theirSide);
	}
	int timeHorizon = 25;
	// Look a bit farther into the future when in a total war
	if(prepTime <= 0 && warEvalParams.isTotal())
		timeHorizon += 5;
	// Look a bit farther on Marathon speed
	if(GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).
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

	// They're all guaranteed to be allocated (no is-NULL checks needed)
	delete ig;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		delete lostCitiesPerCiv[i];
		delete conqueredCitiesPerCiv[i];
		delete DoWOn[i];
		delete DoWBy[i];
		delete warsCont[i];
	}
	for(int i = 0; i < MAX_CIV_TEAMS; i++)
		delete capitulationsAcceptedPerTeam[i];
}

WarEvalParameters& MilitaryAnalyst::evaluationParameters() {

	return warEvalParams;
}

PlayerTypes MilitaryAnalyst::ourId() const {

	return weId;
}

bool MilitaryAnalyst::isOnOurSide(TeamTypes tId) const {

	return GET_TEAM(tId).getMasterTeam() == TEAMREF(weId).getMasterTeam() ||
			warEvalParams.isWarAlly(tId);
}

bool MilitaryAnalyst::isOnTheirSide(TeamTypes tId, bool defensivePacts) const {

	bool r = GET_TEAM(tId).getMasterTeam() == GET_TEAM(warEvalParams.targetId()).
			getMasterTeam() || warEvalParams.isExtraTarget(tId);
	if(!defensivePacts || r)
		return r;
	for(size_t i = 0; i < getWPAI.properTeams().size(); i++) {
		TeamTypes loopTeamId = getWPAI.properTeams()[i];
		if(GET_TEAM(tId).isDefensivePact(loopTeamId) && isOnTheirSide(loopTeamId))
			return true;
	}
	return false;
}

bool MilitaryAnalyst::isPeaceScenario() const {

	return peaceScenario;
}

bool MilitaryAnalyst::doWePlanToDeclWar(PlayerTypes civId) const {

	CvTeamAI& agent = TEAMREF(weId);
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
	CvTeamAI const& agent = TEAMREF(weId);
	WarAndPeaceCache const& ourCache = we.warAndPeaceAI().getCache();
	/*  Counts the number of nukes. Assume no further build-up of nukes throughout
		the military analysis. (They take long to build.) */
	double ourNukes = ourCache.getPowerValues()[NUCLEAR]->num();
	double ourTargets = 0;
	for(size_t i = 0; i < getWPAI.properCivs().size(); i++) {
		CvPlayer const& target = GET_PLAYER(getWPAI.properCivs()[i]);
		if(isWar(weId, target.getID())) {
			double v = (target.isAVassal() ? 0.5 : 1);
			ourTargets += v;
		}
	}
	for(size_t j = 0; j < getWPAI.properCivs().size(); j++) {
		// Can't call it "they" b/c that's what this class calls the primary war target
		CvPlayerAI const& civ = GET_PLAYER(getWPAI.properCivs()[j]);
		if(!isWar(weId, civ.getID()))
			continue;
		double civNukes = civ.warAndPeaceAI().getCache().
				getPowerValues()[NUCLEAR]->num();
		if(ourNukes + civNukes < 0.5)
			return;
		// The AI is (much) more willing to fire nukes in total war than in limited war
		WarPlanTypes ourWarplan = agent.AI_getWarPlan(civ.getTeam());
		WarPlanTypes civWarplan = GET_TEAM(civ.getTeam()).AI_getWarPlan(agent.getID());
		// If one side goes total, the other will follow suit
		bool isTotal = false;
		if(ourWarplan == WARPLAN_PREPARING_TOTAL || ourWarplan == WARPLAN_TOTAL ||
				civWarplan == WARPLAN_PREPARING_TOTAL || civWarplan == WARPLAN_TOTAL)
			isTotal = true;
		// (Human: let's trust the war plan type of the human proxy AI)
		if(warEvalParams.targetId() == civ.getTeam())
			isTotal = warEvalParams.isTotal();
		double portionFired = isTotal ? 0.75 : 0.25;
		// Assume that vassals are hit by fewer nukes, and fire fewer nukes.
		double vassalFactorWe = 1;
		if(agent.isAVassal())
			vassalFactorWe = 0.5;
		double vassalFactorCiv = 1;
		if(agent.isAVassal())
			vassalFactorCiv = 0.5;
		double civTargets = 0;
		for(size_t i = 0; i < getWPAI.properCivs().size(); i++) {
			CvPlayer const& target = GET_PLAYER(getWPAI.properCivs()[i]);
			if(isWar(civ.getID(), target.getID())) {
				double v = (target.isAVassal() ? 0.5 : 1);
				civTargets += v;
			}
		}
		double firedOnUs = portionFired * vassalFactorWe * vassalFactorCiv *
				/*  sqrt for pessimism - we may get hit worse than the
					average target, and that worries us. */
				civNukes / std::sqrt((double)civTargets);
		firedOnUs = std::min(firedOnUs, 1.5 * we.getNumCities());
		double firedByUs = 0;
		if(ourTargets > 0) {
			firedByUs = portionFired * vassalFactorWe * vassalFactorCiv * ourNukes *
					/*  Apparently short for "Our words are backed with(/by)
						nuclear weapons" */
					(we.AI_isDoStrategy(AI_STRATEGY_OWABWNW) ? 1.1 : 0.9) /
					ourTargets;
		}
		firedByUs = std::min(firedByUs, 1.5 * civ.getNumCities());
		double hitUs = firedOnUs * interceptionMultiplier(agent.getID());
		double hitCiv = firedByUs * interceptionMultiplier(civ.getTeam());
		nukesSufferedBy[weId] += hitUs;
		nukesSufferedBy[civ.getID()] += hitCiv;
		nukesFiredBy[weId] += firedByUs;
		nukesFiredBy[civ.getID()] += firedOnUs;
		nukedCities[weId][civ.getID()] += hitCiv;
		nukedCities[civ.getID()][weId] += hitUs;
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
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		PlayerTypes civId = (PlayerTypes)i;
		InvasionGraph::Node* node = ig->getNode(civId);
		if(node == NULL)
			continue;
		node->getConquests(*conqueredCitiesPerCiv[i]);
		node->getLosses(*lostCitiesPerCiv[i]);
		node->getCapitulationsAccepted(*capitulationsAcceptedPerTeam[i]);
	}
	// Predict scores as current game score modified based on gained/ lost population
	CvGame& g = GC.getGame();
	WarAndPeaceCache const& ourCache = GET_PLAYER(weId).warAndPeaceAI().getCache();
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		PlayerTypes civId = (PlayerTypes)i;
		double popIncr = 0;
		for(std::set<int>::const_iterator it = conqueredCities(civId).begin();
				it != conqueredCities(civId).end(); it++) {
			WarAndPeaceCache::City* c = ourCache.lookupCity(*it);
			if(c == NULL) continue;
			popIncr += c->city()->getPopulation();
		}
		for(std::set<int>::const_iterator it = lostCities(civId).begin();
				it != lostCities(civId).end(); it++) {
			WarAndPeaceCache::City* c = ourCache.lookupCity(*it);
			if(c == NULL) continue;
			popIncr -= c->city()->getPopulation();
		}
		popIncr /= std::max(1, GET_PLAYER(civId).getTotalPopulation());
		/*  Only apply half of the relative increase b/c game score is only partially
			determined by population */
		gameScore[civId] = g.getPlayerScore(civId) * (1 + popIncr / 2);
	}
	// Precompute a table for isWar b/c it's frequently used
	for(int i = 0; i < MAX_CIV_PLAYERS; i++) {
		PlayerTypes c1 = (PlayerTypes)i;
		for(int j = 0; j < MAX_CIV_PLAYERS; j++) {
			PlayerTypes c2 = (PlayerTypes)j;
			isWarTable[c1][c2] = (getWarsContinued(c1).count(c2) > 0 ||
					getWarsDeclaredBy(c1).count(c2) > 0 ||
					getWarsDeclaredOn(c1).count(c2) > 0);
		}
	}
}

std::set<int> const& MilitaryAnalyst::lostCities(PlayerTypes civId) const {

	if(lostCitiesPerCiv[civId] == NULL)
		return emptyIntSet;
	return *lostCitiesPerCiv[civId];
}

std::set<int> const& MilitaryAnalyst::conqueredCities(PlayerTypes civId) const {

	if(conqueredCitiesPerCiv[civId] == NULL)
		return emptyIntSet;
	return *conqueredCitiesPerCiv[civId];
}

std::set<PlayerTypes> const& MilitaryAnalyst::getWarsDeclaredBy(
		PlayerTypes civId) const {

	if(DoWBy[civId] == NULL)
		return emptyCivSet;
	return *DoWBy[civId];
}

std::set<PlayerTypes> const& MilitaryAnalyst::getWarsDeclaredOn(
		PlayerTypes civId) const {

	if(DoWOn[civId] == NULL)
		return emptyCivSet;
	return *DoWOn[civId];
}

double MilitaryAnalyst::predictedGameScore(PlayerTypes civId) const {

	return gameScore[civId];
}

std::set<PlayerTypes> const& MilitaryAnalyst::getWarsContinued(
		PlayerTypes civId) const {

	if(warsCont[civId] == NULL)
		return emptyCivSet;
	return *warsCont[civId];
}

bool MilitaryAnalyst::isWar(PlayerTypes c1, PlayerTypes c2) const {

	return isWarTable[c1][c2];
}

bool MilitaryAnalyst::isWar(TeamTypes t1, TeamTypes t2) const {

	return isWar(GET_TEAM(t1).getLeaderID(), GET_TEAM(t2).getLeaderID());
}

int MilitaryAnalyst::turnsSimulated() const {

	return turnsSim;
}

bool MilitaryAnalyst::isPartOfAnalysis(PlayerTypes civId) const {

	return partOfAnalysis.count(civId) > 0;
}


double MilitaryAnalyst::getNukesSufferedBy(PlayerTypes civId) const {

	if(civId == NO_PLAYER)
		return 0;
	return nukesSufferedBy[civId];
}

double MilitaryAnalyst::getNukesFiredBy(PlayerTypes civId) const {

	if(civId == NO_PLAYER)
		return 0;
	return nukesFiredBy[civId];
}

double MilitaryAnalyst::getNukedCities(PlayerTypes byId, PlayerTypes ownerId) const {

	if(byId == NO_PLAYER || ownerId == NO_PLAYER)
		return 0;
	return nukedCities[byId][ownerId];
}

bool MilitaryAnalyst::hasCapitulated(TeamTypes teamId) const {

	CvTeam const& t = GET_TEAM(teamId);
	if(teamId == BARBARIAN_TEAM || !t.isAlive() || t.isAVassal() || t.isMinorCiv())
		return false;
	for(size_t i = 0; i < getWPAI.properCivs().size(); i++) {
		PlayerTypes civId = getWPAI.properCivs()[i];
		InvasionGraph::Node* node = ig->getNode(civId);
		if(TEAMID(civId) == teamId && node != NULL && node->hasCapitulated())
			return true;
	}
	return false;
}

bool MilitaryAnalyst::isEliminated(PlayerTypes civId) const {

	InvasionGraph::Node* node = ig->getNode(civId);
	if(node == NULL)
		return false;
	std::set<int> r;
	node->getLosses(r);
	return (((int)r.size()) == GET_PLAYER(civId).getNumCities());
}

std::set<TeamTypes> const& MilitaryAnalyst::getCapitulationsAccepted(
		TeamTypes masterId) const {

	if(capitulationsAcceptedPerTeam[masterId] == NULL)
		return emptyTeamSet;
	return *capitulationsAcceptedPerTeam[masterId];
}

double MilitaryAnalyst::lostPower(PlayerTypes civId,
		MilitaryBranchTypes mb) const {

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

	set<int> const* cities = conquests ? conqueredCitiesPerCiv[civId] :
			lostCitiesPerCiv[civId];
	if(cities != NULL && !cities->empty())
		report.log("Cities %s:", conquests ? "conquered" : "lost");
	for(set<int>::const_iterator it = cities->begin(); it != cities->end(); it++) {
		CvCity* c = WarAndPeaceCache::City::cityById(*it);
		if(c != NULL)
			report.log("%s", report.cityName(*c));
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
	std::set<TeamTypes>* caps = capitulationsAcceptedPerTeam[teamId];
	if(caps != NULL && !caps->empty())
		report.log("Capitulation accepted from:");
	for(set<TeamTypes>::const_iterator it = caps->begin(); it != caps->end(); it++) {
		// The team name (e.g. Team1) would not be helpful.
		for(size_t i = 0; i < getWPAI.properCivs().size(); i++) {
			PlayerTypes vassalMemberId = getWPAI.properCivs()[i];
			if(TEAMID(vassalMemberId) == *it)
				report.log("%s", report.leaderName(vassalMemberId));
		}
	}
}

void MilitaryAnalyst::logDoW(PlayerTypes civId) {

	if(!DoWBy[civId]->empty()) {
		report.log("Wars declared by %s:",
				report.leaderName(civId));
		for(set<PlayerTypes>::const_iterator it = DoWBy[civId]->begin();
				it != DoWBy[civId]->end(); it++)
			report.log("%s", report.leaderName(*it));
	}
	if(!DoWOn[civId]->empty()) {
		report.log("Wars declared on %s:",
				report.leaderName(civId));
		for(set<PlayerTypes>::const_iterator it = DoWOn[civId]->begin();
				it != DoWOn[civId]->end(); it++)
			report.log("%s", report.leaderName(*it));
	}
	if(!warsCont[civId]->empty()) {
		report.log("Wars continued:");
		for(set<PlayerTypes>::const_iterator it = warsCont[civId]->begin();
				it != warsCont[civId]->end(); it++)
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

// </advc.104>
