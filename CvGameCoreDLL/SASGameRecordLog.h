// AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
// (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

#pragma once

#ifndef SAS_GAME_RECORD_LOG_H
#define SAS_GAME_RECORD_LOG_H

#include "AIStrategies.h" // <!-- custom: Exact AI strategy/victory-stage transition bridges use the native bitfield types while keeping the recorder interface type-safe. (ChatGPT-5.6-Sol) -->
#include <vector> // <!-- custom: Recorder-owned technology-choice candidates and rare goody-result unit lists use std::vector directly; include its defining header rather than relying on transitive core includes. (ChatGPT-5.6-Sol) -->
#include <utility> // <!-- custom: Public religion-decision provenance uses std::pair for already-computed candidate religion/value scores; include its defining header directly instead of relying on transitive core includes. (ChatGPT-5.6-Sol) -->
#include <string> // <!-- custom: The SASGameRecord session-settings bridge exposes only the active ini= argument as std::string; include the defining header directly instead of relying on transitive core includes. (ChatGPT-5.6-Sol) -->

// <!-- custom: Revision 151 added a commerce-logging CvPlayer reference above the previous forward-declaration block; CitySiteEvaluator.cpp then failed with C2065/C2143 because CvPlayer was not declared yet.
// Declare it before the first signature rather than relying on caller include order or adding the full player header. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
class CvPlayer;

// <!-- custom: Structured game-record rows for autoplay comparison, game analysis, user-assistance summaries, and external LLM review.
// This is not a classic BBAI diagnostic category: it has its own XML defines, its own SASGameRecord_*.log files, and its own lightweight public header.
// Call sites should still gate before invoking helpers so disabled logging does not compute logging-only arguments.
// Pointer-only hooks use forward declarations here to avoid pulling city/unit headers into ordinary game files. (ChatGPT-5.5 + GPT-5.5) -->
// <!-- custom: SAS_GAME_RECORD_LOG_LEVEL is copied once after all GlobalDefines/module overrides finish loading.
// Keep the steady-state hot gate as a direct integer read: SASGameRecord hooks are intentionally widespread, and an out-of-line getter would impose a cross-translation-unit function call even at level 0 in ordinary non-LTCG Release builds.
// Before XML setup completes the zero-initialized cache safely means disabled.
// Callers must treat the cache as read-only and continue using gGameRecordLogLevel/getSASGameRecordLogLevel; only cacheSASGameRecordLogLevel writes it during GlobalDefines setup. (ChatGPT-5.6-Sol) -->
extern int g_iSASGameRecordLogLevel;
void cacheSASGameRecordLogLevel();
// <!-- custom: Revision 151 keeps the causal action/interface subject independently suppressible without making every hot caller query XML.
// Cache it beside the log level after all GlobalDefines/module overrides load; level-0 callers still short-circuit before consulting this switch. (ChatGPT-5.6-Sol) -->
extern bool g_bSASGameRecordActionTraceEnabled;
void cacheSASGameRecordActionTraceEnabled();
__forceinline bool isSASGameRecordLogEnabled() { return g_iSASGameRecordLogLevel > 0; }
__forceinline int getSASGameRecordLogLevel() { return g_iSASGameRecordLogLevel; }
__forceinline bool isSASGameRecordActionTraceEnabled() { return g_bSASGameRecordActionTraceEnabled; }
int getSASGameRecordTurnInterval();
// <!-- custom: Finalize buffered observations in the old game state before a new game or loaded save resets/replaces it.
// Only an opened recorder session emits final writer health; startup/repeated resets still retry and release shared output without creating a record. See KI#382. See KI#382.3. (ChatGPT-5.6-Sol + GPT-5.6-Sol + GPT-6.1-Sol) -->
void finalizeSASGameRecordLogSession();
void startSASGameRecordLogForNewGame();
void logSASGameRecordNewGameStarted();
// <!-- custom: Python calls this once after GameStart/OnLoad dispatch completes so BUG/Autolog state reflects the current session, not its pre-handler value; callers must pre-gate logging. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
// <!-- custom: Python supplies graphics-profile dropdown values that are not exposed by the native DLL API; this remains a one-shot post-handler session diagnostic, not per-turn polling. (ChatGPT-5.6-Sol) -->
void logSASGameRecordDiagnosticContext(int iRenderQualityLevel, int iAntiAliasingIndex, int iAntiAliasingSamples, int iGlobeViewRenderLevel, int iMovieQualityLevel);
// <!-- custom: Python-owned configuration sources use one compact row family for the post-handler baseline and rare change callbacks.
// Values are percent-encoded before crossing the bridge, so the native writer never parses or guesses arbitrary configuration text. (ChatGPT-5.6-Sol) -->
void logSASGameRecordSessionSettings(char const* szEvent, char const* szReason, char const* szSource, char const* szGroup, int iCount, char const* szValues);
// <!-- custom: Runtime map-view state has authoritative native getters and exact application points, unlike configured profile preferences.
// Keep one compact baseline plus change-only rows so support screenshots/view differences are reconstructible without UI polling. (ChatGPT-5.6-Sol) -->
void logSASGameRecordRuntimeViewStateSnapshot(char const* szReason);
void logSASGameRecordRuntimeViewBoolChange(char const* szReason, char const* szKey, bool bOldValue, bool bNewValue);
void logSASGameRecordRuntimeViewLayerChange(char const* szReason, GlobeLayerTypes eOldLayer, GlobeLayerTypes eNewLayer);
// <!-- custom: Effective player options are applied asynchronously after the post-GameStart Python baseline.
// Log the native CvPlayer application point so startup synchronization and real user changes are not misattributed to an unrelated options-screen exit. (ChatGPT-5.6-Sol) -->
void logSASGameRecordPlayerOptionChange(PlayerTypes ePlayer, PlayerOptionTypes eOption, bool bOldValue, bool bNewValue);
// <!-- custom: Revision 151 adds a compact causal interface/action layer distinct from state snapshots: native controls report requests, Python screens report open/navigation events, and scoreboard local state reports only real changes.
// Screen/interface strings are fixed machine-readable tokens assembled by SAS-owned call sites; never pass user-entered/free-form text through these bridges.
// Callers pre-gate at level 2+ and, from revision 151 onward, the independent action-trace subject switch. (ChatGPT-5.6-Sol) -->
// <!-- custom: Accept the native control enum instead of separate action/source strings so accepted and rejected paths share one curated mapping. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
void logSASGameRecordControlRequest(ControlTypes eControl, bool bAccepted);
void logSASGameRecordScreenAction(char const* szScreen, char const* szAction, int iPrimary = -1, int iSecondary = -1);
void logSASGameRecordInterfaceState(char const* szComponent, char const* szEvent, char const* szReason, int iCount, char const* szValues);
// <!-- custom: Four camera toggles have authoritative pre/post Boolean getters.
// Keep their resulting state in the same compact interface family as scoreboard/annotation state, while rotation/flying-mode-cycle controls remain request-only because no equivalent Boolean result exists.
// Replace the draft string triple with ControlTypes; one recorder-owned label list serves both request and camera-result serialization. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
void logSASGameRecordCameraBoolChange(ControlTypes eControl, bool bOldValue, bool bNewValue);
// <!-- custom: Commerce setters can rebalance several percentages in one application.
// Record the authoritative active-human result as one compact row instead of inferring slider intent from button presses. (ChatGPT-5.6-Sol) -->
void logSASGameRecordCommercePercentChange(CvPlayer const& kPlayer, CommerceTypes eRequestedCommerce, bool bForced, int const* aiOldPercent);
// <!-- custom: The EXE can select an alternate CivilizationIV.ini through an ini= command-line argument. Expose only that raw argument internally to Python so the recorder can read the configured startup file; the path itself is never written to SASGameRecord. (ChatGPT-5.6-Sol) -->
std::string getSASGameRecordIniFileArgument();
void startSASGameRecordLogForLoadedSave();
// <!-- custom: Record the exact checkpoint metadata embedded in a real CvGame serialization; checkpoint persistence is unconditional save provenance, while the caller pre-gates only this diagnostic row at level 1+. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
void logSASGameRecordSaveCheckpointSerialized(char const* szKind, int iTurn, int iElapsedTurn, char const* szUtc, uint uiMapRandState, uint uiSyncRandState, char const* szReason, char const* szOriginalBasename);

class CvRandom;
// <!-- custom: Level-3 RNG divergence tracking observes every advance of the authoritative game map/synchronized RNGs, including calls with NULL RandLog messages.
// Async randomness is intentionally excluded because it is deliberately non-lockstep/client-sensitive; local/temporary CvRandom helpers are excluded because they do not advance CvGame's authoritative map/synchronized streams.
// Keep the hot-path gate as one shared extern boolean so disabled/lower-detail runs do not call a recorder helper for every random number.
// Tracking state is recorder-local only and never serialized into CvRandom or savegames. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
extern bool g_bSASGameRecordRngTrackingActive;
// <!-- custom: Keep this closed recorder-only checkpoint vocabulary beside its API; unlike gameplay-wide cause metadata in CvEnums.h, these values exist solely to make distant SASGameRecord lifecycle hooks typo-safe and do not belong in CvGameCoreUtils. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
// <!-- custom: Recorder-owned vocabulary lists now generate both enum entries and their existing schema labels instead of maintaining each name again in a separate switch.
// NAME is the full C++ identifier; LABEL is the exact shorter log token stringified by the serializer, including deliberate exceptions such as SAS_EARLY_REMOTE_BARB.
// Keep row order stable to preserve numeric values; NONE sentinels, fallback handling and semantic mappings remain explicit.
// These macros generate declarations/cases only, with no runtime registry or string construction. (GPT-6.1-Sol) -->
#define SAS_RECORD_RNG_CHECKPOINT_REASON_ROWS(DO) \
	DO(SAS_RNG_CHECKPOINT_NEW_GAME_INITIALIZED, NEW_GAME_INITIALIZED) \
	DO(SAS_RNG_CHECKPOINT_MAP_REGENERATION_BEGIN, MAP_REGENERATION_BEGIN) \
	DO(SAS_RNG_CHECKPOINT_MAP_REGENERATION_END, MAP_REGENERATION_END) \
	DO(SAS_RNG_CHECKPOINT_AUTOPLAY_BEGIN, AUTOPLAY_BEGIN) \
	DO(SAS_RNG_CHECKPOINT_AUTOPLAY_END, AUTOPLAY_END) \
	DO(SAS_RNG_CHECKPOINT_END_GAME_TURN, END_GAME_TURN) \
	DO(SAS_RNG_CHECKPOINT_VICTORY, VICTORY) \
	DO(SAS_RNG_CHECKPOINT_GAME_END, GAME_END) \
	DO(SAS_RNG_CHECKPOINT_SAVE_LOADED, SAVE_LOADED) \
	DO(SAS_RNG_CHECKPOINT_SESSION_FINALIZE, SESSION_FINALIZE)
enum SASGameRecordRngCheckpointReason
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_RNG_CHECKPOINT_REASON_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
void initializeSASGameRecordRngTracking();
void noteSASGameRecordRandomCall(CvRandom const* pRandom, unsigned short usRange, TCHAR const* szLog, int iData1, int iData2);
void noteSASGameRecordExternalRandomCall(CvRandom const* pRandom);
void noteSASGameRecordRandomSeedSet(CvRandom const* pRandom, unsigned int uiOldState, unsigned int uiNewState, bool bReseed);
void logSASGameRecordRngCheckpoint(int iGameTurn, SASGameRecordRngCheckpointReason eReason);
// <!-- custom: Emit one tiny authoritative completion marker after all current-turn gameplay/autoplay/sync cleanup and before the game clock advances.
// This remains available at levels 1/2 where the level-3 END_GAME_TURN RNG/state checkpoint is absent.
// Caller pre-gates at level 1+. (ChatGPT-5.6-Sol) -->
void logSASGameRecordTurnCompleted(int iGameTurn);

class CvCity;
class CvCityAI; // <!-- custom: Realized AI building-choice hooks accept a CvCityAI reference; forward-declare it so this lightweight recorder header does not pull CvCityAI.h into every caller. (GPT-5.6-Sol) -->
// <!-- custom: Required by random-event APIs using CvPlayer references; this lightweight declaration fixed the resulting AgentIterator/CvPlayer compile errors. (GPT-5.6-Sol) -->
class CvPlayerAI;
class CvTeamAI;
class CvArea;
// <!-- custom: Record final authoritative AIStrategy transitions from AI_updateStrategyHash without exposing its protected raw strategy hash or rerunning any strategy evaluation; caller must pre-gate at GameRecord level 2+. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIStrategyChanges(CvPlayerAI const& kPlayer, AIStrategy eOldStrategies, AIStrategy eNewStrategies);
// <!-- custom: Record final authoritative 0..4 victory-route stage transitions from AI_updateVictoryStageHash using its already-computed before/after bitfields; caller must pre-gate at GameRecord level 2+. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIVictoryStageChanges(CvPlayerAI const& kPlayer, AIVictoryStage eOldStages, AIVictoryStage eNewStages);
// <!-- custom: Record only the final committed team worst-enemy replacement; caller passes the real selection pass's already-computed old/new enmity values, whether it is AdvCiv's recursive corrective pass, and pre-gates at GameRecord level 2+. (ChatGPT-5.6-Sol) -->
void logSASGameRecordWorstEnemyChanged(CvTeamAI const& kTeam, TeamTypes eOldEnemy, TeamTypes eNewEnemy, int iOldEnmity, int iNewEnmity, bool bRecursiveRecheck);
// <!-- custom: Record AreaAI changes at the two authoritative writers only: ordinary team calculation and UWAI's later alignment override.
// Callers pre-gate at GameRecord level 2+. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAreaAIChanged(CvTeamAI const& kTeam, CvArea const& kArea, AreaAITypes eOldType, AreaAITypes eNewType, char const* szSource);
// <!-- custom: AI target-city state is player+area scoped and has several authoritative/effective change paths.
// Keep the closed recorder-only source vocabulary typed so distant callers cannot silently drift in spelling; callers pre-gate at GameRecord level 2+. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_TARGET_CITY_CHANGE_SOURCE_ROWS(DO) \
	DO(SAS_AI_TARGET_CITY_AREA_SEARCH, AREA_SEARCH) \
	DO(SAS_AI_TARGET_CITY_RANDOM_CLEAR, RANDOM_CLEAR) \
	DO(SAS_AI_TARGET_CITY_DIPLO_COORDINATION, DIPLO_COORDINATION) \
	DO(SAS_AI_TARGET_CITY_AREA_REASSIGN_CLEAR, AREA_REASSIGN_CLEAR) \
	DO(SAS_AI_TARGET_CITY_CITY_REMOVED, CITY_REMOVED)
enum SASGameRecordAITargetCityChangeSource
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_TARGET_CITY_CHANGE_SOURCE_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
void logSASGameRecordAITargetCityChanged(CvPlayerAI const& kPlayer, CvArea const& kArea, CvCity const* pOldCity, CvCity const* pNewCity, SASGameRecordAITargetCityChangeSource eSource, int iSelectionValue = -1);
// <!-- custom: Closed recorder-only outcomes/reasons for AI captured-city disposition.
// These describe realized KEEP/RAZE/LIBERATE decisions rather than gameplay-wide enums, so they remain with SASGameRecord. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_CONQUER_CITY_OUTCOME_ROWS(DO) \
	DO(SAS_AI_CONQUER_CITY_KEEP, KEEP) \
	DO(SAS_AI_CONQUER_CITY_RAZE, RAZE) \
	DO(SAS_AI_CONQUER_CITY_LIBERATE, LIBERATE)
enum SASGameRecordAIConquerCityOutcome
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_CONQUER_CITY_OUTCOME_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
#define SAS_RECORD_AI_CONQUER_CITY_REASON_ROWS(DO) \
	DO(SAS_AI_CONQUER_CITY_CANNOT_RAZE, CANNOT_RAZE) \
	DO(SAS_AI_CONQUER_CITY_DOMINATION3_PRIMARY_AREA_KEEP, DOMINATION3_PRIMARY_AREA_KEEP) \
	DO(SAS_AI_CONQUER_CITY_CULTURE_VICTORY, CULTURE_VICTORY) \
	DO(SAS_AI_CONQUER_CITY_UNLIKELY_LONG_TERM_BENEFIT, SAS_UNLIKELY_LONG_TERM_BENEFIT) \
	DO(SAS_AI_CONQUER_CITY_EARLY_REMOTE_BARB, SAS_EARLY_REMOTE_BARB) \
	DO(SAS_AI_CONQUER_CITY_EARLY_REMOTE_NONBARB, SAS_EARLY_REMOTE_NONBARB) \
	DO(SAS_AI_CONQUER_CITY_BARBARIAN_VALUE, BARBARIAN_VALUE) \
	DO(SAS_AI_CONQUER_CITY_NORMAL_VALUE, NORMAL_VALUE) \
	DO(SAS_AI_CONQUER_CITY_LIBERATION, LIBERATION) \
	DO(SAS_AI_CONQUER_CITY_LIBERATION_WITHHELD_HOSTAGE, LIBERATION_WITHHELD_HOSTAGE)
enum SASGameRecordAIConquerCityReason
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_CONQUER_CITY_REASON_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
// <!-- custom: Record one realized AI disposition for every AI_conquerCity path.
// Value/component fields are valid only when gameplay reached the corresponding raze valuation; callers pass already-computed values and pre-gate at GameRecord level 2+. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIConquerCityDecision(CvPlayerAI const& kPlayer, CvCity const& kCity, SASGameRecordAIConquerCityOutcome eOutcome, SASGameRecordAIConquerCityReason eReason, bool bEverOwned, int iCloseness = -1, bool bValueValid = false, int iRazeValueBeforeRandom = 0, int iRazeRandom = -1, int iRazeValue = 0, bool bComponentsValid = false, int iDistanceAndLocalPower = 0, int iMaintenanceDelta = 0, int iPopulationDelta = 0, int iPersonalityDominationDelta = 0, int iOtherDelta = 0, int iFinancialTrouble = -1, int iBarbarianRollPassed = -1, PlayerTypes eLiberationPlayer = NO_PLAYER);
// <!-- custom: Closed recorder-only reasons for foreground-UWAI war-plan lifecycle mutations.
// These describe why a real plan changed; they are not gameplay enums and therefore remain with SASGameRecord rather than CvGameCoreUtils. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_UWAI_WAR_PLAN_DECISION_REASON_ROWS(DO) \
	DO(SAS_UWAI_WAR_PLAN_ILLEGAL_TARGET, ILLEGAL_TARGET) \
	DO(SAS_UWAI_WAR_PLAN_VICTORY_DENIAL_DIRECT, VICTORY_DENIAL_DIRECT) \
	DO(SAS_UWAI_WAR_PLAN_IMMINENT_NEGATIVE_UTILITY, IMMINENT_NEGATIVE_UTILITY) \
	DO(SAS_UWAI_WAR_PLAN_IMMINENT_TIMEOUT, IMMINENT_TIMEOUT) \
	DO(SAS_UWAI_WAR_PLAN_PREPARATION_DEADLINE_REACHED, PREPARATION_DEADLINE_REACHED) \
	DO(SAS_UWAI_WAR_PLAN_PREPARATION_DEADLINE_NEGATIVE_UTILITY, PREPARATION_DEADLINE_NEGATIVE_UTILITY) \
	DO(SAS_UWAI_WAR_PLAN_SEVERE_NEGATIVE_UTILITY, SEVERE_NEGATIVE_UTILITY) \
	DO(SAS_UWAI_WAR_PLAN_TARGET_SWITCH, TARGET_SWITCH) \
	DO(SAS_UWAI_WAR_PLAN_ATTACKED_RECENT_MATURED, ATTACKED_RECENT_MATURED) \
	DO(SAS_UWAI_WAR_PLAN_ACTIVE_TYPE_SWITCH, ACTIVE_TYPE_SWITCH) \
	DO(SAS_UWAI_WAR_PLAN_DIRECT_UTILITY_THRESHOLD, DIRECT_UTILITY_THRESHOLD)
enum SASGameRecordUWAIWarPlanDecisionReason
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_UWAI_WAR_PLAN_DECISION_REASON_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
// <!-- custom: Record only realized foreground-UWAI lifecycle decisions immediately before their authoritative plan/declaration mutation.
// Comparison/decision fields reuse values already computed by UWAI; callers pre-gate level 2+ and never rerun war evaluation or RNG for logging. (ChatGPT-5.6-Sol) -->
void logSASGameRecordUWAIWarPlanDecision(TeamTypes eAgent, TeamTypes eTarget, SASGameRecordUWAIWarPlanDecisionReason eReason, WarPlanTypes eOldWarPlan, WarPlanTypes eNewWarPlan, int iUtility, int iStateCounter, int iPrepTurnsRemaining, TeamTypes eComparisonTarget = NO_TEAM, int iComparisonUtility = -1, int iDecisionValue = -1, int iDecisionThreshold = -1, int iVictoryDenialBoost = 0);
class CvPlot;
class CvUnit;
class CvUnitAI;
// <!-- custom: Closed recorder-only outcomes for ordinary AI Great Person action selection.
// These describe AI_greatPersonMove's realized action/continuation/fallback rather than a gameplay-wide enum, so keep them with SASGameRecord.
// Great Generals use the separate AI_generalMove path and are intentionally outside this schema. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_GREAT_PERSON_ACTION_ROWS(DO) \
	DO(SAS_AI_GREAT_PERSON_DISCOVER_TECH, DISCOVER_TECH) \
	DO(SAS_AI_GREAT_PERSON_TRADE_MISSION, TRADE_MISSION) \
	DO(SAS_AI_GREAT_PERSON_MOVE_TO_TRADE_MISSION, MOVE_TO_TRADE_MISSION) \
	DO(SAS_AI_GREAT_PERSON_GREAT_WORK, GREAT_WORK) \
	DO(SAS_AI_GREAT_PERSON_MOVE_TO_GREAT_WORK, MOVE_TO_GREAT_WORK) \
	DO(SAS_AI_GREAT_PERSON_GOLDEN_AGE, GOLDEN_AGE) \
	DO(SAS_AI_GREAT_PERSON_JOIN_CITY, JOIN_CITY) \
	DO(SAS_AI_GREAT_PERSON_MOVE_TO_JOIN_CITY, MOVE_TO_JOIN_CITY) \
	DO(SAS_AI_GREAT_PERSON_CONSTRUCT_BUILDING, CONSTRUCT_BUILDING) \
	DO(SAS_AI_GREAT_PERSON_MOVE_TO_CONSTRUCT_BUILDING, MOVE_TO_CONSTRUCT_BUILDING) \
	DO(SAS_AI_GREAT_PERSON_HURRY_BUILDING, HURRY_BUILDING) \
	DO(SAS_AI_GREAT_PERSON_MOVE_TO_HURRY_BUILDING, MOVE_TO_HURRY_BUILDING) \
	DO(SAS_AI_GREAT_PERSON_DANGER_DISCOVER_TECH, DANGER_DISCOVER_TECH) \
	DO(SAS_AI_GREAT_PERSON_RECON_SPY, RECON_SPY) \
	DO(SAS_AI_GREAT_PERSON_RETREAT, RETREAT) \
	DO(SAS_AI_GREAT_PERSON_STRANDED, STRANDED) \
	DO(SAS_AI_GREAT_PERSON_SAFETY, SAFETY) \
	DO(SAS_AI_GREAT_PERSON_SKIP, SKIP)
enum SASGameRecordAIGreatPersonAction
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_GREAT_PERSON_ACTION_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
// <!-- custom: Recorder-only output from the existing Great-Merchant trade-target scan.
// AI_tradeMissionValue fills the final city, raw mission gold, full path length and this-turn waypoint only when level-2 provenance requests it; no second city scan, valuation or pathfinding is permitted for logging. (ChatGPT-5.6-Sol) -->
struct SASGreatMerchantTradeChoiceContext
{
	CvCity const* pTargetCity;
	CvPlot const* pWaypointPlot;
	int iGold;
	int iPathTurns;
	int iGoldWeight;
	int iResearchMultiplier;
	int iGoldMultiplier;
	int iFlexPercent;
	int iContinuationPercent;
	int iStrategyRandPercent;
	void reset()
	{
		pTargetCity = NULL;
		pWaypointPlot = NULL;
		iGold = 0;
		iPathTurns = -1;
		iGoldWeight = -1;
		iResearchMultiplier = -1;
		iGoldMultiplier = -1;
		iFlexPercent = -1;
		iContinuationPercent = -1;
		iStrategyRandPercent = -1;
	}
};
// <!-- custom: Record one realized ordinary Great Person decision/continuation from AI_greatPersonMove.
// Scores, slow-action details and movement state are values gameplay already computed; callers pre-gate at GameRecord level 2+ and never repeat valuation, candidate search, pathfinding or RNG for logging.
// Great-Merchant fields preserve the winning raw city/path result behind the already-recorded transformed Trade score. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIGreatPersonDecision(CvUnitAI const& kUnit, CvPlot const* pDecisionPlot, SASGameRecordAIGreatPersonAction eAction, int iChoiceRank, int iSelectedValue, int iScoreThreshold, int iSlowValue, int iSlowBaseValue, int iSlowPathTurns, MissionAITypes eSlowMissionAI, CvCity const* pSlowCity, SpecialistTypes eSpecialist, BuildingTypes eBuilding, int iDiscoverValue, TechTypes eDiscoverTech, int iGoldenAgeValue, int iTradeValue, int iCultureValue, SASGreatMerchantTradeChoiceContext const* pTradeChoice, CvPlot const* pTargetPlot, MissionAITypes ePreviousMissionAI, CvPlot const* pPreviousMissionPlot);

// <!-- custom: Closed recorder-only policy stages for AI_generalMove.
// The stage names preserve the actual ordered fallback chain rather than inventing a cross-action score that Great-General AI does not compute. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_GREAT_GENERAL_STAGE_ROWS(DO) \
	DO(SAS_AI_GREAT_GENERAL_PREFERRED_INSTRUCTOR, PREFERRED_INSTRUCTOR) \
	DO(SAS_AI_GREAT_GENERAL_FIRST_ACADEMY, FIRST_ACADEMY) \
	DO(SAS_AI_GREAT_GENERAL_FIRST_INSTRUCTOR, FIRST_INSTRUCTOR) \
	DO(SAS_AI_GREAT_GENERAL_DANGER_LEAD, DANGER_LEAD) \
	DO(SAS_AI_GREAT_GENERAL_OFFENSE_LEAD_ATTACK_CITY, OFFENSE_LEAD_ATTACK_CITY) \
	DO(SAS_AI_GREAT_GENERAL_OFFENSE_LEAD_ATTACK, OFFENSE_LEAD_ATTACK) \
	DO(SAS_AI_GREAT_GENERAL_JOIN_LIMIT_2, JOIN_LIMIT_2) \
	DO(SAS_AI_GREAT_GENERAL_ACADEMY_LIMIT_2, ACADEMY_LIMIT_2) \
	DO(SAS_AI_GREAT_GENERAL_JOIN_LIMIT_4, JOIN_LIMIT_4) \
	DO(SAS_AI_GREAT_GENERAL_RANDOM_CONSTRUCT, RANDOM_CONSTRUCT) \
	DO(SAS_AI_GREAT_GENERAL_FINAL_JOIN, FINAL_JOIN) \
	DO(SAS_AI_GREAT_GENERAL_RETREAT, RETREAT) \
	DO(SAS_AI_GREAT_GENERAL_STRANDED, STRANDED) \
	DO(SAS_AI_GREAT_GENERAL_SAFETY, SAFETY) \
	DO(SAS_AI_GREAT_GENERAL_SKIP, SKIP)
enum SASGameRecordAIGreatGeneralStage
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_GREAT_GENERAL_STAGE_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
#define SAS_RECORD_AI_GREAT_GENERAL_ACTION_ROWS(DO) \
	DO(SAS_AI_GREAT_GENERAL_ACTION_JOIN, JOIN) \
	DO(SAS_AI_GREAT_GENERAL_ACTION_CONSTRUCT, CONSTRUCT) \
	DO(SAS_AI_GREAT_GENERAL_ACTION_LEAD, LEAD) \
	DO(SAS_AI_GREAT_GENERAL_ACTION_RETREAT, RETREAT) \
	DO(SAS_AI_GREAT_GENERAL_ACTION_STRANDED, STRANDED) \
	DO(SAS_AI_GREAT_GENERAL_ACTION_SAFETY, SAFETY) \
	DO(SAS_AI_GREAT_GENERAL_ACTION_SKIP, SKIP)
enum SASGameRecordAIGreatGeneralAction
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_GREAT_GENERAL_ACTION_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
// <!-- custom: Recorder-owned optional output context for AI_generalMove's existing Great-General helper passes.
// The object is initialized/prepared only at SASGameRecord level 2+; helpers fill only the candidate/value/path they already selected.
// Keeping the definition here rather than CvUnitAI.h makes ownership explicit while CvUnitAI exposes only an opaque optional pointer. (ChatGPT-5.6-Sol) -->
struct SASGreatGeneralChoiceContext
{
	void resetChoice()
	{
		bMove = false;
		iSelectedValue = -1;
		pWaypointPlot = NULL;
		pTargetPlot = NULL;
		pTargetCity = NULL;
		eSpecialist = NO_SPECIALIST;
		eBuilding = NO_BUILDING;
		pTargetUnit = NULL;
		iTargetStrengthScore = -1;
		iTargetHealing = -1;
		iLeadByHealing = -1;
	}
	void initialize(CvPlot const* pDecisionPlot_, int iPreferThroughEra_, bool bPreferInstructorFirst_, MissionAITypes ePreviousMissionAI_, CvPlot const* pPreviousMissionPlot_)
	{
		pDecisionPlot = pDecisionPlot_;
		iPreferThroughEra = iPreferThroughEra_;
		bPreferInstructorFirst = bPreferInstructorFirst_;
		ePreviousMissionAI = ePreviousMissionAI_;
		pPreviousMissionPlot = pPreviousMissionPlot_;
	}
	void prepare(SASGameRecordAIGreatGeneralAction eAction_, SASGameRecordAIGreatGeneralStage eStage_, int iPolicyLimit_, int iValueThreshold_, int iMinStrength_, int iMinHealing_, int iRandomConstructRoll_)
	{
		resetChoice();
		eAction = eAction_;
		eStage = eStage_;
		iPolicyLimit = iPolicyLimit_;
		iValueThreshold = iValueThreshold_;
		iMinStrength = iMinStrength_;
		iMinHealing = iMinHealing_;
		iRandomConstructRoll = iRandomConstructRoll_;
	}
	CvPlot const* pDecisionPlot;
	SASGameRecordAIGreatGeneralAction eAction;
	SASGameRecordAIGreatGeneralStage eStage;
	int iPolicyLimit;
	int iValueThreshold;
	int iMinStrength;
	int iMinHealing;
	int iPreferThroughEra;
	bool bPreferInstructorFirst;
	int iRandomConstructRoll;
	MissionAITypes ePreviousMissionAI;
	CvPlot const* pPreviousMissionPlot;
	bool bMove;
	int iSelectedValue;
	CvPlot const* pWaypointPlot;
	CvPlot const* pTargetPlot;
	CvCity const* pTargetCity;
	SpecialistTypes eSpecialist;
	BuildingTypes eBuilding;
	CvUnit const* pTargetUnit;
	int iTargetStrengthScore;
	int iTargetHealing;
	int iLeadByHealing;
};
// <!-- custom: Record one realized Great-General policy stage from AI_generalMove.
// Optional helper context is filled by the live AI_join/AI_construct/AI_lead pass only when level 2+ is enabled; no target search, valuation, pathfinding or RNG is repeated for recording. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIGreatGeneralDecision(CvUnitAI const& kUnit, SASGreatGeneralChoiceContext const& kChoice);
// <!-- custom: Recorder-owned optional output from the live espionage chooser.
// Plain scalars preserve already-computed winning/runner-up values only when the caller enables level 2+ recording; no mission is reevaluated for logging. (ChatGPT-5.6-Sol) -->
struct SASEspionageCandidateContext
{
	EspionageMissionTypes eMission;
	int iData;
	int iRawValue;
	int iRandomPercent;
	int iRandomizedValue;
	int iOverhead;
	int iCostPenalty;
	int iCost;
	int iFinalValue;
};
struct SASEspionageChoiceContext
{
	int iSpyValue;
	int iEstimatedBaseInterceptPercent;
	int iEscapeCost;
	int iEspionagePoints;
	int iEspionageRate;
	SASEspionageCandidateContext kBest;
	SASEspionageCandidateContext kRunnerUp;
	bool bBigEspionage;
	bool bEspionageEconomy;
};
// <!-- custom: Recorder-owned optional output from the live technology chooser.
// AI_bestTech fills these path/candidate values only for its real research/free-tech caller when SASGameRecord requests them; no technology valuation or chooser RNG is repeated for logging. (ChatGPT-5.6-Sol) -->
struct SASTechChoiceCandidate
{
	TechTypes eTech;
	TechTypes eAimTech;
	int iImmediateValue;
	int iPathValue;
};
struct SASTechChoiceContext
{
	explicit SASTechChoiceContext(bool bCollect = false) : eAimTech(NO_TECH), eRunnerUpTech(NO_TECH), eRunnerUpAimTech(NO_TECH), iBestImmediateValue(-1), iBestPathValue(-1), iRunnerUpImmediateValue(-1), iRunnerUpPathValue(-1), iSelectedPathIndex(-1), iImmediateCandidateCount(0), iEvaluatedTechCount(0), iPathCount(0), bCollectCandidates(bCollect) {}
	TechTypes eAimTech;
	TechTypes eRunnerUpTech;
	TechTypes eRunnerUpAimTech;
	int iBestImmediateValue;
	int iBestPathValue;
	int iRunnerUpImmediateValue;
	int iRunnerUpPathValue;
	int iSelectedPathIndex;
	int iImmediateCandidateCount;
	int iEvaluatedTechCount;
	int iPathCount;
	bool bCollectCandidates;
	std::vector<SASTechChoiceCandidate> aCandidates;
};
// <!-- custom: Exact level-3 plot-owner transitions use a small recorder-owned root/mechanism vocabulary instead of guessing causes from the final setter.
// `tx` identifies the outer causal operation; this separate cause scope identifies the immediate owner-change mechanism/source. Nested cause scopes therefore override temporarily and restore on exit (e.g. tx=VASSALAGE with cause=WAR_BORDER or CULTURE_UPDATE).
// Call sites gate before gathering logging-only state; disabled scopes perform only their cheap constructor branch. NONE reports UNKNOWN rather than borrowing the active transaction kind, keeping root operation (`tx`) and immediate mechanism (`cause`) semantically distinct. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
#define SAS_RECORD_PLOT_OWNER_CHANGE_CAUSE_ROWS(DO) \
	DO(SAS_PLOT_OWNER_CAUSE_CITY_FOUNDING, CITY_FOUNDING) \
	DO(SAS_PLOT_OWNER_CAUSE_CITY_ACQUISITION, CITY_ACQUISITION) \
	DO(SAS_PLOT_OWNER_CAUSE_CULTURE_UPDATE, CULTURE_UPDATE) \
	DO(SAS_PLOT_OWNER_CAUSE_WAR_BORDER, WAR_BORDER) \
	DO(SAS_PLOT_OWNER_CAUSE_PEACE_BORDER, PEACE_BORDER) \
	DO(SAS_PLOT_OWNER_CAUSE_WORLDBUILDER, WORLDBUILDER) \
	DO(SAS_PLOT_OWNER_CAUSE_PYTHON_EXTERNAL, PYTHON_EXTERNAL)
enum SASGameRecordPlotOwnerChangeCause
{
	SAS_PLOT_OWNER_CAUSE_NONE,
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_PLOT_OWNER_CHANGE_CAUSE_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
class SASGameRecordPlotOwnerChangeCauseScope
{
public:
	// <!-- custom: begin assigns m_ePreviousCause before setting m_bActive, so a disabled scope need only initialize its one active gate; do not add recorder-only member writes back to the common level-0 path. (ChatGPT-5.6-Sol) -->
	SASGameRecordPlotOwnerChangeCauseScope(SASGameRecordPlotOwnerChangeCause eCause, bool bEnabled) : m_bActive(false)
	{
		if (bEnabled) begin(eCause);
	}
	~SASGameRecordPlotOwnerChangeCauseScope()
	{
		if (m_bActive) end();
	}
private:
	void begin(SASGameRecordPlotOwnerChangeCause eCause);
	void end();
	bool m_bActive;
	SASGameRecordPlotOwnerChangeCause m_ePreviousCause;
};
// <!-- custom: Flush older delayed recorder output before CvPlot begins mutating ownership, then emit the exact transition at the authoritative owner assignment. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void prepareSASGameRecordPlotOwnerChange();
void logSASGameRecordPlotOwnerChanged(CvPlot const& kPlot, PlayerTypes eOldOwner, PlayerTypes eNewOwner, int iOwnershipDurationBefore, bool bOwnershipScoreBefore);
// <!-- custom: Forward-declare the vote payload because SASGameRecord only passes it by pointer.
// This keeps the lightweight recorder header from needing CvStructs.h solely for diplomatic vote-source history hooks. (ChatGPT-5.6-Sol) -->
struct VoteSelectionSubData;
struct VoteSelectionData;
struct VoteTriggeredData;
// <!-- custom: Random-event lifecycle diagnostics pass the existing player-local trigger payload by const pointer/reference without exposing its save-layout definition through this lightweight recorder header. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
struct EventTriggeredData;
// <!-- custom: Random-event city-result logging snapshots only realized city state that can otherwise disappear between periodic rows.
// The caller captures before/after only at level 2+.
// The default object is only a caller-gated placeholder that is assigned a real captured state before any read, so intentionally leave it uninitialized rather than writing every field while logging is disabled. (ChatGPT-5.6-Sol) -->
struct SASGameRecordRandomEventCityState
{
	SASGameRecordRandomEventCityState() {}
	explicit SASGameRecordRandomEventCityState(CvCity const& kCity, EventTypes eEvent);
	int iPopulation;
	int iFood;
	int iFoodYield;
	int iProductionYield;
	int iCommerceYield;
	int iGoldRate;
	int iResearchRate;
	int iCultureRate;
	int iEspionageRate;
	int iOwnerCultureTimes100;
	int iOccupationTurns;
	int iCultureUpdateTurns;
	int iExtraHappiness;
	int iExtraHealth;
	int iHurryAngerTurns;
	int iHappinessTurns;
	int iAngryPopulation;
	int iHappyLevel;
	int iUnhappyLevel;
	int iGoodHealth;
	int iBadHealth;
	int iSpaceProductionModifier;
	int iFreeSpecialistInstances;
	BuildingTypes eBuilding;
	int iRealBuildingCount;
};
// <!-- custom: Unit-local EventInfo effects are rare but can heal/award XP, immobilize, rename, promote or disband the selected stored unit.
// Snapshot only the fields consumed by CvUnit::applyEvent so the result row remains compact and gameplay-owned semantics stay authoritative. (ChatGPT-5.6-Sol) -->
struct SASGameRecordRandomEventUnitState
{
	SASGameRecordRandomEventUnitState();
	explicit SASGameRecordRandomEventUnitState(CvUnit const& kUnit, EventTypes eEvent);
	int iExists;
	int iUnitId;
	UnitTypes eUnit;
	UnitAITypes eUnitAI;
	int iX;
	int iY;
	int iDamage;
	int iExperience;
	int iImmobileTurns;
	PromotionTypes ePromotion;
	int iHasPromotion;
};

// <!-- custom: Player-level EventInfo consequences can otherwise be visible only indirectly at a later snapshot.
// Keep this to durable native values not already covered by dedicated gold/tech/Golden-Age/war rows.
// The default object is only a caller-gated placeholder assigned a real captured state before any read, so its constructor intentionally does no level-0 recorder work. (ChatGPT-5.6-Sol) -->
struct SASGameRecordRandomEventPlayerState
{
	SASGameRecordRandomEventPlayerState() {}
	SASGameRecordRandomEventPlayerState(CvPlayer const& kPlayer, EventTypes eEvent, PlayerTypes eOtherPlayer);
	int iExtraHappiness;
	int iExtraHealth;
	int iBaseFreeUnits;
	int iSpaceProductionModifier;
	int iInflationRate;
	int iEspionagePointsAgainstOther;
	BonusTypes eBonusRevealed;
	int iForceRevealedBonus;
};
// <!-- custom: Session-local transaction IDs tie together structured rows emitted synchronously by one consequential gameplay operation after filtering/chunking has separated them from raw file order.
// A nested recorder scope deliberately joins the already-active outer transaction instead of creating parent/child IDs; this keeps one causal chain for operations such as city acquisition -> immediate AI/auto raze.
// The recorder owns ID allocation and row decoration.
// Callers only bracket the existing operation, pre-gated at the detail level that already records its consequences. (ChatGPT-5.6-Sol) -->
class SASGameRecordTransactionScope
{
public:
	SASGameRecordTransactionScope(char const* szKind, bool bEnabled) : m_bOwnsTransaction(false)
	{
		if (bEnabled) begin(szKind);
	}
	~SASGameRecordTransactionScope()
	{
		if (m_bOwnsTransaction) end();
	}
private:
	void begin(char const* szKind);
	void end();
	bool m_bOwnsTransaction;
};

// <!-- custom: Observe one AI_chooseProduction call as a scope so every early return is handled without teaching the AI decision tree about recorder schema.
// When level 2+ is pre-enabled by the caller, the destructor compares the final head order with the entry state and records only meaningful switches, clears, or resumptions of stored production.
// Earlier versions put the whole constructor/destructor in SASGameRecordLog.cpp.
// In ordinary Release builds that hides the disabled fast path across the translation-unit boundary.
// Final_Release /GL + /LTCG may recover such inlining, but keeping these tiny wrappers here makes the cheap path visible in every build while begin/end remain cold level-2+ work. (ChatGPT-5.6-Sol) -->
class SASGameRecordAIProductionChoiceScope
{
public:
	// <!-- custom: m_pCity remains the gate: disabled construction leaves it NULL, and the inline destructor keeps the old null check before calling end().
	// No recorder state besides the pointer is touched at level 0/1. (ChatGPT-5.6-Sol) -->
	SASGameRecordAIProductionChoiceScope(CvCity const& kCity, bool bEnabled) : m_pCity(NULL)
	{
		if (bEnabled) begin(kCity);
	}
	~SASGameRecordAIProductionChoiceScope()
	{
		if (m_pCity != NULL) end();
	}
private:
	void begin(CvCity const& kCity);
	void end();
	CvCity const* m_pCity;
	OrderTypes m_eOldOrder;
	int m_iOldData1;
	int m_iOldStored;
	int m_iOldNeeded;
	int m_iOldTurnsLeft;
	int m_iOldAccumulatedInactiveTurns;
};
// <!-- custom: Goody huts are rare but can have compound randomized effects (gold, map reveal, partial/full tech, free/promoted units, hostile units and AdvCiv follow-up outcomes).
// Collect only realized effects at the authoritative gameplay boundary, then let SASGameRecord own the stable row formatting.
// Callers must gate level 2+ before doing logging-only measurements.
// The result object itself is intentionally cheap to default-construct (scalar sentinels plus empty vectors); avoid pointer/optional lifetime machinery merely to skip this tiny setup when recording is disabled. (ChatGPT-5.6-Sol) -->
struct SASGameRecordGoodyResult
{
	SASGameRecordGoodyResult();
	bool bFollowupOutcome;
	bool bUpgradeRoll;
	bool bUpgradeApplied;
	bool bAdditionalOutcomeAttempted;
	int iGold;
	int iNewlyRevealedPlots;
	int iExperienceGained;
	int iDamageHealed;
	TechTypes eTech;
	int iTechRewardValue;
	int iTechProgressBefore;
	int iTechProgressAfter;
	int iTechCost;
	bool bTechCompleted;
	int iFreePromotionsGranted;
	std::vector<CvUnit const*> apFreeUnits;
	std::vector<CvUnit const*> apBarbarianUnits;
};
// <!-- custom: Diplomacy logging declarations below use CvDeal, TradeData and CLinkList only by reference/pointer.
// Forward declarations avoid pulling deal/trade-list implementation headers into every SASGameRecordLog.h includer. (ChatGPT-5.6-Sol) -->
class CvDeal;
struct TradeData;
template <class tVARTYPE> class CLinkList;
// <!-- custom: Capture a plot before a logical action so one combined record can describe terrain, feature, resource, improvement, route and permanent event-yield changes.
// Every default-constructed instance is a caller-gated placeholder that is assigned SASGameRecordPlotState(kPlot) before any read.
// Intentionally leave it uninitialized: initializing five enum fields plus the yield array would impose recorder-only writes at level 0, including inside nuke/global-warming plot loops.
// Never read a default instance unless the matching capture gate ran. (GPT-5.6-Sol + ChatGPT-5.6-Sol) -->
struct SASGameRecordPlotState
{
	SASGameRecordPlotState() {}
	explicit SASGameRecordPlotState(CvPlot const& kPlot);
	TerrainTypes eTerrain;
	FeatureTypes eFeature;
	BonusTypes eBonus;
	ImprovementTypes eImprovement;
	RouteTypes eRoute;
	int aiExtraYield[NUM_YIELD_TYPES];
};
void logSASGameRecordTurn(int iGameTurn);
// <!-- custom: Plot changes and permanent team map revelation are buffered into compact coordinate lists and flushed once per turn; detailed before/after rows are reserved for non-routine causes.
// The environment hook receives values already computed by CvGame::doGlobalWarming so it adds no map scan. (GPT-5.6-Sol) -->
void flushSASGameRecordTurnChanges(int iGameTurn);
void recordSASGameRecordPlotChange(CvPlot const& kPlot, SASGameRecordPlotState const& kOldState, char const* szCategory, char const* szCause, bool bDetailed);
// <!-- custom: Aggregate completed Worker Builds by player/turn so broad records preserve improvement churn and irrigation outcomes without copying per-candidate BBAI scoring or emitting one verbose row per Build; caller must pre-gate at GameRecord level 2+. (GPT-5.6-Sol) -->
void recordSASGameRecordWorkerBuild(CvPlot const& kPlot, SASGameRecordPlotState const& kOldState, PlayerTypes ePlayer, BuildTypes eBuild);
// <!-- custom: Directional river edits are rare and independent from ordinary plot-state actions, so record them separately instead of adding unused river fields to every detailed plot-change row.
// Callers gate this helper before computing logging-only arguments. (GPT-5.6-Sol) -->
void logSASGameRecordRiverEdgeChanged(CvPlot const& kPlot, bool bOldSouthBoundary, bool bOldEastBoundary);
void recordSASGameRecordPlotRevealed(CvPlot const& kPlot, TeamTypes eTeam);
// <!-- custom: Map-visible technologies reveal every plot through thousands of ordinary setRevealed calls.
// Bracket that bulk operation so the record writes one exact full-map row instead of redundant coordinate chunks. (GPT-5.6-Sol) -->
void beginSASGameRecordFullMapRevelation(TeamTypes eTeam, TechTypes eTech);
void endSASGameRecordFullMapRevelation(TeamTypes eTeam, TechTypes eTech);
void logSASGameRecordEnvironmentTurn(int iPollution, int iSustainabilityThreshold, int iLandDefense, int iIndexBefore, int iIndexBeforeRestoration, int iIndexEnd, int iWarmingChances, int iEventTally);
// <!-- custom: High-level research-plan mutations can tag a shared ResearchTargetChangeCause for the recorder; the later player-turn observer emits it only if an invested incomplete-tech redirection actually materializes. (ChatGPT-5.6-Sol) -->
void noteSASGameRecordResearchTargetChangeCause(PlayerTypes ePlayer, ResearchTargetChangeCause eCause);
// <!-- custom: Preserve the exact fresh-research/carried-overflow split only for level-2 ordinary research application; the recorder consumes it if that same call completes the technology. (ChatGPT-5.6-Sol) -->
void noteSASGameRecordResearchApplication(PlayerTypes ePlayer, TechTypes eTech, int iModifiedResearchRate, int iIncomingOverflowUnmodified, int iIncomingOverflowModified);
// <!-- custom: This mixed-level player-turn updater is called whenever SASGameRecord is enabled because Golden Age/anarchy counters support lower-detail rows.
// Only the research-target comparison self-gates at level 2+, so do not pre-gate the whole helper at level 2. (ChatGPT-5.6-Sol) -->
void updateSASGameRecordPlayerTurnState(PlayerTypes ePlayer);
// <!-- custom: Fog-spawn call sites pass the explicit Barbarian-unit source; ordinary city production continues through logSASGameRecordUnitCompleted. (GPT-5.6-Sol) -->
void logSASGameRecordBarbarianSpawn(CvUnit const* pUnit, char const* szCause);
// <!-- custom: Barbarian city creation already performs a full gameplay-required CitySiteEvaluator scan; pass its exact top chooser results to SASGameRecord instead of trying to reuse normal-civ AI city-site caches. (ChatGPT-5.6-Sol) -->
void logSASGameRecordBarbarianCitySiteChoice(bool bSkipCivAreas, int iProbModifierPercent, int iTargetCitiesMultiplier, int iDiscouragedRange, CvPlot const* const apPlots[], int const aiRawValues[], int const aiAreaValues[], int const aiFinalValues[], int const aiRandomPercents[], int iCandidateCount);
// <!-- custom: Level-2 goody rows preserve each realized hut outcome; level 3 keeps the existing exact Barbarian-spawn rows as complementary tactical detail. (ChatGPT-5.6-Sol) -->
void logSASGameRecordGoodyReceived(PlayerTypes ePlayer, CvPlot const* pPlot, CvUnit const* pTriggerUnit, GoodyTypes eGoody, SASGameRecordGoodyResult const& kResult);
void logSASGameRecordGoodyNoOutcome(PlayerTypes ePlayer, CvPlot const* pPlot, CvUnit const* pTriggerUnit, GoodyTypes eTaboo, int iAttempts);
// <!-- custom: Random-event delivery paths, apply dispositions, expiry reasons, affected scopes, and occurrence-clear scopes are separate small diagnostic vocabularies, each currently produced by one tightly coupled gameplay path and consumed only by SASGameRecord.
// Keep their fixed string literals instead of conflating them in one permissive catch-all enum or adding several one-consumer enums and conversion switches.
// Promote an individual vocabulary to a typed enum if another independent producer or subsystem begins reusing it. (GPT-5.6-Sol) -->
// <!-- custom: Civ4 EventInfo/random-event history is deliberately separate from generic GAME_RECORD_ACTION rows.
// Calls are level-2 pre-gated at authoritative gameplay boundaries so disabled/level-1 runs do not collect target validity or construct diagnostic strings. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordRandomEventTriggered(CvPlayer const& kPlayer, EventTriggeredData const& kTriggeredData, char const* szDeliveryPath);
void logSASGameRecordRandomEventNoSelection(CvPlayer const& kPlayer, EventTriggeredData const& kTriggeredData, char const* szResolution);
void logSASGameRecordRandomEventApply(CvPlayer const& kPlayer, EventTypes eEvent, int iTriggeredId, EventTriggeredData const* pTriggeredData, bool bUpdateTrigger, char const* szDisposition, int iCanDoEvent, int iTriggerFiredBefore, int iEventOccurredBefore);
void logSASGameRecordRandomEventGoldResult(CvPlayer const& kPlayer, EventTypes eEvent, int iTriggeredId, int iRangeLow, int iRangeHigh, int iPlayerGoldDelta, PlayerTypes eOtherPlayer, bool bGoldToPlayer);
void logSASGameRecordRandomEventTechResult(CvPlayer const& kPlayer, EventTypes eEvent, int iTriggeredId, TechTypes eTech, int iTechPercent, int iResearchBefore, int iBeakersApplied, int iResearchAfter, int iTechCost, int iCompleted);
// <!-- custom: Preserve realized deterministic city consequences separately from EventInfo selection.
// This complements canonical war/plot/tech rows without dumping static XML magnitudes into RANDOM_EVENT_APPLY.
// Call only at level 2+ with caller-captured before/after state. (ChatGPT-5.6-Sol) -->
void logSASGameRecordRandomEventCityResult(PlayerTypes ePlayer, PlayerTypes eAffectedPlayer, int iTriggeredId, EventTypes eEvent, char const* szScope, CvCity const& kCity, SASGameRecordRandomEventCityState const& kBefore, SASGameRecordRandomEventCityState const& kAfter);
// <!-- custom: Building modifier EventInfos can create durable latent state even when current yields do not change (e.g. before the affected building exists).
// Record realized modifier operations separately rather than bloating every city-result row; call only at level 2+. (ChatGPT-5.6-Sol) -->
void logSASGameRecordRandomEventBuildingModifierResults(CvPlayer const& kPlayer, EventTypes eEvent, int iTriggeredId, char const* szScope, CvCity const* pCity);
// <!-- custom: Complete the EventInfo result layer with concrete stored-unit, player-wide and persistent free-promotion consequences; call only at level 2+ with state already captured around the authoritative gameplay operation. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordRandomEventUnitResult(PlayerTypes ePlayer, int iTriggeredId, EventTypes eEvent, SASGameRecordRandomEventUnitState const& kBefore, SASGameRecordRandomEventUnitState const& kAfter);
void logSASGameRecordRandomEventPlayerResult(CvPlayer const& kPlayer, EventTypes eEvent, int iTriggeredId, PlayerTypes eOtherPlayer, SASGameRecordRandomEventPlayerState const& kBefore, SASGameRecordRandomEventPlayerState const& kAfter);
void logSASGameRecordRandomEventFreePromotionResult(CvPlayer const& kPlayer, EventTypes eEvent, int iTriggeredId, char const* szScope, int iScopeId, PromotionTypes ePromotion, int iExistingUnitsNewlyPromoted, int iFreePromotionBefore, int iFreePromotionAfter);
void logSASGameRecordRandomEventFreeUnitsResult(PlayerTypes ePlayer, PlayerTypes eAffectedPlayer, EventTypes eEvent, int iTriggeredId, UnitClassTypes eUnitClass, UnitTypes eUnit, int iRequestedCount, int iCreatedCount, CvCity const* pSpawnCity);
void logSASGameRecordRandomEventOccurrenceCleared(CvPlayer const& kPlayer, EventTypes eSourceEvent, EventTypes eClearedEvent, int iTriggeredId, int iClearChance, char const* szScope, TeamTypes eScopeTeam, int iScopePlayerSlots, int iScopeEverAlivePlayers, int iClearedOccurrences);
void logSASGameRecordRandomEventExpired(CvPlayer const& kPlayer, EventTypes eEvent, EventTriggeredData const& kTriggeredData, char const* szReason);
void logSASGameRecordRandomEventPillageResult(char const* szScope, PlayerTypes ePlayer, PlayerTypes eAffectedPlayer, int iCityId, int iTriggeredId, EventTypes eEvent, int iMinPillage, int iMaxPillage, int iAttempts, int iDestroyed);
void logSASGameRecordRandomEventCountdownScheduled(CvPlayer const& kPlayer, EventTypes eSourceEvent, EventTypes eFollowupEvent, int iTriggeredId, int iRequestedDueTurn, int iPreviousDueTurn, int iScheduledDueTurn);
// <!-- custom: Keep factual unit-completion provenance explicit: ordinary production, true population conscription, and free colony bootstrap defenders share unit-init machinery but have different strategic/economic meaning. (ChatGPT-5.6-Sol) -->
enum SASGameRecordUnitCompletionSource
{
	SAS_UNIT_COMPLETION_PRODUCTION = 0,
	SAS_UNIT_COMPLETION_CONSCRIPT,
	SAS_UNIT_COMPLETION_COLONY_FREE_DEFENDER
};
void logSASGameRecordUnitCompleted(CvCity const* pCity, CvUnit const* pUnit, SASGameRecordUnitCompletionSource eSource, int iRawModifiedOverflow = 0, int iUnmodifiedOverflow = 0, int iKeptOverflow = 0, int iLostProduction = 0, int iUnusedOverflowCapacity = 0, int iOverflowGold = 0);
// <!-- custom: Research completion has its own accounting row because generic TECH_ACQUIRED also covers trades, free technologies, espionage and other sources where research overflow fields would be meaningless.
// Call only for actual TECH_ACQUISITION_RESEARCH threshold crossings at level 2+. (ChatGPT-5.6-Sol) -->
void logSASGameRecordResearchCompleted(TechTypes eTech, TeamTypes eTeam, PlayerTypes ePlayer, int iProgressBefore, int iProgressBeforeClamp, int iResearchModifier, int iUnmodifiedOverflow);
// <!-- custom: Added eCause so the existing TECH_ACQUIRED action can name its explicit source. (GPT-5.6-Sol + GPT-5.6 Thinking) -->
void logSASGameRecordTechAcquired(TechTypes eType, TeamTypes eTeam, PlayerTypes ePlayer, TechAcquisitionCause eCause);
// <!-- custom: Preserve the exact ordinary Settler target assignment that precedes travel/founding without replaying site selection.
// This compact level-2 event records already-computed shortlist/path scores only when the MISSIONAI_FOUND target is newly assigned or retargeted; repeated travel toward the same target emits nothing. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_SETTLER_SITE_DECISION_SOURCE_ROWS(DO) \
	DO(SAS_AI_SETTLER_SITE_DECISION_SHORTLIST, SHORTLIST_PATH_SCORE) \
	DO(SAS_AI_SETTLER_SITE_DECISION_CURRENT_PLOT_RECHECK, CURRENT_PLOT_RECHECK)
enum SASGameRecordAISettlerSiteDecisionSource
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_SETTLER_SITE_DECISION_SOURCE_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
// <!-- custom: Removed the previous-mission parameters after folding SAS_recordSettlerSiteDecisionIfChanged into this helper; it now reads prior mission state and suppresses unchanged destinations internally.
// Call before mission dispatch because founding can consume the Settler; this records the choice, not successful movement/founding. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
void logSASGameRecordAISettlerSiteDecision(CvUnitAI const& kSettler, CvPlot const& kTargetPlot, bool bFoundNow, SASGameRecordAISettlerSiteDecisionSource eSource, int iDecisionRawFoundValue, int iDecisionSelectionFoundValue, int iPathTurns, int iPathAdjustedScore, bool bSafe);
// <!-- custom: MISSIONAI_EXPLORE bypasses the ordinary Settler-site assignment row, so preserve only realized scouting actions rather than BBAI's rejected-candidate dump.
// First-city rows retain the already-computed information/upside/delay terms that explain whether another scouting turn was worth postponing the capital; later promising-fog rows retain the selected-site versus scout-site evidence that caused the detour. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_SETTLER_SCOUT_END_REASON_ROWS(DO) \
	DO(SAS_AI_SETTLER_SCOUT_END_DEADLINE, DEADLINE) \
	DO(SAS_AI_SETTLER_SCOUT_END_LOW_UPSIDE, LOW_UPSIDE) \
	DO(SAS_AI_SETTLER_SCOUT_END_NO_SAFE_STEP, NO_SAFE_STEP)
enum SASGameRecordAISettlerScoutEndReason
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_SETTLER_SCOUT_END_REASON_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
void logSASGameRecordAIFirstCityCoreScoutStep(CvUnitAI const& kSettler, CvPlot const& kTargetPlot, CvPlot const* pEndTurnPlot, int iCurrentFoundValue, int iTargetFoundValue, int iCurrentBest6, int iCurrentBest10, int iTargetBest6, int iTargetBest10, int iCoreGrowthValue, int iAdjustedValue, int iPathTurns, int iElapsedTurns, int iMaxFirstCityTurns);
void logSASGameRecordAIFirstCityScoutStep(CvUnitAI const& kSettler, CvPlot const& kEndTurnPlot, CvPlot const* pScoutOrigin, CvPlot const* pSettleNowPlot, int iSettleNowFoundValue, int iSettleNowBest6, int iSettleNowBest10, int iSettleNowCoreQualityPercent, int iInformationValue, int iDiscountedInformationValue, int iScoutOpportunityLinearPercent, int iScoutOpportunityPercent, int iEndTurnFoundValue, int iSettleNowFoundValueGain, int iScoutExpectedGain, int iScoutDelayBaseCost, int iScoutWindowProgressPercent, int iScoutTimePressurePercent, int iScoutDelayCost, int iDirectionScore, int iElapsedTurns, int iMaxFirstCityTurns);
void logSASGameRecordAIFirstCityScoutEnd(CvUnitAI const& kSettler, SASGameRecordAISettlerScoutEndReason eReason, bool bFoundNow, CvPlot const* pTargetPlot, int iSettleNowFoundValue, int iAdjustedValue, int iSettleNowBest6, int iSettleNowBest10, int iSettleNowCoreQualityPercent, int iScoutOpportunityLinearPercent, int iScoutOpportunityPercent, int iBestScoutExpectedGain, int iScoutDelayBaseCost, int iScoutWindowProgressPercent, int iScoutTimePressurePercent, int iScoutDelayCost, int iPathTurns, int iElapsedTurns, int iMaxFirstCityTurns);
void logSASGameRecordAISettlerFogScoutStep(CvUnitAI const& kSettler, CvPlot const& kSelectedSite, CvPlot const& kScoutSite, CvPlot const& kEndTurnPlot, int iSelectedFoundValue, int iScoutFoundValue, int iSelectedRevealedBFC, int iSelectedUnrevealedBFC, int iScoutRevealedBFC, int iScoutUnrevealedBFC, int iSelectedPathTurns, int iScoutPathTurns);
// <!-- custom: Diagnose only the rare naval case where a loaded ship competes with another eligible defender.
// The row is level-3-only and records the selected ship plus the best alternative under the same defender rules, including LFB odds/value/cargo protection context.
// Keep it observational: transported cargo is not a defender candidate and inherited K-Mod LFB already contains loaded-transport protection, so an external report alone is not evidence for changing combat selection. (ChatGPT-5.6-Sol) -->
void logSASGameRecordNavalTransportDefenderSelection(CvUnit const& kAttacker, CvUnit const& kSelectedDefender, CvPlot const& kPlot);
// <!-- custom: Recorder-owned numeric founding-time Food evidence; the evaluator fills this optional output from its internal plot data, and SASGameRecord formats it afterward.
// Keep this recorder-only container here alongside the other choice contexts; no strings or persistent state are attached to normal AI evaluations.
// Core sizes count worked non-home plots; the free city center contributes yields without consuming a slot.
// bAvailable distinguishes rejected/incomplete evaluations, while core availability distinguishes insufficient accepted plots from a genuine negative surplus; unsupported score -1 means no measurable Food-feasible comparison. See KI#144.4. (GPT-6.1-Sol) -->
struct SASGameRecordCitySiteFoodSupportResult
{
	enum { CORE_COUNT = 4 };
	bool bAvailable;
	int iKnownUsablePlots;
	int iUnrevealedPlots;
	int iAppliedFoodSupportPenalty;
	bool abCoreAvailable[CORE_COUNT];
	int aiValueFoodSurplus[CORE_COUNT];
	int aiValueNaturalFoodSurplus[CORE_COUNT];
	int aiFoodRankedSurplus[CORE_COUNT];
	int aiUnsupportedProjectedScore[CORE_COUNT];
	SASGameRecordCitySiteFoodSupportResult() : bAvailable(false), iKnownUsablePlots(0), iUnrevealedPlots(0), iAppliedFoodSupportPenalty(0)
	{
		for (int i = 0; i < CORE_COUNT; ++i)
		{
			abCoreAvailable[i] = false;
			aiValueFoodSurplus[i] = aiValueNaturalFoodSurplus[i] = aiFoodRankedSurplus[i] = 0;
			aiUnsupportedProjectedScore[i] = -1;
		}
	}
	// <!-- custom: Share the recorder's four observation horizons between collection and output; these counts are diagnostic choices, not XML food-consumption constants. (GPT-6.1-Sol) -->
	static int coreSize(int i)
	{
		FAssert(i >= 0 && i < CORE_COUNT);
		int const aiCoreSizes[CORE_COUNT] = {3, 6, 10, 14};
		return aiCoreSizes[i];
	}
};

// <!-- custom: Preserve the strategic shortlist plus one cold quality summary for the site that is actually founded, immediately before normal founding mutates the plot/city state.
// Level 2 compares the chosen BFC from player-known and diagnostic-omniscient perspectives; rejected-candidate reasons remain BBAI territory and periodic snapshots do no settlement rescoring. (ChatGPT-5.6-Sol) -->
// <!-- custom: Added player-known Food-support summaries and a fresh comparison of one cached alternative at this same pre-founding boundary; no gameplay choice is recomputed for use by the AI. See KI#144.4. (GPT-6.1-Sol) -->
void logSASGameRecordCityFoundingSite(CvPlayer const& kPlayer, CvPlot const& kPlot);
void logSASGameRecordCityBuilt(CvCity const* pCity);
// <!-- custom: City razing is rare but strategically consequential.
// Capture level-2 context immediately before destruction, then finalize after disband so exact land/population/victory deltas reflect what the raze actually changed.
// The recorder owns the context stack and formatting; callers only pre-gate and bracket the existing raze. (ChatGPT-5.6-Sol) -->
void beginSASGameRecordCityRaze(CvCity const* pCity, PlayerTypes ePlayer);
void endSASGameRecordCityRaze(PlayerTypes ePlayer);
void logSASGameRecordCityAcquired(PlayerTypes eOldOwner, PlayerTypes eNewOwner, CvCity const* pCity, bool bConquest, bool bTrade);
void logSASGameRecordWarStarted(TeamTypes eDeclarer, TeamTypes eTarget, WarPlanTypes eWarPlan, bool bPrimaryDoW, bool bNewDiplo, PlayerTypes eSponsor, bool bRandomEvent, WarDeclarationCause eCause);
// <!-- custom: Added the pre-reset war-success and peace-context parameters so synthetic war summaries retain the final result before Base AdvCiv's AI_postMakePeace clears it. (GPT-5.6-Sol) -->
void logSASGameRecordWarEnded(TeamTypes eTeam, TeamTypes eOtherTeam, int iTeamAWarSuccess, int iTeamBWarSuccess, bool bCapitulate, TeamTypes eBroker, bool bRandomEvent, bool bReparations);
// <!-- custom: Record the exact team-membership boundary before CvTeam::addTeam reassigns the absorbed players and erases their old membership.
// Call only at level 2+. (ChatGPT-5.6-Sol) -->
void logSASGameRecordTeamMerged(TeamTypes eSurvivingTeam, TeamTypes eAbsorbedTeam);
void logSASGameRecordTeamMet(TeamTypes eTeam, TeamTypes eOtherTeam, bool bNewDiplo, int iX1, int iY1, int iX2, int iY2, CvPlot const* pTeamContactPlot, CvPlot const* pOtherContactPlot);
void logSASGameRecordPlayerGoldTrade(PlayerTypes eFromPlayer, PlayerTypes eToPlayer, int iAmount);
// <!-- custom: Recorder-owned raw deal/event serializers keep SASGameRecord schema text out of CvDeal/CvPlayer while their authoritative gameplay call sites retain the existing level gates and already-live values.
// These helpers perform no chooser/validity work beyond formatting the passed deal/event state. (ChatGPT-5.6-Sol) -->
void logSASGameRecordDealAction(CvDeal const& kDeal, CLinkList<TradeData> const& kFirstList, CLinkList<TradeData> const& kSecondList, bool bCheckAllowed, bool bMakingPeace, bool bAIRequest, TeamTypes ePeaceTradeTarget, TeamTypes eWarTradeTarget);
void logSASGameRecordDealEndAction(CvDeal const& kDeal, bool bKillTeam, bool bUpdateAttitude, PlayerTypes eCancelPlayer);
void logSASGameRecordTradeItemAction(char const* szActionType, int iDealId, TradeData const& kItem, PlayerTypes eFromPlayer, PlayerTypes eToPlayer, bool bFlagA, bool bFlagB, PlayerTypes eCancelPlayer = NO_PLAYER);
bool isSASGameRecordLowValueDiploEvent(DiploEventTypes eDiploEvent);
void logSASGameRecordDiploEventAction(PlayerTypes ePlayer, DiploEventTypes eDiploEvent, PlayerTypes eOtherPlayer, int iData1, int iData2);
// <!-- custom: Capture only the compact relationship state needed to describe a resolved human diplomacy interaction before/after CvPlayer::handleDiploEvent; callers gate at SASGameRecord level 2+ before capture. (ChatGPT-5.6-Sol) -->
struct SASGameRecordDiploRelationState
{
	int iActorAttitude;
	int iOtherAttitude;
	int aiActorMemory[NUM_MEMORY_TYPES];
	int aiOtherMemory[NUM_MEMORY_TYPES];
	WarPlanTypes eActorWarPlan;
	WarPlanTypes eOtherWarPlan;
	bool bAtWar;
};
bool isSASGameRecordResolvedDiploInteraction(DiploEventTypes eDiploEvent);
void captureSASGameRecordDiploRelationState(PlayerTypes eActor, PlayerTypes eOther, SASGameRecordDiploRelationState& kState);
void logSASGameRecordResolvedDiploInteraction(PlayerTypes eActor, DiploEventTypes eDiploEvent, PlayerTypes eOther, int iData1, SASGameRecordDiploRelationState const& kBefore, SASGameRecordDiploRelationState const& kAfter);
// <!-- custom: EXE trade-table wrappers expose authoritative submitted human->AI offer/counterproposal boundaries; call only at level 2+ so exact trade-list serialization stays out of disabled/low-detail logging. (ChatGPT-5.6-Sol) -->
void logSASGameRecordDiploOfferEvaluated(PlayerTypes eProposer, PlayerTypes eResponder, CLinkList<TradeData> const& kProposerGives, CLinkList<TradeData> const& kResponderGives, int iChange, bool bAccepted, SASGameRecordDiploRelationState const& kBefore, SASGameRecordDiploRelationState const& kAfter);
void logSASGameRecordDiploCounterProposal(PlayerTypes eProposer, PlayerTypes eResponder, CLinkList<TradeData> const& kOriginalProposerGives, CLinkList<TradeData> const& kOriginalResponderGives, CLinkList<TradeData> const& kProposerAdds, CLinkList<TradeData> const& kResponderAdds, bool bProposed);
// <!-- custom: AI->human rejected ordinary offers are visible only at the Python diplomacy UI boundary; the CyGame bridge rebuilds these lists and calls this canonical recorder formatter. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIToHumanOfferRejected(PlayerTypes eProposer, PlayerTypes eResponder, CLinkList<TradeData> const& kProposerGives, CLinkList<TradeData> const& kResponderGives);
// <!-- custom: Preserve one realized proactive AI diplomacy intent only after the live branch has formed a real contact/deal package.
// `subject` is reserved for non-package requests such as religion/civic pressure, joint war or embargo; specialized peace/resource-trade provenance remains authoritative for those paths. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIDiploContactIntent(CvPlayerAI const& kPlayer, PlayerTypes eTarget, ContactTypes eContact, TradeData const* pSubject, CLinkList<TradeData> const* pAIGives, CLinkList<TradeData> const* pAIReceives);
// <!-- custom: AI help/tribute requests can originate in ordinary AI_doDiplo or UWAI's amendTensions pass; the generic revision-100 contact package cannot recover that caller or the live request chooser values.
// Keep the origin and helper-specific output contexts recorder-owned and optional so logging-off gameplay performs no extra chooser bookkeeping. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_HUMAN_REQUEST_ORIGIN_ROWS(DO) \
	DO(SAS_AI_HUMAN_REQUEST_AI_DO_DIPLO, AI_DO_DIPLO) \
	DO(SAS_AI_HUMAN_REQUEST_UWAI_AMEND_TENSIONS, UWAI_AMEND_TENSIONS)
enum SASGameRecordAIHumanRequestOrigin
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_HUMAN_REQUEST_ORIGIN_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
struct SASGameRecordAIHelpRequestContext
{
	SASGameRecordAIHumanRequestOrigin eOrigin;
	TechTypes eBestTech;
	int iTechValue;
	int iBestCityId;
	int iCityValue;
	int iCityChoiceRoll;
	bool bBestCityLiberation;
	TradeableItems eSelectedItemType;
	int iSelectedItemData;
};
struct SASGameRecordAITributeRequestContext
{
	SASGameRecordAIHumanRequestOrigin eOrigin;
	AIDemandTypes eDemand;
	int iMinValueX100;
	int iFinalDealValue;
	TechTypes eMostUsefulTech;
	TechTypes eSelectedTech;
	int iSelectedTechSortScore;
	int iSelectedTechTradeValue;
	int iMapTradeValue;
	bool bMapAdded;
};
void logSASGameRecordAIHelpRequest(CvPlayerAI const& kPlayer, PlayerTypes eHuman, SASGameRecordAIHelpRequestContext const& kContext);
void logSASGameRecordAITributeRequest(CvPlayerAI const& kPlayer, PlayerTypes eHuman, SASGameRecordAITributeRequestContext const& kContext);
// <!-- custom: CONTACT_GIVE_HELP can be produced by three different chooser algorithms that revision 100's final package cannot distinguish: relation-based resource aid, relation-based technology aid, and ordinary proactive technology aid.
// Keep the origin vocabulary recorder-local and pass only already-selected live chooser/gate values; callers pre-gate at level 2+ and never rerun trade valuation or synchronized RNG for recording. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_GIVE_HELP_ORIGIN_ROWS(DO) \
	DO(SAS_AI_GIVE_HELP_RELATION_RESOURCE, RELATION_RESOURCE) \
	DO(SAS_AI_GIVE_HELP_RELATION_TECH, RELATION_TECH) \
	DO(SAS_AI_GIVE_HELP_PROACTIVE_TECH, PROACTIVE_TECH)
enum SASGameRecordAIGiveHelpOrigin
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_GIVE_HELP_ORIGIN_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
void logSASGameRecordAIGiveHelpDecision(CvPlayerAI const& kPlayer, PlayerTypes eTarget, SASGameRecordAIGiveHelpOrigin eOrigin, BonusTypes eSelectedBonus, TechTypes eSelectedTech, int iSelectionScore, int iTechScoreRatioX1000 = -1, int iGiftProbX1000 = -1, int iContactProbMultX1000 = -1);
// <!-- custom: CONTACT_TRADE_TECH can resolve through the ordinary randomized tech proposal (directly acceptable or balanced by AI_counterPropose) or AdvC's separate research-progress tech-for-gold purchase.
// Revision 100 preserves the final package but cannot distinguish those live algorithms or recover their selected chooser values; callers log only realized contacts/deals and never repeat trade valuation or synchronized RNG. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_TECH_TRADE_ORIGIN_ROWS(DO) \
	DO(SAS_AI_TECH_TRADE_RANDOM_DIRECT, RANDOM_TECH_DIRECT) \
	DO(SAS_AI_TECH_TRADE_RANDOM_COUNTERPROPOSE, RANDOM_TECH_COUNTERPROPOSE) \
	DO(SAS_AI_TECH_TRADE_PROGRESS_GOLD, PROGRESS_TECH_GOLD)
enum SASGameRecordAITechTradeOrigin
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_TECH_TRADE_ORIGIN_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
void logSASGameRecordAITechTradeDecision(CvPlayerAI const& kPlayer, PlayerTypes eTarget, SASGameRecordAITechTradeOrigin eOrigin, int iContactProbMultX1000, int iBestKnownTechScorePercent, TechTypes eRandomReceiveTech, int iRandomReceiveScore, bool bRandomReceiveLocusSuppressed, TechTypes eProgressTech, int iProgressResearchPoints, bool bProgressLocusSuppressed, TechTypes eGiveTech, int iOurReceiveTechValue, int iTargetReceiveTechValue, int iGiveMatchDeltaValue, int iProgressReceiveTechValue, int iProgressMaxGold);
// <!-- custom: DIPLO_DEAL_ENDED preserves the final deal payload but not why AI_doDeals chose to end it; AI_checkCancel also discards its denial/resource trigger after returning.
// Keep only realized cancellation causes and already-live trigger/overdraft context.
// Callers gate each row after the existing decision succeeds, so no offer valuation, denial query or synchronized RNG is repeated solely for recording. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_DEAL_CANCELLATION_REASON_ROWS(DO) \
	DO(SAS_AI_DEAL_CANCEL_OFFER_REJECTED_RENEGOTIATE, OFFER_REJECTED_RENEGOTIATE) \
	DO(SAS_AI_DEAL_CANCEL_DUAL_DENIAL, DUAL_DENIAL) \
	DO(SAS_AI_DEAL_CANCEL_AI_RESOURCE_DENIAL, AI_RESOURCE_DENIAL) \
	DO(SAS_AI_DEAL_CANCEL_OTHER_RESOURCE_JOKING, OTHER_RESOURCE_JOKING) \
	DO(SAS_AI_DEAL_CANCEL_GPT_LIMIT, GPT_LIMIT)
enum SASGameRecordAIDealCancellationReason
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_DEAL_CANCELLATION_REASON_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
void logSASGameRecordAIDealCancellationDecision(CvPlayerAI const& kPlayer, PlayerTypes eOther, CvDeal const& kDeal, SASGameRecordAIDealCancellationReason eReason, TradeData const* pTriggerItem = NULL, bool bTriggerFromAI = true, DenialTypes eDenial = NO_DENIAL, int iGptOverdraftBefore = -1, int iDealGpt = -1, int iGptOverdraftAfter = -1);
// <!-- custom: CvDeal::verify automatically tears down deals whose live validity has failed, but DIPLO_DEAL_ENDED cannot distinguish resource-supply/network/obsolescence failures, broke-GPT enforcement or normal peace-treaty expiry.
// Preserve only the first live failure already encountered by verify; the deal-end action remains authoritative for the payload and no validity query is repeated solely for recording. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_DEAL_INVALIDATION_REASON_ROWS(DO) \
	DO(SAS_DEAL_INVALID_RESOURCE_SUPPLY_DEFICIT, RESOURCE_SUPPLY_DEFICIT) \
	DO(SAS_DEAL_INVALID_TRADE_NETWORK_LOST, TRADE_NETWORK_LOST) \
	DO(SAS_DEAL_INVALID_GIVER_BONUS_OBSOLETE, GIVER_BONUS_OBSOLETE) \
	DO(SAS_DEAL_INVALID_RECIPIENT_BONUS_OBSOLETE, RECIPIENT_BONUS_OBSOLETE) \
	DO(SAS_DEAL_INVALID_GPT_INSOLVENT, GPT_INSOLVENT) \
	DO(SAS_DEAL_INVALID_PEACE_TREATY_EXPIRED, PEACE_TREATY_EXPIRED)
enum SASGameRecordDealInvalidationReason
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_DEAL_INVALIDATION_REASON_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
struct SASGameRecordDealInvalidationContext
{
	SASGameRecordDealInvalidationReason eReason;
	PlayerTypes eGiver;
	PlayerTypes eRecipient;
	TradeData const* pTriggerItem;
	int iTradeableBonusCount;
	int iGoldPercent;
	int iGold;
	int iGoldRate;
};
// <!-- custom: Capture only a realized first-failure boundary from CvDeal::verify; callers invoke this only with the level-2 context active, and the helper performs no gameplay query itself. (ChatGPT-5.6-Sol) -->
void setSASGameRecordDealInvalidationContext(SASGameRecordDealInvalidationContext* pContext, SASGameRecordDealInvalidationReason eReason, PlayerTypes eGiver, PlayerTypes eRecipient, TradeData const* pTriggerItem, int iTradeableBonusCount = -1, int iGoldPercent = -1, int iGold = -1, int iGoldRate = -1);
void logSASGameRecordDealInvalidation(CvDeal const& kDeal, SASGameRecordDealInvalidationContext const& kContext);
// <!-- custom: AI_proposeJointWar uses war-duration-adjusted contact cadence plus randomized target selection that the generic CONTACT_JOIN_WAR subject cannot reconstruct.
// Preserve only the two irrecoverable live chooser values; the recorder derives selected-target counters/static context after the successful gates and before diplomacy begins. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIJointWarRequest(CvPlayerAI const& kPlayer, PlayerTypes eHuman, TeamTypes eTarget, int iTargetScore, int iMeanAtWarTurnsX1000);
// <!-- custom: A realized embargo can originate from the ordinary STOP_TRADING contact path or from two distinct AI_proposeJointWar redirects; revision 100 preserves the final embargo subject but cannot reconstruct that origin.
// Keep the tiny origin vocabulary recorder-local and pair it with the already-selected joint-war target/denial context only when such a redirect actually succeeds. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_EMBARGO_REQUEST_ORIGIN_ROWS(DO) \
	DO(SAS_AI_EMBARGO_DIRECT_CONTACT, DIRECT_CONTACT) \
	DO(SAS_AI_EMBARGO_JOINT_WAR_ATTITUDE_REDIRECT, JOINT_WAR_ATTITUDE_REDIRECT) \
	DO(SAS_AI_EMBARGO_JOINT_WAR_UWAI_REDIRECT, JOINT_WAR_UWAI_REDIRECT)
enum SASGameRecordAIEmbargoRequestOrigin
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_EMBARGO_REQUEST_ORIGIN_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
void logSASGameRecordAIEmbargoRequest(CvPlayerAI const& kPlayer, PlayerTypes eHuman, SASGameRecordAIEmbargoRequestOrigin eOrigin, TeamTypes eJointWarTarget, int iJointWarTargetScore, DenialTypes eJointWarDenial);
// <!-- custom: AI-originated vassalage can come from ordinary voluntary diplomacy, UWAI seeking a third-party protector, or legacy/UWAI capitulation to the current enemy.
// The eventual TRADE_VASSAL/TRADE_SURRENDER action cannot reconstruct that decision origin; preserve only realized origin/cadence/package context while detailed UWAI capitulation/protector reasoning remains in BBAI. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_VASSALAGE_ORIGIN_ROWS(DO) \
	DO(SAS_AI_VASSALAGE_VOLUNTARY_CONTACT, VOLUNTARY_CONTACT) \
	DO(SAS_AI_VASSALAGE_UWAI_FIND_MASTER, UWAI_FIND_MASTER) \
	DO(SAS_AI_VASSALAGE_LEGACY_CAPITULATION, LEGACY_CAPITULATION) \
	DO(SAS_AI_VASSALAGE_UWAI_CAPITULATION, UWAI_CAPITULATION)
enum SASGameRecordAIVassalageOrigin
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_VASSALAGE_ORIGIN_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
void logSASGameRecordAIVassalageDecision(CvPlayerAI const& kPlayer, PlayerTypes eTarget, SASGameRecordAIVassalageOrigin eOrigin, TeamTypes eEnemyTeam, int iContactProbMultX1000, int iTargetRank, int iAliveCivs, int iCounterProposal, CLinkList<TradeData> const& kAIGives, CLinkList<TradeData> const& kAIReceives);
// <!-- custom: AI_doSplit commits one high-level colony decision whose pre-transfer aggregate area value is lost once splitEmpire moves the cities.
// Ordinary splitting requires positive value; the only live forced caller is UWAI's last pre-capitulation colony-liberation attempt.
// Keep deeper AI_splitEmpireValue scoring out of SASGameRecord. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIColonySplitDecision(CvPlayerAI const& kPlayer, CvArea const& kArea, int iAreaValue, bool bForce);
// <!-- custom: AI_launch commits a rare high-level spaceship timing choice after its native rival-countdown scan.
// Preserve the realized full-success vs urgent-rival reason and nearest rival deadline before launch mutates team victory state; callers pre-gate at GameRecord level 2 and do not repeat the rival scan. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAISpaceshipLaunchDecision(CvPlayerAI const& kPlayer, VictoryTypes eVictory, int iLaunchSuccessPercent, TeamTypes eNearestRivalTeam, int iNearestRivalCountdown, bool bUrgentRival);
// <!-- custom: A realized proactive map swap is symmetric in the final package but not in the live decision: preserve both pre-exchange map values and the minimum target-value gate without recomputing AI_mapTradeVal after maps change.
// Callers pre-gate this realized-only bridge at GameRecord level 2 so disabled logging never enters the serializer; retain only the cheap NO_PLAYER safety before dereferencing the live diplomacy target. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIMapTradeDecision(CvPlayerAI const& kPlayer, PlayerTypes eTarget, int iOurReceiveValue, int iTargetReceiveValue, int iTargetReceiveMinExclusive);
// <!-- custom: A master's automatic vassal-resource chooser is not an ordinary negotiated bonus trade: it picks the highest live AI_bonusTradeVal among resources the vassal can provide while the master has none.
// Preserve the realized best/runner-up/candidate boundary before the one-sided deal or human tribute popup mutates/obscures that choice; callers pre-gate at GameRecord level 2. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIVassalResourceTributeDecision(CvPlayerAI const& kMaster, PlayerTypes eVassal, bool bHumanPopup, BonusTypes eSelectedBonus, int iSelectedValue, BonusTypes eRunnerUpBonus, int iRunnerUpValue, int iCandidateCount);
// <!-- custom: AI_proposeWarTrade has richer realized joint-war hiring provenance than the generic CONTACT_JOIN_WAR package can preserve.
// Keep only the two execution paths recorder-local; the generic contact row remains authoritative for the final trade items. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_WAR_TRADE_PAYMENT_PATH_ROWS(DO) \
	DO(SAS_AI_WAR_TRADE_TECH_GOLD, TECH_GOLD) \
	DO(SAS_AI_WAR_TRADE_CITY_COUNTERPROPOSE, CITY_COUNTERPROPOSE)
enum SASGameRecordAIWarTradePaymentPath
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_WAR_TRADE_PAYMENT_PATH_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
void logSASGameRecordAIWarTradeIntent(CvPlayerAI const& kPlayer, PlayerTypes eHireling, SASGameRecordAIWarTradePaymentPath ePaymentPath, bool bUWAI, int iMinAtWarCounter, int iOfferDenominator, TeamTypes eTargetTeam, int iUWAITargetValue, int iLegacyTargetScore, int iHirePrice, TechTypes eCandidateTech1, TechTypes eCandidateTech2, int iWarSuccessRating, int iCityId, int iCityFitness, int iFinalWarSideValue, int iFinalPaymentSideValue);
// <!-- custom: AI_proposeCityTrade does not map honestly onto ContactTypes: one realized proposal may be a free liberation, strategic gift, city swap or negotiated city-centered package.
// Keep that compact formation vocabulary recorder-local and log only after the live candidate/counterproposal path has actually formed a contact/deal.
// `initialValueGap` is our city's already-computed cede value minus theirs; callers pre-gate at level 2+. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_CITY_TRADE_FORMATION_ROWS(DO) \
	DO(SAS_AI_CITY_TRADE_FREE_LIBERATION, FREE_LIBERATION) \
	DO(SAS_AI_CITY_TRADE_FREE_CITY_TO_HUMAN, FREE_CITY_TO_HUMAN) \
	DO(SAS_AI_CITY_TRADE_OUR_SIDE_COUNTERPROPOSE, OUR_SIDE_COUNTERPROPOSE) \
	DO(SAS_AI_CITY_TRADE_TARGET_SIDE_COUNTERPROPOSE, TARGET_SIDE_COUNTERPROPOSE) \
	DO(SAS_AI_CITY_TRADE_SAME_TEAM_OVERRIDE, SAME_TEAM_OVERRIDE)
enum SASGameRecordAICityTradeFormation
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_CITY_TRADE_FORMATION_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
void logSASGameRecordAICityTradeIntent(CvPlayerAI const& kPlayer, PlayerTypes eTarget, SASGameRecordAICityTradeFormation eFormation, int iCandidateRank, int iCandidateCount, int iInitialValueGap, bool bInverseGapFallback, int iOurCityId, int iTheirCityId, bool bLiberation, bool bEvacuating, bool bSameTeam, bool bNegotiable, CLinkList<TradeData> const& kAIGives, CLinkList<TradeData> const& kAIReceives);
// <!-- custom: Preserve one resolved AI peace-negotiation boundary rather than copying the full UWAI/BBAI utility trace; callers pre-gate at level 2 so disabled/lower-detail runs do not build logging-only trade context. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIPeaceDecision(PlayerTypes ePlayer, PlayerTypes eOther, int iAtWarTurns, bool bUWAI, int iInitialOurBenefit, int iInitialTheirBenefit, int iFinalOurBenefit, int iFinalTheirBenefit, int iGiveGold, int iReceiveGold, TechTypes eGiveTech, TechTypes eReceiveTech, int iGiveCityId, int iReceiveCityId, bool bCounterProposal, char const* szOutcome, CLinkList<TradeData> const* pWeGive, CLinkList<TradeData> const* pTheyGive);
// <!-- custom: Preserve the real research/free-tech source and, for AI_bestTech, the already-computed selected/runner-up path values and optional level-3 candidate paths; caller and chooser gates ensure no technology valuation or chooser RNG is repeated for SASGameRecord. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIResearchDecision(CvPlayerAI const& kPlayer, char const* szKind, char const* szSource, TechTypes eRequestedTech, int iResearchDepth, PlayerTypes eCoordinatingPlayer, SASTechChoiceContext const* pChoice);
// <!-- custom: Civic provenance mirrors the live AI_doCivics hysteresis/revolution path rather than rerunning AI_bestCivic or AI_civicValue.
// Accepted candidates are level 2; rejected/wanted candidates are level 3.
// Final outcomes preserve the pending/final bundle and real wait/gold/revolution gate. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAICivicCandidate(PlayerTypes ePlayer, char const* szStatus, CivicOptionTypes eCivicOption, CivicTypes eOldCivic, CivicTypes eNewCivic, int iCurrentValue, int iBestValue, int iTestAnarchy, int iCurrentBundleAnarchy, int iThreshold, int iSlack, bool bPassPercent, bool bPassSlack, bool bFirstPass);
void logSASGameRecordAICivicOutcome(PlayerTypes ePlayer, char const* szOutcome, std::vector<std::pair<CivicTypes, CivicTypes> > const& aeChanges, int iAnarchyLength, int iCivicTimerAfter, TechTypes eResearch, int iResearchTurns, CivicTypes eWaitCivic, int iWaitValue, int iWaitCurrentValue, int iGoldNeeded, int iCanRevolution);
// <!-- custom: Proactive resource-trade provenance keeps only values and random draws already produced by AI_proposeResourceTrade; winner context is gathered later by the recorder only for an actual resolved proposal.
// This avoids adding recorder branches to hot AI_bonusTradeVal callers. (ChatGPT-5.6-Sol) -->
struct SASGameRecordBonusTradeSide
{
	// <!-- custom: Replaces CvPlayerAI's initSASGameRecordBonusTradeSide/considerSASGameRecordBonusTradeCandidate helpers; reset only inside the logging gate, preserving uninitialized scalar storage when disabled. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
	void reset();
	void considerCandidate(BonusTypes eBonus, int iBuyerTradeValue, int iSellerKeepValue, int iBias, int iGatePermille, int iRandom, int iScore);
	BonusTypes eBestBonus;
	BonusTypes eRunnerUpBonus;
	int iBestBuyerTradeValue;
	int iBestSellerKeepValue;
	int iBestBias;
	int iBestGatePermille;
	int iBestRandom;
	int iBestScore;
	int iRunnerUpBuyerTradeValue;
	int iRunnerUpSellerKeepValue;
	int iRunnerUpBias;
	int iRunnerUpGatePermille;
	int iRunnerUpRandom;
	int iRunnerUpScore;
	int iEvaluated;
	int iGatePassed;
	int iNoDenial;
};
void logSASGameRecordAIBonusTradeDecision(PlayerTypes ePlayer, PlayerTypes eOther, char const* szAnchor, SASGameRecordBonusTradeSide const& kReceive, SASGameRecordBonusTradeSide const& kGive, CLinkList<TradeData> const& kWeGive, CLinkList<TradeData> const& kTheyGive);
// <!-- custom: Bonus-tribute provenance preserves the existing randomized DEMAND_BONUS sort result, non-surplus adjustment context and final selected bundle/value; no bonus valuation or RNG is repeated for logging. (ChatGPT-5.6-Sol) -->
struct SASGameRecordBonusDemandContext
{
	BonusTypes eBestBonus;
	BonusTypes eRunnerUpBonus;
	int iBestSortValueX100;
	int iBestHumanTradeableCopies;
	int iBestNonSurplusSort;
	int iRunnerUpSortValueX100;
	int iRunnerUpHumanTradeableCopies;
	int iRunnerUpNonSurplusSort;
	int iCandidateCount;
	int iSelectedCount;
	int iSelectedTotalValueX100;
	int iMinValueX100;
	int iDealValue;
};
void logSASGameRecordAIBonusDemandDecision(PlayerTypes ePlayer, PlayerTypes eHuman, SASGameRecordBonusDemandContext const& kContext, CLinkList<TradeData> const& kHumanGives);
// <!-- custom: Preserve realized AI corporation operational intent when a city commits to an Executive, an Executive assigns/retargets a spread city, or an Executive is deliberately rerouted by air/sea transport.
// Callers pre-gate at level 2 and pass only values already computed by the live production/target chooser; periodic posture separately preserves long-lived Executive missions across loaded saves. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIExecutiveProduction(CvCity const* pCity, UnitTypes eUnit, int iExecutiveValue, int iThreshold, char const* szStage);
void logSASGameRecordAICorporationTarget(CvUnit const* pUnit, CorporationTypes eCorporation, PlayerTypes ePreferredPlayer, int iExecutiveSpreadValue, CvCity const* pTargetCity, int iHqBaseValue, int iLocalCorporationValue, int iCompetingCorporationAdjustment, int iPopulationBonus, int iPreferredPlayerMultiplier, int iPathTurns, int iTargetScore, char const* szAction);
void logSASGameRecordAICorporationTransit(CvUnit const* pExecutive, CvUnit const* pTransport, CorporationTypes eCorporation, int iEligibleCorporations, CvCity const* pTargetCity, CvPlot const* pMovePlot, int iPathTurns, int iTargetScore, char const* szRoute);
// <!-- custom: Preserve only a realized/new Missionary religion-spread destination or airlift reroute.
// The live chooser supplies its already-selected religion/city/path/score; deeper player/city multiplier composition remains UnitAI/BBAI territory.
// Callers pre-gate at GameRecord level 2. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIReligionSpreadTarget(CvUnit const* pUnit, ReligionTypes eReligion, CvCity const* pTargetCity, int iPlayerMultiplierPercent, int iPathTurns, int iTargetScore, char const* szAction);
// <!-- custom: Preserve one meaningful AI religion-switch decision from the live chooser/roll without reevaluating religion values; level-3 callers may additionally pass the candidate scores already computed by AI_bestReligion. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIReligionDecision(PlayerTypes ePlayer, ReligionTypes eCurrentReligion, ReligionTypes eEvaluatedBest, ReligionTypes eSelectedReligion, ReligionTypes eRunnerUp, int iBestValue, int iRunnerUpValue, int iCurrentScore, int iCurrentRawValue, int iSelectedRawValue, int iConvertProbabilityPercent, int iRollSuccess, char const* szOutcome, std::vector<std::pair<ReligionTypes, int> > const* paCandidateValues);
// <!-- custom: Closed recorder-only causes for actual AI AP/UN ballots.
// They classify the live AI_diploVote decision path rather than gameplay-wide vote enums, so keep them local to SASGameRecord. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_DIPLO_VOTE_REASON_ROWS(DO) \
	DO(SAS_AI_DIPLO_VOTE_TEAM_SELF_ELIGIBLE, TEAM_SELF_ELIGIBLE) \
	DO(SAS_AI_DIPLO_VOTE_TEAM_VASSAL_MASTER, TEAM_VASSAL_MASTER) \
	DO(SAS_AI_DIPLO_VOTE_TEAM_OWN_DIPLO_VICTORY_ABSTAIN, TEAM_OWN_DIPLO_VICTORY_ABSTAIN) \
	DO(SAS_AI_DIPLO_VOTE_TEAM_ATTITUDE_TIE_ABSTAIN, TEAM_ATTITUDE_TIE_ABSTAIN) \
	DO(SAS_AI_DIPLO_VOTE_TEAM_BEST_ATTITUDE, TEAM_BEST_ATTITUDE) \
	DO(SAS_AI_DIPLO_VOTE_SECRETARY_SELF_OR_MASTER, SECRETARY_SELF_OR_MASTER) \
	DO(SAS_AI_DIPLO_VOTE_FRIENDLY_SECRETARY, FRIENDLY_SECRETARY) \
	DO(SAS_AI_DIPLO_VOTE_FORCE_CIVIC, FORCE_CIVIC) \
	DO(SAS_AI_DIPLO_VOTE_TRADE_ROUTES, TRADE_ROUTES) \
	DO(SAS_AI_DIPLO_VOTE_NO_NUKES, NO_NUKES) \
	DO(SAS_AI_DIPLO_VOTE_FREE_TRADE, FREE_TRADE) \
	DO(SAS_AI_DIPLO_VOTE_OPEN_BORDERS, OPEN_BORDERS) \
	DO(SAS_AI_DIPLO_VOTE_DEFENSIVE_PACT, DEFENSIVE_PACT) \
	DO(SAS_AI_DIPLO_VOTE_FORCE_PEACE, FORCE_PEACE) \
	DO(SAS_AI_DIPLO_VOTE_EMBARGO_RECENT_DEAL, EMBARGO_RECENT_DEAL) \
	DO(SAS_AI_DIPLO_VOTE_EMBARGO_UNMET_ABSTAIN, EMBARGO_UNMET_ABSTAIN) \
	DO(SAS_AI_DIPLO_VOTE_EMBARGO, EMBARGO) \
	DO(SAS_AI_DIPLO_VOTE_FORCE_WAR, FORCE_WAR) \
	DO(SAS_AI_DIPLO_VOTE_ASSIGN_CITY, ASSIGN_CITY) \
	DO(SAS_AI_DIPLO_VOTE_DEFAULT, DEFAULT)
enum SASGameRecordAIDiploVoteReason
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_DIPLO_VOTE_REASON_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
// <!-- custom: Diplomatic vote-source elections and resolutions (the Apostolic Palace and United Nations in standard BtS) are rare, high-impact factual boundaries.
// Preserve the AI secretary's selected proposal and each real AI ballot cause alongside the existing triggered-vote/result history; caller-gate at level 2 and reuse live decision values/RNG results rather than reevaluating. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIElectionChoice(CvTeamAI const& kTeam, VoteSelectionData const& kSelection, int iSelectedIndex, int iSelectedValue, int iSelectedRandomValue, int iSelectedVictoryBoost, int iValidOptions, int iRejectedOptions, int iVictoryBoostedOptions);
void logSASGameRecordAIDiploVoteDecision(CvPlayerAI const& kPlayer, VoteSelectionSubData const& kVoteData, VoteSourceTypes eVoteSource, int iTriggeredVoteId, PlayerVoteTypes eChoice, SASGameRecordAIDiploVoteReason eReason);
void logSASGameRecordAIDiploVoteDecision(CvPlayerAI const& kPlayer, VoteSelectionSubData const& kVoteData, VoteSourceTypes eVoteSource, int iTriggeredVoteId, PlayerVoteTypes eChoice, SASGameRecordAIDiploVoteReason eReason, int iDecisionValue, int iDecisionThreshold, int iRandomRoll);
void logSASGameRecordVoteTriggered(VoteTriggeredData const* pVoteTriggered);
void logSASGameRecordVoteResult(VoteTriggeredData const* pVoteTriggered, bool bThresholdPassed, bool bPassed, bool bCancelled, qword uiDefaultedAbstain, qword uiDefiers, qword uiEndorsers);
void logSASGameRecordReligionFounded(ReligionTypes eReligion, PlayerTypes ePlayer);
void logSASGameRecordCorporationFounded(CorporationTypes eCorporation, PlayerTypes ePlayer);
// <!-- custom: Closed recorder-only triggers for a realized AI city draft.
// NONE is an internal assignment sentinel; current ordinary AI_doDraft can realize TURTLE_STRATEGY, LOCAL_DANGER or NONCRITICAL_RANDOM_VALUE, while FORCED_CALLER preserves the existing bForce API exit without running the ordinary chooser. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_DRAFT_REASON_ROWS(DO) \
	DO(SAS_AI_DRAFT_FORCED_CALLER, FORCED_CALLER) \
	DO(SAS_AI_DRAFT_TURTLE_STRATEGY, TURTLE_STRATEGY) \
	DO(SAS_AI_DRAFT_LOCAL_DANGER, LOCAL_DANGER) \
	DO(SAS_AI_DRAFT_NONCRITICAL_RANDOM_VALUE, NONCRITICAL_RANDOM_VALUE)
enum SASGameRecordAIDraftReason
{
	SAS_AI_DRAFT_NONE,
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_DRAFT_REASON_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
// <!-- custom: Preserve only the realized AI_doDraft trigger and already-computed enabling context that the following conscription mutation cannot reconstruct; callers pre-gate at level 2 immediately before conscript(), so rejected/no-draft city turns add no recorder-only scans, valuations or RNG. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIDraftDecision(CvCity const& kCity, SASGameRecordAIDraftReason eReason, UnitTypes eConscriptUnit, int iConscriptPopulation, int iDanger, int iLandWar, int iGoodValue, int iTooMuchPop, int iPoorPlots, int iHappyDiff, int iUnitCostPerMil, int iLocalDefense, int iLocalEnemyOffense, int iBuildUnitProb);
// <!-- custom: Closed recorder-only reasons for a realized AI/automated-governor city hurry.
// Keep the choice vocabulary beside its serializer rather than exposing diagnostic-only causes through gameplay enums. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_HURRY_REASON_ROWS(DO) \
	DO(SAS_AI_HURRY_FORCED_PANIC, FORCED_PANIC) \
	DO(SAS_AI_HURRY_UNHAPPINESS_RELIEF, UNHAPPINESS_RELIEF) \
	DO(SAS_AI_HURRY_FOOD_LOSS_RELIEF, FOOD_LOSS_RELIEF) \
	DO(SAS_AI_HURRY_UNIT_VALUE, UNIT_VALUE) \
	DO(SAS_AI_HURRY_BUILDING_VALUE, BUILDING_VALUE)
enum SASGameRecordAIHurryReason
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_HURRY_REASON_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
};
// <!-- custom: Preserve only the live realized AI_doHurry comparison that the following CITY_HURRIED mutation cannot reconstruct; callers pre-gate at level 2 immediately before hurry(), so no logging-only hurry valuation or city-state lookup is paid on ordinary no-hurry turns. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIHurryDecision(CvCity const& kCity, HurryTypes eHurry, SASGameRecordAIHurryReason eReason, UnitAITypes eUnitAI, int iHappyBalance, int iHappyDiff, int iFoodDifference, int iPopCost, int iGoldCost, int iDecisionValue, int iOverflowValue, int iDecisionMargin);
// <!-- custom: Closed recorder-only origins distinguish the inherited helper from the few realized AI building commitments that bypass it.
// This keeps broad production evidence complete without copying rejected-candidate BBAI traces. (GPT-5.6-Sol) -->
#define SAS_RECORD_AI_BUILDING_CHOICE_ORIGIN_ROWS(DO) \
	DO(SAS_AI_BUILDING_CHOICE_HELPER, HELPER) \
	DO(SAS_AI_BUILDING_CHOICE_BORDER_CULTURE_SWIFT, BORDER_CULTURE_SWIFT) \
	DO(SAS_AI_BUILDING_CHOICE_BORDER_CULTURE_SLOW, BORDER_CULTURE_SLOW) \
	DO(SAS_AI_BUILDING_CHOICE_PROACTIVE_FORTIFICATION, PROACTIVE_FORTIFICATION) \
	DO(SAS_AI_BUILDING_CHOICE_OPPORTUNISTIC_WONDER, OPPORTUNISTIC_WONDER)
enum SASGameRecordAIBuildingChoiceOrigin
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_BUILDING_CHOICE_ORIGIN_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
	NUM_SAS_AI_BUILDING_CHOICE_ORIGINS
};
// <!-- custom: Overlapping effect families summarize why realized building commitments matter without widening GAME_RECORD_PRODUCTION_FLOW by one scalar per future building mechanic; a single choice may increment several families. (ChatGPT-5.6-Sol) -->
#define SAS_RECORD_AI_BUILDING_CHOICE_EFFECT_ROWS(DO) \
	DO(SAS_AI_BUILDING_EFFECT_HEALTH_FOOD_RELIEF, HEALTH_FOOD_RELIEF) \
	DO(SAS_AI_BUILDING_EFFECT_STARVATION_PREVENTED_BY_HEALTH, STARVATION_PREVENTED_BY_HEALTH) \
	DO(SAS_AI_BUILDING_EFFECT_HAPPINESS_RELIEF, HAPPINESS_RELIEF) \
	DO(SAS_AI_BUILDING_EFFECT_FOOD_KEPT, FOOD_KEPT) \
	DO(SAS_AI_BUILDING_EFFECT_MAINTENANCE_REDUCTION, MAINTENANCE_REDUCTION) \
	DO(SAS_AI_BUILDING_EFFECT_TRADE, TRADE) \
	DO(SAS_AI_BUILDING_EFFECT_PRODUCTION, PRODUCTION) \
	DO(SAS_AI_BUILDING_EFFECT_GOVERNMENT_CENTER, GOVERNMENT_CENTER) \
	DO(SAS_AI_BUILDING_EFFECT_CAPITAL, CAPITAL) \
	DO(SAS_AI_BUILDING_EFFECT_NATIONAL_PARK_STYLE, NATIONAL_PARK_STYLE) \
	DO(SAS_AI_BUILDING_EFFECT_DEFENSE, DEFENSE) \
	DO(SAS_AI_BUILDING_EFFECT_MILITARY_PRODUCTION, MILITARY_PRODUCTION) \
	DO(SAS_AI_BUILDING_EFFECT_DOMAIN_PRODUCTION, DOMAIN_PRODUCTION) \
	DO(SAS_AI_BUILDING_EFFECT_FREE_EXPERIENCE, FREE_EXPERIENCE)
enum SASGameRecordAIBuildingChoiceEffect
{
#define SAS_RECORD_ENUM(NAME, LABEL) NAME,
	SAS_RECORD_AI_BUILDING_CHOICE_EFFECT_ROWS(SAS_RECORD_ENUM)
#undef SAS_RECORD_ENUM
	NUM_SAS_AI_BUILDING_CHOICE_EFFECTS
};
// <!-- custom: Caller contract: pre-gate every realized choice at SASGameRecord level 2+, pass bDetailed only from a caller-side level-3 gate, and build szFocus only when bDetailed is true.
// The emitter intentionally performs no hidden logging-level check/assert, so ordinary disabled paths remain grep-visible and cheap; only invariant assertions remain.
// Helper-only max-turn/threshold/odds fields use -1 on direct chooser origins. (ChatGPT-5.6-Sol) -->
void logSASGameRecordAIBuildingChoice(CvCityAI const& kCity, BuildingTypes eBuilding, SASGameRecordAIBuildingChoiceOrigin eOrigin, int iFocusFlags, bool bDetailed, char const* szFocus, int iHelperMaxTurns, int iHelperMinThreshold, int iDecisionValue, int iHelperBaseOdds, int iHelperRandomRoll);
// <!-- custom: Preserve consequential factual city/unit actions that otherwise disappear between periodic snapshots: hurrying, natural growth/starvation, culture expansion, pillage attribution/economic gain, naval blockade lifecycle/plunder, unit gifting, religion/corporation membership changes, and circumnavigation.
// These hooks record only realized gameplay outcomes at level 2+; routine natural population changes are interval-compacted unless level 3 requests exact city transitions.
// Specialized SASGameRecord rows may preserve compact realized AI provenance, while rejected/speculative chooser detail remains in BBAI diagnostics. (ChatGPT-5.6-Sol) -->
void logSASGameRecordCityHurry(CvCity const* pCity, HurryTypes eHurry, int iProductionBefore, int iProductionAdded, int iGoldCost, int iPopulationCost, int iHurryAngerAdded, int iGoldBefore, int iPopulationBefore, int iHurryAngerBefore);
void logSASGameRecordCityGrowthPrevented(CvCity const* pCity, int iFoodDiscarded);
void logSASGameRecordCityPopulationChanged(CvCity const* pCity, bool bGrowth, int iPopulationBefore, int iFoodDifference, int iFoodBefore, int iFoodAfterDifference, int iFoodKeptBefore, int iFoodKeptBeforePopulationChange, int iGrowthThresholdBefore);
void logSASGameRecordCityCultureExpanded(CvCity const* pCity);
void logSASGameRecordPillage(CvUnit const* pUnit, SASGameRecordPlotState const& kOldPlotState, PlayerTypes eVictimPlayer, int iGoldGained);
void logSASGameRecordBlockadeChanged(CvUnit const* pUnit, bool bStarting);
void logSASGameRecordBlockadePlunder(CvUnit const* pUnit, CvCity const* pCity, int iGold, int iTradeRoutes, int iProfitPerRoute);
void logSASGameRecordUnitGifted(CvUnit const* pUnit, PlayerTypes eGiftingPlayer, CvPlot const* pPlotLocation);
void logSASGameRecordReligionChanged(ReligionTypes eReligion, PlayerTypes ePlayer, CvCity const* pCity, bool bAdded);
// <!-- custom: Membership-change rows say what ultimately changed; these compact attempt rows preserve the consumed Missionary/Executive, exact chance and failed/displacing outcomes that otherwise vanish.
// Callers pre-gate level 2+. (ChatGPT-5.6-Sol) -->
void logSASGameRecordReligionSpreadAttempt(CvUnit const* pUnit, ReligionTypes eReligion, CvCity const* pCity, int iDirectSpreadChance, bool bSuccess, ReligionTypes eDisplacedReligion);
void logSASGameRecordCorporationChanged(CorporationTypes eCorporation, PlayerTypes ePlayer, CvCity const* pCity, bool bAdded);
void logSASGameRecordCorporationSpreadAttempt(CvUnit const* pUnit, CorporationTypes eCorporation, CvCity const* pCity, int iSpreadChance, int iGoldCost, int iGoldBefore, bool bSuccess);
void logSASGameRecordCircumnavigated(TeamTypes eTeam, int iFreeSeaMoves, bool bBonusApplied, int iSeaExtraMovesBefore, int iSeaExtraMovesAfter);
// <!-- custom: Financial strike is a rare player-wide economic crisis; periodic player rows preserve current/cumulative state while this level-2 action records each realized strike turn and any resulting forced unit loss.
// Callers pre-gate before collecting logging-only unit counts. (ChatGPT-5.6-Sol) -->
void logSASGameRecordFinancialStrikeTurn(PlayerTypes ePlayer, int iGoldBefore, int iCalculatedGoldRate, int iGoldAfterClamp, int iCumulativeStrikeTurns, int iUnitsBeforeDisband, int iUnitsAfterDisband);
void logSASGameRecordGoldenAge(PlayerTypes ePlayer, bool bStart);
void logSASGameRecordGoldenAgeTurnsChanged(PlayerTypes ePlayer, int iChange, int iOldGoldenAgeTurns, int iNewGoldenAgeTurns);
void logSASGameRecordAnarchy(PlayerTypes ePlayer, bool bStart);
void logSASGameRecordCivicChanged(PlayerTypes ePlayer, CivicOptionTypes eCivicOption, CivicTypes eOldCivic, CivicTypes eNewCivic, ReligionTypes eOldEffectiveStateReligion, ReligionTypes eNewEffectiveStateReligion);
void logSASGameRecordLastStateReligionChanged(PlayerTypes ePlayer, ReligionTypes eOldReligion, ReligionTypes eNewReligion);
void logSASGameRecordBuildingCompletedByProduction(CvCity const* pCity, BuildingTypes eBuilding, int iRawModifiedOverflow, int iUnmodifiedOverflow, int iKeptOverflow, int iLostProduction, int iUnusedOverflowCapacity, int iOverflowGold);
void logSASGameRecordBuildingBuilt(CvCity const* pCity, BuildingTypes eBuilding);
void logSASGameRecordProjectBuilt(CvCity const* pCity, ProjectTypes eProject, int iRawModifiedOverflow, int iUnmodifiedOverflow, int iKeptOverflow, int iLostProduction, int iUnusedOverflowCapacity, int iOverflowGold);
// <!-- custom: City snapshots expose stored overflow and interval production-flow rows summarize it.
// Level-3 production-completion rows additionally preserve the exact overflow result of each completed unit/building/project; the standalone hook remains for aggregation plus lower-level exceptional/Barbarian rows. (ChatGPT-5.6-Sol) -->
void logSASGameRecordProductionOverflow(CvCity const* pCity, int iRawModifiedOverflow, int iUnmodifiedOverflow, int iKeptOverflow, int iLostProduction, int iUnusedCapacity, int iGold);
void logSASGameRecordProductionFailed(CvCity const* pCity, int iOrderData, bool bProject, int iInvestedProduction, int iGold);
// <!-- custom: Production lifecycle diagnostics distinguish strategic target switching from actual mechanical loss.
// Decay is a real loss path; obsolete-unit production transfer preserves stored production under a new unit type.
// Call only at level 2+. (ChatGPT-5.6-Sol) -->
void logSASGameRecordProductionDecay(CvCity const* pCity, OrderTypes eOrder, int iData1, int iBefore, int iAfter, int iInactiveTurns);
void logSASGameRecordProductionInvalidated(CvCity const* pCity, OrderTypes eOrder, int iData1, int iStoredLost, bool bActiveTarget, bool bQueued);
void logSASGameRecordProductionUpgraded(CvCity const* pCity, UnitTypes eOldUnit, UnitTypes eNewUnit, int iProductionTransferred, int iDestinationProductionBefore);
void logSASGameRecordCityProductionNoTarget(CvCity const& kCity, char const* szPhase);
// <!-- custom: Added the victory type so SASGameRecord can distinguish an actual spaceship launch from ordinary project completion and record its countdown. (GPT-5.6-Sol) -->
void logSASGameRecordVictoryLaunched(PlayerTypes ePlayer, VictoryTypes eVictory);
// <!-- custom: Capital loss and a failed arrival roll both call resetVictoryProgress, which otherwise silently erases the active countdown and spaceship projects.
// These hooks preserve the distinct cause and exact pre-reset state. (GPT-5.6-Sol) -->
void logSASGameRecordVictoryProgressResetForCapital(CvCity const* pCapital);
void logSASGameRecordSpaceshipFailed(TeamTypes eTeam, VictoryTypes eVictory, int iLaunchSuccessPercent);
void logSASGameRecordVassalState(TeamTypes eMaster, TeamTypes eVassal, bool bVassal);
void logSASGameRecordVictory(TeamTypes eWinner, VictoryTypes eVictory);
void logSASGameRecordRunStatus(char const* szReason);
void logSASGameRecordPlayerEliminated(PlayerTypes ePlayer);
void logSASGameRecordPlayerAliveChanged(PlayerTypes ePlayer, bool bRevived);
// <!-- custom: Record successful game-level Debug-mode toggles at their authoritative DLL boundary. (ChatGPT-5.6-Sol) -->
void logSASGameRecordDebugModeChanged(bool bOldDebugMode, bool bNewDebugMode);
// <!-- custom: Added eEndCause for explicit completion attribution; active-player changes update per-request and per-log-session control-transfer counts; callers remain logging-gated. See KI#203. (GPT-5.6-Sol) -->
void logSASGameRecordAutoPlayChanged(int iOldValue, int iNewValue, bool bChangePlayerStatus, SASAutoPlayEndCause eEndCause);
void logSASGameRecordActivePlayerChanged(PlayerTypes eOldPlayer, PlayerTypes eNewPlayer);
void logSASGameRecordAutoPlayPopupDismissed(char const* szPopupKind);
void logSASGameRecordGreatPersonBorn(CvUnit const* pUnit, PlayerTypes ePlayer, CvCity const* pCity);
void logSASGameRecordGreatPersonJoined(CvUnit const* pUnit, CvCity const* pCity, SpecialistTypes eSpecialist);
void logSASGameRecordGreatPersonConstructed(CvUnit const* pUnit, CvCity const* pCity, BuildingTypes eBuilding);
void logSASGameRecordGreatPersonDiscovered(CvUnit const* pUnit, TechTypes eTech, int iResearch);
void logSASGameRecordGreatPersonHurried(CvUnit const* pUnit, CvCity const* pCity, BuildingTypes eBuilding, int iProduction);
// <!-- custom: Realized Great-Merchant missions include the exact compact trade-profit anatomy so unusual payouts remain explainable without candidate dumps or replaying AI search; caller remains level-2 gated. (ChatGPT-5.6-Sol) -->
void logSASGameRecordGreatPersonTradeMission(CvUnit const* pUnit, CvCity const* pCity, int iGold);
void logSASGameRecordGreatPersonGreatWork(CvUnit const* pUnit, CvCity const* pCity, int iCulture);
void logSASGameRecordGreatPersonInfiltrated(CvUnit const* pUnit, CvCity const* pCity, int iEspionage);
void logSASGameRecordGreatPersonGoldenAgeConsumed(CvUnit const* pUnit);
// <!-- custom: Combat can supply its actual target because a dead attacker still reports its origin; other death paths retain the unit's current plot. See KI#377. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordGreatPersonDied(CvUnit const* pUnit, PlayerTypes eResponsiblePlayer, char const* szCause, CvPlot const* pDeathPlot = NULL);
void logSASGameRecordAIEspionageDecision(CvUnitAI const* pUnit, PlayerTypes eTargetPlayer, SASEspionageChoiceContext const& kChoice);
void logSASGameRecordAITacticalEspionageDecision(CvUnitAI const* pUnit, CvCity const* pCity, EspionageMissionTypes eMission);
void logSASGameRecordEspionageMission(CvUnit const* pUnit, EspionageMissionTypes eMission, PlayerTypes eTargetPlayer, CvPlot const* pPlot, int iExtraData, int iCost, int iEPBefore, int iEPAfter, ImprovementTypes eTargetImprovement, RouteTypes eTargetRoute, UnitTypes eTargetUnit, int iEffectValue, char const* szEffectKind);
void logSASGameRecordSpyInterceptionCheck(CvUnit const* pUnit, PlayerTypes eTargetPlayer, char const* szPhase, int iModifier, int iBaseInterceptPercent, int iInterceptChanceX100, int iInterceptRoll, bool bIntercepted, int iCounterespionageMod, bool bCounterSpyDefenseAtPlot, int iTargetSpiesOnPlot, int iTargetCounterSpyUnitsOnPlot, int iAttackerSpiesOnPlot, int iCityEspionageDefenseModifier, bool bRecentMissionBonusApplies, EspionageMissionTypes eMission, int iExtraData, ImprovementTypes eTargetImprovement, RouteTypes eTargetRoute, UnitTypes eTargetUnit);
void logSASGameRecordSpyIntercepted(CvUnit const* pUnit, PlayerTypes eTargetPlayer, char const* szPhase, int iModifier, int iInterceptChanceX100, EspionageMissionTypes eMission, int iExtraData, ImprovementTypes eTargetImprovement, RouteTypes eTargetRoute, UnitTypes eTargetUnit);
void logSASGameRecordGreatGeneralAttached(CvUnit const* pGreatGeneral, CvUnit const* pTargetUnit, PromotionTypes ePromotion);
void logSASGameRecordUnitScrapped(CvUnit const* pUnit);
void logSASGameRecordUnitUpgraded(CvUnit const* pOldUnit, CvUnit const* pNewUnit, int iCost);
void logSASGameRecordUnitCaptured(PlayerTypes eOldOwner, UnitTypes eOldUnitType, CvUnit const* pNewUnit);
// <!-- custom: Military-quality history records actual promotion selections and realized XP changes as compact interval/cumulative facts instead of inferring them from surviving units; callers pre-gate level 2+ so disabled/low-detail logging pays no recorder cost. (ChatGPT-5.6-Sol) -->
void logSASGameRecordUnitPromoted(CvUnit const* pUnit, PromotionTypes ePromotion);
void logSASGameRecordExperienceChange(CvUnit const* pUnit, int iAdjustedChange, int iActualChange, bool bFromCombat);
// <!-- custom: Bracket real combats only when SASGameRecord level 2+ needs outcome-quality history.
// The recorder keeps tiny transient attacker/defender context so delayed visible-combat callbacks retain the true attacker and pre-combat odds without adding gameplay/save state. (ChatGPT-5.6-Sol) -->
void noteSASGameRecordCombatStarted(CvUnit const* pAttacker, CvUnit const* pDefender, CvPlot const* pBattlePlot);
void logSASGameRecordNonlethalCombat(CvUnit const* pAttacker, CvUnit const* pDefender, CvPlot const* pBattlePlot, bool bCombatLimitReached);
void logSASGameRecordWarPlanChanged(TeamTypes eTeam, TeamTypes eTarget, WarPlanTypes eOldWarPlan, WarPlanTypes eNewWarPlan, bool bWar, int iOldStateCounter);
// <!-- custom: Level-3 tactical rows complement periodic city defense and unit-composition snapshots: synthesize consecutive siege/naval/air city bombardment into compact sequences while recording actual air strikes, interception combat, and air bombing of plot structures, so external analysis can distinguish merely owning those units from using them effectively. (GPT-5.6 Thinking) -->
void logSASGameRecordCityBombard(CvUnit const* pUnit, CvCity const* pCity, char const* szMode, int iBombardRate, bool bIgnoreBuildingDefense, int iDefenseModifierBefore, int iDefenseDamageBefore);
void logSASGameRecordAirStrike(CvUnit const* pUnit, CvUnit const* pDefender, int iDefenderDamageBefore, int iDefenderDamageAfter);
void logSASGameRecordAirInterception(CvUnit const* pAttacker, CvUnit const* pInterceptor, CvPlot const* pTargetPlot, int iAttackerDamageTaken, int iInterceptorDamageTaken);
void logSASGameRecordAirBombPlot(CvUnit const* pUnit, CvPlot const* pTargetPlot, char const* szTargetKind, char const* szTarget, bool bSuccess);
// <!-- custom: Record each actual nuke launch with its resolved interception result and pre-detonation target/affected-team context.
// pabAffectedTeams is the caller's MAX_TEAMS copy of CvUnit::nuke's already-computed victim map; call only at level 2+. (ChatGPT-5.6-Sol) -->
void logSASGameRecordNukeLaunched(CvUnit const* pUnit, CvPlot const* pTargetPlot, bool const* pabAffectedTeams, bool bIntercepted, TeamTypes eBestInterceptorTeam, int iInterceptionChance);
// <!-- custom: Record realized post-detonation damage totals already gathered by CvPlot::nukeExplosion; call only at level 2+. (ChatGPT-5.6-Sol) -->
void logSASGameRecordNukeEffects(CvUnit const* pUnit, CvPlot const* pTargetPlot, int iFalloutPlotsCreated, int iImprovementsDestroyed, int iFeaturesDestroyed, int iUnitsDamaged, int iUnitsKilled, int iBuildingsDestroyed, int iCitiesAffected, int iPopulationKilled);
// <!-- custom: Strategic per-city nuke consequences are level 2; exact affected-unit identities/damage are level 3 tactical detail; callers emit them from the existing explosion pass before destroyed objects disappear. (ChatGPT-5.6-Sol) -->
void logSASGameRecordNukeCityEffect(CvUnit const* pNukeUnit, CvCity const* pCity, int iPopulationBefore, int iNukeModifier, std::vector<BuildingTypes> const& aeBuildingsDestroyed);
void logSASGameRecordNukeUnitEffect(CvUnit const* pNukeUnit, CvUnit const* pAffectedUnit, CvPlot const* pPlot, int iDamageBefore, int iDamageAfter, bool bKilled, char const* szCause);
// <!-- custom: pBattlePlot is the actual target supplied by CvUnit; deriving it from pLoser is wrong when the attacker loses. See KI#377. (ChatGPT-5.6-Sol + GPT-5.6-Sol) -->
void logSASGameRecordCombatResult(CvUnit const* pWinner, CvUnit const* pLoser, CvPlot const* pBattlePlot);
void logSASGameRecordBonusChanged(CvPlot const* pPlot, BonusTypes eOldBonus, BonusTypes eNewBonus);

#define gGameRecordLogLevel g_iSASGameRecordLogLevel // <!-- custom: Startup-cached direct hot-path read; keep disabled SASGameRecord hooks free of accessor calls. (ChatGPT-5.5 + GPT-5.5 + ChatGPT-5.6-Sol) -->
#define gGameRecordTurnInterval getSASGameRecordTurnInterval() // <!-- custom: Periodic game-record snapshot interval in game turns. (ChatGPT-5.5) -->

void logSASGameRecord(TCHAR* format, ... );

#endif
