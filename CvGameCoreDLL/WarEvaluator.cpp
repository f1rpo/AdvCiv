// advc.104: New class; see WarEvaluator.h for description.

#include "CvGameCoreDLL.h"
#include "WarEvaluator.h"
#include "UWAIAgent.h"
#include "WarUtilityAspect.h"
#include "MilitaryAnalyst.h"
#include "UWAIReport.h"
#include "WarEvalParameters.h"
#include "CoreAI.h"
#include "CvInfo_GameOption.h"

using std::vector;
using std::string;
using std::ostringstream;

/*  When a player trades with the AI, the EXE asks the DLL to compute trade values
	several times in a row (usually about a dozen times), which is enough to cause
	a noticeable delay. Therefore this primitive caching mechanism. The cache is
	always used on the Trade screen; in other (async) UI contexts, it has to be enabled
	explicitly through the useCache param of the constructor, or the static enableCache.
	As for the latter, it's important to call disableCache before returning to a
	synchronized context b/c the cache doesn't work in all situations
	(see WarEvalParameters::id) and isn't stored in savegames. */
bool WarEvaluator::checkCache = false;
bool WarEvaluator::cacheCleared = true;
/*  Cache size: Calls alternate between naval/ non-naval,
	limited/ total or mutual war utility of two civs,
	so caching just the last result isn't effective. */
#define cacheSz 16
static int lastCallParams[cacheSz];
static int lastCallResult[cacheSz];
static int lastIndex;

void WarEvaluator::enableCache() {

	checkCache = true;
	cacheCleared = false;
}

void WarEvaluator::disableCache() {

	checkCache = false;
}

void WarEvaluator::clearCache() {

	if(cacheCleared) // Just to make sure that repeated clears don't waste time
		return;
	cacheCleared = true;
	for(int i = 0; i < cacheSz; i++)
		lastCallResult[i] = MIN_INT;
}

WarEvaluator::WarEvaluator(WarEvalParameters& warEvalParams, bool useCache) :

	params(warEvalParams), report(params.getReport()),
	agentId(params.agentId()), targetId(params.targetId()),
	agent(GET_TEAM(agentId)), target(GET_TEAM(targetId)),
	useCache(useCache) {

	static bool bInitCache = true;
	if(bInitCache) {
		for(int i = 0; i < cacheSz; i++) {
			lastCallParams[i] = 0;
			lastCallResult[i] = MIN_INT;
		}
		lastIndex = 0;
		bInitCache = false;
	}

	FAssert(agentId != targetId);
	FAssert(!target.isAVassal());
	FAssert(agent.isHasMet(targetId));

	peaceScenario = false;
}

void WarEvaluator::reportPreamble() {

	if(report.isMute())
		return;
	/* Show members in one column per team. Use spaces for alignment, table
	   markers ('|') for Textile. */
	report.log("Evaluating *%s%s war* between %s%s and %s%s", params.isTotal() ?
			report.warPlanName(WARPLAN_TOTAL) : report.warPlanName(WARPLAN_LIMITED),
			params.isNaval() ? " naval" : "",
			report.teamName(agentId), agent.isHuman() ? " (human)" : "",
			report.teamName(targetId), target.isHuman() ? " (human)" : "");
	report.log("");
	for(MemberIter agentIt(agentId), targetIt(targetId); agentIt.hasNext() || targetIt.hasNext();
			++agentIt, ++targetIt) {
		ostringstream os;
		os << "| " << (agentIt.hasNext()
				? report.leaderName(agentIt->getID(), 16)
				: "");
		string msg = os.str().substr(0, 17);
		msg += " |";
		while(msg.length() < 20)
			msg += " ";
		if(targetIt.hasNext())
			msg += report.leaderName(targetIt->getID(), 16);
		msg += "|\n";
		report.log(msg.c_str());
	}
	report.log("Current actual war plan: %s",
			report.warPlanName(agent.AI_getWarPlan(targetId)));
	if(params.isConsideringPeace())
		report.log("(considering peace)");
	report.log("Preparation time vs. target: %d", params.getPreparationTime());
	if(params.isImmediateDoW())
		report.log("Immediate DoW assumed");
	if(agent.isAVassal())
		report.log("Agent is a vassal of %s",
				report.masterName(agent.getMasterTeam()));
	if(target.isAVassal())
		report.log("Target is a vassal of %s",
				report.masterName(target.getMasterTeam()));
	for(int i = 0; i < MAX_CIV_TEAMS; i++) {
		TeamTypes allyId = (TeamTypes)i;
		if(params.isWarAlly(allyId))
			report.log("Joint DoW by %s assumed", report.teamName(allyId));
	}
	for(int i = 0; i < MAX_CIV_TEAMS; i++) {
		TeamTypes extraTargetId = (TeamTypes)i;
		if(params.isExtraTarget(extraTargetId))
			report.log("Extra target: %s", report.teamName(extraTargetId));
	}
	if(params.getSponsor() != NO_PLAYER) {
		report.log("Sponsored by %s", report.leaderName(params.getSponsor()));
		FAssert(params.isImmediateDoW());
	}
	if(params.isIgnoreDistraction())
		report.log("Computation ignoring Distraction cost");
}

int WarEvaluator::defaultPreparationTime(WarPlanTypes wp) {

	int age = 0;
	if(wp == NO_WARPLAN)
		wp = agent.AI_getWarPlan(targetId);
	else age = agent.AI_getWarPlanStateCounter(targetId);
	if(wp == WARPLAN_LIMITED || wp == WARPLAN_TOTAL) // Agent is past preparations
		return 0;
	int baseTime = -1;
	if(params.isTotal()) {
		if(params.isNaval())
			baseTime = UWAI::preparationTimeTotalNaval;
		else baseTime = UWAI::preparationTimeTotal;
	}
	else {
		if(params.isNaval())
			baseTime = UWAI::preparationTimeLimitedNaval;
		else baseTime = UWAI::preparationTimeLimited;
	}
	int r = std::max(baseTime - age, 0);
	r *= GC.getInfo(GC.getGame().getGameSpeedType()).getTrainPercent();
	r = ::round(r / 100.0);
	return r;
}

int WarEvaluator::evaluate(WarPlanTypes wp, int preparationTime) {

	if(params.isImmediateDoW())
		preparationTime = 0;
	// Plan for war scenario
	if(wp == NO_WARPLAN)
		wp = agent.AI_getWarPlan(params.targetId());
	if(wp == NO_WARPLAN) {
		if(preparationTime == 0)
			wp = WARPLAN_LIMITED;
		else wp = WARPLAN_PREPARING_LIMITED;
	}
	bool isTotal = (wp == WARPLAN_TOTAL || wp == WARPLAN_PREPARING_TOTAL);
	if(agent.isAtWar(targetId)) {
		/*  If already at war, MilitaryAnalyst determines whether it should be naval
			(Doesn't currently write this into params though - should it?) */
		bool isNaval = !agent.AI_isLandTarget(targetId); // Might as well not set this?
		int r = evaluate(wp, isNaval, 0);
		params.setNaval(isNaval);
		params.setTotal(isTotal);
		params.setPreparationTime(0);
		return r;
	}
	/*  Just for performance: Don't compute naval war utility if we have
		no cargo ships. Do compute utility of limited naval war. May not give
		us enough time to build cargo ships; then war utility will be low. But
		could also already have cargo ships e.g. from earlier wars.
		(BtS/K-Mod only considers total naval war.) */
	bool skipNaval = true;
	for(MemberIter it(agentId); it.hasNext(); ++it) {
		if(it->AI_totalUnitAIs(UNITAI_ASSAULT_SEA) + it->AI_totalUnitAIs(UNITAI_SETTLER_SEA) > 0)
			skipNaval = false;
	}
	/*  If the report isn't mute anyway, and we're doing two runs, rather than
		flooding the report with logs for both naval and non-naval utility, mute
		the report in both runs, and do an additional run just for logging
		once we know if naval or non-naval war is better. */
	bool extraRun = (!report.isMute() && !skipNaval);
	if(extraRun)
		report.setMute(true);
	int nonNaval = evaluate(wp, false, preparationTime);
	// Assume non-naval war if it hardly makes a difference
	int const antiNavalBias = 3;
	int naval = MIN_INT;
	if(!skipNaval)
		naval = evaluate(wp, true, preparationTime);
	int r = MIN_INT;
	if(extraRun) {
		report.setMute(false);
		r = evaluate(wp, naval > nonNaval + antiNavalBias, preparationTime);
	}
	else {
		if(naval > nonNaval + antiNavalBias)
			r = naval;
		else r = nonNaval;
	}
	/*  Calls to evaluate(3) change some members of params that the caller
		may read */
	params.setNaval(naval > nonNaval + antiNavalBias);
	params.setTotal(isTotal);
	params.setPreparationTime(defaultPreparationTime(wp));
	return r;
}

int WarEvaluator::evaluate(WarPlanTypes wp, bool isNaval, int preparationTime) {

	PROFILE_FUNC(); // All war evaluation goes through here
	peaceScenario = (wp == NO_WARPLAN); // Should only happen in recursive call
	int u = 0;
	params.setNaval(isNaval);
	/*  The original cause of war (dogpile, attacked etc.) has no bearing
		on war utility. */
	params.setTotal(wp == WARPLAN_TOTAL || wp == WARPLAN_PREPARING_TOTAL);
	if(preparationTime < 0) {
		FAssert(!peaceScenario); // Needs to be set in the war scenario call
		preparationTime = defaultPreparationTime(wp);
	}
	params.setPreparationTime(preparationTime);
	// Don't check cache in recursive calls (peaceScenario=true)
	if(!peaceScenario && (checkCache || useCache || gDLL->isDiplomacy())) {
		int id = params.id();
		for(int i = 0; i < cacheSz; i++)
			if(id == lastCallParams[i] && lastCallResult[i] != MIN_INT)
				return lastCallResult[i];
	}
	if(peaceScenario)
		report.log("*Peace scenario*\n");
	else {
		/*  Normally, both are evaluated, and war goes first. Logging the preamble
			once is enough. */
		reportPreamble();
		report.log("*War scenario*\n");
	}
	vector<WarUtilityAspect*> aspects;
	fillWithAspects(aspects);
	for(MemberIter it(agentId); it.hasNext(); ++it)
		evaluate(it->getID(), aspects);
	for(size_t i = 0; i < aspects.size(); i++) {
		int delta = aspects[i]->utility();
		u += delta;
		if(delta != 0)
			report.log("%s total: %d", aspects[i]->aspectName(), delta);
		delete aspects[i];
	}
	report.log("Bottom line: %d\n", u);
	if(!peaceScenario) {
		u -= evaluate(NO_WARPLAN, false, preparationTime);
		report.log("Utility war minus peace: %d\n", u);
		// Restore params (changed by recursive call)
		params.setNaval(isNaval);
		params.setTotal(wp == WARPLAN_TOTAL || wp == WARPLAN_PREPARING_TOTAL);
		params.setPreparationTime(preparationTime);
		/*  Could update cache even when !checkCache, but the cache is so small
			that this can push out just the values that are needed. */
		if(checkCache || useCache) {
			// Cache the total result after returning from the recursive call
			long id = params.id();
			lastCallParams[lastIndex] = id;
			lastCallResult[lastIndex] = u;
			lastIndex = (lastIndex + 1) % cacheSz;
		}
	}
	return u;
}

void WarEvaluator::fillWithAspects(vector<WarUtilityAspect*>& v) {

	v.push_back(new GreedForAssets(params));
	v.push_back(new GreedForVassals(params));
	v.push_back(new GreedForSpace(params));
	v.push_back(new GreedForCash(params));
	v.push_back(new Loathing(params));
	v.push_back(new MilitaryVictory(params));
	v.push_back(new Assistance(params));
	v.push_back(new Reconquista(params));
	v.push_back(new Rebuke(params));
	v.push_back(new Fidelity(params));
	v.push_back(new HiredHand(params));
	v.push_back(new BorderDisputes(params));
	v.push_back(new SuckingUp(params));
	v.push_back(new PreEmptiveWar(params));
	v.push_back(new KingMaking(params));
	v.push_back(new Effort(params));
	v.push_back(new Risk(params));
	v.push_back(new IllWill(params));
	v.push_back(new Affection(params));
	if(!params.isIgnoreDistraction())
		v.push_back(new Distraction(params));
	v.push_back(new PublicOpposition(params));
	v.push_back(new Revolts(params));
	v.push_back(new UlteriorMotives(params));
	v.push_back(new TacticalSituation(params));
	v.push_back(new Bellicosity(params));
	v.push_back(new FairPlay(params));
	v.push_back(new LoveOfPeace(params));
	FAssert(UWAI::NUM_ASPECTS - (params.isIgnoreDistraction() ? 1 : 0) == (int)v.size());
}

void WarEvaluator::evaluate(PlayerTypes weId, vector<WarUtilityAspect*>& aspects) {

	MilitaryAnalyst m(weId, params, peaceScenario);
	for(PlayerIter<MAJOR_CIV,KNOWN_TO> it(agentId); it.hasNext(); ++it) {
		PlayerTypes civId = it->getID();
		if(!GET_TEAM(civId).isCapitulated())
			m.logResults(civId);
	}
	report.log("\nh4.\nComputing utility of %s\n", report.leaderName(weId, 16));
	int ourUtility = 0;
	for(size_t i = 0; i < aspects.size(); i++)
		ourUtility += aspects[i]->evaluate(m);
	report.log("--\nTotal utility for %s: %d", report.leaderName(weId, 16), ourUtility);
}
