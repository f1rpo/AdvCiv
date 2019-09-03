#pragma once

// utils.h

#ifndef CIV4_GAMECORE_UTILS_H
#define CIV4_GAMECORE_UTILS_H

class CvPlot;
class CvCity;
class CvCityAI; // advc.003u
class CvUnit;
class CvUnitAI; // advc.003u
class CvSelectionGroup;
class CvString;
class CvRandom;
class FAStarNode;
class FAStar;
class CvInfoBase;

// advc.003j: Unused here and elsewhere; still defined in CvGameCoreDLL.
//#ifndef SQR
//#define SQR(x) ( (x)*(x))
//#endif*

// <advc.003g> floating point utility
inline int round(double d) { return (int)((d >= 0 ? 0.5 : -0.5) + d); }
int roundToMultiple(double d, int iMultiple);
bool bernoulliSuccess(double pr, // 0 <= pr <= 1
		char const* pszLog = "", bool bAsync = false,
		int iData1 = MIN_INT, int iData2 = MIN_INT);
double dMedian(std::vector<double>& distribution, bool bSorted = false);
double dMean(std::vector<double> const& distribution);
double dMax(std::vector<double> const& distribution);
double dMin(std::vector<double> const& distribution);
// see e.g. Wikipedia: "percentile rank"
double percentileRank(std::vector<double>& distribution, double score,
		bool bSorted = false, // Is the distribution sorted (ascending)?
		bool bScorePartOfDistribution = true); /* Is 'score' to be considered as
		an element of the distribution? If yes, the percentile rank is going to be
		positive. Either way, the caller shouldn't include 'score' in the distribution. */
// </advc.003g>
// <advc>
/*  Hash based on the components of x. Plot index of capital factored in for
	increased range if ePlayer given. (ePlayer is ignored if it has no capital.)
	Result between 0 and 1. Returns float b/c CvRandom uses float (not double). */
float hash(std::vector<long> const& x, PlayerTypes ePlayer = NO_PLAYER);
// For hashing just a single input
float hash(long x, PlayerTypes ePlayer = NO_PLAYER);
/*  'r' is an empty vector in which the 21 CvPlot* in the city radius ("fat cross")
	around p will be placed. &p itself gets placed in r[0]; the others in
	no particular order. If the city cross has fewer than 21 plots (edge of the map),
	then NULL entries will be included.
	For iterating over tiles in the radius of a CvCity, CvCity::getCityIndexPlot
	is faster; but cityCross doesn't require a CvCity to exist in p. */
void cityCross(CvPlot const& p, std::vector<CvPlot*>& r);
// </advc>
void contestedPlots(std::vector<CvPlot*>& r, TeamTypes t1, TeamTypes t2); // advc.035
// <advc.008e>
bool needsArticle(BuildingTypes eBuilding);
bool needsArticle(ProjectTypes eProject);
// </advc.008e>
// <advc.130h>
template<typename T> void removeDuplicates(std::vector<T>& v) {
	std::set<T> aeTmp(v.begin(), v.end());
	v.assign(aeTmp.begin(), aeTmp.end());
} // </advc.130h>
// advc.004w:
void applyColorToString(CvWString& s, char const* szColor, bool bLink = false);
void narrowUnsafe(CvWString const& szWideString, CvString& szNarowString); // advc

//sign function taken from FirePlace - JW
template<class T> __forceinline T getSign( T x ) { return (( x < 0 ) ? T(-1) : x > 0 ? T(1) : T(0)); };

inline int range(int iNum, int iLow, int iHigh)
{
	FAssertMsg(iHigh >= iLow, "High should be higher than low");

	if (iNum < iLow)
		return iLow;
	else if (iNum > iHigh)
		return iHigh;
	else return iNum;
}

inline float range(float fNum, float fLow, float fHigh)
{
	FAssertMsg(fHigh >= fLow, "High should be higher than low");

	if (fNum < fLow)
		return fLow;
	else if (fNum > fHigh)
		return fHigh;
	else return fNum;
}

/*  <advc.003g> Don't want to work with float in places where memory usage isn't a
	concern. */
inline double dRange(double d, double low, double high) {

	if(d < low)
		return low;
	if(d > high)
		return high;
	return d;
} // </advc.003g>

// advc: Body cut from CvUnitAI::AI_sacrificeValue. (K-Mod had used long -> int.)
inline int longLongToInt(long long x) {

	FAssert(x < MAX_INT);
	//return std::min((long)MAX_INT, iValue); // K-Mod
	/*  Erik (BUG1): We cannot change the signature [of AI_sacrificeValue] due to
		the virtual specifier so we have to truncate the final value to an int. */
	/*	Igor: if iValue is greater than MAX_INT, std::min<long long> ensures that it is truncated to MAX_INT, which makes sense logically.
		static_cast<int>(iValue) doesn't guarantee that and the resulting value is implementation-defined. */
	//return static_cast<int>(std::min(static_cast<long long>(MAX_INT), x));
	/*  advc: Can't use std::min as above here, probably b/c of a conflicting definition
		in windows.h. No matter: */
	return static_cast<int>(std::min<long long>(MAX_INT, x));
}

inline short intToShort(int x) {

	FAssert(x < MAX_SHORT);
	return static_cast<short>(std::min<int>(MAX_SHORT, x));
}

// (advc.make: Distance functions moved into CvMap.h)

CvPlot* plotCity(int iX, int iY, int iIndex);																			// Exposed to Python
int plotCityXY(int iDX, int iDY);																									// Exposed to Python
int plotCityXY(const CvCity* pCity, const CvPlot* pPlot);													// Exposed to Python
bool isInnerRing(CvPlot const* pPlot, CvPlot const* pCityPlot); // advc.303

CardinalDirectionTypes getOppositeCardinalDirection(CardinalDirectionTypes eDir);	// Exposed to Python
DirectionTypes cardinalDirectionToDirection(CardinalDirectionTypes eCard);				// Exposed to Python
DllExport bool isCardinalDirection(DirectionTypes eDirection);															// Exposed to Python
DirectionTypes estimateDirection(int iDX, int iDY);																// Exposed to Python
DllExport DirectionTypes estimateDirection(const CvPlot* pFromPlot, const CvPlot* pToPlot);
DllExport float directionAngle(DirectionTypes eDirection);

bool atWar(TeamTypes eTeamA, TeamTypes eTeamB);												// Exposed to Python
bool isPotentialEnemy(TeamTypes eOurTeam, TeamTypes eTheirTeam);					// Exposed to Python

// <advc.003u>
CvCityAI* AI_getCity(IDInfo city);
CvUnitAI* AI_getUnit(IDInfo unit); // </advc.003u>
DllExport CvCity* getCity(IDInfo city);												// Exposed to Python
DllExport CvUnit* getUnit(IDInfo unit);												// Exposed to Python

// (advc.make: inlined isCycleGroup moved to CvSelectionGroup to avoid a dependency)
bool isBeforeUnitCycle(const CvUnit* pFirstUnit, const CvUnit* pSecondUnit);
bool isBeforeGroupOnPlot(const CvSelectionGroup* pFirstGroup, const CvSelectionGroup* pSecondGroup); // K-Mod
int groupCycleDistance(const CvSelectionGroup* pFirstGroup, const CvSelectionGroup* pSecondGroup); // K-Mod
bool isPromotionValid(PromotionTypes ePromotion, UnitTypes eUnit, bool bLeader);	// Exposed to Python

int getPopulationAsset(int iPopulation);											// Exposed to Python
int getLandPlotsAsset(int iLandPlots);												// Exposed to Python
int getPopulationPower(int iPopulation);											// Exposed to Python
int getPopulationScore(int iPopulation);											// Exposed to Python
int getLandPlotsScore(int iLandPlots);												// Exposed to Python
int getTechScore(TechTypes eTech);													// Exposed to Python
int getWonderScore(BuildingClassTypes eWonderClass);								// Exposed to Python

//ImprovementTypes finalImprovementUpgrade(ImprovementTypes eImprovement, int iCount = 0);		// Exposed to Python
ImprovementTypes finalImprovementUpgrade(ImprovementTypes eImprovement);			// Exposed to Python, K-Mod. (I've removed iCount here, and in the python defs. It's a meaningless parameter.)

int getWorldSizeMaxConscript(CivicTypes eCivic);									// Exposed to Python

bool isReligionTech(TechTypes eTech);														// Exposed to Python
// advc.003j: Unused BtS function; wasn't even declared in the header file.
bool isCorporationTech(TechTypes eTech);

bool isTechRequiredForUnit(TechTypes eTech, UnitTypes eUnit);							// Exposed to Python
bool isTechRequiredForBuilding(TechTypes eTech, BuildingTypes eBuilding);			// Exposed to Python
bool isTechRequiredForProject(TechTypes eTech, ProjectTypes eProject);				// Exposed to Python

bool isWorldUnitClass(UnitClassTypes eUnitClass);											// Exposed to Python
bool isTeamUnitClass(UnitClassTypes eUnitClass);											// Exposed to Python
bool isNationalUnitClass(UnitClassTypes eUnitClass);									// Exposed to Python
bool isLimitedUnitClass(UnitClassTypes eUnitClass);										// Exposed to Python
bool isMundaneBuildingClass(BuildingClassTypes eBuildingClass); // advc.104
bool isWorldWonderClass(BuildingClassTypes eBuildingClass);							// Exposed to Python
bool isTeamWonderClass(BuildingClassTypes eBuildingClass);							// Exposed to Python
bool isNationalWonderClass(BuildingClassTypes eBuildingClass);						// Exposed to Python
bool isLimitedWonderClass(BuildingClassTypes eBuildingClass);						// Exposed to Python
// <advc.003w> For convenience
bool isWorldUnitClass(UnitTypes eUnit);
bool isLimitedUnitClass(UnitTypes eUnit);
bool isWorldWonderClass(BuildingTypes eBuilding);
bool isLimitedWonderClass(BuildingTypes eBuilding);
// </advc.003w>
int limitedWonderClassLimit(BuildingClassTypes eBuildingClass);

bool isWorldProject(ProjectTypes eProject);														// Exposed to Python
bool isTeamProject(ProjectTypes eProject);														// Exposed to Python
bool isLimitedProject(ProjectTypes eProject);													// Exposed to Python

__int64 getBinomialCoefficient(int iN, int iK);
int getCombatOdds(const CvUnit* pAttacker, const CvUnit* pDefender);				// Exposed to Python
int estimateCollateralWeight(const CvPlot* pPlot, TeamTypes eAttackTeam, TeamTypes eDefenceTeam = NO_TEAM); // K-Mod

int getEspionageModifier(TeamTypes eOurTeam, TeamTypes eTargetTeam);							// Exposed to Python

DllExport void setTradeItem(TradeData* pItem, TradeableItems eItemType = TRADE_ITEM_NONE, int iData = 0);

bool isPlotEventTrigger(EventTriggerTypes eTrigger);

TechTypes getDiscoveryTech(UnitTypes eUnit, PlayerTypes ePlayer);

void setListHelp(wchar* szBuffer, const wchar* szStart, const wchar* szItem, const wchar* szSeparator, bool bFirst);
void setListHelp(CvWString& szBuffer, const wchar* szStart, const wchar* szItem, const wchar* szSeparator, bool bFirst);
void setListHelp(CvWStringBuffer& szBuffer, const wchar* szStart, const wchar* szItem, const wchar* szSeparator, bool bFirst);

// PlotUnitFunc's...
bool PUF_isGroupHead( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_isPlayer( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isTeam( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isCombatTeam(const CvUnit* pUnit, int iData1, int iData2);
bool PUF_isOtherPlayer( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isOtherTeam( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isEnemy( const CvUnit* pUnit, int iData1, int iData2 = -1);
// advc.122:
bool PUF_isEnemyCityAttacker( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isVisible( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isVisibleDebug( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_canSiege( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isPotentialEnemy( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_canDeclareWar( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_canDefend( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_cannotDefend( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_canDefendGroupHead( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_canDefendEnemy( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_canDefendPotentialEnemy( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_canAirAttack( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_canAirDefend( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_isFighting( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isAnimal( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_isMilitaryHappiness( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_isInvestigate( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_isCounterSpy( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_isSpy( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_isUnitType( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isDomainType( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isUnitAIType( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isCityAIType( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isNotCityAIType( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isSelected( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_makeInfoBarDirty(CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
//bool PUF_isNoMission(const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
// advc.113b:
bool PUF_isMissionPlotWorkingCity(const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_isFiniteRange(const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
// bbai start
bool PUF_isAvailableUnitAITypeGroupie(const CvUnit* pUnit, int iData1, int iData2);
bool PUF_isUnitAITypeGroupie(const CvUnit* pUnit, int iData1, int iData2);
bool PUF_isFiniteRangeAndNotJustProduced(const CvUnit* pUnit, int iData1, int iData2);
// bbai end
bool PUF_isMissionAIType(const CvUnit* pUnit, int iData1, int iData2); // K-Mod
bool PUF_isAirIntercept(const CvUnit* pUnit, int iData1, int iData2); // K-Mod

// FAStarFunc...
int potentialIrrigation(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int checkFreshWater(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int changeIrrigated(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int pathDestValid(int iToX, int iToY, const void* pointer, FAStar* finder);
int pathHeuristic(int iFromX, int iFromY, int iToX, int iToY);
int pathCost(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int pathValid_join(FAStarNode* parent, FAStarNode* node, CvSelectionGroup* pSelectionGroup, int iFlags); // K-Mod
int pathValid_source(FAStarNode* parent, CvSelectionGroup* pSelectionGroup, int iFlags); // K-Mod
int pathValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int pathAdd(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int stepDestValid(int iToX, int iToY, const void* pointer, FAStar* finder);
// advc.104b:
int stepDestValid_advc(int iToX, int iToY, const void* pointer, FAStar* finder);
int stepHeuristic(int iFromX, int iFromY, int iToX, int iToY);
int stepValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int stepCost(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int stepAdd(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
// BETTER_BTS_AI_MOD, 11/30/08, jdog5000:
int teamStepValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
// advc.104b:
int teamStepValid_advc(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int routeValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int borderValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int areaValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int joinArea(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int plotGroupValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int countPlotGroup(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);

int baseYieldToSymbol(int iNumYieldTypes, int iYieldStack);
//bool isPickableName(const TCHAR* szName); // advc.003j

DllExport int* shuffle(int iNum, CvRandom& rand);
void shuffleArray(int* piShuffle, int iNum, CvRandom& rand);

int getTurnMonthForGame(int iGameTurn, int iStartYear, CalendarTypes eCalendar, GameSpeedTypes eSpeed);
int getTurnYearForGame(int iGameTurn, int iStartYear, CalendarTypes eCalendar, GameSpeedTypes eSpeed);

void getDirectionTypeString(CvWString& szString, DirectionTypes eDirectionType);
void getCardinalDirectionTypeString(CvWString& szString, CardinalDirectionTypes eDirectionType);
void getActivityTypeString(CvWString& szString, ActivityTypes eActivityType);
void getMissionTypeString(CvWString& szString, MissionTypes eMissionType);
void getMissionAIString(CvWString& szString, MissionAITypes eMissionAI);
void getUnitAIString(CvWString& szString, UnitAITypes eUnitAI);

// Lead From Behind by UncutDragon
int LFBgetCombatOdds(int iAttackerLowFS, int iAttackerHighFS, int iDefenderLowFS, int iDefenderHighFS, int iNeededRoundsAttacker, int iNeededRoundsDefender, int iAttackerOdds);

#endif
