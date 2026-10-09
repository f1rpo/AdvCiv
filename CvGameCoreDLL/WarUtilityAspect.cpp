#include "CvGameCoreDLL.h"
#include "WarUtilityAspect.h"
#include "UWAICache.h"
#include "UWAILogMuteState.h" // <!-- custom: Utility aspects read the shared nested mute state solely to suppress intentionally silent sub-evaluations. See KI#505.3. (ChatGPT-5.6-Sol) -->
#include "WarEvalParameters.h"
#include "UWAIAgent.h"
#include "MilitaryAnalyst.h"
#include "CoreAI.h"
#include "CvCityAI.h"
#include "CvUnitAI.h"
#include "CvSelectionGroupAI.h"
#include "CvDeal.h"
#include "PlotRadiusIterator.h"
#include "CvArea.h"
#include "CvInfo_GameOption.h"
#include "CvInfo_Building.h" // Just for vote-related info
#include "BBAILog.h" // <!-- custom: Log the exact reason when the AdvCiv-SAS faraway-war hard reject removes a proposed target. (GPT-5.6-Sol) -->

using std::vector;
using std::pair;
using std::make_pair;
typedef UWAICache::City City;

/*	These player and team data are accessed a lot in this file. Refer to the agent
	as "we" and to the current rival (set by evaluate(MilitaryAnalyst const&))
	as "they" -- for brevity, also in comments. */
#define kWe				(getAgentPlayer())
#define kOurTeam		(getAgentTeam())
#define eWe				(kWe.getID())
#define eOurTeam		(kOurTeam.getID())
#define kThey			(getRivalPlayer())
#define eThey			(kThey.getID())
#define kTheirTeam		(GET_TEAM(eThey))
#define eTheirTeam		(TEAMID(eThey))
#define kWeAI			(kWe.uwai())
#define kOurPersonality	(GC.getInfo(kWe.getPersonalityType()))


WarUtilityAspect::WarUtilityAspect(WarEvalParameters const& kParams)
// <!-- custom: Aspects share the evaluator's nested mute state only; their diagnostic output is ordinary caller-pre-gated BBAI. (ChatGPT-5.6-Sol) -->
:	m_kParams(kParams), m_kAgentTeam(GET_TEAM(kParams.getAgent())),
	m_kLogMuteState(kParams.getLogMuteState()), m_kGame(GC.AI_getGame()),
	m_eGameEra(m_kGame.getCurrentEra()),
	m_rGameEraAIFactor(m_kGame.AI_getCurrEraFactor()),
	m_kSpeed(GC.getInfo(m_kGame.getGameSpeedType())),
	m_iU(0)
{
	reset();
}


void WarUtilityAspect::reset()
{
	//PROFILE_FUNC(); // About 100,000 calls per game turn. Not a concern.
	m_pMilitaryAnalyst = NULL;
	m_pAgentPlayer = NULL;
	m_pAgentCache = NULL;
	resetRival();
}


void WarUtilityAspect::resetRival()
{
	m_pRivalPlayer = NULL;
	m_aeAgentConquersFromRival.clear();
	m_eTowardRival = m_eTowardAgent = NO_ATTITUDE;
	m_iDiploTowardRival = m_iDiploTowardAgent = -1;
}


int WarUtilityAspect::evaluate(MilitaryAnalyst const& kMilitaryAnalyst)
{
	PROFILE_FUNC();
	this->m_pMilitaryAnalyst = &kMilitaryAnalyst;
	m_pAgentPlayer = &(GET_PLAYER(kMilitaryAnalyst.getAgentPlayer()));
	m_pAgentCache = &m_pAgentPlayer->uwai().getCache();
	// <!-- custom: Aspect objects persist across all alive agent-team members. Preserve the prior members' weighted total so the XML weight applies once only to this member's raw delta. See KI#424. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	int const iPriorUtility = m_iU;

	int iOverallUtility = preEvaluate();
	bool const bOnlyWarParties = concernsOnlyWarParties();
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	for (PlayerIter<MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itRival(eOurTeam);
		itRival.hasNext(); ++itRival)
	{
		if (bOnlyWarParties && !kMilitaryAnalyst.isPartOfAnalysis(itRival->getID()))
			continue;
		int iPerRivalUtility = evaluate(itRival->getID());
		if (iPerRivalUtility != 0)
		{
			if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_ASPECT_RIVAL_RESULT turn=%d agentPlayer=%d rivalPlayer=%d aspect=%s utility=%d",
					GC.getGame().getGameTurn(), eWe, itRival->getID(), aspectName(), iPerRivalUtility);
		}
	}
	if (iOverallUtility != 0)
	{
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_ASPECT_OVERALL_RESULT turn=%d agentPlayer=%d aspect=%s utility=%d",
				GC.getGame().getGameTurn(), eWe, aspectName(), iOverallUtility);
		m_iU += iOverallUtility;
	}
	int iMemberUtility = m_iU - iPriorUtility;
	scaled const rXMLAdjust = getUWAI().aspectWeight(xmlID());
	if (iMemberUtility != 0 && rXMLAdjust != 1)
	{
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_ASPECT_XML_ADJUSTMENT turn=%d agentPlayer=%d aspect=%s percent=%d",
				GC.getGame().getGameTurn(), eWe, aspectName(), rXMLAdjust.getPercent());
		iMemberUtility = (iMemberUtility * rXMLAdjust).round();
		m_iU = iPriorUtility + iMemberUtility;
	}
	reset();
	// <!-- custom: Return only this member's weighted contribution so WarEvaluator's per-player diagnostic is not cumulative across teammates. See KI#424. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	return iMemberUtility;
}


int WarUtilityAspect::evaluate(PlayerTypes eRivalPlayer)
{
	m_pRivalPlayer = &(GET_PLAYER(eRivalPlayer));
	/*	Should never use human attitude, but an error value like MIN_INT
		wouldn't get caught. Better to use a sane value (0 -> Cautious). */
	m_iDiploTowardAgent = (kThey.isHuman() ? 0 : kThey.AI_getAttitudeVal(eWe));
	/*	If we're human, it can mean that another AI player is evaluating war
		from the point of view of a human rival. Should therefore also
		disregard our attitude. */
	m_iDiploTowardRival = (kWe.isHuman() ? 0 : kWe.AI_getAttitudeVal(eThey));
	/*	In the very early game, war preparations take a long time, and diplo
		will often improve during that time b/c of "years of peace". If this
		brings attitude to Pleased, the war is usually off, and the preparations
		(very costly this early) are for nothing (unless there's an alt. target;
		not considered here). Need to anticipate the change in attitude. */
	/*	No worries once preparations are well underway or when asked to
		declare war immediately (or when already at war) */
	if (!kWe.isHuman() && m_kParams.getPreparationTime() > 5 &&
		m_kGame.gameTurnProgress() < fixp(0.18) &&
		kWe.AI_getPeaceAttitude(eThey) == 0 &&
		kOurTeam.AI_getAtPeaceCounter(eTheirTeam) * 2 >
		kOurPersonality.getAtPeaceAttitudeDivisor())
	{
		m_iDiploTowardRival++;
	}
	m_eTowardRival = (kWe.isHuman() ? ATTITUDE_CAUTIOUS :
			kWe.AI_getAttitudeFromValue(m_iDiploTowardRival));
	/*	advc.130o: If recently received tribute, we're pleased as far
		as war plans go. */
	if (!kWe.isHuman() && !m_kParams.isConsideringPeace() &&
		kWe.AI_getMemoryCount(eThey, MEMORY_ACCEPT_DEMAND) > 0 &&
		kThey.AI_getMemoryCount(eWe, MEMORY_MADE_DEMAND_RECENT) > 0)
	{
		m_eTowardRival = std::max(m_eTowardRival, ATTITUDE_PLEASED);
	}
	m_eTowardAgent = (kThey.isHuman() ? ATTITUDE_CAUTIOUS :
			kThey.AI_getAttitudeFromValue(m_iDiploTowardAgent));
	CitySet const& kWeConquer = militAnalyst().conqueredCities(eWe);
	for (CitySetIter it = kWeConquer.begin(); it != kWeConquer.end(); ++it)
	{
		if (militAnalyst().lostCities(eThey).count(*it) > 0)
			m_aeAgentConquersFromRival.push_back(*it);
	}
	int iUtilityBefore = m_iU;
	evaluate();
	resetRival();
	return m_iU - iUtilityBefore;
}


int WarUtilityAspect::preEvaluate() { return 0; }


bool WarUtilityAspect::concernsOnlyWarParties() const { return true; }


char const* WarUtilityAspect::aspectName() const
{
	return getUWAI().aspectName(xmlID());
}



scaled WarUtilityAspect::normalizeUtility(scaled rUtilityTeamOnTeam, TeamTypes eOther) const
{
	if (eOther == NO_TEAM)
		eOther = eTheirTeam;
	return rUtilityTeamOnTeam /
			(GET_TEAM(eOther).getNumMembers() * kOurTeam.getNumMembers());
}


// <!-- custom: Evaluate the explicit victim rather than implicitly reusing eThey; Kingmaking also calls this for the rival's teammates and vassals. See KI#432. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
scaled WarUtilityAspect::netLostAssetScore(PlayerTypes eVictim, PlayerTypes eTo, scaled* prTotalScore, TeamTypes eIgnoreGains) const
{
	PROFILE_FUNC();
	CvPlayerAI const& kVictim = GET_PLAYER(eVictim);
	scaled rNetLoss;
	scaled rTotal;
	// Cities that they lose
	/*for (int i = 0; i < kVictim.uwai().getCache().size(); i++) {
		City* pCacheCity = kVictim.uwai().getCache().getCity(i);*/
	/*	In large games, looking up their cities is faster than a pass through their
		whole cache. */
	CitySet const& kVictimLosses = militAnalyst().lostCities(eVictim);
	FOR_EACH_CITY(pCity, kVictim)
	{
		City const* pCacheCity = kVictim.uwai().getCache().lookupCity(pCity->plotNum());
		if (pCacheCity == NULL)
			continue;
		if (!kOurTeam.AI_deduceCitySite(*pCity))
			continue;
		/*	Their cached value accounts for GP, but we're not supposed to see those.
			(Their cache contains some other info we shouldn't see, but nothing
			crucial I think.) */
		scaled rAssetScore = pCacheCity->getAssetScore() - 4 * pCity->getNumGreatPeople();
		if (kVictimLosses.count(pCacheCity->getID()) > 0 && (eTo == NO_PLAYER || militAnalyst().conqueredCities(eTo).count(pCacheCity->getID()) > 0))
		{
			rNetLoss += rAssetScore;
			// Losing the capital is bad (but not all that bad b/c the Palace relocates)
			if (pCity->isCapital())
				rNetLoss += 4;
		}
		else if (pCity->isCapital())
			rTotal += 8;
		// National wonders other than Palace are invisible to us
		rTotal += rAssetScore;
	}
	// Cities that they gain
	CitySet const& kVictimConquests = militAnalyst().conqueredCities(eVictim);
	for (CitySetIter it = kVictimConquests.begin(); it != kVictimConquests.end(); ++it)
	{
		City const* pCacheCity = kVictim.uwai().getCache().lookupCity(*it);
		if (pCacheCity == NULL)
			continue;
		if ((eTo == NO_PLAYER || militAnalyst().lostCities(eTo).count(pCacheCity->getID()) > 0) && pCacheCity->city().getTeam() != eIgnoreGains)
		{
			/*	Their cache doesn't account for GP settled in cities of a
				third party (unless espionage visibility), so we
				don't subtract NumGreatPeople.
				I would rather use a score based on our visibility, however,
				the AssetScore from our cache would compute distance maintenance,
				tile culture and trade routes as if we were conquering the city,
				which is far worse.
				The clean solution - computing asset scores for all pairs of
				(oldOwner, newOwner) seems excessive. */
			rNetLoss -= pCacheCity->getAssetScore();
		}
	}
	// Non-city losses (nothing to be gained here) ...
	rNetLoss += lossesFromNukes(eVictim, eTo);
	rTotal.increaseTo(rNetLoss);
	scaled const rBlockadeMultiplier = lossesFromBlockade(eVictim, eTo);
	scaled const rFromBlockade = fixp(0.1) * rBlockadeMultiplier * (rTotal - rNetLoss);
	rNetLoss += rFromBlockade;
	rNetLoss += lossesFromFlippedTiles(eVictim, eTo); // advc.035
	// <!-- custom: Flipped-tile loss is an exclusive-radius land-asset category, so include that category's full value in the denominator rather than comparing its losses with city assets alone. See KI#456. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	rTotal += flippedTileAssetScore(eVictim);
	if (prTotalScore != NULL)
		*prTotalScore = rTotal;
	return rNetLoss;
}


scaled WarUtilityAspect::lossesFromBlockade(PlayerTypes eVictim, PlayerTypes eTo) const
{
	PROFILE_FUNC();
	// Blockades can be painful for maritime empires ...
	CvPlayerAI const& kVictim = GET_PLAYER(eVictim);
	scaled rTotalEnemyFleetPow;
	for (PlayerIter<MAJOR_CIV> itEnemy; itEnemy.hasNext(); ++itEnemy)
	{
		PlayerTypes const eEnemy = itEnemy->getID();
		if (!militAnalyst().isWar(eVictim, eEnemy) ||
			(eTo != NO_PLAYER && eTo != eEnemy))
		{
			continue;
		}
		// Cheat; see UWAICache::updateRelativeNavyPower.
		MilitaryBranch const& kEnemyFleet = *GET_PLAYER(eEnemy).uwai().getCache().
				getPowerValues()[FLEET];
		// I don't think blockading at war happens much before Frigates
		//if (!kEnemyFleet.canBombard()) // Let's cheat a little less (use eObserver arg)
		UnitTypes const eTypicalEnemyWarShip = kEnemyFleet.getTypicalUnit(eOurTeam);
		if (eTypicalEnemyWarShip == NO_UNIT ||
			GC.getInfo(eTypicalEnemyWarShip).getBombardRate() <= 0)
		{
			continue;
		}
		scaled rEnemyFleetPow = kEnemyFleet.power() +
				militAnalyst().gainedPower(eEnemy, FLEET);
		if (rEnemyFleetPow > 100) // Unlikely to send any ships otherwise
			rTotalEnemyFleetPow += rEnemyFleetPow;
	}
	scaled rEnemyToVictimPowRatio = 2;
	MilitaryBranch const& kVictimFleet = *kVictim.uwai().getCache().
			getPowerValues()[FLEET];
	// Allies of the victim? I suppose they wouldn't necessarily defend its shores.
	scaled rVictimFleetPow = kVictimFleet.power() +
			militAnalyst().gainedPower(eVictim, FLEET);
	if (rVictimFleetPow > 0)
		rEnemyToVictimPowRatio = rTotalEnemyFleetPow / rVictimFleetPow;
	if (rEnemyToVictimPowRatio < fixp(1.25)) // Enemies won't bring all their units
		return 0;
	int iCoastalPop = 0;
	int iTotalPop = 0;
	int iCoastalCities = 0;
	CitySet const& kVictimLosses = militAnalyst().lostCities(eVictim);
	FOR_EACH_CITY(pCity, kVictim)
	{
		if (!pCity->isRevealed(eOurTeam) || kVictimLosses.count(pCity->plotNum()) > 0)
			continue;
		int const iPop = pCity->getPopulation();
		iTotalPop += iPop;
		if (pCity->isCoastal())
		{
			iCoastalPop += iPop;
			iCoastalCities++;
		}
	}
	if (iCoastalCities <= 0)
		return 0;
	scaled rAvgCoastalPop(iCoastalPop, iCoastalCities);
	scaled rAttackFactor = rEnemyToVictimPowRatio - 1;
	rAttackFactor.clamp(0, 1);
	scaled rPopFactor(iCoastalPop, iTotalPop);
	// Small cities tend to work few coastal tiles (b/c land tiles are better)
	scaled rVulnerabilityFactor = scaled::min(1, fixp(0.1) + rAvgCoastalPop / 25);
	return rAttackFactor * rPopFactor * rVulnerabilityFactor;
}


scaled WarUtilityAspect::lossesFromNukes(PlayerTypes eVictim, PlayerTypes eSource) const
{
	int const iVictimCitiesLost = (int)militAnalyst().lostCities(eVictim).size();
	CvPlayerAI const& kVictim = GET_PLAYER(eVictim);
	UWAICache const& kVictimCache = kVictim.uwai().getCache();
	// Mild cheat; the total asset score is a pretty abstract number.
	scaled rScorePerCity = kVictimCache.totalAssetScore() /
			std::max(1, kVictim.getNumCities() - iVictimCitiesLost);
	/*	Assume that cities lost to eSource are more likely to take a hit
		(in the process of being conquered by eSource), and such a hit
		doesn't hurt the victim beyond the loss of the city. The remaining nukes
		are more likely to hit large cities, but some will also hit minor cities
		for tactical reasons, or no cities at all; this ought to even out.
		Also assume that the non-lost core cities are always a bit (1/3) afflicted. */
	scaled rHits = (eSource == NO_PLAYER ? militAnalyst().getNukesSufferedBy(eVictim) :
			militAnalyst().getNukedCities(eSource, eVictim));
	rHits = std::max(fixp(1/3.) * rHits, rHits - iVictimCitiesLost * fixp(0.75));
	scaled rLossRate = fixp(0.35);
	// More pessimistic if we're the victim
	if (eVictim == eWe)
		rLossRate = fixp(0.45);
	scaled r = rHits * (kVictimCache.canScrubFallout() ?
			rLossRate - fixp(0.05) : rLossRate + fixp(0.05)) * rScorePerCity;
	if (r >= fixp(0.5))
	{
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_NUKE_ASSET_LOSS turn=%d agentPlayer=%d victimPlayer=%d sourcePlayer=%d lostScore=%d assetsPerCity=%d citiesHit=%f",
				GC.getGame().getGameTurn(), eWe, eVictim, eSource, r.round(), rScorePerCity.uround(), rHits.getFloat());
	}
	return r;
}

// <!-- custom: Use the same player-apportioned category total for the flipped-tile numerator and netLostAssetScore denominator. This keeps the loss ratio within one coherent asset domain under OWN_EXCLUSIVE_RADIUS=1. See KI#456. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
scaled WarUtilityAspect::flippedTileAssetScore(PlayerTypes eVictim) const
{
	if (!GC.getDefineBOOL(CvGlobals::OWN_EXCLUSIVE_RADIUS))
		return 0;
	TeamTypes const eVictimTeam = TEAMID(eVictim);
	if (GET_TEAM(eVictimTeam).getTotalLand(false) <= 0)
		return 0;
	return scaled(125, MemberIter::count(eVictimTeam));
}


// advc.035:
scaled WarUtilityAspect::lossesFromFlippedTiles(PlayerTypes eVictim, PlayerTypes eTo) const
{
	// <!-- custom: Derive the loss from the same apportioned exclusive-radius category total added to the shared denominator. See KI#456. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	scaled const rTileAssetScore = flippedTileAssetScore(eVictim);
	if (rTileAssetScore <= 0)
		return 0;
	int iVictimLostTiles = 0;
	vector<TeamTypes> aeTo;
	if (eTo == NO_PLAYER)
	{	// (Milit. analyst will filter out invalid teams below)
		for (TeamIter<MAJOR_CIV> it; it.hasNext(); ++it)
			aeTo.push_back(it->getID());
	}
	else aeTo.push_back(TEAMID(eTo));
	UWAICache const& kVictimCache = GET_PLAYER(eVictim).uwai().getCache();
	for (size_t i = 0; i < aeTo.size(); i++)
	{
		if (militAnalyst().isWar(aeTo[i], TEAMID(eVictim)))
			iVictimLostTiles += kVictimCache.numPlotsLostAtWar(aeTo[i]);
	}
	// <!-- custom: Use the explicit victim's team when this helper evaluates a teammate or vassal instead of the aspect rival. The lost-tile cache is team-owned, so compare its significance with one team threshold, normalize it by team land and apportion the result across living member callbacks.
	// Player land and assigned-member count made the same physical loss depend on roster partition. See KI#432 and KI#465. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	TeamTypes const eVictimTeam = TEAMID(eVictim);
	if (std::abs(iVictimLostTiles) <= 4)
		return 0;
	int const iVictimLandTiles = GET_TEAM(eVictimTeam).getTotalLand(false);
	// <!-- custom: Apply the lost share of team land to that matching category total instead of manufacturing a numerator on a different scale. See KI#456. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	return rTileAssetScore * iVictimLostTiles / std::max(5, iVictimLandTiles);
}


scaled WarUtilityAspect::conqAssetScore(bool bMute) const
{
	if (ourConquestsFromThem().empty())
		return 0;
	scaled r;
	for (size_t i = 0; i < ourConquestsFromThem().size(); i++)
	{
		City const& kCacheCity = *ourCache().lookupCity(ourConquestsFromThem()[i]);
		// Capture gold negligible b/c it gets reduced if a city is young
		if (kCacheCity.city().isAutoRaze(eWe))
			continue;
		r += kCacheCity.getAssetScore();
	}
	if (r <= 0)
		return 0;
	/*	A little extra for buildings that may survive and things that are there
		but invisible to us. */
	r *= fixp(1.1);
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted() && !bMute);
	// <!-- custom: Combine the inherited city-count and asset-score rows into one event: both describe this same pre-culture-adjustment result, so separate rows only repeat their context. (GPT-6.1-Sol) -->
	if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_CONQUEST_ASSET_TOTAL turn=%d agentPlayer=%d rivalPlayer=%d cityCount=%d assetScore=%d",
			GC.getGame().getGameTurn(), eWe, eThey, (int)ourConquestsFromThem().size(), r.round());
	/*	Reduce score to account for culture pressure from the current owner unless
		we expect that civ to eliminated or to be our vassal.
		Tbd.: Would better to apply this per area, i.e. no culture pressure if we
		take all their cities in one area. Then again, we may not know about
		all their cities in the area. */
	int const iTheirRemainingCities = kThey.getNumCities()
			- (int)militAnalyst().lostCities(eThey).size();
	if (militAnalyst().getCapitulationsAccepted(eOurTeam).count(eTheirTeam) == 0 &&
		iTheirRemainingCities > 0)
	{
		/*	Mean of a quotient based just on how many cities we conquer and another
			based on how much of them remains. Then shift that toward 1 a little. */
		r *= (4 * (1 - (scaled(1, 1 + (int)ourConquestsFromThem().size()) +
				scaled(iTheirRemainingCities, kThey.getNumCities())) / 2) + 1) / 5;
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_CONQUEST_ASSET_CULTURE_PRESSURE turn=%d agentPlayer=%d rivalPlayer=%d assetScore=%d remainingRivalCities=%d",
			GC.getGame().getGameTurn(), eWe, eThey, r.round(), iTheirRemainingCities);
	}
	else
	{
		/*	The penalties for the owner's culture applied by asset score are a
			bit high in this case. */
		r *= fixp(1.2);
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_CONQUEST_ASSET_CULTURE_NEUTRALIZED turn=%d agentPlayer=%d rivalPlayer=%d assetScore=%d",
			GC.getGame().getGameTurn(), eWe, eThey, r.round());
	}
	return r;
}


scaled WarUtilityAspect::remainingCityRatio(PlayerTypes ePlayer) const
{
	int const iCurrCities = GET_PLAYER(ePlayer).getNumCities();
	if (iCurrCities <= 0)
		return 0;
	return 1 - scaled((int)militAnalyst().lostCities(ePlayer).size(), iCurrCities);
}


scaled WarUtilityAspect::partnerUtilFromTech() const
{
	// Some overlap with the "Tech Groundbreaker" code in CvPlayerAI::AI_techValue
	if (m_kGame.isOption(GAMEOPTION_NO_TECH_TRADING))
		return 0;
	// How good our and their attitude needs to be at least to allow tech trade
	AttitudeTypes eOurAttitudeThresh = techRefuseThresh(eWe);
	AttitudeTypes eTheirAttitudeThresh = techRefuseThresh(eThey);
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	if (!kThey.isHuman() && !kWe.isHuman() &&
		(towardThem() < eOurAttitudeThresh || towardUs() < eTheirAttitudeThresh))
	{
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_PARTNER_TECH_REJECT_ATTITUDE turn=%d agentPlayer=%d rivalPlayer=%d ourAttitude=%d ourThreshold=%d rivalAttitude=%d rivalThreshold=%d",
			GC.getGame().getGameTurn(), eWe, eThey, towardThem(), eOurAttitudeThresh, towardUs(), eTheirAttitudeThresh);
		return 0;
	}
	int iWeCanOffer = 0;
	int iTheyCanOffer = 0;
	// Don't rely on human research progress
	bool const bCanSeeResearch = (!kThey.isHuman() && kWe.canSeeResearch(eThey));
	FOR_EACH_ENUM2(Tech, eTech)
	{
		bool bWeHaveIt = (kOurTeam.isHasTech(eTech) ||
				kOurTeam.getResearchProgress(eTech) > 0);
		bool bTheyHaveIt = (kTheirTeam.isHasTech(eTech) ||
				(bCanSeeResearch && kTheirTeam.getResearchProgress(eTech) > 0));
		if (bWeHaveIt && !bTheyHaveIt)
			iWeCanOffer++;
		else if (bTheyHaveIt && !bWeHaveIt)
			iTheyCanOffer++;
	}
	if ((iWeCanOffer == 0 || iTheyCanOffer == 0) &&
		std::abs(iWeCanOffer - iTheyCanOffer) > 4)
	{
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_PARTNER_TECH_REJECT_PROGRESS turn=%d agentPlayer=%d rivalPlayer=%d weCanOffer=%d theyCanOffer=%d",
			GC.getGame().getGameTurn(), eWe, eThey, iWeCanOffer, iTheyCanOffer);
		return 0;
	}
	// Humans are good at making tech trades work
	int iHumanExtra = 0;
	if (kWe.isHuman())
		iHumanExtra += 3;
	if (kThey.isHuman())
		iHumanExtra += 3;
	if (bLogWarUtilityDetail && iHumanExtra > 0) logBBAI("UWAI_WAR_UTILITY_PARTNER_TECH_HUMAN_BONUS turn=%d agentPlayer=%d rivalPlayer=%d bonus=%d",
		GC.getGame().getGameTurn(), eWe, eThey, iHumanExtra);
	// Use their commerce to determine potential for future tech trade
	scaled rTheyToUsCommerceRatio = kOurTeam.AI_estimateYieldRate(eThey, YIELD_COMMERCE) /
			(kOurTeam.AI_estimateYieldRate(eWe, YIELD_COMMERCE) + scaled::epsilon());
	if (rTheyToUsCommerceRatio > 1)
		rTheyToUsCommerceRatio.flipFraction();
	scaled r = SQR(rTheyToUsCommerceRatio) * (17 + iHumanExtra);
	scaled rNearFutureTrades = std::min(iWeCanOffer, iTheyCanOffer);
	if (rNearFutureTrades > 1) // Just 1 isn't likely to result in a trade
	{
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_PARTNER_TECH_FORESEEABLE_TRADES turn=%d agentPlayer=%d rivalPlayer=%d tradeCount=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rNearFutureTrades.floor());
		// Humans tend to make trades immediately, and avoid certain techs entirely.
		if (kThey.isHuman())
			rNearFutureTrades /= 2;
		r += fixp(10/3.) * scaled::min(3, rNearFutureTrades);
	}
	if (bLogWarUtilityDetail && r > 0) logBBAI("UWAI_WAR_UTILITY_PARTNER_TECH_UTILITY turn=%d agentPlayer=%d rivalPlayer=%d utility=%d",
		GC.getGame().getGameTurn(), eWe, eThey, r.round());
	/*	The above assumes that tech trade is only inhibited by overlapping research
		and diverging tech rates. If there's also AI distrust, it's worse. */
	if ((!kWe.isHuman() && towardThem() != ATTITUDE_FRIENDLY) ||
		(!kThey.isHuman() && towardUs() != ATTITUDE_FRIENDLY))
	{
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_PARTNER_TECH_DISTRUST_PENALTY turn=%d agentPlayer=%d rivalPlayer=%d",
			GC.getGame().getGameTurn(), eWe, eThey);
		r /= 2;
	}
	if (m_kGame.isOption(GAMEOPTION_NO_TECH_BROKERING))
		r *= fixp(2/3.);
	return r * kWeAI.amortizationMultiplier();
}


AttitudeTypes WarUtilityAspect::techRefuseThresh(PlayerTypes ePlayer) const
{
	// <!-- custom: Resolve the queried player's identity and personality; AdvCiv used the agent's threshold for both sides of a prospective technology trade. See KI#426. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	// Humans are callous
	if (kPlayer.isHuman())
		return ATTITUDE_FURIOUS;
	// The XML value is mostly ANNOYED; they'll trade at one above that.
	return (AttitudeTypes)(GC.getInfo(kPlayer.getPersonalityType()).getTechRefuseAttitudeThreshold() + 1);
}


scaled WarUtilityAspect::partnerUtilFromTrade() const
{
	scaled rGoldVal;
	scaled rTradeValFromGold;
	int iResourceTrades = 0;
	int const iDefaultTimeHorizon = 25;
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	FOR_EACH_DEAL(pDeal)
	{
		if (!pDeal->isBetween(eWe, eThey) || !pDeal->isEverCancelable(eWe))
			continue;
		bool const bGift = (pDeal->getGivesList(eWe).getLength() <= 0);
		// Handle OB and DP separately (AI_dealVal just counts cities for those)
		bool bSkip = false;
		bool bWeReceiveResource = false;
		FOR_EACH_TRADE_ITEM(pDeal->getReceivesList(eWe))
		{
			if (CvDeal::isDual(pItem->m_eItemType))
			{
				bSkip = true;
				continue;
			}
			if (pItem->m_eItemType == TRADE_RESOURCES)
			{
				bWeReceiveResource = true;
				if (bSkip)
					break; // Know everything we need
			}
			else FAssert(bSkip || pItem->m_eItemType == TRADE_GOLD_PER_TURN);
		}
		if (bSkip)
			continue;
		// Count only the first four resource trades
		if (bWeReceiveResource)
		{
			iResourceTrades++;
			int const iMaxResourceTrades = 4;
			// <!-- custom: AdvCiv documented a four-resource cap but its post-increment >= check discarded the fourth trade too. Skip only trades beyond the cap. See KI#427. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			if (iResourceTrades > iMaxResourceTrades)
			{
				if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_PARTNER_TRADE_RESOURCE_CAP turn=%d agentPlayer=%d rivalPlayer=%d maxResourceTrades=%d",
					GC.getGame().getGameTurn(), eWe, eThey, iMaxResourceTrades);
				continue;
			}
		}
		/*	AI_dealVal is supposed to be gold-per-turn, but seems
			a bit high for that; hence divide by 1.5. */
		scaled rDealVal = kWe.AI_dealVal(eThey, pDeal->getReceivesList(eWe)) /
				(fixp(1.5) * GC.getDefineINT(CvGlobals::PEACE_TREATY_LENGTH));
		if (!bWeReceiveResource)
		{
			int const iMaxTradeValFromGold = 40;
			if (rTradeValFromGold + rDealVal > iMaxTradeValFromGold)
			{
				if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_PARTNER_TRADE_GOLD_CAP turn=%d agentPlayer=%d rivalPlayer=%d maxGoldTradeValue=%d",
					GC.getGame().getGameTurn(), eWe, eThey, iMaxTradeValFromGold);
				rDealVal = iMaxTradeValFromGold - rTradeValFromGold;
			}
			rTradeValFromGold += rDealVal;
		}
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_PARTNER_TRADE_DEAL turn=%d agentPlayer=%d rivalPlayer=%d kind=%s perTurnValue=%d gift=%d",
				GC.getGame().getGameTurn(), eWe, eThey, (bWeReceiveResource ? "RESOURCE" : "GOLD"), rDealVal.round(), bGift);
		/*	Similar approach in CvPlayerAI::AI_stopTradingTradeVal, which
			doubles the trade values of gifts. */
		if (!bGift)
			rDealVal /= 2;
		// This actually restores the result of AI_dealVal
		scaled rTimeHorizon = iDefaultTimeHorizon;
		// Don't trust young deals (could be human manipulation)
		int const iTurnsToCancel = pDeal->turnsToCancel();
		if (iTurnsToCancel > 0)
		{
			rTimeHorizon = scaled(iTurnsToCancel + 10, 2);
			if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_PARTNER_TRADE_RECENT_DEAL_HORIZON turn=%d agentPlayer=%d rivalPlayer=%d turnsToCancel=%d timeHorizon=%d",
				GC.getGame().getGameTurn(), eWe, eThey, iTurnsToCancel, rTimeHorizon.round());
		}
		rDealVal *= rTimeHorizon;
		rGoldVal += rDealVal;
	}
	// Based on TradeUtil::calculateTradeRoutes (Python)
	scaled rTradeRouteProfit;
	FOR_EACH_CITY(pCity, kWe)
	{
		for (int i = 0; i < pCity->getTradeRoutes(); i++)
		{
			CvCity const* pTradeCity = pCity->getTradeCity(i);
			if (pTradeCity == NULL)
				continue;
			if (pTradeCity->getOwner() != eThey)
				continue;
			/*	Division accounts for other potential trade partners,
				in the worst case, domestic. */
			rTradeRouteProfit += pCity->calculateTradeProfit(pTradeCity) / fixp(2.5);
		}
	}
	int const iTradeRouteProfitCap = 40;
	if (rTradeRouteProfit > iTradeRouteProfitCap)
	{
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_PARTNER_TRADE_ROUTE_CAP turn=%d agentPlayer=%d rivalPlayer=%d maxPerTurnProfit=%d",
			GC.getGame().getGameTurn(), eWe, eThey, iTradeRouteProfitCap);
		rTradeRouteProfit = iTradeRouteProfitCap;
	}
	// <!-- custom: Report the inherited net-gold and trade-route components together once both are known, avoiding repeated context while retaining the separate cap/deal diagnostics above. (GPT-6.1-Sol) -->
	if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_PARTNER_TRADE_COMPONENTS turn=%d agentPlayer=%d rivalPlayer=%d netGoldValue=%d perTurnRouteProfit=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rGoldVal.round(), rTradeRouteProfit.uround());
	return kWeAI.tradeValToUtility(rGoldVal + rTradeRouteProfit * iDefaultTimeHorizon) *
			kWeAI.amortizationMultiplier();
}


scaled WarUtilityAspect::partnerUtilFromMilitary() const
{
	if ((kThey.isHuman() || towardUs() < ATTITUDE_FRIENDLY) &&
		!kOurTeam.isDefensivePact(eTheirTeam))
	{
		return 0;
	}
	scaled r = kThey.uwai().getCache().getPowerValues()[ARMY]->power() /
			(ourCache().getPowerValues()[ARMY]->power() + scaled::epsilon());
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_PARTNER_MILITARY_RATIO turn=%d agentPlayer=%d rivalPlayer=%d percent=%d",
		GC.getGame().getGameTurn(), eWe, eThey, r.getPercent());
	/*	Only count their future military support as 10% b/c there are a lot of ifs
		despite friendship/ DP */
	return scaled::min(r * 10, 10);
}


void GreedForAssets::evaluate()
{
	PROFILE_FUNC();
	scaled const rConqScore = conqAssetScore(false);
	if (rConqScore <= 0)
		return;
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	scaled const rPresentScore = ourCache().totalAssetScore();
	/*	(Count non-wonder buildings for rPresentScore? May construct those
		in the conquered cities too eventually ...) */
	scaled rUtility = scaled::min(650, 350 * rConqScore /
			scaled::max(rPresentScore, 1));
	// <!-- custom: The inherited present-score and base-utility lines describe one calculation; keep them in one structured event instead of repeating the same context. (ChatGPT-5.6-Sol) -->
	if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_GREED_ASSETS_BASE turn=%d agentPlayer=%d rivalPlayer=%d conquestAssetScore=%d presentAssetScore=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rConqScore.round(), rPresentScore.round(), rUtility.round());
	scaled const rTeamSzMult = teamSizeMultiplier();
	scaled const rCompetitionMult = competitionMultiplier();
	scaled const rOverextensionMult = overextensionMult();
	scaled const rDefensibilityMult = defensibilityMult();
	// <!-- custom: These four multipliers are applied together to the same base utility; one row preserves the full adjustment state without four near-duplicate context prefixes. (ChatGPT-5.6-Sol) -->
	if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_GREED_ASSETS_MODIFIERS turn=%d agentPlayer=%d rivalPlayer=%d teamSizePercent=%d competitionPercent=%d overextensionPercent=%d defensibilityPercent=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rTeamSzMult.getPercent(), rCompetitionMult.getPercent(),
			rOverextensionMult.getPercent(), rDefensibilityMult.getPercent());
	rUtility *= rCompetitionMult * rTeamSzMult *
			(1 - rOverextensionMult - rDefensibilityMult);
	// Greed shouldn't motivate peaceful leaders too much
	if (kWe.AI_getPeaceWeight() >= 7)
	{
		int iCap = 260 - 10 * kWe.AI_getPeaceWeight();
		if (rUtility > iCap)
		{
			rUtility = iCap;
			if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_GREED_ASSETS_PEACE_WEIGHT_CAP turn=%d agentPlayer=%d rivalPlayer=%d peaceWeight=%d utilityCap=%d",
					GC.getGame().getGameTurn(), eWe, eThey, kWe.AI_getPeaceWeight(), iCap);
		}
	}
	int const iLoveOfPeace = kOurPersonality.getLoveOfPeace();
	if (iLoveOfPeace > 0)
	{
		rUtility *= std::max(fixp(0.1), 1 - per100(iLoveOfPeace));
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_GREED_ASSETS_LOVE_OF_PEACE_REDUCTION turn=%d agentPlayer=%d rivalPlayer=%d percent=%d",
				GC.getGame().getGameTurn(), eWe, eThey, iLoveOfPeace);
	}
	/*	Cap utility per conquered city. Relevant mostly for One-City
		Challenge, but may also matter on dense maps in the early game. */
	rUtility.decreaseTo(120 * ourConquestsFromThem().size());
	m_iU += std::max(0, rUtility.round());
}


scaled GreedForAssets::overextensionMult() const
{
	/*	Number-of-city maintenance and increased civic upkeep from conquered cities
		are difficult to predict; only partially covered by AssetScore.
		Look at our current expenses: are we already overextended? */
	// Don't mind paying 25% for city maintenance
	scaled r = fixp(-0.25);
	scaled const rOurIncome = kOurTeam.AI_estimateYieldRate(eWe, YIELD_COMMERCE);
	if (rOurIncome > 0)
	{
		// Expenses excluding unit cost and supply
		scaled rOurMaintenance = per100(kWe.calculateInflationRate() + 100) *
				(kWe.getTotalMaintenance() + kWe.getCivicUpkeep(NULL, true));
		r += rOurMaintenance / rOurIncome;
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_GREED_ASSETS_OVEREXTENSION turn=%d agentPlayer=%d rivalPlayer=%d maintenance=%d income=%d maintenancePercent=%d",
				GC.getGame().getGameTurn(), eWe, eThey, rOurMaintenance.uround(), rOurIncome.uround(),
				(rOurMaintenance / rOurIncome).getPercent());
	}
	// Can happen in later-era start if a civ immediately changes civics/ religion
	else if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_GREED_ASSETS_OVEREXTENSION_INCOME_UNAVAILABLE turn=%d agentPlayer=%d rivalPlayer=%d",
			GC.getGame().getGameTurn(), eWe, eThey);
	return scaled::max(0, r);
}


scaled GreedForAssets::defensibilityMult() const
{
	// Check for hostile third parties
	scaled const rRemoteness = medianDistFromOurConquests(eWe);
	scaled rThreatFactor;
	for (PlayerIter<MAJOR_CIV> it; it.hasNext(); ++it)
	{
		rThreatFactor += threatToCities(it->getID(), rRemoteness);
	}
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	if (rRemoteness > 5 && !m_kGame.isOption(GAMEOPTION_NO_BARBARIANS) &&
		m_rGameEraAIFactor < fixp(1.5))
	{
		scaled rBarbarianThreat = 1;
		if (m_kGame.isOption(GAMEOPTION_RAGING_BARBARIANS))
			rBarbarianThreat *= fixp(1.5);
		rBarbarianThreat *= rRemoteness / 60;
		rBarbarianThreat.decreaseTo(fixp(0.25));
		if (rBarbarianThreat >= fixp(0.005))
		{
			if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_GREED_ASSETS_DEFENSIBILITY_BARBARIAN_THREAT turn=%d agentPlayer=%d rivalPlayer=%d percent=%d",
					GC.getGame().getGameTurn(), eWe, eThey, rBarbarianThreat.getPercent());
		}
		rThreatFactor += rBarbarianThreat;
	}
	if (rThreatFactor <= 0)
		return 0;
	// A little arcane
	int const iMaxWarMinAdjLand = kOurPersonality.getMaxWarMinAdjacentLandPercent();
	int iPersonalityPercent = 12 + 3 * iMaxWarMinAdjLand;
	/*	In BtS, 0 has a special role b/c it ignores shared borders entirely.
		That's a bit too extreme, but I'm setting it extra low.
		4 is rare (Liz, SiBull, Toku); I'm reinforcing that a bit as well. */
	if (iMaxWarMinAdjLand == 0)
		iPersonalityPercent /= 2;
	if (iMaxWarMinAdjLand >= 3)
		iPersonalityPercent += 5;
	if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_GREED_ASSETS_DEFENSIBILITY_SUMMARY turn=%d agentPlayer=%d rivalPlayer=%d personalityPercent=%d maxWarMinAdjacentLandPercent=%d threatPercent=%d",
			GC.getGame().getGameTurn(), eWe, eThey, iPersonalityPercent, iMaxWarMinAdjLand, rThreatFactor.getPercent());
	return std::min(fixp(0.5), rThreatFactor.sqrt() * per100(iPersonalityPercent));
}


scaled GreedForAssets::threatToCities(PlayerTypes ePlayer, scaled rRemoteness) const
{
	CvPlayerAI& kPlayer = GET_PLAYER(ePlayer);
	if (kPlayer.isAVassal() || !kOurTeam.isHasMet(kPlayer.getTeam()) ||
		kPlayer.getTeam() == eOurTeam || kPlayer.getTeam() == eTheirTeam ||
		!kPlayer.hasCapital() || GET_TEAM(kPlayer.getTeam()).AI_isAvoidWar(eOurTeam))
	{
		return 0;
	}
	// Add a constant b/c power ratios can be volatile in the early game
	scaled rPlayerPower = 150 +
			kPlayer.uwai().getCache().getPowerValues()[ARMY]->power();
	scaled rOurPower = 150 +
			ourCache().getPowerValues()[ARMY]->power();
	// Use present power if kPlayer not part of the analysis
	if (militAnalyst().isPartOfAnalysis(ePlayer))
	{
		rPlayerPower += militAnalyst().gainedPower(kPlayer.getID(), ARMY);
		rOurPower += militAnalyst().gainedPower(eWe, ARMY);
	}
	/*	Skip civs that aren't currently a threat.
		Add some utility for weak civs? Could use the conquered cities as a
		bridgehead for a later war. Too uncertain I think. */
	if (2 * rOurPower > 3 * rPlayerPower)
		return 0;
	// Only worry about civs that are closer to our conquests than we are
	scaled rPlayerDist = medianDistFromOurConquests(ePlayer);
	if (5 * rPlayerDist >= 4 * rRemoteness || rPlayerDist > 10)
		return 0;
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_GREED_ASSETS_DEFENSIBILITY_RIVAL turn=%d agentPlayer=%d rivalPlayer=%d threateningPlayer=%d threateningDistance=%d agentDistance=%d",
			GC.getGame().getGameTurn(), eWe, eThey, kPlayer.getID(), rPlayerDist.uround(), rRemoteness.uround());
	scaled rPowerRatio = scaled::max(0,
			rPlayerPower / scaled::max(10, rOurPower) - fixp(0.1));
	return SQR(rPowerRatio);
}


scaled GreedForAssets::medianDistFromOurConquests(PlayerTypes ePlayer) const
{
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	vector<scaled> arDistances;
	for (size_t i = 0; i < ourConquestsFromThem().size(); i++)
	{
		/*	For a human, it's almost always easy to tell whether one civ is
			closer to a set of cities than another, so the AI figuring it out
			through (possibly) a vision cheat isn't an issue for me. */
		City* pCacheCity = kPlayer.uwai().getCache().lookupCity(
				ourConquestsFromThem()[i]);
		if (pCacheCity == NULL)
			continue;
		/*	OK to use enormous distances for cities that shouldn't affect defensibility;
			median value can handle outliers. */
		// <!-- custom: This defensibility comparison is explicitly overland, so a faster sea route must not shorten the rival's distance. See KI#591. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		int iDist = (pCacheCity->canReachByLand() ? pCacheCity->getDistanceByLand() : 100);
		if (iDist < 0) // unreachable
			iDist = 1000;
		arDistances.push_back(iDist);
	}
	scaled r = 1000;
	if (!arDistances.empty())
		r = stats::median(arDistances);
	return r;
}


scaled GreedForAssets::competitionMultiplier() const
{
	// (Shouldn't have to compute this otherwise)
	FAssert(ourConquestsFromThem().size() > 0);
	int const iTheirCities = kThey.getNumCities();
	/*	Only need this in the early game (I hope), and only if there are
		few cities to go around. */
	if (m_kGame.gameTurnProgress() >= fixp(0.25) || iTheirCities >= 5 ||
		iTheirCities <= 0 || ourConquestsFromThem().size() <= 0)
	{
		return 1;
	}
	/*	Milit. analysis already takes our war allies into account, and tells us
		which cities the allies conquer, and which ones we get. However, this is
		based on just one simulation trajectory, and if they (the target) have
		few cities, we might actually get none at all. Compute a penalty for
		our GreedForAssets utility in order to account for that risk.
		This should reduce early-game dogpiling. */
	int iCompetitors = 0;
	for (PlayerIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itAlly(eOurTeam);
		itAlly.hasNext(); ++itAlly)
	{
		CvPlayer const& kAlly = *itAlly;
		if (!militAnalyst().isWar(kAlly.getID(), eThey))
			continue;
		/*	Only worry if MilitaryAnalyst says that kAlly conquers
			at least as much as we do */
		int iAllyConqFromThem = 0;
		CitySet const& kAllyConquers = militAnalyst().conqueredCities(kAlly.getID());
		for (CitySetIter it = kAllyConquers.begin(); it != kAllyConquers.end(); ++it)
		{
			if (militAnalyst().lostCities(eThey).count(*it) > 0)
				iAllyConqFromThem++;
		}
		if (iAllyConqFromThem >= (int)ourConquestsFromThem().size())
			iCompetitors++;
	}
	FAssert(iTheirCities > iCompetitors); // Because we conquer at least 1 as well
	return scaled::max(0, 1 - scaled(iCompetitors, iTheirCities));
}


scaled GreedForAssets::teamSizeMultiplier() const
{
	/*	To account for team-on-team warfare having a somewhat slower pace than
		player-on-player, meaning that further conquests tend to happen after the
		time horizon of the simulation. (A longer horizon could be too costly
		computationally.) And to account for military assistance within a team
		being a bit underestimated by InvasionGraph (which can't be helped). */
	int iTeamSz = std::min(kOurTeam.getAliveCount(), kTheirTeam.getAliveCount());
	return std::min(fixp(1.43), (1 + scaled(iTeamSz).sqrt()) / 2);
}


void GreedForVassals::evaluate()
{
	TeamSet const& kNewVassals = militAnalyst().getCapitulationsAccepted(eOurTeam);
	if (kNewVassals.count(eTheirTeam) <= 0 ||
		!kWe.hasCapital() || !kThey.hasCapital()) // Will need the capitals later
	{
		return;
	}
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	scaled rOurIncome = kOurTeam.AI_estimateYieldRate(eWe, YIELD_COMMERCE);
	rOurIncome.increaseTo(8);
	/*	Their commerce may be lower than now at the end of the war.
		(Don't try to estimate our own post-war commerce though.) */
	scaled const rTheirCityRatio = remainingCityRatio(eThey);
	// Vassal commerce to account for tech they might research for us
	scaled const rVassalIncome = kOurTeam.AI_estimateYieldRate(eThey, YIELD_COMMERCE);
	scaled const rVassalToOurIncome = rVassalIncome * rTheirCityRatio / rOurIncome;
	scaled rUtilityFromTechTrade = 100 * rVassalToOurIncome;
	// Tech they already have
	scaled const rTechScore = ourCache().vassalTechScore(eThey);
	rUtilityFromTechTrade += kWeAI.tradeValToUtility(rTechScore);
	// <!-- custom: City retention, relative income and existing tech jointly form the same pre-cap technology/income value; log that state once instead of as four consecutive rows. (ChatGPT-5.6-Sol) -->
	if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_GREED_VASSAL_TECH_INCOME turn=%d agentPlayer=%d rivalPlayer=%d cityRatioPercent=%d vassalIncome=%d agentIncome=%d incomeRatioPercent=%d techScore=%d utilityBeforeCap=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rTheirCityRatio.getPercent(), rVassalIncome.uround(), rOurIncome.uround(),
			rVassalToOurIncome.getPercent(), rTechScore.uround(), rUtilityFromTechTrade.uround());
	/*	If they're much more advanced than us, we won't be able to trade for
		all their tech. */
	rUtilityFromTechTrade.decreaseTo(100);
	/*	Reduced a lot b/c of uncertainty and delays in tech trades. Expect human
		master to fare a bit better. */
	scaled rUtility = (kOurTeam.isHuman() ? fixp(0.5) : fixp(0.35)) *
			rUtilityFromTechTrade;
	scaled rUtilityFromResources = ourCache().vassalResourceScore(eThey);
	// Expect just +1.5 commerce per each of their cities from trade routes
	scaled rUtilityFromTR = fixp(1.5) * kThey.getNumCities() / rOurIncome;
	// <!-- custom: Resources and trade routes are combined immediately below as the same passive vassal-trade contribution, so keep their component scores on one row. (ChatGPT-5.6-Sol) -->
	if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_GREED_VASSAL_PASSIVE_TRADE turn=%d agentPlayer=%d rivalPlayer=%d resourceScore=%d tradeRouteScore=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rUtilityFromResources.uround(), rUtilityFromTR.uround());
	// These trades are pretty safe bets; treated as 75% probable
	rUtility += fixp(0.75) * rTheirCityRatio * (rUtilityFromResources + rUtilityFromTR);
	int iOurVassalPlayers = PlayerIter<ALIVE,VASSAL_OF>::count(eOurTeam);
	for (TeamSetIter it = kNewVassals.begin(); it != kNewVassals.end(); ++it)
		iOurVassalPlayers += GET_TEAM(*it).getNumMembers();
	// Diminishing returns from having additional trade partners
	rUtility /= scaled(iOurVassalPlayers).sqrt();
	CvArea const& kOurArea = kWe.getCapital()->getArea();
	CvArea const& kTheirArea = kThey.getCapital()->getArea();
	bool bUsefulArea = false; // Look for a future enemy to dogpile on
	if (&kOurArea == &kTheirArea)
	{
		for (PlayerAIIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itTarget(eOurTeam);
			itTarget.hasNext(); ++itTarget)
		{
			CvPlayerAI const& kTarget = *itTarget;
			CvTeamAI const& kTargetTeam = GET_TEAM(kTarget.getTeam());
			if (3 * kTarget.getNumCities() <= std::max(1, kWe.getNumCities()) ||
				militAnalyst().isEliminated(kTarget.getID()) ||
				militAnalyst().hasCapitulated(kTargetTeam.getID()))
			{
				continue;
			}
			FAssert(kNewVassals.count(kTargetTeam.getID()) <= 0);
			if (kTarget.getCapital()->isArea(kOurArea) &&
				(!kTargetTeam.AI_isAvoidWar(eOurTeam) ||
				!kOurTeam.AI_isAvoidWar(kTargetTeam.getID(), true)))
			{
				bUsefulArea = true;
				break;
			}
		}
	}
	scaled const rTheirPower = kThey.uwai().getCache().getPowerValues()[ARMY]->power();
	scaled const rOurPower = ourCache().getPowerValues()[ARMY]->power();
	// Multiplier reflects coordination problems and lack of commitment
	scaled rUtilityFromMilitary = 45 * rTheirCityRatio * rTheirPower /
			scaled::max(rOurPower, 10);
	// <!-- custom: The power ratio exists only to derive this military base utility; keep both inputs and the resulting utility in one event. (ChatGPT-5.6-Sol) -->
	if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_GREED_VASSAL_MILITARY_BASE turn=%d agentPlayer=%d rivalPlayer=%d vassalPower=%d agentPower=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rTheirPower.round(), rOurPower.round(), rUtilityFromMilitary.uround());
	if (!bUsefulArea)
	{
		rUtilityFromMilitary /= 2;
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_GREED_VASSAL_MILITARY_AREA_REDUCTION turn=%d agentPlayer=%d rivalPlayer=%d percent=50",
				GC.getGame().getGameTurn(), eWe, eThey);
	}
	rUtilityFromMilitary.decreaseTo(30);
	rUtility += rUtilityFromMilitary;
	if (!kWe.AI_atVictoryStage(AI_VICTORY_CONQUEST3))
		rUtility -= 5; // To account for the diplo penalty from capitulated vassals
	int iLoveOfPeace = kOurPersonality.getLoveOfPeace();
	if (iLoveOfPeace > 0)
	{
		rUtility *= std::max(fixp(0.1), 1 - per100(iLoveOfPeace));
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_GREED_VASSAL_LOVE_OF_PEACE_REDUCTION turn=%d agentPlayer=%d rivalPlayer=%d percent=%d",
				GC.getGame().getGameTurn(), eWe, eThey, iLoveOfPeace);
	}
	m_iU += std::max(0, rUtility.round());
}


// <!-- custom: The aspect object is reused across living agent teammates; reset rival-cache dedup for each member; physical ownership below prevents cross-member double counting without leaking first-member state. See KI#1050. (GPT-5.6-Sol) -->
int GreedForSpace::preEvaluate()
{
	m_countedSites.clear();
	return 0;
}


// <!-- custom: Assign each released physical site to the living agent teammate whose existing city is nearest, with player iteration order as a stable tie-break.
// This is a deliberate map-based ownership rule rather than letting whichever teammate is evaluated first capture the opportunity. See KI#1050. (GPT-5.6-Sol) -->
PlayerTypes GreedForSpace::siteOwner(CvPlot const& kSite) const
{
	PlayerTypes eBestOwner = NO_PLAYER;
	int iBestDistance = MAX_INT;
	for (MemberIter itMember(eOurTeam); itMember.hasNext(); ++itMember)
	{
		FOR_EACH_CITY(pCity, *itMember)
		{
			int const iDistance = plotDistance(&kSite, pCity->plot());
			if (iDistance < iBestDistance)
			{
				iBestDistance = iDistance;
				eBestOwner = itMember->getID();
			}
		}
	}
	return eBestOwner;
}


// <!-- custom: Greed for Space concerns a team war opportunity; retain AdvCiv's adjacency gate, but accept it when any living agent teammate borders the eliminated rival so the deterministically assigned owner is not suppressed merely because another teammate provides the shared access. See KI#1050. (GPT-5.6-Sol) -->
bool GreedForSpace::agentTeamAdjacentTo(PlayerTypes eRival) const
{
	for (MemberIter itMember(eOurTeam); itMember.hasNext(); ++itMember)
	{
		// <!-- custom: MemberIter exposes CvPlayer, where calling uwai() failed to compile; recover CvPlayerAI through GET_PLAYER(member ID) before accessing its UWAI cache. See KI#1050. (GPT-5.6-Sol) -->
		if (GET_PLAYER(itMember->getID()).uwai().getCache().numAdjacentLandPlots(eRival) > 0)
			return true;
	}
	return false;
}


void GreedForSpace::evaluate()
{
	// If they lose cities, but aren't eliminated, they'll try to settle sites aggressively; doesn't help us.
	// <!-- custom: Replace the evaluating player's adjacency test with the coordinated agent-team test because another teammate can provide access to the shared opportunity. See KI#1050. (GPT-5.6-Sol) -->
	if (!militAnalyst().isEliminated(eThey) || !agentTeamAdjacentTo(eThey))
	{
		return;
	}
	int const iOurCities = std::max(1, kWe.getNumCities());
	int const iOurSites = std::min(iOurCities, kWe.AI_getNumCitySites());
	int iTheirSites = 0;
	// Would be too brazen a cheat to ignore visibility here. (And might encourage very early wars.)
	for (int i = 0; i < kThey.AI_getNumCitySites(); i++)
	{
		CvPlot const& kSite = kThey.AI_getCitySite(i);
		// <!-- custom: Track visibility without incrementing immediately so the local plot-identity set rejects duplicate rival recommendations and deterministic nearest-city ownership admits the site for only one agent teammate. See KI#454 and KI#1050. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		bool bVisible = kSite.isRevealed(eOurTeam);
		if (!bVisible)
		{
			FOR_EACH_ADJ_PLOT(kSite)
			{
				if (pAdj->isRevealed(eOurTeam))
				{
					bVisible = true;
					break;
				}
			}
		}
		// <!-- custom: Count the site only for its deterministic agent owner, then reject repeated recommendations from this member's rival caches. See KI#454 and KI#1050. (GPT-5.6-Sol) -->
		if (bVisible && siteOwner(kSite) == eWe && m_countedSites.insert(kSite.plotNum()).second)
			iTheirSites++;
	}
	// Expect to raze only when we have to
	for (size_t i = 0; i < ourConquestsFromThem().size(); i++)
	{
		if (ourCache().lookupCity(ourConquestsFromThem()[i])->city().isAutoRaze(eWe))
		{
			iTheirSites++;
		}
	}
	/*	All sites could still be claimed by third parties or could be just barely
		worth settling. Their sites are extra uncertain because they could be far
		away from our capital. */
	scaled rExpectedSites = iOurSites + fixp(0.63) * iTheirSites;
	// Don't expect to more than double our territory in the foreseeable future
	rExpectedSites.decreaseTo(iOurCities);
	// Increase in sites relative to our current city count
	scaled rIncr = (rExpectedSites - iOurSites) / iOurCities;
	FAssert(rIncr >= 0 && rIncr <= 1);
	scaled rUtility = (40 * rIncr * kWeAI.amortizationMultiplier());
	if (rUtility.abs() >= fixp(0.5))
	{
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_GREED_SPACE_SITE_GAIN turn=%d agentPlayer=%d rivalPlayer=%d orphanedSites=%d currentSites=%d currentCities=%d utility=%d",
				GC.getGame().getGameTurn(), eWe, eThey, iTheirSites, iOurSites, iOurCities, rUtility.round());
		m_iU += rUtility.round();
	}
}


void GreedForCash::evaluate()
{
	if (militAnalyst().isEliminated(eThey) ||
		ourCache().numReachableCities(eThey) <= 0)
	{
		return;
	}
	int const iTheyLoseToUs = ourConquestsFromThem().size();
	if (iTheyLoseToUs <= 0)
		return;
	int iWeLoseToThem = 0;
	CitySet const& kTheyConquer = militAnalyst().conqueredCities(eThey);
	for (CitySetIter it = kTheyConquer.begin(); it != kTheyConquer.end(); ++it)
	{
		if (militAnalyst().lostCities(eWe).count(*it) > 0)
		{
			iWeLoseToThem++;
			if (iWeLoseToThem >= iTheyLoseToUs)
				return;
		}
	}
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_GREED_CASH_REPARATIONS turn=%d agentPlayer=%d rivalPlayer=%d",
			GC.getGame().getGameTurn(), eWe, eThey);
	m_iU += normalizeUtility(4).uround(); // Only one team member will pay
}


void Loathing::evaluate()
{
	PROFILE_FUNC();
	if (kWe.isHuman())
		return;
	/*	Only count this if at war with them? No, that could lead to us joining
		a war despite not contributing anything to their losses.
		Can't tell who causes their losses. */
	if ((towardThem() > ATTITUDE_FURIOUS &&
		(kOurTeam.isAtWar(eTheirTeam) ||
		/*	I.e. only loath them if a) furious or b) at peace and worst enemy.
			Rationale: When at war, they'll usually become our worst enemy
			(and I don't want loathing to count virtually always once at war),
			whereas Furious attitude is not a matter of course even when at war. */
		kOurTeam.AI_getWorstEnemy() != eTheirTeam)) ||
		kTheirTeam.isCapitulated() || // Capitulated is as good as dead
		// Don't commit suicide just to inconvenience an enemy
		militAnalyst().isEliminated(eWe))
	{
		return;
	}
	int const iVengefulness = kWeAI.vengefulness();
	if (iVengefulness == 0)
	{
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_LOATHING_DISABLED turn=%d agentPlayer=%d rivalPlayer=%d reason=NO_VENGEFULNESS",
				GC.getGame().getGameTurn(), eWe, eThey);
		return;
	}
	scaled rLossRating = lossRating();
	rLossRating.clamp(-1, 1);
	if (rLossRating.abs() <= 0)
		return;
	rLossRating *= 100;
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_LOATHING_BASE turn=%d agentPlayer=%d rivalPlayer=%d lossRatingPercent=%d vengefulness=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rLossRating.round(), iVengefulness);
	// Utility proportional to iVengefulness and rLossRating would be too extreme
	scaled rFromLosses = fixp(10/3.) * (iVengefulness * rLossRating.abs()).sqrt();
	if (rLossRating.isNegative())
		rFromLosses.flipSign();
	// We like to bring in war allies against our nemesis
	int iJointDoW = 0;
	PlyrSet const& kWarsDeclaredOnThem = militAnalyst().getWarsDeclaredOn(eThey);
	for (PlayerIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itAlly(eOurTeam);
		itAlly.hasNext(); ++itAlly)
	{
		if (kWarsDeclaredOnThem.count(itAlly->getID()) > 0)
		{
			if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_LOATHING_JOINT_DOW turn=%d agentPlayer=%d rivalPlayer=%d allyPlayer=%d",
					GC.getGame().getGameTurn(), eWe, eThey, itAlly->getID());
			iJointDoW++;
		}
	}
	scaled rFromDiplo = 10 * scaled(iVengefulness).sqrt() * iJointDoW;
	int iFreeCivPlayers = countFreeRivals<false>();
	if (!kOurTeam.isAVassal())
		iFreeCivPlayers += kOurTeam.getNumMembers();
	/*	Obsessing over one enemy is unwise if there are many potential enemies.
		Rounded down b/c it would be a bit too high otherwise. */
	int const iFreeCivDivisor = scaled(std::max(1, iFreeCivPlayers)).sqrt().floor();
	int const iUtility = ((rFromLosses + rFromDiplo) /
			// Count Loathing only once per team member
			(iFreeCivDivisor * kOurTeam.getNumMembers())).round();
	if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_LOATHING_RESULT turn=%d agentPlayer=%d rivalPlayer=%d lossUtility=%d jointDoWCount=%d diplomacyUtility=%d freeRivalPlayers=%d divisor=%d teamMembers=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rFromLosses.round(), iJointDoW, rFromDiplo.round(), iFreeCivPlayers,
			iFreeCivDivisor, kOurTeam.getNumMembers(), iUtility);
	m_iU += iUtility;
}

/*	How much they're losing compared with what we have. I.e. we don't much care
	for kicking them when they're down. */
scaled Loathing::lossRating() const
{
	if (militAnalyst().isEliminated(eThey) ||
		militAnalyst().hasCapitulated(eTheirTeam))
	{
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_LOATHING_LOSS_RATING_SCORE turn=%d agentPlayer=%d rivalPlayer=%d rivalScore=%d agentScore=%d",
				GC.getGame().getGameTurn(), eWe, eThey, m_kGame.getPlayerScore(eThey), m_kGame.getPlayerScore(eWe));
		return scaled(m_kGame.getPlayerScore(eThey),
				std::max(10, m_kGame.getPlayerScore(eWe)));
	}
	/*	We mainly care about how much they lose relative to what we have; only
		use their present status as a sanity check, so that we don't get excited
		if we're doing badly and they suffer some minor losses. */
	scaled rTheirAssets;
	// <!-- custom: The generalized asset helper now requires its victim explicitly; Loathing still evaluates the current rival eThey. See KI#432. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	scaled rTheirLostAssets = netLostAssetScore(eThey, NO_PLAYER, &rTheirAssets);
	rTheirAssets.increaseTo(1);
	CitySet const& kTheyLose = militAnalyst().lostCities(eThey);
	for (CitySetIter it = kTheyLose.begin(); it != kTheyLose.end(); ++it)
	{
		City const* pCacheCity = kThey.uwai().getCache().lookupCity(*it);
		if (pCacheCity == NULL)
			continue;
		/*	Already included in theirLostScore, but count it once more for the
			symbolic value. */
		if (pCacheCity->city().isCapital())
			rTheirLostAssets += 4;
	}
	scaled const rOurAssets = scaled::max(1, ourCache().totalAssetScore());
	scaled rTheirAssetsToOurs = scaled::min(rTheirLostAssets / rOurAssets,
			3 * rTheirLostAssets / rTheirAssets);
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	if (bLogWarUtilityDetail && rTheirAssetsToOurs >= fixp(0.005)) logBBAI("UWAI_WAR_UTILITY_LOATHING_LOSS_RATING_ASSETS turn=%d agentPlayer=%d rivalPlayer=%d rivalLostAssets=%d rivalPresentAssets=%d agentPresentAssets=%d assetRatioPercent=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rTheirLostAssets.uround(), rTheirAssets.uround(), rOurAssets.uround(),
			rTheirAssetsToOurs.getPercent());
	/*	This is mostly about their losses, and not ours, but we shouldn't be
		satisfied to have weakened their army if ours fares far worse.
		Slightly weird side-effect: Peace scenario can have higher
		utility for Loathing even if we only fight against other civs. */
	scaled rTheirMinusOurLostPower = scaled::max(0,
			// Never mind losses in HOME_GUARD; LOGISTICS is included in FLEET.
			militAnalyst().lostPower(eThey, ARMY) + militAnalyst().lostPower(eThey, FLEET)
			- fixp(0.5) *
			(militAnalyst().lostPower(eWe, ARMY) + militAnalyst().lostPower(eWe, FLEET)));
	vector<MilitaryBranch*> const& kOurMilitary = ourCache().
			getPowerValues();
	vector<MilitaryBranch*> const& kTheirMilitary = kThey.uwai().getCache().
			getPowerValues();
	/*	Future power is a better point of reference; AI build-up during war
		preparations and reinforcements throughout a war tend to dwarf
		present military power. */
	scaled const rOurPower = kOurMilitary[ARMY]->power() + kOurMilitary[FLEET]->power() +
			militAnalyst().gainedPower(eWe, ARMY) + militAnalyst().gainedPower(eWe, FLEET);
	scaled const rTheirPower = kTheirMilitary[ARMY]->power() + kTheirMilitary[FLEET]->power() +
			militAnalyst().gainedPower(eThey, ARMY) + militAnalyst().gainedPower(eThey, FLEET);
	scaled const rTheirLostPowRatio = std::min(
			rTheirMinusOurLostPower / (rOurPower + scaled::epsilon()),
			3 * rTheirMinusOurLostPower / (rTheirPower + scaled::epsilon()));
	if (bLogWarUtilityDetail && rTheirLostPowRatio.abs() > fixp(0.005)) logBBAI("UWAI_WAR_UTILITY_LOATHING_LOSS_RATING_POWER turn=%d agentPlayer=%d rivalPlayer=%d adjustedRivalMinusAgentLostPower=%d rivalProjectedPower=%d agentProjectedPower=%d lostPowerRatioPercent=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rTheirMinusOurLostPower.round(), rTheirPower.round(), rOurPower.round(),
			rTheirLostPowRatio.getPercent());
	/*	rTheirLostPowRatio is less important, and also tends to be high b/c peace-time
		armies (rOurPower, rTheirPower) are often small compared to war-time armies. */
	return (2 * rTheirAssetsToOurs +
			fixp(0.5) * std::min(fixp(1.8), rTheirLostPowRatio)) / 3;
	/*	rTheirLostPowRatio is important when we can't conquer any of their cities,
		i.e. in all cases when we loath a stronger opponent.
		Hence all the checks and balances. */
}


void MilitaryVictory::evaluate()
{
	PROFILE_FUNC();
	// Vassals shouldn't strive for victory; not until breaking free.
	if (kOurTeam.isAVassal())
		return;
	scaled rTotalProgressRating;
	scaled rMaxProgress = -1;
	std::vector<scaled> arProgressRatings;
	/*	The distinction between stage 3 and 4 is too coarse to be useful;
		treat them the same way. At stage 2, the AI should focus on
		building up military power; too early for conquering land for the
		sake of the victory thresholds. */
	if (kWe.AI_atVictoryStage(AI_VICTORY_CONQUEST3))
		arProgressRatings.push_back(progressRatingConquest());
	if (kWe.AI_atVictoryStage(AI_VICTORY_DOMINATION3))
		arProgressRatings.push_back(progressRatingDomination());
	// Diplomatic victories can come through military means
	if (kWe.AI_atVictoryStage(AI_VICTORY_DIPLOMACY3))
		arProgressRatings.push_back(progressRatingDiplomacy());
	if (arProgressRatings.empty())
		return;
	for (size_t i = 0; i < arProgressRatings.size(); i++)
	{
		scaled rProgress = arProgressRatings[i];
		rProgress.clamp(0, 1);
		if (rProgress > rMaxProgress)
			rMaxProgress = rProgress;
		rTotalProgressRating += rProgress;
	}
	FAssert(rMaxProgress >= 0);
	int const iFreeRivals = countFreeRivals<false>();
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	if (bLogWarUtilityDetail && rTotalProgressRating > 0) logBBAI("UWAI_WAR_UTILITY_MILITARY_VICTORY_CONTEXT turn=%d agentPlayer=%d rivalPlayer=%d freeRivals=%d victoryConditionCount=%d",
			GC.getGame().getGameTurn(), eWe, eThey, iFreeRivals, (int)arProgressRatings.size());
	if (iFreeRivals <= 0)
		return;
	/*	Division by (sqrt of) iVictories because it's not so useful to pursue
		several victory conditions at once. But don't go below the max of the
		progress values. */
	scaled rProgressRating = std::max(rMaxProgress, rTotalProgressRating /
			scaled(arProgressRatings.size()).sqrt());
	// The fewer rivals remain, the more we're willing to go all-in for victory
	scaled rDivisor = 2 + scaled(iFreeRivals).sqrt();
	scaled rUtility = 540 * rProgressRating / rDivisor;
	/*	May lose votes if nuked. This could be better integrated with the progress
		ratings above. First had it as a separate class, now kind of tacked on. */
	scaled rVoteCost;
	scaled rHitUs = militAnalyst().getNukedCities(eThey, eWe);
	if (rHitUs > 0 &&
		(kWe.AI_atVictoryStage(AI_VICTORY_DIPLOMACY3 |
		/*	The other peaceful victories are similar enough to Diplo;
			don't want our population to be nuked. */
		AI_VICTORY_CULTURE3 | AI_VICTORY_SPACE3) &&
		// Don't fret if we can also win differently
		(!kWe.AI_atVictoryStage(AI_VICTORY_DOMINATION3) &&
		!kWe.AI_atVictoryStage(AI_VICTORY_DOMINATION4) &&
		!kWe.AI_atVictoryStage(AI_VICTORY_CONQUEST3) &&
		!kWe.AI_atVictoryStage(AI_VICTORY_CONQUEST4) &&
		// Probably too late for nukes to stop us then
		!kWe.AI_atVictoryStage(AI_VICTORY_SPACE4) &&
		!kWe.AI_atVictoryStage(AI_VICTORY_CULTURE4))))
	{
		// Slow to grow back if we can't clean up
		scaled rPopLossRate = rHitUs *
				(ourCache().canScrubFallout() ? fixp(0.35) : fixp(0.45)) /
				std::max(1,
				kWe.getNumCities() - (int)militAnalyst().lostCities(eWe).size());
		rPopLossRate.decreaseTo(fixp(0.5));
		rVoteCost += rPopLossRate * 60;
		if (kWe.AI_atVictoryStage(AI_VICTORY_DIPLOMACY4))
			rVoteCost += rPopLossRate * 40;
		if (rVoteCost >= fixp(0.5))
		{
			FErrorMsg("Just sth. to take a look at b/c it seems to come up"
					" very rarely if ever - peaceful victory discouraging nuclear war");
			if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_MILITARY_VICTORY_NUKE_COST turn=%d agentPlayer=%d rivalPlayer=%d utilityCost=%d expectedPopulationLossPercent=%d",
					GC.getGame().getGameTurn(), eWe, eThey, rVoteCost.round(), rPopLossRate.getPercent());
		}
	}
	rUtility -= rVoteCost;
	m_iU += rUtility.round();
}


int MilitaryVictory::preEvaluate()
{
	m_iVotesToGo = kOurTeam.AI_votesToGoForVictory();
	// Rather try to get the last few votes through growth
	m_bEnoughVotes = (m_iVotesToGo < 8);
	return 0;
}


scaled MilitaryVictory::progressRatingConquest() const
{
	if (militAnalyst().isEliminated(eThey) ||
		militAnalyst().getCapitulationsAccepted(eOurTeam).count(eTheirTeam))
	{
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_MILITARY_VICTORY_CONQUEST_COMPLETE turn=%d agentPlayer=%d rivalPlayer=%d",
				GC.getGame().getGameTurn(), eWe, eThey);
		return 1;
	}
	// If we don't take them out entirely - how much do they lose
	scaled rTheirScore;
	// <!-- custom: Conquest progress measures eThey's losses even when our vassals take some of the rival's cities, so keep eThey as the explicit victim for every recipient. See KI#432. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	scaled rTheirLostAssets = netLostAssetScore(eThey, eWe, &rTheirScore);
	if (rTheirScore < 25)
		return 0;
	/*	Utility of vassals is normally computed separately (linked wars),
		but in this case, our vassals may do something that helps the master
		specifically (namely bring down our last remaining rivals). */
	for (PlayerIter<ALIVE,VASSAL_OF> it(eOurTeam); it.hasNext(); ++it)
		rTheirLostAssets += netLostAssetScore(eThey, it->getID());
	if (rTheirLostAssets < 5)
		return 0;
	scaled r = rTheirLostAssets / rTheirScore;
	r.decreaseTo(1);
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_MILITARY_VICTORY_CONQUEST_PROGRESS turn=%d agentPlayer=%d rivalPlayer=%d lostAssetRatioPercent=%d",
			GC.getGame().getGameTurn(), eWe, eThey, r.getPercent());
	/*	Reduced because we can't be sure that we'll eventually finish them off.
		In particular, they could become someone else's vassal. */
	return fixp(2/3.) * r;
}

namespace
{
	VictoryTypes getDominationVictory()
	{
		static VictoryTypes eDomination = NO_VICTORY; // cache it
		if (eDomination == NO_VICTORY)
		{
			// (Same thing is done in CvPlayerAI::AI_calculateDominationVictoryStage.)
			FOR_EACH_ENUM(Victory)
			{
				if (GC.getInfo(eLoopVictory).getLandPercent() > 0)
				{
					eDomination = eLoopVictory;
					break;
				}
			}
		}
		return eDomination;
	}

	// Caller needs to guarantee that Domination victory exists (and is enabled etc.)
	scaled getDominationTargetPopPortion()
	{	// Not something we can cache in a static variable
		return per100(GC.getGame().getAdjustedPopulationPercent(getDominationVictory()));
	}
}

scaled MilitaryVictory::progressRatingDomination() const
{
	VictoryTypes const eDomination = getDominationVictory();
	if (eDomination == NO_VICTORY || militAnalyst().conqueredCities(eWe).empty())
		return 0;
	int const iPopTarget = (getDominationTargetPopPortion() *
			m_kGame.getTotalPopulation()).ceil();
	int const iPopToGo = iPopTarget - kOurTeam.getTotalPopulation();
	scaled const rOurLandPortion(kOurTeam.getTotalLand(), GC.getMap().getLandPlots());
	scaled const rAdjustedLandForDomination = per100(
			m_kGame.getAdjustedLandPercent(eDomination));
	// How close we are to the victory threshold
	scaled rLandVictoryStatus = rOurLandPortion / rAdjustedLandForDomination;
	// Dealing with land tiles is too complicated. Need a target number of cities.
	/*	If rLandVictoryStatus is what our current cities give us, we'll need this
		many for victory: */
	scaled rTargetCities = kOurTeam.getNumCities() /
			(rLandVictoryStatus + scaled::epsilon());
	/*	But the additional cities may well give us less land than our current cities.
		How many cities would we need if the goal was to have a certain fraction
		of cities rather than land tiles? Use the average of these two predictions. */
	rTargetCities += rAdjustedLandForDomination * m_kGame.getNumCities();
	rTargetCities /= 2;
	scaled rCitiesToGo = rTargetCities - kOurTeam.getNumCities();
	scaled rPopGained;
	scaled rCitiesGained;
	CitySet const& kWeConquer = militAnalyst().conqueredCities(eWe);
	CitySet const& kTheyLose = militAnalyst().lostCities(eThey);
	for (CitySetIter it = kWeConquer.begin(); it != kWeConquer.end(); ++it)
	{
		if (kTheyLose.count(*it) == 0)
			continue;
		City const& kCacheCity = *ourCache().lookupCity(*it);
		/*	Minus 0.5 for pop lost upon conquest (not -1 b/c target decreases
			as well) */
		rPopGained += kCacheCity.city().getPopulation() - fixp(0.5);
		rCitiesGained++;
	}
	if (militAnalyst().getCapitulationsAccepted(eOurTeam).count(eTheirTeam) > 0)
	{
		int iTheirPopRemaining = kThey.getTotalPopulation();
		FAssertMsg(!kTheyLose.empty(), "They capitulated w/o losing a city?");
		int const iTheirCitiesRemaining = kThey.getNumCities() - (int)kTheyLose.size();
		for (CitySetIter it = kTheyLose.begin(); it != kTheyLose.end(); ++it)
		{
			iTheirPopRemaining -= ourCache().lookupCity(*it)->city().getPopulation();
		}
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted() && iTheirCitiesRemaining > 0) logBBAI("UWAI_WAR_UTILITY_MILITARY_VICTORY_DOMINATION_VASSAL_GAIN turn=%d agentPlayer=%d rivalPlayer=%d populationGained=%d citiesGained=%d",
				GC.getGame().getGameTurn(), eWe, eThey, iTheirPopRemaining, iTheirCitiesRemaining);
		rCitiesGained += fixp(0.5) * iTheirCitiesRemaining;
		rPopGained += fixp(0.5) * iTheirPopRemaining;
	}
	if (rCitiesGained == 0)
		return 0;
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	// <!-- custom: The inherited report labeled rPopGained as population-to-go. Record the actual threshold remainder and gained values explicitly in one structured row. (ChatGPT-5.6-Sol) -->
	if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_MILITARY_VICTORY_DOMINATION_INPUTS turn=%d agentPlayer=%d rivalPlayer=%d populationToGo=%d citiesToGo=%d populationGained=%d citiesGained=%d",
			GC.getGame().getGameTurn(), eWe, eThey, iPopToGo, rCitiesToGo.round(), rPopGained.uround(), rCitiesGained.uround());
	FAssert(rPopGained > 0);
	// No use in population beyond the victory threshold
	rPopGained.decreaseTo(iPopToGo);
	// <!-- custom: The to-go values can be non-positive. For rCitiesToGo, relatively sparse city borders make it unclear whether the land threshold is truly complete; for iPopToGo, CvGame accepts exact equality with the population threshold.
	// Route equality through the completed-population branch instead of dividing by zero. See KI#428. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (iPopToGo <= 0)
	{
		/*	If rCitiesToGo is non-positive, then boths victory thresholds
			should be met -- but somehow we haven't won.
			Assume that we need a few more cities. */
		int const iMinCitiesToGo = 4;
		if (rCitiesToGo < iMinCitiesToGo)
		{
			if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_MILITARY_VICTORY_DOMINATION_CITY_FLOOR turn=%d agentPlayer=%d rivalPlayer=%d minimumCitiesToGo=%d",
					GC.getGame().getGameTurn(), eWe, eThey, iMinCitiesToGo);
			rCitiesToGo = iMinCitiesToGo;
		}
		rCitiesGained.decreaseTo(rCitiesToGo);
		return rCitiesGained / rCitiesToGo;
	}
	else
	{
		if (rCitiesToGo < 0)
		{
			// Assume that we have enough land.
			return rPopGained / iPopToGo;
		}
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_MILITARY_VICTORY_DOMINATION_MODE turn=%d agentPlayer=%d rivalPlayer=%d mode=BOTH_CITY_AND_POPULATION",
				GC.getGame().getGameTurn(), eWe, eThey);
		return fixp(0.5) * (rCitiesGained / rCitiesToGo + rPopGained / iPopToGo);
	}
}


scaled MilitaryVictory::progressRatingDiplomacy() const
{
	if (m_bEnoughVotes)
	{
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_MILITARY_VICTORY_DIPLOMACY_ENOUGH_VOTES turn=%d agentPlayer=%d rivalPlayer=%d",
				GC.getGame().getGameTurn(), eWe, eThey);
		return 0;
	}
	VoteSourceTypes const eVS = kOurTeam.AI_getLatestVictoryVoteSource();
	if (eVS == NO_VOTESOURCE)
	{	// DIPLO3 should normally rule that out (at the call site)
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_MILITARY_VICTORY_DIPLOMACY_NO_VOTE_SOURCE turn=%d agentPlayer=%d rivalPlayer=%d",
				GC.getGame().getGameTurn(), eWe, eThey);
		return 0;
	}
	ReligionTypes const eVSReligion = m_kGame.getVoteSourceReligion(eVS);
	bool const bSecular = (eVSReligion == NO_RELIGION);
	scaled rPopGained;
	std::map<PlotNumTypes,scaled> weightedConq;
	CitySet conqFriendly;
	CitySet conqPleased;
	CitySet const& kTheyLose = militAnalyst().lostCities(eThey);
	int const iReligionObstacles = kOurTeam.AI_countVSNonMembers(eVS);
	int iReligionObstaclesRemoved = 0;
	/*	Vassal conquests just as good as our own for getting votes.
		Conquests by friends and partners can only help a little (reduced weight). */
	for (CitySetIter it = kTheyLose.begin(); it != kTheyLose.end(); ++it)
	{
		if (militAnalyst().conqueredCities(eWe).count(*it))
			weightedConq.insert(make_pair(*it, 1));
	}
	for (PlayerIter<ALIVE,VASSAL_OF> itVassal(eOurTeam);
		itVassal.hasNext(); ++itVassal)
	{
		PlayerTypes const eVassalMember = itVassal->getID();
		if (!GET_PLAYER(eVassalMember).isVotingMember(eVS))
			continue;
		// Halve weight for vassals that are mere voting members
		scaled rWeight = fixp(0.5);
		if (bSecular || GET_PLAYER(eVassalMember).isFullMember(eVS))
			rWeight = 1;
		for (CitySetIter it = kTheyLose.begin(); it != kTheyLose.end(); ++it)
		{
			if (militAnalyst().conqueredCities(eVassalMember).count(*it) > 0)
				weightedConq.insert(make_pair(*it, rWeight));
		}
	}
	// Need to deal with them somehow for AP victory
	if (!bSecular && !kThey.isVotingMember(eVS))
	{
		if (militAnalyst().isEliminated(eThey))
			iReligionObstaclesRemoved++;
	}
	scaled rObstacleProgress;
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	if (iReligionObstacles > 0)
	{
		rObstacleProgress = scaled(iReligionObstaclesRemoved, iReligionObstacles);
		if (rObstacleProgress > 0)
		{
			if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_MILITARY_VICTORY_DIPLOMACY_OBSTACLE_PROGRESS turn=%d agentPlayer=%d rivalPlayer=%d obstacles=%d removed=%d progressPercent=%d",
					GC.getGame().getGameTurn(), eWe, eThey, iReligionObstacles, iReligionObstaclesRemoved, rObstacleProgress.getPercent());
		}
	}
	if (bSecular || kThey.isVotingMember(eVS))
	{
		scaled rMembershipMult = 1;
		if (!kThey.isFullMember(eVS))
			rMembershipMult = fixp(0.5);
		// Conquests by friends from non-friends: 2/3 weight
		addConquestsByPartner(weightedConq, ATTITUDE_FRIENDLY,
				fixp(2/3.) * rMembershipMult);
		// Pleased: 1/3
		addConquestsByPartner(weightedConq, ATTITUDE_PLEASED,
				fixp(1/3.) * rMembershipMult);
	}
	for (std::map<PlotNumTypes,scaled>::const_iterator it = weightedConq.begin();
		it != weightedConq.end(); ++it)
	{
		City const& kCacheCity = *ourCache().lookupCity(it->first);
		/*	Apply weight for new owner. -0.5 for population loss upon conquest
			(not -1 b/c total decreases as well). */
		scaled rPop = it->second * (kCacheCity.city().getPopulation() - fixp(0.5));
		PlayerTypes const eCityOwner = kCacheCity.city().getOwner();
		// Weight for old owner
		CvPlayerAI const& kCityOwner = GET_PLAYER(eCityOwner);
		// <!-- custom: This block weights the conquered city's old owner, but AdvCiv tested the conqueror's AP membership. Use the old player's membership and attitude consistently.
		// GET_TEAM(eCityOwner) is intentionally valid through the PlayerTypes overload; see rejected KI#311/KI#387 and retracted provisional KI#436. See KI#434. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if (!GET_TEAM(eCityOwner).isHuman())
		{
			/*	AP: Conquests from non-full members are always worthwhile
				b/c, as full members, we can make their votes count twice. */
			if (bSecular || kCityOwner.isFullMember(eVS))
			{
				AttitudeTypes const eOwnerTowardUs = kCityOwner.AI_getAttitude(eWe);
				if (eOwnerTowardUs >= ATTITUDE_FRIENDLY)
					rPop /= 3;
				if (eOwnerTowardUs == ATTITUDE_PLEASED)
					rPop *= fixp(2/3.);
			}
		}
		if (!bSecular && !kCacheCity.city().isHasReligion(eVSReligion))
			rPop *= fixp(0.5); // Not 0 b/c religion can still be spread
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_MILITARY_VICTORY_DIPLOMACY_CITY_VOTES turn=%d agentPlayer=%d rivalPlayer=%d cityOwner=%d cityPlot=%d expectedVotes=%d",
				GC.getGame().getGameTurn(), eWe, eThey, eCityOwner, kCacheCity.city().plotNum(), rPop.uround());
		rPopGained += rPop;
	}
	if (militAnalyst().getCapitulationsAccepted(eOurTeam).count(eTheirTeam) > 0)
	{
		scaled rNewVassalVotes;
		// Faster than a full pass through ourCache()
		FOR_EACH_CITY(pCity, kThey)
		{
			City const* pCacheCity = ourCache().lookupCity(pCity->plotNum());
			if (pCacheCity == NULL ||
				militAnalyst().lostCities(eThey).count(pCacheCity->getID()) > 0)
			{
				continue;
			}
			scaled rPop = pCity->getPopulation();
			if (!bSecular && !pCity->isHasReligion(eVSReligion))
				rPop *= fixp(0.5);
			rNewVassalVotes += rPop;
		}
		if (bSecular || kThey.isFullMember(eVS))
		{
			// Not much point if they already love us
			if (towardUs() >= ATTITUDE_FRIENDLY)
				rNewVassalVotes /= 3;
			else if (towardUs() == ATTITUDE_PLEASED)
				rNewVassalVotes *= fixp(2/3.);
		}
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_MILITARY_VICTORY_DIPLOMACY_VASSAL_VOTES turn=%d agentPlayer=%d rivalPlayer=%d expectedVotes=%d",
				GC.getGame().getGameTurn(), eWe, eThey, rNewVassalVotes.uround());
		rPopGained += rNewVassalVotes;
	}
	if (bLogWarUtilityDetail && rPopGained >= fixp(0.5)) logBBAI("UWAI_WAR_UTILITY_MILITARY_VICTORY_DIPLOMACY_TOTAL turn=%d agentPlayer=%d rivalPlayer=%d expectedVotes=%d votesToGo=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rPopGained.uround(), m_iVotesToGo);
	FAssert(m_iVotesToGo > 0);
	rPopGained.decreaseTo(m_iVotesToGo);
	scaled rProgress = rPopGained / m_iVotesToGo;
	if (iReligionObstacles <= 0)
		return rProgress;
	// If there are AP obstacles, give the progress rate on those 25% weight.
	return fixp(0.25) * rObstacleProgress + fixp(0.75) * rProgress;
}


void MilitaryVictory::addConquestsByPartner(std::map<PlotNumTypes, scaled>& kWeightedConquests, AttitudeTypes eAttitudeThresh, scaled rWeight) const
{
	if (kTheirTeam.isHuman()) // Human won't vote for us unless a vassal
	{
		if (!kTheirTeam.isVassal(eOurTeam))
			return;
	}
	else if (towardUs() < eAttitudeThresh)
		return;
	CitySet const& kTheyConquer = militAnalyst().conqueredCities(eThey);
	for (CitySetIter it = kTheyConquer.begin(); it != kTheyConquer.end(); ++it)
	{
		City const& kCacheCity = *ourCache().lookupCity(*it);
		PlayerTypes const eCityOwner = kCacheCity.city().getOwner();
		// Never good news when we lose cities
		if (GET_PLAYER(eCityOwner).getMasterTeam() == kOurTeam.getMasterTeam())
			continue;
		// Don't bother checking AP membership here (although it would be smart to count conquests by friends that are full members at the expense of friends that aren't)
		if (GET_PLAYER(eCityOwner).AI_getAttitude(eWe) < eAttitudeThresh || GET_TEAM(eCityOwner).isHuman())
		{
			kWeightedConquests.insert(make_pair(*it, rWeight));
		}
	}
}

namespace
{
	int const iPartnerUtilFromOB = 8;
}

void Assistance::evaluate()
{
	PROFILE_FUNC();
	/*	Could use CvPlayerAI::AI_stopTradingTradeVal, but that also accounts for
		angering the civ we stop trading with, and it's a bit too crude.
		I use a bit of code from there in partnerUtilFromTrade. */
	if (// Don't want to end up helping a potentially dangerous master
		kTheirTeam.isAVassal() ||
		/*	These checks can make us inclined to attack a friend if it's already
			under attack; b/c the peace scenario will have reduced utility from
			Assistance and the war scenario won't. I guess that's OK.
			"At least, if we attack them too, we don't have to worry about
			their losses (because they won't trade with us anymore anyway)."
			Costs from Affection and Ill Will should still lead to a sensible
			decision overall. */
		kOurTeam.AI_getWarPlan(eTheirTeam) != NO_WARPLAN ||
		militAnalyst().getWarsDeclaredBy(eWe).count(eThey) > 0 ||
		militAnalyst().getWarsDeclaredOn(eWe).count(eThey) > 0)
	{
		return;
	}
	/*	Utility of teammates is added up in the end; don't count extra utility
		here for assisting teammates. Since Assistance isn't a BroaderAspect,
		it should only get called for rivals in the first place. */
	FAssert(eTheirTeam != eOurTeam);
	// Shared-war bonus handled under SuckingUp
	if ((kWe.isHuman() && (kThey.isHuman() || towardUs() < ATTITUDE_CAUTIOUS)) ||
		(!kWe.isHuman() && towardThem() < ATTITUDE_PLEASED))
	{
		return;
	}
	scaled rAssistRatio = assistanceRatio();
	if (rAssistRatio <= 0)
		return;
	scaled rTradeUtility = rAssistRatio * partnerUtilFromTrade();
	// Tech trade and military support are more sensitive to losses than trade
	scaled rOtherUtility = rAssistRatio.sqrt() *
			(partnerUtilFromTech() + partnerUtilFromMilitary());
	scaled rUtility = rTradeUtility + rOtherUtility;
	// <!-- custom: Keep the optional affection and open-borders floors explicit so the consolidated result row shows which inherited lower bound constrained utility. (ChatGPT-5.6-Sol) -->
	scaled rPureAffectionUtility;
	if (!kWe.isHuman() && towardThem() >= ATTITUDE_FRIENDLY)
	{
		rPureAffectionUtility = rAssistRatio * 20;
		rUtility.increaseTo(rPureAffectionUtility);
	}
	scaled rOBUtil;
	if (kOurTeam.isOpenBorders(eTheirTeam))
		rOBUtil = iPartnerUtilFromOB * rAssistRatio;
	if (rUtility < rOBUtil && rOBUtil > 0)
		rUtility = rOBUtil;
	scaled rPersonalityMult = kWeAI.protectiveInstinct();
	// <!-- custom: Apply the same stored rounded penalty that the structured result reports instead of recomputing the final expression. (ChatGPT-5.6-Sol) -->
	int const iUtilityPenalty = (rUtility * rPersonalityMult).round();
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_ASSISTANCE_RESULT turn=%d agentPlayer=%d rivalPlayer=%d assetLossPercent=%d capitulated=%d tradeUtility=%d techMilitaryUtility=%d affectionFloor=%d openBordersFloor=%d personalityPercent=%d utilityBeforePersonality=%d utilityPenalty=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rAssistRatio.getPercent(), militAnalyst().hasCapitulated(eTheirTeam),
			rTradeUtility.round(), rOtherUtility.round(), rPureAffectionUtility.round(), rOBUtil.round(),
			rPersonalityMult.getPercent(), rUtility.round(), iUtilityPenalty);
	/*	Assistance really counts the negative utility from losses
		that our partner suffers */
	m_iU -= iUtilityPenalty;
}


scaled Assistance::assistanceRatio() const
{
	/*	Asset scores aren't entirely reliable, therefore, treat elimination
		separately. */
	if (militAnalyst().isEliminated(eThey))
		return 1;
	scaled rTheirAssets;
	// <!-- custom: The generalized asset helper now requires its victim explicitly; Assistance still evaluates the current rival eThey. See KI#432. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	scaled rTheirLostAssets = netLostAssetScore(eThey, NO_PLAYER, &rTheirAssets);
	if (rTheirAssets <= 0)
		return 0;
	scaled r(rTheirLostAssets / rTheirAssets);
	// Don't want to help them conquer cities - only look at what they lose.
	r.increaseTo(0);
	FAssert(r <= 1);
	if (militAnalyst().hasCapitulated(eTheirTeam))
	{
		// Count capitulation as a 50% devaluation of their remaining assets
		r += fixp(0.5) * (1 - r);
	}
	return r;
}


void Reconquista::evaluate()
{
	CitySet const& kWeConquer = militAnalyst().conqueredCities(eWe);
	if (kWeConquer.empty())
		return;
	int const iProjectedCities = kWe.getNumCities() +
			(int)(kWeConquer.size() - militAnalyst().lostCities(eWe).size());
	scaled const rBaseReconqVal = scaled::max(5, scaled(90, iProjectedCities + 2));
	scaled rUtility;
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	for (CitySetIter it = kWeConquer.begin(); it != kWeConquer.end(); ++it)
	{
		if (militAnalyst().lostCities(eThey).count(*it) <= 0)
			continue;
		CvCity const& kCity = ourCache().lookupCity(*it)->city();
		if (!kCity.isEverOwned(eWe))
			continue;
		/*	Lower the utility if our culture is small; suggests that we only
			held the city briefly or long ago. */
		// <!-- custom: Name the culture input and per-city result so the consolidated row reports the exact values used by the inherited multiplier. (ChatGPT-5.6-Sol) -->
		int const iCulturePercent = kCity.getPlot().calculateCulturePercent(eWe);
		scaled rReconqMult = (scaled::min(1, scaled(iCulturePercent, 50))).sqrt();
		scaled const rCityUtility = rBaseReconqVal * rReconqMult;
		rUtility += rCityUtility;
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_RECONQUISTA_CITY turn=%d agentPlayer=%d rivalPlayer=%d cityPlot=%d culturePercent=%d baseValue=%d modifierPercent=%d utility=%d",
				GC.getGame().getGameTurn(), eWe, eThey, kCity.plotNum(), iCulturePercent,
				rBaseReconqVal.uround(), rReconqMult.getPercent(), rCityUtility.round());
	}
	m_iU += rUtility.round();
}


void Rebuke::evaluate()
{
	// <!-- custom: Rejected-peace context and the later rebuke outcome can both log in one call, so reuse one level/mute gate. (ChatGPT-5.6-Sol) -->
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	/*	<advc.134a> More reluctant to make peace when they (human)
		have recently turned down a peace offer */
	if (kWe.AI_getContactTimer(eThey, CONTACT_PEACE_TREATY) > 0)
	{
		/*	Don't want to use the remaining time here b/c that gets doubled when
			capitulation is offered */
		int iRejectedPeaceCost = intdiv::uround(
				kOurPersonality.getContactDelay(CONTACT_PEACE_TREATY), 5);
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_REBUKE_REJECTED_PEACE turn=%d agentPlayer=%d rivalPlayer=%d utility=%d",
				GC.getGame().getGameTurn(), eWe, eThey, iRejectedPeaceCost);
		m_iU += iRejectedPeaceCost;
	} // </advc.134a>
	// Don't expect humans to follow through on rejected demands
	int const iRebukeDiplo = (kWe.isHuman() ? 0 :
			-1 * kWe.AI_getMemoryAttitude(eThey, MEMORY_REJECTED_DEMAND));
	if (iRebukeDiplo <= 0)
		return;
	if (militAnalyst().getCapitulationsAccepted(eOurTeam).count(eTheirTeam) > 0)
	{
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_REBUKE_CAPITULATION turn=%d agentPlayer=%d rivalPlayer=%d rebukeDiplo=%d utility=30",
				GC.getGame().getGameTurn(), eWe, eThey, iRebukeDiplo);
		/*	OK to count this multiple times for members of the same team; typically,
			only some member pairs are going to have a rebuke memory. */
		m_iU += 30;
		return;
	}
	scaled rTheirAssets;
	// <!-- custom: Rebuke compares what the agent takes against the current rival eThey's losses, now passed explicitly to the generalized helper. See KI#432. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	scaled rTheirLostAssets = netLostAssetScore(eThey, eWe, &rTheirAssets);
	if (rTheirAssets <= 0)
		return;
	/*	Give their loss and our gain equal weight. We want to demonstrate that we
		can take by force what is denied to us (and more), and that those who
		deny us end up paying more than was asked. */
	scaled rPunishmentRatio = fixp(0.5) *
			(rTheirLostAssets / rTheirAssets +
			conqAssetScore() / scaled::max(10, ourCache().totalAssetScore()));
	if (rPunishmentRatio <= 0)
		return;
	int const iUtility = std::min(30,
			(scaled(iRebukeDiplo).sqrt() * 100 * rPunishmentRatio).round());
	if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_REBUKE_PUNISHMENT turn=%d agentPlayer=%d rivalPlayer=%d rebukeDiplo=%d punishmentPercent=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, iRebukeDiplo, rPunishmentRatio.getPercent(), iUtility);
	m_iU += iUtility;
}


void Fidelity::evaluate()
{
	PROFILE_FUNC();
	/*	Don't care about our success here at all. If we declared war, we've proved
		that we don't tolerate attacks on our (declared) friends. */
	if (militAnalyst().getWarsDeclaredBy(eWe).count(eThey) <= 0)
		return;
	// Human doesn't really have friends, needs special treatment.
	if (!kWe.isHuman() && kWe.AI_getMemoryCount(eThey, MEMORY_DECLARED_WAR_ON_FRIEND) <= 0)
		return;
	/*	Check if we still like the "friend" and if they're still at war.
		Unless attacked recently, we can't tell who started it, but that's
		just as well; let's not fuss over water under the bridge. */
	// <!-- custom: Retain the qualifying friend instead of only a boolean so the consolidated result identifies which relationship triggered Fidelity. (ChatGPT-5.6-Sol) -->
	PlayerTypes eAttackedFriend = NO_PLAYER;
	for (PlayerIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itPartner(eOurTeam);
		itPartner.hasNext(); ++itPartner)
	{
		PlayerTypes const ePartner = itPartner->getID();
		if (GET_TEAM(ePartner).isAtWar(eOurTeam) ||
			GET_TEAM(ePartner).AI_getWarPlan(eTheirTeam) != WARPLAN_ATTACKED_RECENT ||
			!kWe.canContact(ePartner, true))
		{
			continue;
		}
		AttitudeTypes eOurAttitude = (kWe.isHuman() ?
				// Assume that humans likes civs that like them back
				GET_PLAYER(ePartner).AI_getAttitude(eWe) :
				kWe.AI_getAttitude(ePartner));
		/*	Pleased vs. Friendly are distinguished under "Assistance".
			I want to be consistent with the relations penalty here,
			which is the same for Pleased and Friendly. */
		if (eOurAttitude >= ATTITUDE_PLEASED)
		{
			eAttackedFriend = ePartner;
			break;
		}
	}
	if (eAttackedFriend == NO_PLAYER)
		return;
	scaled rLeaderFactor = 1;
	if (!kWe.isHuman())
	{
		/*	Only Gandhi applies a penalty of 200% for war on friend.
			Not sure if I want him to act as a peacekeeper ... Let's try it. */
		rLeaderFactor = per100(kOurPersonality.
				getMemoryAttitudePercent(MEMORY_DECLARED_WAR_ON_FRIEND));
		if (!rLeaderFactor.isNegative())
			return;
		rLeaderFactor = (-rLeaderFactor).sqrt();
	}
	int const iUtility = (rLeaderFactor * 10).round();
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_FIDELITY_RESULT turn=%d agentPlayer=%d rivalPlayer=%d attackedFriendPlayer=%d leaderFactorPercent=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, eAttackedFriend, rLeaderFactor.getPercent(), iUtility);
	m_iU += iUtility;
}


void HiredHand::evaluate()
{
	if (!m_kParams.isConsideringPeace() || !kOurTeam.isAtWar(eTheirTeam) ||
		// Perhaps redundant(?)
		militAnalyst().getWarsContinued(eWe).count(eThey) <= 0 ||
		kTheirTeam.isAVassal() || // AI can only be hired vs. master
		/*	Humans can't be hired for war and if they hire someone,
			it doesn't mean the human keeps fighting. */
		kWe.isHuman() ||
		/*	If we start another war, we'll probably focus on the new
			war enemy, and withdraw our attention from the sponsored war.
			This isn't what a faithful ally does. */
		militAnalyst().getWarsDeclaredBy(eWe).size() > 0)
	{
		return;
	}
	scaled rUtility;
	// <!-- custom: Sponsorship and the scenario-long war role are stored for the target team, but this aspect runs once per rival player. Charge those shared obligations only through the target team's leader; keep the separate player-memory hireling branch below player-specific. See KI#451. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	bool const bCanonicalTargetMember = (eThey == kTheirTeam.getLeaderID());
	if (bCanonicalTargetMember)
	{
		PlayerTypes const eSponsor = ourCache().sponsorAgainst(eTheirTeam);
		int const iOriginalUtility = ourCache().bountyAgainst(eTheirTeam);
		FAssert((eSponsor == NO_PLAYER) == (iOriginalUtility <= 0));
		// Between Annoyed and Pleased; has to be strictly better to allow sponsorship. If it becomes strictly worse, we bail.
		if (eSponsor != NO_PLAYER && iOriginalUtility > 0 && GET_PLAYER(eSponsor).isAlive() && kOurTeam.AI_getAttitude(TEAMID(eSponsor)) >= kOurPersonality.getDeclareWarRefuseAttitudeThreshold())
		{
			// Inclined to fight for 20 turns
			rUtility += eval(eSponsor, iOriginalUtility, 20);
			int iDeniedHelpDiplo = 0;
			for (MemberIter itSponsorMember(TEAMID(eSponsor)); itSponsorMember.hasNext(); ++itSponsorMember)
				iDeniedHelpDiplo -= kWe.AI_getMemoryAttitude(itSponsorMember->getID(), MEMORY_DENIED_JOIN_WAR);
			if (iDeniedHelpDiplo > 0) // (The above normally subtracts a negative value)
			{
				rUtility /= scaled(iDeniedHelpDiplo).sqrt();
				if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_HIRED_HAND_SPONSOR_ADJUSTMENT turn=%d agentPlayer=%d rivalPlayer=%d sponsorPlayer=%d deniedHelpDiplo=%d adjustedUtility=%d",
						GC.getGame().getGameTurn(), eWe, eThey, eSponsor, iDeniedHelpDiplo, rUtility.round());
			}
		}
	}
	// <!-- custom: The ally-hire obligation is target-team state, although its fallback evidence is stored in player memories; aggregate that evidence across the target members and charge the obligation once through their leader. See KI#451 and KI#1052. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (bCanonicalTargetMember)
	{
		bool bTargetRecordsHire = false;
		for (MemberAIIter itTargetMember(eTheirTeam); itTargetMember.hasNext(); ++itTargetMember)
		{
			if (itTargetMember->AI_getMemoryCount(eWe, MEMORY_HIRED_WAR_ALLY) > 0)
			{
				bTargetRecordsHire = true;
				break;
			}
		}
		// Have we hired someone to help us against eThey?
		for (PlayerAIIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itAlly(eOurTeam); itAlly.hasNext(); ++itAlly)
		{
			CvPlayerAI const& kAlly = *itAlly;
			/*	No point in checking if the ally is coming through. Need to allow
				some time for that, and we don't feel obliged for that long anyway.
				Or might say: We keep up the war just long enough to find out if
				our ally can make a difference. That's actually almost rational. */
			if (kAlly.uwai().getCache().sponsorAgainst(eTheirTeam) == eWe ||
				(kWe.AI_getMemoryCount(kAlly.getID(), MEMORY_ACCEPTED_JOIN_WAR) > 0 &&
				/*	Still can't be sure that the current war between the (human) ally
					and eThey is the war we've asked the ally to declare, but that's OK. */
				bTargetRecordsHire))
			{
				int const iAttitude = kWe.AI_getAttitude(kAlly.getID());
				if (iAttitude <= ATTITUDE_ANNOYED)
				{
					if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_HIRED_HAND_ALLY_REJECTED turn=%d agentPlayer=%d rivalPlayer=%d allyPlayer=%d attitude=%d",
							GC.getGame().getGameTurn(), eWe, eThey, kAlly.getID(), iAttitude);
					continue;
				}
				/*	Behave as if someone had paid us the equivalent of 25 utility;
					feel obliged to fight along the ally for 10 turns.
					(Or should it matter how much we've paid the ally?) */
				rUtility += eval(kAlly.getID(), 25, 10);
			}
		}
	}
	/*	Have we been at war since the start of the game? Then it's a scenario
		and we should try to play along for a while. Tbd.: Should be a
		separate aspect "Historical Role". */
	// <!-- custom: The scenario-long historical role is also team-owned on the agent side; charge it only through the first living AI member so mixed human/AI teams do not lose it merely because their administrative leader is human. See KI#451 and KI#1051. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	MemberAIIter itCanonicalAgent(eOurTeam);
	if (bCanonicalTargetMember && itCanonicalAgent.hasNext() && eWe == itCanonicalAgent->getID() &&
		kOurTeam.AI_getAtWarCounter(eTheirTeam) >= m_kGame.getElapsedGameTurns())
		rUtility += eval(NO_PLAYER, 50, 12);
	m_iU += rUtility.round();
}


scaled HiredHand::eval(PlayerTypes eAlly, int iOriginalUtility, int iObligationThresh) const
{
	// (These conditions overlap with those under Fidelity)
	if (eAlly != NO_PLAYER && (kOurTeam.isAtWar(TEAMID(eAlly)) ||
		// Important to check isAtWar first; infinite recursion possible otherwise.
		!kWe.canContact(eAlly, true) ||
		kOurTeam.AI_getWorstEnemy() == TEAMID(eAlly) ||
		GET_TEAM(eAlly).isAVassal()))
	{ // Don't feel obliged to vassals
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_HIRED_HAND_OBLIGATION_INELIGIBLE turn=%d agentPlayer=%d rivalPlayer=%d allyPlayer=%d originalUtility=%d thresholdTurns=%d utility=0",
				GC.getGame().getGameTurn(), eWe, eThey, eAlly, iOriginalUtility, iObligationThresh);
		return 0;
	}
	int const iOurAtWarCounter = (kOurTeam.isAtWar(eTheirTeam) ?
			kOurTeam.AI_getAtWarCounter(eTheirTeam) : MAX_INT);
	int const iAllyAtWarCounter =
			(eAlly == NO_PLAYER || !GET_TEAM(eAlly).isAtWar(eTheirTeam) ?
			MAX_INT : GET_TEAM(eAlly).AI_getAtWarCounter(eTheirTeam));
	// Whoever has been hired must have the smaller AtWarCounter
	int iTurnsFought = std::min(iOurAtWarCounter, iAllyAtWarCounter);
	if (iTurnsFought >= iObligationThresh)
	{
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_HIRED_HAND_OBLIGATION_COMPLETE turn=%d agentPlayer=%d rivalPlayer=%d allyPlayer=%d originalUtility=%d thresholdTurns=%d turnsFought=%d utility=0",
				GC.getGame().getGameTurn(), eWe, eThey, eAlly, iOriginalUtility, iObligationThresh, iTurnsFought);
		return 0;
	}
	scaled rObligationRatio(iObligationThresh - iTurnsFought, iObligationThresh);
	/*	Count the value of the sponsorship double initially to make relatively sure
		that we don't make peace right away due to some change in circumstance. */
	scaled rUtility = 2 * SQR(rObligationRatio) * iOriginalUtility;
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_HIRED_HAND_OBLIGATION_ACTIVE turn=%d agentPlayer=%d rivalPlayer=%d allyPlayer=%d originalUtility=%d thresholdTurns=%d turnsFought=%d obligationPercent=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, eAlly, iOriginalUtility, iObligationThresh,
			iTurnsFought, rObligationRatio.getPercent(), rUtility.round());
	return rUtility;
}


void BorderDisputes::evaluate()
{
	PROFILE_FUNC();
	int iDiploPenalty = -1 * kWe.AI_getCloseBordersAttitude(eThey);
	if (iDiploPenalty <= 0)
		return;
	{
		/*	Assume iCloseBordersAttitudeChange of -4 for human instead of the
			value from XML (which is between -2 and -4). Humans don't like
			having their cities culture-pressured. */
		int iCloseBordersAttitudeChange = kOurPersonality.
				getCloseBordersAttitudeChange();
		if (kWe.isHuman() && iCloseBordersAttitudeChange != 0)
		{
			FAssert(iCloseBordersAttitudeChange < 0);
			iDiploPenalty = intdiv::round(-4 * iDiploPenalty,
					iCloseBordersAttitudeChange);
			if (iDiploPenalty <= 0) // ==0 could happen through XML changes
			{
				FAssert(iDiploPenalty == 0);
				return;
			}
		}
	}
	if (militAnalyst().getCapitulationsAccepted(eOurTeam).count(eTheirTeam) > 0)
	{
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_BORDER_DISPUTE_CAPITULATION turn=%d agentPlayer=%d rivalPlayer=%d human=%d diploPenalty=%d utility=%d",
				GC.getGame().getGameTurn(), eWe, eThey, kWe.isHuman(), iDiploPenalty, iDiploPenalty * 8);
		m_iU += iDiploPenalty * 8;
		return;
	}
	scaled rUtility;
	CitySet const& kTheyLose = militAnalyst().lostCities(eThey);
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	for (CitySetIter itCity = kTheyLose.begin(); itCity != kTheyLose.end(); ++itCity)
	{
		CvCity const& kCity = ourCache().lookupCity(*itCity)->city();
		int const iOurPlotCulturePercent = kCity.getPlot().calculateCulturePercent(eWe);
		scaled rNewOwnerMultiplier = 0;
		// <!-- custom: Keep the actual conqueror outside the search loop so the consolidated city result can report who receives it. (ChatGPT-5.6-Sol) -->
		// <!-- custom: The initial logging refactor assigned each candidate to eConquerer before checking its conquests; if no player matched, the result falsely named the last player searched as the conqueror. Assign only after a match so unmatched cities retain NO_PLAYER. (GPT-6.1-Sol) -->
		PlayerTypes eConquerer = NO_PLAYER;
		for (PlayerIter<MAJOR_CIV> itConqueror; itConqueror.hasNext(); ++itConqueror)
		{
			if (militAnalyst().conqueredCities(itConqueror->getID()).count(*itCity) <= 0)
				continue;
			eConquerer = itConqueror->getID();
			// Conquests by our vassals (or by us) help us fully
			if (GET_PLAYER(eConquerer).getMasterTeam() == eOurTeam)
				rNewOwnerMultiplier = 1;
			// Third party: depends on how much culture they have
			else
			{
				// (Might be better to check city culture here?)
				rNewOwnerMultiplier = per100(
						75 - kCity.getPlot().calculateCulturePercent(eConquerer));
				rNewOwnerMultiplier.increaseTo(0);
			}
			/*	If we have very little culture there and we don't conquer it,
				assume that the city is far away from our border. */
			if (eConquerer != eWe && iOurPlotCulturePercent < 1)
			{
				rNewOwnerMultiplier = 0;
				if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_BORDER_DISPUTE_CITY_SKIP_THIRD_PARTY turn=%d agentPlayer=%d rivalPlayer=%d cityPlot=%d conquerorPlayer=%d ourCulturePercent=%d utility=0",
						GC.getGame().getGameTurn(), eWe, eThey, kCity.plotNum(), eConquerer, iOurPlotCulturePercent);
			}
			else if (kCity.getPlot().getCulture(eWe) <= 0)
			{
				rNewOwnerMultiplier = 0;
				if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_BORDER_DISPUTE_CITY_SKIP_ZERO_CULTURE turn=%d agentPlayer=%d rivalPlayer=%d cityPlot=%d conquerorPlayer=%d ourCulturePercent=%d utility=0",
						GC.getGame().getGameTurn(), eWe, eThey, kCity.plotNum(), eConquerer, iOurPlotCulturePercent);
			}
			break;
		}
		if (rNewOwnerMultiplier <= 0)
			continue;
		// advc.003g: Tbd.: Write a fixed-point logarithm function
		double dOurCultureMultiplier = iOurPlotCulturePercent / 100.0;
		/*	E.g. dOurCultureModifier=1 if 0% culture;
			5% -> 0.92; 10% -> 0.67; 20% -> 0.42; 50% -> 0.09.
			If we already have a lot of culture in the city, then it's probably
			not the city causing us to lose border tiles. */
		if (dOurCultureMultiplier > 0.04)
		{
			dOurCultureMultiplier = std::max(0.0,
					1 - 0.25 * fmath::log2(25 * dOurCultureMultiplier));
		}
		else dOurCultureMultiplier = 1;
		scaled rOurCultureModifier = scaled::fromDouble(dOurCultureMultiplier);
		scaled const rCityUtility = 5 * rNewOwnerMultiplier * rOurCultureModifier * iDiploPenalty;
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_BORDER_DISPUTE_CITY_COUNTED turn=%d agentPlayer=%d rivalPlayer=%d cityPlot=%d conquerorPlayer=%d ourCulturePercent=%d newOwnerPercent=%d cultureModifierPercent=%d diploPenalty=%d utility=%d",
				GC.getGame().getGameTurn(), eWe, eThey, kCity.plotNum(), eConquerer, iOurPlotCulturePercent,
				rNewOwnerMultiplier.getPercent(), rOurCultureModifier.getPercent(), iDiploPenalty, rCityUtility.round());
		rUtility += rCityUtility;
	}
	m_iU += rUtility.round();
}


void SuckingUp::evaluate()
{
	/*	Similar to Assistance and Fidelity, but this one is only about improving
		relations through the shared-war diplo modifier, whereas Assistance is about
		preventing a civ that is already a partner from being marginalized, and
		Fidelity is about discouraging future attacks on our friends. */

	/*	Sucking up to humans doesn't work, and it's unclear if relations with our
		enemies are going to matter. */
	if (kThey.isHuman() || militAnalyst().isWar(eWe, eThey) ||
		kOurTeam.AI_getWorstEnemy() == eTheirTeam ||
		// Don't bet on a loser
		militAnalyst().isEliminated(eThey) ||
		militAnalyst().hasCapitulated(eTheirTeam) ||
		militAnalyst().lostCities(eThey).size() >
		militAnalyst().conqueredCities(eThey).size() ||
		/*	(Be more generous about score when aiming at a diplo victory?
			No; better not to declare non-essential wars then.) */
		m_kGame.getPlayerScore(eWe) * 6 > m_kGame.getPlayerScore(eThey) * 10 ||
		// Hopeless case:
		diploTowardUs() <= GC.getDefineINT(CvGlobals::RELATIONS_THRESH_FURIOUS) + 1 ||
		// No need to improve relations further:
		100 * diploTowardUs() >=
		140 * GC.getDefineINT(CvGlobals::RELATIONS_THRESH_FRIENDLY))
	{
		return;
	}
	/*	If they don't need the assist or if we don't actually fight, there'll
		be no diplo bonus. The 10*NumCities threshold is very arbitrary. */
	if (militAnalyst().lostPower(eWe, ARMY) < 10 * kWe.getNumCities() ||
		militAnalyst().lostPower(eThey, ARMY) < 10 * kThey.getNumCities())
	{
		return;
	}
	int iSharedWars = 0;
	int iOurWars = 0;
	for (TeamIter<FREE_MAJOR_CIV> itEnemy; itEnemy.hasNext(); ++itEnemy)
	{
		TeamTypes const eEnemy = itEnemy->getID();
		if (eEnemy == eOurTeam || eEnemy == eTheirTeam)
			continue;
		if (militAnalyst().isWar(eOurTeam, eEnemy))
		{
			iOurWars++;
			if (militAnalyst().isWar(eTheirTeam, eEnemy))
				iSharedWars++;
		}
	}
	// If we'll fight multiple wars, we may not be able to actually assist them.
	if (iSharedWars <= 0 || iOurWars > iSharedWars)
		return;
	// Between 2 (Ashoka) and 6 (DeGaulle)
	int const iMaxDiplo = (m_kGame.isOption(GAMEOPTION_RANDOM_PERSONALITIES) ? 4 :
			GC.getInfo(kThey.getPersonalityType()).getShareWarAttitudeChangeLimit());
	scaled rUtility = fixp(1.6) * iMaxDiplo; // Should iSharedWars have an impact?
	int const iCivPlayersAlive = m_kGame.countCivPlayersAlive();
	// <!-- custom: Store the optional diplo-victory bonus separately so the consolidated result exposes whether it contributed. (ChatGPT-5.6-Sol) -->
	int iDiploVictoryBonus = 0;
	if (towardUs() == ATTITUDE_PLEASED &&
		kWe.AI_atVictoryStage(AI_VICTORY_DIPLOMACY3) && iCivPlayersAlive > 2)
	{
		iDiploVictoryBonus = 5;
		rUtility += iDiploVictoryBonus;
	}
	/*	Diplo bonus with just one civ less important in large games, but also in
		very small games or when there are few civs left. */
	int const iUtility = (rUtility / scaled(std::max(4, iCivPlayersAlive)).sqrt()).round();
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_SUCKING_UP_RESULT turn=%d agentPlayer=%d rivalPlayer=%d sharedWars=%d ourWars=%d maxDiplo=%d diploVictoryBonus=%d civPlayersAlive=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, iSharedWars, iOurWars, iMaxDiplo, iDiploVictoryBonus, iCivPlayersAlive, iUtility);
	m_iU += iUtility;
}


// <!-- custom: Reset team-forecast ownership for each agent member's military analysis. See KI#460. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
int PreEmptiveWar::preEvaluate()
{
	m_evaluatedThreatTeams.clear();
	return 0;
}


void PreEmptiveWar::evaluate()
{
	/*	If an unfavorable war with them seems likely in the long run,
		we rather take our chances now. */
	// Don't worry about long-term threat when getting close to victory
	if (kOurTeam.AI_anyMemberAtVictoryStage3() || militAnalyst().isEliminated(eWe) ||
		militAnalyst().hasCapitulated(kOurTeam.getID()))
	{
		return;
	}
	// <!-- custom: Threat ratings expose one team forecast through player-keyed reachability gates.
	// Consume each rival team's forecast once, using any member able to reach this agent instead of whichever member the generic rival loop encounters. See KI#460. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (m_evaluatedThreatTeams.count(eTheirTeam) > 0)
		return;
	m_evaluatedThreatTeams.insert(eTheirTeam);
	scaled rCurrThreat;
	for (MemberIter itMember(eTheirTeam); itMember.hasNext(); ++itMember)
		rCurrThreat.increaseTo(ourCache().threatRating(itMember->getID()));
	/*	War evaluation should always assume minor threats;
		not worth addressing here explicitly. */
	if (rCurrThreat < fixp(0.15))
		return;
	// rCurrThreat includes their vassals, so include vassals here as well.
	scaled rTheirCurrentCities;
	scaled rTheirPredictedCities;
	for (PlayerIter<MAJOR_CIV> itTheirAlly; itTheirAlly.hasNext(); ++itTheirAlly)
	{	// Them, a teammate or a vassal.
		CvPlayer const& kTheirAlly = *itTheirAlly;
		bool const bNewVassal = (militAnalyst().getCapitulationsAccepted(eTheirTeam).
				count(kTheirAlly.getTeam()) > 0);
		if (!bNewVassal &&
			kTheirAlly.getMasterTeam() != GET_PLAYER(eThey).getMasterTeam())
		{
			continue;
		}
		scaled rVassalMult = 1;
		if (bNewVassal || kTheirAlly.isAVassal())
			rVassalMult = fixp(0.5);
		if (!bNewVassal)
			rTheirCurrentCities += kTheirAlly.getNumCities() * rVassalMult;
		rTheirPredictedCities += rVassalMult *
				(kTheirAlly.getNumCities() +
				((int)militAnalyst().conqueredCities(kTheirAlly.getID()).size()) -
				((int)militAnalyst().lostCities(kTheirAlly.getID()).size()));
		/*	Ignore cities that their side gains from our human target
			(to avoid dogpiling on human) */
		TeamTypes const eOurTarget = m_kParams.getTarget();
		if (eOurTarget != NO_TEAM && eOurTarget != eTheirTeam &&
			GET_TEAM(eOurTarget).isHuman())
		{
			CitySet const& kConq = militAnalyst().conqueredCities(kTheirAlly.getID());
			for (CitySetIter itCity = kConq.begin(); itCity != kConq.end(); ++itCity)
			{
				CvCity const& kCity = *GC.getMap().getPlotByIndex(*itCity).
						getPlotCity();
				if (kCity.getTeam() == eOurTarget)
					rTheirPredictedCities -= rVassalMult;
			}
			FAssert(rTheirPredictedCities >= 0);
		}
	}
	scaled const rTheirPredictedToCurrCities = (rTheirCurrentCities > 0 ?
			rTheirPredictedCities / rTheirCurrentCities : 1);
	/*	We assume that they are going to attack us some 50 turns from now,
		and they're going to choose the time that suits them best. Therefore,
		cities that they conquer are assumed to contribute to their power,
		whereas conquests by us or our vassals are assumed to be too recent to
		contribute. However, cities of new vassals don't need time to become
		productive. */
	// <!-- custom: AdvCiv started from the whole team's city count but subtracted only the current member's predicted losses. Aggregate losses for every alive teammate so the predicted side matches the team-wide baseline. See KI#430. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	scaled rOurPredictedCities = kOurTeam.getNumCities();
	for (PlayerIter<ALIVE,MEMBER_OF> itOurMember(eOurTeam); itOurMember.hasNext(); ++itOurMember)
		rOurPredictedCities -= (int)militAnalyst().lostCities(itOurMember->getID()).size();
	scaled rOurCurrentCities = kOurTeam.getNumCities();
	TeamSet const& kCapAccepted = militAnalyst().getCapitulationsAccepted(eOurTeam);
	for (PlayerIter<MAJOR_CIV> itOurVassal; itOurVassal.hasNext(); ++itOurVassal)
	{
		CvPlayer const& kOurVassal = *itOurVassal;
		if (GET_TEAM(kOurVassal.getTeam()).isVassal(eOurTeam))
		{
			// <!-- custom: Existing vassals belong to both present and predicted sides at half weight; AdvCiv put their postwar count only in the present side, making our coalition appear to shrink without a vassal-state change. See KI#430. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			rOurCurrentCities += fixp(0.5) * kOurVassal.getNumCities();
			rOurPredictedCities += fixp(0.5) * (kOurVassal.getNumCities() - (int)militAnalyst().lostCities(kOurVassal.getID()).size());
		}
		else if (kCapAccepted.count(kOurVassal.getTeam()) > 0)
		{
			rOurPredictedCities += fixp(0.5) * (kOurVassal.getNumCities() - (int)militAnalyst().lostCities(kOurVassal.getID()).size());
		}
	}
	scaled const rOurPredictedToCurrCities = (rOurCurrentCities > 0 ?
			rOurPredictedCities / rOurCurrentCities : 1);
	// Assume that our gain is equally their loss
	scaled rTheirEdge = rTheirPredictedToCurrCities - rOurPredictedToCurrCities;
	rTheirEdge.clamp(fixp(-0.5), fixp(0.5));
	if (rTheirEdge == 0)
		return;
	// Shifts in power tend to affect the threat disproportionately
	scaled rThreatChange = rTheirEdge.abs().sqrt();
	if (rTheirEdge < 0)
		rThreatChange.flipSign();
	scaled rUtility = -90 * rCurrThreat * rThreatChange;
	// Kingmaking should handle the endgame (but not quite there yet)
	bool const bVictoryReduction = (rUtility.abs().uround() >= 1 &&
			kTheirTeam.AI_anyMemberAtVictoryStage3());
	if (bVictoryReduction)
		rUtility *= fixp(0.6);
	scaled rDistrustFactor = kWeAI.distrustRating();
	// <!-- custom: Store the exact rounded value applied to m_iU so the consolidated threat row reports the same result as gameplay. (ChatGPT-5.6-Sol) -->
	int const iUtility = (rUtility * rDistrustFactor).round();
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_PREEMPTIVE_RESULT turn=%d agentPlayer=%d rivalPlayer=%d currentThreatPercent=%d ourCurrentCities=%d ourPredictedCities=%d theirCurrentCities=%d theirPredictedCities=%d theirEdgePercent=%d threatChangePercent=%d victoryReduction=%d utilityBeforeDistrust=%d distrustPercent=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rCurrThreat.getPercent(),
			rOurCurrentCities.uround(), rOurPredictedCities.uround(),
			rTheirCurrentCities.uround(), rTheirPredictedCities.uround(),
			rTheirEdge.getPercent(), rThreatChange.getPercent(), bVictoryReduction,
			rUtility.round(), rDistrustFactor.getPercent(), iUtility);
	m_iU += iUtility;
}

scaled const KingMaking::m_rScoreMargin = fixp(0.25);

int KingMaking::preEvaluate()
{
	PROFILE_FUNC();
	m_winningFuture.clear();
	m_winningPresent.clear();
	// <!-- custom: Scoreboard ranks are unreliable off the bat, but waiting to leave the starting era never ends for a terminal-era start. Keep the era-advance shortcut and bound the grace period to 25 normalized turns, matching DramaticArc's startup clock. See KI#444. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	scaled const rStartupSpeed = 2 / per100(m_kGame.getSpeedPercent() + m_kSpeed.getTrainPercent());
	if (m_eGameEra <= m_kGame.getStartEra() && m_kGame.getElapsedGameTurns() * rStartupSpeed < 25)
		return 0;
	// Don't start or continue suicidal wars out of spite
	if (militAnalyst().isEliminated(eWe))
		return 0;
	addWinning(m_winningFuture, true);
	addWinning(m_winningPresent, false);
	scaled const rEraFactor = kOurTeam.AI_getCurrEraFactor();
	// Sealing the deal through war is a pretty aggressive move
	int const iPeaceWeight = kWe.AI_getPeaceWeight();
	int const iVeryHighPeaceWeight = 11;
	// <!-- custom: Winner sets are keyed by TeamTypes, so recognize our shared candidacy directly instead of depending on which member is the current team leader. See KI#433. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (iPeaceWeight < iVeryHighPeaceWeight &&
		rEraFactor > 0 &&
		m_winningFuture.count(eOurTeam) > 0 &&
		m_winningFuture.size() <= 1 &&
		// We don't want to be the only winner if it means betraying our partners
		(m_kParams.getTarget() == NO_TEAM ||
		kOurTeam.isAtWar(m_kParams.getTarget()) ||
		kOurTeam.AI_isChosenWar(m_kParams.getTarget()) ||
		!kOurTeam.AI_isAvoidWar(m_kParams.getTarget(), true)))
	{
		int const iUtility = (rEraFactor * (iVeryHighPeaceWeight - iPeaceWeight)).round();
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_KINGMAKING_SOLE_WINNER turn=%d agentPlayer=%d peaceWeight=%d veryHighPeaceWeight=%d eraFactorPercent=%d futureWinnerCount=%d utility=%d",
				GC.getGame().getGameTurn(), eWe, iPeaceWeight, iVeryHighPeaceWeight,
				rEraFactor.getPercent(), (int)m_winningFuture.size(), iUtility);
		return iUtility;
	}
	return 0;
}


// <!-- custom: Preserve player-local victory-stage checks, but insert their team so one shared victory candidacy occupies one winner entry. See KI#433. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void KingMaking::addWinning(TeamSet& kWinning, bool bPredict) const
{
	/*	Three categories of civs; all in the best non-empty category are likely
		winners in our book. Important to base this on the game state predicted
		by the military analysis so that the AI can tell when its actions thwart
		a rival victory or prevent a rival from getting way ahead in score.
		anyVictory and addLeadingTeams take care of this. */
	// Category III: Civs at victory stage 4
	for (PlayerAIIter<FREE_MAJOR_CIV> itPlayer; itPlayer.hasNext(); ++itPlayer)
	{
		AIVictoryStage eFlags = itPlayer->AI_getVictoryStageHash();
		if ((eFlags & AI_VICTORY_CULTURE4) &&
			itPlayer->AI_calculateCultureVictoryStage(167) < 4)
		{
			eFlags &= ~AI_VICTORY_CULTURE4;
		}
		if (anyVictory(itPlayer->getID(), eFlags, 4, bPredict))
			kWinning.insert(itPlayer->getTeam());
	}
	int const iMaxTurns = m_kGame.getMaxTurns();
	int const iTurnsRemaining = ((iMaxTurns - m_kGame.getElapsedGameTurns()) * 100) /
			m_kGame.getSpeedPercent(); // normalized turns
	/*	If we're already past the turn limit, then apparently time victory is disabled.
		(Would be safer but slower to check m_kGame.isVictoryValid in addition.) */
	bool const bTimeVictoryImminent = (iMaxTurns > 0 && iTurnsRemaining > 0 &&
			iTurnsRemaining <= 17);
	if (!bTimeVictoryImminent)
	{
		if (!kWinning.empty()) // Only stage 4 matters unless time victory is imminent
			return;
		// (If none at stage 4 and time victory not imminent)
		// Category II: Civs at victory stage 3 or game score near the top
		for (PlayerAIIter<FREE_MAJOR_CIV> itPlayer; itPlayer.hasNext(); ++itPlayer)
		{
			if (anyVictory(itPlayer->getID(),
				itPlayer->AI_getVictoryStageHash(), 3, bPredict))
			{
				kWinning.insert(itPlayer->getTeam());
			}
		}
	}
	// Category I: Civs with a competitive game score
	addLeadingTeams(kWinning, m_rScoreMargin, bPredict);
}


bool KingMaking::anyVictory(PlayerTypes ePlayer, AIVictoryStage eFlags, int iStage, bool bPredict) const
{
	FAssert(iStage == 3 || iStage == 4);
	CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
	if (!bPredict)
	{
		if (iStage == 3)
			return kPlayer.AI_atVictoryStage3();
		return (eFlags & (AI_VICTORY_SPACE4 | AI_VICTORY_MILITARY4 |
				AI_VICTORY_DIPLOMACY4 | AI_VICTORY_CULTURE4));
	}
	if (militAnalyst().isEliminated(ePlayer) ||
		militAnalyst().hasCapitulated(kPlayer.getTeam()))
	{
		return false;
	}
	if ((eFlags & AI_VICTORY_SPACE4) && iStage == 4)
	{
		// <!-- custom: Capital loss resets an active launch, but before launch the Palace merely relocates and completed spaceship Projects survive; reject the predicted Space4 contender only in the former case. See KI#1054. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		VictoryTypes const eSpaceVictory = m_kGame.getSpaceVictory();
		bool const bLaunched = (eSpaceVictory != NO_VICTORY && GET_TEAM(kPlayer.getTeam()).getVictoryCountdown(eSpaceVictory) >= 0);
		if (!bLaunched || (kPlayer.hasCapital() &&
			militAnalyst().lostCities(ePlayer).
			count(kPlayer.getCapital()->plotNum()) <= 0))
		{
			return true;
		}
	}
	if (eFlags & AI_VICTORY_CONQUEST3)
	{
		/*	Very coarse -- consider a player whose power stagnates to be
			no longer on course to a conquest victory */
		if (militAnalyst().gainedPower(ePlayer, ARMY) > 0)
		{
			if (((eFlags & AI_VICTORY_CONQUEST3) && iStage == 3) ||
				((eFlags & AI_VICTORY_CONQUEST4) && iStage == 4))
			{
				return true;
			}
		}
	}
	// Space4 and Conq already handled above
	bool bCultValid = (iStage == 4 && (eFlags & AI_VICTORY_CULTURE4)) ||
			(iStage == 3 && (eFlags & AI_VICTORY_CULTURE3));
	bool bSpaceValid = (iStage == 3 && (eFlags & AI_VICTORY_SPACE3));
	bool bDomValid = (iStage == 4 && (eFlags & AI_VICTORY_DOMINATION4)) ||
			(iStage == 3 && (eFlags & AI_VICTORY_DOMINATION3));
	bool bDiploValid = (iStage == 4 && (eFlags & AI_VICTORY_DIPLOMACY4)) ||
			(iStage == 3 && (eFlags & AI_VICTORY_DIPLOMACY3));
	/*	Conquests could bring kPlayer closer to a military victory, but that seems
		difficult to predict. Only looking at setbacks here -- which are also
		a bit difficult to predict; the main point of this function is really
		thwarted Culture/Space victory. */
	if (!bCultValid && !bSpaceValid && !bDomValid && !bDiploValid)
		return false;
	CvMap const& kMap = GC.getMap();
	int iLostPop = 0;
	int iLostNationalWonders = 0;
	CitySet const& kLostCities = militAnalyst().lostCities(ePlayer);
	for (CitySetIter it = kLostCities.begin(); it != kLostCities.end(); ++it)
	{
		CvCity const& kCity = *kMap.getPlotByIndex(*it).getPlotCity();
		/*	kPlayer could have enough other high-culture cities, but I don't mind
			if the AI is a bit too optimistic about thwarting a rival's victory
			(or extra worried about jeopardizing its own victory). */
		if ((iStage == 3 &&
			kCity.getCultureLevel() >= m_kGame.culturalVictoryCultureLevel() - 1) ||
			(iStage == 4 &&
			2 * kCity.getCulture(ePlayer) >= kCity.getCultureThreshold(
			m_kGame.culturalVictoryCultureLevel())))
		{
			bCultValid = false;
		}
		iLostNationalWonders += kCity.getNumNationalWonders();
		if (kCity.isCapital())
			iLostNationalWonders += 2;
		iLostPop += kCity.getPopulation();
	}
	if (iLostNationalWonders > 1)
		bSpaceValid = false;
	CitySet const& kGainedCities = militAnalyst().conqueredCities(ePlayer);
	for (CitySetIter it = kGainedCities.begin(); it != kGainedCities.end(); ++it)
	{
		CvCity const& kCity = *kMap.getPlotByIndex(*it).getPlotCity();
		iLostPop -= kCity.getPopulation();
	}
	// <!-- custom: Domination progress and its population threshold belong to the whole team. Aggregate projected losses and gains from every member instead of comparing one player's population with the team victory target. See KI#433. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	int iTeamLostPop = 0;
	if (bDomValid)
	{
		for (MemberIter itMember(kPlayer.getTeam()); itMember.hasNext(); ++itMember)
		{
			CitySet const& kMemberLostCities = militAnalyst().lostCities(itMember->getID());
			for (CitySetIter it = kMemberLostCities.begin(); it != kMemberLostCities.end(); ++it)
				iTeamLostPop += kMap.getPlotByIndex(*it).getPlotCity()->getPopulation();
			CitySet const& kMemberGainedCities = militAnalyst().conqueredCities(itMember->getID());
			for (CitySetIter it = kMemberGainedCities.begin(); it != kMemberGainedCities.end(); ++it)
				iTeamLostPop -= kMap.getPlotByIndex(*it).getPlotCity()->getPopulation();
		}
	}
	if (iLostPop > 0)
	{
		scaled const rRemainingWorldPopPortion(
				std::max(0, kPlayer.getTotalPopulation() - iLostPop),
				m_kGame.getTotalPopulation());
		if (rRemainingWorldPopPortion < (iStage == 3 ? fixp(0.32) : fixp(0.375)))
			bDiploValid = false;
	}
	if (iTeamLostPop > 0)
	{
		VictoryTypes const eDomination = getDominationVictory();
		if (eDomination != NO_VICTORY)
		{
			scaled const rRemainingTeamWorldPopPortion(
					std::max(0, GET_TEAM(kPlayer.getTeam()).getTotalPopulation() - iTeamLostPop),
					m_kGame.getTotalPopulation());
			if (rRemainingTeamWorldPopPortion <
				getDominationTargetPopPortion() * (iStage == 3 ? fixp(0.8) : 1))
			{
				bDomValid = false;
			}
		}
	}
	return (bCultValid || bSpaceValid || bDomValid || bDiploValid);
}


// <!-- custom: Build the Score/Time contender quantity from every team member's current or predicted score, preserving player-local commerce and overseas-peaceful-victory adjustments before summing. Add a predicted capitulation's team-shared score only once.
// Centralizing the adjusted team calculation also keeps both Kingmaking leader passes consistent. See KI#429 and KI#433. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
scaled KingMaking::adjustedContenderScore(TeamTypes eTeam, bool bPredict) const
{
	scaled rScore;
	CvCity const* pOurCapital = kWe.getCapital();
	for (MemberIter itMember(eTeam); itMember.hasNext(); ++itMember)
	{
		PlayerTypes const ePlayer = itMember->getID();
		CvPlayerAI const& kPlayer = GET_PLAYER(ePlayer);
		scaled rMemberScore = (bPredict ? militAnalyst().predictedMemberGameScore(ePlayer) : m_kGame.getPlayerScore(ePlayer));
		// Count extra score for commerce so that players that are getting far ahead in tech are identified as a threat
		scaled const rPlayerEraAIFactor = kPlayer.AI_getCurrEraFactor();
		// The late game is covered by victory stages
		if (rPlayerEraAIFactor > fixp(2.5) && rPlayerEraAIFactor < fixp(4.5) && m_rGameEraAIFactor < fixp(4.5))
		{
			scaled rCommerceRate = kOurTeam.AI_estimateYieldRate(ePlayer, YIELD_COMMERCE);
			rMemberScore += rCommerceRate / 2;
			// Beware of peaceful civs on other landmasses
			CvCity const* pPlayerCapital = kPlayer.getCapital();
			if (pOurCapital != NULL && pPlayerCapital != NULL && !pOurCapital->sameArea(*pPlayerCapital) && kPlayer.AI_atVictoryStage(AI_VICTORY_CULTURE2 | AI_VICTORY_SPACE2) && pOurCapital->getArea().getNumStartingPlots() > pPlayerCapital->getArea().getNumStartingPlots())
				rMemberScore *= fixp(4/3.);
		}
		rScore += rMemberScore;
	}
	if (bPredict)
	{
		TeamSet const& kCapitulationsAccepted = militAnalyst().getCapitulationsAccepted(eTeam);
		for (TeamSetIter itVassal = kCapitulationsAccepted.begin(); itVassal != kCapitulationsAccepted.end(); ++itVassal)
			rScore += fixp(0.4) * m_kGame.getTeamScore(*itVassal);
	}
	return rScore;
}


// <!-- custom: Rank Score/Time contenders by team score and insert each leading team once. AdvCiv ranked and stored individual members although the victory test uses CvGame::getTeamScore. See KI#433. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void KingMaking::addLeadingTeams(TeamSet& kLeading, scaled rMargin, bool bPredict) const
{
	scaled rBestScore = 1;
	for (TeamIter<FREE_MAJOR_CIV> itTeam; itTeam.hasNext(); ++itTeam)
		rBestScore.increaseTo(adjustedContenderScore(itTeam->getID(), bPredict));
	for (TeamIter<FREE_MAJOR_CIV> itTeam; itTeam.hasNext(); ++itTeam)
	{
		if (adjustedContenderScore(itTeam->getID(), bPredict) / rBestScore >= (itTeam->isHuman() ? fixp(0.7) : fixp(0.75)))
			kLeading.insert(itTeam->getID());
	}
}


void KingMaking::evaluate()
{
	/*	Someone is always ahead; an empty set indicates a special situation
		(usually: too early in the game) in which no utility should be counted. */
	if (m_winningPresent.empty())
		return;
	// <!-- custom: The winner sets, Score/Time race, Domination forecast and coalition asset change are team-owned. Evaluate them through the rival team leader once instead of repeating the same Kingmaking state for every teammate. See KI#433. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (eThey != kTheirTeam.getLeaderID())
		return;
	/*	If they're only in m_winningPresent, i.e. if we expect them to fall back,
		then that'll be covered by comparing utility with a scenario in which they
		don't fall back. (If they fall back in every scenario, then it's apparently
		out of our hands, and no utility needs to be counted.) */
	if (m_winningFuture.count(eTheirTeam) <= 0)
		return;
	if (kTheirTeam.isAVassal()) // Assumed to be out of contention
		return;
	/*	As humans we are very much not OK with rivals winning the game,
		so ATTITUDE_FURIOUS would be the smarter assumption, however, I don't want
		a leading AI to be extremely alert about a human runner-up. */
	// <!-- custom: Kingmaking is evaluated once through the target team's administrative leader, but that player's personal relation is not the agent member's attitude toward the coalition.
	// Average this agent's attitude values across all target members before applying the team-owned utility. See KI#433 and KI#1053. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	int iAttitude = ATTITUDE_ANNOYED;
	if (!kWe.isHuman())
	{
		int iTargetAttitude = 0;
		int iTargetMembers = 0;
		for (MemberIter itTargetMember(eTheirTeam); itTargetMember.hasNext(); ++itTargetMember)
		{
			iTargetAttitude += kWe.AI_getAttitudeVal(itTargetMember->getID());
			iTargetMembers++;
		}
		FAssert(iTargetMembers > 0);
		if (iTargetMembers > 0)
			iAttitude = kWe.AI_getAttitudeFromValue(iTargetAttitude / iTargetMembers);
	}
	if (kOurTeam.isAtWar(eTheirTeam)) // When at war, bad attitude is normal.
		iAttitude++;
	if (iAttitude >= ATTITUDE_FRIENDLY)
	{	/*	We don't go as far as helping a friend win
			(only indirectly by trying to thwart the victory of a disliked civ) */
		return;
	}
	scaled rAttitudeMult = fixp(0.03) + fixp(0.25) * (ATTITUDE_PLEASED - iAttitude);
	scaled rCaughtUpPremium;
	scaled rCatchUpVal = (16 * m_rGameEraAIFactor) / m_winningPresent.size();
	rCatchUpVal.exponentiate(fixp(0.75));
	rCatchUpVal *= SQR((1 + kWeAI.amortizationMultiplier()) / 2);
	bool bCaughtUp = false;
	// We're less inclined to interfere if several rivals are in competition
	int iWinningRivals = m_winningFuture.size();
	if (m_winningFuture.count(eOurTeam) > 0)
	{
		iWinningRivals--;
		if (m_winningPresent.count(eOurTeam) <= 0)
		{
			/*	We're not presently winning, but our predicted conquests bring us
				back in competition. */
			rCaughtUpPremium += rCatchUpVal;
			bCaughtUp = true;
		}
		else
		{
			/*	If we're among the likely winners, then it's a showdown between us
				and them, and we try to prevent their victory despite being Pleased.
				If we're out of contention (for the time being), we don't mind if
				they win at Pleased. */
			rAttitudeMult += fixp(0.25);
		}
	}
	else if (m_winningPresent.count(eOurTeam) > 0)
	{
		rCaughtUpPremium -= rCatchUpVal;
		// Be more reckless when falling behind
		rAttitudeMult += fixp(0.25);
	}
	if (rAttitudeMult <= 0)
	{
		int const iUtility = rCaughtUpPremium.round();
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_KINGMAKING_NONPOSITIVE_ATTITUDE turn=%d agentPlayer=%d rivalPlayer=%d attitude=%d caughtUp=%d catchUpPremium=%d futureWinnerCount=%d winningRivals=%d utility=%d",
				GC.getGame().getGameTurn(), eWe, eThey, iAttitude, bCaughtUp,
				rCaughtUpPremium.round(), (int)m_winningFuture.size(), iWinningRivals, iUtility);
		m_iU += iUtility;
		return;
	}
	/*	If they (or their vassals) make a net asset gain, we incur a cost;
		if they make a net asset loss, that's our gain. */
	scaled rTheirLoss = theirRelativeLoss();
	// <!-- custom: Keep the inherited negligible-asset-change early return, but replace its obsolete "Don't pollute the log" note: this branch now reports its catch-up-only result in one structured row. (GPT-6.1-Sol) -->
	if (rTheirLoss.abs() < fixp(0.01))
	{
		int const iUtility = rCaughtUpPremium.round();
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_KINGMAKING_NEGLIGIBLE_ASSET_CHANGE turn=%d agentPlayer=%d rivalPlayer=%d attitude=%d caughtUp=%d catchUpPremium=%d futureWinnerCount=%d winningRivals=%d theirLossPercent=%d utility=%d",
				GC.getGame().getGameTurn(), eWe, eThey, iAttitude, bCaughtUp,
				rCaughtUpPremium.round(), (int)m_winningFuture.size(), iWinningRivals,
				rTheirLoss.getPercent(), iUtility);
		m_iU += iUtility;
		return;
	}
	scaled rWeight(10,3); // So that 30% loss correspond to 100 utility
	// We're a bit less worried about helping them indirectly
	if (rTheirLoss < 0)
	{
		// If we'll catch up despite their progress, we're even less worried.
		rWeight = (bCaughtUp ? 2 : 3);
	}
	scaled rUtility = rTheirLoss * 100 * rWeight;
	// <!-- custom: Preserve the pre-clamp asset-shift utility so the consolidated row can distinguish the raw effect from the inherited [-100, 100] cap. (ChatGPT-5.6-Sol) -->
	scaled const rBaseUtility = rUtility;
	scaled rCompetitionMult;
	{
		/*	Over the course of the game, we become more willing to take out rivals
			even if several rivals are still in competition. */
		scaled rProgressFactor = fixp(3/4.) - fixp(1/3.) * m_kGame.gameTurnProgress();
		FAssert(rProgressFactor > 0);
		scaled rDiv = 1 + SQR(rProgressFactor * iWinningRivals);
		rCompetitionMult = 1 / rDiv;
	}
	rUtility.clamp(-100, 100);
	// <!-- custom: Preserve the post-clamp, pre-personality utility so the consolidated row exposes each stage of the inherited adjustment without recomputing it. (ChatGPT-5.6-Sol) -->
	scaled const rClampedUtility = rUtility;
	rUtility *= rAttitudeMult * rCompetitionMult;
	// <!-- custom: The inherited < 0.5 check emitted a separate "negligibly small" explanation so earlier base-utility rows would not mislead readers. Preserve that exact test as negligible in the consolidated row alongside adjustedUtility and the applied utility; no separate prose row is needed. (GPT-6.1-Sol) -->
	int const iUtility = (rUtility + rCaughtUpPremium).round();
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_KINGMAKING_ASSET_SHIFT turn=%d agentPlayer=%d rivalPlayer=%d attitude=%d caughtUp=%d catchUpPremium=%d futureWinnerCount=%d winningRivals=%d theirLossPercent=%d weightPercent=%d baseUtility=%d clampedUtility=%d attitudePercent=%d competitionPercent=%d adjustedUtility=%d negligible=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, iAttitude, bCaughtUp,
			rCaughtUpPremium.round(), (int)m_winningFuture.size(), iWinningRivals,
			rTheirLoss.getPercent(), rWeight.getPercent(), rBaseUtility.round(), rClampedUtility.round(),
			rAttitudeMult.getPercent(), rCompetitionMult.getPercent(), rUtility.round(),
			rUtility.abs() < fixp(0.5), iUtility);
	m_iU += iUtility;
}


scaled KingMaking::theirRelativeLoss() const
{
	PROFILE_FUNC();
	if (!militAnalyst().isPartOfAnalysis(eThey)) // To save time
		return 0;
	// Ignore assets that they gain from human target (to avoid dogpiling on human)
	TeamTypes eIgnoreGains = m_kParams.getTarget();
	if (eIgnoreGains != NO_TEAM && !GET_TEAM(eIgnoreGains).isHuman())
		eIgnoreGains = NO_TEAM;
	TeamSet const& kCapitulationsAccepted = militAnalyst().getCapitulationsAccepted(eTheirTeam);
	scaled rTheirLostAssets;
	scaled rTheirAssets;
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	for (PlayerIter<MAJOR_CIV> itTheirAlly; itTheirAlly.hasNext(); ++itTheirAlly)
	{	// Them or a teammate or vassal of them
		PlayerTypes const eTheirAlly = itTheirAlly->getID();
		CvTeam const& kTheirAllyTeam = GET_TEAM(eTheirAlly);
		bool const bNewVassal = (kCapitulationsAccepted.count(kTheirAllyTeam.getID()) > 0);
		// <!-- custom: Measure every loop player's own assets and losses. Existing and predicted vassals both contribute at half weight; AdvCiv reused eThey and gave a future vassal full weight. See KI#432. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		scaled const rVassalFactor = (kTheirAllyTeam.isAVassal() || bNewVassal ? fixp(0.5) : scaled(1));
		if (kTheirAllyTeam.getMasterTeam() != eTheirTeam)
		{
			if (bNewVassal)
			{
				scaled rAssets;
				netLostAssetScore(eTheirAlly, NO_PLAYER, &rAssets);
				// <!-- custom: Name the remaining-city ratio so the structured vassal row reports the same factor used in the gain calculation. (ChatGPT-5.6-Sol) -->
				scaled const rRemainingCityRatio = remainingCityRatio(eTheirAlly);
				scaled const rTheirGain = rVassalFactor * rRemainingCityRatio * rAssets;
				if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_KINGMAKING_NEW_VASSAL_GAIN turn=%d agentPlayer=%d rivalPlayer=%d vassalPlayer=%d vassalFactorPercent=%d remainingCityPercent=%d assets=%d gain=%d",
						GC.getGame().getGameTurn(), eWe, eThey, eTheirAlly, rVassalFactor.getPercent(),
						rRemainingCityRatio.getPercent(), rAssets.round(), rTheirGain.round());
				rTheirLostAssets -= rTheirGain;
			}
			continue;
		}
		scaled rAssets;
		// can be negative
		scaled rLostAssets = netLostAssetScore(eTheirAlly, NO_PLAYER, &rAssets, eIgnoreGains);
		rTheirAssets += rVassalFactor * rAssets;
		rTheirLostAssets += rVassalFactor * rLostAssets;
	}
	if (rTheirAssets <= 0)
	{
		// They haven't founded a city yet; can happen in late-era start
		return 0;
	}
	return rTheirLostAssets / rTheirAssets;
}

namespace
{
	/*	Kludge. ArmamentForecast will often predict dramatic increases in
		military power. Not sure how wrong those are, but I don't want to
		base the evaluation of future uses for military units on such
		grandiose projections. */
	void adjustPowerChangeProjection(scaled& rChange)
	{
		if (rChange > fixp(1.5))
		{
			rChange.exponentiate(fixp(0.5));
			rChange *= fixp(1.225);
		}
	}
}

int Effort::preEvaluate()
{
	/*	Not nice to put all code into this supposedly preparatory function.
		For most aspects, it makes sense to assign utility to each war opponent
		separately (WarUtilityAspect::evaluate(0)), but, for this aspect, the
		simulation doesn't track against whom we lose units, and our military
		build-up isn't directed against any opponent in particular. */
	scaled rUtility; // (Positive value, will be subtracted in the end.)
	// For civic and research changes at wartime:
	if (!militAnalyst().getWarsContinued(eWe).empty() ||
		!militAnalyst().getWarsDeclaredBy(eWe).empty())
	{
		bool bAllWarsLongDist = true;
		bool bAllPushOver = true;
		PlyrSet const& kWeContinue = militAnalyst().getWarsContinued(eWe);
		for (PlyrSetIter it = kWeContinue.begin(); it != kWeContinue.end(); ++it)
		{
			if (kWe.AI_hasSharedPrimaryArea(*it))
				bAllWarsLongDist = false;
			if (!kOurTeam.AI_isPushover(TEAMID(*it)))
				bAllPushOver = false;
		}
		PlyrSet const& kWeDeclare = militAnalyst().getWarsDeclaredBy(eWe);
		for (PlyrSetIter it = kWeDeclare.begin(); it != kWeDeclare.end(); ++it)
		{
			if (kWe.AI_hasSharedPrimaryArea(*it))
				bAllWarsLongDist = false;
			if (!kOurTeam.AI_isPushover(TEAMID(*it)))
				bAllPushOver = false;
		}
		if (bAllPushOver)
		{
			/*	Can't be sure that this won't lead to a change in civics
				(though it shouldn't); therefore not 0 cost. */
			rUtility += 2;
		}
		else
		{
			/*	Reduced cost for long-distance war; less disturbance of Workers
				and Settlers, and less danger of pillaging. */
			rUtility += militAnalyst().turnsSimulated() /
					((bAllWarsLongDist ? 10 : 7) +
					// Workers not much of a concern later on
					kWe.AI_getCurrEraFactor() / 2);
		}
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_EFFORT_WARTIME_ECONOMY turn=%d agentPlayer=%d targetTeam=%d allPushOver=%d allLongDistance=%d cost=%d",
				GC.getGame().getGameTurn(), eWe, m_kParams.getTarget(), bAllPushOver,
				bAllWarsLongDist, rUtility.round());
	}
	scaled rGoldPerProduction = ourCache().goldValueOfProduction();
	// <!-- custom: Preserve the baseline conversion rate so the final effort row can show any inherited long-war adjustment separately from the value actually used. (ChatGPT-5.6-Sol) -->
	scaled const rBaseGoldPerProduction = rGoldPerProduction;
	// Rather use the max over all wars?
	int const iDuration = kOurTeam.AI_getAtWarCounter(m_kParams.getTarget());
	/*	How powerful are we relative to our rivals at the end of the simulation?
		Simpler to use the BtS power ratio here than to go through all military
		branches. Our rival's per-branch power isn't public info either.
		The simulation yields per-branch losses though. Use relative losses
		in armies to predict the BtS power ratios. */
	scaled rOurPowerChange = militAnalyst().gainedPower(eWe, ARMY) /
			(ourCache().getPowerValues()[ARMY]->power() + scaled::epsilon());
	adjustPowerChangeProjection(rOurPowerChange); // Don't drink the koolaid
	scaled const rOurPower = kWe.getPower() * (1 + rOurPowerChange);
	scaled rHighestRivalPower;
	for (PlayerAIIter<MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itRival(eOurTeam);
		itRival.hasNext(); ++itRival)
	{
		CvPlayerAI const& kRival = *itRival;
		scaled rRivalPowerChange = militAnalyst().gainedPower(kRival.getID(), ARMY);
		/*	If they're not part of the military analysis, estimate their power based
			on BtS statistics. */
		if (rRivalPowerChange == 0)
		{
			rRivalPowerChange = kWeAI.estimateBuildUpRate(kRival.getID(),
					militAnalyst().turnsSimulated());
		}
		else
		{
			rRivalPowerChange /= (GET_PLAYER(kRival.getID()).uwai().getCache().
					getPowerValues()[ARMY]->power() + scaled::epsilon());
		}
		adjustPowerChangeProjection(rRivalPowerChange);
		scaled rRivalPower = kRival.getPower() * (1 + rRivalPowerChange);
		if (kWe.isHuman())
			rRivalPower *= kRival.uwai().confidenceAgainstHuman();
		else if (kRival.isHuman())
			rRivalPower *= 1 / (kWeAI.confidenceAgainstHuman() + scaled::epsilon());
		rHighestRivalPower.increaseTo(rRivalPower);
	}
	// Express our losses in terms of production
	scaled rOurLostProductionInUnits;
	scaled rOurLostUnits;
	for (int i = 0; i < NUM_BRANCHES; i++)
	{
		MilitaryBranchTypes const eBranch = (MilitaryBranchTypes)i;
		if (eBranch == CAVALRY || eBranch == LOGISTICS) // Included in other branches
			continue;
		MilitaryBranch const& kBranch = *ourCache().getPowerValues()[eBranch];
		UnitTypes const eTypicalUnit = kBranch.getTypicalUnit();
		if (eTypicalUnit == NO_UNIT)
			continue;
		scaled rLostBranchPow = militAnalyst().lostPower(eWe, eBranch);
		/*	Really shouldn't be negative, but sometimes it was
			minus 0.0-something; probably not a bug worth chasing. */
		FAssert(rLostBranchPow >= -1);
		if (rLostBranchPow < 0)
			continue;
		scaled const rTypicalPow = kBranch.getTypicalPower(eOurTeam);
		scaled rLostBranchUnits = rLostBranchPow / rTypicalPow;
		rOurLostUnits += rLostBranchUnits;
		scaled const rTypicalCost = kBranch.getTypicalCost(eOurTeam);
		FAssert(rTypicalCost > 0);
		rOurLostProductionInUnits += rTypicalCost * rLostBranchUnits;
	}
	/*	How useful are units that survive the war going forward?
		If we'll be far more powerful than our most dangerous rival, we probably
		have more units than we need.
		Some of the lost units may also be outdated chaff that we would have to
		upgrade soon if we didn't lose them. */
	scaled rFutureUse = rHighestRivalPower / (rOurPower + scaled::epsilon());
	rFutureUse.clamp(fixp(0.35), fixp(1.65));
	/*	Division by e.g. 2.2 means survivors can be valued up to 75%; 2.75: 60%
		(not taking into account the exponentiation below) */
	rFutureUse /= fixp(2.55);
	rFutureUse.exponentiate(fixp(0.75));
	if (!kWe.isHuman())
	{
		rFutureUse += scaled::max(0,
				// I.e. add between 3 (Gandhi) and 8 (Ragnar) percentage points
				scaled(kOurPersonality.getBuildUnitProb(), 500));
	}
	scaled const rInvested = militAnalyst().militaryProduction(eWe);
	scaled const rOurLostProduction = rOurLostProductionInUnits * rFutureUse +
			rInvested *
			/*	Total vs. limited war is already reflected by ArmamentForecast,
				but a strong focus on military build-up is extra harmful b/c
				even essential buildings may not get constructed then. */
			(1 - rFutureUse) * (m_kParams.isTotal() ? fixp(1.1) : 1) *
			// Future use of transports is a long shot
			(m_kParams.isNaval() ? fixp(1.2) : 1);
	scaled rSupplyCost; // Assume none if we're losing (i.e. on the defensive)
	scaled rSupplyCostBeforeAmortization;
	if (militAnalyst().lostCities(eWe).empty() && rOurLostUnits > 0)
	{
		/*	Use rOurLostUnits as a measure of the number of active fighters.
			No need to apply the handicap modifier; humans can avoid supply much
			better than the AI; that ought to cancel out.
			Halved because some of the simulated turns may be preparation,
			and even fighting units aren't constantly in foreign borders. */
		rSupplyCost = militAnalyst().turnsSimulated() * fixp(0.5) * rOurLostUnits;
		if (militAnalyst().conqueredCities(eWe).empty())
		{	// Not clear that we're on the offensive
			rSupplyCost /= 2;
		}
		// <!-- custom: Preserve the pre-amortization supply estimate so the consolidated result can distinguish the simulation estimate from the discounted cost actually used. (ChatGPT-5.6-Sol) -->
		rSupplyCostBeforeAmortization = rSupplyCost;
		rSupplyCost *= kWeAI.amortizationMultiplier();
	}
	/*	If the war has been going on for a long time, increase rGoldPerProduction
		because opportunities for peaceful development might get overlooked
		(by UWAICache::updateGoldPerProduction) in a wartime economy. (Maybe this
		is handwaving and I just want to discourage long, indecisive wars.) */
	int const iGiveWarAChanceDuration = 15;
	int iExtraDuration = iDuration - iGiveWarAChanceDuration;
	if (iExtraDuration > 0)
	{
		scaled rGameSpeedDiv = per100(m_kGame.getSpeedPercent());
		scaled const rVagueOpportunityWeight = fixp(2.3); // pretty arbitrary
		rGoldPerProduction = scaled::min(5, rGoldPerProduction *
				(1 + rVagueOpportunityWeight * fixp(0.025) *
				std::min(iExtraDuration, 40) / rGameSpeedDiv));
	}
	scaled rTradeVal = rSupplyCost + rGoldPerProduction * rOurLostProduction;
	rUtility += kWeAI.tradeValToUtility(rTradeVal);
	/*	Nukes are included in army, and therefore already covered by the costs above.
		But these don't take into account that nukes are always lost when used.
		Therefore add some extra cost. */
	scaled const rFired = militAnalyst().getNukesFiredBy(eWe);
	// <!-- custom: Moved the inherited nuke-production and nuke-cost values outside the fired-nuke branch so the final effort row can report them; both default to zero when that branch adds no cost. (GPT-6.1-Sol) -->
	scaled rNukeProduction;
	scaled rNukeCost;
	MilitaryBranch const& kNukeBranch = *ourCache().getPowerValues()[NUCLEAR];
	UnitTypes const eNuke = kNukeBranch.getTypicalUnit();
	if (eNuke != NO_UNIT && rFired > 0)
	{
		/*	Not clear that we have to replace the fired nukes.
			Reduce the lost production to about a third for that reason,
			and b/c already partially covered by army losses. */
		rNukeProduction = fixp(0.3) * rFired * kNukeBranch.getTypicalCost();
		rNukeCost = kWeAI.tradeValToUtility(rGoldPerProduction * rNukeProduction);
		rUtility += rNukeCost;
	}
	// <!-- custom: Apply and log the same rounded/capped effort cost so diagnostics cannot drift from the returned utility while still exposing the inherited component stages. (ChatGPT-5.6-Sol) -->
	int const iCostBeforeCap = rUtility.round();
	int const iCost = std::min(200, iCostBeforeCap);
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_EFFORT_RESULT turn=%d agentPlayer=%d targetTeam=%d baseGoldPerProductionPercent=%d finalGoldPerProductionPercent=%d lostUnitProduction=%d investedProduction=%d futureUsePercent=%d adjustedLostProduction=%d lostUnits=%d supplyCostBeforeAmortization=%d supplyCost=%d turnsSimulated=%d warDuration=%d tradeValue=%d nukesFiredX100=%d nukeProduction=%d nukeCost=%d costBeforeCap=%d cost=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, m_kParams.getTarget(),
			rBaseGoldPerProduction.getPercent(), rGoldPerProduction.getPercent(),
			rOurLostProductionInUnits.round(), rInvested.round(), rFutureUse.getPercent(),
			rOurLostProduction.round(), rOurLostUnits.round(),
			rSupplyCostBeforeAmortization.round(), rSupplyCost.round(),
			militAnalyst().turnsSimulated(), iDuration, rTradeVal.round(),
			rFired.getPercent(), rNukeProduction.round(), rNukeCost.round(),
			iCostBeforeCap, iCost, -iCost);
	return -iCost;
}


void Effort::evaluate() {}


int Risk::preEvaluate()
{
	// <!-- custom: also handle risk of going to war too far away and leaving our cities defenseless and dying pathetically if i may say or stupidly shortly after, as is very problematic as of now in base advciv and advciv-sas as of now, see known issue as of now 61 for details, also code is provided thanks to chatgpt 5, check if accurate; result of this code change: we seem to live a bit longer but still attack the wrong target when a closer and weaker one was in reach, but since it seems harmless and we seem to live longer due to not attacking first target if not due to autoplay fluctuation, kept as such. -->
	// <!-- custom: If a fixed, super-simple turns-to-contact cap is wanted, replace these calls with constants or XML defines. The cached city distances (`getDistanceByLand` for land and `getDistance` for mixed/cargo routes) are the team pathfinder's notion of turns and already factor roads, terrain and domain speed. See KI#591. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	// --- SIMPLE HARD REJECT / RPE FILTER ---

	// <!-- custom: we get very very good results with 2, now spain ai (our ai in autoplay) doesn't get baited by faraway ais and finishes off weak nearby one, as a result we don't get targeted by cyrus ai and keep our lead, now trying to extend the range a bit to see if still safe -->
	// <!-- custom: result: seemingly even better, see known issue as of now 61 for details, not increased further to not risk falling back to old pitfalls, while keeping variety enough in war outcomes with larger window -->
	static int const iMaxLandTurns = std::max(0, GC.getDefineINT("SAS_UWAI_WAR_TARGET_MAX_LAND_CONTACT_TURNS_UNSCALED_GAMESPEED"));
	static int const iMaxSeaTurns = std::max(0, GC.getDefineINT("SAS_UWAI_WAR_TARGET_MAX_SEA_CONTACT_TURNS_UNSCALED_GAMESPEED"));
	static int const iExistingPlanTolerance = std::max(0, GC.getDefineINT("SAS_UWAI_WAR_TARGET_EXISTING_PLAN_CONTACT_TOLERANCE_TURNS_UNSCALED_GAMESPEED"));

	// <!-- custom: The older AdvCiv-SAS hard reject is only a pre-war target guard. Letting it fire in the recursive peace scenario made mediocre wars look artificially excellent (KI#183).
	// Letting it fire during an ongoing war instead injected -100000 into peace reviews and forced stronger conquerors to stop after reaching a more distant remaining city. Evaluate distance and transport capacity only before war begins, which also avoids the cache scan in both excluded scenarios. (GPT-5.6-Sol) -->
	const TeamTypes eTarget = m_kParams.getTarget();
	// <!-- custom: This is a team-level target-eligibility gate, not a per-member cost. Evaluate it once across every alive agent member so one remote teammate cannot veto another member's reachable front, and one collectively invalid target contributes only one hard reject. See KI#425. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (!m_bSASContactGateEvaluated)
	{
		m_bSASContactGateEvaluated = true;
		if (!militAnalyst().isPeaceScenario() && !kOurTeam.isAtWar(eTarget))
		{
			bool const bNaval = m_kParams.isNaval();
			int iMinLandContact = INT_MAX;
			int iMinAnyContact = INT_MAX;
			int iMinCargoContact = INT_MAX;
			for (MemberIter itMember(eOurTeam); itMember.hasNext(); ++itMember)
			{
				UWAICache const& kMemberCache = GET_PLAYER(itMember->getID()).uwai().getCache();
				bool const bMemberCanTrainCargo = kMemberCache.canTrainAnyCargo();
				for (int i = 0; i < kMemberCache.numCities(); ++i)
				{
					UWAICache::City const& kCity = kMemberCache.cityAt(i);
					if (kCity.city().getTeam() != eTarget) continue; // Target-specific
					if (!kCity.canReach()) continue;
					iMinAnyContact = std::min(iMinAnyContact, kCity.getDistance()); // Approximate turns including roads
					if (bMemberCanTrainCargo) iMinCargoContact = std::min(iMinCargoContact, kCity.getDistance());
					// <!-- custom: Keep the SAS land-contact gate on the qualifying land duration while its generic and cargo alternatives retain the fastest mixed route. See KI#591. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
					if (kCity.canReachByLand()) iMinLandContact = std::min(iMinLandContact, kCity.getDistanceByLand());
				}
			}
			// <!-- custom: Save-file 450 logging showed the same land-reachable target passing this guard as a naval plan during selection, then receiving about -100000 utility under the land limit during its next review. Holy Rome consequently started 12 preparations and canceled 11 while up to 110 of 142 military units waited in one staging city.
			// When any target city is reachable by land, apply the land distance consistently even if UWAI temporarily prefers a naval scenario. Use the wider sea limit and transport requirement only when reaching the target actually requires naval transport; this prevents a single transport or evaluator-mode change from repeatedly creating and destroying the same plan. (GPT-5.6-Sol) -->
			bool const bDistanceGateNaval = (bNaval && iMinLandContact == INT_MAX);
			// <!-- custom: A naval qualification requires the same teammate to supply both the reachable route and cargo-building capability; unrelated members cannot combine disconnected local caches. See KI#425. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			int const iMinContact = (iMinLandContact != INT_MAX ? iMinLandContact : (bDistanceGateNaval ? iMinCargoContact : INT_MAX));
			int const iCanTrainCargo = (bDistanceGateNaval ? (iMinCargoContact != INT_MAX) : -1);
			bool const bNoLift = (bDistanceGateNaval && iCanTrainCargo == 0);
			bool const bExistingPlan = (kOurTeam.AI_getWarPlan(eTarget) != NO_WARPLAN);
			int const iContactTolerance = (bExistingPlan ? iExistingPlanTolerance : 0);
			int const iMaxTurns = (bDistanceGateNaval ? iMaxSeaTurns : iMaxLandTurns) + iContactTolerance;
			bool const bOrdinaryHardReject = (iMinContact == INT_MAX || iMinContact > iMaxTurns || bNoLift);
			// <!-- custom: Save-file 449 showed the ordinary 3-turn land-contact gate assigning about -100000 utility to Celts and Aztecs even though the victory-denial policy approved them as nearby, stronger emergency responders to India's launched spaceship.
			// If the exact shared urgency, power, distance and land-access policy approves the target using this evaluator's cached path turns, bypass only the ordinary distance veto; unreachable targets and naval plans without transport remain rejected.
			// Applying the wider distance at stage 3 caused four independent declarations at only 8/16 spaceship parts. Preserve close stage-3 direct wars, but require stage 4/countdown urgency for this new bypass unless the separate XML switch explicitly enables it. (GPT-5.6-Sol) -->
			bool const bVictoryDenialContactCandidate = (bOrdinaryHardReject && iMinContact != INT_MAX && !bNoLift);
			int const iTargetMaxVictoryStage = (bVictoryDenialContactCandidate ? getSASTeamMaxVictoryStage(eTarget) : -1);
			int const iTargetVictoryCountdown = (bVictoryDenialContactCandidate ? GET_TEAM(eTarget).AI_getLowestVictoryCountdown() : -1);
			static const bool bStage3SpaceContactBypassEnable = GC.getDefineBOOL("SAS_UWAI_VICTORY_DENIAL_DIRECT_STAGE3_SPACE_CONTACT_BYPASS_ENABLE");
			bool const bWiderVictoryDenialContactAllowed = (bStage3SpaceContactBypassEnable || iTargetMaxVictoryStage >= 4 || iTargetVictoryCountdown >= 0);
			bool const bVictoryDenialContactBypass = (bVictoryDenialContactCandidate && bWiderVictoryDenialContactAllowed && kOurTeam.uwai().isSASVictoryDenialDirectWarAllowed(eTarget, iTargetMaxVictoryStage, bNaval, iMinContact));
			// <!-- custom: In a confirming save-file 450 run, Holy Rome chose a profitable target at 3 cached path turns, but an Arabian road was pillaged two turns later and raised contact to 4; the hard reject then canceled the direct plan.
			// Preserve the strict limit for new targets, but give an already-started plan a small XML-tunable tolerance so a single route or border fluctuation does not waste its mobilization. (GPT-5.6-Sol) -->
			if (bOrdinaryHardReject && !bVictoryDenialContactBypass)
			{
				if (gWarLogLevel >= 3 || (gOverseasTransportLogLevel >= 3 && bDistanceGateNaval) || (gOverseasTransportLogLevel >= 2 && bNoLift)) logBBAI("WAR_TARGET_HARD_REJECT turn=%d agentTeam=%d targetTeam=%d total=%d naval=%d distanceGateNaval=%d existingPlan=%d contactToleranceTurns=%d preparationTurns=%d nearestContactTurns=%d nearestLandContactTurns=%d nearestAnyContactTurns=%d maxContactTurns=%d canTrainCargo=%d unreachable=%d tooFar=%d noLift=%d",
					GC.getGame().getGameTurn(), eOurTeam, eTarget, m_kParams.isTotal(), bNaval, bDistanceGateNaval, bExistingPlan,
					iContactTolerance, m_kParams.getPreparationTime(), (iMinContact == INT_MAX ? -1 : iMinContact),
					(iMinLandContact == INT_MAX ? -1 : iMinLandContact), (iMinAnyContact == INT_MAX ? -1 : iMinAnyContact), iMaxTurns,
					iCanTrainCargo, (iMinContact == INT_MAX), (iMinContact != INT_MAX && iMinContact > iMaxTurns), bNoLift);
				return -100000; // kill this (agent,target) war plan
			}
			if (gWarLogLevel >= 1 && bVictoryDenialContactBypass) logBBAI("WAR_TARGET_VICTORY_DENIAL_CONTACT_BYPASS turn=%d agentTeam=%d targetTeam=%d total=%d naval=%d existingPlan=%d nearestContactTurns=%d ordinaryMaxContactTurns=%d targetMaxVictoryStage=%d targetVictoryCountdown=%d targetPowerPercent=%d",
				GC.getGame().getGameTurn(), eOurTeam, eTarget, m_kParams.isTotal(), bNaval, bExistingPlan, iMinContact, iMaxTurns,
				iTargetMaxVictoryStage, iTargetVictoryCountdown,
				100 * GET_TEAM(eTarget).getDefensivePower(eOurTeam) / std::max(1, kOurTeam.getPower(true)));
		}
	}

	// --- END RPE FILTER ---

	// Handle potential losses of our vassals here
	scaled rUtility; // (Positive value, gets subtracted in the end.)
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	for (PlayerIter<ALIVE,VASSAL_OF> itVassal(eOurTeam); itVassal.hasNext(); ++itVassal)
	{
		PlayerTypes const eVassal = itVassal->getID();
		CvPlayerAI const& kVassal = GET_PLAYER(eVassal);
		// OK to peek into our vassal's cache
		UWAICache const& kVassalCache = kVassal.uwai().getCache();
		scaled rRelativeVassalLoss = 1;
		// <!-- custom: Moved the inherited lost-assets accumulator outside the surviving-vassal branch so the consolidated row can read it afterward. scaled defaults to zero; for eliminated vassals this sum is not calculated, while the inherited relative-loss value remains 100 percent. (GPT-6.1-Sol) -->
		scaled rLostVassalAssets;
		if (!militAnalyst().isEliminated(eVassal))
		{
			CitySet const& kLostCities = militAnalyst().lostCities(eVassal);
			for (CitySetIter it = kLostCities.begin(); it != kLostCities.end(); ++it)
			{
				City const* pCacheCity = kVassalCache.lookupCity(*it);
				if (pCacheCity == NULL)
					continue;
				rLostVassalAssets += pCacheCity->getAssetScore();
			}
			rRelativeVassalLoss = rLostVassalAssets /
					scaled::max(1, kVassalCache.totalAssetScore());
		}
		scaled rVassalCost; // (i.e. negative utility)
		if (rRelativeVassalLoss > 0)
		{	/*	advc.test: Total asset score less than the sum of the city scores.
				Had not worked correctly when called from UWAI::processNewPlayerInGame.
				Should be fixed now; let's see ... */
			FAssert(rRelativeVassalLoss <= 1);
			rRelativeVassalLoss.decreaseTo(1);
			rVassalCost = rRelativeVassalLoss * 33;
		}
		// <!-- custom: Moved the inherited break-away cost outside its conditional branch so the final vassal row reports the exact added component; its default zero means no break-away cost was applied. (GPT-6.1-Sol) -->
		scaled rBreakAwayCost;
		if (!GET_TEAM(eVassal).isCapitulated())
		{
			scaled rRelativePow =
					(militAnalyst().gainedPower(eVassal, ARMY) +
					kVassalCache.getPowerValues()[ARMY]->power()) /
					(militAnalyst().gainedPower(eWe, ARMY) +
					ourCache().getPowerValues()[ARMY]->power() + scaled::epsilon());
			rRelativePow -= fixp(0.9);
			if (rRelativePow > 0)
			{
				rBreakAwayCost = scaled::min(20, rRelativePow.sqrt() * 40);
				rVassalCost += rBreakAwayCost;
			}
		}
		if (bLogWarUtilityDetail && rVassalCost >= fixp(0.5)) logBBAI("UWAI_WAR_UTILITY_RISK_VASSAL turn=%d agentPlayer=%d vassalPlayer=%d eliminated=%d capitulated=%d lostAssets=%d relativeLossPercent=%d breakAwayCost=%d totalCost=%d",
				GC.getGame().getGameTurn(), eWe, eVassal, militAnalyst().isEliminated(eVassal),
				GET_TEAM(eVassal).isCapitulated(), rLostVassalAssets.round(),
				rRelativeVassalLoss.getPercent(), rBreakAwayCost.round(), rVassalCost.round());
		rUtility += rVassalCost;
	}
	return -rUtility.round();
}


void Risk::evaluate()
{
	PROFILE_FUNC();
	/*	Thwarted victory is mainly handled by Kingmaking::anyVictory, but, so long
		as one victory condition remains feasible/imminent, that function won't
		yield negative utility. Therefore count some extra lost assets for lost cities
		that are important for a victory condition. */
	bool const bSpace4 = kWe.AI_atVictoryStage(AI_VICTORY_SPACE4);
	bool const bCulture3 = kWe.AI_atVictoryStage(AI_VICTORY_CULTURE3);
	bool const bCulture4 = bCulture3 && kWe.AI_atVictoryStage(AI_VICTORY_CULTURE4);
	scaled rLostAssets;
	CitySet const& kWeLose = militAnalyst().lostCities(eWe);
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	for (CitySetIter it = kWeLose.begin(); it != kWeLose.end(); ++it)
	{
		/*	It doesn't matter here whom the city is lost to, but looking at
			one enemy at a time produces better debug output. */
		if (militAnalyst().conqueredCities(eThey).count(*it) <= 0)
			continue;
		City const& kCacheCity = *ourCache().lookupCity(*it);
		scaled rAssetScore = kCacheCity.getAssetScore();
		scaled const rBaseAssetScore = rAssetScore;
		int iVictoryAssetBonus = 0;
		CvCity const& kCity = kCacheCity.city();
		if (bSpace4)
		{
			if (kCity.isCapital() && kWe.AI_atVictoryStage(AI_VICTORY_SPACE4))
			{
				rAssetScore += 15;
				iVictoryAssetBonus += 15;
			}
		}
		if (bCulture3)
		{
			// Same condition as in CvPlayerAI::AI_conquerCity
			if (2 * kCity.getCulture(kCity.getOwner()) >=
				kCity.getCultureThreshold(m_kGame.culturalVictoryCultureLevel()))
			{
				int const iCultureVictoryBonus = (bCulture4 ? 15 : 8);
				rAssetScore += iCultureVictoryBonus;
				iVictoryAssetBonus += iCultureVictoryBonus;
			}
		}
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_RISK_LOST_CITY turn=%d agentPlayer=%d rivalPlayer=%d cityPlot=%d baseAssetScore=%d victoryAssetBonus=%d assetScore=%d spaceStage4=%d cultureStage3=%d cultureStage4=%d",
				GC.getGame().getGameTurn(), eWe, eThey, kCity.plotNum(), rBaseAssetScore.round(),
				iVictoryAssetBonus, rAssetScore.round(), bSpace4, bCulture3, bCulture4);
		rLostAssets += rAssetScore;
	}
	rLostAssets.increaseTo(0); // Per-city score can be negative (maintenance)
	scaled rUtility = 400 * rLostAssets; // (Will be subtracted in the end)
	scaled const rTotalAssets = ourCache().totalAssetScore() + scaled::epsilon();
	scaled rFromBlockade = lossesFromBlockade(eWe, eThey) * fixp(0.1) *
			(rTotalAssets - rLostAssets);
	rFromBlockade.increaseTo(0);
	rUtility += rFromBlockade;
	scaled rFromNukes = 400 * lossesFromNukes(eWe, eThey);
	scaled const rNukeCostBeforeScare = rFromNukes;
	bool bNukeScareFloorApplied = false;
	int const iScareCost = 26;
	if (kThey.getNumNukeUnits() > 0 &&
		militAnalyst().getWarsDeclaredBy(eWe).count(eThey) > 0 &&
		kTheirTeam.getNumWars() <= 0 && rFromNukes <= iScareCost)
	{
		rFromNukes = iScareCost;
		bNukeScareFloorApplied = true;
	}
	rUtility += rFromNukes;
	/*	advc.035 (comment): Don't consider lossFromFlippedTiles here. We might
		capture the very cities that steal our tiles by continuing the war. */
	rUtility /= rTotalAssets;
	// <!-- custom: Preserve the normalized asset-risk cost before capitulation/elimination adjustments so the consolidated result exposes the same inherited stages that feed the final utility. (ChatGPT-5.6-Sol) -->
	scaled const rAssetRiskCost = rUtility;
	bool const bCapitulationAccepted = (militAnalyst().getCapitulationsAccepted(eTheirTeam).count(eOurTeam) > 0);
	int iCapitulationCost = 0;
	if (bCapitulationAccepted)
	{	/*	Counting lost cities in addition to capitulation might make us too
			afraid of decisively lost wars. Then again, perhaps one can never be too
			afraid of those? */
		rUtility /= 2;
		iCapitulationCost = 100;
		rUtility += iCapitulationCost;
	}
	/*	Count something extra for elimination just to make sure that peace is
		sought even when fighting multiple hopeless wars. */
	int iEliminationCost = 0;
	if (militAnalyst().isEliminated(eWe) && militAnalyst().isWar(eWe, eThey) &&
		/*	Don't require them to conquer any cities in the military analysis.
			Another war enemy may get to us much faster, but it's still good
			to end wars with any dangerous enemy. */
		kThey.uwai().getCache().numReachableCities(eWe) >= kWe.getNumCities() &&
		/*	Require _some_ action though; don't want players to declare war
			just to extract some reparations. */
		kTheirTeam.AI_getWarSuccess(eOurTeam) >= GC.getWAR_SUCCESS_CITY_CAPTURING())
	{
		iEliminationCost = kThey.getNumCities(); // a little arbitrary ...
		rUtility += iEliminationCost;
	}
	// <!-- custom: Apply and report the same rounded risk cost while keeping the inherited blockade, nuke, capitulation and elimination stages explicit in one result event. (ChatGPT-5.6-Sol) -->
	int const iUtility = rUtility.round();
	if (bLogWarUtilityDetail && iUtility != 0) logBBAI("UWAI_WAR_UTILITY_RISK_RESULT turn=%d agentPlayer=%d rivalPlayer=%d lostAssets=%d totalAssets=%d blockadeCost=%d nukeCostBeforeScare=%d nukeScareFloorApplied=%d nukeCost=%d assetRiskCost=%d capitulationAccepted=%d capitulationCost=%d eliminationCost=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rLostAssets.round(), rTotalAssets.round(),
			(rFromBlockade / rTotalAssets).round(), (rNukeCostBeforeScare / rTotalAssets).round(),
			bNukeScareFloorApplied, (rFromNukes / rTotalAssets).round(), rAssetRiskCost.round(),
			bCapitulationAccepted, iCapitulationCost, iEliminationCost, -iUtility);
	m_iU -= iUtility;
}


int IllWill::preEvaluate()
{
	int iHostiles = 0;
	for (PlayerAIIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itHostile(eOurTeam);
		itHostile.hasNext(); ++itHostile)
	{
		if (militAnalyst().isWar(eWe, itHostile->getID()) ||
			(!itHostile->isHuman() &&
			itHostile->AI_getAttitude(eWe) <= ATTITUDE_ANNOYED))
		{
			iHostiles++;
		}
	}
	int iPartners = countFreeRivals<true>() - iHostiles;
	m_rAltPartnerFactor = scaled(1, std::max(1, iPartners - 3)).sqrt();
	return 0;
}


void IllWill::evaluate() 
{
	PROFILE_FUNC();
	m_rCost = 0;
	// We can't trade with our war enemies
	evalLostPartner();
	bool const bEndNigh = (kOurTeam.AI_anyMemberAtVictoryStage4() ||
			kTheirTeam.AI_anyMemberAtVictoryStage4());
	if ((!bEndNigh && !kOurTeam.isAVassal() && !kTheirTeam.isAVassal()) ||
		kWe.AI_atVictoryStage(AI_VICTORY_DIPLOMACY4) ||
		kThey.AI_atVictoryStage(AI_VICTORY_DIPLOMACY4))
	{
		// Civs (our potential partners) don't like when we attack their (other) partners
		evalAngeredPartners();
	}
	if (!bEndNigh && !kOurTeam.isAVassal() && !kTheirTeam.isAVassal() &&
		!militAnalyst().hasCapitulated(eOurTeam) && !militAnalyst().isEliminated(eWe) &&
		!militAnalyst().hasCapitulated(eTheirTeam) && !militAnalyst().isEliminated(eThey))
	{
		// Ill will from war makes them more likely to attack us in the future
		evalRevenge();
	}
	scaled rNukeCost;
	// Fired but intercepted nukes don't cause bad diplo
	scaled rCitiesWeNuked = militAnalyst().getNukedCities(eWe, eThey);
	if (rCitiesWeNuked > 0)
		rNukeCost = nukeCost(rCitiesWeNuked);
	// NB: Subroutines have added to m_rCost
	m_rCost += rNukeCost;
	// <!-- custom: Apply and report the same rounded aggregate ill-will cost; detailed subevents remain available for the partner, revenge, third-party and nuke contributors. (ChatGPT-5.6-Sol) -->
	int const iCost = m_rCost.round();
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted() && iCost != 0) logBBAI("UWAI_WAR_UTILITY_ILL_WILL_RESULT turn=%d agentPlayer=%d rivalPlayer=%d nukedCitiesX100=%d nukeCost=%d totalCost=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rCitiesWeNuked.getPercent(),
			rNukeCost.round(), iCost, -iCost);
	m_iU -= iCost;
}


scaled IllWill::nukeCost(scaled rCitiesWeNuked) const
{
	if (kThey.isHuman() ||
		100 * diploTowardUs() >
		120 * GC.getDefineINT(CvGlobals::RELATIONS_THRESH_FRIENDLY) ||
		towardUs() <= ATTITUDE_FURIOUS || towardThem() <= ATTITUDE_FURIOUS)
	{
		return 0;
	}
	scaled rNukeCost;
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	for (PlayerAIIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itThird(eOurTeam);
		itThird.hasNext(); ++itThird)
	{
		CvPlayerAI const& kThirdPlayer = *itThird;
		if (kThirdPlayer.getTeam() == eTheirTeam || kThirdPlayer.isHuman() ||
			militAnalyst().isWar(eWe, kThirdPlayer.getID()) ||
			kThirdPlayer.AI_getAttitudeVal(eWe) > 12 ||
			kThirdPlayer.AI_getAttitude(eThey) < ATTITUDE_PLEASED)
		{
			continue;
		}
		scaled rAttitudeFactor = 3;
		/*	If they like us too, we don't necessarily get a penalty due to change
			130q. Small factor anyway since, this late in the game, we mostly care
			about other civs as war allies or enemies. */
		if (kThirdPlayer.AI_getAttitude(eWe) >= ATTITUDE_PLEASED)
			rAttitudeFactor /= 2;
		// <!-- custom: Name the exact per-partner contribution so the structured row and the accumulated nuke cost consume the same value. (ChatGPT-5.6-Sol) -->
		scaled const rContribution = rAttitudeFactor * rCitiesWeNuked;
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_ILL_WILL_NUKE_THIRD_PARTY turn=%d agentPlayer=%d rivalPlayer=%d thirdPlayer=%d attitudeFactorPercent=%d nukedCitiesX100=%d contribution=%d",
				GC.getGame().getGameTurn(), eWe, eThey, kThirdPlayer.getID(),
				rAttitudeFactor.getPercent(), rCitiesWeNuked.getPercent(), rContribution.round());
		rNukeCost += rContribution;
	}
	rNukeCost.decreaseTo(40);
	return rNukeCost;
}


void IllWill::evalLostPartner()
{
	if (countFreeRivals<false>() < 2)
	{
		// Zero-sum game then, and our loss is equally their loss.
		return;
	}
	bool const bTheyCapitulated = (kTheirTeam.isCapitulated() ||
			militAnalyst().hasCapitulated(eTheirTeam));
	if (kOurTeam.isAtWar(eTheirTeam))
	{
		if (militAnalyst().isWar(eWe, eThey) ||
			militAnalyst().isEliminated(eThey))
		{
			return;
		}
		// Ending the war - can we reconcile with them?
		scaled rDiploDiv = fixp(0.7);
		if (!kThey.isHuman())
		{
			// Only master attitude matters for capitulated vassals (b/c of change 130v)
			if (!bTheyCapitulated &&
				(towardUs() <= ATTITUDE_FURIOUS ||
				// Takes two to tango
				(!kWe.isHuman() && towardThem() <= ATTITUDE_FURIOUS)))
			{
				rDiploDiv += fixp(0.3);
			}
			// Rare as this may be ...
			if (towardThem() >= ATTITUDE_CAUTIOUS)
				rDiploDiv -= fixp(0.15);
			if (towardUs() >= ATTITUDE_CAUTIOUS)
				rDiploDiv -= fixp(0.15);
		}
		/*	Humans are forgiving and tend to end wars quickly. This
			special treatment should lead to slightly greater willingness to
			declare war against human (handled at the end of this function),
			but also slightly greater willingness to make peace again. */
		else rDiploDiv -= fixp(0.15);
		// Don't expect much from reconciling with minor players
		if (militAnalyst().isEliminated(eThey) || bTheyCapitulated)
			rDiploDiv = 2;
		// Reconciliation is a long shot for the AI
		scaled rReconciliationUtility =
				4 * (m_rAltPartnerFactor / rDiploDiv) *
				kWeAI.amortizationMultiplier();
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_ILL_WILL_RECONCILIATION turn=%d agentPlayer=%d rivalPlayer=%d capitulated=%d diploDivPercent=%d alternativePartnerPercent=%d amortizationPercent=%d utility=%d",
				GC.getGame().getGameTurn(), eWe, eThey, bTheyCapitulated, rDiploDiv.getPercent(),
				m_rAltPartnerFactor.getPercent(), kWeAI.amortizationMultiplier().getPercent(),
				rReconciliationUtility.round());
		m_rCost -= rReconciliationUtility;
		return;
	}
	if (!militAnalyst().isWar(eWe, eThey))
		return;
	scaled rPartnerUtil = partnerUtilFromTrade() +
			/*	Don't expect to trade anything with a capitulated vassal that the
				master wouldn't trade */
			(bTheyCapitulated ? 0 : partnerUtilFromTech()) +
			partnerUtilFromMilitary();
	// Halved because can still enter their borders by force.
	if (kOurTeam.isOpenBorders(eTheirTeam))
		rPartnerUtil += iPartnerUtilFromOB / (bTheyCapitulated ? 4 : 2);
	scaled rUtility = rPartnerUtil * m_rAltPartnerFactor;
	// <!-- custom: Moved the inherited gift utility outside the help-memory branch so the consolidated partner row retains this component; its default zero records that no gift utility was added. (GPT-6.1-Sol) -->
	scaled rGiftUtility;
	// They may help us again in the future (if we don't go to war with them)
	if (kWe.AI_getMemoryCount(eThey, MEMORY_GIVE_HELP) > 0)
	{
		rGiftUtility = 5 * kWeAI.amortizationMultiplier();
		rUtility += rGiftUtility;
	}
	scaled const rHumanMult = fixp(0.81);
	// <!-- custom: Preserve the pre-human-adjustment utility so one result row can expose the inherited partner, alternative-partner, gift and human-forgiveness stages without recomputing them. (ChatGPT-5.6-Sol) -->
	scaled const rUtilityBeforeHuman = rUtility;
	if (kThey.isHuman())
		rUtility *= rHumanMult;
	if (kWe.isHuman())
		rUtility *= rHumanMult;
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted() && rUtility.abs() >= fixp(0.5)) logBBAI("UWAI_WAR_UTILITY_ILL_WILL_LOST_PARTNER turn=%d agentPlayer=%d rivalPlayer=%d capitulated=%d openBorders=%d partnerUtility=%d alternativePartnerPercent=%d giftUtility=%d utilityBeforeHuman=%d rivalHuman=%d agentHuman=%d humanMultiplierPercent=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, bTheyCapitulated, kOurTeam.isOpenBorders(eTheirTeam),
			rPartnerUtil.round(), m_rAltPartnerFactor.getPercent(), rGiftUtility.round(),
			rUtilityBeforeHuman.round(), kThey.isHuman(), kWe.isHuman(), rHumanMult.getPercent(),
			rUtility.round());
	m_rCost += rUtility;
}


void IllWill::evalRevenge()
{
	if (!militAnalyst().isWar(eWe, eThey) ||
		// Looks like they won't be a threat for long
		militAnalyst().lostCities(eThey).size() > 1)
	{
		return;
	}
	scaled const rTheirToOurPow = theirToOurPowerRatio();
	/*	Mainly care about the case where relations are worsened by our DoW;
		otherwise, PreEmptiveWar handles it. Special case however:
		They're far more powerful than we are, have already taken all our cities
		that were easy to reach and now we expect them to go after some easily
		reachable war ally of ours (who also won't stand a chance).
		So we're (presumably) safe for the moment, but staying at war is a
		long-term risk. Offering a token of goodwill might help and can't really hurt.
		This isn't a matter of PreEmptiveWar b/c the trend of the power ratio
		isn't relevant here - they'll always be stronger than us. */
	if (kOurTeam.isAtWar(eTheirTeam))
	{
		if (rTheirToOurPow > fixp(1.45) &&
			kTheirTeam.AI_getWarSuccess(eOurTeam) -
			kOurTeam.AI_getWarSuccess(eTheirTeam) >
			fixp(2.4) * GC.getWAR_SUCCESS_CITY_CAPTURING())
		{
			/*	If we expect to lose further cities to them,
				then they're not distracted; the Risk aspect will handle it. */
			CitySet const& kWeLose = militAnalyst().lostCities(eWe);
			for (CitySetIter it = kWeLose.begin(); it != kWeLose.end(); ++it)
			{
				if (militAnalyst().conqueredCities(eThey).count(*it) > 0)
					return;
			}
			// If they're not succeeding overall, then our coalition may yet beat them.
			if (kTheirTeam.AI_getWarSuccessRating() < 30)
				return;
			/*scaled rMult(kOurTeam.AI_getWarSuccessRating(), -10);
			rMult.clamp(3, fixp(6.5));*/ // Instead of the 5.5?
			scaled rHopelessStalemateCost = (1 - kWe.uwai().prideRating()) * fixp(5.5);
			if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_ILL_WILL_HOPELESS_STALEMATE turn=%d agentPlayer=%d rivalPlayer=%d powerRatioPercent=%d pridePercent=%d cost=%d",
					GC.getGame().getGameTurn(), eWe, eThey, rTheirToOurPow.getPercent(),
					kWe.uwai().prideRating().getPercent(), rHopelessStalemateCost.round());
			m_rCost += rHopelessStalemateCost;
		}
		return;
	}
	scaled rRevengeCost;
	/*	Even if they're not a major threat, they may well cause us some inconvenience,
		in particular, as part of a coalition. */
	if (rTheirToOurPow > fixp(0.7))
		rRevengeCost += 4;
	// If relations are already bad, a war won't make matters far worse.
	scaled rAttitudeMult = 1;
	if (kThey.isHuman())
		rAttitudeMult = fixp(0.67);
	else if (towardUs() <= ATTITUDE_FURIOUS)
		rAttitudeMult = fixp(0.2);
	else if (towardUs() == ATTITUDE_ANNOYED)
		rAttitudeMult = fixp(0.5);
	else if (towardUs() == ATTITUDE_CAUTIOUS)
		rAttitudeMult = fixp(0.85);
	scaled rPowRatioMult = rTheirToOurPow - 1;
	rPowRatioMult.clamp(0, 1);
	rRevengeCost += rPowRatioMult * 28 * rAttitudeMult;
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted() && rRevengeCost >= fixp(0.5)) logBBAI("UWAI_WAR_UTILITY_ILL_WILL_REVENGE turn=%d agentPlayer=%d rivalPlayer=%d powerRatioPercent=%d attitudeMultiplierPercent=%d powerMultiplierPercent=%d cost=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rTheirToOurPow.getPercent(),
			rAttitudeMult.getPercent(), rPowRatioMult.getPercent(), rRevengeCost.round());
	m_rCost += rRevengeCost;
}


scaled IllWill::theirToOurPowerRatio() const
{
	/*	Don't count vassals; they're not a reliable defense (and it's easier
		to exclude them). */
	scaled rOurPow = ourCache().getPowerValues()[ARMY]->power() +
			militAnalyst().gainedPower(eWe, ARMY);
	if (kThey.isHuman())
		rOurPow *= kWeAI.confidenceAgainstHuman();
	scaled rTheirPow = kThey.uwai().getCache().getPowerValues()[ARMY]->power() +
			militAnalyst().gainedPower(eThey, ARMY);
	if (kWe.isHuman())
		rTheirPow *= kThey.uwai().confidenceAgainstHuman();
	scaled r = rTheirPow / scaled::max(10, rOurPow);
	return r;
}


void IllWill::evalAngeredPartners()
{
	// When nothing can alienate them ...
	if (kThey.isHuman() || militAnalyst().isWar(eWe, eThey) || 100 * diploTowardUs() > 120 * GC.getDefineINT(CvGlobals::RELATIONS_THRESH_FRIENDLY) || eTheirTeam == eOurTeam)
	{
		return;
	}
	// 2 only for Gandhi; else 1.
	scaled const rPenaltyPerDoW = (m_kGame.isOption(GAMEOPTION_RANDOM_PERSONALITIES) ? 1 : -per100(GC.getInfo(kThey.getPersonalityType()).getMemoryAttitudePercent(MEMORY_DECLARED_WAR_ON_FRIEND)));
	scaled rPenalties;
	PlyrSet const& kWeDecl = militAnalyst().getWarsDeclaredBy(eWe);
	// <!-- custom: MilitaryAnalyst represents one declaration against a team through all of its player members, while the real war-on-friend memory is recorded once per victim team.
	// Deduplicate only that victim side; distinct defensive-pact target teams remain separate. See KI#462. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	TeamSet countedVictimTeams;
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	for (PlyrSetIter it = kWeDecl.begin(); it != kWeDecl.end(); ++it)
	{
		TeamTypes const eVictimTeam = TEAMID(*it);
		if (GET_TEAM(*it).isAVassal() || eTheirTeam == eVictimTeam || countedVictimTeams.count(eVictimTeam) > 0)
		{
			continue;
		}
		countedVictimTeams.insert(eVictimTeam);
		//if (kThey.AI_getAttitude(*it) >= ATTITUDE_PLEASED)
		if (kThey.AI_disapprovesOfDoW(eOurTeam, eVictimTeam)) // advc.130h
		{
			if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_ILL_WILL_ANGERED_PARTNER_DOW turn=%d agentPlayer=%d rivalPlayer=%d victimPlayer=%d victimTeam=%d relationPenalty=%d",
					GC.getGame().getGameTurn(), eWe, eThey, *it, eVictimTeam, rPenaltyPerDoW.floor());
			rPenalties += rPenaltyPerDoW;
		}
	}
	if (rPenalties <= 0)
		return;
	// -1 b/c barely Pleased could quickly tip to Cautious
	bool const bWillDisplease = (kThey.AI_getAttitudeFromValue(diploTowardUs() - rPenalties.floor() - 1) <= ATTITUDE_CAUTIOUS);
	scaled rTheirToOurPow = theirToOurPowerRatio();
	rTheirToOurPow.decreaseTo(1000); // avoid overflow
	scaled rCostPerPenalty = partnerUtilFromTrade() + partnerUtilFromTech() + partnerUtilFromMilitary() + (kOurTeam.isOpenBorders(eTheirTeam) ? iPartnerUtilFromOB : 0) + (bWillDisplease ? std::min(fixp(14.5), 9 * SQR(rTheirToOurPow)) : 0);
	// Don't worry quite as much about diplo in team games. AI DoW aren't as dynamic, and it's sufficient for tech trading if some team members get along.
	rCostPerPenalty /= ((bWillDisplease && towardUs() >= ATTITUDE_CAUTIOUS) ? fixp(3.75) : fixp(5.25)) * scaled(kOurTeam.getNumMembers()).sqrt();
	// costPerPenalty already adjusted to game progress, but want to dilute the impact of leader personality in addition to that. diploWeight is mostly about trading, and trading becomes less relevant in the latem_kGame.
	scaled rDiploWeight = 1 + (kWeAI.diploWeight() - 1) * kWeAI.amortizationMultiplier();
	// The bad diplo hurts us, but our anger at eThey is difficult to contain.
	if (!kWe.isHuman() && towardThem() <= ATTITUDE_FURIOUS)
		rDiploWeight /= 2;
	// We've actually one partner less b/c we're considering alternatives to a partner that (probably) isn't counted by preEvaluate as hostile.
	// <!-- custom: Name the exact partner and diplomatic-victory costs so the consolidated result reports the values added to m_rCost rather than recomputing them for logging. (ChatGPT-5.6-Sol) -->
	scaled const rPartnerCost = rCostPerPenalty * rPenalties * rDiploWeight * m_rAltPartnerFactor;
	m_rCost += rPartnerCost;
	// <!-- custom: Moved the inherited diplomatic-victory cost outside its eligibility branch so the consolidated row can report it alongside partner cost; its default zero means no victory cost was added. (GPT-6.1-Sol) -->
	scaled rVictoryCost;
	if (towardUs() >= ATTITUDE_PLEASED && kWe.AI_atVictoryStage(AI_VICTORY_DIPLOMACY4 | AI_VICTORY_DIPLOMACY3))
	{
		scaled rVictoryFactor(kThey.getTotalPopulation(), std::max(1, m_kGame.getTotalPopulation()));
		rVictoryCost = 25 * rVictoryFactor * rPenalties;
		m_rCost += rVictoryCost;
	}
	if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_ILL_WILL_ANGERED_PARTNER_RESULT turn=%d agentPlayer=%d rivalPlayer=%d relationPenalties=%d willDisplease=%d powerRatioPercent=%d costPerPenalty=%d diploWeightPercent=%d alternativePartnerPercent=%d partnerCost=%d victoryCost=%d totalAddedCost=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rPenalties.round(), bWillDisplease,
			rTheirToOurPow.getPercent(), rCostPerPenalty.round(), rDiploWeight.getPercent(),
			m_rAltPartnerFactor.getPercent(), rPartnerCost.round(), rVictoryCost.round(),
			(rPartnerCost + rVictoryCost).round());
}


Affection::Affection(WarEvalParameters const& kParams) : WarUtilityAspect(kParams)
{
	m_rGameProgressFactor = 1 - fixp(0.2) * m_kGame.gameTurnProgress();
}


void Affection::evaluate()
{
	// Humans are unaffectionate
	if (kWe.isHuman() ||
		// No qualms about attacking capitulated vassals
		kOurTeam.isCapitulated() || kTheirTeam.isCapitulated() ||
		!militAnalyst().isWar(eWe, eThey) || kOurTeam.AI_getWorstEnemy() == eTheirTeam)
	{
		return;
	}
	scaled rWarImminentMult = 1;
	WarPlanTypes const eWP = kOurTeam.AI_getWarPlan(eTheirTeam);
	/*	Once we've adopted a direct warplan, affection is greatly reduced -- a civ
		that'll only please us when it sees our soldiers approach isn't a true friend. */
	if (eWP != WARPLAN_PREPARING_LIMITED && eWP != WARPLAN_PREPARING_TOTAL &&
		eWP != NO_WARPLAN)
	{
		rWarImminentMult = scaled::min(1,
				fixp(0.085) * scaled::max(1,
				/*	If war stays imminent for a long time, and relations remain good,
					affection should matter again. Adjust a bit for game progress b/c
					turns feel longer in the late game. (This runs counter to the
					progress adjustment in the end.) If a human player _feels_ that
					relations have been good for quite some time when war is declared,
					then that's a problem.
					Take into account the time spent on preparations? That would reflect the
					sunk cost of the AI and would also tend to be shorter in the late game.
					But that duration isn't currently kept track of by CvTeamAI ... */
				kOurTeam.AI_getWarPlanStateCounter(eTheirTeam)
				- 4 * (fixp(2.5) * m_rGameProgressFactor - fixp(1.5))));
	}
	// We're not fully responsible for wars triggered by our DoW
	scaled rLinkedWarMult;
	if (militAnalyst().getWarsDeclaredBy(eWe).count(eThey) > 0)
	{
		if (kTheirTeam.isAVassal()) // non-capitulated
			rLinkedWarMult = fixp(0.25);
		else rLinkedWarMult = 1;
	}
	/*	We're also somewhat responsible for wars declared on us through
		defensive pacts (DP). No need to check for current DP b/c a DP is
		the only way that a DoW on us can happen in the military analysis
		(all other DoW on us are unforeseen). */
	if (militAnalyst().getWarsDeclaredBy(eThey).count(eWe) > 0)
		rLinkedWarMult = fixp(0.4);
	if (rLinkedWarMult <= 0)
		return;
	int iNoWarPercent = kOurTeam.AI_noWarProbAdjusted(eTheirTeam);
	scaled rUtility; // (Positive value; gets subtracted in the end.)
	// <!-- custom: Keep the applied vassal reduction and uncertainty adjustment as named components so one result row can report the exact affection calculation without separate prose messages. (ChatGPT-5.6-Sol) -->
	int iVassalPenalty = 0;
	if (iNoWarPercent > 0) // for efficiency
	{
		/*	Capitulated vassals don't interest us, but a voluntary vassal can
			reduce our affection for its master. The Affection aspect also gets
			evaluated with eThey set to the vassal, but that only accounts for
			our scruples about attacking the vassal.
			There's already a diplo penalty for having an unpopular vassal,
			so I'm only applying a minor penalty here. */
		scaled rVassalPenalty;
		for (PlayerIter<MAJOR_CIV,VASSAL_OF> itTheirVassal(eTheirTeam);
			itTheirVassal.hasNext(); ++itTheirVassal)
		{
			PlayerTypes const eTheirVassal = itTheirVassal->getID();
			if (GET_TEAM(eTheirVassal).isCapitulated())
				continue;
			int iVassalNoWarPercent = kOurPersonality.getNoWarAttitudeProb(
					kWe.AI_getAttitude(eTheirVassal));
			int iDelta = iNoWarPercent - iVassalNoWarPercent;
			if (iDelta > 0)
				rVassalPenalty += scaled(iDelta, 10);
		}
		iVassalPenalty = rVassalPenalty.uround();
		if (iVassalPenalty > 0)
			iNoWarPercent = std::max(iNoWarPercent / 2, iNoWarPercent - iVassalPenalty);
		//rUtility = per100(iNoWarPercent).pow(fixp(5.5)) * 75;
		// ^That progression is a bit too steep
		// The new formula would go toward infinity for high iNoWarPercent
		int const iNoWarPercentCapped = std::min(iNoWarPercent, 94);
		/*	Subtract 1 so that iNoWarPercent=0 yields 0. The exponent and
			dividend (1) are magic constants. */
		rUtility += 1 / (1 - per100(iNoWarPercentCapped).pow(fixp(0.223)));
		rUtility -= 1; // So that iNoWarPercent=0 yields 0
		rUtility += iNoWarPercent - iNoWarPercentCapped;

	}
	if (towardThem() >= ATTITUDE_FRIENDLY)
		rUtility += 34; // 40 prior to AdvCiv 0.97
	bool const bIgnDistr = m_kParams.isIgnoreDistraction();
	/*	The Catherine clause - doesn't make sense for her to consider sponsored
		war on a friend if the cost is always prohibitive. */
	bool const bHiredAgainstFriend = (towardThem() >= ATTITUDE_FRIENDLY &&
			(m_kParams.getSponsor() != NO_PLAYER ||
			/*	The computations ignoring distraction are also used for decisions
				on joint wars (see UWAI::Team::declareWarTrade) */
			bIgnDistr));
	if (bHiredAgainstFriend)
		rUtility = 50;
	rUtility *= rLinkedWarMult * rWarImminentMult * m_rGameProgressFactor;
	// <!-- custom: Moved the inherited uncertainty adjustment outside its branch so the final affection row reports the exact signed amount added; scaled defaults to zero when no uncertainty adjustment applies. (GPT-6.1-Sol) -->
	scaled rUncertainVal;
	// When there's supposed to be uncertainty
	if (!bIgnDistr && ((iNoWarPercent > 0 && iNoWarPercent < 100) || bHiredAgainstFriend))
	{
		vector<int> aiInputs;
		aiInputs.push_back(eThey);
		aiInputs.push_back(towardThem());
		aiInputs.push_back(kWe.AI_getMemoryCount(eThey, MEMORY_DECLARED_WAR));
		aiInputs.push_back(kThey.AI_getMemoryCount(eWe, MEMORY_DECLARED_WAR));
		scaled rHashVal = scaled::hash(aiInputs, eWe);
		scaled const rUncertaintyBound = 12;
		rUncertainVal = rHashVal * std::min(2 * rUncertaintyBound, rUtility) -
				std::min(rUncertaintyBound, fixp(0.5) * rUtility);
		rUtility += rUncertainVal;
		FAssert(rUtility >= 0);
	}
	/*	In team games, the pairwise affection adds up more than the
		pairwise conquests; need to correct that a bit. */
	if (kOurTeam.getNumMembers() > 1)
		rUtility = normalizeUtility(rUtility) * kOurTeam.getNumMembers() * fixp(2/3.);
	// <!-- custom: Keep the inherited >= 0.5 application threshold: iUtility remains zero when no penalty is applied, so the consolidated row can report other adjustments without implying that their rounded result changed gameplay. (GPT-6.1-Sol) -->
	int iUtility = 0;
	if (rUtility >= fixp(0.5))
	{
		iUtility = rUtility.round();
		m_iU -= iUtility;
	}
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted() && (iVassalPenalty > 0 || rUncertainVal.abs() >= fixp(0.5) || iUtility != 0)) logBBAI("UWAI_WAR_UTILITY_AFFECTION_RESULT turn=%d agentPlayer=%d rivalPlayer=%d warPlan=%d attitude=%d noWarPercent=%d vassalPenalty=%d hiredAgainstFriend=%d linkedWarPercent=%d imminentWarPercent=%d gameProgressPercent=%d uncertainty=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, eWP, towardThem(), iNoWarPercent,
			iVassalPenalty, bHiredAgainstFriend, rLinkedWarMult.getPercent(),
			rWarImminentMult.getPercent(), m_rGameProgressFactor.getPercent(),
			rUncertainVal.round(), -iUtility);
}


// <!-- custom: Reset hostile-team ownership for each agent member's distinct military analysis. See KI#449. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
int Distraction::preEvaluate()
{
	m_evaluatedTeams.clear();
	return 0;
}


void Distraction::evaluate()
{
	if (kOurTeam.isAVassal() || kTheirTeam.isAVassal() ||
		!militAnalyst().isWar(eWe, eThey) ||
		m_evaluatedTeams.count(eTheirTeam) > 0)
	{
		return;
	}
	// <!-- custom: One team war creates one alternative-war opportunity cost.
	// Consume it once per agent member when either side can reach through any living target member, rather than once for every reachable teammate. See KI#449. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	m_evaluatedTeams.insert(eTheirTeam);
	bool bCanReach = false;
	for (MemberIter itMember(eTheirTeam); itMember.hasNext(); ++itMember)
	{
		if (kWeAI.canReach(itMember->getID()) || GET_PLAYER(itMember->getID()).uwai().canReach(eWe))
		{
			bCanReach = true;
			break;
		}
	}
	if (!bCanReach)
	{
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_DISTRACTION_UNREACHABLE turn=%d agentPlayer=%d rivalPlayer=%d rivalTeam=%d",
				GC.getGame().getGameTurn(), eWe, eThey, eTheirTeam);
		return;
	}
	FAssert(!m_kParams.isIgnoreDistraction());
	int const iWarDuration = kOurTeam.AI_getAtWarCounter(eTheirTeam);
	// Utility ignoring Distraction cost
	scaled const rWarUtilityVsThem = normalizeUtility(ourCache().warUtilityIgnoringDistraction(eTheirTeam));
	scaled rDistractionCost;
	int iAltWars = 0;
	scaled rTotalOpportunityCost;
	scaled rHighestOpportunityCost;
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	for (TeamIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itAltTarget(eOurTeam);
		itAltTarget.hasNext(); ++itAltTarget)
	{
		TeamTypes const eAltTarget = itAltTarget->getID();
		if (eAltTarget == eTheirTeam ||
			kOurTeam.AI_isPushover(eAltTarget) ||
			(!kOurTeam.uwai().canReach(eAltTarget) &&
			!GET_TEAM(eAltTarget).uwai().canReach(eOurTeam)))
		{
			continue;
		}
		scaled rWarUtilityVsAlt = normalizeUtility(
				ourCache().warUtilityIgnoringDistraction(eAltTarget), eAltTarget);
		if (rWarUtilityVsAlt >= 50 && kOurTeam.isAtWar(eTheirTeam))
		{
			if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_DISTRACTION_ALT_HIGH_UTILITY turn=%d agentPlayer=%d rivalPlayer=%d alternativeTeam=%d alternativeUtility=%d",
					GC.getGame().getGameTurn(), eWe, eThey, eAltTarget, rWarUtilityVsAlt.round());
			continue;
		}
		/*	The war against eThey distracts us from our war against eAltTarget.
			The comparison between war and peace scenario should cover this kind of
			distraction - but doesn't always b/c military analysis can have
			counterintuitive outcomes with greater losses in the peace scenario.
			Needs a little extra nudge. */
		if (kOurTeam.AI_getWarPlan(eAltTarget) != NO_WARPLAN)
		{
			scaled const rPlanBaseCost = fixp(5.5);
			rDistractionCost += rPlanBaseCost;
			/*	The cached value is for limited war in 5 turns, which isn't
				necessarily the best war plan against eAltTarget. */
			rWarUtilityVsAlt += fixp(7.5);
			// <!-- custom: Keep the optional preparation-only increment outside its branch so the consolidated active-plan row can report zero when it did not apply. (ChatGPT-5.6-Sol) -->
			scaled rPreparationExtraCost;
			if (rWarUtilityVsAlt >= fixp(0.5) &&
				!militAnalyst().isOnTheirSide(eAltTarget, true) &&
				!kOurTeam.isAtWar(eAltTarget))
			{
			/*	This means, war against eAltTarget is still in preparation or imminent,
				and we're considering peace with eThey; or there's a special offer
				(sponsored or diplo vote) to declare war on eThey. (Not possible:
				preparations against eAltTarget and eThey at the same time.) */
				rPreparationExtraCost = rWarUtilityVsAlt;
				rDistractionCost += rPreparationExtraCost;
			}
			if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_DISTRACTION_ALT_ACTIVE_PLAN turn=%d agentPlayer=%d rivalPlayer=%d alternativeTeam=%d adjustedAlternativeUtility=%d basePlanCost=%d preparationExtraCost=%d totalAddedCost=%d",
					GC.getGame().getGameTurn(), eWe, eThey, eAltTarget, rWarUtilityVsAlt.round(),
					rPlanBaseCost.round(), rPreparationExtraCost.round(),
					(rPlanBaseCost + rPreparationExtraCost).round());
			// NB: Imminent war against eAltTarget is covered by UWAI::Team::considerPeace
		}
		/*	eAltTarget as a potential alternative war target. rOurWarUtility is the
			utility of fighting both eThey and eAltTarget. Won't be able to tell
			how worthwhile war against eAltTarget really is until we make peace
			with eThey. Assume that we wouldn't have started a war against eThey
			if eAltTarget was the better target. But the longer the war against
			eThey lasts, the likelier a change in circumstances becomes. */
		else if (kOurTeam.isAtWar(eTheirTeam) &&
			// If eAltTarget==getTarget, then eAltTarget isn't an "alternative".
			m_kParams.getTarget() != eAltTarget &&
			/*	If there's a peace treaty, then we probably
				don't want to attack them urgently. */
			kOurTeam.canDeclareWar(eAltTarget) &&
			rWarUtilityVsAlt > -iWarDuration)
		{
			scaled rOpportunityCost = iWarDuration +
					rWarUtilityVsAlt - scaled::max(0, rWarUtilityVsThem);
			rOpportunityCost /= fixp(1.55);
			if (rOpportunityCost >= fixp(0.5))
			{
				rOpportunityCost.decreaseTo(15);
				if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_DISTRACTION_ALT_OPPORTUNITY turn=%d agentPlayer=%d rivalPlayer=%d alternativeTeam=%d warDuration=%d currentWarUtility=%d alternativeUtility=%d opportunityCost=%d",
						GC.getGame().getGameTurn(), eWe, eThey, eAltTarget, iWarDuration,
						rWarUtilityVsThem.round(), rWarUtilityVsAlt.round(), rOpportunityCost.round());
				rHighestOpportunityCost.increaseTo(rOpportunityCost);
				rTotalOpportunityCost += rOpportunityCost;
				iAltWars++;
			}
		}
	}
	// <!-- custom: Keep the combined opportunity cost available for the final distraction row; scaled defaults to zero when no alternative war contributes. (ChatGPT-5.6-Sol) -->
	scaled rOverallOpportunityCost;
	if (iAltWars > 0)
	{
		/*	We're going start at most one of the potential wars, but having
			several candidates should be an extra incentive for freeing our hands. */
		rOverallOpportunityCost = rHighestOpportunityCost +
				(rTotalOpportunityCost - rHighestOpportunityCost) /
				scaled(iAltWars).sqrt();
		if (rOverallOpportunityCost >= fixp(0.5))
			rDistractionCost += rOverallOpportunityCost;
	}
	// <!-- custom: Base AdvCiv UWAI could abandon a decisively successful war because alternative-war opportunities outweighed the current war.
	// In save file 452, Mali still had 7 cities vs Maya's 3, 891 vs 397 power and an 83-point war-success lead, while the current war remained profitable before Distraction; nevertheless, a new India preparation raised Distraction from -18 to -96 and caused immediate peace.
	// The first prototype capped only the preparation-specific portion, reducing Distraction to -75, but Mali still accepted peace on the next turn. A 50-percent cap on the whole cost kept Mali at war long enough to capture another city, but its turn-161 review remained negative and only skipped peace by chance. When the current enemy is clearly weaker and losing, apply the stricter tunable cap relative to the cached positive value of the current war; effort, risk, war weariness and emergency multi-war peace remain unchanged. (GPT-5.6-Sol) -->
	// <!-- custom: Retain the optional winning-war and almost-finished adjustments as named result components so the final row reports the exact path to the applied distraction cost. (ChatGPT-5.6-Sol) -->
	int iTargetPowerPercent = -1;
	scaled rWarSuccessLead;
	scaled rCostBeforeWinningCap = rDistractionCost;
	bool bWinningCapApplied = false;
	if (rDistractionCost > 0 && rWarUtilityVsThem > 0 && kOurTeam.getNumCities() > kTheirTeam.getNumCities())
	{
		static int const iMaxTargetPowerPercent = GC.getDefineINT("SAS_UWAI_DISTRACTION_WINNING_WAR_MAX_TARGET_POWER_PERCENT");
		static int const iMaxCostPercent = GC.getDefineINT("SAS_UWAI_DISTRACTION_WINNING_WAR_MAX_COST_PERCENT");
		iTargetPowerPercent = 100 * kTheirTeam.getPower(true) / std::max(1, kOurTeam.getPower(true));
		rWarSuccessLead = kOurTeam.AI_getWarSuccess(eTheirTeam) - kTheirTeam.AI_getWarSuccess(eOurTeam);
		if (iTargetPowerPercent <= iMaxTargetPowerPercent && rWarSuccessLead >= GC.getWAR_SUCCESS_CITY_CAPTURING())
		{
			rDistractionCost.decreaseTo(rWarUtilityVsThem * per100(iMaxCostPercent));
			bWinningCapApplied = (rDistractionCost < rCostBeforeWinningCap);
		}
	}
	/*	If we expect to knock them out, the current war may be over before the
		time horizon of the simulation; then it's a smaller distraction.
		(InvasionGraph could store elimination timestamps, but that seems too much
		work to implement. Can guess the time well enough here by counting their
		remaining cities.) */
	// <!-- custom: The war is almost finished only when every living target member is eliminated in the simulation or the target team capitulates.
	// Apply the reduction to the one team-war cost using the team's combined current cities. See KI#449. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	bool bTheirTeamEliminated = true;
	for (MemberIter itMember(eTheirTeam); itMember.hasNext(); ++itMember)
	{
		if (!militAnalyst().isEliminated(itMember->getID()))
		{
			bTheirTeamEliminated = false;
			break;
		}
	}
	// <!-- custom: Moved the inherited almost-finished multiplier outside its branch for the final result row. Record the applied factor: 1 means unchanged cost, including cases where the candidate factor is >= 1 and the inherited code applies no reduction. (GPT-6.1-Sol) -->
	scaled rAlmostDoneMult = 1;
	if (rDistractionCost > 0 && (bTheirTeamEliminated || militAnalyst().getCapitulationsAccepted(eOurTeam).count(eTheirTeam) > 0))
	{
		rAlmostDoneMult = scaled(kTheirTeam.getNumCities(), 3);
		if (rAlmostDoneMult < 1)
			rDistractionCost *= rAlmostDoneMult;
		else rAlmostDoneMult = 1;
	}
	int const iUtility = rDistractionCost.round();
	if (bLogWarUtilityDetail && iUtility != 0) logBBAI("UWAI_WAR_UTILITY_DISTRACTION_RESULT turn=%d agentPlayer=%d rivalPlayer=%d rivalTeam=%d warDuration=%d currentWarUtility=%d alternativeWarCount=%d overallOpportunityCost=%d costBeforeWinningCap=%d winningCapApplied=%d targetPowerPercent=%d warSuccessLead=%d teamEliminated=%d capitulationExpected=%d almostDonePercent=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, eTheirTeam, iWarDuration, rWarUtilityVsThem.round(),
			iAltWars, rOverallOpportunityCost.round(), rCostBeforeWinningCap.round(), bWinningCapApplied,
			iTargetPowerPercent, rWarSuccessLead.round(), bTheirTeamEliminated,
			militAnalyst().getCapitulationsAccepted(eOurTeam).count(eTheirTeam) > 0,
			rAlmostDoneMult.getPercent(), -iUtility);
	m_iU -= iUtility;
}


void PublicOpposition::evaluate()
{
	if (kWe.isAnarchy() || !militAnalyst().isWar(eWe, eThey) ||
		militAnalyst().isEliminated(eWe))
	{
		return;
	}
	int iTotalPop = kWe.getTotalPopulation();
	if (iTotalPop <= 0)
		return;
	scaled rFaithAnger;
	FOR_EACH_CITY(pCity, kWe)
	{
		if (pCity->isDisorder())
			continue;
		scaled rAngry = pCity->angryPopulation(0,
				!kWe.AI_atVictoryStage(AI_VICTORY_CULTURE3));
		rFaithAnger += std::min(rAngry,
				pCity->getReligionPercentAnger(eThey) * pCity->getPopulation() /
				GC.getPERCENT_ANGER_DIVISOR());
	}
	// Known issue: This is 0 if not currently at war with them
	scaled rWWAnger = ourCache().angerFromWarWeariness(eThey);
	if (rWWAnger + rFaithAnger <= 0)
		return;
	// Assume that more WW is coming, and especially if we take the fight to them.
	WarPlanTypes const eWarPlan = kOurTeam.AI_getWarPlan(eTheirTeam);
	bool bTotal = (eWarPlan == WARPLAN_PREPARING_TOTAL || eWarPlan == WARPLAN_TOTAL);
	if (m_kParams.getTarget() == eTheirTeam && m_kParams.isTotal())
		bTotal = true;
	scaled rExtraAngerPortion = (bTotal ? fixp(0.5) : fixp(0.35));
	rExtraAngerPortion += (fixp(1.5) * ourConquestsFromThem().size()) /
			std::max(kWe.getNumCities(), 1);
	// <!-- custom: Keep the inherited war-weariness forecast as an explicit component of the final opposition result instead of a separate prose line. (ChatGPT-5.6-Sol) -->
	scaled const rAngerRate = (rWWAnger * (1 + rExtraAngerPortion) + rFaithAnger) /
			iTotalPop;
	if (rAngerRate <= 0)
		return;
	scaled rAngerCost = 125 * rAngerRate;
	// Don't go too high
	rAngerCost.exponentiate(fixp(0.75));
	rAngerCost *= 2; // (normalization factor)
	/*	Shouldn't be much of a deterrent when playing for military victory.
		(Letting the utility counted for the MilitaryVictory aspect cancel out
		the PublicOpposition isn't quite working out, I think, b/c the
		latter aspect is being overvalued so that it can serve as a safeguard
		against interminable wars.) */
	// <!-- custom: Track the product of the inherited stage-3/stage-4 divisors for the result row while keeping the original sequential scaled divisions, whose rounding need not equal one division by their product. (GPT-6.1-Sol) -->
	int iMilitaryVictoryDivisor = 1;
	if (kWe.AI_atVictoryStage(AI_VICTORY_MILITARY3))
	{
		int const iDiv = (kWe.isHuman() ? 3 : 2); // Humans are especially goal-driven
		iMilitaryVictoryDivisor *= iDiv;
		rAngerCost /= iDiv;
		if (kWe.AI_atVictoryStage(AI_VICTORY_MILITARY4))
		{
			iMilitaryVictoryDivisor *= iDiv;
			rAngerCost /= iDiv;
		}
	}
	int const iUtility = rAngerCost.round();
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_PUBLIC_OPPOSITION_RESULT turn=%d agentPlayer=%d rivalPlayer=%d faithAnger=%d warWearinessAnger=%d totalPopulation=%d totalWar=%d conqueredCities=%d extraWarWearinessPercent=%d angerRatePercent=%d militaryVictoryDivisor=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, rFaithAnger.round(), rWWAnger.round(),
			iTotalPop, bTotal, (int)ourConquestsFromThem().size(), rExtraAngerPortion.getPercent(),
			rAngerRate.getPercent(), iMilitaryVictoryDivisor, -iUtility);
	m_iU -= iUtility;
}


// <!-- custom: The old rival loop preserved city identities across calls but reset the loss numerator and asset denominator for each rival, making the sum depend on which rival encountered a city first.
// Build one scenario-wide city union and one matching ratio instead. See KI#455. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
int Revolts::preEvaluate()
{
	scaled rLossesFromRevolts;
	int iTotalAssets = 0;
	std::set<PlotNumTypes> countedCities;
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	for (PlayerIter<MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itRival(eOurTeam); itRival.hasNext(); ++itRival)
	{
		CvPlayerAI const& kRival = GET_PLAYER(itRival->getID());
		TeamTypes const eRivalTeam = kRival.getTeam();
		// <!-- custom: Preserve the old per-rival admission rules while forming the scenario union: only analysis participants in a simulated non-pushover war can contribute primary areas. See KI#455. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if (!militAnalyst().isPartOfAnalysis(kRival.getID()) || !militAnalyst().isWar(eWe, kRival.getID()) || kOurTeam.AI_isPushover(eRivalTeam))
			continue;
		FOR_EACH_AREA(pArea)
		{
			if (!kRival.AI_isPrimaryArea(*pArea))
				continue;
			if (kOurTeam.isAtWar(eRivalTeam)) // Can't rely on AreaAIType otherwise
			{
				AreaAITypes const eAreaAI = pArea->getAreaAIType(eOurTeam);
				// Training defenders is the CityAI's best remedy for revolts
				if (eAreaAI == AREAAI_DEFENSIVE || eAreaAI == AREAAI_NEUTRAL || eAreaAI == NO_AREAAI)
					continue;
			}
			FOR_EACH_CITY(pCity, kWe)
			{
				// <!-- custom: Accept a city through any relevant rival area, but count it only once in this agent member's local scenario union; unlike the removed member set, this identity state cannot leak into a later agent evaluation. See KI#455. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				if (!pCity->isArea(*pArea) || countedCities.count(pCity->plotNum()) > 0)
					continue;
				City const* pCacheCity = ourCache().lookupCity(pCity->plotNum());
				if (pCacheCity == NULL)
					continue;
				countedCities.insert(pCity->plotNum());
				// <!-- custom: Per-city asset scores can be negative after maintenance, but possible revolt damage is a loss rather than a benefit. Use zero threatened value for such cities so they neither create positive war utility nor cancel another city's expected revolt losses. See KI#441. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				int const iCityAssets = std::max(0, pCacheCity->getAssetScore());
				iTotalAssets += iCityAssets;
				scaled rRevoltProb = pCity->revoltProbability(false, false, true);
				if (rRevoltProb <= 0)
					continue;
				/*	If we've been failing to suppress a city for a long time,
					then it's probably not mainly a matter of distracted units. */
				if (pCity->getNumRevolts() > GC.getDefineINT(CvGlobals::NUM_WARNING_REVOLTS))
				{
					if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_REVOLT_CITY_SKIPPED turn=%d agentPlayer=%d cityPlot=%d revoltPercent=%d assetScore=%d revoltCount=%d warningRevoltThreshold=%d reason=HOPELESS",
							GC.getGame().getGameTurn(), eWe, pCity->plotNum(), rRevoltProb.getPercent(),
							iCityAssets, pCity->getNumRevolts(), GC.getDefineINT(CvGlobals::NUM_WARNING_REVOLTS));
					continue;
				}
				scaled rLossMult = 5 * std::min(fixp(0.1), rRevoltProb);
				if (rLossMult > 0)
				{
					scaled const rExpectedLoss = iCityAssets * rLossMult;
					if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_REVOLT_CITY_RISK turn=%d agentPlayer=%d cityPlot=%d revoltPercent=%d assetScore=%d lossMultiplierPercent=%d expectedLoss=%d",
							GC.getGame().getGameTurn(), eWe, pCity->plotNum(), rRevoltProb.getPercent(),
							iCityAssets, rLossMult.getPercent(), rExpectedLoss.round());
					rLossesFromRevolts += rExpectedLoss;
				}
			}
		}
	}
	if (iTotalAssets <= 0)
		return 0;
	/*	Dilute amortizationMultiplier because revolts make cities less useful
		economically, but can also disrupt victory conditions. */
	scaled rRevoltCost = (1 + kWeAI.amortizationMultiplier()) *
			50 * rLossesFromRevolts / iTotalAssets;
	return -rRevoltCost.round();
}


// <!-- custom: Revolts now returns its scenario-wide result from preEvaluate; leave the mandatory per-rival hook empty so the same union is not charged again for every rival. See KI#455. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void Revolts::evaluate() {}


void UlteriorMotives::evaluate()
{
	if (kWe.isHuman())
		return;
	/*	Difference between ourCache().sponsorAgainst(t) and
		m_kParams.getSponsor() returning t:
		The former tells us that our war against t has been sponsored; so, we've
		already agreed to the war trade. The latter says that we're currently
		considering war against t. This aspect needs to apply when considering the
		offer. Once we've declared war, we should forget about our issues with the
		sponsor's motives, and instead live up to our promise (as enforced by
		HiredHand). */
	if (militAnalyst().isPeaceScenario() || eThey != m_kParams.getSponsor() || !kWe.hasCapital() || !kThey.hasCapital())
	{
		return;
	}
	if (kOurTeam.AI_getWarPlan(eTheirTeam) != NO_WARPLAN)
	{
		/*	They're probably trying to deflect our attack. That doesn't have to
			be bad for us; Distraction cost accounts for the difference in
			war utility vs. sponsor and target. But if they're willing to pay us off,
			we should ask for sth. extra. And don't want to make it too easy
			for humans to avoid wars. */
		FAssert(!m_kParams.isIgnoreDistraction()); // Sponsor should be NO_PLAYER then
		int const iUtilityVsThem = ourCache().warUtilityIgnoringDistraction(eTheirTeam);
		int const iUtility = std::max(5, iUtilityVsThem / 2);
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_ULTERIOR_MOTIVES_DEFLECTION turn=%d agentPlayer=%d sponsorPlayer=%d sponsorTeam=%d sponsorWarUtility=%d utility=%d",
				GC.getGame().getGameTurn(), eWe, eThey, eTheirTeam, iUtilityVsThem, -iUtility);
		m_iU -= iUtility;
		return;
	}
	TeamTypes const eTarget = m_kParams.getTarget();
	CvTeamAI const& kTarget = GET_TEAM(eTarget);
	bool const bJointWar = militAnalyst().isWar(eTheirTeam, eTarget);
	// If they're in a hot war, their motives seem clear (and honest) enough.
	bool const bHot = (bJointWar && kTarget.AI_getWarSuccess(eTheirTeam) + 2 * kTheirTeam.AI_getWarSuccess(eTarget) > 3 * GC.getWAR_SUCCESS_CITY_CAPTURING() && kTarget.uwai().canReach(eTheirTeam)); // Perhaps no longer hot
	// If the target is in our area but not theirs, that's fishy.
	if (bHot && (!kTarget.AI_isPrimaryArea(kWe.getCapital()->getArea()) || kTarget.AI_isPrimaryArea(kThey.getCapital()->getArea())))
	{
		return;
	}
	/*	Otherwise, they might want to hurt us as much as the target, and we should
		demand extra payment to allay our suspicions. Use DWRAT (greater than
		Annoyed, Cautious or Pleased respectively) as an indicator of how
		suspicious we are. */
	int iSuspicionFactor = kOurPersonality.getDeclareWarRefuseAttitudeThreshold() - towardThem() + 1;
	if (towardThem() == ATTITUDE_FRIENDLY)
		iSuspicionFactor--;
	int iMotivesCost = 8 + 4 * iSuspicionFactor;
	if (!bJointWar)
		iMotivesCost += 4;
	// An AI sponsor can, in principle, have ulterior motives. For example, if they expect the war to hurt us, this could result in positive utility from Kingmaking for them. This is a bit farfetched though, so ...
	if (!bHot && !kThey.isHuman())
		iMotivesCost /= 2;
	if (iMotivesCost > 0)
	{
		// <!-- custom: Keep the refusal threshold and suspicion inputs in the same structured row as the exact motives cost they produce. AdvCiv's report omitted the refusal-threshold vararg, producing undefined output when reporting was enabled; preserve the corrected argument. See KI#431. (ChatGPT-5.6-Sol + GPT-5.6-Sol + GPT-6.1-Sol) -->
		if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_ULTERIOR_MOTIVES_SUSPICION turn=%d agentPlayer=%d sponsorPlayer=%d targetTeam=%d jointWar=%d hotWar=%d attitude=%d refusalThreshold=%d suspicionFactor=%d sponsorHuman=%d utility=%d",
				GC.getGame().getGameTurn(), eWe, eThey, eTarget, bJointWar, bHot, towardThem(),
				kOurPersonality.getDeclareWarRefuseAttitudeThreshold(), iSuspicionFactor, kThey.isHuman(), -iMotivesCost);
		m_iU -= iMotivesCost;
	}
}


void FairPlay::evaluate()
{
	if (kWe.isHuman() || // Don't expect humans to pull punches
		kTheirTeam.isAVassal() ||
		militAnalyst().getWarsDeclaredBy(eWe).count(eThey) <= 0 ||
		// No kid gloves if they've attacked us recently (or repeatedly)
		kWe.AI_getMemoryAttitude(eThey, MEMORY_DECLARED_WAR) < -2 ||
		kThey.AI_atVictoryStage3() ||
		// Then our attack dooms them regardless of other war parties
		(kOurTeam.AI_isPushover(eTheirTeam) &&
		(!kThey.isHuman() || kThey.getCurrentEra() > 0)) ||
		/*	If they can't win anymore, we shouldn't hold back. Don't want to
			leave all the loot to others. A human with such poor war success
			isn't going to win either, but if the human is a good sport and
			keeps playing, the AI shouldn't punish that. */
		(!kThey.isHuman() && kTheirTeam.AI_getWarSuccessRating() <= -60))
	{
		return;
	}
	// Avoid big dogpiles (or taking turns attacking a single target)
	scaled rOtherEnemies; // Apart from us
	int iPotentialOtherEnemies = 0;
	int const iTheirRank = m_kGame.getPlayerRank(eThey);
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	for (PlayerAIIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itOther(eOurTeam);
		itOther.hasNext(); ++itOther)
	{
		CvPlayerAI const& kOther = *itOther;
		// <!-- custom: A teammate of the target is not an independent third party. Counting one inflated the potential-enemy denominator and could even classify that teammate as the target's own ally. See KI#437. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if (kOther.getTeam() == eTheirTeam || !kTheirTeam.isHasMet(kOther.getTeam()))
			continue;
		iPotentialOtherEnemies++;
		bool const bWar = (kTheirTeam.isAtWar(kOther.getTeam()) ||
				militAnalyst().getWarsDeclaredBy(kOther.getID()).count(eThey) > 0);
		int const iTheirWarMemoryAttitude = kThey.AI_getMemoryAttitude(kOther.getID(),
				MEMORY_DECLARED_WAR);
		// <!-- custom: Keep the mutually compatible third-party adjustments as separate zero-default components so one row can show the exact contribution from this player without repeated prose. (ChatGPT-5.6-Sol) -->
		scaled rEnemyWeight;
		scaled rAllyReduction;
		scaled rPriorAttackReduction;
		if (bWar || iTheirWarMemoryAttitude <= -2)
		{
			rEnemyWeight = fixp(1.15);
			if (!bWar)
			{
				rEnemyWeight *= fixp(0.73);
				if (iTheirWarMemoryAttitude == -2)
					rEnemyWeight *= fixp(0.85);
			}
			int iOtherRank = m_kGame.getPlayerRank(kOther.getID());
			// Ganging up on the leader is less problematic
			if (iTheirRank < iOtherRank)
				rEnemyWeight *= scaled(iTheirRank + 1, iOtherRank + 1);
			rOtherEnemies += rEnemyWeight;
		}
		else if (kTheirTeam.AI_shareWar(kOther.getTeam()))
		{
			rAllyReduction = scaled(1,
					std::max(1, GET_TEAM(kOther.getTeam()).getNumWars(true, true)));
			rOtherEnemies -= rAllyReduction;
		}
		if (kOther.AI_getMemoryAttitude(eThey, MEMORY_DECLARED_WAR) <= -2)
		{
			rPriorAttackReduction = fixp(0.5);
			rOtherEnemies -= rPriorAttackReduction;
		}
		if (bLogWarUtilityDetail && (rEnemyWeight > 0 || rAllyReduction > 0 || rPriorAttackReduction > 0)) logBBAI("UWAI_WAR_UTILITY_FAIR_PLAY_THIRD_PARTY turn=%d agentPlayer=%d rivalPlayer=%d otherPlayer=%d atWar=%d rivalWarMemory=%d enemyWeightPercent=%d allyReductionPercent=%d priorAttackReductionPercent=%d runningOtherEnemiesPercent=%d",
				GC.getGame().getGameTurn(), eWe, eThey, kOther.getID(), bWar,
				iTheirWarMemoryAttitude, rEnemyWeight.getPercent(), rAllyReduction.getPercent(),
				rPriorAttackReduction.getPercent(), rOtherEnemies.getPercent());
	}
	// <!-- custom: Keep the inherited dogpile cost and optional sneak-attack multiplier available after their branches for the consolidated result. The defaults mean zero computed cost, no multiplier adjustment and zero applied penalty; preserve the original > 0.1 calculation and >= 0.5 application thresholds. (GPT-6.1-Sol) -->
	scaled rFromOtherEnemies;
	scaled rPotentialEnemyMult = 1;
	int iOtherEnemiesUtility = 0;
	if (rOtherEnemies > fixp(0.1))
	{
		rFromOtherEnemies = 30 * ((rOtherEnemies + scaled(std::max(0,
				/*	The number of cities we expect them to lose to others.
					The subtracted conquests of ours could include cities of
					other civs, but, in that case, we're apparently busy with
					another enemy and our DoW won't really hurt. */
				((int)militAnalyst().lostCities(eThey).size()) -
				((int)militAnalyst().conqueredCities(eWe).size()))).sqrt()) /
				(scaled(std::max(1, iPotentialOtherEnemies)).sqrt() - fixp(1/3.)));
		rFromOtherEnemies *= (1 - fixp(2/3.) * m_kGame.gameTurnProgress());
		/*	Once we've gone through the trouble of preparing war, we'd like to go
			through with it, but if there are many civs in the game, there's a good
			chance that several of them start plotting at the same time, so
			enforcing fairness only before and during preparations isn't enough. */
		if (kOurTeam.AI_isSneakAttackReady(eTheirTeam))
		{
			rPotentialEnemyMult = scaled(iPotentialOtherEnemies, 10);
			rPotentialEnemyMult.clamp(fixp(0.25), fixp(0.75));
			rFromOtherEnemies *= rPotentialEnemyMult;
		}
		if (rFromOtherEnemies >= fixp(0.5))
		{
			iOtherEnemiesUtility = rFromOtherEnemies.uround();
			m_iU -= iOtherEnemiesUtility;
		}
	}
	if (bLogWarUtilityDetail && (rOtherEnemies.abs() >= fixp(0.01) || iOtherEnemiesUtility != 0))
	{
		logBBAI("UWAI_WAR_UTILITY_FAIR_PLAY_DOGPILE_RESULT turn=%d agentPlayer=%d rivalPlayer=%d potentialOtherEnemies=%d otherEnemiesPercent=%d lostRivalCities=%d agentConquests=%d sneakAttackReady=%d potentialEnemyMultiplierPercent=%d cost=%d utility=%d",
				GC.getGame().getGameTurn(), eWe, eThey, iPotentialOtherEnemies,
				rOtherEnemies.getPercent(), (int)militAnalyst().lostCities(eThey).size(),
				(int)militAnalyst().conqueredCities(eWe).size(), kOurTeam.AI_isSneakAttackReady(eTheirTeam),
				rPotentialEnemyMult.getPercent(), rFromOtherEnemies.round(), -iOtherEnemiesUtility);
	}
	// The rest of this function deals with the early game
	/*	Assume that early AI-on-AI wars are always fair b/c they have the same
		handicap. Not actually true in e.g. the EarthAD1000 scenario. Still,
		early attacks on AI civs aren't a serious problem. */
	if (!kThey.isHuman() ||
		kWe.AI_getMemoryAttitude(eThey, MEMORY_DECLARED_WAR_ON_FRIEND) < 0)
	{
		return;
	}
	/*	Mostly care about Archery, which has power 6. The Wheel isn't unfair
		(power 4). BW and IW have power 8 and 10. */
	/*CvHandicapInfo const& kGameHandicap = GC.getInfo(m_kGame.getHandicapType());
	bool bPowerTechFound = false;
	FOR_EACH_ENUM(Tech)
	{
		if (kGameHandicap.isAIFreeTechs(i) &&
			GC.getInfo(eLoopTech).getPowerValue() >= 5)
		{
			bPowerTechFound = true;
			break;
		}
	}
	if (!bPowerTechFound)
		return;*/
	// Actually, never mind checking for starting tech. Don't want early rushes on low difficulty either.
	EraTypes const eStartEra = m_kGame.getStartEra();
	scaled const rTrainMod = per100(m_kSpeed.getTrainPercent()) * per100(GC.getInfo(eStartEra).getTrainPercent());
	FAssert(rTrainMod > 0);

	scaled rFairnessCost;
	// All bets off by turn 100, but, already by turn 50, the cost may no longer be prohibitive.
	// <!-- custom: AdvCiv's nonzero-start fix added the full start turn even though ordinary later-era starts already subtract StartPercent below, making protection outlast the remaining game. Reconstruct that normal baseline, preserve only genuinely extra start turns and normalize them to the same training-time scale as elapsed turns. See KI#443. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	int iScheduledTurns = 0;
	for (int i = 0; i < m_kSpeed.getNumTurnIncrements(); i++)
		iScheduledTurns += m_kSpeed.getGameTurnInfo(i).iNumGameTurnsPerIncrement;
	int const iStartPercent = GC.getInfo(eStartEra).getStartPercent();
	int const iNormalEraStartTurn = (iScheduledTurns * iStartPercent) / 100;
	int const iExtraStartTurns = std::max(0, m_kGame.getStartTurn() - iNormalEraStartTurn);
	int iTargetTurn = 100 + (iExtraStartTurns / rTrainMod).uround();
	// Allow earlier aggression on crowded maps
	iTargetTurn = (iTargetTurn * ((1 + fixp(1.5) * scaled(m_kGame.getRecommendedPlayers(), m_kGame.getCivPlayersEverAlive())) / fixp(2.5))).uround();
	int const iElapsed = (m_kGame.getElapsedGameTurns() / rTrainMod).uround();
	int iTurnsRemaining = iTargetTurn - iElapsed - iStartPercent;
	bool const bGameEraAdvanced = (m_eGameEra > eStartEra);
	bool const bRivalEraAdvanced = (kThey.getCurrentEra() > eStartEra);
	// <!-- custom: Preserve both the raw score comparison and the inherited shifted/clamped multiplier so the early-fairness row reports the actual adjustment stage rather than recomputing it. (ChatGPT-5.6-Sol) -->
	scaled rScoreRatioRaw;
	scaled rScoreRatioApplied = 1;
	if (iTurnsRemaining > 0)
	{
		rFairnessCost += scaled(iTurnsRemaining, 2).pow(fixp(1.28));
		if (bGameEraAdvanced)
			rFairnessCost *= fixp(3/4.);
		if (bRivalEraAdvanced)
			rFairnessCost *= fixp(3/4.);
		rScoreRatioRaw = scaled(m_kGame.getPlayerScore(eWe), m_kGame.getPlayerScore(eThey));
		rScoreRatioApplied = rScoreRatioRaw - fixp(0.1);
		rScoreRatioApplied.clamp(fixp(0.4), fixp(5/3.));
		rFairnessCost *= rScoreRatioApplied;
	}
	if (m_rGameEraAIFactor > fixp(1.5)) // Dogpiling remains an issue in the Classical era
	{
		if (bLogWarUtilityDetail && iTurnsRemaining > 0) logBBAI("UWAI_WAR_UTILITY_FAIR_PLAY_EARLY_ERA_SKIP turn=%d agentPlayer=%d rivalPlayer=%d turnsRemaining=%d gameEraAdvanced=%d rivalEraAdvanced=%d scoreRatioPercent=%d appliedScoreRatioPercent=%d preliminaryCost=%d gameEraAIFactorPercent=%d",
				GC.getGame().getGameTurn(), eWe, eThey, iTurnsRemaining, bGameEraAdvanced,
				bRivalEraAdvanced, rScoreRatioRaw.getPercent(), rScoreRatioApplied.getPercent(),
				rFairnessCost.round(), m_rGameEraAIFactor.getPercent());
		return;
	}
	// Don't dogpile when human has lost cities in the early game
	int const iCitiesTheyFounded = kThey.getPlayerRecord()->getNumCitiesBuilt();
	int const iCitiesTheyHave = kThey.getNumCities();
	scaled rFromCityLoss;
	if (iCitiesTheyFounded > 0 && iCitiesTheyHave < iCitiesTheyFounded)
		rFromCityLoss = 100 * (1 - scaled(iCitiesTheyHave, iCitiesTheyFounded)).pow(fixp(0.85));
	// <!-- custom: Moved the inherited recent-declaration cost outside its branch so the final fairness row reports it alongside city-loss cost; zero means this alternative contribution was not added. (GPT-6.1-Sol) -->
	int iFromRecentDoW = 0;
	if (rFromCityLoss >= fixp(0.5))
		rFairnessCost += rFromCityLoss;
	// If no cities gained nor lost, at least don't DoW in quick succession.
	else if (kThey.getNumCities() == iCitiesTheyFounded)
	{
		iFromRecentDoW = 35 * kTheirTeam.AI_getNumWarPlans(WARPLAN_ATTACKED_RECENT);
		rFairnessCost += iFromRecentDoW;
	}
	int iAttitudeDiv = 3 - towardThem() +
			kWe.AI_getMemoryAttitude(eThey, MEMORY_REJECTED_DEMAND); // negative
	if (iAttitudeDiv > 1)
		rFairnessCost /= iAttitudeDiv;
	int const iUtility = std::max(0, rFairnessCost.round());
	if (bLogWarUtilityDetail && (iTurnsRemaining > 0 || rFromCityLoss >= fixp(0.5) || iFromRecentDoW != 0 || iAttitudeDiv > 1 || iUtility != 0)) logBBAI("UWAI_WAR_UTILITY_FAIR_PLAY_EARLY_RESULT turn=%d agentPlayer=%d rivalPlayer=%d startEra=%d targetTurn=%d elapsedTurns=%d turnsRemaining=%d gameEraAdvanced=%d rivalEraAdvanced=%d scoreRatioPercent=%d appliedScoreRatioPercent=%d foundedCities=%d currentCities=%d cityLossCost=%d recentDoWCost=%d attitudeDivisor=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, eStartEra, iTargetTurn, iElapsed,
			iTurnsRemaining, bGameEraAdvanced, bRivalEraAdvanced, rScoreRatioRaw.getPercent(),
			rScoreRatioApplied.getPercent(), iCitiesTheyFounded, iCitiesTheyHave,
			rFromCityLoss.round(), iFromRecentDoW, iAttitudeDiv, -iUtility);
	m_iU -= iUtility;
}


// (no longer used)
/*int FairPlay::initialMilitaryUnits(PlayerTypes ePlayer)
{
	CvPlayer const& kPlayer = GET_PLAYER(ePlayer);
	CvHandicapInfo const& kPlayerHandicap = GC.getInfo(kPlayer.getHandicapType());
	// DefenseUnits aren't all that defensive
	return kPlayerHandicap.getStartingDefenseUnits() +
			(kPlayer.isHuman() ? 0 : kPlayerHandicap.getAIStartingDefenseUnits());
}*/

void Bellicosity::evaluate()
{
	// <!-- custom: Bellicosity compares losses for one hostile team war. Evaluate it through that team's leader once, but preserve reachability when any living member is reachable instead of depending on the representative player. See KI#446. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (kWe.isHuman() || !militAnalyst().isWar(eOurTeam, eTheirTeam) || eThey != kTheirTeam.getLeaderID())
		return;
	bool bCanReach = false;
	for (MemberIter itMember(eTheirTeam); itMember.hasNext(); ++itMember)
	{
		if (kWeAI.canReach(itMember->getID()))
		{
			bCanReach = true;
			break;
		}
	}
	if (!bCanReach)
		return;
	// One war is enough
	if (kOurTeam.getNumWars() > 0 && !kOurTeam.isAtWar(eTheirTeam))
		return;
	/*	This is what makes units attack against the odds. 6 for Napoleon and Ragnar,
		4 for Alexander, Boudica, Brennus, Genghis, Gilgamesh, Hannibal, Montezuma,
		Peter, Shaka and Sitting Bull. Just the suicidal types I need except
		for Sitting Bull. Subtract peace-weight to exclude him - all the others
		have a peace-weight near 0. */
	int const iBellicosity = kOurPersonality.getBaseAttackOddsChange() - kOurPersonality.getBasePeaceWeight();
	if (iBellicosity <= 0)
		return;
	scaled rOurMinusTheirLostPow;
	scaled rCurrentAggrPow;
	for (int i = 0; i < NUM_BRANCHES; i++)
	{
		MilitaryBranchTypes const eBranch = (MilitaryBranchTypes)i;
		// Included in other branches; and we never itch for nuclear war.
		if (eBranch == CAVALRY || eBranch == LOGISTICS || eBranch == NUCLEAR)
			continue;
		scaled const rOurLostPow = militAnalyst().lostPower(eWe, eBranch);
		// <!-- custom: The same agent loss budget was reset for every rival member. Sum the hostile team's living-member losses first, then apply the anti-third-party cap once so ownership partition cannot change the result. See KI#446. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		scaled rTheirLostPow;
		for (MemberIter itMember(eTheirTeam); itMember.hasNext(); ++itMember)
			rTheirLostPow += militAnalyst().lostPower(itMember->getID(), eBranch);
		/*	If they lose much more than we do, chances are that a third party is
			causing this. And don't want a warlike AI to attack a weak civ just
			for sport. */
		rOurMinusTheirLostPow += std::min(fixp(2.35) * rOurLostPow, rTheirLostPow) - fixp(0.75) * rOurLostPow;
		if (eBranch != HOME_GUARD) // Not tracked by cache, and not really relevant here.
			rCurrentAggrPow += ourCache().getPowerValues()[eBranch]->power();
	}
	if (rOurMinusTheirLostPow <= 0 || rCurrentAggrPow < 1)
		return;
	// A good war is one that we win, and that occupies many of our eager warriors.
	scaled rGloryRate = rOurMinusTheirLostPow / rCurrentAggrPow;
	rGloryRate.decreaseTo(1);
	// <!-- custom: Store the rounded contribution once so the structured result reports the exact bellicosity utility applied below. (ChatGPT-5.6-Sol) -->
	int const iUtility = (2 * iBellicosity * rGloryRate).round();
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_BELLICOSITY_RESULT turn=%d agentPlayer=%d rivalPlayer=%d rivalTeam=%d lostPowerAdvantage=%d aggressivePower=%d bellicosity=%d gloryPercent=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, eTheirTeam, rOurMinusTheirLostPow.uround(),
			rCurrentAggrPow.uround(), iBellicosity, rGloryRate.getPercent(), iUtility);
	m_iU += iUtility;
}


void TacticalSituation::evaluate()
{
	if (!militAnalyst().isWar(eWe, eThey))
		return;
	if (kOurTeam.isAtWar(eTheirTeam))
		evalEngagement();
	else evalOperational();
}


void TacticalSituation::evalEngagement()
{
	PROFILE_FUNC();
	/*	Can't move this to UWAICache (to compute it only once at the start of a turn)
		b/c it needs to be up-to-date during the turns of other civs, in particular
		the humans that could propose peace at any point of their turns. */
	int iOurExposed = 0;
	int iTheirExposed = 0;
	int iEntangled = 0;
	int iOurTotal = 0;
	int iOurMissions = 0;
	int const iDamagedHPThresh = 60;
	// How many of our units threaten each plot
	std::map<PlotNumTypes,int> weThreaten;
	CvMap const& kMap = GC.getMap();
	FOR_EACH_GROUPAI(pGroup, kWe)
	{
		CvUnit const* pHeadUnit = pGroup->getHeadUnit();
		if (pHeadUnit == NULL || !pHeadUnit->canDefend())
			continue;
		int iGroupSize = pGroup->getNumUnits();
		iOurTotal += iGroupSize;
		// Assume units in cities to be less engaged
		if (pGroup->getPlot().isCity())
			iGroupSize = intdiv::uround(iGroupSize, 2);
		int iRange = 1;
		CvPlot const& kGroupPlot = pGroup->getPlot();
		PlayerTypes const ePlotOwner = kGroupPlot.getOwner();
		/*	Limit range, not least for performance reasons, to cover combat imminent
			on our current turn or the opponent's next turn. If our group is on a
			friendly route, we can probably attack units two tiles away,
			though we probably won't if our units are damaged. */
		if (pHeadUnit->maxHitPoints() - pHeadUnit->getDamage() >= 80)
		{
			if (kGroupPlot.getArea().isWater())
				iRange++;
			else if (ePlotOwner != NO_PLAYER && kGroupPlot.isRoute() &&
				(ePlotOwner == eWe || kOurTeam.canPeacefullyEnter(TEAMID(ePlotOwner))))
			{
				iRange++;
			}
		}
		/*	(Using CvPlayerAI::AI_getPlotDanger for entanglement
			could result in double counting) */
		for (SquareIter itPlot(kGroupPlot, iRange, false); itPlot.hasNext(); ++itPlot)
		{
			CvPlot const& kPlot = *itPlot;
			if (!kPlot.isVisible(eOurTeam) || !kPlot.sameArea(kGroupPlot) ||
				!kPlot.isUnit() || // shortcut
				// Could do without this if it's too slow
				!pHeadUnit->canMoveInto(kPlot, true, false, false, true,
				/*	We're about to move our units, but their movement points haven't
					been restored yet, so it's important not to check for moves. */
				/*bDangerCheck=*/true))
			{
				continue;
			}
			PlotNumTypes ePlot = kPlot.plotNum();
			std::map<PlotNumTypes,int>::iterator pos = weThreaten.find(ePlot);
			if (pos == weThreaten.end())
				weThreaten.insert(std::make_pair(ePlot, iGroupSize));
			else pos->second += iGroupSize;
		}
		if (!kGroupPlot.isCity()) // ourEvac covers exposed cities
		{
			// For identifying exposed units of ours, AI_getPlotDanger is adequate.
			int iOurDamaged = 0;
			int iDanger = kWe.AI_getPlotDanger(
					kGroupPlot, 1, false, iGroupSize, false, eThey);
			if (iDanger > 0)
			{
				FOR_EACH_UNIT_IN(pGroupUnit, *pGroup)
				{ 
					if (pGroupUnit->maxHitPoints() - pGroupUnit->getDamage() <=
						iDamagedHPThresh)
					{
						iOurDamaged++;
						if (iOurDamaged >= iDanger)
							break;
					}
				}
			}
			// Damaged units protected by healthy units aren't exposed
			iOurDamaged = ::range(iDanger + iOurDamaged - iGroupSize, 0, iOurDamaged);
			iOurExposed += iOurDamaged;
		}
		if (!kWe.isHuman())
		{
			// Akin to CvPlayerAI::AI_enemyTargetMissions
			MissionAITypes const eMission = pGroup->AI_getMissionAIType();
			CvPlot const* pMissionPlot = pGroup->AI_getMissionAIPlot();
			bool const bPillage = (eMission == MISSIONAI_PILLAGE && ePlotOwner == eThey);
			if (bPillage ||
				// (K-Mod uses MISSIONAI_ASSAULT also for land-based assaults on cities)
				((eMission == MISSIONAI_ASSAULT || eMission == MISSIONAI_GROUP ||
				eMission == MISSIONAI_REINFORCE) &&
				pMissionPlot != NULL && pMissionPlot->getOwner() == eThey &&
				// Don't count besiegers
				(!pMissionPlot->isCity() || stepDistance(&kGroupPlot, pMissionPlot) > 1)))
			{
				int iMissionScore = pGroup->getNumUnits() + pGroup->getCargo();
				if (bPillage)
					iMissionScore = intdiv::uround(iMissionScore, 2);
				iOurMissions += iMissionScore;
			}
		}
	}
	// <!-- custom: Name the inherited pushover condition once so the same gameplay branch and consolidated diagnostic row share the exact state. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	bool const bPushoverMissionBoost = (iOurMissions > 0 && kOurTeam.AI_isPushover(eTheirTeam));
	if (bPushoverMissionBoost)
	{
		/*	If the target is weak, even a small fraction of our military en route
			could have a big impact once it arrives. */
		iOurMissions *= 3;
		iOurMissions /= 2;
	}
	/*	So long as we check canMoveInto with bAttack=true above, this here
		probably won't save time. */
	/*FOR_EACH_GROUPAI(pGroup, kWe)
	{
		CvUnit const* pHeadUnit = pGroup->getHeadUnit();
		if (pHeadUnit == NULL || !pHeadUnit->canDefend())
			continue;
		weThreaten.erase(kMap.plotNum(pGroup->getPlot()));
	}*/
	scaled rEraMult = 1 - fixp(0.2) *
			(kThey.AI_getCurrEraFactor() - kWe.AI_getCurrEraFactor());
	rEraMult.clamp(fixp(0.4), fixp(1.6));
	{	PROFILE("TacticalSituation::evalEngagement - weThreaten");
	for (std::map<PlotNumTypes,int>::const_iterator it = weThreaten.begin();
		it != weThreaten.end(); ++it)
	{
		CvPlot const& kPlot = kMap.getPlotByIndex(it->first);
		int iOurUnits = it->second;
		int iTheirUnits = 0;
		int iTheirDamaged = 0;
		// Some overlap with CvPlayerAI::AI_countDangerousUnits
		FOR_EACH_UNIT_IN(pPlotUnit, kPlot)
		{
			if (pPlotUnit->getCombatOwner(eOurTeam) != eThey)
				continue;
			iTheirUnits++;
			if (pPlotUnit->currHitPoints() <= iDamagedHPThresh)
				iTheirDamaged++;
			/*if (iTheirUnits - iTheirDamaged >= iOurUnits)
				break;*/ // Need the full count of iTheirUnits after all
		}
		// Damaged units protected by healthy units aren't exposed
		iTheirDamaged = ::range(iOurUnits + iTheirDamaged - iTheirUnits,
				0, iTheirDamaged);
		/*	If their healthy units outnumber or outclass ours,
			then we probably won't want to engage. */
		if (iTheirUnits > iTheirDamaged)
		{
			iOurUnits = (iOurUnits * scaled::min(1,
					(fixp(1.3) * iOurUnits * rEraMult) /
					(iTheirUnits - iTheirDamaged))).round();
		}
		/*	Count at most one enemy unit per unit of ours as "entangled",
			i.e. count pairs of units. */
		iTheirUnits = std::min(iTheirUnits, iOurUnits);
		if (kPlot.isCity())
		{
			iTheirUnits = intdiv::uround(iTheirUnits, 2);
			iTheirDamaged = 0; // Covered by theirEvac
		}
		iEntangled += iTheirUnits;
		iTheirExposed += iTheirDamaged;
	}
	} // (end of profile scope)
	int const iOurEvac = evacPop(eWe, eThey);
	int const iTheirEvac = evacPop(eThey, eWe);
	/*	If a human is involved or if it's our turn, then we shouldn't worry too
		much about our units being exposed - humans may well have already made
		all attacks against vulnerable units before contacting us. If it's our turn,
		it's still the beginning of the turn, so we can probably save our units. */
	scaled rInitiativeMult = fixp(0.25); // Low if they have the initiative
	if (kWe.isHuman() || kThey.isHuman())
		rInitiativeMult *= 2;
	scaled rUtility = (4 * (rInitiativeMult * iTheirExposed -
			(1 - rInitiativeMult) * iOurExposed) + iOurMissions) /
			std::max(iOurTotal, 1);
	// getNumUnits is an information cheat, but it's all quite fuzzy anyway.
	rUtility += (2 * iEntangled) / std::max(1, iOurTotal + kThey.getNumUnits());
	if (kWe.getTotalPopulation() > 0)
		rUtility += 3 * (iTheirEvac - fixp(1.35) * iOurEvac) / kWe.getTotalPopulation();
	rUtility *= 100;
	int iRecentlyLostPop = 0;
	/*	A mod could add requirements for building roads, but it seems fair to assume
		that roads will be available by the time that roads become faster. */
	bool const bFastRoads = (GET_TEAM(eOurTeam).getRouteChange((RouteTypes)0) <= -10);
	FOR_EACH_CITY(pCity, kThey)
	{
		int iRange = 2;
		if (bFastRoads)
			iRange++;
		if (pCity->isOccupation() && pCity->isEverOwned(eWe) &&
			/*	Just want to know if we have some presence beyond a single
				stray unit near the city */
			kThey.AI_getPlotDanger(pCity->getPlot(), iRange, false, 2, false) > 1)
		{
			iRecentlyLostPop += pCity->getPopulation();
		}
	}
	scaled rRecentlyLostPopPortion;
	if (kWe.getTotalPopulation() > 0)
	{
		rRecentlyLostPopPortion = scaled(
				2 * iRecentlyLostPop,
				kWe.getTotalPopulation() + kThey.getTotalPopulation());
	}
	rUtility += rRecentlyLostPopPortion * 100;
	int iUtility = 0;
	if (rUtility.abs() >= fixp(0.5))
	{
		iUtility = rUtility.round();
		m_iU += iUtility;
	}
	// <!-- custom: The inherited mission-boost sentence and engagement summary describe one tactical state. Emit one row when either the boost occurred or a utility contribution was actually applied. (ChatGPT-5.6-Sol) -->
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted() && (bPushoverMissionBoost || iUtility != 0)) logBBAI("UWAI_WAR_UTILITY_TACTICAL_ENGAGEMENT_RESULT turn=%d agentPlayer=%d rivalPlayer=%d pushoverMissionBoost=%d theirExposed=%d ourExposed=%d entangled=%d theirEvacPopulation=%d ourEvacPopulation=%d missionScore=%d totalMilitaryUnits=%d agentPopulation=%d recentlyLostPopulation=%d recentlyLostPercent=%d initiativePercent=%d eraMultiplierPercent=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, bPushoverMissionBoost, iTheirExposed,
			iOurExposed, iEntangled, iTheirEvac, iOurEvac, iOurMissions, iOurTotal,
			kWe.getTotalPopulation(), iRecentlyLostPop, rRecentlyLostPopPortion.getPercent(),
			rInitiativeMult.getPercent(), rEraMult.getPercent(), iUtility);
}


int TacticalSituation::evacPop(PlayerTypes eOwner, PlayerTypes eInvader) const
{
	int iPop = 0;
	CvPlayerAI const& kOwner = GET_PLAYER(eOwner);
	FOR_EACH_CITYAI(pCity, kOwner)
	{
		/*	Check PlotDanger b/c we don't want to count cities that are threatened
			by a third party */
		if (pCity->AI_isEvacuating() && kOwner.AI_getPlotDanger(
			pCity->getPlot(), 1, false, 2, false, eInvader) >= 2)
		{
			iPop += pCity->getPopulation();
		}
	}
	return iPop;
}


// <!-- custom: Reset the one-shot target-team readiness calculation for each agent member's distinct unit inventory. See KI#461. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
int TacticalSituation::preEvaluate()
{
	m_bOperationalEvaluated = false;
	return 0;
}


// <!-- custom: Canonicalize the configured target team's player callbacks into one readiness evaluation for the current agent member. See KI#461. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void TacticalSituation::evalOperational()
{
	/*	Evaluate our readiness only for the target that we want to invade
		(not its vassals or allies from def. pact) */
	if (m_kParams.getTarget() != eTheirTeam || m_bOperationalEvaluated)
		return;
	m_bOperationalEvaluated = true;
	/*	When taking the human pov, we mustn't assume to know their exact unit counts.
		Whether they're ready for an attack is secret info. */
	if (kWe.isHuman())
		return;
	// <!-- custom: Aggregate the configured target team's player-keyed reachability and era inputs before charging this agent member's one operational-readiness deficit.
	// This prevents the generic rival-player loop from repeating the same force and preparation state for every target teammate. See KI#461. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	bool bTargetReachable = false;
	scaled rTheyEra;
	EraTypes eTheyEra = NO_ERA;
	for (MemberIter itMember(eTheirTeam); itMember.hasNext(); ++itMember)
	{
		if (ourCache().numReachableCities(itMember->getID()) <= 0)
			continue;
		bTargetReachable = true;
		rTheyEra.increaseTo(GET_PLAYER(itMember->getID()).AI_getCurrEraFactor());
		eTheyEra = std::max(eTheyEra, itMember->getCurrentEra());
	}
	if (!bTargetReachable || (m_kParams.isNaval() &&
		// Won't need invaders then
		ourCache().getPowerValues()[LOGISTICS]->getTypicalUnit() == NO_UNIT))
	{
		return;
	}
	// Similar to CvUnitAI::AI_stackOfDoomExtra
	scaled rTargetAttackers = 4;
	if (rTheyEra > 0)
		rTargetAttackers += rTheyEra.pow(fixp(1.3));
	if (m_kParams.isNaval())
		rTargetAttackers *= fixp(1.3);
	// Smaller target on lower difficulty
	{
		scaled rTrainMod = per100(GC.getInfo(m_kGame.getHandicapType()).
				getAITrainPercent());
		rTrainMod.clamp(1, fixp(1.5));
		rTargetAttackers /= rTrainMod;
	}
	/*	Don't necessarily need a larger initial stack for total war.
		A little maybe ... */
	if (m_kParams.isTotal())
		rTargetAttackers++;
	// Need larger stacks when not all our army units can destroy defenders
	bool const bCanBombard = ourCache().getPowerValues()[ARMY]->canBombard();
	if (bCanBombard)
		rTargetAttackers *= fixp(1.25);
	/*	To account for the AI's inability to put all available attackers in one spot,
		and misc. distractions like barbarians */
	rTargetAttackers += scaled::max(0, fixp(0.75) * kWe.getNumCities() - 2);
	scaled rTargetCargo;
	scaled rCargo;
	scaled rTargetEscort;
	// Minor issue: counts Galleons (even when we could be up against Frigates)
	int iEscort = kWe.AI_totalUnitAIs(UNITAI_ATTACK_SEA) +
			kWe.AI_totalUnitAIs(UNITAI_ESCORT_SEA) +
			kWe.AI_totalUnitAIs(UNITAI_RESERVE_SEA);
	if (m_kParams.isNaval())
	{
		rTargetCargo = rTargetAttackers;
		rCargo = ourCache().getPowerValues()[LOGISTICS]->power();
		/*	If we have 0 escorters, we probably can't build them; probably have
			Galleys then but no Triremes. That's OK.
			Also no need for escort if they're way behind in tech. */
		if (iEscort > 0 && eTheyEra >= kWe.getCurrentEra())
		{
			/*	More escort units would be nice, but I'm checking only
				bare necessities. */
			rTargetEscort = rTargetCargo / 6;
		}
	}
	/*	NB: Sea and air units have their own UNITAI types;
		this counts only land units. */
	int const iAttackers = std::max(0,
			kWe.AI_totalUnitAIs(UNITAI_ATTACK) +
			kWe.AI_totalUnitAIs(UNITAI_ATTACK_CITY) +
			kWe.AI_totalUnitAIs(UNITAI_RESERVE) +
			kWe.AI_totalUnitAIs(UNITAI_PILLAGE) +
			kWe.AI_totalUnitAIs(UNITAI_PARADROP) +
			kWe.AI_totalUnitAIs(UNITAI_COLLATERAL)
			/*	Settlers will occupy potential attackers as escorts.
				NB: The AI scraps Settlers eventually if it can't use them;
				can't block war plans permanently. */
			- 2 * kWe.AI_totalUnitAIs(UNITAI_SETTLE));
	/*	(Can't check if we have enough Siege units b/c their UNITAI types overlap
		with non-Siege attackers.) */
	scaled rReadiness = iAttackers / rTargetAttackers;
	if (rTargetCargo > 0)
		rReadiness.decreaseTo(rCargo / rTargetCargo);
	if (rTargetEscort > 0)
	{	// Escort not totally crucial
		rReadiness.decreaseTo((rReadiness + iEscort / rTargetEscort) / 2);
	}
	if (rReadiness >= 1)
		return;
	int const iRemainingTime = m_kParams.getPreparationTime();
	FAssert(iRemainingTime >= 0);
	int iInitialPrepTime = 0;
	if (!m_kParams.isImmediateDoW())
	{	// Overlaps with WarEvaluator::defaultPreparationTime
		if (m_kParams.isTotal())
		{
			if (m_kParams.isNaval())
				iInitialPrepTime = getUWAI().preparationTimeTotalNaval();
			else iInitialPrepTime = getUWAI().preparationTimeTotal();
		}
		else
		{
			if (m_kParams.isNaval())
				iInitialPrepTime = getUWAI().preparationTimeLimitedNaval();
			else iInitialPrepTime = getUWAI().preparationTimeLimited();
		}
	}
	FAssert(iInitialPrepTime >= 0);
	/*	Cost for lack of preparation is based on the assumption
		that it won't hurt much to prolong preparations a bit.
		Not true for immediate DoW. Treat this case as if half the prep time still
		remained (although actually there isn't and never was time to prepare). */
	scaled rPassedPortion = fixp(0.5);
	if (iInitialPrepTime > 0)
		rPassedPortion = 1 - scaled::min(1, scaled(iRemainingTime, iInitialPrepTime));
	if (rPassedPortion <= 0)
		return;
	int const iUnreadinessCost = (100 * rPassedPortion.pow(fixp(1.5)) *
			(1 - rReadiness.pow(fixp(3.7)))).uround();
	// <!-- custom: The inherited bombardment/readiness/negligible-cost messages all describe this one operational-readiness calculation; keep its force inputs and exact applied cost together. (ChatGPT-5.6-Sol) -->
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_TACTICAL_OPERATIONAL_RESULT turn=%d agentPlayer=%d rivalPlayer=%d targetTeam=%d naval=%d totalWar=%d canBombard=%d readinessPercent=%d attackers=%d targetAttackers=%d cargo=%d targetCargo=%d escort=%d targetEscort=%d remainingPrepTurns=%d initialPrepTurns=%d passedPrepPercent=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, eTheirTeam, m_kParams.isNaval(), m_kParams.isTotal(),
			bCanBombard, rReadiness.getPercent(), iAttackers, rTargetAttackers.round(),
			rCargo.uround(), rTargetCargo.uround(), iEscort, rTargetEscort.uround(),
			iRemainingTime, iInitialPrepTime, rPassedPortion.getPercent(), -iUnreadinessCost);
	m_iU -= iUnreadinessCost;
}


// <!-- custom: LoveOfPeace had no inherited UWAI diagnostic prose to convert. Keep its valuation unchanged without adding diagnostic rows merely for symmetry with the other aspects. (GPT-6.1-Sol) -->
void LoveOfPeace::evaluate()
{
	int iLoPCost = kOurPersonality.getLoveOfPeace();
	if (iLoPCost <= 0 || !militAnalyst().isWar(eWe, eThey) || kWe.isHuman())
		return;
	if (kOurTeam.isAtWar(eTheirTeam))
	{
		if (militAnalyst().lostPower(eWe, ARMY) < 5 ||
			// Lost power alone isn't reliable; could be to a third party.
			(ourCache().numReachableCities(eThey) <= 0 &&
			kThey.uwai().getCache().numReachableCities(eWe) <= 0))
		{
			return;
		}
		iLoPCost /= 2;
	}
	m_iU -= iLoPCost;
}


int ThirdPartyIntervention::preEvaluate()
{
	scaled rOurLostPow = militAnalyst().lostPower(eWe, ARMY) +
			militAnalyst().lostPower(eWe, HOME_GUARD) +
			/*	Squared b/c logistics power is only the cargo capacity.
				(Fixme: should track power of cargo ships separately.) */
			SQR(militAnalyst().lostPower(eWe, LOGISTICS));
	scaled const rGainedPow = militAnalyst().gainedPower(eWe, ARMY) +
			militAnalyst().gainedPower(eWe, HOME_GUARD);
	m_rDefPow = ourCache().getPowerValues()[ARMY]->power(); // includes HOME_GUARD
	for (PlayerAIIter<MAJOR_CIV,OTHER_KNOWN_TO> itAlly(eOurTeam);
		itAlly.hasNext(); ++itAlly)
	{
		if(militAnalyst().isEliminated(itAlly->getID()))
			continue;
		if (itAlly->getMasterTeam() == kOurTeam.getMasterTeam() ||
			(kOurTeam.isDefensivePact(itAlly->getTeam()) &&
			// This distinction should make us reluctant to kill our DPs
			militAnalyst().getWarsDeclaredBy(eWe).empty()))
		{
			m_rDefPow += itAlly->uwai().getCache().
					getPowerValues()[ARMY]->power() / 2;
			// Let's not bother with their guard units, but count army fully.
			rOurLostPow += militAnalyst().lostPower(itAlly->getID(), ARMY);
		}
	}
	m_rLostDefPowRatio = rOurLostPow / std::max(m_rDefPow,
			m_rDefPow + rGainedPow);
	m_rLostDefPowRatio.clamp(0, fixp(0.5));
	/*	The timing of the intervention is difficult to predict. They may or may not
		need to build up units, and we may be weakest early in the simulation or
		late, but, even in the latter case, they might attack early b/c we already
		appear weak enough ... Too complicated to estimate their build-up, but it'll
		probably be less than ours because we'll start right away and won't stop
		until the war ends. So let's count our estimated change in power half
		and theirs not at all - and later adjust our power to account for
		m_rLostDefPowRatio (losses, distraction). */
	m_rDefPow.increaseTo(m_rDefPow + rGainedPow / 2);
	return 0;
}


void ThirdPartyIntervention::evaluate()
{
	/*	For setting a debugger breakpoint, may want to use this condition:
		m_kAgentTeam.m_eID == x && m_kParams.m_eTarget == y && m_pRivalPlayer->m_eID == z
	*/
	// Mustn't modify those members here
	scaled rOurPow = m_rDefPow;
	scaled rOurLostPowRatio = m_rLostDefPowRatio;
	TeamTypes const eTarget = m_kParams.getTarget();
	// <!-- custom: The decisive-war shortcut concerns the evaluated target team. Require every living target member to be eliminated in the simulation or that exact team to capitulate.
	// One real team leader's elimination or an unrelated capitulation did not establish this. See KI#458. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	bool bTargetDefeated = false;
	if (m_kParams.isConsideringPeace() && eTarget != NO_TEAM)
	{
		bTargetDefeated = (militAnalyst().getCapitulationsAccepted(eOurTeam).count(eTarget) > 0);
		if (!bTargetDefeated)
		{
			bTargetDefeated = true;
			for (MemberIter itTargetMember(eTarget); itTargetMember.hasNext(); ++itTargetMember)
			{
				if (!militAnalyst().isEliminated(itTargetMember->getID()))
				{
					bTargetDefeated = false;
					break;
				}
			}
		}
	}
	/*	Count this cost only when we're actually incurring losses from fighting
		against a 2nd party. Otherwise, we'd veer into the lane of the
		PreEmptiveWar aspect. If we're not incurring losses, then we're doing
		all that we can to avoid exposing us to the 3rd party (eThey). Active
		preparations for an anticipated DoW are handled by the Alert AI strategy. */
	if (rOurLostPowRatio <= 0 ||
		// War against them is already covered by the military analysis
		militAnalyst().isOnTheirSide(eTheirTeam, true) ||
		/*	They're busy fighting someone else. Need to be careful not to use a
			narrow check here - like them conquering cities - b/c that could
			indirectly encourage us to support their war effort. */
		militAnalyst().lostPower(eThey, ARMY) >= 1 ||
		/*	When already at war and winning decisively, try to wrap it up
			before worrying about 3rd parties. */
		// <!-- custom: Use the target-scoped result prepared above instead of the removed real-leader/any-capitulation proxies. See KI#458. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		bTargetDefeated ||
		// Vassals are covered when evaluating their master
		!kTheirTeam.uwai().canSchemeAgainst(eOurTeam, true) ||
		!kTheirTeam.uwai().isLandTarget(eOurTeam))
	{
		return;
	}
	bool const bLogWarUtilityDetail = (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted());
	{	/*	Our losses should be small if our enemies are weak, but perhaps
			better not to rely on that. Fear of 3rd parties should not stop
			us from waging minor wars. (Well, having our troops away from
			home could actually hurt us a lot, but I'm more worried about
			the AI just sitting there in tonic immobility.) */
		bool bAllPushOver = true;
		for (TeamIter<FREE_MAJOR_CIV,KNOWN_POTENTIAL_ENEMY_OF> itEnemy(eOurTeam);
			itEnemy.hasNext(); ++itEnemy)
		{
			if (militAnalyst().isWar(eOurTeam, itEnemy->getID()) &&
				!kOurTeam.AI_isPushover(itEnemy->getID()))
			{
				bAllPushOver = false;
				break;
			}
		}
		if (bAllPushOver)
		{
			if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_THIRD_PARTY_SKIP_WEAK_ENEMIES turn=%d agentPlayer=%d rivalPlayer=%d targetTeam=%d lostDefensivePowerPercent=%d",
					GC.getGame().getGameTurn(), eWe, eThey, eTarget, rOurLostPowRatio.getPercent());
			return;
		}
	}
	// <!-- custom: Keep the alternative probability inputs as zero/sentinel-default state so whichever branch is used can report one truthful structured result without reproducing its narration. (ChatGPT-5.6-Sol) -->
	scaled rUtilityVsUs;
	scaled rNoise;
	int iParanoia = 0;
	int iDogpileWarRand = -1;
	// Sending troops abroad is dangerous; will take long to redeploy.
	// <!-- custom: Replace the mutable false/true diagnostic flag with the inherited deployment condition itself; the same named condition now controls the loss adjustment and describes it in the result row. (GPT-6.1-Sol) -->
	bool const bOverseasDeployment = (!militAnalyst().isPeaceScenario() && eTarget != NO_TEAM &&
		!kOurTeam.uwai().isLandTarget(eTarget) &&
		!GET_TEAM(eTarget).AI_hasSharedPrimaryArea(eTheirTeam));
	if (bOverseasDeployment)
	{
		rOurLostPowRatio *= 2;
		rOurLostPowRatio.decreaseTo(fixp(0.6));
	}
	scaled rInterventionProb;
	/*	(If we applied different measures in the war/peace scenario, we could
		get inconsistent results, making us feel safer in the war scenario.) */
	if (//militAnalyst().isPeaceScenario() ||
		m_kParams.isConsideringPeace())
	{
		/*	In this case, all relevant wars are already reflected by their
			war utility against us. No worries if that utility is 0.
			Not sure if war utility _including_ Distraction would be better here;
			well, that's not cached and it shouldn't matter much. */
		rUtilityVsUs = kThey.uwai().getCache().
				warUtilityIgnoringDistraction(eOurTeam);
		/*	We're not supposed to know everything that enters their war utility
			calculation. Add some noise. Hashing rank should result in a somewhat
			stable error. */
		rNoise = 30 *
				(scaled::hash(m_kGame.getPlayerRank(eWe), eWe) - fixp(0.5));
		rUtilityVsUs += rNoise;
		if (rUtilityVsUs > 0)
		{
			rUtilityVsUs.decreaseTo(94);
			rInterventionProb = fixp(0.8) * rUtilityVsUs / 100;
		}
	}
	else
	{	/*	If there are undeclared wars, it's really difficult to say how much
			higher their war utility will be after those DoW. Not worth letting
			the AI cheat more. Better just rely on paranoia. */
		int iOurDefPow = kOurTeam.getDefensivePower();
		iOurDefPow = (iOurDefPow * (1 - rOurLostPowRatio)).uround();
		iParanoia = kWe.AI_paranoiaRating(eThey, iOurDefPow, false, true);
		iParanoia = std::min(iParanoia, 210);
		rInterventionProb = scaled(iParanoia - 28, 233);
		if (rInterventionProb > 0 && !kTheirTeam.isHuman() &&
			!m_kGame.isOption(GAMEOPTION_RANDOM_PERSONALITIES))
		{
			scaled rWarRand = kTheirTeam.AI_dogpileWarRand();
			if (bLogWarUtilityDetail) iDogpileWarRand = rWarRand.floor();
			rWarRand.mulDiv(4, 3);
			rWarRand.clamp(25, 200);
			// War rands matter less when war utility is high
			rInterventionProb /= (rWarRand / 100 + 5 * rInterventionProb) /
					(5 * rInterventionProb + 1);
		}
	}
	if (rInterventionProb < per100(1))
	{
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_THIRD_PARTY_PROBABILITY_REJECT_INITIAL turn=%d agentPlayer=%d rivalPlayer=%d targetTeam=%d consideringPeace=%d overseasDeployment=%d lostDefensivePowerPercent=%d rivalWarUtility=%d noise=%d paranoia=%d dogpileWarRand=%d interventionProbabilityPercent=%d",
				GC.getGame().getGameTurn(), eWe, eThey, eTarget, m_kParams.isConsideringPeace(),
				bOverseasDeployment, rOurLostPowRatio.getPercent(), rUtilityVsUs.round(), rNoise.round(),
				iParanoia, iDogpileWarRand, rInterventionProb.getPercent());
		return;
	}
	/*	(Would be nice to do this also when there has been no war, but there
		is no counter for the number of turns that we've been considering
		war against the target or have been afraid of an intervention.) */
	// <!-- custom: Retain the normalized peace duration and applied decay for the combined intervention row: -1 means the prior-war-memory branch was skipped; multiplier 1 means no decay. Only the diagnostic duration is rounded; the KI#438 calculation below keeps its scaled precision. (GPT-6.1-Sol) -->
	int iNormalizedPeaceTurns = -1;
	scaled rPeaceDecayMult = 1;
	if (kWe.AI_getMemoryCount(eThey, MEMORY_DECLARED_WAR) +
		kThey.AI_getMemoryCount(eWe, MEMORY_DECLARED_WAR) > 0)
	{	// Avoid stalemates: don't be afraid of interventions forever
		int const iAtPeaceTurns = kOurTeam.AI_getAtPeaceCounter(eTheirTeam);
		int const iThresh = (5 * GC.getDefineINT(CvGlobals::PEACE_TREATY_LENGTH)) / 3 - 1;
		// <!-- custom: AdvCiv computed a game-speed-normalized peace duration but discarded it, then gated and decayed intervention fear in raw turns. Use the prepared duration consistently for both operations. See KI#438. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		scaled const rAtPeaceTurns = fixp(1.5) * iAtPeaceTurns /
				(per100(m_kSpeed.getGoldenAgePercent()) + fixp(0.5));
		if (bLogWarUtilityDetail) iNormalizedPeaceTurns = rAtPeaceTurns.round();
		if (rAtPeaceTurns > iThresh)
		{
			rPeaceDecayMult = std::max(fixp(1/3.), 1 - (rAtPeaceTurns - iThresh) / 30);
			rInterventionProb *= rPeaceDecayMult;
		}
	}
	// <!-- custom: Capture the personality war-randomness average before its inherited 25..200 clamp; -1 marks the skipped human/random-personalities branch. Gameplay still uses the scaled, clamped value below. (GPT-6.1-Sol) -->
	int iWarRand = -1;
	if (!kTheirTeam.isHuman() &&
		!m_kGame.isOption(GAMEOPTION_RANDOM_PERSONALITIES))
	{	/*	(DogpileWarRand is already accounted for above;
			is part of the war utility calc.) */
		int iLimitedWarRand = kTheirTeam.AI_limitedWarRand();
		scaled rWarRand(iLimitedWarRand +
				std::min(iLimitedWarRand, kTheirTeam.AI_maxWarRand()), 2);
		if (bLogWarUtilityDetail) iWarRand = rWarRand.round();
		rWarRand.clamp(25, 200);
		rInterventionProb /= (rWarRand / 100 + 5 * rInterventionProb) /
				(5 * rInterventionProb + 1);
	}
	if (rInterventionProb < per100(1))
	{
		if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_THIRD_PARTY_PROBABILITY_REJECT_ADJUSTED turn=%d agentPlayer=%d rivalPlayer=%d targetTeam=%d consideringPeace=%d overseasDeployment=%d lostDefensivePowerPercent=%d rivalWarUtility=%d noise=%d paranoia=%d dogpileWarRand=%d normalizedPeaceTurns=%d peaceDecayPercent=%d warRand=%d interventionProbabilityPercent=%d",
				GC.getGame().getGameTurn(), eWe, eThey, eTarget, m_kParams.isConsideringPeace(),
				bOverseasDeployment, rOurLostPowRatio.getPercent(), rUtilityVsUs.round(), rNoise.round(),
				iParanoia, iDogpileWarRand, iNormalizedPeaceTurns, rPeaceDecayMult.getPercent(),
				iWarRand, rInterventionProb.getPercent());
		return;
	}
	/*	(Would be nice to anticipate war trades, but the request frequency is the
		same for almost all leaders anyway, and checking attitude thresholds -
		not to mention trade items - gets pretty complicated, especially
		considering that all parties involved could be human.) */
	/*	(Better not to create an incentive for joining the wars of civs
		that we fear. Let the SuckingUp aspect deal with that.) */
	/*if (militAnalyst().isOnOurSide(eTheirTeam))
		rInterventionProb *= fixp(2/3.);*/
	rInterventionProb.decreaseTo(fixp(0.85));
	scaled rTheirPow = kThey.uwai().getCache().
			getPowerValues()[ARMY]->power();
	if (kWe.isHuman())
		rTheirPow *= kThey.uwai().confidenceAgainstHuman();
	for (PlayerAIIter<MAJOR_CIV,VASSAL_OF> itTheirVassal(eTheirTeam);
		itTheirVassal.hasNext(); ++itTheirVassal)
	{
		rTheirPow += itTheirVassal->uwai().getCache().
				getPowerValues()[ARMY]->power() / 2;
	}
	rTheirPow.increaseTo(scaled::epsilon());
	// Preserve for later
	scaled const rTheirCurPowToOurs = rTheirPow / rOurPow;
	{
		// (Ignore their losses in home guard; we know that they haven't lost cities.)
		scaled rTheirLostPowMult = rTheirPow - militAnalyst().lostPower(eThey, ARMY);
		for (PlayerAIIter<MAJOR_CIV,VASSAL_OF> itTheirVassal(eTheirTeam);
			itTheirVassal.hasNext(); ++itTheirVassal)
		{
			rTheirLostPowMult -= militAnalyst().lostPower(
					itTheirVassal->getID(), ARMY) / 2;
		}
		rTheirLostPowMult /= rTheirPow;
		rTheirLostPowMult.increaseTo(fixp(0.6));
		rTheirPow *= rTheirLostPowMult;
	}
	if (kThey.isHuman())
		rOurPow *= kWeAI.confidenceAgainstHuman();
	rOurPow *= 1 - rOurLostPowRatio;
	scaled rTheirPowToOurs = rTheirPow / rOurPow;
	scaled const rPowRatioFloor = fixp(0.5);
	if (rTheirPowToOurs <= rPowRatioFloor)
		return;
	// <!-- custom: Snapshot the projected ratio only when detail logging is active; gameplay mutates rTheirPowToOurs below, so this preserves the pre-transform value without disabled diagnostic work. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	int iProjectedPowRatioPercent = -1;
	if (bLogWarUtilityDetail) iProjectedPowRatioPercent = rTheirPowToOurs.getPercent();
	/*	Proportional to the square of our losses (which are in the unit interval,
		hence sqrt). We want to avoid any tough fights with 2nd parties, when a
		dangerous 3rd-party intervention looms. */
	scaled rCost = rInterventionProb * 85 * rOurLostPowRatio.sqrt();
	// <!-- custom: Keep the inherited high-power reduction available for the one final intervention result row; 1 means the current-power threshold did not trigger a reduction. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	scaled rVeryPowerfulMult = 1;
	{	// These formulas are a bit of a mess
		scaled const rPowRatioThresh = 2;
		if (rTheirPowToOurs > rPowRatioThresh)
		{
			rTheirPowToOurs = rPowRatioThresh +
					(rTheirPowToOurs - rPowRatioThresh + 1).sqrt() - 1;
		}
		else if (rTheirPowToOurs < 1)
			rTheirPowToOurs.exponentiate(fixp(0.5));
		rCost *= rTheirPowToOurs - rPowRatioThresh + 1 + rPowRatioFloor;
		rCost /= 1 + rPowRatioFloor;
		/*	If we can't stand up to them in any case, then we might as well
			expose ourselves and hope for the best. That said, even if we can't
			win, us being able to put up a fight may dissuade them on the
			bottom line. Important only to look at current power values for
			this - to avoid an incentive for incurring greater losses. */
		scaled const rThresh = fixp(5/3.);
		if (rTheirCurPowToOurs > rThresh)
		{
			rVeryPowerfulMult = rThresh / rTheirCurPowToOurs;
			rCost *= rVeryPowerfulMult;
		}
	}
	// <!-- custom: Retain the AI distrust input for the final row; -1 marks the human branch, which instead applies the inherited fixed 2/3 multiplier. Reuse the one gameplay lookup so logging does not add a second distrustRating call. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	int iDistrustPercent = -1;
	if (kOurTeam.isHuman())
	{	// Humans tend to not worry much about being backstabbed
		rCost *= fixp(2/3.);
	}
	else
	{
		scaled const rDistrust = kWeAI.distrustRating();
		if (bLogWarUtilityDetail) iDistrustPercent = rDistrust.getPercent();
		rCost *= rDistrust.sqrt();
	}
	bool const bDifferentCapitalAreas = (kWe.hasCapital() && kThey.hasCapital() &&
			!kWe.getCapital()->sameArea(*kThey.getCapital()));
	if (bDifferentCapitalAreas)
		rCost /= 2;
	bool const bOurDoWBoost = (!militAnalyst().getWarsDeclaredBy(eWe).empty() &&
			!kOurTeam.AI_isSneakAttackPreparing());
	if (bOurDoWBoost)
	{	/*	Not starting war preparations should be less of an inconvenience
			than abandoning preparations or ending an ongoing war. */
		rCost *= fixp(4/3.);
	}
	{
		int const iThresh = 35;
		if (rCost > iThresh)
			rCost = iThresh + (rCost - iThresh).sqrt();
	}
	{
		rCost *= 2;
		rCost /= (kOurTeam.getNumMembers() + kTheirTeam.getNumMembers());
	}
	int iUtility = 0;
	if (rCost >= fixp(0.5))
	{
		iUtility = rCost.uround();
		m_iU -= iUtility;
	}
	// <!-- custom: Consolidate the inherited probability, power-ratio and cost-adjustment narration into one final threat result using the exact utility applied above. (ChatGPT-5.6-Sol) -->
	if (bLogWarUtilityDetail) logBBAI("UWAI_WAR_UTILITY_THIRD_PARTY_RESULT turn=%d agentPlayer=%d rivalPlayer=%d targetTeam=%d consideringPeace=%d overseasDeployment=%d lostDefensivePowerPercent=%d rivalWarUtility=%d noise=%d paranoia=%d dogpileWarRand=%d normalizedPeaceTurns=%d peaceDecayPercent=%d warRand=%d interventionProbabilityPercent=%d rivalCurrentPowerRatioPercent=%d rivalProjectedPower=%d agentProjectedPower=%d rivalProjectedPowerRatioPercent=%d veryPowerfulReductionPercent=%d distrustPercent=%d differentCapitalAreas=%d ourDoWBoost=%d cost=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, eTarget, m_kParams.isConsideringPeace(),
			bOverseasDeployment, rOurLostPowRatio.getPercent(), rUtilityVsUs.round(), rNoise.round(),
			iParanoia, iDogpileWarRand, iNormalizedPeaceTurns, rPeaceDecayMult.getPercent(),
			iWarRand, rInterventionProb.getPercent(), rTheirCurPowToOurs.getPercent(),
			rTheirPow.uround(), rOurPow.uround(), iProjectedPowRatioPercent,
			rVeryPowerfulMult.getPercent(), iDistrustPercent, bDifferentCapitalAreas,
			bOurDoWBoost, rCost.round(), -iUtility);

}


int DramaticArc::preEvaluate()
{
	m_rTensionIncrease = 0;
	if (kOurTeam.isAVassal())
		return 0;
	scaled const rSpeedMult = 2 / per100(m_kGame.getSpeedPercent() +
			m_kSpeed.getTrainPercent());
	int const iElapsedTurns = m_kGame.getElapsedGameTurns();
	if (iElapsedTurns * rSpeedMult < 25) // (Even when starting in a later era)
		return 0;
	int const iOtherKnown = TeamIter<FREE_MAJOR_CIV,OTHER_KNOWN_TO>::count(eOurTeam);
	if (iOtherKnown <= 0)
		return 0;
	int const iMaxPeaceCounter = 100;
	std::vector<int> aiPeaceCounters;
	for (TeamAIIter<FREE_MAJOR_CIV,KNOWN_TO> itFirst(eOurTeam);
		itFirst.hasNext(); ++itFirst)
	{
		int iLoopMinCounter = iElapsedTurns;
		for (TeamIter<MAJOR_CIV,KNOWN_TO> itSecond(eOurTeam);
			itSecond.hasNext(); ++itSecond)
		{
			TeamTypes const eSecond = itSecond->getID();
			if (itFirst->getID() != eSecond)
			{
				int const iAtPeace = itFirst->getTurnsAtPeace(eSecond);
				// <!-- custom: HasMet and TurnsAtPeace are exact counters. Once counted warfare resets TurnsAtPeace, HasMet stays greater; AdvCiv's eight-turn "noise" tolerance instead misclassified short recent wars as eternal peace. See KI#439. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				if (itFirst->AI_getHasMetCounter(eSecond) > iAtPeace)
					iLoopMinCounter = std::min(iLoopMinCounter, iAtPeace);
				// <!-- custom: Otherwise no counted war has occurred since contact. See KI#439. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			}
		}
		aiPeaceCounters.push_back(std::min(iMaxPeaceCounter, iLoopMinCounter));
	}
	int const iMinPeaceCounter = stats::min(aiPeaceCounters);
	// <!-- custom: Hoist the mean for the combined tension row without computing it in the all-at-peace branch, which uses the minimum instead. -1 marks that unused mean; ongoing-war evaluation still computes and uses the inherited mean below. (GPT-6.1-Sol) -->
	int iMeanPeaceCounter = -1;
	scaled rTension;
	if (iMinPeaceCounter > 0) // No wars ongoing
	{
		if (kWe.AI_isDoStrategy(AI_STRATEGY_ALERT1))
			return 0; // We're foreseeing enough tension
		/*	A lack of late-game warfare is not usually a problem,
			and can be due to Cold War dynamics (defensive pacts, nukes). */
		for (int i = 0; i <= m_kGame.getCurrentEra(); i++)
		{
			if (GC.getInfo((EraTypes)i).get(CvEraInfo::AIAgeOfGuns) ||
				GC.getInfo((EraTypes)i).get(CvEraInfo::AIAtomicAge))
			{
				return 0;
			}
		}
		rTension = 1 - scaled(iMinPeaceCounter, iMaxPeaceCounter) * rSpeedMult;
		rTension.increaseTo(0);
		rTension /= 2; // Less than 0.5 when there are no wars
	}
	else
	{
		// Mean of the per-team minima
		iMeanPeaceCounter = stats::mean(aiPeaceCounters);
		rTension = 1 - scaled(iMeanPeaceCounter, iMaxPeaceCounter) * rSpeedMult;
		rTension.increaseTo(0);
		// There is at least one war, so let's not go below 0.5.
		rTension += 1;
		rTension /= 2;
	}
	scaled rTensionTarget = m_kGame.gameTurnProgress() * fixp(10/3.);
	rTensionTarget.decreaseTo(fixp(2/3.));
	if (rTension.approxEquals(rTensionTarget, fixp(0.1)))
		return 0;
	m_rTensionIncrease = (rTensionTarget - rTension) *
			scaled(iOtherKnown).sqrt() / 2;
	m_rTensionIncrease.clamp(-1, 1);
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted()) logBBAI("UWAI_WAR_UTILITY_DRAMATIC_ARC_TENSION turn=%d agentPlayer=%d knownOtherTeams=%d minimumPeaceTurns=%d meanPeaceTurns=%d tensionPercent=%d targetTensionPercent=%d adjustmentPercent=%d",
			GC.getGame().getGameTurn(), eWe, iOtherKnown, iMinPeaceCounter, iMeanPeaceCounter,
			rTension.getPercent(), rTensionTarget.getPercent(), m_rTensionIncrease.getPercent());
	return 0;
}


void DramaticArc::evaluate()
{
	if (m_rTensionIncrease == 0) // Often the case, save time.
		return;
	bool const bWillBeAtWar = militAnalyst().isWar(eWe, eThey);
	if (bWillBeAtWar == kOurTeam.isAtWar(eTheirTeam))
		return;
	if (kTheirTeam.isAVassal()) // Only care about wars between free civs
		return;
	/*	We're already doing our part. And don't prolong wars either just b/c there's
		already too little going on. The end of a war is an interesting event
		in itself, and let's see what will develop in the aftermath. */
	if (m_rTensionIncrease > 0 && kOurTeam.getNumWars() > 0)
		return;
	scaled const rWeight = 16;
	scaled rUtil = m_rTensionIncrease * (bWillBeAtWar ? 1 : -1) * rWeight;
	if (rUtil > 0 && bWillBeAtWar) // Don't encourage phoney or unfair wars ...
	{
		int iTheirLostCities = (int)militAnalyst().lostCities(eThey).size();
		if (iTheirLostCities > 2 || // We should be motivated enough (if it's a fair war)
			// I worry that the AI will go after humans too hard and too often ...
			(kThey.isHuman() && iTheirLostCities == 1 && kThey.hasCapital() &&
			militAnalyst().lostCities(eThey).count(kThey.getCapital()->plotNum()) > 0 &&
			TeamIter<MAJOR_CIV,OTHER_KNOWN_TO>::count(eOurTeam) > 2))
		{
			return;
		}
		if ((iTheirLostCities == 0 && militAnalyst().conqueredCities(eWe).empty()) ||
			(militAnalyst().lostPower(eWe, ARMY) + militAnalyst().lostPower(eThey, ARMY)) <
			std::max(ourCache().getPowerValues()[ARMY]->power(), scaled::epsilon()) / 20)
		{
			return;
		}
	}
	// Tension in team games is jumpy, don't try too hard to steer it.
	int const iTeamMemberSum = kOurTeam.getNumMembers() + kTheirTeam.getNumMembers();
	rUtil /= scaled(iTeamMemberSum, 2);
	// <!-- custom: Store the rounded dramatic-arc contribution once so the result row and gameplay use the same final tension adjustment. (ChatGPT-5.6-Sol) -->
	int const iUtility = rUtil.round();
	if (gUWAIWarUtilityLogLevel >= 3 && !m_kLogMuteState.isMuted() && iUtility != 0) logBBAI("UWAI_WAR_UTILITY_DRAMATIC_ARC_RESULT turn=%d agentPlayer=%d rivalPlayer=%d willBeAtWar=%d currentlyAtWar=%d tensionAdjustmentPercent=%d teamMemberSum=%d utility=%d",
			GC.getGame().getGameTurn(), eWe, eThey, bWillBeAtWar, kOurTeam.isAtWar(eTheirTeam),
			m_rTensionIncrease.getPercent(), iTeamMemberSum, iUtility);
	m_iU += iUtility;
}
