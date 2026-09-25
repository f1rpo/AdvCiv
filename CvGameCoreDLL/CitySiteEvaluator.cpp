#include "CvGameCoreDLL.h"
#include "CitySiteEvaluator.h"
#include "CoreAI.h"
#include "CvCityAI.h"
#include "CvCivilization.h"
#include "PlotRange.h"
#include "CvArea.h"
#include "CvInfo_City.h"
// <!-- custom: Needed for settlement-specific bonus health/happiness from plausible buildings, e.g. Harbor seafood health and capped Incense Cathedral happiness, without using broad AI_bonusVal trade logic. (GPT-5.5 + GPT-5.6-Sol) -->
#include "CvInfo_Building.h"
#include "CvInfo_Terrain.h"
#include "CvInfo_GameOption.h"
#include "CvInfo_Civics.h"
#include "BBAILog.h"
// <!-- custom: added to use our bonusspecific helpers as recommended by chatgpt 5 thanks a lot hehe, check if accurate -->
#include "CvUnitAI.h"

// advc: New file; see comment in the header.

static int const iDEFAULT_BARB_DISCOURAGED_RANGE = 8; // advc.303

// <!-- custom: Keep the three top-level Settler yield-emphasis percentages shared by ordinary plot potential, resource-improvement value, home-plot opportunity and XML-derived scouting references. (GPT-5.6-Sol) -->
static int getSASEvaluateYieldValuePercent(YieldTypes eYield)
{
	static const int iFoodPercent = std::max(0, GC.getDefineINT("SAS_EVALUATE_YIELD_FOOD_VALUE_PERCENT"));
	static const int iProductionPercent = std::max(0, GC.getDefineINT("SAS_EVALUATE_YIELD_PRODUCTION_VALUE_PERCENT"));
	static const int iCommercePercent = std::max(0, GC.getDefineINT("SAS_EVALUATE_YIELD_COMMERCE_VALUE_PERCENT"));
	switch (eYield)
	{
	case YIELD_FOOD: return iFoodPercent;
	case YIELD_PRODUCTION: return iProductionPercent;
	case YIELD_COMMERCE: return iCommercePercent;
	default: FErrorMsg("Unexpected yield type"); return 100;
	}
}

#define IFLOG if (gFoundLogLevel > 0 && AIFoundValue::isLoggingEnabled()) // advc.031c

CitySiteEvaluator::PlotPotentialYield::PlotPotentialYield()
:	iValue(0), eImprovement(NO_IMPROVEMENT), iTimingPercent(0)
{
	FOR_EACH_ENUM(Yield)
		aiYield[eLoopYield] = 0;
}


bool CitySiteEvaluator::getCachedPlotPotentialYield(CvPlot const& kPlot, int& iValue, ImprovementTypes& eImprovement, int* aiYield, int& iTimingPercent) const
{
	std::map<PlotNumTypes, PlotPotentialYield>::const_iterator const it = m_plotPotentialYieldCache.find(kPlot.plotNum());
	if (it == m_plotPotentialYieldCache.end())
		return false;
	PlotPotentialYield const& kCached = it->second;
	iValue = kCached.iValue;
	eImprovement = kCached.eImprovement;
	iTimingPercent = kCached.iTimingPercent;
	FOR_EACH_ENUM(Yield)
		aiYield[eLoopYield] = kCached.aiYield[eLoopYield];
	return true;
}


void CitySiteEvaluator::cachePlotPotentialYield(CvPlot const& kPlot, int iValue, ImprovementTypes eImprovement, int const* aiYield, int iTimingPercent) const
{
	PlotPotentialYield& kCached = m_plotPotentialYieldCache[kPlot.plotNum()];
	kCached.iValue = iValue;
	kCached.eImprovement = eImprovement;
	kCached.iTimingPercent = iTimingPercent;
	FOR_EACH_ENUM(Yield)
		kCached.aiYield[eLoopYield] = aiYield[eLoopYield];
}


bool CitySiteEvaluator::getCachedImprovementProduction(CvPlot const& kPlot, scaled& rValue) const
{
	std::map<PlotNumTypes, scaled>::const_iterator const it = m_improvementProductionCache.find(kPlot.plotNum());
	if (it == m_improvementProductionCache.end())
		return false;
	rValue = it->second;
	return true;
}


void CitySiteEvaluator::cacheImprovementProduction(CvPlot const& kPlot, scaled rValue) const
{
	m_improvementProductionCache[kPlot.plotNum()] = rValue;
}

// Body cut from K-Mod's CvPlayerAI::CvFoundSettings::CvFoundSettings
CitySiteEvaluator::CitySiteEvaluator(CvPlayerAI const& kPlayer, int iMinRivalRange, bool bStartingLoc, /* advc.031e: */ bool bNormalize)
:	m_kPlayer(kPlayer), m_iMinRivalRange(iMinRivalRange), m_bStartingLoc(bStartingLoc),
	m_bNormalize(bNormalize),
	m_bScenario(false), m_bAmbitious(false), m_bExtraYieldThresh(false),
	m_bExtraYieldNaturalThresh(false), // advc.908a
	m_bDefensive(false), m_bSeafaring(false), m_bExpansive(false),
	m_bDebug(false), // advc.007
	m_bAdvancedStart(false), /* advc.027: */ m_bIgnoreStartingSurroundings(false),
	m_iBarbDiscouragedRange(iDEFAULT_BARB_DISCOURAGED_RANGE) // advc.303
{
	m_bScenario = (bStartingLoc ? GC.getInitCore().getWBMapScript() : // advc.031f
			GC.getGame().isScenario()); // advc
	m_bAdvancedStart = (kPlayer.getAdvancedStartPoints() > 0);
	m_bEasyCulture = (kPlayer.getNumCities() <= 0 || // advc.108: from MNAI (was =bStartingLoc)
			m_bAdvancedStart); // advc.031
	m_bAllSeeing = (bStartingLoc || bNormalize || kPlayer.isBarbarian());
	m_bDiagnosticOmniscience = false;
	// advc.031e: No longer use StartingLoc logic for normalization
	FAssert(!bNormalize || !bStartingLoc);

	CvLeaderHeadInfo const* pPersonality = NULL;
	CvCivilization const& kCiv = kPlayer.getCivilization();
	if (!m_bStartingLoc && !m_bNormalize)
	{
		// advc.001: Make sure that personality isn't used for human or StartingLoc
		if (!kPlayer.isHuman() || m_bDebug)
			pPersonality = &GC.getInfo(kPlayer.getPersonalityType());
		bool bEasyCultureFromTrait = false;
		m_bEasyCulture = kPlayer.AI_isEasyCulture(&bEasyCultureFromTrait);
		if (bEasyCultureFromTrait && pPersonality != NULL &&
			//pPersonality->getBasePeaceWeight() <= 5
			pPersonality->getBuildUnitProb() > 25) // advc.031
		{
			m_bAmbitious = true;
		}
		FOR_EACH_ENUM(Trait)
		{
			if (!kPlayer.hasTrait(eLoopTrait))
				continue;

			CvTraitInfo const& kLoopTrait = GC.getInfo(eLoopTrait);
			if (kLoopTrait.getExtraYieldThreshold(YIELD_COMMERCE) > 0)
				m_bExtraYieldThresh = true;
			// <advc.908a>
			if (kLoopTrait.getExtraYieldNaturalThreshold(YIELD_COMMERCE) > 0)
				m_bExtraYieldNaturalThresh = true; // </advc.908a>
			if (kLoopTrait.isAnyFreePromotion()) // advc.003t
			{
				FOR_EACH_ENUM(Promotion)
				{
					if (kLoopTrait.isFreePromotion(eLoopPromotion))
					{
						// aggressive, protective... it doesn't really matter to me.
						if (pPersonality != NULL && pPersonality->getBasePeaceWeight() >= 5)
							m_bDefensive = true;
					}
				}
			}
			if (pPersonality != NULL && pPersonality->getMaxWarRand() <= 150) // advc.opt: moved up
			{
				for (int i = 0; i < kCiv.getNumUnits(); i++)
				{
					UnitTypes eFoundUnit = kCiv.unitAt(i);
					if (GC.getInfo(eFoundUnit).isFound() &&
						// advc.031: was > 0
						GC.getInfo(eFoundUnit).getProductionTraits(eLoopTrait) > 20 &&
						kPlayer.canTrain(eFoundUnit))
					{
						m_bAmbitious = true;
					}
				}
			}
		}
		// seafaring test for unique unit and unique building
		if (kPlayer.getCoastalTradeRoutes() > 0)
			m_bSeafaring = true;
		if (!m_bSeafaring)
		{
			for (int i = 0; i < kCiv.getNumUniqueUnits(); i++) // advc.003w
			{
				UnitTypes eUniqueUnit = kCiv.uniqueUnitAt(i);
				if (GC.getInfo(eUniqueUnit).getDomainType() == DOMAIN_SEA)
				{
					m_bSeafaring = true;
					break;
				}
			}
		}
		if (!m_bSeafaring)
		{
			for (int i = 0; i < kCiv.getNumUniqueBuildings(); i++) // advc.003w
			{
				BuildingTypes eUniqueBuilding = kCiv.uniqueBuildingAt(i);
				if (GC.getInfo(eUniqueBuilding).isWater())
				{
					m_bSeafaring = true;
					break;
				}
			}
		}
	}

	if (pPersonality != NULL && kPlayer.AI_getFlavorValue(FLAVOR_GROWTH) > 0)
		m_bExpansive = true;

	m_iClaimThreshold = 100; // will be converted into culture units
	if (!bStartingLoc && !bNormalize)
	{
		int iCitiesTarget = std::max(1,
				GC.getInfo(GC.getMap().getWorldSize()).getTargetNumCities());
		m_iClaimThreshold = 100 +
				(100 * kPlayer.getCurrentEra()) / std::max(1, GC.getNumEraInfos() - 1);
		m_iClaimThreshold += 80 * std::max(0,
				iCitiesTarget - kPlayer.getNumCities()) / iCitiesTarget;

		m_iClaimThreshold *= (!m_bEasyCulture ? 100 :
				(kPlayer.getCurrentEra() < 2 ? 200 : 150));
		m_iClaimThreshold *= (m_bAmbitious ? 150 : 100);
		m_iClaimThreshold /= 10000;
	}
	m_iClaimThreshold *= 2 * GC.getGame().getCultureThreshold((CultureLevelTypes)
			std::min(2, GC.getNumCultureLevelInfos() - 1));
	// note: plot culture is roughly 10x city culture (cf. CvCity::doPlotCultureTimes100)
	FAssert(m_iClaimThreshold > 0);

	if (m_bAdvancedStart)
	{
		FAssert(GC.getGame().isOption(GAMEOPTION_ADVANCED_START));
		if (m_bStartingLoc || m_bNormalize)
			m_bAdvancedStart = false;
	}
	IFLOG logSettings();
}


// <!-- custom: found-value evaluation now returns int (not short) to avoid overflow/underflow. (GPT-5.2-Codex (summarized)) -->
int CitySiteEvaluator::evaluate(CvPlot const& kPlot) const
{
	AIFoundValue foundVal(kPlot, *this);
	return foundVal.get();
}


int CitySiteEvaluator::evaluate(int iX, int iY) const
{
	CvPlot const* pPlot = GC.getMap().plot(iX, iY);
	if (pPlot == NULL)
		return 0;
	return evaluate(*pPlot);
}


// <!-- custom: Build a compact exact-stage breakdown only for guarded first-city diagnostics. Normal evaluation passes no output string and does no formatting. (GPT-5.5) -->
int CitySiteEvaluator::evaluateWithBreakdown(CvPlot const& kPlot, CvString& szBreakdown) const
{
	AIFoundValue foundVal(kPlot, *this, &szBreakdown);
	return foundVal.get();
}


// <!-- custom: Expand the prior best-six output with best ten so first-city decisions can compare both early and developed worked-plot cores in one evaluation; finer best-1/2/3 and mature best-14 values remain diagnostic only. (GPT-5.6-Sol) -->
int CitySiteEvaluator::evaluateWithGrowthCorePlotValues(CvPlot const& kPlot, int& iBest6PlotValue, int& iBest10PlotValue, int& iSustainableProductivePlotValue) const
{
	int aiGrowthCorePlotValues[2] = {0, 0};
	iSustainableProductivePlotValue = 0;
	AIFoundValue foundVal(kPlot, *this, NULL, true, aiGrowthCorePlotValues, &iSustainableProductivePlotValue);
	iBest6PlotValue = aiGrowthCorePlotValues[0];
	iBest10PlotValue = aiGrowthCorePlotValues[1];
	return foundVal.get();
}


// <!-- custom: Expose the complete 1/2/3/6/10/14 plot-value distribution only to guarded diagnostics such as SASGameRecord city founding.
// AIFoundValue already computes these values in the ordinary site pass; returning them here avoids a second BFC valuation or text parsing. (ChatGPT-5.6-Sol) -->
int CitySiteEvaluator::evaluateWithPlotValueDistribution(CvPlot const& kPlot, int* aiCoreSums, int& iPositivePlots, int& iSustainableProductivePlotValue) const
{
	FAssert(aiCoreSums != NULL);
	int aiCoreCutoffs[6] = {0, 0, 0, 0, 0, 0};
	for (int iCore = 0; iCore < 6; ++iCore)
		aiCoreSums[iCore] = 0;
	iPositivePlots = 0;
	iSustainableProductivePlotValue = 0;
	AIFoundValue foundVal(kPlot, *this, NULL, true, NULL, &iSustainableProductivePlotValue, aiCoreSums, aiCoreCutoffs, &iPositivePlots);
	return foundVal.get();
}


int CitySiteEvaluator::getSustainableProductivePlotValue()
{
	static const int iExtraValue = GC.getDefineINT("SAS_EVALUATE_NATURE_YIELD_SELF_SUSTAINING_EXTRA_VALUE");
	static const int iFoodValue = GC.getDefineINT("SAS_EVALUATE_NATURE_YIELD_SELF_SUSTAINING_FOOD_VALUE");
	static const int iProductionValue = GC.getDefineINT("SAS_EVALUATE_NATURE_YIELD_SELF_SUSTAINING_PRODUCTION_VALUE");
	return iExtraValue + (GC.getFOOD_CONSUMPTION_PER_POPULATION() * iFoodValue * getSASEvaluateYieldValuePercent(YIELD_FOOD)) / 100 + (iProductionValue * getSASEvaluateYieldValuePercent(YIELD_PRODUCTION)) / 100;
}

// advc.300:
void CitySiteEvaluator::discourageBarbarians(int iRange)
{
	m_iBarbDiscouragedRange = iRange;
}

// <advc.027>
void CitySiteEvaluator::setIgnoreStartingSurroundings(bool b)
{
	m_bIgnoreStartingSurroundings = b;
}


scaled CitySiteEvaluator::evaluateWorkablePlot(CvPlot const& kPlot) const
{
	// Should only be used for StartingPositionIteration
	FAssert(isNormalizing() && !isDebug());
	/*	The evaluation is going to be largely independent from any city tile,
		but it could matter whether it's coastal and on the same landmass.
		And for the evaluation of water tiles, I want to be able to guess
		based on the potential city tile whether it's arctic water. */
	CvPlot const* pBestCityPlot = NULL;
	int iBestScore = 0;
	int const iBestPossibleScore = (kPlot.isWater() ? 4 : 5);
	for (CityPlotIter it(kPlot, false); it.hasNext(); ++it)
	{
		if (!m_kPlayer.canFound(*it))
			continue;
		int iScore = 1;
		if (kPlot.isWater())
		{
			if (it->isCoastalLand(-1))
				iScore += 2;
			if (GC.getInfo(it->getTerrainType()).getYield(YIELD_FOOD) > 0)
				iScore++;
		}
		else
		{
			if (kPlot.sameArea(*it))
				iScore += 4;
		}
		if (iScore > iBestScore)
		{
			pBestCityPlot = &*it;
			iBestScore = iScore;
			if (iScore >= iBestPossibleScore)
				break;
		}
	}
	if (pBestCityPlot == NULL) // kPlot not workable
		return 0;
	// <!-- custom: The independent workable-plot path needs the evaluator context but not the constructor's full city-site result. See KI#492. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	AIFoundValue foundVal(*pBestCityPlot, *this, NULL, false);
	CvGame& kGame = GC.getGame();
	CvGame::StartingPlotNormalizationLevel eOldLevel = kGame.
			getStartingPlotNormalizationLevel();
	// Make sure that resources get revealed
	kGame.setStartingPlotNormalizationLevel(CvGame::NORMALIZE_HIGH);
	scaled r = foundVal.evaluateWorkablePlot(kPlot);
	kGame.setStartingPlotNormalizationLevel(eOldLevel);
	return r;
} // </advc.027>

// advc.007:
void CitySiteEvaluator::setDebug(bool b)
{
	m_bDebug = b;
}

// <advc.031c>
int CitySiteEvaluator::evaluateWithLogging(CvPlot const& kPlot) const
{
	// <!-- custom: avoid the logging pass and log-argument evaluation when found-value logging is disabled. (ChatGPT 5.5) -->
	if (gFoundLogLevel <= 0)
		return evaluate(kPlot);

	AIFoundValue::setLoggingEnabled(true);
	int r = evaluate(kPlot);
	AIFoundValue::setLoggingEnabled(false);
	return r;
}


// <!-- custom: Selected-site breakdown was already compact, but save-file 450's uMgungundlovu regression needed the same compact row for adjacent/next alternatives too; otherwise the relevant 18,40 vs 17,40 delta was buried in verbose per-plot logs. (GPT-5.5) -->
void CitySiteEvaluator::logComparedSiteBreakdown(char const* szLabel, CvPlot const& kPlot) const
{
	CvString szBreakdown;
	int const iBreakdownValue = evaluateWithBreakdown(kPlot, szBreakdown);
	logBBAI("%s breakdown value=%d %s", szLabel, iBreakdownValue, szBreakdown.GetCString());
}


// <!-- custom: Founding logs explained seafood only when that site happened to be the selected, next-best, or adjacent comparison.
// Count either founder-known non-obsolete water bonuses or true-map water bonuses so diagnostics can distinguish low valuation, hidden resources and a site never surviving into the maintained candidate list; diagnostic only. (GPT-5.6-Sol) -->
int CitySiteEvaluator::countWaterBonuses(CvPlot const& kCityPlot, TeamTypes eTeam, bool bDiagnosticOmniscience)
{
	int iWaterBonuses = 0;
	for (CityPlotIter it(kCityPlot, false); it.hasNext(); ++it)
	{
		if (it->isWater() && (bDiagnosticOmniscience ? it->getBonusType() : it->getNonObsoleteBonusType(eTeam)) != NO_BONUS)
			iWaterBonuses++;
	}
	return iWaterBonuses;
}


void CitySiteEvaluator::log(CvPlot const& kPlot)
{
	/*  Important to ignore other city sites. Because, when CvPlayerAI::
		AI_updateCitySites computes the found value of the best site,
		none of the other sites are chosen yet. Here, all sites are chosen. */
	setDebug(true);
	if (isStartingLoc())
		logBBAI("\n\nStarting location found at (%d,%d)", kPlot.getX(), kPlot.getY());
	else if (isNormalizing())
		logBBAI("\n\nNormalizing starting plot (%d,%d)", kPlot.getX(), kPlot.getY());
	else
	{
		logBBAI("\n\n%S is about to found a city at (%d,%d); turn %d (year %d)", getPlayer().getName(),
				kPlot.getX(), kPlot.getY(), GC.getGame().getGameTurn(), GC.getGame().getGameTurnYear());
		logBBAI("Lower bound for found value: %d", getPlayer().AI_getMinFoundValue());
	}
	// <!-- custom: Log the selected site's final value beside adjacent/next alternatives; the verbose component dump alone made save file 450's 18,40 vs 17,40 comparison unnecessarily hard to verify. (GPT-5.5) -->
	int const iCurrentValue = evaluateWithLogging(kPlot);
	bool const bLogComparedSiteBreakdown = (gFoundLogLevel >= 2);
	if (gFoundLogLevel >= 2)
	{
		CvString szCurrentBreakdown;
		int const iCurrentBreakdownValue = evaluateWithBreakdown(kPlot, szCurrentBreakdown);
		logBBAI("Selected site breakdown value=%d %s", iCurrentBreakdownValue, szCurrentBreakdown.GetCString());
	}
	if (isStartingLoc())
		return;
	if (getPlayer().isBarbarian())
	{
		// <!-- custom: Barbarian cities are spawned by a global scan rather than normal Settler city-site lists, and old Barbarian scoring can ignore outer-BFC seafood or other long-term capture value.
		// Log nearby alternatives so cases like Yue-Chi, Sarmatian, Aryan, and Numidian can show whether the selected spawn tile or a one-tile shift was actually better. (GPT-5.5) -->
		logBBAI("Barbarian selected site spawn eligibility: water=%d visibleToCivTeam=%d spawnEligible=%d",
			kPlot.isWater(), kPlot.isVisibleToCivTeam(), !kPlot.isWater() && !kPlot.isVisibleToCivTeam());
		CvPlot const* pBestAdjSite = NULL;
		CvPlot const* pBestEligibleAdjSite = NULL;
		int iBestAdj = 0;
		int iBestEligibleAdj = 0;
		FOR_EACH_ADJ_PLOT(kPlot)
		{
			int iValue = evaluate(*pAdj);
			if (iValue > iBestAdj)
			{
				pBestAdjSite = pAdj;
				iBestAdj = iValue;
			}
			if (!pAdj->isWater() && !pAdj->isVisibleToCivTeam() && iValue > iBestEligibleAdj)
			{
				pBestEligibleAdjSite = pAdj;
				iBestEligibleAdj = iValue;
			}
		}
		if (pBestAdjSite != NULL)
		{
			int iAdjX = pBestAdjSite->getX();
			int iAdjY = pBestAdjSite->getY();
			logBBAI("\nBest Barbarian site adjacent to (%d,%d): selected=%d adjacent=%d delta=%+d (%d,%d) water=%d visibleToCivTeam=%d spawnEligible=%d",
				kPlot.getX(), kPlot.getY(), iCurrentValue, iBestAdj, iBestAdj - iCurrentValue, iAdjX, iAdjY, pBestAdjSite->isWater(),
				pBestAdjSite->isVisibleToCivTeam(), !pBestAdjSite->isWater() && !pBestAdjSite->isVisibleToCivTeam());
			evaluateWithLogging(*pBestAdjSite);
			if (bLogComparedSiteBreakdown) logComparedSiteBreakdown("Best adjacent Barbarian site", *pBestAdjSite);
		}
		if (pBestEligibleAdjSite != NULL && pBestEligibleAdjSite != pBestAdjSite)
		{
			int iAdjX = pBestEligibleAdjSite->getX();
			int iAdjY = pBestEligibleAdjSite->getY();
			logBBAI("\nBest spawn-eligible Barbarian site adjacent to (%d,%d): selected=%d adjacent=%d delta=%+d (%d,%d)",
				kPlot.getX(), kPlot.getY(), iCurrentValue, iBestEligibleAdj, iBestEligibleAdj - iCurrentValue, iAdjX, iAdjY);
			evaluateWithLogging(*pBestEligibleAdjSite);
			if (bLogComparedSiteBreakdown) logComparedSiteBreakdown("Best spawn-eligible adjacent Barbarian site", *pBestEligibleAdjSite);
		}
		CvPlot const* pBestRange2Site = NULL;
		CvPlot const* pBestEligibleRange2Site = NULL;
		int iBestRange2 = 0;
		int iBestEligibleRange2 = 0;
		for (int iDX = -2; iDX <= 2; iDX++)
		{
			for (int iDY = -2; iDY <= 2; iDY++)
			{
				CvPlot const* pLoopPlot = GC.getMap().plot(kPlot.getX() + iDX, kPlot.getY() + iDY);
				if (pLoopPlot == NULL || pLoopPlot == &kPlot || plotDistance(&kPlot, pLoopPlot) != 2)
					continue;
				int iValue = evaluate(*pLoopPlot);
				if (iValue > iBestRange2)
				{
					pBestRange2Site = pLoopPlot;
					iBestRange2 = iValue;
				}
				if (!pLoopPlot->isWater() && !pLoopPlot->isVisibleToCivTeam() && iValue > iBestEligibleRange2)
				{
					pBestEligibleRange2Site = pLoopPlot;
					iBestEligibleRange2 = iValue;
				}
			}
		}
		if (pBestRange2Site != NULL)
		{
			int iRangeX = pBestRange2Site->getX();
			int iRangeY = pBestRange2Site->getY();
			logBBAI("\nBest Barbarian site at distance 2 from (%d,%d): selected=%d range2=%d delta=%+d (%d,%d) water=%d visibleToCivTeam=%d spawnEligible=%d",
				kPlot.getX(), kPlot.getY(), iCurrentValue, iBestRange2, iBestRange2 - iCurrentValue, iRangeX, iRangeY,
				pBestRange2Site->isWater(), pBestRange2Site->isVisibleToCivTeam(),
				!pBestRange2Site->isWater() && !pBestRange2Site->isVisibleToCivTeam());
			evaluateWithLogging(*pBestRange2Site);
			if (bLogComparedSiteBreakdown) logComparedSiteBreakdown("Best distance-2 Barbarian site", *pBestRange2Site);
		}
		if (pBestEligibleRange2Site != NULL && pBestEligibleRange2Site != pBestRange2Site)
		{
			int iRangeX = pBestEligibleRange2Site->getX();
			int iRangeY = pBestEligibleRange2Site->getY();
			logBBAI("\nBest spawn-eligible Barbarian site at distance 2 from (%d,%d): selected=%d range2=%d delta=%+d (%d,%d)",
				kPlot.getX(), kPlot.getY(), iCurrentValue, iBestEligibleRange2, iBestEligibleRange2 - iCurrentValue, iRangeX, iRangeY);
			evaluateWithLogging(*pBestEligibleRange2Site);
			if (bLogComparedSiteBreakdown) logComparedSiteBreakdown("Best spawn-eligible distance-2 Barbarian site", *pBestEligibleRange2Site);
		}
		return;
	}
	CvPlot const* pNextBestSite = NULL;
	int iNextBestValue = 0;
	for (int i = 0; i < getPlayer().AI_getNumCitySites(); i++)
	{
		CvPlot const& kLoopPlot = getPlayer().AI_getCitySite(i);
		if (&kLoopPlot == &kPlot)
			continue;
		int const iValue = evaluate(kLoopPlot);
		if (iValue > iNextBestValue)
		{
			pNextBestSite = &kLoopPlot;
			iNextBestValue = iValue;
		}
	}
	if (pNextBestSite != NULL)
	{
		int const iNextX = pNextBestSite->getX();
		int const iNextY = pNextBestSite->getY();
		logBBAI("\nNext best site compared with selected (%d,%d): selected=%d next=%d delta=%+d (%d,%d)",
			kPlot.getX(), kPlot.getY(), iCurrentValue, iNextBestValue, iNextBestValue - iCurrentValue, iNextX, iNextY);
		evaluateWithLogging(*pNextBestSite);
		if (bLogComparedSiteBreakdown) logComparedSiteBreakdown("Next best site", *pNextBestSite);
	}
	if (bLogComparedSiteBreakdown)
	{
		const int iMinWaterSizeForOcean = GC.getDefineINT(CvGlobals::MIN_WATER_SIZE_FOR_OCEAN);
		CvPlot const* pBestKnownWaterBonusSite = NULL;
		int iBestKnownWaterBonusSiteValue = 0;
		int iBestKnownWaterBonuses = 0;
		for (int i = 0; i < getPlayer().AI_getNumCitySites(); i++)
		{
			CvPlot const& kLoopPlot = getPlayer().AI_getCitySite(i);
			if (&kLoopPlot == &kPlot || !kLoopPlot.isCoastalLand(iMinWaterSizeForOcean))
				continue;
			int const iKnownWaterBonuses = countWaterBonuses(kLoopPlot, getPlayer().getTeam(), false);
			if (iKnownWaterBonuses <= 0)
				continue;
			int const iValue = evaluate(kLoopPlot);
			if (iValue > iBestKnownWaterBonusSiteValue)
			{
				pBestKnownWaterBonusSite = &kLoopPlot;
				iBestKnownWaterBonusSiteValue = iValue;
				iBestKnownWaterBonuses = iKnownWaterBonuses;
			}
		}
		bool const bSelectedCoastal = kPlot.isCoastalLand(iMinWaterSizeForOcean);
		int const iSelectedKnownWaterBonuses = (bSelectedCoastal ? countWaterBonuses(kPlot, getPlayer().getTeam(), false) : 0);
		if (pBestKnownWaterBonusSite == NULL)
		{
			logBBAI("\nListed coastal water-bonus site audit: selectedCoastal=%d selectedKnownWaterBonuses=%d alternative=NONE citySites=%d",
				bSelectedCoastal, iSelectedKnownWaterBonuses, getPlayer().AI_getNumCitySites());
		}
		else
		{
			bool const bSameAsNextBest = (pBestKnownWaterBonusSite == pNextBestSite);
			logBBAI("\nListed coastal water-bonus site audit: selectedCoastal=%d selectedKnownWaterBonuses=%d selected=%d alternative=%d delta=%+d (%d,%d) alternativeKnownWaterBonuses=%d sameAsNextBest=%d citySites=%d",
				bSelectedCoastal, iSelectedKnownWaterBonuses, iCurrentValue, iBestKnownWaterBonusSiteValue,
				iBestKnownWaterBonusSiteValue - iCurrentValue, pBestKnownWaterBonusSite->getX(), pBestKnownWaterBonusSite->getY(), iBestKnownWaterBonuses, bSameAsNextBest, getPlayer().AI_getNumCitySites());
			if (!bSameAsNextBest)
			{
				evaluateWithLogging(*pBestKnownWaterBonusSite);
				logComparedSiteBreakdown("Best listed coastal water-bonus site", *pBestKnownWaterBonusSite);
			}
		}
	}
	{
		CvPlot const* pBestAdjSite = NULL;
		int iBest = 0;
		FOR_EACH_ADJ_PLOT(kPlot)
		{
			int iValue = evaluate(*pAdj);
			if (iValue > iBest)
			{
				pBestAdjSite = pAdj;
				iBest = iValue;
			}
		}
		if (pBestAdjSite != NULL)
		{
			int iAdjX = pBestAdjSite->getX();
			int iAdjY = pBestAdjSite->getY();
			logBBAI("\nBest site adjacent to (%d,%d): selected=%d adjacent=%d delta=%+d (%d,%d)",
				kPlot.getX(), kPlot.getY(), iCurrentValue, iBest, iBest - iCurrentValue, iAdjX, iAdjY);
			evaluateWithLogging(*pBestAdjSite);
			if (bLogComparedSiteBreakdown) logComparedSiteBreakdown("Best adjacent site", *pBestAdjSite);
		}
	}
}


bool AIFoundValue::bLoggingEnabled = false;


void AIFoundValue::setLoggingEnabled(bool b)
{
	bLoggingEnabled = b;
} // </advc.031c>

// <!-- custom: Keep ordinary callers evaluating immediately, while allowing SPI's separate workable-plot precomputation to skip the unused full result.
// Optional outputs expose best-6/10 sums, the shared productive-plot reference and defense adjustment already computed in the same pass without diagnostic string formatting or repeated site evaluations. See KI#492. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
AIFoundValue::AIFoundValue(CvPlot const& kPlot, CitySiteEvaluator const& kSettings, CvString* pszBreakdown, bool bEvaluateSite, int* paiGrowthCorePlotValues, int* piSustainableProductivePlotValue, int* paiPlotCoreSums, int* paiPlotCoreCutoffs, int* piPositivePlots) :
	m_iResult(0), m_pszBreakdown(pszBreakdown), m_paiGrowthCorePlotValues(paiGrowthCorePlotValues), m_piSustainableProductivePlotValue(piSustainableProductivePlotValue),
	m_paiPlotCoreSums(paiPlotCoreSums), m_paiPlotCoreCutoffs(paiPlotCoreCutoffs), m_piPositivePlots(piPositivePlots), kPlot(kPlot),
	kArea(kPlot.getArea()), kSet(kSettings), kPlayer(kSet.getPlayer()), ePlayer(kPlayer.getID()),
	eTeam(kPlayer.getTeam()), kTeam(GET_TEAM(eTeam)), kGame(GC.getGame()), iX(kPlot.getX()), iY(kPlot.getY())
{
	PROFILE_FUNC();
	if (!kPlayer.canFound(kPlot, false,
		/*	advc.181: Don't let action recommendations for human settlers
			give away rival cities founded in the fog of war. */
		!kPlayer.isHuman() || kSet.isAllSeeing()))
	{
		return;
	}
	bBarbarian = kPlayer.isBarbarian();
	eEra = kPlayer.getCurrentEra();
	rAIEraFactor = kPlayer.AI_getCurrEraFactor();
	bCoastal = kPlot.isCoastalLand(-1);
	iAreaCities = kArea.getCitiesPerPlayer(ePlayer);
	pCapital = kPlayer.AI_getCapital();
	// <advc.108> Barbarians shouldn't distinguish between earlier and later cities
	iCities = (bBarbarian ? 5 : kPlayer.getNumCities());
	FAssert(iCities > 0 || !kPlayer.isFoundedFirstCity());
	// </advc.108>

	//"nice hacky way to avoid messing with normalizer"
	// advc.031e: Now only used for calls from Python; see CyPlayer::AI_foundValue.
	//bNormalize = (kSet.isStartingLoc() && &kPlot == kPlayer.getStartingPlot());

	bFirstColony = false;
	iUnrevealedTiles = 0;

	// <!-- custom: Only the SPI workable-plot caller disables the otherwise immediate full evaluation. See KI#492. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (bEvaluateSite)
		m_iResult = evaluate();
}

// <!-- custom: we now return int (no longer short) in AIFoundValue::evaluate, so overflow risk is much lower; keep penalties bounded but don't expect short wraparound behavior here. (GPT-5.2-Codex (summarized)) -->
/*  Body from CvPlayerAI::AI_foundValue_bulk, split into subroutines.
	More refactoring could be done; for one thing, some of the functions have
	long (return) parameter lists. Should probably introduce a class CityPlot
	for the data (and behavior) in the plot evaluation loop. At least it's
	no longer a single 1000-LoC function inside the largest GameCore class.
	K-Mod: Heavily edited (some changes marked, others not.)
	note, this function is called for every revealed plot for every player
	at the start of every turn. try to not make it too slow! */
int AIFoundValue::evaluate()
{
	IFLOG logSite();
	if (m_pszBreakdown != NULL)
		m_pszBreakdown->clear();

	if (!isSiteValid() || !computeOverlap())
	{
		IFLOG logBBAI("Site disregarded");
		return 0;
	}

	// Bad tiles, unrevealed tiles, first colony
	int iBadTiles = 0;
	int iInnerBadTiles = 0;
	int iLandTiles = 0; // advc.031
	// <advc.040>

	// <!-- custom: add support for counting water tiles in starting locations -->
	// int const iElapsedTurns = GC.getGame().getElapsedGameTurns();
	// <!-- custom: AIFoundValue normalizes Barbarian city count so their first settlement is not treated like a capital. Reuse that established state for SAS first-city gates instead of raw player city count. See KI#489. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	bool const bStartPhase = (iCities == 0);

	// <!-- custom: Count first-city BFC slots by XML-aware usability rather than the old no-peak-land + water-bonus approximation. "Good enough" is intentionally weaker than truly good: it means a slot is usable enough for the first-city minimum-tile sanity check. (ChatGPT-5.5) -->
	int iGoodEnoughFirstCityBFCTiles = 0;
	int iVeryBadBFCTiles = 0;

	static const int iMaxToleratedVeryBadTilesStart = GC.getDefineINT("SAS_EVALUATE_MAX_TOLERATED_NOT_HOME_VERY_BAD_TILES_START");
	static const int iMaxToleratedVeryBadTilesLater = GC.getDefineINT("SAS_EVALUATE_MAX_TOLERATED_NOT_HOME_VERY_BAD_TILES_LATER");
	static const int iMinAcceptableVeryBadPlotPotentialYieldScore = GC.getDefineINT("SAS_EVALUATE_MIN_ACCEPTABLE_NOT_HOME_PLOT_POTENTIAL_YIELD_SCORE");

	static const int iBaseValueVeryBadTileStart = GC.getDefineINT("SAS_EVALUATE_BASE_VALUE_NOT_HOME_VERY_BAD_TILE_START");
	static const int iBaseValueVeryBadTileLater = GC.getDefineINT("SAS_EVALUATE_BASE_VALUE_NOT_HOME_VERY_BAD_TILE_LATER");

	int iCautiousHealthPercent = 0;

	bFirstColony = isPrioritizeAsFirstColony();
	IFLOG if(bFirstColony) logBBAI("First colony");
	// Scope for countBadTiles return parameters
	{
		int iRevealedDecentLand = 0;
		// </advc.040>
		int iUnrevealed = 0;
		iBadTiles = countBadTiles(iInnerBadTiles, iUnrevealed, iLandTiles,
				iRevealedDecentLand);
		iUnrevealedTiles = iUnrevealed;
		/*  <advc.040> Make sure we do naval exploration near the site before
			sending a Settler */
		if (iRevealedDecentLand < 4)
		{
			IFLOG if(bFirstColony) logBBAI("Only %d decent revealed land tiles; first-colony logic disabled.", iRevealedDecentLand);
			bFirstColony = false;
		} // </advc.040>
	}
	if (isTooManyBadTiles(iBadTiles, iInnerBadTiles))
	{
		IFLOG logBBAI("Too many bad tiles");
		return 0;
	}

	const int iBreakdownBase = baseCityValue();
	int iValue = iBreakdownBase;
	IFLOG logBBAI("Base city value: %d", iValue);
	int iBreakdownHomeWater = 0;
	int iBreakdownRiverBFC = 0;
	int iBreakdownDirect = 0;
	int iBreakdownPlots = 0;
	int iBreakdownNonYieldResources = 0;
	int iBreakdownBonusImprovementYields = 0;
	int iBreakdownResourcesAdded = 0;
	int iBreakdownHealth = 0;
	int iBreakdownFeatureProduction = 0;
	int iBreakdownSea = 0;
	int iBreakdownLowFood = 0;
	int iBreakdownVeryBad = 0;
	// <!-- custom: Diagnostic-only outputs are fully assigned by sumUpPlotValues when requested; leave them untouched on the normal hot path.
	// Best-1/2/3 reveal whether an opening depends on one exceptional resource or several strong plots, while best-6/10/14 remain the broader growth stages used by first-city decisions. (GPT-5.6-Sol) -->
	int aiBreakdownPlotCoreSums[6];
	int aiBreakdownPlotCoreCutoffs[6];
	int iBreakdownPositivePlots;

	IFLOG logBBAI("Evaluate city radius ...");
	std::vector<int> aiPlotValues(NUM_CITY_PLOTS, 0);

	std::vector<int> aiBonusCount(GC.getNumBonusInfos(), 0);
	// <!-- custom: add extra valuation for unowned bonuses. See KI#147. -->
	std::vector<bool> abSASUnownedBonusExtraApplied(GC.getNumBonusInfos(), false);
	int iResourceValue = 0;
	int aiSpecialYield[NUM_YIELD_TYPES] = {0, 0, 0}; // advc: Instead of individual variables
	/*  advc (comment): So that we can tell whether the city
		will be able to work its high-yield tiles */
	int iSpecialFoodPlus = 0;
	int iSpecialFoodMinus = 0;
	int iSpecialYieldTiles = 0; // advc.031: Number of tiles with special yield

	// advc: TeamMateTakenTiles code deleted (was dead code b/c of K-Mod changes)
	//int iYieldLostHere = 0; // advc: unused
	int iTakenTiles = 0;
	//bool bNeutralTerritory = true;
	bool bAnyForeignOwned = false; // advc.040: Replacing the above
	// <advc.031>
	int iGoody = 0; // for normalization
	int iStealPercent = 0;
	int iRiverTiles = 0;
	int iGreenTiles = 0;
	// <!-- custom: enhance and add logging for first-city found logic: add scouting, moving away from high bad plot count starts, and running again an evaluate city site scoring when a site is no longer bad instead of settling right away; also add logging. See KI#144. Additionally, add a stronger food and river/fresh water valuation for first city because it is more critical for it. -->
	int iStartingRiverTiles = 0;
	int iTotalFeatureProduction = 0; // </advc.031>
	int iHealth = 0;

	// K-Mod. (used to devalue cities which are unable to get any production.)
	scaled rBaseProduction; // (advc.031: scaled)

	// <!-- custom: Sum each usable non-home BFC plot's shared potential-food score instead of naming terrain types here. Food from bonus-specific improvements offsets food-poor plots; an ocean-coastal candidate assumes its configured, civilization-specific Harbor-class water-food yield because that cheap city-wide improvement is the practical long-term water baseline. Very-bad plots are excluded because they are already handled as economically dead BFC capacity and will not normally consume a citizen. (GPT-5.5) -->
	int iLowFoodLocationScore = 0;
	bool const bOceanCoastal = kPlot.isCoastalLand(GC.getDefineINT(CvGlobals::MIN_WATER_SIZE_FOR_OCEAN));
	int const iAssumedSeaPlotFoodChange = (bOceanCoastal ? CvPlot::SAS_getWaterFoodBuildingSeaPlotFoodChange(ePlayer) : 0);

	static const int iExtraValueHomeFreshWaterRiver = GC.getDefineINT("SAS_EVALUATE_EXTRA_VALUE_HOME_FRESH_WATER_RIVER");

	// <!-- custom: no need to make the cached CvGlobals defines into static const according to chatgpt 5, but hoist them as recommended by chatgpt 5, check if accurate -->
	// <!-- custom: code/performance optimization: hoist -->
	// Hoist once per call; no need for static
	const bool bGlobalsOwnExclusiveRadius = GC.getDefineBOOL(CvGlobals::OWN_EXCLUSIVE_RADIUS);
	const int iGlobalsExtraYield = GC.getDefineINT(CvGlobals::EXTRA_YIELD);

	static const int iMinOnBonusFoodImproveWorth = GC.getDefineINT("SAS_EVALUATE_VALUE_HOME_ON_BONUS_FOOD_MIN_IMPROVE_WORTH");
	static const int iBaseValueOnBonusFood = GC.getDefineINT("SAS_EVALUATE_BASE_VALUE_HOME_ON_BONUS_FOOD");

	static const int iMinOnBonusProductionImproveWorth = GC.getDefineINT("SAS_EVALUATE_VALUE_HOME_ON_BONUS_PRODUCTION_MIN_IMPROVE_WORTH");
	static const int iBaseValueOnBonusProduction = GC.getDefineINT("SAS_EVALUATE_VALUE_HOME_ON_BONUS_PRODUCTION");
	static const int iValueVeryLowFoodOnBonusProduction = GC.getDefineINT("SAS_EVALUATE_VALUE_HOME_ON_BONUS_PRODUCTION_VERY_LOW_FOOD");
	static const int iBaseValueVeryLowFoodOnBonusProduction = GC.getDefineINT("SAS_EVALUATE_BASE_VALUE_HOME_ON_BONUS_PRODUCTION_VERY_LOW_FOOD");

	static const int iMinOnBonusCommerceImproveWorth = GC.getDefineINT("SAS_EVALUATE_VALUE_HOME_ON_BONUS_COMMERCE_MIN_IMPROVE_WORTH");
	static const int iBaseValueOnBonusCommerce = GC.getDefineINT("SAS_EVALUATE_VALUE_HOME_ON_BONUS_COMMERCE");
	static const int iValueVeryLowFoodOnBonusCommerce = GC.getDefineINT("SAS_EVALUATE_VALUE_HOME_ON_BONUS_COMMERCE_VERY_LOW_FOOD");
	static const int iBaseValueVeryLowFoodOnBonusCommerce = GC.getDefineINT("SAS_EVALUATE_BASE_VALUE_HOME_ON_BONUS_COMMERCE_VERY_LOW_FOOD");

	static const int iValueHomeWaterBonusNoCoast = GC.getDefineINT("SAS_EVALUATE_VALUE_HOME_WATER_BONUS_NO_COAST");
	static const int iStartingHomeFreshWaterValue = GC.getDefineINT("SAS_EVALUATE_STARTING_HOME_FRESH_WATER_VALUE");
	static const int iStartingFoodBonusValuePercent = GC.getDefineINT("SAS_EVALUATE_STARTING_FOOD_BONUS_VALUE_PERCENT");
	static const int iStartingRiverTileValue = GC.getDefineINT("SAS_EVALUATE_STARTING_RIVER_TILE_VALUE");
	static const int iStartingNonFoodBonusValuePercent = GC.getDefineINT("SAS_EVALUATE_STARTING_NON_FOOD_BONUS_VALUE_PERCENT");
	static const int iBFCBonusImprovementFoodValue = GC.getDefineINT("SAS_EVALUATE_BFC_BONUS_IMPROVEMENT_FOOD_VALUE");
	static const int iBFCBonusImprovementProductionValue = GC.getDefineINT("SAS_EVALUATE_BFC_BONUS_IMPROVEMENT_PRODUCTION_VALUE");
	static const int iBFCBonusImprovementCommerceValue = GC.getDefineINT("SAS_EVALUATE_BFC_BONUS_IMPROVEMENT_COMMERCE_VALUE");

	static const int iExtraValueNotHomeAIObjectiveBonusExtraValueMultiplier = GC.getDefineINT("SAS_EVALUATE_EXTRA_VALUE_NOT_HOME_BONUS_IAIOBJECTIVE_MULTIPLIER");
	static const int iUnownedBonusExtraValue = GC.getDefineINT("SAS_EVALUATE_UNOWNED_BONUS_EXTRA_VALUE");

	FOR_EACH_ENUM(CityPlot)
	{
		// <!-- custom: refactor below and adding our new logics -->
		// <advc.031>
		// CvPlot const* pLoopPlot = plotCity(iX, iY, eLoopCityPlot);
		// if (pLoopPlot != NULL && pLoopPlot->isGoody() &&
		// 	eLoopCityPlot != CITY_HOME_PLOT) // advc.027
		// {
		// 	iGoody++;
		// }

		CvPlot const* pLoopPlot = plotCity(iX, iY, eLoopCityPlot);

		if (pLoopPlot != NULL && pLoopPlot->isGoody() && eLoopCityPlot != CITY_HOME_PLOT) // advc.027)
			iGoody++;

		bool bShare = false;
		bool bSteal = false; // </advc.031>
		bool bCityRadius = false;
		bool bForeignOwned = false;
		if (!isUsablePlot(eLoopCityPlot, iTakenTiles, bCityRadius, bForeignOwned,
			bAnyForeignOwned, bShare, bSteal))
		{
			continue;
		}
		CvPlot const& p = *pLoopPlot;
		bool const bHome = isHome(p);

		// <!-- custom: Preliminary BFC quality counters previously ran before isUsablePlot and could affect first-city acceptance or later penalties through plots that the main evaluator then rejected as reserved or unavailable overlap.
		// Classify only plots that passed the established usability contract; fog remains represented separately by iUnrevealedTiles. See KI#490. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if (!bHome)
		{
			BonusTypes const eBonusPlot = getBonus(p);
			bool const bCanAssumeWaterBonusImprovement = p.SAS_canAssumeWaterBonusImprovement(eBonusPlot, ePlayer, kPlot);
			int iBestPotentialYieldScore = 0;
			bool const bVeryBadBFCPlot = p.SAS_isVeryBadBFCPlot(eBonusPlot, ePlayer, iAssumedSeaPlotFoodChange, iMinAcceptableVeryBadPlotPotentialYieldScore, iBestPotentialYieldScore);
			if (bVeryBadBFCPlot)
			{
				++iVeryBadBFCTiles;
				IFLOG logBBAI("Very bad BFC plot (%d,%d): %s, potential yield score %d below %d",
					p.getX(), p.getY(), (p.isImpassable() ? "impassable" : "weak despite improvements"), iBestPotentialYieldScore,
					iMinAcceptableVeryBadPlotPotentialYieldScore);
			}
			else iLowFoodLocationScore += p.SAS_getLowFoodEnvironmentScore(eBonusPlot, iAssumedSeaPlotFoodChange, bCanAssumeWaterBonusImprovement);
			if (bStartPhase && p.SAS_isGoodEnoughFirstCityBFCPlot(eBonusPlot, ePlayer, kPlot, bVeryBadBFCPlot))
				++iGoodEnoughFirstCityBFCTiles;
		}

		// advc.035: The own-exclusive-radius rule only helps if the radii don't overlap
		bool const bOwnExcl = (bGlobalsOwnExclusiveRadius && !bCityRadius && bForeignOwned);

		bool bRemovableFeature = false; // K-Mod
		bool bPersistentFeature = false; // advc: was "eventuallyRemovable" in K-Mod
		int iFeatureProduction = 0; // advc.031

		FeatureTypes const eFeature = p.getFeatureType();

		bRemovableFeature = isRemovableFeature(p, bPersistentFeature,
					/* <advc.031> */ iFeatureProduction);
		bool const bCanNeverImprove = (eFeature != NO_FEATURE &&
				GC.getInfo(eFeature).isNoImprovement());
		/*  (Non-resource water also can't be improved; that's already covered by K-Mod code in evaluateYield) </advc.031> */
		if (!bHome)
		{
			FAssert(eFeature != NO_FEATURE || (!bRemovableFeature && !bPersistentFeature));
			int const iFeatureHealth = (eFeature == NO_FEATURE ? 0 :
					GC.getInfo(eFeature).getHealthPercent());
			if (!bRemovableFeature || iFeatureHealth > 0)
				iHealth += iFeatureHealth;
		}
		BonusTypes const eBonus = getBonus(p);
		// <advc.031> (This was much coarser in K-Mod)
		bool bCanTradeBonus = false;
		bool bCanSoonTradeBonus = false;
		bool bCanImproveBonus = false;
		bool bCanSoonImproveBonus = false;
		int aiBonusImprovementYield[NUM_YIELD_TYPES] = {0, 0, 0};
		ImprovementTypes eBonusImprovement = NO_IMPROVEMENT;
		{
			bool bImprovementRemovesFeature = false;
			eBonusImprovement = getBonusImprovement(eBonus, p,
					bCanTradeBonus, bCanSoonTradeBonus, aiBonusImprovementYield,
					bCanImproveBonus, bCanSoonImproveBonus, bImprovementRemovesFeature);
			if (!bImprovementRemovesFeature && eBonusImprovement != NO_IMPROVEMENT)
				iFeatureProduction = 0;
		} // </advc.031>

		int iCultureModifier = 100;
		if (!bHome) // advc.031: Shouldn't matter on home plot
		{
			iCultureModifier = calculateCultureModifier(
					p, bForeignOwned, bShare, bCityRadius, bSteal,
					abFlip[eLoopCityPlot], bOwnExcl, iTakenTiles, iStealPercent);
			FAssertBounds(0, 101, iCultureModifier);
		}  // <advc.031>
		iFeatureProduction = (iFeatureProduction * iCultureModifier) / 100;
		iTotalFeatureProduction += iFeatureProduction; // </advc.031>
		// K-Mod note. This used to be called iTempValue.
		int iPlotValue = 0;
		// advc: Renamed from aiYield for clarity. Improvement yields are only predicted for resource plots (aiSpecialYield).
		int aiNatureYield[NUM_YIELD_TYPES];
		FOR_EACH_ENUM(Yield)
		{
			aiNatureYield[eLoopYield] = //p->getYield(eLoopYield);  // K-Mod replacement:
					p.calculateNatureYield(eLoopYield,
					/*	advc.031: NO_TEAM makes calculateNatureYield ignore the resource (whereas CvPlot::getBonusType treats NO_TEAM as all-revealing) */
					eBonus == NO_BONUS ? NO_TEAM : eTeam,
					!bPersistentFeature || bHome); // advc.031: City removes all features
			// <advc.031> Replacing deleted K-Mod code
			if (bHome)
			{
				aiNatureYield[eLoopYield] += p.calculateCityPlotYieldChange(
						eLoopYield, aiNatureYield[eLoopYield], 1);
			} // </advc.031>
		}
		if (bHome)
		{
			rBaseProduction += aiNatureYield[YIELD_PRODUCTION];
			iSpecialFoodPlus += std::max(0, aiNatureYield[YIELD_FOOD] -
					GC.getFOOD_CONSUMPTION_PER_POPULATION());

			// <!-- custom: add home plot health valorization of fresh water, as it seems to be missing; or if not missing somehow then not strongly enough applied, as AIs blatantly avoid in range fresh water home plot that is arguably better, most likely to not seeing its value if i may say in this case, so opening its eyes or trying to without going overboard and in this case i mean too -->
			if (!p.isWater())
			{
				const int iHomeWaterValue = ((p.isRiver() || p.isFreshWater()) ? iExtraValueHomeFreshWaterRiver : 0) + (kSet.isStartingLoc() && p.isFreshWater() ? iStartingHomeFreshWaterValue : 0);
				iPlotValue += iHomeWaterValue;
				if (m_pszBreakdown != NULL)
					iBreakdownHomeWater += 2 * iHomeWaterValue;
				if (kSet.isStartingLoc() && p.isFreshWater())
				{
					IFLOG if(iStartingHomeFreshWaterValue!=0) logBBAI("%d from starting home fresh water", iStartingHomeFreshWaterValue);
				}
			}
		}
		else
		{
			if (!bPersistentFeature && eFeature != NO_FEATURE)
			{
				iPlotValue += removableFeatureYieldVal(eFeature,
						bRemovableFeature, eBonus != NO_BONUS);
			}
			// <advc.031>
			if (!kSet.isStartingLoc()) // Don't factor traits into starting sites
			{
				FOR_EACH_ENUM(Yield)
				{

					{
						int iThresh = kPlayer.getExtraYieldThreshold(eLoopYield);
						if (iThresh > 0 &&
							aiNatureYield[eLoopYield] >= iThresh)
						{
							aiNatureYield[eLoopYield] += iGlobalsExtraYield;
						}
					}
					// <advc.908a>
					{
						int iThresh = kPlayer.getExtraYieldNaturalThreshold(eLoopYield);
						if (iThresh > 0 &&
							aiNatureYield[eLoopYield] + 1 >= iThresh)
						{
							aiNatureYield[eLoopYield] += iGlobalsExtraYield;
						}
					} // </advc.908a>
				}
			} // </advc.031>
			// K-Mod. add non-home plot production to BaseProduction.
			if (!bShare)
				rBaseProduction += aiNatureYield[YIELD_PRODUCTION];
			if (!bSteal)
				rBaseProduction += estimateImprovementProduction(p);
			if (!p.isWater())
			{
				// <!-- custom: this is most likely a lie or an omission, i checked other functions and this one for fresh water calls or such, no mention of this for home plot, so added ourselves -->
				// freshwater health (home plot) will be counted later
				iPlotValue += evaluateFreshWater(p, aiNatureYield, bSteal, iRiverTiles, iGreenTiles);
			}
		}
		int const iNatureYieldValue = evaluateYield(aiNatureYield, &p, bCanNeverImprove); // (K-Mod: iTempValue in BtS)
		ImprovementTypes eBestPotentialImprovement = NO_IMPROVEMENT;
		int aiBestPotentialYield[NUM_YIELD_TYPES] = {0, 0, 0};
		int iPotentialTimingPercent = 0;
		// <!-- custom: Ordinary water has no Build/Improvement outcome to enumerate, and its value depends on whether this candidate city is coastal.
		// Retain the already computed contextual nature value instead of scanning XML or placing a candidate-specific water value in the plot-intrinsic land cache. (GPT-5.6-Sol) -->
		int const iBestPotentialYieldValue = (eBonus == NO_BONUS && !p.isWater() ? evaluateBestPotentialPlotYield(p, bCanNeverImprove, eBestPotentialImprovement, aiBestPotentialYield, iPotentialTimingPercent) : iNatureYieldValue);
		// <!-- custom: Resource improvements retain their established strategic/non-yield and dedicated yield valuation for now.
		// For ordinary plots, replace the named terrain/feature bonuses with the strongest XML-valid improvement outcome, including improvements that retain their feature and a discounted share of later-tech or upgrade-chain value.
		// Old SAS added a separate terrain adjustment on top of the inherited yield score: e.g. Flood Plains +100, Grassland Hill +80, flat Grassland +65, Plains Hill +15, flat Plains -20 and Desert Hill -25.
		// Current-rule diagnostic examples instead produced complete ordinary-plot values of flat Desert 0, Desert Hill 45, flat Plains 83, flat Grassland 134, Forest Grassland 144 and River Grassland 172.
		// Those new totals are contextual and will change with XML, player modifiers and improvement timing; they are not direct replacements for the old additive constants, and the strongest plots receive further core weighting when the BFC is summed. (GPT-5.6-Sol) -->
		iPlotValue += (eBonus == NO_BONUS && !bHome ? std::max(iNatureYieldValue, iBestPotentialYieldValue) : iNatureYieldValue);
		IFLOG if(eBonus == NO_BONUS) logBBAI("PLOT_POTENTIAL plot=%d,%d home=%d natureValue=%d potentialValue=%d improvement=%S potentialYields=%dF%dP%dC timing=%d%%",
			p.getX(), p.getY(), bHome, iNatureYieldValue, iBestPotentialYieldValue,
			(eBestPotentialImprovement == NO_IMPROVEMENT ? L"-" : GC.getInfo(eBestPotentialImprovement).getDescription()),
			aiBestPotentialYield[YIELD_FOOD], aiBestPotentialYield[YIELD_PRODUCTION], aiBestPotentialYield[YIELD_COMMERCE], iPotentialTimingPercent);
		// <!-- custom: note: bHome means it is the tile where we'll plant our city, which chatgpt 5 confirmed too as the "home plot" but check to be sure -->
		if (bHome)
		{
			// <!-- custom: Old SAS introduced a named terrain/feature table after repeatedly observing Base AdvCiv settle on productive Grassland, Grassland Hills or Flood Plains even when a nearby weak Desert or other low-Food plot could host the city without consuming a valuable workable plot.
			// That table substantially reduced the behavior, but its empirical values condensed free city-center output, lost worked-improvement potential, resource loss and defensive preference into one XML-specific adjustment that became brittle when yields or terrain rules changed.
			// Keep those concerns separate in the generic model.
			// The actual XML city-center output below is free every turn: it needs no citizen or Food allocation, receives the minimum city yields, retains eligible natural/resource yields and is therefore counted twice for immediate availability.
			// Founding permanently consumes the plot's worked-improvement potential; the resource and ordinary-plot branches below measure that opportunity separately.
			// Inherent post-founding terrain defense is evaluated later and never changes either yield comparison.
			// This lets e.g. a low-Food Plains Hill gain free city-center Food/Production while preserving a productive Grassland Hill for a citizen, without naming either terrain and while adapting automatically to XML/modmod changes. (GPT-5.6-Sol) -->
			// <advc.031> Count home plot yield twice b/c it's immediately available
			iPlotValue *= 2;
			iValue += iPlotValue;
			// <!-- custom: removed foundOnResourceValue, now code added directly here; and this is the place where we can tell AI to not settle on bonuses and how so; also merging the bonus has improvement vs no improvement logic at the common part of penalty calculation and using aiNatureYield instead for it rather than aiBonusImprovementYield as in old code as advised by chatgpt 5, splitting it only later at the finer calculation (with fixp and such quite similarly to how it was in the old code); also nature yield shouldn't be that much to begin with, what we want most is to count improved yields and the base +1 are quite minor, if they are hard or tricky to take into account at least for me and i'd rather avoid it, we have our yields here xd -->
			// if (eBonus != NO_BONUS)
			// {
			// 	// Replacing K-Mod code that had adjusted the home plot yield
			// 	iValue += foundOnResourceValue( // (result normally negative)
			// 			eBonusImprovement == NO_IMPROVEMENT ? NULL : aiBonusImprovementYield);
			// } // </advc.031>
			/*	<advc.031> A plot next to a resource will usually have a higher found value
			than the resource plot itself because of the improvement yields counted by
			evaluateSpecialYields. But not always - it depends on what else is in the
			city radius. Founding on a resource usually cannibalizes other potential
			city sites, so it needs to be discouraged a bit. */
			if (eBonus != NO_BONUS)
			{
				// <!-- custom: Resource home plots retain a separate policy: city-center scoring above rewards eligible natural bonus yields kept for free, while this branch discourages losing the bonus-specific improvement yields and relaxes Production/Commerce penalties on intrinsically zero-Food plots.
				// This preserves the useful old SAS distinction without also applying the ordinary-plot opportunity comparison and double-counting the loss. (GPT-5.6-Sol) -->
				// <!-- custom: The old bonus-settlement exception named Snow, Desert, Flood Plains and Grass Hills.
				// Underlying natural Food <= 0 expresses the intended opportunity-cost distinction directly and remains correct when terrain/feature XML changes. (GPT-5.6-Sol) -->
				const bool bVeryLowFoodBonusPlot = (p.calculateNatureYield(YIELD_FOOD, NO_TEAM) <= 0);

				// <!-- custom: was -5 for some reason in old code, chatgpt 5 used 0 so going with it -->
				//int r = -5;
				int r = 0;

				// <!-- custom: new code by chatgpt 5 using our bonusspecific helpers, which i adjusted a bit, check if accurate -->
				// 1) What improvement would we build on this bonus? (tech-agnostic)
				ImprovementTypes const eBonusSpecificImprovement = p.SAS_getBonusSpecificImprovement(eBonus);

				// 2) Bonus-specific improved yields (no city-center bump, no base imp extras)
				// <!-- custom: may be more reliable than aiBonusImprovementYield just in case it does some weird stuff as we had issues with AI settling on camel desert or sometimes before deer tundra when penalties were not high enough (and even if very high for desert camel), so trying to rework and see if we can fix the issue -->
				int fImp = 0, hImp = 0, cImp = 0;
				if (eBonusSpecificImprovement != NO_IMPROVEMENT)
				{
					// <!-- custom: note to chatgpt 5 or other AIs or such anywyas etc: GC.getImprovementInfo doesn't seem to exist in .cpp files only .py files, while we have plenty instances of using code like `CvImprovementInfo const& kImprovement = GC.getInfo(eImprovement);` in the mod's code, code that i did not add myself as well, so use this rather -->
					// You’re right — in your C++ DLL (AdvCiv/AdvCiv-SAS) the correct accessor is the templated:
					// C++: GC.getInfo(ImprovementTypes) → CvImprovementInfo&
					// Python: gc.getImprovementInfo(i) → Python wrapper
					CvImprovementInfo const& impInfo = GC.getInfo(eBonusSpecificImprovement);
					fImp = impInfo.getImprovementBonusYield(eBonus, YIELD_FOOD);
					hImp = impInfo.getImprovementBonusYield(eBonus, YIELD_PRODUCTION);
					cImp = impInfo.getImprovementBonusYield(eBonus, YIELD_COMMERCE);

					// <!-- custom: use this to detect food bonuses, as according to chatgpt 5 nature yield is not reliable in accurately assessing a tile's yield of a bonus is indeed food bonus or not something else like the city base's tile yield from little i understood of it, check if accurate; also note: for food any food is extremely valuable, even if a commerce bonus that happens to give some improved food, value it much still as recommended by chatgpt 5 and what i understood got the idea to do of it too i mean -->
					if (fImp >= iMinOnBonusFoodImproveWorth)
					{
						// <!-- custom: it seems we sometimes still found on deer tundra in autoplay, try to increase the penalty further, while trying not to increase it too much in case it is locally best to found as such, 400 may seem high but even 300 was not enough although it fluctuated a bit before (all this is with the old formula, not this new one below as untested with old file to know) staying there, i could try 350 maybe but since 400 does fine keep as is for possible edge cases, hopefully this doesn't prevent locally good spots where settling on food is ideal, but even if then, statistically should be better for ai to avoid settling on food bonuses.
						// update: the other new issue i had of ai settling on camel desert is now averted to much later with our new change to improved yield rather than nature ones, but although ai doesn't settle at turn 50, it considers it at turn 100 when other sites are taken, nearby home plots should be much more attractive, so i guess we are not valuing it enough or something, testing and see hopefully not causing ai to chosoe worse sites just to avoid settling on bonus too much but even if it were to happen that may still be better in most cases although not ideal than settling on food bonuses, ideally i would dig why this happens or such and fix this more cleanly, but hopefully this helps (since issue only happens on desert camel, maybe base desert food yield being so low interferes or some other logics conflicts somewhere else? These are just guesses, could be mistaken or not, check if accurate) -->
						int const iOnBonusFoodPenalty = iBaseValueOnBonusFood * fImp;
						// <!-- custom: be careful to not mix up as noted by chatgpt 5, we add the cumulative penalty here (a positive number if we penalize, a negative number if we for some reason don't penalize but valorize weirdly instead), then later at the end we subract to iValue this positive penalty number so add penalty to r, do not substract -->
						r += iOnBonusFoodPenalty;
						IFLOG logBBAI("%d penalty for founding on food bonus", iOnBonusFoodPenalty);
					}
					// <!-- custom: use combinatory rather than else if in case some weird bonuses have a mix of several effects (e.g. quite high hammer and quite high commerce overall but each itself is just a bit above average not so good yet when combined gold + hammer is very nice for example, then we want to take both into account in such cases) -->
					if (hImp >= iMinOnBonusProductionImproveWorth)
					{
						int iOnBonusProductionPenalty = iBaseValueOnBonusProduction;

						// <!-- custom: but if bonus is very low-food, and yield is not too high, this is attractive, especially on a hill, but the higher the yields, the more we'd rather hve low food and high yields than settle on the bonus to save food -->
						if (bVeryLowFoodBonusPlot)
						{
							iOnBonusProductionPenalty += iValueVeryLowFoodOnBonusProduction;
							iOnBonusProductionPenalty += iBaseValueVeryLowFoodOnBonusProduction * hImp;
						}

						// <!-- custom: be careful to not mix up as noted by chatgpt 5, we add the cumulative penalty here (a positive number if we penalize, a negative number if we for some reason don't penalize but valorize weirdly instead), then later at the end we subract to iValue this positive penalty number so add penalty to r, do not substract -->
						r += iOnBonusProductionPenalty;
						IFLOG logBBAI("%d penalty for founding on production bonus", iOnBonusProductionPenalty);
					}
					if (cImp >= iMinOnBonusCommerceImproveWorth)
					{
						int iOnBonusCommercePenalty = iBaseValueOnBonusCommerce;

						// <!-- custom: but if bonus is very low-food, and yield is not too high, this is attractive, especially on a hill; note: although a bit redundant, it gives flexibility to tweak values -->
						if (bVeryLowFoodBonusPlot)
						{
							iOnBonusCommercePenalty += iValueVeryLowFoodOnBonusCommerce;
							iOnBonusCommercePenalty += iBaseValueVeryLowFoodOnBonusCommerce * cImp;
						}

						// <!-- custom: be careful to not mix up as noted by chatgpt 5, we add the cumulative penalty here (a positive number if we penalize, a negative number if we for some reason don't penalize but valorize weirdly instead), then later at the end we subract to iValue this positive penalty number so add penalty to r, do not substract -->
						r += iOnBonusCommercePenalty;
						IFLOG logBBAI("%d penalty for founding on commerce bonus", iOnBonusCommercePenalty);
					}
				}
				else
				{
					IFLOG logBBAI("Warning: no mapped improvement for bonus %S", GC.getInfo(eBonus).getDescription());
				}

				// <!-- custom: we now fetch bonusspecific improvement directly from our ideal map, regardless of if available or not for better or worse, but assume AI can always improve bonuses so it doesn't discard/dismiss those it can't improve yet so disable this (can probably remove but check if accurate) -->
				// // <!-- custom: improvement specific logic here only, as in old code of the now merged here and removed thereAIFoundValue::foundOnResourceValue -->
				// // Improvement-specific penalty for lost yield potential
				// if (eBonusImprovement != NO_IMPROVEMENT)
				// {
				// 	// <!-- custom: code comment below added by chatgpt, i don't know if accurate but maybe is, seems so from quick glance and reflection on it from me but i don't know too much about these so check if accurate; note: also some logic added as of now in AIFoundValue::AIFoundValue -->
				// 	// General yield penalty for lost improvement potential
				// 	int const iImprovementYieldValue = evaluateYield(aiBonusImprovementYield);
				// 	if (iImprovementYieldValue > 0) // Make sure not to exponentiate a negative value
				// 		r -= (scaled(iImprovementYieldValue).pow(fixp(1.5)) / fixp(4.2)).round();
				// }

				// /*	In (historical) scenarios, resources are sometimes placed just so that the AI
				// 	doesn't settle in a particular tile. Try -a little bit- to respect that. */
				// if (kGame.isScenario())
				// 	r -= 13;
				// iValue -= r;
				// IFLOG logBBAI("Penalty (substracted to iValue) %d for founding on bonus", r);
				// <!-- custom: note: opposite sign now, adding a negative value is clearer than guessing if we're substracting or not -->
				iValue += r;
				IFLOG logBBAI("Penalty (added to iValue) %d for founding on bonus", r); // adds a negative -> penalty ✅
			}
			// <!-- custom: Founding consumes this plot's future worked-tile potential; the first generic replacement compared two nonlinear evaluateYield totals: with current XML weights, a 2F1P city center scored 120 but a valuable 1F3P Grassland Hill Mine only 105, so consuming the strong Hill could be rewarded instead of discouraged.
			// Compare the actual city-center yields with the timing-adjusted best XML-valid improvement under one consistent set of marginal yield weights.
			// This preserves the old SAS home/non-home table's economic intent without terrain names: weak Plains/Desert Hills gain free Food and sometimes Production as city centers, while productive Grassland Hills, Flood Plains and growing improvements are better preserved for citizens; XML yield, improvement, feature or city-minimum changes automatically alter the result.
			// Old SAS applied fixed home adjustments including Flood Plains -150, Grassland Hill -75, flat Grassland -50, Plains Hill +250, flat Plains +50 and flat or hilly barren Desert +50.
			// Under the new opportunity comparison, current-rule examples instead produce River Grassland Hill -40, ordinary flat Grassland +4 and barren flat Desert/Ice Sheet +52. A strong plot is likely to belong to the early worked core, so apply its permanent loss at the same doubled scale as the home plot.
			// Conversely, removing a weak marginal plot helps only if the city would eventually have worked it, while its free city-center output was already counted twice above; provisional 40% represents roughly eight of twenty non-home BFC plots.
			// The Aachen test at (137,72) demonstrates why the home adjustment must remain one component rather than force settling on the weakest tile: the neighboring Oil Desert at (136,72), hidden and therefore evaluated as ordinary barren Desert, gained +52 instead of the Grassland home's +4 and brought Cattle into the BFC instead of Sugar.
			// It was 16 points stronger raw (5408 vs 5392) and gained another +38 from its slightly stronger core.
			// The old local-recheck travel charge nevertheless subtracted 75 because the adjacent route reported pathTurns=1 and made Aachen consume the Grassland; the fixed replay moved to the Desert and still founded on turn 0, so this did not represent a lost capital turn.
			// Removing that contradictory charge lets the strictly two-turn local recheck select the better complete site while the broader scouting logic still limits long delays. See KI#144. (GPT-5.6-Sol) -->
			else
			{
				static const int iFoodValue = GC.getDefineINT("SAS_EVALUATE_NATURE_YIELD_SELF_SUSTAINING_FOOD_VALUE");
				static const int iProductionValue = GC.getDefineINT("SAS_EVALUATE_NATURE_YIELD_SELF_SUSTAINING_PRODUCTION_VALUE");
				static const int iCommerceValue = GC.getDefineINT("SAS_EVALUATE_NATURE_YIELD_SELF_SUSTAINING_COMMERCE_VALUE");
				int const aiYieldValue[NUM_YIELD_TYPES] = {
					(iFoodValue * getSASEvaluateYieldValuePercent(YIELD_FOOD)) / 100,
					(iProductionValue * getSASEvaluateYieldValuePercent(YIELD_PRODUCTION)) / 100,
					(iCommerceValue * getSASEvaluateYieldValuePercent(YIELD_COMMERCE)) / 100};
				int iCityCenterYieldValue = 0;
				int iWorkedPotentialYieldValue = 0;
				FOR_EACH_ENUM(Yield)
				{
					int const iUnimprovedYield = p.calculateNatureYield(eLoopYield, NO_TEAM);
					iCityCenterYieldValue += aiNatureYield[eLoopYield] * aiYieldValue[eLoopYield];
					iWorkedPotentialYieldValue += iUnimprovedYield * aiYieldValue[eLoopYield] + ((aiBestPotentialYield[eLoopYield] - iUnimprovedYield) * aiYieldValue[eLoopYield] * iPotentialTimingPercent) / 100;
				}
				int const iHomeOpportunityDelta = iCityCenterYieldValue - iWorkedPotentialYieldValue;
				static const int iStrongPlotOpportunityValuePercent = std::max(0, GC.getDefineINT("SAS_EVALUATE_HOME_STRONG_PLOT_OPPORTUNITY_VALUE_PERCENT"));
				static const int iWeakPlotOpportunityValuePercent = std::max(0, GC.getDefineINT("SAS_EVALUATE_HOME_WEAK_PLOT_OPPORTUNITY_VALUE_PERCENT"));
				int const iHomeOpportunityValue = (iHomeOpportunityDelta < 0 ? (iHomeOpportunityDelta * iStrongPlotOpportunityValuePercent) / 100 : (iHomeOpportunityDelta * iWeakPlotOpportunityValuePercent) / 100);
				iValue += iHomeOpportunityValue;
				IFLOG logBBAI("%d home-plot opportunity value: delta=%d strongPlotPercent=%d weakPlotPercent=%d cityCenter=%d cityCenterYields=%dF%dP%dC workedPotential=%d improvement=%S potentialYields=%dF%dP%dC timing=%d%%",
					iHomeOpportunityValue, iHomeOpportunityDelta, iStrongPlotOpportunityValuePercent, iWeakPlotOpportunityValuePercent, iCityCenterYieldValue,
					aiNatureYield[YIELD_FOOD], aiNatureYield[YIELD_PRODUCTION], aiNatureYield[YIELD_COMMERCE],
					iWorkedPotentialYieldValue,
					(eBestPotentialImprovement == NO_IMPROVEMENT ? L"-" : GC.getInfo(eBestPotentialImprovement).getDescription()),
					aiBestPotentialYield[YIELD_FOOD], aiBestPotentialYield[YIELD_PRODUCTION], aiBestPotentialYield[YIELD_COMMERCE], iPotentialTimingPercent);
			}
		}
		else
		{
			iPlotValue = applyCultureModifier(p, iPlotValue, iCultureModifier, bShare);
			aiPlotValues[eLoopCityPlot] = iPlotValue; // Sum these up later
		}
		bool const bEasyAccess = /* K-Mod (!!): */ ((p.isWater() && (bCoastal ||
				p.getArea().getCitiesPerPlayer(ePlayer, true) > 0)) || // advc.031
				p.sameArea(kPlot) || p.getArea().getCitiesPerPlayer(ePlayer) > 0);
		IFLOG logPlot(p, iPlotValue, aiNatureYield, iCultureModifier, eBonus, eBonusImprovement,
				bCanTradeBonus, bCanSoonTradeBonus, bCanImproveBonus, bCanSoonImproveBonus,
				bEasyAccess, iFeatureProduction, bPersistentFeature, bRemovableFeature);
		// <advc.031>
		if (bShare)
		{
			IFLOG if(eBonus!=NO_BONUS) logBBAI("Resource ignored (shared tile)");
			continue;
		}
		// Was 33 flat; the modifier is now computed differently.
		if (iCultureModifier <= (bSteal ? 40 : 20) &&// </advc.031>
			!bOwnExcl) // advc.035
		{
			IFLOG if(eBonus!=NO_BONUS) logBBAI("Resource ignored (unlikely to flip)");
			continue;
		}

		// <!-- custom: Keep these diagnostic locals explicitly zero-initialized.
		// Guarding both initialization and reads with bLogBonusScore still triggered VC++ 2003 C4701 under /WX; eight trivial zero stores are preferable to fragile compiler-flow micro-optimization. (GPT-5.6-Sol) -->
		int iBonusScoreHappyHealth = 0;
		int iBonusScoreBuildingHappyHealth = 0;
		int iBonusScoreAdjustPercent = 0;
		int iBonusScoreWaterPenalty = 0;
		int iBonusScoreNonYield = 0;
		int iBonusScoreDiversity = 0;
		int iBonusScoreAIObjective = 0;
		int iBonusScoreYield = 0;
		bool const bLogBonusScore = (eBonus != NO_BONUS && gFoundLogLevel > 0 && AIFoundValue::isLoggingEnabled());
		if (eBonus != NO_BONUS) // advc.040: Same-area checks moved into nonYieldBonusValue
		{
			// <!-- custom: For the first city, food bonuses and rivers snowball earlier than non-food bonuses because growth unlocks more tiles, whipping/specialists, and faster worker/settler development. Apply capital-only bonus value percents and concrete river/fresh-water boosts so Karakorum/Beijing-like starts prefer nearby corn/fresh-water positions over slower commerce/hammer-bonus positions. (GPT-5.5) -->
			// <!-- custom: Raw XML food is not enough to classify a starting bonus as food: AdvCiv-SAS Elephants have XML food but Camp adds no food, and seafood inside an inland BFC is not usable like coastal seafood. For capital-only food-bonus scaling, use normal improved food on land and XML food only for water bonuses when the candidate city is coastal. This keeps AI_foundValue from treating inland fish or low-food bonuses like Pig + Corn. (GPT-5.5) -->
			const bool bStartingFoodBonus = (kSet.isStartingLoc() && (p.isWater() ? bCoastal && GC.getInfo(eBonus).getYieldChange(YIELD_FOOD) > 0 : aiBonusImprovementYield[YIELD_FOOD] > 0));

			// <!-- custom: Base AdvCiv skipped barbarian non-yield bonus value because barbarians do not care about ordinary bonus trade. AdvCiv-SAS still wants barbarian-founded cities to be good long-term sites, both for barbarian strength and because they are often captured by normal players, so value health/happiness/building effects while ignoring barbarian trade-timing limits. See KI#178. (GPT-5.5) -->
			// if (!bBarbarian && // advc.303: Barbarians don't care about bonus trade
			// 	// advc.031: Otherwise we can already trade the bonus
			if (getRevealedOwner(p) != ePlayer)
			{
				int* const piBonusScoreHappyHealth = (bLogBonusScore ? &iBonusScoreHappyHealth : NULL);
				int* const piBonusScoreBuildingHappyHealth = (bLogBonusScore ? &iBonusScoreBuildingHappyHealth : NULL);
				int* const piBonusScoreAdjustPercent = (bLogBonusScore ? &iBonusScoreAdjustPercent : NULL);
				int* const piBonusScoreWaterPenalty = (bLogBonusScore ? &iBonusScoreWaterPenalty : NULL);
				int iBonusValue = nonYieldBonusValue(p, eBonus, bBarbarian ? true : bCanTradeBonus, bEasyAccess, bCoastal, &aiBonusCount, iCultureModifier, piBonusScoreHappyHealth, piBonusScoreBuildingHappyHealth, piBonusScoreAdjustPercent, piBonusScoreWaterPenalty);
				if (kSet.isStartingLoc())
				{
					const int iStartingBonusValuePercent = (bStartingFoodBonus ? iStartingFoodBonusValuePercent : iStartingNonFoodBonusValuePercent);
					const int iOldBonusValue = iBonusValue;
					iBonusValue = (iBonusValue * iStartingBonusValuePercent) / 100;
					IFLOG if(iBonusValue!=iOldBonusValue) logBBAI("Starting %S bonus value x%d%%: %d -> %d (%S)",
						bStartingFoodBonus ? L"food" : L"non-food", iStartingBonusValuePercent, iOldBonusValue, iBonusValue,
						GC.getInfo(eBonus).getDescription());
				}
				IFLOG if(iBonusValue!=0) logBBAI("+%d non-yield bonus value (%S)", iBonusValue, GC.getInfo(eBonus).getDescription());
				//iValue += (iBonusValue + 10);
				iResourceValue += iBonusValue; // K-Mod
				iBonusScoreNonYield = iBonusValue;
			}

			// <!-- custom: Add a small flat city-site value for each distinct bonus in this BFC that the AI does not already own or plausibly cover through an existing city's future BFC. Local bonus tile yields are already valued normally.
			// This only nudges empire resource diversity, e.g. prefer new Maize over extra Wheat, or Whales over another Crab (e.g., allows to gain +1 health +1 with Granary again with a new Maize rather than no effect even with 2 new Wheat we already own, so a new Maize is more valuable).
			// Trade imports do not count as owned here because they are not permanent. Apply it once per bonus type and only after the first city, so it does not disturb starting-location food/river/coast tuning. See KI#147. (ChatGPT-5.5) -->
			if (iUnownedBonusExtraValue != 0 && !kSet.isStartingLoc() && !kSet.isNormalizing() &&
					!abSASUnownedBonusExtraApplied[eBonus])
			{
				abSASUnownedBonusExtraApplied[eBonus] = true;
				bool const bAlreadyOwnedBonus = isBonusOwnedOrClaimedByFutureBFC(eBonus);
				if (!bAlreadyOwnedBonus)
				{
					iResourceValue += iUnownedBonusExtraValue;
					IFLOG logBBAI("+%d unowned bonus diversity value (%S)", iUnownedBonusExtraValue, GC.getInfo(eBonus).getDescription());
					iBonusScoreDiversity = iUnownedBonusExtraValue;
				}
				else IFLOG logBBAI("Unowned bonus diversity skipped; already owned or claimed by future BFC (%S)", GC.getInfo(eBonus).getDescription());
			}

			/*if (p.isWater())
				iValue += (bCoastal ? 0 : -800);*/ // (was ? 100 : -800)
			// <advc.031> Replacing the above
			if (p.isWater() && !bCoastal)
			{
				//int const iPenalty = 165;
				// IFLOG logBBAI("-%d from water resource near non-coastal site", iPenalty);
				// iValue -= iPenalty;
				// <!-- custom: note: opposite sign here (addition vs substraction before) unlike in base advciv, so value as of now also has opposite sign in global defines -->
				iValue += iValueHomeWaterBonusNoCoast;
				IFLOG logBBAI("%d from water resource near non-coastal site", iValueHomeWaterBonusNoCoast);
			} // </advc.031>
		}
		// <!-- custom: Accumulate cautious health for removable non-home features by XML property rather than naming Forest/Jungle.
		// The city removes its home feature; persistent BFC features such as Flood Plains remain represented by their plot yield instead of receiving a second health penalty here. See KI#488. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if (!bHome && eFeature != NO_FEATURE && !bPersistentFeature)
			iCautiousHealthPercent += GC.getInfo(eFeature).getHealthPercent();

		if (kSet.isStartingLoc() && !bHome && !bSteal && p.isRiver())
		{
			iStartingRiverTiles++;
			iValue += iStartingRiverTileValue;
			if (m_pszBreakdown != NULL)
				iBreakdownRiverBFC += iStartingRiverTileValue;
		}

		if (!bHome) // (Home plot was handled upfront)
		{
			if (eBonus != NO_BONUS)
			{
				// <!-- custom: value iAIObjective bonuses especially more to settle on first, could be critical for our life and death (ideally check if obsolete too). Not having copper or iron may make us die, so really value it more -->
				const int iAIObjectiveBonus = GC.getBonusInfo(eBonus).getAIObjective(); // e.g. 10 for iron

				iBonusScoreAIObjective = iExtraValueNotHomeAIObjectiveBonusExtraValueMultiplier * iAIObjectiveBonus;
				iValue += iBonusScoreAIObjective;

				// <!-- custom: AdvCiv's aggregated special-yield formula applied hardcoded weights, nonlinear power, and a second food modifier after older SAS logic had already weighted Food x3 and Production x2. This made six resources contribute 5015 points to Karakorum's low-food (49,43) candidate and overwhelm the stronger Pig + Maize river-grass site at (52,40) (save file 360).
				// Value only the actual improvement yield changes once through simple XML-tunable Food/Production/Commerce values; non-yield health, happiness, strategic, duplicate, and trade value remains separate. (GPT-5.5) -->
				const int iBonusImprovementYieldValue = (aiBonusImprovementYield[YIELD_FOOD] * iBFCBonusImprovementFoodValue * getSASEvaluateYieldValuePercent(YIELD_FOOD)) / 100 + (aiBonusImprovementYield[YIELD_PRODUCTION] * iBFCBonusImprovementProductionValue * getSASEvaluateYieldValuePercent(YIELD_PRODUCTION)) / 100 + (aiBonusImprovementYield[YIELD_COMMERCE] * iBFCBonusImprovementCommerceValue * getSASEvaluateYieldValuePercent(YIELD_COMMERCE)) / 100;
				iResourceValue += iBonusImprovementYieldValue;
				if (m_pszBreakdown != NULL)
					iBreakdownBonusImprovementYields += iBonusImprovementYieldValue;
				iBonusScoreYield = iBonusImprovementYieldValue;
				IFLOG if(iBonusImprovementYieldValue != 0) logBBAI("%d from tunable bonus improvement yields %dF%dP%dC (%S)",
					iBonusImprovementYieldValue, aiBonusImprovementYield[YIELD_FOOD], aiBonusImprovementYield[YIELD_PRODUCTION],
					aiBonusImprovementYield[YIELD_COMMERCE], GC.getInfo(eBonus).getDescription());
			}

			if (bLogBonusScore)
				logBBAI("BONUS_SCORE plot=%d,%d bonus=%S happyHealth=%d buildingHappyHealth=%d adjustPercent=%d waterPenalty=%d nonYield=%d diversity=%d aiObjective=%d bonusYield=%d total=%d",
					p.getX(), p.getY(), GC.getInfo(eBonus).getDescription(), iBonusScoreHappyHealth, iBonusScoreBuildingHappyHealth,
					iBonusScoreAdjustPercent, iBonusScoreWaterPenalty, iBonusScoreNonYield, iBonusScoreDiversity, iBonusScoreAIObjective,
					iBonusScoreYield, iBonusScoreNonYield + iBonusScoreDiversity + iBonusScoreAIObjective + iBonusScoreYield);

			int iSpecialYieldModifier = calculateSpecialYieldModifier(iCultureModifier, bEasyAccess, eBonus != NO_BONUS, bCanSoonImproveBonus, bCanImproveBonus);
			calculateSpecialYields(p, eBonusImprovement == NO_IMPROVEMENT ? NULL : aiBonusImprovementYield, aiNatureYield, iSpecialYieldModifier, aiSpecialYield, iSpecialFoodPlus, iSpecialFoodMinus, iSpecialYieldTiles);
		}
	}

	if (m_pszBreakdown != NULL)
		iBreakdownDirect = iValue - iBreakdownBase;
	bool const bNeedPlotDistribution = (m_pszBreakdown != NULL || m_paiGrowthCorePlotValues != NULL || m_paiPlotCoreSums != NULL);
	iBreakdownPlots = sumUpPlotValues(aiPlotValues, (bNeedPlotDistribution ? aiBreakdownPlotCoreSums : NULL),
		(bNeedPlotDistribution ? aiBreakdownPlotCoreCutoffs : NULL), (bNeedPlotDistribution ? &iBreakdownPositivePlots : NULL));
	if (m_paiGrowthCorePlotValues != NULL)
	{
		for (int iCore = 0; iCore < 2; ++iCore)
			m_paiGrowthCorePlotValues[iCore] = aiBreakdownPlotCoreSums[iCore + 3];
	}
	if (m_paiPlotCoreSums != NULL)
	{
		FAssert(m_paiPlotCoreCutoffs != NULL && m_piPositivePlots != NULL);
		for (int iCore = 0; iCore < 6; ++iCore)
		{
			m_paiPlotCoreSums[iCore] = aiBreakdownPlotCoreSums[iCore];
			m_paiPlotCoreCutoffs[iCore] = aiBreakdownPlotCoreCutoffs[iCore];
		}
		*m_piPositivePlots = iBreakdownPositivePlots;
	}
	if (m_piSustainableProductivePlotValue != NULL)
		*m_piSustainableProductivePlotValue = CitySiteEvaluator::getSustainableProductivePlotValue();
	iValue += iBreakdownPlots;
	// A sensible order (CITY_HOME_PLOT first) isn't guaranteed anymore, hence:
	aiPlotValues.clear();
	IFLOG if(kSet.isStartingLoc() && iStartingRiverTiles > 0 && iStartingRiverTileValue != 0)
		logBBAI("+%d from %d starting river BFC tiles", iStartingRiverTiles * iStartingRiverTileValue, iStartingRiverTiles);
	// <!-- custom: disabled as part of the change there as well where this variable is used -->
	// advc.031: Preserve this for later
	// int iNonYieldResourceVal = std::max(0, iResourceValue);
	// <!-- custom: Replaced AdvCiv's opaque aggregated special-yield score with the linear XML-tunable per-bonus calculation above. Keep the old call commented for a visible reference while calculateSpecialYields still supplies food/production context used by other established logic below. (GPT-5.5) -->
	// if (m_pszBreakdown != NULL)
	// 	iBreakdownNonYieldResources = iResourceValue;
	// if (iSpecialYieldTiles > 0) // advc.031
	// {
	// 	iBreakdownSpecialYields = evaluateSpecialYields(aiSpecialYield, iSpecialYieldTiles, iSpecialFoodPlus, iSpecialFoodMinus);
	// 	iResourceValue += iBreakdownSpecialYields;
	// }
	if (m_pszBreakdown != NULL)
		iBreakdownNonYieldResources = iResourceValue - iBreakdownBonusImprovementYields;

	// <!-- custom: simplify logic and attempt to spread cities more, currently they are way too crowded which is inefficient -->
	// if (isTooManyTakenTiles(iTakenTiles, iResourceValue, iValue < 780))
	if (isTooManyTakenTiles(iTakenTiles, iResourceValue))
	{
		IFLOG logBBAI("Too many taken tiles (%d)", iTakenTiles);
		return 0;
	}
	iBreakdownResourcesAdded = std::max(0, iResourceValue);
	iValue += iBreakdownResourcesAdded;

	iBreakdownHealth = evaluateLongTermHealth(iHealth); // (may increase iHealth)
	iValue += iBreakdownHealth;
	iBreakdownFeatureProduction = evaluateFeatureProduction(iTotalFeatureProduction); // advc.031
	iValue += iBreakdownFeatureProduction;

	if (bCoastal)
	{
		iBreakdownSea = evaluateSeaAccess(
				// advc: Same conditions as in BtS ...
				kArea.getCitiesPerPlayer(ePlayer) == 0 &&
				!bAnyForeignOwned && iResourceValue > 0 && kSet.isSeafaring(),
				// <advc.031>
				fixp(0.7) * scaled::clamp(
				scaled::min(rBaseProduction + aiSpecialYield[YIELD_PRODUCTION],
				// Probably can't work high-production tiles when there is no food
				scaled(4 * (1 + iGreenTiles + iSpecialFoodPlus), 18)), fixp(0.1), 2),
				iLandTiles); // </advc.031>
		iValue += iBreakdownSea;
	}

	// <!-- custom: discourage non-coastal location for low-food terrain environments: better settle on coast and make profit than starve soon by settling in non-coastal -->
	else
	{
		static const int iMaxToleratedNotCoastalLowFoodScore = GC.getDefineINT("SAS_EVALUATE_MAX_TOLERATED_NOT_COASTAL_LOW_FOOD_SCORE");
		static const int iBaseValueNotCoastalLowFood = GC.getDefineINT("SAS_EVALUATE_BASE_VALUE_NOT_COASTAL_LOW_FOOD");

		if (iLowFoodLocationScore > iMaxToleratedNotCoastalLowFoodScore)
		{
			// <!-- custom: old value was sometimes still insufficient even though it helped in some cities; increase low food location penalty more, which successfully solved the issue on testing. See known issue for details. (Claude code Sonnet 4.5 (summarized)) -->
			// iValue += (-50) * iLowFoodLocationScore;
			iBreakdownLowFood = iBaseValueNotCoastalLowFood * iLowFoodLocationScore;
			iValue += iBreakdownLowFood;
		}
	}

	// <!-- custom: Penalize impassable or persistently low-yield BFC slots after a small allowance. Dynamic XML-property/yield scoring replaces the old Peak/Ice/flat Desert/flat Snow name list, so terrain and improvement changes automatically affect this classification. Visible bonuses on usable plots stay exempt because their specialized value is evaluated separately. See KI#26.2. (GPT-5.5) -->
	if (bStartPhase)
	{
		// <!-- custom: regardless of coastal status, tolerate some low number of non land non bonus tiles, but past a certain threshold, strongly penalize it so AIs wouldn't take this city site unless nothing at all is better; also for our first city we don't want to be too close to the coast anyway and more like closer to the center for more radial expansion or possibilities, lower distance-to-capital maintenance cost, stronger early position for later expansions, etc if any other advantages, and if the spot is good, keep it for city 2 not for starting one where we really want nice yields on land and a closer to center position for later expansion; as for value we have 20 bfc tiles (minus home plot) if i'm not mistaken, so if it has more than 8 water tiles no bonus, or equivalent, so fewer than the required good-enough first-city BFC tiles changes/rejects the site (ChatGPT-5.5) -->

		static const int iMinRequiredGoodEnoughFirstCityBFCTiles = GC.getDefineINT("SAS_EVALUATE_MIN_REQUIRED_GOOD_ENOUGH_FIRST_CITY_BFC_TILES");

		if (iGoodEnoughFirstCityBFCTiles + iUnrevealedTiles < iMinRequiredGoodEnoughFirstCityBFCTiles)
		{
			// <!-- custom: note: AIFoundValue now stores and returns int, so the old short-overflow workaround is obsolete so removed it (as for previous overflow issue, see KI#44/Moscow). (ChatGPT-5.5) -->
			IFLOG logBBAI("Site rejected: first-city BFC has %d known good-enough plots + %d unrevealed = %d, below required %d",
				iGoodEnoughFirstCityBFCTiles, iUnrevealedTiles, iGoodEnoughFirstCityBFCTiles + iUnrevealedTiles,
				iMinRequiredGoodEnoughFirstCityBFCTiles);
			return 0;
		}
		const int iExcessVeryBadTiles = std::max(0, iVeryBadBFCTiles - iMaxToleratedVeryBadTilesStart);
		if (iExcessVeryBadTiles > 0)
		{
			// <!-- custom: The allowance is free rather than a cliff that retroactively penalizes every bad slot. In the Berlin test (save file 442), alternative (33,13) had Rice, Cattle, Silver, four grass hills, and four peaks; charging all four peaks favored fresh-water (36,9) despite ten tundra tiles. Penalize only the excess. (GPT-5.5) -->
			const int iTotalValueVeryBadTiles = iBaseValueVeryBadTileStart * iExcessVeryBadTiles;
			iValue += iTotalValueVeryBadTiles;
			iBreakdownVeryBad = iTotalValueVeryBadTiles;
			IFLOG logBBAI("%d from %d very bad BFC plots beyond starting allowance %d", iTotalValueVeryBadTiles, iVeryBadBFCTiles, iMaxToleratedVeryBadTilesStart);
		}
	}
	else
	{
		const int iExcessVeryBadTiles = std::max(0, iVeryBadBFCTiles - iMaxToleratedVeryBadTilesLater);
		if (iExcessVeryBadTiles > 0)
		{
			const int iTotalValueVeryBadTiles = iBaseValueVeryBadTileLater * iExcessVeryBadTiles;
			iValue += iTotalValueVeryBadTiles;
			iBreakdownVeryBad = iTotalValueVeryBadTiles;
			IFLOG logBBAI("%d from %d very bad BFC plots beyond later-city allowance %d", iTotalValueVeryBadTiles, iVeryBadBFCTiles, iMaxToleratedVeryBadTilesLater);
		}
	}

	// <!-- custom: SAS's former named home/non-home terrain values substantially reduced the observed AdvCiv-era tendency to found on valuable workable plots such as Grassland Hills; that direct table already encoded both yield opportunity and defensible-terrain preferences, so SAS disabled AdvCiv's separate fixed Hill-only defense bonus as potentially interfering rather than as a separately proven cause.
	// After replacing the terrain table with generic XML-valid yield potential, restore defense generically too: XML terrain defense plus the global hill defense modifies only the value of the productive plots that remain around the city.
	// Resources, coast, fresh water and the consumed home plot therefore cannot enlarge the defense adjustment merely because the founding terrain is defensible. (GPT-5.6-Sol) -->
	int const iBreakdownDefense = evaluateDefense(iBreakdownPlots);
	iValue += iBreakdownDefense;

	// <!-- custom: try to remove this interference as we have a finer algorithm now, and this old code may lead to unexpected results -->
	// if (!kSet.isStartingLoc() /* advc.031f: */ || kSet.isScenario())
	// {
	// 	/*  <advc.031> Moved down to the other modifiers. But don't adjust the
	// 		non-yield resource value -- don't need food for a trade connection. */
	// 	iValue = adjustToFood(iValue - iNonYieldResourceVal, iSpecialFoodPlus,
	// 			iSpecialFoodMinus, iGreenTiles) + iNonYieldResourceVal;
	// 	// </advc.031>
	// }

	rBaseProduction += aiSpecialYield[YIELD_PRODUCTION]; // K-Mod
	// <!-- custom: Level-2 compact breakdowns report this checkpoint as preModifiers, superseding the old disabled standalone "total before modifiers" IFLOG line without duplicating output. (GPT-5.6-Sol) -->
	const int iBreakdownPreModifiers = iValue;
	int iBreakdownNothingSpecial = 0;
	int iBreakdownHomeResource = 0;
	int iBreakdownLandBoundary = 0;
	int iBreakdownStartingSurroundings = 0;
	int iBreakdownDistance = 0;
	int iBreakdownNearestOwnCityX = -1;
	int iBreakdownNearestOwnCityY = -1;
	int iBreakdownNearestOwnCityDistance = -1;
	int iBreakdownDistanceMinOk = -1;
	int iBreakdownDistanceMaxOk = -1;
	int iBreakdownDistancePercent = 100;
	int iBreakdownCulture = 0;
	int iBreakdownCitiesPerArea = 0;
	int iBreakdownBonusCount = 0;
	int iBreakdownBadHealth = 0;
	int iBreakdownGoodies = 0;
	int iBreakdownNavalHeavy = 0;

	// <!-- custom: see code comment there for details -->
	// iValue = adjustToProduction(iValue, rBaseProduction);

	// <advc.031>
	const int iBeforeNothingSpecial = iValue;
	if (iResourceValue <= 0 && iSpecialFoodPlus <= 0 &&
		iUnrevealedTiles < 5) // advc.040
	{
		scaled rMultiplier = per100(60);
		if (iRiverTiles >= 4)
			rMultiplier = scaled::min(1, rMultiplier + per100(6) * iRiverTiles);
		iValue = (rMultiplier * iValue).round();
		IFLOG if(rMultiplier!=1) logBBAI("Times %d percent because the site offers nothing special", rMultiplier.getPercent());
	} // </advc.031>
	iBreakdownNothingSpecial = iValue - iBeforeNothingSpecial;
	/*  advc.108: Obsoletion check added. Probably better not to let players start on a hidden resource; i.e. don't check this->getBonus(kPlot) != NO_BONUS. */
	const int iBeforeHomeResource = iValue;
	if ((kSet.isDiagnosticOmniscience() ? kPlot.getBonusType() : kPlot.getNonObsoleteBonusType(eTeam)) != NO_BONUS)
	{
		int iModifier = 100;
		if (kSet.isStartingLoc())
			iModifier = 50;
		else // advc.031: I don't think both penalties should be applied
			if (kSet.isAdvancedStart())
		{
			iModifier = 70;
		}
		iValue *= iModifier;
		iValue /= 100;
		IFLOG if(iModifier!=100) logBBAI("Times %d percent for starting on a resource", iModifier);
	}
	iBreakdownHomeResource = iValue - iBeforeHomeResource;
	const int iBeforeLandBoundary = iValue;
	if (kSet.isStartingLoc())
		iValue = adjustToLandAreaBoundary(iValue);
	iBreakdownLandBoundary = iValue - iBeforeLandBoundary;
	const int iBeforeStartingSurroundings = iValue;
	static const bool bRuntimeStartingSurroundings = GC.getDefineBOOL("SAS_AI_FOUND_FIRST_CITY_RUNTIME_STARTING_SURROUNDINGS_ADJUSTMENT_ENABLE");
	// <!-- custom: adjustToStartingSurroundings uses range-6 terrain and every assigned starting plot to spread civilizations during all-seeing map generation/normalization, so isAllSeeing keeps that use enabled.
	// Once play begins, the live first settler should choose its best revealed site even if that moves nearer to or farther from a neighbor; only the opt-in SAS define restores this modifier at runtime. In the Berlin test (save file 442), (33,13) led (35,11) by 4569 to 3934 beforehand, but its -1482 adjustment reversed the choice. See KI#173. (GPT-5.5) -->
	if ((kSet.isStartingLoc() || /* advc.031e: */ kSet.isNormalizing()) && ((kSet.isAllSeeing() && !kSet.isDiagnosticOmniscience()) || bRuntimeStartingSurroundings))
	{	// <advc.027
		if (kSet.isIgnoreStartingSurroundings())
			iValue = adjustToStartingChoices(iValue);
		else // </advc.027>
			iValue = adjustToStartingSurroundings(iValue); // (advc: Moved down a bit)
	}
	iBreakdownStartingSurroundings = iValue - iBeforeStartingSurroundings;
	// <!-- custom: see code comment there for details -->
	// if (bBarbarian)
	// 	iValue = adjustToBarbarianSurroundings(iValue);
	// else if (!kSet.isStartingLoc() /* advc.031e: */ && !kSet.isNormalizing())

	// <!-- custom: through trial and error, while trying to find why we settle on camel desert in middle game (turns 50+, while having saner choices earlier in map view ingame (circled tiles), i have found that commenting the block below causes the issue to be solved, AI has sane sites as always and now settles around or near this but no AI player considers settling on camel desert or near it anymore, so i assume something is majorly faulty in it or didn't accommodate/account for food desert bonuses or such. Since i don't like interferences, commented out since i have found it to be reproducible that uncommenting it triggers again the error, replaced with a very simplified version of the logic we want, inline in this function that is its only caller, with the help of chatgpt 5, check if accurate, see known issue as of now 54 for details -->
	// if (!kSet.isStartingLoc() /* advc.031e: */ && !kSet.isNormalizing())
	// 	iValue = adjustToCivSurroundings(iValue, iStealPercent);
	const int iBeforeDistance = iValue;
	int iBeforeCulture = iValue;
	if (!kSet.isStartingLoc() && !kSet.isNormalizing())
	{
		if (!bBarbarian)
		{
			// <!-- custom: This is normal-civ empire distance shaping. Do not apply it to Barbarian city creation: Barbarians are spawned by a global map scan, and using distance from existing Barbarian cities hard-rejected better one-tile shifts such as 70,37 near Zapotec despite good local city value. (GPT-5.5) -->
			// <!-- custom: 1) distance to nearest city penalties (as per our previous refactor/change but enhanced/simplified thanks to chatgpt 5 too) -->
			// --- SAS inline distance shaping (simple & bounded) ---
			// C++03-safe, integer math only, overflow-safe via percent scaling.
			// Notes
			// Setting FAR_CAP = 100 + the early return 0; gives you the hard “nope" at extreme distances.
			// I left the “too close" branch clamped at 50% so proximity never kills a locally amazing site.
			// This keeps your logic simple, deterministic, and aligned with your “better no city than a split empire" philosophy.
			// <!-- custom: added const* for efficiency but then chatgpt 5 said to add another one xd, check if accurate and thanks -->
			// const on the left (CvCity const*) means you won’t mutate the city via this pointer.
			// const on the right (* const) means you won’t reassign pNearest.
			CvCity const* const pNearest = GC.getMap().findCity(iX, iY, ePlayer, eTeam, false);
			if (pNearest != NULL)
			{
				const int iDistRaw = ::plotDistance(iX, iY, pNearest->getX(), pNearest->getY());
				iBreakdownNearestOwnCityX = pNearest->getX();
				iBreakdownNearestOwnCityY = pNearest->getY();
				iBreakdownNearestOwnCityDistance = iDistRaw;

				// Derive from city radius so mods changing radius stay sane.
				static const int CITYR_ADD = GC.getDefineINT("SAS_EVALUATE_DISTANCE_CITYR_ADD");
				static const int CITYR_DIVIDE = std::max(1, GC.getDefineINT("SAS_EVALUATE_DISTANCE_CITYR_DIVIDE"));
				static const int CITYR   = (CITYR_ADD + CITY_PLOTS_DIAMETER) / CITYR_DIVIDE; // default 5 <!-- custom: or so claims chatgpt.. 5.. could be accurate though i really don't know, its code helped me lot in all cases thanks-->
				static const int MIN_OK_ADD = GC.getDefineINT("SAS_EVALUATE_DISTANCE_MIN_OK_ADD");
				static const int MAX_OK_ADD = GC.getDefineINT("SAS_EVALUATE_DISTANCE_MAX_OK_ADD");
				static const int MIN_OK  = CITYR + MIN_OK_ADD; // ≤ MIN_OK-1 = “too close"
				static const int MAX_OK  = CITYR + MAX_OK_ADD; // ≥ MAX_OK+1 = “too far"
				iBreakdownDistanceMinOk = MIN_OK;
				iBreakdownDistanceMaxOk = MAX_OK;

				static const int CLOSE_STEP = GC.getDefineINT("SAS_EVALUATE_DISTANCE_CLOSE_STEP"); // % per tile inside MIN_OK
				static const int FAR_STEP   = GC.getDefineINT("SAS_EVALUATE_DISTANCE_FAR_STEP"); // % per tile beyond MAX_OK
				static const int CLOSE_CAP  = GC.getDefineINT("SAS_EVALUATE_DISTANCE_CLOSE_CAP"); // max close penalty
				static const int FAR_CAP    = GC.getDefineINT("SAS_EVALUATE_DISTANCE_FAR_CAP"); // allow hard kill

				int mult = 100; // 100% = no change

				// Too close
				if (iDistRaw <= MIN_OK - 1)
				{
					int pct = (MIN_OK - iDistRaw) * CLOSE_STEP;
					if (pct > CLOSE_CAP) pct = CLOSE_CAP;
					mult -= pct;
				}
				// Too far
				else if (iDistRaw >= MAX_OK + 1)
				{
					int pct = (iDistRaw - MAX_OK) * FAR_STEP;
					if (pct > FAR_CAP) pct = FAR_CAP;

					// Hard cutoff for extreme stretch
					if (pct >= 100)
					{
						IFLOG logBBAI("Dist shaping: d=%d -> hard reject near %S", iDistRaw, cityName(*pNearest));
						return 0;
					}
					mult -= pct;
				}
				iBreakdownDistancePercent = mult;

				const int oldVal = iValue;
				iValue = (iValue * mult) / 100;
				if (iValue < 0) iValue = 0;
				IFLOG if (iValue != oldVal) logBBAI("Dist shaping: d=%d -> x%d%% (%d→%d) near %S", iDistRaw, mult, oldVal, iValue, cityName(*pNearest));
			}
			// --- end SAS distance shaping ---
		}
		iBreakdownDistance = iValue - iBeforeDistance;
		iBeforeCulture = iValue;

		// <!-- custom: 2) -->
		// --- SAS: super-simple culture pressure gate (no cheat, cheap) ---
		// Goal: it's fine if a site is a *little* pressed, but skip it if it's *too* pressed.
		// Notes
		// - No-cheat preserved: foreign cities only counted if KNOWN_TO and AI_deduceCitySite says we could infer them.
		// - Cheap: one pass, integer math, no squares or fixed-point.
		// - Behavior:
		// 		- A little culture nearby? Ignore it.
		// 		- Medium pressure? Keep the site but down-weight it.
		// 		- Heavy pressure (e.g., right next to a big foreign border)? Return 0 so it drops out.
		int iMaxPressure = 0; // we just look at the worst single nearby foreign city

		for (PlayerIter<CIV_ALIVE, KNOWN_TO> it(eTeam); it.hasNext(); ++it)
		{
			const CvPlayer& kLoop = *it;
			if (kLoop.getTeam() == eTeam) continue;
			if (GET_TEAM(kLoop.getTeam()).isVassal(eTeam)) continue;

			FOR_EACH_CITY(pCity, kLoop)
			{
				// don't cheat: only count cities we could reasonably know about
				if (!kSet.isAllSeeing() && !kTeam.AI_deduceCitySite(*pCity))
					continue;

				// <!-- custom: Culture can cross area boundaries through the narrower culture-level + 1 range. Use that real engine range for another-island cities instead of discarding their pressure, while retaining the inherited +3 same-area scale. See KI#487. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				const int d = plotDistance(iX, iY, pCity->getX(), pCity->getY());
				bool const bSameArea = pCity->isArea(kArea);
				const int r = pCity->getCultureLevel() + (bSameArea ? CvCity::plotCultureExtraRange() : 1);
				if (!bSameArea && d > r)
					continue;

				int iLocal = 0;
				if (d <= r)
				{
					// Linear 10..100 scale: edge of culture ring ~10, adjacent to city ~100.
					// (Cheap & monotone; no divisions by zero.)
					iLocal = 10 + (90 * (r - d)) / (r > 0 ? r : 1);
				}
				else if (d == r + 1)
				{
					// slight pressure just outside the ring
					iLocal = 5;
				}

				if (iLocal > iMaxPressure) iMaxPressure = iLocal;
			}
		}

		// Tuning: "soft" tolerance vs "hard" refusal
		static const int SOFT = GC.getDefineINT("SAS_EVALUATE_CULTURE_PRESSURE_SOFT"); // <35: ignore pressure
		static const int HARD = GC.getDefineINT("SAS_EVALUATE_CULTURE_PRESSURE_HARD"); // >=70: too pressed → reject site outright

		if (iMaxPressure >= HARD)
		{
			IFLOG logBBAI("Site rejected: culture pressure=%d (>= %d)", iMaxPressure, HARD);
			return 0; // too risky; likely to be crushed/flipped
		}
		else if (iMaxPressure >= SOFT)
		{
			// Mild penalty between SOFT..HARD. Clamp so we never nuke more than -70% here.
			int mult = 100 - ((iMaxPressure - SOFT) * 2); // linear fade
			if (mult < 30) mult = 30;
			int const iOld = iValue;
			iValue = (iValue * mult) / 100;
			IFLOG if(iValue!=iOld) logBBAI("Culture pressure %d -> x%d%% (%d→%d)", iMaxPressure, mult, iOld, iValue);
		}
		// --- end culture pressure gate ---
	}
	iBreakdownCulture = iValue - iBeforeCulture;

	const int iBeforeCitiesPerArea = iValue;
	iValue = adjustToCitiesPerArea(iValue);
	iBreakdownCitiesPerArea = iValue - iBeforeCitiesPerArea;

	const int iBeforeBonusCount = iValue;
	if (!kSet.isStartingLoc() /* advc.031e: */ && !kSet.isNormalizing())
		iValue = adjustToBonusCount(iValue, aiBonusCount);
	iBreakdownBonusCount = iValue - iBeforeBonusCount;

	// <!-- custom: try to remove this interference as we have a finer algorithm now, and this old code may lead to unexpected results -->
	// iValue = adjustToBadTiles(iValue, iBadTiles /* advc.031: */ + (4 * iTakenTiles) / 10
	// 		-(bBarbarian ? 2 : (4 + iGreenTiles + iSpecialFoodPlus)) // advc.303
	// 		/ (kSet.isStartingLoc() && !kSet.isScenario() ? 2 : 1)); // advc.108, advc.031f

	// <!-- custom: this only matters for our first city that we want to grow fast, but when we have a lot of workers, not a big problem, especially if site is overall better long term potential, do not discard it then and bet on long term value rather and; logic is that as long we don't reach threshold all is as good as no health, but as soon as we reach it, all unhealthiness is bad and more the more unhealthy we are as explained to clarify to chapt 5; also be more lenient for later cities, we'd have workers by then to handle chopping or such, and capital would have grown fine -->
	// iValue = adjustToBadHealth(iValue, iHealth);
	// // <advc.031>
	// <!-- custom: our code instead-->
	// Bad-health dampener (pure integer math; no floats)
	if (bStartPhase)
	{
		// <!-- custom: for example as of now 7 jungle tiles is -50 * 7 = -350, or 8 jungle tiles and 2 forest tiles is (8 * (-50)) + ((2 * 50)) = -400 + 100 = -300 -->
		static const int UNHEALTH_THRESH = GC.getDefineINT("SAS_EVALUATE_MIN_UNHEALTH_THRESH_START");

		// Apply only if city-area health is bad enough
		// e.g. -350 <= -300 → apply
		if (iCautiousHealthPercent <= UNHEALTH_THRESH)
		{
			// RATE is "% penalty per -100 health-percent"
			// Example: CHP=-350, RATE=10 → penalty = (10 * 350)/100 = 35 (%)
			static const int UNHEALTH_RATE = GC.getDefineINT("SAS_EVALUATE_VALUE_LOSS_UNHEALTH_RATE_START");

			// -(-350) = 350
			const int unhealthAbs = -iCautiousHealthPercent;
			// (10 * 350) / 100 = 3500 / 100 = 35  ← 35%
			int penaltyPct = (UNHEALTH_RATE * unhealthAbs) / 100;
			// clamp: never >100%
			if (penaltyPct > 100) penaltyPct = 100;

			// 100 - 35 = 65 (%)
			const int mult = 100 - penaltyPct;
			const int old  = iValue;
			// iValue *= 65/100
			// Result: 35% penalty → x65%
			iValue = (iValue * mult) / 100;
			iBreakdownBadHealth = iValue - old;
			IFLOG if (iValue != old)
				logBBAI("Bad health %d%% (START) rate=%d -> -%d%% => x%d%% (%d→%d)",
						iCautiousHealthPercent, UNHEALTH_RATE, penaltyPct, mult, old, iValue);
		}
	}
	else
	{
		// Later cities: same dampener, but usually milder settings
		static const int UNHEALTH_THRESH = GC.getDefineINT("SAS_EVALUATE_MIN_UNHEALTH_THRESH_LATER"); // e.g. -450

		// Apply only if city-area health is bad enough (e.g. -460 <= -450 → apply)
		if (iCautiousHealthPercent <= UNHEALTH_THRESH)
		{
			// RATE is "% penalty per -100 health-percent"
			// Example: CHP=-450, RATE=5 → (5 * 450) / 100 = 2250 / 100 = 22 (% penalty, int trunc)
			static const int UNHEALTH_RATE = GC.getDefineINT("SAS_EVALUATE_VALUE_LOSS_UNHEALTH_RATE_LATER");

			const int unhealthAbs = -iCautiousHealthPercent;              // -(-450)=450
			int penaltyPct = (UNHEALTH_RATE * unhealthAbs) / 100;   // e.g. 22
			if (penaltyPct > 100) penaltyPct = 100;                       // cap at 100%

			const int mult = 100 - penaltyPct;                            // e.g. 78%
			const int old  = iValue;
			iValue = (iValue * mult) / 100;                               // x78%
			iBreakdownBadHealth = iValue - old;
			IFLOG if (iValue != old)
				logBBAI("Bad health %d%% (LATER) rate=%d -> -%d%% => x%d%% (%d→%d)",
						iCautiousHealthPercent, UNHEALTH_RATE, penaltyPct, mult, old, iValue);
		}
	}

	if (kSet.isNormalizing())
	{
		iBreakdownGoodies = evaluateGoodies(iGoody);
		iValue += iBreakdownGoodies; // </advc.031>
	}

	// <!-- custom: Optionally give coastal sites a flat naval-heavy-map bias in the central CitySiteEvaluator path, affecting cached found values and direct AI_foundValue calls without duplicating it elsewhere.
	// Following the KI#173 scoring changes, save file 420 still chose the strong Cuzco coastal site with both biases set to 0, while Japan no longer favored the old tundra-heavy candidates.
	// Keep this disabled or small if an extra strategic preference for coastal access, work boats, contact, trade, and galleys is wanted. See KI#146 (Update). (GPT-5.5) -->
	if (!kPlayer.isHuman() && !kSet.isNormalizing() && bCoastal &&
			kGame.isNavalHeavyMapnameCached() && !kGame.isLandHeavyMapnameCached())
	{
		static const int iNavalHeavyFirstCityCoastalExtraValue = GC.getDefineINT("SAS_EVALUATE_NAVAL_HEAVY_FIRST_CITY_COASTAL_EXTRA_VALUE");
		static const int iNavalHeavyLaterCoastalExtraValue = GC.getDefineINT("SAS_EVALUATE_NAVAL_HEAVY_OTHER_CITIES_COASTAL_EXTRA_VALUE");
		const int iNavalHeavyCoastalExtraValue = (kSet.isStartingLoc() ?
				iNavalHeavyFirstCityCoastalExtraValue : iNavalHeavyLaterCoastalExtraValue);
		if (iNavalHeavyCoastalExtraValue != 0)
		{
			iValue += iNavalHeavyCoastalExtraValue;
			iBreakdownNavalHeavy = iNavalHeavyCoastalExtraValue;
			IFLOG logBBAI("+%d naval-heavy coastal site value", iNavalHeavyCoastalExtraValue);
		}
	}

	// advc: BtS code (iDifferentAreaTile) deleted
	// (disabled by K-Mod. This kind of stuff is already taken into account.)
	if (m_pszBreakdown != NULL)
	{
		const int iBreakdownDirectOther = iBreakdownDirect - iBreakdownHomeWater - iBreakdownRiverBFC;
		const int iBreakdownModifiers = iValue - iBreakdownPreModifiers;
		// <!-- custom: A zero distance adjustment was ambiguous: it could mean no existing city or a candidate inside the neutral distance window.
		// Eight independent turn-100/150 tests across SAS48 Pangaea/Continents and Huge Archipelago/Pangaea applied distance reductions to 148 of 731 selected sites that had an existing own city; distance also reversed the selected/next site's pre-distance ordering in 103 of 727 paired comparisons.
		// Include the already-computed nearest-city coordinates, raw distance, neutral window, pre-distance value and applied percent so compact comparisons expose this material spacing decision without another map scan; diagnostic only. (GPT-5.6-Sol) -->
		*m_pszBreakdown = CvString::format("base=%d directOther=%d homeWater=%d riverBFC=%d plots=%d(core1=%d/%d,core2=%d/%d,core3=%d/%d,core6=%d/%d,core10=%d/%d,core14=%d/%d,positive=%d) bonuses=%d(nonYield=%d,bonusImprovementYields=%d) health=%d featureProduction=%d sea=%d defense=%d lowFood=%d veryBad=%d preModifiers=%d modifiers=%d(nothingSpecial=%d,homeResource=%d,landBoundary=%d,startingSurroundings=%d,distance=%d(before=%d,nearest=%d,%d,raw=%d,neutral=%d..%d,percent=%d),culture=%d,citiesPerArea=%d,bonusCount=%d,badHealth=%d,goodies=%d,navalHeavy=%d) final=%d",
				iBreakdownBase, iBreakdownDirectOther, iBreakdownHomeWater, iBreakdownRiverBFC, iBreakdownPlots,
				aiBreakdownPlotCoreSums[0], aiBreakdownPlotCoreCutoffs[0], aiBreakdownPlotCoreSums[1], aiBreakdownPlotCoreCutoffs[1],
				aiBreakdownPlotCoreSums[2], aiBreakdownPlotCoreCutoffs[2], aiBreakdownPlotCoreSums[3], aiBreakdownPlotCoreCutoffs[3],
				aiBreakdownPlotCoreSums[4], aiBreakdownPlotCoreCutoffs[4], aiBreakdownPlotCoreSums[5], aiBreakdownPlotCoreCutoffs[5], iBreakdownPositivePlots,
				iBreakdownResourcesAdded, iBreakdownNonYieldResources, iBreakdownBonusImprovementYields, iBreakdownHealth, iBreakdownFeatureProduction, iBreakdownSea, iBreakdownDefense, iBreakdownLowFood, iBreakdownVeryBad, iBreakdownPreModifiers, iBreakdownModifiers, iBreakdownNothingSpecial, iBreakdownHomeResource, iBreakdownLandBoundary, iBreakdownStartingSurroundings, iBreakdownDistance, iBeforeDistance,
				iBreakdownNearestOwnCityX, iBreakdownNearestOwnCityY, iBreakdownNearestOwnCityDistance, iBreakdownDistanceMinOk, iBreakdownDistanceMaxOk, iBreakdownDistancePercent,
				iBreakdownCulture, iBreakdownCitiesPerArea, iBreakdownBonusCount, iBreakdownBadHealth, iBreakdownGoodies, iBreakdownNavalHeavy, iValue);
	}

	return iValue;
}


bool AIFoundValue::isSiteValid() const
{
	//if (!kSet.isStartingLoc() && !kSet.isAdvancedStart())
	if(iCities > 0 && !bBarbarian) // advc.108
	{
		if (!bCoastal && iAreaCities == 0)
		{
			IFLOG logBBAI("First colony in area must be coastal");
			return false;
		}
	}

	int iMinRivalRange = kSet.getMinRivalRange();
	if (iMinRivalRange > 0)
	{
		for (SquareIter it(kPlot, iMinRivalRange); it.hasNext(); ++it)
		{
			if (it->plotCheck(PUF_isOtherTeam, ePlayer) != NULL)
			{
				IFLOG logBBAI("Rival plot (%d,%d) found in MinRivalRange", it->getX(), it->getY());
				return false;
			}
		}
	}
	if (kSet.isStartingLoc())
	{
		if (kPlot.isGoody() /* advc.027: */ && kSet.isScenario())
		{
			IFLOG logBBAI("Can't start on goody hut");
			return false;
		}
		FOR_EACH_ENUM(CityPlot)
		{
			CvPlot const* p = plotCity(iX, iY, eLoopCityPlot);
			if (p == NULL)
			{
				IFLOG logBBAI("Can't start near the edge of a flat map");
				return false;
			}
		}
	}
	else // advc.031: Not relevant for StartingLoc
	{
		int iOwnedTiles = 0;
		FOR_EACH_ENUM(CityPlot)
		{
			CvPlot const* p = plotCity(iX, iY, eLoopCityPlot);
			if (p == NULL)
				iOwnedTiles += 2;
			else if (getRevealedOwner(*p) != NO_PLAYER && getRevealedTeam(*p) != eTeam)
			{
				/*  advc.035 (comment): Would be good to check abFlip[i] here, but computeOverlap is a little costly. */
				iOwnedTiles++;
				/*  <advc.031> Count tiles only half if they're in our inner ring and can't be worked by any foreign city. */
				if (!adjacentOrSame(*p, kPlot) || p->isCityRadius())
					iOwnedTiles++; // </advc.031>
			}
		}
		//if(iOwnedTiles > NUM_CITY_PLOTS / 3)
		/*  <advc.031> Most owned tiles are counted twice now, and I want sth. closer to half of the tiles being owned. */
		int const iMaxOwnedTimes100 = 82 * NUM_CITY_PLOTS;
		if (100 * iOwnedTiles > iMaxOwnedTimes100) // </advc.031>
		{
			IFLOG logBBAI("%d tiles owned by other teams; allowed (times 100): %d", iOwnedTiles, iMaxOwnedTimes100);
			return false;
		}
	}
	return true;
}

// Returns false when there is too much overlap
bool AIFoundValue::computeOverlap()
{
	/*	K-Mod explanation of city site adjustment:
		Any plot which is otherwise within the radius of a city site is basically
		treated as if it's within an existing city radius */
	/*  advc.031: Changed to int in order to store the site id
		(only used for logging currently though) */
	aiCitySiteRadius.resize(NUM_CITY_PLOTS, -1);
	/*  <advc.035>
		Need to distinguish tiles within the radius of one of our team's cities
		from those within just any city radius. */
	abOwnCityRadius.resize(NUM_CITY_PLOTS, false);
	// Whether the tile flips to us once we settle near it
	abFlip.resize(NUM_CITY_PLOTS, false); // </advc.035>
	// K-Mod. bug fixes etc. (original code deleted)
	if (!kSet.isStartingLoc() /* advc.031e: */ && !kSet.isNormalizing() &&
		!kSet.isDebug() && // advc.007
		!bBarbarian) // advc.303: Barbarians don't have city sites
	{
		// <!-- custom: no need to make the cached CvGlobals defines static const according to chatgpt 5, but hoist them as recommended by chatgpt 5, check if accurate -->
		// <!-- custom: code/performance optimization: hoist -->
		// Hoist once per call; no need for static
		const int iGlobalsMinCityRange = GC.getDefineINT(CvGlobals::MIN_CITY_RANGE);

		for (int iSite = 0; iSite < kPlayer.AI_getNumCitySites(); iSite++)
		{
			CvPlot const& kCitySitePlot = kPlayer.AI_getCitySite(iSite);
			if (&kCitySitePlot == &kPlot)
				continue;
			if (plotDistance(&kPlot, &kCitySitePlot) <=
				iGlobalsMinCityRange &&
				kCitySitePlot.sameArea(kPlot))
			{
				IFLOG logBBAI("Too close to one of the sites we've already chosen");
				return false;
			}
			for (CityPlotIter it(kPlot); it.hasNext(); ++it)
			{
				if (plotDistance(&(*it), &kCitySitePlot) <= CITY_PLOTS_RADIUS)
				{
					//Plot is inside the radius of a city site
					aiCitySiteRadius[it.currID()] = iSite;
				}
			}
		}
	} // K-Mod (bugfixes etc.) end
	// <advc.035> (also for advc.031)
	if (!kSet.isStartingLoc() && !kSet.isNormalizing() && !bBarbarian)
	{
		FOR_EACH_CITY(c, kPlayer)
		{
			FOR_EACH_ENUM(CityPlot)
			{
				CvPlot const* p = plotCity(iX, iY, eLoopCityPlot);
				if(p != NULL && plotDistance(p->getX(), p->getY(),
					c->getX(), c->getY()) <= CITY_PLOTS_RADIUS)
				{
					abOwnCityRadius[eLoopCityPlot] = true;
				}
			}
		}
		if (GC.getDefineBOOL(CvGlobals::OWN_EXCLUSIVE_RADIUS))
		{
			FOR_EACH_ENUM(CityPlot)
			{
				CvPlot const* p = plotCity(iX, iY, eLoopCityPlot);
				abFlip[eLoopCityPlot] = (!abOwnCityRadius[eLoopCityPlot] &&
						p != NULL && getRevealedOwner(*p) != NO_PLAYER &&
						p->isCityRadius() && getRevealedTeam(*p) != eTeam &&
						(p->getSecondOwner() == ePlayer ||
						/*  The above is enough b/c the tile may not be within the
							culture range of one of our cities; but it will be once
							the new city is founded. */
						p->findHighestCulturePlayer(true) == ePlayer));
			}
		} // </advc.035>
	}
	return true;
}

// <advc.040>
bool AIFoundValue::isPrioritizeAsFirstColony() const
{

	return (bCoastal && iAreaCities <= 0 && !bBarbarian && iCities > 0 &&
			(kPlot.isFreshWater() ||
			/*  Don't prioritize colonies in tundra, snow and desert b/c
				these are likely surrounded by more (unrevealed) bad terrain. */
			kPlot.calculateNatureYield(YIELD_FOOD, eTeam, true) +
			kPlot.calculateNatureYield(YIELD_PRODUCTION, eTeam, true) > 1));
} // </advc.040>


int AIFoundValue::countBadTiles(/* advc.031: */ int& iInnerRadius, int& iUnrevealed, /* advc.031: */ int& iLand, int& iRevealedDecentLand) const // advc.040
{
	int iBadTiles = 0;
	FOR_EACH_ENUM(CityPlot)
	{
		CvPlot const* p = plotCity(iX, iY, eLoopCityPlot);
		// <advc.031>
		bool const bInner = (eLoopCityPlot < NUM_INNER_PLOTS);
		/*  NULL and impassable count as 2 bad tiles (as in BtS),
			but don't cheat with unrevealed tiles. */
		if (p == NULL)
		{
			if (bInner)
				iInnerRadius += 2;
			iBadTiles += 2;
			continue;
		}
		if (!isRevealed(*p))
		{
			iUnrevealed++; // advc.040
			if (bInner)
				iInnerRadius++;
			continue;
		}
		if (isHome(*p))
			continue;
		// <advc.303>
		static const bool bSAS_EVALUATE_BARBARIAN_FULL_BFC_ENABLE = GC.getDefineBOOL("SAS_EVALUATE_BARBARIAN_FULL_BFC_ENABLE");
		if (bBarbarian && !bSAS_EVALUATE_BARBARIAN_FULL_BFC_ENABLE && !adjacentOrSame(*p, kPlot))
		{
			// Rational Barbarians wouldn't mind settling one off the coast, but human players do mind, and some really hate this. Therefore, count the outer ring coast as bad if !bCoastal.
			// <!-- custom: This is separate from the Barbarian coastal value bonus: it keeps the inherited adjacent/inner-focused Barbarian scoring, which can ignore outer-BFC seafood or other long-term captured-city value. AdvCiv-SAS makes this toggleable because Barbarian cities should also be useful city sites for themselves and later conquerors; set SAS_EVALUATE_BARBARIAN_FULL_BFC_ENABLE to use normal full-BFC scoring. (GPT-5.5) -->
			if(p != NULL && p->isWater() && !bCoastal && p->calculateBestNatureYield(YIELD_FOOD, eTeam) <= 1)
			{
				iBadTiles++;
				if (bInner)
					iInnerRadius++;
			}
			continue;
		} // </advc.303>

		if (!p->isWater())
			iLand++;
		int iBadLoop = 0;
		if (p->isImpassable())
			iBadLoop += 2;
		// </advc.031>
		// K-Mod (original code deleted)
		else if (//!p->isFreshWater() &&
		/*  advc.031: Flood plains have high nature yield anyway,
			so this is about snow.
			Bonus check added (Incense). */
			getBonus(*p) == NO_BONUS &&
			(p->calculateBestNatureYield(YIELD_FOOD, eTeam) == 0 ||
			p->calculateTotalBestNatureYield(eTeam) <= 1))
		{
			/*  advc.031: Snow hills will count for BaseProduction, but, generally
				they're really bad. */
			iBadLoop += 2;
		}
		else if (p->isWater() && p->calculateBestNatureYield(YIELD_FOOD, eTeam) <= 1)
		{	/*  <advc.031> Removed the bCoastal check from the
				condition above b/c I want to count ocean tiles as
				half bad even when the city is at the coast. */
			if(!kSet.isSeafaring() &&
				p->calculateBestNatureYield(YIELD_COMMERCE, eTeam) <= 1)
			{
				iBadLoop++;
				// <advc.108> Ocean can't be "normalized" away later on
				if (kSet.isStartingLoc() /* adv.031f: */ && !kSet.isScenario())
					iBadLoop++; // </advc.108>
			}
			if(!bCoastal)
				iBadLoop++; // </advc.031>
		}
		else if (getRevealedOwner(*p) != NO_PLAYER)
		{
			if(!abFlip[eLoopCityPlot]) // advc.035
			{
				if (getRevealedTeam(*p) != eTeam || p->isBeingWorked())
					iBadLoop++;
				/*  (K-Mod) note: this final condition is...
					not something I intend to keep permanently. */
				// advc.031: poof
				/*else if (p->isCityRadius() || aiCitySiteRadius[iI] >= 0)
					iBadTiles++;*/
			} // advc.035
		}
		else iRevealedDecentLand++; // advc.040
		// <advc.031>
		iBadTiles += iBadLoop;
		if (bInner)
			iInnerRadius += iBadLoop;
	}
	iInnerRadius /= 2; // </advc.031>
	iBadTiles /= 2;
	// <!-- custom: countBadTiles has already computed the fresh reference value, while member iUnrevealedTiles is assigned only after this function returns. Log the local result instead of always reporting the constructor-initialized zero. See KI#505. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	IFLOG (iUnrevealed > 0 ? logBBAI("Bad tiles: %d known bad, %d unrevealed", iBadTiles, iUnrevealed) :
							 logBBAI("Bad tiles: %d", iBadTiles));
	return iBadTiles;
}

// Returns true if the site should be disregarded
bool AIFoundValue::isTooManyBadTiles(int iBadTiles, int iInnerBadTiles) const // advc.031
{
	if (kSet.isStartingLoc() /* advc.031e: */ || kSet.isNormalizing())
		return false;
	if (2 * iBadTiles <= NUM_CITY_PLOTS && kArea.getNumTiles() > 2 &&
		(!bBarbarian || iBadTiles <= 3)) // advc.303
	{
		return false;
	}
	/*	advc.031: Cities with only worthless tiles in the inner ring will
		struggle to ever expand their borders. */
	bool bInnerOnly = (!kSet.isEasyCulture() && 3 * iInnerBadTiles >= 2 * NUM_INNER_PLOTS);
	bool bHasGoodBonus = false;
	int iMediocreBonuses = 0; // advc.031
	int iFreshWaterTiles = (kPlot.isFreshWater() ? 1 : 0); // advc.031 (count kPlot twice)
	for (CityPlotIter it(kPlot); it.hasNext(); ++it)
	{
		CvPlot const& p = *it;
		if(!isRevealed(p))
			continue;
		// <advc.031>
		if (bInnerOnly && it.currID() >= NUM_INNER_PLOTS &&
			getRevealedOwner(p) != ePlayer)
		{
			continue;
		} // </advc.031>
		// <advc.303>
		static const bool bSAS_EVALUATE_BARBARIAN_FULL_BFC_ENABLE = GC.getDefineBOOL("SAS_EVALUATE_BARBARIAN_FULL_BFC_ENABLE");
		if(bBarbarian && !bSAS_EVALUATE_BARBARIAN_FULL_BFC_ENABLE && !adjacentOrSame(p, kPlot))
			continue; // </advc.303>
		if(getRevealedOwner(p) != NO_PLAYER &&
			// <advc.031>
			(getRevealedOwner(p) != ePlayer ||
			p.getCityRadiusCount() > 0)) // </advc.031>
		{
			continue;
		}
		BonusTypes eBonus = getBonus(p); // advc.108
		if (eBonus != NO_BONUS &&
			!kTeam.isBonusObsolete(eBonus)) // K-Mod
		{
			if ((kPlayer.getNumTradeableBonuses(eBonus) == 0 ||
				kPlayer.AI_bonusVal(eBonus, 1, true) > 10 ||
				GC.getInfo(eBonus).getYieldChange(YIELD_FOOD) > 0) &&
				// <advc.031> Moved from above
				(p.isWater() || p.isArea(kArea) ||
				p.getArea().getCitiesPerPlayer(ePlayer) > 0)) // </advc.031>
			{
				bHasGoodBonus = true;
				break;
			} // <advc.031>
			else iMediocreBonuses++;
		}
		if(p.isFreshWater())
			iFreshWaterTiles++; // </advc.031>
	}

	return (!bHasGoodBonus && /* advc.040: */ !bFirstColony &&
			iFreshWaterTiles < 3 && iMediocreBonuses < 2); // advc.031
}


int AIFoundValue::baseCityValue() const
{
	// advc.031: was 800 in K-Mod and 1000 before K-Mod
	int r = 150;
	// <advc.108>
	if (iUnrevealedTiles <= 0)
		return r;
	if (iCities <= 0)
	{
		int const iCapitalValue = (50 * scaled(iUnrevealedTiles).sqrt()).round();
		IFLOG if(iCapitalValue>0) logBBAI("+%d base value for unrevealed tiles near initial city", iCapitalValue);
		r += iCapitalValue;
	} // </advc.108>
	// <advc.040>
	else if (bFirstColony)
	{
		int const iFirstColonyValue = 55 * std::min(5, iUnrevealedTiles);
		IFLOG logBBAI("+%d base value for unrevealed tiles near first colony", iFirstColonyValue);
		r += iFirstColonyValue;
	} // </advc.040>
	return r;
}


bool AIFoundValue::isUsablePlot(CityPlotTypes ePlot, int& iTakenTiles, bool& bCityRadius, bool& bForeignOwned, bool& bAnyForeignOwned, bool& bShare, bool& bSteal) const
{
	CvPlot const* p = plotCity(iX, iY, ePlot);
	if (p == NULL)
	{
		iTakenTiles++;
		return false;
	}
	bool const bInnerRing = adjacentOrSame(*p, kPlot);
	// <advc.303>
	// <!-- custom: This was the remaining inherited inner-ring-only Barbarian city-site shortcut. Gate it with SAS_EVALUATE_BARBARIAN_FULL_BFC_ENABLE too, otherwise outer-BFC seafood still gets ignored even when bad-tile checks use full BFC scoring. (GPT-5.5) -->
	static const bool bSAS_EVALUATE_BARBARIAN_FULL_BFC_ENABLE = GC.getDefineBOOL("SAS_EVALUATE_BARBARIAN_FULL_BFC_ENABLE");
	if (bBarbarian && !bSAS_EVALUATE_BARBARIAN_FULL_BFC_ENABLE && !bInnerRing)
		return false; // </advc.303>
	/*	advc.031: Moved up. If we can't see the tile, we don't really know if it's
		already taken by a rival. (Will find out when our Settler gets there.) */
	if (!isRevealed(*p))
	{
		IFLOG logBBAI("Unrevealed plot skipped: (%d,%d)", p->getX(), p->getY());
		return false;
	}
	if (isHome(*p))
		return true;
	// <advc.035>
	// <!-- custom: debugStr now returns owning text; pass its live c_str() to the variadic found-value logger. See KI#349. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (abFlip[ePlot])
	{
		IFLOG logBBAI("Assumed to flip: %S", p->debugStr().c_str());
		return true;
	}
	// </advc.035>
	// <advc.031>
	bCityRadius = p->isCityRadius();
	PlayerTypes const eOwner = getRevealedOwner(*p);
	bForeignOwned = (eOwner != NO_PLAYER && eOwner != ePlayer);
	bSteal = (bCityRadius && bForeignOwned);
	 // </advc.031>
	if (bCityRadius || aiCitySiteRadius[ePlot] >= 0)
	{
		iTakenTiles++;
		// <advc.040>
		if(bForeignOwned)
			bAnyForeignOwned = true; // </advc.040>
		//return false;
	}
	/*  K-Mod Note: it kind of sucks that no value is counted for taken tiles.
		Tile sharing / stealing should be allowed. */
	// <advc.031> ^Exactly
	/*  Still don't allow tiles to be shared between (planned) city sites
		-- too difficult to estimate how many tiles each site will need. */
	if (aiCitySiteRadius[ePlot] >= 0)
	{
		IFLOG logBBAI("%S reserved for higher-priority site at (%d,%d)", p->debugStr().c_str(),
				kPlayer.AI_getCitySite(aiCitySiteRadius[ePlot]).getX(), kPlayer.AI_getCitySite(aiCitySiteRadius[ePlot]).getY());
		return false;
	}
	bool bOtherInnerRing = false;
	CvCityAI const* pOtherCity = NULL;
	if (bCityRadius)
	{
		if (bBarbarian)
		{
			IFLOG logBBAI("(%d,%d) is in the radius of another city", p->getX(), p->getY());
			return false;
		}
		pOtherCity = p->AI_getWorkingCity();
		if (pOtherCity == NULL && abOwnCityRadius[ePlot])
		{
			IFLOG logBBAI("(%d,%d) is in the radius of a %S city whose borders haven't expanded yet",
					p->getX(), p->getY(), kPlayer.getCivilizationShortDescription());
			/*  Difficult to judge whether tile sharing makes sense;
				better wait for borders to expand. */
			return false;
		}
		if (pOtherCity != NULL && (kPlayer.isHuman() ? pOtherCity->isRevealed(eTeam) :
			(kTeam.AI_deduceCitySite(*pOtherCity) ||
			/*  At the start of the game, a single revealed tile should
				be enough to locate the city. */
			iCities == 0)))
		{
			bOtherInnerRing = adjacentOrSame(*p, *pOtherCity->plot());
			FAssert(!bInnerRing || !bOtherInnerRing || !pOtherCity->isArea(kArea) ||
					/*	In that case, stealing from the inner ring should
						perhaps be considered. (But AdvCiv uses range 2 like BtS,
						so I'm not going to address range 1.) */
					GC.getDefineINT(CvGlobals::MIN_CITY_RANGE) < 2);
			if (bForeignOwned && (bOtherInnerRing ||
				// Don't try to overlap with team member or master
				TEAMID(eOwner) == eTeam ||
				kTeam.isVassal(TEAMID(eOwner))))
			{
				IFLOG logBBAI("Don't count on stealing %S from %S", p->debugStr().c_str(), cityName(*pOtherCity));
				return false;
			}
			if (!bForeignOwned)
				bShare = true;
			// Else let iCultureModifier handle bSteal
		}
	}
	if (!bShare)
		return true;

	if(p->isBeingWorked() || bOtherInnerRing)
	{
		IFLOG logBBAI("Don't want to take (%d,%d) away from %S", p->getX(), p->getY(), cityName(*pOtherCity));
		return false;
	}
	CityPlotTypes const eOtherPlotIndex = pOtherCity->getCityPlotIndex(*p);
	if (GC.getCityPlotPriority()[ePlot] >= GC.getCityPlotPriority()[eOtherPlotIndex])
	{
		IFLOG logBBAI("%S has higher priority for (%d,%d)", cityName(*pOtherCity), p->getX(), p->getY());
		return false;
	}
	// Check if the other city is going to need the tile in the medium term
	if (pOtherCity->AI_isGoodPlot(eOtherPlotIndex) &&
		pOtherCity->AI_countGoodPlots() -
		pOtherCity->getPopulation() +
		pOtherCity->getSpecialistPopulation() <= 3)
	{
		IFLOG logBBAI("Don't want to take (%d,%d); %S might need it soon", p->getX(), p->getY(), cityName(*pOtherCity));
		return false;
	}
	// Else let the caller deal with bShare
	/*  (No special treatment for bShare&&bSteal; we'll
		probably win the tile if we have two cities near it.) */
	return true;
	// </advc.031>
}

// based on K-Mod code
bool AIFoundValue::isRemovableFeature(CvPlot const& p, bool& bPersistent, int& iFeatureProduction) const // advc.031
{
	bPersistent = false;
	iFeatureProduction = 0;
	// <advc.031>
	if (isHome(p))
		return true; // </advc.031>
	FeatureTypes const eFeature = p.getFeatureType();
	if (eFeature == NO_FEATURE)
		return false;

	CvFeatureInfo const& kFeature = GC.getInfo(eFeature);
	bPersistent = true;
	FOR_EACH_ENUM(Build)
	{
		CvBuildInfo const& kLoopBuild = GC.getInfo (eLoopBuild);
		if (!kLoopBuild.isFeatureRemove(eFeature))
			continue;

		bPersistent = false;
		// <advc.031>
		CvTerrainInfo const& kTerrain = GC.getInfo(p.getTerrainType());
		int iTerrainYieldSum = 0;
		int iFeatureYieldSum = 0;
		FOR_EACH_ENUM(Yield)
		{
			iTerrainYieldSum += kTerrain.getYield(eLoopYield);
			iFeatureYieldSum += kFeature.getYieldChange(eLoopYield);
		}
		// Count feature production only if the feature is (presumably) worth removing
		if (iTerrainYieldSum > iFeatureYieldSum)
		{
			CvCity* pDummy;
			iFeatureProduction = p.getFeatureProduction(eLoopBuild, eTeam, &pDummy, &kPlot, ePlayer);
			if (getRevealedTeam(p) == eTeam)
				iFeatureProduction /= 3; // Can already chop it
		}
		// <!-- custom: AdvCiv required one current-research slot to equal both prerequisites, which is impossible when the Build and feature-removal technologies differ.
		// Test each prerequisite independently against team knowledge or any member's current research. See KI#481. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		TechTypes const eTech1 = kLoopBuild.getTechPrereq();
		TechTypes const eTech2 = kLoopBuild.getFeatureTech(eFeature);
		// </advc.031>
		bool bTech1Available = kTeam.isHasTech(eTech1);
		bool bTech2Available = kTeam.isHasTech(eTech2);
		if (bTech1Available && bTech2Available)
			return true;
		// <advc.031>
		for (MemberIter it(eTeam); it.hasNext(); ++it)
		{
			CvPlayer const& kMember = *it;
			TechTypes const eCurrentResearch = kMember.getCurrentResearch();
			bTech1Available = (bTech1Available || eCurrentResearch == eTech1);
			bTech2Available = (bTech2Available || eCurrentResearch == eTech2);
			if (bTech1Available && bTech2Available)
				return true;
		} // </advc.031>
	}
	return false;
}

// (replacing all CvPlot::isRevealed calls)
bool AIFoundValue::isRevealed(CvPlot const& p) const
{
	return (kSet.isAllSeeing() || p.isRevealed(eTeam));
}

// (replacing all CvPlot::getOwner and isOwned calls)
PlayerTypes AIFoundValue::getRevealedOwner(CvPlot const& p) const
{
	if (kSet.isAllSeeing())
		return p.getOwner();
	return p.getRevealedOwner(eTeam);
}

// (replacing all CvPlot::getTeam calls)
TeamTypes AIFoundValue::getRevealedTeam(CvPlot const& p) const
{
	PlayerTypes ePlayer = getRevealedOwner(p);
	if (ePlayer == NO_PLAYER)
		return NO_TEAM;
	return TEAMID(ePlayer);
}

// <!-- custom: Raw CvArea totals reveal cities beyond the evaluating team's knowledge. Count own/team cities and foreign cities whose sites are deducible, optionally for one revealed owner.
// Without an owner filter, exclude Barbarian cities like AdvCiv's getNumCivCities test; with one, preserve the culture modifier's ability to count cities belonging to a revealed Barbarian owner. See KI#493 and KI#495. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
int AIFoundValue::countKnownCities(CvArea const& kLoopArea, PlayerTypes eOwner) const
{
	int iKnownCities = 0;
	for (PlayerIter<ALIVE> it; it.hasNext(); ++it)
	{
		CvPlayer const& kLoopPlayer = *it;
		if ((eOwner == NO_PLAYER && kLoopPlayer.isBarbarian()) ||
			(!kSet.isAllSeeing() && !GET_TEAM(kLoopPlayer.getTeam()).isHasMet(eTeam)) ||
			(eOwner != NO_PLAYER && kLoopPlayer.getID() != eOwner))
			continue;
		FOR_EACH_CITY(pLoopCity, kLoopPlayer)
		{
			if (!pLoopCity->isArea(kLoopArea))
				continue;
			if (!kSet.isAllSeeing() && kLoopPlayer.getTeam() != eTeam && !kTeam.AI_deduceCitySite(*pLoopCity))
				continue;
			iKnownCities++;
		}
	}
	return iKnownCities;
}

// (replacing all CvPlot::getBonusType and getNonObsoleteBonusType calls)
BonusTypes AIFoundValue::getBonus(CvPlot const& p) const
{
	BonusTypes const eBonus = p.getBonusType();
	if (eBonus == NO_BONUS)
		return NO_BONUS;
	if (kSet.isDiagnosticOmniscience())
		return eBonus;
	// <advc.108>
	if (kSet.isStartingLoc() /* advc.031e: */ || kSet.isNormalizing())
	{
		if (kGame.getStartingPlotNormalizationLevel() > CvGame::NORMALIZE_LOW)
			return eBonus;
		/*	Treat resources revealed by any starting tech as revealed.
			(No such resources exist in BtS, AdvCiv.) */
		TechTypes const eTech = GC.getInfo(eBonus).getTechReveal();
		if (eTech != NO_TECH &&
			GC.getInfo(eTech).getNumOrTechPrereqs() <= 0 &&
			GC.getInfo(eTech).getNumAndTechPrereqs() <= 0)
		{
			return eBonus;
		}
	} // </advc.108>
	return p.getBonusType(eTeam);
}

/*  advc.031: Rewritten. K-Mod had added a crucial isImprovementBonusTrade check, but
	other important checks were still missing. */
ImprovementTypes AIFoundValue::getBonusImprovement(BonusTypes eBonus, CvPlot const& p, bool& bCanTrade, bool& bCanTradeSoon, int* aiYield, bool& bCanImprove, bool& bCanImproveSoon, bool& bRemoveFeature) const
{
	bCanTrade = bCanTradeSoon = bCanImprove = bCanImproveSoon = bRemoveFeature = false;
	if (eBonus == NO_BONUS)
		return NO_IMPROVEMENT;

	ImprovementTypes eBestImprovement = NO_IMPROVEMENT;
	int iBestYield = -1;
	FeatureTypes const eFeature = p.getFeatureType();
	FOR_EACH_ENUM(Build)
	{
		CvBuildInfo const& kLoopBuild = GC.getInfo(eLoopBuild);
		ImprovementTypes eImprovement = kLoopBuild.getImprovement();
		if (eImprovement == NO_IMPROVEMENT)
			continue;
		CvImprovementInfo const& kImprovement = GC.getInfo(eImprovement);
		// <!-- custom: The inherited scan could select a land Well for water Oil before the legal Offshore Platform technology. Reject improvements whose domain differs from the actual bonus plot while retaining intentional near-future technology evaluation. See KI#483. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if (kImprovement.isWater() != p.isWater())
			continue;
		// <!-- custom: trying to make extra extra sure we don't build forts as they are very inefficient (long time to build, yield less than improvements, and unlikely a human or other player would ideally attack units garrisoned there), they could have some uses (maybe prebuilding connection, allowing naval units to pass/cross land), but more often than not they should not benefit the AI, and currently the AI often spends a lot of time undoing existing improvements in base advciv as i have noticed many times. I don't know too much how to fix this, but with chatgpt's help i am adding a few bits of code that try to prevent that, here is one of them, see the Main Changes Guide or some similar or related or other docs in our mod for update status rather than here. -->
		if (kImprovement.isActsAsCity()) // Usually means Fort
		{
			continue;
		}
		TechTypes const eBuildPrereq = kLoopBuild.getTechPrereq();
		if (!kImprovement.isImprovementBonusMakesValid(eBonus) ||
			!kImprovement.isImprovementBonusTrade(eBonus) ||
			!isNearTech(eBuildPrereq))
		{
			continue;
		}
		TechTypes const eFeaturePrereq = (eFeature == NO_FEATURE ? NO_TECH :
				kLoopBuild.getFeatureTech(eFeature));
		if (!isNearTech(eFeaturePrereq))
			continue;
		bCanTradeSoon = true;
		bCanImproveSoon = true;
		if (kTeam.isHasTech(eBuildPrereq) && kTeam.isHasTech(eFeaturePrereq))
		{
			bCanTrade = true;
			bCanImprove = true;
		}
		// <!-- custom: based on my understanding of this code thanks to gemini ai's help as well, that is quite different from what gemini ai seems to say about it that it is related to workers or such if i understood it correctly, which i also asked as this code seemed weird so i asked it about it, this may explain why we are happy to count an improvement as good enough, say a farm, even if a plantation is better in terms of yield, so don't bother calculating the rest. I assume this is done for computational effiency, but may lead to other problems down the line. I'd rather prefer it be accurate and see what happens and if for example AI settles better or worse on tiles. I also want to see if this somehow affects AI building farms on grapes plains instead of waiting for plantations, so commenting it out. And i want to see if it's really much faster with it, although we should not have too much settlers or bonuses, except at turn 1, and even then it doesn't seem like it would cost a lot of extra computation to run this, so if it can help in other ways, try to do the full calculation/bonus yield evaluation rather and not just of first eligible found build -->
		// else if (bCanTrade) // Prefer currently available build - regardless of yield
		// 	continue;
		int iYieldValue = 0;
		bool bRemove = (eFeature != NO_FEATURE && kLoopBuild.isFeatureRemove(eFeature));
		FOR_EACH_ENUM(Yield) // Make sure we're not picking Fort over a yield improvement
		{
			iYieldValue += kImprovement.getYieldChange(eLoopYield) +
					kImprovement.getImprovementBonusYield(eBonus, eLoopYield);
			if (!bRemove && eFeature != NO_FEATURE)
				iYieldValue += GC.getInfo(eFeature).getYieldChange(eLoopYield);
		}
		if (iYieldValue > iBestYield)
		{
			eBestImprovement = eImprovement;
			iBestYield = iYieldValue;
			bRemoveFeature = bRemove;
		}
	}
	TechTypes eTradeTech = GC.getInfo(eBonus).getTechCityTrade();
	bCanTradeSoon = (bCanTradeSoon && isNearTech(eTradeTech));
	bCanTrade = (bCanTrade && bCanTradeSoon && kTeam.isHasTech(eTradeTech));
	/*  <advc.108> Depending on the NormalizationLevel, eBonus can be unrevealed
		when placing starting locations. */
	if (kSet.isStartingLoc() || kSet.isNormalizing())
	{
		TechTypes eRevealTech = GC.getInfo(eBonus).getTechReveal();
		if (!kTeam.isHasTech(eRevealTech))
		{
			bCanTrade = false;
			bCanImprove = false;
			if (!isNearTech(eRevealTech))
			{
				bCanTradeSoon = false;
				bCanImproveSoon = false;
			}
		}
	} // </advc.108>
	if (eBestImprovement == NO_IMPROVEMENT)
		return NO_IMPROVEMENT;
	// <!-- custom: attempted improvement with chatgpt's help, as an extension of base advciv's commit https://github.com/f1rpo/AdvCiv/commit/1a372d417a6001e2afe2b40e69824b45fa375907 and approach i (actually yes was me... Hello youtube xd or whoever but not xd) asked f1rpo who kindly gave this partial fix, now trying to improve it, in particular with(/in?) regards to food yields being underestimated, but also the AI still planting (cities) on metals and often on food as wellso trying to fix or improve that at least. Again i don't know too much about these but this is a tentative approach with chatgpt's help and quite cautiously, hopefully safe perhaps even ideally and as intended in this case etcwould fix/improve the yield issue -->

	// <!-- custom: Keep this array as actual improvement yield changes. The caller now applies simple XML-tunable Food/Production/Commerce values once; the older SAS Food x3 and Production x2 preprocessing here was then weighted again by AdvCiv's evaluateSpecialYields and produced extreme resource scores. (GPT-5.5) -->
	FOR_EACH_ENUM(Yield)
	{
		aiYield[eLoopYield] = p.calculatePotentialImprovementYieldChange(eBestImprovement, eLoopYield, ePlayer, eBonus);

		// <!-- custom: old AdvCiv stop-gap retained for reference; calculateImprovementYieldChange includes the improvement's normal, bonus, hill/river/irrigation, player, and team yield changes directly. (GPT-5.5) -->
		// aiYield[eLoopYield] = GC.getInfo(eBestImprovement).getImprovementBonusYield(eBonus, eLoopYield);
		// /*	Open issue: We don't count the improvement's YieldChange and
		// 	IrrigatedYieldChange, i.e. we treat wet farms, dry farms and
		// 	improvements that don't add any yield (plantation, pasture)
		// 	all the same and thus overestimate the latter. This separate
		// 	accounting for "special" yields really needs to go, and then
		// 	CvPlot::calculateImprovementYieldChange could perhaps be used
		// 	(for all plots). Stop-gap measure for Mines: */
		// aiYield[eLoopYield] += GC.getInfo(eBestImprovement).getYieldChange(eLoopYield) / 2;
		// if (!bRemoveFeature && eFeature != NO_FEATURE)
		// {
		// 	aiYield[eLoopYield] += GC.getInfo(eFeature).getYieldChange(eLoopYield);
		// 	if (p.isRiver())
		// 		aiYield[eLoopYield] += GC.getInfo(eFeature).getRiverYieldChange(eLoopYield);
		// }
		// else if (p.isRiver())
		// 	aiYield[eLoopYield] += GC.getInfo(p.getTerrainType()).getRiverYieldChange(eLoopYield);
	}
	return eBestImprovement;
} // </advc.031>


bool AIFoundValue::isNearTech(TechTypes eTech) const
{
	return (eTech == NO_TECH || kTeam.isHasTech(eTech) ||
			kPlayer.getCurrentResearch() == eTech ||
			/*  <advc.108> With our first city, we can wait a bit a longer for the
				proper tech. (Not when starting in a later era though; research
				starts out slow then.) */
			(iCities <= 0 && eEra >= GC.getInfo(eTech).getEra() &&
			kGame.getStartEra() <= 0) || // </advc.108>
			/*	(The HasTech and CurrentResearch checks are redundant, I think,
				but faster.) */ kPlayer.canResearch(eTech));
}

// (not much pre-AdvCiv code left)
int AIFoundValue::calculateCultureModifier(CvPlot const& p, bool bForeignOwned, bool bShare, bool bCityRadius, bool bSteal, bool bFlip, bool bOwnExcl, int& iTakenTiles, int& iStealPercent) const
{
	scaled r = 100;
	/*  K-Mod note: iClaimThreshold is bigger for bEasyCulture and bAmbitious civs.
		Also note, if the multiplier was to be properly used for unowned plots,
		it would need to take into account the proximity of foreign cities and so on.
		(similar to the iForeignProximity calculation) */
	// advc.031: I'm including unowned plots, but only those in a foreign city radius.
	if ((!bForeignOwned && (bShare || !bCityRadius)) /* advc.035: */ || bFlip)
		return r.round();

	int iOurCulture = p.getCulture(ePlayer);
	int iOtherCulture = std::max(1,
			/*  advc.031: Could find out the owner of the bCityRadius
				city in the loop that computes abFlip, but it should be
				fine to assume that the owner has very little tile culture. */
			(!bForeignOwned ? 1 :
			p.getCulture(getRevealedOwner(p))));
	// <advc.031> Don't settle near rival capital (e.g. 2nd starting settler, OCC)
	if (bCityRadius && kGame.getElapsedGameTurns() <= 5)
		iOtherCulture = std::max(iOtherCulture, 200);
	CvCity* pForeignCity = p.getWorkingCity();
	if (bForeignOwned)
	{
		if ((pForeignCity != NULL && pForeignCity->isCapital()) ||
			// <!-- custom: Keep the single-colony culture penalty, but base it on rival cities the evaluating team can locate rather than hidden CvArea totals. See KI#495. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			(iAreaCities <= 0 && countKnownCities(p.getArea(), getRevealedOwner(p)) > 1))
		{
			iOtherCulture = (3 * iOtherCulture) / 2;
		}
	} // </advc.031>
	r.mulDiv(iOurCulture + kSet.getClaimThreshold(), iOtherCulture + kSet.getClaimThreshold());
	if (!bOwnExcl) // advc.035: The above is OK if cities own their exclusive radius ...
	{	// <advc.031>
		// ... but w/o that rule, it's too optimistic when iOurCulture is small.
		r = (r + (iOurCulture * 100) / kSet.getClaimThreshold()) / 2;
	}
	// Take into account at least some factors that determine each side's culture rate
	scaled rRateModifier = 1;
	/*	Don't be _too_ optimistic against Barbarians - a civ (a rival - or us) will
		probably conquer pForeignCity eventually and might not raze it. */
	if (bForeignOwned && p.isBarbarian())
		rRateModifier *= fixp(1.55);
	else if(pForeignCity != NULL &&
		// advc.303: Barbarian borders won't expand
		!adjacentOrSame(p, *pForeignCity->plot()))
	{
		rRateModifier *= fixp(1.8);
	}
	if (!p.isArea(kArea))
		rRateModifier /= fixp(1.3);
	else if (pForeignCity != NULL && !pForeignCity->isArea(p.getArea()))
		rRateModifier *= fixp(1.3);
	/*  K-Mod had done *5/4 if EasyCulture; I think only free culture will really
		swing culture wars. */
	int iFreeForeignCulture = (bForeignOwned ? GET_PLAYER(getRevealedOwner(p)).
			getFreeCityCommerce(COMMERCE_CULTURE) : 0);
	if (pForeignCity != NULL && pForeignCity->isCapital())
	{
		/*  Tbd.: Should just use pForeignCity->getCommerceRate(COMMERCE_CULTURE).
			advc.045 hides foreign buildings, but it should, in theory, be possible
			to infer culture buildings from visible tile culture percentages.
			But will then also have to predict our own culture rate. */
		static BuildingTypes const eCapitalBuilding = GC.getInfo(
				pForeignCity->getCivilizationType()).
				getCivilizationBuildings(GC.getDefineINT("CAPITAL_BUILDINGCLASS"));
		if (eCapitalBuilding != NO_BUILDING)
		{
			iFreeForeignCulture += GC.getInfo(eCapitalBuilding).
					getCommerceChange(COMMERCE_CULTURE); // (normally 0)
			iFreeForeignCulture += GC.getInfo(eCapitalBuilding).
					getObsoleteSafeCommerceChange(COMMERCE_CULTURE);
		}
	}
	scaled const rFreeForeignCultureModifier(iFreeForeignCulture + 7, 7);
	// Extra pessimism about tiles that another civ is able to work (borders already expanded)
	if (bSteal)
	{
		rRateModifier = fixp(0.62);
		if (pForeignCity != NULL && adjacentOrSame(p, *pForeignCity->plot()))
			rRateModifier /= fixp(1.5);
		if (bForeignOwned)
			rRateModifier *= rFreeForeignCultureModifier;
	}
	if (adjacentOrSame(p, kPlot))
		rRateModifier *= fixp(1.5); // as in K-Mod
	else rRateModifier *= rFreeForeignCultureModifier;	
	if (bCityRadius && !bShare)
	{
		r *= rRateModifier * fixp(0.8);
		iStealPercent += std::min(75, r.round());
	}
	else r *= rRateModifier;
	// </advc.031>
	r.decreaseTo(100);
	// advc.031: Moved up
	if (r < (iAreaCities > 0 ? 25 : 50) &&
		!bSteal) // advc.031: Already counted as iTakenTiles
	{
		//discourage hopeless cases, especially on other continents.
		iTakenTiles += (iAreaCities > 0 ? 1 : 2);
	}
	// <advc.099b>
	if (!bCityRadius && bForeignOwned)
	{
		scaled rExclRadiusWeight = GC.AI_getGame().AI_exclusiveRadiusWeight(
				::plotDistance(&p, &kPlot));
		r *= 1 + rExclRadiusWeight;
		r.decreaseTo(100);
		// </advc.099b>  <advc.035>
		if (bOwnExcl)
		{
			// (The discouragement above is still useful for avoiding revolts)
			r.increaseTo(65);
		} // </advc.035>
	}
	return r.round();
}

// K-Mod: adjust for removable features (advc: moved into a subroutine)
int AIFoundValue::removableFeatureYieldVal(FeatureTypes eFeature, bool bRemovableFeature, bool bBonus) const
{
	int iR = 0;
	CvFeatureInfo const& kFeature = GC.getInfo(eFeature);
	FOR_EACH_ENUM(Yield)
	{
		if (bRemovableFeature)
			iR += 10 * kFeature.getYieldChange(eLoopYield);
		else if (kFeature.getYieldChange(eLoopYield) < 0)
		{
			iR -= (bBonus ? 25 : 5);
			// advc.031: was 30 *...
			iR += 25 * kFeature.getYieldChange(eLoopYield);
		}
	}
	IFLOG if(iR!=0) logBBAI("From (removable) feature yield: %d", iR);
	return iR;
}


// <!-- custom: This replaces the old AdvCiv 031 block rather than extending its fixed Hill/feature heuristic.
// AdvCiv itself called the earlier Hills=200/other=100 shortcut "pretty bad" because it assumed Workshops and ignored Peaks and Deserts; any fixed Hill value is also unsuitable when a mod-mod can change Hill yields and legal improvements.
// The removed bPersistentFeature parameter was a plot-wide approximation; the replacement derives feature removal separately for every legal XML Build, while one-time feature-removal production remains valued elsewhere. (GPT-5.6-Sol) -->
/*	An estimate of how much production an improvement might add
	in the medium term if production is prioritized. Precision: times 100.
	Note: Any additional production from improving a bonus resource
	is counted as iSpecialProduction elsewhere. This function ignores
	bonus resources. */
scaled AIFoundValue::estimateImprovementProduction(CvPlot const& p) const
{
	scaled rCached;
	if (kSet.getCachedImprovementProduction(p, rCached))
		return rCached;
	FeatureTypes const eFeature = p.getFeatureType();
	BonusTypes const eBonus = getBonus(p);
	int aiCurrentYield[NUM_YIELD_TYPES];
	FOR_EACH_ENUM(Yield)
		aiCurrentYield[eLoopYield] = p.calculateNatureYield(eLoopYield, eBonus == NO_BONUS ? NO_TEAM : eTeam);
	scaled r;
	// <!-- custom: Owning one existing improvement is not evidence that the player can build it, and a newly unlocked improvement has no existing copy yet.
	// Compare each currently legal Build's resulting XML yields with the unimproved plot; the shared yield helper includes applicable route, technology and player/team modifiers, including civic-derived changes, unlike the old shortcut that intentionally omitted civics and routes.
	// Bonus-specific improvement yield is removed here because it is valued separately.
	// This handles feature removal, water improvements and unusual terrain/feature rules without assuming that every early Hill supplies two future hammers or that a productive feature implies a fixed Lumbermill/chop result.
	// One-time feature-removal production remains valued separately. See KI#504. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	FOR_EACH_ENUM(Build)
	{
		CvBuildInfo const& kLoopBuild = GC.getInfo(eLoopBuild);
		ImprovementTypes const eLoopImprovement = kLoopBuild.getImprovement();
		if (eLoopImprovement == NO_IMPROVEMENT)
			continue;
		TechTypes const eBuildTech = kLoopBuild.getTechPrereq();
		if (eBuildTech != NO_TECH && !kTeam.isHasTech(eBuildTech))
			continue;
		if (eFeature != NO_FEATURE && kLoopBuild.isFeatureRemove(eFeature))
		{
			TechTypes const eFeatureTech = kLoopBuild.getFeatureTech(eFeature);
			if (eFeatureTech != NO_TECH && !kTeam.isHasTech(eFeatureTech))
				continue;
		}
		if (!p.canHaveImprovement(eLoopImprovement, eTeam, false, eLoopBuild, false))
			continue;
		bool const bRemoveFeature = (eFeature != NO_FEATURE && kLoopBuild.isFeatureRemove(eFeature));
		CvImprovementInfo const& kLoopImprovement = GC.getInfo(eLoopImprovement);
		int aiResultYield[NUM_YIELD_TYPES];
		FOR_EACH_ENUM(Yield)
		{
			aiResultYield[eLoopYield] = p.calculateNatureYield(eLoopYield, eBonus == NO_BONUS ? NO_TEAM : eTeam, bRemoveFeature) + p.calculatePotentialImprovementYieldChange(eLoopImprovement, eLoopYield, ePlayer, eBonus);
			if (eBonus != NO_BONUS)
				aiResultYield[eLoopYield] -= kLoopImprovement.getImprovementBonusYield(eBonus, eLoopYield);
		}
		int iYieldChange = aiResultYield[YIELD_PRODUCTION] - aiCurrentYield[YIELD_PRODUCTION];
		// Will be less inclined to build improvement if it hurts other yields
		FOR_EACH_ENUM(Yield)
		{
			if (eLoopYield != YIELD_PRODUCTION)
				iYieldChange += std::min(0, aiResultYield[eLoopYield] - aiCurrentYield[eLoopYield]);
		}
		if (iYieldChange <= 0)
			continue;
		// <!-- custom: Do not retain AdvCiv's iYieldChange <= 3 assertion: XML, route, technology and player/team modifiers can legitimately exceed that old fixed-yield assumption. (GPT-5.6-Sol) -->
		r.increaseTo(iYieldChange);
	}
	kSet.cacheImprovementProduction(p, r);
	return r;
}


// <!-- custom: Score the strongest plausible worked-tile outcome by enumerating Build/Improvement XML instead of naming Farm, Mine, Cottage, terrain or features.
// The XML defaults give immediate improvement yields two-thirds weight and the final upgrade one-third, so growth chains matter without treating a new first-stage improvement as fully mature.
// Builds available now retain full value; the XML defaults retain 75% for near-researchable Builds and 50% for later Builds; this lets sites retain tunable long-term potential without allowing late infrastructure to erase early terrain differences. (GPT-5.6-Sol) -->
int AIFoundValue::evaluateBestPotentialPlotYield(CvPlot const& p, bool bCanNeverImprove, ImprovementTypes& eBestImprovement, int* aiBestYield, int& iTimingPercent) const
{
	bool const bLogCandidates = (gFoundLogLevel >= 2 && AIFoundValue::isLoggingEnabled());
	bool const bLogCandidateDetails = (gFoundLogLevel >= 3 && AIFoundValue::isLoggingEnabled());
	static const int iImmediateImprovementWeight = std::max(0, GC.getDefineINT("SAS_EVALUATE_PLOT_POTENTIAL_IMMEDIATE_IMPROVEMENT_WEIGHT"));
	static const int iFinalUpgradeWeight = std::max(0, GC.getDefineINT("SAS_EVALUATE_PLOT_POTENTIAL_FINAL_UPGRADE_WEIGHT"));
	static const int iImprovementStageWeight = iImmediateImprovementWeight + iFinalUpgradeWeight;
	static const int iNearTechValuePercent = std::max(0, std::min(100, GC.getDefineINT("SAS_EVALUATE_PLOT_POTENTIAL_NEAR_TECH_VALUE_PERCENT")));
	static const int iLaterTechValuePercent = std::max(0, std::min(100, GC.getDefineINT("SAS_EVALUATE_PLOT_POTENTIAL_LATER_TECH_VALUE_PERCENT")));
	int iCachedValue = 0;
	if (!bLogCandidates && kSet.getCachedPlotPotentialYield(p, iCachedValue, eBestImprovement, aiBestYield, iTimingPercent))
		return iCachedValue;
	eBestImprovement = NO_IMPROVEMENT;
	iTimingPercent = 0;
	int const iExtraYield = GC.getDefineINT(CvGlobals::EXTRA_YIELD);
	int aiCurrentNatureYield[NUM_YIELD_TYPES];
	int aiUnimprovedYield[NUM_YIELD_TYPES];
	FOR_EACH_ENUM(Yield)
	{
		aiCurrentNatureYield[eLoopYield] = p.calculateNatureYield(eLoopYield, NO_TEAM);
		aiUnimprovedYield[eLoopYield] = aiCurrentNatureYield[eLoopYield];
		if (!kSet.isStartingLoc())
		{
			int const iExtraYieldThreshold = kPlayer.getExtraYieldThreshold(eLoopYield);
			if (iExtraYieldThreshold > 0 && aiUnimprovedYield[eLoopYield] >= iExtraYieldThreshold)
				aiUnimprovedYield[eLoopYield] += iExtraYield;
			int const iNaturalThreshold = kPlayer.getExtraYieldNaturalThreshold(eLoopYield);
			if (iNaturalThreshold > 0 && aiUnimprovedYield[eLoopYield] + 1 >= iNaturalThreshold)
				aiUnimprovedYield[eLoopYield] += iExtraYield;
		}
		aiBestYield[eLoopYield] = aiUnimprovedYield[eLoopYield];
	}
	int const iUnimprovedValue = evaluateYield(aiUnimprovedYield, &p, bCanNeverImprove, false);
	int iBestValue = iUnimprovedValue;
	FeatureTypes const eFeature = p.getFeatureType();
	// <!-- custom: Found level 2 records the three strongest legal Build outcomes for each inspected plot.
	// Level 3 expands those same three with immediate/final yields and feature-removal state, enough to distinguish XML, maturation and timing effects without logging every rejected Build. (GPT-5.6-Sol) -->
	BuildTypes aeTopBuild[3] = {NO_BUILD, NO_BUILD, NO_BUILD};
	int aiTopBuildValue[3] = {MIN_INT, MIN_INT, MIN_INT};
	int aiTopBuildTimingPercent[3] = {0, 0, 0};
	int aiTopBuildImmediateValue[3] = {0, 0, 0};
	int aiTopBuildFinalValue[3] = {0, 0, 0};
	int aiTopBuildMaturedValue[3] = {0, 0, 0};
	bool abTopBuildRemovesFeature[3] = {false, false, false};
	int aaiTopBuildImmediateYield[3][NUM_YIELD_TYPES] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
	int aaiTopBuildFinalYield[3][NUM_YIELD_TYPES] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
	int iCandidateCount = 0;
	int aiRemovedFeatureNatureYield[NUM_YIELD_TYPES];
	bool bRemovedFeatureNatureYieldCached = false;
	FOR_EACH_ENUM(Build)
	{
		CvBuildInfo const& kBuild = GC.getInfo(eLoopBuild);
		ImprovementTypes const eImprovement = kBuild.getImprovement();
		if (eImprovement == NO_IMPROVEMENT || GC.getInfo(eImprovement).isActsAsCity() || !p.canHaveImprovement(eImprovement, eTeam, true, eLoopBuild, false))
			continue;

		bool const bRemoveFeature = (eFeature != NO_FEATURE && kBuild.isFeatureRemove(eFeature));
		ImprovementTypes const eFinalImprovement = CvImprovementInfo::finalUpgrade(eImprovement);
		int aaiStageYield[2][NUM_YIELD_TYPES];
		int aiPotentialNatureYield[NUM_YIELD_TYPES];
		// <!-- custom: Every legal Build on this plot sees one of only two natural-yield states: retain the feature or remove it.
		// Reuse the current state and compute the removed-feature state once lazily instead of repeating calculateNatureYield for every Build and Yield. (GPT-5.6-Sol) -->
		if (bRemoveFeature && !bRemovedFeatureNatureYieldCached)
		{
			FOR_EACH_ENUM(Yield)
				aiRemovedFeatureNatureYield[eLoopYield] = p.calculateNatureYield(eLoopYield, NO_TEAM, true);
			bRemovedFeatureNatureYieldCached = true;
		}
		FOR_EACH_ENUM(Yield)
		{
			int const iNatureYield = (bRemoveFeature ? aiRemovedFeatureNatureYield[eLoopYield] : aiCurrentNatureYield[eLoopYield]);
			aiPotentialNatureYield[eLoopYield] = iNatureYield;
			aaiStageYield[0][eLoopYield] = iNatureYield + p.calculatePotentialImprovementYieldChange(eImprovement, eLoopYield, ePlayer, NO_BONUS);
			// <!-- custom: A non-growing improvement's final stage is its immediate stage; avoid repeating the same full route/technology/civic yield calculation. (GPT-5.6-Sol) -->
			aaiStageYield[1][eLoopYield] = (eFinalImprovement == NO_IMPROVEMENT ? aaiStageYield[0][eLoopYield] : iNatureYield + p.calculatePotentialImprovementYieldChange(eFinalImprovement, eLoopYield, ePlayer, NO_BONUS));
		}
		if (!kSet.isStartingLoc())
		{
			for (int iStage = 0; iStage < 2; ++iStage)
			{
				FOR_EACH_ENUM(Yield)
				{
					int const iExtraYieldThreshold = kPlayer.getExtraYieldThreshold(eLoopYield);
					if (iExtraYieldThreshold > 0 && aaiStageYield[iStage][eLoopYield] >= iExtraYieldThreshold)
						aaiStageYield[iStage][eLoopYield] += iExtraYield;
					int const iNaturalThreshold = kPlayer.getExtraYieldNaturalThreshold(eLoopYield);
					if (iNaturalThreshold > 0 && aiPotentialNatureYield[eLoopYield] + 1 >= iNaturalThreshold)
						aaiStageYield[iStage][eLoopYield] += iExtraYield;
				}
			}
		}
		int const iImmediateValue = evaluateYield(aaiStageYield[0], &p, false, false);
		int const iFinalValue = evaluateYield(aaiStageYield[1], &p, false, false);
		int const iMaturedValue = (iImprovementStageWeight <= 0 ? iUnimprovedValue : (iImmediateImprovementWeight * iImmediateValue + iFinalUpgradeWeight * iFinalValue) / iImprovementStageWeight);
		TechTypes const eBuildTech = kBuild.getTechPrereq();
		TechTypes const eFeatureTech = (eFeature == NO_FEATURE ? NO_TECH : kBuild.getFeatureTech(eFeature));
		bool const bAvailableNow = ((eBuildTech == NO_TECH || kTeam.isHasTech(eBuildTech)) && (eFeatureTech == NO_TECH || kTeam.isHasTech(eFeatureTech)));
		bool const bAvailableSoon = (isNearTech(eBuildTech) && isNearTech(eFeatureTech));
		int const iCandidateTimingPercent = (bAvailableNow ? 100 : (bAvailableSoon ? iNearTechValuePercent : iLaterTechValuePercent));
		int const iCandidateValue = iUnimprovedValue + ((iMaturedValue - iUnimprovedValue) * iCandidateTimingPercent) / 100;
		if (bLogCandidates)
		{
			++iCandidateCount;
			for (int iRank = 0; iRank < 3; ++iRank)
			{
				if (iCandidateValue <= aiTopBuildValue[iRank])
					continue;
				for (int iShift = 2; iShift > iRank; --iShift)
				{
					aeTopBuild[iShift] = aeTopBuild[iShift - 1];
					aiTopBuildValue[iShift] = aiTopBuildValue[iShift - 1];
					aiTopBuildTimingPercent[iShift] = aiTopBuildTimingPercent[iShift - 1];
					aiTopBuildImmediateValue[iShift] = aiTopBuildImmediateValue[iShift - 1];
					aiTopBuildFinalValue[iShift] = aiTopBuildFinalValue[iShift - 1];
					aiTopBuildMaturedValue[iShift] = aiTopBuildMaturedValue[iShift - 1];
					abTopBuildRemovesFeature[iShift] = abTopBuildRemovesFeature[iShift - 1];
					FOR_EACH_ENUM(Yield)
					{
						aaiTopBuildImmediateYield[iShift][eLoopYield] = aaiTopBuildImmediateYield[iShift - 1][eLoopYield];
						aaiTopBuildFinalYield[iShift][eLoopYield] = aaiTopBuildFinalYield[iShift - 1][eLoopYield];
					}
				}
				aeTopBuild[iRank] = eLoopBuild;
				aiTopBuildValue[iRank] = iCandidateValue;
				aiTopBuildTimingPercent[iRank] = iCandidateTimingPercent;
				aiTopBuildImmediateValue[iRank] = iImmediateValue;
				aiTopBuildFinalValue[iRank] = iFinalValue;
				aiTopBuildMaturedValue[iRank] = iMaturedValue;
				abTopBuildRemovesFeature[iRank] = bRemoveFeature;
				FOR_EACH_ENUM(Yield)
				{
					aaiTopBuildImmediateYield[iRank][eLoopYield] = aaiStageYield[0][eLoopYield];
					aaiTopBuildFinalYield[iRank][eLoopYield] = aaiStageYield[1][eLoopYield];
				}
				break;
			}
		}
		if (iCandidateValue <= iBestValue)
			continue;
		iBestValue = iCandidateValue;
		eBestImprovement = eImprovement;
		iTimingPercent = iCandidateTimingPercent;
		FOR_EACH_ENUM(Yield)
			aiBestYield[eLoopYield] = (iImprovementStageWeight <= 0 ? aiUnimprovedYield[eLoopYield] : (iImmediateImprovementWeight * aaiStageYield[0][eLoopYield] + iFinalUpgradeWeight * aaiStageYield[1][eLoopYield]) / iImprovementStageWeight);
	}
	if (bLogCandidates) logBBAI("PLOT_POTENTIAL_CANDIDATES plot=%d,%d home=%d unimproved=%d candidates=%d best=%S/%d/%d%% second=%S/%d/%d%% third=%S/%d/%d%%",
		p.getX(), p.getY(), isHome(p), iUnimprovedValue, iCandidateCount,
		(aeTopBuild[0] == NO_BUILD ? L"-" : GC.getInfo(aeTopBuild[0]).getDescription()), (aeTopBuild[0] == NO_BUILD ? 0 : aiTopBuildValue[0]), aiTopBuildTimingPercent[0],
		(aeTopBuild[1] == NO_BUILD ? L"-" : GC.getInfo(aeTopBuild[1]).getDescription()), (aeTopBuild[1] == NO_BUILD ? 0 : aiTopBuildValue[1]), aiTopBuildTimingPercent[1],
		(aeTopBuild[2] == NO_BUILD ? L"-" : GC.getInfo(aeTopBuild[2]).getDescription()), (aeTopBuild[2] == NO_BUILD ? 0 : aiTopBuildValue[2]), aiTopBuildTimingPercent[2]);
	if (bLogCandidateDetails)
	{
		for (int iRank = 0; iRank < 3 && aeTopBuild[iRank] != NO_BUILD; ++iRank)
		{
			ImprovementTypes const eImprovement = GC.getInfo(aeTopBuild[iRank]).getImprovement();
			ImprovementTypes const eFinalImprovement = CvImprovementInfo::finalUpgrade(eImprovement);
			logBBAI("PLOT_POTENTIAL_CANDIDATE_DETAIL plot=%d,%d home=%d rank=%d build=%S improvement=%S final=%S removeFeature=%d timing=%d%% immediate=%dF%dP%dC/%d finalYields=%dF%dP%dC/%d matured=%d candidate=%d",
				p.getX(), p.getY(), isHome(p), iRank + 1, GC.getInfo(aeTopBuild[iRank]).getDescription(), GC.getInfo(eImprovement).getDescription(),
				(eFinalImprovement == NO_IMPROVEMENT ? GC.getInfo(eImprovement).getDescription() : GC.getInfo(eFinalImprovement).getDescription()),
				abTopBuildRemovesFeature[iRank], aiTopBuildTimingPercent[iRank],
				aaiTopBuildImmediateYield[iRank][YIELD_FOOD], aaiTopBuildImmediateYield[iRank][YIELD_PRODUCTION], aaiTopBuildImmediateYield[iRank][YIELD_COMMERCE], aiTopBuildImmediateValue[iRank],
				aaiTopBuildFinalYield[iRank][YIELD_FOOD], aaiTopBuildFinalYield[iRank][YIELD_PRODUCTION], aaiTopBuildFinalYield[iRank][YIELD_COMMERCE], aiTopBuildFinalValue[iRank], aiTopBuildMaturedValue[iRank], aiTopBuildValue[iRank]);
		}
	}
	if (!bLogCandidates)
		kSet.cachePlotPotentialYield(p, iBestValue, eBestImprovement, aiBestYield, iTimingPercent);
	return iBestValue;
}


// <!-- custom: bTreatHomeAsCity preserves the usual city-center self-sustaining treatment by default, but evaluateBestPotentialPlotYield disables it when measuring the workable yield sacrificed by founding on the home plot. (GPT-5.6-Sol) -->
int AIFoundValue::evaluateYield(int const* aiYield, CvPlot const* p, bool bCanNeverImprove, bool bTreatHomeAsCity) const
{
	int r = 0;
	static const int iSelfSustainingExtraValue = GC.getDefineINT("SAS_EVALUATE_NATURE_YIELD_SELF_SUSTAINING_EXTRA_VALUE");
	static const int iDefaultFoodValue = GC.getDefineINT("SAS_EVALUATE_NATURE_YIELD_DEFAULT_FOOD_VALUE");
	static const int iDefaultProductionValue = GC.getDefineINT("SAS_EVALUATE_NATURE_YIELD_DEFAULT_PRODUCTION_VALUE");
	static const int iDefaultCommerceValue = GC.getDefineINT("SAS_EVALUATE_NATURE_YIELD_DEFAULT_COMMERCE_VALUE");
	static const int iSelfSustainingFoodValue = GC.getDefineINT("SAS_EVALUATE_NATURE_YIELD_SELF_SUSTAINING_FOOD_VALUE");
	static const int iSelfSustainingProductionValue = GC.getDefineINT("SAS_EVALUATE_NATURE_YIELD_SELF_SUSTAINING_PRODUCTION_VALUE");
	static const int iSelfSustainingCommerceValue = GC.getDefineINT("SAS_EVALUATE_NATURE_YIELD_SELF_SUSTAINING_COMMERCE_VALUE");
	static const int iOneFoodShortFoodValue = GC.getDefineINT("SAS_EVALUATE_NATURE_YIELD_ONE_FOOD_SHORT_FOOD_VALUE");
	static const int iOneFoodShortProductionValue = GC.getDefineINT("SAS_EVALUATE_NATURE_YIELD_ONE_FOOD_SHORT_PRODUCTION_VALUE");
	static const int iOneFoodShortCommerceValue = GC.getDefineINT("SAS_EVALUATE_NATURE_YIELD_ONE_FOOD_SHORT_COMMERCE_VALUE");
	// <!-- custom: Externalize AdvCiv's plot-yield weights so Food/Production/Commerce priorities can be tuned without recompiling.
	// Natural yields and ordinary improvement outcomes use these weights; resource-improvement yield remains separately valued by SAS_EVALUATE_BFC_BONUS_IMPROVEMENT_*. (GPT-5.5 + GPT-5.6-Sol) -->
	int aiWeight[NUM_YIELD_TYPES] = {iDefaultFoodValue, iDefaultProductionValue, iDefaultCommerceValue};
	// (note: these numbers have been adjusted for K-Mod)
	if (p != NULL && !p->isWater() && // advc.031: Exclude seafood
		((bTreatHomeAsCity && isHome(*p)) || aiYield[YIELD_FOOD] >= GC.getFOOD_CONSUMPTION_PER_POPULATION()))
	{
		r += iSelfSustainingExtraValue;
		aiWeight[YIELD_FOOD] = iSelfSustainingFoodValue;
		aiWeight[YIELD_PRODUCTION] = iSelfSustainingProductionValue;
		aiWeight[YIELD_COMMERCE] = iSelfSustainingCommerceValue;
		/*if (kSet.isStartingLoc())
			r *= 2;*/ // BtS
	}
	else if (aiYield[YIELD_FOOD] == GC.getFOOD_CONSUMPTION_PER_POPULATION() - 1)
	{
		aiWeight[YIELD_FOOD] = iOneFoodShortFoodValue;
		aiWeight[YIELD_PRODUCTION] = iOneFoodShortProductionValue;
		aiWeight[YIELD_COMMERCE] = iOneFoodShortCommerceValue;
	}
	// <!-- custom: Disabled the first-two-cities Commerce bonus. Commerce is already valued by every set above and is not uniquely important, while early Food and Production drive growth, workers, settlers, defense, and military. City sites persist into later eras, so changing their Commerce valuation only during the first era is not a coherent long-term settlement criterion; use one consistent value instead of this extra complexity. (GPT-5.5) -->
	// /*  <advc.108> For moving the starting Settler and for more
	// 	early-game commerce in general */
	// if(iCities <= 1 && eEra <= 0)
	// 	aiWeight[YIELD_COMMERCE] += 5; // </advc.108>
	// <!-- custom: Disabled the small Barbarian-specific production bias. Barbarian cities still need Food for growth and Commerce for research/economy, while their military priorities are handled elsewhere; these minor value changes did not justify the extra complexity of a separate tile-yield valuation layer. (GPT-5.5) -->
	// <advc.303>
	// if (bBarbarian)
	// {
	// 	aiWeight[YIELD_FOOD] -= 4;
	// 	aiWeight[YIELD_PRODUCTION] += 5;
	// 	aiWeight[YIELD_COMMERCE] -= 3;
	// } // </advc.303>
	FOR_EACH_ENUM(Yield)
	{
		aiWeight[eLoopYield] = (aiWeight[eLoopYield] * getSASEvaluateYieldValuePercent(eLoopYield)) / 100;
		FAssert(aiWeight[eLoopYield] >= 0); // advc.303
		int iYieldValue = aiYield[eLoopYield] * aiWeight[eLoopYield];
		/*	<advc.031> Mined resources (-1 production) get a too low value otherwise.
			See the comment at the end of getBonusImprovement. */
		if (iYieldValue < 0)
			iYieldValue /= 2; // </advc.031>
		r += iYieldValue;
	}
	if (p != NULL && p->isWater())
	{
		// K-Mod. kludge to account for lighthouse and lack of improvements.
		if (bCoastal)
		{
			r /= 2;
			r += 8 * (aiYield[YIELD_COMMERCE] + aiYield[YIELD_PRODUCTION]);
		}
		else r /= 3;
		// <!-- custom: disable non coastal penalty, let ai decide if it wants coast or not and not induce a bias in it.
		// With our changes to how ai evaluate terrains and features in evaluate function, AIs should be able to properly assess is coast is better (as of now for low-food environments or if there are water bonuses nearby which they were not so good if not at all able to do before), and if not able we'd better do it there rather than here and complexify formula.
		// Also, generally coastal start is a bit weaker, so give AI a chance to go for land starter with more plots to grow, perhaps more forests to chop, unless it assesses it best to not do otherwise. -->
		// if (kSet.isStartingLoc() && !bCoastal)
		// {
		// 	r -= 75; // advc.031: was -400 in BtS, -120 in K-Mod
		// 	/*	(K-Mod comment: "I'm pretty much forbidding starting 1 tile
		// 		inland non-coastal with more than a few non-lake water tiles.) */
		// }
	}  // <advc.031>
	if (bCanNeverImprove)
		r = (r * 77) / 100; // </advc.031>
	return r;
}


int AIFoundValue::evaluateFreshWater(CvPlot const& p, int const* aiYield, bool bSteal, int& iRiverTiles, int& iGreenTiles) const
{
	int iR = 0;
	// <advc.053>
	bool bLowYield = (aiYield[YIELD_FOOD] + aiYield[YIELD_PRODUCTION] +
			aiYield[YIELD_COMMERCE] / 2 < 2); // </advc.053>
	if (p.isRiver())
	{
		//iR += 10; // BtS
		// <K-Mod>
		//iR += (kSet.isExtraYieldThreshold() || kSet.isStartingLoc()) ? 30 : 10;
		//iR += (pPlot->isRiver() ? 15 : 0); // </K-Mod>
		// advc: Replaced by the code below
		// <advc.053>
		if (bLowYield)
		{
			if (kSet.isExtraYieldThreshold())
				iR += 9;
			// advc.908a>
			if (kSet.isExtraYieldNaturalThreshold())
				iR += 5; // </advc.908a>
		}
		else // </advc.053>
		{
			// <advc.031>
			if (kSet.isExtraYieldThreshold())
				iR += 13;
			// <advc.908a>
			if (kSet.isExtraYieldNaturalThreshold())
				iR += 8; // </advc.908a>
			if (!bSteal)
			{
				iRiverTiles++;
				if (iCities <= 0) // advc.108
					iR += 10;
				/*  I'm guessing this K-Mod clause is supposed to
					steer the AI toward settling at rivers rather
					than trying to make all river plots workable. */
				if (kPlot.isRiver())
				{
					iR += (/* advc.108: */ iCities <= 0
							? 10 : 4);
				}
			} // </advc.031>
		}
	}  // in addition:
	if (p.canHavePotentialIrrigation() && /* advc.053: */ !bLowYield)
	{
		// <advc.031> was 5/5
		iR += 4;
		if (p.isFreshWater())
		{
			iR += 6;
			if (aiYield[YIELD_FOOD] >= GC.getFOOD_CONSUMPTION_PER_POPULATION() &&
				!bSteal)
			{
				iGreenTiles++;
			}
		} // <advc.031>
	}
	return iR;
}


int AIFoundValue::applyCultureModifier(CvPlot const& p, int iPlotValue, int iCultureModifier, bool bShare) const
{
	scaled r = iPlotValue;
	r *= per100(iCultureModifier);
	// <advc.031>
	if (bShare) // bSteal is already factored into iCultureModifier
		r *= fixp(0.375);
	// </advc.031>
	return r.round();
}

bool AIFoundValue::isPlotInKnownRivalFutureBFC(CvPlot const& p) const
{
	for (PlayerIter<CIV_ALIVE, KNOWN_TO> it(eTeam); it.hasNext(); ++it)
	{
		CvPlayer const& kLoop = *it;
		if (kLoop.getTeam() == eTeam)
			continue;
		if (kLoop.isBarbarian())
			continue;
		if (GET_TEAM(kLoop.getTeam()).isVassal(eTeam))
			continue;

		FOR_EACH_CITY(pCity, kLoop)
		{
			// <!-- custom: kArea belongs to the new candidate city, not to bonus plot p. Distance alone identifies rival cities whose future BFC can contest p, including a coastal city contesting a water bonus. See KI#484. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			if (!kSet.isAllSeeing() && !kTeam.AI_deduceCitySite(*pCity))
				continue;
			if (plotDistance(p.getX(), p.getY(), pCity->getX(), pCity->getY()) <= CITY_PLOTS_RADIUS)
				return true;
		}
	}
	return false;
}


bool AIFoundValue::isBonusOwnedOrClaimedByFutureBFC(BonusTypes eBonus) const
{
	// <!-- custom: Available bonuses are net of exports and include temporary imports. Reconstruct connected permanent supply so an import cannot erase the settlement value of acquiring the first owned copy, while an exported owned copy still counts. See KI#486. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	int const iPermanentConnectedBonuses = kPlayer.getNumAvailableBonuses(eBonus) + kPlayer.getBonusExport(eBonus) - kPlayer.getBonusImport(eBonus);
	if (iPermanentConnectedBonuses > 0)
		return true;

	// <!-- custom: For empire-wide bonus effects, treat a bonus type as already covered when an existing city can plausibly claim it in its full BFC, even before border expansion or connection.
	// In save file 450, after uMgungundlovu correctly moved to (17,40), Zulu city 3 still settled far south for Crab + Camel; uMgungundlovu will safely claim Crab, so chasing another Crab should not beat the closer western Maize site near the capital. Camel is contested by Arabia, and only one side will get it, so keep its duplicate value when a known rival future BFC also threatens the plot.
	// Local tile/improvement yield remains separate: settling near a contested bonus can still be worthwhile because we may win or steal the tile, but empire-wide duplicate value should only be suppressed when ownership is reliable. See KI#178. (GPT-5.5) -->
	FOR_EACH_CITYAI(pCity, kPlayer)
	{
		for (CityPlotTypes ePlot = FIRST_ADJACENT_PLOT; ePlot < NUM_CITY_PLOTS; ++ePlot)
		{
			CvPlot const* pLoopPlot = plotCity(pCity->getX(), pCity->getY(), ePlot);
			if (pLoopPlot == NULL || pLoopPlot->getBonusType(kPlayer.getTeam()) != eBonus)
				continue;

			// <!-- custom: The bonus is team-known, but ownership can change under fog; use remembered ownership so hidden border changes cannot alter settlement value. See KI#496. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
			PlayerTypes const ePlotOwner = getRevealedOwner(*pLoopPlot);
			if (ePlotOwner == NO_PLAYER || ePlotOwner == ePlayer)
			{
				// <!-- custom: A contested occurrence establishes nothing about other copies. Keep searching until any matching future-BFC bonus is safely claimable, making this empire-wide result independent of city/plot iteration order. See KI#485. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
				if (!isPlotInKnownRivalFutureBFC(*pLoopPlot))
					return true;
				continue;
			}
			if (GET_PLAYER(ePlotOwner).getTeam() == kPlayer.getTeam())
				return true;
		}
	}
	return false;
}


// Note: aiBonusCount is only a partial count (resources evaluated up to this point)
bool AIFoundValue::isBonusBuildingEffectValued(BuildingTypes eBuilding, int iMaxPrereqEra, bool bCoastal) const
{
	CvBuildingInfo const& kBuilding = GC.getInfo(eBuilding);
	// <!-- custom: Settlement scoring should anticipate ordinary Granary/Harbor-style and conservatively estimated special-building bonus health/happiness without importing full AI_bonusVal trade logic. Keep obsolete buildings, non-coastal water buildings, and prerequisites beyond the configured era out of this narrow long-term city-site value. Limited buildings are rejected by the caller. See KI#178. (GPT-5.5 + GPT-5.6-Sol) -->
	if (kBuilding.isWater() && !bCoastal)
		return false;
	TechTypes const eObsoleteTech = kBuilding.getObsoleteTech();
	if (eObsoleteTech != NO_TECH && GET_TEAM(eTeam).isHasTech(eObsoleteTech))
		return false;
	TechTypes const ePrereqTech = kBuilding.getPrereqAndTech();
	if (ePrereqTech != NO_TECH && GC.getInfo(ePrereqTech).getEra() > iMaxPrereqEra)
		return false;
	for (int i = 0; i < kBuilding.getNumPrereqAndTechs(); i++)
	{
		TechTypes const eAndTech = kBuilding.getPrereqAndTechs(i);
		if (eAndTech != NO_TECH && GC.getInfo(eAndTech).getEra() > iMaxPrereqEra)
			return false;
	}
	SpecialBuildingTypes const eSpecialBuilding = kBuilding.getSpecialBuildingType();
	if (eSpecialBuilding != NO_SPECIALBUILDING)
	{
		CvSpecialBuildingInfo const& kSpecialBuilding = GC.getInfo(eSpecialBuilding);
		TechTypes const eSpecialObsoleteTech = kSpecialBuilding.getObsoleteTech();
		if (eSpecialObsoleteTech != NO_TECH && GET_TEAM(eTeam).isHasTech(eSpecialObsoleteTech))
			return false;
		TechTypes const eSpecialPrereqTech = kSpecialBuilding.getTechPrereq();
		if (eSpecialPrereqTech != NO_TECH && GC.getInfo(eSpecialPrereqTech).getEra() > iMaxPrereqEra)
			return false;
		TechTypes const eSpecialPrereqAnyoneTech = kSpecialBuilding.getTechPrereqAnyone();
		if (eSpecialPrereqAnyoneTech != NO_TECH && GC.getInfo(eSpecialPrereqAnyoneTech).getEra() > iMaxPrereqEra)
			return false;
	}
	return true;
}


int AIFoundValue::calculateBonusBuildingHappyHealthValue(BonusTypes eBonus, bool bCoastal) const
{
	static const int iMaxPrereqEra = GC.getDefineINT("SAS_EVALUATE_NON_YIELD_BONUS_BUILDING_EFFECT_MAX_PREREQ_ERA");
	if (iMaxPrereqEra < 0)
		return 0;
	static const int iHealthValue = GC.getDefineINT("SAS_EVALUATE_NON_YIELD_BONUS_HEALTH_VALUE");
	static const int iHappinessValue = GC.getDefineINT("SAS_EVALUATE_NON_YIELD_BONUS_HAPPINESS_VALUE");
	static const int iReligiousSpecialBuildingValuePerPoint = std::max(0, GC.getDefineINT("SAS_EVALUATE_NON_YIELD_BONUS_RELIGIOUS_SPECIAL_BUILDING_HAPPY_HEALTH_VALUE_PER_POINT"));
	static const int iNonReligiousSpecialBuildingValuePerPoint = std::max(0, GC.getDefineINT("SAS_EVALUATE_NON_YIELD_BONUS_NON_RELIGIOUS_SPECIAL_BUILDING_HAPPY_HEALTH_VALUE_PER_POINT"));
	static const int iSpecialBuildingMaxValue = std::max(0, GC.getDefineINT("SAS_EVALUATE_NON_YIELD_BONUS_SPECIAL_BUILDING_HAPPY_HEALTH_MAX_VALUE"));
	int iValue = 0;
	int iReligiousSpecialBuildingHealthPoints = 0;
	int iReligiousSpecialBuildingHappinessPoints = 0;
	int iNonReligiousSpecialBuildingHealthPoints = 0;
	int iNonReligiousSpecialBuildingHappinessPoints = 0;
	CvCivilization const& kCiv = kPlayer.getCivilization();
	for (int i = 0; i < kCiv.getNumBuildings(); i++)
	{
		BuildingClassTypes const eBuildingClass = kCiv.buildingClassAt(i);
		if (GC.getInfo(eBuildingClass).isLimited())
			continue;
		BuildingTypes const eBuilding = kCiv.buildingAt(i);
		if (!isBonusBuildingEffectValued(eBuilding, iMaxPrereqEra, bCoastal))
			continue;
		CvBuildingInfo const& kBuilding = GC.getInfo(eBuilding);
		int const iHealthPoints = std::max(0, kBuilding.getBonusHealthChanges(eBonus));
		int const iHappinessPoints = std::max(0, kBuilding.getBonusHappinessChanges(eBonus));
		SpecialBuildingTypes const eSpecialBuilding = kBuilding.getSpecialBuildingType();
		if (eSpecialBuilding == NO_SPECIALBUILDING)
			iValue += iHealthPoints * iHealthValue + iHappinessPoints * iHappinessValue;
		else
		{
			bool const bReligionLinked = (kBuilding.getReligionType() != NO_RELIGION || kBuilding.getPrereqReligion() != NO_RELIGION || kBuilding.getStateReligion() != NO_RELIGION || kBuilding.getHolyCity() != NO_RELIGION);
			if (bReligionLinked)
			{
				iReligiousSpecialBuildingHealthPoints += iHealthPoints;
				iReligiousSpecialBuildingHappinessPoints += iHappinessPoints;
			}
			else
			{
				iNonReligiousSpecialBuildingHealthPoints += iHealthPoints;
				iNonReligiousSpecialBuildingHappinessPoints += iHappinessPoints;
			}
		}
	}
	int const iReligiousSpecialBuildingValue = (iReligiousSpecialBuildingHealthPoints + iReligiousSpecialBuildingHappinessPoints) * iReligiousSpecialBuildingValuePerPoint;
	int const iNonReligiousSpecialBuildingValue = (iNonReligiousSpecialBuildingHealthPoints + iNonReligiousSpecialBuildingHappinessPoints) * iNonReligiousSpecialBuildingValuePerPoint;
	int const iSpecialBuildingRawValue = iReligiousSpecialBuildingValue + iNonReligiousSpecialBuildingValue;
	int const iSpecialBuildingValue = std::min(iSpecialBuildingRawValue, iSpecialBuildingMaxValue);
	iValue += iSpecialBuildingValue;
	IFLOG if (iSpecialBuildingRawValue != 0) logBBAI("Special-building bonus (%S): religiousHealth=%d religiousHappiness=%d nonReligiousHealth=%d nonReligiousHappiness=%d religiousValue=%d nonReligiousValue=%d raw=%d cap=%d value=%d",
		GC.getInfo(eBonus).getDescription(), iReligiousSpecialBuildingHealthPoints, iReligiousSpecialBuildingHappinessPoints,
		iNonReligiousSpecialBuildingHealthPoints, iNonReligiousSpecialBuildingHappinessPoints, iReligiousSpecialBuildingValue,
		iNonReligiousSpecialBuildingValue, iSpecialBuildingRawValue, iSpecialBuildingMaxValue, iSpecialBuildingValue);
	return iValue;
}


int AIFoundValue::calculateBonusConnectionEra(BonusTypes eBonus, CvPlot const& p) const
{
	CvBonusInfo const& kBonus = GC.getInfo(eBonus);
	TechTypes const eTradeTech = kBonus.getTechCityTrade();
	TechTypes const eRevealTech = kBonus.getTechReveal();
	int const iTradeEra = (eTradeTech == NO_TECH ? 0 : GC.getInfo(eTradeTech).getEra());
	int const iRevealEra = (eRevealTech == NO_TECH ? 0 : GC.getInfo(eRevealTech).getEra());
	FeatureTypes const eFeature = p.getFeatureType();
	int iEarliestConnectionEra = MAX_INT;
	FOR_EACH_ENUM(Build)
	{
		CvBuildInfo const& kLoopBuild = GC.getInfo(eLoopBuild);
		ImprovementTypes const eImprovement = kLoopBuild.getImprovement();
		if (eImprovement == NO_IMPROVEMENT)
			continue;
		CvImprovementInfo const& kImprovement = GC.getInfo(eImprovement);
		if (kImprovement.isActsAsCity() || !kImprovement.isImprovementBonusMakesValid(eBonus) || !kImprovement.isImprovementBonusTrade(eBonus))
			continue;
		TechTypes const eBuildTech = kLoopBuild.getTechPrereq();
		TechTypes const eFeatureTech = (eFeature == NO_FEATURE ? NO_TECH : kLoopBuild.getFeatureTech(eFeature));
		int const iBuildEra = (eBuildTech == NO_TECH ? 0 : GC.getInfo(eBuildTech).getEra());
		int const iFeatureEra = (eFeatureTech == NO_TECH ? 0 : GC.getInfo(eFeatureTech).getEra());
		int const iConnectionEra = std::max(std::max(iTradeEra, iRevealEra), std::max(iBuildEra, iFeatureEra));
		iEarliestConnectionEra = std::min(iEarliestConnectionEra, iConnectionEra);
	}
	FAssertMsg(iEarliestConnectionEra != MAX_INT, "Every visible bonus should have a non-Fort improvement that can connect it");
	return (iEarliestConnectionEra == MAX_INT ? GC.getNumEraInfos() - 1 : iEarliestConnectionEra);
}


int AIFoundValue::nonYieldBonusValue(CvPlot const& p, BonusTypes eBonus, bool bCanTrade, bool bEasyAccess, bool bCoastal, std::vector<int>* paiBonusCount, int iCultureModifier, int* piHappyHealthValue, int* piBuildingHappyHealthValue, int* piAdjustPercent, int* piWaterPenalty) const
{
	// <!-- custom: In save file 450, the old broad AI_bonusVal path made Elephants at (20,41) greatly outscore nearby Crab, pushing Shaka's uMgungundlovu toward the weaker (18,40) site over the Crab and greener (17,40) alternative; this is wrong because Crab is locally a valuable high food source, and its empire health gain remains useful especially later, while Elephants are mostly classical-era pressure with some happiness. Settlement scoring cares less about immediate current-era trade value than about long-term health and happiness city-site value, which can be very different.
	// Score explicit new health/happiness, ordinary building effects, and conservatively estimated special-building effects here. The old first-growth/luxury extra is removed because explicit health/happiness already represents that empire-wide effect. Diversity, strategic AIObjective, and improvement yields are handled separately by the caller. See KI#178. (GPT-5.5 + GPT-5.6-Sol) -->
	// int r = kPlayer.AI_bonusVal(eBonus, 1, true) * (!kSet.isStartingLoc() && kPlayer.getNumTradeableBonuses(eBonus) == 0 && aiBonusCount[eBonus] == 1 ? 80 : 20); // BtS
	// int iCount = kPlayer.getNumTradeableBonuses(eBonus) == 0 + aiBonusCount[eBonus];
	// int r = AI_bonusVal(eBonus, 0, true) * 80 / (1 + 2*iCount); // K-Mod
	// <advc.031> The "==0" looks like an error. Rather than correct that, I'll let AI_bonusVal handle the number of bonuses already connected (iChange=1). A division by NumTradeableBonuses here won't work well for strategic resources.
	// Coefficient was 80; active AdvCiv-SAS used 57 before replacing this path.
	// K-Mod: try not to make the value of strategic resources too overwhelming. (note: I removed a bigger value reduction from the original code.)
	// <advc.031> High values for strategic resources remain a problem during the early game
	// (note): Instead of special treatment just for the early game, the multiplier should arguably be based on an estimate of how many cities we'll have in some medium term - b/c AI_bonusVal is per city.
	// (cf. getBonusImprovement)
	// AI_bonusValue can check for tech requirements, but it can't check requirements for building the improvement. Hence bAssumeEnabled=true is used in the AI_bonusValue call above and we take care of tech requirements ourselves.
	// <!-- custom: The old coefficient, early-game modifier, strategic-resource dampening, and AI_bonusValue/bAssumeEnabled tech-requirement handling belonged to the old AI_bonusVal path commented out below. That path was replaced with explicit health/happiness settlement value; strategic need remains handled through AIObjective, and improvement availability remains handled through getBonusImprovement and separate bonus-yield scoring. See KI#178. (GPT-5.5) -->
	static const int iHealthValue = GC.getDefineINT("SAS_EVALUATE_NON_YIELD_BONUS_HEALTH_VALUE");
	static const int iHappinessValue = GC.getDefineINT("SAS_EVALUATE_NON_YIELD_BONUS_HAPPINESS_VALUE");
	static const int iConnectionEraValueLossPercent = GC.getDefineINT("SAS_EVALUATE_NON_YIELD_BONUS_CONNECTION_ERA_VALUE_LOSS_PERCENT");
	CvBonusInfo const& kBonus = GC.getInfo(eBonus);
	int const iBuildingHappyHealthValue = calculateBonusBuildingHappyHealthValue(eBonus, bCoastal);
	int const iHappyHealthValue = kBonus.getHealth() * iHealthValue + kBonus.getHappiness() * iHappinessValue + iBuildingHappyHealthValue;
	int iWaterPenalty = 0;
	// if (piBaseBonusVal != NULL)
	// 	*piBaseBonusVal = iBaseBonusVal;
	// if (piMultiplier != NULL)
	// 	*piMultiplier = iMultiplier;
	// scaled r = iBaseBonusVal * iMultiplier;
	if (piAdjustPercent != NULL)
		*piAdjustPercent = 100;
	if (piWaterPenalty != NULL)
		*piWaterPenalty = iWaterPenalty;
	bool bSurplus = isBonusOwnedOrClaimedByFutureBFC(eBonus);
	if (paiBonusCount != NULL)
	{
		if ((*paiBonusCount)[eBonus] > 0)
		{
			// r /= 1 + (*paiBonusCount)[eBonus];
			bSurplus = true;
		} // </advc.031>
		(*paiBonusCount)[eBonus]++;
	}
	// // <advc.031> (Would be cleaner to handle this in CvPlayerAI::AI_baseBonusVal.)
	// bool bGrowthBonus = (!bAnyGrowthBonus && bCanTradeSoon && !bSurplus &&
	// 		(GC.getInfo(eBonus).getHappiness() +
	// 		// Basically only luxury resources qualify
	// 		GC.getInfo(eBonus).getHealth() / 2 > 0) &&
	// 		(bCanTrade || p.getFeatureType() == NO_FEATURE ||
	// 		GC.getInfo(p.getFeatureType()).getHealthPercent() >= 0));
	// if (bGrowthBonus) // Reward only the first new luxury
	// 	bAnyGrowthBonus = true;
	// // </advc.031>
	// /*  K-Mod: try not to make the value of strategic resources too overwhelming. (note: I removed a bigger value reduction from the original code.) */
	// scaled rEarlyGameModifier = 1;
	// if (kSet.isStartingLoc())
	// {	// <advc.031>
	// 	if (bGrowthBonus)
	// 		rEarlyGameModifier = fixp(0.4); // </advc.031>
	// 	else
	// 	{
	// 		// (advc: Divisor was 4 in K-Mod; BtS had divided by 2 after evaluateSpecialYields.)
	// 		rEarlyGameModifier = 
	// 			/*  <advc.108> Don't need to decrease as much if reveal-techs are respected (also: advc.036 improves the evaluation of non-strategic resources) */
	// 				(kGame.getStartingPlotNormalizationLevel() <= CvGame::NORMALIZE_LOW ?
	// 				fixp(1/3.) : fixp(1/4.));
	// 	}
	// }
	// else if (iCities <= 0) // For moving the starting Settler and normalization
	// {
	// 	if (bGrowthBonus)
	// 		rEarlyGameModifier = fixp(0.75);
	// 	else rEarlyGameModifier = fixp(0.55);
	// } // </advc.108>
	// // <advc.031> High values for strategic resources remain a problem during the early game
	// /*	(note): Instead of special treatment just for the early game, the multiplier should arguably be based on an estimate of how many cities we'll have in some medium term - b/c AI_bonusVal is per city. */
	// else
	// {
	// 	int const iTargetCities = GC.getInfo(GC.getMap().getWorldSize()).getTargetNumCities();
	// 	if (iCities + 1 < iTargetCities)
	// 	{
	// 		FAssert(iTargetCities >= 3);
	// 		if (bGrowthBonus) // Modifier climbs to 120%, then decreases to 100%.
	// 		{
	// 			int iPeakCity = (iTargetCities + 1) / 2;
	// 			scaled rIncrement = fixp(0.3) / std::max(1, iPeakCity - 1);
	// 			rEarlyGameModifier = fixp(0.9) + rIncrement *
	// 					(iCities < iPeakCity ? iCities : iTargetCities - iCities - 1);
	// 		}
	// 		else // Modifier climbs to 100%
	// 		{
	// 			scaled rIncrement = fixp(0.5) / std::max(1, iTargetCities - 1);
	// 			rEarlyGameModifier = fixp(0.55) + rIncrement * iCities;
	// 		}
	// 	}
	// }
	// IFLOG if(rEarlyGameModifier.getPercent()!=100) logBBAI("Early-game modifier for non-yield resource value: %d percent", rEarlyGameModifier.getPercent());
	// if (piEarlyPercent != NULL)
	// 	*piEarlyPercent = rEarlyGameModifier.getPercent();
	// r *= rEarlyGameModifier;
	// // (cf. getBonusImprovement) AI_bonusValue can check for tech requirements, but it can't check requirements for building the improvement. Hence bAssumeEnabled=true is used in the AI_bonusValue call above and we take care of tech requirements ourselves.

	if (!bCanTrade && !bSurplus)
	{
		// Important for high-value strategic bonuses that get revealed long before they can be traded, especially Oil. Scanning the whole map through CvPlayerAI::AI_countOwnedBonuses is too expensive I think, but I'm copying the city bonus count from there.
		FOR_EACH_CITYAI(pCity, kPlayer)
		{
			if (pCity->AI_countNumBonuses(eBonus, true, true, -1) > 0)
			{
				bSurplus = true;
				break;
			}
		}
	}
	int const iEffectiveHappyHealthValue = (bSurplus ? 0 : iHappyHealthValue);
	if (piHappyHealthValue != NULL)
		*piHappyHealthValue = iEffectiveHappyHealthValue;
	if (piBuildingHappyHealthValue != NULL)
		*piBuildingHappyHealthValue = (bSurplus ? 0 : iBuildingHappyHealthValue);
	scaled r = iEffectiveHappyHealthValue;
	scaled rAdjustment = (r == 0 ? 0 : 1);
	// <!-- custom: Base AdvCiv gave only 70% value when a bonus was connectable through a currently researchable technology and about one third otherwise. That immediate-availability split was too harsh and arbitrary for long-term settlement value: save file 453 gave Incense only 70% immediately before Spain completed same-era Calendar on the founding turn.
	// Keep full value when the earliest valid connection is in the current/earlier era, and lose an XML-tunable percent only per later era. Surplus duplicates remain zero because local improvement yields and diversity are scored separately. See KI#187. (GPT-5.6-Sol) -->
	if (!bCanTrade)
	{
		IFLOG if(bSurplus) logBBAI("Surplus bonus");
		if (bSurplus)
			r = 0;
		else if (r != 0)
		{
			int const iConnectionEra = calculateBonusConnectionEra(eBonus, p);
			int const iConnectionEraDistance = std::max(0, iConnectionEra - (int)eEra);
			int const iConnectionEraAdjustPercent = std::max(0, 100 - iConnectionEraDistance * iConnectionEraValueLossPercent);
			rAdjustment *= scaled(iConnectionEraAdjustPercent, 100);
			IFLOG logBBAI("Bonus connection timing (%S): currentEra=%d connectionEra=%d eraDistance=%d lossPerEra=%d timingPercent=%d",
				GC.getInfo(eBonus).getDescription(), eEra, iConnectionEra, iConnectionEraDistance, iConnectionEraValueLossPercent,
				iConnectionEraAdjustPercent);
		}
		// <advc.040>
		if (!bEasyAccess)
		{
			/*  Might be better to place a city in p.area(). But if that area is tiny, then accessing the resource from a different landmass is probably our best bet. */
			rAdjustment *= (p.getArea().getNumTiles() <= 2 ? fixp(0.6) : fixp(0.45));
		} // </advc.040>
	}
	r *= rAdjustment;
	if (piAdjustPercent != NULL)
		*piAdjustPercent = rAdjustment.getPercent();
	if (kSet.isStartingLoc() /* advc.031e: */ || kSet.isNormalizing())
		return r.round();

	// K-Mod. (original code deleted)
	if (!isHome(p))
	{
		// <advc.031> Why halve the value of water bonuses? Perhaps because they're costly to improve. But that's only true in the early game. Because they tend to be common? AI_bonusVal takes care of that.
		if (p.isWater()/*) {//r /= 2;*/ && eEra < CvEraInfo::AI_getAgeOfExploration())
		{
			iWaterPenalty = (CvEraInfo::AI_getAgeOfExploration() - eEra) * 16;
			r -= iWaterPenalty;
			r.increaseTo(0);
			if (piWaterPenalty != NULL)
				*piWaterPenalty = iWaterPenalty;
			IFLOG logBBAI("Penalty for water bonus: %d", iWaterPenalty);
		}
		// iCultureModifier should have this covered
		/*if (getRevealedOwner(p) != ePlayer && ::stepDistance(&kPlot, &p) > 1) {
			if (!kSet.isEasyCulture())
				r *= fixp(0.75);
		}*/
		// advc.031: Don't ignore culture modifier < 60
		scaled rModifier = (kSet.isAmbitious() && iCultureModifier >= 60 ?
				fixp(1.1) : per100(iCultureModifier));
		r *= rModifier;
		if (piAdjustPercent != NULL)
			*piAdjustPercent = (rAdjustment * rModifier).getPercent();
		IFLOG if(rModifier!=per100(iCultureModifier)) logBBAI("Non-yield bonus value increased b/c of ambitious personality");
	}
	else if (kSet.isAmbitious())
	{
		r *= fixp(1.1);
		if (piAdjustPercent != NULL)
			*piAdjustPercent = (rAdjustment * fixp(1.1)).getPercent();
	}
	// K-Mod end
	return r.round();
}


int AIFoundValue::calculateSpecialYieldModifier(int iCultureModifier, bool bEasyAccess, bool bBonus, bool bCanSoonImproveBonus, bool bCanImproveBonus) const
{
	scaled r = iCultureModifier;
	// </advc.040>
	if (!bEasyAccess)
		r /= 2; // <advc.040>
	// <advc.031>
	if (bBonus)
	{
		if (!bCanSoonImproveBonus)
		{
			// Don't want to discount e.g. Wine too much near capital
			r = (r * (kSet.isStartingLoc() ? 4 : 3)) / 10;	
		}
		else if (!bCanImproveBonus)
			r *= fixp(0.75);
	} // </advc.031>
	return r.round();
}


void AIFoundValue::calculateSpecialYields(CvPlot const& p, int const* aiBonusImprovementYield, int const* aiNatureYield, int iModifier, int* aiSpecialYield, int& iSpecialFoodPlus, int& iSpecialFoodMinus, int& iSpecialYieldTiles) const
{
	int aiBuildingYield[NUM_YIELD_TYPES];
	calculateBuildingYields(p, aiNatureYield, aiBuildingYield);
	int const iEffectiveFood = aiNatureYield[YIELD_FOOD] + aiBuildingYield[YIELD_FOOD];
	if (aiBonusImprovementYield == NULL) // K-Mod: non bonus related special food
	{
		int iSurplus = (std::max(0,
				iEffectiveFood - GC.getFOOD_CONSUMPTION_PER_POPULATION()) *
				per100(iModifier)).uround(); // advc.031
		iSpecialFoodPlus += iSurplus;
		aiSpecialYield[YIELD_FOOD] += iSurplus; // advc.031: A little extra love for Flood Plains
		return;
	}
	// advc.031: No need to recompute nature yield here
	int iSpecialFood = 
			((aiNatureYield[YIELD_FOOD] + aiBonusImprovementYield[YIELD_FOOD]) *
			per100(iModifier)).round(); // advc.031
	aiSpecialYield[YIELD_FOOD] += iSpecialFood;
	int iFoodTemp = iSpecialFood - GC.getFOOD_CONSUMPTION_PER_POPULATION();
	// advc.031:
	iFoodTemp = (iFoodTemp * per100(iModifier)).round();
	iSpecialFoodPlus += std::max(0, iFoodTemp);
	iSpecialFoodMinus -= std::min(0, iFoodTemp);
	// <advc.031>
	int iSpecialProd = aiNatureYield[YIELD_PRODUCTION] +
			aiBonusImprovementYield[YIELD_PRODUCTION] +
			aiBuildingYield[YIELD_PRODUCTION];
	int iSpecialComm = aiNatureYield[YIELD_COMMERCE] +
			aiBonusImprovementYield[YIELD_COMMERCE] +
			aiBuildingYield[YIELD_COMMERCE];
	/*  No functional change above. Now count the tile as special and
		apply culture modifier. */
	bool const bSpecial =  (iSpecialFood > 0 || iSpecialComm > 0 || iSpecialProd > 0);
	if (bSpecial)
		iSpecialYieldTiles++;
	/*  To avoid rounding all yields to 0, don't reduce production and commerce
		to less than 1. */
	iSpecialProd = std::max(std::min(1, iSpecialProd),
			(iSpecialProd * per100(iModifier)).round());
	aiSpecialYield[YIELD_PRODUCTION] += iSpecialProd;
	iSpecialComm = std::max(std::min(1, iSpecialComm),
			(iSpecialComm * per100(iModifier)).round());
	aiSpecialYield[YIELD_COMMERCE] += iSpecialComm;
	IFLOG if(bSpecial) logBBAI("Special yield: %dF%dP%dC (modifier: %d percent)",
			iSpecialFood, iSpecialProd, iSpecialComm, iModifier);
	// </advc.031>
}

// advc: Function for K-Mod's "lighthouse kludge"
void AIFoundValue::calculateBuildingYields(CvPlot const& p, int const* aiNatureYield, int* aiBuildingYield) const
{
	FOR_EACH_ENUM(Yield)
		aiBuildingYield[eLoopYield] = 0;
	if (bCoastal && p.isWater() &&
		aiNatureYield[YIELD_COMMERCE] > 1)
	{
		aiBuildingYield[YIELD_FOOD]++;
	}
}

// <!-- custom: Optional distribution outputs reuse the vector sorted by this function for first-city decisions and diagnostics; they do not replace or alter AdvCiv's complete weighted sum. (GPT-5.6-Sol) -->
/*	advc.031: Weighted sum. (Using floating-point math until such a time that a
	logarithm function gets added to ScaledNum.) */
int AIFoundValue::sumUpPlotValues(std::vector<int>& aiPlotValues, int* aiCoreSums, int* aiCoreCutoffs, int* piPositivePlots) const
{
	std::sort(aiPlotValues.begin(), aiPlotValues.end(), std::greater<int>());
	// CITY_HOME_PLOT should have 0 value here, others could have negative values.
	FAssert(aiPlotValues[NUM_CITY_PLOTS - 1] <= 0);
	bool const bLogDistribution = (gFoundLogLevel >= 3 && AIFoundValue::isLoggingEnabled());
	if (aiCoreSums != NULL || bLogDistribution)
	{
		FAssert((aiCoreSums == NULL) == (aiCoreCutoffs == NULL));
		FAssert((aiCoreSums == NULL) == (piPositivePlots == NULL));
		int aiSums[6] = {0, 0, 0, 0, 0, 0};
		int const aiCoreSizes[6] = {1, 2, 3, 6, 10, 14};
		int iPositivePlots = 0;
		FOR_EACH_ENUM(CityPlot)
		{
			int const iPlotValue = aiPlotValues[eLoopCityPlot];
			if (iPlotValue <= 0)
				break;
			iPositivePlots++;
			for (int iCore = 0; iCore < 6; ++iCore)
			{
				if (eLoopCityPlot < aiCoreSizes[iCore])
					aiSums[iCore] += iPlotValue;
			}
		}
		if (aiCoreSums != NULL)
		{
			for (int iCore = 0; iCore < 6; ++iCore)
			{
				aiCoreSums[iCore] = aiSums[iCore];
				aiCoreCutoffs[iCore] = aiPlotValues[aiCoreSizes[iCore] - 1];
			}
			*piPositivePlots = iPositivePlots;
		}
		// <!-- custom: The final weighted total distinguishes stronger plots implicitly, but these unweighted core sums expose the site's practical growth curve.
		// Best 1/2/3 distinguish one exceptional opening resource from several strong early plots for diagnosis only; first-city decisions deliberately remain on best 6/10 so one resource cannot drive the cure.
		// In the Wang Kon test, the original and chosen capitals tied at best-1=210, while best-2/3 separated 380/550 from 420/630; this showed that both had one equally strong anchor but the chosen site had the stronger immediate supporting plots.
		// A city works few plots through much of the game, so six strong early plots, ten developed-city plots or fourteen mature-city plots with weak outskirts can be more useful than twenty uniformly average plots; e.g. a fertile river core beside desert can outperform a broad tundra/plains BFC long before either city works every tile.
		// Keep the sums, cutoff values and positive-plot count in both verbose logs and compact first-city breakdowns so scouting can be tuned from the same evidence without changing site valuation. (GPT-5.6-Sol) -->
		if (bLogDistribution) logBBAI("BFC_VALUE_DISTRIBUTION best1Sum=%d best2Sum=%d best3Sum=%d best6Sum=%d best10Sum=%d best14Sum=%d positivePlots=%d first=%d second=%d third=%d sixth=%d tenth=%d fourteenth=%d",
			aiSums[0], aiSums[1], aiSums[2], aiSums[3], aiSums[4], aiSums[5], iPositivePlots, aiPlotValues[0], aiPlotValues[1], aiPlotValues[2], aiPlotValues[5], aiPlotValues[9], aiPlotValues[13]);
	}
	double dMaxMultPercent = 153;
	double dMinMultPercent = 47;
	if (iCities <= 0) // Capital will grow large
	{
		dMaxMultPercent -= 8;
		dMinMultPercent += 8;
	}
	double const dNormalizMult = 1;
	double const dSubtr = 29;
	double const dExp = std::log(dMaxMultPercent - dMinMultPercent) /
			std::log(NUM_CITY_PLOTS - 1.0);
	int iR = 0;
	FOR_EACH_ENUM(CityPlot)
	{
		int iPlotValue = aiPlotValues[eLoopCityPlot];
		if (iPlotValue > 0)
		{
			iPlotValue = std::max(iPlotValue / 3,
					fmath::round(std::max(0.0, iPlotValue - dSubtr) * 0.01 *
					// Linearly decreasing multiplier:
					/*(maxMultPercent - i * ((maxMultPercent -
					minMultPercent) / (NUM_CITY_PLOTS - 1))));*/
					// Try power law instead:
					dNormalizMult * (dMinMultPercent + std::pow((double)
					std::abs(NUM_CITY_PLOTS - 1 - eLoopCityPlot), dExp))));
		}
		iR += iPlotValue;
	}
	IFLOG logBBAI("Weighted sum of plot values:\n+%d", iR);
	return iR;
}

// <!-- custom: Disabled after our simple XML-tunable SAS bonus-improvement yield valuation made this path redundant. Keeping both paths overscored bonus-heavy sites during follow-up testing around KI#173, letting resource quantity overwhelm food and terrain quality. This nonlinear formula is also obscure, convoluted, and hardcoded, making it poor for tuning and clarity; keep it commented for reference. (GPT-5.5) -->
// /*	Note: aiSpecialYield includes aiNatureYield. Thus, nature yield is counted twice:
// 	once in evaluateYield, a second time in evaluateSpecialYield. This was already
// 	the case in BtS and it might work out more or less correctly on the bottom line,
// 	but it's messy. */
// int AIFoundValue::evaluateSpecialYields(int const* aiSpecialYield, int iSpecialYieldTiles, int iSpecialFoodPlus, int iSpecialFoodMinus) const
// {
// 	/*return iSpecialFood*50+iSpecialProduction*50+iSpecialCommerce*50;
// 	if (kSet.isStartingLoc())
// 		r /= 2;*/ // BtS
// 	/*	K-mod. It's tricky to get this right. Special commerce is great
// 		in the early game, but not so great later on. Food is always great -
// 		unless we already have too much.
// 		iSpecialFood is whatever food happens to be associated with bonuses.
// 		Don't value it highly, because it's also counted in a bunch of other ways. */
// 	//return iSpecialFood*20+iSpecialProduction*40+iSpecialCommerce*35;
// 	// <advc.031>
// 	scaled arWeight[NUM_YIELD_TYPES] = {
// 			// advc.108: So that less food gets placed during normalization - hopefully.
// 			kSet.isStartingLoc() ? fixp(0.45) :
// 			fixp(0.24),
// 			fixp(0.36),
// 			/*  advc.108: For moving the starting Settler. Though a commercial
// 				resource at the second city is also valuable, so: */
// 			iCities <= 1 && eEra <= 0 ? fixp(0.48) : fixp(0.32)};
// 	// <advc.031f> When there will be no normalization
// 	if (kSet.isStartingLoc() && kSet.isScenario())
// 	{
// 		arWeight[YIELD_FOOD] *= 2;
// 		arWeight[YIELD_PRODUCTION] *= fixp(1.7);
// 		arWeight[YIELD_COMMERCE] *= fixp(1.4);
// 	} // </advc.031f>
// 	scaled const rDiv = iSpecialYieldTiles;
// 	scaled rFromSpecial;
// 	FOR_EACH_ENUM(Yield)
// 		rFromSpecial += (aiSpecialYield[eLoopYield] / rDiv) * arWeight[eLoopYield];
// 	if(rFromSpecial > 0)
// 		rFromSpecial = rFromSpecial.pow(fixp(1.5)) * 75 * iSpecialYieldTiles;
// 	// advc.031: Apply the BtS/K-Mod food modifier only to the special yield value
// 	int iFoodSurplus = std::max(0, iSpecialFoodPlus - iSpecialFoodMinus);
// 	int iFoodDeficit = std::max(0, iSpecialFoodMinus - iSpecialFoodPlus);
// 	/*r *= 100 + 20 * std::max(0, std::min(iFoodSurplus, 2 * GC.getFOOD_CONSUMPTION_PER_POPULATION()));
// 	r /= 100 + 20 * std::max(0, iFoodDeficit);*/ // BtS
// 	// K-Mod. (note that iFoodSurplus and iFoodDeficit already have the "max(0, x)" built in.
// 	/*r *= 100 + (kSet.isExpansive() ? 20 : 15) * std::min(
// 			(iFoodSurplus + iSpecialFoodPlus)/2,
// 			2 * GC.getFOOD_CONSUMPTION_PER_POPULATION());
// 	r /= 100 + (kSet.isExpansive() ? 20 : 15) * iFoodDeficit;*/ // K-Mod end
// 	// Turn it into a single multiplier 'rFoodModifier' ...
// 	scaled rFoodWeight = fixp(0.15);
// 	if (kSet.isExpansive())
// 		rFoodWeight += fixp(0.05);
// 	scaled rSurplusMean(iFoodSurplus + iSpecialFoodPlus, 2);
// 	rSurplusMean.decreaseTo(2 * GC.getFOOD_CONSUMPTION_PER_POPULATION());
// 	scaled rFoodModifier = (1 + rFoodWeight * rSurplusMean) /
// 			(1 + rFoodWeight * iFoodDeficit);
// 	// ... and reduce the impact b/c of the new food modifier in adjustToFood
// 	if (!kSet.isStartingLoc())
// 		rFoodModifier = (rFoodModifier + 2) / 3;
// 	/*	Starting sites are exempt from adjustToFood. Mostly don't want them
// 		to be exempt from the special food adjustment. */
// 	else rFoodModifier = (rFoodModifier + fixp(0.5)) / fixp(1.5);
// 	int iResult = (rFromSpecial * rFoodModifier).round();
// 	IFLOG logBBAI("+%d from special yields %dF%dP%dC (food surplus modifier: %d percent)", iResult,
// 			aiSpecialYield[YIELD_FOOD], aiSpecialYield[YIELD_PRODUCTION], aiSpecialYield[YIELD_COMMERCE],
// 			rFoodModifier.getPercent());
// 	return iResult;
// 	// </advc.031>
// }


// <!-- custom: simplify logic and attempt to spread cities more, currently they are way too crowded which is inefficient -->
bool AIFoundValue::isTooManyTakenTiles(int iTaken, int iResourceValue) const
{
	// return (((iTaken > NUM_CITY_PLOTS / 3 ||
	// 		(bBarbarian && iTaken > 2)) &&// advc.303
	// 		iResourceValue < 250 && bLowValue) ||
	// 		/* <advc.031> */ (iTaken > (2 * NUM_CITY_PLOTS) / 3 &&
	// 		iResourceValue < 800)); // </advc.031>
	if (iResourceValue < 500)
	{
		return (iTaken > NUM_CITY_PLOTS / 6);
	}
	// <!-- custom: compete for bonuses by having a bit tighter placement, but still overall do not like to crowd cities too close to each other, is very inefficient, look for best tiels rather in other parts of the code if handled there i mean such as where we refactored/reworked logic or in other places if any, and aim for later big city size -->
	else
	{
		return (iTaken > NUM_CITY_PLOTS / 4);
	}
}

// <!-- custom: try to remove this interference as we have a finer algorithm now, and this old code may lead to unexpected results -->
/*	advc.031: Unlike the food modifier in evaluateSpecialYields, this modifier applies
	to all yields and takes into account grassland farms.*/
// int AIFoundValue::adjustToFood(int iValue, int iSpecialFoodPlus, int iSpecialFoodMinus,
// 	int iGreenTiles) const
// {
// 	scaled rLowFoodModifier = 1;
// 	if (eEra < CvEraInfo::AI_getAgeOfFertility())
// 	{
// 		int iSpecialSurplus = (iSpecialFoodPlus - iSpecialFoodMinus + 1) / 2; // ceil
// 		rLowFoodModifier = (fixp(8.5) + iGreenTiles + iSpecialSurplus) / fixp(11.5);
// 		rLowFoodModifier.clamp(fixp(0.5), 1);
// 	}
// 	IFLOG if(rLowFoodModifier.getPercent()!=100) logBBAI("Times %d percent for lack of food "
// 			"(special food +/-: %d/%d, green tiles: %d)", rLowFoodModifier.getPercent(),
// 			iSpecialFoodPlus, iSpecialFoodMinus, iGreenTiles);
// 	return (iValue * rLowFoodModifier).round();
// }

// (adjustToBadHealth deals with short-term health)
int AIFoundValue::evaluateLongTermHealth(int& iHealthPercent) const
{
	int iR = 0;
	int iFreshWaterHealth = 0;
	if (kPlot.isFreshWater())
	{
		iFreshWaterHealth = GC.getDefineINT(CvGlobals::FRESH_WATER_HEALTH_CHANGE);
		iHealthPercent += 100 * iFreshWaterHealth; // advc.031
	}
	//iValue += (iHealth / 5);
	/*  <advc.031> The above may have accounted for feature production; now
		evaluated separately elsewhere. */
	if (iHealthPercent > 0 || eEra >= CvEraInfo::AI_getAgeOfPestilence())
		iR += std::min(iHealthPercent, 350) / 6;
	// Extra bonus for persistent health (as in BtS/ K-Mod): // </advc.031> 
	iR += iFreshWaterHealth * 30;
	// (K-Mod (commented this out, compensated by the river bonuses I added.)
	/*if (iFreshWaterHealth > 0)
		r += 40;*/
	IFLOG if(iR!=0) logBBAI("+%d from %d/100 health", iR, iHealthPercent);
	return iR;
}

// advc.031:
int AIFoundValue::evaluateFeatureProduction(int iProduction) const
{
	/*  Can't chop in the very early game (would be nicer to check for
		feature removal tech and sufficient workers than to go by era) */
	scaled r = iProduction * 3;
	if (rAIEraFactor <= 0)
		r /= 4;
	else r /= rAIEraFactor + 2;
	IFLOG if(r!=0) logBBAI("+%d from %d feature production", r.round(), iProduction);
	return r.round();
}


int AIFoundValue::evaluateSeaAccess(bool bGoodFirstColony, scaled rProductionModifier, int iLandTiles) const
{
	int iR = 0;
	// <advc.303>
	if (bBarbarian)
	{
		// <!-- custom: Keep the inherited Barbarian coastal push XML-tunable. Coastal Barbarian cities can maintain Galley/naval pressure from lone islands, but too much coastal bias can outweigh stronger long-term city sites that Barbarians or later conquerors would use better; adjust SAS_EVALUATE_BARBARIAN_COASTAL_EXTRA_VALUE in XML. (GPT-5.5) -->
		static const int iSAS_EVALUATE_BARBARIAN_COASTAL_EXTRA_VALUE = GC.getDefineINT("SAS_EVALUATE_BARBARIAN_COASTAL_EXTRA_VALUE");
		iR += iSAS_EVALUATE_BARBARIAN_COASTAL_EXTRA_VALUE;
		IFLOG logBBAI("+%d for coastal (Barbarian)", iR);
		return iR;
	} // </advc.303>
	//if (kSet.isStartingLoc())
	if (/* advc.108: */ iCities <= 0)
	{
		// BtS: "let other penalties bring this down."
		iR += (kSet.isStartingLoc() ? 360 // advc.031: was 600 in BtS, 500 in K-Mod
				: 200); // advc.031: Less when normalizing or moving 1st settler
		if (!kSet.isNormalizing() && kArea.getNumStartingPlots() <= 0)
		{
			// advc.031: An inland sea will probably do an isolated civ no good
			if (GC.getMap().findBiggestArea(true) == kPlot.waterArea(true))
				iR += 450; // advc.031: was 1000 in BtS, 600 in K-Mod
		}
		IFLOG logBBAI("+%d for coastal (1st city)", iR);
		return iR;
	}
	if (kArea.getCitiesPerPlayer(ePlayer) <= 0)
	{
		//r += (iResourceValue > 0) ? 800 : 100; // (BtS)
		// K-Mod replacement:
		//r += iResourceValue > 0 ? (kSet.isSeafaring() ? 600 : 400) : 100;
		// advc.040: Mostly handled by the bFirstColony code elsewhere now
		if (bGoodFirstColony)
		{
			iR += 250;
			IFLOG logBBAI("+%d for promising first colony", iR);
		}
		return iR + 60; // advc.031: For trade routes, ability to produce ships
	}
	/*  BETTER_BTS_AI_MOD, Settler AI, 02/03/09, jdog5000: START
		(edited by K-Mod) */
	// Push players to get more coastal cities so they can build navies
	CvArea* pWaterArea = kPlot.waterArea(true);
	if (pWaterArea != NULL)
	{
		//120 + (kSet.isSeafaring() ? 160 : 0);
		iR += (kSet.isSeafaring() ? 150 : 100); // advc.031
		if (kTeam.AI_isWaterAreaRelevant(*pWaterArea) &&
			/*  advc.031: Don't worry about coastal production if we
				already have many coastal cities. */
			kPlayer.countNumCoastalCities() <= iCities / 3)
		{
			//iR += 120 + (kSet.isSeafaring() ? 160 : 0);
			iR += (kSet.isSeafaring() ? 240 : 125); // advc.031
			/*if (kPlayer.countNumCoastalCities() < iCities / 4 ||
				kPlayer.countNumCoastalCitiesByArea(kPlot.getArea()) == 0)*/
			if (// advc.031: Disabled this clause
				//kPlayer.countNumCoastalCities() < iCities / 4 ||
				(kPlot.getArea().getCitiesPerPlayer(ePlayer) > 0 &&
				kPlayer.countNumCoastalCitiesByArea(kPlot.getArea()) == 0))
			{
				iR += 200;
			}
		}
	}  // <advc.031>
	// Modify based on production (since the point is to build a navy)
	scaled rMult = rProductionModifier;
	if (GC.getInitCore().isPangaea())
		rMult /= 2;
	iR = (iR * rMult).round();
	// Encourage canals
	if (pWaterArea != NULL &&
		/*  ... but not if there is so little land that some city
			will probably create a canal in any case */
		iLandTiles >= 8)
	{
		CvArea* pWater2 = kPlot.secondWaterArea();
		if (pWater2 != NULL && pWater2 != pWaterArea)
		{
			int iSz1 = pWaterArea->getNumTiles();
			int iSz2 = pWater2->getNumTiles();
			int iSizeFactor = std::min(30, std::min(iSz1, iSz2));
			if (iSizeFactor >= GC.getDefineINT(CvGlobals::MIN_WATER_SIZE_FOR_OCEAN))
			{
				iR += 9 * iSizeFactor;
				IFLOG logBBAI("Connecting waterbodies of at least size %d", iSizeFactor);
			}
		}
	} // </advc.031>
	iR += 50; // advc: as in K-Mod (was 200 in BBAI)
	// BETTER_BTS_AI_MOD: END
	IFLOG logBBAI("+%d from coastal", iR);
	return iR;
}

// <!-- custom: Value the defense that remains after founding rather than the pre-city plot's feature/improvement defense, because current city creation removes both.
// This replaces BtS's flat 200, K-Mod's 100+100 and old SAS's disabled 75+75 defense constants.
// The first proportional version scaled the whole site and redirected several tested opening choices toward hills; the XML default instead scales by one tenth of the combat-defense percent against only the remaining workable-plot economy, making an ordinary 25% hill worth +2.5% of that economy (+5% for a defensive personality) without carrying resource and other non-plot rewards into defense.
// XML terrain defense and HILLS_EXTRA_DEFENSE remain authoritative, so a mod-added 50% defensible founding terrain receives +5% naturally.
// Do not exclude Barbarians: defensible sites benefit them and later conquerors without lowering local yield quality. (GPT-5.6-Sol) -->
int AIFoundValue::evaluateDefense(int iWorkablePlotValue) const
{
	static const int iDefenseValuePercent = std::max(0, GC.getDefineINT("SAS_EVALUATE_CITY_SITE_DEFENSE_WORKABLE_VALUE_PERCENT_PER_100_DEFENSE"));
	static const int iDefensivePersonalityValuePercent = std::max(0, GC.getDefineINT("SAS_EVALUATE_CITY_SITE_DEFENSIVE_PERSONALITY_VALUE_PERCENT"));
	int iDefenseModifier = GC.getInfo(kPlot.getTerrainType()).getDefenseModifier();
	if (kPlot.isHills())
		iDefenseModifier += GC.getDefineINT(CvGlobals::HILLS_EXTRA_DEFENSE);
	if (iDefenseModifier == 0 || iWorkablePlotValue <= 0)
		return 0;

	scaled rValue = per10000(iDefenseModifier * iDefenseValuePercent) * iWorkablePlotValue;
	if (kSet.isDefensive())
		rValue *= per100(iDefensivePersonalityValuePercent);
	int const iValue = rValue.round();
	IFLOG logBBAI("CITY_SITE_DEFENSE modifier=%d defensivePersonality=%d workablePlotValue=%d adjustment=%+d", iDefenseModifier, kSet.isDefensive(), iWorkablePlotValue, iValue);
	return iValue;
}


int AIFoundValue::evaluateGoodies(int iGoodies) const
{
	return iGoodies * 50;
}

/*	advc.027: Starting near a mountain chain that separates a landmass
	into two sizable areas is bad b/c normalization may (or may not)
	remove that obstacle; makes the site difficult to evaluate. */
int AIFoundValue::adjustToLandAreaBoundary(int iValue) const
{
	/*	Change advc.030 makes this check easy to implement. Can't do it
		if that's disabled. */
	if (!GC.getDefineBOOL(CvGlobals::PASSABLE_AREAS))
		return iValue;
	std::set<int> otherLandAreas;
	bool bFoundImpassable = false;
	int const iReprArea = kArea.getRepresentativeArea();
	/*	Range corresponds to CvGame::normalizeRemovePeaks. Need to go one farther
		b/c the boundary itself takes up one tile and could belong to kArea. */
	int const iPeakRemovalRange = 3;
	for (SquareIter itPlot(kPlot, iPeakRemovalRange + 1); itPlot.hasNext(); ++itPlot)
	{
		if (!itPlot->isArea(kArea) &&
			itPlot->getArea().getRepresentativeArea() == iReprArea)
		{
			otherLandAreas.insert(itPlot->getArea().getID());
		}
		if (itPlot->isImpassable() && itPlot.currStepDist() <= iPeakRemovalRange)
			bFoundImpassable = true;
	}
	if (!bFoundImpassable)
		return iValue;
	int iOtherLandAreaSize = 0;
	for (std::set<int>::const_iterator it = otherLandAreas.begin();
		it != otherLandAreas.end(); ++it)
	{
		iOtherLandAreaSize += GC.getMap().getArea(*it)->getNumTiles();
	}
	if (iOtherLandAreaSize <= 0)
		return iValue;
	int iAreaSize = kArea.getNumTiles();
	scaled rAdjust;
	if (iOtherLandAreaSize >= iAreaSize)
	{
		rAdjust = scaled(iAreaSize, iOtherLandAreaSize);
		rAdjust.increaseTo(fixp(1/4.));
	}
	else rAdjust = 1 - scaled(iOtherLandAreaSize, 2 * iAreaSize);
	return (iValue * rAdjust).round();
}

// Taking into account tiles beyond the city radius
int AIFoundValue::adjustToStartingSurroundings(int iValue) const
{
	int iR = iValue;
	int iGreaterBadTile = 0;
	int const iRange = 6; // K-Mod (was 5)
	for (SquareIter it(kPlot, iRange); it.hasNext(); ++it)
	{
		CvPlot const& p = *it;
		// <!-- custom: AdvCiv restored an outer same-area gate that made K-Mod's retained other-island badness branch unreachable. Let every distance-qualified plot reach the branch; only water and same-area land contribute usable yields below. See KI#482. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
		if (it.currPlotDist() <= iRange)
		{
			/*int iTempValue = (p->getYield(YIELD_FOOD) * 15);
			iTempValue += (p->getYield(YIELD_PRODUCTION) * 11);
			iTempValue += (p->getYield(YIELD_COMMERCE) * 5);
			r += iTempValue;
			if (iTempValue < 21) {
				iGreaterBadTile += 2;
				if (p->isFeature()) {
					if (p->calculateBestNatureYield(YIELD_FOOD, eTeam) > 1)
						iGreaterBadTile--;
				}
			}*/ // BtS
			// K-Mod
			int iTempValue = 0;
			iTempValue += p.getYield(YIELD_FOOD) * 9;
			iTempValue += p.getYield(YIELD_PRODUCTION) * 5;
			iTempValue += p.getYield(YIELD_COMMERCE) * 3;
			iTempValue += p.isRiver() ? 1 : 0;
			iTempValue += p.isWater() ? -2 : 0;
			if (iTempValue < 13)
			{
				/*	3 points for unworkable plots (desert, ice, far-ocean)
					2 points for bad plots (ocean, tundra)
					1 point for fixable bad plots (jungle) */
				iGreaterBadTile++;
				if (p.calculateBestNatureYield(YIELD_FOOD, eTeam) < 2)
				{
					iGreaterBadTile++;
					if (iTempValue <= 0)
						iGreaterBadTile++;
				}
			}
			if (p.isWater() || p.isArea(kArea))
				iR += iTempValue;
			else if (iTempValue >= 13)
				iGreaterBadTile++; // add at least 1 badness point for other islands.
			// K-Mod end
		}
	}
	IFLOG logBBAI("+%d from surroundings", iR - iValue);
	if (kSet.isNormalizing())
		return iR;
	/*iGreaterBadTile /= 2;
	if (iGreaterBadTile > 12) {
		r *= 11;
		r /= iGreaterBadTile;
	}*/ // BtS
	// K-Mod. note: the range has been extended, and the 'bad' counting has been rescaled.
	iGreaterBadTile /= 3;
	int iGreaterRangePlots = 2 * (SQR(iRange) + iRange) + 1;
	int iGreaterRangeFactor = iGreaterRangePlots / 6;
	if (iGreaterBadTile > iGreaterRangeFactor)
	{
		iR *= iGreaterRangeFactor;
		iR /= iGreaterBadTile;
		IFLOG logBBAI("Times %d/%d for bad tiles in the greater range", iGreaterRangeFactor, iGreaterBadTile);
	}

	// Maybe we can make a value adjustment based on the resources and players currently in this area
	// (wip)
	// advc.031: Deleted. This should be the responsibility of CvPlayer::findStartingAreas.

	/*  advc: Unused BtS and K-Mod code dealing with WaterCount and
		MinOriginalStartDist deleted */

	int const iTempValue = iR; // advc.031c
	int iMinDistanceFactor = MAX_INT;
	int const iMinRange = //startingPlotRange();
			kGame.getStartingPlotRange(); // advc.opt (now cached)
	//r *= 100; // (disabled by K-Mod to prevent int overflow)
	for (PlayerIter<CIV_ALIVE> it; it.hasNext(); ++it)
	{
		CvPlayer const& kOther = *it;
		if (kOther.getID() == ePlayer)
			continue;
		int iClosenessFactor = kOther.startingPlotDistanceFactor(kPlot, ePlayer, iMinRange);
		iMinDistanceFactor = std::min(iClosenessFactor, iMinDistanceFactor);
		if (iClosenessFactor < 1000)
		{
			iR *= 2000 + iClosenessFactor;
			iR /= 3000;
		}
	}
	if (iMinDistanceFactor > 1000)
	{
		//give a maximum boost of 25% for somewhat distant locations, don't go overboard.
		iMinDistanceFactor = std::min(1500, iMinDistanceFactor);
		iR *= (1000 + iMinDistanceFactor);
		iR /= 2000;
	}
	else if (iMinDistanceFactor < 1000)
	{
		//this is too close so penalize again.
		iR *= iMinDistanceFactor;
		iR /= 1000;
		/*iR *= iMinDistanceFactor;
		iR /= 1000;*/
		// <advc.031> Squaring the iMinDistanceFactor/1000 ratio is too drastic
	}
	if (iMinDistanceFactor < 666)
	{
		iR *= iMinDistanceFactor;
		iR /= 666;
	} // </advc.031>
	IFLOG logBBAI("%d from distance to other players", iR - iTempValue);
	return iR;
}

/*	advc.027:  (Not sure how to name this function. It's supposed to encourage
	-some- strong tiles in the first ring of tiles outside the city radius
	in order to make the choice whether to settle in place more interesting.) */
int AIFoundValue::adjustToStartingChoices(int iValue) const
{
	int iResources = 0;
	int iHighYield = 0;
	int iLowYield = 0;
	for (PlotCircleIter it(kPlot, CITY_PLOTS_RADIUS + 1, false); it.hasNext(); ++it)
	{
		if (it.currPlotDist() <= CITY_PLOTS_RADIUS)
			continue;
		CvPlot const& p = *it;
		// Off the cuff ...
		if (getBonus(p) != NO_BONUS)
		{
			iResources++; // Don't care about the yield or other specifics
			continue;
		}
		int iYieldVal = 0;
		FOR_EACH_ENUM(Yield)
		{
			iYieldVal += (eLoopYield == YIELD_COMMERCE ? 1 : 2) *
					p.calculateNatureYield(eLoopYield);
		}
		if (p.isRiver())
			iYieldVal++;
		if (iYieldVal > 4) // E.g. Plains tile as a baseline
			iHighYield++;
		else if (iYieldVal < 4)
			iLowYield++;
	}
	iLowYield = std::max(0, iLowYield - 4); // A few low-yield tiles aren't a problem
	// One resource and one high-yield tile is plenty
	int iModifier = std::min(10, 4 * iHighYield - 2 * iLowYield + 7 * iResources);
	if (iModifier < 0)
		iModifier /= 2;
	return (iValue * (100 + iModifier / 2)) / 100;
}


// <!-- custom: remove this, as we already have a very new fine tuned yield terrain focus logic, and lack of production is not necessarily bad (cottage city may be strong locally if AI otherwise has good empire production, but point is don't reject city just because it may have low production, look at overall terrain and such yields which we already do, so remove this specific logic that may interfere with that and our changes in a way that may make AI less efficient maybe if i'm not mistaken) -->
// int AIFoundValue::adjustToProduction(int iValue, scaled rBaseProduction) const
// {
// 	// K-Mod. reduce value of cities which will struggle to get any productivity.
// 	// <advc.040>
// 	if (bFirstColony)
// 	{
// 		rBaseProduction += scaled::max(iUnrevealedTiles * fixp(0.5),
// 				(rBaseProduction * iUnrevealedTiles) / NUM_CITY_PLOTS);
// 	} // </advc.040>
// 	rBaseProduction.increaseTo(GC.getInfo(YIELD_PRODUCTION).getMinCity()); // advc.031
// 	FAssert(!isRevealed(kPlot) ||
// 			// advc.test: I've seen this fail when there are Gems under Jungle (fixme)
// 			rBaseProduction >= GC.getInfo(YIELD_PRODUCTION).getMinCity());
// 	scaled rThreshold = fixp(8.5); // advc.031: was 9
// 	// <advc.303> Can't expect that much production from just the inner ring.
// 	if (bBarbarian)
// 		rThreshold = 4; // </advc.303>
// 	if (rBaseProduction < rThreshold)
// 	{
// 		// <advc.031> Was linear (times BaseProduction/Threshold)
// 		iValue = (iValue * (scaled::max(1, rBaseProduction - 1) /
// 				rThreshold).sqrt()).round(); // </advc.031>
// 		IFLOG logBBAI("Times (%d/%d) for low production", rBaseProduction.getPercent(), rThreshold.getPercent());
// 	} // K-Mod end
// 	return iValue;
// }


// <!-- custom: CvGame::createBarbarianCity already allows a new Barbarian city only on land not currently visible to any civilization team, subject to normal founding/spacing rules. Base AdvCiv additionally used this function to lower the score of otherwise eligible fogged tiles close to any existing city, using a randomized distance range to spread Barbarian cities out and discourage touching borders. It did not make normal civilization Settlers avoid Barbarian cities. Disable only this extra penalty: a stronger nearby fogged site can now beat a weaker farther one, leaving a better city for both the Barbarians and whoever later captures it. (GPT-5.6-Sol) -->
// int AIFoundValue::adjustToBarbarianSurroundings(int iValue) const
// {
// 	int r = iValue;
// 	int const iRange = kSet.getBarbarianDiscouragedRange();
// 	if (iRange <= 0)
// 		return r;
// 	CvCity* pNearestCity = GC.getMap().findCity(iX, iY, NO_PLAYER);
// 	/*  <advc.303>, advc.300. Now that the outer ring isn't counted, I worry
// 		that an absolute penalty would reduce iValue to 0 too often. Also want
// 		to discourage touching borders a bit more, which the relative penalty
// 		should accomplish better.
// 		I don't see the need for special treatment of nearest city in a
// 		different area. CvGame avoids settling uninhabited areas anyway. */
// 	if (pNearestCity == NULL)
// 		pNearestCity = GC.getMap().findCity(iX, iY, NO_PLAYER, NO_TEAM, false);
// 	if (pNearestCity != NULL)
// 	{
// 		r *= std::min(iRange, plotDistance(iX, iY,
// 				pNearestCity->getX(), pNearestCity->getY()));
// 		r /= iRange;
// 		IFLOG if(iValue!=r) logBBAI("%d from %S being near a Barbarian site (discouraged range: %d)",
// 				r - iValue, cityName(*pNearestCity), iRange);
// 	}
// 	/*if (pNearestCity)
// 		r -= (std::max(0, (8 - plotDistance(iX, iY, pNearestCity->getX(), pNearestCity->getY()))) * 200);
// 	else {
// 		pNearestCity = GC.getMap().findCity(iX, iY, NO_PLAYER, NO_TEAM, false);
// 		if (pNearestCity != NULL) {
// 			int iDistance = plotDistance(iX, iY, pNearestCity->getX(), pNearestCity->getY());
// 			r -= std::min(500 * iDistance, (8000 * iDistance) / GC.getMap().maxPlotDistance());
// 		}
// 	}*/ // </advc.303>
// 	return r;
// }


// <!-- custom: this adjustToCivSurroundings caused a bug of AI settler settling on bonus camel desert which is very bad in a desert surroudning even worse, it is seemingly called only once in AIFoundValue::evaluate, may as well disable it (including our changes in it) since it is so complicated and who knows where the bugs is(/are?) and instead migrate only a very simplified version of the logic we want directly inline in its only caller so in AIFoundValue::evaluate, done so with the help of chatgpt 5, check if accurate -->
// int AIFoundValue::adjustToCivSurroundings(int iValue, int iStealPercent) const
// {
// 	// K-Mod. Adjust based on proximity to other players, and the shape of our empire.
// 	int iForeignProximity = 0;
// 	bool bFreeForeignCulture = false; // advc.031
// 	int iOurProximity = 0;
// 	CvCity const* pOurNearestCity = NULL;
// 	CvCity const* pNearestForeignCity = NULL; // advc.031
// 	int iMaxDistanceFromCapital = 0;
// 	for (PlayerIter<CIV_ALIVE,KNOWN_TO> it(eTeam); it.hasNext(); ++it)
// 	{
// 		CvPlayer const& kLoopPlayer = *it;
// 		if (kArea.getCitiesPerPlayer(kLoopPlayer.getID()) <= 0 ||
// 			GET_TEAM(kLoopPlayer.getTeam()).isVassal(eTeam))
// 		{
// 			continue;
// 		}
// 		int iProximity = 0;
// 		FOR_EACH_CITY(pLoopCity, /* *this */ kLoopPlayer) // advc.001: Not this player!
// 		{
// 			if (kLoopPlayer.getID() == ePlayer && pCapital != NULL)
// 			{
// 				iMaxDistanceFromCapital = std::max(iMaxDistanceFromCapital,
// 						plotDistance(pCapital->plot(), pLoopCity->plot()));
// 			}
// 			if (!pLoopCity->isArea(kArea))
// 				continue;
// 			// <advc.031> Don't cheat
// 			if (!kSet.isAllSeeing() && !kTeam.AI_deduceCitySite(*pLoopCity))
// 				continue; // </advc.031>
// 			int const iDistance = plotDistance(iX, iY,
// 					pLoopCity->getX(), pLoopCity->getY());
// 			if (kLoopPlayer.getID() == ePlayer)
// 			{
// 				if (pOurNearestCity == NULL ||
// 					iDistance < plotDistance(iX, iY,
// 					pOurNearestCity->getX(), pOurNearestCity->getY()))
// 				{
// 					pOurNearestCity = pLoopCity;
// 				}
// 			}
// 			// <advc.031>
// 			else if (kLoopPlayer.hasCapital())
// 			{
// 				if (pNearestForeignCity == NULL ||
// 					iDistance < plotDistance(iX, iY,
// 					pNearestForeignCity->getX(), pNearestForeignCity->getY()))
// 				{
// 					pNearestForeignCity = pLoopCity;
// 				}
// 			} // </advc.031>
// 			int iCultureRange = pLoopCity->getCultureLevel() + 3;
// 			if (kTeam.AI_deduceCitySite(*pLoopCity))
// 			{
// 				if (iDistance <= iCultureRange)
// 				{
// 					// cf. culture distribution in CvCity::doPlotCultureTimes100
// 					iProximity += 90*(iDistance-iCultureRange)*(iDistance-iCultureRange)/
// 							(iCultureRange*iCultureRange) + 10;
// 				}
// 				/*	<advc.031> A little extra range is useful to consider for
// 					contested sites */
// 				else if (iDistance == iCultureRange + 1)
// 					iProximity += 5; // </advc.031>
// 			}
// 		}
// 		if (kLoopPlayer.getTeam() == eTeam)
// 			iOurProximity = std::max(iOurProximity, iProximity);
// 		else if (iProximity > iForeignProximity)
// 		{
// 			iForeignProximity = iProximity;
// 			// advc.031:
// 			bFreeForeignCulture = (kLoopPlayer.getFreeCityCommerce(COMMERCE_CULTURE) > 1 ||
// 					(pNearestForeignCity != NULL && pNearestForeignCity->isCapital()));
// 		}
// 	}
// 	// <advc.031>
// 	int iOurFreeCultureAdvantage = (bFreeForeignCulture ? -1 : 0) +
// 			(kSet.isEasyCulture() ? 1 : 0); // </advc.031>
// 	/*	Reduce the value if we are going to get squeezed out by culture.
// 		Increase the value if we are hoping to block the other player! */
// 	if (iForeignProximity > 0)
// 	{
// 		/*	As a rough guide of scale, settling 3 steps from a level 2 city
// 			in isolation would give a proximity of 24.
// 			4 steps from a level 2 city = 13
// 			4 steps from a level 3 city = 20 */
// 		int iDelta = iForeignProximity - iOurProximity;
// 		IFLOG logBBAI("Proximity difference (foreign minus ours): %d - %d = %d", iForeignProximity, iOurProximity, iDelta);
// 		if (iDelta > 47 + iOurFreeCultureAdvantage * 8) // advc.031: was 50 flat
// 		{
// 			IFLOG logBBAI("Site disregarded: proximity difference too great");
// 			return 0; // we'd be crushed and eventually flipped if we settled here.
// 		}
// 		int const iTempValue = iValue; // advc.031
// 		if (iDelta > -20 &&
// 			//iDelta <= (kSet.isAmbitious() ? 10 : 0) * (bEasyCulture ? 2 : 1))
// 			// advc.031:
// 			iDelta <= 5 + iOurFreeCultureAdvantage * 5 + (kSet.isAmbitious() ? 8 : 0))
// 		{
// 			/*	we want to get this spot before our opponents do.
// 				The lower our advantage, the more urgent the site is. */
// 			iValue *= 115 + // advc.031: was 120
// 					iDelta/2 + (kSet.isAmbitious() ? 5 : 0)
// 					// advc.031: The 2nd city should focus more on high yields
// 					- (iCities <= 1 ? 11 : 0);
// 			iValue /= 100;
// 			/*  <advc.031> Don't rush to settle marginal spots (which might
// 				not even make the MinFoundValue cut w/o the boost above). */
// 			if (iTempValue < 2000)
// 			{
// 				iValue *= iTempValue;
// 				iValue /= 2000;
// 				iValue = std::max(iTempValue, iValue);
// 			} // </advc.031>
// 		}
// 		//iDelta -= (kSet.isEasyCulture() ? 20 : 10);
// 		iDelta -= 10 + iOurFreeCultureAdvantage * 10; // advc.031
// 		if (iDelta > 0 &&
// 			iForeignProximity > 5) // advc.031
// 		{
// 			iValue *= 100 - iDelta*3/2;
// 			iValue /= 100;
// 		}
// 		IFLOG if(iValue!=iTempValue) logBBAI("%d from foreign proximity", iValue - iTempValue);
// 		/*  <advc.031> This is not about being squeezed, but squeezing others
// 			and thereby angering them. StealPercent says how much we
// 			squeeze them (cultural strength is already taken into account). */
// 		if (iStealPercent >= 100)
// 		{
// 			/*  Between 130 (Alexander, G. Khan, Louis, Montezuma) and
// 				80 (Gandhi, Joao, Justinian). A leader who likes limited war
// 				should be less concerned about creating border troubles. */
// 			scaled rDiploFactor = (kPlayer.isHuman() && !kSet.isDebug() ? 100 :
// 					GC.getInfo(kPlayer.getPersonalityType()).getLimitedWarPowerRatio());
// 			if (kSet.isDefensive())
// 				rDiploFactor += 33;
// 			if (kPlot.isHills())
// 				rDiploFactor += 16;
// 			// The importance of a few stolen tiles decreases over time
// 			rDiploFactor += rAIEraFactor * 13;
// 			rDiploFactor = fixp(1.6) * rDiploFactor / iStealPercent;
// 			rDiploFactor.clamp(fixp(0.6), 1);
// 			iValue = (iValue * rDiploFactor).round();
// 			IFLOG logBBAI("Times %d percent (diplo modifier) from stealing %d/100 tiles",
// 					rDiploFactor.getPercent(), iStealPercent);
// 		} // </advc.031>
// 	}  // <advc.108> Avoid moving the starting settler far on crowded maps
// 	else if (iCities <= 0 && !kSet.isStartingLoc() && !kSet.isNormalizing() &&
// 		kPlayer.getStartingPlot() != NULL && &kPlot != kPlayer.getStartingPlot())
// 	{
// 		int iCivAlive = PlayerIter<CIV_ALIVE>::count();
// 		int iRecommended = kGame.getRecommendedPlayers();
// 		if (iCivAlive > iRecommended && iRecommended > 0)
// 		{
// 			int iDistFromStart = plotDistance(kPlayer.getStartingPlot(), &kPlot);
// 			if (iDistFromStart > 1)
// 			{
// 				scaled rMultiplier(iRecommended,
// 						iCivAlive + std::min(iDistFromStart, 5) - 1);
// 				IFLOG logBBAI("Times %d percent for moving starting Settler %d tiles on a crowded map",
// 						rMultiplier.getPercent(), iDistFromStart);
// 				iValue = (iValue * rMultiplier).round();
// 			}
// 		}
// 	} // </advc.108>
// 	// K-Mod end (the rest is original code - but I've made some edits...)

// 	if (pOurNearestCity != NULL)
// 	{
// 		// <!-- custom: disabled as part of the change where this variable is (was,) used in this function -->
// 		// int const iTempValue = iValue; // advc.031c
// 		int const iDistance = plotDistance(&kPlot, pOurNearestCity->plot());
// 		/*  advc: BtS code dealing with iDistance deleted;
// 			K-Mod comment: Close cities are penalised in other ways */
// 		// with that max distance, we could fit a city in the middle!
// 		int const iTargetRange = //(kSet.isExpansive() ? 6 : 5)
// 				/*	advc.031: Simply 5 unless a mod changes the city radius.
// 					Handle expansive setting below. */
// 				(5 + CITY_PLOTS_DIAMETER) / 2;
// 		int iNearestDistance = iDistance;
// 		/*	<advc.031> There can already be a city "in the middle" that iDistance
// 			doesn't account for - namely when it's a foreign city. */
// 		if (iNearestDistance > iTargetRange && pNearestForeignCity != NULL)
// 		{
// 			CvPlayer const& kNearestOwner = GET_PLAYER(pNearestForeignCity->getOwner());
// 			// (We've already ensured that they have a capital)
// 			CvPlot const& kTheirCapitalPlot = kNearestOwner.getCapital()->getPlot();
// 			int iForeignDistance = plotDistance(&kPlot, pNearestForeignCity->plot());
// 			if (iForeignDistance < iNearestDistance && pCapital != NULL &&
// 				(!kTheirCapitalPlot.isArea(kArea) ||
// 				3 * plotDistance(&kPlot, &kTheirCapitalPlot) >
// 				2 * plotDistance(&kPlot, pCapital->plot())))
// 			{
// 				iNearestDistance = (2 * iNearestDistance + 3 * iForeignDistance) / 5;
// 			}
// 		} // </advc.031>
// 		// K-Mod.

// 		// <!-- custom: remove long distance penalties, currently cities are too crowded which is inefficient and indeed as said in base advciv a bad AI settlment, i hope this helps, while our overall yield logic that we added makes AI simply go for best sites rather and spreading cities a bit more at least in theory although i didn't check in detail how this works (advised to remove penalty by claude ai as i asked it what this does and how we could spread cities further or something similar) --> 
// 		/*  advc.031: Make expansive leaders indifferent about distance 5 vs. 6,
// 			but don't encourage greater distances. */
// 		// if (iNearestDistance > iTargetRange +
// 		// 	(kSet.isExpansive() ? 1 : 0)) // advc.031
// 		// {
// 		// 	int const iExcessDistance = std::min(iTargetRange,
// 		// 			iNearestDistance - iTargetRange);
// 		// 	//iValue -= iExcessDistance * 400;
// 		// 	/*	<advc.031> Penalizing distance like this will lead to cities that aren't
// 		// 		locally optimal. Can't really do anything about that here. Should perhaps
// 		// 		be handled by AI_updateCitySites instead. */
// 		// 	if (iExcessDistance > 0)
// 		// 	{
// 		// 		iValue -= 150;
// 		// 		if (iExcessDistance > 1)
// 		// 			iValue -= 250;
// 		// 		if (iExcessDistance > 2)
// 		// 			iValue -= (iExcessDistance - 2) * 325;
// 		// 	} // </advc.031>
// 		// }
// 		// iValue *= 8 + 4 * iCities;
// 		// // 5, not iTargetRange, because 5 is better. (advc: iTargetRange is 5 now)
// 		// iValue /= 2 + 4 * iCities + std::max(iTargetRange, iDistance);
// 		// IFLOG if(iTempValue!=iValue) logBBAI("%d from %d distance to %S",
// 		// 		iValue - iTempValue, iDistance, cityName(*pOurNearestCity));

// 		// <!-- custom: replace long distances penalty with our own, not too long, not too short distance system -->
// 		// <!-- custom: try to spread cities more as they are too crowded as of now and simplify formula as well in doing so, as advised as a general idea by claude ai when i asked it about this code thanks thanks-->
// 		// <!-- custom: attempt to increase distance penalties as with flat 1000 with our change, cities are a bit too far as of now with this change, but try not to make them crowded again either, try to make first cities closer to each other, less important for later ones -->

// 		int const iDistanceToOurNearestCity = /* advc.031: */ std::min(GC.getMap().maxMaintenanceDistance(),
// 				::plotDistance(iX, iY, pOurNearestCity->getX(), pOurNearestCity->getY()));
// 		int iDistPenalty = 800;

// 		// </advc.031> (no functional change below)
// 		iDistPenalty *= iDistanceToOurNearestCity;
// 		iDistPenalty /= GC.getMap().maxTypicalDistance(); // advc.140: was maxPlotDistance
// 		iDistPenalty = std::min(500 * iDistanceToOurNearestCity, iDistPenalty);
// 		iValue -= iDistPenalty;
// 		IFLOG logBBAI("%d from distance penalty (%d distance to %S)", iDistPenalty, iDistanceToOurNearestCity, cityName(*pOurNearestCity));

// 		// <!-- custom: on top of that, add a num cities penalty, meaning the less cities we have, the more we care about them being close knit, but not necessarily in relation to capital, this would lead to too much star shaped empires and maybe miss locally fine or nice thin or such other shapes. What i care about is that cities are close to each other not necessarily to capital at all, and that they are not too close else their plots overlap as seems to be the case now, especially when we plant our cities; later in the game, more city spots would be taken, and our economy stronger to support them, and our military ideally stronger to protect them, so we can be more creative or combative about which spots we want maybe especially for local benefits, code added with the help of gemini ai thanks. -->
// 		// --- Custom City Spacing Logic ---
// 		// We want to avoid cities being too close, and to control how far they spread.
// 		// We'll base this on the distance to the nearest city, not the capital.
// 		int const iMinDistanceToNearestCity = 4; // Minimum distance we want to see between cities.
// 		int const iMaxDistanceToNearestCity = 8; // Maximum distance we'll tolerate for new cities.

// 		int const iCities = kPlayer.getNumCities();
// 		int iMinMaxDistanceToNearestCityModifier = 0;

// 		// Penalty for cities being too close. This is a severe penalty.
// 		if (iDistanceToOurNearestCity < iMinDistanceToNearestCity)
// 		{
// 			iMinMaxDistanceToNearestCityModifier = -1700 * (iMinDistanceToNearestCity - iDistanceToOurNearestCity);
// 		}
// 		// Penalty for cities being too far. This penalty decreases as the empire grows.
// 		else if (iDistanceToOurNearestCity > iMaxDistanceToNearestCity)
// 		{
// 			// The penalty is less severe for larger empires, as they can afford to stretch.
// 			int iExcessDistanceToOurNearestCity = iDistanceToOurNearestCity - iMaxDistanceToNearestCity;
// 			// <!-- custom: make it scale slower with city num, as it seems AI is a bit/sometimes too adventurous with more cities -->
// 			// <!-- custom: with 600 they are still a bit too far from core cities some times (high yields), so penalize these far locations a bit more, trying not to make them too close again; adjusting old 1500 + 600 * d formula i madein an attempt to space cities a little bit more but also reduce them sometimes going a bit too far but trying not to overdo it either -->
// 			// iMinMaxDistanceToNearestCityModifier = -600 * iExcessDistanceToOurNearestCity / (1 + iCities);
// 			iMinMaxDistanceToNearestCityModifier = -1700 * iExcessDistanceToOurNearestCity / (1 + (iCities / 3));
// 		}

// 		// Apply the final modifier to the city value.
// 		iValue += iMinMaxDistanceToNearestCityModifier;
// 		IFLOG logBBAI("%d from min / max distance to nearest city modifier (%d distance to our nearest city %S)", iMinMaxDistanceToNearestCityModifier, iDistanceToOurNearestCity, cityName(*pOurNearestCity));				

// 		if (!pOurNearestCity->isCapital() && pCapital != NULL)
// 		// K-Mod end
// 		{
// 			/*  Provide up to a 50% boost to value (80% for adv.start) for
// 				city sites which are relatively close to the core compared
// 				with the most distant city from the core (having a boost
// 				rather than distance penalty avoids some distortion).
// 				This is not primarily about maintenance but more about empire
// 				shape as such[, so] forbidden palace/state property are not
// 				[a] big deal. */ // advc: "so" added b/c the comment didn't make sense to me
// 			int iDistanceToCapital = ::plotDistance(pCapital->plot(), &kPlot);
// 			FAssert(iMaxDistanceFromCapital > 0);
// 			/*iValue *= 100 + (((kSet.isAdvancedStart() ? 80 : 50) * std::max(0, (iMaxDistanceFromCapital - iDistance))) / iMaxDistanceFromCapital);
// 			iValue /= 100;*/ // BtS
// 			/*  K-Mod. just a touch of flavour. (note, for a long time this
// 				adjustment used iDistance instead of iDistanceToCapital; and
// 				so I've reduced the scale to compensate) */
// 			/*int iShapeWeight = kSet.isAdvancedStart() ? 50 : (kSet.isAmbitious() ? 15 : 30);
// 			iValue *= 100 + iShapeWeight * std::max(0, iMaxDistanceFromCapital - iDistanceToCapital) / iMaxDistanceFromCapital;
// 			iValue /= 100 + iShapeWeight;*/
// 			// K-Mod end
// 			// <advc> I'm folding this into a single multiplier for easier debugging
// 			scaled const rShapeWeight = (kSet.isAdvancedStart() ? fixp(0.5) :
// 					(kSet.isAmbitious() ? fixp(0.15) : fixp(0.3)));
// 			scaled rShapeModifier = (1 + rShapeWeight * std::max(0,
// 					iMaxDistanceFromCapital - iDistanceToCapital) /
// 					iMaxDistanceFromCapital) / (1 + rShapeWeight);
// 			iValue = (iValue * rShapeModifier).round(); // </advc>
// 			IFLOG logBBAI("Times %d percent (shape modifier); distance from capital: %d, max. distance: %d",
// 					rShapeModifier.getPercent(), iDistanceToCapital, iMaxDistanceFromCapital);
// 		}
// 		return iValue;
// 	}
// 	pOurNearestCity = GC.getMap().findCity(iX, iY, ePlayer, eTeam, false);
// 	if (pOurNearestCity == NULL)
// 		return iValue;

// 	// <!-- custom: disable distance to capital, use distance to nearest city rather now, see nearest city block in this function for details -->
// 	// int iDistance = /* advc.031: */ std::min(GC.getMap().maxMaintenanceDistance(),
// 	// 		::plotDistance(iX, iY, pOurNearestCity->getX(), pOurNearestCity->getY()));
// 	// // <advc.031> Don't discourage settling on small nearby landmasses
// 	// if (pCapital == NULL || pCapital->isArea(kArea) ||
// 	// 	::plotDistance(&kPlot, pCapital->plot()) >= 10 ||
// 	// 	kArea.getNumTiles() >= NUM_CITY_PLOTS)
// 	// {
// 	// 	int iDistPenalty = 8000;
// 	// 	int iDistPenalty = 5100 - (scaled::min(4, rAIEraFactor) * 775).round();
// 	// }

// 	return iValue;
// }


int AIFoundValue::adjustToCitiesPerArea(int iValue) const
{
	// <advc.130v>
	if (kTeam.isCapitulated() || iCities <= 0)
		return iValue; // </advc.130v>

	// <!-- custom: Preserve AdvCiv's civilian first-colony test without letting an unknown rival city remove the bonus; use the same known/deducible-city policy as ordinary site evaluation. See KI#493. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	if (countKnownCities(kArea) <= 0) // advc.031: Had been counting Barbarian cities
	{
		//iValue *= 2;
		// K-Mod: presumably this is meant to be a bonus for being the first on a new continent.
		// But I don't want it to be a bonus for settling on tiny islands, so I'm changing it.
		int const iModifier = range(100 * (kArea.//getNumTiles()
				getNumRevealedTiles(eTeam) // advc.031: Don't cheat
				- 15) / //15
				20 // req. 40 revealed tiles for the full bonus
				, 100, 200);
		iValue *= iModifier;
		iValue /= 100;
		// K-Mod end
		IFLOG if(iModifier!=100) logBBAI("Times %d for being the first to colonize this landmass", iModifier);
	}
	/*  advc.031: BtS code deleted that was supposed to discourage colonies on
		the home continents of other civs. */
	return iValue;
}


int AIFoundValue::adjustToBonusCount(int iValue, std::vector<int> const& aiBonusCount) const
{
	if (iCities > 0)
	{
		int iBonusCount = 0;
		/*  <advc.052> Count bonus in the city tile double, as settling on a bonus
			is especially greedy. (I think this section is about not grabbing all
			the resources with a single city when there are a lot of resources
			in one place.) */
		if (getBonus(kPlot) != NO_BONUS)
			iBonusCount++; // </advc.052>
		int iUniqueBonusCount = 0;
		FOR_EACH_ENUM(Bonus)
		{
			iBonusCount += aiBonusCount[eLoopBonus];
			iUniqueBonusCount += (aiBonusCount[eLoopBonus] > 0 ? 1 : 0);
		}
		scaled rModifier = 1; // advc: No functional change except rounding
		if (iBonusCount > 4) 
			rModifier.mulDiv(5, 1 + iBonusCount);
		/*else if (iUniqueBonusCount > 2) {
			iValue *= 5;
			iValue /= (3 + iUniqueBonusCount);
		}*/
		/*  <advc.031> I can see how multiple bonuses of the same type
			could help city specialization and thus shouldn't be discouraged as much,
			but iBonus < 5 (unique or not) shouldn't be discouraged at all. */
		if (iBonusCount + iUniqueBonusCount >= 10)
		{
			rModifier *= scaled::max(fixp(0.7),
					(1 - fixp(0.08) * (iBonusCount + iUniqueBonusCount - 9)));
		}
		iValue = (iValue * rModifier).round();
		IFLOG if(rModifier.getPercent()!=100) logBBAI("Times %d percent for high resource count (%d resources, %d unique)",
				rModifier.getPercent(), iBonusCount, iUniqueBonusCount);
	}
	if (!bBarbarian) // advc.303
	{
		int iDeadLockCount = countDeadlockedBonuses();
		if (kSet.isAdvancedStart() && iDeadLockCount > 0)
			iDeadLockCount += /*advc.031 (was 2):*/ 1;
		//iValue /= (1 + iDeadLockCount);
		// advc.031: Replacing the above, which is too harsh.
		iValue = (2 * iValue) / (2 + iDeadLockCount);
		IFLOG if(iDeadLockCount!=0) logBBAI("Times %d/%d for %d deadlocked resources", 2, 2 + iDeadLockCount, iDeadLockCount);
	}
	return iValue;
}


// <!-- custom: try to remove this interference as we have a finer algorithm now, and this old code may lead to unexpected results -->
// int AIFoundValue::adjustToBadTiles(int iValue, int iBadTiles) const
// {
// 	scaled r = iValue;
// 	// <advc.040>
// 	if(bFirstColony)
// 		iBadTiles += iUnrevealedTiles / 2; // </advc.040>
// 	// <advc.031>
// 	if(iBadTiles > 0)
// 	{
// 		/*	A scenario is more likely to mix some very good tiles with a lot of
// 			bad ones. Better to be more conservative on regular maps. */
// 		scaled const rExponent = (kSet.isScenario() && !kSet.isStartingLoc() ?
// 				fixp(1.385) : fixp(1.5));
// 		r -= scaled(iBadTiles).pow(rExponent) * 100 *  // <advc.108>
// 				(fixp(1/3.) + (kSet.isStartingLoc() ?
// 				fixp(1/3.) : 0) + (iCities <= 0 ? fixp(1/3.) : 0)); // </advc.108>
// 		r.increaseTo(0);
// 	} // </advc.031>
// 	IFLOG if(r.round()!=iValue) logBBAI("%d from %d bad tiles too many", r.round() - iValue, iBadTiles);
// 	return r.round();
// }

// <!-- custom: go for yield potential not immediate bad health, also now in our mod jungle yields as of now production on remove as well although a bit less than forest for a bit longer time, but what i mean is look at plot potential rather than immediate health, it can't be that different except in mod mods or such, which we don't intend to support with this change -->
// // advc.031: Stifling bad health needs to be discouraged rigorously
// int AIFoundValue::adjustToBadHealth(int iValue, int iGoodHealth) const
// {
// 	int iBonusHealth = 0;
// 	if (pCapital != NULL)
// 		iBonusHealth += pCapital->getBonusGoodHealth();
// 	if (iCities > 0) // Connection to the capital can be slow/costly in the early game
// 		iBonusHealth = std::min(iBonusHealth, (iBonusHealth * (1 + iCities)) / 4);
// 	int iBadHealth = -iGoodHealth/100 - iBonusHealth -
// 			GC.getInfo(kPlayer.getHandicapType()).getHealthBonus();
// 	if (iBadHealth >= -2) // I.e. can only grow to size 2
// 	{
// 		scaled rDiv = scaled::max(1, 3 - rAIEraFactor + iBadHealth);
// 		int iMult = 1;
// 		if (rDiv <= 1)
// 		{
// 			iMult = 2;
// 			rDiv = 3;
// 			if (kSet.isStartingLoc() && !kSet.isScenario())
// 			{
// 				iMult = 3;
// 				rDiv = 4;
// 			}
// 		}
// 		iValue = ((iMult * iValue) / rDiv).round();
// 		IFLOG if (rDiv.round()>1) logBBAI("Times %d/%d for bad health", iMult, rDiv.round());
// 	}
// 	return iValue;
// }

// advc: Moved from CvPlayerAI since it's only used for computing found values
int AIFoundValue::countDeadlockedBonuses() const
{
	int r = 0;
	int const iMinRange = GC.getDefineINT(CvGlobals::MIN_CITY_RANGE); // 2
	int const iRange = iMinRange * 2;
	for (SquareIter it(kPlot, iRange); it.hasNext(); ++it)
	{
		if (it.currPlotDist() > CITY_PLOTS_RADIUS)
		{
			// <advc.031>
			if (!isRevealed(*it))
				continue; // </advc.031>
			// <advc> Checks moved into subroutine
			if(isDeadlockedBonus(*it, iMinRange))
				r++; // </advc>
		}
	}
	return r;
}

// advc: Cut from countDeadlockedBonuses
bool AIFoundValue::isDeadlockedBonus(CvPlot const& kBonusPlot, int iMinRange) const
{
	if(getBonus(kBonusPlot) == NO_BONUS || kBonusPlot.isCityRadius())
		return false;
	// advc: kArea is the area of the first potential city site (kPlot) near kBonusPlot
	if(!kBonusPlot.isArea(kArea) && !kBonusPlot.isWater())
		return false;
	bool bCanFound = false;
	bool bNeverFound = true;
	//look for a city site [kOtherSite] within a city radius [around kBonusPlot]
	for (CityPlotIter it(kBonusPlot); it.hasNext(); ++it)
	{
		CvPlot const& kOtherSite = *it;
		// <advc.031> Won't want to settle on top of another resource
		if(getBonus(kOtherSite) != NO_BONUS)
			continue; // </advc.031>
		// <advc.031> Need to be able to see if there's land
		if (!isRevealed(kOtherSite) && !kOtherSite.isAdjacentRevealed(eTeam))
			continue; // </advc.031>
		//canFound usually returns very quickly
		if (kPlayer.canFound(kOtherSite,
			false, kSet.isAllSeeing())) // advc.181
		{
			bNeverFound = false;
			if (stepDistance(&kPlot, &kOtherSite) > iMinRange ||
				// advc.031: No distance restriction for cities in different land areas
				!kPlot.sameArea(kOtherSite))
			{
				bCanFound = true;
				break;
			}
		}
	}
	return (!bNeverFound && !bCanFound);
}

// <advc.031c>
void CitySiteEvaluator::logSettings() const
{
	// <!-- custom: These helpers are normally reached through IFLOG or log() guards; keep local guards too so future direct calls cannot leak found-value output into unrelated BBAI categories. (GPT-5.5 + ChatGPT 5.5) -->
	if (gFoundLogLevel <= 0 || !AIFoundValue::isLoggingEnabled())
		return;

	logBBAI("Found parameters for %S:", isStartingLoc() ?
			L"starting location" : getPlayer().getName());
	logBBAI("Culture claim treshold: %d", getClaimThreshold());
	if (getMinRivalRange() != -1)
		logBBAI("MinRivalRange: %d", getMinRivalRange());
	// <advc.300>
	if (getBarbarianDiscouragedRange() != iDEFAULT_BARB_DISCOURAGED_RANGE)
		logBBAI("BarbarianDiscouragedRange: %d", getBarbarianDiscouragedRange());
	// </advc.300>
	if (isStartingLoc())
		logBBAI("StartingLoc");
	if (isScenario())
		logBBAI("WBScenario");
	// <advc.031e>
	if (isNormalizing())
		logBBAI("Normalizing"); // </advc.031e>
	// <advc.007>  <advc.027>
	if (isIgnoreStartingSurroundings())
		logBBAI("Ignoring starting surroundings");
	// <advc.027>
	if (isDebug())
		logBBAI("Ignoring other sites"); // </advc.007>
	if (isAllSeeing())
		logBBAI("All-seeing");
	if (isAdvancedStart())
		logBBAI("in Advanced Start");
	if (isEasyCulture())
		logBBAI("Easy culture");
	if (isAmbitious())
		logBBAI("Ambitious");
	// <advc.908a>
	if (isExtraYieldNaturalThreshold())
		logBBAI("Financial (AdvCiv effect)"); // </advc.908a>
	if (isExtraYieldThreshold())
		logBBAI("Financial (BtS effect)");
	if (isDefensive())
		logBBAI("Defensive");
	if (isSeafaring())
		logBBAI("Seafaring");
	if (isExpansive())
		logBBAI("Expansive (tall)");
}

void AIFoundValue::logSite() const
{
	// <!-- custom: same found-value logging self-guard as logSettings. (GPT-5.5 + ChatGPT 5.5) -->
	if (gFoundLogLevel <= 0 || !isLoggingEnabled())
		return;

	// <!-- custom: Adapt the variadic logger to debugStr's owning return value. See KI#349. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	logBBAI("Computing found value for %S", kPlot.debugStr().c_str());
	if (bCoastal)
		logBBAI("Site is coastal");
	if (!kSet.isStartingLoc() && !kSet.isNormalizing())
		logBBAI("%d other %S cities in the area, %d in total", iAreaCities, kPlayer.getCivilizationShortDescription(), iCities);
}

void AIFoundValue::logPlot(CvPlot const& p, int iPlotValue, int const* aiYield, int iCultureModifier, BonusTypes eBonus, ImprovementTypes eBonusImprovement, bool bCanTradeBonus, bool bCanSoonTradeBonus, bool bCanImproveBonus, bool bCanSoonImproveBonus, bool bEasyAccess, int iFeatureProduction, bool bPersistentFeature, bool bRemovableFeature) const
{
	// <!-- custom: same found-value logging self-guard as logSettings. (GPT-5.5 + ChatGPT 5.5) -->
	if (gFoundLogLevel <= 0 || !isLoggingEnabled())
		return;

	int const F = YIELD_FOOD, P = YIELD_PRODUCTION, C = YIELD_COMMERCE;
	if (isHome(p))
		logBBAI("Home plot: val=%d, %dF%dP%dC", iPlotValue, aiYield[F], aiYield[P], aiYield[C]);
	// <!-- custom: Adapt the variadic logger to debugStr's owning return value. See KI#349. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	else logBBAI("Plot in radius: %S; val=%d, %dF%dP%dC", p.debugStr().c_str(), iPlotValue, aiYield[F], aiYield[P], aiYield[C]);
	if (iCultureModifier != 100)
		logBBAI("Culture modifier: %d", iCultureModifier);
	if (eBonus != p.getBonusType())
	{
		FAssert(eBonus == NO_BONUS);
		logBBAI("Bonus resource hidden");
	}
	if (eBonus != NO_BONUS)
	{
		// Try not to output redundant information here ...
		if (eBonusImprovement == NO_IMPROVEMENT)
		{
			logBBAI("Can't improve resource");
			FAssert(!bCanImproveBonus && !bCanTradeBonus);
		}
		else
		{
			FAssert(bCanSoonImproveBonus || kSet.isAllSeeing());
			if (!bCanSoonTradeBonus)
				logBBAI("Can't connect resource");
			else if (!bCanTradeBonus)
				logBBAI("Can soon connect resource");
			if ((!bCanSoonTradeBonus || !bCanTradeBonus) && bCanImproveBonus)
				logBBAI("Can improve resource");
			else if (!bCanSoonTradeBonus && !bCanTradeBonus)
				logBBAI("Can soon improve resource");
		}
		FAssert(!bCanTradeBonus || bCanSoonTradeBonus);
		if (!bEasyAccess)
			logBBAI("Difficult to access");
	}
	if (isHome(p))
		return;
	if (iFeatureProduction != 0)
	{
		FAssert(!bPersistentFeature);
		logBBAI("Feature production: %d%s", iFeatureProduction, bRemovableFeature ? "" :
				" (reduced b/c can't remove yet)");
	}
	else if (bRemovableFeature)
	{
		FAssert(!bPersistentFeature);
		logBBAI("can remove feature");
	}
}

wchar const* AIFoundValue::cityName(CvCity const& kCity)
{
	static CvWString szName;
	szName = kCity.getName().GetCString();
	return szName;
}
// </advc.031c>

/*	advc.027: Computes a hypothetical contribution that p could make to the found value of some future city, e.g. in this->kPlot, but, for the most part, it shouldn't matter where the city will be and we can't really tell. */
scaled AIFoundValue::evaluateWorkablePlot(CvPlot const& p) const
{
	FAssert(!isLoggingEnabled()); // The output wouldn't make much sense

	scaled r;

	/*	This preamble is mostly copy-pasted from AIFoundValue::evaluate. A bit different b/c we have a longer time horizon here. */
	bool bRemovableFeature = false;
	bool bPersistentFeature = false;
	FeatureTypes const eFeature = p.getFeatureType();
	{
		int iFeatureProduction = 0;
		bRemovableFeature = isRemovableFeature(p, bPersistentFeature, iFeatureProduction);
		r += evaluateFeatureProduction(iFeatureProduction);
	}
	BonusTypes const eBonus = getBonus(p);
	bool bCanTradeBonus = false;
	bool bCanImproveBonus = false;
	bool bCanSoonTradeBonus = false;
	bool bCanSoonImproveBonus = false;
	int aiBonusImprovementYield[NUM_YIELD_TYPES] = {0, 0, 0};
	ImprovementTypes eBonusImprovement = NO_IMPROVEMENT;
	if (eBonus != NO_BONUS)
	{
		bool bImprovementRemovesFeature = false;
		eBonusImprovement = getBonusImprovement(eBonus, p,
				bCanTradeBonus, bCanSoonTradeBonus, aiBonusImprovementYield,
				bCanImproveBonus, bCanSoonImproveBonus, bImprovementRemovesFeature);
		if (eBonusImprovement == NO_IMPROVEMENT) // We're nowhere near the tech reqs
		{
			FAssert(aiBonusImprovementYield[YIELD_FOOD] == 0 &&
					aiBonusImprovementYield[YIELD_PRODUCTION] == 0 &&
					aiBonusImprovementYield[YIELD_COMMERCE] == 0);
			// To account for w/e yields the improved resource will provide eventually
			aiBonusImprovementYield[YIELD_PRODUCTION] = 1;
		}
	}
	if (!bPersistentFeature && eFeature != NO_FEATURE)
	{
		int iRemovableFeatureYieldVal = removableFeatureYieldVal(
				eFeature, bRemovableFeature, eBonus != NO_BONUS);
		int iNonPersistentFeatureYieldVal = removableFeatureYieldVal(
				eFeature, true, eBonus != NO_BONUS);
		/*	Weighted average to account for chopping becoming available later. Mostly treat chopping as available b/c Jungle seems to get valued too lowly; I guess b/c bonus improvement yields aren't counted. */
		r += scaled(iRemovableFeatureYieldVal + 3 * iNonPersistentFeatureYieldVal, 4);
	}
	int aiYield[NUM_YIELD_TYPES];
	FOR_EACH_ENUM(Yield)
	{
		aiYield[eLoopYield] = p.calculateNatureYield(eLoopYield,
				eBonus == NO_BONUS ? NO_TEAM : eTeam,
				!bPersistentFeature);
	}
	scaled const rSpecialYieldModifier =
			per100(calculateSpecialYieldModifier(100,
			true, eBonus != NO_BONUS, true, bCanImproveBonus)) +
			per100(calculateSpecialYieldModifier(100,
			true, eBonus != NO_BONUS, bCanSoonImproveBonus, bCanImproveBonus));

	/*	Here I deviate from AIFoundValue::evaluate b/c evaluating special yields separately wouldn't work here (and it was a bad idea in the first place). I'll apply the special yield modifier to the (whole) yield value in the end. */
	int aiBuildingYield[NUM_YIELD_TYPES];
	calculateBuildingYields(p, aiYield, aiBuildingYield);
	bool bAnySpecial = false;
	FOR_EACH_ENUM(Yield)
	{
		int iSpecial = aiBonusImprovementYield[eLoopYield] +
				aiBuildingYield[eLoopYield];
		if (iSpecial != 0)
		{
			aiYield[eLoopYield] += iSpecial;
			bAnySpecial = true;
		}
	}
	scaled rYieldVal;
	// Equal weight for food and production, but penalize low food later.
	int const aiYieldWeight[NUM_YIELD_TYPES] = {20, 20, 10};
	FOR_EACH_ENUM(Yield)
	{
		scaled rWeight = aiYieldWeight[eLoopYield];
		int iYield = aiYield[eLoopYield];
		if (iYield < 0) // BonusImprovementYield can be negative
			rWeight /= 2;
		rYieldVal += aiYield[eLoopYield] * rWeight;
	}
	{
		bool const bCanNeverImprove = (eFeature != NO_FEATURE &&
				GC.getInfo(eFeature).isNoImprovement());
		// Anticipate terrain improvement
		if (!bCanNeverImprove && !bAnySpecial && !p.isWater())
		{
			rYieldVal *= (  // Too slow for what it accomplishes?
					/*p.isFreshWater() && p.canHavePotentialIrrigation()) ? fixp(1.8) :*/
					fixp(1.75));
		}
		if (bCanNeverImprove) // Not having to improve it is valuable
			rYieldVal *= fixp(4/3.);
	}
	if (aiYield[YIELD_FOOD] < GC.getFOOD_CONSUMPTION_PER_POPULATION())
		rYieldVal *= fixp(0.9); // 0 food isn't necessarily worse
	rYieldVal -= (GC.getFOOD_CONSUMPTION_PER_POPULATION() + fixp(1/3.))
			* aiYieldWeight[YIELD_FOOD] // for pop growth and sustenance
			+ scaled(aiYieldWeight[YIELD_COMMERCE], 4); // expenses per population
	if (rYieldVal > 0)
	{
		rYieldVal.exponentiate(fixp(1.3));
		rYieldVal *= rSpecialYieldModifier;
		r += rYieldVal;
	}
	if (!p.isImpassable())
	{	/*	Even with marginal tile yields, just having space is valuable
			for city placement. */
		int iVal = aiYield[YIELD_FOOD] + aiYield[YIELD_PRODUCTION] +
				aiYield[YIELD_COMMERCE] / 2 +
				(eBonus != NO_BONUS || eFeature != NO_FEATURE ? 2 : 0);
		if ((iVal >= 1 && !p.isWater()) || iVal >= 2)
		{
			r += 4;
			if (iVal >= 2)
			{
				iVal += 4;
				if (!p.isWater() || p.isLake())
					r += 10;
			}
		}
	}
	if (eBonus != NO_BONUS)
	{
		// It won't necessarily be the first instance of eBonus, but we also want to look a bit farther ahead than "soon"; that ought to even out.
		scaled rNonYieldBonusVal = nonYieldBonusValue(p, eBonus, bCanTradeBonus, true, kPlot.isCoastalLand(-1), NULL, 100);
		if (!bCanSoonImproveBonus)
		{	/*	Midgame and late-game resources need to be (greatly) devalued though; b/c their reward is greatly delayed and b/c they're not supposed to steer starting positions much in any case. */
			TechTypes eTech = GC.getInfo(eBonus).getTechImprove(p.isWater());
			scaled rEraDiff = CvEraInfo::normalizeEraNum(
					(eTech == NO_TECH ? 0 : GC.getInfo(eTech).getEra() - eEra));
			if (rEraDiff >= 4)
			{	/*	Some special yield is counted for all resources; that should be enough and more for late-game resources. */
				rNonYieldBonusVal = 0;
			}
			else if (rEraDiff.isPositive())
			{
				rNonYieldBonusVal *= 2;
				rNonYieldBonusVal /= (2 + SQR(rEraDiff));
			}
		}
		// Settling near low-yield resources (especially Snow Fur) is an inconvenience
		scaled const rYieldThresh = 48;
		if (rYieldVal < rYieldThresh)
		{
			// Halve if rYieldVal equal to -rYieldThresh or less; 75% if rYieldVal=0.
			scaled rLowYieldMult = fixp(0.75) +
					(1 / (4 * rYieldThresh)) * scaled::max(-rYieldThresh, rYieldVal);
			rNonYieldBonusVal *= rLowYieldMult;
		}
		r += rNonYieldBonusVal;
	}
	{
		int const iFeatureHealth = (eFeature == NO_FEATURE ? 0 :
				GC.getInfo(eFeature).getHealthPercent());
		if (iFeatureHealth > 0 || bPersistentFeature)
			r += iFeatureHealth * fixp(1/6.);
		else if (!bRemovableFeature)
			r += iFeatureHealth * fixp(1/9.);
	}
	if (r > 0 && p.isWater())
	{
		/*	Decrease value if surrounding land is bad. Save time by only looking at kPlot. */
		int iYieldScore = 5 * kPlot.calculateNatureYield(YIELD_FOOD, NO_TEAM, true, true) +
				4 * kPlot.calculateNatureYield(YIELD_PRODUCTION, NO_TEAM, true, true) +
				2 * kPlot.calculateNatureYield(YIELD_COMMERCE, NO_TEAM, true, true);
		int const iTargetScore = 8;
		if (iYieldScore < iTargetScore)
			r *= 1 - (iTargetScore - iYieldScore) * per100(4);
	}
	if (p.isGoody())
	{
		//r += evaluateGoodies(1);
		/*	They're more valuable than this, but don't want to encourage goodies close to starting sites. */
		r += 18;
	}
	return scaled::max(r, 0);
}
