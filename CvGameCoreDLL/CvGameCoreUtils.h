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

#define SQR(x) ((x)*(x))

// K-Mod: Created the following function for rounded integer division
// advc: Moved from CvGlobals.h and static specifier removed
inline int ROUND_DIVIDE(int a, int b)
{
	//return (a+((a/b>0)?1:-1)*(b/2)) / b;
	// <advc.001> ^That'll round 2/3 to 0
	int iSign = ((a ^ b) >= 0 ? 1 : -1);
	return (a + iSign * b / 2) / b;
	// </advc.001>
}

/*	<advc.opt> Non-branching implementations.
	MSVC produces branches for std::max and std::min. There is a cmov instruction,
	but 32-bit MSVC won't generate that. (There might be an intrinsic that we could
	use though.)
	Tbd.: Replace std::max, std::min calls with this? */
inline int max(int x, int y)
{
	return x ^ ((x ^ y) & -(x < y));
}

inline int min(int x, int y)
{
	return y + ((x - y) & -(x < y));
} // </advc.opt>
// <advc.003g>
// This is a better approach than the fmath stuff below
namespace stats // Seems too generic, but what else to name it?
{
	template<typename T>
	T median(std::vector<T>& kSamples, bool bSorted = false)
	{
		FAssert(!kSamples.empty());
		if (!bSorted)
			std::sort(kSamples.begin(), kSamples.end());
		int iMedian = kSamples.size() / 2;
		if (kSamples.size() % 2 != 0)
			return kSamples[iMedian];
		return (kSamples[iMedian] + kSamples[iMedian - 1]) / 2;
	}
	template<typename T>
	T max(std::vector<T> const& kSamples)
	{
		FAssert(!kSamples.empty());
		T r = kSamples[0];
		for(size_t i = 1; i < kSamples.size(); i++)
			r = std::max(r, kSamples[i]);
		return r;
	}
	template<typename T>
	T min(std::vector<T> const& kSamples)
	{
		FAssert(!kSamples.empty());
		T r = kSamples[0];
		for(size_t i = 1; i < kSamples.size(); i++)
			r = std::min(r, kSamples[i]);
		return r;
	}
	template<typename T>
	T mean(std::vector<T> const& kSamples)
	{
		FAssert(!kSamples.empty());
		T r = 0;
		for(size_t i = 0; i < kSamples.size(); i++)
			r += kSamples[i];
		return r / static_cast<T>(kSamples.size());
	}
	// Count the number of samples in the closed interval [nLower,nUpper]
	template<typename T>
	int intervalFreq(std::vector<T> const& kSamples, T nLower, T nUpper)
	{
		int r = 0;
		for(size_t i = 0; i < kSamples.size(); i++)
		{
			if (kSamples[i] >= nLower && kSamples[i] <= nUpper)
				r++;
		}
		return r;
	}
}
//namespace fmath // (For the time being, these functions are used too frequently for a namespace.)
//{
	inline int round(double d) { return (int)((d >= 0 ? 0.5 : -0.5) + d); }
	int roundToMultiple(double d, int iMultiple);
	bool bernoulliSuccess(double pr, // 0 <= pr <= 1
			char const* pszLog = "", bool bAsync = false,
			int iData1 = MIN_INT, int iData2 = MIN_INT);
	inline double dMedian(std::vector<double>& distribution, bool bSorted = false)
	{
		return stats::median(distribution, bSorted);
	}
	// see e.g. Wikipedia: "percentile rank"
	double percentileRank(std::vector<double>& distribution, double score,
			bool bSorted = false, // Is the distribution sorted (ascending)?
			bool bScorePartOfDistribution = true); /* Is 'score' to be considered as
			an element of the distribution? If yes, the percentile rank is going to be
			positive. Either way, the caller shouldn't include 'score' in the distribution. */

	/*  Hash based on the components of x. Plot index of capital factored in for
		increased range if ePlayer given. (ePlayer is ignored if it has no capital.) */
	int intHash(std::vector<int> const& x, PlayerTypes ePlayer = NO_PLAYER);
	/*	See intHash about the parameters.
		Result between 0 and 1. Returns float b/c CvRandom uses float (not double).
		(Similar but more narrow: CvUnitAI::AI_unitBirthmarkHash, AI_unitPlotHash) */
	inline float hash(std::vector<int> const& x, PlayerTypes ePlayer = NO_PLAYER)
	{
		/*  Use ASyncRand to avoid the overhead of creating a new object?
			Or use stdlib's rand/sRand? I don't think it matters. */
		/*CvRandom& rng = GC.getASyncRand();
		rng.reset(hashVal);*/
		CvRandom rng;
		rng.init(intHash(x, ePlayer));
		return rng.getFloat();
	}
	// For hashing just a single input
	inline float hash(int x, PlayerTypes ePlayer = NO_PLAYER)
	{
		std::vector<int> v;
		v.push_back(x);
		return hash(v, ePlayer);
	}
//} // </advc.003g>

void contestedPlots(std::vector<CvPlot*>& r, TeamTypes t1, TeamTypes t2); // advc.035
// <advc.130h>
template<typename T> void removeDuplicates(std::vector<T>& v)
{
	std::set<T> aeTmp(v.begin(), v.end());
	v.assign(aeTmp.begin(), aeTmp.end());
} // </advc.130h>
// <advc>
// Erik: "Back-ported" from C++11
namespace std11
{
template<class ForwardIt, class T>
void iota(ForwardIt first, ForwardIt last, T value)
{
	while (first != last)
	{
		*first++ = value;
		++value;
	}
}
}; // </advc>
// advc.004w:
void applyColorToString(CvWString& s, char const* szColor, bool bLink = false);
void narrowUnsafe(CvWString const& szWideString, CvString& szNarowString); // advc

// (advc: getSign move to CvPlot.cpp)

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
inline double dRange(double d, double low, double high)
{
	if(d < low)
		return low;
	if(d > high)
		return high;
	return d;
} // </advc.003g>

// advc: Body cut from CvUnitAI::AI_sacrificeValue. (K-Mod had used long -> int.)
inline int longLongToInt(long long x)
{
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

inline short intToShort(int x)
{
	FAssert(x < MAX_SHORT);
	return static_cast<short>(std::min<int>(MAX_SHORT, x));
}

inline char intToChar(int x)
{
	FAssert(x < MAX_CHAR);
	return static_cast<char>(std::min<char>(MAX_CHAR, x));
}

float colorDifference(NiColorA const& c1, NiColorA const& c2); // advc.002i

// (advc.make: Distance functions moved into CvMap.h)

inline CardinalDirectionTypes getOppositeCardinalDirection(CardinalDirectionTypes eDir)	// Exposed to Python
{
	return (CardinalDirectionTypes)((eDir + 2) % NUM_CARDINALDIRECTION_TYPES); // advc.inl
}
DirectionTypes cardinalDirectionToDirection(CardinalDirectionTypes eCard);				// Exposed to Python
DllExport bool isCardinalDirection(DirectionTypes eDirection);															// Exposed to Python
DirectionTypes estimateDirection(int iDX, int iDY);																// Exposed to Python
DllExport DirectionTypes estimateDirection(const CvPlot* pFromPlot, const CvPlot* pToPlot);

bool atWar(TeamTypes eTeamA, TeamTypes eTeamB);										// Exposed to Python
//isPotentialEnemy(TeamTypes eOurTeam, TeamTypes eTheirTeam); // advc: Use CvTeamAI::AI_mayAttack instead

/*	(advc.opt: getCity, getUnit moved to CvPlayer.h. CvCity::fromIDInfo and
	CvUnit::fromIDInfo as alternatives in files that don't include CvPlayer.h.)

// (advc: unit cycling functions moved to CvSelectionGroup, CvUnit)

/*	advc: asset score functions moved to CvGame; no longer exposed to Python.
	isPromotionValid moved to CvUnitInfo, finalImprovementUpgrade to CvImprovementInfo,
	getEspionageModifier to CvTeam, getWorldSizeMaxConscript to CvGame (as getMaxConscript;
	no longer exposed to Python). */
/*	advc.003w: Moved some two dozen functions to CvInfo classes;
	mostly functions dealing with building and unit class limitations.
	Removed isTechRequiredForProject. */

__int64 getBinomialCoefficient(int iN, int iK);
int getCombatOdds(const CvUnit* pAttacker, const CvUnit* pDefender);				// Exposed to Python
int estimateCollateralWeight(const CvPlot* pPlot, TeamTypes eAttackTeam, TeamTypes eDefenceTeam = NO_TEAM); // K-Mod

/*	advc (note): Still used in the DLL by CvPlayer::buildTradeTable, but mostly deprecated.
	Use the TradeData constructor instead. */
DllExport void setTradeItem(TradeData* pItem, TradeableItems eItemType = TRADE_ITEM_NONE, int iData = 0);

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
// advc.ctr:
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
int pathValid_join(FAStarNode* parent, FAStarNode* node, CvSelectionGroup const* pSelectionGroup, int iFlags); // K-Mod
int pathValid_source(FAStarNode* parent, CvSelectionGroup const* pSelectionGroup, int iFlags); // K-Mod
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
void shuffleVector(std::vector<int>& aiIndices, CvRandom& rand); // advc.enum

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
