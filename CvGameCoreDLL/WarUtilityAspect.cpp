// <advc.104> New class hierarchy; see WarUtilityAspect.h

#include "CvGameCoreDLL.h"
#include "WarUtilityAspect.h"
#include "CvStatistics.h"

using std::vector;
using std::string;
using std::set;
using std::map;
using std::pair;
typedef set<int>::const_iterator iSetIt;
typedef map<int,int>::const_iterator iMapIt;
typedef WarAndPeaceCache::City City;

WarUtilityAspect::WarUtilityAspect(WarEvalParameters& params) :

	params(params), agentId(params.agentId()), agent(GET_TEAM(agentId)),
	agentAI(agent.warAndPeaceAI()), agentTeam(agentAI.teamMembers()),
	properTeams(getWPAI.properTeams()), properCivs(getWPAI.properCivs()),
	report(params.getReport()) {

	u = 0;
	// In case a sub-class tries to access these before 'evaluate' is called
	reset();
	numRivals = numKnownRivals = 0;
	for(size_t i = 0; i < properCivs.size(); i++) {
		PlayerTypes civId = properCivs[i];
		if(TEAMID(civId) != agentId && !TEAMREF(civId).isAVassal()) {
			numRivals++;
			if(agent.isHasMet(TEAMID(civId)))
				numKnownRivals++;
		}
	}
	FAssert(numRivals > 0);
}

void WarUtilityAspect::reset() {

	m = NULL; weId = NO_PLAYER; we = NULL; weAI = NULL; ourCache = NULL;
	resetCivOnCiv();
}

void WarUtilityAspect::resetCivOnCiv() {

	theyId = NO_PLAYER; they = NULL; theyAI = NULL; weConquerFromThem.clear();
	towardsThem = towardsUs = NO_ATTITUDE;
	valTowardsThem = valTowardsUs = 0;
}

int WarUtilityAspect::evaluate(MilitaryAnalyst& m) {

	PROFILE_FUNC();
	this->m = &m;
	weId = m.ourId();
	we = &(GET_PLAYER(weId));
	weAI = &we->warAndPeaceAI();
	ourCache = &weAI->getCache();
	int overall = preEvaluate();
	for(size_t j = 0; j < properCivs.size(); j++) {
		PlayerTypes theyId = properCivs[j];
		if(GET_PLAYER(theyId).getMasterTeam() == agent.getMasterTeam() ||
				!agent.isHasMet(TEAMID(theyId)) || (concernsOnlyWarParties() &&
				!m.isPartOfAnalysis(theyId)))
			continue;
		int delta = evaluate(theyId);
		if(delta != 0)
			report.log("*%s from %s: %d*", aspectName(),
					report.leaderName(theyId, 16), delta);
	}
	if(overall != 0) {
		report.log("*%s (from no one in particular): %d*", aspectName(), overall);
		u += overall;
	}
	double xmlAdjust = getWPAI.aspectWeight(xmlId());
	if(u != 0 && (xmlAdjust < 0.99 || xmlAdjust > 1.01)) {
		report.log("Adjustment from XML: %d percent", ::round(xmlAdjust * 100));
		u = ::round(u * xmlAdjust);
	}
	reset();
	return utility();
}

int WarUtilityAspect::evaluate(PlayerTypes theyId) {

	this->theyId = theyId;
	they = &(GET_PLAYER(theyId));
	theyAI = &they->warAndPeaceAI();
	valTowardsThem = we->AI_getAttitudeVal(theyId);
	valTowardsUs = they->AI_getAttitudeVal(weId);
	/*  In the very early game, war preparations take a long time, and diplo
		will often improve during that time b/c of "years of peace". If this
		brings attitude to Pleased, the war is usually off, and the preparations
		(very costly this early) are for nothing (unless there's an alt. target;
		not considered here). Need to anticipate the change in attitude. */
	/*  No worries once preparations are well underway or when asked to
		declare war immediately (or when already at war) */
	if(params.getPreparationTime() > 5 && GC.getGameINLINE().
			gameTurnProgress() < 0.18 &&
			we->AI_getPeaceAttitude(theyId) == 0 &&
			agent.AI_getAtPeaceCounter(TEAMID(theyId)) >
			GC.getLeaderHeadInfo(we->getPersonalityType()).
			getAtPeaceAttitudeDivisor() / 2)
		valTowardsThem++;
	towardsThem = we->AI_getAttitudeFromValue(valTowardsThem);
	/*  advc.130o: If recently received tribute, we're pleased as far
		as war plans go. */
	if(!params.isConsideringPeace() &&
			we->AI_getMemoryCount(theyId, MEMORY_ACCEPT_DEMAND) > 0 &&
			they->AI_getMemoryCount(weId, MEMORY_MADE_DEMAND_RECENT) > 0)
		towardsThem = std::max(towardsThem, ATTITUDE_PLEASED);
	towardsUs = they->AI_getAttitudeFromValue(valTowardsUs);
	for(iSetIt it = m->conqueredCities(weId).begin();
			it != m->conqueredCities(weId).end();
			it++)
		if(m->lostCities(theyId).count(*it) > 0)
			weConquerFromThem.push_back(*it);
	int utilityBefore = u;
	evaluate();
	resetCivOnCiv();
	return u - utilityBefore;
}

int WarUtilityAspect::preEvaluate() { return 0; }

bool WarUtilityAspect::concernsOnlyWarParties() const {

	return true;
}

void WarUtilityAspect::log(char const* fmt, ...) {

	va_list args;
	va_start(args, fmt);
	string msg = CvString::formatv(fmt, args);
	va_end(args);
	report.log(msg.c_str());
}

int WarUtilityAspect::utility() const {

	return u;
}

double WarUtilityAspect::lostAssetScore(PlayerTypes to, double* returnTotal,
		TeamTypes ignoreGains) {

	double r = 0;
	double total = 0;
	for(int i = 0; i < theyAI->getCache().size(); i++) {
		City* cp = theyAI->getCache().getCity(i);
		if(cp == NULL) continue; City const& c = *cp;
		if(!agent.AI_deduceCitySite(c.city()))
			continue;
		/*  Their cached value accounts for GP, but we're not supposed to see those.
			(Their cache contains some other info we shouldn't see, but nothing
			crucial.) */
		double sc = c.getAssetScore() - 4 * c.city()->getNumGreatPeople();
		if(m->lostCities(theyId).count(c.id()) > 0 &&
				(to == NO_PLAYER || m->conqueredCities(to).count(c.id()) > 0)) {
			r += sc;
			// Losing the capital isn't so bad b/c the Palace respawns
			if(c.city()->isCapital())
				r += 4;
		}
		else if(m->conqueredCities(theyId).count(c.id()) > 0 &&
				(to == NO_PLAYER || m->lostCities(to).count(c.id()) > 0) &&
				c.city()->getTeam() != ignoreGains) {
			/*  Their cache doesn't account for GP settled in cities of a third
				party (unless espionage visibility), so we don't subtract
				NumGreatPeople.
				I would rather use a score based on our visibility, however, the
				AssetScore from our cache would compute distance maintenance,
				tile culture and trade routes as if we were conquering the city,
				which is far worse.
				The clean solution - computing asset scores for all pairs of
				(oldOwner, newOwner) seems excessive. */
			r -= c.getAssetScore();
		}
		else if(c.city()->isCapital())
			sc += 8;
		// National wonders other than Palace are invisible to us
		total += sc;
	}
	r += lossesFromNukes(theyId, to);
	total = std::max(r, total);
	double blockadeMultiplier = lossesFromBlockade(theyId, to);
	double fromBlockade = 0.1 * blockadeMultiplier * (total - r);
	r += fromBlockade;
	r += lossesFromFlippedTiles(theyId, to); // advc.035
	if(returnTotal != NULL)
		*returnTotal = total;
	return r;
}

double WarUtilityAspect::lossesFromBlockade(PlayerTypes victimId, PlayerTypes to) {

	// Blockades can be painful for maritime empires
	CvPlayerAI const& victim = GET_PLAYER(victimId);
	double enemyNavy = 0;
	for(size_t i = 0; i < properCivs.size(); i++) {
		PlayerTypes enemyId =  properCivs[i];
		if(!m->isWar(victimId, enemyId) || (to != NO_PLAYER && to != enemyId))
			continue;
		MilitaryBranch const* fleet = victim.warAndPeaceAI().getCache().
				getPowerValues()[FLEET];
		// At least Frigates (Privateers can blockade regardless of war)
		if(!fleet->canBombard())
			continue;
		double pow = m->gainedPower(enemyId, FLEET) +
				GET_PLAYER(enemyId).warAndPeaceAI().getCache().getPowerValues()
				[FLEET]->power();
		if(pow > 100) // Unlikely to send any ships otherwise
			enemyNavy += pow;
	}
	double navyRatio = 2;
	double victimNavy = m->gainedPower(victimId, FLEET) + GET_PLAYER(victimId).
			warAndPeaceAI().getCache().getPowerValues()[FLEET]->power();
	if(victimNavy > 0)
		navyRatio = enemyNavy / victimNavy;
	if(navyRatio < 1.25) // Enemies won't bring all their units
		return 0;
	int coastalPop = 0;
	int totalPop = 0;
	int coastalCities = 0; int dummy;
	for(CvCity* c = victim.firstCity(&dummy); c != NULL; c = victim.nextCity(&dummy)) {
		if(!c->isRevealed(TEAMID(weId), false) ||
				m->lostCities(victimId).count(c->plotNum()) > 0)
			continue;
		int pop = c->getPopulation();
		totalPop += pop;
		if(c->isCoastal()) {
			coastalPop += pop;
			coastalCities++;
		}
	}
	if(coastalCities <= 0)
		return 0;
	double avgCoastalSize = coastalPop / (double)coastalCities;
	double attackFactor = std::min(1.0, navyRatio - 1);
	double popFactor = coastalPop / (double)totalPop;
	// Small cities tend to work few coastal tiles (b/c land tiles are better)
	double vulnerabilityFactor = std::min(1.0, 0.1 + avgCoastalSize / 25.0);
	return attackFactor * popFactor * vulnerabilityFactor;
}

double WarUtilityAspect::lossesFromNukes(PlayerTypes victimId, PlayerTypes srcId) {

	int citiesLost = m->lostCities(victimId).size();
	CvPlayerAI const& victim = GET_PLAYER(victimId);
	WarAndPeaceCache const& victimCache = victim.warAndPeaceAI().getCache();
	double scorePerCity = victimCache.totalAssetScore() / std::max(1,
			victim.getNumCities() - citiesLost);
	/*  Assume that cities lost to srcId are more likely to take a hit, and such a
		hit doesn't hurt the victim beyond the loss of the city. The remaining nukes
		are more likely to hit large cities, but some will also hit minor cities
		for tactical reasons, or no cities at all; this ought to even out.
		Also assume that the non-lost core cities are always a bit (0.33) afflicted. */
	double hits = (srcId == NO_PLAYER ? m->getNukesSufferedBy(victimId) :
			m->getNukedCities(srcId, victimId));
	hits = std::max(0.33 * hits, hits - citiesLost * 0.75);
	double lossRate = 0.35;
	// More pessimistic if we're the victim
	if(victimId == weId)
		lossRate = 0.45;
	double r = hits * (victimCache.canScrubFallout() ?
			lossRate - 0.05 : lossRate + 0.05) * scorePerCity;
	if(r > 0.5)
		log("Lost score of %s for cities hit: %d; assets per city: %d; cities hit: %.2f; ",
				report.leaderName(victimId), ::round(r),
				::round(scorePerCity), hits);
	return r;
}

// <advc.035>
double WarUtilityAspect::lossesFromFlippedTiles(PlayerTypes victimId,
		PlayerTypes srcId) {
	
	if(GC.getOWN_EXCLUSIVE_RADIUS() <= 0)
		return 0;
	double r = 0;
	int victimLostTiles = 0;
	std::vector<TeamTypes> sources;
	if(srcId == NO_PLAYER) {
		for(size_t i = 0; i < properTeams.size(); i++)
			sources.push_back(properTeams[i]);
	}
	else sources.push_back(TEAMID(srcId));
	WarAndPeaceCache const& victimCache = GET_PLAYER(victimId).warAndPeaceAI().
			getCache();
	for(size_t i = 0; i < sources.size(); i++) {
		if(m->isWar(sources[i], TEAMID(victimId))) {
			victimLostTiles += victimCache.numLostTilesAtWar(sources[i]);
		}
	}
	int const teamSz = TEAMREF(theyId).getNumMembers();
	if(std::abs(victimLostTiles) <= 4 * teamSz)
		return 0;
	int victimLandTiles = GET_PLAYER(victimId).getTotalLand();
	double const weight = 125;
	r = (weight * victimLostTiles) / std::max(5, victimLandTiles);
	r /= teamSz; // Tiles are counted per team, but this is a per-civ function.
	return r;
} // </advc.035>

double WarUtilityAspect::conqAssetScore(bool mute) {

	if(weConquerFromThem.empty())
		return 0;
	double r = 0;
	for(size_t i = 0; i < weConquerFromThem.size(); i++) {
		City* cp = ourCache->lookupCity(weConquerFromThem[i]);
		if(cp == NULL) continue; City const& c = *cp;
		// Capture gold negligible b/c it gets reduced if a city is young
		if(c.city()->isAutoRaze())
			continue;
		r += c.getAssetScore();
	}
	if(r == 0)
		return 0;
	/*  A little extra for buildings that may survive and things that are there
		but invisible to us. */
	r *= 1.1;
	if(!mute) {
		log("%d cities conquered from %s", (int)weConquerFromThem.size(),
				report.leaderName(theyId));
		log("Total asset score: %d", ::round(r));
	}
	/*  Reduce score to account for culture pressure from the current owner unless
		we expect that civ to eliminated or to be our vassal.
		Tbd.: Would better to apply this per area, i.e. no culture pressure if we
		take all their cities in one area. Then again, we may not know about
		all their cities in the area. */
	if(m->getCapitulationsAccepted(agentId).count(TEAMID(theyId)) == 0 &&
			((int)m->lostCities(theyId).size()) < they->getNumCities()) {
		// Equivalent to subtracting 50% of the mean per-city score
		r *= 1 - (1.0 / (2 * weConquerFromThem.size()));
		if(!mute)
			log("Asset score reduced to %d due to culture pressure", ::round(r));
	}
	else {
		/*  The penalties for the owner's culture applied by asset score are a
			bit high in this case. */
		r *= 1.2;
		if(!mute)
			log("Asset score increased to %d b/c enemy culture neutralized",
					::round(r));
	}
	return r;
}

double WarUtilityAspect::cityRatio(PlayerTypes civId) const {

	double n = GET_PLAYER(civId).getNumCities();
	if(n <= 0) return 0;
	return 1.0 - m->lostCities(civId).size() / n;
}

double WarUtilityAspect::partnerUtilFromTech() {

	// How good our and their attitude needs to be at least to allow tech trade
	AttitudeTypes ourThresh = techRefuseThresh(weId),
				  theirThresh = techRefuseThresh(theyId);
	if(towardsThem < ourThresh || towardsUs < theirThresh) {
		log("No tech trade b/c of attitude");
		return 0;
	}
	int weCanOffer = 0;
	int theyCanOffer = 0;
	// Don't rely on human research progress
	bool canSeeResearch = !they->isHuman() && we->canSeeResearch(theyId);
	for(int i = 0; i < GC.getNumTechInfos(); i++) {
		TechTypes tech = (TechTypes)i;
		bool weHaveIt = agent.isHasTech(tech) || agent.getResearchProgress(tech) > 0;
		bool theyHaveIt = TEAMREF(theyId).isHasTech(tech) || (canSeeResearch &&
				TEAMREF(theyId).getResearchProgress(tech) > 0);
		if(weHaveIt && !theyHaveIt)
			weCanOffer++;
		else if(theyHaveIt && !weHaveIt)
			theyCanOffer++;
	}
	if((weCanOffer == 0 || theyCanOffer == 0) &&
			std::abs(weCanOffer - theyCanOffer) > 3) {
		log("No utility from tech trade b/c progress too far apart");
		return 0;
	}
	// Humans are good at making tech trades work
	double humanBonus = 0;
	if(we->isHuman())
		humanBonus += 3;
	if(they->isHuman())
		humanBonus += 3;
	if(humanBonus > 0)
		log("Tech trade bonus for human civs: %d", ::round(humanBonus));
	// Use their commerce to determine potential for future tech trade
	double commRatio = they->estimateYieldRate(YIELD_COMMERCE) /
			we->estimateYieldRate(YIELD_COMMERCE);
	if(commRatio > 1)
		commRatio = 1 / commRatio;
	double r = std::pow(commRatio, 2) * (20 + humanBonus);
	int nearFutureTrades = std::min(weCanOffer, theyCanOffer);
	if(nearFutureTrades > 1) { // Just 1 isn't likely to result in a trade
		log("Added utility for %d foreseeable trades", nearFutureTrades);
		// Humans tend to make trades immediately, and avoid certain techs entirely
		if(they->isHuman())
			nearFutureTrades /= 2;
		r += 4 * std::min(3, nearFutureTrades);
	}
	if(r > 0)
		log("Tech trade utility: %d", ::round(r));
	/*  The above assumes that tech trade is only inhibited by overlapping research
		and diverging tech rates. If there's also AI distrust, it's worse. */
	if((!we->isHuman() && towardsThem != ATTITUDE_FRIENDLY) ||
			(!they->isHuman() && towardsUs != ATTITUDE_FRIENDLY)) {
		log("Tech trade utility halved for distrust");
		r /= 2;
	}
	return r * weAI->amortizationMultiplier();
}

AttitudeTypes WarUtilityAspect::techRefuseThresh(PlayerTypes civId) {

	// Humans are callous
	if(GET_PLAYER(civId).isHuman())
		return ATTITUDE_FURIOUS;
	// The XML value is mostly ANNOYED; they'll trade at one above that
	return (AttitudeTypes)(GC.getLeaderHeadInfo(we->getPersonalityType()).
			getTechRefuseAttitudeThreshold() + 1);
}

double WarUtilityAspect::partnerUtilFromTrade() {

	CvGame& g = GC.getGameINLINE();
	int const treatyLength = GC.getDefineINT("PEACE_TREATY_LENGTH");
	double goldVal = 0;
	int resourceTradeCount = 0;
	double tradeValFromGold = 0;
	int const defaultTimeHorizon = 25;
	int dummy;
	for(CvDeal* dp = g.firstDeal(&dummy); dp != NULL; dp = g.nextDeal(&dummy)) {
		CvDeal& d = *dp;
		if(!d.isEverCancelable(weId))
			continue;
		/*  Now ensured that the deal involves us, but not clear if we're first
			or second. */
		CLinkList<TradeData> const* weReceive = NULL;
		bool isGift = false;
		if(TEAMID(d.getFirstPlayer()) == TEAMID(theyId)) {
			weReceive = d.getFirstTrades();
			isGift = (d.getLengthSecondTrades() == 0);
		}
		if(TEAMID(d.getSecondPlayer()) == TEAMID(theyId)) {
			weReceive = d.getSecondTrades();
			isGift = (d.getLengthFirstTrades() == 0);
		}
		if(weReceive == NULL)
			continue;
		// Handle OB and DP separately (AI_dealVal just counts cities for those)
		bool skip = false;
		bool weReceiveResource = false;
		for(CLLNode<TradeData>* item = weReceive->head(); item != NULL;
				item = weReceive->next(item)) {
			if(CvDeal::isDual(item->m_data.m_eItemType)) {
				skip = true;
				continue;
			}
			if(item->m_data.m_eItemType == TRADE_RESOURCES) {
				weReceiveResource = true;
				if(skip) break; // Know everything we need
			}
			else FAssert(skip || item->m_data.m_eItemType == TRADE_GOLD_PER_TURN);
		}
		if(skip) continue;
		// Count only the first four resource trades
		if(weReceiveResource) {
			resourceTradeCount++;
			int const maxResourceTrades = 4;
			if(resourceTradeCount >= maxResourceTrades) {
				log("Skipped resource trades in excess of %d", maxResourceTrades);
				continue;
			}
		}
		/*  AI_dealVal is supposed to be gold-per-turn, but seems a bit high for
			that; hence divide by 1.5. Time horizon is ten turns (treatyLength). */
		double dealVal = we->AI_dealVal(theyId, weReceive) / (1.5 * treatyLength);
		if(!weReceiveResource) {
			int const maxTradeValFromGold = 40;
			if(tradeValFromGold + dealVal > maxTradeValFromGold) {
				log("Trade value from gold capped at %d", maxTradeValFromGold);
				dealVal = maxTradeValFromGold - tradeValFromGold;
			}
			tradeValFromGold += dealVal;
		}
		log("GPT value for a %s trade: %d", (weReceiveResource ? "resource" : "gold"),
				::round(dealVal));
		/*  Similar approach in CvPlayerAI::AI_stopTradingTradeVal. There,
			trade val is doubled if it's a gift. */
		if(!isGift)
			dealVal /= 2;
		// This actually restores the result of AI_dealVal
		double timeHorizon = defaultTimeHorizon;
		// Don't trust young deals (could be human manipulation)
		int toCancel = d.turnsToCancel();
		if(toCancel > 0) {
			timeHorizon = (toCancel + 10) / 2.0;
			log("Reduced time horizon for recent deal: %d", timeHorizon);
		}
		dealVal *= timeHorizon;
		goldVal += dealVal;
	}
	log("Net gold value of resource and gold: %d", ::round(goldVal));
	// Based on TradeUtil::calculateTradeRoutes (Python)
	double trProfit = 0;
	for(CvCity* cp = we->firstCity(&dummy); cp != NULL; cp = we->nextCity(&dummy)) {
		CvCity const& c = *cp;
		for(int i = 0; i < c.getTradeRoutes(); i++) {
			CvCity* cp2 = c.getTradeCity(i);
			if(cp2 == NULL) continue;
			CvCity const& c2 = *cp2;
			if(c2.getOwnerINLINE() != theyId)
				continue;
			/*  Division accounts for other potential trade partners,
				in the worst case, domestic. */
			trProfit += c.calculateTradeProfit(cp2) / 2.5;
		}
	}
	int const trProfitCap = 40;
	if(trProfit > trProfitCap) {
		log("Per-turn Trade route profit capped at %d", trProfitCap);
		trProfit = trProfitCap;
	}
	else if(trProfit > 0) log("Per-turn trade route profit: %d", ::round(trProfit));
	return weAI->tradeValToUtility(goldVal + trProfit * defaultTimeHorizon) *
			weAI->amortizationMultiplier();
}

double WarUtilityAspect::partnerUtilFromMilitary() {

	if((they->isHuman() || towardsUs < ATTITUDE_FRIENDLY) &&
			!agent.isDefensivePact(TEAMID(theyId)))
		return 0;
	double r = theyAI->getCache().getPowerValues()[ARMY]->power() /
			ourCache->getPowerValues()[ARMY]->power();
	log("Their military relative to ours: %d percent", ::round(100 * r));
	/*  Only count their future military support as 10% b/c there are a lot of ifs
		despite friendship/ DP */
	return std::min(r * 10, 10.0);
}

WarUtilityBroaderAspect::WarUtilityBroaderAspect(WarEvalParameters& params)
	: WarUtilityAspect(params) {}

bool WarUtilityBroaderAspect::concernsOnlyWarParties() const {

	return false;
}

GreedForAssets::GreedForAssets(WarEvalParameters& params)
	: WarUtilityAspect(params) { ourDist = -1; }

char const* GreedForAssets::aspectName() const { return "Greed for assets"; }
int GreedForAssets::xmlId() const { return 0; }

void GreedForAssets::evaluate() {

	double conqScore = conqAssetScore(false);
	if(conqScore < 0.01)
		return;
	double compMult = competitionMultiplier();
	if(compMult < 0.99 || compMult > 1.01)
		log("Competition multiplier: %d percent", ::round(compMult * 100));
	double teamSzMult = teamSizeMultiplier();
	if(teamSzMult < 0.99 || teamSzMult > 1.01)
		log("Team size multiplier: %d percent", ::round(teamSzMult * 100));
	double presentScore = ourCache->totalAssetScore();
	log("Score for present assets: %d", ::round(presentScore));
	/*  Count mundane buildings for presentScore? May build these in the
		conquered cities too eventually ... */
	double baseUtility = std::min(600.0, 300 * conqScore / (presentScore+0.001));
	log("Base utility from assets: %d", ::round(baseUtility));
	double overextCost = overextensionCost();
	double defCost = defensibilityCost();
	log("Cost modifiers: %d percent for overextension, %d for defensibility",
			::round(100 * overextCost), ::round(100 * defCost));
	double uPlus = compMult * teamSzMult * baseUtility *
			(1 - overextCost - defCost);
	// Greed shouldn't motivate peaceful leaders too much
	if(we->AI_getPeaceWeight() >= 7) {
		double cap = 260 - 10 * we->AI_getPeaceWeight();
		if(cap < uPlus) {
			uPlus = cap;
			log("Greed capped at %d b/c of peace-weight", ::round(cap));
		}
	}
	/*  Cap utility per conquered city. Relevant mostly for One-City
		Challenge, but may also matter on dense maps in the early game. */
	uPlus = std::min(uPlus, 120.0 * weConquerFromThem.size());
	u += std::max(0, ::round(uPlus));
}

double GreedForAssets::overextensionCost() {

	/*  Number-of-city maintenance and increased civic upkeep from conquered cities
		are difficult to predict; only partially covered by AssetScore.
		Look at our current expenses: are we already overextended? */
	// Don't mind paying 25% for city maintenance
	double r = -0.25;
	double ourIncome = we->estimateYieldRate(YIELD_COMMERCE);
	// Expenses excluding unit cost and supply
	double ourMaintenance = (we->getTotalMaintenance() +
			we->getCivicUpkeep(NULL, true)) *
			(we->calculateInflationRate() + 100) / 100.0;
	if(ourIncome > 0) {
		r += ourMaintenance / ourIncome;
		log("Rel. maint. = %d / %d = %d percent",
				::round(ourMaintenance), ::round(ourIncome),
				::round(100 * ourMaintenance / ourIncome));
	}
	// Can happen in later-era start if a civ immediately changes civics/ religion
	else log("Failed to estimate income");
	return std::max(0.0, r);
}

double GreedForAssets::defensibilityCost() {

	/*  Check for hostile third parties. As for the owner of 'cities'
		(if not eliminated or capitulated): we might be able to conquer more
		cities later, so, conquering cities near them could also be an advantage. */
	double threatFactor = 0;
	initCitiesPerArea();
	ourDist = medianDistFromOurConquests(weId);
	for(size_t i = 0; i < properCivs.size(); i++)
		threatFactor += threatToCities(properCivs[i]);
	freeCitiesPerArea();
	CvGame const& g = GC.getGameINLINE();
	if(ourDist > 5 && !g.isOption(GAMEOPTION_NO_BARBARIANS) &&
			g.getCurrentEra() < 2) {
		double barbThreat = 1;
		if(g.isOption(GAMEOPTION_RAGING_BARBARIANS))
			barbThreat *= 1.5;
		barbThreat *= ourDist / 60;
		barbThreat = std::min(0.25, barbThreat);
		if(barbThreat > 0.005)
			log("Threat factor from barbarians: %d percent",
					::round(barbThreat * 100));
		threatFactor += barbThreat;
	}
	if(threatFactor <= 0)
		return 0;
	// A little arcane
	double personalityPercent = 12;
	int mwmalp = GC.getLeaderHeadInfo(we->getPersonalityType()).
			getMaxWarMinAdjacentLandPercent();
	personalityPercent += 3 * mwmalp;
	/*  In BtS, 0 has a special role b/c it ignores shared borders entirely.
		That's a bit too extreme, but I'm setting it extra low.
		4 is rare (Liz, SiBull, Toku); I'm reinforcing that a bit as well. */
	if(mwmalp == 0)
		personalityPercent /= 2;
	if(mwmalp >= 3)
		personalityPercent += 5;
	log("Personality factor %d (mwmalp %d); threat factor %d", 
			::round(personalityPercent), mwmalp,
			::round(threatFactor * 100));
	return std::min(0.5, std::sqrt(threatFactor) * personalityPercent / 100);
}

double GreedForAssets::threatToCities(PlayerTypes civId) {

	CvPlayerAI& civ = GET_PLAYER(civId);
	if(civ.getTeam() == agentId || civ.getTeam() == TEAMID(theyId) ||
			GET_TEAM(civ.getTeam()).isAVassal() ||
			!agent.isHasMet(civ.getTeam()) ||
			we->getCapitalCity() == NULL ||
			civ.getCapitalCity() == NULL ||
			!we->AI_deduceCitySite(civ.getCapitalCity()) ||
			(!civ.isHuman() && civ.AI_getAttitude(weId) >= ATTITUDE_FRIENDLY))
		return 0;
	/*  Use present power if civ not part of the analysis. Add a constant b/c
		power ratios can still shift a lot in the early game. */
	double civPower = 150 + civ.warAndPeaceAI().getCache().
			getPowerValues()[ARMY]->power() + (m->isPartOfAnalysis(civId) ?
			m->gainedPower(civ.getID(), ARMY) : 0);
	double ourPower = 150 + ourCache->getPowerValues()[ARMY]->power() +
			(m->isPartOfAnalysis(civId) ?
			m->gainedPower(weId, ARMY) : 0);
	/*  Skip civs that aren't currently a threat.
		Add some utility for weak civs? Could use the conquered cities as a
		bridgehead for a later war. Too uncertain I think. */
	if(2 * ourPower > 3 * civPower)
		return 0;
	int areaId = civ.getCapitalCity()->area()->getID();
	iMapIt pos = citiesPerArea[civ.getID()]->find(areaId);
	if(pos == citiesPerArea[civ.getID()]->end() ||
			pos->second <= (int)weConquerFromThem.size() / 2)
		return 0;
	// Only worry about civs that are closer to our conquests than we are
	double ourDist = medianDistFromOurConquests(weId);
	double civDist = medianDistFromOurConquests(civId);
	if(5 * civDist >= 4 * ourDist || civDist > 10)
		return 0;
	log("Dangerous civ near our conquests: %s (dist. ratio %d/%d)",
			report.leaderName(civ.getID()), ::round(civDist), ::round(ourDist));
	double powerRatio = std::max(0.0, civPower / std::max(10.0, ourPower) - 0.1);
	FAssert(powerRatio > 0);
	return powerRatio * powerRatio;
}

double GreedForAssets::medianDistFromOurConquests(PlayerTypes civId) {

	CvPlayerAI const& civ = GET_PLAYER(civId);
	vector<double> distances;
	for(size_t i = 0; i < weConquerFromThem.size(); i++) {
		/*  Cheating a bit: We know where the capital of 'civ' is, but the
			distance info is based on all of 'civ' cities. For a human,
			it's almost always easy to tell whether one civ is closer to a set
			of cities than another, so the AI figuring it out magically isn't
			really an issue for me. */
		City* cp = civ.warAndPeaceAI().getCache().lookupCity(weConquerFromThem[i]);
		if(cp == NULL) continue; City const& c = *cp;
		int d = c.getDistance();
		if(!c.canReachByLand()) // Don't worry about naval attacks
			d += 100;
		if(d < 0) d = INT_MAX; // -1 means unreachable
		distances.push_back(d);
	}
	double r = INT_MAX;
	if(!distances.empty())
		r = ::median(distances);
	return r;
}

double GreedForAssets::competitionMultiplier() {

	FAssert(weConquerFromThem.size() > 0); /* Shouldn't have to compute this
											  otherwise */
	int theirCities = they->getNumCities();
	/*  Only need this in the early game (I hope), and only if there are few
		cities to go around */
	if(GC.getGameINLINE().gameTurnProgress() >= 0.25 || theirCities >= 5 ||
			theirCities <= 0 || weConquerFromThem.size() <= 0)
		return 1;
	/*  MilitaryAnalyst already takes our war allies into account, and tells us
		which cities the allies conquer, and which ones we get. However, this is
		based on just one simulation trajectory, and if they (the target) have
		few cities, we might actually get none at all. Compute a reduction rate
		for our GreedForAssets to account for that risk.
		Should reduce early-game dogpiling. */
	int competitors = 0;
	for(size_t i = 0; i < properCivs.size(); i++) {
		CvPlayer const& competitor = GET_PLAYER(properCivs[i]);
		if(competitor.getTeam() == agentId ||
				GET_TEAM(competitor.getTeam()).isAVassal() ||
				!m->isWar(competitor.getID(), theyId))
			continue;
		/*  Only worry if MilitaryAnalyst says that civ conquers at least as much
			as we do */
		unsigned int civConqFromThem = 0;
		for(iSetIt it = m->conqueredCities(competitor.getID()).begin();
				it != m->conqueredCities(competitor.getID()).end(); it++) {
			if(m->lostCities(theyId).count(*it) > 0)
				civConqFromThem++;
		}
		if(civConqFromThem >= weConquerFromThem.size())
			competitors++;
	}
	FAssert(theirCities > competitors); // B/c we conquer at least 1 as well
	return std::max(0.0, 1 - competitors / (double)theirCities);
}

double GreedForAssets::teamSizeMultiplier() {

	/*  To account for team-on-team warfare having a somewhat slower pace than
		civ-on-civ, meaning that further conquests tend to happen after the
		time horizon of the simulation. (A longer horizon could be too costly
		computationally.) And to account for military assistance within a team
		being a bit underestimated by InvasionGraph (which can't be helped). */
	double teamSz = std::min(agent.getAliveCount(), TEAMREF(theyId).getAliveCount());
	return std::min(1.43, (1 + std::sqrt(teamSz)) / 2);
}

void GreedForAssets::initCitiesPerArea() {

	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
		citiesPerArea[i] = new map<int,int>();
	for(int i = 0; i < ourCache->size(); i++) {
		City* cp = ourCache->getCity(i);
		if(cp == NULL) continue; City const& c = *cp;
		PlayerTypes ownerId = c.city()->getOwnerINLINE();
		// City may have been conquered by barbarians since the last update
		if(ownerId == BARBARIAN_PLAYER || ownerId == NO_PLAYER)
			continue;
		int areaId = c.city()->area()->getID();
		iMapIt pos = citiesPerArea[ownerId]->find(areaId);
		if(pos == citiesPerArea[ownerId]->end())
			citiesPerArea[ownerId]->insert(pair<int,int>(areaId, 1));
		else (*citiesPerArea[ownerId])[areaId]++;
	}
}

void GreedForAssets::freeCitiesPerArea() {

	for(int i = 0; i < MAX_CIV_PLAYERS; i++)
		SAFE_DELETE(citiesPerArea[i]);
}

GreedForVassals::GreedForVassals(WarEvalParameters& params)
	: WarUtilityAspect(params) {}

char const* GreedForVassals::aspectName() const { return "Greed for vassals"; }
int GreedForVassals::xmlId() const { return 1; }

void GreedForVassals::evaluate() {

	// Military analysis already checks if vassal states disabled in options
	set<TeamTypes> const& newVassals = m->getCapitulationsAccepted(agentId);
	// Will need their capital later
	if(newVassals.count(TEAMID(theyId)) == 0 || they->getCapitalCity() == NULL)
		return;
	double ourIncome = std::max(8.0, we->estimateYieldRate(YIELD_COMMERCE));
	double totalUtility = 0;
	/*  Their commerce may be lower than now at the end of the war.
		(Don't try to estimate our own post-war commerce though.) */
	double cr = cityRatio(theyId);
	log("Ratio of cities kept by the vassal: cr=%d percent", ::round(cr * 100));
	// Vassal commerce to account for tech they might research for us
	double vassalIncome = they->estimateYieldRate(YIELD_COMMERCE);
	double relIncome = vassalIncome * cr / ourIncome;
	log("Rel. income of vassal %s = %d percent = cr * %d / %d",
			report.leaderName(theyId), ::round(100 * relIncome),
			::round(vassalIncome), ::round(ourIncome));
	double utilityFromTechTrade = 100 * relIncome;
	// Tech they already have
	double techScore = ourCache->vassalTechScore(theyId);
	log("Vassal score from tech they have in advance of us: %d",
			::round(techScore));
	utilityFromTechTrade += weAI->tradeValToUtility(techScore);
	log("Utility from vassal tech and income (still to be reduced): %d",
			::round(utilityFromTechTrade));
	/*  If they're much more advanced than us, we won't be able to trade for
		all their tech. */
	utilityFromTechTrade = std::min(100.0, utilityFromTechTrade);
	/*  Reduced a lot b/c of uncertainty and delays in tech trades. Expect human
		master to fare a bit better. */
	totalUtility += (agent.isHuman() ? 0.5 : 0.35) * utilityFromTechTrade;
	double utilityFromResources = ourCache->vassalResourceScore(theyId);
	log("Resource score: %d", ::round(utilityFromResources));
	// Expect just +1.5 commerce per each of their cities from trade routes
	double utilityFromTR = 1.5 * they->getNumCities() / ourIncome;
	log("Trade route score: %d", ::round(utilityFromTR));
	// These trades are pretty safe bets; treated as 75% probable
	totalUtility += 0.75 * cr * (utilityFromResources + utilityFromTR);
	int nVassalCivs = 0;
	for(set<TeamTypes>::const_iterator it = newVassals.begin();
			it != newVassals.end(); it++)
		nVassalCivs += GET_TEAM(*it).getNumMembers();
	for(size_t i = 0; i < properTeams.size(); i++)
		if(GET_TEAM(properTeams[i]).isVassal(agentId))
			nVassalCivs += GET_TEAM(properTeams[i]).getNumMembers();
	// Diminishing returns from having additional trade partners
	totalUtility /= std::sqrt((double)nVassalCivs);
	int ourArea = we->getCapitalCity()->area()->getID();
	int theirArea = they->getCapitalCity()->area()->getID();
	bool isUsefulArea = (ourArea == theirArea);
	if(isUsefulArea) {
		isUsefulArea = false;
		for(size_t i = 0; i < properCivs.size(); i++) {
			CvPlayerAI const& civ = GET_PLAYER(properCivs[i]);
			CvTeamAI const& t = GET_TEAM(civ.getTeam());
			if(civ.getCapitalCity() == NULL || m->isEliminated(civ.getID()) ||
					m->hasCapitulated(t.getID()))
				continue;
			if(civ.getCapitalCity()->area()->getID() == ourArea &&
					!t.isAVassal() && newVassals.count(civ.getTeam()) == 0 &&
					(civ.isHuman() || t.AI_getAttitude(agentId) < ATTITUDE_PLEASED ||
					we->isHuman() || agent.AI_getAttitude(t.getID()) < ATTITUDE_PLEASED)) {
				isUsefulArea = true;
				break;
			}
		}
	}
	double theirPower = theyAI->getCache().getPowerValues()[ARMY]->power();
	double ourPower = ourCache->getPowerValues()[ARMY]->power();
	log("Their army/ ours: %d/%d", ::round(theirPower), ::round(ourPower));
	// 55% reduction for coordination problems and vassal not fully committing
	double utilityFromMilitary = 0.45 * cr * 100 * theirPower /
			std::max(ourPower, 10.0);
	log("Base utility from military: %d", ::round(utilityFromMilitary));
	if(!isUsefulArea) {
		utilityFromMilitary /= 2;
		log("Utility halved b/c vassal not in a useful area");
	}
	utilityFromMilitary = std::min(30.0, utilityFromMilitary);
	totalUtility += utilityFromMilitary;
	/*  To account for downsides of vassals, in particular wars with third parties
		that dislike the vassal: */
	totalUtility -= 5;
	u += std::max(0, ::round(totalUtility));
}

GreedForSpace::GreedForSpace(WarEvalParameters& params)
	: WarUtilityAspect(params) {}

char const* GreedForSpace::aspectName() const { return "Greed for space"; }
int GreedForSpace::xmlId() const { return 2; }

void GreedForSpace::evaluate() {

	/*  If they lose cities, but aren't eliminated, they'll try to settle sites
		aggressively; doesn't help us. */
	if(!m->isEliminated(theyId) || ourCache->numAdjacentLandPlots(theyId) <= 0)
		return;
	int ourCities = std::max(1, we->getNumCities());
	int ourSites = std::min(ourCities, we->AI_getNumCitySites());
	int theirSites = they->AI_getNumCitySites();
	// Expect to raze only when we have to
	for(size_t i = 0; i < weConquerFromThem.size(); i++) {
		City* c = ourCache->lookupCity(weConquerFromThem[i]);
		if(c != NULL && c->city()->isAutoRaze())
			theirSites++;
	}
	/*  All sites could still be claimed by third parties or could be just barely
		worth settling. Their sites are extra uncertain because they could be far
		away from our capital. (Better times 0.6 instead of 2/3?) */
	double expectedSites = ourSites + (2.0/3) * theirSites;
	// Don't expect to more than double our territory in the foreseeable future
	expectedSites = std::min((double)ourCities, expectedSites);
	// Increase in sites relative to our current city count
	double incr = (expectedSites - ourSites) / ourCities;
	FAssert(incr >= 0 && incr <= 1);
	int uPlus = ::round(40 * incr * weAI->amortizationMultiplier());
	if(uPlus > 0.5 || uPlus < -0.5) {
		log("Their orphaned sites: %d, our current sites: %d, our current cities: %d",
				theirSites, ourSites, ourCities);
		u += uPlus;
	}
}

GreedForCash::GreedForCash(WarEvalParameters& params)
	: WarUtilityAspect(params) {}
char const* GreedForCash::aspectName() const { return "Greed for cash"; }
int GreedForCash::xmlId() const { return 3; }

void GreedForCash::evaluate() {

	int theyLoseToUs = 0;
	for(iSetIt it = m->conqueredCities(weId).begin();
			it != m->conqueredCities(weId).end(); it++)
		if(m->lostCities(theyId).count(*it) > 0)
			theyLoseToUs++;
	int weLoseToThem = 0;
	for(iSetIt it = m->conqueredCities(theyId).begin();
			it != m->conqueredCities(theyId).end(); it++)
		if(m->lostCities(weId).count(*it) > 0)
			weLoseToThem++;
	if(theyLoseToUs > weLoseToThem && !m->isEliminated(theyId)) {
		log("Adding utility for future reparations");
		u += 4;
	}
}

Loathing::Loathing(WarEvalParameters& params)
	: WarUtilityAspect(params) {}

void Loathing::evaluate() {

	/*  Only count this if at war with them? No, that could lead to us joining 
		a war despite not contributing anything to their losses.
		Can't tell who causes their losses. */
	/*bool atWar = (agent.isAtWar(TEAMID(theyId) &&
			!m->getWarsContinued(weId).count(theyId)>0) ||
			(!m->isPeaceScenario() && (m->getWarsDeclaredOn(theyId).count(weId)>0 ||
			m->getWarsDeclaredOn(weId).count(theyId)>0)));*/
	if((towardsThem > ATTITUDE_FURIOUS && (agent.isAtWar(TEAMID(theyId)) ||
			/*  I.e. only loath them if a) furious or b) at peace and worst enemy.
				Rationale: When at war, it's not unusual for them to become our
				worst enemy, but Furious attitude is unusual even when at war. */
			agent.AI_getWorstEnemy() != TEAMID(theyId))) ||
			// Capitulated is as good as dead
			TEAMREF(theyId).isCapitulated())
		return;
	int veng = weAI->vengefulness();
	if(veng == 0) {
		log("No loathing b/c of our leader's personality");
		return;
	}
	double lr = 100 * ::dRange(lossRating(), -1.0, 1.0);
	if(abs(lr) < 0.01)
		return;
	log("Loss rating for %s: %d", report.leaderName(theyId), ::round(lr));
	log("Our vengefulness: %d", veng);
	int nNonVassalCivs = numRivals;
	if(!agent.isAVassal())
		nNonVassalCivs += agent.getNumMembers();
	/*  Obsessing over one enemy is unwise if there are many potential enemies.
		Rounded down deliberately b/c it would be a bit too high otherwise. */
	int rivalDivisor = (int)std::sqrt((double)nNonVassalCivs);
	log("Divisor from number of rivals (%d): %d",
			nNonVassalCivs, rivalDivisor);
	// Utility proportional to veng and lr would be too extreme
	double fromLosses = 5 * std::sqrt(veng * abs(lr));
	if(lr < 0) fromLosses *= -1;
	// We like to bring in war allies against our nemesis
	int nJointDoW = 0;
	for(size_t i = 0; i < properCivs.size(); i++) {
		PlayerTypes civId = properCivs[i];
		if(GET_PLAYER(civId).getMasterTeam() != agent.getMasterTeam() &&
				m->getWarsDeclaredOn(theyId).count(civId) > 0) {
			log("Joint DoW by %s", report.leaderName(civId));
			nJointDoW++;
		}
	}
	double fromDiplo = 10 * std::sqrt((double)veng) * nJointDoW;
	u += ::round((fromLosses + fromDiplo) /
			// Count Loathing only once per team member
			(rivalDivisor * (int)agentTeam.size()));
}

/*  How much they're losing compared with what we have. I.e. we don't much care
	for kicking them when they're down. */
double Loathing::lossRating() {

	if(m->isEliminated(theyId) || m->hasCapitulated(TEAMID(theyId))) {
		CvGame& g = GC.getGameINLINE();
		log("Loss rating based on score");
		return g.getPlayerScore(theyId) / (double)std::max(10, g.getPlayerScore(weId));
	}
	/*  We mainly care about how much they lose relative to what we have; only
		use their present status as a sanity check, so that we don't get excited
		if we're doing badly and they suffer some minor losses. */
	double theirScore = 0;
	double theirLostScore = lostAssetScore(NO_PLAYER, &theirScore);
	for(iSetIt it = m->lostCities(theyId).begin(); it != m->lostCities(theyId).end();
			it++) {
		City* c = theyAI->getCache().lookupCity(*it);
		if(c == NULL || !agent.AI_deduceCitySite(c->city()))
			continue;
		/*  Already included in theirLostScore, but count it once more for the
			symbolic value. */
		if(c->city()->isCapital())
			theirLostScore += 4;
	}
	double ourScore = std::max(1.0, ourCache->totalAssetScore());
	double assetRatio = std::min(theirLostScore / ourScore,
			3 * theirLostScore / std::max(1.0, theirScore));
	if(assetRatio > 0.005)
		log("Their lost assets: %d; their present assets: %d;"
				" our present assets: %d; asset ratio: %d percent",
				::round(theirLostScore), ::round(theirScore), ::round(ourScore),
				::round(100 * assetRatio));
	/*  This is mostly about their losses, and not ours, but we shouldn't be
		satisfied to have weakened their army if ours fares far worse.
		Slightly weird side-effect: Peace scenario tends to have slightly higher
		utility for Loathing even if we only fight against other civs. */
	double deltaLostPower = std::max(0.0, m->lostPower(theyId, ARMY) +
		// Never mind losses in HOME_GUARD; LOGISTICS is included in FLEET
			m->lostPower(theyId, FLEET) - 0.5 * (m->lostPower(weId, ARMY) +
			m->lostPower(weId, FLEET)));
	vector<MilitaryBranch*> const& ourMilitary = ourCache->getPowerValues();
	vector<MilitaryBranch*> const& theirMilitary = theyAI->getCache().getPowerValues();
	/*  Future power is a better point of reference; AI build-up during war
		preparations and reinforcements throughout a war tend to dwarf
		present military power. */
	double ourPower = ourMilitary[ARMY]->power() + ourMilitary[FLEET]->power()
			+ m->gainedPower(weId, ARMY) + m->gainedPower(weId, FLEET) + 0.01;
	double theirPower = theirMilitary[ARMY]->power() + theirMilitary[FLEET]->power()
			+ m->gainedPower(theyId, ARMY) + m->gainedPower(theyId, FLEET) + 0.01;
	double militaryRatio = std::min(deltaLostPower / (ourPower+0.1),
			3 * deltaLostPower / (theirPower+0.1));
	if(abs(militaryRatio) > 0.005)
		log("Difference in lost power (adjusted): %d; their projected power: %d;"
			" our projected power: %d; military ratio: %d percent",
			::round(deltaLostPower), ::round(theirPower), ::round(ourPower),
			::round(100 * militaryRatio));
	/*  Military ratio is less important, and also tends to be high b/c peace-time
		armies (ourPower, theirPower) are often small compared to war-time armies. */
	return (2 * assetRatio + 0.5 * std::min(1.8, militaryRatio)) / 3;
	/*  The military ratio is important when we can't conquer any of their cities,
		i.e. in all cases when we loath a stronger opponent. Hence all the checks
		and balances. */
}

char const* Loathing::aspectName() const { return "Loathing"; }
int Loathing::xmlId() const { return 4; }

MilitaryVictory::MilitaryVictory(WarEvalParameters& params)
	: WarUtilityAspect(params) {}

void MilitaryVictory::evaluate() {

	// Vassals shouldn't strive for victory; not until breaking free.
	if(agent.isAVassal())
		return;
	double progressRating = 0;
	int nVictories = 0;
	double maxProgress = -1;
	/*  The distinction between stage 3 and 4 is too coarse to be useful; treat
		them the same way. */
	if(we->AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3 | AI_VICTORY_CONQUEST4)) {
		double prgr = ::dRange(progressRatingConquest(), 0.0, 1.0);
		if(prgr > maxProgress)
			maxProgress = prgr;
		progressRating += prgr;
		nVictories++;
	}
	if(we->AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION3 | AI_VICTORY_DOMINATION4)) {
		double prgr = ::dRange(progressRatingDomination(), 0.0, 1.0);
		if(prgr > maxProgress)
			maxProgress = prgr;
		progressRating += prgr;
		nVictories++;
	}
	// Diplomatic victories can come through military means
	if(we->AI_isDoVictoryStrategy(AI_VICTORY_DIPLOMACY3 | AI_VICTORY_DIPLOMACY4)) {
		double prgr = ::dRange(progressRatingDiplomacy(), 0.0, 1.0);
		if(prgr > maxProgress)
			maxProgress = prgr;
		progressRating += prgr;
		nVictories++;
	}
	if(nVictories == 0)
		return;
	FAssert(maxProgress >= 0);
	if(progressRating > 0) {
		log("Rivals remaining: %d", numRivals);
		log("Pursued mil. victory conditions: %d", nVictories);
	}
	if(numRivals <= 0)
		return;
	/*  Division by (sqrt of) nVictories because it's not so useful to pursue
		several victory conditions at once. But don't go below the max of the
		progress values. */
	double prgr = std::max(maxProgress, progressRating /
			std::sqrt((double)nVictories));
	// The fewer rivals remain, the more we're willing to go all-in for victory
	double div = 2 + std::sqrt((double)numRivals);
	double uPlus = 540 * prgr / div;
	/*  May lose votes if nuked. This could be better integrated with the progress
		ratings above. First had it as a separate class, now kind of tacked on. */
	double voteCost = 0;
	double hitUs = m->getNukedCities(theyId, weId);
	if(hitUs > 0.01 && (we->AI_isDoVictoryStrategy(AI_VICTORY_DIPLOMACY3 |
			/*  The other peaceful victories are similar enough to Diplo;
				don't want our population to be nuked. */
			AI_VICTORY_CULTURE3 | AI_VICTORY_SPACE3) &&
			// Don't fret if we can also win differently
			(!we->AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION3) &&
			!we->AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION4) &&
			!we->AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3) &&
			!we->AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST4) &&
			// Probably too late for nukes to stop us then
			!we->AI_isDoVictoryStrategy(AI_VICTORY_SPACE4) &&
			!we->AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4)))) {
		// Slow to grow back if we can't clean up
		double popLossRate = hitUs * (ourCache->canScrubFallout() ? 0.35 : 0.45) /
				(we->getNumCities() - m->lostCities(weId).size());
		popLossRate = std::min(0.5, popLossRate);
		voteCost += popLossRate * 60;
		if(we->AI_isDoVictoryStrategy(AI_VICTORY_DIPLOMACY4))
			voteCost += popLossRate * 40;
		if(voteCost > 0.5) {
			log("Nuclear war jeopardizes diplo victory: %d "
					"(%d percent population loss expected)",
					::round(voteCost), ::round(100 * popLossRate));
		}
	}
	uPlus -= voteCost;
	u += ::round(uPlus);
}

int MilitaryVictory::preEvaluate() {

	votesToGo = agentAI.computeVotesToGoForVictory();
	// Rather try to get the last few votes through growth
	enoughVotes = (votesToGo < 8);
	return 0;
}

double MilitaryVictory::progressRatingConquest() {

	if(m->isEliminated(theyId) ||
			m->getCapitulationsAccepted(agentId).count(TEAMID(theyId))) {
		log("They're conquered entirely");
		return 1;
	}
	// If we don't take them out entirely - how much do they lose
	double theirScore = 0;
	double theirLostScore = lostAssetScore(weId, &theirScore);
	/*  Utility of vassals is normally computed separately (linked wars),
		but in this case, our vassals may do something that helps the master
		specifically (namely bring down our last remaining rivals). */
	for(size_t i = 0; i < properCivs.size(); i++)
		if(TEAMREF(properCivs[i]).isVassal(agentId))
			theirLostScore += lostAssetScore(properCivs[i]);
	if(theirScore < 25 || theirLostScore < 5)
		return 0;
	double r = std::min(theirLostScore / (theirScore+0.1), 1.0);
	log("Their loss ratio: %d percent", ::round(100 * r));
	/*  Reduced to 2/3 because we can't be sure that we'll eventually
		finish them off. In particular, they could become someone else's vassal. */
	return (2 * r) / 3;
}

double MilitaryVictory::progressRatingDomination() {

	CvGame& g = GC.getGameINLINE();
	VictoryTypes dom = NO_VICTORY;
	/*  There is no literal for domination victory. I don't care about
		supporting modded custom victory conditions; just find the index of
		domination.
		(Same thing is done in CvPlayerAI::AI_calculateDominationVictoryStage.) */
	for(int i = 0; i < GC.getNumVictoryInfos(); i++) {
		VictoryTypes victId = (VictoryTypes)i;
		CvVictoryInfo& vict = GC.getVictoryInfo(victId);
		if(vict.getLandPercent() > 0) {
			dom = victId;
			break;
		}
	}
	if(dom == NO_VICTORY || m->conqueredCities(weId).empty())
		return 0;
	double popToGo = g.getAdjustedPopulationPercent(dom) * g.getTotalPopulation() /
			100.0 - agent.getTotalPopulation();
	double ourLandRatio = agent.getTotalLand() /
			((double)GC.getMapINLINE().getLandPlots());
	// How close we are to the victory threshold
	double landVictoryRatio =  (100 * ourLandRatio) / g.getAdjustedLandPercent(dom);
	// Dealing with land tiles is too complicated. Need a target number of cities.
	/*  If landVictoryRatio is what our current cities give us, we'll need this
		many for victory: */
	double targetCities = agent.getNumCities() / (landVictoryRatio+0.001);
	/*  But the additional cities may well give us less land than our current cities.
		How many cities would we need if the goal was to have a certain fraction
		of cities rather than land tiles? Use the average of these two predictions. */
	targetCities = (targetCities +
			0.01 * g.getAdjustedLandPercent(dom) * g.getNumCities()) / 2;
	double citiesToGo = targetCities - agent.getNumCities();
	double popGained = 0;
	double citiesGained = 0;
	set<int> const& conq = m->conqueredCities(weId);
	for(iSetIt it = conq.begin(); it != conq.end(); it++) {
		if(m->lostCities(theyId).count(*it) == 0)
			continue;
		City* cp = ourCache->lookupCity(*it);
		if(cp == NULL) continue; City const& c = *cp;
		/*  Minus 0.5 for pop lost upon conquest (not -1 b/c target decreases
			as well) */
		popGained += c.city()->getPopulation() - 0.5;
		citiesGained = citiesGained + 1;
	}
	if(m->getCapitulationsAccepted(agentId).count(TEAMID(theyId)) > 0) {
		int theirPopRemaining = they->getTotalPopulation();
		set<int> const& lost = m->lostCities(theyId);
		FAssertMsg(!lost.empty(), "They capitulated w/o losing a city?");
		int theirCitiesRemaining = they->getNumCities() - lost.size();
		for(iSetIt it = lost.begin(); it != lost.end(); it++) {
			City* cp = ourCache->lookupCity(*it);
			if(cp == NULL)
				continue;
			theirPopRemaining -= cp->city()->getPopulation();
		}
		if(theirCitiesRemaining > 0) {
			log("%d vassal pop gained, and %d cities", theirPopRemaining,
					theirCitiesRemaining);
		}
		citiesGained += 0.5 * theirCitiesRemaining;
		popGained += 0.5 * theirPopRemaining;
	}
	if(citiesGained == 0)
		return 0;
	log("%d population-to-go for domination, %d cities", ::round(popToGo),
			::round(citiesToGo));
	FAssert(popGained > 0);
	log("%d pop gained, and %d cities", ::round(popGained), ::round(citiesGained));
	// No use in population beyond the victory threshold
	popGained = std::min(popGained, popToGo);
	/*  The to-go values can be negative (already past the threshold). In the case
		of citiesToGo it's not clear if we've really reached the land threshold -
		perhaps our cities own relatively few land tiles. If popToGo is negative,
		it's certain we've reached the pop threshold. */
	if(popToGo < 0) {
		if(citiesToGo < 0) {
			/*  Boths thresholds met, but somehow we haven't won. Assume we need 
				a few more cities. */
			log("Cities-to-go negative, set to %d", ::round(citiesToGo));
			citiesToGo = 5;
		}
		else citiesGained = std::min(citiesGained, citiesToGo);
		return citiesGained / citiesToGo;
	}
	else {
		if(citiesToGo < 0)
			// Assume that we have enough land.
			return popGained / popToGo;
		log("Progress towards domination based on both gained cities and population");
		return 0.5 * (citiesGained / citiesToGo + popGained / popToGo);
	}
}

double MilitaryVictory::progressRatingDiplomacy() {

	if(enoughVotes) {
		log("Votes for diplo victory already secured");
		return 0;
	}
	VoteSourceTypes voteSource = agent.getLatestVictoryVoteSource();
	if(voteSource == NO_VOTESOURCE) {
		log("No vote source yet"); // DIPLO3 should normally rule that out
		return 0;
	}
	bool isUN = GC.getVoteSourceInfo(voteSource).getVoteInterval() < 7;
	ReligionTypes apRel = GC.getGameINLINE().getVoteSourceReligion(voteSource);
	double popGained = 0;
	map<int,double> conquestsWithWeights;
	set<int> conqFriendly;
	set<int> conqPleased;
	set<int> const& conq = m->lostCities(theyId);
	int apObstacles = agentAI.countNonMembers(voteSource);
	int apObstaclesRemoved = 0;
	/*  Vassal conquests just as good as our own for getting votes.
		Conquests by friends and partners can only help a little (reduced weight). */
	for(iSetIt it = conq.begin(); it != conq.end(); it++)
		if(m->conqueredCities(weId).count(*it))
			conquestsWithWeights.insert(pair<int,double>(*it, 1));
	for(size_t i = 0; i < properCivs.size(); i++) {
		PlayerTypes civId = properCivs[i];
		if(TEAMID(civId) == agentId)
			continue;
		if(TEAMREF(civId).isVassal(agentId) &&
				GET_PLAYER(civId).isVotingMember(voteSource)) {
			// Half weight for vassals that are mere voting members
			double weight = 0.5;
			if(isUN || GET_PLAYER(civId).isFullMember(voteSource))
				weight = 1;
			for(iSetIt it = conq.begin(); it != conq.end(); it++)
				if(m->conqueredCities(civId).count(*it))
					conquestsWithWeights.insert(pair<int,double>(*it, weight));
			continue;
		}
	}
	// Need to deal with them somehow for AP victory
	if(!isUN && !they->isVotingMember(voteSource)) {
		if(m->isEliminated(theyId))
			apObstaclesRemoved++;
	}
	double obstacleProgress = 0;
	if(apObstacles > 0) {
		obstacleProgress = apObstaclesRemoved / (double)apObstacles;
		if(obstacleProgress > 0)
			log("Progress on AP obstacles: %d percent",
					::round(100 * obstacleProgress));
	}
	if(isUN || they->isVotingMember(voteSource)) {
		double membershipMult = 1;
		if(!they->isFullMember(voteSource))
			membershipMult = 0.5;
		// Conquests by friends from non-friends: 2/3 weight
		addConquestsByPartner(conquestsWithWeights, ATTITUDE_FRIENDLY, (2.0/3)
				* membershipMult);
		// Pleased: 1/3
		addConquestsByPartner(conquestsWithWeights, ATTITUDE_PLEASED, (1.0/3)
				* membershipMult);
	}
	for(map<int,double>::const_iterator it = conquestsWithWeights.begin();
			it != conquestsWithWeights.end(); it++) {
		City* cp = ourCache->lookupCity(it->first);
		if(cp == NULL) continue; City const& c = *cp;
		/*  Apply weight for new owner. -0.5 for population loss upon conquest
			(not -1 b/c total decreases as well). */
		double pop = it->second * (c.city()->getPopulation() - 0.5);
		PlayerTypes cityOwnerId = c.city()->getOwnerINLINE();
		// Weight for old owner
		AttitudeTypes att = GET_PLAYER(cityOwnerId).AI_getAttitude(weId);
		if(!TEAMREF(cityOwnerId).isHuman() &&
				/*  AP: Conquests from non-full members are always worthwhile
					b/c, as full members, we can make their votes count twice. */
				(isUN || they->isFullMember(voteSource))) {
			if(att >= ATTITUDE_FRIENDLY)
				pop /= 3;
			if(att == ATTITUDE_PLEASED)
				pop = 2 * pop / 3;
		}
		if(!isUN && !c.city()->isHasReligion(apRel))
			pop *= 0.5; // Not 0 b/c religion can still be spread
		log("Votes expected from %s: %d", report.cityName(*c.city()), ::round(pop));
		popGained += pop;
	}
	if(m->getCapitulationsAccepted(agentId).count(TEAMID(theyId)) > 0) {
		double newVassalVotes = 0;
		for(int i = 0; i < ourCache->size(); i++) {
			City* cp = ourCache->getCity(i); if(cp == NULL) continue;
			if(m->lostCities(theyId).count(cp->id()) > 0)
				continue;
			CvCity const& c = *cp->city();
			if(c.getOwnerINLINE() != theyId &&
					m->conqueredCities(theyId).count(cp->id()) <= 0)
				continue;
			double pop = c.getPopulation();
			if(!isUN && !c.isHasReligion(apRel))
				pop *= 0.5;
			newVassalVotes += pop;
		}
		if(isUN || they->isFullMember(voteSource)) {
			AttitudeTypes att = GET_PLAYER(theyId).AI_getAttitude(weId);
			if(att >= ATTITUDE_FRIENDLY)
				newVassalVotes /= 3;
			if(att == ATTITUDE_PLEASED)
				newVassalVotes = 2 * newVassalVotes / 3;
		}
		log("Votes expected from capitulated cities of %s: %d",
				report.leaderName(theyId), ::round(newVassalVotes));
		popGained += newVassalVotes;
	}
	if(popGained > 0.5)
		log("Total expected votes: %d, current votes-to-go: %d",
				::round(popGained), ::round(votesToGo));
	FAssert(votesToGo > 0);
	popGained = std::min(popGained, votesToGo);
	double r = popGained / votesToGo;
	if(apObstacles < 1)
		return r;
	// If there are AP obstacles, give the progress rate on those 25% weight.
	return 0.25 * obstacleProgress + 0.75 * r;
}

void MilitaryVictory::addConquestsByPartner(map<int,double>& r,
		AttitudeTypes attThresh, double weight) {

	// Humans won't vote for us unless vassal'd
	if((TEAMREF(theyId).isHuman() && !TEAMREF(theyId).isVassal(agentId)) ||
			towardsUs < attThresh)
		return;
	set<int> const& conq = m->conqueredCities(theyId);
	for(iSetIt it = conq.begin(); it != conq.end(); it++) {
		City* cp = ourCache->lookupCity(*it);
		if(cp == NULL) continue; City const& c = *cp;
		PlayerTypes cityOwnerId = c.city()->getOwnerINLINE();
		// Never good news when we lose cities
		if(GET_PLAYER(cityOwnerId).getMasterTeam() == agent.getMasterTeam())
			continue;
		/*  Don't bother checking AP membership here (although it would be smart
			to count conquests by friends that are full members at the expense of
			friends that aren't) */
		if(GET_PLAYER(cityOwnerId).AI_getAttitude(weId) < attThresh ||
				TEAMREF(cityOwnerId).isHuman())
			r.insert(pair<int,double>(*it, weight));
	}
}

char const* MilitaryVictory::aspectName() const { return "Military Victory"; }
int MilitaryVictory::xmlId() const { return 5; }

Assistance::Assistance(WarEvalParameters& params)
	: WarUtilityAspect(params) {}

void Assistance::evaluate() {

	/*  Could use CvPlayerAI::AI_stopTradingTradeVal, but that also accounts for
		angering the civ we stop trading with, and it's a bit too crude.
		I'm adopting a bit of code in partnerUtilFromTrade. */
	if(TEAMREF(theyId).isAVassal() || /* Don't want to end up helping a potentially
										 dangerous master. */
			// Their utility is added to ours in the end; don't count it here.
			they->getTeam() == agentId ||
			/*  These checks can make us inclined to attack a friend if it's already
				under attack; b/c the peace scenario will have reduced utility from
				Assistance and the war scenario won't. I guess that's OK.
				"At least, if we attack them too, we don't have to worry about
				their losses (because they won't trade with us anymore anyway)."
				Costs from Affection and Ill Will should still lead to a sensible
				decision overall. */
			agent.AI_getWarPlan(TEAMID(theyId)) != NO_WARPLAN ||
			m->getWarsDeclaredBy(weId).count(theyId) > 0 ||
			m->getWarsDeclaredOn(weId).count(theyId) > 0)
		return;
	// Shared-war bonus handled under SuckingUp
	if((we->isHuman() && towardsUs < ATTITUDE_CAUTIOUS) ||
			(!we->isHuman() && towardsThem < ATTITUDE_PLEASED))
		return;
	double assistRatio = assistanceRatio();
	if(assistRatio > 0)
		log("Expecting them to lose %d percent of their assets",
				::round(100 * assistRatio));
	else return;
	double uTrade = assistRatio * partnerUtilFromTrade();
	// Tech trade and military support are more sensitive to losses than trade
	double uOther = std::sqrt(assistRatio) *
			(partnerUtilFromTech() + partnerUtilFromMilitary());
	if(uTrade > 0 || uOther > 0)
		log("Utility for trade: %d, tech/military: %d, both weighted by saved assets",
				::round(uTrade), ::round(uOther));
	double uMinus = uTrade + uOther;
	if(!we->isHuman() && towardsThem >= ATTITUDE_FRIENDLY) {
		double uPureAffection = assistRatio * 20;
		log("Utility raised to %d for pure affection", ::round(uPureAffection));
		uMinus = std::min(uMinus, uPureAffection);
	}
	double OBUtil = 0;
	if(agent.isOpenBorders(TEAMID(theyId)))
		OBUtil = partnerUtilFromOB * assistRatio;
	if(uMinus < OBUtil && OBUtil > 0) {
		uMinus = OBUtil;
		log("Utility raised to %d for strategic value of OB", ::round(uMinus));
	}
	// DogPileWarRand also factors in through the military analysis.
	double personalityMult = weAI->protectiveInstinct();
	log("Personality multiplier: %d percent", ::round(100 * personalityMult));
	u -= ::round(uMinus * personalityMult);
}

double Assistance::assistanceRatio() {

	// Don't want to help them conquer cities; only look at what that they lose
	/*  Asset scores aren't entirely reliable, therefore, treat elimination
		separately. */
	if(m->isEliminated(theyId)) return 1;
	double theirScore = 0;
	double theirLostScore = lostAssetScore(NO_PLAYER, &theirScore);
	if(theirScore <= 0) return 0;
	double r = std::max(theirLostScore / theirScore, 0.0);
	FAssert(r <= 1);
	if(m->hasCapitulated(TEAMID(theyId))) {
		// Count capitulation as a 50% devaluation of their remaining assets
		r += 0.5 * (1 - r);
		log("%s capitulates", report.leaderName(theyId));
	}
	return r;
}

char const* Assistance::aspectName() const {

	// "Assistance" isn't distinct enough from Fidelity and SuckingUp
	return "Preservation of partners";
}
int Assistance::xmlId() const { return 6; }

Reconquista::Reconquista(WarEvalParameters& params)
	: WarUtilityAspect(params) {}

void Reconquista::evaluate() {

	set<int> conq = m->conqueredCities(weId);
	double uPlus = 0;
	int nProjectedCities = we->getNumCities() + (int)(conq.size() -
			m->lostCities(weId).size());
	double reconqValBase = std::max(5.0, 90.0 / (nProjectedCities + 2));
	for(iSetIt it = conq.begin(); it != conq.end(); it++) {
		if(m->lostCities(theyId).count(*it) <= 0)
			continue;
		City* cp = ourCache->lookupCity(*it);
		if(cp == NULL) continue; CvCity& c = *cp->city();
		if(!c.isEverOwned(weId))
			continue;
		/*  Lower the utility if our culture is small; suggests that we only
			held the city briefly or long ago */
		double reconqVal = std::sqrt(std::min(1.0,
				c.plot()->calculateCulturePercent(weId) / 50.0));
		uPlus += reconqValBase * reconqVal;
		log("Reconquering %s; base val %d, modifier %d percent", report.cityName(c),
				::round(reconqValBase), ::round(100 * reconqVal));
	}
	if(uPlus < 0.01)
		return;
	u += ::round(uPlus);
}

char const* Reconquista::aspectName() const { return "Reconquista"; }
int Reconquista::xmlId() const { return 7; }

Rebuke::Rebuke(WarEvalParameters& params)
	: WarUtilityAspect(params) {}

void Rebuke::evaluate() {

	int rebukeDiplo = -1 * we->AI_getMemoryAttitude(theyId, MEMORY_REJECTED_DEMAND);
	if(we->isHuman()) {
		rebukeDiplo = 0;
		/*  This would have humans care a little about rebukes; -100 like
			most leaders; however, humans don't normally follow up on rejected
			tribute demands. */
		/*rebukeDiplo = ::round(rebukeDiplo * -100.0 / GC.getLeaderHeadInfo(we->getLeaderType()).
				getMemoryAttitudePercent(MEMORY_REJECTED_DEMAND));
		rebukeDiplo *= -1 * we->AI_getMemoryCount(theyId, MEMORY_REJECTED_DEMAND) / 2;*/
	}
	if(rebukeDiplo <= 0)
		return;
	if(m->getCapitulationsAccepted(agentId).count(TEAMID(theyId)) > 0) {
		log("%s capitulates to us after rebuke", report.leaderName(theyId));
		/*  OK to count this multiple times for members of the same team; typically,
			only some civ pairs are going to have a rebuke memory. */
		u += 30;
		return;
	}
	double theirTotal = 0;
	double theirLoss = lostAssetScore(weId, &theirTotal);
	if(theirTotal <= 0) return;
	/*  Give their loss and our gain equal weight. We want to demonstrate that we
		can take by force what is denied to us (and more), and that those who
		deny us end up paying more than was asked. */
	double punishmentRatio = 0.5 * (theirLoss / theirTotal +
			conqAssetScore() / std::max(10.0, ourCache->totalAssetScore()));
	if(punishmentRatio > 0.001) {
		log("Punishment ratio: %d percent, rebuke diplo: %d",
				::round(100 * punishmentRatio), rebukeDiplo);
		u += ::round(std::min(30.0, std::sqrt((double)rebukeDiplo)
				* 100 * punishmentRatio));
	}
}

char const* Rebuke::aspectName() const { return "Rebuke"; }
int Rebuke::xmlId() const { return 8; }

Fidelity::Fidelity(WarEvalParameters& params)
	: WarUtilityAspect(params) {}

void Fidelity::evaluate() {

	/*  Don't care about our success here at all. If we declared war, we've proved
		that we don't tolerate attacks on our (declared) friends. */
	if(m->getWarsDeclaredBy(weId).count(theyId) <= 0)
		return;
	// Human doesn't really have friends, needs special treatment
	if(!we->isHuman() && we->AI_getMemoryCount(theyId, MEMORY_DECLARED_WAR_ON_FRIEND) <= 0)
		return;
	/*  Check if we still like the "friend" and if they're still at war.
		Unless attacked recently, we can't tell which civ was attacked, but that's
		just as well, let's not fuss about water under the bridge. */
	bool warOngoing = false;
	for(size_t i = 0; i < properCivs.size(); i++) {
		PlayerTypes civId = properCivs[i];
		if(TEAMREF(civId).AI_getWarPlan(TEAMID(theyId)) != WARPLAN_ATTACKED_RECENT ||
				TEAMREF(civId).isAtWar(agentId) ||
				!we->canContactAndTalk(civId))
			continue;
		// Assume that humans likes civs that like them back
		AttitudeTypes att = (we->isHuman() ? GET_PLAYER(civId).AI_getAttitude(weId) :
				we->AI_getAttitude(civId));
		/*  Pleased vs. Friendly: Distinguished under "Assistance". I want to be
			consistent with the relations penalty here, which is the same for
			Pleased and Friendly. */
		if(att >= ATTITUDE_PLEASED) {
			warOngoing = true;
			log("%s recently attacked our friend %s", report.leaderName(theyId),
					report.leaderName(civId));
			break;
		}
	}
	if(!warOngoing)
		return;
	/*  Only Gandhi applies a penalty of 2 for war on friend (-200% memory-attitude).
		Not sure if I want him to act as a peacekeeper ... Let's try it. */
	double leaderFactor = ::sqrt(-0.01 * GC.getLeaderHeadInfo(we->getPersonalityType()).
			getMemoryAttitudePercent(MEMORY_DECLARED_WAR_ON_FRIEND));
	if(we->isHuman())
		leaderFactor = 1;
	u += ::round(leaderFactor * 10);
}

char const* Fidelity::aspectName() const { return "Fidelity"; }
int Fidelity::xmlId() const { return 9; }

HiredHand::HiredHand(WarEvalParameters& params)
	: WarUtilityAspect(params) {}

void HiredHand::evaluate() {

	if(!params.isConsideringPeace() || !agent.isAtWar(TEAMID(theyId)) ||
			// Perhaps redundant(?)
			m->getWarsContinued(weId).count(theyId) <= 0 ||
			TEAMREF(theyId).isAVassal() || // AI can only be hired vs. master
			we->isHuman() || /*  Humans can't be hired for war and if they hire
								 someone, it doesn't mean the human keeps fighting */
			/*  If we start another war, we'll probably focus on the new
				war enemy, and withdraw our attention from the sponsored war.
				This isn't what a faithful ally does. */
			m->getWarsDeclaredBy(weId).size() > 0)
		return;
	PlayerTypes sponsor = ourCache->sponsorAgainst(TEAMID(theyId));
	int originalUtility = ourCache->sponsorshipAgainst(TEAMID(theyId));
	FAssert((sponsor == NO_PLAYER) == (originalUtility <= 0));
	double uPlus = 0;
	if(sponsor != NO_PLAYER && originalUtility > 0 && we->AI_getAttitude(sponsor) >=
			GC.getLeaderHeadInfo(we->getPersonalityType()).
			/*  Between Annoyed and Pleased; has to be strictly better to allow
				sponsorship. If it becomes strictly worse, we bail. */
			getDeclareWarRefuseAttitudeThreshold()) {
		log("We've been hired by %s; original utility of payment: %d",
				report.leaderName(sponsor), originalUtility);
		// Inclined to fight for 20 turns
		uPlus += eval(sponsor, originalUtility, 20);
		int deniedHelpDiplo = -1 * we->AI_getMemoryAttitude(sponsor,
				MEMORY_DENIED_JOIN_WAR);
		if(deniedHelpDiplo > 0) {
			log("Utility reduced b/c of denied help");
			uPlus /= std::sqrt((double)deniedHelpDiplo);
		}
	}
	// Have we hired someone to help us against theyId?
	for(size_t i = 0; i < properCivs.size(); i++) {
		CvPlayerAI const& ally = GET_PLAYER(properCivs[i]);
		/*  No point in checking if the ally is coming through. Need to allow
			some time for that, and we don't feel obliged for that long anyway.
			Or might say: We keep up the war just long enough to find out if
			our ally can make a difference. That's actually almost rational. */
		if(ally.warAndPeaceAI().getCache().sponsorAgainst(TEAMID(theyId)) == weId ||
				(we->AI_getMemoryCount(ally.getID(), MEMORY_ACCEPTED_JOIN_WAR) > 0 &&
				/*  Still can't be sure that the current war between the (human) ally
					and theyId is the war we've asked ally to declare, but that's OK. */
				they->AI_getMemoryCount(weId, MEMORY_HIRED_WAR_ALLY) > 0)) {
			log("We've hired %s for war against %s", report.leaderName(ally.getID()),
					report.leaderName(theyId));
			if(we->AI_getAttitude(ally.getID()) <= ATTITUDE_ANNOYED) {
				log("... but we don't like our hireling enough to care");
				continue;
			}
			/*  Behave as if someone had paid us the equivalent of 20 utility;
				feel obliged to fight along the ally for 10 turns.
				Or should it matter how much we've paid the ally? */
			uPlus += eval(ally.getID(), 20, 10);
		}
	}
	/*  Have we been at war since the start of the game? Then it's a scenario
		and we should try to play along for a while. Tbd.: Should be a
		separate aspect "Historical Role". */
	if(agent.AI_getAtWarCounter(TEAMID(theyId)) == GC.getGameINLINE().
			getElapsedGameTurns())
		uPlus += eval(NO_PLAYER, 50, 12);
	u += ::round(uPlus);
}

double HiredHand::eval(PlayerTypes allyId, int originalUtility, int obligationThresh) {

	// (These conditions overlap with those under Fidelity)
	if(allyId != NO_PLAYER && (!we->canContactAndTalk(allyId) ||
			agent.isAtWar(TEAMID(allyId)) ||
			agent.AI_getWorstEnemy() == TEAMID(allyId) ||
			TEAMREF(allyId).isAVassal())) { // Don't feel obliged to vassals
		log("We don't feel obliged to fight for %s", report.leaderName(allyId));
		return 0;
	}
	// Whoever has been hired must have the smaller AtWarCounter
	int turnsFought = std::min(agent.AI_getAtWarCounter(TEAMID(theyId)),
			allyId == NO_PLAYER ? INT_MAX :
			TEAMREF(allyId).AI_getAtWarCounter(TEAMID(theyId)));
	log("Hired team has fought %d turns against %s", turnsFought,
			report.leaderName(theyId));
	if(turnsFought >= obligationThresh) {
		log("Fought long enough");
		return 0;
	}
	double obligationRatio = (obligationThresh - ((double)turnsFought)) /
			obligationThresh;
	/*  Count the value of the sponsorship doubly initially to make relatively sure
		that we don't make peace right away due to some change in circumstance. */
	double uPlus = 2 * obligationRatio * obligationRatio * originalUtility;
	if(uPlus > 0.5)
		log("Utility for sticking with %s: %d", allyId == NO_PLAYER ?
				"our historical role" : report.leaderName(allyId),
				::round(uPlus));
	return uPlus;
}

char const* HiredHand::aspectName() const { return "Hired hand"; }
int HiredHand::xmlId() const { return 10; }

BorderDisputes::BorderDisputes(WarEvalParameters& params)
	: WarUtilityAspect(params) {}

void BorderDisputes::evaluate() {

	int diplo = -1 * we->AI_getCloseBordersAttitude(theyId);
	if(diplo <= 0)
		return;
	/*  Assume CloseBordersAttitudeChange of -4 for human instead of the value
		from XML (which is between -2 and -4). Humans don't like having their cities
		culture-pressed. */
	int cbac = GC.getLeaderHeadInfo(we->getPersonalityType()).
			getCloseBordersAttitudeChange();
	if(we->isHuman() && cbac != 0)
		diplo = ::round((-4.0 * diplo) / cbac);
	if(m->getCapitulationsAccepted(agentId).count(TEAMID(theyId)) > 0) {
		int uPlus = diplo * 8;
		log("They capitulate; shared-borders diplo%s: %d",
				(we->isHuman() ? " (human)" : ""), diplo);
		return;
	}
	double uPlus = 0;
	set<int> conq = m->lostCities(theyId);
	for(iSetIt it = conq.begin(); it != conq.end(); it++) {
		City* cp = ourCache->lookupCity(*it); if(cp == NULL) continue;
		CvCity const& c = *cp->city();
		int ourTileCulturePercent = c.plot()->calculateCulturePercent(weId);
		double newOwnerMultiplier = -1;
		for(size_t i = 0; i < properCivs.size(); i++) {
			PlayerTypes civId = properCivs[i];
			if(m->conqueredCities(civId).count(*it) > 0) {
				// Conquests by our vassals help us fully
				if(GET_PLAYER(civId).getMasterTeam() == agentId)
					newOwnerMultiplier = 1;
				// Third party: depends on how much culture they have
				else newOwnerMultiplier = std::max(0,
						75 - c.plot()->calculateCulturePercent(civId)) / 100.0;
				log("%s possible border city; our culture: %d percent",
						report.cityName(c), ourTileCulturePercent);
				/*  If we have very little culture there and we don't conquer it,
					assume that the city is far from our border */
				if(civId != weId && ourTileCulturePercent < 1) {
					newOwnerMultiplier = -1;
					log("Skipped b/c conquered by third party and our culture very"
							" small");
				}
				else if(c.plot()->getCulture(weId) <= 0) {
					newOwnerMultiplier = -1;
					log("Skipped b/c we have 0 culture there");
				}
				break;
			}
		}
		if(newOwnerMultiplier <= 0)
			continue;
		/*  E.g. ourCultureModifier=1 if 0% culture; 5% -> 0.92; 10% -> 0.67;
			20% -> 0.42; 50% -> 0.08.
			If we already have a lot of culture in the city, then it's probably
			not the city causing us to lose border tiles. */
		double ourCultureMultiplier = ourTileCulturePercent / 100.0;
		if(ourCultureMultiplier > 0.04)
			ourCultureMultiplier = std::max(0.0, 1 -
					0.25 * std::log(25 * ourCultureMultiplier) / std::log(2.0));
		else ourCultureMultiplier = 1;
		log("Multiplier for our rel. tile culture: %d percent",
				::round(100 * ourCultureMultiplier));
		log("Diplo from border dispute: %d", diplo);
		uPlus += 5 * newOwnerMultiplier * ourCultureMultiplier * diplo;
	}
	u += ::round(uPlus);
}

char const* BorderDisputes::aspectName() const { return "Border disputes"; }
int BorderDisputes::xmlId() const { return 11; }

SuckingUp::SuckingUp(WarEvalParameters& params)
	: WarUtilityAspect(params) {}

void SuckingUp::evaluate() {

	/*  Similar to Assistance and Fidelity, but this one is only about improving
		relations through the shared-war diplo bonus, whereas Assistance is about
		preventing a civ that is already a partner from being marginalized, and
		Fidelity is about discouraging future attacks on our friends. */
	CvGame& g = GC.getGameINLINE();
	/*  Sucking up to humans doesn't work, and unclear if relations with our
		enemies are going to matter. */
	if(they->isHuman() || m->isWar(weId, theyId) ||
			agent.AI_getWorstEnemy() == TEAMID(theyId) ||
			m->isEliminated(theyId) ||
			m->hasCapitulated(TEAMID(theyId)) ||
			// Don't bet on a loser
			m->lostCities(theyId).size() > m->conqueredCities(theyId).size() ||
			g.getPlayerScore(weId) * 6 > g.getPlayerScore(theyId) * 10 ||
			valTowardsUs <= -7 || // Hopeless case
			valTowardsUs >= 14) // No need to improve relations further
		return;
	/*  If they don't need the assist or if we don't actually fight, there'll
		be no diplo bonus. The 10*nCities threshold is very arbitrary. */
	if(m->lostPower(weId, ARMY) < 10 * we->getNumCities() ||
			m->lostPower(theyId, ARMY) < 10 * they->getNumCities())
		return;
	int sharedWars = 0;
	int ourWars = 0;
	for(size_t i = 0; i < properTeams.size(); i++) {
		TeamTypes tId = properTeams[i];
		if(GET_TEAM(tId).isAVassal() || tId == agentId || tId == TEAMID(theyId))
			continue;
		if(m->isWar(agentId, tId)) {
			ourWars++;
			if(m->isWar(TEAMID(theyId), tId))
				sharedWars++;
		}
	}
	// If we'll fight multiple wars, we may not be able to actually assist them
	if(sharedWars == 0 || ourWars > sharedWars)
		return;
	// Between 2 (Ashoka) and 6 (DeGaulle)
	int diplo = GC.getLeaderHeadInfo(they->getPersonalityType()).
			getShareWarAttitudeChangeLimit();
	double uPlus = 1.6 * diplo; // Should sharedWars have an impact?
	log("Sharing a war with %s; up to +%d diplo", report.leaderName(theyId), diplo);
	int nAlive = g.countCivPlayersAlive();
	if(towardsUs == ATTITUDE_PLEASED && we->AI_isDoVictoryStrategy(
			AI_VICTORY_DIPLOMACY3) && nAlive > 2) {
		log("Bonus utility for them being pleased and us close to diplo victory");
		uPlus += 5;
	}
	/*  Diplo bonus with just one civ less important in large games, but also in
		very small games or when there are few civs left. */
	u += ::round(uPlus / std::sqrt((double)std::min(4, nAlive)));
}

char const* SuckingUp::aspectName() const { return "Sucking up"; }
int SuckingUp::xmlId() const { return 12; }

PreEmptiveWar::PreEmptiveWar(WarEvalParameters& params)
	: WarUtilityBroaderAspect(params) {}

void PreEmptiveWar::evaluate() {

	/*  If an unfavorable war with them seems likely in the long run, we rather take
		our chances now. */
	/*  Don't worry about long-term threat if already in the endgame; Kingmaking
		handles this. */
	if(agent.AI_isAnyMemberDoVictoryStrategyLevel3() || m->isEliminated(weId) ||
			m->hasCapitulated(agent.getID()))
		return;
	double threat = ourCache->threatRating(theyId);
	/*  War evaluation should always assume minor threats; not worth addressing here
		explicitly. */
	if(threat < 0.15)
		return;
	// threatRating includes their vassals, so include vassals here as well
	double theirPresentCities = 0, theirPredictedCities = 0;
	for(size_t i = 0; i < properCivs.size(); i++) {
		CvPlayer const& civ = GET_PLAYER(properCivs[i]);
		double vassalFactor = 1;
		if(GET_TEAM(civ.getTeam()).isAVassal())
			vassalFactor = 0.5;
		if(civ.getMasterTeam() == GET_PLAYER(theyId).getMasterTeam() ||
				m->getCapitulationsAccepted(TEAMID(theyId)).
				count(civ.getTeam()) > 0) {
			if(civ.getMasterTeam() == GET_PLAYER(theyId).getMasterTeam())
				theirPresentCities += civ.getNumCities() * vassalFactor;
			theirPredictedCities += (civ.getNumCities() +
					m->conqueredCities(civ.getID()).size() -
					m->lostCities(civ.getID()).size()) * vassalFactor;
			/*  Ignore cities that their side gains from our human target
				(to avoid dogpiling on human) */
			TeamTypes targetId = params.targetId();
			if(targetId != NO_TEAM && targetId != theyId &&
					GET_TEAM(targetId).isHuman()) {
				for(iSetIt it = m->conqueredCities(civ.getID()).begin();
						it != m->conqueredCities(civ.getID()).end(); it++) {
					CvCity* c = WarAndPeaceCache::City::cityById(*it);
					if(c != NULL && c->getTeam() == targetId)
						theirPredictedCities--;
				}
				FAssert(theirPredictedCities >= 0);
			}
		}
	}
	double theirConqRatio = 1;
	if(theirPresentCities > 0)
		theirConqRatio = theirPredictedCities / theirPresentCities;
	/*  We assume that they are going to attack us some 50 turns from now,
		and they're going to choose the time that suits them best. Therefore,
		cities that they conquer are assumed to contribute to their power,
		whereas conquests by us or our vassals are assumed to be too recent to
		contribute. However, cities of new vassals don't need time to become
		productive. */
	double ourPredictedCities = agent.getNumCities() - m->lostCities(weId).size();
	double ourPresentCities = agent.getNumCities();
	for(size_t i = 0; i < properCivs.size(); i++) {
		CvPlayer const& civ = GET_PLAYER(properCivs[i]);
		if(m->getCapitulationsAccepted(agentId).count(civ.getTeam()) > 0)
			ourPredictedCities += 0.5 * (civ.getNumCities() -
					m->lostCities(civ.getID()).size());
		else if(GET_TEAM(civ.getTeam()).isVassal(agentId))
			ourPresentCities += 0.5 * (civ.getNumCities() -
					m->lostCities(civ.getID()).size());
	}
	double ourConqRatio = 1;
	if(ourPresentCities > 0)
		ourConqRatio = ourPredictedCities / ourPresentCities;
	// Assume that our gain is equally their loss
	double theirGain = ::dRange(theirConqRatio + (1 - ourConqRatio) - 1, -0.5, 0.5);
	if(std::abs(theirGain) < 0.0001) return;
	log("Long-term threat rating for %s: %d percent", report.leaderName(theyId),
			::round(threat * 100));
	log("Our cities (now/predicted) and theirs: %d/%d, %d/%d",
			::round(ourPresentCities), ::round(ourPredictedCities),
			::round(theirPresentCities), ::round(theirPredictedCities));
	log("Their gain in power: %d percent", ::round(theirGain * 100));
	// Shifts in power tend to affect the threat disproportionately
	double threatChange = sqrt((double)abs(theirGain));
	if(theirGain < 0) threatChange *= -1;
	log("Change in threat: %d percent", ::round(threatChange * 100));
	double uPlus = threat * -100 * theirGain;
	double distrustFactor = 1;
	if(!we->isHuman())
		distrustFactor = weAI->distrustRating();
	log("Our distrust: %d percent", ::round(distrustFactor * 100));
	u += ::round(uPlus * distrustFactor);
}

char const* PreEmptiveWar::aspectName() const { return "Pre-emptive War"; }
int PreEmptiveWar::xmlId() const { return 13; }

KingMaking::KingMaking(WarEvalParameters& params)
	: WarUtilityBroaderAspect(params) {

	// Need non-vassal civs repeatedly; store them in this->civs
	for(size_t i = 0; i < properCivs.size(); i++)
		if(!TEAMREF(properCivs[i]).isAVassal())
			civs.push_back(properCivs[i]);
}

 double const KingMaking::scoreMargin = 0.25;

int KingMaking::preEvaluate() {

	/*  Never too early to think about winning, but need to reduce the frequency
		of very early wars. */
	if(GC.getGameINLINE().getCurrentEra() <= GC.getGameINLINE().getStartEra() &&
			we->getCurrentEra() < 3)
		return 0;
	/*  Three classes of civs; all in the best non-empty category are likely
		winners in our book. */
	// I: Civs at victory stage 4
	for(size_t i = 0; i < civs.size(); i++) {
		CvPlayerAI const& civ = GET_PLAYER(properCivs[i]);
		// Tbd.: Merge with code in RiseFall::victoryStage
		if(civ.AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST4 | AI_VICTORY_DOMINATION4 |
				 AI_VICTORY_DIPLOMACY4 | AI_VICTORY_SPACE4) ||
				 /*  Check isDoStrategy first in order to avoid the costlier
					calculate... call */
				 (civ.AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4) &&
				  civ.AI_calculateCultureVictoryStage(55) >= 4))
			winning.insert(civ.getID());
	}
	if(!winning.empty())
		return 0;
	//  II: Civs at victory stage 3 or game score near the top
	for(size_t i = 0; i < civs.size(); i++)
		if(GET_PLAYER(properCivs[i]).AI_isDoVictoryStrategyLevel3())
			winning.insert(properCivs[i]);
	// III: Civs with a competitive game score
	/*  Important to use scores predicted based on military analysis because the
		AI needs to be able to respond quickly when a civ starts running away with
		the game; can't wait until rival conquests have actually happened. */
	addLeadingCivs(winning, scoreMargin);
	if(winning.count(weId) > 0 && winning.size() <= 1 &&
			// We don't want to be the only winner if it means betraying our partners
			(params.targetId() == NO_TEAM || agent.isAtWar(params.targetId()) ||
			agent.AI_isChosenWar(params.targetId()) || GC.getLeaderHeadInfo(we->getPersonalityType()).
			getNoWarAttitudeProb(agent.AI_getAttitude(params.targetId())) < 100)) {
		log("We'll be the only winners");
		return 45;
	}
	return 0;
}

void KingMaking::addLeadingCivs(set<PlayerTypes>& r, double margin,
		bool predictScore) const {

	CvGame& g = GC.getGameINLINE();
	double bestScore = 1;
	for(size_t i = 0; i < civs.size(); i++) {
		double sc = predictScore ? m->predictedGameScore(civs[i]) :
				g.getPlayerScore(civs[i]);
		/*  Count extra score for commerce so that civs that are getting far
			ahead in tech are identified as a threat */
		CvPlayerAI const& civ = GET_PLAYER(civs[i]);
		if(civ.getCurrentEra() >= 3 &&
				// The late game is covered by victory stages
				civ.getCurrentEra() <= 4 && g.getCurrentEra() <= 4) {
			double commerceRate = civ.estimateYieldRate(YIELD_COMMERCE);
			sc += commerceRate / 2;
		}
		if(sc > bestScore)
			bestScore = sc;
	}
	for(size_t i = 0; i < civs.size(); i++) {
		if((predictScore ? m->predictedGameScore(civs[i]) :
				g.getPlayerScore(civs[i])) / bestScore >= 0.75)
			r.insert(civs[i]);
	}
}

void KingMaking::evaluate() {

	if(winning.empty() || winning.count(theyId) <= 0)
		return;
	// Vassals are assumed to be out of contention
	if(TEAMREF(theyId).isAVassal())
		return;
	AttitudeTypes att = towardsThem;
	/*  As humans we are very much not OK with rivals winning the game,
		so ATTITUDE_FURIOUS would be the smarter assumption, however, I don't want
		a leading AI to be extremely alert about a human runner-up; could make it too
		difficult to catch up. */
	if(we->isHuman())
		att = ATTITUDE_ANNOYED;
	/*  We don't go as far as helping a friend win (only indirectly by trying to
		thwart the victory of a disliked civ) */
	if(att >= ATTITUDE_FRIENDLY)
		return;
	// NB: The two conditions above are superfluous; just for performance.
	double attitudeMultiplier = 0.03 + 0.25 * (ATTITUDE_PLEASED - att);
	// If we're human and did not believe we could win, we would've quit already.
	if(we->isHuman())
		winning.insert(weId);
	// We're less inclined to interfere if several rivals are in competition
	int winningRivals = winning.size();
	double caughtUpBonus = 0;
	set<PlayerTypes> presentWinners;
	addLeadingCivs(presentWinners, scoreMargin, false);
	double catchUpVal = 24.0 / std::pow((double)winning.size(), 0.75);
	catchUpVal *= std::pow((1 + weAI->amortizationMultiplier()) / 2, 2);
	if(winning.count(weId) > 0) {
		winningRivals--;
		/*  If we're among the likely winners, then it's a showdown between us
			and them, and we try to prevent their victory despite being Pleased.
			If we're out of contention (for the time being), we don't mind if
			they win at Pleased. */
		attitudeMultiplier += 0.25;
		if(presentWinners.count(weId) == 0) {
			/*  We're not presently winning, but our predicted conquests bring us
				back in competition.
				Note that this code block isn't reached if we're Friendly
				with them. */
			caughtUpBonus += catchUpVal;
			log("%d for catching up with %s", ::round(caughtUpBonus),
					report.leaderName(theyId));
		}
	}
	else if(presentWinners.count(weId) > 0) {
		caughtUpBonus -= catchUpVal;
		log("%d for falling behind %s", ::round(caughtUpBonus),
				report.leaderName(theyId));
	}
	if(attitudeMultiplier <= 0) {
		u += ::round(caughtUpBonus);
		return;
	}
	CvCity* cap = they->getCapitalCity();
	double uPlus = 0;
	/*  Foiled Space Race; other anti-victory conditions would be more elaborate to
		implement (even culture). */
	if(they->AI_isDoVictoryStrategy(AI_VICTORY_SPACE4) && cap != NULL) {
		if(m->lostCities(theyId).count(cap->plotNum()) > 0)
			uPlus = 100;
		else uPlus = -70;
		log("%d for Space victory", ::round(uPlus));
	}
	/*  If they (or their vassals) make a net asset gain, we incur a cost;
		if they make a net asset loss, that's our war gain. */
	else {
		uPlus = theirRelativeLoss();
		if(abs(uPlus) < 0.01) { // Don't pollute the log then
			u += ::round(caughtUpBonus);
			return;
		}
		double weight = 5; // So that 25% loss correspond to 125 utility
		// We're a bit less worried about helping them indirectly
		if(uPlus < 0) weight = 3.5;
		uPlus *= 100 * weight;
		log("%d base utility for change in asset ratio", ::round(uPlus));
	}
	/*  winningRivals seems too big a divisor when there are just 2 or 3
		competitors */
	double div = 1 + std::pow(winningRivals / 3.0, 2.0);
	double competitionMultiplier = 1.0 / div;
	// It doesn't get more competitive than this:
	if(winningRivals == 1) competitionMultiplier = 1;
	log("Winning civs: %d (%d rivals)", ::round(winning.size()), ::round(winningRivals));
	uPlus = ::dRange(uPlus, -100.0, 100.0);
	log("Attitude multiplier: %d percent, competition multiplier: %d percent",
			::round(100 * attitudeMultiplier), ::round(100 * competitionMultiplier));
	uPlus *= attitudeMultiplier * competitionMultiplier;
	if(uPlus < 0.5 && uPlus > -0.5) // Log confusing to read w/o this
		log("(Kingmaking gain from %s negligibly small)", report.leaderName(theyId));
	u += ::round(uPlus + caughtUpBonus);
}

double KingMaking::theirRelativeLoss() {

	if(!m->isPartOfAnalysis(theyId)) // To save time
		return 0;
	bool humanTarget = false;
	// Ignore assets that they gain from human target (to avoid dogpiling on human)
	TeamTypes ignoreGains = params.targetId();
	if(ignoreGains != NO_TEAM && !GET_TEAM(ignoreGains).isHuman())
		ignoreGains = NO_TEAM;
	double theirLosses = 0; double theirAssets = 0;
	for(size_t i = 0; i < properCivs.size(); i++) {
		CvTeam const& t = TEAMREF(properCivs[i]);
		double vassalFactor = 1;
		if(t.isAVassal()) vassalFactor *= 0.5;
		double assets = 0;
		if(t.getMasterTeam() != TEAMID(theyId)) {
			if(m->getCapitulationsAccepted(TEAMID(theyId)).count(t.getID()) > 0) {
				lostAssetScore(NO_PLAYER, &assets);
				double gain = vassalFactor * cityRatio(properCivs[i]) * assets;
				log("Gained from new vassal: %d", ::round(gain));
				theirLosses -= gain;
			}
			continue;
		}
		// can be negative
		double losses = lostAssetScore(NO_PLAYER, &assets, ignoreGains);
		theirAssets += vassalFactor * assets;
		theirLosses += vassalFactor * losses;
	}
	if(theirAssets <= 0) {
		// They haven't founded a city yet; can happen in late-era start
		return 0;
	}
	return theirLosses / theirAssets;
}

char const* KingMaking::aspectName() const { return "Kingmaking"; }
int KingMaking::xmlId() const { return 14; }

Effort::Effort(WarEvalParameters& params)
	: WarUtilityAspect(params) {}

int Effort::preEvaluate() {

	/*  Not nice to put all code into this supposedly preparatory function.
		For most aspects, it makes sense to assign utility to each war opponent
		separately (WarUtilityAspect::evaluate(0)), but, for this aspect, the
		simulation doesn't track against whom we lose units, and our military
		build-up isn't directed against any opponent in particular. */
	double uMinus = 0;
	// For civic and research changes at wartime:
	if(!m->getWarsContinued(weId).empty() || !m->getWarsDeclaredBy(weId).empty()) {
		bool allWarsLongDist = true;
		bool allPushOver = true;
		for(set<PlayerTypes>::const_iterator it = m->getWarsContinued(weId).begin();
				it != m->getWarsContinued(weId).end(); it++) {
			if(we->AI_hasSharedPrimaryArea(*it))
				allWarsLongDist = false;
			if(!agent.warAndPeaceAI().isPushover(TEAMID(*it)))
				allPushOver = false;
		}
		for(set<PlayerTypes>::const_iterator it = m->getWarsDeclaredBy(weId).begin();
				it != m->getWarsDeclaredBy(weId).end(); it++) {
			if(we->AI_hasSharedPrimaryArea(*it))
				allWarsLongDist = false;
			if(!agent.warAndPeaceAI().isPushover(TEAMID(*it)))
				allPushOver = false;
		}
		if(allPushOver) {
			/*  Can't be sure that this won't lead to a change in civics (though
				it shouldn't); therefore not 0 cost. */
			uMinus += 2;
			log("All targets are short work; only %d for wartime economy",
					::round(uMinus));
		}
		else {
			/*  Reduced cost for long-distance war; less disturbance of Workers
				and Settlers, and less danger of pillaging */
			uMinus += m->turnsSimulated() / ((allWarsLongDist ? 8.0 : 5.5) +
					// Workers not much of a concern later on
					we->getCurrentEra() / 2);
			log("Cost for wartime economy and ravages: %d%s", ::round(uMinus),
					(allWarsLongDist ? " (reduced b/c of distance)" : ""));
		}
	}
	double g = ourCache->goldValueOfProduction();
	log("1 production valued as %.2f gold", g);
	int d = agent.AI_getAtWarCounter(params.targetId()); // rather use the max over all wars?
	/*  How powerful are we relative to our rivals at the end of the simulation?
		Simpler to use the BtS power ratio here than to go through all military
		branches. Our rival's per-branch power isn't public info either.
		The simulation yields per-branch losses though. Use relative losses
		in armies to predict the BtS power ratios. */
	double ourPowerChange = m->gainedPower(weId, ARMY) /
			ourCache->getPowerValues()[ARMY]->power();
	double ourPower = we->getPower() * (1 + ourPowerChange);
	double highestRivalPower = 0;
	for(size_t i = 0; i < properCivs.size(); i++) {
		CvPlayerAI const& civ = GET_PLAYER(properCivs[i]);
		if(civ.getMasterTeam() == agentId || !agent.isHasMet(civ.getTeam()))
			continue;
		double theirPowerChange = m->gainedPower(properCivs[i], ARMY);
		/*  If they're not part of the military analysis, estimate their power based
			on BtS statistics. */
		if(theirPowerChange == 0)
			theirPowerChange = weAI->estimateBuildUpRate(properCivs[i]);
		else theirPowerChange /= GET_PLAYER(properCivs[i]).warAndPeaceAI().getCache().
				getPowerValues()[ARMY]->power();
		double theirPower = civ.getPower() * (1 + theirPowerChange);
		if(we->isHuman())
			theirPower *= civ.warAndPeaceAI().confidenceAgainstHuman();
		else if(civ.isHuman())
			theirPower *= 1 / weAI->confidenceAgainstHuman();
		highestRivalPower = std::max(highestRivalPower, theirPower);
	}
	// Express our losses in terms of production
	double lostProduction = 0;
	double lostUnits = 0;
	for(int i = 0; i < NUM_BRANCHES; i++) {
		MilitaryBranchTypes mb = (MilitaryBranchTypes)i;
		if(mb == CAVALRY || mb == LOGISTICS) // Included in other branches
			continue;
		UnitTypes ut = ourCache->getPowerValues()[mb]->getTypicalUnitType();
		if(ut == NO_UNIT) continue;
		CvUnitInfo const& u = GC.getUnitInfo(ut);
		double lost = m->lostPower(weId, mb);
		/*  Really shouldn't be negative, but sometimes is minus 0.0-something;
			probably not a bug worth chasing. */
		FAssert(lost >= -1); if(!(lost>=0)) continue;
		double typicalPow = weAI->militaryPower(u, u.getPowerValue());
		if(typicalPow <= 0) continue;
		double typicalCost = we->getProductionNeeded(ut);
		double n = lost / typicalPow;
		lostUnits += n;
		lostProduction += typicalCost * n;
	}
	/*  How useful are units that survive the war going forward?
		If we'll be far more powerful than our most dangerous rival, we probably
		have more units than we need.
		Some of the lost units may also be outdated chaff that we would have to
		upgrade soon if we didn't lose them. */
	double futureUse = ::dRange(highestRivalPower / (ourPower + 0.01), 0.35, 1.65) /
			2.6; // Division by 2.2 means survivors can be valued up to 75%; 2.75: 60%
	if(!we->isHuman())
		futureUse = std::pow(futureUse, 0.75); // sqrt would be a bit much
	/*  If we're at peace, units trained are apparently deemed useful by CvCityAI;
		we still shouldn't assume that they'll _certainly_ be useful. */
	/*bool anyWar = false;
	for(size_t i = 0; i < properCivs.size(); i++)
		if(m->isWar(weId, properCivs[i]))
			anyWar = true;
	if(!anyWar) futureUse = 1;*/
	double invested = m->militaryProduction(weId);
	double totalLostProduction = lostProduction * futureUse + invested * (1 - futureUse);
	log("Production value of lost units: %d, invested production: %d,"
			" multiplier for future use of trained units: %d percent, "
			"adjusted production value of build-up and losses: %d",
			::round(lostProduction), :: round(invested), ::round(100 * futureUse),
			::round(totalLostProduction));
	double supply = 0; // And unit cost I guess (not a big factor)
	// None if we're losing
	if(m->lostCities(weId).empty() && lostUnits > 0) { 
		/*  Use lostUnits as a measure of the number of active fighters.
			No need to apply the handicap modifier; humans can avoid supply much
			better than AI; this cancels out.
			Times 0.5 because some of the simulated turns may be preparation,
			and even fighting units aren't constantly in foreign borders. */
		supply = m->turnsSimulated() * 0.5 * lostUnits;
		if(m->conqueredCities(weId).empty()) // If we aren't on the offensive
			supply /= 2;
		log("Estimated gold for supply: %d (%d turns simulated, %d units lost)",
				::round(supply), ::round(m->turnsSimulated()), ::round(lostUnits));
		supply *= weAI->amortizationMultiplier();
	}
	double const vagueOpportunityWeight = 2.3; // pretty arbitrary
	int const giveWarAChanceDuration = 15;
	/*  Convert the lost production into gold based on current opportunities for
		peaceful development. The conversion rate is precomputed by
		WarAndPeaceCache. */
	int extraDuration = d - giveWarAChanceDuration;
	if(extraDuration > 0) {
		double gameSpeedDiv = GC.getGameSpeedInfo(
				GC.getGameINLINE().getGameSpeedType()).getResearchPercent() / 100.0;
		g = std::min(5.0, g * (1 + vagueOpportunityWeight * 0.025 *
				std::min(extraDuration, 40) / gameSpeedDiv));
		log("Gold per production adjusted to %.2f based on war duration (%d turns)",
				g, d);
	}
	g = supply + g * totalLostProduction;
	log("Gold value of build-up and war effort: %d", ::round(g));
	uMinus += weAI->tradeValToUtility(g);
	/*  Nukes are included in army, and therefore already covered by the costs above.
		But these don't take into account that nukes are always lost when used.
		Therefore add some extra cost. */
	double nukeCost = 0;
	double fired = m->getNukesFiredBy(weId);
	UnitTypes ut = ourCache->getPowerValues()[NUCLEAR]->getTypicalUnitType();
	if(fired > 0.01 && ut != NO_UNIT) {
		/*  Not clear that we have to replace the fired nukes.
			Reduce the lost production to 0.3 for that reason,
			and b/c already partially covered by army losses. */
		double lostProduction = 0.3 * fired * we->getProductionNeeded(ut);
		double prodVal = ourCache->goldValueOfProduction();
		nukeCost = weAI->tradeValToUtility(prodVal * lostProduction);
		log("Cost for fired nukes: %d; gold val per prod: %.2f; lost prod: %d",
				::round(nukeCost), prodVal, ::round(lostProduction));
		uMinus += nukeCost;
	}
	return -std::min(200, ::round(uMinus));
}

void Effort::evaluate() {}

char const* Effort::aspectName() const { return "Effort"; }
int Effort::xmlId() const { return 15; }

Risk::Risk(WarEvalParameters& params) : WarUtilityAspect(params) {}

int Risk::preEvaluate() {

	// Handle potential losses of our vassals here
	double uMinus = 0;
	for(size_t i = 0; i < properCivs.size(); i++) {
		PlayerTypes const vId = properCivs[i];
		CvPlayerAI const& vassal = GET_PLAYER(vId);
		if(TEAMID(vId) == TEAMID(weId) || vassal.getMasterTeam() != agentId)
			continue;
		// OK to peek into our vassal's cache
		WarAndPeaceCache const& vassalCache = vassal.warAndPeaceAI().getCache();
		double relativeVassalLoss = 1;
		if(!m->isEliminated(vId)) {
			double lostVassalScore = 0;
			for(iSetIt it = m->lostCities(vId).begin();
					it != m->lostCities(vId).end(); it++) {
				City* c = vassalCache.lookupCity(*it);
				if(c == NULL) continue;
				lostVassalScore += c->getAssetScore();
			}
			relativeVassalLoss = lostVassalScore /
					(vassalCache.totalAssetScore() + 0.01);
		}
		double vassalCost = 0;
		if(relativeVassalLoss > 0) {
			vassalCost = relativeVassalLoss * 33;
			log("Cost for losses of vassal %s: %d", report.leaderName(vId),
					::round(vassalCost));
		}
		if(!TEAMREF(vId).isCapitulated()) {
			double relativePow = (m->gainedPower(vId, ARMY) +
					vassalCache.getPowerValues()[ARMY]->power()) /
					(m->gainedPower(weId, ARMY) +
					ourCache->getPowerValues()[ARMY]->power()) - 0.9;
			if(relativePow > 0) {
				double breakAwayCost = std::min(20.0, std::sqrt(relativePow) * 40);
				if(breakAwayCost > 0.5)
					log("Cost for %s breaking free: %d", report.leaderName(vId),
							::round(breakAwayCost));
				vassalCost += breakAwayCost;
			}
		}
		uMinus += vassalCost;
	}
	return ::round(uMinus);
}

void Risk::evaluate() {

	double lostAssets = 0;
	for(iSetIt it = m->lostCities(weId).begin(); it != m->lostCities(weId).end(); it++) {
		/*  It doesn't matter here whom the city is lost to, but looking at
			one enemy at a time produces better debug output. */
		if(m->conqueredCities(theyId).count(*it) <= 0)
			continue;
		City* c = ourCache->lookupCity(*it);
		if(c == NULL) continue;
		double sc = c->getAssetScore();
		log("%s: %d lost assets", report.cityName(*c->city()), ::round(sc));
		lostAssets += sc;
	}
	double uMinus = 400 * lostAssets;
	double total = ourCache->totalAssetScore() + 0.01;
	double fromBlockade = lossesFromBlockade(weId, theyId) * 0.1 *
			(total - lostAssets);
	uMinus += fromBlockade;
	double fromNukes = 400 * lossesFromNukes(weId, theyId);
	double const scareCost = 28;
	if(they->getNumNukeUnits() > 0 && m->getWarsDeclaredBy(weId).count(theyId) > 0 &&
			TEAMREF(theyId).getAtWarCount() <= 0 && fromNukes <= scareCost) {
		log("Nuke cost raised to %d for fear", ::round(scareCost));
		fromNukes = scareCost;
	}
	uMinus += fromNukes;
	/*  advc.035 (comment): Don't consider lossFromFlippedTiles here. We might
	capture the very cities that steal our tiles by continuing the war. */
	uMinus /= total;
	if(uMinus > 0.5) {
		if(fromBlockade > 0.5)
			log("From naval blockade: %d", ::round(fromBlockade));
		if(fromNukes > 0.5)
			log("From nukes: %d", ::round(fromNukes));
		log("Cost for lost assets: %d (loss: %d, present: %d)", ::round(uMinus),
				::round(lostAssets), ::round(total));
	}
	if(m->getCapitulationsAccepted(TEAMID(theyId)).count(agentId) > 0) {
		/*  Counting lost cities in addition to capitulation might make us too
			afraid of decisively lost wars. Then again, perhaps one can never be too
			afraid of those? */
		uMinus /= 2;
		int const costForCapitulation = 100;
		uMinus += costForCapitulation;
		log("Cost halved because of capitulation; %d added", costForCapitulation);
	}
	u -= ::round(uMinus);
}

char const* Risk::aspectName() const { return "Risk"; }
int Risk::xmlId() const { return 16; }

IllWill::IllWill(WarEvalParameters& params) : WarUtilityBroaderAspect(params) {}

int IllWill::preEvaluate() {

	int hostiles = 0;
	for(size_t i = 0; i < properCivs.size(); i++)
		if(properCivs[i] != weId && (m->isWar(weId, properCivs[i]) ||
				GET_PLAYER(properCivs[i]).AI_getAttitude(weId) <= ATTITUDE_ANNOYED))
			hostiles++;
	int partners = numKnownRivals - hostiles;
	altPartnerFactor = std::sqrt(1.0 / std::max(1, partners - 3));
	return 0;
}

void IllWill::evaluate() {

	uMinus = 0; // Have the subroutines use this instead of u to avoid rounding
	// We can't trade with our war enemies
	evalLostPartner();
	bool endNigh = agent.AI_isAnyMemberDoVictoryStrategyLevel4() ||
			TEAMREF(theyId).AI_isAnyMemberDoVictoryStrategyLevel4();
	// Reasons we might not care about diplo effects
	if(!agent.isAVassal() && !TEAMREF(theyId).isAVassal() &&
			!m->hasCapitulated(agentId) && !m->isEliminated(weId) &&
			!m->hasCapitulated(TEAMID(theyId)) && !m->isEliminated(theyId) &&
			towardsUs > ATTITUDE_FURIOUS && // Lost cause
			towardsThem > ATTITUDE_FURIOUS) { // We don't even want to appease them
		if(!endNigh) {
			// Ill will from war makes them more likely to attack us in the future
			evalRevenge();
		}
		if(!endNigh || we->AI_isDoVictoryStrategy(AI_VICTORY_DIPLOMACY4) ||
				they->AI_isDoVictoryStrategy(AI_VICTORY_DIPLOMACY4)) {
			// Civs (our potential partners) don't like when we attack their (other) partners
			evalAngeredPartners();
		}
	}
	double nc = 0;
	// Fired but intercepted nukes don't cause bad diplo
	double nukes = m->getNukedCities(weId, theyId);
	if(nukes > 0.01)
		nc = nukeCost(nukes);
	if(nc > 0.5)
		log("Diplo cost for nukes: %d", ::round(nc));
	uMinus += nc;
	u -= ::round(uMinus);
}

double IllWill::nukeCost(double nukes) {

	if(they->isHuman() || valTowardsUs > 12 || towardsUs <= ATTITUDE_FURIOUS ||
			towardsThem <= ATTITUDE_FURIOUS)
		return 0;
	double r = 0;
	for(size_t i = 0; i < properCivs.size(); i++) {
		CvPlayerAI const& civ = GET_PLAYER(properCivs[i]);
		if(civ.getTeam() == agentId || civ.getTeam() == TEAMID(theyId) ||
				civ.isHuman() || GET_TEAM(civ.getTeam()).isAVassal() ||
				m->isWar(weId, civ.getID()) || civ.AI_getAttitudeVal(weId) > 12 ||
				civ.AI_getAttitude(theyId) < ATTITUDE_PLEASED)
			continue;
		double attFactor = 3;
		/*  If they like us too, we don't necessarily get a penalty due to change
			130q. Small factor anyway since, this late in the game, we mostly care
			about other civs as war allies or enemies. */
		if(civ.AI_getAttitude(weId) >= ATTITUDE_PLEASED)
			attFactor /= 2;
		log("Diplo penalty from %s: %d times %.2f", report.leaderName(civ.getID()),
				::round(attFactor), nukes);
		r += attFactor * nukes;
	}
	return std::min(r, 40.0);
}

void IllWill::evalLostPartner() {

	if(numRivals < 2) // Zero-sum game then, and our loss is equally their loss.
		return;
	// Only master attitude matters for capitulated vassals (b/c of change 130v)
	bool theyCapit = TEAMREF(theyId).isAVassal() || m->hasCapitulated(TEAMID(theyId));
	double diploFactor = 0.7;
	if(!they->isHuman()) {
		if(!theyCapit && (towardsUs <= ATTITUDE_FURIOUS ||
				towardsThem <= ATTITUDE_FURIOUS || // Takes two to tango
				// If not Furious now, this may do it:
				m->getWarsDeclaredBy(weId).count(theyId) > 0))
			diploFactor += 0.3;
		// Rare as this may be ...
		if(towardsThem >= ATTITUDE_CAUTIOUS)
			diploFactor -= 0.15;
		if(towardsUs >= ATTITUDE_CAUTIOUS)
			diploFactor -= 0.15;
	}
	/*  Humans are forgiving and tend to end wars quickly. This special treatment
		should lead to slightly greater willingness to declare war against human
		(handled at the end of this function), but also slightly greater 
		willingness to make peace again. */
	else diploFactor -= 0.15;
	// Don't expect much from reconciling with minor players
	if(m->isEliminated(theyId) || theyCapit)
		diploFactor = 2;
	if(!m->isWar(weId, theyId)) {
		if(agent.isAtWar(TEAMID(theyId)) && !m->isEliminated(theyId)) {
			// Reconciliation is a long shot for the AI
			int reconciliationUtility = ::round(4 * (altPartnerFactor / diploFactor)
					* weAI->amortizationMultiplier());	
			uMinus -= reconciliationUtility;
			log("%d for the possibility of reconciliation", reconciliationUtility);
		}
		return;
	}
	if(agent.isAtWar(TEAMID(theyId))) // Ongoing war handled above
		return;
	double partnerUtil = partnerUtilFromTrade() +
			/*  Don't expect to trade anything with a capitulated vassal that the
				master wouldn't trade */
			(theyCapit ? 0 : partnerUtilFromTech()) +
			partnerUtilFromMilitary();
	// Halved because, for now, can still enter their borders by force
	if(agent.isOpenBorders(TEAMID(theyId)))
		partnerUtil += partnerUtilFromOB / (theyCapit ? 4 : 2);
	log("Base partner utility from %s: %d", report.leaderName(theyId),
			::round(partnerUtil));
	log("Modifier for alt. partners: %d percent", ::round(100 * altPartnerFactor));
	double uMinusTmp = partnerUtil * altPartnerFactor;
	// They may help us again in the future (if we don't go to war with them)
	if(we->AI_getMemoryCount(theyId, MEMORY_GIVE_HELP) > 0) {
		double giftUtility = 5 * weAI->amortizationMultiplier();
		log("%d for given help", ::round(giftUtility));
		uMinusTmp += giftUtility;
	}
	double const humanMult = 0.81;
	if(they->isHuman()) {
		log("Lost-partner cost reduced b/c humans are forgiving");
		uMinusTmp *= humanMult;
	}
	if(we->isHuman())
		uMinusTmp *= humanMult;
	uMinus += uMinusTmp;
}

void IllWill::evalRevenge() {

	/*  Only care about the case where relations are worsened by our DoW.
		Otherwise, PreEmptiveWar handles it. */
	if(agent.isAtWar(TEAMID(theyId)) || !m->isWar(weId, theyId) ||
			// Looks like they won't be a threat for long
			m->lostCities(theyId).size() > 1)
		return;
	double ourPow = ourCache->getPowerValues()[ARMY]->power() +
			// (Included in ARMY)
			//ourCache->getPowerValues()[NUCLEAR]->power() +
			m->gainedPower(weId, ARMY);
	if(they->isHuman())
		ourPow *= weAI->confidenceAgainstHuman();
	/*  Don't count vassals; they're not a reliable defense (and it's easier
		to exclude them). */
	WarAndPeaceCache* theirCache = &theyAI->getCache();
	double theirPow = theirCache->getPowerValues()[ARMY]->power() +
			//theirCache->getPowerValues()[NUCLEAR]->power() +
			m->gainedPower(theyId, ARMY);
	if(we->isHuman())
		theirPow *= theyAI->confidenceAgainstHuman();
	double powRatio = theirPow / std::max(10.0, ourPow);
	log("Power ratio they:us after military analysis: %d percent",
			::round(100 * powRatio));
	double revengeCost = 0;
	/*  Even if they're not a major threat, they may well cause us some inconvenience,
		in particular, as part of a coalition. */
	if(powRatio > 0.7) revengeCost += 4;
	powRatio = ::dRange(powRatio - 1, 0.0, 1.0);
	double attitudeMultiplier = 1;
	if(towardsUs == ATTITUDE_ANNOYED)
		attitudeMultiplier = 0.5;
	else if(towardsUs == ATTITUDE_CAUTIOUS)
		attitudeMultiplier = 0.85;
	revengeCost += powRatio * (they->isHuman() ? 24 : 28) * attitudeMultiplier;
	if(revengeCost > 0.5)
		log("Cost for possible revenge: %d (attitude mult.: %d percent)",
				::round(revengeCost), ::round(attitudeMultiplier * 100));
	uMinus += revengeCost;
}

void IllWill::evalAngeredPartners() {

	if(they->isHuman() || m->isWar(weId, theyId) ||
			valTowardsUs > 12 || // Nothing can alienate them
			they->getTeam() == agentId ||
			towardsThem <= ATTITUDE_FURIOUS) // We don't mind angering them
		return;
	// 2 only for Gandhi; else 1
	int penaltyPerDoW = ::round(GC.getLeaderHeadInfo(they->getPersonalityType()).
			getMemoryAttitudePercent(MEMORY_DECLARED_WAR_ON_FRIEND) / -100);
	int penalties = 0;
	for(set<PlayerTypes>::const_iterator it = m->getWarsDeclaredBy(weId).begin();
			it != m->getWarsDeclaredBy(weId).end(); it++) {
		if(TEAMREF(*it).isAVassal() || they->getTeam() == TEAMID(*it))
			continue;
		if(they->AI_getAttitude(*it) >= ATTITUDE_PLEASED) {
			log("-%d relations with %s for DoW on %s", penaltyPerDoW,
					report.leaderName(theyId), report.leaderName(*it));
			penalties += penaltyPerDoW;
		}
	}
	if(penalties <= 0)
		return;
	double costPerPenalty = (partnerUtilFromTrade() + partnerUtilFromTech() +
			partnerUtilFromMilitary() + (agent.isOpenBorders(TEAMID(theyId)) ?
			partnerUtilFromOB : 0)) /
			/*  We lose only a little bit of goodwill, but we're also exposing
				ourselves to a dogpile war. Menace handles that, but doesn't factor
				in the diplo penalties from our DoW. */
			((they->AI_getAttitudeFromValue(valTowardsUs - penalties) <=
			ATTITUDE_CAUTIOUS ? 3.5 : 5) *
			/*  Don't worry quite as much about diplo in team games. AI DoW
				aren't as dynamic, and sufficient for tech trading if some
				team members get along. */
			std::sqrt((double)agent.getNumMembers()));
	log("Cost per -1 relations: %d", ::round(costPerPenalty));
	/*  costPerPenalty already adjusted to game progress, but want to dilute
		the impact of leader personality in addition to that. diploWeight is
		mostly about trading, and trading becomes less relevant in the lategame. */
	double diploWeight = 1 + (weAI->diploWeight() - 1) *
			weAI->amortizationMultiplier();
	/*  We've actually one partner less b/c we're considering alternatives to a
		partner that (probably) isn't counted by preEvaluate as hostile. */
	uMinus += costPerPenalty * penalties * diploWeight * altPartnerFactor;
	log("Diplo weight: %d percent, alt.-partner factor: %d percent",
			::round(100 * diploWeight), ::round(100 * altPartnerFactor));
	if(towardsUs >= ATTITUDE_PLEASED && we->AI_isDoVictoryStrategy(
			AI_VICTORY_DIPLOMACY4 | AI_VICTORY_DIPLOMACY3)) {
		double victoryFactor = they->getTotalPopulation() /
				(double)std::max(1, GC.getGameINLINE().getTotalPopulation());
		double victoryCost = 25 * victoryFactor * penalties;
		log("-%d for jeopardizing diplo victory", ::round(victoryCost));
		uMinus += victoryCost;
	}
}

char const* IllWill::aspectName() const { return "Ill Will"; }
int IllWill::xmlId() const { return 17; }

Affection::Affection(WarEvalParameters& params) : WarUtilityAspect(params) {}

void Affection::evaluate() {

	// Humans are unaffectionate.	It's OK to liberate capitulated vassals.
	if(we->isHuman() || agent.isCapitulated() || TEAMREF(theyId).isCapitulated() ||
			!m->isWar(weId, theyId) || agent.AI_getWorstEnemy() == TEAMID(theyId))
		return;
	WarPlanTypes wp = agent.AI_getWarPlan(TEAMID(theyId));
	// Once we've adopted a direct warplan, affection can't stay our hand
	if(wp != WARPLAN_PREPARING_LIMITED && wp != WARPLAN_PREPARING_TOTAL &&
			wp != NO_WARPLAN)
		return;
	// We're not fully responsible for wars triggered by our DoW
	double linkedWarFactor = 0;
	if(m->getWarsDeclaredBy(weId).count(theyId) > 0) {
		if(TEAMREF(theyId).isAVassal()) // non-capitulated
			linkedWarFactor = 0.25;
		else linkedWarFactor = 1;
	}
	/*  We're also somewhat responsible for wars declared on us through
		defensive pacts (DP). No need to check for current DP b/c a DP is
		the only way that a DoW on us can happen in the military analysis
		(all other DoW on us are unforeseen). */
	if(m->getWarsDeclaredBy(theyId).count(weId) > 0)
		linkedWarFactor = 0.4;
	if(linkedWarFactor <= 0)
		return;
	CvLeaderHeadInfo& lh = GC.getLeaderHeadInfo(we->getPersonalityType());
	int noWarPercent = lh.getNoWarAttitudeProb(towardsThem);
	/*  Capitulated vassals don't interest us, but a voluntary vassal can reduce
		our affection for its master. This function also gets called with theyId
		set to the vassal, but that call only accounts for our scruples about
		attacking the vassal.
		There's already a diplo penalty for having an unpopular vassal, so
		I'm only applying a minor penalty here. */
	if(noWarPercent > 0) { // for efficiency
		double vassalPenalty = 0;
		for(size_t i = 0; i < properCivs.size(); i++) {
			CvTeam const& t = TEAMREF(properCivs[i]);
			if(t.isCapitulated() || !t.isVassal(TEAMID(theyId)))
				continue;
			int vassalNoWarPercent = lh.getNoWarAttitudeProb(
					we->AI_getAttitude(properCivs[i]));
			int delta = noWarPercent - vassalNoWarPercent;
			if(delta > 0)
				vassalPenalty += delta / 10.0;
		}
		int iVassalPenalty = ::round(vassalPenalty);
		if(iVassalPenalty > 0) {
			noWarPercent = std::max(noWarPercent / 2, noWarPercent - iVassalPenalty);
			log("No-war-chance for %s reduced by %d b/c of peace vassals",
					report.leaderName(theyId), iVassalPenalty);
		}
	}
	double pr = noWarPercent / 100.0;
	double uMinus = std::pow(pr, 5.5) * 80;
	if(noWarPercent >= 100) {
		uMinus += 5;
		if(towardsThem >= ATTITUDE_FRIENDLY)
			uMinus += 45;
	}
	bool const ignDistr = params.isIgnoreDistraction();
	/*  The Catherine clause - doesn't make sense for her to consider sponsored
		war on a friend if the cost is always prohibitive. */
	bool hiredAgainstFriend = (towardsThem >= ATTITUDE_FRIENDLY &&
			(params.getSponsor() != NO_PLAYER ||
			/*  The computations ignoring distraction are also used for decisions
				on joint wars (see WarAndPeaceAI::Team::declareWarTrade) */
			ignDistr));
	if(hiredAgainstFriend)
		uMinus = 50;
	uMinus *= linkedWarFactor;
	// When there's supposed to be uncertainty
	if(!ignDistr && ((noWarPercent > 0 && noWarPercent < 100) || hiredAgainstFriend)) {
		vector<long> hashInputs;
		hashInputs.push_back(theyId);
		hashInputs.push_back(towardsThem);
		hashInputs.push_back(we->AI_getMemoryCount(theyId, MEMORY_DECLARED_WAR));
		hashInputs.push_back(they->AI_getMemoryCount(weId, MEMORY_DECLARED_WAR));
		double r = ::hash(hashInputs, weId);
		double const uncertaintyBound = 12;
		double uncertainty = r * std::min(2 * uncertaintyBound, uMinus) -
				std::min(uncertaintyBound, 0.5 * uMinus);
		if(uncertainty > 0.5 || uncertainty < -0.5)
			log("%d %s for uncertainty", ::round(std::abs(uncertainty)),
					(uncertainty > 0 ? "added" : "subtracted"));
		uMinus += uncertainty;
		FAssert(uMinus >= 0);
	}
	/*  In team games, the pairwise affection adds up more than the
		pairwise conquests; need to correct that a bit. */
	if(agent.getNumMembers() > 1)
		uMinus = normalizeUtility(uMinus) * agent.getNumMembers() * 0.75;
	if(uMinus > 0.5) {
		log("NoWarAttProb: %d percent, our attitude: %d, for linked war: %d percent",
				::round(100 * pr), towardsThem, ::round(100 * linkedWarFactor));
		u -= ::round(uMinus);
	}
}

char const* Affection::aspectName() const { return "Affection"; }
int Affection::xmlId() const { return 18; }

Distraction::Distraction(WarEvalParameters& params) : WarUtilityAspect(params) {}

void Distraction::evaluate() {

	if(agent.isAVassal() || TEAMREF(theyId).isAVassal() || !m->isWar(weId, theyId))
		return;
	if(!agent.warAndPeaceAI().canReach(TEAMID(theyId)) &&
			!TEAMREF(theyId).warAndPeaceAI().canReach(agentId)) {
		log("No distraction from %s b/c neither side can reach the other",
				report.leaderName(theyId));
		return;
	}
	FAssert(!params.isIgnoreDistraction());
	int warDuration = agent.AI_getAtWarCounter(TEAMID(theyId));
	// Utility ignoring Distraction cost
	double utilityVsThem = normalizeUtility(
			ourCache->warUtilityIgnoringDistraction(TEAMID(theyId)));
	double uMinus = 0;
	double totalCostForPotential = 0;
	double maxPotential = 0;
	int numPotentialWars = 0;
	for(size_t i = 0; i < properCivs.size(); i++) {
		TeamTypes tId = TEAMID(properCivs[i]);
		if(tId == TEAMID(theyId) || !agent.isHasMet(tId) ||
				agent.warAndPeaceAI().isPushover(tId) ||
				// Making peace with the master won't help us to focus on the vassal
				GET_TEAM(tId).isAVassal() ||
				(!agent.warAndPeaceAI().canReach(tId) &&
				!GET_TEAM(tId).warAndPeaceAI().canReach(agentId)))
			continue;
		double ut = normalizeUtility(
				ourCache->warUtilityIgnoringDistraction(tId), tId);
		if(ut >= 50 && agent.isAtWar(TEAMID(theyId))) {
			log("No distraction as war utility against %s is high (%d)",
					report.leaderName(properCivs[i]), ::round(ut));
			continue;
		}
		/*  The war against 'they' distracts us from our war against tId.
			The comparison between war and peace scenario should cover this kind of
			distraction, but doesn't always b/c military analysis can have
			counterintuitive outcomes with greater losses in the peace scenario.
			Needs a little extra nudge. */
		if(agent.AI_getWarPlan(tId) != NO_WARPLAN) {
			uMinus += 5.5;
			log("War plan against %s distracts us from (actual)"
					" war plan against %s", report.leaderName(theyId),
					report.leaderName(properCivs[i]));
			if(ut > 0.5 && !m->isOnTheirSide(tId, true) && !agent.isAtWar(tId)) {
			/*  This means, war against tId is still in preparation or imminent,
				and we're considering peace with theyId; or there's a special offer
				(sponsored or diplo vote) to declare war on theyId.
				(Not possible: preparations against tId and theyId at the same time.) */
				uMinus += ut;
				log("%d extra cost for distraction from war in preparation",
						::round(ut));
			}
			// NB: Imminent war against tId is covered by WarAndPeaceAI::Team::considerPeace
		}
		/*  tId as a potential alternative war target. 'ut' is the utility of
			fighting both theyId and tId. Won't be able to tell how worthwhile war
			against tId really is until we make peace with theyId.
			Assume that we wouldn't have started a war against theyId if tId
			was the better target. But the longer the war against theyId lasts, the
			likelier a change in circumstances. */
		else if(agent.isAtWar(TEAMID(theyId)) &&
					// If tId==targetId, then tId isn't a "potential" target
					params.targetId() != tId &&
					/*  If there's a peace treaty, we probably don't want to
						attack them urgently */
					agent.canDeclareWar(tId) &&
				ut > -warDuration) {
			double costForPotential = std::min(15.0,
					(ut - std::max(0.0, utilityVsThem) + warDuration) / 1.55);
			if(costForPotential > 0.5) {
				log("War against %s (%d turns) distracts us from potential war plan "
						"against %s. Current utilities: %d/%d; Distraction cost: %d",
						report.leaderName(theyId), warDuration,
						report.leaderName(properCivs[i]), ::round(utilityVsThem),
						::round(ut), ::round(costForPotential));
				maxPotential = std::max(maxPotential, costForPotential);
				totalCostForPotential += costForPotential;
				numPotentialWars++;
			}
		}
	}
	if(numPotentialWars > 0) {
		/*  We're going start at most one of the potential wars, but having more
			candidates is good */
		double adjustedCostForPotential = maxPotential +
				(totalCostForPotential - maxPotential) /
				std::sqrt((double)numPotentialWars);
		if(adjustedCostForPotential > 0.5) {
			log("Adjusted cost for all (%d) potential wars: %d", numPotentialWars,
					::round(adjustedCostForPotential));
			uMinus += adjustedCostForPotential;
		}
	}
	/*  If we expect to knock them out, the current war may be over before the
		time horizon of the simulation; then it's a smaller distraction.
		(InvasionGraph could store elimination timestamps, but seems too much work
		to implement. Can guess the time well enough here by counting their remaining
		cities.) */
	if(uMinus > 0.01 && (m->isEliminated(theyId) ||
			m->getCapitulationsAccepted(agentId).count(TEAMID(theyId)) > 0)) {
		double mult = they->getNumCities() / 3.0;
		if(mult < 1) {
			log("Distraction cost reduced to %d percent b/c we're almost done "
					"with %s", ::round(mult * 100), report.leaderName(theyId));
			uMinus *= mult;
		}
	}
	u -= ::round(uMinus);
}

char const* Distraction::aspectName() const { return "Distraction"; }
int Distraction::xmlId() const { return 19; }

double WarUtilityAspect::normalizeUtility(double utilityTeamOnTeam, TeamTypes other) {

	if(other == NO_TEAM)
		other = TEAMID(theyId);
	return utilityTeamOnTeam /
			(GET_TEAM(other).getNumMembers() * agent.getNumMembers());
}

PublicOpposition::PublicOpposition(WarEvalParameters& params) : WarUtilityAspect(params) {}

void PublicOpposition::evaluate() {

	if(we->isAnarchy() || !m->isWar(weId, theyId) || m->isEliminated(weId))
		return;
	int pop = we->getTotalPopulation();
	if(pop <= 0) // at game start
		return;
	double faithAnger = 0;
	int dummy; for(CvCity* cp = we->firstCity(&dummy); cp != NULL;
			cp = we->nextCity(&dummy)) { CvCity const& c = *cp;
		if(c.isDisorder())
			continue;
		double angry = c.angryPopulation(0,
				!we->AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3));
		faithAnger += std::min(angry, c.getReligionPercentAnger(theyId) *
				c.getPopulation() / (double)GC.getPERCENT_ANGER_DIVISOR());
	}
	double wwAnger = ourCache->angerFromWarWeariness(theyId);
	if(wwAnger + faithAnger <= 0) return;
	log("Angry citizens from religion: %d, from ww: %d; total citizens: %d",
			::round(faithAnger), ::round(wwAnger), pop);
	// Assume that more ww is coming, and especially if we take the fight to them
	WarPlanTypes wp = agent.AI_getWarPlan(TEAMID(theyId));
	bool isTotal = (wp == WARPLAN_PREPARING_TOTAL || wp == WARPLAN_TOTAL);
	if(params.targetId() == TEAMID(theyId) && params.isTotal())
		isTotal = true;
	double incr = (isTotal ? 0.5 : 0.35);
	int conqueredFromThem = 0;
	for(iSetIt it = m->conqueredCities(weId).begin(); it != m->conqueredCities(weId).end(); it++)
		if(m->lostCities(theyId).count(*it) > 0)
			conqueredFromThem++;
	incr += (1.5 * conqueredFromThem) / std::max(we->getNumCities(), 1);
	if(wwAnger > 0)
		log("Expected increase in ww: %d percent (%d conquered cities, %s war)",
				::round(incr * 100), conqueredFromThem, (isTotal ? "total" : "limited"));
	double angerRate = (wwAnger * (1 + incr) + faithAnger) / pop;
	double uMinus = 125 * angerRate;
	log("War anger rate: %d percent%s", ::round(angerRate * 100),
			(uMinus < 0.5 ? " (negligible)" : ""));
	u -= ::round(uMinus);
}

char const* PublicOpposition::aspectName() const { return "Public opposition"; }
int PublicOpposition::xmlId() const { return 20; }

Revolts::Revolts(WarEvalParameters& params) : WarUtilityAspect(params) {}

void Revolts::evaluate() {

	/*  The war against theyId occupies our units, and rebels are taking advantage
		in the primary areas of theyId */
	double revoltLoss = 0;
	int totalAssets = 0; 
	CvMap& map = GC.getMapINLINE(); int foo;
	for(CvArea* a = map.firstArea(&foo); a != NULL; a = map.nextArea(&foo)) {
		if(!they->AI_isPrimaryArea(a) || (we->AI_isPrimaryArea(a) &&
				a->getCitiesPerPlayer(theyId) <= 2)) // Almost done here
			continue;
		// Can't rely on AreaAIType then
		bool willBeWar = m->isWar(weId, theyId) && !agent.isAtWar(TEAMID(theyId));
		AreaAITypes aai = a->getAreaAIType(agentId);
		// Training defenders is the CityAI's best remedy for revolts
		if((!willBeWar && (aai == AREAAI_DEFENSIVE || aai == AREAAI_NEUTRAL ||
				aai == NO_AREAAI)) || !m->isWar(weId, theyId))
			continue;
		int bar;
		for(CvCity* c = we->firstCity(&bar); c != NULL; c = we->nextCity(&bar)) {
			if(c->area()->getID() != a->getID())
				continue;
			City* cacheCity = ourCache->lookupCity(c->plotNum());
			// Count each city only once
			if(cacheCity == NULL || countedCities.count(c->plotNum()) > 0)
				continue;
			countedCities.insert(c->plotNum());
			int assetScore = cacheCity->getAssetScore();
			totalAssets += assetScore;
			double prRevolt = c->revoltProbability(false, false, true);
			if(prRevolt <= 0)
				continue;
			/*  If the AI has been failing to supress a city for a long time,
				then it's probably not a matter of distracted units. */
			if(c->getNumRevolts() > GC.getNUM_WARNING_REVOLTS()) {
				log("%s skipped as hopeless", report.cityName(*c));
				continue;
			}
			double lossFactor = 5 * std::min(0.1, prRevolt);
			if(lossFactor > 0) {
				log("%s in danger of revolt (%d percent; assets: %d)",
						report.cityName(*c), ::round(prRevolt * 100), assetScore);
				revoltLoss += assetScore * lossFactor;
			}
		}
	}
	if(totalAssets <= 0)
		return;
	/*  Give amortizationMultiplier 50% weight - revolts make cities less useful
		economically, but can also disrupt victory conditions. */
	double uMinus = (1 + weAI->amortizationMultiplier()) * 50 *
			revoltLoss / totalAssets;
	u -= ::round(uMinus);
}

char const* Revolts::aspectName() const { return "Revolts"; }
int Revolts::xmlId() const { return 21; }

UlteriorMotives::UlteriorMotives(WarEvalParameters& params) :
	WarUtilityBroaderAspect(params) {} // Need to evaluate for theyId==sponsorId

void UlteriorMotives::evaluate() {

	/*  Difference between ourCache->sponsorAgainst(TeamTypes t) and
		params.getSponsor (returns t):
		The former tells us that our war against t has been sponsored; so, we've
		already agreed to the war trade. The latter says that we're currently
		considering war against t. This aspect needs to apply when considering the
		offer. Once we've declared war, we should forget about our issues with the
		sponsor's motives, and instead live up to our promise (as enforced by
		HiredHand). */
	if(m->isPeaceScenario() || theyId != params.getSponsor() ||
			we->getCapitalCity() == NULL || they->getCapitalCity() == NULL)
		return;
	if(agent.AI_getWarPlan(TEAMID(theyId)) != NO_WARPLAN) {
		/*  They're probably trying to deflect our attack. That doesn't have to
			be bad for us; Distraction cost accounts for the difference in
			war utility vs. sponsor and target. But if they're willing to pay us
			off, we should ask for sth. extra. And don't want to make it too
			easy for humans to avoid wars. */
		FAssert(!params.isIgnoreDistraction()); // sponsor should be NO_PLAYER then
		int uSponsor = ourCache->warUtilityIgnoringDistraction(TEAMID(theyId));
		u -= std::max(5, uSponsor / 2);
		return;
	}
	TeamTypes targetId = params.targetId();
	CvTeamAI const& target = GET_TEAM(targetId);
	bool jointWar = m->isWar(TEAMID(theyId), targetId);
	// If they're in a hot war, their motives seem clear (and honest) enough
	if(jointWar && target.AI_getWarSuccess(TEAMID(theyId)) +
			TEAMREF(theyId).AI_getWarSuccess(targetId) >
			(3 * GC.getWAR_SUCCESS_CITY_CAPTURING()) / 2 &&
			// Perhaps no longer hot?
			target.warAndPeaceAI().canReach(TEAMID(theyId)) &&
			// If the target is in our area but not theirs, that's fishy
			target.AI_isPrimaryArea(we->getCapitalCity()->area()) <=
			target.AI_isPrimaryArea(they->getCapitalCity()->area()))
		return;
	/*  Otherwise, they might want to hurt us as much as the target, and we should
		demand extra payment to allay our suspicions. Use DWRAT (greater than
		Annoyed, Cautious or Pleased respectively) as an indicator of how
		suspicious we are. */
	int delta = GC.getLeaderHeadInfo(we->getPersonalityType()).
			getDeclareWarRefuseAttitudeThreshold() - towardsThem + 1;
	int uMinus = 15 + 5 * (delta - (towardsThem == ATTITUDE_FRIENDLY ? 1 : 0));
	if(!jointWar) uMinus += 5;
	if(uMinus > 0) {
		log("Difference between attitude towards %s (%d) and refuse-thresh: %d",
				report.leaderName(theyId), towardsThem, -delta);
		u -= uMinus;
	}
}

char const* UlteriorMotives::aspectName() const { return "Ulterior motives"; }
int UlteriorMotives::xmlId() const { return 22; }

FairPlay::FairPlay(WarEvalParameters& params) : WarUtilityAspect(params) {}

void FairPlay::evaluate() {

	if(TEAMREF(theyId).isAVassal() ||
			// Once we've gone through the trouble of preparing war, it's too late.
			m->getWarsDeclaredBy(weId).count(theyId) <= 0 ||
			agent.AI_isSneakAttackReady(TEAMID(theyId)) ||
			// No kid gloves if they've attacked us before
			we->AI_getMemoryAttitude(theyId, MEMORY_DECLARED_WAR) < 0 ||
			they->AI_isDoVictoryStrategyLevel3())
		return;
	// Avoid big dogpiles (or taking turns attacking a single target)
	double otherEnemies = 0;
	int potentialOtherEnemies = 0;
	CvGame& g = GC.getGameINLINE();
	int theirRank = g.getPlayerRank(theyId);
	for(size_t i = 0; i < properCivs.size(); i++) {
		CvPlayerAI const& other = GET_PLAYER(properCivs[i]);
		if(other.getTeam() == agentId || other.getID() == theyId || other.isAVassal())
			continue;
		potentialOtherEnemies++;
		bool immediateWar = (TEAMREF(theyId).isAtWar(other.getTeam()) ||
				m->getWarsDeclaredBy(other.getID()).count(theyId) > 0);
		if(immediateWar || they->AI_getMemoryAttitude(other.getID(),
				MEMORY_DECLARED_WAR) <= -2) {
			double incr = 1.15;
			if(!immediateWar) {
				incr *= 0.73;
				if(they->AI_getMemoryAttitude(other.getID(),
						MEMORY_DECLARED_WAR) == -2)
					incr *= 0.85;
			}
			// Ganging up on the leader is less problematic
			if(theirRank < g.getPlayerRank(other.getID()))
				incr /= 2;
			otherEnemies += incr;
			log("Another enemy of %s: %s; increment: %d percent",
					report.leaderName(theyId),
					report.leaderName(other.getID()), ::round(incr * 100));
		}
		else if(TEAMREF(theyId).AI_shareWar(other.getTeam())) {
			log("An ally of %s: %s", report.leaderName(theyId),
					report.leaderName(other.getID()));
			otherEnemies--;
		}
		if(other.AI_getMemoryAttitude(theyId,
				MEMORY_DECLARED_WAR) <= -2) {
			log("They've attacked %s before", report.leaderName(other.getID()));
			otherEnemies -= 0.5;
		}
	}
	if(potentialOtherEnemies > 0 && otherEnemies > 0) {
		double fromOtherEnemies = 30 * (otherEnemies /
				std::sqrt((double)potentialOtherEnemies) - 0.5);
		if(fromOtherEnemies > 0.5) {
			log("From other enemies: %d", ::round(fromOtherEnemies));
			u -= ::round(fromOtherEnemies);
		}
	}
	// The rest of this function deals with the early game
	/*  Assume that early AI-on-AI wars are always fair b/c they have the same
		handicap. Not actually true in e.g. EarthAD1000 scenario. Still, early
		attacks on AI civs aren't a serious problem. */
	if(!they->isHuman() || we->isHuman() ||
			we->AI_getMemoryAttitude(theyId, MEMORY_DECLARED_WAR_ON_FRIEND) < 0)
		return;
	CvHandicapInfo& h = GC.getHandicapInfo(g.getHandicapType());
	/*  Mostly care about Archery, which has power 6. The Wheel isn't unfair
		(power 4). BW and IW have power 8 and 10. */
	/*bool powerTechFound = false;
	for(int i = 0; i < GC.getNumTechInfos(); i++) {
		if(h.isAIFreeTechs(i) && GC.getTechInfo((TechTypes)i).
				getPowerValue() >= 5) {
			powerTechFound = true;
			break;
		}
	}
	if(!powerTechFound)
		return;*/
	/*  Actually, never mind checking for starting tech. Don't want early rushes
		on low difficulty either, and on King, the AI doesn't get Archery, but
		lots of other freebies. */
	int trainPercent = GC.getGameSpeedInfo(g.getGameSpeedType()).getTrainPercent();
	if(trainPercent <= 0) {
		FAssert(trainPercent > 0);
		return;
	}
	int t = ::round(g.getElapsedGameTurns() / (trainPercent / 100.0));
	/*  All bets off by turn 100, but, already by turn 50, the cost may
		no longer be prohibitive. */
	double uMinus = std::pow((100 - t) / 2.0, 1.28);
	if(g.getCurrentEra() > 0) // The above only matters in the Ancient era
		uMinus = 0;
	// ... but dogpiling remains an issue in the Classical era
	if(g.getCurrentEra() > 1)
		return;
	// Don't dogpile when human has lost cities in the early game
	int citiesBuilt = they->getPlayerRecord()->getNumCitiesBuilt();
	double fromCityLoss = 0;
	if(citiesBuilt > 0) {
		fromCityLoss = 100 * std::pow((1 -
				they->getNumCities() / (double)citiesBuilt), 0.85);
	}
	if(fromCityLoss >= 0.5) {
		log("From lost human cities: %d", ::round(fromCityLoss));
		uMinus += fromCityLoss;
	}
	// If no cities gained nor lost, at least don't DoW in quick succession.
	else if(they->getNumCities() == citiesBuilt) {
		int fromRecentDoW = 35 * TEAMREF(theyId).getWarPlanCount(WARPLAN_ATTACKED_RECENT);
		log("From recent DoW: %d", fromRecentDoW);
		uMinus += fromRecentDoW;
	}
	int div = 3 - towardsThem + we->AI_getMemoryAttitude(theyId, MEMORY_REJECTED_DEMAND);
	if(div > 1) {
		log("Divided by %d because of attitude", div);
		uMinus /= div;
	}
	u -= std::max(0, ::round(uMinus));
}

int FairPlay::initialMilitaryUnits(PlayerTypes civId) {

	// (this function isn't currently used)
	CvPlayer const& civ = GET_PLAYER(civId);
	CvHandicapInfo& h = GC.getHandicapInfo(civ.getHandicapType());
	// DefenseUnits aren't all that defensive
	return h.getStartingDefenseUnits() +
			(civ.isHuman() ? 0 : h.getAIStartingDefenseUnits());
}

char const* FairPlay::aspectName() const { return "Fair play"; }
int FairPlay::xmlId() const { return 23; }

Bellicosity::Bellicosity(WarEvalParameters& params) : WarUtilityAspect(params) {}

void Bellicosity::evaluate() {

	if(we->isHuman() || !m->isWar(agentId, TEAMID(theyId)) ||
			!agent.warAndPeaceAI().canReach(TEAMID(theyId)))
		return;
	// One war is enough
	if(agent.getAtWarCount() > 0 && !agent.isAtWar(TEAMID(theyId)))
		return;
	/*  This is what makes units attack against the odds. 6 for Napoleon and Ragnar,
		4 for Alexander, Boudica, Brennus, Genghis, Gilgamesh, Hannibal, Montezuma,
		Peter, Shaka and Sitting Bull. Just the suicidal types I need except
		for Sitting Bull. Subtract peace-weight to exclude him - all the others
		have a peace-weight near 0. */
	CvLeaderHeadInfo const& lh = GC.getLeaderHeadInfo(we->getPersonalityType());
	int bellicosity = lh.getBaseAttackOddsChange() - lh.getBasePeaceWeight();
	if(bellicosity <= 0)
		return;
	double deltaLostPow = 0;
	double presentAggrPow = 0;
	for(int i = 0; i < NUM_BRANCHES; i++) {
		MilitaryBranchTypes mb = (MilitaryBranchTypes)i;
		// Included in other branches, and we never itch for nuclear war
		if(mb == CAVALRY || mb == LOGISTICS || mb == NUCLEAR)
			continue;
		double ourLostPow = m->lostPower(weId, mb);
		/*  If they lose much more than we do, chances are that a third party is
			causing this. And don't want a warlike AI to attack a weak civ just
			for sport. */
		deltaLostPow += std::min(2.35 * ourLostPow,
				m->lostPower(theyId, mb)) - 0.75 * ourLostPow;
		if(mb != HOME_GUARD) // Not tracked by cache, and not really relevant here
			presentAggrPow += ourCache->getPowerValues()[mb]->power();
	}
	if(deltaLostPow <= 0 || presentAggrPow < 1)
		return;
	// A good war is one that we win, and that occupies many of our eager warriors
	double gloryRate = std::min(1.0, deltaLostPow / presentAggrPow);
	log("Difference in lost power: %d; present aggressive power: %d; bellicosity: %d",
			::round(deltaLostPow), ::round(presentAggrPow), bellicosity);
	u += ::round(2 * bellicosity * gloryRate);
}

char const* Bellicosity::aspectName() const { return "Bellicosity"; }
int Bellicosity::xmlId() const { return 24; }

TacticalSituation::TacticalSituation(WarEvalParameters& params)
		: WarUtilityAspect(params) {}

void TacticalSituation::evaluate() {

	if(!m->isWar(weId, theyId))
		return;
	if(agent.isAtWar(TEAMID(theyId)))
		evalEngagement();
	else evalOperational();
}

void TacticalSituation::evalEngagement() {

	/*  Probably the most computationally expensive part of war evaluation.
		Still pretty sure that's it's not a performance bottleneck by any means.
		Can't move this to WarAndPeaceCache (to compute it only once at the start
		of a turn) b/c it needs to be up-to-date during the turns of other civs,
		in particular the humans that could propose peace at any point of their
		turns.
		Note that CvCityAI::isEvacuating is only updated at the start of the city
		owner's turn. This is OK; a unit that approaches the city during an
		enemy's turn usually won't have enough moves to attack right away. */
	int ourExposed = 0;
	int theirExposed = 0;
	int entangled = 0;
	int ourTotal = 0;
	double ourMissions = 0;
	int const hpThresh = 60;
	int dummy;
	for(CvSelectionGroup* gr = we->firstSelectionGroup(&dummy); gr != NULL;
			gr = we->nextSelectionGroup(&dummy)) {
		CvUnit* head = gr->getHeadUnit();
		if(head == NULL || !head->canDefend())
			continue;
		int groupSize = gr->getNumUnits();
		if(groupSize <= 0)
			continue;
		ourTotal += groupSize;
		int range = 1;
		CvPlot* pl = gr->plot();
		PlayerTypes plotOwner = pl->getOwnerINLINE();
		/*  Limit range, not least for performance reasons, to cover combat imminent
			on our current turn or the opponent's next turn. If our group is on a
			friendly route, we can probably attack units two tiles away,
			though we probably won't if our units are damaged. */
		if(head->maxHitPoints() - head->getDamage() >= 80) {
			if(pl->area()->isWater())
				range++;
			else if(plotOwner != NO_PLAYER &&
					(plotOwner == weId || agent.isOpenBorders(TEAMID(plotOwner)))
					&& (pl->isRoute() || pl->isCity()))
				range++;
		}
		bool isCity = gr->plot()->isCity();
		int theirDamaged = 0;
		/*  Count at most one enemy unit per unit of ours as "entangled", i.e. count
			pairs of units. */
		int pairs = we->AI_getPlotDanger(pl, range, false, false, &theirDamaged,
				hpThresh, groupSize, theyId);
		/*  Should perhaps also count fewer units as entangled if their units are
			in a city, but that gets complicated b/c AI_getPlotDanger would have to
			check. */
		entangled += pairs / (isCity ? 2 : 1);
		// "Exposed" means damaged and likely about to be attacked
		theirExposed += theirDamaged;
		int ourDamaged = 0;
		if(pairs > 0) {
			for(CLLNode<IDInfo>* node = gr->headUnitNode(); node != NULL; node =
					gr->nextUnitNode(node)) { CvUnit& u = *::getUnit(node->m_data);
				if(u.maxHitPoints() - u.getDamage() <= hpThresh)
					ourDamaged++;
			}
		}
		/*  Our damaged units are unlikely to be attacked if we have enough healthy
			units grouped with them. */
		if(!isCity) // Assume here that cities are safe here (ourEvac handles them)
			ourExposed += std::max(ourDamaged + pairs - groupSize, 0);
		if(!we->isHuman()) {
			// Akin to CvPlayerAI::AI_enemyTargetMissions
			MissionAITypes mission = gr->AI_getMissionAIType();
			CvPlot* missionPlot = gr->AI_getMissionAIPlot();
			/*  K-Mod uses MISSIONAI_ASSAULT also for land-based assaults on cities.
				This should be more helpful than CvPlayerAI::AI_enemyTargetMission,
				which counts all missions that target a hostile tile, i.e. also
				reinforcements. (CvTeamAI::AI_endWarVal relies on that function.) */
			if((mission == MISSIONAI_PILLAGE && plotOwner == theyId) ||
					(mission == MISSIONAI_ASSAULT && missionPlot != NULL &&
					missionPlot->getOwnerINLINE() == theyId))
				// Don't count entangled units on missions
				ourMissions += groupSize + gr->getCargo() - pairs;
		}
	}
	if(ourMissions > 0 && agent.warAndPeaceAI().isPushover(TEAMID(theyId))) {
		/*  If the target is weak, even a small fraction of our military en route
			could have a big impact once it arrives. */
		ourMissions *= 1.5;
		log("Mission count increased b/c target is short work");
	}
	int ourEvac = evacPop(weId, theyId);
	int theirEvac = evacPop(theyId, weId);
	/*  If a human is involved or if it's our turn, then we shouldn't worry too
		much about our units being exposed - humans may well have already made
		all attacks against vulnerable units before contacting us. If it's our
		turn, it's still the beginning of the turn, so we can probably save
		our units. */
	double initiativeFactor = 0.25; // Low if they have the initiative
	if(gDLL->isDiplomacy() || // I.e. we're negotiating with a human
			we->isHuman() || they->isHuman())
		initiativeFactor = 0.5;
	double uPlus = (4.0 * (initiativeFactor * theirExposed -
			(1 - initiativeFactor) * ourExposed) +
			entangled + ourMissions) / ourTotal;
	if(we->getTotalPopulation() > 0)
		uPlus += 3.0 * (theirEvac - 1.35 * ourEvac) / we->getTotalPopulation();
	uPlus *= 100;
	int recentlyLostPop = 0;
	for(CvCity* c = they->firstCity(&dummy); c != NULL; c = they->nextCity(&dummy)) {
		int range = 2;
		// Do we have Engineering?
		if(agentAI.isFastRoads()) {
			// Range 3 could be costly, but there'll be few such cities
			range++;
		}
		if(c->isOccupation() && c->isEverOwned(weId) && they->AI_getPlotDanger(
				c->plot(), 3, false, false) >
				1) /* Just want to know if we have some presence beyond a single
					  stray unit near the city */
			recentlyLostPop += c->getPopulation();
	}
	double recentlyLostPopRatio = 0;
	if(we->getTotalPopulation() > 0)
		recentlyLostPopRatio = recentlyLostPop / we->getTotalPopulation();
	uPlus += recentlyLostPopRatio * 100;
	if(uPlus > 0.5 || uPlus < -0.5) {
		log("Their exposed units: %d, ours: %d; entanglement: %d; "
			"their evacuating population: %d, ours: %d; our missions: %d; "
			"our total milit. units: %d; our population: %d; "
			"recently lost population: %d (%d percent)",
			theirExposed, ourExposed, entangled, theirEvac,
			ourEvac, ::round(ourMissions), ourTotal, we->getTotalPopulation(),
			recentlyLostPop, ::round(100 * recentlyLostPopRatio));
		u += ::round(uPlus);
	}
}

int TacticalSituation::evacPop(PlayerTypes ownerId, PlayerTypes invaderId) {

	int r = 0; int dummy;
	CvPlayer const& o = GET_PLAYER(ownerId);
	for(CvCity* c = o.firstCity(&dummy); c != NULL; c = o.nextCity(&dummy)) {
		/*  Check PlotDanger b/c we don't want to count cities that are threatened
			by a third party */
		if(c->isEvacuating() && o.AI_getPlotDanger(c->plot(),
				1, false, false, NULL, 60, 2, invaderId) >= 2)
			r += c->getPopulation();
	}
	return r;
}

void TacticalSituation::evalOperational() {

	/*  Evaluate our readiness only for the target that we want to invade
		(not its vassals or allies from def. pact) */
	if(params.targetId() != TEAMID(theyId))
		return;
	/*  When taking the human pov, we mustn't assume to know their exact unit counts.
		Whether they're ready for an attack is secret info. */
	if(we->isHuman()) return;
	if(ourCache->numReachableCities(theyId) <= 0 || (params.isNaval() &&
			// Won't need invaders then
			ourCache->getPowerValues()[LOGISTICS]->getTypicalUnitType() == NO_UNIT))
		return;
	// Similar to CvUnitAI::AI_stackOfDoomExtra
	double targetAttackers = 4 + std::pow((double)they->getCurrentEra(), 1.3);
	if(params.isNaval())
		targetAttackers *= 1.3;
	double trainMod = GC.getHandicapInfo(GC.getGameINLINE().
			getHandicapType()).getAITrainPercent() / 100.0;
	// Smaller target on lower difficulty
	targetAttackers /= ::dRange(trainMod, 1.0, 1.5);
	/*  Don't necessarily need a larger initial stack for total war.
		A little maybe ... */
	if(params.isTotal())
		targetAttackers++;
	// Need larger stacks when not all our army units can destroy defenders
	bool canBombard = ourCache->getPowerValues()[ARMY]->canBombard();
	if(canBombard)
		targetAttackers *= 1.3;
	/*  To account for the AI's inability to put all available attackers in one spot,
		and misc. distractions like barbarians */
	targetAttackers += std::max(0, we->getNumCities() - 2);
	double targetCargo = 0;
	double cargo = 0;
	double targetEscort = 0;
	// Minor issue: counts Galleons (even when we could be up against Frigates)
	int escort = we->AI_totalUnitAIs(UNITAI_ATTACK_SEA) +
			we->AI_totalUnitAIs(UNITAI_ESCORT_SEA) +
			we->AI_totalUnitAIs(UNITAI_RESERVE_SEA);
	if(params.isNaval()) {
		targetCargo = targetAttackers;
		cargo = ourCache->getPowerValues()[LOGISTICS]->power();
		/*  If we have 0 escorters, we probably can't build them; probably have
			Galleys then but no Triremes. That's OK.
			Also no need for escort if they're way behind in tech. */
		if(escort > 0 && they->getCurrentEra() >= we->getCurrentEra()) {
			/*  More escort units would be nice, but I'm checking only
				bare necessities. */
			targetEscort = targetCargo / 6;
		}
	}
	// NB: Sea and air units have their own UNITAI types; this counts only land units
	int attackers = we->AI_totalUnitAIs(UNITAI_ATTACK) +
			we->AI_totalUnitAIs(UNITAI_ATTACK_CITY) +
			we->AI_totalUnitAIs(UNITAI_RESERVE) +
			we->AI_totalUnitAIs(UNITAI_PILLAGE) +
			we->AI_totalUnitAIs(UNITAI_PARADROP) +
			we->AI_totalUnitAIs(UNITAI_COLLATERAL) -
			/*  Settlers will occupy potential attackers as escorts.
				NB: The AI scraps Settlers eventually if it can't use them;
				can't block war plans permanently. */
			2 * we->AI_totalUnitAIs(UNITAI_SETTLE);
	/*  (Can't check if we have enough Siege units b/c their UNITAI types overlap
		with non-Siege attackers.) */
	double readiness = std::min(1.0, attackers / targetAttackers);
	if(targetCargo > 0)
		readiness = std::min(readiness, std::min(1.0, cargo / targetCargo));
	if(targetEscort > 0) {
		readiness = std::min(readiness, std::min(1.0,
				// escort not totally crucial
				0.5 * (readiness + escort / targetEscort)));
	}
	if(readiness > 0.99) return;
	int remaining = params.getPreparationTime();
	FAssert(remaining >= 0);
	int initialPrepTime = 0;
	if(!params.isImmediateDoW()) {
		WarPlanTypes wp = agent.AI_getWarPlan(TEAMID(theyId));
		if(params.isTotal()) {
			if(params.isNaval())
				initialPrepTime = WarAndPeaceAI::preparationTimeTotalNaval;
			else initialPrepTime = WarAndPeaceAI::preparationTimeTotal;
		}
		else { 
			if(params.isNaval())
				initialPrepTime = WarAndPeaceAI::preparationTimeLimitedNaval;
			else initialPrepTime = WarAndPeaceAI::preparationTimeLimited;
		}
	}
	FAssert(initialPrepTime >= 0);
	/*  Cost for lack of preparation is based on the assumption that it won't
		hurt much to prolong preparations a bit. Not true for immediate DoW.
		Treat this case as if half the prep time still remained (although actually
		there isn't and never was time to prepare). */
	double passedPortion = 0.5;
	if(initialPrepTime > 0)
		passedPortion = 1 - std::min(1.0, remaining / (double)initialPrepTime);
	if(passedPortion < 0.01) return;
	if(canBombard)
		log("Extra attackers needed for mixed siege/attack stacks");
	log("Readiness %d percent (%d of %d attackers, %d of %d cargo, "
			"%d of %d escort); %d of %d turns for preparation remain",
			::round(readiness * 100), attackers, ::round(targetAttackers),
			::round(cargo), ::round(targetCargo), escort, ::round(targetEscort), 
			remaining, initialPrepTime);
	int uMinus = ::round(100 * std::pow(passedPortion, 1.5) *
			(1 - std::pow(readiness, 4)));
	if(uMinus == 0)
		log("Cost for lack of readiness vs. %s negligible",
				report.leaderName(theyId));
	u -= uMinus;
}

char const* TacticalSituation::aspectName() const { return "Tactical situation"; }
int TacticalSituation::xmlId() const { return 25; }

// </advc.104>