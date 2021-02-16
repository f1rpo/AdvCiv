// advc.104: New classes; see UWAI.h.

#include "CvGameCoreDLL.h"
#include "UWAIAgent.h"
#include "WarEvaluator.h"
#include "UWAIReport.h"
#include "WarEvalParameters.h"
#include "MilitaryBranch.h"
#include "CvInfo_GameOption.h"
#include "CvInfo_Building.h" // Just for vote-related info
//#include "CvInfo_Unit.h" // for UWAI::Civ::militaryPower (now in PCH)
#include "CoreAI.h"
#include "CvCityAI.h"
#include "CvDiploParameters.h"
#include "TeamPathFinder.h"
#include "CvArea.h"
#include "RiseFall.h" // advc.705

using std::vector;
using std::set;
using std::pair;

int const maxReparations = 25;

UWAI::Team::Team() {

	agentId = NO_TEAM;
	inBackgr = false;
	report = NULL;
	bForceReport = false;
}

UWAI::Team::~Team() {

	SAFE_DELETE(report);
}

void UWAI::Team::init(TeamTypes agentId) {

	this->agentId = agentId;
	reset();
}

void UWAI::Team::turnPre() {

	/*  This calls UWAI::Civ::turnPre before CvPlayerAI::AI_turnPre.
		That's OK, CvPlayerAI::AI_turnPre doesn't do anything that's crucial
		for UWAI::Civ.
		Need to call UWAI::Civ::turnPre already during the team turn
		b/c the update to UWAICache is important for war planning. */
	for(MemberIter it(agentId); it.hasNext(); ++it)
		it->uwai().turnPre();
}

void UWAI::Team::write(FDataStreamBase* stream) {

	stream->Write(agentId);
}

void UWAI::Team::read(FDataStreamBase* stream) {

	stream->Read((int*)&agentId);
	reset();
}

void UWAI::Team::reset() {

	/*  This function is called at the start of each session, i.e. after starting a
		new game and after loading a game.
		inBackgr doesn't need to be reset after loading, but can't put this line
		in the constructor b/c that gets called before all XML files have been
		loaded. (Could just remove UWAI::Team::inBackgr and call getUWAI
		every time, but that's a bit clunky.) */
	inBackgr = getUWAI.isEnabled(true);
}

void UWAI::Team::doWar() {

	if(!getUWAI.isUpdated())
		return;
	CvTeamAI& agent = GET_TEAM(agentId);
	if(!agent.isAlive() || agent.isBarbarian() || agent.isMinorCiv())
		return;
	FAssertMsg(!agent.isAVassal() || agent.getNumWars() > 0 ||
			agent.AI_getNumWarPlans(WARPLAN_DOGPILE) +
			agent.AI_getNumWarPlans(WARPLAN_LIMITED) +
			agent.AI_getNumWarPlans(WARPLAN_TOTAL) <= 0,
			"Vassals shouldn't have non-preparatory war plans unless at war");
	startReport();
	if(agent.isHuman() || agent.isAVassal()) {
		report->log("%s is %s", report->teamName(agentId), (agent.isHuman() ?
				"human" : "a vassal"));
		for(TeamIter<MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> it(agentId); it.hasNext(); ++it) {
			TeamTypes targetId = it->getID();
			WarPlanTypes wp = agent.AI_getWarPlan(targetId);
			if(wp == WARPLAN_ATTACKED_RECENT)
				considerPlanTypeChange(targetId, 0);
			/*  Non-human vassals abandon human-instructed war prep. after
				20 turns. Humans can have war preparations from AIAutoPlay that they
				should also abandon (but not immediately b/c AIAutoPlay could be
				enabled again). */
			if((wp == WARPLAN_PREPARING_LIMITED || wp == WARPLAN_PREPARING_TOTAL) &&
					(agent.isHuman() || !GET_TEAM(agent.getMasterTeam()).isHuman()) &&
					agent.AI_getWarPlanStateCounter(targetId) > 20)
				considerAbandonPreparations(targetId, -1, 0);
			if(agent.isAVassal()) {
				/*  Make sure we match our master's war plan.
					CvTeamAI::AI_setWarPlan mostly handles this, but doesn't
					align vassal's plans after signing the vassal agreement.
					(Could do that in CvTeam::setVassal I guess.) */
				CvTeamAI const& master = GET_TEAM(agent.getMasterTeam());
				if(master.getID() == targetId)
					continue;
				if(master.isAtWar(targetId))
					agent.AI_setWarPlan(targetId, WARPLAN_DOGPILE);
				else if(master.AI_getWarPlan(targetId) != NO_WARPLAN &&
						!master.isHuman()) // Human master will have to instruct us
					agent.AI_setWarPlan(targetId, WARPLAN_PREPARING_LIMITED);
			}
		}
		report->log("Nothing more to do for this team");
		closeReport();
		return;
	}
	UWAICache& cache = leaderCache();
	if(reviewWarPlans()) {
		scheme();
		for(TeamIter<CIV_ALIVE,KNOWN_POTENTIAL_ENEMY_OF> it(agentId); it.hasNext(); ++it) {
			TeamTypes targetId = it->getID();
			if(agent.AI_isSneakAttackPreparing(targetId))
				cache.setCanBeHiredAgainst(targetId, true);
		}
	}
	else {
		report->log("No scheming b/c clearly busy with current wars");
		for(TeamIter<CIV_ALIVE> it; it.hasNext(); ++it)
			cache.setCanBeHiredAgainst(it->getID(), false);
	}
	closeReport();
}

bool UWAI::Team::isKnownToBeAtWar(TeamTypes observer) const {

	for(TeamIter<MAJOR_CIV,ENEMY_OF> it(agentId); it.hasNext(); ++it) {
		if(observer == NO_TEAM || GET_TEAM(observer).isHasMet(it->getID()))
			return true;
	}
	return false;
}

bool UWAI::Team::hasDefactoDefensivePact(TeamTypes allyId) const {

	CvTeamAI& agent = GET_TEAM(agentId);
	return (agent.isDefensivePact(allyId) || agent.isVassal(allyId) ||
			GET_TEAM(allyId).isVassal(agentId) ||
			(agent.getMasterTeam() == GET_TEAM(allyId).getMasterTeam() &&
			agent.getID() != allyId));
}

bool UWAI::Team::canReach(TeamTypes targetId) const {

	for(MemberIter targetIt(targetId); targetIt.hasNext(); ++targetIt) {
		for(MemberIter agentIt(agentId); agentIt.hasNext(); ++agentIt) {
			if (agentIt->uwai().getCache().
					numReachableCities(targetIt->getID()) > 0)
				return true;
		}
	}
	return false;
}

UWAI::Civ const& UWAI::Team::leaderUWAI() const {

	return GET_PLAYER(GET_TEAM(agentId).getLeaderID()).uwai();
}

UWAI::Civ& UWAI::Team::leaderUWAI() {

	return GET_PLAYER(GET_TEAM(agentId).getLeaderID()).uwai();
}

void UWAI::Team::addTeam(PlayerTypes otherLeaderId) {

	for(MemberIter it(agentId); it.hasNext(); ++it)
		it->uwai().getCache().addTeam(otherLeaderId);
}

double UWAI::Team::utilityToTradeVal(double u) const {

	double r = 0;
	MemberIter it(agentId);
	for(; it.hasNext(); ++it)
		r += utilityToTradeVal(u, it->getID());
	return r / it.nextIndex();
}

double UWAI::Team::tradeValToUtility(double tradeVal) const {

	double r = 0;
	MemberIter it(agentId);
	for(; it.hasNext(); ++it)
		r += tradeValToUtility(tradeVal, it->getID());
	return r / it.nextIndex();
}

double UWAI::Team::utilityToTradeVal(double u,
		PlayerTypes memberId) const {

	return u / GET_PLAYER(memberId).uwai().
			tradeValUtilityConversionRate();
}

double UWAI::Team::tradeValToUtility(double tradeVal,
		PlayerTypes memberId) const {

	return tradeVal * GET_PLAYER(memberId).uwai().
			tradeValUtilityConversionRate();
}

void UWAI::Team::doWarReport() {

	if(!getUWAI.isEnabled())
		return;
	bool bInBackgr = getUWAI.isEnabled(true); // To be restored in the end
	getUWAI.setInBackground(true);
	setForceReport(true);
	doWar();
	setForceReport(false);
	getUWAI.setInBackground(bInBackgr);
}

struct PlanData {
	PlanData(int u, TeamTypes id, int t, bool isNaval) :
		u(u), id(id), t(t), isNaval(isNaval) {}
	bool operator<(PlanData const& o) { return u < o.u; }
	int u;
	TeamTypes id;
	int t;
	bool isNaval;
};
bool UWAI::Team::reviewWarPlans() {

	CvTeamAI& agent = GET_TEAM(agentId);
	if(!agent.AI_isAnyWarPlan()) {
		report->log("%s has no war plans to review", report->teamName(agentId));
		return true;
	}
	bool r = true;
	report->log("%s reviews its war plans", report->teamName(agentId));
	set<TeamTypes> done; // Don't review these for a second time
	bool planChanged = false;
	bool allNaval = true, anyNaval = false;
	bool allLand = true, anyLand = false;
	do {
		vector<PlanData> plans;
		for(TeamIter<MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> it(agentId); it.hasNext(); ++it) {
			CvTeamAI const& target = *it;
			TeamTypes targetId = target.getID();
			if(done.count(targetId) > 0)
				continue;
			if(target.isHuman()) // considerCapitulation might set it to true
				leaderUWAI().getCache().setReadyToCapitulate(targetId, false);
			WarPlanTypes wp = agent.AI_getWarPlan(targetId);
			if(wp == NO_WARPLAN) {
				// As good a place as any to make sure of this
				FAssert(agent.AI_getWarPlanStateCounter(targetId) == 0);
				continue;
			}
			if(target.isAVassal())
				continue;
			WarEvalParameters params(agentId, targetId, *report);
			WarEvaluator eval(params);
			int u = eval.evaluate(wp);
			// 'evaluate' sets preparation time and isNaval in params
			plans.push_back(PlanData(u, targetId, params.getPreparationTime(),
					params.isNaval()));
			/*  Skip scheming when in a very bad war. Very unlikely that another war
				could help then. And I worry that, in rare situations, when the
				outcome of a war is close, but potentially disastrous, that an
				additional war could produce a more favorable simulation outcome. */
			if(u < -100 && agent.isAtWar(targetId))
				r = false;
		}
		std::sort(plans.begin(), plans.end());
		planChanged = false;

		for(size_t i = 0; i < plans.size(); i++) {
			planChanged = !reviewPlan(plans[i].id, plans[i].u, plans[i].t);
			if(planChanged) {
				done.insert(plans[i].id);
				if(done.size() < plans.size())
					report->log("War plan against %s has changed, repeating review",
							report->teamName(plans[i].id));
				break;
			} // Ignore isNaval if we're not sure about the war (low utility)
			else if(plans[i].u > 5) {
				if(plans[i].isNaval) {
					anyNaval = true;
					allLand = false;
				}
				else {
					anyLand = true;
					allNaval = false;
				}
			}
		}
	} while(planChanged);
	/*  As CvTeamAI::AI_updateAreaStrategies is called before CvTeamAI::AI_doWar,
		this is going to be the last word on AreaAI. */
	if(allNaval && anyNaval)
		alignAreaAI(true);
	if(allLand && anyLand)
		alignAreaAI(false);
	return r;
}

void UWAI::Team::alignAreaAI(bool isNaval) {

	PROFILE_FUNC();
	std::set<int> areasToAlign;
	std::set<int> areasNotToAlign;
	for(MemberIter it(agentId); it.hasNext(); ++it) {
		CvPlayerAI const& member = *it;
		CvCity* capital = member.getCapital();
		if(capital == NULL)
			continue;
		CvArea& a = capital->getArea();
		CvCity* targetCity = a.AI_getTargetCity(member.getID());
		bool bAlign = true;
		if(isNaval) {
			if(targetCity!= NULL && (GET_TEAM(targetCity->getTeam()).
					AI_isPrimaryArea(a) || 3 * a.getCitiesPerPlayer(
					targetCity->getOwner()) > a.getCitiesPerPlayer(
					member.getID()))) {
				WarPlanTypes wp = GET_TEAM(agentId).AI_getWarPlan(targetCity->getTeam());
				if(!GET_TEAM(agentId).AI_isPushover(targetCity->getTeam()) ||
						(wp != WARPLAN_TOTAL && wp != WARPLAN_PREPARING_TOTAL)) {
					// Make sure there isn't an easily reachable target in the capital area
					TeamPathFinder<TeamPath::LAND> pf(GET_TEAM(agentId),
							&GET_TEAM(targetCity->getTeam()), 8);
					if(pf.generatePath(capital->getPlot(), targetCity->getPlot()))
						bAlign = false;
				}
			}
		}
		else {
			// Make sure some city can be attacked in the capital area
			if(targetCity == NULL) {
				// Target city is sometimes randomly set to NULL
				targetCity = member.AI_findTargetCity(a);
			}
			if(targetCity == NULL)
				bAlign = false;
			else {
				UWAICache::City* c = member.uwai().getCache().
						lookupCity(targetCity->plotNum());
				if(c == NULL || !c->canReachByLandFromCapital())
					bAlign = false;
			}
		}
		if(bAlign)
			areasToAlign.insert(a.getID());
		else areasNotToAlign.insert(a.getID());
	}
	std::set<int> diff;
	std::set_difference(areasToAlign.begin(), areasToAlign.end(),
					areasNotToAlign.begin(), areasNotToAlign.end(),
					std::inserter(diff, diff.begin()));
	CvMap& m = GC.getMap();
	for(std::set<int>::const_iterator it = diff.begin(); it != diff.end(); ++it) {
		CvArea& a = *m.getArea(*it);
		AreaAITypes oldAAI = a.getAreaAIType(agentId);
		AreaAITypes newAAI = oldAAI;
		if(isNaval) {
			if(oldAAI == AREAAI_MASSING)
				newAAI = AREAAI_ASSAULT_MASSING;
			else if(oldAAI == AREAAI_OFFENSIVE)
				newAAI = AREAAI_ASSAULT;
		}
		else {
			if(oldAAI == AREAAI_ASSAULT_MASSING)
				newAAI = AREAAI_MASSING;
			else if(oldAAI == AREAAI_ASSAULT)
				newAAI = AREAAI_OFFENSIVE;
		}
		if(newAAI != oldAAI)
			a.setAreaAIType(agentId, newAAI);
	}
}

bool UWAI::Team::reviewPlan(TeamTypes targetId, int u, int prepTime) {

	CvTeamAI& agent = GET_TEAM(agentId);
	WarPlanTypes wp = agent.AI_getWarPlan(targetId);
	FAssert(wp != NO_WARPLAN);
	bool bAtWar = agent.isAtWar(targetId);
	int wpAge = agent.AI_getWarPlanStateCounter(targetId);
	FAssert(wpAge >= 0);
	report->log("Reviewing war plan \"%s\" (age: %d turns) against %s (%su=%d)",
			report->warPlanName(wp), wpAge, report->teamName(targetId),
			(bAtWar ? "at war; " : ""), u);
	if(bAtWar) {
		FAssert(wp != WARPLAN_PREPARING_LIMITED && wp != WARPLAN_PREPARING_TOTAL);
		if(!considerPeace(targetId, u))
			return false;
		considerPlanTypeChange(targetId, u);
		/*  Changing between attacked_recent, limited and total is unlikely
			to affect our other war plans, so ignore the return value of
			considerPlanTypeChange and report "no changes" to the caller. */
		return true;
	}
	else {
		if(!canSchemeAgainst(targetId, true)) {
			report->log("War plan \"%s\" canceled b/c %s is no longer a legal target",
					report->warPlanName(wp), report->teamName(targetId));
			if(!inBackgr) {
				agent.AI_setWarPlan(targetId, NO_WARPLAN);
				showWarPlanAbandonedMsg(targetId);
			}
			return false;
		}
		if(wp != WARPLAN_PREPARING_LIMITED && wp != WARPLAN_PREPARING_TOTAL) {
			/*  BtS uses WARPLAN_DOGPILE both for preparing and fighting dogpile wars.
				AdvCiv doesn't distinguish dogpile wars; only uses PREPARING_LIMITED
				or PREPARING_TOTAL for preparations. */
			FAssert(wp == WARPLAN_LIMITED || wp == WARPLAN_TOTAL ||
					(inBackgr && wp == WARPLAN_DOGPILE));
			if(u < 0) {
				report->log("Imminent war canceled; no longer worthwhile");
				if(!inBackgr) {
					agent.AI_setWarPlan(targetId, NO_WARPLAN);
					showWarPlanAbandonedMsg(targetId);
				}
				return false;
			}
			else {
				// Consider switching when war remains imminent for a long time
				double pr = std::min(0.33, wpAge * 0.04);
				report->log("pr of considering to switch target: %d",
						::round(100 * pr));
				if(::bernoulliSuccess(pr, "advc.104 (switch while imminent)")) {
					/*  considerSwitchTarget was written under the assumption
						that no war is imminent. Could be difficult to rectify;
						instead, change the war plan temporarily. */
					agent.AI_setWarPlanNoUpdate(targetId, WARPLAN_PREPARING_LIMITED);
					bool bSwitch = !considerSwitchTarget(targetId, u, 0);
					/*  If we do switch, then considerSwitchTarget has
						already reset the war plan against targetId --
						except when running in the background. */
					if(!bSwitch || inBackgr)
						agent.AI_setWarPlanNoUpdate(targetId, wp);
					if(bSwitch)
						return false;
				}
			}
			CvMap const& m = GC.getMap();
			// 12 turns for a Standard-size map, 9 on Small
			int timeout = std::max(m.getGridWidth(), m.getGridHeight()) / 7;
			// Checking missions is a bit costly, don't do it if timeout isn't near.
			if(wpAge > timeout) {
				// Akin to code in CvTeamAI::AI_endWarVal
				for(MemberIter it(agentId); it.hasNext(); ++it) {
					if(it->AI_isAnyEnemyTargetMission(targetId)) {
						timeout *= 2;
						break;
					}
				}
			}
			if(wpAge > timeout) {
				report->log("Imminent war canceled b/c of timeout (%d turns)",
						timeout);
				if(!inBackgr) {
					agent.AI_setWarPlan(targetId, NO_WARPLAN);
					showWarPlanAbandonedMsg(targetId);
				}
				return false;
			}
			report->log("War remains imminent (%d turns until timeout)",
					1 + timeout - wpAge);
		}
		else {
			if(!considerConcludePreparations(targetId, u, prepTime))
				return false;
			if(!considerAbandonPreparations(targetId, u, prepTime))
				return false;
			if(!considerSwitchTarget(targetId, u, prepTime))
				return false;
		}
	}
	return true;
}

bool UWAI::Team::considerPeace(TeamTypes targetId, int u) {

	CvTeamAI& agent = GET_TEAM(agentId);
	if(!agent.canChangeWarPeace(targetId))
		return true;
	CvTeamAI& target = GET_TEAM(targetId);
	double peaceThresh = peaceThreshold(targetId);
	bool human = target.isHuman();
	if(human) /* They might pay us for peace (whereas, for AI-AI deals, the side
				 that expects to be paid waits for the other side to sue for peace). */
		peaceThresh += 10;
	report->log("Threshold for seeking peace: %d", ::round(peaceThresh));
	if(u >= peaceThresh) {
		/*  Peace so we can free our hands for a different war.
			(The "distraction" war utility aspect also deals with this,
			but it's normally not enough to get the AI to stop a successful
			war before starting one that looks even more worthwhile.) */
		for(TeamIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> it(agentId); it.hasNext(); ++it) {
			TeamTypes otherId = it->getID();
			if(!agent.AI_isSneakAttackReady(otherId))
				continue;
			report->log("Considering peace with %s to focus on"
					" imminent war against %s; evaluating two-front war:",
					report->teamName(targetId), report->teamName(otherId));
			WarEvalParameters params(agentId, otherId, *report, true);
			params.addExtraTarget(targetId);
			/*  We're sure that we want to attack otherId. Only consider
				peace with the ExtraTarget. */
			params.setNotConsideringPeace();
			WarEvaluator eval(params);
			static int const iUWAI_MULTI_WAR_RELUCTANCE = GC.getDefineINT("UWAI_MULTI_WAR_RELUCTANCE");
			/*  Check both limited and total war instead of agent
				.AI_getWarPlan(otherId) in order to avoid a preference for
				the two-front case on account of greater military build-up. */
			int uLim = eval.evaluate(WARPLAN_LIMITED, 0) - iUWAI_MULTI_WAR_RELUCTANCE;
			int uTot = eval.evaluate(WARPLAN_TOTAL, 0) - iUWAI_MULTI_WAR_RELUCTANCE;
			u = std::min(uLim, uTot);
			// Tbd.: If the war plan against otherId is TOTAL
			report->log("Utility of a two-front war compared with a war "
					"only against %s: %d", report->teamName(otherId), u);
			break; // Only one war can be imminent at a time
		}
		if(u >= peaceThresh) {
			report->log("No peace sought b/c war utility is above the peace threshold");
			return true;
		}
	}
	// We refuse to talk for 1 turn
	if(agent.AI_getAtWarCounter(targetId) <= 1) {
		report->log("Too early to consider peace");
		return true;
	}
	CvPlayerAI& targetLeader = GET_PLAYER(target.getLeaderID());
	CvPlayerAI& agentLeader = GET_PLAYER(agent.getLeaderID());
	if(!agentLeader.canContact(targetLeader.getID(), true)) {
		report->log("Can't talk to %s about peace",
				report->leaderName(targetLeader.getID()));
		return true; // Can't contact them for capitulation either
	}
	double prPeace = 0;
	bool bOfferPeace = true;
	int theirReluct = MIN_INT; // Costly, don't compute this sooner than necessary.
	if(human) {
		int contactDelay = agentLeader.AI_getContactTimer(target.getLeaderID(),
				CONTACT_PEACE_TREATY);
		int atWarCounter = agent.AI_getAtWarCounter(targetId);
		if(contactDelay > 0 || agent.AI_getWarPlan(targetId) == WARPLAN_ATTACKED_RECENT ||
				agentLeader.AI_refuseToTalkTurns(target.getLeaderID()) > atWarCounter) {
			if(contactDelay > 0) {
				report->log("No peace with human sought b/c of contact delay: %d",
						contactDelay);
			}
			else report->log("No peace sought b/c war too recent: %d turns", atWarCounter);
			prPeace = 0; // Don't return; capitulation always needs to be checked.
			bOfferPeace = false;
		}
		else {
			// 5 to 10%
			prPeace = 1.0 / std::max(1, GC.getInfo(agentLeader.
					getPersonalityType()).getContactRand(CONTACT_PEACE_TREATY));
			// Adjust probability based on whether peace looks like win-win or zero-sum
			theirReluct = target.uwai().reluctanceToPeace(agentId, false);
			double winWinFactor = (theirReluct + u) / -15.0;
			if(winWinFactor < 0) {
				winWinFactor = std::min(-1 * winWinFactor, 2.5);
				winWinFactor = 1 / winWinFactor;
			}
			else winWinFactor = std::min(winWinFactor, 2.5);
			/*  Tbd.: (5*2.5)% seems a bit much for leaders that should be reluctant
				to sue for peace. Exponentiate prPeace? Subtract a percentage point
				or two before applying the winWinFactor? */
			prPeace *= winWinFactor;
			report->log("Win-win factor: %d percent", ::round(winWinFactor * 100));
		}
	}
	else prPeace = std::sqrt(peaceThresh - u) * 0.03;
	if(bOfferPeace) {
		report->log("Probability for peace negotiation: %d percent",
				::round(prPeace * 100));
		if(::bernoulliSuccess(1 - prPeace, "advc.104 (peace)")) {
			report->log("Peace negotiation randomly skipped");
			if(!human) {
				// Don't consider capitulation to AI w/o having tried peace negotiation
				return true;
			}
			bOfferPeace = false;
		}
	}
	if(theirReluct == MIN_INT)
		theirReluct = target.uwai().reluctanceToPeace(agentId, false);
	report->log("Their reluctance to peace: %d", theirReluct);
	if(bOfferPeace) {
		if(theirReluct <= maxReparations) {
			int tradeVal = 0;
			int demandVal = 0; // (Demand only from humans)
			if(human) {
				tradeVal = endWarVal(targetId) - target.uwai().endWarVal(agentId);
				// A bit higher than the K-Mod discounts (advc.134a)
				double discountFactor = 1.3;
				if(tradeVal < 0) {
					demandVal = -tradeVal;
					// Offer a square deal when it's close
					if(demandVal < utilityToTradeVal(4.25, targetLeader.getID()))
						demandVal = 0;
					else demandVal = ::round(demandVal / discountFactor);
					report->log("Seeking reparations with a trade value of %d", demandVal);
					tradeVal = 0;
				}
				else tradeVal = ::round(discountFactor * tradeVal);
			}
			else {
				// Base the reparations they demand on their economy
				tradeVal = ::round(target.uwai().utilityToTradeVal(
						std::max(0, theirReluct)));
				/*  Reduce the trade value b/c the war isn't completely off the table;
					could continue after 10 turns. */
				tradeVal = ::round(tradeVal * UWAI::reparationsAIPercent / 100.0);
			}
			if(tradeVal > 0 || demandVal == 0)
				report->log("Trying to offer reparations with a trade value of %d", tradeVal);
			bool r = true;
			if(!inBackgr)
				r = !agentLeader.AI_negotiatePeace(targetLeader.getID(), demandVal, tradeVal);
			if(human) {
				if(r)
					report->log("Failed to find a peace offer");
				else report->log("Peace offer sent");
			}
			else report->log("Peace negotiation %s", (r ? "failed" : "succeeded"));
			return r;
		}
		else report->log("No peace negotiation attempted; they're too reluctant");
	}
	if(considerCapitulation(targetId, u, theirReluct))
		return true; // No surrender
	int cities = agent.getNumCities();
	/*  Otherwise, considerCapitulation guarantees that surrender (to AI)
		is possible; before we do it, one last attempt to find help: */
	if(cities > 1 && !tryFindingMaster(targetId))
		return false; // Have become a vassal, or awaiting human response.
	// Liberate any colonies (to leave sth. behind, or just to spite the enemy)
	if(!inBackgr) {
		for(MemberIter it(agentId); it.hasNext(); ++it)
			it->AI_doSplit(true);
	}
	if(agent.getNumCities() != cities) {
		report->log("Empire split");
		return false; // Leads to re-evaluation of war plans; may yet capitulate.
	}
	if(agentLeader.AI_getContactTimer(target.getLeaderID(), CONTACT_PEACE_TREATY) <= 0) {
		report->log("%s capitulation to %s", human ? "Offering" : "Implementing",
				report->leaderName(target.getLeaderID()));
		if(!inBackgr) {
			agentLeader.AI_offerCapitulation(target.getLeaderID());
			return false;
		}
	}
	return true;
}

bool UWAI::Team::considerCapitulation(TeamTypes masterId, int ourWarUtility,
		int masterReluctancePeace) {

	{	int const uThresh = -75;
		if(ourWarUtility * 4 > uThresh) {
			report->log("Don't compute capitulation utility b/c probably not low enough (%d>%d)",
					ourWarUtility, uThresh / 4);
			return true;
		}
		if(ourWarUtility > uThresh) {
			int capUtility = ourWarUtility;
			if(GET_TEAM(agentId).getNumWars(true, true) > 1) {
				/*	Looks like war utility is low, but not low enough. Perhaps
					this is b/c we haven't yet accounted for the protection that
					the master grants us from third parties.
					NB: Ideally, considerCapitulation should not rely on ourWarUtility
					at all when there are multiple (free) war enemies, but that's
					now difficult to change at the call site. */
				report->log("Computing war utility of capitulation (%d>%d)",
						ourWarUtility, uThresh);
				WarEvalParameters params(agentId, masterId, *report, false, NO_PLAYER,
							masterId);
				WarEvaluator eval(params);
				capUtility = eval.evaluate(GET_TEAM(agentId).AI_getWarPlan(masterId));
			}
			if(capUtility > uThresh) {
				report->log("No capitulation b/c utility not low enough (%d>%d)",
						capUtility, uThresh);
				return true;
			}
		}
	}
	/*  Low reluctance to peace can just mean that there isn't much left for
		them to conquer; doesn't have to mean that they'll soon offer peace.
		Probability test to ensure that we eventually capitulate even if
		master's reluctance remains low. */
	double prSkip = 1 - (masterReluctancePeace * 0.015 + 0.3);
	int ourCities = GET_TEAM(agentId).getNumCities();
	prSkip = ::dRange(prSkip, 0.0,
			// Reduce maximal waiting time in the late game
			0.87 - 0.04 * GET_TEAM(masterId).AI_getCurrEraFactor().getDouble());
	// If few cities remain, we can't afford to wait
	if(ourCities <= 2)
		prSkip -= 0.2;
	if(ourCities <= 1)
		prSkip -= 0.1;
	report->log("%d percent probability to delay capitulation based on master's "
			"reluctance to peace (%d)", ::round(100 * prSkip), masterReluctancePeace);
	if(::bernoulliSuccess(prSkip, "advc.104 (cap)")) {
		report->log("No capitulation this turn");
		return true;
	}
	if(prSkip > 0)
		report->log("Not skipped");
	/*  Diplo is civ-on-civ, but the result affects the whole team, and since
		capitulation isn't a matter of attitude, it doesn't matter which team
		members negotiate it. */
	CvPlayerAI const& ourLeader = GET_PLAYER(GET_TEAM(agentId).getLeaderID());
	CvTeamAI const& master = GET_TEAM(masterId);
	if(!ourLeader.canTradeItem(master.getLeaderID(), TradeData(TRADE_SURRENDER))) {
		report->log("Capitulation to %s impossible", report->teamName(masterId));
		return true;
	}
	bool human = GET_TEAM(masterId).isHuman();
	/*  Make master accept if it's not sure about continuing the war. Note that
		due to change advc.130v, gaining a vassal can't really hurt the master. */
	bool checkAccept = !human && masterReluctancePeace >= 15;
	if(!checkAccept && !human)
		report->log("Master accepts capitulation b/c of low reluctance to peace (%d)",
				masterReluctancePeace);
	if(human) {
		// Otherwise, AI_surrenderTrade can't return true for a human master
		leaderUWAI().getCache().setReadyToCapitulate(masterId, true);
	}
	// Checks our willingness and that of the master
	DenialTypes denial = GET_TEAM(agentId).AI_surrenderTrade(
			masterId, CvTeamAI::VASSAL_POWER_MOD_SURRENDER, checkAccept);
	if(denial != NO_DENIAL) {
		report->log("Not ready to capitulate%s; denial code: %d",
				checkAccept ? " (or master refuses)" : "", (int)denial);
		if(human) {
			/*  To ensure that the capitulation decision is made on an AI turn;
				so that tryFindingMaster and AI_doSplit are called by considerPeace. */
			leaderUWAI().getCache().setReadyToCapitulate(masterId, false);
		}
		return true;
	}
	report->log("%s ready to capitulate to %s", report->teamName(agentId),
			report->teamName(masterId));
	return false;
}

bool UWAI::Team::tryFindingMaster(TeamTypes enemyId) {

	CvPlayerAI& ourLeader = GET_PLAYER(GET_TEAM(agentId).getLeaderID());
	for(TeamIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF,true> it(agentId); it.hasNext(); ++it) {
		CvTeamAI& master = *it;
		if(master.isAtWar(agentId) ||
				// No point if they're already our ally
				master.isAtWar(enemyId) ||
				!ourLeader.canContact(master.getLeaderID(), true))
			continue;
		// Based on code in CvPlayerAI::AI_doDiplo
		CvPlayerAI& masterLeader = GET_PLAYER(master.getLeaderID());
		TradeData item(TRADE_VASSAL);
		/*  Test Denial separately b/c it can cause the master to evaluate war
			against enemyId, which is costly. */
		if(!ourLeader.canTradeItem(masterLeader.getID(), item))
			continue;
		// Don't nag them (especially not humans)
		if(ourLeader.AI_getContactTimer(masterLeader.getID(),
				// Same contact memory for alliance and vassal agreement
				CONTACT_PERMANENT_ALLIANCE) != 0) {
			report->log("%s not asked for protection b/c recently contacted",
					report->leaderName(masterLeader.getID()));
			continue;
		}
		// Checks both our and master's willingness
		if(ourLeader.getTradeDenial(masterLeader.getID(), item) != NO_DENIAL)
			continue;
		if(master.isHuman())
			report->log("Asking human %s for vassal agreement",
					report->leaderName(masterLeader.getID()));
		else report->log("Signing vassal agreement with %s",
				report->teamName(master.getID()));
		if(!inBackgr) {
			CLinkList<TradeData> ourList, theirList;
			ourList.insertAtEnd(item);
			if(master.isHuman()) {
				ourLeader.AI_changeContactTimer(masterLeader.getID(),
						CONTACT_PERMANENT_ALLIANCE,
						ourLeader.AI_getContactDelay(CONTACT_PERMANENT_ALLIANCE));
				CvDiploParameters* pDiplo = new CvDiploParameters(ourLeader.getID());
				pDiplo->setDiploComment(GC.getAIDiploCommentType("OFFER_VASSAL"));
				pDiplo->setAIContact(true);
				pDiplo->setOurOfferList(theirList);
				pDiplo->setTheirOfferList(ourList);
				gDLL->beginDiplomacy(pDiplo, masterLeader.getID());
			}
			else GC.getGame().implementDeal(ourLeader.getID(),
					masterLeader.getID(), ourList, theirList);
		}
		return false;
	}
	report->log("No partner for a voluntary vassal agreement found");
	return true;
}

bool UWAI::Team::considerPlanTypeChange(TeamTypes targetId, int u) {

	CvTeamAI& agent = GET_TEAM(agentId);
	CvTeamAI& target = GET_TEAM(targetId);
	FAssert(agent.isAtWar(targetId));
	WarPlanTypes wp = agent.AI_getWarPlan(targetId);
	int wpAge = agent.AI_getWarPlanStateCounter(targetId);
	WarPlanTypes altWarPlan = NO_WARPLAN;
	switch(wp) {
	case WARPLAN_ATTACKED_RECENT:
		// Same as BBAI in CvTeamAI::AI_doWar, but switch after 8 turns (not 10)
		if(wpAge >= 4) {
			if(!target.AI_isLandTarget(agentId) || wpAge >= 8) {
				report->log("Switching to war plan \"attacked\" after %d turns",
						wpAge);
				if(!inBackgr) {
					agent.AI_setWarPlan(targetId, WARPLAN_ATTACKED);
					// Don't reset wpAge
					agent.AI_setWarPlanStateCounter(targetId, wpAge);
				}
				return false;
			}
		}
		report->log("Too early to switch to \"attacked\" war plan");
		break;
	// Treat these three as limited wars, and consider switching to total
	case WARPLAN_ATTACKED:
	case WARPLAN_DOGPILE:
	case WARPLAN_LIMITED:
		altWarPlan = WARPLAN_TOTAL;
		break;
	case WARPLAN_TOTAL:
		altWarPlan = WARPLAN_LIMITED;
		break;
	default: FErrorMsg("Unsuitable war plan type");
	}
	if(altWarPlan == NO_WARPLAN)
		return true;
	UWAIReport silentReport(true);
	WarEvalParameters params(agentId, targetId, silentReport);
	WarEvaluator eval(params);
	int altU = eval.evaluate(altWarPlan);
	report->log("Utility of alt. war plan (%s): %d", report->warPlanName(altWarPlan),
			altU);
	double pr = 0;
	if(altU > u) {
		// Increase both utilities if u is close to 0
		int padding = 0;
		if(u < 20)
			padding += 20 - u;
		pr = (altU + padding) / (4.0 * (u + padding));
	}
	if(pr > 0) {
		double const lww = limitedWarWeight();
		if(altWarPlan == WARPLAN_LIMITED)
			pr *= lww;
		else pr = (lww < 0.01 ? 1 : pr / lww);
		if(lww < 0.99 || lww > 1.01)
			report->log("Bias for/against limited war: %d percent", ::round(100 * lww));
	}
	report->log("Probability of switching: %d percent", ::round(pr * 100));
	if(pr <= 0)
		return true;
	if(::bernoulliSuccess(pr, "advc.104 (sw plan)")) {
		report->log("Switching to war plan \"%s\"", report->warPlanName(altWarPlan));
		if(!inBackgr) {
			agent.AI_setWarPlan(targetId, altWarPlan);
			agent.AI_setWarPlanStateCounter(targetId, wpAge); // Don't reset wpAge
		}
		return false;
	}
	report->log("War plan not switched; still \"%s\"", report->warPlanName(wp));
	return true;
}

bool UWAI::Team::considerAbandonPreparations(TeamTypes targetId, int u,
		int timeRemaining) {

	CvTeamAI& agent = GET_TEAM(agentId);
	if(agent.AI_countWarPlans() > agent.getNumWars(true, true) + 1) {
		/*  Only one war imminent or preparing at a time.
			(WarEvaluator doesn't handle this properly, i.e. would ignore the
			all but one plan). Can only occur here if UWAI was running in the
			background for some time. */
		if(!inBackgr) {
			agent.AI_setWarPlan(targetId, NO_WARPLAN);
			showWarPlanAbandonedMsg(targetId);
		}
		report->log("More than one war in preparation, canceling the one against %s",
				report->teamName(targetId));
		return false;
	}
	if(u >= 0)
		return true;
	if(timeRemaining <= 0) {
		report->log("Time limit for preparations reached; plan abandoned");
		if(!inBackgr) {
			agent.AI_setWarPlan(targetId, NO_WARPLAN);
			showWarPlanAbandonedMsg(targetId);
		}
		return false;
	}
	WarPlanTypes wp = agent.AI_getWarPlan(targetId);
	if(inBackgr && wp == WARPLAN_DOGPILE)
		wp = WARPLAN_PREPARING_LIMITED;
	int warRand = -1;
	if(wp == WARPLAN_PREPARING_LIMITED)
		warRand = agent.AI_limitedWarRand();
	if(wp == WARPLAN_PREPARING_TOTAL)
		warRand = agent.AI_maxWarRand();
	FAssert(warRand >= 0);
	// WarRand is between 40 (aggro) and 400 (chilled)
	double pr = std::min(1.0, -u * warRand / 7500.0);
	report->log("Abandoning preparations with probability %d percent (warRand=%d)",
			::round(pr * 100), warRand);
	if(::bernoulliSuccess(pr, "advc.104 (aband)")) {
		report->log("Preparations abandoned");
		if(!inBackgr) {
			agent.AI_setWarPlan(targetId, NO_WARPLAN);
			showWarPlanAbandonedMsg(targetId);
		}
		return false;
	}
	else report->log("Preparations not abandoned");
	return true;
}

bool UWAI::Team::considerSwitchTarget(TeamTypes targetId, int u,
		int timeRemaining) {

	CvTeamAI& agent = GET_TEAM(agentId);
	WarPlanTypes wp = agent.AI_getWarPlan(targetId);
	TeamTypes bestAltTargetId = NO_TEAM;
	int bestUtility = 0;
	bool qualms = (timeRemaining > 0 && agent.AI_isAvoidWar(targetId, true));
	bool altQualms = false;
	for(TeamIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> it(agentId); it.hasNext(); ++it) {
		TeamTypes altTargetId = it->getID();
		if(!canSchemeAgainst(altTargetId, false) ||
				agent.turnsOfForcedPeaceRemaining(altTargetId) > timeRemaining)
			continue;
		bool loopQualms = agent.AI_isAvoidWar(altTargetId, true);
		if(loopQualms && !qualms)
			continue;
		UWAIReport silentReport(true);
		WarEvalParameters params(agentId, altTargetId, silentReport, true);
		WarEvaluator eval(params);
		int uSwitch = eval.evaluate(wp, timeRemaining);
		if(uSwitch > std::max(bestUtility, qualms ? 0 : u)) {
			bestAltTargetId = altTargetId;
			bestUtility = uSwitch;
			altQualms = loopQualms;
		}
	}
	if(bestAltTargetId == NO_TEAM) {
		report->log("No other promising target for war preparations found");
		return true;
	}
	double padding = 0;
	if(std::min(u, bestUtility) < 20)
		padding += 20 - std::min(u, bestUtility);
	double pr = 0.75 * (1 - (u + padding) / (bestUtility + padding));
	if(qualms && !altQualms)
		pr += 1.8;
	report->log("Switching target for war preparations to %s (u=%d) with pr=%d percent",
			report->teamName(bestAltTargetId), bestUtility, ::round(100 * pr));
	if(!::bernoulliSuccess(pr, "advc.104 (sw target)")) {
		report->log("Target not switched");
		return true;
	}
	report->log("Target switched");
	if(!inBackgr) {
		int wpAge = agent.AI_getWarPlanStateCounter(targetId);
		agent.AI_setWarPlan(targetId, NO_WARPLAN);
		showWarPlanAbandonedMsg(targetId);
		agent.AI_setWarPlan(bestAltTargetId, wp);
		showWarPrepStartedMsg(bestAltTargetId);
		agent.AI_setWarPlanStateCounter(bestAltTargetId, wpAge);
	}
	return false;
}

bool UWAI::Team::considerConcludePreparations(TeamTypes targetId, int u,
		int timeRemaining) {

	CvTeamAI& agent = GET_TEAM(agentId);
	if(agent.AI_countWarPlans() > agent.getNumWars(true, true) + 1)
		return true; // More than 1 war in preparation; let considerAbandonPreparations handle it.
	int turnsOfPeace = agent.turnsOfForcedPeaceRemaining(targetId);
	if(turnsOfPeace > 3) {
		report->log("Can't finish preparations b/c of peace treaty (%d turns"
				" to cancel)", turnsOfPeace);
		return true;
	}
	WarPlanTypes wp = agent.AI_getWarPlan(targetId);
	WarPlanTypes directWp = WARPLAN_LIMITED;
	if(wp == WARPLAN_PREPARING_TOTAL)
		directWp = WARPLAN_TOTAL;
	bool conclude = false;
	if(timeRemaining <= 0) {
		if(u < 0)
			return true; // Let considerAbandonPreparations handle it
		report->log("Time limit for preparation reached; adopting direct war plan");
		conclude = true;
	}
	else {
		UWAIReport silentReport(true);
		WarEvalParameters params(agentId, targetId, silentReport);
		WarEvaluator eval(params);
		int u0 = eval.evaluate(directWp);
		report->log("Utility of immediate switch to direct war plan: %d", u0);
		if(u0 > 0) {
			double rand = GC.getGame().getSRand().get(MAX_UNSIGNED_SHORT,
					// (CvRandom::getFloat doesn't allow a szLog param)
					"conclude war prep") / (double)MAX_UNSIGNED_SHORT;
			/*  The more time remains, the longer we'd still have to wait in order
				to get utility u. Therefore low thresh if high timeRemaining.
				Example: 10 turns remaining, u=80: thresh between 32 and 80.
						 8 turns later, if still u=80: thresh between 64 and 80. */
			double randPortion = std::min(0.4, timeRemaining / 10.0);
			double thresh = u * ((1 - randPortion) + rand * randPortion);
			report->log("Utility threshold for direct war plan randomly set to %d",
					::round(thresh));
			if(thresh < (double)u0)
				conclude = true;
			report->log("%sirect war plan adopted", (conclude ? "D" : "No d"));
		}
	}
	if(conclude) {
		if(!inBackgr) {
			// Don't AI_setWarPlanStateCounter; let CvTeamAI reset it to 0
			agent.AI_setWarPlan(targetId, directWp);
		}
		return false;
	}
	return true;
}

int UWAI::Team::peaceThreshold(TeamTypes targetId) const {

	// Computation is similar to BtS CvPlayerAI::AI_isWillingToTalk
	CvTeamAI const& agent = GET_TEAM(agentId);
	if(agent.isHuman())
		return 0;
	CvTeamAI const& target = GET_TEAM(targetId);
	double r = -7.5; // To give it time
	// Give it more time then; also more bitterness I guess.
	if(agent.AI_getWarPlan(targetId) == WARPLAN_TOTAL)
		r -= 5;
	// If they're attacked or att. recent (they could switch to total war eventually)
	if(!target.AI_isChosenWar(agentId))
		r -= 5;
	// This puts the term between -30 and 10
	r += (1 - leaderUWAI().prideRating()) * 40 - 30;
	r += std::min(15.0, (agent.AI_getWarSuccess(targetId) +
				2 * target.AI_getWarSuccess(agentId)) /
				(0.5 * GC.getWAR_SUCCESS_CITY_CAPTURING()) +
				agent.AI_getAtWarCounter(targetId));
	int rounded = ::round(r);
	if(!target.isHuman()) {
		/* Never set the threshold above 0 for inter-AI peace
			(let _them_ sue for peace instead) */
		rounded = std::min(0, rounded);
	}
	return rounded;
}

int UWAI::Team::uJointWar(TeamTypes targetId, TeamTypes allyId) const {

	// Only log about inter-AI war trades
	bool silent = GET_TEAM(agentId).isHuman() || GET_TEAM(allyId).isHuman() ||
			!isReportTurn();
	/*  Joint war against vassal/ master: This function then gets called twice.
		The war evaluation is the same though; so disable logging for one of the
		calls. */
	if(GET_TEAM(targetId).isAVassal()) {
		silent = true;
		targetId = GET_TEAM(targetId).getMasterTeam();
	}
	UWAIReport rep(silent);
	if(!silent) {
		rep.log("h3.\nNegotiation of joint war\n");
		rep.log("%s is evaluating the utility of %s joining the war"
				" against %s\n", rep.teamName(agentId),
				rep.teamName(allyId), rep.teamName(targetId));
	}
	WarEvalParameters params(agentId, targetId, rep);
	params.addWarAlly(allyId);
	params.setImmediateDoW(true);
	WarPlanTypes wp = WARPLAN_LIMITED;
	// (agent at war currently guarenteed by caller CvPlayerAI::AI_doDiplo)
	if(GET_TEAM(agentId).isAtWar(targetId)) {
		params.setNotConsideringPeace();
		wp = NO_WARPLAN; // Evaluate the current plan
	}
	WarEvaluator eval(params);
	int r = eval.evaluate(wp);
	rep.log(""); // newline
	/*  Military analysis may conclude that ally is going to send ships, perhaps
		just a few, but enough to tip the scales. Highly unlikely in the first half
		of the game. */
	if(!GET_TEAM(allyId).uwai().isLandTarget(targetId) &&
			GET_TEAM(allyId).getCurrentEra() < CvEraInfo::AI_getAgeOfExploration())
		return std::min(0, r);
	return r;
}

int UWAI::Team::tradeValJointWar(TeamTypes targetId,
		TeamTypes allyId) const {

	/*  This function could handle a human ally, but the AI isn't supposed to
		pay humans for war (and I've no plans for changing that). */
	FAssert(!GET_TEAM(allyId).isHuman());
	PROFILE_FUNC();
	int u = uJointWar(targetId, allyId); // Compares joint war with solo war
	/*  Low u suggests that we're not sure that we'll need help. Also,
		war evaluation doesn't account for MEMORY_HIRED_WAR_ALLY and
		CvTeam::makeUnwillingToTalk (advc.104o). */
	if(u < 5 + 9 * ::hash(GC.getGame().getGameTurn(),
			GET_TEAM(agentId).getLeaderID()))
		return 0;
	// NB: declareWarTrade applies an additional threshold
	return ::round(utilityToTradeVal(std::min(u, -dwtUtilityThresh)));
}

int UWAI::Team::reluctanceToPeace(TeamTypes otherId,
		bool nonNegative) const {

	int r = -uEndWar(otherId) - std::min(0, peaceThreshold(otherId));
	if(nonNegative)
		return std::max(0, r);
	return r;
}

bool UWAI::Team::canSchemeAgainst(TeamTypes targetId,
		bool assumeNoWarPlan) const {

	if(targetId == NO_TEAM || targetId == BARBARIAN_TEAM || targetId == agentId)
		return false;
	CvTeamAI const& agent = GET_TEAM(agentId);
	// Vassals don't scheme
	if(agent.isAVassal())
		return false;
	CvTeam const& target = GET_TEAM(targetId);
	/*  advc.130o: Shouldn't attack right after peace from demand; therefore
		don't plan war during the peace treaty. */
	if(agent.isForcePeace(targetId) && agent.AI_getMemoryCount(
			targetId, MEMORY_ACCEPT_DEMAND) > 0)
		return false;
	return target.isAlive() && !target.isMinorCiv() && agent.isHasMet(targetId) &&
			!target.isAVassal() && target.getNumCities() > 0 && (assumeNoWarPlan ||
			agent.AI_getWarPlan(targetId) == NO_WARPLAN) &&
			agent.canEventuallyDeclareWar(targetId);
}

double UWAI::Team::limitedWarWeight() const {

	int const limitedWarRand = GET_TEAM(agentId).AI_limitedWarRand();
	if(limitedWarRand <= 0)
		return 0;
	/*  The higher exp, the greater the impact of personal preference for
		limited or total war. */
	double const exp = 0.75;
	// Bias for total war b/c the WarRand values are biased toward limited war
	double const base = 0.8 * GET_TEAM(agentId).AI_maxWarRand() / limitedWarRand;
	FAssert(base >= 0);
	return std::pow(base, exp);
}

struct TargetData {
	TargetData(double drive, TeamTypes id, bool total, int u) :
			drive(drive),id(id),total(total),u(u) {}
	bool operator<(TargetData const& o) { return drive < o.drive; }
	double drive;
	TeamTypes id;
	bool total;
	int u;
};
void UWAI::Team::scheme() {

	CvTeamAI& agent = GET_TEAM(agentId);
	if(agent.AI_countWarPlans() > agent.getNumWars(true, true)) {
		report->log("No scheming b/c already a war in preparation");
		return;
	}
	for(TeamIter<CIV_ALIVE> it; it.hasNext(); ++it) {
		CvTeamAI const& minor = *it;
		if(!minor.isMinorCiv())
			continue;
		int closeness = agent.AI_teamCloseness(minor.getID());
		if(closeness >= 40 && minor.AI_isLandTarget(agentId) &&
				agent.AI_isLandTarget(minor.getID())) {
			report->log("No scheming b/c busy fighting minor civ %s at closeness %d",
					report->teamName(minor.getID()), closeness);
			return;
		}
	}
	vector<TargetData> targets;
	double totalDrive = 0;
	UWAICache& cache = leaderCache();
	for(TeamIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> it(agentId); it.hasNext(); ++it) {
		TeamTypes targetId = it->getID();
		if(!canSchemeAgainst(targetId, true))
			cache.setCanBeHiredAgainst(targetId, false);
		if(!canSchemeAgainst(targetId, false))
			continue;
		report->log("Scheming against %s", report->teamName(targetId));
		bool shortWork = agent.AI_isPushover(targetId);
		if(shortWork)
			report->log("Target assumed to be short work");
		bool skipTotal = (agent.AI_isAnyWarPlan() || shortWork);
		/*  Skip scheming entirely if already in a total war? Probably too
			restrictive in the lategame. Perhaps have reviewWarPlans compute the
			smallest utility among current war plans, and skip scheming if that
			minimum is, say, -55 or less. */
		report->setMute(true);
		WarEvalParameters params(agentId, targetId, *report);
		WarEvaluator eval(params);
		int uTotal = INT_MIN;
		bool totalNaval = false;
		int totalPrepTime = -1;
		if(!skipTotal) {
			uTotal = eval.evaluate(WARPLAN_PREPARING_TOTAL);
			totalNaval = params.isNaval();
			totalPrepTime = params.getPreparationTime();
		}
		int uLimited = !shortWork ? eval.evaluate(WARPLAN_PREPARING_LIMITED):
				eval.evaluate(WARPLAN_PREPARING_LIMITED, 0);
		bool limitedNaval = params.isNaval();
		int limitedPrepTime = params.getPreparationTime();
		bool total = false;
		if(uLimited < 0 && uTotal > 0)
			total = true;
		if(uLimited < 0 && uTotal < 0) // Only relevant for logging
			total = (uTotal > uLimited);
		if(uLimited > 0 && uTotal > 0) {
			double const lww = limitedWarWeight();
			if(lww < 0.99 || lww > 1.01)
				report->log("Bias for/against limited war: %d percent", ::round(100 * lww));
			int const padding = GC.getGame().getSorenRandNum(40,
					"advc.104 (total vs. limited)");
			total = (uTotal + padding > (padding + uLimited) * lww);
		}
		int u = std::max(uLimited, uTotal);
		report->setMute(false);
		static int const UWAI_REPORT_THRESH = GC.getDefineINT("UWAI_REPORT_THRESH");
		static int const UWAI_REPORT_THRESH_HUMAN = GC.getDefineINT("UWAI_REPORT_THRESH_HUMAN");
		int reportThresh = (GET_TEAM(targetId).isHuman() ? UWAI_REPORT_THRESH_HUMAN : UWAI_REPORT_THRESH);
		// Extra evaluation just for logging
		if(!report->isMute() && u > reportThresh) {
			if(total)
				eval.evaluate(WARPLAN_PREPARING_TOTAL, totalNaval, totalPrepTime);
			else eval.evaluate(WARPLAN_PREPARING_LIMITED, limitedNaval, limitedPrepTime);
		}
		else report->log("%s %s war has %d utility", (total ? "total" : "limited"),
				(((total && totalNaval) || (!total && limitedNaval)) ?
				"naval" : ""), u);
		bool const canHireOld = cache.canBeHiredAgainst(targetId);
		cache.updateCanBeHiredAgainst(targetId, u, dwtUtilityThresh);
		bool const canHireNew = cache.canBeHiredAgainst(targetId);
		if(canHireOld != canHireNew) {
			if(canHireNew)
				report->log("Can now (possibly) be hired for war");
			else report->log("Can no longer be hired for war");
		}
		if(u <= 0)
			continue;
		double drive = u;
		/*  WarRand of e.g. Alexander and Montezuma 50 for total war, else 40;
			Gandhi and Mansa Musa 400 for total, else 200.
			I.e. peaceful leaders hesitate longer before starting war preparations;
			warlike leaders have more drive. */
		double div = (total ? agent.AI_maxWarRand() : agent.AI_limitedWarRand());
		FAssert(div >= 0);
		// Let's make the AI a bit less patient, especially the peaceful types
		div = std::pow(div, 0.95); // This maps e.g. 400 to 296 and 40 to 33
		if(div < 0.01)
			drive = 0;
		else drive /= div;
		/*  Delay preparations probabilistically (by lowering drive) when there's
			still a peace treaty */
		double peacePortionRemaining = std::min(0.95, agent.turnsOfForcedPeaceRemaining(targetId) /
				// +1.0 b/c I don't want 0 drive at this point
				(GC.getDefineINT(CvGlobals::PEACE_TREATY_LENGTH) + 1.0));
		drive *= std::pow(1 - peacePortionRemaining, 1.0); // was ^1.5; let's try it w/o exponentiation
		targets.push_back(TargetData(drive, targetId, total, u));
		totalDrive += drive;
	}
	// Descending by drive
	std::sort(targets.begin(), targets.end());
	std::reverse(targets.begin(), targets.end());
	for(size_t i = 0; i < targets.size(); i++) {
		TeamTypes targetId = targets[i].id;
		double drive = targets[i].drive;
		// Conscientious hesitation
		if(agent.AI_isAvoidWar(targetId, true)) {
			drive -= totalDrive / 2;
			if(drive <= 0)
				continue;
		}
		WarPlanTypes wp = (targets[i].total ? WARPLAN_PREPARING_TOTAL :
				WARPLAN_PREPARING_LIMITED);
		report->log("Drive for war preparations against %s: %d percent",
				report->teamName(targetId), ::round(100 * drive));
		if(::bernoulliSuccess(drive, "advc.104 (drive)")) {
			if(!inBackgr) {
				agent.AI_setWarPlan(targetId, wp);
				showWarPrepStartedMsg(targetId);
			}
			report->log("War plan initiated (%s)", report->warPlanName(wp));
			break; // Prepare only one war at a time
		}
		report->log("No preparations begun this turn");
		if(GET_TEAM(targetId).isHuman() && targets[i].u <= 23) {
			PlayerTypes theirLeaderId = GET_TEAM(targetId).getLeaderID();
			if(GET_PLAYER(agent.getLeaderID()).canContact(theirLeaderId, true)) {
				report->log("Trying to amend tensions with human %s",
						report->teamName(targetId));
				if(!inBackgr) {
					if(leaderUWAI().amendTensions(theirLeaderId))
						report->log("Diplo message sent");
					else report->log("No diplo message sent");
				}
			}
			else report->log("Can't amend tension b/c can't contact %s",
					report->leaderName(theirLeaderId));
		}
	}
}

DenialTypes UWAI::Team::declareWarTrade(TeamTypes targetId,
		TeamTypes sponsorId) const {

	if(!canReach(targetId))
		return DENIAL_NO_GAIN;
	CvTeam const& sponsor = GET_TEAM(sponsorId);
	/*  Check canBeHiredAgainst only in large games (to reduce the number of
		war trade alerts seen by humans) */
	bool bInsufficientPayment = false;
	if(!sponsor.isHuman() || sponsor.getHasMetCivCount() < 8 ||
			leaderCache().canBeHiredAgainst(targetId)) {
		int utilityThresh = dwtUtilityThresh + 2;
		UWAIReport silentReport(true);
		PlayerTypes const sponsorLeaderId = sponsor.getLeaderID();
		WarEvalParameters params(agentId, targetId, silentReport, false,
				sponsorLeaderId);
		WarEvaluator eval(params, true);
		// Has to be negative; we can't be hired for free.
		int u = std::min(-1, eval.evaluate(WARPLAN_LIMITED));
		if(u > utilityThresh) {
			if(GET_TEAM(sponsorId).isHuman()) {
				int humanTradeVal = -1;
				leaderUWAI().canTradeAssets(::round(utilityToTradeVal(
						utilityThresh)), sponsorLeaderId, &humanTradeVal);
				// Don't return NO_DENIAL if human can't pay enough
				utilityThresh = std::max(utilityThresh,
						-::round(tradeValToUtility(humanTradeVal) +
						// Add 5 for gold that the human might be able to procure
						((GET_TEAM(sponsorId).isGoldTrading() ||
						GET_TEAM(agentId).isGoldTrading() ||
						// Or they could ask nicely
						(GET_TEAM(sponsorId).isAtWar(targetId) &&
						GET_TEAM(agentId).AI_getAttitude(sponsorId) >=
						ATTITUDE_PLEASED)) ? 5 : 0)));
			}
			if(u > utilityThresh)
				return NO_DENIAL;
			else bInsufficientPayment = true;
		}
		/* "Maybe we'll change our mind" when it's (very) close?
			No, don't provide this info after all. */
		/*if(u > utilityThresh - 5)
			return DENIAL_RECENT_CANCEL;*/
	}
	CvTeamAI const& agent = GET_TEAM(agentId);
	// We don't know why utility is so small; can only guess.
	if(!bInsufficientPayment && 4 * agent.getPower(true) +
			(sponsor.isAtWar(targetId) ? 2 * sponsor.getPower(true) : 0) <
			3 * GET_TEAM(targetId).getPower(true))
		return DENIAL_POWER_THEM;
	if(agent.AI_anyMemberAtVictoryStage(AI_VICTORY_CULTURE4 | AI_VICTORY_SPACE4))
		return DENIAL_VICTORY;
	// "Too much on our hands" can mean anything
	return DENIAL_TOO_MANY_WARS;
}

int UWAI::Team::declareWarTradeVal(TeamTypes targetId,
		TeamTypes sponsorId) const {

	bool silent = GET_TEAM(sponsorId).isHuman() || !isReportTurn();
	UWAIReport rep(silent);
	if(!silent) {
		rep.log("*Considering sponsored war*");
		rep.log("%s is considering to declare war on %s at the request of %s",
				rep.teamName(agentId), rep.teamName(targetId),
				rep.teamName(sponsorId));
		/*  Will see the above lines multiple times in the log when this team
			agrees to declare war b/c CvGame::implementDeal causes the dealValue
			to be recomputed twice for diplomatic consequences ("traded with enemy",
			"fair and forthright"). */
	}
	CvTeamAI const& sponsor = GET_TEAM(sponsorId);
	// Don't log details of war evaluation
	UWAIReport silentReport(true);
	WarEvalParameters params(agentId, targetId, silentReport, false, sponsor.getLeaderID());
	WarEvaluator eval(params);
	int u = eval.evaluate(WARPLAN_LIMITED);
	/*  Sponsored war results in a peace treaty with the sponsor. Don't check if
		we're planning war against the sponsor - too easy to read (b/c there are
		just two possibilities). Instead check war utility against the sponsor. */
	if(canSchemeAgainst(sponsorId, true)) {
		WarEvalParameters params2(agentId, sponsorId, silentReport);
		WarEvaluator eval2(params2);
		int uVsSponsor = eval2.evaluate(WARPLAN_LIMITED, 3);
		if(uVsSponsor > 0)
			u -= ::round(0.67 * uVsSponsor);
	}
	/*  Don't trust utility completely; human sponsor will try to pick the time
		when we're most willing. Need to be conservative. Also, apparently the
		sponsor gets sth. out of the DoW, and therefore we should always ask for
		a decent price, even if we don't mind declaring war. */
	int lowerBound = -2;
	if(!sponsor.isAtWar(targetId))
		lowerBound -= 4;
	// War utility esp. unreliable when things get desperate
	int wsr = GET_TEAM(agentId).AI_getWarSuccessRating();
	if(wsr < 0)
		lowerBound += wsr / 10;
	u = std::min(lowerBound, u);
	double priceOurEconomy = utilityToTradeVal(-u);
	double priceSponsorEconomy = sponsor.uwai().utilityToTradeVal(-u);
	/*  If the sponsor has the bigger economy, use the mean of the price based on
		our economy and his, otherwise, base it only on our economy. */
	double price = (priceOurEconomy + std::max(priceOurEconomy, priceSponsorEconomy))
			/ 2;
	rep.log("War utility: %d, base price: %d", u, ::round(price));
	CvTeamAI const& agent = GET_TEAM(agentId);
	/*  Adjust the price based on our attitude and obscure it so that humans
		can't learn how willing we are exactly */
	AttitudeTypes towardSponsor = agent.AI_getAttitude(sponsorId);
	std::vector<int> hashInput;
	/*  Allow hash to change w/e the target's rank or our attitude toward the
		sponsor changes */
	hashInput.push_back(GC.getGame().getRankTeam(targetId));
	hashInput.push_back(towardSponsor);
	double h = ::hash(hashInput, agent.getLeaderID());
	// 0.25 if pleased, 0.5 cautious, 1 furious
	double attitudeModifier = (ATTITUDE_FRIENDLY - towardSponsor) /
			(double)ATTITUDE_FRIENDLY;
	// Mates' rates, but still obscured (!= 0).
	if(towardSponsor == ATTITUDE_FRIENDLY)
		attitudeModifier = -0.25;
	/*  Put our money where our mouth is: discount for war on our worst enemy
		(other than that, our attitude toward the target is sufficiently reflected
		by war utility) */
	if(agent.AI_getWorstEnemy() == targetId) {
		attitudeModifier -= 0.25;
		if(attitudeModifier == 0) // Avoid 0 for obscurity
			attitudeModifier -= 0.25;
	}
	double obscured = price + price * attitudeModifier * 0.6 * h;
	int r = ::roundToMultiple(obscured, 10); // Makes gold cost a multiple of 5
	rep.log("Obscured price: %d (attitude modifier: %d percent)\n", r,
			::round(100 * attitudeModifier));
	return r;
}

DenialTypes UWAI::Team::makePeaceTrade(TeamTypes enemyId, TeamTypes brokerId) const {

	/*  Can't broker in a war that they're involved in (not checked by caller;
		BtS allows it). */
	if(GET_TEAM(brokerId).isAtWar(enemyId))
		return DENIAL_JOKING;
	bool const bEnemyWillTalk = GET_PLAYER(GET_TEAM(agentId).getLeaderID()).
			canContact(GET_TEAM(enemyId).getLeaderID(), true);
	if(!bEnemyWillTalk && !gDLL->isDiplomacy()) {
		// Don't waste time with a more specific answer if human won't read it anyway
		return DENIAL_RECENT_CANCEL;
	}
	int ourReluct = reluctanceToPeace(enemyId);
	int enemyReluct = GET_TEAM(enemyId).uwai().reluctanceToPeace(agentId);
	bool bNoDenial = false; // Will still have to check bEnemyWillTalk then
	if(enemyReluct <= 0) {
		if(ourReluct < 55)
			bNoDenial = true;
		else {
			CvGameAI const& g = GC.AI_getGame();
			scaled scoreRatio(g.getTeamScore(agentId),
					g.getTeamScore(g.getRankTeam(0)));
			scaled const gameEra = g.AI_getCurrEraFactor();
			if(gameEra > 0 &&
					scoreRatio < ((gameEra - 1) / gameEra + fixp(2/3.)) / 2) {
				// We're not doing well in score; the broker might be doing much better.
				if(ourReluct < 70)
					bNoDenial = true;
				else return DENIAL_TOO_MUCH;
			}
			return DENIAL_VICTORY;
		}
	}
	if(!bEnemyWillTalk) {
		if (ourReluct <= 0 && enemyReluct > 0 && enemyReluct - ourReluct >= 15) {
			/*	They'll refuse with "not right now", so it's a bit pointless to
				"contact them", but if _we_ say "not right now" it'll be misleading
				b/c we probably won't be able to make peace even once they become
				willing to talk. */
			return DENIAL_CONTACT_THEM;
		}
		return DENIAL_RECENT_CANCEL;
	}
	if(bNoDenial)
		return NO_DENIAL;
	/*  Unusual case: both sides want to continue, so both would have to be paid
		by the broker, which is too complicated. "Not right now" is true enough -
		will probably change soon. */
	if(ourReluct > 0)
		return DENIAL_RECENT_CANCEL;
	return DENIAL_CONTACT_THEM;
}

int UWAI::Team::makePeaceTradeVal(TeamTypes enemyId, TeamTypes brokerId) const {

	int ourReluct = reluctanceToPeace(enemyId);
	// Demand at least a token payment equivalent to 3 war utility
	double r = utilityToTradeVal(std::max(3, ourReluct));
	CvTeamAI const& agent = GET_TEAM(agentId);
	AttitudeTypes towardBroker = agent.AI_getAttitude(brokerId);
	// Make it a bit easier to broker peace as part of a peace treaty
	if(agent.isAtWar(brokerId) && towardBroker < ATTITUDE_CAUTIOUS)
		towardBroker = ATTITUDE_CAUTIOUS;
	/*  Between 400% and 77%. 100% when we're pleased with both.
		We prefer to get even with our enemy, so letting the broker pay 1 for 1 is
		already a concession. */
	double attitudeModifier = std::min(10.0 / (2 * towardBroker +
			GET_TEAM(enemyId).AI_getAttitude(brokerId) + 1), 4.0);
	int warDuration = agent.AI_getAtWarCounter(enemyId);
	FAssert(warDuration > 0);
	/*  warDuration could be as small as 1 I think. Then the mark-up is
		+175% in the Ancient era. None for a Renaissance war after 15 turns. */
	double timeModifier = std::max(0.75,
			(5.5 - agent.AI_getCurrEraFactor().getDouble() / 2) /
			std::sqrt(warDuration + 1.0));
	r = r * attitudeModifier * timeModifier;
	return agent.AI_roundTradeVal(::round(r));
}

int UWAI::Team::endWarVal(TeamTypes enemyId) const {

	bool agentHuman = GET_TEAM(agentId).isHuman();
	FAssertMsg(agentHuman || GET_TEAM(enemyId).isHuman(),
				"This should only be called for human-AI peace");
	CvTeamAI const& human = (agentHuman ? GET_TEAM(agentId) : GET_TEAM(enemyId));
	CvTeamAI const& ai =  (agentHuman ? GET_TEAM(enemyId) : GET_TEAM(agentId));
	int aiReluct = ai.uwai().reluctanceToPeace(human.getID(), false);
	// If no payment is possible, human utility shouldn't matter.
	if(aiReluct <= 0 &&
			!GET_PLAYER(human.getLeaderID()).
			canPossiblyTradeItem(ai.getLeaderID(), TRADE_GOLD) &&
			!GET_PLAYER(human.getLeaderID()).
			canPossiblyTradeItem(ai.getLeaderID(), TRADE_TECHNOLOGIES))
		return 0;
	// Really just utility given how peaceThreshold is computed for humans
	int humanUtility = human.uwai().reluctanceToPeace(ai.getID(), false);
	/*	Neither side pays if both want peace and the AI wants it
		more badly than the human - but not far more badly. */
	if(humanUtility <= 0 && aiReluct < humanUtility && aiReluct >= 2 * humanUtility)
		return 0;
	double reparations = 0;
	// Only AI wants to end the war or AI wants to end it much more badly
	if(aiReluct < 0 && (humanUtility >= 0 || aiReluct < 2 * humanUtility)) {
		// Human pays nothing if AI pays
		if(agentHuman)
			return 0;
		if(humanUtility >= 0) {
			// Rely more on war utility of the AI side than on human war utility
			reparations = 0.5 * (std::min(-aiReluct, humanUtility) - aiReluct);
		}
		// Both negative: A rather symbolic payment - unless the war is disastrous for the AI
		else reparations = humanUtility - 0.5 * aiReluct;
		/*  What if human declares war, but never sends units, although we believe
			that it would hurt us and benefit them (all things considered)?
			Then we're probably wrong somewhere and shouldn't trust our
			utility computations. */
		if(ai.AI_getMemoryCount(human.getID(), MEMORY_DECLARED_WAR) > 0 &&
				ai.getNumCities() > 0) {
			int wsDelta = std::max(0, human.AI_getWarSuccess(ai.getID()) -
					ai.AI_getWarSuccess(human.getID()));
			double wsAdjustment = std::min(1.0, (4.0 * wsDelta) /
					(GC.getWAR_SUCCESS_CITY_CAPTURING() * ai.getNumCities()));
			reparations *= wsAdjustment;
		}
		reparations = ai.uwai().reparationsToHuman(reparations);
		reparations *= UWAI::reparationsAIPercent / 100.0;
	}
	else {
		// AI pays nothing if human pays
		if(!agentHuman)
			return 0;
		// Don't demand payment if willing to capitulate to human
		if(ai.AI_surrenderTrade(human.getID(), CvTeamAI::VASSAL_POWER_MOD_SURRENDER,
				false) == NO_DENIAL)
			return 0;
		reparations = 0;
		// (No limit on human reparations)
		// This is enough to make peace worthwhile for the AI
		if(aiReluct > 0)
			reparations = ai.uwai().utilityToTradeVal(aiReluct);
		/*  But if human wants to end the war more badly than AI, AI also tries
			to take advantage of that. */
		if(humanUtility < 0) {
			/*  How much we try to squeeze the human. Not much b/c trade values
				go a lot higher now than they do in BtS. 5 utility can easily
				correspond to 1000 gold in the midgame. The AI evaluation of human
				utility isn't too reliable (may well assume that the human starts
				some costly but futile offensive) and fluctuates a lot from turn
				to turn, whereas peace terms mustn't fluctuate too much.
				And at least if aiReluct < 0, we do want peace negotiations to
				succeed. */
			double greedFactor = 0.05;
			if(aiReluct > 0)
				greedFactor += 0.1;
			int delta = std::min(0, aiReluct) - humanUtility;
			if (delta == 0)
				return 0;
			FAssert(delta > 0);
			// Conversion based on human's economy
			reparations += greedFactor * human.uwai().utilityToTradeVal(delta);
			reparations *= UWAI::reparationsHumanPercent / 100.0;
			/*  Demand less if human has too little. Akin to the
				tech/gold trading clause higher up. */
			if(aiReluct < 0 && human.getNumMembers() == 1) {
				int maxHumanCanPay = -1;
				ai.uwai().leaderUWAI().canTradeAssets(
						::round(reparations), human.getLeaderID(),
						&maxHumanCanPay);
				if(maxHumanCanPay < reparations) {
					/*  This means that the human player may want to make peace
						right away when the AI becomes willing to talk b/c the
						AI could change its mind again on the next turn. */
					if(::hash((GC.getGame().getGameTurn() -
							/*  Integer division to avoid flickering, e.g.
								when aiReluct/-50.0 is about 0.5. Don't just
								hash game turn b/c that would mean that the
								AI can change its mind only every so many turns -
								too predictable. */
							ai.AI_getWarSuccessRating()) / 8,
							ai.getLeaderID()) < aiReluct / -40.0)
						reparations = maxHumanCanPay;
				}
			}
		}
	}
	return ::round(reparations);
}

int UWAI::Team::uEndAllWars(VoteSourceTypes vs) const {

	vector<TeamTypes> warEnemies;
	for(TeamIter<FREE_MAJOR_CIV,ENEMY_OF> it(agentId); it.hasNext(); ++it) {
		if(vs == NO_VOTESOURCE || it->isVotingMember(vs))
			warEnemies.push_back(it->getID());
	}
	if(warEnemies.empty()) {
		FAssert(!warEnemies.empty());
		return 0;
	}
	bool silent = !isReportTurn();
	UWAIReport rep(silent);
	if(!silent) {
		rep.log("h3.\nPeace vote\n");
		rep.log("%s is evaluating the utility of war against %s in order to "
				"decide whether to vote for peace between self and everyone",
				rep.teamName(agentId), rep.teamName(warEnemies[0]));
	}
	WarEvalParameters params(agentId, warEnemies[0], rep);
	for(size_t i = 1; i < warEnemies.size(); i++) {
		params.addExtraTarget(warEnemies[i]);
		rep.log("War enemy: %s", rep.teamName(warEnemies[i]));
	}
	WarEvaluator eval(params);
	int r = -eval.evaluate();
	rep.log(""); // newline
	return r;
}

int UWAI::Team::uJointWar(TeamTypes targetId, VoteSourceTypes vs) const {

	bool silent = !isReportTurn();
	UWAIReport rep(silent);
	if(!silent) {
		rep.log("h3.\nWar vote\n");
		rep.log("%s is evaluating the utility of war against %s through diplo vote",
				rep.teamName(agentId),
				rep.teamName(targetId));
	}
	vector<TeamTypes> allies;
	for(PlayerIter<FREE_MAJOR_CIV,POTENTIAL_ENEMY_OF> it(targetId); it.hasNext(); ++it) {
		CvPlayer const& ally = *it;
		if(ally.isVotingMember(vs) && ally.getTeam() != agentId &&
				!GET_TEAM(ally.getTeam()).isAtWar(targetId)) {
			rep.log("%s would join as a war ally", rep.leaderName(ally.getID()));
			allies.push_back(ally.getTeam());
		}
	}
	WarEvalParameters params(agentId, targetId, rep);
	for(size_t i = 0; i < allies.size(); i++)
		params.addWarAlly(allies[i]);
	params.setImmediateDoW(true);
	WarPlanTypes wp = WARPLAN_LIMITED;
	// (CvPlayerAI::AI_diploVote actually rules this out)
	if(GET_TEAM(agentId).isAtWar(targetId)) {
		params.setNotConsideringPeace();
		wp = NO_WARPLAN; // evaluate the current plan
	}
	WarEvaluator eval(params);
	int r = eval.evaluate(wp);
	rep.log(""); // newline
	return r;
}

int UWAI::Team::uEndWar(TeamTypes enemyId) const {

	UWAIReport silentReport(true);
	WarEvalParameters params(agentId, enemyId, silentReport);
	WarEvaluator eval(params);
	return -eval.evaluate();
}

double UWAI::Team::reparationsToHuman(double u) const {

	double r = u;
	/*  maxReparations is the upper limit for inter-AI peace;
		be less generous with humans */
	double const top = (4 * maxReparations) / 5;
	/*  If utility for reparations is above the cap, we become less
		willing to pay b/c we don't believe that the peace can last. */
	if(r > top) {
		double const bottom = top / 4.0;
		double const gradient = -1/6.0;
		double delta = (r - top);
		/*	Decreasing linearly from top to bottom seems a bit too
			steep for small delta. Choose an exponent and divisor
			so that the delta for which r reaches the bottom
			remains unchanged (fixpoint). */
		double const deltaBottom = (-1 / gradient) * (top - bottom);
		double const div = 3;
		double const exponent = std::log(deltaBottom * div) / std::log(deltaBottom);
		delta = std::pow(delta, exponent) / div;
		r = std::max(bottom, top + delta * gradient);
	}
	// Trade value based on our economy
	return utilityToTradeVal(r);
}

void UWAI::Team::respondToRebuke(TeamTypes targetId, bool prepare) {

	/*  Caveat: Mustn't use PRNG here b/c this is called from both async (prepare=false)
		and sync (prepare=true) contexts */
	CvTeamAI& agent = GET_TEAM(agentId);
	if(!canSchemeAgainst(targetId, true) || (prepare ?
			agent.AI_isSneakAttackPreparing(targetId) :
			agent.AI_isSneakAttackReady(targetId)))
		return;
	if(!prepare && !agent.canDeclareWar(targetId))
		return;
	FAssert(GET_TEAM(targetId).isHuman());
	UWAIReport silentReport(true);
	WarEvalParameters params(agentId, targetId, silentReport);
	WarEvaluator eval(params);
	int u = eval.evaluate(prepare ? WARPLAN_PREPARING_LIMITED : WARPLAN_LIMITED);
	if(u < 0)
		return;
	if(prepare)
		agent.AI_setWarPlan(targetId, WARPLAN_PREPARING_LIMITED);
	else agent.AI_setWarPlan(targetId, WARPLAN_LIMITED);
}

DenialTypes UWAI::Team::acceptVassal(TeamTypes vassalId) const {

	vector<TeamTypes> warEnemies; // Just the new ones
	for(TeamIter<FREE_MAJOR_CIV,ENEMY_OF> it(vassalId); it.hasNext(); ++it) {
		CvTeam const& t = *it;
		if(!t.isAtWar(agentId)) {
			warEnemies.push_back(t.getID());
			FAssert(t.isHasMet(agentId)); // vassalId shouldn't ask us otherwise
		}
	}
	if(warEnemies.empty()) {
		FAssert(!warEnemies.empty());
		return NO_DENIAL;
	}
	/*  Ideally, WarEvaluator would have a mode for assuming a vassal agreement,
		and would do everything this function needs. I didn't think of this early
		enough. As it is, WarEvaluator can handle the resulting wars well enough,
		but won't account for the utility of us gaining a vassal, nor for any
		altruistic desire to protect the vassal. (Assistance::evaluate isn't
		altruistic.)
		GreedForVassals::evaluate has quite sophisticated code for evaluating
		vassals, but can't be easily separated from the context of WarEvaluator.
		I'm using only the cached part of that computation. */
	bool silent = GET_TEAM(agentId).isHuman() || !isReportTurn();
	UWAIReport rep(silent);
	if(!silent) {
		rep.log("h3.\nConsidering war to accept vassal\n");
		rep.log("%s is considering to accept %s as its vassal; implied DoW on:",
				rep.teamName(agentId), rep.teamName(vassalId));
		for(size_t i = 0; i < warEnemies.size(); i++)
			rep.log("%s", rep.teamName(warEnemies[i]));
	}
	int resourceScore = 0;
	int techScore = 0;
	for(MemberIter it(vassalId); it.hasNext(); ++it) {
		PlayerTypes vassalMemberId = it->getID();
		resourceScore += leaderUWAI().getCache().vassalResourceScore(vassalMemberId);
		techScore += leaderUWAI().getCache().vassalTechScore(vassalMemberId);
	}
	// resourceScore is already utility
	double vassalUtility = leaderUWAI().tradeValToUtility(techScore) + resourceScore;
	rep.log("%d utility from vassal resources, %d from tech", ::round(resourceScore),
		::round(vassalUtility - resourceScore));
	CvTeamAI const& agent = GET_TEAM(agentId);
	vassalUtility += (GET_TEAM(vassalId).getNumCities() * 30.0) /
			(agent.getNumCities() + 1);
	if(agent.AI_anyMemberAtVictoryStage(AI_VICTORY_DIPLOMACY4 | AI_VICTORY_CONQUEST4))
			vassalUtility *= 2;
	else if(agent.AI_anyMemberAtVictoryStage(AI_VICTORY_DIPLOMACY3 | AI_VICTORY_CONQUEST3))
		vassalUtility *= 1.4;
	/*  If the war will go badly for us, we'll likely not be able to protect
		the vassal. vassalUtility therefore mustn't be so high that it could
		compensate for a lost war; should only compensate for bad diplo and
		military build-up.
		Except, maybe, if we're Friendly toward the vassal (see below). */
	vassalUtility = std::min(25.0, vassalUtility);
	rep.log("Utility after adding vassal cities: %d", ::round(vassalUtility));
	/*  CvTeamAI::AI_vassalTrade already does an attitude check - we know we don't
		_dislike_ the vassal */
	if(GET_TEAM(agentId).AI_getAttitude(vassalId) >= ATTITUDE_FRIENDLY) {
		vassalUtility += 5;
		rep.log("Utility increased b/c of attitude");
	}
	//UWAIReport silentReport(true); // use this one for fewer details
	WarEvalParameters params(agentId, warEnemies[0], rep);
	for(size_t i = 1; i < warEnemies.size(); i++)
		params.addExtraTarget(warEnemies[i]);
	params.setImmediateDoW(true);
	WarEvaluator eval(params);
	int warUtility = eval.evaluate(WARPLAN_LIMITED);
	rep.log("War utility: %d", warUtility);
	int totalUtility = ::round(vassalUtility) + warUtility;
	if(totalUtility > 0) {
		rep.log("Accepting vassal\n");
		return NO_DENIAL;
	}
	rep.log("Vassal not accepted\n");
	// Doesn't matter which denial; no one gets to read this
	return DENIAL_POWER_THEM;
}

bool UWAI::Team::isLandTarget(TeamTypes theyId) const {

	PROFILE_FUNC();
	bool hasCoastalCity = false;
	bool canReachAnyByLand = false;
	int distLimit = getUWAI.maxLandDist();
	for(MemberIter it(agentId); it.hasNext(); ++it) {
		UWAICache const& cache = it->uwai().getCache();
		// Sea route then unlikely to be much slower
		if(!cache.canTrainDeepSeaCargo())
			distLimit = MAX_INT;
		for(int j = 0; j < cache.size(); j++) {
			UWAICache::City& c = cache.cityAt(j);
			if(c.city().getTeam() != theyId)
				continue;
			if(c.city().isCoastal())
				hasCoastalCity = true;
			if(c.canReachByLand()) {
				canReachAnyByLand = true;
				if(c.getDistance() <= distLimit)
					return true;
			}
		}
	}
	/*  Tactical AI can't do naval assaults on inland cities. Assume that landlocked
		civs are land targets even if they're too far away; better than treating
		them as entirely unreachable. */
	if(!hasCoastalCity && canReachAnyByLand)
		return true;
	return false;
}

void UWAI::Team::startReport() {

	bool doReport = isReportTurn();
	report = new UWAIReport(!doReport);
	if(!doReport)
		return;
	int year = GC.getGame().getGameTurnYear();
	report->log("h3.");
	report->log("Year %d, %s:", year, report->teamName(agentId));
	for(MemberIter it(agentId); it.hasNext(); ++it)
		report->log(report->leaderName(it->getID(), 16));
	report->log("");
}

void UWAI::Team::closeReport() {

	report->log("\n");
	SAFE_DELETE(report);
}

void UWAI::Team::setForceReport(bool b) {

	bForceReport = b;
}

bool UWAI::Team::isReportTurn() const {

	if(bForceReport)
		return true;
	int turnNumber = GC.getGame().getGameTurn();
	static int const reportInterval = GC.getDefineINT("REPORT_INTERVAL");
	return (reportInterval > 0 && turnNumber % reportInterval == 0);
}

void UWAI::Team::showWarPrepStartedMsg(TeamTypes targetId) {

	showWarPlanMsg(targetId, "TXT_KEY_WAR_PREPARATION_STARTED");
}

void UWAI::Team::showWarPlanAbandonedMsg(TeamTypes targetId) {

	showWarPlanMsg(targetId, "TXT_KEY_WAR_PLAN_ABANDONED");
}

void UWAI::Team::showWarPlanMsg(TeamTypes targetId, char const* txtKey) {

	CvPlayer& activePl = GET_PLAYER(GC.getGame().getActivePlayer());
	if(!activePl.isSpectator() || !GC.getDefineBOOL("UWAI_SPECTATOR_ENABLED"))
		return;
	CvWString szBuffer = gDLL->getText(txtKey,
			GET_TEAM(agentId).getName().GetCString(),
			GET_TEAM(targetId).getName().GetCString());
	gDLL->UI().addMessage(activePl.getID(), false, -1, szBuffer,
			0, MESSAGE_TYPE_MAJOR_EVENT,  // <advc.127b>
			NULL, NO_COLOR, GET_TEAM(agentId).getCapitalX(activePl.getTeam(), true),
			GET_TEAM(agentId).getCapitalY(activePl.getTeam(), true)); // </advc.127b>
}

UWAICache& UWAI::Team::leaderCache() {

	return GET_PLAYER(GET_TEAM(agentId).getLeaderID()).uwai().getCache();
}

UWAICache const& UWAI::Team::leaderCache() const {
	// Duplicate code; see also UWAICache::leaderCache
	return GET_PLAYER(GET_TEAM(agentId).getLeaderID()).uwai().getCache();
}

double UWAI::Team::confidenceFromWarSuccess(TeamTypes targetId) const {

	/*  Need to be careful not to overestimate
		early successes (often from a surprise attack). Bound the war success
		ratio based on how long the war lasts and the extent of successes. */
	CvTeamAI const& t = GET_TEAM(agentId);
	CvTeamAI const& target = GET_TEAM(targetId);
	int timeAtWar = t.AI_getAtWarCounter(targetId);
	/*  Can differ by 1 b/c of turn difference. Can apparently also differ by 2
		somehow, which I don't understand, but it's not a problem really. */
	FAssert(std::abs(timeAtWar - target.AI_getAtWarCounter(agentId)) <= 2);
	if(timeAtWar <= 0)
		return -1;
	int ourSuccess = std::max(1, t.AI_getWarSuccess(targetId));
	int theirSuccess = std::max(1, target.AI_getWarSuccess(agentId));
	float successRatio = ((float)ourSuccess) / theirSuccess;
	float const fixedBound = 0.5;
	// Reaches fixedBound after 20 turns
	float timeBasedBound = (100 - 2.5f * timeAtWar) / 100.0f;
	/*  Total-based bound: Becomes relevant once a war lasts long; e.g. after
		25 turns, in the Industrial era, will need a total war success of 250
		in order to reach fixedBound. Neither side should feel confident if there
		isn't much action. */
	float progressFactor = std::max(3.0f,
			11 - GET_TEAM(agentId).AI_getCurrEraFactor().getFloat() * 1.5f);
	float totalBasedBound = (100 - (progressFactor *
			(ourSuccess + theirSuccess)) / timeAtWar) / 100;
	float r = successRatio;
	r = ::range(r, fixedBound, 2 - fixedBound);
	r = ::range(r, timeBasedBound, 2 - timeBasedBound);
	r = ::range(r, totalBasedBound, 2 - totalBasedBound);
	return r;
}

void UWAI::Team::reportWarEnding(TeamTypes enemyId,
		CLinkList<TradeData> const* weReceive, CLinkList<TradeData> const* wePay) {

	/*  This isn't really team-on-team data b/c each team member can have its
		own interpretation of whether the war was successful. */
	for(MemberIter it(agentId); it.hasNext(); ++it)
		it->uwai().getCache().reportWarEnding(enemyId, weReceive, wePay);
}

int UWAI::Team::countNonMembers(VoteSourceTypes voteSource) const {

	int r = 0;
	for(PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it) {
		if(!it->isVotingMember(voteSource))
			r++;
	}
	return r;
}

bool UWAI::Team::isPotentialWarEnemy(TeamTypes tId) const {

	return canSchemeAgainst(tId, true) || (!GET_TEAM(tId).isAVassal() &&
			!GET_TEAM(agentId).isAVassal() && GET_TEAM(tId).isAtWar(agentId));
}

bool UWAI::Team::isFastRoads() const {

	return (GET_TEAM(agentId).getRouteChange((RouteTypes)0) <= -10);
}

UWAI::Civ::Civ() {

	weId = NO_PLAYER;
}

void UWAI::Civ::init(PlayerTypes weId) {

	this->weId = weId;
	cache.init(weId);
}

void UWAI::Civ::uninit() {

	cache.uninit();
}

void UWAI::Civ::turnPre() {

	cache.update();
}

void UWAI::Civ::write(FDataStreamBase* stream) {

	stream->Write(GET_PLAYER(weId).getID());
	cache.write(stream);
}

void UWAI::Civ::read(FDataStreamBase* stream) {

	int tmp;
	stream->Read(&tmp);
	weId = (PlayerTypes)tmp;
	cache.read(stream);
}

bool UWAI::Civ::considerDemand(PlayerTypes theyId, int tradeVal) const {

	CvPlayerAI const& we = GET_PLAYER(weId);
	// When furious, they'll have to take it from our cold dead hands
	if(we.AI_getAttitude(theyId) <= ATTITUDE_FURIOUS)
		return false;
	/*  (I don't think the interface even allows demanding tribute when there's a
		peace treaty) */
	if(!GET_TEAM(theyId).canDeclareWar(we.getTeam()))
		return false;
	UWAIReport silentReport(true);
	WarEvalParameters ourParams(we.getTeam(), TEAMID(theyId), silentReport);
	WarEvaluator ourEval(ourParams);
	double ourUtility = ourEval.evaluate(WARPLAN_LIMITED);
	// Add -5 to 40 for self-assertion
	ourUtility += 45 * prideRating() - 5;
	/*  The more prior demands we remember, the more recalcitrant we become
		(does not count the current demand) */
	int iMemoryDemand = we.AI_getMemoryCount(theyId, MEMORY_MADE_DEMAND);
	if (iMemoryDemand > 0)
		ourUtility += std::pow((double)iMemoryDemand, 2);
	if(ourUtility >= 0) // Bring it on!
		return false;
	WarEvalParameters theirParams(TEAMID(theyId), we.getTeam(), silentReport);
	WarEvaluator theirEval(theirParams);
	/*  If they don't intend to attack soon, the peace treaty from tribute
		won't help us. Total war scares us more than limited. */
	int theirUtility = theirEval.evaluate(WARPLAN_TOTAL);
	if(theirUtility < 0)
		return false; // Call their bluff
	// Willing to pay at most this much
	double paymentCap = GET_TEAM(weId).uwai().reparationsToHuman(
			// Interpret theirUtility as a probability of attack
			-ourUtility * 2 * (4 + theirUtility) / 100.0);
	return paymentCap >= (double)tradeVal;
	/*  Some randomness? None in the BtS code (AI_considerOffer) either.
		Would have to use ::hash. */
}

bool UWAI::Civ::amendTensions(PlayerTypes humanId) const {

	FAssert(GET_TEAM(weId).getLeaderID() == weId);
	CvPlayerAI& we = GET_PLAYER(weId);
	// Lower contact probabilities in later eras
	double const era = we.AI_getCurrEraFactor().getDouble();
	CvLeaderHeadInfo const& lh = GC.getInfo(we.getPersonalityType());
	if(we.AI_getAttitude(humanId) <= lh.getDemandTributeAttitudeThreshold()) {
		FOR_EACH_ENUM(AIDemand) {
			int cr = lh.getContactRand(CONTACT_DEMAND_TRIBUTE);
			if(cr > 0) {
				double pr = (8.5 - era) / (2 * cr);
				// Excludes Gandhi (cr=10000 => pr<0.0005)
				if(pr > 0.001 && ::bernoulliSuccess(pr, "advc.104 (trib)") &&
						we.AI_demandTribute(humanId, eLoopAIDemand))
				return true;
			}
			else FAssert(cr > 0);
		}
	}
	else {
		int cr = lh.getContactRand(CONTACT_ASK_FOR_HELP);
		if(cr > 0) {
			double pr = (5.5 - era) / (1.25 * cr);
			if(::bernoulliSuccess(pr, "advc.104 (help)") && we.AI_askHelp(humanId))
				return true;
		}
		else FAssert(cr > 0);
	}
	int crReligion = lh.getContactRand(CONTACT_RELIGION_PRESSURE);
	int crCivics = lh.getContactRand(CONTACT_CIVIC_PRESSURE);
	if(crReligion <= crCivics) {
		if(crReligion > 0) {
			double pr = (8.0 - era) / crReligion;
			if(::bernoulliSuccess(pr, "advc.104 (relig)") &&
					we.AI_contactReligion(humanId))
				return true;
		}
		else FAssert(crReligion > 0);
	} else {
		if(crCivics > 0) {
			double pr = 2.5 / crCivics;
			// Exclude Saladin (cr=10000)
			if(pr > 0.001 && ::bernoulliSuccess(pr, "advc.104 (civic)") &&
					we.AI_contactCivics(humanId))
				return true;
		}
		else FAssert(crCivics > 0);
	}
	//  Embargo request - too unlikely to succeed I think.
	/*int cr = lh.getContactRand(CONTACT_STOP_TRADING);
	if(cr > 0) {
		double pr = ?;
		if(::bernoulliSuccess(pr) && we.AI_proposeEmbargo(humanId))
			return true;
	}
	else FAssert(cr > 0); */
	return false;
}

bool UWAI::Civ::considerGiftRequest(PlayerTypes theyId, int tradeVal) const {

	/*  Just check war utility and peace treaty here; all the other conditions
		are handled by CvPlayerAI::AI_considerOffer. */
	if(GET_TEAM(weId).isForcePeace(TEAMID(theyId)) && GET_PLAYER(weId).
			AI_getMemoryAttitude(theyId, MEMORY_GIVE_HELP) <= 0)
		return false;
	// If war not possible, might as well sign a peace treaty
	if(!GET_TEAM(weId).canDeclareWar(TEAMID(theyId)) ||
			!GET_TEAM(theyId).canDeclareWar(TEAMID(weId)))
		return true;
	CvPlayerAI const& we = GET_PLAYER(weId);
	/*  Accept probabilistically regardless of war utility (so long as we're
		not planning war yet, which the caller ensures).
		Probability to accept is 45% for Gandhi, 0% for Tokugawa. */
	double prSuccess = 0.5 - we.AI_prDenyHelp().getDouble();
	// Can't use sync'd RNG here, but don't want the outcome to change after reload.
	std::vector<int> inputs;
	inputs.push_back(GC.getGame().getGameTurn());
	inputs.push_back(tradeVal);
	if(::hash(inputs, weId) < prSuccess)
		return true;
	// Probably won't want to attack theyId then
	if(GET_TEAM(weId).AI_isSneakAttackReady())
		return true;
	UWAIReport silentReport(true);
	WarEvalParameters params(we.getTeam(), TEAMID(theyId), silentReport);
	WarEvaluator eval(params);
	double u = eval.evaluate(WARPLAN_LIMITED, 5) - 5; // minus 5 for goodwill
	if(u >= 0)
		return false;
	double thresh = utilityToTradeVal(-u);
	return thresh >= tradeVal;
}
// Wrapper that handles the war evaluator cache
int UWAI::Civ::willTalk(PlayerTypes theyId, int atWarCounter, bool useCache) const {

	if(useCache)
		WarEvaluator::enableCache();
	int r = willTalk(theyId, atWarCounter);
	if(useCache)
		WarEvaluator::disableCache();
	return r;
}

int UWAI::Civ::willTalk(PlayerTypes theyId, int atWarCounter) const {

	CvTeamAI const& agent = GET_TEAM(weId);
	if(agent.AI_surrenderTrade(TEAMID(theyId)) == NO_DENIAL)
		return 1;
	// 1 turn RTT and let the team leader handle peace negotiation
	if(atWarCounter <= 1 || agent.getLeaderID() != weId)
		return -1;
	// valid=true: want to return 1, but still need to check for DECLARED_WAR_RECENT.
	bool valid = false;
	/*  Checking for a possible peace deal only serves as a convenience for
		human players; no need to do it for AI-AI peace. */
	if(!GET_PLAYER(theyId).isHuman())
		valid = true;
	else valid = (agent.AI_surrenderTrade(TEAMID(theyId)) == NO_DENIAL ||
			isPeaceDealPossible(theyId));
	if(GET_PLAYER(weId).AI_getMemoryCount(theyId, MEMORY_DECLARED_WAR_RECENT) > 0) {
		if(valid)
			return 0;
		return -1;
	}
	return (valid ? 1 : -1);
}

bool UWAI::Civ::isPeaceDealPossible(PlayerTypes humanId) const {

	/*  Could simply call CvPlayerAI::AI_counterPropose, but I think there are rare
		situations when a deal is possible but AI_counterPropose doesn't find it.
		It would also be slower. */
	// <advc.705>
	CvGame const& g = GC.getGame();
	if(g.isOption(GAMEOPTION_RISE_FALL) &&
			g.getRiseFall().isCooperationRestricted(weId) &&
			GET_TEAM(weId).uwai().reluctanceToPeace(TEAMID(humanId)) >= 20)
		return false;
	// </advc.705>
	int targetTradeVal = GET_TEAM(humanId).uwai().endWarVal(TEAMID(weId));
	if(targetTradeVal <= 0)
		return true;
	return canTradeAssets(targetTradeVal, humanId);
}

bool UWAI::Civ::canTradeAssets(int targetTradeVal, PlayerTypes humanId, int* r,
		// (advc.ctr: Now unused b/c the AI will always accept cities as payment
		bool ignoreCities) const {

	int totalTradeVal = 0;
	CvPlayerAI const& human = GET_PLAYER(humanId);
	if(human.canTradeItem(weId, TradeData(TRADE_GOLD, human.getGold()), true))
		totalTradeVal += human.getGold();
	if(totalTradeVal >= targetTradeVal && r == NULL)
		return true;
	FOR_EACH_ENUM(Tech) {
		if(human.canTradeItem(weId, TradeData(TRADE_TECHNOLOGIES, eLoopTech), true)) {
			totalTradeVal += GET_TEAM(weId).AI_techTradeVal(eLoopTech,
					human.getTeam(), true, true);
			if(totalTradeVal >= targetTradeVal && r == NULL)
				return true;
		}
	}
	int cityLimit = (int)std::ceil(human.getNumCities() / 6.0);
	int cityCount = 0;
	if(!ignoreCities) {
		FOR_EACH_CITYAI(c, human) {
			if(human.canTradeItem(weId, TradeData(TRADE_CITIES, c->getID()), true)) {
				if(totalTradeVal < targetTradeVal)
					cityCount++;
				totalTradeVal += GET_PLAYER(weId).AI_cityTradeVal(*c);
				if(cityCount > cityLimit) {
					if(r == NULL)
						return false;
				}
				else if(totalTradeVal >= targetTradeVal && r == NULL)
					return true;
			}
		}
	}
	if(r != NULL) {
		*r = totalTradeVal;
		return false;
	}
	return (cityCount <= cityLimit && totalTradeVal >= targetTradeVal);
}

double UWAI::Civ::tradeValUtilityConversionRate() const {

	// Based on how long it would take us to produce as much trade value
	double speedFactor = 1;
	int trainPercent = GC.getInfo(GC.getGame().getGameSpeedType()).
			getTrainPercent();
	if(trainPercent > 0)
		speedFactor = 100.0 / trainPercent;
	CvPlayer const& we = GET_PLAYER(weId);
	return (3 * speedFactor) / (std::max(10.0, we.estimateYieldRate(YIELD_COMMERCE))
			+ 2 * std::max(1.0, we.estimateYieldRate(YIELD_PRODUCTION)));
	/*  Note that change advc.004s excludes espionage and culture from the
		Economy history, and estimateYieldRate(YIELD_COMMERCE) doesn't account
		for these yields either. Not a problem for culture, I think, which is
		usually produced in addition to gold and research, but the economic output
		of civs running the Big Espionage strategy will be underestimated.
		Still better than just adding up all commerce types. */
}

double UWAI::Civ::utilityToTradeVal(double u) const {

	return GET_TEAM(weId).uwai().utilityToTradeVal(u);
}

double UWAI::Civ::tradeValToUtility(double tradeVal) const {

	return GET_TEAM(weId).uwai().tradeValToUtility(tradeVal);
}

double UWAI::Civ::amortizationMultiplier() const {

	// 25 turns delay b/c war planning is generally about a medium-term future
	return GET_PLAYER(weId).AI_amortizationMultiplier(25);
}

// Currently unused; don't remember what I wanted to use it for.
bool UWAI::Civ::isNearMilitaryVictory(int stage) const {

	if(stage <= 0)
		return true;
	CvPlayerAI& we = GET_PLAYER(weId);
	switch(stage) {
		case 1: return we.AI_atVictoryStage(AI_VICTORY_MILITARY1);
		case 2: return we.AI_atVictoryStage(AI_VICTORY_MILITARY2);
		case 3: return we.AI_atVictoryStage(AI_VICTORY_MILITARY3);
		case 4: return we.AI_atVictoryStage(AI_VICTORY_MILITARY4);
		default: return false;
	}
}

int UWAI::Civ::getConquestStage() const {

	CvPlayerAI& we = GET_PLAYER(weId);
	if(we.AI_atVictoryStage(AI_VICTORY_CONQUEST1))
		return 1;
	if(we.AI_atVictoryStage(AI_VICTORY_CONQUEST2))
		return 2;
	if(we.AI_atVictoryStage(AI_VICTORY_CONQUEST3))
		return 3;
	if(we.AI_atVictoryStage(AI_VICTORY_CONQUEST4))
		return 4;
	return 0;
}

int UWAI::Civ::getDominationStage() const {

	CvPlayerAI& we = GET_PLAYER(weId);
	if(we.AI_atVictoryStage(AI_VICTORY_DOMINATION1))
		return 1;
	if(we.AI_atVictoryStage(AI_VICTORY_DOMINATION2))
		return 2;
	if(we.AI_atVictoryStage(AI_VICTORY_DOMINATION3))
		return 3;
	if(we.AI_atVictoryStage(AI_VICTORY_DOMINATION4))
		return 4;
	return 0;
}

double UWAI::Civ::militaryPower(CvUnitInfo const& u, double baseValue) const {

	double r = baseValue;
	if(r < 0)
		r = u.getPowerValue();
	/* A good start. Power values mostly equal combat strength; the
	   BtS developers have manually increased the value of first strikers
	   (usually +1 power), and considerably decreased the value of units
	   that can only defend.
	   Collateral damage and speed seem underappreciated. */
	if(u.getCollateralDamage() > 0 && u.getDomainType() == DOMAIN_LAND ||
			u.getMoves() > 1)
		r *= 1.12;
	// Sea units that can't bombard cities are overrated in Unit XML
	if(u.getDomainType() == DOMAIN_SEA && u.getBombardRate() == 0)
		r *= 0.66;

	/*  The BtS power value for Tactical Nuke seems low (30, same as Tank),
		but considering that the AI isn't good at using nukes tactically, and that
		the strategic value is captured by the NuclearDeterrent WarUtilityAspect,
		it seems just about right. */
	//if(u.isSuicide()) r *= 1.33; // all missiles

	/*  Combat odds don't increase linearly with strength. Use a power law
		with a power between 1.5 and 2 (configured in XML). */
	r = std::pow(r, GC.getDefineINT(CvGlobals::POWER_CORRECTION) / 100.0);
	return r;
}

bool UWAI::Civ::canHurry() const {

	FOR_EACH_ENUM(Hurry) {
		if(GET_PLAYER(weId).canHurry(eLoopHurry))
			return true;
	}
	return false;
}

double UWAI::Civ::buildUnitProb() const {

	CvPlayerAI& we = GET_PLAYER(weId);
	if(we.isHuman())
		return humanBuildUnitProb();
	return GC.getInfo(we.getPersonalityType()).getBuildUnitProb() / 100.0;
}

int UWAI::Civ::shipSpeed() const {

	MilitaryBranch const* logistics = getCache().getPowerValues()[LOGISTICS];
	if(logistics != NULL) {
		CvUnitInfo const* typicalTransport = logistics->getTypicalUnit();
		if(typicalTransport != NULL)
			return typicalTransport->getMoves();
	}
	// Fallback (needed?)
	return ::range(GET_PLAYER(weId).getCurrentEra() + 1, 3, 5);
}

double UWAI::Civ::humanBuildUnitProb() const {

	CvPlayerAI const& human = GET_PLAYER(weId);
	double r = 0.25; // 30 is about average, Gandhi 15
	if(human.getCurrentEra() == 0)
		r += 0.1;
	if(GC.getGame().isOption(GAMEOPTION_RAGING_BARBARIANS) &&
			human.AI_getCurrEraFactor() <= 2)
		r += 0.05;
	return r;
}

/*  Doesn't currently use any members of this object, but perhaps it will in the
	future. Could e.g. check if 'weId' is able to see the demographics of 'civId'. */
double UWAI::Civ::estimateBuildUpRate(PlayerTypes civId, int period) const {

	CvGame const& g = GC.getGame();
	period *= GC.getInfo(g.getGameSpeedType()).getTrainPercent();
	period /= 100;
	if(g.getElapsedGameTurns() < period + 1)
		return 0;
	int turnNumber = g.getGameTurn();
	CvPlayerAI& civ = GET_PLAYER(civId);
	int pastPow = std::max(1, civ.getHistory(PLAYER_HISTORY_POWER, turnNumber - 1 - period));
	double delta = civ.getHistory(PLAYER_HISTORY_POWER, turnNumber - 1) - pastPow;
	return std::max(0.0, delta / pastPow);
}

double UWAI::Civ::confidenceFromPastWars(TeamTypes targetId) const {

	int sign = 1;
	double sc = cache.pastWarScore(targetId) / 100.0;
	if(sc < 0)
		sign = -1;
	// -15% for the first lost war, less from further wars
	double r = 1 + sign * std::sqrt(std::abs(sc)) * 0.15;
	return ::dRange(r, 0.5, 1.5);
}

double UWAI::Civ::distrustRating() const {

	int result = GC.getInfo(GET_PLAYER(weId).getPersonalityType()).
			getEspionageWeight() - 10;
	if(cache.hasProtectiveTrait())
		result += 30;
	return std::max(0, result) / 100.0;
}

double UWAI::Civ::warConfidencePersonal(bool isNaval, bool isTotal, PlayerTypes vs) const {

	// (param 'vs' currently unused)
	CvPlayerAI const& we = GET_PLAYER(weId);
	/* AI assumes that human confidence depends on difficulty. NB: This doesn't
	   mean that the AI thinks that humans are good at warfare - this is handled
	   by confidenceAgainstHuman. Here, the AI thinks that humans think that
	   humans are good at war, and that humans may therefore attack despite
	   having little power. */
	if(we.isHuman()) {
		// e.g. 0.62 at Settler, 1.59 at Deity
		return (we.trainingModifierFromHandicap() /
				GET_PLAYER(vs).trainingModifierFromHandicap()).getDouble();
	}
	CvLeaderHeadInfo const& lh = GC.getInfo(we.getPersonalityType());
	int const maxWarNearbyPR = lh.getMaxWarNearbyPowerRatio();
	int const maxWarDistPR = lh.getMaxWarDistantPowerRatio();
	int const limWarPR = lh.getLimitedWarPowerRatio();
	double r = // Montezuma: 1.3; Elizabeth: 0.85
	//(maxWarNearbyPR + limWarPR) / 200.0;
	/*  Limited and total mostly affect the military build-up, not how the war is
		conducted. So it may not make much sense for a leader to be e.g. more
		optimistic about limited than total war. But the difference between the
		PowerRatio values should somehow matter. Perhaps some leaders think e.g.
		that they can't use large stacks so effectively ... */
			(isTotal ? maxWarNearbyPR : limWarPR);
			// Or perhaps use a weighted mean as a compromise?
	if(isNaval) {
		/*  distantWar refers to intercontinental war. The values in LeaderHead are
			between 0 (Sitting Bull) and 100 (Isabella), i.e. almost everyone is
			reluctant to fight cross-ocean wars. That reluctance is now covered
			elsewhere (e.g. army power reduced based on cargo capacity in
			simulations); hence the +35. This puts the return value between 0.35 and
			1.35. Exceeding the PR for land war is dangerous though; could cause
			the AI to plan for naval war when ships aren't needed at all. */
		r = std::min(r + 3, maxWarDistPR + 35.0);
	}
	return r / 100.0;
}

double UWAI::Civ::warConfidenceLearned(PlayerTypes targetId, bool ignoreDefOnly) const {

	double fromWarSuccess = GET_TEAM(weId).uwai().confidenceFromWarSuccess(
			TEAMID(targetId));
	double fromPastWars = confidenceFromPastWars(TEAMID(targetId));
	if(ignoreDefOnly == (fromPastWars > 1))
		fromPastWars = 1;
	double r = 1;
	if(fromWarSuccess > 0)
		r += fromWarSuccess - 1;
	/*  Very high/low fromWarSuccess implies relatively high reliability (long war
		and high total war successes); disregard the experience from past wars in
		this case. */
	if(fromPastWars > 0 && r > 0.6 && r < 1.4)
		r += fromPastWars - 1;
	return std::min(r, 1.5);
	// Tbd.: Consider using statistics (e.g. units killed/ lost) in addition
}

double UWAI::Civ::warConfidenceAllies() const {

	CvPlayerAI const& we = GET_PLAYER(weId);
	// AI assumes that humans have normal confidence
	if(we.isHuman())
		return 0.8;
	double dpwr = GC.getInfo(we.getPersonalityType()).getDogpileWarRand();
	if(dpwr <= 0)
		return 0;
	/* dpwr is between 20 (DeGaulle, high confidence) and
	   150 (Lincoln, low confidence). These values are too far apart to convert
	   them proportionally. Hence the square root. The result is normally between
	   1 and 0.23. */
	double r = ::dRange(std::sqrt(30 / dpwr) - 0.22, 0.0, 1.8);
	/*  Should have much greater confidence in civs on our team, but
		can't tell in this function who the ally is. Hard to rewrite
		InvasionGraph such that each ally is evaluated individually; wasn't
		written with team games in mind. As a temporary measure, just generally
		increase confidence when part of a team: */
	if(GET_TEAM(we.getTeam()).getNumMembers() > 1)
		r = std::max(r, ::dRange(2 * r, 0.6, 1.2));
	return r;
}

double UWAI::Civ::confidenceAgainstHuman() const {

	/*  Doesn't seem necessary so far; AI rather too reluctant to attack humans
		due to human diplomacy, and other special treatment of humans; e.g.
		can't get a capitulation from human. */
	return 1;
	/*  Older comment:
		How hesitant should the AI be to engage humans?
		This will have to be set based on experimentation. 90% is moderate
		discouragement against wars vs. humans. Perhaps unneeded, perhaps needs to
		be lower than 90%. Could set it based on the difficulty setting, however,
		while a Settler player can be expected to be easy prey, the AI arguably
		shouldn't exploit this, and while a Deity player will be difficult to
		defeat, the AI should arguably still try.
		The learning-which-civs-are-dangerous approach in warConfidenceLearned
		is more elgant, but wouldn't prevent an AI-on-human dogpile in the
		early game. */
	//return GET_PLAYER(weId).isHuman() ? 1.0 : 0.9;
}

int UWAI::Civ::vengefulness() const {

	CvPlayerAI const& we = GET_PLAYER(weId);
	/*	AI assumes that humans are mostly calculating.
		But player feedback has shown that most humans are at least
		a little bit vengeful, casual players a bit more so.
		On the bottom line, this is relevant mainly for the reparations
		that the AI is willing to pay. */
	if(we.isHuman()) {
		if(GC.getGame().isOption(GAMEOPTION_RISE_FALL)) // Difficulty not so telling in R&F
			return 1;
		int const diffic = GC.getInfo(we.getHandicapType()).getDifficulty();
		if(diffic >= 50)
			return 1;
		if(diffic > 20)
			return 2;
		return 3;
	}
	/*  RefuseToTalkWarThreshold loses its original meaning because UWAI
		doesn't sulk. It fits pretty well for carrying a grudge. Sitting Bull
		has the highest value (12) and De Gaulle the lowest (5).
		BasePeaceWeight (between 0 and 10) now has a dual use; continues to be
		used for inter-AI diplo modifiers.
		The result is between 0 (Gandhi) and 10 (Montezuma). */
	CvLeaderHeadInfo& lhi = GC.getInfo(we.getPersonalityType());
	return std::max(0, lhi.getRefuseToTalkWarThreshold() - lhi.getBasePeaceWeight());
}

double UWAI::Civ::protectiveInstinct() const {

	CvPlayerAI const& we = GET_PLAYER(weId);
	if(we.isHuman()) return 1;
	/*  DogPileWarRand is not a good indicator; more about backstabbing.
		Willingness to sign DP makes some sense. E.g. Roosevelt and the
		Persian leaders do that at Cautious, while Pleased is generally more
		common. Subtract WarMongerRespect to sort out the ones that just like
		DP because they're fearful, e.g. Boudica or de Gaulle. */
	CvLeaderHeadInfo& lh = GC.getInfo(we.getPersonalityType());
	int dpVal = 2 * (ATTITUDE_FRIENDLY - lh.getDefensivePactRefuseAttitudeThreshold());
	int wmrVal = lh.getWarmongerRespect();
	wmrVal *= wmrVal;
	return 0.9 + (dpVal - wmrVal) / 10.0;
}

double UWAI::Civ::diploWeight() const {

	CvPlayerAI const& we = GET_PLAYER(weId);
	if(we.isHuman())
		return 0;
	CvLeaderHeadInfo& lh = GC.getInfo(we.getPersonalityType());
	int ctr = lh.getContactRand(CONTACT_TRADE_TECH);
	if(ctr <= 1)
		return 1.75;
	if(ctr <= 3)
		return 1.5;
	if(ctr <= 7)
		return 1;
	if(ctr <= 15)
		return 0.5;
	return 0.25;
}

double UWAI::Civ::prideRating() const {

	// Fixme: Should be a Team function and call CvTeamAI::AI_makePeaceRand
	CvPlayerAI const& we = GET_PLAYER(weId);
	if(we.isHuman())
		return 0;
	CvLeaderHeadInfo& lh = GC.getInfo(we.getPersonalityType());
	return ::dRange(lh.getMakePeaceRand() / 110.0 - 0.09, 0.0, 1.0);
}
