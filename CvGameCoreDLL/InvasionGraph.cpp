// <advc.104> New class; see InvasionGraph.h for description

#include "CvGameCoreDLL.h"
#include "InvasionGraph.h"
#include "WarAndPeaceAgent.h"
#include "ArmamentForecast.h"
#include "MilitaryAnalyst.h"
#include "WarEvalParameters.h"
#include "CvGamePlay.h"
#include "CvPlot.h"
#include "CvArea.h"
#include <sstream>

using std::ostringstream;
using std::set;
using std::vector;
using std::string;


InvasionGraph::InvasionGraph(MilitaryAnalyst& m,
		set<PlayerTypes> const& warParties, bool peaceScenario) :
		m(m), warParties(warParties),
		report(m.evaluationParameters().getReport()),
		isPeaceScenario(peaceScenario) {

	lossesDone = false;
	allWarPartiesKnown = false;
	firstSimulateCall = true;
	timeLimit= -1;
	weId = m.ourId();
	report.log("Constructing invasion graph");
	// Barbs get an element, but it remains NULL
	for(int i = 0; i < MAX_PLAYERS; i++)
		nodeMap[i] = NULL;
	for(set<PlayerTypes>::const_iterator it = warParties.begin();
			it != warParties.end(); it++)
		nodeMap[*it] = new Node(*it, *this);
	for(set<PlayerTypes>::const_iterator it = warParties.begin();
			it != warParties.end(); it++)
		nodeMap[*it]->findAndLinkTarget();
	if(warParties.empty())
		report.log("(no civs are currently at war)");
	report.log("");
}

InvasionGraph::~InvasionGraph() {

	for(int i = 0; i < MAX_PLAYERS; i++)
		if(nodeMap[i] != NULL)
			delete nodeMap[i];
}

InvasionGraph::Node* InvasionGraph::getNode(PlayerTypes civId) const {

	return nodeMap[civId];
}

InvasionGraph::Node& InvasionGraph::owner() const {

	return *getNode(m.ourId());
}

void InvasionGraph::addFutureWarParties(
		std::set<PlayerTypes> const& ourSide,
		std::set<PlayerTypes> const& ourFutureOpponents) {

	allWarPartiesKnown = true;
	if(ourSide.empty())
		return; // Don't update targets unnecessarily
	for(set<PlayerTypes>::const_iterator it = ourSide.begin();
			it != ourSide.end(); it++) {
		if(nodeMap[*it] == NULL)
			nodeMap[*it] = new Node(*it, *this);
		nodeMap[*it]->addWarOpponents(ourFutureOpponents);
	}
	for(set<PlayerTypes>::const_iterator it = ourFutureOpponents.begin();
			it != ourFutureOpponents.end(); it++) {
		if(nodeMap[*it] == NULL)
			nodeMap[*it] = new Node(*it, *this);
		nodeMap[*it]->addWarOpponents(ourSide);
	}
	/* Finding targets for the new nodes doesn't necessarily suffice b/c
	   phase 1 may have erased edges which may become valid again after
	   the armament forecast. */
	report.log("Done adding war parties");
	updateTargets();
}

void InvasionGraph::removeWar(std::set<PlayerTypes> const& ourSide,
		std::set<PlayerTypes> const& theirSide) {

	allWarPartiesKnown = true;
	if(ourSide.empty())
		return;
	for(set<PlayerTypes>::const_iterator it = ourSide.begin();
			it != ourSide.end(); it++) {
		if(nodeMap[*it] != NULL)
			nodeMap[*it]->removeWarOpponents(theirSide);
	}
	for(set<PlayerTypes>::const_iterator it = theirSide.begin();
			it != theirSide.end(); it++) {
		if(nodeMap[*it] != NULL)
			nodeMap[*it]->removeWarOpponents(ourSide);
	}
	report.log("Done removing war parties");
	updateTargets();
	/* Don't delete any nodes; even if no longer a war party, may want to
	   know their (peacetime) armament forecast. */
}

void InvasionGraph::updateTargets() {

	report.log("War parties (may) have changed; reassigning targets");
	for(int i = 0; i < MAX_PLAYERS; i++) {
		if(nodeMap[i] != NULL)
			nodeMap[i]->findAndLinkTarget();
	}
	report.log("");
}

void InvasionGraph::addUninvolvedParties(std::set<PlayerTypes> const& parties) {

	for(set<PlayerTypes>::const_iterator it = parties.begin();
			it != parties.end(); it++) {
		if(nodeMap[*it] == NULL)
			nodeMap[*it] = new Node(*it, *this);
	}
}

InvasionGraph::Node::Node(PlayerTypes civId, InvasionGraph& outer) :
	outer(outer), report(outer.report),
	// I.e. weId is going to cheat by using info from other civs' caches
	cache(GET_PLAYER(civId).warAndPeaceAI().getCache()) {

	// Properly initialized in prepareForSimulation
	tempArmyLosses = emergencyDefPow = distractionByConquest = distractionByDefense = -1;
	cacheIndex = -1;
	componentDone = false;

	id = civId;
	weId = outer.weId;
	eliminated = false;
	capitulated = false;
	hasClashed = false;
	warTimeSimulated = 0;
	productionInvested = 0;
	primaryTarget = NULL;
	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
		isWarOpponent[i] = false;
	CvPlayerAI& thisCiv = GET_PLAYER(id);
	CvTeamAI& thisTeam = GET_TEAM(thisCiv.getTeam());
	// Collect war opponents
	for(size_t i = 0; i < getWPAI.properCivs().size(); i++) {
		PlayerTypes civId = getWPAI.properCivs()[i];
		TeamTypes tId = TEAMID(civId);
		// The second condition excludes opponents that we haven't met
		if(thisTeam.isAtWar(tId) && outer.warParties.count(civId) > 0) {
			warOpponents.insert(civId);
			isWarOpponent[civId] = true;
		}
	}
	for(size_t i = 0; i < NUM_BRANCHES; i++)
		lostPower[i] = shiftedPower[i] = 0;
	initMilitary();
	logPower("Present power");
	report.log("");
}

void InvasionGraph::Node::initMilitary() {

	PROFILE_FUNC();
	// Copy present military from cache and split HOME_GUARD off from ARMY
	std::vector<MilitaryBranch*> const& pm = cache.getPowerValues();
	MilitaryBranch::HomeGuard* hg = new MilitaryBranch::HomeGuard(*pm[HOME_GUARD]);
	double guardRatio = hg->initUnitsTrained(cache.numNonNavyUnits(),
			pm[ARMY]->power() - pm[NUCLEAR]->power());
	military.push_back(hg);
	MilitaryBranch::Army* army = new MilitaryBranch::Army(*pm[ARMY]);
	army->setUnitsTrained(::round(pm[ARMY]->num() * (1 - guardRatio)),
			(1 - guardRatio) * (pm[ARMY]->power() - pm[NUCLEAR]->power()) +
			std::min(pm[NUCLEAR]->power(),
			/*  Limit contribution of nukes to invasions (someone needs to actually
				conquer and occupy the enemy cities) */
				0.35 * (pm[ARMY]->power() - pm[NUCLEAR]->power())));
	military.push_back(army);
	military.push_back(new MilitaryBranch::Fleet(*pm[FLEET]));
	military.push_back(new MilitaryBranch::Logistics(*pm[LOGISTICS]));
	military.push_back(new MilitaryBranch::Cavalry(*pm[CAVALRY]));
	military.push_back(new MilitaryBranch::NuclearArsenal(*pm[NUCLEAR]));
	/*  Remember the current values (including home guard, which isn't in cache)
		for computing GainedPower later */
	for(size_t i = 0; i < military.size(); i++)
		currentPow.push_back(military[i]->power());
}

InvasionGraph::Node::~Node() {

	for(size_t i = 0; i < military.size(); i++)
		delete military[i];
}

void InvasionGraph::Node::addWarOpponents(std::set<PlayerTypes> const& wo) {

	for(set<PlayerTypes>::const_iterator it = wo.begin(); it != wo.end(); it++) {
		if(!isWarOpponent[*it] &&
				/*  Should arguably be guaranteed somewhere higher up; not currently
					guaranteed when there is a holy war vote. */
				TEAMREF(id).getMasterTeam() != TEAMREF(*it).getMasterTeam()) {
			isWarOpponent[*it] = true;
			warOpponents.insert(*it);
		}
	}
	FAssertMsg(!warOpponents.empty(), "Node w/o war opponents");
}

void InvasionGraph::Node::removeWarOpponents(std::set<PlayerTypes> const& wo) {

	for(set<PlayerTypes>::const_iterator it = wo.begin(); it != wo.end(); it++) {
		if(isWarOpponent[*it]) {
			isWarOpponent[*it] = false;
			warOpponents.erase(*it);
		}
	}
}

bool InvasionGraph::Node::hasTarget() const {

	return primaryTarget != NULL;
}

bool InvasionGraph::Node::isIsolated() const {

	return !hasTarget() && targetedBy.empty();
}

void InvasionGraph::Node::logPower(char const* msg) const {

	if(report.isMute())
		return;
	ostringstream out;
	out << msg << " of " << report.leaderName(id)
			<< "\n\np(.\n";
	int nLogged = 0;
	for(size_t i = 0; i < military.size(); i++) {
		int pow = ::round(military[i]->power() - lostPower[i]);
		if(pow <= 0) {
			/*  -5 as threshold b/c minor errors in the simulation aren't worth
				fixing */
			FAssertMsg(pow > -5, "More lost power than there was to begin with");
			continue;
		}
		if(nLogged > 0)
			out << ", ";
		out << (*military[i]) << " " << pow;
		nLogged++;
	}
	report.log("%s", out.str().c_str());
	if(nLogged == 0)
		report.log("0");
}

void InvasionGraph::Node::findAndLinkTarget() {

	PROFILE_FUNC();
	PlayerTypes targetId = NO_PLAYER;
	bool canHaveTarget = true;
	if(isEliminated() || hasCapitulated()) {
		canHaveTarget = false;
		report.log("(%s defeated in phase I)", report.leaderName(id));
	}
	if(canHaveTarget) {
		targetId = findTarget();
		if(targetId == NO_PLAYER) {
			if(warOpponents.empty())
				report.log("%s has no war opponents.", report.leaderName(id));
			else report.log("%s can't reach any of his/her war opponents.",
				report.leaderName(id));
		}
	}
	if(targetId == NO_PLAYER) {
		if(primaryTarget != NULL) {
			primaryTarget->targetedBy.erase(id);
			report.log("(no longer targeting %s)",
					report.leaderName(primaryTarget->getId()));
			primaryTarget = NULL;
		}
		return;
	}
	report.log("%s assumes _%s_ to be the target of _%s_.",
				report.leaderName(weId), report.leaderName(targetId),
				report.leaderName(id));
	if(primaryTarget != NULL && primaryTarget->getId() != targetId) {
		primaryTarget->targetedBy.erase(id);
		report.log("(switching from %s)", report.leaderName(primaryTarget->getId(),
				8));
	}
	primaryTarget = outer.nodeMap[targetId];
	if(primaryTarget == NULL) {
		FAssert(!TEAMREF(weId).isHasMet(TEAMID(targetId)));
		report.log("%s hasn't met the above target yet, ignores it.",
				report.leaderName(weId));
	}
	else primaryTarget->targetedBy.insert(id);
}


PlayerTypes InvasionGraph::Node::findTarget(TeamTypes include) const {

	PlayerTypes likeliestTarget = NO_PLAYER;
	/*  W/e the current target is according to unit missions, assume it
		will change once war is declared. */
	bool bSearchedLikeliest = false;
	if(outer.allWarPartiesKnown || include != NO_TEAM) {
		bSearchedLikeliest = true;
		/*  Find the city with the highest targetValue and check which potential
			target has reachable cities left. (City::canReach says which ones are
			reachable in the actual game state; doesn't take into account
			Node::hasLost.) */
		for(int i = 0; i < cache.size(); i++) {
			WarAndPeaceCache::City* city = cache.getCity(i);
			if(city == NULL)
				continue;
			if(!city->canReach() || city->isOwnCity())
				break; // Because of sorting, rest is also going to be invalid.
			if(!isValidTarget(*city, include))
				continue;
			// First hit is best target b/c cache is sorted
			if(likeliestTarget == NO_PLAYER) {
				PlayerTypes owner = city->cityOwner();
				if(!TEAMREF(id).isAtWar(TEAMID(owner)))
					return owner;
				/*  Otherwise, check unit missions, but may still fall back
					on likeliestTarget. */
				likeliestTarget = owner;
				break;
			}
		}
		if(likeliestTarget == NO_PLAYER)
			return NO_PLAYER;
	}
	// Against whom does this Node have the most missions?
	PlayerTypes mostMissions = NO_PLAYER;
	// Tbd. possibly: Use a fraction of cache.numNonNavy as the threshold
	int maxCount = 3;
	for(set<PlayerTypes>::const_iterator it = warOpponents.begin();
				it != warOpponents.end(); it++) {
		PlayerTypes oppId = *it;
		int n = cache.targetMissionCount(oppId);
		if(n <= maxCount)
			continue;
		// Same conditions as in isValidTarget
		if(outer.nodeMap[oppId] == NULL ||
				outer.nodeMap[oppId]->isEliminated() ||
				outer.nodeMap[oppId]->hasCapitulated())
			continue;
		// Still need to ensure that oppId has a valid city
		bool bValidFound = false;
		/*  In large games, looking up oppId's cities is faster than a pass through
			our whole cache. */
		int foo=-1;
		for(CvCity* cvCity = GET_PLAYER(oppId).firstCity(&foo); cvCity != NULL;
				cvCity = GET_PLAYER(oppId).nextCity(&foo)) {
			WarAndPeaceCache::City* c = cache.lookupCity(*cvCity);
			if(c != NULL && c->canReach() && !outer.nodeMap[oppId]->
					hasLost(c->id())) {
				bValidFound = true;
				break;
			}
		}
		if(!bValidFound)
			continue;
		maxCount = n;
		mostMissions = oppId;
	}
	if(mostMissions != NO_PLAYER) {
		report.log("Target of %s determined based on unit missions.",
				report.leaderName(id));
		return mostMissions;
	}
	report.log("Too few missions to determine target of %s",
			report.leaderName(id));
	// Fall back on likeliestTarget.
	if(likeliestTarget != NO_PLAYER)
		return likeliestTarget;
	if(bSearchedLikeliest)
		return NO_PLAYER;
	// Haven't looked for the likeliest target yet (b/c that's expensive); do so now.
	for(int i = 0; i < cache.size(); i++) {
		WarAndPeaceCache::City* city = cache.getCity(i);
		if(city == NULL)
			continue;
		if(!city->canReach() || city->isOwnCity())
			break;
		if(isValidTarget(*city, include))
			return city->cityOwner();
	}
	return NO_PLAYER;
}

bool InvasionGraph::Node::isValidTarget(WarAndPeaceCache::City const& c,
		TeamTypes include) const {

	PlayerTypes const owner = c.cityOwner();
	return !(owner == NO_PLAYER || (!isWarOpponent[owner] &&
			(include == NO_TEAM || GET_PLAYER(owner).getMasterTeam() !=
			GET_TEAM(include).getMasterTeam())) ||
			outer.nodeMap[owner] == NULL ||
			outer.nodeMap[owner]->hasLost(c.id()) ||
			// Important for phase II:
			outer.nodeMap[owner]->isEliminated() ||
			outer.nodeMap[owner]->hasCapitulated());
}

WarAndPeaceReport& InvasionGraph::getReport() {

	return report;
}

PlayerTypes InvasionGraph::Node::getId() const {

	return id;
}

size_t InvasionGraph::Node::findCycle(vector<Node*>& path) {

	// Check path to detect cycle
	for(size_t i = 0; i < path.size(); i++)
		if(path[i]->getId() == getId())
			return i;
	path.push_back(this);
	if(primaryTarget == NULL)
		return path.size();
	return primaryTarget->findCycle(path);
}

void InvasionGraph::Node::resolveLossesRec() {

	componentDone = true;
	if(targetedBy.empty())
		return;
	// Copy targetedBy b/c resolveLossesRec may remove elements
	set<PlayerTypes> tmp;
	tmp.insert(targetedBy.begin(), targetedBy.end());
	for(set<PlayerTypes>::iterator it = tmp.begin(); it != tmp.end(); it++) {
		if(targetedBy.count(*it) == 0) // Check if current element has been removed
			continue;
		outer.nodeMap[*it]->resolveLossesRec();
	}
	resolveLosses();
}

void InvasionGraph::Node::addConquest(WarAndPeaceCache::City const& c) {

	report.log("*%s* (%s) assumed to be *conquered* by %s",
			report.cityName(*c.city()),
			report.leaderName(c.cityOwner()), report.leaderName(id));
	conquests.push_back(&c);
	// Advance cache index past the city just conquered
	while(cacheIndex < cache.size()) {
		 WarAndPeaceCache::City* cacheCity = cache.getCity(cacheIndex);
		 if(cacheCity != NULL && cacheCity->id() == c.id())
			break;
		 cacheIndex++;
	}
	cacheIndex++;
}

void InvasionGraph::Node::addLoss(WarAndPeaceCache::City const& c) {

	losses.insert(c.id());
}

void InvasionGraph::Node::prepareForSimulation() {

	PROFILE_FUNC();
	cacheIndex = 0;
	//cache.sortCitiesByAttackPriority(); // Done in WarAndPeaceCache now
	componentDone = false;
	emergencyDefPow = 0;
	distractionByConquest = 0;
	distractionByDefense = 0;
	tempArmyLosses = 0;
}

void InvasionGraph::Node::logTypicalUnits() {

	// Should no longer be needed; now done in initMilitary (where it belongs)
	/*if(military[HOME_GUARD] != NULL &&
			military[HOME_GUARD]->getTypicalUnit() == NULL)
		military[HOME_GUARD]->updateTypicalUnit();*/
	if(report.isMute())
		return;
	// Log typical units only once per evaluation (they don't change)
	if(outer.isPeaceScenario || (outer.allWarPartiesKnown &&
			outer.m.evaluationParameters().getPreparationTime() > 0))
		return;
	report.log("Typical unit ratings of *%s* (by military branch):",
			report.leaderName(id));
	report.log("\nbq."); // Textile block quote
	for(size_t i = 0; i < military.size(); i++) {
		MilitaryBranch& mb = *military[i];
		CvUnitInfo const* uptr = mb.getTypicalUnit();
		if(uptr == NULL)
			continue;
		CvUnitInfo const& u = *uptr;
		int actualCost = u.getProductionCost();
		int actualPow = ::round(mb.getTypicalUnitPower());
		report.log("%s: %d (%s, cost: %d)", mb.str(), actualPow,
				report.unitName(u), actualCost);
		int povCost = mb.getTypicalUnitCost(outer.weId);
		int povPow = ::round(mb.getTypicalUnitPower(outer.weId));
		if(povPow != actualPow) {
			/*  povCost and actualCost often won't match b/c actualCost here
				ignores handicap */
			report.log("(%s's estimate: %d cost, %d power)",
					report.leaderName(outer.weId), povCost, povPow);
		}
	}
	report.log("");
}

void InvasionGraph::Node::predictArmament(int duration, bool noUpgrading) {

	if(isEliminated()) {
		report.log("No armament for %s (eliminated)", report.leaderName(id));
		return;
	}
	// Target city assumed for the forecast (to decide on naval build-up)
	WarAndPeaceCache::City const* tC = targetCity();
	WarEvalParameters const& params = outer.m.evaluationParameters();
	TeamTypes masterTeamId = GET_PLAYER(id).getMasterTeam();
	/* Simulated DoW is taken into account by targetCity, but a
	   (simulated) warplan isn't. */
	if(masterTeamId == GET_PLAYER(weId).getMasterTeam() ||
			params.isWarAlly(masterTeamId)) {
		TeamTypes targetId = params.targetId();
		if(!outer.isPeaceScenario && !outer.allWarPartiesKnown &&
				!TEAMREF(weId).isAtWar(targetId)) {
			report.setMute(true);
			PlayerTypes actualTarget = findTarget(targetId);
			tC = targetCity(actualTarget);
			report.setMute(false);
		}
	}
	report.log("");
	ArmamentForecast forec(id, outer.m, military, duration, productionPortion(),
			tC, outer.isPeaceScenario, !outer.lossesDone, outer.allWarPartiesKnown,
			noUpgrading);
	productionInvested += forec.getProductionInvested();
	logPower("Predicted power");
	report.log("");
}

double InvasionGraph::Node::productionPortion() const {

	int lostPop = 0;
	for(std::set<int>::const_iterator it = losses.begin();
			it != losses.end(); it++) {
		CvCity* c = WarAndPeaceCache::City::cityById(*it);
		if(c != NULL)
			lostPop += c->getPopulation();
	}
	int originalPop = GET_PLAYER(id).getTotalPopulation();
	if(originalPop <= 0) return 0;
	FAssert(lostPop <= originalPop);
	return (originalPop - lostPop) / ((double)originalPop);
}

/*  Some notes based on re-reading this function some months after writing it:
	clashOnly: isNaval if neither side can reach its target city by land; then
	  the clash is between fleets only. If !isNaval, then the two armies are
	  assumed to meet each other halfway in the largest shared area and fleets
	  don't play any role.
	!clashOnly: isNaval if the attacker (*this) can't reach its target city by
	  land. Then there is a naval battle (same as fleet clash above) and the
	  surviving cargo of the attacker proceeds as described under !isNaval:
	  If !isNaval, then some naval fighting can happen along the coast. A clash
	  may occur near the target city, but it's skipped if the defending army is
	  too weak. If the attacker wins the clash, a siege of the target city is
	  simulated. */
SimulationStep* InvasionGraph::Node::step(double armyPortionDefender,
		double armyPortionAttacker, bool clashOnly) const {

	PROFILE_FUNC();
	WarAndPeaceCache::City const* const c = (clashOnly ? NULL : targetCity());
	if(c == NULL && !clashOnly)
		return NULL;
	CvCity const* const cvCity = (c == NULL ? NULL : c->city());
	if(cvCity == NULL && !clashOnly) {
		FAssert(cvCity != NULL);
		return NULL;
	}
	Node& defender = *primaryTarget;
	int const defCities = GET_PLAYER(defender.id).getNumCities();
	int const attCities = GET_PLAYER(id).getNumCities();
	double confAlliesAtt = 1, confAlliesDef = 1;
	// If the portion is 0, then the army should be ignored by all leaders
	if(id == weId && armyPortionDefender > 0.001)
		confAlliesAtt = GET_PLAYER(id).warAndPeaceAI().warConfidenceAllies();
	if(defender.id == weId && armyPortionAttacker > 0.001)
		confAlliesDef = GET_PLAYER(defender.id).warAndPeaceAI().
				warConfidenceAllies();
	/*  NB: There's also InvasionGraph::willingness, which is applied before
		computing the army portions. */
	/* Adjust the portion of the army that is assumed to be absent,
	   i.e. 1 minus portion */
	armyPortionDefender = std::max(0.0, 1 - (1 - armyPortionDefender) *
			confAlliesAtt);
	armyPortionAttacker = std::max(0.0, 1 - (1 - armyPortionAttacker) *
			confAlliesDef);
	// No clash if no mutual reachability
	FAssert(!clashOnly || (targetCity() != NULL && defender.targetCity() != NULL));
	if(clashOnly) {
		report.log("*Clash* of %s and %s",
				report.leaderName(getId()),
				report.leaderName(defender.getId()));
	}
	else {
		report.log("Attack on *%s* by %s",
				report.cityName(*cvCity), report.leaderName(id));
	}
	report.log("Employing %d (%s) and %d (%s) percent of armies",
			::round(100 * armyPortionAttacker), report.leaderName(getId()),
			::round(100 * armyPortionDefender),
			report.leaderName(defender.getId()));
	// Only log if portions are non-trivial
	if(armyPortionDefender > 0.001 && armyPortionDefender < 0.999 &&
			::round(confAlliesAtt * 100) != 100)
		report.log("Confidence in allies of %s: %d percent",
				report.leaderName(id), ::round(confAlliesAtt * 100));
	if(armyPortionAttacker > 0.001 && armyPortionAttacker < 0.999 &&
			::round(confAlliesDef * 100) != 100)
		report.log("Confidence in allies of %s: %d percent",
				report.leaderName(defender.id), ::round(confAlliesDef * 100));
	SimulationStep* rptr = new SimulationStep(id, c);
	SimulationStep& r = *rptr;
	double armyPowRaw = military[ARMY]->power() - lostPower[ARMY];
	double cavPowRaw = military[CAVALRY]->power() - lostPower[CAVALRY];
	double cavRatioRaw = ::dRange(armyPowRaw <= 0 ? 0 : cavPowRaw / (armyPowRaw +
			military[HOME_GUARD]->power() - lostPower[HOME_GUARD]), 0.0, 1.0);
	armyPowRaw -= tempArmyLosses;
	cavPowRaw -= tempArmyLosses * cavRatioRaw;
	double armyPow = armyPowRaw * armyPortionAttacker;
	armyPow = std::max(0.0, armyPow);
	double cavPow = cavPowRaw * armyPortionAttacker;
	/* Cavalry being used defensively (counter) is not uncommon, and then
	   cavPow can be greater than armyPow. (Still assume that cavalry is used
	   aggressively most of the time.) */
	cavPow = std::min(cavPow, armyPow);
	// defender.TempArmyLosses assumed to be available
	double targetArmyPow = (defender.military[ARMY]->power()
			- defender.lostPower[ARMY]) * armyPortionDefender;
	double targetCavPow = (defender.military[CAVALRY]->power()
			- defender.lostPower[CAVALRY]) * armyPortionDefender;
	targetArmyPow = std::max(0.0, targetArmyPow);
	targetCavPow = std::min(targetCavPow, targetArmyPow);
	bool isNaval = false;
	if(clashOnly) {
		WarAndPeaceCache::City const* defTargetCity = defender.targetCity();
		WarAndPeaceCache::City const* attTargetCity = targetCity();
		if(defTargetCity != NULL && attTargetCity != NULL) {
			isNaval = !canReachByLand(attTargetCity->id()) &&
					!defender.canReachByLand(defTargetCity->id());
		}
		else FAssert(false); // They shouldn't clash then
	}
	else isNaval = !canReachByLand(c->id());
	bool canBombard = false;
	bool canBombardFromSea = false;
	bool canSoften = false;
	for(int i = 0; i < NUM_BRANCHES; i++) {
		MilitaryBranchTypes mb = (MilitaryBranchTypes)i;
		if(military[i]->canSoftenCityDefenders())
			canSoften = true;
		if(military[i]->canBombard()) {
			if(mb != FLEET)
				canBombard = true;
			else if(!clashOnly && cvCity->isCoastal())
				canBombardFromSea = true;
		}
	}
	// Optimism/ pessimism, learning from past success/ failure
	double confAttPers = 1, confDefPers = 1;
	if(id == weId || defender.id == weId) {
		bool const bTotal = outer.m.evaluationParameters().isTotal();
		if(id == weId) {
			confAttPers = GET_PLAYER(weId).warAndPeaceAI().warConfidencePersonal(
					isNaval, bTotal, defender.id);
		}
		else if(defender.id == weId) {
			confDefPers = GET_PLAYER(weId).warAndPeaceAI().warConfidencePersonal(
					// Defense doesn't hinge on navies and fighting in faraway lands
					false, bTotal, id);
		}
	}
	if(GET_PLAYER(id).isHuman())
		confDefPers *= GET_PLAYER(defender.id).warAndPeaceAI().confidenceAgainstHuman();
	if(GET_PLAYER(defender.id).isHuman())
		confAttPers *= GET_PLAYER(id).warAndPeaceAI().confidenceAgainstHuman();
	if(confAttPers > 1.001 || confDefPers > 1.001 || confAttPers < 0.999 || confDefPers < 0.999) {
		report.log("Personal confidence (%s/%s): %d/%d percent",
				report.leaderName(id), report.leaderName(defender.id),
				::round(confAttPers * 100), ::round(confDefPers * 100));
	}
	double confAtt = confAttPers;
	double confDef = confDefPers;
	// Assume that we don't know war successes of third parties
	if(id == weId || defender.id == weId) {
		double confAttLearned = 0, confDefLearned = 0;
		if(id == weId)
			confAttLearned = (GET_PLAYER(id).warAndPeaceAI().
					warConfidenceLearned(defender.id, true) - 1);
		if(defender.id == weId)
			confDefLearned = (GET_PLAYER(defender.id).warAndPeaceAI().
					warConfidenceLearned(id, false) - 1);
		report.log("Learned confidence (%s/%s): %d/%d percent",
					report.leaderName(id), report.leaderName(defender.id),
					::round(confAttLearned * 100 + 100),
					::round(confDefLearned * 100 + 100));
		/*  To avoid extreme bias, don't multiply with conf...Pers unless
			learned and personal confidence contradict each other. */
		if((confAttLearned < 0) != (confAtt < 1) && confAtt != 1 && confAttLearned != 0)
			confAttLearned *= (1 + std::abs(1 - confAtt));
		if((confDefLearned < 0) != (confDef < 1) && confDef != 1 && confDefLearned != 0)
			confDefLearned *= (1 + std::abs(1 - confDef));
		confAtt += confAttLearned;
		confDef += confDefLearned;
	}
	confAtt = ::dRange(confAtt, 0.4, 1.8);
	confDef = ::dRange(confDef, 0.4, 1.8);
	// Coastal battle (much overlap with clash code)
	if(!clashOnly && canBombardFromSea && !isNaval) {
		/*  Only defender is assumed to bring cargo units into battle.
			Fixme: [LOGISTICS]->power is the cargo capacity. The military power
			is greater, but currently not tracked. */
		double fleetPow = std::max(0.0, (military[FLEET]->power() - military[LOGISTICS]->power()
				- lostPower[FLEET] + lostPower[LOGISTICS]) * confAtt
				// Use attacker/ defender portion also for fleet
				* armyPortionAttacker);
		double targetFleetPow = std::max(0.0, (defender.military[FLEET]->power()
				- defender.lostPower[FLEET]) * confDef
				* armyPortionDefender);
		/* Don't factor in distance; naval units tend to be fast;
		   would have to use a special metric b/c City::getDistance is
		   for land units only. */
		bool attWin = (fleetPow > targetFleetPow);
		// att=false: Defender has no mobility advantage at sea
		std::pair<double,double> lwl = clashLossesWinnerLoser(fleetPow,
				targetFleetPow, false, true);
		/* Losses in logistics don't matter b/c no naval landing
		   is attempted, and the cost (of having to rebuild units) is already
		   included in the fleet losses. */
		double lossesAtt, lossesDef;
		if(attWin) {
			lossesAtt = lwl.first / confAtt;
			lossesDef = lwl.second / confDef;
		} else {
			lossesAtt = lwl.second / confAtt;
			lossesDef = lwl.first / confDef;
			report.log("Sea bombardment averted");
			canBombardFromSea = false;
		}
		r.reducePower(id, FLEET, lossesAtt);
		r.reducePower(defender.id, FLEET, lossesDef);
		report.log("Losses from sea battle (A/D): %d/%d",
				::round(lossesAtt), ::round(lossesDef));
	}
	if(canBombardFromSea)
		canBombard = true;
	bool cavalryAttack = !clashOnly && !canBombard && !isNaval && !canSoften
	/* Minor issue: a cavalry attack might be assumed when assessing priority
	   b/c then targetArmyPow is 0, but for the actual simulation, no cavalry
	   attack would be assumed. This is inconsistent. */
			&& cavPow / (armyPow+0.001) > 0.66 && cavPow > targetArmyPow;
	if(!clashOnly) {
		if(cavalryAttack) {
			armyPow = cavPow;
			report.log("Cavalry attack assumed; power: %d", ::round(armyPow));
		}
		else report.log("Attacker power: %d", ::round(armyPow));
		report.log("Defending army's power: %d", ::round(targetArmyPow));
	}
	bool sneakAttack = isSneakAttack(defender, clashOnly);
	bool attackerUnprepared = defender.isSneakAttack(*this, false);
	double deploymentDistDefender = 0;
	double deploymentDistAttacker = 0;
	double mwmalpFactor = 1;
	if(id == weId) {
		int mwmalp = GC.getLeaderHeadInfo(GET_PLAYER(id). // in the interval [0,4]
				getPersonalityType()).getMaxWarMinAdjacentLandPercent();
		mwmalpFactor = std::max(1, mwmalp + 5 - (mwmalp == 0 ? 1 : 0)) / 6.0;
		// NB: mwmalp also factors into defensibilityCost (WarUtilityAspect::Greed)
	}
	int healDuration = 0;
	/* The city-based distances are deliberately not updated upon the simulated
	   conquest of a city -- a conquered city won't immediately start producing
	   troops and won't immediately provide full mobility through tile ownership. */
	if(!clashOnly) {
		/*  If distance is so far that no city can be expected to be conquered
			within our maximal time horizon, we'd have to conclude that no conquest
			will ever happen, which is probably incorrect. Better to assume a
			shorter distance in this case. */
		deploymentDistAttacker = std::min(getWPAI.maxSeaDist(), c->getDistance());
		deploymentDistAttacker *= mwmalpFactor; /* If this makes leaders with a
			high mwmalp assume that they can't reach a very distant target -- OK. */
		/* Often, the attacker can move from one conquered city to the next.
		   Too complicated to do the distance measurements. The code above
		   assumes that the attacking units always start at a city owned in the
		   actual game. Reduce distance for subsequent attacks a bit in order to
		   match an average case. */
		if(!conquests.empty()) {
			CvCity const& latestConq = *conquests[conquests.size() - 1]->city();
			if(latestConq.getOwner() == defender.id &&
					latestConq.area() == cvCity->area()) {
				deploymentDistAttacker *= 0.6;
				// Will have to wait for some units to heal then though
				healDuration = 2;
				report.log("Deployment distance reduced b/c of prior conquest");
			}
		}
	}
	else {
		double dist = clashDistance(defender);
		FAssert(dist > 0); /* WarAndPeaceCache::City::updateDistance should
							  guarantee this. */
		deploymentDistAttacker = dist;
		deploymentDistDefender = dist;
		// Can't rule out that both armies are eliminated at this point
		if(armyPow + targetArmyPow > 1) {
			deploymentDistAttacker *= armyPow / (armyPow + targetArmyPow);
			deploymentDistAttacker *= mwmalpFactor;
			deploymentDistDefender -= deploymentDistAttacker;
		}
	}
	int deploymentDuration = ::round(deploymentDistAttacker);
	if(attackerUnprepared)
		deploymentDuration += (5 - GET_PLAYER(id).getCurrentEra() / 2);
	double reinforcementDist = deploymentDistAttacker;
	/* The reduction in attacking army power based on distance mainly accounts for
	   two effects:
	   a) Produced units become available with a delay.
	   b) The combat AI tends to form smaller stacks when distances are long; they
		 arrive in a trickle.
	   When sneak attacking, b) is less of a concern, but don't want to encourage
	   early long-distance wars; reduce deploymentDist only slightly. */
	if(sneakAttack) {
		if(clashOnly && defender.id == weId) {
			deploymentDistDefender *= 0.8;
			deploymentDistAttacker = 0;
		}
		else if(!clashOnly) {
			deploymentDistAttacker *= 0.8;
			deploymentDistDefender = 0;
		}
		/* This often allows sneak attackers to attack a city before civs that
		   have been at war for some time. These civs already get a chance to
		   conquer cities during the simulation prolog. That said, no build-up
		   is assumed during the prolog, so the sneak-attacking AI may over-
		   estimate its chances when it comes to grabbing cities. The proper
		   solution would be to interleave the simulation and build-up steps. */
		deploymentDuration = 3;
	}
	else if(isContinuedWar(defender)) {
		/*  War that is already being fought in the actual game state. Assume that
			units are already being deployed then. */
		deploymentDuration /= 2;
	}
	if(cavalryAttack) {
		deploymentDuration = ::round(0.6 * deploymentDuration);
		deploymentDistAttacker = ::round(0.75 * deploymentDistAttacker);
	}
	report.log("Deployment distances (%s/%s): %d/%d",
				report.leaderName(getId()),
				report.leaderName(defender.getId()),
				::round(deploymentDistAttacker),
				::round(deploymentDistDefender));
	if(!clashOnly) // Duration has no bearing on clashes
		report.log("Deployment duration: %d%s", deploymentDuration,
				(sneakAttack ? " (sneak attack)" : (attackerUnprepared ?
				" (attacker unprepared)" : "")));
	// Sea battle about naval landing
	if(isNaval) {
		double fleetPow = (military[FLEET]->power() - lostPower[FLEET]) * confAtt
				 * armyPortionAttacker;
		double targetFleetPow = (defender.military[FLEET]->power()
				- defender.lostPower[FLEET]) * confDef * armyPortionDefender;
		// Reduced b/c not all cargo ships are available for military purposes
		double cargoCap = 0.73 * (military[LOGISTICS]->power() - lostPower[LOGISTICS]);
		double logisticsPortion = 0;
		/*  Fixme: This logistics portion is way too small; cargoCap counts
			cargo space, whereas fleetPow is a power rating (strength^1.7).
			Means that Logistics losses aren't really counted. Will need
			to track power and cargo separately for the Logistics branch to
			fix this. */
		if(fleetPow > 1)
			logisticsPortion = cargoCap / fleetPow;
		// Cargo units can go multiple times once the naval battle is won
		cargoCap *= 1 + 1 / std::max(2.0, reinforcementDist);
		double logisticsPortionTarget = 0;
		if(targetFleetPow > 1)
			logisticsPortionTarget = (defender.military[LOGISTICS]->power() -
					defender.lostPower[LOGISTICS]) * confDef / targetFleetPow;
		// +30% for attacker b/c only a clear victory can prevent a naval landing
		double fleetPowMult = 1.3;
		if(clashOnly) {
			fleetPowMult = 1;
			/*  In a clash, let every civ overestimate its own chances; this
				should make it harder to deter the AI through naval build-up. */
			if(id == weId)
				fleetPowMult *= 1.25;
			else if(defender.id == weId)
				fleetPowMult /= 1.25;
		}
		bool attWin = (fleetPow > 1 && fleetPowMult * fleetPow > targetFleetPow);
		std::pair<double,double> lwl = clashLossesWinnerLoser(fleetPow,
				targetFleetPow, false, true);
		double lossesAtt, lossesDef;
		double typicalArmyUnitPow = military[ARMY]->getTypicalUnitPower(outer.weId);
		if(military[ARMY]->getTypicalUnit() == NULL) {
			FAssertMsg(attCities <= 0, "No typical army unit found");
			typicalArmyUnitPow = 3.25; // That's a Warrior
		}
		/* Tend to underestimate the head count b/c of outdated
		   units with power lower than typical. Hence the extra 20%. */
		double armySize = 1.2 * armyPow / typicalArmyUnitPow;
		if(attWin) {
			lossesAtt = lwl.first / confAtt;
			r.reducePower(id, FLEET, lossesAtt);
			// Don't try to predict lost army in cargo
			double logisticsLosses = lwl.first * logisticsPortion / confAtt;
			r.reducePower(id, LOGISTICS, logisticsLosses);
			lossesDef = lwl.second / confDef;
			r.reducePower(defender.id, FLEET, lossesDef);
			r.reducePower(defender.id, LOGISTICS,
					lwl.second * logisticsPortionTarget / confDef);
			if(clashOnly) {
				// Only clash fleets in a naval war
				r.setSuccess(true);
				return rptr;
			}
			else {
				double cargoSize = cargoCap - logisticsLosses;
				report.log("Naval landing succeeds with %d surviving cargo",
						::round(cargoSize));
				if(armySize > 0.5) {
					armyPow *= std::min(1.0, cargoSize / armySize);
					cavPow *= std::min(1.0, cargoSize / armySize);
				}
				report.log("Power of landing party: %d", ::round(armyPow));
			}
		} else {
			lossesAtt = lwl.second / confAtt;
			r.reducePower(id, FLEET, lossesAtt);
			double logisticsLosses = lossesAtt * logisticsPortion;
			r.reducePower(id, LOGISTICS, logisticsLosses);
			lossesDef = lwl.first / confDef;
			r.reducePower(defender.id, FLEET, lossesDef);
			r.reducePower(defender.id, LOGISTICS,
					lossesDef * logisticsPortionTarget);
			if(!clashOnly) {
				double drowned = 0;
				if(armySize > 0.5)
					drowned = std::min(1.0, logisticsLosses / armySize) *
							armyPow;
				r.reducePower(id, ARMY, drowned);
				if(armyPow > 0.5)
					r.reducePower(id, CAVALRY, drowned * cavPow / armyPow);
				report.log("Naval landing repelled");
			}
			r.setSuccess(false);
			r.setDuration(deploymentDuration + (sneakAttack ? 0 : 2));
			return rptr;
		}
		if(lossesAtt > 0.01 || lossesDef > 0.01)
			report.log("Losses from sea battle (A/D): %d/%d",
					::round(lossesAtt), ::round(lossesDef));
	}
	// Subtract 2% per distance unit, but at most 50%
	double defDeploymentMod = std::max(100 -  2 * deploymentDistDefender, 50.0)
			/ 100;
	targetArmyPow *= defDeploymentMod;
	armyPow *= std::max(100 - 1.55 * std::pow(deploymentDistAttacker, 1.15), 50.0) / 100;
   // Units available in battle area
	CvArea* battleArea = NULL;
	if(c != NULL)
		battleArea = cvCity->area();
	else if(!isNaval) {
		battleArea = clashArea(defender.id);
		/*  This can happen when a civ has just lost its foothold in an area
			and reachability hasn't been updated yet. */
		//FAssertMsg(battleArea != NULL, "No shared area should imply isNaval");
	} // (Else only naval battle; assume that fleets are fully deployed.)
	int const remainingCitiesAtt = attCities - losses.size();
	int const remainingCitiesDef = defCities - defender.losses.size();
	double areaWeightAtt, areaWeightDef;
	areaWeightAtt = areaWeightDef = 1;
	/*  Don't base the weight on battleArea->getPower(id); too fleeting
		(and hidden knowledge unless id==weId). */
	if(battleArea != NULL) {
		if(remainingCitiesAtt > 0) {
			if(!isNaval && !clashOnly) {
				/*  Fixme(?): getCitiesPerPlayer should be reduced based on lost
					cities */
				areaWeightAtt = battleArea->getCitiesPerPlayer(id) /
						(double)remainingCitiesAtt;
				CvCity* capital = GET_PLAYER(id).getCapitalCity();
				if(capital != NULL && capital->area() == battleArea)
					areaWeightAtt *= (GET_PLAYER(id).isHuman() ? 1.5 : 1.33);
				areaWeightAtt = std::min(1.0, areaWeightAtt);
			}
			else {
				CvCity* capital = GET_PLAYER(id).getCapitalCity();
				if(capital != NULL) {
					areaWeightAtt = capital->area()->getCitiesPerPlayer(id) /
							(double)GET_PLAYER(id).getNumCities();
				}
				/*  A human naval attack focuses much of the army (if supported by
					LOGISTICS), while the AI struggles with deploying units from
					different areas. */
				if(GET_PLAYER(id).isHuman())
					areaWeightAtt += 0.25;
				areaWeightAtt = ::dRange(areaWeightAtt, 0.5, 1.0);
			}
			if(areaWeightAtt < 0.99) {
				report.log("Area weight attacker: %d percent",
						::round(areaWeightAtt * 100));
				armyPow *= areaWeightAtt;
			}
		}
		if(remainingCitiesDef > 0) {
			areaWeightDef = battleArea->getCitiesPerPlayer(defender.id) /
					(double)remainingCitiesDef;
			CvCity* capital = GET_PLAYER(defender.id).getCapitalCity();
			if(capital != NULL && capital->area() == battleArea)
				areaWeightDef *= (GET_PLAYER(defender.id).isHuman() ? 1.5 : 1.33);
			/*  Even if the local army is too small to prevent the (temporary)
				capture of a city, reinforcements will arrive before long. */
			areaWeightDef = ::dRange(areaWeightDef, 0.33, 1.0);
			if(areaWeightDef < 0.99) {
				report.log("Area weight defender: %d percent",
						::round(areaWeightDef * 100));
				targetArmyPow *= areaWeightDef;
				targetCavPow *= areaWeightDef;
			}
		}
	}
   // Combat bonuses
	/* Aggressive trait; should probably exclude cavalry in any case or check the
	   combat type of the typical army unit; tedious to implement. */
	double armyModAtt = 0, armyModDef = 0;
	if(GET_PLAYER(id).warAndPeaceAI().getCache().hasAggressiveTrait()
			&& !cavalryAttack)
		armyModAtt += 0.1;
	if(GET_PLAYER(defender.getId()).warAndPeaceAI().getCache().hasAggressiveTrait())
		armyModDef += 0.1;
	double armyModAttCorr = powerCorrect(1 + armyModAtt);
	double armyModDefCorr = powerCorrect(1 + armyModDef);
	// These bonuses are removed again (through division) when applying losses
	double armyPowMod = armyPow * armyModAttCorr * confAtt,
		   targetArmyPowMod = targetArmyPow * armyModDefCorr * confDef;
	// Two turns for the actual attack
	r.setDuration(healDuration + deploymentDuration + (sneakAttack ? 0 : 2));
	//bool defenderOutnumbered = (1.5 * targetArmyPowMod < armyPowMod);
	/*  Don't do a second clash. Can reward the attacker for having fewer units.
		Not unrealistic (a larger army can cause the defending army to dig in),
		but can lead to erratic AI decisions since I'm only considering a
		single simulation trajectory. */
	bool defenderOutnumbered = true;
	if(!defenderOutnumbered || clashOnly) {
		bool attWin = (armyPowMod > targetArmyPowMod);
		report.log("Army clash with modified power (A/D): %d/%d",
						::round(armyPowMod), ::round(targetArmyPowMod));
		std::pair<double,double> lwl = clashLossesWinnerLoser(armyPowMod,
				targetArmyPowMod, !clashOnly, false);
		double cavRatio = 0;
		double targetCavRatio = 0;
		if(armyPow > 0.5) {
			cavRatio = ::dRange(cavPow / (armyPow + military[HOME_GUARD]->power() -
					lostPower[HOME_GUARD]), 0.0, 1.0);
		}
		if(targetArmyPow > 0.5) {
			targetCavRatio = ::dRange(targetCavPow / (targetArmyPow +
					defender.military[HOME_GUARD]->power() -
					defender.lostPower[HOME_GUARD]), 0.0, 1.0);
		}
		double lossesWinner = lwl.first;
		double lossesLoser = lwl.second;
		double tempLosses = clashLossesTemporary(armyPowMod, targetArmyPowMod);
		if(attWin) {
			// Have tempLosses take effect immediately (not necessary if clashOnly)
			double unavail = lossesWinner + tempLosses;
			armyPow -= unavail;
			cavPow -= cavRatio * unavail;
			// Remember the tempLosses
			r.setTempLosses(tempLosses / armyModAttCorr);
			lossesLoser /= armyModDefCorr;
			lossesWinner /= armyModAttCorr;
			r.reducePower(id, ARMY, lossesWinner);
			r.reducePower(defender.id, ARMY, lossesLoser);
			r.reducePower(id, CAVALRY, lossesWinner * cavRatio);
			r.reducePower(defender.id, CAVALRY, lossesLoser * targetCavRatio);
			if(!clashOnly)
				report.log("Defending army defeated; losses (A/D): %d/%d",
						::round(lossesWinner), ::round(lossesLoser));
			else {
				r.setSuccess(true);
				return rptr;
			}
		}
		else {
			FAssert(armyModAttCorr > 0 && armyModDefCorr > 0);
			if(clashOnly) // No tempLosses for repelled city attack
				r.setTempLosses(tempLosses / armyModDefCorr);
			lossesLoser /= armyModAttCorr;
			lossesWinner /= armyModDefCorr;
			r.reducePower(id, ARMY, lossesLoser);
			r.reducePower(defender.id, ARMY, lossesWinner);
			r.reducePower(id, CAVALRY, lossesLoser * cavRatio);
			r.reducePower(defender.id, CAVALRY, lossesWinner * targetCavRatio);
			r.setSuccess(false);
			if(!clashOnly)
				report.log("Attack repelled by defending army; "
						"losses (A/D): %d/%d",
						::round(lossesLoser), ::round(lossesWinner));
			return rptr;
		}
	}
	else targetArmyPow /= defDeploymentMod; // Retreated army has depl. dist. 0
	/*  Attackers normally assumed to have city raider promotions;
		cavalry doesn't, but might ignore first strikes. */
	if(cavalryAttack) {
		armyModAtt -= 0.12;
		armyModAttCorr = powerCorrect(1 + armyModAtt);
	}
	/* Needs to be updated in any case in order to take into account potential
	   losses from clash. */
	armyPowMod = armyPow * armyModAttCorr * confAtt;
	FAssert(remainingCitiesDef > 0);
	/* Assume that the defenders stationed in a city are 50% static city defenders
	   and 50% floating defenders that can move to reinforce a nearby city that is
	   threatened. To this end, assume that each city has two garrisons; a
	   garrison may be just one unit or many. One garrison can move,
	   the other can't. */
	double powerPerGarrison = 0.5 * (defender.military[HOME_GUARD]->power() -
			defender.lostPower[HOME_GUARD] + defender.emergencyDefPow)
			/ remainingCitiesDef;
	int nLocalGarrisons = 2; // Could also make this fractional
	/* Extra garrisons assumed to be stationed in important cities
	   (similar to code in CvCityAI::AI_neededDefenders) */
	bool isCityImportant = cvCity->isCapital() || cvCity->isHolyCity()
			|| cvCity->hasActiveWorldWonder();
	if(isCityImportant)
		nLocalGarrisons += 2;
	/*  If a civ has only 3 cities, then the code above assigns 4 (of the 6)
		garrisons to an important city (e.g. the capital). That adds up
		(1 garrisons remains for each of the other cities), but CvCityAI doesn't
		distribute defenders quite as unevenly. */
	nLocalGarrisons = std::min(nLocalGarrisons, 2 + remainingCitiesDef / 2);
	double typicalGarrisonPow = defender.military[HOME_GUARD]->getTypicalUnitPower(outer.weId);
	if(defender.military[HOME_GUARD]->getTypicalUnit() == NULL) {
		FAssertMsg(defCities <= 0, "No typical garrison unit found");
		typicalGarrisonPow = 3.25; // That's a Warrior
	}
	// Fewer rallies if all spread thin
	int nRallied = ::round(powerPerGarrison / typicalGarrisonPow);
	// Upper bound for rallies based on importance of city
	int rallyBound = 0;
	// Population above 75% of the average
	if(cvCity->getPopulation() > 0.75 *
			GET_PLAYER(defender.id).getTotalPopulation() /
			std::max(1.0, (double)defCities))
		rallyBound = 1;
	if(isCityImportant)
		rallyBound = 2;
	if(battleArea != NULL) { /*  -1: garrison of c already counted as local.
			Fixme(?): Should subtract lost cities in battleArea. */
		rallyBound = std::min(battleArea->getCitiesPerPlayer(defender.id) - 1,
				rallyBound);
		rallyBound = std::min(remainingCitiesDef - 1, rallyBound);
		rallyBound = std::max(0, rallyBound);
	}
	else FAssert(false);
	nRallied = std::min(rallyBound, nRallied);
	if(cavalryAttack) // Swift attack
		nRallied = 0;
	int nGarrisons = nLocalGarrisons + nRallied;
	/* Example: Just 2 cities left, i.e. 4 garrisons. 1 has to stay
	   in the other city, therefore only 3 in the attacked city. */
	nGarrisons = std::min(remainingCitiesDef + 1, nGarrisons);
	// Recently conquered city likely to lack a garrison
	if(cvCity->isEverOwned(id) && cvCity->isOccupation() &&
			TEAMREF(id).isAtWar(TEAMID(defender.id)))
		nGarrisons = 1;
	nLocalGarrisons = std::min(nGarrisons, nLocalGarrisons);
	double guardPowUnmodified = nGarrisons * powerPerGarrison;
	// Only for local garrisons
	double fortificationBonus = 0.25;
	MilitaryBranch* g = defender.military[HOME_GUARD];
	bool noGuardUnit = (g->getTypicalUnit() == NULL);
	FAssert(!noGuardUnit || defCities <= 0);
	// For all garrisons
	double cityDefenderBonus = noGuardUnit ? 0 :
			g->getTypicalUnit()->getCityDefenseModifier() / 100.0;
	// Would be nicer to check all traits for defense bonuses
	if(GET_PLAYER(defender.weId).warAndPeaceAI().getCache().hasProtectiveTrait())
		cityDefenderBonus += 0.3;
	bool isGunp = military[ARMY]->getTypicalUnit() != NULL &&
			military[ARMY]->getTypicalUnit()->isIgnoreBuildingDefense();
	// Normal defensive promotions are assumed to be countered by city raider
	// For all non-mounted defenders
	double tileBonus = military[ARMY]->getTypicalUnit() == NULL ? 0 :
			cvCity->getDefenseModifier(isGunp);
	/* AI tends to run low on siege after a while. era+1 based on assumption that
	   AI tends to bring enough siege to bomb. twice per turn on average
	   (on the final turn, some siege units will also attack, i.e. can't bomb).
	   Too little? 10 * instead of 8? */
	double bombPerTurn = 8 * (GET_PLAYER(id).getCurrentEra() + 1) *
			(6.0 - conquests.size()) / 6;
	/*  Don't assume that AI will endlessly bombard.
		For a Medieval Castle backed by Chichen Itza (i.e. 125% def),
		tileBonus / bombPerTurn is about 5.2. W/o bombardment, an attack is pretty
		hopeless. Don't want such a city to completely discourage war, therefore
		set the threshold slightly higher than 5.2; at first, I had set it to 5. */
	if(bombPerTurn < 8 || tileBonus / bombPerTurn > 5.25)
		canBombard = false;
	double bombDmg = 0;
	if(canBombard) {
		bombDmg = 0.8 * tileBonus;
		// Assume that city defense still helps a little
		tileBonus -= bombDmg;
	}
	// Don't log canSoften - it's usually implied
	else report.log("Can't bomb down defenses");
	// Assume 20 damage per turn
	double bombDuration = canBombard ? bombDmg / bombPerTurn : 0;
	/* Walls and Castle slow down bombardment.
	   min(75...: A mod could grant 100% bombard defense; treat >75% as 75% */
	int bombDefPercent = std::min(75, cvCity->getBuildingBombardDefense());
	if(isGunp)
		bombDefPercent = 0;
	FAssert(bombDefPercent >= 0);
	if(bombDefPercent >= 85) { // No building does that
		bombDuration = 0;
		canBombard = false;
	}
	else bombDuration *= 100.0 / (100 - bombDefPercent);
	// Just the hill defense (help=true skips city defense)
	tileBonus += cvCity->plot()->defenseModifier(NO_TEAM, true, NO_TEAM, true);
	tileBonus /= 100.0;
	/*  Would be nice to check also if the city is enclosed by rivers, e.g. in a
		river delta, but I think this can't really happen on randomly generated maps,
		and the test could be a bit slow. */
	double localGarrisonPow = (nLocalGarrisons * powerPerGarrison
			* powerCorrect(
			1 + fortificationBonus + cityDefenderBonus + tileBonus)) * confDef;
	double ralliedGarrisonPow = ((nGarrisons - nLocalGarrisons)
			* powerPerGarrison
			* powerCorrect(1 + cityDefenderBonus + tileBonus)) * confDef;
	double defendingArmyPow = 0;
	double defArmyPortion = std::min(1.0, (sneakAttack ? 0.375 : 0.5) /
			// More units rallied at the first few cities that are attacked
			std::sqrt((defender.losses.size() + 1.0)) + nRallied * 0.25);
	if(remainingCitiesDef == 1)
		defArmyPortion = 1;
	else if(!defender.hasClashed) {
		/*  If their army hasn't been engaged yet, expect the bulk of it to be
			rallied to any attacked city */
		defArmyPortion = std::max(defArmyPortion, 0.75);
		report.log("Assuming high portion of defending army b/c not clashed yet");
	}
	if(defenderOutnumbered)
		defendingArmyPow = (targetCavPow + (targetArmyPow - targetCavPow) *
				powerCorrect(1 + tileBonus + armyModDef)) * confDef *
				defArmyPortion;
	double defenderPow = localGarrisonPow + ralliedGarrisonPow
			+ defendingArmyPow;
	report.log("City defender power: %d (%d from local garrisons, "
			   "%d from rallied garrisons, %d from retreated army"
			   " (%d percent))",
			::round(defenderPow), ::round(localGarrisonPow),
			::round(ralliedGarrisonPow), ::round(defendingArmyPow),
			::round(100 * defArmyPortion));
	report.log("Besieger power: %d", ::round(armyPowMod));
	double powRatio = armyPowMod / std::max(1.0, defenderPow);
	double threat = powRatio;
	report.log("Power ratio (A/D): %d percent", ::round(100 * powRatio));
	/* Attacks on important cities may result in greater distraction for
	   the defender -- or perhaps not; attacks on remote cities could
	   be equally distracting ... */
	//if(isCityImportant) threat *= 1.5;
	r.setThreat(threat);
	double delta = armyPowMod - defenderPow;
	/* Add extra turns for coordinating city raiders and siege. Even with ships
	   or aircraft, it takes (the AI) some effort, and in the lategame
	   deploymentDuration should be small; hopefully, not special treatment
	   needed. */
	if(canBombard || canSoften)
		bombDuration += deploymentDuration * 0.2;
	int bombDurationRounded = ::round(bombDuration / std::max(0.75, powRatio));
	if(bombDurationRounded > 0)
		report.log("Bombardment assumed to take %d turns",
				bombDurationRounded);
	// Faster conquest when defenders outnumbered
	r.setDuration(bombDurationRounded + r.getDuration());
	FAssert(armyModAttCorr > 0 && confAtt > 0);
	if(delta > 0) {
		/*  Full losses for defender would be realistic, but doesn't capture the
			uncertainty of success if threat is near 100%. Therefore: */
		double defLossRatio = std::min(1.0, 0.65 * std::pow(threat, 1.5));
		r.reducePower(defender.id, ARMY, defLossRatio * targetArmyPow * defArmyPortion);
		r.reducePower(defender.id, CAVALRY, defLossRatio * targetCavPow * defArmyPortion);
		/* Assume that not all emergency defenders were able to reach this city
		   (i.e. their true power is actually 2 * emergencyDefPow) */
		defender.emergencyDefPow /= 2;
		r.reducePower(defender.id, HOME_GUARD, defLossRatio * guardPowUnmodified);
		/*  These losses are a bit exaggerated I think, at least when threat is
			near 1. Deliberate, in order to account for uncertainty. */
		double attLossRatio = 0.55 / std::pow(threat, 0.75);
		double lossesAttArmy = (attLossRatio * armyPowMod / armyModAttCorr) / confAtt;
		/* Assume less impact of softening when already some cities conquered
		   b/c AI tends to run out of/ low on siege units after a while.
		   Reduce losses to 12/18 (66%) for the first city, then 13/18 etc. */
		if(canSoften)
			lossesAttArmy *= std::min(1.0, (12.0 + conquests.size()) / 18);
		r.reducePower(id, ARMY, lossesAttArmy);
		if(armyPow > 0.5)
			r.reducePower(id, CAVALRY, lossesAttArmy * cavPow / armyPow);
		r.setSuccess(true);
	}
	else {
		/*  Few losses for city defenders (can heal and reinforce quickly).
			A multiplier of 0.4 might be realistic (0.3 for army); use higher values
			to account (at least a bit) for the possibility of losing the city. */
		r.reducePower(defender.id, HOME_GUARD, guardPowUnmodified * 0.75 * threat);
		// Few losses for defending army
		if(defenderOutnumbered &&
				armyPowMod > localGarrisonPow + ralliedGarrisonPow) {
			double lossesDefArmy = targetArmyPow * defArmyPortion * 0.50 * threat;
			r.reducePower(defender.id, ARMY, lossesDefArmy);
			if(targetArmyPow > 0.5)
				r.reducePower(defender.id, CAVALRY, lossesDefArmy * defArmyPortion *
						targetCavPow / targetArmyPow);
		}
		/* Attacker may lose nothing b/c city is so strongly defended that no
		   attack is attempted; could also have heavy losses. I don't think this
		   can be foreseen; go with an optimistic outcome (expected could be
		   perhaps 35% losses). Need to avoid cases where a greater army brings
		   the attacker greater losses (by winning the initial clash, but not
		   taking a city) as this can lead to counterintuitive AI decisions.
		   Also, whether an attack happens in the simulation depends on the
		   time horizon; timing quirks mustn't have a major impact on the outcome. */
		double lossesAttArmy = ((0.2 * std::min(armyPowMod, defenderPow)) /
				armyModAttCorr) / confAtt;
		r.reducePower(id, ARMY, lossesAttArmy);
		if(armyPow > 0.5)
			r.reducePower(id, CAVALRY, lossesAttArmy * cavPow / armyPow);
		r.setSuccess(false);
	}
	return rptr;
}

bool InvasionGraph::Node::canReachByLand(int cityId) const {

	// Allow weId to magically know whether other civs can reach a city by land
	WarAndPeaceCache::City* c = cache.lookupCity(cityId);
	if(c == NULL) // Then they don't even know where it is
		return false;
	return c->canReachByLand() && (c->getDistance() <= getWPAI.maxLandDist() ||
			!cache.canTrainDeepSeaCargo());
}

CvArea* InvasionGraph::Node::clashArea(PlayerTypes otherId) const {

	// Lots of const here b/c I'm a bit worried about performance
	CvPlayer const& civ1 = GET_PLAYER(id);
	CvPlayer const& civ2 = GET_PLAYER(otherId);
	/*  For better performance, treat the very common case of a common
		capital area upfront. */
	CvCity* cap1 = civ1.getCapitalCity();
	if(cap1 != NULL) {
		CvCity* cap2 = civ2.getCapitalCity();
		CvArea* const r = cap1->area();
		if(cap2 != NULL && r == cap2->area())
			return r;
	}
	CvArea* r = NULL;
	int maxCities = 0;
	// Going through cities should be faster than going through all areas
	CvPlayer const& fewerCitiesCiv = (civ1.getNumCities() < civ2.getNumCities() ?
			civ1 : civ2); int dummy=-1;
	for(CvCity* c = fewerCitiesCiv.firstCity(&dummy); c != NULL;
			c = fewerCitiesCiv.nextCity(&dummy)) {
		CvArea* const a = c->area();
		int citiesMin = std::min(a->getCitiesPerPlayer(civ1.getID()),
				a->getCitiesPerPlayer(civ2.getID()));
		if(citiesMin <= 0)
			continue;
		if(c->isCapital()) {
			if(c->getOwner() == civ1.getID())
				citiesMin++;
			if(c->getOwner() == civ2.getID())
				citiesMin++;
		}
		if(citiesMin > maxCities) {
			maxCities = citiesMin;
			r = a;
		}
	}
	return r;
}

void InvasionGraph::Node::applyStep(SimulationStep const& step) {

	Node& attacker = *outer.nodeMap[step.getAttacker()];
	bool reportedLosses = false;
	for(int i = 0; i < NUM_BRANCHES; i++) {
		MilitaryBranchTypes mb = (MilitaryBranchTypes)i;
		double lpa = step.getLostPower(attacker.id, mb);
		double lpd = step.getLostPower(id, mb);
		if(::round(lpa) > 0 || ::round(lpd) > 0) {
			report.log("Losses in branch %s: %d (%s), %d (%s)",
					military[mb]->str(),
					::round(lpa), report.leaderName(attacker.id),
					::round(lpd), report.leaderName(id));
			reportedLosses = true;
		}
		lostPower[i] += lpd;
		lostPower[i] = std::min(lostPower[i], military[mb]->power());
		attacker.lostPower[i] += lpa;
		attacker.lostPower[i] = std::min(attacker.lostPower[i],
				attacker.military[mb]->power());
	}
	// Remembered until end of phase
	double tempLosses = step.getTempLosses();
	if(step.isAttackerSuccessful())
		attacker.tempArmyLosses += tempLosses;
	else if(step.isClashOnly())
		tempArmyLosses += tempLosses;
	if(tempLosses > 0.5) {
		report.log("Temporary army losses (damaged): %d", ::round(tempLosses));
		reportedLosses = true;
	}
	if(!reportedLosses)
		report.log("(no loss of milit. power on either side)");
	if(step.isAttackerSuccessful()) {
		if(step.isClashOnly())
			changePrimaryTarget(NULL);
		else {
			WarAndPeaceCache::City const& c = *step.getCity();
			addLoss(c);
			int nCurrentActualCities = GET_PLAYER(id).getNumCities();
			if(losses.size() == nCurrentActualCities)
				setEliminated(true);
			/* Assume that own offensive stops upon losing a city to a third civ,
			   and whole army becomes available for defense.
			   distractionByDefense stays in effect for the whole
			   simulation stage. The distraction staying constant
			   already implies that losses on the offensive front are recuperated
			   with units from the defensive front over time. */
			distractionByConquest = 0;
			/* This value is currently not read. See comment in resolveLosses.
			   The code here only counts time spent on successful city attacks;
			   might also want to count failed attacks or clashes. */
			attacker.warTimeSimulated += step.getDuration();
			attacker.addConquest(c);
			// The conquerer leaves part of his army behind to protect the city
			int nUnitsLeftBehind = 2 + GET_PLAYER(id).getCurrentEra();
			if(GET_PLAYER(attacker.id).isHuman())
				nUnitsLeftBehind = (2 * nUnitsLeftBehind) / 3;
			double powLeftBehind = military[ARMY]->getTypicalUnitPower(outer.weId)
					* nUnitsLeftBehind;
			powLeftBehind = std::min(powLeftBehind, attacker.military[ARMY]->power()
					- attacker.lostPower[ARMY]);
			report.log("%d army power assumed to be left behind for defense",
					::round(powLeftBehind));
			attacker.lostPower[ARMY] += powLeftBehind;
			/* Don't want to add the power to guard b/c the newly conquered city
			   can't be attacked by third parties (nor reconquered) and doesn't
			   count when computing garrison strength. However, mustn't treat
			   units left behind as losses; record them in shiftedPower. */
			attacker.shiftedPower[ARMY] += powLeftBehind;
			CvTeamAI const& attackerTeam = TEAMREF(attacker.id);
			CvTeamAI const& nodeTeam = TEAMREF(id);
			if(!isEliminated() && !nodeTeam.isAVassal() &&
					!attackerTeam.isAVassal() &&
					attackerTeam.isVassalStateTrading() &&
					!GC.getGame().isOption(GAMEOPTION_NO_VASSAL_STATES) &&
					// Don't expect node to capitulate so long as it has vassals
					nodeTeam.getVassalCount(TEAMID(outer.m.ourId())) <= 0 &&
					GET_PLAYER(attacker.id).warAndPeaceAI().getConquestStage() <=
					GET_PLAYER(attacker.id).warAndPeaceAI().getDominationStage()) {
				int nConqueredByAtt = 0;
				for(size_t i = 0; i < attacker.conquests.size(); i++) {
					if(attacker.conquests[i] != NULL &&
							losses.count(attacker.conquests[i]->id()) > 0)
						nConqueredByAtt++;
				}
				int nConqueredByOther = (int)losses.size() - nConqueredByAtt;
				FAssert(nConqueredByOther >= 0);
				/*  If at war, attacker may already have war successes outside
					the simulation that could scare this node into capitulation. */
				bool bWar = attackerTeam.isAtWar(TEAMID(id));
				if(!GET_PLAYER(id).isHuman() &&
						// Last team member standing
						TEAMREF(id).getAliveCount() <= 1 &&
						GET_PLAYER(id).getNumNukeUnits() <= 0 && // advc.143b
						/*  To decide whether to capitulate to a given team, we need
							to know what will happen if we don't capitulate. */
						(id != weId || TEAMID(attacker.id) !=
						outer.m.evaluationParameters().getCapitulationTeam()) &&
						nConqueredByAtt >= std::max(nConqueredByOther + 1,
						(bWar ? 1 : 2))) {
					double powModPercent = GC.getLeaderHeadInfo(GET_PLAYER(id).
							getPersonalityType()).getVassalPowerModifier();
					bool powModNegative = (powModPercent < 0);
					powModPercent = (powModNegative ? -1 : 1) *
							std::pow((powModNegative ? -1 : 1) *
							powModPercent , 0.75);
					/*  Need to conquer about half of the cities; a bit fewer if
						there are a lot of them. */
					double thresh = std::pow((double)nCurrentActualCities -
							nConqueredByOther, 0.95) * std::max(36.0,
							55 + (bWar ? -10 : 0) + powModPercent) / 100.0;
					if(nConqueredByAtt >= ::round(thresh)) {
						setCapitulated(TEAMID(attacker.id));
						report.log("%s has *capitulated* to %s", report.leaderName(id),
								report.leaderName(attacker.id));
					}
				}
			}
			/* Assume that some defenders are built upon the loss of a city.
			   Defenders built while the war is conducted are generally
			   covered by ArmamentForecast, including defenders hurried just prior
			   to an attack, but not emergency defenders built while the invading
			   army heals and approaches its next target. */
			if(!isEliminated()) {
				double div = 3.0;
				if(GET_PLAYER(id).getMaxConscript() > 0)
					div = 2.0;
				emergencyDefPow += military[HOME_GUARD]->getTypicalUnitPower(outer.weId)
						* (step.getDuration() / div);
				report.log("Emergency defender power for %s: %d",
						report.leaderName(id), ::round(emergencyDefPow));
			}
		}
	}
	// Remove link from graph
	else attacker.changePrimaryTarget(NULL);
	// Remove link if army eliminated
	PlayerTypes armyElimId = NO_PLAYER;
	if(military[ARMY]->power() <= lostPower[ARMY] && primaryTarget != NULL) {
		armyElimId = id;
		changePrimaryTarget(NULL);
	}
	if(attacker.military[ARMY]->power() <= attacker.lostPower[ARMY] &&
			attacker.primaryTarget != NULL) {
		armyElimId = attacker.id;
		attacker.changePrimaryTarget(NULL);
	}
	if(armyElimId != NO_PLAYER)
		report.log("Army of %s eliminated", report.leaderName(armyElimId));
	attacker.hasClashed = true;
	if(step.isClashOnly() || step.isAttackerSuccessful())
		hasClashed = true;
	/*  Don't set hasClashed if the attack fails b/c then the attack may have been
		very minor; could overstate the distractive effect of some small war party */
}

void InvasionGraph::Node::setEliminated(bool b) {

	eliminated = b;
}

bool InvasionGraph::Node::isEliminated() const {

	return eliminated;
}

void InvasionGraph::Node::setCapitulated(TeamTypes masterId) {

	capitulated = true;
	/* Treat capitulated as eliminated, i.e. assume no further losses.
	   According to the actual rules, the vassal immediately joins the
	   wars of its master, but that gets too complicated. */
	setEliminated(true);
	for(int i = 0; i < MAX_PLAYERS; i++) {
		if(outer.nodeMap[i] == NULL || i == id || outer.nodeMap[i]->isEliminated())
			continue;
		TeamTypes tId = (TeamTypes)TEAMID((PlayerTypes)i);
		if(tId == masterId)
			outer.nodeMap[i]->capitulationsAccepted.insert(TEAMID(id));
		else if(tId == TEAMID(id)) {
			bool const hasCap = outer.nodeMap[i]->hasCapitulated();
			if(hasCap) { // Make sure not to produce a stack overflow
				FAssert(!hasCap);
				continue;
			}
			outer.nodeMap[i]->setCapitulated(masterId);
		}
	}
}

bool InvasionGraph::Node::hasCapitulated() const {

	return capitulated;
}

WarAndPeaceCache::City const* InvasionGraph::Node::targetCity(
		PlayerTypes owner) const {

	if(primaryTarget == NULL && owner == NO_PLAYER)
		return NULL;
	for(int i = cacheIndex; i < cache.size(); i++) {
		WarAndPeaceCache::City* r = cache.getCity(i);
		if(r == NULL)
			continue;
		if(!r->canReach() || r->isOwnCity())
			break; // Because of sorting, rest is also going to be invalid.
		PlayerTypes const cityOwner = r->cityOwner();
		if((cityOwner == owner ||
				(owner == NO_PLAYER && cityOwner == primaryTarget->getId())) &&
				/* Target may also have conquered the city; however,
				   cities being won and lost within one military analysis
				   gets too complicated. */
				(primaryTarget == NULL || !primaryTarget->hasLost(r->id())))
			return r;
	}
	return NULL;
}

void InvasionGraph::Node::resolveLosses() {

	report.log("*Resolving losses of %s*", report.leaderName(getId()));
	if(targetedBy.empty()) {
		report.log("Targeted by no one - no losses");
		return;
	}
	SimulationStep* steps[MAX_PLAYERS];
	int turnsSimulated[MAX_PLAYERS];
	bool isTargetting[MAX_PLAYERS];
	for(int i = 0; i < MAX_PLAYERS; i++) {
		steps[i] = NULL;
		turnsSimulated[i] = 0;
		isTargetting[i] = false;
	}
	/* Reduces distractionByDefense to 70%. Otherwise it would be implied that
	   losses incurred from an attack by a third power
	   lead to potential attackers closing the gap at that front.
	   While the UnitAI might do that, accounting for this on the defensive front
	   gets too complicated. Counting the distraction only partially seems
	   like a reasonable approximation. */
	double const invDistrMod = 0.7;
	int timeLimit = outer.timeLimit;
	// Threat that carries over from one iteration to the next
	double pastThreat = 0;
	do {
		for(set<PlayerTypes>::iterator it = targetedBy.begin();
				it != targetedBy.end(); it++) {
			InvasionGraph::Node& inv = *outer.nodeMap[*it];
			report.log("Assessing invasion priority (att. duration) of %s",
					report.leaderName(inv.getId()));
			report.setMute(true);
			steps[*it] = outer.nodeMap[*it]->step(0,
					1 - (invDistrMod * inv.distractionByDefense));
			report.setMute(false);
			if(steps[*it] != NULL)
				report.log("Duration at least %d", steps[*it]->getDuration());
			else report.log("Can't reach any city");
			isTargetting[*it] = true;
		}
		int shortestDuration = std::numeric_limits<int>::max();
		PlayerTypes nextInvader = NO_PLAYER;
		double nextInvaderThreat = -1;
		double sumOfThreats = 0;
		for(int i = 0; i < MAX_PLAYERS; i++) {
			if(steps[i] == NULL) {
				/* Could remove the NULL entries during the iteration above
				   (more efficient), but modifying a container during
				   iteration is a can of worms.
				   NULL means the invader can't pick a target city (none left
				   or none revealed on the map). NB: applyStep may also erase
				   Nodes from targetedBy. */
				if(isTargetting[i]) {
					outer.nodeMap[i]->changePrimaryTarget(NULL);
					isTargetting[i] = false;
				}
				continue;
			}
			else {
				sumOfThreats += steps[i]->getThreat();
				int duration = steps[i]->getDuration() + turnsSimulated[i];
				if(duration < shortestDuration) {
					shortestDuration = duration;
					nextInvader = (PlayerTypes)i;
					nextInvaderThreat = steps[i]->getThreat();
				}
			}
		}
		/* Steps so far are preliminary, ignoring the defending army.
		   Need those steps only to determine how the defenders split up
		   against several invaders, and which invader's step is
		   resolved next. */
		for(int i = 0; i < MAX_PLAYERS; i++)
			SAFE_DELETE(steps[i]);
		/* Shortest duration is an optimistic lower bound (ignoring defending
		   army). If the shortest duration exceeds the time limit, all actual
		   durations are going to exceed the time limit as well. */
		if(nextInvader == NO_PLAYER || sumOfThreats < 0.001)
			break;
		if(timeLimit >= 0 && shortestDuration > timeLimit) {
			report.log("All steps exceed the time limit; none executed");
			break;
		}
		InvasionGraph::Node& inv = *outer.nodeMap[nextInvader];
		SimulationStep& nextStep = *inv.step(nextInvaderThreat /
				(sumOfThreats + pastThreat +
				// Multiplicative distractionByConquest instead of additive?
				distractionByConquest),
				1 - (invDistrMod * inv.distractionByDefense));
		int actualDuration = nextStep.getDuration();
		// Don't apply step if it would exceed the time limit
		if(timeLimit < 0 || turnsSimulated[nextInvader] + actualDuration
				/* Per-node time limit. Not sure if really needed. Would have to
				   toggle isTargeting and unset primaryTarget as well in the
				   else branch (instead of breaking). */
				//+ (*outer.nodeMap)[nextInvader].warTimeSimulated
				<= timeLimit) {
			applyStep(nextStep);
			/* If attack fails, the attacker doesn't get another step. However,
			   the defending army portion assigned to that attacker mustn't
			   be assumed to instantaneously redeploy to another front. */
			if(!nextStep.isAttackerSuccessful())
				pastThreat = nextInvaderThreat;
			else pastThreat = 0;
			turnsSimulated[nextInvader] += actualDuration;
			report.log("Now simulated %d invasion turns of %s",
					turnsSimulated[nextInvader],
					report.leaderName(nextInvader));
			delete &nextStep;
		}
		else {
			delete &nextStep;
			report.log("Simulation step exceeds time limit; not executed");
			break;
		}
	} while(!isEliminated());
}

void InvasionGraph::Node::changePrimaryTarget(Node* newTarget) {

	Node* oldTarget = primaryTarget;
	primaryTarget = newTarget;
	if(newTarget != NULL) {
		newTarget->targetedBy.insert(id);
		report.log("%s now targets %s", report.leaderName(id),
				report.leaderName(newTarget->getId()));
	}
	if(oldTarget != NULL) {
		oldTarget->targetedBy.erase(id);
		report.log("%s no longer targets %s", report.leaderName(id),
				report.leaderName(oldTarget->getId()));
	}
}

bool InvasionGraph::Node::isComponentDone() const {

	return componentDone;
}

bool InvasionGraph::Node::hasLost(int cityId) const {

	return losses.count(cityId) > 0;
}

InvasionGraph::Node& InvasionGraph::Node::findSink() {

	if(!hasTarget())
		return *this;
	return primaryTarget->findSink();
}

void InvasionGraph::Node::clash(double armyPortion1, double armyPortion2) {

	Node& n1 = *this;
	Node& n2 = *n1.primaryTarget;
	/* Handling the clash in 'step' is a bit of a hack. It turned out that
	   'clash' would have a lot of overlap with 'step'. 'clash' should be
	   symmetrical, but, the way it's now implemented, step needs to be called
	   on n2 and applied on n1.
	   I've considered resolving clashes only as part of city attacks,
	   but want to resolve the clash first in order to give third parties
	   a chance to conquer cities (pre-empting the winner of the clash). */
	SimulationStep& clashStep = *n2.step(armyPortion1, armyPortion2, true);
	n1.applyStep(clashStep);
	/* Army surviving clash and going after cities won't be available for
	   defense. */
	if(clashStep.isAttackerSuccessful()) {
		n2.distractionByConquest = armyPortion1;
		distractionByDefense = armyPortion2;
	}
	else {
		distractionByConquest = armyPortion2;
		n2.distractionByDefense = armyPortion1;
	}
	delete &clashStep;
}

set<PlayerTypes> const& InvasionGraph::Node::getTargetedBy() const {

	return targetedBy;
}

InvasionGraph::Node* InvasionGraph::Node::getPrimaryTarget() const {

	return primaryTarget;
}

double InvasionGraph::Node::clashDistance(InvasionGraph::Node const& other) const {

	WarAndPeaceCache::City const* c1 = targetCity();
	WarAndPeaceCache::City const* c2 = other.targetCity();
	// Clash half-way in the middle
	if(c1 != NULL && c2 != NULL)
		return (c1->getDistance() + c2->getDistance()) / 2.0;
	else {
		FAssertMsg(false, "Shouldn't clash if not mutually reachable");
		return -1;
	}
}

bool InvasionGraph::Node::isSneakAttack(InvasionGraph::Node const& other,
		bool bClash) const {

	if((id != weId && (!bClash || other.id != weId)) || TEAMREF(id).isAtWar(TEAMID(other.getId())))
		return false;
	if(!bClash) {
		for(size_t i = 0; i < conquests.size(); i++)
			if(conquests[i]->cityOwner() == other.getId())
				return false;
	}
	WarEvalParameters const& params = outer.m.evaluationParameters();
	// Can't ready our units then
	return !params.isImmediateDoW();
}

bool InvasionGraph::Node::isContinuedWar(Node const& other) const {

	return TEAMREF(id).isAtWar(TEAMID(other.getId())) && conquests.empty();
}

double InvasionGraph::Node::powerCorrect(double multiplier) {

	return std::pow(multiplier, (double)GC.getPOWER_CORRECTION());
}

double InvasionGraph::Node::getLostPower(MilitaryBranchTypes mb) const {

	return lostPower[mb] - shiftedPower[mb];
}

double InvasionGraph::Node::getGainedPower(MilitaryBranchTypes mb) const {

	return military[mb]->power() - currentPow[mb];
}

double InvasionGraph::Node::getProductionInvested() const {

	return productionInvested;
}

void InvasionGraph::Node::getConquests(std::set<int>& r) const {

	for(size_t i = 0; i < conquests.size(); i++) {
		 if(conquests[i] == NULL)
			 continue;
		 FAssert(conquests[i]->city() != NULL);
		 r.insert(conquests[i]->id());
	}
}

void InvasionGraph::Node::getLosses(std::set<int>& r) const {

	for(set<int>::const_iterator it = losses.begin(); it != losses.end(); it++)
		r.insert(*it);
}

void InvasionGraph::Node::getCapitulationsAccepted(std::set<TeamTypes>& r) const {

	for(set<TeamTypes>::const_iterator it = capitulationsAccepted.begin();
			it != capitulationsAccepted.end(); it++)
		r.insert(*it);
}

void InvasionGraph::simulate(int duration) {

	PROFILE_FUNC();
	/* Build-up assumed to take place until 50% into the war. Use the result for
	   the simulation of losses. Maybe should simulate the timing of reinforcements
	   being deployed in more detail. As it is, the initial assault is overestimated
	   and subsequent steps underestimated. */
	int t1 = ::round(0.5 * duration);
	int t2 = duration - t1;
	for(int i = 0; i < MAX_PLAYERS; i++) {
		if(nodeMap[i] != NULL) {
			nodeMap[i]->logTypicalUnits();
			nodeMap[i]->prepareForSimulation();
		}
	}
	report.log("Simulating initial build-up (%d turns)", t1);
	// Assume that upgrades are done in the prolog, if any, and only in phase I.
	simulateArmament(t1, !firstSimulateCall);
	firstSimulateCall = false;
	// Simulation of losses over the whole duration
	timeLimit = duration;
	simulateLosses();
	if(allWarPartiesKnown)
		lossesDone = true;
	/* Needed for estimating the war effort. Assumes reduced production output if
	   cities have been lost. */
	report.log("Simulating concurrent build-up (%d turns)", t2);
	report.setMute(true); // Some more build-up isn't very interesting
	simulateArmament(t2, true);
	report.setMute(false);
}

void InvasionGraph::simulateArmament(int duration, bool noUpgrading) {

	PROFILE_FUNC();
	for(int i = 0; i < MAX_PLAYERS; i++)
		if(nodeMap[i] != NULL)
			nodeMap[i]->predictArmament(duration, noUpgrading);
}

void InvasionGraph::simulateLosses() {

	PROFILE_FUNC();
	if(nodeMap[weId] != NULL)
		simulateComponent(*nodeMap[weId]);
	/* Start with our component, but simulate the other ones as well.
	   While they can't affect the outcome of our wars, anticipating
	   conquests of other civs is going to be helpful for war evaluation. */
	for(int i = 0; i < MAX_PLAYERS; i++) {
		if(nodeMap[i] != NULL && !nodeMap[i]->isComponentDone())
			simulateComponent(*nodeMap[i]);
	}
}

void InvasionGraph::simulateComponent(Node& start) {

	/* Key observation about invasion graphs: Each component contains
	   at most one cycle. The cycle often consists of just two civs targeting
	   each other, but it could be a triangle as well, or a more complex
	   polygon. The cycle property is a consequence of each node having an
	   out-degree of at most 1. Around the central cycle, there can be trees,
	   each rooted at a node within the cycle. (The edges point towards
	   the root.) If an edge is removed from the cycle, the remaining dag
	   has a single sink (out-degree 0). */
	report.log("Simulating graph component starting at %s",
			report.leaderName(start.getId()));
	if(start.isIsolated())
		report.log("Isolated node; nothing to do for this component");
	vector<Node*> forwardPath;
	size_t startOfCycle = start.findCycle(forwardPath);
	vector<Node*> cycle;
	cycle.insert(cycle.begin(), forwardPath.begin() + startOfCycle,
			forwardPath.end());
	if(cycle.size() == 1) {
		FAssert(cycle.size() != 1);
		return;
	}
	if(!cycle.empty()) {
		report.log("*Cycle*");
		string msg = "";
		for(size_t i = 0; i < cycle.size(); i++) {
			msg += report.leaderName(cycle[i]->getId());
			msg += " --> ";
		}
		report.log("%s\n", msg.c_str());
		breakCycle(cycle);
	}
	start.findSink().resolveLossesRec();
	report.log("");
}

void InvasionGraph::breakCycle(vector<Node*> const& cyc) {

	report.log("Breaking a cycle of length %d", cyc.size());
	/* Treat the simplest case separately: two nodes targetting each other, and
	   no further nodes involved. */
	if(cyc.size() == 2 && cyc[0]->getTargetedBy().size() == 1 &&
			cyc[1]->getTargetedBy().size() == 1) {
		cyc[0]->clash(1, 1);
		return;
		/* The code below could handle this case as well, but leads to
		   unnecessary computations and logging. */
	}
	// Only for cycles of length > 2
	int toughestEnemies_cycIndex = -1;
	double highestEnemyThreat = -1;
	// Only for length 2
	double armyPortion1, armyPortion2;
	for(size_t i = 0; i < cyc.size(); i++) {
		Node& n = *cyc[i];
		double threatOfAttackFromCycle = -1;
		double sumOfEnemyThreat = 0;
		/* The targeting Nodes may themselves be busy fending off attackers.
		   This is taken into account when resolving losses, but not when
		   breaking cycles. */
		set<PlayerTypes> const& targetedBy = n.getTargetedBy();
		for(set<PlayerTypes>::const_iterator it = targetedBy.begin();
				it != targetedBy.end(); it++) {
			const char* nodeName = report.leaderName(n.getId());
			report.log("Assessing threat of %s's army to %s's garrisons "
					   "(ignoring army of %s)",
					report.leaderName(nodeMap[*it]->getId()),
					nodeName, nodeName);
			report.setMute(true);
			SimulationStep* ss = nodeMap[*it]->step();
			report.setMute(false);
			if(ss == NULL) {
				/*  Might be fine in some circumstances, but suggests inconsistent
					results of the targetCity and findTarget function. The former
					relies on the order of the cache. */
				FAssert(ss != NULL);
				// Call 'step' again for investigation in the debugger
				FAssert(nodeMap[*it]->step() != NULL);
				continue;
				/*  Not sure how to handle it gracefully when there's no threat from
					_any_ node targeting n, i.e. when step returns NULL for all *it. */
			}
			double baseThreat = ss->getThreat();
			double thr = baseThreat * willingness(nodeMap[*it]->getId(), n.getId());
			if(std::abs(thr - baseThreat) > 0.005)
				report.log("Threat set to %d/%d percent (base/modified by willingness)",
						::round(100 * baseThreat), ::round(100 * thr));
			else report.log("Threat set to %d percent", ::round(100 * thr));
			/* Whether power ratios or power values are used, makes no
			   difference in this context. Ratios are convenient b/c there's
			   a function for those anyway. */
			sumOfEnemyThreat += thr;
			// Only relevant for cyc.size == 2
			if(ss->getAttacker() == n.getPrimaryTarget()->getId())
				threatOfAttackFromCycle = thr;
			delete ss;
		}
		if(sumOfEnemyThreat > highestEnemyThreat) {
			highestEnemyThreat = sumOfEnemyThreat;
			toughestEnemies_cycIndex = i;
		}
		/* Only relevant for cyc.size == 2. Two civs target each other, and
		   others may intefere from the outside. Army portions available for
		   clash depend on these outside threats. */
		FAssert(sumOfEnemyThreat + 0.001 > threatOfAttackFromCycle);
		// 1.25: Prioritize primary target
		double armyPortion = 1.25 * threatOfAttackFromCycle / sumOfEnemyThreat;
		armyPortion = std::min(1.0, armyPortion);
		if(i == 0) armyPortion1 = armyPortion;
		if(i == 1) armyPortion2 = armyPortion;
	}
	if(cyc.size() > 2) {
		Node& toughestEnemiesNode = *cyc[toughestEnemies_cycIndex];
		/* Break the cycle by removing the target of the most besieged node.
		   Rationale: This node is likely to need all its units for defending
		   itself, or at least likelier than any other node in the cycle. */
		toughestEnemiesNode.changePrimaryTarget(NULL);
	}
	else cyc[0]->clash(armyPortion1, armyPortion2);
}

/*  WarAndPeaceAI::Civ::warConfidenceAllies is about our general confidence in
	war allies, regardless of circumstances. This function is about gauging
	rationally whether a war party ('agg') is willing to commit resources
	against another ('def').
	This is to make sure that the AI can't be goaded into a joint war by a human,
	and that joint wars are generally expensive enough. */
double InvasionGraph::willingness(PlayerTypes agg, PlayerTypes def) const {

	if(m.evaluationParameters().getSponsor() != agg)
		return 1;
	return dRange((TEAMREF(agg).AI_getWarSuccess(TEAMID(def)) +
			TEAMREF(def).AI_getWarSuccess(TEAMID(agg))) /
			(TEAMREF(agg).getNumCities() * 4.0), 0.5, 1.0);
}

SimulationStep::SimulationStep(PlayerTypes attacker,
		WarAndPeaceCache::City const* contestedCity)
	: attacker(attacker), contestedCity(contestedCity) {

	duration = 0;
	threat = 0;
	tempLosses = 0;
	for(int i = 0; i < NUM_BRANCHES; i++)
		lostPowerAttacker[i] = lostPowerDefender[i] = 0;
	success = false;
}

void SimulationStep::setDuration(int duration) {

	this->duration = duration;
}

void SimulationStep::setThreat(double threat) {

	this->threat = threat;
}

void SimulationStep::reducePower(PlayerTypes id, MilitaryBranchTypes mb,
		double subtrahend) {

	bool ofAttacker = (id == attacker);
	if(ofAttacker)
		lostPowerAttacker[mb] += subtrahend;
	else lostPowerDefender[mb] += subtrahend;
	FAssert(lostPowerAttacker[mb]>-0.001 && lostPowerDefender[mb]>-0.001);
}

void SimulationStep::setSuccess(bool b) {

	success = b;
}

int SimulationStep::getDuration() const {

	return duration;
}

double SimulationStep::getThreat() const {

	return threat;
}

double SimulationStep::getLostPower(PlayerTypes id,
		MilitaryBranchTypes mb) const {

	bool ofAttacker = (id == attacker);
	if(ofAttacker)
		return lostPowerAttacker[mb];
	return lostPowerDefender[mb];
}

bool SimulationStep::isAttackerSuccessful() const {

	return success;
}

bool SimulationStep::isClashOnly() const {

	return contestedCity == NULL;
}

PlayerTypes SimulationStep::getAttacker() const {

	return attacker;
}

WarAndPeaceCache::City const* SimulationStep::getCity() const {

	return contestedCity;
}

void SimulationStep::setTempLosses(double d) {

	tempLosses = d;
}

double SimulationStep::getTempLosses() const {

	return tempLosses;
}

/* Underlying model: Only a portion of the invaders clashes - clashPortion -,
   the rest arrive in time for siege, but not for a clash.
   Moreover, the less powerful side will avoid a clash with probability
   equal to the power ratio; assume average-size forces by multiplying by the
   power ratio.
   Assume that the outcome of the actual clash depends only on the power values,
   not on which side has more advanced units. Can then assume w.l.o.g. that
   all units are equal and that each unit of power corresponds to one combat unit.
   Assume that each unit of the losing side fights once, leading to equal losses
   of 0.5 * lowerPow on both sides. The additional units of the winning side then
   manage to attack the damaged surviving units a few times more, leading
   to additional losses only for the losing side. Assume that half of the additional
   units manage to make such an additional attack.
   After some derivation, this result in the following formulas:
   lossesWinner = 0.5 * clashPortion * lesserPow^2 / greaterPow
   lossesLoser = 0.5 * clashPortion * lesserPow.
   att=true means that the clash happens as part of an attack, i.e. near a city
   of the defender. In this case, if the attacker loses, higher losses are assumed
   b/c it's difficult to withdraw from hostile territory. Assume that a clash is
   1.6 times as likely (i.e. more units assumed to clash) as in the att=false case
   - the attacker can't easily backup. This assumption leads to higher losses for
   both sides. Additionally, 2/3 of the damaged units of the (unsuccessful)
   attacker are assumed to be killed. To simplify the math a little, I'm assuming
   that 4 in 5 attackers are killed, i.e. not based on by how much the attacker
   is outnumbered. Formulas:
   stake = clashPortion * min{1, 1.6 * powerRatio}
   lossesAtt = stake * 4/5 * powAtt
   lossesDef = stake * 1/2 * powAtt
   clashLossesTemporary accounts for units of the winner
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
double const InvasionGraph::Node::clashPortion = 0.65;

std::pair<double,double> InvasionGraph::Node::clashLossesWinnerLoser(double powAtt,
		double powDef, bool att, bool naval) {

	double lesserPow = std::min(powAtt, powDef);
	double greaterPow = std::max(powAtt, powDef);
	if(greaterPow < 1)
		return std::make_pair(0, 0);
	double cpw = clashPortion, cpl = clashPortion;
	/*  Since I've gotten rid of the attack=true case for non-naval
		attacks, the initial clash needs to produce higher losses;
		hence the increased clashPortion.
		Need higher losses for the loser in order to ensure that it's better to win
		a clash and fail to conquer a city than to lose clash. */
	if(!naval && !att) {
		cpw = clashPortion + 0.15;
		cpl = clashPortion + 0.35;
	}
	if(!att || powAtt > powDef)
		return std::make_pair(
			0.5 * cpw * lesserPow * lesserPow / greaterPow,
			0.5 * cpl * lesserPow
		);
	return std::make_pair(
		0.5 * stake(powAtt, powDef) * powAtt,
		0.8 * stake(powAtt, powDef) * powAtt
	);
}

double InvasionGraph::Node::clashLossesTemporary(double powAtt, double powDef) {

	return clashPortion * std::min(powAtt, powDef) / 3;
}

double InvasionGraph::Node::stake(double powAtt, double powDef) {

	return clashPortion * std::min(1.0, 1.6 * powRatio(powAtt, powDef));
}

double InvasionGraph::Node::powRatio(double pow1, double pow2) {

	return std::min(pow1, pow2) / (std::max(pow1, pow2) + 0.001);
}

// </advc.104>
