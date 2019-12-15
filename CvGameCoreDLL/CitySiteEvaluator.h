#pragma once

#ifndef CITY_SITE_EVALUATOR_H
#define CITY_SITE_EVALUATOR_H

// advc: New classes extracted from CvPlayerAI::AI_foundValue
class CvPlayerAI;
class CvTeamAI;
class CvCityAI;
class CvPlot;
class CvArea;
class CvGame;

// Corresponds to K-Mod's CvPlayerAI::CvFoundSettings
class CitySiteEvaluator
{
public:
	CitySiteEvaluator(CvPlayerAI const& kPlayer, int iMinRivalRange = -1,
			bool bStartingLoc = false);
	short evaluate(CvPlot const& kPlot) const;
	short evaluate(int iX, int iY) const;
	short evaluateWithLogging(int iX, int iY) const; // advc.031c
	CvPlayerAI const& getPlayer() const { return m_kPlayer; }
	bool isStartingLoc() const { return m_bStartingLoc; }
	int getMinRivalRange() const { return m_iMinRivalRange; }
	// <advc.300>
	void discourageBarbarians(int iRange);
	int getBarbarianDiscouragedRange() const { return m_iBarbDiscouragedRange; }
	// </advc.300>
	bool isAdvancedStart() const { return m_bAdvancedStart; } // advc
	/*  <advc.007> Ignores whether kPlot is close to another tentative city site
		and treats kPlayer as non-human */
	void setDebug(bool b);
	bool isDebug() const { return m_bDebug; }
	// </advc.007>
	int getGreed() const; // advc: Mostly obsolete
	// (The comments below about the found settings are from K-Mod)
	// culture required to pop the 2nd borders (as in BtS)
	int getClaimThreshold() const { return m_iClaimThreshold; }
	// doesn't need vision of a plot to know what's there [but doesn't reveal resources]
	bool isAllSeeing() const { return m_bAllSeeing; }
	// some trait information that will influence where we settle ...
	// easy for us to pop the culture to the 2nd border
	bool isEasyCulture() const { return m_bEasyCulture; }
	// expectation of taking foreign land, either by culture or by force
	bool isAmbitious() const { return m_bAmbitious; }
	// more value for rivers
	bool isExtraCommerceThreshold() const { return m_bFinancial; }
	// more value for settlings on hills
	bool isDefensive() const { return m_bDefensive; }
	// special affection for coast cities due to unique building or unit.
	bool isSeafaring() const { return m_bSeafaring; }
	// willing to place cities further apart. (not directly based on the expansive trait)
	bool isExpansive() const { return m_bExpansive; }
	// <advc.031c>
	void log(int iX, int iY);
	void logSettings() const; // </advc.031c>

private:
	CvPlayerAI const& m_kPlayer;
	bool m_bStartingLoc;
	int m_iMinRivalRange;
	int m_iBarbDiscouragedRange; // advc.300
	bool m_bAdvancedStart; // advc
	int m_bDebug; // advc.007
	bool m_bAllSeeing;
	int m_iClaimThreshold;
	int m_iGreed;
	bool m_bEasyCulture;
	bool m_bAmbitious;
	bool m_bFinancial;
	bool m_bDefensive;
	bool m_bSeafaring;
	bool m_bExpansive;
};

/*  AIFoundValue::evaluate corresponds to K-Mod's CvPlayerAI::AI_foundValue_bulk
	and gets called by the constructor. */
class AIFoundValue
{
public:
	AIFoundValue(CvPlot const& kPlot, CitySiteEvaluator const& kSettings);
	short get() const { return m_iResult; }

	// <advc.031c> Will have to enable the found log in BBAILog.h in addition
	static void setLoggingEnabled(bool b);
	static bool isLoggingEnabled() { return bLoggingEnabled; } // </advc.031c>

private:
	short m_iResult;
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
	int iX, iY;
	CvCityAI const* pCapital;
	bool bCoastal;
	int iAreaCities;
	int iCities;
	bool bNormalize;
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
	void logPlot(CvPlot const& p, int iPlotValue, int const* aiYield,
			int iCultureModifier, BonusTypes eBonus, ImprovementTypes eBonusImprovement,
			bool bCanTradeBonus, bool bCanSoonTradeBonus, bool bEasyAccess,
			int iFeatureProduction, bool bPersistentFeature, bool bRemovableFeature) const;
	// </advc.031c>
	short evaluate();
	// Subroutines of evaluate ...
	inline bool isHome(CvPlot const& p) const { return (&p == &kPlot); }
	bool isSitePromising() const;
	bool computeOverlap();
	bool isPrioritizeAsFirstColony() const; // advc.040
	int countBadTiles(int& iUnrevealed, int& iLand, int& iRevealedDecentLand) const;
	bool isTooManyBadTiles(int iBadTiles) const;
	int baseCityValue() const;
	bool isUsablePlot(int iPlot, int& iTakenTiles, bool& bCityRadius, bool& bForeignOwned,
			bool& bAnyForeignOwned, bool& bShare, bool& bSteal) const;
	bool isRemovableFeature(CvPlot const& p, bool& bPersistent,
			int& iFeatureProduction) const;
	bool isRevealed(CvPlot const& p) const;
	BonusTypes getBonus(CvPlot const& p) const;
	ImprovementTypes getBonusImprovement(BonusTypes eBonus,
			bool& bCanTrade, bool& bCanTradeSoon) const;
	bool isNearTech(TechTypes eTech) const;
	int calculateCultureModifier(CvPlot const& p, bool bForeignOwned, bool bShare,
			bool bCityRadius, bool bSteal, bool bFlip, bool bOwnExcl,
			int& iTakenTiles, int& iStealPercent) const;
	/*void adjustHomePlotYields(int* aiYield, BonusTypes eBonus,
			ImprovementTypes eImprovement, FeatureTypes ePersistentFeature) const;*/
	int estimateImprovementProduction(CvPlot const& p, bool bPersistentFeature) const;
	int evaluateYield(CvPlot const& p, int const* aiYield) const;
	int evaluateFreshWater(CvPlot const& p, int const* aiYield, bool bSteal,
			int& iRiverTiles, int& iGreenTiles) const;
	int applyCultureModifier(CvPlot const& p, int iPlotValue, int iCultureModifier,
			bool bShare) const;
	int nonYieldBonusValue(CvPlot const& p, BonusTypes eBonus, bool bCanTrade,
			bool bCanTradeSoon, bool bEasyAccess,
			std::vector<int>& aiBonusCount, int iCultureModifier) const;
	void calculateSpecialYields(CvPlot const& p, BonusTypes eBonus,
			ImprovementTypes eImprovement, int iCultureModifier, bool bEasyAccess,
			int iEffectiveFood, int* aiSpecialYield,
			int& iSpecialFoodPlus, int& iSpecialFoodMinus, int& iSpecialYieldTiles) const;
	int sumUpPlotValues(std::vector<int>& aiPlotValues) const;
	int evaluateSpecialYields(int const* aiSpecialYield, int iSpecialYieldTiles) const;
	bool isTooManyTakenTiles(int iTaken, int iResourceValue, bool bLowValue) const;
	double calculateFoodModifier(int iSpecialFoodPlus, int iSpecialFoodMinus,
			int iGreenTiles) const;
	int evaluateLongTermHealth(int& iHealthPercent) const;
	int evaluateFeatureProduction(int iProduction) const;
	int evaluateSeaAccess(bool bGoodFirstColony, double productionModifier,
			int iLandTiles) const;
	int evaluateDefense() const;
	int adjustToStartingSurroundings(int iValue) const;
	int adjustToProduction(int iValue, int iBaseProductionTimes100) const;
	int adjustToBarbarianSurroundings(int iValue) const;
	int adjustToCivSurroundings(int iValue, int iStealPercent) const;
	int adjustToCitiesPerArea(int iValue) const;
	int adjustToBonusCount(int iValue, std::vector<int> const& aiBonusCount) const;
	int adjustToBadTiles(int iValue, int iBadTiles) const;
	int adjustToBadHealth(int iValue, int iGoodHealth) const;
	int countDeadlockedBonuses() const;
	bool isDeadlockedBonus(CvPlot const& kBonusPlot, int iMinRange) const;
};

#endif
