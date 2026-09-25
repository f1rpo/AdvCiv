#pragma once

#ifndef CITY_SITE_EVALUATOR_H
#define CITY_SITE_EVALUATOR_H

class CvPlayerAI;
class CvTeamAI;
class CvCityAI;
class CvPlot;
class CvArea;
class CvGame;

// advc: New classes extracted from CvPlayerAI::AI_foundValue

// Corresponds to K-Mod's CvPlayerAI::CvFoundSettings
class CitySiteEvaluator
{
public:
	CitySiteEvaluator(CvPlayerAI const& kPlayer, int iMinRivalRange = -1,
			bool bStartingLoc = false, bool bNormalize = false);
	// <!-- custom: found-value path uses int (not short) to avoid overflow/underflow. (GPT-5.2-Codex (summarized)) -->
	int evaluate(CvPlot const& kPlot) const;
	int evaluate(int iX, int iY) const;
	// <!-- custom: Opt-in diagnostic entry point: return the same found value while filling an exact stage-by-stage summary; callers guard its use behind logging so normal city-site evaluation does no string formatting. (GPT-5.5) -->
	int evaluateWithBreakdown(CvPlot const& kPlot, CvString& szBreakdown) const;
	// <!-- custom: First-city roaming can distinguish a genuinely weak site from a food-imperfect site with strong practical worked-plot cores.
	// Compute the best-6/10 decision metrics without formatting the finer best-1/2/3 or mature best-14 diagnostics. (GPT-5.6-Sol) -->
	int evaluateWithGrowthCorePlotValues(CvPlot const& kPlot, int& iBest6PlotValue, int& iBest10PlotValue, int& iSustainableProductivePlotValue) const;
	// <!-- custom: SASGameRecord founding snapshots can reuse the same plot-value distribution already computed inside AIFoundValue without parsing BBAI text or rescanning the BFC afterward.
	// Core sums use sizes 1/2/3/6/10/14; positive-plot count describes the same sorted potential-plot values; diagnostic only. (ChatGPT-5.6-Sol) -->
	int evaluateWithPlotValueDistribution(CvPlot const& kPlot, int* aiCoreSums, int& iPositivePlots, int& iSustainableProductivePlotValue) const;
	// <!-- custom: Share one XML-driven scale for heuristics expressed in found-value units.
	// A sustainable productive reference plot supplies the current food consumption per citizen plus 1 Production and uses the same self-sustaining yield weights as ordinary site evaluation. (GPT-5.6-Sol) -->
	static int getSustainableProductivePlotValue();
	// <!-- custom: Share settlement-specific seafood counts between selected-site, city-site-list and broad founding diagnostics.
	// Ordinary diagnostics use the founder's information and non-obsolete resources; SASGameRecord can additionally request the true-map count to explain hidden seafood at the site actually founded; callers keep the BFC scan behind diagnostic gates. (GPT-5.6-Sol) -->
	static int countWaterBonuses(CvPlot const& kCityPlot, TeamTypes eTeam, bool bDiagnosticOmniscience);
	int evaluateWithLogging(CvPlot const& kPlot) const; // advc.031c
	scaled evaluateWorkablePlot(CvPlot const& kPlot) const; // advc.027
	CvPlayerAI const& getPlayer() const { return m_kPlayer; }
	bool isStartingLoc() const { return m_bStartingLoc; }
	bool isScenario() const { return m_bScenario; }
	bool isNormalizing() const { return m_bNormalize; } // advc.031e
	int getMinRivalRange() const { return m_iMinRivalRange; }
	// <advc.300>
	void discourageBarbarians(int iRange);
	int getBarbarianDiscouragedRange() const { return m_iBarbDiscouragedRange; }
	// </advc.300>  <advc.027>
	void setIgnoreStartingSurroundings(bool b);
	bool isIgnoreStartingSurroundings() const { return m_bIgnoreStartingSurroundings; }
	// </advc.027>
	bool isAdvancedStart() const { return m_bAdvancedStart; } // advc
	/*  <advc.007> Ignores whether kPlot is close to another tentative city site
		and treats kPlayer as non-human */
	void setDebug(bool b);
	bool isDebug() const { return m_bDebug; }
	// </advc.007>
	//int getGreed() const; // advc.031: Not used anymore
	// (The comments below about the found settings are from K-Mod)
	// culture required to pop the 2nd borders (as in BtS)
	int getClaimThreshold() const { return m_iClaimThreshold; }
	/*	doesn't need vision of a plot to know what's there
		[But doesn't necessarily reveal resources; see AIFoundValue::getBonus.] */
	bool isAllSeeing() const { return m_bAllSeeing; }
	// <!-- custom: First-settler roaming needs starting-capital weights without map-generation omniscience. (GPT-5.5) -->
	void setAllSeeing(bool b) { m_bAllSeeing = b; }
	// <!-- custom: Level-3 diagnostics can compare the player's actual information with the true map, including technology-hidden bonuses that ordinary starting-location all-seeing intentionally still conceals; this mode is diagnostic only and must not feed AI choices. (GPT-5.6-Sol) -->
	void setDiagnosticOmniscience(bool b) { m_bDiagnosticOmniscience = b; if (b) m_bAllSeeing = true; }
	bool isDiagnosticOmniscience() const { return m_bDiagnosticOmniscience; }
	// some trait information that will influence where we settle ...
	// easy for us to pop the culture to the 2nd border
	bool isEasyCulture() const { return m_bEasyCulture; }
	// expectation of taking foreign land, either by culture or by force
	bool isAmbitious() const { return m_bAmbitious; }
	// more value for rivers
	bool isExtraYieldThreshold() const { return m_bExtraYieldThresh; }
	bool isExtraYieldNaturalThreshold() const { return m_bExtraYieldNaturalThresh; }
	// more value for settlings on hills
	bool isDefensive() const { return m_bDefensive; }
	// special affection for coast cities due to unique building or unit.
	bool isSeafaring() const { return m_bSeafaring; }
	// willing to place cities further apart. (not directly based on the expansive trait)
	bool isExpansive() const { return m_bExpansive; }
	// <advc.031c>
	void log(CvPlot const& kPlot);
	// <!-- custom: Detailed found-value breakdown for comparison sites logged next to a selected site, e.g. adjacent/current-site checks that explained and fixed the uMgungundlovu stale-target case in KI#185. (GPT-5.5) -->
	void logComparedSiteBreakdown(char const* szLabel, CvPlot const& kPlot) const;
	void logSettings() const; // </advc.031c>

private:
	// <!-- custom: Cache the best XML-valid improvement outcome for each plot across the many overlapping candidate BFCs evaluated by one CitySiteEvaluator.
	// AIFoundValue performs the plot scoring, so grant it access to these private cache helpers instead of exposing their internals publicly. (GPT-5.6-Sol) -->
	struct PlotPotentialYield
	{
		PlotPotentialYield();
		int iValue;
		ImprovementTypes eImprovement;
		int aiYield[NUM_YIELD_TYPES];
		int iTimingPercent;
	};
	bool getCachedPlotPotentialYield(CvPlot const& kPlot, int& iValue, ImprovementTypes& eImprovement, int* aiYield, int& iTimingPercent) const;
	void cachePlotPotentialYield(CvPlot const& kPlot, int iValue, ImprovementTypes eImprovement, int const* aiYield, int iTimingPercent) const;
	bool getCachedImprovementProduction(CvPlot const& kPlot, scaled& rValue) const;
	void cacheImprovementProduction(CvPlot const& kPlot, scaled rValue) const;
	friend class AIFoundValue;
	CvPlayerAI const& m_kPlayer;
	bool m_bStartingLoc;
	bool m_bScenario; // advc
	bool m_bNormalize;
	int m_iMinRivalRange;
	int m_iBarbDiscouragedRange; // advc.300
	bool m_bIgnoreStartingSurroundings; // advc.027
	bool m_bAdvancedStart; // advc
	bool m_bDebug; // advc.007
	bool m_bAllSeeing;
	// <!-- custom: True only for the level-3 true-map comparison evaluator; an evaluator whose result can drive an AI decision must leave this false. (GPT-5.6-Sol) -->
	bool m_bDiagnosticOmniscience;
	int m_iClaimThreshold;
	bool m_bEasyCulture;
	bool m_bAmbitious;
	bool m_bExtraYieldThresh; // (advc.908a: "bFinancial" in K-Mod)
	bool m_bExtraYieldNaturalThresh; // advc.908a
	bool m_bDefensive;
	bool m_bSeafaring;
	bool m_bExpansive;
	// <!-- custom: AI_updateFoundValues evaluates overlapping BFCs across every revealed map plot.
	// Cache plot-intrinsic XML improvement scans for this evaluator instance so the de-hardcoded logic scans each ordinary plot once, while diagnostic reevaluations may still bypass the cache to emit candidate details. (GPT-5.6-Sol) -->
	mutable std::map<PlotNumTypes, PlotPotentialYield> m_plotPotentialYieldCache;
	mutable std::map<PlotNumTypes, scaled> m_improvementProductionCache;
};

/*  AIFoundValue::evaluate corresponds to K-Mod's CvPlayerAI::AI_foundValue_bulk
	and gets called by the constructor. */
class AIFoundValue
{
public:
	// <!-- custom: A non-null breakdown output enables diagnostic accounting; normal evaluation passes NULL and keeps that work disabled. (GPT-5.5) -->
	// <!-- custom: Let SPI construct the shared workable-plot context without also running and discarding a complete city-site evaluation; optional output pointers expose decision components already computed during the same pass. See KI#492. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	AIFoundValue(CvPlot const& kPlot, CitySiteEvaluator const& kSettings, CvString* pszBreakdown = NULL, bool bEvaluateSite = true, int* paiGrowthCorePlotValues = NULL, int* piSustainableProductivePlotValue = NULL, int* paiPlotCoreSums = NULL, int* paiPlotCoreCutoffs = NULL, int* piPositivePlots = NULL);
	int get() const { return m_iResult; }
	scaled evaluateWorkablePlot(CvPlot const& p) const; // advc.027

	// <advc.031c> Will have to enable the found log in BBAILog.h in addition
	static void setLoggingEnabled(bool b);
	static bool isLoggingEnabled() { return bLoggingEnabled; } // </advc.031c>

private:
	int m_iResult;
	CvString* m_pszBreakdown;
	// <!-- custom: Optional caller-owned outputs reuse results already computed by this evaluation.
	// Growth-core returns the best-6 and best-10 values for first-city logic; the productive-plot output shares its reference threshold; the broader diagnostic outputs return the best-1/2/3/6/10/14 sums and cutoffs plus the positive-plot count. (GPT-5.6-Sol) -->
	int* m_paiGrowthCorePlotValues;
	int* m_piSustainableProductivePlotValue;
	int* m_paiPlotCoreSums;
	int* m_paiPlotCoreCutoffs;
	int* m_piPositivePlots;
	/*  The rest aren't prefixed with "m_"; too awkward. Note that the order of
		the reference members needs to match their order in the ctor initalizer list. */
	CvPlot const& kPlot;
	CvArea const& kArea;
	CitySiteEvaluator const& kSet;
	CvPlayerAI const& kPlayer;
	PlayerTypes ePlayer;
	TeamTypes eTeam;
	CvTeamAI const& kTeam;
	CvGame const& kGame;
	bool bBarbarian;
	EraTypes eEra;
	scaled rAIEraFactor;
	int iX, iY;
	CvCityAI const* pCapital;
	bool bCoastal;
	int iAreaCities;
	int iCities;
	// Intermediate values ...
	std::vector<int> aiCitySiteRadius;
	// <advc.035>
	std::vector<bool> abOwnCityRadius;
	std::vector<bool> abFlip; // </advc.035>
	bool bFirstColony;
	int iUnrevealedTiles; // advc.040
	// <advc.031c>
	static bool bLoggingEnabled;
	static wchar const* cityName(CvCity const& kCity);
	void logSite() const;
	void logPlot(CvPlot const& p, int iPlotValue, int const* aiYield, int iCultureModifier, BonusTypes eBonus, ImprovementTypes eBonusImprovement, bool bCanTradeBonus, bool bCanSoonTradeBonus, bool bCanImproveBonus, bool bCanSoonImproveBonus, bool bEasyAccess, int iFeatureProduction, bool bPersistentFeature, bool bRemovableFeature) const;
	// </advc.031c>
	int evaluate();
	// Subroutines of evaluate ...
	bool isHome(CvPlot const& p) const { return (&p == &kPlot); }
	bool isSiteValid() const;
	bool computeOverlap();
	bool isPrioritizeAsFirstColony() const; // advc.040
	int countBadTiles(int& iInner, int& iUnrevealed, int& iLand, int& iRevealedDecentLand) const;
	bool isTooManyBadTiles(int iBadTiles, int iInnerBadTiles) const;
	int baseCityValue() const;
	bool isUsablePlot(CityPlotTypes ePlot, int& iTakenTiles, bool& bCityRadius, bool& bForeignOwned, bool& bAnyForeignOwned, bool& bShare, bool& bSteal) const;
	bool isRemovableFeature(CvPlot const& p, bool& bPersistent, int& iFeatureProduction) const;
	bool isRevealed(CvPlot const& p) const;
	PlayerTypes getRevealedOwner(CvPlot const& p) const;
	TeamTypes getRevealedTeam(CvPlot const& p) const;
	// <!-- custom: Count only cities the evaluating team owns or can locate when an ordinary found-value heuristic asks about an area's settlement state. See KI#493 and KI#495. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
	int countKnownCities(CvArea const& kLoopArea, PlayerTypes eOwner = NO_PLAYER) const;
	BonusTypes getBonus(CvPlot const& p) const;
	ImprovementTypes getBonusImprovement(BonusTypes eBonus, CvPlot const& p, bool& bCanTrade, bool& bCanTradeSoon, int* aiImprovementYield, bool& bCanImprove, bool& bCanImproveSoon, bool& bRemoveFeature) const;
	bool isNearTech(TechTypes eTech) const;
	int calculateCultureModifier(CvPlot const& p, bool bForeignOwned, bool bShare, bool bCityRadius, bool bSteal, bool bFlip, bool bOwnExcl, int& iTakenTiles, int& iStealPercent) const;
	int removableFeatureYieldVal(FeatureTypes eFeature, bool bRemovableFeature, bool bBonus) const;
	// <!-- custom: The XML-driven replacement for AdvCiv's earlier fixed improvement-production estimate no longer accepts bPersistentFeature: feature retention or removal depends on each legal Build candidate and is derived inside the function. (GPT-5.6-Sol) -->
	scaled estimateImprovementProduction(CvPlot const& p) const;
	// <!-- custom: Settler sites now value ordinary plots through XML-valid improvement outcomes rather than named terrain/feature tables.
	// Keep the city-center distinction optional so the home plot's lost workable potential can be evaluated as an ordinary BFC plot. (GPT-5.6-Sol) -->
	int evaluateYield(int const* aiYield, CvPlot const* p = NULL, bool bCanNeverImprove = false, bool bTreatHomeAsCity = true) const;
	// <!-- custom: Return both the best XML-valid Build outcome and the weighted yield/timing details needed by the caller and diagnostics; this replaces separate terrain-, feature- and named-improvement assumptions. (GPT-5.6-Sol) -->
	int evaluateBestPotentialPlotYield(CvPlot const& p, bool bCanNeverImprove, ImprovementTypes& eBestImprovement, int* aiBestYield, int& iTimingPercent) const;
	int evaluateFreshWater(CvPlot const& p, int const* aiYield, bool bSteal, int& iRiverTiles, int& iGreenTiles) const;
	// <!-- custom: removed and now added inline in parent caller AIFoundValue::evaluate() directly, as it seems to be called only once, and we'd have more parameters to fine tune it further in parent caller rather, it is also clearer this way i think -->
	//int foundOnResourceValue(int const* aiBonusImprovementYield) const;
	int applyCultureModifier(CvPlot const& p, int iPlotValue, int iCultureModifier, bool bShare) const;
	// <!-- custom: Base AdvCiv had no diagnostic pointer parameters here; our previous SAS diagnostics exposed baseBonusVal/multiplier/earlyPercent from the broad AI_bonusVal path. Replace those with explicit new health/happiness value, plausible ordinary and capped special-building health/happiness effects, final adjustment, and water-penalty diagnostics. The old first-growth/luxury extra is not a separate parameter because new health/happiness already represents that empire-wide effect; adding a flat first-bonus value would double-count it. See KI#178. (GPT-5.5 + GPT-5.6-Sol) -->
	bool isPlotInKnownRivalFutureBFC(CvPlot const& p) const;
	bool isBonusOwnedOrClaimedByFutureBFC(BonusTypes eBonus) const;
	bool isBonusBuildingEffectValued(BuildingTypes eBuilding, int iMaxPrereqEra, bool bCoastal) const;
	int calculateBonusBuildingHappyHealthValue(BonusTypes eBonus, bool bCoastal) const;
	int calculateBonusConnectionEra(BonusTypes eBonus, CvPlot const& p) const;
	int nonYieldBonusValue(CvPlot const& p, BonusTypes eBonus, bool bCanTrade, bool bEasyAccess, bool bCoastal, std::vector<int>* paiBonusCount, int iCultureModifier, int* piHappyHealthValue = NULL, int* piBuildingHappyHealthValue = NULL, int* piAdjustPercent = NULL, int* piWaterPenalty = NULL) const;
	int calculateSpecialYieldModifier(int iCultureModifier, bool bEasyAccess, bool bBonus, bool bCanSoonImproveBonus, bool bCanImproveBonus) const;
	void calculateSpecialYields(CvPlot const& p, int const* aiBonusImprovementYield, int const* aiNatureYield, int iModifier, int* aiSpecialYield, int& iSpecialFoodPlus, int& iSpecialFoodMinus, int& iSpecialYieldTiles) const;
	void calculateBuildingYields(CvPlot const& p, int const* aiNatureYield, int* aiBuildingYield) const;
	// <!-- custom: The weighted site total still uses the complete sorted BFC, while optional outputs expose best-1/2/3/6/10/14 sums, their cutoff plots and the positive-plot count from that same sort without rescanning or changing the score. (GPT-5.6-Sol) -->
	int sumUpPlotValues(std::vector<int>& aiPlotValues, int* aiCoreSums = NULL, int* aiCoreCutoffs = NULL, int* piPositivePlots = NULL) const;
	// <!-- custom: Disabled after XML-tunable SAS bonus-improvement yield valuation made this obscure hardcoded path redundant and retaining both overscored bonus-heavy sites in KI#173 follow-up testing. Kept commented with its implementation for reference. (GPT-5.5) -->
	// int evaluateSpecialYields(int const* aiSpecialYield, int iSpecialYieldTiles, int iSpecialFoodPlus, int iSpecialFoodMinus) const;
	// <!-- custom: simplify logic and attempt to spread cities more, currently they are way too crowded which is inefficient -->
	// bool isTooManyTakenTiles(int iTaken, int iResourceValue, bool bLowValue) const;
	bool isTooManyTakenTiles(int iTaken, int iResourceValue) const;
	int evaluateLongTermHealth(int& iHealthPercent) const;
	int evaluateFeatureProduction(int iProduction) const;
	int evaluateSeaAccess(bool bGoodFirstColony, scaled rProductionModifier, int iLandTiles) const;
	// <!-- custom: Pass the remaining workable-plot value so inherent post-founding defense remains a proportional economic tiebreaker instead of AdvCiv's fixed Hill-only bonus or a multiplier on unrelated site rewards. (GPT-5.6-Sol) -->
	int evaluateDefense(int iWorkablePlotValue) const;
	int evaluateGoodies(int iGoodies) const;
	int adjustToLandAreaBoundary(int iValue) const;
	int adjustToStartingSurroundings(int iValue) const;
	int adjustToStartingChoices(int iValue) const;
	// <!-- custom: try to remove this interference as we have a finer algorithm now, and this old code may lead to unexpected results -->
	// int adjustToFood(int iValue, int iSpecialFoodPlus, int iSpecialFoodMinus,
	// 		int iGreenTiles) const;
	// <!-- custom: see code comment there for details -->
	// int adjustToProduction(int iValue, scaled rBaseProduction) const;
	// <!-- custom: Barbarian spawning already requires eligible fogged land; disable Base AdvCiv's additional penalty against a new Barbarian city near an existing city because it could make a stronger nearby tile lose to a weaker farther one. This never controlled normal Settlers avoiding Barbarian cities. (GPT-5.6-Sol) -->
	//int adjustToBarbarianSurroundings(int iValue) const;
	// <!-- custom: this adjustToCivSurroundings caused a bug of AI settler settling on bonus camel desert which is very bad in a desert surroudning even worse, it is seemingly called only once in AIFoundValue::evaluate, may as well disable it since it is so complicated and who knows where the bugs is(/are?) and instead migrate only a very simplified version of the logic we want directly inline in its only caller so in AIFoundValue::evaluate, done so with the help of chatgpt 5, check if accurate, see known issue as of now 54 for details -->
	// int adjustToCivSurroundings(int iValue, int iStealPercent) const;
	int adjustToCitiesPerArea(int iValue) const;
	int adjustToBonusCount(int iValue, std::vector<int> const& aiBonusCount) const;
	// <!-- custom: try to remove this interference as we have a finer algorithm now, and this old code may lead to unexpected results -->
	// int adjustToBadTiles(int iValue, int iBadTiles) const;
	// <!-- custom: same -->
	//int adjustToBadHealth(int iValue, int iGoodHealth) const;
	int countDeadlockedBonuses() const;
	bool isDeadlockedBonus(CvPlot const& kBonusPlot, int iMinRange) const;
};

#endif
