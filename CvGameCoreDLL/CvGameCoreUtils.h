#pragma once

#ifndef CIV4_GAMECORE_UTILS_H
#define CIV4_GAMECORE_UTILS_H

#include "AIStrategies.h" // <!-- custom: getSAS*VictoryStageLevel helpers use AIVictoryStage bitfields. (GPT-5.5) -->
#include <time.h> // <!-- custom: createSASUtcTimestamp accepts an already sampled time_t so a lifecycle identity and its filename can share one wall-clock reading. See KI#629. (GPT-5.6-Sol) -->

class CvPlot;
class CvCity;
class CvCityAI; // advc.003u
class CvUnit;
class CvUnitAI; // advc.003u
class CvSelectionGroup;
class CvDeal;
struct TradeData;
template <class tVARTYPE> class CLinkList;
class CvString;
class CvWString;
class CvRandom;
class FAStarNode;
class FAStar;

/*	advc:
 +	All functions dealing with arithmetics moved to ArithmeticUtils.h
	except getSign (now in CvPlot.cpp) and any functions involving randomness.
 +	Distance functions moved into CvMap.h.
 +	Shuffle functions moved to CvRandom.
 +	advc.opt: getCity, getUnit moved to CvPlayer.h. CvCity::fromIDInfo and
	CvUnit::fromIDInfo as alternatives in files that don't include CvPlayer.h.
 +	Unit cycling functions moved to CvSelectionGroup, CvUnit.
 +	Asset score functions moved to CvGame; no longer exposed to Python.
 +	isPromotionValid moved to CvUnitInfo, finalImprovementUpgrade to CvImprovementInfo,
	getEspionageModifier to CvTeam, getWorldSizeMaxConscript to CvGame (as getMaxConscript;
	no longer exposed to Python).
 +	advc.003w: Moved some two dozen functions to CvInfo classes;
	mostly functions dealing with building and unit class limitations.
	Removed isTechRequiredForProject.
 +	getCombatOdds, LFBgetCombatOdds moved to CombatOdds.
 +	advc.pf: FAStar functions moved into new header FAStarFunc.h
 What's left here are (non-arithmetic) things that people (such as myself)
 have been too lazy to find a proper place for. */

// advc:
namespace sequtil
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
};

/*	advc: Based K-Mod code in CvPlayer::getNextGroupInCycle; I find myself using
	this pattern from time to time:
	When a function optionally returns an additional value through a
	pointer argument that can be NULL, then define a local reference that
	refers to the same memory as the pointer arg - unless the pointer arg
	is NULL, in which case a local dummy variable is referenced instead. */
#define LOCAL_REF(T, localRefVarName, pointerArgName, tInitialVal) \
	T localRefVarName##_local = tInitialVal; /* dummy */ \
	T& localRefVarName = (pointerArgName == NULL ? localRefVarName##_local : *pointerArgName); \
	localRefVarName = tInitialVal; /* ensure initialization */

void contestedPlots(std::vector<CvPlot*>& r, TeamTypes t1, TeamTypes t2); // advc.035
// advc.130h:
template<typename T> void removeDuplicates(std::vector<T>& v)
{
	std::set<T> aeTmp(v.begin(), v.end());
	v.assign(aeTmp.begin(), aeTmp.end());
}

// advc.004w:
void applyColorToString(CvWString& s, char const* szColor, bool bLink = false);

// <!-- custom: Format one supplied time_t as second-precision UTC for stable diagnostic identities/filenames. See KI#629. (GPT-5.6-Sol) -->
CvString createSASUtcTimestamp(const time_t kTime);
// <!-- custom: Sample and format the current UTC time at second precision. See KI#629. (GPT-5.6-Sol) -->
CvString createSASUtcTimestamp();
// <!-- custom: Sample current UTC with milliseconds for event/snapshot timestamps that benefit from sub-second ordering. (ChatGPT-5.6-Sol) -->
CvString createSASUtcTimestampMilliseconds();
// <!-- custom: Sample the shared Win32 monotonic millisecond clock for diagnostic durations; unlike UTC, OS clock corrections do not affect it. (ChatGPT-5.6-Sol) -->
uint getSASMonotonicMilliseconds();
// <!-- custom: Compute elapsed diagnostic milliseconds with unsigned subtraction so the established single timeGetTime rollover remains safe. (ChatGPT-5.6-Sol) -->
uint getSASElapsedMilliseconds(uint uiStartMilliseconds, uint uiEndMilliseconds);
// <!-- custom: Return the one DLL-process UTC identity shared by BBAI and SASGameRecord. See KI#629. (GPT-5.6-Sol) -->
CvString const& getSASProcessUtcTimestamp();
// <!-- custom: Write one already-formatted narrow diagnostic line through Civ4's EXE logger; the EXE treats percent signs in the message as printf syntax again, so escape every literal percent only at this final boundary.
// BBAI and SASGameRecord use this after their own formatting pass; producer format strings still use normal printf escaping (%% for one literal percent). See KI#375.3. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASDiagnosticLiteralLine(char const* szLogName, char const* szLine);
// <!-- custom: Serialize the canonical active-mod display/folder/path fields shared by diagnostic log headers. (ChatGPT-5.6-Sol) -->
CvString getSASModContextFields();
// <!-- custom: Serialize runtime source/version provenance shared by BBAI and SASGameRecord; dirty is tri-state (-1 unavailable, 0 verified clean, 1 tracked changes). (ChatGPT-5.6-Sol) -->
CvString getSASSourceContextFields();
// <!-- custom: Compact player-facing source identity formatter shared by replay settings and future C++ UI surfaces; accepts either live ModName data or persisted save-history fields. (ChatGPT-5.6-Sol) -->
CvWString getSASCompactSourceVersion(char const* szVersion, char const* szCommitHash, int iDirtyState);
// <!-- custom: Combine the configured mod display name with the current cached runtime source identity for replay/UI surfaces. (ChatGPT-5.6-Sol) -->
CvWString getSASRuntimeDisplayNameAndVersion();
// <!-- custom: Serialize exact loaded-DLL build/binary provenance shared by diagnostic log headers. (ChatGPT-5.6-Sol) -->
CvString getSASDllContextFields();
// <!-- custom: Serialize exact Civ4 executable binary provenance independently from source/mod identity. (ChatGPT-5.6-Sol) -->
CvString getSASExeContextFields();
// <!-- custom: Serialize privacy-tiered installation provenance: low-detail distribution hints at level 2 and exact EXE/mod paths only at explicitly selected level 3. (ChatGPT-5.6-Sol) -->
CvString getSASInstallContextFields(int iSystemContextLevel);
// <!-- custom: Quote/escape narrow diagnostic free text; NULL becomes the unquoted missing token "-". (ChatGPT-5.6-Sol) -->
CvString getSASDiagnosticQuoted(char const* szValue);
// <!-- custom: Wide-string counterpart of getSASDiagnosticQuoted with identical escaping and missing-value semantics. (ChatGPT-5.6-Sol) -->
CvWString getSASDiagnosticQuoted(wchar const* szValue);
// <!-- custom: Serialize an empty diagnostic list/value as "-" while preserving nonempty text unchanged. (ChatGPT-5.6-Sol) -->
CvString getSASDiagnosticOrDash(CvString const& szValue);
// <!-- custom: Serialize one integer for shared diagnostic fields without recorder-specific wrappers. (ChatGPT-5.6-Sol) -->
CvString getSASDiagnosticIntText(int iValue);
// <!-- custom: Append one integer to a comma-separated diagnostic list without duplicating list plumbing across logs. (ChatGPT-5.6-Sol) -->
void appendSASDiagnosticIntListValue(CvString& szList, int iValue);
// <!-- custom: Construct the common timestamp/context/active-player diagnostic filename while each log retains its own rollover/session state. (ChatGPT-5.6-Sol) -->
CvString getSASDiagnosticLogName(char const* szBaseName, CvString const& szTimestamp, CvString const& szContext, bool bTimestamped);
// <!-- custom: Serialize authoritative finalized membership, identity, relations and trading capabilities for one initial team. (ChatGPT-5.6-Sol) -->
CvString getSASInitialTeamStateFields(TeamTypes eTeam);
// <!-- custom: Serialize exact finalized initial tech ownership/levels, handling ordinary and repeat technologies through their distinct Civ4 storage. (ChatGPT-5.6-Sol) -->
CvString getSASInitialTeamTechFields(TeamTypes eTeam);
// <!-- custom: Return the same exact technology payload without a team identifier so diagnostic callers can group teams with identical finalized technology sets. (GPT-5.6-Sol) -->
CvString getSASInitialTeamTechLevelFields(TeamTypes eTeam);
// <!-- custom: Serialize a team ID as TEAM_n, or "-" for NO_TEAM, for shared diagnostic payloads. (ChatGPT-5.6-Sol) -->
CvString getSASTeamDiagnosticText(TeamTypes eTeam);
// <!-- custom: Serialize the data payload of one diplomacy trade item using stable type/team/city identifiers where available. (ChatGPT-5.6-Sol) -->
CvString getSASTradeDataText(TradeData const& kItem, PlayerTypes eFromPlayer);
// <!-- custom: Serialize a complete diplomacy trade list as comma-separated TYPE:data entries, or "-" when empty. (ChatGPT-5.6-Sol) -->
CvString getSASTradeListText(CLinkList<TradeData> const& kList, PlayerTypes eFromPlayer);
// <!-- custom: Serialize one surviving finalized initial deal, including players/teams, age/cancel timing and both exact trade lists. (ChatGPT-5.6-Sol) -->
CvString getSASInitialDealStateFields(CvDeal const& kDeal);
// <!-- custom: Recognize only the simple reciprocal peace-deal shape that is safe to collapse in ordinary non-scenario Advanced Start. (ChatGPT-5.6-Sol) -->
bool isSASCollapsibleAdvancedStartPeaceDeal(CvDeal const& kDeal);
// <!-- custom: Summarize finalized starting deals, including collapsed Advanced Start peace counts/cancel windows and exact detail-row count. (ChatGPT-5.6-Sol) -->
CvString getSASInitialDealSummaryFields(bool bDealDetailEnabled, int iLoggedDealRows);

float colorDifference(NiColorA const& c1, NiColorA const& c2); // advc.002i
HandicapTypes handicapFromDifficulty(int iDifficulty); // <!-- custom: map iDifficulty scores back to XML handicap entries after adding non-BtS handicap levels. (ChatGPT-5.5) -->

// <advc> Replacing (unused) tables in CvGlobals for single-step rotation
inline DirectionTypes rotateDirClockw(DirectionTypes eDir, int i45DegRotations = 1) // Mustn't be less than -NUM_DIRECTION_TYPES
{
	/*	Could also try
		return static_cast<DirectionTypes>((eDir + i45DegRotations) & (NUM_DIRECTION_TYPES - 1));
		... but I guess the optimizer will handle it. */
	return static_cast<DirectionTypes>((eDir + i45DegRotations
			+ NUM_DIRECTION_TYPES) // To avoid negative remainder
			% NUM_DIRECTION_TYPES);
}

inline DirectionTypes rotateDirCounterClockw(DirectionTypes eDir, int i45DegRotations = 1) { return rotateDirClockw(eDir, -i45DegRotations); } // </advc>
inline CardinalDirectionTypes getOppositeCardinalDirection(CardinalDirectionTypes eDir) { return (CardinalDirectionTypes)((eDir + 2) % NUM_CARDINALDIRECTION_TYPES); } // Exposed to Python
DirectionTypes cardinalDirectionToDirection(CardinalDirectionTypes eCard);					// Exposed to Python
DllExport inline bool isCardinalDirection(DirectionTypes eDirection)						// Exposed to Python
{
	switch (eDirection)
	{
	case DIRECTION_EAST:
	case DIRECTION_NORTH:
	case DIRECTION_SOUTH:
	case DIRECTION_WEST:
		return true;
	}
	return false;
}
DirectionTypes estimateDirection(int iDX, int iDY);											// Exposed to Python
DllExport DirectionTypes estimateDirection(const CvPlot* pFromPlot, const CvPlot* pToPlot);

// advc: Moved from CvXMLLoadUtility. (CvHotKeyInfo might be an even better place?)
namespace hotkeyDescr
{
	CvWString keyStringFromKBCode(TCHAR const* szDescr);
	CvWString hotKeyFromDescription(TCHAR const* szDescr, bool bShift = false, bool bAlt = false, bool bCtrl = false);
}

bool atWar(TeamTypes eTeamA, TeamTypes eTeamB);												// Exposed to Python
//isPotentialEnemy(TeamTypes eOurTeam, TeamTypes eTheirTeam); // advc: Use CvTeamAI::AI_mayAttack instead
char const* getSASDiploEventType(DiploEventTypes eDiploEvent); // <!-- custom: Shared raw enum-token text for DiploEventTypes because static enum values have no CvInfo type strings; use user-facing text helpers for translated/prose labels. (GPT-5.5) -->
char const* getSASContactType(ContactTypes eContact); // <!-- custom: Shared raw enum-token text for ContactTypes so diplomacy diagnostics can describe proactive contact classes without opaque enum integers. (ChatGPT-5.6-Sol) -->
char const* getSASTradeItemType(TradeableItems eItem); // <!-- custom: Shared raw enum-token text for TradeableItems because static enum values have no CvInfo type strings; use user-facing text helpers for translated/prose labels. (GPT-5.5) -->
char const* getSASWarPlanType(WarPlanTypes eWarPlan); // <!-- custom: Shared raw enum-token text for WarPlanTypes because static enum values have no CvInfo type strings; use user-facing text helpers for translated/prose labels. (GPT-5.5) -->
char const* getSASGameType(GameType eType); // <!-- custom: Shared raw enum-token text for game/session diagnostics because GameType has no CvInfo type strings. (GPT-5.6-Sol) -->
char const* getSASGameMode(GameMode eMode); // <!-- custom: Shared raw enum-token text for game/session diagnostics because GameMode has no CvInfo type strings. (GPT-5.6-Sol) -->
char const* getSASCalendarType(CalendarTypes eCalendar); // <!-- custom: Shared raw enum-token text for calendar context because CalendarTypes has no CvInfo type strings. (GPT-5.6-Sol) -->
char const* getSASWarDeclarationCause(WarDeclarationCause eCause); // <!-- custom: Shared stable labels for war-declaration origins so SASGameRecord and any later BBAI diagnostics do not duplicate the switch. (GPT-5.6-Sol) -->
char const* getSASTechAcquisitionCause(TechAcquisitionCause eCause); // <!-- custom: Shared stable labels for technology-acquisition origins so callers pass semantic context instead of GameRecord guessing from ambiguous flags. (GPT-5.6-Sol + GPT-5.6 Thinking) -->
char const* getSASResearchTargetChangeCause(ResearchTargetChangeCause eCause); // <!-- custom: Shared stable labels for factual research-target redirection causes so SASGameRecord and later diagnostics can reuse one vocabulary. (ChatGPT-5.6-Sol) -->
char const* getSASAutoPlayEndCause(SASAutoPlayEndCause eCause); // <!-- custom: Shared stable labels for explicit AI Auto Play completion causes so later diagnostics can reuse the enum without depending on SASGameRecord. See KI#203. (GPT-5.6-Sol) -->
char const* getSASMemoryType(MemoryTypes eMemory); // <!-- custom: Shared raw enum-token text for diplomatic memories because static enum values have no CvInfo type strings. (GPT-5.6-Sol) -->
char const* getSASAIStrategyType(AIStrategy eStrategy); // <!-- custom: Shared raw enum-token text for AIStrategy because the static bitfield enum has no CvInfo type strings; BBAI and SASGameRecord use one diagnostic vocabulary without preprocessor stringification. (ChatGPT-5.6-Sol) -->
char const* getSASAreaAIType(AreaAITypes eAreaAI); // <!-- custom: Shared raw enum-token text for AreaAITypes so SASGameRecord and detailed AI diagnostics can describe theater posture without opaque enum integers. (ChatGPT-5.6-Sol) -->
int getSASVictoryStageLevel(AIVictoryStage eVictoryStageHash, AIVictoryStage eStage1, AIVictoryStage eStage2, AIVictoryStage eStage3, AIVictoryStage eStage4); // <!-- custom: Shared victory-stage bitfield helper for compact AI victory diagnostics without repeating AI_atVictoryStage checks. (GPT-5.5) -->
int getSASCultureVictoryStageLevel(AIVictoryStage eVictoryStageHash); // <!-- custom: Named wrappers avoid repeating four enum constants at every logging/evaluation call site. (GPT-5.5) -->
int getSASSpaceVictoryStageLevel(AIVictoryStage eVictoryStageHash);
int getSASConquestVictoryStageLevel(AIVictoryStage eVictoryStageHash);
int getSASDominationVictoryStageLevel(AIVictoryStage eVictoryStageHash);
int getSASDiplomacyVictoryStageLevel(AIVictoryStage eVictoryStageHash);
int getSASTeamMaxVictoryStage(TeamTypes eTeam); // <!-- custom: Shared team-level wrapper for victory-denial logs and rules that need the highest current victory stage among team members. (GPT-5.5) -->
int getSASTeamSpaceVictoryStage(TeamTypes eTeam); // <!-- custom: Shared team-level Space victory-stage helper for victory-denial rules that need Space-specific progress instead of the highest route-agnostic stage. (GPT-5.5) -->
int getSASTeamSpaceshipPartsBuilt(TeamTypes eTeam); // <!-- custom: Shared spaceship-parts count for Space victory-denial rules and logs; using one helper keeps UWAI and peace-refusal thresholds consistent. (GPT-5.5) -->
int getSASSpaceshipPartsRequired();
int getSASTeamSpaceshipPartsPercent(TeamTypes eTeam);
int getSASVictoryDelayTurnsFromNormalGameSpeed(int iNormalTurns); // <!-- custom: Scale Normal-speed victory-countdown turn gates with the same VictoryDelayPercent used by CvGame::victoryDelay. (GPT-5.6 Thinking) -->
int getSASTeamStage3SpaceLeaderPartGap(TeamTypes eTeam);
bool isSASTeamStage3SpaceVictoryThreat(TeamTypes eTeam); // <!-- custom: Stage-3 plus high spaceship completion near the Space-progress leader is a pre-countdown Space threat; save-file 450 Arabia reached the final countdown too late, and save-file 452 showed a raw 10-parts threshold could still fire too late. (GPT-5.5) -->
bool isSASUWAIVictoryDenialPeaceThreat(TeamTypes eTeam, int* piVictoryCountdown = NULL, int* piMaxVictoryStage = NULL); // <!-- custom: Shared by UWAI peace decisions and CvDeal's final guard so blocked victory-denial treaties are rejected before negotiation instead of returning false success. (GPT-5.6-Sol) -->

int estimateCollateralWeight(const CvPlot* pPlot, TeamTypes eAttackTeam, TeamTypes eDefenseTeam = NO_TEAM); // K-Mod

/*	advc (note): Still used in the DLL by CvPlayer::buildTradeTable, but mostly deprecated.
	Use the TradeData constructor instead. */
DllExport void setTradeItem(TradeData* pItem, TradeableItems eItemType = TRADE_ITEM_NONE, int iData = 0);

/*	advc: Unused. Thought about moving these to CvGameTextMgr,
	but that'll lead to more header inclusions. */
//void setListHelp(wchar* szBuffer, const wchar* szStart, const wchar* szItem, const wchar* szSeparator, bool bFirst);
void setListHelp(CvWString& szBuffer, wchar const* szStart, wchar const* szItem, wchar const* szSeparator, bool& bFirst); // advc: bool&
void setListHelp(CvWStringBuffer& szBuffer, wchar const* szStart, wchar const* szItem, wchar const* szSeparator, bool& bFirst); // advc: bool&
/*	<advc> Add variants for items that can go into one list only when a value
	matches the most recently added item. (This stuff should really be wrapped
	into a class.) */
void setListHelp(CvWString& szBuffer, wchar const* szStart, wchar const* szItem, wchar const* szSeparator, int& iLastListID, int iListID);
void setListHelp(CvWStringBuffer& szBuffer, wchar const* szStart, wchar const* szItem, wchar const* szSeparator, int& iLastListID, int iListID); // </advc>

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

int baseYieldToSymbol(int iNumYieldTypes, int iYieldStack);
//bool isPickableName(const TCHAR* szName); // advc.003j

/*  advc: Hash based on kInputs. Plot index of capital factored in for
	increased range if ePlayer given. (ePlayer is ignored if it has no capital.) */
int intHash(std::vector<int> const& kInputs, PlayerTypes ePlayer = NO_PLAYER);

// <advc.003g>
namespace fmath
{
	/*	See intHash about the parameters.
		Result between 0 and 1. Returns float b/c CvRandom uses float (not double).
		(Similar but more narrow: CvUnitAI::AI_unitBirthmarkHash, AI_unitPlotHash) */
	inline float hash(std::vector<int> const& kInputs, PlayerTypes ePlayer = NO_PLAYER)
	{
		/*  Use ASyncRand to avoid the overhead of creating a new object?
			Or use stdlib's rand/srand? I don't think it matters. */
		/*CvRandom& rng = GC.getASyncRand();
		rng.reset(hashVal);*/
		CvRandom rng;
		rng.init(intHash(kInputs, ePlayer));
		return rng.getFloat();
	}
	// For hashing just a single input
	inline float hash(int iInputs, PlayerTypes ePlayer = NO_PLAYER)
	{
		std::vector<int> inputs;
		inputs.push_back(iInputs);
		return hash(inputs, ePlayer);
	}
} // </advc.003g>

int getTurnMonthForGame(int iGameTurn, int iStartYear, CalendarTypes eCalendar, GameSpeedTypes eSpeed);
int getTurnYearForGame(int iGameTurn, int iStartYear, CalendarTypes eCalendar, GameSpeedTypes eSpeed);

void getDirectionTypeString(CvWString& szString, DirectionTypes eDirectionType);
void getCardinalDirectionTypeString(CvWString& szString, CardinalDirectionTypes eDirectionType);
void getActivityTypeString(CvWString& szString, ActivityTypes eActivityType);
void getMissionTypeString(CvWString& szString, MissionTypes eMissionType);
void getMissionAIString(CvWString& szString, MissionAITypes eMissionAI);
void getUnitAIString(CvWString& szString, UnitAITypes eUnitAI);

#endif
