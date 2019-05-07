// <advc.104> New class; see ArmamentForecast.h for description

#include "CvGameCoreDLL.h"
#include "ArmamentForecast.h"
#include "WarAndPeaceAgent.h"
#include "MilitaryAnalyst.h"
#include "WarEvalParameters.h"
#include "WarAndPeaceReport.h"
#include "CvGamePlay.h"
#include "AI_Defines.h"
#include "CvMap.h"
#include <sstream>

using std::vector;
using std::ostringstream;

ArmamentForecast::ArmamentForecast(PlayerTypes civId, MilitaryAnalyst& m,
			std::vector<MilitaryBranch*>& military, int timeHorizon,
			double productionPortion, WarAndPeaceCache::City const* targetCity,
			bool peaceScenario, bool partyAddedRecently, bool allPartiesKnown,
			bool noUpgrading) :
		m(m), civId(civId), wpai(GET_PLAYER(civId).warAndPeaceAI()),
		report(m.evaluationParameters().getReport()),
		military(military), timeHorizon(timeHorizon) {

	bool const bLogAI = false; // Clogs up the log too much
	if(!bLogAI && !GET_PLAYER(civId).isHuman())
	  report.setMute(true);
	report.log("Armament forecast for *%s*",
			report.leaderName(civId));
	WarEvalParameters& params = m.evaluationParameters();
	/* The current production rate. It's probably going to increase a bit
	   over the planning interval, but not much since the forecast doesn't
	   reach far into the future; ignore that increase. */
	double productionEstimate = GET_PLAYER(civId).estimateYieldRate(YIELD_PRODUCTION);
	/* A very rough estimate of hurry hammers. Would be nicer to base this on
	   the actual hurry effect, e.g., for Slavery, per-use production divided
	   by anger duration. */
	if(wpai.canHurry())
		productionEstimate += 3 * GET_PLAYER(civId).getNumCities();
	/* Civs will often change civics when war is declared. For now, the AI makes
	   no effort to anticipate this. Will have to adapt once it happens. */
	report.log("Production per turn: %d",  ::round(productionEstimate));
	productionEstimate *= productionPortion;
	if(productionPortion < 0.99)
		report.log("Production considering lost cities: %d", 
				::round(productionEstimate));
	// Express upgrades in terms of differences in production costs
	double prodFromUpgrades = 0;
	if(!noUpgrading)
		prodFromUpgrades = productionFromUpgrades();
	if(prodFromUpgrades > 0.01)
		report.log("Production from upgrades: %d", ::round(prodFromUpgrades));

	CvPlayerAI& civ = GET_PLAYER(civId);
	TeamTypes tId = TEAMID(civId);
	CvTeamAI& t = GET_TEAM(tId);
	PlayerTypes weId = m.ourId();
	// Expect caller to make sure of this
	FAssert(targetCity == NULL || targetCity->city() != NULL);
	Intensity intensity = NORMAL;
	if(civ.AI_isDoStrategy(AI_STRATEGY_ALERT1))
		intensity = INCREASED;
	bool navalArmament = false;
	if(targetCity != NULL) {
		if(!targetCity->canReachByLand() ||
				targetCity->getDistance() > getWPAI.maxLandDist())
			navalArmament = true;
		report.log("Target city: %s%s", report.cityName(*targetCity->city()),
				(navalArmament ? " (naval target)": ""));
	}
	/*  Much of this function could be written more concisely based on the
		interface of MilitaryAnalyst; currently relies mostly on WarEvalParamters. */
	TeamTypes targetTeamId = params.targetId();
	// Master teams: Needed in order to disregard ongoing wars when in peaceScenario
	TeamTypes ourMaster = GET_PLAYER(weId).getMasterTeam();
	TeamTypes targetMaster = GET_TEAM(targetTeamId).getMasterTeam();
	TeamTypes master = GET_PLAYER(civId).getMasterTeam();
	int iTotalWars = 0, iWars = 0;
	// Whether simulation assumes peace between civId and any other civ
	bool peaceAssumed = false;
	// Whether simulation assumes civId to have been recently attacked by anyone
	bool attackedRecently = false;
	// Any war or peace assumed that involves civId, or war preparations by civId
	bool fictionalScenario = false;
	TeamTypes singleWarEnemy = NO_TEAM; // Only relevant if there is just one enemy
	bool const noWarVsExtra = peaceScenario && params.isNoWarVsExtra();
	for(size_t i = 0; i < getWPAI.properTeams().size(); i++) {
		TeamTypes loopTeamId = getWPAI.properTeams()[i];
		if(loopTeamId == tId)
			continue;
		CvTeamAI& loopTeam = GET_TEAM(loopTeamId);
		TeamTypes loopMaster = loopTeam.getMasterTeam();
		// Whether the simulation assumes peace between loopTeam and t
		bool peaceAssumedLoop = peaceScenario && ((m.isOnOurSide(tId) &&
				m.isOnTheirSide(loopTeamId)) || (m.isOnOurSide(loopTeamId) &&
				m.isOnTheirSide(tId))) &&
				(tId != weId || !noWarVsExtra || loopTeamId != params.targetId());
		/* Important to check warplan (not just war) when
		   second-guessing preparations underway */
		if(peaceAssumedLoop && t.AI_getWarPlan(loopTeamId) != NO_WARPLAN)
			peaceAssumed = true;
		/* True if simulation assumes a war between loopTeam and t to have
		   already started, or if simulation assumes a war plan.
		   Not true if already at war in the actual game (outside the simulation).*/
		bool warAssumed = (allPartiesKnown || tId == weId) && (
				(m.isOnOurSide(loopTeamId) && m.isOnTheirSide(tId, true)) ||
				(m.isOnOurSide(tId) && m.isOnTheirSide(loopTeamId, true)));
		if(peaceAssumedLoop || loopTeam.isAtWar(tId))
			warAssumed = false;
		bool reachEither = canReachEither(loopTeamId, tId);
		if(reachEither && (warAssumed || peaceAssumed))
			fictionalScenario = true;
		if((loopTeam.isAtWar(tId) || warAssumed) && !peaceAssumedLoop &&
				/* If neither side can reach the other, the war doesn't count
				   because it doesn't (or shouldn't) lead to additional buildup. */
			    reachEither) {
			iWars++;
			if(iWars <= 1)
				singleWarEnemy = loopTeam.getID();
			else singleWarEnemy = NO_TEAM;
			if(intensity == NORMAL)
				intensity = INCREASED;
			// t recently attacked by loopTeam
			bool attackedRecentlyLoop = warAssumed && partyAddedRecently
					&& m.isOnTheirSide(tId, true);
			if(attackedRecentlyLoop && loopTeam.warAndPeaceAI().canReach(tId))
				attackedRecently = true;
			/*  Vassals only get dragged along into limited wars; however,
				an attacked vassal may build up as if in total war. */
			bool totalWarAssumed = warAssumed && ((TEAMID(weId) == tId &&
					loopTeamId == targetTeamId && params.isTotal()) ||
					attackedRecentlyLoop) &&
					// Only assume total war if we can reach them (not just them us)
					t.warAndPeaceAI().canReach(loopTeamId);
			if((t.AI_getWarPlan(loopTeamId) == WARPLAN_TOTAL &&
				    /* If we already have a (total) war plan against the target,
					   that plan is going to be replaced by the war plan
					   under consideration if adopted. */
					(TEAMID(weId) != tId || loopTeamId != targetTeamId)) ||
					totalWarAssumed)
				iTotalWars++;
		}
	}
	int iWarPlans = t.getAnyWarPlanCount(true);
	/*  Assume that we don't pursue any (aggressive) war preparations in the
		peace scenario. (Should only be relevant when considering an immediate DoW,
		e.g. on request of another civ, while already planning war. I think the
		baseline should then be a scenario with no war preparations. Instead,
		WarAndPeaceAI::Team::scheme should compare the utility of the immediate war
		with that of the war in preparation. */
	if(peaceScenario && civId == m.ourId())
		iWarPlans = iWars;
	report.log("War plans: %d; assuming %d wars, %d total wars%s%s",
				iWarPlans, iWars, iTotalWars,
				(peaceAssumed ? ", peace assumed" : ""),
				(attackedRecently ? ", attacked recently" : ""));
	if(iTotalWars > 0 ||
			/* When planning for limited war while being alert2 or dagger,
		       the strategies take precedence. However, shouldn't trust
			   alert2 when assuming peace b/c alert2 may well be caused
			   by the ongoing war.
			   (alert2 now treated separately farther below.)
			   In contrast, ongoing war doesn't have much of an impact on dagger. */
			/* Assume that human is alert about impending attacks, but doesn't
			   adopt a dagger strategy (at least not under
			   the same circumstances as an AI would).
			   [Check obsolete now that Dagger is disabled.] */
			(civ.AI_isDoStrategy(AI_STRATEGY_DAGGER) && !civ.isHuman()) ||
			/* Concurrent war preparations: probably shouldn't be possible anyway.
			   Must definitely be disregarded for weId b/c the war preparations
			   currently under consideration may lead to abandonment of
			   concurrent war preparations. */
			(t.getWarPlanCount(WARPLAN_PREPARING_TOTAL) > 0 &&
			master != ourMaster))
		intensity = FULL;
	bool attackedUnprepared = attackedRecently && iWarPlans == 0;
		    // Count preparing limited as unprepared?
			//nWarPlans <= t.getWarPlanCount(WARPLAN_PREPARING_LIMITED);
	bool defensive = false;
	/* Trust defensive AreaAI even when assuming peace b/c defensive build-up will
	   (or should) continue despite peace. */
	if((getAreaAI(civId) == AREAAI_DEFENSIVE &&
		    /* Trust AreaAI for the first phase of simulation, for the second phase,
			   if increased or full build-up (always the case when
			   defensive AreaAI?), assume that t will be able to get on the
			   offensive, or that defenses reach a saturation point. */
			(intensity == NORMAL || !allPartiesKnown)) ||
			attackedUnprepared || (iWars == 0 && iWarPlans == 0 &&
			(civ.AI_isDoStrategy(AI_STRATEGY_ALERT1) ||
			civ.AI_isDoStrategy(AI_STRATEGY_ALERT2))))
		defensive = true;
	/* alert2 implies full build-up, but if build-up otherwise wouldn't be full,
	   then alert2 makes it defensive. */
	if(civ.AI_isDoStrategy(AI_STRATEGY_ALERT2) && !peaceAssumed &&
			intensity != FULL) {
		intensity = FULL;
		defensive = true;
	}
	/*  Humans build few dedicated defensive units; there may be exceptions to this,
		but the AI can't identify those. */
	if(civ.isHuman())
		defensive = false;
	/* Should assume that our other preparations are abandoned
	   in the peace scenario. In military analysis, the comparison should
	   be war vs. (attempted) peace.
	   Can't trust Area AI when war preparations are supposed to be
	   abandoned. */
	if(m.isOnOurSide(tId) && peaceScenario && iWarPlans > iWars)
		fictionalScenario = true;
	// During war preparations, params take precedence when it comes to naval armament
	if(!peaceScenario && fictionalScenario && civ.getID() == weId &&
			!t.isAtWar(params.targetId()))
		navalArmament = params.isNaval();
	/*  Assume that war against a single small enemy doesn't increase the
		build-up intensity. Otherwise, the AI will tend to leave 1 or 2 cities
		alive. Would be cleaner to assume a shorter time horizon, but that's a
		can of worms. */
	if(singleWarEnemy != NO_TEAM && !navalArmament && iTotalWars <= 0 &&
			iWarPlans <= 1 && !civ.AI_isDoStrategy(AI_STRATEGY_ALERT1
			| AI_STRATEGY_ALERT2) && t.warAndPeaceAI().isPushover(singleWarEnemy)) {
		intensity = NORMAL;
		fictionalScenario = true; // Don't check AreAI either
	}
	// Rely on Area AI (only) if there is no hypothetical war or peace
	if(fictionalScenario)
		predictArmament(timeHorizon, productionEstimate, prodFromUpgrades,
				intensity, defensive, navalArmament);
	else {
		report.log("Checking AreaAI");
		AreaAITypes aai = getAreaAI(civId);
		// Offensive Area AI builds fewer units than 'massing' and 'defensive'
		if(aai == AREAAI_OFFENSIVE || aai == AREAAI_ASSAULT_ASSIST ||
				aai == AREAAI_ASSAULT || ((aai == AREAAI_MASSING ||
				aai == AREAAI_ASSAULT_MASSING || aai == AREAAI_DEFENSIVE) &&
				iTotalWars == 0 && iWars > 0) ||
				civ.AI_isDoStrategy(AI_STRATEGY_ALERT1) ||
				// advc.018: Crush now actually trains fewer units
				civ.AI_isDoStrategy(AI_STRATEGY_CRUSH))
			intensity = INCREASED;
		if(civ.AI_isFocusWar() &&
				(aai == AREAAI_MASSING || aai == AREAAI_ASSAULT_MASSING ||
				(aai == AREAAI_DEFENSIVE && iTotalWars > 0) ||
				civ.AI_isDoStrategy(AI_STRATEGY_ALERT2) ||
				// [Obsolete check; Dagger disabled.]
				(civ.AI_isDoStrategy(AI_STRATEGY_DAGGER) && !civ.isHuman() &&
				/* AI assumes that human players don't use the dagger strat., and
				   are also unable to notice when an AI uses dagger. */
				!GET_PLAYER(m.ourId()).isHuman()) ||
				civ.AI_isDoStrategy(AI_STRATEGY_TURTLE) ||
				civ.AI_isDoStrategy(AI_STRATEGY_LAST_STAND) ||
				civ.AI_isDoStrategy(AI_STRATEGY_FINAL_WAR)))
			intensity = FULL;
		if(civId != weId) { // Trust our own AreaAI more than our power curve
			/* Humans only have a war plan when they're actually at
			   war (total or attacked). Even then, the human build-up doesn't have to
			   match the war plan. Make a projection based on the human power curve. */
			double bur = GET_PLAYER(weId).warAndPeaceAI().estimateBuildUpRate(civId);
			Intensity basedOnBUR = INCREASED;
			if(bur > 0.25)
				basedOnBUR = FULL;
			else if(bur < 0.18)
				basedOnBUR = NORMAL;
			report.log("Build-up intensity based on power curve: %d", (int)basedOnBUR);
			if(civ.isHuman()) {
				intensity = basedOnBUR;
				report.log("Using estimated intensity for forecast");
				if(intensity <= NORMAL && GET_TEAM(civ.getTeam()).
						isAtWar(TEAMID(weId)) && !GET_TEAM(civ.getTeam()).
						warAndPeaceAI().isPushover(TEAMID(weId))) {
					// Have to expect that human will increase build-up as necessary
					intensity = INCREASED;
					report.log("Increased intensity for human at war with us");
				}
			}
			/*  For AI civs, only use the projection as a sanity check for now.
				Would be nice to not look at the Area AI of other civs at all (b/c it's
				a cheat), but I think the estimate may not be reliable enough b/c losses
				can hide build-up in the power curve. Less of a problem against humans
				b/c they don't tend to have high losses. */
			else if((basedOnBUR == FULL && intensity == NORMAL) ||
					(basedOnBUR == NORMAL && intensity == FULL)) {
				report.log("Estimates based on power curve and area differ widely,"
						" assuming increased build-up");
				intensity = INCREASED;
			}
		}
		navalArmament = (aai == AREAAI_ASSAULT ||
				aai == AREAAI_ASSAULT_MASSING || aai == AREAAI_ASSAULT_ASSIST);
		defensive = (civ.AI_isDoStrategy(AI_STRATEGY_ALERT1) ||
				civ.AI_isDoStrategy(AI_STRATEGY_ALERT2) ||
				aai == AREAAI_DEFENSIVE);
		predictArmament(timeHorizon, productionEstimate, prodFromUpgrades,
				intensity, defensive, navalArmament);
	}
	if(!bLogAI && !GET_PLAYER(civId).isHuman())
		report.setMute(false); // advc.test
}

double ArmamentForecast::getProductionInvested() const {

	return productionInvested;
}

void ArmamentForecast::predictArmament(int turnsBuildUp, double perTurnProduction,
		double additionalProduction, Intensity intensity, bool defensive,
		bool navalArmament) {

	PROFILE_FUNC();
	CvPlayerAI const& civ = GET_PLAYER(civId);
	if(!defensive) {
		/*  Space and culture victory tend to overrule military build-up (even when
			military victory is pursued in parallel), and divert a lot of production
			into cultural things or spaceship parts. */
		bool peacefulVictory = civ.AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) ||
				civ.AI_isDoVictoryStrategy(AI_VICTORY_SPACE3) ||
				civ.AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4) ||
				civ.AI_isDoVictoryStrategy(AI_VICTORY_SPACE4);
		if(peacefulVictory) {
			report.log("Build-up reduced b/c pursuing peaceful victory");
			if(intensity == FULL)
				intensity = INCREASED;
			else if(intensity == INCREASED)
				intensity = NORMAL;
			else if(intensity == NORMAL)
				intensity = DECREASED;
		}
	}

	report.log("Forecast for the next %d turns", turnsBuildUp);
	report.log("Defensive: %s, naval: %s, intensity: %s", defensive ? "yes" : "no",
			navalArmament ? "yes" : "no", strIntensity(intensity));

	// Armament portion of the total production; based on intensity
	double armamentPortion = wpai.buildUnitProb();
	/*  Total vs. limited war mostly affects pre-war build-up, but there are also
		a few lines of (mostly K-Mod) code that make the AI focus more on production
		when in a total war. The computation of the intensity doesn't cover this
		behavior.
		Doesn't matter much if I increase perTurnProduction or armamentPortion;
		they're multiplied in the end. */
	double atWarAdjustment = 0;
	WarEvalParameters const& params = m.evaluationParameters();
	if(params.isConsideringPeace()) {
		if(params.isTotal()) {
			atWarAdjustment = 0.05;
			/*  If considering to switch from limited to total war, assume that
				total war will bring full build-up. Switching to total war may
				not actually have this effect on the AreaAI, but UWAI assumes
				that preparations for total war always result in full build-up.
				Starting war preparations against some additional opponent mustn't,
				as a side-effect, lead to a better war outcome against the current
				opponent. */
			if(GET_TEAM(params.agentId()).AI_getWarPlan(params.targetId()) !=
					WARPLAN_TOTAL)
				intensity = FULL;
		}
		else atWarAdjustment -= 0.05;
	}
	if(intensity == DECREASED)
		armamentPortion -= 0.1;
	else if(intensity == INCREASED)
		armamentPortion += 0.25;
	else if(intensity == FULL)
		armamentPortion = std::max(0.7,
			armamentPortion); // Only relevant if a mod sets a buildUnitProb >= 50%
	armamentPortion += atWarAdjustment;
	if(GET_PLAYER(civId).getMaxConscript() > 0 && intensity >= INCREASED) {
		if(intensity == INCREASED)
			armamentPortion += 0.05;
		else if(intensity == FULL)
			armamentPortion += 0.1;
		report.log("Armament portion increased b/c of conscription");
	}
	armamentPortion = ::dRange(armamentPortion, 0.0, 0.8);
	// Portions of the military branches
	double branchPortions[NUM_BRANCHES] = {0.0};
	// Little defense by default
	branchPortions[HOME_GUARD] = 0.13;
	if(navalArmament) {
		branchPortions[FLEET] = 0.2;
		int rev = 0, revCoast = 0, dummy = 0;
		for(CvCity* c = civ.firstCity(&dummy); c != NULL; c = civ.nextCity(&dummy)) {
			if(c->isRevealed(civ.getTeam(), false)) {
				rev++;
				if(c->isCoastal())
					revCoast++;
			}
		}
		if(rev > 0) {
			if(rev >= civ.getNumCities() && revCoast <= 0)
				branchPortions[FLEET] = 0;
			// Assume very little defensive build-up when attacking across the sea
			else if(!defensive)
				branchPortions[HOME_GUARD] = 0.05;
			double coastRatio = revCoast / (double)rev;
			branchPortions[FLEET] = std::min(0.08 + coastRatio / 2.8, 0.35);
		}
		double typicalCargo = military[LOGISTICS]->getTypicalUnitPower(m.ourId());
		if(typicalCargo > 0.1 && military[LOGISTICS]->getTypicalUnit() != NULL) {
			branchPortions[LOGISTICS] = std::min(branchPortions[FLEET],
			/*  As the game progresses, the production portion needed for cargo
				units decreases. Factoring in the typical cargo capacity also
				makes the code robust against XML changes to cargo capacities. */
					1.0 / typicalCargo);
			if(!defensive && intensity > NORMAL && civ.getCurrentEra() > 0)  {
				/*  Need to assume more naval build-up if 'civ' hardly has any
					navy; otherwise, the AI may assume that a naval assault is
					hopeless. */
				double mult = (::dRange(2 - military[LOGISTICS]->power() /
						(typicalCargo * civ.getCurrentEra()), 1, 1.5) + 1) / 2;
				branchPortions[LOGISTICS] *= mult;
				double typicalFleetPow = military[FLEET]->getTypicalUnitPower(m.ourId());
				if(typicalFleetPow > 0.1) {
					mult = (::dRange(2 - military[FLEET]->power() /
							(typicalFleetPow * civ.getCurrentEra()), 1, 1.5) + 1) / 2;
					branchPortions[FLEET] *= mult;
				}
			}
		}
	}
	branchPortions[ARMY] = 1 - branchPortions[HOME_GUARD] - branchPortions[FLEET] -
			branchPortions[LOGISTICS];
	FAssert(branchPortions[ARMY] >= 0);
	if(defensive || intensity == NORMAL) {
		// No cargo in defensive naval wars
		branchPortions[FLEET] += branchPortions[LOGISTICS];
		branchPortions[LOGISTICS] = 0;
		// Shift half of the fleet budget to home guard
		branchPortions[FLEET] /= 2;
		branchPortions[HOME_GUARD] += branchPortions[FLEET];
		if(!GET_PLAYER(civId).isHuman() && defensive) {
			// Shift half of the army budget to home guard
			branchPortions[ARMY] /= 2;
			branchPortions[HOME_GUARD] += branchPortions[ARMY];
		}
		else { // Shift 1/3 for humans -- they build fewer garrisons
			branchPortions[ARMY] *= (2.0/ 3);
			branchPortions[HOME_GUARD] += (0.5 * branchPortions[ARMY]);
		}
	}
	if(!defensive) {
		// Undone below if unable to build nukes
		double nukePortion = 0.2;
		if(civ.isHuman())
			nukePortion += 0.05;
		branchPortions[NUCLEAR] = nukePortion;
		for(int i = 0; i < NUM_BRANCHES; i++) {
			if(i == NUCLEAR) continue;
			branchPortions[i] *= (1 - nukePortion);
		}
	}
	// Shift weights away from milit. branches the civ can't build units for.
	double surplus = 0;
	for(int i = 0; i < NUM_BRANCHES; i++) {
		if(military[i]->getTypicalUnit() == NULL) {
			surplus += branchPortions[i];
			branchPortions[i] = 0;
		}
	}
	if(surplus > 0.999) {
		report.log("Armament forecast canceled b/c no units can be trained");
		return;
	}
	double checksum = 0;
	for(int i = 0; i < NUM_BRANCHES; i++) {
		if(military[i]->getTypicalUnit() != NULL)
			branchPortions[i] += surplus * branchPortions[i] / (1 - surplus);
		checksum += branchPortions[i];
	}
	FAssert(checksum > 0.99 && checksum < 1.01);
	/* Assume no deliberate build-up of Cavalry. Army can happen to be mounted
	   though. */
	CvUnitInfo const* typicalArmyUnit = military[ARMY]->getTypicalUnit();
	if(typicalArmyUnit != NULL && military[CAVALRY]->canEmploy(*typicalArmyUnit))
		branchPortions[CAVALRY] = branchPortions[ARMY];
	if(!report.isMute()) {// Logging
		ostringstream msg;
		msg << "Branch portions for ";
		bool firstItemDone = false; // for commas
		for(int i = 0; i < NUM_BRANCHES; i++) {
			if(i == NUCLEAR || branchPortions[i] < 0.01)
				continue;
			MilitaryBranch& mb = *military[i];
			if(firstItemDone)
				msg << ", ";
			msg << mb;
			firstItemDone = true;
		}
		msg << ": ";
		firstItemDone = false;
		for(int i = 0; i < NUM_BRANCHES; i++) {
			if(i == NUCLEAR || branchPortions[i] < 0.01)
				continue;
			if(firstItemDone)
				msg << ", ";
			msg << ::round(100 * branchPortions[i]);
			firstItemDone = true;
		}
		report.log("%s", msg.str().c_str());
	}
	// Compute total production for armament
	double totalProductionForBuildUp = additionalProduction;
	totalProductionForBuildUp += turnsBuildUp * armamentPortion * perTurnProduction;
	report.log("Total production for build-up: %d hammers", 
			::round(totalProductionForBuildUp));
	productionInvested = totalProductionForBuildUp;

	// Increase military power
	//report.log("\nbq."); // Textile block quote (takes up too much space)
	for(int i = 0; i < NUM_BRANCHES; i++) {
		MilitaryBranch& mb = *military[i];
		int typicalProd = mb.getTypicalUnitCost(m.ourId());
		if(typicalProd <= 0)
			continue;
		double pow = mb.getTypicalUnitPower(m.ourId());
		double incr = branchPortions[i] * totalProductionForBuildUp * pow /
				typicalProd;
		mb.changePower(incr);
		int iincr = ::round(incr);
		if(iincr > 0)
			report.log("Predicted power increase in %s by %d",
					mb.str(), iincr);
	}
	report.log("");
}

bool ArmamentForecast::canReachEither(TeamTypes t1, TeamTypes t2) const {

	return GET_TEAM(t1).warAndPeaceAI().canReach(t2) ||
				GET_TEAM(t2).warAndPeaceAI().canReach(t1);
}

char const* ArmamentForecast::strIntensity(Intensity in) {

	switch(in) {
	case DECREASED: return "decreased";
	case NORMAL: return "normal";
	case INCREASED: return "increased";
	case FULL: return "full";
	default: return "(unknown intensity)";
	}
}

double ArmamentForecast::productionFromUpgrades() {

	/* Going through all the units and checking for possible upgrades would be
	   somewhat expensive and complicated: Which upgrades would be prioritized
	   if upgrading them all is too costly?
	   Failed attempt: Predict the power of a fully upgraded army based on
	   unit counts and typical units -- the resulting power value turned out to
	   be a poor estimate of an army's upgrade potential.
	   Actual approach: Rely on CvPlayerAI::AI_updateGoldToUpgradeAllUnits,
	   convert the result into hammers, and (later) the hammers into power. */
	CvPlayerAI& civ = GET_PLAYER(civId);
	double r = civ.AI_getGoldToUpgradeAllUnits();
	if(r > 0.01)
		report.log("Total gold needed for upgrades: %d", ::round(r));
	/* 'civ' may not have the funds to make all the upgrades in the
	   medium term. Think of a human player keeping stacks of Warriors around,
	   or a vassal receiving tech quickly from its master.
	   Spend at most incomeTurns turns worth of income on upgrades. The subtrahend
	   will be 0 during anarchy -- not a big problem I think. */
	double const incomeTurns = 4;
	double income = civ.estimateYieldRate(YIELD_COMMERCE, 3) -
			civ.calculateInflatedCosts();
	double incomeBound = incomeTurns * income;
	// Also take into account current gold stockpile
	int iGold = civ.getGold();
	incomeBound = std::max((2 * incomeBound + iGold) / 3, (2 * iGold + incomeBound) / 3);
	if(incomeBound < r)
		report.log("Upgrades bounded by income (%d gpt)", ::round(income));
	r = std::min(incomeBound, r);
	// An approximate inversion of CvUnit::upgradePrice
	double upgrCostPerProd = GC.getDefineINT("UNIT_UPGRADE_COST_PER_PRODUCTION");
	r /= upgrCostPerProd;
	/* The base upgrade cost is paid per unit, but CvPlayerAI doesn't track
	   how many units need an upgrade. Assuming a mean training cost difference
	   of 30 hammers, one can look at the base cost as a multiplicative modifier. */
	double typicalGoldForProdDiff = upgrCostPerProd * 30;
	double baseCostModifier = typicalGoldForProdDiff / (typicalGoldForProdDiff +
			GC.getDefineINT("BASE_UNIT_UPGRADE_COST"));
	r *= baseCostModifier;
	if(!civ.isHuman()) {
		CvHandicapInfo& gameHandicap = GC.getHandicapInfo(GC.getGameINLINE().
				getHandicapType());
		double aiUpgradeFactor = gameHandicap.getAIUnitUpgradePercent();
		// advc.250d: The per-era modifier no longer applies to upgrade cost
			// + gameHandicap.getAIPerEraModifier() * civ.getCurrentEra();
		aiUpgradeFactor /= 100.0;
		/* Shouldn't draw conclusions from AI_getGoldToUpgradeAllUnits when
		   AI upgrades are (modded to be) free or almost free. */
		if(aiUpgradeFactor > 0.1)
			r /= aiUpgradeFactor;
	}
	return std::max(0.0, r);
}

CvArea* ArmamentForecast::getCapitalArea(PlayerTypes civId) const {

	if(civId == NO_PLAYER)
		civId = m.ourId();
	CvCity* capital = GET_PLAYER(civId).getCapitalCity();
	return GC.getMap().getArea(capital->plot()->getArea());
}

AreaAITypes ArmamentForecast::getAreaAI(PlayerTypes civId) const {

	if(civId == NO_PLAYER)
		civId = m.ourId();
	CvCity* capital = GET_PLAYER(civId).getCapitalCity();
	if(capital == NULL)
		return NO_AREAAI;
	CvArea& capitalArea = *getCapitalArea(civId);
	return capitalArea.getAreaAIType(TEAMID(civId));
}

// </advc.104>
