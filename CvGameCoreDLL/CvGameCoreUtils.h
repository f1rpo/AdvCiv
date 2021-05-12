#pragma once

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

/*	(advc: Can also use ScaledNum. Maybe these functions read a bit better in
	code that doesn't use fractions much. The "u" versions -- only for
	nonnegative numbers -- are also a bit more efficient than ScaledNum.) */
namespace intdiv
{
	// Replacing K-Mod's (incorrectly implemented) ROUND_DIVIDE (CvGlobals.h)
	inline int round(int iDividend, int iDivisor)
	{
		int iSign = ((iDividend ^ iDivisor) >= 0 ? 1 : -1);
		return (iDividend + iSign * iDivisor / 2) / iDivisor;
	}

	inline int uround(int iDividend, int iDivisor)
	{
		FAssert((iDividend ^ iDivisor) >= 0); // Both negative is OK
		return (iDividend + iDivisor / 2) / iDivisor;
	}

	inline int uceil(int iDividend, int iDivisor)
	{
		FAssert(iDividend >= 0 && iDivisor > 0);
		return 1 + (iDividend - 1) / iDivisor;
	}
}

/*	advc.opt: MSVC produces branches for std::max and std::min.
	There is a cmov instruction, but 32-bit MSVC won't generate that.
	(There might be an intrinsic that we could use though.)
	Tbd.: Replace more std::max, std::min calls with this? */
namespace branchless
{
	inline int max(int x, int y)
	{
		return x ^ ((x ^ y) & -(x < y));
	}

	inline int min(int x, int y)
	{
		return y + ((x - y) & -(x < y));
	}
}
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
namespace std11
{
// Erik: "Back-ported" from C++11
template<class ForwardIt, class T>
void iota(ForwardIt first, ForwardIt last, T value)
{
	while (first != last)
	{
		*first++ = value;
		value++;
	}
}
}; // </advc>

/*	advc: For arrays initialized through brace-enclosed list.
	The array must not be empty. Corresponds to std::size in C++17
	(but we don't have constexpr, so it has to be a macro). */
#define ARRAY_LENGTH(aArray) (sizeof(aArray) / sizeof(aArray[0]))

// advc.004w:
void applyColorToString(CvWString& s, char const* szColor, bool bLink = false);
void narrowUnsafe(CvWString const& szWideString, CvString& szNarowString); // advc

// (advc: getSign move to CvPlot.cpp)

inline int range(int iNum, int iLow, int iHigh)
{
	FAssert(iHigh >= iLow);

	if (iNum < iLow)
		return iLow;
	else if (iNum > iHigh)
		return iHigh;
	else return iNum;
}

inline float range(float fNum, float fLow, float fHigh)
{
	FAssert(fHigh >= fLow);

	if (fNum < fLow)
		return fLow;
	else if (fNum > fHigh)
		return fHigh;
	else return fNum;
}

// advc.003g:
inline double dRange(double d, double low, double high)
{
	if(d < low)
		return low;
	if(d > high)
		return high;
	return d;
}

// advc: Body cut from CvUnitAI::AI_sacrificeValue
inline int longLongToInt(long long x)
{
	FAssert(x <= MAX_INT && x >= MIN_INT);
	//return std::min((long)MAX_INT, iValue); // K-Mod
	/*	Igor: if iValue is greater than MAX_INT, std::min<long long> ensures that
		it is truncated to MAX_INT, which makes sense logically.
		static_cast<int>(iValue) doesn't guarantee that and the resulting value is implementation-defined. */
	//return static_cast<int>(std::min(static_cast<long long>(MAX_INT), x));
	/*  advc: Can't use std::min as above here, probably b/c of a conflicting definition
		in windows.h. No matter: */
	return static_cast<int>(std::max<long long>(std::min<long long>(MAX_INT, x), MIN_INT));
}
// <advc.make> Enabling level-4 c4244 warnings makes such functions pretty indispensable
template<typename T>
inline short toShort(T x)
{
	BOOST_STATIC_ASSERT(sizeof(T) > sizeof(short));
	FAssert(x <= MAX_SHORT && x >= MIN_SHORT);
	return static_cast<short>(std::max<T>(std::min<T>(MAX_SHORT, x), MIN_SHORT));
}

template<typename T>
inline char toChar(T x)
{
	BOOST_STATIC_ASSERT(sizeof(T) > sizeof(char));
	FAssert(x <= MAX_CHAR && x >= MIN_CHAR);
	return static_cast<char>(std::max<T>(std::min<T>(MAX_CHAR, x), MIN_CHAR));
}

template<typename T>
inline wchar toWChar(T x)
{
	BOOST_STATIC_ASSERT(sizeof(T) > sizeof(wchar));
	FAssert(x <= WCHAR_MAX && x >= WCHAR_MIN);
	return static_cast<wchar>(std::max<T>(std::min<T>(WCHAR_MAX, x), WCHAR_MIN));
} // </advc.make>

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

// advc: Moved from CvXMLLoadUtility. (CvHotKeyInfo might be an even better place?)
namespace hotkeyDescr
{
	CvWString keyStringFromKBCode(TCHAR const* szDescr);
	CvWString hotKeyFromDescription(TCHAR const* szDescr,
			bool bShift = false, bool bAlt = false, bool bCtrl = false);
}

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
// advc: getCombatOdds, LFBgetCombatOdds moved to CombatOdds

int estimateCollateralWeight(const CvPlot* pPlot, TeamTypes eAttackTeam, TeamTypes eDefenceTeam = NO_TEAM); // K-Mod

/*	advc (note): Still used in the DLL by CvPlayer::buildTradeTable, but mostly deprecated.
	Use the TradeData constructor instead. */
DllExport void setTradeItem(TradeData* pItem, TradeableItems eItemType = TRADE_ITEM_NONE, int iData = 0);

/*	advc: Unused. Thought about moving these to CvGameTextMgr,
	but that'll lead to more header inclusions. */
//void setListHelp(wchar* szBuffer, const wchar* szStart, const wchar* szItem, const wchar* szSeparator, bool bFirst);
void setListHelp(CvWString& szBuffer, wchar const* szStart, wchar const* szItem,
		wchar const* szSeparator, bool& bFirst); // advc: bool&
void setListHelp(CvWStringBuffer& szBuffer, wchar const* szStart, wchar const* szItem,
		wchar const* szSeparator, bool& bFirst); // advc: bool&
/*	<advc> Add variants for items that can go into one list only when a value
	matches the most recently added item. (This stuff should really be wrapped
	into a class.) */
void setListHelp(CvWString& szBuffer, wchar const* szStart, wchar const* szItem,
		wchar const* szSeparator, int& iLastListID, int iListID);
void setListHelp(CvWStringBuffer& szBuffer, wchar const* szStart, wchar const* szItem,
		wchar const* szSeparator, int& iLastListID, int iListID); // </advc>

// PlotUnitFunc's...  (advc: Parameters iData1, iData2 renamed)
bool PUF_isGroupHead(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1);
bool PUF_isPlayer(CvUnit const* pUnit, int iOwner, int iForTeam = NO_TEAM);
bool PUF_isTeam(CvUnit const* pUnit, int iTeam, int iDummy = -1);
bool PUF_isCombatTeam(CvUnit const* pUnit, int iTeam, int iForTeam);
bool PUF_isOtherPlayer(CvUnit const* pUnit, int iPlayer, int iDummy = -1);
bool PUF_isOtherTeam(CvUnit const* pUnit, int iPlayer, int iDummy = -1);
bool PUF_canDefend(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1);
bool PUF_cannotDefend(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1);
bool PUF_canDefendGroupHead(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1);
bool PUF_canDefendPotentialEnemy(CvUnit const* pUnit, int iPlayer, BOOL iAlwaysHostile = false);
bool PUF_canDefendEnemy(CvUnit const* pUnit, int iPlayer, BOOL iAlwaysHostile = false);
bool PUF_isPotentialEnemy(CvUnit const* pUnit, int iPlayer, BOOL iAlwaysHostile = false);
bool PUF_isEnemy(CvUnit const* pUnit, int iPlayer, BOOL iAlwaysHostile = false);
bool PUF_canDeclareWar(CvUnit const* pUnit, int iPlayer, BOOL iAlwaysHostile = false);
// advc.ctr:
bool PUF_isEnemyCityAttacker(CvUnit const* pUnit, int iPlayer, int iAssumePeaceTeam = NO_TEAM);
bool PUF_isVisible(CvUnit const* pUnit, int iPlayer, int iDummy = -1);
bool PUF_isVisibleDebug(CvUnit const* pUnit, int iTargetPlayer, int iDummy = -1);
bool PUF_isLethal(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1); // advc.298
bool PUF_canSiege(CvUnit const* pUnit, int iTargetPlayer, int iDummy = -1);
bool PUF_canAirAttack(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1);
bool PUF_canAirDefend(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1);
bool PUF_isAirIntercept(CvUnit const* pUnit, int iDummy1, int iDummy2); // K-Mod
bool PUF_isFighting(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1);
bool PUF_isAnimal(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1);
bool PUF_isMilitaryHappiness(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1);
bool PUF_isInvestigate(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1);
bool PUF_isCounterSpy(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1);
bool PUF_isSpy(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1);
bool PUF_isDomainType(CvUnit const* pUnit, int iDomain, int iDummy = -1);
bool PUF_isUnitType(CvUnit const* pUnit, int iUnit, int iDummy = -1);
bool PUF_isUnitAIType(CvUnit const* pUnit, int iUnitAI, int iDummy = -1);
bool PUF_isMissionAIType(CvUnit const* pUnit, int iMissionAI, int iDummy = -1); // K-Mod
bool PUF_isCityAIType(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1);
bool PUF_isNotCityAIType(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1);
bool PUF_isSelected(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1);
//bool PUF_isNoMission(const CvUnit* pUnit, int iDummy1 = -1, int iDummy2 = -1);
// advc.113b:
bool PUF_isMissionPlotWorkingCity(CvUnit const* pUnit, int iCity, int iOwner);
bool PUF_isFiniteRange(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1);
// bbai start
bool PUF_isAvailableUnitAITypeGroupie(CvUnit const* pUnit, int iUnitAI, int iDummy);
bool PUF_isFiniteRangeAndNotJustProduced(CvUnit const* pUnit, int iDummy1 = -1, int iDummy2 = -1);
// bbai end

bool PUF_makeInfoBarDirty(CvUnit* pUnit, int iDummy1 = -1, int iDummy2 = -1);

// FAStarFunc... // advc.pf: Moved into new header FAStarFunc.h

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

#endif
